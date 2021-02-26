/*******************************************************************************
 Filename:      pb_utils.c
 Revised:        $Date: 2014-11-18 18:32:59 -0800 (Tue, 18 Nov 2014) $
 Revision:       $Revision: 41168 $

 Description:   Protobuf Interface


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

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdbool.h>

#include "pb_utils.h"
#include "trace.h"
#include "stdio.h"

#define RPC_SUBSYSTEM_MASK  0x1F

#ifdef SAMPLE_APP_BUILD
#include "user_interface.h"
#define PROJECT_SPECIFIC_TRACE UI_PRINT_LOG_EX
#else
#define PROJECT_SPECIFIC_TRACE(traceGroup ,fmt ,...) do {uiPrintfEx(traceGroup, fmt, ##__VA_ARGS__); } while (0)
#endif

/******************************************************************************
 * Static Variables
 *****************************************************************************/
static pb_pointer_list_t * master_pb_pointer_list = NULL;

/******************************************************************************
 * Functions
 *****************************************************************************/
//get_enum_name() asusmes sorted enum descriptors list.
const char * get_enum_name(int number, const ProtobufCEnumDescriptor * enum_descriptor)
{
	const char * enum_name = "<NOT_FOUND>";
	int i = number;

	while ((i >= 0) && (i < enum_descriptor->n_values) && (enum_descriptor->values[i].value != number))
	{
		if (enum_descriptor->values[i].value > number)
		{
			i--;
		}
		else
		{
			//in an enum list, each value can appear only once, so in a sorted enum list, the value of an item cannot be smaller than the index of that item
			enum_name = "<UNEXPECTED_UNSORTED_ENUM_DESCRIPTOR_LIST>";
			break;
		}
	}

	if (enum_descriptor->values[i].value == number)
	{
		enum_name = enum_descriptor->values[i].name;
	}

	return enum_name;
}  

void print_unpacked_pb_msg(const ProtobufCMessageDescriptor * descriptor, uint8_t * msg, int line_offset)
{
	static int nestingLevel = 1;
	uint8_t * pBuf = msg;
	int field_index;
	uint32_t quantifier;
	uint8_t * sub_message;
	uint32_t uint32_value;
	uint32_t count;
	size_t binary_data_len;
	uint8_t * binary_data_ptr;
	int i,j;
	char log_string[180]; //TBD (size)

	for (field_index = 0; field_index < descriptor->n_fields; field_index++)
	{
		STRING_START(log_string, BLUE "%*s%s", nestingLevel * 2 + line_offset, "", descriptor->fields[field_index].name);

		count = 1;

		if (descriptor->fields[field_index].label == PROTOBUF_C_LABEL_REPEATED) //??? verify this is a bitmask
		{
			pBuf = msg + descriptor->fields[field_index].quantifier_offset;
			count = LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf);
			if (count == 0)
			{
				continue; //skip unused optional fields
			}
		}
		else if (descriptor->fields[field_index].label == PROTOBUF_C_LABEL_OPTIONAL) //??? verify this is a bitmask
		{
			if (descriptor->fields[field_index].type == PROTOBUF_C_TYPE_MESSAGE)
			{
				if (descriptor->fields[field_index].quantifier_offset != 0) //TODO: understand
				{
					PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, BLUE "%*s    %s: " RED "Unexpected non-zero quantifier_offset = %d", nestingLevel * 2 + line_offset, "", descriptor->fields[field_index].name, descriptor->fields[field_index].quantifier_offset);
				}
			}
			else
			{
				pBuf = msg + descriptor->fields[field_index].quantifier_offset;

				quantifier = LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf);

				if (quantifier == 0)
				{
					continue; //skip unused optional fields //??? need to continue to next field, not next count...
				}
			}
		}

		pBuf = msg + descriptor->fields[field_index].offset;


		if (descriptor->fields[field_index].label == PROTOBUF_C_LABEL_REPEATED)
		{
			pBuf = ((uint8_t **)pBuf)[0];
			if (pBuf == NULL)
			{
				PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, BLUE "%*s    %s: " RED "Unexpected null pointer for non-empty list", nestingLevel * 2 + line_offset, "", descriptor->fields[field_index].name);
				continue; //skip unused optional fields //???
			}

			STRING_ADD(log_string, " :");
			PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, log_string);

			nestingLevel++;
			STRING_START(log_string, BLUE "%*s[%03d]", nestingLevel * 2 + line_offset, "", 0);
		}

		for (i = 0; i < count; i++)
		{
			switch (descriptor->fields[field_index].type)
			{
				case PROTOBUF_C_TYPE_FIXED64:
					STRING_ADD(log_string, " = " DARK_MAGENTA "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", pBuf[7], pBuf[6], pBuf[5], pBuf[4], pBuf[3], pBuf[2], pBuf[1], pBuf[0] );
					PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, log_string);
					break;

				case PROTOBUF_C_TYPE_UINT32:
					uint32_value = LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf);
					STRING_ADD(log_string, " = " DARK_MAGENTA "0x%08X (%d)", uint32_value, uint32_value);
					PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, log_string);
					break;

				case PROTOBUF_C_TYPE_INT32:
					uint32_value = LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf);
					STRING_ADD(log_string, " = " DARK_MAGENTA "0x%08X (%d)", uint32_value, uint32_value);
					PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, log_string);
					break;

				case PROTOBUF_C_TYPE_BOOL:
					uint32_value = *pBuf;
					pBuf += sizeof(protobuf_c_boolean);
					STRING_ADD(log_string, " = " DARK_MAGENTA "%d", uint32_value);
					PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, log_string);
					break;

				case PROTOBUF_C_TYPE_ENUM:
					STRING_ADD(log_string, " = " DARK_MAGENTA "%s", get_enum_name(LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf), descriptor->fields[field_index].descriptor));
					PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, log_string);
					break;

				case PROTOBUF_C_TYPE_MESSAGE:
                  if (descriptor->fields[field_index].flags & PROTOBUF_C_FIELD_FLAG_PACKED)
                    {
						PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, BLUE "%*s    %s: " RED "<HANDLING_OF_PACKED_MESSAGES_NOT_DONE_YET>", nestingLevel * 2 + line_offset, "", descriptor->fields[field_index].name);
					}
					else
					{
						sub_message = (uint8_t *)LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf);
						if (sub_message == NULL)
						{
							if (descriptor->fields[field_index].label != PROTOBUF_C_LABEL_OPTIONAL) //??? verify this is a bitmask
							{
								PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, BLUE "%*s    %s: " RED "Unexpected null pointer for non-optional message field", nestingLevel * 2 + line_offset, "", descriptor->fields[field_index].name);
							}
							continue; //skip unused optional fields
						}

						STRING_ADD(log_string, " :");
						PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, log_string);
						nestingLevel++;
						print_unpacked_pb_msg(descriptor->fields[field_index].descriptor, sub_message, line_offset);
						nestingLevel--;
					}
					break;

				case PROTOBUF_C_TYPE_BYTES:
					binary_data_len = LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf);
					binary_data_ptr = (uint8_t *)LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf);
					STRING_ADD(log_string, " (hex string) = " DARK_MAGENTA);
					for (j = 0; j < binary_data_len; j++)
					{
						STRING_ADD(log_string, "%02X", *binary_data_ptr++);
						if (j < (binary_data_len - 1))
						{
							STRING_ADD(log_string, ":");
						}
					}
					PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, log_string);

					break;

				case PROTOBUF_C_TYPE_SINT32:
				case PROTOBUF_C_TYPE_SFIXED32:
				case PROTOBUF_C_TYPE_INT64:
				case PROTOBUF_C_TYPE_SINT64:
				case PROTOBUF_C_TYPE_SFIXED64:
				case PROTOBUF_C_TYPE_FIXED32:
				case PROTOBUF_C_TYPE_UINT64:
				case PROTOBUF_C_TYPE_FLOAT:
				case PROTOBUF_C_TYPE_DOUBLE:
				case PROTOBUF_C_TYPE_STRING:
				default:
					PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, BLUE "%*s%s: " RED "<UNKNOWN_TYPE %d>", nestingLevel * 2 + line_offset, "", descriptor->fields[field_index].name, descriptor->fields[field_index].type);
					break;
			}

			STRING_START(log_string, BLUE "%*s[%03d]", nestingLevel * 2 + line_offset, "", i + 1);
		}

		if (descriptor->fields[field_index].label == PROTOBUF_C_LABEL_REPEATED)
		{
			nestingLevel--;
		}
	}
}  

char * current_timestamp(void)
{
	static char timestamp_str[100]; //??? set exact size
	char timestamp[26];
	struct timeval tv;
	int64_t time_delta;
	time_t time_delta_sec;
	suseconds_t time_delta_usec;
	static int64_t prev_time = 0;
	int64_t curr_time = 0;

	gettimeofday(&tv,NULL);
	ctime_r (&tv.tv_sec, timestamp);

	curr_time = (int64_t)(tv.tv_sec) * 1000000 + tv.tv_usec;
	if (prev_time == 0)
	{
		prev_time = curr_time;
	}

	time_delta = curr_time - prev_time;
	prev_time = curr_time;

	time_delta_sec = time_delta / 1000000;
	time_delta_usec = time_delta % 1000000;

	sprintf(timestamp_str, "[%.*s.%03ld,%.03ld / %02ld:%02ld:%02ld.%03ld,%.03ld]", 8, strchr(timestamp, ':') - 2, tv.tv_usec / 1000,  tv.tv_usec % 1000, (time_delta_sec / 3600) % 60, (time_delta_sec / 60) % 60, time_delta_sec % 60, time_delta_usec / 1000,  time_delta_usec % 1000);

	return timestamp_str;
}

pb_pointer_list_t * register_pb_pointer_list(pb_pointer_list_t * pb_ptrs_list)
{
	if ((pb_ptrs_list == NULL) || (pb_ptrs_list->next != NULL))
	{
		return NULL;
	}

	pb_ptrs_list->next = master_pb_pointer_list;
	master_pb_pointer_list = pb_ptrs_list;

	return master_pb_pointer_list;
}

const pb_pointers_entry * find_pb_pointers(uint32_t subsystemId, uint32_t commandId)
{
	int i;
	pb_pointer_list_t * pb_ptrs_list = master_pb_pointer_list;

	while ((pb_ptrs_list != NULL) && (pb_ptrs_list->subsystem_id != subsystemId))
	{
		pb_ptrs_list = pb_ptrs_list->next;
	}

	if (pb_ptrs_list == NULL)
	{
		return NULL;
	}

	if ((commandId < pb_ptrs_list->n_descriptors) && (pb_ptrs_list->pb_ptrs[commandId].commandId == commandId))
	{
		return &(pb_ptrs_list->pb_ptrs[commandId]);
	}

	for (i = 0; i < pb_ptrs_list->n_descriptors; i++)
	{
		if (pb_ptrs_list->pb_ptrs[i].commandId == commandId)
		{
			return &(pb_ptrs_list->pb_ptrs[i]);
		}
	}

	return NULL;
}

void print_pb_msg(uint32_t _subsystemId, uint32_t commandId, const uint8_t * buf, size_t len, int _line_offset, char * description, bool include_timestamp, visualization_args_t * visualization_args, int header_length, char * alternative_description )
{
	uint8_t * msg;
	int line_offset = _line_offset;
	const pb_pointers_entry * pb_ptrs;
	uint32_t subsystemId = _subsystemId & RPC_SUBSYSTEM_MASK;
	char description_string[500]; //todo: instead of 500, use actual maximum possible length. must include space for the ansi formatting characters.

	if (((MASK(trPKT_TYPE) | MASK(trPKT_CONTNT)) & (THREAD_TRACE_MASK() | MASK(trUNMASKABLE))) == 0)
	{
		return;
	}

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
	// TODO: Redefine condition with function that checks payload to ensure the packet to be omitted is the write attribute req and write attribute rsp indication
	//This condition prevents the Identify Timer to fill the server log with spam
	if(!((_subsystemId==19)&&(commandId==22)))
	{
		if(!((_subsystemId==19)&&(commandId==23)))
		{
			STRING_START(description_string, LIGHT_BLUE "%*s%s%s%s", line_offset, "", include_timestamp ? current_timestamp() : "", include_timestamp ? " " : "", description);

			trace_generate_transport_flow_visualization(description_string + strlen(description_string), visualization_args);

			line_offset += INVALID_LAYER_NUMBER * 9 + 1; //strlen(description_string) does not work because the string contains the ansi formatting escape sequences
			
			pb_ptrs = find_pb_pointers(subsystemId, commandId);

			if (pb_ptrs == NULL)
			{
				int i;
				char hex_line[(len + 4) * 3 + strlen(alternative_description)];

				STRING_START(hex_line, "%s", alternative_description != NULL ? alternative_description : "");
				
				if (header_length == 4)
				{
					STRING_ADD(hex_line, "%02X:%02X:%02X:%02X", len & 0xFF, (len >> 8) & 0xFF, _subsystemId, commandId);
				}
				else
				{
					STRING_ADD(hex_line, "%02X:%02X:%02X", len, _subsystemId, commandId);
				}
				
				STRING_ADD(hex_line, "%s", len > 0 ? ":" : "");

				for ( i = 0; i < len; i++ )
				{
					STRING_ADD(hex_line,"%02X", ((uint8_t *)buf)[i]);
					if (i < (len - 1))
					{
						STRING_ADD(hex_line,":");
					}
				}

				PROJECT_SPECIFIC_TRACE(trPKT_TYPE,"%s" LIGHT_MAGENTA "%s", description_string, hex_line); //subsystemId, commandId, len);
			}
			else
			{
				STRING_ADD(description_string, LIGHT_MAGENTA "%s", pb_ptrs->descriptor->name);
				PROJECT_SPECIFIC_TRACE(trPKT_TYPE, description_string);

				if ((MASK(trPKT_CONTNT) & (THREAD_TRACE_MASK() | MASK(trUNMASKABLE))) != 0)
				{
					msg = pb_ptrs->f_unpack(NULL, len, buf);
					if (msg != NULL)
					{
						print_unpacked_pb_msg(pb_ptrs->descriptor, msg, line_offset);
				
						pb_ptrs->f_free_unpacked(msg, NULL);
					}
					else
					{
						STRING_ADD(description_string, RED " failed unpacking: subsystemId=%d, commandId=%d, len=%d", subsystemId, commandId, len);
						PROJECT_SPECIFIC_TRACE(trPKT_CONTNT, description_string);
					}
				}
			}
		}
	}
}

