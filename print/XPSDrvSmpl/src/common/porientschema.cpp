/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   porientschema.cpp

Abstract:

   PageOrientation PrintSchema implementation. This implements the features,
   options and enumerations that describe the PrintSchema PageOrientation feature.

--*/

#include "precomp.h"
#include "porientschema.h"

LPCWSTR XDPrintSchema::PageOrientation::ORIENTATION_FEATURE = L"PageOrientation";

LPCWSTR XDPrintSchema::PageOrientation::ORIENTATION_OPTIONS[EOrientationOptionMax] = {
    L"Landscape",
    L"Portrait",
    L"ReverseLandscape",
    L"ReversePortrait"
};

