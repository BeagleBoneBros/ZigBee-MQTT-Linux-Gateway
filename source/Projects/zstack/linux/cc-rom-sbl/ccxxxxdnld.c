/******************************************************************************

 @file cc13xxdnld.c

 @brief API for downloading to the CC13xx/CC26xx/CC2538 Flash using the ROM
        Bootloader

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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/time.h>

#include "ccxxxxdnld.h"
#include "deviceTypes.h"

/* \brief CcDnld SBL Commands */
typedef enum
{
    CCDNLD_CMD_PING                 = 0x20,
    CCDNLD_CMD_DOWNLOAD             = 0x21,
    CCDNLD_CMD_GET_STATUS           = 0x23,
    CCDNLD_CMD_SEND_DATA            = 0x24,
    CCDNLD_CMD_RESET                = 0x25,
    CCDNLD_CMD_SECTOR_ERASE         = 0x26,
    CCDNLD_CMD_CRC32                = 0x27,
    CCDNLD_CMD_GET_CHIP_ID          = 0x28,
    CCDNLD_CMD_MEMORY_READ          = 0x2A,
    CCDNLD_CMD_MEMORY_WRITE         = 0x2B,
    CCDNLD_CMD_BANK_ERASE           = 0x2C,
    CCDNLD_CMD_SET_CCFG             = 0x2D,
    CCDNLD_CMD_RET_SUCCESS          = 0x40,
    CCDNLD_CMD_RET_UNKNOWN_CMD      = 0x41,
    CCDNLD_CMD_RET_INVALID_CMD      = 0x42,
    CCDNLD_CMD_RET_INVALID_ADR      = 0x43,
    CCDNLD_CMD_RET_FLASH_FAIL       = 0x44,
} CcDnld_Commands;

/* \brief CcDnld SBL Ack's */
typedef enum
{
    CCDNLD_DEVICE_ACK               = 0xCC,
    CCDNLD_DEVICE_NACK              = 0x33,
} CcDnld_Dev_Ack;

static CcDnld_UartFxns_t* uartFxns;

static uint8_t generateCheckSum(uint8_t cmdId, const uint8_t *pData, uint32_t dataLen);
static CcDnld_Status sendCmd(uint8_t cmdId, const uint8_t *pSendData, uint32_t sendLen);
static CcDnld_Status getCmdResponse(CcDnld_Dev_Ack* bAck);
static CcDnld_Status getResponseData(uint8_t *pData, uint32_t *maxLen);
static CcDnld_Status sendCmdResponse(CcDnld_Dev_Ack bAck);
static CcDnld_Status readStatus(uint8_t *status);
static void ui32ToByteArray(uint32_t src, uint8_t *pDst);
static void byteArrayToUi32(const uint8_t* pSrc, uint32_t *pDst);
static CcDnld_Status addressInFlash(uint32_t startAddress, uint32_t byteCount, DeviceType_t deviceType);
static uint32_t calcVerifyCrc(const uint8_t *pData, uint32_t byteCount);

//*****************************************************************************
//
//! \brief Initialises UART function pointer table.
//!
//! This function initializes the UART functions for sending SBL commands to the
//! CC13/26xx
//!
//! \param uartFxns - The UART function pioiter table.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
void CcDnld_init(CcDnld_UartFxns_t* ccDnld_uartFxns)
{
    uartFxns = ccDnld_uartFxns;
}

//*****************************************************************************
//
//! \brief Establishes a connnection with the ROM Bootloader.
//!
//! This function establishes a connnection with the ROM Bootloader by setting
//! the autoboard rate. By sending 0x55 the CC163/26xx ROM bootloader uses the
//! alternating 0's/1's to detect the board rate and sends an Ack/0xCC in
//! response
//!
//! \param None.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_connect()
{
    CcDnld_Dev_Ack devAck = 0;

    /* Send 0x55 0x55 and expect ACK */
    uartFxns->sbl_UartWriteByte(0x55);
    uartFxns->sbl_UartWriteByte(0x55);

    if(getCmdResponse(&devAck) != CcDnld_Status_Success)
    {
         return CcDnld_Status_Cmd_Error;
    }

    return CcDnld_Status_Success;
}

DeviceType_t CcDnld_parseDeviceType(char * deviceType)
{
  // ignore case of user input
  for(int i = 0; deviceType[i]; i++){
    deviceType[i] = tolower(deviceType[i]);
  }

  if((strcmp(deviceType, "cc13x0") == 0) || 
     (strcmp(deviceType, "cc1310") == 0) ||
     (strcmp(deviceType, "cc1350") == 0))
  {
      return DEVICE_CC13X0;
  }
  else if((strcmp(deviceType, "cc13x2") == 0) ||
          (strcmp(deviceType, "cc1312") == 0) ||
          (strcmp(deviceType, "cc1352") == 0))
  {
      return DEVICE_CC13X2;
  }
  else if((strcmp(deviceType, "cc26x0") == 0) ||
          (strcmp(deviceType, "cc2630") == 0) || 
          (strcmp(deviceType, "cc2640") == 0) ||
          (strcmp(deviceType, "cc2650") == 0))
  {
      return DEVICE_CC26X0;
  }
  else if((strcmp(deviceType, "cc26x2") == 0) ||
          (strcmp(deviceType, "cc2652") == 0))
  {
      return DEVICE_CC26X2;
  }
  else if((strcmp(deviceType, "cc2538") == 0))
  {
      return DEVICE_CC2538;
  }
  else {
      /* Invalid device type */
      return INVALID_DEVICE;
  }
}

//*****************************************************************************
//
//! \brief Determines the deviceId for specific # pages and page size.
//!
//! This function requires an existing connection with the ROM Bootloader by 
//! calling CcDnld_connect(). After the connection has been made successfully 
//! this function can request the deviceId from the ROM Bootloader. This 
//! deviceId can then be used in partnership with the CCDNLD_CCX[0/2]_DEV_ID
//! constants to choose the correct CCDNLD_CC13X[0/2]_PAGE_SIZE and 
//! CCDNLD_CC13X[0/2]_NUM_PAGES in your application logic.
//!
//! \param deviceId - pointer to store the ROM Bootloader's deviceId.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_determineDeviceId(uint32_t *deviceId)
{
    CcDnld_Status retCode = CcDnld_Status_Success;
    CcDnld_Dev_Ack devAck = 0;
    uint8_t retBuffer[4];

    /* Send command */
    if((retCode = sendCmd(CCDNLD_CMD_GET_CHIP_ID, 0, 0)) != CcDnld_Status_Success)
    {
        return retCode;
    }

    /* Receive command response (ACK/NAK) */
    if((retCode = getCmdResponse(&devAck)) != CcDnld_Status_Success)
    {
        return retCode;
    }

   if(devAck != CCDNLD_DEVICE_ACK)
    {
        return CcDnld_Status_Cmd_Error;
    }

    /* Receive command response data */
    uint32_t numBytes = 4;
    if((retCode = getResponseData(retBuffer, &numBytes)) != CcDnld_Status_Success)
    {
            //
            // Respond with NAK
            //
            sendCmdResponse(CCDNLD_DEVICE_NACK);
            return retCode;
    }

    /* Respond with ACK */
    sendCmdResponse(CCDNLD_DEVICE_ACK);

    byteArrayToUi32(retBuffer, deviceId);

    return CcDnld_Status_Success;
}

//*****************************************************************************
//
//! \brief Gets the number of pages needing to be erased Erases the specified page.
//!
//! This function gets the number of pages needing to be erased based on Flash start address
//! and the Bytes to be written
//!
//! \param startAddress - The Flash start address.
//! \param byteCount        - Number of bytes to be erased.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_flashEraseRange(uint32_t startAddress, uint32_t byteCount, DeviceType_t deviceType)
{
    // CC2538 expects [4] Address + [4] byteCount in erase payload
    // Others expect [4] Address in erase payload
    uint8_t payloadSize = (deviceType == DEVICE_CC2538) ? 8 : 4;
    uint8_t eraseCmd[payloadSize];

    uint32_t pageIdx;
    uint32_t pageCount;
    uint32_t retCode = CcDnld_Status_Success;
    CcDnld_Dev_Ack devAck;
    uint8_t devStatus = 0;

    uint32_t pageSize = CcDnld_getPageSize(deviceType);

    pageCount = byteCount / pageSize;
    if(byteCount % pageSize) pageCount ++;

    for(pageIdx = 0; pageIdx < pageCount; pageIdx++)
    {
        /* Build payload - 4B address (MSB first) */
        ui32ToByteArray(startAddress + pageIdx*(pageSize), &eraseCmd[0]);

        // CC2538 also expects byteCount in erase payload
        if(deviceType == DEVICE_CC2538)
        {
          ui32ToByteArray(byteCount, &eraseCmd[4]);
        }

        /* Send command */
        if((retCode = sendCmd(CCDNLD_CMD_SECTOR_ERASE, eraseCmd, sizeof(eraseCmd))) != CcDnld_Status_Success)
        {
            return retCode;
        }

        /* Receive command response (ACK/NAK) */
        if((retCode = getCmdResponse(&devAck)) != CcDnld_Status_Success)
        {
            return retCode;
        }
        if(devAck != CCDNLD_DEVICE_ACK)
        {
            return CcDnld_Status_Cmd_Error;
        }

        /* Check device status (Flash failed if page(s) locked) */
        readStatus(&devStatus);
        if(devStatus != CCDNLD_CMD_RET_SUCCESS)
        {
            return CcDnld_Status_State_Error;
        }
    }

    return retCode;
}

//*****************************************************************************
//
//! \brief Starts the download to the CC13/26xx ROM bootloader.
//!
//! This function starts the download to the CC13/26xx ROM Bootloader
//!
//! \param startAddress - The Flash start address.
//! \param byteCount        - Total Number of bytes to be programmed.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_startDownload(uint32_t address, uint32_t size,  DeviceType_t deviceType)
{
    CcDnld_Status retCode = CcDnld_Status_Success;
    CcDnld_Dev_Ack devAck;
    uint8_t devStatus;

    /* Check input arguments */
    if(addressInFlash(address, size, deviceType) != CcDnld_Status_Success)
    {
        return CcDnld_Status_Param_Error;
    }

    /*
       Generate payload
       - 4B Program address
       - 4B Program size
    */
    uint8_t pPayload[8];
    ui32ToByteArray(address, &pPayload[0]);
    ui32ToByteArray(size, &pPayload[4]);

    /* Send command */
    if((retCode = sendCmd(CCDNLD_CMD_DOWNLOAD, pPayload, 8)) != CcDnld_Status_Success)
    {
        return retCode;
    }

    /* Receive command response (ACK/NAK) */
    if((retCode = getCmdResponse(&devAck)) != CcDnld_Status_Success)
    {
        return retCode;
    }

    /* Check device status */
    devStatus = 0;
    readStatus(&devStatus);
    if(devStatus != CCDNLD_CMD_RET_SUCCESS)
    {
        return CcDnld_Status_State_Error;
    }

    /* Return command response */
    return (devAck == CCDNLD_DEVICE_ACK) ? CcDnld_Status_Success : CcDnld_Status_Cmd_Error;
}

//*****************************************************************************
//
//! \brief Loads a chunk of data into the CC13/26xx Flash.
//!
//! This function Loads a chunk of data into the CC13/26xx Flash thorugh the
//! ROM bootloader
//!
//! \param startAddress - The Flash start address.
//! \param pData                        - Pointer to a buffer containing the data.
//! \param byteCount        - Number of bytes to be programmed.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_sendData(uint8_t *pData, uint32_t byteCount)
{
    CcDnld_Status retCode = CcDnld_Status_Success;
    CcDnld_Dev_Ack devAck;
    uint8_t devStatus;

    /* Check input arguments */
    if(byteCount > CCDNLD_MAX_BYTES_PER_TRANSFER)
    {
            return CcDnld_Status_Param_Error;
    }

    /* Send command */
    if((retCode = sendCmd(CCDNLD_CMD_SEND_DATA, pData, byteCount)) !=
                                                         CcDnld_Status_Success)
    {
        return retCode;
    }

    /* Receive command response (ACK/NAK) */
    if((retCode = getCmdResponse(&devAck)) != CcDnld_Status_Success)
    {
        return retCode;
    }
    if(devAck != CCDNLD_DEVICE_ACK)
    {
        return CcDnld_Status_Cmd_Error;
    }

    devStatus = 0;

    /* Check device status */
    readStatus(&devStatus);
    if(devStatus != CCDNLD_CMD_RET_SUCCESS)
    {
        return CcDnld_Status_State_Error;
    }

    return CcDnld_Status_Success;
}

CcDnld_Status CcDnld_resetDevice(void)
{
    CcDnld_Status retCode = CcDnld_Status_Success;
    CcDnld_Dev_Ack devAck;

    /* Send command */
    if((retCode = sendCmd(CCDNLD_CMD_RESET, 0, 0)) != CcDnld_Status_Success)
    {
        return retCode;
    }

    /* Receive command response (ACK/NAK) */
    if((retCode = getCmdResponse(&devAck)) != CcDnld_Status_Success)
    {
        return retCode;
    }
    if(devAck != CCDNLD_DEVICE_ACK)
    {
        return CcDnld_Status_Cmd_Error;
    }

    return CcDnld_Status_Success;
}

//*****************************************************************************
//
//! \brief Loads a chunk of data into the CC13/26xx Flash.
//!
//! This function Loads a chunk of data into the CC13/26xx Flash thorugh the
//! ROM bootloader
//!
//! \param dataAddress  - start address of data in Flash to be verified.
//! \param pData                - Pointer to a buffer containing the data to be
//!                                             verified.
//! \param byteCount        - Number of bytes to be programmed.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_verifyData(uint32_t dataAddress, uint8_t *pData,
                                uint32_t byteCount, DeviceType_t deviceType)
{
    // CC2538 expects [4] Address + [4] byteCount in verify payload
    // Others expect [4] Address + [4] byteCount + [4] "Repeat count" in verify payload
    uint8_t payloadSize = (deviceType == DEVICE_CC2538) ? 8 : 12;
    uint8_t crcCommandPayload[payloadSize];

    uint32_t localCrc;
    uint32_t targetCrc;
    uint8_t crcCommandResp[4];
    CcDnld_Dev_Ack devAck = 0;
    CcDnld_Status retCode;

    /*
       Build CRC command payload (MSB first)
       - 4B Start address
       - 4B Size
       - 4B Repeat count (0)
    */

    ui32ToByteArray(dataAddress, &crcCommandPayload[0]);
    ui32ToByteArray(byteCount, &crcCommandPayload[4]);

    // CC2538 does not expect "Repeat count"
    if(deviceType != DEVICE_CC2538)
    {
      ui32ToByteArray(0, &crcCommandPayload[8]);
    }

    /* Send command */
    if((retCode = sendCmd(CCDNLD_CMD_CRC32, crcCommandPayload, sizeof(crcCommandPayload))) !=
                                                         CcDnld_Status_Success)
    {
        return retCode;
    }

    /* Receive command response (ACK/NAK) */
    if((retCode = getCmdResponse(&devAck)) != CcDnld_Status_Success)
    {
        return retCode;
    }

    if(devAck != CCDNLD_DEVICE_ACK)
    {
        return CcDnld_Status_Cmd_Error;
    }

    /* Receive command response data */
    uint32_t numBytes = 4;
    if((retCode = getResponseData(crcCommandResp, &numBytes)) !=
                                                         CcDnld_Status_Success)
    {
        /* Respond with NAK */
        sendCmdResponse(CCDNLD_DEVICE_NACK);
        return retCode;
    }

    /* Respond with ACK */
    sendCmdResponse(CCDNLD_DEVICE_ACK);

    byteArrayToUi32(crcCommandResp, &targetCrc);
    localCrc = calcVerifyCrc(pData, byteCount);

    if(localCrc != targetCrc)
    {
        return CcDnld_Status_Crc_Error;
    }

    return CcDnld_Status_Success;
}

/** \brief Utility function to obtain the page size based on the targets device type.
 *
 * \param[in] deviceType
 *          String containing the device type [cc13x0, cc13x2, cc26x0, cc26x2]
 * \return
 *          uint32_t pageSize - The flash page size for the target device type, 0 if Id not found.
 */
uint32_t CcDnld_getPageSize(DeviceType_t deviceType)
{
    uint32_t pageSize;

    /* Get page size */
    if(deviceType == DEVICE_CC13X0 || deviceType == DEVICE_CC26X0)
    {
        pageSize = CCDNLD_CC13X0_PAGE_SIZE;
    }
    else if(deviceType == DEVICE_CC13X2 || deviceType == DEVICE_CC26X2)
    {
        pageSize = CCDNLD_CC13X2_PAGE_SIZE;
    }
    else if(deviceType == DEVICE_CC2538)
    {
        pageSize = CCDNLD_CC2538_PAGE_SIZE;
    }
    else {
        /* Invalid device type */
        pageSize= 0;
    }
    return (pageSize);
}


/** \brief Utility function to obtain the number of available pages based on the targets device type.
 *
 * \param[in] deviceType
 *          String containing the device type [cc13x0, cc13x2, cc26x0, cc26x2]
 * \return
 *          uint32_t numPages - The number of available flash pages for the target device type, 0 if Id not found.
 */
uint32_t CcDnld_getNumPages(DeviceType_t deviceType)
{
    uint32_t numPages;

    /* Get number of pages supported */
    if(deviceType == DEVICE_CC13X0 || deviceType == DEVICE_CC26X0)
    {
        numPages = CCDNLD_CC13X0_NUM_PAGES;
    }
    else if(deviceType == DEVICE_CC13X2 || deviceType == DEVICE_CC26X2)
    {
        numPages = CCDNLD_CC13X2_NUM_PAGES;
    }
    else if(deviceType == DEVICE_CC2538)
    {
        numPages = CCDNLD_CC2538_NUM_PAGES;
    }
    else {
        /* Invalid device type */
        numPages= 0;
    }
    return (numPages);

}

/*
 * Private Helper Functions
 *
 */

/** \brief This function caclualtes the flash checksum
 *
 * \param[in] pData
 *          pointer to the data buffer
 * \param[in] byteCount
 *          length of data buffer.
 *
 * \return
 *          Returns CcDnld_Status_Success, ...
 */
static uint32_t calcVerifyCrc(const uint8_t *pData, uint32_t byteCount)
{
    uint32_t d, ind;
    uint32_t acc = 0xFFFFFFFF;
    const uint32_t crcRand32Lut[] =
    {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
        0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
        0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
    };

    while (byteCount--)
    {
        d = *pData++;
        ind = (acc & 0x0F) ^ (d & 0x0F);
        acc = (acc >> 4) ^ crcRand32Lut[ind];
        ind = (acc & 0x0F) ^ (d >> 4);
        acc = (acc >> 4) ^ crcRand32Lut[ind];
    }

    return (acc ^ 0xFFFFFFFF);
}

/** \brief This function caclualtes the command checksum
 *
 * \param[in] cmdId
 *          The Command ID
 * \param[in] pData
 *          pointer to the data buffer
 * \param[in] dataLen
 *          length of data buffer.
 *
 * \return
 *          Returns CcDnld_Status_Success, ...
 */
static uint8_t generateCheckSum(uint8_t cmdId, const uint8_t *pData,
                                                              uint32_t dataLen)
{
    uint8_t checkSum = cmdId;
    uint32_t i;

    for(i = 0; i < dataLen; i++)
    {
            checkSum += pData[i];
    }

    return checkSum;
}

/** \brief This function sends a command
 *
 * \param[in] cmdId
 *          The Command ID
 * \param[in] pSendData
 *          pointer to the data buffer
 * \param[in] sendLen
 *          length of data buffer.
 *
 * \return
 *          Returns CcDnld_Status_Success, ...
 */
static CcDnld_Status sendCmd(uint8_t cmdId, const uint8_t *pSendData,
                                                uint32_t sendLen)
{
    uint8_t pktLen = sendLen + 3; // +3 => <1b Length>, <1B cksum>, <1B cmd>
    uint8_t pktSum = generateCheckSum(cmdId, pSendData, sendLen);
    uint32_t i;

    /* Send packet */
    uartFxns->sbl_UartWriteByte(pktLen);
    uartFxns->sbl_UartWriteByte(pktSum);
    uartFxns->sbl_UartWriteByte(cmdId);

    for(i=0;i<sendLen;i++)
    {
        uartFxns->sbl_UartWriteByte(*(pSendData+i));
    }

    return CcDnld_Status_Success;
}

/** \brief Get ACK/NAK from the CC26xx/13xx device.
 *
 * \param[out] bAck
 *          CCDNLD_DEVICE_ACK or CCDNLD_DEVICE_NACK
 *
 * \return
 *          Returns CcDnld_Status_Success, ...
 */
static CcDnld_Status getCmdResponse(CcDnld_Dev_Ack* bAck)
{
    uint8_t pIn[2];
    uint32_t recvData = 0;
    uint32_t bytesRecv = 0;

    /* Expect 2 bytes (ACK or NAK) */
    do
    {
        recvData = uartFxns->sbl_UartReadByte();
        if((recvData&0x00000500) == 0)
        {
            pIn[bytesRecv]=(uint8_t) recvData;
            bytesRecv += 1;
        }
    }
    while(bytesRecv < 2);

    if((bytesRecv >= 2) &&
            (pIn[0] == 0x00 &&
            (pIn[1] == CCDNLD_DEVICE_ACK || pIn[1] == CCDNLD_DEVICE_NACK)))
    {
            *bAck = pIn[1];
            return CcDnld_Status_Success;
    }

    return CcDnld_Status_Cmd_Error;
}

/** \brief Get response data from CC13x/26xx device.
 *
 * \param[out] pData
 *          Pointer to where received data is stored.
 * \param[in|out] maxLen
 *          Max number of bytes that can be received. Is populated with the
 *          actual number of bytes received.
 * \return
 *          Returns CcDnld_Status_Success, ...
 */
static CcDnld_Status getResponseData(uint8_t *pData, uint32_t *maxLen)
{
    uint8_t pHdr[2];
    uint32_t numPayloadBytes;
    uint8_t hdrChecksum, dataChecksum;
    uint32_t bytesRecv = 0;

    /* Read length and checksum */
    do
    {
     pHdr[bytesRecv]=(uint8_t) uartFxns->sbl_UartReadByte();
     bytesRecv += 1;
    }
    while(bytesRecv < 2);

    numPayloadBytes = pHdr[0]-2;
    hdrChecksum = pHdr[1];

    /* Check if length byte is too long. */
    if(numPayloadBytes > *maxLen)
    {
        return CcDnld_Status_Cmd_Error;
    }

    /* Read the payload data */
    bytesRecv = 0;

    do
    {
        *(pData + bytesRecv) = (uint8_t) uartFxns->sbl_UartReadByte();
        bytesRecv++;
    }
    while(bytesRecv < numPayloadBytes);

    /* Verify data checksum */
    dataChecksum = generateCheckSum(0, pData, numPayloadBytes);
    if(dataChecksum != hdrChecksum)
    {
        return CcDnld_Status_Cmd_Error;
    }

    *maxLen = bytesRecv;
    return CcDnld_Status_Success;
}

/** \brief Send command response (ACK/NAK) to CC13x/26xx device.
 *
 * \param[in] bAck
 *          True if response is ACK, false if response is NAK.
 * \return
 *          Returns CcDnld_Status_Success, ...
 */
static CcDnld_Status sendCmdResponse(CcDnld_Dev_Ack bAck)
{

        /* Send response */
        uartFxns->sbl_UartWriteByte(0x00);
        uartFxns->sbl_UartWriteByte(bAck);

        return CcDnld_Status_Success;
}

//-----------------------------------------------------------------------------
/** \brief Get status from device.
 *
 * \param[out] pStatus
 *          Pointer to where status is stored.
 * \return
 *          Returns CcDnld_Status_Success, ...
 */
//-----------------------------------------------------------------------------
static CcDnld_Status readStatus(uint8_t *status)
{
    CcDnld_Status retCode;
    CcDnld_Dev_Ack devAck;

    /* Send command */
    if((retCode = sendCmd(CCDNLD_CMD_GET_STATUS,0,0)) != CcDnld_Status_Success)
    {
        return retCode;
    }

    /* Receive command response */
    if((retCode = getCmdResponse(&devAck)) != CcDnld_Status_Success)
    {
        return retCode;
    }

    if(devAck != CCDNLD_DEVICE_ACK)
    {
        return CcDnld_Status_Cmd_Error;
    }

    /* Receive command response data */
    uint32_t numBytes = 1;
    if((retCode = getResponseData(status, &numBytes)) != CcDnld_Status_Success)
    {
            //
            // Respond with NAK
            //
            sendCmdResponse(CCDNLD_DEVICE_NACK);
            return retCode;
    }

    /* Respond with ACK */
    sendCmdResponse(CCDNLD_DEVICE_ACK);

    return CcDnld_Status_Success;
}

/** \brief Utility function for splitting 32 bit variable into uint8_t array
 *          (4 elements). Data are converted MSB, ie. \e pcDst[0] is the
 *          most significant byte.
 *
 * \param[in] src
 *          The 32 bit variable to convert.
 * \param[out] pDst
 *          Pointer to the uint8_t array where the data is stored.
 *
 * \return
 *          void
 */
static void ui32ToByteArray(uint32_t src, uint8_t *pDst)
{
        /* MSB first */
        pDst[0] =    (uint8_t)(src >> 24);
        pDst[1] =    (uint8_t)(src >> 16);
        pDst[2] =    (uint8_t)(src >> 8);
        pDst[3] =    (uint8_t)(src >> 0);
}

/** \brief Utility function for converting uint8_t array into 32 bit variable
 *          (4 elements). Data are converted MSB, ie. \e pcDst[0] is the
 *          most significant byte.
 *
 * \param[in] pSrc
 *          The 32 bit variable to convert.
 * \param[out] pDst
 *          Pointer to the uint8_t array where the data is stored.
 *
 * \return
 *          void
 */
static void byteArrayToUi32(const uint8_t* pSrc, uint32_t *pDst)
{
        *pDst = (pSrc[0] << 24) & 0xFF000000;
        *pDst |= (pSrc[1] << 16) & 0x00FF0000;
        *pDst |= (pSrc[2] << 8) & 0x0000FF00;
        *pDst |= pSrc[3] & 0x000000FF;
}

/** \brief This function checks if the specified startAddress (and range)
 *          is located within the device flash.
 *
 * \return
 *          Returns CcDnld_Status_Success if the address/range is within the device
 *          flash.
 */
static CcDnld_Status addressInFlash(uint32_t startAddress, uint32_t byteCount, DeviceType_t deviceType)
{
    uint32_t endAddr = startAddress + byteCount;
    uint32_t checkAddress = (deviceType == DEVICE_CC2538) ? CCDNLD_CC2538_FLASH_START_ADDRESS : CCDNLD_FLASH_START_ADDRESS;

    uint32_t pageSize = CcDnld_getPageSize(deviceType);
    uint32_t numPages = CcDnld_getNumPages(deviceType);

    if(startAddress < checkAddress)
    {
        return CcDnld_Status_Param_Error;
    }
    if(endAddr > (checkAddress + (pageSize * numPages)))
    {
        return CcDnld_Status_Param_Error;
    }

    return CcDnld_Status_Success;
}

/*
 *  ========================================
 *  Texas Instruments Micro Controller Style
 *  ========================================
 *  Local Variables:
 *  mode: c
 *  c-file-style: "bsd"
 *  tab-width: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  End:
 *  vim:set  filetype=c tabstop=4 shiftwidth=4 expandtab=true
 */

