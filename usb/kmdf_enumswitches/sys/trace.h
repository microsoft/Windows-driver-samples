/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    TRACE.h

Abstract:

    Header file for the debug tracing related function defintions and macros.

Environment:

    Kernel mode

--*/

#include <evntrace.h> // For TRACE_LEVEL definitions

#if !defined(EVENT_TRACING)

//
// TODO: These defines are missing in evntrace.h
// in some DDK build environments (XP).
//
#if !defined(TRACE_LEVEL_NONE)
  #define TRACE_LEVEL_NONE        0
  #define TRACE_LEVEL_CRITICAL    1
  #define TRACE_LEVEL_FATAL       1
  #define TRACE_LEVEL_ERROR       2
  #define TRACE_LEVEL_WARNING     3
  #define TRACE_LEVEL_INFORMATION 4
  #define TRACE_LEVEL_VERBOSE     5
  #define TRACE_LEVEL_RESERVED6   6
  #define TRACE_LEVEL_RESERVED7   7
  #define TRACE_LEVEL_RESERVED8   8
  #define TRACE_LEVEL_RESERVED9   9
#endif


//
// Define Debug Flags
//
#define DBG_INIT                0x00000001
#define DBG_PNP                 0x00000002
#define DBG_POWER               0x00000004
#define DBG_WMI                 0x00000008
#define DBG_CREATE_CLOSE        0x00000010
#define DBG_IOCTL               0x00000020
#define DBG_WRITE               0x00000040
#define DBG_READ                0x00000080


VOID
TraceEvents    (
    _In_ ULONG   DebugPrintLevel,
    _In_ ULONG   DebugPrintFlag,
    _Printf_format_string_
    _In_ PCSTR   DebugMessage,
    ...
    );

#define WPP_INIT_TRACING(DriverObject, RegistryPath)
#define WPP_CLEANUP(DriverObject)

#else
//
// If software tracing is defined in the sources file..
// WPP_DEFINE_CONTROL_GUID specifies the GUID used for this driver.
// *** REPLACE THE GUID WITH YOUR OWN UNIQUE ID ***
// WPP_DEFINE_BIT allows setting debug bit masks to selectively print.
// The names defined in the WPP_DEFINE_BIT call define the actual names
// that are used to control the level of tracing for the control guid
// specified.
//
// NOTE: If you are adopting this sample for your driver, please generate
//       a new guid, using tools\other\i386\guidgen.exe present in the
//       DDK.
//
//    Name of the logger is OSRUSBFX2 and the guid is
//   {D23A0C5A-D307-4f0e-AE8E-E2A355AD5DAB}
//   (0xd23a0c5a, 0xd307, 0x4f0e, 0xae, 0x8e, 0xe2, 0xa3, 0x55, 0xad, 0x5d, 0xab);
//

#define WPP_CHECK_FOR_NULL_STRING  //to prevent exceptions due to NULL strings

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(OsrUsbFxTraceGuid,(d23a0c5a,d307,4f0e,ae8e,E2A355AD5DAB), \
        WPP_DEFINE_BIT(DBG_INIT)             /* bit  0 = 0x00000001 */ \
        WPP_DEFINE_BIT(DBG_PNP)              /* bit  1 = 0x00000002 */ \
        WPP_DEFINE_BIT(DBG_POWER)            /* bit  2 = 0x00000004 */ \
        WPP_DEFINE_BIT(DBG_WMI)              /* bit  3 = 0x00000008 */ \
        WPP_DEFINE_BIT(DBG_CREATE_CLOSE)     /* bit  4 = 0x00000010 */ \
        WPP_DEFINE_BIT(DBG_IOCTL)            /* bit  5 = 0x00000020 */ \
        WPP_DEFINE_BIT(DBG_WRITE)            /* bit  6 = 0x00000040 */ \
        WPP_DEFINE_BIT(DBG_READ)             /* bit  7 = 0x00000080 */ \
 /* You can have up to 32 defines. If you want more than that,\
   you have to provide another trace control GUID */\
        )


#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level  >= lvl)


#endif



