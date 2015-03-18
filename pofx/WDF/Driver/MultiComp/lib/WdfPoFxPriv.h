/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    WdfPoFxPriv.h

Abstract:
    Private header file for the power framework helper library. Contains 
    structures and function headers that are used by various source files in the
    library.

Environment:

    Kernel mode

--*/
#if !defined(_WDFPOFXPRIV_H_)
#define _WDFPOFXPRIV_H_

#include <ntddk.h>
#include <ntintsafe.h>
#include "WdfPoFx.h"

#define DO_NOTHING()        (0)

//
// The following structure is used to capture the driver layer's settings for a
// device
//
typedef struct _HELPER_DEVICE_INIT {
    //
    // KMDF callbacks registered by the driver layer. The power framework helper
    // library invokes these callbacks when KMDF invokes that corresponding 
    // callback that the library has registered.
    //
    PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT EvtDeviceSelfManagedIoInit;
    PFN_WDF_DEVICE_SELF_MANAGED_IO_FLUSH EvtDeviceSelfManagedIoFlush;
    PFN_WDF_DEVICE_SELF_MANAGED_IO_RESTART EvtDeviceSelfManagedIoRestart;
    PFN_WDF_DEVICE_D0_ENTRY EvtDeviceD0Entry;

    //
    // Callbacks that notify the driver layer about registration and 
    // unregistration with the power framework. The power framework helper 
    // library invokes these callbacks after registering with the power 
    // framework and before unregistering from the power framework.
    //
    PPFH_CALLBACK_POHANDLE_AVAILABLE PfhCallbackPoHandleAvailable;
    PPFH_CALLBACK_POHANDLE_UNAVAILABLE PfhCallbackPoHandleUnavailable;

    //
    // The driver layer's WDM pre-process callback for power IRPs. The power 
    // framework helper library invokes this callback when KMDF invokes the 
    // corresponding callback that the library has registered.
    //
    PFN_WDFDEVICE_WDM_IRP_PREPROCESS EvtDeviceWdmPowerIrpPreprocess;

    //
    // Memory object containing the array of power IRP minor functions that the
    // driver layer is interested in preprocessing.
    //
    WDFMEMORY PowerIrpPreprocessMinorFunctions;

    //
    // The following members track the status of the initialization performed by
    // the driver layer for a given device. They are used in verification.
    //
    BOOLEAN PnpPowerEventCallbacksIntercepted;
    BOOLEAN PoHandleAvailabilityCallbacksSet;
    BOOLEAN PowerIrpPreprocessCallbackAssigned;
#if PFH_S0IDLE_SUPPORTED
    BOOLEAN S0IdleConfigSet;

    //
    // The driver's S0-idle configuration
    //
    PFH_S0IDLE_CONFIG S0IdleConfig;
#endif    

} HELPER_DEVICE_INIT, *PHELPER_DEVICE_INIT;

//
// The following structure is used to capture the driver layer's settings for a
// component queue
//
typedef struct _HELPER_QUEUE_INIT {
    //
    // Component number of the component that the queue is associated with
    //
    ULONG Component;

    //
    // The driver layer's EvtIoCanceledOnQueue callback for the component queue.
    // The power framework helper library invokes this callback when KMDF 
    // invokes the corresponding callback that the library has registered.
    //
    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE EvtIoCanceledOnQueue;

    //
    // The following members track the status of the initialization performed by
    // the driver layer for a given queue. They are used in verification.
    //
    BOOLEAN ComponentQueueConfigIntercepted;
    BOOLEAN ComponentSet;
} HELPER_QUEUE_INIT, *PHELPER_QUEUE_INIT;

//
// Enumeration of KMDF objects that can be initialized using the power framework 
// helper library's initializer object.
//
typedef enum _HELPER_INIT_TYPE {
    HelperInitTypeNone,
    HelperInitTypeDevice,
    HelperInitTypeQueue,
} HELPER_INIT_TYPE, *PHELPER_INIT_TYPE;

//
// The following structure represents the object context space for the power 
// framework helper library's initializer object.
//
typedef struct _HELPER_INIT {
    //
    // Type of KMDF object being initialized using the power framework helper
    // library's initializer object.
    //
    HELPER_INIT_TYPE InitType;
    union {
        //
        // Driver layer's device initialization settings
        //
        HELPER_DEVICE_INIT DeviceInit;

        //
        // Driver layer's queue initialization settings
        //
        HELPER_QUEUE_INIT QueueInit;
    } u;
} HELPER_INIT, *PHELPER_INIT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HELPER_INIT, 
                                   HelperGetInitContext)

//
// The following structure represents the array of power IRP minor functions 
// that the driver layer is interested in preprocessing
//
#pragma warning(push)
#pragma warning(disable:4200)
typedef struct _POFX_DRIVER_LAYER_POWER_IRP_PREPROCESS_INFO {
    //
    // Number of power IRP minor functions that the driver layer is interested
    // in preprocessing
    //
    ULONG NumMinorFunctions;

    //
    // Array of power IRP minor functions that the driver layer is interested in 
    // preprocessing
    //
    UCHAR MinorFunctions[];
} POFX_DRIVER_LAYER_POWER_IRP_PREPROCESS_INFO, 
        *PPOFX_DRIVER_LAYER_POWER_IRP_PREPROCESS_INFO;
#pragma warning(pop)

//
// Per-component information maintained by the power framework helper library
//
typedef struct _POFX_COMPONENT_INFO {
    //
    // Queue associated with the component
    //
    WDFQUEUE Queue;

    //
    // Whether or not the component is active
    //
    BOOLEAN IsActive;
} POFX_COMPONENT_INFO, *PPOFX_COMPONENT_INFO;

//
// The following structure represents the driver layer's power framework 
// callbacks. The power framework helper library invokes these callbacks when 
// the power framework invokes the corresponding callbacks that the library has 
// registered.
//
typedef struct _POFX_DRIVER_LAYER_POWER_CALLBACKS {
    PPO_FX_COMPONENT_IDLE_STATE_CALLBACK ComponentIdleStateCallback;
    PPO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK ComponentActiveConditionCallback;
    PPO_FX_COMPONENT_IDLE_CONDITION_CALLBACK ComponentIdleConditionCallback;
    PPO_FX_DEVICE_POWER_REQUIRED_CALLBACK DevicePowerRequiredCallback;                                            
    PPO_FX_DEVICE_POWER_NOT_REQUIRED_CALLBACK DevicePowerNotRequiredCallback;
} POFX_DRIVER_LAYER_POWER_CALLBACKS, *PPOFX_DRIVER_LAYER_POWER_CALLBACKS;

//
// The following structure represents the power framework helper library's 
// device context space.
//
typedef struct _POFX_DEVICE_CONTEXT {
    //
    // Pointer to the power framework settings registered by the power framework
    // helper library
    //
    PPO_FX_DEVICE PoFxDeviceInfo;

    //
    // Pointer to the driver layer's power framework callbacks
    //
    POFX_DRIVER_LAYER_POWER_CALLBACKS DriverLayerPoFxCallbacks;
    
    //
    // Pointer to the driver layer's context for power framework callbacks
    //
    PVOID DriverLayerPoFxContext;

    //
    // Pointer to the information that is tracked by the power framework helper
    // library for each component
    //
    PPOFX_COMPONENT_INFO ComponentInfo;

    //
    // POHANDLE representing the registration with the power framework
    //
    POHANDLE PoHandle;

    //
    // Array of power IRP minor functions that the driver layer is interested in
    // preprocessing.
    //
    PPOFX_DRIVER_LAYER_POWER_IRP_PREPROCESS_INFO 
                        DriverLayerPowerIrpPreprocessInfo;

    //
    // The driver layer's settings for the device
    //
    HELPER_DEVICE_INIT DeviceInitSettings;

    //
    // Whether or not we should call PoFxReportDevicePoweredOn from our next
    // EvtDeviceD0Entry callback
    //
    BOOLEAN ShouldReportDevicePoweredOn;

#if PFH_S0IDLE_SUPPORTED
    //
    // Whether or not we invoked WdfDeviceStopIdle during device start so that
    // the device remains in D0 until the power framework permits it to go to Dx
    //
    BOOLEAN StopIdleInvokedOnDeviceStart;

    //
    // Work item that is queued in response to device-power-required callback.
    // When the work item callback is invoked, we make a blocking call to 
    // WdfDeviceStopIdle in order to bring the device to D0. This blocking call
    // cannot be made within the device-power-required callback because it can 
    // be invoked at dispatch level. Therefore, we make this call at passive 
    // level from a work item callback.
    //
    WDFWORKITEM PowerRequiredWorkItem;
#endif    

} POFX_DEVICE_CONTEXT, *PPOFX_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(POFX_DEVICE_CONTEXT, HelperGetDeviceContext)

//
// The following structure represents the power framework helper library's 
// component queue context space.
//
typedef struct _POFX_QUEUE_CONTEXT {
    //
    // The driver layer's settings for the component queue
    //
    HELPER_QUEUE_INIT QueueInitSettings;
} POFX_QUEUE_CONTEXT, *PPOFX_QUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(POFX_QUEUE_CONTEXT, HelperGetQueueContext)

//
// Power framework helper library's KMDF callbacks
//
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT _PfhEvtSelfManagedIoInit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_FLUSH _PfhEvtSelfManagedIoFlush;
EVT_WDF_DEVICE_SELF_MANAGED_IO_RESTART _PfhEvtSelfManagedIoRestart;

EVT_WDF_DEVICE_D0_ENTRY _PfhEvtD0Entry;

EVT_WDFDEVICE_WDM_IRP_PREPROCESS _PfhEvtWdmPowerIrpPreprocess;

EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE _PfhEvtRequestCanceledOnComponentQueue;

//
// Power framework helper library's power framework callbacks
//
PO_FX_COMPONENT_IDLE_STATE_CALLBACK _PfhComponentIdleStateCallback;
PO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK _PfhComponentActiveConditionCallback;
PO_FX_COMPONENT_IDLE_CONDITION_CALLBACK _PfhComponentIdleConditionCallback;
PO_FX_DEVICE_POWER_REQUIRED_CALLBACK _PfhDevicePowerRequiredCallback;
PO_FX_DEVICE_POWER_NOT_REQUIRED_CALLBACK _PfhDevicePowerNotRequiredCallback;

//
// Routines used by source files within the power framework helper library
//
PHELPER_DEVICE_INIT
GetDeviceInitSettings(
    _In_ WDFOBJECT Initializer
    );

VOID
UnregisterWithPowerFrameworkWorker(
    _In_ WDFDEVICE Device
    );

BOOLEAN
IsDeviceInitialized(
    _In_ WDFDEVICE Device
    );
    
BOOLEAN
IsQueueInitialized(
    _In_ WDFQUEUE Queue
    );

BOOLEAN
ArePowerFrameworkSettingsAvailable(
    _In_ WDFDEVICE Device
    );

NTSTATUS
CopyAndUpdateMinorFunctionsArray(
    _In_ WDFOBJECT Initializer,
    _In_reads_opt_(NumMinorFunctions) PUCHAR MinorFunctions,
    _In_ ULONG NumMinorFunctions,
    _Out_ WDFMEMORY * MinorFunctionsMemory,
    _Outptr_result_buffer_(*UpdatedNumMinorFunctions) PUCHAR *UpdatedMinorFunctions,
    _Out_ PULONG UpdatedNumMinorFunctions
    );

NTSTATUS
CopyMinorFunctionsArray(
    _In_ WDFDEVICE Device,
    _In_ WDFMEMORY SourceMemory,
    _Out_ WDFMEMORY * DestinationMemory
    );
    
#include "s0idle.h"

//
// Define the tracing flags.
//
// The (0,0,0,0,0) GUID below is a dummy. The driver layer's GUID is the one 
// that gets used.
//
#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        WdfPoFxTraceControl, (0,0,0,0,0),                                   \
        WPP_DEFINE_BIT(WDFPOFX_ALL_INFO)                                    \
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
// FUNC Trace{FLAG=WDFPOFX_ALL_INFO}(LEVEL, MSG, ...);
// end_wpp
//    

#endif // _WDFPOFXPRIV_H_
