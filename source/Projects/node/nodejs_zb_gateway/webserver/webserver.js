/******************************************************************************

 @file weberver.js

 @brief Webserver to host a local webapp

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

var express = require('express');
var http = require('http');
var socket_io = require('socket.io');
var events = require('events');
var ZBL = require('../zigbee_library/zb_lib.js');
var ZigbeeGateway = require("../zb_gateway/zb-gateway.js");

// Constructor
function Webserver(ip_address) {
  // create the Zigbee Gateway
  var zigbeeGateway = new ZigbeeGateway(ip_address);

  // setup event emitter
  events.EventEmitter.call(this);
  webserverInstance = this;
  
  // define the port that the webapp will be hosted on
  port = 5000;

  /* Set up webserver */
	var	app = express();
	var	server = http.createServer(app);
	webserverInstance.io = socket_io.listen(server);
	server.listen(port, getIPAddress());
	var path = require('path');
	app.use(express.static(path.join(__dirname, '/public')));
	app.get('/', function(req, res){
		res.sendFile(path.join(__dirname, '/public', 'webapp.html'));
  });
  
  // print webserver IP address
  console.log('webserver: Server running on http://' + getIPAddress() + ':5000');

  //-----------------------------------------------------------------------------------------------
  // Handle Events from the ZigBee Gateway
  //-----------------------------------------------------------------------------------------------
  // ######################### Zigbee Network - New Device ########################################
  zigbeeGateway.on('zb-gateway:newDev', function (devList, unbindedList, bindedList) {    
    var listData = {
      dL: devList,
      uL: unbindedList,
      bL: bindedList
    };

    webserverInstance.io.sockets.emit('devList', listData);   
  })
  // ####################### Zigbee Network - Latest Device List ##################################
  .on('zb-gateway:deviceList', function (devList, unbindedList, bindedList) {
    console.log("Webserver: Received zb-gateway:deviceList", devList, "The unbindedList: ", unbindedList, "The BindedList: ", bindedList);
    
    var listData = {
      dL: devList,
      uL: unbindedList,
      bL: bindedList
    };

    webserverInstance.io.sockets.emit('devList', listData);   
  })
  // ####################### Zigbee Network - Remove Device Card ##################################
  .on('zb-gateway:removeDeviceCard', function(data) {
    console.log("Webserver: Received Remove Device Card");
    webserverInstance.io.sockets.emit('webserver:removeDeviceCard', data);  
  })
  // ################### Zigbee Network - Network is Connected/Ready ##############################
  .on('zb-gateway:network:ready', function () {
    console.log("Webserver: Received Network Ready Ind");
    webserverInstance.io.sockets.emit('webserver:network:ready');  
  })
  // ###################### Zigbee Network - Latest Network Info ##################################
  .on('zb-gateway:network:info', function (data) {
    console.log("Webserver: Received Network Info Ind");
    webserverInstance.io.sockets.emit('webserver:network:info', data);  
  })
  // ####################### Zigbee Network - Binding Event #######################################
  .on('zb-gateway:binding:event', function (devList, unbindedList, bindedList) {
    console.log("Webserver: Received zb-gateway:binding:event, unbindedList: ", unbindedList, "The BindedList: ", bindedList);
    
    var listData = {
      dL: devList,
      uL: unbindedList,
      bL: bindedList
    };

    webserverInstance.io.sockets.emit('webserver:bindingDevList', listData);   
  })
  // ############## Zigbee Network - Binding Event Failed #########################################
  .on('zb-gateway:binding:failed', function () {
    console.log("Webserver: Received zb-gateway:binding:failed");
    webserverInstance.io.sockets.emit('webserver:binding:failed');  
  })
  // ############## Zigbee Network - Latest Network Info ##########################################
  .on('zb-gateway:unbinding:failed', function () {
    console.log("Webserver: Received zb-gateway:unbinding:failed");
    webserverInstance.io.sockets.emit('webserver:unbinding:failed');  
  })
  // ############## Light Device - Got State Response #############################################
  .on('zb-gateway:light_device:state', function(data) {
    console.log("Webserver: Got light device state update: ", data);
    webserverInstance.io.sockets.emit('webserver:light_device:state', data);  
  })
  // ############## Light Device - Got Level Response #############################################
  .on('zb-gateway:light_device:level', function(data) {
    console.log("Webserver: Got light device state update: ", data);
    webserverInstance.io.sockets.emit('webserver:light_device:level', data);  
  })
  // ############## Color Light Device - Got Color Response #######################################
  .on('zb-gateway:light_device:color', function(data) {
    console.log("Webserver: Got light device color update: ", data);
    webserverInstance.io.sockets.emit('webserver:light_device:color', data);  
  })
  // ############## Temperature Device - Got Temperature Response #################################
  .on('zb-gateway:temp_device:temp', function(data) {
    console.log("Webserver: Got temp device state update: ", data);
    webserverInstance.io.sockets.emit('webserver:temp_device:temp', data);  
  })
  // ############## Doorlock Device - Got State Response ##########################################
  .on('zb-gateway:doorlock_device:state', function(data) {
    console.log("Webserver: Got doorlock device state update: ", data);
    webserverInstance.io.sockets.emit('webserver:doorlock_device:state', data);  
  })
  // ############## Doorlock Device - Got Set Lock Response #######################################
  .on('zb-gateway:doorlock_device:set_rsp', function(data) {
    console.log("Webserver: Got doorlock device set response: ", data);
    webserverInstance.io.sockets.emit('webserver:doorlock_device:set_rsp', data);  
  })
  // ############## Thermostat Device - Got Attribute Response ####################################
  .on('zb-gateway:thermostat:attribute', function(data) {
    console.log("Webserver: Got thermostat device attribute response: ", data);
    webserverInstance.io.sockets.emit('webserver:thermostat:attribute', data);  
  })
  // ############## HaGateway - Got Light Level Control, Unsupported Attribute ####################
  .on(EVT('zb-gateway', ZBL.HA_CLUSTER_ID.LEVEL_CONTROL, ZBL.STATUS.UNSUPPORTED_ATTRIBUTE), function(data) {
    console.log("Webserver: Got level control, on/off transition time, unsupported device attribute response: ", data);
    webserverInstance.io.sockets.emit(EVT('zb-gateway', ZBL.HA_CLUSTER_ID.LEVEL_CONTROL, ZBL.STATUS.UNSUPPORTED_ATTRIBUTE), data.guid);  
  });

  
  //-----------------------------------------------------------------------------------------------
  // Handle Events from the hosted WebApp
  //-----------------------------------------------------------------------------------------------
  webserverInstance.io.sockets.on('connection', function (socket) {
    socket
    // ############## Zigbee Network Request - Get Network Info ###################################
    .on('network:getinfo', function (data) {
      console.log("webserver: get network info");
      zigbeeGateway.getNetworkInfo();
    })
    // ############## Zigbee Network Request - Open the Network ###################################
    .on('network:open', function (data) {
      console.log("webserver: open network");
      zigbeeGateway.openNetwork();
    })
    // ############## Zigbee Network Request - Close the Network ##################################
    .on('network:close', function (data) {
      console.log("webserver: close network");
      zigbeeGateway.closeNetwork();
    })
    // ############## Zigbee Network Request - Emit Hard Network Reset ############################
    .on('network:hard:reset', function (data) {
      console.log("webserver: hard system reset");
      zigbeeGateway.hardSystemReset();
    })
    // ############## Zigbee Network Request - Emit Soft Network Reset ############################
    .on('network:soft:reset', function (data) {
      console.log("webserver: soft system reset");
      zigbeeGateway.softSystemReset();
    })
    // ############## Zigbee Network Request - Get Network Device List ############################
    .on('getDeviceList', function (data) {
      console.log("webserver: get device lists");
      zigbeeGateway.getDeviceList();           
    })
    // ############## Zigbee Network Request - Get Latest Bind Lists ##############################
    .on('getBindList', function (data) {
      console.log("webserver: get bind list");
      zigbeeGateway.getBindList();
    })
    // ############## Light Device Request - Get Network Info #####################################
    .on('light_device:set:ONOFF', function(data){
      console.log("Webserver: set light");
      zigbeeGateway.DimmableLight.set_ONOFF_state(data);      
    })
      // ############## Light Device Request - Get On/Off State ###################################
    .on('light_device:get:ONOFF', function(data){
      console.log("Webserver: get light");
      zigbeeGateway.DimmableLight.get_ONOFF_state(data);      
    })
      // ############## Light Device Request - Get Level ##########################################
    .on('light_device:get:level', function(data){
      console.log("Webserver: get level");
      zigbeeGateway.DimmableLight.get_level(data);      
    })
    // ############## Light Device Request - Get Color Info #######################################
    .on('light_device:get:color', function(data){
      console.log("Webserver: get color");
      zigbeeGateway.ColorLight.get_color(data);      
    })
    // ############## Light Device Request - Set Level Transition #################################
    .on('light_device:set:level_transition', function(data){
      console.log("Webserver: set light level and transition");
      zigbeeGateway.DimmableLight.set_level_transition(data);      
    })
    // ############## Light Device Request - Set On/Off Transition Time ###########################
    .on('light_device:set:onoff_transition', function(data){
      console.log("Webserver: set light on/off transition");
      zigbeeGateway.ColorLight.set_onoff_transition(data);      
    })
    // ############## Light Device Request - Set Color ############################################
    .on('light_device:set:color', function(data){
      console.log("Webserver: set light color");
      zigbeeGateway.ColorLight.set_color(data);      
    })
    // ############## Temperature Device Request - Get Temperature ################################
    .on('temp_device:get:temp', function(data){
      console.log("Webserver: get temp");
      zigbeeGateway.TempSensor.get_temp(data);      
    })
    // ############## Temp Device Request - Set Reporting Interval ################################
    .on('temp_device:set:interval', function(data){
      console.log("Webserver: set temp interval");
      zigbeeGateway.TempSensor.set_interval(data);      
    })
    // ############## Doorlock Device Request - Get Lock State ####################################
    .on('doorlock_device:get:state', function(data){
      console.log("Webserver: get door lock state");
      zigbeeGateway.DoorLockDevice.get_state(data);      
    })
    // ############## Doorlock Device Request - Set Lock State ####################################
    .on('doorlock_device:set:lock', function(data){
      console.log("Webserver: set door lock to locked");
      zigbeeGateway.DoorLockDevice.lock(data);      
    })
    // ############## Doorlock Device Request - Set Unlock State ##################################
    .on('doorlock_device:set:unlock', function(data){
      console.log("Webserver: set door lock to unlocked");
      zigbeeGateway.DoorLockDevice.unlock(data);      
    })
    // ############## Thermostat Device Request - Change Setpoint Value ###########################
    .on('thermostat_device:change:setpoint', function(data){
      console.log("Webserver: set thermostat values");
      zigbeeGateway.ThermostatDevice.setpoint_change(data);      
    })
    // ############## Thermostat Device Request - Get Minimum Heat Setpoint Value #################
    .on('thermostat_device:get:minheat:setpoint', function(data){
      console.log("Webserver: get thermostat min heat setpoint ");
      zigbeeGateway.ThermostatDevice.get_minheat_setpoint(data);      
    })
    // ############## Thermostat Device Request - Get All Attributes ##############################
    .on('thermostat_device:get:all:attributes', function(data){
      console.log("Webserver: get thermostat all attributes ");
      zigbeeGateway.ThermostatDevice.get_all_attributes(data);      
    })
    // ############## Binding Request - Set Bind between Two Devices ##############################
    .on('binding:set:bind', function(data){
      console.log("Webserver: set binding");
      zigbeeGateway.DeviceBinding.set_binding(data);      
    })
    // ############## Binding Request - Set Unbinding between Two Devices #########################
    .on('binding:set:unbind', function(data){
      console.log("Webserver: set un-binding");
      zigbeeGateway.DeviceBinding.set_unbinding(data);      
    })
    // ############## Zigbee Network Request - Remove the Device from Network #####################
    .on('removeDevice', function (data) {
      console.log("webserver: removeDevice: ", data.ieee, " guid: ", data.guid);
      zigbeeGateway.removeDevice(data);
    })  
    // ############## General - Webapp Had Error ##################################################
    .on('error', function (err) {
      console.log("webserver: socket error: ", err);
    });
  });


  //----------------------------------------------------------------------------------------------------------
  // getIPAddress()
  //    -Input: NULL
  //    -Return: IP Address (string)
  // 
  // Get server IP address on LAN
  //----------------------------------------------------------------------------------------------------------
  function getIPAddress() {
    var interfaces = require('os').networkInterfaces();
    for (var devName in interfaces) {
      var iface = interfaces[devName];
      for (var i = 0; i < iface.length; i++) {
        var alias = iface[i];
        if (alias.family === 'IPv4' && alias.address !== '127.0.0.1' && !alias.internal)
          return alias.address;
      }
    }
    return '0.0.0.0';
  }


  //----------------------------------------------------------------------------------------------------------
  // EVT()
  //    -Input: multiple event codes (string, string, string, ...)
  //    -Return: Event Command (string)
  // 
  // create an event command string from all input arguments
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
}

Webserver.prototype.__proto__ = events.EventEmitter.prototype;

// export the class
module.exports = Webserver;
