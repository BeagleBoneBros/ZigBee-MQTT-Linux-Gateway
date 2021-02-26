/*******************************************************************************
 Filename:      otasrvr_pb_ctrl.c
 Revised:        $Date: 2014-11-17 16:49:39 -0800 (Mon, 17 Nov 2014) $
 Revision:       $Revision: 41158 $

 Description:   Protobuf Interface


 Copyright 2013 Texas Instruments Incorporated. All rights reserved.

 IMPORTANT: Your use of this Software is limited to those specific rights
 granted under the terms of a software license agreement between the user
 who downloaded the software, his/her employer (which must be your employer)
 and Texas Instruments Incorporated (the "License").  You may not use this
 Software unless you agree to abide by the terms of the License. The License
 limits your use, and you acknowledge, that the Software may not be modified,
 copied or distributed unless used solely and exclusively in conjunction with
 a Texas Instruments radio frequency device, which is integrated into
 your product.  Other than for the foregoing purpose, you may not use,
 reproduce, copy, prepare derivative works of, modify, distribute, perform,
 display or sell this Software and/or its documentation for any purpose.

 YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
 PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,l
 INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
 NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
 TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
 NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
 LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
 INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
 OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
 OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
 (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

 Should you have any questions regarding your right to use this Software,
 contact Texas Instruments Incorporated at www.TI.com.
*******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "server.pb-c.h"
#include "pb_utils.h"

/******************************************************************************
 * Data Structures
 *****************************************************************************/
const pb_pointers_entry _server_pb_pointer_list[] = 
{
	{&srvr_get_ieee_address_req__descriptor,  (pb__unpack)srvr_get_ieee_address_req__unpack,  (pb__free_unpacked)srvr_get_ieee_address_req__free_unpacked,  SRVR_CMD_ID_T__SRVR_GET_IEEE_ADDRESS_REQ},
	{&srvr_get_ieee_address_cnf__descriptor,  (pb__unpack)srvr_get_ieee_address_cnf__unpack,  (pb__free_unpacked)srvr_get_ieee_address_cnf__free_unpacked,  SRVR_CMD_ID_T__SRVR_GET_IEEE_ADDRESS_CNF},
	{&srvr_get_short_address_req__descriptor, (pb__unpack)srvr_get_short_address_req__unpack, (pb__free_unpacked)srvr_get_short_address_req__free_unpacked, SRVR_CMD_ID_T__SRVR_GET_SHORT_ADDRESS_REQ},
	{&srvr_get_short_address_cnf__descriptor, (pb__unpack)srvr_get_short_address_cnf__unpack, (pb__free_unpacked)srvr_get_short_address_cnf__free_unpacked, SRVR_CMD_ID_T__SRVR_GET_SHORT_ADDRESS_CNF},
	{&srvr_get_device_info_req__descriptor,   (pb__unpack)srvr_get_device_info_req__unpack,   (pb__free_unpacked)srvr_get_device_info_req__free_unpacked,   SRVR_CMD_ID_T__SRVR_GET_DEVICE_INFO_REQ},
	{&srvr_get_device_info_cnf__descriptor,   (pb__unpack)srvr_get_device_info_cnf__unpack,   (pb__free_unpacked)srvr_get_device_info_cnf__free_unpacked,   SRVR_CMD_ID_T__SRVR_GET_DEVICE_INFO_CNF},
	{&srvr_get_device_status_req__descriptor, (pb__unpack)srvr_get_device_status_req__unpack, (pb__free_unpacked)srvr_get_device_status_req__free_unpacked, SRVR_CMD_ID_T__SRVR_GET_DEVICE_STATUS_REQ},
	{&srvr_get_device_status_cnf__descriptor, (pb__unpack)srvr_get_device_status_cnf__unpack, (pb__free_unpacked)srvr_get_device_status_cnf__free_unpacked, SRVR_CMD_ID_T__SRVR_GET_DEVICE_STATUS_CNF},
	{&srvr_set_device_status_req__descriptor, (pb__unpack)srvr_set_device_status_req__unpack, (pb__free_unpacked)srvr_set_device_status_req__free_unpacked, SRVR_CMD_ID_T__SRVR_SET_DEVICE_STATUS_REQ},
	{&srvr_set_device_status_cnf__descriptor, (pb__unpack)srvr_set_device_status_cnf__unpack, (pb__free_unpacked)srvr_set_device_status_cnf__free_unpacked, SRVR_CMD_ID_T__SRVR_SET_DEVICE_STATUS_CNF},
};

pb_pointer_list_t server_pb_pointer_list = {Z_STACK_SERVER_SYS_ID_T__RPC_SYS_PB_SRVR, NULL, sizeof(_server_pb_pointer_list) / sizeof(_server_pb_pointer_list[0]), _server_pb_pointer_list};
  
