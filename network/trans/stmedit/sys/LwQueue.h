#ifndef _LWQUEUE_H
#define _LWQUEUE_H

#define POOL_ZERO_DOWN_LEVEL_SUPPORT
#include <wdm.h>

#define STMEDIT_TAG_LQWI 'wLeS'   // Light Weight Queue Work Items.

typedef struct _LW_ENTRY 
{
    struct _LW_ENTRY *Next;
} LW_ENTRY, *PLW_ENTRY;

typedef struct _LW_QUEUE
{
    // Queue Head
    PLW_ENTRY Head;

    // Queue Tail
    PLW_ENTRY Tail;

    // Dummy Queue head.
    LW_ENTRY Dummy;

    // Lock for queue synchronization
    KSPIN_LOCK Lock;

    // If a workitem is already scheduled
    BOOLEAN WorkerScheduled;

    // Is the Queue initialized
    BOOLEAN Initialized;

    // Workitem for IoXxxWorkItem
    PIO_WORKITEM WorkItem;

    // Callback routine to be invoked
    PIO_WORKITEM_ROUTINE WorkerRoutine;

    // One of the caller's device objects.
    PVOID IoObject;

} LW_QUEUE, *PLW_QUEUE;


NTSTATUS
LwInitializeQueue(
    _In_  PVOID IoObject,
    _Out_ PLW_QUEUE Queue,
    _In_  PIO_WORKITEM_ROUTINE WorkerRoutine
    );

VOID
LwUninitializeQueue(
    _Inout_ PLW_QUEUE Queue
    );

VOID
LwEnqueue(
    _In_ PLW_QUEUE Queue,
    _In_ PLW_ENTRY Entry
    );

PLW_ENTRY
LwDequeueAll(
    _In_ PLW_QUEUE Queue
    );


#endif // _LWQUEUE_H
