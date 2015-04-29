/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkprps.cpp

Abstract:

   Booklet properties class implementation. The booklet properties class
   is responsible for interpreting booklet data appropriate to the booklet
   filter.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "bkprps.h"

using XDPrintSchema::Binding::BindingData;
using XDPrintSchema::Binding::JobBindAllDocuments;
using XDPrintSchema::Binding::None;

/*++

Routine Name:

    CBkPTProperties::CBkPTProperties

Routine Description:

    Default constructor for the booklet PrintTicket properties class which
    sets the internal binding setting to the setting supplied

Arguments:

    bindingData - Structure containing the booklet binding settings
                  from the PrintTicket

Return Value:

    None

--*/
CBkPTProperties::CBkPTProperties(
    _In_ CONST BindingData& bindingData
    ) :
    m_bindData(bindingData)
{
}

/*++

Routine Name:

    CBkPTProperties::~CBkPTProperties

Routine Description:

    Default destructor for the CBkPTProperties class

Arguments:

    None

Return Value:

    None

--*/
CBkPTProperties::~CBkPTProperties()
{
}

/*++

Routine Name:

    CBkPTProperties::GetScope

Routine Description:

    Method to return the booklet scope which can be either none, job wide or document wide

Arguments:

    pBkScope - Booklet printing scope which can be None, Job wide or Document

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CBkPTProperties::GetScope(
    _Out_ EBookletScope* pBkScope
    )
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr = CHECK_POINTER(pBkScope, E_POINTER)))
    {
        *pBkScope = CBkPTProperties::None;

        if (m_bindData.bindOption != None)
        {
            *pBkScope = m_bindData.bindFeature == JobBindAllDocuments ? CBkPTProperties::Job : CBkPTProperties::Document;
        }
    }

    ERR_ON_HR(hr);
    return hr;
}
