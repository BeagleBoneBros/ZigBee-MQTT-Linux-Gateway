# <a name="building-images"></a>
# Building Images

CC253x ZNP images are built from the ZNP project of [Z-Stack 3.0.2](http://www.ti.com/tool/Z-STACK).
CC1352/CC2652 ZNP images are built from the ZNP project of the [SimpleLink&trade; CC13x2/26x2 SDK](http://www.ti.com/tool/download/SIMPLELINK-CC13X2-26X2-SDK) (3.10 version).

The sampleapps are built with the corresponding projects in the [SimpleLink&trade; CC13x2/26x2 SDK](http://www.ti.com/tool/download/SIMPLELINK-CC13X2-26X2-SDK) (3.10 version).

To build any of the CC13x2/CC26x2 images yourself, download the [SimpleLink&trade; CC13x2/26x2 SDK](http://www.ti.com/tool/download/SIMPLELINK-CC13X2-26X2-SDK), import the project of interest, and build.  
To build the CC253x images, download [Z-Stack 3.0.2](http://www.ti.com/tool/Z-STACK), import the project of interest, and build.  
For ZNP images, please apply the relevent [configuration options](#configuration-options-for-znp-images).  
Note that all prebuilt images arbitrarily use channel 13 (as determined by DEFAULT_CHANLIST).


# <a name="configuration-options-for-znp-images"></a>
## Configuration Options for ZNP images
CC2538/CC2652/CC1352 ZNP images are built with the following configuration options:

```
/* MTO concentrator configuration */
-DCONCENTRATOR_ENABLE=TRUE
-DCONCENTRATOR_DISCOVERY_TIME=60
-DCONCENTRATOR_ROUTE_CACHE=TRUE
-DMAX_RTG_SRC_ENTRIES=200
-DSRC_RTG_EXPIRY_TIME=255
/* -DCONCENTRATOR_RADIUS // leave this as default */

-DHAL_UART=TRUE
```

CC2538_GW_ZNP_EM_StandAlone_USB.hex is (additionally) built with:

```
-DHAL_UART_USB
-DUSB_SETUP_MAX_NUMBER_OF_INTERFACES=5
-DBB_ZNP
(in npiInit(), uartConfig.flowControl = FALSE;)

/* Device lists configuration */
-DNWK_MAX_DEVICE_LIST=50
-DZDSECMGR_TC_DEVICE_MAX=200
/* -DMAX_NEIGHBOR_ENTRIES // leave this as default */
/* -DMAX_RTG_ENTRIES // leave this as default */
```


CC2531 ZNP is built with the following configuration options:

```
HAL_UART_DMA_RX_MAX=128
INT_HEAP_LEN=2700 // heap size
NWK_MAX_DEVICE_LIST=10
```


CC2530 ZNP is built with the following configuration options:

```
BDB_FINDING_BINDING_CAPABILITY_ENABLED=0
TC_LINKKEY_JOIN
NWK_MAX_DEVICE_LIST=2
ZDSECMGR_TC_DEVICE_MAX=2
DISABLE_GREENPOWER_BASIC_PROXY
POWER_SAVING
xFEATURE_SYSTEM_STATS
MT_GP_CB_FUNC
ZNP_ALT                     // To disable flow control
```


> Different serial interfaces take up different amounts of RAM.
RAM was optimized for CC2530 as mentioned [here](http://processors.wiki.ti.com/index.php/Optimizing_Flash_and_RAM_Usage_of_Z-Stack_for_CC2530).


# <a name="how-to-use-binaries"></a>
# How to use binaries:

This binary can be flashed using TI Flash Programmer tool
- CC2531_GW_ZNP_USB.hex

- CC2530_GW_ZNP_SRF05+EM_UART.hex
	- This is intended to be used on the CC2530 EM with the SmartRF05 EB.
	- If an FTDI device is used in the initialing servers script zigbeeHAgw change the line according to device location:  
	  ```
		export ZBEE_SERIAL_PORT=/dev/ttyACM0
		```
		to
		```
		export ZBEE_SERIAL_PORT=/dev/ttyUSBx {0,1,...}
	  ```
	- If UART pins are used, connect them as follows:

		- CC2530DK UART RX P0.2 (P18 Pin9)  <----> BBB UART4 TX (P9 Pin13)
		- CC2530DK UART TX P0.3 (P18 Pin11) <----> BBB UART4 RX (P9 Pin11)
		- CC2530DK GND (P18 Pin20)          <----> BBB DGND (P9 Pin1)

  - Then in the initialing servers script zigbeeHAgw change the line according to device location:

	 ```
  	export ZBEE_SERIAL_PORT=/dev/ttyACM0
	 ```
	 to
	 ```
	 export ZBEE_SERIAL_PORT=/dev/ttyO4
	 ```
  - If the Gateway fails to startup on the first try and the script automatically starts the reset loop, you must MANUALLY reset the SRF05 by power cycling it or pressing the "EM Reset" button.

This binary can be flashed using TI UniFlash tool

- CC2652R1LP_GW_ZNP_UART.hex
- CC1352R1LP_GW_ZNP_UART.hex
- CC1352P2LP_GW_ZNP_UART.hex

These binaries can be flashed using TI Flash Programmer 2 or UniFlash tool

- CC2538_GW_ZNP_EM_StandAlone_USB.hex

	> NOTE: This is intended to be used on the CC2538 EM standalone through its USB connector.

- CC2538_GW_ZNP_SRF06+EM_UART.hex
	
	> NOTE: This is intended to be used on the CC2538 EM with the smartRF06 EB.

To evaluate CC2538_GW_ZNP_EM_StandAlone_USB.hex without smartRF06 EB, the jumper P5 can be swapped to select VDD and USB, instead of VDD and EB Power, this makes CC2538 EM take its power supply from USB port instead of the smartRF06 EB.

To evaluate CC2538_GW_ZNP_SRF06+EM_UART.hex using smartRF06 EB the following configuration is required:

1. Place CC2538 EM on the top of smartRF06 and connect USBXDS100 to BeagleBone USB  
* The device must be added to the list of devices in the Linux connected device list (/dev), the device can be added with the following commands executed in
	```
	/dev
	modprobe ftdi_sio vendor=0x403 product=0xa6d1
	modprobe ftdio_sio
	echo 0403 a6d1 > /sys/bus/usb-serial/drivers/ftdi_sio/new_id
	```  
* Make sure ttyUSB1 is listed in the device tree, by typing ls in /dev this command ( ls ) will list all connected devices.  
* In the initialing servers script zigbeeHAgw change the line
	```
	export ZBEE_SERIAL_PORT=/dev/ttyACM0
	```
	to
	```
	export ZBEE_SERIAL_PORT=/dev/ttyUSB1
	```
* In NPI_Gateway.cfg disable flow control defined by:
	```
	[UART]
	speed=115200 ; speed
	flowcontrol=0 ; 1=enabled 0=disable			
	```
