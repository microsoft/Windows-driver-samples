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

   NUp properties class definition. The NUp properties class is
   responsible for holding and controling NUp properties.

--*/

#pragma once

#include "nupdata.h"
#include "bkdata.h"
#include "psizedata.h"
#include "porientdata.h"

class CNUpPTProperties
{
public:
    enum ENUpScope
    {
        None = 0,
        Job,
        Document
    };

public:
    CNUpPTProperties(
        _In_ CONST XDPrintSchema::NUp::NUpData&                         nupData,
        _In_ CONST XDPrintSchema::Binding::BindingData&                 bindingData,
        _In_ CONST XDPrintSchema::PageMediaSize::PageMediaSizeData&     pageMediaSizeData,
        _In_ CONST XDPrintSchema::PageOrientation::PageOrientationData& pageOrientData
        );

    virtual ~CNUpPTProperties();

    HRESULT
    GetCount(
        _Out_ UINT* pcNUpPages
        );

    HRESULT
    GetPresentationDirection(
        _Out_ XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOption* pPresentationDirection
        );

    HRESULT
    GetScope(
        _In_ ENUpScope* pNUpScope
        );

    HRESULT
    GetPageSize(
        _Out_ SizeF* pSizePage
        );

    HRESULT
    GetPageOrientation(
        _In_ XDPrintSchema::PageOrientation::EOrientationOption* pPageOrientation
        );

    HRESULT
    GetBindingOption(
        _Out_ XDPrintSchema::Binding::EBindingOption* pBindingOption
        );

private:
    CNUpPTProperties& operator = (CONST CNUpPTProperties&);

    HRESULT
    PresentDirFromBindOption(
        _Out_ XDPrintSchema::NUp::PresentationDirection::ENUpDirectionOption* pPresentationDirection
        );

private:
    CONST XDPrintSchema::NUp::NUpData                         m_nupData;

    CONST XDPrintSchema::Binding::BindingData                 m_bindingData;

    CONST XDPrintSchema::PageMediaSize::PageMediaSizeData     m_pageMediaSizeData;

    CONST XDPrintSchema::PageOrientation::PageOrientationData m_pageOrientData;
};

