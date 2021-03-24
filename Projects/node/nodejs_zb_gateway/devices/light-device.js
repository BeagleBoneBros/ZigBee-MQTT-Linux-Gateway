/******************************************************************************

 @file light-device.js

 @brief Creates a light device type

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

// Constructor
function LightDevice(_nwkmgr, ieee, ep, type) {
  
  var ld = this;
  
  var nwkmgr = _nwkmgr;  
  
  var ieeeBb = new ByteBuffer(8, ByteBuffer.LITTLE_ENDIAN).append(ieee, "hex", 0).reverse();
  
  //id is first 3 byes of ieee, last 2 bytes of ieee and 
  //ep with 2 digits i.e. ep of 8 = 08
  ld.info = {
  	id: ieeeBb.toString('hex').substring(0,4) + ieeeBb.toString('hex').substring(10,16) + (("0" + ep).slice(-2)),
  	ieee: ieee,
  	ep: ep,
  };
    
  // initialize data properties
  ld.data = { 
  	guid: ieeeBb.toString('hex') + ":" + ld.info.ep,
  	name: "Zigbee Light",
  	type: type 	
  };
  
  switch (type) {
    case "ColorLight":
      ld.data.hue = 0;
      ld.data.saturation = 0;
      break;

    case "DimmableLight":
      ld.data.level = 0;
      ld.data.transitionTime = 10;
      break;
      
    default:
      //default attributes for all light devices
      ld.data.on = 0;
      break;
  }
    
  return ld;
}

// export the class
module.exports = LightDevice;