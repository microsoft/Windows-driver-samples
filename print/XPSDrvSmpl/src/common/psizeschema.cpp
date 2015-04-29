/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   psizeschema.cpp

Abstract:

   PageMediaSize PrintSchema implementation. This implements the features,
   options and enumerations that describe the PrintSchema PageMediaSize feature.

--*/

#include "precomp.h"
#include "psizeschema.h"

LPCWSTR XDPrintSchema::PageMediaSize::PAGESIZE_FEATURE = L"PageMediaSize";

LPCWSTR XDPrintSchema::PageMediaSize::PAGESIZE_PROPS[] = {
    L"MediaSizeWidth",
    L"MediaSizeHeight"
};

