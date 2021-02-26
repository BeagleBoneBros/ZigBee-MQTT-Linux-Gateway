/*******************************************************************************
 Filename:       OtaServer_db.h
 Revised:        $Date$
 Revision:       $Revision$

 Description:    Interface for saving OtaServer RAM contents to database file

 Copyright 2013-2014 Texas Instruments Incorporated. All rights reserved.

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

*****************************************************************************/

#ifndef OTASERVER_DB_H
#define OTASERVER_DB_H

#ifdef __cplusplus
extern "C"
{
#endif


/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stdbool.h>

#include "OtaServer.h"

//states of db entries
#define FILEENTRY_STATE_NOT_ACTIVE    0
#define FILEENTRY_STATE_ACTIVE        1
#define MAX_SUPPORTED_FILE_NAME_LENGTH 128 

typedef struct {
  char * fileName;
  uint8 executionType;
  uint32 executionDelay;
  uint32 executionTime; 
  uint8 deviceNumber;
  uint64_t * deviceList;
} fileDbInfo_t;


/*
 * otaListAddEntry - create a device and add a rec to the list.
 */
void otaListAddEntry( fileDbInfo_t *epInfo);

/*
 * otaListRemoveEntryByNaEp - remove a device rec from the list.
fileDbInfo_t * otaListRemoveEntryByNaEp( uint16_t nwkAddr, uint8_t endpoint );
 */

/*
 * otaListNumEntries - get the number of devices in the list.
 */
uint32_t otaListNumEntries( void );

/*
 * otaListInitDatabase - restore device list from file.
 */
bool otaListInitDatabase( char * dbFilename );

fileDbInfo_t * otaListGetNextEntry(int *context);

fileDbInfo_t * otaListGetDeviceByFilename(char * fileName);

void otaListReleaseRecord (fileDbInfo_t * entry);

/*
fileDbInfo_t * otaListGetDeviceByNaEp( uint16_t nwkAddr, uint8_t endpoint );
*/

fileDbInfo_t * otaListRemoveEntryByFilename( char * fileName);

bool otaListCloseDatabase(void);

#ifdef __cplusplus
}
#endif

#endif /* OTASERVER_DB_H */
