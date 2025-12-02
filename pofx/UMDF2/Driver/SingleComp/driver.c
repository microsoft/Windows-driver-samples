/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Driver.c

Abstract:
    This module implements a UMDF2 sample driver for a single-component device.
    The driver uses the power framework to manage the power state of the
    component that represents device.

    The device used in this sample is a root-enumerated device whose components
    are simulated entirely in software. The simulation of the components is 
    implemented in HwSim.h and HwSim.c.

    This driver works only on Win8.1 and above.

Environment:

    User mode

--*/

#include "include.h"
#include "hwsim.h"

#include <initguid.h>
#include "AppInterface.h"

#include "driver.tmh"

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Driver initialization entry point. This entry point is called directly by 
    the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path to the 
                   driver-specific key in the registry.

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG   config;
    WDF_OBJECT_ATTRIBUTES attributes;
   
    WPP_INIT_TRACING(DriverObject, RegistryPath);
    
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! Entry of driver");
    
    //
    // Initialize driver config to control the attributes that are global to
    // the driver. Note that framework by default provides a driver unload 
    // routine. If DriverEntry creates any resources that require clean-up in
    // driver unload, you can manually override the default by supplying a 
    // pointer to the EvtDriverUnload callback in the config structure. In 
    // general xxx_CONFIG_INIT macros are provided to initialize most commonly
    // used members.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = SingleCompEvtDriverCleanup;
    WDF_DRIVER_CONFIG_INIT(
        &config,
        SingleCompEvtDeviceAdd
        );

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes, // Driver Attributes
                             &config,     // Driver Config Info
                             WDF_NO_HANDLE
                             );

    if (FALSE == NT_SUCCESS(status)) {
        KdPrint( ("WdfDriverCreate failed with status 0x%x\n", status));
        WPP_CLEANUP(NULL);
    }

    return status;
}

VOID
SingleCompEvtDriverCleanup(
    _In_ WDFOBJECT Driver
    )
{
    UNREFERENCED_PARAMETER(Driver);
    
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! Entry\n");
    WPP_CLEANUP(NULL);
}

NTSTATUS
SingleCompEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by UMDF in response to AddDevice call from 
    the PnP manager. 

Arguments:

    Driver - Handle to the UMDF driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    An NTSTATUS value representing success or failure of the function.

--*/
{
    NTSTATUS status;
    WDFDEVICE device;
    WDFQUEUE queue;
    WDF_IO_QUEUE_CONFIG     queueConfig;
    FDO_DATA               *fdoContext = NULL;
    WDF_OBJECT_ATTRIBUTES   objectAttributes;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;

    UNREFERENCED_PARAMETER(Driver);
    
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! Entry\n");

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttributes, FDO_DATA);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
    pnpCallbacks.EvtDeviceD0Entry = SingleCompEvtDeviceD0Entry;
    pnpCallbacks.EvtDeviceD0Exit = SingleCompEvtDeviceD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);
    
    status = WdfDeviceCreate(&DeviceInit, &objectAttributes, &device);
    if (!NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfDeviceCreate failed with %!status!.", 
              status);
        goto exit;
    }

    fdoContext = FdoGetContext(device);

    //
    // Our initial state is active
    //
    fdoContext->IsActive = TRUE;

    //
    // Create a power-managed queue for IOCTL requests.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, 
                                           WdfIoQueueDispatchParallel);
    queueConfig.EvtIoDeviceControl = SingleCompEvtIoDeviceControl;

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. If the driver 
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver.  Normally 
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests or 
    // forward them to other drivers. This driver completes the requests 
    // directly in the queue's handlers. If the EvtIoStop callback is not 
    // implemented, the framework waits for all driver-owned requests to be
    // done before moving in the Dx/sleep states or before removing the 
    // device, which is the correct behavior for this type of driver.
    // If the requests were taking an indeterminate amount of time to complete,
    // or if the driver forwarded the requests to a lower driver/another stack,
    // the queue should have an EvtIoStop/EvtIoResume.
    //
    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &queue);
    __analysis_assume(queueConfig.EvtIoStop == 0);

    if (FALSE == NT_SUCCESS (status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfIoQueueCreate for IoDeviceControl failed with %!status!.", 
              status);
        goto exit;
    }

    status = WdfDeviceConfigureRequestDispatching(device,
                                                  queue,
                                                  WdfRequestTypeDeviceControl);
    if (FALSE == NT_SUCCESS (status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfDeviceConfigureRequestDispatching for "
              "WdfRequestTypeDeviceControl failed with %!status!.", 
              status);
        goto exit;
    }

    status = AssignS0IdleSettings(device);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Create a device interface so that applications can open a handle to this
    // device.
    //
    status = WdfDeviceCreateDeviceInterface(device, 
                                            &GUID_DEVINTERFACE_POWERFX,
                                            NULL /* ReferenceString */);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfDeviceCreateDeviceInterface failed with %!status!.", 
              status);
        goto exit;
    }

    //
    // Initialize the hardware simulator
    //
    status = HwSimInitialize(device);
    if (FALSE == NT_SUCCESS(status)) {
        goto exit;
    }    
    
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! Exit\n");
    
exit:
    return status;
}

NTSTATUS
SingleCompEvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
    )
/*++
Routine Description:

    UMDF calls this routine when the device has entered D0.

Arguments:

    Device - Handle to the framework device object

    PreviousState - Previous device power state

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    UNREFERENCED_PARAMETER(PreviousState);
    
    HwSimD0Entry(Device);
    
    return STATUS_SUCCESS;
}

NTSTATUS
SingleCompEvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
    )
/*++
Routine Description:

    UMDF calls this routine when the device is about to leave D0.

Arguments:

    Device - Handle to the framework device object

    TargetState - Device power state that the device is about to enter

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    UNREFERENCED_PARAMETER(TargetState);

    HwSimD0Exit(Device);

    return STATUS_SUCCESS;
}

NTSTATUS 
AssignS0IdleSettings(
    _In_ WDFDEVICE Device
    )
/*++
Routine Description:

    Helper function to assign S0 idle settings for the device

Arguments:

    Device - Handle to the framework device object

Return Value:

    An NTSTATUS value representing success or failure of the function.
    
--*/
{
    NTSTATUS status;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS powerPolicy;
    
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! Entry\n");

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&powerPolicy, 
                                               IdleCannotWakeFromS0);
    powerPolicy.IdleTimeoutType = SystemManagedIdleTimeout;

    status = WdfDeviceAssignS0IdleSettings(Device, &powerPolicy);
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR,
              "%!FUNC! - WdfDeviceAssignS0IdleSettings failed with %!status!.", 
              status);
    }
    
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! Exit\n");
    
    return status;
}

VOID
SingleCompEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++
Routine Description:

    Callback invoked by WDFQUEUE for a Device Io Control request.

Arguments:

    Queue - Device I/O control queue
              
    Request - Device I/O control request

    OutputBufferLength - Output buffer length for the I/O control

    InputBufferLength - Input buffer length for the I/O control

    IoControlCode - I/O control code
                
--*/
{
    NTSTATUS status;
    PPOWERFX_READ_COMPONENT_OUTPUT outputBuffer = NULL;
    WDFDEVICE device = NULL;
    ULONG componentData;
    ULONG_PTR information = 0;
    FDO_DATA *fdoContext = NULL;
    
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! Entry\n");
    
    //
    // When we complete the request, make sure we don't get the I/O manager to 
    // copy any more data to the client address space than what we write to the
    // output buffer. The only data that we write to the output buffer is the 
    // component data and the C_ASSERT below ensures that the output buffer does
    // not have room to contain anything other than that.
    //
    C_ASSERT(sizeof(componentData) == sizeof(*outputBuffer));

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    //
    // This is a power-managed queue. So our queue stop/start logic should have 
    // ensured that we are in the active condition when a request is dispatched
    // from this queue.
    //
    device = WdfIoQueueGetDevice(Queue);
    fdoContext = FdoGetContext(device);
    if (FALSE == fdoContext->IsActive) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - IOCTL %d was dispatched from WDFQUEUE %p when the "
              "component was not in an active condition.",
              IOCTL_POWERFX_READ_COMPONENT,
              Queue);
        WdfVerifierDbgBreakPoint();
    }
    
    //
    // Validate Ioctl code
    //
    if (IOCTL_POWERFX_READ_COMPONENT != IoControlCode) {
        status = STATUS_NOT_SUPPORTED;
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! -Unsupported IoControlCode. Expected: %d. Actual: %d."
              " %!status!.", 
              IOCTL_POWERFX_READ_COMPONENT,
              IoControlCode,
              status);
        goto exit;
    }
    
    //
    // Get the output buffer
    //
    status = WdfRequestRetrieveOutputBuffer(Request,
                                            sizeof(*outputBuffer),
                                            (PVOID*) &outputBuffer,
                                            NULL // Length
                                            );
    if (FALSE == NT_SUCCESS(status)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfRequestRetrieveOutputBuffer failed with %!status!.",
              status);
        goto exit;
    }
    
    //
    // Read the data from the component
    //
    componentData = HwSimReadComponent(device);
    outputBuffer->ComponentData = componentData;
    information = sizeof(*outputBuffer);
    
    status = STATUS_SUCCESS;
    
    Trace(TRACE_LEVEL_INFORMATION, "%!FUNC! Exit\n");
    
exit:
    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, status, information);
    return;
}

//
// Read and write queues are only for illustration purposes - on how to stop
// multiple queues. Currently app doesn't send Read/Write to the driver.
//
VOID
SingleCompEvtIoRead(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength
    )
/*++
Routine Description:

    Callback invoked by WDFQUEUE for a read request.

Arguments:

    Queue - Read queue
              
    Request - Read request

    OutputBufferLength - Length of read
                
--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    status = STATUS_NOT_SUPPORTED;
        
    Trace(TRACE_LEVEL_ERROR, 
          "%!FUNC! -Reads are currently not supported: %!status!.",
          status);
    
    WdfRequestComplete(Request, status);
}

VOID
SingleCompEvtIoWrite(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t InputBufferLength
    )
/*++
Routine Description:

    Callback invoked by WDFQUEUE for a write request.

Arguments:

    Queue - Write queue
              
    Request - Write request

    InputBufferLength - Length of write
                
--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(InputBufferLength);
        
    status = STATUS_NOT_SUPPORTED;
        
    Trace(TRACE_LEVEL_ERROR, 
          "%!FUNC! -Writes are currently not supported: %!status!.",
          status);
    
    WdfRequestComplete(Request, status);
}

