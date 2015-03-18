/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Driver.h

Abstract:
    Header file for the UMDF2 sample driver for a single-component device.

Environment:

    User mode

--*/

#define QUEUE_COUNT     3

//
// This structure represents the driver's device context space
//
typedef struct _FDO_DATA
{
    //
    // Tracks the active/idle state of the component
    //
    BOOLEAN IsActive;
} FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, FdoGetContext)

//
// Driver's UMDF2 callbacks
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD           SingleCompEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP      SingleCompEvtDriverCleanup;

EVT_WDF_DEVICE_D0_ENTRY             SingleCompEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT              SingleCompEvtDeviceD0Exit;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  SingleCompEvtIoDeviceControl;

//
// Helper functions
// 

NTSTATUS 
AssignS0IdleSettings(
    _In_ WDFDEVICE Device
    );

//
// Define the tracing flags.
//
#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        MyDriverTraceControl, (f9eb5c3a,c292,4c69,8b52,ccf043c25ab0),       \
                                                                            \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                                   \
        )

#define WPP_FLAGS_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAGS_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAGS=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// end_wpp
//    
