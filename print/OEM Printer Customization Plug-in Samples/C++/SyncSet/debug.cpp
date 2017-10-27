//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1996 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   Debug.cpp
//
//  PURPOSE:  Debug functions.
//
//

#include "precomp.h"
#include "debug.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

////////////////////////////////////////////////////////
//      INTERNAL DEFINES
////////////////////////////////////////////////////////

#define DEBUG_BUFFER_SIZE       1024
#define PATH_SEPARATOR          '\\'
#define MAX_LOOP                10

// Determine what level of debugging messages to eject.
#ifdef VERBOSE_MSG
    #define DEBUG_LEVEL     DBG_VERBOSE
#elif TERSE_MSG
    #define DEBUG_LEVEL     DBG_TERSE
#elif WARNING_MSG
    #define DEBUG_LEVEL     DBG_WARNING
#elif ERROR_MSG
    #define DEBUG_LEVEL     DBG_ERROR
#elif RIP_MSG
    #define DEBUG_LEVEL     DBG_RIP
#elif NO_DBG_MSG
    #define DEBUG_LEVEL     DBG_NONE
#else
    #define DEBUG_LEVEL     DBG_WARNING
#endif



////////////////////////////////////////////////////////
//      EXTERNAL GLOBALS
////////////////////////////////////////////////////////

INT giDebugLevel = DEBUG_LEVEL;




////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

static BOOL DebugMessageV(LPCSTR lpszMessage, va_list arglist);
static BOOL DebugMessageV(DWORD dwSize, LPCWSTR lpszMessage, va_list arglist);




//////////////////////////////////////////////////////////////////////////
//  Function:   DebugMessageV
//
//  Description:  Outputs variable argument debug string.
//
//
//  Parameters:
//
//      dwSize          Size of temp buffer to hold formated string.
//
//      lpszMessage     Format string.
//
//      arglist         Variable argument list..
//
//
//  Returns:
//
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////

static BOOL DebugMessageV(LPCSTR lpszMessage, va_list arglist)
{
    DWORD   dwSize      = DEBUG_BUFFER_SIZE;
    DWORD   dwLoop      = 0;
    LPSTR   lpszMsgBuf  = NULL;
    HRESULT hr;


    // Parameter checking.
    if( (NULL == lpszMessage)
        ||
        (0 == dwSize)
      )
    {
      return FALSE;
    }

    do
    {
        // Allocate memory for message buffer.
        if(NULL != lpszMsgBuf)
        {
            delete[] lpszMsgBuf;
            dwSize *= 2;
        }
        lpszMsgBuf = new CHAR[dwSize + 1];
        if(NULL == lpszMsgBuf)
        {
            return FALSE;
        }

        hr = StringCbVPrintfA(lpszMsgBuf, (dwSize + 1) * sizeof(CHAR), lpszMessage, arglist);

    // Pass the variable parameters to wvsprintf to be formated.
    } while (FAILED(hr) && (STRSAFE_E_INSUFFICIENT_BUFFER == hr) && (++dwLoop < MAX_LOOP) );

    // Dump string to Debug output.
    OutputDebugStringA(lpszMsgBuf);

    // Cleanup.
    delete[] lpszMsgBuf;

    return SUCCEEDED(hr);
}


//////////////////////////////////////////////////////////////////////////
//  Function:   DebugMessageV
//
//  Description:  Outputs variable argument debug string.
//
//
//  Parameters:
//
//      dwSize          Size of temp buffer to hold formated string.
//
//      lpszMessage     Format string.
//
//      arglist         Variable argument list..
//
//
//  Returns:
//
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////

static BOOL DebugMessageV(DWORD dwSize, LPCWSTR lpszMessage, va_list arglist)
{
    LPWSTR      lpszMsgBuf;
    HRESULT     hResult;


    // Parameter checking.
    if( (NULL == lpszMessage)
        ||
        (0 == dwSize)
      )
    {
      return FALSE;
    }

    // Allocate memory for message buffer.
    lpszMsgBuf = new WCHAR[dwSize + 1];
    if(NULL == lpszMsgBuf)
        return FALSE;

    // Pass the variable parameters to wvsprintf to be formated.
    hResult = StringCbVPrintfW(lpszMsgBuf, (dwSize + 1) * sizeof(WCHAR), lpszMessage, arglist);

    // Dump string to debug output.
    OutputDebugStringW(lpszMsgBuf);

    // Clean up.
    delete[] lpszMsgBuf;

    return SUCCEEDED(hResult);
}


//////////////////////////////////////////////////////////////////////////
//  Function:   DebugMessage
//
//  Description:  Outputs variable argument debug string.
//
//
//  Parameters:
//
//      lpszMessage     Format string.
//
//
//  Returns:
//
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////

BOOL DebugMessage(LPCSTR lpszMessage, ...)
{
    BOOL    bResult;
    va_list VAList;


    // Pass the variable parameters to DebugMessageV for processing.
    va_start(VAList, lpszMessage);
    bResult = DebugMessageV(lpszMessage, VAList);
    va_end(VAList);

    return bResult;
}


//////////////////////////////////////////////////////////////////////////
//  Function:   DebugMessage
//
//  Description:  Outputs variable argument debug string.
//
//
//  Parameters:
//
//      lpszMessage     Format string.
//
//
//  Returns:
//
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////

BOOL DebugMessage(LPCWSTR lpszMessage, ...)
{
    BOOL    bResult;
    va_list VAList;


    // Pass the variable parameters to DebugMessageV to be processed.
    va_start(VAList, lpszMessage);
    bResult = DebugMessageV(MAX_PATH, lpszMessage, VAList);
    va_end(VAList);

    return bResult;
}

void Dump(PPUBLISHERINFO pPublisherInfo)
{
    VERBOSE(TEXT("pPublisherInfo:\r\n"));
    if(NULL == pPublisherInfo)
    {
        VERBOSE(TEXT("\tpPublisherInfo is NULL!\r\n"));
        return;
    }
    VERBOSE(TEXT("\tdwMode           =   %#x\r\n"), pPublisherInfo->dwMode);
    VERBOSE(TEXT("\twMinoutlinePPEM  =   %d\r\n"), pPublisherInfo->wMinoutlinePPEM);
    VERBOSE(TEXT("\twMaxbitmapPPEM   =   %d\r\n"), pPublisherInfo->wMaxbitmapPPEM);
}

void Dump(POEMDMPARAM pOemDMParam)
{
    VERBOSE(TEXT("pOemDMParam:\r\n"));
    if(NULL == pOemDMParam)
    {
        VERBOSE(TEXT("\tpOemDMParam is NULL!\r\n"));
        return;
    }
    VERBOSE(TEXT("\tcbSize = %d\r\n"), pOemDMParam->cbSize);
    VERBOSE(TEXT("\tpdriverobj = %#x\r\n"), pOemDMParam->pdriverobj);
    VERBOSE(TEXT("\thPrinter = %#x\r\n"), pOemDMParam->hPrinter);
    VERBOSE(TEXT("\thModule = %#x\r\n"), pOemDMParam->hModule);
    VERBOSE(TEXT("\tpPublicDMIn = %#x\r\n"), pOemDMParam->pPublicDMIn);
    VERBOSE(TEXT("\tpPublicDMOut = %#x\r\n"), pOemDMParam->pPublicDMOut);
    VERBOSE(TEXT("\tpOEMDMIn = %#x\r\n"), pOemDMParam->pOEMDMIn);
    VERBOSE(TEXT("\tpOEMDMOut = %#x\r\n"), pOemDMParam->pOEMDMOut);
    VERBOSE(TEXT("\tcbBufSize = %d\r\n"), pOemDMParam->cbBufSize);
}

void Dump(PPROPSHEETUI_INFO pPSUIInfo)
{
    VERBOSE(TEXT("pPSUIInfo:\r\n"));
    if(NULL == pPSUIInfo)
    {
        VERBOSE(TEXT("\tpPSUIInfo is NULL!\r\n"));
        return;
    }
    VERBOSE(TEXT("\tcbSize          = %d\r\n"), pPSUIInfo->cbSize);
    VERBOSE(TEXT("\tVersion         = %#x\r\n"), pPSUIInfo->Version);
    VERBOSE(TEXT("\tFlags           = %#x\r\n"), pPSUIInfo->Flags);
    VERBOSE(TEXT("\tReason          = %d\r\n"), pPSUIInfo->Reason);
    VERBOSE(TEXT("\thComPropSheet   = %#x\r\n"), pPSUIInfo->hComPropSheet);
    VERBOSE(TEXT("\tpfnComPropSheet = %#x\r\n"), pPSUIInfo->pfnComPropSheet);
    VERBOSE(TEXT("\tlParamInit      = %#x\r\n"), pPSUIInfo->lParamInit);
    VERBOSE(TEXT("\tUserData        = %#x\r\n"), pPSUIInfo->UserData);
    VERBOSE(TEXT("\tResult          = %#x\r\n"), pPSUIInfo->Result);
}

void Dump(POPTITEM pOptItem)
{
    VERBOSE(TEXT("pOptItem:\r\n"));
    if(NULL == pOptItem)
    {
        VERBOSE(TEXT("\tpOptItem is NULL!\r\n"));
        return;
    }
    VERBOSE(TEXT("\tcbSize          = %d\r\n"),     pOptItem->cbSize);
    VERBOSE(TEXT("\tLevel           = %d\r\n"),     pOptItem->Level);
    VERBOSE(TEXT("\tDlgPageIdx      = %d\r\n"),     pOptItem->DlgPageIdx);
    VERBOSE(TEXT("\tFlags           = 0x%x\r\n"),   pOptItem->Flags);
    VERBOSE(TEXT("\tUserData        = 0x%p\r\n"),   pOptItem->UserData);
    VERBOSE(TEXT("\tpName           = %s\r\n"),     pOptItem->pName ? pOptItem->pName : TEXT("<NULL>"));
    VERBOSE(TEXT("\tpSel            = 0x%p\r\n"),   pOptItem->pSel);
    VERBOSE(TEXT("\tpExtChkBox      = 0x%p\r\n"),   pOptItem->pExtChkBox);
    VERBOSE(TEXT("\tpExtChkBox      = 0x%p\r\n"),   pOptItem->pExtChkBox);
    VERBOSE(TEXT("\tHelpIndex       = 0x%x\r\n"),   pOptItem->HelpIndex);
    VERBOSE(TEXT("\tDMPubID         = 0x%x\r\n"),   pOptItem->DMPubID);
    VERBOSE(TEXT("\tUserItemID      = 0x%x\r\n"),   pOptItem->UserItemID);
    VERBOSE(TEXT("\twReserved       = 0x%x\r\n"),   pOptItem->wReserved);
    VERBOSE(TEXT("\tpOIExt          = 0x%p\r\n"),   pOptItem->pOIExt);

    Dump(pOptItem->pOptType);
}

void Dump(POPTTYPE pOptType)
{
    VERBOSE(TEXT("\tpOptType:\r\n"));
    if(NULL == pOptType)
    {
        VERBOSE(TEXT("\t\tpOptType is NULL!\r\n"));
        return;
    }
    VERBOSE(TEXT("\t\tcbSize    = %d\r\n"),     pOptType->cbSize);
    VERBOSE(TEXT("\t\tType      = 0x%x\r\n"),   pOptType->Type);
    VERBOSE(TEXT("\t\tFlags     = 0x%x\r\n"),   pOptType->Flags);
    VERBOSE(TEXT("\t\tCount     = %d\r\n"),     pOptType->Count);
    VERBOSE(TEXT("\t\tCount     = 0x%x\r\n"),   pOptType->BegCtrlID);
    VERBOSE(TEXT("\t\tStyle     = 0x%x\r\n"),   pOptType->Style);

    Dump(pOptType->pOptParam, pOptType->Count);
}

void Dump(POPTPARAM pOptParam, WORD wCount)
{
    if(NULL == pOptParam)
    {
        VERBOSE(TEXT("\t\tpOptParam is NULL!\r\n"));
        return;
    }

    for(WORD wIndex = 0; wIndex < wCount; ++wIndex)
    {
        VERBOSE(TEXT("\t\tpOptParam[wIndex]:\r\n"));
        VERBOSE(TEXT("\t\t\tcbSize    = %d\r\n"),     pOptParam[wIndex].cbSize);
        VERBOSE(TEXT("\t\t\tFlags     = 0x%x\r\n"),   pOptParam[wIndex].Flags);
        VERBOSE(TEXT("\t\t\tStyle     = 0x%x\r\n"),   pOptParam[wIndex].Style);
        VERBOSE(TEXT("\t\t\tpData     = 0x%p\r\n"),   pOptParam[wIndex].pData);
        VERBOSE(TEXT("\t\t\tIconID    = 0x%p\r\n"),   pOptParam[wIndex].IconID);
        VERBOSE(TEXT("\t\t\tlParam    = 0x%p\r\n"),   pOptParam[wIndex].lParam);
    }
}


PCSTR
StripDirPrefixA(
    IN PCSTR    pstrFilename
    )

/*++

Routine Description:

    Strip the directory prefix off a filename (ANSI version)

Arguments:

    pstrFilename - Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    PCSTR   pstr;

    pstr = strrchr(pstrFilename, PATH_SEPARATOR);
    if (pstr)
        return pstr + 1;

    return pstrFilename;
}


