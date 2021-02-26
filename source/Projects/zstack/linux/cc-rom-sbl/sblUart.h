/******************************************************************************

 @file sblUart.c

 @brief CCxxxx Bootloader Platform Specific UART functions

 Group: WCS LPRF
 $Target Devices: Linux: AM335x, Embedded Devices: CC13xx/CC26xx/CC2538$

 ******************************************************************************
 $License: BSD3 2016 $
  
   Copyright (c) 2015, Texas Instruments Incorporated
   All rights reserved.
  
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
  
   *  Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
  
   *  Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
  
   *  Neither the name of Texas Instruments Incorporated nor the names of
      its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
  
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 $Release Name: Zigbee 3.0 Linux Gateway$
 $Release Date: April 24th, 2018$
 *****************************************************************************/

#ifndef SBL_UART_H
#define SBL_UART_H

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "deviceTypes.h"


//*****************************************************************************
//
//! \brief Opens the UART to the CC13/26xx ROM bootloader.
//!
//! This functions opens the UART to the CC13/26xx ROM bootloader.
//!
//! \param devicePath - path to the UART device.
//!
//! \return returns 0 for success and -1 for error
//
//*****************************************************************************
int SblUart_open(const char *devicePath, DeviceType_t deviceType);

//*****************************************************************************
//
//! \brief Closes the UART to the CC13/26xx ROM bootloader.
//!
//! This functions closes the UART to the CC13/26xx ROM bootloader.
//!
//! \param none.
//!
//! \return true if the port was successfully closed, false otherwise.
//
//*****************************************************************************
bool SblUart_close(void);

//*****************************************************************************
//
//! \brief Writes to the CC13/26xx ROM bootloader UART.
//!
//! This functions writes to the CC13/26xx ROM bootloader UART.
//!
//! \param buf - pointer to a buffer of byte to be written
//! \param len - length of the buffer to be written
//!
//! \return none
//
//*****************************************************************************
void SblUart_write(const unsigned char* buf, size_t len);

//*****************************************************************************
//
//! \brief Reads from the CC13/26xx ROM bootloader UART.
//!
//! This functions reads from the CC13/26xx ROM bootloader UART.
//!
//! \param buf - pointer to a buffer to be read in to
//! \param len - length of the buffer
//!
//! \return returns number of bytes read
//
//*****************************************************************************
unsigned char SblUart_read(unsigned char* buf, size_t len);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //SBL_UART_H
