/*********************************************************************
 Filename:       main.c
 Revised:        $Date$
 Revision:       $Revision$

 Description:    Main loop and initialization


 Copyright 2013 -2014 Texas Instruments Incorporated. All rights reserved.

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
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <time.h>
#include <errno.h>

#include "hal_types.h"
#include "api_client.h"
#include "hal_rpc.h"
#include "api_server.h"
#include "configparser.h"
#include "trace.h"

/*********************************************************************
 * Constant
 *********************************************************************/

#if defined ( NPI_VERSION )
#define DISPLAY_VERSION  TRUE     // Yes, display NPI version information
#else
#define DISPLAY_VERSION  FALSE    // Don't display the NPI Server's version
#endif

#define CALL_STACK_TRACE_DEPTH 10

/*********************************************************************
 * Typedefs
 *********************************************************************/

/*********************************************************************
 * Locals
 *********************************************************************/
apisSysParams_t *pAPIS_SysParams = NULL;
static uint32_t parent_process_id = 0;
	

/*********************************************************************
 * Function Prototypes
 *********************************************************************/
void segmentation_fault_handler( int signum, siginfo_t *info, void *context );
void register_segmentation_fault_handler( void );
void unregister_segmentation_fault_handler( void );
int processUint32Flag(const char * flagLong, const char * flagShort, 
                      uint32_t * value, int * argc_p, char *argv[]);

/*********************************************************************
 *********************************************************************/

int main( int argc, char *argv[] )
{
  int exitCode = 0;
  
  processUint32Flag("--verbose", "-v", &default_trace_enable_mask, &argc, argv);
  processUint32Flag("--parent_pid", "-p", &parent_process_id, &argc, argv);

  trace_init_main("MAIN");

  register_segmentation_fault_handler();

  // Seed the random number generator with the current time
  srand( time( NULL ) );

  exitCode = appArgs( &argc, &argv );

  // Parse the command line parameters
  if ((exitCode == 0) && (argc == 1))
  {
    uiPrintf( "\nUsage:\n\n" );uiPrintf( "  %s ipaddr:port config.ini\n\n", argv[0] );
    uiPrintf( "Example:\n\n" );uiPrintf( "  $ %s 192.168.0.1:2531 config.ini\n\n", argv[0] );
    exitCode = -1;
  }

  if (exitCode == 0)
  {
    pAPIS_SysParams = appInit();
    if ( pAPIS_SysParams == NULL )
    {
      uiPrintf( "\nApp initialization problem - exiting!!\n\n" );
      exitCode = -1;
    }
  }
  
  if ((exitCode == 0) && (argc > (pAPIS_SysParams->numClients + 1)))
  {
    if ( pAPIS_SysParams->pConfigDesc )
    {
      if ( parseConfigFile( (char *) argv[(pAPIS_SysParams->numClients + 1)],
          pAPIS_SysParams->pConfigDesc, pAPIS_SysParams->numConfigDescs ) != 0 )
      {
        uiPrintf( "\nConfig File Error (%s) - exiting!!\n\n", argv[(pAPIS_SysParams->numClients+1)] );
        exitCode = -1;
      }
    }
  }

  if (exitCode == 0)
  {
    exitCode = appInitPhase2();
  }

  if (exitCode == 0)
  {
    // Setup the API Client interface
    apicHandle_t handles[pAPIS_SysParams->numClients];
    size_t i;

    for ( i = 0; i < pAPIS_SysParams->numClients; i++ )
    {
      handles[i] = apicInit( argv[1 + i], // the IP address and port to a TCP Server
          DISPLAY_VERSION, pAPIS_SysParams->pfNPICB );
    }

    exitCode = appInitPhase3(handles);
  }
  
  if (exitCode == 0)
  {
    // Setup the API Server
    APIS_Init( pAPIS_SysParams->port, pAPIS_SysParams->serverVerbose,
      pAPIS_SysParams->pfServerCB );

    exitCode = appInitPhase4();
  }
  
  if (exitCode == 0)
  {
    //signal the calling script that the server is ready to receive client connections
    if (parent_process_id != 0) //todo: make sure 0 is not a valid PID - done. verified with Rob
    {
      int rc;

      rc = kill(parent_process_id, SIGUSR1);

      if (rc != 0)
      {
        uiPrintfEx(trERROR, "\nsending signal %d to PID %d failed: rc=%d, errno=%d\n\n", SIGUSR1, parent_process_id, rc, errno);
      }
    }

    // Pass control to the application
    exitCode = appMain();  // No Return from here
  }
  
  unregister_segmentation_fault_handler();

  trace_deinit();

  exit( exitCode );
}

/*********************************************************************
 *
 * @fn          segmentation_fault_handler
 *
 * @brief       This handles segmentation faults by printing error
 *
 *********************************************************************/
void segmentation_fault_handler( int signum, siginfo_t *info, void *context )
{
  void *array[CALL_STACK_TRACE_DEPTH];
  size_t size;

  uiPrintfEx(trERROR, "ERROR: signal %d was trigerred:\n", signum );

  uiPrintfEx(trERROR, "  Fault address: %p\n", info->si_addr );

  switch ( info->si_code )
  {
    case SEGV_MAPERR:
      uiPrintfEx(trERROR, "  Fault reason: address not mapped to object\n" );
      break;

    case SEGV_ACCERR:
      uiPrintfEx(trERROR,
          "  Fault reason: invalid permissions for mapped object\n" );
      break;

    default:
      uiPrintfEx(trERROR, "  Fault reason: %d (this value is unexpected).\n",
          info->si_code );
      break;
  }

  // get pointers for the stack entries
  size = backtrace( array, CALL_STACK_TRACE_DEPTH );

  if ( size == 0 )
  {
    uiPrintfEx(trERROR, "Stack trace unavailable\n" );
  }
  else
  {
    uiPrintfEx(trERROR, "Stack trace folows%s:\n",
        (size < CALL_STACK_TRACE_DEPTH) ? "" : " (partial)" );

    // print out the stack frames symbols to stderr
    backtrace_symbols_fd( array, size, STDERR_FILENO );
  }

  /* unregister this handler and let the default action execute */
  uiPrintfEx(trERROR, "Executing original handler...\n" );
  exit(-1); //todo: need to pass execution to the original fault handler. This was causing some issues, and shall be investigated at later stage.
  unregister_segmentation_fault_handler();
}

/*********************************************************************
 *
 * @fn          register_segmentation_fault_handler
 *
 * @brief       This function registers the  segmentation fault
 *              handler function.
 *
 *********************************************************************/
void register_segmentation_fault_handler( void )
{
  struct sigaction action;

  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = segmentation_fault_handler;
  memset( &action.sa_mask, 0, sizeof(action.sa_mask) );
  action.sa_restorer = NULL;

  if ( sigaction( SIGSEGV, &action, NULL ) < 0 )
  {
    perror( "sigaction" );
  }
}

/*********************************************************************
 *
 * @fn          unregister_segmentation_fault_handler
 *
 * @brief       This function unregisters the  segmentation fault
 *              handler function.
 *
 *********************************************************************/
void unregister_segmentation_fault_handler( void )
{
  struct sigaction action;

  action.sa_flags = 0;
  action.sa_handler = SIG_DFL;
  memset( &action.sa_mask, 0, sizeof(action.sa_mask) );
  action.sa_restorer = NULL;

  sigaction( SIGSEGV, &action, NULL );
}
