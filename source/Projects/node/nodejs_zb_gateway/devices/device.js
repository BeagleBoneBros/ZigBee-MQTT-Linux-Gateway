/******************************************************************************

 @file device.js

 @brief Responsible for creating device types 

 Group: WCS LPC
 $Target Devices: Linux: AM335x $

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

var LightDevice = require('./light-device.js');
var SwitchDevice = require('./switch-device.js');
var TempSensorDevice = require('./tempsensor-device.js');
var DoorLockDevice = require('./doorlock-device.js');
var DoorLockControllerDevice = require('./doorlockcontroller-device.js');
var ThermostatDevice = require('./thermostat-device.js');

var ZBL = require('../zigbee_library/zb_lib.js');


function Device(nwkmgr, ieee, parentIeee, simpleDesc) {
	//console.log("Device: creating new device");
  return createDevType(nwkmgr, ieee, simpleDesc);

  //private methods with private member access
  function createDevType(nwkmgr, ieee, simpleDesc)
  {
    if(//filter out HA Ep's with no clusters
      ((simpleDesc.profileId === ZBL.ZB_PROFILE_ID.HOME_AUTOMATION) &&
      ((simpleDesc.inputClusters.length > 0) ||
      (simpleDesc.outputClusters.length > 0))) ||				 				      							 
      //filter out ZLL profile with num clusters = 1 (tl ep)
      ((simpleDesc.profileId === ZBL.ZB_PROFILE_ID.LIGHT_LINK) &&
      ((simpleDesc.inputClusters.length > 1) ||
      (simpleDesc.outputClusters.length > 1))))
    {	
      //HA profile 
      if(simpleDesc.profileId  === ZBL.ZB_PROFILE_ID.HOME_AUTOMATION)
      { 
        //HA Color Dimmable Light
        if(simpleDesc.deviceId === ZBL.HA_DEVICE_ID.COLOR_DIMMABLE_LIGHT)
        {
          return new LightDevice(nwkmgr, ieee, simpleDesc.endpointId, "ColorLight");
        }
        //HA Dimmable Light
        else if(simpleDesc.deviceId === ZBL.HA_DEVICE_ID.DIMMABLE_LIGHT)
        {
          return new LightDevice(nwkmgr, ieee, simpleDesc.endpointId, "DimmableLight");
        }
        //HA OnOff Light
        else if(simpleDesc.deviceId === ZBL.HA_DEVICE_ID.ON_OFF_LIGHT)
        {
          return new LightDevice(nwkmgr, ieee, simpleDesc.endpointId, "OnOffLight");
        }
        //HA Switch
        else if(simpleDesc.deviceId === ZBL.HA_DEVICE_ID.ON_OFF_LIGHT_SWITCH)
        {
          return new SwitchDevice(nwkmgr, ieee, simpleDesc.endpointId, "OnOffSwitch");
        }
        //HA Door Lock
        else if(simpleDesc.deviceId === ZBL.HA_DEVICE_ID.DOOR_LOCK)
        {
          return new DoorLockDevice(nwkmgr, ieee, simpleDesc.endpointId, "DoorLock");
        }
        //HA Door Lock Controller
        else if(simpleDesc.deviceId === ZBL.HA_DEVICE_ID.DOOR_LOCK_CONTROLLER)
        {
          return new DoorLockControllerDevice(nwkmgr, ieee, simpleDesc.endpointId, "DoorLockController");
        }
        //HA Thermostat
        else if(simpleDesc.deviceId === ZBL.HA_DEVICE_ID.THERMOSTAT)
        {
          return new ThermostatDevice(nwkmgr, ieee, simpleDesc.endpointId, "Thermostat");
        }
        //HA Temp Sensor
        else if(simpleDesc.deviceId === ZBL.HA_DEVICE_ID.TEMPERATURE_SENSOR)
        {
          return new TempSensorDevice(nwkmgr, ieee, parentIeee, simpleDesc.endpointId, "TempSensor");
        }                    
      }
      //ZLL profile
      else if(simpleDesc.profileId  === ZBL.ZB_PROFILE_ID.LIGHT_LINK)
      {
        //ZLL Dimmalbe Light
        if(simpleDesc.deviceId === ZBL.ZLL_DEVICE_ID.DIMMABLE_LIGHT)
        {
          return new LightDevice(nwkmgr, ieee, simpleDesc.endpointId, "DimmableLight");
        }
        //ZLL Color Light
        if(simpleDesc.deviceId === ZBL.ZLL_DEVICE_ID.COLOR_LIGHT)
        {
          return new LightDevice(nwkmgr, ieee, simpleDesc.endpointId, "ColorLight");
        }
        //ZLL Extended Color Light
        if(simpleDesc.deviceId === ZBL.ZLL_DEVICE_ID.EXTENDED_COLOR_LIGHT)
        {
          return new LightDevice(nwkmgr, ieee, simpleDesc.endpointId, "ColorLight");
        }
      }
    }

    console.log("Device Type Not Recognized: ", simpleDesc.profileId, ":", simpleDesc.deviceId);
  }
}


// export the class
module.exports = Device;