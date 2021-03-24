/**************************************************************************************************
 Filename:       nwkmgrsrvr.c
 Revised:        $Date$
 Revision:       $Revision 1.0.2$

 Description:    This file contains the Network Manager Server .


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
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>

#include "zstack.pb-c.h"
#include "nwkmgr.pb-c.h"
#include "server.pb-c.h"
#include "api_client.h"
#include "hal_rpc.h"
#include "api_server.h"
#include "nwkmgrsrv.h"
#include "configparser.h"
#include "nwkmgrservices.h"
#include "nwkmgrp2p.h"
#include "serverep.h"

#include "zcl_ha.h"

#include "trace.h"
#include "nwkmgr_pb_ctrl.h"
#include "zstack_pb_ctrl.h"
#include "server_pb_ctrl.h"

/**************************************************************************************************
 * Constant
 **************************************************************************************************/

#define DEVICE_TIMEOUT            9000  // 9 seconds (in milliseconds)

#define NM_DEVICE_VERSION         0
#define NM_FLAGS                  0

#define NM_HWVERSION              0
#define NM_ZCLVERSION             0

#define IGNORE_ADDR_TYPE          0xFF  // used for posting to transaction table without address mode

#define TRANSACTION_TIMEOUT             9000      // 9 seconds (ms)
#define SECURITY_TIMEOUT                9000      // 9 seconds to timeout
#define TRANSACTION_INTERVAL_TIME       250       // subtract timer value by 1/4 second intervals (ms)

#define ADD_TRANS_TABLE_SIZE            32        // adds 32 entries at a time

#define NWKMGRSRVR_SERVER_DEFAULT_PORT  2540

#define NWKMGRSRVR_EXITCODE_SELFSHUTDOWN  1
#define NWKMGRSRVR_EXITCODE_SOFTRESET  	  2
#define NWKMGRSRVR_EXITCODE_HARDRESET	    3
#define NWKMGRSRVR_EXITCODE_SLEEP 		    4
#define NWKMGRSRVR_EXITCODE_WAKEUP 		    5

/**************************************************************************************************
 * Typedefs
 **************************************************************************************************/

/**************************************************************************************************
 * Structures
 **************************************************************************************************/
typedef struct
{
  // use fields
  bool   inUse;
  int    timeout;  // timeout (will cause the default response to be sent)

  // field for sending back to the app
  int connection;
  uint8 appRsp;
  uint16 sequence;
  NwkAddressStructT addr;

  // fields for detecting when ZDP responds
  uint8 zdoRsp;
  uint16 srcaddr;
} transTable_t;

/**************************************************************************************************
 * Globals
 **************************************************************************************************/
// Paths for database .csv files, will be appended with gszNwkMgrDb_DeviceInfo_c
// and gszNwkMgrDb_Endpoints_c
char gszDbPath[MAX_CONFIG_STRING_LEN] = "";
char gszDbDeviceInfoPath[MAX_CONFIG_STRING_LEN] = "";
char gszDbEndpointsPath[MAX_CONFIG_STRING_LEN] = "";

int giNmDeviceTimeout = DEVICE_TIMEOUT;
int giNmStateTicks;

int8 zgChannelPowerList[16] = {127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127}; //default use maximum power

uint8 nwkFindingAndBindingTimerStatus = 0;

/**************************************************************************************************
 * Function Prototypes
 **************************************************************************************************/

// helper functions
void nmSyncPbCb( int connection, uint8 subSys, uint8 cmdId, uint16 len, 
                 uint8 *pData, uint8 type );
void nmHandleAppPbCb( int connection, uint8 subSys, uint8 cmdId, uint16 len, 
                      uint8 *pData, uint8 type );
void nmHandleServerPbCb( int connection, uint8 subSys, uint8 cmdId, uint16 len, 
                         uint8 *pData, uint8 type );
ZStatusValues sendAPICExpectDefaultStatus( int cmdID, int len, uint8 *pData );
static bool nmInit( void );
static void nmAppendTlgPathString( void );
static void timerHandler( int sig );
static void nmSendTimeoutRsp( transTable_t *pTrans );
static void nmDeviceReqRetryHandler( uint64_t ieeeAddr, uint16 shortAddr );
static void nmUpdateDeviceInRetryTable( uint64_t ieeeAddr );
static void getUserInput( void );
static void transitionState( bool keypressed, int key );
static bool nmRegCallbacks( void );
static void nmUiPrintData( ProtobufCBinaryData *pBinData );
void nmHandleStartupConfirms(int connection);

// transactions
static uint8 nmTransPost( int connection, uint8 appRsp, uint16 sequence, NwkAddressStructT *pAddr, uint8 zdoRsp, uint16 srcaddr );
static void nmTransFreeEntry(  transTable_t *pTrans );
static transTable_t *nmTransFindByZdoRsp( uint8 zdoRsp, uint16 srcaddr );

// helper functions
NwkDeviceInfoT *allocPbDeviceInfoList( int count, sNwkMgrDb_DeviceInfo_t *pInDeviceInfo );
void freePbDeviceInfoList( int count, NwkDeviceInfoT *pDeviceInfo );
static uint16 nmHandleCnfRsp( int connection, uint8 rspType, uint8 addrType, uint8 status );
void nwkSrvAddDeviceTcInfoCB(uint16 appSeq, sNwkMgrDb_DeviceInfo_t *pDeviceInfo, int err);  // callback function for device announce
void nwkSrvAddDeviceMaintenanceReqCB( uint16 appSeq, sNwkMgrDb_DeviceInfo_t *pDeviceInfo, int err );

// receive ZDO responses (from ZStack, lower layer)
static void handleAsyncMsgs( apicHandle_t handle, uint8 subSys, uint8 cmdID, uint16 len, uint8 *pMsg );

// process incoming ZDO and AF messages (from ZStack, lower layer)
static void processAfIncomingMsgInd( AfIncomingMsgInd *pInMsg );
static void processAfDataConfirmInd( AfDataConfirmInd *pInMsg );
static void processZdoNwkAddrRspInd( ZdoNwkAddrRspInd *pNwkAddrRsp );
static void processZdoIeeeAddrRspInd( ZdoIeeeAddrRspInd *pIeeeAddrRsp );
static void processZdoNodeDescRspInd( ZdoNodeDescRspInd *pNodeDescRsp );
static void processZdoSimpleDescRspInd( ZdoSimpleDescRspInd *pSimpleRsp );
static void processZdoActiveEpRspInd( ZdoActiveEndpointsRspInd *pNodeDescRsp );
static void processZdoMgmtPermitJoinRspInd( ZdoMgmtPermitJoinRspInd *pPermitJoinRsp );
static void processZdoTcDeviceInd( ZdoTcDeviceInd *pInMsg );
static void processSysResetInd( SysResetInd *pResetInd );
static void processZdoDeviceAnnounce( ZdoDeviceAnnounceInd *pInMsg );
static void processDevStateChangeInd( void );
static void processNwkCommissioningNoti( void );
static void processZdoLeaveInd( ZdoLeaveInd *pLeaveInd );
static void processZdoBindRspInd( ZdoBindRspInd *pInMsg );
static void processZdoUnbindRspInd( ZdoUnbindRspInd *pInMsg );
static void processZdoMgmtLqiRspInd( ZdoMgmtLqiRspInd *pInMsg );
static void processZdoMgmtRtgRspInd( ZdoMgmtRtgRspInd *pInMsg );
static void processZdoMgmtBindRspInd( ZdoMgmtBindRspInd *pInMsg );

// send funtions to ZDO (to ZStack, lower layer)
ZStatusValues sendAPICExpectDefaultStatus( int cmdID, int len, uint8 *pData );
ZStatusValues sendSysNwkInfoReadReqAndSetTxPower( void );
ZStatusValues sendSimpleDescReq( uint16 dstAddr, uint8 endpointId );
ZStatusValues sendMatchDescReq( uint16 dstAddr );
ZStatusValues sendZdoNodeDescReq( uint16 shortAddr );
ZStatusValues sendNwkRouteReq( DevNwkRouteReq *pRouteReq );
ZStatusValues sendZdoNwkAddrReq( uint64_t ieeeAddr );
ZStatusValues sendZdoActiveEndpointReq( uint16 shortAddr );
ZStatusValues sendSysResetReq( SysResetReq *pResetReq );
ZStatusValues sendZdoMgmtPermitJoinReq( ZdoMgmtPermitJoinReq *pPermitJoinReq );
ZStatusValues sendAppCnfStartCommissioningReq( AppCnfStartCommissioningReq *pStartCommissioningReq );
ZStatusValues sendAppCnfAddInstallCodeReq( AppCnfAddInstallCodeReq *pAddInstallCodeReq );
ZStatusValues sendZdoBindReq( ZdoBindReq *pBindReq );
ZStatusValues sendZdoUnbindReq( ZdoUnbindReq *pUnbindReq );
ZStatusValues sendMgmtBindReq( ZdoMgmtBindReq *pMgmtBindReq);
ZStatusValues sendZdoMgmtRtgReq( uint16 nwkaddr, uint8 startindex );
ZStatusValues sendZdoMgmtLqiReq( uint16 nwkaddr, uint8 startindex );
ZStatusValues sendSecNwkKeySetReq( SecNwkKeySetReq *pSetReq );
ZStatusValues sendSecNwkKeyUpdateReq( SecNwkKeyUpdateReq *pUpdateReq );
ZStatusValues sendSecNwkKeySwitchReq( void );

// App to NWK Server process functions
static void processNwkZigbeeSystemResetReq( int connection, NwkZigbeeSystemResetReq *pSystemResetReq );
static void processNwkZigBeeSystemSelfShutdownReq( void );
static void processNwkSetZigbeePowerModeReq( int connection, NwkSetZigbeePowerModeReq *pSetZigBeePowerModeReq );
static void processNwkGetLocalDeviceInfoReq( int connection );
//static void processNwkGetGwEndpointInfoReq( int connection, NwkGetGwEndpointInfoReq *pGwEndpointInfoReq );
static void processNwkGetGwEndpointInfoReq( int connection );
static void processNwkStartCommissioningReq( int connection, NwkStartCommissioningReq *pStartCommissioningReq );
static void processNwkSendSimpleDescriptorReq( int connection, NwkSendSimpleDescriptorReq *pSendSimpleDescriptorReq );
static void processNwkSetInstallCodeReq( int connection, NwkSetInstallCodeReq *pSetInstallCodeReq );
static void processNwkZigbeeNwkInfoReq( int connection, NwkZigbeeNwkInfoReq *pNwkInfoReq );
static void processNwkSetPermitJoinReq( int connection, NwkSetPermitJoinReq *pSetPermitJoinReq );
static void processNwkManagePeriodicMtoRouteReq( int connection, NwkManagePeriodicMtoRouteReq *pManagePeriodicMtoRouteReq);
static void processNwkGetNeighborTableReq( int connection, NwkGetNeighborTableReq *pGetNeighborTableReq);
static void processNwkGetRoutingTableReq( int connection, NwkGetRoutingTableReq *pGetRoutingTableReq);
static void processNwkGetDeviceListReq( int connection, NwkGetDeviceListReq *pGetDeviceListReq );
static void processNwkDeviceListMaintenanceReq( int connection, NwkDeviceListMaintenanceReq *pDeviceListMaintenanceReq );
static void processNwkRemoveDeviceReq( int connection, NwkRemoveDeviceReq *pRemoveDeviceReq );
static void processNwkSetBindingEntryReq( int connection, NwkSetBindingEntryReq *pBindingEntryReq );
static void processNwkGetBindingTableReq( int connection, NwkGetBindingTableReq *pBindingTableReq );

// send functions NWK to App (higher layer)
static void sendZbGenericCnf( int connection, NwkZigbeeGenericCnf *pGenericCnf );
static ZStatusValues sendSysSetTxPowerReq( int8 power );
static void sendNwkZigBeeSystemResetCnf( int connection, NwkZigbeeSystemResetCnf *pCnf );
static void sendNwkSetZigbeePowerModeCnf( int connection, NwkSetZigbeePowerModeCnf *pCnf );
static void sendNwkZigbeeNwkReadyInd( NwkZigbeeNwkReadyInd *pNwkReadyInd );
void sendNwkZigbeeDeviceInd( NwkZigbeeDeviceInd *pDeviceInd );
static void sendNwkGetDeviceListCnf( int connection, NwkGetDeviceListCnf *pGetDeviceListCnf );
static ZStatusValues sendNwkRemoveDeviceReq( NwkRemoveDeviceReq *pRemoveDeviceReq );
static void sendNwkZigbeeNwkInfoCnf( int connection, NwkZigbeeNwkInfoCnf *pNwkInfoCnf );
static void sendNwkSetBindingEntryRspInd( int connection, uint16 sequence, uint32_t status, NwkAddressStructT *srcaddr );
static void sendNwkSimpleDescriptorRspInd( int connection, ZdoSimpleDescRspInd *pInMsg );
static void sendNwkGetBindingTableRspInd( int connection, uint8 status, ZdoMgmtBindRspInd *pInMsg);
static void sendNwkGetNeighborTableRspInd( uint8 status, transTable_t *pTrans, ZdoMgmtLqiRspInd *pZdoMgmtLqiRspInd );
static void sendNwkGetRoutingTableRspInd( uint8 status, transTable_t *pTrans, ZdoMgmtRtgRspInd *pZdoMgmtRtgRspInd );
static void processNwkChangeNwkKeyReq( int connection, NwkChangeNwkKeyReq *pChangeNwkKeyReq);
static void processNwkGetNwkKeyReq( int connection, NwkGetNwkKeyReq *pGetNwkKeyReq);
static void sendNwkGetNwkKeyCnf( int connection, NwkGetNwkKeyCnf *pGetNwkKeyCnf );

/**************************************************************************************************
 * Locals
 **************************************************************************************************/

static DevState opState = DEV_STATE__INIT;


SysNwkInfoReadRsp gLocalDeviceInfo = SYS_NWK_INFO_READ_RSP__INIT;
DeviceTypes gLocalDeviceType = DEVICE_TYPES__INIT;

static afAddrType_t gNmDstAddr;

static uint16 gNmAppTransSeqNum = 0;   // App transaction sequence number (higher layer)

static int    gNmKeySwitchTimeout;     // timeout before issueing key switch
static uint8  gNmKeySeqNum = 0;        // rolling key sequence #

static int    gNmProcessCmds = TRUE;   // if FALSE, no commands will be processed
static int    gNmNeedConfirm;

uint8 cmdLineOption = 0;

// for use with NWK_DEVICE_LIST_MAINTENANCE_REQ
static sNwkMgrDb_DeviceList_t * gpnmDeviceList;
static int gnmDeviceIndex;

extern apisSysParams_t sysParams;

// Parameter Descriptors
configTableItem_t configItems[] =
{
 { gszDbPath, "DATABASE_PATH", TYPE_STRING, (MAX_CONFIG_STRING_LEN - 1) },
 { gszConfigTlgPath, "GW_CONFIG_TLG_PATH", TYPE_STRING, (MAX_CONFIG_STRING_LEN - 1) },
 { &(sysParams.port), "NWKMGR_SRVR_PORT", TYPE_UINT16, 1 },
 { &giNmDeviceTimeout, "TRANSACTION_TIMEOUT", TYPE_UINT16, 1 },
 { &zgChannelPowerList, "CHANNEL_POWER_LIST", TYPE_INT8, 16 },
 TRACE_CONFIG_ITEMS
};

apisSysParams_t sysParams =
{
  NWKMGRSRVR_SERVER_DEFAULT_PORT,     // Default port
  TRUE,                               // Network Manager Server Verbose mode
  (configTableItem_t *)configItems,   // Configuration structure array
  (sizeof ( configItems ) / sizeof (configTableItem_t)),
  1,                                  // 1 client connection
  handleAsyncMsgs,                    // function to handle incoming ZStack messages
  nmSyncPbCb,                        // handles incoming Network Manager messages
  NWKMGR_LAYER_NUMBER,
  NWKMGR_SERVER_NAME
};

transTable_t *gpTransTable;       // pointer to transaction table
int giTransTableEntries = 0;      // # of entries in the transaction table

apicHandle_t apiClientHandle;
#define NM_API_CLIENT  apiClientHandle

// one of three response types
enum nmRspType_t { NM_RSP_NONE, NM_RSP_GENERIC, NM_RSP_SPECIFIC };

/**************************************************************************************************
 **************************************************************************************************/

static int reset_hard, reset_soft, power_sleep, power_wakeup, shutdown;

/*********************************************************************
 * @fn      usage
 *
 * @brief   Provides printout of help options
 *
 * @param   pgm_path - pointer to program path
 * @param   exit_code - provides exit code
 *
 * @return  none
 */
static void usage( char *pgm_path )
{
  char *pgm;

  pgm = strrchr(pgm_path,'/');
  if (pgm)
    pgm += 1;
  else
    pgm = pgm_path;

  // note: this usage includes the srvwrapper args
  uiPrintfEx(trUNMASKABLE, "\nUsage: %s [--help |  -h | --reset_hard | " 
        "--reset_soft | --soft_reset | --hard_reset | " 
        "--shutdown | --sleep | --wakeup | -r [h|s] ] "
        "ipaddr:port [config.ini]\n\n", pgm);
}

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
int appArgs( int *p_argc, char ***p_argv )
{
  int argc = *p_argc;
  int new_argc;
  char **argv = *p_argv;
  int c,i,first_char,last_funky = -1, option_index=0;

  static struct option my_options[] = {
  // struct option format:
  //
  // { name, has_arg, int *flag, val } 
  // ----------------------------------
  // 
  // when an option is recognized, option_index will
  // be set to the row for that option in the options
  // array. if the user provides a crazy option that
  // isn't recognized, '?' is returned. If there is
  // a missing argument, either ':' or '?' may be
  // returned (see the manual).

  // when flag is specified, if we see the long name,
  // *flag = val will be done and getopt_long will
  // return 0. if flag is NULL (or 0), 'val' will be
  // returned. 
         
    {"hard_reset",0,&reset_hard,1},
    {"help", 0, 0, 'h'},
    {"reset",0,0,'r'},  // '-r h' or '-r s'
    {"reset_hard",0,&reset_hard,1},
    {"reset_soft",0,&reset_soft,1},
    {"shutdown",0,0,'S'},
    {"sleep",0,0,'s'},
    {"soft_reset",0,&reset_soft,1},
    {"wakeup",0,0,'w'},
  };

  uiPrintfEx(trINFO, " there are %d args\n",argc);
  for (i=0; i<argc; ++i)
  {
    uiPrintfEx(trINFO, " argv[%d] = %s\n",i,argv[i]);
  }

  if( argc < 2 )
  {
    usage(argv[0]);
    return 128;
  }

  // note: the combination of long and short options shown here are 
  // meant to be illustrative only, although they do get the job done. 
  // it is arguably simpler to use a non-zero flag for everything but
  // 'help'

  while (1) 
  {
    c = getopt_long(argc,argv,"hr:Ssw",
                my_options, &option_index);
    if ( c== -1)
      break;

    switch(c)
    {
      case 0:
        // long options where flag was non-null
        uiPrintfEx(trINFO," option %s selected\n",my_options[option_index].name);
        break;

      case '?':
        if (last_funky != optind && optind < argc)
        {
          uiPrintfEx(trINFO," ignoring funky option %s\n",argv[optind]);
          last_funky = optind;
        }
        break;

      case 'h':
        usage(argv[0]);
        return 128;
        break;

      case 'r':
        first_char = tolower(optarg[0]);
        if (first_char == 'h')
          reset_hard = 1;
        else if (first_char == 's')
          reset_soft = 1;
        else
        {
          uiPrintfEx(trINFO," unrecognized option to -r, '%s'\n",optarg);
          uiPrintfEx(trINFO," expected s[oft] or h[ard]\n");
          usage(argv[0]);
          return 129;
        }
        break;

      case 's':
        power_sleep = 1;
        break;

      case 'S':
        shutdown = 1;
        break;

      case 'w':
        power_wakeup = 1;
        break;

      default:
        if (isprint(c))
          uiPrintfEx(trINFO," getopt returned unexpected character '%c'!\n",c);
        else
          uiPrintfEx(trINFO," getopt returned character code 0%o!\n",c);
        break;
    }
  }

  // perhaps this should be under verbosity control but WTH...
  if (reset_hard)
    uiPrintfEx(trINFO,"\treset_hard is set\n");
  if (reset_soft)
    uiPrintfEx(trINFO,"\treset_soft is set\n");
  if (power_sleep)
    uiPrintfEx(trINFO,"\tpower_sleep is set\n");
  if (power_wakeup)
    uiPrintfEx(trINFO,"\tpower_wakeup is set\n");
  if (shutdown)
    uiPrintfEx(trINFO,"\tshutdown is set\n");

  if (reset_hard + reset_soft + power_sleep + power_wakeup + shutdown > 1)
  {
    uiPrintfEx(trINFO," more than one of the exclusive options are set!\n");
    usage(argv[0]);
    return 130;
  }


  new_argc = 1 + argc - optind;

  if (new_argc > 1)
  {
    int n_to_shift = new_argc -1;

    // we want to shift argv[opting] to argv[1], etc
    // we purposely want to leave argv[0] alone so that
    // we don't have to update that pointer on return
    for (i=0; i < n_to_shift; ++i)
      argv[1+i] = argv[optind + i];
  }

  *p_argc = new_argc;

  // flag to send the confirm on startup
  if( reset_hard || reset_soft || power_sleep || power_wakeup)
  {
    gNmNeedConfirm = TRUE;
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
  if ((register_pb_pointer_list(&nwkmgr_pb_pointer_list) == NULL) ||
    (register_pb_pointer_list(&server_pb_pointer_list) == NULL) ||
    (register_pb_pointer_list(&zstack_pb_pointer_list) == NULL))
  {
    uiPrintfEx(trERROR, "failed registering protobuf pointer lists\n");
  }
  return ( &sysParams );
}

int appInitPhase2( void )
{
  //nothing toi do here
  return 0;
}

int appInitPhase3( apicHandle_t *handles )
{
  bool init_rc;

  // Setup the timer handler
  signal( SIGALRM, timerHandler );

  // Usage message for key commands
  uiPrintfEx(trUNMASKABLE, "\n");
  uiPrintfEx(trUNMASKABLE, " ************************************************\n");
  uiPrintfEx(trUNMASKABLE, " *  Network Manager Server v1.0.2d              *\n");
  uiPrintfEx(trUNMASKABLE, " ************************************************\n"); 
  uiPrintfEx(trUNMASKABLE, "\n");

  // when sleeping, just exits, no client handle needed
  if( !power_sleep )
  {
    if(!handles)
    {
      apiClientHandle = NULL;
    }
    else
    {
      apiClientHandle = handles[0];
    }

    if(apiClientHandle == NULL)
    {
      uiPrintfEx(trUNMASKABLE, " Error - No ZStack Server Detected. Exitting...\n" );
      return 254;
    }

    apicInitializeConnectionInfp(apiClientHandle, ZSTACK_SERVER_NAME, ZSTACK_LAYER_NUMBER);

    // Initialize the App
    init_rc = nmInit();

    if ( init_rc == FALSE )
    {
      uiPrintfEx(trUNMASKABLE, "Initialization failed. Quitting now.\n\n" );
      return -1;
    }
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

/*********************************************************************
 * @fn      nmHandleStartupConfirms
 *
 * @brief   Handles start-up proceedures after NWK_ZIGBEE_SYSTEM_RESET_CNF
 *          or NWK_ZIGBEE_POWER_MODE_CNF
 *
 * @param   connection - tcp connection
 *
 * @return  none
 */
void nmHandleStartupConfirms( int connection )
{
  NwkZigbeeSystemResetCnf resetCnf = NWK_ZIGBEE_SYSTEM_RESET_CNF__INIT; 
  NwkSetZigbeePowerModeCnf powerModeCnf = NWK_SET_ZIGBEE_POWER_MODE_CNF__INIT;
  static char *pszConfirmNames[] =
  {
    "HardReset", "SoftReset", "Sleep", "WakeUp", "Unknown"
  };
  char *pConfirmName;
  
  // debugging info
  if( reset_hard )
    pConfirmName = pszConfirmNames[0];
  else if( reset_soft )
    pConfirmName = pszConfirmNames[1];
  else if( power_sleep )
    pConfirmName = pszConfirmNames[2];
  else if( power_wakeup )
    pConfirmName = pszConfirmNames[3];
  else
    pConfirmName = pszConfirmNames[4];
  uiPrintf( "NwkMgr Sending Startup Confirm: %s\n", pConfirmName );

  // it was a reset, send the confirm
  if( reset_hard || reset_soft)
  {
    // inform app with a confirm (and perhaps response)
    resetCnf.status = NWK_STATUS_T__STATUS_SUCCESS;
    resetCnf.resetmode = reset_hard ? NWK_RESET_MODE_T__HARD_RESET : NWK_RESET_MODE_T__SOFT_RESET;
    sendNwkZigBeeSystemResetCnf( connection, &resetCnf );
    
    // indicate if the network is online
	processNwkCommissioningNoti();
  }

  // sleeping or waking
  if(power_sleep || power_wakeup)
  {
    powerModeCnf.status = NWK_STATUS_T__STATUS_SUCCESS;
    powerModeCnf.powermode = power_sleep ? NWK_POWER_MODE_T__SLEEP : NWK_POWER_MODE_T__WAKEUP;
    sendNwkSetZigbeePowerModeCnf( connection, &powerModeCnf );

    if( power_sleep )
    {
      gNmProcessCmds = FALSE;
    }
    else
    {
	  processNwkCommissioningNoti();
    }
  }
}


/**************************************************************************************************
 *
 * @fn          nmSetChannelSpecificTxPower
 *
 * @brief      Set the TX power level
 *
 * @return    none
 *
 **************************************************************************************************/
void nmSetChannelSpecificTxPower(void) // nmSetChannelSpecificTxPower() can only be called AFTER calling sendSysNwkInfoReadReqAndSetTxPower() (e.g. via processDevStateChangeInd()), since it references gLocalDeviceInfo();
{
  static int16 lastSetTxPower = 999; //used for avoiding sending SetTxPower unnecessarily, e.g. on consecutive calls of NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_REQ API. It is assumed that any reset of the SoC will be alongside a reset of the linux servers, which will reset this static variable to its default value.
  int8 requestedTxPower;
	
  //if already active in a network - set TX power according to the channel being used (Otherwise - when a network connection will be made, the state change indication handler will take care of setting the TX power)
  if ( (gLocalDeviceInfo.devstate == DEV_STATE__DEV_ZB_COORD) || (gLocalDeviceInfo.devstate == DEV_STATE__DEV_ROUTER) )
  {
    if ((gLocalDeviceInfo.logicalchannel >= 11) && (gLocalDeviceInfo.logicalchannel <= 26))
    {
      requestedTxPower = zgChannelPowerList[gLocalDeviceInfo.logicalchannel - 11];
      if (lastSetTxPower != requestedTxPower)
      {
        uiPrintfEx( trINFO, "Requested TX power for channel %d is %d\n", gLocalDeviceInfo.logicalchannel, zgChannelPowerList[gLocalDeviceInfo.logicalchannel - 11]);
        if (sendSysSetTxPowerReq(zgChannelPowerList[gLocalDeviceInfo.logicalchannel - 11]) != ZSTATUS_VALUES__ZSuccess)
        {
          uiPrintfEx(trERROR, "Setting TX Power Failed\n" );
        }
        else
        {
          lastSetTxPower = requestedTxPower;
        }
      }
    }
    else
    {
  	  uiPrintfEx(trERROR, "Unexpected channel value\n" );
    }
  }
}

/**************************************************************************************************
 *
 * @fn          nmInit
 *
 * @brief       network manager Initialization
 *
 * @return      TRUE if success, FALSE if failure
 *
 **************************************************************************************************/
static bool nmInit( void )
{
  int status;
  
  // Append database and gateway_config.tlg paths
  nmAppendTlgPathString();
  
  giNmStateTicks = (giNmDeviceTimeout / 250);  // timeout in 1/4 second ticks
  
  // Set destination address to indirect
  gNmDstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  gNmDstAddr.endPoint = 0;
  gNmDstAddr.addr.shortAddr = 0;
  

  bool nmRegCBResult = nmRegCallbacks(); // // Register ZDO callbacks
  bool sendSysNWINFOReadResult = sendSysNwkInfoReadReqAndSetTxPower(); // Get local network information
  bool nwDBInitResult = nwkMgrDb_Init( TRUE ); // Initialize database


  if (nmRegCBResult != TRUE)
  {
     uiPrintfEx(trERROR, "Registering ZDO Callbacks Failed\n" );
  }

  if (sendSysNWINFOReadResult != ZSTATUS_VALUES__ZSuccess)
  {
     uiPrintfEx(trERROR, "Reading Local Network Information Failed\n" );
  }
  
  if (nwDBInitResult != TRUE)
  {
     uiPrintfEx(trERROR, "Database Initialization Failed\n" );
  }

  if ( (nmRegCBResult != TRUE)  // Register ZDO callbacks
       || (sendSysNWINFOReadResult != ZSTATUS_VALUES__ZSuccess) // Get local network information
       || (nwDBInitResult != TRUE) ) // Initialize database
  {
    return FALSE;
  }
  
  // Store gateway device's endpoint defaults and read endpoint config file (if available)
  pgSrvEndpointDefs = srvReadEndpointConfigFile( &status );
  
  if ( (status != SRVEPERR_NONE) && (status != SRVEPERR_NOFILE) )
  {
    uiPrintfEx(trERROR, "Endpoint Registration: Failed to Find Endpoint Information\n" );
    return FALSE;
  }
  
  // Set up the timer for removing transaction entry
  ualarm( TIMER_WAIT_PERIOD, 0 );
  
  return TRUE;
}

/**************************************************************************************************
 *
 * @fn          nmAppendTlgPathString
 *
 * @brief       Combines relevant tlg path and filename strings
 *
 * @return      none
 *
 **************************************************************************************************/
static void nmAppendTlgPathString( void )
{
  uint8 len;
  uint8 i;  // source character
  uint8 j = 0;  // destination character
  
  // Handle database paths
  len = strlen( gszDbPath );
  
  // Remove quotes from path string (e.g. "../test/" to ../test/)
  for ( i = 1; i < (len - 1); i++ )
  {
    gszDbPath[j++] = gszDbPath[i];
  }
  
  gszDbPath[j] = 0;
  
  // DB Device Info file path
  strcpy( gszDbDeviceInfoPath, gszDbPath );
  strcat( gszDbDeviceInfoPath, gszNwkMgrDb_DeviceInfo_c );
  
  // DB Endpoints file path
  strcpy( gszDbEndpointsPath, gszDbPath );
  strcat( gszDbEndpointsPath, gszNwkMgrDb_Endpoints_c );
  
  // Handle gateway_config.tlg file
  len = strlen( gszConfigTlgPath );
  j = 0;
  
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
 * @brief       Timer Callback function, called once ever 1/4 second.
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
  for ( i = 0; i < giTransTableEntries; i++ )
  {
    if ( (gpTransTable[i].inUse == TRUE) && (gpTransTable[i].timeout) )
    {
      gpTransTable[i].timeout -= TRANSACTION_INTERVAL_TIME;
      
      if ( !(gpTransTable[i].timeout) )
      {
        uiPrintfEx(trDEBUG, "NwkMgr Timeout: appRsp %d, srcaddr 0x%04X\n",
          gpTransTable[i].appRsp, gpTransTable[i].srcaddr );
   
        // send the timeout response (if appropriate)
		nmSendTimeoutRsp( &gpTransTable[i] );

        if ( gpTransTable[i].addr.addresstype != NWK_ADDRESS_TYPE_T__SELF )
        {
          // Add entry in device retry table
          nmDeviceReqRetryHandler( gpTransTable[i].addr.ieeeaddr, gpTransTable[i].srcaddr );
        }
   		
        nmTransFreeEntry( &gpTransTable[i] );
      }
    }
  }
  
  // timeout the key switch after the update
  if ( gNmKeySwitchTimeout )
  {
    gNmKeySwitchTimeout -= TRANSACTION_INTERVAL_TIME;
    if ( gNmKeySwitchTimeout == 0 )
    {
      sendSecNwkKeySwitchReq();
    }
  }

  // timers for discovery state machine
  zNwkSrv_UpdateTimers();

  ualarm( TIMER_WAIT_PERIOD, 0 );
}


/**************************************************************************************************
 *
 * @fn          nmSendTimeoutRsp
 *
 * @brief       Sends the timed out response
 *
 * @param       pTrans - pointer to transaction entry
 *
 * @return      none
 *
 **************************************************************************************************/
static void nmSendTimeoutRsp( transTable_t *pTrans )
{
  switch ( pTrans->appRsp )
  {
    case NWK_MGR_CMD_ID_T__NWK_GET_NEIGHBOR_TABLE_RSP_IND:
      sendNwkGetNeighborTableRspInd( NWK_STATUS_T__STATUS_TIMEOUT, pTrans, NULL );
      break;
    
    case NWK_MGR_CMD_ID_T__NWK_GET_ROUTING_TABLE_RSP_IND:
      sendNwkGetRoutingTableRspInd( NWK_STATUS_T__STATUS_TIMEOUT, pTrans, NULL );
      break;

    case NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_RSP_IND:
      //sendNwkSetBindingEntryRspInd( pTrans->connection, pTrans->sequence, NWK_STATUS_T__STATUS_TIMEOUT, &pTrans->addr );
      break;
	  
    case NWK_MGR_CMD_ID_T__NWK_GET_BINDING_TABLE_RSP_IND:
	  sendNwkGetBindingTableRspInd( pTrans->connection, NWK_STATUS_T__STATUS_TIMEOUT, NULL );
	  break;
	  
    default:
      break;
  }
}

/**************************************************************************************************
 *
 * @fn          nmDeviceReqRetryHandler
 *
 * @brief       Indicates how many times a request has been made to a device without a response
 *
 * @param       ieeeAddr - IEEE address of device
 * @param       shortAddr - short address of device
 *
 * @return      none
 *
 **************************************************************************************************/
static void nmDeviceReqRetryHandler( uint64_t ieeeAddr, uint16 shortAddr )
{
  gsNmMsgRetryTable_t *pRetryEntry;
  
  // Look for entry in retry table
  pRetryEntry = zNwkSrv_GetDeviceInRetryTable( ieeeAddr );
  
  // Send route request
  sendUnicastRouteReq( shortAddr );
  
  // If entry found
  if ( pRetryEntry )
  {
    pRetryEntry->failedCount++;
    
    if ( pRetryEntry->failedCount >= MAX_DEVICE_FAILED_ATTEMPTS )
    {
      uint8 status = NWK_DEVICE_STATUS_T__DEVICE_OFF_LINE;
      
      uiPrintf( "Maximum message tries to remote device reached:" );
      uiPrintf( "IeeeAddr: %016llX\n", ieeeAddr );
      
      pRetryEntry->inUse = FALSE; // entry is being handled, remove
      
      // Remote device unresponsive, update device status
      if ( !nwkMgrDb_SetDeviceStatus( ieeeAddr, status ) )
      {
        uiPrintf( "Unable to updated device: %016llX to status: %d\n",
                  ieeeAddr, status );
      }
    }
  }
  else
  {
    // Create new entry
    pRetryEntry = zNwkSrv_UpdateRetryTable();
    
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
 * @fn          nmUpdateDeviceInRetryTable
 *
 * @brief       Remote device has responded, delete retry table entry
 *
 * @param       ieeeAddr - IEEE address of device
 *
 * @return      none
 *
 **************************************************************************************************/
static void nmUpdateDeviceInRetryTable( uint64_t ieeeAddr )
{
  gsNmMsgRetryTable_t *pRetryEntry;
  
  // Look for device in retry table entry
  pRetryEntry = zNwkSrv_GetDeviceInRetryTable( ieeeAddr );
  
  // Remove device from table if found
  if ( pRetryEntry )
  {
    pRetryEntry->inUse = FALSE;
  }
}

/**************************************************************************************************
 *
 * @fn          handleAsyncMsgs
 *
 * @brief       Receives all incoming Asynchronous messages from the ZStack Server
 *
 * @param       handle - API client handles
 * @param       subSys - command field subsystem
 * @param       cmdId - incoming message command ID
 * @param       len - length of message
 * @param       pMsg - pointer to message in protobuf packed form
 *
 * @return      none
 *
 **************************************************************************************************/
static void handleAsyncMsgs( apicHandle_t handle, uint8 subSys, uint8 cmdId, uint16 len, uint8 *pMsg )
{
  void *pNativeMsg;
  
  // ignore incoming responses if sleepy
  if ( gNmProcessCmds == FALSE )
  {
    uiPrintfEx(trDEBUG, "In sleep mode, ZStack cmd %d ignored\n", cmdId );
    return;
  }

  if ( (pMsg == NULL) || (len == 0xFFFF) )
  {
    // Connection terminated
    uiPrintfEx(trDEBUG, "Server connection was terminated\n" );
    return;
  }
  
  if ( (subSys & RPC_SUBSYSTEM_MASK) == ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF )
  {
    switch ( cmdId )
    {
	  case ZSTACK_CMD_IDS__ZDO_MGMT_BIND_RSP:
		pNativeMsg = zdo_mgmt_bind_rsp_ind__unpack( NULL, len, pMsg );
        if ( pNativeMsg )
        {
          processZdoMgmtBindRspInd( (ZdoMgmtBindRspInd *)pNativeMsg );
          zdo_mgmt_bind_rsp_ind__free_unpacked( pNativeMsg, NULL );
        }
		break;

      case ZSTACK_CMD_IDS__ZDO_BIND_RSP:
        pNativeMsg = zdo_bind_rsp_ind__unpack( NULL, len, pMsg );
        if ( pNativeMsg )
        {
          processZdoBindRspInd( (ZdoBindRspInd *)pNativeMsg );
          zdo_bind_rsp_ind__free_unpacked( pNativeMsg, NULL );
        }
      break;
        
      case ZSTACK_CMD_IDS__ZDO_UNBIND_RSP:
        pNativeMsg = zdo_unbind_rsp_ind__unpack( NULL, len, pMsg );
        if ( pNativeMsg )
        {
          processZdoUnbindRspInd( (ZdoUnbindRspInd *)pNativeMsg );
          zdo_unbind_rsp_ind__free_unpacked( pNativeMsg, NULL );
        }
      break;
      
      case ZSTACK_CMD_IDS__ZDO_MGMT_LQI_RSP:
        pNativeMsg = zdo_mgmt_lqi_rsp_ind__unpack( NULL, len, pMsg );
        if ( pNativeMsg )
        {
          processZdoMgmtLqiRspInd( (ZdoMgmtLqiRspInd *)pNativeMsg );
          zdo_mgmt_lqi_rsp_ind__free_unpacked( pNativeMsg, NULL );
        }
      break;
      
      case ZSTACK_CMD_IDS__ZDO_MGMT_RTG_RSP:
        pNativeMsg = zdo_mgmt_rtg_rsp_ind__unpack( NULL, len, pMsg );
        if ( pNativeMsg )
        {
          processZdoMgmtRtgRspInd( (ZdoMgmtRtgRspInd *)pNativeMsg );
          zdo_mgmt_rtg_rsp_ind__free_unpacked( pNativeMsg, NULL );
        }
      break;

      case ZSTACK_CMD_IDS__ZDO_NWK_ADDR_RSP:
        {
          pNativeMsg = zdo_nwk_addr_rsp_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processZdoNwkAddrRspInd( (ZdoNwkAddrRspInd *)pNativeMsg );
            zdo_nwk_addr_rsp_ind__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;        
        
      case ZSTACK_CMD_IDS__ZDO_IEEE_ADDR_RSP:
        {
          pNativeMsg = zdo_ieee_addr_rsp_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processZdoIeeeAddrRspInd( (ZdoIeeeAddrRspInd *)pNativeMsg );
            zdo_ieee_addr_rsp_ind__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;        
        
      case ZSTACK_CMD_IDS__ZDO_NODE_DESC_RSP:
        {
          pNativeMsg = zdo_node_desc_rsp_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processZdoNodeDescRspInd( (ZdoNodeDescRspInd *)pNativeMsg );
            zdo_node_desc_rsp_ind__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;
        
      case ZSTACK_CMD_IDS__ZDO_SIMPLE_DESC_RSP:
        {
          pNativeMsg = zdo_simple_desc_rsp_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processZdoSimpleDescRspInd( (ZdoSimpleDescRspInd *)pNativeMsg );
            zdo_simple_desc_rsp_ind__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;
        
      case ZSTACK_CMD_IDS__ZDO_ACTIVE_EP_RSP:
        {
          pNativeMsg = zdo_active_endpoints_rsp_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processZdoActiveEpRspInd( (ZdoActiveEndpointsRspInd *)pNativeMsg );
            zdo_active_endpoints_rsp_ind__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;
        
      case ZSTACK_CMD_IDS__ZDO_MGMT_PERMIT_JOIN_RSP:
        {
          pNativeMsg = zdo_mgmt_permit_join_rsp_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processZdoMgmtPermitJoinRspInd( (ZdoMgmtPermitJoinRspInd *)pNativeMsg );
            zdo_mgmt_permit_join_rsp_ind__free_unpacked( pNativeMsg, NULL );
          }
        }
        break;
        
      case ZSTACK_CMD_IDS__SYS_RESET_IND:
        {
          pNativeMsg = sys_reset_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processSysResetInd( (SysResetInd *)pNativeMsg );
            sys_reset_ind__free_unpacked( pNativeMsg, NULL );
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
        
      case ZSTACK_CMD_IDS__ZDO_DEVICE_ANNOUNCE:
        pNativeMsg = zdo_device_announce_ind__unpack( NULL, len, pMsg );
        if ( pNativeMsg )
        {
          processZdoDeviceAnnounce( (ZdoDeviceAnnounceInd *)pNativeMsg );
          zdo_device_announce_ind__free_unpacked( pNativeMsg, NULL );
        }       
        break;
        
      case ZSTACK_CMD_IDS__DEV_STATE_CHANGE_IND:
        {   
          pNativeMsg = dev_state_change_ind__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processDevStateChangeInd( );

            dev_state_change_ind__free_unpacked( pNativeMsg, NULL );
          }       
        }
        break;
		
      case ZSTACK_CMD_IDS__APP_CNF_COMMISSIONING_NOTIFICATION:
        {   
		  
          pNativeMsg = app_cnf_commissioning_notification__unpack( NULL, len, pMsg );
          if ( pNativeMsg )
          {
            processNwkCommissioningNoti( );

            app_cnf_commissioning_notification__free_unpacked( pNativeMsg, NULL );
          }       
        }
        break;
		
      case ZSTACK_CMD_IDS__ZDO_LEAVE_IND:
        pNativeMsg = zdo_leave_ind__unpack( NULL, len, pMsg );
        if ( pNativeMsg )
        {
          processZdoLeaveInd( (ZdoLeaveInd *)pNativeMsg );
          zdo_leave_ind__free_unpacked( pNativeMsg, NULL );
        } 
        break;

      case ZSTACK_CMD_IDS__ZDO_TC_DEVICE_IND:
        pNativeMsg = zdo_tc_device_ind__unpack( NULL, len, pMsg );
        if ( pNativeMsg )
        {
          processZdoTcDeviceInd( (ZdoTcDeviceInd *)pNativeMsg );
          zdo_tc_device_ind__free_unpacked( pNativeMsg, NULL );
        } 
        break;

      default:
        uiPrintfEx(trDEBUG, "Undefined asynchronous message received: %d\n\n", cmdId );
        break;
    }
  }
}

/*********************************************************************
 * @fn      nmSyncPbCb
 *
 * @brief   Handles incoming Synchronous App Protobuf messages (from App to NwkManager)
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
void nmSyncPbCb( int connection, uint8 subSys, uint8 cmdId, uint16 len, 
                 uint8 *pData, uint8 type )
{ 
  if ( type == SERVER_CONNECT )
  {
    // New Connection
    if( gNmNeedConfirm )
    {
      nmHandleStartupConfirms( connection );      
      // gNmNeedConfirm = FALSE;
    }
    return;
  }
  else  if ( type == SERVER_DISCONNECT )
  {
    // Connection terminated
    return;
  }
  
  // ignore incoming responses if sleepy
  if ( (gNmProcessCmds == FALSE) && (cmdId != NWK_MGR_CMD_ID_T__NWK_SET_ZIGBEE_POWER_MODE_REQ) )
  {
    uiPrintfEx(trINFO, "In sleep mode, App cmd %d ignored\n", cmdId );
    return;
  }

  if ( (subSys & RPC_SUBSYSTEM_MASK) == Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR )
  {
    // App to Network Manager
    nmHandleAppPbCb( connection, subSys, cmdId, len, pData, type );
  }
  else if ( (subSys & RPC_SUBSYSTEM_MASK) == Z_STACK_SERVER_SYS_ID_T__RPC_SYS_PB_SRVR )
  {
    // Gateway/OTA server to Network Manager
    nmHandleServerPbCb( connection, subSys, cmdId, len, pData, type );
  }
  else
  {
    uiPrintfEx(trDEBUG, "Network Manager Received Unsupported Synchronous Command:" );
    uiPrintfEx(trINFO, "SubSysId: %d, CmdId: %d\n", subSys, cmdId );
  }
}

/*********************************************************************
 * @fn      nmHandleAppPbCb
 *
 * @brief   Handles incoming App Protobuf messages (from App to NwkManager)
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
void nmHandleAppPbCb( int connection, uint8 subSys, uint8 cmdId, uint16 len, 
                      uint8 *pData, uint8 type )
{
  void *pNativeMsg;
  
  switch ( cmdId )
  {
    case NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_RESET_REQ:
      pNativeMsg = nwk_zigbee_system_reset_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkZigbeeSystemResetReq( connection, (NwkZigbeeSystemResetReq *)pNativeMsg );
        nwk_zigbee_system_reset_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_SELF_SHUTDOWN_REQ:
      processNwkZigBeeSystemSelfShutdownReq( );
      break;

    case NWK_MGR_CMD_ID_T__NWK_SET_ZIGBEE_POWER_MODE_REQ:
      pNativeMsg = nwk_set_zigbee_power_mode_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkSetZigbeePowerModeReq( connection, (NwkSetZigbeePowerModeReq *)pNativeMsg );
        nwk_set_zigbee_power_mode_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;
      
    case NWK_MGR_CMD_ID_T__NWK_GET_LOCAL_DEVICE_INFO_REQ:
      processNwkGetLocalDeviceInfoReq( connection );
      break;

	case NWK_MGR_CMD_ID_T__NWK_GET_GW_ENDPOINT_INFO_REQ:
	  processNwkGetGwEndpointInfoReq( connection );
      break;

    case NWK_MGR_CMD_ID_T__NWK_START_COMMISSIONING_REQ:
	  pNativeMsg = nwk_start_commissioning_req__unpack( NULL, len, pData );
	  if ( pNativeMsg )
	  {
		processNwkStartCommissioningReq( connection, (NwkStartCommissioningReq *)pNativeMsg );
		nwk_start_commissioning_req__free_unpacked( pNativeMsg, NULL ); 
	  }
      break;

	case NWK_MGR_CMD_ID_T__NWK_SEND_SIMPLE_DESCRIPTOR_REQ:
	  pNativeMsg = nwk_send_simple_descriptor_req__unpack( NULL, len, pData );
	  if ( pNativeMsg )
	  {
		processNwkSendSimpleDescriptorReq( connection, (NwkSendSimpleDescriptorReq *)pNativeMsg );
		nwk_send_simple_descriptor_req__free_unpacked( pNativeMsg, NULL );
	  }
      break;

	case NWK_MGR_CMD_ID_T__NWK_SET_INSTALLCODE_REQ:
	  pNativeMsg = nwk_set_install_code_req__unpack( NULL, len, pData );
	  if ( pNativeMsg )
	  {
		processNwkSetInstallCodeReq( connection, (NwkSetInstallCodeReq *)pNativeMsg );
		nwk_set_install_code_req__free_unpacked( pNativeMsg, NULL );
	  }
      break;

    case NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_REQ:
      pNativeMsg = nwk_zigbee_nwk_info_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkZigbeeNwkInfoReq( connection, (NwkZigbeeNwkInfoReq  *)pNativeMsg );
        nwk_zigbee_nwk_info_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case NWK_MGR_CMD_ID_T__NWK_SET_PERMIT_JOIN_REQ:
      pNativeMsg = nwk_set_permit_join_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkSetPermitJoinReq( connection, (NwkSetPermitJoinReq *)pNativeMsg );
        nwk_set_permit_join_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case NWK_MGR_CMD_ID_T__NWK_MANAGE_PERIODIC_MTO_ROUTE_REQ:
      pNativeMsg = nwk_manage_periodic_mto_route_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkManagePeriodicMtoRouteReq( connection, (NwkManagePeriodicMtoRouteReq *)pNativeMsg );
        nwk_manage_periodic_mto_route_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case NWK_MGR_CMD_ID_T__NWK_GET_NEIGHBOR_TABLE_REQ:
      pNativeMsg = nwk_get_neighbor_table_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkGetNeighborTableReq( connection, (NwkGetNeighborTableReq *)pNativeMsg );
        nwk_get_neighbor_table_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case NWK_MGR_CMD_ID_T__NWK_GET_ROUTING_TABLE_REQ:
      pNativeMsg = nwk_get_routing_table_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkGetRoutingTableReq( connection, (NwkGetRoutingTableReq *)pNativeMsg );
        nwk_get_routing_table_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case NWK_MGR_CMD_ID_T__NWK_CHANGE_NWK_KEY_REQ:
      pNativeMsg = nwk_change_nwk_key_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkChangeNwkKeyReq( connection, (NwkChangeNwkKeyReq *)pNativeMsg );
        nwk_change_nwk_key_req__free_unpacked( pNativeMsg, NULL ); 
      }
    break;

    case NWK_MGR_CMD_ID_T__NWK_GET_NWK_KEY_REQ:
      pNativeMsg = nwk_get_nwk_key_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkGetNwkKeyReq( connection, (NwkGetNwkKeyReq *)pNativeMsg );
        nwk_get_nwk_key_req__free_unpacked( pNativeMsg, NULL ); 
      }
    break;

    case NWK_MGR_CMD_ID_T__NWK_GET_DEVICE_LIST_REQ:
      pNativeMsg = nwk_get_device_list_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkGetDeviceListReq( connection, (NwkGetDeviceListReq *)pNativeMsg );
        nwk_get_device_list_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case NWK_MGR_CMD_ID_T__NWK_DEVICE_LIST_MAINTENANCE_REQ:
      pNativeMsg = nwk_device_list_maintenance_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkDeviceListMaintenanceReq( connection, (NwkDeviceListMaintenanceReq *)pNativeMsg );
        nwk_device_list_maintenance_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case NWK_MGR_CMD_ID_T__NWK_REMOVE_DEVICE_REQ:
      pNativeMsg = nwk_remove_device_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkRemoveDeviceReq( connection, (NwkRemoveDeviceReq *)pNativeMsg );
        nwk_remove_device_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_REQ:
      pNativeMsg = nwk_set_binding_entry_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkSetBindingEntryReq( connection, (NwkSetBindingEntryReq *)pNativeMsg );
        nwk_set_binding_entry_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;
	case NWK_MGR_CMD_ID_T__NWK_GET_BINDING_TABLE_REQ:
	  pNativeMsg = nwk_get_binding_table_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        processNwkGetBindingTableReq( connection, (NwkGetBindingTableReq *)pNativeMsg );
        nwk_get_binding_table_req__free_unpacked( pNativeMsg, NULL ); 
      }
	  break;
    default:
      uiPrintfEx(trDEBUG, "Network Manager Server Received Unsupported App Command %d\n",cmdId );
      break;
  }
}

/*********************************************************************
 * @fn      nmHandleServerPbCb
 *
 * @brief   Handles incoming Server Protobuf messages 
 *          (from GW/OTA Servers to NwkManager)
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
void nmHandleServerPbCb( int connection, uint8 subSys, uint8 cmdId, uint16 len, 
                         uint8 *pData, uint8 type )
{
  void *pNativeMsg;
  
  switch ( cmdId )
  {
    case SRVR_CMD_ID_T__SRVR_GET_IEEE_ADDRESS_REQ:
      pNativeMsg = srvr_get_ieee_address_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        nmPb_processSrvrGetIeeeAddressReq( connection, (SrvrGetIeeeAddressReq *)pNativeMsg );
        srvr_get_ieee_address_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;
      
    case SRVR_CMD_ID_T__SRVR_GET_SHORT_ADDRESS_REQ:
      pNativeMsg = srvr_get_short_address_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        nmPb_processSrvrGetShortAddressReq( connection, (SrvrGetShortAddressReq *)pNativeMsg );
        srvr_get_short_address_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    case SRVR_CMD_ID_T__SRVR_GET_DEVICE_INFO_REQ:
      pNativeMsg = srvr_get_device_info_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        nmPb_processSrvrGetDeviceInfoReq( connection, (SrvrGetDeviceInfoReq *)pNativeMsg );
        srvr_get_device_info_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;
      
    case SRVR_CMD_ID_T__SRVR_GET_DEVICE_STATUS_REQ:
      pNativeMsg = srvr_get_device_status_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        nmPb_processSrvrGetDeviceStatusReq( connection, (SrvrGetDeviceStatusReq *)pNativeMsg );
        srvr_get_device_status_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;
      
    case SRVR_CMD_ID_T__SRVR_SET_DEVICE_STATUS_REQ:
      pNativeMsg = srvr_set_device_status_req__unpack( NULL, len, pData );
      if ( pNativeMsg )
      {
        nmPb_processSrvrSetDeviceStatusReq( connection, (SrvrSetDeviceStatusReq *)pNativeMsg );
        srvr_set_device_status_req__free_unpacked( pNativeMsg, NULL ); 
      }
      break;

    default:
      uiPrintfEx(trDEBUG, "Network Manager Server Received Unsupported Gateway/OTA Server Command %d\n",
                cmdId );
      break;
  }
}

/*********************************************************************
 * @fn      processZigBeeSystemResetReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pNwkInfoReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkZigbeeSystemResetReq( int connection, NwkZigbeeSystemResetReq *pSystemResetReq )
{
  SysResetReq resetReq = SYS_RESET_REQ__INIT;
  
  uiPrintf("NWK_ZIGBEE_SYSTEM_RESET: ResetMode %d\n", pSystemResetReq->mode );
  
  if ( pSystemResetReq->mode == NWK_RESET_MODE_T__HARD_RESET )
  {
    resetReq.newnwkstate = TRUE;
    
    uiPrintfEx(trINFO, "Clearing database...\n" );
    nwkMgrDatabaseReset(); // shut-down and clear the database entries
    
  }
  else if ( pSystemResetReq->mode == NWK_RESET_MODE_T__SOFT_RESET )
  {
    resetReq.newnwkstate = FALSE;
  }
  
  // reset ZStack layer
  resetReq.type = RESET_TYPES__DEVICE;
  sendSysResetReq( &resetReq );

  if ( pSystemResetReq->mode == NWK_RESET_MODE_T__HARD_RESET )
  {
   exit(NWKMGRSRVR_EXITCODE_HARDRESET);
  }
  else if ( pSystemResetReq->mode == NWK_RESET_MODE_T__SOFT_RESET )
  {
   exit(NWKMGRSRVR_EXITCODE_SOFTRESET);
  }
}

/*********************************************************************
 * @fn      processNwkZigBeeSystemSelfShutdownReq
 *
 * @brief   Called to shut down the ZigBee system. Shuts down the ZigBee
 *          and Gateway managers.
 *
 * @param   connection - connection handle (tcp)
 *
 * @return  none
 */
static void processNwkZigBeeSystemSelfShutdownReq( void )
{
  SysResetReq resetReq = SYS_RESET_REQ__INIT;
  int counter = 5;
  ZStatusValues returnCode = ZSuccess;
  
  uiPrintf( "NWK_ZIGBEE_SYSTEM_SELF_SHUTDOWN\n" );

  resetReq.has_shutdown = TRUE;
  resetReq.shutdown = TRUE;
    
  // reset ZStack layer
  resetReq.type = RESET_TYPES__DEVICE;
  
  while (counter--)
  {
     returnCode = sendSysResetReq(&resetReq);
    if (ZSuccess == returnCode)
      break;
    usleep(1000000);
  }

  if (ZSuccess != returnCode)
  {
    uiPrintfEx(trERROR, "processNwkZigBeeSystemSelfShutdownReq> Error "
    "returned from call to reset ZNP device. Proceeding to exit\n"); 
  } 
  exit(NWKMGRSRVR_EXITCODE_SELFSHUTDOWN);
}

/*********************************************************************
 * @fn      NwkSetZigbeePowerModeReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetZigBeePowerModeReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkSetZigbeePowerModeReq( int connection, NwkSetZigbeePowerModeReq *pSetZigBeePowerModeReq )
{
  ZStatusValues returnCode = ZSuccess;
  uiPrintf( "NWK_SET_ZIGBEE_POWER_MODE_REQ: PowerMode %d\n", pSetZigBeePowerModeReq->powermode );
  
  // the confirm is sent once the NwkMgr is restarted and one or more Apps connect
  if(pSetZigBeePowerModeReq->powermode == NWK_POWER_MODE_T__WAKEUP)
  {
    exit ( NWKMGRSRVR_EXITCODE_WAKEUP );
  }
  else
  {
    SysResetReq resetReq = SYS_RESET_REQ__INIT;
    resetReq.newnwkstate = FALSE;
    resetReq.has_shutdown = TRUE;
    resetReq.shutdown = TRUE;

    // reset ZStack layer
    resetReq.type = RESET_TYPES__DEVICE;
    returnCode = sendSysResetReq( &resetReq );
    if (ZSuccess == returnCode)
    {
      exit ( NWKMGRSRVR_EXITCODE_SLEEP );
    }
    else 
    {
      NwkSetZigbeePowerModeCnf powerModeCnf = 
        NWK_SET_ZIGBEE_POWER_MODE_CNF__INIT;

      powerModeCnf.status = NWK_STATUS_T__STATUS_FAILURE;
      powerModeCnf.powermode = NWK_POWER_MODE_T__SLEEP; 
      sendNwkSetZigbeePowerModeCnf( connection, &powerModeCnf );
    }

  }
}

/*********************************************************************
 * @fn      processNwkGetLocalDeviceInfoReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 *
 * @return  none
 */
static void processNwkGetLocalDeviceInfoReq( int connection )
{
  uint8 *pBuf;
  int i;
  int len;
  int clusterListCount = 0;
  uint32 *pClusterLists;
  NwkGetLocalDeviceInfoCnf getLocalDeviceInfoCnf = NWK_GET_LOCAL_DEVICE_INFO_CNF__INIT;
  NwkDeviceInfoT deviceInfo = NWK_DEVICE_INFO_T__INIT;
  NwkSimpleDescriptorT **ppSimpleDesc;
  NwkSimpleDescriptorT *pSimpleDesc;
  
  uiPrintf( "NWK_GET_LOCAL_DEVICE_INFO_REQ:\n" );
  
  getLocalDeviceInfoCnf.deviceinfolist = &deviceInfo;

  // Get local device information, including simple descriptor
  deviceInfo.networkaddress = gLocalDeviceInfo.nwkaddr;
  deviceInfo.ieeeaddress = gLocalDeviceInfo.ieeeaddr;

  deviceInfo.manufacturerid = 7;    // Texas Instruments, from 05-3874
  deviceInfo.n_simpledesclist = pgSrvEndpointDefs->endpointCount;
  
  // Count number of input/output clusters
  for ( i = 0; i < pgSrvEndpointDefs->endpointCount; i++ )
  {
    clusterListCount += (pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_inputclusters +
                         pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_outputclusters);
  }
  
  ppSimpleDesc = malloc ( sizeof( NwkSimpleDescriptorT * ) * pgSrvEndpointDefs->endpointCount );
  pSimpleDesc = malloc ( sizeof( NwkSimpleDescriptorT ) * pgSrvEndpointDefs->endpointCount );
  pClusterLists = malloc( sizeof( uint32 ) * clusterListCount );
  
  if ( !pSimpleDesc || !ppSimpleDesc || !pClusterLists )
  {
    uiPrintf( "NWK_GET_LOCAL_DEVICE_INFO_REQ: Memory Error\n" );
    return;
  }
  
  deviceInfo.simpledesclist = ppSimpleDesc;
  
  for ( i = 0; i < pgSrvEndpointDefs->endpointCount; i++ )
  {
    ppSimpleDesc[i] = pSimpleDesc;
    nwk_simple_descriptor_t__init( pSimpleDesc );

    pSimpleDesc->endpointid = pgSrvEndpointDefs->ppEndpoints[i]->endpoint;
    pSimpleDesc->profileid = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->profileid;
    pSimpleDesc->deviceid = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->deviceid;
    pSimpleDesc->devicever = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->devicever;
    pSimpleDesc->n_inputclusters = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_inputclusters;
    pSimpleDesc->inputclusters = pClusterLists;
    
    memcpy( pClusterLists, pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->inputclusters,
            (pSimpleDesc->n_inputclusters * sizeof( uint32 )) ); 
    pClusterLists += pSimpleDesc->n_inputclusters;                           
    
    pSimpleDesc->n_outputclusters = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_outputclusters;
    pSimpleDesc->outputclusters = pClusterLists;
  
    memcpy( pClusterLists, pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->outputclusters,
            (pSimpleDesc->n_outputclusters * sizeof( uint32 )) ); 
    pClusterLists += pSimpleDesc->n_outputclusters;  
    
    pSimpleDesc++;            
  }
  
  deviceInfo.devicestatus = NWK_DEVICE_STATUS_T__DEVICE_ON_LINE; //the gateway self device status is always reported as "online". The actual network status is reported in NWK_ZIGBEE_NWK_INFO_CNF
  
  len = nwk_get_local_device_info_cnf__get_packed_size( &getLocalDeviceInfoCnf );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_get_local_device_info_cnf__pack( &getLocalDeviceInfoCnf, pBuf );

    // Send response back to app (synchronous)
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, FALSE, 
                   NWK_MGR_CMD_ID_T__NWK_GET_LOCAL_DEVICE_INFO_CNF, len, pBuf );
    
    free( pBuf );
  }
  
  // Free memory
  free( ppSimpleDesc[0]->inputclusters );
  free( ppSimpleDesc[0] );
  free( ppSimpleDesc );
}

/*********************************************************************
 * @fn      processNwkGetGwEndpointInfoReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param 	pGwEndpointInfoReq - pointer to structure message
 *
 * @return  none
 */
//static void processNwkGetGwEndpointInfoReq( int connection, NwkGetGwEndpointInfoReq *pGwEndpointInfoReq )
static void processNwkGetGwEndpointInfoReq( int connection )
{
  uint8 *pBuf;
  int i;
  int len;
  int clusterListCount = 0;
  uint32 *pClusterLists;
  
  NwkGetGwEndpointInfoCnf getGwEndpointInfoCnf = NWK_GET_GW_ENDPOINT_INFO_CNF__INIT;
  NwkDeviceInfoT deviceInfo = NWK_DEVICE_INFO_T__INIT;
  NwkSimpleDescriptorT **ppSimpleDesc;
  NwkSimpleDescriptorT *pSimpleDesc;
  
  uiPrintf( "NWK_GET_GW_ENDPOINT_INFO_REQ:\n" );
  
  getGwEndpointInfoCnf.deviceinfolist = &deviceInfo;
  //getGwEndpointInfoCnf.endpoint = pGwEndpointInfoReq->endpoint;

  // Get local device information, including simple descriptor
  deviceInfo.networkaddress = gLocalDeviceInfo.nwkaddr;
  deviceInfo.ieeeaddress = gLocalDeviceInfo.ieeeaddr;

  deviceInfo.manufacturerid = 7;    // Texas Instruments, from 05-3874
  deviceInfo.n_simpledesclist = pgSrvEndpointDefs->endpointCount;
  
  // Count number of input/output clusters
  for ( i = 0; i < pgSrvEndpointDefs->endpointCount; i++ )
  {
    clusterListCount += (pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_inputclusters +
                         pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_outputclusters);
  }
  
  ppSimpleDesc = malloc ( sizeof( NwkSimpleDescriptorT * ) * pgSrvEndpointDefs->endpointCount );
  pSimpleDesc = malloc ( sizeof( NwkSimpleDescriptorT ) * pgSrvEndpointDefs->endpointCount );
  pClusterLists = malloc( sizeof( uint32 ) * clusterListCount );
  
  if ( !pSimpleDesc || !ppSimpleDesc || !pClusterLists )
  {
    uiPrintf( "NWK_GET_GW_ENDPOINT_INFO_REQ: Memory Error\n" );
    return;
  }
  
  deviceInfo.simpledesclist = ppSimpleDesc;
  
  for ( i = 0; i < pgSrvEndpointDefs->endpointCount; i++ )
  {
    ppSimpleDesc[i] = pSimpleDesc;
    nwk_simple_descriptor_t__init( pSimpleDesc );

    pSimpleDesc->endpointid = pgSrvEndpointDefs->ppEndpoints[i]->endpoint;
    pSimpleDesc->profileid = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->profileid;
    pSimpleDesc->deviceid = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->deviceid;
    pSimpleDesc->devicever = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->devicever;
    pSimpleDesc->n_inputclusters = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_inputclusters;
    pSimpleDesc->inputclusters = pClusterLists;
    
    memcpy( pClusterLists, pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->inputclusters,
            (pSimpleDesc->n_inputclusters * sizeof( uint32 )) ); 
    pClusterLists += pSimpleDesc->n_inputclusters;                           
    
    pSimpleDesc->n_outputclusters = pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->n_outputclusters;
    pSimpleDesc->outputclusters = pClusterLists;
  
    memcpy( pClusterLists, pgSrvEndpointDefs->ppEndpoints[i]->simpledesc->outputclusters,
            (pSimpleDesc->n_outputclusters * sizeof( uint32 )) ); 
    pClusterLists += pSimpleDesc->n_outputclusters;  
    
    pSimpleDesc++;            
  }
  
  deviceInfo.devicestatus = NWK_DEVICE_STATUS_T__DEVICE_ON_LINE; //the gateway self device status is always reported as "online". The actual network status is reported in NWK_ZIGBEE_NWK_INFO_CNF
  
  len = nwk_get_gw_endpoint_info_cnf__get_packed_size( &getGwEndpointInfoCnf );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_get_gw_endpoint_info_cnf__pack( &getGwEndpointInfoCnf, pBuf );

    // Send response back to app (synchronous)
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, FALSE, 
                   NWK_MGR_CMD_ID_T__NWK_GET_GW_ENDPOINT_INFO_CNF, len, pBuf );
    
    free( pBuf );
  }
  
  // Free memory
  free( ppSimpleDesc[0]->inputclusters );
  free( ppSimpleDesc[0] );
  free( ppSimpleDesc );
}

/*********************************************************************
 * @fn      processNwkStartCommissioningReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 *
 * @return  none
 */
static void processNwkStartCommissioningReq( int connection, NwkStartCommissioningReq *pStartCommissioningReq )
{
  ZStatusValues status = ZSTATUS_VALUES__ZDecodeError;
  AppCnfStartCommissioningReq startCommissioningReq = APP_CNF_START_COMMISSIONING_REQ__INIT;
	
  uiPrintf( "Processing Start Commissioning.\n" );

  startCommissioningReq.commissioningmode= pStartCommissioningReq->commmode;
	
  status = sendAppCnfStartCommissioningReq( &startCommissioningReq );
	
  // inform app with a confirm (and perhaps response)
  nmHandleCnfRsp( connection, NM_RSP_NONE, IGNORE_ADDR_TYPE, status );
	
  // some debug code
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
	uiPrintf( "Processing System Reset Request Failed - Status:%d\n", status );
  }

}

/*********************************************************************
 * @fn      processNwkSendSimpleDescriptorReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 *
 * @return  none
 */
static void processNwkSendSimpleDescriptorReq( int connection, NwkSendSimpleDescriptorReq *pSendSimpleDescriptorReq )
{
  ZStatusValues status = ZSTATUS_VALUES__ZDecodeError;
  uint16 sequence = 0;
  uint64_t ieeeAddr = 0;
  NwkAddressStructT srcAddress = NWK_ADDRESS_STRUCT_T__INIT;

  status = sendSimpleDescReq( (uint16)pSendSimpleDescriptorReq->nwkaddr, (uint8)pSendSimpleDescriptorReq->endpoint );

  if (!nwkMgrDb_GetIeeeAddr( (uint16)pSendSimpleDescriptorReq->nwkaddr, &ieeeAddr ))
  {
	status = ZSTATUS_VALUES__ZFailure;
  }
  else
  {
	srcAddress.addresstype = NWK_ADDRESS_TYPE_T__UNICAST;
	srcAddress.has_ieeeaddr = true;
	srcAddress.ieeeaddr = ieeeAddr;
	srcAddress.has_endpointid = true;
	srcAddress.endpointid = pSendSimpleDescriptorReq->endpoint;
  }
	
  // some debug code
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
	uiPrintf( "Processing System Reset Request Failed - Status:%d\n", status );
  }  
  
  sequence = nmHandleCnfRsp( connection, NM_RSP_SPECIFIC, NWK_ADDRESS_TYPE_T__UNICAST, status );

  // if worked, post for the reply
  if( status == ZSTATUS_VALUES__ZSuccess )
  {
    // post, waiting for the response
    
    if( !nmTransPost( connection, NWK_MGR_CMD_ID_T__NWK_SEND_SIMPLE_DESCRIPTOR_RSP_IND, sequence, 
          &srcAddress, ZSTACK_CMD_IDS__ZDO_SIMPLE_DESC_RSP, pSendSimpleDescriptorReq->nwkaddr) )
    {
      status = ZSTATUS_VALUES__ZFailure;
    }
  }  

}

/*********************************************************************
 * @fn      processNwkSetInstallCodeReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 *
 * @return  none
 */
static void processNwkSetInstallCodeReq( int connection, NwkSetInstallCodeReq *pSetInstallCodeReq )
{
  ZStatusValues status = ZSTATUS_VALUES__ZDecodeError;
  AppCnfAddInstallCodeReq addInstallCodeReq = APP_CNF_ADD_INSTALL_CODE_REQ__INIT;
  uint8_t installCodeValue[16];
  int x=0;

  addInstallCodeReq.ieeeaddress = pSetInstallCodeReq->ieeeaddress;
  addInstallCodeReq.crc = pSetInstallCodeReq->crc;

  for (x=0;x<16;x++){
	installCodeValue[x] = (uint8_t)pSetInstallCodeReq->installcode[x];
  }

  addInstallCodeReq.installcode.data = installCodeValue;
  addInstallCodeReq.installcode.len = pSetInstallCodeReq->n_installcode;
  
  status = sendAppCnfAddInstallCodeReq( &addInstallCodeReq );

  // inform app with a confirm (and perhaps response)
  nmHandleCnfRsp( connection, NM_RSP_NONE, IGNORE_ADDR_TYPE, status );

  // some debug code
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
	uiPrintf( "Processing System Reset Request Failed - Status:%d\n", status );
  }

}


/*********************************************************************
 * @fn      processNwkZigbeeNwkInfoReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pNwkInfoReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkZigbeeNwkInfoReq( int connection, NwkZigbeeNwkInfoReq *pNwkInfoReq )
{
  NwkZigbeeNwkInfoCnf nwkInfoCnf = NWK_ZIGBEE_NWK_INFO_CNF__INIT;
  
  uiPrintf( "NWK_ZIGBEE_NWK_INFO_REQ\n" );
  
  // synchronous call, gets info into gLocalDeviceInfo
  
  sendSysNwkInfoReadReqAndSetTxPower();
  
  if ( ( gLocalDeviceInfo.devstate == DEV_STATE__DEV_ROUTER ) || (gLocalDeviceInfo.devstate == DEV_STATE__DEV_ZB_COORD ) )
  {
    nwkInfoCnf.status = NWK_NETWORK_STATUS_T__NWK_UP;
  }
  else
  {
    nwkInfoCnf.status = NWK_NETWORK_STATUS_T__NWK_DOWN;
  }
  
  nwkInfoCnf.nwkchannel = gLocalDeviceInfo.logicalchannel;
  nwkInfoCnf.panid = gLocalDeviceInfo.panid;
  nwkInfoCnf.extpanid = gLocalDeviceInfo.extendedpanid;
  
  sendNwkZigbeeNwkInfoCnf( connection, &nwkInfoCnf );
}

/*********************************************************************
 * @fn      processNwkSetPermitJoinReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pSetPermitJoinReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkSetPermitJoinReq( int connection, NwkSetPermitJoinReq *pSetPermitJoinReq )
{
  ZStatusValues status = ZSTATUS_VALUES__ZDecodeError;
  ZdoMgmtPermitJoinReq mgmtPermitJoinReq = ZDO_MGMT_PERMIT_JOIN_REQ__INIT;
  
  uiPrintf( "Processing Set Permit Join Request.\n" );
  
  mgmtPermitJoinReq.duration = pSetPermitJoinReq->permitjointime;
  mgmtPermitJoinReq.tcsignificance = TRUE;
  
  mgmtPermitJoinReq.nwkaddr = 0xFFFC; // broadcast
  
  if ( pSetPermitJoinReq->permitjoin == NWK_PERMIT_JOIN_TYPE_T__PERMIT_LOCAL )
  {    
    mgmtPermitJoinReq.nwkaddr = gLocalDeviceInfo.nwkaddr; // self-address
    
    status = sendZdoMgmtPermitJoinReq( &mgmtPermitJoinReq );
  }
  else
  {
    status = sendZdoMgmtPermitJoinReq( &mgmtPermitJoinReq );
  }
  
  // inform app with a confirm (and perhaps response)
  nmHandleCnfRsp( connection, NM_RSP_NONE, IGNORE_ADDR_TYPE, status );

  // some debug code
  if ( status != ZSTATUS_VALUES__ZSuccess )
  {
    uiPrintf( "Processing System Reset Request Failed - Status:%d\n", status );
  }
}

/*********************************************************************
 * @fn      processNwkManagePeriodicMtoRouteReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pManagePeriodicMtoRouteReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkManagePeriodicMtoRouteReq( int connection, NwkManagePeriodicMtoRouteReq *pManagePeriodicMtoRouteReq)
{
  DevNwkRouteReq routereq = DEV_NWK_ROUTE_REQ__INIT;
  uiPrintf( "NWK_MANAGE_PERIODIC_MTO_ROUTE_REQ: mode %d\n", pManagePeriodicMtoRouteReq-> mode );

  routereq.dstaddr = 0;   // ignored by ZStack
  routereq.mtoroute = (pManagePeriodicMtoRouteReq->mode == NWK_MTO_ROUTE_MODE_T__MTO_ROUTE_START) ? TRUE : FALSE;
  routereq.has_mtonocache = TRUE;
  routereq.mtonocache = FALSE;
  routereq.has_multicast = FALSE;
  routereq.radius = NWK_ROUTE_RADIUS;

  nmHandleCnfRsp( connection, NM_RSP_NONE, IGNORE_ADDR_TYPE, NWK_STATUS_T__STATUS_SUCCESS );
  
  sendNwkRouteReq( &routereq );
}

/*********************************************************************
 * @fn      processNwkGetNeighborTableReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetNeighborTableReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkGetNeighborTableReq( int connection, NwkGetNeighborTableReq *pGetNeighborTableReq)
{
  uint16 nwkaddr;
  uint8 status = NWK_STATUS_T__STATUS_SUCCESS;
  uint16 sequence;

  uiPrintf( "NWK_GET_NEIGHBOR_TABLE_REQ: startindex %d ", pGetNeighborTableReq->startindex );

  // local device
  if( pGetNeighborTableReq->dstaddr->addresstype == NWK_ADDRESS_TYPE_T__SELF )
  {
    // SysNwkInfoReadRsp 
    uiPrintf( "0x%016llX (Self)\n\n", gLocalDeviceInfo.ieeeaddr );
    nwkaddr = gLocalDeviceInfo.nwkaddr;
  }

  // remote device
  else
  {
    if( !pGetNeighborTableReq->dstaddr->has_ieeeaddr || 
      !nwkMgrDb_GetShortAddr( pGetNeighborTableReq->dstaddr->ieeeaddr, &nwkaddr ) )
    {
      status = NWK_STATUS_T__STATUS_FAILURE;
    }
    else
    {
      uiPrintf( "ieeeaddr 0x%016llX\n\n", pGetNeighborTableReq->dstaddr->ieeeaddr );
    }
  }

  // OK to send, we have the short address
  if( status == NWK_STATUS_T__STATUS_SUCCESS)
  {
    status = sendZdoMgmtLqiReq( nwkaddr, pGetNeighborTableReq->startindex );
  }

  // send confirm
  sequence = nmHandleCnfRsp( connection, NM_RSP_SPECIFIC, NWK_ADDRESS_TYPE_T__UNICAST, status );

  // if worked, post for the reply
  if( status == NWK_STATUS_T__STATUS_SUCCESS )
  {
    // post, waiting for the response
    if( !nmTransPost( connection, NWK_MGR_CMD_ID_T__NWK_GET_NEIGHBOR_TABLE_RSP_IND, sequence, 
          pGetNeighborTableReq->dstaddr, ZSTACK_CMD_IDS__ZDO_MGMT_LQI_RSP, nwkaddr ) )
    {
      status = NWK_STATUS_T__STATUS_FAILURE;
    }
  }
  else
  {
    uiPrintf( "failed status %d\n", status );
  }
}

/*********************************************************************
 * @fn      processNwkGetRoutingTableReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetRoutingTableReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkGetRoutingTableReq( int connection, NwkGetRoutingTableReq *pGetRoutingTableReq)
{
  uint16 nwkaddr;
  uint8 status = NWK_STATUS_T__STATUS_SUCCESS;
  uint16 sequence;

  uiPrintf( "NWK_GET_ROUTING_TABLE_REQ: startindex %d, ", pGetRoutingTableReq->startindex );

  // local device
  if( pGetRoutingTableReq->dstaddr->addresstype == NWK_ADDRESS_TYPE_T__SELF )
  {
    // SysNwkInfoReadRsp 
    uiPrintf( "ieeeaddr 0x%016llX (Self)\n\n", gLocalDeviceInfo.ieeeaddr );
    nwkaddr = gLocalDeviceInfo.nwkaddr;
  }

  // remote device
  else
  {
    if( !pGetRoutingTableReq->dstaddr->has_ieeeaddr || 
      !nwkMgrDb_GetShortAddr( pGetRoutingTableReq->dstaddr->ieeeaddr, &nwkaddr ) )
    {
      status = NWK_STATUS_T__STATUS_FAILURE;
    }
    else
    {
      uiPrintf( "ieeeaddr 0x%016llX\n\n", pGetRoutingTableReq->dstaddr->ieeeaddr );
    }
  }

  // OK to send, we have the short address
  if( status == NWK_STATUS_T__STATUS_SUCCESS)
  {
    status = sendZdoMgmtRtgReq( nwkaddr, pGetRoutingTableReq->startindex );
  }

  // send confirm
  sequence = nmHandleCnfRsp( connection, NM_RSP_SPECIFIC, NWK_ADDRESS_TYPE_T__UNICAST, status );

  // if worked, post for the reply
  if( status == NWK_STATUS_T__STATUS_SUCCESS )
  {
    // post, waiting for the response
    if( !nmTransPost( connection, NWK_MGR_CMD_ID_T__NWK_GET_ROUTING_TABLE_RSP_IND, sequence, 
          pGetRoutingTableReq->dstaddr, ZSTACK_CMD_IDS__ZDO_MGMT_RTG_RSP, nwkaddr ) )
    {
      status = NWK_STATUS_T__STATUS_FAILURE;
    }
  }
  else
  {
    uiPrintf( "failed status %d\n", status );
  }
}

/*********************************************************************
 * @fn      processNwkChangeNwkKeyReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetRoutingTableReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkChangeNwkKeyReq( int connection, NwkChangeNwkKeyReq *pChangeNwkKeyReq)
{
  uint8 status = NWK_STATUS_T__STATUS_SUCCESS;
  SecNwkKeySetReq    setKeyReq = SEC_NWK_KEY_SET_REQ__INIT;
  SecNwkKeyUpdateReq updateKeyReq = SEC_NWK_KEY_UPDATE_REQ__INIT;

  uiPrintf( "NWK_CHANGE_NWK_KEY_REQ: " );
  
  // create a random key
  if(pChangeNwkKeyReq->has_newkey)
  {
    uiPrintf("key ");
    nmUiPrintData( &pChangeNwkKeyReq->newkey );
  }
  else
  {
    uiPrintf("random key ");
  }
  uiPrintf("\n");
  
  // choose a new sequence #
  ++gNmKeySeqNum;
  
  // set the new key (the spare, not the active, we'll switch later)
  setKeyReq.activekey = FALSE;
  setKeyReq.seqnum = gNmKeySeqNum;
  setKeyReq.has_key = pChangeNwkKeyReq->has_newkey; // if no key, then random
  if(setKeyReq.has_key)
  {
    setKeyReq.key.len = pChangeNwkKeyReq->newkey.len;
    setKeyReq.key.data = pChangeNwkKeyReq->newkey.data;
  }
  status = sendSecNwkKeySetReq( &setKeyReq );

  // send generic confirm (no sequence)
  nmHandleCnfRsp( connection, NM_RSP_GENERIC, IGNORE_ADDR_TYPE, status );

  // send the transport key
  updateKeyReq.seqnum = gNmKeySeqNum;
  updateKeyReq.dstaddr = 0xffff;
  sendSecNwkKeyUpdateReq( &updateKeyReq );
  
  // wait to send the switch key
  gNmKeySwitchTimeout = SECURITY_TIMEOUT;
}

/*********************************************************************
 * @fn      processNwkGetRoutingTableReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetRoutingTableReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkGetNwkKeyReq( int connection, NwkGetNwkKeyReq *pGetNwkKeyReq)
{
  SecNwkKeyGetReq getKeyReq = SEC_NWK_KEY_GET_REQ__INIT;  // request to ZStack
  SecNwkKeyGetRsp *pGetKeyRsp;
  NwkGetNwkKeyCnf getNwkKeyCnf = NWK_GET_NWK_KEY_CNF__INIT; // Cnf to APP
  int len;
  uint8 *pBuf;
  uint8 rspcmdid;
  uint8 *pRsp;
  uint8 *pData;
  uint16 rsplen;

  uiPrintf( "NWK_GET_NWK_KEY_REQ:\n" );
  
  getKeyReq.activekey = TRUE;
  
  len = sec_nwk_key_get_req__get_packed_size( &getKeyReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    sec_nwk_key_get_req__pack( &getKeyReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    pRsp = apicSendSynchData( NM_API_CLIENT, ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF, 
                              ZSTACK_CMD_IDS__SEC_NWK_KEY_GET_REQ, len, pBuf,
                              NULL, &rspcmdid, &rsplen );

    // if we got a response
    if ( pRsp )
    {
      // unpack ZStack response, and send confirm to APP
      if ( (ZSTACK_CMD_IDS__SEC_NWK_KEY_GET_RSP == rspcmdid) && (rsplen > 0) )
      {
        pGetKeyRsp = sec_nwk_key_get_rsp__unpack( NULL, rsplen, pRsp );
        if ( pGetKeyRsp )
        {
          // convert from ZStack response to APP confirm
          getNwkKeyCnf.status = pGetKeyRsp->status;
          getNwkKeyCnf.newkey.len = pGetKeyRsp->key.len;
          if ( getNwkKeyCnf.newkey.len != 0 )
          {
            // Allocate memory for incoming data
            pData = malloc( getNwkKeyCnf.newkey.len );
            if ( !pData )
            {
              sec_nwk_key_get_rsp__free_unpacked( pGetKeyRsp, NULL );
              apicFreeSynchData( pRsp );
              free( pBuf );

              return; // memory error  
            }

            getNwkKeyCnf.newkey.data = pData;
            memcpy( getNwkKeyCnf.newkey.data, pGetKeyRsp->key.data, getNwkKeyCnf.newkey.len );
          }

          sec_nwk_key_get_rsp__free_unpacked( pGetKeyRsp, NULL );

          // remember the key sequence #
          if( !gNmKeySeqNum )
            gNmKeySeqNum = pGetKeyRsp->seqnum;

          uiPrintf( "active %d, keyseq %d, key: ", pGetKeyRsp->activekey, pGetKeyRsp->seqnum );
          nmUiPrintData( &getNwkKeyCnf.newkey );
          
          // send response 
          sendNwkGetNwkKeyCnf( connection, &getNwkKeyCnf );

          // free data buffer memory
          if ( getNwkKeyCnf.newkey.len != 0 )
          {
            free( pData );
          }
        }
      }
    
      apicFreeSynchData( pRsp );
    }
    
    free( pBuf );
  }
}

/*********************************************************************
 * @fn      processNwkGetDeviceListReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pGetDeviceListReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkGetDeviceListReq( int connection, NwkGetDeviceListReq *pGetDeviceListReq )
{
  int deviceCount = 0;
  NwkDeviceInfoT *pOutDeviceInfo = NULL;       // array of devices
  NwkDeviceInfoT **ppOutDeviceInfoList; // array of pointers to devices
  sNwkMgrDb_DeviceInfo_t *pDeviceInfo = NULL;
  sNwkMgrDb_DeviceList_t *pDeviceList = NULL;
  NwkGetDeviceListCnf getDeviceListCnf = NWK_GET_DEVICE_LIST_CNF__INIT;

  uiPrintf( "Processing Get Device List Request.\n" );

  if ( pGetDeviceListReq->dstaddr )
  {
    pDeviceInfo = nwkMgrDb_GetDeviceInfo( pGetDeviceListReq->dstaddr->ieeeaddr ); 
    
    pDeviceList = NULL;
  }
  else
  { 
    pDeviceList = nwkMgrDb_GetAllDevices();
    
    pDeviceInfo = NULL;
  }

  if ( pDeviceInfo )
  {
    // Single device
    deviceCount = 1;
    
    getDeviceListCnf.n_devicelist = deviceCount;
    
    ppOutDeviceInfoList = malloc ( sizeof( NwkDeviceInfoT * ) );
    if ( ppOutDeviceInfoList )
    {
      pOutDeviceInfo = allocPbDeviceInfoList( 1, pDeviceInfo );

      getDeviceListCnf.devicelist = ppOutDeviceInfoList;
      ppOutDeviceInfoList[0] = pOutDeviceInfo;
      
      if ( pOutDeviceInfo )
      {
        getDeviceListCnf.status = NWK_STATUS_T__STATUS_SUCCESS;
      }
      else
      {
        getDeviceListCnf.status = NWK_STATUS_T__STATUS_FAILURE;
      }
    }
  }
  else if ( pDeviceList )
  {
    // All devices in list
    deviceCount = pDeviceList->count;
    
    getDeviceListCnf.n_devicelist = deviceCount;

    ppOutDeviceInfoList = malloc( deviceCount * sizeof( NwkDeviceInfoT * ) );
    if ( ppOutDeviceInfoList )
    {
      int i;
      
      pOutDeviceInfo = allocPbDeviceInfoList( getDeviceListCnf.n_devicelist, pDeviceList->pDevices );
      
      getDeviceListCnf.devicelist = ppOutDeviceInfoList;
      
      for ( i = 0; i < deviceCount; i++ )
      {
        // Filling in pointer list
        ppOutDeviceInfoList[i] = &pOutDeviceInfo[i];
      }
      
      if ((pOutDeviceInfo == NULL) && (deviceCount != 0))
      {
        getDeviceListCnf.status = NWK_STATUS_T__STATUS_FAILURE;
      }
      else
      {
        getDeviceListCnf.status = NWK_STATUS_T__STATUS_SUCCESS;
      }
    }
  }
  else
  {
    // No device(s) found
    getDeviceListCnf.status = NWK_STATUS_T__STATUS_FAILURE;
    getDeviceListCnf.n_devicelist = 0;
  }
    
  sendNwkGetDeviceListCnf( connection, &getDeviceListCnf );

  // Free device info list for Protobufs
  if ( deviceCount )
  {
    if ( pOutDeviceInfo )
    {
      freePbDeviceInfoList( deviceCount, pOutDeviceInfo );
    }
    
    if ( pDeviceInfo )
    {
      nwkMgrDb_FreeDeviceInfo( pDeviceInfo );
    }
    
    if ( pDeviceList )
    {
      nwkMgrDb_FreeAllDevices( pDeviceList );
    }
    
    if ( ppOutDeviceInfoList )
    {
      free ( ppOutDeviceInfoList );
    }
  }
}

/*********************************************************************
 * @fn      processNwkDeviceListMaintenanceReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pDeviceListMaintenanceReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkDeviceListMaintenanceReq( int connection, NwkDeviceListMaintenanceReq *pDeviceListMaintenanceReq )
{
  NwkStatusT status = NWK_STATUS_T__STATUS_SUCCESS;
  uint16 shortaddr;
  int64_t ieeeaddr = 0;
  int count;

  uiPrintf( "NWK_DEVICE_LIST_MAINTENANCE_REQ " );
  if( pDeviceListMaintenanceReq->dstaddr && pDeviceListMaintenanceReq->dstaddr->has_ieeeaddr )
  {
    uiPrintf(" on %016llX\n", pDeviceListMaintenanceReq->dstaddr->ieeeaddr );
  }
  else
  {
    uiPrintf(" on all\n" );
  }

  // only 1 Maintanance Req can be oustanding at any given time
  if(gpnmDeviceList)
    nwkMgrDb_FreeAllDevices( gpnmDeviceList );

  if( pDeviceListMaintenanceReq->dstaddr && pDeviceListMaintenanceReq->dstaddr->has_ieeeaddr )
  {
    // must be unicast and must exist in the database
    if ( !nwkMgrDb_GetShortAddr( pDeviceListMaintenanceReq->dstaddr->ieeeaddr, &shortaddr ) )
    {
      status = NWK_STATUS_T__STATUS_INVALID_PARAMETER;
    }
    
    else
    {
      ieeeaddr = pDeviceListMaintenanceReq->dstaddr->ieeeaddr;
    }
    count = 1;
  }

  // start up state machine to get all parameters
  else
  {
    // get the device list
    gpnmDeviceList = nwkMgrDb_GetAllDevices();
    if(!gpnmDeviceList)
    {
      status = NWK_STATUS_T__STATUS_FAILURE;
    }
    else
    {
      gnmDeviceIndex = 0;
      count = gpnmDeviceList->count;
      if( !gpnmDeviceList->count )
      {
        nwkMgrDb_FreeAllDevices( gpnmDeviceList );    // no devices
        gpnmDeviceList = NULL;
      }
      else
      {
        ieeeaddr = gpnmDeviceList->pDevices[0].ieeeAddr;  // start on 1st device
      }
    }
  }

  // send generic confirm
  nmHandleCnfRsp( connection, NM_RSP_NONE, IGNORE_ADDR_TYPE, status );

  // start the state machine
  if( count && status == NWK_STATUS_T__STATUS_SUCCESS )
  {
    zNwkSrv_AddDevice( ieeeaddr, NULL, 0, nwkSrvAddDeviceMaintenanceReqCB );
  }
}

/**************************************************************************************************
 *
 * @fn          nwkSrvAddDeviceMaintenanceReqCB
 *
 * @brief       An Add Device state machine has completed. Process it, perhaps starting up a
 *              new one.
 *
 * @param       appSeq - application sequence number
 * @param       pDeviceInfo - passed device information pointer
 * @param       err - state machine error codes
 *
 * @return      none
 *
 **************************************************************************************************/
void nwkSrvAddDeviceMaintenanceReqCB( uint16 appSeq, sNwkMgrDb_DeviceInfo_t *pDeviceInfo, int err )
{
  NwkZigbeeDeviceInd deviceInd = NWK_ZIGBEE_DEVICE_IND__INIT;
  NwkDeviceInfoT *pPbDeviceInfo;    // protobuf DevInfo
  sNwkMgrDb_DeviceInfo_t *pNewDeviceInfo;
  bool worked;
  uint8 devStatus;
  
  // worked, indicate on-line. If nothing has changed, don't send indication
  if ( err == NS_ADS_OK )
  {
    uiPrintfEx(trDEBUG, "- AddDeviceMaintenanceReqCB complete on 0x%016llX", pDeviceInfo->ieeeAddr );
    pDeviceInfo->status = devInfoFlags_OnLine_c;
  }

  // failed, indicate off-line if changed
  else
  {
    if( pDeviceInfo )
    {
      uiPrintfEx(trDEBUG, "- AddDeviceMaintenanceReqCB failed %d on 0x%016llX", err, pDeviceInfo->ieeeAddr );
      pDeviceInfo->status = devInfoFlags_OffLine_c;
    }
    else
    {
      uiPrintfEx(trDEBUG, "- AddDeviceMaintenanceReqCB failed %d\n", err );
    }
  }

  // get the current status. If changed, update the database.
  if( pDeviceInfo )
  {
    worked = nwkMgrDb_GetDeviceStatus( pDeviceInfo->ieeeAddr, &devStatus );
    if( worked && pDeviceInfo->status != devStatus )
    {
      uiPrintfEx(trDEBUG, " - changed, sending DeviceInd\n" );

      // sets the status of a device in the database
      nwkMgrDb_SetDeviceStatus( pDeviceInfo->ieeeAddr, pDeviceInfo->status );

      // send DeviceInd after converting pDeviceInfo to DeviceInd
      pNewDeviceInfo = nwkMgrDb_GetDeviceInfo( pDeviceInfo->ieeeAddr );
      if( pNewDeviceInfo )
      {
        pPbDeviceInfo = allocPbDeviceInfoList( 1, pNewDeviceInfo );
        if ( pPbDeviceInfo )
        {
          deviceInd.deviceinfo = pPbDeviceInfo;
          
          // Send device indication
          sendNwkZigbeeDeviceInd( &deviceInd );
          
          // Free allocated device info
          freePbDeviceInfoList( 1, pPbDeviceInfo );
        }

        nwkMgrDb_FreeDeviceInfo( pNewDeviceInfo );
      }
    }
    else
    {
      uiPrintfEx(trDEBUG, " - no change\n" );
    }
  }

  // if we might have more device, start up a new state machine on next device
  if( gpnmDeviceList )
  {
    // on to next state machine
    ++gnmDeviceIndex;
    if( gnmDeviceIndex < gpnmDeviceList->count )
    {
      zNwkSrv_AddDevice( gpnmDeviceList->pDevices[gnmDeviceIndex].ieeeAddr, NULL, 0, nwkSrvAddDeviceMaintenanceReqCB );
    }

    // done, no more state machines
    else
    {
      nwkMgrDb_FreeAllDevices( gpnmDeviceList );
      gpnmDeviceList = NULL;
      gnmDeviceIndex = 0;
    }
  }
}

/*********************************************************************
 * @fn      processNwkRemoveDeviceReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pRemoveDeviceReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkRemoveDeviceReq( int connection, NwkRemoveDeviceReq *pRemoveDeviceReq )
{
  ZStatusValues sendStatus;
  NwkDeviceInfoT *pPbDeviceInfo; // protobuf device information to send to app
  sNwkMgrDb_DeviceInfo_t *pDbDeviceInfo; // device info pulled from the database
  NwkZigbeeDeviceInd deviceInd = NWK_ZIGBEE_DEVICE_IND__INIT;
  
  uiPrintf( "Processing Remove Device Request.\n" );
  
  // Send remove device request to ZStack server
  sendStatus = sendNwkRemoveDeviceReq( pRemoveDeviceReq );
  
  if ( sendStatus == ZSTATUS_VALUES__ZSuccess )
  {
    // Stop service discovery
    zNwkSrv_AD_HaltStateMachine( pRemoveDeviceReq->dstaddr->ieeeaddr );
    
    // Get device for updated ZigBee device indication
    pDbDeviceInfo = nwkMgrDb_GetDeviceInfo( pRemoveDeviceReq->dstaddr->ieeeaddr );
    
    // Check to see if we got the device information from the database
    if ( pDbDeviceInfo )
    {
      // Convert device information to protobuf form
      pPbDeviceInfo = allocPbDeviceInfoList( 1, pDbDeviceInfo );
      
      if ( pPbDeviceInfo )
      {
        // inform app with a confirm (and perhaps response)
        nmHandleCnfRsp( connection, NM_RSP_NONE, IGNORE_ADDR_TYPE, NWK_STATUS_T__STATUS_SUCCESS );
        
        // Indicate that device was removed
        pPbDeviceInfo->devicestatus = NWK_DEVICE_STATUS_T__DEVICE_REMOVED;
      
        // Send updated device indication to app(s)
        deviceInd.deviceinfo = pPbDeviceInfo;
        sendNwkZigbeeDeviceInd( &deviceInd );
        
        // Free protobuf device info list
        freePbDeviceInfoList( 1, pPbDeviceInfo );
        
        // Remove device from device list
        nwkMgrDb_RemoveDevice( pRemoveDeviceReq->dstaddr->ieeeaddr );
      }
      
      // Free database device info list
      nwkMgrDb_FreeDeviceInfo( pDbDeviceInfo );
    }
  }
  else
  {
    uiPrintf( "Sending Remove Device Request Failed - Status:%d\n", sendStatus );
    
    // inform app with a confirm (and perhaps response)
    nmHandleCnfRsp( connection, NM_RSP_NONE, IGNORE_ADDR_TYPE, NWK_STATUS_T__STATUS_FAILURE );
  }
}

/*********************************************************************
 * @fn      processNwkSetBindingEntryReq
 *
 * @brief   Handles incoming App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pBindingEntryReq - pointer to message structure
 *
 * @return  none
 */
static void processNwkSetBindingEntryReq( int connection, NwkSetBindingEntryReq *pBindingEntryReq )
{
  ZdoBindReq   bindreq = ZDO_BIND_REQ__INIT;      // bind requeest
  ZdoUnbindReq unbindreq = ZDO_UNBIND_REQ__INIT;  // unbind request
  BindRec      bindrec = BIND_REC__INIT;          // bind record
  AFAddr       dstaddr = AFADDR__INIT;            // destination of the binding
  uint16       shortaddr;
  uint16       sequence;
  uint8        status = NWK_STATUS_T__STATUS_SUCCESS;
  uint8        zdoRsp;
  uint8 	   internal = FALSE;

  //GROUP_ADDR_FIX
  // sanity check the binding request (make sure it's unicast with an endpoint as src address)
  if( (pBindingEntryReq->srcaddr->addresstype != NWK_ADDRESS_TYPE_T__UNICAST) ||
      (!pBindingEntryReq->srcaddr->has_ieeeaddr) ||
      (!pBindingEntryReq->srcaddr->has_endpointid) ||
  // sanity check the binding request (make sure it's unicast with an endpoint as dst address if unicast bind)      
      ((pBindingEntryReq->dstaddr->addresstype == NWK_ADDRESS_TYPE_T__UNICAST) && 
      (!pBindingEntryReq->dstaddr->has_ieeeaddr) && 
      (!pBindingEntryReq->dstaddr->has_endpointid))   ||
  // sanity check the binding request (make sure it's unicast with an endpoint as dst address if group address bind)            
    ((pBindingEntryReq->dstaddr->addresstype == NWK_ADDRESS_TYPE_T__GROUPCAST) && 
    (!pBindingEntryReq->dstaddr->has_groupaddr)) )
  {
    status = NWK_STATUS_T__STATUS_FAILURE;
  }
  
  // since we need the short address, we MUST have the entry in the database
  if((status == NWK_STATUS_T__STATUS_SUCCESS) && !nwkMgrDb_GetShortAddr( pBindingEntryReq->srcaddr->ieeeaddr, &shortaddr ) && (pBindingEntryReq->srcaddr->ieeeaddr != gLocalDeviceInfo.ieeeaddr))
  {
	status = NWK_STATUS_T__STATUS_FAILURE;
  }

  if((!pBindingEntryReq->dstaddr->has_groupaddr)&&(pBindingEntryReq->dstaddr->addresstype == NWK_ADDRESS_TYPE_T__GROUPCAST))
  {
    status = NWK_STATUS_T__STATUS_FAILURE;
  }

  // This needs to happen if the bind needs to be created internally on the GW
  if ((status == NWK_STATUS_T__STATUS_SUCCESS) && (pBindingEntryReq->srcaddr->ieeeaddr == gLocalDeviceInfo.ieeeaddr))
  {
  	shortaddr = gLocalDeviceInfo.nwkaddr;
	status = NWK_STATUS_T__STATUS_SUCCESS;
	sequence = nmHandleCnfRsp( connection, NM_RSP_SPECIFIC, NWK_ADDRESS_TYPE_T__UNICAST, status );
	
	if( pBindingEntryReq->bindingmode == NWK_BINDING_MODE_T__BIND )
    {
      zdoRsp = ZSTACK_CMD_IDS__ZDO_BIND_RSP;
    }
	else
    {
      zdoRsp = ZSTACK_CMD_IDS__ZDO_UNBIND_RSP;
    }
	
	nmTransPost( connection, NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_RSP_IND, sequence, pBindingEntryReq->srcaddr, zdoRsp, shortaddr );
	internal = TRUE;
  }

  // only send to ZStack if parameters OK
  if( status == NWK_STATUS_T__STATUS_SUCCESS )
  {
    //GROUP_ADDR_FIX
    //Set group addressing
    if(pBindingEntryReq->dstaddr->addresstype == NWK_ADDRESS_TYPE_T__GROUPCAST)
    {
      // destinaton of the binding/unbinding
      dstaddr.addrmode = AFADDR_MODE__GROUP;
      dstaddr.has_shortaddr = TRUE;
      dstaddr.shortaddr = pBindingEntryReq->dstaddr->groupaddr;
      dstaddr.has_panid = FALSE;
      dstaddr.has_extaddr = FALSE;
      dstaddr.extaddr = 0;
      dstaddr.has_endpoint = FALSE;
      dstaddr.endpoint = 0;
    }
    //Assume it is unicast address type (other types got filter above)
    else 
    {
      // destinaton of the binding/unbinding
      dstaddr.addrmode = AFADDR_MODE__EXT;
      dstaddr.has_shortaddr = FALSE;
      dstaddr.has_panid = FALSE;
      dstaddr.has_extaddr = TRUE;
      dstaddr.extaddr = pBindingEntryReq->dstaddr->ieeeaddr;
      dstaddr.has_endpoint = TRUE;
      dstaddr.endpoint = pBindingEntryReq->dstaddr->endpointid;
    }

    bindrec.srcaddr = pBindingEntryReq->srcaddr->ieeeaddr;
    bindrec.srcendpoint = pBindingEntryReq->srcaddr->endpointid;
    bindrec.clusterid = pBindingEntryReq->clusterid;
    bindrec.dstaddr = &dstaddr;

    // set up ZDP call and send it
    if( pBindingEntryReq->bindingmode == NWK_BINDING_MODE_T__BIND )
    {
      bindreq.nwkaddr = shortaddr;
      bindreq.bindinfo = &bindrec;
      sendZdoBindReq( &bindreq );
      zdoRsp = ZSTACK_CMD_IDS__ZDO_BIND_RSP;
    }
	else
    {
      unbindreq.nwkaddr = shortaddr;
      unbindreq.bindinfo = &bindrec;
      sendZdoUnbindReq( &unbindreq );
      zdoRsp = ZSTACK_CMD_IDS__ZDO_UNBIND_RSP;
    }
  
  }

  if(!internal)
  {
    sequence = nmHandleCnfRsp( connection, NM_RSP_SPECIFIC, NWK_ADDRESS_TYPE_T__UNICAST, status );
    if( status == NWK_STATUS_T__STATUS_SUCCESS )
    {
      // post, waiting for the response
      if( !nmTransPost( connection, NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_RSP_IND, sequence, 
            pBindingEntryReq->srcaddr, zdoRsp, shortaddr ) )
      {
        status = NWK_STATUS_T__STATUS_FAILURE;
      }
  
    }
  }
}

/**************************************************************************************************
 * @fn      processNwkGetBindingTableReq
 *
 * @brief   Handles outgoing App Protobuf messages.
 *
 * @param   connection - connection handle (tcp)
 * @param   pBindingTableReq - pointer to message structure
 *
 * @return  none
 *************************************************************************************************/
static void processNwkGetBindingTableReq( int connection, NwkGetBindingTableReq *pBindingTableReq )
{
  uint16       nwkaddr;
  uint16       sequence;
  uint8        status = NWK_STATUS_T__STATUS_SUCCESS;
  uint8        zdoRsp;
  ZdoMgmtBindReq mgmtBindReq = ZDO_MGMT_BIND_REQ__INIT;  
  
  uiPrintf( "NwkGetBindingTableReq: srcAddr:0x%016llX, \n", 
    pBindingTableReq->dstaddr->ieeeaddr);

  // sanity check the binding request (make sure it's unicast with an endpoint)
  if( (pBindingTableReq->dstaddr->addresstype != NWK_ADDRESS_TYPE_T__UNICAST) || (!pBindingTableReq->dstaddr->has_ieeeaddr) )
  {
    status = NWK_STATUS_T__STATUS_FAILURE;
  }

  // since we need the short address, we MUST have the entry in the database
  if( !nwkMgrDb_GetShortAddr( pBindingTableReq->dstaddr->ieeeaddr, &nwkaddr ))
  {
	status = NWK_STATUS_T__STATUS_FAILURE;
  }
  
  if( pBindingTableReq->dstaddr->ieeeaddr == gLocalDeviceInfo.ieeeaddr)
  {
    nwkaddr = gLocalDeviceInfo.nwkaddr;
  }


  // only send to ZStack if parameters OK
  if( status == NWK_STATUS_T__STATUS_SUCCESS )
  {
    // destinaton of the binding/unbinding
    mgmtBindReq.nwkaddr = nwkaddr;
    mgmtBindReq.startindex = pBindingTableReq->startindex;
	
    // set up ZDP call and send it
	status = sendMgmtBindReq( &mgmtBindReq);
    zdoRsp = ZSTACK_CMD_IDS__ZDO_MGMT_BIND_RSP;
  }

  // send confirm
  sequence = nmHandleCnfRsp( connection, NM_RSP_SPECIFIC, NWK_ADDRESS_TYPE_T__UNICAST, status );

  // if worked, post for the reply
  if( status == NWK_STATUS_T__STATUS_SUCCESS )
  {
    // post, waiting for the response
    if( !nmTransPost( connection, NWK_MGR_CMD_ID_T__NWK_GET_BINDING_TABLE_RSP_IND, sequence, 
          pBindingTableReq->dstaddr, zdoRsp, nwkaddr) )
    {
      status = NWK_STATUS_T__STATUS_FAILURE;
    }
  }
  
}

/**************************************************************************************************
 *
 * @fn      processZdoNwkAddrRspInd
 *
 * @brief   Process incoming ZDO Network Address Response Indication message
 * 
 * @param   pNwkAddrRsp - pointer to command structure
 *
 * @return  none, actually doesn't return
 *
 **************************************************************************************************/
static void processZdoNwkAddrRspInd( ZdoNwkAddrRspInd *pNwkAddrRsp )
{  
  uiPrintf( "Received ZDO Network Address Response\n" );
  
  // Send data to database state machine
  zNwkSrv_UpdateAddDeviceStateMachine( ZSTACK_CMD_IDS__ZDO_NWK_ADDR_RSP, pNwkAddrRsp );
}

/**************************************************************************************************
 *
 * @fn      processZdoNwkAddrRspInd
 *
 * @brief   Process incoming ZDO Network Address Response Indication message
 * 
 * @param   pIeeeAddrRsp - pointer to command structure
 *
 * @return  none, actually doesn't return
 *
 **************************************************************************************************/
static void processZdoIeeeAddrRspInd( ZdoIeeeAddrRspInd *pIeeeAddrRsp )
{  
  uiPrintf("Received ZDO Ieee Address Response\n" );
  
  // Send data to database state machine
  zNwkSrv_UpdateAddDeviceStateMachine( ZSTACK_CMD_IDS__ZDO_IEEE_ADDR_RSP, pIeeeAddrRsp );
}

/**************************************************************************************************
 *
 * @fn      processZdoNodeDescRspInd
 *
 * @brief   Process incoming ZDO Node Descriptor Response Indication message
 * 
 * @param   pNodeDescRsp - pointer to command structure
 *
 * @return  none, actually doesn't return
 *
 **************************************************************************************************/
static void processZdoNodeDescRspInd( ZdoNodeDescRspInd *pNodeDescRsp )
{  
  uiPrintf( "Received ZDO Node Descriptor Response addr %04X\n", pNodeDescRsp->srcaddr );
  
  // Send data to database state machine
  zNwkSrv_UpdateAddDeviceStateMachine( ZSTACK_CMD_IDS__ZDO_NODE_DESC_RSP, pNodeDescRsp );
}

/**************************************************************************************************
 *
 * @fn      processDevStateChangeInd
 *
 * @brief   State has changed. causes a NWK_ZIGBEE_NWK_READY_IND__INIT to
 *          sent if the network is ready.
 *
 * @return  none
 *
 **************************************************************************************************/
static void processDevStateChangeInd( void )
{
  NwkZigbeeNwkReadyInd nwkReadyInd = NWK_ZIGBEE_NWK_READY_IND__INIT;
  
  uiPrintf("Received Device State Change\n" );
  
  //  update gLocalDeviceInfo if device state has changed (so all info is updated)
  sendSysNwkInfoReadReqAndSetTxPower();
  
  if ( (gLocalDeviceInfo.devstate == DEV_STATE__DEV_ZB_COORD) || (gLocalDeviceInfo.devstate == DEV_STATE__DEV_ROUTER) )
  {
    nwkReadyInd.nwkchannel = gLocalDeviceInfo.logicalchannel;
    nwkReadyInd.panid = gLocalDeviceInfo.panid;
    nwkReadyInd.extpanid = gLocalDeviceInfo.extendedpanid;

    sendNwkZigbeeNwkReadyInd( &nwkReadyInd );
  }
}


/**************************************************************************************************
 *
 * @fn      processNwkCommissioningNoti
 *
 * @brief   App Cnf Start Commissioning was triggered and this is the status. This causes a NWK_ZIGBEE_NWK_READY_IND__INIT to
 *          sent if the network is ready.
 *
 * @param   none
 * 
 * @return  none
 *
 **************************************************************************************************/
static void processNwkCommissioningNoti( void )
{
  NwkZigbeeNwkReadyInd nwkReadyInd = NWK_ZIGBEE_NWK_READY_IND__INIT;
  
  uiPrintf("Received Commissioning Notification\n" );
  
  // udpate gLocalDeviceInfo if device state has changed (so all info is updated)

  sendSysNwkInfoReadReqAndSetTxPower();
  
  if ( (gLocalDeviceInfo.devstate == DEV_STATE__DEV_ZB_COORD) || (gLocalDeviceInfo.devstate == DEV_STATE__DEV_ROUTER) )
  {
    nwkReadyInd.nwkchannel = gLocalDeviceInfo.logicalchannel;
    nwkReadyInd.panid = gLocalDeviceInfo.panid;
    nwkReadyInd.extpanid = gLocalDeviceInfo.extendedpanid;

    sendNwkZigbeeNwkReadyInd( &nwkReadyInd );
  }
}

/**************************************************************************************************
 *
 * @fn      processZdoLeaveInd
 *
 * @brief   Process incoming Device State Change message
 *
 * @param   pLeaveInd - pointer to command structure
 * 
 * @return  none, actually doesn't return
 *
 **************************************************************************************************/
static void processZdoLeaveInd( ZdoLeaveInd *pLeaveInd )
{
  NwkZigbeeDeviceInd deviceInd = NWK_ZIGBEE_DEVICE_IND__INIT;
  sNwkMgrDb_DeviceInfo_t *pDbDeviceInfo;
  NwkDeviceInfoT *pPbDeviceInfo;
  
  uiPrintf("Received ZDO Leave Indication: NwkAddr: %04X\n\n", pLeaveInd->srcaddr );
  
  // Stop service discovery (if applicable)
  zNwkSrv_AD_HaltStateMachine( pLeaveInd->extendedaddr );
    
  // Get device for updated ZigBee device indication
  pDbDeviceInfo = nwkMgrDb_GetDeviceInfo( pLeaveInd->extendedaddr );
  
  // Check to see if we got the device information from the database
  if ( pDbDeviceInfo )
  {
    // Convert device information to protobuf form
    pPbDeviceInfo = allocPbDeviceInfoList( 1, pDbDeviceInfo );
    
    if ( pPbDeviceInfo )
    {
      // Indicate that device was removed
      pPbDeviceInfo->devicestatus = NWK_DEVICE_STATUS_T__DEVICE_REMOVED;
    
      // Send updated device indication to app(s)
      deviceInd.deviceinfo = pPbDeviceInfo;
      sendNwkZigbeeDeviceInd( &deviceInd );
      
      // Free protobuf device info list
      freePbDeviceInfoList( 1, pPbDeviceInfo );
      
      // Remove device from device list
      if ( nwkMgrDb_RemoveDevice( pLeaveInd->extendedaddr ) )
      {
        uiPrintf("NwkAddr: %04X removed from device list\n", pLeaveInd->srcaddr );
      }
    }
        
    // Free database device info list
    nwkMgrDb_FreeDeviceInfo( pDbDeviceInfo );
  }
  else
  {
    uiPrintf("NwkAddr: %04X already removed from device list\n", pLeaveInd->srcaddr );
  }
}

/**************************************************************************************************
 *
 * @fn      processZdoSimpleDescRspInd
 *
 * @brief   Process incoming ZDO Simple Descriptor Response message
 * 
 * @param   pSimpleRsp - pointer to command structure
 *
 * @return  none
 *
 **************************************************************************************************/
static void processZdoSimpleDescRspInd( ZdoSimpleDescRspInd *pSimpleRsp )
{
  // Send data to database state machine
  zNwkSrv_UpdateAddDeviceStateMachine( ZSTACK_CMD_IDS__ZDO_SIMPLE_DESC_RSP, pSimpleRsp );

  // Send data to Commissioning Engine too
  transTable_t * pTrans;
  
  pTrans = nmTransFindByZdoRsp( ZSTACK_CMD_IDS__ZDO_SIMPLE_DESC_RSP, pSimpleRsp->srcaddr );
  if( pTrans )
  {
    sendNwkSimpleDescriptorRspInd( pTrans->connection, pSimpleRsp);
    nmTransFreeEntry(pTrans);
  }
}

/**************************************************************************************************
 *
 * @fn          sendNwkSimpleDescriptorRspInd
 *
 * @brief       send message
 *
 * @param       connection  - tcp connection
 * @parm        sequence    - app sequence #
 * @param       status      - NwkStatusT
 * @param       srcaddr     - source address
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendNwkSimpleDescriptorRspInd( int connection, ZdoSimpleDescRspInd *pInMsg )
{
  NwkSendSimpleDescriptorRspInd rspInd = NWK_SEND_SIMPLE_DESCRIPTOR_RSP_IND__INIT;
  int len;
  uint8 *pBuf;

  rspInd.status = pInMsg->status;
  rspInd.srcaddr = pInMsg->srcaddr;
  rspInd.nwkaddrofinterest = pInMsg->nwkaddrofinterest;
  rspInd.simpledesc = (NwkSimpleDescriptorT *)pInMsg->simpledesc;

  len = nwk_send_simple_descriptor_rsp_ind__get_packed_size(&rspInd);
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_send_simple_descriptor_rsp_ind__pack( &rspInd, pBuf );

    // Send response back to app
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, TRUE, 
                   NWK_MGR_CMD_ID_T__NWK_SEND_SIMPLE_DESCRIPTOR_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn      processZdoActiveEpRspInd
 *
 * @brief   Process incoming ZDO Active Endpoint Response Indication message
 * 
 * @param   pActiveEpRsp - pointer to command structure
 *
 * @return  none, actually doesn't return
 *
 **************************************************************************************************/
static void processZdoActiveEpRspInd( ZdoActiveEndpointsRspInd *pActiveEpRsp )
{  
  uiPrintf("Received ZDO Node Descriptor Response\n\n" );
  
  // Send data to database state machine
  zNwkSrv_UpdateAddDeviceStateMachine( ZSTACK_CMD_IDS__ZDO_ACTIVE_EP_RSP, pActiveEpRsp );
}

/**************************************************************************************************
 *
 * @fn      processZdoMgmtPermitJoinRspInd
 *
 * @brief   Process incoming ZDO Mgmt Permit Join Response Indication message
 * 
 * @param   pPermitJoinRsp - pointer to command structure
 *
 * @return  none, actually doesn't return
 *
 **************************************************************************************************/
static void processZdoMgmtPermitJoinRspInd( ZdoMgmtPermitJoinRspInd *pPermitJoinRsp )
{  
  uiPrintf("Received ZDO Mgmt Permit Join Response:" );
  uiPrintf("Source Address: %04X", pPermitJoinRsp->srcaddr );
  uiPrintf("ZDP Status: %d\n\n", pPermitJoinRsp->status );  
}

/**************************************************************************************************
 *
 * @fn      processSysResetInd
 *
 * @brief   Process incoming SYS Reset Indication message
 * 
 * @param   pResetInd - pointer to command structure
 *
 * @return  none, actually doesn't return
 *
 **************************************************************************************************/
static void processSysResetInd( SysResetInd *pResetInd )
{  
  NwkZigbeeNwkReadyInd nwkReadyInd = NWK_ZIGBEE_NWK_READY_IND__INIT;
  
  uiPrintf("Received SYS Reset Ind, reason: %d\n\n", pResetInd->reason );
  
  sendSysNwkInfoReadReqAndSetTxPower();
          
  nwkReadyInd.nwkchannel = gLocalDeviceInfo.logicalchannel;
  nwkReadyInd.panid = gLocalDeviceInfo.panid;
  nwkReadyInd.extpanid = gLocalDeviceInfo.extendedpanid;
  
  sendNwkZigbeeNwkReadyInd( &nwkReadyInd );
}

/**************************************************************************************************
 *
 * @fn      processAfIncomingMsgInd
 *
 * @brief   Process AF Incoming Message Indication message
 * 
 * @param   pInMsg - pointer to incoming AF message indication
 *
 * @return  none
 *
 **************************************************************************************************/
static void processAfIncomingMsgInd( AfIncomingMsgInd *pInMsg )
{
  uiPrintf("AfIncomingMsgInd: clusterID:%d\n", pInMsg->clusterid );
}

/**************************************************************************************************
 *
 * @fn      processAfDataConfirmInd
 *
 * @brief   Process AF Data Confirm Message Indication message
 * 
 * @param   pInMsg - pointer to incoming AF data confirmation indication
 *
 * @return  none
 *
 **************************************************************************************************/
static void processAfDataConfirmInd( AfDataConfirmInd *pInMsg )
{
  // Do nothing
}

/**************************************************************************************************
 *
 * @fn      processZdoBindRspInd
 *
 * @brief   Process ZDO Bind Response message
 * 
 * @param   pInMsg - pointer to message structure
 *
 * @return  none
 *
 **************************************************************************************************/
static void processZdoBindRspInd( ZdoBindRspInd *pInMsg )
{
  transTable_t * pTrans;
  
  pTrans = nmTransFindByZdoRsp( ZSTACK_CMD_IDS__ZDO_BIND_RSP, pInMsg->srcaddr );
  if( pTrans )
  {
    sendNwkSetBindingEntryRspInd( pTrans->connection, pTrans->sequence, pInMsg->status, &pTrans->addr );
    nmTransFreeEntry(pTrans);
  }
}

/**************************************************************************************************
 *
 * @fn      processZdoUnbindRspInd
 *
 * @brief   Process ZDO Unbind Response message
 * 
 * @param   pInMsg - pointer to message structure
 *
 * @return  none
 *
 **************************************************************************************************/
static void processZdoUnbindRspInd( ZdoUnbindRspInd *pInMsg )
{
  transTable_t * pTrans;
  uint32_t status;

  uiPrintf( "NwkMgr processZdoUnbindRspInd: NwkAddr %04X, status %d\n", pInMsg->srcaddr, pInMsg->status );

  pTrans = nmTransFindByZdoRsp( ZSTACK_CMD_IDS__ZDO_UNBIND_RSP, pInMsg->srcaddr );
  if( pTrans )
  {
    /*if ( pInMsg->status == ZDP_STATUS__SUCCESS )
    {
      status = NWK_STATUS_T__STATUS_SUCCESS;
    }
    else if ( pInMsg->status == ZDP_STATUS__INVALID_EP )
    {
      status = NWK_STATUS_T__STATUS_INVALID_PARAMETER;
    }
    else
    {
      status = NWK_STATUS_T__STATUS_FAILURE;
    }*/
    status = pInMsg->status;
    sendNwkSetBindingEntryRspInd( pTrans->connection, pTrans->sequence, status, &pTrans->addr );
    nmTransFreeEntry(pTrans);
  }
}

/**************************************************************************************************
 *
 * @fn      processZdoUnbindRspInd
 *
 * @brief   Process ZDO Unbind Response message
 * 
 * @param   pInMsg - pointer to message structure
 *
 * @return  none
 *
 **************************************************************************************************/
static void processZdoMgmtLqiRspInd( ZdoMgmtLqiRspInd *pInMsg )
{
  transTable_t *pTrans;
  
  uiPrintf( "NwkMgr processZdoMgmtLqiRspInd: status %d, nwkaddr %04X, count %d\n",
    pInMsg->status, pInMsg->srcaddr, pInMsg->n_lqilist );

  pTrans = nmTransFindByZdoRsp( ZSTACK_CMD_IDS__ZDO_MGMT_LQI_RSP, pInMsg->srcaddr );
  if(pTrans)
  {
    sendNwkGetNeighborTableRspInd( pInMsg->status, pTrans, pInMsg );
    nmTransFreeEntry(pTrans);
  }
}

/**************************************************************************************************
 *
 * @fn      processZdoTcDeviceInd
 *
 * @brief   Process ZDO Trust Center Device Indication message
 * 
 * @param   pInMsg - pointer to message structure
 *
 * @return  none
 *
 **************************************************************************************************/
static void processZdoTcDeviceInd( ZdoTcDeviceInd *pInMsg )
{
  uiPrintf("Processing ZDO TC Device Ind: %016llX\n", pInMsg->extendedaddr );

  zNwkSrv_AddDevice( pInMsg->extendedaddr, pInMsg, 0, nwkSrvAddDeviceTcInfoCB );
}

/**************************************************************************************************
 *
 * @fn      processZdoUnbindRspInd
 *
 * @brief   Process ZDO Unbind Response message
 * 
 * @param   pInMsg - pointer to message structure
 *
 * @return  none
 *
 **************************************************************************************************/
static void processZdoMgmtRtgRspInd( ZdoMgmtRtgRspInd *pInMsg )
{
  transTable_t *pTrans;
  
  uiPrintf( "NwkMgr processZdoMgmtRtgRspInd: status %d, nwkaddr %04X, count %d\n", 
    pInMsg->status, pInMsg->srcaddr, pInMsg->n_rtglist );

  pTrans = nmTransFindByZdoRsp( ZSTACK_CMD_IDS__ZDO_MGMT_RTG_RSP, pInMsg->srcaddr );
  if(pTrans)
  {
    sendNwkGetRoutingTableRspInd( pInMsg->status, pTrans, pInMsg );
    nmTransFreeEntry(pTrans);
  }
}

/**************************************************************************************************
 *
 * @fn      processZdoMgmtBindRspInd
 *
 * @brief   Process ZDO Mgmt Bind Response message
 * 
 * @param   pInMsg - pointer to message structure
 *
 * @return  none
 *
 **************************************************************************************************/
static void processZdoMgmtBindRspInd( ZdoMgmtBindRspInd *pInMsg )
{
  transTable_t *pTrans;
  NwkStatusT status;
  
  uiPrintf( "NwkMgr processZdoMgmtBindRsp: NwkAddr %04X, status %d\n", pInMsg->srcaddr, pInMsg->status );

  pTrans = nmTransFindByZdoRsp( ZSTACK_CMD_IDS__ZDO_MGMT_BIND_RSP, pInMsg->srcaddr );
  if ( pInMsg->status == ZDP_STATUS__SUCCESS )
  {
    status = NWK_STATUS_T__STATUS_SUCCESS;
  }
  else if ( pInMsg->status == ZDP_STATUS__INVALID_EP )
  {
    status = NWK_STATUS_T__STATUS_INVALID_PARAMETER;
  }
  else
  {
    status = NWK_STATUS_T__STATUS_FAILURE;
  }

  sendNwkGetBindingTableRspInd( pTrans->connection, status, pInMsg);
  nmTransFreeEntry(pTrans);

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
  uint8 capInfo = 0;

  // SJS_119, fix for removedevice
  // convert CapInfo to byte
  if(pInMsg && pInMsg->capinfo)
  {
    if(pInMsg->capinfo->pancoord)
      capInfo |= gCapInfo_Coord_c;
    if(pInMsg->capinfo->ffd)
      capInfo |= gCapInfo_Router_c;
    if(pInMsg->capinfo->mainspower)
      capInfo |= gCapInfo_MainsPowered_c;
    if(pInMsg->capinfo->rxonwhenidle)
      capInfo |= gCapInfo_RxOnIdle_c;
    if(pInMsg->capinfo->security)
      capInfo |= gCapInfo_Secure_c;
  }

  uiPrintf( "NwkMgr processZdoDeviceAnnounce: ieeeaddr %016llX, nwkaddr %04X, capInfo %02X\n", 
    pInMsg->devextaddr, pInMsg->devaddr, capInfo );

  // try to update cap info in the state machine
  if( !zNwkSrv_AD_UpdateCapInfo( pInMsg->devextaddr, capInfo ) )
  {
    // no state machine live, try to update the database record
    nwkMgrDb_SetDevInfo( pInMsg->devextaddr, capInfo, pInMsg->devaddr);
  }
  
  // update the service discovery timeout
  zNwkSrv_AD_ProcessDeviceAnnounce( pInMsg->devextaddr );
}

/**************************************************************************************************
 *
 * @fn          sendAPICExpectDefaultStatus
 *
 * @brief       Send a request message and expect the normal "default" response (MacDefaultRsp)
 *
 * @param       cmdId - messages command ID
 * @param       len - length of pData buffer to send
 * @param       pData - pointer to buffer to send
 *
 * @return      synchronous return status
 *
 **************************************************************************************************/
ZStatusValues sendAPICExpectDefaultStatus( int cmdId, int len, uint8 *pData )
{
  uint8 rspCmdId;
  uint8 *pRsp;
  uint16 rspLen;
  ZstackDefaultRsp *pDefaultRsp;
  ZStatusValues status = ZSTATUS_VALUES__ZDecodeError;

  // Send protobuf packed request to API Client synchronously
  pRsp = apicSendSynchData( NM_API_CLIENT, ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF, 
                            cmdId, len, pData,
                            NULL, &rspCmdId, &rspLen );

  if ( pRsp )
  {
    if ( (ZSTACK_CMD_IDS__ZSTACK_DEFAULT_RSP == rspCmdId) && (rspLen > 0) )
    {
      pDefaultRsp = zstack_default_rsp__unpack( NULL, rspLen, pRsp );
      if ( pDefaultRsp )
      {
        if (pDefaultRsp->cmdid == cmdId)
        {
          status = pDefaultRsp->status;
        }
        zstack_default_rsp__free_unpacked( pDefaultRsp, NULL );
      }
    }
    
    apicFreeSynchData( pRsp );
  }

  // return the status
  return ( status );
}

/**************************************************************************************************
 *
 * @fn      sendZbGenericCnf
 *
 * @brief   Send ZigBee Generic Confirmation message
 *
 * @param   connection - connection handle (tcp)
 * @param   pGenericCnf - generic confirmation command structure
 *
 * @return  ZStatusValues
 *
 **************************************************************************************************/
static void sendZbGenericCnf( int connection, NwkZigbeeGenericCnf *pGenericCnf )
{
  int len;
  uint8 *pBuf;

  uiPrintf("Sending ZigBee Generic Confirmation \n" );

  len = nwk_zigbee_generic_cnf__get_packed_size( pGenericCnf );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_zigbee_generic_cnf__pack( pGenericCnf, pBuf );

    // Send response back to app (synchronous)
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, FALSE, 
                   NWK_MGR_CMD_ID_T__ZIGBEE_GENERIC_CNF, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn			sendSysSetTxPowerReq
 *
 * @brief		Send System TX Power Request
 *
 * @param		power - power level in dBm
 *
 * @return		ZStatusValues
 *
 **************************************************************************************************/
static ZStatusValues sendSysSetTxPowerReq( int8 power )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;
  SysSetTxPowerReq SetTxPowerReq = SYS_SET_TX_POWER_REQ__INIT;
  uint8 rspCmdId;
  uint8 *pRsp;
  uint16 rspLen;
  SysSetTxPowerRsp *SetTxPowerRsp;
  
  uiPrintfEx(trINFO, "Setting Tx Power to 0x%02X\n", power );

  SetTxPowerReq.requestedtxpower = power;

  len = sys_set_tx_power_req__get_packed_size( &SetTxPowerReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    sys_set_tx_power_req__pack( &SetTxPowerReq, pBuf );
    
    // Send protobuf packed request to API Client synchronously
    pRsp = apicSendSynchData( NM_API_CLIENT, ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF, 
      ZSTACK_CMD_IDS__SYS_SET_TX_POWER_REQ, len, pBuf,
      NULL, &rspCmdId, &rspLen );
    
    if ( pRsp )
    {
      if ( (ZSTACK_CMD_IDS__SYS_SET_TX_POWER_RSP == rspCmdId) && (rspLen > 0) )
      {
        SetTxPowerRsp = sys_set_tx_power_rsp__unpack( NULL, rspLen, pRsp );
        if ( SetTxPowerRsp )
        {
          uiPrintfEx( trINFO, "TX power set to %d%s\n", (int8)SetTxPowerRsp->txpower, power == (int8)SetTxPowerRsp->txpower ? " (as requested)" : " (nearest supported value)");
          
          status = ZSTATUS_VALUES__ZSuccess;
          sys_set_tx_power_rsp__free_unpacked( SetTxPowerRsp, NULL );
        }
      }
      else
      {
        status = ZSTATUS_VALUES__ZDecodeError;
        uiPrintfEx( trERROR, "Unexpected incoming command ID when expecting ZSTACK_CMD_IDS__SYS_SET_TX_POWER_RSP\n");
      }
      
      apicFreeSynchData( pRsp );
    }
    
    free( pBuf );
  }
  
  return status;
}


/**************************************************************************************************
 *
 * @fn          sendNwkZigbeeNwkReadyInd
 *
 * @brief       Send Network Ready Indication message
 *
 * @param       pNwkReadyInd - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static void sendNwkZigbeeNwkReadyInd( NwkZigbeeNwkReadyInd *pNwkReadyInd )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf("Sending Network Ready Indication \n" );

  len = nwk_zigbee_nwk_ready_ind__get_packed_size( pNwkReadyInd );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_zigbee_nwk_ready_ind__pack( pNwkReadyInd, pBuf );

    // Send response back to app(s)
    APIS_SendData( -1, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, TRUE, 
                   NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_READY_IND, len, pBuf );
    
    free( pBuf );
  }  
}

/**************************************************************************************************
 *
 * @fn          sendSysNwkInfoReadReqAndSetTxPower
 *
 * @brief       Read the current state of the ZStack system and place in gLocalDeviceInfo, then set TX power if appropriate
 *
 * @param       none
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendSysNwkInfoReadReqAndSetTxPower( void )
{
  int len;
  uint8 rspcmdid;
  uint8 *pBuf;
  uint8 *pRsp;
  uint16 rsplen;
  ZStatusValues status = ZSTATUS_VALUES__ZFailure;
  SysNwkInfoReadReq niReq = SYS_NWK_INFO_READ_REQ__INIT;

  uiPrintf( "NwkMgr sendSysNwkInfoReadReqAndSetTxPower:\n" );

  niReq.cmdid = (ZStackCmdIDs) ZSTACK_CMD_IDS__SYS_NWK_INFO_READ_REQ;

  len = sys_nwk_info_read_req__get_packed_size( &niReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    sys_nwk_info_read_req__pack( &niReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    pRsp = apicSendSynchData( NM_API_CLIENT, ZSTACK_SYS_IDS__RPC_SYS_PROTOBUF, 
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
          
          uiPrintf("NwkInfoReadRsp:\n" );
          uiPrintf("- nwkaddr:%04X\n", gLocalDeviceInfo.nwkaddr);
      uiPrintf("- ieeeaddr:%016llX\n", gLocalDeviceInfo.ieeeaddr);
      uiPrintf("- devstate:%d\n", gLocalDeviceInfo.devstate);
          uiPrintf( "- panid:%04X\n", gLocalDeviceInfo.panid);
      uiPrintf("- extendedpanid:%016llX\n", gLocalDeviceInfo.extendedpanid );
          uiPrintf( "- coordaddr:%04X\n", gLocalDeviceInfo.coordaddr);
      uiPrintf("- coordextaddr:%016llX\n", gLocalDeviceInfo.coordextaddr );
          uiPrintf( "- devtypes: Coordinator: %d\n", gLocalDeviceInfo.devtypes->coodinator );
          uiPrintf( "- devtypes: Router: %d\n", gLocalDeviceInfo.devtypes->router );
          uiPrintf( "- logicalchannel:%d\n\n", gLocalDeviceInfo.logicalchannel );

          sys_nwk_info_read_rsp__free_unpacked( pNiRsp, NULL );
          
          status = ZSTATUS_VALUES__ZSuccess;
		  
          nmSetChannelSpecificTxPower(); //after successfully updating network information, update the TX power if necessary
        }
      }
      else
      {
        uiPrintfEx(trERROR, "Expected NwkInfoReadRsp, got %d\n", rspcmdid );
      }
      
      apicFreeSynchData( pRsp );
    }
    
    free( pBuf );
  }
  else
  {
    status = ZSTATUS_VALUES__ZMemError;
  }

  return ( status );
}

/**************************************************************************************************
 *
 * @fn          sendNwkZigbeeDeviceInd
 *
 * @brief       Send Device Indication message
 *
 * @param       pNwkReadyInd - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
void sendNwkZigbeeDeviceInd( NwkZigbeeDeviceInd *pDeviceInd )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf( "NwkMgr sendNwkZigbeeDeviceInd:\n" );

  len = nwk_zigbee_device_ind__get_packed_size( pDeviceInd );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_zigbee_device_ind__pack( pDeviceInd, pBuf );

    // Send response back to app(s)
    APIS_SendData( -1, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, TRUE, 
                   NWK_MGR_CMD_ID_T__NWK_ZIGBEE_DEVICE_IND, len, pBuf );
    
    free( pBuf );
  }  
}

/**************************************************************************************************
 *
 * @fn          sendSimpleDescReq
 *
 * @brief       Send ZDO Simple Descriptor Request message
 *
 * @param       dstAddr - destination address of device in question
 * @param       endpointId - destination node's application endpoint ID
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendSimpleDescReq( uint16 dstAddr, uint8 endpointId )
{
  int len;
  uint8 *pBuf;
  ZdoSimpleDescReq sdReq = ZDO_SIMPLE_DESC_REQ__INIT;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;

  uiPrintf( "NwkMgr sendSimpleDescReq: nwkaddr %04X\n", dstAddr );

  sdReq.dstaddr = dstAddr;
  sdReq.nwkaddrofinterest = dstAddr;
  sdReq.endpoint = endpointId;

  len = zdo_simple_desc_req__get_packed_size( &sdReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_simple_desc_req__pack( &sdReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_SIMPLE_DESC_REQ, len, pBuf );

    free( pBuf );
  }
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendMatchDescReq
 *
 * @brief       Send ZDO Match Descriptor Request message
 *
 * @param       dstAddr - destination address of device in question
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendMatchDescReq( uint16 dstAddr )
{
  int len;
  uint8 *pBuf;
  uint32_t clustID = ZCL_CLUSTER_ID_GEN_ON_OFF;
  ZdoMatchDescReq mdReq = ZDO_MATCH_DESC_REQ__INIT;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;

  uiPrintf( "NwkMgr sendMatchDescReq: nwkaddr %04X\n", dstAddr );

  mdReq.cmdid = (ZStackCmdIDs) ZSTACK_CMD_IDS__ZDO_MATCH_DESC_REQ;
  mdReq.dstaddr = dstAddr;
  mdReq.nwkaddrofinterest = 0xFFFF; // Broadcast - ALL
  mdReq.profileid = ZCL_HA_PROFILE_ID;
  mdReq.n_inputclusters = 1;
  mdReq.inputclusters = &clustID;

  len = zdo_match_desc_req__get_packed_size( &mdReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    zdo_match_desc_req__pack( &mdReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_MATCH_DESC_REQ, len, pBuf );

    free( pBuf );
  }
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendZdoMgmtPermitJoinReq
 *
 * @brief       Send ZDO MGMT Permit Join Request message
 *
 * @param       pNwkReadyInd - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendZdoMgmtPermitJoinReq( ZdoMgmtPermitJoinReq *pPermitJoinReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;
  
  uiPrintf( "NwkMgr sendZdoMgmtPermitJoinReq: nwkaddr %04X\n", pPermitJoinReq->nwkaddr );
  
  len = zdo_mgmt_permit_join_req__get_packed_size( pPermitJoinReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_mgmt_permit_join_req__pack( pPermitJoinReq, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_MGMT_PERMIT_JOIN_REQ, len, pBuf );

    free( pBuf );
  }
  
  return( status );
}

/**************************************************************************************************
 *
 * @fn          sendAppCnfStartCommissioningReq
 *
 * @brief       Send APP CONFIG START COMMISSIONING Request message
 *
 * @param       pStartCommissioningReq - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendAppCnfStartCommissioningReq( AppCnfStartCommissioningReq *pStartCommissioningReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;
  
  uiPrintf( "NwkMgr sendAppCnfStartCommissioningReq: " );
  
  len = app_cnf_start_commissioning_req__get_packed_size( pStartCommissioningReq);
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = app_cnf_start_commissioning_req__pack( pStartCommissioningReq, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__APP_CNF_START_COMMISSIONING_REQ, len, pBuf );

    free( pBuf );
  }
  
  return( status );
}

/**************************************************************************************************
 *
 * @fn          sendAppCnfAddInstallCodeReq
 *
 * @brief       Send APP CONFIG ADD INSTALLCODE Request message
 *
 * @param       pAddInstallCodeReq - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendAppCnfAddInstallCodeReq( AppCnfAddInstallCodeReq *pAddInstallCodeReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;  
 
  len = app_cnf_add_install_code_req__get_packed_size( pAddInstallCodeReq );

  pBuf = malloc( len );
  if ( pBuf )
  {
    len = app_cnf_add_install_code_req__pack( pAddInstallCodeReq, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__APP_CNF_ADD_INSTALL_CODE_REQ, len, pBuf );

    free( pBuf );
  }

  return( status );
}

/**************************************************************************************************
 *
 * @fn          sendZdoBindReq
 *
 * @brief       Send ZDO Bind Request message
 *
 * @param       pBindReq - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendZdoBindReq( ZdoBindReq *pBindReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;
  
  uiPrintf( "NwkMgr sendZdoBindReq: nwkaddr %04X\n", pBindReq->nwkaddr );
  
  len = zdo_bind_req__get_packed_size( pBindReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_bind_req__pack( pBindReq, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_BIND_REQ, len, pBuf );

    free( pBuf );
  }
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendZdoUnbindReq
 *
 * @brief       Send ZDO Bind Request message
 *
 * @param       pUnbindReq - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendZdoUnbindReq( ZdoUnbindReq *pUnbindReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;
  
  uiPrintf( "NwkMgr sendZdoUnbindReq: nwkaddr %04X\n", pUnbindReq->nwkaddr );
  
  len = zdo_unbind_req__get_packed_size( pUnbindReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_unbind_req__pack( pUnbindReq, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_UNBIND_REQ, len, pBuf );

    free( pBuf );
  }
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendZdoMgmtRtgReq
 *
 * @brief       Send Routing Table Request message
 *
 * @param       dstaddr - address to send the request
 * @param       startindex - starting index
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendZdoMgmtRtgReq( uint16 nwkaddr, uint8 startindex )
{
  ZdoMgmtRtgReq req = ZDO_MGMT_RTG_REQ__INIT;
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError; // in case we can't allocate
  
  uiPrintf( "NwkMgr sendZdoMgmtRtgReq: nwkaddr %04X, index %d\n", nwkaddr, startindex );
  
  // set up the request
  req.nwkaddr = nwkaddr;
  req.startindex = startindex;
  
  len = zdo_mgmt_rtg_req__get_packed_size( &req );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_mgmt_rtg_req__pack( &req, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_MGMT_RTG_REQ, len, pBuf );

    free( pBuf );
  }
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendMgmtBindReq
 *
 * @brief       Send ZDO MGMT BIND Request message
 *
 * @param       pMgmtBindReq - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendMgmtBindReq( ZdoMgmtBindReq *pMgmtBindReq)
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;
  
  uiPrintf( "NwkMgr sendMgmtBindReq: " );

  len = zdo_mgmt_bind_req__get_packed_size( pMgmtBindReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_mgmt_bind_req__pack( pMgmtBindReq, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_MGMT_BIND_REQ, len, pBuf );

    free( pBuf );
  }
  
  return( status );
}

/**************************************************************************************************
 *
 * @fn          sendZdoMgmtLqiReq
 *
 * @brief       Send LQI (Neighbor Table) Request message
 *
 * @param       dstaddr - address to send the request
 * @param       startindex - starting index
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendZdoMgmtLqiReq( uint16 nwkaddr, uint8 startindex )
{
  ZdoMgmtLqiReq req = ZDO_MGMT_LQI_REQ__INIT;
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError; // in case we can't allocate
  
  uiPrintf( "NwkMgr sendZdoMgmtLqiReq: nwkaddr %04X, index %d\n", nwkaddr, startindex );
  
  req.nwkaddr = nwkaddr;
  req.startindex = startindex;
  
  len = zdo_mgmt_lqi_req__get_packed_size( &req );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_mgmt_lqi_req__pack( &req, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_MGMT_LQI_REQ, len, pBuf );

    free( pBuf );
  }
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendZdoNodeDescReq
 *
 * @brief       Send ZDO Node Descriptor Request message
 *
 * @param       shortAddr - short address of remote node
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendZdoNodeDescReq( uint16 shortAddr )
{
  ZdoNodeDescReq nodeDescReq = ZDO_NODE_DESC_REQ__INIT;
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError; // in case we can't allocate


  uiPrintf( "NwkMgr sendZdoNodeDescReq: nwkaddr %04X\n", shortAddr );
  
  nodeDescReq.dstaddr = shortAddr;
  nodeDescReq.nwkaddrofinterest = shortAddr;
  
  len = zdo_node_desc_req__get_packed_size( &nodeDescReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_node_desc_req__pack( &nodeDescReq, pBuf );

    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_NODE_DESC_REQ, len, pBuf );

    free( pBuf );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendZdoNwkAddrReq
 *
 * @brief       Send ZDO Network Address Request message
 *
 * @param       ieeeAddr - IEEE address of remote node
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendZdoNwkAddrReq( uint64_t ieeeAddr )
{
  int len;
  uint8 *pBuf;
  ZdoNwkAddrReq nwkAddrReq = ZDO_NWK_ADDR_REQ__INIT;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError; // in case we can't allocate

  uiPrintf( "NwkMgr sendZdoNwkAddrReq: ieeeaddr 0x%016llX\n", ieeeAddr );
  
  nwkAddrReq.ieeeaddr = ieeeAddr;
  nwkAddrReq.type = NWK_ADDR_REQ_TYPE__SINGLE_DEVICE;
  nwkAddrReq.startindex = 0;
  
  len = zdo_nwk_addr_req__get_packed_size( &nwkAddrReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_nwk_addr_req__pack( &nwkAddrReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_NWK_ADDR_REQ, len, pBuf );

    free( pBuf );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendZdoNwkAddrReq
 *
 * @brief       Send ZDO Network Address Request message
 *
 * @param       nwkAddr - short address of remote node
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendZdoIeeeAddrReq( uint16 nwkAddr )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError; // in case we can't allocate
  ZdoIeeeAddrReq ieeeAddrReq = ZDO_IEEE_ADDR_REQ__INIT;

  uiPrintf( "NwkMgr sendZdoIeeeAddrReq: ieeeaddr %04X\n", nwkAddr );
  
  ieeeAddrReq.nwkaddr = nwkAddr;
  ieeeAddrReq.type = NWK_ADDR_REQ_TYPE__SINGLE_DEVICE;
  ieeeAddrReq.startindex = 0;
  
  len = zdo_ieee_addr_req__get_packed_size( &ieeeAddrReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_ieee_addr_req__pack( &ieeeAddrReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_IEEE_ADDR_REQ, len, pBuf );

    free( pBuf );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendZdoActiveEndpointReq
 *
 * @brief       Send ZDO Active Endpoint Request message
 *
 * @param       shortAddr - short address of remote node
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendZdoActiveEndpointReq( uint16 shortaddr )
{
  int len;
  uint8 *pBuf;
  ZdoActiveEndpointReq activeEpReq = ZDO_ACTIVE_ENDPOINT_REQ__INIT;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError; // in case we can't allocate

  uiPrintf( "NwkMgr sendZdoActiveEndpointReq: nwkaddr %04X\n", shortaddr );
  
  activeEpReq.dstaddr = shortaddr;
  activeEpReq.nwkaddrofinterest = shortaddr;
  
  len = zdo_active_endpoint_req__get_packed_size( &activeEpReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = zdo_active_endpoint_req__pack( &activeEpReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__ZDO_ACTIVE_ENDPOINT_REQ, len, pBuf );

    free( pBuf );
  }
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendSysResetReq
 *
 * @brief       Send SYS Reset Req message to ZStack
 *
 * @param       pResetReq - short address of remote node
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendSysResetReq( SysResetReq *pResetReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;


  uiPrintf("NwkMgr sendSysResetReq: resettype %d\n", pResetReq->type);
  
  len = sys_reset_req__get_packed_size( pResetReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = sys_reset_req__pack( pResetReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__SYS_RESET_REQ, len, pBuf );

    free( pBuf );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendSecNwkKeySetReq
 *
 * @brief       Send SYS Reset Req message to ZStack
 *
 * @param       pResetReq - short address of remote node
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendSecNwkKeySetReq( SecNwkKeySetReq *pSetReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;

  uiPrintf( "NwkMgr sendSecNwkKeySetReq\n" );
  
  len = sec_nwk_key_set_req__get_packed_size( pSetReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = sec_nwk_key_set_req__pack( pSetReq, pBuf );

    // Send protobuf packed request to API Client with synchronous status response
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__SEC_NWK_KEY_SET_REQ, len, pBuf );

    free( pBuf );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendSecNwkKeySetReq
 *
 * @brief       Send SYS Reset Req message to ZStack
 *
 * @param       pResetReq - short address of remote node
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendSecNwkKeyUpdateReq( SecNwkKeyUpdateReq *pUpdateReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;

  uiPrintf( "NwkMgr sendSecNwkKeyUpdateReq\n" );
  
  len = sec_nwk_key_update_req__get_packed_size( pUpdateReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = sec_nwk_key_update_req__pack( pUpdateReq, pBuf );

    // Send protobuf packed request to API Client with synchronous status response
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__SEC_NWK_KEY_UPDATE_REQ, len, pBuf );

    free( pBuf );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendSecNwkKeySwitchReq
 *
 * @brief       Send SYS Reset Req message to ZStack
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendSecNwkKeySwitchReq( void )
{
  SecNwkKeySwitchReq switchReq = SEC_NWK_KEY_SWITCH_REQ__INIT;
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;

  uiPrintf( "NwkMgr sendSecNwkKeySetReq\n" );
  
  switchReq.seqnum = gNmKeySeqNum;
  switchReq.dstaddr = 0xffff;

  len = sec_nwk_key_switch_req__get_packed_size( &switchReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = sec_nwk_key_switch_req__pack( &switchReq, pBuf );

    // Send protobuf packed request to API Client with synchronous status response
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__SEC_NWK_KEY_SWITCH_REQ, len, pBuf );

    free( pBuf );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendUnicastRouteReq
 *
 * @brief       Send a Unicast Route Request
 *
 * @param       addr - destination short address
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendUnicastRouteReq( uint16 addr )
{
  DevNwkRouteReq req = DEV_NWK_ROUTE_REQ__INIT;
  
  req.dstaddr = addr;
  req.radius = NWK_ROUTE_RADIUS;
  
  return sendNwkRouteReq( &req );
}

/**************************************************************************************************
 *
 * @fn          sendNwkRouteReq
 *
 * @brief       Send a Route Request
 *
 * @param       pRouteReq - pointer to command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
ZStatusValues sendNwkRouteReq( DevNwkRouteReq *pRouteReq )
{
  int len;
  uint8 *pBuf;
  ZStatusValues status = ZSTATUS_VALUES__ZMemError;

  uiPrintf( "NwkMgr MOT sendNwkRouteReq\n" );
  
  len = dev_nwk_route_req__get_packed_size( pRouteReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    len = dev_nwk_route_req__pack( pRouteReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__DEV_NWK_ROUTE_REQ, len, pBuf );

    free( pBuf );
  }
  
  return status;
}

/**************************************************************************************************
 *
 * @fn          sendNwkGetDeviceListCnf
 *
 * @brief       Send Get Device List Confirmation message
 *
 * @param       connection - tcp connection
 * @param       pGetDeviceListCnf - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendNwkGetDeviceListCnf( int connection, NwkGetDeviceListCnf *pGetDeviceListCnf )
{
  int len;
  uint8 *pBuf;
  
  uiPrintf("Sending Network Get Device List Confirmation\n" );
  
  len = nwk_get_device_list_cnf__get_packed_size( pGetDeviceListCnf );
  pBuf = malloc( len );
  if ( pBuf )
  { 
    nwk_get_device_list_cnf__pack( pGetDeviceListCnf, pBuf );
      
    // Send synchronous response back to app
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, FALSE, 
                   NWK_MGR_CMD_ID_T__NWK_GET_DEVICE_LIST_CNF, len, pBuf );

    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          sendNwkRemoveDeviceReq
 *
 * @brief       Send Remove Device Request message
 *
 * @param       pRemoveDeviceReq - command structure
 *
 * @return      ZStatusValues
 *
 **************************************************************************************************/
static ZStatusValues sendNwkRemoveDeviceReq( NwkRemoveDeviceReq *pRemoveDeviceReq )
{
  int len;
  uint8 *pBuf;
  uint16 shortAddr;
  ZStatusValues status = ZSTATUS_VALUES__ZFailure;
  SecApsRemoveReq removeReq = SEC_APS_REMOVE_REQ__INIT;
  uint64_t ieeeaddr;
  uint64_t parentLongAddr;
  uint16   parentShortAddr;
  uint8    capInfo = 0; // if we know nothing about it, assume ZED

  uiPrintf("Sending Remove Device Request \n" );
  
  if ( pRemoveDeviceReq->dstaddr->addresstype == NWK_ADDRESS_TYPE_T__UNICAST )
  {
    ieeeaddr = pRemoveDeviceReq->dstaddr->ieeeaddr;

    // SJS_119
    // will try to read capInfo. Defaults to router if can't
    nwkMgrDb_GetCapInfo( ieeeaddr, &capInfo );  
    
    // verify we know about the device
    if ( nwkMgrDb_GetShortAddr( ieeeaddr, &shortAddr ) )
    {
      if( capInfo & gCapInfo_Router_c )
      {
        removeReq.parentaddr = shortAddr;   // send to router, not parent
      }
      else if( nwkMgrDb_GetParentAddr( ieeeaddr, &parentLongAddr ) && nwkMgrDb_GetShortAddr( parentLongAddr, &parentShortAddr ) )
      {
        removeReq.parentaddr = parentShortAddr;
      }
      else
      {
        uiPrintf( "- parent not in database, using gateway for parent\n" );
        removeReq.parentaddr = gLocalDeviceInfo.nwkaddr;
      }
      removeReq.nwkaddr = shortAddr;
      removeReq.extaddr = pRemoveDeviceReq->dstaddr->ieeeaddr;
    }
    else
    {
      uiPrintf( "- Unknown device %016llX\n", ieeeaddr );
      return ZSTATUS_VALUES__ZNwkUnknownDevice;
    }
  }
  else
  {
      uiPrintf( "- Must be unicast\n" );
    return ZSTATUS_VALUES__ZNwkInvalidRequest;
  }
  
  len = sec_aps_remove_req__get_packed_size( &removeReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    sec_aps_remove_req__pack( &removeReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__SEC_APS_REMOVE_REQ, len, pBuf );

    free( pBuf );
  }
  
  return( status );
}

/**************************************************************************************************
 *
 * @fn          sendNwkZigbeeNwkInfoCnf
 *
 * @brief       Send Network Information Confirmation message
 *
 * @param       connection - tcp connection
 * @param       pNwkInfoCnf - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendNwkZigbeeNwkInfoCnf( int connection, NwkZigbeeNwkInfoCnf *pNwkInfoCnf )
{
  int len;
  uint8 *pBuf;
  
  len = nwk_zigbee_nwk_info_cnf__get_packed_size( pNwkInfoCnf );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_zigbee_nwk_info_cnf__pack( pNwkInfoCnf, pBuf );

    // Send synchronous response back to app
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, FALSE, 
                   NWK_MGR_CMD_ID_T__NWK_ZIGBEE_NWK_INFO_CNF, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          sendNwkGetNwkKeyCnf
 *
 * @brief       Send Get Network Key Confirmation message
 *
 * @param       connection - tcp connection
 * @param       pGetNwkKeyCnf - command structure
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendNwkGetNwkKeyCnf( int connection, NwkGetNwkKeyCnf *pGetNwkKeyCnf )
{
  int len;
  uint8 *pBuf;
  
  len = nwk_get_nwk_key_cnf__get_packed_size( pGetNwkKeyCnf );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_get_nwk_key_cnf__pack( pGetNwkKeyCnf, pBuf );

    // Send synchronous response back to app
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, FALSE, 
                   NWK_MGR_CMD_ID_T__NWK_GET_NWK_KEY_CNF, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          sendNwkGetNeighborTableRspInd
 *
 * @brief       send message
 *
 * @param       status    - NwkStatusT
 * @param       pTrans    - transaction table entry
 * @parm        pZdoMgmtLqiRspInd - pointer to LQI entries (NULL if none)
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendNwkGetNeighborTableRspInd( uint8 status, transTable_t *pTrans, ZdoMgmtLqiRspInd *pZdoMgmtLqiRspInd )
{
  NwkGetNeighborTableRspInd rspInd = NWK_GET_NEIGHBOR_TABLE_RSP_IND__INIT;
  NwkNeighborInfoT **ppNeighborlist = NULL;
  NwkNeighborInfoT *pNeighborlist = NULL;
  NeighborLqiItem **ppLqiList;    // from ZStack structure
  uint64_t ieeeaddr = 0;
  uint8 startindex;
  uint8 entries;
  int count;
  int i;
  int len;
  uint8 *pBuf;
  
  // nothing to do (should never happen)
  if(!pTrans)
  {
    return;
  }
    
  // no neighbors
  if(!pZdoMgmtLqiRspInd)
  {
    count = 0;
    ppLqiList = NULL;  
    startindex = 0;
    entries = 0;
  }
  
  // neighbors
  else
  {
    count = pZdoMgmtLqiRspInd->n_lqilist;
    ppLqiList = pZdoMgmtLqiRspInd->lqilist;
    startindex = pZdoMgmtLqiRspInd->startindex;
    entries = pZdoMgmtLqiRspInd->neighborlqientries;

    // allocate memory
    ppNeighborlist = malloc( count * sizeof(NwkNeighborInfoT *) );
    pNeighborlist  = malloc( count * sizeof(NwkNeighborInfoT) );
    
    // no memory, give up
    if(!ppNeighborlist || !pNeighborlist)
    {
      if( ppNeighborlist )
        free( ppNeighborlist );
      if( pNeighborlist )
        free( pNeighborlist );
      uiPrintf( "NoMem\n" );        
      return;
    }
  }

  uiPrintf( "NwkMgr sendNwkGetNeighborTableRspInd: count %d, startindex %d", count, startindex );
    
  // convert from ZStack neighbors to NwkMgr neighbors
  for(i = 0; i < count; ++i)
  {
    // initialize neighbor list
    ppNeighborlist[i] = &pNeighborlist[i];
    nwk_neighbor_info_t__init( &pNeighborlist[i] );    

    // local (gateway) device is not in the database
    if(ppLqiList[i]->nwkaddr == gLocalDeviceInfo.nwkaddr)
    {
      ieeeaddr = gLocalDeviceInfo.ieeeaddr;
    }

    // get ieeeaddr from database
    else
    {
      nwkMgrDb_GetIeeeAddr( (uint16)(ppLqiList[i]->nwkaddr), &ieeeaddr );
    }

    // fill out neighbor list
    pNeighborlist[i].extendedpanid = ppLqiList[i]->extendedpanid;
    pNeighborlist[i].extendedaddress = ieeeaddr;
    pNeighborlist[i].networkaddress = ppLqiList[i]->nwkaddr;
    pNeighborlist[i].devicetype = ppLqiList[i]->devicetype;
    pNeighborlist[i].idle = ppLqiList[i]->rxonwhenidle;
    pNeighborlist[i].relation = ppLqiList[i]->relationship;
    pNeighborlist[i].permitjoining = ppLqiList[i]->permit;
    pNeighborlist[i].depth = ppLqiList[i]->depth;
    pNeighborlist[i].lqi = ppLqiList[i]->lqi;
  }

  // fill in the response
  rspInd.sequencenumber = pTrans->sequence;
  rspInd.status = status;
  rspInd.srcaddr = &pTrans->addr;
  rspInd.neighbortableentries = entries;
  rspInd.startindex = startindex;
  rspInd.n_neighborlist = count;
  rspInd.neighborlist = ppNeighborlist;

  uiPrintf( "  sequencenumber %d\n", rspInd.sequencenumber );
  uiPrintf( "  status %d\n", rspInd.status );
  uiPrintf( "  neighbortableentries %d\n", rspInd.neighbortableentries );
  uiPrintf( "  startindex %d\n", rspInd.startindex );
  uiPrintf( "  count %d\n", rspInd.n_neighborlist );
  for(i = 0; i < count; ++i)
  {
    uiPrintf( "    extendedpanid 0x%016llX\n", rspInd.neighborlist[i]->extendedpanid );
    uiPrintf( "    ieeeaddr 0x%016llX\n", rspInd.neighborlist[i]->extendedaddress );
    uiPrintf( "    nwkaddr %04X\n", rspInd.neighborlist[i]->networkaddress );
  }

  // send to App
  len = nwk_get_neighbor_table_rsp_ind__get_packed_size( &rspInd );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_get_neighbor_table_rsp_ind__pack( &rspInd, pBuf );

    // Send response back to app
    APIS_SendData( pTrans->connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, TRUE, 
                   NWK_MGR_CMD_ID_T__NWK_GET_NEIGHBOR_TABLE_RSP_IND, len, pBuf );
    
    free( pBuf );
  }

  // done, free the structure
  free( ppNeighborlist );
  free( pNeighborlist );
}

/**************************************************************************************************
 *
 * @fn          sendNwkGetRoutingTableRspInd
 *
 * @brief       send message
 *
 * @param       status    - NwkStatusT
 * @param       pTrans    - transaction table entry
 * @parm        pZdoMgmtRtgRspInd - pointer to RTG entries (NULL if none)
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendNwkGetRoutingTableRspInd( uint8 status, transTable_t *pTrans, ZdoMgmtRtgRspInd *pZdoMgmtRtgRspInd )
{
  NwkGetRoutingTableRspInd rspInd = NWK_GET_ROUTING_TABLE_RSP_IND__INIT;
  NwkRoutingInfoT **ppRoutingInfo = NULL;
  NwkRoutingInfoT *pRoutingInfo = NULL;
  RtgItem **ppRtgList;  // from ZStack
  int count;
  int i;
  uint8 startindex;
  uint8 entries;
  int len;
  uint8 *pBuf;

  // nothing to do
  if(!pTrans)
    return;

  // no routing info (timeout)
  if(!pZdoMgmtRtgRspInd)
  {
    count = 0;
    startindex = 0;
    entries = 0;
  }

  // routing info
  else
  {
    count = pZdoMgmtRtgRspInd->n_rtglist;
    startindex = pZdoMgmtRtgRspInd->startindex;
    entries = pZdoMgmtRtgRspInd->rtgentries;

    // allocate memory
    ppRoutingInfo = malloc(count * sizeof(NwkRoutingInfoT *));
    pRoutingInfo  = malloc(count * sizeof(NwkRoutingInfoT));
    
    // no memory, give up
    if(!ppRoutingInfo || !pRoutingInfo)
    {
      if( ppRoutingInfo )
        free( ppRoutingInfo );
      if( pRoutingInfo )
        free( pRoutingInfo );
      uiPrintf( "NoMem\n" );        
      return;
    }
  }

  uiPrintf( "NwkMgr sendNwkGetRoutingTableRspInd: count %d, startindex %d", count, startindex );
    
  // convert from ZStack routing info to NwkMgr routing info
  if( pZdoMgmtRtgRspInd )
  {
    ppRtgList = pZdoMgmtRtgRspInd->rtglist;
  }
  for(i = 0; i < count; ++i)
  {
    ppRoutingInfo[i] = &pRoutingInfo[i];

    nwk_routing_info_t__init( &pRoutingInfo[i] );    

    pRoutingInfo[i].dstaddr = ppRtgList[i]->dstaddr;
    pRoutingInfo[i].status  = ppRtgList[i]->status;
    pRoutingInfo[i].nexthop = ppRtgList[i]->nexthop;
  }

  // fill in response
  rspInd.sequencenumber = pTrans->sequence;
  rspInd.status = status;
  rspInd.srcaddr = &pTrans->addr;
  rspInd.routingtableentries = entries;
  rspInd.startindex = startindex;
  rspInd.n_routinglist = count;
  rspInd.routinglist = ppRoutingInfo;

  // send response to app
  len = nwk_get_routing_table_rsp_ind__get_packed_size( &rspInd );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_get_routing_table_rsp_ind__pack( &rspInd, pBuf );

    // Send response back to app
    APIS_SendData( pTrans->connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, TRUE, 
                   NWK_MGR_CMD_ID_T__NWK_GET_ROUTING_TABLE_RSP_IND, len, pBuf );
    
    free( pBuf );
  }

  // done, free the routing structure
  free( ppRoutingInfo );
  free( pRoutingInfo );
}

/**************************************************************************************************
 *
 * @fn          sendNwkSetBindingEntryRspInd
 *
 * @brief       send message
 *
 * @param       connection  - tcp connection
 * @parm        sequence    - app sequence #
 * @param       status      - NwkStatusT
 * @param       srcaddr     - source address
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendNwkSetBindingEntryRspInd( int connection, uint16 sequence, uint32_t status, NwkAddressStructT *srcaddr )
{
  NwkSetBindingEntryRspInd rspInd = NWK_SET_BINDING_ENTRY_RSP_IND__INIT;
  int len;
  uint8 *pBuf;

  // fill in response structure
  rspInd.sequencenumber = sequence;
  rspInd.status = status;
  rspInd.srcaddr = srcaddr;

  len = nwk_set_binding_entry_rsp_ind__get_packed_size( &rspInd );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_set_binding_entry_rsp_ind__pack( &rspInd, pBuf );

    // Send response back to app
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, TRUE, 
                   NWK_MGR_CMD_ID_T__NWK_SET_BINDING_ENTRY_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 *
 * @fn          sendNwkGetBindingTableRspInd
 *
 * @brief       send message
 *
 * @param       connection  - tcp connection
 * @parm        sequence    - app sequence #
 * @param       status      - NwkStatusT
 * @param       srcaddr     - source address
 *
 * @return      none
 *
 **************************************************************************************************/
static void sendNwkGetBindingTableRspInd( int connection, uint8 status, ZdoMgmtBindRspInd *pInMsg )
{
  NwkGetBindingTableRspInd rspInd = NWK_GET_BINDING_TABLE_RSP_IND__INIT;
  int len;
  uint8 *pBuf;

  if(status == NWK_STATUS_T__STATUS_SUCCESS)
  {
	rspInd.status = pInMsg->status;
	rspInd.srcaddr = pInMsg->srcaddr;
    rspInd.bindentries = pInMsg->bindentries;
    rspInd.startindex = pInMsg->startindex;
    rspInd.n_bindlist = pInMsg->n_bindlist;
    rspInd.bindlist = (NwkBindItem**)pInMsg->bindlist;
  }
  else if(status == NWK_STATUS_T__STATUS_TIMEOUT)
  {
    //uiPrintfEx(trERROR,"TIMEOUT");
	rspInd.status = 1;
    rspInd.srcaddr = 0;
    rspInd.bindentries = 0;
    rspInd.startindex = 0;
	rspInd.n_bindlist = (size_t) 0;
    rspInd.bindlist = NULL;
  }
  else
  {
    rspInd.status = 1;
    rspInd.srcaddr = 0;
    rspInd.bindentries = 0;
    rspInd.startindex = 0;
	  rspInd.n_bindlist = (size_t) 0;
    rspInd.bindlist = NULL;
  }
    
  len = nwk_get_binding_table_rsp_ind__get_packed_size(&rspInd);
  
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_get_binding_table_rsp_ind__pack( &rspInd, pBuf );

    // Send response back to app
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, TRUE, 
                   NWK_MGR_CMD_ID_T__NWK_GET_BINDING_TABLE_RSP_IND, len, pBuf );
    
    free( pBuf );
  }
  
}

/**************************************************************************************************
 * @fn          sendNwkZigBeeSystemResetCnf
 *
 * @brief       sends a confirm
 *
 * @param       connection  - tcp connection
 * @param       pCnf        - confirm structure
 *
 * @return      none
 **************************************************************************************************/
static void sendNwkZigBeeSystemResetCnf( int connection, NwkZigbeeSystemResetCnf *pCnf )
{
  int len;
  uint8 *pBuf;
  len = nwk_zigbee_system_reset_cnf__get_packed_size( pCnf );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_zigbee_system_reset_cnf__pack( pCnf, pBuf );

    // Send response back to app (asynchronous data)
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, TRUE,
                   NWK_MGR_CMD_ID_T__NWK_ZIGBEE_SYSTEM_RESET_CNF, len, pBuf );
    
    free( pBuf );
  }
}

/**************************************************************************************************
 * @fn          sendNwkSetZigbeePowerModeCnf
 *
 * @brief       sends a confirm
 *
 * @param       connection  - tcp connection
 * @param       pCnf        - confirm structure
 *
 * @return      none
 **************************************************************************************************/
static void sendNwkSetZigbeePowerModeCnf( int connection, NwkSetZigbeePowerModeCnf *pCnf )
{
  int len;
  uint8 *pBuf;
  
  len = nwk_set_zigbee_power_mode_cnf__get_packed_size( pCnf );
  pBuf = malloc( len );
  if ( pBuf )
  {
    nwk_set_zigbee_power_mode_cnf__pack( pCnf, pBuf );

    // Send response back to app (asynchronous data)
    APIS_SendData( connection, Z_STACK_NWK_MGR_SYS_ID_T__RPC_SYS_PB_NWK_MGR, TRUE,
                   NWK_MGR_CMD_ID_T__NWK_SET_ZIGBEE_POWER_MODE_CNF, len, pBuf );
    
    free( pBuf );
  }
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
      uiPrintfEx(trUNMASKABLE, " got a 'q', quitting\n");
      return;
    }
    else if (key < 0) 
    {
      // stdin is not open, pause until we get a signal
      // note that we get a signal when we switch between FG & BG
      pause();
    }
    else 
    {
       ; // uiPrintfEx(trUNMASKABLE, " ignoring key '%x' (%c)\n",key,key);
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
 * @fn          nmRegCallbacks
 *
 * @brief       Send message to register for ZDO callbacks.
 *
 * @param       none
 *
 * @return      TRUE for success, FALSE for failure
 *
 **************************************************************************************************/
static bool nmRegCallbacks( void )
{
  bool rc = TRUE;
  int len;
  uint8 *pBuf;
  ZStatusValues status;
  DevZDOCBReq cbReq = DEV_ZDOCBREQ__INIT;

  uiPrintf( "NwkMgr: Sending Device ZDO CB Request\n" );

  cbReq.cmdid = (ZStackCmdIDs) ZSTACK_CMD_IDS__DEV_ZDO_CBS_REQ;

  // Register for all ZDO callbacks
  cbReq.has_srcrtgindcb = TRUE;
  cbReq.srcrtgindcb = TRUE;
  cbReq.has_concentratorindcb = TRUE;
  cbReq.concentratorindcb = TRUE;
  cbReq.has_nwkdisccnfcb = TRUE;
  cbReq.nwkdisccnfcb = TRUE;
  cbReq.has_beaconnotindcb = TRUE;
  cbReq.beaconnotindcb = TRUE;
  cbReq.has_joincnfcb = TRUE;
  cbReq.joincnfcb = TRUE;
  cbReq.has_leavecnfcb = TRUE;
  cbReq.leavecnfcb = TRUE;
  cbReq.has_leaveindcb = TRUE;
  cbReq.leaveindcb = TRUE;
  cbReq.has_nwkaddrrsp = TRUE;
  cbReq.nwkaddrrsp = TRUE;
  cbReq.has_ieeeaddrrsp = TRUE;
  cbReq.ieeeaddrrsp = TRUE;
  cbReq.has_nodedescrsp = TRUE;
  cbReq.nodedescrsp = TRUE;
  cbReq.has_powerdescrsp = TRUE;
  cbReq.powerdescrsp = TRUE;
  cbReq.has_simpledescrsp = TRUE;
  cbReq.simpledescrsp = TRUE;
  cbReq.has_activeendpointrsp = TRUE;
  cbReq.activeendpointrsp = TRUE;
  cbReq.has_matchdescrsp = TRUE;
  cbReq.matchdescrsp = TRUE;
  cbReq.has_complexdescrsp = TRUE;
  cbReq.complexdescrsp = TRUE;
  cbReq.has_userdescrsp = TRUE;
  cbReq.userdescrsp = TRUE;
  cbReq.has_discoverycachersp = TRUE;
  cbReq.discoverycachersp = TRUE;
  cbReq.has_userdesccnf = TRUE;
  cbReq.userdesccnf = TRUE;
  cbReq.has_serverdiscoveryrsp = TRUE;
  cbReq.serverdiscoveryrsp = TRUE;
  cbReq.has_enddevicetimeoutrsp = TRUE;
  cbReq.enddevicetimeoutrsp = TRUE;
  cbReq.has_bindrsp = TRUE;
  cbReq.bindrsp = TRUE;
  cbReq.has_enddevicebindrsp = TRUE;
  cbReq.enddevicebindrsp = TRUE;
  cbReq.has_unbindrsp = TRUE;
  cbReq.unbindrsp = TRUE;
  cbReq.has_mgmtnwkdiscrsp = TRUE;
  cbReq.mgmtnwkdiscrsp = TRUE;
  cbReq.has_mgmtlqirsp = TRUE;
  cbReq.mgmtlqirsp = TRUE;
  cbReq.has_mgmtrtgrsp = TRUE;
  cbReq.mgmtrtgrsp = TRUE;
  cbReq.has_mgmtbindrsp = TRUE;
  cbReq.mgmtbindrsp = TRUE;
  cbReq.has_mgmtleaversp = TRUE;
  cbReq.mgmtleaversp = TRUE;
  cbReq.has_mgmtdirectjoinrsp = TRUE;
  cbReq.mgmtdirectjoinrsp = TRUE;
  cbReq.has_mgmtpermitjoinrsp = TRUE;
  cbReq.mgmtpermitjoinrsp = TRUE;
  cbReq.has_mgmtnwkupdatenotify = TRUE;
  cbReq.mgmtnwkupdatenotify = TRUE;
  cbReq.has_deviceannounce = TRUE;
  cbReq.deviceannounce = TRUE;
  cbReq.has_devstatechange = TRUE;
  cbReq.devstatechange = TRUE;
  cbReq.has_tcdeviceind = TRUE;
  cbReq.tcdeviceind = TRUE;
  
  len = dev_zdocbreq__get_packed_size( &cbReq );
  pBuf = malloc( len );
  if ( pBuf )
  {
    dev_zdocbreq__pack( &cbReq, pBuf );

    // Send protobuf packed request to API Client synchronously
    status = sendAPICExpectDefaultStatus( ZSTACK_CMD_IDS__DEV_ZDO_CBS_REQ, len, pBuf );
    
    if ( status == ZSTATUS_VALUES__ZSuccess )
    {
      uiPrintf( "ZDO Callback Register Response Successful\n" );
    }
    else
    {
      uiPrintf( "ZDO Callback Register Unsuccessful: %d \n", status );
      
      rc = FALSE;
    }
    
    free( pBuf );
  }
  else
  {
    rc = FALSE;
  }
  
  return rc;
}

/**************************************************************************************************
 *
 * @fn          allocPbDeviceInfoList
 *
 * @brief       Allocate and convert device information from the database to protobuf device information
 *
 * @param       count - Number of sNwkMgrDb_DeviceInfo_t structures in the array
 * @param       pInDeviceInfo - pointer to device information entry in database form
 *
 * @return      pointer to "count" structures converted to NwkDeviceInfoT
 *
 **************************************************************************************************/
NwkDeviceInfoT *allocPbDeviceInfoList( int count, sNwkMgrDb_DeviceInfo_t *pInDeviceInfo )
{
  uint8 i;
  uint8 j;
  uint8 k;
  int countEndpoints = 0;   // total # of endpoints, all devices
  int countClusters = 0;    // total # of clusters, all devices
  uint32 *pClusters = NULL;
  NwkDeviceInfoT *pOutDeviceInfo = NULL;
  NwkDeviceInfoT *pInitialDeviceInfoAddr = NULL;
  NwkSimpleDescriptorT *pSimpleDesc = NULL;      // array of simple descriptors
  NwkSimpleDescriptorT **ppSimpleDesc = NULL;    // array of simple descriptor pointers

  // Determine total number of endpoints and clusters
  for ( i = 0; i < count; i++ )
  {    
    countEndpoints += pInDeviceInfo[i].endpointCount;
    
    for ( j = 0; j < pInDeviceInfo[i].endpointCount; j++ )
    {
      countClusters += pInDeviceInfo[i].aEndpoint[j].inputClusterCount;
      countClusters += pInDeviceInfo[i].aEndpoint[j].outputClusterCount;
    }
  }

  // Allocate entire device information list pointer
  if (count != 0)
  {
    pOutDeviceInfo = malloc( count * sizeof( NwkDeviceInfoT ) );

    if ( !pOutDeviceInfo )
    {
      return NULL;
    }
  }

  // Allocate array of simple descriptor pointers
  if (countEndpoints != 0)
  {
    ppSimpleDesc = malloc( countEndpoints * sizeof( NwkSimpleDescriptorT * ) );

    if ( !ppSimpleDesc )
    {
      free( pOutDeviceInfo );
      return NULL;
    }
  
    // Allocate entire simple descriptor list pointer
    pSimpleDesc = malloc( countEndpoints * sizeof( NwkSimpleDescriptorT ) );
    if ( !pSimpleDesc )
    {
      free( ppSimpleDesc );
      free( pOutDeviceInfo );
      return NULL;
    }
  }
  
  if (countClusters != 0)
  {
    // Allocate entire input/output cluster list pointer
    pClusters = malloc( countClusters * sizeof( uint32 ) );
    if ( !pClusters )
    {
      if (NULL != pSimpleDesc) free( pSimpleDesc );
      if (NULL != ppSimpleDesc) free( ppSimpleDesc );
      free( pOutDeviceInfo );
      return NULL;
    }
  }
  
  // Store pointer address for first device info item, return when done
  pInitialDeviceInfoAddr = pOutDeviceInfo;
  for ( i = 0; i < count; i++ )
  {
    nwk_device_info_t__init( pOutDeviceInfo );
    
    pOutDeviceInfo->networkaddress = pInDeviceInfo[i].nwkAddr;
    pOutDeviceInfo->ieeeaddress = pInDeviceInfo[i].ieeeAddr;
    pOutDeviceInfo->manufacturerid = pInDeviceInfo[i].manufacturerId;
    pOutDeviceInfo->has_parentieeeaddress = TRUE;
    pOutDeviceInfo->parentieeeaddress = pInDeviceInfo[i].parentAddr;
    pOutDeviceInfo->n_simpledesclist = pInDeviceInfo[i].endpointCount;
    pOutDeviceInfo->simpledesclist = ppSimpleDesc;
    
    pOutDeviceInfo->devicestatus = ( (pInDeviceInfo[i].status & devInfoFlags_OnLine_c) ? 
      NWK_DEVICE_STATUS_T__DEVICE_ON_LINE : NWK_DEVICE_STATUS_T__DEVICE_OFF_LINE );
    
    for ( j = 0; j < pInDeviceInfo[i].endpointCount; j++ )
    {
      nwk_simple_descriptor_t__init( pSimpleDesc );
      
      *ppSimpleDesc++ = pSimpleDesc;
      
      pSimpleDesc->endpointid = pInDeviceInfo[i].aEndpoint[j].endpointId;
      pSimpleDesc->profileid = pInDeviceInfo[i].aEndpoint[j].profileId;
      pSimpleDesc->deviceid = pInDeviceInfo[i].aEndpoint[j].deviceId;
      pSimpleDesc->devicever = pInDeviceInfo[i].aEndpoint[j].deviceVer;
      pSimpleDesc->n_inputclusters = pInDeviceInfo[i].aEndpoint[j].inputClusterCount;
      pSimpleDesc->inputclusters = pClusters;
      
      for ( k = 0; k < pInDeviceInfo[i].aEndpoint[j].inputClusterCount; k++ )
      {
        *pClusters++ = pInDeviceInfo[i].aEndpoint[j].inputClusters[k];
      }
      
      pSimpleDesc->n_outputclusters = pInDeviceInfo[i].aEndpoint[j].outputClusterCount;
      pSimpleDesc->outputclusters = pClusters;
      
      for ( k = 0; k < pInDeviceInfo[i].aEndpoint[j].outputClusterCount; k++ )
      {
        *pClusters++ = pInDeviceInfo[i].aEndpoint[j].outputClusters[k];
      }
      
      pSimpleDesc++;
    }
    
    pOutDeviceInfo++;
  }
  
  return pInitialDeviceInfoAddr;
}

/**************************************************************************************************
 *
 * @fn          freePbDeviceInfoList
 *
 * @brief       Free allocated device information list
 *
 * @param       count - number of device lists
 * @param       pDeviceInfo - pointer to device information entry
 *
 * @return      none
 *
 **************************************************************************************************/
void freePbDeviceInfoList( int count, NwkDeviceInfoT *pDeviceInfo )
{  

  uiPrintfEx(trCUSTOM, "GUN freePbDeviceInfoList entered with pDeviceInfo = 0x%x\n", pDeviceInfo); 

  if (NULL == pDeviceInfo)
  {
    uiPrintfEx(trERROR, "freePbDeviceInfoList: Received bad pDeviceInfo = 0x%x\n", pDeviceInfo);
    return;
  }

  if (NULL == pDeviceInfo->simpledesclist)
  {
    free(pDeviceInfo);
    return;
  }

  uiPrintfEx(trDEBUG, "Freeing inputclusters 0x%x\n", pDeviceInfo->simpledesclist[0]->inputclusters);
  if ( pDeviceInfo->simpledesclist[0]->inputclusters )
  {
    free( pDeviceInfo->simpledesclist[0]->inputclusters );
  }
  else if ( pDeviceInfo->simpledesclist[0]->outputclusters )
  {
    free( pDeviceInfo->simpledesclist[0]->outputclusters );
  }
  
  free( *pDeviceInfo->simpledesclist );
  
  free( pDeviceInfo->simpledesclist );
  
  free( pDeviceInfo );
}

/**************************************************************************************************
 *
 * @fn          nwkSrvAddDeviceTcInfoCB
 *
 * @brief       Called when adding device from device announce, generates a device indications
 *
 * @param       appSeq - application sequence number
 * @param       pDeviceInfo - passed device information pointer
 * @param       err - state machine error codes
 *
 * @return      none
 *
 **************************************************************************************************/
void nwkSrvAddDeviceTcInfoCB( uint16 appSeq, sNwkMgrDb_DeviceInfo_t *pDeviceInfo, int err )
{
  NwkZigbeeDeviceInd deviceInd = NWK_ZIGBEE_DEVICE_IND__INIT;
  NwkDeviceInfoT *pPbDeviceInfo;
  bool worked;
  
  if ( err == NS_ADS_OK )
  {
    uiPrintfEx(trDEBUG, "AddDevice: Adding 0x%016llX to database... ", pDeviceInfo->ieeeAddr);

    // add the device to the database
    worked = nwkMgrDb_AddDevice( pDeviceInfo );
    if( worked )
    {
      uiPrintfEx(trDEBUG, "Added\n" );
    }
    else
    {
      uiPrintfEx(trDEBUG, "Failed to Add\n" );
    }
    uiPrintfEx(trDEBUG, "- AddDevice Complete on 0x%016llX\n", pDeviceInfo->ieeeAddr );

    // send DeviceInd to the app
    pPbDeviceInfo = allocPbDeviceInfoList( 1, pDeviceInfo );
    if ( pPbDeviceInfo )
    {
      deviceInd.deviceinfo = pPbDeviceInfo;
      
      // Send device indication
      sendNwkZigbeeDeviceInd( &deviceInd );

      
      // Free allocated device info
      freePbDeviceInfoList( 1, pPbDeviceInfo );
    }
  }

  // failed to add device on TC Info
  else
  {
    if( pDeviceInfo )
    {
      uiPrintfEx(trDEBUG, "- AddDevice Failed %d on %016llX\n", err, pDeviceInfo->ieeeAddr );
    }
    else
    {
      uiPrintfEx(trDEBUG, "- AddDevice Failed %d\n", err );
    }
  }
}

/**************************************************************************************************
 *
 * @fn          nmHandleCnfRsp
 *
 * @brief       Sends the appropriate type of confirm, and response (if syncrhonous). Does NOT
 *              post to the transaction table (that's up to the caller).
 *
 * @param       rspType - NM_RSP_NONE, NM_RSP_GENERIC, NM_RSP_SPECIFIC
 * @param       addrType - address mode type
 * @param       connection - tcp connection, used for sending response
 * @param       cmdId - requesting command ID
 * @param       status - status used for generic confirmation
 *
 * @return      returns app cnf sequence # (0xffff = self or none)
 *
 **************************************************************************************************/
static uint16 nmHandleCnfRsp( int connection, uint8 rspType, uint8 addrType, uint8 status )
{
  NwkZigbeeGenericCnf genericCnf = NWK_ZIGBEE_GENERIC_CNF__INIT;
  bool hasSequence = FALSE;
  bool hasConfirm = TRUE;
  uint16 sequence = 0xFFFF; // for self

  // set up status field
  genericCnf.status = ((status == ZSTATUS_VALUES__ZSuccess) ? NWK_STATUS_T__STATUS_SUCCESS : NWK_STATUS_T__STATUS_FAILURE);

  // no response expected, just a confirm
  if ( rspType == NM_RSP_NONE )
  {
    // hasConfirm, but not hasSequence
  }
  else if ( rspType == NM_RSP_GENERIC ) // a generic response is expected
  {
    // unicast will response on the data confirm
    if ( addrType == NWK_ADDRESS_TYPE_T__UNICAST )
    {
      sequence = gNmAppTransSeqNum++;
      if( gNmAppTransSeqNum == 0xffff ) // 0xffff reserved for self
      {
        gNmAppTransSeqNum = 0;
      }
      hasSequence = TRUE;
    }
    else  // no sequence # for self, broadcast or groupcast
    {
      hasSequence = FALSE;
    }
  }
  else if ( rspType == NM_RSP_SPECIFIC )  // a specific response is expected
  {
    // self, no confirm
    if ( addrType == NWK_ADDRESS_TYPE_T__SELF )
    {
      hasConfirm = FALSE;
    }

    if ( addrType == NWK_ADDRESS_TYPE_T__UNICAST )   // unicast will response on the data confirm
    {
      sequence = gNmAppTransSeqNum++;
      if( gNmAppTransSeqNum == 0xffff)  // 0xffff reserved for self
      {
        gNmAppTransSeqNum = 0;
      }
      hasSequence = TRUE;
    }
    else  // no sequence # for self, broadcast or groupcast
    {
      hasSequence = FALSE;
    }
  }
  
  // send the confirm
  if ( hasConfirm )
  {
    if( status != ZSTATUS_VALUES__ZSuccess )
      hasSequence = FALSE;

    genericCnf.has_sequencenumber = hasSequence;
    
    if ( hasSequence )
    {
      genericCnf.sequencenumber = sequence;
    }
    
    sendZbGenericCnf( connection, &genericCnf );
  }
  
  return sequence;  // 0xffff for self or none
}

/**************************************************************************************************
 *
 * @fn          nmTransPost
 *
 * @brief       Insert new transaction entry in gateway transaction table
 *
 * @param       connection - connection handle (tcp)
 * @param       appRsp - command ID for application rsp
 * @param       sequence - application transaction ID
 * @param       pAddr - address (ieeeaddr, ep)
 * @param       zdoRsp - response from ZDP to trigger the response
 *
 * @return      returns TRUE if worked, FALSE if failed.
 *
 **************************************************************************************************/
static uint8 nmTransPost( int connection, uint8 appRsp, uint16 sequence, NwkAddressStructT *pAddr, uint8 zdoRsp, uint16 srcaddr )
{
  transTable_t *pNewTransTable;
  int i;

  while(1)
  {
    for ( i = 0; i < giTransTableEntries ; i++ )
    {
      if ( gpTransTable[i].inUse != TRUE )
      {
        // set up table
        gpTransTable[i].inUse = TRUE;
        gpTransTable[i].timeout = TRANSACTION_TIMEOUT;

        // field for sending back to the app
        gpTransTable[i].connection = connection;
        gpTransTable[i].appRsp = appRsp;
        gpTransTable[i].sequence = sequence;

        // remember address structure
        gpTransTable[i].addr = *pAddr;

        // fields for detecting when ZDP responds
        gpTransTable[i].zdoRsp = zdoRsp;
        gpTransTable[i].srcaddr = srcaddr;    

        return TRUE;
      }
    }
    
    // not found, get a larger transaction table
    pNewTransTable = malloc( (giTransTableEntries + ADD_TRANS_TABLE_SIZE) * sizeof(transTable_t) );
    if(!pNewTransTable)
    {
      break;
    }
    memset( pNewTransTable, 0, (giTransTableEntries + ADD_TRANS_TABLE_SIZE) * sizeof(transTable_t) );
    if(giTransTableEntries)
    {
      memcpy( pNewTransTable, gpTransTable, giTransTableEntries * sizeof(transTable_t) );
    }
    giTransTableEntries += ADD_TRANS_TABLE_SIZE;
    if(gpTransTable)
    {
      free(gpTransTable);
    }
    gpTransTable = pNewTransTable;

    uiPrintfEx(trDEBUG,"Added %d entries, total %d\n", ADD_TRANS_TABLE_SIZE, giTransTableEntries);
  }

  return FALSE;
}

/**************************************************************************************************
 *
 * @fn          nmUiPrintData
 *
 * @brief       print out a binary data structure
 *
 * @param       pBinData - binary data
 *
 * @return      none
 *
 **************************************************************************************************/
static void nmUiPrintData( ProtobufCBinaryData *pBinData )
{
  int i;

  uiPrintf( "len %d, data ", pBinData->len );
  for(i = 0; i < pBinData->len; ++i)
  {
    if(i)
      uiPrintf(":");
    uiPrintf( "%02X", pBinData->data[i]);
  }
}

/**************************************************************************************************
 *
 * @fn          nmFindTransactionByZdoRsp
 *
 * @brief       Retrieve transaction entry in network manager transaction table
 *
 * @param       zdoRsp - Response code from the zdo rsp
 * @param       srcaddr - source address from the zdo rsp
 *
 * @return      pointer to transaction table entry, or NULL if not found
 *
 **************************************************************************************************/
static void nmTransFreeEntry(  transTable_t *pTrans )
{  
  uiPrintfEx(trDEBUG, "nmTransFreed: srcaddr %04X\n", pTrans->srcaddr );

  pTrans->inUse = FALSE;
}

/**************************************************************************************************
 *
 * @fn          nmFindTransactionByZdoRsp
 *
 * @brief       Retrieve transaction entry in network manager transaction table
 *
 * @param       zdoRsp - Response code from the zdo rsp
 * @param       srcaddr - source address from the zdo rsp
 *
 * @return      pointer to transaction table entry, or NULL if not found
 *
 **************************************************************************************************/
static transTable_t *nmTransFindByZdoRsp( uint8 zdoRsp, uint16 srcaddr )
{
  int i;
  
  for ( i = 0; i < giTransTableEntries; i++ )
  {
    if ( ( gpTransTable[i].inUse == TRUE ) && ( zdoRsp == gpTransTable[i].zdoRsp ) && 
         ( gpTransTable[i].srcaddr == srcaddr ) )
    {
      // update retry table, if necessary
      nmUpdateDeviceInRetryTable( gpTransTable[i].addr.ieeeaddr );
      
      return ( &gpTransTable[i] );
    }
  }
  
  return NULL;  // transaction table entry not found
}

