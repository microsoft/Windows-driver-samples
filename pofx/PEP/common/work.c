/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    work.c

Abstract:

    This module implements scheduling system for synchronous or
    asynchronous works.


Environment:

    Kernel Mode

--*/

//
//-------------------------------------------------------------------- Includes
//

#include <initguid.h>
#include "pch.h"

#if defined(EVENT_TRACING)
#include "work.tmh"
#endif

//
//------------------------------------------------------------------- Functions
//

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PepCreateWorkRequest (
    _In_ PEP_NOTIFICATION_CLASS WorkType,
    _In_ ULONG NotificationId,
    _In_ PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice,
    _In_ PPEP_DEVICE_DEFINITION DeviceDefinitionEntry,
    _In_opt_ PVOID WorkContext,
    _In_ SIZE_T WorkContextSize,
    _In_opt_ PNTSTATUS WorkRequestStatus,
    _Out_ PPEP_WORK_CONTEXT *OutputWorkRequest
    )

/*++

Routine Description:

    This routine creates a new work request. Note the caller is responsible
    for adding this request to the pending queue after filling in
    request-specific data.

Arguments:

    WorkType - Supplies the type of the work (ACPI/DPM/PPM).

    NotificationId - Supplies the PEP notification type.

    PepInternalDevice - Supplies the internal PEP device.

    DeviceDefinitionEntry - Supplies a pointer to the device definition for
        the device.

    WorkContext - Supplies optional pointer to the context of the work request.

    WorkContextSize - Supplies the size of the work request context.

    WorkRequestStatus -  Supplies optional pointer to report the
        status of the work request.

    OutputWorkRequest - Supplies a pointer that receives the created request.

Return Value:

    NTSTATUS.

--*/

{

    PVOID LocalWorkContext;
    NTSTATUS Status;
    PPEP_WORK_CONTEXT WorkRequest;

    NT_ASSERT(WorkType != PEP_NOTIFICATION_CLASS_NONE);

    WorkRequest = ExAllocatePoolWithTag(NonPagedPoolNx,
                                        sizeof(PEP_WORK_CONTEXT),
                                        PEP_POOL_TAG);

    if (WorkRequest == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        TraceEvents(ERROR,
                    DBG_PEP,
                    "%s: Insufficient resource for creating work request.\n",
                    __FUNCTION__);

        goto CreateWorkRequestEnd;
    }

    LocalWorkContext = NULL;
    if (WorkContext != NULL) {

        NT_ASSERT(WorkContextSize != 0);

        LocalWorkContext = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                 WorkContextSize,
                                                 PEP_POOL_TAG);

        if (LocalWorkContext == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s: Insufficient resource for "
                        "creating work request context.\n",
                        __FUNCTION__);

            goto CreateWorkRequestEnd;
        }

        RtlCopyMemory(LocalWorkContext, WorkContext, WorkContextSize);

    } else {
        WorkContextSize = 0;
    }

    RtlZeroMemory(WorkRequest, sizeof(PEP_WORK_CONTEXT));
    InitializeListHead(&WorkRequest->ListEntry);
    WorkRequest->WorkType = WorkType;
    WorkRequest->NotificationId = NotificationId;
    WorkRequest->PepInternalDevice = PepInternalDevice;
    WorkRequest->DeviceDefinitionEntry = DeviceDefinitionEntry;
    WorkRequest->WorkContextSize = WorkContextSize;
    WorkRequest->WorkContext = LocalWorkContext;
    WorkRequest->WorkRequestStatus = WorkRequestStatus;
    WorkRequest->WorkCompleted = FALSE;
    *OutputWorkRequest = WorkRequest;
    Status = STATUS_SUCCESS;

CreateWorkRequestEnd:
    return Status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PepDestroyWorkRequest (
    _In_ PPEP_WORK_CONTEXT WorkRequest
    )

/*++

Routine Description:

    This routine destroys the given work request.

Arguments:

    WorkRequest - Supplies a pointer to the work request.

Return Value:

    None.

--*/

{

    if (WorkRequest->WorkContext != NULL) {
        ExFreePoolWithTag(WorkRequest->WorkContext, PEP_POOL_TAG);
    }

    ExFreePoolWithTag(WorkRequest, PEP_POOL_TAG);
    return;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PepPendWorkRequest (
    _In_ PPEP_WORK_CONTEXT WorkRequest
    )

/*++

Routine Description:

    This routine adds the given work request to the pending queue.

Arguments:

    WorkRequest - Supplies a pointer to the work request.

Return Value:

    None.

--*/

{

    //
    // Ensure that the request is not already on some other queue.
    //

    NT_ASSERT(IsListEmpty(&WorkRequest->ListEntry) != FALSE);

    TraceEvents(INFO,
                DBG_PEP,
                "%s: Insert pending work request. "
                "Device=%p, WorkType=%d, NotificationId=%d.\n",
                __FUNCTION__,
                (PVOID)WorkRequest->PepInternalDevice,
                (ULONG)WorkRequest->WorkType,
                (ULONG)WorkRequest->NotificationId);

    //
    // Add the new request to the end of tail of the pending work queue.
    //

    WdfSpinLockAcquire(PepWorkListLock);
    InsertTailList(&PepPendingWorkList, &WorkRequest->ListEntry);
    WdfSpinLockRelease(PepWorkListLock);

    //
    // Schedule a worker to pick up the new work.
    //

    PepScheduleWorker(WorkRequest);
    return;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PepCompleteWorkRequest (
    _In_ PPEP_WORK_CONTEXT WorkRequest
    )

/*++

Routine Description:

    This routine adds the given work request to the completed queue.

Arguments:

    WorkRequest - Supplies a pointer to the work request.

Return Value:

    None.

--*/

{

    //
    // Mark the request as completed.
    //

    PepMarkWorkRequestComplete(WorkRequest);

    //
    // Ensure that the request is not already on some other queue.
    //

    NT_ASSERT(IsListEmpty(&WorkRequest->ListEntry) != FALSE);
    NT_ASSERT(WorkRequest->WorkCompleted != FALSE);

    TraceEvents(INFO,
                DBG_PEP,
                "%s: Insert complete work request. "
                "Device=%p, WorkType=%d, NotificationId=%d.\n",
                __FUNCTION__,
                (PVOID)WorkRequest->PepInternalDevice,
                (ULONG)WorkRequest->WorkType,
                (ULONG)WorkRequest->NotificationId);

    //
    // Move the request into the completed queue.
    //

    WdfSpinLockAcquire(PepWorkListLock);
    InsertTailList(&PepCompletedWorkList, &(WorkRequest->ListEntry));
    WdfSpinLockRelease(PepWorkListLock);

    //
    // Request Windows Runtime Power framework to query PEP for more work. It
    // will be notified of completed work in the context of that query.
    //

    PepKernelInformation.RequestWorker(PepKernelInformation.Plugin);
    return;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
PepMarkWorkRequestComplete (
    _In_ PPEP_WORK_CONTEXT WorkRequest
    )

/*++

Routine Description:

    This routine marks the given work request as completed. This will cause
    it to be moved to the completion queue.

Arguments:

    WorkRequest - Supplies a pointer to the work request.

Return Value:

    None.

--*/

{

    //
    // Ensure the request wasn't already completed in a different context
    // (and thus potentially already on the completed queue).
    //

    NT_ASSERT(WorkRequest->WorkCompleted == FALSE);

    WorkRequest->WorkCompleted = TRUE;
    return;
}

NTSTATUS
PepCompleteSelfManagedWork (
    _In_ PEP_NOTIFICATION_CLASS WorkType,
    _In_ ULONG NotificationId,
    _In_ PPEP_INTERNAL_DEVICE_HEADER PepInternalDevice,
    _In_ PPEP_WORK_INFORMATION PoFxWorkInfo
    )

/*++

Routine Description:

    This routine completes PEP self-managed work that was previously deferred.

Arguments:

    WorkType - Supplies the type of the work (ACPI/DPM/PPM).

    NotificationId - Supplies the PEP notification type.

    PepInternalDevice - Supplies the internal PEP device.

    PoFxWorkInfo - Supplies a pointer to the PPEP_WORK_INFORMATION structure
        used to report result to PoFx.

Return Value:

    NTSTATUS code.

--*/

{

    PPEP_DEVICE_DEFINITION DeviceDefinition;
    NTSTATUS Status;
    PPEP_WORK_CONTEXT WorkRequest;

    DeviceDefinition = PepInternalDevice->DeviceDefinition;
    Status = PepCreateWorkRequest(WorkType,
                                  NotificationId,
                                  PepInternalDevice,
                                  DeviceDefinition,
                                  NULL,
                                  0,
                                  NULL,
                                  &WorkRequest);

    if (!NT_SUCCESS(Status)) {
        TraceEvents(ERROR,
                    DBG_PEP,
                    "%s: PepCreateWorkRequest() failed!. "
                    "Status = %!STATUS!.\n ",
                    __FUNCTION__,
                    Status);

        goto CompleteWorkSelfManagedEnd;
    }

    RtlCopyMemory(&WorkRequest->LocalPoFxWorkInfo,
                  PoFxWorkInfo,
                  sizeof(PEP_WORK_INFORMATION));

    PepCompleteWorkRequest(WorkRequest);

CompleteWorkSelfManagedEnd:
    return Status;
}

NTSTATUS
PepScheduleWorker (
    _In_ PPEP_WORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function schedules a worker thread to process pending work requests.

Arguments:

    WorkContext - Supplies the context of the work.

Return Value:

    NTSTATUS.

--*/

{

    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;
    BOOLEAN Synchronous;
    WDFWORKITEM WorkItem;
    WDF_WORKITEM_CONFIG WorkItemConfiguration;

    WorkItem = NULL;
    Synchronous = FALSE;

    //
    // Create a workitem to process events.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&Attributes,
                                           PEP_WORK_ITEM_CONTEXT);

    Attributes.ParentObject = PepGlobalWdfDevice;

    //
    // Initialize the handler routine and create a new workitem.
    //

    WDF_WORKITEM_CONFIG_INIT(&WorkItemConfiguration, PepWorkerWrapper);

    //
    // Disable automatic serialization by the framework for the worker thread.
    // The parent device object is being serialized at device level (i.e.,
    // WdfSynchronizationScopeDevice), and the framework requires it to be
    // passive level (i.e., WdfExecutionLevelPassive) if automatic
    // synchronization is desired.
    //

    WorkItemConfiguration.AutomaticSerialization = FALSE;

    //
    // Create the work item and queue it. If the workitem cannot be created
    // for some reason, just call the worker routine synchronously.
    //

    Status = WdfWorkItemCreate(&WorkItemConfiguration,
                               &Attributes,
                               &WorkItem);

    if (!NT_SUCCESS(Status)) {
        Synchronous = TRUE;
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s: Failed to allocate work item to process pending"
                    "work! Status = %!STATUS!. Will synchronously process.\n",
                    __FUNCTION__,
                    Status);
    }

    //
    // If the operation is to be performed synchronously, then directly
    // invoke the worker routine. Otherwise, queue a workitem to run the
    // worker routine.
    //

    if (Synchronous != FALSE) {
        PepProcessPendingWorkRequests();

    } else {
        TraceEvents(INFO,
                    DBG_PEP,
                    "%s: Work request scheduled to run asynchronously. "
                    "Device=%p, WorkType=%d, NotificationId=%d.\n",
                    __FUNCTION__,
                    (PVOID)WorkContext->PepInternalDevice,
                    (ULONG)WorkContext->WorkType,
                    (ULONG)WorkContext->NotificationId);

        WdfWorkItemEnqueue(WorkItem);
    }

    return STATUS_SUCCESS;
}

VOID
PepWorkerWrapper (
    _In_ WDFWORKITEM WorkItem
    )

/*++

Routine Description:

    This routine is wrapper for the actual worker routine that processes
    pending work.

Arguments:

    WorkItem -  Supplies a handle to the workitem supplying the context.

Return Value:

    None.

--*/

{

    PPEP_WORK_ITEM_CONTEXT Context;

    Context = PepGetWorkItemContext(WorkItem);
    PepProcessPendingWorkRequests();

    //
    // Delete the work item as it is no longer required.
    //

    WdfObjectDelete(WorkItem);
    return;
}

VOID
PepProcessPendingWorkRequests (
    VOID
    )

/*++

Routine Description:

    This function processes all pending work. It calls the handler
    routine for each pending work.

Arguments:

    WorkType - Supplies the type of the work (ACPI/DPM/PPM).

Return Value:

    None.

--*/

{

    PLIST_ENTRY NextEntry;
    PPEP_WORK_CONTEXT WorkRequest;

    //
    // Go through pending work list and handle them.
    //

    WdfSpinLockAcquire(PepWorkListLock);

    while (IsListEmpty(&PepPendingWorkList) == FALSE) {
        NextEntry = RemoveHeadList(&PepPendingWorkList);
        InitializeListHead(NextEntry);
        WorkRequest = CONTAINING_RECORD(NextEntry,
                                        PEP_WORK_CONTEXT,
                                        ListEntry);

        //
        // Drop the request list lock prior to processing work.
        //

        WdfSpinLockRelease(PepWorkListLock);

        //
        // Invoke the request processing async handler.
        //

        NT_ASSERT(WorkRequest->WorkCompleted == FALSE);

        TraceEvents(INFO,
                    DBG_PEP,
                    "%s: Asynchronously processing request. "
                    "Device=%p, WorkType=%d, NotificationId=%d.\n",
                    __FUNCTION__,
                    (PVOID)WorkRequest->PepInternalDevice,
                    (ULONG)WorkRequest->WorkType,
                    (ULONG)WorkRequest->NotificationId);

        PepInvokeNotificationHandler(
            WorkRequest->WorkType,
            WorkRequest,
            PepHandlerTypeWorkerCallback,
            WorkRequest->NotificationId,
            WorkRequest->PepInternalDevice,
            WorkRequest->WorkContext,
            WorkRequest->WorkContextSize,
            WorkRequest->WorkRequestStatus);

        //
        // Reacquire the request list lock prior to dequeuing next request.
        //

        WdfSpinLockAcquire(PepWorkListLock);
    }

    WdfSpinLockRelease(PepWorkListLock);
    return;
}

VOID
PepProcessCompleteWorkRequests (
    _In_ PVOID Data
    )

/*++

Routine Description:

    This function completes work by calling into the specific
    completion handler, which is responsible for filling in the PEP_WORK
    structure.

    Differences with pending worker routine:
        - Keeps invoking PepKernelInformation.RequestWorker until the
          completed work queue is not drained completely.

Arguments:

    Data - Supplies a pointer to the PEP_WORK structure.

    WorkType - Supplies the type of the work (ACPI/DPM/PPM).

Return Value:

    None.

--*/

{

    PLIST_ENTRY NextEntry;
    BOOLEAN MoreWork;
    PPEP_WORK_CONTEXT WorkRequest;
    PPEP_WORK PoFxWork;

    MoreWork = FALSE;
    NextEntry = NULL;
    PoFxWork = (PPEP_WORK)Data;

    //
    // Grab the next item from the completed work queue.
    //

    WdfSpinLockAcquire(PepWorkListLock);

    if (IsListEmpty(&PepCompletedWorkList) == FALSE) {
        NextEntry = RemoveHeadList(&PepCompletedWorkList);

        //
        // Check if there is more work after this request.
        //

        if (IsListEmpty(&PepCompletedWorkList) == FALSE) {
            MoreWork = TRUE;
        }
    }

    //
    // Drop the request list lock prior to processing work.
    //

    WdfSpinLockRelease(PepWorkListLock);

    //
    // If a completed request was found, report back to PoFx and
    // reclaim its resources.
    //

    if (NextEntry != NULL) {
        InitializeListHead(NextEntry);
        WorkRequest = CONTAINING_RECORD(NextEntry,
                                        PEP_WORK_CONTEXT,
                                        ListEntry);

        //
        // Invoke the request processing routine.
        //

        switch (WorkRequest->WorkType) {
        case PEP_NOTIFICATION_CLASS_ACPI:
            PoFxWork->NeedWork = TRUE;
            RtlCopyMemory(PoFxWork->WorkInformation,
                          &WorkRequest->LocalPoFxWorkInfo,
                          sizeof(PEP_WORK_INFORMATION));

        case PEP_NOTIFICATION_CLASS_DPM:
            break;

        default:
            TraceEvents(ERROR,
                        DBG_PEP,
                        "%s: Unknown WorkType = %d.\n",
                        __FUNCTION__,
                        (ULONG)WorkRequest->WorkType);
        }

        //
        // Destroy the request.
        //

        PepDestroyWorkRequest(WorkRequest);
    }

    //
    // If there is more work left, then request another PEP_WORK.
    //

    if (MoreWork != FALSE) {
        PepKernelInformation.RequestWorker(PepKernelInformation.Plugin);
    }

    return;
}

