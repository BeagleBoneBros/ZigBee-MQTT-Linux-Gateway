/******************************************************************************

 @file zb-gateway.js

 @brief Manages all Zigbee events in network. Handles all data flow through
 Node.js Gateway

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

var Nwkmgr = require('../linux_gw_adapters/nwkmgr.js');
var ByteBuffer = require("bytebuffer"); 
var events = require('events');
var Hagateway = require('../linux_gw_adapters/hagateway.js');
var reverse = require("buffer-reverse");
var ZBL = require('../zigbee_library/zb_lib.js');

//declare out side of the scope of the function 
//so it can be used for all instances  
var ZigbeeGatewayInstance;

// Constructor
function ZigbeeGateway(ip_address) {
  if (typeof ZigbeeGatewayInstance !== "undefined") {
    return ZigbeeGatewayInstance;
  }
  
  var nwkmgr = new Nwkmgr(ip_address);
  var hagateway = new Hagateway(ip_address);
  
  events.EventEmitter.call(this);
  ZigbeeGatewayInstance = this;

  // initialize an empty array for device list
  ZigbeeGatewayInstance.devArray = [];

  // initialize an empty array for possible unbound device list
  ZigbeeGatewayInstance.unboundDeviceList = {}; 

  // initialize an empty array for possible binded device list
  ZigbeeGatewayInstance.bindedDeviceList = {}; 


  // -------------------------------------------------------------------------------------------
  // Handle All Events from Network Manager
  // -------------------------------------------------------------------------------------------
  // ################################# New Device ##############################################
  nwkmgr.on('nwkmgr:newDev', function(data) {
    console.log("nwkmgr emitted newDev: ", data);  

    //check we do not have the device already
    for(var devIdx in ZigbeeGatewayInstance.devArray)
    {
      // if the new device is in the device array, return
      if(ZigbeeGatewayInstance.devArray[devIdx].info.id === data.info.id) {
        return;
      }	
    }

    // add new deivce to device array list
    ZigbeeGatewayInstance.devArray.push(data);

    // check for possible deive pairs that can be bound, add them to the UnBoundDeviceList
    checkPossibleBinding();
    
    // emit event from zb-gateway, informing of new device, sending device data
    ZigbeeGatewayInstance.emit('zb-gateway:newDev', ZigbeeGatewayInstance.devArray,  ZigbeeGatewayInstance.unboundDeviceList, ZigbeeGatewayInstance.bindedDeviceList);
  })
  // ################################ Remove Device ###########################################
  .on('nwkmgr:removeDev', function(data) {
   console.log("nwkmgr emitted removeDev: ", data);  
   //remove all devices with this ieee addr
   for(devIdx = 0; devIdx < ZigbeeGatewayInstance.devArray.length; devIdx++)
   {
   	 var ieeeBb = new ByteBuffer(8, ByteBuffer.LITTLE_ENDIAN).append(ZigbeeGatewayInstance.devArray[devIdx].info.ieee, "hex", 0);
   	 var ieeeRemoveBb = new ByteBuffer(8, ByteBuffer.LITTLE_ENDIAN).append(data, "hex", 0);
            
   	 console.log("searching[", devIdx, "]: ", ieeeRemoveBb.toString('hex'), ":", ieeeBb.toString('hex'), ":", ieeeRemoveBb.toString('hex').localeCompare(ieeeBb.toString('hex')));
     if(ieeeRemoveBb.toString('hex').localeCompare(ieeeBb.toString('hex')) === 0)
     { 
      console.log("zb-gateway: removing device", ZigbeeGatewayInstance.devArray[devIdx]);
      
      // send event to webapp to remove from device list
      ZigbeeGatewayInstance.emit('zb-gateway:removeDeviceCard', ZigbeeGatewayInstance.devArray[devIdx]);
      
      // remove device info from all bind lists
      removeBindingItembyGuid(ZigbeeGatewayInstance.devArray[devIdx].data.guid);

      // remove device from master device list
      var removeDevice = ZigbeeGatewayInstance.devArray.splice(devIdx, 1);       
      delete removeDevice[0];
      
      //we just removed an array element, so reduce devIdx by 1;
      devIdx = devIdx-1;
     }	
   }
  })
  // ################################ Network Ready ###########################################
  .on('nwkmgr:network:ready', function() {
    console.log("Network Is Ready!");
    ZigbeeGatewayInstance.emit('zb-gateway:network:ready');
  })
  // ################################ Network Info ###########################################
  .on('nwkmgr:network:info', function(data) {
    console.log("zbgateway: Got Network Info");
    ZigbeeGatewayInstance.emit('zb-gateway:network:info', data);
  })
  // ############################## Binding Response ##########################################
  .on('nwkmgr:binding:rsp', function(data) {

    //check status of the binding
    if (data.status == 0 || data.status == 136){
      console.log("Zb-Gateway: Binding Successful");

      // construct the GUID, EP and ID
      source_guid = reverse(data.srcIeee).toString('hex') + ":" + data.srcEp;
      dest_guid = reverse(data.dstIeee).toString('hex') + ":" + data.dstEp;

      console.log("ZB_Gateway: sourceGUID: ", source_guid, " destGUID: ", dest_guid);

      // check to see if both GUIDs are devices on the network, otherwise we are looking at a binding event with the coordinator
      if (checkGUIDinDevArray(source_guid) && checkGUIDinDevArray(dest_guid)){
        updateBindingList(source_guid, dest_guid);
        console.log("End unboundedDeviceList: ", ZigbeeGatewayInstance.unboundDeviceList, " bindedDeviceList: ", ZigbeeGatewayInstance.bindedDeviceList);
      }     
      // emit event from zb-gateway, informing of new binding event
      ZigbeeGatewayInstance.emit('zb-gateway:binding:event', ZigbeeGatewayInstance.devArray, ZigbeeGatewayInstance.unboundDeviceList, ZigbeeGatewayInstance.bindedDeviceList); 
    }
    else if (data.status == 136){
      console.log("ZB_Gateway: No Entry, device is already unbound.", data);
    }
    else {
      console.log("Zb-Gateway: Binding Not Successful");
      ZigbeeGatewayInstance.emit('zb-gateway:binding:failed');
    }   
   });

  
  // -------------------------------------------------------------------------------------------
  // Handle All Events from the HAGateway
  // -------------------------------------------------------------------------------------------  
  // ######################### Light Device - State Update #####################################
  hagateway.on('hagateway:light_device:state', function(data) {
    // console.log('ZB-Gateway: The state is', data.on, data.guid);
    ZigbeeGatewayInstance.emit('zb-gateway:light_device:state', data);
  })
  // ######################### Light Device - Level Update #####################################
  .on('hagateway:light_device:level', function(data) {
    // console.log('ZB-Gateway: The level is', data.level, data.guid);
    ZigbeeGatewayInstance.emit('zb-gateway:light_device:level', data);
  })
  // ######################### Light Device - Color Update #####################################
  .on('hagateway:light_device:color', function(data) {
    // console.log('ZB-Gateway: The level is', data.level, data.guid);
    ZigbeeGatewayInstance.emit('zb-gateway:light_device:color', data);
  })
  // ################## Temperature Device - Temperature Update ################################
  .on('hagateway:temp_device:temp', function(data){
    // console.log('ZB-Gateway: The temp is: ', data);
    ZigbeeGatewayInstance.emit('zb-gateway:temp_device:temp', data);
  })
  // ##################### DoorLock Device - State Update ######################################
  .on('hagateway:doorlock_device:state', function(data){
    console.log('ZB-Gateway: The doorlock is: ', data);
    ZigbeeGatewayInstance.emit('zb-gateway:doorlock_device:state', data);
  })
  // ##################### DoorLock Device - Set Response ######################################
  .on('hagateway:doorlock_device:set_rsp', function(data){
    console.log('ZB-Gateway: The doorlock rsp status: ', data);
    ZigbeeGatewayInstance.emit('zb-gateway:doorlock_device:set_rsp', data);
  })
  // #################### Device Attribute - Read Response #####################################
  .on('hagateway:attribute:read:rsp', function(data) {
    console.log("ZB-Gateway: Got attribute read rsp: ", data);  
    
    // check all devices in deviceList and determine what type of device the attribute response is from 
    for(devIdx = 0; devIdx < ZigbeeGatewayInstance.devArray.length; devIdx++) {
      if (data.guid == ZigbeeGatewayInstance.devArray[devIdx].data.guid){
        switch(ZigbeeGatewayInstance.devArray[devIdx].data.type){

          case 'Thermostat':
            console.log("zb-gateway: Thermostat Attribute value: ", reverse(data.attributeRecordList[0].attributeValue).toString('hex'));
            
            // convert the array buffers to string values
            for (idx = 0; idx < data.attributeRecordList.length; idx++){
              data.attributeRecordList[idx].attributeValue = reverse(data.attributeRecordList[idx].attributeValue).toString('hex');
            }

            ZigbeeGatewayInstance.emit('zb-gateway:thermostat:attribute', data);
        }

      }
    }
    
   })
  // #################### Device Attribute - Write Response ###################################
  .on('hagateway:attribute:write:rsp', function(data) {
    console.log("ZB-Gateway: Got attribute write rsp: ", data);  
    
    // if the attribute write was successful
    if(data.status == ZBL.STATUS.SUCCESS){
      console.log("Attribute Write Sent Successful");
        
      // loop through all arritbute responses
      for(devIdx = 0; devIdx < data.attributeWriteErrorList.length; devIdx++) {

        clusterId = data.clusterId;
        attributeStatus = data.attributeWriteErrorList[devIdx].status;

        rsp_data = {
          guid: data.guid,
          ieee: data.srcAddress.ieeeAddr,
          ep: data.srcAddress.endpointId,
          clusterId: clusterId,
          status: data.status,
          srcAddress: data.srcAddress,
          attributeStatus: attributeStatus
        };

        // emit an event based on the attribute response, based on clusterID and the attribute Status code
        ZigbeeGatewayInstance.emit(EVT('zb-gateway', clusterId, attributeStatus), rsp_data);
      }
    }
    else{
      // emit an event based on the attribute response, based on clusterID and the attribute Status code
      ZigbeeGatewayInstance.emit('zb-gateway:attribute:write:failed', rsp_data);
    } 
    
   })
   // #################### Device Report - Incoming Report ####################################
  .on('hagateway:gw:report', function(data){
    // read incoming report based on what its ClusterID is
    switch(data.clusterId){
      // incoming temperature measurement
      case ZBL.HA_CLUSTER_ID.TEMPERATURE_MEASUREMENT:     
        if( (data.attributeRecordList[0].attributeId === ZBL.TEMPERATURE_MEASUREMENT_CLUSTER.MeasuredValue) && (data.attributeRecordList[0].attributeType === ZBL.DATA_TYPE.INT16)) {
          var raw_temp = data.attributeRecordList[0].attributeValue;
          raw_temp_string = reverse(raw_temp).toString('hex');
          raw_temp_int = parseInt(raw_temp_string, 16);
          var temp = raw_temp_int / 100;   
          console.log("Zb-Gateway: The temp is: ", temp);  

          var temp_data = {
            guid: data.guid,
            temp: temp
          };

          ZigbeeGatewayInstance.emit('zb-gateway:temp_device:temp', temp_data);
        }
      break;   
      
    }
  })
  // ######################### Connected to Network ##########################################
  .on('hagateway:network:connected', function() {
    console.log('ZB-Gateway: Connected to Gateways');
    ZigbeeGatewayInstance.emit('zb-gateway:network:ready');
  });


  // ##########################################################################################
  //  LOCAL FUNCTIONS
  // ##########################################################################################  

  //----------------------------------------------------------------------------------------------------------
  // zb_gateway_getNetworkInfo()
  //    -Input: NULL
  //    -Return: NULL
  // 
  //  open the network 
  //----------------------------------------------------------------------------------------------------------
  function zb_gateway_getNetworkInfo(){
    console.log("ZB-Gateway: Called getNetworkInfo()");
    nwkmgr.getNetworkInfo();
  }
  
  //----------------------------------------------------------------------------------------------------------
  // zb_gateway_openNetwork()
  //    -Input: NULL
  //    -Return: NULL
  // 
  //  open the network 
  //----------------------------------------------------------------------------------------------------------
  function zb_gateway_openNetwork(){
    console.log("ZB-Gateway: Called openNetwork()");
    nwkmgr.openNetwork(180);
  }


  //----------------------------------------------------------------------------------------------------------
  // zb_gateway_closeNetwork()
  //    -Input: NULL
  //    -Return: NULL
  // 
  // close the network
  //----------------------------------------------------------------------------------------------------------
  function zb_gateway_closeNetwork(){
    console.log("ZB-Gateway: Called closeNetwork()");
    nwkmgr.openNetwork(0);
  }

  
  //----------------------------------------------------------------------------------------------------------
  // zb_gateway_hardSystemReset()
  //    -Input: NULL
  //    -Return: NULL
  // 
  // implement a Hard System Reset
  //----------------------------------------------------------------------------------------------------------
  function zb_gateway_hardSystemReset(){
    console.log("ZB-Gateway: Hard System Reset");

    // empty array for device list
    ZigbeeGatewayInstance.devArray = [];

    // empty array for possible unbound device list
    ZigbeeGatewayInstance.unboundDeviceList = {};

    // empty array for possible binded device list
    ZigbeeGatewayInstance.bindedDeviceList = {};

    // call network manager to initiate Hard Reset
    nwkmgr.hardSystemReset();

    //start a timer to reconnect to the Linux Gateway after 10 seconds
    ZigbeeGatewayInstance.ResetTimer = setTimeout(function () {
      nwkmgr.reconnectNwkmgr();
      hagateway.reconnectHagateway();
    }, 10000);
  }

  
  //----------------------------------------------------------------------------------------------------------
  // zb_gateway_softSystemReset()
  //    -Input: NULL
  //    -Return: NULL
  // 
  // implement a Soft System Reset
  //----------------------------------------------------------------------------------------------------------
  function zb_gateway_softSystemReset(){
    console.log("ZB-Gateway: Soft System Reset");
    nwkmgr.softSystemReset();

    //start a timer to reconnect to the Linux Gateway after 10 seconds
    ZigbeeGatewayInstance.ResetTimer = setTimeout(function () {
      nwkmgr.reconnectNwkmgr();
      hagateway.reconnectHagateway();
    }, 10000);
  }


  //----------------------------------------------------------------------------------------------------------
  // zb_gateway_getDeviceList()
  //    -Input: NULL
  //    -Return: NULL
  // 
  // get all device lists
  //----------------------------------------------------------------------------------------------------------
  function zb_gateway_getDeviceList(){
    console.log("ZB-Gateway: Get Device List");
    ZigbeeGatewayInstance.emit('zb-gateway:deviceList', ZigbeeGatewayInstance.devArray,  ZigbeeGatewayInstance.unboundDeviceList, ZigbeeGatewayInstance.bindedDeviceList);
  }


  //----------------------------------------------------------------------------------------------------------
  // zb_gateway_getBindList()
  //    -Input: NULL
  //    -Return: NULL
  // 
  // get the unbound and binded device lists
  //----------------------------------------------------------------------------------------------------------
  function zb_gateway_getBindList(){
    console.log("ZB-Gateway: Get Bind List");
    ZigbeeGatewayInstance.emit('zb-gateway:binding:event', ZigbeeGatewayInstance.devArray, ZigbeeGatewayInstance.unboundDeviceList, ZigbeeGatewayInstance.bindedDeviceList);
  }


  //----------------------------------------------------------------------------------------------------------
  // zb_gateway_removeDevice()
  //    -Inputs: Data Object with following properties:
  //                                                  - ieee
  //                                                  - guid
  //    -Return: NULL
  // 
  // remove a device from the network
  //----------------------------------------------------------------------------------------------------------
  function zb_gateway_removeDevice(data){
    console.log("webserver called removeDevice: ", data);  	

    var devIdx = 0;
    var found = false;

    // loop through all devices in the device list
    for(devIdx = 0; devIdx < ZigbeeGatewayInstance.devArray.length; devIdx++)
    {
      var ieeeBb = new ByteBuffer(8, ByteBuffer.LITTLE_ENDIAN)
        .append(ZigbeeGatewayInstance.devArray[devIdx].info.ieee, "hex", 0);
      var ieeeRemoveBb = new ByteBuffer(8, ByteBuffer.LITTLE_ENDIAN)
        .append(data.ieee, "hex", 0);
              
      console.log("searching[", devIdx, "]: ", ieeeRemoveBb.toString('hex'), ":", ieeeBb.toString('hex'), ":", ieeeRemoveBb.toString('hex').localeCompare(ieeeBb.toString('hex')));
      if(ieeeRemoveBb.toString('hex').localeCompare(ieeeBb.toString('hex')) === 0)
      {     	
        console.log("found: ", ieeeRemoveBb.buffer);  	
        //only send network leave for 1 endpoint
        if( found === false) {
          nwkmgr.removeDev(ZigbeeGatewayInstance.devArray[devIdx].info.ieee);
        }

        // remove device from the device array
        var removeDevice = ZigbeeGatewayInstance.devArray.splice(devIdx, 1);       
        delete removeDevice[0];

        //we just removed an array element, so reduce devIdx by 1;
        devIdx = devIdx-1;
        found = true;
      }	
    }

    // once device is removed from network, neede to remove the device from the binded/unbinded lists if binable item
    removeBindingItembyGuid(data.guid);
  }


  //----------------------------------------------------------------------------------------------------------
  // checkPossibleBinding()
  //    -Input: NULL
  //    -Return: NULL
  // 
  // Create an array that includes all possible binding combinations. 
  //----------------------------------------------------------------------------------------------------------
  function checkPossibleBinding() {

    // import the possible binding table object
    possibleBindingTable = ZBL.POSSIBLE_BINDING_TABLE;

    // loop through all devices in our device list
    for(devIdx = 0; devIdx < ZigbeeGatewayInstance.devArray.length; devIdx++){
      sourceDeviceType = ZigbeeGatewayInstance.devArray[devIdx].data.type;

      // check if the current device type is a binding source device. 
      if (sourceDeviceType in possibleBindingTable){
        console.log(sourceDeviceType, " in possibleBindingTable");

        // using the current device type as a key, obtain array of all possible devices it can bind to
        possibleDestDevices = possibleBindingTable[sourceDeviceType];
        console.log("destDevices: ", possibleDestDevices);

        // now given this source device, loop through all devices to see if it can bind to source device
        for(destIdx = 0; destIdx < ZigbeeGatewayInstance.devArray.length; destIdx++){
          destDeviceType = ZigbeeGatewayInstance.devArray[destIdx].data.type;

          // check if the current destination deviceType is a possible destination device
          if (possibleDestDevices.includes(destDeviceType)){
            console.log(destDeviceType, " ==> ", sourceDeviceType);
            tempArray = [ZigbeeGatewayInstance.devArray[devIdx].data.guid, ZigbeeGatewayInstance.devArray[destIdx].data.guid];


            // flag to determine whether to add to unbound list or not
            add_to_list = true;

            if (ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType]){
              for(uIdx = 0; uIdx < ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType].length; uIdx++){
                if (ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType][uIdx][0] == tempArray[0] && ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType][uIdx][1] == tempArray[1]){
                  // pair already present in list, dont add to list again
                  add_to_list = false;
                  break;
                }
              }
            }

            if (ZigbeeGatewayInstance.bindedDeviceList[sourceDeviceType]){
              for(uIdx = 0; uIdx < ZigbeeGatewayInstance.bindedDeviceList[sourceDeviceType].length; uIdx++){
                if (ZigbeeGatewayInstance.bindedDeviceList[sourceDeviceType][uIdx][0] == tempArray[0] && ZigbeeGatewayInstance.bindedDeviceList[sourceDeviceType][uIdx][1] == tempArray[1]){
                  // pair already present in list, dont add to list again
                  add_to_list = false;
                  break;
                }
              }
            }

            if (add_to_list){
              console.log("Add to unbound list: ", tempArray)
              // if we haven't created an array for this device type yet, create one and push pair
              if (ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType] == null){
                ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType] = [];
                ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType].push(tempArray);
                continue;
              }
              // if the previous array is empty, push pair
              else {
                // add the bindable pair to the unbound list
                ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType].push(tempArray);
                continue;
              }
            }
    
          }
        }
      }
    }

    console.log("UnboundList:", ZigbeeGatewayInstance.unboundDeviceList, " BindList: ", ZigbeeGatewayInstance.bindedDeviceList);
  }


  //----------------------------------------------------------------------------------------------------------
  // removeBindingItembyGuid()
  //    -Input: guid
  //    -Return: NULL
  // 
  // Remove a Device from all Occurances in Binding Lists. Based on the GUID of the device
  //----------------------------------------------------------------------------------------------------------
  function removeBindingItembyGuid(guid){
    // loop through all unbound pairs
    for (var sourceDeviceType in ZigbeeGatewayInstance.unboundDeviceList) {
      unboundDeviceArray = ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType];
      
      // if the guid is present in the array, remove entire entry
      for (uIdx = 0; uIdx < unboundDeviceArray.length; uIdx++){
        if (unboundDeviceArray[uIdx][0] == guid || unboundDeviceArray[uIdx][1] == guid){
          unboundDeviceArray.splice(uIdx, 1);
        }

        // update the unbound device list with latest
        ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType] = unboundDeviceArray;
      } 
    }

    // loop through all bound pairs
    for (var bindedSourceDeviceType in ZigbeeGatewayInstance.bindedDeviceList) {
      bindedDeviceArray = ZigbeeGatewayInstance.bindedDeviceList[bindedSourceDeviceType];

      // if the guid is present in the array, remove from entire array
      for (uIdx = 0; uIdx < bindedDeviceArray.length; uIdx++){
        if (bindedDeviceArray[uIdx][0] == guid || bindedDeviceArray[uIdx][1] == guid){
          bindedDeviceArray.splice(uIdx, 1);
        }

        // update the unbound device list with the latest
        ZigbeeGatewayInstance.bindedDeviceList[bindedSourceDeviceType] = bindedDeviceArray;
      }
    }
  }


  //----------------------------------------------------------------------------------------------------------
  // updateBindingList()
  //    -Input: source GUID, destination GUID
  //    -Return: NULL
  // 
  // Update the array that includes all possible binding combinations after a binding event takes place.
  // updates the unbound and binded device lists 
  //----------------------------------------------------------------------------------------------------------
  function updateBindingList(sourceGuid, destGuid) {

    console.log("Update the tables: ", sourceGuid, destGuid);
    console.log("Start unboundedDeviceList: ", ZigbeeGatewayInstance.unboundDeviceList, " bindedDeviceList: ", ZigbeeGatewayInstance.bindedDeviceList);

    // loop through each device type in the unbound device list
    for (var sourceDeviceType in ZigbeeGatewayInstance.unboundDeviceList) {
      unboundDeviceArray = ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType];

      // for each element in unbound device array, check if it matches the sourceGuid and destGuid
      for (uIdx = 0; uIdx < unboundDeviceArray.length; uIdx++){
        if (unboundDeviceArray[uIdx][0] == sourceGuid && unboundDeviceArray[uIdx][1] == destGuid){
          // found in unboundDeviceList, remove from unboundDeviceList
          unboundDeviceArray.splice(uIdx, 1);
          ZigbeeGatewayInstance.unboundDeviceList[sourceDeviceType] = unboundDeviceArray;

          // add to bindedDeviceList
          if (ZigbeeGatewayInstance.bindedDeviceList[sourceDeviceType] == null){
            ZigbeeGatewayInstance.bindedDeviceList[sourceDeviceType] = [];
            ZigbeeGatewayInstance.bindedDeviceList[sourceDeviceType].push([sourceGuid, destGuid]);
          }
          else{
            ZigbeeGatewayInstance.bindedDeviceList[sourceDeviceType].push([sourceGuid, destGuid]);
          }

          // found what we need, return 
          return;
        }
      }
    }

    // loop through each device type in the binded device list
    for (var bindedSourceDeviceType in ZigbeeGatewayInstance.bindedDeviceList) {
      bindedDeviceArray = ZigbeeGatewayInstance.bindedDeviceList[bindedSourceDeviceType];

      // loop through each element in the array
      for (uIdx = 0; uIdx < bindedDeviceArray.length; uIdx++){
        if (bindedDeviceArray[uIdx][0] == sourceGuid && bindedDeviceArray[uIdx][1] == destGuid){
          // found in bindedDeviceList, remove from bindedDeviceList
          bindedDeviceArray.splice(uIdx, 1);
          ZigbeeGatewayInstance.bindedDeviceList[bindedSourceDeviceType] = bindedDeviceArray;

          // add to unbindedDeviceList
          if (ZigbeeGatewayInstance.unboundDeviceList[bindedSourceDeviceType] == null){
            ZigbeeGatewayInstance.unboundDeviceList[bindedSourceDeviceType] = [];
            ZigbeeGatewayInstance.unboundDeviceList[bindedSourceDeviceType].push([sourceGuid, destGuid]);
          }
          else{
            ZigbeeGatewayInstance.unboundDeviceList[bindedSourceDeviceType].push([sourceGuid, destGuid]);
          }
          // found what we need, return
          return;
        }
      }
    }

    console.log("ERROR: No combination of sourceGUID and destGUID was found in eitherthe unbound or binded lists");
  }


  //----------------------------------------------------------------------------------------------------------
  // checkGUIDinDevArray()
  //    -Input: guid
  //    -Return: BOOLEAN --> FALSE = Device Not in Device List. 
  //                     --> TRUE = Device in Device List. 
  // 
  // searches the devArray to determine if the device, based on GUID, is in the array
  //----------------------------------------------------------------------------------------------------------
  function checkGUIDinDevArray(guid){
    if (ZigbeeGatewayInstance.devArray.length == 0){
      // There are no devices in the DevArray
      return false;
    }

    for (i=0; i < ZigbeeGatewayInstance.devArray.length; i++){
      if (ZigbeeGatewayInstance.devArray[i].data.guid == guid){
        // GUID found in the DevArray
        return true;
      }
    }
    // GUID not in the DevArray
    return false;
  }


  //----------------------------------------------------------------------------------------------------------
  // EVT()
  //  -Inputs: as many string input arguments as necessary
  //
  // Generate an event command string. This can be used generate command strings that will be received or sent
  // - A command string is composed of multiple arguments, joined via colons (:)
  // - To create a command, enter arguments in a respective order
  // 
  // ex) EVT(arg1, arg2, arg3) --> returns string  "arg1:arg2:arg3" 
  //----------------------------------------------------------------------------------------------------------
  function EVT(){
    evt_string = '';

    evt_string += arguments[0] + ':';

    for (var i = 1; i < arguments.length; i++) {
      evt_string += ':';
      evt_string += arguments[i];
    }

    return evt_string;
  }


  // ##########################################################################################
  //  DEVICE FUNCTIONS
  // ##########################################################################################  
  // -------------------------------------------------------------------------------------------
  // Define Dimmable light functions
  // -------------------------------------------------------------------------------------------  
  var OnOffLight = {

    //----------------------------------------------------------------------------------------------------------
    // set_ONOFF_state()
    //    -Inputs: Data Object with following properties:
    //                                                  - on
    //                                                  - ieee
    //                                                  - ep
    //    - Output: NULL
    //
    // set the ON/OFF state of a light
    //----------------------------------------------------------------------------------------------------------
    set_ONOFF_state: function(data){
      console.log("Toggle Light", data);
      
      hagateway.setState(data.on, data.ieee, data.ep, 
        function(status, seq){
          console.log("setState request seqNum: ", seq);
        if(status !== ZBL.STATUS.SUCCESS) {
          console.log("setState request confirmation failed: ", status);
        }
      });
    }, 

    //----------------------------------------------------------------------------------------------------------
    // get_ONOFF_state()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get the current ON/OFF state of the light
    //----------------------------------------------------------------------------------------------------------
    get_ONOFF_state: function(data){
      hagateway.getState(data.ieee, data.ep,
        function(status, seq){
          console.log("getState request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setState request confirmation failed: ", status);
          }
        });
    }
  };

  // -------------------------------------------------------------------------------------------
  // Define Dimmable light functions
  // -------------------------------------------------------------------------------------------  
  var DimmableLight = {

    //----------------------------------------------------------------------------------------------------------
    // set_ONOFF_state()
    //    -Inputs: Data Object with following properties:
    //                                                  - on
    //                                                  - ieee
    //                                                  - ep
    //    - Output: NULL
    //
    // set the ON/OFF state of a light
    //----------------------------------------------------------------------------------------------------------
    set_ONOFF_state: function(data){
      console.log("Toggle Light", data);
      
      hagateway.setState(data.on, data.ieee, data.ep, 
        function(status, seq){
          console.log("setState request seqNum: ", seq);
        if(status !== ZBL.STATUS.SUCCESS) {
          console.log("setState request confirmation failed: ", status);
        }
      });
    }, 

    //----------------------------------------------------------------------------------------------------------
    // get_ONOFF_state()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get the current ON/OFF state of the light
    //----------------------------------------------------------------------------------------------------------
    get_ONOFF_state: function(data){
      hagateway.getState(data.ieee, data.ep,
        function(status, seq){
          console.log("getState request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setState request confirmation failed: ", status);
          }
        });
    },


    //----------------------------------------------------------------------------------------------------------
    // get_level()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get the current level value of the light
    //----------------------------------------------------------------------------------------------------------
    get_level: function(data){
      hagateway.getLevel(data.ieee, data.ep,
        function(status, seq){
          console.log("getLevel request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("getLevel request confirmation failed: ", status);
          }
        });
    },


    //----------------------------------------------------------------------------------------------------------
    // set_level_transition()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //                                                  - level
    //                                                  - transitionTime
    //  - Output: NULL
    //
    // set the level and transition time of the light
    //----------------------------------------------------------------------------------------------------------
    set_level_transition: function(data){
      hagateway.setLevel(data.ieee, data.ep, data.level, data.transitionTime,
        function(status, seq){
          console.log("setLevel request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setLevel request confirmation failed: ", status);
          }
        });
    },

    //----------------------------------------------------------------------------------------------------------
    // set_onoff_transition()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //                                                  - transitionTime
    //  - Output: NULL
    //
    // set the level and transition time of the light
    //----------------------------------------------------------------------------------------------------------
    set_onoff_transition: function(data){
      hagateway.writeDeviceAttribute_uint16(data.ieee, data.ep, ZBL.HA_CLUSTER_ID.LEVEL_CONTROL, ZBL.LEVEL_CONTROL_CLUSTER.ON_OFF_TRANSITION_TIME, parseInt(data.transitionTime),
        function(status, seq){
          console.log("setOnOff request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setOnOff request confirmation failed: ", status);
          }
        });
    }
  };


  // -------------------------------------------------------------------------------------------
  // Define Color light functions
  // -------------------------------------------------------------------------------------------  
  var ColorLight = {
    //----------------------------------------------------------------------------------------------------------
    // set_ONOFF_state()
    //    -Inputs: Data Object with following properties:
    //                                                  - on
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // set the ON/OFF state of a light
    //----------------------------------------------------------------------------------------------------------
    set_ONOFF_state: function(data){
      console.log("Toggle Light", data);
      hagateway.setState(data.on, data.ieee, data.ep, 
        function(status, seq){
          console.log("setState request seqNum: ", seq);
        if(status !== ZBL.STATUS.SUCCESS) {
          console.log("setState request confirmation failed: ", status);
        }
      });
    }, 

    //----------------------------------------------------------------------------------------------------------
    // get_ONOFF_state()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get the current ON/OFF state of the light
    //----------------------------------------------------------------------------------------------------------
    get_ONOFF_state: function(data){
      hagateway.getState(data.ieee, data.ep,
        function(status, seq){
          console.log("getState request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("getState request confirmation failed: ", status);
          }
        });
    },


    //----------------------------------------------------------------------------------------------------------
    // set_level_transition()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //                                                  - level
    //                                                  - transitionTime
    //  - Output: NULL
    //
    // set the level and transition time of the light
    //----------------------------------------------------------------------------------------------------------
    set_level_transition: function(data){
      hagateway.setLevel(data.ieee, data.ep, data.level, data.transitionTime,
        function(status, seq){
          console.log("setLevel request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setLevel request confirmation failed: ", status);
          }
        });
    },

        
    //----------------------------------------------------------------------------------------------------------
    // set_onoff_transition()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //                                                  - transitionTime
    //  - Output: NULL
    //
    // set the level and transition time of the light
    //----------------------------------------------------------------------------------------------------------
    set_onoff_transition: function(data){
      hagateway.writeDeviceAttribute_uint16(data.ieee, data.ep, ZBL.HA_CLUSTER_ID.LEVEL_CONTROL, ZBL.LEVEL_CONTROL_CLUSTER.ON_OFF_TRANSITION_TIME, parseInt(data.transitionTime),
        function(status, seq){
          console.log("setOnOff request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setOnOff request confirmation failed: ", status);
          }
        });
    },


    //----------------------------------------------------------------------------------------------------------
    // set_color()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //                                                  - hue
    //                                                  - saturation
    //  - Output: NULL
    //
    // set the hue and saturation of the light
    //----------------------------------------------------------------------------------------------------------
    set_color: function(data){
      hagateway.setColor(data.ieee, data.ep, data.hue, data.saturation,
        function(status, seq){
          console.log("setColor request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setColor request confirmation failed: ", status);
          }
        });
    },


    //----------------------------------------------------------------------------------------------------------
    // get_color()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get the current color state of the color light
    //----------------------------------------------------------------------------------------------------------
    get_color: function(data){
      hagateway.getColor(data.ieee, data.ep,
        function(status, seq){
          console.log("getColor request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("getColor request confirmation failed: ", status);
          }
        });
    }
  };

  
  // -------------------------------------------------------------------------------------------
  // Define Temperature Sensor functions
  // -------------------------------------------------------------------------------------------  
  var TempSensor = {
    //----------------------------------------------------------------------------------------------------------
    // get_temp()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get the current temperature reading 
    //----------------------------------------------------------------------------------------------------------
    get_temp: function(data){
      console.log("Zb-Gateway: get the temp");
      hagateway.getTemp(data.ieee, data.ep, 
        function(status, seq){
          console.log("getTemp request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("getTemp request confirmation failed: ", status);
          }
        });
    },

    //----------------------------------------------------------------------------------------------------------
    // set_interval()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //                                                  - clusterId
    //                                                  - attrList
    //  - Output: NULL
    //
    // set the reporting interval of the temperature device
    //----------------------------------------------------------------------------------------------------------
    set_interval: function(data){
      console.log("Zb-Gateway: set the temp reporting interval");
      hagateway.setAttrReport(data.ieee, data.ep, data.clusterId, data.attrList, 
        function(status, seq){
          console.log("setInterval request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setInterval request confirmation failed: ", status);
          }
        });
    }
  };

  // -------------------------------------------------------------------------------------------
  // Define Switch Device functions
  // -------------------------------------------------------------------------------------------  
  var SwitchDevice = {    
    //----------------------------------------------------------------------------------------------------------
    // get_state()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get the current switch reading 
    //----------------------------------------------------------------------------------------------------------
    get_state: function(data){
      console.log("Zb-Gateway: get the state of the switch");
      hagateway.getState(data.ieee, data.ep, 
        function(status, seq){
          console.log("getState request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("getState request confirmation failed: ", status);
          }
        });
    }
  };


  // -------------------------------------------------------------------------------------------
  // Define Door Lock Device functions
  // -------------------------------------------------------------------------------------------  
  var DoorLockDevice = {
    //----------------------------------------------------------------------------------------------------------
    // get_state()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get the current door lock state reading 
    //----------------------------------------------------------------------------------------------------------
    get_state: function(data){
      console.log("Zb-Gateway: get the state of the door lock");
      hagateway.getDoorlockState(data.ieee, data.ep, 
        function(status, seq){
          console.log("getDoorlockState request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("getDoorlockState request confirmation failed: ", status);
          }
        });
    },

    //----------------------------------------------------------------------------------------------------------
    // unlock()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //                                                  - pin    
    //  - Output: NULL
    //
     // set the doorlock to unlock state 
    //----------------------------------------------------------------------------------------------------------
     unlock: function(data){
      console.log("Zb-Gateway: unlock the door lock");
      hagateway.setDoorlockState(data.ieee, data.ep, ZBL.DOOR_LOCK_CLUSTER_LockMode.LOCK_MODE_UNLOCK, data.pin,
        function(status, seq){
          console.log("setDoorLockState request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setDoorLockState request confirmation failed: ", status);
          }
        });
    },

    //----------------------------------------------------------------------------------------------------------
    // lock()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //                                                  - pin    
    //  - Output: NULL
    //
     // set the doorlock to lock state 
    //----------------------------------------------------------------------------------------------------------
     lock: function(data){
      console.log("Zb-Gateway: lock the door lock");
      hagateway.setDoorlockState(data.ieee, data.ep, ZBL.DOOR_LOCK_CLUSTER_LockMode.LOCK_MODE_LOCK, data.pin,
        function(status, seq){
          console.log("setDoorLockState request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setDoorLockState request confirmation failed: ", status);
          }
        });
    }
  };


  // -------------------------------------------------------------------------------------------
  // Define Thermostat Device functions
  // -------------------------------------------------------------------------------------------  
  var ThermostatDevice = {
    //----------------------------------------------------------------------------------------------------------
    // setpoint_change()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //                                                  - mode  
    //                                                  - amount    
    //  - Output: NULL
    //
    // set the setpoint change 
    //----------------------------------------------------------------------------------------------------------
    setpoint_change: function(data){
      console.log("Zb-Gateway: setpoint change of the thermostat");
      hagateway.setpointChange(data.ieee, data.ep, data.mode, data.amount,
        function(status, seq){
          console.log("setSetpointChange request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setSetpointChange request confirmation failed: ", status);
          }
        });
    },

    //----------------------------------------------------------------------------------------------------------
    // get_minheat_setpoint()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get the current min heat setpoint  
    //----------------------------------------------------------------------------------------------------------
    get_minheat_setpoint: function(data){
      console.log("Zb-Gateway: get the min heatpoint setpoint");
      hagateway.readDeviceAttribute(data.ieee, data.ep, ZBL.HA_CLUSTER_ID.THERMOSTAT, [ZBL.THERMOSTAT_CLUSTER.MinHeatSetpointLimit],
        function(status, seq){
          console.log("getAttribute request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("getAttribute request confirmation failed: ", status);
          }
        });
    },

    //----------------------------------------------------------------------------------------------------------
    // get_all_attributes()
    //    -Inputs: Data Object with following properties:
    //                                                  - ieee
    //                                                  - ep
    //  - Output: NULL
    //
    // get all of the attributes for the thermostat
    //----------------------------------------------------------------------------------------------------------
    get_all_attributes: function(data){
      var attributes = [
        ZBL.THERMOSTAT_CLUSTER.LocalTemperature,
        ZBL.THERMOSTAT_CLUSTER.PICoolingDemand,
        ZBL.THERMOSTAT_CLUSTER.PIHeatingDemand,
        ZBL.THERMOSTAT_CLUSTER.OccupiedCoolingSetpoint,
        ZBL.THERMOSTAT_CLUSTER.OccupiedHeatingSetpoint,
        ZBL.THERMOSTAT_CLUSTER.MinHeatSetpointLimit,
        ZBL.THERMOSTAT_CLUSTER.MaxHeatSetpointLimit,
        ZBL.THERMOSTAT_CLUSTER.MinCoolSetpointLimit,
        ZBL.THERMOSTAT_CLUSTER.MaxCoolSetpointLimit,
        ZBL.THERMOSTAT_CLUSTER.ControlSequenceOfOperation,
        ZBL.THERMOSTAT_CLUSTER.SystemMode
      ];

      console.log("Zb-Gateway: get all thermostat attributes");
      hagateway.readDeviceAttribute(data.ieee, data.ep, ZBL.HA_CLUSTER_ID.THERMOSTAT, attributes,
        function(status, seq){
          console.log("getAttribute request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("getAttribute request confirmation failed: ", status);
          }
        });
    }    
  };


  // -------------------------------------------------------------------------------------------
  // Define Device Binding functions
  // -------------------------------------------------------------------------------------------  
  var DeviceBinding = {

    //----------------------------------------------------------------------------------------------------------
    // set_binding()
    //    -Inputs: Data Object with following properties:
    //                                                  - source_ieee
    //                                                  - source_ep
    //                                                  - dest_ieee
    //                                                  - dest_ep
    //                                                  - clusterId
    //  - Output: NULL
    //
    // set a binding request between a source and destination device
    //----------------------------------------------------------------------------------------------------------
    set_binding: function(data){
      console.log("Zb-Gateway: set a binding between devices");
      nwkmgr.setDevBinding(0, data.source_ieee, data.source_ep, data.dest_ieee, data.dest_ep, data.clusterId,
        function(status, seq){
          console.log("setBinding request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setBinding request confirmation failed: ", status);
            ZigbeeGatewayInstance.emit('zb-gateway:binding:failed');
          }
        });
    },

    //----------------------------------------------------------------------------------------------------------
    // set_unbinding()
    //    -Inputs: Data Object with following properties:
    //                                                  - source_ieee
    //                                                  - source_ep
    //                                                  - dest_ieee
    //                                                  - dest_ep
    //                                                  - clusterId
    //  - Output: NULL
    //
    // set an un-binding request between a source and destination device
    //----------------------------------------------------------------------------------------------------------
    set_unbinding: function(data){
      console.log("Zb-Gateway: set an un-binding between devices");
      nwkmgr.setDevBinding(1, data.source_ieee, data.source_ep, data.dest_ieee, data.dest_ep, data.clusterId,
        function(status, seq){
          console.log("setUnBinding request seqNum: ", seq);
          if(status !== ZBL.STATUS.SUCCESS){
            console.log("setUnBinding request confirmation failed: ", status);
            ZigbeeGatewayInstance.emit('zb-gateway:unbinding:failed');
          }
        });
    }
  };


  // ##########################################################################################
  //  EXTERNAL FUNCTIONS
  // ##########################################################################################  

  /*!
	* @brief    Sends a requets to get network info
	*
	* @param 		none
	*
	* @return   null
	*/
  ZigbeeGateway.prototype.getNetworkInfo = function(){
    zb_gateway_getNetworkInfo();
  };

  
  /*!
	* @brief    Allows to open the nework for device joining
	*
	* @param 		none
	*
	* @return   null
	*/
  ZigbeeGateway.prototype.openNetwork = function(){
    zb_gateway_openNetwork();
  };


  /*!
	* @brief     Allows to close the nework for device joining
	*
	* @param 		none
	*
	* @return   null
	*/
  ZigbeeGateway.prototype.closeNetwork = function(){
    zb_gateway_closeNetwork();
  };

  /*!
	* @brief     Remove a device from the network
	*
	* @param 		none
	*
	* @return   null
	*/
  ZigbeeGateway.prototype.removeDevice = function(data){
    zb_gateway_removeDevice(data);
  };


  /*!
	* @brief    Perform a hard system reset 
	*
	* @param 		none
	*
	* @return   null
	*/
  ZigbeeGateway.prototype.hardSystemReset = function(data){
    zb_gateway_hardSystemReset();
  };


    /*!
	* @brief    Perform a soft system reset 
	*
	* @param 		none
	*
	* @return   null
	*/
  ZigbeeGateway.prototype.softSystemReset = function(data){
    zb_gateway_softSystemReset();
  };


  /*!
	* @brief     Get the latest Device lists
	*
	* @param 		none
	*
	* @return   null
	*/
  ZigbeeGateway.prototype.getDeviceList = function(){
    zb_gateway_getDeviceList();
  };
  

  /*!
	* @brief     Get the latest Binding list 
	*
	* @param 		none
	*
	* @return   null
	*/
  ZigbeeGateway.prototype.getBindList = function(){
    zb_gateway_getBindList();
  };

  ZigbeeGateway.prototype.OnOffLight = OnOffLight;

  ZigbeeGateway.prototype.DimmableLight = DimmableLight;

  ZigbeeGateway.prototype.ColorLight = ColorLight;

  ZigbeeGateway.prototype.TempSensor = TempSensor;

  ZigbeeGateway.prototype.SwitchDevice = SwitchDevice;

  ZigbeeGateway.prototype.DoorLockDevice = DoorLockDevice;

  ZigbeeGateway.prototype.ThermostatDevice = ThermostatDevice;

  ZigbeeGateway.prototype.DeviceBinding = DeviceBinding;
    
  //create the devices
  nwkmgr.getDevList();

}

ZigbeeGateway.prototype.__proto__ = events.EventEmitter.prototype;
 
// export the class
module.exports = ZigbeeGateway;
