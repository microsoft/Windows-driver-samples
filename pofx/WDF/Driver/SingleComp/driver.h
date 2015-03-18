/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Driver.h

Abstract:
    Header file for the KMDF sample driver for a single-component device.

Environment:

    Kernel mode

--*/

#define QUEUE_COUNT     3
#define FSTATE_COUNT    4
#define DEEPEST_FSTATE_LATENCY_IN_MS    800
#define DEEPEST_FSTATE_RESIDENCY_IN_SEC 12

//
// This structure represents the driver's device context space
//
typedef struct _FDO_DATA
{
    //
    // Handle of registration with Power Framework
    //
    POHANDLE PoHandle;

    //
    // Power-managed queues used for the device.
    // We track them here so that we can stop/start them on active/idle
    // transitions.
    // 
    WDFQUEUE Queues[QUEUE_COUNT];

    //
    // Count of queues to be stopped.
    //
    // It is initialized to QUEUE_COUNT at the beginning of a transition to an
    // idle condition. It is decremented everytime a queue completes stop 
    // transition.
    //
    // When this count reaches 0 we can transition device into an idle 
    // condition.
    //
    LONG QueueStopCount;

    //
    // Tracks the active/idle state of the component
    //
    BOOLEAN IsActive;
} FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, FdoGetContext)

//
// Driver's KMDF callbacks
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD           SingleCompEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP      SingleCompEvtDriverCleanup;

EVT_WDF_DEVICE_D0_ENTRY             SingleCompEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT              SingleCompEvtDeviceD0Exit;


EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  SingleCompEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_READ            SingleCompEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE           SingleCompEvtIoWrite;
EVT_WDF_IO_QUEUE_STATE              SingleCompEvtQueueStopComplete;

EVT_WDFDEVICE_WDM_POST_PO_FX_REGISTER_DEVICE    SingleCompWdmEvtDeviceWdmPostPoFxRegisterDevice;
EVT_WDFDEVICE_WDM_PRE_PO_FX_UNREGISTER_DEVICE   SingleCompWdmEvtDeviceWdmPrePoFxUnregisterDevice;

//
// Driver's power framework callbacks
//
PO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK SingleCompWdmActiveConditionCallback;
PO_FX_COMPONENT_IDLE_CONDITION_CALLBACK SingleCompWdmIdleConditionCallback;
PO_FX_COMPONENT_IDLE_STATE_CALLBACK SingleCompWdmIdleStateCallback;

//
// Helper functions
// 

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS 
AssignS0IdleSettings(
    _In_ WDFDEVICE Device
    );

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS 
AssignPowerFrameworkSettings(
    _In_ WDFDEVICE Device
    );

_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
F0Entry(
    _In_ WDFDEVICE Device
    );

_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
F0Exit(
    _In_ WDFDEVICE Device,
    _In_ ULONG State
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
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// end_wpp
//    
