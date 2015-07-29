/*++

Module Name:

    Trace.h

Abstract:

    Header file for the debug tracing related functions and macros.

Environment:

    Kernel mode

--*/

#pragma once

#include "UfxClientSample.h"

//
// Define the tracing flags.
//
// WPP Tracing GUID - 302A11C3-1DB2-4B1E-B549-F5300003894A
//

#define WPP_CONTROL_GUIDS                                              \
    WPP_DEFINE_CONTROL_GUID(                                           \
        UfxClientSampleWPPGuid, (302A11C3,1DB2,4B1E,B549,F5300003894A), \
        WPP_DEFINE_BIT(FlagDriverWideLog)         \
        WPP_DEFINE_BIT(FlagCallStack)         \
        ) 

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
    WPP_LEVEL_LOGGER(flags)
               
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
    (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace* functions
//
// begin_wpp config
// FUNC TraceMessage(LEVEL,FLAGS, MSG,...);
// FUNC TraceFatal{LEVEL=TRACE_LEVEL_FATAL,FLAGS=FlagDriverWideLog}(MSG,...);
// FUNC TraceError{LEVEL=TRACE_LEVEL_ERROR,FLAGS=FlagDriverWideLog}(MSG,...);
// FUNC TraceWarning{LEVEL=TRACE_LEVEL_WARNING,FLAGS=FlagDriverWideLog}(MSG,...);
// FUNC TraceInformation{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=FlagDriverWideLog}(MSG,...);
// FUNC TraceVerbose{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=FlagDriverWideLog}(MSG,...);
// end_wpp
//


// This block defines the function entry and exit functions
//
// begin_wpp config
// FUNC TraceEntry();
// FUNC TraceExit();
// USESUFFIX(TraceEntry, "==>%!FUNC!");
// USESUFFIX(TraceExit, "<==%!FUNC!");
// end_wpp
//
#define WPP__ENABLED() WPP_LEVEL_ENABLED(FlagCallStack)
#define WPP__LOGGER() WPP_LEVEL_LOGGER(FlagCallStack)


//
// This block defines a CHK_NT_MSG macro that:
// 1. Checks the NTSTATUS message in the first parameter
// 2. Logs status message to WPP and ETW, if the status indicates a failure
// 3. Jumps to label "End"
//
// begin_wpp config
// FUNC CHK_NT_MSG{CHK=DUMMY,LEVEL=TRACE_LEVEL_ERROR,FLAGS=FlagDriverWideLog}(STATUS, MSG, ...);
// USESUFFIX(CHK_NT_MSG, "[%!STATUS!]", STATUS);
// end_wpp
//
#define WPP_CHK_LEVEL_FLAGS_STATUS_PRE(CHK,LEVEL,FLAGS,STATUS) { \
    if (NT_SUCCESS(STATUS) != TRUE) {

#define WPP_CHK_LEVEL_FLAGS_STATUS_POST(CHK,LEVEL,FLAGS,STATUS) ;\
        EventWriteFailedNtStatus(&UfxClientSampleGuid, __FILE__, __LINE__, STATUS); \
        goto End; \
    } }
#define WPP_CHK_LEVEL_FLAGS_STATUS_ENABLED(chk,level,flags,status) WPP_LEVEL_ENABLED(flags)
#define WPP_CHK_LEVEL_FLAGS_STATUS_LOGGER(chk,level,flags,status) WPP_LEVEL_LOGGER(flags)

//
// This block defines a LOG_NT_MSG macro that:
// 1. Checks the NTSTATUS message in the first parameter
// 2. And just logs the status message to WPP and ETW, if it indicates a failure
//
// begin_wpp config
// FUNC LOG_NT_MSG{LOG=DUMMY,LEVEL=TRACE_LEVEL_ERROR,FLAGS=FlagDriverWideLog}(STATUS, MSG, ...);
// USESUFFIX(CHK_NT_MSG, "[%!STATUS!]", STATUS);
// end_wpp
//
#define WPP_LOG_LEVEL_FLAGS_STATUS_PRE(CHK,LEVEL,FLAGS,STATUS) { \
    if (NT_SUCCESS(STATUS) != TRUE) {

#define WPP_LOG_LEVEL_FLAGS_STATUS_POST(CHK,LEVEL,FLAGS,STATUS) ;\
        EventWriteFailedNtStatus(&UfxClientSampleGuid, __FILE__, __LINE__, STATUS); \
    } }

#define WPP_LOG_LEVEL_FLAGS_STATUS_ENABLED(chk,level,flags,status) WPP_LEVEL_ENABLED(flags)
#define WPP_LOG_LEVEL_FLAGS_STATUS_LOGGER(chk,level,flags,status) WPP_LEVEL_LOGGER(flags)

//
// This should optimize WPP macros by disbaling checking for 'WPP_INIT_TRACING'
// (okay, because we enable WPP tracing in Driver entry)
//
#define WPP_CHECK_INIT
