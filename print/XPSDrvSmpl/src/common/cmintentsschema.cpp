/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   cmintentsschema.cpp

Abstract:

   PageICMRenderingIntent PrintSchema implementation. This implements the features,
   options and enumerations that describe the PrintSchema PageICMRenderingIntent feature.

--*/

#include "precomp.h"
#include "cmintentsschema.h"

LPCWSTR XDPrintSchema::PageICMRenderingIntent::ICMINTENT_FEATURE = L"PageICMRenderingIntent";

LPCWSTR XDPrintSchema::PageICMRenderingIntent::ICMINTENT_OPTIONS[] = {
    L"AbsoluteColorimetric",
    L"RelativeColorimetric",
    L"Photographs",
    L"BusinessGraphics"
};

