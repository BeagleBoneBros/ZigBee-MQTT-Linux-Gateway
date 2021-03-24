/*******************************************************************************
 Filename:      zstack_pb_ctrl.c
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
#include "zstack.pb-c.h"
#include "pb_utils.h"

/******************************************************************************
 * Data Structures
 *****************************************************************************/
const pb_pointers_entry _zstack_pb_pointer_list[] = 
{
 {&sys_reset_req__descriptor,			  (pb__unpack)sys_reset_req__unpack,			  (pb__free_unpacked)sys_reset_req__free_unpacked,				ZSTACK_CMD_IDS__SYS_RESET_REQ},
 {&sys_version_req__descriptor,			  (pb__unpack)sys_version_req__unpack,			  (pb__free_unpacked)sys_version_req__free_unpacked,			ZSTACK_CMD_IDS__SYS_VERSION_REQ},
 {&sys_config_read_req__descriptor,		  (pb__unpack)sys_config_read_req__unpack,		  (pb__free_unpacked)sys_config_read_req__free_unpacked,		ZSTACK_CMD_IDS__SYS_CONFIG_READ_REQ},
 {&sys_config_write_req__descriptor, 	  (pb__unpack)sys_config_write_req__unpack, 	  (pb__free_unpacked)sys_config_write_req__free_unpacked,		ZSTACK_CMD_IDS__SYS_CONFIG_WRITE_REQ},
 {&sys_set_tx_power_req__descriptor, 	  (pb__unpack)sys_set_tx_power_req__unpack, 	  (pb__free_unpacked)sys_set_tx_power_req__free_unpacked,		ZSTACK_CMD_IDS__SYS_SET_TX_POWER_REQ},
 {&sys_nwk_info_read_req__descriptor,	  (pb__unpack)sys_nwk_info_read_req__unpack,	  (pb__free_unpacked)sys_nwk_info_read_req__free_unpacked,		ZSTACK_CMD_IDS__SYS_NWK_INFO_READ_REQ},
//  {&sys_nwk_info_write_req__descriptor,	  (pb__unpack)sys_nwk_info_write_req__unpack,	  (pb__free_unpacked)sys_nwk_info_write_req__free_unpacked, 	ZSTACK_CMD_IDS__SYS_NWK_INFO_WRITE_REQ},
 {&sys_version_rsp__descriptor,			  (pb__unpack)sys_version_rsp__unpack,			  (pb__free_unpacked)sys_version_rsp__free_unpacked,			ZSTACK_CMD_IDS__SYS_VERSION_RSP},
 {&sys_config_read_rsp__descriptor,		  (pb__unpack)sys_config_read_rsp__unpack,		  (pb__free_unpacked)sys_config_read_rsp__free_unpacked,		ZSTACK_CMD_IDS__SYS_CONFIG_READ_RSP},
 {&sys_set_tx_power_rsp__descriptor, 	  (pb__unpack)sys_set_tx_power_rsp__unpack, 	  (pb__free_unpacked)sys_set_tx_power_rsp__free_unpacked,		ZSTACK_CMD_IDS__SYS_SET_TX_POWER_RSP},
 {&sys_nwk_info_read_rsp__descriptor,	  (pb__unpack)sys_nwk_info_read_rsp__unpack,	  (pb__free_unpacked)sys_nwk_info_read_rsp__free_unpacked,		ZSTACK_CMD_IDS__SYS_NWK_INFO_READ_RSP},
//  {&sys_nwk_info_write_rsp__descriptor,	  (pb__unpack)sys_nwk_info_write_rsp__unpack,	  (pb__free_unpacked)sys_nwk_info_write_rsp__free_unpacked, 	ZSTACK_CMD_IDS__SYS_NWK_INFO_WRITE_RSP},
 {&dev_start_req__descriptor,			  (pb__unpack)dev_start_req__unpack,			  (pb__free_unpacked)dev_start_req__free_unpacked,				ZSTACK_CMD_IDS__DEV_START_REQ},
 {&dev_nwk_disc_req__descriptor,   (pb__unpack)dev_nwk_disc_req__unpack,   (pb__free_unpacked)dev_nwk_disc_req__free_unpacked,		ZSTACK_CMD_IDS__DEV_NWK_DISCOVERY_REQ},
 {&dev_join_req__descriptor, 			  (pb__unpack)dev_join_req__unpack, 			  (pb__free_unpacked)dev_join_req__free_unpacked,				ZSTACK_CMD_IDS__DEV_JOIN_REQ},
 {&dev_rejoin_req__descriptor,			  (pb__unpack)dev_rejoin_req__unpack,			  (pb__free_unpacked)dev_rejoin_req__free_unpacked, 			ZSTACK_CMD_IDS__DEV_REJOIN_REQ},
 {&dev_zdocbreq__descriptor, 		  (pb__unpack)dev_zdocbreq__unpack, 		  (pb__free_unpacked)dev_zdocbreq__free_unpacked,			ZSTACK_CMD_IDS__DEV_ZDO_CBS_REQ},
 {&dev_nwk_route_req__descriptor,		  (pb__unpack)dev_nwk_route_req__unpack,		  (pb__free_unpacked)dev_nwk_route_req__free_unpacked,			ZSTACK_CMD_IDS__DEV_NWK_ROUTE_REQ},
 {&dev_nwk_check_route_req__descriptor,	  (pb__unpack)dev_nwk_check_route_req__unpack,	  (pb__free_unpacked)dev_nwk_check_route_req__free_unpacked,	ZSTACK_CMD_IDS__DEV_NWK_CHECK_ROUTE_REQ},
 {&dev_jammer_ind__descriptor,			  (pb__unpack)dev_jammer_ind__unpack,			  (pb__free_unpacked)dev_jammer_ind__free_unpacked, 			ZSTACK_CMD_IDS__DEV_JAMMER_IND},
 {&aps_remove_group__descriptor, 		  (pb__unpack)aps_remove_group__unpack, 		  (pb__free_unpacked)aps_remove_group__free_unpacked,			ZSTACK_CMD_IDS__APS_REMOVE_GROUP},
 {&aps_remove_all_groups__descriptor,	  (pb__unpack)aps_remove_all_groups__unpack,	  (pb__free_unpacked)aps_remove_all_groups__free_unpacked,		ZSTACK_CMD_IDS__APS_REMOVE_ALL_GROUPS},
 {&aps_find_all_groups_req__descriptor,	  (pb__unpack)aps_find_all_groups_req__unpack,	  (pb__free_unpacked)aps_find_all_groups_req__free_unpacked,	ZSTACK_CMD_IDS__APS_FIND_ALL_GROUPS_REQ},
 {&aps_find_all_groups_rsp__descriptor,	  (pb__unpack)aps_find_all_groups_rsp__unpack,	  (pb__free_unpacked)aps_find_all_groups_rsp__free_unpacked,	ZSTACK_CMD_IDS__APS_FIND_ALL_GROUPS_RSP},
 {&aps_find_group_req__descriptor,		  (pb__unpack)aps_find_group_req__unpack,		  (pb__free_unpacked)aps_find_group_req__free_unpacked, 		ZSTACK_CMD_IDS__APS_FIND_GROUP_REQ},
 {&aps_find_group_rsp__descriptor,		  (pb__unpack)aps_find_group_rsp__unpack,		  (pb__free_unpacked)aps_find_group_rsp__free_unpacked, 		ZSTACK_CMD_IDS__APS_FIND_GROUP_RSP},
 {&aps_add_group__descriptor,			  (pb__unpack)aps_add_group__unpack,			  (pb__free_unpacked)aps_add_group__free_unpacked,				ZSTACK_CMD_IDS__APS_ADD_GROUP},
 {&aps_count_all_groups__descriptor, 	  (pb__unpack)aps_count_all_groups__unpack, 	  (pb__free_unpacked)aps_count_all_groups__free_unpacked,		ZSTACK_CMD_IDS__APS_COUNT_ALL_GROUPS},
 {&af_register_req__descriptor,			  (pb__unpack)af_register_req__unpack,			  (pb__free_unpacked)af_register_req__free_unpacked,			ZSTACK_CMD_IDS__AF_REGISTER_REQ},
 {&af_un_register_req__descriptor,		  (pb__unpack)af_un_register_req__unpack,		  (pb__free_unpacked)af_un_register_req__free_unpacked, 		ZSTACK_CMD_IDS__AF_UNREGISTER_REQ},
 {&af_data_req__descriptor,				  (pb__unpack)af_data_req__unpack,				  (pb__free_unpacked)af_data_req__free_unpacked,				ZSTACK_CMD_IDS__AF_DATA_REQ},
 {&af_inter_pan_ctl_req__descriptor, 	  (pb__unpack)af_inter_pan_ctl_req__unpack, 	  (pb__free_unpacked)af_inter_pan_ctl_req__free_unpacked,		ZSTACK_CMD_IDS__AF_INTERPAN_CTL_REQ},
 {&af_config_get_req__descriptor,		  (pb__unpack)af_config_get_req__unpack,		  (pb__free_unpacked)af_config_get_req__free_unpacked,			ZSTACK_CMD_IDS__AF_CONFIG_GET_REQ},
 {&af_config_set_req__descriptor,		  (pb__unpack)af_config_set_req__unpack,		  (pb__free_unpacked)af_config_set_req__free_unpacked,			ZSTACK_CMD_IDS__AF_CONFIG_SET_REQ},
 {&zdo_nwk_addr_req__descriptor, 		  (pb__unpack)zdo_nwk_addr_req__unpack, 		  (pb__free_unpacked)zdo_nwk_addr_req__free_unpacked,			ZSTACK_CMD_IDS__ZDO_NWK_ADDR_REQ},
 {&zdo_ieee_addr_req__descriptor,		  (pb__unpack)zdo_ieee_addr_req__unpack,		  (pb__free_unpacked)zdo_ieee_addr_req__free_unpacked,			ZSTACK_CMD_IDS__ZDO_IEEE_ADDR_REQ},
 {&zdo_node_desc_req__descriptor,		  (pb__unpack)zdo_node_desc_req__unpack,		  (pb__free_unpacked)zdo_node_desc_req__free_unpacked,			ZSTACK_CMD_IDS__ZDO_NODE_DESC_REQ},
 {&zdo_power_desc_req__descriptor,		  (pb__unpack)zdo_power_desc_req__unpack,		  (pb__free_unpacked)zdo_power_desc_req__free_unpacked, 		ZSTACK_CMD_IDS__ZDO_POWER_DESC_REQ},
 {&zdo_simple_desc_req__descriptor,		  (pb__unpack)zdo_simple_desc_req__unpack,		  (pb__free_unpacked)zdo_simple_desc_req__free_unpacked,		ZSTACK_CMD_IDS__ZDO_SIMPLE_DESC_REQ},
 {&zdo_active_endpoint_req__descriptor,	  (pb__unpack)zdo_active_endpoint_req__unpack,	  (pb__free_unpacked)zdo_active_endpoint_req__free_unpacked,	ZSTACK_CMD_IDS__ZDO_ACTIVE_ENDPOINT_REQ},
 {&zdo_match_desc_req__descriptor,		  (pb__unpack)zdo_match_desc_req__unpack,		  (pb__free_unpacked)zdo_match_desc_req__free_unpacked, 		ZSTACK_CMD_IDS__ZDO_MATCH_DESC_REQ},
 {&zdo_complex_desc_req__descriptor, 	  (pb__unpack)zdo_complex_desc_req__unpack, 	  (pb__free_unpacked)zdo_complex_desc_req__free_unpacked,		ZSTACK_CMD_IDS__ZDO_COMPLEX_DESC_REQ},
 {&zdo_server_disc_req__descriptor,		  (pb__unpack)zdo_server_disc_req__unpack,		  (pb__free_unpacked)zdo_server_disc_req__free_unpacked,		ZSTACK_CMD_IDS__ZDO_SERVER_DISC_REQ},
 {&zdo_end_device_bind_req__descriptor,	  (pb__unpack)zdo_end_device_bind_req__unpack,	  (pb__free_unpacked)zdo_end_device_bind_req__free_unpacked,	ZSTACK_CMD_IDS__ZDO_END_DEVICE_BIND_REQ},
 {&zdo_bind_req__descriptor, 			  (pb__unpack)zdo_bind_req__unpack, 			  (pb__free_unpacked)zdo_bind_req__free_unpacked,				ZSTACK_CMD_IDS__ZDO_BIND_REQ},
 {&zdo_unbind_req__descriptor,			  (pb__unpack)zdo_unbind_req__unpack,			  (pb__free_unpacked)zdo_unbind_req__free_unpacked, 			ZSTACK_CMD_IDS__ZDO_UNBIND_REQ},
 {&zdo_mgmt_nwk_disc_req__descriptor,	  (pb__unpack)zdo_mgmt_nwk_disc_req__unpack,	  (pb__free_unpacked)zdo_mgmt_nwk_disc_req__free_unpacked,		ZSTACK_CMD_IDS__ZDO_MGMT_NWK_DISC_REQ},
 {&zdo_mgmt_lqi_req__descriptor, 		  (pb__unpack)zdo_mgmt_lqi_req__unpack, 		  (pb__free_unpacked)zdo_mgmt_lqi_req__free_unpacked,			ZSTACK_CMD_IDS__ZDO_MGMT_LQI_REQ},
 {&zdo_mgmt_rtg_req__descriptor, 		  (pb__unpack)zdo_mgmt_rtg_req__unpack, 		  (pb__free_unpacked)zdo_mgmt_rtg_req__free_unpacked,			ZSTACK_CMD_IDS__ZDO_MGMT_RTG_REQ},
 {&zdo_mgmt_bind_req__descriptor,		  (pb__unpack)zdo_mgmt_bind_req__unpack,		  (pb__free_unpacked)zdo_mgmt_bind_req__free_unpacked,			ZSTACK_CMD_IDS__ZDO_MGMT_BIND_REQ},
 {&zdo_mgmt_leave_req__descriptor,		  (pb__unpack)zdo_mgmt_leave_req__unpack,		  (pb__free_unpacked)zdo_mgmt_leave_req__free_unpacked, 		ZSTACK_CMD_IDS__ZDO_MGMT_LEAVE_REQ},
 {&zdo_mgmt_direct_join_req__descriptor,   (pb__unpack)zdo_mgmt_direct_join_req__unpack,   (pb__free_unpacked)zdo_mgmt_direct_join_req__free_unpacked,	ZSTACK_CMD_IDS__ZDO_MGMT_DIRECT_JOIN_REQ},
 {&zdo_mgmt_permit_join_req__descriptor,   (pb__unpack)zdo_mgmt_permit_join_req__unpack,   (pb__free_unpacked)zdo_mgmt_permit_join_req__free_unpacked,	ZSTACK_CMD_IDS__ZDO_MGMT_PERMIT_JOIN_REQ},
 {&zdo_mgmt_nwk_update_req__descriptor,	  (pb__unpack)zdo_mgmt_nwk_update_req__unpack,	  (pb__free_unpacked)zdo_mgmt_nwk_update_req__free_unpacked,	ZSTACK_CMD_IDS__ZDO_MGMT_NWK_UPDATE_REQ},
 {&zdo_device_announce_req__descriptor,	  (pb__unpack)zdo_device_announce_req__unpack,	  (pb__free_unpacked)zdo_device_announce_req__free_unpacked,	ZSTACK_CMD_IDS__ZDO_DEVICE_ANNOUNCE_REQ},
 {&zdo_user_desc_set_req__descriptor,  (pb__unpack)zdo_user_desc_set_req__unpack,  (pb__free_unpacked)zdo_user_desc_set_req__free_unpacked,		ZSTACK_CMD_IDS__ZDO_USER_DESCR_SET_REQ},
 {&zdo_user_desc_req__descriptor,		  (pb__unpack)zdo_user_desc_req__unpack,		  (pb__free_unpacked)zdo_user_desc_req__free_unpacked,			ZSTACK_CMD_IDS__ZDO_USER_DESC_REQ},
 {&zdo_device_announce_ind__descriptor,		  (pb__unpack)zdo_device_announce_ind__unpack,		  (pb__free_unpacked)zdo_device_announce_ind__free_unpacked,		ZSTACK_CMD_IDS__ZDO_DEVICE_ANNOUNCE},
 {&zdo_device_announce_shadow_ind__descriptor,		  (pb__unpack)zdo_device_announce_shadow_ind__unpack,		  (pb__free_unpacked)zdo_device_announce_shadow_ind__free_unpacked,		ZSTACK_CMD_IDS__ZDO_DEVICE_ANNOUNCE_SHADOW},
 {&zdo_nwk_addr_rsp_ind__descriptor, 		  (pb__unpack)zdo_nwk_addr_rsp_ind__unpack, 		  (pb__free_unpacked)zdo_nwk_addr_rsp_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_NWK_ADDR_RSP},
 {&zdo_ieee_addr_rsp_ind__descriptor,		  (pb__unpack)zdo_ieee_addr_rsp_ind__unpack,		  (pb__free_unpacked)zdo_ieee_addr_rsp_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_IEEE_ADDR_RSP},
 {&zdo_node_desc_rsp_ind__descriptor,		  (pb__unpack)zdo_node_desc_rsp_ind__unpack,		  (pb__free_unpacked)zdo_node_desc_rsp_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_NODE_DESC_RSP},
 {&zdo_power_desc_rsp_ind__descriptor,		  (pb__unpack)zdo_power_desc_rsp_ind__unpack,		  (pb__free_unpacked)zdo_power_desc_rsp_ind__free_unpacked, 		ZSTACK_CMD_IDS__ZDO_POWER_DESC_RSP},
 {&zdo_simple_desc_rsp_ind__descriptor,		  (pb__unpack)zdo_simple_desc_rsp_ind__unpack,		  (pb__free_unpacked)zdo_simple_desc_rsp_ind__free_unpacked,		ZSTACK_CMD_IDS__ZDO_SIMPLE_DESC_RSP},
 {&zdo_active_endpoints_rsp_ind__descriptor, 	  (pb__unpack)zdo_active_endpoints_rsp_ind__unpack, 	  (pb__free_unpacked)zdo_active_endpoints_rsp_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_ACTIVE_EP_RSP},
 {&zdo_match_desc_rsp_ind__descriptor,		  (pb__unpack)zdo_match_desc_rsp_ind__unpack,		  (pb__free_unpacked)zdo_match_desc_rsp_ind__free_unpacked, 		ZSTACK_CMD_IDS__ZDO_MATCH_DESC_RSP},
//  {&zdo_complex_desc_rsp_ind__descriptor, 	  (pb__unpack)zdo_complex_desc_rsp_ind__unpack, 	  (pb__free_unpacked)zdo_complex_desc_rsp_ind__free_unpacked,		ZSTACK_CMD_IDS__ZDO_COMPLEX_DESC_RSP},
 {&zdo_user_desc_rsp_ind__descriptor,		  (pb__unpack)zdo_user_desc_rsp_ind__unpack,		  (pb__free_unpacked)zdo_user_desc_rsp_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_USER_DESC_RSP},
//  {&zdo_user_desc_set_rsp_ind__descriptor,	  (pb__unpack)zdo_user_desc_set_rsp_ind__unpack,	  (pb__free_unpacked)zdo_user_desc_set_rsp_ind__free_unpacked,		ZSTACK_CMD_IDS__ZDO_USER_DESC_SET_RSP},
 {&zdo_server_discovery_rsp_ind__descriptor, 	  (pb__unpack)zdo_server_discovery_rsp_ind__unpack, 	  (pb__free_unpacked)zdo_server_discovery_rsp_ind__free_unpacked,		ZSTACK_CMD_IDS__ZDO_SERVER_DISC_RSP},
 {&zdo_end_device_bind_rsp_ind__descriptor,	  (pb__unpack)zdo_end_device_bind_rsp_ind__unpack,	  (pb__free_unpacked)zdo_end_device_bind_rsp_ind__free_unpacked,	ZSTACK_CMD_IDS__ZDO_END_DEVICE_BIND_RSP},
 {&zdo_bind_rsp_ind__descriptor, 			  (pb__unpack)zdo_bind_rsp_ind__unpack, 			  (pb__free_unpacked)zdo_bind_rsp_ind__free_unpacked,				ZSTACK_CMD_IDS__ZDO_BIND_RSP},
 {&zdo_unbind_rsp_ind__descriptor,			  (pb__unpack)zdo_unbind_rsp_ind__unpack,			  (pb__free_unpacked)zdo_unbind_rsp_ind__free_unpacked, 			ZSTACK_CMD_IDS__ZDO_UNBIND_RSP},
 {&zdo_mgmt_nwk_disc_rsp_ind__descriptor,	  (pb__unpack)zdo_mgmt_nwk_disc_rsp_ind__unpack,	  (pb__free_unpacked)zdo_mgmt_nwk_disc_rsp_ind__free_unpacked,		ZSTACK_CMD_IDS__ZDO_MGMT_NWK_DISC_RSP},
 {&zdo_mgmt_lqi_rsp_ind__descriptor, 		  (pb__unpack)zdo_mgmt_lqi_rsp_ind__unpack, 		  (pb__free_unpacked)zdo_mgmt_lqi_rsp_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_MGMT_LQI_RSP},
 {&zdo_mgmt_rtg_rsp_ind__descriptor, 		  (pb__unpack)zdo_mgmt_rtg_rsp_ind__unpack, 		  (pb__free_unpacked)zdo_mgmt_rtg_rsp_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_MGMT_RTG_RSP},
 {&zdo_mgmt_bind_rsp_ind__descriptor,		  (pb__unpack)zdo_mgmt_bind_rsp_ind__unpack,		  (pb__free_unpacked)zdo_mgmt_bind_rsp_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_MGMT_BIND_RSP},
 {&zdo_mgmt_leave_rsp_ind__descriptor,		  (pb__unpack)zdo_mgmt_leave_rsp_ind__unpack,		  (pb__free_unpacked)zdo_mgmt_leave_rsp_ind__free_unpacked, 		ZSTACK_CMD_IDS__ZDO_MGMT_LEAVE_RSP},
 {&zdo_mgmt_direct_join_rsp_ind__descriptor,   (pb__unpack)zdo_mgmt_direct_join_rsp_ind__unpack,   (pb__free_unpacked)zdo_mgmt_direct_join_rsp_ind__free_unpacked,	ZSTACK_CMD_IDS__ZDO_MGMT_DIRECT_JOIN_RSP},
 {&zdo_mgmt_permit_join_rsp_ind__descriptor,   (pb__unpack)zdo_mgmt_permit_join_rsp_ind__unpack,   (pb__free_unpacked)zdo_mgmt_permit_join_rsp_ind__free_unpacked,	ZSTACK_CMD_IDS__ZDO_MGMT_PERMIT_JOIN_RSP},
 {&zdo_mgmt_nwk_update_notify_ind__descriptor, (pb__unpack)zdo_mgmt_nwk_update_notify_ind__unpack, (pb__free_unpacked)zdo_mgmt_nwk_update_notify_ind__free_unpacked, ZSTACK_CMD_IDS__ZDO_MGMT_NWK_UPDATE_NOTIFY},
 {&zdo_end_device_timeout_rsp_ind__descriptor, (pb__unpack)zdo_end_device_timeout_rsp_ind__unpack, (pb__free_unpacked)zdo_end_device_timeout_rsp_ind__free_unpacked, ZSTACK_CMD_IDS__ZDO_END_DEVICE_TIMEOUT_RSP},
 {&zdo_src_rtg_ind__descriptor,			  (pb__unpack)zdo_src_rtg_ind__unpack,			  (pb__free_unpacked)zdo_src_rtg_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_SRC_RTG_IND},
 {&zdo_concentrator_ind__descriptor, 	  (pb__unpack)zdo_concentrator_ind__unpack, 	  (pb__free_unpacked)zdo_concentrator_ind__free_unpacked,		ZSTACK_CMD_IDS__ZDO_CONCENTRATOR_IND},
 {&zdo_nwk_disc_cnf__descriptor, 		  (pb__unpack)zdo_nwk_disc_cnf__unpack, 		  (pb__free_unpacked)zdo_nwk_disc_cnf__free_unpacked,			ZSTACK_CMD_IDS__ZDO_NWK_DISC_CNF},
 {&zdo_beacon_notify_ind__descriptor,	  (pb__unpack)zdo_beacon_notify_ind__unpack,	  (pb__free_unpacked)zdo_beacon_notify_ind__free_unpacked,		ZSTACK_CMD_IDS__ZDO_BEACON_NOTIFY_IND},
 {&zdo_join_cnf__descriptor, 			  (pb__unpack)zdo_join_cnf__unpack, 			  (pb__free_unpacked)zdo_join_cnf__free_unpacked,				ZSTACK_CMD_IDS__ZDO_JOIN_CNF},
 {&zdo_leave_cnf__descriptor,			  (pb__unpack)zdo_leave_cnf__unpack,			  (pb__free_unpacked)zdo_leave_cnf__free_unpacked,				ZSTACK_CMD_IDS__ZDO_LEAVE_CNF},
 {&zdo_leave_ind__descriptor,			  (pb__unpack)zdo_leave_ind__unpack,			  (pb__free_unpacked)zdo_leave_ind__free_unpacked,				ZSTACK_CMD_IDS__ZDO_LEAVE_IND},
 {&af_config_get_rsp__descriptor,		  (pb__unpack)af_config_get_rsp__unpack,		  (pb__free_unpacked)af_config_get_rsp__free_unpacked,			ZSTACK_CMD_IDS__AF_CONFIG_GET_RSP},
 {&sys_reset_ind__descriptor,			  (pb__unpack)sys_reset_ind__unpack,			  (pb__free_unpacked)sys_reset_ind__free_unpacked,				ZSTACK_CMD_IDS__SYS_RESET_IND},
 {&af_data_confirm_ind__descriptor,		  (pb__unpack)af_data_confirm_ind__unpack,		  (pb__free_unpacked)af_data_confirm_ind__free_unpacked,		ZSTACK_CMD_IDS__AF_DATA_CONFIRM_IND},
 {&af_incoming_msg_ind__descriptor,		  (pb__unpack)af_incoming_msg_ind__unpack,		  (pb__free_unpacked)af_incoming_msg_ind__free_unpacked,		ZSTACK_CMD_IDS__AF_INCOMING_MSG_IND},
 {&gp_sec_req_incoming_msg_ind__descriptor,          (pb__unpack)gp_sec_req_incoming_msg_ind__unpack,        (pb__free_unpacked)gp_sec_req_incoming_msg_ind__free_unpacked,        ZSTACK_CMD_IDS__GP_SEC_REQ_INCOMING_MSG},
 {&gp_sec_rsp_msg__descriptor,            (pb__unpack)gp_sec_rsp_msg__unpack,             (pb__free_unpacked)gp_sec_rsp_msg__free_unpacked,             ZSTACK_CMD_IDS__GP_SEC_RSP_MSG},
 {&gp_data_ind_incoming_msg_ind__descriptor,           (pb__unpack)gp_data_ind_incoming_msg_ind__unpack,            (pb__free_unpacked)gp_data_ind_incoming_msg_ind__free_unpacked,            ZSTACK_CMD_IDS__GP_DATA_IND_INCOMING_MSG},
 {&af_reflect_error_ind__descriptor, 	  (pb__unpack)af_reflect_error_ind__unpack, 	  (pb__free_unpacked)af_reflect_error_ind__free_unpacked,		ZSTACK_CMD_IDS__AF_REFLECT_ERROR_IND},
 {&dev_state_change_ind__descriptor, 	  (pb__unpack)dev_state_change_ind__unpack, 	  (pb__free_unpacked)dev_state_change_ind__free_unpacked,		ZSTACK_CMD_IDS__DEV_STATE_CHANGE_IND},
 {&zdo_tc_device_ind__descriptor,		  (pb__unpack)zdo_tc_device_ind__unpack,		  (pb__free_unpacked)zdo_tc_device_ind__free_unpacked,			ZSTACK_CMD_IDS__ZDO_TC_DEVICE_IND},
 {&dev_permit_join_ind__descriptor,		  (pb__unpack)dev_permit_join_ind__unpack,		  (pb__free_unpacked)dev_permit_join_ind__free_unpacked,		ZSTACK_CMD_IDS__DEV_PERMIT_JOIN_IND},
 {&sec_nwk_key_get_req__descriptor,		  (pb__unpack)sec_nwk_key_get_req__unpack,		  (pb__free_unpacked)sec_nwk_key_get_req__free_unpacked,		ZSTACK_CMD_IDS__SEC_NWK_KEY_GET_REQ},
 {&sec_nwk_key_set_req__descriptor,		  (pb__unpack)sec_nwk_key_set_req__unpack,		  (pb__free_unpacked)sec_nwk_key_set_req__free_unpacked,		ZSTACK_CMD_IDS__SEC_NWK_KEY_SET_REQ},
 {&sec_nwk_key_update_req__descriptor,	  (pb__unpack)sec_nwk_key_update_req__unpack,	  (pb__free_unpacked)sec_nwk_key_update_req__free_unpacked, 	ZSTACK_CMD_IDS__SEC_NWK_KEY_UPDATE_REQ},
 {&sec_nwk_key_switch_req__descriptor,	  (pb__unpack)sec_nwk_key_switch_req__unpack,	  (pb__free_unpacked)sec_nwk_key_switch_req__free_unpacked, 	ZSTACK_CMD_IDS__SEC_NWK_KEY_SWITCH_REQ},
 {&sec_aps_link_key_get_req__descriptor,   (pb__unpack)sec_aps_link_key_get_req__unpack,   (pb__free_unpacked)sec_aps_link_key_get_req__free_unpacked,	ZSTACK_CMD_IDS__SEC_APS_LINKKEY_GET_REQ},
 {&sec_aps_link_key_set_req__descriptor,   (pb__unpack)sec_aps_link_key_set_req__unpack,   (pb__free_unpacked)sec_aps_link_key_set_req__free_unpacked,	ZSTACK_CMD_IDS__SEC_APS_LINKKEY_SET_REQ},
 {&sec_aps_link_key_remove_req__descriptor,(pb__unpack)sec_aps_link_key_remove_req__unpack,(pb__free_unpacked)sec_aps_link_key_remove_req__free_unpacked,ZSTACK_CMD_IDS__SEC_APS_LINKKEY_REMOVE_REQ},
 {&sec_aps_remove_req__descriptor,		   (pb__unpack)sec_aps_remove_req__unpack,		   (pb__free_unpacked)sec_aps_remove_req__free_unpacked, 		ZSTACK_CMD_IDS__SEC_APS_REMOVE_REQ},
 {&sec_nwk_key_get_rsp__descriptor,		   (pb__unpack)sec_nwk_key_get_rsp__unpack,		   (pb__free_unpacked)sec_nwk_key_get_rsp__free_unpacked,		ZSTACK_CMD_IDS__SEC_NWK_KEY_GET_RSP},
 {&sec_aps_link_key_get_rsp__descriptor,   (pb__unpack)sec_aps_link_key_get_rsp__unpack,   (pb__free_unpacked)sec_aps_link_key_get_rsp__free_unpacked,	ZSTACK_CMD_IDS__SEC_APS_LINKKEY_GET_RSP},
 {&app_cnf_start_commissioning_req__descriptor,   (pb__unpack)app_cnf_start_commissioning_req__unpack,   (pb__free_unpacked)app_cnf_start_commissioning_req__free_unpacked,	ZSTACK_CMD_IDS__APP_CNF_START_COMMISSIONING_REQ},
 {&app_cnf_add_install_code_req__descriptor,   (pb__unpack)app_cnf_add_install_code_req__unpack,   (pb__free_unpacked)app_cnf_add_install_code_req__free_unpacked,	ZSTACK_CMD_IDS__APP_CNF_ADD_INSTALL_CODE_REQ},
 {&util_set_dev_nwk_info_req__descriptor,   (pb__unpack)util_set_dev_nwk_info_req__unpack,   (pb__free_unpacked)util_set_dev_nwk_info_req__free_unpacked,    ZSTACK_CMD_IDS__UTIL_SET_DEV_NWK_INFO_REQ},
 {&zcl_action_req__descriptor,   		  (pb__unpack)zcl_action_req__unpack,	 		  (pb__free_unpacked)zcl_action_req__free_unpacked, 			ZSTACK_CMD_IDS__ZCL_ACTION_REQ},
 {&zcl_action_rsp__descriptor,   		  (pb__unpack)zcl_action_rsp__unpack,	 		  (pb__free_unpacked)zcl_action_rsp__free_unpacked, 			ZSTACK_CMD_IDS__ZCL_ACTION_RSP},
 {&app_cnf_commissioning_notification__descriptor,   (pb__unpack)app_cnf_commissioning_notification__unpack,	 (pb__free_unpacked)app_cnf_commissioning_notification__free_unpacked, ZSTACK_CMD_IDS__APP_CNF_COMMISSIONING_NOTIFICATION},
 {&zstack_default_rsp__descriptor,        (pb__unpack)zstack_default_rsp__unpack,         (pb__free_unpacked)zstack_default_rsp__free_unpacked,         ZSTACK_CMD_IDS__ZSTACK_DEFAULT_RSP},
 {&send_get_dev_nwk_info_req__descriptor,              (pb__unpack)send_get_dev_nwk_info_req__unpack,              (pb__free_unpacked)send_get_dev_nwk_info_req__free_unpacked,              ZSTACK_CMD_IDS__SEND_GET_DEV_NWK_INFO_REQ},

};

pb_pointer_list_t zstack_pb_pointer_list = {ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF, NULL, sizeof(_zstack_pb_pointer_list) / sizeof(_zstack_pb_pointer_list[0]), _zstack_pb_pointer_list};
 
