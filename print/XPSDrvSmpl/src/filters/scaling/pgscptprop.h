/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscptprop.h

Abstract:

   Page Scaling properties class definition. The Page Scaling properties class
   is responsible for holding and controling Page Scaling properties.

--*/

#pragma once

#include "pgscdata.h"
#include "psizedata.h"
#include "pimagedata.h"
#include "porientdata.h"

class CPGSCPTProperties
{
public:
    CPGSCPTProperties(
        _In_ CONST XDPrintSchema::PageScaling::PageScalingData&         pgscData,
        _In_ CONST XDPrintSchema::PageMediaSize::PageMediaSizeData&     psizeData,
        _In_ CONST XDPrintSchema::PageImageableSize::PageImageableData& pimageableData,
        _In_ CONST XDPrintSchema::PageOrientation::PageOrientationData& pageOrientData
        );

    virtual ~CPGSCPTProperties();

    HRESULT
    GetOption(
        _Out_ XDPrintSchema::PageScaling::EScaleOption* pScaleOption
        );

    HRESULT
    GetWidthOffset(
        _Out_ REAL* pWidthOffset
        );

    HRESULT
    GetHeightOffset(
        _Out_ REAL* pHeightOffset
        );

    HRESULT
    GetWidthScale(
        _Out_ REAL* pWidthScale
        );

    HRESULT
    GetHeightScale(
        _Out_ REAL* pHeightScale
        );

    HRESULT
    GetOffsetOption(
        _Out_ XDPrintSchema::PageScaling::OffsetAlignment::EScaleOffsetOption* pOffsetOption
        );

    HRESULT
    GetPageSize(
        _Out_ SizeF* pSizePage
    );

    HRESULT
    GetImageableRect(
        _Out_ RectF* pImageableRect
        );

protected:
    XDPrintSchema::PageScaling::PageScalingData         m_pgscData;

    XDPrintSchema::PageMediaSize::PageMediaSizeData     m_pageMediaSizeData;

    XDPrintSchema::PageImageableSize::PageImageableData m_pageImageableData;

    XDPrintSchema::PageOrientation::PageOrientationData m_pageOrientData;
};

