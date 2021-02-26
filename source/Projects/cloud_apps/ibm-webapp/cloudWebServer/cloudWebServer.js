/******************************************************************************

 @file cloudWebServer.js

 @brief webserver hosted in cloud environment

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
var events = require("events");
var socket = require("socket.io");
var http = require("http");
 
/* Webserver Instance */
var webserverInstance;
 
function Webserver() {
 
	/* There should be only one webserver client */
	if (typeof webserverInstance !== "undefined") {
		return webserverInstance;
	}

	/* Set up to emit events */
	events.EventEmitter.call(this);
	webserverInstance = this;

	// cfenv provides access to your Cloud Foundry environment
	// for more info, see: https://www.npmjs.com/package/cfenv
	var cfenv = require('cfenv');

	// get the app environment from Cloud Foundry
	var appEnv = cfenv.getAppEnv();

	/* Set up webserver */
	var app = express();
	var server = http.createServer(app);

	// start server on the specified port and binding host
	app.listen(1310, '0.0.0.0', function() {
		// print a message when the server starts listening
		console.log("server starting on " + appEnv.url);
	});

	// setup webserver to listen to IO port
	webserverInstance.io = socket.listen(server);
	server.listen(appEnv.port, '0.0.0.0');

	// require path library
	var path = require('path');

	// point webserver to look at /public folder
	app.use(express.static(path.join(__dirname, '/public')));

	// have the webserver host the webapp page
	app.get('/', function(req, res){
		res.sendFile(path.join(__dirname, '/public', 'webapp.html'));
	});

	/* Handle socket events */
	webserverInstance.io.sockets.on('connection', function (socket) {
		socket
		.on('getDeviceList', function (data) {
			webserverInstance.emit('cloudwebserver:getDeviceList', data);
		})
		.on('getBindList', function (data) {
			webserverInstance.emit('cloudwebserver:getBindList', data);
		})
		.on('removeDevice', function (data) {
			webserverInstance.emit('cloudwebserver:removeDevice', data);
		})
		.on('network:open', function (data) {
			webserverInstance.emit('cloudwebserver:network:open', data);
		})
		.on('network:close', function (data) {
			webserverInstance.emit('cloudwebserver:network:close', data);
		})
		.on('network:getinfo', function (data) {
			webserverInstance.emit('cloudwebserver:network:getinfo', data);
		})
		.on('network:hard:reset', function (data) {
			webserverInstance.emit('cloudwebserver:network:hard:reset', data);
		})
		.on('network:soft:reset', function (data) {
			webserverInstance.emit('cloudwebserver:network:soft:reset', data);
		})
		.on('light_device:set:ONOFF', function (data) {
			webserverInstance.emit('cloudwebserver:light_device:set:ONOFF', data);
		})
		.on('light_device:get:ONOFF', function (data) {
			webserverInstance.emit('cloudwebserver:light_device:get:ONOFF', data);
		})
		.on('light_device:set:level_transition', function (data) {
			webserverInstance.emit('cloudwebserver:light_device:set:level_transition', data);
		})
		.on('light_device:set:onoff_transition', function (data) {
			webserverInstance.emit('cloudwebserver:light_device:set:onoff_transition', data);
		})
		.on('light_device:get:level', function (data) {
			webserverInstance.emit('cloudwebserver:light_device:get:level', data);
		})
		.on('light_device:set:color', function (data) {
			webserverInstance.emit('cloudwebserver:light_device:set:color', data);
		})
		.on('light_device:get:color', function (data) {
			webserverInstance.emit('cloudwebserver:light_device:get:color', data);
		})
		.on('temp_device:get:temp', function (data) {
			webserverInstance.emit('cloudwebserver:temp_device:get:temp', data);
		})
		.on('temp_device:set:interval', function (data) {
			webserverInstance.emit('cloudwebserver:temp_device:set:interval', data);
		})
		.on('doorlock_device:get:state', function (data) {
			webserverInstance.emit('cloudwebserver:doorlock_device:get:state', data);
		})
		.on('doorlock_device:set:unlock', function (data) {
			webserverInstance.emit('cloudwebserver:doorlock_device:set:unlock', data);
		})
		.on('doorlock_device:set:lock', function (data) {
			webserverInstance.emit('cloudwebserver:doorlock_device:set:lock', data);
		})
		.on('thermostat_device:change:setpoint', function (data) {
			webserverInstance.emit('cloudwebserver:thermostat_device:change:setpoint', data);
		})
		.on('thermostat_device:get:minheat:setpoint', function (data) {
			webserverInstance.emit('cloudwebserver:thermostat_device:get:minheat:setpoint', data);
		})
		.on('thermostat_device:get:all:attributes', function (data) {
			webserverInstance.emit('cloudwebserver:thermostat_device:get:all:attributes', data);
		})
		.on('binding:set:bind', function (data) {
			webserverInstance.emit('cloudwebserver:binding:set:bind', data);
		})
		.on('binding:set:unbind', function (data) {
			webserverInstance.emit('cloudwebserver:binding:set:unbind', data);
		})
		.on('setIBMCredentials', function (data) {
			webserverInstance.emit('setIBMCredentials', data);
		});
	});

	/**********************************************************************
	Public method to send Update Messages to the client
	***********************************************************************/
	webserverInstance.webserverSendToClient = function(msgType, data){
		console.log("CloudWebServer: Got msgType: ", msgType, " msgData: ", data);
		webserverInstance.io.sockets.emit(msgType, data);
	};
}
 
Webserver.prototype.__proto__ = events.EventEmitter.prototype;
 
module.exports = Webserver;