/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    buffers.c

Abstract:

    This module contains the code for handling the display of HID report
    buffers for the extended calls dialog box.  

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

#include <windows.h>
#include <malloc.h>
#include <setupapi.h>
#include "hidsdi.h"
#include "hidpi.h"
#include "buffers.h"
#include "strings.h"
#include <intsafe.h>
#include <strsafe.h>
#include <assert.h>

#pragma warning(disable:4057) // indirection to slightly different base types
#pragma warning(disable:28146) // Warning is meant for kernel mode drivers 

#define CURRENT_REPORT(pDisp)   (pDisp -> ReportBuffers + pDisp -> iCurrSelectionIndex)

BOOLEAN
BufferDisplay_Init(
    IN  HWND                hCB,
    IN  HWND                hEB,
    IN  INT                 nBuffers,
    IN  INT                 iBufferSize,
    IN  HIDP_REPORT_TYPE    RType,
    _Outptr_result_maybenull_ PBUFFER_DISPLAY *ppBufferDisplay
)
/*++
Routine Description:
    This routine initializes the buffer display mechanism for a given report type

    The display mechanism maintains a list of list of nBuffers, where is each
    is buffer is of iBufferSize and hCB and hEB are handles to the combo box and
    edit box for displaying the buffer. 

    The variable ppBufferDisplay is allocated block which is passed into other
    buffer routines and contains information about the buffer for the other 
    routines.

    This function will return FALSE if there was a problem allocating memory
--*/
{
#pragma warning(push)
#pragma warning(disable:28197) //  BufferDisplay_Destroy frees memory   
    PBUFFER_DISPLAY pNewDisplay;
    UINT            reportBuffersSize = 0;
    CHAR            *pszBufferHeader;
    CHAR            szBufferName[24];
    UINT            iIndex;
    UINT            iCBIndex;

    pNewDisplay = (PBUFFER_DISPLAY) malloc(sizeof(BUFFER_DISPLAY));

    *ppBufferDisplay = NULL;

    if (NULL == pNewDisplay)
    {
        return (FALSE);
    }

    if(S_OK != ULongMult(sizeof(REPORT_BUFFER), nBuffers, &reportBuffersSize))
    {
        free(pNewDisplay);
        return (FALSE);
    }

    pNewDisplay->ReportBuffers = (PREPORT_BUFFER) malloc(reportBuffersSize);

    if (NULL == pNewDisplay -> ReportBuffers) 
    {
        free(pNewDisplay);
        return (FALSE);
    }
    
    memset(pNewDisplay -> ReportBuffers, 0x00, reportBuffersSize);

    pNewDisplay -> hBufferComboBox = hCB;
    pNewDisplay -> hBufferEditBox = hEB;
    pNewDisplay -> nReportBuffers = nBuffers;
    pNewDisplay -> iBufferSize = iBufferSize;
    pNewDisplay -> ReportType = RType;

    switch (pNewDisplay -> ReportType) 
    {
        case HidP_Input:
            pszBufferHeader = "Input";
            break;

        case HidP_Output:
            pszBufferHeader = "Output";
            break;

        case HidP_Feature:
            pszBufferHeader = "Feature";
            break;

        default:
            pszBufferHeader = "Other";
            break;
    }

    for (iIndex = 0; iIndex < pNewDisplay -> nReportBuffers; iIndex++) 
    {
        if(FAILED(StringCbPrintf(szBufferName, sizeof(szBufferName), 
                    "%s Buffer #%d", pszBufferHeader, iIndex)))
        {
            BufferDisplay_Destroy(pNewDisplay);
            return (FALSE);
        }

        iCBIndex = (INT) SendMessage(pNewDisplay -> hBufferComboBox,
                                     CB_ADDSTRING,
                                     0, 
                                     (LPARAM) szBufferName);

        if (CB_ERR == iCBIndex || CB_ERRSPACE == iCBIndex) 
        {
            BufferDisplay_Destroy(pNewDisplay);
            return (FALSE);
        }

        iCBIndex = (INT) SendMessage(pNewDisplay -> hBufferComboBox,
                                     CB_SETITEMDATA,
                                     iCBIndex,
                                     iIndex);

        if (CB_ERR == iCBIndex || CB_ERRSPACE == iCBIndex)  
        {
            BufferDisplay_Destroy(pNewDisplay);
            return (FALSE);
        }
    }

    SendMessage(pNewDisplay -> hBufferComboBox, CB_SETCURSEL, 0, 0);

    BufferDisplay_ChangeSelection(pNewDisplay);

    *ppBufferDisplay = pNewDisplay;
    return (TRUE);
#pragma warning(pop)  // 28197
}

VOID
BufferDisplay_Destroy(
    IN  PBUFFER_DISPLAY     pBufferDisplay
)
/*++
Routine Description:
    This routine cleans up the buffer display variable that was allocated by
    the initialize routine
--*/
{
    UINT     iIndex;

    for (iIndex = 0; iIndex < pBufferDisplay -> nReportBuffers; iIndex++) 
    {
        if (NULL != pBufferDisplay -> ReportBuffers[iIndex].pBuffer) 
        {
            free(pBufferDisplay -> ReportBuffers[iIndex].pBuffer);
        }
    }

    free(pBufferDisplay -> ReportBuffers);
    free(pBufferDisplay);
    return;
}

VOID
BufferDisplay_ChangeSelection(
    IN  PBUFFER_DISPLAY     pBufferDisplay
)
/*++
Routine Description:
    This routine has the selection of a buffer to display via the combo box
--*/
{
    INT     iNewIndex;

    iNewIndex = (INT) SendMessage(pBufferDisplay -> hBufferComboBox,
                                  CB_GETCURSEL, 
                                  0,
                                  0);

    if (CB_ERR == iNewIndex)
    {
        return;
    }

    iNewIndex = (INT) SendMessage(pBufferDisplay -> hBufferComboBox,
                                  CB_GETITEMDATA,
                                  iNewIndex,
                                  0);

    if (CB_ERR == iNewIndex)
    {
        return;
    }

    pBufferDisplay -> iCurrSelectionIndex = iNewIndex;

    BufferDisplay_OutputBuffer(pBufferDisplay -> hBufferEditBox,
                               &(pBufferDisplay -> ReportBuffers[iNewIndex]));

    return;
}

VOID
BufferDisplay_OutputBuffer(
    HWND            hEditBox,
    PREPORT_BUFFER  pReportBuffer
)
/*++
Routine Description:
    This routine outputs to hEditBox a byte representation of pReportBuffer
--*/
{
    PCHAR           BufferString;

    if (0 == pReportBuffer -> iBufferSize || NULL == pReportBuffer -> pBuffer) 
    {
        SetWindowText(hEditBox, "");
    }
    else 
    {
        /*
        // Create a buffer string the size of the buffer and display 
        //   as bytes
        */
        
        Strings_CreateDataBufferString(pReportBuffer -> pBuffer,
                                       pReportBuffer -> iBufferSize,
                                       pReportBuffer -> iBufferSize,
                                       1,
                                       &BufferString);

        if (NULL == BufferString) 
        {
            SetWindowText(hEditBox, "");
        }
        else
        {
            SetWindowText(hEditBox, BufferString);
            free(BufferString);
        }
    }
    return;
}

BOOLEAN
BufferDisplay_UpdateBuffer(
    _In_ PBUFFER_DISPLAY     pBufferDisplay,
    _In_reads_bytes_(ulNewBufferLength) PCHAR pNewBuffer,
    _In_ ULONG              ulNewBufferLength
)
/*++
Routine Description:
    This routine changes the data of the currently active report buffer for the
    given buffer display structure.  

    It returns FALSE if it needed to allocate a new buffer and the memory allocation
    failed.
--*/
{
    PREPORT_BUFFER          pCurrentReport;

    if (pBufferDisplay->nReportBuffers <= pBufferDisplay->iCurrSelectionIndex)
    {
        return (FALSE);  // error case
    }

    pCurrentReport = CURRENT_REPORT(pBufferDisplay);
    
    if (NULL == pCurrentReport->pBuffer) 
    {
        pCurrentReport->pBuffer = malloc(pBufferDisplay->iBufferSize);
        if (NULL == pCurrentReport->pBuffer)
        {
            return (FALSE);
        }

        pCurrentReport->iBufferSize = pBufferDisplay->iBufferSize;
    }

    if(ulNewBufferLength < pCurrentReport->iBufferSize)
    {
        return (FALSE);
    }

    memmove (pCurrentReport->pBuffer, pNewBuffer, pCurrentReport->iBufferSize);

    BufferDisplay_OutputBuffer(pBufferDisplay->hBufferEditBox, pCurrentReport);

    return (TRUE);
}

INT
BufferDisplay_GetBufferSize(
    IN  PBUFFER_DISPLAY      pBufferDisplay
)
/*++
Routine Description:
    This routine simply returns the size of the given buffer
--*/
{
    return (pBufferDisplay -> iBufferSize);
}

VOID
BufferDisplay_CopyCurrentBuffer(
    _In_ PBUFFER_DISPLAY     pBufferDisplay,
    _Out_writes_bytes_(ulCopyBufferLength) PCHAR pCopyBuffer,
    _In_ ULONG              ulCopyBufferLength
)
/*++
Routine Description:
    This routine copies the currently active buffer for the given buffer display
    into the buffer passed in by the caller.

    It is the caller's responsibility to allocate a buffer of the appropriate size
    The appropriate size can be obtain by calling BufferDisplay_GetBufferSize
--*/
{
    PREPORT_BUFFER          pCurrentReport;

    pCurrentReport = CURRENT_REPORT(pBufferDisplay);    

    if (NULL == pCurrentReport->pBuffer)
    {
        if(ulCopyBufferLength >= pBufferDisplay->iBufferSize)
        {
            memset(pCopyBuffer, 0x0, pBufferDisplay->iBufferSize);
        }
    }
    else
    {
        if(ulCopyBufferLength >= pCurrentReport->iBufferSize)
        {
            memcpy(pCopyBuffer, pCurrentReport->pBuffer, pCurrentReport->iBufferSize);
        }
    }
    return;
}

INT
BufferDisplay_GetCurrentBufferNumber(
    IN  PBUFFER_DISPLAY      pBufferDisplay
)
/*++
Routine Description:
    This routine returns the buffer number of the current buffer selection
--*/
{
    return (pBufferDisplay -> iCurrSelectionIndex);
}

UCHAR
BufferDisplay_GetCurrentReportID(
    IN  PBUFFER_DISPLAY      pBufferDisplay
)
/*++
Routine Description:
    This routine returns the report ID of the current buffer selection
--*/
{
    PREPORT_BUFFER pCurrentReport;

    pCurrentReport = CURRENT_REPORT(pBufferDisplay);

    return (pCurrentReport -> ucReportID);
}

VOID
BufferDisplay_ClearBuffer(
    IN  PBUFFER_DISPLAY pBufferDisplay
)
/*++
Routine Description:
    This routine frees the current report buffer and set's it to NULL
--*/
{
    PREPORT_BUFFER pCurrentReport;

    if (pBufferDisplay->nReportBuffers <= pBufferDisplay->iCurrSelectionIndex)
    {
        assert(FALSE);
        return;
    }

    pCurrentReport = CURRENT_REPORT(pBufferDisplay);

    if (NULL != pCurrentReport -> pBuffer) 
    {
        free(pCurrentReport -> pBuffer);

        pCurrentReport -> iBufferSize = 0;
        pCurrentReport -> ucReportID = 0;
        pCurrentReport -> pBuffer = NULL;
    }

    BufferDisplay_OutputBuffer(pBufferDisplay -> hBufferEditBox,
                               pCurrentReport);
    return;
}    
