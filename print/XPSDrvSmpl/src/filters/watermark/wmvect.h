/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmvect.h

Abstract:

   VectorGraphic watermark class definition. CVectorWatermark is the
   Vector implementation of the CWatermark class. This implements methods
   for creating the page mark-up and adding the watermark resource to fixed page.

--*/

#pragma once

#include "wmbase.h"
#include "wmxps.h"
#include "wmptprop.h"

class CVectorWatermark : public CWatermark
{
public:
    CVectorWatermark(
        _In_ CONST CWMPTProperties& wmProps,
        _In_ CONST INT              resourceID
        );

    virtual ~CVectorWatermark();

    virtual HRESULT
    CreateXMLElement(
        VOID
        );

    virtual HRESULT
    GetXML(
        _Inout_ _At_(*pbstrXML, _Pre_maybenull_ _Post_valid_) BSTR* pbstrXML
        );

    virtual HRESULT
    AddParts(
        _In_ IXpsDocumentConsumer* pXpsConsumer,
        _In_ IFixedPage*           pFixedPage,
        _In_ CFileResourceCache*   pResCache
        );

private:
    CWatermarkMarkup  m_wmMarkup;

    CComBSTR          m_bstrMarkup;
};

