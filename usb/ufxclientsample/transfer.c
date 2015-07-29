/*++

Module Name:

    transfer.c

Abstract:

    Transfer related functions

Environment:

    Kernel mode

--*/

#include "transfer.h"
#include "ufxendpoint.h"
#include "registers.h"
#include "device.h"
#include "ufxdevice.h"
#include <usbfnioctl.h>
#include "trace.h"
#include "transfer.tmh"

#define MAX_PHYSICAL_ENDPOINTS 32
#define COMMON_BUFFER_ALIGNMENT 15
#define ENDPOINT_COMMON_BUFFER_SIZE 0x60
#define DATA_ALIGNMENT 63

FORCEINLINE
VOID
TraceTransfer (
    _In_ PCSTR Stage,
    _In_ UFXENDPOINT Endpoint,
    _In_opt_ WDFREQUEST Request
    )
{
    WDFDMATRANSACTION Transaction;
    ULONG BytesRequested;
    ULONG BytesProgrammed;
    ULONG BytesTransferred;
    ULONG SgProgrammed;
    ULONG SgLength;

    Transaction = NULL;
    BytesRequested = 0;
    BytesProgrammed = 0;
    BytesTransferred = 0;
    SgProgrammed = 0;
    SgLength = 0;

    if (Request == NULL) {    
        Transaction = UfxEndpointGetTransferContext(Endpoint)->Transaction;
        if (Transaction != NULL) {
            PDMA_CONTEXT DmaContext;
            DmaContext = DmaGetContext(Transaction);
            Request = DmaContext->Request;
        }
    }

    if (Request != NULL) {
        PREQUEST_CONTEXT RequestContext;
        RequestContext = RequestGetContext(Request);

        if (RequestContext->Transaction != NULL) {
            PDMA_CONTEXT DmaContext;
            DmaContext = DmaGetContext(RequestContext->Transaction);

            Transaction = RequestContext->Transaction;
            BytesRequested = DmaContext->BytesRequested;
            BytesProgrammed = DmaContext->BytesProgrammed;
            BytesTransferred = BytesProgrammed - DmaContext->BytesRemaining;
            if (DmaContext->SgList != NULL) {
                SgProgrammed = DmaContext->SgIndex;
                SgLength = DmaContext->SgList->NumberOfElements;
            }
        }
    }

    EventWriteTransfer( 
        &UfxClientSampleGuid, 
        Stage,
        (ULONG) Endpoint,
        UfxEndpointGetContext(Endpoint)->PhysicalEndpoint,
        (ULONG) Request,
        (ULONG) Transaction,
        BytesRequested,
        BytesProgrammed,
        BytesTransferred,
        SgProgrammed,
        SgLength);

    TraceInformation("TRANSFER %s: %08X (%d), RQ: %08X, DMA: %08X, BytesReq: %08X, BytesProg: %08X, BytesTrans: %08X, SG: %d/%d",
        Stage, (ULONG) Endpoint, UfxEndpointGetContext(Endpoint)->PhysicalEndpoint, (ULONG) Request,
        (ULONG) Transaction, BytesRequested, BytesProgrammed, BytesTransferred, SgProgrammed, SgLength);
}

#define TRACE_TRANSFER(Stage, Endpoint, Request) \
    TraceTransfer(Stage, Endpoint, Request)

EVT_WDF_PROGRAM_DMA OnEvtProgramDma;
EVT_WDF_REQUEST_CANCEL OnEvtRequestCancel;
EVT_WDF_WORKITEM TransferCompleteNext;

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFDMATRANSACTION
TransferNextRequest (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFDMATRANSACTION
TransferRequestTryUnmarkCancelableAndCleanup (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFDMATRANSACTION
TransferCancelCleanup (
    _In_ UFXENDPOINT Endpoint
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_raises_(DISPATCH_LEVEL)
_IRQL_saves_global_(EpTransferLock, Endpoint)
VOID
TransferLock (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Recursive Spinlock Lock

Parameters Description:

    Endpoint - The endpoint to lock

--*/
{
    WdfSpinLockAcquire(UfxEndpointGetTransferContext(Endpoint)->TransferLock);
}

_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_restores_global_(EpTransferLock, Endpoint)
VOID
TransferUnlock (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Recursive Spinlock Unlock

Parameters Description:

    Endpoint - The endpoint to unlock

--*/
{
    WdfSpinLockRelease(UfxEndpointGetTransferContext(Endpoint)->TransferLock);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
CommandStallSet (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Sends a stall set command on the endpoint.

Parameters Description:

    Endpoint - Endpoint to set stall.
--*/
{
    PTRANSFER_CONTEXT TransferContext;
 
    TraceEntry();

    TransferContext = UfxEndpointGetTransferContext(Endpoint);

    //
    // #### TODO: Insert code to set stall on the endpoint ####
    //

    TransferContext->Stalled = TRUE;

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
CommandStallClear (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Sends a stall clear command on the endpoint.

Parameters Description:

    Endpoint - Endpoint to clear stall.

--*/
{
    PTRANSFER_CONTEXT TransferContext;

    TraceEntry();

    TransferContext = UfxEndpointGetTransferContext(Endpoint);

    NT_ASSERT(!CONTROL_ENDPOINT(Endpoint));

    //
    // #### TODO: Insert code to clear stall on the endpoint ####
    //

    TransferContext->Stalled = FALSE;

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandEnd (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Commands the controller to End Transfer.

Parameters Description:

    Endpoint - The endpoint on which to transfer.

--*/
{
    PTRANSFER_CONTEXT TransferContext;

    TraceEntry();

    TransferContext = UfxEndpointGetTransferContext(Endpoint);

    NT_ASSERT(TransferContext->TransferStarted);
    if (!TransferContext->EndRequested) {

        TransferContext->EndRequested = TRUE;

        //
        // #### TODO: Insert code to end the transfer on the endpoint ####
        //

    }

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandStart (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Commands the controller to Start Transfer.

Parameters Description:

    Endpoint - The endpoint on which to transfer.

--*/
{
    PTRANSFER_CONTEXT TransferContext;

    TraceEntry();

    TransferContext = UfxEndpointGetTransferContext(Endpoint);

    TransferContext->TransferStarted = TRUE;
    TransferContext->TransferCommandStartComplete = FALSE;

    //
    // #### TODO: Insert code to start the transfer on the endpoint ####
    //

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandUpdate (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Commands the controller to Update Transfer.

Parameters Description:

    Endpoint - The endpoint on which to transfer.

--*/
{
    TraceEntry();

    UNREFERENCED_PARAMETER(Endpoint);

    //
    // #### TODO: Insert code to update the transfer on the endpoint ####
    //

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandStartOrUpdate (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    If Start Transfer has not been called the the ProgrammingRequest,
    then this will call Start Transfer, otherwise Update Transfer.

Parameters Description:

    Endpoint - The endpoint on which to transfer.

--*/
{
    PTRANSFER_CONTEXT TransferContext;

    TraceEntry();

    TransferContext = UfxEndpointGetTransferContext(Endpoint);
                           
    if (TransferContext->TransferStarted) {
        TransferCommandUpdate(Endpoint);

    } else {
        TransferCommandStart(Endpoint);
    }

    TraceExit();
}

VOID
TransferDmaExecute (
    _In_opt_ WDFDMATRANSACTION Transaction
    )
/*++
Routine Description:

    Execute a DMA transfer for non-zero length transfer.

Parameters Description:

    Transaction - Transaction to execute.

--*/
{
    TraceEntry();

    if (Transaction != NULL) {
        NTSTATUS Status;
        PDMA_CONTEXT DmaContext;
        UFXENDPOINT Endpoint;
        
        DmaContext = DmaGetContext(Transaction);
        Endpoint = DmaContext->Endpoint;

        if (!DmaContext->ZeroLength) {
            Status = WdfDmaTransactionExecute(Transaction, DmaContext->Endpoint);
            LOG_NT_MSG(Status, "Failed to execute DMA Transaction");

            if (!NT_SUCCESS(Status)) {
                WDFDMATRANSACTION NewTransaction = NULL;

                TransferLock(Endpoint);
                if (DmaContext->CleanupOnProgramDma) {
                    NewTransaction = TransferCancelCleanup(Endpoint);
                } else {
                    NewTransaction = TransferRequestTryUnmarkCancelableAndCleanup(Endpoint);
                }
                TransferUnlock(Endpoint);
                TransferDmaExecute(NewTransaction);
            }
        }
    }

    TraceExit();
}

VOID
OnEvtRequestCancel (
    _In_ WDFREQUEST Request
    )
/*++
Routine Description:

    EvtRequestCancel callback for cancellable transfer requests.
    This cancels a programmed transfer request by issuing a corresponding 
    'TransferEnd' command to the contoller.

Parameters Description:

    Request - Request being cancelled.    

--*/
{
    TraceEntry();
    TransferRequestCancel(Request, FALSE);
    TraceExit();
}

_Use_decl_annotations_
VOID
TransferRequestCancel (
    WDFREQUEST Request,
    BOOLEAN QueueIsStopped
    )
/*++
Routine Description:

    This cancels a programmed transfer request by issuing a corresponding 
    'TransferEnd' command to the contoller.

Parameters Description:

    Request - Request being cancelled.    

    QueueIsStopped - indicates if this request is being stopped and cancellation 
                     should avoid fetching subsequent request from queue

--*/
{
    PREQUEST_CONTEXT RequestContext;
    PTRANSFER_CONTEXT TransferContext;
    WDFDMATRANSACTION NewTransaction;
    UFXENDPOINT Endpoint;
    NTSTATUS Status;
    BOOLEAN Cleanup;

    TraceEntry();

    RequestContext = RequestGetContext(Request);
    TransferContext = UfxEndpointGetTransferContext(RequestContext->Endpoint);
    NewTransaction = NULL;
    Endpoint = RequestContext->Endpoint;

    TransferLock(Endpoint);

    if (QueueIsStopped) {
        TRACE_TRANSFER("STOP", Endpoint, Request);
        RequestContext->QueueIsStopped = TRUE;

        Status = WdfRequestUnmarkCancelable(Request);
        CHK_NT_MSG(Status, "WdfRequestUnmarkCancelable failed during queue stop");
        
    } else {
        TRACE_TRANSFER("CANCEL", Endpoint, Request);
    }

    //
    // After this function exists, KMDF will release the reference to this
    // request. Acquire a reference to make sure it doesn't get deleted.
    //
    RequestContext->ReferencedOnCancel = TRUE;
    WdfObjectReference(Request);

    Cleanup = TRUE;
    if (RequestContext->Transaction != NULL) {
        PDMA_CONTEXT DmaContext;

        DmaContext = DmaGetContext(RequestContext->Transaction);

        //
        // Try to cancel the transaction. If we succeed, we can complete the request.
        //
        if (!DmaContext->State == NotExecuted) {
            if (WdfDmaTransactionCancel(RequestContext->Transaction)) {
                //
                // Contrary to what MSDN says, completing the transaction at this point
                // is considered an error by driver verifier since the DMA state is
                // FxDmaTransactionStateTransferFailed.
                //
                
                DmaContext->State = Cancelled;

            } else {
                DmaContext->CleanupOnProgramDma = TRUE;
                Cleanup = FALSE;
            }
        }
    }

    //
    // If a transfer is in progress, end it. Otherwise, just clean
    // up the request.
    //
    if (TransferContext->TransferStarted) {
        TransferContext->CleanupOnEndComplete = TRUE;
        TransferCommandEnd(Endpoint);
            
    } else if (Cleanup) {
        NewTransaction = TransferCancelCleanup(Endpoint);
    }

End:
    TransferUnlock(Endpoint);
    TransferDmaExecute(NewTransaction);

    TraceExit();
}


_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferSetupPacket (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Programs a setup packet for a control endpoint. This must be called
    when the endpoint is first configured, when it is stalled,
    and every time a handshake is completed.

Parameters Description:

    Endpoint - The endpoint on which to transfer.

--*/
{
    PCONTROL_CONTEXT ControlContext;

    TraceEntry();

    ControlContext = UfxEndpointGetControlContext(Endpoint);

    TRACE_TRANSFER("SETUP", Endpoint, NULL);

    //
    // Start transfer
    //
                  
    TransferCommandStart(Endpoint);
    ControlContext->SetupRequested = TRUE;
    ControlContext->HandshakeRequested = FALSE;
    ControlContext->DataStageExists = FALSE;
    ControlContext->ReadyForHandshake = FALSE;

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFDMATRANSACTION
TransferRequestTryUnmarkCancelableAndCleanup (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Attempt to cancel a pending transfer for an endpoint

Parameters Description:

    Endpoint - The endpoint for the transfer to be cancelled.

--*/
{
    NTSTATUS Status;
    PTRANSFER_CONTEXT TransferContext;
    WDFDMATRANSACTION NewTransaction;
    WDFREQUEST RequestToCancel;

    TraceEntry();

    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    NewTransaction = NULL;
    RequestToCancel = NULL;

    //
    // Try to find a request to clean up.
    //
    if (TransferContext->Transaction != NULL) {
        PDMA_CONTEXT DmaContext;
        DmaContext = DmaGetContext(TransferContext->Transaction);

        RequestToCancel = DmaContext->Request;

    } else if (CONTROL_ENDPOINT(Endpoint)) {
        PCONTROL_CONTEXT ControlContext;
        ControlContext = UfxEndpointGetControlContext(Endpoint);

        RequestToCancel = ControlContext->HandshakeRequest;
    }   

    //
    // Before canceling, we need to make sure we can unmark it cancelable.
    // If not, then EvtRequestCancel will get called, and it will cancel
    // the request.
    //
    if (RequestToCancel != NULL) {
        Status = WdfRequestUnmarkCancelable(RequestToCancel);
        CHK_NT_MSG(Status, "Failed to unmark cancelable on try cleanup");
    }

    NewTransaction = TransferCancelCleanup(Endpoint);

End:
    TraceExit();
    return NewTransaction;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFDMATRANSACTION
TransferCancelCleanup (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Cleans up and cancels the current transfer for an endpoint

Parameters Description:

    Endpoint - The endpoint for the transfer to be cancelled.

--*/
{
    PTRANSFER_CONTEXT TransferContext;
    NTSTATUS Status;
    WDFDMATRANSACTION Transaction;
    BOOLEAN FetchNextRequest;
    WDFREQUEST RequestToCancel;
    WDFDMATRANSACTION NewTransaction;

    TraceEntry();

    TransferContext = UfxEndpointGetTransferContext(Endpoint);

    FetchNextRequest = FALSE;
    Transaction = NULL;
    RequestToCancel = NULL;
    NewTransaction = NULL;

    //
    // Reset data transfer state and complete request
    //
    Transaction = TransferContext->Transaction;
    if (Transaction != NULL) {
        PDMA_CONTEXT DmaContext;
        
        DmaContext = DmaGetContext(Transaction);

        RequestToCancel = DmaContext->Request;
        TRACE_TRANSFER("CLEANUP (Data)", Endpoint, RequestToCancel);
        if (DmaContext->State == Executed) {
            (void) WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &Status);
            DmaContext->State = Completed;
        }
        WdfObjectDelete(Transaction);
        TransferContext->Transaction = NULL;
    }

    //
    // Reset control endpoint state.
    //
    if (CONTROL_ENDPOINT(Endpoint)) {
        PCONTROL_CONTEXT ControlContext;

        ControlContext = UfxEndpointGetControlContext(Endpoint);
        ControlContext->DataStageExists = FALSE;
        ControlContext->HandshakeRequested = FALSE;
        ControlContext->SetupRequested = FALSE;
        ControlContext->ReadyForHandshake = FALSE;
        if (ControlContext->HandshakeRequest != NULL) {
            RequestToCancel = ControlContext->HandshakeRequest;
            TRACE_TRANSFER("CLEANUP (Handshake)", Endpoint, RequestToCancel);
            ControlContext->HandshakeRequest = NULL;
        }
    }

    if (RequestToCancel != NULL) {
        BOOLEAN Referenced;
        PREQUEST_CONTEXT RequestContext;

        RequestContext = RequestGetContext(RequestToCancel);

        FetchNextRequest = !RequestContext->QueueIsStopped;

        Referenced = RequestContext->ReferencedOnCancel;
        
        //
        // In case the completion work-item has already been queued, prevent
        // it from also trying to complete this request when it executes.
        //
        TransferContext->PendingCompletion = FALSE;
        
        WdfRequestComplete(RequestToCancel, STATUS_CANCELLED);
        if (Referenced) {
            WdfObjectDereference(RequestToCancel);
        }
    }

    //
    // If endpoint is disabled, need to clear any stall.
    //
    if (!TransferContext->Enabled) {
        if (TransferContext->Stalled) {
            CommandStallClear(Endpoint);
        }
    
    //
    // Bring control endpoint back to setup stage.
    //
    } else if (CONTROL_ENDPOINT(Endpoint)) {
        TransferSetupPacket(Endpoint);

    //
    // Otherwise, we can check if there's another request.
    //
    } else if (FetchNextRequest) {
        NewTransaction = TransferNextRequest(Endpoint);
    }

    TraceExit();

    return NewTransaction;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferProgram (
    UFXENDPOINT Endpoint,
    WDFDMATRANSACTION Transaction
    )
/*++
Routine Description:

    DMA callback for transfers that use DMA. This function may be called
    multiple times for a single transfer request.

Parameters Description:

    Endpoint - The endpoint for the transfer.

    Transaction - The DMA transaction object associated with the request.

Return Value:

    TRUE if nothing went wrong, FALSE otherwise.

--*/
{
    PTRANSFER_CONTEXT TransferContext;
    PDMA_CONTEXT DmaContext;
    PUFXENDPOINT_CONTEXT EpContext;

    TraceEntry();
    
    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    EpContext = UfxEndpointGetContext(Endpoint);
    DmaContext = DmaGetContext(Transaction);

    DmaContext->BytesProgrammedCurrent = 0;
    DmaContext->BytesRemaining = 0;

    TRACE_TRANSFER("PROGRAMMING", Endpoint, DmaContext->Request);

    //
    // Try to program transfer structures
    //
    while (DmaContext->SgIndex < DmaContext->SgList->NumberOfElements) {
        PSCATTER_GATHER_ELEMENT Sg;
        BOOLEAN NoMoreSgs;
        BOOLEAN DmaComplete;
        BOOLEAN LastSgInDma;

        Sg = &DmaContext->SgList->Elements[DmaContext->SgIndex];

        NoMoreSgs = (DmaContext->SgIndex == DmaContext->SgList->NumberOfElements - 1);
        LastSgInDma = ((DmaContext->BytesProgrammed + Sg->Length) == DmaContext->BytesRequested);

        //
        // #### TODO: Insert code to map scatter gather buffers to controller transfer structures ####
        //
        
        //
        // Need to remember how much we really programmed
        //
        DmaContext->BytesProgrammed += Sg->Length;
        DmaContext->BytesProgrammedCurrent += Sg->Length;
        DmaComplete = (DmaContext->BytesProgrammed == DmaContext->BytesRequested);

        //
        // Advance to next SG
        //
        DmaContext->SgIndex += 1;
    }

    //
    // Append IN ZLP or extra OUT buffer if needed.
    //
    if (DmaContext->BytesProgrammed == DmaContext->BytesRequested &&
        DmaContext->NeedExtraBuffer) {

        TRACE_TRANSFER("EXTRA", Endpoint, DmaContext->Request);

        //
        // #### TODO: Insert code to append an extra buffer to the transfer structures #### 
        //
        
    }

    //
    // Update the BytesRemaining to reflect all the programmed bytes
    //
    DmaContext->BytesRemaining = DmaContext->BytesProgrammedCurrent;

    //
    // Finally, send the command to start or continue this transfer.
    //
    TransferCommandStartOrUpdate(Endpoint);

    TRACE_TRANSFER("PROGRAMMING--", Endpoint, DmaContext->Request);

    TraceExit();
}

BOOLEAN
OnEvtProgramDma (
    _In_ WDFDMATRANSACTION Transaction,
    _In_ WDFDEVICE Device,
    _In_ WDFCONTEXT Context,
    _In_ WDF_DMA_DIRECTION Direction,
    _In_ PSCATTER_GATHER_LIST SgList
    )
/*++
Routine Description:

    WDF EvtProgramDma event callback function

Parameters Description:

    Transaction - The DMA transaction object associated with the request.

    Device - The device.

    Context - The UFXENDPOINT associated with the request.

    Direction - The direction of the transfer.

    SgList - An array of logical pointers to chunks of the data buffer.

--*/
{
    NTSTATUS Status;
    UFXENDPOINT Endpoint;
    PDMA_CONTEXT DmaContext;
    WDFDMATRANSACTION NewTransaction;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Direction);

    TraceEntry();

    Endpoint = (UFXENDPOINT) Context;
    DmaContext = DmaGetContext(Transaction);

    TransferLock(Endpoint);

    DmaContext->State = Executed;
    NewTransaction = NULL;

    //
    // EvtRequestCancel was called, but couldn't cancel the
    // request becuase this function needed to run.
    //
    if (DmaContext->CleanupOnProgramDma) {
        NewTransaction = TransferCancelCleanup(Endpoint);
        goto End;
    }

    //
    // Don't program cancelled requests.
    //
    Status = WdfRequestUnmarkCancelable(DmaContext->Request);
    CHK_NT_MSG(Status, "WdfRequestUnmarkCancelable failed during programming");
    
    Status = WdfRequestMarkCancelableEx(DmaContext->Request, OnEvtRequestCancel);
    if (Status == STATUS_CANCELLED) {
        LOG_NT_MSG(Status, "Request cancelled during programming");
        NewTransaction = TransferCancelCleanup(Endpoint);
        goto End;

    } else {
        CHK_NT_MSG(Status, "Failed to mark request cancelable");
    }

    DmaContext->SgList = SgList;
    DmaContext->SgIndex = 0;
    TransferProgram(Endpoint, Transaction);
    Transaction = NULL;

End:
    TransferUnlock(Endpoint);
    TransferDmaExecute(NewTransaction);
    TraceExit();
    return TRUE;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandEndComplete (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Handles endpoint command complete event for the End Transfer commmand.

Parameters Description:

    Endpoint - The endpoint that received the event.

--*/
{
    PTRANSFER_CONTEXT TransferContext;
    WDFDMATRANSACTION Transaction;

    TraceEntry();

    TransferLock(Endpoint);

    TransferContext = UfxEndpointGetTransferContext(Endpoint);

    TransferContext->TransferStarted = FALSE;
    TransferContext->EndRequested = FALSE;

    Transaction = NULL;
    if (TransferContext->CleanupOnEndComplete) {
        TransferContext->CleanupOnEndComplete = FALSE;
        Transaction = TransferCancelCleanup(Endpoint);
    }
    
    TransferUnlock(Endpoint);
    TransferDmaExecute(Transaction);
    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandStartComplete (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Handles endpoint command complete event for the Start Transfer commmand.

Parameters Description:

    Endpoint - The endpoint that received the event.

--*/
{
    PTRANSFER_CONTEXT TransferContext;
    WDFDMATRANSACTION Transaction;
    ULONG CommandStatus;

    TraceEntry();

    TransferLock(Endpoint);

    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    Transaction = NULL;

    //
    // #### TODO: Insert code to determine status of command ####
    //
    
    // sample will assume no error for illustration purposes
    CommandStatus = 0;
    
    //
    // Command start has failed. Clean up the request.
    //
    if (CommandStatus != 0) {
        TRACE_TRANSFER("START FAIL", Endpoint, NULL);
    
        TransferContext->TransferStarted = FALSE;
    
        if (CONTROL_ENDPOINT(Endpoint)) {
            PCONTROL_CONTEXT ControlContext;
            
            ControlContext = UfxEndpointGetControlContext(Endpoint);
       
            if (ControlContext->SetupRequested) {
                //
                // Error on a setup request.  Wait for host to reset us or user reconnect.
                //
                TraceError("ERROR: Failed a setup packet!");
                NT_ASSERT(FALSE);
                
            } else {
                CommandStallSet(Endpoint);
            }

        } else {
            Transaction = TransferRequestTryUnmarkCancelableAndCleanup(Endpoint);
        }
    
    } else {
        TransferContext->TransferCommandStartComplete = TRUE;
    }

    TransferUnlock(Endpoint);
    TransferDmaExecute(Transaction);
    TraceExit();
}

VOID
TransferCompleteNext (
    WDFWORKITEM WorkItem
)
/*++
Routine Description:

    Work item function that handles completion of a transfer

Parameters Description:

    WorkItem - Work item that was queued to execute this function.

--*/
{
    NTSTATUS Status;
    WDFDMATRANSACTION Transaction;
    PTRANSFER_CONTEXT TransferContext;
    PDMA_CONTEXT DmaContext;
    UFXENDPOINT Endpoint;

    TraceEntry();

    Endpoint = WdfWorkItemGetParentObject(WorkItem);
    TransferContext = UfxEndpointGetTransferContext(Endpoint);

    TransferLock(Endpoint);

    //
    // See if the request was completed by the cancellation code
    // before this work-item executed.
    //
    if (TransferContext->PendingCompletion == FALSE) {
        Transaction = NULL;
        goto End;
    }

    Transaction = TransferContext->Transaction;
    
    //
    // Make sure request wasn't cancelled before work item got to run.
    //
    if (Transaction == NULL) {
        goto End;
    }

    DmaContext = DmaGetContext(Transaction);

    if (DmaContext->State == Executed) {
        DmaContext->State = Completed;
        (void) WdfDmaTransactionDmaCompletedFinal(Transaction, 0, &Status);
    }

    Status = WdfRequestUnmarkCancelable(DmaContext->Request);
    if (NT_SUCCESS(Status)) {
        ULONG BytesTransferred;
        TRACE_TRANSFER("COMPLETE", Endpoint, DmaContext->Request);

        BytesTransferred = DmaContext->BytesProgrammed - DmaContext->BytesRemaining;
        if (BytesTransferred > DmaContext->BytesRequested) {
            TraceWarning("Transferred more than what is asked for: "
                        "Bytes Requested: %d, Bytes Transferred:%d", 
                        DmaContext->BytesRequested, BytesTransferred);
            BytesTransferred = DmaContext->BytesRequested;
        }

        TransferContext->PendingCompletion = FALSE;

        WdfRequestCompleteWithInformation(
            DmaContext->Request,
            Status,
            BytesTransferred);
        WdfObjectDelete(Transaction);
        TransferContext->Transaction = NULL;
    } else {
        Transaction = NULL;
        CHK_NT_MSG(Status, "Failed to unmark status cancellable");
    }

    TransferContext->TransferStarted = FALSE;
    Transaction = TransferNextRequest(Endpoint);

End:    
    TransferUnlock(Endpoint);

    if (Transaction != NULL) {
        TransferDmaExecute(Transaction);
    }

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCompleteData (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Handles completion of a data transfer for an endpoint.

Parameters Description:

    Endpoint - The endpoint that completed the data transfer.

--*/
{
    PTRANSFER_CONTEXT TransferContext;
    ULONG BytesRemaining;
    WDFDMATRANSACTION Transaction;
    PDMA_CONTEXT DmaContext;
    WDFDMATRANSACTION NewTransaction;
    BOOLEAN TransferComplete;

    TraceEntry();

    TransferLock(Endpoint);

    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    BytesRemaining = 0;
    NewTransaction = NULL;

    //
    // Make sure request wasn't cancelled.
    //
    Transaction = TransferContext->Transaction;
    if (Transaction == NULL) {
        TRACE_TRANSFER("COMPLETION SKIPPED - Cancelled", Endpoint, NULL);
        goto End;
    }

    DmaContext = DmaGetContext(Transaction);

    //
    // #### TODO: Insert code to determine if the transfer is complete, or terminated due to a short packet. ####

    // Sample will assume transfer is complete
    TransferComplete = TRUE;
    
    //
    // If the transfer is complete we need to complete the request.  
    //
    if (TransferComplete) {
        DmaContext->BytesRemaining = BytesRemaining;
        TRACE_TRANSFER("COMPLETE (Last packet or short packet)", Endpoint, NULL);
        TransferContext->PendingCompletion = TRUE;
        WdfWorkItemEnqueue(TransferContext->CompletionWorkItem);
    
    //
    // If transfer is still in progress, we need to program more transfers
    //
    } else {
        NTSTATUS Status;
        ULONG BytesTransferred;

        TRACE_TRANSFER("COMPLETE (In Progress)", Endpoint, NULL);

        DmaContext->State = NotExecuted;

        BytesTransferred = DmaContext->BytesProgrammedCurrent - BytesRemaining;

        TransferUnlock(Endpoint);
        WdfDmaTransactionDmaCompletedWithLength(Transaction, BytesTransferred, &Status);
        TransferLock(Endpoint);

        NT_ASSERT(!NT_SUCCESS(Status));

        if (Status != STATUS_MORE_PROCESSING_REQUIRED && !NT_SUCCESS(Status)) {
            NewTransaction = TransferRequestTryUnmarkCancelableAndCleanup(Endpoint);
        }   
    }

End:
    TransferUnlock(Endpoint);
    TransferDmaExecute(NewTransaction);
    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCompleteControl (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Handles completion control transfer stage.

Parameters Description:

    Endpoint - The control endpoint that completed the control transfer stage.

--*/
{
    PTRANSFER_CONTEXT TransferContext;
    PCONTROL_CONTEXT ControlContext;

    TraceEntry();

    TransferLock(Endpoint);

    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    ControlContext = UfxEndpointGetControlContext(Endpoint);

    NT_ASSERT(!(ControlContext->SetupRequested && ControlContext->HandshakeRequested));

    //
    // Handle setup packet completion
    //
    if (ControlContext->SetupRequested) {
        TRACE_TRANSFER("COMPLETE (Setup)", Endpoint, NULL);
        
        ControlContext->SetupRequested = FALSE;
        TransferContext->TransferStarted = FALSE;

        UfxEndpointNotifySetup(Endpoint, ControlContext->SetupPacket);

    //
    // Handle control status handshake completion
    //
    } else if (ControlContext->HandshakeRequested) {
        NTSTATUS Status;

        TRACE_TRANSFER("COMPLETE (Handshake)", Endpoint, ControlContext->HandshakeRequest);
        
        Status = WdfRequestUnmarkCancelable(ControlContext->HandshakeRequest);
        if (Status != STATUS_CANCELLED) {
            WdfRequestComplete(ControlContext->HandshakeRequest, Status);
        } else {
            CHK_NT_MSG(Status, "Failed to unmark cancelable");
        }

        ControlContext->HandshakeRequested = FALSE;
        ControlContext->HandshakeRequest = NULL;
        TransferContext->TransferStarted = FALSE;

        TransferSetupPacket(Endpoint);

    //
    // Handle data stage
    //
    } else if (ControlContext->DataStageExists) {
        TransferUnlock(Endpoint);
        TransferCompleteData(Endpoint);
        TransferLock(Endpoint);

    } else {
        TraceWarning("Unexpected transfer complete on control endpoint!");
        NT_ASSERT(FALSE);
    }

End:
    TransferUnlock(Endpoint);
    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferComplete (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Handles endpoint XferComplete or XferInProgress events.

Parameters Description:

    Endpoint - The endpoint that received the event.

    Event - The event received.

--*/
{
    TraceEntry();

    if (CONTROL_ENDPOINT(Endpoint)) {
        TransferCompleteControl(Endpoint);

    } else {
        TransferCompleteData(Endpoint);
    }

    TraceExit();
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
TransferDestroy (
    _In_ UFXENDPOINT Endpoint
    )

/*++
Routine Description:

    Cleans up the endpoint's TRANSFER_CONTEXT during endpoint destroy

Parameters Description:

    Endpoint - Endpoint being destroyed

--*/
{
    PTRANSFER_CONTEXT TransferContext;
    PUFXENDPOINT_CONTEXT EpContext;
    PUFXDEVICE_CONTEXT DeviceContext;
    WDFCOMMONBUFFER ExtraBuffer;
    WDFCOMMONBUFFER SetupPacketBuffer;

    TraceEntry();

    TransferLock(Endpoint);

    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    EpContext = UfxEndpointGetContext(Endpoint);
    DeviceContext = UfxDeviceGetContext(EpContext->UfxDevice);

    DeviceContext->PhysicalEndpointToUfxEndpoint[EpContext->PhysicalEndpoint] = NULL;
    if (CONTROL_ENDPOINT(Endpoint)) {
        DeviceContext->PhysicalEndpointToUfxEndpoint[EpContext->PhysicalEndpoint + 1] = NULL;
    }

    //
    // Need to do this first so another transfer isn't scheduled on cleanup.
    //
    TransferContext->Enabled = FALSE;

    TransferRequestTryUnmarkCancelableAndCleanup(Endpoint);

    ExtraBuffer = TransferContext->ExtraBuffer;
    TransferContext->ExtraBuffer = NULL;

    SetupPacketBuffer = NULL;
    if (CONTROL_ENDPOINT(Endpoint)) {
        PCONTROL_CONTEXT ControlContext;
        
        ControlContext = UfxEndpointGetControlContext(Endpoint);
        SetupPacketBuffer = ControlContext->SetupPacketBuffer;
        ControlContext->SetupPacketBuffer = NULL;
    }

    //
    // Most of these are probably already reset, but let's
    // reset them anyway.
    //
    TransferContext->TransferStarted = FALSE;
    TransferContext->Stalled = FALSE;
    TransferContext->EndRequested = FALSE;
    TransferContext->TransferCommandStartComplete = FALSE;
    TransferContext->CleanupOnEndComplete = FALSE;

    //
    // Cannot delete common buffers with spinlock held.
    //
    TransferUnlock(Endpoint);

    if (ExtraBuffer != NULL) {
        WdfObjectDelete(ExtraBuffer);
    }

    if (SetupPacketBuffer != NULL) {
        WdfObjectDelete(SetupPacketBuffer);
    }

    TraceExit();
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
TransferInitialize (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Sets up all fields in the endpoint's TRANSFER_CONTEXT during
    endpoint creation.

Parameters Description:

    Endpoint - The UFXENDPOINT which may have pending transfers.

Return Value:

    STATUS_SUCCESS if successful, appropriate NTSTATUS message otherwise.

--*/
{
    NTSTATUS Status;
    PCONTROLLER_CONTEXT ControllerContext;
    PUFXENDPOINT_CONTEXT EpContext;
    PTRANSFER_CONTEXT TransferContext;
    PUFXDEVICE_CONTEXT DeviceContext;
    WDF_OBJECT_ATTRIBUTES Attributes;
    ULONG PhysicalEndpoint;
    WDF_COMMON_BUFFER_CONFIG BufferConfig;
    PUCHAR Buffer;
    WDF_WORKITEM_CONFIG WorkItemConfig;

    TraceEntry();

    EpContext = UfxEndpointGetContext(Endpoint);
    ControllerContext = DeviceGetControllerContext(EpContext->WdfDevice);
    DeviceContext = UfxDeviceGetContext(EpContext->UfxDevice);

    //
    // Allocate transfer-related context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, TRANSFER_CONTEXT);
    WdfObjectAllocateContext(Endpoint, &Attributes, (PVOID*) &TransferContext);
    TransferContext->WdfDevice = EpContext->WdfDevice;

    //
    // Create transfer spin lock
    //
    if (TransferContext->TransferLock == NULL) {
        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = Endpoint;
        Status = WdfSpinLockCreate(&Attributes, &TransferContext->TransferLock);
        CHK_NT_MSG(Status, "Failed to create transfer spinlock");
    }

    //
    // Allocate and initialize common buffer for the endpoint
    //
    if (TransferContext->CommonBuffer == NULL) {
        WDF_COMMON_BUFFER_CONFIG_INIT(&BufferConfig, COMMON_BUFFER_ALIGNMENT);
        Status = WdfCommonBufferCreateWithConfig(
                    ControllerContext->DmaEnabler,
                    ENDPOINT_COMMON_BUFFER_SIZE,
                    &BufferConfig,
                    WDF_NO_OBJECT_ATTRIBUTES,
                    &TransferContext->CommonBuffer);
        CHK_NT_MSG(Status, "Failed to create common buffer");
    }

    Buffer = WdfCommonBufferGetAlignedVirtualAddress(TransferContext->CommonBuffer);
    TransferContext->LogicalCommonBuffer = 
        WdfCommonBufferGetAlignedLogicalAddress(TransferContext->CommonBuffer); 
    TransferContext->Buffer = Buffer;

    RtlZeroMemory(TransferContext->CommonBuffer, ENDPOINT_COMMON_BUFFER_SIZE);
    
    //
    // #### TODO: Insert code to initialize controller data structures in the shared common buffer
    //
    
    //
    // Map physical endpoint
    //
    PhysicalEndpoint = (EpContext->Descriptor.bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK) * 2;
    if (DIRECTION_IN(Endpoint) && !CONTROL_ENDPOINT(Endpoint)) {
        PhysicalEndpoint++;
    }

    if (PhysicalEndpoint >= MAX_PHYSICAL_ENDPOINTS) {
        NT_ASSERTMSG("Physical endpoint maxium exceeded", FALSE);
        Status = STATUS_UNSUCCESSFUL;
        CHK_NT_MSG(Status, "Physical endpoint maxium exceeded");
    }

    DeviceContext->PhysicalEndpointToUfxEndpoint[PhysicalEndpoint] = Endpoint;
    EpContext->PhysicalEndpoint = PhysicalEndpoint;

    //
    // Control endpoints have two consecutive physical endpoints and
    // need a place to store setup packets
    //
    if (CONTROL_ENDPOINT(Endpoint)) {
        PCONTROL_CONTEXT ControlContext;

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, CONTROL_CONTEXT);
        WdfObjectAllocateContext(Endpoint, &Attributes, (PVOID*) &ControlContext);

        PhysicalEndpoint++;

        if (PhysicalEndpoint >= MAX_PHYSICAL_ENDPOINTS) {
            NT_ASSERTMSG("Physical endpoint maxium exceeded", FALSE);
            Status = STATUS_UNSUCCESSFUL;
            CHK_NT_MSG(Status, "Physical endpoint maxium exceeded");
        }

        DeviceContext->PhysicalEndpointToUfxEndpoint[PhysicalEndpoint] = Endpoint;

        if (ControlContext->SetupPacketBuffer == NULL) {
            WDF_COMMON_BUFFER_CONFIG_INIT(&BufferConfig, DATA_ALIGNMENT);
            Status = WdfCommonBufferCreateWithConfig(
                ControllerContext->DmaEnabler,
                sizeof(USB_DEFAULT_PIPE_SETUP_PACKET),
                &BufferConfig,
                WDF_NO_OBJECT_ATTRIBUTES,
                &ControlContext->SetupPacketBuffer);
            CHK_NT_MSG(Status, "Failed to create setup packet buffer");
        }

        Buffer = WdfCommonBufferGetAlignedVirtualAddress(ControlContext->SetupPacketBuffer);
        ControlContext->SetupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET) Buffer;
   }

    //
    // Extra buffer for OUT transfers
    //
    if (TransferContext->ExtraBuffer == NULL) {
        WDF_COMMON_BUFFER_CONFIG_INIT(&BufferConfig, DATA_ALIGNMENT);
        Status = WdfCommonBufferCreateWithConfig(
            ControllerContext->DmaEnabler,
            1024,
            &BufferConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &TransferContext->ExtraBuffer);
        CHK_NT_MSG(Status, "Failed to create extra OUT buffer");
    }

    if (TransferContext->CompletionWorkItem == NULL) {
        WDF_WORKITEM_CONFIG_INIT(&WorkItemConfig, TransferCompleteNext);
        Attributes.ParentObject = Endpoint;
        Status = WdfWorkItemCreate(&WorkItemConfig, &Attributes, &TransferContext->CompletionWorkItem);
        CHK_NT_MSG(Status, "Failed to create completion work item");
    }

    Status = STATUS_SUCCESS;

End:
    TraceExit();
    return Status;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFDMATRANSACTION
TransferBegin (
    _In_ UFXENDPOINT Endpoint,
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN DirectionIn,
    _In_ BOOLEAN AppendZlp
    )
/*++
Routine Description:

    Starts a DMA transfer on the endpoint.

Parameters Description:

    Endpoint - The endpoint to transfer on.

    Request - The WDFREQUEST which requested this transfer.

    DirectionIn - Transfer to host.

    AppendZlp - Append zero-length packet at the end of the transfer.
                Only valid for DirectionIn.

--*/
{
    NTSTATUS Status;
    PCONTROLLER_CONTEXT ControllerContext;
    PUFXENDPOINT_CONTEXT EpContext;
    PTRANSFER_CONTEXT TransferContext;
    WDF_OBJECT_ATTRIBUTES Attributes;
    PDMA_CONTEXT DmaContext;
    PREQUEST_CONTEXT RequestContext;
    WDFDMATRANSACTION Transaction;
    WDF_REQUEST_PARAMETERS Params;

    TraceEntry();

    EpContext = UfxEndpointGetContext(Endpoint);
    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    ControllerContext = DeviceGetControllerContext(EpContext->WdfDevice);

    NT_ASSERT(DirectionIn || !AppendZlp);

    TransferContext->PendingCompletion = FALSE;

    //
    // Create a DMA Transaction over the data
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, DMA_CONTEXT);
    Status = WdfDmaTransactionCreate(ControllerContext->DmaEnabler,
                                     &Attributes,
                                     &Transaction);
    CHK_NT_MSG(Status, "Failed to create DMA Transaction");

    //
    // Set up DMA context
    //
    WDF_REQUEST_PARAMETERS_INIT(&Params);
    WdfRequestGetParameters(Request, &Params);
    DmaContext = DmaGetContext(Transaction);
    DmaContext->NeedExtraBuffer = AppendZlp;
    DmaContext->DirectionIn = DirectionIn;
    DmaContext->Endpoint = Endpoint;
    DmaContext->Request = Request;
    DmaContext->BytesRequested = (ULONG) Params.Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Need to add extra buffer if OUT transfer not multiple of MaxPacketSize
    //
    if (!DirectionIn) {
        if ((DmaContext->BytesRequested % EpContext->Descriptor.wMaxPacketSize) > 0) {
            DmaContext->ExtraBytes = EpContext->Descriptor.wMaxPacketSize -
                (DmaContext->BytesRequested % EpContext->Descriptor.wMaxPacketSize);
            DmaContext->NeedExtraBuffer = TRUE;
        }
    }

    DmaContext->State = NotExecuted;

    //
    // Set up request context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, REQUEST_CONTEXT);
    Status = WdfObjectAllocateContext(Request, &Attributes, (PVOID*) &RequestContext);
    CHK_NT_MSG(Status, "Failed to allocate request context");

    //
    // It's possible that the request was reused, in which case the context already
    // exists. Therefore, we need to make sure to zero it out.
    //
    RtlZeroMemory(RequestContext, sizeof(REQUEST_CONTEXT));

    RequestContext->Endpoint = Endpoint;
    RequestContext->Transaction = Transaction;

    //
    // Set up control data stage context
    //
    if (CONTROL_ENDPOINT(Endpoint)) {
        PCONTROL_CONTEXT ControlContext;
        ControlContext = UfxEndpointGetControlContext(Endpoint);
        ControlContext->DataStageExists = TRUE;
    }
 
    //
    // Keep track of DMA
    //
    TransferContext->Transaction = Transaction;

    //
    // We need to mark cancelable before starting DMA (which may take time)
    //
    Status = WdfRequestMarkCancelableEx(Request, OnEvtRequestCancel);
    CHK_NT_MSG(Status, "Failed to mark request cancellable");

    TRACE_TRANSFER("BEGIN", Endpoint, Request);

    //
    // Can't use a DMA transaction for 0-length transfers.
    //
    if (DmaContext->BytesRequested == 0) {

        DmaContext->ZeroLength = TRUE;
        
        TRACE_TRANSFER("ZERO LENGTH", Endpoint, DmaContext->Request);

        DmaContext->ExtraBytes = 0;

        TransferCommandStartOrUpdate(Endpoint);

    } else {
        //
        // Use request to initialize DMA
        //
        Status = WdfDmaTransactionInitializeUsingRequest(
                    Transaction,
                    Request,
                    OnEvtProgramDma,
                    DirectionIn ? WdfDmaDirectionWriteToDevice : WdfDmaDirectionReadFromDevice);
        CHK_NT_MSG(Status, "Failed to initialize DMA transaction using request");

        //
        // Caller is expected to execute the DMA.
        //
    } 

End:
    if (!NT_SUCCESS(Status)) {
        WdfRequestComplete(Request, Status);
        if (Transaction != NULL) {
            TransferContext->Transaction = NULL;
            WdfObjectDelete(Transaction);
            Transaction = NULL;
        }
    }
        
    TraceExit();
    return Transaction;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferHandshake (
    _In_ UFXENDPOINT Endpoint,
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN DirectionIn
    )
/*++
Routine Description:

    Starts a control handshake (zero-length packet) transfer on the endpoint.
    The caller is expected to acquire the TransferLock.

Parameters Description:

    Endpoint - The endpoint to transfer on.

    Request - The WDFREQUEST which requested this transfer.

    DirectionIn - Transfer to host.

--*/
{
    NTSTATUS Status;
    PCONTROL_CONTEXT ControlContext;
    PREQUEST_CONTEXT RequestContext;
    WDF_OBJECT_ATTRIBUTES Attributes;

    TraceEntry();

    UNREFERENCED_PARAMETER(DirectionIn);

    ControlContext = UfxEndpointGetControlContext(Endpoint);

    NT_ASSERT(CONTROL_ENDPOINT(Endpoint));
    NT_ASSERT(ControlContext->SetupRequested == FALSE);
    NT_ASSERT(ControlContext->HandshakeRequested == FALSE);
    NT_ASSERT(ControlContext->HandshakeRequest == Request ||
              ControlContext->HandshakeRequest == NULL);

    Status = STATUS_SUCCESS;

    if (ControlContext->HandshakeRequest != Request) {
        //
        // Set up request context
        //
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, REQUEST_CONTEXT);
        Status = WdfObjectAllocateContext(Request, &Attributes, (PVOID*) &RequestContext);
        CHK_NT_MSG(Status, "Failed to allocate request context");

        //
        // It's possible that the request was reused, in which case the context already
        // exists. Therefore, we need to make sure to zero it out.
        //
        RtlZeroMemory(RequestContext, sizeof(REQUEST_CONTEXT));

        RequestContext->Endpoint = Endpoint;

        //
        // Mark request cancellable, in case host takes a long time.
        //
        Status = WdfRequestMarkCancelableEx(Request, OnEvtRequestCancel);
        CHK_NT_MSG(Status, "Failed to mark quest cancelable");
    }

    ControlContext->HandshakeRequest = Request;

    //
    // #### TODO: Insert code to initialize endpoint data structures for handshake ####
    //

    TRACE_TRANSFER("HANDSHAKE", Endpoint, Request);

    //
    // We should always start a new transfer for this, since it must be in
    // the opposite direction of the DMA or Setup Packet.
    //
    TransferCommandStart(Endpoint);
    ControlContext->HandshakeRequested = TRUE;

End:
    if (!NT_SUCCESS(Status)) {
        WdfRequestComplete(Request, Status);
    }
    TraceExit();
}


_IRQL_requires_max_(DISPATCH_LEVEL)
WDFDMATRANSACTION
TransferNextRequest (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Handles IOCTL requests on the endpoint.

Parameters Description:

    Endpoint - Endpoint with request.

--*/
{
    NTSTATUS Status;
    WDFREQUEST Request;
    WDF_REQUEST_PARAMETERS Params;
    ULONG Ioctl;
    PUFXENDPOINT_CONTEXT EpContext;
    WDFDMATRANSACTION Transaction;

    TraceEntry();

    EpContext = UfxEndpointGetContext(Endpoint);
    Transaction = NULL;

Fetch:

    //
    // Fetch next request
    //
    Status = WdfIoQueueRetrieveNextRequest(UfxEndpointGetTransferQueue(Endpoint),
                                           &Request);
    if (Status == STATUS_NO_MORE_ENTRIES || Status == STATUS_WDF_PAUSED) {
        goto End;
    }

    CHK_NT_MSG(Status, "Failed to retrieve next request");
    
    WDF_REQUEST_PARAMETERS_INIT(&Params);
    WdfRequestGetParameters(Request, &Params);
    Ioctl = Params.Parameters.DeviceIoControl.IoControlCode;

    if (Ioctl == IOCTL_INTERNAL_USBFN_CONTROL_STATUS_HANDSHAKE_IN) {
        TransferHandshake(Endpoint, Request, TRUE);
    
    } else if (Ioctl == IOCTL_INTERNAL_USBFN_CONTROL_STATUS_HANDSHAKE_OUT) {
        TransferHandshake(Endpoint, Request, FALSE);
    
    } else if (DIRECTION_IN(Endpoint) &&
               Ioctl == IOCTL_INTERNAL_USBFN_TRANSFER_IN) {

        Transaction = TransferBegin(Endpoint, Request, TRUE, FALSE);

    } else if (DIRECTION_IN(Endpoint) && 
               Ioctl == IOCTL_INTERNAL_USBFN_TRANSFER_IN_APPEND_ZERO_PKT) {

        Transaction = TransferBegin(Endpoint, Request, TRUE, TRUE);

    } else if (DIRECTION_OUT(Endpoint) &&
               Ioctl == IOCTL_INTERNAL_USBFN_TRANSFER_OUT) {

        Transaction = TransferBegin(Endpoint, Request, FALSE, FALSE);

    } else {
        TraceWarning("INVALID: %08X (%d), Ioctl: %08X",
            (ULONG) Endpoint, EpContext->PhysicalEndpoint, Ioctl);
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
        goto Fetch;
    }

End:
    TraceExit();

    return Transaction;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferReset (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Disables and ends an endpoint. On end complete, if the endpoint
    is stalled, it will be cleared. After end complete, the endpoint
    can be re-configured.

Parameters Description:

    Endpoint - endpoint to reset

--*/
{
    PUFXENDPOINT_CONTEXT EpContext;
    PTRANSFER_CONTEXT TransferContext;

    TraceEntry();

    EpContext = UfxEndpointGetContext(Endpoint);
    TransferContext = UfxEndpointGetTransferContext(Endpoint);
     
    TransferLock(Endpoint);

    TRACE_TRANSFER("RESET", Endpoint, NULL);

    //
    // Need to get EP0 back to "setup" stage.
    //
    if (EpContext->PhysicalEndpoint <= 1) {
        PCONTROL_CONTEXT ControlContext;

        ControlContext = UfxEndpointGetControlContext(Endpoint);

        if (!ControlContext->SetupRequested) {
            if (TransferContext->TransferStarted) {
                TransferCommandEnd(Endpoint);
            } else {
                CommandStallSet(Endpoint);
            }
        }

    } else {
        TransferContext->Enabled = FALSE;
        if (TransferContext->TransferStarted) {
            TransferCommandEnd(Endpoint);
        }
    }
    
    TransferUnlock(Endpoint);
    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferStart (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Enables an endpoint to start transfers and starts fetching
    requests. If this is a control endpoint, programs a setup
    packet.

Parameters Description:

    Endpoint - endpoint to start

--*/
{
    PTRANSFER_CONTEXT TransferContext;
    WDFDMATRANSACTION Transaction;

    TraceEntry();

    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    Transaction = NULL;

    TransferLock(Endpoint);

    TRACE_TRANSFER("START", Endpoint, NULL);

    TransferContext->Enabled = TRUE;

    //
    // EP0 is programmed once on creation, not on each reset.
    //
    if (CONTROL_ENDPOINT(Endpoint)) {
        TransferSetupPacket(Endpoint);

    } else {
        Transaction = TransferNextRequest(Endpoint);
    }

    TransferUnlock(Endpoint);
    TransferDmaExecute(Transaction);
    TraceExit();
}

VOID
TransferReadyNotify (
    WDFQUEUE Queue,
    WDFCONTEXT Context
    )
/*++
Routine Description:

    Indicates the number of events in the queue went from 0 -> 1.
    This will fetch the next request and program it.

Parameters Description:

    Queue - queue with request.

    Context - UFXENDPOINT

--*/
{
    UFXENDPOINT Endpoint;
    PTRANSFER_CONTEXT TransferContext;
    WDFDMATRANSACTION Transaction;

    UNREFERENCED_PARAMETER(Queue);

    TraceEntry();

    Endpoint = (UFXENDPOINT) Context;
    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    Transaction = NULL;

    //
    // Make sure we didn't get hijacked from WdfRequestComplete...
    //
    if (TransferContext->TransferStarted) {
        goto Skip;
    }

    TransferLock(Endpoint);

    if (TransferContext->Enabled && TransferContext->Transaction == NULL) {
        if (CONTROL_ENDPOINT(Endpoint)) {
            PCONTROL_CONTEXT ControlContext;
            ControlContext = UfxEndpointGetControlContext(Endpoint);

            if (!ControlContext->SetupRequested) {
                Transaction = TransferNextRequest(Endpoint);
            }

        } else {
            Transaction = TransferNextRequest(Endpoint);
        }
    }

    TransferUnlock(Endpoint);
    TransferDmaExecute(Transaction);
Skip:
    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferStallSetComplete (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Handles command completion event for Stall Set. If this is a
    control endpoint, programs a setup packet.

Parameters Description:

    Endpoint - Endpoint associated with event.

--*/
{
    PUFXENDPOINT_CONTEXT EpContext;
    PTRANSFER_CONTEXT TransferContext;
    WDFREQUEST Request;
    WDFDMATRANSACTION NewTransaction;

    TraceEntry();

    EpContext = UfxEndpointGetContext(Endpoint);
    TransferContext = UfxEndpointGetTransferContext(Endpoint);
    NewTransaction = NULL;

    TransferLock(Endpoint);
    
    //
    // If we stall a control endpoint, we need to reset its state and start
    // over from the setup packet request.
    //
    if (CONTROL_ENDPOINT(Endpoint)) {
        TransferContext->Stalled = FALSE;
        if (!TransferContext->TransferStarted) {
            NewTransaction = TransferRequestTryUnmarkCancelableAndCleanup(Endpoint);
        }
    }
    
    if (EpContext->StallRequest) {
        Request = EpContext->StallRequest;
        EpContext->StallRequest = NULL;
        WdfRequestComplete(Request, STATUS_SUCCESS);
    }

    TransferUnlock(Endpoint);
    TransferDmaExecute(NewTransaction);
    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferStallClearComplete (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Handles command completion event for Stall Clear.

Parameters Description:

    Endpoint - Endpoint associated with event.

--*/
{
    PUFXENDPOINT_CONTEXT EpContext;
    WDFREQUEST Request;

    TraceEntry();

    EpContext = UfxEndpointGetContext(Endpoint);

    TransferLock(Endpoint);
    
    NT_ASSERT(!CONTROL_ENDPOINT(Endpoint));
    
    if (EpContext->ClearRequest) {
        Request = EpContext->ClearRequest;
        EpContext->ClearRequest = NULL;
        WdfRequestComplete(Request, STATUS_SUCCESS);
    }

    TransferUnlock(Endpoint);

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandStallSet (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Sets an endpoint to the STALL state.

Parameters Description:

    Endpoint - Endpoint to set the STALL state on.

--*/
{
    TraceEntry();

    TransferLock(Endpoint);

    CommandStallSet(Endpoint);

    TransferUnlock(Endpoint);

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferCommandStallClear (
    _In_ UFXENDPOINT Endpoint
    )
/*++
Routine Description:

    Clears an endpoint to the STALL state.

Parameters Description:

    Endpoint - Endpoint to clear the STALL state on.

--*/
{
    TraceEntry();

    TransferLock(Endpoint);

    CommandStallClear(Endpoint);

    TransferUnlock(Endpoint);

    TraceExit();
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
TransferGetStall (
    _In_ UFXENDPOINT Endpoint,
    _Out_ PBOOLEAN Stalled
    )
/*++
Routine Description:

    Returns the current STALL state for an endpoint.

Parameters Description:

    Endpoint - Endpoint to retreive the STALL state for.

    Stalled - Pointer to a boolean value to receive the STALL state.

--*/
{
    PTRANSFER_CONTEXT TransferContext;

    TraceEntry();

    TransferContext = UfxEndpointGetTransferContext(Endpoint);

    TransferLock(Endpoint);

    *Stalled = TransferContext->Stalled;

    TransferUnlock(Endpoint);

    TraceExit();
}