/******************************************************************************

 @file app.js

 @brief Main app hosted in cloud environment

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

//------------------------------------------------------------------------------
// node.js starter application for IBM Cloud
//------------------------------------------------------------------------------

var Webserver = require("./cloudWebServer/cloudWebServer.js");

function Gateway() {
	var webserver = new Webserver();

	// cfenv provides access to your Cloud Foundry environment
    // for more info, see: https://www.npmjs.com/package/cfenv
	var cfenv = require('cfenv');
	
    // get the app environment from Cloud Foundry
	var appEnv = cfenv.getAppEnv();
	
	// IBM IoT service
	var Client = require('ibmiotf').IotfApplication;
	var connected = false;
	var type;
	var typeId;
	var appClient;

	// import ibm config setting, Automatically sign into the IOT service
	var mycredentials = require('./ibmConfig.json');
	  
	// login to IBM IoT Service
	setIBMCredentials(mycredentials);
	
	// ############################################################################################
	// Handle Events from Webserver	
	// ############################################################################################
	webserver.on('cloudwebserver:getDeviceList', function (data) {
		appClient.publishDeviceCommand(type, typeId, "cloud:getDeviceList", "json");
	})
	.on('cloudwebserver:getBindList', function (data) {
		appClient.publishDeviceCommand(type, typeId, "cloud:getBindList", "json");
	})
	.on('cloudwebserver:removeDevice', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:removeDevice", "json", mydata);
	})
	.on('cloudwebserver:network:open', function (data) {
		appClient.publishDeviceCommand(type, typeId, "cloud:network:open", "json");
	})
	.on('cloudwebserver:network:close', function (data) {
		appClient.publishDeviceCommand(type, typeId, "cloud:network:close", "json");
	})
	.on('cloudwebserver:network:getinfo', function (data) {
		appClient.publishDeviceCommand(type, typeId, "cloud:network:getinfo", "json");
	})
	.on('cloudwebserver:network:hard:reset', function (data) {
		appClient.publishDeviceCommand(type, typeId, "cloud:network:hard:reset", "json");
	})
	.on('cloudwebserver:network:soft:reset', function (data) {
		appClient.publishDeviceCommand(type, typeId, "cloud:network:soft:reset", "json");
	})
	.on('cloudwebserver:light_device:set:ONOFF', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:light_device:set:ONOFF", "json", mydata);
	})
	.on('cloudwebserver:light_device:get:ONOFF', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:light_device:get:ONOFF", "json", mydata);
	})
	.on('cloudwebserver:light_device:set:level_transition', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:light_device:set:level_transition", "json", mydata);
	})
	.on('cloudwebserver:light_device:set:onoff_transition', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:light_device:set:onoff_transition", "json", mydata);
	})
	.on('cloudwebserver:light_device:get:level', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:light_device:get:level", "json", mydata);
	})
	.on('cloudwebserver:light_device:set:color', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:light_device:set:color", "json", mydata);
	})
	.on('cloudwebserver:light_device:get:color', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:light_device:get:color", "json", mydata);
	})
	.on('cloudwebserver:temp_device:get:temp', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:temp_device:get:temp", "json", mydata);
	})
	.on('cloudwebserver:temp_device:set:interval', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:temp_device:set:interval", "json", mydata);
	})
	.on('cloudwebserver:doorlock_device:get:state', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:doorlock_device:get:state", "json", mydata);
	})
	.on('cloudwebserver:doorlock_device:set:unlock', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:doorlock_device:set:unlock", "json", mydata);
	})
	.on('cloudwebserver:doorlock_device:set:lock', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:doorlock_device:set:lock", "json", mydata);
	})
	.on('cloudwebserver:thermostat_device:change:setpoint', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:thermostat_device:change:setpoint", "json", mydata);
	})
	.on('cloudwebserver:thermostat_device:get:minheat:setpoint', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:thermostat_device:get:minheat:setpoint", "json", mydata);
	})
	.on('cloudwebserver:thermostat_device:get:all:attributes', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:thermostat_device:get:all:attributes", "json", mydata);
	})
	.on('cloudwebserver:binding:set:bind', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:binding:set:bind", "json", mydata);
	})
	.on('cloudwebserver:binding:set:unbind', function (data) {
		mydata = JSON.stringify(data);
		appClient.publishDeviceCommand(type, typeId, "cloud:binding:set:unbind", "json", mydata);
	});



    //----------------------------------------------------------------------------------------------------------
    // setIBMCredentials()
    //    -Inputs: Data Object with following properties:
    //                                                  - ServiceName
	//                                                  - Org
	//													- type
	//													- typeId
	//                                                  - Id
	//													- ApiKey
	//													- ApiToken
	//
    //  - Output: NULL
    //
    // connect to the IBM IoT web client. Handle various incomming events from the cloud client
    //----------------------------------------------------------------------------------------------------------
	function setIBMCredentials(data) {

		if(connected){
			appClient.disconnect();
			connected = false;
		}

		// get the IoT platform service name
		iotPlatformServiceName = data.ServiceName;

		// Loop through configuration internally defined in Bluemix and retrieve the credential from the IoT service
		baseConfig = appEnv.getServices(iotPlatformServiceName);
		iotCredentials = baseConfig[iotPlatformServiceName];

		// setup IBM config credentials
		iotAppConfig = {
			"org" : data.Org,
			"id" : data.Id,
			"auth-method" : "apikey",
			"auth-key" : data.ApiKey,
			"auth-token" : data.ApiToken,
			"type" : "shared"
		};

		appClient = new Client(iotAppConfig);

		// connect to the cloud client
		appClient.connect();

		type = data.type;
		typeId = data.typeId;

		// subscribe to input events
        appClient.on("connect", function () {
            console.log("Connected");
            appClient.subscribeToDeviceEvents(type, typeId, "+");
            connected = true;
        });

		// handle error events from the Cloud client
        appClient.on("error", function (err) {
            connected = false;
            console.log("Error : " + err);
        });

		// handle reconnecting to the CLoud client
        appClient.on("reconnect", function () {
            console.log("Re-connected");
            appClient.subscribeToDeviceEvents(data.type, data.typeId, "+");
            connected = true;
        });

		// handle disconnecting from the cloud client
        appClient.on("disconnect", function () {
            connected = false;
            console.log("Disconnected");
        });

		// send all incoming deviceEvents from the remote Gateway to the webserver
		appClient.on("deviceEvent", function(deviceType, deviceId, eventType, format, payload){
			var msg = JSON.parse(payload);
			console.log("Message received: " + JSON.stringify(msg));

			// send the incomming commands to the webapp client
			webserver.webserverSendToClient(eventType, msg);
        });
	}

}

/* create gateway */
var gateway = new Gateway();