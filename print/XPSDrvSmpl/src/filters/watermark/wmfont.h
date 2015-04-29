/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmfont.h

Abstract:

   Watermark font definition. The CWatermarkFont class is responsible
   for managing the font resource for a text watermark. This implements
   the IResWriter interface so that the font can be added to the resource
   cache.

Known Issues:

    The watermark font does not yet use the Uniscript interface to retrieve glyph
    indices.

--*/

#pragma once

#include "rescache.h"
#include "wmptprop.h"

class CWatermarkFont : public IResWriter
{
public:
    CWatermarkFont(
        _In_ CONST CWMPTProperties& wmProps
        );

    ~CWatermarkFont();

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
    HRESULT
    SetFont(
        VOID
        );

    VOID
    UnsetFont(
        VOID
        );

private:
    HDC                  m_hDC;

    HFONT                m_hFont;

    HFONT                m_hOldFont;

    CComBSTR             m_bstrFaceName;

    CWMPTProperties      m_WMProps;
};

