/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    trace.h

Abstract:

    This module contains the trace definitions for the SPB
    peripheral driver.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _TRACE_H_
#define _TRACE_H_

extern "C" 
{
//
// Tracing Definitions:
//
// Control GUID: 
// {73e3b785-f5fb-423e-94a9-56627fea9053}
//

#define WPP_CONTROL_GUIDS                           \
    WPP_DEFINE_CONTROL_GUID(                        \
        SpbTestToolTraceGuid,                       \
        (73e3b785,f5fb,423e,94a9,56627fea9053),     \
        WPP_DEFINE_BIT(TRACE_FLAG_WDFLOADING)       \
        WPP_DEFINE_BIT(TRACE_FLAG_SPBAPI)           \
        WPP_DEFINE_BIT(TRACE_FLAG_OTHER)            \
        )
}

#define WPP_LEVEL_FLAGS_LOGGER(level,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(level, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)

// begin_wpp config
// FUNC FuncEntry{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// FUNC FuncExit{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// USEPREFIX(FuncEntry, "%!STDPREFIX! [%!FUNC!] --> entry");
// USEPREFIX(FuncExit, "%!STDPREFIX! [%!FUNC!] <--");
// end_wpp

#endif _TRACE_H_
