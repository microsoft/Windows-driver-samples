//Copyright (C) Microsoft Corporation, All Rights Reserved
//
//Abstract:
//
//    Header file for the debug tracing related function defintions and macros.
//
//Environment:
//
//    User mode

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Define the tracing flags.
//
// Tracing GUID - 3C773167-F26D-4F72-A1DE-95F1FD795840

#define WPP_CONTROL_GUIDS                                                               \
    WPP_DEFINE_CONTROL_GUID(                                                            \
        SimpleDeviceOrientationSensorTraceGuid, (3C773167,F26D,4F72,A1DE,95F1FD795840), \
        WPP_DEFINE_BIT(EntryExit)                                                       \
        WPP_DEFINE_BIT(DataFlow)                                                        \
        WPP_DEFINE_BIT(Verbose)                                                         \
        WPP_DEFINE_BIT(Information)                                                     \
        WPP_DEFINE_BIT(Warning)                                                         \
        WPP_DEFINE_BIT(Error)                                                           \
        WPP_DEFINE_BIT(Fatal)                                                           \
        WPP_DEFINE_BIT(DriverStatus)                                                    \
        )

#define WPP_FLAG_LEVEL_LOGGER(flag, level)      WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)     (WPP_LEVEL_ENABLED(flag) && WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(level,flags)     WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(level, flags)   (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)

// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
//
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
//
// FUNC TraceFatal{LEVEL=TRACE_LEVEL_FATAL,FLAGS=Fatal}(MSG,...);
// FUNC TraceError{LEVEL=TRACE_LEVEL_ERROR,FLAGS=Error}(MSG,...);
// FUNC TraceWarning{LEVEL=TRACE_LEVEL_WARNING,FLAGS=Warning}(MSG,...);
// FUNC TraceInformation{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=Information}(MSG,...);
// FUNC TraceVerbose{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=Verbose}(MSG,...);
// FUNC TracePerformance{PERF=DUMMY,LEVEL=TRACE_LEVEL_PERF}(FLAGS,MSG,...);
//
// FUNC TraceData{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=DataFlow}(MSG,...);
//
// FUNC TraceDriverStatus{LEVEL=TRACE_LEVEL_INFORMATION,FLAGS=DriverStatus}(MSG,...);
//
// end_wpp

// SENSOR ------------------------------------------------------------------------------------------------

// MACRO: SENSOR_FunctionEnter
//
// begin_wpp config
// USEPREFIX (SENSOR_FunctionEnter, "%!STDPREFIX! SENSOR %!FUNC! FunctionEnter");
// FUNC SENSOR_FunctionEnter{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=EntryExit}(...);
// end_wpp


// MACRO: SENSOR_FunctionExit
//
// begin_wpp config
// USEPREFIX (SENSOR_FunctionExit, "%!STDPREFIX! SENSOR %!FUNC! FunctionExit: %!STATUS!", __status);
// FUNC SENSOR_FunctionExit{LEVEL=TRACE_LEVEL_VERBOSE,FLAGS=EntryExit}(SENSOREXIT);
// end_wpp
#define WPP_LEVEL_FLAGS_SENSOREXIT_ENABLED(LEVEL, FLAGS, status)    WPP_LEVEL_FLAGS_ENABLED(LEVEL, FLAGS)
#define WPP_LEVEL_FLAGS_SENSOREXIT_LOGGER(LEVEL, FLAGS, status)     WPP_LEVEL_FLAGS_LOGGER(LEVEL, FLAGS)

#define WPP_LEVEL_FLAGS_SENSOREXIT_PRE(LEVEL, FLAGS, status)        {                                    \
                                                                        NTSTATUS __status = status;
#define WPP_LEVEL_FLAGS_SENSOREXIT_POST(LEVEL, FLAGS, status)           /*TraceMessage()*/;              \
                                                                    }



// WPP Recorder -------------------------------------------------------------------------------------------
//
// The following two macros are required to enable WPP Recorder functionality for clients of the Sensor Class Extension
//
#define WPP_RECORDER_LEVEL_FLAGS_SENSOREXIT_FILTER(LEVEL, FLAGS, status)   WPP_RECORDER_LEVEL_FLAGS_FILTER(LEVEL, FLAGS)
#define WPP_RECORDER_LEVEL_FLAGS_SENSOREXIT_ARGS(LEVEL, FLAGS, status)     WPP_RECORDER_LEVEL_FLAGS_ARGS(LEVEL, FLAGS)


#ifdef __cplusplus
}
#endif


