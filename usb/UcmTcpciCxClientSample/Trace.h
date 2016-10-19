/*++

Module Name:

    Trace.h

Abstract:

    This file contains the debug tracing related function declarations and macros.

Environment:

    Kernel mode

--*/

#pragma once

// Define the tracing flags.
// TODO: Define your own tracing GUID
// Tracing GUID - 1dc982f3-068f-4577-bcdf-1bc844e457b2
#define WPP_CONTROL_GUIDS                                                            \
    WPP_DEFINE_CONTROL_GUID(                                                         \
        UcmTcpciCxClientSampleTraceGuid, (1dc982f3,068f,4577,bcdf,1bc844e457b2),     \
                                                                                     \
        WPP_DEFINE_BIT(TRACE_ALERT)                                                  \
        WPP_DEFINE_BIT(TRACE_DRIVER)                                                 \
        WPP_DEFINE_BIT(TRACE_DEVICE)                                                 \
        WPP_DEFINE_BIT(TRACE_QUEUE)                                                  \
        WPP_DEFINE_BIT(TRACE_I2C)                                                    \
        WPP_DEFINE_BIT(TRACE_PORTCONTROLLERINTERFACE)                                \
        )

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                           \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                          \
    (WPP_LEVEL_ENABLED(flag) &&                                                      \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
//
// FUNC TRACE_FUNC_ENTRY{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, ...);
// USESUFFIX(TRACE_FUNC_ENTRY, "%!FUNC! Entry");
//
// FUNC TRACE_FUNC_EXIT{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, ...);
// USESUFFIX(TRACE_FUNC_EXIT, "%!FUNC! Exit");
//
// end_wpp
//
