/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    transfer.h

Abstract:

    Header file for the transfer related functions

Environment:

    Kernel mode

--*/

#pragma once

#include "ufxclient.h"
#include "registers.h"
#include "event.h"

#define MAX_TRANSFER_SIZE 0x20000000

// nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)

typedef enum _DMA_STATE {
    NotExecuted = 0,
    Executed,
    Cancelled,
    Completed
} DMA_STATE;

typedef struct _CONTROL_CONTEXT {
    WDFCOMMONBUFFER SetupPacketBuffer;
    BOOLEAN SetupRequested;
    BOOLEAN ReadyForHandshake;
    BOOLEAN HandshakeRequested;
    BOOLEAN DataStageExists;
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;
    WDFREQUEST HandshakeRequest;
} CONTROL_CONTEXT, *PCONTROL_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_CONTEXT, UfxEndpointGetControlContext);

typedef struct _TRANSFER_CONTEXT {
    WDFDEVICE WdfDevice;
    WDFCOMMONBUFFER CommonBuffer;
    WDFCOMMONBUFFER ExtraBuffer;
    WDFDMATRANSACTION Transaction;
    PHYSICAL_ADDRESS LogicalCommonBuffer;
    PUCHAR Buffer;
    WDFSPINLOCK TransferLock;
    BOOLEAN TransferStarted;
    BOOLEAN EndRequested;
    BOOLEAN Stalled;
    BOOLEAN Enabled;
    BOOLEAN TransferCommandStartComplete;
    BOOLEAN CleanupOnEndComplete;
    WDFWORKITEM CompletionWorkItem;
    BOOLEAN PendingCompletion;
} TRANSFER_CONTEXT, *PTRANSFER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(TRANSFER_CONTEXT, UfxEndpointGetTransferContext);

typedef struct _DMA_CONTEXT {
    UFXENDPOINT Endpoint;
    ULONG BytesRequested;
    ULONG BytesProgrammed;
    ULONG BytesProgrammedCurrent;
    ULONG ExtraBytes;
    BOOLEAN DirectionIn;
    PSCATTER_GATHER_LIST SgList;
    ULONG SgIndex;
    WDFREQUEST Request;
    BOOLEAN ZeroLength;
    DMA_STATE State;
    ULONG BytesRemaining;
    BOOLEAN CleanupOnProgramDma;
    BOOLEAN NeedExtraBuffer;
} DMA_CONTEXT, *PDMA_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DMA_CONTEXT, DmaGetContext);

typedef struct _REQUEST_CONTEXT {
    UFXENDPOINT Endpoint;
    WDFDMATRANSACTION Transaction;
    BOOLEAN QueueIsStopped;
    BOOLEAN ReferencedOnCancel;
} REQUEST_CONTEXT, *PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, RequestGetContext);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
TransferInitialize (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandStartComplete (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandEndComplete (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID

TransferComplete (
    _In_ UFXENDPOINT Endpoint
    );


_IRQL_requires_(PASSIVE_LEVEL)
VOID
TransferDestroy (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferReset (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferStart (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferRequestCancel (
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN QueueIsStopped
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandStallClear (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandStallSet (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferStallClearComplete (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferStallSetComplete (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferGetStall (
    _In_ UFXENDPOINT Endpoint,
    _Out_ PBOOLEAN Stalled
    );

EVT_WDF_IO_QUEUE_STATE TransferReadyNotify;