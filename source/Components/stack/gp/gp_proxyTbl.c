/**************************************************************************************************
  Filename:       gp_proxyTbl.c
  Revised:        $Date: 2016-02-25 11:51:49 -0700 (Thu, 25 Feb 2016) $
  Revision:       $Revision: - $

  Description:    This file contains the implementation of the cGP stub.


  Copyright 2006-2015 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
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
#include "ZDObject.h"
#include "nwk_util.h"   
#include "ZComDef.h"
#include "cGP_stub.h"
#include "gp_common.h"
#include "gp_interface.h"
#include "zcl_green_power.h"
#include "OSAL.h"
#include "zcl.h"
#include "trace.h"

#if !defined (DISABLE_GREENPOWER_BASIC_PROXY)
   
 /*********************************************************************
 * MACROS
 */ 

 /*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */
   
 /*********************************************************************
 * GLOBAL VARIABLES
 */
ZDO_DeviceAnnce_t aliasConflictAnnce;

 /*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

extern void gwGpAddressConflict( uint16 addr );

/*********************************************************************
 * LOCAL VARIABLES
 */

 /*********************************************************************
 * LOCAL FUNCTIONS
 */
static void gp_PopulateProxyTblEntry( uint8* pData, uint8 size, uint8* pTblEntryVector, uint8* vectorLen );
static uint8 pt_getAlias( uint8* pNew );
static uint8 pt_getSecurity( uint8* pNew );
static uint8 pt_getSecFrameCounterCapabilities( uint8* pNew );
static uint8 pt_updateLightweightUnicastSink( uint8* pNew, uint8* pCurr, uint8 addSink );
static uint8 pt_addSinkGroup( uint8* pNew, uint8* pCurr );
static uint8 pt_lookForGpd( uint16 currProxyEntryId, uint8* pNew );
static uint16 gp_pairingSetProxyTblOptions ( uint32 pairingOpt );

/*********************************************************************
 * PUBLIC FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn          pt_ZclReadGetProxyEntry
 *
 * @brief       Populate the given item data
 *
 * @param       nvId - NV entry to read
 *              pData - Pointer to OTA message payload
 *              len - Lenght of the payload
 *
 * @return
 */
uint8 pt_ZclReadGetProxyEntry( uint16 nvId, uint8* pData, uint8* len )
{
  uint8  freeSinkEntry[LSINK_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8  currEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint8  gpdEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint8  gpdEntryLen = 0;
  uint8  status;
  uint16 options;

  status = gp_getProxyTableByIndex( nvId, currEntry );

  // if FAIL or Empty
  if ( ( status == NV_OPER_FAILED ) || ( status == NV_INVALID_DATA ) )
  {
    return status;
  }

  zcl_memcpy( &gpdEntry[gpdEntryLen], &currEntry[PROXY_TBL_ENTRY_OPT], sizeof ( uint16 ) );
  zcl_memcpy( &options, &currEntry[PROXY_TBL_ENTRY_OPT], sizeof ( uint16 ) );
  gpdEntryLen += sizeof ( uint16 );

  // Options bitfield
  // If Application Id bitfield is 0b000
  if( GP_IS_APPLICATION_ID_GPD( options ) )
  {        
    // Populate GPD ID
    gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_GPD_ID + 4], sizeof ( uint32 ), gpdEntry, &gpdEntryLen );
  }
  // If Application Id bitfield is 0b010

  else if( GP_IS_APPLICATION_ID_IEEE( options ) )
  {   
    gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_GPD_ID], Z_EXTADDR_LEN, gpdEntry, &gpdEntryLen );
    gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_GPD_EP], sizeof ( uint8 ), gpdEntry, &gpdEntryLen );
  }

  if ( PROXY_TBL_GET_ASSIGNED_ALIAS( currEntry[PROXY_TBL_ENTRY_OPT + 1] ) )
  {
    gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_ALIAS], sizeof ( uint16 ), gpdEntry, &gpdEntryLen );
  }
  
  if ( PROXY_TBL_GET_SEC_USE( currEntry[PROXY_TBL_ENTRY_OPT + 1] ) )
  {
    gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_SEC_OPT], sizeof ( uint8 ), gpdEntry, &gpdEntryLen );
  }
  
  if ( ( PROXY_TBL_GET_SEC_USE( currEntry[PROXY_TBL_ENTRY_OPT + 1] ) || ( PROXY_TBL_GET_SEC_CAP( currEntry[PROXY_TBL_ENTRY_OPT] ) ) ) )
  {
    gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_SEC_FRAME], sizeof ( uint32 ), gpdEntry, &gpdEntryLen );
  }
  
  if ( PROXY_TBL_GET_SEC_USE( currEntry[PROXY_TBL_ENTRY_OPT + 1] ) )
  {
    gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_GPD_KEY], SEC_KEY_LEN, gpdEntry, &gpdEntryLen );
  }

  if ( PROXY_TBL_GET_LIGHTWIGHT_UNICAST( currEntry[PROXY_TBL_ENTRY_OPT] ) )
  {
    uint8 sinkCount = 0;
    uint8 sinkCountByte = gpdEntryLen;
    
    gpdEntryLen += sizeof ( sinkCountByte );
    
    if ( !zcl_memcmp( freeSinkEntry, &currEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], LSINK_ADDR_LEN ) )
    {
      sinkCount += 1;
      zcl_memcpy( &gpdEntry[sinkCountByte], &sinkCount, sizeof ( uint8 ) );
      gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], LSINK_ADDR_LEN, gpdEntry, &gpdEntryLen );
    }
    if ( !zcl_memcmp( freeSinkEntry, &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], LSINK_ADDR_LEN ) )
    {
      sinkCount += 1;
      zcl_memcpy( &gpdEntry[sinkCountByte], &sinkCount, sizeof ( uint8 ) );
      gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], LSINK_ADDR_LEN, gpdEntry, &gpdEntryLen );
    }
  }

  if ( PROXY_TBL_GET_CGROUP( currEntry[PROXY_TBL_ENTRY_OPT + 1] ) )
  {
    uint8 groupCount = 0;
    uint8 groupCountByte = gpdEntryLen;
    
    gpdEntryLen++;
    
    if ( GET_BIT( &currEntry[PROXY_TBL_ENTRY_GRP_TBL_ENTRIES], PROXY_TBL_ENTRY_1ST_GRP_BIT ) )
    {
      groupCount += 1;
      zcl_memcpy( &gpdEntry[groupCountByte], &groupCount, sizeof ( uint8 ) );
      gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_1ST_GRP_ADDR], sizeof ( uint32 ), gpdEntry, &gpdEntryLen );
    }
    if ( GET_BIT( &currEntry[PROXY_TBL_ENTRY_GRP_TBL_ENTRIES], PROXY_TBL_ENTRY_2ND_GRP_BIT ) )
    {
      groupCount += 1;
      zcl_memcpy( &gpdEntry[groupCountByte], &groupCount, sizeof( uint8 ) );
      gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_2ND_GRP_ADDR], sizeof ( uint32 ), gpdEntry, &gpdEntryLen );
    }
  }

  gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_GRP_RAD], sizeof ( uint8 ), gpdEntry, &gpdEntryLen );
  
  if ( ( PROXY_TBL_GET_ENTRY_ACTIVE( currEntry[PROXY_TBL_ENTRY_OPT] ) == 0 ) || ( PROXY_TBL_GET_ENTRY_VALID( currEntry[PROXY_TBL_ENTRY_OPT] ) == 0 ) )
  {
    gp_PopulateProxyTblEntry( ( uint8* )&currEntry[PROXY_TBL_ENTRY_SEARCH_COUNTER], sizeof ( uint8 ), gpdEntry, &gpdEntryLen );
  }

  if ( pData != NULL )
  {
    uint8 sinkEntryLen;
    uint16 totalLen;
    sinkEntryLen = gpdEntryLen;
    if ( *len != 0 )
    {
    gpdEntryLen -= sizeof ( uint16 );
    }
    totalLen = gpdEntryLen;
    totalLen += *len;
    zcl_memcpy( pData, &totalLen, sizeof ( uint16 ) );
    if ( *len == 0 )
    {
      pData += sizeof ( uint16 );
    }
    gpdEntryLen += sizeof ( uint16 );
    pData += *len;
    zcl_memcpy( pData, gpdEntry, sinkEntryLen );
    if ( *len != 0 )
    {
      *len = totalLen;
      return status;
    }
    *len += gpdEntryLen;
    return status;
  }
  else
  {
    if ( *len == 0 )
    {
      gpdEntryLen += sizeof ( uint16 );
    }
    *len += gpdEntryLen;
    return status;
  }
}

/*********************************************************************
 * @fn      zclGpp_ReadWriteAttrCB
 *
 * @brief   Handle Diagnostics attributes.
 *
 * @param   clusterId - cluster that attribute belongs to
 * @param   attrId - attribute to be read or written
 * @param   oper - ZCL_OPER_LEN, ZCL_OPER_READ, or ZCL_OPER_WRITE
 * @param   pValue - pointer to attribute value, OTA endian
 * @param   pLen - length of attribute value read, native endian
 *
 * @return  status
 */
ZStatus_t zclGpp_ReadWriteAttrCB( uint16 clusterId, uint16 attrId, uint8 oper,
                                         uint8 *pValue, uint16 *pLen )
{
  ZStatus_t status = ZSuccess;
  uint16 proxyTableIndex;
  uint8 i;
  
  switch ( oper )
  {
    case ZCL_OPER_LEN:
    case ZCL_OPER_READ:
      if ( attrId == ATTRID_GP_PROXY_TABLE )
      { 
        for ( i = 0; i < GPP_MAX_PROXY_TABLE_ENTRIES ; i++ )
        {
          proxyTableIndex = ZCD_NV_PROXY_TABLE_START + i;
          status = pt_ZclReadGetProxyEntry ( proxyTableIndex, pValue, (uint8*)pLen );
          if ( status != ZSuccess )
          {
            status = ZFailure;
            break;
          }
        }
      }
      break;

    case ZCL_OPER_WRITE:
          status = ZFailure;
          break;
  }

  return ( status );
}

/*********************************************************************
 * @fn          gp_pairingSetProxyTblOptions
 *
 * @brief       Function to fill the options pramenter in a Proxy Table entry
 *              from a GP Pairing Command
 *
 * @param       pBitField - pointer to the bit field.
 *              bit       - position of the bit to set in the given bitfield.
 *
 * @return      
 */
static uint16 gp_pairingSetProxyTblOptions ( uint32 pairingOpt )
{
  uint8  proxyOpt[2] = {0x00, 0x00};
  uint16 opt = 0; 
  
  PAIRING_PROXY_TBL_SET_APPLICATION_ID(( uint8* )&proxyOpt[0], pairingOpt);
  PROXY_TBL_SET_ENTRY_ACTIVE( &proxyOpt[0] );
  PROXY_TBL_SET_ENTRY_VALID( &proxyOpt[0] );
  PAIRING_PROXY_TBL_SET_MAC_SEQ_CAP(( uint16* )&proxyOpt, pairingOpt);
  
  if ( GP_IS_COMMUNICATION_MODE_LIGHT_UNICAST( pairingOpt ) )
  {
    PROXY_TBL_SET_LIGHTWIGHT_UNICAST( &proxyOpt[0] );
  }
  else
  {
    PROXY_TBL_CLR_LIGHTWIGHT_UNICAST( &proxyOpt[0] );
  }
  if ( GP_IS_COMMUNICATION_MODE_GRPCAST_DGROUP_ID( pairingOpt ) )
  {
    PROXY_TBL_SET_DGROUP( &proxyOpt[0] );
  }
  else
  {
    PROXY_TBL_CLR_DGROUP( &proxyOpt[0] );
  }
  if ( GP_IS_COMMUNICATION_MODE_GRPCAST_GROUP_ID( pairingOpt ) )
  {
    PROXY_TBL_SET_CGROUP( &proxyOpt[1] );
  }
  else
  {
    PROXY_TBL_CLR_CGROUP( &proxyOpt[1] );
  }
  
  // FALSE by default
  PROXY_TBL_CLR_FIRST_TO_FORWARD( &proxyOpt[1] );
  
  // The InRange sub-field, if set to 0b1, indicates that this GPD is in range 
  // of this proxy. The default value is FALSE
  PROXY_TBL_CLR_IN_RANGE( &proxyOpt[1] );
  // The GPDfixed sub-field, if set to 0b1, indicates portability capabilities 
  if ( GP_GET_GPD_FIXED_BIT( pairingOpt ) )
  {
    PROXY_TBL_SET_GPD_FIXED( &proxyOpt[1] );
  }
  else
  {
    PROXY_TBL_CLR_GPD_FIXED( &proxyOpt[1] );
  }
  
  if ( GP_IS_COMMUNICATION_MODE_FULL_UNICAST( pairingOpt ) )
  {
    PROXY_TBL_SET_HAS_ALL_ROUTES( &proxyOpt[1] );
  }
  else
  {
    PROXY_TBL_CLR_HAS_ALL_ROUTES( &proxyOpt[1] );
  }
  if ( (GP_ALIAS ( pairingOpt )) && (!GP_IS_COMMUNICATION_MODE_GRPCAST_GROUP_ID( pairingOpt )) )
  {
    PROXY_TBL_SET_ASSIGNED_ALIAS( &proxyOpt[1] );
  }
  else
  {
    PROXY_TBL_CLR_ASSIGNED_ALIAS( &proxyOpt[1]);
  }
  if ( GP_GET_SEC_LEVEL( pairingOpt ) > 1 )
  {
    PROXY_TBL_SET_SEC_USE( &proxyOpt[1] );
  }
  else
  {
    PROXY_TBL_CLR_SEC_USE( &proxyOpt[1] );
  }
  PROXY_TBL_SET_OPT_EXT( &proxyOpt[1] );
  PROXY_TBL_CLR_OPT_EXT( &proxyOpt[1] );
  
  opt |= ( ( proxyOpt[0] ) & 0x00FF );
  opt |= ( ( ( proxyOpt[1] ) <<  8 ) & 0xFF00 );
    
  return opt;
}

/*********************************************************************
 * @fn          gp_PairingUpdateProxyTlb
 *
 * @brief       General function fill the proxy table vector
 *
 * @param       
 *
 * @return      
 */
void gp_PairingUpdateProxyTlb( gpPairingCmd_t* payload )
{
  uint8  entryVector[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint8  entryLen = 0;
  uint8  addSink;
  uint8  newSinkGroup = 0;
  uint16 entryOptions = 0;
  uint8  securityOpt = 0;
  uint8  searchCounter = 0x0A;
  uint16 invalidAlias = 0xFFFF;
  gp_ProxyTblUpdateStatus updateStatus;

  addSink = GP_ADD_SINK( payload->options );

  
  entryOptions = gp_pairingSetProxyTblOptions( payload->options );
  gp_PopulateProxyTblEntry( ( uint8* )&entryOptions, sizeof( entryOptions ), entryVector, &entryLen );

  // Options bitfield
  // If Application Id bitfield is 0b000
  if( GP_IS_APPLICATION_ID_GPD( payload->options ) )
  {        
    // Populate GPD ID
    entryLen += sizeof( uint32 );
    gp_PopulateProxyTblEntry( ( uint8* )&payload->gpdId, sizeof( payload->gpdId ), entryVector, &entryLen );

  }
  // If Application Id bitfield is 0b010
  else if( GP_IS_APPLICATION_ID_IEEE( payload->options ) )
  {   
    gp_PopulateProxyTblEntry( payload->gpdIEEE, Z_EXTADDR_LEN, entryVector, &entryLen );
  }
  
   gp_PopulateProxyTblEntry( ( uint8* )&payload->ep, sizeof( payload->ep ), entryVector, &entryLen );

  if( !GP_IS_COMMUNICATION_MODE_GRPCAST_GROUP_ID( payload->options ) )
  {
    gp_PopulateProxyTblEntry( ( uint8* )&payload->assignedAlias, sizeof( payload->assignedAlias ), entryVector, &entryLen );
  }
  else
  {
    gp_PopulateProxyTblEntry( ( uint8* )&invalidAlias, sizeof( uint16 ), entryVector, &entryLen );
  }


  //Security options
  securityOpt |= GP_GET_SEC_LEVEL( payload->options );
  securityOpt |= GP_GET_SEC_KEY_TYPE (payload->options ) << GP_OPT_SEC_LEVEL_LEN;
  zcl_memcpy( &entryVector[entryLen], &securityOpt, sizeof ( uint8 ) );
  entryLen += sizeof ( uint8 );

  gp_PopulateProxyTblEntry( ( uint8* )&payload->gpdSecCounter, sizeof( payload->gpdSecCounter ), entryVector, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&payload->gpdKey, SEC_KEY_LEN, entryVector, &entryLen );
  
  gp_PopulateProxyTblEntry( payload->sinkIEEE, Z_EXTADDR_LEN, entryVector, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&payload->sinkNwkAddr, sizeof( payload->sinkNwkAddr ), entryVector, &entryLen );


  // to add the invalid entries
  zcl_memset ( &payload->sinkIEEE, 0xFF, Z_EXTADDR_LEN );
  payload->sinkNwkAddr = 0xFFFF;
  gp_PopulateProxyTblEntry( payload->sinkIEEE, Z_EXTADDR_LEN, entryVector, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&payload->sinkNwkAddr, sizeof( payload->sinkNwkAddr ), entryVector, &entryLen );

  newSinkGroup = PROXY_TBL_GET_DGROUP( entryOptions & 0x00FF );
  newSinkGroup |= PROXY_TBL_GET_CGROUP( ( entryOptions & 0xFF00 ) >> 8 );
  zcl_memset( &entryVector[entryLen], newSinkGroup, sizeof ( uint8 ) );
  entryLen++;
  
  gp_PopulateProxyTblEntry( ( uint8* )&payload->sinkGroupID, sizeof( payload->sinkGroupID ), entryVector, &entryLen );

  if( GP_IS_COMMUNICATION_MODE_GRPCAST_GROUP_ID( payload->options ) || GP_IS_COMMUNICATION_MODE_GRPCAST_DGROUP_ID( payload->options ) )
  {
    gp_PopulateProxyTblEntry( ( uint8* )&payload->assignedAlias, sizeof( payload->assignedAlias ), entryVector, &entryLen );
  }
  else
  {
    gp_PopulateProxyTblEntry( ( uint8* )&invalidAlias, sizeof ( uint16 ), entryVector, &entryLen );
  }
  // to add the invalid entries
  payload->sinkGroupID = 0xFFFF;
  gp_PopulateProxyTblEntry( ( uint8* )&payload->sinkGroupID, sizeof( payload->sinkGroupID ), entryVector, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&invalidAlias, sizeof ( uint16 ), entryVector, &entryLen );

  gp_PopulateProxyTblEntry( &payload->forwardingRadius, sizeof( payload->forwardingRadius ), entryVector, &entryLen );
 
  if ( ( PROXY_TBL_GET_ENTRY_ACTIVE( entryVector[0] ) == 0 ) && ( PROXY_TBL_GET_ENTRY_VALID( entryVector[0] ) == 0) )
  {
    zcl_memcpy( &entryVector[entryLen], &searchCounter, sizeof ( uint8 ) );
  }
  else
  {
    searchCounter = 0xFF;
    zcl_memcpy( &entryVector[entryLen], &searchCounter, sizeof ( uint8 ) );
  }
  // Update the proxy table
   updateStatus = gp_UpdateProxyTlb( entryVector, addSink );

   if( updateStatus.status == SUCCESS )
   {
     zcl_memcpy( &proxyTableEntry[updateStatus.idx], &updateStatus.entryStruct, sizeof( ProxyTableEntryFormat_t) );
   }
}

/*********************************************************************
 * @fn          gp_UpdateProxyTlb
 *
 * @brief       To update the proxy table NV vectors
 *
 * @param       pNpEntryew - New entry array to be added
 *
 * @return
 */
gp_ProxyTblUpdateStatus gp_UpdateProxyTlb( uint8* pEntry, uint8 addSink )
{
  uint8 i;
  uint8 newEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint8 currEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint16 proxyTableIndex;
  gp_ProxyTblUpdateStatus updateStatus = { 0, 0 };
  
  // Copy the new entry pointer to array
  zcl_memcpy( &newEntry, pEntry, PROXY_TBL_ENTRY_LEN );

  for ( i = 0; i < GPP_MAX_PROXY_TABLE_ENTRIES ; i++ )
  {
    proxyTableIndex = ZCD_NV_PROXY_TABLE_START + i;

    updateStatus.status = gp_getProxyTableByIndex( proxyTableIndex, currEntry );
    
    if ( updateStatus.status == NV_OPER_FAILED )
    {
      // FAIL
      return updateStatus;
    }
    // if the entry is empty
    if ( ( updateStatus.status == NV_INVALID_DATA ) && ( addSink == TRUE ) )
    {
      // Save new entry to NVM
	  updateStatus.status = zcl_nv_actions(2, proxyTableIndex, 0, PROXY_TBL_ENTRY_LEN, newEntry);
	  gp_copyProxyTblArray( &(updateStatus.entryStruct), newEntry );
      updateStatus.idx = i;
      /*****************************************************************************************
       * Since linux Gateway is always a coordinator this condition doesn't apply
       *****************************************************************************************
      if ( zcl_memcmp( &selfAddr, &newEntry[PROXY_TBL_ENTRY_ALIAS], sizeof( uint16 ) )        ||
           zcl_memcmp( &selfAddr, &newEntry[PROXY_TBL_ENTRY_1ST_GRP_ADDR], sizeof( uint16 ) ) ||
           zcl_memcmp( &selfAddr, &newEntry[PROXY_TBL_ENTRY_2ND_GRP_ADDR], sizeof( uint16 ) )   )
      {
    	// Send address conflict
      }
      */
      return updateStatus;
    }

    if ( pt_lookForGpd( proxyTableIndex, newEntry ) )
    {
      // Entry found
      break;
    }

    if ( i >= GPP_MAX_PROXY_TABLE_ENTRIES )
    {
      // No space for new entries
      updateStatus.status = FAILURE;
      return updateStatus;
    }
  }

  // Remove the entry
  if ( addSink == FALSE )
  {
    gp_ResetProxyBasicTblEntry( currEntry );

    // Save new entry to NVM
    updateStatus.status = zcl_nv_actions(2, proxyTableIndex, 0, PROXY_TBL_ENTRY_LEN, currEntry);
    gp_copyProxyTblArray( &(updateStatus.entryStruct), currEntry );

    updateStatus.idx = i;
    return updateStatus;
  }
  
  if ( PROXY_TBL_GET_LIGHTWIGHT_UNICAST( newEntry[PROXY_TBL_ENTRY_OPT] ) )
  {
    if ( ( !pt_updateLightweightUnicastSink( newEntry, currEntry, addSink ) ) )
    {
      // The entry is full
      updateStatus.status = NV_BAD_ITEM_LEN;
      return updateStatus;
    }
  }
  else if ( PROXY_TBL_GET_CGROUP( newEntry[PROXY_TBL_ENTRY_OPT + 1] ) )
  {
    if ( ( !pt_addSinkGroup( newEntry, currEntry ) ) )
    {
      // The entry is full
      updateStatus.status = NV_BAD_ITEM_LEN;
      return updateStatus;
    }
  }

  if ( pt_getAlias( newEntry ) )
  {
    zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_ALIAS], &newEntry[PROXY_TBL_ENTRY_ALIAS], sizeof ( uint16 ) );
  }

  if ( pt_getSecurity( newEntry ) )
  {
    PROXY_TBL_SET_SEC_USE( &currEntry[PROXY_TBL_ENTRY_OPT + 1] );
    zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_SEC_OPT], &newEntry[PROXY_TBL_ENTRY_SEC_OPT], sizeof ( uint8 ) );
    zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_GPD_KEY], &newEntry[PROXY_TBL_ENTRY_GPD_KEY], SEC_KEY_LEN );
  }

  if ( pt_getSecFrameCounterCapabilities( newEntry ) )
  {
    PROXY_TBL_SET_SEC_CAP( &currEntry[PROXY_TBL_ENTRY_OPT] );
  }

  zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_SEC_FRAME], &newEntry[PROXY_TBL_ENTRY_SEC_FRAME], sizeof ( uint32 ) );

  currEntry[PROXY_TBL_ENTRY_GRP_RAD] = newEntry[PROXY_TBL_ENTRY_GRP_RAD];

  currEntry[PROXY_TBL_ENTRY_SEARCH_COUNTER] = newEntry[PROXY_TBL_ENTRY_SEARCH_COUNTER];

  // Save new entry to NVM
  updateStatus.status = zcl_nv_actions(2, proxyTableIndex, 0, PROXY_TBL_ENTRY_LEN, currEntry);
  gp_copyProxyTblArray( &(updateStatus.entryStruct), currEntry );

  /*****************************************************************************************
   * Since linux Gateway is always a coordinator this condition doesn't apply
   *****************************************************************************************
  if ( zcl_memcmp( &selfAddr, &newEntry[PROXY_TBL_ENTRY_ALIAS], sizeof( uint16 ) )        ||
       zcl_memcmp( &selfAddr, &newEntry[PROXY_TBL_ENTRY_1ST_GRP_ADDR], sizeof( uint16 ) ) ||
       zcl_memcmp( &selfAddr, &newEntry[PROXY_TBL_ENTRY_2ND_GRP_ADDR], sizeof( uint16 ) )   )
  {
    // Send address conflict
  }
  */

  updateStatus.idx = i;
  return updateStatus;
}

/*********************************************************************
 * @fn          gp_CheckAnnouncedDevice
 *
 * @brief       General function to check if it has the announced device 
 *              listed in the SinkAddressList and look for address conflict
 *              resolution.
 *
 * @param       
 *
 * @return      
 */
uint8 gp_CheckAnnouncedDevice ( uint8 *sinkIEEE, uint16 sinkNwkAddr )
{
  uint8 i;
  uint8 status;
  //uint8 annceDelay;
  uint8 ProxyTableEntry[PROXY_TBL_ENTRY_LEN];

  for ( i = 0; i < GPP_MAX_PROXY_TABLE_ENTRIES ; i++ )
  {
    status = gp_getProxyTableByIndex( ( ZCD_NV_PROXY_TABLE_START + i ), ProxyTableEntry );

    if ( status == NV_OPER_FAILED )
    {
      // FAIL

      return ZFailure;
    }
    
    // if the entry is empty
    if ( status == NV_INVALID_DATA )
    {
      continue;
    }
    
    // Compare for nwk alias address conflict
    if ( zcl_memcmp( &sinkNwkAddr, &ProxyTableEntry[PROXY_TBL_ENTRY_ALIAS], sizeof( uint16 ) )        ||
         zcl_memcmp( &sinkNwkAddr, &ProxyTableEntry[PROXY_TBL_ENTRY_1ST_GRP_ADDR], sizeof( uint16 ) ) ||
         zcl_memcmp( &sinkNwkAddr, &ProxyTableEntry[PROXY_TBL_ENTRY_2ND_GRP_ADDR], sizeof( uint16 ) )   )
    {
      // Do address conflict resolution
      gwGpAddressConflict(sinkNwkAddr);

      aliasConflictAnnce.nwkAddr = sinkNwkAddr;
      zcl_memset( aliasConflictAnnce.extAddr, 0xFF, Z_EXTADDR_LEN );
      aliasConflictAnnce.capabilities = 0;
      //TODO:
      //annceDelay = ( osal_rand()/650 ) + 5; // ( 65535/650 = 100 ) + 5
      // where:
      // 100 is Dmax
      // 5   is Dmin
      gpProxyAliasConflictTimeout  = true;
    }

    if( sinkIEEE )
    {
      //Check if one of the Sink IEEE is the same
      if ( zcl_memcmp( sinkIEEE, &ProxyTableEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], Z_EXTADDR_LEN ) )
      {
        // If Nwk address is different, then update the new address
        if ( !zcl_memcmp( &sinkNwkAddr, &ProxyTableEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR + Z_EXTADDR_LEN], sizeof( uint16 ) ) )
        {
          zcl_memcpy( &ProxyTableEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR + Z_EXTADDR_LEN], &sinkNwkAddr, sizeof( uint16 ) );
        }
      }
      else if ( zcl_memcmp( sinkIEEE, &ProxyTableEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], Z_EXTADDR_LEN ) )
      {
        // If Nwk address is different, then update the new address
        if ( !zcl_memcmp( &sinkNwkAddr, &ProxyTableEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR + Z_EXTADDR_LEN], sizeof( uint16 ) ) )
        {
          zcl_memcpy( &ProxyTableEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR + Z_EXTADDR_LEN], &sinkNwkAddr, sizeof( uint16 ) );
        }
      }
    }
  }
    
  return ZSuccess;
}

 /*********************************************************************
 * PRIVATE FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn          gp_PopulateProxyTblEntry
 *
 * @brief       Populate the given item data
 *
 * @param       pData - Pointer to the new data
 *              size - size of the new data
 *              pTblEntryVector - Pointer to the new vector
 *              vectorLen - Position in the vector to write the data
 *
 * @return
 */
static void gp_PopulateProxyTblEntry( uint8* pData, uint8 size, uint8* pTblEntryVector, uint8* vectorLen )
{
  pTblEntryVector += *(uint8*)vectorLen;
  
  zcl_memcpy( pTblEntryVector, pData, size );
  *(uint8*)vectorLen += size;
}

/*********************************************************************
 * @fn          pt_getAlias
 *
 * @brief       Get Bit to update assigned alias field or not
 *
 * @param       pNew - New entry array to be added
 *
 * @return      newAlias - TRUE if assigned alias bit is set
 */
static uint8 pt_getAlias( uint8* pNew )
{
  bool newAlias;
  
  newAlias = PROXY_TBL_GET_ASSIGNED_ALIAS( pNew[PROXY_TBL_ENTRY_OPT + 1] );
  
  return newAlias;
}

/*********************************************************************
 * @fn          pt_getSecurity
 *
 * @brief       Get Bit to update security options field or not
 *
 * @param       pNew - New entry array to be added
 *
 * @return      secUse - TRUE if security use bit is set
 */
static uint8 pt_getSecurity( uint8* pNew )
{
  bool secUse;
  
  secUse = PROXY_TBL_GET_SEC_USE( pNew[PROXY_TBL_ENTRY_OPT + 1] );

  return secUse;
}

/*********************************************************************
 * @fn          pt_getSecFrameCounterCapabilities
 *
 * @brief       Get Bit to update security frame counter capabilities
 *
 * @param       pNew - New entry array to be added
 *
 * @return      secCap - TRUE if security frame counter capabilities bit is set
 */
static uint8 pt_getSecFrameCounterCapabilities( uint8* pNew )
{
  bool secCap;
  
  secCap = PROXY_TBL_GET_SEC_CAP( pNew[PROXY_TBL_ENTRY_OPT] );
  
  return secCap;
}

/*********************************************************************
 * @fn          pt_updateLightweightUnicastSink
 *
 * @brief       To add new sink entry
 *
 * @param       pNew - New entry array to be added
 *              pCurr - The current entry array in the proxy table
 *
 * @return      TRUE new entry added, FALSE if not
 */
static uint8 pt_updateLightweightUnicastSink( uint8* pNew, uint8* pCurr, uint8 addSink )
{
  uint8 newEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint8 currEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint8 freeSinkEntry[LSINK_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  
  zcl_memcpy( &newEntry, pNew, PROXY_TBL_ENTRY_LEN );
  zcl_memcpy( &currEntry, pCurr, PROXY_TBL_ENTRY_LEN );
  
  
  // if addSink is TRUE then add the sink to current entry
  if ( addSink ) 
  {
    // if the incomming Sink entry is different to the current entry
    if ( !zcl_memcmp( &currEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], &newEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], LSINK_ADDR_LEN ) )
    {
      // add the sink entry in the next slot if empty
      if ( zcl_memcmp( freeSinkEntry, &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], LSINK_ADDR_LEN ) )
      {
        zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], &newEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], LSINK_ADDR_LEN );
        zcl_memcpy( pCurr, &currEntry, PROXY_TBL_ENTRY_LEN );
        return TRUE;
      }
    }
    else 
    {
      zcl_memcpy( pCurr, &currEntry, PROXY_TBL_ENTRY_LEN );
     return TRUE;
    }
    // if the incomming Sink entry is different to the current entry
    if ( !zcl_memcmp( &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], &newEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], LSINK_ADDR_LEN ) )
    {
      return FALSE;
    }
  }
  else
  {
    // if the incomming Sink entry is the same to the current entry
    if ( zcl_memcmp( &currEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], &newEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], LSINK_ADDR_LEN ) )
    {
      // Move the next sink entry to current
      zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], LSINK_ADDR_LEN );
      // Invalidate the sink entry
      zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], freeSinkEntry, LSINK_ADDR_LEN );
      
      // if the sink entry is empty then remove the proxy table entry
      if ( zcl_memcmp( freeSinkEntry, &currEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], LSINK_ADDR_LEN ) )
      {
        gp_ResetProxyBasicTblEntry( currEntry );
      }
      zcl_memcpy( pCurr, &currEntry, PROXY_TBL_ENTRY_LEN );
      return TRUE;
    }
    // if the incomming Sink entry is the same to the current entry
    else if ( zcl_memcmp( &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], &newEntry[PROXY_TBL_ENTRY_1ST_LSINK_ADDR], LSINK_ADDR_LEN ) )
    {
      // Invalidate the sink entry
      zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_2ND_LSINK_ADDR], freeSinkEntry, LSINK_ADDR_LEN );
      zcl_memcpy( pCurr, &currEntry, PROXY_TBL_ENTRY_LEN );
      return TRUE;
    }
  }
  return FALSE;
}

/*********************************************************************
 * @fn          pt_addSinkGroup
 *
 * @brief       To add new sink group entry
 *
 * @param       pNew - New entry array to be added
 *              pCurr - The current entry array in the proxy table
 *
 * @return      TRUE new entry added, FALSE if not
 */
static uint8 pt_addSinkGroup( uint8* pNew, uint8* pCurr )
{
  bool newSinkGroup;
  uint8 newEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint8 currEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  
  zcl_memcpy( &newEntry, pNew, PROXY_TBL_ENTRY_LEN );
  zcl_memcpy( &currEntry, pCurr, PROXY_TBL_ENTRY_LEN );
  
  newSinkGroup = PROXY_TBL_GET_DGROUP( newEntry[PROXY_TBL_ENTRY_OPT] );
  newSinkGroup = PROXY_TBL_GET_CGROUP( newEntry[PROXY_TBL_ENTRY_OPT + 1] );
   
  if ( newSinkGroup )
  {
    if ( !GET_BIT( &currEntry[PROXY_TBL_ENTRY_GRP_TBL_ENTRIES], PROXY_TBL_ENTRY_2ND_GRP_BIT ) )
    {
      if ( !GET_BIT( &currEntry[PROXY_TBL_ENTRY_GRP_TBL_ENTRIES], PROXY_TBL_ENTRY_1ST_GRP_BIT ) )
      {
        zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_1ST_GRP_ADDR], &newEntry[PROXY_TBL_ENTRY_1ST_GRP_ADDR], sizeof ( uint32 ) );
        SET_BIT( &currEntry[PROXY_TBL_ENTRY_GRP_TBL_ENTRIES], PROXY_TBL_ENTRY_1ST_GRP_BIT );
        zcl_memcpy( pCurr, &currEntry, PROXY_TBL_ENTRY_LEN );
        return TRUE;
      }
      else
      {
        zcl_memcpy( &currEntry[PROXY_TBL_ENTRY_2ND_GRP_ADDR], &newEntry[PROXY_TBL_ENTRY_1ST_GRP_ADDR], sizeof ( uint32 ) );
        SET_BIT( &currEntry[PROXY_TBL_ENTRY_GRP_TBL_ENTRIES], PROXY_TBL_ENTRY_2ND_GRP_BIT );
        zcl_memcpy( pCurr, &currEntry, PROXY_TBL_ENTRY_LEN );
        return TRUE;
      }
    }
  }
  return FALSE;
}

/*********************************************************************
 * @fn          pt_lookForGpd
 *
 * @brief       To see if the GPD id is in this NV entry
 *
 * @param       currProxyEntryId - NV ID of the proxy table
 *              pNew - New tabble entry array to be added
 *
 * @return      TRUE if the GPD has an entry in current NV vector
 */
static uint8 pt_lookForGpd( uint16 currProxyEntryId, uint8* pNew )
{
  uint8 rtrn;
  uint8 newEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint8 currEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint16 idx = currProxyEntryId - ZCD_NV_PROXY_TABLE_START;

  zcl_memcpy( &newEntry, pNew, PROXY_TBL_ENTRY_LEN );
  
  rtrn = gp_copyProxyTblStruct( &proxyTableEntry[idx], currEntry);

  
  if ( rtrn != SUCCESS )
  {
    return FALSE;
  }

  if ( PROXY_TBL_COMP_APPLICTION_ID( newEntry[PROXY_TBL_ENTRY_OPT], currEntry[PROXY_TBL_ENTRY_OPT] ) )
  {
    if ( zcl_memcmp( &newEntry[PROXY_TBL_ENTRY_GPD_ID], &currEntry[PROXY_TBL_ENTRY_GPD_ID], Z_EXTADDR_LEN ) )
    {
    return TRUE;
    }
  }
  return FALSE;
}

/*********************************************************************
 * @fn          gp_copyProxyTblStruct
 *
 * @brief       Copy the RAM structure to proxy table entry
 *
 * @param       pEntryStruct - Pointer to struct
 *              pEntry - Pointer to the entry
 *
 * @return
 */
uint8 gp_copyProxyTblStruct( ProxyTableEntryFormat_t *pEntryStruct, uint8* pEntry )
{
  uint8 currEntry[PROXY_TBL_ENTRY_LEN] = PROXY_BASIC_TBL_EMPTY_ENTRY;
  uint8  entryLen = 0;
  
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->options, sizeof( pEntryStruct->options ), currEntry, &entryLen );

  // If Application Id bitfield is 0b000
  if( GP_IS_APPLICATION_ID_GPD( pEntryStruct->options ) )
  {
    // Populate GPD ID
    entryLen += sizeof( uint32 );
    gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->GPDId.SrcID, sizeof( pEntryStruct->GPDId.SrcID ), currEntry, &entryLen );
  }

  // If Application Id bitfield is 0b010
  else if( GP_IS_APPLICATION_ID_IEEE( pEntryStruct->options ) )
  {
    gp_PopulateProxyTblEntry( pEntryStruct->GPDId.GPDExtAddr, Z_EXTADDR_LEN, currEntry, &entryLen );
  }

  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->endPoint, sizeof( pEntryStruct->endPoint ), currEntry, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->gpdAssignedAlias, sizeof( pEntryStruct->gpdAssignedAlias ), currEntry, &entryLen );
  zcl_memcpy( &currEntry[entryLen], &pEntryStruct->securityOptions, sizeof( pEntryStruct->securityOptions ) );
  entryLen += sizeof ( pEntryStruct->securityOptions );

  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->gpdSecurityFramecounter, sizeof( pEntryStruct->gpdSecurityFramecounter ), currEntry, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->gpdKey, SEC_KEY_LEN, currEntry, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->FirstLightweightSinkAddr.sinkIEEE, Z_EXTADDR_LEN, currEntry, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->FirstLightweightSinkAddr.sinkNwkAddr, sizeof( pEntryStruct->FirstLightweightSinkAddr.sinkNwkAddr ), currEntry, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->SecondLightweightSinkAddr.sinkIEEE, Z_EXTADDR_LEN, currEntry, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->SecondLightweightSinkAddr.sinkNwkAddr, sizeof( pEntryStruct->SecondLightweightSinkAddr.sinkNwkAddr ), currEntry, &entryLen );

  zcl_memset( &currEntry[entryLen], pEntryStruct->SinkGroupEntriesBit, sizeof ( pEntryStruct->SinkGroupEntriesBit ) );
  entryLen++;
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->FirstSinkGroupAddr.sinkGroup, sizeof( pEntryStruct->FirstSinkGroupAddr.sinkGroup ), currEntry, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->FirstSinkGroupAddr.alias, sizeof( pEntryStruct->FirstSinkGroupAddr.alias ), currEntry, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->SecondSinkGroupAddr.sinkGroup, sizeof( pEntryStruct->SecondSinkGroupAddr.sinkGroup ), currEntry, &entryLen );
  gp_PopulateProxyTblEntry( ( uint8* )&pEntryStruct->SecondSinkGroupAddr.alias, sizeof( pEntryStruct->SecondSinkGroupAddr.alias ), currEntry, &entryLen );

  gp_PopulateProxyTblEntry( &pEntryStruct->groupcastRadius, sizeof( pEntryStruct->groupcastRadius ), currEntry, &entryLen );
  zcl_memcpy( &currEntry[entryLen], &pEntryStruct->SearchCounter, sizeof ( uint8 ) );

  zcl_memcpy( pEntry, currEntry, PROXY_TBL_ENTRY_LEN);

  return SUCCESS;
}

/*********************************************************************
 * @fn          gp_copyProxyTblArray
 *
 * @brief       Copy the proxy table entry to ram
 *
 * @param       pEntryStruct - Pointer to struct
 *              pEntry - Pointer to the entry
 *
 * @return
 */
uint8 gp_copyProxyTblArray( ProxyTableEntryFormat_t *pEntryStruct, uint8* pEntry )
{
  gp_PopulateField( ( uint8* )&pEntryStruct->options, &pEntry, sizeof( uint16 ) );


  // If Application Id bitfield is 0b000
  if( GP_IS_APPLICATION_ID_GPD( pEntryStruct->options ) )
  {
    // Populate GPD ID
    pEntry += 4;
    gp_PopulateField( ( uint8* )&pEntryStruct->GPDId.SrcID, &pEntry, sizeof( uint32 ) );
  }

  // If Application Id bitfield is 0b010
  else if( GP_IS_APPLICATION_ID_IEEE( pEntryStruct->options ) )
  {
    // Populate IEEE
    gp_PopulateField( ( uint8* )&pEntryStruct->GPDId.GPDExtAddr, &pEntry, Z_EXTADDR_LEN );
  }

  gp_PopulateField( ( uint8* )&pEntryStruct->endPoint, &pEntry, sizeof( uint8 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->gpdAssignedAlias, &pEntry, sizeof( uint16 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->securityOptions, &pEntry, sizeof( uint8 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->gpdSecurityFramecounter, &pEntry, sizeof( uint32 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->gpdKey, &pEntry, SEC_KEY_LEN);
  gp_PopulateField( ( uint8* )&pEntryStruct->FirstLightweightSinkAddr.sinkIEEE, &pEntry, Z_EXTADDR_LEN );
  gp_PopulateField( ( uint8* )&pEntryStruct->FirstLightweightSinkAddr.sinkNwkAddr, &pEntry, sizeof( uint16 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->SecondLightweightSinkAddr.sinkIEEE, &pEntry, Z_EXTADDR_LEN );
  gp_PopulateField( ( uint8* )&pEntryStruct->SecondLightweightSinkAddr.sinkNwkAddr, &pEntry, sizeof( uint16 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->SinkGroupEntriesBit, &pEntry, sizeof( uint8 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->FirstSinkGroupAddr.sinkGroup, &pEntry, sizeof( uint16 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->FirstSinkGroupAddr.alias, &pEntry, sizeof( uint16 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->SecondSinkGroupAddr.sinkGroup, &pEntry, sizeof( uint16 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->SecondSinkGroupAddr.alias, &pEntry, sizeof( uint16 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->groupcastRadius, &pEntry, sizeof( uint8 ) );
  gp_PopulateField( ( uint8* )&pEntryStruct->SearchCounter, &pEntry, sizeof( uint8 ) );

  return SUCCESS;
}

#endif
/*********************************************************************
*********************************************************************/

