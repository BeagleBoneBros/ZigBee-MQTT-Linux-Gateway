/*******************************************************************************
 Filename:      user_interface.h
 Revised:        $Date$
 Revision:       $Revision$

 Description:   User Interface


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
#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <gateway.pb-c.h>
#include <otasrvr.pb-c.h>

#include "types.h"
#include "trace.h"

/******************************************************************************
 * Constants
 *****************************************************************************/
#define UI_STATUS_NOTIFICATION_TIMEOUT 1000

#define UI_LOG_LINE_LENGTH 150

#define CLUSTER_ID_IAS_ZONE 0x0500

/******************************************************************************
 * Global variables
 *****************************************************************************/
extern uint32_t log_enable_mask;

/******************************************************************************
 * Function Prototypes
 *****************************************************************************/
void ui_refresh_display(void); //shall be called whenever making any change to the global data-structure
int ui_init(char * log_filename);
void ui_deinit(void);
void _ui_print_log(int trace_group, char * format, ...);
void ui_print_status(uint64_t timeout_ms, char * format, ...);
char * ui_make_string_GwAddrStructT(GwAddressStructT * addr);
char * ui_make_string_OtaAddrStruct(AddressStruct * addr);
void ui_print_packet_to_log(pkt_buf_t * pkt, char * prefix, char * hilight);
void ui_redraw_server_state(void);
void ui_redraw_network_info(void);
void ui_redraw_toggles_indications(void);
uint8_t ui_get_selected_local_endpoint_id(void);

/******************************************************************************
 * Functional Macros
 *****************************************************************************/
#define  UI_PRINT_LOG_EX(trace_group, fmt, ...) do {_ui_print_log(trace_group, (fmt), ##__VA_ARGS__);} while (0)
#define  UI_PRINT_LOG(fmt, ...) UI_PRINT_LOG_EX(trUNDEFINED, (fmt), ##__VA_ARGS__);

#define LITTLE_ENDIAN_TO_INT16(ADDR) \
((int32_t)( \
	(((uint32_t)(((uint8_t *)(ADDR))[0])) * 1) + \
	(((uint32_t)(((uint8_t *)(ADDR))[1])) * 0x100) \
))

#define LITTLE_ENDIAN_TO_UINT16(ADDR) \
((uint32_t)( \
	(((uint32_t)(((uint8_t *)(ADDR))[0])) * 1) + \
	(((uint32_t)(((uint8_t *)(ADDR))[1])) * 0x100) \
))


#define LITTLE_ENDIAN_TO_INT24(ADDR) \
	((int32_t)( \
		(((uint32_t)(((uint8_t *)(ADDR))[0])) * 1) + \
		(((uint32_t)(((uint8_t *)(ADDR))[1])) * 0x100) + \
		(((uint32_t)(((uint8_t *)(ADDR))[2])) * 0x10000)+ \
		(((((uint8_t *)(ADDR))[2]) & 0x80) ? 0xFF000000 : 0) \
	))

#define LITTLE_ENDIAN_TO_UINT24(ADDR) \
((uint32_t)( \
	(((uint32_t)(((uint8_t *)(ADDR))[0])) * 1) + \
	(((uint32_t)(((uint8_t *)(ADDR))[1])) * 0x100) + \
	(((uint32_t)(((uint8_t *)(ADDR))[2])) * 0x10000) \
))
	
#define LITTLE_ENDIAN_TO_INT32(ADDR) \
((int32_t)( \
	(((uint32_t)(((uint8_t *)(ADDR))[0])) * 1) + \
	(((uint32_t)(((uint8_t *)(ADDR))[1])) * 0x100) + \
	(((uint32_t)(((uint8_t *)(ADDR))[2])) * 0x10000) + \
	(((uint32_t)(((uint8_t *)(ADDR))[3])) * 0x1000000) \
))

#define LITTLE_ENDIAN_TO_UINT32(ADDR) \
((uint32_t)( \
	(((uint32_t)(((uint8_t *)(ADDR))[0])) * 1) + \
	(((uint32_t)(((uint8_t *)(ADDR))[1])) * 0x100) + \
	(((uint32_t)(((uint8_t *)(ADDR))[2])) * 0x10000) + \
	(((uint32_t)(((uint8_t *)(ADDR))[3])) * 0x1000000) \
))

#define LITTLE_ENDIAN_STREAM_TO_UINT32(pBuf) \
((pBuf += 4), (uint32_t)( \
	(((uint32_t)((pBuf - 4)[0])) * 1) + \
	(((uint32_t)((pBuf - 4)[1])) * 0x100) + \
	(((uint32_t)((pBuf - 4)[2])) * 0x10000) + \
	(((uint32_t)((pBuf - 4)[3])) * 0x1000000) \
))

#define DEVICE_ID_IAS_ZONE 0x0402

#endif /* USER_INTERFACE_H */
