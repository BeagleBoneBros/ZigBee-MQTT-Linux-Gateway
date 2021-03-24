/******************************************************************************

 @file MQTTCloudAdapter.js

 @brief Adapter to send and receive messages from an MQTT broker

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
var mqtt = require('mqtt');
var ZigbeeGateway = require("../zb_gateway/zb-gateway.js");

var MQTTCloudAdapterInstance;

function MQTTCloudAdapter(ip_address) {

  // Make sure there is only one instance created
  if (typeof MQTTCloudAdapterInstance !== "undefined") {
    return MQTTCloudAdapterInstance;
  }

  /* Set up to emit events */
  events.EventEmitter.call(this);
  MQTTCloudAdapterInstance = this;

  // import MQTT config settings
  var MQTTConfig = require('./MQTTConfig.json');

  // connect to MQTT broker and create MQTT client object
  var gatewayClient  = mqtt.connect(MQTTConfig.broker_address, {username:"mosquitto"})

  // create Zigbee Gateway
  var zigbeeGateway = new ZigbeeGateway(ip_address);

  // once connected MQTT Broker, subscribe the device commands
  gatewayClient.on('connect', function(){
    MQTTCloudAdapterInstance.connected = true;
    console.log("Connected to MQTT Broker!");

    // HA-Gateway Subscriptions
    gatewayClient.subscribe(MQTTConfig.gateway_getDeviceList_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.gateway_getBindList_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.gateway_removeDevice_topic, MQTTConfig.sub_options);
    // Network Subscriptions
	gatewayClient.subscribe(MQTTConfig.network_open_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.network_close_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.network_getinfo_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.network_hard_reset_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.network_soft_reset_topic, MQTTConfig.sub_options);
    // Light Device Subscriptions
	gatewayClient.subscribe(MQTTConfig.light_set_ONOFF_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.light_get_ONOFF_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.light_set_level_transition_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.light_set_onoff_transition_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.light_get_level_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.light_set_color_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.light_get_color_topic, MQTTConfig.sub_options);
    // Temp Sensor Device Subscriptions
	gatewayClient.subscribe(MQTTConfig.temp_get_temp_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.temp_set_interval_topic, MQTTConfig.sub_options);
    // Doorlock Device Subscriptions
	gatewayClient.subscribe(MQTTConfig.doorlock_get_state_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.doorlock_set_unlock_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.doorlock_set_lock_topic, MQTTConfig.sub_options);
    // Thermostat Device Subscriptions
	gatewayClient.subscribe(MQTTConfig.thermostat_change_setpoint_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.thermostat_get_minheat_setpoint_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.thermostat_get_all_attributes, MQTTConfig.sub_options);
    // Binding Device Subscriptions
	gatewayClient.subscribe(MQTTConfig.binding_set_bind_topic, MQTTConfig.sub_options);
	gatewayClient.subscribe(MQTTConfig.binding_set_unbind_topic, MQTTConfig.sub_options);
  });
  
//handle errors
	client.on("error",function(error){
	console.log("Can't connect to MQTT: " + error);
	});

  // handle incoming commands from Cloud
  gatewayClient.on('message', function(topic, msgData, packet){
    console.log("Message Topic: %s  Message: %s  \n" ,topic, msgData);


    switch (topic){
      // ----------------- HA-Gateway Control ----------------------------
      case MQTTConfig.gateway_getDeviceList_topic:
        zigbeeGateway.getDeviceList();
        break;

      case MQTTConfig.gateway_getBindList_topic:
        zigbeeGateway.getBindList();
        break;

      case MQTTConfig.gateway_removeDevice_topic:
        zigbeeGateway.removeDevice(msgData);
        break;

      // ----------------- Network Control ----------------------------
      case MQTTConfig.network_open_topic:
        zigbeeGateway.openNetwork();
        break;
        
      case MQTTConfig.network_close_topic:
        zigbeeGateway.closeNetwork();
        break;
      
      case MQTTConfig.network_getinfo_topic:
        zigbeeGateway.getNetworkInfo();
        break;

      case MQTTConfig.network_hard_reset_topic:
        zigbeeGateway.hardSystemReset();
        break;

      case MQTTConfig.network_soft_reset_topic:
        zigbeeGateway.softSystemReset();
        break;

      // ----------------- Light Device ----------------------------
      case MQTTConfig.light_set_ONOFF_topic:
        zigbeeGateway.DimmableLight.set_ONOFF_state(msgData);
        break;

      case MQTTConfig.light_get_ONOFF_topic:
        zigbeeGateway.DimmableLight.get_ONOFF_state(msgData);
        break;

      case MQTTConfig.light_set_level_transition_topic:
        zigbeeGateway.DimmableLight.set_level_transition(msgData);
        break;

      case MQTTConfig.light_set_onoff_transition_topic:
        zigbeeGateway.DimmableLight.set_onoff_transition(msgData);
        break;

      case MQTTConfig.light_get_level_topic:
        zigbeeGateway.DimmableLight.get_level(msgData);
        break;

      case MQTTConfig.light_set_color_topic:
        zigbeeGateway.ColorLight.set_color(msgData);
        break;

      case MQTTConfig.light_get_color_topic:
        zigbeeGateway.ColorLight.get_color(msgData);
        break;

      // ----------------- Temeprature Sensor Device ----------------------------
      case MQTTConfig.temp_get_temp_topic:
        zigbeeGateway.TempSensor.get_temp(msgData);
        break;

      case MQTTConfig.temp_set_interval_topic:
        zigbeeGateway.TempSensor.set_interval(msgData);
        break;

      // ----------------- DoorLock Device ----------------------------
      case MQTTConfig.doorlock_get_state_topic:
        zigbeeGateway.DoorLockDevice.get_state(msgData);
        break;

      case MQTTConfig.doorlock_set_unlock_topic:
        zigbeeGateway.DoorLockDevice.unlock(msgData);
        break;

      case MQTTConfig.doorlock_set_lock_topic:
        zigbeeGateway.DoorLockDevice.lock(msgData);
        break;

      // ----------------- Thermostat Device ----------------------------
      case MQTTConfig.thermostat_change_setpoint_topic:
        zigbeeGateway.ThermostatDevice.setpoint_change(msgData);
        break;

      case MQTTConfig.thermostat_get_minheat_setpoint_topic:
        zigbeeGateway.ThermostatDevice.get_minheat_setpoint(msgData);
        break;

      case MQTTConfig.thermostat_get_all_attributes:
        zigbeeGateway.ThermostatDevice.get_all_attributes(msgData);
        break;


      // ----------------- Device Binding ----------------------------
      case MQTTConfig.binding_set_bind_topic:
        zigbeeGateway.DeviceBinding.set_binding(msgData);
        break;

      case MQTTConfig.binding_set_unbind_topic:
        zigbeeGateway.DeviceBinding.set_unbinding(msgData);
        break;
      
    }
  }); 


  //---------------------------------------------------------------------------------------------------------
  // Handle Events from the ZigBee Gateway
  //---------------------------------------------------------------------------------------------------------
  // ################################# Push New Device Info to Cloud ########################################
  zigbeeGateway.on('zb-gateway:newDev', function (devList, unbindedList, bindedList) {
    console.log("MQTTAdapter: Received zb-gateway:newDev", devList, "The unbindedList: ", unbindedList, "The BindedList: ", bindedList);
    
    var listData = {
      dL: devList,
      uL: unbindedList,
      bL: bindedList
    };
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(listData);
		gatewayClient.publish(MQTTConfig.server_devList_topic, msgData, MQTTConfig.sub_options);  
	}
  })

  // ############################### Push Cuurent Device List to Cloud ######################################
  .on('zb-gateway:deviceList', function (devList, unbindedList, bindedList) {
    console.log("MQTTAdapter: Received zb-gateway:deviceList", devList, "The unbindedList: ", unbindedList, "The BindedList: ", bindedList);
    
    var listData = {
      dL: devList,
      uL: unbindedList,
      bL: bindedList
    };

    if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(listData);
		gatewayClient.publish(MQTTConfig.server_devList_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ############################ Send Remove Device Card Command to Cloud ##################################
  .on('zb-gateway:removeDeviceCard', function (data) {
    console.log("MQTTAdapter: Received removeDeviceCard");

	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(data);
		gatewayClient.publish(MQTTConfig.server_removeDeviceCard_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ############################## Send Network Ready Flag to Cloud ########################################
  .on('zb-gateway:network:ready', function () {
    console.log("MQTTCloudAdapter: Received Network Ready Ind");

	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		//gatewayClient.publish(MQTTConfig.server_network_ready'_topic, "", MQTTConfig.sub_options);  
	}  
  })

  // ################################ Send Network Info to Cloud ############################################
  .on('zb-gateway:network:info', function (data) {
    console.log("MQTTCloudAdapter: Received Network Info Ind");

	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(data);
		gatewayClient.publish(MQTTConfig.server_network_info_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ################################ Send Binding Event to Cloud ###########################################
  .on('zb-gateway:binding:event', function (devList, unbindedList, bindedList) {
    console.log("MQTTCloudAdapter: Received zb-gateway:binding:event, unbindedList: ", unbindedList, "The BindedList: ", bindedList);
    
    var listData = {
      dL: devList,
      uL: unbindedList,
      bL: bindedList
    };


	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(listData);
		gatewayClient.publish(MQTTConfig.server_binding_event_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ############################ Send Binding Failed Command to Cloud ######################################
  .on('zb-gateway:binding:failed', function () {
    console.log("MQTTCloudAdapter: Received zb-gateway:binding:failed");
    
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);

		//gatewayClient.publish(MQTTConfig.server_binding_failed_topic, "", MQTTConfig.sub_options);  
	}  	
  })

  // ########################### Send UnBinding Failed Command to Cloud #####################################
  .on('zb-gateway:unbinding:failed', function () {
    console.log("MQTTCloudAdapter: Received zb-gateway:unbinding:failed");
  
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);

		//gatewayClient.publish(MQTTConfig.server_unbinding_failed_topic, "", MQTTConfig.sub_options);  
	}  
  })

  // ############################ Send Light Device State to Cloud ##########################################
  .on('zb-gateway:light_device:state', function(data) {
    console.log("MQTTCloudAdapter: Got light device state update: ", data); 
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(data);
		gatewayClient.publish(MQTTConfig.server_light_device_state_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ############################ Send Light Device Level to Cloud ##########################################
  .on('zb-gateway:light_device:level', function(data) {
    console.log("MQTTCloudAdapter: Got light device state update: ", data);  
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(data);
		gatewayClient.publish(MQTTConfig.server_light_device_level_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ############################ Send Light Device Color to Cloud ##########################################
  .on('zb-gateway:light_device:color', function(data) {
    console.log("MQTTCloudAdapter: Got light device color update: ", data);
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(data);
		gatewayClient.publish(MQTTConfig.server_light_device_color_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ######################### Send Temperature Device Temp to Cloud ########################################
  .on('zb-gateway:temp_device:temp', function(data) {
    console.log("MQTTCloudAdapter: Got temp device state update: ", data);
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(data);
		gatewayClient.publish(MQTTConfig.server_temp_device_temp_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ########################## Send Doorlock Device State to Cloud #########################################
  .on('zb-gateway:doorlock_device:state', function(data) {
    console.log("MQTTCloudAdapter: Got doorlock device state update: ", data);
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(data);
		gatewayClient.publish(MQTTConfig.server_doorlock_device_state_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ###################### Send Doorlock Device Set Response to Cloud ######################################
  .on('zb-gateway:doorlock_device:set_rsp', function(data) {
    console.log("MQTTCloudAdapter: Got doorlock device set response: ", data);
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(data);
		gatewayClient.publish(MQTTConfig.server_doorlock_device_set_rsp_topic, msgData, MQTTConfig.sub_options);  
	}  
  })

  // ###################### Send Thermostat Device Attribute to Cloud #######################################
  .on('zb-gateway:thermostat:attribute', function(data) {
    console.log("MQTTCloudAdapter: Got thermostat device attribute response: ", data);
	if(gatewayClient.connected == true){
		console.log("MQTT Connection Status: ", gatewayClient.connected);
		msgData = JSON.stringify(data);
		gatewayClient.publish(MQTTConfig.server_thermostat_attribute_topic, msgData, MQTTConfig.sub_options);  
	}  
  });


}


MQTTCloudAdapter.prototype.__proto__ = events.EventEmitter.prototype;

module.exports = MQTTCloudAdapter;