/*******************************************************************************
 Filename:       system_engine.c
 Revised:        $Date$
 Revision:       $Revision$

 Description:     Handle misc system related APIs


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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "data_structures.h"
#include "socket_interface.h"
#include "device_list_engine.h"
#include "nwkmgr.pb-c.h"
#include "gateway.pb-c.h"
#include "user_interface.h" 
#include "macros.h"
#include "attribute_engine.h"
#include "system_engine.h"
#include "timer_utils.h"

/******************************************************************************
 * Consts
 *****************************************************************************/
#define RESET_CONFIRMATION_TIMEOUT 60000

#define CLUSTER_ID_MFC_SMART_THINGS_HUMIDITY_MEASUREMENT 0xFC45

/*******************************************************************************
 * Functions
 ******************************************************************************/

void system_reset_request_confirm(pkt_buf_t * pkt, void * arg)
{
 	NwkZigbeeSystemResetCnf * msg = NULL;
	bool maintain_network_information = (bool)arg;

	if (pkt->header.cmd_id != NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_RESET_CNF)
	{
		return;
	}

	UI_PRINT_LOG("system_reset_request_confirm: Received NWK_ZIGBEE_SYSTEM_RESET_CNF");

	msg = nwk_zigbee_system_reset_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);   

	if (msg)
	{
		if (msg->status == NWK_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("system_reset_request_confirm: Status SUCCESS.");
			if (!maintain_network_information)
			{
				macro_clear_all();
			}
		} 
		else
		{
			UI_PRINT_LOG("system_reset_request_confirm: Status FAILURE (%d)", msg->status);
		}
		
		nwk_zigbee_system_reset_cnf__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("system_reset_request_confirm: Could not unpack msg");
	}
} 

void system_send_reset_request(bool maintain_network_information)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkZigbeeSystemResetReq msg = NWK_ZIGBEE_SYSTEM_RESET_REQ__INIT;

	if (maintain_network_information)
	{
		msg.mode = NWK_RESET_MODE_T__SOFT_RESET;
	}
	else
	{
		msg.mode = NWK_RESET_MODE_T__HARD_RESET;
	}

	len = nwk_zigbee_system_reset_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_RESET_REQ;

		nwk_zigbee_system_reset_req__pack(&msg, pkt->packed_protobuf_packet);

		si_set_confirmation_timeout_for_next_request(SI_SERVER_ID_NWK_MGR, RESET_CONFIRMATION_TIMEOUT);

		if (si_send_packet(pkt, &system_reset_request_confirm ,(void *)maintain_network_information) !=0)
		{
			UI_PRINT_LOG("system_send_reset_request: Error: Could not send msg.");
		}
		
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("system_send_reset_request: Error: Could not pack msg.");
	}
}

void system_send_selfshutdown_request()
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	NwkZigbeeSystemSelfShutdownReq msg = NWK_ZIGBEE_SYSTEM_SELF_SHUTDOWN_REQ__INIT;


	len = nwk_zigbee_system_self_shutdown_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR;
		pkt->header.cmd_id = NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_SELF_SHUTDOWN_REQ;

		nwk_zigbee_system_self_shutdown_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt, NULL,NULL) !=0)
		{
			UI_PRINT_LOG("system_send_selfshutdown_reques: Error: Could not send msg.");
		}

		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("system_send_selfshutdown_reques: Error: Could not pack msg.");
	}
}

void system_send_default_response(GwAddressStructT *dstaddress, uint32_t sequencenumber, uint8_t endpointidsource, uint8_t commandId, uint8_t statusCode)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	GwSendZclFrameReq msg = GW_SEND_ZCL_FRAME_REQ__INIT;
	uint8_t payloadData[2];

	UI_PRINT_LOG("system_send_default_response()");

	msg.clientserverdirection = 1;
	msg.clusterid = 0x0003;
	msg.commandid = 0xB;
	msg.disabledefaultrsp = 1;
	msg.dstaddress = dstaddress;
	msg.endpointidsource = endpointidsource;
	msg.frametype = 0;
	msg.has_manufacturercode = 0;
	msg.has_sequencenumber = 1;
	msg.manufacturerspecificflag = 0;
	msg.profileid = 0x0104;
	msg.qualityofservice = 0; //???
	msg.securityoptions = 0;
	msg.sequencenumber = sequencenumber;
	msg.payload.len = 2;
	msg.payload.data = &payloadData[0];
	msg.payload.data[0] = commandId;
	msg.payload.data[1] = statusCode;
	
	len = gw_send_zcl_frame_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len);

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW;
		pkt->header.cmd_id = GW_CMD_ID_T__GW_SEND_ZCL_FRAME_REQ;

		gw_send_zcl_frame_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt, NULL,NULL) !=0)
		{
			UI_PRINT_LOG("system_send_default_response: Error: Could not send msg.");
		}

		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("system_send_default_response: Error: Could not pack msg.");
	}
}

void system_process_zcl_frame_receive_indication(pkt_buf_t * pkt)
{
	GwZclFrameReceiveInd * msg = NULL;
	pkt_buf_t * assigned_macro;
	uint32_t endpointidsrc;
	

	if (pkt->header.cmd_id != GW_CMD_ID_T__GW_ZCL_FRAME_RECEIVE_IND)
	{
		return;
	}
	
	UI_PRINT_LOG("system_process_zcl_frame_receive_indication: Received GW_ZCL_FRAME_RECEIVE_IND");

	msg = gw_zcl_frame_receive_ind__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		endpointidsrc = msg->srcaddress->has_endpointid ? msg->srcaddress->endpointid : 0xFF;
		UI_PRINT_LOG("profileid = 0x%04X", msg->profileid);
		UI_PRINT_LOG("endpointiddest = 0x%04X", msg->endpointiddest);
		UI_PRINT_LOG("endpointidsrc = 0x%04X", endpointidsrc);
		UI_PRINT_LOG("clusterid = 0x%04X", msg->clusterid);
		UI_PRINT_LOG("frametype = %d", msg->frametype);
		UI_PRINT_LOG("manufacturerspecificflag = %d", msg->manufacturerspecificflag);
		if (msg->has_manufacturercode)
		{
			UI_PRINT_LOG("manufacturercode = 0x%04X", msg->manufacturercode);
		}
		else
		{
			UI_PRINT_LOG("manufacturercode = (none)");
		}
		UI_PRINT_LOG("clientserverdirection = %d", msg->clientserverdirection);
		UI_PRINT_LOG("commandid = %d", msg->commandid);
		UI_PRINT_LOG("payload = 0x%08X", (uint32_t *)&msg->payload);

		if ((msg->profileid == 0x0104) &&
			(msg->clusterid == CLUSTER_ID_MFC_SMART_THINGS_HUMIDITY_MEASUREMENT) &&
			(msg->frametype == GW_FRAME_TYPE_T__FRAME_VALID_ACCROSS_PROFILE) &&
			(msg->manufacturerspecificflag == GW_MFR_SPECIFIC_FLAG_T__MFR_SPECIFIC) &&
			(msg->has_manufacturercode == true) &&
			(msg->manufacturercode == 0xC2DF) &&
			(msg->clientserverdirection == GW_CLIENT_SERVER_DIR_T__SERVER_TO_CLIENT) &&
			(msg->commandid == 1))
		{ 
			//assuming always used to read a single specific attribute, so some values below are hardcoded
			attribute_info_t attr_recvd[1];

			attr_recvd[0].valid = true;
			attr_recvd[0].attr_id = 0x0000;
			attr_recvd[0].attr_type = 0x21;

			memcpy(attr_recvd[0].attr_val, 
				&msg->payload.data[4],
				2);

			attr_update_attribute_in_dev_table(msg->srcaddress->ieeeaddr, 
				msg->srcaddress->endpointid, msg->clusterid,
				1, attr_recvd);  

			ui_refresh_display();
		}

		if ((msg->profileid == 0x0104) &&
			(msg->clusterid == 0x0402) &&
			(msg->frametype == GW_FRAME_TYPE_T__FRAME_VALID_ACCROSS_PROFILE) &&
			(msg->manufacturerspecificflag == GW_MFR_SPECIFIC_FLAG_T__NON_MFR_SPECIFIC) &&
			(msg->has_manufacturercode == false) &&
			(msg->clientserverdirection == GW_CLIENT_SERVER_DIR_T__SERVER_TO_CLIENT) &&
			(msg->commandid == 1))
		{ 
			//assuming always used to read a single specific attribute, so some values below are hardcoded
			attribute_info_t attr_recvd[1];

			attr_recvd[0].valid = true;
			attr_recvd[0].attr_id = 0x0000;
			attr_recvd[0].attr_type = 0x29;

			memcpy(attr_recvd[0].attr_val, 
				&msg->payload.data[4],
				2);

			attr_update_attribute_in_dev_table(msg->srcaddress->ieeeaddr, 
				msg->srcaddress->endpointid, msg->clusterid, 
				1, attr_recvd);  

			ui_refresh_display();
		}

		if ((msg->profileid == 0x0104) &&
			(msg->clusterid == 0x0006) &&
			(msg->frametype == GW_FRAME_TYPE_T__FRAME_CLUSTER_SPECIFIC) &&
			(msg->manufacturerspecificflag == 0) &&
			(msg->has_manufacturercode == false) &&
			(msg->clientserverdirection == 0) &&
			(msg->commandid == 2))
		{
			if (assign_macro)
			{
				assign_macro = false;
				
				if (pending_macro != NULL)
				{
					if (macro_assign_new(msg->endpointiddest, endpointidsrc) == true)
					{
						UI_PRINT_LOG("Macro assigned successfully to endpoint %d", msg->endpointiddest);
					}
				}

				ui_redraw_toggles_indications();
			}
			else
			{
				assigned_macro = macro_retrieve(msg->endpointiddest, endpointidsrc);
				if (assigned_macro != NULL)
				{
					UI_PRINT_LOG("Executing macro (endpoint %d-->%d)", endpointidsrc, msg->endpointiddest);
					si_send_packet(assigned_macro, &macro_confirmation_handler, NULL);
				}
			}
		}

		if ((msg->profileid == 0x0104) &&
			(msg->clusterid == 0x0003) && //identify
			(msg->frametype == GW_FRAME_TYPE_T__FRAME_CLUSTER_SPECIFIC) && //cluster-specific command
			(msg->manufacturerspecificflag == 0) && //not manufacturere specific
			(msg->has_manufacturercode == false) && //same
			(msg->clientserverdirection == 0)) //client to server
		{
			attr_process_identify_cluster_commands(msg);
		}
			
		gw_zcl_frame_receive_ind__free_unpacked(msg, NULL);  
	}
	else
	{
		UI_PRINT_LOG("system_process_zcl_frame_receive_indication: Error Could not unpack msg");
	}
}
