/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    wdf.c

Abstract:

    The module implements WDF boilerplate for the simulate power limit client.

--*/

//-------------------------------------------------------------------- Includes

#include "plclient.h"

//--------------------------------------------------------------------- Globals

WDFWAITLOCK GlobalMutex;

//------------------------------------------------------------------ Prototypes

DRIVER_INITIALIZE                   DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD           EvtDriverDeviceAdd;
EVT_WDF_DRIVER_UNLOAD               EvtDriverUnload;
EVT_WDF_OBJECT_CONTEXT_DESTROY      EvtDeviceDestroy;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  EvtIoDeviceControl;

//--------------------------------------------------------------------- Pragmas

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, EvtDriverDeviceAdd)
#pragma alloc_text(PAGE, EvtDriverUnload)
#pragma alloc_text(PAGE, EvtIoDeviceControl)
#pragma alloc_text(PAGE, EvtDeviceDestroy)

//------------------------------------------------------------------- Functions

_Use_decl_annotations_
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
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

    WDF_DRIVER_CONFIG DriverConfig;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(RegistryPath);

    //
    // Initiialize the DriverConfig data that controls the attributes that are
    // global to this driver.
    //

    DebugEnter();
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, EvtDriverDeviceAdd);
    DriverConfig.EvtDriverUnload = EvtDriverUnload;

    //
    // Create the driver object
    //

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &DriverConfig,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLCLIENT_PRINT_ERROR,
                   "WdfDriverCreate() Failed. Status 0x%x\n",
                   Status);

        goto DriverEntryEnd;
    }

    //
    // Initialize global mutex.
    //

    Status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &GlobalMutex);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLCLIENT_PRINT_ERROR,
                   "WdfWaitLockCreate() Failed! 0x%x\n",
                   Status);

        goto DriverEntryEnd;
    }

DriverEntryEnd:
    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
NTSTATUS
EvtDriverDeviceAdd (
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    This routine is called by the framework in response to AddDevice call from
    the PnP manager. A WDF device object is created and initialized to represent
    a new instance of the power limit client device.

Arguments:

    Driver - Supplies a handle to the WDF Driver object.

    DeviceInit - Supplies a pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/

{

    PFDO_DATA DevExt;
    WDF_OBJECT_ATTRIBUTES DeviceAttributes;
    WDFDEVICE DeviceHandle;
    POWER_LIMIT_INTERFACE PowerLimitInterface;
    WDFQUEUE Queue;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    WDF_QUERY_INTERFACE_CONFIG QueryInterfaceConfig;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    DevExt = NULL;

    DebugEnter();

    //
    // Initialize attributes and a context area for the device object.
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, FDO_DATA);
    DeviceAttributes.EvtDestroyCallback = &EvtDeviceDestroy;

    //
    // Create a framework device object.  This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //

    Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &DeviceHandle);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLCLIENT_PRINT_ERROR,
                   "WdfDeviceCreate() Failed. 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    //
    // Configure a default queue for IO requests. This queue processes requests
    // to read the simulated state.
    // 
    // N.B. Those IOCTLs supplies another approach to validate device driver 
    //      interface, which are not needed by production code.
    //

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig,
                                           WdfIoQueueDispatchSequential);

    QueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    Status = WdfIoQueueCreate(DeviceHandle,
                              &QueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &Queue);

    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLCLIENT_PRINT_ERROR,
                   "WdfIoQueueCreate() Failed. 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    //
    // Initialize the device extension.
    //

    DevExt = GetDeviceExtension(DeviceHandle);
    Status = InitPowerLimitValues(DevExt);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLCLIENT_PRINT_ERROR,
                   "InitPowerLimitValues() Failed. 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

    //
    // Create a device interface for this device to advertise the simulated
    // power limit client IO interface.
    //

    Status = WdfDeviceCreateDeviceInterface(
                DeviceHandle,
                &GUID_DEVINTERFACE_POWER_LIMIT,
                NULL);

    if (!NT_SUCCESS(Status)) {
        goto DriverDeviceAddEnd;
    }

    //
    // Create a driver interface for this device to advertise the power limit
    // interface.
    //

    RtlZeroMemory(&PowerLimitInterface, sizeof(PowerLimitInterface));
    PowerLimitInterface.Version = 1;
    PowerLimitInterface.Size = sizeof(PowerLimitInterface);
    PowerLimitInterface.Context = DeviceHandle;
    PowerLimitInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    PowerLimitInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;
    PowerLimitInterface.DomainCount = PLCLIENT_DEFAULT_DOMAIN_COUNT;
    PowerLimitInterface.QueryAttributes = PLCQueryAttributes;
    PowerLimitInterface.SetPowerLimit = PLCSetLimits;
    PowerLimitInterface.QueryPowerLimit = PLCQueryLimitValues;
    WDF_QUERY_INTERFACE_CONFIG_INIT(&QueryInterfaceConfig,
                                    (PINTERFACE)&PowerLimitInterface,
                                    &GUID_POWER_LIMIT_INTERFACE,
                                    NULL);

    Status = WdfDeviceAddQueryInterface(DeviceHandle, &QueryInterfaceConfig);
    if (!NT_SUCCESS(Status)) {
        DebugPrint(PLCLIENT_PRINT_ERROR,
                   "WdfDeviceAddQueryInterface() Failed. 0x%x\n",
                   Status);

        goto DriverDeviceAddEnd;
    }

DriverDeviceAddEnd:
    if (!NT_SUCCESS(Status)) {
        CleanupPowerLimitValues(DevExt);
    }

    DebugExitStatus(Status);
    return Status;
}

_Use_decl_annotations_
VOID
EvtDeviceDestroy (
    WDFOBJECT Object
    )

/*++

Routine Description:

    This routine destroys the device's data.

Arguments:

    Object - Supplies the WDF reference to the device that is being removed.

Return Value:

    None.

--*/

{

    PFDO_DATA DevExt;

    PAGED_CODE();

    DebugEnter();
    DevExt = GetDeviceExtension(Object);
    CleanupPowerLimitValues(DevExt);
    DebugExit();
    return;
}

_Use_decl_annotations_
VOID
EvtIoDeviceControl (
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
    )

/*++

Routine Description:

    Handles requests to read the simulated device state.

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
        code (IOCTL) that is associated with the request.

Return Value:

   VOID

--*/

{

    ULONG BytesReturned;
    WDFDEVICE Device;
    PFDO_DATA DevExt;
    PVOID OutputBuffer;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(InputBufferLength);

    PAGED_CODE();

    Device = WdfIoQueueGetDevice(Queue);
    DevExt = GetDeviceExtension(Device);
    DebugPrint(PLCLIENT_PRINT_TRACE,
               "EvtIoDeviceControl: 0x%08x\n",
               IoControlCode);

    BytesReturned = 0;
    OutputBuffer = NULL;
    if (OutputBufferLength > 0) {
        Status = WdfRequestRetrieveOutputBuffer(Request,
                                                OutputBufferLength,
                                                &OutputBuffer,
                                                NULL);

        if (!NT_SUCCESS(Status)) {
            goto DeviceIoControlEnd;
        }
    }

    Status = STATUS_NOT_SUPPORTED;
    switch(IoControlCode) {
    case IOCTL_POWERLIMIT_CLIENT_QUERY_LIMIT_COUNT:
        if (OutputBufferLength == sizeof(ULONG)) {
            AcquireGlobalMutex();
            *((PULONG)OutputBuffer) = DevExt->LimitCount;
            BytesReturned = sizeof(ULONG);
            ReleaseGlobalMutex();
            Status = STATUS_SUCCESS;

        } else {
            Status = STATUS_BUFFER_OVERFLOW;
        }

        break;

    case IOCTL_POWERLIMIT_CLIENT_QUERY_ATTRIBUTES:
        if (OutputBufferLength == sizeof(POWER_LIMIT_ATTRIBUTES) * DevExt->LimitCount) {
            AcquireGlobalMutex();
            RtlCopyMemory(OutputBuffer, DevExt->LimitAttributes, OutputBufferLength);
            ReleaseGlobalMutex();
            BytesReturned = (ULONG)OutputBufferLength;
            Status = STATUS_SUCCESS;

        } else {
            Status = STATUS_BUFFER_OVERFLOW;
        }

        break;

    case IOCTL_POWERLIMIT_CLIENT_QUERY_LIMITS:
        if (OutputBufferLength == sizeof(POWER_LIMIT_VALUE) * DevExt->LimitCount) {
            AcquireGlobalMutex();
            RtlCopyMemory(OutputBuffer, DevExt->LimitValues, OutputBufferLength);
            ReleaseGlobalMutex();
            BytesReturned = (ULONG)OutputBufferLength;
            Status = STATUS_SUCCESS;

        } else {
            Status = STATUS_BUFFER_OVERFLOW;
        }

        break;

    default:
        break;
    }

DeviceIoControlEnd:
    WdfRequestCompleteWithInformation(Request, Status, BytesReturned);
    DebugExitStatus(Status);
    return;
}

_Use_decl_annotations_
VOID
EvtDriverUnload (
    WDFDRIVER Driver
    )

/*++

Routine Description:

    EvtDriverUnload is called when the driver is being unloaded to clean up
    driver state.

Arguments:

    Driver - Supplies a handle to the WDF Driver object.

Return Value:

    None

--*/

{

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    DebugEnter();

    //
    // N.B. Does nothing since we don't have anything to clean up, just print
    //      some debug info.
    //

    DebugExit();
    return;
}
