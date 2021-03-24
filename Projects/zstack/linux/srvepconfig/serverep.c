/**************************************************************************************************
  Filename:       serverep.c
  Revised:        $Date$
  Revision:       $Revision$

  Description:    This file contains the Server Endpoint Configuration File
                  functions.

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
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/file.h>

#include "ZDObject.h"
#include "zcl.h"
#include "zcl_general.h"
#include "zcl_poll_control.h"
#include "zcl_ha.h"
#include "serverep.h"
#include "trace.h"
#ifdef GW_BUILD
#include "gatewaysrvr.h"
#endif

/**************************************************************************************************
 * Constants
 **************************************************************************************************/

// filename is not flexible, added to gszConfigTlgPath on init
CONST char gszConfigTlgFileName[] = "gateway_config.tlg";

#define MAX_TOKEN_SIZE    64    // maximum size for a single token
#define MAX_LINE_SIZE     256   // maximum line size (if lines are longer, they may split tokens)

// keywords
CONST char gszSrvKeywordDefine[]      = "#define";
CONST char gszSrvKeywordEndpoint[]    = "endpoint";
CONST char gszSrvKeywordEndpointDef[] = "endpointdef";
CONST char gszSrvKeywordClusterList[] = "clusterlist";
CONST char gszSrvKeywordAttr[]        = "attr";
CONST char gszSrvKeywordAttrList[]    = "attrlist";
CONST char gszSrvKeywordPollControl[]    = "pollcontrol";

const cId_t bdb_ZclType1Clusters[] =
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


const cId_t bdb_ZclType2Clusters[] =
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



/**************************************************************************************************
 * Typedefs
 **************************************************************************************************/
/*
typedef struct srvNameValuePair_tag // #define name value
{
  char  *name;     // pointer to the name
  uint16 value;    // number associated with name
} srvNameValuePair_t;

typedef struct srvNameValuePairLL_tag
{
  srvNameValuePair_t pair;            // name/value pair (#define)
  struct srvNameValuePairLL_tag *next;
} srvNameValuePairLL_t;

typedef struct srvAttrLL_tag
{
  zclAttrRec_t   attrrec;
  uint8          size;
  void          *defaultValue;   // default value
  char          *name;
  struct srvAttrLL_tag *next;
} srvAttrLL_t;

typedef struct srvAttrListLL_tag
{
  uint8           count;        // # of attributes in the list
  srvAttrLL_t   **ppAttrs;      // array of attributeLL pointers
  char           *name;         // name, for looking up
  struct srvAttrListLL_tag *next;
} srvAttrListLL_t;

typedef struct srvClusterListLL_tag
{
  uint8         count;
  uint16        *clusters;
  char          *name;
  struct srvClusterListLL_tag *next;
} srvClusterListLL_t;

typedef struct srvEndpointDefLL_tag
{
  uint16              profileid;
  uint16              deviceid;
  uint8               devicever;
  uint8               endpointType;
  srvClusterListLL_t *pInClusters;
  srvClusterListLL_t *pOutClusters;
  char               *name;
  struct srvEndpointDefLL_tag *next;
} srvEndpointDefLL_t;

typedef struct srvEndpointLL_tag
{
  uint8                 id;
  srvEndpointDefLL_t   *pEndpointDef;
  srvAttrListLL_t      *pAttrList;
  struct srvEndpointLL_tag *next;
} srvEndpointLL_t;
*/
/**************************************************************************************************
 * Function Prototypes
 **************************************************************************************************/

// display functions
static void SrvPrintEndpointConfiguration(int status, sSrvEndpointDef_t *pSrvEpDef);
static void SrvUnitTestPrintClusters( int nClusters, uint32_t *pClusters );
static void SrvUnitTestPrintAttrList( sSrvAttrList_t *pAttrList );
static void SrvUnitTestPrintAfEndpoint( AfRegisterReq *pAfEp );
static void srvPrintError( int status );

// parsing functions
static int srvParseTLGFile( FILE *fp );
static int srvParseDefine( FILE *fp );
static int srvParseAttr( FILE *fp );
static int srvParseAttrList( FILE *fp );
static int srvParseClusterList( FILE *fp );
static int srvParseEndpointDef( FILE *fp );
static int srvParseEndpoint( FILE *fp );
static int srvParseAllAttrs( FILE *fp );
static int srvParseAllAttrLists( FILE *fp );
static int srvParseAllClusterLists( FILE *fp );
static int srvParseAllEndpointDefs( FILE *fp );
static int srvParseAllEndpoints( FILE *fp );
static int srvConvertParsedFile(void);


// find functions
static srvAttrLL_t * srvFindAttr( char *name );
static srvAttrListLL_t * srvFindAttrList( char *name );
static srvClusterListLL_t * srvFindClusterList( char *name );
static srvEndpointDefLL_t * srvFindEndpointDef( char *name );
static bool srvLookupDefine(char *szName, uint16 *pValue);

// free functions
static void srvFreeLists(void);
static void srvFreeNameValuePairs(void);
static void srvFreeAttrs(void);
static void srvFreeAttrLists(void);
static void srvFreeClusterLists(void);
static void srvFreeEndpointDefs(void);
static void srvFreeEndpoints(void);

// helper functions
static bool srvIsSpace(char c);
static bool srvIsDigit(char c);
static bool srvIsCNameChar(char c);
static bool srvGetValue(char *sz, uint16 *pValue);
//static bool srvGetLongValue(char *sz, uint32 *pValue);
static bool srvGetValueWithLookup(char *sz, uint16 *pValue);
static char *srvParseAndCopyNextToken(FILE *fp);
static char *srvParseNextToken(FILE *fp);
static void srvParseRewind(FILE *fp);
static bool srvParseOpenBrace(FILE *fp);
static bool srvParseCloseBrace(FILE *fp);
static bool srvParse16BitValue( FILE *fp, uint16 *pValue );
static bool srvParse8BitValue( FILE *fp, uint8 *pValue );
static char *srvParseName( FILE *fp );
static int  srvParseCount( FILE *fp );
static uint8 bdb_zclFindingBindingEpType( srvClusterListLL_t *pInClusters, srvClusterListLL_t *pOutClusters );
static byte ZDO_AnyClusterMatches( byte ACnt, uint16 *AList, byte BCnt, uint16 *BList );

/**************************************************************************************************
 * Locals and Globals
 **************************************************************************************************/

// hard-coded #defines
CONST srvNameValuePair_t gaSrvNameValuePairs[] =
{
  { "ha_profile", 0x0104 },         // profile IDs
  { "ha_onofflight", 0x0100 },      // device IDs
  { "ha_dimmablelight", 0x0101 },
  { "ha_colordimmablelight", 0x0102 },
  { "ha_onoffswitch", 0x0103 },
  { "ha_dimmerswitch", 0x0104 },
  { "ha_colordimmerswitch", 0x0105 },
  { "basic", 0x0000 },              // cluster IDs
  { "identify", 0x0003 },
  { "groups", 0x0004 },
  { "scenes", 0x0005 },
  { "onoff", 0x0006 },
  { "rdonly", 0x01 },               // access control
  { "rdwr", 0x03 },                  //TODO: Create new access type to reflect client side read, both sides read, non specified assumes server side
  { "uint8", ZCL_DATATYPE_UINT8 },
  { "uint16", ZCL_DATATYPE_UINT16 },
  { "uint32", ZCL_DATATYPE_UINT32 },
  { "boolean", ZCL_DATATYPE_BOOLEAN },
  { "octetstr", ZCL_DATATYPE_OCTET_STR },
  { "charstr", ZCL_DATATYPE_CHAR_STR },
  { "enum8", ZCL_DATATYPE_ENUM8 }
};
CONST int giSrvNumNameValuePairs = sizeof(gaSrvNameValuePairs) / sizeof(srvNameValuePair_t);

// ZCL Gateway Attributes
uint8 srvAttrOnOff = 0;
uint16 srvAttrIdentifyTime = 0;

char gszConfigTlgPath[MAX_CONFIG_STRING_LEN] = "";

/*********************************************************************
 * Default Gateway Attributes
 */
zclAttrRec_t gsSrvDefaultZclAttrRecs[] =
{
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_ON_OFF,                      // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_BOOLEAN,               // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      NULL                                // NULL for read/write CB handler
    }
  },
  
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,             
    {  // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      NULL 
    }
  }
};

// MUST be same # of entries and in same order as gsSrvDefaultZclAttrRecs
// this points to the 
void * gsSrvDefaultAttrPtrs[] =
{
  &srvAttrOnOff,
  &srvAttrIdentifyTime
};

#define SRV_MAX_DEFAULT_ATTRS (sizeof( gsSrvDefaultZclAttrRecs ) / sizeof( gsSrvDefaultZclAttrRecs[0] ))

static srvNameValuePairLL_t *gpSrvNameValuePairHead;
static srvAttrLL_t          *gpSrvAttrHead;
static srvAttrListLL_t      *gpSrvAttrListHead;
static srvClusterListLL_t   *gpSrvClusterListHead;
static srvEndpointDefLL_t   *gpSrvEndpointDefHead;
static srvEndpointLL_t      *gpSrvEndpointHead;

// this will contain the list of all endpoints
static sSrvEndpointDef_t gsSrvEndpointDef;

// for reporting errors
static long int giSrvLineStart;        // file position of line
static int giSrvLineNum;               // current line # in file
static int giSrvLinePos;               // position in current line
static int giSrvLineLen;               // when parsing tokens
static char gszSrvLine[MAX_LINE_SIZE]; // current line

#ifdef GW_BUILD
//default endpoint to use for outgoing commands when not explicitly specified in API calls received from app
static uint8_t gw_src_ep = INVALID_EP;
#endif

/**************************************************************************************************
 * Code
 **************************************************************************************************/
/*********************************************************************
 * @fn      srvReadEndpointConfigFile
 *
 * @brief   Reads the configuration file. If no configuration file 
 *          available, uses hard-coded constants.
 *
 * @param   pStatus - SRVEPERR_NONE, SRVEPERR_NOFILE, SRVEPERR_BADFILE,
 *                    or SRVEPERR_MEMERROR
 *
 * @return  pointer to endpoint definition
 */
sSrvEndpointDef_t * srvReadEndpointConfigFile( int *pStatus )
{
  FILE *fp;

  fp = fopen(gszConfigTlgPath, "r");
  
  if ( fp )
  {
    uiPrintf(" Parsing %s\n", gszConfigTlgPath);

    // parse the file into gpSrvEndpointDef
    *pStatus = srvParseTLGFile( fp );
    fclose( fp );
    
    // display error
    if(*pStatus != SRVEPERR_NONE)
    {
      srvPrintError(*pStatus);
    }
  }
  else
  {
    uiPrintf(" Cannot open %s, using default endpoint only\n", gszConfigTlgPath);

    // parse the defaults into gpSrvEndpointDef
    *pStatus = SRVEPERR_NOFILE;    // using defaults, no file
  }

  // display the found endpoints if debug is on
  SrvPrintEndpointConfiguration(*pStatus, &gsSrvEndpointDef);

  return &gsSrvEndpointDef;
}

/*********************************************************************
 * @fn      srvGetServerAttrListOnEndpoint
 *
 * @brief   Finds server attributes registered to an endpoint
 *
 * @param   endpointId - endpoint ID to look for
 * @param   pAttrCount - returned attribute count
 *
 * @return  pointer to attribute record, NULL if not found
 */
zclAttrRec_t * srvGetServerAttrListOnEndpoint( uint8 endpointId, int *pAttrCount )
{
  int i;
  
  for ( i = 0; i < gsSrvEndpointDef.endpointCount; i++ )
  {
    if ( gsSrvEndpointDef.ppAttrLists[i]->endpoint == endpointId )
    {
      *pAttrCount = gsSrvEndpointDef.ppAttrLists[i]->numAttr;
      return gsSrvEndpointDef.ppAttrLists[i]->pAttrRecords;
    }
  }  
  return NULL;
}

/*********************************************************************
 * @fn      srvGetInClusterListOnEndpoint
 *
 * @brief   Finds input clusters registered to an endpoint
 *
 * @param   endpointId - endpoint ID to look for
 * @param   pClusterCount - returned cluster count
 *
 * @return  pointer to cluster list, NULL if not found
 */
uint32 * srvGetInClusterListOnEndpoint( uint8 endpointId, uint8 *pClusterCount )
{
  int i;
  
  for ( i = 0; i < gsSrvEndpointDef.endpointCount; i++ )
  {
    if ( gsSrvEndpointDef.ppEndpoints[i]->endpoint == endpointId )
    {
      *pClusterCount = gsSrvEndpointDef.ppEndpoints[i]->simpledesc->n_inputclusters;
      return gsSrvEndpointDef.ppEndpoints[i]->simpledesc->inputclusters;
    }
  }
  
  return NULL;
}

/*********************************************************************
 * @fn      srvFindAttributeRec
 *
 * @brief   Finds input clusters registered to an endpoint
 *
 * @param   endpointId - endpoint ID to look for
 * @param   clusterId  - cluster ID to look for
 * @param   attrId     - attribute ID to look for
 *
 * @return  pointer to the attribute record, or NULL if invalid attribute (doesn't exist)
 */
zclAttrRec_t * srvFindAttributeRec( uint8 endpointId, uint16 clusterId, uint16 attrId )
{
  zclAttrRec_t *pAttrRecs;
  int ep;
  int i;
  
  // look for a matching endpoint, clusterid, attrid
  for ( ep = 0; ep < gsSrvEndpointDef.endpointCount; ++ep )
  {
    if ( gsSrvEndpointDef.ppAttrLists[ep]->endpoint == endpointId )
    {
      pAttrRecs = gsSrvEndpointDef.ppAttrLists[ep]->pAttrRecords;
      for ( i = 0; i < gsSrvEndpointDef.ppAttrLists[ep]->numAttr; ++i )
      {
        if ( pAttrRecs[i].clusterID == clusterId && pAttrRecs[i].attr.attrId == attrId )
        {
          return &pAttrRecs[i];
        }
      }
    }
  }
  
  // not found
  return NULL;
}

/*********************************************************************
 * @fn      SrvUnitTestPrintClusters
 *
 * @brief   Prints clusters
 *
 * @param   nClusters - number of clusters
 * @param   pClusters - pointer to clusters
 *
 * @return  none
 */
static void SrvUnitTestPrintClusters( int nClusters, uint32_t *pClusters )
{
  int i;
  for(i=0; i<nClusters; ++i)
  {
    uiPrintfEx(trDEBUG, " %04X ", pClusters[i] );
  }
  uiPrintfEx(trDEBUG, "\n");
}

/*********************************************************************
 * @fn      SrvUnitTestPrintAfEndpoint
 *
 * @brief   Prints endpoint data
 *
 * @param   pAfEp - pointer to endpoint structure
 *
 * @return  none
 */
static void SrvUnitTestPrintAfEndpoint( AfRegisterReq *pAfEp )
{
  SimpleDescriptor *pSimpleDesc = pAfEp->simpledesc;
  
  uiPrintfEx(trDEBUG, "Endpoint %d\n", pSimpleDesc->endpoint);
  uiPrintfEx(trDEBUG, "profileid %04X\n", pSimpleDesc->profileid);
  uiPrintfEx(trDEBUG, "deviceid %04X\n", pSimpleDesc->deviceid);
  uiPrintfEx(trDEBUG, "devicever %d\n", pSimpleDesc->devicever );

  uiPrintfEx(trDEBUG, "inclusters  %d: \n", pSimpleDesc->n_inputclusters );
  SrvUnitTestPrintClusters(pSimpleDesc->n_inputclusters, pSimpleDesc->inputclusters );

  uiPrintfEx(trDEBUG, "outclusters %d: \n", pSimpleDesc->n_outputclusters );
  SrvUnitTestPrintClusters(pSimpleDesc->n_outputclusters, pSimpleDesc->outputclusters );
}

/*********************************************************************
 * @fn      SrvGetDataTypeLength
 *
 * @brief   Return the length of the datatype in octet.
 *
 *          NOTE: Should not be called for ZCL_DATATYPE_OCTECT_STR or
 *                ZCL_DATATYPE_CHAR_STR data types.
 *
 * @param   dataType - data type
 *
 * @return  length of data
 */
uint8 SrvGetDataTypeLength( uint8 dataType )
{
  uint8 len;

  switch ( dataType )
  {
    case ZCL_DATATYPE_DATA8:
    case ZCL_DATATYPE_BOOLEAN:
    case ZCL_DATATYPE_BITMAP8:
    case ZCL_DATATYPE_INT8:
    case ZCL_DATATYPE_UINT8:
    case ZCL_DATATYPE_ENUM8:
      len = 1;
      break;

    case ZCL_DATATYPE_DATA16:
    case ZCL_DATATYPE_BITMAP16:
    case ZCL_DATATYPE_UINT16:
    case ZCL_DATATYPE_INT16:
    case ZCL_DATATYPE_ENUM16:
    case ZCL_DATATYPE_SEMI_PREC:
    case ZCL_DATATYPE_CLUSTER_ID:
    case ZCL_DATATYPE_ATTR_ID:
      len = 2;
      break;

    case ZCL_DATATYPE_DATA24:
    case ZCL_DATATYPE_BITMAP24:
    case ZCL_DATATYPE_UINT24:
    case ZCL_DATATYPE_INT24:
      len = 3;
      break;

    case ZCL_DATATYPE_DATA32:
    case ZCL_DATATYPE_BITMAP32:
    case ZCL_DATATYPE_UINT32:
    case ZCL_DATATYPE_INT32:
    case ZCL_DATATYPE_SINGLE_PREC:
    case ZCL_DATATYPE_TOD:
    case ZCL_DATATYPE_DATE:
    case ZCL_DATATYPE_UTC:
    case ZCL_DATATYPE_BAC_OID:
      len = 4;
      break;

   case ZCL_DATATYPE_UINT40:
   case ZCL_DATATYPE_INT40:
       len = 5;
       break;

   case ZCL_DATATYPE_UINT48:
   case ZCL_DATATYPE_INT48:
       len = 6;
       break;

   case ZCL_DATATYPE_UINT56:
   case ZCL_DATATYPE_INT56:
       len = 7;
       break;

   case ZCL_DATATYPE_DOUBLE_PREC:
   case ZCL_DATATYPE_IEEE_ADDR:
   case ZCL_DATATYPE_UINT64:
   case ZCL_DATATYPE_INT64:
     len = 8;
     break;

    case ZCL_DATATYPE_128_BIT_SEC_KEY:
     len = SEC_KEY_LEN;
     break;

    case ZCL_DATATYPE_LONG_OCTET_STR:
      len = 64;
    break;

    case ZCL_DATATYPE_NO_DATA:
    case ZCL_DATATYPE_UNKNOWN:
      // Fall through

    default:
      len = 0;
      break;
  }

  return ( len );
}

/*********************************************************************
 * @fn      SrvUnitTestAttrValue
 *
 * @brief   Returns the value of an attribute in string form.
 *
 * @param   datatype - attribute data type
 * @param   pValue - pointer to attribute value
 *
 * @return  pointer to string
 */
static char * SrvUnitTestAttrValue(uint8 datatype, uint8 *pValue)
{
  static char szAttrValue[64] = "0";
  uint8 len;
  
  len = SrvGetDataTypeLength( datatype );
  if(len == 1)
  {
    sprintf(szAttrValue, "%02X", *pValue);
  }
  else if(len == 2)
  {
    sprintf(szAttrValue, "%04X", *(uint16 *)pValue);
  }
  else if(len == 4)
  {
    sprintf(szAttrValue, "%08X", *(uint32 *)pValue);
  }
  else
  {
    if(datatype == ZCL_DATATYPE_CHAR_STR || datatype == ZCL_DATATYPE_OCTET_STR)
    {
      len = *pValue;
      sprintf(szAttrValue, "attrlen=%d", len);
    }
    else
    {
      sprintf(szAttrValue, "uknown size for type=%02X", datatype);
    }
  }
  
  return szAttrValue;
}

/*********************************************************************
 * @fn      SrvUnitTestPrintAttrList
 *
 * @brief   Prints an attribute list
 *
 * @param   pAttrList - pointer to attribute list
 *
 * @return  none
 */
static void SrvUnitTestPrintAttrList( sSrvAttrList_t *pAttrList )
{
  zclAttrRec_t *pAttrRecArray;
  void **ppAttrPtrArray;
  int i;

  uiPrintfEx(trDEBUG, "%d Attributes:\n", pAttrList->numAttr);
  uiPrintfEx(trDEBUG, "{ Cluster, AttrId, type, accesscontrol, value }\n");
  pAttrRecArray = pAttrList->pAttrRecords;
  ppAttrPtrArray = pAttrList->ppAttrPtrs;
  
  for( i=0; i < pAttrList->numAttr; ++i )
  {
    uiPrintfEx(trDEBUG, "{ %04X, %04X, %02X, %02X, %s}\n",
      pAttrRecArray[i].clusterID,
      pAttrRecArray[i].attr.attrId,
      pAttrRecArray[i].attr.dataType,
      pAttrRecArray[i].attr.accessControl,
      SrvUnitTestAttrValue(pAttrRecArray[i].attr.dataType, ppAttrPtrArray[i]) );
  }
}

/*********************************************************************
 * @fn      SrvPrintEndpointConfiguration
 *
 * @brief   Handles the printing of information on an endpoint
 *
 * @param   status - error status
 * @param   pSrvEpDef - pointer to endpoint information
 *
 * @return  none
 */
static void SrvPrintEndpointConfiguration(int status, sSrvEndpointDef_t *pSrvEpDef)
{
  int ep;

  if( !pSrvEpDef || (status != SRVEPERR_NONE && status != SRVEPERR_NOFILE) )
  {
    uiPrintfEx (trERROR, "File Error %d\n", status );
    return;
  }

  for(ep = 0; ep < pSrvEpDef->endpointCount; ++ep)
  {
    // display the endpoint
    SrvUnitTestPrintAfEndpoint( pSrvEpDef->ppEndpoints[ep] );
    
    // display the attributes for the endpoint
    SrvUnitTestPrintAttrList( pSrvEpDef->ppAttrLists[ep] );
  }
}

/*********************************************************************
 * @fn      srvPrintError
 *
 * @brief   print the error (line and position)
 * 
 * @param   status - error status
 *
 * @return  none
 */
static void srvPrintError(int status)
{
  int i;
  char *szError;

  // show the error header
  if(status == SRVEPERR_MEMERROR)
    szError = "No Memory";
  else if ( status == SRVEPERR_NOSYMBOL )
    szError = "Missing Symbol";
  else if ( status == SRVEPERR_NO_HA_EP )
    szError = "At least one Home Automation endpoint (ProfileID=0x0104) must be defined";
  else
    szError = "Syntax Error";
  uiPrintfEx(trFATAL, "When processing config file %s: %s\n", gszConfigTlgPath, szError );

  if ( status != SRVEPERR_NO_HA_EP )
  {
    // display line
    uiPrintfEx(trFATAL, "%s", gszSrvLine );
	
    for(i=0; i<giSrvLinePos; ++i)
    {
      uiPrintfEx(trFATAL, " " );
    }
    uiPrintfEx(trFATAL, "^  Line %d, Col %d\n", giSrvLineNum, giSrvLinePos+1);
  }
}

/*********************************************************************
 * @fn      srvFindAttributePtr
 *
 * @brief   Finds input clusters registered to an endpoint
 *
 * @param   endpointId - endpoint ID to look for
 * @param   clusterId  - cluster ID to look for
 * @param   attrId     - attribute ID to look for
 *
 * @return  pointer to the attribute data, or NULL if invalid attribute (doesn't exist)
 */
void * srvFindAttributePtr( uint8 endpointId, uint16 clusterId, uint16 attrId )
{
  zclAttrRec_t *pAttrRecArray;
  void **ppAttrPtrArray;
  int ep;
  int i;
  
  // look for a matching endpoint, clusterid, attrid
  for ( ep = 0; ep < gsSrvEndpointDef.endpointCount; ++ep )
  {
    if( gsSrvEndpointDef.ppAttrLists[ep]->endpoint == endpointId )
    {
      pAttrRecArray  = gsSrvEndpointDef.ppAttrLists[ep]->pAttrRecords;
      ppAttrPtrArray = gsSrvEndpointDef.ppAttrLists[ep]->ppAttrPtrs;
      
      for(i=0; i< gsSrvEndpointDef.ppAttrLists[ep]->numAttr; ++i)
      {
        // did we find the attribute?
        if(pAttrRecArray[i].clusterID == clusterId && pAttrRecArray[i].attr.attrId == attrId )
        {
          uiPrintfEx(trDEBUG, "Attribute Pointer: Found attribute index - i: %d", i );
          uiPrintfEx(trDEBUG, "EndpointId: %d, ClusterId: %04X, AttrId: %04X\n", 
                       endpointId, clusterId, attrId );
          return ppAttrPtrArray[i];
        }
      }
    }
  }

  // not found
  return NULL;
}

/*********************************************************************
 * @fn      srvIsSpace
 *
 * @brief   Checks if character is a space
 * 
 * @param   c - character
 *
 * @return  TRUE if space, FALSE if not a space
 */
static bool srvIsSpace(char c)
{
  if( c==0 || c== ' ' || c == '\r' || c == '\n' || c == '\t' || c==',')
    return TRUE;
  return FALSE;
}

/*********************************************************************
 * @fn      srvIsDigit
 *
 * @brief   Checks if character is a digit
 * 
 * @param   c - character
 *
 * @return  TRUE if digit, FALSE if not a digit
 */
static bool srvIsDigit(char c)
{
  if( c >= '0' && c <= '9' )
    return TRUE;
  return FALSE;
}

/*********************************************************************
 * @fn      srvIsCNameChar
 *
 * @brief   Checks if a name character is a valid number or letter 
 * 
 * @param   c - character
 *
 * @return  TRUE if valid, FALSE if not valid
 */
static bool srvIsCNameChar(char c)
{
  if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c=='_') )
  {
    return TRUE;
  }
  return FALSE;
}

/*********************************************************************
 * @fn      srvXDigitVal
 *
 * @brief   Checks if character is hex digit
 * 
 * @param   c - character
 *
 * @return  0-15 (0-F) if a hex digit char, -1 otherwise
 */
static int srvXDigitVal(char c)
{
  if( c >= '0' && c <= '9' )
  {
    return (c - '0');
  }
  else if( c >= 'a' && c <= 'f' )
  {
    return 10 + (c - 'a');
  }
  else if( c >= 'A' && c <= 'F' )
  {
    return 10 + (c - 'A');
  }
  return -1;
}

/*********************************************************************
 * @fn      srvGetValueWithLookup
 *
 * @brief   Tries to lookup the value first. Otherwise, treats it as a number.
 * 
 * @param   sz - pointer to string value
 * @param   pValue - pointer to returned value
 *
 * @return  TRUE if successful, FALSE if failure
 */
static bool srvGetValueWithLookup(char *sz, uint16 *pValue)
{
  *pValue = 0;
  if( !srvLookupDefine( sz, pValue ) )
  {
    return srvGetValue( sz, pValue );
  }
  return TRUE;
}

/*********************************************************************
 * @fn      srvGetValue
 *
 * @brief   Returns a hex or decimal value (up to 16 bits).
 * 
 * @param   sz - pointer to string value
 * @param   pValue - pointer to returned value
 *
 * @return  TRUE if successful, FALSE if failure
 */
static bool srvGetValue(char *sz, uint16 *pValue)
{
  uint16 value = 0;
  
  // no string, has no value
  if( !sz )
    return FALSE;

  // hex value (e.g. 0x1E30)
  if(sz[0] == '0' && sz[1] == 'x')
  {
    sz += 2;

    while( srvXDigitVal(*sz) != -1 )
    {
      value = (value * 16) + srvXDigitVal(*sz);
      ++sz;
    }
  }
  
  // decimal value (e.g. 123)
  else if( srvIsDigit(*sz) )
  {
    do
    {
      value = (value * 10) + (*sz - '0');
      ++sz;
    } while( srvIsDigit(*sz) );
  }
  
  // neither a hex or decimal #
  else
  {
    return FALSE;
  }
  
  *pValue = value;
  return TRUE;
}

#if 0
/*********************************************************************
 * @fn      srvGetLongValue
 *
 * @brief   Used for 32-bit values Returns a hex or decimal value 
 *          (up to 16 bits).
 * 
 * @param   sz - pointer to string value
 * @param   pValue - pointer to returned value
 *
 * @return  TRUE if successful, FALSE if failure
 */
static bool srvGetLongValue(char *sz, uint32 *pValue)
{
  uint32 value = 0;
  
  // no string, has no value
  if( !sz )
    return FALSE;

  // hex value (e.g. 0x1E30)
  if(sz[0] == '0' && sz[1] == 'x')
  {
    sz += 2;

    while( srvXDigitVal(*sz) != -1 )
    {
      value = (value * 16) + srvXDigitVal(*sz);
      ++sz;
    }
  }
  
  // decimal value (e.g. 123)
  else if( srvIsDigit(*sz) )
  {
    do
    {
      value = (value * 10) + (*sz - '0');
      ++sz;
    } while( srvIsDigit(*sz) );
  }
  
  // neither a hex or decimal #
  else
  {
    return FALSE;
  }
  
  *pValue = value;
  return TRUE;
}
#endif
  
/*********************************************************************
 * @fn      srvParseGetNextToken
 *
 * @brief   get the next token (e.g. a keyword, a {, etc...)
 *
 * @param   fp - file pointer
 *
 * @return  NULL, not found (end of file)
 *          ptr to token, with fp just AFTER the token
 */
static char *srvParseNextToken(FILE *fp)
{
  static char szToken[MAX_TOKEN_SIZE];
  int tokenlen;
  bool found;

  // keep getting lines until end of file or we get a token
  found = FALSE;
  while(!found)
  {
    // get a new line if not at end of this line
    if(giSrvLinePos >= giSrvLineLen)
    {
      // get a line (or partial line, depending on position in file)
      // end of file, no more to get
      giSrvLineStart = ftell( fp );
      if( !fgets(gszSrvLine, sizeof(gszSrvLine), fp) )
      {
        return NULL;  // at end of file
      }

      // got a new line
      ++giSrvLineNum;      // lines are numbered 1-n
      giSrvLinePos = 0;
      giSrvLineLen = strlen(gszSrvLine);
    }

    // look for start of token in this line
    while(!found && gszSrvLine[giSrvLinePos])
    {
      // line
      if( srvIsSpace(gszSrvLine[giSrvLinePos]) )
      {
        ++giSrvLinePos;
      }

      // a comment
      else if(gszSrvLine[giSrvLinePos] == '/' )
      {
        break;  // on to next line the rest of this line is a comment
      }

      // not a space, must be start of a token
      else
      {
        found = TRUE;   // found a token
      }
    }

    // if broke because of the end of the line, advance line
    if( (gszSrvLine[giSrvLinePos] == 0) || (gszSrvLine[giSrvLinePos] == '/') )
    {
      giSrvLinePos = giSrvLineLen = 0;
    }
  }

  // determine token length (stops on comment char or space)
  tokenlen = 0;
  while( !srvIsSpace(gszSrvLine[giSrvLinePos]) && (tokenlen < (MAX_TOKEN_SIZE-1)) )
  {
    // beginning of a comment
    if(gszSrvLine[giSrvLinePos] == '/')
    {
      break;
    }

    // copy the token
    szToken[tokenlen] = gszSrvLine[giSrvLinePos];

    // on to next
    ++tokenlen;
    ++giSrvLinePos;
  }
  szToken[tokenlen] = 0;  // null terminate the token

  // if no token, then NULL
  if(tokenlen == 0)
  {
    return NULL;
  }

  return szToken;
}

/*********************************************************************
 * @fn      srvParseAndCopyNextToken
 *
 * @brief   allocate memory for a copy of the next token. Caller must free.
 *
 * @param   fp - file pointer
 *
 * @return  NULL, not found (end of file), or no memory
 *          ptr to allocated token, with fp just AFTER the token.
 *          caller MUST FREE the token
 */
static char *srvParseAndCopyNextToken(FILE *fp)
{
  char *psz;
  char *pszCopy;

  // parse to the next token
  psz = srvParseNextToken(fp);
  if(!psz)
  {
    return NULL;    // no token
  }
  
  pszCopy = malloc( strlen(psz) + 1 );
  if(!pszCopy)
  {
    return NULL;    // no memory
  }

  // copy the token and return it
  strcpy(pszCopy, psz);
  return pszCopy;
}

/*********************************************************************
 * @fn      srvParseGetNextSpecificToken
 *
 * @brief   find the next occurance of a specific token
 *
 * @param   fp - file pointer
 * @param   szToken - token to fine
 *
 * @return  NULL, not found (end of file)
 *          ptr to token, with fp just AFTER the token
 */
char *srvParseGetNextSpecificToken(FILE *fp, const char *szToken)
{
  char *psz;
  
  while(1)
  {
    // get the next token
    psz = srvParseNextToken(fp);
    if(!psz)
    {
      break;  // end of FILE
    }
    
    // same string?
    if(strcmp(szToken, psz) == 0)
    {
      return psz;
    }
  }
  return NULL;
}

/*********************************************************************
 * @fn      srvParseOpenBrace
 *
 * @brief   Verify the next token is a '{'
 *
 * @param   fp - file pointer
 *
 * @return  TRUE if it's an open brace.
 */
static bool srvParseOpenBrace(FILE *fp)
{
  char *psz;
  psz = srvParseNextToken(fp);
  if(*psz == '{')
  {
    return TRUE;
  }
  return FALSE;
}

/*********************************************************************
 * @fn      srvParseCloseBrace
 *
 * @brief   Verify the next token is a '}'
 *
 * @param   fp - file pointer
 *
 * @return  TRUE if it's an open brace.
 */
static bool srvParseCloseBrace(FILE *fp)
{
  char *psz;
  psz = srvParseNextToken(fp);
  if(*psz == '}')
  {
    return TRUE;
  }
  return FALSE;
}

/*********************************************************************
 * @fn      srvParse16BitValue
 *
 * @brief   Parse a 16-bit value
 *
 * @param   fp - file pointer
 * @param   pValue - pointer to returned value
 *
 * @return  TRUE if successful, FALSE if failure
 */
static bool srvParse16BitValue( FILE *fp, uint16 *pValue )
{
  return srvGetValueWithLookup( srvParseNextToken(fp), pValue );
}

/*********************************************************************
 * @fn      srvParse8BitValue
 *
 * @brief   Parse a 8-bit value
 *
 * @param   fp - file pointer
 * @param   pValue - pointer to returned value
 *
 * @return  TRUE if successful, FALSE if failure
 */
static bool srvParse8BitValue( FILE *fp, uint8 *pValue )
{
  uint16 value;
  bool worked;
  worked = srvGetValueWithLookup( srvParseNextToken(fp), &value );
  if(worked)
  {
    *pValue = (uint8)value;
  }
  return worked;
}

/*********************************************************************
 * @fn      srvParseName
 *
 * @brief   Parse a name. Allocates memory for the name.
 *
 * @param   fp - file pointer
 *
 * @return  NULL if not a name, or no memory
 */
static char *srvParseName( FILE *fp )
{
  char *name;

  name = srvParseAndCopyNextToken( fp );
  if( !name )
  {
    return NULL;
  }
  if( !srvIsCNameChar( *name ) ) // make sure it starts with a valid name char
  {
    return NULL;
  }  
  return name;
}

/*********************************************************************
 * @fn      srvParseCount
 *
 * @brief   Counts the # of items between braces, e.g. { a, b, c } would be 3.
 *          If no error, leaves the file pointer (and global line #, etc..) 
 *          where it started.
 *
 * @param   fp - file pointer
 *
 * @return  Returns count, or -1 if opening or closing brace is missing
 */
static int  srvParseCount( FILE *fp )
{
  char *szToken;
  int count = 0;
  int linepos;
  int linenum;
  long int linestart;
  
  // verify next token is an open brace
  if( !srvParseOpenBrace( fp ) )
  {
    return -1;
  }

  // remember current file position, after {
  linepos = giSrvLinePos;
  linenum = giSrvLineNum;
  linestart = giSrvLineStart;

  while(1)
  {
    szToken = srvParseNextToken( fp );
    if(!szToken)
    {
      return -1;  // hit end of file before closing brace
    }
    if( *szToken == '}' )
    {
      break;
    }
    ++count;
  }

  // restore current file position / line
  giSrvLinePos = linepos;
  giSrvLineNum = linenum;
  giSrvLineStart = linestart;
  fseek( fp, giSrvLineStart, SEEK_SET );
  fgets( gszSrvLine, sizeof(gszSrvLine), fp );

  return count;
}

/*********************************************************************
 * @fn      srvParseRewind
 *
 * @brief   Rewind to start of file
 *
 * @param   fp - file pointer
 *
 * @return  none
 */
void srvParseRewind( FILE *fp )
{
  rewind( fp );
  gszSrvLine[0] = 0; // clear line
  giSrvLineNum = 0;  // back to top of file
  giSrvLinePos = 0;
  giSrvLineLen = 0;
}

/*********************************************************************
 * @fn      srvLookupDefine
 *
 * @brief   Look up a pre-defined value
 *
 * @param   szName - name to lookup value
 * @param   pValue - returned value (if found)
 *
 * @return  TRUE if found FALSE if not
 */
static bool srvLookupDefine(char *szName, uint16 *pValue)
{
  int i;
  srvNameValuePairLL_t *pNameValue;

  if(!szName)
    return FALSE;

  // check built-in list
  for(i=0; i< giSrvNumNameValuePairs; ++i)
  {
    if(strcmp(gaSrvNameValuePairs[i].name, szName) == 0)
    {
      *pValue = gaSrvNameValuePairs[i].value;
      return TRUE;
    }
  }
  
  // check user-defined list
  pNameValue = gpSrvNameValuePairHead;
  while(pNameValue)
  {
    if(strcmp(pNameValue->pair.name, szName) == 0)
    {
      *pValue = pNameValue->pair.value;
      return TRUE;
    }
    pNameValue = pNameValue->next;
  }

  return FALSE;
}

/*********************************************************************
 * @fn      srvFindAttr
 *
 * @brief   find an attribute by name from linked list
 *
 * @param   name - name to lookup
 *
 * @return  pointer if found, NULL if not
 */
static srvAttrLL_t * srvFindAttr( char *name )
{
  srvAttrLL_t *pAttr;

  if(!name)
    return NULL;

  pAttr = gpSrvAttrHead;
  while(pAttr)
  {
    if(pAttr->name && (strcmp(pAttr->name, name) == 0) )
    {
      return pAttr;
    }
    pAttr = pAttr->next;
  }
  return NULL;
}

/*********************************************************************
 * @fn      srvFindAttrList
 *
 * @brief   find an attribute list by name from linked list
 *
 * @param   name - name to lookup value
 *
 * @return  pointer if found, NULL if not
 */
static srvAttrListLL_t * srvFindAttrList( char *name )
{
  srvAttrListLL_t *pAttrList;

  if(!name)
    return NULL;

  pAttrList = gpSrvAttrListHead;
  while(pAttrList)
  {
    if(pAttrList->name && (strcmp(pAttrList->name, name) == 0) )
    {
      return pAttrList;
    }
    pAttrList = pAttrList->next;
  }
  return NULL;
}

/*********************************************************************
 * @fn      srvFindClusterList
 *
 * @brief   find an cluster list by name from linked list
 *
 * @param   name - name to lookup value
 *
 * @return  pointer if found, NULL if not
 */
static srvClusterListLL_t * srvFindClusterList( char *name )
{
  srvClusterListLL_t *pClusterList;

  if(!name)
    return NULL;

  pClusterList = gpSrvClusterListHead;
  while(pClusterList)
  {
    if(pClusterList->name && (strcmp(pClusterList->name, name) == 0) )
    {
      return pClusterList;
    }
    pClusterList = pClusterList->next;
  }
  return NULL;
}

/*********************************************************************
 * @fn      srvFindEndpointDef
 *
 * @brief   find an endpoint definition by name from linked list
 *
 * @param   name - name to lookup
 *
 * @return  pointer if found, NULL if not
 */
static srvEndpointDefLL_t * srvFindEndpointDef( char *name )
{
  srvEndpointDefLL_t *pEndpointDef;

  if(!name)
    return NULL;

  pEndpointDef = gpSrvEndpointDefHead;
  while(pEndpointDef)
  {
    if(pEndpointDef->name && (strcmp(pEndpointDef->name, name) == 0) )
    {
      return pEndpointDef;
    }
    pEndpointDef = pEndpointDef->next;
  }
  return NULL;
}

/*********************************************************************
 * @fn      srvParseDefine
 *
 * @brief   Just found a #define token. The next 2 fields are name value
 *
 * @param   fp - file pointer
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseDefine( FILE *fp )
{
  char *pszName;
  char *pszValue;
  uint16 value;
  srvNameValuePairLL_t *pNameValue;
  srvNameValuePairLL_t *pThis;

  // #define name value (e.g. #define abc 0x1e)
  // verify name part
  pszName = srvParseName( fp );
  if( !pszName )
  {
    return SRVEPERR_BADFILE;
  }

  // verify value part
  pszValue = srvParseNextToken(fp);
  if( !srvGetValue(pszValue, &value) )
  {
    return SRVEPERR_BADFILE;
  }

  // allocate and fill out the structure
  pNameValue = malloc( sizeof(srvNameValuePairLL_t) );
  if(!pNameValue)
  {
    return SRVEPERR_MEMERROR;
  }
  pNameValue->pair.name = pszName;
  pNameValue->pair.value = value;
  pNameValue->next = NULL;
  
  // add to end of linked list
  if( !gpSrvNameValuePairHead )
    gpSrvNameValuePairHead = pNameValue;
  else
  {
    pThis = gpSrvNameValuePairHead;
    while(pThis->next)
    {
      pThis = pThis->next;
    }
    pThis->next = pNameValue;
  }
  
  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvFreeLists
 *
 * @brief   Free all the lists not needed at run-time (used only during parsing .tlg)
 *
 * @param   none
 *
 * @return  none
 */
static void srvFreeLists( void )
{
  srvFreeNameValuePairs();
  srvFreeAttrs();
  srvFreeAttrLists();
  srvFreeClusterLists();
  srvFreeEndpointDefs();
  srvFreeEndpoints();
}

/*********************************************************************
 * @fn      srvFreeLists
 *
 * @brief   Free name value pairs
 *
 * @param   none
 *
 * @return  none
 */
static void srvFreeNameValuePairs( void )
{
  srvNameValuePairLL_t *pNameValue;
  srvNameValuePairLL_t *next;

  // free defines
  pNameValue = gpSrvNameValuePairHead;
  while(pNameValue)
  {
    next = pNameValue->next;
    if(pNameValue->pair.name)
      free( pNameValue->pair.name );
    free( pNameValue );
    pNameValue = next;
  }
  gpSrvNameValuePairHead = NULL;

}

/*********************************************************************
 * @fn      srvFreeAttrs
 *
 * @brief   Free attributes
 *
 * @param   none
 *
 * @return  none
 */
static void srvFreeAttrs( void )
{
  srvAttrLL_t *pAttr;
  srvAttrLL_t *pNext;

  pAttr = gpSrvAttrHead;
  while( pAttr )
  {
    if(pAttr->name)
      free(pAttr->name);
    pNext = pAttr->next;
    free(pAttr);
    pAttr = pNext;
  }
  gpSrvAttrHead = NULL;
}

/*********************************************************************
 * @fn      srvFreeAttrLists
 *
 * @brief   Free attribute lists
 *
 * @param   none
 *
 * @return  none
 */
static void srvFreeAttrLists( void )
{
  srvAttrListLL_t *pAttrList;
  srvAttrListLL_t *pNext;

  pAttrList = gpSrvAttrListHead;
  while(pAttrList)
  {
    if(pAttrList->name)
      free(pAttrList->name);
    pNext = pAttrList->next;
    free(pAttrList);
    pAttrList = pNext;
  }
  gpSrvAttrListHead = NULL;
}

/*********************************************************************
 * @fn      srvFreeClusterLists
 *
 * @brief   Free cluster lists
 *
 * @param   none
 *
 * @return  none
 */
static void srvFreeClusterLists( void )
{
  srvClusterListLL_t *pClusterList;
  srvClusterListLL_t *pNext;
  
  pClusterList = gpSrvClusterListHead;
  while(pClusterList)
  {
    if(pClusterList->name)
      free(pClusterList->name);
    if(pClusterList->clusters)
      free(pClusterList->clusters);
    pNext = pClusterList->next;
    free(pClusterList);
    pClusterList = pNext;
  }
  gpSrvClusterListHead = NULL;
}

/*********************************************************************
 * @fn      srvFreeEndpointDefs
 *
 * @brief   Free endpoint definitions
 *
 * @param   none
 *
 * @return  none
 */
static void srvFreeEndpointDefs( void )
{
  srvEndpointDefLL_t *pEndpointDef;
  srvEndpointDefLL_t *pNext;

  pEndpointDef = gpSrvEndpointDefHead;
  while(pEndpointDef)
  {
    if(pEndpointDef->name)
      free(pEndpointDef->name);
    pNext = pEndpointDef->next;
    free(pEndpointDef);
    pEndpointDef = pNext;
  }
  gpSrvEndpointDefHead = NULL;
}

/*********************************************************************
 * @fn      srvFreeEndpoints
 *
 * @brief   Free endpoints
 *
 * @param   none
 *
 * @return  none
 */
static void srvFreeEndpoints( void )
{
  srvEndpointLL_t *pEndpoint;
  srvEndpointLL_t *pNext;
  
  pEndpoint = gpSrvEndpointHead;
  while(pEndpoint)
  {
    pNext = pEndpoint->next;
    free(pEndpoint);
    pEndpoint = pNext;
  }
  gpSrvEndpointHead = NULL;
}

/*********************************************************************
 * @fn      srvParseEndpoint
 *
 * @brief   parse this endpoint
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseEndpoint( FILE *fp )
{
  srvEndpointDefLL_t   *pEndpointDef;
  srvAttrListLL_t      *pAttrList;
  srvEndpointLL_t      *pEndpoint;
  srvEndpointLL_t      *pNewEndpoint;
  uint8 id;

  // endpoint { 3, OnOffLightEpDef, OnOffAttrDef }
  if( !srvParseOpenBrace( fp ) )
  {
    return SRVEPERR_BADFILE;
  }
  id = 0;
  if( !srvParse8BitValue( fp, &id ) || id == ZDO_EP || id == INVALID_EP || id == ZCL_OTA_ENDPOINT)
  {
    uiPrintfEx(trERROR, "Invalid Endpoint ID %d, use 1 - 254, except %d which is reserved for OTA\n", id, ZCL_OTA_ENDPOINT );
    return SRVEPERR_BADFILE;
  }
  pEndpointDef = srvFindEndpointDef( srvParseNextToken( fp ) );
  if( !pEndpointDef )
  {
    return SRVEPERR_BADFILE;
  }
  pAttrList = srvFindAttrList( srvParseNextToken( fp ) );
  if( !pAttrList )
  {
    return SRVEPERR_BADFILE;
  }
  if( !srvParseCloseBrace( fp ) )
  {
    return SRVEPERR_BADFILE;
  }
  
  // allocate a new structure and fill it in
  pNewEndpoint = malloc( sizeof( srvEndpointLL_t ) );
  if(!pNewEndpoint)
  {
    return SRVEPERR_MEMERROR;
  }
  pNewEndpoint->id = id;
  pNewEndpoint->pEndpointDef = pEndpointDef;
  pNewEndpoint->pAttrList = pAttrList;
  pNewEndpoint->next = NULL;

  // add it to the endpoint linked list
  if( !gpSrvEndpointHead )
  {
    gpSrvEndpointHead = pNewEndpoint;
  }
  else
  {
    pEndpoint = gpSrvEndpointHead;
    while( pEndpoint->next )
    {
      pEndpoint = pEndpoint->next;
    }
    pEndpoint->next = pNewEndpoint;
  }

#ifdef GW_BUILD
  //The user must define at least one home-automation endpoint in the tlg file.
  //This endpoint will be used as the default source endpoint for outgoing commands, if no endpoint is specified in the API arguments.
  //If more than one HA endpoint is defined, the first one that appears in the tlg file is used for this purpose.
  if ((pEndpointDef->profileid == 0x0104) && (gw_src_ep == INVALID_EP))
  {
  	gw_src_ep = id;
  }
#endif

  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseEndpointDef
 *
 * @brief   parse the endpoint definitions in the file
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseEndpointDef( FILE *fp )
{
  char *name;
  uint16 profileid;
  uint16 deviceid;
  uint8 devicever;
  srvClusterListLL_t *pInClusters;
  srvClusterListLL_t *pOutClusters;
  srvEndpointDefLL_t *pNewEndpointDef;
  srvEndpointDefLL_t *pEndpointDef;

  // endpointdef name { profileID, deviceID, deviceVer, inClusters, outClusters }
  name = srvParseName( fp );
  if( !name )
  {
    return SRVEPERR_BADFILE;
  }
  if( !srvParseOpenBrace( fp ) )
  {
    return SRVEPERR_BADFILE;
  }
  if( !srvParse16BitValue( fp, &profileid ) )
  {
    return SRVEPERR_BADFILE;
  }
  if( !srvParse16BitValue( fp, &deviceid ) )
  {
    return SRVEPERR_BADFILE;
  }
  if( !srvParse8BitValue( fp, &devicever ) )
  {
    return SRVEPERR_BADFILE;
  }
  pInClusters = srvFindClusterList( srvParseNextToken( fp ) );
  if( !pInClusters )
  {
    return SRVEPERR_NOSYMBOL;
  }
  pOutClusters = srvFindClusterList( srvParseNextToken( fp ) );
  if( !pOutClusters )
  {
    return SRVEPERR_NOSYMBOL;
  }
  if( !srvParseCloseBrace( fp ) )
  {
    return SRVEPERR_BADFILE;
  }

  // allocate and fill in the new endpointdef
  pNewEndpointDef = malloc( sizeof( srvEndpointDefLL_t ) );
  if( !pNewEndpointDef )
  {
    return SRVEPERR_MEMERROR;
  }
  pNewEndpointDef->profileid = profileid;
  pNewEndpointDef->deviceid = deviceid;
  pNewEndpointDef->devicever = devicever;
  pNewEndpointDef->pInClusters = pInClusters;
  pNewEndpointDef->pOutClusters = pOutClusters;

  //Get the finding & binding endpoint type
  pNewEndpointDef->endpointType = bdb_zclFindingBindingEpType(pInClusters, pOutClusters);
  pNewEndpointDef->name = name;
  pNewEndpointDef->next = NULL;

  // add new endpointdef to the linked list
  if( !gpSrvEndpointDefHead )
  {
    gpSrvEndpointDefHead = pNewEndpointDef;
  }
  else
  {
    pEndpointDef = gpSrvEndpointDefHead;
    while( pEndpointDef->next )
    {
      pEndpointDef = pEndpointDef->next;
    }
    pEndpointDef->next = pNewEndpointDef;
  }

  return SRVEPERR_NONE;
}




/*********************************************************************
 * @fn      bdb_zclFindingBindingEpType
 *
 * @brief   Gives the Ep Type according to application clusters in
 *          simple descriptor
 *
 * @return  epType - If Target, Initiator or both
 */
static uint8 bdb_zclFindingBindingEpType( srvClusterListLL_t *pInClusters, srvClusterListLL_t *pOutClusters )
{
  uint8 epType = 0;
  uint8 status;
  uint8 type1ClusterCnt;
  uint8 type2ClusterCnt;
  
  type1ClusterCnt = sizeof( bdb_ZclType1Clusters )/sizeof( uint16 );
  type2ClusterCnt = sizeof( bdb_ZclType2Clusters )/sizeof( uint16 );
  

  // Are there matching type 1 on server side?
  status = ZDO_AnyClusterMatches( pInClusters->count, 
                                  pInClusters->clusters,
                                  type1ClusterCnt,
                                  (uint16*)bdb_ZclType1Clusters);
  
  if( status == TRUE )
  {
    epType |= BDB_FINDING_AND_BINDING_TARGET;
  }
  
  // Are there matching type 1 on client side?
  status = ZDO_AnyClusterMatches( pOutClusters->count, 
                                  pOutClusters->clusters,
                                  type1ClusterCnt,
                                  (uint16*)bdb_ZclType1Clusters);
  
  if( status == TRUE )
  {
    epType |= BDB_FINDING_AND_BINDING_INITIATOR;
  }
  
  // Are there matching type 2 on server side?
  status = ZDO_AnyClusterMatches( pInClusters->count, 
                                  pInClusters->clusters,
                                  type2ClusterCnt,
                                  (uint16*)bdb_ZclType2Clusters);
  
  if( status == TRUE )
  {
    epType |= BDB_FINDING_AND_BINDING_INITIATOR;
  }
  
  // Are there matching type 2 on client side?
  status = ZDO_AnyClusterMatches( pOutClusters->count, 
                                  pOutClusters->clusters,
                                  type2ClusterCnt,
                                  (uint16*)bdb_ZclType2Clusters);
  
  if( status == TRUE )
  {
    epType |= BDB_FINDING_AND_BINDING_TARGET;
  }

  return epType;

}

/*********************************************************************
 * @fn          ZDO_AnyClusterMatches
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
static byte ZDO_AnyClusterMatches( byte ACnt, uint16 *AList, byte BCnt, uint16 *BList )
{
  byte x, y;

  for ( x = 0; x < ACnt; x++ )
  {
    for ( y = 0; y < BCnt; y++ )
    {
      if ( AList[x] == BList[y] )
      {
        return true;
      }
    }
  }

  return false;
}

/*********************************************************************
 * @fn      srvParseAttr
 *
 * @brief   parse a single attribute
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseAttr( FILE *fp )
{
  char *name;
  uint16 clusterid;
  uint16 attrid;
  uint8  type;
  uint8  access;
  uint8  size = 0;
  srvAttrLL_t *pAttrNew;
  srvAttrLL_t *pAttr;

  // attr Name { clusterid, attrid, type, size(opt) access }
  // e.g.  attr OnOff_OnOffAttr { onoff, 0, boolean, rdonly }
  // e.g.  attr OnOff_OnOffAttr { myClusterId, myAttrId, charstr, 16, rdwr }
  // note: if octetstr or charstr type, then the type is followed by a size
  name = srvParseName(fp);
  if( !name )
  {
    return SRVEPERR_BADFILE;
  }
  if(!srvParseOpenBrace( fp ))
  {
    return SRVEPERR_BADFILE;
  }
  if( !srvParse16BitValue( fp, &clusterid ) )
  {
    return SRVEPERR_BADFILE;
  }
  if( !srvParse16BitValue( fp, &attrid ) )
  {
    return SRVEPERR_BADFILE;
  }
  if( !srvParse8BitValue( fp, &type ) )
  {
    return SRVEPERR_BADFILE;
  }
  if( type == ZCL_DATATYPE_OCTET_STR || type == ZCL_DATATYPE_CHAR_STR )
  {
    if( !srvParse8BitValue( fp, &size ) )
    {
      return SRVEPERR_BADFILE;
    }
    // uiPrintfEx(trDEBUG, "-got size %d\n", size );
  }
  if( !srvParse8BitValue( fp, &access ) )
  {
    return SRVEPERR_BADFILE;
  }
  if(!srvParseCloseBrace( fp ))
  {
    return SRVEPERR_BADFILE;
  }

  // alloc and fill in the attribute
  pAttrNew = malloc( sizeof(srvAttrLL_t) );
  if(!pAttrNew)
  {
    return SRVEPERR_MEMERROR;
  }
  pAttrNew->attrrec.clusterID = clusterid;
  pAttrNew->attrrec.attr.attrId = attrid;
  pAttrNew->attrrec.attr.dataType = type;
  pAttrNew->attrrec.attr.accessControl = access;
  pAttrNew->attrrec.attr.dataPtr = NULL;
  pAttrNew->size = size;
  pAttrNew->name = name;
  pAttrNew->next = NULL;

  // add to end of linked list
  if( !gpSrvAttrHead )
  {
    gpSrvAttrHead = pAttrNew;
  }
  else
  {
    pAttr = gpSrvAttrHead;
    while(pAttr->next)
    {
      pAttr = pAttr->next;
    }
    pAttr->next = pAttrNew;
  }

  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseAttrList
 *
 * @brief   parse an attribute list
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseAttrList( FILE *fp )
{
  char *name;
  int count;
  int i;
  srvAttrListLL_t *pNewAttrList;
  srvAttrListLL_t *pAttrList;
  srvAttrLL_t *pAttr;
  char *szAttrName;

  // attrlist Name { AttrName, AttrName2... }
  name = srvParseName( fp );
  if( !name )
  {
    return SRVEPERR_BADFILE;
  }
  count = srvParseCount( fp ); // leaves position just after {
  if(count < 0 )
  {
    return SRVEPERR_BADFILE;
  }

  pNewAttrList = malloc( sizeof( srvAttrListLL_t ) );
  if(!pNewAttrList)
  {
    return SRVEPERR_MEMERROR;
  }
  pNewAttrList->ppAttrs = malloc( count * sizeof(srvAttrLL_t *) );
  if(!pNewAttrList->ppAttrs)
  {
    return SRVEPERR_MEMERROR;
  }
  pNewAttrList->count = count;
  pNewAttrList->name = name;
  pNewAttrList->next = NULL;

  // fill in array of pointers
  for(i = 0; i < count; ++i)
  {
    szAttrName = srvParseNextToken( fp );
    pAttr = srvFindAttr( szAttrName );
    if(!pAttr)
    {
      return SRVEPERR_NOSYMBOL;  // can't find the attribute
    }
    pNewAttrList->ppAttrs[i] = pAttr;
  }
  
  // add into attrlist list
  if( !gpSrvAttrListHead )
  {
    gpSrvAttrListHead = pNewAttrList;
  }
  else
  {
    pAttrList = gpSrvAttrListHead;
    while(pAttrList->next)
    {
      pAttrList = pAttrList->next;
    }
    pAttrList->next = pNewAttrList;
  }

  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseClusterList
 *
 * @brief   parse the cluster list
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseClusterList( FILE *fp )
{  
  char *name;
  int count;
  int i;
  uint16 *clusters;
  srvClusterListLL_t *pNewClusterList;
  srvClusterListLL_t *pClusterList;

  // clusterlist OnOffInputClusters { basic, identify, groups, scenes, onoff, 0x0501 }
  name = srvParseName( fp );
  if( !name )
  {
    return SRVEPERR_BADFILE;
  }

  // determine # of clusters in list
  count = srvParseCount( fp );
  if(count < 0 )
  {
    return SRVEPERR_BADFILE;
  }

  // get the clusters
  clusters = malloc( count * sizeof(uint16) );
  if(!clusters)
  {
    return SRVEPERR_MEMERROR;
  }
  for( i=0; i<count; ++i)
  {
    if( !srvParse16BitValue( fp, &clusters[i] ) )
    {
      return SRVEPERR_BADFILE;
    }
  }

  // allocate and fill in the new endpointdef
  pNewClusterList = malloc( sizeof( srvClusterListLL_t ) );
  if( !pNewClusterList )
  {
    return SRVEPERR_MEMERROR;
  }
  pNewClusterList->count = count;
  pNewClusterList->clusters = clusters;
  pNewClusterList->name = name;
  pNewClusterList->next = NULL;

  // add new endpointdef to the linked list
  if( !gpSrvClusterListHead )
  {
    gpSrvClusterListHead = pNewClusterList;
  }
  else
  {
    pClusterList = gpSrvClusterListHead;
    while( pClusterList->next )
    {
      pClusterList = pClusterList->next;
    }
    pClusterList->next = pNewClusterList;
  }

  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseAllDefines
 *
 * @brief   parse the defines in the file
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseAllDefines( FILE *fp )
{
  char *pszToken;
  int status;

  // start at beginning of file (in case not already there)
  srvParseRewind( fp );

  // parse the file for #defines
  while(1)
  {
    // look for #define
    pszToken = srvParseGetNextSpecificToken(fp, gszSrvKeywordDefine);

    // no more #defines
    if(!pszToken)
    {
      break;
    }

    // parse a define (and add to global linked list)
    status = srvParseDefine( fp );
    if( status != SRVEPERR_NONE )
    {
      return status;
    }
  }

  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseAllAttrs
 *
 * @brief   parse all the attributes in the file
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseAllAttrs( FILE *fp )
{
  char *pszToken;
  int status;

  // start at beginning of file (in case not already there)
  srvParseRewind( fp );

  // parse the file for attrs
  while(1)
  {
    // look for attr
    pszToken = srvParseGetNextSpecificToken(fp, gszSrvKeywordAttr);

    // no more attrs
    if(!pszToken)
    {
      break;
    }

    // parse an attr (and add to global linked list)
    status = srvParseAttr( fp );
    if( status != SRVEPERR_NONE )
    {
      return status;
    }
  }

  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseAllAttrLists
 *
 * @brief   parse all the attribute lists in the file
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseAllAttrLists( FILE *fp )
{
  char *pszToken;
  int status;

  srvParseRewind( fp );

  // parse the file for attrlists
  while(1)
  {
    // look for attrlist
    pszToken = srvParseGetNextSpecificToken(fp, gszSrvKeywordAttrList);

    // no more attrlists
    if(!pszToken)
    {
      break;
    }

    // parse an attrlist (and add to global linked list)
    status = srvParseAttrList( fp );
    if( status != SRVEPERR_NONE )
    {
      return status;
    }
  }
  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseAllClusterLists
 *
 * @brief   parse all the cluster lists in the file
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseAllClusterLists( FILE *fp )
{
  char *pszToken;
  int status;

  srvParseRewind( fp );

  // parse the file for attdefs
  while(1)
  {
    // look for cluster lists
    pszToken = srvParseGetNextSpecificToken(fp, gszSrvKeywordClusterList);

    // no more cluster lists
    if(!pszToken)
    {
      break;
    }

    // parse an attrlist (and add to global linked list)
    status = srvParseClusterList( fp );
    if( status != SRVEPERR_NONE )
    {
      return status;
    }
  }
  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseAllEndpointDefs
 *
 * @brief   parse all the endpoint definitions in the file
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseAllEndpointDefs( FILE *fp )
{
  char *pszToken;
  int status;

  srvParseRewind( fp );

  // parse the file for endpointdefs
  while(1)
  {
    // find the next endpointdef
    pszToken = srvParseGetNextSpecificToken(fp, gszSrvKeywordEndpointDef);

    // no more endpointdefs
    if(!pszToken)
    {
      break;
    }

    // parse an endpointdef (and add to global linked list)
    status = srvParseEndpointDef( fp );
    if( status != SRVEPERR_NONE )
    {
      return status;
    }
  }
  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseAllEndpoints
 *
 * @brief   parse all the endpoints in the file
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseAllEndpoints( FILE * fp)
{
  char *pszToken;
  int  status;

  // parse all endpoints
  // start at beginning of file
  srvParseRewind( fp );

  // context will point to just after the next endpoint token
  while(1)
  {
    // find next endpoint
    pszToken = srvParseGetNextSpecificToken(fp, gszSrvKeywordEndpoint);

    // no more endpoints, we're done
    if(!pszToken)
    {
      break;
    }
  
    // parse a single endpoint (adds to the global linked list)
    status = srvParseEndpoint( fp );
    if( status != SRVEPERR_NONE )
    {
      return status;
    }    
  }
  
  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvConvertParsedFile
 *
 * @brief   Convert parsed file from linked lists (xxHead) to sSrvEndpointDef_t
 *          in global gsSrvEndpointDef. Meant to be called only once.
 *
 * @param   none
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_MEMERROR
 */
static int srvConvertParsedFile(void)
{
  int endpointCount = 0;
  int ep;
  int i;
  uint8 size;
  uint8 type;
  uint8 *attrbuf;
  srvEndpointLL_t *pEndpoint;
  srvEndpointDefLL_t *pEndpointDef;
  srvClusterListLL_t *pClusterList;
  srvAttrListLL_t *pAttrList;
  AfRegisterReq *pAfReq;      // pointer to array of requests
  sSrvAttrList_t *pSrvAttrList; // list of attrbutes for a particular endpoint
  SimpleDescriptor *pSimpleDesc;  // simple descriptor

  // count # of endpoints
  pEndpoint = gpSrvEndpointHead;
  while(pEndpoint)
  {
    ++endpointCount;
    pEndpoint = pEndpoint->next;
  }

  if (endpointCount == 0)
  {
    uiPrintfEx(trERROR, "No endpoints defined in tlg file\n");
    return SRVEPERR_NO_HA_EP;
  }

  // tell the user
  uiPrintf( " %d app endpoints parsed in %s\n", endpointCount, gszConfigTlgPath);

  // allocate array of entry pointers
  gsSrvEndpointDef.endpointCount = endpointCount;
  gsSrvEndpointDef.ppEndpoints = malloc( endpointCount * sizeof(AfRegisterReq *) );
  gsSrvEndpointDef.ppAttrLists = malloc( endpointCount * sizeof(sSrvAttrList_t *) );
  if(!gsSrvEndpointDef.ppEndpoints || !gsSrvEndpointDef.ppAttrLists)
  {
    uiPrintfEx(trERROR, "Memory Error allocating endpoints\n");
    return SRVEPERR_MEMERROR;
  }

  // allocate the reset of the entries
  pAfReq = malloc( endpointCount * sizeof(AfRegisterReq) );         // array
  pSimpleDesc = malloc( endpointCount * sizeof(SimpleDescriptor) ); // array
  pSrvAttrList = malloc( endpointCount * sizeof(sSrvAttrList_t) );    // array
  if(!pAfReq || !pSrvAttrList || !pSimpleDesc)
  {
    uiPrintfEx(trERROR, "Memory Error allocating endpoints\n");
    return SRVEPERR_MEMERROR;
  }

  // fill out the endpoints from the linked list
  pEndpoint = gpSrvEndpointHead;

  
  for(ep=0; ep<endpointCount; ++ep)
  {
    // initialize the protobuf structures
    af_register_req__init( &pAfReq[ep] );
    simple_descriptor__init( &pSimpleDesc[ep] );

    // fill in AfRegisterReq
    pEndpointDef = pEndpoint->pEndpointDef;
    pAfReq[ep].endpoint = pEndpoint->id;
    pAfReq[ep].simpledesc = &pSimpleDesc[ep];
    pSimpleDesc[ep].endpoint = pEndpoint->id;
    pSimpleDesc[ep].profileid = pEndpointDef->profileid;
    pSimpleDesc[ep].deviceid = pEndpointDef->deviceid;
    pSimpleDesc[ep].devicever = pEndpointDef->devicever;

    // fill in input clusters
    pClusterList = pEndpointDef->pInClusters;
    pSimpleDesc[ep].n_inputclusters = pClusterList->count;
    pSimpleDesc[ep].inputclusters = malloc( pClusterList->count * sizeof(uint32_t) );
    if(!pSimpleDesc[ep].inputclusters)
    {
      uiPrintfEx(trERROR, "Memory Error allocating input clusters\n");
      return SRVEPERR_MEMERROR;
    }
    for(i=0; i<pClusterList->count; ++i)
    {
      pSimpleDesc[ep].inputclusters[i] = pClusterList->clusters[i];
    }
    
    // fill in output clusters
    pClusterList = pEndpointDef->pOutClusters;
    pSimpleDesc[ep].n_outputclusters = pClusterList->count;
    pSimpleDesc[ep].outputclusters = malloc( pClusterList->count * sizeof(uint32_t) );
    if(!pSimpleDesc[ep].outputclusters)
    {
      uiPrintf("- Memory Error allocating output clusters\n");
      return SRVEPERR_MEMERROR;
    }
    for(i=0; i<pClusterList->count; ++i)
    {
      pSimpleDesc[ep].outputclusters[i] = pClusterList->clusters[i];
    }
    
    // fill in sSrvAttrList_t
    pAttrList = pEndpoint->pAttrList;
    pSrvAttrList[ep].endpoint = pEndpoint->id;
    pSrvAttrList[ep].numAttr = pAttrList->count;
    pSrvAttrList[ep].pAttrRecords = malloc( pAttrList->count * sizeof(zclAttrRec_t) );
    pSrvAttrList[ep].ppAttrPtrs = malloc( pAttrList->count * sizeof( void * ) );
    
    if(!pSrvAttrList[ep].pAttrRecords || !pSrvAttrList[ep].ppAttrPtrs)
    {
      uiPrintf("- Memory Error allocating attribute records\n");
      return SRVEPERR_MEMERROR;
    }

    // fill in attribute records and create attrPtrs
    for(i=0; i < pAttrList->count; ++i)
    {
      pSrvAttrList[ep].pAttrRecords[i] = pAttrList->ppAttrs[i]->attrrec;

      // allocate all the attributes themselves
      type = pSrvAttrList[ep].pAttrRecords[i].attr.dataType;
      if(type == ZCL_DATATYPE_OCTET_STR || type == ZCL_DATATYPE_CHAR_STR)
      {
        size = 1 + pAttrList->ppAttrs[i]->size; // length byte, followed by octet str
      }
      else
      {
        size = SrvGetDataTypeLength( type );
      }
      if(!size)
      {
        uiPrintf( "- invalid size, attr %04X on cluster %04X\n",
          pSrvAttrList[ep].pAttrRecords[i].attr.attrId, pSrvAttrList[ep].pAttrRecords[i].clusterID ); 
        return SRVEPERR_MEMERROR;
      }

      attrbuf = malloc(size);
      if( !attrbuf )
      {
        uiPrintf("- Memory Error allocating attrbutes\n");
        return SRVEPERR_MEMERROR;
      }
      else
      {
        memset(attrbuf, 0, size);
        if(type == ZCL_DATATYPE_OCTET_STR || type == ZCL_DATATYPE_CHAR_STR)
        {
          attrbuf[0] = size-1;
        }
      }

      pSrvAttrList[ep].ppAttrPtrs[i] = attrbuf;
    }

    // set up global pointer, allowing for default GW_EP
    gsSrvEndpointDef.ppEndpoints[ep] = &pAfReq[ep];
    gsSrvEndpointDef.ppAttrLists[ep] = &pSrvAttrList[ep];

    // on to next endpoint
    pEndpoint = pEndpoint->next;
  }
    
  return SRVEPERR_NONE;
}

/*********************************************************************
 * @fn      srvParseTLGFile
 *
 * @brief   Parses TI Linux Gateway Endpoint Configuration file (.tlg).
 *          Memory is not freed on error, as it assumes caller will exit.
 *          main keywords: endpoint, endpointdef, clusterlist, attr, attrlist, attrdef
 *          parses file from lowest-level items (attr) to highest level (endpoint)
 *
 * @param   fp - file pointer, open for read
 *
 * @return  SRVEPERR_NONE (success), SRVEPERR_BADFILE, SRVEPERR_MEMERROR
 */
static int srvParseTLGFile( FILE *fp )
{
  int  status;

  // no file pointer, bad file
  if( !fp )
    return SRVEPERR_BADFILE;
    
  // parse defines
  uiPrintfEx(trDEBUG, " parsing defines\n");
  status = srvParseAllDefines( fp );
  if( status != SRVEPERR_NONE)
  {
    return status;
  }
  
  // parse attributes
  uiPrintfEx(trDEBUG, " parsing attributes\n");
  status = srvParseAllAttrs( fp );
  if( status != SRVEPERR_NONE)
  {
    return status;
  }
  
  // parse attribute lists
  uiPrintfEx(trDEBUG, " parsing attrlists\n");
  status = srvParseAllAttrLists( fp );
  if( status != SRVEPERR_NONE)
  {
    return status;
  }

  // parse cluster lists
  uiPrintfEx(trDEBUG, " parsing clusterlists\n");
  status = srvParseAllClusterLists( fp );
  if( status != SRVEPERR_NONE)
  {
    return status;
  }

  // parse endpoint definitions
  uiPrintfEx(trDEBUG, " parsing endpointdefs\n");
  status = srvParseAllEndpointDefs( fp );
  if( status != SRVEPERR_NONE)
  {
    return status;
  }

  // parse endpoints
  uiPrintfEx(trDEBUG, " parsing endpoints\n");
  status = srvParseAllEndpoints( fp );
  if( status != SRVEPERR_NONE)
  {
    return status;
  }

  // convert from linked list forms to the array form we need to return
  status = srvConvertParsedFile();

  // free up the endpoints
  srvFreeLists();

#ifdef GW_BUILD
  if (gw_src_ep == INVALID_EP)
  {
    status = SRVEPERR_NO_HA_EP;
  }
#endif

  return status;
}

#ifdef GW_BUILD
/**************************************************************************************************
 *
 * @fn          srvParseGwEndpointId
 *
 * @brief       Retrieve the gateway endpoint id from an address structure, if available. otherwise selects the default source endpoint
 *
 * @return     see brief above
 *
 **************************************************************************************************/
uint8 srvParseGwEndpointId(GwAddressStructT *addr)
{
	if (addr->has_gwendpointid)
	{
		return addr->gwendpointid;
	}

	return gw_src_ep;
}
#endif
