/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    trace.h

Abstract:

    This file contains prototypes and definitions related to debugging.


Environment:

    Kernel mode

--*/

#pragma once

#include <evntrace.h> // For TRACE_LEVEL definitions

#if defined(EVENT_TRACING)

#define WPP_CHECK_FOR_NULL_STRING  // to prevent exceptions due to NULL strings.

//
// WPP_DEFINE_CONTROL_GUID specifies the GUID used for Sample ACPI PEP driver.
// {CA02EEA8-D08F-4C60-BC75-B8C81C485342}
//

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(SampleAcpiPepTraceGuid,(CA02EEA8,D08F,4C60,BC75,B8C81C485342), \
        WPP_DEFINE_BIT(DBG_FUNC_TRACE)       /* bit  0  = 0x00000001 */ \
        WPP_DEFINE_BIT(DBG_INIT)             /* bit  1  = 0x00000002 */ \
        WPP_DEFINE_BIT(DBG_PNP)              /* bit  2  = 0x00000004 */ \
        WPP_DEFINE_BIT(DBG_POWER)            /* bit  3  = 0x00000008 */ \
        WPP_DEFINE_BIT(DBG_WMI)              /* bit  4  = 0x00000010 */ \
        WPP_DEFINE_BIT(DBG_IRP_CREATE_CLOSE) /* bit  5  = 0x00000020 */ \
        WPP_DEFINE_BIT(DBG_IOCTL)            /* bit  6  = 0x00000040 */ \
        WPP_DEFINE_BIT(DBG_DEVICE)           /* bit  7  = 0x00000080 */ \
        WPP_DEFINE_BIT(DBG_REQUEST)          /* bit  8  = 0x00000100 */ \
        WPP_DEFINE_BIT(DBG_PEP)              /* bit  9  = 0x00000200 */ \
        WPP_DEFINE_BIT(DBG_ACPI)             /* bit  10 = 0x00000400 */ \
        )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags)                                 \
    (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level  >= lvl)

#define WPP_RECORDER_LEVEL_FLAGS_ARGS(lvl,flags) \
    WPP_CONTROL(WPP_BIT_ ## flags).AutoLogContext, 0, WPP_BIT_ ## flags
#define WPP_RECORDER_LEVEL_FLAGS_FILTER(lvl,flags) \
    (lvl < TRACE_LEVEL_VERBOSE || WPP_CONTROL(WPP_BIT_ ## flags).AutoLogVerboseEnabled)

#else

//
// Define Debug Flags.
//

#define DBG_FUNC_TRACE          0x00000001
#define DBG_INIT                0x00000002
#define DBG_PNP                 0x00000004
#define DBG_POWER               0x00000008
#define DBG_WMI                 0x00000010
#define DBG_IRP_CREATE_CLOSE    0x00000020
#define DBG_IOCTL               0x00000040
#define DBG_DEVICE              0x00000080
#define DBG_REQUEST             0x00000100
#define DBG_PEP                 0x00000200
#define DBG_ACPI                0x00000400

VOID
TraceEvents (
    __in ULONG DebugPrintLevel,
    __in ULONG DebugPrintFlag,
    __drv_formatString(printf) __in PCSTR DebugMessage,
    ...
    );

#define WPP_INIT_TRACING(DriverObject, RegistryPath)
#define WPP_CLEANUP(DriverObject)

#endif

//
// Friendly names for various trace levels.
//

typedef enum _INTERNAL_TRACE_LEVELS {
    ERROR = TRACE_LEVEL_ERROR,
    WARN = TRACE_LEVEL_WARNING,
    INFO = TRACE_LEVEL_INFORMATION,
    VERBOSE = TRACE_LEVEL_VERBOSE
} INTERNAL_TRACE_LEVELS, *PINTERNAL_TRACE_LEVELS;

#define DebugAssert(_exp) NT_ASSERT(_exp)
