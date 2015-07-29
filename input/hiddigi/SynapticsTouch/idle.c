/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved.
    Sample code. Dealpoint ID #843729.

    Module Name:

        idle.c

    Abstract:

        This file contains the declarations for Power Idle specific callbacks
	    and function definitions

    Environment:

        Kernel mode

    Revision History:

--*/

#include "internal.h"
#include "controller.h"
#include "idle.h"
#include "idle.tmh"

NTSTATUS
TchProcessIdleRequest(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request,
    OUT BOOLEAN *Pending
    )
/*++

Routine Description:

   Handles HIDClass's idle notification request.

   This request is provided to a HID miniport, and provides a callback
   routine typically used by HID miniports to self-manage power.

   The callback routine is invoked by the miniport to indicate that all
   peripherals are idle, and in response the HID class driver will:
     1) Queue a wait/wake IRP to the device, and
     2) Set the device to D3

   In the case of this touch miniport, we are using the HID class driver's
   enhanced power management functionality, whereby invoking the callback
   results in an immediate exit from D0, powering off touch.

   The Request will be completed when either HIDCLASS cancels it or
   there is a device wake signal that will cause us to complete it.

Arguments:

   Device - Handle to WDF Device Object

   Request - Handle to request object

   Pending - flag to monitor if the request was sent down the stack

Return Value:

   On success, the function returns STATUS_SUCCESS
   On failure it passes the relevant error code to the caller.

--*/
{
    PDEVICE_EXTENSION devContext;
    PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO idleCallbackInfo;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    devContext = GetDeviceContext(Device);

    NT_ASSERT(Pending != NULL);
    *Pending = FALSE;

    //
    // Retrieve request parameters and validate
    //
    irp = WdfRequestWdmGetIrp(Request);
    irpSp = IoGetCurrentIrpStackLocation(irp);

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO))
    {
        status = STATUS_INVALID_BUFFER_SIZE;

        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_HID,
            "Error: Input buffer is too small to process idle request - %!STATUS!", 
            status);

        goto exit;
    }

    //
    // Grab the callback
    //
    idleCallbackInfo = (PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO)
        irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    NT_ASSERT(idleCallbackInfo != NULL);

    if (idleCallbackInfo == NULL || idleCallbackInfo->IdleCallback == NULL)
    {
        status = STATUS_NO_CALLBACK_ACTIVE;
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_HID,
            "Error: Idle Notification request %p has no idle callback info - %!STATUS!",
            Request,
            status);
        goto exit;
    }

    {
        //
        // Create a workitem for the idle callback
        //
        WDF_OBJECT_ATTRIBUTES workItemAttributes;
        WDF_WORKITEM_CONFIG workitemConfig;
        WDFWORKITEM idleWorkItem;
        PIDLE_WORKITEM_CONTEXT idleWorkItemContext;

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&workItemAttributes, IDLE_WORKITEM_CONTEXT);
        workItemAttributes.ParentObject = devContext->FxDevice;

        WDF_WORKITEM_CONFIG_INIT(&workitemConfig, TchIdleIrpWorkitem);

        status = WdfWorkItemCreate(
                    &workitemConfig,
                    &workItemAttributes,
                    &idleWorkItem
                    );

        if (!NT_SUCCESS(status)) {
            Trace(
                TRACE_LEVEL_ERROR,
                TRACE_FLAG_HID,
                "Error creating creating idle work item - %!STATUS!",
                status);
            goto exit;
        }

        //
        // Set the workitem context
        //
        idleWorkItemContext = GetWorkItemContext(idleWorkItem);
        idleWorkItemContext->FxDevice = devContext->FxDevice;
        idleWorkItemContext->FxRequest = Request;
    
        //
        // Enqueue a workitem for the idle callback
        //
        WdfWorkItemEnqueue(idleWorkItem);

        //
        // Mark the request as pending so that 
        // we can complete it when we come out of idle
        //
        *Pending = TRUE;
    }

exit:

    return status;
}

VOID
TchIdleIrpWorkitem(
    IN WDFWORKITEM IdleWorkItem
    )
/*++

Routine Description:
 
    This is a workitem routine that TchProcessIdleRequest queues when 
    handling the HIDClass's idle notification IRP, so the idle callback can be made in
    a different thread context, instead of the Idle Irp's dispatch call.

Arguments:

    IdleWorkItem    -   Handle to a WDF workitem object

Return Value:

    VOID

--*/
{ 
    NTSTATUS status;
    PIDLE_WORKITEM_CONTEXT idleWorkItemContext;
    PDEVICE_EXTENSION deviceContext;
    PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO idleCallbackInfo;

    idleWorkItemContext = GetWorkItemContext(IdleWorkItem);    
    NT_ASSERT(idleWorkItemContext != NULL);

    deviceContext = GetDeviceContext(idleWorkItemContext->FxDevice);
    NT_ASSERT(deviceContext != NULL);

    //
    // Get the idle callback info from the workitem context
    //
    idleCallbackInfo = (PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO) 
        IoGetCurrentIrpStackLocation(WdfRequestWdmGetIrp(idleWorkItemContext->FxRequest))->\
        Parameters.DeviceIoControl.Type3InputBuffer;

    //
    // idleCallbackInfo is validated already, so invoke idle callback
    //
    idleCallbackInfo->IdleCallback(idleCallbackInfo->IdleContext);

    //
    // Park this request in our IdleQueue and mark it as pending
    // This way if the IRP was cancelled, WDF will cancel it for us
    //
    status = WdfRequestForwardToIoQueue(
                            idleWorkItemContext->FxRequest,
                            deviceContext->IdleQueue);

    if (!NT_SUCCESS(status))
    {
        //
        // IdleQueue is a manual-dispatch, non-power-managed queue. This should
        // *never* fail.
        //

        NT_ASSERTMSG("WdfRequestForwardToIoQueue to IdleQueue failed!", FALSE);

        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_IDLE,
            "Error forwarding idle notification Request:0x%p to IdleQueue:0x%p - %!STATUS!",
            idleWorkItemContext->FxRequest,
            deviceContext->IdleQueue,
            status);

        //
        // Complete the request if we couldnt forward to the Idle Queue
        //
        WdfRequestComplete(idleWorkItemContext->FxRequest, status);
    }
    else
    {
        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_IDLE,
            "Forwarded idle notification Request:0x%p to IdleQueue:0x%p - %!STATUS!",
            idleWorkItemContext->FxRequest,
            deviceContext->IdleQueue,
            status);
    }
    
    //
    // Delete the workitem since we're done with it
    //
    WdfObjectDelete(IdleWorkItem);

    return;
}


VOID
TchCompleteIdleIrp(
    IN PDEVICE_EXTENSION FxDeviceContext
    )
/*++

Routine Description:
 
    This is invoked when we enter D0. 
    We simply complete the Idle Irp if it hasn't been cancelled already.

Arguments:

    FxDeviceContext -  Pointer to Device Context for the device

Return Value:

    

--*/
{
    NTSTATUS status;
    WDFREQUEST request = NULL;

    //
    // Lets try to retrieve the Idle IRP from the Idle queue
    //
    status = WdfIoQueueRetrieveNextRequest(
                FxDeviceContext->IdleQueue,
                &request);

    //
    // We did not find the Idle IRP, maybe it was cancelled
    // 
    if (!NT_SUCCESS(status) || (request == NULL))
    {
        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_IDLE,
            "Error finding idle notification request in IdleQueue:0x%p - %!STATUS!",
            FxDeviceContext->IdleQueue,
            status);
    }
    else
    {
        //
        // Complete the Idle IRP
        //
        WdfRequestComplete(request, status);

        Trace(
            TRACE_LEVEL_INFORMATION,
            TRACE_FLAG_IDLE,
            "Completed idle notification Request:0x%p from IdleQueue:0x%p - %!STATUS!",
            request,
            FxDeviceContext->IdleQueue,
            status);
    }

    return;
}