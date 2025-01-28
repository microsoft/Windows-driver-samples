//
// File Name:
//
//    CustomWppCommands.cpp
//
// Abstract:
//
//    Custom WPP Tracing functions.
//

#include "precomp.h"
#include "WppTrace.h"

#include "CustomWppCommands.tmh"

namespace v4PrintDriver_Render_Filter
{

void TraceFailedHRESULT(
    HRESULT hr,
    char const *fileName,
    int lineNum,
    wchar_t const *extraText
    )
{
    DoTraceMessage(
        RENDERFILTER_TRACE_ERROR,
        "Failed HRESULT (%!HRESULT!) at %s:%d (%S)",
        hr,
        fileName,
        lineNum,
        extraText
        );
}

} // namespace v4PrintDriver_Render_Filter