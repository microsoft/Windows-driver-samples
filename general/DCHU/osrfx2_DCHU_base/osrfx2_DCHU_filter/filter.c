/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    filter.c

Abstract:

    This module shows how to a write a generic filter driver. The driver demonstrates how 
    to support device I/O control requests through queues. All I/O requests are passed on to 
    the lower driver. This filter driver shows how to handle IRP postprocessing by forwarding 
    the requests with and without a completion routine. To forward with a completion routine
    set the define FORWARD_REQUEST_WITH_COMPLETION to 1. 

Environment:

    User mode

--*/

#include "filter.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, FilterEvtDeviceAdd)
#endif


/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - Pointer to the driver object

    RegistryPath - Pointer to a unicode string representing the path
                   to the driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG Config;
    NTSTATUS          Status;
    WDFDRIVER         hDriver;

    KdPrint(("Generic Upper Filter Driver Sample - Driver Framework Edition.\n"));

    //
    // Initialize driver config to control the attributes that
    // are global to the driver. Note that the framework by default
    // provides a driver unload routine. If you created any resources
    // in DriverEntry and want them to be cleaned up when the driver unloads,
    // you can override that by manually setting the EvtDriverUnload in the
    // config structure. In general xxx_CONFIG_INIT macros are provided to
    // initialize most commonly used members.
    //

    WDF_DRIVER_CONFIG_INIT(&Config,
                           FilterEvtDeviceAdd);

    //
    // Create a framework driver object to represent our driver.
    //
    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &Config,
                             &hDriver);

    if (!NT_SUCCESS(Status))
    {
        KdPrint(("WdfDriverCreate failed with status 0x%x\n", Status));
    }
    
    return Status;
}


/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to the AddDevice
    call from the PnP manager. Here you can query the device properties
    using WdfFdoInitWdmGetPhysicalDevice/IoGetDeviceProperty and based
    on that, decide to create a filter device object and attach it to the
    function stack. If you are not interested in filtering this particular
    instance of the device, you can just return STATUS_SUCCESS without creating
    a framework device.

Arguments:

    Driver     - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
NTSTATUS
FilterEvtDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
{
    WDF_OBJECT_ATTRIBUTES DeviceAttributes;
    PFILTER_EXTENSION     FilterExt;
    NTSTATUS              Status;
    WDFDEVICE             Device;    
    WDF_IO_QUEUE_CONFIG   IoQueueConfig;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER(Driver);

    //
    // Tell the framework that you are a filter driver. The framework
    // takes care of inherting all the device flags & characteristics
    // from the higher device that you are attaching to.
    //
    WdfFdoInitSetFilter(DeviceInit);

    //
    // Specify the size of the device extension where we track per device
    // context.
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, FILTER_EXTENSION);

    //
    // Create a framework device object. This call will in turn create
    // a WDM device object, attach it to the lower stack, and set the
    // appropriate flags and attributes.
    //
    Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &Device);

    if (!NT_SUCCESS(Status))
    {
        KdPrint(("WdfDeviceCreate failed with status code 0x%x\n", Status));
        return Status;
    }

    FilterExt = FilterGetData(Device);

    //
    // Configure the default queue to be Parallel. 
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&IoQueueConfig,
                                           WdfIoQueueDispatchParallel);

    //
    // The framework by default creates non-power managed queues for
    // filter drivers.
    //
    IoQueueConfig.EvtIoDeviceControl = FilterEvtIoDeviceControl;

    Status = WdfIoQueueCreate(Device,
                              &IoQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              WDF_NO_HANDLE // Pointer to default queue
                              );

    if (!NT_SUCCESS(Status))
    {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n", Status));
        return Status;
    }   

    return Status;
}


/*++

Routine Description:

    This routine is the dispatch routine for internal device control requests.
    
Arguments:

    Queue              - Handle to the framework queue object that is associated
                         with the I/O request

    Request            - Handle to a framework request object

    OutputBufferLength - Length of the request's output buffer,
                         if an output buffer is available

    InputBufferLength  - Length of the request's input buffer,
                         if an input buffer is available

    IoControlCode      - The driver-defined or system-defined I/O control code
                         (IOCTL) that is associated with the request

Return Value:

   VOID

--*/
VOID
FilterEvtIoDeviceControl(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     OutputBufferLength,
    IN size_t     InputBufferLength,
    IN ULONG      IoControlCode
    )
{
    PFILTER_EXTENSION FilterExt;
    NTSTATUS          Status = STATUS_SUCCESS;
    WDFDEVICE         Device;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    KdPrint(("Entered FilterEvtIoDeviceControl\n"));

    Device = WdfIoQueueGetDevice(Queue);

    FilterExt = FilterGetData(Device);

    switch (IoControlCode)
    {

    //
    // Put your cases for handling IOCTLs here
    //

    default:
        Status = STATUS_SUCCESS;
    }
    
    if (!NT_SUCCESS(Status))
    {
        WdfRequestComplete(Request, Status);
        return;
    }

    //
    // Forward the request down. WdfDeviceGetIoTarget returns
    // the default target, which represents the device attached to us below in
    // the stack.
    //
#if FORWARD_REQUEST_WITH_COMPLETION
    //
    // Use this routine to forward a request if you are interested in post
    // processing the IRP.
    //
        FilterForwardRequestWithCompletionRoutine(Request, 
                                                  WdfDeviceGetIoTarget(Device));
#else   
        FilterForwardRequest(Request, WdfDeviceGetIoTarget(Device));
#endif

    return;
}


/*++
Routine Description:

    Passes a request on to the lower driver.

Arguments:

    Request - The request to pass on to the lower driver

    Target  - The lower driver to pass the request to

Return Value:

    VOID

--*/
VOID
FilterForwardRequest(
    IN WDFREQUEST  Request,
    IN WDFIOTARGET Target
    )
{
    WDF_REQUEST_SEND_OPTIONS Options;
    BOOLEAN                  RequestSent;
    NTSTATUS                 Status;

    //
    // We are not interested in post processing the IRP so 
    // fire and forget.
    //
    WDF_REQUEST_SEND_OPTIONS_INIT(&Options,
                                  WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    RequestSent = WdfRequestSend(Request, Target, &Options);

    if (RequestSent == FALSE) {
        Status = WdfRequestGetStatus(Request);
        KdPrint(("WdfRequestSend failed: 0x%x\n", Status));
        WdfRequestComplete(Request, Status);
    }

    return;
}

#if FORWARD_REQUEST_WITH_COMPLETION

VOID
FilterForwardRequestWithCompletionRoutine(
    IN WDFREQUEST  Request,
    IN WDFIOTARGET Target
    )
/*++
Routine Description:

    This routine forwards the request to a lower driver with
    a completion so that when the request is completed by the
    lower driver, it can regain control of the request and look
    at the result.

Arguments:

    Request - The request to pass on to the lower driver

    Target  - The lower driver to pass the request to

Return Value:

    VOID

--*/
{
    BOOLEAN  RequestSent;
    NTSTATUS Status;

    //
    // The following function essentially copies the content of the
    // current stack location of the underlying IRP to the next one. 
    //
    WdfRequestFormatRequestUsingCurrentType(Request);

    WdfRequestSetCompletionRoutine(Request,
                                   FilterRequestCompletionRoutine,
                                   WDF_NO_CONTEXT);

    RequestSent = WdfRequestSend(Request,
                                 Target,
                                 WDF_NO_SEND_OPTIONS);

    if (RequestSent == FALSE)
    {
        Status = WdfRequestGetStatus(Request);
        KdPrint(("WdfRequestSend failed: 0x%x\n", Status));
        WdfRequestComplete(Request, Status);
    }

    return;
}


/*++

Routine Description:

    Completion Routine.

Arguments:

    Target  - Target handle

    Request - Request handle

    Params  - Request completion params

    Context - Driver supplied context

Return Value:

    VOID

--*/
VOID
FilterRequestCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
   )
{
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    WdfRequestComplete(Request, CompletionParams->IoStatus.Status);

    return;
}

#endif //FORWARD_REQUEST_WITH_COMPLETION




