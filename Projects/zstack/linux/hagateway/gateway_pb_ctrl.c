/*******************************************************************************
 Filename:      gateway_pb_ctrl.c
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
#include "gateway.pb-c.h"
#include "pb_utils.h"

/******************************************************************************
 * Data Structures
 *****************************************************************************/
const pb_pointers_entry _gateway_pb_pointers_list[] = 
{
 {&gw_zigbee_generic_cnf__descriptor,				 (pb__unpack)gw_zigbee_generic_cnf__unpack, 			   (pb__free_unpacked)gw_zigbee_generic_cnf__free_unpacked, 			   GW_CMD_ID_T__ZIGBEE_GENERIC_CNF},
 {&gw_zigbee_generic_rsp_ind__descriptor,			 (pb__unpack)gw_zigbee_generic_rsp_ind__unpack, 		   (pb__free_unpacked)gw_zigbee_generic_rsp_ind__free_unpacked, 		   GW_CMD_ID_T__ZIGBEE_GENERIC_RSP_IND},
 {&gw_add_group_req__descriptor,					 (pb__unpack)gw_add_group_req__unpack,					   (pb__free_unpacked)gw_add_group_req__free_unpacked,					   GW_CMD_ID_T__GW_ADD_GROUP_REQ},
 {&gw_get_group_membership_req__descriptor, 		 (pb__unpack)gw_get_group_membership_req__unpack,		   (pb__free_unpacked)gw_get_group_membership_req__free_unpacked,		   GW_CMD_ID_T__GW_GET_GROUP_MEMBERSHIP_REQ},
 {&gw_get_group_membership_rsp_ind__descriptor, 	 (pb__unpack)gw_get_group_membership_rsp_ind__unpack,	   (pb__free_unpacked)gw_get_group_membership_rsp_ind__free_unpacked,	   GW_CMD_ID_T__GW_GET_GROUP_MEMBERSHIP_RSP_IND},
 {&gw_remove_from_group_req__descriptor,			 (pb__unpack)gw_remove_from_group_req__unpack,			   (pb__free_unpacked)gw_remove_from_group_req__free_unpacked,			   GW_CMD_ID_T__GW_REMOVE_FROM_GROUP_REQ},
 {&gw_store_scene_req__descriptor,					 (pb__unpack)gw_store_scene_req__unpack,				   (pb__free_unpacked)gw_store_scene_req__free_unpacked,				   GW_CMD_ID_T__GW_STORE_SCENE_REQ},
 {&gw_remove_scene_req__descriptor, 				 (pb__unpack)gw_remove_scene_req__unpack,				   (pb__free_unpacked)gw_remove_scene_req__free_unpacked,				   GW_CMD_ID_T__GW_REMOVE_SCENE_REQ},
 {&gw_recall_scene_req__descriptor, 				 (pb__unpack)gw_recall_scene_req__unpack,				   (pb__free_unpacked)gw_recall_scene_req__free_unpacked,				   GW_CMD_ID_T__GW_RECALL_SCENE_REQ},
 {&gw_get_scene_membership_req__descriptor, 		 (pb__unpack)gw_get_scene_membership_req__unpack,		   (pb__free_unpacked)gw_get_scene_membership_req__free_unpacked,		   GW_CMD_ID_T__GW_GET_SCENE_MEMBERSHIP_REQ},
 {&gw_get_scene_membership_rsp_ind__descriptor, 	 (pb__unpack)gw_get_scene_membership_rsp_ind__unpack,	   (pb__free_unpacked)gw_get_scene_membership_rsp_ind__free_unpacked,	   GW_CMD_ID_T__GW_GET_SCENE_MEMBERSHIP_RSP_IND},
 {&gw_sleepy_device_packet_pending_req__descriptor,  (pb__unpack)gw_sleepy_device_packet_pending_req__unpack,  (pb__free_unpacked)gw_sleepy_device_packet_pending_req__free_unpacked,  GW_CMD_ID_T__GW_SLEEPY_DEVICE_PACKET_PENDING_REQ},
 {&gw_sleepy_device_check_in_ind__descriptor,		 (pb__unpack)gw_sleepy_device_check_in_ind__unpack, 	   (pb__free_unpacked)gw_sleepy_device_check_in_ind__free_unpacked, 	   GW_CMD_ID_T__GW_SLEEPY_DEVICE_CHECK_IN_IND},
 {&gw_attribute_change_ind__descriptor, 			 (pb__unpack)gw_attribute_change_ind__unpack,			   (pb__free_unpacked)gw_attribute_change_ind__free_unpacked,			   GW_CMD_ID_T__GW_ATTRIBUTE_CHANGE_IND},
 {&gw_get_device_attribute_list_req__descriptor,	 (pb__unpack)gw_get_device_attribute_list_req__unpack,	   (pb__free_unpacked)gw_get_device_attribute_list_req__free_unpacked,	   GW_CMD_ID_T__GW_GET_DEVICE_ATTRIBUTE_LIST_REQ},
 {&gw_get_device_attribute_list_rsp_ind__descriptor, (pb__unpack)gw_get_device_attribute_list_rsp_ind__unpack, (pb__free_unpacked)gw_get_device_attribute_list_rsp_ind__free_unpacked, GW_CMD_ID_T__GW_GET_DEVICE_ATTRIBUTE_LIST_RSP_IND},
 {&gw_read_device_attribute_req__descriptor,		 (pb__unpack)gw_read_device_attribute_req__unpack,		   (pb__free_unpacked)gw_read_device_attribute_req__free_unpacked,		   GW_CMD_ID_T__GW_READ_DEVICE_ATTRIBUTE_REQ},
 {&gw_read_device_attribute_rsp_ind__descriptor,	 (pb__unpack)gw_read_device_attribute_rsp_ind__unpack,	   (pb__free_unpacked)gw_read_device_attribute_rsp_ind__free_unpacked,	   GW_CMD_ID_T__GW_READ_DEVICE_ATTRIBUTE_RSP_IND},
 {&dev_process_identify_query_rsp_ind__descriptor,	 (pb__unpack)dev_process_identify_query_rsp_ind__unpack,	   (pb__free_unpacked)dev_process_identify_query_rsp_ind__free_unpacked,	   GW_CMD_ID_T__DEV_PROCESS_IDENTIFY_QUERY_RSP_IND},
 {&gw_write_device_attribute_req__descriptor,		 (pb__unpack)gw_write_device_attribute_req__unpack, 	   (pb__free_unpacked)gw_write_device_attribute_req__free_unpacked, 	   GW_CMD_ID_T__GW_WRITE_DEVICE_ATTRIBUTE_REQ},
 {&gw_write_device_attribute_rsp_ind__descriptor,	 (pb__unpack)gw_write_device_attribute_rsp_ind__unpack,    (pb__free_unpacked)gw_write_device_attribute_rsp_ind__free_unpacked,    GW_CMD_ID_T__GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND},
 {&gw_set_attribute_reporting_req__descriptor,		 (pb__unpack)gw_set_attribute_reporting_req__unpack,	   (pb__free_unpacked)gw_set_attribute_reporting_req__free_unpacked,	   GW_CMD_ID_T__GW_SET_ATTRIBUTE_REPORTING_REQ},
 {&gw_set_attribute_reporting_rsp_ind__descriptor,	 (pb__unpack)gw_set_attribute_reporting_rsp_ind__unpack,   (pb__free_unpacked)gw_set_attribute_reporting_rsp_ind__free_unpacked,   GW_CMD_ID_T__GW_SET_ATTRIBUTE_REPORTING_RSP_IND},
 {&gw_attribute_reporting_ind__descriptor,			 (pb__unpack)gw_attribute_reporting_ind__unpack,		   (pb__free_unpacked)gw_attribute_reporting_ind__free_unpacked,		   GW_CMD_ID_T__GW_ATTRIBUTE_REPORTING_IND},
 {&gw_send_zcl_frame_req__descriptor,				 (pb__unpack)gw_send_zcl_frame_req__unpack, 			   (pb__free_unpacked)gw_send_zcl_frame_req__free_unpacked, 			   GW_CMD_ID_T__GW_SEND_ZCL_FRAME_REQ},
 {&gw_zcl_frame_receive_ind__descriptor,			 (pb__unpack)gw_zcl_frame_receive_ind__unpack,			   (pb__free_unpacked)gw_zcl_frame_receive_ind__free_unpacked,			   GW_CMD_ID_T__GW_ZCL_FRAME_RECEIVE_IND},
 {&gw_alarm_ind__descriptor,						 (pb__unpack)gw_alarm_ind__unpack,						   (pb__free_unpacked)gw_alarm_ind__free_unpacked,						   GW_CMD_ID_T__GW_ALARM_IND},
 {&gw_alarm_reset_req__descriptor,					 (pb__unpack)gw_alarm_reset_req__unpack,				   (pb__free_unpacked)gw_alarm_reset_req__free_unpacked,				   GW_CMD_ID_T__GW_ALARM_RESET_REQ},
 {&dev_zone_enrollment_req_ind__descriptor, 		 (pb__unpack)dev_zone_enrollment_req_ind__unpack,		   (pb__free_unpacked)dev_zone_enrollment_req_ind__free_unpacked,		   GW_CMD_ID_T__DEV_ZONE_ENROLLMENT_REQ_IND},
 {&dev_zone_enrollment_rsp__descriptor, 			 (pb__unpack)dev_zone_enrollment_rsp__unpack,			   (pb__free_unpacked)dev_zone_enrollment_rsp__free_unpacked,			   GW_CMD_ID_T__DEV_ZONE_ENROLLMENT_RSP},
 {&dev_zone_status_change_ind__descriptor,			 (pb__unpack)dev_zone_status_change_ind__unpack,		   (pb__free_unpacked)dev_zone_status_change_ind__free_unpacked,		   GW_CMD_ID_T__DEV_ZONE_STATUS_CHANGE_IND},
 {&dev_ace_arm_req_ind__descriptor, 				 (pb__unpack)dev_ace_arm_req_ind__unpack,				   (pb__free_unpacked)dev_ace_arm_req_ind__free_unpacked,				   GW_CMD_ID_T__DEV_ACE_ARM_REQ_IND},
 {&dev_ace_arm_rsp__descriptor, 					 (pb__unpack)dev_ace_arm_rsp__unpack,					   (pb__free_unpacked)dev_ace_arm_rsp__free_unpacked,					   GW_CMD_ID_T__DEV_ACE_ARM_RSP},
 {&dev_ace_bypass_ind__descriptor,					 (pb__unpack)dev_ace_bypass_ind__unpack,				   (pb__free_unpacked)dev_ace_bypass_ind__free_unpacked,				   GW_CMD_ID_T__DEV_ACE_BYPASS_IND},
 {&dev_ace_emergency_condition_ind__descriptor, 	 (pb__unpack)dev_ace_emergency_condition_ind__unpack,	   (pb__free_unpacked)dev_ace_emergency_condition_ind__free_unpacked,	   GW_CMD_ID_T__DEV_ACE_EMERGENCY_CONDITION_IND},
 {&dev_ace_get_zone_id_map_req_ind__descriptor, 	 (pb__unpack)dev_ace_get_zone_id_map_req_ind__unpack,	   (pb__free_unpacked)dev_ace_get_zone_id_map_req_ind__free_unpacked,	   GW_CMD_ID_T__DEV_ACE_GET_ZONE_ID_MAP_REQ_IND},
 {&dev_ace_get_zone_id_map_rsp__descriptor, 		 (pb__unpack)dev_ace_get_zone_id_map_rsp__unpack,		   (pb__free_unpacked)dev_ace_get_zone_id_map_rsp__free_unpacked,		   GW_CMD_ID_T__DEV_ACE_GET_ZONE_ID_MAP_RSP},
 {&dev_ace_get_zone_information_req_ind__descriptor, (pb__unpack)dev_ace_get_zone_information_req_ind__unpack, (pb__free_unpacked)dev_ace_get_zone_information_req_ind__free_unpacked, GW_CMD_ID_T__DEV_ACE_GET_ZONE_INFORMATION_REQ_IND},
 {&dev_ace_get_zone_information_rsp__descriptor,	 (pb__unpack)dev_ace_get_zone_information_rsp__unpack,	   (pb__free_unpacked)dev_ace_get_zone_information_rsp__free_unpacked,	   GW_CMD_ID_T__DEV_ACE_GET_ZONE_INFORMATION_RSP},
 {&dev_set_identify_mode_req__descriptor,			 (pb__unpack)dev_set_identify_mode_req__unpack, 		   (pb__free_unpacked)dev_set_identify_mode_req__free_unpacked, 		   GW_CMD_ID_T__DEV_SET_IDENTIFY_MODE_REQ},
 {&dev_set_on_off_state_req__descriptor,			 (pb__unpack)dev_set_on_off_state_req__unpack,			   (pb__free_unpacked)dev_set_on_off_state_req__free_unpacked,			   GW_CMD_ID_T__DEV_SET_ONOFF_STATE_REQ},
 {&dev_send_identify_query_req__descriptor,			 (pb__unpack)dev_send_identify_query_req__unpack,		   (pb__free_unpacked)dev_send_identify_query_req__free_unpacked,		   GW_CMD_ID_T__DEV_SEND_IDENTIFY_QUERY_REQ},
 {&dev_set_finding_and_binding_timer__descriptor,			 (pb__unpack)dev_set_finding_and_binding_timer__unpack,		   (pb__free_unpacked)dev_set_finding_and_binding_timer__free_unpacked,		   GW_CMD_ID_T__DEV_SET_FINDING_AND_BINDING_TIMER_STATUS_REQ},
 {&dev_set_level_req__descriptor,					 (pb__unpack)dev_set_level_req__unpack, 				   (pb__free_unpacked)dev_set_level_req__free_unpacked, 				   GW_CMD_ID_T__DEV_SET_LEVEL_REQ},
 {&dev_get_level_req__descriptor,					 (pb__unpack)dev_get_level_req__unpack, 				   (pb__free_unpacked)dev_get_level_req__free_unpacked, 				   GW_CMD_ID_T__DEV_GET_LEVEL_REQ},
 {&dev_get_level_rsp_ind__descriptor,				 (pb__unpack)dev_get_level_rsp_ind__unpack, 			   (pb__free_unpacked)dev_get_level_rsp_ind__free_unpacked, 			   GW_CMD_ID_T__DEV_GET_LEVEL_RSP_IND},
 {&dev_get_on_off_state_req__descriptor,			 (pb__unpack)dev_get_on_off_state_req__unpack,			   (pb__free_unpacked)dev_get_on_off_state_req__free_unpacked,			   GW_CMD_ID_T__DEV_GET_ONOFF_STATE_REQ},
 {&dev_get_on_off_state_rsp_ind__descriptor,		 (pb__unpack)dev_get_on_off_state_rsp_ind__unpack,		   (pb__free_unpacked)dev_get_on_off_state_rsp_ind__free_unpacked,		   GW_CMD_ID_T__DEV_GET_ONOFF_STATE_RSP_IND},
 {&dev_set_color_req__descriptor,					 (pb__unpack)dev_set_color_req__unpack, 				   (pb__free_unpacked)dev_set_color_req__free_unpacked, 				   GW_CMD_ID_T__DEV_SET_COLOR_REQ},
 {&dev_get_color_req__descriptor,					 (pb__unpack)dev_get_color_req__unpack, 				   (pb__free_unpacked)dev_get_color_req__free_unpacked, 				   GW_CMD_ID_T__DEV_GET_COLOR_REQ},
 {&dev_get_color_rsp_ind__descriptor,				 (pb__unpack)dev_get_color_rsp_ind__unpack, 			   (pb__free_unpacked)dev_get_color_rsp_ind__free_unpacked, 			   GW_CMD_ID_T__DEV_GET_COLOR_RSP_IND},
 {&dev_set_color_temp_req__descriptor,               (pb__unpack)dev_set_color_temp_req__unpack,               (pb__free_unpacked)dev_set_color_temp_req__free_unpacked,               GW_CMD_ID_T__DEV_SET_COLOR_TEMP_REQ},
 {&dev_get_color_temp_req__descriptor,               (pb__unpack)dev_get_color_temp_req__unpack,               (pb__free_unpacked)dev_get_color_temp_req__free_unpacked,               GW_CMD_ID_T__DEV_GET_COLOR_TEMP_REQ},
 {&dev_get_color_temp_rsp_ind__descriptor,           (pb__unpack)dev_get_color_temp_rsp_ind__unpack,           (pb__free_unpacked)dev_get_color_temp_rsp_ind__free_unpacked,           GW_CMD_ID_T__DEV_GET_COLOR_TEMP_RSP_IND},
 {&dev_get_temp_req__descriptor,					 (pb__unpack)dev_get_temp_req__unpack,					   (pb__free_unpacked)dev_get_temp_req__free_unpacked,					   GW_CMD_ID_T__DEV_GET_TEMP_REQ},
 {&dev_get_temp_rsp_ind__descriptor,				 (pb__unpack)dev_get_temp_rsp_ind__unpack,				   (pb__free_unpacked)dev_get_temp_rsp_ind__free_unpacked,				   GW_CMD_ID_T__DEV_GET_TEMP_RSP_IND},
 {&dev_get_power_req__descriptor,					 (pb__unpack)dev_get_power_req__unpack, 				   (pb__free_unpacked)dev_get_power_req__free_unpacked, 				   GW_CMD_ID_T__DEV_GET_POWER_REQ},
 {&dev_get_power_rsp_ind__descriptor,				 (pb__unpack)dev_get_power_rsp_ind__unpack, 			   (pb__free_unpacked)dev_get_power_rsp_ind__free_unpacked, 			   GW_CMD_ID_T__DEV_GET_POWER_RSP_IND},
 {&dev_get_humidity_req__descriptor,				 (pb__unpack)dev_get_humidity_req__unpack,				   (pb__free_unpacked)dev_get_humidity_req__free_unpacked,				   GW_CMD_ID_T__DEV_GET_HUMIDITY_REQ},
 {&dev_get_humidity_rsp_ind__descriptor,			 (pb__unpack)dev_get_humidity_rsp_ind__unpack,			   (pb__free_unpacked)dev_get_humidity_rsp_ind__free_unpacked,			   GW_CMD_ID_T__DEV_GET_HUMIDITY_RSP_IND},
 {&dev_set_door_lock_req__descriptor,				 (pb__unpack)dev_set_door_lock_req__unpack, 			   (pb__free_unpacked)dev_set_door_lock_req__free_unpacked, 			   GW_CMD_ID_T__DEV_SET_DOOR_LOCK_REQ},
 {&dev_set_door_lock_rsp_ind__descriptor,			 (pb__unpack)dev_set_door_lock_rsp_ind__unpack, 		   (pb__free_unpacked)dev_set_door_lock_rsp_ind__free_unpacked, 		   GW_CMD_ID_T__DEV_SET_DOOR_LOCK_RSP_IND},
 {&dev_get_door_lock_state_req__descriptor, 		 (pb__unpack)dev_get_door_lock_state_req__unpack,		   (pb__free_unpacked)dev_get_door_lock_state_req__free_unpacked,		   GW_CMD_ID_T__DEV_GET_DOOR_LOCK_STATE_REQ},
 {&dev_get_door_lock_state_rsp_ind__descriptor, 	 (pb__unpack)dev_get_door_lock_state_rsp_ind__unpack,	   (pb__free_unpacked)dev_get_door_lock_state_rsp_ind__free_unpacked,	   GW_CMD_ID_T__DEV_GET_DOOR_LOCK_STATE_RSP_IND},
 {&dev_thermostat_setpoint_change_req__descriptor,	 (pb__unpack)dev_thermostat_setpoint_change_req__unpack,   (pb__free_unpacked)dev_thermostat_setpoint_change_req__free_unpacked,   GW_CMD_ID_T__DEV_THERMOSTAT_SETPOINT_CHANGE_REQ},
 {&dev_window_covering_action_req__descriptor,		 (pb__unpack)dev_window_covering_action_req__unpack,	   (pb__free_unpacked)dev_window_covering_action_req__free_unpacked,	   GW_CMD_ID_T__DEV_WINDOW_COVERING_ACTION_REQ},
};

pb_pointer_list_t gateway_pb_pointer_list = {Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, NULL, sizeof(_gateway_pb_pointers_list) / sizeof(_gateway_pb_pointers_list[0]), _gateway_pb_pointers_list};

