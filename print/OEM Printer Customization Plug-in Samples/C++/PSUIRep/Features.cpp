//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  2001 - 2003  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    Features.cpp
//
//  PURPOSE:  Implementation wrapper class for PScript Driver Features and Options.
//

#include "precomp.h"
#include "resource.h"
#include "debug.h"
#include "globals.h"
#include "devmode.h"
#include "stringutils.h"
#include "helper.h"
#include "features.h"
#include "oemui.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);


////////////////////////////////////////////////////////
//      Internal Defines and Macros
////////////////////////////////////////////////////////

#define INITIAL_ENUM_FEATURES_SIZE          1024
#define INITIAL_ENUM_OPTIONS_SIZE           64
#define INITIAL_FEATURE_DISPLAY_NAME_SIZE   64
#define INITIAL_OPTION_DISPLAY_NAME_SIZE    32
#define INITIAL_GET_OPTION_SIZE             64
#define INITIAL_GET_REASON_SIZE             1024

#define DRIVER_FEATURE_PREFIX               '%'
#define IS_DRIVER_FEATURE(f)                (DRIVER_FEATURE_PREFIX == (f)[0])

// Flags that the uDisplayNameID should be returned as
// MAKEINTRESOURCE() instead of loading the string resource.
#define RETURN_INT_RESOURCE     1

// Macros to test for conditions of KEYWORDMAP entry.
#define IS_MAPPING_INT_RESOURCE(p)  ((p)->dwFlags & RETURN_INT_RESOURCE)

// TAG the identifies feature OPTITEM data stuct.
#define FEATURE_OPTITEM_TAG     'FETR'


////////////////////////////////////////////////////////
//      Type Definitions
////////////////////////////////////////////////////////

// Struct used to identify OPTITEM as
// feature OPTITEM and to map back from
// an OPTITEM to the feature.
typedef struct _tagFeatureOptitemData
{
    DWORD       dwSize;
    DWORD       dwTag;
    PCSTR       pszFeatureKeyword;
    COptions    *pOptions;

} FEATUREOPTITEMDATA, *PFEATUREOPTITEMDATA;



////////////////////////////////////////////////////////
//      Internal Constants
////////////////////////////////////////////////////////

static KEYWORDMAP gkmFeatureMap[] =
{
    "%AddEuro",                 NULL,                   IDS_ADD_EURO,           OEMCUIP_PRNPROP,    0,
    "%CtrlDAfter",              NULL,                   IDS_CTRLD_AFTER,        OEMCUIP_PRNPROP,    0,
    "%CtrlDBefore",             NULL,                   IDS_CTRLD_BEFORE,       OEMCUIP_PRNPROP,    0,
    //"%CustomPageSize",          NULL,                   IDS_PSCRIPT_CUSTOMSIZE, OEMCUIP_DOCPROP,    0,
    "%GraphicsTrueGray",        NULL,                   IDS_TRUE_GRAY_GRAPH,    OEMCUIP_PRNPROP,    0,
    "%JobTimeout",              NULL,                   IDS_JOBTIMEOUT,         OEMCUIP_PRNPROP,    0,
    "%MaxFontSizeAsBitmap",     NULL,                   IDS_PSMAXBITMAP,        OEMCUIP_PRNPROP,    0,
    "%MetafileSpooling",        NULL,                   IDS_METAFILE_SPOOLING,  OEMCUIP_DOCPROP,    0,
    "%MinFontSizeAsOutline",    NULL,                   IDS_PSMINOUTLINE,       OEMCUIP_PRNPROP,    0,
    "%Mirroring",               NULL,                   IDS_MIRROR,             OEMCUIP_DOCPROP,    0,
    "%Negative",                NULL,                   IDS_NEGATIVE_PRINT,     OEMCUIP_DOCPROP,    0,
    "%Orientation",             TEXT("COMPSTUI.DLL"),   IDS_CPSUI_ORIENTATION,  OEMCUIP_DOCPROP,    RETURN_INT_RESOURCE,
    "%OutputFormat",            NULL,                   IDS_PSOUTPUT_OPTION,    OEMCUIP_DOCPROP,    0,
    "%OutputProtocol",          NULL,                   IDS_PSPROTOCOL,         OEMCUIP_PRNPROP,    0,
    "%OutputPSLevel",           NULL,                   IDS_PSLEVEL,            OEMCUIP_DOCPROP,    0,
    "%PageOrder",               TEXT("COMPSTUI.DLL"),   IDS_CPSUI_PAGEORDER,    OEMCUIP_DOCPROP,    RETURN_INT_RESOURCE,
    "%PagePerSheet",            TEXT("COMPSTUI.DLL"),   IDS_CPSUI_NUP,          OEMCUIP_DOCPROP,    RETURN_INT_RESOURCE,
    "%PSErrorHandler",          NULL,                   IDS_PSERROR_HANDLER,    OEMCUIP_DOCPROP,    0,
    "%PSMemory",                NULL,                   IDS_POSTSCRIPT_VM,      OEMCUIP_PRNPROP,    0,
    "%TextTrueGray",            NULL,                   IDS_TRUE_GRAY_TEXT,     OEMCUIP_PRNPROP,    0,
    "%TTDownloadFormat",        NULL,                   IDS_PSTT_DLFORMAT,      OEMCUIP_DOCPROP,    0,
    "%WaitTimeout",             NULL,                   IDS_WAITTIMEOUT,        OEMCUIP_PRNPROP,    0,
};
static const NUM_FEATURE_MAP    = (sizeof(gkmFeatureMap)/sizeof(gkmFeatureMap[0]));


static KEYWORDMAP gkmOptionMap[] =
{
    "True",             TEXT("COMPSTUI.DLL"),       IDS_CPSUI_TRUE,             0,  RETURN_INT_RESOURCE,
    "False",            TEXT("COMPSTUI.DLL"),       IDS_CPSUI_FALSE,            0,  RETURN_INT_RESOURCE,
    "Portrait",         TEXT("COMPSTUI.DLL"),       IDS_CPSUI_PORTRAIT,         0,  RETURN_INT_RESOURCE,
    "Landscape",        TEXT("COMPSTUI.DLL"),       IDS_CPSUI_LANDSCAPE,        0,  RETURN_INT_RESOURCE,
    "RotatedLandscape", TEXT("COMPSTUI.DLL"),       IDS_CPSUI_ROT_LAND,         0,  RETURN_INT_RESOURCE,
    "Speed",            NULL,                       IDS_PSOPT_SPEED,            0,  RETURN_INT_RESOURCE,
    "Portability",      NULL,                       IDS_PSOPT_PORTABILITY,      0,  0,
    "EPS",              NULL,                       IDS_PSOPT_EPS,              0,  0,
    "Archive",          NULL,                       IDS_PSOPT_ARCHIVE,          0,  0,
    "ASCII",            NULL,                       IDS_PSPROTOCOL_ASCII,       0,  0,
    "BCP",              NULL,                       IDS_PSPROTOCOL_BCP,         0,  0,
    "TBCP",             NULL,                       IDS_PSPROTOCOL_TBCP,        0,  0,
    "Binary",           NULL,                       IDS_PSPROTOCOL_BINARY,      0,  0,
    "FrontToBack",      TEXT("COMPSTUI.DLL"),       IDS_CPSUI_FRONTTOBACK,      0,  RETURN_INT_RESOURCE,
    "BackToFront",      TEXT("COMPSTUI.DLL"),       IDS_CPSUI_BACKTOFRONT,      0,  RETURN_INT_RESOURCE,
    "1",                TEXT("COMPSTUI.DLL"),       IDS_CPSUI_NUP_NORMAL,       0,  RETURN_INT_RESOURCE,
    "2",                TEXT("COMPSTUI.DLL"),       IDS_CPSUI_NUP_TWOUP,        0,  RETURN_INT_RESOURCE,
    "4",                TEXT("COMPSTUI.DLL"),       IDS_CPSUI_NUP_FOURUP,       0,  RETURN_INT_RESOURCE,
    "6",                TEXT("COMPSTUI.DLL"),       IDS_CPSUI_NUP_SIXUP,        0,  RETURN_INT_RESOURCE,
    "9",                TEXT("COMPSTUI.DLL"),       IDS_CPSUI_NUP_NINEUP,       0,  RETURN_INT_RESOURCE,
    "16",               TEXT("COMPSTUI.DLL"),       IDS_CPSUI_NUP_SIXTEENUP,    0,  RETURN_INT_RESOURCE,
    "Booklet",          TEXT("COMPSTUI.DLL"),       IDS_CPSUI_BOOKLET,          0,  RETURN_INT_RESOURCE,
    "Automatic",        NULL,                       IDS_TTDL_DEFAULT,           0,  0,
    "Outline",          NULL,                       IDS_TTDL_TYPE1,             0,  0,
    "Bitmap",           NULL,                       IDS_TTDL_TYPE3,             0,  0,
    "NativeTrueType",   NULL,                       IDS_TTDL_TYPE42,            0,  0,
};
static const NUM_OPTION_MAP     = (sizeof(gkmOptionMap)/sizeof(gkmOptionMap[0]));




////////////////////////////////////////////
//
//  COptions Methods
//

//
//  Private Methods
//

// Initializes class data members.
void COptions::Init()
{
    m_wOptions      = 0;
    m_cType         = TVOT_COMBOBOX;
    m_pmszRaw       = NULL;
    m_pszFeature    = NULL;
    m_ppszOptions   = NULL;
    m_ptRange.x     = 0;
    m_ptRange.y     = 0;
    m_dwSize        = 0;
    m_pszUnits      = NULL;
    m_hHeap         = NULL;
    m_pInfo         = NULL;
}

void COptions::Clear()
{
    // Free memory associated with data members.
    if(NULL != m_pmszRaw)       HeapFree(m_hHeap, 0, m_pmszRaw);
    if(NULL != m_ppszOptions)   HeapFree(m_hHeap, 0, m_ppszOptions);
    if( (NULL != m_pszUnits) && !IS_INTRESOURCE(m_pszUnits))    HeapFree(m_hHeap, 0, m_pszUnits);

    // Free option info.
    FreeOptionInfo();

    // Initialize data members.
    Init();
}

// Frees memory associated with Option Info array.
void COptions::FreeOptionInfo()
{
    // Validate parameters.
    if( (NULL == m_hHeap)
        ||
        (NULL == m_pInfo)
      )
    {
        return;
    }

    // Free strings in the array.
    for(WORD wIndex = 0; wIndex < m_wOptions; ++wIndex)
    {
        PWSTR   pszDisplay = m_pInfo[wIndex].pszDisplayName;

        if( (NULL != pszDisplay)
            &&
            !(IS_INTRESOURCE(pszDisplay))
          )
        {
            HeapFree(m_hHeap, 0, pszDisplay);
        }
    }

    // Free the array.
    HeapFree(m_hHeap, 0, m_pInfo);
    m_pInfo = NULL;
}

// Will do init for features that need special handling.
HRESULT COptions::GetOptionsForSpecialFeatures(CUIHelper &Helper, POEMUIOBJ poemuiobj)
{
    HRESULT hrResult    = E_NOTIMPL;


    // See if this is a special feature.
    // If it is, then call the special option init for the
    // feature.
    if(!lstrcmpA(m_pszFeature, "%CustomPageSize"))
    {
        // There is no option init for CustomPageSize.
        // The item itself must be handled specially.
        hrResult = S_OK;
    }
    else if( !lstrcmpA(m_pszFeature, "%JobTimeout")
             ||
             !lstrcmpA(m_pszFeature, "%WaitTimeout")
           )
    {
        // JobTimeout and WaitTimeout feature options are string representations
        // of an integer that represents number of seconds in the range 0 through
        // 2,147,483,647 (i.e. LONG_MAX).  However, COMPSTUI limits us to
        // WORD size, which has range of 0 to 32,767 (i.e. SHRT_MAX).
        m_cType     = TVOT_UDARROW;
        m_ptRange.x = 0;
        m_ptRange.y = SHRT_MAX;
        hrResult    = GetOptionSelectionShort(Helper, poemuiobj);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("COptions::GetOptionsForSpecialFeatures() failed to get current selection for feature %hs. (hrResult = 0x%x)\r\n"),
                m_pszFeature,
                hrResult);

            goto Exit;
        }
    }
    else if( !lstrcmpA(m_pszFeature, "%MaxFontSizeAsBitmap")
             ||
             !lstrcmpA(m_pszFeature, "%MinFontSizeAsOutline")
           )
    {
        // MaxFontSizeAsBitmap, and MinFontSizeAsOutline feature options
        // are string representations of an integer that represents number
        // of pixels in the range 0 through 32,767 (i.e. SHRT_MAX).
        m_cType     = TVOT_UDARROW;
        m_ptRange.x = 0;
        m_ptRange.y = SHRT_MAX;
        hrResult    = GetOptionSelectionShort(Helper, poemuiobj);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("COptions::GetOptionsForSpecialFeatures() failed to get current selection for feature %hs. (hrResult = 0x%x)\r\n"),
                m_pszFeature,
                hrResult);

            goto Exit;
        }
    }
    else if( !lstrcmpA(m_pszFeature, "%PSMemory") )
    {
        DWORD   dwType;
        DWORD   dwLevel;
        DWORD   dwNeeded;


        // PSMemory option is string representation of an integer
        // that represents number of seconds in the range 0
        // through 32,767 (i.e. SHRT_MAX).
        // However, the minimum is 172 KB for Level 1 and 249 KB for level 2
        m_cType     = TVOT_UDARROW;
        m_ptRange.y = SHRT_MAX;
        hrResult    = GetOptionSelectionShort(Helper, poemuiobj);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("COptions::GetOptionsForSpecialFeatures() failed to get current selection for feature %hs. (hrResult = 0x%x)\r\n"),
                m_pszFeature,
                hrResult);

            goto Exit;
        }

        // Get the global attribute for max language level.
        hrResult = Helper.GetGlobalAttribute(poemuiobj,
                                             0,
                                             "LanguageLevel",
                                             &dwType,
                                             (PBYTE) &dwLevel,
                                             sizeof(dwLevel),
                                             &dwNeeded);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("COptions::GetOptionsForSpecialFeatures() failed to get global attribute \"LanguageLevel\". (hrResult = 0x%x)\r\n"),
                hrResult);

            goto Exit;
        }

        // Set minimum range based on PS Max Language level.
        switch(dwLevel)
        {
            case 1:
                m_ptRange.x     = 172;
                break;

            default:
            case 2:
            case 3:
                m_ptRange.x     = 249;
                break;
        }

        // Get Units string.
        //GetStringResource(m_pszUnits
    }
    else if(!lstrcmpA(m_pszFeature, "%OutputPSLevel"))
    {
        hrResult = GetOptionsForOutputPSLevel(Helper, poemuiobj);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("COptions::GetOptionsForSpecialFeatures() failed to get current selection for feature %hs. (hrResult = 0x%x)\r\n"),
                m_pszFeature,
                hrResult);

            goto Exit;
        }
    }


Exit:

    return hrResult;
}

// Do init for PS Level options.
HRESULT COptions::GetOptionsForOutputPSLevel(CUIHelper &Helper, POEMUIOBJ poemuiobj)
{
    WORD    wCount      = 0;
    DWORD   dwLevel     = 0;
    DWORD   dwType      = 0;
    DWORD   dwNeeded    = 0;
    HRESULT hrResult    = E_NOTIMPL;


    // PS Level is integers from 1 to "LanguageLevel"
    // global atribute.
    m_cType = TVOT_COMBOBOX;

    // Get the global attribute for max language level.
    hrResult = Helper.GetGlobalAttribute(poemuiobj,
                                         0,
                                         "LanguageLevel",
                                         &dwType,
                                         (PBYTE) &dwLevel,
                                         sizeof(dwLevel),
                                         &dwNeeded);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("COptions::GetOptionsForOutputPSLevel() failed to get global attribute \"LanguageLevel\". (hrResult = 0x%x)\r\n"),
            hrResult);

        goto Exit;
    }

    //
    // Create Options for PS Level
    //

    // Set the number of options to the PS Level supported.
    m_wOptions = (WORD) dwLevel;

    // Allocate keyword list.
    // Allocate memory for pointer to keyword and the keyword itself, so that
    // the memory for the keyword strings will get de-allocated with the keyword list
    // on object destruction, just like regular features for which EnumOptions works.
    // User the size of a pointer (4 bytes on x86, and 8 on IA64) so that
    // it begining of the keyword strings will be DWORD or QUADWORD aligned
    // for x86 and IA64 respectively.  Keyword strings aren't required to
    // be DWORD or QUADWORD aligned, but it is more optimal.  Also, this gives
    // some additional space for the case of %OutputPSLevel keywords, which are
    // in the range of 1 through the max PostScript level supported, and only
    // require 2 CHARs (1 for the digit and one for the NULL terminator).
    m_ppszOptions = (PCSTR *) HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_wOptions * ( sizeof(PSTR) + sizeof(PCSTR *) ) );
    if(NULL == m_ppszOptions)
    {
        ERR(ERRORTEXT("COptions::GetOptionsForOutputPSLevel() failed to allocate option keyword array for PS Level.\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }


    // Allocate option info array.
    m_pInfo = (POPTION_INFO) HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_wOptions * sizeof(OPTION_INFO));
    if(NULL == m_pInfo)
    {
        ERR(ERRORTEXT("COptions::GetOptionsForOutputPSLevel() failed to allocate info array for PS Level.\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    // Init the option info.
    for(wCount = 0; wCount < m_wOptions; ++wCount)
    {
        // Init keyword.
        // The memory for both the keyword list and the keyword strings was allocated above.
        m_ppszOptions[wCount] = (PSTR)(m_ppszOptions + m_wOptions) + (sizeof(PSTR) * wCount);
        hrResult = StringCbPrintfA(const_cast<PSTR>(m_ppszOptions[wCount]), sizeof(PSTR), "%d", wCount + 1);
        if(FAILED(hrResult))
        {
            ERR(ERRORTEXT("COptions::GetOptionsForOutputPSLevel() failed to string representation of item %d.\r\n"),
                          wCount + 1);

            goto Exit;
        }

        // Init option display name.
        m_pInfo[wCount].pszDisplayName = (PWSTR) HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, 2 * sizeof(WCHAR));
        if(NULL == m_pInfo[wCount].pszDisplayName)
        {
            ERR(ERRORTEXT("COptions::GetOptionsForOutputPSLevel() failed to allocate display string for Level %d.\r\n"),
                          wCount);

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }

        // Init option display name.
        hrResult = StringCchPrintfW(m_pInfo[wCount].pszDisplayName, 2, TEXT("%d"), wCount + 1);
        if(FAILED(hrResult))
        {
            ERR(ERRORTEXT("COptions::GetOptionsForOutputPSLevel() failed to create display name for item %d.\r\n"),
                          wCount + 1);

            goto Exit;
        }
    }

    //
    // Get Current Selection
    //

    hrResult = GetOptionSelectionIndex(Helper, poemuiobj);


Exit:

    // Don't need to clean up memory allocation on error, since
    // all memory allocated are assigned to data members, which
    // will be cleaned up in the object destructor.

    return hrResult;
}

HRESULT COptions::GetOptionSelectionString(CUIHelper &Helper, POEMUIOBJ poemuiobj, _Outptr_result_maybenull_ PSTR *ppszSel)
{
    PSTR    pmszFeature     = NULL;
    PSTR    pmszBuf         = NULL;
    WORD    wCount          = 0;
    PCSTR  *ppszList        = NULL;
    DWORD   dwFeatureSize   = 0;
    DWORD   dwNeeded        = 0;
    DWORD   dwSize          = INITIAL_GET_OPTION_SIZE;
    HRESULT hrResult        = S_OK;

    *ppszSel = NULL;


    //
    // Make single feature multi-sz.
    //

    // Allocate single feature multi-sz buffer that's the size of the Feature name, plus two bytes for multi-sz termination
    if (FAILED(hrResult = SizeTToDWord(strlen(m_pszFeature) + 2, &dwFeatureSize)))
    {
        goto Exit;
    }

    pmszFeature = (PSTR) HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, dwFeatureSize);
    if(NULL == pmszFeature)
    {
        ERR(ERRORTEXT("COptions::GetOptionSelectionString() failed to allocate buffer for single feature multi-sz for feature %hs.\r\n"),
                      m_pszFeature);

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    // Just need to do a regular string copy, since the buffer is already zero filled.
    hrResult = StringCbCopyA(pmszFeature, dwFeatureSize, m_pszFeature);
    if(FAILED(hrResult))
    {
        ERR(ERRORTEXT("COptions::GetOptionSelectionString() failed to copy feature string %hs.\r\n"), m_pszFeature);
    }

    // Allocated initial buffer of reasonible size.
    pmszBuf = (PSTR) HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, dwSize);
    if(NULL == pmszBuf)
    {
        ERR(ERRORTEXT("COptions::GetOptionSelectionString() failed to allocate buffer to get current setting for feature %hs.\r\n"),
                      m_pszFeature);

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    // Get current option selection.
    hrResult = Helper.GetOptions(poemuiobj,
                                 0,
                                 pmszFeature,
                                 dwFeatureSize,
                                 pmszBuf,
                                 dwSize,
                                 &dwNeeded);
    if( (E_OUTOFMEMORY == hrResult) && (dwSize < dwNeeded) )
    {
        PSTR    pTemp   = NULL;


        // INVARIANT:  initial buffer not large enough.

        // Re-alloc buffer and try again.
        pTemp = (PSTR) HeapReAlloc(m_hHeap, HEAP_ZERO_MEMORY, pmszBuf, dwNeeded);
        if(NULL == pTemp)
        {
            ERR(ERRORTEXT("COptions::GetOptionSelectionString() failed to re-allocate buffer to get current setting for feature %hs.\r\n"),
                          m_pszFeature);

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }
        pmszBuf = pTemp;


        // Try to get current option selection again.
        hrResult = Helper.GetOptions(poemuiobj,
                                     0,
                                     pmszFeature,
                                     dwFeatureSize,
                                     pmszBuf,
                                     dwNeeded,
                                     &dwNeeded);
    }

    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("COptions::GetOptionSelectionString() failed to get current setting for feature %hs. (hrResult = 0x%x)\r\n"),
            m_pszFeature,
            hrResult);

        goto Exit;
    }

    // NOTE: The return string from GetOptions() may return
    //       contain no strings and not return a HRESULT error
    //       when the feature isn't supported in the current document or
    //       printer sticky mode.
    if('\0' == pmszBuf[0])
    {
        // Feature not supported for this sticky mode.
        goto Exit;
    }

    // Parse the results buffer to see what the current setting is.
    hrResult = MakeStrPtrList(m_hHeap, pmszBuf, &ppszList, &wCount);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("COptions::GetOptionSelectionString() failed to make string list for GetOptions() return for feature %hs. (hrResult = 0x%x)\r\n"),
            m_pszFeature,
            hrResult);

        goto Exit;
    }

    // Check that we got 2 strings back.
    if(2 != wCount)
    {
        WARNING(DLLTEXT("COptions::GetOptionSelectionString() the GetOption() return string list for \r\n\tfeature %hs is not of size 2.\r\n\tNumber of string is %d\r\n"),
                        m_pszFeature,
                        wCount);

        // Bail if we don't have at least 2 strings.
        if(2 > wCount)
        {
            goto Exit;
        }
    }

    // Return copy of just the GetOption() result.
    *ppszSel = MakeStringCopy(m_hHeap, ppszList[1]);
    if(NULL == *ppszSel)
    {
        ERR(ERRORTEXT("COptions::GetOptionSelectionString() failed to duplicate string GetOptions() return for feature %hs. (hrResult = 0x%x)\r\n"),
            m_pszFeature,
            hrResult);

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }


Exit:

    // Free local buffers.
    if(NULL != pmszFeature) HeapFree(m_hHeap, 0, pmszFeature);
    if(NULL != pmszBuf)     HeapFree(m_hHeap, 0, pmszBuf);
    if(NULL != ppszList)    HeapFree(m_hHeap, 0, ppszList);

    return hrResult;
}

// Gets current Options selection for LONG value.
HRESULT COptions::GetOptionSelectionLong(CUIHelper &Helper, POEMUIOBJ poemuiobj)
{
    PSTR    pszSel      = NULL;
    HRESULT hrResult    = S_OK;


    // Get option selection string.
    hrResult = GetOptionSelectionString(Helper, poemuiobj, &pszSel);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("COptions::GetOptionSelectionLong() failed to get string for GetOptions() return for feature %hs. (hrResult = 0x%x)\r\n"),
            m_pszFeature,
            hrResult);

        goto Exit;
    }

    // Convert string option to LONG and uses that as the selection.
    if(NULL != pszSel) m_Sel = atol(pszSel);


Exit:

    // Free local buffers.
    if(NULL != pszSel)  HeapFree(m_hHeap, 0, pszSel);

    return hrResult;
}

// Gets current Options selection for SHORT value.
HRESULT COptions::GetOptionSelectionShort(CUIHelper &Helper, POEMUIOBJ poemuiobj)
{
    PSTR    pszSel      = NULL;
    HRESULT hrResult    = S_OK;


    // Get option selection string.
    hrResult = GetOptionSelectionString(Helper, poemuiobj, &pszSel);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("COptions::GetOptionSelectionLong() failed to get string for GetOptions() return for feature %hs. (hrResult = 0x%x)\r\n"),
            m_pszFeature,
            hrResult);

        goto Exit;
    }

    // Convert string option to LONG and uses that as the selection.
    if(NULL != pszSel) m_Sel = atoi(pszSel) & 0x00ffff;


Exit:

    // Free local buffers.
    if(NULL != pszSel)  HeapFree(m_hHeap, 0, pszSel);

    return hrResult;
}

// Gets current option selection for feature.
HRESULT COptions::GetOptionSelectionIndex(CUIHelper &Helper, POEMUIOBJ poemuiobj)
{
    PSTR    pszSel      = NULL;
    HRESULT hrResult    = S_OK;


    // Get option selection string.
    hrResult = GetOptionSelectionString(Helper, poemuiobj, &pszSel);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("COptions::GetOptionSelectionIndex() failed to get string for GetOptions() return for feature %hs. (hrResult = 0x%x)\r\n"),
            m_pszFeature,
            hrResult);

        goto Exit;
    }

    // Find the matching option for the string returned from GetOption.
    m_Sel = FindOption(pszSel, m_wOptions - 1);


Exit:

    // Free local buffers.
    if(NULL != pszSel)  HeapFree(m_hHeap, 0, pszSel);

    return hrResult;
}



//
//  Public Methods
//

// Default constructor
COptions::COptions()
{
    Init();
}

// Destructor
COptions::~COptions()
{
    Clear();
}

// Get the option list for a feature
HRESULT COptions::Acquire(_In_ HANDLE hHeap,
                          CUIHelper &Helper,
                          _In_ POEMUIOBJ poemuiobj,
                          _In_ PCSTR pszFeature)
{
    DWORD   dwNeeded    = 0;
    HRESULT hrResult    = S_OK;


    VERBOSE(DLLTEXT("COptions::Acquire(0x%p, Helper, 0x%p, %hs) entered.\r\n"),
            hHeap,
            poemuiobj,
            pszFeature ? pszFeature : "NULL");

    // Don't retreive the Options again if we already got them.
    if( (0 < m_wOptions)
        &&
        (NULL != m_pszFeature)
        &&
        !lstrcmpA(m_pszFeature, pszFeature)
      )
    {
        VERBOSE(DLLTEXT("COptions::Acquire() already have options for feature %hs.\r\n"), m_pszFeature);
        VERBOSE(DLLTEXT("COptions::Acquire() returning with HRESULT of S_OK\r\n"));

        return S_OK;
    }

    // Save the heap handle for use later, such as freeing memory at destruction.
    m_hHeap = hHeap;

    // Store Keyword string.
    m_pszFeature = pszFeature;


    //
    // Enumerate Options.
    //


    // Some features require special handling for initializing their options.
    // EnumOpionts isn't implemented for these features.
    // Return of E_NOTIMPL indicates it isn't the feature doesn't
    // need special handling.
    hrResult = GetOptionsForSpecialFeatures(Helper, poemuiobj);
    if( SUCCEEDED(hrResult)
        ||
        (!SUCCEEDED(hrResult) && (E_NOTIMPL != hrResult) )
      )
    {
        // We either dealt with the special feature or incounter an error
        // trying to deal with the special feature.

        goto Exit;
    }

    // To try to cut down on having to call EnumOptions more than once,
    // pre-allocate a buffer of reasonable size.
    m_dwSize = INITIAL_ENUM_OPTIONS_SIZE;
    m_pmszRaw = (PSTR) HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_dwSize);
    if(NULL == m_pmszRaw)
    {
        ERR(ERRORTEXT("COptions::Acquire() alloc for options for feature %hs failed.\r\n"), m_pszFeature);

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }


    // Try to get options list with initial buffer.
    hrResult = Helper.EnumOptions(poemuiobj, 0, m_pszFeature, m_pmszRaw, m_dwSize, &dwNeeded);
    if( (E_OUTOFMEMORY == hrResult) && (m_dwSize < dwNeeded))
    {
        PSTR    pTemp;


        // INVARIANT:  options list multi-sz wasn't large enough.

        // Re-allocate the buffer and try again.
        pTemp = (PSTR) HeapReAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_pmszRaw, dwNeeded);
        if(NULL == pTemp)
        {
            ERR(ERRORTEXT("COptions::Acquire() re-alloc for options list for feature %hs failed.\r\n"), m_pszFeature);

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }
        m_pmszRaw = pTemp;
        m_dwSize = dwNeeded;

        // Try again to get the options list.
        hrResult = Helper.EnumOptions(poemuiobj, 0, m_pszFeature, m_pmszRaw, m_dwSize, &dwNeeded);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("COptions::Acquire() failed to EnumOptions() for feature %hs after re-allocating buffer.\r\n"), m_pszFeature);

            goto Exit;
        }
    }

    // Make sure we got the option list.
    // Can't do anything more with out it.
    if(!SUCCEEDED(hrResult))
    {
        if(E_NOTIMPL != hrResult) ERR(ERRORTEXT("COptions::Acquire() failed to enumerate options for feature %hs. (hrResult = 0x%x)\r\n"), m_pszFeature, hrResult);

        goto Exit;
    }

    // INVARIANT:  successfully got option keyword list.

    // Create array of string pointers to the Option names
    // in the multi-sz we got from EnumOptions().
    hrResult = MakeStrPtrList(m_hHeap, m_pmszRaw, &m_ppszOptions, &m_wOptions);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("COptions::Acquire() failed to create pointer list to options. (hrResult = 0x%x)\r\n"), hrResult);

        goto Exit;
    }

    //
    //  Build Option Information
    //

    // Allocate array to hold feature info
    m_pInfo = (POPTION_INFO) HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_wOptions * sizeof(OPTION_INFO));
    if(NULL == m_pInfo)
    {
        ERR(ERRORTEXT("COptions::Acquire() failed to alloc feature info array.\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    // For each option, build or get useful information, such as display name.
    for(WORD wIndex = 0; wIndex < m_wOptions; ++wIndex)
    {
        POPTION_INFO    pCurrent    = m_pInfo + wIndex;


        // Get or build a keyword mapping entry
        // that maps from keyword to usefully where to get info, such as
        // display name, icon, option type, for keywords that may not be
        // able to get info for from Helper.
        pCurrent->pMapping = FindKeywordMapping(gkmOptionMap, NUM_OPTION_MAP, m_ppszOptions[wIndex]);

        // Get display names for each of the Options.
        // The function implements a heuristic for detemining the display name,
        // since can't get the display name from the UI Helper for all Options.
        hrResult = DetermineOptionDisplayName(m_hHeap,
                                              Helper,
                                              poemuiobj,
                                              m_pszFeature,
                                              m_ppszOptions[wIndex],
                                              pCurrent->pMapping,
                                              &pCurrent->pszDisplayName);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("COptions::Acquire() failed to get display name for %hs of feature %hs. (hrResult = 0x%x)\r\n"),
                          m_ppszOptions[wIndex],
                          m_pszFeature,
                          hrResult);

            goto Exit;
        }
    }

    //
    // Get current option selection.
    //

    hrResult = GetOptionSelectionIndex(Helper, poemuiobj);


Exit:

    // Clean up if weren't successful.
    if(!SUCCEEDED(hrResult))
    {
        Clear();
    }

    VERBOSE(DLLTEXT("COptions::Acquire() returning with HRESULT of 0x%x\r\n"), hrResult);

    return hrResult;
}

// Return nth options keyword.
PCSTR COptions::GetKeyword(WORD wIndex) const
{
    // Validate parameters.
    if( (wIndex >= m_wOptions)
        ||
        (NULL == m_ppszOptions)
      )
    {
        return NULL;
    }

    return m_ppszOptions[wIndex];
}

// Return nth options display name.
PCWSTR COptions::GetName(WORD wIndex) const
{
    // Validate parameters.
    if( (wIndex >= m_wOptions)
        ||
        (NULL == m_pInfo)
      )
    {
        ERR(ERRORTEXT("COptions::GetName() invalid parameters.\r\n"));

        return NULL;
    }

    if(NULL == m_pInfo[wIndex].pszDisplayName) ERR(ERRORTEXT("COptions::GetName() returning NULL option display name.\r\n"));

    return m_pInfo[wIndex].pszDisplayName;
}

// Find option with matching keyword string.
WORD COptions::FindOption(_In_opt_ PCSTR pszOption, WORD wDefault) const
{
    BOOL    bFound  = FALSE;
    WORD    wMatch  = wDefault;


    // Validate parameters.
    if( (NULL == pszOption)
        ||
        (NULL == m_ppszOptions)
      )
    {
        return wDefault;
    }

    // Walk the option keyword looking for a match.
    for(WORD wIndex = 0; !bFound && (wIndex < m_wOptions); ++wIndex)
    {
        bFound = !lstrcmpA(pszOption, m_ppszOptions[wIndex]);
        if(bFound)
        {
            wMatch = wIndex;
        }
    }

    return wMatch;
}

// Initializes OptItem with options for a feature.
HRESULT COptions::InitOptItem(_In_ HANDLE hHeap, POPTITEM pOptItem)
{
    WORD    wParams     = 0;
    WORD    wOptions    = 0;
    HRESULT hrResult    = S_OK;


    // Set option selection.
    pOptItem->pSel = m_pSel;

    // Get count of number of options.
    // NOTE: Some feature options have no counts.
    wOptions = GetCount();

    // Different OPTTYPE types require different number of OPTPARAMs.
    switch(m_cType)
    {
        // For up down arrow control, the OPTPARAMs need is 2.
        case TVOT_UDARROW:
            wParams = 2;
            break;

        // For combobox, the OPTPARAMs needed is on per options.
        case TVOT_COMBOBOX:
            wParams = wOptions;
            break;

        // The default is the option count.
        default:
            WARNING(DLLTEXT("COptions::InitOptItem() OPTTYPE type %d num of OPTPARAMs not handled. Default to option count of %d.\r\n"),
                            m_cType,
                            wOptions);
            wParams = wOptions;
            break;
    }

    // Only do OPTTYPEs if we have non-Zero number of OPTPARAMs.
    // Every OPTTYPE has at leas one OPTPARAM.
    if(0 < wParams)
    {
        // Allocate memory for feature options.
        pOptItem->pOptType = CreateOptType(hHeap, wParams);
        if(NULL == pOptItem->pOptType)
        {
            ERR(ERRORTEXT("COptions::InitOptItem() failed to allocate OPTTYPEs for OPTITEM %hs.\r\n"),
                           m_pszFeature);

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }

        // Set OPTTYPE.
        pOptItem->pOptType->Type = m_cType;

        // Different OPTTYPE types require different initialization.
        switch(m_cType)
        {
            // For up down arrow control, OPTPARAM[0] is used by the contrl.
            // pOptParam[0]->pData is the Units description string.
            // pOptParam[1].IconID is the min limit.
            // pOptParam[1].lParam is the max limit.
            case TVOT_UDARROW:
                assert(2 == wParams);
                pOptItem->pOptType->pOptParam[0].pData  = m_pszUnits;
                pOptItem->pOptType->pOptParam[1].IconID = m_ptRange.x;
                pOptItem->pOptType->pOptParam[1].lParam = m_ptRange.y;
                break;

            // For combobox, the pOptParam[n].pData is the display name of the option.
            case TVOT_COMBOBOX:
                for(WORD wIndex = 0; wIndex < wParams; ++wIndex)
                {
                    pOptItem->pOptType->pOptParam[wIndex].pData = const_cast<PWSTR>(GetName(wIndex));
                }
                break;

            default:
                ERR(ERRORTEXT("COptions::InitOptItem() OPTTYPE type %d OPTTYPE init not handled.\r\n"),
                              m_cType);
                break;
        }
    }


Exit:

    return hrResult;
}

// Refresh option selection.
HRESULT COptions::RefreshSelection(CUIHelper &Helper, POEMUIOBJ poemuiobj)
{
    HRESULT     hrResult = S_OK;


    // Method for getting option selection is based
    // on OPTTYPE type.
    switch(m_cType)
    {

        case TVOT_UDARROW:
            hrResult = GetOptionSelectionShort(Helper, poemuiobj);
            break;

        case TVOT_COMBOBOX:
            hrResult = GetOptionSelectionIndex(Helper, poemuiobj);
            break;

        default:
            ERR(ERRORTEXT("COptions::RefreshSelection() not handled for type %d OPTTYPE.\r\n"),
                           m_cType);
            break;
    }

    return hrResult;
}



////////////////////////////////////////////
//
//  CFeatures Methods
//

//
//  Private Methods
//

// Initializes class
void CFeatures::Init()
{
    // Initialize data members.
    m_wFeatures         = 0;
    m_wDocFeatures      = 0;
    m_wPrintFeatures    = 0;
    m_pmszRaw           = NULL;
    m_ppszKeywords      = NULL;
    m_dwSize            = 0;
    m_hHeap             = NULL;
    m_pInfo             = NULL;
}

// Cleans up class and re-initialize it.
void CFeatures::Clear()
{
    // Free memory associated with data members.
    if(NULL != m_pmszRaw)       HeapFree(m_hHeap, 0, m_pmszRaw);
    if(NULL != m_ppszKeywords)  HeapFree(m_hHeap, 0, m_ppszKeywords);

    // Free feature info
    FreeFeatureInfo();

    // Re-initialize
    Init();
}

// Free feature info
void CFeatures::FreeFeatureInfo()
{
    // Validate parameters.
    if( (NULL == m_hHeap)
        ||
        (NULL == m_pInfo)
      )
    {
        return;
    }

    // Free memory associated with feature info.
    for(WORD wIndex = 0; wIndex < m_wFeatures; ++wIndex)
    {
        PWSTR   pszDisplay =  m_pInfo[wIndex].pszDisplayName;


        // Free display name.
        if( (NULL != pszDisplay)
            &&
            !IS_INTRESOURCE(pszDisplay)
          )
        {
            HeapFree(m_hHeap, 0, pszDisplay);
        }
    }

    // Free feature info array.
    // Feature Info array allocated with new so
    // that each of the constructors for COptions
    // in fhte Feature Info array will be called.
    delete[] m_pInfo;
}

// Turns index for mode to modeless index, which
// is the real index to the feature.
WORD CFeatures::GetModelessIndex(WORD wIndex, DWORD dwMode) const
{
    WORD    wCount = 0;


    switch(dwMode)
    {
        // Number of features, all modes
        case 0:
            wCount = wIndex;
            break;

        // Find the nth feature that matches the mode
        case OEMCUIP_DOCPROP:
        case OEMCUIP_PRNPROP:
            // Walk the feature list looking for nth feature
            // with matching mode.
            for(wCount = 0; wCount < m_wFeatures; ++wCount)
            {
                // Count down to the feature we want.
                // Only count down for matching modes.
                if(dwMode == m_pInfo[wCount].dwMode)
                {
                    if(0 == wIndex)
                    {
                        break;
                    }
                    else
                    {
                        --wIndex;
                    }
                }
            }
            break;
    }

    return wCount;
}


//
//  Public Methods
//

// Default constructor
CFeatures::CFeatures()
{
    Init();
}

// Destructor
CFeatures::~CFeatures()
{
    // Clean up class.
    Clear();
}

// Gets Core Driver Features, if not already retrieved.
HRESULT CFeatures::Acquire(_In_ HANDLE hHeap,
                           CUIHelper &Helper,
                           _In_ POEMUIOBJ poemuiobj
                          )
{
    WORD    wIndex      = 0;
    DWORD   dwNeeded    = 0;
    HRESULT hrResult    = S_OK;


    VERBOSE(DLLTEXT("CFeatures::Acquire(0x%p, Helper, 0x%p) entered.\r\n"),
            hHeap,
            poemuiobj);

    // Don't retreive the Features again if we already got them.
    if(0 < m_wFeatures)
    {
        VERBOSE(DLLTEXT("CFeatures::Acquire() features already enumerated.\r\n"));
        VERBOSE(DLLTEXT("CFeatures::Acquire() returning S_OK.\r\n"));

        return S_OK;
    }

    // Save the heap handle for use later, such as freeing memory at destruction.
    m_hHeap = hHeap;

    //
    // Enumerate features.
    //

    // To try to cut down on having to call EnumFeatures more than once,
    // pre-allocate a buffer of reasonable size.
    m_dwSize = INITIAL_ENUM_FEATURES_SIZE;
    m_pmszRaw = (PSTR) HeapAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_dwSize);
    if(NULL == m_pmszRaw)
    {
        ERR(ERRORTEXT("CFeatures::Acquire() alloc for feature list failed.\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }


    // Try to get feature list with initial buffer.
    hrResult = Helper.EnumFeatures(poemuiobj, 0, m_pmszRaw, m_dwSize, &dwNeeded);
    if( (E_OUTOFMEMORY == hrResult) && (m_dwSize < dwNeeded))
    {
        PSTR    pTemp;


        // INVARIANT:  feature list multi-sz wasn't large enough.


        // Re-allocate the buffer and try again.
        pTemp = (PSTR) HeapReAlloc(m_hHeap, HEAP_ZERO_MEMORY, m_pmszRaw, dwNeeded);
        if(NULL == pTemp)
        {
            ERR(ERRORTEXT("CFeatures::Acquire() re-alloc for feature list failed.\r\n"));

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }
        m_pmszRaw = pTemp;
        m_dwSize = dwNeeded;

        // Try again to get the feature list.
        hrResult = Helper.EnumFeatures(poemuiobj, 0, m_pmszRaw, m_dwSize, &dwNeeded);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("CFeatures::Acquire() failed to EnumFeatures() after re-allocating buffer.\r\n"));

            goto Exit;
        }
    }

    // Make sure we got the feature list.
    // Can't do anything more with out it.
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("CFeatures::Acquire() failed to enumerate features. (hrResult = 0x%x)\r\n"), hrResult);

        goto Exit;
    }

    // INVARIANT:  successfully got feature keyword list.

    // Create array of string pointers to the feature keywords
    // in the multi-sz we got from EnumFreatures().
    hrResult = MakeStrPtrList(m_hHeap, m_pmszRaw, &m_ppszKeywords, &m_wFeatures);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("CFeatures::Acquire() failed to create pointer list to keywords. (hrResult = 0x%x)\r\n"), hrResult);

        goto Exit;
    }


    //
    //  Build Feature Information
    //


    // Allocate array to hold feature info
    // Use new for allocation so class
    // constructors/destructors get called.
    m_pInfo = new FEATURE_INFO[m_wFeatures];
    if(NULL == m_pInfo)
    {
        ERR(ERRORTEXT("CFeatures::Acquire() failed to alloc feature info array.\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    // For each feature, build/get feature info....
    for(wIndex = 0; wIndex < m_wFeatures; ++wIndex)
    {
        PFEATURE_INFO   pCurrent    = m_pInfo + wIndex;


        // Get or build a keyword mapping entry
        // that maps from keyword to usefully where to get info, such as
        // display name, icon, option type, for keywords that may not be
        // able to get info for from Helper.
        pCurrent->pMapping = FindKeywordMapping(gkmFeatureMap, NUM_FEATURE_MAP, m_ppszKeywords[wIndex]);

        // Get display names for each of the featurs.
        // The function implements a heuristic for detemining the display name,
        // since can't get the display name from the UI Helper for all features.
        hrResult = DetermineFeatureDisplayName(m_hHeap,
                                               Helper,
                                               poemuiobj,
                                               m_ppszKeywords[wIndex],
                                               pCurrent->pMapping,
                                               &pCurrent->pszDisplayName);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("CFeatures::Acquire() failed to get display name for feature %hs. (hrResult = 0x%x)\r\n"),
                          m_ppszKeywords[wIndex],
                          hrResult);

            goto Exit;
        }

        // Get options for each feature.
        // NOTE: some features don't have options; the HRESULT will be E_NOTIMPL for these.
        hrResult = pCurrent->Options.Acquire(hHeap,
                                             Helper,
                                             poemuiobj,
                                             m_ppszKeywords[wIndex]);
        if(!SUCCEEDED(hrResult) && (E_NOTIMPL != hrResult))
        {
            ERR(ERRORTEXT("CFeatures::Acquire() failed to get options for feature %hs. (hrResult = 0x%x)\r\n"),
                          m_ppszKeywords[wIndex],
                          hrResult);

            goto Exit;
        }

        // Determine if feature is Printer or Document sticky.
        hrResult = DetermineStickiness(Helper,
                                       poemuiobj,
                                       m_ppszKeywords[wIndex],
                                       pCurrent->pMapping,
                                       &pCurrent->dwMode);
        // Don't propagate error if failure from unhandled driver feature.
        if( !SUCCEEDED(hrResult)
            &&
            !IS_DRIVER_FEATURE(m_ppszKeywords[wIndex])
          )
        {
            ERR(ERRORTEXT("CFeatures::Acquire() failed to determine stickiness for feature %hs. (hrResult = 0x%x)\r\n"),
                          m_ppszKeywords[wIndex],
                          hrResult);

            goto Exit;
        }

        // Keep track of mode counts.
        switch(pCurrent->dwMode)
        {
            case OEMCUIP_DOCPROP:
                ++m_wDocFeatures;
                break;

            case OEMCUIP_PRNPROP:
                ++m_wPrintFeatures;
                break;

            default:
                ERR(ERRORTEXT("CFeatures::Acquire() unknown stickiness for feature %hs.\r\n"),
                              m_ppszKeywords[wIndex]);
                break;
        }

    }

    // INVARIANT:  successfully build feature list.

    // Make sure that we always return success if we reach this point.
    hrResult = S_OK;


Exit:

    // Clean up if weren't successful.
    if(!SUCCEEDED(hrResult))
    {
        Clear();
    }


    VERBOSE(DLLTEXT("CFeatures::Acquire() returning HRESULT of 0x%x.\r\n"), hrResult);

    return hrResult;
}

// Returns number of features contained in class instance.
WORD CFeatures::GetCount(DWORD dwMode) const
{
    WORD    wCount = 0;


    switch(dwMode)
    {
        // Number of features, all modes
        case 0:
            wCount = m_wFeatures;
            break;

        case OEMCUIP_DOCPROP:
            wCount = m_wDocFeatures;
            break;

        case OEMCUIP_PRNPROP:
            wCount = m_wPrintFeatures;
            break;

    }

    VERBOSE(DLLTEXT("CFeatures::GetCount() returning %d\r\n"), wCount);

    return wCount;
}

// Returns nth feature's keyword
PCSTR CFeatures::GetKeyword(WORD wIndex, DWORD dwMode) const
{
    // Validate parameters.
    if( (wIndex >= GetCount(dwMode))
        ||
        (NULL == m_ppszKeywords)
      )
    {
        return NULL;
    }

    // Get internal index.
    wIndex = GetModelessIndex(wIndex, dwMode);

    // Return keyword
    return m_ppszKeywords[wIndex];
}

// Return nth feature's Display Name.
PCWSTR CFeatures::GetName(WORD wIndex, DWORD dwMode) const
{
    // Validate parameters.
    if( (wIndex >= GetCount(dwMode))
        ||
        (NULL == m_pInfo)
      )
    {
        return NULL;
    }

    // Get internal index.
    wIndex = GetModelessIndex(wIndex, dwMode);

    // Return display name.
    return m_pInfo[wIndex].pszDisplayName;
}

// Returns pointer to option class for nth feature.
COptions* CFeatures::GetOptions(WORD wIndex, DWORD dwMode) const
{
    // Validate parameters.
    if( (wIndex >= GetCount(dwMode))
        ||
        (NULL == m_pInfo)
      )
    {
        return NULL;
    }

    // Get internal index.
    wIndex = GetModelessIndex(wIndex, dwMode);

    // Return options pointer.
    return &m_pInfo[wIndex].Options;
}

// Formats OPTITEM for specied feature.
HRESULT CFeatures::InitOptItem(_In_ HANDLE hHeap, POPTITEM pOptItem, WORD wIndex, DWORD dwMode)
{
    COptions            *pOptions   = NULL;
    HRESULT             hrResult    = S_OK;
    PFEATUREOPTITEMDATA pData       = NULL;


    // Validate parameters.
    if( (wIndex >= GetCount(dwMode))
        ||
        (NULL == m_pInfo)
        ||
        (NULL == pOptItem)
      )
    {
        return E_INVALIDARG;
    }

    // Map mode index to internal index.
    wIndex = GetModelessIndex(wIndex, dwMode);

    // Get name of feature.
    pOptItem->pName = const_cast<PWSTR>(GetName(wIndex));

    // Add feature OPTITEM data to OPTITEM to facilitate saving
    // selection changes.
    pData = (PFEATUREOPTITEMDATA) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(FEATUREOPTITEMDATA));
    if(NULL == pData)
    {
        ERR(ERRORTEXT("CFeatures::InitOptItem() failed to allocated memory for feature OPTITEM data."));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }
    pData->dwSize               = sizeof(FEATUREOPTITEMDATA);
    pData->dwTag                = FEATURE_OPTITEM_TAG;
    pData->pszFeatureKeyword    = GetKeyword(wIndex);
    pData->pOptions             = GetOptions(wIndex);
    pOptItem->UserData          = (ULONG_PTR) pData;


    // Get pointer to options for this feature.
    // NOTE: some features do not have option list for various reasons.
    pOptions = GetOptions(wIndex);
    if(NULL != pOptions)
    {
        // Initialize COption parts of the OPTITEM
        hrResult = pOptions->InitOptItem(hHeap, pOptItem);
    }

Exit:

    Dump(pOptItem);

    return hrResult;
}

//////////////////////////////////////////////////
//
//  Regular functions not part of class
//
//////////////////////////////////////////////////


// Maps feature keywords to display names for the features.
HRESULT DetermineFeatureDisplayName(_In_ HANDLE hHeap,
                                    CUIHelper &Helper,
                                    POEMUIOBJ poemuiobj,
                                    _In_ PCSTR pszKeyword,
                                    const PKEYWORDMAP pMapping,
                                    _Outptr_result_maybenull_ PWSTR *ppszDisplayName)
{
    DWORD   dwDataType  = 0;
    DWORD   dwSize      = INITIAL_FEATURE_DISPLAY_NAME_SIZE;
    DWORD   dwNeeded    = 0;
    HRESULT hrResult    = S_OK;


    // Validate parameters.
    if( (NULL == hHeap)
        ||
        (NULL == pszKeyword)
        ||
        (NULL == ppszDisplayName)
      )
    {
        ERR(ERRORTEXT("DetermineFeatureDisplayName() invalid arguement.\r\n"));

        hrResult = E_INVALIDARG;
        goto Exit;
    }

    //
    // Call the Helper function.
    //

    // Helper will return Display Names for PPD Features, but
    // not for Driver Synthisized features (i.e. features prefixed with %).
    // Do it for Driver Synthisized features, just in case the helper
    // interface changes to support it.

    // Pre-allocate a buffer of reasonable size to try
    // to one have to call GetFeatureAttribute() once.
    *ppszDisplayName = (PWSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize);
    if(NULL == *ppszDisplayName)
    {
        ERR(ERRORTEXT("DetermineFeatureDisplayName() alloc for feature display name failed.\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    // Try to get diplay name for feature from Helper.
    hrResult = Helper.GetFeatureAttribute(poemuiobj,
                                          0,
                                          pszKeyword,
                                          "DisplayName",
                                          &dwDataType,
                                          (PBYTE) *ppszDisplayName,
                                          dwSize,
                                          &dwNeeded);
    if( (E_OUTOFMEMORY == hrResult) && (dwSize < dwNeeded))
    {
        PWSTR   pTemp;


        // INVARIANT: initial buffer wasn't large enough.

        // Re-alloc buffer and try again.
        pTemp = (PWSTR) HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, *ppszDisplayName, dwNeeded);
        if(NULL == pTemp)
        {
            ERR(ERRORTEXT("DetermineFeatureDisplayName() re-alloc for feature display name failed.\r\n"));

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }
        *ppszDisplayName = pTemp;

        // Try to get the display name from Helper, again.
        hrResult = Helper.GetFeatureAttribute(poemuiobj,
                                              0,
                                              pszKeyword,
                                              "DisplayName",
                                              &dwDataType,
                                              (PBYTE) *ppszDisplayName,
                                              dwNeeded,
                                              &dwNeeded);
    }

    if(SUCCEEDED(hrResult))
    {
        // INVARIANT:  Successfully got display name from Helper for feature.
        //             Don't need to do anything more.

        // Check the data type, it should be kADT_UNICODE.
        if(kADT_UNICODE != dwDataType) WARNING(DLLTEXT("DetermineFeatureDisplayName() feature attribute type not kADT_UNICODE. (dwDataType = %d)\r\n"), dwDataType);

        goto Exit;
    }

    // INVARIANT:  Did not get the display name from the Helper.

    // Free memory allocated for call to Helper.
    if(NULL != *ppszDisplayName)
    {
        HeapFree(hHeap, 0, *ppszDisplayName);
        *ppszDisplayName = NULL;
    }

    // Try alternative methods for getting the display name other
    // than from the Helper function.
    // If we have a mapping entry, then try to get resource string
    // for the display name.
    // Otherwise, covert the keyword to UNICODE and use that.
    if(NULL != pMapping)
    {
        //
        // Try mapping the keyword to resource string.
        //

        hrResult = GetDisplayNameFromMapping(hHeap, pMapping, ppszDisplayName);
    }
    else
    {
        //
        // Convert the keyword to UNICODE and use that.
        //

        // Convert ANSI keyword to Unicode string for display name.
        // Need to remove the % for Driver Synthisized features.
        // For debug version, add marker that shows that the display name was faked.
        PCSTR   pConvert = IS_DRIVER_FEATURE(pszKeyword) ? pszKeyword + 1 : pszKeyword;
    #if DBG
        CHAR    szTemp[256];
        if(FAILED(StringCbPrintfA(szTemp, sizeof(szTemp), "%s (Keyword)", pConvert)))
        {
            ERR(ERRORTEXT("DetermineFeatureDisplayName() StringCbPrintfA() called failed.\r\n"));
        }
        pConvert = szTemp;
    #endif
        *ppszDisplayName = MakeUnicodeString(hHeap, pConvert);
        if(NULL == *ppszDisplayName)
        {
            ERR(ERRORTEXT("DetermineFeatureDisplayName() alloc for feature display name failed.\r\n"));

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }

        // Return success even though we faked a display name.
        hrResult = S_OK;
    }


Exit:

    // If failed, then return no string.
    if(!SUCCEEDED(hrResult))
    {
        if(ppszDisplayName && NULL != *ppszDisplayName)
        {
            HeapFree(hHeap, 0, *ppszDisplayName);
            *ppszDisplayName = NULL;
        }
    }

    return hrResult;
}

// Maps option keywords to display names for the option.
HRESULT DetermineOptionDisplayName(_In_ HANDLE hHeap,
                                   CUIHelper &Helper,
                                   POEMUIOBJ poemuiobj,
                                   _In_ PCSTR pszFeature,
                                   _In_ PCSTR pszOption,
                                   const PKEYWORDMAP pMapping,
                                   _Outptr_result_maybenull_ PWSTR *ppszDisplayName)
{
    DWORD   dwDataType  = 0;
    DWORD   dwSize      = INITIAL_OPTION_DISPLAY_NAME_SIZE;
    DWORD   dwNeeded    = 0;
    HRESULT hrResult    = S_OK;


    // Validate parameters.
    if( (NULL == hHeap)
        ||
        (NULL == pszFeature)
        ||
        (NULL == pszOption)
        ||
        (NULL == ppszDisplayName)
      )
    {
        ERR(ERRORTEXT("DetermineOptionDisplayName() invalid arguement.\r\n"));

        hrResult = E_INVALIDARG;
        goto Exit;
    }

    //
    // Call the Helper function.
    //

    // Helper will return Display Names for PPD Feature Options, but
    // not for Driver Synthisized features options (i.e. features prefixed with %).
    // Do it for all options, just in case the helper interface changes to support it.

    // Pre-allocate a buffer of reasonable size to try
    // to one have to call GetOptionAttribute() once.
    *ppszDisplayName = (PWSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize);
    if(NULL == *ppszDisplayName)
    {
        ERR(ERRORTEXT("DetermineOptionDisplayName() alloc for feature display name failed.\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    // Try to get diplay name for feature from Helper.
    hrResult = Helper.GetOptionAttribute(poemuiobj,
                                         0,
                                         pszFeature,
                                         pszOption,
                                         "DisplayName",
                                         &dwDataType,
                                         (PBYTE) *ppszDisplayName,
                                         dwSize,
                                         &dwNeeded);
    if( (E_OUTOFMEMORY == hrResult) && (dwSize < dwNeeded))
    {
        PWSTR   pTemp;


        // INVARIANT: initial buffer wasn't large enough.

        // Re-alloc buffer and try again.
        pTemp = (PWSTR) HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, *ppszDisplayName, dwNeeded);
        if(NULL == pTemp)
        {
            ERR(ERRORTEXT("GetOptionAttribute() re-alloc for feature display name failed.\r\n"));

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }
        *ppszDisplayName = pTemp;

        // Try to get the display name from Helper, again.
        hrResult = Helper.GetOptionAttribute(poemuiobj,
                                             0,
                                             pszFeature,
                                             pszOption,
                                             "DisplayName",
                                             &dwDataType,
                                             (PBYTE) *ppszDisplayName,
                                             dwNeeded,
                                             &dwNeeded);
    }

    if(SUCCEEDED(hrResult))
    {
        // INVARIANT:  Successfully got display name from Helper for feature.
        //             Don't need to do anything more.

        // Check the data type, it should be kADT_UNICODE.
        if(kADT_UNICODE != dwDataType) WARNING(DLLTEXT("DetermineOptionDisplayName() feature attribute type not kADT_UNICODE. (dwDataType = %d)\r\n"), dwDataType);

        goto Exit;
    }

    // INVARIANT:  Did not get the display name from the Helper.

    // Free memory allocated for call to Helper.
    if(NULL != *ppszDisplayName)
    {
        HeapFree(hHeap, 0, *ppszDisplayName);
        *ppszDisplayName = NULL;
    }

    // Try alternative methods for getting the display name other
    // than from the Helper function.
    // If we have a mapping entry, then try to get resource string
    // for the display name.
    // Otherwise, covert the keyword to UNICODE and use that.
    if(NULL != pMapping)
    {
        //
        // Try mapping the keyword to resource string.
        //

        hrResult = GetDisplayNameFromMapping(hHeap, pMapping, ppszDisplayName);
    }
    else
    {
        //
        // Convert the keyword to UNICODE and use that.
        //

        // Convert ANSI keyword to Unicode string for display name.
        // For debug version, add marker that shows that the display name was faked.
        PCSTR   pConvert    = pszOption;
    #if DBG
        CHAR    szTemp[256];
        if(FAILED(StringCbPrintfA(szTemp, sizeof(szTemp), "%s (Keyword)", pConvert)))
        {
            ERR(ERRORTEXT("DetermineOptionDisplayName() StringCbPrintfA() called failed.\r\n"));
        }
        pConvert = szTemp;
    #endif
        *ppszDisplayName = MakeUnicodeString(hHeap, pConvert);
        if(NULL == *ppszDisplayName)
        {
            ERR(ERRORTEXT("DetermineOptionDisplayName() alloc for feature display name failed.\r\n"));

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }

        // Return success even though we faked a display name.
        hrResult = S_OK;
    }


Exit:

    // If failed, then return no string.
    if(!SUCCEEDED(hrResult))
    {
        if(ppszDisplayName && NULL != *ppszDisplayName)
        {
            HeapFree(hHeap, 0, *ppszDisplayName);
            *ppszDisplayName = NULL;
        }
    }

    return hrResult;
}

// Determines sticky mode for the feature.
HRESULT DetermineStickiness(CUIHelper &Helper,
                            POEMUIOBJ poemuiobj,
                            PCSTR pszKeyword,
                            const PKEYWORDMAP pMapping,
                            PDWORD pdwMode)
{
    CHAR    szGroupType[32]     = {0};
    DWORD   dwType              = 0;
    DWORD   dwNeeded            = 0;
    HRESULT hrResult            = S_OK;


    // Use mapping to see what stickiness of the feature is.
    if(NULL != pMapping)
    {
        *pdwMode = pMapping->dwMode;
        goto Exit;
    }

    // By default make feature Document sticky, if we don't have mapping.
    *pdwMode = OEMCUIP_DOCPROP;

    // Try to use Helper to determine stickiness.
    hrResult = Helper.GetFeatureAttribute(poemuiobj,
                                          0,
                                          pszKeyword,
                                          "OpenGroupType",
                                          &dwType,
                                          (PBYTE) szGroupType,
                                          sizeof(szGroupType),
                                          &dwNeeded);
    if(SUCCEEDED(hrResult))
    {
        // INVARIANT:  found out if feature is an installable option.
        //             Installable options are the only PPD features
        //             that are Printer sticky.

        if(!lstrcmpA(szGroupType, "InstallableOptions"))
        {
            *pdwMode = OEMCUIP_PRNPROP;
        }
        goto Exit;
    }


Exit:

    return hrResult;
}

// Find the mapping entry from the keyword.
PKEYWORDMAP FindKeywordMapping(PKEYWORDMAP pKeywordMap, WORD wMapSize, PCSTR pszKeyword)
{
    BOOL        bFound      = FALSE;
    PKEYWORDMAP pMapping    = NULL;


    // Walk mapping array for matching keyword.
    for(WORD wIndex = 0; !bFound && (wIndex < wMapSize); ++wIndex)
    {
        bFound = !lstrcmpA(pszKeyword, pKeywordMap[wIndex].pszKeyword);
        if(bFound)
        {
            pMapping = pKeywordMap + wIndex;
        }
    }

    return pMapping;
}

// Get display name from mapping entry.
HRESULT GetDisplayNameFromMapping(_In_ HANDLE hHeap, PKEYWORDMAP pMapping, _Outptr_result_maybenull_ PWSTR *ppszDisplayName)
{
    HMODULE hModule     = NULL;
    HRESULT hrResult    = S_OK;


    // Validate parameters.
    if( (NULL == hHeap)
        ||
        (NULL == pMapping)
        ||
        (NULL == ppszDisplayName)
      )
    {
        hrResult = E_INVALIDARG;
        goto Exit;
    }

    // Check for simple case of returning INT resource.
    if( (NULL == pMapping->pwszModule)
        ||
        IS_MAPPING_INT_RESOURCE(pMapping)
      )
    {
        // Just need to do MAKEINTRESOURCE on the resource ID and return.
        *ppszDisplayName = MAKEINTRESOURCE(pMapping->uDisplayNameID);
        goto Exit;
    }

    // We only need to get the module if we aren't loading the resource from
    // this module (i.e. if module name isn't NULL).
    // Also, as an optimization, we assume that the module has already been loaded,
    // since the only cases currently are this module, driver ui, and Compstui.dll.
    hModule = GetModuleHandle(pMapping->pwszModule);
    if(NULL == hModule)
    {
        DWORD   dwError = GetLastError();


        ERR(ERRORTEXT("GetDisplayNameFromMapping() for failed to load module %s. (Error %d)\r\n"),
                      pMapping->pwszModule,
                      dwError);

        hrResult = HRESULT_FROM_WIN32(dwError);
        goto Exit;
    }

    // INVARIANT:  have handle to module to load resource from or the
    //             resource is being loaded from this module.

    // Get the string resouce.
    hrResult = GetStringResource(hHeap, hModule, pMapping->uDisplayNameID, ppszDisplayName);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("GetDisplayNameFromMapping() failed to load string. (hrResult = 0x%x)\r\n"),
                      hrResult);
        goto Exit;
    }


Exit:

    return hrResult;
}

// Test if an OPTITEM is an OPTITEM for a feature.
// Macro for testing if OPTITEM is feature OPTITEM
BOOL IsFeatureOptitem(POPTITEM pOptItem)
{
    BOOL                bRet    = FALSE;
    PFEATUREOPTITEMDATA pData   = NULL;


    // Make sure pointers are NULL.
    if( (NULL == pOptItem)
        ||
        (NULL == pOptItem->UserData)
       )
    {
        // INVARIANT:  can't be feature OPTITEM, since one of
        //             the necessary pointer are NULL.

        return FALSE;
    }

    // For convienience, assign to pointer to feature OPTITEM data.
    pData = (PFEATUREOPTITEMDATA)(pOptItem->UserData);

    // Check size and tag.
    bRet = (sizeof(FEATUREOPTITEMDATA) == pData->dwSize)
           &&
           (FEATURE_OPTITEM_TAG == pData->dwTag);

    return bRet;
}



// Walks array of OPTITEMs saving each feature OPTITEM
// that has changed.
HRESULT SaveFeatureOptItems(_In_ HANDLE hHeap,
                            CUIHelper *pHelper,
                            POEMUIOBJ poemuiobj,
                            HWND hWnd,
                            POPTITEM pOptItem,
                            WORD wItems)
{
    PSTR        pmszPairs           = NULL;
    WORD        wPairs              = 0;
    WORD        wReasons            = 0;
    DWORD       dwSize              = 0;
    DWORD       dwResult            = 0;
    PCSTR       *ppszReasons        = NULL;
    PWSTR       pszConfilictFeature = NULL;
    PWSTR       pszConfilictOption  = NULL;
    PWSTR       pszCaption          = NULL;
    PWSTR       pszFormat           = NULL;
    PWSTR       pszMessage          = NULL;
    HRESULT     hrResult            = S_OK;


    // Validate parameters
    if( (NULL == hHeap)
        ||
        (NULL == pHelper)
        ||
        (NULL == pOptItem)
      )
    {
        ERR(ERRORTEXT("SaveFeatureOptItems() called with invalid parameters.\r\n"));

        hrResult = E_INVALIDARG;
        goto Exit;
    }

    // Get feature option pairs to save.
    hrResult = GetChangedFeatureOptions(hHeap, pOptItem, wItems, &pmszPairs, &wPairs, &dwSize);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("SaveFeatureOptItems() failed to get changed feature option pairs. (hrResult = 0x%x)\r\n"),
                       hrResult);

        goto Exit;
    }

    // Don't need to do anything more if no feature options changed.
    if(0 == wPairs || !pmszPairs)
    {
        VERBOSE(DLLTEXT("SaveFeatureOptItems() no feature options that need to be set.\r\n"));

        goto Exit;
    }

    // Set the change feature options.
    // For the first SetOptions() call, don't have the
    // core driver UI resolve conflicts, so we can
    // prompt user for automatic resolution or let
    // them do the conflict resolving.
    hrResult = pHelper->SetOptions(poemuiobj,
                                   SETOPTIONS_FLAG_KEEP_CONFLICT,
                                   pmszPairs,
                                   dwSize,
                                   &dwResult);
    if(!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("SaveFeatureOptItems() call to SetOptions() failed. (hrResult = 0x%x, dwResult = %d\r\n"),
                       hrResult,
                       dwResult);

        goto Exit;
    }

    // Check to see if we were able to save changed feature options,
    // or if there is a conflict that needs resolution.
    if(SETOPTIONS_RESULT_CONFLICT_REMAINED == dwResult)
    {
        int         nRet;
        DWORD       dwRet;
        CONFLICT    Conflict;
        PKEYWORDMAP pMapping    = NULL;


        // INVARIANT:  constraint conflict, options weren't saved.

        // Get list of all features that have conflict.
        hrResult = GetFirstConflictingFeature(hHeap,
                                              pHelper,
                                              poemuiobj,
                                              pOptItem,
                                              wItems,
                                              &Conflict);
        if(!SUCCEEDED(hrResult))
        {
            goto Exit;
        }

        // Create string pointer list in to multi-sz of first conflict.
        hrResult = MakeStrPtrList(hHeap, Conflict.pmszReasons, &ppszReasons, &wReasons);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("SaveFeatureOptItems() failed to make string list for conflict reasons. (hrResult = 0x%x)\r\n"),
                           hrResult);

            goto Exit;
        }

        // Ensure that we have at least 2 reasons. Later code assumes that we do.
        if (wReasons < 2)
        {
            hrResult = E_UNEXPECTED;

            goto Exit;
        }

        //
        // Get display versions of feature/option conflict reason.
        //

        // Get or build a keyword mapping entry
        // that maps from keyword to usefully where to get info, such as
        // display name, icon, option type, for keywords that may not be
        // able to get info for from Helper.
        pMapping = FindKeywordMapping(gkmFeatureMap, NUM_FEATURE_MAP, ppszReasons[0]);

        // Get display names for each of the featurs.
        // The function implements a heuristic for detemining the display name,
        // since can't get the display name from the UI Helper for all features.
        hrResult = DetermineFeatureDisplayName(hHeap,
                                               *pHelper,
                                               poemuiobj,
                                               ppszReasons[0],
                                               pMapping,
                                               &pszConfilictFeature);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("SaveFeatureOptItems failed to get display name for feature %hs. (hrResult = 0x%x)\r\n"),
                          ppszReasons[0],
                          hrResult);

            goto Exit;
        }

        // Get or build a keyword mapping entry
        // that maps from keyword to usefully where to get info, such as
        // display name, icon, option type, for keywords that may not be
        // able to get info for from Helper.
        pMapping = FindKeywordMapping(gkmOptionMap, NUM_OPTION_MAP, ppszReasons[1]);

        // Get option display name.
        hrResult = DetermineOptionDisplayName(hHeap,
                                              *pHelper,
                                              poemuiobj,
                                              ppszReasons[0],
                                              ppszReasons[1],
                                              pMapping,
                                              &pszConfilictOption);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("SaveFeatureOptItems() failed to get display name for %hs of feature %hs. (hrResult = 0x%x)\r\n"),
                          ppszReasons[1],
                          ppszReasons[0],
                          hrResult);

            goto Exit;
        }

        //
        // Prompt user about how to resolve conflict.
        //

        // Get caption name.
        hrResult = GetStringResource(hHeap, ghInstance, IDS_NAME, &pszCaption);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("SaveFeatureOptItems() failed to get caption name. (hrResult = 0x%x)\r\n"),
                          hrResult);
            goto Exit;
        }

        // Get message body format string.
        hrResult = GetStringResource(hHeap, ghInstance, IDS_CONSTRAINT_CONFLICT, &pszFormat);
        if(!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("SaveFeatureOptItems() failed to get constraint conflict format. (hrResult = 0x%x)\r\n"),
                          hrResult);
            goto Exit;
        }


        // Get messsage body.
        PVOID   paArgs[4] = {pszConfilictFeature,
                             pszConfilictOption,
                             const_cast<PWSTR>(Conflict.pszFeature),
                             Conflict.pszOption
                            };
        dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                              pszFormat,
                              0,
                              0,
                              (PWSTR) &pszMessage,
                              0,
                              (va_list *) paArgs);
        if(0 == dwRet)
        {
            DWORD   dwError = GetLastError();

            ERR(ERRORTEXT("SaveFeatureOptItems() failed to FormatMessage() for constraint conflict of feature %hs option %hs. (Last Error %d)\r\n"),
                          Conflict.pszFeatureKeyword,
                          Conflict.pszOptionKeyword,
                          dwError);

            hrResult = HRESULT_FROM_WIN32(dwError);
            goto Exit;
        }

        // Display simple message box with prompt.
        nRet = MessageBox(hWnd, pszMessage, pszCaption, MB_YESNO | MB_ICONWARNING);

        // Check to see how user wants to resolve conflict.
        if(IDYES == nRet)
        {
            // Let core driver resolve conflict resolution.
            hrResult = pHelper->SetOptions(poemuiobj,
                                           SETOPTIONS_FLAG_RESOLVE_CONFLICT,
                                           pmszPairs,
                                           dwSize,
                                           &dwResult);

            // Conflict resolution requires refreshing current option
            // selection for each feature, since selection may have
            // changed because of conflict resolution.
            RefreshOptItemSelection(pHelper, poemuiobj, pOptItem, wItems);
        }

        // Return failure if there are still conflictts.
        if(SETOPTIONS_RESULT_CONFLICT_REMAINED == dwResult)
        {
            hrResult = E_FAIL;
        }
    }


Exit:

    // Clean up...

    // cleanup heap allocs.
    if(NULL != pmszPairs)       HeapFree(hHeap, 0, pmszPairs);
    if(NULL != ppszReasons)         HeapFree(hHeap, 0, ppszReasons);
    if(NULL != pszConfilictFeature) HeapFree(hHeap, 0, pszConfilictFeature);
    if(NULL != pszConfilictOption)  HeapFree(hHeap, 0, pszConfilictOption);
    if(NULL != pszCaption)          HeapFree(hHeap, 0, pszCaption);
    if(NULL != pszFormat)           HeapFree(hHeap, 0, pszFormat);
    if(NULL != pszMessage)          LocalFree(pszMessage);

    return hrResult;
}

// Allocates buffer, if needed, and calls IPrintCoreUI2::WhyConsrained
// to get reason for constraint.
HRESULT GetWhyConstrained(_In_ HANDLE hHeap,
                          CUIHelper *pHelper,
                          POEMUIOBJ poemuiobj,
                          _In_ PCSTR     pszFeature,
                          _In_ PCSTR     pszOption,
                          _Inout_ PSTR   *ppmszReason,
                          _Inout_ PDWORD pdwSize)
{
    PSTR    pmszReasonList  = *ppmszReason;
    DWORD   dwNeeded        = 0;
    HRESULT hrResult        = S_OK;


    // If buffer wasn't passed in, then allocate one.
    if( (NULL == pmszReasonList) || (0 == *pdwSize) )
    {
        // If no size or size is smaller than default, then change to default
        // size.  We want to try to only allocate and call once.
        if(*pdwSize < INITIAL_GET_REASON_SIZE)
        {
            *pdwSize = INITIAL_GET_REASON_SIZE;
        }

        // Alloc initial buffer for reason constrained.
        pmszReasonList = (PSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, *pdwSize);
        if(NULL == pmszReasonList)
        {
            ERR(ERRORTEXT("GetWhyConstrained() failed to alloc buffer for constraint reasons for feature %hs and option %hs.\r\n"),
                           pszFeature,
                           pszOption);

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    // Get reason for constraint.
    hrResult = pHelper->WhyConstrained(poemuiobj,
                                       0,
                                       pszFeature,
                                       pszOption,
                                       pmszReasonList,
                                       *pdwSize,
                                       &dwNeeded);
    if( (E_OUTOFMEMORY == hrResult)
        &&
        (*pdwSize < dwNeeded)
      )
    {
        PSTR    pTemp;


        // INVARIANT:  initial buffer not large enough.

        // Re-alloc buffer to needed size.
        pTemp = (PSTR) HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, pmszReasonList, dwNeeded);
        if(NULL == pTemp)
        {
            ERR(ERRORTEXT("GetWhyConstrained() failed to re-allocate buffer for constraint reason for feature %hs and option %hs.\r\n"),
                           pszFeature,
                           pszOption);

            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }
        pmszReasonList  = pTemp;
        *pdwSize        = dwNeeded;

        // Retry getting constaint reason.
        hrResult = pHelper->WhyConstrained(poemuiobj,
                                           0,
                                           pszFeature,
                                           pszOption,
                                           pmszReasonList,
                                           *pdwSize,
                                           &dwNeeded);
    }


Exit:

    // On error, do clean up.
    if(!SUCCEEDED(hrResult))
    {
        // Free reason buffer.
        if(NULL != pmszReasonList) HeapFree(hHeap, 0, pmszReasonList);

        // Return NULL and zero size.
        *ppmszReason    = NULL;
        *pdwSize        = 0;
    }
    else
    {
        *ppmszReason = pmszReasonList;
    }

    return hrResult;
}

// Creates multi-sz list of feature option pairs that have changed.
HRESULT GetChangedFeatureOptions(_In_ HANDLE           hHeap,
                                 POPTITEM              pOptItem,
                                 WORD                  wItems,
                                 _Outptr_result_maybenull_ PSTR  *ppmszPairs,
                                 _Out_ PWORD           pwPairs,
                                 _Out_ PDWORD          pdwSize)
{
    WORD                wCount      = 0;
    WORD                wChanged    = 0;
    WORD                wPairs      = 0;
    PSTR                pmszPairs   = NULL;
    DWORD               dwNeeded    = 2;
    DWORD               dwOffset    = 0;
    HRESULT             hrResult    = S_OK;
    PFEATUREOPTITEMDATA pData       = NULL;


    // Walk OPTITEM array looking or changed options,
    // and calculating size needed for multi-sz buffer.
    for(wCount = 0; wCount < wItems; ++wCount)
    {
        PSTR    pszOption   = NULL;


        // Just go to next item if this OPTITEM hasn't
        // changed or isn't a feature OPTITEM.
        if( !(OPTIF_CHANGEONCE & pOptItem[wCount].Flags)
            ||
            !IsFeatureOptitem(pOptItem + wCount)
          )
        {
            continue;
        }

        // For convienience, assign to pointer to feature OPTITEM data.
        pData = (PFEATUREOPTITEMDATA)(pOptItem[wCount].UserData);

        // Increment options changed and size needed.
        pszOption = GetOptionKeywordFromOptItem(hHeap, pOptItem + wCount);
        if(NULL != pszOption)
        {
            ++wChanged;

            DWORD dwTmp = 0;

            if (FAILED(hrResult = SizeTToDWord(strlen(pData->pszFeatureKeyword) + strlen(pszOption) + 2, &dwTmp)))
            {
                goto Exit;
            }

            dwNeeded += dwTmp;

            // Need to free option keyword string copy
            // allocated in GetOptionKeywordFromOptItem().
            HeapFree(hHeap, 0, pszOption);
            pszOption = NULL;
        }
    }

    // Don't need to do anything more if no feature options changed.
    if(0 == wChanged)
    {
        goto Exit;
    }

    // Allocate multi-sz buffer.
    pmszPairs = (PSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwNeeded);
    if(NULL == pmszPairs)
    {
        ERR(ERRORTEXT("GetChangedFeatureOptions() failed to allocate multi-sz.\r\n"));

        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    // Build mult-sz list of feature option pairs
    // that changed.
    for(wCount = 0, wPairs = 0; (wCount < wItems) && (wPairs < wChanged); ++wCount)
    {
        PSTR    pszOption   = NULL;


        // Just go to next item if this OPTITEM hasn't
        // changed or isn't a feature OPTITEM.
        if( !(OPTIF_CHANGEONCE & pOptItem[wCount].Flags)
            ||
            !IsFeatureOptitem(pOptItem + wCount)
          )
        {
            continue;
        }

        // For convienience, assign to pointer to feature OPTITEM data.
        pData = (PFEATUREOPTITEMDATA)(pOptItem[wCount].UserData);

        // Add feature option pair.
        pszOption = GetOptionKeywordFromOptItem(hHeap, pOptItem + wCount);
        if(NULL != pszOption)
        {
            if (FAILED(hrResult = StringCbCopyA(pmszPairs + dwOffset, dwNeeded - dwOffset, pData->pszFeatureKeyword)))
            {
                goto Exit;
            }

            DWORD dwTmp = 0;

            if (FAILED(hrResult = SizeTToDWord(strlen(pData->pszFeatureKeyword) + 1, &dwTmp)))
            {
                goto Exit;
            }

            dwOffset += dwTmp;

            if (FAILED(hrResult = StringCbCopyA(pmszPairs + dwOffset, dwNeeded - dwOffset, pszOption)))
            {
                goto Exit;
            }

            if (FAILED(hrResult = SizeTToDWord(strlen(pszOption) + 1, &dwTmp)))
            {
                goto Exit;
            }

            dwOffset += dwTmp;

            // Keep track of number of pairs added, so
            // we are able to exit loop as soon as
            // we added all changed feature options.
            ++wPairs;

            // Need to free option keyword string copy
            // allocated in GetOptionKeywordFromOptItem().
            HeapFree(hHeap, 0, pszOption);
            pszOption = NULL;
        }
    }


Exit:

    if(SUCCEEDED(hrResult))
    {
        // INVARIANT: either successfully build mutli-sz of changed
        //            feature option pairs, or there are no feature
        //            that options changed.

        // Return pairs and number of pairs.
        *pwPairs    = wPairs;
        *pdwSize    = dwNeeded;
        *ppmszPairs = pmszPairs;
    }
    else
    {
        // INVARINAT:   error.

        // Clean up.
        if(NULL == pmszPairs)    HeapFree(hHeap, 0, pmszPairs);

        // Return NULL and zero count.
        *pwPairs    = 0;
        *pdwSize    = 0;
        *ppmszPairs = NULL;
    }

    return hrResult;
}

// Returns pointer to option keyword for a feature OPTITEM.
PSTR GetOptionKeywordFromOptItem(_In_ HANDLE hHeap, POPTITEM pOptItem)
{
    char                szNumber[16]    = {0};
    PSTR                pszOption       = NULL;
    PFEATUREOPTITEMDATA pData           = NULL;


    // Validate parameter.
    if(!IsFeatureOptitem(pOptItem))
    {
        ERR(ERRORTEXT("GetOptionKeywordFromOptItem() invalid parameter.\r\n"));

        goto Exit;
    }

    // For convienience, assign to pointer to feature OPTITEM data.
    pData = (PFEATUREOPTITEMDATA)(pOptItem->UserData);

    // Option selection is based on type of OPTTYPE.
    switch(pOptItem->pOptType->Type)
    {
        // For up down arrow control, selection is just pOptItem->Sel
        // converted to string.
        case TVOT_UDARROW:
            if(FAILED(StringCbPrintfA(szNumber, sizeof(szNumber)/sizeof(szNumber[0]), "%u", pOptItem->Sel)))
            {
                ERR(ERRORTEXT("GetOptionKeywordFromOptItem() failed to convert number to string.\r\n"));
            }
            szNumber[sizeof(szNumber)/sizeof(szNumber[0]) - 1] = '\0';
            pszOption = MakeStringCopy(hHeap, szNumber);
            break;

        // For combobox, pOptItem->Sel is the index in to the
        // option array.
        case TVOT_COMBOBOX:
            pszOption = MakeStringCopy(hHeap, pData->pOptions->GetKeyword((WORD)pOptItem->Sel));
            break;

        // The default is the option count.
        default:
            ERR(ERRORTEXT("GetOptionKeywordFromOptItem() OPTTYPE type %d num of OPTPARAMs not handled.\r\n"),
                            pOptItem->pOptType->Type);

            goto Exit;
            break;
    }


Exit:

    return pszOption;
}

// Returns pointer to option display name for a feature OPTITEM.
PWSTR GetOptionDisplayNameFromOptItem(_In_ HANDLE hHeap, POPTITEM pOptItem)
{
    WCHAR               szNumber[16]    = {0};
    PWSTR               pszOption       = NULL;
    PFEATUREOPTITEMDATA pData           = NULL;


    // Validate parameter.
    if(!IsFeatureOptitem(pOptItem))
    {
        ERR(ERRORTEXT("GetOptionDisplayNameFromOptItem() invalid parameter.\r\n"));

        goto Exit;
    }

    // For convienience, assign to pointer to feature OPTITEM data.
    pData = (PFEATUREOPTITEMDATA)(pOptItem->UserData);

    // Option selection is based on type of OPTTYPE.
    switch(pOptItem->pOptType->Type)
    {
        // For up down arrow control, selection is just pOptItem->Sel
        // converted to string.
        case TVOT_UDARROW:
            if(FAILED(StringCbPrintfW(szNumber, sizeof(szNumber)/sizeof(szNumber[0]), L"%u", pOptItem->Sel)))
            {
                ERR(ERRORTEXT("GetOptionDisplayNameFromOptItem() failed to convert number to string.\r\n"));
            }
            szNumber[sizeof(szNumber)/sizeof(szNumber[0]) - 1] = L'\0';
            pszOption = MakeStringCopy(hHeap, szNumber);
            break;

        // For combobox, pOptItem->Sel is the index in to the
        // option array.
        case TVOT_COMBOBOX:
            pszOption = MakeStringCopy(hHeap, pOptItem->pOptType->pOptParam[pOptItem->Sel].pData);
            break;

        // The default is the option count.
        default:
            ERR(ERRORTEXT("GetOptionDisplayNameFromOptItem() OPTTYPE type %d num of OPTPARAMs not handled.\r\n"),
                            pOptItem->pOptType->Type);

            goto Exit;
            break;
    }


Exit:

    return pszOption;
}

// Refreshes option selection for each feature OPTITEM
HRESULT RefreshOptItemSelection(CUIHelper *pHelper,
                                POEMUIOBJ poemuiobj,
                                POPTITEM pOptItems,
                                WORD wItems)
{
    HRESULT             hrResult    = S_OK;
    PFEATUREOPTITEMDATA pData       = NULL;


    // Walk OPTITEM array refreshing feature OPTITEMs.
    for(WORD wCount = 0; wCount < wItems; ++wCount)
    {
        // Just go to next item if this OPTITEM isn't a feature OPTITEM.
        if(!IsFeatureOptitem(pOptItems + wCount))
        {
            continue;
        }

        // For convienience, assign to pointer to feature OPTITEM data.
        pData = (PFEATUREOPTITEMDATA)(pOptItems[wCount].UserData);

        // Refresh COption selection.
        pData->pOptions->RefreshSelection(*pHelper, poemuiobj);

        // Assign COption selection to OPTITEM selection.
        pOptItems[wCount].pSel = pData->pOptions->GetSelection();

    }

    return hrResult;
}

// Creates array of conflict features.
HRESULT GetFirstConflictingFeature(_In_ HANDLE hHeap,
                                   CUIHelper *pHelper,
                                   POEMUIOBJ poemuiobj,
                                   POPTITEM pOptItem,
                                   WORD wItems,
                                   PCONFLICT pConflict)
{
    WORD                wCount      = 0;
    HRESULT             hrResult    = S_OK;
    PFEATUREOPTITEMDATA pData       = NULL;


    // Walk OPTITEM array looking or changed options that are in conflict.
    for(wCount = 0; wCount < wItems; ++wCount)
    {
        // Just go to next item if this OPTITEM hasn't
        // changed or isn't a feature OPTITEM.
        if( !(OPTIF_CHANGEONCE & pOptItem[wCount].Flags)
            ||
            !IsFeatureOptitem(pOptItem + wCount)
          )
        {
            continue;
        }

        // For convienience, assign to pointer to feature OPTITEM data.
        pData = (PFEATUREOPTITEMDATA)(pOptItem[wCount].UserData);

        // Init conflict record if this feature is in conflict.
        pConflict->pszOptionKeyword = GetOptionKeywordFromOptItem(hHeap, pOptItem + wCount);
        if(NULL != pConflict->pszOptionKeyword)
        {
            // Get reason for conflict
            // If the feature isn't in conflict,
            // then the pmszReasonList will start
            // with NULL terminator.
            hrResult = GetWhyConstrained(hHeap,
                                         pHelper,
                                         poemuiobj,
                                         pData->pszFeatureKeyword,
                                         pConflict->pszOptionKeyword,
                                         &pConflict->pmszReasons,
                                         &pConflict->dwReasonsSize);
            if(!SUCCEEDED(hrResult))
            {
                // NOTE: driver features aren't supported by WhyConstrained.
                if((E_INVALIDARG == hrResult) && IS_DRIVER_FEATURE(pData->pszFeatureKeyword))
                {
                    // Need to reset the result in case it is the last
                    // feature OPTITEM.
                    hrResult = S_OK;
                    continue;
                }

                ERR(ERRORTEXT("GetConflictingFeatures() failed to get reason for feature %hs option %hs constraint. (hrResult = 0x%x)\r\n"),
                               pData->pszFeatureKeyword,
                               pConflict->pszOptionKeyword,
                               hrResult);

                goto Exit;
            }

            // Record conflict if feature is in conflict.
            if( (NULL != pConflict->pmszReasons)
                &&
                (pConflict->pmszReasons[0] != '\0')
              )
            {
                // Save pointer to feature keyword.
                pConflict->pszFeatureKeyword = pData->pszFeatureKeyword;
                pConflict->pszFeature        = pOptItem[wCount].pName;
                pConflict->pszOption         = GetOptionDisplayNameFromOptItem(hHeap, pOptItem + wCount);

                // Found first conflict.
                break;
            }
        }
    }


Exit:

    return hrResult;
}



