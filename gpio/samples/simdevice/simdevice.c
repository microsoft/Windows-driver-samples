/*++

Copyright (c) 1990-2010  Microsoft Corporation

Module Name:

    SimpleDevice.c

Abstract:

    This is a simple device driver that consumes GPIO pins for I/O and interrupt.


Environment:

    Kernel mode

--*/

//
// ------------------------------------------------------------------- Includes
//

#include "common.h"
#include <gpio.h>
#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>

//
// -------------------------------------------------------------------- Defines
//

#define MAX_NUMBER_IO_RESOURCES 2

//
// -------------------------------------------------------------------- Types
//

typedef struct _SAMPLE_DRV_DEVICE_EXTENSION {
    ULONG IoResourceCount;
    ULONG InterruptCount;
    LARGE_INTEGER ConnectionIds[MAX_NUMBER_IO_RESOURCES];
} SAMPLE_DRV_DEVICE_EXTENSION, *PSAMPLE_DRV_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(SAMPLE_DRV_DEVICE_EXTENSION, SampleDrvGetDeviceExtension)

//
// ----------------------------------------------------------------- Prototypes
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD SampleDrvEvtDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY SampleDrvEvtDeviceD0Entry;
EVT_WDF_DEVICE_PREPARE_HARDWARE SampleDrvEvtDevicePrepareHardware;
EVT_WDF_INTERRUPT_DPC SampleDrvInterruptDpc;
EVT_WDF_INTERRUPT_ISR SampleDrvInterruptIsr;
EVT_WDF_INTERRUPT_ISR SampleDrvInterruptPassiveCallback;

NTSTATUS
TestReadWrite (
    _In_ WDFDEVICE Device,
    _In_ PCUNICODE_STRING RequestString,
    _In_ BOOLEAN ReadOperation,
    _Inout_ PUCHAR Data,
    _In_ _In_range_(>, 0) ULONG Size,
    _Out_ WDFIOTARGET *IoTargetOut
    );

//
// -------------------------------------------------------------------- Pragmas
//

#pragma alloc_text(PAGE, SampleDrvEvtDeviceAdd)
#pragma alloc_text(PAGE, SampleDrvEvtDevicePrepareHardware)

//
// ------------------------------------------------------------------ Functions
//

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the driver initialization entry point.

Arguments:

    DriverObject - Pointer to the driver object created by the I/O manager.

    RegistryPath - Pointer to the driver specific registry key.

Return Value:

    NTSTATUS code.

--*/

{

    WDFDRIVER Driver;
    WDF_DRIVER_CONFIG DriverConfig;
    NTSTATUS Status;

    //
    // Initialize the driver configuration structure.
    //

    WDF_DRIVER_CONFIG_INIT(&DriverConfig, SampleDrvEvtDeviceAdd);

    //
    // Create a framework driver object to represent our driver.
    //

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &DriverConfig,
                             &Driver);

    if (!NT_SUCCESS(Status)) {
        goto DriverEntryEnd;
    }

DriverEntryEnd:
    return Status;
}

BOOLEAN
SampleDrvInterruptIsr (
    _In_ WDFINTERRUPT Interrupt,
    _In_ ULONG MessageID
    )

/*++

Routine Description:

    This routine is the interrupt service routine for the sample device

    N.B. This driver assumes that the interrupt line is not shared with any
         other device. Hence it always claims the interrupt.

Arguments:

    Interupt - Supplies a handle to interrupt object (WDFINTERRUPT) for this
        device.

    MessageID - Supplies the MSI message ID for MSI-based interrupts.

Return Value:

    Always TRUE.

--*/

{


    UNREFERENCED_PARAMETER(Interrupt);
    UNREFERENCED_PARAMETER(MessageID);

    //
    //  The sample driver always returns TRUE (e.g. claiming the interrupt)
    //  from its ISR. In reality, the driver needs to do whatever necessary to
    //  quiesce the interrupt before claiming the interrupt. In case of spurious
    //  interrupts, the ISR returns FALSE. If additional work needs to be done
    //  at a lower IRQL, schedule a DPC.
    //

    return TRUE;
}

#if 0
BOOLEAN
SampleDrvInterruptPassiveCallback (
    _In_ WDFINTERRUPT Interrupt,
    _In_ ULONG MessageID
    )

/*++

Routine Description:

    This routine is the passive interrupt callback routine for the sample device.
    As its name suggests, this routine is always invoked at PASSIVE_LEVEL.
    This is useful in scenarios where the device is located behind a slow serial
    peripheral bus(SPB) and requires communication (possible only at PASSIVE_LEVEL)
    over the bus in quiescing the interrupt source.

    N.B. It is possible for passive interrupt callback and DIRQL ISRs to
    coexist for the same device and/or IDT entry. Interrupt objects chained to a
    given IDT entry are always ordered (at interrupt connect time) by the OS such
    that the DIRQL ISR interrupt objects are located before the passive callback
    ones. Consequently, during interrupt dispatching, the OS would walk the list
    in that order until the first ISR/passive callback returns TRUE to claim the
    interrupt.

Arguments:

    Interupt - Supplies a handle to interrupt object (WDFINTERRUPT) for this
        device.

    MessageID - Supplies the MSI message ID for MSI-based interrupts.

Return Value:

    Always TRUE.

--*/

{


    UNREFERENCED_PARAMETER(Interrupt);
    UNREFERENCED_PARAMETER(MessageID);

    //
    //  The sample driver always returns TRUE (e.g. claiming the interrupt)
    //  from its passive callback. In reality, the driver needs to do whatever necessary to
    //  quiesce the interrupt before claiming the interrupt.
    //

    return TRUE;
}
#endif

VOID
SampleDrvInterruptDpc (
    _In_ WDFINTERRUPT WdfInterrupt,
    _In_ WDFOBJECT WdfDevice
    )

/*++

Routine Description:

    This routine is the DPC callback for the ISR. This routine is unused.

Arguments:

    Interupt - Supplies a handle to interrupt object (WDFINTERRUPT) for this
        device.

    Device - Supplies a handle to the framework device object.

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER(WdfInterrupt);
    UNREFERENCED_PARAMETER(WdfDevice);
    return;
}

_Use_decl_annotations_
NTSTATUS
SampleDrvEvtDeviceAdd (
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    This routine is the AddDevice entry point for the sample device driver.
    It sets the ISR and DPC routine handlers for the interrupt and the passive
    level callback for the passive interrupt

    N.B. The sample device expects two interrupt resources in connecting its
    DIRQL ISR and PASSIVE_LEVEL callback.

Arguments:

    Driver - Supplies a handle to the driver object created in DriverEntry.

    DeviceInit - Supplies a pointer to a framework-allocated WDFDEVICE_INIT
        structure.

Return Value:

    NTSTATUS code.

--*/

{

    WDF_PNPPOWER_EVENT_CALLBACKS Callbacks;
    WDFDEVICE Device;
    WDF_OBJECT_ATTRIBUTES FdoAttributes;
    WDF_INTERRUPT_CONFIG InterruptConfiguration;
    NTSTATUS Status;
    WDFINTERRUPT WdfInterrupt;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    //
    // Set PnP callbacks for prepare/release hardware and D0 entry/exit. All
    // callbacks not overriden here will be handled by the framework in the
    // default manner.
    //

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&Callbacks);
    Callbacks.EvtDevicePrepareHardware = SampleDrvEvtDevicePrepareHardware;
    Callbacks.EvtDeviceD0Entry = SampleDrvEvtDeviceD0Entry;

    //
    // Register the PnP callbacks with the framework.
    //

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &Callbacks);

    //
    // Initialize FDO attributes with the sample device extension.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&FdoAttributes);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&FdoAttributes, SAMPLE_DRV_DEVICE_EXTENSION);

    //
    // Call the framework to create the device and attach it to the lower stack.
    //

    Status = WdfDeviceCreate(&DeviceInit, &FdoAttributes, &Device);
    if (!NT_SUCCESS(Status)) {
        goto EvtDeviceAddEnd;
    }

    //
    // Create an interrupt object. For the KMDF driver, the ISR will be run at
    // DIRQL. For UMDF(v2), it will be run at PASSIVE_LEVEL, hence the DPC is
    // only defined in the KMDF case.
    //

    WDF_INTERRUPT_CONFIG_INIT(&InterruptConfiguration,
                              SampleDrvInterruptIsr,
#ifdef _KERNEL_MODE
                              SampleDrvInterruptDpc
#else
                              NULL
#endif
                              );

    Status = WdfInterruptCreate(Device,
                                &InterruptConfiguration,
                                WDF_NO_OBJECT_ATTRIBUTES,
                                &WdfInterrupt);

    if (!NT_SUCCESS(Status)) {

        goto EvtDeviceAddEnd;
    }

#if 0
    //
    // Create an interrupt object for the passive interrupt callback. Note that
    // the interrupt object is chained to the same interrupt line/IDT as the
    // DIRQL one.
    //

    WDF_INTERRUPT_CONFIG_INIT(&InterruptConfiguration,
                              SampleDrvInterruptIsr,
                              NULL);

    //
    //  Set passive handling to true
    //

    InterruptConfiguration.PassiveHandling = TRUE;

    Status = WdfInterruptCreate(Device,
                                &InterruptConfiguration,
                                WDF_NO_OBJECT_ATTRIBUTES,
                                &WdfInterrupt);
#endif

EvtDeviceAddEnd:
    return Status;
}

NTSTATUS
SampleDrvEvtDevicePrepareHardware (
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
    )

/*++

Routine Description:

    This routine is called by the framework when the PnP manager sends an
    IRP_MN_START_DEVICE request to the driver stack.

Arguments:

    Device - Supplies a handle to a framework device object.

    ResourcesRaw - Supplies a handle to a collection of framework resource
        objects. This collection identifies the raw (bus-relative) hardware
        resources that have been assigned to the device.

    ResourcesTranslated - Supplies a handle to a collection of framework
        resource objects. This collection identifies the translated
        (system-physical) hardware resources that have been assigned to the
        device. The resources appear from the CPU's point of view.

Return Value:

    NT status code.

--*/

{

    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PSAMPLE_DRV_DEVICE_EXTENSION SampleDrvExtension;
    ULONG Index;
    ULONG ResourceCount;
    NTSTATUS Status;
    ULONG IoResourceIndex;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesRaw);

    PAGED_CODE();

    SampleDrvExtension = SampleDrvGetDeviceExtension(Device);
    Status = STATUS_SUCCESS;
    IoResourceIndex = 0;

    SampleDrvExtension->InterruptCount = 0;

    //
    // Walk through the resource list and map all the resources. Only one
    // memory resource and one interrupt is expected.
    //

    ResourceCount = WdfCmResourceListGetCount(ResourcesTranslated);
    for (Index = 0; Index < ResourceCount; Index += 1) {
        Descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, Index);
        switch(Descriptor->Type) {

        //
        // This memory resource supplies the base of the device registers.
        //

        case CmResourceTypeConnection:

            //
            //  Check against expected connection type
            //

            if ((Descriptor->u.Connection.Class ==
                 CM_RESOURCE_CONNECTION_CLASS_GPIO) &&
                (Descriptor->u.Connection.Type ==
                 CM_RESOURCE_CONNECTION_TYPE_GPIO_IO)) {

                SampleDrvExtension->ConnectionIds[IoResourceIndex].LowPart =
                    Descriptor->u.Connection.IdLowPart;
                SampleDrvExtension->ConnectionIds[IoResourceIndex].HighPart =
                      Descriptor->u.Connection.IdHighPart;
                IoResourceIndex++;
            } else {

                Status = STATUS_UNSUCCESSFUL;
            }

            break;

        //
        //  Interrupt resource
        //

        case CmResourceTypeInterrupt:
            SampleDrvExtension->InterruptCount++;

        default:
            break;
        }

        if (!NT_SUCCESS(Status)) {
            goto DevicePrepareHardwareEnd;
        }
    }

    //
    //  Ensure that at least one interrupt resource is defined.
    //

#ifdef _KERNEL_MODE
    NT_ASSERT(SampleDrvExtension->InterruptCount > 0);
#endif

    if (SampleDrvExtension->InterruptCount < 1) {
        Status = STATUS_UNSUCCESSFUL;
        goto DevicePrepareHardwareEnd;
    }

    //
    //  Store the number of GPIO IO connection strings
    //

    SampleDrvExtension->IoResourceCount = IoResourceIndex;

DevicePrepareHardwareEnd:
    return Status;
}

NTSTATUS
SampleDrvEvtDeviceD0Entry (
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousPowerState
    )

/*++

Routine Description:

    This routine is invoked by the framework to program the device to goto
    D0, which is the working state. The framework invokes callback every
    time the hardware needs to be (re-)initialized.  This includes after
    IRP_MN_START_DEVICE, IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE,
    and IRP_MN_SET_POWER-D0.

    N.B. This function is not marked pageable because this function is in
         the device power up path. When a function is marked pagable and the
         code section is paged out, it will generate a page fault which could
         impact the fast resume behavior because the client driver will have
         to wait until the system drivers can service this page fault.

Arguments:

    Device - Supplies a handle to the framework device object.

    PreviousPowerState - WDF_POWER_DEVICE_STATE-typed enumerator that identifies
        the device power state that the device was in before this transition
        to D0.

Return Value:

    NTSTATUS code. A failure here will indicate a fatal error and cause the
    framework to tear down the stack.

--*/

{

    BYTE Data;
    NTSTATUS Status;
    WDFIOTARGET ReadTarget;
    WDFIOTARGET WriteTarget;
    PSAMPLE_DRV_DEVICE_EXTENSION SampleDrvExtension;
    UNICODE_STRING ReadString;
    WCHAR ReadStringBuffer[100];
    UNICODE_STRING WriteString;
    WCHAR WriteStringBuffer[100];

    UNREFERENCED_PARAMETER(PreviousPowerState);

    ReadTarget = NULL;
    WriteTarget = NULL;

    SampleDrvExtension = SampleDrvGetDeviceExtension(Device);

    //
    //  For demonstration purporses, the sample device consumes two IO resources,
    //  the first of which will be used for input, and the second for output.
    //

    RtlInitEmptyUnicodeString(&ReadString,
                              ReadStringBuffer,
                              sizeof(ReadStringBuffer));

    RtlInitEmptyUnicodeString(&WriteString,
                              WriteStringBuffer,
                              sizeof(WriteStringBuffer));


    //
    //  Construct full-path string for GPIO read operation
    //

    Status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&ReadString,
                                              SampleDrvExtension->ConnectionIds[0].LowPart,
                                              SampleDrvExtension->ConnectionIds[0].HighPart);

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    //  Construct full-path string for GPIO write operation
    //

    Status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&WriteString,
                                              SampleDrvExtension->ConnectionIds[1].LowPart,
                                              SampleDrvExtension->ConnectionIds[1].HighPart);

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Perform the read operation
    //

    Data = 0x0;
    Status = TestReadWrite(Device, &ReadString, TRUE, &Data, sizeof(Data), &ReadTarget);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    //  Perform the write operation
    //

    Status = TestReadWrite(Device, &WriteString, FALSE, &Data, sizeof(Data), &WriteTarget);
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

Cleanup:

    if (ReadTarget != NULL) {
        WdfIoTargetClose(ReadTarget);
        WdfObjectDelete(ReadTarget);
    }

    if (WriteTarget != NULL) {
        WdfIoTargetClose(WriteTarget);
        WdfObjectDelete(WriteTarget);
    }

    return Status;
}

NTSTATUS
TestReadWrite (
    _In_ WDFDEVICE Device,
    _In_ PCUNICODE_STRING RequestString,
    _In_ BOOLEAN ReadOperation,
    _Inout_ PUCHAR Data,
    _In_ _In_range_(>, 0) ULONG Size,
    _Out_ WDFIOTARGET *IoTargetOut
    )

/*++

Routine Description:

    This is a utility routine to test read or write on a set of GPIO pins.

Arguments:

    Device - Supplies a handle to the framework device object.

    RequestString - Supplies a pointer to the unicode string to be opened.

    ReadOperation - Supplies a boolean that identifies whether read (TRUE) or
        write (FALSE) should be performed.

    Data - Supplies a pointer containing the buffer that should be read from
        or written to.

    Size - Supplies the size of the data buffer in bytes.

    IoTargetOut - Supplies a pointer that receives the IOTARGET created by
        WDF.

Return Value:

    None.

--*/

{

    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFREQUEST IoctlRequest;
    WDFIOTARGET IoTarget;
    ULONG DesiredAccess;
    WDFMEMORY WdfMemory;
    WDF_OBJECT_ATTRIBUTES RequestAttributes;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    NTSTATUS Status;
    WDF_OBJECT_ATTRIBUTES ObjectAttributes;
    WDF_IO_TARGET_OPEN_PARAMS OpenParams;

    WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
    ObjectAttributes.ParentObject = Device;

    IoctlRequest = NULL;
    IoTarget = NULL;

    if ((Data == NULL) || (Size == 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto TestReadWriteEnd;
    }

    Status = WdfIoTargetCreate(Device,
                               &ObjectAttributes,
                               &IoTarget);

    if (!NT_SUCCESS(Status)) {
        goto TestReadWriteEnd;
    }

    //
    //  Specify desired file access
    //

    if (ReadOperation != FALSE) {
        DesiredAccess = FILE_GENERIC_READ;

    } else {
        DesiredAccess = FILE_GENERIC_WRITE;
    }

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&OpenParams,
                                                RequestString,
                                                DesiredAccess);

    //
    //  Open the IoTarget for I/O operation
    //

    Status = WdfIoTargetOpen(IoTarget, &OpenParams);
    if (!NT_SUCCESS(Status)) {
        goto TestReadWriteEnd;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&RequestAttributes);
    Status = WdfRequestCreate(&RequestAttributes, IoTarget, &IoctlRequest);
    if (!NT_SUCCESS(Status)) {
        goto TestReadWriteEnd;
    }

    //
    // Set up a WDF memory object for the IOCTL request
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = IoctlRequest;
    Status = WdfMemoryCreatePreallocated(&Attributes, Data, Size, &WdfMemory);
    if (!NT_SUCCESS(Status)) {
        goto TestReadWriteEnd;
    }

    //
    // Format the request as read or write operation
    //

    if (ReadOperation != FALSE) {
        Status = WdfIoTargetFormatRequestForIoctl(IoTarget,
                                                  IoctlRequest,
                                                  IOCTL_GPIO_READ_PINS,
                                                  NULL,
                                                  0,
                                                  WdfMemory,
                                                  0);

    } else {
        Status = WdfIoTargetFormatRequestForIoctl(IoTarget,
                                                  IoctlRequest,
                                                  IOCTL_GPIO_WRITE_PINS,
                                                  WdfMemory,
                                                  0,
                                                  WdfMemory,
                                                  0);
    }

    if (!NT_SUCCESS(Status)) {
        goto TestReadWriteEnd;
    }

    //
    // Send the request synchronously with an arbitrary timeout of 60 seconds
    //

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions,
                                  WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions,
                                         WDF_REL_TIMEOUT_IN_SEC(60));

    Status = WdfRequestAllocateTimer(IoctlRequest);
    if (!NT_SUCCESS(Status)) {
        goto TestReadWriteEnd;
    }

    if (!WdfRequestSend(IoctlRequest, IoTarget, &SendOptions)) {
        Status = WdfRequestGetStatus(IoctlRequest);
    }

    if (NT_SUCCESS(Status)) {
        *IoTargetOut = IoTarget;
    }

TestReadWriteEnd:
    if (IoctlRequest != NULL) {
        WdfObjectDelete(IoctlRequest);
    }

    if (!NT_SUCCESS(Status) && (IoTarget != NULL)) {
        WdfIoTargetClose(IoTarget);
        WdfObjectDelete(IoTarget);
    }

    return Status;
}



