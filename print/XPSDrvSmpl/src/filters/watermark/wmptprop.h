/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmptprop.h

Abstract:

   Watermark properties class definition. The Watermark properties class
   is responsible for holding and controling Watermark properties.

--*/

#pragma once

#include "wmdata.h"

class CWMPTProperties
{
public:
    CWMPTProperties(
        _In_ CONST XDPrintSchema::PageWatermark::WatermarkData& wmData
        );

    virtual ~CWMPTProperties();

    HRESULT
    GetType(
        _Out_ XDPrintSchema::PageWatermark::EWatermarkOption* pType
        );

    HRESULT
    GetLayering(
        _Out_ XDPrintSchema::PageWatermark::Layering::ELayeringOption* pLayering
        );

    HRESULT
    GetBounds(
        _Out_ RectF* pBounds
        );

    HRESULT GetOrigin(
        _Out_ PointF* pOrigin
        );

    HRESULT
    GetAngle(
        _Out_ REAL* pAngle
        );

    HRESULT
    GetTransparency(
        _Out_ INT* pTransparency
        );

    HRESULT
    GetOpacity(
        _Out_ REAL* pOpacity
        );

    HRESULT
    GetTransparency(
        _Outptr_ BSTR* pbstrTransparency
        );

    HRESULT
    GetOpacity(
        _Outptr_ BSTR* pbstrOpacity
        );

    HRESULT
    GetFontColor(
        _Inout_ _At_(*pbstrColor, _Pre_maybenull_ _Post_valid_) BSTR* pbstrColor
        );

    HRESULT
    GetText(
        _Inout_ _At_(*pbstrText, _Pre_maybenull_ _Post_valid_) BSTR* pbstrText
        );

    HRESULT
    GetFontSize(
        _Out_ INT* pSize
        );

    HRESULT
    GetFontEmSize(
        _Outptr_ BSTR* pbstrSize
        );

protected:
    XDPrintSchema::PageWatermark::WatermarkData m_wmData;
};

