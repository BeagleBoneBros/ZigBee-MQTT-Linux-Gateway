To get Protoc Compiler from Linux repositories

This guide assumes an out-of-the-box Beagle Bone Black, some steps may be skipped if the devices already has updated its versions.
The following steps are done to upgrade to Debian 9, in order to obtain the right protoc compiler from Linux repositories.

1.//Replace all "jessie" with "stretch"
	_> sed -i 's/jessie/stretch/g' /etc/apt/sources.list

2.//Checking the outcome by
	_> more /etc/apt/sources.list

3.//Update the repository list
	_> apt-get update

4.//System upgrade (no packages will be removed at this stage) 
	_> apt-get upgrade

5.//Update to stretch version
	_> apt-get dist-upgrade

6.//reboot the system
	_> systemctl reboot

	
To install protoc compiler 1.2.1-1 use the following commands.
	_> apt-get install protobuf-c-compiler

This will work on any Debian linux distribution or any distribution that has apt-get (included Beable Bone Black). To do this be sure you have internet access (for configuring your Beagle Bone to have internet access refer to: http://beagleboard.org/getting-started)
	
Once this package is installed, proceed to generate the structures/functions to manage protobuffers in .c/.h files using protoc compiler.
	_> protoc-c --c_out=.  gateway.proto
	
The resulting files must be placed into the gateway source code:

File: gateway.proto
path: Zigbee_Linux_Gateway-3.0.0\source\Projects\zstack\linux\hagateway	
	
file: nwkmgr.proto	
path: Zigbee_Linux_Gateway-3.0.0\source\Projects\zstack\linux\nwkmgr	
	
file: otasrvr.proto
path: Zigbee_Linux_Gateway-3.0.0\source\Projects\zstack\linux\otaserver

file: server.proto
path: Zigbee_Linux_Gateway-3.0.0\source\Projects\zstack\linux\serverpb

file: zstack.proto	
path: Zigbee_Linux_Gateway-3.0.0\source\Projects\zstack\linux\zstackpb


The protoc compiler can also be downloaded directly from Google's git repository following the readme from Google's git repository in the following link:
https://github.com/google/protobuf/blob/master/src/README.md
