//
// File Name:
//
//    CustomWppCommands.h
//
// Abstract:
//
//    Custom WPP Tracing function declarations/macro definitions.
//

#pragma once

namespace v4PrintDriver_Render_Filter
{

void
TraceFailedHRESULT(
    HRESULT hr,
    char const *fileName,
    int lineNum,
    wchar_t const *extraText
    );

} // namespace v4PrintDriver_Render_Filter


#define WPP_LOG_ON_FAILED_HRESULT_WITH_TEXT(func_,text_)    \
    {                                                       \
        HRESULT hr_ = func_;                                \
        if (FAILED(hr_))                                    \
        {                                                   \
           v4PrintDriver_Render_Filter::TraceFailedHRESULT(                \
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

