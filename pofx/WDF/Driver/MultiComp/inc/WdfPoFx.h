/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    WdfPoFx.h

Abstract:
    This header file describes the interface between the driver layer and the
    power framework helper library.

Environment:

    Kernel mode

--*/
#if !defined(_WDFPOFX_H_)
#define _WDFPOFX_H_
    
#include <ntddk.h>
#include <wdf.h>

//
// Routine description:
//      This routine is invoked by the power framework helper library to notify
//      the driver layer that it has registered with the power framework. The 
//      driver layer can provide this callback to the power framework helper 
//      library by calling PfhSetPoHandleAvailabilityCallbacks. The power 
//      framework helper library invokes this routine during the processing of 
//      the first start IRP for the device. (Note that the device might receive
//      more than one start IRP if a resource rebalance occurs while it is 
//      operational).
//
// Arguments:
//      Device - Handle to the KMDF device object
//
//      PoHandle - Handle that represents the device's registration with the
//          power framework.
//
// Return value:
//      An NTSTATUS value representing success or failure of the function. If 
//      the driver layer returns an NTSTATUS value representing failure, the 
//      power framework helper library immediately unregisters with the power
//      framework and fails the start IRP.
//
typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PFH_CALLBACK_POHANDLE_AVAILABLE(
    _In_ WDFDEVICE Device,
    _In_ POHANDLE PoHandle
    );

typedef PFH_CALLBACK_POHANDLE_AVAILABLE *PPFH_CALLBACK_POHANDLE_AVAILABLE;

//
// Routine description:
//      This routine is invoked by the power framework helper library to notify
//      the driver layer that it is about to unregister with the power 
//      framework. The driver layer can provide this callback to the power 
//      framework helper library by calling 
//      PfhSetPoHandleAvailabilityCallbacks. The POHANDLE representing the 
//      device's registration with the power framework is not guaranteed to be 
//      valid after the driver layer has returned from this routine. Therefore, 
//      the driver layer must stop using the POHANDLE before it returns from 
//      this routine. 
//
// Arguments:
//      Device - Handle to the KMDF device object
//
//      PoHandle - Handle that represents the device's registration with the
//          power framework.
//
// Return value:
//      None
//
typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
PFH_CALLBACK_POHANDLE_UNAVAILABLE(
    _In_ WDFDEVICE Device,
    _In_ POHANDLE PoHandle
    );

typedef PFH_CALLBACK_POHANDLE_UNAVAILABLE *PPFH_CALLBACK_POHANDLE_UNAVAILABLE;

//
// PfhInitializerCreate
// ====================
// Routine description:
//      This routine is invoked by the driver layer to create a power framework
//      helper library initializer object. This initializer object helps the
//      power framework library to initialize its own settings for KMDF objects.
//      After creating the initializer object, the driver layer passes it in 
//      as an argument to several initialization methods in the power framework
//      helper library.
//
//      If the driver layer calls any of the following methods when creating a 
//      KMDF device object, the initializer object needs to be passed in as an
//      argument:
//          PfhAssignWdmPowerIrpPreProcessCallback
//          PfhInterceptWdfPnpPowerEventCallbacks
//          PfhSetPoHandleAvailabilityCallbacks
//          PfhInitializeDeviceSettings
//
//      If the driver layer calls any of the following methods when creating a 
//      KMDF queue object, the initializer object needs to be passed in as an
//      argument:
//          PfhInterceptComponentQueueConfig
//          PfhSetComponentForComponentQueue
//          PfhInitializeComponentQueueSettings
//
//      The same initializer object can be used to initialize more than one KMDF
//      object, but at any given time it must be used to initialize only one 
//      KMDF object. For example, the driver layer could use an initializer 
//      object to initialize a KMDF device object and when it is done 
//      initializing the device object, the same object can be used to 
//      initialize a different KMDF object (not necessarily a device object).
//
//      The initializer object is a child of the KMDF driver object and is 
//      deleted by KMDF when the driver object is deleted. This means that by 
//      default the initializer object remains in memory until the driver is 
//      unloaded. However, the driver layer can proactively delete the 
//      initializer object using the WdfObjectDelete method when it has finished
//      using it.
//
// Arguments:
//      Initializer - Pointer to a location that receives a handle to the new
//          initializer object.
//
// Return value:
//      An NTSTATUS value representing success or failure of the function.
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhInitializerCreate(
    _Out_ WDFOBJECT * Initializer
    );

//
// PfhAssignWdmPowerIrpPreProcessCallback
// ======================================
// Routine description:
//      This routine is invoked by the driver layer to enable the power 
//      framework helper library to register a WDM preprocess callback function
//      to handle power IRPs before KMDF has a chance to process them. The 
//      driver layer must invoke this routine before creating the KMDF device 
//      object. 
//
//      This routine must be invoked for any device whose power management is 
//      done using the power framework helper library, even if the driver layer
//      is not interested in preprocessing power IRPs for that device.
//
//      If the driver layer also wishes to pre-process power IRPs before KMDF
//      has a chance to preprocess them, it can specify this via the arguments
//      to this routine. The driver layer must *not* directly call the 
//      WdfDeviceInitAssignWdmIrpPreprocessCallback method to register a pre-
//      process callback routine for power IRPs, i.e. IRPs with a major function
//      code of IRP_MJ_POWER. Instead it must use 
//      PfhAssignWdmPowerIrpPreProcessCallback to accomplish this. However, for
//      preprocessing IRPs with other major function codes, the driver should 
//      use WdfDeviceInitAssignWdmIrpPreprocessCallback.
//
//      If the driver layer specifies a preprocess callback via 
//      PfhAssignWdmPowerIrpPreProcessCallback, it gets to preprocess the power
//      IRP before the power framework helper library preprocesses it. Once the
//      driver layer is done preprocessing the power IRP, it must call
//      PfhWdmDispatchPreprocessedPowerIrp to allow the power framework helper
//      library and eventually KMDF to process the IRP. The driver layer must 
//      *not* directly call WdfDeviceWdmDispatchPreprocessedIrp to forward a
//      pre-processed power IRP, i.e. an IRP with major function code 
//      IRP_MJ_POWER, to KMDF. Preprocessed IRPs with other major function codes
//      should be forwarded to KMDF via WdfDeviceWdmDispatchPreprocessedIrp.
//
// Arguments:
//      Initializer - Handle to the initializer object that is being used to 
//          initialize the power framework helper library's device object 
//          settings.
//
//      DeviceInit - Pointer to a WDFDEVICE_INIT structure
//
//      EvtDeviceWdmPowerIrpPreprocess - Pointer to the driver layer's 
//          EVT_WDFDEVICE_WDM_IRP_PREPROCESS callback function for power IRPs. 
//          This argument is optional. If the driver layer does not want to 
//          preprocess power IRPs, it can specify NULL.
//
//      MinorFunctions - Pointer to an array of one or more minor function codes
//          for IRP_MJ_POWER that the driver layer wants to pre-process. This 
//          argument is optional. If the driver layer does not want to 
//          preprocess power IRPs, it can specify NULL.
//
//      NumMinorFunctions - Number of minor function codes that are contained in
//          the MinorFunctions array.
//
// Return value:
//      An NTSTATUS value representing success or failure of the function.
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhAssignWdmPowerIrpPreProcessCallback(
    _In_ WDFOBJECT Initializer,
    _In_ PWDFDEVICE_INIT DeviceInit,
    _In_opt_ PFN_WDFDEVICE_WDM_IRP_PREPROCESS EvtDeviceWdmPowerIrpPreprocess,
    _In_reads_opt_(NumMinorFunctions) PUCHAR MinorFunctions,
    _In_ ULONG NumMinorFunctions
    );

//
// PfhWdmDispatchPreprocessedPowerIrp
// ==================================
// Routine description:
//      This routine is invoked by the driver layer to forward a pre-processed
//      power IRP to the power framework helper library. It is similar to KMDF's
//      WdfDeviceWdmDispatchPreprocessedIrp method. For power IRPs, the driver 
//      layer must use PfhWdmDispatchPreprocessedPowerIrp instead of 
//      WdfDeviceWdmDispatchPreprocessedIrp. The power framework helper library 
//      eventually forwards it to KMDF after pre-processing it.
//
// Arguments:
//      Device - Handle to a KMDF device object
//
//      Irp - Pointer to the pre-processed IRP
//
// Return value:
//      An NTSTATUS value that the power framework helper library provides as a
//      result of processing the IRP. The driver *must* use this return value as
//      return value for its EvtDeviceWdmPowerIrpPreprocess callback function.
//
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PfhWdmDispatchPreprocessedPowerIrp(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp
    );

//
// PfhInterceptWdfPnpPowerEventCallbacks
// =====================================
// Routine description:
//      This routine is invoked by the driver layer to enable the power 
//      framework helper library to register its PNP and power event callbacks
//      with KMDF. If the driver layer has specified a callback for a PNP or 
//      power event that the power framework helper library is also interested 
//      in, then the power framework helper library saves the driver layer's 
//      callback and invokes it from when KMDF invokes its own callback. The
//      driver layer must call PfhInterceptWdfPnpPowerEventCallbacks after it 
//      has initialized the WDF_PNPPOWER_EVENT_CALLBACKS structure but before it
//      has called WdfDeviceInitSetPnpPowerEventCallbacks.
//
//      This routine must be invoked for any device whose power management is 
//      done using the power framework helper library, even if the driver layer
//      does not want to specify any PNP or power event callbacks for that 
//      device. After invoking this routine, the driver layer must invoke 
//      WdfDeviceInitSetPnpPowerEventCallbacks so that the power framework 
//      helper library's callbacks are registered with KMDF.
//
// Arguments:
//      Initializer - Handle to the initializer object that is being used to 
//          initialize the power framework helper library's device object 
//          settings.
//
//      DriverLayerPnpPowerCallbacks - Pointer to a caller-initialized 
//          WDF_PNPPOWER_EVENT_CALLBACKS structure.
//
// Return value:
//      None
//
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
PfhInterceptWdfPnpPowerEventCallbacks(
    _In_ WDFOBJECT Initializer,
    _Inout_ PWDF_PNPPOWER_EVENT_CALLBACKS DriverLayerPnpPowerCallbacks
    );
    
//
// PfhSetPoHandleAvailabilityCallbacks
// ===================================
// Routine description:
//      This routine is invoked by the driver layer to receive the POHANDLE that
//      represents the device's registration with the power framework and to be
//      notified when the handle is about to become invalid because of an 
//      impending unregistration. The POHANDLE received in this manner can be 
//      used to directly invoke power framework routines. However, the driver 
//      layer must *not* invoke the following power framework routines because 
//      they can interfere with the operation of the power framework helper 
//      library:
//          PoFxStartDevicePowerManagement
//          PoFxUnregisterDevice 
//
//      PfhSetPoHandleAvailabilityCallbacks must be invoked before the driver
//      layer has created the KMDF device object. The driver layer need not 
//      invoke this routine if it is not interested in using the POHANDLE 
//      directly.
//
// Arguments:
//      Initializer - Handle to the initializer object that is being used to 
//          initialize the power framework helper library's device object 
//          settings.
//
//      EvtPfhPoHandleAvailable - Pointer to the driver layer's 
//          PFH_CALLBACK_POHANDLE_AVAILABLE callback function.
//
//      EvtPfhPoHandleUnavailable - Pointer to the driver layer's 
//          PFH_CALLBACK_POHANDLE_UNAVAILABLE callback function.
//
// Return value:
//      None
//
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
PfhSetPoHandleAvailabilityCallbacks(
    _In_ WDFOBJECT Initializer,
    _In_ PPFH_CALLBACK_POHANDLE_AVAILABLE PfhCallbackPoHandleAvailable,
    _In_ PPFH_CALLBACK_POHANDLE_UNAVAILABLE PfhCallbackPoHandleUnavailable
    );

//
// The following enum defines the S0-idle power management configurations that 
// are supported by the power framework helper library
//
typedef enum _PFH_S0IDLE_CONFIG {
    //
    // S0-idle power management is not supported for the device
    //
    PfhS0IdleNotSupported,

    //
    // S0-idle power management is supported for the device and the driver can 
    // access pageable data during the device's power transitions. This implies
    // that the device can *never* be in the paging path.
    //
    PfhS0IdleSupportedPowerPageable,
    
    //
    // S0-idle power management is supported for the device and the driver 
    // *cannot* access pageable data during the device's power transitions. This
    // implies that the device can be in the paging path.
    //
    // If this configuration is chosen, the driver layer is responsible for 
    // supplying a PO_FX_DEVICE_POWER_REQUIRED_CALLBACK callback. This callback 
    // will be invoked when the power framework instructs the driver to power up
    // the device. In this callback, the driver layer must initiate device power
    // up. When the device has powered up, the driver layer must call 
    // PoFxReportDevicePoweredOn to inform the power framework about it. The 
    // driver layer should accomplish this in the following manner:
    //    - In the PO_FX_DEVICE_POWER_REQUIRED_CALLBACK callback it should queue
    //      work to its own worker thread (and not a system worker thread) to 
    //      make a blocking call to WdfDeviceStopIdle. Since this is a blocking
    //      call, the driver layer cannot make it directly from within 
    //      PO_FX_DEVICE_POWER_REQUIRED_CALLBACK for a couple of reasons. First,
    //      the PO_FX_DEVICE_POWER_REQUIRED_CALLBACK callback can be invoked at
    //      dispatch level. Second, the PO_FX_DEVICE_POWER_REQUIRED_CALLBACK
    //      callback might get invoked in the context of the I/O dispatch 
    //      routine of a power-managed queue and a blocking call to 
    //      WdfDeviceStopIdle in such a context can lead to a deadlock. Hence,
    //      the blocking call to WdfDeviceStopIdle must be made from a worker 
    //      thread. The driver cannot use a system worker thread because queuing
    //      a system work item to the system worker thread can cause pageable 
    //      data to be accessed. But by definition, pageable data cannot be 
    //      accessed in the PfhS0IdleSupportedNotPowerPageable configuration. 
    //      Hence the need for a driver-created worker thread.
    //    - Once the blocking call to WdfDeviceStopIdle has returned, the device
    //      is guaranteed to have returned to D0 and/or to remain in D0.
    //      Therefore the driver layer should call PoFxReportDevicePoweedOn at
    //      this point. If the call to WdfDeviceStopIdle failed, KMDF would have
    //      already declared the device to be in a failed state and initiated a
    //      PNP removal. However, even in the failure case, it is necessary to
    //      call PoFxReportDevicePoweredOn in order to unblock the power 
    //      framework and move it to a consistent state.
    //
    PfhS0IdleSupportedNotPowerPageable
    
} PFH_S0IDLE_CONFIG, *PPFH_S0IDLE_CONFIG;

//
// PfhSetS0IdleConfiguration
// =========================
// Routine description:
//      This routine is invoked by the driver layer to specify whether or not
//      the device supports S0-idle power management. PfhSetS0IdleConfiguration
//      must be invoked before the driver layer has created the KMDF device 
//      object. If the driver layer does not invoke this routine, the power 
//      framework helper library assumes the device's S0-idle power management 
//      configuration to be PfhS0IdleNotSupported.
//
// Arguments:
//      Initializer - Handle to the initializer object that is being used to 
//          initialize the power framework helper library's device object 
//          settings.
//      S0IdleConfig - A PFH_S0IDLE_CONFIG value that specifies the device's
//          S0-idle power management configuration.
//
// Return value:
//      None
//
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
PfhSetS0IdleConfiguration(
    _In_ WDFOBJECT Initializer,
    _In_ PFH_S0IDLE_CONFIG S0IdleConfig
    );

//
// PfhInitializeDeviceSettings
// ===========================
// Routine description:
//      This routine is invoked by the driver layer to enable the power 
//      framework helper library to initialize the latter's settings for a KMDF
//      device object. The driver layer must invoke this routine for any KMDF
//      device object whose power management is done using the power framework
//      helper library. 
//
//      This routine must be invoked after the KMDF device object has been 
//      created and before the initializer object that is being used to 
//      initialize the device object is reused to initialize some other KMDF 
//      object. After calling this routine, the initializer object can be used 
//      for initializing other KMDF objects if needed.
//
// Arguments:
//      Device - Handle to the KMDF device object
//
//      Initializer - Handle to the initializer object that is being used to 
//          initialize the power framework helper library's device object 
//          settings.
//
// Return value:
//      An NTSTATUS value representing success or failure of the function.
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhInitializeDeviceSettings(
    _In_ WDFDEVICE Device,
    _In_ WDFOBJECT Initializer
    );

//
// PfhInitializePowerFrameworkSettings
// ===================================
// Routine description:
//      This routine is invoked by the driver layer to specify its power 
//      framework settings. These settings are used by the power framework 
//      helper library when it registers with the power framework.
//      
//      This routine must be invoked before the driver layer's 
//      EvtDeviceSelfManagedIoInit callback is invoked or from within the driver
//      layer's EvtDeviceSelfManagedIoInit callback. Typically, the driver layer
//      would invoke this routine from with its EvtDriverDeviceAdd callback or
//      from one of the following callbacks that are invoked the first time a 
//      device is being started:
//          EvtDevicePrepareHardware
//          EvtDeviceD0Entry
//          EvtDeviceD0EntryPostInterruptsEnabled
//          EvtDeviceSelfManagedIoInit
//
//      The driver layer must not invoke PfhInitializePowerFrameworkSettings 
//      more than once for a given device. Note that a device might be started 
//      more than once if a resource rebalance occurs while the device is 
//      operational. In this case, the EvtDevicePrepareHardware, 
//      EvtDeviceD0Entry and EvtDeviceD0EntryPostInterruptsEnabled callbacks can
//      be invoked more than once. Therefore, if the driver layer chooses to 
//      call PfhInitializePowerFrameworkSettings from within one of these 
//      callbacks, it must take care to call it only when the callback is 
//      invoked for the first time.
//
// Arguments:
//      Device - Handle to the KMDF device object
//
//      PoFxDeviceInfo - Pointer to a caller-initialized 
//          PO_FX_DEVICE structure that contains the driver layer's power 
//          framework settings.
//
// Return value:
//      An NTSTATUS value representing success or failure of the function.
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhInitializePowerFrameworkSettings(
    _In_ WDFDEVICE Device,
    _In_ PPO_FX_DEVICE PoFxDeviceInfo
    );

//
// PfhInterceptComponentQueueConfig
// ================================
// Routine description:
//      This routine is invoked by the driver layer to enable the power 
//      framework helper library to register its queue callbacks with KMDF. If
//      the driver layer has specified a queue callback that the power framework
//      helper library is also interested in, then the power framework helper 
//      library saves the driver layer's callback and invokes it from when KMDF
//      invokes its own callback. The driver layer must call 
//      PfhInterceptComponentQueueConfig after it has initialized the 
//      WDF_IO_QUEUE_CONFIG structure but before it has created the KMDF queue 
//      object. 
//
//      This routine must be invoked for each of the driver layer's component 
//      queues. Component queues are described in the routine description for 
//      PfhInitializeComponentQueueSettings. 
//
// Arguments:
//      Initializer - Handle to the initializer object that is being used to 
//          initialize the power framework helper library's queue object 
//          settings.
//
//      DriverLayerQueueConfig - Pointer to a caller-initialized 
//          WDF_IO_QUEUE_CONFIG structure.
//
// Return value:
//      None
//
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PfhInterceptComponentQueueConfig(
    _In_ WDFOBJECT Initializer,
    _Inout_ PWDF_IO_QUEUE_CONFIG DriverLayerQueueConfig
    );

//
// PfhSetComponentForComponentQueue
// ================================
// Routine description:
//      This routine is invoked by the driver layer to specify the component
//      that is associated with a component queue. The current implementation of
//      the power framework helper library does not support associating a queue
//      with more than one component, so this routine must be called no more 
//      than once for a given queue.
//
//      This routine must be invoked for each queue that acts as a component 
//      queue. Component queues are described in the routine description for 
//      PfhInitializeComponentQueueSettings. 
//
// Arguments:
//      Initializer - Handle to the initializer object that is being used to 
//          initialize the power framework helper library's queue object 
//          settings.
//
//      Component - Component associated with the queue.
//
// Return value:
//      None
//
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PfhSetComponentForComponentQueue(
    _In_ WDFOBJECT Initializer,
    _In_ ULONG Component
    );

//
// PfhInitializeComponentQueueSettings
// ===================================
// Routine description:
//      This routine is invoked by the driver layer to enable the power 
//      framework helper library to initialize the latter's settings for a KMDF
//      queue object. 
//
//      The driver layer must invoke this routine for any KMDF queue object that
//      acts as a component queue. A component queue is a queue whose state is 
//      tied to the active/idle state for a component. The queue can dispatch 
//      requests when the component is active and cannot dispatch requests when
//      the component is idle. A component queue must be power-managed. The 
//      driver layer must not explicitly change the state of any of its 
//      component queues.
//
//      This routine must be invoked after the KMDF queue object has been 
//      created and before the initializer object that is being used to 
//      initialize the queue object is reused to initialize some other KMDF 
//      object. After calling this routine, the initializer object can be used
//      for initializing other KMDF objects if needed.
//
// Arguments:
//      Queue - Handle to the KMDF queue object for the component queue
//
//      Initializer - Handle to the initializer object that is being used to 
//          initialize the power framework helper library's queue object 
//          settings.
//
// Return value:
//      An NTSTATUS value representing success or failure of the function.
//
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PfhInitializeComponentQueueSettings(
    _In_ WDFQUEUE Queue,
    _In_ WDFOBJECT Initializer
    );

//
// PfhRegisterDeviceProactive
// ==========================
// Routine description:
//      This routine is invoked by the driver layer to in order to proactively
//      register with the power framework. By default, the power framework 
//      helper library registers with the power framework in its 
//      EvtDeviceSelfManagedIoInit allback, after it has invoked the driver 
//      layer's EvtDeviceSelfManagedIoInit callback (if present). If the driver
//      layer is satisfied with this default behavior, it need not invoke 
//      PfhRegisterDeviceProactive at all. 
//
//      However, if the driver layer needs to interact with the power framework
//      before the point at which the power framework helper library registers 
//      by default, it can proactively register by calling 
//      PfhRegisterDeviceProactive. Before calling PfhRegisterDeviceProactive, 
//      the driver layer must ensure that it has already provided its power 
//      framework settings to the power framework helper library by calling 
//      PfhInitializePowerFrameworkSettings.
//
//      The power framework helper library unregisters with the power framework
//      helper library during device removal, in the EvtDeviceSelfManagedIoFlush
//      callback. 
//
//      The driver layer must ensure that it does not call 
//      PfhRegisterDeviceProactive if the device is already registered with 
//      the power framework. This means that if the driver layer calls 
//      PfhRegisterDeviceProactive in one of the following callbacks, it needs
//      to handle cases where the callback is called more than once. 
//          EvtDevicePrepareHardware
//          EvtDeviceD0Entry
//          EvtDeviceD0EntryPostInterruptsEnabled
//
//      If the device is being started for the first time or if the device is a 
//      PDO that is being restarted after being removed, the above callbacks 
//      would get invoked while the device is not yet registered with the power
//      framework.
//
//      If the device is being restarted after being stopped for resource 
//      rebalance, the above callbacks would get invoked while the device is 
//      already registered with the power framework.
//
//      If the device returns to the working state (D0) after being in a low-
//      power state (Dx), then EvtDeviceD0Entry and 
//      EvtDeviceD0EntryPostInterruptsEnabled would get invoked while the device
//      is already registered with the power framework (and 
//      EvtDevicePrepareHardware is not invoked at all).
//
//      Therefore, the driver layer must be careful to invoke 
//      PfhRegisterDeviceProactive only if the device is not already registered
//      with the power framework. It can determine whether or not the device is 
//      already registered by supplying the PFH_CALLBACK_POHANDLE_AVAILABLE and 
//      PFH_CALLBACK_POHANDLE_UNAVAILABLE callbacks via 
//      PfhSetPoHandleAvailabilityCallbacks.
//
//      Note that even if the driver layer invokes PfhRegisterDeviceProactive 
//      more than once, it must invoke PfhInitializePowerFrameworkSettings only 
//      once. The power framework settings that are supplied once get used for 
//      subsequent registrations as well.
//
// Arguments:
//      Device - Handle to the KMDF device object
//
// Return value:
//      An NTSTATUS value representing success or failure of the function.
//
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhRegisterDeviceProactive(
    _In_ WDFDEVICE Device
    );

//
// PfhForwardRequestToQueue
// ========================
// Routine description:
//      This routine is invoked by the driver layer to forward a request to an
//      I/O queue. It is similar to the WdfRequestForwardToIoQueue method. At a
//      minimum, the driver layer must use PfhForwardRequestToQueue instead
//      of WdfRequestForwardToIoQueue when forwarding a request to or from a 
//      component queue. However, the driver layer can use 
//      PfhForwardRequestToQueue to forward requests between any two queues.
//      The driver layer might find it convenient to always use 
//      PfhForwardRequestToQueue instead of using it only in some places and 
//      using WdfRequestForwardToIoQueue in others.
//
// Arguments:
//      Request - Handle to the KMDF request object for the request that is to 
//          be forwarded.
//
//      Queue - Handle to the KMDF queue object for the destination queue
//
// Return value:
//      None
//
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PfhForwardRequestToQueue(
    _In_ WDFREQUEST Request,
    _In_ WDFQUEUE Queue
    );

//
// PfhCompleteRequest
// ==================
// Routine description:
//      This routine is invoked by the driver layer to complete a request. It is
//      similar to the WdfRequestCompleteWithInformation method. At a minimum, 
//      the driver layer must use PfhCompleteRequest instead of 
//      WdfRequestComplete/WdfRequestCompleteWithInformation when it is 
//      completing a request that is associated with a component queue. However,
//      the driver layer can use PfhCompleteRequest to complete any request.
//      The driver layer might find it convenient to always use 
//      PfhCompleteRequest instead of using it only in some places and using
//      WdfRequestComplete/WdfRequestCompleteWithInformation in others.
//
// Arguments:
//      Request - Handle to the KMDF request object for the request that is to 
//          be completed.
//
//      Status - An NTSTATUS value that is used as the completion status of the 
//          request.
//
//      Information - Driver-defined completion status information for the 
//          request, such as the number of bytes that were transferred.
//
// Return value:
//      None
//
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PfhCompleteRequest(
    _In_ WDFREQUEST Request,
    _In_ NTSTATUS Status,
    _In_ ULONG_PTR Information
    );

#endif // _WDFPOFX_H_
