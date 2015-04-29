/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   rescpy.cpp

Abstract:

   Page resource copy class implementation. This class stores resources
   from one page and copies them to a destination. This is required when
   copying page markup into a new page - without doing so any resources
   referenced in the source page are lost.

--*/

#include "precomp.h"
#include "debug.h"
#include "globals.h"
#include "rescpy.h"

/*++

Routine Name:

    CResourceCopier::CResourceCopier

Routine Description:

    CResourceCopier class constructor

Arguments:

    None

Return Value:

    None

--*/
CResourceCopier::CResourceCopier()
{
}

/*++

Routine Name:

    CResourceCopier::~CResourceCopier

Routine Description:

    CResourceCopier class destructor

Arguments:

    None

Return Value:

    None

--*/
CResourceCopier::~CResourceCopier()
{
}

/*++

Routine Name:

    CResourceCopier::CopyPageResources

Routine Description:

    This routine takes a source page and copies all resources to a destination page.
    This is required when a filter copies mark-up to a new FixedPage part else the
    resources for the mark-up will be lost.

Arguments:

    pFPSrc - Pointer to the source IFixedPage interface
    pFPDst - Pointer to the destination IFixedPage interface

Return Value:

    HRESULT
    S_OK - On success
    E_*  - On error

--*/
HRESULT
CResourceCopier::CopyPageResources(
    _In_    CONST IFixedPage* pFPSrc,
    _Inout_ IFixedPage*       pFPDst
    )
{
    HRESULT hr = S_OK;
    CComPtr<IXpsPartIterator> pXpsPartIt(NULL);

    if (SUCCEEDED(hr = CHECK_POINTER(pFPSrc, E_POINTER)) &&
        SUCCEEDED(hr = CHECK_POINTER(pFPDst, E_POINTER)) &&
        SUCCEEDED(hr = const_cast<IFixedPage*>(pFPSrc)->GetXpsPartIterator(&pXpsPartIt)))
    {
        pXpsPartIt->Reset();
        while (!pXpsPartIt->IsDone() &&
               SUCCEEDED(hr))
        {
            CComBSTR bstrPartURI;
            CComPtr<IUnknown> pXPSPart(NULL);

            if (SUCCEEDED(hr = pXpsPartIt->Current(&bstrPartURI, &pXPSPart)))
            {
                hr = pFPDst->SetPagePart(pXPSPart);

                pXpsPartIt->Next();
            }
        }
    }

    ERR_ON_HR(hr);
    return hr;
}

