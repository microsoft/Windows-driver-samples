/*--

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.


Module Name:

    PrmFuncSample.c

Abstract:

    This module implements a sample that utilizes the 
    Windows PRM direct call interface.

Environment:

    Kernel mode only.

--*/

#include "prmfuncsample.h"

//
// General client/class interfaces
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD PrmFuncTestEvtDeviceAdd;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, PrmFuncTestEvtDeviceAdd)
#pragma alloc_text (PAGE, PrmFuncTestEvtIoDeviceControl)
#endif

HANDLE FileHandle = NULL;

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry configures and creates a WDF driver
    object.
    .
Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
        into memory. DriverObject is allocated by the system before the
        driver is loaded, and it is released by the system after the system unloads
        the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
        The function driver can use the path to store driver related data between
        reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/

{
    WDF_DRIVER_CONFIG Config;
    WDFDRIVER Driver;
    NTSTATUS Status;

    KdPrint(("PRM Function Test Driver - Driver Framework Edition.\n"));

    WDF_DRIVER_CONFIG_INIT(
        &Config,
        PrmFuncTestEvtDeviceAdd);

    //
    // Create a framework driver object to represent our driver.
    //

    Status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES, // Driver Attributes
        &Config,                  // Driver Config Info
        &Driver);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("WdfDriverCreate failed with status 0x%x\n", Status));
        return Status;
    }

    return Status;
}

NTSTATUS
PrmFuncTestEvtDeviceAdd (
    _In_ WDFDRIVER       Driver,
    _In_ PWDFDEVICE_INIT DeviceInit
    )

/*++

Routine Description:

    ToasterEvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a WDF device object to
    represent a new instance of toaster device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                Status;
    WDF_IO_QUEUE_CONFIG     QueueConfig;
    WDFDEVICE               Device;
    WDFQUEUE                Queue;
    WDF_FILEOBJECT_CONFIG   FileConfig;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    KdPrint(("PrmFuncTestEvtDeviceAdd called\n"));

    //
    // Initialize WDF_FILEOBJECT_CONFIG_INIT struct to tell the
    // framework whether you are interested in handling Create, Close and
    // Cleanup requests that gets genereate when an application or another
    // kernel component opens an handle to the device. If you don't register,
    // the framework default behaviour would be complete these requests
    // with STATUS_SUCCESS. A driver might be interested in registering these
    // events if it wants to do security validation and also wants to maintain
    // per handle (fileobject) context.
    //

    WDF_FILEOBJECT_CONFIG_INIT(
        &FileConfig,
        PrmFuncTestEvtDeviceFileCreate,
        NULL,
        NULL);

    FileConfig.FileObjectClass = WdfFileObjectNotRequired;
    WdfDeviceInitSetFileObjectConfig(
        DeviceInit,
        &FileConfig,
        WDF_NO_OBJECT_ATTRIBUTES);

    //
    // Create a framework device object.This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //

    Status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &Device);
    if (!NT_SUCCESS(Status)) {
        KdPrint( ("WdfDeviceCreate failed with status code 0x%x\n", Status));
        return Status;
    }

    //
    // Tell the Framework that this device will need an interface
    //

    Status = WdfDeviceCreateDeviceInterface(
                 Device,
                 (LPGUID) &GUID_DEVINTERFACE_PRMFUNCTEST,
                 NULL);

    if (!NT_SUCCESS (Status)) {
        KdPrint( ("WdfDeviceCreateDeviceInterface failed 0x%x\n", Status));
        return Status;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig,  WdfIoQueueDispatchParallel);
    QueueConfig.EvtIoDeviceControl = PrmFuncTestEvtIoDeviceControl;
    Status = WdfIoQueueCreate(
        Device,
        &QueueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &Queue
        );

    if (!NT_SUCCESS (Status)) {
        KdPrint( ("WdfIoQueueCreate failed 0x%x\n", Status));
        return Status;
    }

    return Status;
}

VOID
PrmFuncTestEvtDeviceFileCreate (
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request,
    _In_ WDFFILEOBJECT FileObject
    )

/*++

Routine Description:

    The framework calls a driver's EvtDeviceFileCreate callback
    when the framework receives an IRP_MJ_CREATE request.
    The system sends this request when a user application opens the
    device to perform an I/O operation, such as reading or writing to a device.
    This callback is called in the context of the thread
    that created the IRP_MJ_CREATE request.

Arguments:

    Device - Handle to a framework device object.
    FileObject - Pointer to fileobject that represents the open handle.
    CreateParams - Parameters for create

Return Value:

   NT status code

--*/

{

    PIRP Irp;
    WDFIOTARGET IoTarget;
    WDF_REQUEST_SEND_OPTIONS  RequestSendOptions;
    UNICODE_STRING FilePath;
    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PWSTR DeviceNames;

    PAGED_CODE();
    
    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Device);

    KdPrint( ("PrmFuncTestEvtDeviceFileCreate %p\n", Device));

    DeviceNames = NULL;
    Irp = WdfRequestWdmGetIrp(Request);
    if (Irp->RequestorMode == KernelMode) {

        //
        // Forward the IRP if coming from kernel-mode
        //

        IoTarget = WdfDeviceGetIoTarget(Device);
        WdfRequestFormatRequestUsingCurrentType(Request);
        WDF_REQUEST_SEND_OPTIONS_INIT(
              &RequestSendOptions, 
              WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET
              );

        WdfRequestSend(Request, IoTarget, &RequestSendOptions);
            
    } else {
        Status = IoGetDeviceInterfaces(
            &GUID_DEVINTERFACE_PRMFUNCTEST,
            WdfDeviceWdmGetPhysicalDevice(Device),
            0,
            &DeviceNames
            );

        if (NT_SUCCESS(Status)) {
            if (DeviceNames == NULL || DeviceNames[0] == UNICODE_NULL) {
                Status = STATUS_DEVICE_DOES_NOT_EXIST;

            } else {
                RtlInitUnicodeString(&FilePath, DeviceNames);
                InitializeObjectAttributes(&ObjA,
                    &FilePath,
                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                    NULL,
                    NULL);

                Status = ZwCreateFile(&FileHandle,
                    GENERIC_WRITE,
                    &ObjA,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    0,
                    NULL,
                    0);
            }
        }

        WdfRequestComplete(Request, Status);    
    }

    if (DeviceNames != NULL) {
        ExFreePool(DeviceNames);
    }

    return;
}

VOID
PrmFuncTestEvtIoDeviceControl(
    _In_ WDFQUEUE     Queue,
    _In_ WDFREQUEST   Request,
    _In_ size_t       OutputBufferLength,
    _In_ size_t       InputBufferLength,
    _In_ ULONG        IoControlCode
    )

/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.

Return Value:

   VOID

--*/

{

    ULONG BytesReturned;
    PVOID InputBuffer;
    PVOID OutputBuffer;
    WDFMEMORY InputMemory;
    WDFMEMORY OutputMemory;
    size_t InputSize;
    size_t OutputSize;
    NTSTATUS Status;
    PPRM_DIRECT_CALL_PARAMETERS TestParameters;
    PPRM_TEST_RESULT PrmResult;
    PRM_INTERFACE PrmInterface;
    ULONG64 EfiStatus;
    BOOLEAN Found;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Queue);

    Status = STATUS_SUCCESS;
    InputBuffer = NULL;
    OutputBuffer = NULL;
    BytesReturned = 0;
    InputSize = 0;
    OutputSize = 0;

    //
    // VAlidate the input and output buffers
    //

    if (InputBufferLength != 0) {
        Status = WdfRequestRetrieveInputMemory(Request, &InputMemory);
        if (!NT_SUCCESS(Status)) {
            goto EvtIoDeviceControlEnd;
        }

        InputBuffer = WdfMemoryGetBuffer(InputMemory, &InputSize);
    }

    if (OutputBufferLength != 0) {
        Status = WdfRequestRetrieveOutputMemory(Request, &OutputMemory);
        if (!NT_SUCCESS(Status)) {
            goto EvtIoDeviceControlEnd;
        }

        OutputBuffer = WdfMemoryGetBuffer(OutputMemory, &OutputSize);
    }

    //
    // Use WdfRequestRetrieveInputBuffer and WdfRequestRetrieveOutputBuffer
    // to get the request buffers.
    //

    switch (IoControlCode) {
    case IOCTL_PRMFUNCTEST_DIRECT_CALL_TEST:
        if (InputSize < sizeof(PRM_DIRECT_CALL_PARAMETERS)) {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

         if (OutputSize < sizeof(PRM_TEST_RESULT)) {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        TestParameters = (PPRM_DIRECT_CALL_PARAMETERS)InputBuffer;
        PrmResult = (PPRM_TEST_RESULT)OutputBuffer;
        
        //
        // Acquire the direct-call PRM interface, which is defined in
        // prminterface.h.
        //
        //     typedef struct _PRM_INTERFACE {
        //        ULONG Version;
        //        PPRM_UNLOCK_MODULE UnlockModule;
        //        PPRM_LOCK_MODULE LockModule;
        //        PPRM_INVOKE_HANDLER InvokeHandler;
        //        PPRM_QUERY_HANDLER  QueryHandler;
        //     } PRM_INTERFACE, *PPRM_INTERFACE;
        //

        Status = ExGetPrmInterface(1, &PrmInterface);
        if (!NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // Lock the handler's PRM module to synchronize against any potential
        // runtime update to the PRM module.
        //
        // N.B. Note that technically this is only needed if a series of PRM
        // handlers need to be called transactionally (thus preventing
        // interleaving of PRM module updates). However, we will do it here
        // as an example of how it could be done.
        //

        Status = PrmInterface.LockModule(
                (LPGUID)&TestParameters->Guid);

        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Query for the presence of the PRM handler.
        //

        Status = PrmInterface.QueryHandler(
                (LPGUID)&TestParameters->Guid,
                &Found);

        if ((!NT_SUCCESS(Status)) || (Found == FALSE)) {
            break;
        }

        //
        // Invoke the PRM handler
        //

        Status = PrmInterface.InvokeHandler(
                (LPGUID)&TestParameters->Guid,
                TestParameters->ParameterBuffer,
                0,
                &EfiStatus);

        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Unlock the PRM module
        //
        // N.B. Note that technically this is only needed if a series of PRM
        // handlers need to be called as part of transaction. However, we will
        // do it here as an example of how it could be done.
        //

        Status = PrmInterface.UnlockModule(
                (LPGUID)&TestParameters->Guid);

        if (!NT_SUCCESS(Status)) {
            break;
        }

        PrmResult->Status = Status;
        PrmResult->EfiStatus = EfiStatus;
        Status = STATUS_SUCCESS;
        BytesReturned = sizeof(PRM_TEST_RESULT);
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

EvtIoDeviceControlEnd:
    WdfRequestCompleteWithInformation(Request, Status, BytesReturned);
}