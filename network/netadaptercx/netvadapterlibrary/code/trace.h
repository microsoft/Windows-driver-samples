// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include <initguid.h>

// 5EC34F87-7705-4B5B-B5F6-1F3E4665A31E
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID( \
        NetvadapterTraceGuid, \
        (5EC34F87,7705,4B5B,B5F6,1F3E4665A31E), \
        WPP_DEFINE_BIT(FLAG_DRIVER) \
    )

#define WPP_FLAG_LEVEL_LOGGER(flag, level) \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level) \
    (WPP_LEVEL_ENABLED(flag) && WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
    WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
    (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

// begin_wpp config
// USEPREFIX (LogInformation, "%!FUNC! ->");
// LogVerbose{LEVEL=TRACE_LEVEL_VERBOSE}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (LogInformation, "%!FUNC! ->");
// LogWarning{LEVEL=TRACE_LEVEL_WARNING}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (LogInformation, "%!FUNC! ->");
// LogError{LEVEL=TRACE_LEVEL_ERROR}(FLAGS, MSG, ...);
// end_wpp

// begin_wpp config
// USEPREFIX (LogInformation, "%!FUNC! ->");
// LogInformation{LEVEL=TRACE_LEVEL_INFORMATION}(FLAGS, MSG, ...);
// end_wpp

//
// WPP orders static parameters before dynamic parameters. To support the Trace function
// defined below which sets FLAGS=MYDRIVER_ALL_INFO, a custom macro must be defined to
// reorder the arguments to what the .tpl configuration file expects.
//
#define WPP_RECORDER_FLAGS_LEVEL_ARGS(flags, lvl) \
    WPP_RECORDER_LEVEL_FLAGS_ARGS(lvl, flags)

#define WPP_RECORDER_FLAGS_LEVEL_FILTER(flags, lvl) \
    WPP_RECORDER_LEVEL_FLAGS_FILTER(lvl, flags)

// begin_wpp config
// USEPREFIX (RETURN_IF_NOT_STATUS_SUCCESS, "%!STATUS! %!FUNC! ->%!s!", nt__wpp, #NTSTATUS);
// FUNC RETURN_IF_NOT_STATUS_SUCCESS{FLAG=FLAG_DRIVER,LEVEL=TRACE_LEVEL_ERROR}(NTSTATUS);
// end_wpp

#define WPP_FLAG_LEVEL_NTSTATUS_PRE(flag, level, ntstatus) do { NTSTATUS nt__wpp = (ntstatus); if (STATUS_SUCCESS != nt__wpp) {
#define WPP_FLAG_LEVEL_NTSTATUS_POST(flag, level, ntstatus); return nt__wpp; } } while (0)
#define WPP_RECORDER_FLAG_LEVEL_NTSTATUS_FILTER(flag, level, ntstatus) WPP_RECORDER_LEVEL_FLAGS_FILTER(level, flag)
#define WPP_RECORDER_FLAG_LEVEL_NTSTATUS_ARGS(flag, level, ntstatus) WPP_RECORDER_LEVEL_FLAGS_ARGS(level, flag)

// begin_wpp config
// USEPREFIX (RETURN_NTSTATUS_IF, "%!STATUS! %!FUNC! ->%!s!", nt__wpp, #CONDITION);
// FUNC RETURN_NTSTATUS_IF{FLAG=FLAG_DRIVER,LEVEL=TRACE_LEVEL_ERROR}(NTSTATUS, CONDITION);
// end_wpp

#define WPP_FLAG_LEVEL_NTSTATUS_CONDITION_PRE(flag, level, ntstatus, condition) if (condition) { NTSTATUS nt__wpp = (ntstatus);
#define WPP_FLAG_LEVEL_NTSTATUS_CONDITION_POST(flag, level, ntstatus, condition); return nt__wpp; }
#define WPP_RECORDER_FLAG_LEVEL_NTSTATUS_CONDITION_FILTER(flag, level, ntstatus, condition) WPP_RECORDER_LEVEL_FLAGS_FILTER(level, flag)
#define WPP_RECORDER_FLAG_LEVEL_NTSTATUS_CONDITION_ARGS(flag, level, ntstatus, condition) WPP_RECORDER_LEVEL_FLAGS_ARGS(level, flag)

// begin_wpp config
// USEPREFIX (RETURN_FAILED_NTSTATUS_MSG, "%!STATUS! %!FUNC! ->", nt__wpp);
// FUNC RETURN_FAILED_NTSTATUS_MSG{FLAG=FLAG_DRIVER,FAILEDLEVEL=TRACE_LEVEL_ERROR}(NTSTATUS, MSG, ...);
// end_wpp

#define WPP_FLAG_FAILEDLEVEL_NTSTATUS_PRE(flag, level, ntstatus); do { NTSTATUS nt__wpp = (ntstatus);
#define WPP_FLAG_FAILEDLEVEL_NTSTATUS_POST(flag, level, ntstatus); return nt__wpp; } while (0)
#define WPP_RECORDER_FLAG_FAILEDLEVEL_NTSTATUS_FILTER(flag, level, ntstatus) WPP_RECORDER_LEVEL_FLAGS_FILTER(level, flag)
#define WPP_RECORDER_FLAG_FAILEDLEVEL_NTSTATUS_ARGS(flag, level, ntstatus) WPP_RECORDER_LEVEL_FLAGS_ARGS(level, flag)

// begin_wpp config
// USEPREFIX (RETURN_STATUS_SUCCESS, "%!STATUS! %!FUNC!", STATUS_SUCCESS);
// FUNC RETURN_STATUS_SUCCESS{FLAG=FLAG_DRIVER,SUCCESSLEVEL=TRACE_LEVEL_INFORMATION,NTSTATUS=STATUS_SUCCESS}();
// end_wpp

#define WPP_FLAG_SUCCESSLEVEL_NTSTATUS_POST(flag, level, ntstatus); return (ntstatus);
#define WPP_RECORDER_FLAG_SUCCESSLEVEL_NTSTATUS_FILTER(flag, level, ntstatus) WPP_RECORDER_LEVEL_FLAGS_FILTER(level, flag)
#define WPP_RECORDER_FLAG_SUCCESSLEVEL_NTSTATUS_ARGS(flag, level, ntstatus) WPP_RECORDER_LEVEL_FLAGS_ARGS(level, flag)

