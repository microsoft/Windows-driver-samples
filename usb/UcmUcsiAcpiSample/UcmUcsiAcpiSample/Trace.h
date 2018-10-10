/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    Trace.h

Abstract:

    WPP tracing definitions.

Environment:

    Kernel-mode.

--*/

//
// Tracing GUID - {EDEF8E04-4E22-4A95-9D04-539EBD112A5E}
//

#pragma once


#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
        UcmUcsiAcpiClient, (EDEF8E04,4E22,4A95,9D04,539EBD112A5E), \
        WPP_DEFINE_BIT(TRACE_FLAG_DRIVER) \
        WPP_DEFINE_BIT(TRACE_FLAG_FDO) \
        WPP_DEFINE_BIT(TRACE_FLAG_PPM) \
        WPP_DEFINE_BIT(TRACE_FLAG_ACPI) \
        )

#define WPP_LEVEL_FLAGS_LOGGER(lvl, flags) \
    WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
    (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// begin_wpp config
//
// FUNC TRACE_ERROR{LEVEL=TRACE_LEVEL_ERROR}(FLAGS, MSG,...);
//
// FUNC TRACE_WARN{LEVEL=TRACE_LEVEL_WARNING}(FLAGS, MSG, ...);
//
// FUNC TRACE_INFO{LEVEL=TRACE_LEVEL_INFORMATION}(FLAGS, MSG, ...);
//
// FUNC TRACE_VERBOSE{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, MSG, ...);
//
// FUNC TRACE_FUNC_ENTRY{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, ...);
// USESUFFIX(TRACE_FUNC_ENTRY, "%!FUNC! Entry");
//
// FUNC TRACE_FUNC_EXIT{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, ...);
// USESUFFIX(TRACE_FUNC_EXIT, "%!FUNC! Exit");
//
// end_wpp
//

//
// Definitions to satisy the Intellisense compiler in Visual Studio.
// Conditionally include the .tmh file as well, otherwise it will override these definitions.
//


#define TRACE_FLAG_DRIVER
#define TRACE_FLAG_FDO
#define TRACE_FLAG_PPM
#define TRACE_FLAG_ACPI
#define TRACE_ERROR(...)
#define TRACE_WARN(...)
#define TRACE_INFO(...)
#define TRACE_VERBOSE(...)
#define TRACE_FUNC_ENTRY(...)
#define TRACE_FUNC_EXIT(...)
