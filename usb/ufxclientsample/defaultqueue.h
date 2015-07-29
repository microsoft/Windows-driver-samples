/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    defaultqueue.h

Abstract:

    Defines the export functions for the UFXDEVICE object.

Environment:

    Kernel mode

--*/

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _QUEUE_CONTEXT {

    ULONG PrivateDeviceData;  // just a placeholder

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

    
_Must_inspect_result_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
DefaultQueueCreate(
    _In_ WDFDEVICE Device
    );

