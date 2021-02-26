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
#include "otasrvr.pb-c.h"
#include "pb_utils.h"

/******************************************************************************
 * Data Structures
 *****************************************************************************/
const pb_pointers_entry _otasrvr_pb_pointer_list[] = 
{
 {&ota_zigbee_generic_cnf__descriptor,			   (pb__unpack)ota_zigbee_generic_cnf__unpack,			   (pb__free_unpacked)ota_zigbee_generic_cnf__free_unpacked,			 OTA_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF},
 {&ota_zigbee_generic_rsp_ind__descriptor,		   (pb__unpack)ota_zigbee_generic_rsp_ind__unpack,		   (pb__free_unpacked)ota_zigbee_generic_rsp_ind__free_unpacked,		 OTA_MGR_CMD_ID_T__ZIGBEE_GENERIC_RSP_IND},
 {&ota_update_image_registeration_req__descriptor, (pb__unpack)ota_update_image_registeration_req__unpack, (pb__free_unpacked)ota_update_image_registeration_req__free_unpacked, OTA_MGR_CMD_ID_T__OTA_UPDATE_IMAGE_REGISTERATION_REQ},
 {&ota_update_enable_req__descriptor,			   (pb__unpack)ota_update_enable_req__unpack,			   (pb__free_unpacked)ota_update_enable_req__free_unpacked, 			 OTA_MGR_CMD_ID_T__OTA_UPDATE_ENABLE_REQ},
 {&ota_update_enable_cnf__descriptor,			   (pb__unpack)ota_update_enable_cnf__unpack,			   (pb__free_unpacked)ota_update_enable_cnf__free_unpacked, 			 OTA_MGR_CMD_ID_T__OTA_UPDATE_ENABLE_CNF},
 {&ota_update_dl_finished_ind__descriptor,		   (pb__unpack)ota_update_dl_finished_ind__unpack,		   (pb__free_unpacked)ota_update_dl_finished_ind__free_unpacked,		 OTA_MGR_CMD_ID_T__OTA_UPDATE_DL_FINISHED_IND},
 {&ota_update_apply_image_req__descriptor,		   (pb__unpack)ota_update_apply_image_req__unpack,		   (pb__free_unpacked)ota_update_apply_image_req__free_unpacked,		 OTA_MGR_CMD_ID_T__OTA_UPDATE_APPLY_IMAGE_REQ},
};

pb_pointer_list_t otasrvr_pb_pointer_list = {ZSTACK_OTASYS_IDS__RPC_SYS_PB_OTA_MGR, NULL, sizeof(_otasrvr_pb_pointer_list) / sizeof(_otasrvr_pb_pointer_list[0]), _otasrvr_pb_pointer_list};
  
