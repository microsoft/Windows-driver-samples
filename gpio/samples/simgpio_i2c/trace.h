/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    trace.h

Abstract:

    This sample implements a GPIO client driver for simulated GPIO (SimGpio)
    controller.

    Note: DIRQL in the comments below refers to device IRQL, which is any
        IRQL > DISPATCH_LEVEL (and less than some IRQL reserved for OS use).

Environment:

    Kernel mode

--*/

#ifndef _TRACE_H
#define _TRACE_H

#include <evntrace.h> // For TRACE_LEVEL definitions

//
// Define the tracing flags.
//
// Tracing GUID - e71e3b6e-aaf9-402c-97fe-7610cb3985db
//

#define WPP_CONTROL_GUIDS                                               \
    WPP_DEFINE_CONTROL_GUID(                                            \
        SimGpioI2CTraceGuid,                                            \
        (e71e3b6e,aaf9,402c,97fe,7610cb3985db),                         \
        WPP_DEFINE_BIT(TRACE_FLAG_INIT)                                 \
        WPP_DEFINE_BIT(TRACE_FLAG_SPB)                                  \
        WPP_DEFINE_BIT(TRACE_FLAG_REQUEST)                              \
        WPP_DEFINE_BIT(TRACE_FLAG_INTERRUPT)                            \
        WPP_DEFINE_BIT(TRACE_FLAG_IO)                            	\
        )

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
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
// FUNC FuncEntry{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// FUNC FuncExit{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS);
// USEPREFIX(FuncEntry, "%!STDPREFIX! [%!FUNC!] --> entry");
// USEPREFIX(FuncExit, "%!STDPREFIX! [%!FUNC!] <--");
// end_wpp
//

#endif _TRACE_H