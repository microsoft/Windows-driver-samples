/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   wmschema.cpp

Abstract:

   PageWatermark PrintSchema implementation. This implements the features,
   options and enumerations that describe the PrintSchema PageWatermark feature.

--*/

#include "precomp.h"
#include "wmschema.h"

LPCWSTR XDPrintSchema::PageWatermark::WATERMARK_FEATURE = L"PageWatermark";

LPCWSTR XDPrintSchema::PageWatermark::WATERMARK_OPTIONS[] = {
    L"None",
    L"Text",
    L"BitmapGraphic",
    L"VectorGraphic"
};

LPCWSTR XDPrintSchema::PageWatermark::CMN_WATERMARK_PROPS[] = {
    L"OriginWidth",
    L"OriginHeight",
    L"Transparency",
    L"Angle"
};

LPCWSTR XDPrintSchema::PageWatermark::TXT_WATERMARK_PROPS[] = {
    L"TextColor",
    L"TextFontSize",
    L"TextText"
};

LPCWSTR XDPrintSchema::PageWatermark::VECTBMP_WATERMARK_PROPS[] = {
    L"SizeWidth",
    L"SizeHeight"
};

LPCWSTR XDPrintSchema::PageWatermark::Layering::LAYERING_FEATURE  = L"Layering";

LPCWSTR XDPrintSchema::PageWatermark::Layering::LAYERING_OPTIONS[] = {
    L"Underlay",
    L"Overlay"
};


