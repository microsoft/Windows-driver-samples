//+--------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2005  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    Features.cpp
//
//  PURPOSE:    Implements wrapper classes for managing the features &
//              options supported by the driver.
//
//--------------------------------------------------------------------------

#include "precomp.h"

// This indicates to Prefast that this is a usermode driver file.
_Analysis_mode_(_Analysis_code_type_user_driver_);

////////////////////////////////////////////////////////
//      Internal Defines and Macros
////////////////////////////////////////////////////////

//
// TAG the identifies feature OPTITEM data stuct.
//
#define FEATURE_OPTITEM_TAG     'FETR'


////////////////////////////////////////////////////////
//      Type Definitions
////////////////////////////////////////////////////////
//
// Struct used to identify OPTITEM as feature OPTITEM and to map back
// from an OPTITEM to the feature.
//
typedef struct _tagFeatureOptItemData
{
    DWORD       dwSize;
    DWORD       dwTag;
    PCSTR       pszFeatureKeyword;
    CFeature    *pFeature;

} FEATUREOPTITEMDATA, *PFEATUREOPTITEMDATA;

//
// Mapping of feature names to keywords
//
static CONST KEYWORDMAP gkmFeatureMap[] =
{
    {"RESDLL", NULL, IDS_RESDLL, OEMCUIP_PRNPROP},
    {"InputBin", TEXT("COMPSTUI.DLL"), IDS_CPSUI_SOURCE, OEMCUIP_DOCPROP},
    {"Orientation", TEXT("COMPSTUI.DLL"), IDS_CPSUI_ORIENTATION, OEMCUIP_DOCPROP},
    {"Resolution", TEXT("COMPSTUI.DLL"), IDS_CPSUI_RESOLUTION, OEMCUIP_DOCPROP},
    {"GraphicsMode", NULL, IDS_GRAPHICS_MODE, OEMCUIP_DOCPROP},
    {"PaperSize", TEXT("COMPSTUI.DLL"), IDS_CPSUI_FORMNAME, OEMCUIP_DOCPROP},
    {"MediaType", TEXT("COMPSTUI.DLL"), IDS_CPSUI_MEDIA, OEMCUIP_DOCPROP},
    {"ColorMode", TEXT("COMPSTUI.DLL"), IDS_CPSUI_COLOR_APPERANCE, OEMCUIP_DOCPROP},
    {"Halftone", TEXT("COMPSTUI.DLL"), IDS_CPSUI_HALFTONE, OEMCUIP_DOCPROP},
    {"DuplexUnit", NULL, IDS_DUPLEX_UNIT, OEMCUIP_PRNPROP},
    {"Duplex", TEXT("COMPSTUI.DLL"), IDS_CPSUI_DUPLEX, OEMCUIP_DOCPROP},
    {"Memory", NULL, IDS_INSTALLED_MEMORY, OEMCUIP_PRNPROP},
    {"OutputBin", TEXT("COMPSTUI.DLL"), IDS_CPSUI_OUTPUTBIN, OEMCUIP_DOCPROP},
    {"PageProtect", TEXT("COMPSTUI.DLL"), IDS_CPSUI_PAGEPROTECT, OEMCUIP_PRNPROP},
    {"%PagePerSheet", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NUP, OEMCUIP_DOCPROP},
    {"%PageOrder", TEXT("COMPSTUI.DLL"), IDS_CPSUI_PAGEORDER, OEMCUIP_DOCPROP},
    {"%PagesPerSheetOrdering", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NUP_DIRECTION, OEMCUIP_DOCPROP},
    {"%PagesPerSheetBorder", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NUP_BORDER, OEMCUIP_DOCPROP},
    {"%TextAsGraphics", NULL, IDS_TEXT_ASGRX, OEMCUIP_DOCPROP},
    {"%MetafileSpooling", NULL, IDS_METAFILE_SPOOLING, OEMCUIP_DOCPROP}
};

//
// Number of elements in the feature map table.
//
static const NUM_FEATURE_MAP    = (sizeof(gkmFeatureMap)/sizeof(gkmFeatureMap[0]));

//
// Mapping of option names to resources.  A more complete driver would parse
// this mapping from the GDL file, rather than use hard-coded mappings.
//
static CONST KEYWORDMAP gkmOptionMap[] =
{
    {"ON", TEXT("COMPSTUI.DLL"), IDS_CPSUI_ON, 0},
    {"OFF", TEXT("COMPSTUI.DLL"), IDS_CPSUI_OFF, 0},
    {"NONE", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NONE, 0},
    {"None", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NONE, 0},
    {"True", TEXT("COMPSTUI.DLL"), IDS_CPSUI_TRUE, 0},
    {"False", TEXT("COMPSTUI.DLL"), IDS_CPSUI_FALSE, 0,},
    {"PORTRAIT", TEXT("COMPSTUI.DLL"), IDS_CPSUI_PORTRAIT, 0},
    {"LANDSCAPE", TEXT("COMPSTUI.DLL"), IDS_CPSUI_LANDSCAPE, 0},
    {"LANDSCAPE_CC90", TEXT("COMPSTUI.DLL"), IDS_CPSUI_ROT_LAND, 0},
    {"FORMSOURCE", TEXT("COMPSTUI.DLL"), IDS_CPSUI_FORMSOURCE, 0},
    {"UPPER", TEXT("COMPSTUI.DLL"), IDS_CPSUI_UPPER_TRAY, 0},
    {"AUTO", TEXT("COMPSTUI.DLL"), IDS_CPSUI_PRINTFLDSETTING, 0},
    {"Auto", TEXT("COMPSTUI.DLL"), IDS_CPSUI_PRINTFLDSETTING, 0},
    {"PLAIN", TEXT("COMPSTUI.DLL"), IDS_CPSUI_STANDARD, 0},
    {"TRANSPARENCY", TEXT("COMPSTUI.DLL"), IDS_CPSUI_TRANSPARENCY, 0},
    {"HORIZONTAL", TEXT("COMPSTUI.DLL"), IDS_CPSUI_HORIZONTAL, 0},
    {"VERTICAL", TEXT("COMPSTUI.DLL"), IDS_CPSUI_VERTICAL, 0},
    {"FrontToBack", TEXT("COMPSTUI.DLL"), IDS_CPSUI_FRONTTOBACK, 0},
    {"BackToFront", TEXT("COMPSTUI.DLL"), IDS_CPSUI_BACKTOFRONT, 0},
    {"1", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NUP_NORMAL, 0},
    {"2", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NUP_TWOUP, 0},
    {"4", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NUP_FOURUP, 0},
    {"6", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NUP_SIXUP, 0},
    {"9", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NUP_NINEUP, 0},
    {"16", TEXT("COMPSTUI.DLL"), IDS_CPSUI_NUP_SIXTEENUP, 0},
    {"Booklet", TEXT("COMPSTUI.DLL"), IDS_CPSUI_BOOKLET, 0},
    {"RightThenDown", TEXT("COMPSTUI.DLL"), IDS_CPSUI_RIGHT_THEN_DOWN, 0},
    {"DownThenRight", TEXT("COMPSTUI.DLL"), IDS_CPSUI_DOWN_THEN_RIGHT, 0},
    {"LeftThenDown", TEXT("COMPSTUI.DLL"), IDS_CPSUI_LEFT_THEN_DOWN, 0},
    {"DownThenLeft", TEXT("COMPSTUI.DLL"), IDS_CPSUI_DOWN_THEN_LEFT, 0},
    {"UniresDLL", NULL, IDS_UNIRESDLL, 0},
    {"600dpi", NULL, IDS_RESOLUTION_600_DPI, 0},
    {"300dpi", NULL, IDS_RESOLUTION_300_DPI, 0},
    {"150dpi", NULL, IDS_RESOLUTION_150_DPI, 0},
    {"RASTERMODE", NULL, IDS_RASTERMODE, 0},
    {"HPGL2MODE", NULL, IDS_HPGL2MODE, 0},
    {"LETTER", NULL, IDS_PAPERSIZE_LETTER, 0},
    {"LEGAL", NULL, IDS_PAPERSIZE_LEGAL, 0},
    {"EXECUTIVE", NULL, IDS_PAPERSIZE_EXECUTIVE, 0},
    {"A4", NULL, IDS_PAPERSIZE_A4, 0},
    {"B5", NULL, IDS_PAPERSIZE_B5, 0},
    {"Mono", NULL, IDS_COLORMODE_MONO, 0},
    {"Color", TEXT("COMPSTUI.DLL"), IDS_CPSUI_COLOR_APPERANCE, 0},
    {"24bpp", NULL, IDS_COLORMODE_24BPP, 0},
    {"8bpp", NULL, IDS_COLORMODE_8BPP, 0},
    {"HT_PATSIZE_AUTO", NULL, IDS_HALFTONE_AUTO, 0},
    {"HT_PATSIZE_SUPERCELL_M", NULL, IDS_HALFTONE_SUPERCELL_M, 0},
    {"HT_PATSIZE_6x6_M", NULL, IDS_HALFTONE_6x6_M, 0},
    {"HT_PATSIZE_8x8_M", NULL, IDS_HALFTONE_8x8_M, 0},
    {"Option1", NULL, IDS_OUTBIN_DEFAULT, 0},
    {"Option14", NULL, IDS_OUTBIN_STACKER, 0},
    {"EnvTray", NULL, IDS_UIREP_ENVTRAY, 0},
    {"Automatic", NULL, IDS_TTDL_DEFAULT, 0},
    {"Outline", NULL, IDS_TTDL_TYPE1, 0},
    {"Bitmap", NULL, IDS_TTDL_TYPE3, 0},
    {"NativeTrueType", NULL, IDS_TTDL_TYPE42, 0},
    {"16384KB", NULL, IDS_MEMORY_STD, 0},
    {"32768KB", NULL, IDS_MEMORY_UPGRADE, 0}
};

//
// Number of elements in the option map table.
//
static const NUM_OPTION_MAP     = (sizeof(gkmOptionMap)/sizeof(gkmOptionMap[0]));

//
// Fall-back mapping for any feature or option that doesn't
// have a pre-defined display name.
//
KEYWORDMAP gDefaultMapping = {"", NULL, IDS_MISSINGNAME, OEMCUIP_DOCPROP};


//+---------------------------------------------------------------------------
//
//  Member:
//      COption::Acquire
//
//  Synopsis:
//      Load the option information as described in the mapping.
//
//
//----------------------------------------------------------------------------
HRESULT
COption::Acquire(
    HANDLE hHeap,
    PCKEYWORDMAP pMapping
    )
{
    m_pMapping = pMapping;
    m_hHeap = hHeap;

    return GetDisplayNameFromMapping(hHeap, m_pMapping, &m_pszDisplayName);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeaturePairs::ClearPair(
//
//  Synopsis:
//      Remove a feature-option pair entry from the set of feature / option
//      pairs.
//
//
//----------------------------------------------------------------------------
VOID
CFeaturePairs::ClearPair(
    size_t iIndex
        // Index of the feature-option pair to be removed.
    )
{
    if (m_pData[iIndex].pszFeature)
    {
        free((VOID*)m_pData[iIndex].pszFeature);
        m_pData[iIndex].pszFeature = NULL;
    }

    if (m_pData[iIndex].pszOption)
    {
        free((VOID*)m_pData[iIndex].pszOption);
        m_pData[iIndex].pszOption = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeaturePairs Constructor
//
//  Synopsis:
//      Create a feature-pairs object containing cElements entries, and
//      initialize them all to NULL.
//
//
//----------------------------------------------------------------------------
CFeaturePairs::CFeaturePairs(
    size_t cElements
        // number of elements to remove.
    )
{
    m_pData = NULL;
    size_t cbFeatureOptions = 0;
    m_cElements = 0;

    if (SUCCEEDED(SizeTMult(cElements, sizeof(PRINT_FEATURE_OPTION), &cbFeatureOptions)))
    {
        m_pData = new PRINT_FEATURE_OPTION[cElements];

        if (m_pData)
        {
            m_cElements = cElements;
            memset(m_pData, 0, cbFeatureOptions);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeaturePairs destructor
//
//  Synopsis:
//      Release any memory used by this object
//
//
//----------------------------------------------------------------------------
CFeaturePairs::~CFeaturePairs()
{
    if (m_pData)
    {
        for (size_t iPair = 0; iPair < m_cElements; iPair++)
        {
            ClearPair(iPair);
        }

        delete m_pData;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeaturePairs::SetPair
//
//  Synopsis:
//      Set the pair at the specified index to the specified value.  Each
//      pair represents a feature and a currently selected option for that
//      feature.
//
//  Returns:
//      S_OK
//      E_OUTOFMEMORY
//      E_INVALIDARG if the index is out of of bounds.
//
//
//----------------------------------------------------------------------------
HRESULT
CFeaturePairs::SetPair(
    size_t iIndex,
        // Index of the entry to set.
    PCSTR pcszFeature,
        // The name of the feature.  This doesn't
        // hold a pointer to the string passed in.
    PCSTR pcszOption
        // The name of the option.  This doesn't
        // hold a pointer to the string passed in.
    )
{

    if (iIndex > m_cElements || !pcszFeature || !pcszOption)
    {
        return E_INVALIDARG;
    }

    //
    // Deferred error result from failed constructor...
    //
    if (!m_pData)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Configure the data elements
    //
    m_pData[iIndex].pszFeature = new CHAR[strlen(pcszFeature)+1];
    m_pData[iIndex].pszOption =  new CHAR[strlen(pcszOption)+1];

    if (m_pData[iIndex].pszFeature && m_pData[iIndex].pszOption)
    {
        memcpy((PSTR)m_pData[iIndex].pszFeature, pcszFeature, (strlen(pcszFeature)+1));
        memcpy((PSTR)m_pData[iIndex].pszOption, pcszOption, (strlen(pcszOption)+1));
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CFeature::Clear
//
//  Synopsis:
//      Reset the option collection information to it's state before otions
//      were acquired.
//
//
//----------------------------------------------------------------------------

VOID
CFeature::Clear()
{
    if (m_pOptions != NULL)
    {
        delete [] m_pOptions;
        m_pOptions = NULL;
    }

    if (m_pszDisplayName != NULL && !IS_INTRESOURCE(m_pszDisplayName))
    {
        HeapFree(m_hHeap, 0, m_pszDisplayName);
    }

    m_dwOptions       = 0;
    m_Sel             = 0;
    m_pszFeature      = NULL;
    m_hHeap           = NULL;
    m_pOptions        = NULL;
    m_pMapping        = &gDefaultMapping;
    m_pszDisplayName  = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeature (constructor)
//
//  Synopsis:
//      Initialize the option collection to a default state.  Does
//      not acquire the options from the GPD / helper.
//
//
//----------------------------------------------------------------------------
CFeature::CFeature() :
    m_dwOptions(0),
    m_Sel(0),
    m_pszFeature(NULL),
    m_hHeap(NULL),
    m_pOptions(NULL),
    m_pMapping(&gDefaultMapping),
    m_pszDisplayName(NULL)
{
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeature (destructor)
//
//  Synopsis:
//      Clean up any memory we're holding.
//
//
//----------------------------------------------------------------------------
CFeature::~CFeature()
{
    Clear();
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeature::Acquire
//
//  Synopsis:
//      Populate the options in the collection using the
//      supplied helper to acquire options for the specified
//      feature.
//
//  Returns:
//      S_OK on success, else E_*
//
//
//----------------------------------------------------------------------------
HRESULT
CFeature::Acquire(
    _In_ HANDLE hHeap,
        // The heap to make allocations from
    _In_ IPrintCoreHelper* pHelper,
        // Pointer to the Unidrv supplied helper object
    _In_ PCSTR pszFeature
        // The name of the feature to read settings for.
    )
{
    VERBOSE(DLLTEXT("CFeature::Acquire entry."));

    HRESULT hrResult = S_OK;
    PCSTR *ppszOptions = NULL;

    //
    // Don't retreive the Options again if we already got them.
    //
    if ((m_dwOptions > 0) &&
        (m_pszFeature != NULL) &&
        !lstrcmpA(m_pszFeature, pszFeature))
    {
        WARNING(DLLTEXT("CFeature::Acquire: Already have options for feature %hs. Returning S_OK.\r\n"), m_pszFeature);
        return S_OK;
    }

    //
    // Save the heap handle for use later, such as freeing memory at destruction.
    //
    m_hHeap = hHeap;

    //
    // Store Keyword string.
    //
    m_pszFeature = pszFeature;

    //
    // Get or build a keyword mapping entry that
    // maps from keyword to info such as display name &
    // whether the feature is a device setting or document
    // setting.
    //
    m_pMapping = GetKeywordMapping(gkmFeatureMap, NUM_FEATURE_MAP, pszFeature);

    //
    // Get the display name for this feature
    //
    hrResult = GetDisplayNameFromMapping(hHeap, m_pMapping, &m_pszDisplayName);

    if (!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("CFeature::Acquire: Failed to get display name for feature %hs with error code 0x%x!\r\n"),
            pszFeature,
            hrResult);
        goto Exit;
    }

    //
    // Enumerate Options.
    //
    hrResult = pHelper->EnumOptions(m_pszFeature,
                                    &ppszOptions,
                                    &m_dwOptions
                                    );

    //
    // Make sure we got the option list.
    // Can't do anything more with out it.
    //
    if (!SUCCEEDED(hrResult))
    {
        ERR(ERRORTEXT("CFeature::Acquire: Failed to enumerate options for feature %hs. (hrResult = 0x%x)\r\n"), m_pszFeature, hrResult);
        goto Exit;
    }

    //
    // Build COption
    //
    m_pOptions = new COption[m_dwOptions];

    if (m_pOptions == NULL)
    {
        ERR(ERRORTEXT("COption::Acquire: Failed to allocate memory for option info array!\r\n"));
        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // For each option, build and/or get useful info like display name etc.
    //
    for (DWORD dwIndex = 0; dwIndex < m_dwOptions; dwIndex++)
    {
        PCKEYWORDMAP pMapping =
            GetKeywordMapping(gkmOptionMap, NUM_OPTION_MAP, ppszOptions[dwIndex]);

        //
        // Get or build a keyword mapping entry that
        // maps from keyword to info such as display name,
        // icon, option type etc. which we can't get from
        // the helper interface
        //
        hrResult = m_pOptions[dwIndex].Acquire(m_hHeap,
                                               pMapping);

        if (!SUCCEEDED(hrResult))
        {
            ERR(ERRORTEXT("CFeature::Acquire: Failed to load information for option %hs of feature %hs with error code 0x%x!\r\n"),
                        ppszOptions[dwIndex],
                        m_pszFeature,
                        hrResult);
            goto Exit;
        }
    }

    //
    // Get current option selection.
    //
    if(SUCCEEDED(hrResult) && m_dwOptions > 0)
    {
        hrResult = RefreshSelection(pHelper);
    }

Exit:

    //
    // Cleanup if weren't successful
    //
    if (!SUCCEEDED(hrResult))
    {
        Clear();
    }

    return hrResult;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeature::GetOption
//
//  Synopsis:
//      Get a reference to the description of the option at the specified index.
//
//  Returns:
//      A pointer to the requested option if available, otherwise NULL.
//
//
//----------------------------------------------------------------------------
CONST COption *
CFeature::GetOption(
    DWORD dwIndex
        // Index of the option for which the display name is requested.
    ) const
{
    VERBOSE(DLLTEXT("CFeature::GetOption entry."));

    if ((dwIndex >= m_dwOptions) || (m_pOptions == NULL))
    {
        ERR(ERRORTEXT("CFeature::GetOption: Invalid parameters!\r\n"));
        return NULL;
    }

    return &(m_pOptions[dwIndex]);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeature::FindOption
//
//  Synopsis:
//      Find the index of the option with the specified keyword.  If no such
//      option exists, this routine returns the specified result instead.
//
//
//----------------------------------------------------------------------------
_Success_(return != FALSE) BOOL
CFeature::FindOption(
    _In_ PCSTR pszOption,
        // The name of the option to look for
    _Out_ DWORD *pdwOption
        // The index of the default option to use if the name isn't found
    ) const
{
    VERBOSE(DLLTEXT("CFeature::FindOption entry."));

    BOOL bFound = FALSE;

    //
    // Validate parameters
    //
    if (pszOption == NULL ||
        pdwOption == NULL)
    {
        return FALSE;
    }

    //
    // Walk the option keyword looking for a match.
    //
    for(DWORD dwIndex = 0; !bFound && (dwIndex < m_dwOptions); dwIndex++)
    {
        bFound = !lstrcmpA(pszOption, m_pOptions[dwIndex].GetKeyword());

        if(bFound)
        {
            *pdwOption = dwIndex;
        }
    }

    return bFound;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeature::InitOptItem
//
//  Synopsis:
//      Configure the provided OPTITEM with the options in this collection.
//
//  Returns:
//
//  Notes:
//
//
//----------------------------------------------------------------------------
HRESULT
CFeature::InitOptItem(
    _In_ HANDLE hHeap,
        // The heap to allocate memory from.
    _Inout_ POPTITEM pOptItem
        // The optitem struct being initialized.
    )
{
    VERBOSE(DLLTEXT("CFeature::InitOptItem entry."));

    HRESULT hrResult = S_OK;

    //
    // Add feature OPTITEM data to OPTITEM to facilitate saving
    // selection changes
    //
    PFEATUREOPTITEMDATA pData =
        (PFEATUREOPTITEMDATA)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(FEATUREOPTITEMDATA));

    if (pData == NULL)
    {
        ERR(ERRORTEXT("CFeatureCollection::InitOptItem: Failed to allocated memory for feature OPTITEM data."));
        hrResult = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Configure the callback data structure
    //
    pData->dwSize = sizeof(FEATUREOPTITEMDATA);
    pData->dwTag = FEATURE_OPTITEM_TAG;
    pData->pszFeatureKeyword = this->GetKeyword();
    pData->pFeature = this;

    //
    // Configure the optitem
    //
    pOptItem->UserData = (ULONG_PTR)pData;
    pOptItem->pName = const_cast<PWSTR>(this->GetDisplayName());
    pOptItem->Sel = m_Sel;

    //
    // If there's only 1 option, mark the feature as disabled.
    //
    if (GetCount() > 1)
    {
        pOptItem->Flags = 0;
    }
    else
    {
        pOptItem->Flags |= OPTIF_DISABLED;
    }

    //
    // Only create options if there are options defined
    // for this feature.
    //
    if (GetCount() > 0)
    {
        //
        // Allocate memory for feature options.
        //
        pOptItem->pOptType = CreateOptType(hHeap, (WORD)GetCount());

        if (pOptItem->pOptType == NULL)
        {
            ERR(ERRORTEXT("CFeature::InitOptItem: Failed to allocate OPTTYPEs for OPTITEM %hs!\r\n"),
                        this->GetKeyword());
            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }

        //
        // Set OPTTYPE type & display string for each OPTPARAM in the ot
        //
        pOptItem->pOptType->Type = TVOT_COMBOBOX;

        for (DWORD dwIndex = 0; dwIndex < GetCount(); dwIndex++)
        {
            pOptItem->pOptType->pOptParam[dwIndex].pData = const_cast<PWSTR>(this->GetOption(dwIndex)->GetDisplayName());
        }
    }

Exit:

    return hrResult;

}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeature::RefreshSelection
//
//  Synopsis:
//      Synchronize the currently selected option with the option
//      that Unidrvui recognizes as being selected.
//
//  Returns:
//      S_OK on success, else E_*
//
//
//----------------------------------------------------------------------------
HRESULT
CFeature::RefreshSelection(
    _In_ IPrintCoreHelper *pHelper
        // Pointer to the Unidrv supplied helper object
    )
{
    VERBOSE(DLLTEXT("CFeature::RefreshSelection entry."));

    PCSTR pszSel = NULL;
    HRESULT hrResult = S_OK;

    //
    // Get current option selection.
    //
    hrResult = pHelper->GetOption(NULL, 0, m_pszFeature, &pszSel);

    if(SUCCEEDED(hrResult))
    {
        if (!FindOption(pszSel, &m_Sel))
        {
            //
            // The option name should always match something in the option
            // array.  If for some reason that's not the case, default
            // back to option 0.
            //
            WARNING(ERRORTEXT("CFeature::RefreshSelection: Failed to find selected option!\r\n"));
            m_Sel = 0;
        }
    }
    else if(hrResult == E_FAIL)
    {
        //
        // GetOption fails with E_FAIL when trying to read document-
        // sticky options if the printer preferences is being shown.
        // Hide the error & default the selection.  The feature won't
        // be shown in the UI.
        //
        m_Sel = 0;
        hrResult = S_OK;
    }

    return hrResult;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CFeatureCollection::Clear
//
//  Synopsis:
//      Release any memory associated with this feature collection and reset
//      values to defaults.
//
//
//----------------------------------------------------------------------------
VOID
CFeatureCollection::Clear()
{
    //
    // feature info array allocated with new so
    // that each of the constructors for CFeature
    // will be called.
    //
    if (m_pFeatures)
    {
        delete [] m_pFeatures;
    }

    //
    // Re-initialize
    //
    m_dwFeatures        = 0;
    m_dwDocFeatures     = 0;
    m_dwPrintFeatures   = 0;
    m_hHeap             = NULL;
    m_pFeatures         = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CFeatureCollection::GetModelessIndex
//
//  Synopsis:
//      Get the absolute index of the feature, assuming the index provided is
//      for the specified mode.  This if there are three features,
//
//          DocFeature1
//          DocFeature2
//          PrinterFeature1
//
//      If this routine is called with index 0, and mode OEMCUIP_PRNPROP,
//      it will return the absolute index of the first printer-sticky feature,
//      which in this case would be 2 (remember, the first index is 0).
//
//
//----------------------------------------------------------------------------
DWORD
CFeatureCollection::GetModelessIndex(
    DWORD dwIndex,
        // The index relative to the specified mode.
    DWORD dwMode
        // The mode for which the index applies.  See synopsis above for
        // a more detailed explanation of mode.
    ) const
{
    DWORD dwCount = 0;

    switch(dwMode)
    {
        //
        // Number of features, all modes
        //
        case 0:
            dwCount = dwIndex;
            break;

        case OEMCUIP_DOCPROP:
        case OEMCUIP_PRNPROP:
            //
            // Walk the feature list looking for the nth feature
            // with matching mode
            //
            for (dwCount = 0; dwCount < m_dwFeatures; dwCount++)
            {
                //
                // Count down to the feature we want
                // Only countdown for matching modes
                //
                if (dwMode == m_pFeatures[dwCount].GetMode())
                {
                    if (dwIndex == 0)
                    {
                        break;
                    }
                    else
                    {
                        --dwIndex;
                    }
                }
            }
            break;
    }

    return dwCount;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeatureCollection (constructor)
//
//  Synopsis:
//
//
//----------------------------------------------------------------------------
CFeatureCollection::CFeatureCollection() :
    m_dwFeatures(0),
    m_dwDocFeatures(0),
    m_dwPrintFeatures(0),
    m_hHeap(NULL),
    m_pFeatures(NULL)
{
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeatureCollection (destructor)
//
//  Synopsis:
//      Release all resources held by this object
//
//
//----------------------------------------------------------------------------
CFeatureCollection::~CFeatureCollection()
{
    //
    // Clean up class.
    //
    Clear();
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeatureCollection::Acquire
//
//  Synopsis:
//      Load all of the features supported by unidrv as well as those specified
//      in the GPD using the provided helper object
//
//  Returns:
//      S_OK on success else E_*
//
//
//----------------------------------------------------------------------------
HRESULT
CFeatureCollection::Acquire(
    HANDLE hHeap,
        // The heap from which any allocations should be made
    IPrintCoreHelper *pHelper
        // The helper object that provides access to the core driver features.
    )
{
    VERBOSE(DLLTEXT("CFeatureCollection::Acquire entry."));

    HRESULT hrResult = S_OK;

    PCSTR *ppszKeywords = NULL;
        // String list that points to each feature keyword

    //
    // Don't retreive the Features again if we already got them.
    //
    if (m_dwFeatures > 0)
    {
        WARNING(DLLTEXT("CFeatureCollection::Acquire: Features have already been enumerated. Returning S_OK.\r\n"));
        return S_OK;
    }

    // Save the heap handle for use later, such as freeing memory at destruction.
    m_hHeap = hHeap;

    // Enumerate features.
    hrResult = pHelper->EnumFeatures(&ppszKeywords, &m_dwFeatures);

    if (FAILED(hrResult))
    {
        ERR(ERRORTEXT("CFeatureCollection::Acquire() failed to enumerate features. (hrResult = 0x%x)\r\n"), hrResult);
    }
    else
    {
        //
        // Build Feature information
        //
        m_pFeatures = new CFeature[m_dwFeatures];

        if (m_pFeatures == NULL)
        {
            ERR(ERRORTEXT("CFeatureCollection::Acquire: Failed to allocate feature info array!\r\n"));
            hrResult = E_OUTOFMEMORY;
        }

        //
        // For each feature, build/get feature info
        //
        for (DWORD dwIndex = 0; SUCCEEDED(hrResult) && dwIndex < m_dwFeatures ; dwIndex++)
        {
            //
            // Get options for each feature
            // Note: Some features don't have options, the HRESULT will be E_NOTIMPL for these
            //
            hrResult = m_pFeatures[dwIndex].Acquire(hHeap,
                                                    pHelper,
                                                    ppszKeywords[dwIndex]);

            if (!SUCCEEDED(hrResult) && (hrResult != E_NOTIMPL))
            {
                ERR(ERRORTEXT("CFeatureCollection::Acquire: Failed to get options for feature %hs with error code 0x%x!\r\n"),
                            ppszKeywords[dwIndex],
                            hrResult);
            }

            //
            // Keep track of mode counts
            //
            if (SUCCEEDED(hrResult))
            {
                switch(m_pFeatures[dwIndex].GetMode())
                {
                    case OEMCUIP_DOCPROP:
                        ++m_dwDocFeatures;
                        break;

                    case OEMCUIP_PRNPROP:
                        ++m_dwPrintFeatures;
                        break;

                    default:
                        ERR(ERRORTEXT("CFeatureCollection::Acquire: Unknown stickiness for feature %hs!\r\n"),
                            ppszKeywords[dwIndex]);
                        break;
                }
            }
        }
    }

    //
    // Clean up if we weren't successful
    //
    if (FAILED(hrResult))
    {
        Clear();
    }

    return hrResult;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeatureCollection::GetCount
//
//  Synopsis:
//      Get the number of features in the collection.
//
//
//----------------------------------------------------------------------------
DWORD
CFeatureCollection::GetCount(
    DWORD dwMode
        // Should this routine count the doc-sticky features, printer sticky
        // features, or both?
    ) CONST
{
    DWORD dwCount = 0;

    switch(dwMode)
    {
        case 0:
            dwCount = m_dwFeatures;
            break;

        case OEMCUIP_DOCPROP:
            dwCount = m_dwDocFeatures;
            break;

        case OEMCUIP_PRNPROP:
            dwCount = m_dwPrintFeatures;
            break;
    }

    return dwCount;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CFeatureCollection::GetFeature
//
//  Synopsis:
//      Get the options for the feature at the specified index.
//
//
//----------------------------------------------------------------------------
CFeature*
CFeatureCollection::GetFeature(
    DWORD dwIndex,
        // The index of the feature to look up.
    DWORD dwMode
        // The mode associated with the feature index.
    ) const
{
    //
    // Validate parameters
    //
    if ((dwIndex >= GetCount(dwMode)) || (m_pFeatures == NULL))
    {
        return NULL;
    }

    //
    // Get internal index
    //
    dwIndex = GetModelessIndex(dwIndex, dwMode);

    //
    // Return options pointer
    //
    return &(m_pFeatures[dwIndex]);
}


//+---------------------------------------------------------------------------
//
//  Member:
//      ::GetDisplayNameFromMapping
//
//  Synopsis:
//      Load the string resource associated with the specified mapping entry.
//
//  Returns:
//      S_OK on success, else E_*
//
//
//----------------------------------------------------------------------------
HRESULT
GetDisplayNameFromMapping(
    _In_ HANDLE hHeap,
        // The heap to perform allocations from
    _In_ PCKEYWORDMAP pMapping,
        // The mapping element for the element.  Could be a feature or option.
    _Out_ PWSTR *ppszDisplayName
        // The location where this routine supplies the display name to the
        // caller.
    )
{
    HMODULE hModule = NULL;
    HRESULT hrResult = S_OK;

    VERBOSE(DLLTEXT("GetDisplayNameFromMapping entry."));

    if ((hHeap == NULL) || (pMapping == NULL) || (ppszDisplayName == NULL))
    {
        return E_INVALIDARG;
    }

    //
    // Check for simple case of returning INT resource
    //
    if (pMapping->pwszModule == NULL)
    {
        //
        // Just need to do MAKEINTRESOURCE on the resource ID and return
        //
        *ppszDisplayName = MAKEINTRESOURCE(pMapping->uDisplayNameID);
    }
    else
    {
        //
        // We need to get the module name if we aren't loading the
        // resource from this module (pwszModule != NULL). As an
        // optimization, we assume that the module has already been loaded,
        // since the only cases currently are this module, driver ui and compstui.dll.
        //
        hModule = GetModuleHandle(pMapping->pwszModule);

        if (hModule == NULL)
        {
            DWORD dwError = GetLastError();

            ERR(ERRORTEXT("GetDisplayNameFromMapping: Failed to load module %s with error code %d!\r\n"),
                        pMapping->pwszModule,
                        dwError);

            hrResult = HRESULT_FROM_WIN32(dwError);

            //
            // In case last error wasn't set correctly, force it to an error value.
            //
            if (SUCCEEDED(hrResult))
            {
                hrResult = E_FAIL;
            }
        }

        //
        // Get the string resource
        //
        if (SUCCEEDED(hrResult))
        {
            hrResult = GetStringResource(hHeap, hModule, pMapping->uDisplayNameID, ppszDisplayName);
            if (FAILED(hrResult))
            {
                ERR(ERRORTEXT("GetDisplayNameFromMapping: Failed to load string with error code 0x%x!\r\n"), hrResult);
            }
        }
    }

    return hrResult;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      ::GetKeywordMapping
//
//  Synopsis:
//      Find the mapping in the provided table associated with the specified
//      keyword.  This routine can be used to look up both feature & option
//      mappings.
//
//  Returns:
//      The requested mapping on success, otherwise NULL.
//
//
//----------------------------------------------------------------------------
PCKEYWORDMAP
GetKeywordMapping(
    _In_reads_(dwMapSize) PCKEYWORDMAP pKeywordMap,
        // The mapping table to search in
    DWORD dwMapSize,
        // Number of elements in the mapping table.
    PCSTR pszKeyword
        // Keyword to look for in the table.
    )
{
    BOOL bFound = FALSE;
    PCKEYWORDMAP pMapping = &gDefaultMapping;

    if ((pKeywordMap != NULL) && (pszKeyword != NULL))
    {
        //
        // Walk mapping array for matching keyword
        //
        for (DWORD dwIndex = 0; !bFound && (dwIndex < dwMapSize); dwIndex++)
        {
            bFound = !lstrcmpA(pszKeyword, pKeywordMap[dwIndex].pszKeyword);

            if (bFound)
            {
                pMapping = pKeywordMap + dwIndex;
            }
        }
    }

    return pMapping;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      ::IsFeatureOptItem
//
//  Synopsis:
//      Test if an OPTITEM is an OPTITEM for a feature.
//
//  Returns:
//      TRUE if the OPTITEM represents a feature, else FALSE.
//
//
//----------------------------------------------------------------------------
BOOL
IsFeatureOptItem(
    POPTITEM pOptItem
        // The otitem to check
    )
{
    BOOL                bRet    = FALSE;
    PFEATUREOPTITEMDATA pData   = NULL;


    // Make sure pointers are NULL.
    if( (NULL == pOptItem)
        ||
        (NULL == pOptItem->UserData)
       )
    {
        //
        // INVARIANT:  can't be feature OPTITEM, since one of
        //             the necessary pointer are NULL.
        //

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


//+---------------------------------------------------------------------------
//
//  Member:
//      ::SaveFeatureOptItems
//
//  Synopsis:
//      Walks array of OPTITEMs saving each feature OPTITEM
//      that has changed.
//
//  Returns:
//      S_OK on success, else E_*
//
//
//----------------------------------------------------------------------------
HRESULT
SaveFeatureOptItems(
    HANDLE hHeap,
        // The heap to perform allocations from
    IPrintCoreHelper* pHelper,
        // Pointer to the Unidrv supplied helper object
    HWND hWnd,
        // The current window object.  Used as the parent of any model dialogs /
        // message boxes displayed from this routine.
    _In_reads_(dwItems) POPTITEM pOptItem,
        // The option items to save
    DWORD dwItems
        // The number of optitems in the array
    )
{
    VERBOSE(DLLTEXT("SaveFeatureOptItems entry."));

    DWORD dwPairsWritten = 0;
    DWORD dwResult = 0;
    HRESULT hrResult = S_OK;
    CFeaturePairs *pPairs = NULL;

    //
    // Validate parameters
    //
    if (!(hHeap && pHelper && pOptItem))
    {
        ERR(ERRORTEXT("SaveFeatureOptItems: Invalid arguments!\r\n"));
        return E_INVALIDARG;
    }

    //
    // Get feature option pairs to save
    //
    hrResult = GetChangedFeatureOptions(pOptItem, dwItems, &pPairs);

    if(FAILED(hrResult))
    {
        ERR(ERRORTEXT("SaveFeatureOptItems: Failed to get changed feature option pairs! (error code 0x%x)\r\n"),
                    hrResult);
    }

    //
    // Only update options if something changed.
    //
    if (SUCCEEDED(hrResult) &&
        pPairs &&
        pPairs->GetElementCount() > 0)
    {

        //
        // Set the change feature options.
        // For the first SetOptions() call, don't have the
        // core driver UI resolve conflicts, so we can
        // prompt user for automatic resolution or let
        // them do the conflict resolving.
        //
        hrResult = pHelper->SetOptions(NULL,
                                       0,
                                       FALSE,
                                       pPairs->GetData(),
                                       (DWORD)pPairs->GetElementCount(),
                                       &dwPairsWritten,
                                       &dwResult);

        if(FAILED(hrResult))
        {
            ERR(ERRORTEXT("SaveFeatureOptItems: SetOptions failed with dwResult = %d! (hrResult = 0x%x)\r\n"),
                           dwResult,
                           hrResult);
        }
        else if(SETOPTIONS_RESULT_CONFLICT_REMAINED == dwResult)
        {
            //
            // SetOptions succeeded, but there was a conflict in the selected
            // options that was not resolved.  Ask the user if they'd like the
            // plug-in to resolve the conflict for them.
            //
            int nRet;
            CONST PRINT_FEATURE_OPTION *pConflicts;
            DWORD dwConflicts;
            PWSTR pszMessage = NULL;

            hrResult = GetStringResource(hHeap, ghInstance, IDS_CONSTRAINT_CONFLICT, &pszMessage);
            if(FAILED(hrResult))
            {
                ERR(ERRORTEXT("SaveFeatureOptItems: Failed to load conflict resource string (error code 0x%x)\r\n"),
                            hrResult);
            }

            //
            // INVARIANT:  constraint conflict, options weren't saved.
            //
            // Get list of all features that have conflict.
            //
            if (SUCCEEDED(hrResult))
            {
                hrResult = GetFirstConflictingFeature(pHelper,
                                                      pOptItem,
                                                      dwItems,
                                                      &pConflicts,
                                                      &dwConflicts);

                if (SUCCEEDED(hrResult) && dwConflicts == 0)
                {
                    hrResult = E_FAIL;
                    ERR(ERRORTEXT("SaveFeatureOptItems: Driver indicated conflict, but didn't provide conflicting options!\r\n"));
                }

                if(FAILED(hrResult))
                {
                    ERR(ERRORTEXT("GetFirstConflictingFeature failed!"));
                }
            }

            if (SUCCEEDED(hrResult))
            {
                //
                // Display simple message box with prompt
                //
                nRet = MessageBox(hWnd,
                                  pszMessage,
                                  L"UI Replacement Test",
                                  MB_YESNO | MB_ICONWARNING);

                //
                // Check to see how user wants to resolve conflict
                //
                if(nRet == IDYES)
                {
                    //
                    // Let core driver resolve conflict resolution
                    //
                    hrResult = pHelper->SetOptions(NULL,
                                                   0,
                                                   TRUE,
                                                   pPairs->GetData(),
                                                   (DWORD)pPairs->GetElementCount(),
                                                   &dwPairsWritten,
                                                   &dwResult);

                    //
                    // Conflict resolution requires refreshing current option
                    // selection for each feature, since selection may have
                    // changed because of conflict resolution.
                    //
                    if (SUCCEEDED(hrResult))
                    {
                        RefreshOptItemSelection(pHelper, pOptItem, dwItems);
                    }
                }

                //
                // Return failure if there are still conflicts
                //
                if(dwResult == SETOPTIONS_RESULT_CONFLICT_REMAINED)
                {
                    ERR(ERRORTEXT("SaveFeatureOptItems: Conflicts still exist even after calling core driver to resolve them!"));
                    hrResult = E_FAIL;
                }
            }

            if(pszMessage != NULL)
            {
                HeapFree(hHeap, 0, pszMessage);
            }
        }
    }

    //
    // Clean up
    //

    if (pPairs)
    {
        delete pPairs;
    }

    return hrResult;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      GetChangedFeatureOptions
//
//  Synopsis:
//      Creates a collection of all of the features that have changed in the
//      provided collection of OPTITEMs
//
//  Returns:
//      S_OK on success, else E_*.
//
//  Notes:
//      if no settings have been changed, the routine may return S_OK
//      and *ppPairs == NULL.
//
//
//----------------------------------------------------------------------------
HRESULT
GetChangedFeatureOptions(
    _In_reads_(dwItems) POPTITEM pOptItem,
        // The array of optitems to search through
    DWORD dwItems,
        // The number of optitems in the provided array
    _Outptr_result_maybenull_ CFeaturePairs **ppPairs
        // feature / option keyword pairs describing the options that
        // have changed.
    )
{
    VERBOSE(DLLTEXT("GetChangedFeatureOptions entry."));

    DWORD dwCount = 0;
    DWORD dwChanged = 0;
    HRESULT hrResult = S_OK;
    CFeaturePairs *pPairs;

    *ppPairs = NULL;

    //
    // Walk OPTITEM array looking or changed options,
    // and calculating size needed for multi-sz buffer.
    //
    for (dwCount = 0; dwCount < dwItems; dwCount++)
    {
        //
        // Just go to next item if this OPTITEM hasn't
        // changed or isn't a feature OPTITEM
        //
        if ((OPTIF_CHANGEONCE & pOptItem[dwCount].Flags) &&
            IsFeatureOptItem(&(pOptItem[dwCount]))
            )
        {
            ++dwChanged;
        }
    }

    //
    // Don't need to do anything more if no feature options changed.
    //
    if(dwChanged != 0)
    {
        pPairs = new CFeaturePairs(dwChanged);

        if (pPairs                    == NULL ||
            pPairs->GetElementCount() == 0
           )
        {
            ERR(ERRORTEXT("GetChangedFeatureOptions: Failed to allocate feature-option array!\r\n"));
            hrResult = E_OUTOFMEMORY;

            if ( pPairs )
            {
                delete pPairs;
                pPairs = NULL;
            }

        }

        //
        // Build array of feature option pairs that changed
        //
        for (dwCount = 0;
            SUCCEEDED(hrResult) && (dwCount < dwItems) && dwChanged;
            dwCount++)
        {
            //
            // Just go to next item if this OPTITEM hasn't
            // changed or isn't a feature OPTITEM
            //
            if((OPTIF_CHANGEONCE & pOptItem[dwCount].Flags) &&
                IsFeatureOptItem(&(pOptItem[dwCount]))
                )
            {
                //
                // For convienience, assign to pointer to feature OPTITEM data
                //
                PFEATUREOPTITEMDATA pData = (PFEATUREOPTITEMDATA)(pOptItem[dwCount].UserData);

                //
                // Add feature option pair
                //
                PCSTR pszOption = GetOptionKeywordFromOptItem(&pOptItem[dwCount]);

                if (pszOption != NULL)
                {
                    --dwChanged;
                    hrResult = pPairs->SetPair(dwChanged, pData->pszFeatureKeyword, pszOption);
                }
            }
        }

        if(SUCCEEDED(hrResult))
        {
            *ppPairs = pPairs;
        }
        else if (pPairs)
        {
            delete pPairs;
        }
    }

    return hrResult;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      ::GetOptionKeywordFromOptItem
//
//  Synopsis:
//      Returns pointer to option keyword for a feature OPTITEM
//
//  Returns:
//      The requested string on success, otherwise NULL
//
//
//----------------------------------------------------------------------------
PCSTR
GetOptionKeywordFromOptItem(
    POPTITEM pOptItem
        // The opt item from which the keyword shoudl be read
    )
{
    VERBOSE(DLLTEXT("GetOptionKeywordFromOptItem entry."));

    PCSTR pszOption = NULL;

    //
    // Validate parameters
    //
    if(!IsFeatureOptItem(pOptItem))
    {
        ERR(ERRORTEXT("GetOptionKeywordFromOptItem: Invalid parameter!\r\n"));
    }
    else if(pOptItem->pOptType->Type != TVOT_COMBOBOX)
    {
        ERR(ERRORTEXT("GetOptionKeywordFromOptItem: OPTTYPE type %d number of OPTPARAMS not handled!\r\n"),
                        pOptItem->pOptType->Type);
    }
    else
    {
        //
        // Option selection is based on type of OPTTYPE
        //
        PFEATUREOPTITEMDATA pData = ((PFEATUREOPTITEMDATA)(pOptItem->UserData));
        CONST COption *pOption = pData->pFeature->GetOption((DWORD)pOptItem->Sel);

        if (pOption)
        {
            pszOption = pOption->GetKeyword();
        }
        else
        {
            ERR(ERRORTEXT("GetOptionKeywordFromOptItem: Invalid option index %d!\r\n"), (DWORD)pOptItem->Sel);
        }
    }

    return pszOption;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      ::RefreshOptItemSelection
//
//  Synopsis:
//      For each feature update the currently selected option with the value
//      that the core driver has selected.  This is useful for updating the
//      UI after resolving constraints.
//
//  Returns:
//      S_OK on success, else E_*
//
//
//----------------------------------------------------------------------------
HRESULT
RefreshOptItemSelection(
    IPrintCoreHelper *pHelper,
        // Pointer to the Unidrv supplied helper object
    _In_reads_(dwItems) POPTITEM pOptItems,
        // An array of options to update
    DWORD dwItems
        // The number of options in the provided array
    )
{
    HRESULT hrResult = S_OK;
    PFEATUREOPTITEMDATA pData = NULL;

    //
    // Walk OPTITEM array refreshing feature OPTITEMs
    //
    for (DWORD dwCount = 0; dwCount < dwItems; dwCount++)
    {
        //
        // Just go to next item if this OPTITEM isn't a feature OPTITEM
        //
        if (!IsFeatureOptItem(pOptItems + dwCount))
        {
            continue;
        }

        //
        // For convienience, assign to pointer to feature OPTITEM data
        //
        pData = (PFEATUREOPTITEMDATA)(pOptItems[dwCount].UserData);

        //
        // Refresh COption selection
        //
        pData->pFeature->RefreshSelection(pHelper);

        //
        // Assign COption selection to OPTITEM selection
        //
        pOptItems[dwCount].Sel = pData->pFeature->GetSelection();
    }

    return hrResult;
}

//+---------------------------------------------------------------------------
//
//  Member:
//      ::GetFirstConflictingFeature
//
//  Synopsis:
//      Finds the first features that it finds to be in conflict with one
//      another and supplies those features in the ppConstraints parameter
//
//  Returns:
//      S_OK on success, else E_*.  Note that this routine can succeed even
//      if no features are in conflict.  In that case, *ppConstraints will
//      be NULL.
//
//
//----------------------------------------------------------------------------
HRESULT
GetFirstConflictingFeature(
    IPrintCoreHelper *pHelper,
        // Pointer to the Unidrv supplied helper object
    _In_reads_(dwItems) POPTITEM pOptItem,
        // The array of options to look for a conflict in
    DWORD dwItems,
        // The number of options in the array
    CONST PRINT_FEATURE_OPTION **ppConstraints,
        // If any constraints are found, supplies pairs of
        // feature / option keywords to the caller indicating
        // which options cannot be used together.
    DWORD *pdwConstraints
        // The number of constrained options
    )
{
    VERBOSE(DLLTEXT("GetFirstConflictingFeature entry."));

    DWORD dwCount = 0;
    HRESULT hrResult = S_OK;
    PCSTR pszOptionKeyword = NULL;

    *ppConstraints = NULL;

    //
    // Walk OPTITEM array looking for changed options that are in conflict
    //
    for (dwCount = 0; dwCount < dwItems; dwCount++)
    {
        //
        // Just go to next item if this OPTITEM hasn't
        // changed or isn't a feature OPTITEM
        //
        if(!(OPTIF_CHANGEONCE & pOptItem[dwCount].Flags) ||
            !IsFeatureOptItem(&(pOptItem[dwCount]))
            )
        {
            continue;
        }

        PFEATUREOPTITEMDATA pData = (PFEATUREOPTITEMDATA)(pOptItem[dwCount].UserData);

        //
        // Init conflict record if this feature is in conflict
        //
        pszOptionKeyword = GetOptionKeywordFromOptItem(&pOptItem[dwCount]);

        if (pszOptionKeyword != NULL)
        {
            //
            // Get reason for conflict
            //
            hrResult = pHelper->WhyConstrained(NULL,
                                               0,
                                               pData->pszFeatureKeyword,
                                               pszOptionKeyword,
                                               ppConstraints,
                                               pdwConstraints);

            if (FAILED(hrResult))
            {
                ERR(ERRORTEXT("GetConflictingFeatures: Failed to get reason for feature %hs option %hs constraint! (hrResult = 0x%x)\r\n"),
                            pData->pszFeatureKeyword,
                            pszOptionKeyword,
                            hrResult);
                break;
            }

            if (*pdwConstraints > 0)
            {
                break;
            }
        }
    }

    return hrResult;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CreateOptType
//
//  Synopsis:
//      Allocates and initializes OptType for OptItem & OPTPARAMs for the
//      options under that struct.
//
//  Returns:
//      A newly allocated POPTYPE from the specified heap on success, else
//      NULL.
//
//
//----------------------------------------------------------------------------
POPTTYPE
CreateOptType(
    HANDLE hHeap,
        // heap to allocate the type from.
    WORD wOptParams
        // The number of options available for this feature.
    )
{
    VERBOSE(DLLTEXT("CreateOptType entry."));

    POPTTYPE    pOptType = NULL;
        // out parameter containing the allocated type struct & array of options.

    //
    // Allocate memory from the heap for the OPTTYPE; the driver will take
    // care of clean up.
    //
    pOptType = (POPTTYPE) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(OPTTYPE));

    if(NULL != pOptType)
    {
        // Initialize OPTTYPE.
        pOptType->cbSize = sizeof(OPTTYPE);
        pOptType->Count = wOptParams;

        // Allocate memory from the heap for the OPTPARAMs for the OPTTYPE.
        pOptType->pOptParam = (POPTPARAM) HeapAlloc(hHeap,
                                                    HEAP_ZERO_MEMORY,
                                                    wOptParams * sizeof(OPTPARAM));

        if(NULL != pOptType->pOptParam)
        {
            // Initialize the OPTPARAMs.
            for(WORD wCount = 0; wCount < wOptParams; wCount++)
            {
                pOptType->pOptParam[wCount].cbSize = sizeof(OPTPARAM);
            }
        }
        else
        {
            // Free allocated memory and return NULL.
            ERR(ERRORTEXT("CreateOptType() failed to allocated memory for OPTPARAMs!\r\n"));
            HeapFree(hHeap, 0, pOptType);
            pOptType = NULL;
        }
    }
    else
    {
        ERR(ERRORTEXT("CreateOptType() failed to allocated memory for OPTTYPE!\r\n"));
    }

    return pOptType;
}


//+---------------------------------------------------------------------------
//
//  Member:
//      ::GetStringResource
//
//  Synopsis:
//      Load a string from the resource section of hte specied module.
//
//  Returns:
//      S_OK on failure, else E_*
//
//
//----------------------------------------------------------------------------
HRESULT
GetStringResource(
    _In_ HANDLE hHeap,
        // Heap to allocate space for the resource from.
    _In_ HMODULE hModule,
        // Module to load the resource from
    UINT uResource,
        // resource ID
    _Out_ PWSTR *ppszString
        // out pointer to string resource loaded.
    )
{
    VERBOSE(DLLTEXT("GetStringResource entry."));

    HRESULT hr = S_OK;

    //
    // Allocate buffer for string resource from heap;
    // let the driver clean it up.
    //
    *ppszString = NULL;
    *ppszString = (PTSTR) HeapAlloc(hHeap,
                                    HEAP_ZERO_MEMORY,
                                    MAX_PATH * sizeof(TCHAR));

    if(NULL != *ppszString)
    {
        //
        // Load string resource; resize after loading so
        // as not to waste memory.
        //
        INT nResult = LoadString(hModule,
                                 uResource,
                                 *ppszString,
                                 MAX_PATH);

        if( nResult > 0)
        {
            PWSTR pszTemp = (PWSTR) HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, *ppszString, (nResult + 1) * sizeof(WCHAR));

            if (NULL != pszTemp)
            {
                *ppszString = pszTemp;
            }
            else
            {
                //
                // a realloc failure doesn't free the original string, so
                // re-alloc failing in this case is not critical.
                //
                VERBOSE(ERRORTEXT("GetStringResource() HeapReAlloc() of string retrieved failed! (Last Error was %d)\r\n"), GetLastError());
            }
        }
        else
        {
            ERR(ERRORTEXT("LoadString() returned %d! (Last Error was %d)\r\n"), nResult, GetLastError());
            ERR(ERRORTEXT("GetStringResource() failed to load string resource %d!\r\n"), uResource);
            HeapFree(hHeap, 0, *ppszString);
            *ppszString = NULL;
            hr = E_FAIL;
        }
    }
    else
    {
        ERR(ERRORTEXT("GetStringResource() failed to allocate string buffer!\r\n"));
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

