/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   pimageschema.cpp

Abstract:

   PageImageableSize PrintSchema implementation. This implements the features,
   options and enumerations that describe the PrintSchema PageImageableSize feature.

--*/

#include "precomp.h"
#include "pimageschema.h"

LPCWSTR XDPrintSchema::PageImageableSize::PAGE_IMAGEABLE_PROPERTY = L"PageImageableSize";

LPCWSTR XDPrintSchema::PageImageableSize::PAGE_IMAGEABLE_PROPS[] = {
    L"ImageableSizeWidth",
    L"ImageableSizeHeight",
    L"ImageableArea"
};

LPCWSTR XDPrintSchema::PageImageableSize::PAGE_IMAGEABLE_PROPS_AREA[] = {
    L"OriginWidth",
    L"OriginHeight",
    L"ExtentWidth",
    L"ExtentHeight"
};

