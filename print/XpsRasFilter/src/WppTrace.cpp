// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    WppTrace.h
//
// Abstract:
//
//    WPP tracing definitions.
//

#include "precomp.h"
#include "WppTrace.h"

#include "WppTrace.tmh"

namespace xpsrasfilter
{

void TraceFailedHRESULT(
    HRESULT hr,
    char const *fileName,
    int lineNum,
    wchar_t const *extraText
    )
{
    DoTraceMessage(XPSRASFILTER_TRACE_ERROR,
        "Failed HRESULT (%!HRESULT!) at %s:%d (%S)",
        hr,
        fileName,
        lineNum,
        extraText
        );
}

} // namespace xpsrasfilter
