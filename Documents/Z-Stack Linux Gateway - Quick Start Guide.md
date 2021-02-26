# Z-Stack Linux Gateway - Quick Start Guide

This document aims to help the user quickly evaluate what the Z-Stack Linux Gateway offers.

* [Setting Up](#setting-up)
* [Running Gateway Out-of-Box](#running-gateway-out-of-box)
* [Running the Web Application](#running-the-web-application)

The Z-Stack&trade; Linux Gateway is a complete software development suite which can be used to develop a
Zigbee-to-IP gateway solution.
It features an abstracted socket API for application developers to access the Zigbee functionality, implemented through a set of Linux-based servers.
The Zigbee 3.0 profile is used.
The CC13x2/CC26x2 MCUs and the corresponding SimpleLink&trade; CC13x2/CC26x2 SDK (from 3.10 and onwards) are R22 certified.
The CC253x MCUs, TI's legacy Zigbee platform, are R21 certified.

## <a name="setting-up"></a>
## Setting Up
This section will walk through all of the hardware and software setup.

**Hardware Prerequisites**
The following hardware is required:

- Host Processor (eg. BeagleBone Black, Linux OS machine)
- 5V, 2A AC/DC power supply (if BeagleBone Black is used as host)
- microSD card (minimum 8Gb) (if BeagleBone Black is used as host)
- 2 or more Launchpads ([LAUNCHXL-CC1352R1](http://www.ti.com/tool/LAUNCHXL-CC1352R1), [LAUNCHXL-CC1352P](http://www.ti.com/tool/LAUNCHXL-CC1352P), [LAUNCHXL-CC26X2R1](http://www.ti.com/tool/LAUNCHXL-CC26X2R1))
- Ethernet Cable
- A-Male to Micro-Male USB Cable
- A-Male to Mini-Male USB Cable (if BeagleBone Black is used as host)
- Router with internet connection

**Software Prerequisites**
The following software/tools is required:

- Debian (if BeagleBone Black is used as host)
    - [9.3](http://debian.beagleboard.org/images/bone-debian-9.3-iot-armhf-2018-03-05-4gb.img.xz) or [later](http://beagleboard.org/latest-images/)
- Software Packages/Tools
    - dos2unix
    - killall
    - protobuf (optional)
- PC Software
    - [BeagleBone Black USB Drivers](http://beagleboard.org/getting-started) (if BeagleBone Black is used as host)
    - [PuTTY](https://www.putty.org/)
    - [Etcher](https://etcher.io/) or similar image SD card burning software (if BeagleBone Black is used as host)
    - [WinSCP](https://winscp.net/eng/index.php) (if BeagleBone Black is used as host)
    - [TI Flash Programmer 2](http://www.ti.com/tool/FLASH-PROGRAMMER) or [TI UniFlash](http://www.ti.com/tool/UNIFLASH)

**BeagleBone Black Setup**:

This section will detail how to setup the BBB as the host.

1. Download the Linux Debian Image for BeagleBone Black using the link above.
* Burn the Debian image to the microSD card using Etcher.
* With the BeagleBone Black powered down, insert the microSD card into the BeagleBone Black microSD port.
* Connect an Ethernet cable between the BeagleBone Black and an internet connected router.
* On the BeagleBone Black, press and hold down the Boot Button (S2) and insert the 5V power plug.
* Keep holding down the Boot Button until the LEDS start blinking (about 5-15 seconds).
* Make sure you have the latest BeagleBone Black USB drivers installed on your PC (follow instructions from http://beagleboard.org/getting-started for more information).
* Wait for the BeableBone Black to finish booting. You can tell when it's finished booting when you can open an SSH session (with Host Name/IP address as 192.168.7.2) with PuTTY.
* The default login is "debian", and the default password is "temppwd".
* Update and upgrade all packages.
    - `sudo apt-get update`
    - `sudo apt-get upgrade`
* Install killall tools.
    - `sudo apt-get install psmisc`
* Install dos2unix package.
    - `sudo apt-get install dos2unix`
* Ensure that the `source/scripts/package_builder` script has `TARGET_PLATFORM` set to `"BEAGLEBONE_BLACK"` (ie. `export TARGET_PLATFORM="BEAGLEBONE_BLACK"`) .
* If you are planning on updating Protobuf files please install Protobuf by following the steps below.
**NOTE: this is not required for initial setup or to try the included sample application**
    - `sudo apt-get install autoconf automake libtool curl make g++ unzip`
    - `apt-get install protobuf-c-compiler`
    - `git clone https://github.com/google/protobuf.git`
    - `cd protobuf`
    - `./autogen.sh`
    - `./configure`
    - `make`
    - `make check`
    - `sudo make install`
    - `sudo ldconfig # refresh shared library cache.`

**Linux OS setup**:

This section will detail how to setup a Linux OS (32-bit or 64-bit) as the host.
(This has been tested with Ubuntu 18.0.4 64-bit.)

1. Update and upgrade all packages.
    - `sudo apt-get update`
    - `sudo apt-get upgrade`
* Install killall tools (if not already).
    - `sudo apt-get install psmisc`
* Install dos2unix package (if not already).
    - `sudo apt-get install dos2unix`
* Install libprotobuf-c 32-bit libraries and cross compiler.
    - `sudo apt-get install libprotobuf-c-dev`
    - `sudo apt-get install libprotobuf-c-dev:i386`
    - `sudo apt-get install gcc-multilib`  
    **Note**: The 32-bit libraries and cross compiler are installed because the Gateway servers were designed for (and thus assume) a 32-bit machine.
* Copy libprotobuf-c libraries to the gateway's protobuf library directory:
    - Navigate to gateway installation home directory (eg. Zigbee_3_0_Linux_Gateway_1_0_0)
    - `cp /usr/lib/i386-linux-gnu/libprotobuf-c.a source/Projects/zstack/linux/protobuf-c/lib/x86`
    - `cp /usr/lib/i386-linux-gnu/libprotobuf-c.la source/Projects/zstack/linux/protobuf-c/lib/x86`
    - `cp /usr/lib/i386-linux-gnu/libprotobuf-c.so.1 source/Projects/zstack/linux/protobuf-c/lib/x86`
    - `cp /usr/lib/i386-linux-gnu/libprotobuf-c.so.1.0.0 source/Projects/zstack/linux/protobuf-c/lib/x86`
* Ensure that `source/scripts/package_builder` script has `TARGET_PLATFORM` set to `"x86"` (ie. `export TARGET_PLATFORM="x86"`).


**Linux Gateway Setup**:
The following instructions will cover setting up the Zigbee Gateway project.

1. Using WinSCP, connect to the BeagleBone Black. (Skip this step if using Linux OS machine.)
    * Copy the source/ folder to the BeagleBone Black home directory.
* On the host, navigate to the source/ directory.
    - `cd source/`
* Convert the setup script to Unix line endings and make executable. Then run the setup script. This will convert all necessary scripts to Unix line endings, will delete (if applicable) and rebuild the source Linux Gateway code, extract binaries, make files executable and install all Node Modules:
    - `sudo dos2unix setup.sh`
    - `sudo chmod +x setup.sh`
    - `sudo ./setup.sh`

**ZNP Setup**:
The following instructions will assist in programming one of the LaunchPads (LAUNCHXL-CC1352P, LAUNCHXL-CC1352R1, or LAUNCHXL-CC26X2R1) with a ZNP hex image.
Please see the [Firmware README](../Firmware/README.html#building-images) for how the images were built, what Zigbee channel the ZNP uses, etc.

1. Connect the LaunchPad to your PC.
* Open Flash Programmer 2 (or UniFlash).
* Erase all flash sections on the LaunchPad.
* Select the appropriate image for the Launchpad:
    - ZNP images are located in Firmware/znp/
    - For example, a CC26x2R1 Launchpad will use CC2652R1LP_GW_ZNP_UART.hex
* Flash this image to the LaunchPad.
* When flashed successfully, connect the LaunchPad to the host, using the A-Male to Mini-Male USB Cable.

## <a name="running-gateway-out-of-box"></a>
## Running Gateway Out-of-Box
The local gateway sample application may be used to create and manage a Zigbee network on the host processor itself.

1. To start the gateway:
    - Open a new terminal accessing the host (eg PuTTY for BeagleBone Black, Terminal for Linux OS machine)
    - `cd ~/source/out/Precompiled/servers`
    - `sudo ./zigbeeHAgw`
    - This will start and monitor the following servers:
        - NPI_lnx
        - ZLSZNP
        - NWKMGR_SRVR
        - GATEWAY_SRVR
        - OTA_SRVR
* To start the sample gateway application
    - `cd ~/source/out/Precompiled/servers`
    - `sudo ./start_application`
* You will see the following screen:

![Local Gateway Application](/resources/localapp_connected.png)

For further instructions on how to use this user interface, please refer to the [User Interface section](Z-Stack Linux Gateway - User's Guide.html#using-the-local-gateway-sample-application) of the User's Guide.

## <a name="running-the-web-application"></a>
## Running the Web Application
A web application may be hosted on the local host platform to control the Zigbee network.

1. If you have already started the Zigbee Linux Gateway servers, start the web application with the following commands:
    - cd ~/source/Projects/node/nodejs_zb_gateway
    - node main.js &
* If you have not started the Zigbee Linux Gateway servers, you may start both the servers and the web application with the following commands:
    - cd ~/source/Projects/node
    - sudo ./start_local.sh
* Wait for the Gateway to start. An IP Address will be printed out in the terminal indicating where the local web application is being hosted. On your PC in a web browser, navigate to the IP address of your BeagleBone Black, followed by port 5000.
    - For example: http://192.168.7.2:5000/
    - Note your Host platform's IP address and port may vary in the output.
* The integrated local web server should now be accessible via your Host platform's IP address on port 5000 (e.g. http://192.168.7.2:5000/).
Open a web browser on any machine connected to the same local network as the Host platform.
When you've navigated to your Host platform's Sensor-to-Cloud address in your browser, you should be presented with a similar screen as pictured below.

![Web application](/resources/nodejs_start.png)

For further instructions on how to use the web app, please refer to the [Web Application section](Z-Stack Linux Gateway - User's Guide.html#using-the-web-application) of the User's Guide.
