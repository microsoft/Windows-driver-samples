/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name: 

    i2ctrace.h

Abstract:

    This module contains the trace definitions for the PBC
    controller driver.

Environment:

    kernel-mode only

Revision History:

--*/

#ifndef _I2CTRACE_H_
#define _I2CTRACE_H_

extern "C" 
{
//
// Tracing Definitions:
//
// TODO: Define a unique tracing guid.
//
// Control GUID: 
// {3AD0F092-64C8-4e69-B93D-7FB64933FFDD}

#define WPP_CONTROL_GUIDS                           \
    WPP_DEFINE_CONTROL_GUID(                        \
        PbcTraceGuid,                               \
        (3AD0F092,64C8,4e69,B93D,7FB64933FFDD),     \
        WPP_DEFINE_BIT(TRACE_FLAG_WDFLOADING)       \
        WPP_DEFINE_BIT(TRACE_FLAG_SPBDDI)           \
        WPP_DEFINE_BIT(TRACE_FLAG_PBCLOADING)       \
        WPP_DEFINE_BIT(TRACE_FLAG_TRANSFER)         \
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

#endif // _I2CTRACE_H_
