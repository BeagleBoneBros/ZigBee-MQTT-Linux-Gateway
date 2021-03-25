/******************************************************************************

 @file main.js

 @brief Handles creating a local or Cloud webserver

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

var Webserver = require('./webserver/webserver.js');
var MQTTAdapter = require("./cloud_adapters/MQTTCloudAdapterSimple.js");


// top level function starting appropriate webserver
function Main() {

  // update the IP address of the Linux Gateway 
  ip_address = '127.0.0.1';

  // check to see if an argument has been entered
  if(process.argv.length > 2){
    webserver_mode = process.argv[2];

    if (webserver_mode == 'localhost'){
      console.log("Started Local Hosted Webserver");
      this.webserver = new Webserver(ip_address); 
    }
    else if (webserver_mode == 'MQTT'){
      console.log("Started MQTT Adapter");
      this.cloudadapter = new MQTTAdapter(ip_address);
    }
    else if (webserver_mode == 'MQTTandLocal'){
      console.log("Started MQTT Adapter");
      this.cloudadapter = new MQTTAdapter(ip_address);
      this.webserver = new Webserver(ip_address);
  
    }

    else{
      console.log("ERROR: Incorrect mode set, please enter either `localhost`, `ibm`, 'MQTT', or 'MQTTandLocal'");
    }
  }

  // default to local hosted webserever
  else{
    this.webserver = new Webserver(ip_address); 
  }
}

// start the main 
var Main = new Main();
