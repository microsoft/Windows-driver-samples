/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmprofileschema.cpp

Abstract:

   PageSourceColorProfile PrintSchema implementation. This implements the
   features, options and enumerations that describe the PrintSchema
   PageSourceColorProfile feature.

--*/

#include "precomp.h"
#include "globals.h"
#include "privatedefs.h"
#include "cmprofileschema.h"

LPCWSTR XDPrintSchema::PageSourceColorProfile::PROFILE_FEATURE = L"PageSourceColorProfile";

LPCWSTR XDPrintSchema::PageSourceColorProfile::PROFILE_OPTIONS[] = {
    L"RGB",
    L"CMYK"
};

LPCWSTR XDPrintSchema::PageSourceColorProfile::PROFILE_URI_PROP = L"SourceColorProfileURI";
LPCWSTR XDPrintSchema::PageSourceColorProfile::PROFILE_URI_REF = L"PageSourceColorProfileURI";

PRIVATE_DEF_STRINGS XDPrintSchema::PageSourceColorProfile::PROFILE_PARAM_DEF =  {
    "PageSourceColorProfileURI",
    NULL,
    "xdCMYKPrinter.icc",
    0,
    65536,
    "characters"
};



