# Z-Stack Linux Gateway - User's Guide

This document provides directions on how to use the gateway, gateway application, and web application.

* [Purpose](#purpose)
* [Getting Started](#getting-started)
* [Installation Contents](#installation-contents)
* [Configuring the Gateway Servers](#configuring-the-gateway-servers)
* [Using the Local Gateway Sample Application](#using-the-local-gateway-sample-application)
* [Using the Web Application](#using-the-web-application)
    * [IBM&reg; Cloud&reg; Instructions](#ibm-cloud-instructions)

# <a name="purpose"></a>
#Purpose

The Z-Stack&trade; Linux Gateway is a complete software development suite which can be used to develop a
Zigbee-to-IP gateway solution.
It features an abstracted socket API for application developers to access the Zigbee functionality, implemented through a set of Linux-based servers.
The targeted product SW architecture consists of a Linux host SW component (running on a Linux
machine) and Zigbee Network Processor (ZNP) firmware, running on one of TI's SimpleLink&trade; CC13x2/26x2 wireless MCUs.
The software designed to run on the Linux host is a collection of server applications that accomplish two key
goals:

* Abstraction of the details of Zigbee network management
* Acceleration of application development through a simple and intuitive API

<img src="/resources/block diagram.png" alt="Gateway Block Diagram" width="750"/>

# <a name="getting-started"></a>
# Getting Started

To get started, please see the [Z-Stack Linux Gateway - Quick Start Guide](Z-Stack Linux Gateway - Quick Start Guide.html),
which details the required hardware/software and how to set them up to use the out of box Z-Stack Linux Gateway.

# <a name="installation-contents"></a>
# Installation Contents

This section will describe what the directories in this installation include.

- **Documents**
    - Documentation on how to use the gateway.
- **Firmware**
    - Prebuilt firmware images to use for the ZNP (Zigbee Network Processor) and some Z-Stack sample applications.
    - These are provided for convenience to quickly evaluate the Z-Stack Linux Gateway; all images were built with sample applications from the [SimpleLink&trade; CC13x2/CC26x2 SDK](http://www.ti.com/tool/simplelink-cc13x2-26x2-sdk).
- **Proto_files**
    - The .proto files from which the protobuf interface is built.
- **source**
    - Complete source installation for all the servers.
    - Includes build makefiles and scripts required for re-building the servers.
    - *Components*
        - Other components of the Z-Stack that the various servers depend on.
    - *out*
        - Precompiled gateway servers for the BeagleBone Black.
    - *Projects*
        - Contains the sources for all the Linux servers, the gateway sample application, and the Node.js apps/components.
        - Further description of this directory is described [here](#project-directory).
    - *scripts/package_builder*
        - Script used for building the package after compiling the servers and gateway sample application.
        - The host platform on which the gateway will run is specified by TARGET_PLATFORM
            - If the host is a BeagleBone Black, use `export TARGET_PLATFORM="BEAGLEBONE_BLACK"`
                - Server executables will have "_arm" appended (eg GATEWAY_SRVR**_arm**)
            - If the host is a Linux OS machine, use `export TARGET_PLATFORM="x86"`
                - Server executables will have "_x86" appended (eg GATEWAY_SRVR**_x86**)
    - *build_all*
        - Script used to re-build all the servers using the appropriate toolchain for your gateway target platform.
        - Creates a directory **Source/out** with a **Precompiled.tar** containing all the built servers and sample application.
        - You may use this script to rebuild the gateway servers and gateway sample application by running `sudo /bin/bash ./build_all`
            - Then, execute the same sequence of commands as in `setup.sh` to extract and make images/scripts executable:
                1. `cd out/Precompiled`
                * `sudo tar -xvf ../Precompiled.tar`
                * `cd servers/`
                * `sudo chmod +x zigbeeHAgw track_servers start_application`
                * `sudo chmod +x ../tools/*`
    - *setup.sh*
        - A script which performs all steps required (building, installing necessary components) to run the Z-Stack Linux Gateway and Sensor to Cloud application.

# <a name="project-directory"></a>
## Project Directory  
This section will detail the contents of the Project directory.

* *cloud_apps*  
    Contains all example applications to be hosted on various Cloud platforms (IBM Cloud, AWS, etc.).
    - *ibm-webapp*  
    Example web application to be hosted on IBM Cloud, Cloud Foundry service.
* *node*  
    Includes all of the source code and necessary scripts to run the Node.js Zigbee Gateway.
    - *nodejs_zb_gateway*  
    This contains all of the modules to run the Node.js Zigbee Gateway. 
    It includes all of the necessary Node Modules, yet they can be reinstalled using the package.json file and `npm install` command.
        - **NOTE**: Node.js must already be installed on the device running the Node.js Zigbee Gateway. 
        In this case, Node.js is already included with the version of Debian running on the BeagleBone Black.
    -  *scripts*   
    There are also various scripts included in this directory that assist in 
    initial setup, starting and stopping the gateway applications:
        - **end.sh**
            - Ends all servers associated with Linux and Node.js gateways.
        - **start_gateway.sh**
            - Starts the Linux Zigbee Gateway servers.
        - **start_ibm.sh**
            - Starts the Linux Zigbee Gateway and Node.js Zigbee Gateway servers to connect to IBM Cloud service. 
        - **start_local.sh**
            - Starts the Linux Zigbee Gateway and Node.js Zigbee Gateway servers to connect to local webserver.
- *zstack*  
Includes all of the C source code for the Linux Zigbee Gateway.
    - *linux*  
     Includes all of the source code for the Linux based gateways
    - *OTA*  
     Includes source code for OTA examples applications.

# <a name="configuring-the-gateway-servers"></a>
# Configuring the Gateway Servers

Often a user might want to have more control over the network that is created by the servers, or over the local Gateway device itself.
Configuration files are available that allow certain parameters to be tweaked to achieve this. 
Discussed below are some of the more common things that a user might want to tweak through these files.
See more configurables [here](#more-configurables).

**Zigbee Network Parameters**

A Zigbee network is created when the servers are run via the zigbeeHAgw script. This script individually
brings up all the servers in the correct order, and also supplies them configuration files that they read upon
startup. config.ini is the configuration file read by the Z-Stack ZNP server that provides the core Zigbee stack
interface (Application Framework and Zigbee Device Object layers). This file contains parameters that let
you configure the network created with the Z-Stack device. The user could change the channel the network
uses via the parameter DEFAULT_CHANLIST, or it could provide a PAN ID that it wants the network to use
via ZDAPP_CONFIG_PAN_ID. Proper Gateway operation requires the local Gateway device to act as the
network Coordinator, so DEVICE_TYPE should be left as is (set to 0). If you would like the network to be
open for joining devices on start up, you can tweak the parameter PERMIT_JOIN. If the devices that need to
join the network will require an install code to join, the BDB_DEFAULT_JOIN_USES_INSTALL_CODE_KEY
should be set to 0. More information on config.ini can be found [here](#zstack-znp-server-configurables).
All the above parameters in config.ini are read at startup, and will be read again if a hard reset is issued to
the network manager via the NWK_ZIGBEE_SYSTEM_RESET_REQ command (Z-Stack Linux Gateway
API document, section 8.1.1).

**Defining Endpoints on the Gateway device**

The local device that acts as the coordinator of the network is also a Zigbee node and can support Zigbee
endpoints and clusters. This allows the local device to 'act' like a Zigbee device and communicate with the
endpoints on other devices in the network. All messages sent by other devices to this endpoint would be
received by the application.

An interesting use-case for defining endpoints would be if a Zigbee device in the network needed to control
a non-Zigbee device. For instance, if we had a Wi-Fi Light that we wanted to control via a Zigbee switch in
our network, we could represent that light via an endpoint defined on our local device. Binding the Zigbee
switch with this local light endpoint, could allow the application to receive messages whenever the switch is
operated. The application would typically receive this message as an incoming ZCL frame
GW_ZCL_FRAME_RECEIVE_IND (Z-Stack Linux Gateway API document, Section 9.3.12) . This message
could then be translated by the application into a Wi-Fi message that the switch understands.

The gateway endpoints can be defined in a configuration file called gateway_config.tlg that the Gateway
server reads at startup. The file is re-read every time the application issues a reset via the
NWK_ZIGBEE_SYSTEM_RESET_REQ command. Details of this configuration file, and an example of how
to create some endpoints is provided [here](#gateway-manager-server-configurables).
The NWK_GET_LOCAL_DEVICE_INFO_REQ (Z-Stack Linux Gateway API document, Section 8.1.6) command
can be used to query the gateway device for a list of registered endpoints and their descriptors. This API can
be used by the application to display this information for the end-user as required. Z-Stack Linux Gateway
API document, Section 9.3 has more Gateway APIs that can be leveraged to keep track of or bridge-over
the attribute-values on local Gateway device endpoints.

The ability to define endpoints on the Gateway device is a very powerful feature that can be leveraged to
create a host of interesting features in the user's gateway application. The sample application also
demonstrates this feature by allowing the user to record a single-command macro and associate with a
gateway endpoint. This macro will now be recalled whenever a message is sent to the Gateway endpoint.
Details of this feature, and instructions on using it, are [here](#other-helpful-features).

# <a name="more-configurables"></a>
## More Configurables

Some of the servers that are part of this release have certain configuration parameters that allow the
network to be tuned per the requirements of the end-user application. These parameters are read once at
init-time and are used throughout the length of operation of the servers.
These servers are associated with some configuration files that indicate the current/default values assigned
to these parameters
Provided below is a brief description of some of the important configuration parameters that can be tuned for
some of these servers. More details and default values can be found in the configuration files included with
the installer.

### NPI Server

The NPI Server can be configured by modifying parameters in the NPI_Gateway.cfg file. This file contains
parameters pertaining to the hardware configuration of the board and the serial interface used to connect to
the network processor.

- [PORT] port: Port number exposed for the Z-Stack Server to connect to, defaults to 2533
- [DEVICE] 
    - deviceKey: Choose between UART (0), SPI (1) and I2C(2) 
    - devPath: String path to the device, e.g., /dev/ttyACM0
    - **Note:** Currently only UART is supported out-of-box.
- [UART]
    - speed : Baud rate, e.g., 115200 
    - flowcontrol: Flag to indicate if flow control is enabled(1) or disabled(0) for your device.

# <a name="zstack-znp-server-configurables"></a>
###ZSTACK ZNP SERVER

config.ini contains parameters pertaining to the configuration of the network created with the Z-Stack device
as coordinator. Here are some brief descriptions of these parameters:

- DEFAULT_CHANLIST: 
    - Channel number used to create the network
- ZDAPP_CONFIG_PAN_ID:
    - Choose the default PAN ID to be used to create the network.
    - Any value other than 0xFFFF causes the Coordinator to use it as the PAN ID.
- DEVICE_TYPE:
    - Can set the device to be of type Coordinator(0), Router(1), End Device(2). 
    - For proper gateway operation, this needs to be set as Coordinator.
- SERVER_PORT:
    - TCP listening port for this server. 
    - Other servers that talk to this server use this port to connect to it.
- PERMIT_JOIN: 
    - Status of network at startup can be either closed (0), or open (1) for new devices to join.
- NWK_USE_MULTICAST: 
    - Flag to disable (0) or enable (1) the Network Multicast feature.
- BDB_DEFAULT_TC_REQUIRE_KEY_EXCHANGE: 
    - Flag to enable (1) or disable (0) if the Trust Center (ZC) mandates Trust Center Link Key (TCLK) exchange.
- BDB_DEFAULT_JOIN_USES_INSTALL_CODE_KEY: 
    - Flag to set if Trust Center (ZC) will require an InstallCode from the new devices joining the network.
- BDB_DEFAULT_REMOTE_ENDDEVICE_TIMEOUT: 
    - Bitmask to set the end device timeout.

# <a name="gateway-manager-server-configurables"></a>
###GATEWAY MANAGER SERVER

The Gateway Manager Server contains a Gateway Endpoint Configuration file named gateway_config.tlg
that allows a user to define endpoints and clusters on the Gateway device itself. Other devices can then
potentially bind to the end points to this Gateway device, and also send it commands that the end-user
application then receives.

For example, an "ON/OFF/TOGGLE Light" endpoint may be defined in the gateway_config.tlg file as follows:
    
    // endpoint { ID, EndpointDef, AttrDef }
    endpoint { 3, OnOffLightEpDef, OnOffAttrList }

where the EndpointDef onOffLightEpDef is defined (in the same file) as follows:

    endpointdef OnOffLightEpDef { ha_profile, ha_onofflight, 0, OnOffInputClusters, OnOffOutputClusters }

Please refer to the gateway_config.tlg file for more examples and explanation of how to define endpoints.
**Note:**
Some endpoint IDs are reserved and must not be used as user-defined endpoint ids.
Endpoint numbers 0 (Zigbee Device Object) 2 (Gateway endpoint),14 (OTA upgrade endpoint) and 242 (Green Power endpoint) are reserved.

# <a name="using-the-local-gateway-sample-application"></a>
# Using the Local Gateway Sample Application

* [User Interface Sections](#sample-application-user-interface-sections)
* [User Interactions](#sample-application-user-interactions)

# <a name="sample-application-user-interface-sections"></a>
## User Interface Sections

![Local Gateway Application](/resources/localapp_connected.png)

There are 9 different sections of the User Interface display. Below is a brief description of each. The backsingle-
quote ( `) can be used to move to Devices, Groups, Scenes, or Install Code. The space bar can be
used to move to the actions section.

**NETWORK**

This section provides informative details of the network that has been created by the servers
started by the zigbeeHAgw script. It indicates the External PAN ID (IEEE address of the Gateway
device), the local PAN ID, channel number on which the network has been formed, and its current
state for devices to join (open/close). It also shows the current state of the Identify timer, displaying
"READY" when the timer has ended and it's ready to be set again.

**DEVICES**

This section provides details on the devices that are part of the network that has been created. At
start-up, the only device listed, is the gateway device itself. The fields displayed include its unique
IEEE address, network address (always 0x0 for the Coordinator), list of active endpoints, and some
other details about the endpoint such as the Device ID and Profile ID that it is associated with
(Home Automation i.e., HA). Device ID number is the one assigned by the specific profile
specification (or defined by the user, in case of a manufacturer specific profile).

**GROUPS**

Once you have devices added to the network you can create groups within them and that will allow
you to send an "ACTION" to a group rather than to a single device. This section
indicates which Group is active.

**SCENES**

"Scenes" can be used to remember a set of states for a group of devices. For instance, if you have
multiple lights in your network, you can change their settings individually or as a group, using the ACTIONS keys. 
Once you do that, you can save these settings in a Scene, and recall them at will.

**INSTALL CODE**

This interface allows the user to add an install code to the Gateway (Zigbee Coordinator). The first
field Install Code is a 16-byte key to establish the connection between a a joining device and the
Trust Center. The second field CRC is a 2-byte error-detecting code that will confirm if the Install
Code is correct. The third and final field is the 8-byte IEEE Address of the joining device. All three
fields are necessary to properly add the Install Code to the Gateway.

**ACTIONS**

This provides a list of commands you can send targeting the network, or individual/group of devices.
The commands include Light related settings (Off/On, Level, Hue, Saturation), Network related
settings (Open the network for devices to join, remove devices from network, bind and/or unbind
say a Switch to a Light etc), Group/Scene commands, or Base Device Behavior (Start the finding
and binding process for a specific endpoint, Add an Install Code to join a new device).

**LOG**

This section provides trace information that is generated from the application itself as it processes
user-input, and also input from the servers.

**APPLICATION STATUS**

This section provides information regarding the status of the application. The letters N, G and O in
the title-bar refer to the 3 servers that this application is connected to (Network Manager, Gateway
Manager and OTA Upgrade). Upper case indicates that the connection has been established and
lower case indicates that it has been disconnected. The **LOG** section will indicate it when an application has established connection.

**HELP**

This section provides Help. A more detailed Help Guide is available by pressing the "?" key.

# <a name="sample-application-user-interactions"></a>
## User Interactions

# <a name="sample-application-adding-devices"></a>
**Adding Devices**:  
After you bring up the sample application and it has established connection to all the servers ("NGO" visible
in the Application Status and Network Status indicates "ACTIVE"), the network is ready to accept
connections from Zigbee devices. In the "ACTIONS" section, use the spacebar to navigate to the "Permit
Join" (shortcut key "P") to open the network up for joining devices. You can change the time for which the
network remains open by pressing +/- key. The "NETWORK" section will indicate a State of "OPEN" with a
countdown timer.
At this point you can add other Zigbee devices to the network, by following the respective "Join" instructions
in their documentation. Once a device has successfully joined the network, it will be listed in the "DEVICES"
section along with all its details.

**Light Control**:  
In the "DEVICES" section, use the up/down arrow keys to navigate to a 'Light' device and use
"ON/OFF/TOGGLE" or "LVL/HUE/SAT" from the "ACTIONS" section to observe the Gateway control the
'Light' Device.

**Temperature/Humidity Sensor**:  
Temperature/Humidity Sensors added to the network will register changes automatically and periodically
when you press the "A" shortcut key.

# <a name="sample-application-bind-unbind"></a>
**Bind and Unbind**:  
The BIND/UNBIND (shortcut keys "D" and "U") action lets you bind and unbind one or more devices.

For instance, to bind a Switch to a Light, you would follow the steps below:

1. In the "DEVICES" section, use the up/down arrow keys to navigate to the desired Switch device and press "D". 
The trace log will confirm that a binding source has been selected. (Pressing "D" again, will unselect your device).
* Next navigate to a Light device and press "D" again. The trace log will confirm that a binding destination has been selected.
* Trace log will show the messages that are being sent to initiate this bind, 
and also the final result (successful or unsuccessful). 
The entries should flash green to indicate that binding has been initiated.
* After a successful bind, the Switch should be able to control the light.
    - To Unbind, press "U" for the devices one by one, in the same order.

**Groups**:  
One or more devices (such as lights) can be added to a Group and then a command can be sent to them as
a group.  

- As described in the help section ("?"), using the "\`" key will let you navigate between the "DEVICES", "GROUPS" and "SCENES" section. 
Switch to the "GROUPS" section, and use the arrow keys to select a particular group number.
- Go back to the "DEVICES" section, and select (using the arrow keys) devices to add to the group, one at a time. 
Use the "G" shortcut key to add a device to a particular group.
- Now you can switch these devices on and off as a group. 
Use the "\`" key to navigate to the "GROUPS" section and then try any of the light related "ACTIONS" (OFF/ON, LVL, HUE, SAT etc).
- Also note that pressing the "b" key will toggle between the 3 transmission modes (Unicast/Groupcast/Broadcast).

**Scenes**:  
Once a group has been created, you can set each device (individually or as a group) to a particular state and store that as a scene.

- Create a Group (as described above) consisting of 1 or more devices.
- Adjust the settings of each device to a desired value (for Light devices, use OFF/ON, LVL, HUE, SAT).
- Use the "\`" key to navigate to the "SCENES" section and pick a Scene #, then use the shortcut key "E" to store the scene.
- Change the settings of the device individually or as a group. 
Try and recall the scene using "C" either as 
a group (if in Groupcast transmission mode) or individually (if in Unicast transmission mode).

**OTA Upgrade**:  
To initiate an "OTA Upgrade" you will need to do the following:

- Prepare an OTA client, as outlined in the **Zigbee Over-The-Air Firmware Upgrade** section 
of the [Z-Stack User's Guide](http://dev.ti.com/tirex/explore/node?node=ANbR0LtTwkMnDCiEhygF7A__pTTHBmu__LATEST).
- Add the OTA Client to the network.
- Edit the existing "sample_app_ota.cfg" configuration file with following information:
<FULL PATH TO UPGRADE IMAGE > <NUM DEVICES TO UPGRADE> < IEEE ADDR of DEVICE>
- Press "O" to initiate the upgrade. The status log will show you a message when the upgrade is done (typically takes several minutes to complete).

**Adding Devices Using Install Codes**:  
You can also add devices using an extra level of security with an Install Code. 
Some devices may include a prebuilt-in code and others, like development boards, 
may have an interface that allows the user to add it manually on run time. 
To add devices using Install Code you need to set the BDB_DEFAULT_JOIN_USES_INSTALL_CODE_KEY flag to "1" 
on the config.ini file located on zstack server znp. 
This boolean flag's default value is 0 and needs to be changed before runtime in order for it to take effect. 
To add a device using an Install Code to the network through the sample app you need to:

- Use the back-single-quote(`) to navigate to the "INSTALL CODE" section and press the
DOWN arrow on your keyboard.
- Type in the 16-byte (32 digit) Install Code in hexadecimal format and press [Enter]. A message will
briefly display on the "APPLICATION STATUS" section with a confirmation status.
- Press the DOWN arrow again to move to CRC, input the 2-byte (4 digit) value and press [Enter]. In
some devices the InstallCode and CRC may appear as a single number but you must input them
as mentioned before.
- Finally, input the 8-byte (16 digit) number corresponding to the IEEE Address of the device that will
join the network and press [Enter].
- Once you complete this steps use the space bar on your keyboard and go to ADD_INSTALLCODE
(shortcut key "I").
- You can now open the network as previously described.

**Finding and Binding**:  
You can automatically create binds between the Gateway and the device in the network using START_FINDING_BINDING on the "ACTIONS" section. 
You can indicate the endpoint using the [+/-] keys and then pressing [Enter]. 
This function automatically creates binds 
by cross checking the input and output clusters of the target devices in the network that also have activated this function.

# <a name="other-helpful-features"></a>
**Other Helpful Features**:  
There are several other "shortcuts", details of which are provided in the Help menu.

- As described [here](#zstack-znp-server-configurables), you can change the channel or PAN ID for the network in the config.ini
file. However this file is only read once at start-up. To change either of these parameters, you can
modify the config.ini file, and do a hard-reset by pressing the "X" (uppercase) shortcut key.
- As described in [here](#gateway-manager-server-configurables), endpoints on the gateway can be defined using the gateway_config.tlg
file included with the sample application (as well as the Gateway server source code). Modify this
file to create the endpoints required, and then do a soft reset by pressing the "x" shortcut key. This
will allow the Gateway manager to come up again and this gateway_config.tlg file to be processed.
**Note**: Some endpoint numbers are reserved and must not be used as user-defined endpoint ids.
Endpoint number 2 (Gateway endpoint), 14 (OTA upgrade endpoint) and 242 (Green Power
endpoint) are reserved
- Endpoint 2 Simple Descriptor is defined as follows:
    - Profile: HA
    - Device ID: Combined Interface
    - Device Ver: 0
    - Flags: 0
    - Input Clusters:
        - Basic
        - Identify
        - On Off
        - IAS ACE
    - Output Clusters:
        - On Off
        - Alarms
        - Poll Control
- Endpoint 14 Simple Descriptor is defined as follows:
    - Profile: HA
    - Device ID: 0
    - Device Ver: 0
    - Flags: 0
    - Input Clusters:
        - OTA
    - Output Clusters:
        - NULL
- Endpoint 242 Simple Descriptor is defined as follows:
    - Profile: GP (0xA1E0)
    - Device ID: GP Proxy (0x0061)
    - Device Ver: 0
    - Flags: 0
    - Input Clusters:
        - NULL
    - Output Clusters:
        - Green Power
- You could potentially bind a device in the network to a Gateway endpoint, and that would allow
actions on this device to be reported to the gateway.
- The sample application has a feature wherein you can store a ZCL over-the-air command as an
application "macro", and this macro can be associated with the Gateway endpoint. Here's a quick
set of steps to try it out:
    - Add the following lines in gateway_config.tlg:
        - endpoint { 3, CombinedInterfaceEpDef, CombinedInterfaceAttrList }
    - Do a soft reset via shortcut key "x" (lower case). When the servers come back up, you
    should see the gateway device (first line in the "DEVICES" section) listed with the new endpoint.
    - Add a switch to your network (as mentioned [here](#sample-application-adding-devices)).
    - Bind this switch to one of the endpoints in the device, say endpoint 3 (as mentioned [here](#sample-application-bind-unbind)).
    Note that once you have selected a device using the UP/DOWN arrow keys, you can use
    the RIGHT/LEFT arrows keys to navigate between the endpoints. Every time you press
    the switch, you should see a message on the Status Log, confirming that the Gateway
    endpoint has received the message.
    - Add a light device to the network.
    - Now we will create/save a macro that switches this light on via a Gateway command.
        - Navigate to the light device in the DEVICES list.
        - Press the "[" key to record the macro (you should see a "R" in the ACTIONS title bar to indicate that recording has commenced).
        - Press the "N" key to switch the light on.
        - Press the "]" key to record the macro (you will see an "M" in the ACTIONS title bar to indicate a successful recording).
    - Associate this macro with the switch, by pressing the switch. Now the macro has been associated with the Switch.
    - Switch the light off using the gateway shortcut ("N"). Now you can switch it back on, by pressing the Switch.
You could also create a group and store a group ON/OFF/TOGGLE command using the above example. 
The macro store/recall is a feature of the application. 
However, the creation and binding of custom endpoints on the gateway is a very important feature of the Z-Stack Linux Gateway.
This feature allows us to associate one/more non-Zigbee actions with Zigbee device operation.  
- Several other shortcuts are available in the sample application. Here is an excerpt from the Help Menu:
```
Usage help start -------------------------------------------------
- Either Devices, Groups or Scenes are selectable at a given time.
[`] - toggle between the four sections "DEVICES", "GROUPS", "SCENES" and "INSTALL CODE".
The section title of the current selection mode is highlighted with a brighter white, and
marked with [].
- [UP/DOWN] - select a device,
[LEFT/RIGHT] - select an endpoint/group/scene
- [SPACE] toggles between the available actions,
[+/-] changes the value associated with the action,
[ENTER] performs the selected action.
- Each action has a quick access key associated with it (underlined in the action name);
Press a quick access key to select an action and perform it immidiately.
- To issue a Bind/Unbind the action needs to be executed twice:
First it will select the binding source device. Second it will select the binding destination
and perform the action.
if the selected binding destination is the same as the source, the operation is canceled.
- Actions that requires destination address will use unicast, groupcast or broadcast
addressing,depending on the current addressing mode (binding and device removal
always use unicast)
- [A/a] - Automatically read data from all sensors periodically.
- [x] - Perform a soft reset.
- [X] - Perform a hard reset.
- [K/k] - Shutdown the gateway subsystem.
- [O/o] - Initiate OTA operation.
- [B/b] - Enable/Disable Broadcast addressing. (When enabled, 'B' appears on the actions
title line.)
- [*] - Enable/Disable immediate execution. (When enabled, '*' appears on the actions title
line.)
When immediate execution is enabled, actions will be triggered as soon as [+/-] is
pressed, after the respective value is updated.
- [[] - Record a single-command macro
- []] - Assign a a recorded macro
- [1] - Display less log lines
- [2] - Display more log lines
- [3] - Display less device lines, but more log lines
- [4] - Display more device lines, but less log lines
- [=] - Redraw user interface according to actual terminal dimentions
```

# <a name="using-the-web-application"></a>
# Using the Web Application
The Zigbee web application allows the user to control various aspects of the Zigbee Network and view/control various devices that are commissioned to the network.
This section of the guide will walk through how various tasks can be completed for network control as well as guidelines for all current compatible devices.

**Dropdown Menu Features**:

The following features can be found when one opens the Dropdown Menu.
This can be opened by clicking the three horizontal lines in the upper right hand corner of the web application.

![Dropdown Menu](/resources/webapp_dropdown_menu.png)

**Network Information**:

The Dropdown Menu displays the current network information. 
The Channel is the current channel that the coordinator is operating on. 
When connecting devices, make sure they have permissions to connect to this channel. 
The Pan ID (Personal Area Network ID) is the unique identifier of the current Zigbee Network.

![Network Information](/resources/nwk_information.png)

**Open/Close Network**:

This option allows the user to toggle the ability to allow devices to join the network.
This button can be accessed in the Dropdown Menu, by pressing the three horizontal lines in the upper right hand corner.
When a user clicks the green button "Open Network", this will send a command to the Linux Gateway to allow devices to join the Zigbee Network for 180 seconds.
When a user clicks the red button "Close Network", this will send a command to the Linux Gateway to not allow devices to join the Zigbee Network.

![Open/Close Network](/resources/open_close_nwk.png)

**Soft System Reset**:

When a user clicks this button, the Linux Gateway will perform a soft system reset. 
This will reset all the Linux Gateways. 
The ZNP device and all devices that were previously on the network will remain connected to the network. 
The device cards on the web interface will remain as well. 
When a user clicks this button, the Linux Gateway will perform a soft system reset. 
This will reset all the Linux Gateways. 
The ZNP device and all devices that were previously on the network will remain connected to the network. 
The device cards on the web interface will remain as well. 

![Soft System Reset](/resources/soft_system_reset.png)

**Hard System Reset**:

When a user clicks this button, the Linux Gateway will perform a hard system reset. 
This will reset all the Linux Gateways, the ZNP device, clear all devices that were previously on the network, and clear all device cards from the web interface. 
Once clicked, a popup will appear informing the user to wait until the reset has completed. 
A separate popup will appear when the reset has been successful. 
If this second popup does not appear after 1 minute, refresh the page and try to perform the hard reset again. 

![Hard System Reset](/resources/hard_system_reset.png)

**Device Cards**:

Each device that joins the network is displayed in a Device Card on the web interface. 
Device Cards create an organized list of all devices on the network, while displaying controls and data that can customized for each device type.

Each device card includes a few common elements. First is the Device Type description. 
This is the bold text in the header section of the card. 
The Device Type displays what type of device has joined the network. 
The number next to the Device Type is the Device ID. This is a unique ID that is composed of the IEEE address and endpoint number of the device. Each ID is unique to each device on the network. The "X" button in the upper right hand corner of each Device Card will remove the Device from the Zigbee Network and will also remove the Device Card from the web interface. 

![Common Device Card Characteristics](/resources/common_device_card_characteristics.png)

The next sections further detail each device card.

**Temperature Sensor**:

The temperature sensor card displays the latest temperature attribute reported from a temperature sensor end device. 
The progress bar at the top of the card displays the latest temperature. 
The plot will display the last 20 temperature values reported. 
The figure will automatically scale its vertical axis to fit all data plotted. 

There are two buttons:

- Poll Temp Sensor 
    - Button will immediately poll the temperature sensor device and get the latest temperature value.
- Toggle Plot
    - Button will toggle the visibility of a plot of the temperature.

![Temperature Sensor](/resources/temperature_sensor.png)

**On/Off Light**:

The On/Off Light Device Card displays the current state and allows control of a connected On/Off Light device. 
The light bulb image indicates whether the current status of the light is on or off. 
If the light is on, the light bulb image will be yellow. If the light is off, the light bulb image will be white. 
There are two buttons that allow the user to control the on/off state of the light. 

- Light On 
    - The gateway will send a command to turn the light on, even if the light device is already on. 
- Light Off 
    - The gateway will send a command to turn the light off, even if the light device is already off. 

When the card is initially created, the webapp will send out a request to get the current state of the light. 
This includes the on/off status of the light. 
Once this data is received by the webapp, the light bulb image will be updated as necessary. 

![On/Off Light Device Card](/resources/on_off_light.png)

**Dimmable Light**:

The Dimmable Light Device Card displays the current state and allows control of a connected Dimmable Light device. 
The light bulb image indicates whether the current status of the light is on or off. 
If the light is on, the light bulb image will be yellow. If the light is off, the light bulb image will be white. 

There are two sliders located in the middle of the device card. 

- Set Level 
    - This allows the user to set the current level or brightness of the light. 
- Set Transition Time 
    - This allows the user to change the time duration the light takes to change brightness.
    - This will not adjust the time it takes the light to turn on or off. 

There are two buttons that allow the user to control the on/off state of the light. 

- Light On 
    - The gateway will send a command to turn the light on, even if the light device is already on. 
- Light Off 
    - The gateway will send a command to turn the light off, even if the light device is already off.

When the card is initially created, the webapp will send out a request to get the current state of the light. 
This includes the on/off status of the light as well as the current level the light is at. 
Once this data is received by the webapp, the light bulb image and sliders will be updated as necessary. 

![Dimmable Light Device Card](/resources/dimmable_light.png)

**Color Light**:

The Color Light Device card displays the current state and allows control of a connected Color Light device. 
The light bulb image indicates whether the current status of the light is on or off. 
If the light is on, the light bulb image will be yellow. 
If the light is off, the light bulb image will be white.

There are four sliders located in the middle of the device card. 

- Set Level 
    - This allows the user to set the current level or brightness of the light. 
- Set Transition Time 
    - This allows the user to change the time duration the light takes to change brightness.
    - This will not adjust the time it takes the light to turn on or off. 
- Set Hue Value 
    - This allows the user to adjust the hue of the compatible color light. 
- Set Saturation Value 
        - This sets the saturation value of the light with respect to the current hue value selected.

There are two buttons that allow the user to control the on/off state of the light.
    
-   Light On 
    - The gateway will send a command to turn the light on, even if the light device is already on.
-   Light Off 
    - The gateway will send a command to turn the light off, even if the light device is already off.

When the card is initially created, the webapp will send out a request to get the current state of the light. 
This includes the on/off status, current level, hue and saturation the light is at. 
Once this data is received by the webapp, the light bulb image and sliders will be updated as necessary.

![Color Light Device Card](/resources/color_light.png)

**Switch**:

The Switch Device card allows a user to bind compatible devices to the switch and then view devices that are bound to the switch. 
Once a switch has joined the network, Switch Device Card will appear with just a "Bind Devices" button. 

![Switch Device Card](/resources/switch.png)

When selected, a popup binding popup modal will appear with a list of compatible devices that can be bound the switch. 
Currently, this switch supports being bound to On/Off, Dimmable and Color Light devices. 
Within each item in the bind list, it will consist of the device type, the device ID, a button to test toggle the light device (to allow the user to visually determine which light they are binding) and a bind button to initiate the bind process.
When the Bind button is pressed, a progress bar will appear, indicating the user to wait for the bind process to finish. 
If successful, the device in the bind list will disappear and will then appear as a bounded item in the Switch Device Card. 
If the bind was not successful, the item will still be visible in the bind list. 
The user should try again if the bind failed the first time. 

![Switch Popup](/resources/switch_popup.png)

Once a device has been bound, it will appear in the Switch Device Card with the device type, device ID and a button to Unbind the device from the switch. 
The user should be able to press the physical switch button/user input method to control the device the switch is bound too. 
To unbind the device from the switch, the user may press the Unbind button. 
A progress bar will appear indicating that the unbind process is occurring. 
If successful, the previously bound item will disappear from the bound list. 
If the unbind was not successful, the item will remain in the list. 
The user should try to unbind again if it failed the first time.

![Switch Binds](/resources/switch_bind.png)

**Door Lock**:

The Door Lock Device Card displays the current state and controls a connected door lock device. 
The lock image indicates whether the door lock is currently locked or unlocked. 
The Pin input field allows a user to indicate a four digit pin. 
This pin is used as a method of security to allow only authorized users to control the door lock. 
Then there are two buttons that allow the user to either lock or unlock the door lock. 

![Door Lock Device Card](/resources/door_lock.png)

Once a command is sent to lock/unlock a door lock, a response is received indicating if the command was successful. 
If the Pin is entered incorrectly, a popup modal will appear informing the user that the pin was incorrect and the last door lock command sent was not successful. 
If this occurs, the lock image and button status will revert back to their previous state. 

![Door Lock Pin Invalid Popup](/resources/door_lock_pin_invalid_popup.png)

**Door Lock Controller**:

The Door Lock Controller card allows a user to bind and unbind compatible Door Lock devices to the Door Lock Controller. 
Once a door lock controller joins the network, the card will appear with just a Bind Devices button. 

![Door Lock Controller](/resources/door_lock_controller.png)

When selected, a popup binding popup modal will appear with a list of compatible devices that can be bound the switch. 
Currently, this door lock controller only supports being bound to door lock devices. 
Within each item in the bind list, it will consist of the device type, the device ID and a bind button to initiate the bind process. 
When the Bind button is pressed, a progress bar will appear, indicating the user to wait for the bind process to finish. 
If successful, the device in the bind list will disappear and will then appear as a bounded item in the Door Lock Controller Device Card. 
If the bind was not successful, the item will still be visible in the bind list. 
The user should try again if the bind failed the first time.

![Door Lock Controller Bind Popup](/resources/door_lock_controller_bind_popup.png)

Once a device has been bound, it will appear in the Door Lock Controller Device Card with the device type, device ID and a button to Unbind the device from the controller. 
The user should be able to press the physical switch button/user input method to control the door lock the controller is bound too. 
To unbind the device from the controller, the user may press the Unbind button. 
A progress bar will appear indicating that the unbind process is occurring. 
If successful, the previously bound item will disappear from the bound list. 
If the unbind was not successful, the item will remain in the list. 
The user should try to unbind again if it failed the first time.

![Door Lock Bound to Door Lock Controller](/resources/door_lock_bound_to_door_lock_controller.png)

# <a name="ibm-cloud-instructions"></a>
# IBM&reg; Cloud&reg; Instructions

The Zigbee Sensor to Cloud is a complete solution for interfacing a TI SimpleLink Zigbee network with any Cloud infrastructure; allowing full remote control, monitoring and interaction from anywhere in the world. 
The objective of this section is to cover basic instructions for setting up and running the IBM Cloud sample application included with the Zigbee Sensor to Cloud package.

For more details on the architecture and modules, please see the [Node.js Zigbee Gateway - Cloud Connected High Level Architecture  ](Z-Stack Linux Gateway - Developer's Guide.html#nodejs-cloud-connectivity) section of the Developer's Guide.

**Install Cloud Foundry CLI**

1. Download and install Cloud Foundry CLI from [https://github.com/cloudfoundry/cli#downloads](https://github.com/cloudfoundry/cli#downloads)
* Verify installation by typing the following command in the system terminal:
    - $ cf  
    ![Cloud Foundry CLI](/resources/cloud_foundry_cli.png)

# <a name="create-ibm-cloud-account"></a>
**Create IBM Cloud Account**

1. If you do not already have one, you will need to open and configure an IBM Cloud Account.
* Create an [IBM Cloud Account](https://console.bluemix.net/) and register for a 30-day trial account.
* Confirm the IBM Cloud account using the link provided by IBM through email.
* Log into the IBM Cloud account and navigate to the "Dashboard".  
![IBM Cloud Dashboard](/resources/ibm_cloud_dashboard.png)

# <a name="create-ibm-watson-iot-platform-service"></a>
**Create IBM Watson IoT Platform Service**

1. In the upper right hand corner, click Create resource. This will navigate you to the catalog page.  
![IBM Cloud Catalog Page](/resources/ibm_cloud_catalog_page.png)  
* In the left hand menu, under **Platform**, select **Internet of Things**.  
![IoT Platform](/resources/iot_platform_menu.png)
* Now select *Internet of Things Platform*.  
![IoT Platform](/resources/iot_platform.png)
* For this guide, use **myZigbeeIotService** as the Service name, though any name may be used. 
The service name will be used later on in this guide. 
Enter this **Service Name** in the Service Name field. The Lite pricing plan should be pre-selected and is free for trial users. Click **Create**.  
![Creating IoT Service](/resources/creating_iot_service.png)
* Select **Launch**.  
![Launch the Watson IoT Platform Service](/resources/launch_watson_iot_platform_service.png)
* From the right hand menu, select **Devices** and click **Add Device** in the upper right hand corner.  
![Add Device to the Watson IoT Platform Service](/resources/add_device_iot.png)
* From the **Device Types** tab, click **Add Device Type** button.  
![Create a Device Type before you create a Device](/resources/iot_device_type.png)
* Select the **Gateway** choice in the **Type** field.  
For this guide, *zigbeeGateway* will be used as the **Gateway Device Type**, though any name can be used. 
Enter *zigbeeGateway* as the **Name** and add an optional description in the **Description** field.
Select **Next** to continue.  
![Screenshot of Creating Gateway Device Type](/resources/iot_gateway_device_type.png)
* You may enter additional device information if desired. Click **Done** to create the device type.  
* Navigate back to the **Browse** tab. Click **Add Device**.  
![Adding a Device to the Watson IoT Platform Service](/resources/iot_adding_device.png)
* From the **Select Existing Device Type** field, select *zigbeeGateway*. Enter a name for the gateway device in the Device ID field. For this guide, enter *myZigbeeGatewayDevice*. Click **Next**.  
![Screenshot of Creating Device ID](/resources/creating_device_id.png)
* You may enter additional device information if desired. Click **Next** to continue.  
* Either enter an authentication token or let one be automatically generated. 
For this guide we will enter one manually.  
Enter *authToken1234* in the **Authentication Token** field. Click **Next** and then **Done** to create the device.  
![Screenshot of Creating Authentication Token](/resources/creating_authentication_token.png)
* Verify and make note of all the **Device Credentials**. Click **Back** to return to the IBM Watson IoT Platform dashboard.  
![Verify the Device Credentials](/resources/verify_device_credentials.png)

**Upload Web App to IBM Cloud Foundry**

1. Navigate to **source/Projects/cloud_apps/ibm-webapp**.
* Using a text editor, edit the **manifest.yml** file.
* Select a **name** and unique hostname (**host**). 
The **host** will be used to generate a unique link to your cloud application. 
You will be able to tell if your selected hostname is already used in step 5. 
For this guide, **name** and **host** will be *myZigbeeWebApp*. 
Add your service name (*myZigbeeIotService*) to **services** using the service name from step 4 of 
[Create IBM Watson IoT Platform Service](#create-ibm-watson-iot-platform-service).  
![Screenshot of Editing the manifest.yml file](/resources/editing_manifest_yml.png)  
* Open a system terminal and navigate to the **source/Projects/cloud_apps/ibm-webapp** directory.  
Enter the command, `cf api https://api.ng.bluemix.net`.  
Then use the `cf login` command and login with your account credentials created in 
[Create IBM Cloud Account](#create-ibm-cloud-account).  
* After successfully logging in, use `cf push` to upload the application. 
Note that if a unique hostname is not used, an error message will be generated. 
In that case, return to the **manifest.yml** and modify the host, then try again.  
* Navigate back to your [IBM Cloud Dashboard](https://console.bluemix.net/dashboard/apps). 
Here you will see the web app has been started. Click on the web app name in the table.  
![Launchpad Web App running in IBM Cloud Foundry](/resources/launchpad_web_app_running_in_cloud.png)  
* In the left hand side, click on **Connections**. 
The service (*myZigbeeIotService*) should be a visible connection. If not, use **Create connection** to add the service.  
![Creating Connection Between the IoT Service and the Web App](/resources/iot_create_connection.png)  
* From the service option menu (three vertical dots), click **View credentials**. 
This will display the necessary credentials needed to connect the webapp to the IoT service. Make note of these credentials.  
![View Credentials to Connect to the IoT Service](/resources/iot_view_credentials.png)  

# <a name="enter-iot-service-credentials-to-ibm-web-app"></a>
**Enter IoT Service Credentials to IBM Web App**

1. Navigate to **source/Projects/cloud_apps/ibm-webapp**.
* Using a text editor, edit the **ibmConfig.json** file using the guidelines shown below.  
![Screenshot of Entering the IoT Service Credentials to the ibmConfig.json File](/resources/entering_iot_service_credentials_ibmConfig_json.png)
* Save the changes and use `cf push` to update the web app.  

**Configure the Node.js Zigbee Gateway to connect the IBM IoT Service**

1. Navigate to **source/Projects/node/nodejs_zb_gateway/cloud_adapters**.
* Using a text editor, edit the **ibmConfig.json** file.  
Use the same **org** value used in step 2 of [Enter IoT Service Credentials to IBM Web App](#enter-iot-service-credentials-to-ibm-web-app).  
Use the same auth-token used in step 13 of [Create IBM Watson IoT Platform Service](#create-ibm-watson-iot-platform-service).  
* Upload the new version of this file over to your BeagleBone Black.

**Run Node.js Zigbee Gateway with IBM Cloud Connection**

1. Run the IBM gateway:
    - `sudo ./start_ibm.sh`
* This will launch the Linux Zigbee Gateway and Node.js Zigbee Gateway with an IBM Cloud Connection. Wait for the scripts to start and then proceed to the next step.

**Launch IBM Hosted Web App**

1. Back in the IBM Cloud Foundry app page, click **Visit App URL** to open the webapp.   
![Screenshot to View the IBM Hosted Web App](/resources/to_view_ibm_hosted_web_app.png)
