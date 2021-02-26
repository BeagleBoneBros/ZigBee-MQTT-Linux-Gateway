/******************************************************************************

 @file tempsensor-device.js

 @brief Creates a temperature sensor device type

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

var ByteBuffer = require("bytebuffer"); 
var Hagateway = require('../linux_gw_adapters/hagateway.js');
var storage = require('node-persist');
var ZBL = require('../zigbee_library/zb_lib.js');

// Constructor
function TempSensorDevice(_nwkmgr, ieee, parentIeee, ep, type) {

  var td = this;
  
  var nwkmgr = _nwkmgr;
    
  var ieeeBb = new ByteBuffer(8, ByteBuffer.LITTLE_ENDIAN).append(ieee, "hex", 0).reverse();
  
  //id is first 3 byes of ieee, last 2 bytes of ieee and 
  //ep with 2 digits i.e. ep of 8 = 08
  td.info = {
  	id: ieeeBb.toString('hex').substring(0,4) + ieeeBb.toString('hex').substring(10,16) + (("0" + ep).slice(-2)),
  	ep: ep,
  	ieee: ieee
  };
      
    
  // initialize data properties
  td.data = { 
  	guid: ieeeBb.toString('hex') + ":" + td.info.ep,
  	name: "Zigbee Temperature Sensor", 
  	type: type,
  	temp: 0 	
  };

  //start the report timer
  // td.data.reportInterval = 5000;      
  
  //create the connection to the HA Gateway  
  // var hagateway = new Hagateway();

  // //Afer the bind response send the configure reportInterval    
  // nwkmgr.on('bind-ind', function (data) {
  	
  // 	//check the bind response was for this device to the GW
  // 	if( (data.srcIeee.toString('hex') === td.info.ieee.toString('hex')) && (data.srcEp === td.info.ep) &&
  // 	    (data.dstIeee.toString('hex') === nwkmgr.getGwInfo().ieeeAddress.toString('hex')) && 
  // 	    (data.dstEp === nwkmgr.getGwInfo().simpleDescList[0].endpointId)) { 

	//     //setup a report so the tempsensor sends temp reports
	//     var clusterId = 0x0402; // Temperature Measurement Cluster
	//     var attrList = [];
	        
	//     var attrRec= {
	//   	  attributeId: 0, //MeasuredValue
	//   	  attributeType: 41, //ZCL_DATATYPE_INT16
	//   	  minReportInterval: (td.data.reportInterval / 1000),
	//   	  maxReportInterval: 60, //defined by HA spec
	//   	  reportableChange: 0 //force report to be sent on min interval
	//   	  //alternatively you can set reportableChange to report an temp delta 
	//   	  //or maxReportInterval if the delta was not reached
	//     };
	  
	//     hagateway.setAttrReport(td.info.ieee, td.info.ep, clusterId, [attrRec]);
  //   }
  // });
    
  //now send the bind request to bind the Temp cluster of the 
  //temp sensor to the GW so  
  var srcIeee = td.info.ieee;
  var srcEp = td.info.ep;
  var dstEp = nwkmgr.getGwInfo().simpleDescList[0].endpointId;
  
  // setup binding between the temperature sensor and the coordinator for periodic temp reporting
  nwkmgr.setDevBinding(0, srcIeee, srcEp, parentIeee, dstEp, ZBL.HA_CLUSTER_ID.TEMPERATURE_MEASUREMENT, 
    function(status, seq){  
      if(status !== 0) {
        console.log("setDevBinding request confirmation failed: ", status);
      }
      else {
        console.log("setDevBinding tempsensor success");
      }
  });
      
  
}

// export the class
module.exports = TempSensorDevice;