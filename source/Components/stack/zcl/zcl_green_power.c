/**************************************************************************************************
  Filename:       zcl_green_power.c
  Revised:        $Date: 2015-09-10 09:36:48 -0700 (Thu, 10 Sep 2015) $
  Revision:       $Revision: 44493 $

  Description:    Zigbee Cluster Library - General.  This application receives all
                  ZCL messages and initially parses them before passing to application.


  Copyright 2006-2015 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License"). You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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



/*********************************************************************
 * INCLUDES
 */

#include "ZGlobals.h"  
#include "ZComDef.h"
#include "zcl.h"
#include "zcl_green_power.h"
#include "stub_aps.h"
#include "gp_common.h"
#include "OSAL.h"
#include "gateway.pb-c.h"
#include "trace.h"
#include "gatewaysrvr.h"

#if !defined (DISABLE_GREENPOWER_BASIC_PROXY)

/*********************************************************************
 * MACROS
 */
#define ALL_APP_CONNECTIONS         -1    // send commands to all connected apps

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

typedef struct zclGpCBRec
{
  struct zclGpCBRec        *next;
  uint8                     endpoint; // Used to link it into the endpoint descriptor
  zclGp_AppCallbacks_t     *CBs;      // Pointer to Callback function
} zclGpCBRec_t;


/*********************************************************************
 * GLOBAL VARIABLES
 */
ProxyTableEntryFormat_t proxyTableEntry[GPP_MAX_PROXY_TABLE_ENTRIES];

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static gpNotificationMsg_t *pNotificationMsgHead = NULL;
static gpCmdPayloadMsg_t *pCmdPayloadMsgHead = NULL;

//static zclGenSceneItem_t *zclGenSceneTable = (zclGenSceneItem_t *)NULL;


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static ZStatus_t zclGp_HdlInSpecificCommands( zclIncoming_t *pInMsg );
static uint8 gp_addPairedSinksToMsgQueue( uint8 appId, uint8 *pId, gpCmdPayloadMsg_t* pMsg );
static ZStatus_t zclGp_ProcessInProxyBasic( zclIncoming_t *pInMsg );


/*********************************************************************
 * @fn      zclGp_HdlIncoming
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library or Profile commands for attributes
 *          that aren't in the attribute list
 *
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGp_HdlIncoming( zclIncoming_t *pInMsg )
{
  ZStatus_t stat = ZSuccess;

#if defined ( INTER_PAN )
  if ( StubAPS_InterPan( pInMsg->msg->srcAddr.panId, pInMsg->msg->srcAddr.endPoint ) )
    return ( stat ); // Cluster not supported thru Inter-PAN
#endif
  if ( zcl_ClusterCmd( pInMsg->hdr.fc.type ) )
  {
    // Is this a manufacturer specific command?
    if ( pInMsg->hdr.fc.manuSpecific == 0 )
    {
      stat = zclGp_HdlInSpecificCommands( pInMsg );
    }
    else
    {
      // We don't support any manufacturer specific command.
      stat = ZFailure;
    }
  }
  else
  {
    // Handle all the normal (Read, Write...) commands -- should never get here
    stat = ZFailure;
  }
  return ( stat );
}

/*********************************************************************
 * @fn      zclGp_HdlInSpecificCommands
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library

 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGp_HdlInSpecificCommands( zclIncoming_t *pInMsg )
{
  ZStatus_t stat;

  switch ( pInMsg->msg->clusterId )
  {
    case ZCL_CLUSTER_ID_GREEN_POWER:
      // Proxy Basic handler
      stat = zclGp_ProcessInProxyBasic( pInMsg );
      break;

    default:
      stat = ZFailure;
      break;
  }

  return ( stat );
}

/*********************************************************************
 * @fn      zclGp_ProcessInProxyBasic
 *
 * @brief   Process in the received client commands for the Proxy Basic.
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclGp_ProcessInProxyBasic( zclIncoming_t *pInMsg )
{
  // Server side command handler
  if ( zcl_ServerCmd( pInMsg->hdr.fc.direction ) )
  {
    // Server side command handling is not supported
    zclDefaultRspCmd_t defaultRspCmd;

    defaultRspCmd.commandID = pInMsg->hdr.commandID;
    defaultRspCmd.statusCode = ZCL_STATUS_UNSUP_CLUSTER_COMMAND;
    zcl_SendDefaultRspCmd( pInMsg->msg->endPoint, &(pInMsg->msg->srcAddr),
                           pInMsg->msg->clusterId, &defaultRspCmd,
                           ZCL_FRAME_SERVER_CLIENT_DIR, true, 0, pInMsg->hdr.transSeqNum );
  }
  // Client side command handler
  if ( zcl_ClientCmd( pInMsg->hdr.fc.direction ) )
  {
    switch ( pInMsg->hdr.commandID )
    {
      zclGpPairing_t gpPairngCmd;
      zclGpProxyTableRequest_t gpTblReq;
      zclGpProxyCommissioningMode_t gpCommissioningModeCmd;
      zclGpResponse_t gpRspCmd;

      case COMMAND_GP_PAIRING:

        zcl_InSeqNum = pInMsg->hdr.transSeqNum;

        gpPairngCmd.srcAddr = &(pInMsg->msg->srcAddr);
        zcl_memcpy( gpPairngCmd.options, (void*)pInMsg->pData, sizeof(gpPairngCmd.options) );
        pInMsg->pData += sizeof(gpPairngCmd.options);  // Move data pointer after options field

        if ( pInMsg->pData == NULL )
        {
          return ZFailure;
        }

        gpPairngCmd.pData = pInMsg->pData;
        zclGp_GpPairingCommand( &gpPairngCmd );
        break;
        
      case COMMAND_GP_PROXY_TABLE_REQUEST:

        zcl_InSeqNum = pInMsg->hdr.transSeqNum;

        gpTblReq.srcAddr = &(pInMsg->msg->srcAddr);
        zcl_memcpy( &gpTblReq.options, (void*)pInMsg->pData, sizeof(gpTblReq.options) );
        pInMsg->pData += sizeof(gpTblReq.options);  // Move data pointer after options field

        if ( pInMsg->pData == NULL )
        {
          return ZFailure;
        }

        gpTblReq.pData = (void*)pInMsg->pData;
        zclGp_GpProxyTableReq( &gpTblReq );

        break;

      case COMMAND_GP_PROXY_COMMISSIONING_MODE:

        zcl_memcpy( &gpCommissioningModeCmd.options, (void*)pInMsg->pData, sizeof(gpCommissioningModeCmd.options) );
        pInMsg->pData += sizeof(gpCommissioningModeCmd.options);  // Move data pointer after options field

        if ( pInMsg->pData == NULL )
        {
          return ZFailure;
        }

        gpPairngCmd.pData = pInMsg->pData;

        if(pInMsg->msg->srcAddr.addrMode == afAddr16Bit)
        {
        	gpCommissioningModeCmd.srcAddr = pInMsg->msg->srcAddr.addr.shortAddr;
        }

        zclGp_GpProxyCommissioningMode( &gpCommissioningModeCmd );

        break;

        case COMMAND_GP_RESPONSE:

          zcl_memcpy( &gpRspCmd.options, (void*)pInMsg->pData, sizeof(gpRspCmd.options) );
          pInMsg->pData += sizeof(gpRspCmd.options);  // Move data pointer after options field

          gpRspCmd.tempMasterShortAddr = zcl_build_uint16(pInMsg->pData);
          pInMsg->pData += sizeof(gpRspCmd.tempMasterShortAddr);  // Move data pointer after tempMaster field

          zcl_memcpy( &gpRspCmd.tempMasterTxChannel, (void*)pInMsg->pData, sizeof(gpRspCmd.tempMasterTxChannel) );
          pInMsg->pData += sizeof(gpRspCmd.tempMasterTxChannel);  // Move data pointer after options field

          gpRspCmd.dstAddr = pInMsg->msg->macDestAddr;

          if ( pInMsg->pData == NULL )
          {
            return ZFailure;
          }

          gpPairngCmd.pData = pInMsg->pData;

          zclGp_GpResponseCommand( &gpRspCmd );

        break;
    }
  }

  return ( ZSuccess );
}

/*********************************************************************
 * @fn      zclGp_SendGpNotificationCommand
 *
 * @brief   Send the Green Power Notification Command to a device
 *
 * @param   pCmd - Pointer to GP Notification Struct
 *          secNum - Sequence number given by the GPDF  
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGp_SendGpNotificationCommand( gpNotificationCmd_t *pCmd, uint8 secNum )
{
  uint8 *buf = NULL;
  uint8 *pBuf = NULL;
  uint8 *pGpdId = NULL;
  uint8 appId;
  ZStatus_t status;
  uint16 len = 11;  // options + GPD Sec Counter + Cmd ID + payloadLen + GPP Short Address + GPP-GPD link
  gpCmdPayloadMsg_t *pCmdPayloadMsgCurr = NULL;

  // Check some stuff to calculate the packet len
  // If Application Id bitfield is 0b000
  if( GP_IS_APPLICATION_ID_GPD( pCmd->options ) )
  {
    len += 4;  // GPD ID
  }
  // If Application Id bitfield is 0b010
  else if( GP_IS_APPLICATION_ID_IEEE( pCmd->options ) )
  {
    len += 9;  // IEEE addr + EP
  }
  
  // Length of the command payload
  len += pCmd->payloadLen;

  buf = zcl_mem_alloc( len );
  if ( buf )
  {
    pBuf = buf;
    
    zcl_memcpy( pBuf, &pCmd->options, sizeof ( uint16 ) );
    pBuf += sizeof ( uint16 );
    
    // If Application Id bitfield is 0b000
    if( GP_IS_APPLICATION_ID_GPD( pCmd->options ) )
    {
      zcl_memcpy( pBuf, &pCmd->gpdId, sizeof ( uint32 ) );
      pGpdId = ( uint8* )&pCmd->gpdId;
      appId = GP_OPT_APP_ID_GPD;
      pBuf += sizeof ( uint32 );
    }
    // If Application Id bitfield is 0b010
    else if( GP_IS_APPLICATION_ID_IEEE( pCmd->options ) )
    {
      zcl_memcpy( pBuf, &pCmd->gpdIEEE, Z_EXTADDR_LEN );
      pGpdId = pCmd->gpdIEEE;
      appId = GP_OPT_APP_ID_IEEE;
      pBuf += Z_EXTADDR_LEN;
      *pBuf++ = pCmd->ep;
    }
    
    zcl_memcpy( pBuf, &pCmd->gpdSecCounter, sizeof ( uint32 ) );
    pBuf += sizeof ( uint32 );

    *pBuf++ = pCmd->cmdId;
    
    *pBuf++ = pCmd->payloadLen;
    if ( pCmd->payloadLen > 0 )
    {
      zcl_memcpy( pBuf, &pCmd->cmdPayload, pCmd->payloadLen );
      pBuf += pCmd->payloadLen;
    }
    
    zcl_memcpy( pBuf, &pCmd->gppShortAddr, sizeof ( uint16 ) );
    pBuf += sizeof ( uint16 );
    
    *pBuf++ = pCmd->gppGpdLink;
    
    pCmdPayloadMsgCurr = gp_AddCmdPayloadMsgNode ( &pCmdPayloadMsgHead, buf, len );

    if ( pCmdPayloadMsgCurr != NULL )
    {
      pCmdPayloadMsgCurr->secNum = secNum;
      gp_addPairedSinksToMsgQueue( appId, pGpdId, pCmdPayloadMsgCurr );
    }

  }
  else
    status = ZMemError;

  return ( status );
}
  
/*********************************************************************
 * @fn      zclGp_SendGpCommissioningNotificationCommand
 *
 * @brief   Send the Green Power Commissioning Notification Command to a device
 *
 * @param   pCmd - Pointer to GP Commissioning Notification Struct
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGp_SendGpCommissioningNotificationCommand( gpCommissioningNotificationCmd_t *pCmd )
{
  uint8 *buf = NULL;
  uint8 *pBuf = NULL;
  ZStatus_t status;
  uint16 len = 11;  // options + GPD Sec Counter + Cmd ID + payloadLen + GPP Short Address + GPP-GPD link
  nwkIB_t _NIB =  {0};

  //gwExtNwkInfo( &_NIB );
  
  // Check some stuff to calculate the packet len
  // If Application Id bitfield is 0b000
  if( GP_IS_APPLICATION_ID_GPD( pCmd->options ) )
  {
    len += 4;  // GPD ID
  }
  // If Application Id bitfield is 0b010
  else if( GP_IS_APPLICATION_ID_IEEE( pCmd->options ) )
  {
    len += 9;  // IEEE addr + EP
  }
  
  // Length of the command payload
  len += pCmd->payloadLen;

  buf = zcl_mem_alloc( len );
  if ( buf )
  {
    pBuf = buf;
    
    zcl_memcpy( pBuf, &pCmd->options, sizeof ( uint16 ) );
    pBuf += sizeof ( uint16 );
    
    // If Application Id bitfield is 0b000
    if( GP_IS_APPLICATION_ID_GPD( pCmd->options ) )
    {
      zcl_memcpy( pBuf, &pCmd->gpdId, sizeof ( uint32 ) );
      pBuf += sizeof ( uint32 );
    }
    // If Application Id bitfield is 0b010
    else if( GP_IS_APPLICATION_ID_IEEE( pCmd->options ) )
    {
      zcl_memcpy( pBuf, &pCmd->gpdIEEE, Z_EXTADDR_LEN );
      pBuf += Z_EXTADDR_LEN;
      *pBuf++ = pCmd->ep;
    }
    
    zcl_memcpy( pBuf, &pCmd->gpdSecCounter, sizeof ( uint32 ) );
    pBuf += sizeof ( uint32 );

    *pBuf++ = pCmd->cmdId;
    
    *pBuf++ = pCmd->payloadLen;
    if ( pCmd->payloadLen > 0 )
    {
      zcl_memcpy( pBuf, &pCmd->cmdPayload, pCmd->payloadLen );
      pBuf += pCmd->payloadLen;
    }
    
    zcl_memcpy( pBuf, &pCmd->gppShortAddr, sizeof ( uint16 ) );
    pBuf += sizeof ( uint16 );
    
    *pBuf++ = pCmd->gppGpdLink;
    
    afAddrType_t addr;
    addr.endPoint = GREEN_POWER_INTERNAL_ENDPOINT;
    addr.panId = _NIB.nwkPanId;
    addr.addr.shortAddr = gp_commissionerAddress; 
        
    if ( gp_commissionerAddress == 0xFFFF )
    {
      addr.addrMode = afAddrBroadcast;
    }
    else
    {
      addr.addrMode = afAddr16Bit;
    }
        
    zcl_SendCommand( GREEN_POWER_INTERNAL_ENDPOINT, &addr, ZCL_CLUSTER_ID_GREEN_POWER,
                      COMMAND_GP_COMMISSIONING_NOTIFICATION, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
                      TRUE, 0,  5/*TODO:bdb_getZCLFrameCounter()*/, len, buf );
      
    zcl_mem_free( buf );
  }
  else
    status = ZMemError;

  return ( status );
}
/*********************************************************************
 * @fn      zclGp_SendGpProxyTableResponse
 *
 * @brief   Send the Green Power Proxy Table Response to a device
 *
 * @param   dstAddr - where to send the request
 * @param   groupID - pointer to the group structure
 * @param   groupName - pointer to Group Name.  This is a Zigbee
 *          string data type, so the first byte is the length of the
 *          name (in bytes), then the name.
 *
 * @return  ZStatus_t
 */
ZStatus_t zclGp_SendGpProxyTableResponse( afAddrType_t *dstAddr, zclGpProxyTableResponse_t *rsp, 
                                          uint8 seqNum )
{
  uint8 *buf = NULL;
  uint8 *pBuf = NULL;
  uint8 *pEntry = NULL;
  
  ZStatus_t status;
  uint16 entryLen = 0;
  uint16 len = 0;
  
  if ( rsp->proxyTableEntry != NULL )
  {
    pEntry = rsp->proxyTableEntry;
    zcl_memcpy( &entryLen, pEntry, sizeof ( uint16 ) );
    pEntry += sizeof ( uint16 );
  }
  
  len = entryLen + 4;

  buf = zcl_mem_alloc( len );
  if ( buf )
  {
    pBuf = buf;
    *pBuf++ = rsp->status;
    *pBuf++ = rsp->tableEntriesTotal;
    *pBuf++ = rsp->startIndex;
    *pBuf++ = rsp->entriesCount;
    if ( rsp->proxyTableEntry != NULL )
    {
      zcl_memcpy( pBuf, pEntry, entryLen );
    }
    status = zcl_SendCommand( GREEN_POWER_INTERNAL_ENDPOINT, dstAddr, ZCL_CLUSTER_ID_GREEN_POWER,
                              COMMAND_GP_PROXY_TABLE_RESPONSE, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
                              TRUE, 0, seqNum, len, buf );
    zcl_mem_free( buf );
  }
  else
    status = ZMemError;

  return ( status );
}

 /*********************************************************************
 * @fn          gp_addPairedSinksToMsgQueue
 *
 * @brief       General function to add Notification Msg to Queue for all 
 *              Sinks paired to the given GPD
 *
 * @param       nvIndex - NV Id of proxy table
 *              pEntry  - pointer to PROXY_TBL_ENTRY_LEN array
 *
 * @return      
 */
static uint8 gp_addPairedSinksToMsgQueue( uint8 appId, uint8 *pId, gpCmdPayloadMsg_t* pMsg )
{
  uint8 freeSinkEntry[LSINK_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8 status;
  uint8 i;
  uint8 currEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  afAddrType_t dstAddr;
  gpNotificationMsg_t *pNotificationMsgCurr = NULL;
  nwkIB_t _NIB =  {0};

  //gwExtNwkInfo( &_NIB );
    
  for ( i = 0; i < GPP_MAX_PROXY_TABLE_ENTRIES ; i++ )
  {
    status = gp_getProxyTableByIndex( ( ZCD_NV_PROXY_TABLE_START + i ), currEntry );
    
    if ( status == NV_OPER_FAILED )
    {
      break;
    }
    
    // if the entry is empty
    if ( status == NV_INVALID_DATA )
    {
      if ( i >= GPP_MAX_PROXY_TABLE_ENTRIES )
      {
        return status;
      }
      continue;
    }

    if ( PROXY_TBL_COMP_APPLICTION_ID( appId, currEntry[PROXY_TBL_ENTRY_OPT] ) )
    {
      if ( appId == GP_OPT_APP_ID_GPD )
      {
        if ( zcl_memcmp( pId, &currEntry[PROXY_TBL_ENTRY_GPD_ID + 4], sizeof ( uint32 ) ) )
        {
          // Entry found
          break;
        }
      }
      else if ( appId == GP_OPT_APP_ID_IEEE )
      {
        if ( zcl_memcmp( pId, &currEntry[PROXY_TBL_ENTRY_GPD_ID], Z_EXTADDR_LEN ) )
        {
          // Entry found
          break;
        }
      }
    }
  }
  
  dstAddr.endPoint = GREEN_POWER_INTERNAL_ENDPOINT;
  dstAddr.panId = _NIB.nwkPanId;
  
  if ( PROXY_TBL_GET_LIGHTWIGHT_UNICAST( currEntry[PROXY_TBL_ENTRY_OPT] ) )
  {
    dstAddr.addrMode = afAddr16Bit;
    if ( !zcl_memcmp( freeSinkEntry, &currEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], LSINK_ADDR_LEN ) )
    {
      zcl_memcpy( &dstAddr.addr.shortAddr, &currEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR + Z_EXTADDR_LEN], sizeof ( uint16 ) );
      
      pNotificationMsgCurr = gp_AddNotificationMsgNode( &pNotificationMsgHead, pMsg );
      if ( pNotificationMsgCurr != NULL )
      {
        pNotificationMsgCurr->addr = dstAddr;
      }
    }
    if ( !zcl_memcmp( freeSinkEntry, &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], LSINK_ADDR_LEN ) )
    {
      zcl_memcpy( &dstAddr.addr.shortAddr, &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR + Z_EXTADDR_LEN], sizeof ( uint16 ) );
      
      pNotificationMsgCurr = gp_AddNotificationMsgNode( &pNotificationMsgHead, pMsg );
      if ( pNotificationMsgCurr != NULL )
      {
        pNotificationMsgCurr->addr = dstAddr;
      }
    }
  }
  
  if ( PROXY_TBL_GET_DGROUP( currEntry[PROXY_TBL_ENTRY_OPT] ) || PROXY_TBL_GET_CGROUP( currEntry[PROXY_TBL_ENTRY_OPT + 1] ) )
  {
    dstAddr.addrMode = afAddrGroup;
    
    if ( GET_BIT( &currEntry[PROXY_TBL_ENTRY_GRP_TBL_ENTRIES], PROXY_TBL_ENTRY_1ST_GRP_BIT ) )
    {      
      zcl_memcpy( &dstAddr.addr.shortAddr, &currEntry[PROXY_TBL_ENTRY_1ST_GRP_ADDR], sizeof ( uint16 ) );
      
      pNotificationMsgCurr = gp_AddNotificationMsgNode( &pNotificationMsgHead, pMsg );
      if ( pNotificationMsgCurr != NULL )
      {
        pNotificationMsgCurr->addr = dstAddr;
      }
    }
    if ( GET_BIT( &currEntry[PROXY_TBL_ENTRY_GRP_TBL_ENTRIES], PROXY_TBL_ENTRY_2ND_GRP_BIT ) )
    {
      zcl_memcpy( &dstAddr.addr.shortAddr, &currEntry[PROXY_TBL_ENTRY_2ND_GRP_ADDR], sizeof ( uint16 ) );
      
      pNotificationMsgCurr = gp_AddNotificationMsgNode( &pNotificationMsgHead, pMsg );
      if ( pNotificationMsgCurr != NULL )
      {
        pNotificationMsgCurr->addr = dstAddr;
      }
    }
  }

  return status;
}

/*********************************************************************
 * @fn      gp_CreateNotificationMsgList
 *
 * @brief   Create Notification Msg List for paired sinks if empty
 *
 * @param   pHead - pointer to a pointer of the list head
 *
 * @return  none
 */
void gp_CreateNotificationMsgList( gpNotificationMsg_t **pHead )
{

  // Create the list if empty
  if ( *pHead == NULL )
  {
    *pHead = ( gpNotificationMsg_t* )zcl_mem_alloc( sizeof( gpNotificationMsg_t ) );
    
    if ( *pHead != NULL )
    {
      (*pHead)->pNext = NULL;
    }
  }
  return;
}

/*********************************************************************
 * @fn      gp_CreateCmdPayloadMsgList
 *
 * @brief   Create Notification Msg List for paired sinks if empty
 *
 * @param   pHead - pointer to a pointer of the list head
 *
 * @return  none
 */
void gp_CreateCmdPayloadMsgList( gpCmdPayloadMsg_t **pHead )
{

  // Create the list if empty
  if ( *pHead == NULL )
  {
    *pHead = ( gpCmdPayloadMsg_t* )zcl_mem_alloc( sizeof( gpCmdPayloadMsg_t ) );
    ( ( gpCmdPayloadMsg_t* )*pHead )->pMsg = NULL;
    
    if ( *pHead != NULL )
    {
      (*pHead)->pNext = NULL;
    }
  }
  return;
}

/*********************************************************************
 * @fn      gp_AddNotificationMsgNode
 *
 * @brief   Add node to Notification Msg list
 *
 * @param   pHead - pointer to a pointer of the list head
 *
 * @return  pointer to new node
 */
gpNotificationMsg_t* gp_AddNotificationMsgNode( gpNotificationMsg_t **pHead, gpCmdPayloadMsg_t *pMsg )
{
  gpNotificationMsg_t **pCurr;
  
    // Create respondent list if empty
  if ( *pHead == NULL )
  {
    gp_CreateNotificationMsgList( pHead );
    ( ( gpNotificationMsg_t* ) *pHead )->pMsg = ( uint8* )pMsg;
    return *pHead;
  }
  
  pCurr = &((*pHead)->pNext);
  
  while ( *pCurr != NULL )
  {
    pCurr = &((*pCurr)->pNext);
  }

  *pCurr = ( gpNotificationMsg_t* )zcl_mem_alloc( sizeof( gpNotificationMsg_t ) );
    
  if ( (*pCurr)->pNext != NULL )
  {
    (*pCurr)->pNext = NULL;
  }
  ( ( gpNotificationMsg_t* ) *pCurr )->pMsg = ( uint8* )pMsg;
  return *pCurr;
}

/*********************************************************************
 * @fn      gp_AddCmdPayloadMsgNode
 *
 * @brief   Add node to Notification Msg list
 *
 * @param   pHead - pointer to a pointer of the list head
 *
 * @return  pointer to new node
 */
gpCmdPayloadMsg_t* gp_AddCmdPayloadMsgNode( gpCmdPayloadMsg_t **pHead, uint8* pBuf, uint8 len )
{
  gpCmdPayloadMsg_t **pCurr;
  
    // Create respondent list if empty
  if ( *pHead == NULL )
  {
    gp_CreateCmdPayloadMsgList( pHead );
    
    if ( pHead != NULL )
    {
      ( ( ( gpCmdPayloadMsg_t* ) *pHead )->lenght ) = len;
      ( ( ( gpCmdPayloadMsg_t* ) *pHead )->pMsg ) = ( uint8* )pBuf;
    }
    return *pHead;
  }
  
  pCurr = &((*pHead)->pNext);
  
  while ( *pCurr != NULL )
  {
    pCurr = &((*pCurr)->pNext);
  }

  *pCurr = ( gpCmdPayloadMsg_t* )zcl_mem_alloc( sizeof( gpCmdPayloadMsg_t ) );
    
  if ( (*pCurr)->pNext != NULL )
  {
    (*pCurr)->pNext = NULL;
  }
  ( ( ( gpCmdPayloadMsg_t* ) *pCurr )->lenght ) = len;
  ( ( ( gpCmdPayloadMsg_t* ) *pCurr )->pMsg ) = ( uint8* )pBuf;
  
  return *pCurr;
}

/*********************************************************************
 * @fn      gp_GetHeadNotificationMsg
 *
 * @brief   Returns head pointer for  finding and binding respondent list
 *
 * @param   none
 *
 * @return  pointer to head
 */
gpNotificationMsg_t* gp_GetHeadNotificationMsg(void)
{
  return ( pNotificationMsgHead );
}

/*********************************************************************
 * @fn      gp_GetPHeadNotification
 *
 * @brief   Returns head pointer for  finding and binding respondent list
 *
 * @param   none
 *
 * @return  pointer to head
 */
gpNotificationMsg_t** gp_GetPHeadNotification(void)
{
  return ( &pNotificationMsgHead );
}

/*********************************************************************
 * @fn      gp_GetHeadCmdPayloadMsg
 *
 * @brief   Returns head pointer for  finding and binding respondent list
 *
 * @param   none
 *
 * @return  pointer to head
 */
gpCmdPayloadMsg_t* gp_GetHeadCmdPayloadMsg(void)
{
  return ( pCmdPayloadMsgHead );
}

/*********************************************************************
 * @fn      gp_GetHeadCmdPayloadMsg
 *
 * @brief   Returns head pointer for  finding and binding respondent list
 *
 * @param   none
 *
 * @return  pointer to head
 */
gpCmdPayloadMsg_t** gp_GetPHeadCmdPayload(void)
{
  return ( &pCmdPayloadMsgHead );
}

/*********************************************************************
 * @fn      gp_NotificationMsgClean
 *
 * @brief   This function free reserved memory for respondent list
 *
 * @param   pHead - begin of the Notification Msg list
 *
 * @return  status
 */
void gp_NotificationMsgClean( gpNotificationMsg_t **pHead )
{
  gpNotificationMsg_t **pCurr;
  gpNotificationMsg_t **pNext;
  
  if ( *pHead == NULL )
  {
    return;
  }
  
  pCurr = pHead;
  
  if( *pCurr != NULL )
  {
    pNext = &((*pCurr)->pNext);
    while( *pNext != NULL )
    {
      if ( ( ( gpNotificationMsg_t* ) *pCurr )->pMsg == ( ( gpNotificationMsg_t* ) *pNext )->pMsg )
      {
        break;
      }
      pNext = &((*pNext)->pNext);
    }

    if( *pNext == NULL )
    {
      gp_CmdPayloadMsgClean( gp_GetPHeadCmdPayload( ) );
    }

    zcl_mem_free( *pCurr );
    *pCurr = ( gpNotificationMsg_t* )NULL;
    pCurr = pNext;
  }
  *pHead = *pCurr;
}

/*********************************************************************
 * @fn      gp_NotificationMsgClean
 *
 * @brief   This function free reserved memory for respondent list
 *
 * @param   pHead - begin of the Notification Msg list
 *
 * @return
 */
void gp_CmdPayloadMsgClean( gpCmdPayloadMsg_t **pHead )
{
  gpCmdPayloadMsg_t **pCurr;
  gpCmdPayloadMsg_t **pNext;

  if ( *pHead == NULL )
  {
    return;
  }
  
  pCurr = pHead;
  
  if( *pCurr != NULL )
  {
    pNext = &((*pCurr)->pNext);
    zcl_mem_free ( ( ( gpCmdPayloadMsg_t* ) *pCurr )->pMsg );
    ( ( gpCmdPayloadMsg_t* ) *pCurr )->pMsg = NULL;
    zcl_mem_free( *pCurr );
    *pCurr = ( gpCmdPayloadMsg_t* )NULL;
    pCurr = pNext;
  }
  pHead = pCurr;
}

/*********************************************************************
 * @fn      gp_ResetProxyBasicTblEntry
 *
 * @brief   This function removes data of the given entry
 *
 * @param   entry - pointer to the proxy table entry array
 *
 * @return
 */
void gp_ResetProxyBasicTblEntry( uint8* entry )
{
  uint8 emptyEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  
  zcl_memcpy( entry, emptyEntry, PROXY_TBL_ENTRY_LEN );
}

/*********************************************************************
 * @fn      gp_ResetProxyBasicTblRAMEntry
 *
 * @brief   This function removes data of the given entry
 *
 * @param   entry - pointer to the proxy table entry array RAM struct
 *
 * @return
 */
void gp_ResetProxyBasicTblRAMEntry( ProxyTableEntryFormat_t* entry )
{
  uint8 freeIeee[Z_EXTADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8 freeSinkEntry[Z_EXTADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint8 freekey[SEC_KEY_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  entry->options = 0xFFFF;
  entry->GPDId.SrcID = 0xFFFFFFFF;
  zcl_memcpy(&entry->GPDId.GPDExtAddr, freeIeee, Z_EXTADDR_LEN);
  entry->endPoint = 0x00;
  entry->gpdAssignedAlias = 0xFFFF;
  entry->securityOptions = 0x00;
  entry->gpdSecurityFramecounter = 0xFFFFFFFF;
  zcl_memcpy(&entry->gpdKey, freekey, SEC_KEY_LEN);
  entry->FirstLightweightSinkAddr.sinkNwkAddr = 0x0000;
  zcl_memcpy(&entry->FirstLightweightSinkAddr.sinkIEEE, freeSinkEntry, Z_EXTADDR_LEN);
  entry->SecondLightweightSinkAddr.sinkNwkAddr = 0x0000;
  zcl_memcpy(&entry->SecondLightweightSinkAddr.sinkIEEE, freeSinkEntry, Z_EXTADDR_LEN);
  entry->SinkGroupEntriesBit = 0x00;
  entry->FirstSinkGroupAddr.alias = 0x0000;
  entry->FirstSinkGroupAddr.sinkGroup = 0x0000;
  entry->SecondSinkGroupAddr.alias = 0x0000;
  entry->SecondSinkGroupAddr.sinkGroup = 0x0000;
  entry->groupcastRadius = 0x00;
  entry->SearchCounter = 0x00;
}

void gp_zcl_send_data( void )
{
  gpNotificationMsg_t *gpNotification = NULL;
  gpCmdPayloadMsg_t *pCmdPayloadMsgCurr = NULL;
  uint8* pAddr;
  uint8 appId;
  uint8 nwkSeqNum;
  uint16 nwkAddr;
  nwkIB_t NIB = {0};

  gpNotification = gp_GetHeadNotificationMsg( );
  if ( gpNotification == NULL )
  {
    return;
  }

  pCmdPayloadMsgCurr = ( gpCmdPayloadMsg_t* ) gpNotification->pMsg ;
  appId = PROXY_TBL_GET_APPLICTION_ID((uint16)*pCmdPayloadMsgCurr->pMsg);

  // To save the NIB nwk sequense number and use the GP alias nwk sequence number
  // for the GP notification
  gwGetDevNwkInfo(&NIB);

  nwkSeqNum = NIB.SequenceNum;
  NIB.SequenceNum = pCmdPayloadMsgCurr->secNum;

  // To save the NIB nwk address and use the GP alias nwk address for the GP notification
  nwkAddr = NIB.nwkDevAddress;

  if( appId == GP_OPT_APP_ID_GPD )
  {
    pAddr = pCmdPayloadMsgCurr->pMsg + sizeof( uint16 );
    NIB.nwkDevAddress = BUILD_UINT16(pAddr[0], pAddr[1]);
  }
  else if( appId == GP_OPT_APP_ID_IEEE )
  {
    pAddr = pCmdPayloadMsgCurr->pMsg + Z_EXTADDR_LEN;
    NIB.nwkDevAddress = BUILD_UINT16(pAddr[1], pAddr[0]);
  }

  gwSetDevNwkInfo(&NIB);

  zcl_SendCommand( GREEN_POWER_INTERNAL_ENDPOINT, &gpNotification->addr, ZCL_CLUSTER_ID_GREEN_POWER,
                        COMMAND_GP_NOTIFICATION, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
                        TRUE, 0,  1/*bdb_getZCLFrameCounter()*/, pCmdPayloadMsgCurr->lenght, pCmdPayloadMsgCurr->pMsg );

  NIB.nwkDevAddress = nwkAddr;
  NIB.SequenceNum = nwkSeqNum;
  gwSetDevNwkInfo(&NIB);

  gp_NotificationMsgClean( gp_GetPHeadNotification ( ) );

  if ( gp_GetHeadNotificationMsg ( ) != NULL )
  {
    // TODO: CREATE TIMER HERE
    // gw_set_timer( &gp_zcl_data_send_timer, 50, false, (timer_handler_cb_t)gp_zcl_send_data, NULL );
  }
}

#endif
/***************************************************************************
****************************************************************************/
