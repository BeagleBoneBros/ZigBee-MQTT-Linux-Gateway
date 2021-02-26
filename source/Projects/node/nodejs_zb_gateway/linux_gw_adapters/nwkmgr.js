/******************************************************************************

 @file nwkmgr.js

 @brief Defines the network management profile, encodes and decodes Protobuf 
 messages

 Group: WCS LPC

 ******************************************************************************
 $License: BSD3 2018 $

   Copyright (c) 2018, Texas Instruments Incorporated
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   *  Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   *  Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   *  Neither the name of Texas Instruments Incorporated nor the names of
      its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 $Release Name: ZStack Linux Gateway SDK ENG$
 $Release Date: May 07, 2018 $
 *****************************************************************************/

var Device = require('../devices/device.js');
var protobuf = require('protocol-buffers');
var fs = require("fs");
var net = require("net");
var events = require('events');
var ZBL = require('../zigbee_library/zb_lib.js');

//declare out side of the scope of the function 
//so it can be used for all instances  
var nwkmgrInstance;

var PKT_HEADER_SIZE = 4;
var PKT_HEADER_LEN_FIELD = 0;
var PKT_HEADER_SUBSYS_FIELD = 2;
var PKT_HEADER_CMDID_FIELD = 3;

var NWKMGR_PORT = 2540;
  
// Constructor
function Nwkmgr(ip_address) {
	
  //only create 1 nwkmgr instance
  if(typeof nwkmgrInstance !== "undefined")
  {
    return nwkmgrInstance;
  }
  nwkmgrInstance = this;
  
  events.EventEmitter.call(this);

  // initialize instance properties
  var nwkmgr_pb = protobuf(fs.readFileSync('linux_gw_adapters/nwkmgr.proto'));    
  var client = net.Socket();
  var responsetHndlQ = [];
  var txQue= [];  
  var gwInfo;

  client
    .connect(NWKMGR_PORT, ip_address, function() {
      console.log('nwkmgr connected');
  });
  client
  .on('data', function(data) {

    var dataIdx = 0;
    while(dataIdx < data.length)
    {
      var rx_pkt_len = data[dataIdx + PKT_HEADER_LEN_FIELD] + (data[dataIdx + PKT_HEADER_LEN_FIELD + 1] << 8) + PKT_HEADER_SIZE;
      var ByteBuffer = require("bytebuffer");
      var rx_pkt_buf = new ByteBuffer(rx_pkt_len, ByteBuffer.LITTLE_ENDIAN);

      rx_pkt_buf.append(data.slice(dataIdx, dataIdx + rx_pkt_len), "hex", 0);
      dataIdx = dataIdx + rx_pkt_len;

      var rx_cmd_id = rx_pkt_buf.readUint8(PKT_HEADER_CMDID_FIELD);
      
      switch(rx_cmd_id) { 
        // #################### Received Generic Confirmation #####################################
        case nwkmgr_pb.nwkMgrCmdId_t.ZIGBEE_GENERIC_CNF:        
          var genericCnf = nwkmgr_pb.NwkZigbeeGenericCnf.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          // console.log("nwkmgr genericCnf: ", genericCnf);
      
          //get the req in the queue which does not have a seqNum
          var requestHndl = getRspHndlr(-1);
          
          if(typeof requestHndl !== 'undefined')
          {
            if(genericCnf.status === nwkmgr_pb.nwkStatus_t.STATUS_SUCCESS)
            {              
              //request was confirmed, add seqNum so we can see the associated rsp.
              requestHndl.seqNum = genericCnf.sequenceNumber;
              //push back on the que
              putRspHndlr(requestHndl);
              //call the callback indicating the successfull confirmation of the request
              requestHndl.callback(genericCnf.status, genericCnf.sequenceNumber);            
            }
            else
            {
              //There was something wrong with the request, call the cb indicating an error	
              requestHndl.callback(genericCnf.status, -1);               	
            }
          }
          else
          {
            console.log("RspHndl for Seq ", genericCnf.sequenceNumber, "not found - status", genericCnf.status);
          }
                  
          //we got a confirmation for previous message, so send the next
          txNextMessage();
                                  
          break;
      
        // #################### Received Generic Response #########################################
        case nwkmgr_pb.nwkMgrCmdId_t.ZIGBEE_GENERIC_RSP_IND:
          var genericRspInd = nwkmgr_pb.NwkZigbeeGenericRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          console.log("nwkmgr: ZIGBEE_GENERIC_RSP_IND");
          
          var responseHndl = getRspHndlr(genericRspInd.sequenceNumber);
          
          if(typeof responseHndl !== 'undefined')
          {
            nwkmgrInstance.emit('gen-rsp', {
              status: genericRspInd.status,
              ieee: responseHndl.reqAddr.ieeeAddr,
              ep: responseHndl.reqAddr.endpointId,        	
            });                   
          }
          else
          {
            console.log("nwkmgr RspHndl for Seq ", genericRspInd.sequenceNumber, "not found");
          }
          break;

        // #################### Received Set Binding Response #####################################
        case nwkmgr_pb.nwkMgrCmdId_t.NWK_SET_BINDING_ENTRY_RSP_IND:
          
          // try to decode the incoming data packet
          try{
            console.log("NWKMGR: Got binding response.");
            bindingRspInd = nwkmgr_pb.NwkSetBindingEntryRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
            responseHndl = getRspHndlr(bindingRspInd.sequenceNumber);
          }
          // if unable to decode the packet, check to see if the packet contains another error code
          catch(err){
            console.log("ERROR: ", rx_pkt_buf);

            for(i = 0; i <= rx_pkt_len; i++){
              // check if the "No Entry" error code found
              if (rx_pkt_buf.readUint8() == 136){
                console.log("ERROR: Got No Entry");
                // return error in status
                nwkmgrInstance.emit('nwkmgr:binding:rsp', {
                  status: 136
                });
                break;
              }
            }

            // return error in status
            nwkmgrInstance.emit('nwkmgr:binding:rsp', {
              status: ZBL.STATUS.FAILURE,
              mode: bindingRspInd.bindingMode,
              dstIeee: responseHndl.reqAddr.ieeeAddr,
              dstEp: responseHndl.reqAddr.endpointId, 
              srcIeee: bindingRspInd.srcAddr.ieeeAddr,
              srcEp: bindingRspInd.srcAddr.endpointId    
            });
            break;
          }
          
          if(typeof responseHndl !== 'undefined')
          {
            nwkmgrInstance.emit('nwkmgr:binding:rsp', {
              status: bindingRspInd.status,
              dstIeee: responseHndl.reqAddr.ieeeAddr,
              dstEp: responseHndl.reqAddr.endpointId, 
              srcIeee: bindingRspInd.srcAddr.ieeeAddr,
              srcEp: bindingRspInd.srcAddr.endpointId,                   	
            });                   
          }
          else
          {
            console.log("nwkmgr RspHndl for Seq ", bindingRspInd.sequenceNumber, "not found");
          }
          break;
                                
        // #################### Received Get Device List Confirmation #############################
        case nwkmgr_pb.nwkMgrCmdId_t.NWK_GET_DEVICE_LIST_CNF:
          var devListCnfMsg = nwkmgr_pb.NwkGetDeviceListCnf.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          var devIdx;

          for(devIdx in devListCnfMsg.deviceList){
            for(var simpleDescDeviceIdx in devListCnfMsg.deviceList[devIdx].simpleDescList) {              
              ieee = devListCnfMsg.deviceList[devIdx].ieeeAddress;//.toString("hex");
              parentIeee = devListCnfMsg.deviceList[devIdx].parentIeeeAddress;//.toString("hex");
              simpleDesc = devListCnfMsg.deviceList[devIdx].simpleDescList[simpleDescDeviceIdx];
              
              // create new device
              newDev = Device(nwkmgrInstance, ieee, parentIeee, simpleDesc);
              
              if(typeof newDev !== 'undefined')
              {
                nwkmgrInstance.emit('nwkmgr:newDev', newDev);
              }
            }
          }
          break;

        // #################### Received New Zigbee Device on the Network #########################
        case nwkmgr_pb.nwkMgrCmdId_t.NWK_ZIGBEE_DEVICE_IND:                 
          var devInd = nwkmgr_pb.NwkZigbeeDeviceInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);

          // did it join the network
          if(devInd.deviceInfo.deviceStatus === nwkmgr_pb.nwkDeviceStatus_t.DEVICE_ON_LINE)
          {
            for(var simpleDescIdx in devInd.deviceInfo.simpleDescList) {                            
              ieee = devInd.deviceInfo.ieeeAddress;
              parentIeee = devInd.deviceInfo.parentIeeeAddress;
              simpleDesc = devInd.deviceInfo.simpleDescList[simpleDescIdx];

              newDev = Device(nwkmgrInstance, ieee, parentIeee, simpleDesc);

              if(typeof newDev !== 'undefined')
              {      
                nwkmgrInstance.emit('nwkmgr:newDev', newDev);
              }
            }
          }
          else if(devInd.deviceInfo.deviceStatus === nwkmgr_pb.nwkDeviceStatus_t.DEVICE_REMOVED)
          {        	 
            nwkmgrInstance.emit('nwkmgr:removeDev', devInd.deviceInfo.ieeeAddress);
          }
          break;

        // #################### Received Local Device Information #################################
        case nwkmgr_pb.nwkMgrCmdId_t.NWK_GET_LOCAL_DEVICE_INFO_CNF:        
          var gwInfoCnf = nwkmgr_pb.NwkGetLocalDeviceInfoCnf.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          gwInfo = gwInfoCnf.deviceInfoList;  
          break;

        // #################### Received Network Ready Indication #################################
        case nwkmgr_pb.nwkMgrCmdId_t.NWK_ZIGBEE_NWK_READY_IND:        
          console.log("NWK_ZIGBEE_NWK_READY_IND");   
          nwkmgrInstance.emit('nwkmgr:network:ready');
          break;

        // #################### Received Network Info Confirmation ################################
        case nwkmgr_pb.nwkMgrCmdId_t.NWK_ZIGBEE_NWK_INFO_CNF:        
          gwInfoCnf = nwkmgr_pb.NwkZigbeeNwkInfoCnf.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          console.log("nwkmgr: Network Info: ", gwInfoCnf);
          nwkmgrInstance.emit('nwkmgr:network:info', gwInfoCnf);
          break;                        

        default:
          console.log("nwkmgr: CmdId not processed: ", rx_cmd_id);
      }
    }
  })  
  .on('error', function(err){
    console.log("Error: ", err.message);
    clientReconnect();
  });
  
  //get gw info
  getGwInfoReq();    


  //----------------------------------------------------------------------------------------------------------
  // clientReconnect()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // periodically attempt to reconnect to the HaGateway server
  //----------------------------------------------------------------------------------------------------------
  function clientReconnect() { 	
  	if(typeof nwkmgrInstance.clientReconnectTimer === 'undefined')
  	{  	
  		  //start a connection timer that tries to reconnect 5s,
	    nwkmgrInstance.clientReconnectTimer = setTimeout(function(){ 
		    console.log("nwkmgr clientReconnectTimer: attempting to reconnect");  		    
        client.destroy();
        client
          .connect(NWKMGR_PORT, ip_address, function() {
            console.log('nwkmgr connected');
            nwkmgrInstance.emit('nwkmgr:network:ready');                       
          });
        clearTimeout(nwkmgrInstance.clientReconnectTimer);       
        delete nwkmgrInstance.clientReconnectTimer;  
		  }, 5000);
  	}  		  		  		
  }


  //----------------------------------------------------------------------------------------------------------
  // sendMessage()
  //    - Inputs: msg_buf = encoded protobuf
  //              gwAddress = object with:
  //                            - addressType (protobuf defined addressType)
  //                            - ieeeAddr (buffer)
  //                            - endpointId (integer)
  //              responseCb = callback function
  //    - Output: NULL
  //
  // packages a message to sent to the Linux gateway/ZNP device. Will add message to be sent to queue
  //----------------------------------------------------------------------------------------------------------
  function sendMessage(msg_buf, gwAddress, responseCb) {       
    var ByteBuffer = require("bytebuffer");       
    var pkt_buf = new ByteBuffer(PKT_HEADER_SIZE + msg_buf.length, ByteBuffer.LITTLE_ENDIAN)
           .writeShort(msg_buf.length, PKT_HEADER_LEN_FIELD)
           .writeUint8(nwkmgr_pb.zStackNwkMgrSysId_t.RPC_SYS_PB_NWK_MGR, PKT_HEADER_SUBSYS_FIELD)
           .writeUint8(msg_buf[1], PKT_HEADER_CMDID_FIELD)
           .append(msg_buf, "hex", PKT_HEADER_SIZE);
    
    var responseHndl;   
    if(typeof responseCb !== 'undefined')
    {
    	// console.log("sendMessage: responseHndl -1");
      responseHndl = {
      	callback: responseCb,
      	reqAddr: gwAddress,
      	seqNum: -1
      };	
    }
    else
    {
    	console.log("sendMessage: no responseHndl");
    }
    
    queTxMessage(pkt_buf, responseHndl);
  }


  //----------------------------------------------------------------------------------------------------------
  // queTxMessage()
  //    - Inputs: pkt_buf = buffer to be sent
  //              responseHndl = response handle with:
  //                              - callback (callback function)
  //                              - reqAddr (address of gateway being sent)
  //                              - seqNum (integer)
  //    - Output: NULL
  //
  // Will add message to queue. If queue is empty, will send to gateway 
  //----------------------------------------------------------------------------------------------------------
  function queTxMessage(pkt_buf, responseHndl) {   
    //if there is nothing in que then there 
  	//is no pending cnf, send now
    if(txQue.length === 0) {
    	if(typeof responseHndl !== 'undefined')
    	{
        //puh the rspHndle on the responsetHndlQ 
        putRspHndlr(responseHndl);
      }
    	// console.log("queTxMessage: sending message ");
      client.write(pkt_buf.buffer); 
    }
    else {
  	  //que the message
    	var txMsg = {
    	  responseHndl: responseHndl,
    	  pkt_buf: pkt_buf
      };
    
  	  // console.log("queTxMessage: queing message ");
  	  txQue.push(txMsg);
  	}
  }
    

  //----------------------------------------------------------------------------------------------------------
  // txNextMessage()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Will take the next message waiting in queue and will send to gateway 
  //----------------------------------------------------------------------------------------------------------
  function txNextMessage() {  	      
    // console.log("txNextMessage++")
    //get the tx message
    var txMsg = txQue.pop();
    
    //make sure there was a message in the que  
    if(typeof txMsg !== 'undefined') {
    	if(typeof responseHndl !== 'undefined')
    	{
    	  //puh the rspHndle on the responsetHndlQ 
    	  txMsg.responseHndl.id = txMsg.id;
        putRspHndlr(txMsg.responseHndl);
      }    	
    
    	// console.log("txNextMessage: sending message ", txMsg.id );
      client.write(txMsg.pkt_buf.buffer); 
    } 
  }
      

  //----------------------------------------------------------------------------------------------------------
  // getRspHndlr()
  //    - Inputs: sequenceNumber = integer value response handle 
  //    - Output: rsp = response handle object
  //
  // Return the appropriate response handle object given its sequence number
  //----------------------------------------------------------------------------------------------------------
  function getRspHndlr(sequenceNumber)
  {  	
    //get the queued respone handler
    if(responsetHndlQ.length > 0)    
    {
      //get response handler index for this seq num
      var rspHndlIdx;
      for(rspHndlIdx in responsetHndlQ)
      {
      	//console.log("getRspHndlr: searching", responsetHndlQ[rspHndlIdx]);
        if( responsetHndlQ[rspHndlIdx].seqNum === sequenceNumber) 
        { 	          	
          console.log("getRspHndlr found: ", sequenceNumber);
          //stop the timeout
          clearTimeout(responsetHndlQ[rspHndlIdx].timeout);
          var rsp = responsetHndlQ.splice(rspHndlIdx, 1)[0];
          return rsp;            
        }      
      }
    } 
  }
  

  //----------------------------------------------------------------------------------------------------------
  // putRspHndlr()
  //    - Inputs: responseHndl = response handle object to be stored 
  //    - Output: NULL
  //
  // Store the response handle to an array. Response handle will be appended to the array
  //----------------------------------------------------------------------------------------------------------
  function putRspHndlr(responseHndl)
  {
  	console.log("putRspHndlr[", responsetHndlQ.length, "]: ", responseHndl.id, ":", responseHndl.seqNum);  	
  	responseHndl.timeout = setTimeout(function(){ 
  		console.log("timeout: ", responseHndl.seqNum);
  		timeoutRspHndlr(responseHndl.seqNum);
  		}, 10000);
    responsetHndlQ.push(responseHndl);
      
  	//If response hndls are not removed something is seriously wrong
  	//The GW should always responed even if there is an error,
  	//However to protect the Q getting too long do a siple length check 
  	if(responsetHndlQ.length > 500)
  	{ 
  	  console.log("nwkmgr putRspHndlr: response Q overflow, removing oldest rspHndle");
  	  var responsetHndl = responsetHndlQ.splice(rspHndlIdx, 1)[0];  	
      responsetHndl.callback(nwkmgr_pb.nwkStatus_t.STATUS_TIMEOUT, responsetHndl);
    }
  }
 

  //---------------------------------------------------------------------------------------------------------
  // timeoutRspHndlr()
  //    - Inputs: seqNum = seuqnece number of a response handle
  //    - Output: NULL
  //
  // Create a timeout function based on the sequence number. Will be called if no response is received within 
  // timeout period.
  //---------------------------------------------------------------------------------------------------------
  function timeoutRspHndlr(seqNum)
  { 
    console.log("responseHndl: ", seqNum, " timed out"); 
    responseHndl = getRspHndlr(seqNum);
    responseHndl.callback(nwkmgr_pb.nwkStatus_t.STATUS_TIMEOUT, responseHndl);
  }
      

  //---------------------------------------------------------------------------------------------------------
  // getGwInfoReq()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Send a request to get the local gateway info
  //---------------------------------------------------------------------------------------------------------
  function getGwInfoReq() {         
    console.log("nwkmgr: getGwInfoReq");

    //create the message
    var msg_buf = nwkmgr_pb.NwkGetLocalDeviceInfoReq
      .encode({
        cmdId: nwkmgr_pb.nwkMgrCmdId_t.NWK_GET_LOCAL_DEVICE_INFO_REQ
      });
        
    sendMessage(msg_buf);      
  }


  //---------------------------------------------------------------------------------------------------------
  // getNetworkInfoReq()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Send a request to get the Zigbee Network Info
  //---------------------------------------------------------------------------------------------------------
  function getNetworkInfoReq() {         
    console.log("nwkmgr: getNetworkInfoReq");

    //create the message
    var msg_buf = nwkmgr_pb.NwkZigbeeNwkInfoReq
      .encode({
        cmdId: nwkmgr_pb.nwkMgrCmdId_t.NWK_ZIGBEE_NWK_INFO_REQ
      });
        
    sendMessage(msg_buf);      
  }

  //#########################################################################################################
  //################################## Network Manager Functions ############################################
  //#########################################################################################################

  //---------------------------------------------------------------------------------------------------------
  // getNetworkInfo()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Send a request to get the Zigbee Network Info
  //---------------------------------------------------------------------------------------------------------
  Nwkmgr.prototype.getNetworkInfo = function(){
    getNetworkInfoReq();
  };
     

  //---------------------------------------------------------------------------------------------------------
  // openNetwork()
  //    - Inputs: duration = integer, amount of time to keep the Network open
  //    - Output: NULL
  //
  // Send a request to set the Zigbee Network Join permission to open for a duration of time
  //---------------------------------------------------------------------------------------------------------
  Nwkmgr.prototype.openNetwork = function(duraion) {        
    console.log("nwkmgr: openNetwork");

    //create the message
    var msg_buf = nwkmgr_pb.NwkSetPermitJoinReq
      .encode({
        cmdId: nwkmgr_pb.nwkMgrCmdId_t.NWK_SET_PERMIT_JOIN_REQ,
        permitJoin: nwkmgr_pb.nwkPermitJoinType_t.PERMIT_ALL,
        permitJoinTime: duraion
      });
        
    sendMessage(msg_buf);      
  };


  //---------------------------------------------------------------------------------------------------------
  // getDevList()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Send a request to get a list of all devices on the Network
  //---------------------------------------------------------------------------------------------------------
  Nwkmgr.prototype.getDevList = function() {        
    //create the message
    var msg_buf = nwkmgr_pb.NwkGetDeviceListReq
      .encode({
        cmdId: nwkmgr_pb.nwkMgrCmdId_t.NWK_GET_DEVICE_LIST_REQ
      });
        
    sendMessage(msg_buf);      
  };


  //---------------------------------------------------------------------------------------------------------
  // setDevBinding()
  //    - Inputs: 
  //              - bindingMode = integer, determines whether ot bind or unbind devices. 0=>BIND, 1=>UNBIND
  //              - srcIeee = buffer, IEEE address of the source device to be bound 
  //              - srcEp = integer, endpoint of the source device to be bound
  //              - dstIeee = buffer, IEEE address of the destination device to be bound
  //              - dstEp = integer, endpoint of the destination device to be bound
  //              - clusterId = integer, cluster ID of the cluster that will be bound between the two devices. 
  //              - bindCnfCb = callback function
  //    - Output: NULL
  //
  // Send a request to bind or unbind two devices. 
  //---------------------------------------------------------------------------------------------------------
  Nwkmgr.prototype.setDevBinding = function(bindingMode, srcIeee, srcEp, dstIeee, dstEp, clusterId, bindCnfCb) {            
   
    var srcBindAddress = {
      addressType: nwkmgr_pb.nwkAddressType_t.UNICAST, // Address Mode
      ieeeAddr: srcIeee,  // extended address
      endpointId: srcEp   // endpoint 
    };
  
    var dstBindAddress = {
      addressType: nwkmgr_pb.nwkAddressType_t.UNICAST, // Address Mode
      ieeeAddr: dstIeee,  // extended address
      endpointId: dstEp   // endpoint 
    };        
    
    //create the message
    var msg_buf = nwkmgr_pb.NwkSetBindingEntryReq
      .encode({
      cmdId: nwkmgr_pb.nwkMgrCmdId_t.NWK_SET_BINDING_ENTRY_REQ,
      srcAddr: srcBindAddress,
      dstAddr: dstBindAddress,
      clusterId: clusterId,
      bindingMode: bindingMode //BIND = 0, UNBIND = 1  
    });
        
    sendMessage(msg_buf, dstBindAddress, bindCnfCb);      
  };  


  //---------------------------------------------------------------------------------------------------------
  // removeDev()
  //    - Inputs: 
  //              - ieee = buffer, IEEE address of the device to be removed from the network
  //    - Output: NULL
  //
  // Send a request to remove a device from the Zigbee Network based on the IEEE address of the device
  //---------------------------------------------------------------------------------------------------------
  Nwkmgr.prototype.removeDev = function(ieee) {            
    console.log("nwkmgr: removeDev ", ieee );
    
    var dstAddress = {
      addressType: nwkmgr_pb.nwkAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
    };
    
    //create the message
    var msg_buf = nwkmgr_pb.NwkRemoveDeviceReq
      .encode({
      cmdId: nwkmgr_pb.nwkMgrCmdId_t.NWK_REMOVE_DEVICE_REQ,
      dstAddr: dstAddress,
      leaveMode: nwkmgr_pb.nwkLeaveMode_t.LEAVE
    });
        
    sendMessage(msg_buf);      

  };

    
  //---------------------------------------------------------------------------------------------------------
  // getGwInfo()
  //    - Inputs: NULL
  //    - Output: gwInfo = an object containing the local gateway info
  //
  // Return the current gateway info
  //---------------------------------------------------------------------------------------------------------
  Nwkmgr.prototype.getGwInfo = function() {        
    //console.log("nwkmgr: getgwInfo: ", gwInfo);
    return gwInfo;    
  };

      
  //---------------------------------------------------------------------------------------------------------
  // hardSystemReset()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Send a request to perform a Hard System Reset of the Gateway. This will remove all devices from the network.
  //---------------------------------------------------------------------------------------------------------
  Nwkmgr.prototype.hardSystemReset = function() {        
    console.log("nwkmgr: hardSystemReset");

    //create the message
    var msg_buf = nwkmgr_pb.NwkZigbeeSystemResetReq.encode({
      cmdId: nwkmgr_pb.nwkMgrCmdId_t.NWK_ZIGBEE_SYSTEM_RESET_REQ,
      mode: nwkmgr_pb.nwkResetMode_t.HARD_RESET
    });
        
    sendMessage(msg_buf);      
  };


  //---------------------------------------------------------------------------------------------------------
  // softSystemReset()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Send a request to perform a Soft System Reset of the Gateway.
  //---------------------------------------------------------------------------------------------------------
  Nwkmgr.prototype.softSystemReset = function() {        
    console.log("nwkmgr: softSystemReset");

    //create the message
    var msg_buf = nwkmgr_pb.NwkZigbeeSystemResetReq.encode({
      cmdId: nwkmgr_pb.nwkMgrCmdId_t.NWK_ZIGBEE_SYSTEM_RESET_REQ,
      mode: nwkmgr_pb.nwkResetMode_t.SOFT_RESET
    });
        
    sendMessage(msg_buf);      
  };


  //---------------------------------------------------------------------------------------------------------
  // reconnectNwkmgr()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Periodically attempt to reconnect to the Network Manager server
  //---------------------------------------------------------------------------------------------------------
  Nwkmgr.prototype.reconnectNwkmgr = function(){
    clientReconnect();
  };
}

Nwkmgr.prototype.__proto__ = events.EventEmitter.prototype;
 
// export the class
module.exports = Nwkmgr;