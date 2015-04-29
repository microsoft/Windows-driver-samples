/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmxps.h

Abstract:

   Watermark XPS markup class definition. The CWatermarkMarkup class is responsible
   for creating a stream that contains markup loaded from a resource.

--*/

#pragma once

#include "wmptprop.h"

class CWatermarkMarkup
{
public:
    CWatermarkMarkup(
        _In_ CONST CWMPTProperties& wmProps,
        _In_ CONST INT              resourceID
        );

    ~CWatermarkMarkup();

    HRESULT
    GetImageDimensions(
        _Out_ SizeF* pDimensions
        );

    HRESULT
    GetStream(
        _Out_ IStream** ppStream
        );

private:
    HRESULT
    CreateXPSStream(
        VOID
        );

private:
    CWMPTProperties  m_WMProps;

    INT              m_resourceID;

    CComPtr<IStream> m_pXPSStream;
};

