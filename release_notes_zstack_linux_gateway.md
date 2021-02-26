# Zigbee Linux Gateway 3.0 Release Notes


## Introduction
The Zigbee Linux Gateway is a complete software development suite which can be used to develop a Zigbee gateway solution. It features an abstracted socket API for application developers, to access the Zigbee functionality, implemented through a set of Linux-based servers. In this release, a Node.js powered example application is introduced to facilitate control of a Zigbee Network through Cloud infrastructure. 


## What's New
- Updated CC2652/CC1352P2/CC1352R1 ZNP images for production-ready (Rev E) silicon.
- Updated/combined overall documentation into [Quick Start Guide](Documents/Z-Stack Linux Gateway - Quick Start Guide.html), [User's Guide](Documents/Z-Stack Linux Gateway - User's Guide.html), and [Developer's Guide](Documents/Z-Stack Linux Gateway - Developer's Guide.html).
- Supports Linux 64-bit OS.


## Fixed Issues
- Function `zcl_nv_actions` now uses the correct length for `memcpy` (in `zcl_port.c`).


## Known Issues
- Occasional incoming data reports are not received by Node.js Zigbee Gateway.
- ZNP server occasionally fails at startup.


## Upgrade Compatibility Information
- Node.js requires minimum Linux distribution version Debian 9.3 for BeagleBone Black devices.


## Operating System Support
- Linux Debian 9.3 on BeagleBone Black (ARM)
- Ubuntu 18.04 64-bit (x86)


## Dependecies
- Node 6.3 or later
- dos2unix
- killall
- protobuf v1
- protobuf-c v1
- gcc version:
    - Debian 6.3.0
    - Ubuntu 7.3.0


## Device Support
- [BeagleBone Black](https://beagleboard.org/black)
- [SimpleLink CC2652R](http://www.ti.com/product/CC2652R)
- [SimpleLink CC1352R](http://www.ti.com/product/CC1352R)
- [SimpleLink CC1352P](http://www.ti.com/product/CC1352P)
- [CC2530](http://www.ti.com/product/CC2530)
- [CC2531](http://www.ti.com/product/CC2531)
- [CC2538](http://www.ti.com/product/CC2538)


## Development Board Support
- [CC26X2R1 LaunchPad](http://www.ti.com/tool/LAUNCHXL-CC26X2R1)
- [CC1352R1 LaunchPad](http://www.ti.com/tool/LAUNCHXL-CC1352R1)
- [CC1352P2 LaunchPad](http://www.ti.com/tool/LAUNCHXL-CC1352P)
- [CC2530](http://www.ti.com/tool/cc2530emk)
- [CC2531 USB Dongle](http://www.ti.com/tool/cc2531emk)
- [CC2538 EM](http://www.ti.com/tool/CC2538DK)