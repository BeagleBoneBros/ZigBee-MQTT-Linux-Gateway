/*******************************************************************************
 Filename:       pb_utils.h
 Revised:        $Date: 2014-10-27 13:36:41 -0700 (Mon, 27 Oct 2014) $
 Revision:       $Revision: 40822 $

 Description:   Utilities for use with protobuf


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
#ifndef PB_UTILS_H
#define PB_UTILS_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "protobuf-c.h"
#include "stdbool.h"
#include "trace.h"

/******************************************************************************
 * Types
 *****************************************************************************/

typedef void   (*pb__init)
                     (void         *message);
typedef size_t (*pb__get_packed_size)
                     (const void   *message);
typedef size_t (*pb__pack)
                     (const void   *message,
                      uint8_t             *out);
typedef size_t (*pb__pack_to_buffer)
                     (const void   *message,
                      ProtobufCBuffer     *buffer);
typedef void *
               (*pb__unpack)
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
typedef void   (*pb__free_unpacked)
                     (void *message,
                      ProtobufCAllocator *allocator);
typedef struct
{
  const ProtobufCMessageDescriptor * descriptor;
//  pb__init
//  pb__get_packed_size
//  pb__pack
//  pb__pack_to_buffer
  const pb__unpack f_unpack;
  const pb__free_unpacked f_free_unpacked;
  const int commandId;
} pb_pointers_entry;

typedef struct _pb_pointer_list_t * pb_pointer_list_ptr_t;

typedef struct _pb_pointer_list_t
{
	uint32_t subsystem_id;
	pb_pointer_list_ptr_t next;
	const uint32_t n_descriptors;
	const pb_pointers_entry * pb_ptrs;
} pb_pointer_list_t;

pb_pointer_list_t * register_pb_pointer_list(pb_pointer_list_t * pb_ptrs_list);
void print_pb_msg(uint32_t _subsystemId, uint32_t commandId, const uint8_t * buf, size_t len, int line_offset, char * description, bool include_timestamp, visualization_args_t * visualization_args, int header_length, char * alternative_description);

#endif
