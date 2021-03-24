# <a name="nodejs-technical-user-guide"></a>
## Node.js Zigbee Gateway - Technical User Guide

* [zb-gateway.js](#zb-gatway-js)
    * [Incoming Events - Network Manager](#zb-gateway-incoming-events-nwkmgr)
        * [nwkmgr:newDev](#nwkmgr-newDev)
        * [nwkmgr:removeDev](#nwkmgr-removeDev)
        * [nwkmgr:network:ready](#nwkmgr-network-ready)
        * [nwkmgr:binding:rsp](#nwkmgr-binding-rsp)
    * [Incoming Events - Home Automation Gateway](#zb-gateway-incoming-events-hagw)
        * [hagateway:light_device:state](#hagateway-light_device-state)
        * [hagateway:light_device:level](#hagateway-light_device-level)
        * [hagateway:light_device:color](#hagateway-light_device-color)
        * [hagateway:temp_device:temp](#hagateway-temp_device-temp)
        * [hagateway:doorlock_device:state](#hagateway-doorlock_device-state)
        * [hagateway:doorlock_device:set_rsp](#hagateway-doorlock_device-set_rsp)
        * [hagateway:attribute:read:rsp](#hagateway-attribute-read-rsp)
        * [hagateway:attribute:write:rsp](#hagateway-attribute-write-rsp)
        * [hagateway:gw:report](#hagateway-gw-report)
        * [hagateway:network:connected](#hagateway-network-connected)
    * [Local Functions](#zb-gateway-local-functions)
        * [zb_gateway_getNetworkInfo](#zb_gateway_getNetworkInfo)
        * [zb_gateway_openNetwork](#zb_gateway_openNetwork)
        * [zb_gateway_closeNetwork](#zb_gateway_closeNetwork)
        * [zb_gateway_hardSystemReset](#zb_gateway_hardSystemReset)
        * [zb_gateway_softSystemReset](#zb_gateway_softSystemReset)
        * [zb_gateway_getDeviceList](#zb_gateway_getDeviceList)
        * [zb_gateway_getBindList](#zb_gateway_getBindList)
        * [zb_gateway_removeDevice](#zb_gateway_removeDevice)
        * [checkPossibleBinding](#zb-gateway-checkPossibleBinding)
        * [removeBindingItembyGuid](#zb-gateway-removeBindingItembyGuid)
        * [updateBindingList](#zb-gateway-updateBindingList)
        * [checkGUIDinDevArray](#zb-gateway-checkGUIDinDevArray)
        * [EVT](#zb-gateway-EVT)
    * [Device Functions](#zb-gateway-device-functions)
        * [OnOffLight](#zb-gateway-OnOffLight)
            * [set_ONOFF_state](#OnOffLight-set_ONOFF_state)
            * [get_ONOFF_state](#OnOffLight-get_ONOFF_state)
        * [DimmableLight](#zb-gateway-DimmableLight)
            * [set_ONOFF_state](#DimmableLight-set_ONOFF_state)
            * [get_ONOFF_state](#DimmableLight-get_ONOFF_state)
            * [get_level](#DimmableLight-get_level)
            * [set_level_transition](#DimmableLight-set_level_transition)
            * [set_onoff_transition](#DimmableLight-set_onoff_transition)
        * [ColorLight](#zb-gateway-ColorLight)
            * [set_ONOFF_state](#ColorLight-set_ONOFF_state)
            * [get_ONOFF_state](#ColorLight-get_ONOFF_state)
            * [set_level_transition](#ColorLight-set_level_transition)
            * [set_onoff_transition](#ColorLight-set_onoff_transition)
            * [set_color](#ColorLight-set_color)
            * [get_color](#ColorLight-get_color)
        * [TempSensor](#zb-gateway-TempSensor)
            * [get_temp](#TempSensor-get_temp)
            * [set_interval](#TempSensor-set_interval)
        * [Switch Device](#zb-gateway-SwitchDevice)
            * [get_state](#SwitchDevice-get_state)
        * [DoorLockDevice](#zb-gateway-DoorLockDevice)
            * [get_state](#DoorLockDevice-get_state)
            * [unlock](#DoorLockDevice-unlock)
            * [lock](#DoorLockDevice-lock)
        * [ThermostatDevice](#zb-gateway-ThermostatDevice)
            * [setpoint_change](#ThermostatDevice-setpoint_change)
            * [get_minheat_setpoint](#ThermostatDevice-get_minheat_setpoint)
            * [get_all_attributes](#ThermostatDevice-get_all_attributes)
        * [Device Binding](#zb-gateway-Device-Binding)
            * [set_binding](#Device-Binding-set_binding)
            * [set_unbinding](#Device-Binding-set_unbinding)
    * [External Functions](#zb-gateway-external-functions)
        * [getNetworkInfo](#External-Functions-getNetworkInfo)
        * [openNetwork](#External-Functions-openNetwork)
        * [closeNetwork](#External-Functions-closeNetwork)
        * [removeDevice](#External-Functions-removeDevice)
        * [hardSystemReset](#External-Functions-hardSystemReset)
        * [softSystemReset](#External-Functions-softSystemReset)
        * [getDeviceList](#External-Functions-getDeviceList)
        * [getBindList](#External-Functions-getBindList)
* [webserver.js](#webserver-js)
    * [Incoming Events - Zigbee Gateway](#webserver-incoming-events-zb-gw)
        * [zb-gateway:newDev](#zb-gateway-newDev)
        * [zb-gateway:deviceList](#zb-gateway-deviceList)
        * [zb-gateway:removeDeviceCard](#zb-gateway-removeDeviceCard)
        * [zb-gateway:network:ready](#zb-gateway-network-ready)
        * [zb-gateway:network:info](#zb-gateway-network-info)
        * [zb-gateway:binding:event](#zb-gateway-binding-event)
        * [zb-gateway:binding:failed](#zb-gateway-binding-failed)
        * [zb-gateway:unbinding:failed](#zb-gateway-unbinding-failed)
        * [zb-gateway:light_device:state](#zb-gateway-light_device-state)
        * [zb-gateway:light_device:color](#zb-gateway-newDev-light_device-color)
        * [zb-gateway:temp_device:temp](#zb-gateway-temp_device-temp)
        * [zb-gateway:doorlock_device:state](#zb-gateway-doorlock_device-state)
        * [zb-gateway:doorlock_device:set_rsp](#zb-gateway-doorlock_device:set_rsp)
        * [zb-gateway:thermostat:attribute](#zb-gateway-thermostat-attribute)
    * [Incoming Events - Web Application](#webserver-incoming-events-web-app)
    * [Local Functions](#webserver-local-functions)
        * [getIPAddress](#webserver-getIPAddress)
        * [EVT](#webserver-EVT)
* [hagateway.js](#hagateway-js)
    * [Incoming Protobuf Messages](#hagw-incoming-protobuf-messages)
        * [ZIGBEE_GENERIC_CNF](#hagw-pb-ZIGBEE_GENERIC_CNF)
        * [ZIGBEE_GENERIC_RSP_IND](#hagw-pb-ZIGBEE_GENERIC_RSP_IND)
        * [GW_ZCL_FRAME_RECEIVE_IND](#hagw-pb-GW_ZCL_FRAME_RECEIVE_IND)
        * [GW_SET_ATTRIBUTE_REPORTING_RSP_IND](#hagw-pb-GW_SET_ATTRIBUTE_REPORTING_RSP_IND)
        * [DEV_GET_ONOFF_STATE_RSP_INDDEV_GET_LEVEL_RSP_IND](#hagw-pb-DEV_GET_ONOFF_STATE_RSP_IND)
        * [DEV_GET_LEVEL_RSP_IND](#hagw-pb-DEV_GET_LEVEL_RSP_IND)
        * [DEV_GET_COLOR_RSP_IND](#hagw-pb-DEV_GET_COLOR_RSP_IND)
        * [DEV_GET_TEMP_RSP_IND](#hagw-pb-DEV_GET_TEMP_RSP_IND)
        * [GW_ATTRIBUTE_REPORTING_IND](#hagw-pb-GW_ATTRIBUTE_REPORTING_IND)
        * [DEV_GET_DOOR_LOCK_STATE_RSP_IND](#hagw-pb-DEV_GET_DOOR_LOCK_STATE_RSP_IND)
        * [DEV_SET_DOOR_LOCK_RSP_IND](#hagw-pb-DEV_SET_DOOR_LOCK_RSP_IND)
        * [GW_READ_DEVICE_ATTRIBUTE_RSP_IND](#hagw-pb-GW_READ_DEVICE_ATTRIBUTE_RSP_IND)
        * [GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND](#hagw-pb-GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND)
    * [Local Functions](#hagw-local-functions)
        * [clientReconnect](#hagw-local-functions-clientReconnect)
        * [sendMessage](#hagw-local-functions-sendMessage)
        * [queTxMessage](#hagw-local-functions-queTxMessage)
        * [txNextMessage](#hagw-local-functions-txNextMessage)
        * [getRspHndlr](#hagw-local-functions-getRspHndlr)
        * [putRspHndlr](#hagw-local-functions-putRspHndlr)
        * [timeoutRspHndlr](#hagw-local-functions-timeoutRspHndlr)
    * [Public Functions - Light Device](#hagw-public-functions-light-dev)
        * [setState](#hagw-public-functions-light-dev-setState)
        * [getState](#hagw-public-functions-light-dev-getState)
        * [setLevel](#hagw-public-functions-light-dev-setLevel)
        * [getLevel](#hagw-public-functions-light-dev-getLevel)
        * [setColor](#hagw-public-functions-light-dev-setColor)
        * [getColor](#hagw-public-functions-light-dev-getColor)
    * [Public Functions - Thermostat Device](#hagw-public-functions-thermostat-dev)
        * [setpointChange](#hagw-public-functions-thermostat-dev-setpointChange)
        * [getTemp](#hagw-public-functions-thermostat-dev-getTemp)
    * [Public Functions - Door Lock Device](#hagw-public-functions-door-lock-dev)
        * [getDoorlockState](#hagw-public-functions-door-lock-dev-public-functions-getDoorlockState)
        * [setDoorlockState](#hagw-public-functions-door-lock-dev-public-functions-setDoorlockState)
    * [Public Functions - Generic Devices](#hagw-public-functions-generic-dev)
        * [setAttrReport](#hagw-public-functions-generic-dev-setAttrReport)
        * [readDeviceAttribute](#hagw-public-functions-generic-dev-readDeviceAttribute)
        * [writeDeviceAttribute_uint16](#hagw-public-functions-generic-dev-writeDeviceAttribute_uint16)
        * [reconnectHagateway](#hagw-public-functions-generic-dev-reconnectHagateway)
* [nwkmgr.js](#nwkmgr-js)
    * [Incoming protobuf Messages](#nwkmgr-incoming-protobuf-messages)
        * [ZIGBEE_GENERIC_CNF](#nwkmgr-incoming-protobuf-messages-ZIGBEE_GENERIC_CNF)
        * [ZIGBEE_GENERIC_RSP_IND](#nwkmgr-incoming-protobuf-messages-ZIGBEE_GENERIC_RSP_IND)
        * [NWK_SET_BINDING_ENTRY_RSP_IND](#nwkmgr-incoming-protobuf-messages-NWK_SET_BINDING_ENTRY_RSP_IND)
        * [NWK_GET_DEVICE_LIST_CNF](#nwkmgr-incoming-protobuf-messages-NWK_GET_DEVICE_LIST_CNF)
        * [NWK_ZIGBEE_DEVICE_IND](#nwkmgr-incoming-protobuf-messages-NWK_ZIGBEE_DEVICE_IND)
        * [NWK_GET_LOCAL_DEVICE_INFO_CNF](#nwkmgr-incoming-protobuf-messages-NWK_GET_LOCAL_DEVICE_INFO_CNF)
        * [NWK_ZIGBEE_NWK_READY_IND](#nwkmgr-incoming-protobuf-messages-NWK_ZIGBEE_NWK_READY_IND)
        * [NWK_ZIGBEE_NWK_INFO_CNF](#nwkmgr-incoming-protobuf-messages-NWK_ZIGBEE_NWK_INFO_CNF)
    * [Local Functions](#nwkmgr-local-functions)
        * [clientReconnect](#nwkmgr-local-functions-clientReconnect)
        * [sendMessage](#nwkmgr-local-functions-sendMessage)
        * [queTxMessage](#nwkmgr-local-functions-queTxMessage)
        * [txNextMessage](#nwkmgr-local-functions-txNextMessage)
        * [getRspHndlr](#nwkmgr-local-functions-getRspHndlr)
        * [putRspHndlr](#nwkmgr-local-functions-putRspHndlr)
        * [timeoutRspHndlr](#nwkmgr-local-functions-timeoutRspHndlr)
        * [getGwInfoReq](#nwkmgr-local-functions-getGwInfoReq)
    * [Public Functions](#nwkmgr-public-functions)
        * [getNetworkInfo](#nwkmgr-public-functions-getNetworkInfo)
        * [openNetwork](#nwkmgr-public-functions-openNetwork)
        * [getDevList](#nwkmgr-public-functions-getDevList)
        * [setDevBinding](#nwkmgr-public-functions-setDevBinding)
        * [removeDev](#nwkmgr-public-functions-removeDev)
        * [getGwInfo](#nwkmgr-public-functions-getGwInfo)
        * [hardSystemReset](#nwkmgr-public-functions-hardSystemReset)
        * [softSystemReset](#nwkmgr-public-functions-softSystemReset)
        * [reconnectNwkmgr](#nwkmgr-public-functions-reconnectNwkmgr)
* [ibmCloudAdapter.js](#ibm-cloud-adapter-js)
    * [Incoming Events - Zigbee Gateway](#ibmcloudadapter-incoming-events-zb-gw)
        * [zb-gateway:newDev](#ibmcloudadapter-incoming-events-zb-gw-newDev)
        * [zb-gateway:deviceList](#ibmcloudadapter-incoming-events-zb-gw-deviceList)
        * [zb-gateway:removeDeviceCard](#ibmcloudadapter-incoming-events-zb-gw-removeDeviceCard)
        * [zb-gateway:network:ready](#ibmcloudadapter-incoming-events-zb-gw-network-ready)
        * [zb-gateway:network:info](#ibmcloudadapter-incoming-events-zb-gw-network-info)
        * [zb-gateway:binding:event](#ibmcloudadapter-incoming-events-zb-gw-binding-event)
        * [zb-gateway:binding:failed](#ibmcloudadapter-incoming-events-zb-gw-binding-failed)
        * [zb-gateway:unbinding:failed](#ibmcloudadapter-incoming-events-zb-gw-unbinding-failed)
        * [zb-gateway:light_device:state](#ibmcloudadapter-incoming-events-zb-gw-light_device-state)
        * [zb-gateway:light_device:level](#ibmcloudadapter-incoming-events-zb-gw-light_device-level)
        * [zb-gateway:light_device:color](#ibmcloudadapter-incoming-events-zb-gw-light_device-color)
        * [zb-gateway:temp_device:temp](#ibmcloudadapter-incoming-events-zb-gw-temp_device-temp)
        * [zb-gateway:doorlock_device:state](#ibmcloudadapter-incoming-events-zb-gw-doorlock_device:state)
        * [zb-gateway:doorlock_device:set_rsp](#ibmcloudadapter-incoming-events-zb-gw-doorlock_device-set_rsp)
        * [zb-gateway:thermostat:attribute](#ibmcloudadapter-incoming-events-zb-gw-thermostat-attribute)
    * [Local Functions](#ibmcloudadapter-local-functions)
        * [EVT](#ibmcloudadapter-local-functions-EVT)
        * [bufferPayloadToObject](#ibmcloudadapter-local-functions-bufferPayloadToObject)

# <a name="zb-gatway-js"></a>
### zb-gateway.js  
The *zb-gateway* is the center JavaScript module that is responsible for communicating between 
the network managers and home automation gateways and the web applications. 
It is capable of handling incoming events and setting up outgoing commands properly. 
This portion of the guide will go over all of the components and basic operation of *zb-gateway.js*.

* [Incoming Events - Network Manager](#zb-gateway-incoming-events-nwkmgr)
* [Incoming Events - Home Automation Gateway](#zb-gateway-incoming-events-hagw)
* [Local Functions](#zb-gateway-local-functions)
* [Device Functions](#zb-gateway-device-functions)
* [External Functions](#zb-gateway-external-functions)

# <a name="zb-gateway-incoming-events-nwkmgr"></a>
#### Incoming Events - Network Manager  
This section describes how all incoming commands/events from the network manager (nwkmgr.js) is handled.

* [nwkmgr:newDev](#nwkmgr-newDev)
* [nwkmgr:removeDev](#nwkmgr-removeDev)
* [nwkmgr:network:ready](#nwkmgr-network-ready)
* [nwkmgr:binding:rsp](#nwkmgr-binding-rsp)

# <a name="nwkmgr-newDev"></a>
**nwkmgr:newDev**  
Handles when a new device is added to the network. Checks to see if the device has not already been added to the Device Array. 
If it has already been added, return from function. 
If it has not been added yet, add the device to the Device Array, check if a possible binding pair can be created, 
and then emit a *zb-gateway:newDev* event. 
With this event, send the updated device array along with the "binded" and "unbinded" device arrays.

# <a name="nwkmgr-removeDev"></a>
**nwkmgr:removeDev**  
Handles when a device has been removed from the network. 
This function call takes the IEEE address as an input, and matches it with a device in the Device Array. 
Once the device is found in the Device array, it is removed from the Array.

# <a name="nwkmgr-network-ready"></a>
**nwkmgr:network:ready** 
When the Network Manager receives a network ready incoming command, 
this event is forwarded by the *zb-gateway* to any listening modules using the *zb-gateway:network:ready event*.

# <a name="nwkmgr-binding-rsp"></a>
**nwkmgr:binding:rsp**  
When the network manager receives a binding response the *zb-gateway* will determine if the bind is successful. 
In the case that the bind is successful, the GUID, endpoint and ID of the devices are constructed to the appropriate format. 
Using the GUID's, the binding list is updated. 
If the two GUID's are not in the bound device list, then they are added. 
If the pair is already in the bounded device list, then they are removed and then added to the unbounded device list. 
The *zb-gateway* will then emit the *zb-gateway:binding:event* event along with the updated device lists.

If the binding response is received as being unsuccessful, then the *zb-gateway* emits the *zb-gateway:binding:failed* event.

# <a name="zb-gateway-incoming-events-hagw"></a>
#### Incoming Events - Home Automation Gateway  
This section describes how all incoming commands/events from the home automation gateway (hagateway.js) is handled.

* [hagateway:light_device:state](#hagateway-light_device-state)
* [hagateway:light_device:level](#hagateway-light_device-level)
* [hagateway:light_device:color](#hagateway-light_device-color)
* [hagateway:temp_device:temp](#hagateway-temp_device-temp)
* [hagateway:doorlock_device:state](#hagateway-doorlock_device-state)
* [hagateway:doorlock_device:set_rsp](#hagateway-doorlock_device-set_rsp)
* [hagateway:attribute:read:rsp](#hagateway-attribute-read-rsp)
* [hagateway:attribute:write:rsp](#hagateway-attribute-write-rsp)
* [hagateway:gw:report](#hagateway-gw-report)
* [hagateway:network:connected](#hagateway-network-connected)

# <a name="hagateway-light_device-state"></a>
**hagateway:light_device:state**  
Handles a change in state of a light device. 
The *zb-gateway* simply forwards the relevant data by emitting the *zb-gateway:light_device:state* event.

# <a name="hagateway-light_device-level"></a>
**hagateway:light_device:level**  
Handles a change in level of a light device. 
The *zb-gateway* simply forwards the relevant data by emitting the *zb-gateway:light_device:level event*.

# <a name="hagateway-light_device-color"></a>
**hagateway:light_device:color**  
Handles a change in color of a light device. 
The *zb-gateway*simply forwards the relevant data by emitting the *zb-gateway:light_device:color* event.

# <a name="hagateway-temp_device-temp"></a>
**hagateway:temp_device:temp**  
Handles a change in temperature of a temperature device. 
The *zb-gateway* simply forwards the relevant data by emitting the *zb-gateway:temp_device:temp event*.

# <a name="hagateway-doorlock_device-state"></a>
**hagateway:doorlock_device:state**  
Handles a change in state of a door lock device. 
The *zb-gateway* simply forwards the relevant data by emitting the *zb-gateway:doorlock_device:state* event.

# <a name="hagateway-doorlock_device-set_rsp"></a>
**hagateway:doorlock_device:set_rsp**  
Handles a Set Response from a door lock device. 
The *zb-gateway* simply forwards the relevant data by emitting the *zb-gateway:doorlock_device:set_rsp* event.

# <a name="hagateway-attribute-read-rsp"></a>
**hagateway:attribute:read:rsp**  
Handles Attribute Read Response from certain devices. 
This will determine what the device type is by using the incoming GUID to return the device type from the device array. 
Based on the device type, it will extract the attributes as necessary and send an event ending with "*...:attribute*" and will send the extracted data along with it.

Ex). Given a Thermostat Device Type, it will send the *zb-gateway:thermostat:attribute* event along with the attribute values.

# <a name="hagateway-attribute-write-rsp"></a>
**hagateway:attribute:write:rsp**  
Handles Attribute Write Responses. 
Will first determine if the status of the response was successful or failed. 
If the Attribute Write was successful, the function will loop through all loop attribute responses that were received and send 
an event that is dynamically created based on the attribute cluster ID and attribute status.

If the Attribute Write failed, then the *zb-gateway* emits a *zb-gateway:attribute:write:failed* event along with the corresponding response data.

# <a name="hagateway-gw-report"></a>
**hagateway:gw:report**  
Handles all incoming reports from various end devices. 
Based on the cluster ID of the incoming report, the data will be extracted or converted as necessary. 
Based on the cluster ID or data type, the *zb-gateway* will then emit an event that corresponds to the data type.

Ex). Incoming Report from Temperature Sensor Device: If the temperature attribute ID is present, 
convert the temperature data from a buffer to an integer value.
 Then emit the event *zb-gateway:temp_device:temp* from the *zb-gateway*.

# <a name="hagateway-network-connected"></a>
**hagateway:network:connected**  
Handles when the Home Automation Gateway is connected to the Linux Gateway. 
This event is forwarded by the *zb-gateway* to any listening modules using the *zb-gateway:network:ready* event.

# <a name="zb-gateway-local-functions"></a>
#### Local Functions

* [zb_gateway_getNetworkInfo](#zb_gateway_getNetworkInfo)
* [zb_gateway_openNetwork](#zb_gateway_openNetwork)
* [zb_gateway_closeNetwork](#zb_gateway_closeNetwork)
* [zb_gateway_hardSystemReset](#zb_gateway_hardSystemReset)
* [zb_gateway_softSystemReset](#zb_gateway_softSystemReset)
* [zb_gateway_getDeviceList](#zb_gateway_getDeviceList)
* [zb_gateway_getBindList](#zb_gateway_getBindList)
* [zb_gateway_removeDevice](#zb_gateway_removeDevice)
* [checkPossibleBinding](#zb-gateway-checkPossibleBinding)
* [removeBindingItembyGuid](#zb-gateway-removeBindingItembyGuid)
* [updateBindingList](#zb-gateway-updateBindingList)
* [checkGUIDinDevArray](#zb-gateway-checkGUIDinDevArray)
* [EVT](#zb-gateway-EVT)

# <a name="zb_gateway_getNetworkInfo"></a>
**zb_gateway_getNetworkInfo**  
Calls the Network Manager (*nwkmgr.js*) to request the latest information regarding the network. 
When the information is ready, the Network Manager will emit an event with the network channel and PanID.  

* **Input**  
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb_gateway_openNetwork"></a>
**zb_gateway_openNetwork**  
Calls the Network Manager (*nwkmgr.js*) to open the network for 180 seconds. This will allow new devices to join the network.

* **Input**  
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb_gateway_closeNetwork"></a>
**zb_gateway_closeNetwork**  
Calls the Network Manager (nwkmgr.js) to close the network. This will prohibit new devices from joining the network.  

* **Input**  
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb_gateway_hardSystemReset"></a>
**zb_gateway_hardSystemReset**  
Will clear all device lists from the zb-gateway and then call the Network Manager (*nwkmgr.js*) to perform a hard system reset. 
This will restart the Linux Gateway, deleting all devices from the network and restart the ZNP device, clearing the NV flash memory.

* **Input**  
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb_gateway_softSystemReset"></a>
**zb_gateway_softSystemReset**  
Will call the Network Manager (*nwkmgr.js*) to perform a soft system reset. 
This will restart the Linux Gateway and restart the ZNP device. All devices will remain commissioned to the network.

* **Input**  
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb_gateway_getDeviceList"></a>
**zb_gateway_getDeviceList**  
Emits an event (*zb-gateway:deviceList*) from the Zigbee Gateway to either the Webserver or the CloudAdapter, with 
the latest list of devices that are on the network. 
This will include a list of all devices on the network, a list of device pairs that can be bound, and a list of device pairs that are currently bound.

* **Input**  
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb_gateway_getBindList"></a>
**zb_gateway_getBindList**  
Emits an event (*zb-gateway:binding:event*) from the Zigbee Gateway to either the Webserver or the CloudAdapter, with the latest list of devices that are on the network. This will include a list of all devices on the network, a list of device pairs that can be bound, and a list of device pairs that are currently bound. 

* **Input**  
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb_gateway_removeDevice"></a>
**zb_gateway_removeDevice**  
Responsible for removing a specific device from the network. 
Will use the IEEE address to determine which device to remove. 
Will delete the device from the Device Array and will call the Network Manager to remove the device from the network. 
Once the device is removed, it will also be removed from any binding lists if applicable.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object
            - ieee (Buffer)
            - guid (String)
    * *Description*
        - JS object that consists of the following properties
            - ieee
            - guid
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb-gateway-checkPossibleBinding"></a>
**checkPossibleBinding**
Will use the current Device Array to check for any device pairs that may be able to be bound together. 
Will first go through all devices and determine what Device Type they are. 
If the Device Type could be bound to another device, it is added to a specific device list array. 
Using these device arrays, all combinations of possible bind pairs are reported in an *unboundDeviceArray[]*. 
Each element of the array will consist of the Source Device GUID and the Destination Device GUID.

* **Input**  
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb-gateway-removeBindingItembyGuid"></a>
**removeBindingItembyGuid**  
Remove a specific device from the all necessary binding arrays, based on the GUID of the device. 
This includes removing the device from both the unbound and bound device arrays. 
This function is usually called when the device is removed from the network.

* **Input**  
    * *Parameter*
        - GUID
    * *Type*
        - String
    * *Description*
        - Pass the GUID of the device that is to be removed from the binding lists.
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb-gateway-updateBindingList"></a>
**updateBindingList**  
Update the binding arrays to include which device pairs are bound and unbound. 
Based on the device pair argument, this function will toggle which binding list the pair is present in. 
If the pair is present in the unbound list, it will be remove from the unbound list and added to the bound list. 
If the pair is present in the bound list, it will be removed from the bound list and added to the unbound list.

* **Input**  
    * *Parameter*
        - sourceGUID
        - destGUID
    * *Type*
        - String
    * *Description*
        - Pass the GUID of the source device and the GUID of the destination device that have just received a binding event.
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb-gateway-checkGUIDinDevArray"></a>
**checkGUIDinDevArray**  
Determine if a GUID can be found in the Device Array. Used to determine if a device is currently present in the Device Array.

* **Input**  
    * *Parameter*
        - GUID
    * *Type*
        - String
    * *Description*
        - Pass the GUID of the device that is to be removed from the binding lists.
* **Output**
    * *Parameter*
        - Status
    * *Type*
        - Bool
    * *Description*
        - True = Device GUID is present in Device Array
        - False = Device GUID is not present in Device Array

# <a name="zb-gateway-EVT"></a>
**EVT**  
Generates a string of the input string arguments separated by colons. 
This is the same format used to define events between modules in the gateway. 
This is typically used to create event strings based on variable types. 
The function can use a variable number of input arguments.

Ex): EVT(arg1, arg2, arg3) --> returns string "arg1:arg2:arg3"

* **Input**  
    * *Parameter*
        - Evt1, Evt2, ...
    * *Type*
        - String
    * *Description*
        - A variable number of string arguments.
* **Output**
    * *Parameter*
        - Event_string
    * *Type*
        - String
    * *Description*
        - A string of all argument inputs separated by colons.

# <a name="zb-gateway-device-functions"></a>
#### Device Functions  
The Zigbee Gateway is responsible for handling a set of device specific functions for each compatible device type. 
These functions make handling specific device commands or data very simple for the either the webserver or cloud adapter.

* [OnOffLight](#zb-gateway-OnOffLight)
* [DimmableLight](#zb-gateway-DimmableLight)
* [ColorLight](#zb-gateway-ColorLight)
* [TempSensor](#zb-gateway-TempSensor)
* [Switch Device](#zb-gateway-SwitchDevice)
* [DoorLockDevice](#zb-gateway-DoorLockDevice)
* [ThermostatDevice](#zb-gateway-ThermostatDevice)
* [Device Binding](#zb-gateway-Device-Binding)

# <a name="zb-gateway-OnOffLight"></a>
##### OnOffLight

* [set_ONOFF_state](#OnOffLight-set_ONOFF_state)
* [get_ONOFF_state](#OnOffLight-get_ONOFF_state)

# <a name="OnOffLight-set_ONOFF_state"></a>
**set_ONOFF_state**  
Will send either an ON or OFF command to a light device. 
Input argument must be a JavaScript object containing an "on", "ieee" and "ep" property. 
The "on" property will determine the state to set the light to. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - on (INT)
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - on = "0" or "1" defining the state to set the light to
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="OnOffLight-get_ONOFF_state"></a>
**get_ONOFF_state**  
Send a request to a particular light device to get the current state of the light. 
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="zb-gateway-DimmableLight"></a>
##### DimmableLight

* [set_ONOFF_state](#DimmableLight-set_ONOFF_state)
* [get_ONOFF_state](#DimmableLight-get_ONOFF_state)
* [get_level](#DimmableLight-get_level)
* [set_level_transition](#DimmableLight-set_level_transition)
* [set_onoff_transition](#DimmableLight-set_onoff_transition)

# <a name="DimmableLight-set_ONOFF_state"></a>
**set_ONOFF_state**  
Will send either an ON or OFF command to a light device. 
Input argument must be a JavaScript object containing an "on", "ieee" and "ep" property. 
The "on" property will determine the state to set the light to. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object
            - on (INT)
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - on = "0" or "1" defining the state to set the light to
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
* **Output**
    * *Parameter*
        * NULL
    * *Type*
        * NULL
    * *Description*
        * NULL

# <a name="DimmableLight-get_ONOFF_state"></a>
**get_ONOFF_state**  
Send a request to a particular light device to get the current state of the light. 
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="DimmableLight-get_level"></a>
**get_level**  
Send a request to get the current level of the brightness of a dimmable light device. 
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="DimmableLight-set_level_transition"></a>
**set_level_transition**  
Set the level and transition time a dimmable light takes to change to a new level of brightness. 
Input argument must be a JavaScript object containing an "ieee", "ep", "level" and "transitionTime" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The "level" property defines the level of brightness to set the light device to. 
The "transitionTime" property defines the amount of time in milliseconds to the light will take to transition to the new brightness level. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
            - level (INT)
            - transitionTime (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
        - level = level of brightness
        - transitionTime = time to transition to new brightness level
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="DimmableLight-set_onoff_transition"></a>
**set_onoff_transition**  
Set the level and transition time a dimmable light takes to turn on or off. 
Input argument must be a JavaScript object containing an "ieee", "ep" and "transitionTime" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The "transitionTime" property defines the amount of time in milliseconds to the light will take to transition to the new brightness level. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
            - transitionTime (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
        - transitionTime = time to transition to new brightness level
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="zb-gateway-ColorLight"></a>
##### ColorLight

* [set_ONOFF_state](#ColorLight-set_ONOFF_state)
* [get_ONOFF_state](#ColorLight-get_ONOFF_state)
* [set_level_transition](#ColorLight-set_level_transition)
* [set_onoff_transition](#ColorLight-set_onoff_transition)
* [set_color](#ColorLight-set_color)
* [get_color](#ColorLight-get_color)

# <a name="ColorLight-set_ONOFF_state"></a>
**set_ONOFF_state**  
Will send either an ON or OFF command to a light device. 
Input argument must be a JavaScript object containing an "on", "ieee" and "ep" property. 
The "on" property will determine the state to set the light to. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - on (INT)
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - on = "0" or "1" defining the state to set the light to
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="ColorLight-get_ONOFF_state"></a>
**get_ONOFF_state**  
Send a request to a particular light device to get the current state of the light. 
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="ColorLight-set_level_transition"></a>
**set_level_transition**  
Set the level and transition time a dimmable light takes to change to a new level of brightness. 
Input argument must be a JavaScript object containing an "ieee", "ep", "level" and "transitionTime" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The "level" property defines the level of brightness to set the light device to. 
The "transitionTime" property defines the amount of time in milliseconds to the light will take to transition to the new brightness level. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
            - level (INT)
            - transitionTime (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
        - level = level of brightness
        - transitionTime = time to transition to new brightness level
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="ColorLight-set_onoff_transition"></a>
**set_onoff_transition**  
Set the level and transition time a dimmable light takes to turn on or off. 
Input argument must be a JavaScript object containing an "ieee", "ep" and "transitionTime" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The "transitionTime" property defines the amount of time in milliseconds to the light will take to transition to the new brightness level. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
            - transitionTime (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
        - transitionTime = time to transition to new brightness level
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="ColorLight-set_color"></a>
**set_color**  
Set the color (hue and saturation) of a color light device. 
Input argument must be a JavaScript object containing an "ieee", "ep", "hue" and "saturation" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The "hue" property is the hue value to send to the color light device. 
The "saturation" property is the amount of saturation to apply to the hue color. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
            - hue (INT)
            - saturation (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
        - hue = hue level to set color light
        - saturation = saturation level to set color light
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="ColorLight-get_color"></a>
**get_color**  
Send a request to get the current color data associated with the color light. 
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the light device application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="zb-gateway-TempSensor"></a>
##### TempSensor

* [get_temp](#TempSensor-get_temp)
* [set_interval](#TempSensor-set_interval)

# <a name="TempSensor-get_temp"></a>
**get_temp**  
Send a request to get the current temperature reading from a temperature sensor device. 
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the temperature sensor application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the temperature application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="TempSensor-set_interval"></a>
**set_interval**  
Send a command to the temperature device to update the reporting interval of temperature data. 
Input argument must be a JavaScript object containing an "ieee", "ep", "clusterId" and "attrList" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the temperature sensor application running on the device. 
The "clusterId" property defines what cluster ID will be targeted. The "attrList" property defines what attributes to update. 
Must contain the "minReportInterval". T
he function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
            - clusterID (INT)
            - attrList (JS obj)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the light device application
        - clusterID = the cluster number being targeted
        - attrList = the attribute list in object form
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="zb-gateway-SwitchDevice"></a>
##### SwitchDevice

* [get_state](#SwitchDevice-get_state)

**get_state**  
Send a request to get the current state of a switch device. 
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the switch application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the switch application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="zb-gateway-DoorLockDevice"></a>
##### DoorLockDevice

* [get_state](#DoorLockDevice-get_state)
* [unlock](#DoorLockDevice-unlock)
* [lock](#DoorLockDevice-lock)

# <a name="DoorLockDevice-get_state"></a>
**get_state**  
Send a request to get the current state of the door lock device. 
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the door lock application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the door lock application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="DoorLockDevice-unlock"></a>
**unlock**  
Send a command to unlock the door lock device. 
Input argument must be a JavaScript object containing an "ieee", "ep" and "pin" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the door lock application running on the device. 
The "pin" property defines the security pin code used to authenticate that the door lock can be unlocked. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
            - pin (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the door lock application
        - pin = the pin code used to authenticate the door lock
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="DoorLockDevice-lock"></a>
**lock**  
Send a command to lock the door lock device. 
Input argument must be a JavaScript object containing an "ieee", "ep" and "pin" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the door lock application running on the device. 
The "pin" property defines the security pin code used to authenticate that the door lock can be locked. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
            - pin (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command too
        - ep = endpoint of the door lock application
        - pin = the pin code used to authenticate the door lock
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="zb-gateway-ThermostatDevice"></a>
##### ThermostatDevice

* [setpoint_change](#ThermostatDevice-setpoint_change)
* [get_minheat_setpoint](#ThermostatDevice-get_minheat_setpoint)
* [get_all_attributes](#ThermostatDevice-get_all_attributes)

# <a name="ThermostatDevice-setpoint_change"></a>
**setpoint_change**  
Send a command to update the current set point of a thermostat device. 
Input argument must be a JavaScript object containing an "ieee", "ep", "mode" and "amount" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the door lock application running on the device. 
The "mode" property defines what mode to set the thermostat to. 
The "amount" property will define the increment amount to adjust the current set point of the thermostat by. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
            - mode (INT)
            - amount (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command to
        - ep = endpoint of the door lock application
        - mode = the mode to set the thermostat
        - amount = amount to increment or decrement the current setpoint
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="ThermostatDevice-get_minheat_setpoint"></a>
**get_minheat_setpoint**  
Send a request to get the minimum heat set point of a thermostat device. 
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the thermostat application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command too
        - ep = endpoint of the thermostat application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="ThermostatDevice-get_all_attributes"></a>
**get_all_attributes**  
Send a request to an array of attributes from a thermostat device. 
Will currently request to read the attributes of the local temperature, cooling demand, heating demand, 
occupied cooling set point, occupied heating set point, minimum heating set point limit, maximum heating set point limit, 
minimum cooling set point limit, maximum set point limit, maximum cooling set point limit, control sequence of operation and system mode.
Input argument must be a JavaScript object containing an "ieee" and "ep" property. 
The "ieee" property is the IEEE address of the device the command should be sent to. 
The "ep" property is the endpoint of the thermostat application running on the device. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - ieee (BUFFER)
            - ep (INT)
    * *Description*
        - ieee = the IEEE address of the device to send command too
        - ep = endpoint of the thermostat application
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="zb-gateway-Device-Binding"></a>
##### Device Binding

* [set_binding](#Device-Binding-set_binding)
* [set_unbinding](#Device-Binding-set_unbinding)

# <a name="Device-Binding-set_binding"></a>
**set_binding**  
This function will request a bind between a pair of devices. 
This will be based on a source device and a destination device. 
The source device will initiate the bind to a destination device and maintain a local bind table. 
Input argument must be a JavaScript object containing an 
"source_ieee", "source_ep", "dest_ieee", "dest_ieee", "dest_ep" and "clusterId" property. 
The "source_ieee" property is the IEEE address of the source device the command should be sent to. 
The "source_ep" property is the endpoint of the application running on the source device. 
The "dest_ieee" property is the IEEE address of the destination device the command should be sent to. 
The "dest_ep" property is the endpoint of the application running on the destination device. The "clusterId" property is the cluster ID of the devices that should be paired. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - source_ieee (BUFFER)
            - source_ep (INT)
            - dest_ieee (BUFFER)
            - dest_ep (INT)
            - clusterId (INT)
    * *Description*
        - source_ieee = the IEEE address of the source device to bind
        - source_ep = end point of the device application to bind
        - dest_ieee = the IEEE address of the destination device to bind
        - dest_ep = the end point of the device application to bind
        - clusterId = the cluster ID the bind is referring to
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="Device-Binding-set_unbinding"></a>
**set_unbinding**  
This function will request an unbind between a pair of devices. 
This will be based on a source device and a destination device. 
The source device will initiate the unbind to a destination device and remove it from a local bind table. 
Input argument must be a JavaScript object containing an 
"source_ieee", "source_ep", "dest_ieee", "dest_ieee", "dest_ep" and "clusterId" property. 
The "source_ieee" property is the IEEE address of the source device the command should be sent to. 
The "source_ep" property is the endpoint of the application running on the source device. 
The "dest_ieee" property is the IEEE address of the destination device the command should be sent to. 
The "dest_ep" property is the endpoint of the application running on the destination device. 
The "clusterId" property is the cluster ID of the devices that should be paired. 
The function will wait for a callback from the home automation gateway to ensure the request was sent successfully.

* **Input**  
    * *Parameter*
        - data
    * *Type*
        - JS Object:
            - source_ieee (BUFFER)
            - source_ep (INT)
            - dest_ieee (BUFFER)
            - dest_ep (INT)
            - clusterId (INT)
    * *Description*
        - source_ieee = the IEEE address of the source device to unbind
        - source_ep = end point of the device application to unbind
        - dest_ieee = the IEEE address of the destination device to unbind
        - dest_ep = the end point of the device application to unbind
        - clusterId = the cluster ID the bind is referring to
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="zb-gateway-external-functions"></a>
#### External Functions  
These are functions that can be referenced externally by other modules that are calling functions defined in "zb_gateway".

* [getNetworkInfo](#External-Functions-getNetworkInfo)
* [openNetwork](#External-Functions-openNetwork)
* [closeNetwork](#External-Functions-closeNetwork)
* [removeDevice](#External-Functions-removeDevice)
* [hardSystemReset](#External-Functions-hardSystemReset)
* [softSystemReset](#External-Functions-softSystemReset)
* [getDeviceList](#External-Functions-getDeviceList)
* [getBindList](#External-Functions-getBindList)

# <a name="External-Functions-getNetworkInfo"></a>
**getNetworkInfo**  
Calls the "zb_gateway_getNetworkInfo()" function. 
Sends a request to get the current information regarding the network.

# <a name="External-Functions-openNetwork"></a>
**openNetwork**  
Calls the "zb_gateway_openNetwork()" function. 
This sends a command to open the Zigbee Network to allow devices to join the network.

# <a name="External-Functions-closeNetwork"></a>
**closeNetwork**  
Calls the "zb_gateway_closeNetwork()" function. 
This sends a command to close the Zigbee Network to prohibit devices from joining the network.

# <a name="External-Functions-removeDevice"></a>
**removeDevice**  
Calls the "zb_gateway_removeDevice()" function. 
This sends a command to remove a device from the network and remove the device from all table entries.

# <a name="External-Functions-hardSystemReset"></a>
**hardSystemReset**  
Calls the "zb_gateway_hardSystemReset()" function. 
This sends a command to the Linux Zigbee Gateway to perform a hard reset of the network and gateway applications.

# <a name="External-Functions-softSystemReset"></a>
**softSystemReset**  
Calls the "zb_gateway_softSystemReset()" function. 
This sends a command to the Linux Zigbee Gateway to perform a soft reset of the network and gateway applications.

# <a name="External-Functions-getDeviceList"></a>
**getDeviceList**  
Calls the "zb_gateway_getDeviceList" function. 
This will emit an event with the most updated device lists.

# <a name="External-Functions-getBindList"></a>
**getBindList**  
Calls the "zb_gateway_getBindList" function. 
This will emit an event with the most updated bind lists.

# <a name="webserver-js"></a>
### webserver.js

The "webserver" is a JavaScript module that is responsible for hosting a local web application. 
It also provides a means of communication between the Zigbee Gateway and the web applications. 
It is capable of handling incoming events and setting up outgoing commands properly. 
This portion of the guide will go over all of the components and basic operation of "webserver.js".

* [Incoming Events - Zigbee Gateway](#webserver-incoming-events-zb-gw)
* [Incoming Events - Web Application](#webserver-incoming-events-web-app)
* [Local Functions](#webserver-local-functions)

# <a name="webserver-incoming-events-zb-gw"></a>
#### Incoming Events - Zigbee Gateway

* [zb-gateway:newDev](#zb-gateway-newDev)
* [zb-gateway:deviceList](#zb-gateway-deviceList)
* [zb-gateway:removeDeviceCard](#zb-gateway-removeDeviceCard)
* [zb-gateway:network:ready](#zb-gateway-network-ready)
* [zb-gateway:network:info](#zb-gateway-network-info)
* [zb-gateway:binding:event](#zb-gateway-binding-event)
* [zb-gateway:binding:failed](#zb-gateway-binding-failed)
* [zb-gateway:unbinding:failed](#zb-gateway-unbinding-failed)
* [zb-gateway:light_device:state](#zb-gateway-light_device-state)
* [zb-gateway:light_device:color](#zb-gateway-newDev-light_device-color)
* [zb-gateway:temp_device:temp](#zb-gateway-temp_device-temp)
* [zb-gateway:doorlock_device:state](#zb-gateway-doorlock_device-state)
* [zb-gateway:doorlock_device:set_rsp](#zb-gateway-doorlock_device:set_rsp)
* [zb-gateway:thermostat:attribute](#zb-gateway-thermostat-attribute)

# <a name="zb-gateway-newDev"></a>
**zb-gateway:newDev**  
Event occurs when the Zigbee Gateway receives a new device on the network. 
When this event is received by the webserver, the lists will be packaged and a "devList" event will be emitted to the web application.

# <a name="zb-gateway-deviceList"></a>
**zb-gateway:deviceList**  
Event occurs when the Zigbee Gateway receives an updated list of new devices on the network. 
When this event is received by the webserver, the lists will be packaged and a "devList" event will be emitted to the web application.

# <a name="zb-gateway-removeDeviceCard"></a>
**zb-gateway:removeDeviceCard**  
Event occurs when the Zigbee Gateway emits that a device has been removed from the network. 
Therefore the device card needs to be removed from the web application view. 
When this event is received by the webserver, a *webserver:removeDeviceCard* event will be emitted to the web application.

# <a name="zb-gateway-network-ready"></a>
**zb-gateway:network:ready**  
Event occurs when the Zigbee Gateway emits an event confirming the Network and Linux Gateway are ready. 
When this event is received by the webserver, a webserver:network:ready event will be emitted to the web application.

# <a name="zb-gateway-network-info"></a>
**zb-gateway:network:info**  
Event occurs when the Zigbee Gateway is sending information regarding the network. 
When this event is received by the webserver, a webserver:network:info event will be emitted to the web application.

# <a name="zb-gateway-binding-event"></a>
**zb-gateway:binding:event**  
Event refers to when a binding event has occurred. 
This includes when a pair of devices is successfully bound or unbound. 
Will include an updated device list, bind list and unbound list. 
When this event is received by the webserver, 
the lists will be packaged and an *webserver:bindingDevList* event will be sent to the web application.

# <a name="zb-gateway-binding-failed"></a>
**zb-gateway:binding:failed**  
Event refers to when a binding event has failed. 
When this event is received by the webserver, a *webserver:binding:failed* event will be emitted to the web application.

# <a name="zb-gateway-unbinding-failed"></a>
**zb-gateway:unbinding:failed**  
Event refers to when an unbinding event has failed. 
When this event is received by the webserver, a *webserver:unbinding:failed* event will be emitted to the web application.

# <a name="zb-gateway-light_device-state"></a>
**zb-gateway:light_device:state**  
Event refers to when a light device state has changed. 
When this event is received by the webserver, a *webserver:light_device:state event* will be emitted to the web application.

# <a name="zb-gateway-newDev-light_device-color"></a>
**zb-gateway:light_device:color**  
Event refers to when a light device color has changed. 
When this event is received by the webserver, a *webserver:light_device:color* event will be emitted to the web application.

# <a name="zb-gateway-temp_device-temp"></a>
**zb-gateway:temp_device:temp**  
Event refers to when a temperature device temperature reading has changed. 
When this event is received by the webserver, a *webserver:temp_device:temp* event will be emitted to the web application.

# <a name="zb-gateway-doorlock_device-state"></a>
**zb-gateway:doorlock_device:state**  
Event refers to when a door lock device state has changed. 
When this event is received by the webserver, a *webserver:doorlock_device:state* event will be emitted to the web application.

# <a name="zb-gateway-doorlock_device:set_rsp"></a>
**zb-gateway:doorlock_device:set_rsp**  
Event refers to when a door lock device set response has been received by the gateway. 
When this event is received by the webserver, a *webserver:doorlock_device:set_rsp* event will be emitted to the web application.

# <a name="zb-gateway-thermostat-attribute"></a>
**zb-gateway:thermostat:attribute**  
Event refers to when a thermostat device attribute has been received by the gateway. 
When this event is received by the webserver, a *webserver:thermostat:attribute* event will be emitted to the web application.

# <a name="webserver-incoming-events-web-app"></a>
#### Incoming Events - Web Application

All of the incoming events received from the web application will call an appropriate Zigbee Gateway function. 
If necessary, data from the web application will be used as an input argument to the Zigbee Gateway functions.

# <a name="webserver-local-functions"></a>
#### Local Functions

* [getIPAddress](#webserver-getIPAddress)
* [EVT](#webserver-EVT)

# <a name="webserver-getIPAddress"></a>
**getIPAddress**  
Will return the IP address of the device the current Node.js application is running on. 
If no IP address is able to be found, it will return an IP address of "0.0.0.0".

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - IP Address
    * *Type*
        - String
    * *Description*
        - The IP address of the current device.

# <a name="webserver-EVT"></a>
**EVT**  
Generates a string of the input string arguments separated by colons. 
This is the same format used to define events between modules in the gateway. 
This is typically used to create event strings based on variable types. 
The function can accept a variable number of input arguments.

Ex): EVT(arg1, arg2, arg3) --> returns string "arg1:arg2:arg3"

* **Input**  
    * *Parameter*
        - Evt1, Evt2, ...
    * *Type*
        - String
    * *Description*
        - A variable number of string arguments.
* **Output**
    * *Parameter*
        - Event_string
    * *Type*
        - String
    * *Description*
        - A string of all argument inputs separated by colons.

# <a name="hagateway-js"></a>
### hagateway.js

The "hagateway" is a JavaScript module that is responsible handling incoming home automation gateway events from the Linux Zigbee Gateway. 
It also provides a means of communication between the Zigbee Gateway and Linux Zigbee Gateway.

* [Incoming Protobuf Messages](#hagw-incoming-protobuf-messages)
* [Local Functions](#hagw-local-functions)
* [Public Functions - Light Device](#hagw-public-functions-light-dev)
* [Public Functions - Thermostat Device](#hagw-public-functions-thermostat-dev)
* [Public Functions - Door Lock Device](#hagw-public-functions-door-lock-dev)
* [Public Functions - Generic Devices](#hagw-public-functions-generic-dev)

# <a name="hagw-incoming-protobuf-messages"></a>
#### Incoming Protobuf Messages

* [ZIGBEE_GENERIC_CNF](#hagw-pb-ZIGBEE_GENERIC_CNF)
* [ZIGBEE_GENERIC_RSP_IND](#hagw-pb-ZIGBEE_GENERIC_RSP_IND)
* [GW_ZCL_FRAME_RECEIVE_IND](#hagw-pb-GW_ZCL_FRAME_RECEIVE_IND)
* [GW_SET_ATTRIBUTE_REPORTING_RSP_IND](#hagw-pb-GW_SET_ATTRIBUTE_REPORTING_RSP_IND)
* [DEV_GET_ONOFF_STATE_RSP_INDDEV_GET_LEVEL_RSP_IND](#hagw-pb-DEV_GET_ONOFF_STATE_RSP_IND)
* [DEV_GET_LEVEL_RSP_IND](#hagw-pb-DEV_GET_LEVEL_RSP_IND)
* [DEV_GET_COLOR_RSP_IND](#hagw-pb-DEV_GET_COLOR_RSP_IND)
* [DEV_GET_TEMP_RSP_IND](#hagw-pb-DEV_GET_TEMP_RSP_IND)
* [GW_ATTRIBUTE_REPORTING_IND](#hagw-pb-GW_ATTRIBUTE_REPORTING_IND)
* [DEV_GET_DOOR_LOCK_STATE_RSP_IND](#hagw-pb-DEV_GET_DOOR_LOCK_STATE_RSP_IND)
* [DEV_SET_DOOR_LOCK_RSP_IND](#hagw-pb-DEV_SET_DOOR_LOCK_RSP_IND)
* [GW_READ_DEVICE_ATTRIBUTE_RSP_IND](#hagw-pb-GW_READ_DEVICE_ATTRIBUTE_RSP_IND)
* [GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND](#hagw-pb-GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND)

All incoming Protobuf messages from the Linux Zigbee Gateway are received as "data" events and are 
then handled in a "case" statement based on the command ID received. 
If an error has occurred, an "error" event will be emitted and received by the hagateway. 
When this occurs, the home automation gateway will attempt to reconnect to the Linux Zigbee Gateway client.

# <a name="hagw-pb-ZIGBEE_GENERIC_CNF"></a>
**ZIGBEE_GENERIC_CNF**  
This command ID describes a generic confirmation received by the Gateway. 
This is used to determine if a command was sent successfully.

# <a name="hagw-pb-ZIGBEE_GENERIC_RSP_IND"></a>
**ZIGBEE_GENERIC_RSP_IND**  
This command ID describes when a generic response is received by the Gateway. 
When it is received by the "hagateway", 
it will emit a "gen-rsp" event along with the status of the response and the IEEE address and end point the response was coming from.

# <a name="hagw-pb-GW_ZCL_FRAME_RECEIVE_IND"></a>
**GW_ZCL_FRAME_RECEIVE_IND**  
This command ID describes when a Zigbee Cluster library frame has been received. 
The incoming Protobuf message is decoded and the header and payload are packaged. 
All of the data is then sent as a *hagateway:zcl-ind* event.

# <a name="hagw-pb-GW_SET_ATTRIBUTE_REPORTING_RSP_IND"></a>
**GW_SET_ATTRIBUTE_REPORTING_RSP_IND**  
This command ID describes when a set attribute report response has been received.
The status of the response is packaged and a *hagateway:attrreport:rsp event* is emitted to the Zigbee Gateway.

# <a name="hagw-pb-DEV_GET_ONOFF_STATE_RSP_IND"></a>
**DEV_GET_ONOFF_STATE_RSP_IND**  
This command ID describes when an on/off state response is received.
This is a response indicating the current state of a light device.
When this is received, the status and GUID of the light are packaged and emitted as a *hagateway:light_device:state* event to 
the Zigbee Gateway.

# <a name="hagw-pb-DEV_GET_LEVEL_RSP_IND"></a>
**DEV_GET_LEVEL_RSP_IND**  
This command ID describes when a level response is received.
This is a response indicating the current brightness level of a light device.
When this is received, the level value and GUID of the light are packaged and emitted as a *hagateway:light_device:level* event to 
the Zigbee Gateway.

# <a name="hagw-pb-DEV_GET_COLOR_RSP_IND"></a>
**DEV_GET_COLOR_RSP_IND**  
This command ID describes when a color response is received.
This is a response indicating the current color data of a light device.
When this is received, the hue, saturation and GUID of the light are packaged and emitted as a *hagateway:light_device:color* event to 
the Zigbee Gateway.

# <a name="hagw-pb-DEV_GET_TEMP_RSP_IND"></a>
**DEV_GET_TEMP_RSP_IND**  
This command ID describes when a temperature response is received.
This is a response indicating the current temperature data of a temperature sensor device.
When this is received, the temperature data and GUID of the temperature sensor are packaged and emitted as 
a *hagateway:temp_device:temp* event to the Zigbee Gateway.

# <a name="hagw-pb-GW_ATTRIBUTE_REPORTING_IND"></a>
**GW_ATTRIBUTE_REPORTING_IND**  
This command ID describes when an incoming attribute report is received.
This is a response with an attribute record list for multiple attributes from a particular device.
When this is received, the attribute record list and GUID of the device the list was received from are packaged and emitted as 
a *hagateway:gw:report* event to the Zigbee Gateway.

# <a name="hagw-pb-DEV_GET_DOOR_LOCK_STATE_RSP_IND"></a>
**DEV_GET_DOOR_LOCK_STATE_RSP_IND**  
This command ID describes when a door lock state response is received.
This is a response indicating the current state of a door lock device.
When this is received, the state of the lock and GUID are packaged and emitted as a *hagateway:doorlock_device:state* event to 
the Zigbee Gateway.

# <a name="hagw-pb-DEV_SET_DOOR_LOCK_RSP_IND"></a>
**DEV_SET_DOOR_LOCK_RSP_IND**  
This command ID describes when a door lock set status response is received.
This is a response indicating that the setting of a door lock was either successful or failed.
When this is received, the status of the lock and GUID are packaged and emitted as a *hagateway:doorlock_device:set_rsp* event to 
the Zigbee Gateway.

# <a name="hagw-pb-GW_READ_DEVICE_ATTRIBUTE_RSP_IND"></a>
**GW_READ_DEVICE_ATTRIBUTE_RSP_IND**  
This command ID describes when a read device attribute response is received.
This is a response consisting of the data requested by a certain attribute from a device.
When this is received, the attribute list and GUID are packaged and emitted as a *hagateway:attribute:read:rsp* event to 
the Zigbee Gateway.

# <a name="hagw-pb-GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND"></a>
**GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND**  
This command ID describes when a write device attribute response is received.
This is a response consisting whether writing to an attribute list was successful.
When this is received, the error list and GUID are packaged and emitted as a *hagateway:attribute:write:rsp* event to the Zigbee Gateway.

# <a name="hagw-local-functions"></a>
#### Local Functions

* [clientReconnect](#hagw-local-functions-clientReconnect)
* [sendMessage](#hagw-local-functions-sendMessage)
* [queTxMessage](#hagw-local-functions-queTxMessage)
* [txNextMessage](#hagw-local-functions-txNextMessage)
* [getRspHndlr](#hagw-local-functions-getRspHndlr)
* [putRspHndlr](#hagw-local-functions-putRspHndlr)
* [timeoutRspHndlr](#hagw-local-functions-timeoutRspHndlr)

# <a name="hagw-local-functions-clientReconnect"></a>
**clientReconnect**  
This function is responsible for attempting to reconnect to the home automation module on the Linux Zigbee Gateway side. 
Every 5 seconds it will attempt to reconnect to the gateway until it is successful.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-local-functions-sendMessage"></a>
**sendMessage**  
This function will send a Protobuf buffer message to a specific device.
 A callback function will wait for a response to ensure the message was sent successfully.

* **Input**  
    * *Parameter*
        - msg_buf
        - gwAddress
        - responseCb
    * *Type*
        - BUFFER
        - JS OBJECT
        - FUNCTION
    * *Description*
        - The input consists of a message buffer, destination address information and a callback function.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-local-functions-queTxMessage"></a>
**queTxMessage**  
Adds a message to be sent to a queue if there are other messages waiting to be sent. 
If the queue is currently empty, the function will simply write the message to the server.

* **Input**  
    * *Parameter*
        - pkt_buf
        - responseCb
    * *Type*
        - BUFFER
        - FUNCTION
    * *Description*
        - The input consists of a message buffer and a callback function.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-local-functions-txNextMessage"></a>
**txNextMessage**  
This function will take the next message ready to be sent from the queue and send the message. 
This will be called when a message can be sent.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-local-functions-getRspHndlr"></a>
**getRspHndlr**  
This function is responsible for getting response information that was temporarily stored when a message was sent. 
Each of these responses are given a specific sequence number so they can be referenced.

* **Input**  
    * *Parameter*
        - sequenceNumber
    * *Type*
        - INT
    * *Description*
        - The specific sequence number given to a response message.
* **Output**
    * *Parameter*
        - rsp
    * *Type*
        - JS OBJECT
    * *Description*
        - A JavaScript object containing the stored response message.

# <a name="hagw-local-functions-putRspHndlr"></a>
**putRspHndlr**  
This function is responsible for storing response information when a message is sent. 
This message is added to the handler queue. 
This allows to keep track of timeout errors for messages that are not sending or are not responding in a proper amount of time.

* **Input**  
    * *Parameter*
        - responseHndl
    * *Type*
        - JS OBJECT
    * *Description*
        - A response object to be stored.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-local-functions-timeoutRspHndlr"></a>
**timeoutRspHndlr**  
This function creates a timeout function based on the sequence number of a sent message. 
This will be called if not response is received from a message request within a particular timeout period.

* **Input**  
    * *Parameter*
        - seqNum
    * *Type*
        - INT
    * *Description*
        - The sequence number of the message sent.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-light-dev"></a>
#### Public Functions - Light Device

* [setState](#hagw-public-functions-light-dev-setState)
* [getState](#hagw-public-functions-light-dev-getState)
* [setLevel](#hagw-public-functions-light-dev-setLevel)
* [getLevel](#hagw-public-functions-light-dev-getLevel)
* [setColor](#hagw-public-functions-light-dev-setColor)
* [getColor](#hagw-public-functions-light-dev-getColor)

# <a name="hagw-public-functions-light-dev-setState"></a>
**setState**  
This function will set the state of a Zigbee Light device. 
Requires a "state", "ieee", "ep" and "requestCb" arguments. 
The "state" argument defines the state to set the light too. 
The "ieee" argument defines the IEEE address of the light device the command is being sent to. 
The "ep" is the end point of the light device application. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - state
        - ieee
        - ep
        - requestCb
    * *Type*
        - INT
        - BUFFER
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the state to set the light, the IEEE address of the light and the end point of the light.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-light-dev-getState"></a>
**getState**  
This function will get the state of a Zigbee Light device. 
Requires an "ieee", "ep" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the light device the command is being sent to. 
The "ep" is the end point of the light device application. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the IEEE address of the light and the end point of the light.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-light-dev-setLevel"></a>
**setLevel**  
This function will set the brightness level of a Zigbee Light device. 
Requires a "level", "ieee", "ep" and "requestCb" arguments. 
The "level" argument defines the brightness level to set the light too. 
The "ieee" argument defines the IEEE address of the light device the command is being sent to. 
The "ep" is the end point of the light device application. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - level
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the level to set the light, the IEEE address of the light and the end point of the light.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-light-dev-getLevel"></a>
**getLevel**  
This function will get the brightness level of a Zigbee Light device. 
Requires an "ieee", "ep" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the light device the command is being sent to. 
The "ep" is the end point of the light device application. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the IEEE address of the light and the end point of the light.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-light-dev-setColor"></a>
**setColor**  
This function will set the color level of a Zigbee Light device. 
Requires a "ieee", "ep", "hue", "saturation" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the light device the command is being sent to. 
The "ep" is the end point of the light device application. 
The "hue" is the hue level to send to the light device. 
The "saturation" is the saturation color level to send to the light device. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - hue
        - saturation
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - INT
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the hue and saturation to set the light, the IEEE address of the light and the end point of the light.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-light-dev-getColor"></a>
**getColor**  
This function will get the color data of a Zigbee Light device. 
Requires an "ieee", "ep" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the light device the command is being sent to. 
The "ep" is the end point of the light device application. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the IEEE address of the light and the end point of the light.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-thermostat-dev"></a>
#### Public Functions - Thermostat Device

* [setpointChange](#hagw-public-functions-thermostat-dev-setpointChange)
* [getTemp](#hagw-public-functions-thermostat-dev-getTemp)

# <a name="hagw-public-functions-thermostat-dev-setpointChange"></a>
**setpointChange**  
This function sets a thermostat set point mode by a certain increment/decrement from its current set value. 
Requires an "ieee", "ep", "mode", "amount" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the thermostat device the command is being sent to. 
The "ep" is the end point of the thermostat device application. The "mode" is the request thermostat mode to set. 
The "amount" is the value to increment or decrement the mode set point by. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - mode
        - amount
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - INT
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the mode and amount to set the thermostat, the IEEE address of the thermostat and the end point of the thermostat.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-thermostat-dev-getTemp"></a>
**getTemp**  
This function will get the temperature data of a Zigbee Thermostat device. 
Requires an "ieee", "ep" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the thermostat device the command is being sent to. 
The "ep" is the end point of the thermostat device application. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the IEEE address of the thermostat and the end point of the thermostat.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-door-lock-dev"></a>
#### Public Functions - Door Lock Device

* [getDoorlockState](#hagw-public-functions-door-lock-dev-public-functions-getDoorlockState)
* [setDoorlockState](#hagw-public-functions-door-lock-dev-public-functions-setDoorlockState)

# <a name="hagw-public-functions-door-lock-dev-public-functions-getDoorlockState"</a>
**getDoorlockState**  
This function will get the state of a Zigbee Door Lock device. 
Requires an "ieee", "ep" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the door lock device the command is being sent to. 
The "ep" is the end point of the door lock device application. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the IEEE address of the thermostat and the end point of the thermostat.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-door-lock-dev-public-functions-setDoorlockState"></a>
**setDoorlockState**  
This function sets the state of a Zigbee Door Lock device. 
Requires an "ieee", "ep", "state", "pin" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the door lock device the command is being sent to. 
The "ep" is the end point of the door lock device application. 
The "state" argument is the state to set the door lock device to. 
The "pin" argument is the security pin value used to authenticate making changes to the lock. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - state
        - pin
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - INT
        - INT
        - FUNCTION
    * *Description*
        - Input arguments include the state and pin to set the door lock, the IEEE address of the door lock and the end point of the door lock.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-generic-dev"></a>
#### Public Functions - Generic Devices

* [setAttrReport](#hagw-public-functions-generic-dev-setAttrReport)
* [readDeviceAttribute](#hagw-public-functions-generic-dev-readDeviceAttribute)
* [writeDeviceAttribute_uint16](#hagw-public-functions-generic-dev-writeDeviceAttribute_uint16)
* [reconnectHagateway](#hagw-public-functions-generic-dev-reconnectHagateway)

# <a name="hagw-public-functions-generic-dev-setAttrReport"></a>
**setAttrReport**  
This function sets a certain attribute to be reported at a requested interval of time. 
Requires an "ieee", "ep", "clusterId", "attrList" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the device the command is being sent to. 
The "ep" is the end point of the device application. 
The "clusterId" argument is the cluster ID number that the attribute is associated with. 
The "attrList" argument is an object containing the attribute ID, attribute type and the min and max reporting intervals. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - clusterId
        - attrList
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - INT
        - JS OBJECT
        - FUNCTION
    * *Description*
        - Input arguments include the cluster ID and attribute list to set the device, the IEEE address of the device and the end point of the device.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-generic-dev-readDeviceAttribute"></a>
**readDeviceAttribute**  
This function sends a request to read a certain attribute from a particular device. 
Requires an "ieee", "ep", "clusterId", "attrList" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the device the command is being sent to. 
The "ep" is the end point of the device application. 
The "clusterId" argument is the cluster ID number that the attribute is associated with. 
The "attrList" argument is an object containing the attribute ID and attribute type. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - clusterId
        - attrList
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - INT
        - JS OBJECT
        - FUNCTION
    * *Description*
        - Input arguments include the cluster ID and attribute list to set the device, the IEEE address of the device and the end point of the device.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-generic-dev-writeDeviceAttribute_uint16"></a>
**writeDeviceAttribute_uint16**  
This function sends a request to write a UINT16 attribute to a particular device. 
Requires an "ieee", "ep", "clusterId", "attrId", "attrValue" and "requestCb" arguments. 
The "ieee" argument defines the IEEE address of the device the command is being sent to. 
The "ep" is the end point of the device application. 
The "clusterId" argument is the cluster ID number that the attribute is associated with. 
The "attrId" argument is the ID of the attribute that is going to be written too. 
The "attrValue" argument is the UINT16 value to write to the attribute. 
The "requestCb" is a callback function that will be called when a response from the sent message is received.

* **Input**  
    * *Parameter*
        - ieee
        - ep
        - clusterId
        - attrId
        - attrValue
        - requestCb
    * *Type*
        - BUFFER
        - INT
        - INT
        - INT
        - UINT16
        - FUNCTION
    * *Description*
        - Input arguments include the cluster ID, attribute ID and attribute value to set the device, the IEEE address of the device and the end point of the device.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="hagw-public-functions-generic-dev-reconnectHagateway"></a>
**reconnectHagateway**  
This function is responsible for calling the "clientReconnect" function. This allows the Zigbee Gateway to trigger an attempt to reconnect to the Linux Zigbee Gateway.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-js"></a>
### nwkmgr.js

The "nwkmgr" is a JavaScript module that is responsible handling incoming network manager gateway events from the Linux Zigbee Gateway. 
It also provides a means of communication between the Zigbee Gateway and Linux Zigbee Gateway regarding general Zigbee Network management.

* [Incoming protobuf Messages](#nwkmgr-incoming-protobuf-messages)
* [Local Functions](#nwkmgr-local-functions)
* [Public Functions](#nwkmgr-public-functions)

# <a name="nwkmgr-incoming-protobuf-messages"></a>
#### Incoming protobuf Messages

* [ZIGBEE_GENERIC_CNF](#nwkmgr-incoming-protobuf-messages-ZIGBEE_GENERIC_CNF)
* [ZIGBEE_GENERIC_RSP_IND](#nwkmgr-incoming-protobuf-messages-ZIGBEE_GENERIC_RSP_IND)
* [NWK_SET_BINDING_ENTRY_RSP_IND](#nwkmgr-incoming-protobuf-messages-NWK_SET_BINDING_ENTRY_RSP_IND)
* [NWK_GET_DEVICE_LIST_CNF](#nwkmgr-incoming-protobuf-messages-NWK_GET_DEVICE_LIST_CNF)
* [NWK_ZIGBEE_DEVICE_IND](#nwkmgr-incoming-protobuf-messages-NWK_ZIGBEE_DEVICE_IND)
* [NWK_GET_LOCAL_DEVICE_INFO_CNF](#nwkmgr-incoming-protobuf-messages-NWK_GET_LOCAL_DEVICE_INFO_CNF)
* [NWK_ZIGBEE_NWK_READY_IND](#nwkmgr-incoming-protobuf-messages-NWK_ZIGBEE_NWK_READY_IND)
* [NWK_ZIGBEE_NWK_INFO_CNF](#nwkmgr-incoming-protobuf-messages-NWK_ZIGBEE_NWK_INFO_CNF)

# <a name="nwkmgr-incoming-protobuf-messages-ZIGBEE_GENERIC_CNF"></a>
**ZIGBEE_GENERIC_CNF**  
This command ID describes a generic confirmation received by the Gateway.
This is used to determine if a command was sent successfully.

# <a name="nwkmgr-incoming-protobuf-messages-ZIGBEE_GENERIC_RSP_IND"></a>
**ZIGBEE_GENERIC_RSP_IND**  
This command ID describes when a generic response is received by the Gateway.
When it is received by the "nwkmgr", it will emit a "gen-rsp" event along with the status of the response and the IEEE address and end point the response was coming from.

# <a name="nwkmgr-incoming-protobuf-messages-NWK_SET_BINDING_ENTRY_RSP_IND"></a>
**NWK_SET_BINDING_ENTRY_RSP_IND**  
This command ID describes when a response is received from a set binding request.
When this ID is received, the Protobuf buffer will be decoded and the message will then be checked to see if any error codes exist.
If the response has an error, the error will be sent with a nwkmgr:binding:rsp event.
If the binding was successful, all of the necessary binding information will be packaged and sent with a nwkmgr:binding:rsp message.

# <a name="nwkmgr-incoming-protobuf-messages-NWK_GET_DEVICE_LIST_CNF"></a>
**NWK_GET_DEVICE_LIST_CNF**  
This command ID is received when the network manager receives a confirmation of the latest device list.
When this is received, each device in the list is checked to see if it is already recognized as a device on the network.
If there is a new device, a device object will be generated and the device object will be emitted with a nwkmgr:newDev event to the Zigbee Gateway.

# <a name="nwkmgr-incoming-protobuf-messages-NWK_ZIGBEE_DEVICE_IND"></a>
**NWK_ZIGBEE_DEVICE_IND**  
This command ID is received when the network manager receives a confirmation of a new device joining the network.
When this is received, each device in the list is checked to see if it is already recognized as a device on the network.
If there is a new device, a device object will be generated and the device object will be emitted with an nwkmgr:newDev event to the Zigbee Gateway.

# <a name="nwkmgr-incoming-protobuf-messages-NWK_GET_LOCAL_DEVICE_INFO_CNF"></a>
**NWK_GET_LOCAL_DEVICE_INFO_CNF**  
This command ID is received when the network manager receives confirmation of the Zigbee network information.
When this is received, the buffer is decoded and the information list is stored as a variable within the network manager module.

# <a name="nwkmgr-incoming-protobuf-messages-NWK_ZIGBEE_NWK_READY_IND"></a>
**NWK_ZIGBEE_NWK_READY_IND**  
This command ID is received when the Linux Zigbee Gateway is ready to start sending and receiving commands.
When this is received, an nwkmgr:network:ready event is emitted to the Zigbee Gateway.

# <a name="nwkmgr-incoming-protobuf-messages-NWK_ZIGBEE_NWK_INFO_CNF"></a>
**NWK_ZIGBEE_NWK_INFO_CNF**  
This command ID is received when the network manager receives confirmation of the Zigbee network information.
When this is received, the buffer is decoded and the information is sent along with a nwkmgr:network:info event.

# <a name="nwkmgr-local-functions"></a>
#### Local Functions

* [clientReconnect](#nwkmgr-local-functions-clientReconnect)
* [sendMessage](#nwkmgr-local-functions-sendMessage)
* [queTxMessage](#nwkmgr-local-functions-queTxMessage)
* [txNextMessage](#nwkmgr-local-functions-txNextMessage)
* [getRspHndlr](#nwkmgr-local-functions-getRspHndlr)
* [putRspHndlr](#nwkmgr-local-functions-putRspHndlr)
* [timeoutRspHndlr](#nwkmgr-local-functions-timeoutRspHndlr)
* [getGwInfoReq](#nwkmgr-local-functions-getGwInfoReq)

# <a name="nwkmgr-local-functions-clientReconnect"></a>
**clientReconnect**  
This function is responsible for attempting to reconnect to the home automation module on the Linux Zigbee Gateway side. 
Every 5 seconds it will attempt to reconnect to the gateway until it is successful.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-local-functions-sendMessage"></a>
**sendMessage**  
This function will send a Protobuf buffer message to a specific device. 
A callback function will wait for a response to ensure the message was sent successfully.

* **Input**  
    * *Parameter*
        - msg_buf
        - gwAddress
        - responseCb
    * *Type*
        - BUFFER
        - JS OBJECT
        - FUNCTION
    * *Description*
        - The input consists of a message buffer, destination address information and a callback function.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-local-functions-queTxMessage"></a>
**queTxMessage**  
Adds a message to be sent to a queue if there are other messages waiting to be sent. 
If the queue is currently empty, the function will simply write the message to the server.

* **Input**  
    * *Parameter*
        - pkt_buf
        - responseCb
    * *Type*
        - BUFFER
        - FUNCTION
    * *Description*
        - The input consists of a message buffer and a callback function.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-local-functions-txNextMessage"></a>
**txNextMessage**  
This function will take the next message ready to be sent from the queue and send the message. 
This will be called when a message can be sent.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-local-functions-getRspHndlr"></a>
**getRspHndlr**  
This function is responsible for getting response information that was temporarily stored when a message was sent. 
Each of these responses are given a specific sequence number so they can be referenced.

* **Input**  
    * *Parameter*
        - sequenceNumber
    * *Type*
        - INT
    * *Description*
        - The specific sequence number given to a response message.
* **Output**
    * *Parameter*
        - rsp
    * *Type*
        - JS OBJECT
    * *Description*
        - A JavaScript object containing the stored response message.

# <a name="nwkmgr-local-functions-putRspHndlr"></a>
**putRspHndlr**  
This function is responsible for storing response information when a message is sent. 
This message is added to the handler queue. 
This allows to keep track of timeout errors for messages that are not sending or are not responding in a proper amount of time.

* **Input**  
    * *Parameter*
        - responseHndl
    * *Type*
        - JS OBJECT
    * *Description*
        - A response object to be stored.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-local-functions-timeoutRspHndlr"></a>
**timeoutRspHndlr**  
This function creates a timeout function based on the sequence number of a sent message. 
This will be called if not response is received from a message request within a particular timeout period.

* **Input**  
    * *Parameter*
        - seqNum
    * *Type*
        - INT
    * *Description*
        - The sequence number of the message sent.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-local-functions-getGwInfoReq"></a>
**getGwInfoReq**  
This function requests for the latest local device information. 
When the request is met, the Gateway will send the latest information regarding the gateway.

* **Input**  
    * *Parameter*
        - seqNum
    * *Type*
        - INT
    * *Description*
        - The sequence number of the message sent.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-public-functions"></a>
#### Public Functions

* [getNetworkInfo](#nwkmgr-public-functions-getNetworkInfo)
* [openNetwork](#nwkmgr-public-functions-openNetwork)
* [getDevList](#nwkmgr-public-functions-getDevList)
* [setDevBinding](#nwkmgr-public-functions-setDevBinding)
* [removeDev](#nwkmgr-public-functions-removeDev)
* [getGwInfo](#nwkmgr-public-functions-getGwInfo)
* [hardSystemReset](#nwkmgr-public-functions-hardSystemReset)
* [softSystemReset](#nwkmgr-public-functions-softSystemReset)
* [reconnectNwkmgr](#nwkmgr-public-functions-reconnectNwkmgr)

# <a name="nwkmgr-public-functions-getNetworkInfo"></a>
**getNetworkInfo**  
This function calls the "getNetworkInfoReq()" function to request the latest Zigbee Network Information.

* **Input**  
    * *Parameter*
        - seqNum
    * *Type*
        - INT
    * *Description*
        - The sequence number of the message sent.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-public-functions-openNetwork"></a>
**openNetwork**  
This function sends a request to open the Zigbee Network join permission for a desired duration of time. 
When a duration of "0" is sent, the Zigbee Network will close and no devices will be able to join the network.

* **Input**  
    * *Parameter*
        - Duration
    * *Type*
        - INT
    * *Description*
        - The number of seconds to keep the Zigbee Network open.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-public-functions-getDevList"></a>
**getDevList**  
This function will send a request to get the latest list of all devices on the network.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-public-functions-setDevBinding"></a>
**setDevBinding**  
This function requests a bind or an unbind to occur between two desired devices. 
Requires a "bindingMode", "srcIeee", "srcEp", "dstIeee", "dstEp", "clusterId" and "bindCnfCb" arguments. 
The "bindingMode" argument defines whether a bind or unbind is going to occur between the two devices. 
The "srcIeee" argument defines the IEEE address of the source device that will be bound or unbound. 
The "srcEp" argument defines the end point of the source application that will be bound or unbound. 
The "dstIeee" argument defines the IEEE address of the destination device that will be bound or unbound. 
The "dstEp" argument defines the end point of the destination application that will be bound or unbound. 
The "clusterId" argument defines the cluster ID number that the bind will be occurring on. 
The "bindCnfCb" argument is a callback function to determine if the bind action is successful or not.

* **Input**  
    * *Parameter*
        - bindingMode
        - srcIeee
        - srcEp
        - dstIeee
        - dstEp
        - clusterId
        - bindCnfCb
    * *Type*
        - INT
        - BUFFER
        - INT
        - BUFFER
        - INT
        - INT
        - FUNCTION
    * *Description*
        - The input arguments include the bindingMode (0 = BIND, 1 = UNBIND), the source IEEE and endpoint, the destination IEEE and endpoint and the cluster ID.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-public-functions-removeDev"></a>
**removeDev**  
This function is responsible for sending a leave request to a device on the network to leave the network. 
This request is based on the IEEE address of the device requested to leave the network.

* **Input**  
    * *Parameter*
        - ieee
    * *Type*
        - BUFFER
    * *Description*
        - The IEEE address of the device that is being requested to leave the network.
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-public-functions-getGwInfo"></a>
**getGwInfo**  
This function is responsible for returning the latest known data regarding the gateway. 
This is the data that is stored as a variable from the gateway info confirmation event.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-public-functions-hardSystemReset"></a>
**hardSystemReset**  
This function is responsible for sending a request to commence a hard system reset of the Zigbee Gateway and ZNP device. 
This will reset the Zigbee Network and remove all currently joined devices from the network. 
This will also remove all devices and binding information from the Node.js Zigbee Gateway module.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-public-functions-softSystemReset"></a>
**softSystemReset**  
This function is responsible for sending a request to commence a soft system reset of the Zigbee Gateway. 
This will reset the Linux Zigbee Gateway servers and maintain all currently joined devices on the network.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="nwkmgr-public-functions-reconnectNwkmgr"></a>
**reconnectNwkmgr**  
This function is responsible for calling the "clientReconnect" function. 
This allows the Zigbee Gateway to trigger an attempt to reconnect to the Linux Zigbee Gateway.

* **Input**  
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL
* **Output**
    * *Parameter*
        - NULL
    * *Type*
        - NULL
    * *Description*
        - NULL

# <a name="ibm-cloud-adapter-js"></a>
### ibmCloudAdapter.js

The "ibmCloudAdapter" is a JavaScript module that is responsible handling subscribing to and publishing messages from 
the IBM Watson IoT service. 
It is basically an extension of the "webserver" that handles an authenticated connection to IBM Cloud.

* [Incoming Events - Zigbee Gateway](#ibmcloudadapter-incoming-events-zb-gw)
* [Local Functions](#ibmcloudadapter-local-functions)

# <a name="ibmcloudadapter-incoming-events-zb-gw"></a>
#### Incoming Events - Zigbee Gateway

* [zb-gateway:newDev](#ibmcloudadapter-incoming-events-zb-gw-newDev)
* [zb-gateway:deviceList](#ibmcloudadapter-incoming-events-zb-gw-deviceList)
* [zb-gateway:removeDeviceCard](#ibmcloudadapter-incoming-events-zb-gw-removeDeviceCard)
* [zb-gateway:network:ready](#ibmcloudadapter-incoming-events-zb-gw-network-ready)
* [zb-gateway:network:info](#ibmcloudadapter-incoming-events-zb-gw-network-info)
* [zb-gateway:binding:event](#ibmcloudadapter-incoming-events-zb-gw-binding-event)
* [zb-gateway:binding:failed](#ibmcloudadapter-incoming-events-zb-gw-binding-failed)
* [zb-gateway:unbinding:failed](#ibmcloudadapter-incoming-events-zb-gw-unbinding-failed)
* [zb-gateway:light_device:state](#ibmcloudadapter-incoming-events-zb-gw-light_device-state)
* [zb-gateway:light_device:level](#ibmcloudadapter-incoming-events-zb-gw-light_device-level)
* [zb-gateway:light_device:color](#ibmcloudadapter-incoming-events-zb-gw-light_device-color)
* [zb-gateway:temp_device:temp](#ibmcloudadapter-incoming-events-zb-gw-temp_device-temp)
* [zb-gateway:doorlock_device:state](#ibmcloudadapter-incoming-events-zb-gw-doorlock_device:state)
* [zb-gateway:doorlock_device:set_rsp](#ibmcloudadapter-incoming-events-zb-gw-doorlock_device-set_rsp)
* [zb-gateway:thermostat:attribute](#ibmcloudadapter-incoming-events-zb-gw-thermostat-attribute)

# <a name="ibmcloudadapter-incoming-events-zb-gw-newDev"></a>
**zb-gateway:newDev**  
Event occurs when the Zigbee Gateway receives a new device on the network. 
When this event is received by the webserver, the lists will be packaged and a "devList" event will be published to 
the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-deviceList"></a>
**zb-gateway:deviceList**  
Event occurs when the Zigbee Gateway receives an updated list of new devices on the network. 
When this event is received by the webserver, the lists will be packaged and a "devList" event will be published to 
the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-removeDeviceCard"></a>
**zb-gateway:removeDeviceCard**  
Event occurs when the Zigbee Gateway emits that a device has been removed from the network. 
Therefore the device card needs to be removed from the web application view. 
When this event is received by the webserver, a **webserver:removeDeviceCard** event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-network-ready"></a>
**zb-gateway:network:ready**  
Event occurs when the Zigbee Gateway emits an event confirming the Network and Linux Gateway are ready. 
When this event is received by the webserver, a webserver:network:ready event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-network-info"></a>
**zb-gateway:network:info**  
Event occurs when the Zigbee Gateway is sending information regarding the network. 
When this event is received by the webserver, a webserver:network:info event will be emitted to the web application.

# <a name="ibmcloudadapter-incoming-events-zb-gw-binding-event"></a>
**zb-gateway:binding:event**  
Event refers to when a binding event has occurred. 
This includes when a pair of devices is successfully bound or unbound. Will include an updated device list, bind list and unbound list. 
When this event is received by the webserver, the lists will be packaged and an *webserver:bindingDevList* event will be published to 
the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-binding-failed"></a>
**zb-gateway:binding:failed**  
Event refers to when a binding event has failed. 
When this event is received by the webserver, a *webserver:binding:failed* event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-unbinding-failed"></a>
**zb-gateway:unbinding:failed**  
Event refers to when an unbinding event has failed. 
When this event is received by the webserver, a *webserver:unbinding:failed* event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-light_device-state"></a>
**zb-gateway:light_device:state**  
Event refers to when a light device state has changed. 
When this event is received by the webserver, a *webserver:light_device:state* event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-light_device-level"></a>
**zb-gateway:light_device:level**  
Event refers to when a light device level has changed. 
When this event is received by the webserver, a *webserver:light_device:level* event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-light_device-color"></a>
**zb-gateway:light_device:color**  
Event refers to when a light device color has changed. 
When this event is received by the webserver, a *webserver:light_device:color* event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-temp_device-temp"></a>
**zb-gateway:temp_device:temp**  
Event refers to when a temperature device temperature reading has changed. 
When this event is received by the webserver, a *webserver:temp_device:temp* event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-doorlock_device:state"></a>
**zb-gateway:doorlock_device:state**  
Event refers to when a door lock device state has changed. 
When this event is received by the webserver, a *webserver:doorlock_device:state* event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-doorlock_device-set_rsp"></a>
**zb-gateway:doorlock_device:set_rsp**  
Event refers to when a door lock device set response has been received by the gateway. 
When this event is received by the webserver, a *webserver:doorlock_device:set_rsp* event will be published to 
the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-incoming-events-zb-gw-thermostat-attribute"></a>
**zb-gateway:thermostat:attribute**  
Event refers to when a thermostat device attribute has been received by the gateway. 
When this event is received by the webserver, a webserver:thermostat:attribute event will be published to the IBM Watson IoT Platform.

# <a name="ibmcloudadapter-local-functions"></a>
#### Local Functions

* [EVT](#ibmcloudadapter-local-functions-EVT)
* [bufferPayloadToObject](#ibmcloudadapter-local-functions-bufferPayloadToObject)

# <a name="ibmcloudadapter-local-functions-EVT"></a>
**EVT**  
Generates a string of the input string arguments separated by colons. 
This is the same format used to define events between modules in the gateway. 
This is typically used to create event strings based on a variable types. 
The function can a variable number of input arguments.

Ex): EVT(arg1, arg2, arg3) --> returns string "arg1:arg2:arg3"

* **Input**  
    * *Parameter*
        - Evt1, Evt2, ...
    * *Type*
        - String
    * *Description*
        - A variable number of string arguments.
* **Output**
    * *Parameter*
        - Event_string
    * *Type*
        - String
    * *Description*
        - A string of all argument inputs separated by colons.

# <a name="ibmcloudadapter-local-functions-bufferPayloadToObject"></a>
**bufferPayloadToObject**  
This function is responsible for converting an incoming serial buffer to a JavaScript Object. 
The function will go through the serial buffer and convert it to a JavaScript Object. 
It will then iterate through all properties in the object and look for buffer types. 
If a buffer type is found, it will convert the string based buffer to an actual buffer object.

* **Input**  
    * *Parameter*
        - obj
    * *Type*
        - JS OBJECT
    * *Description*
        - A JS object with nested buffer elements.
* **Output**
    * *Parameter*
        - obj
    * *Type*
        - JS OBJECT
    * *Description*
        - A JS object with converted JS buffer elements.
