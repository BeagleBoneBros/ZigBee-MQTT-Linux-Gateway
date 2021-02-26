/*******************************************************************************
 Filename:       commissioning_engine.c
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

#ifndef COMMISSIONING_ENGINE_H
#define COMMISSIONING_ENGINE_H

#define BDB_NWK_FINDING_AND_BINDING_INITIATOR                  0x01
#define BDB_NWK_FINDING_AND_BINDING_TARGET                     0x02

// General Clusters
#define ZCL_CLUSTER_ID_GEN_BASIC                             0x0000
#define ZCL_CLUSTER_ID_GEN_POWER_CFG                         0x0001
#define ZCL_CLUSTER_ID_GEN_DEVICE_TEMP_CONFIG                0x0002
#define ZCL_CLUSTER_ID_GEN_IDENTIFY                          0x0003
#define ZCL_CLUSTER_ID_GEN_GROUPS                            0x0004
#define ZCL_CLUSTER_ID_GEN_SCENES                            0x0005
#define ZCL_CLUSTER_ID_GEN_ON_OFF                            0x0006
#define ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG              0x0007
#define ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL                     0x0008
#define ZCL_CLUSTER_ID_GEN_ALARMS                            0x0009
#define ZCL_CLUSTER_ID_GEN_TIME                              0x000A
#define ZCL_CLUSTER_ID_GEN_LOCATION                          0x000B
#define ZCL_CLUSTER_ID_GEN_ANALOG_INPUT_BASIC                0x000C
#define ZCL_CLUSTER_ID_GEN_ANALOG_OUTPUT_BASIC               0x000D
#define ZCL_CLUSTER_ID_GEN_ANALOG_VALUE_BASIC                0x000E
#define ZCL_CLUSTER_ID_GEN_BINARY_INPUT_BASIC                0x000F
#define ZCL_CLUSTER_ID_GEN_BINARY_OUTPUT_BASIC               0x0010
#define ZCL_CLUSTER_ID_GEN_BINARY_VALUE_BASIC                0x0011
#define ZCL_CLUSTER_ID_GEN_MULTISTATE_INPUT_BASIC            0x0012
#define ZCL_CLUSTER_ID_GEN_MULTISTATE_OUTPUT_BASIC           0x0013
#define ZCL_CLUSTER_ID_GEN_MULTISTATE_VALUE_BASIC            0x0014
#define ZCL_CLUSTER_ID_GEN_COMMISSIONING                     0x0015
#define ZCL_CLUSTER_ID_GEN_PARTITION                         0x0016

#define ZCL_CLUSTER_ID_OTA                                   0x0019

#define ZCL_CLUSTER_ID_GEN_POWER_PROFILE                     0x001A
#define ZCL_CLUSTER_ID_GEN_APPLIANCE_CONTROL                 0x001B

#define ZCL_CLUSTER_ID_GEN_POLL_CONTROL                      0x0020

#define ZCL_CLUSTER_ID_GREEN_POWER_PROXY                     0x0021

// Closures Clusters
#define ZCL_CLUSTER_ID_CLOSURES_SHADE_CONFIG                 0x0100
#define ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK                    0x0101
#define ZCL_CLUSTER_ID_CLOSURES_WINDOW_COVERING              0x0102

// HVAC Clusters
#define ZCL_CLUSTER_ID_HVAC_PUMP_CONFIG_CONTROL              0x0200
#define ZCL_CLUSTER_ID_HVAC_THERMOSTAT                       0x0201
#define ZCL_CLUSTER_ID_HVAC_FAN_CONTROL                      0x0202
#define ZCL_CLUSTER_ID_HVAC_DIHUMIDIFICATION_CONTROL         0x0203
#define ZCL_CLUSTER_ID_HVAC_USER_INTERFACE_CONFIG            0x0204

// Lighting Clusters
#define ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL                0x0300
#define ZCL_CLUSTER_ID_LIGHTING_BALLAST_CONFIG               0x0301

// Measurement and Sensing Clusters
#define ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT            0x0400
#define ZCL_CLUSTER_ID_MS_ILLUMINANCE_LEVEL_SENSING_CONFIG   0x0401
#define ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT            0x0402
#define ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT               0x0403
#define ZCL_CLUSTER_ID_MS_FLOW_MEASUREMENT                   0x0404
#define ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY                  0x0405
#define ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING                  0x0406

// Security and Safety (SS) Clusters
#define ZCL_CLUSTER_ID_SS_IAS_ZONE                           0x0500
#define ZCL_CLUSTER_ID_SS_IAS_ACE                            0x0501
#define ZCL_CLUSTER_ID_SS_IAS_WD                             0x0502

// Protocol Interfaces
#define ZCL_CLUSTER_ID_PI_GENERIC_TUNNEL                     0x0600
#define ZCL_CLUSTER_ID_PI_BACNET_PROTOCOL_TUNNEL             0x0601
#define ZCL_CLUSTER_ID_PI_ANALOG_INPUT_BACNET_REG            0x0602
#define ZCL_CLUSTER_ID_PI_ANALOG_INPUT_BACNET_EXT            0x0603
#define ZCL_CLUSTER_ID_PI_ANALOG_OUTPUT_BACNET_REG           0x0604
#define ZCL_CLUSTER_ID_PI_ANALOG_OUTPUT_BACNET_EXT           0x0605
#define ZCL_CLUSTER_ID_PI_ANALOG_VALUE_BACNET_REG            0x0606
#define ZCL_CLUSTER_ID_PI_ANALOG_VALUE_BACNET_EXT            0x0607
#define ZCL_CLUSTER_ID_PI_BINARY_INPUT_BACNET_REG            0x0608
#define ZCL_CLUSTER_ID_PI_BINARY_INPUT_BACNET_EXT            0x0609
#define ZCL_CLUSTER_ID_PI_BINARY_OUTPUT_BACNET_REG           0x060A
#define ZCL_CLUSTER_ID_PI_BINARY_OUTPUT_BACNET_EXT           0x060B
#define ZCL_CLUSTER_ID_PI_BINARY_VALUE_BACNET_REG            0x060C
#define ZCL_CLUSTER_ID_PI_BINARY_VALUE_BACNET_EXT            0x060D
#define ZCL_CLUSTER_ID_PI_MULTISTATE_INPUT_BACNET_REG        0x060E
#define ZCL_CLUSTER_ID_PI_MULTISTATE_INPUT_BACNET_EXT        0x060F
#define ZCL_CLUSTER_ID_PI_MULTISTATE_OUTPUT_BACNET_REG       0x0610
#define ZCL_CLUSTER_ID_PI_MULTISTATE_OUTPUT_BACNET_EXT       0x0611
#define ZCL_CLUSTER_ID_PI_MULTISTATE_VALUE_BACNET_REG        0x0612
#define ZCL_CLUSTER_ID_PI_MULTISTATE_VALUE_BACNET_EXT        0x0613
#define ZCL_CLUSTER_ID_PI_11073_PROTOCOL_TUNNEL              0x0614

// Advanced Metering Initiative (SE) Clusters
#define ZCL_CLUSTER_ID_SE_PRICING                            0x0700
#define ZCL_CLUSTER_ID_SE_LOAD_CONTROL                       0x0701
#define ZCL_CLUSTER_ID_SE_SIMPLE_METERING                    0x0702
#define ZCL_CLUSTER_ID_SE_MESSAGE                            0x0703
#define ZCL_CLUSTER_ID_SE_SE_TUNNELING                       0x0704
#define ZCL_CLUSTER_ID_SE_PREPAYMENT                         0x0705
#define ZCL_CLUSTER_ID_SE_ENERGY_MGMT                        0x0706
#define ZCL_CLUSTER_ID_SE_TOU_CALENDAR                       0x0707
#define ZCL_CLUSTER_ID_SE_DEVICE_MGMT                        0x0708
#define ZCL_CLUSTER_ID_SE_EVENTS                             0x0709
#define ZCL_CLUSTER_ID_SE_MDU_PAIRING                        0x070A

#define ZCL_CLUSTER_ID_GEN_KEY_ESTABLISHMENT                 0x0800

#define ZCL_CLUSTER_ID_HA_APPLIANCE_IDENTIFICATION           0x0B00
#define ZCL_CLUSTER_ID_HA_METER_IDENTIFICATION               0x0B01
#define ZCL_CLUSTER_ID_HA_APPLIANCE_EVENTS_ALERTS            0x0B02
#define ZCL_CLUSTER_ID_HA_APPLIANCE_STATISTICS               0x0B03
#define ZCL_CLUSTER_ID_HA_ELECTRICAL_MEASUREMENT             0x0B04
#define ZCL_CLUSTER_ID_HA_DIAGNOSTIC                         0x0B05

// Light Link cluster
#define ZCL_CLUSTER_ID_LIGHT_LINK                           0x1000


/*******************************************************************************
 * Enumerations
 ******************************************************************************/

typedef enum _nwkBindStatus {
  BIND_RESPONSE_SUCCESS = 0,
  BIND_RESPONSE_FAILURE = 1,
} nwkBindStatus;

typedef enum _nwkZdpStatus {
  NWK_ZDP_STATUS__SUCCESS = 0,
  NWK_ZDP_STATUS__INVALID_REQTYPE = 128,
  NWK_ZDP_STATUS__DEVICE_NOT_FOUND = 129,
  NWK_ZDP_STATUS__INVALID_EP = 130,
  NWK_ZDP_STATUS__NOT_ACTIVE = 131,
  NWK_ZDP_STATUS__NOT_SUPPORTED = 132,
  NWK_ZDP_STATUS__TIMEOUT = 133,
  NWK_ZDP_STATUS__NO_MATCH = 134,
  NWK_ZDP_STATUS__NO_ENTRY = 136,
  NWK_ZDP_STATUS__NO_DESCRIPTOR = 137,
  NWK_ZDP_STATUS__INSUFFICIENT_SPACE = 138,
  NWK_ZDP_STATUS__NOT_PERMITTED = 139,
  NWK_ZDP_STATUS__TABLE_FULL = 140,
  NWK_ZDP_STATUS__NOT_AUTHORIZED = 141,
  NWK_ZDP_STATUS__BINDING_TABLE_FULL = 142
} nwkZdpStatus;

/*******************************************************************************
 * Function prototypes
 ******************************************************************************/

void comm_send_permit_join(uint8_t joinTime);
void comm_start_commissioning_req(uint8_t commMode);
uint8_t comm_start_finding_and_binding_req( uint8_t endpoint );
void comm_add_installcode_req(InstallCode_t installcode);
void comm_remove_device_request(zb_addr_t * addr);
void comm_device_binding_entry_request(zb_addr_t * source_addr, zb_addr_t * dsta_ddr, uint32_t cluster_id , binding_mode_t binding_mode);
void comm_device_binding_table_request(zb_addr_t * dst_addr, uint8_t startIndex);
void comm_device_binding_entry_request_rsp_ind (pkt_buf_t *pkt);
void comm_device_binding_table_request_rsp_ind (pkt_buf_t *pkt);
uint8_t nwk_zclFindingBindingEpType( uint8_t n_inputClusters, uint16_t *inputClusters, uint8_t n_outputClusters, uint16_t *outputClusters );
uint8_t ZDO_nwk_AnyClusterMatches( uint8_t ACnt, uint16_t *AList, uint8_t BCnt, uint16_t *BList );
void dev_get_gw_endpoint_info_request(void);
void dev_send_identify_query( uint8_t endpoint );
void dev_send_simple_descriptor_req( uint16_t nwkAddr, uint8_t endpoint, uint8_t srcEndpoint );
void dev_set_finding_and_binding_timer_status_req( uint8_t status );
void dev_finding_and_binding_initiator_state_machine(uint8_t endpoint);
uint8_t dev_lookup_duplicates_respondants_table(uint16_t newNwkAddr, uint8_t newEndpoint);
void debug_print_table(void);
void dev_delete_respondant_table(void);
void dev_identify_query_response_handler(pkt_buf_t * pkt);
void dev_simple_descriptor_response_handler(pkt_buf_t * pkt);
//void dev_ieee_address_response_handler(pkt_buf_t * pkt);
void dev_binding_entry_request(zb_addr_t *srcAddr, zb_addr_t *dstAddr, uint16_t bindClusterId);
uint8_t dev_FindIfAppCluster( uint16_t ClusterId );
uint8_t dev_zclFindingBindingAddBindEntry( uint16_t bindClusterId, uint8_t n_clusterList, uint16_t *clusterList,
							                                  uint64_t dstIeeeAddr, uint8_t dstEndpoint );

void dev_checkMatchingEndpoints(void);
void dev_delete_binding_list(void);
void dev_process_binding_list_request(void);
uint8_t debug_calculate_entries(void);
extern void attr_update_identify_time_attribute(uint8_t endpoint, uint16_t identifyTime);
void dev_setBdbCommissioningGroupID(uint16_t groupID);
extern void gs_add_group(zb_addr_t * addr, uint16_t groupid, char * groupname);





#endif /* COMMISSIONING_ENGINE_H */
