/*++
Copyright (c) Microsoft Corporation. All rights reserved

Abstract:
	Stream Edit Callout Driver Sample.
	This file implements Light Weight queues using Worker Item routines.

Environment:
	Kernel mode

--*/

#define POOL_ZERO_DOWN_LEVEL_SUPPORT
#include "LwQueue.h"

IO_WORKITEM_ROUTINE     LwWorker;

NTSTATUS
LwInitializeQueue(
    _In_  PVOID IoObject,
    _Out_ PLW_QUEUE Queue,
    _In_  PIO_WORKITEM_ROUTINE WorkerRoutine
    )
/*
    Initializes Light Weight Queue data structures!
*/
{
    RtlZeroMemory(Queue, sizeof(Queue[0]));

    Queue->Head = &Queue->Dummy;
    Queue->Tail = &Queue->Dummy;
    Queue->WorkerScheduled = FALSE;

    Queue->WorkerRoutine = WorkerRoutine;
    Queue->IoObject = IoObject;

    KeInitializeSpinLock(&Queue->Lock);

    Queue->WorkItem =
        ExAllocatePool2(
            POOL_FLAG_NON_PAGED, IoSizeofWorkItem(), STMEDIT_TAG_LQWI);

    if (Queue->WorkItem == NULL) 
	{
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoInitializeWorkItem(IoObject, Queue->WorkItem);
    Queue->Initialized = TRUE;
    return STATUS_SUCCESS;
}

VOID
LwUninitializeQueue(
    _Inout_ PLW_QUEUE Queue
    )
/*
    De-Initializes the Light Weight Queue!
*/
{
    if (Queue->Initialized == FALSE)
	{
        return;
    }
    
    Queue->Initialized = FALSE;

    IoUninitializeWorkItem(Queue->WorkItem);
    ExFreePoolWithTag(Queue->WorkItem, STMEDIT_TAG_LQWI);
    Queue->WorkItem = NULL;
}

__drv_functionClass(IO_WORKITEM_ROUTINE)
__drv_requiresIRQL(PASSIVE_LEVEL)
__drv_sameIRQL
VOID
LwWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
    )
/*
    Queue processing workitem routine.
    Drains the task queue and invokes OOB Workitem to process the tasks.
*/
{
    PLW_ENTRY Entry;
    PLW_QUEUE Queue = (PLW_QUEUE)Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    NT_ASSERT(Queue != NULL);
    Entry = LwDequeueAll(Queue);

    //
    // Why were we scheduled if there are no entries?
    //
    NT_ASSERT(Entry != NULL);

    while (Entry != NULL)
    {
        //
        // Invoke the caller's worker routine.
        //
        Queue->WorkerRoutine(DeviceObject, Entry);

        //
        // Check if any other entries were added while we were
        // working.
        //
        Entry = LwDequeueAll(Queue);
    }
}

VOID
LwEnqueue(
    _In_ PLW_QUEUE Queue,
    _In_ PLW_ENTRY Entry
    )
/*
    Queue a task into the task queue.
*/
{
    KLOCK_QUEUE_HANDLE LockHandle;

    NT_ASSERT(Entry != NULL);
    NT_ASSERT(Queue != NULL);

    KeAcquireInStackQueuedSpinLock(&Queue->Lock, &LockHandle);

    Entry->Next = NULL;
    Queue->Tail->Next = Entry;
    Queue->Tail = Entry;

    if (!Queue->WorkerScheduled) 
	{
        Queue->WorkerScheduled = TRUE;

        IoQueueWorkItem(
            Queue->WorkItem, LwWorker, DelayedWorkQueue, Queue);
    }

    KeReleaseInStackQueuedSpinLock(&LockHandle);
}

PLW_ENTRY
LwDequeueAll(
    _In_ PLW_QUEUE Queue
    )
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PLW_ENTRY Entry = NULL;

    KeAcquireInStackQueuedSpinLock(&Queue->Lock, &LockHandle);

    NT_ASSERT(Queue->Head == &Queue->Dummy);

    //
    // Snap and return the entire queue contents.
    //
    Entry = Queue->Head->Next;
    Queue->Head->Next = NULL;
    Queue->Tail = Queue->Head;

    if (Entry == NULL) 
	{
        Queue->WorkerScheduled = FALSE;
    }

    KeReleaseInStackQueuedSpinLock(&LockHandle);
    return Entry;
}
