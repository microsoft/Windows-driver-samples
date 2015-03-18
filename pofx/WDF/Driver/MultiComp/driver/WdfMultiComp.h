/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    WdfMultiComp.h

Abstract:
    Header file for the KMDF sample driver for a multi-component device.

Environment:

    Kernel mode

--*/

#if !defined(_WDFMULTICOMP_H_)
#define _WDFMULTICOMP_H_
    

#include <ntddk.h>
#include <wdf.h>
#include "AppInterface.h"
#include "WdfPoFx.h"

//
// Some dummy values
//
#define TRANSITION_LATENCY_F1       WDF_ABS_TIMEOUT_IN_MS(5) // 5 milliseconds
#define RESIDENCY_REQUIREMENT_F1    WDF_ABS_TIMEOUT_IN_MS(50) // 50 milliseconds

//
// This structure contains the driver's power framework settings
//
typedef struct _PO_FX_DEVICE_MCOMP_EXT {
    PO_FX_DEVICE PoFxDevice;
    PO_FX_COMPONENT AdditionalComponents[COMPONENT_COUNT-1];
} PO_FX_DEVICE_MCOMP_EXT, *PPO_FX_DEVICE_MCOMP_EXT;

//
// This structure represents the driver's device context space
//
typedef struct _DEVICE_EXTENSION {
    //
    // Array of component queues
    //
    WDFQUEUE Queue[COMPONENT_COUNT];

    //
    // POHANDLE representing the device's registration with the power framework
    //
    POHANDLE PoHandle;
    
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, DeviceGetData)

//
// Driver's KMDF callbacks
//
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_OBJECT_CONTEXT_CLEANUP MCompEvtDriverCleanup;
EVT_WDF_DRIVER_DEVICE_ADD MCompEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MCompEvtIoDeviceControlPrimary;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MCompEvtIoDeviceControlSecondary;
EVT_WDF_DEVICE_D0_ENTRY MCompEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT MCompEvtDeviceD0Exit;

//
// Driver's power framework callbacks
//
PO_FX_COMPONENT_IDLE_STATE_CALLBACK MCompComponentIdleStateCallback;

//
// Driver's power framework helper callbacks
//
PFH_CALLBACK_POHANDLE_AVAILABLE MCompPoHandleAvailable;
PFH_CALLBACK_POHANDLE_UNAVAILABLE MCompPoHandleUnavailable;

//
// Define the tracing flags.
//
// TODO: Use a different trace control GUID below
//
#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        WdfMCompTraceControl, (C3061C7C,5719,4700,BD4D,A12B40347632),       \
        WPP_DEFINE_BIT(WDFMCOMP_ALL_INFO)                                   \
        )

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=WDFMCOMP_ALL_INFO}(LEVEL, MSG, ...);
// end_wpp
//    


#endif // _WDFMULTICOMP_H_
