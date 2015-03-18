/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    component.c

Abstract:
    This module contains routines that implement power management for a 
    component of the device

Environment:

    Kernel mode

--*/

#include "WdfPoFxPriv.h"
#include "component.tmh"

VOID
AcquirePowerReferenceForRequest(
    _In_ WDFQUEUE Queue
    )
/*++
Routine description:
    This routine acquires a power reference on behalf of a request, if the 
    request is being forwarded to a component queue. If the request is not being
    forwarded to a component queue, then this routine does nothing.

Arguments:
    Queue - Handle to the KMDF queue object to which the request is being 
      forwarded

Return value:
    None
--*/
{
    PPOFX_QUEUE_CONTEXT qCtx = NULL;
    WDFDEVICE device = NULL;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    ULONG component;
    
    //
    // Get the queue context
    //
    qCtx = HelperGetQueueContext(Queue);
    if (NULL == qCtx) {
        //
        // The request is not being forwarded to a component queue. So there is 
        // no need to take a power reference at this time.
        //
        return;
    }
    
    //
    // Get the component on which the power reference needs to be taken
    //
    component = qCtx->QueueInitSettings.Component;
    
    //
    // Get the device that the queue is associated with
    //
    device = WdfIoQueueGetDevice(Queue);

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(device);

    //
    // Take the power reference
    //
    PoFxActivateComponent(devCtx->PoHandle, component, 0 /* Flags */);
    return;
}

VOID
ReleasePowerReferenceForRequest(
    _In_ WDFREQUEST Request
    )
/*++
Routine description:
    This routine releases a power reference on behalf of a request, if a power
    reference had been taken previously. If no power reference had been taken
    previously for this request, then this routine does nothing.

Arguments:
    Request - Handle to the KMDF request object on behalf of which the power 
      reference needs to be released

Return value:
    None
--*/
{
    WDFQUEUE queue = NULL;
    PPOFX_QUEUE_CONTEXT qCtx = NULL;
    WDFDEVICE device = NULL;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    ULONG component;

    //
    // Get the queue that the request is associated with
    //
    queue = WdfRequestGetIoQueue(Request);
    if (NULL == queue) {
        return;
    }

    //
    // Get the queue context
    //
    qCtx = HelperGetQueueContext(queue);
    if (NULL == qCtx) {
        //
        // The request is currently not associated with a component queue. So
        // no power reference has been taken on behalf of the request and hence
        // there is no power reference to release at this time.
        //
        return;
    }

    //
    // Get the component on which the power reference has been taken on behalf
    // of this request
    //
    component = qCtx->QueueInitSettings.Component;
    
    //
    // Get the device that the queue is associated with
    //
    device = WdfIoQueueGetDevice(queue);

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(device);

    //
    // Release the power reference that was previously taken
    //
    PoFxIdleComponent(devCtx->PoHandle, component, 0 /* Flags */);

    return;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PfhForwardRequestToQueue(
    _In_ WDFREQUEST Request,
    _In_ WDFQUEUE Queue
    )
// See comments in WdfPoFx.h
{
    NTSTATUS status;
    
    //
    // If the request currently has a power reference on a component, then 
    // release the power reference.
    //
    ReleasePowerReferenceForRequest(Request);

    //
    // If the destination queue requires a power reference on a component, then
    // take the power reference now.
    //
    AcquirePowerReferenceForRequest(Queue);
    
    //
    // Queue the request to the caller-specified queue
    //
    status = WdfRequestForwardToIoQueue(Request, Queue);
    if (FALSE == NT_SUCCESS(status)) {
        //
        // If the queue has not been purged or drained, forwarding a request to
        // a queue should always succeeded.
        //
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - WdfRequestForwardToIoQueue failed with %!status!.", 
              status);
        WdfVerifierDbgBreakPoint();
    }

    return;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PfhCompleteRequest(
    _In_ WDFREQUEST Request,
    _In_ NTSTATUS Status,
    _In_ ULONG_PTR Information
    )
// See comments in WdfPoFx.h
{
    //
    // If the request currently has a power reference on a component, then 
    // release the power reference.
    //
    ReleasePowerReferenceForRequest(Request);
    
    //
    // Complete the request
    //
    WdfRequestCompleteWithInformation(Request, Status, Information);
}

VOID
_PfhEvtRequestCanceledOnComponentQueue(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
    )
/*++
Routine Description:

    This routine is invoked when an IO request is canceled while it is in a
    component queue

Arguments:

    Queue - Handle to the framework queue object for the component queue

    Request - Handle to the framework request object for the request being 
      canceled

Return Value:

    None

--*/
{
    PPOFX_QUEUE_CONTEXT qCtx = NULL;
    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE evtIoCanceledOnQueue;

    if (FALSE == IsQueueInitialized(Queue)) {
        Trace(TRACE_LEVEL_ERROR, 
              "%!FUNC! - PfhInitializeComponentQueueSettings has not yet been "
              "called for WDFQUEUE %p.",
              Queue);
        WdfVerifierDbgBreakPoint();
    }

    //
    // Get the queue context
    //
    qCtx = HelperGetQueueContext(Queue);
    
    evtIoCanceledOnQueue = qCtx->QueueInitSettings.EvtIoCanceledOnQueue;
    if (NULL != evtIoCanceledOnQueue) {
        //
        // The driver layer supplied a canceled-on-queue callback, so invoke it
        //
        evtIoCanceledOnQueue(Queue, Request);
        
    } else {
        //
        // Just complete the request
        //
        PfhCompleteRequest(Request, STATUS_CANCELLED, 0 /* Information */);
    }
    return;
}

VOID
ComponentActive(
    _In_ PPOFX_DEVICE_CONTEXT DevCtx,
    _In_ ULONG Component
    )
/*++
Routine Description:

    In this routine, we perform operations that are needed when a component 
    becomes active.

Arguments:

    DevCtx - Pointer to our context space for the device object

    Component - The component that has become active

Return Value:

    None

--*/
{
    //
    // Mark the component as active
    //
    DevCtx->ComponentInfo[Component].IsActive = TRUE;

    if (NULL != DevCtx->ComponentInfo[Component].Queue) {
        //
        // Start the component-specific queue
        //
        WdfIoQueueStart(DevCtx->ComponentInfo[Component].Queue);
    }

    return;
}

VOID
ComponentIdle(
    _In_ PPOFX_DEVICE_CONTEXT DevCtx,
    _In_ ULONG Component
    )
/*++
Routine Description:

    In this routine, we perform operations that are needed when a component 
    becomes idle.

Arguments:

    DevCtx - Pointer to our context space for the device object

    Component - The component that has become idle

Return Value:

    None

--*/
{
    ULONG queueRequests;

    //
    // Mark the component as idle
    //
    DevCtx->ComponentInfo[Component].IsActive = FALSE;

    if (NULL != DevCtx->ComponentInfo[Component].Queue) {
        //
        // NOTE: Given that the component is idle, there shouldn't be any 
        // outstanding requests sitting in the component queue. There may be 
        // some driver-owned requests, but they would all have dropped their 
        // power references by now and would be about to complete. We also know 
        // that no new requests will get added to the queue while this callback
        // is running. The reason is described below.
        //
        // We do not add any new request to this queue until the 
        // PoFxActivateComponent call for that request has returned. If 
        // PoFxActivateComponent is called on another thread while this callback 
        // is running or about to run, then the power framework makes sure 
        // that PoFxActivateComponent does not return until this callback has 
        // finished running. Therefore we know that no new request would get
        // added to this queue while we are attempting to stop the queue 
        // within this callback.
        //
        WdfIoQueueGetState(DevCtx->ComponentInfo[Component].Queue,
                           &queueRequests,
                           NULL /* DriverRequests */);
        if (0 != queueRequests) {
            Trace(TRACE_LEVEL_ERROR, 
                  "%!FUNC! - Expected the queue for component %d to have no "
                  "requests, but there are currently %d requests in the queue.",
                  Component,
                  queueRequests);
            WdfVerifierDbgBreakPoint();
        }
        
        //
        // Stop the component-specific queue.
        //
        // No need for synchronous stop. Given that the component is idle, 
        // there anyway shouldn't be any outstanding requests in sitting in
        // the queue. As explained above, there can be up to one outstanding
        // request that is delivered to the driver layer, but that would 
        // also be just about to complete. We stop this queue only to 
        // prevent new requests that get added to the queue from being 
        // dispatched to the driver layer until the component becomes active
        // again.
        //
        WdfIoQueueStop(DevCtx->ComponentInfo[Component].Queue,
                       NULL, // StopComplete
                       NULL  // Context
                       );
    }            

    return;
}

VOID
_PfhComponentActiveConditionCallback(
    _In_ PVOID Context,
    _In_ ULONG Component
    )
/*++
Routine Description:

    The power framework invokes this routine to notify us that one of our 
    components has become active.

Arguments:

    Context - Context that we passed in to the power framework
      
    Component - Index of component that has become active or idle
    
Return Value:

    None
    
--*/
{
    WDFDEVICE device = NULL;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    PPO_FX_COMPONENT_ACTIVE_CONDITION_CALLBACK 
                        componentActiveConditionCallback = NULL;

    //
    // Get the handle to the framework device object
    //
    device = (WDFDEVICE) Context;

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(device);

    //
    // Component is active
    //
    ComponentActive(devCtx, Component);

    //
    // If the driver layer supplied a component-active-condition callback, 
    // invoke it
    //
    componentActiveConditionCallback = 
        devCtx->DriverLayerPoFxCallbacks.ComponentActiveConditionCallback;
    if (NULL != componentActiveConditionCallback) {
        componentActiveConditionCallback(devCtx->DriverLayerPoFxContext,
                                         Component);
    }

    return;
}

VOID
_PfhComponentIdleConditionCallback(
    _In_ PVOID Context,
    _In_ ULONG Component
    )
/*++
Routine Description:

    The power framework invokes this routine to notify us that one of our 
    components has become idle.

Arguments:

    Context - Context that we passed in to the power framework
      
    Component - Index of component that has become active or idle
    
Return Value:

    None
    
--*/
{
    WDFDEVICE device = NULL;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    PPO_FX_COMPONENT_IDLE_CONDITION_CALLBACK 
                      componentIdleConditionCallback = NULL;

    //
    // Get the handle to the framework device object
    //
    device = (WDFDEVICE) Context;

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(device);

    //
    // Component is idle
    //
    ComponentIdle(devCtx, Component);

    //
    // If the driver layer supplied a component-idle-condition callback, invoke
    // it
    //
    componentIdleConditionCallback = 
        devCtx->DriverLayerPoFxCallbacks.ComponentIdleConditionCallback;
    if (NULL != componentIdleConditionCallback) {
        componentIdleConditionCallback(devCtx->DriverLayerPoFxContext,
                                       Component);
    } else {
        //
        // Complete the transition to idle
        //
        PoFxCompleteIdleCondition(devCtx->PoHandle, Component);
    }

    return;
}

VOID
_PfhComponentIdleStateCallback(
    _In_ PVOID Context,
    _In_ ULONG Component,
    _In_ ULONG State
    )
/*++
Routine Description:

    The power framework invokes this routine to change the F-state of one of our
    components.

Arguments:

    Context - Context that we passed in to the power framework
      
    Component - Index of component for which the F-state change is to be made
    
    State - The new F-state to transition the component to

Return Value:

    None
    
--*/
{
    WDFDEVICE device = NULL;
    PPOFX_DEVICE_CONTEXT devCtx = NULL;
    PPO_FX_COMPONENT_IDLE_STATE_CALLBACK componentIdleStateCallback = NULL;
    //
    // Get the handle to the framework device object
    //
    device = (WDFDEVICE) Context;

    //
    // Get the device context
    //
    devCtx = HelperGetDeviceContext(device);

    //
    // If the driver layer supplied a component-idle-state callback, invoke it
    //
    componentIdleStateCallback = 
        devCtx->DriverLayerPoFxCallbacks.ComponentIdleStateCallback;
    if (NULL != componentIdleStateCallback) {
        componentIdleStateCallback(devCtx->DriverLayerPoFxContext,
                                   Component,
                                   State);
    } else {
        //
        // Complete the idle state transition
        //
        PoFxCompleteIdleState(devCtx->PoHandle, Component);
    }
}
