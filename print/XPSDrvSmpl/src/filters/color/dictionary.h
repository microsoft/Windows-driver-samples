/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   dict.h

Abstract:

   Dictionary class definition. The CRemoteDictionary class is responsible
   for representing a single dictionay including managing any color
   transforms in that dictionary and writing the dictionary out.

--*/

#pragma once

#include "colconv.h"

class CRemoteDictionary : public IResWriter
{
public:
    CRemoteDictionary(
        _In_ IFixedPage*            pFixedPage,
        _In_ CBitmapColorConverter* pBmpConverter,
        _In_ CColorRefConverter*    pRefConverter,
        _In_ BSTR                   bstrResURI
        );

    ~CRemoteDictionary();

    HRESULT
    WriteData(
        _In_ IPartBase*         pResource,
        _In_ IPrintWriteStream* pStream
        );

    HRESULT
    GetKeyName(
        _Outptr_ BSTR* pbstrKeyName
        );

    HRESULT
    GetResURI(
        _Outptr_ BSTR* pbstrResURI
        );

private:
    CComBSTR                m_bstrDictionaryURI;

    CBitmapColorConverter*  m_pBmpConverter;

    CColorRefConverter*     m_pRefConverter;

    CComPtr<IFixedPage>     m_pFixedPage;
};

