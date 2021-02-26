_default: host
#/******************************************************************************
#
# @file Makefile
#
# @brief CCxxxx ROM Bootloader Example Application
#
# Group: WCS LPRF
# $Target Devices: Linux: AM335x, Embedded Devices: CC13xx/CC26xx/CC2538$
#
#############################################################
# $License: BSD3 2016 $
#  
#   Copyright (c) 2015, Texas Instruments Incorporated
#   All rights reserved.
#  
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#  
#   *  Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#  
#   *  Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#  
#   *  Neither the name of Texas Instruments Incorporated nor the names of
#      its contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#  
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#############################################################
# $Release Name: Zigbee 3.0 Linux Gateway$
# $Release Date: April 24th, 2018$
#############################################################

SBU_REV= "0.1"

CFLAGS= -c -Wall -g -std=gnu99
# Unconnemt to enable two wire UART interface
#CFLAGS += -DSBL_TWO_WIRE

CC_HOST= gcc

PROJ_DIR=

OBJ_HOST=objs/host

.PHONY:: all
.PHONY:: host

all: cc-rom-sbl
host: cc-rom-sbl

${OBJ_HOST}:
	mkdir -p $@

cc-rom-sbl: ${OBJ_HOST}/main.o ${OBJ_HOST}/ccxxxxdnld.o ${OBJ_HOST}/sblUart.o
	$(CC_HOST) -o cc-rom-sbl ${OBJ_HOST}/main.o ${OBJ_HOST}/ccxxxxdnld.o ${OBJ_HOST}/sblUart.o

# rule for file "main.o".
${OBJ_HOST}/main.o: main.c ${OBJ_HOST}
	$(CC_HOST) $(CFLAGS) $(INCLUDE) $(DEFS) main.c -o $@

# rule for file "ccxxxxdnld.o".
${OBJ_HOST}/ccxxxxdnld.o: ccxxxxdnld.c  ${OBJ_HOST}
	$(CC_HOST) $(CFLAGS) $(INCLUDE) $(DEFS) ccxxxxdnld.c  -o $@

# rule for file "sblUart.o".
${OBJ_HOST}/sblUart.o: sblUart.h sblUart.c  ${OBJ_HOST}
	$(CC_HOST) $(CFLAGS) $(INCLUDE) $(DEFS) sblUart.c -o $@

# rule for cleaning files generated during compilations.
clean:
	/bin/rm -f  cc-rom-sbl
	/bin/rm -rf objs

remake: clean _default

