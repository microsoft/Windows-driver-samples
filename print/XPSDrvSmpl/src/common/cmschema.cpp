/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmschema.cpp

Abstract:

   PageColorManagement PrintSchema implementation. This implements the features,
   options and enumerations that describe the PrintSchema PageColorManagement feature.

--*/

#include "precomp.h"
#include "cmschema.h"

LPCWSTR XDPrintSchema::PageColorManagement::PCM_FEATURE = L"PageColorManagement";

LPCWSTR XDPrintSchema::PageColorManagement::PCM_OPTIONS[] = {
    L"None",
    L"Device",
    L"Driver",
    L"System"
};

