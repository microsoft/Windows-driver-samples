/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nuptprps.cpp

Abstract:

   NUp properties class implementation. The NUp properties class is
   responsible for interpreting NUp, Binding, PageMediaSize and
   PageOrientation data for the NUp filter. Binding data is required
   as the NUp filter is responsible for applying two up for booklet
   processing.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "nuptprps.h"

using XDPrintSchema::NUp::NUpData;
using XDPrintSchema::NUp::JobNUpAllDocumentsContiguously;

using XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOption;
using XDPrintSchema::NUp::PresentationDirection::LeftBottom;
using XDPrintSchema::NUp::PresentationDirection::TopLeft;
using XDPrintSchema::NUp::PresentationDirection::BottomLeft;
using XDPrintSchema::NUp::PresentationDirection::RightBottom;

using XDPrintSchema::Binding::BindingData;
using XDPrintSchema::Binding::EBindingOption;
using XDPrintSchema::Binding::None;
using XDPrintSchema::Binding::BindLeft;
using XDPrintSchema::Binding::BindRight;
using XDPrintSchema::Binding::BindTop;
using XDPrintSchema::Binding::BindBottom;
using XDPrintSchema::Binding::JobBindAllDocuments;
using XDPrintSchema::Binding::EdgeStitchLeft;
using XDPrintSchema::Binding::EdgeStitchRight;
using XDPrintSchema::Binding::EdgeStitchTop;
using XDPrintSchema::Binding::EdgeStitchBottom;

using XDPrintSchema::PageMediaSize::PageMediaSizeData;

using XDPrintSchema::PageOrientation::PageOrientationData;
using XDPrintSchema::PageOrientation::EOrientationOption;

/*++

Routine Name:

    CNUpPTProperties::CNUpPTProperties

Routine Description:

    Constructor for the CNUpPTProperties class which
    initialises members to sensible default values

Arguments:

    nupData           - Structure containing nup settings from the PrintTicket
    bindingData       - Structure containing nup binding settings from the PrintTicket
    pageMediaSizeData - Structure containing nup page media size settings from the PrintTicket
    pageOrientData    - Structure containing nup page orientation settings from the PrintTicket

Return Value:

    None

--*/
CNUpPTProperties::CNUpPTProperties(
    _In_ CONST NUpData&             nupData,
    _In_ CONST BindingData&         bindingData,
    _In_ CONST PageMediaSizeData&   pageMediaSizeData,
    _In_ CONST PageOrientationData& pageOrientData
    ) :
    m_nupData(nupData),
    m_bindingData(bindingData),
    m_pageMediaSizeData(pageMediaSizeData),
    m_pageOrientData(pageOrientData)
{
}

/*++

Routine Name:

    CNUpPTProperties::~CNUpPTProperties

Routine Description:

    Default destructor for the CNUpPTProperties class

Arguments:

    None

Return Value:

    None

--*/
CNUpPTProperties::~CNUpPTProperties()
{
}

/*++

Routine Name:

    CNUpPTProperties::GetCount

Routine Description:

    Method to get the nup count. For booklet printing this will always be set to two

Arguments:

    pcNUpPages - Integer value which will contain the nup count

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPTProperties::GetCount(
    _Out_ UINT* pcNUpPages
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pcNUpPages, E_POINTER)))
    {
        *pcNUpPages = m_nupData.cNUp;

        if (m_bindingData.bindOption != XDPrintSchema::Binding::None)
        {
            *pcNUpPages = 2;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPTProperties::GetPresentationDirection

Routine Description:

    Method to get the nup presentation direction

Arguments:

    pPresentationDirection - Enumeration value which is set to the current
                             presentation direction

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPTProperties::GetPresentationDirection(
    _Out_ ENUpDirectionOption* pPresentationDirection
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPresentationDirection, E_POINTER)))
    {
        *pPresentationDirection = m_nupData.nUpPresentDir;

        if (m_bindingData.bindOption != XDPrintSchema::Binding::None)
        {
            hr = PresentDirFromBindOption(pPresentationDirection);
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPTProperties::GetScope

Routine Description:

    Method to get the nup scope which can be either
    document wide or job wide.

Arguments:

    pNUpScope - Enumeration value which will be set to the
                current scope of nup.

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPTProperties::GetScope(
    _In_ ENUpScope* pNUpScope
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pNUpScope, E_POINTER)))
    {
        *pNUpScope = CNUpPTProperties::None;

        if (m_bindingData.bindOption != XDPrintSchema::Binding::None)
        {
            *pNUpScope = m_bindingData.bindFeature == JobBindAllDocuments ? Job : Document;
        }
        else if (m_nupData.cNUp > 1)
        {
            *pNUpScope = m_nupData.nUpFeature == JobNUpAllDocumentsContiguously ? Job : Document;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPTProperties::GetPageSize

Routine Description:

    Method to get the nup page media size

Arguments:

    pSizePage - Value which will be set to the nup page media size

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPTProperties::GetPageSize(
    _Out_ SizeF* pSizePage
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pSizePage, E_POINTER)))
    {
        //
        // Convert microns to 96th of an inch.
        //
        pSizePage->Width  = static_cast<REAL>(m_pageMediaSizeData.pageWidth)/k96thInchAsMicrons;
        pSizePage->Height = static_cast<REAL>(m_pageMediaSizeData.pageHeight)/k96thInchAsMicrons;

        if (pSizePage->Width <= 0.0f ||
            pSizePage->Height <= 0.0f)
        {
            //
            // The page media size is incorrect
            //
            ERR("Could not acquire a valid media page size\n");

            hr = E_FAIL;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPTProperties::GetPageOrientation

Routine Description:

    Method to get the nup page orientation which can be either landscape or portrait

Arguments:

    pPageOrientation - Enumeration value which will be set to either landscape or portrait

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPTProperties::GetPageOrientation(
    _In_ EOrientationOption* pPageOrientation
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPageOrientation, E_POINTER)))
    {
        *pPageOrientation = m_pageOrientData.orientation;
    }

    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPTProperties::PresentDirFromBindOption

Routine Description:

    Method to obtain a presentation direction from the current binding setting

Arguments:

    pPresentationDirection - Enumeration value which will be set to a presentation direction

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPTProperties::PresentDirFromBindOption(
    _Out_ ENUpDirectionOption* pPresentationDirection
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pPresentationDirection, E_POINTER)))
    {
        switch (m_bindingData.bindOption)
        {
            case BindRight:
            case EdgeStitchRight:
            {
                *pPresentationDirection = LeftBottom;
            }
            break;

            case BindBottom:
            case EdgeStitchBottom:
            {
                //
                // Vertical direction takes precidence over horizontal direction
                //
                *pPresentationDirection = TopLeft;
            }
            break;

            case BindTop:
            case EdgeStitchTop:
            {
                //
                // Vertical direction takes precidence over horizontal direction
                //
                *pPresentationDirection = BottomLeft;
            }
            break;

            //
            // Everything else is RightBottom
            //
            default:
            {
                *pPresentationDirection = RightBottom;
            }
            break;
        }
    }


    ERR_ON_HR(hr);
    return hr;
}

/*++

Routine Name:

    CNUpPTProperties::GetBindingOption

Routine Description:

    Method to obtain the binding direction

Arguments:

    pBindingOption - Enumeration value which will be set to a binding direction

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CNUpPTProperties::GetBindingOption(
    _Out_ EBindingOption*   pBindingOption
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pBindingOption, E_POINTER)))
    {
        *pBindingOption = m_bindingData.bindOption;
    }

    ERR_ON_HR(hr);
    return hr;
}
