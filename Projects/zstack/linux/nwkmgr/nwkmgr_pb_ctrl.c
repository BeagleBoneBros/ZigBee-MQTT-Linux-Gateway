/*******************************************************************************
 Filename:      nwkmgr_pb_ctrl.c
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
#include "nwkmgr.pb-c.h"
#include "pb_utils.h"

/******************************************************************************
 * Data Structures
 *****************************************************************************/
const pb_pointers_entry _nwkmgr_pb_pointer_list[] = 
{
	{&nwk_zigbee_generic_cnf__descriptor,              (pb__unpack)nwk_zigbee_generic_cnf__unpack,              (pb__free_unpacked)nwk_zigbee_generic_cnf__free_unpacked,              NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF}, 								
	{&nwk_zigbee_generic_rsp_ind__descriptor,          (pb__unpack)nwk_zigbee_generic_rsp_ind__unpack,          (pb__free_unpacked)nwk_zigbee_generic_rsp_ind__free_unpacked,          NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_RSP_IND}, 							
	{&nwk_zigbee_system_reset_req__descriptor,         (pb__unpack)nwk_zigbee_system_reset_req__unpack,         (pb__free_unpacked)nwk_zigbee_system_reset_req__free_unpacked,         NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_RESET_REQ}, 					
	{&nwk_zigbee_system_reset_cnf__descriptor,         (pb__unpack)nwk_zigbee_system_reset_cnf__unpack,         (pb__free_unpacked)nwk_zigbee_system_reset_cnf__free_unpacked,         NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_RESET_CNF}, 					
	{&nwk_zigbee_system_self_shutdown_req__descriptor, (pb__unpack)nwk_zigbee_system_self_shutdown_req__unpack, (pb__free_unpacked)nwk_zigbee_system_self_shutdown_req__free_unpacked, NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_SELF_SHUTDOWN_REQ}, 			
	{&nwk_set_zigbee_power_mode_req__descriptor,       (pb__unpack)nwk_set_zigbee_power_mode_req__unpack,       (pb__free_unpacked)nwk_set_zigbee_power_mode_req__free_unpacked,       NWK_MGR_CMD_ID_T__NWK_SET_ZIGBEE_POWER_MODE_REQ}, 					
	{&nwk_set_zigbee_power_mode_cnf__descriptor,       (pb__unpack)nwk_set_zigbee_power_mode_cnf__unpack,       (pb__free_unpacked)nwk_set_zigbee_power_mode_cnf__free_unpacked,       NWK_MGR_CMD_ID_T__NWK_SET_ZIGBEE_POWER_MODE_CNF}, 					
	{&nwk_get_local_device_info_req__descriptor,       (pb__unpack)nwk_get_local_device_info_req__unpack,       (pb__free_unpacked)nwk_get_local_device_info_req__free_unpacked,       NWK_MGR_CMD_ID_T__NWK_GET_LOCAL_DEVICE_INFO_REQ},
	{&nwk_get_local_device_info_cnf__descriptor,       (pb__unpack)nwk_get_local_device_info_cnf__unpack,       (pb__free_unpacked)nwk_get_local_device_info_cnf__free_unpacked,       NWK_MGR_CMD_ID_T__NWK_GET_LOCAL_DEVICE_INFO_CNF},
	{&nwk_get_gw_endpoint_info_cnf__descriptor,        (pb__unpack)nwk_get_gw_endpoint_info_cnf__unpack,        (pb__free_unpacked)nwk_get_gw_endpoint_info_cnf__free_unpacked,        NWK_MGR_CMD_ID_T__NWK_GET_GW_ENDPOINT_INFO_CNF},
	{&nwk_start_commissioning_req__descriptor,	   	   (pb__unpack)nwk_start_commissioning_req__unpack,			(pb__free_unpacked)nwk_start_commissioning_req__free_unpacked,	   	   NWK_MGR_CMD_ID_T__NWK_START_COMMISSIONING_REQ},
	{&nwk_send_simple_descriptor_req__descriptor,	           (pb__unpack)nwk_send_simple_descriptor_req__unpack,	     	(pb__free_unpacked)nwk_send_simple_descriptor_req__free_unpacked,	   NWK_MGR_CMD_ID_T__NWK_SEND_SIMPLE_DESCRIPTOR_REQ},
	{&nwk_set_install_code_req__descriptor,	   	   	   (pb__unpack)nwk_set_install_code_req__unpack,			    (pb__free_unpacked)nwk_set_install_code_req__free_unpacked,	   	   NWK_MGR_CMD_ID_T__NWK_SET_INSTALLCODE_REQ},
	{&nwk_zigbee_nwk_ready_ind__descriptor,            (pb__unpack)nwk_zigbee_nwk_ready_ind__unpack,            (pb__free_unpacked)nwk_zigbee_nwk_ready_ind__free_unpacked,            NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_READY_IND}, 						
	{&nwk_zigbee_nwk_info_req__descriptor,             (pb__unpack)nwk_zigbee_nwk_info_req__unpack,             (pb__free_unpacked)nwk_zigbee_nwk_info_req__free_unpacked,             NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_REQ}, 						
	{&nwk_zigbee_nwk_info_cnf__descriptor,             (pb__unpack)nwk_zigbee_nwk_info_cnf__unpack,             (pb__free_unpacked)nwk_zigbee_nwk_info_cnf__free_unpacked,             NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_CNF}, 						
	{&nwk_set_permit_join_req__descriptor,             (pb__unpack)nwk_set_permit_join_req__unpack,             (pb__free_unpacked)nwk_set_permit_join_req__free_unpacked,             NWK_MGR_CMD_ID_T__NWK_SET_PERMIT_JOIN_REQ}, 						
	{&nwk_manage_periodic_mto_route_req__descriptor,   (pb__unpack)nwk_manage_periodic_mto_route_req__unpack,   (pb__free_unpacked)nwk_manage_periodic_mto_route_req__free_unpacked,   NWK_MGR_CMD_ID_T__NWK_MANAGE_PERIODIC_MTO_ROUTE_REQ}, 				
	{&nwk_get_neighbor_table_req__descriptor,          (pb__unpack)nwk_get_neighbor_table_req__unpack,          (pb__free_unpacked)nwk_get_neighbor_table_req__free_unpacked,          NWK_MGR_CMD_ID_T__NWK_GET_NEIGHBOR_TABLE_REQ}, 						
	{&nwk_get_neighbor_table_rsp_ind__descriptor,      (pb__unpack)nwk_get_neighbor_table_rsp_ind__unpack,      (pb__free_unpacked)nwk_get_neighbor_table_rsp_ind__free_unpacked,      NWK_MGR_CMD_ID_T__NWK_GET_NEIGHBOR_TABLE_RSP_IND}, 					
	{&nwk_get_routing_table_req__descriptor,           (pb__unpack)nwk_get_routing_table_req__unpack,           (pb__free_unpacked)nwk_get_routing_table_req__free_unpacked,           NWK_MGR_CMD_ID_T__NWK_GET_ROUTING_TABLE_REQ}, 						
	{&nwk_get_routing_table_rsp_ind__descriptor,       (pb__unpack)nwk_get_routing_table_rsp_ind__unpack,       (pb__free_unpacked)nwk_get_routing_table_rsp_ind__free_unpacked,       NWK_MGR_CMD_ID_T__NWK_GET_ROUTING_TABLE_RSP_IND}, 					
	{&nwk_change_nwk_key_req__descriptor,              (pb__unpack)nwk_change_nwk_key_req__unpack,              (pb__free_unpacked)nwk_change_nwk_key_req__free_unpacked,              NWK_MGR_CMD_ID_T__NWK_CHANGE_NWK_KEY_REQ}, 							
	{&nwk_get_nwk_key_req__descriptor,                 (pb__unpack)nwk_get_nwk_key_req__unpack,                 (pb__free_unpacked)nwk_get_nwk_key_req__free_unpacked,                 NWK_MGR_CMD_ID_T__NWK_GET_NWK_KEY_REQ}, 							
	{&nwk_get_nwk_key_cnf__descriptor,                 (pb__unpack)nwk_get_nwk_key_cnf__unpack,                 (pb__free_unpacked)nwk_get_nwk_key_cnf__free_unpacked,                 NWK_MGR_CMD_ID_T__NWK_GET_NWK_KEY_CNF}, 							
	{&nwk_zigbee_device_ind__descriptor,               (pb__unpack)nwk_zigbee_device_ind__unpack,               (pb__free_unpacked)nwk_zigbee_device_ind__free_unpacked,               NWK_MGR_CMD_ID_T__NWK_ZIGBEE_DEVICE_IND}, 							
	{&nwk_get_device_list_req__descriptor,             (pb__unpack)nwk_get_device_list_req__unpack,             (pb__free_unpacked)nwk_get_device_list_req__free_unpacked,             NWK_MGR_CMD_ID_T__NWK_GET_DEVICE_LIST_REQ}, 						
	{&nwk_get_device_list_cnf__descriptor,             (pb__unpack)nwk_get_device_list_cnf__unpack,             (pb__free_unpacked)nwk_get_device_list_cnf__free_unpacked,             NWK_MGR_CMD_ID_T__NWK_GET_DEVICE_LIST_CNF}, 						
	{&nwk_device_list_maintenance_req__descriptor,     (pb__unpack)nwk_device_list_maintenance_req__unpack,     (pb__free_unpacked)nwk_device_list_maintenance_req__free_unpacked,     NWK_MGR_CMD_ID_T__NWK_DEVICE_LIST_MAINTENANCE_REQ}, 				
	{&nwk_remove_device_req__descriptor,               (pb__unpack)nwk_remove_device_req__unpack,               (pb__free_unpacked)nwk_remove_device_req__free_unpacked,               NWK_MGR_CMD_ID_T__NWK_REMOVE_DEVICE_REQ}, 							
	{&nwk_set_binding_entry_req__descriptor,           (pb__unpack)nwk_set_binding_entry_req__unpack,           (pb__free_unpacked)nwk_set_binding_entry_req__free_unpacked,           NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_REQ}, 						
	{&nwk_set_binding_entry_rsp_ind__descriptor,       (pb__unpack)nwk_set_binding_entry_rsp_ind__unpack,       (pb__free_unpacked)nwk_set_binding_entry_rsp_ind__free_unpacked,       NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_RSP_IND},
	{&nwk_get_binding_table_req__descriptor,           (pb__unpack)nwk_get_binding_table_req__unpack,           (pb__free_unpacked)nwk_get_binding_table_req__free_unpacked,           NWK_MGR_CMD_ID_T__NWK_GET_BINDING_TABLE_REQ},
	{&nwk_get_binding_table_rsp_ind__descriptor,       (pb__unpack)nwk_get_binding_table_rsp_ind__unpack,       (pb__free_unpacked)nwk_get_binding_table_rsp_ind__free_unpacked,       NWK_MGR_CMD_ID_T__NWK_GET_BINDING_TABLE_RSP_IND},
	{&nwk_send_simple_descriptor_rsp_ind__descriptor,        (pb__unpack)nwk_send_simple_descriptor_rsp_ind__unpack,        (pb__free_unpacked)nwk_send_simple_descriptor_rsp_ind__free_unpacked,        NWK_MGR_CMD_ID_T__NWK_SEND_SIMPLE_DESCRIPTOR_RSP_IND},
	{&nwk_get_gw_endpoint_info_req__descriptor,        (pb__unpack)nwk_get_gw_endpoint_info_req__unpack,        (pb__free_unpacked)nwk_get_gw_endpoint_info_req__free_unpacked,        NWK_MGR_CMD_ID_T__NWK_GET_GW_ENDPOINT_INFO_REQ},
};

pb_pointer_list_t nwkmgr_pb_pointer_list = {Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, NULL, sizeof(_nwkmgr_pb_pointer_list) / sizeof(_nwkmgr_pb_pointer_list[0]), _nwkmgr_pb_pointer_list};

