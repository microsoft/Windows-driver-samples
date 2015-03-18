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

#pragma once

#define WPP_CONTROL_GUIDS   WPP_DEFINE_CONTROL_GUID(                                \
                                XpsRasFilter,                                       \
                                (EB4C6075, 0B67, 4a79, A0A3, 7CD9DF881194),         \
                                WPP_DEFINE_BIT(XPSRASFILTER_TRACE_ERROR)            \
                                WPP_DEFINE_BIT(XPSRASFILTER_TRACE_WARNING)          \
                                WPP_DEFINE_BIT(XPSRASFILTER_TRACE_INFO)             \
                                WPP_DEFINE_BIT(XPSRASFILTER_TRACE_VERBOSE)          \
                                )

#define WPP_LOG_ON_FAILED_HRESULT_WITH_TEXT(func_,text_)    \
    {                                                       \
        HRESULT hr_ = func_;                                \
        if (FAILED(hr_))                                    \
        {                                                   \
            xpsrasfilter::TraceFailedHRESULT(               \
                hr_,                                        \
                __FILE__,                                   \
                __LINE__,                                   \
                text_                                       \
                );                                          \
        }                                                   \
    }

#define WPP_LOG_ON_FAILED_HRESULT(func_)                    \
    {                                                       \
        WPP_LOG_ON_FAILED_HRESULT_WITH_TEXT(func_, L"")     \
    }

namespace xpsrasfilter
{

void 
TraceFailedHRESULT(
    HRESULT hr,
    char const *fileName,
    int lineNum,
    wchar_t const *extraText
    );

} // namespace xpsrasfilter
