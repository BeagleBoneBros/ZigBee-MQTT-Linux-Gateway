/*******************************************************************************
 Filename:      commissioning_engine.c
 Revised:        $Date$
 Revision:       $Revision$

 Description:	 Commissioning Engine handles the addition/deletion of devices in the network.


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

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "types.h"
#include "data_structures.h"
#include "socket_interface.h"
#include "user_interface.h"
#include "nwkmgr.pb-c.h"
#include "commissioning_engine.h"
#include "timer_utils.h"

/*******************************************************************************
 * Locals
 ******************************************************************************/
static uint8_t requested_join_time = 0;
static tu_timer_t pj_timer = TIMER_RESET_VALUE;

/*******************************************************************************
 * Variables
 ******************************************************************************/
tu_timer_t finding_and_binding_timer = TIMER_RESET_VALUE;
tu_timer_t finding_and_binding_start = TIMER_RESET_VALUE;
//tu_timer_t finding_and_binding_bind_delay = TIMER_RESET_VALUE;

/*******************************************************************************
 * Global
 ******************************************************************************/
uint8_t comm_status = 0;

uint8_t bdb_sequence = 0;

uint8_t FBInitiatorTimeShadow = 0;

uint8_t gw_fb_endpoint = 0;

uint8_t gw_bindTableFull = 0;

uint8_t gw_addGroups = 0;

uint16_t bdbCommissioningGroupID = 0xFFFF;

uint64_t gw_ieeeAddr = 0;

#define FINDING_AND_BINDING_IN_PROGRESS 			4
#define FINDING_AND_BINDING_IDENTIFY_UNSUPPORTED 	3
#define FINDING_AND_BINDING_FAILURE				 	2
#define FINDING_AND_BINDING_SUCCESS 				1
#define FINDING_AND_BINDING_READY 					0

#define FINDING_AND_BINDING_DURATION				180		//Seconds
#define FINDING_AND_BINDING_PERIOD					15	 	//Seconds

#define FINDING_AND_BINDING_MAX_RETRIES				3

#define TRUE 										1
#define FALSE 										0

#define FINDING_AND_BINDING_TIMER_ACTIVE			1
#define FINDING_AND_BINDING_TIMER_INACTIVE			0


#define FindingAndBindingTimeout 150000 //milliseconds

typedef struct gwBindQueue_tag
{
  zb_addr_t *pSrcAddr;
  zb_addr_t *pDstAddr;
  uint16_t clusterId;
  uint8_t bindStatus;
  uint8_t isComplete;
  struct gwBindQueue_tag *next;
} gwBindQueue;

gwBindQueue *p_fb_bind_list_head;
gwBindQueue *p_fb_bind_list_current;
gwBindQueue *p_fb_bind_list_processing;


typedef struct gwSimpleDescriptor_tag
{
  uint32_t endpointid;
  uint32_t profileid;
  uint32_t deviceid;
  uint32_t devicever;
  uint8_t n_inputclusters;
  uint16_t *inputclusters;
  uint8_t n_outputclusters;
  uint16_t *outputclusters;
  struct gwSimpleDescriptor_tag *next;
} gwSimpleDescriptor;

static gwSimpleDescriptor *p_gw_simple_descriptor;

typedef struct FBRespondantsTable_tag
{
  uint8_t  attempts;
  uint16_t nwkAddress;
  uint8_t  endpoint;
  uint8_t  hasSimpleDesc;
  gwSimpleDescriptor *simpleDesc;
  uint8_t  hasIeeeAddress;
  uint64_t ieeeAddress;
  uint8_t  isComplete;
  struct FBRespondantsTable_tag *next;
} FBRespondantsTable;

static FBRespondantsTable *p_gw_respondants_table_head = NULL;
static FBRespondantsTable *p_respondants_table_current= NULL;
//static FBRespondantsTable *p_respondants_table_addGroup= NULL;

const uint16_t nwk_ZclType1Clusters[] =
{
  ZCL_CLUSTER_ID_GEN_SCENES,
  ZCL_CLUSTER_ID_GEN_ON_OFF,
  ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL,
  ZCL_CLUSTER_ID_GEN_ALARMS,
  ZCL_CLUSTER_ID_GEN_PARTITION,
  ZCL_CLUSTER_ID_CLOSURES_WINDOW_COVERING,
  ZCL_CLUSTER_ID_HVAC_FAN_CONTROL,
  ZCL_CLUSTER_ID_HVAC_DIHUMIDIFICATION_CONTROL,
  ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL,
  ZCL_CLUSTER_ID_LIGHTING_BALLAST_CONFIG,
  ZCL_CLUSTER_ID_SS_IAS_ACE,
  ZCL_CLUSTER_ID_SS_IAS_WD,
  ZCL_CLUSTER_ID_PI_GENERIC_TUNNEL,
  ZCL_CLUSTER_ID_PI_BACNET_PROTOCOL_TUNNEL,
  ZCL_CLUSTER_ID_HA_ELECTRICAL_MEASUREMENT,
  ZCL_CLUSTER_ID_PI_11073_PROTOCOL_TUNNEL,
};


const uint16_t nwk_ZclType2Clusters[] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG,
  ZCL_CLUSTER_ID_GEN_TIME,
  ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC,
  ZCL_CLUSTER_ID_GEN_ANALOG_OUTPUT_BASIC,
  ZCL_CLUSTER_ID_GEN_ANALOG_VALUE_BASIC,
  ZCL_CLUSTER_ID_GEN_BINARY_INPUT_BASIC,
  ZCL_CLUSTER_ID_GEN_BINARY_OUTPUT_BASIC,
  ZCL_CLUSTER_ID_GEN_BINARY_VALUE_BASIC,
  ZCL_CLUSTER_ID_GEN_MULTISTATE_INPUT_BASIC,
  ZCL_CLUSTER_ID_GEN_MULTISTATE_OUTPUT_BASIC,
  ZCL_CLUSTER_ID_GEN_MULTISTATE_VALUE_BASIC,
  ZCL_CLUSTER_ID_OTA,
  ZCL_CLUSTER_ID_GEN_APPLIANCE_CONTROL,
  ZCL_CLUSTER_ID_CLOSURES_SHADE_CONFIG,
  ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
  ZCL_CLUSTER_ID_HVAC_PUMP_CONFIG_CONTROL,
  ZCL_CLUSTER_ID_HVAC_THERMOSTAT,
  ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
  ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT,
  ZCL_CLUSTER_ID_MS_ILLUMINANCE_LEVEL_SENSING_CONFIG,
  ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT,
  ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT,
  ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY,
  ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING,
  ZCL_CLUSTER_ID_SS_IAS_ZONE,
  ZCL_CLUSTER_ID_PI_ANALOG_INPUT_BACNET_REG,
  ZCL_CLUSTER_ID_PI_ANALOG_INPUT_BACNET_EXT,
  ZCL_CLUSTER_ID_PI_ANALOG_OUTPUT_BACNET_REG,
  ZCL_CLUSTER_ID_PI_ANALOG_OUTPUT_BACNET_EXT,
  ZCL_CLUSTER_ID_PI_ANALOG_VALUE_BACNET_REG,
  ZCL_CLUSTER_ID_PI_ANALOG_VALUE_BACNET_EXT,
  ZCL_CLUSTER_ID_PI_BINARY_INPUT_BACNET_REG,
  ZCL_CLUSTER_ID_PI_BINARY_INPUT_BACNET_EXT,
  ZCL_CLUSTER_ID_PI_BINARY_OUTPUT_BACNET_REG,
  ZCL_CLUSTER_ID_PI_BINARY_OUTPUT_BACNET_EXT,
  ZCL_CLUSTER_ID_PI_BINARY_VALUE_BACNET_REG,
  ZCL_CLUSTER_ID_PI_BINARY_VALUE_BACNET_EXT,
  ZCL_CLUSTER_ID_PI_MULTISTATE_INPUT_BACNET_REG,
  ZCL_CLUSTER_ID_PI_MULTISTATE_INPUT_BACNET_EXT,
  ZCL_CLUSTER_ID_PI_MULTISTATE_OUTPUT_BACNET_REG,
  ZCL_CLUSTER_ID_PI_MULTISTATE_OUTPUT_BACNET_EXT,
  ZCL_CLUSTER_ID_PI_MULTISTATE_VALUE_BACNET_REG,
  ZCL_CLUSTER_ID_PI_MULTISTATE_VALUE_BACNET_EXT,
  ZCL_CLUSTER_ID_HA_APPLIANCE_IDENTIFICATION,
  ZCL_CLUSTER_ID_HA_METER_IDENTIFICATION,
  ZCL_CLUSTER_ID_HA_APPLIANCE_EVENTS_ALERTS,
  ZCL_CLUSTER_ID_HA_APPLIANCE_STATISTICS,
};

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/
void dev_finding_and_binding_timer_handler(void * arg);
void dev_finding_and_binding_start_handler(void * arg);
//void dev_finding_and_binding_bind_delay_handler(void * arg);


void dev_delete_respondant_table_entry(FBRespondantsTable *p_respondants_table_entry);
void dev_clear_respondant_table_entry(FBRespondantsTable *p_respondants_table_entry);

void dev_delete_binding_list_entry(gwBindQueue *p_bind_list_entry);
void dev_clear_bind_list_entry(gwBindQueue *p_bind_list_entry);

/*******************************************************************************
 * Functions
 ******************************************************************************/

void comm_permit_join_timer_handler(void * arg)
{
	if ((ds_network_status.permit_remaining_time == 0) || (ds_network_status.permit_remaining_time == 255))
	{
		tu_kill_timer(&pj_timer);
	} 

	ui_refresh_display();

	if ((ds_network_status.permit_remaining_time > 0) && (ds_network_status.permit_remaining_time < 255))
	{
		ds_network_status.permit_remaining_time--;
	}
}

void comm_process_permit_join (pkt_buf_t *pkt, void *cbArg) 
{

	NwkZigbeeGenericCnf * msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}

	UI_PRINT_LOG("comm_process_permit_join: Received ZIGBEE_GENERIC_CNF");

	msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len,	pkt->packed_protobuf_packet);   

	if (msg)
	{
		if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("comm_process_permit_join: Status SUCCESS.");

			ds_network_status.permit_remaining_time = requested_join_time;

			UI_PRINT_LOG("comm_process_permit_join: Requested join time %d",
			requested_join_time);

			if ((requested_join_time > 0) && (requested_join_time < 255))
			{
				tu_set_timer(&pj_timer, 1000, true, &comm_permit_join_timer_handler, NULL);
			}
			
			comm_permit_join_timer_handler(NULL);
		}
		else
		{
			UI_PRINT_LOG("comm_process_permit_join: Error: Status FAILURE.");
		}

		nwk_zigbee_generic_cnf__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("comm_process_permit_join: Could not unpack msg");
	}
}

void comm_process_start_commissioning_response(pkt_buf_t * pkt, void * cbArg)
{
	NwkZigbeeGenericCnf *msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}
	
	UI_PRINT_LOG("comm_process_start_commissioning_response: Received NWK_START_COMMISSIONING_CNF");

	msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("comm_start_commissioning_confirm: Status SUCCESS.");
		}

		nwk_zigbee_generic_cnf__free_unpacked(msg, NULL);  
	}
	else
	{
		UI_PRINT_LOG("comm_start_commissioning_confirm: Error Could not unpack msg");
	}

}

void comm_process_start_finding_and_binding_response(pkt_buf_t * pkt, void * cbArg)
{
	NwkZigbeeGenericCnf *msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}
	
	UI_PRINT_LOG("comm_process_start_finding_and_binding_response: Received NWK_START_FINDING_AND_BINDING_CNF");

	msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("comm_start_finding_and_binding_confirm: Status SUCCESS.");
		}

		nwk_zigbee_generic_cnf__free_unpacked(msg, NULL);  
	}
	else
	{
		UI_PRINT_LOG("comm_start_finding_and_binding_confirm: Error Could not unpack msg");
	}

}

void comm_process_add_installcode_response(pkt_buf_t * pkt, void * cbArg)
{
	NwkZigbeeGenericCnf *msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}
	
	UI_PRINT_LOG("comm_process_set_installcode_response: Received NWK_ADD_INSTALLCODE_CNF");

	msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("comm_set_installcode_confirm: Status SUCCESS.");
		}

		nwk_zigbee_generic_cnf__free_unpacked(msg, NULL);  
	}
	else
	{
		UI_PRINT_LOG("comm_set_installcode_confirm: Error Could not unpack msg");
	}

}

void comm_remove_device_confirm (pkt_buf_t *pkt, void *cbArg) 
{

	NwkZigbeeGenericCnf * msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}

	UI_PRINT_LOG("comm_remove_device_confirm: Received ZIGBEE_GENERIC_CNF");

	msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len,	pkt->packed_protobuf_packet);   

	if (msg)
	{
		if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("comm_remove_device_confirm: Status SUCCESS.");
		}

		nwk_zigbee_generic_cnf__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("comm_remove_device_confirm: Error Could not unpack msg.");
	}
}

void comm_device_binding_entry_request_confirm (pkt_buf_t *pkt, void *cbArg)
{

	NwkZigbeeGenericCnf * msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}

	//UI_PRINT_LOG("comm_device_binding_entry_request_confirm: Received ZIGBEE_GENERIC_CNF");

	msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len,	pkt->packed_protobuf_packet);

	if (msg)
	{
		if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("comm_device_binding_entry_request_confirm: Status SUCCESS.");
		}

		nwk_zigbee_generic_cnf__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("comm_device_binding_entry_request_confirm: Error Could not unpack msg.");
	}
}

void comm_device_binding_entry_request_rsp_ind (pkt_buf_t *pkt)
{

	NwkSetBindingEntryRspInd * msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_RSP_IND)
	{
		UI_PRINT_LOG("comm_device_binding_entry_request_rsp_ind wrong command id");
		return;
	}

	UI_PRINT_LOG("comm_device_binding_entry_request_rsp_ind Received NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_RSP_IND");

	msg = nwk_set_binding_entry_rsp_ind__unpack(NULL, pkt->header.len,	pkt->packed_protobuf_packet);

	if (msg)
	{
		if(FBInitiatorTimeShadow > 0)
		{
			
			if(p_fb_bind_list_processing->pSrcAddr != 0)
			{
				if(msg->srcaddr->ieeeaddr == p_fb_bind_list_processing->pSrcAddr->ieee_addr)
				{
					p_fb_bind_list_processing->isComplete = TRUE;
					p_fb_bind_list_processing->bindStatus = msg->status;

					if(msg->status == NWK_ZDP_STATUS__SUCCESS)
					{
						// ADD GROUP
						if ((bdbCommissioningGroupID != 0xFFFF)&&(p_respondants_table_current->isComplete))
						{
							zb_addr_t addGroupDstAddr;
							char groupName[5] = {'G','W','F','n','B'};
							
							addGroupDstAddr.endpoint = p_respondants_table_current->endpoint;
							addGroupDstAddr.ieee_addr = p_respondants_table_current->ieeeAddress;
							addGroupDstAddr.groupaddr = 0;
							gs_add_group(&addGroupDstAddr, bdbCommissioningGroupID, groupName); 
						}

						if (p_fb_bind_list_processing->next != 0)
						{
							p_fb_bind_list_processing = p_fb_bind_list_processing->next;
							dev_process_binding_list_request();
						}
						
					}
					else if(msg->status == NWK_ZDP_STATUS__TABLE_FULL)
					{
						UI_PRINT_LOG(" *****  TABLE FULL!!  *****");
						gw_bindTableFull = TRUE;
					}
					else
					{
						UI_PRINT_LOG("Bind/Unbind not successful (status=%d)", msg->status);
					}

				}
			}
		}
		else
		{
			if (msg->status == NWK_ZDP_STATUS__SUCCESS)
			{
				UI_PRINT_LOG("Bind/Unbind successful");
				UI_PRINT_LOG("sourceaddress 0x%LX endpoint 0x%x", msg->srcaddr->ieeeaddr,msg->srcaddr->endpointid);
			}
			else if(msg->status == NWK_ZDP_STATUS__TABLE_FULL) 
			{
				UI_PRINT_LOG(" *****  TABLE FULL!!  *****");
			}
			else
			{
				UI_PRINT_LOG("Bind/Unbind not successful (status=%d)", msg->status);
			}
		}
		nwk_set_binding_entry_rsp_ind__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("comm_device_binding_entry_request_rsp_ind: Error Could not unpack msg.");
	}
}

void comm_device_binding_table_request_rsp_ind (pkt_buf_t *pkt)
{
	NwkGetBindingTableRspInd * msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__NWK_GET_BINDING_TABLE_RSP_IND)
	{
		UI_PRINT_LOG("comm_device_binding_table_request_rsp_ind wrong command id");
		return;
	}

	UI_PRINT_LOG("comm_device_binding_table_request_rsp_ind Received NWK_MGR_CMD_ID_T__NWK_GET_BINDING_TABLE_RSP_IND");

	msg = nwk_get_binding_table_rsp_ind__unpack(NULL, pkt->header.len,	pkt->packed_protobuf_packet);

	if (msg)
	{
		if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("MGMT BIND RESPONSE successful\n");
			UI_PRINT_LOG("Source Address 0x%LX\n", msg->srcaddr);
			UI_PRINT_LOG("Bind Entries 0x%LX\n", msg->bindentries);
			UI_PRINT_LOG("Start Index 0x%LX\n", msg->startindex);
			UI_PRINT_LOG("BIND LIST\n");
			//User code to display info from device binds
		}
		else
		{
			UI_PRINT_LOG("MGMT BIND RSP not successful (status=%d)", msg->status);
		}

		nwk_get_binding_table_rsp_ind__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("comm_device_binding_table_request_rsp_ind: Error Could not unpack msg.");
	}
}

void comm_device_binding_table_request_confirm (pkt_buf_t *pkt, void *cbArg)
{

	NwkZigbeeGenericCnf * msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}

	UI_PRINT_LOG("comm_device_binding_table_request_confirm: Received ZIGBEE_GENERIC_CNF");

	msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len,	pkt->packed_protobuf_packet);

	if (msg)
	{
		if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("comm_device_binding_table_request_confirm: Status SUCCESS.");
		}

		nwk_zigbee_generic_cnf__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("comm_device_binding_table_request_confirm: Error Could not unpack msg.");
	}
}

void comm_send_permit_join(uint8_t joinTime)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkSetPermitJoinReq msg = NWK_SET_PERMIT_JOIN_REQ__INIT;

	msg.permitjoin = NWK_PERMIT_JOIN_TYPE_T__PERMIT_NETWORK;
	msg.permitjointime = joinTime;
	len = nwk_set_permit_join_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	UI_PRINT_LOG("comm_send_permit_join: Sending NWK_SET_PERMIT_JOIN_REQ with Join Time 0x%x", joinTime);

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id= NWK_MGR_CMD_ID_T__NWK_SET_PERMIT_JOIN_REQ;

		requested_join_time = joinTime;

		nwk_set_permit_join_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt, &comm_process_permit_join, NULL) !=0 )
		{
			UI_PRINT_LOG("comm_send_permit_join: Error: Could not send msg.");
		}

		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("comm_send_permit_join: Error: Could not unpack msg");
	}
}

void comm_start_commissioning_req(uint8_t commMode)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkStartCommissioningReq msg = NWK_START_COMMISSIONING_REQ__INIT;
	
	msg.commmode = commMode;
	len = nwk_start_commissioning_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	UI_PRINT_LOG("comm_start_commissioning_request: Sending NWK_START_COMMISSIONING_REQ 2");

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_START_COMMISSIONING_REQ;

		nwk_start_commissioning_req__pack(&msg, pkt->packed_protobuf_packet);
		
		if (si_send_packet(pkt,	&comm_process_start_commissioning_response, NULL) != 0)
		{
			UI_PRINT_LOG("comm_start_commissioning_request: Error: Could not send msg");
		}
		
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("comm_start_commissioning_request: Error: Could not pack msg");
	}
}

#define SELF(endpointX) {0, (endpointX), 0xFFFFFFFE}
#define ATTR_REC_IDENTIFY(valueX) { PROTOBUF_C_MESSAGE_INIT (&gw_attribute_record_t__descriptor) , 0, 0x21, {2, (uint8_t *)(valueX)} }

void comm_add_installcode_req (InstallCode_t installcode)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkSetInstallCodeReq msg = NWK_SET_INSTALL_CODE_REQ__INIT;
	int x = 0;
	int y = 0;
	uint32_t installCodeValue[16];
	
	for (x=0;x<8;x++){ 
		msg.ieeeaddress = (msg.ieeeaddress * 0x100) + installcode.ieeeAddress[x];
	}

	for (x=0;x<8;x++){ 
		installCodeValue[y] = (uint32_t) ((installcode.installCode[x] & 0xFF00)>>8);
		installCodeValue[y+1] = (uint32_t) (installcode.installCode[x] & 0x00FF);
		y=y+2;
	}
	
	msg.n_installcode = 16;
	msg.installcode = &installCodeValue[0];

	msg.crc = installcode.crc;
	
	len = nwk_set_install_code_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id= NWK_MGR_CMD_ID_T__NWK_SET_INSTALLCODE_REQ;

		nwk_set_install_code_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt, &comm_process_add_installcode_response, NULL) !=0 )
		{
			UI_PRINT_LOG("comm_add_installcode: Error: Could not send msg.");
		}

		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("comm_add_installcode: Error: Could not unpack msg");
	}

}

void comm_remove_device_request(zb_addr_t * addr)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkRemoveDeviceReq msg = NWK_REMOVE_DEVICE_REQ__INIT;
	NwkAddressStructT nwkaddr = NWK_ADDRESS_STRUCT_T__INIT;

	UI_PRINT_LOG("comm_remove_device_request: Sending NWK_REMOVE_DEVICE_REQ with addr 0x%LX endpoint 0x%x", addr->ieee_addr, addr->endpoint); 

	nwkaddr.addresstype = NWK_ADDRESS_TYPE_T__UNICAST; 
	nwkaddr.has_ieeeaddr = true; 
	nwkaddr.ieeeaddr = addr->ieee_addr; 
	nwkaddr.has_endpointid = true;
	nwkaddr.endpointid = addr->endpoint;

	msg.leavemode = NWK_LEAVE_MODE_T__LEAVE;
	msg.dstaddr = &nwkaddr;

	len = nwk_remove_device_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_REMOVE_DEVICE_REQ;

		nwk_remove_device_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt, &comm_remove_device_confirm, NULL) !=0 )
		{
			UI_PRINT_LOG("comm_remove_device_request: Error: Could not send msg.");
		}
		
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("comm_remove_device_request: Error: Could not pack msg");
	}
}

void comm_device_binding_entry_request(zb_addr_t * source_addr, zb_addr_t * dst_addr, uint32_t cluster_id, binding_mode_t binding_mode )
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkSetBindingEntryReq msg = NWK_SET_BINDING_ENTRY_REQ__INIT;
	NwkAddressStructT source_address = NWK_ADDRESS_STRUCT_T__INIT;
	NwkAddressStructT destination_address = NWK_ADDRESS_STRUCT_T__INIT;

	source_address.addresstype = NWK_ADDRESS_TYPE_T__UNICAST;
	source_address.has_ieeeaddr = true;
	source_address.ieeeaddr = source_addr->ieee_addr;
	source_address.has_endpointid = true;
	source_address.endpointid = source_addr->endpoint;

	UI_PRINT_LOG("binding  source_address 0x%LX endpoint 0x%x", source_address.ieeeaddr, source_address.endpointid);

	if ((dst_addr->ieee_addr != 0) && (dst_addr->groupaddr == 0))
	{
		destination_address.addresstype = NWK_ADDRESS_TYPE_T__UNICAST;
		destination_address.has_ieeeaddr = true;
		destination_address.ieeeaddr = dst_addr->ieee_addr;
		destination_address.has_groupaddr = false;
        destination_address.groupaddr = 0;
		destination_address.has_endpointid = true;
		destination_address.endpointid = dst_addr->endpoint;
	}
	else
	{
        destination_address.has_groupaddr = true;
        destination_address.groupaddr = dst_addr->groupaddr;
        destination_address.addresstype = NWK_ADDRESS_TYPE_T__GROUPCAST;
        destination_address.has_ieeeaddr = false;
        destination_address.ieeeaddr = 0;
        destination_address.has_endpointid = false;
        destination_address.endpointid = 0;
	}

	UI_PRINT_LOG("binding  destination_address 0x%LX endpoint 0x%x", destination_address.ieeeaddr, destination_address.endpointid);

	msg.srcaddr = &source_address;
	msg.dstaddr = &destination_address;
	msg.clusterid = cluster_id;
	msg.bindingmode = (binding_mode == BINDING_MODE_BIND) ? NWK_BINDING_MODE_T__BIND : NWK_BINDING_MODE_T__UNBIND;

	len = nwk_set_binding_entry_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_REQ;

		nwk_set_binding_entry_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt, &comm_device_binding_entry_request_confirm, NULL) !=0 )
		{
			UI_PRINT_LOG("comm_device_binding_entry_request: Error: Could not send msg.");
		}

		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("comm_device_binding_entry_request: Error: Could not pack msg");
	}
}

void comm_device_binding_table_request(zb_addr_t * dst_addr, uint8_t startIndex)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkGetBindingTableReq msg = NWK_GET_BINDING_TABLE_REQ__INIT;
	NwkAddressStructT nwkaddr = NWK_ADDRESS_STRUCT_T__INIT;
	
	UI_PRINT_LOG("comm_device_binding_table_request: Sending NWK_DEVICE_BINDING_TABLE_REQ with addr 0x%LX endpoint 0x%x", dst_addr->ieee_addr, dst_addr->endpoint); 

	nwkaddr.addresstype = NWK_ADDRESS_TYPE_T__UNICAST; 
	nwkaddr.has_ieeeaddr = true; 
	nwkaddr.ieeeaddr = dst_addr->ieee_addr; 
	msg.dstaddr = &nwkaddr;
	msg.startindex = startIndex;

	len = nwk_get_binding_table_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_GET_BINDING_TABLE_REQ;

		nwk_get_binding_table_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt, &comm_device_binding_table_request_confirm, NULL) !=0 )
		{
			UI_PRINT_LOG("comm_device_binding_table_request: Error: Could not send msg.");
		}

		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("comm_device_binding_table_request: Error: Could not pack msg");
	}
}

/***************************************************************************************************
 ***************************************************************************************************
 								FINDING AND BINDING FUNCTIONALITY
 ***************************************************************************************************
 ***************************************************************************************************/
uint8_t nwk_zclFindingBindingEpType( uint8_t n_inputClusters, uint16_t *inputClusters, uint8_t n_outputClusters, uint16_t *outputClusters )
{
  uint8_t epType = 0;
  uint8_t status;
  uint8_t type1ClusterCnt;
  uint8_t type2ClusterCnt;
  
  type1ClusterCnt = sizeof( nwk_ZclType1Clusters )/sizeof( uint16_t );
  type2ClusterCnt = sizeof( nwk_ZclType2Clusters )/sizeof( uint16_t );

  // Are there matching type 1 on server side?
  status = ZDO_nwk_AnyClusterMatches( n_inputClusters, 
                                  		inputClusters,
                                  		type1ClusterCnt,
                                  		(uint16_t*)nwk_ZclType1Clusters);
  
  if( status == true )
  {
    epType |= BDB_NWK_FINDING_AND_BINDING_TARGET;// 2
  }
  
  // Are there matching type 1 on client side?
  status = ZDO_nwk_AnyClusterMatches( n_outputClusters, 
                                  		outputClusters,
                                  		type1ClusterCnt,
                                  		(uint16_t*)nwk_ZclType1Clusters);
  
  if( status == true )
  {
    epType |= BDB_NWK_FINDING_AND_BINDING_INITIATOR;
  }
  
  // Are there matching type 2 on server side?
  status = ZDO_nwk_AnyClusterMatches( n_inputClusters, 
                                  		inputClusters,
                                  		type2ClusterCnt,
                                  		(uint16_t*)nwk_ZclType2Clusters);
  
  if( status == true )
  {
    epType |= BDB_NWK_FINDING_AND_BINDING_INITIATOR;// 1
  }
  
  // Are there matching type 2 on client side?
  status = ZDO_nwk_AnyClusterMatches( n_outputClusters, 
                                  		outputClusters,
                                  		type2ClusterCnt,
                                  		(uint16_t*)nwk_ZclType2Clusters);
  
  if( status == true )
  {
    epType |= BDB_NWK_FINDING_AND_BINDING_TARGET;
  }

  return epType;

}

/*********************************************************************
 * @fn          ZDO_nwk_AnyClusterMatches
 *
 * @brief       Compares two lists for matches.
 *
 * @param       ACnt  - number of entries in list A
 * @param       AList  - List A
 * @param       BCnt  - number of entries in list B
 * @param       BList  - List B
 *
 * @return      true if a match is found
 */
uint8_t ZDO_nwk_AnyClusterMatches( uint8_t ACnt, uint16_t *AList, uint8_t BCnt, uint16_t *BList )
{
  uint8_t x, y;

  for ( x = 0; x < ACnt; x++ )
  {
    for ( y = 0; y < BCnt; y++ )
    {
      //UI_PRINT_LOG("CL: %X", BList[y]);
      if ( AList[x] == BList[y] )
      {
        return true;
      }
    }
  }

  return false;
}

void dev_process_gw_endpoint_info_response(pkt_buf_t * pkt, void * cbarg)
{
	NwkGetGwEndpointInfoCnf *msg = NULL;
	gwSimpleDescriptor *pSimpleDesc;
	int clusterListCount = 0;
	uint16_t *pClusterLists;
	int i=0;
	int x=0;
	
	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__NWK_GET_GW_ENDPOINT_INFO_CNF)
	{
		return;
	}
	
	UI_PRINT_LOG("device_process_local_info_response: Received NWK_GET_GW_ENDPOINT_INFO_CNF");

	msg = nwk_get_gw_endpoint_info_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		gw_ieeeAddr = msg->deviceinfolist->ieeeaddress;
		
	  	for ( i = 0; i < msg->deviceinfolist->n_simpledesclist; i++ )
	  	{
	    	clusterListCount += (msg->deviceinfolist->simpledesclist[i]->n_inputclusters +
	                         msg->deviceinfolist->simpledesclist[i]->n_outputclusters);
	  	}
		pClusterLists = malloc( sizeof( uint16_t ) * clusterListCount );
			
		pSimpleDesc = malloc( sizeof( gwSimpleDescriptor ) );
		p_gw_simple_descriptor = pSimpleDesc;

		for ( i=0 ; i<msg->deviceinfolist->n_simpledesclist ; i++ )
		{
			pSimpleDesc->endpointid = msg->deviceinfolist->simpledesclist[i]->endpointid;
			pSimpleDesc->profileid = msg->deviceinfolist->simpledesclist[i]->profileid;
			pSimpleDesc->deviceid = msg->deviceinfolist->simpledesclist[i]->deviceid;
			pSimpleDesc->devicever = msg->deviceinfolist->simpledesclist[i]->devicever;
			pSimpleDesc->n_inputclusters = msg->deviceinfolist->simpledesclist[i]->n_inputclusters;
			pSimpleDesc->n_outputclusters = msg->deviceinfolist->simpledesclist[i]->n_outputclusters;
			
			pSimpleDesc->inputclusters = pClusterLists;

			for( x=0 ; x<pSimpleDesc->n_inputclusters ; x++ )
			{
				pClusterLists[x] = (uint16_t)msg->deviceinfolist->simpledesclist[i]->inputclusters[x];
				//UI_PRINT_LOG("CL: %X", pSimpleDesc->inputclusters[x]);
				//UI_PRINT_LOG("CL: %X", pClusterLists[x]);
			}

		    pClusterLists += pSimpleDesc->n_inputclusters;                           
		    pSimpleDesc->outputclusters = pClusterLists;

			for( x=0 ; x<pSimpleDesc->n_outputclusters ; x++ )
			{
				pClusterLists[x] = (uint16_t)msg->deviceinfolist->simpledesclist[i]->outputclusters[x];
				//UI_PRINT_LOG("CL: %X", pSimpleDesc->inputclusters[x]);
				//UI_PRINT_LOG("CL: %X", pClusterLists[x]);
			}

		    pClusterLists += pSimpleDesc->n_outputclusters;

			if ((msg->deviceinfolist->n_simpledesclist-1) > i)
			{
				pSimpleDesc->next = malloc( sizeof( gwSimpleDescriptor ) );
				pSimpleDesc = pSimpleDesc->next;
			}
			else
			{
				pSimpleDesc->next = malloc( sizeof( gwSimpleDescriptor ) );
				pSimpleDesc->next = NULL;
			}

		}
		
		pSimpleDesc = p_gw_simple_descriptor;
		for ( i=0 ; i<msg->deviceinfolist->n_simpledesclist ; i++ )
		{
			//UI_PRINT_LOG("Endpoint[%d]: %X", i, pSimpleDesc->endpointid);
			pSimpleDesc = pSimpleDesc->next;
		}
		nwk_get_gw_endpoint_info_cnf__free_unpacked(msg, NULL);
		//free(pClusterLists);
	}
	else
	{
		UI_PRINT_LOG("device_process_gw_endpoint_info_response: Error Could not unpack msg");
	}

}

uint8_t comm_start_finding_and_binding_req( uint8_t endpoint )
{
	uint8_t status;
	uint8_t epType;

	UI_PRINT_LOG("Start Finding And Binding");

	gwSimpleDescriptor *pEndpointSD;
	pEndpointSD = p_gw_simple_descriptor;

	//This function is used to force the use of groups for Finding and Binding
	//dev_setBdbCommissioningGroupID(0xCAFE); 

	while(pEndpointSD != NULL)
	{
	  if(pEndpointSD->endpointid == endpoint )
	  {
	  	gw_fb_endpoint = endpoint;
		break;
	  }
	  pEndpointSD = pEndpointSD->next;
	}
	
	if((pEndpointSD != NULL) && (gw_fb_endpoint != 0))
	{
		epType = nwk_zclFindingBindingEpType(pEndpointSD->n_inputclusters, pEndpointSD->inputclusters, pEndpointSD->n_outputclusters, pEndpointSD->outputclusters);
		if (epType & BDB_NWK_FINDING_AND_BINDING_TARGET)
		{
			UI_PRINT_LOG("Target");
			status = TRUE;
			attr_update_identify_time_attribute(gw_fb_endpoint, 180);
		}
		if ((epType & BDB_NWK_FINDING_AND_BINDING_INITIATOR)&&(!(comm_status & FINDING_AND_BINDING_IN_PROGRESS)))
		{
			UI_PRINT_LOG("Initiator");
			FBInitiatorTimeShadow = FINDING_AND_BINDING_DURATION;
			comm_status = FINDING_AND_BINDING_IN_PROGRESS;
			status = TRUE;
			p_gw_respondants_table_head = malloc( sizeof( FBRespondantsTable ) );
			dev_clear_respondant_table_entry(p_gw_respondants_table_head);
			p_respondants_table_current = p_gw_respondants_table_head;

			p_fb_bind_list_head = malloc( sizeof( gwBindQueue ) );
			dev_clear_bind_list_entry(p_fb_bind_list_head);
			p_fb_bind_list_current = p_fb_bind_list_head;
			p_fb_bind_list_processing = p_fb_bind_list_head;
			tu_set_timer(&finding_and_binding_start, 1000, TRUE, dev_finding_and_binding_start_handler, NULL);
		}
		if(!(epType & BDB_NWK_FINDING_AND_BINDING_TARGET) && (epType & BDB_NWK_FINDING_AND_BINDING_INITIATOR))
		{
			UI_PRINT_LOG("Out of scope");
			status = FALSE;
		}
	}
	else
	{
		UI_PRINT_LOG("Endpoint: %X is not Finding and Binding compatible", endpoint);
		status = FALSE;
	}

	return status;
}

void dev_get_gw_endpoint_info_request(void)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkGetGwEndpointInfoReq msg = NWK_GET_GW_ENDPOINT_INFO_REQ__INIT;

	UI_PRINT_LOG("device_get_gw_endpoint_info_request: Sending NWK_GET_GW_ENDPOINT_INFO_REQ");
	
	len = nwk_get_gw_endpoint_info_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_GET_GW_ENDPOINT_INFO_REQ;

		nwk_get_gw_endpoint_info_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt,	(confirmation_processing_cb_t)&dev_process_gw_endpoint_info_response, NULL) != 0)
		{
			UI_PRINT_LOG("device_get_gw_endpoint_info_request: Error: Could not send msg");
		}
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("device_get_gw_endpoint_info_request: Error: Could not pack msg");
	}
}

void dev_send_identify_query_response(pkt_buf_t * pkt, void * cbarg)
{
	GwZigbeeGenericCnf *msg = NULL;

	if (pkt->header.cmd_id != GW_CMD_ID_T__ZIGBEE_GENERIC_CNF) 
	{
		UI_PRINT_LOG("(pkt->header.cmd_id != GW_CMD_ID_T__ZIGBEE_GENERIC_CNF)");
		return;
	}

	UI_PRINT_LOG("dev_send_identify_query_response: Received ZIGBEE_GENERIC_CNF");


	msg = gw_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg) 
	{
		//Add code
		gw_zigbee_generic_cnf__free_unpacked(msg, NULL);	
	}
	else
	{
		UI_PRINT_LOG("dev_send_identify_query_response: Error Could not unpack msg.");
	}

}

void dev_set_finding_and_binding_response(pkt_buf_t * pkt, void * cbarg)
{
	GwZigbeeGenericCnf *msg = NULL;

	if (pkt->header.cmd_id != GW_CMD_ID_T__ZIGBEE_GENERIC_CNF) 
	{
		UI_PRINT_LOG("(pkt->header.cmd_id != GW_CMD_ID_T__ZIGBEE_GENERIC_CNF)");
		return;
	}

	UI_PRINT_LOG("dev_set_finding_and_binding_response: Received ZIGBEE_GENERIC_CNF");


	msg = gw_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg) 
	{
		//Add code
		gw_zigbee_generic_cnf__free_unpacked(msg, NULL);	
	}
	else
	{
		UI_PRINT_LOG("dev_set_finding_and_binding_response: Error Could not unpack msg.");
	}

}

void dev_finding_and_binding_initiator_state_machine(uint8_t endpoint)
{
	//UI_PRINT_LOG("************** STATE MACHINE **************");
	//int x = 0;
	FBRespondantsTable *p_respondants_table = p_respondants_table_current; //Reference to the first incomplete entry of respondants table

	if (p_respondants_table->nwkAddress > 0) //Field is set
	{
	    UI_PRINT_LOG("Entry has NwkAddress: 0x%X", p_respondants_table->nwkAddress);
		if ((p_respondants_table->hasSimpleDesc)||(p_respondants_table->attempts >= FINDING_AND_BINDING_MAX_RETRIES )) //Field is set or attempts are maxed out
		{
			UI_PRINT_LOG("Entry has SimpleDescriptor");
			if ((p_respondants_table->hasIeeeAddress)||(p_respondants_table->attempts >= FINDING_AND_BINDING_MAX_RETRIES )) //Field is set or attempts are maxed out
			{
				UI_PRINT_LOG("Entry has IEEEAddress: 0x%llX", p_respondants_table->ieeeAddress);
				if ((p_respondants_table->isComplete)||(p_respondants_table->attempts >= FINDING_AND_BINDING_MAX_RETRIES )) //Field is set or attempts are maxed out //Field is set
				{
					if (p_respondants_table->next == 0) //If next slot is not allocated yet DO IT
					{
						p_respondants_table->next = malloc( sizeof( FBRespondantsTable ) );
						dev_clear_respondant_table_entry(p_respondants_table->next);
					}
					//Whether it was or not allocated doesn't matter just point to the next entry
					p_respondants_table_current = p_respondants_table->next;
					dev_finding_and_binding_initiator_state_machine(endpoint);
				}
			}
			else
			{
				//Get IEEE Address
				UI_PRINT_LOG("Sending IEEEAddressReq");
				//p_respondants_table->attempts++;
				//Not really needed since the first time the node is added to the table the IEEE Address is already available
			}
		}
		else //Probe for IEEE Address of nwkAddress 
		{
			//Get simpleDescriptor
			//UI_PRINT_LOG("Sending SimpleDescriptorReq");
			p_respondants_table->attempts++; 
			dev_send_simple_descriptor_req( p_respondants_table->nwkAddress, p_respondants_table->endpoint, endpoint);
		}
	}
	else //Probe for more entries
	{
		// Send Identify Query Req
		//UI_PRINT_LOG("Identify Query Request");
		dev_send_identify_query( endpoint );
	}

	//debug_print_table();
	tu_set_timer(&finding_and_binding_timer, (FINDING_AND_BINDING_PERIOD*1000), TRUE, dev_finding_and_binding_timer_handler, NULL);
}

//Function to set all values to Zero
void dev_clear_respondant_table_entry(FBRespondantsTable *p_respondants_table_entry)
{
	p_respondants_table_entry->attempts = 0;
	p_respondants_table_entry->nwkAddress = 0;
	p_respondants_table_entry->endpoint = 0;
	p_respondants_table_entry->hasSimpleDesc = 0;
	p_respondants_table_entry->hasIeeeAddress = 0;
	p_respondants_table_entry->ieeeAddress = 0;
	p_respondants_table_entry->isComplete = 0;
	p_respondants_table_entry->next = 0;
	p_respondants_table_entry->simpleDesc = 0;
}

//Function to release memory from one entry
void dev_delete_respondant_table_entry(FBRespondantsTable *p_respondants_table_entry)
{
	if(p_respondants_table_entry->simpleDesc != 0)
	{
		free(p_respondants_table_entry->simpleDesc);
	}
	free(p_respondants_table_entry);
}

//Function to release memory from whole table
void dev_delete_respondant_table(void)
{	
	FBRespondantsTable *p_respondants_table_entryA = p_gw_respondants_table_head;
	FBRespondantsTable *p_respondants_table_entryB;
	
	while (p_respondants_table_entryA != 0)
	{
		p_respondants_table_entryB = p_respondants_table_entryA->next;
		dev_delete_respondant_table_entry(p_respondants_table_entryA);
		p_respondants_table_entryA = p_respondants_table_entryB;
	}
}

//Just a little debug function to help monitor the state on the table while the state machine runs
void debug_print_table(void)
{
	FBRespondantsTable *p_respondants_table;
	p_respondants_table = p_gw_respondants_table_head;

	UI_PRINT_LOG("Debug Print Table");
	while(p_respondants_table != 0)
	{
		UI_PRINT_LOG("attempts:   %d", p_respondants_table->attempts);
		UI_PRINT_LOG("nwkAddress: 0x%X", p_respondants_table->nwkAddress);
		UI_PRINT_LOG("endpoint:   0x%X", p_respondants_table->endpoint);
		UI_PRINT_LOG("hasSimpleD: %d", p_respondants_table->hasSimpleDesc);
		UI_PRINT_LOG("hasIEEEAdd: %d", p_respondants_table->hasIeeeAddress);
		UI_PRINT_LOG("hasIEEEAdd: 0x%llX", p_respondants_table->ieeeAddress);
		UI_PRINT_LOG("isComplete: %d", p_respondants_table->isComplete);
		UI_PRINT_LOG("next Addr:  %X", p_respondants_table->next);
		p_respondants_table = p_respondants_table->next;
	}
}

//This function helps the response handler for identify query rsp to check if the device already exists in the table
uint8_t dev_lookup_duplicates_respondants_table(uint16_t newNwkAddr, uint8_t newEndpoint)
{
    FBRespondantsTable *p_respondants_table_lookup;
	//Check if this combination of parameters is already used 
	p_respondants_table_lookup = p_gw_respondants_table_head;
	uint8_t status = 0; //Not in table
	
	while (p_respondants_table_lookup->nwkAddress > 0 )
	{
		if ((p_respondants_table_lookup->nwkAddress == newNwkAddr) && (p_respondants_table_lookup->endpoint == newEndpoint)) //New entry exists already
		{
			// Drop new entry because it exists in the table and get out
			status = 1; // Already on table
			break;
		}

		if (p_respondants_table_lookup->next != 0) //If entry is allocated
		{
			p_respondants_table_lookup = p_respondants_table_lookup->next;
		}
		else
		{
			break;
		}

	}
	
	return status;

}

//Timer to delay a little the launch of state machine after setting the forwarding flag at the gatewaysrvr
void dev_finding_and_binding_start_handler(void * arg)
{
	// Kill this first timer
	tu_kill_timer(&finding_and_binding_start);
	
	// Set finding and binding response forwarding flag
	dev_set_finding_and_binding_timer_status_req(FINDING_AND_BINDING_TIMER_ACTIVE);
	
	// Set 1sec delay to start sending messages for finding and binding initiator
	tu_set_timer(&finding_and_binding_timer, 1000, TRUE, dev_finding_and_binding_timer_handler, NULL);
}

//Timer handler for periodic state machine operation
void dev_finding_and_binding_timer_handler(void * arg)
{
	if((FBInitiatorTimeShadow <= 0)||(gw_bindTableFull)) //If duration expires or binding table full
	{
		//Stop response forwarding
		dev_set_finding_and_binding_timer_status_req(FINDING_AND_BINDING_TIMER_INACTIVE);
		
		//Set status flag to ready for new round of FINDING AND BINDING
		comm_status = FINDING_AND_BINDING_READY;
		
		//Kill all entries inside the table. FREE SPACE
		dev_delete_respondant_table();

		//Kill timer
		tu_kill_timer(&finding_and_binding_timer);
		
		//Bind section
		//Kill all entries on bind list
		dev_delete_binding_list();

		//Reset flag
		gw_bindTableFull = FALSE;

		//Kill bind timer
		//tu_kill_timer(&finding_and_binding_bind_delay);
		UI_PRINT_LOG("\n\n ***** END OF FINDING AND BINDING INITIATOR ***** \n\n");
	}
	else
	{
		//Take off the TimerShadow
		FBInitiatorTimeShadow = FBInitiatorTimeShadow - FINDING_AND_BINDING_PERIOD;
		//Send value to GW
		dev_finding_and_binding_initiator_state_machine(gw_fb_endpoint);
	}
}

//Send Identify Query command
void dev_send_identify_query( uint8_t endpoint )
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	DevSendIdentifyQueryReq msg = DEV_SEND_IDENTIFY_QUERY_REQ__INIT;

	msg.endpoint = endpoint;
	len = dev_send_identify_query_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);
	
	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW;
		pkt->header.cmd_id = GW_CMD_ID_T__DEV_SEND_IDENTIFY_QUERY_REQ;

		dev_send_identify_query_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt,	(confirmation_processing_cb_t)&dev_send_identify_query_response, NULL) != 0)
		{
			UI_PRINT_LOG("comm_send_identify_query_request: Error: Could not send msg");
		}
		
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("comm_send_identify_query_request: Error: Could not pack msg");
	}
	
}

//Generic Response
void dev_send_simple_descriptor_response(pkt_buf_t * pkt, void * cbArg)
{
	NwkZigbeeGenericCnf *msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}
	
	UI_PRINT_LOG("dev_send_simple_descriptor_response: Received NWK_SEND_SIMPLE_DESCRIPTOR_RSP");

	msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		UI_PRINT_LOG("Simple Descriptor: SUCCESS.");

		nwk_zigbee_generic_cnf__free_unpacked(msg, NULL);  
	}
	else
	{
		UI_PRINT_LOG("dev_send_simple_descriptor_response: Error Could not unpack msg");
	}
}

//Send Simple Descriptor Req command
void dev_send_simple_descriptor_req( uint16_t nwkAddr, uint8_t endpoint, uint8_t srcEndpoint )
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkSendSimpleDescriptorReq msg = NWK_SEND_SIMPLE_DESCRIPTOR_REQ__INIT;
	
	msg.nwkaddr = nwkAddr;
	msg.endpoint = endpoint;
	msg.srcendpoint = srcEndpoint;

	len = nwk_send_simple_descriptor_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	UI_PRINT_LOG("dev_send_simple_desc_req: Sending NWK_SEND_SIMPLE_DESCRIPTOR_REQ");

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_SEND_SIMPLE_DESCRIPTOR_REQ;

		nwk_send_simple_descriptor_req__pack(&msg, pkt->packed_protobuf_packet);
		
		if (si_send_packet(pkt,	&dev_send_simple_descriptor_response, NULL) != 0)
		{
			UI_PRINT_LOG("dev_send_simple_desc_req: Error: Could not send msg");
		}
		
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("dev_send_simple_desc_req: Error: Could not pack msg");
	}
	
}

//Generic response
void dev_send_ieee_address_response(pkt_buf_t * pkt, void * cbArg)
{
	NwkZigbeeGenericCnf *msg = NULL;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}
	
	UI_PRINT_LOG("dev_send_ieee_address_response: Received NWK_SEND_IEEE_ADDRESS_RSP");

	msg = nwk_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		UI_PRINT_LOG("IEEE Address Req: SUCCESS.");

		nwk_zigbee_generic_cnf__free_unpacked(msg, NULL);  
	}
	else
	{
		UI_PRINT_LOG("dev_send_ieee_address_response: Error Could not unpack msg");
	}
}

//Function to set a boolean flag that allows the forwarding of the identify query response to create table entries
//status can either be active = 1 or inactive = 0
void dev_set_finding_and_binding_timer_status_req( uint8_t status )
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	DevSetFindingAndBindingTimer msg = DEV_SET_FINDING_AND_BINDING_TIMER__INIT;

	msg.status = status;
	len = dev_set_finding_and_binding_timer__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);
	
	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW;
		pkt->header.cmd_id = GW_CMD_ID_T__DEV_SET_FINDING_AND_BINDING_TIMER_STATUS_REQ;

		dev_set_finding_and_binding_timer__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt,	(confirmation_processing_cb_t)&dev_set_finding_and_binding_response, NULL) != 0)
		{
			UI_PRINT_LOG("dev_set_finding_and_binding_timer_status_req: Error: Could not send msg");
		}
		
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("dev_set_finding_and_binding_timer_status_req: Error: Could not pack msg");
	}
	
}

//This function will catch the information of the identify query response
void dev_identify_query_response_handler(pkt_buf_t * pkt)
{
	DevProcessIdentifyQueryRspInd * msg = NULL;
	FBRespondantsTable *p_respondants_table_buffer;

	if(pkt->header.cmd_id != GW_CMD_ID_T__DEV_PROCESS_IDENTIFY_QUERY_RSP_IND)
	{
		return;
	}

	UI_PRINT_LOG("dev_process_identify_query_response_handler: Received GW_CMD_ID_T__DEV_PROCESS_IDENTIFY_QUERY_RSP_IND");

	msg = dev_process_identify_query_rsp_ind__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);	

	if(msg)
	{
		//Check for existing entry
		if ( !(dev_lookup_duplicates_respondants_table(msg->nwkaddr, msg->endpoint)) )//If it's not in the table already
		{
			//UI_PRINT_LOG("New Device!!");	
			//This is the beginning for every entry of the respondant table
			if(p_respondants_table_current->nwkAddress == 0) //If the entry expected nwkaddress + endpoint
			{
				//UI_PRINT_LOG("Current entry is waiting for this!!");
				p_respondants_table_current->nwkAddress = msg->nwkaddr;
				p_respondants_table_current->endpoint = msg->endpoint;

				p_respondants_table_buffer = p_respondants_table_current;
			}
			else //if current entry didn't expect the nwkAddr + EP check the entries
			{
				//UI_PRINT_LOG("Current entry is NOT waiting for this!!");
				//UI_PRINT_LOG("Make new entry");
				p_respondants_table_buffer = p_respondants_table_current;

				while(p_respondants_table_buffer->next != 0)//search for the next available slot
				{
					p_respondants_table_buffer = p_respondants_table_buffer->next;
				}
				
				p_respondants_table_buffer->next = malloc( sizeof( FBRespondantsTable ) );
				p_respondants_table_buffer = p_respondants_table_buffer->next;

				dev_clear_respondant_table_entry(p_respondants_table_buffer);
					
				p_respondants_table_buffer->nwkAddress = msg->nwkaddr;
				p_respondants_table_buffer->endpoint = msg->endpoint;
				
			}

			if(msg->ieeeaddr != 0) //if the gw was able to get an IEEE addr
			{	
				p_respondants_table_buffer->hasIeeeAddress = 1;
				p_respondants_table_buffer->ieeeAddress = msg->ieeeaddr;
			}

		}
		//else
		//{
			//UI_PRINT_LOG("Device is already in table");	
		//}

		dev_process_identify_query_rsp_ind__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("dev_process_identify_query_response_handler: Error Could not unpack msg.");
    }
}

void dev_simple_descriptor_response_handler(pkt_buf_t * pkt)
{  
  NwkSendSimpleDescriptorRspInd * msg = NULL;
  gwSimpleDescriptor *pSimpleDesc;
  int clusterListCount = 0;
  uint16_t *pClusterLists;
  int x=0;

  if ( FBInitiatorTimeShadow != 0 )
  {
    if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__NWK_SEND_SIMPLE_DESCRIPTOR_RSP_IND)
    {
    	UI_PRINT_LOG("dev_simple_descriptor_response_handler wrong command id");
    	return;
    }
    
    UI_PRINT_LOG("dev_simple_descriptor_response_handler Received NWK_MGR_CMD_ID_T__NWK_SEND_SIMPLE_DESCRIPTOR_RSP_IND");
    
    msg = nwk_send_simple_descriptor_rsp_ind__unpack(NULL, pkt->header.len,	pkt->packed_protobuf_packet);
    
    if (msg)
    {
    	p_respondants_table_current->attempts = 0;
		p_respondants_table_current->hasSimpleDesc = 1;

    	clusterListCount += (msg->simpledesc->n_inputclusters +
	                         msg->simpledesc->n_outputclusters);

		pClusterLists = malloc( sizeof( uint16_t ) * clusterListCount );
			
		pSimpleDesc = malloc( sizeof( gwSimpleDescriptor ) );
		p_respondants_table_current->simpleDesc = pSimpleDesc;

			pSimpleDesc->endpointid = msg->simpledesc->endpointid;
			pSimpleDesc->profileid = msg->simpledesc->profileid;
			pSimpleDesc->deviceid = msg->simpledesc->deviceid;
			pSimpleDesc->devicever = msg->simpledesc->devicever;
			pSimpleDesc->n_inputclusters = msg->simpledesc->n_inputclusters;
			pSimpleDesc->n_outputclusters = msg->simpledesc->n_outputclusters;
			
			pSimpleDesc->inputclusters = pClusterLists;

			for( x=0 ; x<pSimpleDesc->n_inputclusters ; x++ )
			{
				pClusterLists[x] = (uint16_t)msg->simpledesc->inputclusters[x];
				//UI_PRINT_LOG("CL: %X", pSimpleDesc->inputclusters[x]);
				//UI_PRINT_LOG("CL: %X", pClusterLists[x]);
			}

		    pClusterLists += pSimpleDesc->n_inputclusters;                           
		    pSimpleDesc->outputclusters = pClusterLists;

			for( x=0 ; x<pSimpleDesc->n_outputclusters ; x++ )
			{
				pClusterLists[x] = (uint16_t)msg->simpledesc->outputclusters[x];
				//UI_PRINT_LOG("CL: %X", pSimpleDesc->inputclusters[x]);
				//UI_PRINT_LOG("CL: %X", pClusterLists[x]);
			}

		    pClusterLists += pSimpleDesc->n_outputclusters;
	
    	nwk_send_simple_descriptor_rsp_ind__free_unpacked(msg, NULL);

		if (p_respondants_table_current->hasIeeeAddress)
		{
			dev_checkMatchingEndpoints();
		}
		else
		{
			UI_PRINT_LOG("\n\nMissing IEEE Address\n\n");
		}
    }
    else
    {
    	UI_PRINT_LOG("dev_simple_descriptor_response_handler: Error Could not unpack msg.");
    }
  }
}

//Add matching clusters to a bind queue to be processed after some time has passed.
void dev_binding_entry_request(zb_addr_t *srcAddr, zb_addr_t *dstAddr, uint16_t bindClusterId)
{
	gwBindQueue *pBindList;
	pBindList = p_fb_bind_list_current;
	//UI_PRINT_LOG("Allocate SrcAddr struct");
	pBindList->pSrcAddr = malloc( sizeof( zb_addr_t ) );
	//UI_PRINT_LOG("Fill SrcAddr struct");
	pBindList->pSrcAddr->endpoint = srcAddr->endpoint;
	pBindList->pSrcAddr->ieee_addr = srcAddr->ieee_addr;
	pBindList->pSrcAddr->groupaddr = srcAddr->groupaddr;
	//UI_PRINT_LOG("Allocate DstAddr struct");
	pBindList->pDstAddr = malloc( sizeof( zb_addr_t ) );
 	//UI_PRINT_LOG("Fill DstAddr struct");
	pBindList->pDstAddr->endpoint = dstAddr->endpoint;
	pBindList->pDstAddr->ieee_addr = dstAddr->ieee_addr;
	pBindList->pDstAddr->groupaddr = dstAddr->groupaddr;

	pBindList->clusterId = bindClusterId;
	//UI_PRINT_LOG("Allocate Next struct bind entry");
	pBindList->next = malloc( sizeof( gwBindQueue ) );
	p_fb_bind_list_current = pBindList->next;
	dev_clear_bind_list_entry(p_fb_bind_list_current);

}

//Function to release memory from one entry
void dev_delete_binding_list_entry(gwBindQueue *p_bind_list_entry)
{
	if(p_bind_list_entry->pSrcAddr != 0) 
	{
		free(p_bind_list_entry->pSrcAddr);
	}
	if(p_bind_list_entry->pDstAddr != 0) 
	{
		free(p_bind_list_entry->pDstAddr);
	}
	free(p_bind_list_entry);
}

//Function to release memory from whole table
void dev_delete_binding_list(void)
{	
	gwBindQueue *p_bind_list_entryA = p_fb_bind_list_head;
	gwBindQueue *p_bind_list_entryB;
	
	while (p_bind_list_entryA != 0)
	{
		p_bind_list_entryB = p_bind_list_entryA->next;
		dev_delete_binding_list_entry(p_bind_list_entryA);
		p_bind_list_entryA = p_bind_list_entryB;
	}
}

//Function to set all values to Zero
void dev_clear_bind_list_entry(gwBindQueue *p_bind_list_entry)
{
	p_bind_list_entry->pSrcAddr = 0;
	p_bind_list_entry->pDstAddr = 0;
	
	p_bind_list_entry->clusterId = 0;
	p_bind_list_entry->bindStatus = 0;
	p_bind_list_entry->isComplete = 0;
	
	p_bind_list_entry->next = 0;
}

//Process the contents of binding list with a slight delay between requests (internal operation)
void dev_process_binding_list_request(void)
{
	gwBindQueue *pBindList;

	pBindList = p_fb_bind_list_processing;

	//Continue unless the bind table is Full!
	if (!gw_bindTableFull)
	{
		//Process the current entry if it is filled (has ieeeaddr) and it is not complete
		if ((pBindList->pSrcAddr != 0) && (pBindList->pDstAddr != 0))
		{	
			if (!pBindList->isComplete)
			{
				comm_device_binding_entry_request(pBindList->pSrcAddr, pBindList->pDstAddr, (uint32_t)pBindList->clusterId, BINDING_MODE_BIND);
			}
		
		}
		else //Empty slot end of list
		{
			return;
		}
	}
}

uint8_t dev_FindIfAppCluster( uint16_t ClusterId )
{
  uint8_t i;
  uint8_t ClusterCnt;

  ClusterCnt = sizeof( nwk_ZclType1Clusters )/sizeof( uint16_t );
  
  for ( i = 0; i < ClusterCnt; i++ )
  {
    if ( nwk_ZclType1Clusters[i] == ClusterId )
    {
      return ( TRUE );
    }
  }
  
  ClusterCnt = sizeof( nwk_ZclType2Clusters )/sizeof( uint16_t );
  
  for ( i = 0; i < ClusterCnt; i++ )
  {
    if ( nwk_ZclType2Clusters[i] == ClusterId )
    {
      return ( TRUE );
    }
  }
  
  // If not found, take it as application cluster it will be filtered
  // by simple descriptor at some point
  return FALSE;
}

void dev_setBdbCommissioningGroupID(uint16_t groupID)
{
	bdbCommissioningGroupID = groupID;
}

uint8_t dev_zclFindingBindingAddBindEntry( uint16_t bindClusterId, uint8_t n_clusterList, uint16_t *clusterList,
							                                  uint64_t dstIeeeAddr, uint8_t dstEndpoint )
{
  uint8_t status;
  zb_addr_t srcAddr;
  zb_addr_t dstAddr;
  
  if ( !dev_FindIfAppCluster ( bindClusterId ))
  {
    return FALSE; // No App cluster
  }

  // Are there matching clusters?
  status = ZDO_nwk_AnyClusterMatches( n_clusterList, 
	                                  clusterList,
	                                  1,
	                                  &bindClusterId);
  if ( status == FALSE )
  {
    return FALSE; // No matched Cluster
  }

  // Add the entry into the binding table
  /*if (!pbindAddEntry( SrcEndpInt, DstAddr, DstEndpInt,
                         1, &BindClusterId ) )
  {
    return ( ZApsTableFull );
  }*/

  if ( status == TRUE )
  {
    //Fill srcAddr structure
    srcAddr.endpoint = gw_fb_endpoint;
    srcAddr.ieee_addr = gw_ieeeAddr;
    srcAddr.groupaddr = 0x0000;
 
    //Fill dstAddr structure
	if ( bdbCommissioningGroupID != 0xFFFF )	  
	{
      	dstAddr.ieee_addr = 0;
		dstAddr.endpoint = 0;
	  	dstAddr.groupaddr = bdbCommissioningGroupID;
	}
	else
	{	
    	dstAddr.ieee_addr = dstIeeeAddr;
		dstAddr.endpoint = dstEndpoint;
	    dstAddr.groupaddr = 0;
	}

	UI_PRINT_LOG("Dst Addr: 0x%llX", dstAddr.ieee_addr);
	UI_PRINT_LOG("Endpoint: 0x%X", dstAddr.endpoint);
	UI_PRINT_LOG("Grp Addr: 0x%X", dstAddr.groupaddr);
    //comm_device_binding_entry_request(&srcAddr, &dstAddr, bindClusterId, BINDING_MODE_BIND);
    UI_PRINT_LOG("Add cluster: 0x%X", bindClusterId);
    dev_binding_entry_request(&srcAddr, &dstAddr, bindClusterId);
  }
  return TRUE;
}

uint8_t debug_calculate_entries(void)
{
	gwBindQueue *pBindList;
	pBindList = p_fb_bind_list_head;
	int count = 0;

	while (pBindList != 0)
	{
		if (pBindList->pSrcAddr != 0)
		{
			count++;
		}
		pBindList = pBindList->next;
	}
	
	return count;
}

void dev_checkMatchingEndpoints(void)
{
	int i;
	uint8_t status;
	uint8_t matchFound;

	gwSimpleDescriptor *pEndpointSD;
	pEndpointSD = p_gw_simple_descriptor;

	while(pEndpointSD != NULL)
	{
	  if(pEndpointSD->endpointid == gw_fb_endpoint )
	  {
		break;
	  }
	  pEndpointSD = pEndpointSD->next;
	}

    for(i = 0; i < pEndpointSD->n_outputclusters; i++)
    {
      //Filter for Application clusters (to bind app clusters only)
      status = dev_zclFindingBindingAddBindEntry(pEndpointSD->outputclusters[i],
                              p_respondants_table_current->simpleDesc->n_inputclusters,
                              p_respondants_table_current->simpleDesc->inputclusters,
                              p_respondants_table_current->ieeeAddress, 
                              p_respondants_table_current->endpoint );
	  if(status == TRUE)
      {
        matchFound = TRUE;
	  }
	  
    }
	
    //Only search for other matches if the table is not full and we have not 
    //found any match or we have to add bind as many as we can 
    if(matchFound == FALSE)
    {
      for(i = 0; i < pEndpointSD->n_inputclusters; i++)
      {
        //Filter for Application clusters (to bind app clusters only)
        status = dev_zclFindingBindingAddBindEntry(pEndpointSD->inputclusters[i],
                              p_respondants_table_current->simpleDesc->n_outputclusters,
                              p_respondants_table_current->simpleDesc->outputclusters,
                              p_respondants_table_current->ieeeAddress, 
                              p_respondants_table_current->endpoint );
        if(status == TRUE)
        {
          matchFound = TRUE;
  	    }

      }
    }

    //Check if we have found any match
    if( matchFound == TRUE )
    {
      //entries = debug_calculate_entries();
      //UI_PRINT_LOG("\n\nNumber of entries: %d\n\n", entries);
      dev_process_binding_list_request();
	  //UI_PRINT_LOG("Filled addresses buffers");
      p_respondants_table_current->attempts = 0;
      p_respondants_table_current->isComplete = TRUE;
    }
    else
    {
      //No matching cluster, then we are done with this respondent
      p_respondants_table_current->attempts = 3;
      p_respondants_table_current->isComplete = FALSE;
    }
}


