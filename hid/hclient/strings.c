/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    strings.c

Abstract:

    This module contains code for converting data buffers and integer values
    to and from string representation for display.

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

#include <windows.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <intsafe.h>
#include "strings.h"
#include <strsafe.h>

#pragma warning(disable:4057) // indirection to slightly different base types
#pragma warning(disable:28146) // Warning is meant for kernel mode drivers 

#define ROUND_UP_ON_DIVIDE(d, n)    (0 == ((d) % (n)) ? ((d)/(n)) : ((d)/(n))+1)

VOID
Strings_CreateDataBufferString(
    _In_reads_bytes_(DataBufferLength) PCHAR    DataBuffer,
    _In_  ULONG    DataBufferLength,
    _In_  ULONG    NumBytesToDisplay,
    _In_  ULONG    DisplayBlockSize,
   _Outptr_result_maybenull_ LPSTR  *BufferString
)
/*++
Routine Description:
    This routine takes a DataBuffer of size DataBufferLength and creates a string
    in BufferString that contains a string representation of the bytes stored in
    data buffer.  

    The parameter NumBytesToDisplay tells the routine the maximum number of bytes
    from the buffer to display.  For instance, a caller may only want to convert
    the first four bytes of an eight byte buffer to a string

    The parameter DisplayBlockSize indicates how many bytes should be grouped 
    together in the display.  Valid values are 1, 2, 4 and would indicate whether
    the displayed bytes should be displayed as bytes, words, or dwords.

    The routine allocates a buffer big enough to store the data.  Callers of this
    routine are responsible for freeing this string buffer.  
--*/
{
    ULONG   BufferStringLength;
    ULONG   MaxDisplayedBytes;
    PUCHAR  NextByte;
    PUCHAR  String = NULL;
    PUCHAR  CurrentBufferOffset;
    PUCHAR  CurrentBufferOffsetBase;
    UINT    nFullIterations;
    INT     LeftOverBytes;
    UINT    IterationIndex;
    INT     ByteOffset;
    BOOLEAN Success = FALSE;

    Success = FALSE;

    /*
    // Determine the maximum number of bytes that will be displayed in 
    //    the string
    */
    
    MaxDisplayedBytes = (NumBytesToDisplay > DataBufferLength) ? DataBufferLength
                                                               : NumBytesToDisplay;
    
    *BufferString = NULL;

    if(NumBytesToDisplay > DataBufferLength)
    {
        goto Done; // error case
    }    

    /*
    // Determine the size of the string we'll need: This is based on the 
    //   maximum number of displayed bytes (MaxDisplayedBytes) and the 
    //   DisplayBlockSize
    */

    BufferStringLength = 2*MaxDisplayedBytes + ROUND_UP_ON_DIVIDE(MaxDisplayedBytes,
                                                                  DisplayBlockSize
                                                                 );

    /*
    // Now we need to allocate string space
    */

    String = (PCHAR) malloc(BufferStringLength * sizeof(CHAR));

    if (NULL != String) {

        /*
        // Determine how many iterations through the conversion routine must be made.
        */
        
        nFullIterations = MaxDisplayedBytes / DisplayBlockSize;

        /*
        // Initialize our variables which point to data in the buffer to convert
        //   and the byte in the string in which to put the converted data value. 
        //   Next byte is set to String-1 because it is incremented on entry into the
        //   loop.
        */
        
        CurrentBufferOffset = DataBuffer;
        CurrentBufferOffsetBase = CurrentBufferOffset;
        
        NextByte = String-1;

        /*
        // Each iteration of the loop creates a block of DisplayBlockSize.  Any
        //   partial iterations are performed afterwards if the number of bytes
        //   to display is not a multiple of the display block size
        */
        IterationIndex = 0;
        do
        {
            NextByte++;

            /*
            // Output a block of data size.  Notice the bytes are accessed in
            //    reverse order to display the the MSB of a block as the first
            //    value in the string
            */

            if(DisplayBlockSize > DataBufferLength)
            {
                goto Done;  // error case
            }
            
            for (ByteOffset = DisplayBlockSize-1; ByteOffset >= 0; ByteOffset--) 
            {
                if(DataBufferLength == 1 && NumBytesToDisplay <=  1 && 
                    ((2*DisplayBlockSize + (CurrentBufferOffset - CurrentBufferOffsetBase) - IterationIndex - 1) / 4 <= 0)) 
                {
                    goto Done; // overrun condition
                }
                if((PLONG)(CurrentBufferOffset + ByteOffset) - (PLONG)DataBuffer >= (LONG)DataBufferLength) 
                {
                    goto Done;
                }
                if(FAILED(StringCbPrintf(NextByte,(String + BufferStringLength - NextByte),
                            "%02X", *(CurrentBufferOffset+ByteOffset))))
                {
                    goto Done; // Error case
                }

                NextByte += 2;
            }

            /*
            // Insert the space to separate blocks
            */
            
            *(NextByte) = ' ';

            CurrentBufferOffset += DisplayBlockSize;

            IterationIndex++;
        }while(IterationIndex < nFullIterations);

        /*
        // Resolve any other bytes that are left over
        */
        
        LeftOverBytes = (MaxDisplayedBytes % DisplayBlockSize);

        if (0 == LeftOverBytes) 
        {
            *(NextByte) = '\0';
        }

        for (ByteOffset = LeftOverBytes-1, NextByte++; ByteOffset >= 0; ByteOffset--) 
        {

            if (ByteOffset < 0)
            {
                goto Done;
            } 

            if(ByteOffset + DataBuffer > CurrentBufferOffset+ByteOffset)
            {
                goto Done;
            }
        
            StringCbPrintf(NextByte,(String + BufferStringLength - NextByte),
                "%02X", *(CurrentBufferOffset+ByteOffset));

            NextByte += 2;
        }
 
        *BufferString = String;
        Success = TRUE;
    }

Done:
    if (FALSE == Success)
    {
        if (NULL != String)
        {
            free(String);
            String = NULL;
        }
    }

    return;
}

_When_(*nUnsigneds == 0, _At_(*UnsignedList, _Post_null_))
_When_(*nUnsigneds > 0, _At_(*UnsignedList, _Post_notnull_))
_Success_(return != FALSE)
BOOL
Strings_StringToUnsignedList(
    _Inout_ LPSTR   InString,
    _In_    ULONG   UnsignedSize,
    _In_    ULONG   Base,
    _Outptr_result_bytebuffer_maybenull_(*nUnsigneds) PCHAR *UnsignedList,
    _Out_   PULONG  nUnsigneds
)
/*++
Routine Description:
    This routine takes an input string, InString, and creates a list of unsigned
    values of all the values that are in the list.  The caller can specify a
    base, Base, for all the numbers in the list or specify 0 to let the function
    determine the base depending on the format of the number in the string.

    The parameter UnsignedSize specifies the size of unsigneds to store in the list.
    
    The routine allocates a CHAR buffer to store the list of unsigned values.  

    On exit, nUnsigneds will report the number of unsigned values stored in 
    UnsignedList.
    
    The function will return TRUE if it could convert all of the numbers in the
    string into the unsigned list.  It will return FALSE if there was a problem
    with the string or if there was a problem allocating memory to store the 
    unsigned list.  
--*/
{
    CHAR    tokDelims[] = "\t,; ";
    PCHAR   strToken;
    PCHAR   strContext = '\0';
    PCHAR   endp;
    BOOL    fStatus = FALSE;
    ULONG   ulValue;
    PCHAR   pList = NULL;
    PCHAR   pNewList;
    ULONG   nAllocUnsigneds;
    ULONG   nActualUnsigneds = 0;
    ULONG   ulMaxValue;

    if(NULL == UnsignedList)
    {
        goto Done;
    }

    /*
    // Begin by initializing our unsigned list
    //      1) Start with initial allocation for 2 unsigneds, this will
    //          be expanded if necessary
    //      2) If initial allocation fails, return FALSE;
    */

    nAllocUnsigneds = 2;
    nActualUnsigneds = 0;

    pList = (PCHAR) malloc(nAllocUnsigneds * sizeof(ULONG));

    if (NULL == pList) 
    {
        goto Done;
    }

    /*
    // Calculate the maximum value that can be represented with the value for
    //   iBufferSize;
    */

    ulMaxValue = (sizeof(ULONG) == UnsignedSize) ? ULONG_MAX 
                                                 : (1 << (UnsignedSize*8)) - 1;

    /*
    // Begin our processing of the token string.
    //  1) Set fStatus to TRUE to get through loop the first time
    //  2) Try to get the first token -- if we can't get the first token
    //        then we pass through loop
    */

    fStatus = TRUE;

    strToken = strtok_s(InString, tokDelims, &strContext);

    /*
    // Loop until there are no more tokens or we detect an error (fStatus == FALSE)
    */

    while (NULL != strToken && fStatus) 
    {
        /*
        // Set fStatus initially to false.  Only if nothing goes wrong in 
        //    the loop will this get set to TRUE
        */

        fStatus = FALSE;

        /*
        // Attempt to convert the token
        */

        ulValue = strtoul(strToken, &endp, Base);

        /*
        // To be a valid value, *endp must point to the NULL character
        */

        if ((0 != ulValue) && '\0' == *endp) 
        {
            /*
            // Check to see that the ulValue found is less than or equal to 
            //     the maximum allowed by UnsignedSize.
            */

            if (ulValue <= ulMaxValue) 
            {    
                /*
                // If we're set to overrun our buffer, attempt to allocate
                //    more space.  If we can't then release the old space
                //    and fail the loop.  
                */

                if (nAllocUnsigneds == nActualUnsigneds) 
                {
                    ULONG newListSize = 0;
                    
                    if(FAILED(ULongMult(nAllocUnsigneds, 2, &nAllocUnsigneds)) ||
                        FAILED(ULongMult(nAllocUnsigneds, UnsignedSize, &newListSize)))
                    {
                        break;
                    }

                    pNewList = (PCHAR) realloc(pList, newListSize);

                    if (NULL == pNewList)
                    {
                        break;
                    }
                    pList = pNewList;
                }

                /*
                // Add the token to the end of the list of unsigneds
                */

                memcpy(pList + (UnsignedSize * nActualUnsigneds),
                       &ulValue,
                       UnsignedSize);

                nActualUnsigneds++;

                /*
                // Prepare to reenter the loop.  Set fStatus = TRUE 
                //    Try to get another token
                */

                fStatus = TRUE;

                strToken = strtok_s(NULL, tokDelims, &strContext);
            }
        }
    }

Done:
    /*
    // If the loop failed for some reason or we found no unsigneds
    //     release the list
    */

    if (!fStatus || 0 == nActualUnsigneds) 
    {
        if (NULL != pList)
        {
            free(pList);
            pList = NULL;
        }
        nActualUnsigneds = 0;
    }

    *UnsignedList = pList;
    *nUnsigneds = nActualUnsigneds;

    return (fStatus);
}

