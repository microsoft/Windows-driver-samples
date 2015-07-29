/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    trace.h

Abstract:

    This module contains the required definitions to enable WppRecorder
    for the KMDF Toaster driver sample.

Environment:

    Windows Kernel-Mode Driver Framework (KMDF)

--*/

#pragma once

// {32134778-E6AA-4089-B5A8-9E58E9EE4687}
#define WPP_CONTROL_GUIDS                                          \
    WPP_DEFINE_CONTROL_GUID(                                       \
    ToasterSampleKMDF,                                             \
    (32134778,E6AA,4089,B5A8,9E58E9EE4687),                        \
    WPP_DEFINE_BIT(TRACE_FLAG_DRIVER)                              \
    WPP_DEFINE_BIT(TRACE_FLAG_DEVICE)                              \
    )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level  >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our Trace function.
//
// These trace functions, located between "begin_wpp config" and "end_wpp", will define the functions
//	to call to print traces. The "begin_wpp config" and "end_wpp" lines must be present or else
//	the WPP Preprocessor will not pick up your trace functions.
//
// Notice the double parenthesis in the parameters of the function KdPrint. This is only done to ensure
//  backward compatibility with the regular KdPrint. Future functions should not use double parenthesis
//  for readability.
//
// The "WPP_FLAGS(-public:<trace macro>);" lines make TMF information appear in the public PDBs so that 
// those with the public PDB can still see the traces. Remove these lines if you do not wish to have
// the public viewing the trace messages.
//
// Notice the IFRLOG parameter in WppPrintDevice and WppPrintDeviceError. Adding that parameter here,
//  allows traces statements to specify which WppRecorder Log to print to.
//
// begin_wpp config
//
// FUNC KdPrint{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=TRACE_FLAG_DRIVER}((MSG, ...));
// WPP_FLAGS(-public:KdPrint);
//
// FUNC WppPrintDevice{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=TRACE_FLAG_DEVICE}(IFRLOG, MSG, ...);
// WPP_FLAGS(-public:WppPrintDevice);
//
// FUNC WppPrintDeviceError{LEVEL=TRACE_LEVEL_ERROR, FLAGS=TRACE_FLAG_DEVICE}(IFRLOG, MSG, ...);
// WPP_FLAGS(-public:WppPrintDeviceError);
//
// end_wpp
//


