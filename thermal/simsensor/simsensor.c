/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    simsensor.c

Abstract:

    The module implements a simulated sensor device.

@@BEGIN_DDKSPLIT
Author:

    Nicholas Brekhus (NiBrekhu) 02-Aug-2011

Revision History:

@@END_DDKSPLIT
--*/

//-------------------------------------------------------------------- Includes

#include "simsensor.h"


//--------------------------------------------------------------------- Globals

ULONG SimSensorDebug = SIMSENSOR_PRINT_ALWAYS;

#define VIRTUAL_SENSOR_RESET_TEMPERATURE 42

// {FCB15302-14A9-4bf8-8A0B-888E0D33BEDE}
DEFINE_GUID(GUID_VIRTUAL_TEMPERATURE_SENSOR,
0xfcb15302, 0x14a9, 0x4bf8, 0x8a, 0xb, 0x88, 0x8e, 0xd, 0x33, 0xbe, 0xde);

//------------------------------------------------------------------ Prototypes

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD           SimSensorDriverDeviceAdd;


EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  SimSensorIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL SimSensorIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP SimSensorQueueIoStop;
EVT_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND SimSensorSelfManagedIoSuspend;

EVT_WDF_TIMER                       SimSensorExpiredRequestTimer;

EVT_WDF_WORKITEM SimSensorTemperatureInterruptWorker;

_IRQL_requires_(PASSIVE_LEVEL)
VOID
SimSensorAddReadRequest (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST ReadRequest
    );

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimSensorScanPendingQueue (
    _In_ WDFDEVICE Device
    );

_IRQL_requires_(PASSIVE_LEVEL)
VOID
SimSensorCheckQueuedRequest (
    _In_ WDFDEVICE Device,
    _In_ ULONG Temperature,
    _Inout_ PULONG LowerBound,
    _Inout_ PULONG UpperBound,
    _In_ WDFREQUEST Request
    );

_IRQL_requires_(PASSIVE_LEVEL)
BOOLEAN
SimSensorAreConstraintsSatisfied (
    _In_ ULONG Temperature,
    _In_ ULONG LowerBound,
    _In_ ULONG UpperBound,
    _In_ LARGE_INTEGER DueTime);

VOID
SimSensorTemperatureInterrupt (
    _In_ WDFDEVICE Device
    );

//
// Virtual hardware programming interface
//

VOID
SimSensorSetVirtualInterruptThresholds (
    _In_ WDFDEVICE Device,
    _In_ ULONG LowerBound,
    _In_ ULONG UpperBound
    );

ULONG
SimSensorReadVirtualTemperature (
    _In_ WDFDEVICE Device
    );

//
// Virtual hardware internal routines
//

EVT_WDF_DEVICE_D0_ENTRY SimSensorDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT SimSensorDeviceD0Exit;
POWER_SETTING_CALLBACK SimSensorSettingCallback;


//--------------------------------------------------------------------- Pragmas

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SimSensorAddReadRequest)
#pragma alloc_text(PAGE, SimSensorAreConstraintsSatisfied)
#pragma alloc_text(PAGE, SimSensorCheckQueuedRequest)
#pragma alloc_text(PAGE, SimSensorSelfManagedIoSuspend)
#pragma alloc_text(PAGE, SimSensorDriverDeviceAdd)
#pragma alloc_text(PAGE, SimSensorExpiredRequestTimer)
#pragma alloc_text(PAGE, SimSensorScanPendingQueue)
#pragma alloc_text(PAGE, SimSensorIoDeviceControl)

//------------------------------------------------------------------- Functions

NTSTATUS
DriverEntry (
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry configures and creates a WDF
    driver object.

Parameters Description:

    DriverObject - Supplies a pointer to the driver object.

    RegistryPath - Supplies a pointer to a unicode string representing the path
        to the driver-specific key in the registry.

Return Value:

    NTSTATUS.

--*/

{

    WDF_OBJECT_ATTRIBUTES DriverAttributes;
    WDF_DRIVER_CONFIG DriverConfig;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(RegistryPath);

    DebugEnter();

    WDF_DRIVER_CONFIG_INIT(&DriverConfig, SimSensorDriverDeviceAdd);

    //
    // Initialize attributes and a context area for the driver object.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&DriverAttributes);
    DriverAttributes.SynchronizationScope = WdfSynchronizationScopeNone;

    //
    // Create the driver object
    //

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &DriverAttributes,
                             &DriverConfig,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMSENSOR_ERROR,
                   "WdfDriverCreate() Failed. Status 0x%x\n",
                   Status);

        goto DriverEntryEnd;
    }

DriverEntryEnd:
    DebugExitStatus(Status);
    return Status;
}

NTSTATUS
SimSensorDriverDeviceAdd (
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    EvtDriverDeviceAdd is called by the framework in response to AddDevice call
    from the PnP manager. A WDF device object is created and initialized to
    represent a new instance of the battery device.

Arguments:

    Driver - Supplies a handle to the WDF Driver object.

    DeviceInit - Supplies a pointer to a framework-allocated WDFDEVICE_INIT
        structure.

Return Value:

    NTSTATUS

--*/

{

    WDF_OBJECT_ATTRIBUTES DeviceAttributes;
    WDFDEVICE DeviceHandle;
    PFDO_DATA DevExt;
    BOOLEAN LockHeld;
    WDF_IO_QUEUE_CONFIG PendingRequestQueueConfig;
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;
    WDFQUEUE Queue;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    NTSTATUS Status;
    WDF_OBJECT_ATTRIBUTES WorkitemAttributes;
    WDF_WORKITEM_CONFIG WorkitemConfig;

    UNREFERENCED_PARAMETER(Driver);

    DebugEnter();
    PAGED_CODE();

    LockHeld = FALSE;

    //
    // Initialize attributes and a context area for the device object.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&DeviceAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&DeviceAttributes, FDO_DATA);

    //
    // Initailize power callbacks
    //

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
    PnpPowerCallbacks.EvtDeviceD0Entry = SimSensorDeviceD0Entry;
    PnpPowerCallbacks.EvtDeviceD0Exit = SimSensorDeviceD0Exit;
    PnpPowerCallbacks.EvtDeviceSelfManagedIoSuspend =
        SimSensorSelfManagedIoSuspend;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPowerCallbacks);

    //
    // Create a framework device object.  This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //

    Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &DeviceHandle);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMSENSOR_ERROR,
                   "WdfDeviceCreate() Failed. 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    DevExt = GetDeviceExtension(DeviceHandle);

    //
    // Configure a default queue for IO requests. This queue processes requests
    // to read the sensor state.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig,
                                           WdfIoQueueDispatchParallel);

    QueueConfig.EvtIoDeviceControl = SimSensorIoDeviceControl;

    //
    // The system uses IoInternalDeviceControl requests to communicate with the
    // ACPI driver on the device stack. For proper operation of thermal zones,
    // these requests must be forwarded unless the driver knows how to handle
    // them.
    //

    QueueConfig.EvtIoInternalDeviceControl = SimSensorIoInternalDeviceControl;
    Status = WdfIoQueueCreate(DeviceHandle,
                              &QueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMSENSOR_ERROR,
                   "WdfIoQueueCreate() (Default) Failed.  0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    //
    // Configure a manual dispatch queue for pending requests. This queue
    // stores requests to read the sensor state which can't be retired
    // immediately.
    //

    WDF_IO_QUEUE_CONFIG_INIT(&PendingRequestQueueConfig,
                             WdfIoQueueDispatchManual);


    Status = WdfIoQueueCreate(DeviceHandle,
                              &PendingRequestQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &DevExt->PendingRequestQueue);

    PendingRequestQueueConfig.EvtIoStop = SimSensorQueueIoStop;
    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMSENSOR_ERROR,
                "WdfIoQueueCreate() (Pending) Failed. 0x%x\n",
                Status);

        goto DriverDeviceAddEnd;
    }


    //
    // Configure a workitem to process the simulated interrupt.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&WorkitemAttributes);
    WorkitemAttributes.ParentObject = DeviceHandle;
    WDF_WORKITEM_CONFIG_INIT(&WorkitemConfig,
                             SimSensorTemperatureInterruptWorker);

    Status = WdfWorkItemCreate(&WorkitemConfig,
                               &WorkitemAttributes,
                               &DevExt->InterruptWorker);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMSENSOR_ERROR,
                   "WdfWorkItemCreate() Failed. 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    //
    // Create the request queue waitlock.
    //

    Status = WdfWaitLockCreate(NULL, &DevExt->QueueLock);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMSENSOR_ERROR,
                   "WdfWaitLockCreate() Failed. Status 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    //
    // Initilize the simulated sensor hardware.
    //

    DevExt->Sensor.LowerBound = 0;
    DevExt->Sensor.UpperBound = (ULONG)-1;
    DevExt->Sensor.Temperature = VIRTUAL_SENSOR_RESET_TEMPERATURE;
    Status = WdfWaitLockCreate(NULL, &DevExt->Sensor.Lock);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(SIMSENSOR_ERROR,
                   "WdfWaitLockCreate() Failed. 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

DriverDeviceAddEnd:

    DebugExitStatus(Status);
    return Status;
}

VOID
SimSensorQueueIoStop (
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
    )

/*++

Routine Description:

    This routine is called when the framework is stopping the request's I/O
    queue.

Arguments:

    Queue - Supplies handle to the framework queue object that is associated
            with the I/O request.

    Request - Supplies handle to a framework request object.

    ActionFlags - Supplies the reason that the callback is being called.

Return Value:

    None.

--*/

{

    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Queue);

    if(ActionFlags & WdfRequestStopRequestCancelable) {
        Status = WdfRequestUnmarkCancelable(Request);
        if (Status == STATUS_CANCELLED) {
            goto SimSensorQueueIoStopEnd;
        }

        NT_ASSERT(NT_SUCCESS(Status));
    }

    WdfRequestStopAcknowledge(Request, FALSE);

SimSensorQueueIoStopEnd:

    return;
}

VOID
SimSensorIoDeviceControl(
    WDFQUEUE   Queue,
    WDFREQUEST Request,
    size_t     OutputBufferLength,
    size_t     InputBufferLength,
    ULONG      IoControlCode
    )

/*++

Routine Description:

    Handles requests to read or write the simulated device state.

Arguments:

    Queue - Supplies a handle to the framework queue object that is associated
        with the I/O request.

    Request - Supplies a handle to a framework request object. This one
        represents the IRP_MJ_DEVICE_CONTROL IRP received by the framework.

    OutputBufferLength - Supplies the length, in bytes, of the request's output
        buffer, if an output buffer is available.

    InputBufferLength - Supplies the length, in bytes, of the request's input
        buffer, if an input buffer is available.

    IoControlCode - Supplies the Driver-defined or system-defined I/O control
        code (IOCtl) that is associated with the request.

Return Value:

   VOID

--*/

{
    ULONG BytesReturned;
    WDFDEVICE Device;
    BOOLEAN Result;
    WDF_REQUEST_SEND_OPTIONS RequestSendOptions;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    PAGED_CODE();

    Device = WdfIoQueueGetDevice(Queue);
    DebugPrint(SIMSENSOR_NOTE, "SimSensorIoDeviceControl: 0x%p\n", Device);
    BytesReturned = 0;
    switch(IoControlCode) {
    case IOCTL_THERMAL_READ_TEMPERATURE:

        //
        // This call will either complete the request or put it in the pending
        // queue.
        //

        SimSensorAddReadRequest(Device, Request);
        break;
    default:

        //
        // Unrecognized IOCtls must be forwarded down the stack.
        //

        WDF_REQUEST_SEND_OPTIONS_INIT(
            &RequestSendOptions,
            WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

        WdfRequestFormatRequestUsingCurrentType(Request);

        Result = WdfRequestSend(
                    Request,
                    WdfDeviceGetIoTarget(Device),
                    &RequestSendOptions);

        if (Result == FALSE) {
            Status = WdfRequestGetStatus(Request);
            DebugPrint(SIMSENSOR_WARN,
                       "WdfRequestSend() Failed. Request Status = 0x%x\n",
                       Status);

            WdfRequestComplete(Request, Status);
        }
        break;
    }
}

VOID
SimSensorIoInternalDeviceControl (
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode)

/*++

Description:

    The system uses IoInternalDeviceControl requests to communicate with the
    ACPI driver on the device stack. For proper operation of thermal zones,
    these requests must be forwarded unless the driver knows how to handle
    them.

--*/

{
    WDF_REQUEST_SEND_OPTIONS RequestSendOptions;
    BOOLEAN Return;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    DebugEnter();

    WdfRequestFormatRequestUsingCurrentType(Request);

    WDF_REQUEST_SEND_OPTIONS_INIT(
        &RequestSendOptions,
        WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    Return = WdfRequestSend(
                Request,
                WdfDeviceGetIoTarget(WdfIoQueueGetDevice(Queue)),
                &RequestSendOptions);

    if (Return == FALSE) {
        Status = WdfRequestGetStatus(Request);
        DebugPrint(SIMSENSOR_WARN,
                   "WdfRequestSend() Failed. Request Status=0x%x\n",
                   Status);

        WdfRequestComplete(Request, Status);
    }

    DebugExit();
}

_IRQL_requires_(PASSIVE_LEVEL)
BOOLEAN
SimSensorAreConstraintsSatisfied (
    _In_ ULONG Temperature,
    _In_ ULONG LowerBound,
    _In_ ULONG UpperBound,
    _In_ LARGE_INTEGER DueTime
    )

/*++

Routine Description:

    Checks whether a request can be retired.

Arguments:

    Temperature - Supplies the device's current temperature.

    LowerBound - Supplies the request's lower temperature bound.

    UpperBound - Supplies the request's upper temperature bound.

    DueTime - Supplies when the request expires.

Return Value:

    TRUE - The request is retireable.

    FALSE - The request is not retireable.

--*/

{
    LARGE_INTEGER CurrentTime;

    PAGED_CODE();

    if (Temperature < LowerBound || Temperature > UpperBound) {
        return TRUE;
    }

    //
    // Negative due times are meaningless, except for the special value -1,
    // which represents no timeout.
    //

    if (DueTime.QuadPart < 0) {
        return FALSE;
    }

    KeQuerySystemTime(&CurrentTime);

    if ((CurrentTime.QuadPart - DueTime.QuadPart) >= 0) {

        //
        // This request expired in the past.
        //

        return TRUE;
    }

    return FALSE;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
SimSensorAddReadRequest (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST ReadRequest
    )

/*++

Routine Description:

    Handles IOCTL_THERMAL_READ_TEMPERATURE. If the request can be satisfied,
    it is completed immediately. Else, adds request to pending request queue.

Arguments:

    Device - Supplies a handle to the device that received the request.

    ReadRequest - Supplies a handle to the request.

--*/

{
    ULONG BytesReturned;
    PREAD_REQUEST_CONTEXT Context;
    WDF_OBJECT_ATTRIBUTES ContextAttributes;
    PFDO_DATA DevExt;
    LARGE_INTEGER ExpirationTime;
    size_t Length;
    BOOLEAN LockHeld;
    PULONG RequestTemperature;
    NTSTATUS Status;
    ULONG Temperature;
    WDFTIMER Timer;
    WDF_OBJECT_ATTRIBUTES TimerAttributes;
    WDF_TIMER_CONFIG TimerConfig;
    PTHERMAL_WAIT_READ ThermalWaitRead;


    DebugEnter();
    PAGED_CODE();

    DevExt = GetDeviceExtension(Device);
    BytesReturned = 0;
    LockHeld = FALSE;
    Status = WdfRequestRetrieveInputBuffer(ReadRequest,
                                           sizeof(THERMAL_WAIT_READ),
                                           &ThermalWaitRead,
                                           &Length);

    if (!NT_SUCCESS(Status) || Length != sizeof(THERMAL_WAIT_READ)) {

        //
        // This request is malformed, bail.
        //

        WdfRequestCompleteWithInformation(ReadRequest, Status, BytesReturned);
        goto AddReadRequestEnd;
    }


    if (ThermalWaitRead->Timeout != -1 /* INFINITE */ ) {

        //
        // Estimate the system time this request will expire at.
        //

        KeQuerySystemTime(&ExpirationTime);
        ExpirationTime.QuadPart += ThermalWaitRead->Timeout * 10000;

    } else {

        //
        // Value which indicates the request never expires.
        //

        ExpirationTime.QuadPart = -1LL /* INFINITE */;
    }

    //
    // Handle the immediate timeout case in the fast path.
    //

    Temperature = SimSensorReadVirtualTemperature(Device);
    if (SimSensorAreConstraintsSatisfied(Temperature,
                                         ThermalWaitRead->LowTemperature,
                                         ThermalWaitRead->HighTemperature,
                                         ExpirationTime)) {

        Status = WdfRequestRetrieveOutputBuffer(ReadRequest,
                                                sizeof(ULONG),
                                                &RequestTemperature,
                                                &Length);

        if(NT_SUCCESS(Status) && Length == sizeof(ULONG)) {
            *RequestTemperature = Temperature;
            BytesReturned = sizeof(ULONG);

        } else {
            Status = STATUS_INVALID_PARAMETER;
            DebugPrint(SIMSENSOR_ERROR,
                       "WdfRequestRetrieveOutputBuffer() Failed. 0x%x",
                       Status);

        }

        WdfRequestCompleteWithInformation(ReadRequest, Status, BytesReturned);
    } else {

        WdfWaitLockAcquire(DevExt->QueueLock, NULL);
        LockHeld = TRUE;

        //
        // Create a context to store request-specific information.
        //

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ContextAttributes,
                                                READ_REQUEST_CONTEXT);

        Status = WdfObjectAllocateContext(ReadRequest,
                                          &ContextAttributes,
                                          &Context);

        if(!NT_SUCCESS(Status)) {
            DebugPrint(SIMSENSOR_ERROR,
                       "WdfObjectAllocateContext() Failed. 0x%x",
                       Status);

            WdfRequestCompleteWithInformation(ReadRequest,
                                              Status,
                                              BytesReturned);

            goto AddReadRequestEnd;
        }

        Context->ExpirationTime.QuadPart = ExpirationTime.QuadPart;
        Context->LowTemperature = ThermalWaitRead->LowTemperature;
        Context->HighTemperature = ThermalWaitRead->HighTemperature;
        if(Context->ExpirationTime.QuadPart != -1LL /* INFINITE */ ) {

            //
            // This request eventually expires, create a timer to complete it.
            //

            WDF_TIMER_CONFIG_INIT(&TimerConfig, SimSensorExpiredRequestTimer);
            WDF_OBJECT_ATTRIBUTES_INIT(&TimerAttributes);
            TimerAttributes.ExecutionLevel = WdfExecutionLevelPassive;
            TimerAttributes.SynchronizationScope = WdfSynchronizationScopeNone;
            TimerAttributes.ParentObject = Device;
            Status = WdfTimerCreate(&TimerConfig,
                                    &TimerAttributes,
                                    &Timer);

            if(!NT_SUCCESS(Status)) {
                DebugPrint(SIMSENSOR_ERROR,
                        "WdfTimerCreate() Failed. 0x%x",
                        Status);

                WdfRequestCompleteWithInformation(ReadRequest,
                                                  Status,
                                                  BytesReturned);

                goto AddReadRequestEnd;
            }

            WdfTimerStart(Timer,
                          WDF_REL_TIMEOUT_IN_MS(ThermalWaitRead->Timeout));

        }

        Status = WdfRequestForwardToIoQueue(ReadRequest,
                                            DevExt->PendingRequestQueue);

        if(!NT_SUCCESS(Status)) {
            DebugPrint(SIMSENSOR_ERROR,
                       "WdfRequestForwardToIoQueue() Failed. 0x%x",
                       Status);

            WdfRequestCompleteWithInformation(ReadRequest,
                                              Status,
                                              BytesReturned);

            goto AddReadRequestEnd;
        }

        //
        // Force a rescan of the queue to update the interrupt thresholds.
        //

        SimSensorScanPendingQueue(Device);
    }

AddReadRequestEnd:

    if(LockHeld == TRUE) {
        WdfWaitLockRelease(DevExt->QueueLock);
    }

    DebugExitStatus(Status);
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
SimSensorScanPendingQueue (
    _In_ WDFDEVICE Device
    )

/*++

Routine Description:

    This routine scans the device's pending queue for retirable requests.

    N.B. This routine requires the QueueLock be held.

Arguments:

    Device - Supplies a handle to the device.

--*/

{
    WDFREQUEST CurrentRequest;
    PFDO_DATA DevExt;
    WDFREQUEST LastRequest;
    ULONG LowerBound;
    NTSTATUS Status;
    ULONG Temperature;
    ULONG UpperBound;

    DebugEnter();
    PAGED_CODE();

    DevExt = GetDeviceExtension(Device);

    Status = STATUS_SUCCESS;

    LastRequest = NULL;
    CurrentRequest = NULL;
    Temperature = SimSensorReadVirtualTemperature(Device);

    //
    // Prime the walk by finding the first request present. If there are no
    // requests, bail out immediately.
    //

    LowerBound = 0;
    UpperBound = (ULONG)-1;
    Status = WdfIoQueueFindRequest(DevExt->PendingRequestQueue,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &CurrentRequest);

    //
    // Due to a technical limitation in SDV analysis engine, the following
    // analysis assume has to be inserted to supress a false defect for
    // the wdfioqueueretrievefoundrequest rule.
    //

    _Analysis_assume_(Status == STATUS_NOT_FOUND);

    while (NT_SUCCESS(Status)) {

        //
        // Walk past the current request. By walking past the current request
        // before checking it, the walk doesn't have to restart every time a
        // request is satisfied and removed form the queue.
        //

        LastRequest = CurrentRequest;
        Status = WdfIoQueueFindRequest(DevExt->PendingRequestQueue,
                                       LastRequest,
                                       NULL,
                                       NULL,
                                       &CurrentRequest);

        //
        // Process the last request.
        //

        SimSensorCheckQueuedRequest(Device,
                                    Temperature,
                                    &LowerBound,
                                    &UpperBound,
                                    LastRequest);

        WdfObjectDereference(LastRequest);

        if(Status == STATUS_NOT_FOUND) {

            //
            // LastRequest unexpectedly disappeared from the queue. Start over.
            //

            LowerBound = 0;
            UpperBound = (ULONG)-1;
            Status = WdfIoQueueFindRequest(DevExt->PendingRequestQueue,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &CurrentRequest);

        }

    }

    //
    // Update the thresholds based on the latest contents of the queue.
    //

    SimSensorSetVirtualInterruptThresholds(Device, LowerBound, UpperBound);
    DebugExitStatus(Status);
    return Status;
}


NTSTATUS
SimSensorSelfManagedIoSuspend (
    _In_ WDFDEVICE Device
    )

/*++

Routine Description:

    Stops self-managed IO queues in preparation for D0 exit.

Return Value:

    NTSTATUS

--*/

{

    PFDO_DATA DevExt;

    PAGED_CODE();

    DevExt = GetDeviceExtension(Device);
    WdfIoQueueStopSynchronously(DevExt->PendingRequestQueue);

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
SimSensorCheckQueuedRequest (
    _In_ WDFDEVICE Device,
    _In_ ULONG Temperature,
    _Inout_ PULONG LowerBound,
    _Inout_ PULONG UpperBound,
    _In_ WDFREQUEST Request
    )

/*++

Routine Description:

    Examines a request and performs one of the following actions:

    * Retires the request if it is satisfied (the sensor temperature has
      exceeded the bounds specified in the request)

    * Retires the request if it is expired (the timer due time is in the past)

    * Tightens the upper and lower bounds if the request remains in the queue.

Arguments:

    Device - Supplies a handle to the device which owns this request.

    Temperature - Supplies the current thermal zone temperature.

    LowerBound - Supplies the lower bound threshold to adjust.

    UpperBound - Supplies the upper bound threshold to adjust.

    Request - Supplies a handle to the request.

--*/

{
    ULONG BytesReturned;
    LARGE_INTEGER CurrentTime;
    PFDO_DATA DevExt;
    size_t Length;
    PREAD_REQUEST_CONTEXT Context;
    WDFREQUEST RetrievedRequest;
    PULONG RequestTemperature;
    NTSTATUS Status;

    DebugEnter();
    PAGED_CODE();

    KeQuerySystemTime(&CurrentTime);
    DevExt = GetDeviceExtension(Device);
    Context = WdfObjectGetTypedContext(Request, READ_REQUEST_CONTEXT);

    RetrievedRequest = NULL;

    //
    // Complete the request if:
    //
    // 1. The temperature has exceeded one of the request thresholds.
    // 2. The request timeout is in the past (but not negative).
    //

    if (SimSensorAreConstraintsSatisfied(Temperature,
                                         Context->LowTemperature,
                                         Context->HighTemperature,
                                         Context->ExpirationTime)) {

        Status = WdfIoQueueRetrieveFoundRequest(DevExt->PendingRequestQueue,
                                                Request,
                                                &RetrievedRequest);

        if(!NT_SUCCESS(Status)) {
            DebugPrint(SIMSENSOR_ERROR,
                       "WdfIoQueueRetrieveFoundRequest() Failed. 0x%x",
                       Status);

            //
            // Bail, likely because the request disappeared from the
            // queue.
            //

            goto CheckQueuedRequestEnd;
        }

        Status = WdfRequestRetrieveOutputBuffer(RetrievedRequest,
                                                sizeof(ULONG),
                                                &RequestTemperature,
                                                &Length);

        if(NT_SUCCESS(Status) && (Length == sizeof(ULONG))) {
            *RequestTemperature = Temperature;
            BytesReturned = sizeof(ULONG);

        } else {

            //
            // The request's return buffer is malformed.
            //

            BytesReturned = 0;
            Status = STATUS_INVALID_PARAMETER;
            DebugPrint(SIMSENSOR_ERROR,
                       "WdfRequestRetrieveOutputBuffer() Failed. 0x%x",
                        Status);

        }

        WdfRequestCompleteWithInformation(RetrievedRequest,
                                          Status,
                                          BytesReturned);

    } else {

        //
        // The request will remain in the queue. Update the bounds accordingly.
        //

        if (*LowerBound < Context->LowTemperature) {
            *LowerBound = Context->LowTemperature;
        }

        if (*UpperBound > Context->HighTemperature) {
            *UpperBound = Context->HighTemperature;
        }
    }

CheckQueuedRequestEnd:
    DebugExit();
    return;
}

VOID
SimSensorExpiredRequestTimer (
    WDFTIMER Timer
    )

/*++

Routine Description:

    This routine is invoked when a request timer expires. A scan of the pending
    queue to complete expired and satisfied requests is initiated.

Arguments:

    Timer - Supplies a handle to the timer which expired.

--*/

{

    PFDO_DATA DevExt;
    WDFDEVICE Device;

    DebugEnter();
    PAGED_CODE();

    Device = (WDFDEVICE)WdfTimerGetParentObject(Timer);
    DevExt = GetDeviceExtension(Device);
    WdfWaitLockAcquire(DevExt->QueueLock, NULL);
    SimSensorScanPendingQueue(Device);
    WdfWaitLockRelease(DevExt->QueueLock);

    DebugExit();
}

VOID
SimSensorTemperatureInterruptWorker (
    _In_ WDFWORKITEM WorkItem
    )

/*++

Routine Description:

    This routine is invoked to call into the device to notify it of a
    temperature change.

Arguments:

    WorkItem - Supplies a handle to this work item.

Return Value:

    None.

--*/

{

    PFDO_DATA DevExt;
    WDFDEVICE Device;

    Device = (WDFDEVICE)WdfWorkItemGetParentObject(WorkItem);
    DevExt = GetDeviceExtension(Device);
    WdfWaitLockAcquire(DevExt->QueueLock, NULL);
    SimSensorScanPendingQueue(Device);
    WdfWaitLockRelease(DevExt->QueueLock);
    return;
}

VOID
SimSensorTemperatureInterrupt (
    _In_ WDFDEVICE Device
    )


/*++

Routine Description:

    This routine is invoked to simulate an interrupt from the virtual sensor
    device. It performs all the work a normal ISR would perform.

Arguments:

    Device - Supplies a handle to the device.

Return Value:

    None.

--*/

{

    PFDO_DATA DevExt;

    DevExt = GetDeviceExtension(Device);
    WdfWorkItemEnqueue(DevExt->InterruptWorker);
    return;
}



//-------------------------------------------------- Virtual Temperature Sensor

NTSTATUS
SimSensorDeviceD0Entry (
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )

/*++

Routine Description:

    This routine is invoked when the device enters the D0 power state, and
    registers for the power policy setting that drives the virtual interrupt.

Arguments:

    Device - Supplies a handle to the device that is powering up.

    PreviousState - Supplies the previous power state of the device.

Return Value:

    NTSTATUS

--*/

{

    PFDO_DATA DevExt;
    PVOID Handle;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(PreviousState);

    Status = PoRegisterPowerSettingCallback(
                 WdfDeviceWdmGetDeviceObject(Device),
                 &GUID_VIRTUAL_TEMPERATURE_SENSOR,
                 SimSensorSettingCallback,
                 (PVOID)Device,
                 &Handle);

    if (NT_SUCCESS(Status)) {
        DevExt = GetDeviceExtension(Device);
        DevExt->Sensor.PolicyHandle = Handle;
    }

    return Status;
}

NTSTATUS
SimSensorDeviceD0Exit (
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )

/*++

Routine Description:

    This routine is invoked when the device exits the D0 power state, and
    unregisters the power policy setting that drives the virtual interrupt.

Arguments:

    Device - Supplies a handle to the device that is powering up.

    TargetState - Supplies the next power state of the device.

Return Value:

    NTSTATUS

--*/

{

    PFDO_DATA DevExt;

    UNREFERENCED_PARAMETER(TargetState);

    DevExt = GetDeviceExtension(Device);
    if (DevExt->Sensor.PolicyHandle != NULL) {
        PoUnregisterPowerSettingCallback(DevExt->Sensor.PolicyHandle);
        DevExt->Sensor.PolicyHandle = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SimSensorSettingCallback (
    _In_ LPCGUID SettingGuid,
    _In_reads_bytes_(ValueLength) PVOID Value,
    _In_ ULONG ValueLength,
    _Inout_opt_ PVOID Context
    )

/*++

Routine Description:

    This routine is invoked to notify the device of a change to the power
    setting that drives the virtual temperature sensor interrupt.

Arguments:

    SettingGuid - Supplies the GUID of the power setting that changed.

    Value - Supplies the power setting value.

    ValueLength - Supplies the power setting value.

    Context - Supplies the device to update.

Return Value:

    NTSTATUS

--*/

{

    PFDO_DATA DevExt;
    BOOLEAN Interrupt;
    NTSTATUS Status;
    ULONG Temperature;

    UNREFERENCED_PARAMETER(SettingGuid);

    if (ValueLength != sizeof(ULONG)) {
        Status = STATUS_INVALID_PARAMETER;
        goto SettingCallbackEnd;
    }

    Temperature = *(PULONG)Value;

    _Analysis_assume_(Context != NULL);
    NT_ASSERT(Context != NULL);
    DevExt = GetDeviceExtension((WDFDEVICE)Context);

    WdfWaitLockAcquire(DevExt->Sensor.Lock, NULL);

    //
    // If the policy setting has reached the reset value, enable the
    // temperature sensor. This prevents the policy from being set to a high
    // temperture, causing a critical shutdown, and then starting out above
    // the critical shutdown tempertaure at the next boot.
    //

    if (Temperature == VIRTUAL_SENSOR_RESET_TEMPERATURE) {
        DevExt->Sensor.Enabled = TRUE;
    }

    //
    // If the reset value hasn't been reached yet, use a known safe low
    // temperature as a placeholder.
    //

    if (DevExt->Sensor.Enabled == FALSE) {
        Temperature = VIRTUAL_SENSOR_RESET_TEMPERATURE;
    }

    //
    // Special case boundary temperature values to avoid logic for handling
    // them elsewhere. Avoid the boundary values because it is impossible for
    // the system to request a lower or higher value. This should not be an
    // issue for a real sensor device.
    //

    if (Temperature == 0) {
        Temperature = 1;
    }

    if (Temperature == (ULONG)-1) {
        Temperature = (ULONG)-2;
    }

    DevExt->Sensor.Temperature = Temperature;

    //
    // Check to see if the temperature has exceeded either of the thresholds
    // for noticing a temperature change. If so, the virtual interrupt will
    // need to be fired.
    //

    if ((DevExt->Sensor.Temperature < DevExt->Sensor.LowerBound) ||
        (DevExt->Sensor.Temperature > DevExt->Sensor.UpperBound)) {

        Interrupt = TRUE;

    } else {
        Interrupt = FALSE;
    }

    WdfWaitLockRelease(DevExt->Sensor.Lock);

    //
    // Fire the virtual interrupt outside the lock, to avoid any locking issues.
    //

    if (Interrupt != FALSE) {
        SimSensorTemperatureInterrupt((WDFDEVICE)Context);
    }

    Status = STATUS_SUCCESS;

SettingCallbackEnd:
    return Status;
}

VOID
SimSensorSetVirtualInterruptThresholds (
    _In_ WDFDEVICE Device,
    _In_ ULONG LowerBound,
    _In_ ULONG UpperBound
    )

/*++

Routine Description:

    This routine is invoked to change the thresholds the virtual sensor driver
    uses to compare against for an interrupt to occur.

Arguments:

    Device - Supplies a handle to the device.

    LowerBound - Supplies the temperature below which the device should issue
        an interrupt.

    UpperBound - Supplies the temperature above which the device should issue
        an interrupt.

Return Value:

    NTSTATUS

--*/

{

    PFDO_DATA DevExt;
    BOOLEAN Interrupt;

    DevExt = GetDeviceExtension(Device);
    WdfWaitLockAcquire(DevExt->Sensor.Lock, NULL);

    DevExt->Sensor.LowerBound = LowerBound;
    DevExt->Sensor.UpperBound = UpperBound;
    if ((DevExt->Sensor.Temperature < DevExt->Sensor.LowerBound) ||
        (DevExt->Sensor.Temperature > DevExt->Sensor.UpperBound)) {

        Interrupt = TRUE;

    } else {
        Interrupt = FALSE;
    }

    WdfWaitLockRelease(DevExt->Sensor.Lock);

    //
    // Fire the virtual interrupt outside the lock, to avoid any locking issues.
    //

    if (Interrupt != FALSE) {
        SimSensorTemperatureInterrupt(Device);
    }

    return;
}

ULONG
SimSensorReadVirtualTemperature (
    _In_ WDFDEVICE Device
    )

/*++

Routine Description:

    This routine is invoked to read the current temperature of the device.

Arguments:

    Device - Supplies a handle to the device.

    LowerBound - Supplies the temperature below which the device should issue
        an interrupt.

    UpperBound - Supplies the temperature above which the device should issue
        an interrupt.

Return Value:

    NTSTATUS

--*/

{

    PFDO_DATA DevExt;
    ULONG Temperature;

    DevExt = GetDeviceExtension(Device);
    WdfWaitLockAcquire(DevExt->Sensor.Lock, NULL);
    Temperature = DevExt->Sensor.Temperature;
    WdfWaitLockRelease(DevExt->Sensor.Lock);
    return Temperature;
}


