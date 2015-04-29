/*++

Copyright (c) 2005 Microsoft Corporation

All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

File Name:

   bkschema.cpp

Abstract:

   Binding (booklet) PrintSchema implementation. This implements the features,
   options and enumerations that describe the PrintSchema JobBindAllDocuments and
   DocumentBinding features.

--*/

#include "precomp.h"
#include "bkschema.h"

LPCWSTR XDPrintSchema::Binding::BIND_FEATURES[] = {
    L"JobBindAllDocuments",
    L"DocumentBinding"
};

LPCWSTR XDPrintSchema::Binding::BIND_OPTIONS[] = {
    L"Bale",
    L"BindBottom",
    L"BindLeft",
    L"BindRight",
    L"BindTop",
    L"Booklet",
    L"EdgeStitchBottom",
    L"EdgeStitchLeft",
    L"EdgeStitchRight",
    L"EdgeStitchTop",
    L"Fold",
    L"JogOffset",
    L"Trim",
    L"None"
};

LPCWSTR XDPrintSchema::Binding::BIND_PROP = L"BindingGutter";
LPCWSTR XDPrintSchema::Binding::BIND_PROP_REF_SUFFIX = L"Gutter";

