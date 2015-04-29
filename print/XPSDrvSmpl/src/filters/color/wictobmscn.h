/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wictobmscn.h

Abstract:

   WIC pixel format to BMFORMAT conversion class implementation. This class provides methods
   for converting between a source WIC scanline to a target destination BMFORMAT scanline.

--*/

#pragma once

#include "bmpdata.h"

class CWICToBMFormatScan
{
public:
    CWICToBMFormatScan();

    virtual ~CWICToBMFormatScan();

    HRESULT
    Initialize(
        _In_ CONST WICToBMFORMAT& WICToBM
        );

    HRESULT
    SetData(
        _In_                     CONST BOOL bIsSrc,
        _In_reads_bytes_(cbWicPxData) PBYTE      pWicPxData,
        _In_                     CONST UINT cbWicPxData,
        _In_                     CONST UINT cWidth
        );

    HRESULT
    Commit(
        _In_reads_bytes_(cbWicPxData) PBYTE      pWicPxData,
        _In_                     CONST UINT cbWicPxData
        );

    HRESULT
    GetData(
        _Outptr_result_bytebuffer_(*pcbStride) PBYTE*    ppData,
        _Out_                          BMFORMAT* pBmFormat,
        _Out_                          UINT*     pcWidth,
        _Out_                          UINT*     pcbStride
        );

private:
    HRESULT
    CreateScanBuffer(
        _In_ CONST UINT   cWidth,
        _In_ CONST SIZE_T cbDataType,
        _In_ CONST UINT   cChannels
        );

    VOID
    FreeScanBuffer(
        VOID
        );

private:
    WICToBMFORMAT m_convData;

    PBYTE   m_pScanBuffer;

    size_t  m_cbScanBuffer;

    UINT    m_cWidth;

    PBYTE   m_pData;

    size_t  m_cbData;

    BOOL    m_bInitialized;
};
