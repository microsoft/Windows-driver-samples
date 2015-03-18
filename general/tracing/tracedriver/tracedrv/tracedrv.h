/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    tracedrv.h

Abstract:

    tracedrv.h defines:
    - The provider GUID for the driver. 
    - Macros for tracing with levels and flags
    - Tracing enumerations using custom type
    - Trace macro that incorporates PRE/POST macros

Environment:

    Kernel mode

--*/


#define TRACEDRV_NT_DEVICE_NAME     L"\\Device\\TraceKmp"
#define TRACEDRV_WIN32_DEVICE_NAME  L"\\DosDevices\\TRACEKMP"

//
// Software Tracing Definitions 
//

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(CtlGuid,(d58c126f, b309, 11d1, 969e, 0000f875a5bc),  \
        WPP_DEFINE_BIT(FLAG_ONE)                \
        WPP_DEFINE_BIT(FLAG_TWO) )

//
// DoTraceLevelMessage is a custom macro that adds support for levels to the 
// default DoTraceMessage, which supports only flags. In this version, both
// flags and level are conditions for generating the trace message. 
// The preprocessor is told to recognize the function by using the -func argument
// in the RUN_WPP line on the source file. In the source file you will find
// -func:DoTraceLevelMessage(LEVEL,FLAGS,MSG,...). The conditions for triggering 
// this event in the macro are the Levels defined in evntrace.h and the flags 
// defined above and are evaluated by the macro WPP_LEVEL_FLAGS_ENABLED below. 
// 
#define WPP_LEVEL_FLAGS_LOGGER(level,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(level, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= level)

typedef enum _MachineState {
	Offline = 2,
	Online = 1,
	Failed = 0xFF000001,
	Stalled = 0xFF000002
} MachineState;
//
// Configuration block to scan the enumeration definition MachineState. Used when  
// viewing the trace to display names instead of the integer values that users must decode
//
// begin_wpp config
// CUSTOM_TYPE(state, ItemEnum(_MachineState));
// end_wpp


// MACRO: TRACE_RETURN
// Configuration block that defines trace macro. It uses the PRE/POST macros to include
// code as part of the trace macro expansion. TRACE_MACRO is equivalent to the code below:
//
// {if (Status != STATUS_SUCCESS){  // This is the code in the PRE macro
//     DoTraceMessage(FLAG_ONE, "Function Return = %!STATUS!", Status)
// ;}}                              // This is the code in the POST macro
//                                 
// 
// USEPREFIX statement: Defines a format string prefix to be used when logging the event, 
// below the STDPREFIX is used. The first value is the trace function name with out parenthesis
// and the second value is the format string to be used.
// 
// USESUFFIX statement: Defines a suffix format string that gets logged with the event. 
// 
// FUNC statement: Defines the name and signature of the trace function. The function defined 
// below takes one argument, no format string, and predefines the flag equal to FLAG_ONE.
//
//
//begin_wpp config
//USEPREFIX (TRACE_RETURN, "%!STDPREFIX!");
//FUNC TRACE_RETURN{FLAG=FLAG_ONE}(EXP);
//USESUFFIX (TRACE_RETURN, "Function Return=%!STATUS!",EXP);
//end_wpp

//
// PRE macro: The name of the macro includes the condition arguments FLAGS and EXP
//            define in FUNC above
//
#define WPP_FLAG_EXP_PRE(FLAGS, HR) {if (HR != STATUS_SUCCESS) {

//
// POST macro
// The name of the macro includes the condition arguments FLAGS and EXP
//            define in FUNC above
#define WPP_FLAG_EXP_POST(FLAGS, HR) ;}}

// 
// The two macros below are for checking if the event should be logged and for 
// choosing the logger handle to use when calling the ETW trace API
//
#define WPP_FLAG_EXP_ENABLED(FLAGS, HR) WPP_FLAG_ENABLED(FLAGS)
#define WPP_FLAG_EXP_LOGGER(FLAGS, HR) WPP_FLAG_LOGGER(FLAGS)


