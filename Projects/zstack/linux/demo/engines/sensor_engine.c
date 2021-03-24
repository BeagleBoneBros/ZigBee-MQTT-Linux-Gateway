/*******************************************************************************
 Filename:      sensor_engine.c
 Revised:        $Date$
 Revision:       $Revision$

 Description:	Handle sensor activity


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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "data_structures.h"
#include "socket_interface.h"
#include "actions_engine.h"
#include "gateway.pb-c.h"
#include "user_interface.h"
#include "state_reflector.h"
#include "attribute_engine.h"
#include "string.h"

/*******************************************************************************
 * Types
 ******************************************************************************/

typedef struct
{
	zb_addr_t addr;
	uint8_t seqNum;
	uint32_t zoneId;
} szer_args;


/*******************************************************************************
 * Variables
 ******************************************************************************/

endpoint_info_t * zoneEnrollEndpointPtr = NULL;

/*******************************************************************************
 * Functions
 ******************************************************************************/

void snsr_process_power_response(pkt_buf_t * pkt)
{
	DevGetPowerRspInd *msg = NULL;
	int cluster_id = ZCL_CLUSTER_ID_SE_SIMPLE_METERING;
	attribute_info_t attr_info;

	if (pkt->header.cmd_id != GW_CMD_ID_T__DEV_GET_POWER_RSP_IND)
	{
		return;
	}

	msg = dev_get_power_rsp_ind__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		if (msg->status == GW_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("snsr_process_power_response: SUCCESS.");

			attr_info.valid = true;
			attr_info.attr_id = ATTRID_SE_METERING_INSTANTANEOUS_DEMAND;
			attr_info.attr_type =  GW_ZCL_ATTRIBUTE_DATA_TYPES_T__ZCL_DATATYPE_INT24;
			memcpy(attr_info.attr_val, &msg->powervalue, 4);

			attr_update_attribute_in_dev_table(msg->srcaddress->ieeeaddr, 
			msg->srcaddress->endpointid, cluster_id, 1, &attr_info);

			ui_refresh_display();
		}
		else
		{
			UI_PRINT_LOG("snsr_process_power_response: FAILURE (%d)", msg->status);
		}

		dev_get_power_rsp_ind__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("snsr_process_power_response: Error Could not unpack msg");
	}
}

void snsr_process_temperature_response(pkt_buf_t * pkt)
{
	DevGetTempRspInd *msg = NULL;
	int cluster_id = ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT;
	attribute_info_t attr_info;

	if (pkt->header.cmd_id != GW_CMD_ID_T__DEV_GET_TEMP_RSP_IND)
	{
		return;
	}
	
	msg = dev_get_temp_rsp_ind__unpack(NULL, pkt->header.len, 
	pkt->packed_protobuf_packet);

	if (msg )
	{
		if (msg->status == GW_STATUS_T__STATUS_SUCCESS)
		{
			UI_PRINT_LOG("snsr_process_temperature_response: Status SUCCESS.");

			attr_info.valid = true;
			attr_info.attr_id = ATTRID_MS_TEMPERATURE_MEASURED_VALUE;
			attr_info.attr_type =  GW_ZCL_ATTRIBUTE_DATA_TYPES_T__ZCL_DATATYPE_INT16;
			memcpy(attr_info.attr_val, &msg->temperaturevalue, 2);

			attr_update_attribute_in_dev_table(msg->srcaddress->ieeeaddr, msg->srcaddress->endpointid, cluster_id, 1, &attr_info);

			ui_refresh_display();
		}
		else
		{
			UI_PRINT_LOG("snsr_process_temperature_response: Error Status FAILURE");
		}

		dev_get_temp_rsp_ind__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("snsr_process_temperature_response: Error Could not unpack msg");
	}
}

void snsr_process_confirmation(pkt_buf_t * pkt, void * arg)
{

	GwZigbeeGenericCnf *msg = NULL;

	if (pkt->header.cmd_id != GW_CMD_ID_T__ZIGBEE_GENERIC_CNF)
	{
		return;
	}

	msg = gw_zigbee_generic_cnf__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		UI_PRINT_LOG("Sensor read confirmation: Status : %s", (msg->status == GW_STATUS_T__STATUS_SUCCESS) ? "SUCCESS." : "FAILURE.");

		gw_zigbee_generic_cnf__free_unpacked(msg, NULL);	
	}
}

void snsr_get_power(zb_addr_t * addr)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	DevGetPowerReq msg = DEV_GET_POWER_REQ__INIT;
	GwAddressStructT dstaddr;

	UI_PRINT_LOG("snsr_get_power: started.");

	si_compose_address(addr, &dstaddr);

	msg.dstaddress = &dstaddr;

	len = dev_get_power_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len); 

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW;
		pkt->header.cmd_id = GW_CMD_ID_T__DEV_GET_POWER_REQ;

		dev_get_power_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt,(confirmation_processing_cb_t)&snsr_process_confirmation, NULL) != 0)
		{
			UI_PRINT_LOG("snsr_get_power: Error: Could not send msg.");
		}
		
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("snsr_get_power: Error: Could not pack msg.");
	}
}
 
void snsr_get_temperature(zb_addr_t * addr)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	DevGetTempReq msg = DEV_GET_TEMP_REQ__INIT;
	GwAddressStructT dstaddr;

	si_compose_address(addr, &dstaddr);

	msg.dstaddress = &dstaddr;

	len = dev_get_temp_req__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len); 
	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW; //gateway
		pkt->header.cmd_id = GW_CMD_ID_T__DEV_GET_TEMP_REQ;

		dev_get_temp_req__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt, (confirmation_processing_cb_t)&snsr_process_confirmation, NULL) != 0)
		{
			UI_PRINT_LOG("snsr_get_temperature: Error: Could not send msg.");
		}
		
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("snsr_get_temperature: Error: Could not pack msg.");
	}
}

void snsr_send_zone_enrollment_rsp(bool timed_out, szer_args * arg)
{
	pkt_buf_t * pkt = NULL;
	uint8_t len = 0;
	DevZoneEnrollmentRsp msg = DEV_ZONE_ENROLLMENT_RSP__INIT;
	GwAddressStructT dstaddr;

	UI_PRINT_LOG("snsr_send_zone_enrollment_rsp: started.");

	si_compose_address(&arg->addr, &dstaddr);

	msg.dstaddress = &dstaddr;
	msg.zoneid = arg->zoneId;
	msg.enrollmentresponsecode = GW_ENROLL_RSP_CODE_T__ZONE_ENROLL_SUCCESS;

	len = dev_zone_enrollment_rsp__get_packed_size(&msg);
	pkt = malloc(sizeof(pkt_buf_hdr_t) + len); 

	if (pkt)
	{
		pkt->header.len = len;
		pkt->header.subsystem = Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW;
		pkt->header.cmd_id = GW_CMD_ID_T__DEV_ZONE_ENROLLMENT_RSP;

		dev_zone_enrollment_rsp__pack(&msg, pkt->packed_protobuf_packet);

		if (si_send_packet(pkt,NULL /*(confirmation_processing_cb_t)&snsr_process_confirmation*/, NULL) != 0)
		{
			UI_PRINT_LOG("snsr_send_zone_enrollment_rsp: Error: Could not send msg.");
		}
		
		free(pkt);
	}
	else
	{
		UI_PRINT_LOG("snsr_send_zone_enrollment_rsp: Error: Could not pack msg.");
	}

	free(arg);

	si_unregister_idle_callback();
}
 
void snsr_process_zone_enrollment_req(pkt_buf_t * pkt)
{
	DevZoneEnrollmentReqInd *msg = NULL;
	static uint32_t zoneId = 0;
	szer_args * arg;

	if (pkt->header.cmd_id != GW_CMD_ID_T__DEV_ZONE_ENROLLMENT_REQ_IND)
	{
		return;
	}

	msg = dev_zone_enrollment_req_ind__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{

		arg = malloc(sizeof(szer_args));
		if (arg == NULL)
		{
			UI_PRINT_LOG("snsr_send_zone_enrollment_rsp: Error allocating memory");
		}
		else
		{
			arg->addr.ieee_addr = msg->srcaddress->ieeeaddr;
			arg->addr.endpoint = msg->srcaddress->endpointid;
			arg->zoneId = zoneId++;
			arg->seqNum = msg->sequencenumber;
			
			si_register_idle_callback((si_idle_calback_t)&snsr_send_zone_enrollment_rsp , arg);
			if (!si_is_waiting_for_confirmation())
			{
				si_initiate_idle_callback();
			}
		}

		dev_zone_enrollment_req_ind__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("snsr_send_zone_enrollment_rsp: Error Could not unpack msg");
	}
}


void snsr_process_zone_status_change_ind(pkt_buf_t * pkt)
{
	DevZoneStatusChangeInd *msg = NULL;
	char log_string[180]; //TBD (size)
	attribute_info_t attr = {true, ATTRID_SS_IAS_ZONE_STATUS, GW_ZCL_ATTRIBUTE_DATA_TYPES_T__ZCL_DATATYPE_BITMAP16, {0}};

	if (pkt->header.cmd_id != GW_CMD_ID_T__DEV_ZONE_STATUS_CHANGE_IND)
	{
		return;
	}

	msg = dev_zone_status_change_ind__unpack(NULL, pkt->header.len, pkt->packed_protobuf_packet);

	if (msg)
	{
		STRING_START(log_string, "===========> snsr_process_zone_status_change_ind: ");
		STRING_ADD(log_string, "addr = %08LX,", msg->srcaddress->ieeeaddr);
		STRING_ADD(log_string, "ep = %08X,", msg->srcaddress->endpointid);
		STRING_ADD(log_string, "zonestatus = %08X,", msg->zonestatus);
		STRING_ADD(log_string, "extendedstatus = %08X", msg->extendedstatus);
		UI_PRINT_LOG(log_string);

		memcpy(attr.attr_val, &msg->zonestatus, 2);
				
		attr_update_attribute_in_dev_table(msg->srcaddress->ieeeaddr, 
			msg->srcaddress->endpointid, CLUSTER_ID_IAS_ZONE, 
			1, &attr);

		ui_refresh_display();

		dev_zone_status_change_ind__free_unpacked(msg, NULL);
	}
	else
	{
		UI_PRINT_LOG("snsr_process_zone_status_change_ind: Error Could not unpack msg");
	}
}

void snsr_zone_commissioning_state_machine(bool timed_out, void * arg)
{
	zb_addr_t addr;
	GwAttributeRecordT cie_addr_attribute = GW_ATTRIBUTE_RECORD_T__INIT;
	GwAttributeRecordT * _cie_addr_attribute = &cie_addr_attribute;
	bool found = false;
	uint32_t attr_id;
	
	if (timed_out)
	{
		UI_PRINT_LOG("Timeout during sensor zone commissioning");
	}

	if (!si_is_server_ready(SI_SERVER_ID_GATEWAY))
	{
		UI_PRINT_LOG("sensor zone commissioning - gateway server unavailable");
	}
	else
	{
		int i, j;
		
		for (i = 0; (i < ds_devices_total) && !found; i++)
		{
			for (j = 0; (j < ds_device_table[i].num_endpoints) && !found ; j++)
			{
				if (ds_device_table[i].ep_list[j].device_id == DEVICE_ID_IAS_ZONE)
				{

					UI_PRINT_LOG("sensor zone commissioning - device %d ep %d state %d", i, j, ds_device_table[i].ep_list[j].zoneEnrollState);

					addr.ieee_addr = ds_device_table[i].ieee_addr;
					addr.endpoint = ds_device_table[i].ep_list[j].endpoint_id;

					found = true;
					
					switch (ds_device_table[i].ep_list[j].zoneEnrollState)
					{
						case IAS_ZONE_ENROLL_NONE:
							attr_id = ATTRID_SS_IAS_ZONE_TYPE;
							attr_send_read_attribute_request(&addr, CLUSTER_ID_IAS_ZONE, 1, &attr_id, 0);
							ds_device_table[i].ep_list[j].zoneEnrollState = IAS_ZONE_ENROLL_ZONE_TYPE_READ; //no retry for this step. move to the next step regardless of the confirmation result
							break;
						case IAS_ZONE_ENROLL_ZONE_TYPE_READ:
							cie_addr_attribute.attributeid = ATTRID_SS_IAS_CIE_ADDRESS;
							cie_addr_attribute.attributetype = GW_ZCL_ATTRIBUTE_DATA_TYPES_T__ZCL_DATATYPE_IEEE_ADDR;
							cie_addr_attribute.attributevalue.len = sizeof(ds_device_table[0].ieee_addr);
							cie_addr_attribute.attributevalue.data = (uint8_t *)&(ds_device_table[0].ieee_addr);
							zoneEnrollEndpointPtr = &(ds_device_table[i].ep_list[j]);
							attr_send_write_attribute_request(&addr, CLUSTER_ID_IAS_ZONE, 1, &_cie_addr_attribute);
							break;
						default:
							found = false;
							break;
					}
				}
			}
		}
	}

	if (!found)
	{
		si_unregister_idle_callback();
		UI_PRINT_LOG("sensor read - idle");
	}

	UI_PRINT_LOG("snsr_zone_commissioning_state_machine - done");
}


void snsr_start_zone_commissioning_state_machine(void)
{
	si_register_idle_callback(snsr_zone_commissioning_state_machine, NULL);

	if (!si_is_waiting_for_confirmation())
	{
		si_initiate_idle_callback();
	}
}


