/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bmpconv.h

Abstract:

   WIC bitmap conversion class definition. This class provides a wrapper to a bitmap
   stream that uses WIC to access bitmap data and provide conversion functionality.

--*/

#pragma once

#include "bmpdata.h"

class CBmpConverter
{
public:
    CBmpConverter();

    CBmpConverter(
        _In_ CONST EWICPixelFormat& ePixFormat,
        _In_ CONST UINT&            cWidth,
        _In_ CONST UINT&            cHeight,
        _In_ CONST DOUBLE&          dpiX,
        _In_ CONST DOUBLE&          dpiY
        );

    CBmpConverter(
        _In_ IStream* pStream
        );

    CBmpConverter(
        _In_ CONST CBmpConverter& converter
        );

    virtual ~CBmpConverter();

    HRESULT
    Initialize(
        _In_ CONST EWICPixelFormat& ePixFormat,
        _In_ CONST UINT&            cWidth,
        _In_ CONST UINT&            cHeight,
        _In_ CONST DOUBLE&          dpiX,
        _In_ CONST DOUBLE&          dpiY
        );

    HRESULT
    Initialize(
        _In_ IStream* pStream
        );

    HRESULT
    Initialize(
        _In_ IWICBitmapSource* pSource
        );

    HRESULT
    Write(
        _In_    REFGUID  guidContainerFormat,
        _Inout_ IStream* pStream
        );

    HRESULT
    Convert(
        _In_  EWICPixelFormat ePixFormat,
        _Out_ BOOL*           pbCanConvert
        );

    HRESULT
    LockSurface(
        _In_            WICRect*    prcLock,
        _In_            CONST BOOL& bReadOnly,
        _Out_           UINT*       pcbStride,
        _Out_           UINT*       pcWidth,
        _Out_           UINT*       pcHeight,
        _Inout_         UINT*       pcbData,
        _Outptr_result_bytebuffer_maybenull_(*pcbData)
                        PBYTE*      ppbData
        );

    HRESULT
    UnlockSurface(
        VOID
        );

    HRESULT
    GetColorContext(
        _Outptr_ IWICColorContext** ppColorContext
        );

    BOOL
    HasAlphaChannel(
        VOID
        ) CONST;

    BOOL
    HasColorContext(
        VOID
        ) CONST;

    BOOL
    HasColorProfile(
        VOID
        ) CONST;

    EWICPixelFormat
    GetPixelFormat(
        VOID
        );

    HRESULT
    GetSize(
        _Out_ UINT* pcWidth,
        _Out_ UINT* pcHeight
        );

    HRESULT
    GetResolution(
        _Out_ DOUBLE* pDpiX,
        _Out_ DOUBLE* pDpiY
        );

    HRESULT
    SetProfile(
        _In_z_ LPWSTR szProfile
        );

    CBmpConverter&
    operator=(
        _In_ CONST CBmpConverter& converter
        );

private:
    HRESULT
    CreateImagingFactory(
        VOID
        );

    HRESULT
    PixelFormatFromGUID(
        _In_  REFGUID          pixelFormat,
        _Out_ EWICPixelFormat* pPixFormat
        );

protected:
    CComPtr<IWICImagingFactory>    m_pImagingFactory;

    CComPtr<IWICBitmap>            m_pBitmap;

    CComPtr<IWICColorContext>      m_pColorContext;

    CComPtr<IWICBitmapLock>        m_pCurrentLock;

    EWICPixelFormat                m_ePixelFormat;
};

