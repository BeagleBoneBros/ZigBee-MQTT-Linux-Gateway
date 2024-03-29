/*******************************************************************************
 Filename:      attribute_engine.h
 Revised:        $Date$
 Revision:       $Revision$

 Description:	 Engine to send attribute read requests and handle responses.
 	 	 This engine is called by state reflector unit.


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
#ifndef ATTRIBUTE_ENGINE_H
#define ATTRIBUTE_ENGINE_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "types.h"
#include "gateway.pb-c.h"

/******************************************************************************
 * Function definitions
 *****************************************************************************/
void attr_send_read_attribute_request(zb_addr_t *addr, uint32_t cluster_id, uint16_t attr_count, uint32_t * attr_id, uint8_t isServerToClient);
extern void attr_send_write_attribute_request(zb_addr_t *addr, uint32_t cluster_id, uint16_t attr_count, GwAttributeRecordT ** attrRecord);
bool attr_update_attribute_in_dev_table(uint64_t ieee_address, uint8_t endpoint_id, 	uint32_t clusterid, uint32_t num_attrs, attribute_info_t *attr_list);
void attr_process_read_attribute_response(pkt_buf_t * pkt);
void attr_set_attribute_report_req (zb_addr_t *addr,uint32_t count, uint32_t cluster_id,  uint32_t minreportinterval,  uint32_t maxreportinterval, uint32_t* attributeid,GwZclAttributeDataTypesT* attributetype );
void attr_process_set_attribute_reporting_rsp_ind(pkt_buf_t * pkt);
void attr_process_attribute_report_ind(pkt_buf_t * pkt);
void attr_configure_reporting(zb_addr_t reporting_source_addr, zb_addr_t reporting_destination_addr, uint16_t interval_seconds, uint32_t cluster_id, uint32_t attributeid, GwZclAttributeDataTypesT attributetype );
void attr_get_attribute_list(zb_addr_t * addr);
void attr_process_attribute_list_rsp_ind(pkt_buf_t * pkt);
void attr_change_ind_handler(pkt_buf_t *pkt);
void attr_write_response_indication_handler (pkt_buf_t *pkt);
void attr_process_identify_cluster_commands(GwZclFrameReceiveInd * msg);
void attr_send_identify_request_as_raw_zcl(GwAddressStructT *dstaddress, uint16_t identifyTime);
void attr_send_read_attr_as_raw_zcl(GwAddressStructT *dstaddress, uint32_t cluster_id, uint32_t attr_id, bool manufacturer_specific, uint16_t manufacturercode);

#endif /* ATTRIBUTE_ENGINE_H */
