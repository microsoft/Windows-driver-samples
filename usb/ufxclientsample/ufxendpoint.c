/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ufxendpoint.c

Abstract:

    Defines the functions needed for UFXENDPOINT object

Environment:

    Kernel mode

--*/

#include "ufxendpoint.h"
#include "ufxdevice.h"
#include "transfer.h"
#include <usbfnioctl.h>
#include "registers.h"
#include "trace.h"
#include "device.h"
#include "ufxendpoint.tmh"

EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE EvtIoCanceledOnQueue;
EVT_WDF_IO_QUEUE_IO_STOP EndpointQueue_EvtIoStop;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtEndpointCommandQueue;

EVT_WDF_OBJECT_CONTEXT_CLEANUP UfxEndpoint_Cleanup;

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
UfxEndpointDescriptorUpdate (
    _In_ UFXENDPOINT Endpoint,
    _In_ PUSB_ENDPOINT_DESCRIPTOR Descriptor
    );

_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
UfxEndpointAdd (
    _In_ UFXDEVICE Device,
    _In_ PUSB_ENDPOINT_DESCRIPTOR Descriptor,
    _Inout_ PUFXENDPOINT_INIT EndpointInit
    )
/*++
Routine Description:

    Creates a UFXENDPOINT object, and initializes its contexts.

Parameters Description:

    Device - UFXDEVICE associated with the endpoint.

    Descriptor - Endpoint descriptor for this endpoint.

    EndpointInit - Opaque structure from UFX.

Return Value:

    STATUS_SUCCESS if successful, appropriate NTSTATUS message otherwise.
--*/
{
    NTSTATUS Status;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_IO_QUEUE_CONFIG TransferQueueConfig;
    WDF_OBJECT_ATTRIBUTES TransferQueueAttributes;
    WDF_IO_QUEUE_CONFIG CommandQueueConfig;
    WDF_OBJECT_ATTRIBUTES CommandQueueAttributes;
    UFXENDPOINT Endpoint;
    PUFXENDPOINT_CONTEXT EpContext;
    PUFXDEVICE_CONTEXT DeviceContext;
    UFX_ENDPOINT_CALLBACKS Callbacks;
    PENDPOINT_QUEUE_CONTEXT QueueContext;
    WDFQUEUE Queue;

    TraceEntry();

    DeviceContext = UfxDeviceGetContext(Device);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, UFXENDPOINT_CONTEXT);
    Attributes.ExecutionLevel = WdfExecutionLevelPassive;
    Attributes.EvtCleanupCallback = UfxEndpoint_Cleanup;

    //
    // Note: Execution level needs to be passive to avoid deadlocks with WdfRequestComplete.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&TransferQueueAttributes, ENDPOINT_QUEUE_CONTEXT);
    TransferQueueAttributes.ExecutionLevel = WdfExecutionLevelPassive;
    
    WDF_IO_QUEUE_CONFIG_INIT(&TransferQueueConfig, WdfIoQueueDispatchManual);
    TransferQueueConfig.AllowZeroLengthRequests = TRUE;
    TransferQueueConfig.EvtIoStop = EndpointQueue_EvtIoStop;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&CommandQueueAttributes, ENDPOINT_QUEUE_CONTEXT);
    CommandQueueAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    WDF_IO_QUEUE_CONFIG_INIT(&CommandQueueConfig, WdfIoQueueDispatchSequential);
    CommandQueueConfig.EvtIoInternalDeviceControl = EvtEndpointCommandQueue;

    UFX_ENDPOINT_CALLBACKS_INIT(&Callbacks);
    UfxEndpointInitSetEventCallbacks(EndpointInit, &Callbacks);

    Status = UfxEndpointCreate(
                 Device,
                 EndpointInit,
                 &Attributes,
                 &TransferQueueConfig,
                 &TransferQueueAttributes,
                 &CommandQueueConfig,
                 &CommandQueueAttributes,
                 &Endpoint);
    CHK_NT_MSG(Status, "Failed to create ufxendpoint!");

    Status = WdfCollectionAdd(DeviceContext->Endpoints, Endpoint);
    CHK_NT_MSG(Status, "Failed to add endpoint to collection!");

    EpContext = UfxEndpointGetContext(Endpoint);
    EpContext->UfxDevice = Device;
    EpContext->WdfDevice = DeviceContext->FdoWdfDevice;
    RtlCopyMemory(&EpContext->Descriptor, Descriptor, sizeof(*Descriptor));

    Queue = UfxEndpointGetTransferQueue(Endpoint);
    QueueContext = EndpointQueueGetContext(Queue);
    QueueContext->Endpoint = Endpoint;

    Queue = UfxEndpointGetCommandQueue(Endpoint);
    QueueContext = EndpointQueueGetContext(Queue);
    QueueContext->Endpoint = Endpoint;

    Status = TransferInitialize(Endpoint);
    CHK_NT_MSG(Status, "Failed to initialize endpoint transfers");
    
    //
    // This can happen if we're handling a SetInterface command.
    //
    if (DeviceContext->UsbState == UsbfnDeviceStateConfigured) {
        UfxEndpointConfigure(Endpoint);
    }

    Status = WdfIoQueueReadyNotify(
                 UfxEndpointGetTransferQueue(Endpoint),
                 TransferReadyNotify,
                 Endpoint);
    CHK_NT_MSG(Status, "Failed to register ready notify");

End:
    TraceExit();
    return Status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
UfxEndpointConfigureHardware (
    _In_ UFXENDPOINT Endpoint,
    _In_ BOOLEAN Modify
    )
/*++
Routine Description:

    Configures the endpoint on the controller based on the
    endpoint descriptor. If it is a control endpoint, configures
    for both IN and OUT endpoints.

Parameters Description:

    Endpoint - Endpoint to configure

    Modify - This is only TRUE for endpoint 0 on soft reset.

--*/
{
    PUFXENDPOINT_CONTEXT EpContext;

    TraceEntry();

    UNREFERENCED_PARAMETER(Modify);

    EpContext = UfxEndpointGetContext(Endpoint);

    TraceInformation("CONFIGURE ENDPOINT: %08X Endpoint (%d)", (ULONG) Endpoint, EpContext->PhysicalEndpoint);

    //
    // ControllerContext = DeviceGetControllerContext(EpContext->WdfDevice);
    // 
    // The USB address is:
    //      EpContext->Descriptor.bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK
    //
    // The maxPacketSize is:
    //      (Address == 0 && !Modify) ? 512 : EpContext->Descriptor.wMaxPacketSize;
    //
    // The Interval is:
    //      ((EpContext->Descriptor.bInterval > 0) && (ControllerContext->Speed != UsbFullSpeed)) ?
    //        EpContext->Descriptor.bInterval :
    //        0;
    //

    if (CONTROL_ENDPOINT(Endpoint)) {
        //
        // #### TODO: Configure both IN and OUT endpoints ####
        //
    } else {
        if (DIRECTION_IN(Endpoint)) {
            //
            // #### TODO: Configure the IN endpoint ####
            //
        } else {
            //
            // #### TODO: Configure the OUT endpoint ####
            //
        }
    }

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
UfxEndpointConfigure (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Enables and configures the endpoint on the controller.

Parameters Description:

    Endpoint - Endpoint to configure

--*/
{
    PUFXENDPOINT_CONTEXT EpContext;
    ULONG EndpointMask;
    ULONG Address;

    TraceEntry();

    EpContext = UfxEndpointGetContext(Endpoint);

    EndpointMask = 1 << (EpContext->PhysicalEndpoint);
    if (CONTROL_ENDPOINT(Endpoint)) {
        EndpointMask |= 1 << (EpContext->PhysicalEndpoint + 1);
    }

    Address = EpContext->Descriptor.bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
    
    //
    // Configure the endpoint
    //
    UfxEndpointConfigureHardware(Endpoint, Address == 0);
    
    //
    // #### TODO: Insert code to enable the endpoint on the controller ####
    //

    if (Address != 0) {
        TransferStart(Endpoint);
    }

    TraceExit();
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
UfxEndpointDescriptorUpdate (
    _In_ UFXENDPOINT Endpoint,
    _In_ PUSB_ENDPOINT_DESCRIPTOR Descriptor
    )
/*++
Routine Description:

    Updates the descriptor and re-programs the endpoint.
    If endpoint 0 is passed, starts a new configuration.

Parameters Description:

    Endpoint - Endpoint to update.

    Descriptor - Endpoint descriptor for this endpoint.

Return Value:

    STATUS_SUCCESS if successful, appropriate NTSTATUS message otherwise.

--*/
{
    PUFXENDPOINT_CONTEXT EpContext;
    
    TraceEntry();

    EpContext = UfxEndpointGetContext(Endpoint);

    RtlCopyMemory(&EpContext->Descriptor, Descriptor, sizeof(USB_ENDPOINT_DESCRIPTOR));

    if ((Descriptor->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK) == 0) {
        UfxEndpointConfigure(Endpoint);
    }

    TraceExit();
    return STATUS_SUCCESS;
}

VOID
UfxEndpoint_Cleanup (
    _In_ WDFOBJECT Object
    )
/*++
Routine Description:

    Cleanup routine for the UFX endpoint object.

Parameters Description:

    Object - UFX endpoint object

--*/
{
    UFXENDPOINT Endpoint;
    PUFXENDPOINT_CONTEXT EpContext;
    ULONG EndpointMask;
    PUFXDEVICE_CONTEXT DeviceContext;
    NTSTATUS Status;

    TraceEntry();

    Endpoint = (UFXENDPOINT) Object;
    EpContext = UfxEndpointGetContext(Endpoint);
    DeviceContext = UfxDeviceGetContext(EpContext->UfxDevice);

    //
    // Release all references
    //
    WdfCollectionRemove(DeviceContext->Endpoints, Endpoint);

    //
    // Disable the endpoint. Note queue should already be purged by UFX.
    //
    Status = WdfDeviceStopIdle(DeviceContext->FdoWdfDevice, TRUE);
    CHK_NT_MSG(Status, "Failed to stop idle to disable endpoint");

    EndpointMask = 1 << (EpContext->PhysicalEndpoint);
    if (CONTROL_ENDPOINT(Endpoint)) {
        EndpointMask |= 1 << (EpContext->PhysicalEndpoint + 1);
    }

    //
    // #### TODO: Add code to disable the endpoint on the controller ####
    //

    WdfDeviceResumeIdle(DeviceContext->FdoWdfDevice);

    //
    // Delete allocated stuff
    //
    TransferDestroy(Endpoint);

End:
    TraceExit();
}

_Use_decl_annotations_
VOID
EndpointQueue_EvtIoStop (
    WDFQUEUE Queue,
    WDFREQUEST Request,
    ULONG ActionFlags
    )
/*++
Routine Description:

    EvtIoStop event handler

Parameters Description:

    Queue - Handle to the queue object

    Request - The request to be completed, requeued, or suspended.

    ActionFlags - Bitmask indicating action to take and if request is cancelable.
    
--*/
{   
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(ActionFlags);

    TraceEntry();
   
    TransferRequestCancel(Request, TRUE);

    TraceExit();
}

VOID
EvtEndpointCommandQueue (
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++
Routine Description:

    EvtIoInternalDeviceControl event handler

Parameters Description:

    Queue - Handle to the queue object

    Request - Internal device control request.

    OutputBufferLength - size of the output buffer.

    InputBufferLength - size of the input buffer.

    IoControlCode - IOCTL for the request.
    
--*/

{
    PVOID OutputBuffer;
    PENDPOINT_QUEUE_CONTEXT QueueContext;
    NTSTATUS Status;
    PUFXENDPOINT_CONTEXT EpContext;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    TraceEntry();
    
    QueueContext = EndpointQueueGetContext(Queue);
    EpContext = UfxEndpointGetContext(QueueContext->Endpoint);

    Status = WdfRequestRetrieveOutputBuffer(Request, 0, &OutputBuffer, NULL);
    CHK_NT_MSG(Status, "Failed to retrieve command output buffer");

    switch(IoControlCode) {
        case IOCTL_INTERNAL_USBFN_GET_PIPE_STATE:
            TransferGetStall(
                QueueContext->Endpoint,
                (PBOOLEAN) OutputBuffer);

            WdfRequestComplete(Request, STATUS_SUCCESS);
            break;

        case IOCTL_INTERNAL_USBFN_SET_PIPE_STATE:
            if (*((PBOOLEAN) OutputBuffer)) {
                EpContext->StallRequest = Request;
                TransferCommandStallSet(QueueContext->Endpoint);

            } else {
                EpContext->ClearRequest = Request;
                TransferCommandStallClear(QueueContext->Endpoint);
            }
            break;

        case IOCTL_INTERNAL_USBFN_DESCRIPTOR_UPDATE:
            Status = UfxEndpointDescriptorUpdate(
                         QueueContext->Endpoint,
                         (PUSB_ENDPOINT_DESCRIPTOR) OutputBuffer);
            CHK_NT_MSG(Status, "Failed to update endpoint descriptor");
            WdfRequestComplete(Request, STATUS_SUCCESS);
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto End;
    }
    
End:
    if (!NT_SUCCESS(Status)) {
        WdfRequestComplete(Request, Status);
    }
    TraceExit();
}