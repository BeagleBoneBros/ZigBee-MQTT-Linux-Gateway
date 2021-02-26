/*********************************************************************
 Filename:       trace.c
 Revised:        $Date$
 Revision:       $Revision$

 Description:    Server trace module

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

/*********************************************************************
 * INCLUDES
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

#include "time.h"
#include "sys/time.h"

#include "trace.h"

#ifdef SAMPLE_APP_BUILD
#include "user_interface.h"
#define PROJECT_SPECIFIC_TRACE UI_PRINT_LOG_EX
#else
#define PROJECT_SPECIFIC_TRACE(traceGroup ,fmt ,...) do {uiPrintfEx(traceGroup, fmt, ##__VA_ARGS__); } while (0)
#endif

#ifdef SINGLE_THREADED_PROCESS
#else
pthread_mutex_t traceMutex;
#endif

#ifndef DEFAULT_TRACE_ENABLE_MASK
#define DEFAULT_TRACE_ENABLE_MASK (MASK(trFATAL) | MASK(trERROR) | MASK(trWARNING))
#endif

uint32_t default_trace_enable_mask = DEFAULT_TRACE_ENABLE_MASK;

PTHREAD_KEY_T thread_name_key;
PTHREAD_KEY_T thread_trace_mask_key;

bool trace_engine_initialized = false;

bool traceShowRxFromLayers[6] = {1, 0, 0, 0, 0, 1};
bool traceShowTxToLayers[6] = {1, 1, 1, 1, 1, 1};
uint8_t traceLayerNameCharCount = 7;
uint8_t traceArrowCharCount = 2;

char const * TRACE_GROUP_NAME[] = 
{
	"UNMSKBL",
	"FATAL  ",
	"ERROR  ",
	"WARNING",
	"INFO   ",
	"DEBUG  ",
	"CUSTOM ",
	"MISC1  ",
	"PKT_HEX",
	"PKTTYPE",
	"PKTBODY",
	"MISC2  ",
	"BLNKLIN",
	"HELP   ",
	"CONNECT",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	""
};

void thread_basic_destructor(void * name)
{
	if (name != NULL)
	{
		free(name);
	}
}

bool trace_init_main(char * thread_name)
{
	trace_engine_initialized = true;
	
	if (PTHREAD_KEY_CREATE(&thread_name_key, thread_basic_destructor) != 0)
	{
		return false;
	}

	if (PTHREAD_KEY_CREATE(&thread_trace_mask_key, thread_basic_destructor) != 0)
	{
		return false;
	}

	
#ifdef SINGLE_THREADED_PROCESS
#else
	pthread_mutex_init( &traceMutex, NULL );
#endif
	return trace_init_thread(thread_name);
}

void trace_deinit(void)
{
#ifdef SINGLE_THREADED_PROCESS
#else
	pthread_mutex_destroy( &traceMutex );
#endif
}

bool trace_init_thread(char * thread_name)
{
	char * specific_thread_name;
	uint32_t * thread_trace_mask;

	specific_thread_name = malloc(strlen(thread_name) + 1);
	if (specific_thread_name == NULL)
	{
		return false;
	}
	
	thread_trace_mask = malloc(sizeof(uint32_t));
	if (thread_trace_mask == NULL)
	{
		free(specific_thread_name);
		return false;
	}

	strcpy(specific_thread_name, thread_name);
	
	*thread_trace_mask =  default_trace_enable_mask;

	if (PTHREAD_SETSPECIFIC(thread_name_key, specific_thread_name) != 0)
	{
		free (specific_thread_name);
		free (thread_trace_mask);
		return false;
	}

	if (PTHREAD_SETSPECIFIC(thread_trace_mask_key, thread_trace_mask) != 0)
	{
		PTHREAD_SETSPECIFIC(thread_name_key, NULL);
		free (specific_thread_name);
		free (thread_trace_mask);
		return false;
	}

	return true;
}

void trace_print_buf(void * buf, uint32_t len, char * description, visualization_args_t * visualization_args)
{
	if ((MASK(trPKT_HEX) & (THREAD_TRACE_MASK() | MASK(trUNMASKABLE))) != 0)
	{
		if ((visualization_args != NULL) && (visualization_args->remoteLN < sizeof(traceShowTxToLayers)))
		{
		if (visualization_args->directionSend)
			{
				if (!traceShowTxToLayers[visualization_args->remoteLN])
				{
					return;
				}
			}
			else
			{
				if (!traceShowRxFromLayers[visualization_args->remoteLN])
				{
					return;
				}
			}
		}

		int i;

		char trace_line[len * 3 + strlen(description) + 300]; //todo: instead of 300, use maximum possible length of the visualization string. must include space for the ansi formatting characters.

		STRING_START(trace_line, "%s", "");

		trace_generate_transport_flow_visualization(trace_line + strlen(trace_line), visualization_args);

		STRING_ADD(trace_line, "%s", description);

		for ( i = 0; i < len; i++ )
		{
			STRING_ADD(trace_line,"%02X", ((uint8_t *)buf)[i]);
			if (i < (len - 1))
			{
				STRING_ADD(trace_line,":");
			}
		}
		
		PROJECT_SPECIFIC_TRACE(trPKT_HEX, "%s", trace_line );
	}
}

char * trace_get_current_timestamp(bool include_delta)
{
	static char timestamp_str[100]; //todo: set exact size
	char timestamp[26];
	struct timeval tv;

	gettimeofday(&tv,NULL);
	ctime_r (&tv.tv_sec, timestamp);

	STRING_START(timestamp_str,"[%.*s.%03ld,%.03ld", 8, strchr(timestamp, ':') - 2, tv.tv_usec / 1000,  tv.tv_usec % 1000);

	if (include_delta)
	{
		int64_t time_delta;
		time_t time_delta_sec;
		suseconds_t time_delta_usec;
		static int64_t prev_time = 0;
		int64_t curr_time;
		
		curr_time = (int64_t)(tv.tv_sec) * 1000000 + tv.tv_usec;
		if (prev_time == 0)
		{
			prev_time = curr_time;
		}

		time_delta = curr_time - prev_time;
		prev_time = curr_time;

		time_delta_sec = time_delta / 1000000;
		time_delta_usec = time_delta % 1000000;

		STRING_ADD(timestamp_str, " / %02ld:%02ld:%02ld.%03ld,%.03ld]", (time_delta_sec / 3600) % 60, (time_delta_sec / 60) % 60, time_delta_sec % 60, time_delta_usec / 1000,  time_delta_usec % 1000);
	}
	else
	{
		STRING_ADD(timestamp_str, "]");
	}
	
	return timestamp_str;
}

//todo: remove details structure of disconnected connections, and demove all structures when closing application. 

typedef struct _connection_details_t * connection_details_ptr_t;

typedef struct _connection_details_t
{
	int handle;
	char * name;
	int layerNumber;
	connection_details_ptr_t next;
} connection_details_t;

connection_details_t * connection_details;


connection_details_t * add_connection_details(int handle)
{
	connection_details_t * tmp;

	tmp = malloc(sizeof(connection_details_t));
	if (tmp == NULL)
	{
		return NULL;
	}
	
	tmp->handle = handle;
	tmp->name = NULL;
	tmp->layerNumber = INVALID_LAYER_NUMBER;
	tmp->next = connection_details;

	connection_details = tmp;
	
	return tmp;
}

connection_details_t * get_connection_details_ptr(int handle)
{
	connection_details_t * tmp = connection_details;

	while (tmp != NULL)
	{
		if (tmp->handle == handle)
		{
			return tmp;
		}
		tmp = tmp->next;
	}

	return add_connection_details(handle);
}

void * set_connection_details(int handle, char * name, int name_len, int layerNumber)
{
	connection_details_t * tmp;

	if ((tmp = get_connection_details_ptr(handle)) == NULL)
	{
		return NULL;
	}

	tmp->layerNumber = layerNumber;

	if (tmp->name != NULL)
	{
		free(tmp->name);
	}
	
	tmp->name = malloc(name_len + 1);

	if (tmp->name == NULL)
	{
		return NULL;
	}

	memcpy(tmp->name, name, name_len);
	tmp->name[name_len] = '\0';

	return tmp;
}

int get_connection_layer_number(int handle)
{
	connection_details_t * tmp;
	
	tmp = get_connection_details_ptr(handle);
	
	if (tmp == NULL)
	{
		return INVALID_LAYER_NUMBER;
	}

	return tmp->layerNumber;
}

char * get_connection_name(int handle)
{
	connection_details_t * tmp;
	char temp_name[30];
	
	tmp = get_connection_details_ptr(handle);
	
	if (tmp == NULL)
	{
		return NULL;
	}

	if (tmp->name == NULL)
	{
		sprintf(temp_name, "CON%03d", handle); //using temp_name allows for handle > 3 digits.

		tmp->name = malloc(strlen(temp_name) + 1);
		
		if (tmp->name == NULL)
		{
			return NULL;
		}

		strcpy(tmp->name, temp_name);
	}

	return tmp->name;
}

char * trace_generate_transport_flow_visualization(char * str, visualization_args_t * visualization_args)
{
	int rightPadding;

	if (visualization_args != NULL)
	{
		int localLN = visualization_args->localLN;
		int remoteLN = visualization_args->remoteLN;
		char * localName = visualization_args->localName;
		char * remoteName = visualization_args->remoteName;
		bool directionSend = visualization_args->directionSend;
		int max_layers = visualization_args->max_layers;
		int distance = max(localLN, remoteLN) - min(localLN, remoteLN);

		if (localLN == remoteLN)
		{
			STRING_ADD_UNPROTECTED( LIGHT_BLUE "[" UNDERSCORE "%s" NO_UNDERSCORE "%s%s", localName, directionSend ? "->" : "<-" , remoteName);
		}
		else
		{
			STRING_ADD_UNPROTECTED(str,LIGHT_BLUE "[%*s", min(localLN, remoteLN) * (traceLayerNameCharCount + traceArrowCharCount), "");
			if (localLN < remoteLN)
			{
				STRING_ADD_UNPROTECTED(str, UNDERSCORE "%.*s" NO_UNDERSCORE, traceLayerNameCharCount, localName);
			}
			else
			{
				STRING_ADD_UNPROTECTED(str, "%.*s", traceLayerNameCharCount, remoteName);
			}

			if (((localLN < remoteLN) && directionSend)  || ((localLN > remoteLN) && !directionSend))
			{
				STRING_ADD_UNPROTECTED(str, "%s%.*s" LIGHT_BLUE, directionSend ? LIGHT_YELLOW : DARK_YELLOW, (distance - 1) * (traceLayerNameCharCount + traceArrowCharCount) + traceArrowCharCount, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
			}
			else
			{
				STRING_ADD_UNPROTECTED(str, "%s%.*s" LIGHT_BLUE, directionSend ? LIGHT_YELLOW : DARK_YELLOW, (distance - 1) * (traceLayerNameCharCount + traceArrowCharCount) + traceArrowCharCount, "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
			}

			if (localLN > remoteLN)
			{
				STRING_ADD_UNPROTECTED(str, UNDERSCORE "%.*s" NO_UNDERSCORE, traceLayerNameCharCount, localName);
			}
			else
			{
				STRING_ADD_UNPROTECTED(str, "%.*s", traceLayerNameCharCount, remoteName);
			}
			rightPadding = max(0, (max_layers - 1 - max(localLN, remoteLN)) * (traceLayerNameCharCount + traceArrowCharCount));
			STRING_ADD_UNPROTECTED(str, "%*s", rightPadding, "");
		}
		
		STRING_ADD_UNPROTECTED(str, "] " DARK_WHITE);
	}

	return str;
}

void uiPrintfEx(uint32_t trace_group, char * format, ...) //todo: rename uiPrintfEx to trace_printf
{
	va_list args;

	va_start(args, format);
	
	if (!trace_engine_initialized)
	{
		printf("WARNING: uiPrintf used before calling trace_init_main(). Traces not available.\n");
		fflush(stdout);
	}
	else
	{
		if (((1 << trace_group) & (THREAD_TRACE_MASK() | MASK(trUNMASKABLE))) != 0)
		{
#ifdef SINGLE_THREADED_PROCESS
#else
			if ( pthread_mutex_lock( &traceMutex ) != 0 )
			{
			  printf("ERROR: pthread_mutex_lock failed in uiPrintfEx().\n");
			}
			else
#endif
			{
				printf("%s [%s/%s] %s: ", trace_get_current_timestamp(TRACE_NO_TIME_DELTA), to_string(SERVER_NAME), SPECIFIC_THREAD_NAME(), TRACE_GROUP_NAME[trace_group]);
				vprintf(format, args);
				printf(DARK_WHITE);
				if (format[strlen(format) - 1] != '\n')
				{
					printf("\n");
				}
				fflush(stdout);
#ifdef SINGLE_THREADED_PROCESS
#else
				pthread_mutex_unlock( &traceMutex );
#endif
			}
		}
	}
	
	va_end(args);
}
