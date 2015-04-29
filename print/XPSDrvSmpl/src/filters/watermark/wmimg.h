/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmimg.h

Abstract:

   Watermark image definition. The CWatermarkImage class is responsible
   for managing the image resource for a raster watermark. This implements
   the IResWriter interface so that the font can be added to the resource
   cache.

--*/

#pragma once

#include "rescache.h"
#include "wmptprop.h"

class CWatermarkImage : public IResWriter
{
public:
    CWatermarkImage(
        _In_ CONST CWMPTProperties& wmProps,
        _In_ CONST INT              resourceID
        );

    ~CWatermarkImage();

    HRESULT
    GetImageDimensions(
        _Out_ SizeF* pDimensions
        );

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

    HRESULT
    CheckResID(
        VOID
        );

private:
    HRESULT
    CreatePNGStream(
        VOID
        );

    HRESULT
    LoadPNGResource(
        VOID
        );

private:
    CWMPTProperties  m_WMProps;

    INT              m_resourceID;

    CComPtr<IStream> m_pPNGStream;

    PVOID            m_pPNGData;

    DWORD            m_cbPNGData;

    HGLOBAL          m_hPNGRes;
};

