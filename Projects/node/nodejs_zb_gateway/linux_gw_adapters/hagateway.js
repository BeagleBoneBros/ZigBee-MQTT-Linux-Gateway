/******************************************************************************

 @file hagateway.js

 @brief Defines the Home Automation profile, encodes and decodes Protobuf 
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

var protobuf = require("protocol-buffers");
var reverse = require("buffer-reverse");
var fs = require("fs");
var net = require("net");    
  
//declare out side of the scope of the function 
//so it can be used for all instances  
var hageatwayInstance;

var PKT_HEADER_SIZE = 4;
var PKT_HEADER_LEN_FIELD = 0;
var PKT_HEADER_SUBSYS_FIELD = 2;
var PKT_HEADER_CMDID_FIELD = 3;

var HAGATEAY_PORT = 2541;

var events = require('events');
  
// Constructor
function Hagateway(ip_address) {
	
  //only create 1 hagateway instance
  if(typeof hageatwayInstance !== "undefined")
  {
    return hageatwayInstance;
  }
  hageatwayInstance = this;
  
  
  events.EventEmitter.call(this);
  
  // initialize instance properties
  var hagateway_pb = protobuf(fs.readFileSync('linux_gw_adapters/gateway.proto'));  
  var client = net.Socket();
  var responsetHndlQ = []; 
  var txQue= [];

  client
    .connect(HAGATEAY_PORT, ip_address, function() {
      console.log('hagateway connected');
      hageatwayInstance.emit('connected');
  });
  
  client
  // incoming data/responses from ZNP
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
        case hagateway_pb.gwCmdId_t.ZIGBEE_GENERIC_CNF:
          var genericCnf = hagateway_pb.GwZigbeeGenericCnf.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          console.log("genericCnf: ", genericCnf);
          
          //get the first req in the queue which does not have a seqNum
          var requestHndl = getRspHndlr(-1);
          
          if(typeof requestHndl !== 'undefined')
          {
            if(genericCnf.status === hagateway_pb.gwStatus_t.STATUS_SUCCESS)
            {              
              //request was confirmed, add seqNum so we can see the associated rsp.
              requestHndl.seqNum = genericCnf.sequenceNumber;
              //push back on the que
              putRspHndlr(requestHndl);
              //call the callback indicating the scuccesfull confirmation of the request
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
          
        // #################### Received Generic Response Indicaiton ##############################
        case hagateway_pb.gwCmdId_t.ZIGBEE_GENERIC_RSP_IND:
          var gwZigbeeGenericRspInd = hagateway_pb.GwZigbeeGenericRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          console.log("hagateway: ZIGBEE_GENERIC_RSP_IND");
        
          var responseHndl = getRspHndlr(gwZigbeeGenericRspInd.sequenceNumber);
        
          if(typeof responseHndl !== 'undefined')
          {
            hageatwayInstance.emit('gen-rsp', {
              status: gwZigbeeGenericRspInd.status,
              ieee: responseHndl.reqAddr.ieeeAddr,
              ep: responseHndl.reqAddr.endpointId,        	
            });                   
          }
          else
          {
            console.log("RspHndl for Seq ", gwZigbeeGenericRspInd.sequenceNumber, "not found");
          }
          break;
                      
        // #################### Received Cluster Library Data Frame ###############################
        case hagateway_pb.gwCmdId_t.GW_ZCL_FRAME_RECEIVE_IND:
          var gwZclFrameReceiveInd = hagateway_pb.GwZclFrameReceiveInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          console.log("hagateway: GW_ZCL_FRAME_RECEIVE_IND");
            
          var zclHeader = {
            frameType: gwZclFrameReceiveInd.frameType,
            manufacturerSpecificFlag: gwZclFrameReceiveInd.manufacturerSpecificFlag,
            manufacturerCode: gwZclFrameReceiveInd.manufacturerCode,
            clientServerDirection: gwZclFrameReceiveInd.clientServerDirection,
            disableDefaultRsp: gwZclFrameReceiveInd.disableDefaultRsp,
            commandId: gwZclFrameReceiveInd.commandId   	
          };
          
          // generate the GUID
          var guid = reverse(gwZclFrameReceiveInd.srcAddress.ieeeAddr).toString('hex') + ":" + gwZclFrameReceiveInd.srcAddress.endpointId;

          // send the zclFrame to the devices
          hageatwayInstance.emit('hagateway:zcl-ind', {
            ieee: gwZclFrameReceiveInd.srcAddress.ieeeAddr,
            ep: gwZclFrameReceiveInd.srcAddress.endpointId,
            guid: guid,
            clusterId: gwZclFrameReceiveInd.clusterId, 
            zclHeader: zclHeader,         
            payload: gwZclFrameReceiveInd.payload            
          });

          break;

        // #################### Attribute Reporting Response ######################################
        case hagateway_pb.gwCmdId_t.GW_SET_ATTRIBUTE_REPORTING_RSP_IND:
          var getAttrReportRsp = hagateway_pb.GwSetAttributeReportingRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          console.log("hagateway: GW_SET_ATTRIBUTE_REPORTING_RSP_IND");
        
          responseHndl = getRspHndlr(getAttrReportRsp.sequenceNumber);
        
          if(typeof responseHndl !== 'undefined')
          {          	
            hageatwayInstance.emit('hagateway:attrreport:rsp', {
              status: getAttrReportRsp.status,
              seqNum: getAttrReportRsp.sequenceNumber,
            });
          }
          else
          {
            console.log("RspHndl for Seq ", getAttrReportRsp.sequenceNumber, "not found");
          }
          break; 
                        
        // #################### Light Device - Get On/Off State Repsonse ##########################
        case hagateway_pb.gwCmdId_t.DEV_GET_ONOFF_STATE_RSP_IND:
          var getDevStateRsp = hagateway_pb.DevGetOnOffStateRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          console.log("hagateway: DEV_GET_ONOFF_STATE_RSP_IND", getDevStateRsp);
        
          responseHndl = getRspHndlr(getDevStateRsp.sequenceNumber);
        
          if(typeof responseHndl !== 'undefined')
          {          	
            // if successful in getting state
            if(getDevStateRsp.status === hagateway_pb.gwStatus_t.STATUS_SUCCESS) 
            { 
              // set the current state
              on = getDevStateRsp.stateValue;
            }
            else
            {
              // error, set to a default state OFF
              on = 0;
            } 

            // generate the GUID
            guid = reverse(responseHndl.reqAddr.ieeeAddr).toString('hex') + ":" + responseHndl.reqAddr.endpointId;

            hageatwayInstance.emit('hagateway:light_device:state', {
              status: getDevStateRsp.status,
              seqNum: getDevStateRsp.sequenceNumber,
              guid: guid,
              ieee: responseHndl.reqAddr.ieeeAddr,
              ep: responseHndl.reqAddr.endpointId,
              on: on
            });
          }
          else
          {
            console.log("RspHndl for Seq ", getDevStateRsp.sequenceNumber, "not found");
          }
          break;                           

        // #################### Light Device - Get Level Response #################################
        case hagateway_pb.gwCmdId_t.DEV_GET_LEVEL_RSP_IND:
          var getDevLevelRsp = hagateway_pb.DevGetLevelRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
        
          responseHndl = getRspHndlr(getDevLevelRsp.sequenceNumber);
        
          if(typeof responseHndl !== 'undefined')
          {
            // if successful in getting level
            if(getDevLevelRsp.status === hagateway_pb.gwStatus_t.STATUS_SUCCESS) 
            {
              // set level
              level = getDevLevelRsp.levelValue;
            }
            else
            {
              // default the level to zero
              level = 0;
            } 

            // generate the GUID
            guid = reverse(responseHndl.reqAddr.ieeeAddr).toString('hex') + ":" + responseHndl.reqAddr.endpointId;

            hageatwayInstance.emit('hagateway:light_device:level', {
              status: getDevLevelRsp.status,
              seqNum: getDevLevelRsp.sequenceNumber,
              ieee: responseHndl.reqAddr.ieeeAddr,
              ep: responseHndl.reqAddr.endpointId,
              guid: guid,
              level: level
            });
          }
          else
          {
            console.log("RspHndl for Seq ", getDevLevelRsp.sequenceNumber, "not found");
          }
          break;                           

        // #################### Color Light - Get Color Response ##################################
        case hagateway_pb.gwCmdId_t.DEV_GET_COLOR_RSP_IND:
          var getDevColorRsp = hagateway_pb.DevGetColorRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
        
          responseHndl = getRspHndlr(getDevColorRsp.sequenceNumber);
        
          if(typeof responseHndl !== 'undefined')
          {
            // if successful in getting color values
            if(getDevColorRsp.status === hagateway_pb.gwStatus_t.STATUS_SUCCESS) 
            { 
              // set the hue and saturation
              hue = getDevColorRsp.hueValue;
              saturation = getDevColorRsp.satValue;
            }
            else
            {
              // default both values to zero
              hue = 0;
              saturation = 0;
            } 

            // generate the GUID
            guid = reverse(responseHndl.reqAddr.ieeeAddr).toString('hex') + ":" + responseHndl.reqAddr.endpointId;
            
            hageatwayInstance.emit('hagateway:light_device:color', {
              status: getDevColorRsp.status,
              seqNum: getDevColorRsp.sequenceNumber,
              ieee: responseHndl.reqAddr.ieeeAddr,
              ep: responseHndl.reqAddr.endpointId,
              guid: guid,
              hue: hue,
              saturation: saturation
            });
          }
          else
          {
            console.log("RspHndl for Seq ", getDevColorRsp.sequenceNumber, "not found");
          }
          break; 

        // #################### Temperature Device - Received Temperature Response ################
        case hagateway_pb.gwCmdId_t.DEV_GET_TEMP_RSP_IND:
          var devGetTempRspInd = hagateway_pb.DevGetTempRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
        
          responseHndl = getRspHndlr(devGetTempRspInd.sequenceNumber);
        
          if(typeof responseHndl !== 'undefined')
          {          	
            // generate the GUID
            guid = reverse(responseHndl.reqAddr.ieeeAddr).toString('hex') + ":" + responseHndl.reqAddr.endpointId;

            hageatwayInstance.emit('hagateway:temp_device:temp', {
              status: devGetTempRspInd.status,
              seqNum: devGetTempRspInd.sequenceNumber,
              guid: guid,
              ieee: responseHndl.reqAddr.ieeeAddr,
              ep: responseHndl.reqAddr.endpointId,
              temp: (devGetTempRspInd.temperatureValue / 100)
            });
          }
          else
          {
            console.log("RspHndl for Seq ", devGetTempRspInd.sequenceNumber, "not found");
          }
          break;    
              
        // #################### Received Incoming Attribute Report ################################
        case hagateway_pb.gwCmdId_t.GW_ATTRIBUTE_REPORTING_IND:
          console.log("GW_ATTRIBUTE_REPORTING_IND");
          var attributeReportingInd = hagateway_pb.GwAttributeReportingInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          
          // generate GUID
          guid = reverse(attributeReportingInd.srcAddress.ieeeAddr).toString('hex') + ":" + attributeReportingInd.srcAddress.endpointId;

          hageatwayInstance.emit('hagateway:gw:report', {
            guid: guid,
            ieee: attributeReportingInd.srcAddress.ieeeAddr,
            ep: attributeReportingInd.srcAddress.endpointId,
            clusterId: attributeReportingInd.clusterId,
            attributeRecordList: attributeReportingInd.attributeRecordList
          });
          
          break;    
    
        // #################### Doorlock Device - Door Lock State Response ########################
        case hagateway_pb.gwCmdId_t.DEV_GET_DOOR_LOCK_STATE_RSP_IND:
          console.log("DEV_GET_DOOR_LOCK_STATE_RSP_IND");
          attributeReportingInd = hagateway_pb.DevGetDoorLockStateRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          
          // if timeout
          if(attributeReportingInd.status == 4){
            console.log("Hagateway: ERROR DEV_GET_DOOR_LOCK_STATE_RSP_IND TIMEOUT");
            break;
          }

          // generate GUID
          guid = reverse(attributeReportingInd.srcAddress.ieeeAddr).toString('hex') + ":" + attributeReportingInd.srcAddress.endpointId;

          hageatwayInstance.emit('hagateway:doorlock_device:state', {
            guid: guid,
            ieee: attributeReportingInd.srcAddress.ieeeAddr,
            ep: attributeReportingInd.srcAddress.endpointId,
            clusterId: attributeReportingInd.clusterId,
            lockState: attributeReportingInd.lockState,
            doorState: attributeReportingInd.doorState
          });
          
          break;    

        // #################### Doorlock Device - Received Doorlock Repsonse ######################
        case hagateway_pb.gwCmdId_t.DEV_SET_DOOR_LOCK_RSP_IND:
          console.log("DEV_SET_DOOR_LOCK_RSP_IND");
          attributeReportingInd = hagateway_pb.DevSetDoorLockRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
  
          // if timeout
          if (attributeReportingInd.status == 4){
            console.log("Hagateqay: Status Error: ", attributeReportingInd.status);
            break;
          }

          // generate GUID
          guid = reverse(attributeReportingInd.srcAddress.ieeeAddr).toString('hex') + ":" + attributeReportingInd.srcAddress.endpointId;

          hageatwayInstance.emit('hagateway:doorlock_device:set_rsp', {
            guid: guid,
            ieee: attributeReportingInd.srcAddress.ieeeAddr,
            ep: attributeReportingInd.srcAddress.endpointId,
            clusterId: attributeReportingInd.clusterId,
            status: attributeReportingInd.status
          });
          
          break;    

        // #################### Received Read Attribute Response ##################################
        case hagateway_pb.gwCmdId_t.GW_READ_DEVICE_ATTRIBUTE_RSP_IND:
          console.log("GW_READ_DEVICE_ATTRIBUTE_RSP_IND");
          attributeReportingInd = hagateway_pb.GwReadDeviceAttributeRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
          
          // generate GUID
          guid = reverse(attributeReportingInd.srcAddress.ieeeAddr).toString('hex') + ":" + attributeReportingInd.srcAddress.endpointId;

          console.log("HAgateway: Attribute value: ", reverse(attributeReportingInd.attributeRecordList[0].attributeValue).toString('hex'));
          
          hageatwayInstance.emit('hagateway:attribute:read:rsp', {
            guid: guid,
            ieee: attributeReportingInd.srcAddress.ieeeAddr,
            ep: attributeReportingInd.srcAddress.endpointId,
            clusterId: attributeReportingInd.clusterId,
            status: attributeReportingInd.status,
            attributeRecordList: attributeReportingInd.attributeRecordList
          });
          
          break;    

        // #################### Received Write Attribute Response #################################
        case hagateway_pb.gwCmdId_t.GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND:
          console.log("GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND");
          attributeReportingInd = hagateway_pb.GwWriteDeviceAttributeRspInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);

          // generate GUID
          guid = reverse(attributeReportingInd.srcAddress.ieeeAddr).toString('hex') + ":" + attributeReportingInd.srcAddress.endpointId;
          
          hageatwayInstance.emit('hagateway:attribute:write:rsp', {
            guid: guid,
            ieee: attributeReportingInd.srcAddress.ieeeAddr,
            ep: attributeReportingInd.srcAddress.endpointId,
            clusterId: attributeReportingInd.clusterId,
            status: attributeReportingInd.status,
            srcAddress: attributeReportingInd.srcAddress,
            attributeWriteErrorList: attributeReportingInd.attributeWriteErrorList
          });
          
          break;    


        default:
          console.log("hagateway: CmdId not processed: ", rx_cmd_id);
      }
    }
  })
  .on('error', function(err){
    console.log("Error: ", err.message);
    clientReconnect();
  });    

  //----------------------------------------------------------------------------------------------------------
  // clientReconnect()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // periodically attempt to reconnect to the HaGateway server
  //----------------------------------------------------------------------------------------------------------
  function clientReconnect() { 	
  	if(typeof hageatwayInstance.clientReconnectTimer === 'undefined')
  	{  	
      //start a connection timer that tries to reconnect 5s,
	    hageatwayInstance.clientReconnectTimer = setTimeout(function(){ 
		    console.log("hagateway clientReconnectTimer: attempting to reconnect");  		    
        client.connect(HAGATEAY_PORT, ip_address, function() {
          console.log('hagateway connected');
          hageatwayInstance.emit('hagateway:network:connected');                       
        });
        clearTimeout(hageatwayInstance.clientReconnectTimer);       
        delete hageatwayInstance.clientReconnectTimer;  
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
    
    console.log("message buffer length: ", msg_buf.length);
    
    var pkt_buf = new ByteBuffer(PKT_HEADER_SIZE + msg_buf.length, ByteBuffer.LITTLE_ENDIAN)
           .writeShort(msg_buf.length, PKT_HEADER_LEN_FIELD)
           .writeUint8(hagateway_pb.zStackGwSysId_t.RPC_SYS_PB_GW, PKT_HEADER_SUBSYS_FIELD)
           .writeUint8(msg_buf[1], PKT_HEADER_CMDID_FIELD)
           .append(msg_buf, "hex", PKT_HEADER_SIZE);
    
    var responseHndl;   
    if(typeof responseCb !== 'undefined')
    {
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
  	// console.log("queTxMessage: queLen=", txQue.length);
    
    //if there is nothing in que then there 
  	//is no pending cnf, send now
    if(txQue.length === 0) {
    	if(typeof responseHndl !== 'undefined')
    	{
        //puh the rspHndle on the responsetHndlQ 
        putRspHndlr(responseHndl);
      }
      client.write(pkt_buf.buffer); 
    }
    else {
  	  //que the message
    	var txMsg = {
    	  responseHndl: responseHndl,
    	  pkt_buf: pkt_buf
      };
    
  	  console.log("queTxMessage: queing message ");
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
          //console.log("getRspHndlr found: ", sequenceNumber);
          //stop the timeout
          clearTimeout(responsetHndlQ[rspHndlIdx].timeout);
          var rsp = responsetHndlQ.splice(rspHndlIdx, 1)[0];
          //console.log("getRspHndlr returning[", responsetHndlQ.length, "]: ", rsp.id, ":", rsp.seqNum);    
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
  	//console.log("putRspHndlr[", responsetHndlQ.length, "]: ", responseHndl.id, ":", responseHndl.seqNum);  	
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
  	  //console.log("hagateway putRspHndlr: response Q overflow, removing oldest rspHndle");
  	  var responsetHndl = responsetHndlQ.splice(rspHndlIdx, 1)[0];  	
      responsetHndl.callback(hagateway_pb.gwStatus_t.STATUS_TIMEOUT, responsetHndl);
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
    responseHndl.callback(hagateway_pb.gwStatus_t.STATUS_TIMEOUT, responseHndl);
  }
   

  //#########################################################################################################
  //####################################### HA Gateway Functions ############################################
  //#########################################################################################################

  // *******************************************************************************************************
  // ***************************************** Light Device ************************************************
  // *******************************************************************************************************

  //---------------------------------------------------------------------------------------------------------
  // setState()
  //    - Inputs: 
  //              - state = integer, to set light device (0 == Off, 1 == On)
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the light device
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Sets the state of a Zigbee Light Device. Set the light to be ON or OFF.
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.setState = function(state, ieee, ep, requestCb) {        
    console.log("hagateway setState: ", ieee.toString('hex'), ":", ep, " - ", state);

    //create the message
    var gwAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var devSetOnOffStateReq = hagateway_pb.DevSetOnOffStateReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_SET_ONOFF_STATE_REQ,
        state: state,
        dstAddress: gwAddress
      });
        
    // send the command
    sendMessage(devSetOnOffStateReq, gwAddress, requestCb);      
  };
  

  //---------------------------------------------------------------------------------------------------------
  // getState()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the light device
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Gets the current state of a Zigbee Light Device. Determines whether the light is ON or OFF
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.getState = function(ieee, ep, requestCb) {        
    console.log("hagateway getState: ", ieee.toString('hex'), ":", ep);

    //create the message
    var gwAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var gevGetOnOffStateReq = hagateway_pb.DevGetOnOffStateReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_GET_ONOFF_STATE_REQ,
        dstAddress: gwAddress
      });
        
    // send the command
    sendMessage(gevGetOnOffStateReq, gwAddress, requestCb);      
  };

  
  //---------------------------------------------------------------------------------------------------------
  // setLevel()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the light device
  //              - level = integer, level brightness to set on a light device
  //              - transitionTime = integer, the transition time taken to change a light brightness level
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Sets the current level of a Zigbee Light Device. 
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.setLevel = function(ieee, ep, level, transitionTime, requestCb) {        
    console.log("hagateway setLevel: ", ieee.toString('hex'), ":", ep, " - ", level);

    //create the message
    var gwAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var gevSetLevelReq = hagateway_pb.DevSetLevelReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_SET_LEVEL_REQ,
        levelValue: level,
        transitionTime: transitionTime/100,  //Transition time in ZigBee is measure in 100ms, change from 1ms
        dstAddress: gwAddress
      });
        
    // send the command
    sendMessage(gevSetLevelReq, gwAddress, requestCb);      
  };
    

  //---------------------------------------------------------------------------------------------------------
  // getLevel()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the light device
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Gets the current level of a Zigbee Light Device. Determines how bright the light device is
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.getLevel = function(ieee, ep, requestCb) {        
    console.log("hagateway getLevel: ", ieee.toString('hex'), ":", ep);

    //create the message
    var gwAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var gevGetLevelReq = hagateway_pb.DevGetLevelReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_GET_LEVEL_REQ,
        dstAddress: gwAddress
      });
        
    sendMessage(gevGetLevelReq, gwAddress, requestCb);      
  };
  

  //---------------------------------------------------------------------------------------------------------
  // setColor()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the light device
  //              - hue = integer, hue value to set on a color light device
  //              - saturation = integer, saturation value to set on a color light device
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Sets the current state of a Zigbee Light Device. Determines whether the light is ON or OFF
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.setColor = function(ieee, ep, hue, saturation, requestCb) {        

    //create the message
    var gwAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var gevSetColorReq = hagateway_pb.DevSetColorReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_SET_COLOR_REQ,
        hueValue: hue,
        saturationValue: saturation,
        dstAddress: gwAddress
      });
        
    sendMessage(gevSetColorReq, gwAddress, requestCb);      
  };
    

  //---------------------------------------------------------------------------------------------------------
  // getColor()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the light device
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Get the current color of a Zigbee Color Light Device.
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.getColor = function(ieee, ep, requestCb) {        

    //create the message
    var gwAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var gevGetColorReq = hagateway_pb.DevGetColorReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_GET_COLOR_REQ,
        dstAddress: gwAddress
      });
        
    sendMessage(gevGetColorReq, gwAddress, requestCb);      
  }; 

  // *******************************************************************************************************
  // ************************************** Thermostat Device **********************************************
  // *******************************************************************************************************

  //---------------------------------------------------------------------------------------------------------
  // setpointChange()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the light device
  //              - mode = integer, 0 = heat setpoint, 1 = cool setpoint, 3 = both setpoints
  //              - amount = integer, amount to increment the setpoints by (increments of 0.1 Degreees)
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Sets a thermostat setpoint mode by a certain increment more/less from its current value. 
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.setpointChange = function(ieee, ep, mode, amount, requestCb) {        

    //create the message
    var gwAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var ByteBuffer = require("bytebuffer"); 

    // check to make sure an appropriate mode is being used. Default is Heat Mode
    switch (mode){
      case hagateway_pb.gwThermostatSetpointMode_t.HEAT_SETPOINT:
        modeValue = hagateway_pb.gwThermostatSetpointMode_t.HEAT_SETPOINT;
        break;

      case hagateway_pb.gwThermostatSetpointMode_t.COOL_SETPOINT:
        modeValue = hagateway_pb.gwThermostatSetpointMode_t.COOL_SETPOINT;
        break;

      case hagateway_pb.gwThermostatSetpointMode_t.BOTH_SETPOINTS:
        modeValue = hagateway_pb.gwThermostatSetpointMode_t.BOTH_SETPOINTS;
        break;
       
      default:
        modeValue = hagateway_pb.gwThermostatSetpointMode_t.HEAT_SETPOINT;
    }
    
    var amountValue = new ByteBuffer(1, ByteBuffer.LITTLE_ENDIAN)
      .writeInt32(amount);

    var gevThermostatSetpointChangeReq = hagateway_pb.DevThermostatSetpointChangeReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_THERMOSTAT_SETPOINT_CHANGE_REQ,
        dstAddress: gwAddress,
        mode: modeValue,
        amount: amount
      });
        
    sendMessage(gevThermostatSetpointChangeReq, gwAddress, requestCb);      
  };


  //---------------------------------------------------------------------------------------------------------
  // getTemp()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of thermostat device
  //              - ep = integer, endpoint of application on the thermostat device
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Gets a temperature value from a device. Device must have a temperature value attribute to work.
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.getTemp = function(ieee, ep, requestCb) {        
    console.log("hagateway getTemp: ", ieee.toString('hex'), ":", ep);

    //create the message
    var dstAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var devGetTempReq = hagateway_pb.DevGetTempReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_GET_TEMP_REQ,
        dstAddress: dstAddress
      });
        
    sendMessage(devGetTempReq, dstAddress, requestCb);      

  };   
   

  // *******************************************************************************************************
  // ************************************** Doorlock Device ************************************************
  // *******************************************************************************************************

  //---------------------------------------------------------------------------------------------------------
  // getDoorlockState()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the doorlock device
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Gets the current doorlock state from a particular doorlock device. 
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.getDoorlockState = function(ieee, ep, requestCb) {        
    console.log("hagateway getDoorlockState: ", ieee.toString('hex'), ":", ep);

    //create the message
    var dstAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var devGetDoorLockStateReq = hagateway_pb.DevGetDoorLockStateReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_GET_DOOR_LOCK_STATE_REQ,
        dstAddress: dstAddress
      });
        
    sendMessage(devGetDoorLockStateReq, dstAddress, requestCb);      
  };   


  //---------------------------------------------------------------------------------------------------------
  // setDoorlockState()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the doorlock device
  //              - state = integer, set the state of the doorlock to either lock/unlocked, 0=>lock, 1=>unlock
  //              - pin = integer, a security pin used to set the state of a doorlock, only first 4 digits will be used
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Sets the current doorlock state on a particular doorlock device. 
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.setDoorlockState = function(ieee, ep, state, pin, requestCb) {        
    console.log("hagateway setDoorlockState: ", ieee.toString('hex'), ":", ep, " - ", state);

    //create the message
    var gwAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var ByteBuffer = require("bytebuffer"); 
    var pinCodeValue = new ByteBuffer(4, ByteBuffer.LITTLE_ENDIAN)
      .writeUint8(pin[0].toString().charCodeAt(0))
      .writeUint8(pin[1].toString().charCodeAt(0))
      .writeUint8(pin[2].toString().charCodeAt(0))
      .writeUint8(pin[3].toString().charCodeAt(0));

    var gevSetDoorLockReq = hagateway_pb.DevSetDoorLockReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.DEV_SET_DOOR_LOCK_REQ,
        dstAddress: gwAddress,
        lockMode: state,
        pinCodeValue: pinCodeValue.buffer
      });
        
    sendMessage(gevSetDoorLockReq, gwAddress, requestCb);      
  };


  // *******************************************************************************************************
  // ************************************** Generic Device ************************************************
  // *******************************************************************************************************

  //---------------------------------------------------------------------------------------------------------
  // setAttrReport()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the doorlock device
  //              - clusterId = integer, ID of particular cluster of attribute trying to set
  //              - attrList = object:
  //                              - attributeId = ID of attribute trying to set
  //                              - attributeType = Attribute data type
  //                              - minReportInterval = minimum report interval of the attribute
  //                              - maxReportInterval = maximum report interval of the attribute
  //                              - reportableChange = force a report to be sent on a minimal interval change
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Sets a certain attribute to be reported at a requested interval  
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.setAttrReport = function(ieee, ep, clusterId, attrList, requestCb) {        

    console.log("HaGateway: setAttrReport: ", attrList);

    //create the message
    var dstAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };
    
    //bounds checking - minReportInterval can not be les than 1
    var attrIdx;
    for(attrIdx in attrList) {
    	if(attrList[attrIdx].minReportInterval < 1) attrList[attrIdx].minReportInterval = 1;
    }
    
    console.log("hagateway setAttrReport: ", {
        cmdId: hagateway_pb.gwCmdId_t.GW_SET_ATTRIBUTE_REPORTING_REQ,
        dstAddress: dstAddress,
        clusterId: clusterId,
        attributeReportList: attrList         
    });
    
        
    var setAttributeReportingReq = hagateway_pb.GwSetAttributeReportingReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.GW_SET_ATTRIBUTE_REPORTING_REQ,
        dstAddress: dstAddress,
        clusterId: clusterId,
        attributeReportList: attrList         
      });
     
    sendMessage(setAttributeReportingReq, dstAddress, requestCb);      

  }; 


  //---------------------------------------------------------------------------------------------------------
  // readDeviceAttribute()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the doorlock device
  //              - clusterId = integer, ID of particular cluster of attribute trying to read
  //              - attrList = array of values, each referring to a attribute type
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Gets a certain attribute value from a particular device
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.readDeviceAttribute = function(ieee, ep, clusterId, attrList, requestCb) {        

    console.log("HaGateway: readDeviceAttribute: ", attrList);

    //create the message
    var dstAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };
    
    console.log("hagateway readDeviceAttribute: ", {
        cmdId: hagateway_pb.gwCmdId_t.GW_READ_DEVICE_ATTRIBUTE_REQ,
        dstAddress: dstAddress,
        clusterId: clusterId,
        attributeList: attrList,
        isServerToClient: 0         
    });
    
    var gwReadDeviceAttributeReq = hagateway_pb.GwReadDeviceAttributeReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.GW_READ_DEVICE_ATTRIBUTE_REQ,
        dstAddress: dstAddress,
        clusterId: clusterId,
        attributeList: attrList,
        isServerToClient: 0      
      });
     
    sendMessage(gwReadDeviceAttributeReq, dstAddress, requestCb);      
  }; 


  //---------------------------------------------------------------------------------------------------------
  // writeDeviceAttribute_uint16()
  //    - Inputs: 
  //              - ieee = buffer, containing ieee of light device
  //              - ep = integer, endpoint of application on the doorlock device
  //              - clusterId = integer, ID of particular cluster of attribute trying to set
  //              - attrId = integer, attribute ID that are writing to 
  //              - attrValue = UINT16, value to write to attribute
  //              - requestCb = callback function
  //    - Output: NULL
  //
  // Writes a UINT16 value to an attribute on a particular device.
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.writeDeviceAttribute_uint16 = function(ieee, ep, clusterId, attrId, attrValue, requestCb) {        

    console.log("HaGateway: writeDeviceAttribute");

    //create the message
    var dstAddress = {
      addressType: hagateway_pb.gwAddressType_t.UNICAST, // Address Mode
      ieeeAddr: ieee,  // extended address
      endpointId: ep   // endpoint 
    };

    var first_byte = attrValue & 0x00FF;
    var second_byte = (attrValue & 0xFF00) >> 8;

    console.log("Value: ", attrValue, "First: ", attrValue[0], " Second: ", attrValue[1]);

    var ByteBuffer = require("bytebuffer"); 
    var attrValueBuffer = new ByteBuffer(2, ByteBuffer.LITTLE_ENDIAN)
      .writeUint16(attrValue);

    var attrList = {
      attributeId: attrId,
      attributeType: hagateway_pb.gwZclAttributeDataTypes_t.ZCL_DATATYPE_UINT16,
      attributeValue: attrValueBuffer.buffer
    };
    
    var gwWriteDeviceAttributeReq = hagateway_pb.GwWriteDeviceAttributeReq
      .encode({
        cmdId: hagateway_pb.gwCmdId_t.GW_WRITE_DEVICE_ATTRIBUTE_REQ,
        dstAddress: dstAddress,
        clusterId: clusterId,
        attributeRecordList: [attrList]         
      });

      console.log(gwWriteDeviceAttributeReq);
     
    sendMessage(gwWriteDeviceAttributeReq, dstAddress, requestCb);      
  }; 


  //---------------------------------------------------------------------------------------------------------
  // reconnectHagateway()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Periodically attempts to reconnect to the HA-Gateway server 
  //---------------------------------------------------------------------------------------------------------
  Hagateway.prototype.reconnectHagateway = function(){
    clientReconnect();
  };
}

Hagateway.prototype.__proto__ = events.EventEmitter.prototype;
 
// export the class
module.exports = Hagateway;