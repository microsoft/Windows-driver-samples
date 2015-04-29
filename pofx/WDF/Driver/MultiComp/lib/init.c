/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    init.c

Abstract:
    This module contains routines that are used to initialize the power 
    framework helper library

Environment:

    Kernel mode

--*/

#include "WdfPoFxPriv.h"
#include "init.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GetDeviceInitSettings)
#pragma alloc_text(PAGE, PfhInitializerCreate)
#pragma alloc_text(PAGE, CopyAndUpdateMinorFunctionsArray)
#pragma alloc_text(PAGE, PfhAssignWdmPowerIrpPreProcessCallback)
#pragma alloc_text(PAGE, PfhInterceptWdfPnpPowerEventCallbacks)
#pragma alloc_text(PAGE, PfhSetPoHandleAvailabilityCallbacks)
#pragma alloc_text(PAGE, CopyMinorFunctionsArray)
#pragma alloc_text(PAGE, PfhInitializeDeviceSettings)
#pragma alloc_text(PAGE, PfhInitializePowerFrameworkSettings)
#endif

PHELPER_DEVICE_INIT
GetDeviceInitSettings(
    _In_ WDFOBJECT Initializer
    )
/*++
Routine description:
    This routine gets a pointer to the location in the initializer object's 
    context space where we store the driver layer's device object settings.

Arguments:
    Initializer - Handle to the initializer object that is being used to 
        initialize our device object settings.

Return value:
    Pointer to a HELPER_DEVICE_INIT structure where the driver layer's device 
    object settings are stored.
--*/
{
    PHELPER_INIT initContext = NULL;

    PAGED_CODE();

    initContext = HelperGetInitContext(Initializer);

    if ((initContext->InitType != HelperInitTypeNone) &&
        (initContext->InitType != HelperInitTypeDevice)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Cannot get device initialization settings for "
              "initializer object %p because it is currently being used to "
              "initialize a different object.",
              Initializer);
        WdfVerifierDbgBreakPoint();
        return NULL;
    }

    if (initContext->InitType == HelperInitTypeNone) {
        initContext->InitType = HelperInitTypeDevice;
    }

    return &(initContext->u.DeviceInit);
}

PHELPER_QUEUE_INIT
GetQueueInitSettings(
    _In_ WDFOBJECT Initializer
    )
/*++
Routine description:
    This routine gets a pointer to the location in the initializer object's 
    context space where we store the driver layer's queue object settings.

Arguments:
    Initializer - Handle to the initializer object that is being used to 
        initialize our queue object settings.

Return value:
    Pointer to a HELPER_QUEUE_INIT structure where the driver layer's queue 
    object settings are stored.
--*/
{
    PHELPER_INIT initContext = NULL;
    
    initContext = HelperGetInitContext(Initializer);

    if ((initContext->InitType != HelperInitTypeNone) &&
        (initContext->InitType != HelperInitTypeQueue)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Cannot get queue initialization settings for "
              "initializer object %p because it is currently being used to "
              "initialize a different object.",
              Initializer);
        WdfVerifierDbgBreakPoint();
        return NULL;
    }

    if (initContext->InitType == HelperInitTypeNone) {
        initContext->InitType = HelperInitTypeQueue;
    }

    return &(initContext->u.QueueInit);
}

BOOLEAN
IsDeviceInitialized(
    _In_ WDFDEVICE Device
    )
/*++
Routine description:
    This routine determines whether we have initialized our settings for the 
    given device object.

Arguments:
    Device - Handle to the KMDF device object

Return value:
    TRUE if we have initialized our settings for the device object, FALSE 
    otherwise.
--*/
{
    PPOFX_DEVICE_CONTEXT devCtx;
    devCtx = HelperGetDeviceContext(Device);
    return (NULL != devCtx);
}

BOOLEAN
ArePowerFrameworkSettingsAvailable(
    _In_ WDFDEVICE Device
    )
/*++
Routine description:
    This routine determines whether the driver layer has provided us with its 
    power framework settings for the given device object.

Arguments:
    Device - Handle to the KMDF device object

Return value:
    TRUE if we the driver layer has provided us with its power framework 
    settings for the device object, FALSE otherwise.
--*/
{
    PPOFX_DEVICE_CONTEXT devCtx;
    devCtx = HelperGetDeviceContext(Device);
    return ((NULL != devCtx) && (NULL != devCtx->PoFxDeviceInfo));
}

BOOLEAN
IsQueueInitialized(
    _In_ WDFQUEUE Queue
    )
/*++
Routine description:
    This routine determines whether we have initialized our settings for the 
    given queue object.

Arguments:
    Queue - Handle to the KMDF queue object

Return value:
    TRUE if we have initialized our settings for the queue object, FALSE 
    otherwise.
--*/
{
    PPOFX_QUEUE_CONTEXT qCtx;
    qCtx = HelperGetQueueContext(Queue);
    return (NULL != qCtx);
}

VOID
ResetInitializer(
    _In_ WDFOBJECT Initializer
    )
/*++
Routine description:
    This routine prepares the given initializer object for initializing a new
    KMDF object.

Arguments:
    Initializer - Handle to the initializer object

Return value:
    None
--*/
{
    PHELPER_INIT initContext = NULL;
    
    //
    // We want to make sure that when the initializer context is zeroed, the 
    // default S0-idle power management configuration for the device is 
    // PfhS0IdleNotSupported.
    //
    C_ASSERT(0 == PfhS0IdleNotSupported);
    
    //
    // Reset the initializer context
    //
    initContext = HelperGetInitContext(Initializer);
    RtlZeroMemory(initContext, sizeof(*initContext));
    initContext->InitType = HelperInitTypeNone;

    return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhInitializerCreate(
    _Out_ WDFOBJECT * Initializer
    )
// See comments in WdfPoFx.h
{
    NTSTATUS status;
    WDFOBJECT initializer = NULL;
    WDF_OBJECT_ATTRIBUTES objectAttributes;

    PAGED_CODE();
    
    //
    // Create an initializer object
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttributes, HELPER_INIT);
    status = WdfObjectCreate(&objectAttributes, &initializer);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfObjectCreate failed with %!status!.", 
              status);
        goto exit;
    }

    //
    // Initialize context
    //
    ResetInitializer(initializer);

    *Initializer = initializer;

    status = STATUS_SUCCESS;
    
exit:
    return status;
}

NTSTATUS
CopyAndUpdateMinorFunctionsArray(
    _In_ WDFOBJECT Initializer,
    _In_reads_opt_(NumMinorFunctions) PUCHAR MinorFunctions,
    _In_ ULONG NumMinorFunctions,
    _Out_ WDFMEMORY * MinorFunctionsMemory,
    _Outptr_result_buffer_(*UpdatedNumMinorFunctions) PUCHAR *UpdatedMinorFunctions,
    _Out_ PULONG UpdatedNumMinorFunctions
    )
/*++
Routine description:
    This routine takes an array of power IRP minor functions and makes a copy of
    it. If the input array does not include IRP_MN_SET_POWER as one of the minor
    functions, the copy of the array made by this routine is expanded to include
    IRP_MN_SET_POWER too. This enables us to register a WDM pre-process callback
    for IRP_MN_SET_POWER, regardless of whether the driver layer is interested 
    in that minor function.

Arguments:
    Initializer - Handle to the initializer object

    MinorFunctions - Caller-initialized array of minor functions that the driver
        layer is interested in for the IRP_MJ_POWER major function.

    NumMinorFunctions - The number of minor functions in the MinorFunctions 
        array

    MinorFunctionsMemory - Pointer to a location that receives a handle to the 
        memory object created by this routine to store a copy of the minor 
        functions

    UpdatedMinorFunctions - Pointer to a location that receives an array of 
        minor functions that the driver layer is interested in, plus 
        IRP_MN_SET_POWER if it was not present in the original array

    UpdatedNumMinorFunctions - Pointer to a location that receives the number of
        minor functions in the UpdatedMinorFunctions array

Return value:
    An NTSTATUS value representing success or failure of the function.
--*/
{
    NTSTATUS status;
    BOOLEAN driverLayerPreprocessesSetPower = FALSE;
    UCHAR setPowerMinorFunction;
    ULONG i;
    ULONG driverLayerMinorFunctions = 0;
    ULONG extraMinorFunctions = 0;
    ULONG totalMinorFunctions = 0;
    ULONG totalMinorFunctionsSize = 0;
    ULONG memorySize = 0;
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    WDFMEMORY memory = NULL;
    PPOFX_DRIVER_LAYER_POWER_IRP_PREPROCESS_INFO memoryBuffer = NULL;
    ULONG minorFunctionsOffset;
    ULONG minorFunctionsSize;
    ULONG extraMinorFunctionOffset;

    PAGED_CODE();

    //
    // Check if the driver layer needs to preprocess IRP_MN_SET_POWER
    //
    if ((NULL != MinorFunctions) && (0 != NumMinorFunctions)) {
    
        driverLayerMinorFunctions = NumMinorFunctions;
        
        for (i=0; i < NumMinorFunctions; i++) {
            if (IRP_MN_SET_POWER == MinorFunctions[i]) {
                driverLayerPreprocessesSetPower = TRUE;
                break;
            }
        }
    }

    //
    // If the driver layer does not preprocess IRP_MN_SET_POWER, then we need to
    // update the minor functions array with an extra entry for IRP_MN_SET_POWER
    //
    extraMinorFunctions = driverLayerPreprocessesSetPower ? 0 : 1;

    //
    // Allocate memory to store a copy of the minor functions array
    //
    status = RtlULongAdd(driverLayerMinorFunctions,
                         extraMinorFunctions,
                         &totalMinorFunctions);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to compute total minor functions count for "
              "power IRP preprocessing. RtlUlongAdd failed with %!status!.", 
              status);
        goto exit;
    }

    ASSERT(totalMinorFunctions > 0);

    status = RtlULongMult(sizeof(MinorFunctions[0]),
                          totalMinorFunctions,
                          &totalMinorFunctionsSize);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to compute buffer size needed to store the "
              "functions array. RtlULongMult failed with %!status!.", 
              status);
        goto exit;
    }
    status = RtlULongAdd(sizeof(POFX_DRIVER_LAYER_POWER_IRP_PREPROCESS_INFO),
                         totalMinorFunctionsSize,
                         &memorySize);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to compute buffer size needed to store the "
              "minor functions array and count. RtlUlongAdd failed with "
              "%!status!.",
              status);
        goto exit;
    }
    if (0 == memorySize) {
        status = STATUS_INVALID_BUFFER_SIZE;
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - Unable to set memorySize. Failed with %!status!.",
              status);
        goto exit;
    }
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = Initializer; // auto-delete when parent 
                                                 // deleted
                                                 

    //
    // Suppress the warning that memorySize must be greater than zero
    //       
    #pragma warning(suppress:28160)
    status = WdfMemoryCreate(&objectAttributes,
                             NonPagedPool,
                             0, // PoolTag
                             memorySize,
                             &memory, 
                             (PVOID*) &memoryBuffer);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to allocate memory to store the minor functions"
              " array and count. WdfMemoryCreate failed with %!status!.", 
              status);
        goto exit;
    }

    //
    // We'll remember only the minor functions that the driver layer is 
    // interested in. If we add IRP_MN_SET_POWER to that list, we won't need to 
    // remember that beyond initialization time.
    //
    memoryBuffer->NumMinorFunctions = driverLayerMinorFunctions;

    //
    // Copy the minor functions array
    //
    minorFunctionsOffset = 
      FIELD_OFFSET(POFX_DRIVER_LAYER_POWER_IRP_PREPROCESS_INFO, MinorFunctions);
    minorFunctionsSize = driverLayerMinorFunctions * 
                                sizeof(MinorFunctions[0]); // already performed 
                                                           // intsafe math above
                                                           // won't overflow
    if (driverLayerMinorFunctions > 0) {
        if (0 == minorFunctionsSize) {
            status = STATUS_INVALID_BUFFER_SIZE;
            Trace(TRACE_LEVEL_ERROR,
                  "%!FUNC! - Unable to set minorFunctionsSize. Failed with %!status!.",
                  status);
            goto exit;
        }
        
        //
        // Suppress the warning that minorFunctionsSize must be greater than zero
        //       
        #pragma warning(suppress:28160)
        status = WdfMemoryCopyFromBuffer(memory,
                                         minorFunctionsOffset,
                                         memoryBuffer->MinorFunctions,
                                         minorFunctionsSize); 
        if (FALSE == NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - Unable to copy driver layer's minor functions "
                  "array. WdfMemoryCopyFromBuffer failed with %!status!.", 
                  status);
            goto exit;
        }
    }

    if (extraMinorFunctions > 0) {

        ASSERT(extraMinorFunctions == 1);
        
        //
        // Add an extra minor function for IRP_MN_SET_POWER since it was not in
        // the driver layer's list.
        // NOTE: This minor function is added at the end and is not reflected in
        // memoryBuffer.NumMinorFunctions.
        //
        extraMinorFunctionOffset = 
          minorFunctionsOffset + minorFunctionsSize;// already performed intsafe 
                                                    // math above so this won't 
                                                    // overflow
        setPowerMinorFunction = IRP_MN_SET_POWER;
        status = WdfMemoryCopyFromBuffer(memory,
                                         extraMinorFunctionOffset,
                                         &setPowerMinorFunction,
                                         sizeof(setPowerMinorFunction));
        if (FALSE == NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - Unable to add an extra minor function "
                  "(IRP_MN_SET_POWER) to the minor functions array. "
                  "WdfMemoryCopyFromBuffer failed with %!status!.", 
                  status);
            goto exit;
        }
    }

    *MinorFunctionsMemory = memory;
    *UpdatedMinorFunctions = memoryBuffer->MinorFunctions;
    *UpdatedNumMinorFunctions = 
               totalMinorFunctions;// Do not use memoryBuffer.NumMinorFunctions
                                   // here because it does not include the extra
                                   // minor function that we might have added.

    //
    // If we added an extra minor function, it is present in the buffer but not
    // reflected in memoryBuffer.NumMinorFunctions
    //
    ASSERT((memoryBuffer->NumMinorFunctions + extraMinorFunctions) == 
                totalMinorFunctions);

    status = STATUS_SUCCESS;

exit:
    if (FALSE == NT_SUCCESS(status)) {
        if (NULL != memory) {
            WdfObjectDelete(memory);
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhAssignWdmPowerIrpPreProcessCallback(
    _In_ WDFOBJECT Initializer,
    _In_  PWDFDEVICE_INIT DeviceInit,
    _In_opt_ PFN_WDFDEVICE_WDM_IRP_PREPROCESS EvtDeviceWdmPowerIrpPreprocess,
    _In_reads_opt_(NumMinorFunctions) PUCHAR MinorFunctions,
    _In_ ULONG NumMinorFunctions
    )
// See comments in WdfPoFx.h
{
    NTSTATUS status;
    WDFMEMORY minorFunctionsMemory = NULL;
    ULONG numMinorFunctions = 0;
    PUCHAR minorFunctions = NULL;
    PHELPER_DEVICE_INIT deviceInitSettings = NULL;

    PAGED_CODE();

    deviceInitSettings = GetDeviceInitSettings(Initializer);

    if (deviceInitSettings->PowerIrpPreprocessCallbackAssigned) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhAssignWdmPowerIrpPreProcessCallback has already"
              " been called on initialier %p.  It should not be called again "
              "before the initializer has been used to initialize a KMDF device"
              " object. %!status!.",
              Initializer,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }
    
    //
    // Copy the minor functions array and, if needed, update it to include 
    // IRP_MN_SET_POWER.
    //
    status = CopyAndUpdateMinorFunctionsArray(Initializer,
                                              MinorFunctions,
                                              NumMinorFunctions,
                                              &minorFunctionsMemory,
                                              &minorFunctions,
                                              &numMinorFunctions);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Assign the WDM preprocess callback
    //
    status = WdfDeviceInitAssignWdmIrpPreprocessCallback(
                DeviceInit,
                _PfhEvtWdmPowerIrpPreprocess,
                IRP_MJ_POWER,
                minorFunctions,
                numMinorFunctions
                );
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfDeviceInitAssignWdmIrpPreprocessCallback failed "
              "with %!status!.",
              status);
        goto exit;
    }

    deviceInitSettings->PowerIrpPreprocessMinorFunctions = minorFunctionsMemory;
    deviceInitSettings->EvtDeviceWdmPowerIrpPreprocess = 
                                EvtDeviceWdmPowerIrpPreprocess;
    deviceInitSettings->PowerIrpPreprocessCallbackAssigned = TRUE;
                                
    status = STATUS_SUCCESS;
    
exit:
    if (FALSE == NT_SUCCESS(status)) {
        if (NULL != minorFunctionsMemory) {
            WdfObjectDelete(minorFunctionsMemory);
        }
    }
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
PfhInterceptWdfPnpPowerEventCallbacks(
    _In_ WDFOBJECT Initializer,
    _Inout_ PWDF_PNPPOWER_EVENT_CALLBACKS DriverLayerPnpPowerCallbacks
    )
// See comments in WdfPoFx.h
{
    PHELPER_DEVICE_INIT deviceInitSettings = NULL;

    PAGED_CODE();

    deviceInitSettings = GetDeviceInitSettings(Initializer);

    if (deviceInitSettings->PnpPowerEventCallbacksIntercepted) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInterceptWdfPnpPowerEventCallbacks has already been"
              " called on initialier %p.  It should not be called again before "
              "the initializer has been used to initialize a KMDF device "
              "object.",
              Initializer);
        WdfVerifierDbgBreakPoint();
    }
    
    //
    // Save the driver layer's callbacks that we are going to replace
    //
    deviceInitSettings->EvtDeviceSelfManagedIoInit = 
                    DriverLayerPnpPowerCallbacks->EvtDeviceSelfManagedIoInit;
    deviceInitSettings->EvtDeviceSelfManagedIoFlush = 
                    DriverLayerPnpPowerCallbacks->EvtDeviceSelfManagedIoFlush;
    deviceInitSettings->EvtDeviceSelfManagedIoRestart = 
                    DriverLayerPnpPowerCallbacks->EvtDeviceSelfManagedIoRestart;
    deviceInitSettings->EvtDeviceD0Entry = 
                    DriverLayerPnpPowerCallbacks->EvtDeviceD0Entry;

    //
    // Replace the driver layer's callbacks with our own
    //
    DriverLayerPnpPowerCallbacks->EvtDeviceSelfManagedIoInit = 
                                        _PfhEvtSelfManagedIoInit;
    DriverLayerPnpPowerCallbacks->EvtDeviceSelfManagedIoFlush = 
                                        _PfhEvtSelfManagedIoFlush; 
    DriverLayerPnpPowerCallbacks->EvtDeviceSelfManagedIoRestart = 
                                        _PfhEvtSelfManagedIoRestart; 
    DriverLayerPnpPowerCallbacks->EvtDeviceD0Entry = 
                                        _PfhEvtD0Entry;

    deviceInitSettings->PnpPowerEventCallbacksIntercepted = TRUE;
    
    return;
}
    
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
PfhSetPoHandleAvailabilityCallbacks(
    _In_ WDFOBJECT Initializer,
    _In_ PPFH_CALLBACK_POHANDLE_AVAILABLE PfhCallbackPoHandleAvailable,
    _In_ PPFH_CALLBACK_POHANDLE_UNAVAILABLE PfhCallbackPoHandleUnavailable
    )
// See comments in WdfPoFx.h
{
    PHELPER_DEVICE_INIT deviceInitSettings = NULL;

    PAGED_CODE();

    deviceInitSettings = GetDeviceInitSettings(Initializer);

    if (deviceInitSettings->PoHandleAvailabilityCallbacksSet) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhSetPoHandleAvailabilityCallbacks has already been "
              "called on initialier %p.  It should not be called again before "
              "the initializer has been used to initialize a KMDF device "
              "object.",
              Initializer);
        WdfVerifierDbgBreakPoint();
    }

    //
    // Save the driver layer's callbacks
    //
    deviceInitSettings->PfhCallbackPoHandleAvailable = 
                                PfhCallbackPoHandleAvailable;
    deviceInitSettings->PfhCallbackPoHandleUnavailable =
                                PfhCallbackPoHandleUnavailable;

    deviceInitSettings->PoHandleAvailabilityCallbacksSet = TRUE;
    return;
}

NTSTATUS
CopyMinorFunctionsArray(
    _In_ WDFDEVICE Device,
    _In_ WDFMEMORY SourceMemory,
    _Out_ WDFMEMORY * DestinationMemory
    )
/*++
Routine description:
    This routine creates a new memory object to store the array of power IRP 
    minor functions that the driver layer is interested in. The array is 
    currently stored in a memory object that we had created as a child of the 
    initializer object that was being used to initialize the device object. Now
    that the initialization of the device object is about to complete, we need 
    to make a copy of the contents of that memory object. Once the device object 
    initialization is complete, the driver layer is free to delete the 
    initializer object, which will result in the original memory object also 
    getting delete. That is why we copy the contents to a new memory object.

Arguments:
    Device - Handle to the KMDF device object

    SourceMemory - Handle to the memory object that contains the array of power 
        IRP minor functions that the driver layer is interested in

    DestinationMemory - Pointer to a location that receives a handle to a new 
        memory object created by this routine. The new memory object contains a
        copy of the a

Return value:
    An NTSTATUS value representing success or failure of the function.
--*/
{
    NTSTATUS status;
    PPOFX_DRIVER_LAYER_POWER_IRP_PREPROCESS_INFO powerIrpPreprocessInfo = NULL;
    size_t powerIrpPreprocessInfoSize;
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    WDFMEMORY memory = NULL;

    PAGED_CODE();

    //
    // Get the source memory buffer
    //
    powerIrpPreprocessInfo = WdfMemoryGetBuffer(SourceMemory, 
                                                &powerIrpPreprocessInfoSize);
    if (0 == powerIrpPreprocessInfoSize) {
        status = STATUS_INVALID_BUFFER_SIZE;
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - Unable to get powerIrpPreprocessInfoSize. Failed with %!status!.",
              status);
        goto exit;
    }
                                                
    //
    // Create a new memory object to hold a copy of the buffer
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = Device; // auto-delete when parent deleted
    status = WdfMemoryCreate(&objectAttributes,
                             NonPagedPool,
                             0, // PoolTag
                             powerIrpPreprocessInfoSize,
                             &memory, 
                             NULL // Buffer
                             );
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to allocate memory for the driver layer's minor"
              " functions array. WdfMemoryCreate failed with %!status!.", 
              status);
        goto exit;
    }

    //
    // Copy the buffer into the new memory object
    //
    status = WdfMemoryCopyFromBuffer(memory,
                                     0, // DestinationOffset
                                     powerIrpPreprocessInfo,
                                     powerIrpPreprocessInfoSize);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    *DestinationMemory = memory;

    status = STATUS_SUCCESS;
    
exit:
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhInitializeDeviceSettings(
    _In_ WDFDEVICE Device,
    _In_ WDFOBJECT Initializer
    )
// See comments in WdfPoFx.h
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    PPOFX_DEVICE_CONTEXT devCtx;
    PHELPER_DEVICE_INIT deviceInitSettings = NULL;
    WDFMEMORY memory = NULL;

    PAGED_CODE();

    deviceInitSettings = GetDeviceInitSettings(Initializer);

    if (IsDeviceInitialized(Device)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializeDeviceSettings has already been called "
              "for WDFDEVICE %p. It should not be called again. %!status!.",
              Device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    if (FALSE == deviceInitSettings->PowerIrpPreprocessCallbackAssigned) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhAssignWdmPowerIrpPreProcessCallback has not yet "
              "been called for WDFDEVICE %p. %!status!.",
              Device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    if (FALSE == deviceInitSettings->PnpPowerEventCallbacksIntercepted) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInterceptWdfPnpPowerEventCallbacks has not yet been"
              " called for WDFDEVICE %p. %!status!.",
              Device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    //
    // Allocate our context for this device
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttributes, 
                                            POFX_DEVICE_CONTEXT);
    status = WdfObjectAllocateContext((WDFOBJECT) Device,
                                      &objectAttributes,
                                      (PVOID*) &devCtx);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfObjectAllocateContext failed with %!status!", 
              status);
        goto exit;
    }

    //
    // Copy the device init settings
    //
    devCtx->DeviceInitSettings = *deviceInitSettings;

    //
    // Copy the minor functions array into a new memory object that is a child 
    // of the device object. The array is currently in a memory object that is
    // a child of the initializer object. The initializer object can be deleted
    // by the driver layer any time after initialization is complete, so we need
    // to make a copy now.
    //
    status = CopyMinorFunctionsArray(
                Device, 
                devCtx->DeviceInitSettings.PowerIrpPreprocessMinorFunctions,
                &memory
                );
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Perform initialization specific to S0-idle support (if enabled)
    //
    INITIALIZE_DEVICE_SETTINGS_FOR_S0_IDLE(Device, devCtx);
    
    //
    // In our device context, replace the pointer to the initializer's memory
    // object with a pointer to the new memory object that we just created.
    //
    devCtx->DeviceInitSettings.PowerIrpPreprocessMinorFunctions = memory;

    //
    // For convenient access, also save the memory object's buffer in our device 
    // context.
    //
    devCtx->DriverLayerPowerIrpPreprocessInfo = WdfMemoryGetBuffer(
                                                            memory, 
                                                            NULL // BufferSize
                                                            );

    //
    // By default, we do not need to call PoFxReportDevicePoweredOn from our
    // EvtDeviceD0Entry callback. We only need to call it in the 
    // EvtDeviceD0Entry callback that is invoked right after we receive an S0 
    // IRP.
    //
    devCtx->ShouldReportDevicePoweredOn = FALSE;

    //
    // The initializer can now be used to initialize some other object
    //
    ResetInitializer(Initializer);

    status = STATUS_SUCCESS;

exit:
    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PfhInitializePowerFrameworkSettings(
    _In_ WDFDEVICE Device,
    _In_ PPO_FX_DEVICE PoFxDeviceInfo
    )
// See comments in WdfPoFx.h
{
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    PPOFX_DEVICE_CONTEXT devCtx;
    ULONG i;
    ULONG pofxExtraComponentsSize;
    ULONG pofxDeviceInfoSize;
    WDFMEMORY memory = NULL;
    ULONG idleStatesSize;
    PVOID idleStates = NULL;
    PPO_FX_DEVICE poFxDeviceInfo = NULL;
    PPOFX_COMPONENT_INFO componentInfo = NULL;
    ULONG componentInfoSize;

    PAGED_CODE();

    if (FALSE == IsDeviceInitialized(Device)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializeDeviceSettings has not yet been called "
              "for WDFDEVICE %p. %!status!.",
              Device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    if (ArePowerFrameworkSettingsAvailable(Device)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializePowerFrameworkSettings has already been "
              "called for WDFDEVICE %p. It should not be called again. "
              "%!status!.",
              Device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }
    
    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(Device);

    //
    // Allocate memory to store power framework settings
    //
    status = RtlULongMult(sizeof(PO_FX_COMPONENT),
                          (PoFxDeviceInfo->ComponentCount - 1),
                          &pofxExtraComponentsSize);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to compute buffer size needed for extra "
              "components. RtlULongMult failed with %!status!.", 
              status);
        goto exit;
    }
    status = RtlULongAdd(sizeof(PO_FX_DEVICE),
                         pofxExtraComponentsSize,
                         &pofxDeviceInfoSize);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to compute buffer size needed for power "
              "framework settings. RtlUlongAdd failed with %!status!.", 
              status);
        goto exit;
    }
    if (0 == pofxDeviceInfoSize) {
        status = STATUS_INVALID_BUFFER_SIZE;
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - Unable to set pofxDeviceInfoSize. Failed with %!status!.",
              status);
        goto exit;
    }
    
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = Device; // auto-delete when parent deleted
    status = WdfMemoryCreate(&objectAttributes,
                             NonPagedPool,
                             0, // PoolTag
                             pofxDeviceInfoSize,
                             &memory, 
                             (PVOID*) &poFxDeviceInfo);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to allocate memory for power framework "
              "settings. WdfMemoryCreate failed with %!status!.", 
              status);
        goto exit;
    }

    devCtx->PoFxDeviceInfo = poFxDeviceInfo;
    
    //
    // Copy power framework settings
    //
    status = WdfMemoryCopyFromBuffer(memory,
                                     0, // DestinationOffset
                                     PoFxDeviceInfo,
                                     pofxDeviceInfoSize);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to copy power framework settings. "
              "WdfMemoryCopyFromBuffer failed with %!status!.", 
              status);
        goto exit;
    }
                             
    //
    // Save the driver layer's callbacks and replace them with our own
    //
    devCtx->DriverLayerPoFxCallbacks.ComponentIdleStateCallback =
                 devCtx->PoFxDeviceInfo->ComponentIdleStateCallback;
    devCtx->DriverLayerPoFxCallbacks.ComponentActiveConditionCallback =
                 devCtx->PoFxDeviceInfo->ComponentActiveConditionCallback;
    devCtx->DriverLayerPoFxCallbacks.ComponentIdleConditionCallback =
                 devCtx->PoFxDeviceInfo->ComponentIdleConditionCallback;
    devCtx->DriverLayerPoFxCallbacks.DevicePowerRequiredCallback =
                 devCtx->PoFxDeviceInfo->DevicePowerRequiredCallback;
    devCtx->DriverLayerPoFxCallbacks.DevicePowerNotRequiredCallback =
                 devCtx->PoFxDeviceInfo->DevicePowerNotRequiredCallback;
    devCtx->PoFxDeviceInfo->ComponentIdleStateCallback =
                             _PfhComponentIdleStateCallback;
    devCtx->PoFxDeviceInfo->ComponentActiveConditionCallback =
                             _PfhComponentActiveConditionCallback;
    devCtx->PoFxDeviceInfo->ComponentIdleConditionCallback =
                             _PfhComponentIdleConditionCallback;
    devCtx->PoFxDeviceInfo->DevicePowerRequiredCallback =
                             _PfhDevicePowerRequiredCallback;
    devCtx->PoFxDeviceInfo->DevicePowerNotRequiredCallback =
                             _PfhDevicePowerNotRequiredCallback;

    //
    // Save the driver layer's context and replace it with our own
    //
    devCtx->DriverLayerPoFxContext = devCtx->PoFxDeviceInfo->DeviceContext;
    devCtx->PoFxDeviceInfo->DeviceContext = (PVOID) Device;

    //
    // Store the idle states for each component
    //
    for (i=0; i < devCtx->PoFxDeviceInfo->ComponentCount; i++) {
        
        status = RtlULongMult(sizeof(PO_FX_COMPONENT_IDLE_STATE),
                              PoFxDeviceInfo->Components[i].IdleStateCount,
                              &idleStatesSize);
        if (FALSE == NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - Unable to compute buffer size needed for idle "
                  "states for component %d. RtlUlongMult failed with "
                  "%!status!.",
                  i,
                  status);
            goto exit;
        }
        if (0 == idleStatesSize) {
            status = STATUS_INVALID_BUFFER_SIZE;
            Trace(TRACE_LEVEL_ERROR,
                  "%!FUNC! - Unable to set idleStatesSize. Failed with %!status!.",
                  status);
            goto exit;
        }

        WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
        objectAttributes.ParentObject = Device;//auto-delete when parent deleted
        status = WdfMemoryCreate(&objectAttributes,
                                 NonPagedPool,
                                 0, // PoolTag
                                 idleStatesSize,
                                 &memory,
                                 &idleStates);
        if (FALSE == NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - Unable to allocate memory for idle states for "
                  "component %d. WdfMemoryCreate failed with %!status!.", 
                  i,
                  status);
            goto exit;
        }

        status = WdfMemoryCopyFromBuffer(
                            memory,
                            0, // DestinationOffset
                            devCtx->PoFxDeviceInfo->Components[i].IdleStates,
                            idleStatesSize
                            );
        if (FALSE == NT_SUCCESS(status)) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - Unable to copy idle states for component %d. "
                  "WdfMemoryCopyFromBuffer failed with %!status!.", 
                  i,
                  status);
            goto exit;
        }
        devCtx->PoFxDeviceInfo->Components[i].IdleStates = idleStates;
    }

    //
    // Allocate memory to store our private, per-component information
    //
    status = RtlULongMult(sizeof(POFX_COMPONENT_INFO),
                          devCtx->PoFxDeviceInfo->ComponentCount,
                          &componentInfoSize);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to compute buffer size needed for storing "
              "private, per-component information. RtlULongMult failed with "
              "%!status!.", 
              status);
        goto exit;
    }
    if (0 == componentInfoSize) {
        status = STATUS_INVALID_BUFFER_SIZE;
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - Unable to set componentInfoSize. Failed with %!status!.",
              status);
        goto exit;
    }
    
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
    objectAttributes.ParentObject = Device; // auto-delete when parent deleted
    status = WdfMemoryCreate(&objectAttributes,
                             NonPagedPool,
                             0, // PoolTag
                             componentInfoSize,
                             &memory, 
                             (PVOID*) &componentInfo);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Unable to allocate memory for storing private, per-"
              "component information. WdfMemoryCreate failed with %!status!.", 
              status);
        goto exit;
    }
    RtlZeroMemory(componentInfo, componentInfoSize);
    devCtx->ComponentInfo = componentInfo;

    status = STATUS_SUCCESS;

exit:
    return status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PfhInterceptComponentQueueConfig(
    _In_ WDFOBJECT Initializer,
    _Inout_ PWDF_IO_QUEUE_CONFIG DriverLayerQueueConfig
    )
// See comments in WdfPoFx.h
{
    PHELPER_QUEUE_INIT queueInitSettings = NULL;

    queueInitSettings = GetQueueInitSettings(Initializer);

    if (queueInitSettings->ComponentQueueConfigIntercepted) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInterceptComponentQueueConfig has already been "
              "called on initialier %p.  It should not be called again before "
              "the initializer has been used to initialize a KMDF queue "
              "object.",
              Initializer);
        WdfVerifierDbgBreakPoint();
    }

    //
    // Save the driver layer's callbacks that we are going to replace
    //
    queueInitSettings->EvtIoCanceledOnQueue = 
                            DriverLayerQueueConfig->EvtIoCanceledOnQueue;

    //
    // Replace the driver layer's callbacks with our own
    //
    DriverLayerQueueConfig->EvtIoCanceledOnQueue = 
                            _PfhEvtRequestCanceledOnComponentQueue;

    queueInitSettings->ComponentQueueConfigIntercepted = TRUE;                            
                            
    return;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PfhSetComponentForComponentQueue(
    _In_ WDFOBJECT Initializer,
    _In_ ULONG Component
    )
// See comments in WdfPoFx.h
{
    PHELPER_QUEUE_INIT queueInitSettings = NULL;

    queueInitSettings = GetQueueInitSettings(Initializer);

    if (queueInitSettings->ComponentSet) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhSetComponentForComponentQueue has already been "
              "called on initialier %p.  It should not be called again before "
              "the initializer has been used to initialize a KMDF queue "
              "object.",
              Initializer);
        WdfVerifierDbgBreakPoint();
    }

    queueInitSettings->Component = Component;
    
    queueInitSettings->ComponentSet = TRUE;                            

    return;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PfhInitializeComponentQueueSettings(
    _In_ WDFQUEUE Queue,
    _In_ WDFOBJECT Initializer
    )
// See comments in WdfPoFx.h
{
    NTSTATUS status;
    WDFDEVICE device = NULL;
    ULONG component;
    WDF_OBJECT_ATTRIBUTES objectAttributes;
    PPOFX_DEVICE_CONTEXT devCtx;
    PPOFX_QUEUE_CONTEXT qCtx;
    PHELPER_QUEUE_INIT queueInitSettings = NULL;

    queueInitSettings = GetQueueInitSettings(Initializer);

    if (IsQueueInitialized(Queue)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializeComponentQueueSettings has already been "
              "called for WDFQUEUE %p. It should not be called again. "
              "%!status!.",
              Queue,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    device = WdfIoQueueGetDevice(Queue);
    if (FALSE == ArePowerFrameworkSettingsAvailable(device)) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializePowerFrameworkSettings has not yet been "
              "called for WDFDEVICE %p. %!status!.",
              device,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    if (FALSE == queueInitSettings->ComponentQueueConfigIntercepted) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInterceptComponentQueueConfig has not yet been "
              "called for WDFQUEUE %p. %!status!.",
              Queue,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    if (FALSE == queueInitSettings->ComponentSet) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhSetComponentForComponentQueue has not yet been "
              "called for WDFQUEUE %p. %!status!.",
              Queue,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(device);
    
    //
    // Validate the component number
    //
    component = queueInitSettings->Component;
    if (component >= devCtx->PoFxDeviceInfo->ComponentCount) {
        status = STATUS_INVALID_PARAMETER;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Component number %d is invalid. Component count is %d."
              " %!status!.",
              component,
              devCtx->PoFxDeviceInfo->ComponentCount,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    //
    // Allocate our context for this queue
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttributes, 
                                            POFX_QUEUE_CONTEXT);
    status = WdfObjectAllocateContext((WDFOBJECT) Queue,
                                      &objectAttributes,
                                      (PVOID*) &qCtx);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfObjectAllocateContext failed with %!status!", 
              status);
        goto exit;
    }

    if (NULL != devCtx->ComponentInfo[component].Queue) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - Component %d has already been associated with WDFQUEUE"
              " %p. Associating a component with more than one queue is not "
              "supported. %!status!.",
              component,
              devCtx->ComponentInfo[component].Queue,
              status);
        WdfVerifierDbgBreakPoint();
        goto exit;
    }

    //
    // Copy the queue init settings
    //
    qCtx->QueueInitSettings = *queueInitSettings;

    //
    // The initializer can now be used to initialize some other object
    //
    ResetInitializer(Initializer);

    //
    // Save the queue handle in our private component information
    //
    devCtx->ComponentInfo[component].Queue = Queue;

    status = STATUS_SUCCESS;
    
exit:
    return status;
}
