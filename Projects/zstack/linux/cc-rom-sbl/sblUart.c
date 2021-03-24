/******************************************************************************

 @file sblUart.c

 @brief CCxxxx ROM Bootloader Platform Specific UART functions

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

/*********************************************************************
 * INCLUDES
 */
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "sblUart.h"
#include "deviceTypes.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static int serialPortFd;
static struct termios tioOld;
/*********************************************************************
 * API FUNCTIONS
 */

int SblUart_open(const char *devicePath, DeviceType_t deviceType)
{
    struct termios tio;
    int rtn;

    /* open the device */
    serialPortFd = open(devicePath, O_RDWR | O_NOCTTY);
    if (serialPortFd < 0)
    {
        perror(devicePath);
        printf("sblUartOpen: %s open failed\n", devicePath);
        return (-1);
    }

    rtn = tcgetattr(serialPortFd, &tioOld);
    if(rtn == -1)
    {
        printf("sblUartOpen: tcgetattr error: %d\n", rtn);
        return (-1);
    }

    bzero(&tio, sizeof(tio));

    if(deviceType == DEVICE_CC2538)
    {
      /* set Baud rate */
      rtn = cfsetspeed(&tio, B460800);
    }
    else
    {
      /* set Baud rate */
      rtn = cfsetspeed(&tio, B1500000);
    }

    if(rtn == -1)
    {
        printf("sblUartOpen: cfsetspeed error: %d\n", rtn);
        return (-1);
    }

    /* Set raw mode:
        tio->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                        | INLCR | IGNCR | ICRNL | IXON);
        tio->c_oflag &= ~OPOST;
        tio->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        tio->c_cflag &= ~(CSIZE | PARENB);
        tio->c_cflag |= CS8;
    */
    cfmakeraw(&tio);

    /* Specifically: 1 stop bit!
     *
     * the CC1310 boot loader uses exactly 1 stop bit.
     * And will transmit bytes back to back with ZERO
     * delay between bytes. The only delay is exactly
     * the stopbit count.
     *
     * The USB-ACM (usb-serial) uart on the launchpad
     * will reject these bytes because they are not
     * properly framed. Other serial interfaces will
     * accept the byte (for example an FTDI chip)
     *
     * The result: the bootloader cannot connect.
     */
    if(deviceType != DEVICE_CC2538)
    {
      tio.c_cflag &= (~CSTOPB);
    }

#ifdef SBL_TWO_WIRE
    tio.c_iflag &= ~(IXOFF);
    tio.c_cflag |= (CREAD | CLOCAL);
#endif

    // tio.c_cflag = B460800 | CS8 | CLOCAL | CREAD;
    // tio.c_iflag = IGNPAR;
    // tio.c_oflag = 0;

    /* Make it block for 200ms */
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 2;

    tcflush(serialPortFd, TCIFLUSH);
    rtn = tcsetattr(serialPortFd, TCSANOW, (const struct termios *) (&tio));
    if(rtn == -1)
    {
        printf("sblUartOpen: tcsetattr error: %d\n", rtn);
        return (-1);
    }

    return 0;
}

bool SblUart_close(void)
{
    int rtn;
    bool ret = true;

    tcflush(serialPortFd, TCOFLUSH);

    rtn = tcsetattr(serialPortFd, TCSANOW, (const struct termios *) (&tioOld));
    if(rtn == -1)
    {
        printf("SblUart_close: tcsetattr error: %d %s\n", rtn, strerror(errno));
        ret = false;
    }
    else
    {
        /* Verify we can close the port */
        rtn = close(serialPortFd);
        if(rtn == -1)
        {
            printf("SblUart_close: Port close failure: %d %s\n", rtn, strerror(errno));
            ret = false;
        }
    }

    return (ret);
}

void SblUart_write(const unsigned char* buf, size_t len)
{
#ifdef SBL_TWO_WIRE
  if(write(serialPortFd, buf, len) != len)
  {
    printf("Write failed... Flushing.\n");
    tcflush(serialPortFd, TCOFLUSH);
    write (serialPortFd, buf, len);
  }
#else
    write (serialPortFd, buf, len);
    tcflush(serialPortFd, TCOFLUSH);
#endif
    return;
}

unsigned char SblUart_read(unsigned char* buf, size_t len)
{
    unsigned char ret = read(serialPortFd, buf, len);

    return (ret);
}
