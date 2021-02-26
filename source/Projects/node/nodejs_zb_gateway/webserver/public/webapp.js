/******************************************************************************

 @file webapp.js

 @brief webapp implementation for frontend

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

function WebAppClient() {
   
  // define self as WebAppClient object
  self = this;

  // connect to webserver socket
  self.socket = io.connect();

  // define a device list variable
  self.deviceList;

  // binding variables
  self.unboundList;
  self.bindedList;
  self.BindFlag = 'none';

  // define variable for temperature chart plotting
  var tempChart;

  //-------------------------------------------------------------------------------
  // Handle Incomming Events from Webserver
  //-------------------------------------------------------------------------------
  // ############################## Acquired New Device Lists ###############################################
  self.socket.on('devList', function (devlist_data) {
    console.log("WebApp: Got DevList: ", devlist_data.dL, " Got UnboundList: ", devlist_data.uL, " Got BindedList: ", devlist_data.bL);

    self.deviceList = devlist_data.dL;
    self.unboundList = devlist_data.uL;
    self.bindedList = devlist_data.bL;
    self.drawList();
    self.drawUnboundList_SwitchLight();
    self.drawBindedList_SwitchLight();
    self.drawUnboundList_DoorLockController();
    self.drawBindedList_DoorLockController();
  })
  // ############################## Remove Device Card ######################################################
  .on('webserver:removeDeviceCard', function (data) {
    console.log("WebApp: Got removeDeviceCard: ", data);

    self.removeDeviceCard(data);
  })

  // ############################## Network is Ready ########################################################
  .on('webserver:network:ready', function (devlist_data) {
    // get the modal element based on the guid for the door lock
    var network_reset_modal_id = document.getElementById('networkResetModal');

    // hide the network reset modal if it is visible
    $(document).ready(function() {
      $(network_reset_modal_id).modal("hide");
    });

    // get the modal element based on the guid for the door lock
    var network_ready_modal_id = document.getElementById('networkReadyModal');

    // show the network ready popup modal
    $(document).ready(function() {
      $(network_ready_modal_id).modal("show");
    });

    // get the latest network info
    self.getNetworkInfo();

    // get the current list of devices and bindable devices
    self.getDeviceList();

    // get the latest binding list
    self.getBindList();
  })

  // ############################## Network Info ############################################################
  .on('webserver:network:info', function (data) {
    console.log("Got Network Info: ", data);
    document.getElementById('networkChannelNumber').innerHTML = 'Channel: ' + data.nwkChannel;
    document.getElementById('networkPanID').innerHTML = 'PanID: 0x' + parseInt(data.panId).toString(16).toUpperCase();
  })

  // ############################## Acquired New Binding Lists ##############################################
  .on('webserver:bindingDevList', function (devlist_data) {
    console.log("WebApp: Got DevList: ", devlist_data.dL, " Got UnboundList: ", devlist_data.uL, " Got BindedList: ", devlist_data.bL);

    self.unboundList = devlist_data.uL;
    self.bindedList = devlist_data.bL;

    // remove the bindFLag if it is set
    if (self.BindFlag != 'none'){
      document.getElementById(self.BindFlag).remove();
      self.BindFlag = 'none';
    }

    self.drawBindedList_SwitchLight();
    self.drawBindedList_DoorLockController();
  })

  // ############################## Binding Failed ##########################################################
  .on('webserver:binding:failed', function () {
    console.log('Got Bind Response Failed');
    document.getElementById(self.BindFlag).remove(); 
    self.drawUnboundList_SwitchLight();
    self.drawBindedList_SwitchLight();
    self.drawUnboundList_DoorLockController();
    self.drawBindedList_DoorLockController();
  })

  // ############################## Un-Binding Failed #######################################################
  .on('webserver:unbinding:failed', function () {
    console.log('Got UnBind Response Failed');
    document.getElementById(self.BindFlag).remove(); 
    self.drawUnboundList_SwitchLight();
    self.drawBindedList_SwitchLight();
    self.drawUnboundList_DoorLockController();
    self.drawBindedList_DoorLockController();
  })

  // ############################## Update on Light Device State ############################################
  .on('webserver:light_device:state', function(data){
    console.log("Light Device State: ", data);
    var element_id = data.guid + '_state';

    // update the light device state image
    if (data.on == ON_OFF_CLUSTER_OnOff.ON){
      img = 'dist/img/light_on.png';
    }
    else {
      img = 'dist/img/light_off.png';
    }

    // update light images and state in webapp elements
    document.getElementById(element_id).src = img;
    document.getElementById(element_id).setAttribute('state', data.on);
  })

  // ############################## Update on Light Device Level ############################################
  .on('webserver:light_device:level', function(data){
    // update slider value
    var element_id = data.guid + '_level_slider';
    document.getElementById(element_id).value = data.level;

    // update text value
    element_id = data.guid + '_level_value';
    document.getElementById(element_id).innerHTML = data.level;
  })

  // ############################## Update on Light Device Color ############################################
  .on('webserver:light_device:color', function(data){
    // update slider value
    var element_id = data.guid + '_hue_slider';
    document.getElementById(element_id).value = data.hue;

    // update text value
    element_id = data.guid + '_hue_value';
    document.getElementById(element_id).innerHTML = data.hue;

    // update slider value
    element_id = data.guid + '_saturation_slider';
    document.getElementById(element_id).value = data.saturation;

    // update text value
    element_id = data.guid + '_saturation_value';
    document.getElementById(element_id).innerHTML = data.saturation;
  })

  // ############################## Update on Temp Device Temp ##############################################
  .on('webserver:temp_device:temp', function(data){
    var element_id = data.guid + '_temp';

    // update the temperature progress bar width
    document.getElementById(element_id).innerHTML = data.temp + ' C';
    document.getElementById(element_id).setAttribute('style', 'width:' + data.temp + '%');

    // ------------------------------------------------------------------------------------------------------
    // Create a Plot
    // ------------------------------------------------------------------------------------------------------
    // get the "chart_status". This is used to determine if a Chart Object has been created yet for the temp device
    var chart_canvas_status = document.getElementById(data.guid + '_tempChart').getAttribute('chart_status');

    var device_element = document.getElementById(data.guid);

    if (chart_canvas_status == 'false'){
      // get the canvas element where the chart will be drawn
      var ctx = document.getElementById(data.guid + '_tempChart').getContext('2d');

      tempChartConfig = {
          // The type of chart we want to create
          type: 'line',
      
          // The data that we start our chart with
          data: {
            labels: [''],
            datasets: [{
                backgroundColor: 'rgb(0, 123, 255)',
                borderColor: 'rgb(0, 123, 255)',
                fill: false,
                data: [data.temp],
            }]
          },
      
          // Configuration options for chart go here
          options: {
            scales: {
              yAxes: [{
                  ticks: {
                      stepSize: 1
                  },
                  scaleLabel: {
                    display: true,
                    labelString: 'Temperature (C)'
                  }
              }],
              xAxes: [{
                ticks: {
                    stepSize: 1
                },
                gridLines: {
                  color: "rgba(0, 0, 0, 0)",
                }
              }]
            },
            legend: {
              display: false
            }
          }
      };

      // create the Chart object
      tempChart = new Chart(ctx, tempChartConfig);  

      // change the "chart_status" to True so we dont keep creating chart objects for the same device
      document.getElementById(data.guid + '_tempChart').setAttribute('chart_status', 'true');
      
      // store the chart object in a jQuery data element. 
      $(device_element).data('tempChart', tempChart);
    }
    else{
      // retrieve the Chart onject from jQuery
      tempChart = $(device_element).data('tempChart');

      // push the latest temperature data to the Chart
      tempChart.data.labels.push('');
      tempChart.data.datasets.forEach((dataset) => {
        // if temp data point limit reached, shift out oldest data point
        if(dataset.data.length > 20){
          dataset.data.shift();
          tempChart.data.labels.shift();
        }
        dataset.data.push(data.temp);
      });

      // update the chart view
      tempChart.update();
    }
  })

  // ############################## Got Doorlock Device State ###############################################
  .on('webserver:doorlock_device:state', function(data){
    console.log("Doorlock: ", data.guid, data.lockState, data.doorState);

    var element_id = data.guid + '_lockstate';

    // update the image and button status
    if (data.lockState == DOOR_LOCK_CLUSTER_LockState.LOCKED){
      img = 'dist/img/lock.png';
      document.getElementById(data.guid + '_unlock_btn').disabled = false;
      document.getElementById(data.guid + '_lock_btn').disabled = true;
    }
    else if (data.lockState == DOOR_LOCK_CLUSTER_LockState.UNLOCKED) {
      img = 'dist/img/unlock.png';
      document.getElementById(data.guid + '_unlock_btn').disabled = true;
      document.getElementById(data.guid + '_lock_btn').disabled = false;
    }

    // update image and store cuurent lock state
    document.getElementById(element_id).src = img;
    document.getElementById(element_id).setAttribute('lockstate', data.lockState);
  })

  // ############################## Got Doorlock Device Set Response ########################################
  .on('webserver:doorlock_device:set_rsp', function(data){
    console.log("Doorlock Set RSP: ", data.guid, data.status);

    var element_id = data.guid + '_lockstate';

    // get the current lock state of the doorlock from html element
    var lock_state = document.getElementById(element_id).getAttribute('state');

    // if the response was unsuccessful
    if (data.status == STATUS.FAILURE){
      // if doorlock is currently unlocked, set the doorlock back to being locked
      if (lock_state == DOOR_LOCK_CLUSTER_LockState.UNLOCKED){
        document.getElementById(element_id).setAttribute('state', DOOR_LOCK_CLUSTER_LockState.LOCKED);
        document.getElementById(element_id).src = 'dist/img/lock.png';
        document.getElementById(data.guid + '_unlock_btn').disabled = false;
        document.getElementById(data.guid + '_lock_btn').disabled = true;
      }
      // if doorlock is currently locked, set the doorlock back to being unlocked
      else if (lock_state == DOOR_LOCK_CLUSTER_LockState.LOCKED){
        document.getElementById(element_id).setAttribute('state', DOOR_LOCK_CLUSTER_LockState.UNLOCKED);
        document.getElementById(element_id).src = 'dist/img/unlock.png';
        document.getElementById(data.guid + '_unlock_btn').disabled = true;
        document.getElementById(data.guid + '_lock_btn').disabled = false;
      }

      // get the modal element based on the guid for the door lock
      var modal_id = document.getElementById(data.guid + `_doorlock_modal`);

      // popup the doorlock error modal for the user
      $(document).ready(function() {
        $(modal_id).modal("show");
      });
    }
  })

  // ############################## Got Thermostat Device Attributes  #######################################
  .on('webserver:thermostat:attribute', function(data){
    // for each attribute in the array, determine what attribute type it is
    for (idx = 0; idx < data.attributeRecordList.length; idx++){
      switch(data.attributeRecordList[idx].attributeId){
        // local temperature
        case THERMOSTAT_CLUSTER.LocalTemperature:
          element_id = data.guid + '_thermostat_local_temp';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // cooling demand 
        case THERMOSTAT_CLUSTER.PICoolingDemand:
          element_id = data.guid + '_thermostat_cooling_demand';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // heating demand 
        case THERMOSTAT_CLUSTER.PIHeatingDemand:
          element_id = data.guid + '_thermostat_heating_demand';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // occupied cooling setpoint 
        case THERMOSTAT_CLUSTER.OccupiedCoolingSetpoint:
          var element_id = data.guid + '_thermostat_occupied_cooling_sp';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // occupied heating setpoint 
        case THERMOSTAT_CLUSTER.OccupiedHeatingSetpoint:
          element_id = data.guid + '_thermostat_occupied_heating_sp';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // min heat set point limit 
        case THERMOSTAT_CLUSTER.MinHeatSetpointLimit:
          element_id = data.guid + '_thermostat_min_heat_sp';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // max heat set point limit 
        case THERMOSTAT_CLUSTER.MaxHeatSetpointLimit:
          element_id = data.guid + '_thermostat_max_heat_sp';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // min cool set point limit 
        case THERMOSTAT_CLUSTER.MinCoolSetpointLimit:
          element_id = data.guid + '_thermostat_min_cool_sp';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // max cool set point limit 
        case THERMOSTAT_CLUSTER.MaxCoolSetpointLimit:
          element_id = data.guid + '_thermostat_max_cool_sp';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // control sequence of operation
        case THERMOSTAT_CLUSTER.ControlSequenceOfOperation:
          element_id = data.guid + '_thermostat_ctrl_seq_op';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break;

        // system mode
        case THERMOSTAT_CLUSTER.SystemMode:
          element_id = data.guid + '_thermostat_system_mode';
          document.getElementById(element_id).innerHTML = parseInt(data.attributeRecordList[idx].attributeValue, 16);
          break; 
      }
    }
  })

  // ############################## Light Device Got Unsupported Attribute for Level Control ################
  .on(EVT('zb-gateway', HA_CLUSTER_ID.LEVEL_CONTROL, STATUS.UNSUPPORTED_ATTRIBUTE), function(guid){
     // get the modal element based on the guid for the door lock
     var modal_id = document.getElementById(guid + `_level_control_modal`);

     // show the modal
     $(document).ready(function() {
       $(modal_id).modal("show");
     });
  }); 


  // ##########################################################################################################
  //                                  WebAppClient Object Functions
  // ##########################################################################################################

  //----------------------------------------------------------------------------------------------------------
  // networkButtonPress()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // handle toggling the state of the button used to open/close the network
  //----------------------------------------------------------------------------------------------------------
  WebAppClient.prototype.networkButtonPress = function() {  
    var network_button_state = document.getElementById('NetworkButton').getAttribute('state');

    // get the current state of the network button, then toggle the state
    if (network_button_state === 'closed'){
      $("#NetworkButton").text("Close Network");
      $("#NetworkButton").removeClass("btn btn-success").addClass("btn btn-danger");
      self.openNetwork();
      document.getElementById('NetworkButton').setAttribute('state', 'open');
    }
    else if (network_button_state === 'open'){
      $("#NetworkButton").text("Open Network");
      $("#NetworkButton").removeClass("btn btn-danger").addClass("btn btn-success");
      self.closeNetwork();
      document.getElementById('NetworkButton').setAttribute('state', 'closed');
    }

    // close the Dropdown Menu
    $(document).ready(function() {
      $("#collapsibleNavbar").collapse("toggle");
    });
  };

  //----------------------------------------------------------------------------------------------------------
  // openNetwork()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // open the network. start a timer to close the network after 180 seconds
  //----------------------------------------------------------------------------------------------------------
  WebAppClient.prototype.openNetwork = function() {   
    console.log("Open Network");

    //store the previous dev list so we know the new devices
    self.prevDeviceList = self.deviceList;  

    // send command add device
    self.socket.emit('network:open'); 

    //start a timer to close the network after 180 seconds
    self.AddDevTimer = setTimeout(function () {
      // get the current state of the network button
      var network_button_state = document.getElementById('NetworkButton').getAttribute('state');

      // if the network is still open/has not been closed yet, then close the network 
      if (network_button_state === 'open'){
        // close the network 
        self.closeNetwork();

        $("#NetworkButton").text("Open Network");
        $("#NetworkButton").removeClass("btn btn-danger").addClass("btn btn-success");
        document.getElementById('NetworkButton').setAttribute('state', 'closed');
      }
    }, 180000); 

  };

  //----------------------------------------------------------------------------------------------------------
  // closeNetwork()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // close the network
  //----------------------------------------------------------------------------------------------------------
  WebAppClient.prototype.closeNetwork = function() {   
    console.log("Close Network");

    // send command add device
    self.socket.emit('network:close'); 
  };


  //----------------------------------------------------------------------------------------------------------
  // hardSystemReset()
  //    - Inputs: NULL
  //    - Output: NULL
  //
  // Hard System Reset: includes restarting Linux Gateway and clearing all devices previously on network
  //----------------------------------------------------------------------------------------------------------
  WebAppClient.prototype.hardSystemReset = function() {   
    console.log("Hard System Reset");

    // erase all device cards from the webapp
    self.eraseDeviceList();

    // clear the device lists
    self.deviceList = [];
    self.unboundList = {};
    self.bindedList = {};

    // send command add device
    self.socket.emit('network:hard:reset'); 

    // close the Dropdown Menu
    $(document).ready(function() {
      $("#collapsibleNavbar").collapse("toggle");
    });

    // get the modal element based on the guid for the door lock
    var network_reset_modal_id = document.getElementById('networkResetModal');

    // popup the "waiting for network reset" modal
    $(document).ready(function() {
      $(network_reset_modal_id).modal("show");
    });
  };


  // Soft System Reset: incldues restarting the Linux Gateway, all devices remain 
  WebAppClient.prototype.softSystemReset = function() {   
    console.log("Soft System Reset");

    // send command add device
    self.socket.emit('network:soft:reset'); 

    // close the Dropdown Menu
    $(document).ready(function() {
      $("#collapsibleNavbar").collapse("toggle");
    });

    // get the modal element based on the guid for the door lock
    var network_reset_modal_id = document.getElementById('networkResetModal');

    // popup the "waiting for network reset" modal
    $(document).ready(function() {
      $(network_reset_modal_id).modal("show");
    });
  };


  // Remove Device Card from WebApp
  WebAppClient.prototype.removeDeviceCard = function(device_data) {   
    // get the device card HTML element from the GUID
    var elem = document.getElementById(device_data.data.guid);

    // remove the device card
    elem.parentNode.removeChild(elem);

    // erase the device from the unbound and binded device list
    self.eraseUnboundList_SwitchLight(device_data.data.guid);
    self.eraseBindedList_SwitchLight(device_data.data.guid);
    self.eraseUnboundList_DoorLockController(device_data.data.guid);
    self.eraseBindedList_DoorLockController(device_data.data.guid);
  };


  // send out a command to get the current device list
  WebAppClient.prototype.getNetworkInfo = function() { 
    self.socket.emit('network:getinfo');
  };
  

  // send out a command to get the current device list
  WebAppClient.prototype.getDeviceList = function() { 
    self.socket.emit('getDeviceList');
  };


  // send out a command to get the current Bind List
  WebAppClient.prototype.getBindList = function() { 
    console.log("WebAPP: Get Bind List");
    self.socket.emit('getBindList');
  };


  // Draw a List of all Devices on the Network, each device will be a DEvice Card
  WebAppClient.prototype.drawList = function(deviceList) { 
    for(var i in self.deviceList) { 
      var myElem = document.getElementById(self.deviceList[i].data.guid);
      if (myElem === null){
        self.addDeviceCard(self.deviceList[i]);
      }
    }
  };


  // Erase all Device Cards from the list
  WebAppClient.prototype.eraseDeviceList = function() { 
    for(var i in self.deviceList) { 
      console.log("Remove: ", self.deviceList[i].data.guid);
      //get the modal element based on the guid
      var device_id = document.getElementById(self.deviceList[i].data.guid);

      // empty the list item
      $(device_id).remove();
    }
  };


  // Draw the Current List of Unbound Pairs for Switch/Light pairs
  WebAppClient.prototype.drawUnboundList_SwitchLight = function() { 
    switchUnboundDeviceArray = self.unboundList["OnOffSwitch"];

    for(var i in switchUnboundDeviceArray) { 
      // get the destination device data
      destination_device_data = getDeviceDataFromGuid(switchUnboundDeviceArray[i][1]);

      //get the modal element based on the guid for the door lock
      var unboundlist_id = document.getElementById(switchUnboundDeviceArray[i][0] + `_switch_unboundlist`);

      var unbound_html_string = `
      <li class="list-group-item" id=` + switchUnboundDeviceArray[i][0] + switchUnboundDeviceArray[i][1] + `_switch_unbound_listitem>
        <div class="d-flex flex-row flex-wrap align-content-center justify-content-around">  
          <div class="p-2 align-self-center">
            <h4>` + destination_device_data.data.type + `</h4>
        </div>
        <div class="d-flex flex-row flex-wrap align-content-center justify-content-around">  
          <div class="p-2 align-self-center">ID: ` + destination_device_data.info.id + `
        </div>
        <div class="p-2 align-self-center">
          <button class="btn btn-primary float-right" guid=` + switchUnboundDeviceArray[i][1] + ` onclick="toggle_light_btn(this)" >Test Toggle</button>
        </div>
        <div class="p-2 align-self-center">
          <button class="btn btn-success float-right" guid=` + switchUnboundDeviceArray[i][1] + ` bind_device_guid=` + switchUnboundDeviceArray[i][0] + ` btn-type="_switch_unbound_listitem" onclick="bind_device_btn(this)">Bind</button>
        </div>
        </div>
      </li>
      `;

      // check if the item is already in the unbound list
      if (document.getElementById(switchUnboundDeviceArray[i][0] + switchUnboundDeviceArray[i][1] + `_switch_unbound_listitem`) == null){
        $(unboundlist_id).append(unbound_html_string);
      }
    }
  };


  // Draw the Current List of Unbound Pairs for DoorLockController pairs
  WebAppClient.prototype.drawUnboundList_DoorLockController = function() { 
    doorlockcontrollerUnboundDeviceArray = self.unboundList["DoorLockController"];

    for(var i in doorlockcontrollerUnboundDeviceArray) { 
      // get the destination device data
      destination_device_data = getDeviceDataFromGuid(doorlockcontrollerUnboundDeviceArray[i][1]);

      //get the modal element based on the guid for the door lock
      var unboundlist_id = document.getElementById(doorlockcontrollerUnboundDeviceArray[i][0] + `_doorlockcontroller_unboundlist`);

      var unbound_html_string = `
      <li class="list-group-item" id=` + doorlockcontrollerUnboundDeviceArray[i][0] + doorlockcontrollerUnboundDeviceArray[i][1] + `_doorlockcontroller_unbound_listitem>
        <div class="d-flex flex-row flex-wrap align-content-center justify-content-around">  
          <div class="p-2 align-self-center">
            <h4>` + destination_device_data.data.type + `</h4>
        </div>
        <div class="d-flex flex-row flex-wrap align-content-center justify-content-around">  
          <div class="p-2 align-self-center">ID: ` + destination_device_data.info.id + `
        </div>
        <div class="p-2 align-self-center">
          <button class="btn btn-success float-right" guid=` + doorlockcontrollerUnboundDeviceArray[i][1] + ` bind_device_guid=` + doorlockcontrollerUnboundDeviceArray[i][0] + ` btn-type="_doorlockcontroller_unbound_listitem" onclick="bind_device_btn(this)">Bind</button>
        </div>
        </div>
      </li>
      `;

      // check if the item is already in the unbound list
      if (document.getElementById(doorlockcontrollerUnboundDeviceArray[i][0] + doorlockcontrollerUnboundDeviceArray[i][1] + `_doorlockcontroller_unbound_listitem`) == null){
        $(unboundlist_id).append(unbound_html_string);
      }
    }
  };


  // Erase all Devices in the Unbound Switch/Light List 
  WebAppClient.prototype.eraseUnboundList_SwitchLight = function(guid) { 
    switchUnboundDeviceArray = self.unboundList["OnOffSwitch"];

    for(var i in switchUnboundDeviceArray) { 
      if (switchUnboundDeviceArray[i][1] == guid){
        //get the modal element based on the guid
        var unboundlist_id = document.getElementById(switchUnboundDeviceArray[i][0] + switchUnboundDeviceArray[i][1] + `_switch_unbound_listitem`);

        // empty the switch unbounded modal list
        $(unboundlist_id).remove();
      }
    }
  };


  // Erase all Devices in the Unbound Switch/Light List 
  WebAppClient.prototype.eraseBindedList_SwitchLight = function(guid) { 
    switchBindedDeviceArray = self.bindedList["OnOffSwitch"];

    for(var i in switchBindedDeviceArray) { 
      if (switchBindedDeviceArray[i][1] == guid){
        //get the modal element based on the guid
        var bindedlist_id = document.getElementById(switchBindedDeviceArray[i][0] + switchBindedDeviceArray[i][1] + `_switch_control_listitem`);

        // empty the switch unbounded modal list
        $(bindedlist_id).remove();
      }
    }
  };


  // Draw the Binded Switch/Light List 
  WebAppClient.prototype.drawBindedList_SwitchLight = function() { 

    switchBindedDeviceArray = self.bindedList["OnOffSwitch"];

    for(var i in switchBindedDeviceArray) {   
      // get the destination device data
      destination_device_data = getDeviceDataFromGuid(switchBindedDeviceArray[i][1]);

      // get the modal element based on the guid for the switch
      var bindedlist_id = document.getElementById(switchBindedDeviceArray[i][0] + `_switch_bindedlist`);

      var binded_html_string = `
      <li class="list-group-item list-group-item-action" id=` + switchBindedDeviceArray[i][0] + switchBindedDeviceArray[i][1] + `_switch_control_listitem>
        <div class="container">
          <div class="d-flex flex-row flex-wrap align-items-lg-center justify-content-around"> 
            
            <div class="p-2 align-self-center" style="font-size:25px;">
              ` + destination_device_data.data.type + `
            </div>

            <div class="p-2 align-self-center align-items-center" style="font-size:20px;">
              ID: ` + destination_device_data.info.id + `
            </div>

            <div class="p-2 align-self-center align-items-center" >
              <button class="btn btn-danger float-right" guid=` + switchBindedDeviceArray[i][1] + ` bind_device_guid=` + switchBindedDeviceArray[i][0] + ` btn-type="_switch_control_listitem"  onclick="unbind_device_btn(this)">Unbind</button>
            </div>
            
          </div>
        
        </div>
      </li>
      `;

      // check if the item is already in the binded list
      if (document.getElementById(switchBindedDeviceArray[i][0] + switchBindedDeviceArray[i][1] + `_switch_control_listitem`) == null){
        $(bindedlist_id).append(binded_html_string);
      }
    }
  };


  // Erase all Devices in the Unbound DoorlockController List 
  WebAppClient.prototype.eraseUnboundList_DoorLockController = function(guid) { 
    doorlockcontrollerUnboundDeviceArray = self.unboundList["DoorLockController"];

    for(var i in doorlockcontrollerUnboundDeviceArray) { 
      if (doorlockcontrollerUnboundDeviceArray[i][1] == guid){
        //get the modal element based on the guid
        var unboundlist_id = document.getElementById(doorlockcontrollerUnboundDeviceArray[i][0] + doorlockcontrollerUnboundDeviceArray[i][1] + `_doorlockcontroller_unbound_listitem`);

        // empty the switch unbounded modal list
        $(unboundlist_id).remove();
      }
    }
  };


  // Erase all Devices in the Unbound Doorlock Controller List 
  WebAppClient.prototype.eraseBindedList_DoorLockController = function(guid) { 
    doorlockcontrollerBindedDeviceArray = self.bindedList["DoorLockController"];

    for(var i in doorlockcontrollerBindedDeviceArray) { 
      if (doorlockcontrollerBindedDeviceArray[i][1] == guid){
        //get the modal element based on the guid
        var bindedlist_id = document.getElementById(doorlockcontrollerBindedDeviceArray[i][0] + doorlockcontrollerBindedDeviceArray[i][1] + `_doorlockcontroller_control_listitem`);

        // empty the switch unbounded modal list
        $(bindedlist_id).remove();
      }
    }
  };


   // Draw the Binded Switch/Light List 
   WebAppClient.prototype.drawBindedList_DoorLockController = function() { 

    doorlockcontrollerBindedDeviceArray = self.bindedList["DoorLockController"];

    for(var i in doorlockcontrollerBindedDeviceArray) {   
      // get the destination device data
      destination_device_data = getDeviceDataFromGuid(doorlockcontrollerBindedDeviceArray[i][1]);

      // get the modal element based on the guid for the door lock
      var bindedlist_id = document.getElementById(doorlockcontrollerBindedDeviceArray[i][0] + `_doorlockcontroller_bindedlist`);

      var binded_html_string = `
      <li class="list-group-item list-group-item-action" id=` + doorlockcontrollerBindedDeviceArray[i][0] + doorlockcontrollerBindedDeviceArray[i][1] + `_doorlockcontroller_control_listitem>
        <div class="container">
          <div class="d-flex flex-row flex-wrap align-items-lg-center justify-content-around"> 
                
            <div class="p-2 align-self-center" style="font-size:25px;">
              ` + destination_device_data.data.type + `
            </div>

            <div class="p-2 align-self-center align-items-center" style="font-size:20px;">
              ID: ` + destination_device_data.info.id + `
            </div>

            <div class="p-2 align-self-center align-items-center" >
            <button class="btn btn-danger float-right" guid=` + doorlockcontrollerBindedDeviceArray[i][1] + ` bind_device_guid=` + doorlockcontrollerBindedDeviceArray[i][0] + ` btn-type="_doorlockcontroller_control_listitem"  onclick="unbind_device_btn(this)">Unbind</button>
            </div>
            
          </div>
        </div>
      </li>
      `;

      // check if the item is already in the binded list
      if (document.getElementById(doorlockcontrollerBindedDeviceArray[i][0] + doorlockcontrollerBindedDeviceArray[i][1] + `_doorlockcontroller_control_listitem`) == null){
        $(bindedlist_id).append(binded_html_string);
      }
    }
  };


  // Draw/Add a Device Card to the List of device cards
  WebAppClient.prototype.addDeviceCard = function(device){
    var list_string = `
    <div class="container" id=` + device.data.guid + `>
      <div class="card">
        <h2 class="card-header">` + device.data.name + `<font size="4"> - ` + device.info.id + `</font>`;
    
    // add remove button to list item
    list_string += addRemoveButton(device);

    list_string += `
        </h2>
        <div class="card-body">`;

    // add the device specific features
    list_string += self.addDeviceFeatures(device);

    list_string += `
        </div>
      </div>
    <div class="mt-4 col-md-12"></div>
    </div>`;
   
    // write the new list item
    $('#addDevDevList').append(list_string);
  };


  // function to generate appropriate buttons, inputs, data display for end devices
  WebAppClient.prototype.addDeviceFeatures = function(device){
    var html_string = '';

    //-------------------------------------------------------------------------------------------------------
    // Temperature Sensor Content
    //-------------------------------------------------------------------------------------------------------
    if (device.data.type === 'TempSensor'){      

      html_string += `  
      <div class="d-flex flex-row flex-wrap align-content-center justify-content-center">  
        <div class="col-6 p-2" >
          <div class="progress" style="height:38px">
            <div id=` + device.data.guid + `_temp class="progress-bar font-weight-bold" role="progressbar" style="width: ` +  device.data.temp + `%; height: 200px;" aria-valuemin="0" aria-valuemax="50" >` + device.data.temp + `&#8451;</div>
          </div>
        </div>

        <div class="p-2" align="center">
            <button class="btn btn-primary float-right" guid=` + device.data.guid + ` onclick="get_temp_btn(this)">Poll Temp Sensor</button>
        </div>

        <div class="p-2" align="center">
          <button class="btn btn-primary" type="button" data-toggle="collapse" data-target="#` + device.data.guid + `_tempChart_collapse" aria-expanded="false" aria-controls="collapseExample">
            Toggle Plot
          </button>
        </div>
      </div>

      <div class="collapse" id=` + device.data.guid + `_tempChart_collapse>
          <canvas id=` + device.data.guid + `_tempChart chart_status="false"></canvas>
      </div>
      `;

      //get current temp of temp sensor
      data = {
        id: device.info.id,
        ieee: device.info.ieee,
        ep: device.info.ep
      };

      self.socket.emit('temp_device:get:temp', data);
    
    }

    //-------------------------------------------------------------------------------------------------------
    // OnOff Light Content
    //-------------------------------------------------------------------------------------------------------
    else if (device.data.type === 'OnOffLight'){
      html_string += `
      <div class="d-flex flex-row flex-wrap align-items-lg-center justify-content-around"> 
        <div class="p-2 align-self-center">
          <span class="float-left"><img id=` + device.data.guid + `_state state=` + ON_OFF_CLUSTER_OnOff.OFF + `></img></span> 
        </div>

        <div class="p-2 align-self-center align-items-center" >
          <button class="btn btn-success float-right" guid=` + device.data.guid + ` onclick="light_on_btn(this)">Light On</button>
        </div>

        <div class="p-2 align-self-center align-items-center" >
          <button class="btn btn-danger float-right" guid=` + device.data.guid + ` onclick="light_off_btn(this)">Light Off</button>
        </div>
        
      </div>`;

      //get current status of light
      data = {
        id: device.info.id,
        ieee: device.info.ieee,
        ep: device.info.ep
      };

      self.socket.emit('light_device:get:ONOFF', data);
    }

    //-------------------------------------------------------------------------------------------------------
    // Dimmable Light Content
    //-------------------------------------------------------------------------------------------------------
    else if (device.data.type === 'DimmableLight'){
      html_string += `
      <div class="d-flex flex-row flex-wrap align-items-lg-center justify-content-around"> 
        <div class="p-2 align-self-center">
          <span class="float-left"><img id=` + device.data.guid + `_state state=` + ON_OFF_CLUSTER_OnOff.OFF + `></img></span> 
        </div>
        
        <div class="p-2" align="center">
          <div class="col" align="center">
            <div class="row font-weight-bold justify-content-center align-items-center"> 
                <span>Set Level</span>
            </div>
            <div class="row justify-content-center align-items-center">
              <form class="slider-width200">
                  <input id=` + device.data.guid + `_level_slider` + ' guid=' + device.data.guid + ` style="width: auto; height: 20px;" type="range" value="0" min="0" max="100" onChange="sliderChangeLevel(this)"></input>
              </form>
            </div>
            <div class="row justify-content-center align-items-center" id=` + device.data.guid + `_level_value>
              0
            </div>
          </div>

          <div class="col">
            <div class="row font-weight-bold justify-content-center align-items-center">
              Set Transition Time
            </div>
            <div class="row justify-content-center align-items-center">
              <form class="slider-width200">
                  <input id=` + device.data.guid + `_transition_slider` + ' guid=' + device.data.guid + ` style="width: auto; height: 20px;" type="range" value="100" min="100" max="5000" step="100" onChange="sliderChangeTransition(this)"></input>
              </form>
            </div>
            <div class="row justify-content-center align-items-center" id=` + device.data.guid + `_transition_value>
              100
            </div>
          </div>
        </div>
        
        <div class="p-2 align-self-center align-items-center" >
          <div class="col">
            <div class="row">
              <button class="btn btn-success float-right" guid=` + device.data.guid + ` onclick="light_on_btn(this)">Light On</button>
            </div>
            <div class="row">
              <div class="mt-4 col-md-12"></div>
            </div>
            <div class="row">
              <button class="btn btn-danger float-right" guid=` + device.data.guid + ` onclick="light_off_btn(this)">Light Off</button>
            </div>
          </div>
        </div>
      </div>`;

      //get current status of light
      data = {
        id: device.info.id,
        ieee: device.info.ieee,
        ep: device.info.ep
      };

      self.socket.emit('light_device:get:ONOFF', data);
    }

    //-------------------------------------------------------------------------------------------------------
    // Color Light Content
    //-------------------------------------------------------------------------------------------------------
    else if (device.data.type === 'ColorLight'){
      html_string += `
      <div class="d-flex flex-row flex-wrap align-items-lg-center justify-content-around"> 
        <div class="p-2 align-self-center">
          <span class="float-left"><img id=` + device.data.guid + `_state state=` + ON_OFF_CLUSTER_OnOff.OFF + `></img></span> 
        </div>
        
        <div class="p-2" align="center">
          <div class="col" align="center">
            <div class="row font-weight-bold justify-content-center align-items-center"> 
                <span>Set Level</span>
            </div>
            <div class="row justify-content-center align-items-center">
              <form class="slider-width200">
                  <input id=` + device.data.guid + `_level_slider` + ' guid=' + device.data.guid + ` style="width: auto; height: 20px;" type="range" value="0" min="0" max="255" onChange="sliderChangeLevel(this)"></input>
              </form>
            </div>
            <div class="row justify-content-center align-items-center" id=` + device.data.guid + `_level_value>
              0
            </div>
          </div>

          <div class="col">
            <div class="row font-weight-bold justify-content-center align-items-center">
              Set Transition Time
            </div>
            <div class="row justify-content-center align-items-center">
              <form class="slider-width200">
                  <input id=` + device.data.guid + `_transition_slider` + ' guid=' + device.data.guid + ` style="width: auto; height: 20px;" type="range" value="100" min="100" max="10000" step="100" onChange="sliderChangeLevel(this)"></input>
              </form>
            </div>
            <div class="row justify-content-center align-items-center" id=` + device.data.guid + `_transition_value>
              100
            </div>
          </div>          
        </div>
        
        <div class="p-2 align-self-center align-items-center" >
          <div class="col">
              <div class="row font-weight-bold justify-content-center align-items-center">
                Set Hue Value
              </div>
              <div class="row justify-content-center align-items-center">
                <form class="slidercolor-width200">
                    <input id=` + device.data.guid + `_hue_slider` + ' guid=' + device.data.guid + ` style="width: auto; height: 20px;" type="range" class="colorslider" value="100" min="0" max="254" step="1" onChange="sliderChangeHue(this)"></input>
                </form>
              </div>
              <div class="row justify-content-center align-items-center" id=` + device.data.guid + `_hue_value>
                100
              </div>
            </div>

            <div class="col">
            <div class="row font-weight-bold justify-content-center align-items-center">
              Set Saturation Value
            </div>
            <div class="row justify-content-center align-items-center">
              <form class="slider-width200">
                  <input id=` + device.data.guid + `_saturation_slider` + ' guid=' + device.data.guid + ` style="width: auto; height: 20px;" type="range" value="100" min="0" max="254" step="1" onChange="sliderChangeSaturation(this)"></input>
              </form>
            </div>
            <div class="row justify-content-center align-items-center" id=` + device.data.guid + `_saturation_value>
              100
            </div>
          </div>
        </div>

        <div class="p-2 align-self-center align-items-center" >
          <div class="col">
            <div class="row">
              <button class="btn btn-success float-right" guid=` + device.data.guid + ` onclick="light_on_btn(this)">Light On</button>
            </div>
            <div class="row">
              <div class="mt-4 col-md-12"></div>
            </div>
            <div class="row">
              <button class="btn btn-danger float-right" guid=` + device.data.guid + ` onclick="light_off_btn(this)">Light Off</button>
            </div>
          </div>
        </div>
      </div>
      
      <div class="modal fade" id=` + device.data.guid + `_level_control_modal tabindex="-1" role="dialog" aria-hidden="true">
        <div class="modal-dialog" role="document">
          <div class="modal-content">
            <div class="modal-header">
              <h5 class="modal-title" >Error</h5>
              <button type="button" class="close" data-dismiss="modal" aria-label="Close">
                <span aria-hidden="true">&times;</span>
              </button>
            </div>
            <div class="modal-body">
              Level Control: Was unable to set attribute.
            </div>
          </div>
        </div>
      </div>`;

      //get current status of light
      data = {
        id: device.info.id,
        ieee: device.info.ieee,
        ep: device.info.ep
      };

      self.socket.emit('light_device:get:ONOFF', data);
      self.socket.emit('light_device:get:level', data);
      self.socket.emit('light_device:get:color', data);
    }

    //-------------------------------------------------------------------------------------------------------
    // ON OFF Switch Content
    //-------------------------------------------------------------------------------------------------------
    else if (device.data.type === 'OnOffSwitch'){
      html_string += `  
      <div class="container"> 
        <button id=` + device.data.guid + `_bind_btn class="btn btn-success btn-block" guid=` + device.data.guid + ` onclick="bind_modal_switchlight_btn(this)">Bind Devices</button>
        <div class="mt-4 col-md-12"></div>
        <ul id=` + device.data.guid + `_switch_bindedlist class="list-group"></ul>
      </div>

      <div class="modal fade" id=` + device.data.guid + `_switch_modal tabindex="-1" role="dialog" aria-hidden="true">
        <div class="modal-dialog" role="document">
          <div class="modal-content">
            <div class="modal-header">
              <h5 class="modal-title" >Bind Devices to the Switch</h5>
              <button type="button" class="close" data-dismiss="modal" aria-label="Close">
                <span aria-hidden="true">&times;</span>
              </button>
            </div>
            <div class="modal-body">
              <ul id=` + device.data.guid + `_switch_unboundlist class="list-group"></ul>
            </div>
          </div>
        </div>
      </div>
      `;
    
    }

    //-------------------------------------------------------------------------------------------------------
    // Doorlock Content
    //-------------------------------------------------------------------------------------------------------
    else if (device.data.type === 'DoorLock'){
      html_string += `
        <div class="d-flex flex-row flex-wrap align-items-center justify-content-around"> 
          <div class="p-2">
            <span class="float-center"><img id=` + device.data.guid + `_lockstate state=0 ></img></span> 
          </div>

          <div class="p-2" align="center">
            <form class="form-inline">
              <div class="form-group">
                  <h3 class = mx-2>Pin:</h3>
                  <input id=` + device.data.guid + `_pin_input` + ' guid=' + device.data.guid + ` type="number" class="form-control" value="1234" min="0" max="9999" step="1" aria-label="" aria-describedby="basic-addon1"></input>
              </div>
            </form>
          </div>

          <div class="p-2" align="center">
            <div class="d-flex flex-row flex-wrap align-items-center justify-content-around"> 
              <div class="p-2" align="center">
                  <button id=` + device.data.guid + `_unlock_btn class="btn btn-primary float-right" guid=` + device.data.guid + ` onclick="set_doorlock_unlock_btn(this)">Unlock</button>
              </div>

              <div class="p-2" align="center">
                  <button id=` + device.data.guid + `_lock_btn class="btn btn-primary float-right" guid=` + device.data.guid + ` onclick="set_doorlock_lock_btn(this)">Lock</button>
              </div>
            </div>
          </div>
        </div>

      <div class="modal fade" id=` + device.data.guid + `_doorlock_modal tabindex="-1" role="dialog" aria-hidden="true">
        <div class="modal-dialog" role="document">
          <div class="modal-content">
            <div class="modal-header">
              <h5 class="modal-title" >Unable to Set Lock</h5>
              <button type="button" class="close" data-dismiss="modal" aria-label="Close">
                <span aria-hidden="true">&times;</span>
              </button>
            </div>
            <div class="modal-body">
              Please Enter the Correct Pin and Try Again. 
            </div>
          </div>
        </div>
      </div>
      `;


      //get current state of the doorlock. Assume a default pin of 1234.
      data = {
        id: device.info.id,
        ieee: device.info.ieee,
        ep: device.info.ep,
        pin: [1, 2, 3, 4]
      };

      // send command
      self.socket.emit('doorlock_device:get:state', data);
    }


    //-------------------------------------------------------------------------------------------------------
    // Door Lock Controller Content
    //-------------------------------------------------------------------------------------------------------
    else if (device.data.type === 'DoorLockController'){
      html_string += `  
      <div class="container"> 
        <button id=` + device.data.guid + `_bind_btn class="btn btn-success btn-block" guid=` + device.data.guid + ` onclick="bind_modal_doorlockcontroller_btn(this)">Bind Devices</button>
        <div class="mt-4 col-md-12"></div>
        <ul id=` + device.data.guid + `_doorlockcontroller_bindedlist class="list-group"></ul>
      </div>

      <div class="modal fade" id=` + device.data.guid + `_doorlockcontroller_modal tabindex="-1" role="dialog" aria-hidden="true">
        <div class="modal-dialog" role="document">
          <div class="modal-content">
            <div class="modal-header">
              <h5 class="modal-title" >Bind Devices to the Door Lock Controller</h5>
              <button type="button" class="close" data-dismiss="modal" aria-label="Close">
                <span aria-hidden="true">&times;</span>
              </button>
            </div>
            <div class="modal-body">
              <ul id=` + device.data.guid + `_doorlockcontroller_unboundlist class="list-group"></ul>
            </div>
          </div>
        </div>
      </div>
      `;
    }


    //-------------------------------------------------------------------------------------------------------
    // Thermostat Content
    //-------------------------------------------------------------------------------------------------------
    else if (device.data.type === 'Thermostat'){
      html_string += `
      <div class="d-flex flex-row flex-wrap align-items-center justify-content-center"> 
    
        <div class="p-2" align="center">
          <ul class="list-group">
            <li class="list-group-item">Local Temperature
              <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_local_temp ></span>
            </li>
            <li class="list-group-item">Cooling Demand
              <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_cooling_demand ></span>
            </li>
            <li class="list-group-item">Heating Demand
              <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_heating_demand ></span>
            </li>
            <li class="list-group-item">Occupied Cooling Set Point 
              <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_occupied_cooling_sp ></span>
            </li>
          </ul>
        </div>

        <div class="p-2" align="center">
          <ul class="list-group">
            <li class="list-group-item">Occupied Heating Set Point 
              <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_occupied_heating_sp ></span>
            </li>
            <li class="list-group-item">Min Heat Set Point Limit
              <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_min_heat_sp ></span>
            </li>
            <li class="list-group-item">Max Heat Set Point Limit
              <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_max_heat_sp ></span>
            </li>
            <li class="list-group-item">Min Cool Set Point Limit
              <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_min_cool_sp ></span>
            </li>
          </ul>
        </div>

        <div class="p-2" align="center">
        <ul class="list-group">
          <li class="list-group-item">Max Cool Set Point Limit
            <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_max_cool_sp ></span>
          </li>
          <li class="list-group-item">Control Sequence of Operation
            <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_ctrl_seq_op ></span>
          </li>
          <li class="list-group-item">System Mode
            <span class="badge badge-default badge-pill" id=` + device.data.guid + `_thermostat_system_mode ></span>
          </li>
        </ul>
      </div>
        
        <div class="p-2">
            <button id=` + device.data.guid + `_thermostat_poll_btn class="btn btn-primary float-right" guid=` + device.data.guid + ` onclick="thermostat_poll_btn(this)">Poll Thermostat</button>
        </div>

      </div>
      `;

    
    }

    return html_string;
  };


  // Remove Device fom Network
  WebAppClient.prototype.removeDevice = function(device_data) {   
    device_data = getDeviceData(device_data);

    remove_device_data = {
      ieee: device_data.info.ieee,
      guid:  device_data.data.guid
    };

    // if there is an IEEE modal, send a Remove Device command
    if (device_data.info.ieee != null){
      self.socket.emit('removeDevice', remove_device_data);
    }
    else {
      console.log("ERROR: Device IEEE not found");
    }

    // get the device card HTML element from the GUID
    var elem = document.getElementById(device_data.data.guid);

    // remove the device card
    elem.parentNode.removeChild(elem);

    // erase the device from the unbound and binded device list
    self.eraseUnboundList_SwitchLight(device_data.data.guid);
    self.eraseBindedList_SwitchLight(device_data.data.guid);
    self.eraseUnboundList_DoorLockController(device_data.data.guid);
    self.eraseBindedList_DoorLockController(device_data.data.guid);
  };


  // -------------------- Start Light Device Functions --------------------

  // Toggle a Light Device
  WebAppClient.prototype.toggleLight = function(device_data) { 
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the id of the light that keeps track of the state of the light
    var element_id = device_data.data.guid + '_state';
    var state =  document.getElementById(element_id).getAttribute('state');

    // toggle the current state of the light
    if (state == ON_OFF_CLUSTER_OnOff.OFF){
      state = ON_OFF_CLUSTER_OnOff.ON;
    }
    else if (state == ON_OFF_CLUSTER_OnOff.ON){
      state = ON_OFF_CLUSTER_OnOff.OFF;
    }
    
    // construct the data object that will set the state of the light
    var data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      on: state  
    };
    
    // send command
    self.socket.emit('light_device:set:ONOFF', data);
    
    // set the current state of the light 
    if (state == ON_OFF_CLUSTER_OnOff.OFF){
      document.getElementById(element_id).src = 'dist/img/light_off.png';
      document.getElementById(element_id).setAttribute('state', ON_OFF_CLUSTER_OnOff.OFF);
    }
    else if (state == ON_OFF_CLUSTER_OnOff.ON){
      document.getElementById(element_id).src = 'dist/img/light_on.png';
      document.getElementById(element_id).setAttribute('state', ON_OFF_CLUSTER_OnOff.ON);
    }
  };


  // Turn On a Light Device
  WebAppClient.prototype.lightOn = function(device_data) { 
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the id of the light that keeps track of the state of the light
    var element_id = device_data.data.guid + '_state';
    
    // construct the data object that will set the state of the light
    var data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      on: ON_OFF_CLUSTER_OnOff.ON  
    };
    
    // send command
    self.socket.emit('light_device:set:ONOFF', data);
    
    // update the state of the device to ON
    document.getElementById(element_id).src = 'dist/img/light_on.png';
    document.getElementById(element_id).setAttribute('state', ON_OFF_CLUSTER_OnOff.ON);
  };


  // Turn Off a Light Device
  WebAppClient.prototype.lightOff = function(device_data) { 
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the id of the light that keeps track of the state of the light
    var element_id = device_data.data.guid + '_state';
    
    // construct the data object that will set the state of the light
    var data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      on: ON_OFF_CLUSTER_OnOff.OFF  
    };
    
    // send command
    self.socket.emit('light_device:set:ONOFF', data);
    
    // update the state of the device to OFF
    document.getElementById(element_id).src = 'dist/img/light_off.png';
    document.getElementById(element_id).setAttribute('state', ON_OFF_CLUSTER_OnOff.OFF);
  };


  // Set the Level/Transition values of a Light Device
  WebAppClient.prototype.setLevelTransition = function(device_data){
    // get the button properties
    device_data = getDeviceData(device_data);

    // acquire all of the slider values
    element_id = device_data.data.guid + '_level_slider';
    light_level = document.getElementById(element_id).value;

    element_id = device_data.data.guid + '_transition_slider';
    light_transition = document.getElementById(element_id).value;

    element_id = device_data.data.guid + '_level_value';
    document.getElementById(element_id).innerHTML = light_level;

    element_id = device_data.data.guid + '_transition_value';
    document.getElementById(element_id).innerHTML = light_transition;

    console.log("The light level is: ", light_level, "The transition level is: ", light_transition);

    // construct the data object that will set the state of the light
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      level: light_level,
      transitionTime: light_transition
    };
    
    // send the command
    self.socket.emit('light_device:set:level_transition', data);
  };


  // Set the On/Off Transition of a Light Device
  WebAppClient.prototype.setOnOffTransition = function(device_data){
    // get the button properties 
    device_data = getDeviceData(device_data);

    // get the values of the sliders
    element_id = device_data.data.guid + '_transition_slider';
    light_transition = document.getElementById(element_id).value;

    element_id = device_data.data.guid + '_transition_value';
    document.getElementById(element_id).innerHTML = light_transition;

    console.log("The transition level is: ", light_transition);

    // construct the data object that will set the state of the light
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      transitionTime: light_transition
    };
    
    // send the command
    self.socket.emit('light_device:set:onoff_transition', data);
  };


  // Set the Color of a Color Light Device
  WebAppClient.prototype.setColor = function(device_data){
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the current slider values
    element_id = device_data.data.guid + '_hue_slider';
    light_hue = document.getElementById(element_id).value;

    element_id = device_data.data.guid + '_saturation_slider';
    light_saturation = document.getElementById(element_id).value;

    element_id = device_data.data.guid + '_hue_value';
    document.getElementById(element_id).innerHTML = light_hue;

    element_id = device_data.data.guid + '_saturation_value';
    document.getElementById(element_id).innerHTML = light_saturation;

    console.log("The color light hue is: ", light_hue, " The saturation: ", light_saturation);

    // construct the data object that will set the state of the light
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      hue: light_hue,
      saturation: light_saturation
    };
    
    // send command
    self.socket.emit('light_device:set:color', data);
  };

  // -------------------- End Light Device Functions --------------------


  // -------------------- Start Temperature Device Functions --------------------

  // Get the Temperature of a Temperature Device
  WebAppClient.prototype.getTemp = function(device_data){
    // get the button properties
    device_data = getDeviceData(device_data);

    // construct the data object that will set the state of the light
    //get current temp of temp sensor
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep
    };

    // send command
    self.socket.emit('temp_device:get:temp', data);
  };

  // Set the Report Interval of a Temperature Device
  WebAppClient.prototype.TempSensor_setReportInterval = function(device_data){
    // get the button properties
    device_data = getDeviceData(device_data);

    element_id = device_data.data.guid + '_report_input';
    reportInterval = document.getElementById(element_id).value;

    // make into whole number
    reportInterval = Math.round(reportInterval);

    if (reportInterval < 0) {
      reportInterval = 0;
    }
          
    attrRec= {
      attributeId: 0, //MeasuredValue
      attributeType: DATA_TYPE.INT16, 
      minReportInterval: (reportInterval),
      maxReportInterval: 60, //defined by HA spec
      reportableChange: 0 //force report to be sent on min interval
    };

    // construct the data object that will set the state of the light
    //get current temp of temp sensor
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      clusterId: HA_CLUSTER_ID.TEMPERATURE_MEASUREMENT,
      attrList: attrRec
    };

    // send command    
    self.socket.emit('temp_device:set:interval', data);
  };

  // -------------------- End Temperature Device Functions --------------------

  // -------------------- Start Doorlock Device Functions --------------------

  // Get the State of a DoorLock Device
  WebAppClient.prototype.getDoorLockState = function(device_data) {
    // get the button properties
    device_data = getDeviceData(device_data);

    // construct the data object that will get the state of the door lock
    // get current state of door lock
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep
    };

    // send the command
    self.socket.emit('doorlock_device:get:state', data);
  };


  // Set the DoorLock State to "Locked"
  WebAppClient.prototype.setDoorLockState_Lock = function(device_data) {
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the id of the light that keeps track of the state of the light
    element_id = device_data.data.guid + '_lockstate';

    // get the pin value from the input field
    pin_input = document.getElementById(device_data.data.guid + '_pin_input').value;

    // construct the data object that will set the state of the door lock
    // set state of door lock
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      pin:  pin_input.split("", 4)
    };

    // send command
    self.socket.emit('doorlock_device:set:lock', data);

    // update the DoorLock state, image and buttons
    document.getElementById(element_id).src = 'dist/img/lock.png';
    document.getElementById(element_id).setAttribute('state', DOOR_LOCK_CLUSTER_LockState.LOCKED);
    document.getElementById(device_data.data.guid + '_unlock_btn').disabled = false;
    document.getElementById(device_data.data.guid + '_lock_btn').disabled = true;
  };


  // Set the DoorLock State to "UnLocked"
  WebAppClient.prototype.setDoorLockState_Unlock = function(device_data) {
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the id of the light that keeps track of the state of the light
    var element_id = device_data.data.guid + '_lockstate';

    // get the pin value from the input field
    pin_input = document.getElementById(device_data.data.guid + '_pin_input').value;

    // construct the data object that will set the state of the door lock
    // set state of door lock
    var data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      pin: pin_input.split("", 4)
    };

    // send the command 
    self.socket.emit('doorlock_device:set:unlock', data);

    // update the DoorLock state, image and buttons
    document.getElementById(element_id).src = 'dist/img/unlock.png';
    document.getElementById(element_id).setAttribute('state', DOOR_LOCK_CLUSTER_LockState.UNLOCKED);
    document.getElementById(device_data.data.guid + '_unlock_btn').disabled = true;
    document.getElementById(device_data.data.guid + '_lock_btn').disabled = false;
  };

  // -------------------- End Doorlock Device Functions --------------------


  // -------------------- Start Thermostat Device Functions --------------------

  // Set a Thermostat Device to Increase by 10 degrees
  WebAppClient.prototype.setpointThermostat_increase = function(device_data) {
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the id of the light that keeps track of the state of the light
    element_id = device_data.data.guid + '_thermostat_options';

    thermostat_option = document.getElementById(element_id).value;

    // construct the data object that will set the state of the door lock
    // set thermostat to increase by 10 degrees
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      mode: thermostat_option,
      amount: 10
    };

    // send the command
    self.socket.emit('thermostat_device:change:setpoint', data);
  };


  // Set a Thermostat Device to Decrease by 10 degrees
  WebAppClient.prototype.setpointThermostat_decrease = function(device_data) {
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the id of the thermostat_options
    element_id = device_data.data.guid + '_thermostat_options';

    // get the thermostat option value
    thermostat_option = document.getElementById(element_id).value;

    // construct the data object that will set the thermostat
    // set thermostat to decrease by 10 degrees
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep,
      mode: thermostat_option,
      amount: -10
    };

    // send the command
    self.socket.emit('thermostat_device:change:setpoint', data);
  };


  // Poll the Thermostat for its current Attributes
  WebAppClient.prototype.Thermostat_poll = function(device_data) {
    // get the button properties
    device_data = getDeviceData(device_data);

    // construct the data object that will set the state of the thermostat
    // set state of door lock
    data = {
      id: device_data.info.id,
      ieee: device_data.info.ieee,
      ep: device_data.info.ep
    };

    // send command
    self.socket.emit('thermostat_device:get:all:attributes', data);
  };

  // -------------------- End Thermostat Device Functions --------------------


  // -------------------- Start Bind Device Functions --------------------

  // Bind Two Devices (sourceDevice and destDevice) that are defined in HTML element
  WebAppClient.prototype.bindDevice = function(device_data) {
    // get the button properties
    bind_device_data = getBindData(device_data);

    // construct the data object that will bind the sourceDevice to the destDevice
    data = {
      source_id: bind_device_data.sourceDevice.info.id,
      source_ieee: bind_device_data.sourceDevice.info.ieee,
      source_ep: bind_device_data.sourceDevice.info.ep,
      dest_id: bind_device_data.destDevice.info.id,
      dest_ieee: bind_device_data.destDevice.info.ieee,
      dest_ep: bind_device_data.destDevice.info.ep,
      clusterId: bind_device_data.clusterId
    };

    // send command
    self.socket.emit('binding:set:bind', data);

    // create HTML progress bar element
    progress_html = `
    <div class="progress">
      <div class="progress-bar progress-bar-striped progress-bar-animated" role="progressbar" aria-valuenow="100" aria-valuemin="0" aria-valuemax="100" style="width: 100%"></div>
    </div>
    `;

    console.log("Bind Device: ", bind_device_data.destDevice.data.guid, bind_device_data.btnType);

    // draw the HTML progress bar in the element list defined by destDevice GUID and button type
    document.getElementById(bind_device_data.sourceDevice.data.guid + bind_device_data.destDevice.data.guid +  bind_device_data.btnType).innerHTML = progress_html;

    // Update the current bind flag to consist of the destDevice GUID and the button type
    self.BindFlag = bind_device_data.sourceDevice.data.guid + bind_device_data.destDevice.data.guid +  bind_device_data.btnType;
  };


  // Un-Bind Two Devices (sourceDevice and destDevice) that are defined in HTML element
  WebAppClient.prototype.unbindDevice = function(device_data) {
    // get the button properties
    var bind_device_data = getBindData(device_data);

    // construct the data object that will un-bind the sourceDevice to the destDevice
    var data = {
      source_id: bind_device_data.sourceDevice.info.id,
      source_ieee: bind_device_data.sourceDevice.info.ieee,
      source_ep: bind_device_data.sourceDevice.info.ep,
      dest_id: bind_device_data.destDevice.info.id,
      dest_ieee: bind_device_data.destDevice.info.ieee,
      dest_ep: bind_device_data.destDevice.info.ep,
      clusterId: bind_device_data.clusterId
    };

    // send command
    self.socket.emit('binding:set:unbind', data);

    // create HTML progress bar element
    progress_html = `
    <div class="progress">
      <div class="progress-bar progress-bar-striped progress-bar-animated" role="progressbar" aria-valuenow="100" aria-valuemin="0" aria-valuemax="100" style="width: 100%"></div>
    </div>
    `;

    // draw the HTML progress bar in the element list defined by destDevice GUID and button type
    document.getElementById(bind_device_data.sourceDevice.data.guid + bind_device_data.destDevice.data.guid +  bind_device_data.btnType).innerHTML = progress_html;

    // Update the current bind flag to consist of the destDevice GUID and the button type
    self.BindFlag = bind_device_data.sourceDevice.data.guid + bind_device_data.destDevice.data.guid +  bind_device_data.btnType;
  };


  //----------------------------------------------------------------------------------------------------------
  // getBindData()
  //    -Input: The html element of a button, contains html elements with "guid", "bind_device_guid", and "btn-type"
  //    -Return: an object with all the necessary data to either make a bind or unbind request
  // 
  // generate the necessary data needed to either bind or unbind a device. 
  //----------------------------------------------------------------------------------------------------------
  function getBindData(device_data){
    var device_data_guid = device_data.getAttribute('guid');
    var bind_device_data_guid = device_data.getAttribute('bind_device_guid');
    var source_btn_type = device_data.getAttribute('btn-type');

    for(var i in self.deviceList) { 
      if (device_data_guid == self.deviceList[i].data.guid){
          dest_device = self.deviceList[i];
      }
    }

    for(var j in self.deviceList) { 
      if (bind_device_data_guid == self.deviceList[j].data.guid){
          source_device = self.deviceList[j];
      }
    }

    bind_data = {
      sourceDevice: source_device,
      destDevice: dest_device,
      clusterId: source_device.data.clusterId,
      btnType: source_btn_type
    };

    return bind_data;
  }


  // Function to Open the Bind Modal for Switch Devices
  WebAppClient.prototype.openBindModal_SwitchLight = function(device_data) {
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the modal element based on the guid of the switch modal
    modal_id = document.getElementById(device_data.data.guid + `_switch_modal`);

    // show the switch bind modal
    $(document).ready(function() {
      $(modal_id).modal("show");
    });

    // draw the current unbound list in the modal
    self.drawUnboundList_SwitchLight();
  };


  // Function to Open the Bind Modal for Door Lock Controller Devices
  WebAppClient.prototype.openBindModal_DoorLockController = function(device_data) {
    // get the button properties
    device_data = getDeviceData(device_data);

    // get the modal element based on the guid of the door lock controller modal
    modal_id = document.getElementById(device_data.data.guid + `_doorlockcontroller_modal`);

    // show the doorlock controller bind modal
    $(document).ready(function() {
      $(modal_id).modal("show");
    });

    // draw the current unbound list in the modal
    self.drawUnboundList_DoorLockController();
  };

  // -------------------- End Bind Device Functions --------------------



  //----------------------------------------------------------------------------------------------------------
  // addRemoveButton()
  //    -Input: The device object from the deviceList
  //    -Return: a HTML string that creates a Remove button with the appropriate guid inserted
  // 
  // create HTML code thatwill display a remove button with proper GUID and onclick function
  //----------------------------------------------------------------------------------------------------------
  function addRemoveButton(device){
    // create button
    var html_string = `
    <span class="float-right">
      <button class="close" aria-label="Close" guid=` + device.data.guid + ` onclick="remove_btn(this)">
        <span aria-hidden="true">&times;</span>
      </button>
    </span>`;
    
    return html_string;
  }


  //----------------------------------------------------------------------------------------------------------
  // getDeviceData()
  //    -Input: The html element of a button, contains html elements with "guid"
  //    -Return: the deviceList object that matches the GUID
  // 
  // get data of a particular device by finding matching device guid located in HTML element
  //----------------------------------------------------------------------------------------------------------
  function getDeviceData(device_data){
    var device_data_guid = device_data.getAttribute('guid');

    for(var i in self.deviceList) { 
      if (device_data_guid == self.deviceList[i].data.guid){
          return self.deviceList[i];
      }
    }
  }


  //----------------------------------------------------------------------------------------------------------
  // getDeviceDataFromGuid()
  //    -Input: GUID of device data desired
  //    -Return: the deviceList object that matches the GUID
  // 
  // get data of a particular device by finding matching device guid
  //----------------------------------------------------------------------------------------------------------
  function getDeviceDataFromGuid(guid){
    for(var i in self.deviceList) { 
      if (guid == self.deviceList[i].data.guid){
          return self.deviceList[i];
      }
    }
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


  // ------------ Run on Initial Object Call ----------------
  self.getNetworkInfo();

  // get the current list of devices and bindable devices
  self.getDeviceList();

  // get the latest binding list
  self.getBindList();

}