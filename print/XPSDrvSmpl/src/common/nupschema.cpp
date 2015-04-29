/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   nupschema.cpp

Abstract:

   NUp PrintSchema implementation. This implements the features,
   options and enumerations that describe the PrintSchema JobNUpAllDocumentsContiguously
   and DocumentNUp features.

--*/

#include "precomp.h"
#include "nupschema.h"

LPCWSTR XDPrintSchema::NUp::NUP_FEATURES[] = {
    L"JobNUpAllDocumentsContiguously",
    L"DocumentNUp"
};

LPCWSTR XDPrintSchema::NUp::NUP_PROP = L"PagesPerSheet";

LPCWSTR XDPrintSchema::NUp::PresentationDirection::NUP_DIRECTION_FEATURE = L"PresentationDirection";

LPCWSTR XDPrintSchema::NUp::PresentationDirection::NUP_DIRECTION_OPTIONS[] = {
    L"RightBottom",
    L"BottomRight",
    L"LeftBottom",
    L"BottomLeft",
    L"RightTop",
    L"TopRight",
    L"LeftTop",
    L"TopLeft"
};

