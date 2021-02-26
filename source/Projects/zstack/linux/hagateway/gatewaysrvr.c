/**************************************************************************************************
 Filename:       gatewaysrvr.c
 Revised:        $Date$
 Revision:       $Revision 1.0.2$

 Description:    This file contains the Gateway Server.
 
  Copyright 2013-2014 Texas Instruments Incorporated. All rights reserved.

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
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include "zstack.pb-c.h"
#include "gateway.pb-c.h"
#include "hal_rpc.h"
#include "api_client.h"
#include "api_server.h"
#include "configparser.h"
#include "aps_groups.h"
#include "gatewaysrvr.h"
#include "gatewayservices.h"
#include "serverep.h"
#include "zcl_port.h"
#include "gatewayp2p.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_green_power.h"
#include "zcl_ha.h"
#include "zcl_lighting.h"
#include "zcl_closures.h"
#include "zcl_hvac.h"
#include "zcl_ss.h"
#include "zcl_poll_control.h"
#include "trace.h"
#include "gateway_pb_ctrl.h"
#include "zstack_pb_ctrl.h"
#include "server_pb_ctrl.h"
#include "nwkmgr_pb_ctrl.h"
#include "gp_common.h"
#include "dGP_stub.h"
#include "MT.h"
#include "MT_RPC.h"

/**************************************************************************************************
 * Constant
 **************************************************************************************************/

#define DEVICE_TIMEOUT                  9000      // default = 9 seconds (ms)
#define APP_TRANSACTION_TIMEOUT         15000     // 15 seconds (app needs to respond to server cmd requests within this time)

#define MAX_DEVICE_FAILED_ATTEMPTS      5   // make number of tries to issue message to 
                                            // remote device without handling device status


#define IEEE_ADDR_PRIORITY        FALSE  // if TRUE, use IEEE address for address mode priority when sending commands

#define GW_DEVICE_VERSION         0
#define GW_FLAGS                  0

#define GW_HWVERSION              0
#define GW_ZCLVERSION             0

#define LIGHT_OFF                 0x00
#define LIGHT_ON                  0x01

#define LIGHT_OFF                 0x00
#define LIGHT_ON                  0x01

#define TIMER_WAIT_PERIOD                           250000    // 1/4 second in usecs (must be < 1 sec)
#define TRANSACTION_INTERVAL_TIME                   250       // subtract timer value by 1/4 second intervals (ms)

#define DEFAULT_SET_COLOR_TRANSITION_TIME           0         // units in 1/10th of a second

//ZCL_CLUSTER_ID_SE_SIMPLE_METERING is going to change ZCL_CLUSTER_ID_SE_METERING soon
#ifndef ZCL_CLUSTER_ID_SE_SIMPLE_METERING
#define ZCL_CLUSTER_ID_SE_SIMPLE_METERING ZCL_CLUSTER_ID_SE_METERING
#endif

#define ATTRID_MS_TEMPERATURE_MEASURED_VALUE        0x0000
#define ATTRID_SE_INSTANTANEOUS_DEMAND              0x0400
#define ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE  0x0000

#define ALL_APP_CONNECTIONS         -1    // send commands to all connected apps

#define GATEWAY_SERVER_DEFAULT_PORT 2541

/*** Green Power constants ***/ 
#define GP_ENDPOINT          0xF2 
#define GP_CLUSTER_ID        0x0021 
 
#define GP_MAX_PROXY_TABLE_ENTRIES    0x0010 
#define GP_PROXY_TABLE          0x0011 
#define GP_NOTIFICATION_RETRY_NUMBER  0x0012 
#define GP_NOTIFICATION_RETRY_TIMER    0x0013 
#define GP_FUNCTIONALITY        0x0016 
#define GP_ACTIVE_FUNCTIONALITY      0x0017 
#define GP_SHARED_SECURITY_KEY_TYPE    0x0020 
#define GP_SHARED_SECURITY_KEY      0x0021 
#define GP_GPP_LINK_KEY         0x0022 
#define GP_GPP_CLUSTER_REVISION      0xFFFD 

#define GP_DUPLICATE_FILTERING_QUARTER_SECONDS  8

uint8 duplicateFilteringQuarterSeconds = 0;

#define CLUSTER_IDENTIFY 			0x0003
#define CMD_IDENTIFY_QUERY_RSP 		0x00



/**************************************************************************************************
 * Typedefs
 **************************************************************************************************/

/**************************************************************************************************
 * Structures
 **************************************************************************************************/

/**************************************************************************************************
 * Globals
 **************************************************************************************************/

apicHandle_t giZStackHandle;
apicHandle_t giNwkMgrHandle;

uint8 bdb_identify_query_sequence = 0;

uint8 GwFindingAndBindingTimerStatus = 0;

/**************************************************************************************************
 * Function Prototypes
 **************************************************************************************************/
static bool gwInit( void );
static void gwInitializeAttributes( void );
static void gwAppendTlgPathString( void );
static void timerHandler( int sig );
static void getUserInput( void );
static void transitionState( bool keypressed, int key );
static bool gwRegAllEp( void );
static void gwMsgTransPost( bool multRsp, uint8 rspType, int connection, uint64_t dstAddress, uint8 cmdId, 
                            uint16 appTransId, uint8 zclTransId, int transTimer );
static gsGwMsgTransTable_t *gwMsgTransGetByZclTransId( uint8 zclTransId, uint8 reqCmdId );
static gsGwMsgTransTable_t *gwMsgTransGetByZclTransIdGenericRsp( uint8 zclTransId );
static gsGwMsgTransTable_t *gwMsgTransGetByAppTransId( uint16 appTransId );
static gsGwMsgTransTable_t *gwMsgTransGetBySrcAddrCmdId( uint64_t srcAddress, uint8 reqCmdId );
static bool gwMsgRemoveTrans( uint16 appTransId );
static void gwHandleCnfRsp( uint8 rspType, uint8 addrType, int connection, 
                            uint64_t dstAddress, uint8 cmdId, ZStatusValues status );
static ZStatusValues gwSendUnicastRouteReq( uint16 shortAddr );
static ZStatus_t zcl_HdlIncoming( zclIncoming_t *pInMsg );
void gwHandlePbCb( int connection, uint8 subSys, uint8 cmdId, uint16 len, uint8 *pData, uint8 type );
static ZStatus_t zclSS_HdlCommands( zclIncoming_t *pInMsg );
static ZStatusValues gwHandleClientIndPbCb( zclIncoming_t *pInMsg );
static ZStatus_t gwHandleServerIndPbCb( zclIncoming_t *pInMsg );
static void handleAsyncMsgs( apicHandle_t handle, uint8 subSys, uint8 cmdID, uint16 len, uint8 *pMsg );
ZStatus_t gwZclDoorLockRspCB( zclIncoming_t *pInMsg, uint8 status );
static bool gwConvertAddrPbToAfReq( GwAddressStructT *pPbAddrStruct, afAddrType_t *pAfAddrStruct, 
                                    bool *pDisableDefaultRsp, uint8 addrModeMask );
static bool gwConvertAddrAfToPbRsp( GwAddressStructT *pPbAddrStruct, afAddrType_t *pAfAddrStruct );                                 
static bool gwConvertAddrPbToAfRsp( GwAddressStructT *pPbAddrStruct, afAddrType_t *pAfAddrStruct );                                 
static void gwTransTimeoutRsp( gsGwMsgTransTable_t *pMsgTableEntry );
static void gwDeviceReqRetryHandler( uint64_t ieeeAddr );
static void gwUpdateDeviceInRetryTable( uint64_t ieeeAddr );
static void gwSpecificRspTimeoutHandler( gsGwMsgTransTable_t *pMsgTableEntry );
static ZStatus_t gwZclReadWriteCB( uint16 clusterId, uint16 attrId,
                                   uint8 oper, uint8 *pValue, uint16 *pLen );
static void gwRspCountHandler( gsGwMsgTransTable_t *pMsgTableEntry );
static void gwSendSysNwkInfoReadReq( void );
static uint16 gwIncreaseCurrentAppSeqNum( void );
static void gwSetSelfAddressStruct( uint16 appSeqNum, GwAddressStructT *pPbAddrStruct );
static uint8 gwAnalogDataType( uint8 dataType );

ZStatusValues sendAPICExpectDefaultStatus( int cmdID, int len, uint8 *pData );
static void sendZbGenericCnf( int connection, GwZigbeeGenericCnf *pGenericCnf );
static void sendZbGenericRspInd( int connection, GwZigbeeGenericRspInd *pGenericRsp );
static ZStatus_t sendGwAddGroupReq( GwAddGroupReq *pAddGroupReq );
static ZStatus_t sendGwGetGroupMembershipReq( GwGetGroupMembershipReq *pGetGroupMembershipReq );
static ZStatusValues sendGwGetGroupMembershipRspInd( int connection, GwGetGroupMembershipRspInd *pGetGroupMbrRsp );
static ZStatus_t sendGwRemoveFromGroupReq( GwRemoveFromGroupReq *pRemoveFromGroupReq );
static ZStatus_t sendGwStoreSceneReq( GwStoreSceneReq *pStoreSceneReq );
static ZStatus_t sendGwRemoveSceneReq( GwRemoveSceneReq *pRemoveSceneReq );
static ZStatus_t sendGwRecallSceneReq( GwRecallSceneReq *pRecallSceneReq );
static ZStatus_t sendGetSceneMembershipReq( GwGetSceneMembershipReq *pGetSceneMembershipReq );
static ZStatusValues sendGwGetSceneMembershipRspInd( int connection, GwGetSceneMembershipRspInd *pGetSceneMbrRsp );
static ZStatusValues sendGwSleepyDeviceCheckInInd( GwSleepyDeviceCheckInInd *pSleepyDevCheckInInd );
static void sendGwAttributeChangeInd( GwAttributeChangeInd *pAttrChangeInd );
static void sendGwIdentifyQueryRsp( DevProcessIdentifyQueryRspInd *pIdentifyQueryRsp );
ZStatus_t sendGwGetDeviceAttributeListReq( int connection, GwGetDeviceAttributeListReq *pGetDeviceAttrListReq, uint16 clusterId, uint16 startAttr );
void sendGwGetDeviceAttributeListRspInd( int connection, GwGetDeviceAttributeListRspInd *pGetAttrListRsp );
static ZStatus_t sendGwReadDeviceAttributeReq( GwReadDeviceAttributeReq *pReadDeviceAttrReq );
static void sendGwReadDeviceAttributeRspInd( int connection, GwReadDeviceAttributeRspInd *pReadDeviceAttrRsp );
static ZStatus_t sendGwWriteDeviceAttributeReq( GwWriteDeviceAttributeReq *pWriteDeviceAttrReq );
static void sendGwWriteDeviceAttributeRspInd( int connection, GwWriteDeviceAttributeRspInd *pWriteAttrRsp );
static ZStatus_t sendGwSetAttributeReportingReq( GwSetAttributeReportingReq *pSetAttrReportReq );
static void sendGwSetAttributeReportingRspInd( int connection, GwSetAttributeReportingRspInd *pSetAttrReportingRsp );
static void sendGwAttributeReportingInd( GwAttributeReportingInd *pAttrReportInd );
static ZStatus_t sendGwSendZclFrameReq( GwSendZclFrameReq *pSendZclFrameReq );
static ZStatus_t sendGwAlarmInd( GwAlarmInd *pAlarmInd );
static ZStatusValues sendGwZclFrameReceiveInd( int connection,  GwZclFrameReceiveInd *pZclFrameInd );
static ZStatus_t sendGwAlarmResetReq( GwAlarmResetReq *pAlarmResetReq );
static ZStatus_t sendDevZoneStatusChangeInd( DevZoneStatusChangeInd *pZoneStatusChange );
static ZStatus_t sendDevZoneEnrollmentReqInd( DevZoneEnrollmentReqInd *pZoneEnrollReq );
static ZStatus_t sendDevZoneEnrollmentRsp( gsGwMsgTransTable_t *pTransEntry, DevZoneEnrollmentRsp *pZoneEnrollRsp );
static ZStatus_t sendDevAceArmReqInd( DevAceArmReqInd *pAceArmReq );
static ZStatus_t sendDevAceArmRsp( gsGwMsgTransTable_t *pTransEntry, DevAceArmRsp *pAceArmRsp );
static ZStatus_t sendDevAceBypassInd( DevAceBypassInd *pAceBypassInd );
static ZStatus_t sendDevAceEmergencyConditionInd( DevAceEmergencyConditionInd *pAceEmergencyConditionInd );
static ZStatus_t sendDevAceGetZoneIdMapReqInd( DevAceGetZoneIdMapReqInd *pAceGetZoneIdMapReq );
static ZStatus_t sendDevAceGetZoneIdMapRsp( DevAceGetZoneIdMapRsp *pAceGetZoneIdMapRsp, uint8 transSeqNum );
static ZStatus_t sendDevAceGetZoneInformationReqInd( DevAceGetZoneInformationReqInd *pAceGetZoneInfoReq );
static ZStatus_t sendDevAceGetZoneInformationRsp( gsGwMsgTransTable_t *pTransEntry, DevAceGetZoneInformationRsp *pAceGetZoneInfoRsp );
static ZStatus_t sendDevSetIdentifyModeReq( DevSetIdentifyModeReq *pSetIdentifyModeReq );
static ZStatus_t sendDevSetOnOffStateReq( DevSetOnOffStateReq *pOnOffStateReq );
static ZStatus_t sendDevSetLevelReq( DevSetLevelReq *pSetLevelReq );
static ZStatus_t sendDevGetLevelReq( DevGetLevelReq *pGetLevelReq );
static void sendDevGetLevelRspInd( int connection, DevGetLevelRspInd *pGetLevelRsp );
static ZStatus_t sendDevGetOnOffStateReq( DevGetOnOffStateReq *pGetOnOffStateReq );
static void sendDevGetOnOffStateRspInd( int connection, DevGetOnOffStateRspInd *pGetOnOffStateRsp );
static ZStatus_t sendDevSetColorReq( DevSetColorReq *pSetColorReq );
static ZStatus_t sendDevGetColorReq( DevGetColorReq *pGetColorReq );
static ZStatus_t sendDevSetColorTempReq( DevSetColorTempReq *pSetColorTempReq );
static ZStatus_t sendDevGetColorTempReq( DevGetColorTempReq *pGetColorTempReq );
static void sendDevGetColorRspInd( int connection, DevGetColorRspInd *pGetColorRsp );
static void sendDevGetColorTempRspInd( int connection, DevGetColorTempRspInd *pGetColorTempRsp );
static ZStatus_t sendDevGetTempReq( DevGetTempReq *pGetTempReq );
static void sendDevGetTempRspInd( int connection, DevGetTempRspInd *pGetTempRsp );
static ZStatus_t sendDevGetPowerReq( DevGetPowerReq *pGetPowerReq );
static void sendDevGetPowerRspInd( int connection, DevGetPowerRspInd *pGetPowerRsp );
static ZStatus_t sendDevGetHumidityReq( DevGetHumidityReq *pGetHumidityReq );
static void sendDevGetHumidityRspInd( int connection, DevGetHumidityRspInd *pGetHumidityRsp );
static ZStatus_t sendDevSetDoorLockReq( DevSetDoorLockReq *pSetDoorLockReq );
static ZStatusValues sendDevSetDoorLockRspInd( int connection, DevSetDoorLockRspInd *pSetDoorLockRsp );
static ZStatus_t sendDevGetDoorLockStateReq( DevGetDoorLockStateReq *pDoorLockStateReq );
static ZStatusValues sendDevGetDoorLockStateRspInd( int connection, DevGetDoorLockStateRspInd *pDoorLockStateRsp );
static ZStatus_t sendDevThermostatSetpointChangeReq( DevThermostatSetpointChangeReq *pThermostatSetpointChangeReq );
static ZStatus_t sendDevWindowCoveringActionReq( DevWindowCoveringActionReq *pWindowCoveringReq );


static void processAfIncomingMsgInd( AfIncomingMsgInd *pInMsg );
static void processGpSecReqIncomingMsgInd( GpSecReqIncomingMsgInd *pInMsg );
static void processGpDataIndIncomingMsgInd( GpDataIndIncomingMsgInd *pInMsg );
static void processZdoDeviceAnnounce( ZdoDeviceAnnounceInd *pInMsg );
static void processAfDataConfirmInd( AfDataConfirmInd *pInMsg );
void zclProcessInCmds( zclIncoming_t *pCmd );
static void processZclReadAttributeRsp( GwAddressStructT *pSrcAddress, uint8 zclTransId,
                                        uint16 clusterId, uint16 payloadLen, uint8 *pPayload );
static void processZclWriteAttributeRsp( GwAddressStructT *pSrcAddress, uint8 zclTransId, 
                                         uint16 clusterId, uint16 payloadLen, uint8 *pPayload );
static void processZclConfigReportRsp( GwAddressStructT *pSrcAddress, uint8 zclTransId, 
                                       uint16 clusterId, uint16 payloadLen, uint8 *pPayload );
static void processGenericResponseTriggers( uint8 zclTransId, uint8 status, char * zclCommandString );
static void processZclDiscoverAttributesRsp( GwAddressStructT *pSrcAddress, uint8 zclTransId, 
                                             uint16 clusterId, uint16 payloadLen, uint8 *pPayload );                                                                                 
static ZStatus_t processIasZoneClusterCmdInd( GwAddressStructT *pSrcAddress, zclIncoming_t *pInMsg );                                            
static ZStatus_t processIasAceClusterCmdInd( GwAddressStructT *pSrcAddress, zclIncoming_t *pInMsg );                                            
static void processGwAddGroupReq( int connection, GwAddGroupReq *pAddGroupReq );
static void processGwGetGroupMembershipReq( int connection, GwGetGroupMembershipReq *pGetGroupMembershipReq );
static void processGwGetGroupMembershipRspInd( GwAddressStructT *pSrcAddress, zclGroupRsp_t *pGroupRsp );
static void processGwRemoveFromGroupReq( int connection, GwRemoveFromGroupReq *pRemoveFromGroupReq );
static void processGwStoreSceneReq( int connection, GwStoreSceneReq *pStoreSceneReq );
static void processGwRemoveSceneReq( int connection, GwRemoveSceneReq *pRemoveSceneReq );
static void processGwRecallSceneReq( int connection, GwRecallSceneReq *pRecallSceneReq );
static void processGwGetSceneMembershipReq( int connection, GwGetSceneMembershipReq *pGetSceneMembershipReq );
static void processGwGetSceneMembershipRspInd( GwAddressStructT *pSrcAddress, zclSceneRsp_t *pSceneRsp );
static void processGwSleepyDevicePacketPendingReq( int connection, GwSleepyDevicePacketPendingReq *pSleepyPacketPendingReq );                               
static void processGwAttributeChangeInd( uint8 attrEp, uint16 clusterId, uint16 attrId, uint8 attrDataType, uint8 *pValue, uint16 len );
static void processGwGetDeviceAttributeListReq( int connection, GwGetDeviceAttributeListReq *pGetDeviceAttrListReq );
static void processGwReadDeviceAttributeReq( int connection, GwReadDeviceAttributeReq *pReadDeviceAttrReq );
static void processGwReadDeviceAttributeRspInd( int connection, GwReadDeviceAttributeRspInd *pReadAttrRsp, 
                                                uint16 payloadLen, uint8 *pPayload );
static void processGwWriteDeviceAttributeReq( int connection, GwWriteDeviceAttributeReq *pWriteDeviceAttrReq );
static void processGwWriteDeviceAttributeRspInd( int connection, GwWriteDeviceAttributeRspInd *pWriteAttrRsp, 
                                                 uint16 payloadLen, uint8 *pPayload );
static void processGwSetAttributeReportingReq( int connection, GwSetAttributeReportingReq *pSetAttrReportReq );
static void processGwSetAttributeReportingRspInd( int connection, GwSetAttributeReportingRspInd *pSetAttrReportingRsp, 
                                                  uint16 payloadLen, uint8 *pPayload );
static void processGwAttributeReportingInd( GwAddressStructT *pSrcAddress, uint16 clusterId, uint16 payloadLen, uint8 *pPayload );                                                   
static void processGwSendZclFrameReq( int connection, GwSendZclFrameReq *pSendZclFrameReq );
static ZStatusValues processGwZclFrameReceiveInd( int connection, GwAddressStructT *pSrcAddress, zclIncoming_t *pInMsg );
static ZStatus_t processGwAlarmInd( GwAddressStructT *pSrcAddress, uint8 alarmCode, uint16 clusterId );
static void processGwAlarmResetReq( int connection, GwAlarmResetReq *pAlarmResetReq );
static ZStatus_t processDevZoneStatusChangeInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData );
static ZStatus_t processDevZoneEnrollmentReqInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData );
static void processDevZoneEnrollmentRsp( int connection, DevZoneEnrollmentRsp *pDevZoneEnrollRsp );
static ZStatus_t processDevAceArmReqInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData );
static void processDevAceArmRsp( int connection, DevAceArmRsp *pAceArmRsp );
static ZStatus_t processDevAceBypassInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData );
static ZStatus_t processDevAceEmergencyConditionInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint8 cmdId );
static ZStatus_t processDevAceGetZoneIdMapReqInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData );
static void processDevAceGetZoneIdMapRsp( int connection, DevAceGetZoneIdMapRsp *pAceGetZoneIdMapRsp );
static ZStatus_t processDevAceGetZoneInformationReqInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData );
static void processDevAceGetZoneInformationRsp( int connection, DevAceGetZoneInformationRsp *pAceGetZoneInfoRsp );
static void processDevSetIdentifyModeReq( int connection, DevSetIdentifyModeReq *pSetIdentifyModeReq );
static void processDevSetOnOffStateReq( int connection, DevSetOnOffStateReq *pOnOffStateReq );
static void processDevSendIdentifyQueryReq( int connection, DevSendIdentifyQueryReq *pSendIdentifyQueryReq );
static void processDevSetFindingAndBindingTimerStatusReq( int connection, DevSetFindingAndBindingTimer *pSetFindingAndBindingTimerStatus );
static void processDevSetLevelReq( int connection, DevSetLevelReq *pSetLevelReq );
static void processDevGetLevelReq( int connection, DevGetLevelReq *pGetLevelReq );
static void processDevGetLevelRspInd( int connection, DevGetLevelRspInd *pGetLevelRsp, uint16 payloadLen, uint8 *pPayload );
static void processDevGetOnOffStateReq( int connection, DevGetOnOffStateReq *pGetOnOffStateReq );
static void processDevGetOnOffStateRspInd( int connection, DevGetOnOffStateRspInd *pGetOnOffStateRsp, uint16 payloadLen, uint8 *pPayload );
static void processDevSetColorReq( int connection, DevSetColorReq *pSetColorReq );
static void processDevGetColorReq( int connection, DevGetColorReq *pGetColorReq );
static void processDevGetColorRspInd( int connection, DevGetColorRspInd *pGetColorRsp, uint16 payloadLen, uint8 *pPayload );
static void processDevSetColorTempReq( int connection, DevSetColorTempReq *pSetColorTempReq );
static void processDevGetColorTempReq( int connection, DevGetColorTempReq *pGetColorTempReq );
static void processDevGetColorTempRspInd( int connection, DevGetColorTempRspInd *pGetColorTempRsp, uint16 payloadLen, uint8 *pPayload );
static void processDevGetTempReq( int connection, DevGetTempReq *pGetTempReq );
static void processDevGetTempRspInd( int connection, DevGetTempRspInd *pGetTempRsp, uint16 payloadLen, uint8 *pPayload );
static void processDevGetPowerReq( int connection, DevGetPowerReq *pGetPowerReq );
static void processDevGetPowerRspInd( int connection, DevGetPowerRspInd *pGetPowerRsp, uint16 payloadLen, uint8 *pPayload );
static void processDevGetHumidityReq( int connection, DevGetHumidityReq *pGetHumidityReq );
static void processDevGetHumidityRspInd( int connection, DevGetHumidityRspInd *pGetHumidityRsp, uint16 payloadLen, uint8 *pPayload );
static void processDevSetDoorLockReq( int connection, DevSetDoorLockReq *pSetDoorLockReq );
static ZStatus_t processDevSetDoorLockRspInd( GwAddressStructT *pSrcAddress, uint8 cmdId, uint8 transSeqNum, 
                                              uint16 len, uint8 *pData );
static void processDevGetDoorLockStateReq( int connection, DevGetDoorLockStateReq *pDoorLockStateReq );
static ZStatusValues processDevGetDoorLockStateRspInd( int connection, DevGetDoorLockStateRspInd *pDoorLockStateRsp, 
                                                       uint16 payloadLen, uint8 *pPayload  );
static void processDevThermostatSetpointChangeReq( int connection, DevThermostatSetpointChangeReq *pThermostatSetpointChangeReq );
static void processDevWindowCoveringActionReq( int connection, DevWindowCoveringActionReq *pWindowCoveringReq );
static ZStatus_t gwHandleMSPWCommands(zclIncoming_t * pInMsg); //MSPW stands for ManufacturerSpecificProfileWide
static ZStatusValues gwSendDeviceAnnounce( ZdoDeviceAnnounceReq *pZdoDeviceAnnounceReq );
static void processGwIdentifyQueryRspInd( uint16_t nwkAddr, uint8_t endpoint, uint64_t ieeeAddr);

/**************************************************************************************************
 * Locals
 **************************************************************************************************/

static DevState opState = DEV_STATE__INIT;

endPointDesc_t zEpDesc;


SysNwkInfoReadRsp gLocalDeviceInfo = SYS_NWK_INFO_READ_RSP__INIT;
DeviceTypes gLocalDeviceType = DEVICE_TYPES__INIT;

static uint16 gGwAppTransSeqNum = 0;  // app transaction sequence number (higher layer)

extern apisSysParams_t sysParams;

int giGwDeviceTimeout = DEVICE_TIMEOUT;

bool enableRawProcessingOfUnsupportedClusters = true;

// Parameter Descriptors
configTableItem_t configItems[] =
{
 { &(sysParams.port), "GW_SRVR_PORT", TYPE_UINT16, 1 },
 { gszConfigTlgPath, "GW_CONFIG_TLG_PATH", TYPE_STRING, (MAX_CONFIG_STRING_LEN - 1) },
 { &giGwDeviceTimeout, "TRANSACTION_TIMEOUT", TYPE_UINT16, 1 },
 { &enableRawProcessingOfUnsupportedClusters, "RAW_PROCESS_UNSUPPORTED_CLUSTERS", TYPE_BOOL, 1 },
 { &giLongPollInterval, "LONG_POLL_INTERVAL", TYPE_UINT32, 1 },
 { &giShortPollInterval, "SHORT_POLL_INTERVAL", TYPE_UINT16, 1 },
 TRACE_CONFIG_ITEMS
};

apisSysParams_t sysParams =
{
  GATEWAY_SERVER_DEFAULT_PORT,          // Default port
  TRUE,                                 // Network Manager Server Verbose mode
  (configTableItem_t *)configItems,     // Configuration structure array
  (sizeof ( configItems ) / sizeof (configTableItem_t)),  //
  2,                                    // 2 Client connections (ZStack, NwkMgr)
  handleAsyncMsgs,                      // function to handle incoming ZStack messages
  gwHandlePbCb,                          // handles incoming Gateway messages
  GATEWAY_LAYER_NUMBER,
  GATEWAY_SERVER_NAME
};

// one of three response types
enum gwRspType_t { GW_RSP_NONE, GW_RSP_GENERIC, GW_RSP_SPECIFIC };

//zclOptionRec_t gaGwApsOptions[] = { { 0xFFFE, 0 } };

/**************************************************************************************************
 **************************************************************************************************/

/*********************************************************************
 * @fn      appArgs
 *
 * @brief   Initialization input argument handler.
 *
 * @param   p_argc - pointer to number of arguments
 * @param   p_argv - pointer to arguments
 *
 * @return  Always returns 0
 */
int appArgs(int *p_argc, char ***p_argv)
{
  char **argv = *p_argv;
  int i;
  int argc = *p_argc;
  
  uiPrintfEx(trINFO, "There are %d args\n", argc );
  
  for ( i = 0; i < argc; ++i )
  {
    uiPrintfEx(trINFO, " argv[%d] = %s\n", i, argv[i] );
  }

  if ( argc < (sysParams.numClients + 1) )
  {
    uiPrintfEx(trERROR, "ERROR: Gateway Server must connect to:" );
    uiPrintfEx(trERROR, " ZStack Server Port (default):          2536" );
    uiPrintfEx(trERROR, " Network Manager Server Port (default): 2540\n" );
    
    return 253;  
  }

  return 0;
}

/*********************************************************************
 * @fn      appInit
 *
 * @brief   Provides system parameters
 *
 * @param   none
 *
 * @return  Pointer to sysParams
 */
apisSysParams_t *appInit( void )
{  
  if ((register_pb_pointer_list(&gateway_pb_pointer_list) == NULL) ||
    (register_pb_pointer_list(&nwkmgr_pb_pointer_list) == NULL) ||
    (register_pb_pointer_list(&server_pb_pointer_list) == NULL) ||
    (register_pb_pointer_list(&zstack_pb_pointer_list) == NULL))
  {
    uiPrintfEx(trERROR, "failed registering protobuf pointer lists\n");
  }
	
  return ( &sysParams );
}

int appInitPhase2( void )
{
  //nothing to do here
  return 0;
}

int appInitPhase3( apicHandle_t *handles )
{
  bool init_rc;

  //Usage message for key commands
  uiPrintfEx(trUNMASKABLE, "\n");
  uiPrintfEx(trUNMASKABLE, " ************************************************\n");
  uiPrintfEx(trUNMASKABLE, " *  Gateway Server v1.0.2d                      *\n");
  uiPrintfEx(trUNMASKABLE, " ************************************************\n"); 
  uiPrintfEx(trUNMASKABLE, "\n");

  // Setup the timer handler
  signal( SIGALRM, timerHandler );

  // API client handles
  giZStackHandle = handles[0];
  giNwkMgrHandle = handles[1];

  if ( giZStackHandle == NULL )
  {
    uiPrintfEx(trUNMASKABLE, "Error - No ZStack Server Detected. Exiting...\n\n");
    return 254;
  }

  if ( giNwkMgrHandle == NULL )
  {
    uiPrintfEx(trUNMASKABLE, "Error - No Network Manager Server Detected. Exiting...\n\n");
    return 254;
  }

  apicInitializeConnectionInfp(giZStackHandle, ZSTACK_SERVER_NAME, ZSTACK_LAYER_NUMBER);
  apicInitializeConnectionInfp(giNwkMgrHandle, NWKMGR_SERVER_NAME, NWKMGR_LAYER_NUMBER);

  // Initialize the App
  init_rc = gwInit();

  if (init_rc == FALSE)
  {
    uiPrintfEx(trUNMASKABLE, "Initialization failed. Quitting now.\n\n");
    return -1;
  }

  return 0;
}

int appInitPhase4( void )
{
  //nothing to do here
  return 0;
}

/*********************************************************************
 * @fn      appMain
 *
 * @brief   the main function for this file.
 *
 * @param   handles - pointer to tcp connection handle(s)
 *
 * @return  exit codes 
 */
int appMain( void )
{
  getUserInput();

  return 0;
}

/**************************************************************************************************
 *
 * @fn          gwInit
 *
 * @brief       HA Gateway Server Initialization
 *
 * @return      none
 *
 **************************************************************************************************/
static bool gwInit( void )
{
  bool rc = TRUE;
  int i;
  int status;
  
  uiPrintf( "gwInit\n" );

  gwAppendTlgPathString();
  
  // Store gateway device's endpoint defaults and read endpoint config file (if available)
  pgSrvEndpointDefs = srvReadEndpointConfigFile( &status );

  uiPrintfEx(trDEBUG,"giLongPollInterval = %d, 0x%X\n", giLongPollInterval, giLongPollInterval);
  uiPrintfEx(trDEBUG,"giShortPollInterval = %d, 0x%X\n", giShortPollInterval, giShortPollInterval);

  if ( (status != SRVEPERR_NONE) && (status != SRVEPERR_NOFILE) )
  {
    uiPrintf( "Endpoint Registration: Failed to Find Endpoint Information\n" );
    return FALSE;
  }

  // Register Endpoint(s) with ZStack
  rc = (rc && gwRegAllEp());
  
  // Register attribute lists
  for ( i = 0; i < pgSrvEndpointDefs->endpointCount; i++ )
  {  
    if ( pgSrvEndpointDefs->ppAttrLists[i] != NULL )
    {
      // Register the application's attribute list
      zcl_registerAttrList( pgSrvEndpointDefs->ppAttrLists[i]->endpoint, 
                            pgSrvEndpointDefs->ppAttrLists[i]->numAttr, 
                            pgSrvEndpointDefs->ppAttrLists[i]->pAttrRecords );
      // Register the aplication's read/write callback
        zcl_registerReadWriteCB( pgSrvEndpointDefs->ppAttrLists[i]->endpoint, 
                               gwZclReadWriteCB, NULL );
    }
  }
  
  // Register all clusters
  zcl_registerPlugin( ZCL_CLUSTER_ID_GEN_BASIC, 0xFFFF,
                      zcl_HdlIncoming );
                      
  //register handler for manufacturer-specific profile-wide commands
  zclMSPWCommandsCb = &gwHandleMSPWCommands;

  // Register for cluster APS options
//  zcl_registerClusterOptionList( GW_EP, 1, gaGwApsOptions );
  
  // Set up the timer for removing transaction entry
  ualarm( TIMER_WAIT_PERIOD, 0 );
  
  // Retrieve local device information
  gwSendSysNwkInfoReadReq();
  
  // GP Attributes Initialization 
  gwInitializeAttributes(); 

  return rc;
}

/************************************************************************************************** 
 * 
 * @fn          gwInitializeAttributes       Initialize attributes with default values 
 * 
 * @return      none 
 * 
 **************************************************************************************************/ 
static void gwInitializeAttributes( void ) 
{ 
  uint16 len; 
  int i; 
  int x;  
  int y; 
  int setAttributeValue = 0; 
  uint8 value[16] = {}; 
  uint8 sharedKey[16] = GP_SHARED_KEY;
  uint8 linkKey[16] = GP_LINK_LEY;
  uint8 functionality[3] = GPP_FUNCTIONALITY;
  uint8 activeFunc[3] = GPP_ACTIVE_FUNCTIONALITY;
  //void *pBuf;
 
  for ( i = 0; i < pgSrvEndpointDefs->endpointCount; i++ ) 
  {   
    if ( pgSrvEndpointDefs->ppAttrLists[i]->endpoint == GP_ENDPOINT ) 
    { 
 	  uiPrintfEx(trDEBUG, "GP ENDPOINT ATTRIBUTES INITIALIZATION");
      for ( x = 0; x < pgSrvEndpointDefs->ppAttrLists[i]->numAttr; x++) 
      { 
        if( pgSrvEndpointDefs->ppAttrLists[i]->pAttrRecords[x].clusterID == GP_CLUSTER_ID ) 
        { 
            switch( pgSrvEndpointDefs->ppAttrLists[i]->pAttrRecords[x].attr.attrId ) 
            { 
              case GP_MAX_PROXY_TABLE_ENTRIES: 
                { 
  	              value[0] = GPP_MAX_PROXY_TABLE_ENTRIES;
  	              setAttributeValue = 1; 
                } 
                break;
              case GP_PROXY_TABLE: 
                { 
				  gp_ProxyTblInit();
                } 
                break; 
              case GP_NOTIFICATION_RETRY_NUMBER: 
                { 
                  value[0] = 0x02; 
                  setAttributeValue = 1; 
                }  
                break; 
               case GP_NOTIFICATION_RETRY_TIMER: 
                { 
                  value[0] = 0x64; 
                  setAttributeValue = 1; 
                } 
                break; 
              case GP_FUNCTIONALITY: 
                { 
                  for ( y=0; y<3; y++) 
                  { 
                    value[y] = functionality[y]; 
                  } 
                  setAttributeValue = 1; 
                } 
                break; 
              case GP_ACTIVE_FUNCTIONALITY: 
                { 
                  for ( y=0; y<3; y++) 
                  { 
                    value[y] = activeFunc[y]; 
                  } 
                  setAttributeValue = 1; 
                }
                break; 
              case GP_SHARED_SECURITY_KEY_TYPE: 
                { 
                  value[0] = GP_SHARED_SEC_KEY_TYPE;
                  setAttributeValue = 1;
                }  
                break; 
              case GP_SHARED_SECURITY_KEY: 
                { 
                  for ( y=0; y<16; y++)
                  {
                    value[y]=sharedKey[y];
                  }
                  setAttributeValue = 1; 
                }
                break; 
              case GP_GPP_LINK_KEY: 
                { 
                  for ( y=0; y<16; y++) 
                  { 
                    value[y]=linkKey[y]; 
                  } 
                  setAttributeValue = 1; 
                }  
                break; 
              case GP_GPP_CLUSTER_REVISION: 
                { 
                  value[1] = HI_UINT16(GP_CLUSTER_REVISION);
                  value[0] = LO_UINT16(GP_CLUSTER_REVISION);
                  setAttributeValue = 1; 
                } 
                break; 
                 
              default: 
                break; 
            } 
   
            if(setAttributeValue) 
            { 
              len = zclGetAttrDataLength( pgSrvEndpointDefs->ppAttrLists[i]->pAttrRecords[x].attr.dataType, value );  
              zcl_memcpy( pgSrvEndpointDefs->ppAttrLists[i]->ppAttrPtrs[x], value, len ); 
              setAttributeValue = 0; 
              for ( y=0; y<16; y++) 
              { 
                value[y] = 0; 
              } 
   
            }
   
        } 
      } 
     
    } 
  } 
     
} 

/**************************************************************************************************
 *
 * @fn          gwAppendTlgPathString
 *
 * @brief       Combines relevant tlg path and filename strings
 *
 * @return      none
 *
 **************************************************************************************************/
static void gwAppendTlgPathString( void )
{
  uint8 len;
  uint8 i;  // source character
  uint8 j = 0;  // destination character
  
  len = strlen( gszConfigTlgPath );
  
  // Remove quotes from path string (e.g. "../test/" to ../test/)
  for ( i = 1; i < (len - 1); i++ )
  {
    gszConfigTlgPath[j++] = gszConfigTlgPath[i];
  }
  
  gszConfigTlgPath[j] = 0;
  
  strcat( gszConfigTlgPath, gszConfigTlgFileName );
}

/**************************************************************************************************
 *
 * @fn          timerHandler
 *
 * @brief       Timer Callback function
 *
 * @param       sig - linux signal information
 *
 * @return      none
 *
 **************************************************************************************************/
static void timerHandler( int sig )
{
  int i;
  
  // Find all transactions marked for receiving multiple responses and remove them
  for ( i = 0; i < giGwServices_TransTableCount; i++ )
  {
    if ( (gpGwServices_TransTable[i].inUse == TRUE) && (gpGwServices_TransTable[i].transTimer) )
    {
      gpGwServices_TransTable[i].transTimer -= TRANSACTION_INTERVAL_TIME;
      
      if ( !(gpGwServices_TransTable[i].transTimer) )
      {
        uiPrintfEx(trINFO, "(GW timer) deleted zclTransId: %d\n", gpGwServices_TransTable[i].zclTransId );

        // Response on timeout
        gwTransTimeoutRsp( &gpGwServices_TransTable[i] );
        
        // Check if entry is expecting multiple device responses
        // or is a self-addressed command
        if ( (gpGwServices_TransTable[i].multRsp == FALSE) && 
             (gpGwServices_TransTable[i].dstAddr != 0) )
        {
          // Add entry in device retry table
          gwDeviceReqRetryHandler( gpGwServices_TransTable[i].dstAddr );
        }
        
        gpGwServices_TransTable[i].inUse = FALSE;
      }
    }
  }

  gp_zcl_send_data();

  if (duplicateFilteringQuarterSeconds >= GP_DUPLICATE_FILTERING_QUARTER_SECONDS)
  {
    gp_expireDuplicateFiltering();
    duplicateFilteringQuarterSeconds = 0;
  }
  else
  {
    duplicateFilteringQuarterSeconds++;
  }

  if(gpProxyAliasConflictTimeout)
  {
    ZdoDeviceAnnounceReq aliasConflictDeviceAnnounceReq = ZDO_DEVICE_ANNOUNCE_REQ__INIT;
    CapabilityInfo capabilityInfo = CAPABILITY_INFO__INIT;

    aliasConflictDeviceAnnounceReq.ieeeaddr = BUILD_UINT64( aliasConflictAnnce.extAddr[0], aliasConflictAnnce.extAddr[1],
                                                            aliasConflictAnnce.extAddr[2], aliasConflictAnnce.extAddr[3],
                                                            aliasConflictAnnce.extAddr[4], aliasConflictAnnce.extAddr[5],
                                                            aliasConflictAnnce.extAddr[6], aliasConflictAnnce.extAddr[7] );

    aliasConflictDeviceAnnounceReq.nwkaddr = aliasConflictAnnce.nwkAddr;
    aliasConflictDeviceAnnounceReq.capabilities = &capabilityInfo;

    gwSendDeviceAnnounce(&aliasConflictDeviceAnnounceReq);
    gpProxyAliasConflictTimeout = false;
  }
  ualarm( TIMER_WAIT_PERIOD, 0 );
}

/*********************************************************************
 * @fn      isEndpointClusterExistOnGw
 *
 * @brief   check whether a given endpoint id is defined on the gateway, and whether it
 *              supports a given clustrer
 *
 *
 * @param  endpoint, cluster
 *
 * @return  true if exist, false otherwise
 */
bool isEndpointClusterExistOnGw(uint8_t endpoint, uint16_t cluster, bool directionServerToClient)
{
  int i, j;
  size_t n_clusters;
  uint32_t * clusters;

  uiPrintfEx(trDEBUG,"isEndpointClusterExistOnGw(%d, 0x%04X, %s)\n", endpoint, cluster, directionServerToClient ? "ServerToClient" : "ClientToServer");

  for ( i = 0; i < pgSrvEndpointDefs->endpointCount; i++ )
  {
    if ((endpoint == AF_BROADCAST_ENDPOINT) || ( endpoint == pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->endpoint ))
    {
      if (directionServerToClient)
      {
        n_clusters = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_outputclusters;
        clusters = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->outputclusters;
      }
      else
      {
        n_clusters = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_inputclusters;
        clusters = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->inputclusters;
      }
      
      for (j = 0; j < n_clusters; j++)
      {
        if (clusters[j] == cluster)
        {
          uiPrintfEx(trDEBUG,"isEndpointClusterExistOnGw(): EP/Cluster Found\n");
          return true;
        }
      }
    }
  }

  if (i == pgSrvEndpointDefs->endpointCount)
  {
    uiPrintfEx(trDEBUG,"EP NOT found\n");
  }
  else
  {
    uiPrintfEx(trDEBUG,"Cluster NOT found on EP\n");
  }
  return false;
}

/**************************************************************************************************
 *
 * @fn          sendGwIdentifyQueryRsp
 *
 * @brief       Function to send data for potential new entry to commissioning engine
 *
 * @param       sendGwIdentifyQueryRsp - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendGwIdentifyQueryRsp( DevProcessIdentifyQueryRspInd *pIdentifyQueryRsp )
{
	int len;
	uint8 * pBuf;

	uiPrintf( "Sending Identify Query Response Data\n" );

	len = dev_process_identify_query_rsp_ind__get_packed_size(pIdentifyQueryRsp);
	
	pBuf = malloc(len);
	if (pBuf)
	{
		dev_process_identify_query_rsp_ind__pack(pIdentifyQueryRsp, pBuf);

		// Send response back to app
		APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_PROCESS_IDENTIFY_QUERY_RSP_IND, len, pBuf );
		free(pBuf);
	}
    else
    {
      uiPrintf( "Sending Attribute Change Indication Failed, memory error\n" );
    }
}

/*********************************************************************
 * @fn      processGwIdentifyQueryRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   nwkAddr - incoming IQRsp source addr
 * @param   endpoint - incomding IQRsp source endpoint
 * @param   ieeeAddr - incomding IQRsp source ieee addr
 * 
 * @return  none
 */
static void processGwIdentifyQueryRspInd( uint16_t nwkAddr, uint8_t endpoint, uint64_t ieeeAddr)
{
  DevProcessIdentifyQueryRspInd gwIdentifyQueryRspInd = DEV_PROCESS_IDENTIFY_QUERY_RSP_IND__INIT;
  
  uiPrintf( "Processing Identify Query Response Indication\n" );

  gwIdentifyQueryRspInd.nwkaddr = nwkAddr;
  gwIdentifyQueryRspInd.endpoint = endpoint;
  gwIdentifyQueryRspInd.ieeeaddr = ieeeAddr;

  sendGwIdentifyQueryRsp( &gwIdentifyQueryRspInd );
  
}

/*********************************************************************
 * @fn      zcl_HdlIncoming
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to a cluster or Profile commands for attributes
 *          that aren't in the attribute list
 *
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zcl_HdlIncoming( zclIncoming_t *pInMsg )
{
  ZStatus_t status = ZFailure;
  bool clusterSupportedByApp;

  clusterSupportedByApp = isEndpointClusterExistOnGw(pInMsg->msg->endPoint, pInMsg->msg->clusterId, pInMsg->hdr.fc.direction);

  if (zcl_ClusterCmd( pInMsg->hdr.fc.type ) // Check if specific to a cluster
    && ( pInMsg->hdr.fc.manuSpecific == 0 ) // Is this a non manufacturer specific command?
    && clusterSupportedByApp) //does the endpoint exist, and is the cluster specified in it at the right direction?
  {
    uiPrintfEx(trDEBUG, "zcl_HdlIncoming: Processing abstracted ZCL command\n");
    // Filter by cluster ID
    if ( (pInMsg->msg->clusterId >= ZCL_CLUSTER_ID_GEN_BASIC) &&
         (pInMsg->msg->clusterId <= ZCL_CLUSTER_ID_GEN_TIME) )
    {
      status = zclGeneral_HdlInSpecificCommands( pInMsg );
    }
    else if ( pInMsg->msg->clusterId == ZCL_CLUSTER_ID_GREEN_POWER )
    {
      status = zclGp_HdlIncoming( pInMsg );
    }
    else if ( pInMsg->msg->clusterId == ZCL_CLUSTER_ID_GEN_POLL_CONTROL )
    {
      status = zclPollControl_HdlIncoming( pInMsg );
    }
    else if ( pInMsg->msg->clusterId == ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK )
    {
      status = zclClosures_HdlIncoming( pInMsg );
    }
    else if ( (pInMsg->msg->clusterId >= ZCL_CLUSTER_ID_SS_IAS_ZONE) &&
              (pInMsg->msg->clusterId <= ZCL_CLUSTER_ID_SS_IAS_ACE) )
    {
      status = zclSS_HdlCommands( pInMsg );
    }
  }

  // Send ZCL Frame Indication to app(s) for unhandled commands
  if (( status == ZFailure ) && (clusterSupportedByApp || enableRawProcessingOfUnsupportedClusters))
  {
    uiPrintfEx(trDEBUG, "zcl_HdlIncoming: Processing generic ZCL command\n");
    GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
    
    // Check if not sent to self
    if ( pInMsg->msg->srcAddr.addr.shortAddr != gLocalDeviceInfo.nwkaddr )
    {
      // Store address information          
      srcAddress.addresstype = GW_ADDRESS_TYPE_T__UNICAST;
      
      srcAddress.has_ieeeaddr = TRUE;

      if ( pInMsg->msg->srcAddr.addrMode == afAddr16Bit )
      {
        bool returnStat;
        returnStat = gwPb_SrvrGetIeeeAddress( pInMsg->msg->srcAddr.addr.shortAddr, &srcAddress.ieeeaddr );
        if (FALSE == returnStat)
        {
          return status;
        }
      }
      else if ( pInMsg->msg->srcAddr.addrMode == afAddr64Bit )
      {
        srcAddress.ieeeaddr = BUILD_UINT64( pInMsg->msg->srcAddr.addr.extAddr[0], pInMsg->msg->srcAddr.addr.extAddr[1], 
                                            pInMsg->msg->srcAddr.addr.extAddr[2], pInMsg->msg->srcAddr.addr.extAddr[3], 
                                            pInMsg->msg->srcAddr.addr.extAddr[4], pInMsg->msg->srcAddr.addr.extAddr[5], 
                                            pInMsg->msg->srcAddr.addr.extAddr[6], pInMsg->msg->srcAddr.addr.extAddr[7] );
      }

	  //If an Identify Query Rsp is received and the F & B timer is active
	  if ( (pInMsg->msg->clusterId == CLUSTER_IDENTIFY) && (pInMsg->hdr.commandID == CMD_IDENTIFY_QUERY_RSP) && GwFindingAndBindingTimerStatus )
	  {
	  	  processGwIdentifyQueryRspInd( pInMsg->msg->srcAddr.addr.shortAddr, pInMsg->msg->srcAddr.endPoint, srcAddress.ieeeaddr);
	  }
	  
      srcAddress.has_endpointid = TRUE;
      srcAddress.endpointid = pInMsg->msg->srcAddr.endPoint;
      srcAddress.has_gwendpointid = TRUE;
      srcAddress.gwendpointid = pInMsg->msg->endPoint;

      status = processGwZclFrameReceiveInd( ALL_APP_CONNECTIONS, &srcAddress, pInMsg );
      
      if (status == ZSTATUS_VALUES__ZSuccess)
      {
        status = ZCL_STATUS_CMD_HAS_RSP; //The application should take care of sending Default Response when required
      }
	  
    }
  }
  
  uiPrintfEx(trDEBUG, "zcl_HdlIncoming: status = %d\n", status);
  return ( status );
}

/**************************************************************************************************
 *
 * @fn          handleAsyncMsgs
 *
 * @brief       Receives all incoming Asynchronous messages from the ZStack Server
 *
 * @param       handle - API client handles
 * @param       subSys - command field subsystem
 * @param       cmdID - incoming message command ID
 * @param       len - length of message
 * @param       pMsg - pointer to message in protobuf packed form
 *
 * @return      none
 *
 **************************************************************************************************/
static void handleAsyncMsgs( apicHandle_t handle, uint8 subSys, uint8 cmdID, uint16 len, uint8 *pMsg )
{
  void *pNativeMsg;
  if ( (pMsg == NULL) || (len == 0xFFFF) )
  {
    // Connection terminated
    return;
  }

  if ( (subSys & RPC_SUBSYSTEM_MASK) == ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF )
  {
    switch ( cmdID )
    {   

	  case ZSTACK_CMD_IDS__ZDO_DEVICE_ANNOUNCE_SHADOW:
	  	{
          pNativeMsg = zdo_device_announce_shadow_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
          	processZdoDeviceAnnounce( (ZdoDeviceAnnounceInd *)pNativeMsg );
            zdo_device_announce_shadow_ind__free_unpacked( pNativeMsg, NULL );
          }
	  	}
        break;
		
      case ZSTACK_CMD_IDS__AF_INCOMING_MSG_IND:
        {
          pNativeMsg = af_incoming_msg_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processAfIncomingMsgInd( (AfIncomingMsgInd *)pNativeMsg );
            af_incoming_msg_ind__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;

      case ZSTACK_CMD_IDS__AF_DATA_CONFIRM_IND:
        {
          pNativeMsg = af_data_confirm_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processAfDataConfirmInd( (AfDataConfirmInd *)pNativeMsg );
            af_data_confirm_ind__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;

   	  case ZSTACK_CMD_IDS__GP_SEC_REQ_INCOMING_MSG:
        {
          pNativeMsg = gp_sec_req_incoming_msg_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processGpSecReqIncomingMsgInd( (GpSecReqIncomingMsgInd *)pNativeMsg );
            gp_sec_req_incoming_msg_ind__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;
        
      case ZSTACK_CMD_IDS__GP_DATA_IND_INCOMING_MSG:
      {
        pNativeMsg = gp_data_ind_incoming_msg_ind__unpack( NULL, len, pMsg );
        if ( pNativeMsg )
        {
          processGpDataIndIncomingMsgInd( (GpDataIndIncomingMsgInd *)pNativeMsg );
          gp_data_ind_incoming_msg_ind__free_unpacked( pNativeMsg, NULL );
        }
      }
      break;

      default:
        uiPrintf( "Undefined asynchronous message received: %d\n\n", cmdID );
        break;
    }
  }
}

/*********************************************************************
 * @fn      gwHandlePbCb
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   subSys - subsystem identifier
 * @param   cmdId - incoming command ID
 * @param   len - length of message data
 * @param   pData - pointer to message data in protobuf packed form
 * @param   type - API callback type
 *
 * @return  none
 */
void gwHandlePbCb( int connection, uint8 subSys, uint8 cmdId, uint16 len, uint8 *pData, uint8 type )
{
  void *pNativeMsg;

  if ( type == SERVER_CONNECT )
  {
    // New Connection
    return;
  }
  else  if ( type == SERVER_DISCONNECT )
  {
    // Connection terminated
    return;
  }

  if ( (subSys & RPC_SUBSYSTEM_MASK) == Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW )
  {
    switch ( cmdId )
    {
      case GW_CMD_ID_T__GW_ADD_GROUP_REQ:
        {
          pNativeMsg = gw_add_group_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwAddGroupReq( connection, (GwAddGroupReq *)pNativeMsg );
            gw_add_group_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_GET_GROUP_MEMBERSHIP_REQ:
        {
          pNativeMsg = gw_get_group_membership_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwGetGroupMembershipReq( connection, (GwGetGroupMembershipReq *)pNativeMsg );
            gw_get_group_membership_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_REMOVE_FROM_GROUP_REQ:
        {
          pNativeMsg = gw_remove_from_group_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwRemoveFromGroupReq( connection, (GwRemoveFromGroupReq *)pNativeMsg );
            gw_remove_from_group_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_STORE_SCENE_REQ:
        {
          pNativeMsg = gw_store_scene_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwStoreSceneReq( connection, (GwStoreSceneReq *)pNativeMsg );
            gw_store_scene_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_REMOVE_SCENE_REQ:
        {
          pNativeMsg = gw_remove_scene_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwRemoveSceneReq( connection, (GwRemoveSceneReq *)pNativeMsg );
            gw_remove_scene_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_RECALL_SCENE_REQ:
        {
          pNativeMsg = gw_recall_scene_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwRecallSceneReq( connection, (GwRecallSceneReq *)pNativeMsg );
            gw_recall_scene_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_GET_SCENE_MEMBERSHIP_REQ:
        {
          pNativeMsg = gw_get_scene_membership_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwGetSceneMembershipReq( connection, (GwGetSceneMembershipReq *)pNativeMsg );
            gw_get_scene_membership_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_SLEEPY_DEVICE_PACKET_PENDING_REQ:
        {
          pNativeMsg = gw_sleepy_device_packet_pending_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwSleepyDevicePacketPendingReq( connection, (GwSleepyDevicePacketPendingReq *)pNativeMsg );
            gw_sleepy_device_packet_pending_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_GET_DEVICE_ATTRIBUTE_LIST_REQ:
        {
          pNativeMsg = gw_get_device_attribute_list_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwGetDeviceAttributeListReq( connection, (GwGetDeviceAttributeListReq *)pNativeMsg );
            gw_get_device_attribute_list_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_READ_DEVICE_ATTRIBUTE_REQ:
        {
          pNativeMsg = gw_read_device_attribute_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwReadDeviceAttributeReq( connection, (GwReadDeviceAttributeReq *)pNativeMsg );
            gw_read_device_attribute_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;

      case GW_CMD_ID_T__GW_WRITE_DEVICE_ATTRIBUTE_REQ:
        {
          pNativeMsg = gw_write_device_attribute_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwWriteDeviceAttributeReq( connection, (GwWriteDeviceAttributeReq *)pNativeMsg );
            gw_write_device_attribute_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;

      case GW_CMD_ID_T__GW_SET_ATTRIBUTE_REPORTING_REQ:
        {
          pNativeMsg = gw_set_attribute_reporting_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwSetAttributeReportingReq( connection, (GwSetAttributeReportingReq *)pNativeMsg );
            gw_set_attribute_reporting_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__GW_SEND_ZCL_FRAME_REQ:
        {
          pNativeMsg = gw_send_zcl_frame_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwSendZclFrameReq( connection, (GwSendZclFrameReq *)pNativeMsg );
            gw_send_zcl_frame_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;

      case GW_CMD_ID_T__GW_ALARM_RESET_REQ:
        {
          pNativeMsg = gw_alarm_reset_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processGwAlarmResetReq( connection, (GwAlarmResetReq *)pNativeMsg );
            gw_alarm_reset_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;

      case GW_CMD_ID_T__DEV_ZONE_ENROLLMENT_RSP:
        {
          pNativeMsg = dev_zone_enrollment_rsp__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevZoneEnrollmentRsp( connection, (DevZoneEnrollmentRsp *)pNativeMsg );
            dev_zone_enrollment_rsp__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_ACE_ARM_RSP:
        {
          pNativeMsg = dev_ace_arm_rsp__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevAceArmRsp( connection, (DevAceArmRsp *)pNativeMsg );
            dev_ace_arm_rsp__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_ACE_GET_ZONE_ID_MAP_RSP:
        {
          pNativeMsg = dev_ace_get_zone_id_map_rsp__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevAceGetZoneIdMapRsp( connection, (DevAceGetZoneIdMapRsp *)pNativeMsg );
            dev_ace_get_zone_id_map_rsp__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_ACE_GET_ZONE_INFORMATION_RSP:
        {
          pNativeMsg = dev_ace_get_zone_information_rsp__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevAceGetZoneInformationRsp( connection, (DevAceGetZoneInformationRsp *)pNativeMsg );
            dev_ace_get_zone_information_rsp__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_SET_IDENTIFY_MODE_REQ:
        {
          pNativeMsg = dev_set_identify_mode_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevSetIdentifyModeReq( connection, (DevSetIdentifyModeReq *)pNativeMsg );
            dev_set_identify_mode_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_SET_ONOFF_STATE_REQ:
        {
          pNativeMsg = dev_set_on_off_state_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevSetOnOffStateReq( connection, (DevSetOnOffStateReq *)pNativeMsg );
            dev_set_on_off_state_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;

	  case GW_CMD_ID_T__DEV_SEND_IDENTIFY_QUERY_REQ:
	  	{
		  	pNativeMsg = dev_send_identify_query_req__unpack( NULL, len, pData );
	        if ( pNativeMsg )
	        {
		        processDevSendIdentifyQueryReq( connection, (DevSendIdentifyQueryReq *)pNativeMsg );
		        dev_send_identify_query_req__free_unpacked( pNativeMsg, NULL ); 
	        }
	  	}
        break;

	  case GW_CMD_ID_T__DEV_SET_FINDING_AND_BINDING_TIMER_STATUS_REQ:
	  	{
		  	pNativeMsg = dev_set_finding_and_binding_timer__unpack( NULL, len, pData );
	        if ( pNativeMsg )
	        {
		        processDevSetFindingAndBindingTimerStatusReq( connection, (DevSetFindingAndBindingTimer *)pNativeMsg );
		        dev_set_finding_and_binding_timer__free_unpacked( pNativeMsg, NULL );
	        }
	  	}
        break;
		
      case GW_CMD_ID_T__DEV_SET_LEVEL_REQ:
        {
          pNativeMsg = dev_set_level_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevSetLevelReq( connection, (DevSetLevelReq *)pNativeMsg );
            dev_set_level_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_GET_LEVEL_REQ:
        {
          pNativeMsg = dev_get_level_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevGetLevelReq( connection, (DevGetLevelReq *)pNativeMsg );
            dev_get_level_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_GET_ONOFF_STATE_REQ:
        {
          pNativeMsg = dev_get_on_off_state_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevGetOnOffStateReq( connection, (DevGetOnOffStateReq *)pNativeMsg );
            dev_get_on_off_state_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_SET_COLOR_REQ:
        {
          pNativeMsg = dev_set_color_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevSetColorReq( connection, (DevSetColorReq *)pNativeMsg );
            dev_set_color_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_SET_COLOR_TEMP_REQ:
        {
          pNativeMsg = dev_set_color_temp_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevSetColorTempReq( connection, (DevSetColorTempReq *)pNativeMsg );
            dev_set_color_temp_req__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;

      case GW_CMD_ID_T__DEV_GET_COLOR_REQ:
        {
          pNativeMsg = dev_get_color_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevGetColorReq( connection, (DevGetColorReq *)pNativeMsg );
            dev_get_color_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_GET_COLOR_TEMP_REQ:
      {
        pNativeMsg = dev_get_color_temp_req__unpack( NULL, len, pData );
        if ( pNativeMsg )
        {
          processDevGetColorTempReq( connection, (DevGetColorTempReq *)pNativeMsg );
          dev_get_color_temp_req__free_unpacked( pNativeMsg, NULL );
        }
      }
      break;

      case GW_CMD_ID_T__DEV_GET_TEMP_REQ:
        {
          pNativeMsg = dev_get_temp_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevGetTempReq( connection, (DevGetTempReq *)pNativeMsg );
            dev_get_temp_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_GET_POWER_REQ:
        {
          pNativeMsg = dev_get_power_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevGetPowerReq( connection, (DevGetPowerReq *)pNativeMsg );
            dev_get_power_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_GET_HUMIDITY_REQ:
        {
          pNativeMsg = dev_get_humidity_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevGetHumidityReq( connection, (DevGetHumidityReq *)pNativeMsg );
            dev_get_humidity_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_SET_DOOR_LOCK_REQ:
        {
          pNativeMsg = dev_set_door_lock_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevSetDoorLockReq( connection, (DevSetDoorLockReq *)pNativeMsg );
            dev_set_door_lock_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_GET_DOOR_LOCK_STATE_REQ:
        {
          pNativeMsg = dev_get_door_lock_state_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevGetDoorLockStateReq( connection, (DevGetDoorLockStateReq *)pNativeMsg );
            dev_get_door_lock_state_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_THERMOSTAT_SETPOINT_CHANGE_REQ:
        {
          pNativeMsg = dev_thermostat_setpoint_change_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevThermostatSetpointChangeReq( connection, (DevThermostatSetpointChangeReq *)pNativeMsg );
            dev_thermostat_setpoint_change_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
        
      case GW_CMD_ID_T__DEV_WINDOW_COVERING_ACTION_REQ:
        {
          pNativeMsg = dev_window_covering_action_req__unpack( NULL, len, pData );
          if ( pNativeMsg )
          {
            processDevWindowCoveringActionReq( connection, (DevWindowCoveringActionReq *)pNativeMsg );
            dev_window_covering_action_req__free_unpacked( pNativeMsg, NULL ); 
          }
        }
        break;
      default:
        uiPrintf( "Gateway Server Received Unsupported App Command: %d\n", cmdId );
        break;
    }
  }
}

/*********************************************************************
 * @fn      gwHandleClientIndPbCb
 *
 * @brief   Handles incoming (from ZStack Server) Cluster Specific 
 *          client-side Indications
 *
 * @param   pInMsg - pointer to incoming ZCL message
 *
 * @return  ZStatusValues
 */
static ZStatusValues gwHandleClientIndPbCb( zclIncoming_t *pInMsg )
{
  GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
  ZStatusValues status;
  
  gwConvertAddrAfToPbRsp( &srcAddress, &(pInMsg->msg->srcAddr) );
  srcAddress.has_gwendpointid = true;
  srcAddress.gwendpointid = pInMsg->msg->endPoint;
  
  // Process Cluster ID
  switch ( pInMsg->msg->clusterId )
  {      
    case ZCL_CLUSTER_ID_SS_IAS_ZONE:
      status = processIasZoneClusterCmdInd( &srcAddress, pInMsg );
      break;
      
    default:
      uiPrintf( "Unsupported Client-Side Cluster Command - ClusterId: %04X, CmdId: %02X, TransId: %d\n", 
                pInMsg->msg->clusterId, pInMsg->hdr.commandID, pInMsg->hdr.transSeqNum );
                
      status = ZFailure;
      break;
  }
  
  return status;
}

/*********************************************************************
 * @fn      gwHandleServerIndPbCb
 *
 * @brief   Handles incoming (from ZStack Server) Cluster Specific 
 *          Server-Side Indications
 *
 * @param   pInMsg - pointer to incoming ZCL message
 *
 * @return  ZStatus_t
 */
static ZStatus_t gwHandleServerIndPbCb( zclIncoming_t *pInMsg )
{
  GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
  ZStatus_t status;
  
  gwConvertAddrAfToPbRsp( &srcAddress, &(pInMsg->msg->srcAddr) );
  srcAddress.has_gwendpointid = true;
  srcAddress.gwendpointid = pInMsg->msg->endPoint;

  
  // Process Cluster ID
  switch ( pInMsg->msg->clusterId )
  {      
    case ZCL_CLUSTER_ID_SS_IAS_ACE:
      status = processIasAceClusterCmdInd( &srcAddress, pInMsg );
      break;
      
    default:
      uiPrintf( "Unsupported Server-Side Cluster Command - ClusterId: %04X, CmdId: %02X, TransId: %d\n", 
                pInMsg->msg->clusterId, pInMsg->hdr.commandID, pInMsg->hdr.transSeqNum );
                
      status = ZFailure;
      break;
  }
  
  return status;
}

/*********************************************************************
 * @fn      gwZclReadWriteCB
 *
 * @brief   Read/write attribute data.
 *
 *          Note: This function is only required when the attribute data
 *                format is unknown to ZCL. This function gets called
 *                when the pointer 'dataPtr' to the attribute value is
 *                NULL in the attribute database registered with the ZCL.
 *
 * @param   clusterId - cluster that attribute belongs to
 * @param   attrId - attribute to be read or written
 * @param   oper - ZCL_OPER_LEN, ZCL_OPER_READ, or ZCL_OPER_WRITE
 * @param   pValue - pointer to attribute value, OTA endian form
 * @param   pLen - length of attribute value read, native endian form
 *
 * @return  status
 */
static ZStatus_t gwZclReadWriteCB( uint16 clusterId, uint16 attrId,
                                   uint8 oper, uint8 *pValue, uint16 *pLen )
{
  uint16 dataLenBuf;
  void *pAttrVariable;
  zclAttrRec_t *pAttrRecord;
  afIncomingMSGPacket_t *pMsgPacket;
  ZStatus_t status;
  uint16 proxyTableIndex;
  int i;
  // Retrieve incoming packet to discover destination endpoint
  pMsgPacket = zcl_getRawAFMsg();

  // Find attribute record and pointer to variable
  pAttrRecord = srvFindAttributeRec( pMsgPacket->endPoint, clusterId, attrId );
  pAttrVariable = srvFindAttributePtr( pMsgPacket->endPoint, clusterId, attrId );
  
  // Check to see if record not found
  if ( !pAttrRecord || !pAttrVariable )
  {
    return ZCL_STATUS_NOT_FOUND;
  }
  
  // Get attribute data type length
  dataLenBuf = zclGetDataTypeLength( pAttrRecord->attr.dataType );
  
  if ( dataLenBuf == 0 )
  {
    // Check if attribute is string and return size
    dataLenBuf = zclGetAttrDataLength( pAttrRecord->attr.dataType, pAttrVariable );
  }
  // Check if length parameter needs to be returned  
  if ( pLen )
  {
    if ( (( attrId == ATTRID_GP_PROXY_TABLE ) && ( clusterId == GP_CLUSTER_ID )) == FALSE )
    {
      *pLen = dataLenBuf; // store the length
    }
  }
  
  switch ( oper )
  {
    case ZCL_OPER_LEN:
      // Length already found, drop out
      if ( attrId == ATTRID_GP_PROXY_TABLE )
      { 
        for ( i = 0; i < GPP_MAX_PROXY_TABLE_ENTRIES ; i++ )
        {
          proxyTableIndex = ZCD_NV_PROXY_TABLE_START + i;
          status = pt_ZclReadGetProxyEntry ( proxyTableIndex, pValue, (uint8*)pLen );
		  if ( *pLen >= 78 )
		  {
			return ZCL_STATUS_INSUFFICIENT_SPACE;
		  }
		  
		  if ( status != ZSuccess )
          {
            status = ZFailure;
            break;
          }
        }
      }
      break;

    case ZCL_OPER_READ:
      uiPrintf( "Received Local Attribute Read Command:" );
      uiPrintf( "EndpointId: %d, ClusterId: %04X, AttrId: %04X\n",
                pMsgPacket->endPoint, clusterId, attrId );
      if ( attrId == ATTRID_GP_PROXY_TABLE )
      { 
        for ( i = 0; i < GPP_MAX_PROXY_TABLE_ENTRIES ; i++ )
        {
          proxyTableIndex = ZCD_NV_PROXY_TABLE_START + i;
          status = pt_ZclReadGetProxyEntry ( proxyTableIndex, pValue, (uint8*)pLen );
		  if ( *pLen >= 78 )
		  {
			return ZCL_STATUS_INSUFFICIENT_SPACE;
		  }
		  
		  if ( status != ZSuccess )
          {
            status = ZFailure;
            break;
          }
        }
      }
	  else
	  {
      	memcpy( pValue, pAttrVariable, dataLenBuf );
	  }
      break;

    case ZCL_OPER_WRITE:
      uiPrintf( "Received Local Attribute Write Command\n" );
      uiPrintf( " EndpointId: %d, ClusterId: %04X, AttrId: %04X, ptr %p\n",
                pMsgPacket->endPoint, clusterId, attrId, pAttrVariable );
      
      memcpy( pAttrVariable, pValue, dataLenBuf );
      
      // Send indication to the app(s) that local attribute has changed
      processGwAttributeChangeInd( pMsgPacket->endPoint, clusterId, attrId, 
                                   pAttrRecord->attr.dataType, pValue, dataLenBuf );
      break;

    default:
       status = ZCL_STATUS_SOFTWARE_FAILURE; // should never get here!
     break;
  }
  
  return status;
}

/*********************************************************************
 * @fn      gwGroupsClusterIndCB
 *
 * @brief   Callback from ZCL to process incoming Groups Cluster Indications 
 * 
 * @param   pRsp - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
ZStatus_t gwGroupsClusterIndCB( zclGroupRsp_t *pRsp, uint8_t gwEndpointId )
{
  GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
  ZStatus_t status = ZSuccess;  // assume success
  uint8 zclTransId;
  
  gwConvertAddrAfToPbRsp( &srcAddress, pRsp->srcAddr );
  srcAddress.has_gwendpointid = true;
  srcAddress.gwendpointid = gwEndpointId;
        
  switch ( pRsp->cmdID )
  {
    case COMMAND_GROUP_ADD_RSP:
      zclTransId = zcl_getParsedTransSeqNum();
      processGenericResponseTriggers( zclTransId, pRsp->status, "Add Group Response" );
      break;
    
    case COMMAND_GROUP_GET_MEMBERSHIP_RSP:
      processGwGetGroupMembershipRspInd( &srcAddress, pRsp );
      break;
    
    case COMMAND_GROUP_REMOVE_RSP:
      zclTransId = zcl_getParsedTransSeqNum();
      processGenericResponseTriggers( zclTransId, pRsp->status, "Remove Group Response" );
      break;
      
    default:
      uiPrintf( "Unsupported Groups Cluster Command, CmdId: %02X\n", pRsp->cmdID );
      status = ZFailure;
      break;
  }
  
  return status;
}

/*********************************************************************
 * @fn      gwScenesClusterIndCB
 *
 * @brief   Callback from ZCL to process incoming Scenes Cluster Indications 
 * 
 * @param   pRsp - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
ZStatus_t gwScenesClusterIndCB( zclSceneRsp_t *pRsp, uint8_t gwEndpointId )
{
  GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
  ZStatus_t status = ZSuccess;  // assume success
  uint8 zclTransId;
  
  gwConvertAddrAfToPbRsp( &srcAddress, pRsp->srcAddr );
  srcAddress.has_gwendpointid = true;
  srcAddress.gwendpointid = gwEndpointId;

  switch ( pRsp->cmdID )
  {
    case COMMAND_SCENE_STORE_RSP:
      zclTransId = zcl_getParsedTransSeqNum();
      processGenericResponseTriggers( zclTransId, pRsp->status, "Store Scene Response" );
      break;
    
    case COMMAND_SCENE_REMOVE_RSP:
      zclTransId = zcl_getParsedTransSeqNum();
      processGenericResponseTriggers( zclTransId, pRsp->status, "Remove Scene Response" );
      break;
      
    case COMMAND_SCENE_GET_MEMBERSHIP_RSP:
      processGwGetSceneMembershipRspInd( &srcAddress, pRsp );
      break;
      
    default:
      uiPrintf( "Unsupported Scenes Cluster Command, CmdId: %02X\n", pRsp->cmdID );
      status = ZFailure;
      break;
  }
  
  return status;
}

/*********************************************************************
 * @fn      gwAlarmsClusterIndCB
 *
 * @brief   Callback from ZCL to process incoming Alarms Cluster Indications 
 * 
 * @param   direction - message direction bit
 * @param   pAlarm - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
ZStatus_t gwAlarmsClusterIndCB( zclAlarm_t *pAlarm, uint8_t gwEndpointId )
{
  GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
  ZStatus_t status;
  
  gwConvertAddrAfToPbRsp( &srcAddress, pAlarm->srcAddr );
  srcAddress.has_gwendpointid = true;
  srcAddress.gwendpointid = gwEndpointId;
  
  switch ( pAlarm->cmdID )
  {
    case COMMAND_ALARMS_ALARM:
      status = processGwAlarmInd( &srcAddress, pAlarm->alarmCode, pAlarm->clusterID );
      break;
      
    default:
      uiPrintf( "Unsupported Alarms Cluster Client Command, CmdId: %02X\n", pAlarm->cmdID );
      
      status = ZFailure;
      break;
  }   
  
  return status;                 
}

/*********************************************************************
 * @fn      gwZclPollControlCheckInCB
 *
 * @brief   Callback from ZCL to process incoming Poll Control 
 *          Check-In commands
 * 
 * @param   pCmd - pointer to the incoming message
 *
 * 
 * Poll Control Check-In State Table Example: 
 *  DB - Database entry found (Y: successful, N: not successful)
 *  PP - Packet pending bit set from app
 *  PCC - Poll Control Server is configured
 *  RSP - Check-In Rsp ((Y: send or N: don't send), fast poll timeout (0s = server uses default))
 * 
 *  (X = don't care)
 *  ========================
 *   DB | PP | PCC | RSP
 *  -----------------------
 *    N | X  |  X  | Y,30s
 *    Y | N  |  N  | Y,30s
 *    Y | N  |  Y  | N,X
 *    Y | Y  |  N  | Y,30s
 *    Y | Y  |  Y  | Y,0s  <- power efficient, normal operating mode
 *  ========================
 *
 * @return  ZStatus_t
 */
ZStatus_t gwZclPollControlCheckInCB( zclPollControlCheckIn_t *pCmd, uint8_t gwEndpointId )
{
  GwSleepyDeviceCheckInInd sleepyDevCheckInInd = GW_SLEEPY_DEVICE_CHECK_IN_IND__INIT;
  GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
  ZStatusValues status;
  
  uiPrintf( "Received Poll Control Check-In CB:" );
  uiPrintf( " NwkAddr: %04X, TransId: %d\n", pCmd->srcAddr->addr.shortAddr, pCmd->seqNum );
  
  // Look up device info in database
  if ( !gwConvertAddrAfToPbRsp( &srcAddress, pCmd->srcAddr ) )
  {
    uiPrintf( " Poll Control Check-In - Device Discovery Not Finished\n" );
    
    zclPollControl_Send_CheckInRsp( gwEndpointId, pCmd->srcAddr,
                                    TRUE, FAST_POLL_TIMEOUT,
                                    TRUE, zcl_TransID );
    
    return ZSTATUS_VALUES__ZSuccess;
  }

  srcAddress.has_gwendpointid = true;
  srcAddress.gwendpointid = gwEndpointId;
  
  // Check if device is in queue, add if necessary
  if ( !gwServices_FindPacketQueueEntry( srcAddress.ieeeaddr ) )
  {
    // Add device to packet pending queue
    if ( !gwServices_AddPacketPendingDevice( srcAddress.ieeeaddr ) )
    {
      uiPrintf( " Poll Control Check-In - Memory Error\n" );
  
      return ZSTATUS_VALUES__ZFailure;
    }
  }

  // Verify Poll Control server has been configured
  if ( !gwServices_PollControlStart( pCmd->srcAddr, &srcAddress ) )
  {
    uiPrintf( " Poll Control Check-In - State Machine Error\n" );
    
    return ZSTATUS_VALUES__ZFailure;
  }

  // See if packet pending for device
  if ( gwServices_FindPacketPendingDeviceEntry( srcAddress.ieeeaddr ) )
  {
    uiPrintf( " Poll Control Check-In - Message Waiting\n" );
    
    // Send check-in response
    if ( ZSuccess == zclPollControl_Send_CheckInRsp( gwEndpointId, pCmd->srcAddr,
                                                     TRUE, 0,
                                                     TRUE, zcl_TransID ) )
    {                                                     
      sleepyDevCheckInInd.srcaddress = &srcAddress; // store address info
      
      // Send message to app(s)
      status = sendGwSleepyDeviceCheckInInd( &sleepyDevCheckInInd );
      
      if ( status == ZSTATUS_VALUES__ZSuccess )
      {
        // clear queue entry
        gwServices_ClearPacketQueueEntry( srcAddress.ieeeaddr );
      }
    }
    else
    {
      status = ZSTATUS_VALUES__ZFailure;
    }
  }
  else
  {
    // Nothing to do, go back to long poll
    status = zclPollControl_Send_CheckInRsp( gwEndpointId, pCmd->srcAddr,
                                             FALSE, 0,
                                             TRUE, zcl_TransID );
  }
  
  return status;
}

/*********************************************************************
 * @fn      gwZclDoorLockRspCB
 *
 * @brief   Callback from ZCL to process incoming Door Lock 
 *          Response commands
 * 
 * @param   pInMsg - pointer to the incoming message
 * @param   status - message status
 *
 * @return  ZStatus_t
 */
ZStatus_t gwZclDoorLockRspCB( zclIncoming_t *pInMsg, uint8 status )
{
  GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
  
  gwConvertAddrAfToPbRsp( &srcAddress, &pInMsg->msg->srcAddr );
  srcAddress.has_gwendpointid = true;
  srcAddress.gwendpointid = pInMsg->msg->endPoint;
  
  if ( (pInMsg->hdr.commandID == COMMAND_CLOSURES_LOCK_DOOR_RSP) ||
       (pInMsg->hdr.commandID == COMMAND_CLOSURES_UNLOCK_DOOR_RSP))
  {
    return processDevSetDoorLockRspInd( &srcAddress, pInMsg->hdr.commandID, pInMsg->hdr.transSeqNum, 
                                        pInMsg->pDataLen, pInMsg->pData );
  }
  else
  {
    uiPrintf( "Unsupported DoorLock Cluster Command, CmdId: %02X, TransId: %d\n", 
              pInMsg->hdr.commandID, pInMsg->hdr.transSeqNum );
              
    return ZFailure;
  }
}

/*********************************************************************
 * @fn      zclSS_HdlCommands
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library

 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclSS_HdlCommands( zclIncoming_t *pInMsg )
{
  ZStatus_t status = ZFailure;
  
  if ( zcl_ClientCmd( pInMsg->hdr.fc.direction ) )
  {  
    status = gwHandleClientIndPbCb( pInMsg );
  }
  else
  {
    status = gwHandleServerIndPbCb( pInMsg );
  }
  
  return status;
}

/*********************************************************************
 * @fn      processIasZoneClusterCmdInd
 *
 * @brief   Handles incoming IAS Zone Cluster Server Command Indications
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   pInMsg - pointer to incoming ZCL message
 *
 * @return  ZStatus_t
 */
static ZStatus_t processIasZoneClusterCmdInd( GwAddressStructT *pSrcAddress, zclIncoming_t *pInMsg )
{
  ZStatus_t status;
  
  switch ( pInMsg->hdr.commandID )
  {
    case COMMAND_SS_IAS_ZONE_STATUS_CHANGE_NOTIFICATION:
      status = processDevZoneStatusChangeInd( pSrcAddress, pInMsg->hdr.transSeqNum, 
                                              pInMsg->pDataLen, pInMsg->pData );
      break;
      
    case COMMAND_SS_IAS_ZONE_STATUS_ENROLL_REQUEST:
      status = processDevZoneEnrollmentReqInd( pSrcAddress, pInMsg->hdr.transSeqNum, 
                                               pInMsg->pDataLen, pInMsg->pData );
      break;
      
    default:
      uiPrintf( "Unsupported IAS Zone Cluster Command, CmdId: %02X, TransId: %d\n", 
                pInMsg->hdr.commandID, pInMsg->hdr.transSeqNum );
      
      status = ZFailure;
      break;
  }
    
  return status;
}

/*********************************************************************
 * @fn      processIasAceClusterCmdInd
 *
 * @brief   Handles incoming IAS ACE Cluster Server Command Indications
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   pInMsg - pointer to incoming ZCL message
 *
 * @return  ZStatus_t
 */
static ZStatus_t processIasAceClusterCmdInd( GwAddressStructT *pSrcAddress, zclIncoming_t *pInMsg )
{
  ZStatus_t status;
  
  switch ( pInMsg->hdr.commandID )
  {
    case COMMAND_SS_IAS_ACE_ARM:
      status = processDevAceArmReqInd( pSrcAddress, pInMsg->hdr.transSeqNum, 
                                       pInMsg->pDataLen, pInMsg->pData );
      break;
      
    case COMMAND_SS_IAS_ACE_BYPASS:
      status = processDevAceBypassInd( pSrcAddress, pInMsg->hdr.transSeqNum, 
                                       pInMsg->pDataLen, pInMsg->pData );
      break;
      
    case COMMAND_SS_IAS_ACE_EMERGENCY:
    case COMMAND_SS_IAS_ACE_FIRE:
    case COMMAND_SS_IAS_ACE_PANIC:
      status = processDevAceEmergencyConditionInd( pSrcAddress, pInMsg->hdr.transSeqNum, 
                                                   pInMsg->hdr.commandID );
      break;
      
    case COMMAND_SS_IAS_ACE_GET_ZONE_ID_MAP:
      status = processDevAceGetZoneIdMapReqInd( pSrcAddress, pInMsg->hdr.transSeqNum, 
                                                pInMsg->pDataLen, pInMsg->pData );
      break;
      
    case COMMAND_SS_IAS_ACE_GET_ZONE_INFORMATION:
      status = processDevAceGetZoneInformationReqInd( pSrcAddress, pInMsg->hdr.transSeqNum, 
                                                      pInMsg->pDataLen, pInMsg->pData );
      break;
      
    default:
      uiPrintf( "Unsupported IAS ACE Cluster Command, CmdId: %02X, TransId: %d\n", 
                pInMsg->hdr.commandID, pInMsg->hdr.transSeqNum );
      
      status = ZFailure;
      break;
  }
    
  return status;
}

/*********************************************************************
 * @fn      zclProcessInCmds
 *
 * @brief   Process incoming ZCL commands specific to this profile

 * @param   pInMsg - pointer to the incoming message
 *
 * @return  none
 */
void zclProcessInCmds( zclIncoming_t *pInMsg )
{
  GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
  gsGwMsgTransTable_t *pTransEntry = NULL;
  
  if ( !pInMsg->hdr.fc.manuSpecific )
  {      
    uiPrintf( "Incoming ZCL Command: CmdId: %d, ClusterId: %04X, TransId: %d\n", 
              pInMsg->hdr.commandID, pInMsg->msg->clusterId, pInMsg->hdr.transSeqNum ); 
                
      // Check if not sent to self
      if ( pInMsg->msg->srcAddr.addr.shortAddr != gLocalDeviceInfo.nwkaddr )
      {
        // Store address information          
        srcAddress.addresstype = GW_ADDRESS_TYPE_T__UNICAST;
        
        srcAddress.has_ieeeaddr = TRUE;
        
        if ( pInMsg->msg->srcAddr.addrMode == afAddr16Bit )
        {
          gwPb_SrvrGetIeeeAddress( pInMsg->msg->srcAddr.addr.shortAddr, &srcAddress.ieeeaddr );
        }
        else if ( pInMsg->msg->srcAddr.addrMode == afAddr64Bit )
        {
          srcAddress.ieeeaddr = BUILD_UINT64( pInMsg->msg->srcAddr.addr.extAddr[0], pInMsg->msg->srcAddr.addr.extAddr[1], 
                                              pInMsg->msg->srcAddr.addr.extAddr[2], pInMsg->msg->srcAddr.addr.extAddr[3], 
                                              pInMsg->msg->srcAddr.addr.extAddr[4], pInMsg->msg->srcAddr.addr.extAddr[5], 
                                              pInMsg->msg->srcAddr.addr.extAddr[6], pInMsg->msg->srcAddr.addr.extAddr[7] );
        }
        
        srcAddress.has_endpointid = TRUE;
        srcAddress.endpointid = pInMsg->msg->srcAddr.endPoint;
		srcAddress.has_gwendpointid = TRUE;
		srcAddress.gwendpointid = pInMsg->msg->endPoint;
      }


/* from the zcl spec, section 2.4 General Command Frames:	"Each cluster (server or client) that implements attributes shall support reception
	of, execution of, and response to all commands to discover, read, and write these
	attributes."

	From this it seems that the attribute related commands can be sent with the direction subfield of the ZCL header set to either 0 or 1 - so we shall not check for the direction bit here. 
	In practice, ZBHT-2 temperature and humidity sensor sends the configure reporting response with the direction subfield set to 0, i.e. client to server...
*/

	  // If response is for a raw zcl request
	  // The application should get the response as raw zcl as well
	  pTransEntry = gwMsgTransGetByZclTransId( pInMsg->hdr.transSeqNum, GW_CMD_ID_T__GW_SEND_ZCL_FRAME_REQ );
	  
	  if ( pTransEntry )
	  {
          processGwZclFrameReceiveInd( pTransEntry->connection, &srcAddress, pInMsg ); 
	  }
      else
      {
	      switch ( pInMsg->hdr.commandID )
	      {
	        case ZCL_CMD_READ_RSP:
	          // Process read attribute response
	          processZclReadAttributeRsp( &srcAddress, pInMsg->hdr.transSeqNum, pInMsg->msg->clusterId, 
	                                      pInMsg->pDataLen, pInMsg->pData );
	          break;
	          
	        case ZCL_CMD_WRITE_RSP:
	          // Process write attribute response
	          processZclWriteAttributeRsp( &srcAddress, pInMsg->hdr.transSeqNum, pInMsg->msg->clusterId, 
	                                       pInMsg->pDataLen, pInMsg->pData );
	          break;
	          
	        case ZCL_CMD_CONFIG_REPORT_RSP:
	          // Process read report configuration response
	          processZclConfigReportRsp( &srcAddress, pInMsg->hdr.transSeqNum, pInMsg->msg->clusterId, 
	                                     pInMsg->pDataLen, pInMsg->pData );
	          break;
	          
	        case ZCL_CMD_DEFAULT_RSP:
	          // Process default response
	          processGenericResponseTriggers( pInMsg->hdr.transSeqNum, *((pInMsg->pData) + 1), "ZCL Default Response" );
	          break;
	          
	        case ZCL_CMD_REPORT:
	          // Process attribute report indication
	          processGwAttributeReportingInd( &srcAddress, pInMsg->msg->clusterId, pInMsg->pDataLen, pInMsg->pData );
	          break;
	          
	        case ZCL_CMD_DISCOVER_ATTRS_RSP:
	          // Process discover attributes response
	          processZclDiscoverAttributesRsp( &srcAddress, pInMsg->hdr.transSeqNum, pInMsg->msg->clusterId, 
	                                           pInMsg->pDataLen, pInMsg->pData );
	          break;                                           
	          
	        default:          
	          // Process ZCL frame for unsupported cmd
	          processGwZclFrameReceiveInd( ALL_APP_CONNECTIONS, &srcAddress, pInMsg ); 
	          break;
	      }
      }
  }
}

/*********************************************************************
 * @fn      processGwAddGroupReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pAddGroupReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwAddGroupReq( int connection, GwAddGroupReq *pAddGroupReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Add Group Request\n" );

  // Send to ZigBee
  status = sendGwAddGroupReq( pAddGroupReq );

  // inform app with a confirm (and perhaps response)  
  gwHandleCnfRsp( GW_RSP_GENERIC, pAddGroupReq->dstaddress->addresstype, 
                  connection, pAddGroupReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__GW_ADD_GROUP_REQ, status );

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Add Group Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwGetGroupMembershipReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetGroupMembershipReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwGetGroupMembershipReq( int connection, GwGetGroupMembershipReq *pGetGroupMembershipReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Get Group Membership Request\n" );

  // Send to ZigBee
  status = sendGwGetGroupMembershipReq( pGetGroupMembershipReq );

  // inform app with a confirm (and perhaps response)  
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetGroupMembershipReq->dstaddress->addresstype, 
                  connection, pGetGroupMembershipReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__GW_GET_GROUP_MEMBERSHIP_REQ, status );

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Get Group Membership Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwRemoveFromGroupReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pRemoveFromGroupReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwRemoveFromGroupReq( int connection, GwRemoveFromGroupReq *pRemoveFromGroupReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Remove From Group Request\n" );

  // send to ZigBee
  status = sendGwRemoveFromGroupReq( pRemoveFromGroupReq );
  
  // inform app with a confirm (and perhaps response)  
  gwHandleCnfRsp( GW_RSP_GENERIC, pRemoveFromGroupReq->dstaddress->addresstype, 
                  connection, pRemoveFromGroupReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__GW_REMOVE_FROM_GROUP_REQ, status );

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Remove From Group Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwStoreSceneReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pStoreSceneReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwStoreSceneReq( int connection, GwStoreSceneReq *pStoreSceneReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Store Scene Request\n" );

  // send to ZigBee (increments Zcl Seq)
  status = sendGwStoreSceneReq( pStoreSceneReq );
  
  // inform app with a confirm (and perhaps response)  
  gwHandleCnfRsp( GW_RSP_GENERIC, pStoreSceneReq->dstaddress->addresstype, 
                  connection, pStoreSceneReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__GW_STORE_SCENE_REQ, status );

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Store Scene Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwRemoveSceneReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pRemoveSceneReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwRemoveSceneReq( int connection, GwRemoveSceneReq *pRemoveSceneReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Remove Scene Request\n" );

  // send to ZigBee
  status = sendGwRemoveSceneReq( pRemoveSceneReq );

  // inform app with a confirm (and perhaps response)  
  gwHandleCnfRsp( GW_RSP_GENERIC, pRemoveSceneReq->dstaddress->addresstype, 
                  connection, pRemoveSceneReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__GW_REMOVE_SCENE_REQ, status );

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Remove Scene Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwRecallSceneReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pRecallSceneReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwRecallSceneReq( int connection, GwRecallSceneReq *pRecallSceneReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Recall Scene Request\n" );

  // sent to ZigBee (increments Zcl Seq)
  status = sendGwRecallSceneReq( pRecallSceneReq );

  // inform app with a confirm (and perhaps response)  
  gwHandleCnfRsp( GW_RSP_GENERIC, pRecallSceneReq->dstaddress->addresstype, 
                  connection, pRecallSceneReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__GW_RECALL_SCENE_REQ, status );

  // debug message  
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Recall Scene Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwGetSceneMembershipReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetSceneMembershipReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwGetSceneMembershipReq( int connection, GwGetSceneMembershipReq *pGetSceneMembershipReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Get Scene Membership Request\n" );

  // Send message to ZCL
  status = sendGetSceneMembershipReq( pGetSceneMembershipReq );

  // inform app with a confirm (and perhaps response)  
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetSceneMembershipReq->dstaddress->addresstype, 
                  connection, pGetSceneMembershipReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__GW_GET_SCENE_MEMBERSHIP_REQ, status );

  // debug message  
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Get Scene Membership Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwSleepyDevicePacketPendingReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSleepyPacketPendingReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwSleepyDevicePacketPendingReq( int connection, GwSleepyDevicePacketPendingReq *pSleepyPacketPendReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Sleepy Device Packet Pending Request\n" );

  // Indicate packet pending in check-in queue
  if ( gwServices_SetPacketQueueEntry( pSleepyPacketPendReq->dstaddress->ieeeaddr ) )
  {
    status = ZSTATUS_VALUES__ZSuccess;
  }
  else
  {
    status = ZSTATUS_VALUES__ZFailure;
  }

  // inform app with a confirm (and perhaps response)  
  gwHandleCnfRsp( GW_RSP_NONE, pSleepyPacketPendReq->dstaddress->addresstype, 
                  connection, pSleepyPacketPendReq->dstaddress->ieeeaddr, GW_CMD_ID_T__GW_SLEEPY_DEVICE_PACKET_PENDING_REQ, status );

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Sleepy Device Packet Pending Request Failed - Status: %d\n", status );
  }
}                                         

/*********************************************************************
 * @fn      processGwAttributeChangeInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   attrEp - endpoint ID of write attribute command
 * @param   clusterId - cluster ID of write attribute command
 * @param   attrId - attribute ID
 * @param   attrDataType - attribute data type
 * @param   pValue - new attribute value
 * @param   len - length of attribute
 * 
 * @return  none
 */
static void processGwAttributeChangeInd( uint8 attrEp, uint16 clusterId, uint16 attrId, 
                                         uint8 attrDataType, uint8 *pValue, uint16 len )
{
  GwAttributeChangeInd attrChangeInd = GW_ATTRIBUTE_CHANGE_IND__INIT;
  
  uiPrintf( "Processing Attribute Change Indication\n" );
  
  attrChangeInd.endpointid = attrEp;
  attrChangeInd.clusterid = clusterId;
  attrChangeInd.attributeid = attrId;
  attrChangeInd.attributetype = attrDataType;
  attrChangeInd.attributevalue.len = (uint8)len;
  
  if ( attrChangeInd.attributevalue.len )
  {
    attrChangeInd.attributevalue.data = malloc ( attrChangeInd.attributevalue.len );
    if ( !attrChangeInd.attributevalue.data )
    {
      return; // memory error
    }
    
    memcpy( attrChangeInd.attributevalue.data, pValue, attrChangeInd.attributevalue.len );
  }

  // Send attribute change indication to the app
  sendGwAttributeChangeInd( &attrChangeInd );
  
  if ( attrChangeInd.attributevalue.len )
  {
    // Free memory
    free( attrChangeInd.attributevalue.data );
  }
}

/*********************************************************************
 * @fn      processGwGetDeviceAttributeListReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetDeviceAttrListReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwGetDeviceAttributeListReq( int connection, GwGetDeviceAttributeListReq *pGetDeviceAttrListReq )
{
  uint8 smStatus; // state machine status
  SrvrDeviceInfoT *pDeviceInfo;
  ZStatusValues status = ZSuccess;
  
  uiPrintf( "Processing Get Device Attribute List Request\n" );
  
  // Start state machine for remote attribute list request
  if ( pGetDeviceAttrListReq->dstaddress->addresstype == GW_ADDRESS_TYPE_T__UNICAST )
  {
    pDeviceInfo = gwPb_SrvrGetDeviceInfoReq( pGetDeviceAttrListReq->dstaddress->ieeeaddr );    
    
    if ( !pDeviceInfo )
    {
      uiPrintf( "Processing Get Device Attribute List Request Failed - No Device Info Found\n" );
    
      status = ZFailure;
    }
    else
    {
      // Start state machine
      smStatus = gwServices_StartStateMachine_DeviceAttrList( connection, pDeviceInfo, pGetDeviceAttrListReq );
    
      if ( smStatus == GW_SERVICES_SUCCESS )
      {
        uiPrintf( "Get Device Attribute List Request Success"
                  "Started Machine for DstAddr: %016llX\n", pGetDeviceAttrListReq->dstaddress->ieeeaddr );       
      }
      else if ( smStatus == GW_SERVICES_BUSY )
      {
        uiPrintf( "Get Device Attribute List Request Failed - Already Running State "
                  "Machine for DstAddr: %016llX\n", pGetDeviceAttrListReq->dstaddress->ieeeaddr );
      
        status = ZFailure;
      }
      else
      {
        uiPrintf( "Get Device Attribute List Request Failed"
                  "DstAddr: %016llX\n", pGetDeviceAttrListReq->dstaddress->ieeeaddr );
                  
        status = ZFailure;
      }
    }
    
    // inform app with a confirm (and perhaps response)  
    gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetDeviceAttrListReq->dstaddress->addresstype, 
                    connection, pGetDeviceAttrListReq->dstaddress->ieeeaddr, 
                    GW_CMD_ID_T__GW_GET_DEVICE_ATTRIBUTE_LIST_REQ, status );  
    
    if ( pDeviceInfo )
    {
      // Free device information structure
      gwPb_FreeSrvrGetDeviceInfo( pDeviceInfo ); 
    }
  }
  // Find relevant local attribute information based off of endpoint ID
  else if ( pGetDeviceAttrListReq->dstaddress->addresstype == GW_ADDRESS_TYPE_T__NONE )
  {
    uint8 clusterCount;
    int attrCount;
    uint32 *pInClusterList = NULL;
    zclAttrRec_t *pAttrRecord = NULL;
    GwClusterListT **ppClusterList = NULL;
    GwGetDeviceAttributeListRspInd attrListRsp = GW_GET_DEVICE_ATTRIBUTE_LIST_RSP_IND__INIT;
    
    attrListRsp.sequencenumber = 0xFFFF;  // for self-addressing
    attrListRsp.status = GW_STATUS_T__STATUS_SUCCESS;   // assume success
    attrListRsp.srcaddress = pGetDeviceAttrListReq->dstaddress;
    
    // Retrieve relevant lists
    pAttrRecord = srvGetServerAttrListOnEndpoint( pGetDeviceAttrListReq->dstaddress->endpointid,
                                                 &attrCount );
    pInClusterList = srvGetInClusterListOnEndpoint( pGetDeviceAttrListReq->dstaddress->endpointid,
                                                   &clusterCount );                                                                                       
    
    if ( !pAttrRecord || !pInClusterList )
    {
      attrListRsp.status = GW_STATUS_T__STATUS_FAILURE;
    }
     
    if ( attrListRsp.status != GW_STATUS_T__STATUS_FAILURE )
    {
      ppClusterList = gwServices_ConvertPbClusterList( attrCount, pAttrRecord, 
                                                       clusterCount, pInClusterList, 
                                                       &attrListRsp.n_clusterlist );
      
      // Verify cluster list was formed correctly
      if ( !attrListRsp.n_clusterlist )
      {
        attrListRsp.status = GW_STATUS_T__STATUS_FAILURE;
      }
      else
      {
        attrListRsp.clusterlist = ppClusterList;  // save allocated cluster list
      }
    }
    
    // Send response back to app
    sendGwGetDeviceAttributeListRspInd( connection, &attrListRsp );
    
    if ( attrListRsp.n_clusterlist )
    {
      // Free allocated cluster list
      gwServices_FreePbClusterList( attrListRsp.n_clusterlist, attrListRsp.clusterlist );
    }
  }
}

/*********************************************************************
 * @fn      processGwReadDeviceAttributeReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pReadDeviceAttrReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwReadDeviceAttributeReq( int connection, GwReadDeviceAttributeReq *pReadDeviceAttrReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Read Device Attribute Request\n" );
  
  // If addressed to self, store transaction entry ahead of time
  if ( pReadDeviceAttrReq->dstaddress->addresstype == GW_ADDRESS_TYPE_T__NONE )
  {
    gwMsgTransPost( FALSE, GW_RSP_SPECIFIC, connection, pReadDeviceAttrReq->dstaddress->ieeeaddr, 
                    GW_CMD_ID_T__GW_READ_DEVICE_ATTRIBUTE_REQ, 0xFFFF, 
                    zcl_TransID, giGwDeviceTimeout );
    
    // send request
    status = sendGwReadDeviceAttributeReq( pReadDeviceAttrReq );
  }
  else
  {
    // send request to ZigBee
    status = sendGwReadDeviceAttributeReq( pReadDeviceAttrReq );
    
    // inform app with a confirm (and perhaps response)  
    gwHandleCnfRsp( GW_RSP_SPECIFIC, pReadDeviceAttrReq->dstaddress->addresstype, 
                    connection, pReadDeviceAttrReq->dstaddress->ieeeaddr, 
                    GW_CMD_ID_T__GW_READ_DEVICE_ATTRIBUTE_REQ, status );
  }

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Read Device Attribute Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwReadDeviceAttributeRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pReadAttrRsp - pointer to received message structure
 * @param   payloadLen - length of received APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processGwReadDeviceAttributeRspInd( int connection, GwReadDeviceAttributeRspInd *pReadAttrRsp,
                                                uint16 payloadLen, uint8 *pPayload )
{  
  int totalAttributeDataSize = 0;
  uint8 *pPayloadBuf;
  uint8 *pOutAttributeData;
  int count = 0;
  int attributeCount = 0;
  uint16 attributeRecordLen;
  uint16 attributeDataSizeBuf;
  uint16 remainingAttributeRecordLen;
  GwAttributeRecordT *pOutAttributeRecord;
  GwAttributeRecordT **ppAttributeRecord;
  
  uiPrintf( "Processing Read Device Attribute Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields
  
  pReadAttrRsp->status = GW_STATUS_T__STATUS_SUCCESS; // assume success
  
  remainingAttributeRecordLen = payloadLen;
  
  pPayloadBuf = pPayload;   // retain starting address of attribute record
  
  // Determine number of attributes
  while ( 0 < remainingAttributeRecordLen )
  {
    // Move payload pointer past attribute ID
    pPayloadBuf += 2;
    
    // Check if attribute read was successful
    if ( *pPayloadBuf == ZSuccess )
    {
      pPayloadBuf++; // move pointer past status field
      
      // Check attribute data type
      attributeDataSizeBuf = zclGetDataTypeLength( *pPayloadBuf );
      
      if ( attributeDataSizeBuf == 0 )
      {
        // Check if attribute is string and return size
        attributeDataSizeBuf = zclGetAttrDataLength( pPayloadBuf[0], &pPayloadBuf[1] );
      }
      
      totalAttributeDataSize += attributeDataSizeBuf;
      
      // Store length of current attribute record:
      // size of attribute ID + status + data type
      attributeRecordLen = (2 + 1 + 1 + attributeDataSizeBuf);
      remainingAttributeRecordLen -= attributeRecordLen;  // decrease remaining attribute list length
      
      pPayloadBuf += attributeDataSizeBuf + 1;  // move pPayload to the next attribute record
      
      attributeCount++; // increase number of attributes in attribute record
    }
    else
    {
      pReadAttrRsp->status = GW_STATUS_T__STATUS_FAILURE; // attribute read not successful
      
      pPayloadBuf++; // move pointer to next attribute record
      
      // Store length of current attribute record:
      // size of attribute ID + status + data type
      attributeRecordLen = (2 + 1);
      remainingAttributeRecordLen -= attributeRecordLen;  // decrease remaining attribute list length
    }
  }
  
  pReadAttrRsp->n_attributerecordlist = attributeCount;
  
  if ( attributeCount == 0 )
  {
    ppAttributeRecord = NULL;
    pOutAttributeRecord = NULL;
    pOutAttributeData = NULL;
  }
  else
  {
    // Allocate all memory needed based off attribute count and size of attribute data
    ppAttributeRecord = malloc( attributeCount * sizeof( GwAttributeRecordT * ) );
    if ( !ppAttributeRecord )
    {
      return;
    }
    
    pOutAttributeRecord = malloc( attributeCount * sizeof( GwAttributeRecordT ) );
    if ( !pOutAttributeRecord )
    {
      free( ppAttributeRecord );
      return;
    }
    
    if ( totalAttributeDataSize )
    {
      pOutAttributeData = malloc( totalAttributeDataSize * sizeof( uint8 ) );  // allocate all memory for attribute data
      if ( !pOutAttributeData )
      {
        free( ppAttributeRecord );
        free( pOutAttributeRecord );
        return;      
      }
    }
    else
    {
      pOutAttributeData = NULL;
    }
  }
  
  if ( attributeCount )
  {
    remainingAttributeRecordLen = payloadLen;
    pPayloadBuf = pPayload;
    
    // Determine number of attributes
    while ( (0 < remainingAttributeRecordLen) && (count <= attributeCount) )
    {
      // Check for valid attribute read
      if ( pPayloadBuf[2] == ZSuccess )
      {
        ppAttributeRecord[count++] = pOutAttributeRecord;
        
        gw_attribute_record_t__init( pOutAttributeRecord );
      
        pOutAttributeRecord->attributeid = BUILD_UINT16( pPayloadBuf[0], pPayloadBuf[1] );
        pPayloadBuf += 3;  // move pointer past attribute ID and status field      
      
        pOutAttributeRecord->attributetype = *pPayloadBuf;
        
        // Check attribute data type
        attributeDataSizeBuf = zclGetDataTypeLength( *pPayloadBuf );
        
        if ( attributeDataSizeBuf == 0 )
        {
          // Check if attribute is string and return size
          attributeDataSizeBuf = zclGetAttrDataLength( pPayloadBuf[0], &pPayloadBuf[1] );
        }
        
        pPayloadBuf++; // move pointer past data type field
      
        pOutAttributeRecord->attributevalue.len = attributeDataSizeBuf;
        
        pOutAttributeRecord->attributevalue.data = pOutAttributeData;

        memcpy( pOutAttributeData, pPayloadBuf, attributeDataSizeBuf );
        
        remainingAttributeRecordLen -= 4 + attributeDataSizeBuf; // attrId + status + data type + attribute size
        pPayloadBuf += attributeDataSizeBuf;  // move payload pointer to next data entry
        pOutAttributeData += attributeDataSizeBuf; // move allocated pointer to attribute data memory
        pOutAttributeRecord++;  // move attribute record pointer to next entry
      }
      else
      {
        remainingAttributeRecordLen -= 3;
        pPayloadBuf += 3; // read error, move on to next attribute ID
      }
    }
  }
  
  pReadAttrRsp->attributerecordlist = ppAttributeRecord;

  // Send response back to the app
  sendGwReadDeviceAttributeRspInd( connection, pReadAttrRsp );
  
  if ( attributeCount )
  {
    // Free head of allocated memory
    if ( pOutAttributeData )
    {
      free( pReadAttrRsp->attributerecordlist[0]->attributevalue.data );
    }
    
    free( ppAttributeRecord[0] );
    free( ppAttributeRecord );
  }
}

/*********************************************************************
 * @fn      processGwWriteDeviceAttributeReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pNwkInfoReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwWriteDeviceAttributeReq( int connection, GwWriteDeviceAttributeReq *pWriteDeviceAttrReq )
{
  ZStatusValues status = ZSTATUS_VALUES__ZFailure;
  
  uiPrintf( "Processing Write Device Attribute Request\n" );
  
  // If addressed to self, store transaction entry ahead of time
  if ( pWriteDeviceAttrReq->dstaddress->addresstype == GW_ADDRESS_TYPE_T__NONE )
  {
    int i;
    uint16 dataLenBuf;
    zclAttrRec_t *pAttrRecord;
    void *pAttrVariable;
    GwWriteDeviceAttributeRspInd writeAttrRsp = GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND__INIT;
    GwAttributeWriteStatusT **ppAttrErrorList;
    GwAttributeWriteStatusT *pAttrErrorList;
    
    writeAttrRsp.srcaddress = pWriteDeviceAttrReq->dstaddress;
    writeAttrRsp.clusterid = pWriteDeviceAttrReq->clusterid;
    writeAttrRsp.sequencenumber = 0xFFFF; // used for self-addressing
    writeAttrRsp.status = GW_STATUS_T__STATUS_SUCCESS;
    
    if ( pWriteDeviceAttrReq->n_attributerecordlist )
    {
      ppAttrErrorList = malloc( sizeof( GwAttributeWriteStatusT * ) * 
                                pWriteDeviceAttrReq->n_attributerecordlist );
      if ( !ppAttrErrorList )
      {
        return;   // memory error
      } 
      
      pAttrErrorList = malloc( sizeof( GwAttributeWriteStatusT ) * 
                                pWriteDeviceAttrReq->n_attributerecordlist );
      if ( !pAttrErrorList )
      {
        free( ppAttrErrorList );
        return;   // memory error
      }          
      
      writeAttrRsp.attributewriteerrorlist = ppAttrErrorList;
      ppAttrErrorList[0] = pAttrErrorList;               

      for ( i = 0; i < pWriteDeviceAttrReq->n_attributerecordlist; i++ )
      {
        // Find attribute record and pointer to variable
        pAttrRecord = srvFindAttributeRec( pWriteDeviceAttrReq->dstaddress->endpointid, 
                                          pWriteDeviceAttrReq->clusterid,
                                          pWriteDeviceAttrReq->attributerecordlist[i]->attributeid );
        pAttrVariable = srvFindAttributePtr( pWriteDeviceAttrReq->dstaddress->endpointid, 
                                            pWriteDeviceAttrReq->clusterid,
                                            pWriteDeviceAttrReq->attributerecordlist[i]->attributeid );
                    
        //  Verify that attribute was found
        if ( pAttrRecord && pAttrVariable )    
        {       
          // Get attribute data type length
          dataLenBuf = zclGetDataTypeLength( pAttrRecord->attr.dataType );
  
          if ( dataLenBuf == 0 )
          {
            // Check if attribute is string and return size
            dataLenBuf = zclGetAttrDataLength( pAttrRecord->attr.dataType, pAttrVariable );
          }
                                              
          // Check attribute data type and appropriate value length
          if ( (pWriteDeviceAttrReq->attributerecordlist[i]->attributetype == pAttrRecord->attr.dataType) &&
               (dataLenBuf >= pWriteDeviceAttrReq->attributerecordlist[i]->attributevalue.len) )
          {
            memcpy( pAttrVariable, 
                    pWriteDeviceAttrReq->attributerecordlist[i]->attributevalue.data,
                    pWriteDeviceAttrReq->attributerecordlist[i]->attributevalue.len );
          }
          else
          {
            ppAttrErrorList[writeAttrRsp.n_attributewriteerrorlist] = pAttrErrorList;
            
            gw_attribute_write_status_t__init( pAttrErrorList );
            
            writeAttrRsp.status = GW_STATUS_T__STATUS_FAILURE;
            pAttrErrorList->status = GW_STATUS_T__STATUS_INVALID_PARAMETER;
            pAttrErrorList->attributeid = pWriteDeviceAttrReq->attributerecordlist[i]->attributeid;
            
            pAttrErrorList++;
            writeAttrRsp.n_attributewriteerrorlist++;
          }
        }
        else
        {
          ppAttrErrorList[writeAttrRsp.n_attributewriteerrorlist] = pAttrErrorList;
              
          gw_attribute_write_status_t__init( pAttrErrorList );
          
          writeAttrRsp.status = GW_STATUS_T__STATUS_FAILURE;
          pAttrErrorList->status = GW_STATUS_T__STATUS_FAILURE;
          pAttrErrorList->attributeid = pWriteDeviceAttrReq->attributerecordlist[i]->attributeid;
          
          pAttrErrorList++;
          writeAttrRsp.n_attributewriteerrorlist++;
        }
      }
    }
    
    // Send response back to app
    sendGwWriteDeviceAttributeRspInd( connection, &writeAttrRsp );
    
    // Free memory
    if ( pWriteDeviceAttrReq->n_attributerecordlist )
    {
      free( ppAttrErrorList[0] );
      free( ppAttrErrorList );
    }
  }
  else
  {
    // Send to ZigBee
    status = sendGwWriteDeviceAttributeReq( pWriteDeviceAttrReq );

    // inform app with a confirm (and perhaps response)  
    gwHandleCnfRsp( GW_RSP_SPECIFIC, pWriteDeviceAttrReq->dstaddress->addresstype, 
                    connection, pWriteDeviceAttrReq->dstaddress->ieeeaddr, 
                    GW_CMD_ID_T__GW_WRITE_DEVICE_ATTRIBUTE_REQ, status );
  }

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Write Device Attribute Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwWriteDeviceAttributeRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pWriteAttrRsp - pointer to received message structure
 * @param   payloadLen - length of received APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processGwWriteDeviceAttributeRspInd( int connection, GwWriteDeviceAttributeRspInd *pWriteAttrRsp,
                                                 uint16 payloadLen, uint8 *pPayload )
{  
  uint8 i;
  uint8 *pPayloadBuf;
  uint16 attributeErrorRecordLen = 3;  // sizeof status + attribute identifier
  uint16 attributeErrorCount = 0;
  uint16 remainingAttributeRecordLen;
  GwAttributeWriteStatusT *pAttrWriteStatus;
  GwAttributeWriteStatusT **ppAttrWriteStatusList;
  
  uiPrintf( "Processing Write Device Attribute Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields
  
  remainingAttributeRecordLen = payloadLen;
  
  pPayloadBuf = pPayload;   // retain starting address of attribute record
  
  // Determine number of attributes
  while ( 0 < remainingAttributeRecordLen )
  {    
    // Check if write attribute was not successful
    if ( pPayloadBuf[0] != ZSuccess )
    {
      attributeErrorCount++; // increase number of attributes in attribute record
      
      pPayloadBuf += attributeErrorRecordLen;  // move pointer to the next attribute record
      
      remainingAttributeRecordLen -= attributeErrorRecordLen;  // decrease remaining attribute list length
    }
    else
    {
      pPayloadBuf++; // move pointer to the next attribute record
      
      remainingAttributeRecordLen--;  // decrease remaining attribute list length
    }
  }
  
  pWriteAttrRsp->n_attributewriteerrorlist = attributeErrorCount;
  
  if ( !attributeErrorCount )
  {
    pAttrWriteStatus = NULL;
  }
  else
  {
    ppAttrWriteStatusList = malloc( attributeErrorCount * sizeof( GwAttributeWriteStatusT * ) );
    if ( !ppAttrWriteStatusList )
    {
      return; // memory error
    }
    
    pAttrWriteStatus = malloc( attributeErrorCount * sizeof( GwAttributeWriteStatusT ) );
    if ( !pAttrWriteStatus )
    {
      free( ppAttrWriteStatusList );
      
      return; // memory error
    }
  }
  
  if ( attributeErrorCount )
  {
    pWriteAttrRsp->status = GW_STATUS_T__STATUS_FAILURE;
    
    pWriteAttrRsp->attributewriteerrorlist = ppAttrWriteStatusList;
    
    for ( i = 0; i < attributeErrorCount; i++ )
    {
      ppAttrWriteStatusList[i] = pAttrWriteStatus;
      
      gw_attribute_write_status_t__init( pAttrWriteStatus );
      
      pAttrWriteStatus->status = pPayload[0];
      pAttrWriteStatus->attributeid = BUILD_UINT16( pPayload[1], pPayload[2] );
      
      pPayload += attributeErrorRecordLen;  // move payload pointer to next attribute record   
      pAttrWriteStatus++;  
    }
  }
  
  pWriteAttrRsp->status = GW_STATUS_T__STATUS_SUCCESS;
  
  // Send response back to the app
  sendGwWriteDeviceAttributeRspInd( connection, pWriteAttrRsp );
  
  if ( attributeErrorCount )
  {
    // Free memory
    free( ppAttrWriteStatusList[0] );
    free( ppAttrWriteStatusList );
  }
}

/*********************************************************************
 * @fn      processGwSetAttributeReportingReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetAttrReportReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwSetAttributeReportingReq( int connection, GwSetAttributeReportingReq *pSetAttrReportReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Set Attribute Reporting Request\n" );

  // send packet to ZigBee
  status = sendGwSetAttributeReportingReq( pSetAttrReportReq );

  // inform app with a confirm (and perhaps response)  
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pSetAttrReportReq->dstaddress->addresstype, 
                  connection, pSetAttrReportReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__GW_SET_ATTRIBUTE_REPORTING_REQ, status );

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Set Attribute Reporting Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwSetAttributeReportingRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetAttrReportingRsp - pointer to received message structure
 * @param   payloadLen - length of received APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processGwSetAttributeReportingRspInd( int connection, 
        GwSetAttributeReportingRspInd *pSetAttrReportingRsp,
        uint16 payloadLen, uint8 *pPayload )
{
  uint8 rspStatus = GW_STATUS_T__STATUS_SUCCESS;
  uint8 badAttrRecordLen = (1 + 1 + 2); // status, direction, and 
                                        // attribute ID bytes
  uint8 *pPayloadHead;
  int count;
  int attrRecordNum = 0;
  int remainingLen;
  GwAttributeReportConfigT **ppAttrReportConfig = NULL;
  GwAttributeReportConfigT *pAttrReportConfig = NULL;

  uiPrintf( "Processing Set Attribute Reporting Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields

  pPayloadHead = pPayload;

  remainingLen = payloadLen;

  // Calculate number of attribute records
  while ( remainingLen )
  {
    if ( *pPayload == ZSuccess )
    {      
      pPayload++;
      remainingLen--;
    }
    else
    {

      if (remainingLen < badAttrRecordLen)
      {
        // Error scenario, badly formed packet length 
        rspStatus = GW_STATUS_T__STATUS_FAILURE;
        //Ignore the badly formed packet length
        payloadLen -= remainingLen;
        break;
      }
   
      attrRecordNum++;
      
      pPayload += badAttrRecordLen;
      remainingLen -= badAttrRecordLen;
    }
  }
  
  
  if ( attrRecordNum )
  {
    ppAttrReportConfig = malloc( attrRecordNum * 
        sizeof(GwAttributeReportConfigT *) );

    if ( !ppAttrReportConfig )
    {
      return; // memory error  
    }

    pAttrReportConfig = malloc( attrRecordNum *
        sizeof( GwAttributeReportConfigT ) );

    if ( !pAttrReportConfig )
    {
      free( ppAttrReportConfig );

      return; // memory error
    }
 
    rspStatus = GW_STATUS_T__STATUS_FAILURE;
 
    pPayload = pPayloadHead;  // reset payload pointer
    remainingLen = payloadLen;// and length
    count = 0;

    while ( remainingLen )
    {
      // Check status byte
      if ( *pPayload != ZSuccess )
      {
        // Initialize pointer
        gw_attribute_report_config_t__init( pAttrReportConfig );

        ppAttrReportConfig[count++] = pAttrReportConfig;

        pAttrReportConfig->status = *pPayload++;

        pPayload++; // ignore direction field

        pAttrReportConfig->attributeid = BUILD_UINT16( pPayload[0], 
                pPayload[1] );
        pPayload += 2;
        
        pAttrReportConfig++;
        remainingLen -= badAttrRecordLen;
      }
      else
      {
        pPayload++;
        remainingLen--;
      }
    }
  }

  pSetAttrReportingRsp->status = rspStatus;
  pSetAttrReportingRsp->n_attributereportconfiglist = attrRecordNum;
  pSetAttrReportingRsp->attributereportconfiglist = ppAttrReportConfig;  

  // Send response back to the app
  sendGwSetAttributeReportingRspInd( connection, pSetAttrReportingRsp );

  // Free memory
  if ( attrRecordNum )
  {
    free( ppAttrReportConfig[0] );
    free( ppAttrReportConfig );
  }                                          
}

/*********************************************************************
 * @fn      processGwAttributeReportingInd
 *
 * @brief   Handles incoming ZCL Attribute Reporting messages.
 *
 * @param   pSrcAddress - pointer to source address
 * @param   clusterId - cluster ID of read attribute response
 * @param   payloadLen - length of received ZCL payload
 * @param   pPayload - pointer to received ZCL payload
 *
 * @return  none
 */
static void processGwAttributeReportingInd( GwAddressStructT *pSrcAddress, uint16 clusterId, uint16 payloadLen, uint8 *pPayload )
{
  int totalAttrDataSize = 0;
  uint8 i;
  uint8 *pPayloadHead;
  uint8 *pOutAttrData;
  uint16 attrCount = 0;
  uint16 attrRecordLen;
  uint16 attrDataSizeBuf;
  uint16 remainingAttrRecordLen;
  GwAttributeReportingInd attrReportInd = GW_ATTRIBUTE_REPORTING_IND__INIT;
  GwAttributeRecordT **ppAttrRecord;
  GwAttributeRecordT *pAttrRecord;
  
  uiPrintf( "Processing Attribute Report Indication\n" );
  
  attrReportInd.status = GW_STATUS_T__STATUS_SUCCESS;
  attrReportInd.srcaddress = pSrcAddress;
  attrReportInd.clusterid = clusterId;
  
  remainingAttrRecordLen = payloadLen;
  
  pPayloadHead = pPayload;   // retain starting address of attribute record
  
  // Determine number of attributes
  while ( 0 < remainingAttrRecordLen )
  {
    // Move payload pointer past attribute ID
    pPayload += 2;
    
    // Check attribute data type
    attrDataSizeBuf = zclGetDataTypeLength( *pPayload );
    
    if ( attrDataSizeBuf == 0 )
    {
      // Check if attribute is string and return size
      attrDataSizeBuf = zclGetAttrDataLength( pPayload[0], &pPayload[1] );
    }
    
    totalAttrDataSize += attrDataSizeBuf;
    
    // Store length of current attribute record:
    // size of attribute ID + data type
    attrRecordLen = (2 + 1 + attrDataSizeBuf);
    remainingAttrRecordLen -= attrRecordLen;  // decrease remaining attribute list length
    
    pPayload += attrDataSizeBuf + 1;  // move pPayload to the next attribute record
    
    attrCount++; // increase number of attributes in attribute record
  }
  
  attrReportInd.n_attributerecordlist = attrCount;  // store attribute response count
  
  pPayload = pPayloadHead;   // reset pPayload to head of attribute record
  
  if ( attrCount )
  {
    // Allocate all memory needed based off attribute count and size of attribute data
    ppAttrRecord = malloc( attrCount * sizeof( GwAttributeRecordT * ) );
    if ( !ppAttrRecord )
    {
      return;
    }
    
    pAttrRecord = malloc( attrCount * sizeof( GwAttributeRecordT ) );
    if ( !pAttrRecord )
    {
      free( ppAttrRecord );
      return;
    }
    
    if ( totalAttrDataSize )
    {
      pOutAttrData = malloc( totalAttrDataSize );  // allocate all memory for attribute data
      if ( !pOutAttrData )
      {
        free( ppAttrRecord );
        free( pAttrRecord );
        return;      
      }
    }
    else
    {
      pOutAttrData = NULL;
    }
  }
  
  if ( attrCount )
  {
    for ( i = 0; i < attrCount; i++ )
    {
      ppAttrRecord[i] = pAttrRecord;
      
      gw_attribute_record_t__init( pAttrRecord );
      
      pAttrRecord->attributeid = BUILD_UINT16( pPayload[0], pPayload[1] );
      pPayload += 2;  // move pointer past attribute ID    
    
      pAttrRecord->attributetype = *pPayload;
      
      // Check attribute data type
      attrDataSizeBuf = zclGetDataTypeLength( *pPayload );
      
      if ( attrDataSizeBuf == 0 )
      {
        // Check if attribute is string and return size
        attrDataSizeBuf = zclGetAttrDataLength( pPayload[0], &pPayload[1] );
      }
      
      pPayload++; // move pointer past data type field
    
      pAttrRecord->attributevalue.len = attrDataSizeBuf;
      
      pAttrRecord->attributevalue.data = pOutAttrData;

      memcpy( pOutAttrData, pPayload, attrDataSizeBuf );
      
      pPayload += attrDataSizeBuf;  // move payload pointer to next data entry
      pOutAttrData += attrDataSizeBuf; // move allocated pointer to attribute data memory
      pAttrRecord++;  // move attribute record pointer to next entry
    }
  }
  
  attrReportInd.attributerecordlist = ppAttrRecord;
  
  // Send response back to the app
  sendGwAttributeReportingInd( &attrReportInd );
  
  if ( attrCount )
  {
    // Free head of allocated memory
    if ( pOutAttrData )
    {
      free( attrReportInd.attributerecordlist[0]->attributevalue.data );
    }
    
    free( ppAttrRecord[0] );
    free( ppAttrRecord );
  }
}

/*********************************************************************
 * @fn      processGwSendZclFrameReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSendZclFrameReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwSendZclFrameReq( int connection, GwSendZclFrameReq *pSendZclFrameReq )
{
  uint8 rspType = GW_RSP_SPECIFIC;
  ZStatus_t status;
  
  uiPrintf( "Processing Send ZCL Frame Request\n" );

  // send packet to ZigBee
  status = sendGwSendZclFrameReq( pSendZclFrameReq );

  
  if ( pSendZclFrameReq->has_sequencenumber )
  {
    rspType = GW_RSP_NONE;
  }
  
  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( rspType, pSendZclFrameReq->dstaddress->addresstype, 
                  connection, pSendZclFrameReq->dstaddress->ieeeaddr, GW_CMD_ID_T__GW_SEND_ZCL_FRAME_REQ, status );
  
  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Send ZCL Frame Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processGwZclFrameReceiveInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - pointer to source address information in native protobuf
 * @param   pInMsg - pointer to received ZCL message
 *
 * @return  ZStatusValues
 */
static ZStatusValues processGwZclFrameReceiveInd( int connection, GwAddressStructT *pSrcAddress, zclIncoming_t *pInMsg )
{
  uint16 profileId = 0;
  SrvrDeviceInfoT *pDeviceInfo;
  GwZclFrameReceiveInd zclFrameInd = GW_ZCL_FRAME_RECEIVE_IND__INIT;
  ZStatusValues status = ZFailure;
  gsGwMsgTransTable_t *pTransEntry = NULL;
  
  uiPrintf( "Processing ZCL Frame Receive Indication\n" );

  // Look up device in database to find Profile ID
  pDeviceInfo = gwPb_SrvrGetDeviceInfoReq( pSrcAddress->ieeeaddr );
  if ( pDeviceInfo )
  {
    uint8 i;

    for ( i = 0; i < pDeviceInfo->n_simpledesclist; i++ )
    {
      // Match endpoint with endpoint information in database
      if ( pDeviceInfo->simpledesclist[i]->endpointid == pSrcAddress->endpointid )
      {
        profileId = pDeviceInfo->simpledesclist[i]->profileid;  // store Profile ID

        break;
      }
    }
    
    gwPb_FreeSrvrGetDeviceInfo( pDeviceInfo );
  }
  else
  { 
    return status;
  }

  // If this incoming frame is a response to a transaction initiated by us, use the same sequence number
  pTransEntry = gwMsgTransGetByZclTransId( pInMsg->hdr.transSeqNum, GW_CMD_ID_T__GW_SEND_ZCL_FRAME_REQ );

  if ( pTransEntry )
  {
    zclFrameInd.sequencenumber = pTransEntry->appTransId;
  }
  else
  {
  	zclFrameInd.sequencenumber = gGwAppTransSeqNum;
  }
  
  zclFrameInd.srcaddress = pSrcAddress;
  zclFrameInd.profileid = profileId;
  zclFrameInd.endpointiddest = pInMsg->msg->endPoint;
  zclFrameInd.clusterid = pInMsg->msg->clusterId;
  zclFrameInd.frametype = pInMsg->hdr.fc.type;
  zclFrameInd.manufacturerspecificflag = pInMsg->hdr.fc.manuSpecific;

  if ( pInMsg->hdr.fc.manuSpecific )
  {
    zclFrameInd.has_manufacturercode = TRUE;
    zclFrameInd.manufacturercode = pInMsg->hdr.manuCode;
  }
  
  zclFrameInd.clientserverdirection = pInMsg->hdr.fc.direction;
  zclFrameInd.disabledefaultrsp = pInMsg->hdr.fc.disableDefaultRsp;
  zclFrameInd.commandid = pInMsg->hdr.commandID;
  zclFrameInd.payload.len = pInMsg->pDataLen;
  
  zclFrameInd.payload.data = malloc( pInMsg->pDataLen );
  if ( !zclFrameInd.payload.data )
  {
    uiPrintf( "Processing ZCL Frame Receive Indication Failed - Memory Error\n" );
    
    return ZSTATUS_VALUES__ZMemError;
  }
  
  // Store payload data
  memcpy( zclFrameInd.payload.data, pInMsg->pData, pInMsg->pDataLen );

  // Send packet to app(s)
  status = sendGwZclFrameReceiveInd( connection, &zclFrameInd );

  if (( status == ZSTATUS_VALUES__ZSuccess ) && ( pTransEntry == NULL ))
  {
    // Post transaction
    gwMsgTransPost( FALSE, GW_RSP_NONE, ALL_APP_CONNECTIONS, 0, GW_CMD_ID_T__GW_ZCL_FRAME_RECEIVE_IND, 
                    gGwAppTransSeqNum, pInMsg->hdr.transSeqNum, APP_TRANSACTION_TIMEOUT );
                    
    gwIncreaseCurrentAppSeqNum();                    
  }
  else
  {
  	pTransEntry->transTimer = APP_TRANSACTION_TIMEOUT;
  }
  
  // Free memory
  free( zclFrameInd.payload.data );
  
  return status;
}

/*********************************************************************
 * @fn      processGwGetGroupMembershipRspInd
 *
 * @brief   Handles incoming Get Group Membership Response Indication.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   pGroupRsp - pointer to command data structure
 *
 * @return  none
 */
static void processGwGetGroupMembershipRspInd( GwAddressStructT *pSrcAddress, zclGroupRsp_t *pGroupRsp )
{
  uint8 zclTransId;
  int i;
  gsGwMsgTransTable_t *pTransEntry;
  GwGetGroupMembershipRspInd getGroupMbrRsp = GW_GET_GROUP_MEMBERSHIP_RSP_IND__INIT;
  
  uiPrintf( "Processing Get Group Membership Response Indication\n" );
  
  // Use temporary workaround to find ZCL transID for CB funcs
  zclTransId = zcl_getParsedTransSeqNum();
  
  // Lookup transaction entry
  pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__GW_GET_GROUP_MEMBERSHIP_REQ );
  
  if ( !pTransEntry )
  {
    uiPrintf( "Processing Get Group Membership Response Indication Failed - No Valid Transaction Table Entry Found\n" );
    
    return;
  }
  
  // Determine if response count needs to be handled
  gwRspCountHandler( pTransEntry );
  
  getGroupMbrRsp.sequencenumber = pTransEntry->appTransId;
  getGroupMbrRsp.srcaddress = pSrcAddress;
  
  // Fill in payload information
  if ( pGroupRsp->status == ZCL_STATUS_SUCCESS )
  {
    getGroupMbrRsp.status = GW_STATUS_T__STATUS_SUCCESS;
    getGroupMbrRsp.capacity = pGroupRsp->capacity;
    getGroupMbrRsp.n_grouplist = pGroupRsp->grpCnt; 
    
    if ( getGroupMbrRsp.n_grouplist )
    {
      getGroupMbrRsp.grouplist = malloc( sizeof( uint32 ) * pGroupRsp->grpCnt );  
      if ( !getGroupMbrRsp.grouplist )
      {
        return; // memory error
      }
      
      for ( i = 0; i < getGroupMbrRsp.n_grouplist; i++ )
      {
        getGroupMbrRsp.grouplist[i] = pGroupRsp->grpList[i];
      }
    }
  }
  else
  {
    getGroupMbrRsp.status = GW_STATUS_T__STATUS_FAILURE;
    
    getGroupMbrRsp.grouplist = NULL;
  }
  
  // Send message to the app
  sendGwGetGroupMembershipRspInd( pTransEntry->connection, &getGroupMbrRsp );
  
  if ( getGroupMbrRsp.grouplist )
  {
    free( getGroupMbrRsp.grouplist );
  }
  
  // Handle the transaction entry
  gwMsgRemoveTrans( pTransEntry->appTransId );
}


/*********************************************************************
 * @fn      processGwGetSceneMembershipRspInd
 *
 * @brief   Handles incoming Get Scene Membership Response Indication.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   pSceneRsp - pointer to command data structure
 *
 * @return  none
 */
static void processGwGetSceneMembershipRspInd( GwAddressStructT *pSrcAddress, zclSceneRsp_t *pSceneRsp )
{
  uint8 zclTransId;
  gsGwMsgTransTable_t *pTransEntry;
  GwGetSceneMembershipRspInd getSceneMbrRsp = GW_GET_SCENE_MEMBERSHIP_RSP_IND__INIT;
  
  uiPrintf( "Processing Get Scene Membership Response Indication\n" );
  
  // Use temporary workaround to find ZCL transID for CB funcs
  zclTransId = zcl_getParsedTransSeqNum();
  
  // Lookup transaction entry
  pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__GW_GET_SCENE_MEMBERSHIP_REQ );

  if ( !pTransEntry )
  {
    uiPrintf( "Processing Get Scene Membership Response Indication Failed - No Valid Transaction Table Entry Found\n" );
    
    return;
  }
  
  // Determine if response count needs to be handled
  gwRspCountHandler( pTransEntry );
  
  getSceneMbrRsp.sequencenumber = pTransEntry->appTransId;
  getSceneMbrRsp.srcaddress = pSrcAddress;
  
  // Fill in payload information
  if ( pSceneRsp->status == ZCL_STATUS_SUCCESS )
  {
    getSceneMbrRsp.status = GW_STATUS_T__STATUS_SUCCESS;  
    getSceneMbrRsp.capacity = pSceneRsp->capacity;
    getSceneMbrRsp.groupid = pSceneRsp->scene->groupID;
    getSceneMbrRsp.scenelist.len = pSceneRsp->sceneCnt;
    
    if ( getSceneMbrRsp.scenelist.len )
    {
      getSceneMbrRsp.scenelist.data = malloc( getSceneMbrRsp.scenelist.len );
      if ( !getSceneMbrRsp.scenelist.data )
      {
        return; // memory error
      }
      
      memcpy( getSceneMbrRsp.scenelist.data, pSceneRsp->sceneList, getSceneMbrRsp.scenelist.len );
    }
  }
  else
  {
    getSceneMbrRsp.status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  // Send message to the app
  sendGwGetSceneMembershipRspInd( pTransEntry->connection, &getSceneMbrRsp );
  
  if ( getSceneMbrRsp.scenelist.len )
  {
    // Free memory
    free( getSceneMbrRsp.scenelist.data );
  }
  
  // Handle the transaction entry
  gwMsgRemoveTrans( pTransEntry->appTransId );
}

/*********************************************************************
 * @fn      processGwAlarmInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   alarmCode - alarm code
 * @param   clusterId - cluster ID of generated alarm
 *
 * @return  ZStatus_t
 */
static ZStatus_t processGwAlarmInd( GwAddressStructT *pSrcAddress, uint8 alarmCode, uint16 clusterId )
{  
  GwAlarmInd alarmInd = GW_ALARM_IND__INIT;
  
  uiPrintf( "Processing Alarm Indication\n" );
  
  alarmInd.srcaddress = pSrcAddress;

  alarmInd.alarmcode = alarmCode;
  alarmInd.clusterid = clusterId;
  
  // Send indication to app(s)
  return sendGwAlarmInd( &alarmInd );
}

/*********************************************************************
 * @fn      processGwAlarmResetReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pAlarmResetReq - pointer to received message structure
 *
 * @return  none
 */
static void processGwAlarmResetReq( int connection, GwAlarmResetReq *pAlarmResetReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Alarm Reset Request\n" );

  // send to ZigBee the Alarm Reset Req
  status = sendGwAlarmResetReq( pAlarmResetReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_GENERIC, pAlarmResetReq->dstaddress->addresstype, 
                  connection, pAlarmResetReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__GW_ALARM_RESET_REQ, status );

  // debug message
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Alarm Reset Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevZoneStatusChangeInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   transSeqNum - ZCL transaction ID
 * @param   len - length of ZCL payload
 * @param   pData - pointer to ZCL payload data
 *
 * @return  ZStatus_t
 */
static ZStatus_t processDevZoneStatusChangeInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData )
{
  ZStatus_t status;
  DevZoneStatusChangeInd zoneStatusChange = DEV_ZONE_STATUS_CHANGE_IND__INIT;
  
  uiPrintf( "Processing IAS Zone Status Change Indication, TransId: %d\n", transSeqNum );
  
  // Store sequence number and source address information
  zoneStatusChange.srcaddress = pSrcAddress;
  
  // Store payload data
  zoneStatusChange.zonestatus = BUILD_UINT16( pData[0], pData[1] );
  pData += 2;
  zoneStatusChange.extendedstatus = *pData;
  
  // Send command to the app(s)
  status = sendDevZoneStatusChangeInd( &zoneStatusChange );
  
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing IAS Zone Status Change Indication Failure, status: %d\n", status );
  }
  
  return status;
}

/*********************************************************************
 * @fn      processDevZoneEnrollmentReqInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   transSeqNum - ZCL transaction ID
 * @param   len - length of ZCL payload
 * @param   pData - pointer to ZCL payload data
 *
 * @return  ZStatus_t
 */
static ZStatus_t processDevZoneEnrollmentReqInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData )
{
  ZStatus_t status;
  DevZoneEnrollmentReqInd zoneEnrollReq = DEV_ZONE_ENROLLMENT_REQ_IND__INIT;
  
  uiPrintf( "Processing IAS Zone Enrollment Request Indication, TransId: %d\n", transSeqNum );
  
  // Store sequence number and source address information
  zoneEnrollReq.sequencenumber = gGwAppTransSeqNum;
  zoneEnrollReq.srcaddress = pSrcAddress;
  
  // Store payload data
  zoneEnrollReq.zonetype = BUILD_UINT16( pData[0], pData[1] );
  pData += 2;
  zoneEnrollReq.manufacturercode = BUILD_UINT16( pData[0], pData[1] );
  
  // Send command to the app(s)
  status = sendDevZoneEnrollmentReqInd( &zoneEnrollReq );
  
  if ( status == ZSuccess )
  {
    // Post transaction entry
    gwMsgTransPost( FALSE, GW_RSP_SPECIFIC, ALL_APP_CONNECTIONS, pSrcAddress->ieeeaddr, GW_CMD_ID_T__DEV_ZONE_ENROLLMENT_REQ_IND, 
                    gGwAppTransSeqNum, transSeqNum, APP_TRANSACTION_TIMEOUT );
                    
    gwIncreaseCurrentAppSeqNum();                    
  }
  else
  {
    uiPrintf( "Processing IAS Zone Enrollment Request Indication Failure, status: %d\n", status );
  }
  
  return status;
}

/*********************************************************************
 * @fn      processDevZoneEnrollmentRsp
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pZoneEnrollRsp - pointer to received message structure
 *
 * @return  none
 */
static void processDevZoneEnrollmentRsp( int connection, DevZoneEnrollmentRsp *pZoneEnrollRsp )
{
  gsGwMsgTransTable_t *pTransEntry;
  GwZigbeeGenericCnf genericCnf = GW_ZIGBEE_GENERIC_CNF__INIT;
  ZStatus_t status = ZFailure;
  
  uiPrintf( "Processing ACE Zone Enrollment Response, appTransId: %d\n", pZoneEnrollRsp->sequencenumber );

  // Lookup transaction entry
  pTransEntry = gwMsgTransGetBySrcAddrCmdId( pZoneEnrollRsp->dstaddress->ieeeaddr, GW_CMD_ID_T__DEV_ZONE_ENROLLMENT_REQ_IND );
      
  // Verify transaction table entry
  if ( pTransEntry )
  {
    // Send the command to ZigBee
    status = sendDevZoneEnrollmentRsp( pTransEntry, pZoneEnrollRsp );
  }
  else
  {
    uiPrintf( "Processing IAS ACE Zone Enrollment Response Failure, no transaction entry\n" );
  }
  
  if ( status == ZSuccess )
  {
    genericCnf.status = GW_STATUS_T__STATUS_SUCCESS;
  }
  else if ( ZInvalidParameter )
  {
    uiPrintf( "Processing IAS ACE Zone Enrollment Response Failure, status: %d\n", status );
    
    genericCnf.status = GW_STATUS_T__STATUS_INVALID_PARAMETER;
  }
  else
  {
    uiPrintf( "Processing IAS ACE Zone Enrollment Response Failure, status: %d\n", status );
    
    genericCnf.status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  genericCnf.has_sequencenumber = TRUE;
  genericCnf.sequencenumber = pZoneEnrollRsp->sequencenumber;
  
  sendZbGenericCnf( connection, &genericCnf );
}

/*********************************************************************
 * @fn      processDevAceArmReqInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   transSeqNum - ZCL transaction ID
 * @param   len - length of ZCL payload
 * @param   pData - pointer to ZCL payload data
 *
 * @return  ZStatus_t
 */
static ZStatus_t processDevAceArmReqInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData )
{
  ZStatus_t status;
  DevAceArmReqInd aceArmReq = DEV_ACE_ARM_REQ_IND__INIT;
  
  uiPrintf( "Processing IAS ACE Arm Request Indication, TransId: %d\n", transSeqNum );
  
  // Store sequence number and source address information
  aceArmReq.sequencenumber = gGwAppTransSeqNum;
  aceArmReq.srcaddress = pSrcAddress;
  
  // Store payload data
  aceArmReq.armmode = *pData;
  
  // Send command to the app(s)
  status = sendDevAceArmReqInd( &aceArmReq );
  
  if ( status == ZSuccess )
  {
    // Post transaction entry
    gwMsgTransPost( FALSE, GW_RSP_NONE, ALL_APP_CONNECTIONS, pSrcAddress->ieeeaddr, GW_CMD_ID_T__DEV_ACE_ARM_REQ_IND, 
                    gGwAppTransSeqNum, transSeqNum, APP_TRANSACTION_TIMEOUT );
                    
    gwIncreaseCurrentAppSeqNum();                    
  }
  else
  {
    uiPrintf( "Processing IAS ACE Arm Request Indication Failure, status: %d\n", status );
  }
  
  return status;
}

/*********************************************************************
 * @fn      processDevAceArmRsp
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pAceArmRsp - pointer to received message structure
 *
 * @return  none
 */
static void processDevAceArmRsp( int connection, DevAceArmRsp *pAceArmRsp )
{
  gsGwMsgTransTable_t *pTransEntry;
  GwZigbeeGenericCnf genericCnf = GW_ZIGBEE_GENERIC_CNF__INIT;
  ZStatus_t status = ZFailure;
  
  uiPrintf( "Processing ACE Arm Response, appTransId: %d\n", pAceArmRsp->sequencenumber );

  // Lookup transaction entry
  pTransEntry = gwMsgTransGetByAppTransId( pAceArmRsp->sequencenumber );
      
  // Verify transaction table entry
  if ( pTransEntry && (pTransEntry->cmdId == GW_CMD_ID_T__DEV_ACE_ARM_REQ_IND) )
  {      
    // Send the command to ZigBee
    status = sendDevAceArmRsp( pTransEntry, pAceArmRsp );
  }
  else
  {
    uiPrintf( "Processing IAS ACE Arm Response Failure, no transaction entry\n" );
  }
  
  if ( status == ZSuccess )
  {
    genericCnf.status = GW_STATUS_T__STATUS_SUCCESS;
  }
  else if ( status == ZInvalidParameter )
  {
    uiPrintf( "Processing IAS ACE Arm Response Failure - Invalid Parameter\n" );
    
    genericCnf.status = GW_STATUS_T__STATUS_INVALID_PARAMETER;
  }
  else
  {
    uiPrintf( "Processing IAS ACE Arm Response Failure, status: %d\n", status );
    
    genericCnf.status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  genericCnf.has_sequencenumber = TRUE;
  genericCnf.sequencenumber = pAceArmRsp->sequencenumber;
  
  sendZbGenericCnf( connection, &genericCnf );
}

/*********************************************************************
 * @fn      processDevAceBypassInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   transSeqNum - ZCL transaction ID
 * @param   len - length of ZCL payload
 * @param   pData - pointer to ZCL payload data
 *
 * @return  ZStatus_t
 */
static ZStatus_t processDevAceBypassInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, 
                                         uint16 len, uint8 *pData )
{
  uint8 zoneCount;
  ZStatus_t status;
  DevAceBypassInd aceBypassInd = DEV_ACE_BYPASS_IND__INIT;
  
  uiPrintf( "Processing IAS ACE Bypass Indication, TransId: %d\n", transSeqNum );
  
  // Store sequence number and source address information
  aceBypassInd.srcaddress = pSrcAddress;
  
  zoneCount = *pData++;
  
  // Store payload data
  aceBypassInd.zoneidlist.len = zoneCount;
  
  aceBypassInd.zoneidlist.data = malloc( zoneCount );
  if ( !aceBypassInd.zoneidlist.data )
  {
    return ZMemError;
  }
  
  memcpy( aceBypassInd.zoneidlist.data, pData, zoneCount );  
  
  // Send command to the app(s)
  status = sendDevAceBypassInd( &aceBypassInd );
  
  // Free memory
  free( aceBypassInd.zoneidlist.data );
  
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing IAS ACE Bypass Indication Failure, status: %d\n", status );
  }
  
  return status;
}

/*********************************************************************
 * @fn      processDevAceEmergencyConditionInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   transSeqNum - ZCL transaction ID
 * @param   cmdId - incomming ACE cluster command ID
 *
 * @return  ZStatus_t
 */
static ZStatus_t processDevAceEmergencyConditionInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, 
                                                     uint8 cmdId )
{
  ZStatus_t status;
  DevAceEmergencyConditionInd aceEmergCondInd = DEV_ACE_EMERGENCY_CONDITION_IND__INIT;
  
  uiPrintf( "Processing IAS ACE Emergency Condition Indication, TransId: %d\n", transSeqNum );
  
  // Store sequence number and source address information
  aceEmergCondInd.srcaddress = pSrcAddress;
  
  // Store command ID for emergency type
  aceEmergCondInd.emergencyconditiontype = cmdId;
  
  // Send command to the app(s)
  status = sendDevAceEmergencyConditionInd( &aceEmergCondInd );
  
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing IAS ACE Emergency Condition Indication Failure, status: %d\n", status );
  }
  
  return status;
}

/*********************************************************************
 * @fn      processDevAceGetZoneIdMapReqInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   transSeqNum - ZCL transaction ID
 * @param   len - length of ZCL payload
 * @param   pData - pointer to ZCL payload data
 *
 * @return  ZStatus_t
 */
static ZStatus_t processDevAceGetZoneIdMapReqInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, 
                                                  uint16 len, uint8 *pData )
{
  ZStatus_t status;
  DevAceGetZoneIdMapReqInd aceGetZoneIdMapReq = DEV_ACE_GET_ZONE_ID_MAP_REQ_IND__INIT;
  
  uiPrintf( "Processing IAS ACE Get Zone ID Map Request Indication, TransId: %d\n", transSeqNum );
  
  // Store sequence number and source address information
  aceGetZoneIdMapReq.sequencenumber = gGwAppTransSeqNum;
  aceGetZoneIdMapReq.srcaddress = pSrcAddress;
  
  // Send command to the app(s)
  status = sendDevAceGetZoneIdMapReqInd( &aceGetZoneIdMapReq );
  
  if ( status == ZSuccess )
  {
    // Post transaction entry
    gwMsgTransPost( FALSE, GW_RSP_NONE, ALL_APP_CONNECTIONS, 0, GW_CMD_ID_T__DEV_ACE_GET_ZONE_ID_MAP_REQ_IND, 
                    gGwAppTransSeqNum, transSeqNum, APP_TRANSACTION_TIMEOUT );
                    
    gwIncreaseCurrentAppSeqNum();
  }
  else
  {
    uiPrintf( "Processing IAS ACE Get Zone ID Map Request Indication Failure, status: %d\n", status );
  }
  
  return status;
}

/*********************************************************************
 * @fn      processDevAceGetZoneIdMapRsp
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pAceGetZoneIdMapRsp - pointer to received message structure
 *
 * @return  none
 */
static void processDevAceGetZoneIdMapRsp( int connection, DevAceGetZoneIdMapRsp *pAceGetZoneIdMapRsp )
{
  gsGwMsgTransTable_t *pTransEntry;
  GwZigbeeGenericCnf genericCnf = GW_ZIGBEE_GENERIC_CNF__INIT;
  ZStatus_t status = ZFailure;
  
  uiPrintf( "Processing ACE Get Zone ID Map Response, appTransId: %d\n", pAceGetZoneIdMapRsp->sequencenumber );

  // Lookup transaction entry
  pTransEntry = gwMsgTransGetByAppTransId( pAceGetZoneIdMapRsp->sequencenumber );
      
  // Verify transaction table entry
  if ( pTransEntry && (pTransEntry->cmdId == GW_CMD_ID_T__DEV_ACE_GET_ZONE_ID_MAP_REQ_IND) )
  {      
    // Send the command to ZigBee
    status = sendDevAceGetZoneIdMapRsp( pAceGetZoneIdMapRsp, pTransEntry->zclTransId );
  }
  else
  {
    uiPrintf( "Processing IAS ACE Get Zone ID Map Response Failure, no transaction entry\n" );
  }
  
  if ( status == ZSuccess )
  {
    genericCnf.status = GW_STATUS_T__STATUS_SUCCESS;
  }
  else if ( status == ZInvalidParameter )
  {
    uiPrintf( "Processing IAS ACE Get Zone ID Map Response Failure, Invalid Parameter\n" );
    
    genericCnf.status = GW_STATUS_T__STATUS_INVALID_PARAMETER;
  }
  else
  {
    uiPrintf( "Processing IAS ACE Get Zone ID Map Response Failure, status: %d\n", status );
    
    genericCnf.status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  genericCnf.has_sequencenumber = TRUE;
  genericCnf.sequencenumber = pAceGetZoneIdMapRsp->sequencenumber;
  
  sendZbGenericCnf( connection, &genericCnf );
}

/*********************************************************************
 * @fn      processDevAceGetZoneInformationReqInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   transSeqNum - ZCL transaction ID
 * @param   len - length of ZCL payload
 * @param   pData - pointer to ZCL payload data
 *
 * @return  ZStatus_t
 */
static ZStatus_t processDevAceGetZoneInformationReqInd( GwAddressStructT *pSrcAddress, uint8 transSeqNum, uint16 len, uint8 *pData )
{
  ZStatus_t status;
  DevAceGetZoneInformationReqInd aceGetZoneInformationReq = DEV_ACE_GET_ZONE_INFORMATION_REQ_IND__INIT;
  
  uiPrintf( "Processing IAS ACE Get Zone Information Request Indication, TransId: %d\n", transSeqNum );
  
  // Store sequence number and source address information
  aceGetZoneInformationReq.sequencenumber = gGwAppTransSeqNum;
  aceGetZoneInformationReq.srcaddress = pSrcAddress;
  
  // Store payload data
  aceGetZoneInformationReq.zoneid = *pData;
  
  // Send command to the app(s)
  status = sendDevAceGetZoneInformationReqInd( &aceGetZoneInformationReq );
  
  if ( status == ZSuccess )
  {
    // Post transaction entry
    gwMsgTransPost( FALSE, GW_RSP_NONE, ALL_APP_CONNECTIONS, 0, GW_CMD_ID_T__DEV_ACE_GET_ZONE_INFORMATION_REQ_IND, 
                    gGwAppTransSeqNum, transSeqNum, APP_TRANSACTION_TIMEOUT );
                    
    gwIncreaseCurrentAppSeqNum();                    
  }
  else
  {
    uiPrintf( "Processing IAS ACE Get Zone Information Request Indication Failure, status: %d\n", status );
  }
  
  return status;
}

/*********************************************************************
 * @fn      processDevAceGetZoneInformationRsp
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pAceGetZoneInfoRsp - pointer to received message structure
 *
 * @return  none
 */
static void processDevAceGetZoneInformationRsp( int connection, DevAceGetZoneInformationRsp *pAceGetZoneInfoRsp )
{
  gsGwMsgTransTable_t *pTransEntry;
  GwZigbeeGenericCnf genericCnf = GW_ZIGBEE_GENERIC_CNF__INIT;
  ZStatus_t status = ZFailure;
  
  uiPrintf( "Processing ACE Get Zone Information Response, appTransId: %d\n", pAceGetZoneInfoRsp->sequencenumber );

  // Lookup transaction entry
  pTransEntry = gwMsgTransGetByAppTransId( pAceGetZoneInfoRsp->sequencenumber );
      
  // Verify transaction table entry
  if ( pTransEntry && (pTransEntry->cmdId == GW_CMD_ID_T__DEV_ACE_GET_ZONE_INFORMATION_REQ_IND) )
  {  
    // Send response to ZigBee
    status = sendDevAceGetZoneInformationRsp( pTransEntry, pAceGetZoneInfoRsp );
  }
  else
  {
    uiPrintf( "Processing IAS ACE Get Zone Information Response Failure, no transaction entry\n" );
  }
  
  if ( status == ZSuccess )
  {
    genericCnf.status = GW_STATUS_T__STATUS_SUCCESS;
  }
  else if ( status == ZInvalidParameter )
  {
    uiPrintf( "Processing IAS ACE Get Zone Information Response Failure - Invalid Parameter\n" );
    
    genericCnf.status = GW_STATUS_T__STATUS_INVALID_PARAMETER;
  }
  else
  {
    uiPrintf( "Processing IAS ACE Get Zone Information Response Failure, status: %d\n", status );
    
    genericCnf.status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  genericCnf.has_sequencenumber = TRUE;
  genericCnf.sequencenumber = pAceGetZoneInfoRsp->sequencenumber;
  
  sendZbGenericCnf( connection, &genericCnf );
}

/*********************************************************************
 * @fn      processDevSetIdentifyModeReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetIdentifyModeReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevSetIdentifyModeReq( int connection, DevSetIdentifyModeReq *pSetIdentifyModeReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Set Identify Mode Request\n" );
  
  status = sendDevSetIdentifyModeReq( pSetIdentifyModeReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_GENERIC, pSetIdentifyModeReq->dstaddress->addresstype, 
                  connection, pSetIdentifyModeReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_SET_IDENTIFY_MODE_REQ, status );

  // debug message
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Set Identify Mode Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevSetOnOffStateReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pOnOffStateReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevSetOnOffStateReq( int connection, DevSetOnOffStateReq *pOnOffStateReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Set On/Off State Request\n" );
  
  // send to ZigBee to turn on/off the light
  status = sendDevSetOnOffStateReq( pOnOffStateReq );
  
  // handle the response to the app
  gwHandleCnfRsp( GW_RSP_GENERIC, pOnOffStateReq->dstaddress->addresstype, 
                  connection, pOnOffStateReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_SET_ONOFF_STATE_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Set On/Off State Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevSendIdentifyQueryReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 *
 * @return  none
 */
static void processDevSendIdentifyQueryReq( int connection, DevSendIdentifyQueryReq *pSendIdentifyQueryReq )
{
  ZStatusValues status = ZSTATUS_VALUES__ZDecodeError;
  uint8 endpoint;
  afAddrType_t dstAddr;
  
  uiPrintf( "Processing Send Identify Query.\n" );
  dstAddr.addr.shortAddr = 0xFFFF; //Self short address
  dstAddr.addrMode = afAddr16Bit;
  dstAddr.endPoint = 0xFF;

  endpoint = pSendIdentifyQueryReq->endpoint;
  
  if (bdb_identify_query_sequence < 256)
  {
	bdb_identify_query_sequence++;
  }
  else
  {
  	bdb_identify_query_sequence = 0;
  }
  
  status = zclGeneral_SendIdentifyQuery(endpoint, &dstAddr, TRUE, bdb_identify_query_sequence);
  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_GENERIC, GW_ADDRESS_TYPE_T__BROADCAST, 
                  connection, gLocalDeviceInfo.ieeeaddr, 
                  GW_CMD_ID_T__DEV_SEND_IDENTIFY_QUERY_REQ, status );
  // some debug code
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
	uiPrintf( "Processing Send Identify Query Request Failed - Status:%d\n", status );
  }

}

/*********************************************************************
 * @fn      processDevSetFindingAndBindingTimerStatusReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 *
 * @return  none
 */
void processDevSetFindingAndBindingTimerStatusReq( int connection, DevSetFindingAndBindingTimer *pSetFindingAndBindingTimerStatus )
{
  ZStatusValues status = ZSTATUS_VALUES__ZSuccess;
  
  uiPrintf( "Setting Finding And Binding Timer Status\n" );
  GwFindingAndBindingTimerStatus = pSetFindingAndBindingTimerStatus->status;
  
  gwHandleCnfRsp( GW_RSP_NONE, GW_ADDRESS_TYPE_T__BROADCAST, 
                  connection, gLocalDeviceInfo.ieeeaddr, 
                  GW_CMD_ID_T__DEV_SET_FINDING_AND_BINDING_TIMER_STATUS_REQ, status );
}

/*********************************************************************
 * @fn      processDevSetLevelReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetLevelReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevSetLevelReq( int connection, DevSetLevelReq *pSetLevelReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Set Level Request\n" );
  
  // Send to ZigBee
  status = sendDevSetLevelReq( pSetLevelReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_GENERIC, pSetLevelReq->dstaddress->addresstype, 
                  connection, pSetLevelReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_SET_LEVEL_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Set Level Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetLevelReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetLevelReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevGetLevelReq( int connection, DevGetLevelReq *pGetLevelReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Get Level Request\n" );
  
  status = sendDevGetLevelReq( pGetLevelReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetLevelReq->dstaddress->addresstype, 
                  connection, pGetLevelReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_GET_LEVEL_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Get Level Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetLevelRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetLevelRsp - pointer to command structure
 * @param   payloadLen - length of incoming APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processDevGetLevelRspInd( int connection, DevGetLevelRspInd *pGetLevelRsp, 
                                      uint16 payloadLen, uint8 *pPayload )
{  
  uint16 getLevelAttrId;
  
  uiPrintf( "Processing Get Level Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields
  
  getLevelAttrId = BUILD_UINT16( pPayload[0], pPayload[1] );
  pPayload += 2;
  
  // Verify attribute ID and read success
  if ( ( getLevelAttrId == ATTRID_LEVEL_CURRENT_LEVEL ) && ( *pPayload == ZSuccess ) )
  {
    pPayload += 2;  // move pointer past status and data type fields
    
    pGetLevelRsp->status = GW_STATUS_T__STATUS_SUCCESS;
    
    pGetLevelRsp->levelvalue = *pPayload;
  }
  else
  {
    pGetLevelRsp->status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  // Send response back to app
  sendDevGetLevelRspInd( connection, pGetLevelRsp );
}

/*********************************************************************
 * @fn      sendDevGetLevelRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetLevelRsp - pointer to received message structure
 *
 * @return  none
 */
static void sendDevGetLevelRspInd( int connection, DevGetLevelRspInd *pGetLevelRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Get Level Response Indication\n" );
  
  len = dev_get_level_rsp_ind__get_packed_size( pGetLevelRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_get_level_rsp_ind__pack( pGetLevelRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_GET_LEVEL_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Get Level Response Indication Failed, memory error\n" );
  }
}

/*********************************************************************
 * @fn      processDevGetOnOffStateReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetOnOffStateReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevGetOnOffStateReq( int connection, DevGetOnOffStateReq *pGetOnOffStateReq )
{
  ZStatusValues status;
  
  uiPrintf( "Processing Get On/Off State Request\n" );
  
  status = sendDevGetOnOffStateReq( pGetOnOffStateReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetOnOffStateReq->dstaddress->addresstype, 
                  connection, pGetOnOffStateReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_GET_ONOFF_STATE_REQ, status );

  // some debug code
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing Get On/Off State Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetOnOffStateRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetOnOffStateRsp - pointer to command structure
 * @param   payloadLen - length of incoming APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processDevGetOnOffStateRspInd( int connection, DevGetOnOffStateRspInd *pGetOnOffStateRsp, 
                                           uint16 payloadLen, uint8 *pPayload )
{  
  uint16 getOnOffStateAttrId;
  
  uiPrintf( "Processing Get On/Off State Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields
  
  getOnOffStateAttrId = BUILD_UINT16( pPayload[0], pPayload[1] );
  pPayload += 2;
  
  // Verify attribute ID and read success
  if ( ( getOnOffStateAttrId == ATTRID_ON_OFF ) && ( *pPayload == ZSuccess ) )
  {
    pPayload += 2;  // move pointer past status and data type fields
    
    pGetOnOffStateRsp->status = GW_STATUS_T__STATUS_SUCCESS;
    
    pGetOnOffStateRsp->statevalue = *pPayload;
  }
  else
  {
    pGetOnOffStateRsp->status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  // Send response back to app
  sendDevGetOnOffStateRspInd( connection, pGetOnOffStateRsp );
}

/*********************************************************************
 * @fn      sendDevGetOnOffStateRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetOnOffStateRsp - pointer to received message structure
 *
 * @return  none
 */
static void sendDevGetOnOffStateRspInd( int connection, DevGetOnOffStateRspInd *pGetOnOffStateRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Get On/Off State Response Indication\n" );
  
  len = dev_get_on_off_state_rsp_ind__get_packed_size( pGetOnOffStateRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_get_on_off_state_rsp_ind__pack( pGetOnOffStateRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_GET_ONOFF_STATE_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Get On/Off State Response Indication Failed, memory error\n" );
  }
}

/*********************************************************************
 * @fn      processDevSetColorReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetColorReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevSetColorReq( int connection, DevSetColorReq *pSetColorReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Set Color Request\n" );
  
  // send to ZigBee
  status = sendDevSetColorReq( pSetColorReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_GENERIC, pSetColorReq->dstaddress->addresstype, 
                  connection, pSetColorReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_SET_COLOR_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Set Color Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetColorReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetColorReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevGetColorReq( int connection, DevGetColorReq *pGetColorReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Get Color Request\n" );
  
  status = sendDevGetColorReq( pGetColorReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetColorReq->dstaddress->addresstype, 
                  connection, pGetColorReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_GET_COLOR_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Get Color Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetColorRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetColorRsp - pointer to command structure
 * @param   payloadLen - length of incoming APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processDevGetColorRspInd( int connection, DevGetColorRspInd *pGetColorRsp, 
                                      uint16 payloadLen, uint8 *pPayload )
{  
  uint8 rspStatus = GW_STATUS_T__STATUS_SUCCESS;
  uint16 getColorAttributeId;
  
  uiPrintf( "Processing Get Color Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields
  
  getColorAttributeId = BUILD_UINT16( pPayload[0], pPayload[1] );
  pPayload += 2;
  
  // Verify attribute ID and read success
  if ( ( getColorAttributeId == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE ) && ( *pPayload == ZSuccess ) )
  {
    pPayload += 2;  // move pointer past status and data type fields
    
    pGetColorRsp->huevalue = *pPayload++;
  }
  else
  {
    pPayload++;   // move pointer past status field
    
    rspStatus = GW_STATUS_T__STATUS_FAILURE;
  }
  
  getColorAttributeId = BUILD_UINT16( pPayload[0], pPayload[1] );
  pPayload += 2;
  
  // Verify attribute ID and read success
  if ( ( getColorAttributeId == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION ) && ( *pPayload == ZSuccess ) )
  {
    pPayload += 2;  // move pointer past status and data type fields
    
    pGetColorRsp->satvalue = *pPayload;
  }
  else
  {
    rspStatus = GW_STATUS_T__STATUS_FAILURE;
  }
  
  pGetColorRsp->status = rspStatus;
  
  // Send response back to app
  sendDevGetColorRspInd( connection, pGetColorRsp );
}

/*********************************************************************
 * @fn      sendDevGetColorRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetColorRsp - pointer to received message structure
 *
 * @return  none
 */
static void sendDevGetColorRspInd( int connection, DevGetColorRspInd *pGetColorRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Get Color Response Indication\n" );
  
  len = dev_get_color_rsp_ind__get_packed_size( pGetColorRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_get_color_rsp_ind__pack( pGetColorRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_GET_COLOR_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Get Color Response Indication Failed, memory error\n" );
  }
}

/*********************************************************************
 * @fn      processDevSetColorTempReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetColorTempReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevSetColorTempReq( int connection, DevSetColorTempReq *pSetColorTempReq )
{
  ZStatus_t status;

  uiPrintf( "Processing Set ColorTemp Request\n" );

  // send to ZigBee
  status = sendDevSetColorTempReq( pSetColorTempReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_GENERIC, pSetColorTempReq->dstaddress->addresstype,
                  connection, pSetColorTempReq->dstaddress->ieeeaddr,
                  GW_CMD_ID_T__DEV_SET_COLOR_TEMP_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Set ColorTemp Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetColorTempReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetColorTempReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevGetColorTempReq( int connection, DevGetColorTempReq *pGetColorTempReq )
{
  ZStatus_t status;

  uiPrintf( "Processing Get ColorTemp Request\n" );

  status = sendDevGetColorTempReq( pGetColorTempReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetColorTempReq->dstaddress->addresstype,
                  connection, pGetColorTempReq->dstaddress->ieeeaddr,
                  GW_CMD_ID_T__DEV_GET_COLOR_TEMP_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Get ColorTemp Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetColorTempRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetColorTempRsp - pointer to command structure
 * @param   payloadLen - length of incoming APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processDevGetColorTempRspInd( int connection, DevGetColorTempRspInd *pGetColorTempRsp,
                                      uint16 payloadLen, uint8 *pPayload )
{
  uint8 rspStatus = GW_STATUS_T__STATUS_SUCCESS;
  uint16 getColorTempAttributeId;

  uiPrintf( "Processing Get ColorTemp Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields

  getColorTempAttributeId = BUILD_UINT16( pPayload[0], pPayload[1] );
  pPayload += 2;

  // Verify attribute ID and read success
  if ( ( getColorTempAttributeId == ATTRID_LIGHTING_COLOR_CONTROL_COLOR_TEMPERATURE ) && ( *pPayload == ZSuccess ) )
  {
    pPayload += 2;  // move pointer past status and data type fields

    pGetColorTempRsp->temperaturevalue = *pPayload++;
  }
  else
  {
    pPayload++;   // move pointer past status field

    rspStatus = GW_STATUS_T__STATUS_FAILURE;
  }

  pGetColorTempRsp->status = rspStatus;

  // Send response back to app
  sendDevGetColorTempRspInd( connection, pGetColorTempRsp );
}

/*********************************************************************
 * @fn      sendDevGetColorRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetColorRsp - pointer to received message structure
 *
 * @return  none
 */
static void sendDevGetColorTempRspInd( int connection, DevGetColorTempRspInd *pGetColorTempRsp )
{
  int len;
  uint8 *pBuf;

  uiPrintf( "Sending Get ColorTemp Response Indication\n" );

  len = dev_get_color_temp_rsp_ind__get_packed_size( pGetColorTempRsp );

  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_get_color_temp_rsp_ind__pack( pGetColorTempRsp, pBuf );

    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE,
                   GW_CMD_ID_T__DEV_GET_COLOR_TEMP_RSP_IND, len, pBuf );

    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Get ColorTemp Response Indication Failed, memory error\n" );
  }
}

/*********************************************************************
 * @fn      processDevGetTempReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetTempReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevGetTempReq( int connection, DevGetTempReq *pGetTempReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Get Temperature Request\n" );
  
  status = sendDevGetTempReq( pGetTempReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetTempReq->dstaddress->addresstype, 
                  connection, pGetTempReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_GET_TEMP_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Get Temperature Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetTempRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetTempRsp - pointer to command structure
 * @param   payloadLen - length of incoming APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processDevGetTempRspInd( int connection, DevGetTempRspInd *pGetTempRsp, 
                                     uint16 payloadLen, uint8 *pPayload )
{  
  uint16 measuredValueAttrId;
  
  uiPrintf( "Processing Get Temperature Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields
  
  measuredValueAttrId = BUILD_UINT16( pPayload[0], pPayload[1] );
  
  pPayload += 2;
  
  // Verify attribute ID and read success
  if ( ( measuredValueAttrId == ATTRID_MS_TEMPERATURE_MEASURED_VALUE ) && ( *pPayload == ZSuccess ) )
  {
    pPayload += 2;  // move pointer past status and data type fields
    
    pGetTempRsp->status = GW_STATUS_T__STATUS_SUCCESS;
    pGetTempRsp->temperaturevalue = BUILD_UINT16( pPayload[0], pPayload[1] );
  }
  else
  {
    pGetTempRsp->status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  // Send response back to app
  sendDevGetTempRspInd( connection, pGetTempRsp );
}

/*********************************************************************
 * @fn      sendDevGetTempRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetTempRsp - pointer to received message structure
 *
 * @return  none
 */
static void sendDevGetTempRspInd( int connection, DevGetTempRspInd *pGetTempRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Get Temperature Response Indication\n" );
  
  len = dev_get_temp_rsp_ind__get_packed_size( pGetTempRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_get_temp_rsp_ind__pack( pGetTempRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_GET_TEMP_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Get Temperature Response Indication Failed, memory error\n" );
  }
}

/*********************************************************************
 * @fn      processDevGetPowerReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetPowerReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevGetPowerReq( int connection, DevGetPowerReq *pGetPowerReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Get Power Request\n" );
  
  // send to ZigBee
  status = sendDevGetPowerReq( pGetPowerReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetPowerReq->dstaddress->addresstype, 
                  connection, pGetPowerReq->dstaddress->ieeeaddr, GW_CMD_ID_T__DEV_GET_POWER_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Get Power Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetPowerRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetPowerRsp - pointer to command structure
 * @param   payloadLen - length of incoming APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processDevGetPowerRspInd( int connection, DevGetPowerRspInd *pGetPowerRsp, 
                                      uint16 payloadLen, uint8 *pPayload )
{  
  uint16 measuredValueAttrId;
  
  uiPrintf( "Processing Get Power Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields
  
  measuredValueAttrId = BUILD_UINT16( pPayload[0], pPayload[1] );
  
  pPayload += 2;
  
  // Verify attribute ID
  if ( ( measuredValueAttrId == ATTRID_SE_INSTANTANEOUS_DEMAND ) && ( *pPayload == ZSuccess ) )
  {
    pPayload += 2;  // move pointer past status and data type fields
    
    pGetPowerRsp->status = GW_STATUS_T__STATUS_SUCCESS;
    pGetPowerRsp->powervalue = BUILD_UINT32( pPayload[0], pPayload[1], pPayload[2], 0 );
  }
  else
  {
    pGetPowerRsp->status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  // send response back to app
  sendDevGetPowerRspInd( connection, pGetPowerRsp );
}

/*********************************************************************
 * @fn      processDevGetHumidityReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetHumidityReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevGetHumidityReq( int connection, DevGetHumidityReq *pGetHumidityReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Get Humidity Request\n" );
  
  // send to ZigBee
  status = sendDevGetHumidityReq( pGetHumidityReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pGetHumidityReq->dstaddress->addresstype, 
                  connection, pGetHumidityReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_GET_HUMIDITY_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Get Humidity Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetHumidityRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetHumidityRsp - pointer to command structure
 * @param   payloadLen - length of incoming APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 */
static void processDevGetHumidityRspInd( int connection, DevGetHumidityRspInd *pGetHumidityRsp, 
                                         uint16 payloadLen, uint8 *pPayload )
{  
  uint16 measuredValueAttrId;
  
  uiPrintf( "Processing Get Humidity Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields
  
  measuredValueAttrId = BUILD_UINT16( pPayload[0], pPayload[1] );
  
  pPayload += 2;
  
  // Verify attribute ID
  if ( ( measuredValueAttrId == ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE ) && ( *pPayload == ZSuccess ) )
  {
    pPayload += 2;  // move pointer past status and data type fields
    
    pGetHumidityRsp->status = GW_STATUS_T__STATUS_SUCCESS;
    pGetHumidityRsp->humidityvalue = BUILD_UINT32( pPayload[0], pPayload[1], pPayload[2], 0 );
  }
  else
  {
    pGetHumidityRsp->status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  // send response back to app
  sendDevGetHumidityRspInd( connection, pGetHumidityRsp );
}

/*********************************************************************
 * @fn      sendDevGetPowerRspInd
 *
 * @brief   Send Get Power Response Indication message.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetPowerRsp - pointer to received message structure
 *
 * @return  none
 */
static void sendDevGetPowerRspInd( int connection, DevGetPowerRspInd *pGetPowerRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Get Power Response Indication\n" );
  
  len = dev_get_power_rsp_ind__get_packed_size( pGetPowerRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_get_power_rsp_ind__pack( pGetPowerRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_GET_POWER_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Get Power Response Indication Failed, memory error\n" );
  }
}

/*********************************************************************
 * @fn      sendDevGetHumidityRspInd
 *
 * @brief   Send Get Humidity Response Indication message.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetHumidityRsp - pointer to received message structure
 *
 * @return  none
 */
static void sendDevGetHumidityRspInd( int connection, DevGetHumidityRspInd *pGetHumidityRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Get Humidity Response Indication\n" );
  
  len = dev_get_humidity_rsp_ind__get_packed_size( pGetHumidityRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_get_humidity_rsp_ind__pack( pGetHumidityRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_GET_HUMIDITY_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Get Humidity Response Indication Failed, memory error\n" );
  }
}

/*********************************************************************
 * @fn      processDevSetDoorLockReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetDoorLockReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevSetDoorLockReq( int connection, DevSetDoorLockReq *pSetDoorLockReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Set Door Lock Request\n" );
  
  // send to ZigBee
  status = sendDevSetDoorLockReq( pSetDoorLockReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pSetDoorLockReq->dstaddress->addresstype, 
                  connection, pSetDoorLockReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_SET_DOOR_LOCK_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Set Door Lock Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevSetDoorLockRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   pSrcAddress - source address information (protobuf form)
 * @param   cmdId - ZCL response command ID
 * @param   transSeqNum - ZCL Transaction ID
 * @param   len - length of ZCL payload
 * @param   pData - pointer to ZCL payload data
 *
 * @return  ZStatus_t
 */
static ZStatus_t processDevSetDoorLockRspInd( GwAddressStructT *pSrcAddress, uint8 cmdId, 
                                              uint8 transSeqNum, uint16 len, uint8 *pData  )
{  
  gsGwMsgTransTable_t *pTransEntry;
  DevSetDoorLockRspInd setDoorLockRsp = DEV_SET_DOOR_LOCK_RSP_IND__INIT;
  ZStatus_t status = ZSuccess;
  
  uiPrintf( "Processing Set Door Lock Response Indication\n" );
  
  // Get transaction entry
  pTransEntry = gwMsgTransGetByZclTransId( transSeqNum, GW_CMD_ID_T__DEV_SET_DOOR_LOCK_REQ );
  
  if ( !pTransEntry )
  {
    uiPrintf( "Processing Set Door Lock Response Failed, no transaction table entry - TransId: %d\n", transSeqNum );
    
    return ZFailure;
  }

  if ( *pData == ZCL_STATUS_SUCCESS )
  {
    setDoorLockRsp.status = GW_STATUS_T__STATUS_SUCCESS;
  }
  else
  {
    setDoorLockRsp.status = GW_STATUS_T__STATUS_FAILURE;  
  }
  
  // Determine if response count needs to be handled
  gwRspCountHandler( pTransEntry );

  setDoorLockRsp.sequencenumber = pTransEntry->appTransId;
  setDoorLockRsp.lockmode = cmdId;
  setDoorLockRsp.srcaddress = pSrcAddress;
  
  // Send response back to app
  status = sendDevSetDoorLockRspInd( pTransEntry->connection, &setDoorLockRsp );
  
  if ( status == ZSuccess )
  {
    // Clear transaction entry
    gwMsgRemoveTrans( pTransEntry->appTransId );
  }
  
  return status;
}

/*********************************************************************
 * @fn      sendDevSetDoorLockRspInd
 *
 * @brief   Send Set Door Lock Response Indication message.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetDoorLockRsp - pointer to received message structure
 *
 * @return  ZStatusValues
 */
static ZStatusValues sendDevSetDoorLockRspInd( int connection, DevSetDoorLockRspInd *pSetDoorLockRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Set Door Lock Response Indication\n" );
  
  len = dev_set_door_lock_rsp_ind__get_packed_size( pSetDoorLockRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_set_door_lock_rsp_ind__pack( pSetDoorLockRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_SET_DOOR_LOCK_RSP_IND, len, pBuf );
    
    free( pBuf );
    
    return ZSTATUS_VALUES__ZSuccess;
  }
  else
  {
    uiPrintf( "Sending Set Door Lock Response Indication Failed, memory error\n" );
    
    return ZSTATUS_VALUES__ZMemError;
  }
}

/*********************************************************************
 * @fn      processDevGetDoorLockStateReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pDoorLockStateReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevGetDoorLockStateReq( int connection, DevGetDoorLockStateReq *pDoorLockStateReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Get Door Lock State Request\n" );
  
  // send to ZigBee
  status = sendDevGetDoorLockStateReq( pDoorLockStateReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_SPECIFIC, pDoorLockStateReq->dstaddress->addresstype, 
                  connection, pDoorLockStateReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_GET_DOOR_LOCK_STATE_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Get Door Lock State Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevGetDoorLockStateRspInd
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pDoorLockStateRsp - pointer to command structure
 * @param   payloadLen - length of incoming APS payload
 * @param   pPayload - pointer to received APS payload
 *
 * @return  none
 *
 * @return  ZStatusValues
 */
static ZStatusValues processDevGetDoorLockStateRspInd( int connection, DevGetDoorLockStateRspInd *pDoorLockStateRsp, 
                                                       uint16 payloadLen, uint8 *pPayload  )
{  
  uint16 attrId;
  ZStatusValues status = ZSTATUS_VALUES__ZSuccess;
  
  uiPrintf( "Processing Get Door Lock State Response Indication\n" );

  // Sequence number and source address information already filled,
  // complete remaining fields
  
  attrId = BUILD_UINT16( pPayload[0], pPayload[1] );
  pPayload += 2;
  
  pDoorLockStateRsp->status = GW_STATUS_T__STATUS_SUCCESS; // assume success
  
  // Verify attribute ID
  if ( (attrId == ATTRID_CLOSURES_LOCK_STATE) && (*pPayload == ZSuccess) )
  {
    pPayload += 2;  // move pointer past status and data type fields
    
    pDoorLockStateRsp->lockstate = *pPayload++;
  }
  else
  {
    pDoorLockStateRsp->status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  attrId = BUILD_UINT16( pPayload[0], pPayload[1] );
  pPayload += 2;
  
  if ( (attrId == ATTRID_CLOSURES_DOOR_STATE) && (*pPayload == ZSuccess) )
  {
    pPayload += 2;  // move pointer past status and data type fields
    
    pDoorLockStateRsp->doorstate = *pPayload;
  }
  else
  {
    pDoorLockStateRsp->status = GW_STATUS_T__STATUS_FAILURE;
  }
  
  // send response back to app
  status = sendDevGetDoorLockStateRspInd( connection, pDoorLockStateRsp );
  
  return status;
}

/*********************************************************************
 * @fn      sendDevGetDoorLockStateRspInd
 *
 * @brief   Send Get Door Lock State Response Indication message.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetDoorLockStateRsp - pointer to received message structure
 *
 * @return  ZStatusValues
 */
static ZStatusValues sendDevGetDoorLockStateRspInd( int connection, DevGetDoorLockStateRspInd *pGetDoorLockStateRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Get Door Lock State Response Indication\n" );
  
  len = dev_get_door_lock_state_rsp_ind__get_packed_size( pGetDoorLockStateRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_get_door_lock_state_rsp_ind__pack( pGetDoorLockStateRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_GET_DOOR_LOCK_STATE_RSP_IND, len, pBuf );
    
    free( pBuf );
    
    return ZSTATUS_VALUES__ZSuccess;
  }
  else
  {
    uiPrintf( "Sending Get Door Lock State Response Indication Failed, memory error\n" );
    
    return ZSTATUS_VALUES__ZMemError;
  }
}

/*********************************************************************
 * @fn      processDevThermostatSetpointChangeReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pThermostatSetpointChangeReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevThermostatSetpointChangeReq( int connection, DevThermostatSetpointChangeReq *pThermostatSetpointChangeReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Thermostat Setpoint Change Request\n" );
  
  // send to ZigBee
  status = sendDevThermostatSetpointChangeReq( pThermostatSetpointChangeReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_GENERIC, pThermostatSetpointChangeReq->dstaddress->addresstype, 
                  connection, pThermostatSetpointChangeReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_THERMOSTAT_SETPOINT_CHANGE_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Thermostat Setpoint Change Request Failed - Status: %d\n", status );
  }
}

/*********************************************************************
 * @fn      processDevWindowCoveringActionReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pWindowCoveringReq - pointer to received message structure
 *
 * @return  none
 */
static void processDevWindowCoveringActionReq( int connection, DevWindowCoveringActionReq *pWindowCoveringReq )
{
  ZStatus_t status;
  
  uiPrintf( "Processing Window Covering Action Request\n" );
  
  // send to ZigBee
  status = sendDevWindowCoveringActionReq( pWindowCoveringReq );

  // inform app with a confirm (and perhaps response)
  gwHandleCnfRsp( GW_RSP_GENERIC, pWindowCoveringReq->dstaddress->addresstype, 
                  connection, pWindowCoveringReq->dstaddress->ieeeaddr, 
                  GW_CMD_ID_T__DEV_WINDOW_COVERING_ACTION_REQ, status );

  // some debug code
  if ( status != ZSuccess )
  {
    uiPrintf( "Processing Window Covering Action Request Failed - Status: %d\n", status );
  }
}

/**************************************************************************************************
 *
 * @fn          processAfIncomingMsgInd
 *
 * @brief       Process AF Incoming Message Indication message
 *
 * @return      none
 *
 **************************************************************************************************/
static void processAfIncomingMsgInd( AfIncomingMsgInd *pInMsg )
{
  zclProcMsgStatus_t status;
  afIncomingMSGPacket_t pkt;
  uiPrintf( "Processing Af Incoming Message Indication\n" );

  if ( pInMsg->srcaddr->addrmode == AFADDR_MODE__EXT )
  {
    pkt.srcAddr.addr.extAddr[0] = BREAK_UINT64( pInMsg->srcaddr->extaddr, 0 );
    pkt.srcAddr.addr.extAddr[1] = BREAK_UINT64( pInMsg->srcaddr->extaddr, 1 );
    pkt.srcAddr.addr.extAddr[2] = BREAK_UINT64( pInMsg->srcaddr->extaddr, 2 );
    pkt.srcAddr.addr.extAddr[3] = BREAK_UINT64( pInMsg->srcaddr->extaddr, 3 );
    pkt.srcAddr.addr.extAddr[4] = BREAK_UINT64( pInMsg->srcaddr->extaddr, 4 );
    pkt.srcAddr.addr.extAddr[5] = BREAK_UINT64( pInMsg->srcaddr->extaddr, 5 );
    pkt.srcAddr.addr.extAddr[6] = BREAK_UINT64( pInMsg->srcaddr->extaddr, 6 );
    pkt.srcAddr.addr.extAddr[7] = BREAK_UINT64( pInMsg->srcaddr->extaddr, 7 );
  }
  else
  {
     pkt.srcAddr.addr.shortAddr = pInMsg->srcaddr->shortaddr;
  }
  
  pkt.srcAddr.addrMode = pInMsg->srcaddr->addrmode;
  pkt.srcAddr.endPoint = pInMsg->srcaddr->endpoint;
  pkt.srcAddr.panId = pInMsg->srcaddr->panid;
  pkt.groupId = pInMsg->groupid;
  pkt.clusterId = pInMsg->clusterid;
  pkt.macDestAddr = pInMsg->macdestaddr;
  pkt.endPoint = pInMsg->endpoint;
  pkt.wasBroadcast = pInMsg->wasbroadcast;
  pkt.SecurityUse = pInMsg->securityuse;
  pkt.LinkQuality = pInMsg->linkquality;
  pkt.correlation = pInMsg->correlation;
  pkt.rssi = pInMsg->rssi;
  pkt.timestamp = pInMsg->timestamp;
  pkt.nwkSeqNum = pInMsg->nwkseqnum;
  pkt.macSrcAddr = pInMsg->macsrcaddr;
  pkt.cmd.TransSeqNumber = pInMsg->transseqnum;
  pkt.cmd.DataLength = pInMsg->payload.len;
  pkt.cmd.Data = pInMsg->payload.data;

  status = zcl_ProcessMessageMSG( &pkt );
  
  if ( status != ZCL_PROC_SUCCESS )
  {
    uiPrintf( "ZCL Process Message Failed, Message Status: %d\n", status );
  }   
}

/**************************************************************************************************
 *
 * @fn          processGpSecReqIncomingMsgInd
 *
 * @brief       Process Green Power Security Request Incoming Message Indication
 *
 * @return      none
 *
 **************************************************************************************************/
static void processGpSecReqIncomingMsgInd( GpSecReqIncomingMsgInd *pInMsg )
{
  uint8_t status;
  gp_SecReq_t pkt;

  uiPrintf( "Processing Green Power Security Request Incoming Message Indication\n" );

  pkt.gpd_ID.AppID = pInMsg->appid;

  if ( pInMsg->appid == GP_APP_ID_DEFAULT )
  {
    pkt.gpd_ID.GPDId.SrcID = pInMsg->srcid;
  }
  else if ( pInMsg->appid == GP_APP_ID_GP )
  {
    memcpy(pkt.gpd_ID.GPDId.GPDExtAddr, &pInMsg->gpdextaddr, sizeof(uint64_t));
    pkt.EndPoint = pInMsg->endpoint;
  }

  pkt.gp_SecData.GPDFSecLvl = pInMsg->gpdfseclvl;
  pkt.gp_SecData.GPDFKeyType = pInMsg->gpdfkeytype;
  pkt.gp_SecData.GPDSecFrameCounter = pInMsg->gpdsecframecounter;
  pkt.dGPStubHandle = pInMsg->dgpstubhandle;

  status = GP_SecReq( &pkt );

  if ( status != SUCCESS )
  {
    uiPrintf( "ZCL Process Message Failed, Message Status: %d\n", status );
  }
}

/**************************************************************************************************
 *
 * @fn          processGpDataIndIncomingMsgInd
 *
 * @brief       Process Green Power Data Indication Incoming Message Indication
 *
 * @return      none
 *
 **************************************************************************************************/
static void processGpDataIndIncomingMsgInd( GpDataIndIncomingMsgInd *pInMsg )
{
  uint8_t status;
  gp_DataInd_t pkt;

  uiPrintf( "Processing Green Power Data Indication Incoming Message Indication\n" );

  pkt.status = pInMsg->status;
  pkt.Rssi = pInMsg->rssi;
  pkt.LinkQuality = pInMsg->linkquality;
  pkt.SeqNumber = pInMsg->seqnumber;
  pkt.srcAddr.addrMode = pInMsg->addrmode;
  pkt.srcPanID = pInMsg->srcpanid;

  if( pkt.srcAddr.addrMode == SADDR_MODE_SHORT )
  {
    pkt.srcAddr.addr.shortAddr = pInMsg->shortaddr;
  }
  else if( pkt.srcAddr.addrMode == SADDR_MODE_EXT )
  {
    memcpy(pkt.srcAddr.addr.extAddr, &pInMsg->extaddr, sizeof(uint64_t));
  }

  pkt.appID = pInMsg->appid;
  pkt.GPDFSecLvl = pInMsg->gpdfseclvl;
  pkt.GPDFKeyType = pInMsg->gpdfkeytype;
  pkt.AutoCommissioning = pInMsg->autocommissioning;
  pkt.RxAfterTx = pInMsg->rxaftertx;
  pkt.SrcId = pInMsg->srcid;
  pkt.EndPoint = pInMsg->endpoint;
  pkt.GPDSecFrameCounter = pInMsg->gpdsecframecounter;
  pkt.GPDCmmdID = pInMsg->gpdcmmdid;
  pkt.GPDasduLength = pInMsg->gpdasdu.len;
  //pkt.GPDasdu = pInMsg->GPDasdu.data;
  pkt.MIC = pInMsg->mic;

  status = GP_DataInd( &pkt );

  if ( status != SUCCESS )
  {
    uiPrintf( "ZCL Process Message Failed, Message Status: %d\n", status );
  }
}

/**************************************************************************************************
 *
 * @fn          processAfDataConfirmInd
 *
 * @brief       Process AF Data Confirm Message Indication message
 *
 * @return      none
 *
 **************************************************************************************************/
static void processAfDataConfirmInd( AfDataConfirmInd *pInMsg )
{  
  uiPrintf( "Processing AF Data Confirmation Message: status: %d, sequence number: %d\n", pInMsg->status, pInMsg->transid );
}

/*********************************************************************
 * @fn      processZclReadAttributeRsp
 *
 * @brief   Handles incoming ZCL response protobuf messages.
 *
 * @param   pSrcAddress - protobuf source address information
 * @param   zclTransId - ZCL transaction ID
 * @param   clusterId - attribute list cluster ID
 * @param   payloadLen - length of pPayload
 * @param   pPayload - APS payload from incoming message indication
 *
 * @return  none
 */
static void processZclReadAttributeRsp( GwAddressStructT *pSrcAddress, uint8 zclTransId,
                                        uint16 clusterId, uint16 payloadLen, uint8 *pPayload )
{
  gsGwMsgTransTable_t *pTransEntry = NULL;
  
  uiPrintf( "Processing Read Attribute Response:" );
  uiPrintf( " SrcAddr %016llX, zclTransId %d, clusterId %d, payloadlen %d\n",
            pSrcAddress->ieeeaddr, zclTransId, clusterId, payloadLen );
  
  // See if response was for read attribute request
  // Get transaction entry
  pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__GW_READ_DEVICE_ATTRIBUTE_REQ );
  
  if ( pTransEntry )
  {
    GwReadDeviceAttributeRspInd readAttrRsp = GW_READ_DEVICE_ATTRIBUTE_RSP_IND__INIT;
    
    // Determine if response count needs to be handled
    gwRspCountHandler( pTransEntry );
    
    // Handle self-addressed messages
    gwSetSelfAddressStruct( pTransEntry->appTransId, pSrcAddress );
    
    readAttrRsp.sequencenumber = pTransEntry->appTransId;
    readAttrRsp.srcaddress = pSrcAddress;
    readAttrRsp.clusterid = clusterId;
    
    processGwReadDeviceAttributeRspInd( pTransEntry->connection, &readAttrRsp, payloadLen, pPayload );
    
    gwMsgRemoveTrans( pTransEntry->appTransId );
  }
  else
  {  
    // Check if response to cluster specific command
    switch ( clusterId )
    {
      case ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL:
        // Get transaction entry
        pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__DEV_GET_LEVEL_REQ );
        
        // Fall out if not found
        if ( pTransEntry )
        {
          DevGetLevelRspInd getLevelRsp = DEV_GET_LEVEL_RSP_IND__INIT;
        
          // Determine if response count needs to be handled
          gwRspCountHandler( pTransEntry );
          
          getLevelRsp.sequencenumber = pTransEntry->appTransId;
          getLevelRsp.srcaddress = pSrcAddress;
          
          processDevGetLevelRspInd( pTransEntry->connection, &getLevelRsp, payloadLen, pPayload );
          
          gwMsgRemoveTrans( pTransEntry->appTransId ); 
        }   
        break;     
    
      case ZCL_CLUSTER_ID_GEN_ON_OFF:
        // Get transaction entry
        pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__DEV_GET_ONOFF_STATE_REQ );
        
        // Fall out if not found
        if ( pTransEntry )
        {
          DevGetOnOffStateRspInd getOnOffStateRsp = DEV_GET_ON_OFF_STATE_RSP_IND__INIT;
        
          // Determine if response count needs to be handled
          gwRspCountHandler( pTransEntry );
          
          getOnOffStateRsp.sequencenumber = pTransEntry->appTransId;
          getOnOffStateRsp.srcaddress = pSrcAddress;
          
          processDevGetOnOffStateRspInd( pTransEntry->connection, &getOnOffStateRsp, payloadLen, pPayload );
          
          gwMsgRemoveTrans( pTransEntry->appTransId );  
        }
        break;
        
      case ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL:
        // Get transaction entry
        pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__DEV_GET_COLOR_REQ );
        
        // Fall out if not found
        if ( pTransEntry )
        {
          DevGetColorRspInd getColorRsp = DEV_GET_COLOR_RSP_IND__INIT;
        
          // Determine if response count needs to be handled
          gwRspCountHandler( pTransEntry );
          
          getColorRsp.sequencenumber = pTransEntry->appTransId;
          getColorRsp.srcaddress = pSrcAddress;
          
          processDevGetColorRspInd( pTransEntry->connection, &getColorRsp, payloadLen, pPayload );
          
          gwMsgRemoveTrans( pTransEntry->appTransId ); 
        } 
        else
        {
          pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__DEV_GET_COLOR_TEMP_REQ );

          if ( pTransEntry )
          {
            DevGetColorTempRspInd getColorTempRsp = DEV_GET_COLOR_TEMP_RSP_IND__INIT;

            // Determine if response count needs to be handled
            gwRspCountHandler( pTransEntry );

            getColorTempRsp.sequencenumber = pTransEntry->appTransId;
            getColorTempRsp.srcaddress = pSrcAddress;

            processDevGetColorTempRspInd( pTransEntry->connection, &getColorTempRsp, payloadLen, pPayload );

            gwMsgRemoveTrans( pTransEntry->appTransId );
          }
        }
        break;       
    
      case ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT:
        // Get transaction entry
        pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__DEV_GET_TEMP_REQ );
        
        // Fall out if not found
        if ( pTransEntry )
        {
          DevGetTempRspInd getTempRsp = DEV_GET_TEMP_RSP_IND__INIT;
          
          // Determine if response count needs to be handled
          gwRspCountHandler( pTransEntry );
          
          getTempRsp.sequencenumber = pTransEntry->appTransId;
          getTempRsp.srcaddress = pSrcAddress;
          
          processDevGetTempRspInd( pTransEntry->connection, &getTempRsp, payloadLen, pPayload );
          
          gwMsgRemoveTrans( pTransEntry->appTransId );
        }
        break;
    
      case ZCL_CLUSTER_ID_SE_SIMPLE_METERING:
        // Get transaction entry
        pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__DEV_GET_POWER_REQ );
          
        // Fall out if not found
        if ( pTransEntry )
        {
          DevGetPowerRspInd getPowerRsp = DEV_GET_POWER_RSP_IND__INIT;   
          
          // Determine if response count needs to be handled
          gwRspCountHandler( pTransEntry );
          
          getPowerRsp.sequencenumber = pTransEntry->appTransId;
          getPowerRsp.srcaddress = pSrcAddress;
          
          processDevGetPowerRspInd( pTransEntry->connection, &getPowerRsp, payloadLen, pPayload );
          
          gwMsgRemoveTrans( pTransEntry->appTransId );
        }
        break;
    
      case ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY:
        // Get transaction entry
        pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__DEV_GET_HUMIDITY_REQ );
        
        // Fall out if not found
        if ( pTransEntry )
        {
          DevGetHumidityRspInd getHumidityRsp = DEV_GET_HUMIDITY_RSP_IND__INIT;
          
          // Determine if response count needs to be handled
          gwRspCountHandler( pTransEntry );
          
          getHumidityRsp.sequencenumber = pTransEntry->appTransId;
          getHumidityRsp.srcaddress = pSrcAddress;
          
          processDevGetHumidityRspInd( pTransEntry->connection, &getHumidityRsp, payloadLen, pPayload );
          
          gwMsgRemoveTrans( pTransEntry->appTransId );
        }
        break;

      case ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK:
        // Get transaction entry
        pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__DEV_GET_DOOR_LOCK_STATE_REQ );
        
        // Fall out if not found
        if ( pTransEntry )
        {
          DevGetDoorLockStateRspInd doorLockStateRsp = DEV_GET_DOOR_LOCK_STATE_RSP_IND__INIT;
          
          // Determine if response count needs to be handled
          gwRspCountHandler( pTransEntry );
          
          doorLockStateRsp.sequencenumber = pTransEntry->appTransId;
          doorLockStateRsp.srcaddress = pSrcAddress;
          
          processDevGetDoorLockStateRspInd( pTransEntry->connection, &doorLockStateRsp, payloadLen, pPayload );
          
          gwMsgRemoveTrans( pTransEntry->appTransId );
        }
        break;
    
      default:
        uiPrintf( "Processing Read Attribute Response Failed:" );
        uiPrintf( " SrcAddr %016llX, zclTransId %d, clusterId %d, payloadlen %d\n",
                  pSrcAddress->ieeeaddr, zclTransId, clusterId, payloadLen );
        break;
    }
  }  
}

/*********************************************************************
 * @fn      processZclWriteAttributeRsp
 *
 * @brief   Handles incoming ZCL response protobuf messages.
 *
 * @param   pSrcAddress - protobuf source address information
 * @param   zclTransId - ZCL transaction ID
 * @param   clusterId - attribute list cluster ID
 * @param   payloadLen - length of pPayload
 * @param   pPayload - APS payload from incoming message indication
 *
 * @return  none
 */
static void processZclWriteAttributeRsp( GwAddressStructT *pSrcAddress, uint8 zclTransId,
                                         uint16 clusterId, uint16 payloadLen, uint8 *pPayload )
{
  gsGwMsgTransTable_t *pTransEntry;
  
  uiPrintf( "Processing Write Attribute Response\n" );
  
  // Get transaction entry
  pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__GW_WRITE_DEVICE_ATTRIBUTE_REQ );
    
  if ( pTransEntry )
  {
    GwWriteDeviceAttributeRspInd writeAttrRsp = GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND__INIT;
    
    // Determine if response count needs to be handled
    gwRspCountHandler( pTransEntry );
    
    // Handle self-addressed messages
    gwSetSelfAddressStruct( pTransEntry->appTransId, pSrcAddress );
    
    writeAttrRsp.sequencenumber = pTransEntry->appTransId;
    writeAttrRsp.srcaddress = pSrcAddress;
    writeAttrRsp.clusterid = clusterId;
    
    processGwWriteDeviceAttributeRspInd( pTransEntry->connection, &writeAttrRsp, payloadLen, pPayload );
    
    // Remove transaction entry
    gwMsgRemoveTrans( pTransEntry->appTransId );
  }
}

/*********************************************************************
 * @fn      processZclConfigReportRsp
 *
 * @brief   Handles incoming ZCL response protobuf messages.
 *
 * @param   pSrcAddress - protobuf source address information
 * @param   zclTransId - ZCL transaction ID
 * @param   clusterId - attribute list cluster ID
 * @param   payloadLen - length of pPayload
 * @param   pPayload - APS payload from incoming message indication
 *
 * @return  none
 */
static void processZclConfigReportRsp( GwAddressStructT *pSrcAddress, uint8 zclTransId,
                                       uint16 clusterId, uint16 payloadLen, uint8 *pPayload )
{
  gsGwMsgTransTable_t *pTransEntry;
  
  uiPrintf( "Processing ZCL Configure Reporting Response\n" );
  
  // Get transaction entry
  pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__GW_SET_ATTRIBUTE_REPORTING_REQ );
    
  if ( pTransEntry )
  {
    GwSetAttributeReportingRspInd setAttrReportingRsp = GW_SET_ATTRIBUTE_REPORTING_RSP_IND__INIT;
    
    // Determine if response count needs to be handled
    gwRspCountHandler( pTransEntry );
    
    setAttrReportingRsp.sequencenumber = pTransEntry->appTransId;
    setAttrReportingRsp.srcaddress = pSrcAddress;
    setAttrReportingRsp.clusterid = clusterId;
    
    processGwSetAttributeReportingRspInd( pTransEntry->connection, &setAttrReportingRsp, payloadLen, pPayload );
    
    // Remove transaction entry
    gwMsgRemoveTrans( pTransEntry->appTransId );
  }
}

/*********************************************************************
 * @fn      processGenericResponseTriggers
 *
 * @brief   Handles incoming ZCL default response protobuf messages.
 *
 * @param   pSrcAddress - protobuf source address information 
 * @param   zclTransId - ZCL transaction ID 
 * @param   payloadLen - length of pPayload
 * @param   pPayload - APS payload from incoming message indication
 *
 * @return  none
 */
static void processGenericResponseTriggers( uint8 zclTransId, uint8 status, char * zclCommandString )
{
  gsGwMsgTransTable_t *pTransEntry;
  GwZigbeeGenericRspInd genericRsp = GW_ZIGBEE_GENERIC_RSP_IND__INIT;
  
  uiPrintf( "Processing ZCL %s\n", zclCommandString );
  
  // Get transaction entry
  pTransEntry = gwMsgTransGetByZclTransIdGenericRsp( zclTransId );
  
  // Check if expecting generic response
  if ( pTransEntry )
  {
    genericRsp.sequencenumber = pTransEntry->appTransId;
    
    // Check status byte
    if ( status == ZCL_STATUS_SUCCESS )
    {
      genericRsp.status = GW_STATUS_T__STATUS_SUCCESS;
    }
    else
    {
      genericRsp.status = GW_STATUS_T__STATUS_FAILURE;
    }
    
    sendZbGenericRspInd( pTransEntry->connection, &genericRsp );
    
    // Clear transaction entry
    gwMsgRemoveTrans( pTransEntry->appTransId );
  }
  else
  {
    uiPrintf( "Failed processing %s, unsupported transaction - zclTransId: %d\n", zclCommandString, zclTransId );
  }
}

/*********************************************************************
 * @fn      processZclDiscoverAttributesRsp
 *
 * @brief   Handles incoming ZCL response protobuf messages.
 *
 * @param   pSrcAddress - protobuf source address information
 * @param   zclTransId - ZCL transaction ID
 * @param   clusterId - attribute list cluster ID
 * @param   payloadLen - length of pPayload
 * @param   pPayload - APS payload from incoming message indication
 *
 * @return  none
 */
static void processZclDiscoverAttributesRsp( GwAddressStructT *pSrcAddress, uint8 zclTransId,
                                             uint16 clusterId, uint16 payloadLen, uint8 *pPayload )
{
  uint8 i;
  uint8 numAttrs;
  gsGwMsgTransTable_t *pTransEntry;
  zclDiscoverAttrsRspCmd_t *pDiscAttrsRsp;
  GwGetDeviceAttributeListRspInd getAttrListRsp = GW_GET_DEVICE_ATTRIBUTE_LIST_RSP_IND__INIT;
  
  uiPrintf( "Processing Discover Attributes Response\n" );
  
  // Get transaction entry
  pTransEntry = gwMsgTransGetByZclTransId( zclTransId, GW_CMD_ID_T__GW_GET_DEVICE_ATTRIBUTE_LIST_REQ );
    
  if ( pTransEntry )
  {
    numAttrs = (payloadLen - 1) / (2 + 1); // (data len - Discovery Complete) / Attr ID + Data Type

    pDiscAttrsRsp = malloc( sizeof ( zclDiscoverAttrsRspCmd_t ) + (numAttrs * sizeof(zclDiscoverAttrInfo_t)) );
    if ( !pDiscAttrsRsp )
    {
      return; // memory error  
    }
    
    pDiscAttrsRsp->discComplete = *pPayload++;
    pDiscAttrsRsp->numAttr = numAttrs;

    for ( i = 0; i < numAttrs; i++ )
    {
      pDiscAttrsRsp->attrList[i].attrID = BUILD_UINT16( pPayload[0], pPayload[1] );
      pPayload += 2;
      pDiscAttrsRsp->attrList[i].dataType = *pPayload++;
    }
    
    // Handle self-addressed messages
    gwSetSelfAddressStruct( pTransEntry->appTransId, pSrcAddress );
    
    // Fill in protobuf information
    getAttrListRsp.sequencenumber = pTransEntry->appTransId;
    getAttrListRsp.srcaddress = pSrcAddress;
    
    // Clear previous transaction entry
    gwMsgRemoveTrans( pTransEntry->appTransId ); 
    
    // Update state machine, handles sending response back to app
    if ( GW_SERVICES_PROCESSING == gwServices_UpdateStateMachine_DeviceAttrList( &getAttrListRsp, 
                                                                                 pTransEntry->appTransId, clusterId, 
                                                                                 pDiscAttrsRsp ) )
    {
      // Post new transaction
      gwMsgTransPost( FALSE, GW_RSP_SPECIFIC, pTransEntry->connection, pSrcAddress->ieeeaddr, 
                      GW_CMD_ID_T__GW_GET_DEVICE_ATTRIBUTE_LIST_REQ, 
                      pTransEntry->appTransId, (zcl_TransID - 1), giGwDeviceTimeout );      
    }  
                          
    // Free memory                                                  
    free( pDiscAttrsRsp );                                                
  }
}                                       

/**************************************************************************************************
 *
 * @fn          sendAPICExpectDefaultStatus
 *
 * @brief       Send a request message and expect the normal "default" response (MacDefaultRsp)
 *
 * @param       cmdID - messages command ID
 * @param       len - length of pData buffer to send
 * @param       pData - pointer to buffer to send
 *
 * @return      synchronous return status
 *
 **************************************************************************************************/
ZStatusValues sendAPICExpectDefaultStatus( int cmdID, int len, uint8 *pData )
{
  uint8 rspCmdId;
  uint8 *pRsp;  
  uint16 rspLen;
  ZstackDefaultRsp *pDefaultRsp;
  ZStatusValues status = ZSTATUS_VALUES__ZDecodeError;

  // send serialized request to API Client synchronously
  pRsp = apicSendSynchData( GW_ZSTACK_HANDLE, ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF, 
                            cmdID, len, pData,
                            NULL, &rspCmdId, &rspLen );

  if ( pRsp )
  {
    if ( (ZSTACK_CMD_IDS__ZSTACK_DEFAULT_RSP == rspCmdId) && (rspLen > 0) )
    {
      pDefaultRsp = zstack_default_rsp__unpack( NULL, rspLen, pRsp );
      if ( pDefaultRsp )
      {
        if (pDefaultRsp->cmdid == cmdID)
        {
          status = pDefaultRsp->status;
        }
        zstack_default_rsp__free_unpacked( pDefaultRsp, NULL );
      }
    }
    else
    {
      uiPrintfEx(trERROR, "Expected ZstackDefaultResponse, got %d\n", rspCmdId );
    }
	
    apicFreeSynchData( pRsp );
  }

  // return the status
  return (status);
}

/**************************************************************************************************
 *
 * @fn          sendZbGenericCnf
 *
 * @brief       Send ZigBee Generic Confirmation message
 *
 * @param       connection - connection handle (tcp)
 * @param       pGenericCnf - generic confirmation command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static void sendZbGenericCnf( int connection, GwZigbeeGenericCnf *pGenericCnf )
{
  int len;
  uint8 *pBuf;

  uiPrintf( "Sending ZigBee Generic Confirmation \n" );

  len = gw_zigbee_generic_cnf__get_packed_size( pGenericCnf );
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_zigbee_generic_cnf__pack( pGenericCnf, pBuf );

    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, FALSE, 
                   GW_CMD_ID_T__ZIGBEE_GENERIC_CNF, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          sendZbGenericRspInd
 *
 * @brief       Send ZigBee Generic Response Indication message
 *
 * @param       connection - connection handle (tcp)
 * @param       pGenericRsp - generic response indication command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static void sendZbGenericRspInd( int connection, GwZigbeeGenericRspInd *pGenericRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Generic Response Indication: Status: %d, SequenceNumber: %d\n", pGenericRsp->status, pGenericRsp->sequencenumber );

  len = gw_zigbee_generic_rsp_ind__get_packed_size( pGenericRsp );
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_zigbee_generic_rsp_ind__pack( pGenericRsp, pBuf );

    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__ZIGBEE_GENERIC_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          sendGwAddGroupReq
 *
 * @brief       Send Add Group Request message
 *
 * @param       pAddGroupReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwAddGroupReq( GwAddGroupReq *pAddGroupReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Add Group Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pAddGroupReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return zclGeneral_SendGroupAdd( srvParseGwEndpointId(pAddGroupReq->dstaddress), &dstAddr, 
                                    (uint16)pAddGroupReq->groupid, (uint8 *)pAddGroupReq->groupname, 
                                    disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }
}


/**************************************************************************************************
 *
 * @fn          sendGwGetGroupMembershipReq
 *
 * @brief       Send Get Group Membership Request message
 *
 * @param       pGetGroupMembershipReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwGetGroupMembershipReq( GwGetGroupMembershipReq *pGetGroupMembershipReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Add Group Membership Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pGetGroupMembershipReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return zclGeneral_SendGroupGetMembership( srvParseGwEndpointId(pGetGroupMembershipReq->dstaddress), &dstAddr, 0, NULL, 
                                              disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }
}

/**************************************************************************************************
 *
 * @fn          sendGwGetGroupMembershipRspInd
 *
 * @brief       Send Get Group Membership Response Indication protobuf message to the app 
 *
 * @param       connection - connection handle (tcp)
 * @param       pGetGroupMbrRsp - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static ZStatusValues sendGwGetGroupMembershipRspInd( int connection, GwGetGroupMembershipRspInd *pGetGroupMbrRsp )
{
  uint8 *pBuf;
  int len;
  
  uiPrintf( "Sending Get Group Membership Response Indication - Status: %d, App TransId: %d\n", 
            pGetGroupMbrRsp->status, pGetGroupMbrRsp->sequencenumber );
  
  len = gw_get_group_membership_rsp_ind__get_packed_size( pGetGroupMbrRsp );
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_get_group_membership_rsp_ind__pack( pGetGroupMbrRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_GET_GROUP_MEMBERSHIP_RSP_IND, len, pBuf );
                   
    // Free memory
    free( pBuf );                  
  }
  else
  {
    uiPrintf( "Sending Get Group Membership Response Indication Failed - memory error\n" );
    return ZSTATUS_VALUES__ZMemError;
  }
  
  return ( ZSTATUS_VALUES__ZSuccess );
}

/**************************************************************************************************
 *
 * @fn          sendGwRemoveFromGroupReq
 *
 * @brief       Send Remove From Group Request message
 *
 * @param       pRemoveFromGroupReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwRemoveFromGroupReq( GwRemoveFromGroupReq *pRemoveFromGroupReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Remove From Group Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pRemoveFromGroupReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    // Determine group(s) to remove
    if ( pRemoveFromGroupReq->has_groupid )
    {
      return zclGeneral_SendGroupRemove( srvParseGwEndpointId(pRemoveFromGroupReq->dstaddress), &dstAddr, pRemoveFromGroupReq->groupid, disableDefaultRsp, zcl_TransID );
    }
    else
    {
      return zclGeneral_SendGroupRemoveAll( srvParseGwEndpointId(pRemoveFromGroupReq->dstaddress), &dstAddr, disableDefaultRsp, zcl_TransID );
    }
  }
  else
  {
    return ZInvalidParameter;
  }
}


/**************************************************************************************************
 *
 * @fn          sendGwStoreSceneReq
 *
 * @brief       Send Store Scene Request message
 *
 * @param       pStoreSceneReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwStoreSceneReq( GwStoreSceneReq *pStoreSceneReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Store Scene Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pStoreSceneReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return zclGeneral_SendSceneStore( srvParseGwEndpointId(pStoreSceneReq->dstaddress), &dstAddr, (uint16)pStoreSceneReq->groupid,
                                      (uint8)pStoreSceneReq->sceneid, disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }
}


/**************************************************************************************************
 *
 * @fn          sendGwRemoveSceneReq
 *
 * @brief       Send Remove Scene Request message
 *
 * @param       pRemoveSceneReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwRemoveSceneReq( GwRemoveSceneReq *pRemoveSceneReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Remove Scene Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pRemoveSceneReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return zclGeneral_SendSceneRemove( srvParseGwEndpointId(pRemoveSceneReq->dstaddress), &dstAddr, (uint16)pRemoveSceneReq->groupid,
                                       (uint8)pRemoveSceneReq->sceneid, disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }
}


/**************************************************************************************************
 *
 * @fn          sendGwRecallSceneReq
 *
 * @brief       Send Recall Scene Request message
 *
 * @param       pRecallSceneReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwRecallSceneReq( GwRecallSceneReq *pRecallSceneReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Recall Scene Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pRecallSceneReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return zclGeneral_SendSceneRecall( srvParseGwEndpointId(pRecallSceneReq->dstaddress), &dstAddr, (uint16)pRecallSceneReq->groupid,
                                       (uint8)pRecallSceneReq->sceneid, disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }
}

/**************************************************************************************************
 *
 * @fn          sendGetSceneMembershipReq
 *
 * @brief       Send Get Scene Membership Request message
 *
 * @param       pGetSceneMembershipReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGetSceneMembershipReq( GwGetSceneMembershipReq *pGetSceneMembershipReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Get Scene Membership Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pGetSceneMembershipReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return zclGeneral_SendSceneGetMembership( srvParseGwEndpointId(pGetSceneMembershipReq->dstaddress), &dstAddr,
                                              (uint16)pGetSceneMembershipReq->groupid, 
                                              disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }                                            
}

/**************************************************************************************************
 *
 * @fn          sendGwGetSceneMembershipRspInd
 *
 * @brief       Send Get Scene Membership Response Indication protobuf message to the app 
 *
 * @param       connection - connection handle (tcp)
 * @param       pGetSceneMbrRsp - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static ZStatusValues sendGwGetSceneMembershipRspInd( int connection, GwGetSceneMembershipRspInd *pGetSceneMbrRsp )
{
  uint8 *pBuf;
  int len;
  
  uiPrintf( "Sending Get Scene Membership Response Indication - Status: %d, App TransId: %d\n", 
            pGetSceneMbrRsp->status, pGetSceneMbrRsp->sequencenumber );
  
  len = gw_get_scene_membership_rsp_ind__get_packed_size( pGetSceneMbrRsp );
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_get_scene_membership_rsp_ind__pack( pGetSceneMbrRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_GET_SCENE_MEMBERSHIP_RSP_IND, len, pBuf );
                   
    // Free memory
    free( pBuf );                   
  }
  else
  {
    uiPrintf( "Sending Get Scene Membership Response Indication Failed - memory error\n" );
    return ZSTATUS_VALUES__ZMemError;
  }
  
  return ( ZSTATUS_VALUES__ZSuccess );
}

/**************************************************************************************************
 *
 * @fn          sendGwSleepyDeviceCheckInInd
 *
 * @brief       Send Sleepy Device Check-In Indication message
 *
 * @param       pSleepyDevCheckInInd - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static ZStatusValues sendGwSleepyDeviceCheckInInd( GwSleepyDeviceCheckInInd *pSleepyDevCheckInInd )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZSuccess;
  
  uiPrintf( "Sending Sleepy Device Check-In Indication\n" );
  
  len = gw_sleepy_device_check_in_ind__get_packed_size( pSleepyDevCheckInInd );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_sleepy_device_check_in_ind__pack( pSleepyDevCheckInInd, pBuf );
    
    // Send response back to app(s)
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_SLEEPY_DEVICE_CHECK_IN_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Sleepy Device Check-In Indication Failed, memory error\n" );
    
    status = ZSTATUS_VALUES__ZMemError;
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendGwAttributeChangeInd
 *
 * @brief       Send Attribute Change Indication message
 *
 * @param       pAttrChangeInd - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendGwAttributeChangeInd( GwAttributeChangeInd *pAttrChangeInd )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Attribute Change Indication\n" );
  
  len = gw_attribute_change_ind__get_packed_size( pAttrChangeInd );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_attribute_change_ind__pack( pAttrChangeInd, pBuf );
    
    // Send response back to app(s)
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_ATTRIBUTE_CHANGE_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Attribute Change Indication Failed, memory error\n" );
  }
}

/**************************************************************************************************
 *
 * @fn          sendGwGetDeviceAttributeListReq
 *
 * @brief       Send Get Device Attribute List Request message
 *
 * @param       connection - connection handle (tcp)
 * @param       pGetDeviceAttrListReq - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
ZStatus_t sendGwGetDeviceAttributeListReq( int connection, GwGetDeviceAttributeListReq *pGetDeviceAttrListReq, uint16 clusterId, uint16 startAttr )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_NONE; // valid addr mode(s) 
  zclDiscoverAttrsCmd_t discoverAttrs;
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Get Device Attribute List Request\n" );

  if ( gwConvertAddrPbToAfReq( pGetDeviceAttrListReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    discoverAttrs.startAttr = startAttr;
    discoverAttrs.maxAttrIDs = 0xFF;
    
    return zcl_SendDiscoverAttrsCmd( srvParseGwEndpointId(pGetDeviceAttrListReq->dstaddress), &dstAddr,
                                     clusterId, &discoverAttrs,
                                     disableDefaultRsp, disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }
}

/**************************************************************************************************
 *
 * @fn          sendGwGetDeviceAttributeListRspInd
 *
 * @brief       Send Get Device Attribute List Response Indication message
 *
 * @param       pGetAttrListRsp - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
void sendGwGetDeviceAttributeListRspInd( int connection, GwGetDeviceAttributeListRspInd *pGetAttrListRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Get Device Attribute List Response Indication\n" );
  
  len = gw_get_device_attribute_list_rsp_ind__get_packed_size( pGetAttrListRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_get_device_attribute_list_rsp_ind__pack( pGetAttrListRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_GET_DEVICE_ATTRIBUTE_LIST_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          sendGwReadDeviceAttributeReq
 *
 * @brief       Send Read Device Attribute Request message
 *
 * @param       pReadDeviceAttrReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwReadDeviceAttributeReq( GwReadDeviceAttributeReq *pReadDeviceAttrReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST + ADDR_MODE_NONE; // valid addr mode(s) 
  ZStatus_t status = ZInvalidParameter;
  afAddrType_t dstAddr;
  zclReadCmd_t *pReadCmd;
  
  uiPrintf( "Sending Read Device Attribute Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pReadDeviceAttrReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    pReadCmd = malloc( sizeof( zclReadCmd_t ) + (pReadDeviceAttrReq->n_attributelist * sizeof( uint16 )) );
    if ( pReadCmd )
    {
      uint8 i;
      
      pReadCmd->numAttr = pReadDeviceAttrReq->n_attributelist;
      
      for ( i = 0; i < pReadDeviceAttrReq->n_attributelist; i++ )
      {
        pReadCmd->attrID[i] = pReadDeviceAttrReq->attributelist[i];
      }
      
      status = zcl_SendRead( srvParseGwEndpointId(pReadDeviceAttrReq->dstaddress), &dstAddr,
                             pReadDeviceAttrReq->clusterid, pReadCmd,
                             pReadDeviceAttrReq->isservertoclient ? ZCL_FRAME_SERVER_CLIENT_DIR : ZCL_FRAME_CLIENT_SERVER_DIR,
                             disableDefaultRsp, zcl_TransID );
                             
                             
      free( pReadCmd );                           
    }
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendGwReadDeviceAttributeRspInd
 *
 * @brief       Send Read Device Attribute Response Indication message
 *
 * @param       pReadDeviceAttrRsp - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendGwReadDeviceAttributeRspInd( int connection, GwReadDeviceAttributeRspInd *pReadDeviceAttrRsp )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Read Device Attribute Response Indication\n" );
  
  len = gw_read_device_attribute_rsp_ind__get_packed_size( pReadDeviceAttrRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_read_device_attribute_rsp_ind__pack( pReadDeviceAttrRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_READ_DEVICE_ATTRIBUTE_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          sendGwWriteDeviceAttributeReq
 *
 * @brief       Send Write Device Attribute Request message
 *
 * @param       pWriteDeviceAttrReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwWriteDeviceAttributeReq( GwWriteDeviceAttributeReq *pWriteDeviceAttrReq )
{
  bool disableDefaultRsp;
  uint8 *pAttrData;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST + ADDR_MODE_NONE; // valid addr mode(s) 
  int i;
  int attrDataSize = 0;
  ZStatus_t status = ZInvalidParameter;
  afAddrType_t dstAddr;
  zclWriteCmd_t *pWriteCmd;
  
  uiPrintf( "Sending Write Device Attribute Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pWriteDeviceAttrReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    if ( pWriteDeviceAttrReq->n_attributerecordlist )
    {
      // Add up data size for all attribute data
      for ( i = 0; i < pWriteDeviceAttrReq->n_attributerecordlist; i++ )
      {
        attrDataSize += pWriteDeviceAttrReq->attributerecordlist[i]->attributevalue.len;
      }
    }
      
    if ( attrDataSize )
    {     
      pWriteCmd = malloc( sizeof( zclWriteCmd_t ) + (pWriteDeviceAttrReq->n_attributerecordlist 
                          * sizeof( zclWriteRec_t )) );
      if ( !pWriteCmd )
      {
        return ZSTATUS_VALUES__ZMemError;
      }

      pWriteCmd->numAttr = pWriteDeviceAttrReq->n_attributerecordlist;
      
      pAttrData = malloc( attrDataSize );
      if ( !pAttrData )
      {
        free( pWriteCmd );
      
        return ZSTATUS_VALUES__ZMemError;
      }
      
      for ( i = 0; i < pWriteDeviceAttrReq->n_attributerecordlist; i++ )
      {      
        pWriteCmd->attrList[i].attrID = pWriteDeviceAttrReq->attributerecordlist[i]->attributeid;
        pWriteCmd->attrList[i].dataType = pWriteDeviceAttrReq->attributerecordlist[i]->attributetype;
        pWriteCmd->attrList[i].attrData = pAttrData;

        memcpy( pAttrData, pWriteDeviceAttrReq->attributerecordlist[i]->attributevalue.data, pWriteDeviceAttrReq->attributerecordlist[i]->attributevalue.len );
        
        pAttrData += pWriteDeviceAttrReq->attributerecordlist[i]->attributevalue.len;
      }
      
      // Send Request
      status = zcl_SendWriteRequest( srvParseGwEndpointId(pWriteDeviceAttrReq->dstaddress), &dstAddr,
                                     pWriteDeviceAttrReq->clusterid, pWriteCmd, ZCL_CMD_WRITE,
                                     ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );
          
      // Free data                    
      free( pWriteCmd->attrList[0].attrData );
      free( pWriteCmd );                 
    }
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendGwWriteDeviceAttributeRspInd
 *
 * @brief       Send Write Device Attribute Response message to the app
 *
 * @param       pWriteAttrRsp - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendGwWriteDeviceAttributeRspInd( int connection, GwWriteDeviceAttributeRspInd *pWriteAttrRsp )
{
  uint8 *pBuf;
  int len;
  
  len = gw_write_device_attribute_rsp_ind__get_packed_size( pWriteAttrRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_write_device_attribute_rsp_ind__pack( pWriteAttrRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Write Attribute Record Response Failed, memory error \n" );
  }
}

/**************************************************************************************************
 *
 * @fn          sendGwSetAttributeReportingReq
 *
 * @brief       Send Set Attribute Reporting Request message
 *
 * @param       pSetAttrReportReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwSetAttributeReportingReq( GwSetAttributeReportingReq *pSetAttrReportReq )
{
  uint8 i;
  uint8 index = 0;
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  uint8 *pReportChangeFieldBuf = NULL;
  int bufLen = 0; // used to allocate memory for reportable change field
  afAddrType_t dstAddr;
  zclCfgReportCmd_t *pCfgReportCmd;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Set Attribute Reporting Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pSetAttrReportReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    if ( !pSetAttrReportReq->n_attributereportlist )
    {
      return ZInvalidParameter;
    }
    
    // Find total buffer length for allocating pReportChangeField memory
    for ( i = 0; i < pSetAttrReportReq->n_attributereportlist; i++ )
    {
      if ( zclAnalogDataType( pSetAttrReportReq->attributereportlist[i]->attributetype ) )
      {
        // Check to see if reportable change field was included
        if ( pSetAttrReportReq->attributereportlist[i]->n_reportablechange )
        {
          // Add length for analog data type
          bufLen += gwAnalogDataType( pSetAttrReportReq->attributereportlist[i]->attributetype );
        }
        else
        {
          // Invalid parameter, reportable change field should be listed for analog data types
          return ZInvalidParameter;
        }
      }
    }
    
    pCfgReportCmd = malloc( sizeof( zclCfgReportCmd_t ) + (sizeof( zclCfgReportRec_t ) 
                            * pSetAttrReportReq->n_attributereportlist) );
                            
    if ( !pCfgReportCmd )
    {
      return ZMemError;
    }
    
    if ( bufLen )
    {
      pReportChangeFieldBuf = malloc( bufLen );       
    
      if ( !pReportChangeFieldBuf )
    {
        return ZMemError;
    }
    }
    
    pCfgReportCmd->numAttr = pSetAttrReportReq->n_attributereportlist;
    
    for ( i = 0; i < pSetAttrReportReq->n_attributereportlist; i++ )
    {         
      pCfgReportCmd->attrList[i].direction = 0;  // 0x00 is the default reporting configuration setting                        
      pCfgReportCmd->attrList[i].attrID = pSetAttrReportReq->attributereportlist[i]->attributeid;
      pCfgReportCmd->attrList[i].dataType = pSetAttrReportReq->attributereportlist[i]->attributetype;
      pCfgReportCmd->attrList[i].minReportInt = pSetAttrReportReq->attributereportlist[i]->minreportinterval;
      pCfgReportCmd->attrList[i].maxReportInt = pSetAttrReportReq->attributereportlist[i]->maxreportinterval;
      if ( pSetAttrReportReq->attributereportlist[i]->n_reportablechange )
      {
        // Store length for analog data type
        bufLen = gwAnalogDataType( pSetAttrReportReq->attributereportlist[i]->attributetype );
        
        if ( bufLen )
        {
          pCfgReportCmd->attrList[i].reportableChange = &pReportChangeFieldBuf[index];
          memcpy( &pReportChangeFieldBuf[index], (pSetAttrReportReq->attributereportlist[i]->reportablechange), bufLen );
          index += bufLen;
        }
      }
    }
    
    status = zcl_SendConfigReportCmd( srvParseGwEndpointId(pSetAttrReportReq->dstaddress), &dstAddr, pSetAttrReportReq->clusterid, pCfgReportCmd,
                                      ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );
    
    // Free memory
    if ( pReportChangeFieldBuf )
    {
      free( pReportChangeFieldBuf );
    }
    
    free( pCfgReportCmd );
  }
  
  return status;
}

/*********************************************************************
 * @fn      sendGwSetAttributeReportingRspInd
 *
 * @brief   Send Set Attribute Reporting response to the app
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetAttrReportingRsp - pointer to received message structure
 *
 * @return  none
 */
static void sendGwSetAttributeReportingRspInd( int connection,
        GwSetAttributeReportingRspInd *pSetAttrReportingRsp )
{
  uint8 *pBuf;
  int len;
  
  len = gw_set_attribute_reporting_rsp_ind__get_packed_size(
        pSetAttrReportingRsp );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_set_attribute_reporting_rsp_ind__pack( pSetAttrReportingRsp, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_SET_ATTRIBUTE_REPORTING_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Set Attribute Reporting Response failed, "
        "memory error \n" );
  }
}

/*********************************************************************
 * @fn      sendGwAttributeReportingInd
 *
 * @brief   Send Attribute Reporting Indication to the app
 *
 * @param   pSetAttrReportingRsp - pointer to received message structure
 *
 * @return  none
 */
static void sendGwAttributeReportingInd( GwAttributeReportingInd *pAttrReportInd )
{
  uint8 *pBuf;
  int len;
  
  len = gw_attribute_reporting_ind__get_packed_size( pAttrReportInd );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_attribute_reporting_ind__pack( pAttrReportInd, pBuf );
    
    // Send response back to app
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_ATTRIBUTE_REPORTING_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Attribute Reporting Indication failed, memory error \n" );
  }
}

/**************************************************************************************************
 *
 * @fn          sendGwSendZclFrameReq
 *
 * @brief       Send ZCL Frame Request message
 *
 * @param       pSendZclFrameReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwSendZclFrameReq( GwSendZclFrameReq *pSendZclFrameReq )
{
  uint8 transSeqNum = zcl_TransID;
  uint8 endpointId = 0xFF;  // default
  uint16 mfrCode = 0;
  afAddrType_t dstAddr;
  gsGwMsgTransTable_t *pTransEntry;
  ZStatus_t status;
  uint8 requestedOptions = 0;
  uint8 originalOptions;
  
  uiPrintf( "Sending ZCL Frame Request\n" );
  
  if ( pSendZclFrameReq->dstaddress->addresstype == GW_ADDRESS_TYPE_T__UNICAST )
  {
#if IEEE_ADDR_PRIORITY    
    dstAddr.addr.extAddr[0] = BREAK_UINT64( pSendZclFrameReq->dstaddress->ieeeaddr, 0 );
    dstAddr.addr.extAddr[1] = BREAK_UINT64( pSendZclFrameReq->dstaddress->ieeeaddr, 1 );
    dstAddr.addr.extAddr[2] = BREAK_UINT64( pSendZclFrameReq->dstaddress->ieeeaddr, 2 );
    dstAddr.addr.extAddr[3] = BREAK_UINT64( pSendZclFrameReq->dstaddress->ieeeaddr, 3 );
    dstAddr.addr.extAddr[4] = BREAK_UINT64( pSendZclFrameReq->dstaddress->ieeeaddr, 4 );
    dstAddr.addr.extAddr[5] = BREAK_UINT64( pSendZclFrameReq->dstaddress->ieeeaddr, 5 );
    dstAddr.addr.extAddr[6] = BREAK_UINT64( pSendZclFrameReq->dstaddress->ieeeaddr, 6 );
    dstAddr.addr.extAddr[7] = BREAK_UINT64( pSendZclFrameReq->dstaddress->ieeeaddr, 7 );
      
    dstAddr.addrMode = Addr64Bit;
#else
    if ( !gwPb_SrvrGetShortAddress( pSendZclFrameReq->dstaddress->ieeeaddr, 
                                    &(dstAddr.addr.shortAddr) ) )
    {
      return ZInvalidParameter;
    }

    dstAddr.addrMode = Addr16Bit;
#endif
  }
  else if ( pSendZclFrameReq->dstaddress->addresstype == GW_ADDRESS_TYPE_T__GROUPCAST )
  {
    dstAddr.addr.shortAddr = pSendZclFrameReq->dstaddress->groupaddr;
    dstAddr.addrMode = AddrGroup;
  }
  else
  {
    return ZInvalidParameter; // Incorrect address mode used
  }
  
  if ( pSendZclFrameReq->dstaddress->has_endpointid == TRUE )
  {
    endpointId = pSendZclFrameReq->dstaddress->endpointid;
  }
  
  dstAddr.endPoint = endpointId;

  // Check if response to previous request
  if ( pSendZclFrameReq->has_sequencenumber )
  {
    // Lookup transaction entry
    pTransEntry = gwMsgTransGetByAppTransId( pSendZclFrameReq->sequencenumber );
        
    // Verify transaction table entry
    if ( pTransEntry && (pTransEntry->cmdId == GW_CMD_ID_T__GW_ZCL_FRAME_RECEIVE_IND) )
    {
      transSeqNum = pTransEntry->zclTransId; 
    }
    else
    {
      uiPrintf( "Sending ZCL Frame Request Failure, no valid transaction entry - AppTransId: %d\n", 
                pSendZclFrameReq->sequencenumber ); 
      
      return ZFailure;
    }
  }
  else
  {
    transSeqNum = zcl_TransID;  // store new zcl transaction ID for request
  }
  
  // Handle manufacturer field
  if ( (pSendZclFrameReq->manufacturerspecificflag == GW_MFR_SPECIFIC_FLAG_T__MFR_SPECIFIC) && 
       pSendZclFrameReq->has_manufacturercode )
  {
    mfrCode = (uint16)pSendZclFrameReq->manufacturercode;
  }
  
  // Handle APS options
  if ( pSendZclFrameReq->qualityofservice == GW_QUALITY_OF_SERVICE_T__APS_ACK )
  {
    requestedOptions |= AF_ACK_REQUEST;
  }
  
  if ( pSendZclFrameReq->securityoptions == GW_SECURITY_OPTIONS_T__APS_SECURITY_ENABLED )
  {
    requestedOptions |= AF_EN_SECURITY;
  }

  originalOptions = zclGetClusterOption(pSendZclFrameReq->endpointidsource, pSendZclFrameReq->clusterid);

  if (zclUpdateClusterOption( pSendZclFrameReq->endpointidsource, pSendZclFrameReq->clusterid, (AF_EN_SECURITY | AF_ACK_REQUEST), requestedOptions) != ZSuccess)
  {
    uiPrintfEx( trERROR, "Sending ZCL Frame Request Failure, out of memory setting aps options\n"); 
    return ZFailure;
  }
  
  // Send command to ZCL for further building
  status = zcl_SendCommand( pSendZclFrameReq->endpointidsource, &dstAddr,
                            pSendZclFrameReq->clusterid, pSendZclFrameReq->commandid, 
                            pSendZclFrameReq->frametype, pSendZclFrameReq->clientserverdirection,
                            pSendZclFrameReq->disabledefaultrsp, mfrCode, transSeqNum,
                            pSendZclFrameReq->payload.len, pSendZclFrameReq->payload.data );

  // Restore original aps options. This must be done for endpoints that are also used outside this functuin, e.g. the gateway and the ota endpoints. Could theoretically be skipped for endpoints that are only used by this function.
  if (zclUpdateClusterOption( pSendZclFrameReq->endpointidsource, pSendZclFrameReq->clusterid, (AF_EN_SECURITY | AF_ACK_REQUEST), originalOptions) != ZSuccess)
  {
    uiPrintfEx( trERROR, "Sending ZCL Frame Request Failure, unexpected error when restoring zcl options\n"); 
    return ZFailure;
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendGwZclFrameReceiveInd
 *
 * @brief       Send IAS Zone Status Change Indication to the app(s)
 *
 * @param       pZclFrameInd - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static ZStatusValues sendGwZclFrameReceiveInd( int connection,  GwZclFrameReceiveInd *pZclFrameInd )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending ZCL Frame Receive Indication\n" );
  
  len = gw_zcl_frame_receive_ind__get_packed_size( pZclFrameInd );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_zcl_frame_receive_ind__pack( pZclFrameInd, pBuf );
    
    // Send response back to app
    APIS_SendData( connection, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_ZCL_FRAME_RECEIVE_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending ZCL Frame Receive Indication Failed, memory error\n" );
    
    return ZSTATUS_VALUES__ZMemError;
  }
  
  return ZSTATUS_VALUES__ZSuccess;  
}

/**************************************************************************************************
 *
 * @fn          sendGwAlarmInd
 *
 * @brief       Send Alarm Indication
 *
 * @param       pAlarmInd - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwAlarmInd( GwAlarmInd *pAlarmInd )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending Alarm Indication\n" );
  
  len = gw_alarm_ind__get_packed_size( pAlarmInd );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    gw_alarm_ind__pack( pAlarmInd, pBuf );
    
    // Send response back to app
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__GW_ALARM_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending Alarm Indication Failed, memory error\n" );
    
    return ZMemError;
  }
  
  return ZSuccess;  
}

/**************************************************************************************************
 *
 * @fn          sendGwAlarmResetReq
 *
 * @brief       Send Alarm Reset Request
 *
 * @param       pAlarmResetReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendGwAlarmResetReq( GwAlarmResetReq *pAlarmResetReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Alarm Reset Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pAlarmResetReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {  
    if ( pAlarmResetReq->has_alarmcode && pAlarmResetReq->has_clusterid )
    {
      // Send alarm reset request to specific device
      return zclGeneral_SendAlarmReset( srvParseGwEndpointId(pAlarmResetReq->dstaddress), &dstAddr,
                                        pAlarmResetReq->alarmcode, pAlarmResetReq->clusterid,
                                        disableDefaultRsp, zcl_TransID );
    }
    else
    {
      // Send alarm reset request to all devices
      return zclGeneral_SendAlarmResetAll( srvParseGwEndpointId(pAlarmResetReq->dstaddress), &dstAddr, disableDefaultRsp, zcl_TransID );
    }
  }
  else
  {
    return ZInvalidParameter;
  }
}

/**************************************************************************************************
 *
 * @fn          sendDevZoneStatusChangeInd
 *
 * @brief       Send IAS Zone Status Change Indication to the app(s)
 *
 * @param       pZoneStatusChange - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevZoneStatusChangeInd( DevZoneStatusChangeInd *pZoneStatusChange )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending IAS Zone Status Change Indication\n" );
  
  len = dev_zone_status_change_ind__get_packed_size( pZoneStatusChange );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_zone_status_change_ind__pack( pZoneStatusChange, pBuf );
    
    // Send response back to app
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_ZONE_STATUS_CHANGE_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending IAS Zone Status Change Indication Failed, memory error\n" );
    
    return ZMemError;
  }
  
  return ZSuccess;  
}

/**************************************************************************************************
 *
 * @fn          sendDevZoneEnrollmentReqInd
 *
 * @brief       Send IAS Zone Enrollment Request Indication to app(s)
 *
 * @param       pZoneEnrollReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevZoneEnrollmentReqInd( DevZoneEnrollmentReqInd *pZoneEnrollReq )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending IAS Zone Enrollment Request Indication\n" );
  
  len = dev_zone_enrollment_req_ind__get_packed_size( pZoneEnrollReq );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_zone_enrollment_req_ind__pack( pZoneEnrollReq, pBuf );
    
    // Send response back to app
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_ZONE_ENROLLMENT_REQ_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending IAS Zone Enrollment Request Indication Failed, memory error\n" );
    
    return ZMemError;
  }
  
  return ZSuccess;  
}

/**************************************************************************************************
 *
 * @fn          sendDevZoneEnrollmentRsp
 *
 * @brief       Send IAS Zone Enrollment Response to ZigBee
 *
 * @param       pTransEntry - pointer to the transaction table entry
 * @param       pZoneEnrollReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevZoneEnrollmentRsp( gsGwMsgTransTable_t *pTransEntry, DevZoneEnrollmentRsp *pZoneEnrollRsp )
{
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Zone Enrollment Response\n" );
  
  if ( gwConvertAddrPbToAfRsp( pZoneEnrollRsp->dstaddress, &dstAddr ) )
  {
    // Send the command to ZigBee
    return zclSS_IAS_Send_ZoneStatusEnrollResponseCmd( srvParseGwEndpointId(pZoneEnrollRsp->dstaddress), &dstAddr,
                                                       pZoneEnrollRsp->enrollmentresponsecode, 
                                                       pZoneEnrollRsp->zoneid,
                                                       TRUE, pTransEntry->zclTransId );
  }
  else
  {
    return ZInvalidParameter;
  }
}

/**************************************************************************************************
 *
 * @fn          sendDevAceArmReqInd
 *
 * @brief       Send IAS ACE Arm Request Indication to the app(s)
 *
 * @param       pAceArmReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevAceArmReqInd( DevAceArmReqInd *pAceArmReq )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending IAS ACE Arm Request Indication\n" );
  
  len = dev_ace_arm_req_ind__get_packed_size( pAceArmReq );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_ace_arm_req_ind__pack( pAceArmReq, pBuf );
    
    // Send response back to app
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_ACE_ARM_REQ_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending IAS ACE Arm Request Indication Failed, memory error\n" );
    
    return ZMemError;
  }
  
  return ZSuccess;
}

/**************************************************************************************************
 *
 * @fn          sendDevAceArmRsp
 *
 * @brief       Send IAS ACE Arm Response to ZigBee
 *
 * @param       pTransEntry - pointer to the transaction table entry
 * @param       pAceArmRsp - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevAceArmRsp( gsGwMsgTransTable_t *pTransEntry, DevAceArmRsp *pAceArmRsp )
{
  afAddrType_t dstAddr;
  
  if ( gwConvertAddrPbToAfRsp( pAceArmRsp->dstaddress, &dstAddr ) )
  {
    return zclSS_Send_IAS_ACE_ArmResponse( srvParseGwEndpointId(pAceArmRsp->dstaddress), &dstAddr, pAceArmRsp->armresponse, 
                                           TRUE, pTransEntry->zclTransId );
  }
  else
  {
    return ZInvalidParameter;
  }
}

/**************************************************************************************************
 *
 * @fn          sendDevAceBypassInd
 *
 * @brief       Send IAS ACE Bypass Indication to app(s)
 *
 * @param       pAceBypassInd - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevAceBypassInd( DevAceBypassInd *pAceBypassInd )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending IAS ACE Bypass Indication\n" );
  
  len = dev_ace_bypass_ind__get_packed_size( pAceBypassInd );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_ace_bypass_ind__pack( pAceBypassInd, pBuf );
    
    // Send response back to app
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_ACE_BYPASS_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending IAS ACE Bypass Indication Failed, memory error\n" );
    
    return ZMemError;
  }
  
  return ZSuccess; 
}

/**************************************************************************************************
 *
 * @fn          sendDevAceEmergencyConditionInd
 *
 * @brief       Send IAS ACE Emergency Condition Indication to app(s)
 *
 * @param       pAceEmergencyConditionInd - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevAceEmergencyConditionInd( DevAceEmergencyConditionInd *pAceEmergencyConditionInd )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending IAS ACE Emergency Condition Indication\n" );
  
  len = dev_ace_emergency_condition_ind__get_packed_size( pAceEmergencyConditionInd );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_ace_emergency_condition_ind__pack( pAceEmergencyConditionInd, pBuf );
    
    // Send response back to app
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_ACE_EMERGENCY_CONDITION_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending IAS ACE Emergency Condition Indication Failed, memory error\n" );
    
    return ZMemError;
  }
  
  return ZSuccess;  
}

/**************************************************************************************************
 *
 * @fn          sendDevAceGetZoneIdMapReqInd
 *
 * @brief       Send IAS ACE Get Zone ID Map Request Indication to app(s)
 *
 * @param       pAceGetZoneIdMapReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevAceGetZoneIdMapReqInd( DevAceGetZoneIdMapReqInd *pAceGetZoneIdMapReq )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending IAS ACE Get Zone ID Map Request Indication\n" );
  
  len = dev_ace_get_zone_id_map_req_ind__get_packed_size( pAceGetZoneIdMapReq );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_ace_get_zone_id_map_req_ind__pack( pAceGetZoneIdMapReq, pBuf );
    
    // Send response back to app
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_ACE_GET_ZONE_ID_MAP_REQ_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending IAS ACE Get Zone ID Map Request Indication Failed, memory error\n" );
    
    return ZMemError;
  }
  
  return ZSuccess;  
}

/**************************************************************************************************
 *
 * @fn          sendDevAceGetZoneIdMapRsp
 *
 * @brief       Send IAS ACE Get Zone ID Map Response to ZigBee
 *
 * @param       pAceGetZoneInfoRsp - command structure
 * @param       transSeqNum - ZCL transaction sequence number
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevAceGetZoneIdMapRsp( DevAceGetZoneIdMapRsp *pAceGetZoneIdMapRsp, uint8 transSeqNum )
{
  int i;
  uint16 *pZoneIdMap = NULL;
  afAddrType_t dstAddr;
  ZStatus_t status = ZInvalidParameter;
  
  //This zoneidmapsection is expected to be of size uint16[16]
  if ((pAceGetZoneIdMapRsp->n_zoneidmapsection != 16) || 
  (pAceGetZoneIdMapRsp->zoneidmapsection == NULL)) {
    return ZSTATUS_VALUES__ZInvalidParameter;
  }
  
  {
    pZoneIdMap = malloc( pAceGetZoneIdMapRsp->n_zoneidmapsection * sizeof( uint16 ) );
    if ( !pZoneIdMap )
    {
      return ZSTATUS_VALUES__ZMemError;
    }

    for ( i = 0; i < pAceGetZoneIdMapRsp->n_zoneidmapsection; i++ )
    {
      pZoneIdMap[i] = (uint16)(pAceGetZoneIdMapRsp->zoneidmapsection[i]);
    }
  }
  
  if ( gwConvertAddrPbToAfRsp( pAceGetZoneIdMapRsp->dstaddress, &dstAddr ) )
  {
    status = zclSS_Send_IAS_ACE_GetZoneIDMapResponseCmd( srvParseGwEndpointId(pAceGetZoneIdMapRsp->dstaddress), &dstAddr,
                                                         pZoneIdMap, TRUE, transSeqNum );
  }
   
  if ( pAceGetZoneIdMapRsp->n_zoneidmapsection )
  {                                                         
    // Free memory
    free( pZoneIdMap );
  }
  
  return status;                                                       
}

/**************************************************************************************************
 *
 * @fn          sendDevAceGetZoneInformationReqInd
 *
 * @brief       Send IAS ACE Get Zone Information Request Indication to app(s)
 *
 * @param       pAceGetZoneInfoReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevAceGetZoneInformationReqInd( DevAceGetZoneInformationReqInd *pAceGetZoneInfoReq )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "Sending IAS ACE Get Zone Information Request Indication\n" );
  
  len = dev_ace_get_zone_information_req_ind__get_packed_size( pAceGetZoneInfoReq );
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_ace_get_zone_information_req_ind__pack( pAceGetZoneInfoReq, pBuf );
    
    // Send response back to app
    APIS_SendData( ALL_APP_CONNECTIONS, Z_STACK_GW_SYS_ID_T__RPC_SYS_PB_GW, TRUE, 
                   GW_CMD_ID_T__DEV_ACE_GET_ZONE_INFORMATION_REQ_IND, len, pBuf );
    
    free( pBuf );
  }
  else
  {
    uiPrintf( "Sending IAS ACE Get Zone Information Request Indication Failed, memory error\n" );
    
    return ZMemError;
  }
  
  return ZSuccess;  
}

/**************************************************************************************************
 *
 * @fn          sendDevAceGetZoneInformationRsp
 *
 * @brief       Send IAS ACE Get Zone Information Response to ZigBee
 *
 * @param       pAceGetZoneInfoReq - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static ZStatus_t sendDevAceGetZoneInformationRsp( gsGwMsgTransTable_t *pTransEntry, DevAceGetZoneInformationRsp *pAceGetZoneInfoRsp )
{
  uint8 aIeeeAddr[sizeof( uint64_t )];
  afAddrType_t dstAddr;
  
  if ( gwConvertAddrPbToAfRsp( pAceGetZoneInfoRsp->dstaddress, &dstAddr ) )
  {   
    // Break destination long address into an array for send function
    aIeeeAddr[0] = BREAK_UINT64( pAceGetZoneInfoRsp->ieeeaddress, 0 );
    aIeeeAddr[1] = BREAK_UINT64( pAceGetZoneInfoRsp->ieeeaddress, 1 );
    aIeeeAddr[2] = BREAK_UINT64( pAceGetZoneInfoRsp->ieeeaddress, 2 );
    aIeeeAddr[3] = BREAK_UINT64( pAceGetZoneInfoRsp->ieeeaddress, 3 );
    aIeeeAddr[4] = BREAK_UINT64( pAceGetZoneInfoRsp->ieeeaddress, 4 );
    aIeeeAddr[5] = BREAK_UINT64( pAceGetZoneInfoRsp->ieeeaddress, 5 );
    aIeeeAddr[6] = BREAK_UINT64( pAceGetZoneInfoRsp->ieeeaddress, 6 );
    aIeeeAddr[7] = BREAK_UINT64( pAceGetZoneInfoRsp->ieeeaddress, 7 );
    
    // Send the command to ZigBee
    return zclSS_Send_IAS_ACE_GetZoneInformationResponseCmd( srvParseGwEndpointId(pAceGetZoneInfoRsp->dstaddress), &dstAddr,
                                                             pAceGetZoneInfoRsp->zoneid, 
                                                             pAceGetZoneInfoRsp->zonetype, 
                                                             aIeeeAddr,
                                                             TRUE, pTransEntry->zclTransId );
  }
  
  return ZInvalidParameter;
}

/**************************************************************************************************
 *
 * @fn          sendDevSetIdentifyModeReq
 *
 * @brief       Send Identify request message
 *
 * @param       pSetIdentifyModeReq - incoming DEV_SET_IDENTIFY_MODE_REQ command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevSetIdentifyModeReq( DevSetIdentifyModeReq *pSetIdentifyModeReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Set Identify Mode Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pSetIdentifyModeReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return zclGeneral_SendIdentify( srvParseGwEndpointId(pSetIdentifyModeReq->dstaddress), &dstAddr, pSetIdentifyModeReq->identifytime, disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }
}

/**************************************************************************************************
 *
 * @fn          sendDevSetOnOffStateReq
 *
 * @brief       Send Set On Off State Request message
 *
 * @param       pOnOffStateReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevSetOnOffStateReq( DevSetOnOffStateReq *pOnOffStateReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Set On/Off State Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pOnOffStateReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    switch ( pOnOffStateReq->state )
    {
      case GW_ON_OFF_STATE_T__OFF_STATE:
        return zclGeneral_SendOnOff_CmdOff( srvParseGwEndpointId(pOnOffStateReq->dstaddress), &dstAddr, disableDefaultRsp, zcl_TransID );
        break;
        
      case GW_ON_OFF_STATE_T__ON_STATE:
        return zclGeneral_SendOnOff_CmdOn( srvParseGwEndpointId(pOnOffStateReq->dstaddress), &dstAddr, disableDefaultRsp, zcl_TransID );
        break;
        
      case GW_ON_OFF_STATE_T__TOGGLE_STATE:
        return zclGeneral_SendOnOff_CmdToggle( srvParseGwEndpointId(pOnOffStateReq->dstaddress), &dstAddr, disableDefaultRsp, zcl_TransID );
        break;
        
      default:
        break;
    }
  }

  return ZInvalidParameter;
}

/**************************************************************************************************
 *
 * @fn          sendDevSetLevelReq
 *
 * @brief       Send Set Level Request message
 *
 * @param       pOnOffStateReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevSetLevelReq( DevSetLevelReq *pSetLevelReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Set Level Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pSetLevelReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {  
    return zclGeneral_SendLevelControlMoveToLevel( srvParseGwEndpointId(pSetLevelReq->dstaddress), &dstAddr, 
                                                   pSetLevelReq->levelvalue, pSetLevelReq->transitiontime, 
                                                   disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }                                                   
}

/**************************************************************************************************
 *
 * @fn          sendDevGetLevelReq
 *
 * @brief       Send Get Level Request message
 *
 * @param       pGetLevelReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevGetLevelReq( DevGetLevelReq *pGetLevelReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  zclReadCmd_t *pReadCmd;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Get Level Request\n" );
  
  if ( !gwConvertAddrPbToAfReq( pGetLevelReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return status;
  }
  
  pReadCmd = malloc( sizeof( zclReadCmd_t ) + sizeof( uint16 ) );
  if ( !pReadCmd )
  {
    return ZMemError;
  }
  
  pReadCmd->numAttr = 1;
  pReadCmd->attrID[0] = ATTRID_LEVEL_CURRENT_LEVEL;
  
  status = zcl_SendRead( srvParseGwEndpointId(pGetLevelReq->dstaddress), &dstAddr,
                         ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL, pReadCmd,
                         ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );
                         
  free( pReadCmd );
  
  return status;                         
}

/**************************************************************************************************
 *
 * @fn          sendDevGetOnOffStateReq
 *
 * @brief       Send Get On/Off State Request message
 *
 * @param       pGetOnOffStateReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevGetOnOffStateReq( DevGetOnOffStateReq *pGetOnOffStateReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  zclReadCmd_t *pReadCmd;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Get On/Off State Request\n" );
  
  if ( !gwConvertAddrPbToAfReq( pGetOnOffStateReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return status;
  }
  
  pReadCmd = malloc( sizeof( zclReadCmd_t ) + sizeof( uint16 ) );
  if ( !pReadCmd )
  {
    return ZMemError;
  }
  
  pReadCmd->numAttr = 1;
  pReadCmd->attrID[0] = ATTRID_ON_OFF;
  
  status = zcl_SendRead( srvParseGwEndpointId(pGetOnOffStateReq->dstaddress), &dstAddr,
                         ZCL_CLUSTER_ID_GEN_ON_OFF, pReadCmd,
                         ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );
                         
  free( pReadCmd );
  
  return status;                         
}

/**************************************************************************************************
 *
 * @fn          sendDevSetColorReq
 *
 * @brief       Send Set Color Request message
 *
 * @param       pSetColorReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevSetColorReq( DevSetColorReq *pSetColorReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Set Color Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pSetColorReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return zclLighting_ColorControl_Send_MoveToHueAndSaturationCmd( srvParseGwEndpointId(pSetColorReq->dstaddress), &dstAddr,
                                                                    pSetColorReq->huevalue, pSetColorReq->saturationvalue, 
                                                                    DEFAULT_SET_COLOR_TRANSITION_TIME,
                                                                    disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }                                                                    
}

/**************************************************************************************************
 *
 * @fn          sendDevGetColorReq
 *
 * @brief       Send Get Color Request message
 *
 * @param       pGetTempReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevGetColorReq( DevGetColorReq *pGetColorReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  zclReadCmd_t *pReadCmd;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Get Color Request\n" );
  
  if ( !gwConvertAddrPbToAfReq( pGetColorReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return status;
  }
  
  pReadCmd = malloc( sizeof( zclReadCmd_t ) + (2 * sizeof( uint16 )) );
  if ( !pReadCmd )
  {
    return ZMemError;
  }
  
  pReadCmd->numAttr = 2;
  pReadCmd->attrID[0] = ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE;
  pReadCmd->attrID[1] = ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION;
  
  status = zcl_SendRead( srvParseGwEndpointId(pGetColorReq->dstaddress), &dstAddr,
                         ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, pReadCmd,
                         ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );
                         
  free( pReadCmd );
  
  return status;                         
}

/**************************************************************************************************
 *
 * @fn          sendDevSetColorTempReq
 *
 * @brief       Send Set ColorTemp Request message
 *
 * @param       pSetColorTempReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevSetColorTempReq( DevSetColorTempReq *pSetColorTempReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s)
  afAddrType_t dstAddr;

  uiPrintf( "Sending Set ColorTemp Request\n" );

  if ( gwConvertAddrPbToAfReq( pSetColorTempReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return zclLighting_ColorControl_Send_MoveToColorTemperatureCmd( srvParseGwEndpointId(pSetColorTempReq->dstaddress), &dstAddr,
                                                                    pSetColorTempReq->temperaturevalue,
                                                                    DEFAULT_SET_COLOR_TRANSITION_TIME,
                                                                    disableDefaultRsp, zcl_TransID );
  }
  else
  {
    return ZInvalidParameter;
  }
}

/**************************************************************************************************
 *
 * @fn          sendDevGetColorTempReq
 *
 * @brief       Send Get ColorTemp Request message
 *
 * @param       pGetTempReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevGetColorTempReq( DevGetColorTempReq *pGetColorTempReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s)
  afAddrType_t dstAddr;
  zclReadCmd_t *pReadCmd;
  ZStatus_t status = ZInvalidParameter;

  uiPrintf( "Sending Get ColorTemp Request\n" );

  if ( !gwConvertAddrPbToAfReq( pGetColorTempReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return status;
  }

  pReadCmd = malloc( sizeof( zclReadCmd_t ) + (2 * sizeof( uint16 )) );
  if ( !pReadCmd )
  {
    return ZMemError;
  }

  pReadCmd->numAttr = 2;
  pReadCmd->attrID[0] = ATTRID_LIGHTING_COLOR_CONTROL_COLOR_TEMPERATURE;

  status = zcl_SendRead( srvParseGwEndpointId(pGetColorTempReq->dstaddress), &dstAddr,
                         ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, pReadCmd,
                         ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );

  free( pReadCmd );

  return status;
}
/**************************************************************************************************
 *
 * @fn          sendDevGetTempReq
 *
 * @brief       Send Get Temperature Request message
 *
 * @param       pGetTempReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevGetTempReq( DevGetTempReq *pGetTempReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  zclReadCmd_t *pReadCmd;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Get Temperature Request\n" );
  
  if ( !gwConvertAddrPbToAfReq( pGetTempReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return status;
  }
  
  pReadCmd = malloc( sizeof( zclReadCmd_t ) + sizeof( uint16 ) );
  if ( !pReadCmd )
  {
    return ZMemError;
  }
  
  pReadCmd->numAttr = 1;
  pReadCmd->attrID[0] = ATTRID_MS_TEMPERATURE_MEASURED_VALUE;
  
  status = zcl_SendRead( srvParseGwEndpointId(pGetTempReq->dstaddress), &dstAddr,
                         ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, pReadCmd,
                         ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );
                         
  free( pReadCmd );
  
  return status;                         
}

/**************************************************************************************************
 *
 * @fn          sendDevGetPowerReq
 *
 * @brief       Send Get Power Request message
 *
 * @param       pSetPowerReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevGetPowerReq( DevGetPowerReq *pGetPowerReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  zclReadCmd_t *pReadCmd;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Get Power Request\n" );
  
  if ( !gwConvertAddrPbToAfReq( pGetPowerReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return status;
  }
  
  pReadCmd = malloc( sizeof( zclReadCmd_t ) + sizeof( uint16 ) );
  if ( !pReadCmd )
  {
    return ZMemError;
  }
  
  pReadCmd->numAttr = 1;
  pReadCmd->attrID[0] = ATTRID_SE_INSTANTANEOUS_DEMAND;
  
  status = zcl_SendRead( srvParseGwEndpointId(pGetPowerReq->dstaddress), &dstAddr,
                         ZCL_CLUSTER_ID_SE_SIMPLE_METERING, pReadCmd,
                         ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );
                       
  free( pReadCmd );
  
  return status;                       
}

/**************************************************************************************************
 *
 * @fn          sendDevGetHumidityReq
 *
 * @brief       Send Get Humidity Request message
 *
 * @param       pGetHumidityReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevGetHumidityReq( DevGetHumidityReq *pGetHumidityReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  zclReadCmd_t *pReadCmd;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Get Humidity Request\n" );
  
  if ( !gwConvertAddrPbToAfReq( pGetHumidityReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return status;
  }
  
  pReadCmd = malloc( sizeof( zclReadCmd_t ) + sizeof( uint16 ) );
  if ( !pReadCmd )
  {
    return ZMemError;
  }
  
  pReadCmd->numAttr = 1;
  pReadCmd->attrID[0] = ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE;
  
  status = zcl_SendRead( srvParseGwEndpointId(pGetHumidityReq->dstaddress), &dstAddr,
                         ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY, pReadCmd,
                         ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );
                       
  free( pReadCmd );
  
  return status;                       
}

/**************************************************************************************************
 *
 * @fn          sendDevSetDoorLockReq
 *
 * @brief       Send Set Door Lock Request message
 *
 * @param       pSetDoorLockReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevSetDoorLockReq( DevSetDoorLockReq *pSetDoorLockReq )
{
  bool disableDefaultRsp;
  uint8 pinCodeSize = 1;  // minimum size for PIN
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST; // valid addr mode(s) 
  uint8 *pPinCode;
  afAddrType_t dstAddr;
  zclDoorLock_t doorLockPayload;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Set Door Lock Request\n" );
  
  if ( !gwConvertAddrPbToAfReq( pSetDoorLockReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) ||
       (pSetDoorLockReq->lockmode > GW_LOCK_MODE_T__LOCK_MODE_UNLOCK) )
  {
    return status;
  }
  
  if ( pSetDoorLockReq->pincodevalue.len )
  {
    pinCodeSize += pSetDoorLockReq->pincodevalue.len;  // include size byte  
  }
  
  pPinCode = malloc( pinCodeSize );
  if ( !pPinCode )
  {
    return ZMemError;
  }
  
  doorLockPayload.pPinRfidCode = pPinCode;
  
  *pPinCode++ = pSetDoorLockReq->pincodevalue.len;  // size of string
  
  if ( pSetDoorLockReq->pincodevalue.len )
  {
    // fill in PIN value
    memcpy( pPinCode, pSetDoorLockReq->pincodevalue.data, pSetDoorLockReq->pincodevalue.len );
  }
      
  status = zclClosures_SendDoorLockRequest( srvParseGwEndpointId(pSetDoorLockReq->dstaddress), &dstAddr, pSetDoorLockReq->lockmode,
                                            &doorLockPayload,
                                            disableDefaultRsp, zcl_TransID );                                      
                                            
  // Free memory
  free( doorLockPayload.pPinRfidCode );                                            
                                            
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendDevGetDoorLockStateReq
 *
 * @brief       Send Get Door Lock State Request message
 *
 * @param       pDoorLockStateReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevGetDoorLockStateReq( DevGetDoorLockStateReq *pDoorLockStateReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s)
  afAddrType_t dstAddr;
  zclReadCmd_t *pReadCmd;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Get Door Lock State Request\n" );
  
  if ( !gwConvertAddrPbToAfReq( pDoorLockStateReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return status;
  }
  
  // Allocate memory for 2 attribute IDs
  pReadCmd = malloc( sizeof( zclReadCmd_t ) + (2 * sizeof( uint16 )) );
  if ( !pReadCmd )
  {
    return ZMemError;  
  }
    
  pReadCmd->numAttr = 2;
  pReadCmd->attrID[0] = ATTRID_CLOSURES_LOCK_STATE;
  pReadCmd->attrID[1] = ATTRID_CLOSURES_DOOR_STATE;
  
  status = zcl_SendRead( srvParseGwEndpointId(pDoorLockStateReq->dstaddress), &dstAddr,
                         ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK, pReadCmd,
                         ZCL_FRAME_CLIENT_SERVER_DIR, disableDefaultRsp, zcl_TransID );
                         
  free( pReadCmd );                         
                         
  return status;                         
}

/**************************************************************************************************
 *
 * @fn          sendDevThermostatSetpointChangeReq
 *
 * @brief       Send Thermostat Setpoint Change Request message
 *
 * @param       pThermostatSetpointChangeReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevThermostatSetpointChangeReq( DevThermostatSetpointChangeReq *pThermostatSetpointChangeReq )
{
  bool disableDefaultRsp;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  afAddrType_t dstAddr;
  
  uiPrintf( "Sending Thermostat Setpoint Change Request\n" );
  
  if ( gwConvertAddrPbToAfReq( pThermostatSetpointChangeReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {  
    return zclHVAC_SendSetpointRaiseLower( srvParseGwEndpointId(pThermostatSetpointChangeReq->dstaddress), &dstAddr,
                                           pThermostatSetpointChangeReq->mode, pThermostatSetpointChangeReq->amount,
                                           disableDefaultRsp, zcl_TransID );
  }

  return ZFailure;
}

/**************************************************************************************************
 *
 * @fn          sendDevWindowCoveringActionReq
 *
 * @brief       Send Window Covering Action Request message
 *
 * @param       pSetPowerReq - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
static ZStatus_t sendDevWindowCoveringActionReq( DevWindowCoveringActionReq *pWindowCoveringReq )
{
  bool disableDefaultRsp;
  uint8 cmdId;
  uint16 value = 0;
  uint8 addrModeMask = ADDR_MODE_UNICAST + ADDR_MODE_GROUPCAST + ADDR_MODE_BROADCAST; // valid addr mode(s) 
  uint16 percentageValue = 0;
  afAddrType_t dstAddr;
  ZStatus_t status = ZInvalidParameter;
  
  uiPrintf( "Sending Window Covering Action Request\n" );
  
  if ( !gwConvertAddrPbToAfReq( pWindowCoveringReq->dstaddress, &dstAddr, &disableDefaultRsp, addrModeMask ) )
  {
    return status;  
  }
  
  // Store command ID
  cmdId = pWindowCoveringReq->action;
  
  switch ( cmdId )
  {
    case COMMAND_CLOSURES_GO_TO_LIFT_VALUE:
    case COMMAND_CLOSURES_GO_TO_TILT_VALUE:
      if ( pWindowCoveringReq->has_value )
      {
        value = pWindowCoveringReq->value;
      }
      break;
      
    case COMMAND_CLOSURES_GO_TO_LIFT_PERCENTAGE:
    case COMMAND_CLOSURES_GO_TO_TILT_PERCENTAGE:
      if ( pWindowCoveringReq->has_percentage )
      {
        percentageValue = pWindowCoveringReq->percentage;
      }
      break;
      
    default:
      break;
  }           
  
  if ( (cmdId >= COMMAND_CLOSURES_UP_OPEN) && (cmdId <= COMMAND_CLOSURES_STOP) )
  {
    status = zclClosures_WindowCoveringSimpleReq( srvParseGwEndpointId(pWindowCoveringReq->dstaddress), &dstAddr,
                                                  cmdId, disableDefaultRsp, zcl_TransID );
  }
  else if ( (cmdId == COMMAND_CLOSURES_GO_TO_LIFT_VALUE) || (cmdId == COMMAND_CLOSURES_GO_TO_TILT_VALUE) )
  {
    status = zclClosures_WindowCoveringSendGoToValueReq( srvParseGwEndpointId(pWindowCoveringReq->dstaddress), &dstAddr,
                                                         cmdId, value,
                                                         disableDefaultRsp, zcl_TransID );
  }
  else if ( (cmdId == COMMAND_CLOSURES_GO_TO_LIFT_PERCENTAGE) || (cmdId == COMMAND_CLOSURES_GO_TO_TILT_PERCENTAGE) )
  {
    status = zclClosures_WindowCoveringSendGoToPercentageReq( srvParseGwEndpointId(pWindowCoveringReq->dstaddress), &dstAddr,
                                                              cmdId, percentageValue,
                                                              disableDefaultRsp, zcl_TransID );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          gwSendGpSecRsp
 *
 * @brief       Send Green Power Security Response message
 *
 * @param       pGpSecRsp - command structure
 *
 * @return      ZStatus_t
 *
 **************************************************************************************************/
ZStatus_t gwSendGpSecRsp( gp_SecRsp_t *pGpSecRsp )
{
  afStatus_t status = afStatus_FAILED;
  int len;
  uint8 *pBuf;
  GpSecRspMsg secRsp = GP_SEC_RSP_MSG__INIT;

  uiPrintfEx(trDEBUG,  "Sending Green Power Security Response\n" );

  secRsp.status = pGpSecRsp->Status;
  secRsp.dgpstubhandle = pGpSecRsp->dGPStubHandle;
  secRsp.appid = pGpSecRsp->gpd_ID.AppID;

  if( secRsp.appid == GP_APP_ID_DEFAULT )
  {
    secRsp.srcid = pGpSecRsp->gpd_ID.GPDId.SrcID;
  }
  else if( secRsp.appid == GP_APP_ID_GP )
  {
    memcpy(&secRsp.gpdextaddr, pGpSecRsp->gpd_ID.GPDId.GPDExtAddr, 8);
  }

  secRsp.endpoint = pGpSecRsp->EndPoint;
  secRsp.gpdfseclvl = pGpSecRsp->gp_SecData.GPDFSecLvl;
  secRsp.gpdfkeytype = pGpSecRsp->gp_SecData.GPDFKeyType;
  secRsp.gpdkey.len = SEC_KEY_LEN;
  secRsp.gpdkey.data = malloc( secRsp.gpdkey.len );
  if( secRsp.gpdkey.data )
  {
    memcpy(secRsp.gpdkey.data, pGpSecRsp->GPDKey, secRsp.gpdkey.len);
  }
  secRsp.gpdsecframecounter = pGpSecRsp->gp_SecData.GPDSecFrameCounter;

  len = gp_sec_rsp_msg__get_packed_size( &secRsp );
  pBuf = malloc( len );
  if ( pBuf )
  {
    gp_sec_rsp_msg__pack( &secRsp, pBuf );

    // Send the API client message
    apicSendAsynchData( GW_ZSTACK_HANDLE, ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF, ZSTACK_CMD_IDS__GP_SEC_RSP_MSG, len, pBuf );
    //status = (afStatus_t)sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__GP_SEC_RSP_MSG, len, pBuf );

    if( secRsp.gpdkey.data )
    {
      free( secRsp.gpdkey.data );
    }

    free( pBuf );
  }

  return ( status );
}

/**************************************************************************************************
 *
 * @fn          getUserInput
 *
 * @brief       Get the user's input
 *
 * @return      none, actually doesn't return
 *
 **************************************************************************************************/
static void getUserInput( void )
{
  int key = 0;
  bool keypressed = FALSE;
  //DevState prevState;

  for ( ;; )
  {
    //prevState = opState;

    transitionState( keypressed, key );
    keypressed = FALSE;

    // Wait for a key press
    key = getchar();
    keypressed = TRUE;

    if ( key == 'q' )
    {
      // Quit
      return;
    }
    else if (key < 0) 
    {
      // stdin unavailable. pause until we get a signal.
  // we get a signal when brought to the foreground
  pause();
    }
    else 
    {
      ; // ignoring this key. should we say anything?
    }
  }
}

/**************************************************************************************************
 *
 * @fn          transitionState
 *
 * @brief       Displays State information to the user, and performs user action
 *
 * @param       keypressed - TRUE if key has been pressed, FALSE to display state change
 *
 * @param       key - what key was pressed
 *
 * @return      none
 *
 **************************************************************************************************/
static void transitionState( bool keypressed, int key )
{
  if ( keypressed == TRUE )
  {
    switch ( key )
    {
      default:
        break;
    }
  }
  else
  {
    switch ( opState )
    {
      case DEV_STATE__INIT:
        break;

      default:
        break;
    }
  }
}

/**************************************************************************************************
 *
 * @fn          gwRegAllEp
 *
 * @brief       Send the AF Register Endpoint message
 *
 * @param       none
 *
 * @return      TRUE for success, FALSE for failure
 *
 **************************************************************************************************/
static bool gwRegAllEp( void )
{
  bool rc = TRUE;
  int i;
  int len;
  uint8 *pBuf;

  uiPrintf( "Sending AF Register Request\n" );

  for ( i = 0; i < pgSrvEndpointDefs->endpointCount; i++ )
  {
    len = af_register_req__get_packed_size( pgSrvEndpointDefs->ppEndpoints[i] );
    pBuf = malloc( len );
    if ( pBuf )
    {
      uint8 status = 0;

      af_register_req__pack( pgSrvEndpointDefs->ppEndpoints[i], pBuf );

      // Send the API Client message
      status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__AF_REGISTER_REQ, len, pBuf );

      if ( status != 0 )
      {
        uiPrintf( "AF Register EP: %d Request bad status(%d)\n", 
                  pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->endpoint, status );
        
        rc = FALSE;
      }
      else
      {
        uiPrintf( "AF Register EP: %d Request Successful\n", pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->endpoint );
      }

      free( pBuf );
    }
    else
    {
      rc = FALSE;
    }
  }
  
  return rc;
}

/**************************************************************************************************
 *
 * @fn          gwMsgTransPost
 *
 * @brief       Insert new transaction entry in gateway transaction table
 *
 * @param       multRsp - if TRUE, multiple responses expected for transaction entry
 * @param       rspType - either GW_RSP_SPECIFIC or GW_RSP_GENERIC
 * @param       connection - connection handle (tcp)
 * @param       dstAddress - target IEEE address for the request command 
 * @param       cmdId - requesting command from the app
 * @param       appTransId - application transaction ID
 * @param       transTimer - if non-zero, entry is expecting multiple responses on transaction ID
 *                           and will be deleted once timer reaches 0
 *
 * @return      none
 *
 **************************************************************************************************/
static void gwMsgTransPost( bool multRsp, uint8 rspType, int connection, uint64_t dstAddress, uint8 cmdId, 
                            uint16 appTransId, uint8 zclTransId, int transTimer )
{
  gsGwMsgTransTable_t *pMsgTable;
  
  // Find empty table entry
  pMsgTable = gwServices_UpdateTransTable();
  
  if ( !pMsgTable )
  {
    uiPrintf( "Message Transaction Table Error - Unable to allocate more table entries\n" );
    
    return;
  }
  
  pMsgTable->inUse = TRUE;
  pMsgTable->multRsp = multRsp;
  pMsgTable->rspCount = 0;
  pMsgTable->rspType = rspType;
  pMsgTable->connection = connection;
  pMsgTable->dstAddr = dstAddress;
  pMsgTable->cmdId = cmdId;
  pMsgTable->appTransId = appTransId;
  pMsgTable->zclTransId = zclTransId;
  pMsgTable->transTimer = transTimer;

  uiPrintfEx(trINFO, "(GW posted) cmdId: %d, zclTransId %d\n", cmdId, zclTransId );
}

/**************************************************************************************************
 *
 * @fn          gwMsgTransGetByZclTransId
 *
 * @brief       Retrieve transaction entry in gateway transaction table
 *
 * @param       zclTransId - ZCL transaction ID
 * @param       reqCmdId - requesting command ID
 *
 * @return      pointer to transaction table entry
 *
 **************************************************************************************************/
static gsGwMsgTransTable_t *gwMsgTransGetByZclTransId( uint8 zclTransId, uint8 reqCmdId )
{
  int i;
  
  for ( i = 0; i < giGwServices_TransTableCount; i++ )
  {
    if ( ( gpGwServices_TransTable[i].inUse == TRUE ) && ( zclTransId == gpGwServices_TransTable[i].zclTransId ) &&
         (reqCmdId == gpGwServices_TransTable[i].cmdId) )
    {
      if ( gpGwServices_TransTable[i].multRsp == FALSE )
      {
        // Clear retry table if necessary
        gwUpdateDeviceInRetryTable( gpGwServices_TransTable[i].dstAddr );
      }
      
      return ( &(gpGwServices_TransTable[i]) );
    }
  }
  
  return NULL;  // no transaction table entry found
}

/**************************************************************************************************
 *
 * @fn          gwMsgTransGetByZclTransIdGenericRsp
 *
 * @brief       Retrieve transaction entry in gateway transaction table for generic responses
 *
 * @param       zclTransId - ZCL transaction ID
 *
 * @return      pointer to transaction table entry
 *
 **************************************************************************************************/
static gsGwMsgTransTable_t *gwMsgTransGetByZclTransIdGenericRsp( uint8 zclTransId )
{
  int i;
  
  for ( i = 0; i < giGwServices_TransTableCount; i++ )
  {
    if ( ( gpGwServices_TransTable[i].inUse == TRUE ) && ( zclTransId == gpGwServices_TransTable[i].zclTransId ) &&
         (GW_RSP_GENERIC == gpGwServices_TransTable[i].rspType) )
    {
      if ( gpGwServices_TransTable[i].multRsp == FALSE )
      {
        // Clear retry table if necessary
        gwUpdateDeviceInRetryTable( gpGwServices_TransTable[i].dstAddr );
      }
      
      return ( &(gpGwServices_TransTable[i]) );
    }
  }
  
  return NULL;  // no transaction table entry found
}



/**************************************************************************************************
 *
 * @fn          gwMsgTransGetByAppTransId
 *
 * @brief       Retrieve transaction entry in gateway transaction table
 *
 * @param       appTransId - App transaction ID
 *
 * @return      pointer to transaction table entry
 *
 **************************************************************************************************/
static gsGwMsgTransTable_t *gwMsgTransGetByAppTransId( uint16 appTransId )
{
  int i;
  
  for ( i = 0; i < giGwServices_TransTableCount; i++ )
  {
    if ( ( gpGwServices_TransTable[i].inUse == TRUE ) && ( appTransId == gpGwServices_TransTable[i].appTransId ) )
    {
      return ( &(gpGwServices_TransTable[i]) );
    }
  }
  
  return NULL;  // no transaction table entry found
}

/**************************************************************************************************
 *
 * @fn          gwMsgTransGetBySrcAddrCmdId
 *
 * @brief       Retrieve transaction entry in gateway transaction table
 *
 * @param       srcAddress - IEEE address of transaction entry
 * @param       reqCmdId - requesting command ID
 *
 * @return      pointer to transaction table entry
 *
 **************************************************************************************************/
static gsGwMsgTransTable_t *gwMsgTransGetBySrcAddrCmdId( uint64_t srcAddress, uint8 reqCmdId )
{
  int i;
  
  for ( i = 0; i < giGwServices_TransTableCount; i++ )
  {
    if ( (gpGwServices_TransTable[i].inUse == TRUE) && (srcAddress == gpGwServices_TransTable[i].dstAddr) &&
         (reqCmdId == gpGwServices_TransTable[i].cmdId) )
    {
      if ( gpGwServices_TransTable[i].multRsp == FALSE )
      {
        // Clear retry table if necessary
        gwUpdateDeviceInRetryTable( gpGwServices_TransTable[i].dstAddr );
      }
      
      return ( &(gpGwServices_TransTable[i]) );
    }
  }
  
  return NULL;  // no transaction table entry found
}

/**************************************************************************************************
 *
 * @fn          gwMsgRemoveTrans
 *
 * @brief       Remove transaction entry in gateway transaction table by its ZCL transaction ID
 *
 * @param       appTransId - application transaction ID
 *
 * @return      returns TRUE if entry removed
 *
 **************************************************************************************************/
static bool gwMsgRemoveTrans( uint16 appTransId )
{
  int i;
  int count = 0;
  gsGwMsgTransTable_t *pMsgTable;

  pMsgTable = gwMsgTransGetByAppTransId( appTransId );
  
  if ( pMsgTable && !(pMsgTable->multRsp) )
  {
    pMsgTable->inUse = FALSE;

#ifdef __APP_UI__    
    for ( i = 0; i < giGwServices_TransTableCount; i++ )
    {
      if ( gpGwServices_TransTable[i].inUse == TRUE )
      {
        count++;
      }
    }
    
    uiPrintfEx(trDEBUG, "(GW remove) deleted zclTransId: %d, table size: %d\n", pMsgTable->zclTransId, count );
#endif
    
    return TRUE;
  }
  
  return FALSE;
}

/**************************************************************************************************
 *
 * @fn          gwHandleCnfRsp
 *
 * @brief       Sends the appropriate type of confirm, and response (if syncrhonous). Posts to the
 *              transaction table if asynchronous.
 *
 * @param       rspType - GW_RSP_NONE, GW_RSP_GENERIC, GW_RSP_SPECIFIC
 * @param       addrType - address mode type
 * @param       connection - tcp connection, used for sending response
 * @param       dstAddress - target IEEE address of request command, used for CB responses
 * @param       cmdId - requesting command ID
 * @param       status - status used for generic confirmation
 *
 * @return      none
 *
 **************************************************************************************************/
static void gwHandleCnfRsp( uint8 rspType, uint8 addrType, int connection, 
                            uint64_t dstAddress, uint8 cmdId, ZStatusValues status )
{
  int transTime = giGwDeviceTimeout;
  GwZigbeeGenericCnf genericCnf = GW_ZIGBEE_GENERIC_CNF__INIT;
  bool hasSequence = FALSE;
  bool hasConfirm = TRUE;
  bool multipleRsp = FALSE;
  uint16 sequence = 0xFFFF; // for self

  // no response expected, just a confirm
  if ( rspType == GW_RSP_NONE )
  {
    // Nothing to do, send confirmation
  }
  else if ( rspType == GW_RSP_GENERIC ) // a generic response is expected
  {
    // unicast will response on the data confirm
    if ( addrType == GW_ADDRESS_TYPE_T__UNICAST )
    {
      hasSequence = TRUE;
      
      if ( status == ZSTATUS_VALUES__ZSuccess )
      {
        gwMsgTransPost( multipleRsp, rspType, connection, dstAddress, cmdId, 
                        gGwAppTransSeqNum, (zcl_TransID - 1), transTime );
      }
      
      sequence = gwIncreaseCurrentAppSeqNum();
    }
    else  // no sequence # for self, broadcast or groupcast
    {      
      // App will not expect generic response
    }
  }
  else if ( rspType == GW_RSP_SPECIFIC )  // a specific response is expected
  {
    // self, no confirm
    if ( addrType == GW_ADDRESS_TYPE_T__NONE )
    {
      hasConfirm = FALSE;
 
      gwMsgTransPost( multipleRsp, rspType, connection, dstAddress, cmdId, 
                      gGwAppTransSeqNum, (zcl_TransID - 1), transTime );
    }
    else if ( addrType == GW_ADDRESS_TYPE_T__UNICAST )   // unicast will response on the data confirm
    {
      hasSequence = TRUE;
      
      if ( status == ZSTATUS_VALUES__ZSuccess )
      {
        gwMsgTransPost( FALSE, rspType, connection, dstAddress, cmdId, 
                        gGwAppTransSeqNum, (zcl_TransID - 1), transTime );
      }
      
      sequence = gwIncreaseCurrentAppSeqNum();
    }
    else if ( (addrType == GW_ADDRESS_TYPE_T__BROADCAST) ||
              (addrType == GW_ADDRESS_TYPE_T__GROUPCAST) ) 
    {
      hasSequence = TRUE;
      multipleRsp = TRUE;
      
      if ( status == ZSTATUS_VALUES__ZSuccess )
      {
        gwMsgTransPost( multipleRsp, rspType, connection, 0, cmdId, 
                        gGwAppTransSeqNum, (zcl_TransID - 1), transTime );
                        
        sequence = gwIncreaseCurrentAppSeqNum();                        
      }
    }
    else
    {
      // no sequence # for self address, app expects immediate response
    }
  }
  
  // send the confirm
  if ( hasConfirm )
  {
    genericCnf.has_sequencenumber = hasSequence;
    
    if ( hasSequence )
    {
      genericCnf.sequencenumber = sequence;
    }
    
    // Configure status field
    if ( status == ZSTATUS_VALUES__ZSuccess )
    {
      genericCnf.status = GW_STATUS_T__STATUS_SUCCESS;
    }
    else if ( (status == ZSTATUS_VALUES__ZInvalidParameter) ||
              (status == ZSTATUS_VALUES__ZDecodeError) )
    {
      genericCnf.status = GW_STATUS_T__STATUS_INVALID_PARAMETER;
    }
    else
    {
      genericCnf.status = GW_STATUS_T__STATUS_FAILURE;
    }

    sendZbGenericCnf( connection, &genericCnf );
  }
}

/**************************************************************************************************
 *
 * @fn          gwSendUnicastRouteReq
 *
 * @brief       Send a Unicast Route Request
 *
 * @param       shortAddr - short address of device
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues gwSendUnicastRouteReq( uint16 shortAddr )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;
  DevNwkRouteReq req = DEV_NWK_ROUTE_REQ__INIT;
  
  uiPrintf( "Sending Route Request to device: %04X\n", shortAddr );
  
  req.dstaddr = shortAddr;
  req.radius = NWK_ROUTE_RADIUS;
  
  len = dev_nwk_route_req__get_packed_size( &req );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = dev_nwk_route_req__pack( &req, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__DEV_NWK_ROUTE_REQ, len, pBuf );

    free( pBuf );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          gwConvertAddrPbToAfReq
 *
 * @brief       Converts the the protobuf address structure to AF address structure, converting
 *              long addresses to the correlating short address using the database. Used
 *              for outgoing request commands.
 *
 * @param       pPbAddrStruct - pointer to the protobuf address structure
 * @param       pAfAddrStruct - pointer to the AF address structure
 * @param       pDisableDefaultRsp - pointer to default response variable
 * @param       addrModeMask - bitmask for ADDR_MODE_UNICAST, ADDR_MODE_GROUPCAST, ADDR_MODE_BROADCAST
 *                             and/or ADDR_MODE_NONE
 *
 * @return      Returns TRUE for success, FALSE for invalid parameter
 *
 **************************************************************************************************/
static bool gwConvertAddrPbToAfReq( GwAddressStructT *pPbAddrStruct, afAddrType_t *pAfAddrStruct, 
                                    bool *pDisableDefaultRsp, uint8 addrModeMask )
{
  uint8 endpointId = 0xFF;  // default
  
  *pDisableDefaultRsp = FALSE;
  
  if ( (pPbAddrStruct->addresstype == GW_ADDRESS_TYPE_T__UNICAST) &&
       (addrModeMask & ADDR_MODE_UNICAST) && pPbAddrStruct->has_ieeeaddr)
  {
#if IEEE_ADDR_PRIORITY    
    pAfAddrStruct->addr.extAddr[0] = BREAK_UINT64( pPbAddrStruct->ieeeaddr, 0 );
    pAfAddrStruct->addr.extAddr[1] = BREAK_UINT64( pPbAddrStruct->ieeeaddr, 1 );
    pAfAddrStruct->addr.extAddr[2] = BREAK_UINT64( pPbAddrStruct->ieeeaddr, 2 );
    pAfAddrStruct->addr.extAddr[3] = BREAK_UINT64( pPbAddrStruct->ieeeaddr, 3 );
    pAfAddrStruct->addr.extAddr[4] = BREAK_UINT64( pPbAddrStruct->ieeeaddr, 4 );
    pAfAddrStruct->addr.extAddr[5] = BREAK_UINT64( pPbAddrStruct->ieeeaddr, 5 );
    pAfAddrStruct->addr.extAddr[6] = BREAK_UINT64( pPbAddrStruct->ieeeaddr, 6 );
    pAfAddrStruct->addr.extAddr[7] = BREAK_UINT64( pPbAddrStruct->ieeeaddr, 7 );
      
    pAfAddrStruct->addrMode = Addr64Bit;
#else
    if ( !gwPb_SrvrGetShortAddress( pPbAddrStruct->ieeeaddr,
                                    &(pAfAddrStruct->addr.shortAddr) ) )
    {
      uiPrintf( "Error finding short address for IeeeAddr: %016llX\n", pPbAddrStruct->ieeeaddr );
    
      return FALSE;
    }         
    
    pAfAddrStruct->addrMode = Addr16Bit;
#endif
  }
  else if ( (pPbAddrStruct->addresstype == GW_ADDRESS_TYPE_T__GROUPCAST) &&
            (addrModeMask & ADDR_MODE_GROUPCAST) && pPbAddrStruct->has_groupaddr )
  {
    pAfAddrStruct->addr.shortAddr = pPbAddrStruct->groupaddr;
    pAfAddrStruct->addrMode = AddrGroup;
    
    *pDisableDefaultRsp = TRUE;
  }
  else if ( (pPbAddrStruct->addresstype == GW_ADDRESS_TYPE_T__BROADCAST) &&
            (addrModeMask & ADDR_MODE_BROADCAST) && pPbAddrStruct->has_broadcastaddr )
  {
    pAfAddrStruct->addr.shortAddr = pPbAddrStruct->broadcastaddr;
    pAfAddrStruct->addrMode = AddrBroadcast;
    
    *pDisableDefaultRsp = TRUE;
  }
  else if ( (pPbAddrStruct->addresstype == GW_ADDRESS_TYPE_T__NONE) &&
            (addrModeMask & ADDR_MODE_NONE) )
  {
    pAfAddrStruct->addr.shortAddr = gLocalDeviceInfo.nwkaddr;
    pAfAddrStruct->addrMode = Addr16Bit;
  }
  else
  {
    return FALSE; // Incorrect address mode used
  }
  
  if ( pPbAddrStruct->has_endpointid == TRUE )
  {
    endpointId = pPbAddrStruct->endpointid;
  }
  
  pAfAddrStruct->endPoint = endpointId;
  
  uiPrintfEx(trDEBUG, "AF DstAddr Info:\n" );
  uiPrintfEx(trDEBUG, "AddrMode: %d\n", pAfAddrStruct->addrMode );
  uiPrintfEx(trDEBUG, "ShortAddr: %04X\n", pAfAddrStruct->addr.shortAddr );
  uiPrintfEx(trDEBUG, "EndpointId: %d\n", pAfAddrStruct->endPoint );
  
  return TRUE;
}

/**************************************************************************************************
 *
 * @fn          gwConvertAddrAfToPbRsp
 *
 * @brief       Converts the the protobuf address structure from AF address structure, converting
 *              short addresses to the correlating long address using the database.
 *
 * @param       pPbAddrStruct - pointer to the protobuf address structure
 * @param       pAfAddrStruct - pointer to the AF address structure
 *
 * @return      Returns TRUE for success, FALSE for invalid parameter
 *
 **************************************************************************************************/
static bool gwConvertAddrAfToPbRsp( GwAddressStructT *pPbAddrStruct, afAddrType_t *pAfAddrStruct )
{
  pPbAddrStruct->addresstype = GW_ADDRESS_TYPE_T__UNICAST;
  pPbAddrStruct->has_ieeeaddr = TRUE;
  
  pPbAddrStruct->has_endpointid = TRUE;
  pPbAddrStruct->endpointid = pAfAddrStruct->endPoint;
  
  return gwPb_SrvrGetIeeeAddress( pAfAddrStruct->addr.shortAddr, &(pPbAddrStruct->ieeeaddr) );
}

/**************************************************************************************************
 *
 * @fn          gwConvertAddrPbToAfRsp
 *
 * @brief       Converts the the protobuf address structure to AF address structure, converting
 *              long addresses to the correlating short address using the database. Used
 *              for outgoing response commands
 *
 * @param       pPbAddrStruct - pointer to the protobuf address structure
 * @param       pAfAddrStruct - pointer to the AF address structure
 *
 * @return      Returns TRUE for success, FALSE for invalid parameter
 *
 **************************************************************************************************/
static bool gwConvertAddrPbToAfRsp( GwAddressStructT *pPbAddrStruct, afAddrType_t *pAfAddrStruct )
{
  uint8 endpointId = 0xFF; // default
  
  if ( !gwPb_SrvrGetShortAddress( pPbAddrStruct->ieeeaddr,
                                  &(pAfAddrStruct->addr.shortAddr) ) )
  {
    return FALSE;
  }
    
  pAfAddrStruct->addrMode = Addr16Bit;
    
  if ( pPbAddrStruct->has_endpointid == TRUE )
  {
    endpointId = pPbAddrStruct->endpointid;
  }
  
  pAfAddrStruct->endPoint = endpointId;
  
  return TRUE;
}

/**************************************************************************************************
 *
 * @fn          gwTransTimeoutRsp
 *
 * @brief       Handles response to the app upon timeout of message transaction table entry
 *
 * @param       pMsgTableEntry - pointer to the timed out transaction table entry
 *
 * @return      none
 *
 **************************************************************************************************/
static void gwTransTimeoutRsp( gsGwMsgTransTable_t *pMsgTableEntry )
{ 
  // Handle any notification to the app(s)
  if ( pMsgTableEntry->rspType == GW_RSP_GENERIC )
  {
    GwZigbeeGenericRspInd genericRsp = GW_ZIGBEE_GENERIC_RSP_IND__INIT;
    
    genericRsp.sequencenumber = pMsgTableEntry->appTransId;
    genericRsp.status = GW_STATUS_T__STATUS_TIMEOUT;
   
    // Send generic response to the app 
    sendZbGenericRspInd( pMsgTableEntry->connection, &genericRsp );
  }
  else if ( pMsgTableEntry->rspType == GW_RSP_SPECIFIC )
  {
    // Handle specific responses
    gwSpecificRspTimeoutHandler( pMsgTableEntry );
  }
}

/**************************************************************************************************
 *
 * @fn          gwDeviceReqRetryHandler
 *
 * @brief       Indicates how many times a request has been made to a device without a response
 *
 * @param       ieeeAddr - IEEE address of device
 *
 * @return      none
 *
 **************************************************************************************************/
static void gwDeviceReqRetryHandler( uint64_t ieeeAddr )
{
  uint16 shortAddr;
  gsGwMsgRetryTable_t *pRetryEntry;
  
  // Get the short address
  if ( gwPb_SrvrGetShortAddress( ieeeAddr, &shortAddr ) )
  {
    // Send route request
    gwSendUnicastRouteReq( shortAddr );
  }
  
  // Look for entry in retry table
  pRetryEntry = gwServices_GetDeviceInRetryTable( ieeeAddr );
  
  // If entry found
  if ( pRetryEntry )
  {
    if ( pRetryEntry->failedCount < MAX_DEVICE_FAILED_ATTEMPTS )
    {
      pRetryEntry->failedCount++;
    }
    else if ( pRetryEntry->failedCount == MAX_DEVICE_FAILED_ATTEMPTS )
    {
      uint8 status = SRVR_DEVICE_STATUS_T__DEVICE_OFF_LINE;
      
      uiPrintf( "Maximum message tries to remote device reached:" );
      uiPrintf( " IeeeAddr: %016llX\n", ieeeAddr );
      
      // Remote device unresponsive, update device status
      gwPb_SrvrSetDeviceStatus( ieeeAddr, status );
    }
  }
  else
  {
    // Create new entry
    pRetryEntry = gwServices_UpdateRetryTable();
    
    if ( pRetryEntry )
    {
      pRetryEntry->inUse = TRUE;
      pRetryEntry->failedCount = 1;  // mark first count
      pRetryEntry->ieeeAddr = ieeeAddr;
    }
    else
    {
      uiPrintf( "Retry Table Error - Unable to allocate additional table entries\n" );
    }
  }
}

/**************************************************************************************************
 *
 * @fn          gwUpdateDeviceInRetryTable
 *
 * @brief       Remote device has responded, delete retry table entry
 *
 * @param       ieeeAddr - IEEE address of device
 *
 * @return      pointer to device's entry, NULL if no entry found
 *
 **************************************************************************************************/
static void gwUpdateDeviceInRetryTable( uint64_t ieeeAddr )
{
  gsGwMsgRetryTable_t *pRetryEntry;
  
  // Look for device in retry table entry
  pRetryEntry = gwServices_GetDeviceInRetryTable( ieeeAddr );
  
  // Remove device from table if found
  if ( pRetryEntry )
  {
    pRetryEntry->inUse = FALSE;

    uint8 status = SRVR_DEVICE_STATUS_T__DEVICE_ON_LINE;
    
    uiPrintf( "Device is back online:" );
    uiPrintf( " IeeeAddr: %016llX\n", ieeeAddr );
    
    // Remote device back online, update device status
    gwPb_SrvrSetDeviceStatus( ieeeAddr, status );
  }
}

/**************************************************************************************************
 *
 * @fn          gwSpecificRspTimeoutHandler
 *
 * @brief       Handles specific responses to the application for timed out message transaction table entries
 *
 * @param       pMsgTableEntry - pointer to the timed out transaction table entry
 *
 * @return      none
 *
 **************************************************************************************************/
static void gwSpecificRspTimeoutHandler( gsGwMsgTransTable_t *pMsgTableEntry )
{
  GwStatusT rspStatus = GW_STATUS_T__STATUS_TIMEOUT;
  GwAddressStructT addressInfo = GW_ADDRESS_STRUCT_T__INIT;
  
  // Check if multiple responses were expected
  if ( (pMsgTableEntry->multRsp == TRUE) && (pMsgTableEntry->rspCount > 0) )
  {
    return; // command response was already handled correctly
  }  
  
  switch( pMsgTableEntry->cmdId )
  {
    case GW_CMD_ID_T__GW_GET_GROUP_MEMBERSHIP_REQ:
      {
        GwGetGroupMembershipRspInd getGroupMbrRsp = GW_GET_GROUP_MEMBERSHIP_RSP_IND__INIT;
        
        getGroupMbrRsp.sequencenumber = pMsgTableEntry->appTransId;
        getGroupMbrRsp.status = rspStatus;
        getGroupMbrRsp.srcaddress = &addressInfo;
        
        sendGwGetGroupMembershipRspInd( pMsgTableEntry->connection, &getGroupMbrRsp );
      }
      break;

    case GW_CMD_ID_T__GW_GET_SCENE_MEMBERSHIP_REQ:
      {
        GwGetSceneMembershipRspInd getSceneMbrRsp = GW_GET_SCENE_MEMBERSHIP_RSP_IND__INIT;
        
        getSceneMbrRsp.sequencenumber = pMsgTableEntry->appTransId;
        getSceneMbrRsp.status = rspStatus;
        getSceneMbrRsp.srcaddress = &addressInfo;
        
        sendGwGetSceneMembershipRspInd( pMsgTableEntry->connection, &getSceneMbrRsp );
      }
      break;
      
    case GW_CMD_ID_T__GW_GET_DEVICE_ATTRIBUTE_LIST_REQ:
      {
        GwGetDeviceAttributeListRspInd getAttrListRsp = GW_GET_DEVICE_ATTRIBUTE_LIST_RSP_IND__INIT;
        
        // Clear state machine
        gwServices_StateMachineTimeout_DeviceAttrList( pMsgTableEntry->dstAddr );
        
        getAttrListRsp.sequencenumber = pMsgTableEntry->appTransId;
        getAttrListRsp.status = GW_STATUS_T__STATUS_TIMEOUT;
        getAttrListRsp.srcaddress = &addressInfo;
        
        sendGwGetDeviceAttributeListRspInd( pMsgTableEntry->connection, &getAttrListRsp );
      }
      break;
      
    case GW_CMD_ID_T__GW_READ_DEVICE_ATTRIBUTE_REQ:
      {
        GwReadDeviceAttributeRspInd readAttrRsp = GW_READ_DEVICE_ATTRIBUTE_RSP_IND__INIT;
        
        readAttrRsp.sequencenumber = pMsgTableEntry->appTransId;
        readAttrRsp.status = rspStatus;
        readAttrRsp.srcaddress = &addressInfo;
        
        sendGwReadDeviceAttributeRspInd( pMsgTableEntry->connection, &readAttrRsp );
      }
      break;
      
    case GW_CMD_ID_T__GW_WRITE_DEVICE_ATTRIBUTE_REQ:
      {
        GwWriteDeviceAttributeRspInd writeAttrRsp = GW_WRITE_DEVICE_ATTRIBUTE_RSP_IND__INIT;
        
        writeAttrRsp.sequencenumber = pMsgTableEntry->appTransId;
        writeAttrRsp.status = rspStatus;
        writeAttrRsp.srcaddress = &addressInfo;
        
        sendGwWriteDeviceAttributeRspInd( pMsgTableEntry->connection, &writeAttrRsp );
      }
      break;
      
    case GW_CMD_ID_T__GW_SET_ATTRIBUTE_REPORTING_REQ:
      {
        GwSetAttributeReportingRspInd setAttrReportingRsp = GW_SET_ATTRIBUTE_REPORTING_RSP_IND__INIT;
        
        setAttrReportingRsp.sequencenumber = pMsgTableEntry->appTransId;
        setAttrReportingRsp.status = rspStatus;
        setAttrReportingRsp.srcaddress = &addressInfo;
        
        sendGwSetAttributeReportingRspInd( pMsgTableEntry->connection, &setAttrReportingRsp );
      }
      break;
      
    case GW_CMD_ID_T__DEV_GET_LEVEL_REQ:
      {
        DevGetLevelRspInd getLevelRsp = DEV_GET_LEVEL_RSP_IND__INIT;
        
        getLevelRsp.sequencenumber = pMsgTableEntry->appTransId;
        getLevelRsp.status = rspStatus;
        getLevelRsp.srcaddress = &addressInfo;
        
        sendDevGetLevelRspInd( pMsgTableEntry->connection, &getLevelRsp );
      }
      break;
      
    case GW_CMD_ID_T__DEV_GET_ONOFF_STATE_REQ:
      {
        DevGetOnOffStateRspInd getOnOffStateRsp = DEV_GET_ON_OFF_STATE_RSP_IND__INIT;
        
        getOnOffStateRsp.sequencenumber = pMsgTableEntry->appTransId;
        getOnOffStateRsp.status = rspStatus;
        getOnOffStateRsp.srcaddress = &addressInfo;
        
        sendDevGetOnOffStateRspInd( pMsgTableEntry->connection, &getOnOffStateRsp );
      }
      break;
      
    case GW_CMD_ID_T__DEV_GET_COLOR_REQ:
      {
        DevGetColorRspInd getColorRsp = DEV_GET_COLOR_RSP_IND__INIT;
        
        getColorRsp.sequencenumber = pMsgTableEntry->appTransId;
        getColorRsp.status = rspStatus;
        getColorRsp.srcaddress = &addressInfo;
        
        sendDevGetColorRspInd( pMsgTableEntry->connection, &getColorRsp );
      }
      break;
      
    case GW_CMD_ID_T__DEV_GET_TEMP_REQ:
      {
        DevGetTempRspInd getTempRsp = DEV_GET_TEMP_RSP_IND__INIT;
        
        getTempRsp.sequencenumber = pMsgTableEntry->appTransId;
        getTempRsp.status = rspStatus;
        getTempRsp.srcaddress = &addressInfo;
        
        sendDevGetTempRspInd( pMsgTableEntry->connection, &getTempRsp );
      }
      break;
      
    case GW_CMD_ID_T__DEV_GET_POWER_REQ:
      {
        DevGetPowerRspInd getPowerRsp = DEV_GET_POWER_RSP_IND__INIT;
        
        getPowerRsp.sequencenumber = pMsgTableEntry->appTransId;
        getPowerRsp.status = rspStatus;
        getPowerRsp.srcaddress = &addressInfo;
        
        sendDevGetPowerRspInd( pMsgTableEntry->connection, &getPowerRsp );
      }
      break;
      
    case GW_CMD_ID_T__DEV_GET_HUMIDITY_REQ:
      {
        DevGetHumidityRspInd getHumidityRsp = DEV_GET_HUMIDITY_RSP_IND__INIT;
        
        getHumidityRsp.sequencenumber = pMsgTableEntry->appTransId;
        getHumidityRsp.status = rspStatus;
        getHumidityRsp.srcaddress = &addressInfo;
        
        sendDevGetHumidityRspInd( pMsgTableEntry->connection, &getHumidityRsp );
      }
      break;
      
    case GW_CMD_ID_T__DEV_SET_DOOR_LOCK_REQ:
      {
        DevSetDoorLockRspInd setDoorLockRsp = DEV_SET_DOOR_LOCK_RSP_IND__INIT;
        
        setDoorLockRsp.sequencenumber = pMsgTableEntry->appTransId;
        setDoorLockRsp.status = rspStatus;
        setDoorLockRsp.srcaddress = &addressInfo;
        
        sendDevSetDoorLockRspInd( pMsgTableEntry->connection, &setDoorLockRsp );
      }
      break;
      
    case GW_CMD_ID_T__DEV_GET_DOOR_LOCK_STATE_REQ:
      {
        DevGetDoorLockStateRspInd getDoorLockStateRsp = DEV_GET_DOOR_LOCK_STATE_RSP_IND__INIT;
        
        getDoorLockStateRsp.sequencenumber = pMsgTableEntry->appTransId;
        getDoorLockStateRsp.status = rspStatus;
        getDoorLockStateRsp.srcaddress = &addressInfo;
        
        sendDevGetDoorLockStateRspInd( pMsgTableEntry->connection, &getDoorLockStateRsp );
      }
      break;
      
      case GW_CMD_ID_T__GW_SEND_ZCL_FRAME_REQ:
      {
	  	//no timout response is ever sent for raw-zcl transactions, because it may be impossible to easily tell whether the transaction is expecting a response.
      }
      break;
		
    default:
      uiPrintfEx(trWARNING, "Unhandled Specific Response Timeout - appTransId: %d, zclTransId: %d, cmdId: %02X\n",
                pMsgTableEntry->appTransId, pMsgTableEntry->zclTransId, pMsgTableEntry->cmdId );
      break;  
  }
}

/**************************************************************************************************
 *
 * @fn          gwRspCountHandler
 *
 * @brief       Increases the response count for commands expecting multiple responses
 *
 * @param       pMsgTableEntry - pointer to the timed out transaction table entry
 *
 * @return      none
 *
 **************************************************************************************************/
static void gwRspCountHandler( gsGwMsgTransTable_t *pMsgTableEntry )
{
  // Increase the response count
  if ( pMsgTableEntry->multRsp == TRUE )
  {
    pMsgTableEntry->rspCount++;
  }
}

/**************************************************************************************************
 *
 * @fn          gwSendSysNwkInfoReadReq
 *
 * @brief       Read the current state of the ZStack system and place in gLocalDeviceInfo
 *
 * @param       none
 *
 * @return      none
 *
 **************************************************************************************************/
static void gwSendSysNwkInfoReadReq( void )
{
  int len;
  uint8 rspcmdid;
  uint8 *pBuf;
  uint8 *pRsp;
  uint16 rsplen;
  SysNwkInfoReadReq niReq = SYS_NWK_INFO_READ_REQ__INIT;

  len = sys_nwk_info_read_req__get_packed_size( &niReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    sys_nwk_info_read_req__pack( &niReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    pRsp = apicSendSynchData( GW_ZSTACK_HANDLE, ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF, 
                              ZSTACK_CMD_IDS__SYS_NWK_INFO_READ_REQ, len, pBuf,
                              NULL, &rspcmdid, &rsplen );
    
    
    if ( pRsp )
    {
      if ( (ZSTACK_CMD_IDS__SYS_NWK_INFO_READ_RSP == rspcmdid) && (rsplen > 0) )
      {
        SysNwkInfoReadRsp *pNiRsp;
        
        pNiRsp = sys_nwk_info_read_rsp__unpack( NULL, rsplen, pRsp );
        if ( pNiRsp )
        {                
          gLocalDeviceInfo.nwkaddr = pNiRsp->nwkaddr;
          gLocalDeviceInfo.ieeeaddr = pNiRsp->ieeeaddr;
          gLocalDeviceInfo.devstate = pNiRsp->devstate;
          gLocalDeviceInfo.panid = pNiRsp->panid;
          gLocalDeviceInfo.extendedpanid = pNiRsp->extendedpanid;
          gLocalDeviceInfo.coordaddr = pNiRsp->coordaddr;
          gLocalDeviceInfo.coordextaddr = pNiRsp->coordextaddr;
          
          gLocalDeviceType.coodinator = pNiRsp->devtypes->coodinator;
          gLocalDeviceType.router = pNiRsp->devtypes->router;
          gLocalDeviceType.enddevice = pNiRsp->devtypes->enddevice;
          gLocalDeviceInfo.devtypes = &gLocalDeviceType;
          
          gLocalDeviceInfo.logicalchannel = pNiRsp->logicalchannel;
          
          uiPrintf( "NwkInfoReadRsp:\n" );
          uiPrintf( "- nwkaddr:%04X\n", gLocalDeviceInfo.nwkaddr);
          uiPrintf( "- ieeeaddr:%016llX\n", gLocalDeviceInfo.ieeeaddr);
          uiPrintf( "- devstate:%d\n", gLocalDeviceInfo.devstate);
          uiPrintf( "- panid:%04X\n", gLocalDeviceInfo.panid);
          uiPrintf( "- extendedpanid:%016llX\n", gLocalDeviceInfo.extendedpanid );
          uiPrintf( "- coordaddr:%04X\n", gLocalDeviceInfo.coordaddr);
          uiPrintf( " - coordextaddr:%016llX\n", gLocalDeviceInfo.coordextaddr );
          uiPrintf( "- devtypes: Coordinator: %d\n", gLocalDeviceInfo.devtypes->coodinator );
          uiPrintf( "- devtypes: Router: %d\n", gLocalDeviceInfo.devtypes->router );
          uiPrintf( "- logicalchannel:%d\n\n", gLocalDeviceInfo.logicalchannel );

          sys_nwk_info_read_rsp__free_unpacked( pNiRsp, NULL );
        }
      }
      else
      {
        uiPrintf( "Expected NwkInfoReadRsp, got %d\n", rspcmdid );
      }
      
      apicFreeSynchData( pRsp );
    }

    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          gwIncreaseCurrentAppSeqNum
 *
 * @brief       Handles incrementing and rolling the application sequence number
 *
 * @param       none
 *
 * @return      none
 *
 **************************************************************************************************/
static uint16 gwIncreaseCurrentAppSeqNum( void )
{
  uint16 appSeqBuf;
  
  appSeqBuf = gGwAppTransSeqNum++;
  // Roll the sequence number back to 0
  
  if ( gGwAppTransSeqNum == 0xFFFF )
  {
    gGwAppTransSeqNum = 0;
  }
  
  return appSeqBuf;
}

/**************************************************************************************************
 *
 * @fn          gwSetSelfAddressStruct
 *
 * @brief       Handles setting address structure information for self-addressed requests
 *
 * @param       appSeqNum - application sequence number
 * @param       pPbAddrStruct - pointer to address structure
 *
 * @return      none
 *
 **************************************************************************************************/
static void gwSetSelfAddressStruct( uint16 appSeqNum, GwAddressStructT *pPbAddrStruct )
{
  if ( appSeqNum == 0xFFFF )
  {
    // Make sure all fields are set accordingly
    pPbAddrStruct->addresstype = GW_ADDRESS_TYPE_T__NONE;
    pPbAddrStruct->has_ieeeaddr = TRUE;
    pPbAddrStruct->ieeeaddr = 0;
  }
}

/**************************************************************************************************
 *
 * @fn          gwAnalogDataType
 *
 * @brief       Returns length for analog data types, 0 for unsupported/discrete data types
 *
 * @param       dataType - attribute's data type value
 *
 * @return      Length of analog data types, 0 for unsupported/discrete data types
 *
 **************************************************************************************************/
static uint8 gwAnalogDataType( uint8 dataType )
{
  // Check if analog data type
  if ( zclAnalogDataType( dataType ) )
  {
    // Return size
    return zclGetDataTypeLength( dataType );
  }
  
  return 0; // not supported
}

/**************************************************************************************************
 *
 * @fn          gwHandleMSPWCommands
 *
 * @brief       Send ZCL Frame Indication to app for unhandled commands
 *
 * @param       pInMsg - The incoming message
 *
 * @return      success of failure
 *
 **************************************************************************************************/
static ZStatus_t gwHandleMSPWCommands(zclIncoming_t * pInMsg) //MSPW stands for ManufacturerSpecificProfileWide
{
  ZStatus_t status = ZFailure;

  GwAddressStructT srcAddress = GW_ADDRESS_STRUCT_T__INIT;
  
  // Store address information		  
  srcAddress.addresstype = GW_ADDRESS_TYPE_T__UNICAST;
  
  srcAddress.has_ieeeaddr = TRUE;
  
  if ( pInMsg->msg->srcAddr.addrMode == afAddr16Bit )
  {
	gwPb_SrvrGetIeeeAddress( pInMsg->msg->srcAddr.addr.shortAddr, &srcAddress.ieeeaddr );
  }
  else if ( pInMsg->msg->srcAddr.addrMode == afAddr64Bit )
  {
	srcAddress.ieeeaddr = BUILD_UINT64( pInMsg->msg->srcAddr.addr.extAddr[0], pInMsg->msg->srcAddr.addr.extAddr[1], 
									  pInMsg->msg->srcAddr.addr.extAddr[2], pInMsg->msg->srcAddr.addr.extAddr[3], 
									  pInMsg->msg->srcAddr.addr.extAddr[4], pInMsg->msg->srcAddr.addr.extAddr[5], 
									  pInMsg->msg->srcAddr.addr.extAddr[6], pInMsg->msg->srcAddr.addr.extAddr[7] );
  }
  
  srcAddress.has_endpointid = TRUE;
  srcAddress.endpointid = pInMsg->msg->srcAddr.endPoint;
  
  status = processGwZclFrameReceiveInd( ALL_APP_CONNECTIONS, &srcAddress, pInMsg );

  return status;
}

/******************************************************************************
 * @fn          gwGetDevNwkInfo
 *
 * @brief       Get the NIB info fields.
 *
 */
void gwGetDevNwkInfo( nwkIB_t *pNib )
{
  uint8 *pRsp, rspcmdid;
  uint16 rsplen;
  SendGetDevNwkInfoReq *pInMsg;

  // send request to NPI synchronously
  pRsp = apicSendSynchData( GW_ZSTACK_HANDLE, ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF, ZSTACK_CMD_IDS__UTIL_GET_DEV_NWK_INFO_REQ,
      0, NULL, NULL, &rspcmdid, &rsplen );

  if ( (pRsp != NULL) && (rspcmdid == ZSTACK_CMD_IDS__SEND_GET_DEV_NWK_INFO_REQ) )
  {
    pInMsg = send_get_dev_nwk_info_req__unpack(NULL, rsplen, pRsp);

    if(pInMsg)
    {
      pNib->nwkDevAddress = pInMsg->nwkdevaddress;
      pNib->nwkPanId = pInMsg->nwkpanid;
      pNib->SequenceNum = pInMsg->sequencenum;
      pNib->nwkLogicalChannel = pInMsg->nwklogicalchannel;

      send_get_dev_nwk_info_req__free_unpacked(pInMsg, NULL);
    }
    apicFreeSynchData( pRsp );
  }
}

/******************************************************************************
 * @fn          gwSetDevNwkInfo
 *
 * @brief       Get the NIB info fields.
 *
 */
void gwSetDevNwkInfo( nwkIB_t *pNib )
{
  int len;
  uint8 *pBuf;
  //uint8 status;
  UtilSetDevNwkInfoReq inMsg = UTIL_SET_DEV_NWK_INFO_REQ__INIT;

  inMsg.nwkdevaddress = pNib->nwkDevAddress;
  inMsg.nwkpanid = pNib->nwkPanId;
  inMsg.sequencenum = pNib->SequenceNum;
  inMsg.nwklogicalchannel = pNib->nwkLogicalChannel;

  len = util_set_dev_nwk_info_req__get_packed_size( &inMsg );

  pBuf = malloc( len );
  if ( pBuf )
  {
    len = util_set_dev_nwk_info_req__pack( &inMsg, pBuf );

    // send request to NPI synchronously
    //status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__UTIL_SET_DEV_NWK_INFO_REQ, len, pBuf );
	sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__UTIL_SET_DEV_NWK_INFO_REQ, len, pBuf );

    free( pBuf );
  }
}

/******************************************************************************
 * @fn          gwGpAddressConflict
 *
 * @brief       To send NWK Status with address conflict.
 *
 */
void gwGpAddressConflict( uint16 addr )
{
  int len;
  uint8 *pBuf;
  AFAddr inMsg = AFADDR__INIT;

  inMsg.addrmode = SADDR_MODE_SHORT;
  inMsg.has_shortaddr = true;
  inMsg.shortaddr = addr;

  len = afaddr__get_packed_size( &inMsg );

  pBuf = malloc( len );
  if ( pBuf )
  {
    len = afaddr__pack( &inMsg, pBuf );
    uiPrintfEx(trERROR, "ZSTACK_CMD_IDS__GP_ADDRESS_CONFLICT : %d : 0x%08x", len, inMsg.shortaddr );

    // send request to NPI synchronously
    sendAPICExpectDefaultStatus(ZSTACK_CMD_IDS__GP_ADDRESS_CONFLICT, len, pBuf );

    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          gwSendDeviceAnnounce
 *
 * @brief       Send ZDO device announce message
 *
 * @param       ZdoDeviceAnnounceReq - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static ZStatusValues gwSendDeviceAnnounce( ZdoDeviceAnnounceReq *pZdoDeviceAnnounceReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;

  len = zdo_device_announce_req__get_packed_size( pZdoDeviceAnnounceReq);
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_device_announce_req__pack( pZdoDeviceAnnounceReq, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_DEVICE_ANNOUNCE_REQ, len, pBuf );

    free( pBuf );
  }

  return( status );
}

/**************************************************************************************************
 *
 * @fn      processZdoDeviceAnnounce
 *
 * @brief   Process ZDO Device Announce message
 *
 * @param   pInMsg - pointer to message structure
 *
 * @return  none
 *
 **************************************************************************************************/
static void processZdoDeviceAnnounce( ZdoDeviceAnnounceInd *pInMsg )
{
  ZDO_DeviceAnnce_t devAnnce;
  uint8 invalidIEEE[Z_EXTADDR_LEN] = {0xFF};
  uint32 timeout;
  uint8 invalidAddr;
  uint8 sameAnnce;

  devAnnce.extAddr[0] = BREAK_UINT64( pInMsg->devextaddr, 0 );
  devAnnce.extAddr[1] = BREAK_UINT64( pInMsg->devextaddr, 1 );
  devAnnce.extAddr[2] = BREAK_UINT64( pInMsg->devextaddr, 2 );
  devAnnce.extAddr[3] = BREAK_UINT64( pInMsg->devextaddr, 3 );
  devAnnce.extAddr[4] = BREAK_UINT64( pInMsg->devextaddr, 4 );
  devAnnce.extAddr[5] = BREAK_UINT64( pInMsg->devextaddr, 5 );
  devAnnce.extAddr[6] = BREAK_UINT64( pInMsg->devextaddr, 6 );
  devAnnce.extAddr[7] = BREAK_UINT64( pInMsg->devextaddr, 7 );
  devAnnce.nwkAddr = pInMsg->devaddr;
  devAnnce.capabilities = 0;

  timeout = gpProxyAliasConflictTimeout;
  invalidAddr = !memcmp( devAnnce.extAddr, invalidIEEE, Z_EXTADDR_LEN );
  sameAnnce = !memcmp( &devAnnce, &aliasConflictAnnce, sizeof( ZDO_DeviceAnnce_t ) );

  // Check GP proxy table to update the entry if necesary
  if( timeout && ( invalidAddr && sameAnnce ) )
  {
    gpProxyAliasConflictTimeout = 0;
  }
  else
  {
    gp_CheckAnnouncedDevice( devAnnce.extAddr, devAnnce.nwkAddr );
  }
}
