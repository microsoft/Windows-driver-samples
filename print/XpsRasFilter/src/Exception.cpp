// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// File Name:
//
//    Exception.cpp
//
// Abstract:
//
//    Exception routine definitions.
//

#include "precomp.h"
#include "WppTrace.h"
#include "Exception.h"

#include "Exception.tmh"

namespace xpsrasfilter
{

void ThrowHRException(
    HRESULT hr,
    char const *fileName,
    int lineNum
    )
{
    DoTraceMessage(
        XPSRASFILTER_TRACE_ERROR,
        L"Throwing HRESULT Exception from %s:%d (HRESULT=%!HRESULT!)", 
        fileName, 
        lineNum, 
        hr
        );

    throw hr_error(hr);
}

} // namespace xpsrasfilter
