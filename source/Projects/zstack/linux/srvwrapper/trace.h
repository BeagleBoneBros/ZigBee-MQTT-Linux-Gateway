#ifndef TRACE_H
#define TRACE_H

/*********************************************************************
 Filename:       trace.h
 Revised:        $Date$
 Revision:       $Revision$

 Description:   server trace module

 Copyright 2013 - 2014 Texas Instruments Incorporated. All rights reserved.

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
 *********************************************************************/
#include <stdint.h>
#ifndef SINGLE_THREADED_PROCESS
#include <pthread.h>
#endif

/*********************************************************************
 * Generic definitions
 *********************************************************************/
#define trUNMASKABLE 0
#define trFATAL      1
#define trERROR      2
#define trWARNING    3
#define trINFO       4
#define trDEBUG      5
#define trCUSTOM     6
#define trMISC       7
#define trPKT_HEX    8
#define trPKT_TYPE   9
#define trPKT_CONTNT 10
#define trUNDEFINED  11
#define trEMPTY_LINE 12
#define trHELP       13
#define trCONNECTION 14
#define trSHOW_TYPE  15

#define MASK(bit) (1 << (bit))

#define RESET_ATTRIBUTES		"\x1B[0m"
#define BOLD				"\x1B[1m"
#define UNDERSCORE			"\x1B[4m"
#define NO_UNDERSCORE			"\x1B[24m"
#define NORMAL				"\x1B[22m"
#define BLINK				"\x1B[5m"
#define REVERSED			"\x1B[7m"
#define CONCEALED			"\x1B[8m"

#define BLACK				"\x1B[30m"
#define RED				"\x1B[31m"
#define GREEN				"\x1B[32m"
#define YELLOW				"\x1B[33m"
#define BLUE				"\x1B[34m"
#define MAGENTA				"\x1B[35m"
#define CYAN				"\x1B[36m"
#define WHITE				"\x1B[37m"

#define onBLACK				"\x1B[40m"
#define onRED				"\x1B[41m"
#define onGREEN				"\x1B[42m"
#define onYELLO				"\x1B[43m"
#define onBLUE				"\x1B[44m"
#define onMAGENTA			"\x1B[45m"
#define onCYAN				"\x1B[46m"
#define onWHITE				"\x1B[47m"

#define CURSOR_HOME			"\x1B[H"
#define CURSOR_BACK			"\x1B[D"
#define CURSOR_GET_POS			"\x1B[6n"
#define CURSOR_SAVE			"\x1B[s"
#define CURSOR_RESTORE			"\x1B[u"

#define DARK_BLUE			RESET_ATTRIBUTES BLUE
#define LIGHT_BLUE			RESET_ATTRIBUTES BOLD BLUE

#define DARK_YELLOW			RESET_ATTRIBUTES YELLOW
#define LIGHT_YELLOW			RESET_ATTRIBUTES BOLD YELLOW

#define DARK_GREEN			RESET_ATTRIBUTES GREEN
#define LIGHT_GREEN			RESET_ATTRIBUTES BOLD GREEN

#define DARK_CYAN			RESET_ATTRIBUTES CYAN
#define LIGHT_CYAN			RESET_ATTRIBUTES BOLD CYAN

#define DARK_RED			RESET_ATTRIBUTES RED
#define LIGHT_RED			RESET_ATTRIBUTES BOLD RED

#define DARK_MAGENTA			RESET_ATTRIBUTES MAGENTA
#define LIGHT_MAGENTA			RESET_ATTRIBUTES BOLD MAGENTA

#define DARK_WHITE			RESET_ATTRIBUTES WHITE
#define LIGHT_WHITE			RESET_ATTRIBUTES BOLD WHITE

#define TRACE_INCLUDE_TIME_DELTA 1
#define TRACE_NO_TIME_DELTA 0

/*********************************************************************
 * Protocol trace specific definitions
 *********************************************************************/
#define SOC_LAYER_NUMBER 0
#define NPI_LAYER_NUMBER 1
#define ZSTACK_LAYER_NUMBER 2
#define NWKMGR_LAYER_NUMBER 3
#define GATEWAY_LAYER_NUMBER 4
#define OTASRVR_LAYER_NUMNER 4
#define APP_LAYER_NUMNER 5
#define INVALID_LAYER_NUMBER 6

#define SAMPLE_APP_NAME "SMPLAPP"
#define ZSTACK_SERVER_NAME "Z_STACK"
#define NWKMGR_SERVER_NAME "NWK_MGR"
#define GATEWAY_SERVER_NAME "GATEWAY"
#define OTASRVR_SERVER_NAME "OTASRVR"
#define NPI_SERVER_NAME    "NPISRVR"
#define SOC_NAME           "SOCZIGB"

#ifndef SERVER_NAME
#define SERVER_NAME UNNAMED_PROCESS
#endif

/*********************************************************************
 * Functional macros
 *********************************************************************/
#define SPECIFIC_THREAD_NAME() (PTHREAD_GETSPECIFIC(thread_name_key) != NULL ? (char *)PTHREAD_GETSPECIFIC(thread_name_key) : "UNNAMED_THREAD")
#define THREAD_TRACE_MASK() (*(uint32_t *)PTHREAD_GETSPECIFIC(thread_trace_mask_key))

#define to_string(a) _to_string(a)
#define _to_string(a) # a

#define STRING_START(str, format, ...) snprintf((str), sizeof(str), format, ##__VA_ARGS__)
#define STRING_REMAINING_CHARS(str) (sizeof(str) - strlen(str) - 1)
#define STRING_ADD(str, format, ...) snprintf((str) + strlen(str), sizeof(str) - strlen(str), format, ##__VA_ARGS__)
#define STRING_ADD_UNPROTECTED(str, format, ...) sprintf((str) + strlen(str), format, ##__VA_ARGS__)
#define STRING_ADD_64BIT_HEX(str, val) snprintf((str) + strlen(str), sizeof(str) - strlen(str), "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", (unsigned int)(((val) >> 56) & 0xFF), (unsigned int)(((val) >> 48) & 0xFF), (unsigned int)(((val) >> 40) & 0xFF), (unsigned int)(((val) >> 32) & 0xFF), (unsigned int)(((val) >> 24) & 0xFF), (unsigned int)(((val) >> 16) & 0xFF), (unsigned int)(((val) >> 8) & 0xFF), (unsigned int)(((val) >> 0) & 0xFF))
#define STRING_ADD_ENUM_NAME(descriptor, value, str) do \
{\
	int i; \
	for (i = 0; i < descriptor.n_values; i++) \
	{ \
		if (descriptor.values[i].value == value) \
		{ \
			STRING_ADD(str, "/%s", descriptor.values[i].name); \
			break; \
		} \
	} \
	if (i == descriptor.n_values) \
	{ \
		STRING_ADD(str, "/[unknown zone type %04X]", value); \
	} \
} while (0)

#define uiPrintf(fmt, ...) uiPrintfEx(trMISC, fmt, ##__VA_ARGS__)

/*********************************************************************
 * Utility macros
 *********************************************************************/
#define LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf) \
	((pBuf += 4), (uint32_t)( \
		(((uint32_t)((pBuf - 4)[0])) * 1) + \
		(((uint32_t)((pBuf - 4)[1])) * 0x100) + \
		(((uint32_t)((pBuf - 4)[2])) * 0x10000) + \
		(((uint32_t)((pBuf - 4)[3])) * 0x1000000) \
	))

#define max(A,B) ((A) > (B) ? (A) : (B))
#define min(A,B) ((A) < (B) ? (A) : (B))

#ifdef SINGLE_THREADED_PROCESS
typedef void * PTHREAD_KEY_T;
#define PTHREAD_KEY_CREATE(A,B) (0)
#define PTHREAD_SETSPECIFIC(A,B) ((A) = (B), 0)
#define PTHREAD_GETSPECIFIC(A) (A)
#else
typedef pthread_key_t PTHREAD_KEY_T;
#define PTHREAD_KEY_CREATE(A,B) pthread_key_create((A),(B))
#define PTHREAD_SETSPECIFIC(A,B) pthread_setspecific(A,B)
#define PTHREAD_GETSPECIFIC(A) pthread_getspecific(A)
#endif

/*********************************************************************
 * Types
 *********************************************************************/
typedef struct
{
	int localLN;
	int remoteLN;
	char * localName;
	char * remoteName;
	int max_layers;
	bool directionSend; //WARNING: Beware if adding any member after this one, that would require alignment. some files using this structure use #pragma pack() to disable padding, while others do not. this causes a conflict. This structore should be kept in a form that it has the same offsets whether packed or padded.
} visualization_args_t;

/*********************************************************************
 * Exposed external variables
 *********************************************************************/
extern PTHREAD_KEY_T thread_name_key;
extern PTHREAD_KEY_T thread_trace_mask_key;

extern char const * TRACE_GROUP_NAME[];
extern bool trace_engine_initialized;

extern uint32_t default_trace_enable_mask;

extern bool traceShowRxFromLayers[6];
extern bool traceShowTxToLayers[6];
extern uint8_t traceLayerNameCharCount;
extern uint8_t traceArrowCharCount;

/*********************************************************************
 * Configuration items
 *********************************************************************/
#define TRACE_CONFIG_ITEMS \
  {&traceShowRxFromLayers, "TRACE_SHOW_RX_FROM_LAYERS", TYPE_BOOL, 6}, \
  {&traceShowTxToLayers, "TRACE_SHOW_TX_TO_LAYERS", TYPE_BOOL, 6}, \
  {&traceLayerNameCharCount, "TRACE_LAYER_NAME_CHAR_COUNT", TYPE_BOOL, 1}, \
  {&traceArrowCharCount, "TRACE_ARROW_CHAR_COUNT", TYPE_BOOL, 1}, \

/*********************************************************************
 * Functions
 *********************************************************************/
bool trace_init_main(char * thread_name);
void trace_deinit(void);
bool trace_init_thread(char * thread_name);
void trace_print_buf(void * buf, uint32_t len, char * description, visualization_args_t * visualization_args);

char * trace_get_current_timestamp(bool include_delta);

void * set_connection_details(int handle, char * name, int name_len, int layerNumber);
char * get_connection_name(int handle);
int get_connection_layer_number(int handle);

char * trace_generate_transport_flow_visualization(char * str, visualization_args_t * visualization_args);
void uiPrintfEx(uint32_t trace_group, char * format, ...);

#endif /* TRACE_H */
