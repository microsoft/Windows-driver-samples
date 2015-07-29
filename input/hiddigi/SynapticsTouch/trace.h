/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        trace.h

    Abstract:

        Contains prototypes and definitions related to debugging/tracing

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once

//
// Control GUID:
// {64BAF936-E94C-4747-91E3-BB4CB8328E5F}
//

#define WPP_CONTROL_GUIDS                        \
    WPP_DEFINE_CONTROL_GUID(                     \
        TouchDriverTraceGuid,                    \
        (64BAF936,E94C,4747,91E3,BB4CB8328E5F),  \
        WPP_DEFINE_BIT(TRACE_FLAG_INIT)          \
        WPP_DEFINE_BIT(TRACE_FLAG_REGISTRY)      \
        WPP_DEFINE_BIT(TRACE_FLAG_HID)           \
        WPP_DEFINE_BIT(TRACE_FLAG_PNP)           \
        WPP_DEFINE_BIT(TRACE_FLAG_POWER)         \
        WPP_DEFINE_BIT(TRACE_FLAG_SPB)           \
        WPP_DEFINE_BIT(TRACE_FLAG_CONFIG)        \
        WPP_DEFINE_BIT(TRACE_FLAG_REPORTING)     \
        WPP_DEFINE_BIT(TRACE_FLAG_INTERRUPT)     \
        WPP_DEFINE_BIT(TRACE_FLAG_SAMPLES)       \
        WPP_DEFINE_BIT(TRACE_FLAG_OTHER)         \
        WPP_DEFINE_BIT(TRACE_FLAG_IDLE)          \
        )

#define WPP_LEVEL_FLAGS_LOGGER(level,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(level,flags) \
    (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)

