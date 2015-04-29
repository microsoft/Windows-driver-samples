/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pgscschema.cpp

Abstract:

   PageScaling PrintSchema implementation. This implements the features,
   options and enumerations that describe the PrintSchema PageScaling feature.

--*/

#include "precomp.h"
#include "pgscschema.h"

LPCWSTR XDPrintSchema::PageScaling::SCALE_FEATURE = L"PageScaling";

LPCWSTR XDPrintSchema::PageScaling::SCALE_OPTIONS[EScaleOptionMax] = {
    L"Custom",
    L"CustomSquare",
    L"FitApplicationBleedSizeToPageImageableSize",
    L"FitApplicationContentSizeToPageImageableSize",
    L"FitApplicationMediaSizeToPageImageableSize",
    L"FitApplicationMediaSizeToPageMediaSize",
    L"None"
};

LPCWSTR XDPrintSchema::PageScaling::CUST_SCALE_PROPS[] = {
    L"OffsetWidth",
    L"OffsetHeight",
    L"ScaleWidth",
    L"ScaleHeight"
};

LPCWSTR XDPrintSchema::PageScaling::CUST_SQR_SCALE_PROPS[] = {
    L"OffsetWidth",
    L"OffsetHeight",
    L"Scale"
};

LPCWSTR XDPrintSchema::PageScaling::OffsetAlignment::SCALE_OFFSET_FEATURE = L"ScaleOffsetAlignment";

LPCWSTR XDPrintSchema::PageScaling::OffsetAlignment::SCALE_OFFSET_OPTIONS[] = {
    L"BottomCenter",
    L"BottomLeft",
    L"BottomRight",
    L"Center",
    L"LeftCenter",
    L"RightCenter",
    L"TopCenter",
    L"TopLeft",
    L"TopRight"
};

