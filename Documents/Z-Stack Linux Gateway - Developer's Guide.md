# Z-Stack Linux Gateway - Developer's Guide

This document dives deeper into the architecture and inner components of the Z-Stack Linux Gateway, Gateway Sample application, and the Node.js Zigbee Gateway.

* [Architecture of Gateway](#architecture-of-gateway)
    * [Gateway Servers](#gateway-servers)
    * [Gateway Application](#gateway-application)
* [Node.js Zigbee Gateway](#nodejs-zigbee-gateway)
    * [Local Webserver](#nodejs-local-webserver)
    * [Cloud Connectivity](#nodejs-cloud-connectivity)

# <a name="architecture-of-gateway"></a>
# Architecture of Gateway

The following block diagram shows a high level overview of the key components of the Z-Stack Linux Gateway.  
<img src="/resources/block diagram.png" alt="Gateway Block Diagram" width="750"/>

A typical gateway application needs to communicate only with the servers at the top-level i.e., Network
Manager, HA Gateway and OTA Upgrade servers. These servers, in turn, communicate with the servers
below them via TCP socket. A shell script called zigbeeHAgw is used to bring up the various servers and
track their status.
This section provides details about each of these blocks. 
Users are encouraged to familiarize themselves with each of the blocks before proceeding.

# <a name="gateway-servers"></a>
## Gateway Servers

Following is a brief description of the servers that the application connects to directly.

### HA Gateway Server 
The Gateway server provides APIs for communicating with individual or groups of devices, in order
to control them, read/change their attributes etc. Chapter 9 of the Z-Stack Linux Gateway API
document provides a complete list of APIs supported by the Gateway Server.

Also, it provides an encapsulation of the procedures related to the way devices are accessed
and maintained (as virtual objects) in Zigbee. This encapsulation includes:

- **Endpoint registration:** Automatic registration with the underlying Z-Stack Server and Zigbee core stack as an application endpoint.
- **Simple Descriptor:** Automatic creation of the Simple Descriptor from a configuration file (gateway.tlg).
- **Attribute Table:** Automatic creation of the attribute table from an attribute configuration file (gateway.tlg).

A simplified Home Automation API is provided to user applications over a TCP socket.

### Network Manager Server
The Network Manager server provides support for management of devices on the network using
Zigbee stack services provided by the Z-Stack Server. An application could leverage the APIs exposed
by this server, for creating and maintaining a Zigbee network, acquiring and displaying device
information, network status etc. For a complete list of APIs implemented by the Network Manager
Server, please refer to the Z-Stack Linux Gateway API document Chapter 8.

This server encapsulates management of the Zigbee devices on the network.
Also, it is responsible for automatically creating and maintaining a device database
that contains information for all devices in the Zigbee network.

An API is provided to user applications over a TCP socket.

### OTA Upgrade Server
The OTA Upgrade Server provides firmware upgrade services for devices in the Gateway's Zigbee
network, using the Zigbee stack services provided by the Z-Stack Server. 
It provides APIs to manage device upgrade images, and initiate/control the actual upgrade process over-the-air. 
Chapter 10 of the Z-Stack Linux Gateway API document provides a complete list of APIs supported by the OTA Upgrade Server.

An API is provided to user applications over a TCP socket that allows maintenance of device upgrade
images and control over the air operations.

### Note on Application-accessible Servers
The HA Gateway Server, Network Manger Server and OTA Upgrade Server have some interdependencies and must be brought up in a fixed order. 
A shell script called zigbeeHAgw is provided to bring up all the servers in the correct order with the correct parameters. 
This script then monitors these three servers, and also performs important functions with regards to Power Down, Sleep, Wakeup and Reset.
Details on how this script specifically handles Power down, Reset and Sleep functionality are provided in
[Power and Reset Commands section](#power-and-reset-commands).

### Z-Stack Server
The Z-Stack Server provides the core Zigbee stack interface. It provides an API for services that implement
the Zigbee AF (Application Framework) and ZDO (Zigbee Device Object) layers.

The Network Manager, HA Gateway, and OTA Upgrade servers all communicate with the Z-Stack Server
over TCP sockets, in a manner transparent to the end-user. Z-Stack Server communicates over a serial
interface, via the NPI Server, with the Zigbee ZNP device.

### NPI Server
The NPI (Network Processor Interface) Server provides a socket-based communication channel to the
Network Processor that abstracts the physical serial communication mechanism, eg. UART.

### ZNP
The ZNP is the Z-Stack&trade; Zigbee Network Processor. This contains the core Zigbee stack, configured as a
network coordinator, and the ZNP serial interface software module.

The CC13x2/CC26x2 ZNP may be built using the znp project 
provided in the SimpleLink&trade; [CC13x2/CC26x2 SDK](http://www.ti.com/tool/simplelink-cc13x2-26x2-sdk), using the pre-defined symbols specified in the README in the Firmware folder.


# <a name="gateway-application"></a>
## Gateway Application

The Gateway application communicates with the HA Gateway Server, Network Manager Server and OTA Server to provide the functionality required by the end-user.

<img src="/resources/gateway application.png" alt="Gateway Application Block Diagram" width="500"/>

The application connects to all 3 servers: Network Manager Server, Gateway Server and OTA Upgrade
Server.

The Framework block manages connection and communication with the various servers. It sends out
messages to the servers over TCP socket connections and also receives messages from them. It also has a
component that provides the UI for this application. Details on how to connect and send/receive messages
to/from the various servers, and UI management can be found in the source code for this block.

The Framework block calls into the Engines block to service different requests from the User and/or Devices.
Once the Framework block has identified which engine to call in response to a User action or response from
a ZigBee device in the network, it calls the respective function in the Engines block to act on it. Details
regarding how to call an API from the application or how to interpret an incoming message from the Server
can be found in the source code for this block.

The Z-Stack Linux Gateway offering includes a C-based sample Gateway application that is a good starting point for
anyone trying to develop their own application. The example, included in the release along with source code,
performs basic Gateway functions such as creating the network and adding/controlling common Zigbee
devices. It also demonstrates other interesting Zigbee functionality such as creating Groups and Scenes for
devices connected to the network.

The [Using the Local Gateway Sample Application section](Z-Stack Linux Gateway - User's Guide.html#using-the-local-gateway-sample-application) of the Z-Stack Linux Gateway User's Guide provides several details about the sample application,
and has instructions on how to exercise its various features. Further sections of this guide will refer to source
code in the sample application, the reader is encouraged to install and play around with the sample
application and familiarize themselves with the code layout etc. The location of the Gateway application
sources is &lt;INSTALL&gt;/Source/Projects/zstack/linux/demo and will henceforth be referred to as &lt;GATEWAY
SAMPLE APP&gt;.

Please note that this sample application is for demonstration purposes only, and is not intended to be a
reference implementation.

# <a name="interface-to-gateway-servers"></a>
### Interface to Gateway Servers

The Z-Stack Linux Gateway APIs are defined and implemented using a serialization protocol known as
Protocol Buffers or Protobuf. Protocol Buffers are a language-independent and platform-neutral way of
serializing structured data for efficient data communication.
Typically, APIs are expressed using Protocol Buffers notation and terminology in one or more .proto files.
These files can then be compiled into one of several languages supported by Protobuf using a specialized
Protobuf compiler. This generates source code that provides data access classes, simple accessors for
each field within each message, as well as methods to serialize and parse the whole message structure
to/from raw bytes.
A list of current languages supported and download instructions for Protobuf are available [here](https://github.com/protocolbuffers/protobuf). 

**Protobuf usage in Z-Stack Linux Gateway**

APIs for the three servers that need to be accessed by the Gateway application (HA Gateway, Network
Manager and OTA servers) are defined in their corresponding .proto files (gateway.proto, nwkmgr.proto, otasrvr.proto). An application writer can compile
these proto files into one of the languages supported by Protobuf by downloading the appropriate language
and compiler support. You can find copies of the proto files for the three servers in the Proto_files directory.

The Z-Stack Gateway servers, as well as the sample application are written in C and hence they use a C
implementation of the Protobuf format, called Protobuf-c. Running the Protobuf-c compiler on the (above)
proto files, generates C source and header files that are built into the servers, and will also be required to be
built into the application if it is written in the same language. Details and download instructions for Protobuf-c
are available [here](https://github.com/protobuf-c/protobuf-c).

The 'C' code generated by the Protobuf-c compiler requires an "engine" to run. This is essentially a library of
encoding and decoding routines which also needs to be linked with any executable built with Protobuf
generated C code. The Z-Stack Linux Gateway package provides a copy of the libprotobuf-c engine, at this
location:  
*&lt;INSTALL&gt;/source/Projects/zstack/linux/protobuf-c/lib*

The Protobuf-c generated files for the 3 servers are also included with the release:

- Network Manager
    - &lt;INSTALL&gt;/source/projects/zstack/linux/nwkmgr/nwkmgr.pb-c.c
    - &lt;INSTALL&gt;/source/projects/zstack/linux/nwkmgr/nwkmgr.pb-c.h
- HA Gateway
    - &lt;INSTALL&gt;/source/projects/zstack/linux/hagateway/gateway.pb-c.c
    - &lt;INSTALL&gt;/source/projects/zstack/linux/hagateway/gateway.pb-c.h
- OTA Server
    - &lt;INSTALL&gt;/source/projects/zstack/linux/otaserver/otasrvr.pb-c.c
    - &lt;INSTALL&gt;/source/projects/zstack/linux/otaserver/otasrvr.pb-c.h

Note, the libprotobuf-c libraries and the Protobuf-c generated .c/.h files are useful only if you intend to
implement your application in "C" like the Gateway sample application.

# <a name="protobuf-enc-dec-example"></a>
**Protobuf Encoding/Decoding Example**

Whenever an application sends a command to one of the servers, it needs to first call a fixed set of functions
to "Protobuf-encode" the message. Similarly, when it receives a command from the server, it needs to call a
fixed set of functions to decode it.

For instance, say your application wants to send a command to the Gateway to determine the status of the
network. There's a Network Manager API for it called NWK_ZIGBEE_NWK_INFO_REQ (Z-Stack Linux
Gateway API document, Section 8.2.2). Your application would then construct a message to send to the
Network Manager server as shown here:

*&lt;GATEWAY SAMPLE APP&gt;/engines/network_info-engine.c*:
<pre>
    <code>
void nwk_send_info_request(void)
{
    pkt_buf_t * pkt = NULL;
    uint8_t len = 0;
    <span style="background-color: yellow">NwkZigbeeNwkInfoReq msg = NWK_ZIGBEE_NWK_INFO_REQ__INIT;</span>

    UI_PRINT_LOG("nwk_send_info_request: Sending NWK_INFO_REQ__INIT");
    
    <span style="background-color: yellow">len = nwk_zigbee_nwk_info_req__get_packed_size(&msg);</span>
    pkt = malloc(sizeof(pkt_buf_hdr_t) + len); 

    if (pkt)
    {
        pkt->header.len = len;
        pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
        pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_REQ;

        <span style="background-color: yellow">nwk_zigbee_nwk_info_req__pack(&msg, pkt->packed_protobuf_packet);</span>
    </code>
</pre>

The highlighted lines above show the three APIs that are called to initialize a message object (&lt;&gt;__INIT),
determine its length (&lt;&gt;__get_packed_size), and then pack or encode it (&lt;&gt;_req__pack) into the
actual message that is then sent across the socket interface. Of course, there are several lines of code in
between, where the memory for the message is allocated, and the message is populated according to the
parameters defined in the API document. You also need to free any memory allocated for the message,
after it has been sent over the socket connection.

Similarly, every time a command is received from a particular server, APIs will need to be called to unpack
and decode the message. In the same file look at the following lines that demonstrate how a response to the
message above is handled:

<pre>
    <code>
void nwk_process_ready_ind(pkt_buf_t * pkt)
{
    <span style="background-color: yellow">NwkZigbeeNwkReadyInd *msg = NULL;</span>

    if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_READY_IND)
    {
        return;
    }

    UI_PRINT_LOG("nwk_process_ready_ind: Received NWK_ZIGBEE_NWK_READY_IND");

    <span style="background-color: yellow">msg = nwk_zigbee_nwk_ready_ind__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);</span>

    if (msg)
    {
        ds_network_status.state = ZIGBEE_NETWORK_STATE_READY;
        ds_network_status.nwk_channel = msg->nwkchannel; 
        ds_network_status.pan_id = msg->panid;
        ds_network_status.ext_pan_id = msg->extpanid;
        ds_network_status.permit_remaining_time = 0x0;
        ds_network_status.num_pending_attribs = 0x0;

        <span style="background-color: yellow">nwk_zigbee_nwk_ready_ind__free_unpacked(msg, NULL);</span>
    </code>
</pre>

These types of pack/unpack functions are called every time a command is sent or received by the Gateway
application. All the highlighted functions are declared and defined in the Protobuf-c compiler generated files
for the Network Manager, namely nwkmgr.pb-c.h and nwkmgr.pb-c.c. Generated C-files are included for all the
servers that the application will communicate with.

**Packet Header Information**

The actual data packet sent over the application interface includes more than just the Protobuf-encoded
message (seen above). A 4-byte header is expected to precede the Protobuf-packed message, and it needs
to include the following information:

- **len**: 16-bit number that specifies the length (in bytes) of the protobuf-packed packet.
- **Subsystem**: 1-byte ID of subsystem to/from which the packet is sent/received. It can be one of the following:
    - **18**: Network Manager Server
    - **19**: Gateway Server
    - **20**: OTA Server
- **cmd_id**: 1 byte command ID of the actual command being sent. This value is also available
inside the packed message. The actual command ID numbers are provided in the
protobuf definition files (.proto files) that are part of the Z-Stack Linux Gateway
package. When using command IDs in your code, always use the defined names
(never hardcode the command ID numbers), as the numbers may change between
releases.

Reviewing the same file we did before, the highlighted lines below show memory being allocated for the
encoded message *and* this 4-byte header, and then the fields of the header being populated before the
packet is sent to the socket interface function:

<pre>
    <code>
    len = nwk_zigbee_nwk_info_req__get_packed_size(&msg);
    <span style="background-color: yellow">pkt = malloc(sizeof(pkt_buf_hdr_t) + len); </span>

    if (pkt)
    {
        <span style="background-color: yellow">pkt->header.len = len;</span>
        <span style="background-color: yellow">pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;</span>
        <span style="background-color: yellow">pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_REQ;</span>

        nwk_zigbee_nwk_info_req__pack(&msg, pkt->packed_protobuf_packet);

        if (si_send_packet(pkt, (confirmation_processing_cb_t)&nwk_process_info_cnf, NULL) != 0)
        {
    </code>
</pre>

As mentioned in the packet-header description above, all the constants used to populate the packet header,
come from the Protobuf-generated header file for the Network manager server:

&lt;INSTALL&gt;/source/Projects/zstack/linux/nwkmgr/nwkmgr.pb-c.h:  
<pre>
    <code>
/*
 * MT System ID for Network Manager Protobuf sub-system ID
 */
typedef enum _ZStackNwkMgrSysIdT {
  <span style="background-color: yellow">Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR</span> = 18
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(Z_STACK_NWK_MGR_SYS_ID_T)
} ZStackNwkMgrSysIdT;
/*
 * Command IDs - each of these are associated with Request, Responses, Indications, and Confirm messages
 */
typedef enum _NwkMgrCmdIdT {
  NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF = 0,
  NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_RSP_IND = 1,
  NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_RESET_REQ = 2,
  NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_RESET_CNF = 3,
  NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_SELF_SHUTDOWN_REQ = 4,
  NWK_MGR_CMD_ID_T__NWK_SET_ZIGBEE_POWER_MODE_REQ = 5,
  NWK_MGR_CMD_ID_T__NWK_SET_ZIGBEE_POWER_MODE_CNF = 6,
  <span style="background-color: yellow">NWK_MGR_CMD_ID_T__NWK_GET_LOCAL_DEVICE_INFO_REQ</span> = 7,
  NWK_MGR_CMD_ID_T__NWK_GET_LOCAL_DEVICE_INFO_CNF = 8,
  NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_READY_IND = 9,
  NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_REQ = 10,
    </code>
</pre>

Similar values for subsystem IDs and command IDs can be found in the respective Protobuf- generated
headers of the other servers as well.

# <a name="overview-of-gateway-application"></a>
### Overview of Gateway Application

This section attempts to breakdown a typical Gateway Application into its essential functional blocks and
goes into detail in each one.

Code snippets referred to in subsequent sections will be from the sample Gateway application and in the "C"
programming language. However the concepts covered should be applicable to any language in which you
choose to implement your application.

# <a name="connection-to-the-servers"></a>
#### Connection to the Servers

Before an application can send and receive commands to/from the servers, it needs to connect to them. The
Zigbee gateway software supports one active client application at a time, and that application should be
connected to all the three servers for proper operation. Let's take a look at how a typical Gateway
application might do that.

All the three servers (Network Manager, HA Gateway and OTA Upgrade) have separate TCP socket
connections and service access point interfaces. Typically, an application would need the IP address and
port number for each of the servers it needs to connect to. The servers all have a fixed port number they use,
and this number is displayed for each server when the servers are launched (via the zigbeeHAgw script).
The function in charge of making and then maintaining a connection to the servers, should use this
information (IP address and port number) to create a connection to each of the servers. You can refer to the
following functions for an example of how to populate the server details and create a socket connection:

&lt;GATEWAY SAMPLE APP&gt;/framework/tcp_client.c:
<pre>
    <code>
int tcp_new_server_connection(
    server_details_t * server_details, char * hostname, u_short port, 
    server_incoming_data_handler_t server_incoming_data_handler, char * name,
    server_connected_disconnected_handler_t server_connected_disconnected_handler)

int tcp_connect_to_server(server_details_t * server_details)
    </code>
</pre>
In the above functions, typical calls to Socket APIs such as `connect()` and `socket()` are made to establish
connection with each of the servers. The file descriptors returned are used to populate the `server_details`
data structures shown below. This structure also contains pointers to callback functions to handle socket
connection/disconnection and incoming data:

&lt;GATEWAY SAMPLE APP&gt;/framework/socket_interface.c:  
<pre>
    <code>
server_details_t network_manager_server;
server_details_t gateway_server;
server_details_t ota_server;
    </code>
</pre>
With the structure defined as follows:  
&lt;GATEWAY SAMPLE APP&gt;/framework/tcp_client.h:  
<pre>
    <code>
typedef struct
{
    struct sockaddr_in serveraddr;
    server_incoming_data_handler_t server_incoming_data_handler;
    <span style="background-color: yellow">int fd_index;</span>
    tu_timer_t server_reconnection_timer;
    char * name;
    server_connected_disconnected_handler_t server_connected_disconnected_handler;
    bool connected;
    int confirmation_timeout_interval;
    uint8_t layerNumber;
} server_details_t;
    </code>
</pre>

The variable fd_index points to a single entry into an array of descriptors. Each entry holds information about
a particular server connection such as the file descriptor that represents the socket connection, callback
functions/handlers that handles incoming data on the socket. A lot of these details are specific to the sample
application's implementation, and can/should be modified suitably for the user's own application.

An important requirement for the Gateway application is to constantly monitor server connection status. The
application should expect to lose connection with the servers in certain scenarios, such as when a System
Reset command is issued by the application (via the NWK_ZIGBEE_SYSTEM_RESET_REQ command; for
details, see Z-Stack Linux Gateway API document - Section 8.1.1). An application should constantly attempt
to reconnect to the servers whenever it detects a disconnection.

In the same file referenced above, you can look at how a disconnect event is handled in the sample
application in the following functions:

&lt;GATEWAY SAMPLE APP&gt;/framework/tcp_client.c:
<pre>
    <code>
void tcp_socket_event_handler(server_details_t * server_details)
{
    //...
    
    <span style="background-color: yellow">else if (remaining_len == 0)</span>
    {
        UI_PRINT_LOG("Server %s disconnected", server_details->name);
        <span style="background-color: yellow">close(polling_fds[server_details->fd_index].fd);</span>
        polling_undefine_poll_fd(server_details->fd_index);
        server_details->connected = false;
        
        if (server_details->server_connected_disconnected_handler != NULL)
        {
            <span style="background-color: yellow">server_details->server_connected_disconnected_handler();</span>
        }

        ui_redraw_server_state();
        <span style="background-color: yellow">tcp_socket_reconnect_to_server(server_details);</span>
    }

    //...
    </code>
</pre>

In the event of a disconnection with a particular server, the above function closes the socket connection,
calls a 'connection/disconnection handler', which is a callback function registered specifically for that server,
and then immediately attempts to connect back to the server. The connection handler could do things like
clearing out internal saved state, refresh display etc. You can look at the connection handler for all the
servers here:  
&lt;GATEWAY SAMPLE APP&gt;/framework/socket_interface.c:  
<pre>
    <code>
void nwk_mgr_server_connected_disconnected_handler(void)
void gateway_server_connected_disconnected_handler(void)
void ota_server_connected_disconnected_handler(void)
    </code>
</pre>

In most cases the zigbeeHAgw script brings the servers back up in case one or more of them exit. But in certain
situations, for instance when the application issues a shutdown (via the NWK_ZIGBEE_SELF_SHUTDOWN
command), the servers will not come back up, and it is up to the application to handle that scenario.

#### Sending Commands to the Servers

Once connection is established with the servers, the next step is to send commands to the servers and
process received commands. Details of all the commands that can be sent to/received from the servers are
available in the Z-Stack Linux Gateway API document. Details of the C-structures that are used to represent
these individual commands are in their corresponding Protobuf generated header files.

In our sample application, after establishing the connection to the servers, the state machine of the
application is initiated and the first command is sent to the network manager to obtain network status
information: NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_REQ. This is handled in the following
function called up the application:  
&lt;GATEWAY SAMPLE APP&gt;/engines/network_info_engine.c:  
`void nwk_send_info_request(void)`

We have already covered details of how the protobuf message is initialized and packed [here](#protobuf-enc-dec-example). The
structure of the entire packet that is sent over the socket connection is here:  
&lt;GATEWAY SAMPLE APP&gt;/framework/types.h:  
<pre>
    <code>
typedef struct
{
    uint16_t len;
    uint8_t subsystem;
    uint8_t cmd_id;
} pkt_buf_hdr_t;

typedef struct
{
    pkt_buf_hdr_t header;
    uint8_t packed_protobuf_packet[];
} pkt_buf_t;
    </code>
</pre>

In `pkt_buf_t`, the first field is the packet header that is populated with information regarding the length of the
protobuf message, the subsystem (or server) for which this message is intended, and it's unique command
ID. Then the protobuf message is encoded or packed into the `packed_protobuf_packet` array that follows the
header:  
&lt;GATEWAY SAMPLE APP&gt;/engines/network_info_engine.c: 
<pre>
    <code>
void nwk_send_info_request(void)
{
    pkt_buf_t * pkt = NULL;
    uint8_t len = 0;
    NwkZigbeeNwkInfoReq msg = NWK_ZIGBEE_NWK_INFO_REQ__INIT;

    UI_PRINT_LOG("nwk_send_info_request: Sending NWK_INFO_REQ__INIT");
    
    len = nwk_zigbee_nwk_info_req__get_packed_size(&msg);
    pkt = malloc(sizeof(pkt_buf_hdr_t) + len); 

    if (pkt)
    {
        <span style="background-color: yellow">pkt->header.len = len;</span>
        <span style="background-color: yellow">pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;</span>
        <span style="background-color: yellow">pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_REQ;</span>

        <span style="background-color: yellow">nwk_zigbee_nwk_info_req__pack(&msg, pkt->packed_protobuf_packet);</span>

        <span style="background-color: yellow">if (si_send_packet(pkt, (confirmation_processing_cb_t)&nwk_process_info_cnf, NULL) != 0)</span>
        {
            UI_PRINT_LOG("nwk_send_info_request: Error: Could not send msg");
        }
        
        free(pkt);
    }
    </code>
</pre> 
The last highlighted line shows the call made to send the packet over the socket interface. The
`si_send_packet` function has the following signature:  
&lt;GATEWAY SAMPLE APP&gt;/framework/socket_interface.c:
<pre>
    <code>
int si_send_packet(pkt_buf_t * pkt, confirmation_processing_cb_t _confirmation_processing_cb, void * _confirmation_processing_arg)
    </code>
</pre>  
The first argument is a pointer to the packet itself, the second is a call back function for processing the
confirmation for the message being sent, and the third is the optional argument for this function. Whenever a
request-type command is sent to a server, a confirmation message is sent back to the application by the
server in a synchronous manner. The gateway application is not supposed to send out a new outgoing
request until it receives the confirmation for the previous request.

In the `si_send_packet()` function, the header is first checked to determine which server the message is
meant for, the server details for that server are then extracted, and then (if connection is still intact) the
following function is called to send the packet over the respective connection:  
&lt;GATEWAY SAMPLE APP&gt;/framework/tcp_client.c:
<pre>
    <code>
int tcp_send_packet(server_details_t * server_details, uint8_t * buf, int len)
{
    if (write(polling_fds[server_details->fd_index].fd, buf, len) != len)
    {
        return -1;
    }

    return 0;
}
    </code>
</pre>  
The server that receives this message expects to receive a 4 byte header followed by the actual protobuf
encoded message. The length parameter supplied when making the `write()` call includes the length of the
protobuf message as well as the (4 byte) header.

After sending the packet over the socket interface, the `si_send_packet()` function sets up some internal state
variables to indicate that the application is currently waiting for a confirmation for the command it just sent
out. Note: the application will not (and should not) process any other commands till it receives the
confirmation for the one just sent out. A timer is used in this application to timeout if the confirmation isn't
received within a stipulated time period.

#### Handling Incoming Commands from the Servers

There are several different kinds of messages that an application may receive from the servers. It could
receive a confirmation message for a command it recently sent out, it could receive a response for a
command sent out earlier that solicited some information from a Zigbee device on the network, or it could
receive an unsolicited incoming indication due to some network activity. (Details on the different kind of
messages supported by the servers are available in the Z-Stack Linux Gateway API document, Chapter 4).
In the sample application, all these different types of incoming messages are handled in the same way. Your
application may choose to handle them differently.

In steady state, the sample application polls for activity on the set of file descriptors that represent the
various connections to the servers using the poll() call. Whenever it receives data (corresponding to a
received message) on one of the descriptors, it calls the socket event handler for that particular descriptor.
In the sample application, this function handles incoming messages on the socket interface:

&lt;GATEWAY SAMPLE APP&gt;/framework/tcp_client.c:  
`void tcp_socket_event_handler(server_details_t * server_details)`

This function calls `recv()` on the file descriptor corresponding to the socket connection, and then calls the
corresponding handler function and passes it the entire packet. The handler functions for all the servers are
available here:  
&lt;GATEWAY SAMPLE APP&gt;/framework/socket_interface.c  
<pre>
    <code>
void si_nwk_manager_incoming_data_handler(pkt_buf_t * pkt, int len)
void si_gateway_incoming_data_handler(pkt_buf_t * pkt, int len)
void si_ota_incoming_data_handler(pkt_buf_t * pkt, int len)
    </code>
</pre>  
These functions all look at the command ID in the header of the received packet, and then decide how to
handle it. Going back to the example in the last section, where the application sends a
NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_REQ to the Network Manager server, it immediately
receives a confirmation response from it. The `si_nwk_manager_incoming_data_handler()` (listed above)
knows to call the function that was registered to handle this confirmation, i.e., `nwk_process_info_cnf()`.In this
function, the message is unpacked/decoded as described [here](#protobuf-enc-dec-example). The status of the received
response is extracted from the message, and can be used to update the internal state of the application
and/or refresh the UI display.

&lt;GATEWAY SAMPLE APP&gt;/engines/network_info_engine.c:  
<pre>
    <code>
void nwk_process_info_cnf(pkt_buf_t * pkt, void * cbarg)
{
    <span style="background-color: yellow">NwkZigbeeNwkInfoCnf *msg = NULL;</span>

    if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_CNF)
    {
        return;
    }

    UI_PRINT_LOG("nwk_process_info_cnf: Received NWK_ZIGBEE_NWK_INFO_CNF");

    <span style="background-color: yellow">msg = nwk_zigbee_nwk_info_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);</span>

    if (msg)
    {
        UI_PRINT_LOG("msg->status = %d", msg->status);

        /* Update network info structure with received information */
        <span style="background-color: yellow">if (msg->status == NWK_NETWORK_STATUS_T__NWK_UP)</span>
        {
            ds_network_status.state = ZIGBEE_NETWORK_STATE_READY;
            ds_network_status.nwk_channel = msg->nwkchannel; 
            ds_network_status.pan_id = msg->panid;
            ds_network_status.ext_pan_id = msg->extpanid;
    </code>
</pre>

#### Handling User Input

Once a Gateway application comes up and has established network status etc, it can then start to accept
input from user to start interacting with the network and/or devices.

The sample application is a terminal application with a menu-based UI that accepts user input via short-cut
keys, or by navigating to the desired item (via arrow keys) and then selecting it using the return key. Most of
the UI handling is done in the following functions:  
&lt;GATEWAY SAMPLE APP&gt;/framework/user_interface.c:  
<pre>
    <code>
int ui_init(char * log_filename)
void console_event_handler(void * arg)
    </code>
</pre> 
The function `ui_init()` initializes the UI for the application, and the `console_event_handler()` handles user input.

As an example, one of the first commands that a user is expected to send after starting the application, is to
open the network up for Zigbee devices to join (via the NWK_SET_PERMIT_JOIN_REQ command, see ZStack
Linux Gateway API document - Section 8.2.4). As an argument to this function, the user also
specifies the amount of time for which to keep the network open. You can see the `console_event_handler()`
function handle this user event:  
&lt;GATEWAY SAMPLE APP&gt;/framework/user_interface.c: 
<pre>
    <code>
switch (current_action)
{
    case ACTION_PRMT_JOIN:
        comm_send_permit_join(action_value[current_action]);
        break;
    </code>
</pre>  
In the `comm_send_permit_join()` function the corresponding Protobuf message is constructed and sent over
the socket interface:  
&lt;GATEWAY SAMPLE APP&gt;/engines/commissioning_engine.c:  
<pre>
    <code>
requested_join_time = joinTime;

nwk_set_permit_join_req__pack(&msg, pkt->packed_protobuf_packet);

if (si_send_packet(pkt, &<span style="background-color: yellow">comm_process_permit_join</span>, NULL) !=0 )
    </code>
</pre>

In the confirmation callback function comm_process_permit_join() (in the same file), an internal data
structure ds_network_status is updated and a timer is set up that is triggered every second.  
<pre>
    <code>
    <span style="background-color: yellow">msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);</span>   

    if (msg)
    {
        <span style="background-color: yellow">if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)</span>
        {
            UI_PRINT_LOG("comm_process_permit_join: Status SUCCESS.");

            <span style="background-color: yellow">ds_network_status.permit_remaining_time = requested_join_time;</span>

            UI_PRINT_LOG("comm_process_permit_join: Requested join time %d",
            requested_join_time);

            if ((requested_join_time > 0) && (requested_join_time < 255))
            {
                <span style="background-color: yellow">tu_set_timer(&pj_timer, 1000, true, &comm_permit_join_timer_handler, NULL);</span>
            }
    </code>
</pre>

In the timer callback function (`comm._permit_join_timer_handler()`), display data is updated, and the
`ui_refresh_data()` function is called. On every refresh call, `ds_network_status` is read along with other data
structures to update the UI display:  
&lt;GATEWAY SAMPLE APP&gt;/engines/commissioning_engine.c:  
<pre>
    <code>
void comm_permit_join_timer_handler(void * arg)
{
    if ((ds_network_status.permit_remaining_time == 0) || (ds_network_status.permit_remaining_time == 255))
    {
        tu_kill_timer(&pj_timer);
    } 

    <span style="background-color: yellow">ui_refresh_display();</span>

    if ((ds_network_status.permit_remaining_time > 0) && (ds_network_status.permit_remaining_time < 255))
    {
        <span style="background-color: yellow">ds_network_status.permit_remaining_time--;</span>
    }
}
    </code>
</pre>

On careful inspection of the sample application code, you will note that the application does not translate
user input into an actual command to the server unless it has received a confirmation for a previously sent
command. Only if a confirmation has been received, or a timeout has occurred, will new user input be
processed (See `si_send_packet()` in &lt;GATEWAY SAMPLE APP&gt;framework/socket_interface.c).

The way user input is handled is very specific to the actual application implementation. A real-world
application is expected to have a lot more complexity than the sample application included with this release.

# <a name="typical-api-flow"></a>
#### Typical API Flow: User, Application and Device Interaction

Let's assume the servers are up and running (using the `zigbeeHAgw` or `start_gateway` script), and the application has
successfully connected to the servers (as detailed [here](#connection-to-the-servers)). 
This section walks through the typical flow of APIs from
Gateway application to Gateway servers and vice versa during different stages of the application's lifecycle.
The application should be equipped to send and receive these commands and handle them appropriately as
described in the sections above.

Legend:
-&gt; (Outgoing) Command sent by the Gateway application to one of the Servers.
&lt;- (Incoming) Message/Indication received by the application, sent to it by one of the Servers.

**Note:** The list below omits the Generic Confirmations that are sent in response to most commands. Details
of the APIs and their arguments are available in Z-Stack Linux Gateway API document. This section refers
to them by name, and also includes a brief description.

**At Startup:**  
List of APIs that might be called by a Gateway application at start-up, before accepting any user input.
<!--
@startuml

participant "Gateway Application"
participant "Gateway Servers"

"Gateway Application"->"Gateway Servers": NWK_ZIGBEE_NWK_INFO_REQ
    note right
        Called by the application to request status of the network.
    end note

"Gateway Application"<-"Gateway Servers": NWK_ZIGBEE_NWK_INFO_CNF
    note right
        Response to the above API, informing the application whether the network is up and running yet or not.
    end note

"Gateway Application"<-"Gateway Servers": NWK_ZIGBEE_NWK_READY_IND
    note right
        Informs the application of the status of the newly formed
        network. Includes information about the network such as the
        PAN ID, and the channel ID. This information can be saved
        and used to update the UI.
    end note

"Gateway Application"->"Gateway Servers": NWK_GET_DEVICE_LIST_REQ
    note right
        Called by application to get a list of devices already in the
        network. Is called once network readiness is established by
        previous messages.
    end note

"Gateway Application"<-"Gateway Servers": NWK_GET_DEVICE_LIST_CNF
    note right
        Confirmation for above command, contains list of devices
        Currently part of network, and their details. Will be empty the
        first time network is created.
    end note

"Gateway Application"->"Gateway Servers": NWK_DEVICE_LIST_MAINTENANCE_REQ
    note right
        This is a request to discover services for one more devices
        in the network.
    end note

"Gateway Application"<-"Gateway Servers": NWK_ZIGBEE_DEVICE_IND
    note right
        Informs the application of a new device having joined,
        or removed from the network, or if device details have changed
        (in response to message above).
        Lets the application present the list of actions that can be
        performed on available devices in the network.
    end note

@enduml
-->
![API Flow at Startup](/resources/typical_api_at_startup.png)

**On User Input:**  
List of APIs that the Gateway application may call in response
to user-requests.
<!--
@startuml

participant "Gateway Application"
participant "Gateway Servers"

"Gateway Application"->"Gateway Servers": NWK_SET_PERMIT_JOIN_REQ
    note right
        On user-input, application can instruct Network manager to
        open the network up for new devices to join.
    end note

"Gateway Application"<-"Gateway Servers": NWK_ZIGBEE_DEVICE_IND
    note right
        Informs the application that a new device has joined.
        Information returned can be used to update the UI with the new
        device that joined the network.
    end note

"Gateway Application"->"Gateway Servers": DEV_SET_LEVEL_REQ
    note right
        On user-input, application can call Gateway manager to set a
        level-control on one more more devices in network.
    end note

"Gateway Application"->"Gateway Servers": DEV_GET_LEVEL_REQ
    note right
        Application can call this Gateway manager API to request
        current level of one/more devices. Can be used to update display.
    end note

"Gateway Application"<-"Gateway Servers": DEV_GET_LEVEL_RSP_IND
    note right
        Informs the application of current level of a device, application
        can use this information to update display.
    end note

"Gateway Application"->"Gateway Servers": DEV_SET_ONOFF_STATE_REQ
    note right
        On user-prompt application can send this command to switch a
        device on or off.
    end note

"Gateway Application"->"Gateway Servers": DEV_GET_POWER_REQ
    note right
        Command used to request the power readings of a device.
    end note

"Gateway Application"<-"Gateway Servers": DEV_GET_POWER_RSP_IND
    note right
        Response from the server to the application with the power
        value and/or status.
    end note

"Gateway Application"->"Gateway Servers": NWK_SET_BINDING_ENTRY_REQ
    note right
        On user-input, application can call Network manager to request
        a binding to be made between two remote devices such as a
        light and a switch.
    end note

"Gateway Application"<-"Gateway Servers": NWK_SET_BINDING_ENTRY_RSP_IND
    note right
        Response received by the application that conveys the status
        of the binding request.
    end note

"Gateway Application"->"Gateway Servers": GW_ADD_GROUP_REQ
    note right
        On user-input, application may send request to Gateway
        server to associate one/more with a group id.
    end note

"Gateway Application"->"Gateway Servers": GW_STORE_SCENE_REQ
    note right
        Application can call this Gateway API to associate a
        particular scene for a group of devices, with an ID.
    end note

"Gateway Application"->"Gateway Servers": GW_RECALL_SCENE_REQ
    note right
        On user-input, application can issue this request to recall
        the scene associated with a particular ID.
    end note

"Gateway Application"->"Gateway Servers": GW_SET_ATTRIBUTE_REPORTING_REQ
    note right
        Application can use this API to configure reporting of attributes
        for a particular device in the network. The list of attributes, and
        interval at which to send the report can be specified as
        argument.
    end note

"Gateway Application"<-"Gateway Servers": GW_SET_ATTRIBUTE_REPORTING_RSP_IND
    note right
        Response received for the call above, with status. Information
        returned can be used to update display.
    end note

"Gateway Application"->"Gateway Servers": OTA_UPDATE_IMAGE_REGISTERATION_REQ
    note right
        Whenever a new upgrade image is available for one/more
        devices, application can send this command to register it with
        the OTA server.
    end note

"Gateway Application"->"Gateway Servers": OTA_UPDATE_ENABLE_REQ
    note right
        Application can enable OTA Upgrades using this command.
    end note

"Gateway Application"<-"Gateway Servers": OTA_UPDATE_ENABLE_CNF
    note right
        Response received by the application.
    end note

"Gateway Application"->"Gateway Servers": NWK_ZIGBEE_SYSTEM_RESET_REQ
    note right
        On user-input the application can send a hard or a soft reset
        request to the Network Manager that causes all the servers to
        reset.
    end note

"Gateway Application"<-"Gateway Servers": NWK_ZIGBEE_SYSTEM_RESET_CNF
    note right
        Confirmation sent to the application when the servers come
        back up again. Please note that the application will lose
        connection with the servers after the RESET_REQ, and before the
        response is received.
    end note

@enduml 
-->
![API Flow on User Input](/resources/typical_api_on_user_input.png)

**Steady State or Device Activity:**  
Incoming APIs to the application due to device activity and
corresponding responses.
<!--
@startuml

participant "Gateway Application"
participant "Gateway Servers"

"Gateway Application"<-"Gateway Servers": NWK_ZIGBEE_DEVICE_IND
    note right
        Informs application that a device has joined/been removed/has
        changed its properties etc. Can be used to update the display of
        devices and their information.
    end note

"Gateway Application"<-"Gateway Servers": GW_ATTRIBUTE_REPORTING_IND
    note right
        Informs application of attribute values for devices for which
        reporting was configured. Would typically be used to update display
        and/or take user-defined action.
    end note

"Gateway Application"<-"Gateway Servers": GW_ALARM_IND
    note right
        Informs application that an alarm has been generated for a particular
        cluster. The application would need to identify the reason for alarm and
        notify the user accordingly.
    end note

"Gateway Application"<-"Gateway Servers": DEV_ACE_ARM_REQ_IND
    note right
        Informs application that an ACE device has been armed.
    end note

"Gateway Application"->"Gateway Servers": DEV_ACE_ARM_RSP
    note right
        Application response to the ACE device which sent the arm request.
    end note

"Gateway Application"->"Gateway Servers": NWK_SET_ZIGBEE_POWER_MODE_REQ
    note right
        Application can send this command to the Network Manager to
        put the Network in "SLEEP" mode. This same API is used to
        "WAKE" it up when required.
    end note

"Gateway Application"<-"Gateway Servers": NWK_SET_ZIGBEE_POWER_MODE_CNF
    note right
        Response from Network manager to the above call.
    end note

"Gateway Application"<-"Gateway Servers": OTA_UPDATE_DOWNLOAD_FINISHED_IND
    note right
        Informs application that a particular device has finished upgrading
        to the new image hosted by it.
    end note

@enduml
-->  
![API Flow Steady State or Device Activity](/resources/typical_api_steady_activity.png)

**Wind Down:**  
APIs called during shutdown of Gateway application.
<!--
@startuml

participant "Gateway Application"
participant "Gateway Servers"

"Gateway Application"->"Gateway Servers":
    note right
        Called by the application to shut down the Gateway subsystem.
        Servers are disconnected in response.
    end note

@enduml
-->  
![API Flow Wind Down](/resources/typical_api_wind_down.png)

Please note, that this is just a representative set of APIs that you might expect an application to call/receive
from the Gateway servers. The complete list of APIs is documented in the Z-Stack Linux Gateway API
document.

#### Sequence Numbers

For each command mentioned in the Z-stack Linux Gateway API document, information is also included
about the kind of messages the servers generate in response to them.  
For example:  

- **NWK_ZIGBEE_NWK_INFO_REQ**  
*Command type*: **No-response command** / **NWK_ZIGBEE_NWK_INFO_CNF**

The commands are first classified as 
*Outgoing* (Application -&gt; Gateway subsystem) or *Incoming* (Gateway subsystem -&gt; Application), 
and then sub-classified by the type of responses they elicit from the servers (Details in API document, Chapter 4).

As mentioned before, a Gateway application can receive both synchronous messages ("Confirmation
message"), in response to commands it sends out, and asynchronous messages ("Responses" or
"Indications"), due to network activity or due to servers collating information from multiple over-the-air calls
before replying back to the application. Each outgoing command is immediately followed by a confirmation
message in a synchronous manner. Afterward it may or may not receive an asynchronous Response from
the server, depending on the type of command. In order to allow the application to easily correlate an
incoming Response with a message it sends out, the APIs support a parameter called sequence number.

If a command sent out by an application is expected to generate an incoming response, then the
confirmation message it receives from the server (immediately after sending out the command) includes a
'sequence number'. An incoming indication that arrives later in response to this command also includes this
same sequence number. This sequence number is a rolling 16-bit unsigned integer that can be used to
correlate incoming indications with the commands that are sent out. An application could 'remember' the
sequence number received as part of the confirmation message, and use it to process the incoming
response/indication that might come several seconds or commands later.

The sample application provides an example of how these sequence number might be tracked. Consider a
scenario where a user might want to switch a Zigbee light device ON or OFF, and present the current state
of the device on the UI. The application can respond to user-action by sending a
DEV_SET_ONOFF_STATE_REQ command (Z-Stack Linux Gateway API document, Section 9.7.2) to the
Gateway server. This command will be responded to by a "Confirmation message" from the Gateway server
that includes a sequence number. Eventually a Generic Response Indication is sent by the server which will
include status information regarding this command and also the same sequence number. Information in this
indication can be used to update the UI. 
The code snippet below shows the function `act_set_on_off_cnf()`,
the callback function called when confirmation for the DEV_SET_ONOFF_STATE_REQ command is
received:  

&lt;GATEWAY SAMPLE APP&gt;/engines/actions_engines.c:  
<pre>
    <code>
    <span style="background-color: yellow">msg = gw_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);</span>

    if (msg) 
    {
        if <span style="background-color: yellow">(msg->status == GW_STATUS_T__STATUS_SUCCESS)</span>
        {
            UI_PRINT_LOG("act_process_set_onoff_cnf: Status SUCCESS.");

            if (addr->ieee_addr != 0)
            {
                UI_PRINT_LOG("act_process_set_onoff_cnf: seq_num=%d", msg->sequencenumber);

                cluster_id = ZCL_CLUSTER_ID_GEN_ON_OFF;
                attribute_list[0] = ATTRID_ON_OFF;
                <span style="background-color: yellow">sr_register_attribute_read_request(msg->sequencenumber, addr, cluster_id, attribute_list, 1);</span>
            }
    </code>
</pre>

When a successful return status is received in the confirmation message, the function
`sr_register_attribute_read_request()` is called and the sequence number received in this confirmation is
passed to it along with other information about the command. As seen below, this function stores the
sequence number in a table, where it stays until a corresponding Response indication is received, or until
this table entry times out:

&lt;GATEWAY SAMPLE APP&gt;/engines/state_reflector.c:  
<pre>
    <code>
            pending_attribs[free_index].valid = true;
            <span style="background-color: yellow">pending_attribs[free_index].sequence_num = seq_num;</span>
            pending_attribs[free_index].ieee_addr = addr->ieee_addr;
            pending_attribs[free_index].endpoint_id = addr->endpoint;
            pending_attribs[free_index].cluster_id = cluster_id;
            pending_attribs[free_index].num_attributes = attr_num;
            <span style="background-color: yellow">pending_attribs[free_index].timer_val = READ_ATTR_TIMEOUT_VAL;</span>

            for (j = 0; j < attr_num; j++)
            {
                pending_attribs[free_index].attr_id[j] = attr_ids[j];
                UI_PRINT_LOG("sr_register_attribute_read_request: Adding attribute read for attr_id 0x%x", pending_attribs[i].attr_id[j]);
            }

            ds_network_status.num_pending_attribs++;

            if (ds_network_status.num_pending_attribs == 1)
            {
                <span style="background-color: yellow">tu_set_timer(&aging_timer, TIMER_CHECK_VAL * 1000, true, &aging_engine, 
                NULL);</span>
            }
    </code>
</pre>

The highlighted lines above show the sequence number along with other information being stored in a table,
and a timer being configured that allows these messages to eventually age and timeout. The timer ensures
that the table doesn't grow exponentially in case some devices stop responding and the user continues to try
and send messages to it. Entries will eventually time out, and make space for new entries.

Note that the application has only received a successful confirmation message so far, implying that the
server has successfully received this command. Eventually when it receives the
ZIGBEE_GENERIC_RSP_IND message (Z-Stack Linux Gateway API document, Section 7.1.2), the function
`sr_process_generic_response_indication()` (in the same file) tries to match the received sequence number
against all entries in this table. On finding a match, the application could take further action to update the
display. Saving the sequence number on receipt of the confirmation message allows the application to
correlate the received response with the command it sent out.

Note: In the sample application, the display isn't updated immediately after the receipt of the
ZIGBEE_GENERIC_RSP_IND. In fact, on finding a match for the sequence number, another request is sent
to read the attributes of the device in question. The display is updated only when a response to this "attribute
read" is received. The reason for this extra iteration of commands is that we want the UI of our application to
reflect the actual value of attributes of the device, and not blindly update it with the value that the user asked
it to update to. A study of the source code in the abovementioned file might make this part clearer.

#### Miscellaneous Considerations for Gateway Applications

This section goes through some application specific considerations that might be useful to keep in mind
when implementing your own Gateway application.

- Gateway application should always try to reconnect to the servers as soon as it detects a
disconnection. 
As soon as connection is reestablished to the Network manager it will be sent a
Reset confirmation message NWK_ZIGBEE_SYSTEM_RESET_CNF.
- The list of APIs supported by the various servers is substantial, but it doesn't cover all the ZCL APIs supported by Zigbee. 
However, the application can always send a raw ZCL frame over the air, using this API GW_SEND_ZCL_FRAME_REQ. 
Similarly when the Gateway subsystem receives an unprocessed ZCL frame, 
it sends it to the application using the following API GW_ZCL_FRAME_RECEIVE_IND.
- If your Gateway application needs to poll a particular device for its attributes, it should
configure reporting using these APIs: GW_SET_ATTRIBUTE_REPORTING_REQ,
GW_SET_ATTRIBUTE_REPORTING_RSP_IND, GW_ATTRIBUTE_REPORTING_IND. This
is useful for instance, if the application wants to report values from a temperature or humidity
sensor every few minutes or so. The Gateway sample application includes an example of
attribute reporting configuration for temperature/humidity/occupancy sensors (see <GATEWAY
SAMPLE APP>/demo/framework/user_interface.c: `ui_send_sensor_read()` ).
- As mentioned in the [Typical API Flow section](#typical-api-flow), NWK_GET_DEVICE_LIST_REQ API can be called by the
Gateway application at startup to obtain a list of devices and their information that has been
saved in the device database. However, the entries in this database may have become stale
since the last run, so the application should ideally call
NWK_DEVICE_LIST_MAINTENANCE_REQ API to clean up the device status returned by the first API.

# <a name="power-and-reset-commands"></a>
### Power and Reset Commands

The zigbeeHAgw script is responsible for bringing up all the servers in the correct order in order to setup and
create the Zigbee network. It also tracks all the servers and if any server other than the Network Manager
goes down, it will kill all the servers and bring them up again in the correct order. The network and device
databases remains intact, so it retains memory of all previously-added devices and their addresses etc. The
demise of the network manager is handled differently.

The Network manager supports 5 exit codes: OFF, RESET_HARD, RESET_SOFT, SLEEP, WAKEUP.
Whenever a power down (NWK_SET_ZIGBEE_POWER_MODE_REQ) or Reset
(NWK_ZIGBEE_SYSTEM_RESET_REQ) command is sent to the Network Manager, it exits with one of the
above codes after putting the Zigbee device in appropriate low-power state. The POWER and RESET
functionality of the Network manager is managed partially by the zigbeeHAgw script. The script keeps track
of the exit code with which the Network manager exits, and acts accordingly.

- In all cases, it first stops all the servers
- If the exit code is OFF, that means the intention was to wind down the system (NWK_ZIGBEE_SYSTEM_SELF_SHUTDOWN_REQ), 
so no more action is required.
- If Network manager exited due to a soft or hard reset or wakeup call, the servers are all restarted, but
network manager is sent a command line argument "--reset_soft", "--reset_hard" or "--wakeup" so it
can send appropriate confirmation message to the application after coming up.
- If the exit code is SLEEP, then the other servers are not brought up. Only the network manager is
brought up with a command line argument "--sleep". This puts the network manager in a mode, where it
only accepts the command to wake it up (NWK_SET_ZIGBEE_POWER_MODE_REQ), and doesn't
process any other commands coming from the Gateway application. When the manager receives the
wakeup command, it simply exits with a WAKEUP code, and lets the script handle the rest (see point
above).

A lot of the above is implementation detail, but if the user intends to edit/add functionality to the zigbeeHAgw
script, it is important to understand its role in correct operation of the servers. It also reiterates the
importance of the application to continually maintain connection with the servers, since it may lose contact
with them during the course of normal operation.

# <a name="nodejs-zigbee-gateway"></a>
# Node.js Zigbee Gateway

The Node.js Zigbee Gateway module is a JavaScript application made to run on the Node.js runtime engine. 
The gateway is developed for Node.js due to its high performance and capabilities in developing web applications.

The gateway supports either hosting a local webserver or interfacing to a Cloud Internet of Things service (ex. IBM Watson Internet of Things).

The Node.js Zigbee Gateway uses sockets to communicate with the Linux Zigbee Gateway. 
This allows the Node.js Zigbee Gateway to be hosted on a completely separate device if desired.
In other words, the Node.js Zigbee Gateway does *not* need to run on the same device which runs the Linux Zigbee Gateway; 
due to the cross platform compatibility of Node.js, the Node.js Zigbee Gateway can run on *any* OS with the Node.js runtime engine. 
To connect the Node.js Zigbee Gateway to the Linux Zigbee Gateway, 
simply change `ip_address` in **main.js** to match the IP address of the device on which the Linux Zigbee Gateway is running.

* [Local Webserver](#nodejs-local-webserver)
* [Cloud Connectivity](#nodejs-cloud-connectivity)
* [Components](#nodejs-zb-gw-components) 
* [Binding](#nodejs-zb-gw-binding)
* [Technical User Guide](#nodejs-technical-user-guide)

# <a name="nodejs-local-webserver"></a>
## Node.js Zigbee Gateway - Local Webserver High Level Architecture

This section will discuss the architecture and components of the Node.js Zigbee Gateway. 
Below is a block diagram of the gateway when hosting a local web application.

![Block Diagram of the Node.js Zigbee Gateway with a Local Webserver](/resources/nodejs_block_diagram.png)

### nwkmgr  
The nwkmgr (Network Manager) is a JavaScript module that runs as a subset of the Node.js Zigbee Gateway. 
This module is responsible for decrypting incoming protobuf packets from the Linux Zigbee Gateway regarding network management tasks. 
These tasks include but are not limited to handling the devices on the network, binding devices, adding and removing devices from the network. 
The nwkmgr communicates with the zb-gateway by emitting events through the JavaScript EventEmitter class. 
The zb-gateway can then call a set of functions defined in the nwkmgr to induce network commands.

### hagateway  
The hagateway (Home Automation Gateway) is a JavaScript module that runs as a subset of the Node.js Zigbee Gateway. 
This module is responsible for decrypting incoming protobuf packets from the Linux Zigbee Gateway regarding home automation device tasks.
These tasks include but are not limited to handling data received by sensors, sending commands or sending data to end devices. 
The hagateway communicates with the zb-gateway by emitting events through the JavaScript EventEmitter class. 
The zb-gateway can then call a set of functions defined in the hagateway to induce home automation commands.

### zb-gateway  
The zb-gateway (Zigbee Gateway) is a JavaScript module that runs as a subset of the Node.js Zigbee Gateway. 
This module is responsible for managing all flow of data through the Node.js Zigbee Gateway. 
It unifies all data and commands being sent between the nwkmgr, hagateway and webserver. 
All incoming data from the Linux Zigbee Gateway side is handled and formatted to be sent to the webapp. 
The zb-gateway has a simple to object oriented API that makes it simple to implement commands from a webserver. 
These function call are organized by either device types (ex. light device, door lock device, etc.) or network management function types (ex. binding). 
The zb-gateway also maintains knowledge of the network information and device list. 
Most importantly, the zb-gateway handles the binding process. 
It generates a list of all possible combinations of devices that can be bound and a list of devices that are currently bound.

### webserver  
The webserver is responsible for hosting a local web application and handling all data flow between the zb-gateway and the webapp. 
Events and data coming from the zb-gateway to the webapp are received by events by the webserver and then forwarded to the webapp through a socket connect. 
Events and data coming from the webapp to the zb-gateway are received as events by the webserver and then call zb-gateway functions as needed. 
The webapp is hosted on port 5000 by default at the devices IP address.

### Web App  
The local hosted web application is hosted on the webserver module on the Node.js Zigbee Gateway. 
The webapp is a combination of a single HTML page and JavaScript code. 
The sample webapp included can be hosted on a local webserver or a Cloud hosted runtime service (ex. IBM Cloud Foundry). 
The webapp included in the project is designed to be mobile and touch screen compatible for all modern web browsers. 
It is designed to by highly efficient. All changes that need to be displayed on the webapp are handled dynamically. 
For example, when the status of a light device has changed from ON to OFF, instead of the entire web app re-drawing, only the light element is updated.

# <a name="nodejs-cloud-connectivity"></a>
## Node.js Zigbee Gateway - Cloud Connected High Level Architecture  

This section will discuss the architecture and components of the Node.js Zigbee Gateway. Below is a block diagram of the gateway when connected to a Cloud hosted web application.

![Block Diagram of the Node.js Zigbee Gateway with a Cloud Connected Web Application](/resources/zigbee_gateway_with_cloud_block_diagram.png)

The functional descriptions of the *Linux Zigbee Gateway*, *nwkmgr*, *hagateway*, *zb-gateway* and *webapp* are identical as described 
in the previous section ([Node.js Zigbee Gateway - Local Webserver High Level Architecture](#nodejs-local-webserver)). 
The addition to the Cloud connected version of the *Node.js Zigbee Gateway* is the *cloudAdpater*. 
The second half of this setup requires a Cloud IoT service to be configuring with a Cloud provider. 
The final module is another Node.js application running in a Cloud container. 
This will be responsible for hosting the web application.

### cloudAdapter  
This module is responsible for sending all data from the zb-gateway to an Internet of Things cloud service. 
This is accomplished by converting all JavaScript objects to a JSON string and using 
the Cloud provider's libraries to send the data using the MQTT messaging protocol. 
The cloudAdapter is also responsible for receiving events from the Cloud IoT service. 
This is accomplished by first subscribing to specific events that are expected on the gateway. 
When a subscribed event occurs, the cloudAdapter will call an appropriate zb-gateway function to forward the command to data to the rest of the gateway.

### Cloud IoT Service  
The Cloud IoT service is a low bandwidth messaging service that allows messages from remote hardware devices to be sent to other Cloud services. 
In the Zigbee Sensor to Cloud scenario, a physical gateway device will be established as a device on the IoT platform. 
This gateway will then be authenticated to publish messages as this device. 
Other Cloud services, such as web applications, can then subscribe to particular messages or events.

### Cloud Hosted Node.js App  
In order to host the *webapp* in the Cloud, a separate *Cloud Hosted Node.js App* is necessary to handle messaging transactions between the IoT service and hosting the *webapp*. 
For simplicity and to reduce costs, the Node.js application can be hosted in an application runtime. 
This allows to easily deployed and scale apps in a Cloud platform. 
This service operates independently of the IoT service.

#### app  
The *app* is a JavaScript module running in the *Cloud Hosted Node.js App* that is responsible for authenticating and handling 
all messaging with the IoT service. 
The app subscribes to events from the IoT service and forwards them as is to the *cloudWebserver*. 
Messages coming from the *cloudWebserver* are received as events through the app. 
When the *app* receives on of these events, it will package and publish the message to the IoT service.

#### cloudWebserver  
The *cloudWebserver* is responsible for hosting the cloud based *webapp* and 
facilitating the messaging events between the *app* and *webapp*. 
It has a very similar structure and purpose to the local hosted *webserver*. 
This is designed this way to allow the both the local hosted and cloud hosted *webapp* to be identical in implementation.

### Web App  
The cloud hosted web application is hosted on through the *cloudWebserver* module on the *Node.js Zigbee Gateway*. 
The *webapp* is a combination of a single HTML page and JavaScript code. 
The sample *webapp* included can be hosted on a local webserver or a Cloud hosted runtime service (ex. IBM Cloud Foundry). 
The *webapp* included in the project is designed to be mobile and touch screen compatible for all modern web browsers. 
It is designed to by highly efficient. All changes that need to be displayed on the *webapp* are handled dynamically. 
For example, when the status of a light device has changed from ON to OFF, 
instead of the entire web app re-drawing, only the light element itself is updated.

# <a name="nodejs-zb-gw-components"></a>
## Node.js Zigbee Gateway - Components  

This section will cover the multiple components included in the Node.js Zigbee Gateway directory, *nodejs_zb_gateway*. 
All of these components are used to run the Node.js application so they are therefore written primarily in JavaScript.

### main.js  
This is the top level file included in the *nodejs_zb_gateway* directory. 
It is responsible for starting the gateway in the desired mode and define what IP address is being used to connect to the Linux Zigbee Gateway. 
From a command line argument, the user can define to either start 
the local hosted web application (localhost) or the IBM Cloud hosted web application (ibm). 
If no additional argument has been defined, the gateway will default to launch the local hosted web application.

### cloud_adapters/  
The *cloud_adapters* directory contains JavaScript based code to connect to a particular cloud IoT service. 
Currently, this only includes an adapter for IBM Cloud services, *ibmCloudAdapter.js*. 
This utilizes a Node.js API released by IBM to publish and subscribe messages to the IoT Service. 
To authenticate to the service, the proper credentials must be entered in the *ibmConfig.json* file.

### devices/  
The *devices* directory contains device specific modules that allow creation of devices within the Zigbee Network. 
The *device.js* module is responsible for evaluating what device type is going to be created based on a device ID and profile type. 
This will then call a particular device function that will create a JavaScript object with 
all necessary information regarding the device. 
All of these device functions are included in separate JavaScript files (ex. *doorlock-device.js*, *light-device.js*, etc.).

### linux_gw_adapters/  
The Linux Gateway Adapters directory includes adapter modules for receiving and sending Protobuf buffers between Linux Zigbee Gateway modules. 
The two modules included in this directory include *hagateway.js* and *nwkmgr.js*. 
They allow serial communication between the home automation gateway and network manager respectively. 
The ".proto" files included allow the incoming messages to be decoded and outgoing messages to be decoded to the defined Protobuf format.

### node_modules/  
This is a standard directory generated by *npm* (Node Package Manager). 
This directory will not appear until `npm install` is run (`npm install` is included in `setup.sh`).
This directory includes all modules that are referenced in the project. 
All of these modules are released as open source software packages, commonly under the MIT license. 
All necessary modules are included in the project. 
If for some reason, packages are deleted or corrupted, the modules can be downloaded again using the `npm install` command.

### webserver/  
This directory includes the webserver modules used to host a local based web application. 
This is accomplished by creating a web server and hosting the *webapp.html* located in the *public* directory. 
The *webserver.js* module creates a socket connection with the web application. 
This allows events and data to be sent between the two modules. 
It can also call functions defined by *zb-gateway.js* to implement desired commands from the web application's frontend.

### webserver/public/  
This directory includes all of the necessary components to run the web application. 
The web application is primarily centered around a HTML page (*webapp.html*) that instantiates JavaScript modules. 
The primary JavaScript modules include *webapp.js* and *zb_lib.js*. 
The *webapp.js* module is the primary code used to manage the web applications frontend. 
The *zb_lib.js* is a copy of the Zigbee Library module used throughout the project. 
This is used to reference standard enumerations and constants that are defined by the Zigbee specification. 
The *dist/* directory contains external libraries that are used by the web application. 
This includes necessary CSS files, images, jQuery libraries and various JavaScript libraries.

### zb_gateway/  
This directory hosts the Zigbee gateway module (*zb-gateway.js*). 
As previously described, it is the central module that is responsible for implementing 
all events through the network, managing connected devices and managing binding between devices. 
The module is instantiated by either the *webserver* or one of the *cloud_adapters* 
(depending on whether the user wants to implement a local web application or a Cloud hosted web application). 
It therefore can emit events back to the *webserver* or have functions called from the *webserver* to call other external modules.

### zigbee_library/  
This directory includes a module (*zb_lib.js*) that allows all standard Zigbee Specification defined enumerations and constants to be referenced. This includes but is not limited to status enumerations, data types and cluster IDs. It also includes a simple binding reference table (POSSIBLE_BINDING_TABLE) to define what devices can be bound to one another. All of the entries are defined as JavaScript objects. This allows all constants to be abstracted to structured object definitions. A copy of this Zigbee Library is also included in the *public* folder to be referenced by the *webapp*.

# <a name="nodejs-zb-gw-binding"></a>
## Node.js Zigbee Gateway - Binding

The Zigbee Gateway module (*zb-gateway*) is responsible for handling all bind events on the Zigbee Network. 
It will keep track of bound devices and generate tables of all possible bind combinations. 
These tables allow a developer to handle displaying this key information to the end user. 
The following diagram illustrates the logical process the zb-gateway follows to handle binding events.

![zb-gateway handling Binding Events](/resources/nodejs_binding_event_handling.png)

When a new device joins the Zigbee Network, the *zb-gateway* will check this device type with a 
possible binding table (POSSIBLE_BINDING_TABLE) declared in the Zigbee Library. 
If the newly joined device is declared in the table and a compatible binding pair is already on the network, 
the *zb-gateway* will then proceed to generate a table describing all binding pair combinations. 
This table uses the GUID of each device to define which devices can be bound to each other.

When a user requests a bind event, a pair of GUID's is required to identify which devices the bind will be between. 
The bind request is sent to the Linux Zigbee Gateway, which is then sent to the remote devices to be bound. 
When the bind has completed or failed, a response is received by the *zb-gateway*. 
If the bind event has failed, an event is sent to the webserver to inform the frontend that the bind event has failed. 
If the bind has been successful and the device pairs are both on the current Zigbee Network, 
then the *zb-gateway* proceeds to update the bound and unbound lists. 
These lists keep track of the device pairs that are currently bound or can be bound. 
If the incoming device pair is found in the unbound device list, 
then the pair will be removed from this list and added to the bound device list. 
If the pair is not in the unbound device list, then the bound list will be checked to see if the pair exists there. 
If the pair is in the bound device list, then the devices will be removed from the bound list and added to the unbound device list.

Please note, all of the actions completed by the gateway are not maintained when the Node.js Zigbee Gateway is restarted. 
Therefore, no information regarding currently bound devices will remain after restart.

# <a name="nodejs-technical-user-guide"></a>
## Node.js Zigbee Gateway - Technical User Guide

For more details on the APIs used within the Node.js Zigbee Gateway, please refer to its [API Guide](../source/Projects/node/nodejs_zb_gateway/API.html).

Sections pertaining to each of the .js modules are provided in the API Guide at these sections:

* [zb-gateway.js](../source/Projects/node/nodejs_zb_gateway/API.html#zb-gatway-js)
* [webserver.js](../source/Projects/node/nodejs_zb_gateway/API.html#webserver-js)
* [hagateway.js](../source/Projects/node/nodejs_zb_gateway/API.html#hagateway-js)
* [nwkmgr.js](../source/Projects/node/nodejs_zb_gateway/API.html#nwkmgr-js)
* [ibmCloudAdapter.js](../source/Projects/node/nodejs_zb_gateway/API.html#ibm-cloud-adapter-js)
