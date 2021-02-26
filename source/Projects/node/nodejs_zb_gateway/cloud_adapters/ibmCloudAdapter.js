/******************************************************************************

 @file ibmCloudAdapter.js

 @brief Adapter to send and receive messages from an IBM Cloud IoT Service

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

var events = require("events");
var ibmIot = require('ibmiotf');
var ZigbeeGateway = require("../zb_gateway/zb-gateway.js");

var ibmCloudAdapterInstance;

function IbmCloudAdapter(ip_address) {

  // Make sure there is only one instance created
  if (typeof ibmCloudAdapterInstance !== "undefined") {
    return ibmCloudAdapterInstance;
  }

  /* Set up to emit events */
  events.EventEmitter.call(this);
  ibmCloudAdapterInstance = this;

  // import ibm config settings
  var ibmConfig = require('./ibmConfig.json');

  // create the IBM IoT Gateway Object
  var gatewayClient = new ibmIot.IotfGateway(ibmConfig);

  // connect the gateway client to the Cloud Service
  gatewayClient.log.setLevel('debug');
  gatewayClient.connect();

  // create Zigbee Gateway
  var zigbeeGateway = new ZigbeeGateway(ip_address);

  // once connected to the IBM Cloud, subscribe the device commands
  gatewayClient.on('connect', function(){
    ibmCloudAdapterInstance.connected = true;
    console.log("Connected to IBM Watson!");

    // HA-Gateway Subscriptions
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:getDeviceList');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:getBindList');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:removeDevice');
    // Network Subscriptions
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:network:open');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:network:close');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:network:getinfo');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:network:hard:reset');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:network:soft:reset');
    // Light Device Subscriptions
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:light_device:set:ONOFF');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:light_device:get:ONOFF');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:light_device:set:level_transition');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:light_device:set:onoff_transition');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:light_device:get:level');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:light_device:set:color');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:light_device:get:color');
    // Temp Sensor Device Subscriptions
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:temp_device:get:temp');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:temp_device:set:interval');
    // Doorlock Device Subscriptions
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:doorlock_device:get:state');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:doorlock_device:set:unlock');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:doorlock_device:set:lock');
    // Thermostat Device Subscriptions
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:thermostat_device:change:setpoint');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:thermostat_device:get:minheat:setpoint');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:thermostat_device:get:all:attributes');
    // Binding Device Subscriptions
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:binding:set:bind');
    gatewayClient.subscribeToDeviceCommand(ibmConfig.type, ibmConfig.id,'cloud:binding:set:unbind');
  });

  // handle incoming commands from Cloud
  gatewayClient.on('command', function(type, id, commandName, commandFormat, payload, topic){
    console.log("Type: %s  ID: %s  \nCommand Name : %s Format: %s",type, id, commandName, commandFormat);

    if(payload.length > 0){
      msgData = bufferPayloadToObject(payload);
    }

    switch (commandName){
      // ----------------- HA-Gateway Control ----------------------------
      case "cloud:getDeviceList":
        zigbeeGateway.getDeviceList();
        break;

      case "cloud:getBindList":
        zigbeeGateway.getBindList();
        break;

      case "cloud:removeDevice":
        zigbeeGateway.removeDevice(msgData);
        break;

      // ----------------- Network Control ----------------------------
      case "cloud:network:open":
        zigbeeGateway.openNetwork();
        break;
        
      case "cloud:network:close":
        zigbeeGateway.closeNetwork();
        break;
      
      case "cloud:network:getinfo":
        zigbeeGateway.getNetworkInfo();
        break;

      case "cloud:network:hard:reset":
        zigbeeGateway.hardSystemReset();
        break;

      case "cloud:network:soft:reset":
        zigbeeGateway.softSystemReset();
        break;

      // ----------------- Light Device ----------------------------
      case "cloud:light_device:set:ONOFF":
        zigbeeGateway.DimmableLight.set_ONOFF_state(msgData);
        break;

      case "cloud:light_device:get:ONOFF":
        zigbeeGateway.DimmableLight.get_ONOFF_state(msgData);
        break;

      case "cloud:light_device:set:level_transition":
        zigbeeGateway.DimmableLight.set_level_transition(msgData);
        break;

      case "cloud:light_device:set:onoff_transition":
        zigbeeGateway.DimmableLight.set_onoff_transition(msgData);
        break;

      case "cloud:light_device:get:level":
        zigbeeGateway.DimmableLight.get_level(msgData);
        break;

      case "cloud:light_device:set:color":
        zigbeeGateway.ColorLight.set_color(msgData);
        break;

      case "cloud:light_device:get:color":
        zigbeeGateway.ColorLight.get_color(msgData);
        break;

      // ----------------- Temeprature Sensor Device ----------------------------
      case "cloud:temp_device:get:temp":
        zigbeeGateway.TempSensor.get_temp(msgData);
        break;

      case "cloud:temp_device:set:interval":
        zigbeeGateway.TempSensor.set_interval(msgData);
        break;

      // ----------------- DoorLock Device ----------------------------
      case "cloud:doorlock_device:get:state":
        zigbeeGateway.DoorLockDevice.get_state(msgData);
        break;

      case "cloud:doorlock_device:set:unlock":
        zigbeeGateway.DoorLockDevice.unlock(msgData);
        break;

      case "cloud:doorlock_device:set:lock":
        zigbeeGateway.DoorLockDevice.lock(msgData);
        break;

      // ----------------- Thermostat Device ----------------------------
      case "cloud:thermostat_device:change:setpoint":
        zigbeeGateway.ThermostatDevice.setpoint_change(msgData);
        break;

      case "cloud:thermostat_device:get:minheat:setpoint":
        zigbeeGateway.ThermostatDevice.get_minheat_setpoint(msgData);
        break;

      case "cloud:thermostat_device:get:all:attributes":
        zigbeeGateway.ThermostatDevice.get_all_attributes(msgData);
        break;


      // ----------------- Device Binding ----------------------------
      case "cloud:binding:set:bind":
        zigbeeGateway.DeviceBinding.set_binding(msgData);
        break;

      case "cloud:binding:set:unbind":
        zigbeeGateway.DeviceBinding.set_unbinding(msgData);
        break;
      
    }
  }); 


  gatewayClient.on('error', function(error){
    console.log("IBM Cloud Adapter error: "+error);
  });


  gatewayClient.on('reconnect', function(){
    console.log("Reconnected to IBM!");
  });

  //---------------------------------------------------------------------------------------------------------
  // Handle Events from the ZigBee Gateway
  //---------------------------------------------------------------------------------------------------------
  // ################################# Push New Device Info to Cloud ########################################
  zigbeeGateway.on('zb-gateway:newDev', function (devList, unbindedList, bindedList) {
    console.log("IBMCloudAdapter: Received zb-gateway:newDev", devList, "The unbindedList: ", unbindedList, "The BindedList: ", bindedList);
    
    var listData = {
      dL: devList,
      uL: unbindedList,
      bL: bindedList
    };

    console.log("Connection Status: ", gatewayClient.isConnected);
    msgData = JSON.stringify(listData);
    gatewayClient.publishGatewayEvent('devList', "json", msgData);  
  })

  // ############################### Push Cuurent Device List to Cloud ######################################
  .on('zb-gateway:deviceList', function (devList, unbindedList, bindedList) {
    console.log("IBMCloudAdapter: Received zb-gateway:deviceList", devList, "The unbindedList: ", unbindedList, "The BindedList: ", bindedList);
    
    var listData = {
      dL: devList,
      uL: unbindedList,
      bL: bindedList
    };

    // convert JS Object to String
    devListString = JSON.stringify(listData);
    gatewayClient.publishGatewayEvent('devList', "json", devListString);  
  })

  // ############################ Send Remove Device Card Command to Cloud ##################################
  .on('zb-gateway:removeDeviceCard', function (data) {
    console.log("IBMCloudAdapter: Received removeDeviceCard");
    // convert JS Object to String
    dataString = JSON.stringify(data);
    gatewayClient.publishGatewayEvent('webserver:removeDeviceCard', "json", dataString);  
  })

  // ############################## Send Network Ready Flag to Cloud ########################################
  .on('zb-gateway:network:ready', function () {
    console.log("IBMCloudAdapter: Received Network Ready Ind");
    gatewayClient.publishGatewayEvent('webserver:network:ready', "json", '{"network": 0}');  
  })

  // ################################ Send Network Info to Cloud ############################################
  .on('zb-gateway:network:info', function (data) {
    console.log("IBMCloudAdapter: Received Network Info Ind");
    // convert JS Object to String
    dataString = JSON.stringify(data);
    gatewayClient.publishGatewayEvent('webserver:network:info', "json", dataString);  
  })

  // ################################ Send Binding Event to Cloud ###########################################
  .on('zb-gateway:binding:event', function (devList, unbindedList, bindedList) {
    console.log("IBMCloudAdapter: Received zb-gateway:binding:event, unbindedList: ", unbindedList, "The BindedList: ", bindedList);
    
    var listData = {
      dL: devList,
      uL: unbindedList,
      bL: bindedList
    };

    // convert JS Object to String
    devListString = JSON.stringify(listData);
    gatewayClient.publishGatewayEvent('webserver:bindingDevList', "json", devListString);  
  })

  // ############################ Send Binding Failed Command to Cloud ######################################
  .on('zb-gateway:binding:failed', function () {
    console.log("IBMCloudAdapter: Received zb-gateway:binding:failed");
    gatewayClient.publishGatewayEvent('webserver:binding:failed', "json", '{"binding": 1}');  
  })

  // ########################### Send UnBinding Failed Command to Cloud #####################################
  .on('zb-gateway:unbinding:failed', function () {
    console.log("IBMCloudAdapter: Received zb-gateway:unbinding:failed");
    gatewayClient.publishGatewayEvent('webserver:unbinding:failed', "json", '{"binding": 1}');  
  })

  // ############################ Send Light Device State to Cloud ##########################################
  .on('zb-gateway:light_device:state', function(data) {
    console.log("IBMCloudAdapter: Got light device state update: ", data);
    dataString = JSON.stringify(data);
    gatewayClient.publishGatewayEvent('webserver:light_device:state', "json", dataString);  
  })

  // ############################ Send Light Device Level to Cloud ##########################################
  .on('zb-gateway:light_device:level', function(data) {
    console.log("IBMCloudAdapter: Got light device state update: ", data);
    gatewayClient.publishGatewayEvent('webserver:light_device:level', "json", data);  
  })

  // ############################ Send Light Device Color to Cloud ##########################################
  .on('zb-gateway:light_device:color', function(data) {
    console.log("IBMCloudAdapter: Got light device color update: ", data);
    gatewayClient.publishGatewayEvent('webserver:light_device:color', "json", data);  
  })

  // ######################### Send Temperature Device Temp to Cloud ########################################
  .on('zb-gateway:temp_device:temp', function(data) {
    console.log("IBMCloudAdapter: Got temp device state update: ", data);
    gatewayClient.publishGatewayEvent('webserver:temp_device:temp', "json", data);  
  })

  // ########################## Send Doorlock Device State to Cloud #########################################
  .on('zb-gateway:doorlock_device:state', function(data) {
    console.log("IBMCloudAdapter: Got doorlock device state update: ", data);
    gatewayClient.publishGatewayEvent('webserver:doorlock_device:state', "json", data);  
  })

  // ###################### Send Doorlock Device Set Response to Cloud ######################################
  .on('zb-gateway:doorlock_device:set_rsp', function(data) {
    console.log("IBMCloudAdapter: Got doorlock device set response: ", data);
    gatewayClient.publishGatewayEvent('webserver:doorlock_device:set_rsp', "json", data);  
  })

  // ###################### Send Thermostat Device Attribute to Cloud #######################################
  .on('zb-gateway:thermostat:attribute', function(data) {
    console.log("IBMCloudAdapter: Got thermostat device attribute response: ", data);
    gatewayClient.publishGatewayEvent('webserver:thermostat:attribute', "json", data);  
  });

  
  // ########################################################################################################
  // ##################################### Local Functions ##################################################
  // ########################################################################################################

  //----------------------------------------------------------------------------------------------------------
  // EVT()
  //  -Inputs: as many string input arguments as necessary (strings)
  //  -Output: concatenation of all input strings, separated by ":" (string)
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

  //----------------------------------------------------------------------------------------------------------
  // bufferPayloadToObject()
  //    -Inputs: JavaScript Object with nested Buffer elements
  //    -Output: Javascript Object
  //
  // convert an incoming buffer back to a JS object. 
  // Searches in a javscript object for all buffer types with the following keys:
  //  - "ieee"
  //  - "source_ieee"
  //  - "dest_ieee"
  // and converts them to javascript objects
  //----------------------------------------------------------------------------------------------------------
  function bufferPayloadToObject(obj) {
    if(Buffer.isBuffer(obj)){
      obj = JSON.parse(obj);
    }

    Object.keys(obj).forEach(function (key) {
        var value = obj[key];

        if (typeof value === 'object') {
          bufferPayloadToObject(value);
        }

        // if we found the key same as query, and it is a buffer, convert from JSON to Buffer
        if (key === 'ieee' || key === 'source_ieee' || key === 'dest_ieee') {
          if(obj[key].type == "Buffer"){
            obj[key] = Buffer.from(obj[key].data);
          }
        }
    });

    return obj;
  }

}


IbmCloudAdapter.prototype.__proto__ = events.EventEmitter.prototype;

module.exports = IbmCloudAdapter;