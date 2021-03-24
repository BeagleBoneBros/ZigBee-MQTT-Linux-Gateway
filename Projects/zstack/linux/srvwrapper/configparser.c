/*********************************************************************
 Filename:       configParser.c
 Revised:        $Date$
 Revision:       $Revision$

 Description:    Function to parse a .ini file and return the value given a name.


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
 *********************************************************************/

/*********************************************************************
 * INCLUDES
 *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "configparser.h"
#include "trace.h"

/*********************************************************************
 * MACROS
 *********************************************************************/

/*********************************************************************
 * CONSTANTS
 *********************************************************************/
#define MAX_STRING_LENGTH 180
#define MAX_UINT8_VALUE 255
#define MAX_INT8_VALUE 127
#define MIN_INT8_VALUE (-128)
#define MAX_UINT16_VALUE 65535
#define MAX_UINT32_VALUE 4294967295U
#define ERROR_FLAG -1
#define SUCCESS_FLAG 0

/*********************************************************************
 * TYPEDEFS
 *********************************************************************/

/*********************************************************************
 * GLOBAL VARIABLES
 *********************************************************************/

/*********************************************************************
 * EXTERNAL VARIABLES
 *********************************************************************/

/*********************************************************************
 * EXTERNAL FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * LOCAL VARIABLES
 *********************************************************************/

/*********************************************************************
 * Function Prototypes
 *********************************************************************/
static char * buffCleaner( char * target );

static int arrayParse( char * pArrayList, uint8_t size, int *result );

static int setArrayValue( uint8_t size, uint32_t *pLocation, int * pArrayOfNum,
                          uint8_t type );

static void setValue( uint8_t size, uint32_t *pLocation, int newValue, uint8_t type );

static void setStringValue( uint8_t size, char * pLocation, char * pValue );

static void stringCopy( char * pCopy, char *pOriginal );

static int stringToInt( char * str, int * val );

/********************************************************************
 ********************************************************************/

/*********************************************************************
 * @fn      parseConfigFile
 *
 * @brief   Loop through the structure and set all the values.
 *
 * @param   pFileName - File name of the file to search
 *
 * @param   pItems - Array of structues containing items that need to be set
 *
 * @return  uint8_t - Returns 0 if no errors occurred. Returns 1 otherwise.
 *
 *********************************************************************/
int parseConfigFile( char *pFileName, configTableItem_t * pItems,
                     uint8_t numItems )
{
  int numericValue;
  char parseKey[MAX_STRING_LENGTH];
  char lineBuffer[MAX_STRING_LENGTH];

  // Open the file
  FILE *pOpenFile;
  pOpenFile = fopen( pFileName, "r" );

  if ( pOpenFile == NULL )
  {
    return ERROR_FLAG;
  }

  // Get line by line
  while ( fgets( lineBuffer, MAX_STRING_LENGTH, pOpenFile ) != NULL )
  {
    char *pUpdateVar;
    char *cleanedBuffer = buffCleaner( lineBuffer );

    // Check first character to see if the line is a comment ';'
    if ( lineBuffer[0] == ';' )
    {
      continue;
    }

    // Check to see if there is a keyword in the line by looking for "="
    pUpdateVar = strchr( cleanedBuffer, '=' );

    if ( pUpdateVar != NULL )
    {
      uint8_t k;
      // Get a clean copy of the keyword
      pUpdateVar = buffCleaner( pUpdateVar );
      stringCopy( parseKey, cleanedBuffer );

      // Now search through the arrays for the pkeyword.
      for ( k = 0; k < numItems; ++k )
      {
        // If a keyword is a match convert it if needed, and set it into memory.
        if ( strcmp( pItems[k].pkeyword, parseKey ) == 0 )
        {
          // Value is a string
          if ( pItems[k].type == TYPE_STRING )
          {
            setStringValue( pItems[k].size, pItems[k].pAddr, pUpdateVar );
          }
          // Value is single numeric
          else
          {
            if ( pItems[k].size == 1 )
            {
              if ( stringToInt( pUpdateVar , &numericValue) == ERROR_FLAG )
              {
                uiPrintfEx(trERROR, "keyword '%s' - Value provided is not an uint type, skipping...\n", parseKey );
                break;
              }

              setValue( pItems[k].size, pItems[k].pAddr, numericValue,
                  pItems[k].type );
            }
            // Value is an array
            else
            {
              int errorCheck = 0;
              int arrayHolder[pItems[k].size];
              errorCheck = arrayParse( pUpdateVar, pItems[k].size,
                  arrayHolder );

              if ( errorCheck == ERROR_FLAG )
              {
                uiPrintfEx(trERROR, "keyword '%s' - Value provided is not an uint type, skipping...\n", parseKey );
                break;
              }

              errorCheck = setArrayValue( pItems[k].size, pItems[k].pAddr,
                  arrayHolder, pItems[k].type );

              if ( errorCheck == ERROR_FLAG )
              {
                uiPrintfEx(trERROR, "keyword '%s' - Array parsing error, skipping...\n", parseKey );
              }
            }
          }
        }
      }
    }
  }
  fclose( pOpenFile );
  return SUCCESS_FLAG;
}

/*********************************************************************
 * @fn      buffCleaner
 *
 * @brief   Strips whitespaces then an equal sign then any more whitespaces.
 *
 * @param   target - The char * that needs to be cleaned.
 *
 * @return  void
 *
 *********************************************************************/
static char * buffCleaner( char * pTarget )
{
  // Loop until no more white space, or equal signs.
  while ( pTarget[0] == ' ' || pTarget[0] == '=' )
  {
    ++pTarget;
  }
  return pTarget;
}

/*********************************************************************
 * @fn      arrayParse
 *
 * @brief   Converts a string that looks like an array into a numeric array.
 *
 * @param   pArrayList - The string that needs to be converted.
 *
 * @param   size - The number of elements in the array.
 *
 * @param   result - An array that the results will be stored in.
 *
 * @return  int - 0 for success, -1 for error
 *
 *********************************************************************/
static int arrayParse( char * pArrayList, uint8_t size, int *pResult )
{
  uint8_t i;
  int converted;
  char * buff;
  char * saveptr;

  buff = strtok_r( pArrayList, "\t {,", &saveptr );

  for ( i = 0; (i < size) && (buff != NULL); ++i )
  {
    // Convert the buffer depending on its type.
    if ( stringToInt( buff, &converted ) == ERROR_FLAG )
    {
      return ERROR_FLAG;
    }
    // Put the converted number into the result array. 
    pResult[i] = converted;
	
    buff = strtok_r( NULL, "\t ,}", &saveptr );
  }
  return SUCCESS_FLAG;
}

/*********************************************************************
 * @fn      setArrayValue
 *
 * @brief   Sets the orignal array with the new variables from the configuration file.
 *
 * @param   size - The number of elements in the array
 *
 * @param   pLocation - The pLocation of the variable that needs to be set
 *
 * @param   pArrayOfNum - The array of numbers that will be set.
 *
 * @param   type - The type of variable to be set.
 *
 * @return  int - 0 for success, -1 for error
 *
 *********************************************************************/
static int setArrayValue( uint8_t size, uint32_t *pLocation, int * pArrayOfNum,
                          uint8_t type )
{

  uint8_t *type0;
  uint16_t *type1;
  int8_t *type2;
  uint8_t i;

  switch ( type )
  {
    case TYPE_INT8:
      type2 = (int8_t*) pLocation;
      // Check for errors.
      for ( i = 0; i < size; ++i )
      {
        if ( pArrayOfNum[i] > MAX_INT8_VALUE )
        {
          uiPrintfEx(trERROR, "Value provided is greater than variable can hold\n" );
          return ERROR_FLAG;
        }
        if ( pArrayOfNum[i] < MIN_INT8_VALUE )
        {
          uiPrintfEx(trERROR, "Value provided is smaller than variable can hold\n" );
          return ERROR_FLAG;
        }
      }
      // Set the value
      for ( i = 0; i < size; ++i )
      {
        type2[i] = (int8_t) pArrayOfNum[i];
      }
      break;
	  
    case TYPE_UINT8:
      type0 = (uint8_t*) pLocation;
      // Check for errors.
      for ( i = 0; i < size; ++i )
      {
        if ( pArrayOfNum[i] > MAX_UINT8_VALUE )
        {
          uiPrintfEx(trERROR, "Value provided is greater than variable can hold\n" );
          return ERROR_FLAG;
        }
      }
      // Set the value
      for ( i = 0; i < size; ++i )
      {
        type0[i] = (uint8_t) pArrayOfNum[i];
      }
      break;

    case TYPE_UINT16:
      type1 = (uint16_t*) pLocation;
      // Check for errors
      for ( i = 0; i < size; ++i )
      {
        if ( pArrayOfNum[i] > MAX_UINT16_VALUE )
        {
          uiPrintfEx(trERROR, "Value provided is greater than variable can hold\n" );
          return ERROR_FLAG;
        }
      }
      // Set the value
      for ( i = 0; i < size; ++i )
      {
        type1[i] = (uint16_t) pArrayOfNum[i];
      }
      break;

    case TYPE_UINT32:
      // Check for errors
      for ( i = 0; i < size; ++i )
      {
        if ( pArrayOfNum[i] > MAX_UINT32_VALUE )
        {
          uiPrintfEx(trERROR, "Value provided is greater than variable can hold\n" );
          return ERROR_FLAG;
        }
      }
      // Set the value
      for ( i = 0; i < size; ++i )
      {
        pLocation[i] = (uint32_t) pArrayOfNum[i];
      }
      break;

    default:
      uiPrintfEx(trERROR, "Type provided is invalid\n" );
      return ERROR_FLAG;
      break;
  }
  return SUCCESS_FLAG;
}

/*********************************************************************
 * @fn      setValue
 *
 * @brief   Sets the orignal variable with the updated information from the configuration file.
 *
 * @param   size - The number of elements in the array
 *
 * @param   pLocation - The pLocation of the variable that needs to be set
 *
 * @param   newValue - The new numeric value that will be set into the variable.
 * 
 * @param   type - The type of variable to be set.
 *
 * @return  void
 *
 *********************************************************************/
static void setValue( uint8_t size, uint32_t *pLocation, int newValue, uint8_t type )
{
  switch ( type )
  {
    case TYPE_INT8:
      // Check for errors
      if ( newValue > MAX_INT8_VALUE )
      {
        uiPrintfEx(trERROR, "Value provided is greater than variable can hold\n" );
        break;
      }
      if ( newValue < MIN_INT8_VALUE )
      {
        uiPrintfEx(trERROR, "Value provided is smaller than variable can hold\n" );
        break;
      }
      // Set the value   
      *(int8_t*) (pLocation) = (int8_t) newValue;
      break;
	  
    case TYPE_UINT8:
      // Check for errors
      if ( newValue > MAX_UINT8_VALUE )
      {
        uiPrintfEx(trERROR, "Value provided is greater than variable can hold\n" );
        break;
      }
      // Set the value   
      *(uint8_t*) (pLocation) = (uint8_t) newValue;
      break;

    case TYPE_UINT16:
      // Check for errors
      if ( newValue > MAX_UINT16_VALUE )
      {
        uiPrintfEx(trERROR, "Value provided is greater than variable can hold\n" );
        break;
      }
      // Set the value
      *(uint16_t*) pLocation = (uint16_t) newValue;
      break;

    case TYPE_UINT32:
      // Check for errors
      if ( newValue > MAX_UINT32_VALUE )
      {
        uiPrintfEx(trERROR, "Value provided is greater than variable can hold\n" );
        break;
      }
      // Set the value
      *(uint32_t*) pLocation = (uint32_t) newValue;
      break;

    default:
      uiPrintfEx(trERROR,"Type provided is invalid\n" );
      break;
  }
}

/*********************************************************************
 * @fn      setStringValue
 *
 * @brief   Sets the orignal 
 *
 * @param   size - The number of elements in the array
 *
 * @param   pLocation - The pLocation of the variable that needs to be set
 *
 * @param   pValue - The value that will be set into the original variable.
 *
 * @return  void
 *
 *********************************************************************/
static void setStringValue( uint8_t size, char * pLocation, char * pValue )
{
  uint8_t i = 0;
  uint8_t k = 0;
  uint8_t totalSize = 0;
  uint8_t sizeOfValue = 0;
  char *pStringTok;
  char stringTemp[MAX_STRING_LENGTH];
  char * saveptr;

  pStringTok = strtok_r( pValue, " \n",  &saveptr );
  // Copy over all the tokens deleting any beginning, or trailing whitespace
  while ( pStringTok != NULL )
  {
    uint8_t j = 0;
    sizeOfValue = strlen( pStringTok );

    if ( k != 0 )
    {
      stringTemp[i] = ' ';
      ++i;
      ++totalSize;
    }

    for ( j = 0; j < sizeOfValue; ++j )
    {
      if ( pStringTok[j] == ';' )
      {
        // Comment marker need to stop copying
        break;
      }
      stringTemp[i] = pStringTok[j];
      ++i;
      ++totalSize;
    }

    pStringTok = strtok_r( NULL, " \n", &saveptr );
    ++k;
  }
  // Check for errors
  if ( size < totalSize )
  {
    uiPrintfEx(trDEBUG, "Value provided is greater than variable can hold\n" );
    return;
  }
  else if ( totalSize != 0 )
  {
    uint8_t h;

    for ( i = 0; i < size; ++i )
    {
      pLocation[i] = '\0';
    }
    // Set the string into the memory address.

    for ( h = 0; h < totalSize; ++h )
    {
      pLocation[h] = stringTemp[h];
    }
  }
}

/*********************************************************************
 * @fn      stringToInt
 *
 * @brief   Converts a string into an integer.
 *
 * @param   pValue - The string that needs to be converted.
 *
 * @return  int - 0 for success, -1 for error.
 *
 *********************************************************************/
static int stringToInt( char * str, int * val )
{
  char * pEnd;

  *val = strtol( str, &pEnd, 0 );

  if ((pEnd == str) || ((*pEnd != '\0') && (*pEnd != '\r') && (*pEnd != '\n')))
  {
    uiPrintfEx(trERROR,"stringToInt parsing error\n");
    return ERROR_FLAG;
  }

  return SUCCESS_FLAG;
}

/*********************************************************************
 * @fn      stringCopy
 *
 * @brief   Copies a string up untill an '=' or ' ' appears.
 *
 * @param   pOriginal - The string that needs to be converted.
 *
 * @param   pCopy - Where to store the copy of the stirng.
 *
 * @return  void
 *
 *********************************************************************/
static void stringCopy( char * pCopy, char *pOriginal )
{
  uint i = 0;

  while ( pOriginal[i] != '=' && pOriginal[i] != ' ' )
  {
    pCopy[i] = pOriginal[i];
    ++i;
  }
  pCopy[i] = '\0';
}

/*********************************************************************
 *
 * @fn          processUint32Flag
 *
 * @brief       Looks for flagLong or flagShort followed
 *              by a numeric value. If more than one occurance exist, 
 *              the rightmost takse effect. Any instance of this flag 
 *              found is removed from the argument list, together with
 *              its value. Return value is the number of times this flag 
 *              was found.
 *
 *********************************************************************/
int processUint32Flag(const char * flagLong, const char * flagShort, 
                      uint32_t * value, int * argc_p, char *argv[])
{
  int i;
  int new_i;
  int flagCount = 0;
  char * endptr;

  for (i = 1; i < *argc_p; i++)
  {
    new_i = i + (2 * flagCount);

    argv[i] = argv[new_i];

    if ((strcmp(argv[i] , flagLong) == 0) || (strcmp(argv[i] , flagShort) == 0))
    {
      if ((new_i + 1) >= *argc_p)
      {
        uiPrintf("ERROR: %s was specified without a value\n. Aborting now.", argv[i]);
        exit(-1);
      }

      *value = strtoul(argv[new_i + 1], &endptr, 0);

      if (*endptr != '\0')
      {
        uiPrintf("ERROR: Value following %s has invalid character(s): \"%s\"\n. Aborting now.", 
          argv[i], endptr);
        exit(-1);
      }

      (*argc_p) -= 2;

      flagCount++;

      i--;
    }
  }

  return flagCount;
}

/*********************************************************************
 *
 * @fn          processStringFlag
 *
 * @brief       Looks for flagLong or flagShort followed
 *              by a string value. If more than one occurance exist, 
 *              the rightmost takse effect. Any instance of this flag 
 *              found is removed from the argument list, together with
 *              its value. Return value is the number of times this flag 
 *              was found.
 *
 *********************************************************************/
int processStringFlag(const char * flagLong, const char * flagShort, 
                      char * value, int size, int * argc_p, char *argv[])
{
  int i;
  int new_i;
  int flagCount = 0;

  for (i = 1; i < *argc_p; i++)
  {
    new_i = i + (2 * flagCount);

    argv[i] = argv[new_i];

    if ((strcmp(argv[i] , flagLong) == 0) || (strcmp(argv[i] , flagShort) == 0))
    {
      if ((new_i + 1) >= *argc_p)
      {
        uiPrintf("ERROR: %s was specified without a value\n. Aborting now.", argv[i]);
        exit(-1);
      }

      if (strlen(argv[new_i + 1]) >= size)
      {
        uiPrintf("ERROR: String following %s is too long. Maximum supported length is %d. Aborting now.", 
          argv[i], size - 1);
        exit(-1);
      }

      strcpy(value, argv[new_i + 1]);

      (*argc_p) -= 2;

      flagCount++;

      i--;
    }
  }

  return flagCount;
}

