/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

   IO.c

Abstract:

    This module contains routines that perform read/write IO operations.

Environment:

    Kernel mode only

Revision History:

--*/

#include "driver.h"
#include "IO.tmh"

#pragma warning(disable:4127) // conditional expression is constant

#ifdef ALLOC_PRAGMA
#endif

VOID
CB_RequestFromBthportCancel(
    _In_ WDFREQUEST _RequestFromUpper
    )
/*++

Routine Description:

    Request from upper layer that driver owns is being canceled.  Its associated
    Request to lower (UART) driver will be canceled and then this Request will
    be completed with STATUS_CANCELLED.

    There are different paths for the Request from upper layer:

    1. Completion routine is invoked without cancellation (typical path)
    2. Cancellation routine is invoked while lower Request is pending.  The lower
        request could be completed either
        a. Synchronously - completion routine is invoked before
            WdfRequestCancelSentRequest() is returned in the cancellation routine; or
        b. Asynchronously - completion routine is invoked at later time after
            WdfRequestCancelSentRequest has returned.
    3. Race conditions when both the cancelation and completion routine have independently started
        a. Cancellation routine is ahead and the request is completed with cancellation status.
        b. Completion routine is ahead and the request is completed with the status from the lower request.

Arguments:

    _RequestFromUpper - WDF Request to be cancelled

Return Value:

    none

--*/
{
    PUART_WRITE_CONTEXT TransferContext;
    WDFREQUEST  RequestToUART;
    WDFMEMORY   Memory;
    BOOLEAN CancelSuccess;
    LONG CompletePath = REQUEST_PATH_NONE;

    DoTrace(LEVEL_WARNING, TFLAG_IO, ("+CB_RequestFromBthportCancel: Request(%p) from upper driver", _RequestFromUpper));

    TransferContext = GetWriteRequestContext(_RequestFromUpper);
    NT_ASSERT(TransferContext && L"TransferContext is not valid!");

    // Cancel the write Request that was previously submitted to its I/O target
    RequestToUART = TransferContext->RequestToUART;
    Memory = TransferContext->Memory;

    //
    // The below operation can return one of the following values.
    // REQUEST_PATH_NONE
    //      This value was returned due to one of the following conditions
    //      1. The completion routine was not yet run.
    //      2. The completion routine was run and it relinquished the control of completing the request from bthport to the cancel routine.
    //
    //      No matter what causes this value to be returned, this function is now responsible for completing the request from bthport.
    //
    // REQUEST_PATH_COMPLETION
    //      The completion routine was already called.
    //      The completion routine has not yet had a chance to relinquish control of completing the request from bthport.
    //
    //      This function does not have the control to complete the request from bthport.
    //
    CompletePath = InterlockedOr(&TransferContext->RequestCompletePath, REQUEST_PATH_CANCELLATION);

    if (REQUEST_PATH_NONE == CompletePath) {

        DoTrace(LEVEL_WARNING, TFLAG_IO, ("  >CancelSentRequest(%p) to IO Target", RequestToUART));
        CancelSuccess = WdfRequestCancelSentRequest(RequestToUART);
        DoTrace(LEVEL_WARNING, TFLAG_IO, ("  <CancelSentRequest: %S", CancelSuccess ? L"Cancelled" : L"Failed"));

        // Done sending the cancel.  It can be dereferenced.
        WdfObjectDereference(RequestToUART);

        // No need to access this memory object in the cancellation code path in the completion function.
        WdfObjectDelete(Memory);

        // Cannot access this request, including WdfRequestUnmarkCancelable(), after it has been completed.
        WdfRequestComplete(_RequestFromUpper, STATUS_CANCELLED);
    }

}

NTSTATUS
HLP_AllocateResourceForWrite(
    _In_  WDFDEVICE   _Device,
    _In_  WDFIOTARGET _IoTargetSerial,
    _Out_ WDFREQUEST *_PRequest
    )
/*++

Routine Description:

    This helper function allocate resource to perform a write request

Arguments:

    _Device - WDF Device object

    _IoTargetSerial - WDF IO Target

    _PRequest - WDF Request to allocate in this function

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status;
    WDF_OBJECT_ATTRIBUTES ObjAttributes;

    DoTrace(LEVEL_INFO, TFLAG_IO,("+HLP_AllocateResourceForWrite"));

    // Create a WDF Request that will allocate a context(UART_WRITE_CONTEXT)
    WDF_OBJECT_ATTRIBUTES_INIT(&ObjAttributes);
    ObjAttributes.ParentObject = _Device;

    Status = WdfRequestCreate(&ObjAttributes,
                              _IoTargetSerial,
                              _PRequest);

    if (!NT_SUCCESS(Status))
    {
        DoTrace(LEVEL_ERROR, TFLAG_IO, (" WdfRequestCreate() failed %!STATUS!", Status));
        goto Done;
    }

Done:

    return Status;
}

VOID
HLP_FreeResourceForWrite(
    PUART_WRITE_CONTEXT _TransferContext
    )
/*++

Routine Description:

    This helper function free resource allocated to perform a write request

Arguments:

    _TransferContext - Transfer context used to perform write operation

Return Value:

    none

--*/
{
    DoTrace(LEVEL_INFO, TFLAG_IO,("+HLP_FreeResourceForWrite"));

    if (_TransferContext)
    {
        if (_TransferContext->Memory)
        {
           WdfObjectDelete(_TransferContext->Memory);
           _TransferContext->Memory = NULL;
        }

        if (_TransferContext->RequestToUART)
        {
            WdfObjectDelete(_TransferContext->RequestToUART);
            _TransferContext->RequestToUART = NULL;

        }
    }
}

VOID
CR_WriteDeviceIO(
    _In_  WDFREQUEST  _Request,
    _In_  WDFIOTARGET  _Target,
    _In_  PWDF_REQUEST_COMPLETION_PARAMS  _Params,
    _In_  WDFCONTEXT  _Context
    )
/*++

Routine Description:

    This is the completion function for sending HCI packet to the lower layer.
    This function can also complete the request from the upper layer; see the
    description in the cancellation function for detail on the handling of possible
    race conditions.

    A RequestCompletionPath flag in the write Context  is used with atomic Interlocked function
    to ensure deterministic operation in both the cancellation and this completion functions.

    If the cancellation function has been called, the WdfRequestUnmarkCancelable in the completion function will return STATUS_CANCELLED.
    This return code is used to determine to handle the processing either as a typical completion, or as a cancellation and be in sync
    with the cancellation function.

    Here are what are performed in either situations:

     1. Typical completion (completion function only)
            - WdfRequestUnmarkCancelable() returns not STATUS_CANCELLED
            Exercise its typical completion code path
            - Retrieve data transfer information for success case
            - Dereference(RequestUART) - will not be accessed by cancellation function
            - Complete(RequestFromUpper) & Delete(its Memory Object)

            - Delete(RequestUART)
            - Dereference(RequestFromUpper)

     2. Cancellation (both functions)
            A: Cancellation Function
               WdfRequestCancelSentRequest(RequestToUART) to cancel RequestToUART
            - Dereference(RequestToUART) after cancel is sent
            - Complete(RequestFromUpper) & Delete(its Memory Object)

            B: Completion function
            WdfRequestUnmarkCancelable() returns STATUS_CANCELLED
            Exercise its cancellation code path
            - Delete(RequestToUART)
            - Dereference(RequestFromUpper)

     Note: Code path A & B have no synchronization object to ensure their order of execution, but reference is taken on the Requests to ensure
                that they stay valid until last access.

        RequestToUART - take a reference to protect against being used by the cancellation function; it is de-referenced by the
            - completion function - in its typical completion code path, or
            - cancellation function - after finishing accessing it (to sent cancel)

        RequestFromBthport - take a reference to protect against being completed by the cancellation function and then its context
                                         is later accessed by the completion function; this can happen if the completion function is completed
                                         asynchronously after WdfRequestCancelSentRequest() is returned; it is de-referenced by the
            - completion function - right before it exits.

Arguments:

    _Request - WDF Request allocated by this driver
    _Target - WDF IO Target
    _Params - Completion parameters
    _Context - Context used to process this request

Return Value:

    none

--*/
{
    NTSTATUS Status;
    PUART_WRITE_CONTEXT TransferContext;
    PFDO_EXTENSION FdoExtension;
    WDFREQUEST RequestFromBthport;
    ULONG  BytesDataWritten = 0;
    LONG CompletePath = REQUEST_PATH_NONE;

    UNREFERENCED_PARAMETER(_Target);

    Status = _Params->IoStatus.Status;
    TransferContext = (PUART_WRITE_CONTEXT) _Context;

    DoTrace(LEVEL_INFO, TFLAG_DATA,("+CR_WriteDeviceIO: %!STATUS!, Request %p, Context %p",
            Status, _Request, _Context));

    NT_ASSERT( (Status == STATUS_SUCCESS || Status == STATUS_CANCELLED) && L"WriteHCI request failed!");

    //
    // Request to be completed to upper layer.
    //
    RequestFromBthport = TransferContext->RequestFromBthport;

    //
    // The below operation can return one of the following values.
    // REQUEST_PATH_NONE
    //      This value was returned either because
    //      1. This is the normal operation for this function and the request from bthport has to be completed.
    //      2. The request from bthport has already been cancelled, but the cancellation routine has not yet been called (race condition).
    //
    //      No matter what causes this value to be returned, it is safe to call WdfRequestUnmarkCancelable on the request from bthport
    //
    // REQUEST_PATH_CANCELLATION
    //      The cancellation routine was already called.
    //
    //      This function does not have the control to complete the request from bthport.
    //
    CompletePath = InterlockedOr(&TransferContext->RequestCompletePath, REQUEST_PATH_COMPLETION);

    // Mark RequestFromBthPort not cancellable as it is about to be completed.
    if (REQUEST_PATH_NONE != CompletePath)
    {
        DoTrace(LEVEL_ERROR, TFLAG_IO,(" Request %p is in the process of being cancelled", RequestFromBthport));
    }
    else
    {
        //
        // Call WdfRequestUnmarkCancelable() to check whether this request has already been cancelled.
        //
        if (STATUS_CANCELLED == WdfRequestUnmarkCancelable(RequestFromBthport)) {
            //
            // The request from bthport has already been cancelled.
            // Try to relinquish control of completing the request from bthport to the cancellation routine. It is possible that the cancellation routine
            // has already been executed. In this case, this routine will have to complete the request from bthport.
            //
            // The below operation can return one of the following values.
            // REQUEST_PATH_CANCELLATION | REQUEST_PATH_COMPLETION
            //      The cancellation routine was called. The cancellation will not complete the request, so this function will have to complete it.
            //
            // REQUEST_PATH_COMPLETION
            //      The cancellation routine has not yet been called.
            //      The InterlockedCompareExchange successfully masked the REQUEST_PATH_COMPLETE bit and so the completin routine
            //      will complete this request.
            //
            CompletePath = InterlockedCompareExchange(&TransferContext->RequestCompletePath,
                                                      REQUEST_PATH_NONE,
                                                      REQUEST_PATH_COMPLETION);

            //
            // Since the cancellation was already called and it will not complete the request, reset the value of complete to
            // REQUEST_PATH_NONE so that the request from bthport will be completed.
            //
            if (CompletePath & REQUEST_PATH_CANCELLATION) {
                CompletePath = REQUEST_PATH_NONE;
            }
        }

        if (REQUEST_PATH_NONE == CompletePath) {

            // Dereference this request as cancellation function is not invoked to access it.
            WdfObjectDereference(_Request);

            //
            // Return data transfer information to caller for success Status
            //
            if (NT_SUCCESS(Status))
            {
                WDFMEMORY ReqOutMemory = NULL;
                ULONG  BytesWritten;
                PULONG    OutBuffer = NULL;
                size_t    OutBufferSize = 0;

                BytesWritten =  (ULONG) _Params->Parameters.Write.Length;

                DoTrace(LEVEL_INFO, TFLAG_DATA,("   Packet: Type %d, DataLen %d, BytesWritten %d",
                        TransferContext->HCIContext->Type,
                        TransferContext->HCIContext->DataLen,
                        BytesWritten));

                NT_ASSERT(BytesWritten == TransferContext->HCIPacketLen && "Unexpected incomplete HCI Write!");

                if (BytesWritten != TransferContext->HCIPacketLen)
                {
                    // return a generic failure for an incomplete transfer
                    Status = STATUS_UNSUCCESSFUL;
                    goto Done;
                }

                //
                // return data bytes written in the OutputParameter
                //
                Status = WdfRequestRetrieveOutputMemory(RequestFromBthport, &ReqOutMemory);
                if (NT_SUCCESS(Status))
                {
                    OutBuffer = (PULONG) WdfMemoryGetBuffer(ReqOutMemory, &OutBufferSize);
                    if (OutBufferSize >= sizeof(ULONG))
                    {
                        // Set OutputParameter value and its size
                        *OutBuffer = TransferContext->HCIContext->DataLen;
                        BytesDataWritten = sizeof(ULONG);
                    }
                }
            }
            else
            {
                // Return  the status as is.
            }
        }
    }

Done:

    if (REQUEST_PATH_NONE == CompletePath)
    {
        // Increment the completion count based on packet type.
        FdoExtension = TransferContext->FdoExtension;

        if (TransferContext->HCIContext->Type == (UCHAR) HciPacketCommand)
        {
            InterlockedIncrement(&FdoExtension->CntCommandCompleted);
        }
        else if (TransferContext->HCIContext->Type == (UCHAR) HciPacketAclData)
        {
            InterlockedIncrement(&FdoExtension->CntWriteDataCompleted);
        }

        DoTrace(LEVEL_INFO, TFLAG_IO,(" WriteDeviceIO: Request %p complete with %!STATUS! and %d BytesDataWritten",
                RequestFromBthport, Status, BytesDataWritten));

        // Delete this memory object that is no longer needed.
        WdfObjectDelete(TransferContext->Memory);

        // Cannot access this Request and its context after it is completed.
        WdfRequestCompleteWithInformation(RequestFromBthport, Status, BytesDataWritten);

    }

    // Delete this request in its completion function.
    WdfObjectDelete(_Request);

    // Done accessing it in this function.   This request is either completed in this function for the typical completion situation or in the cancellation function.
    WdfObjectDereference(RequestFromBthport);

    DoTrace(LEVEL_INFO, TFLAG_IO,("-CR_WriteDeviceIO"));
}


VOID
ReadSegmentStateSet(
    PUART_READ_CONTEXT _ReadContext,
    UART_READ_STATE    _NewState
    )
/*++

Routine Description:

    This helper centralize the setting of read state. It can be used to detect
    possible incorrect state transition.

Arguments:

    _ReadContext - read context which has existing state
    _NewState - new read state

Return Value:

    none

--*/
{
    UART_READ_STATE OldState = _ReadContext->ReadSegmentState;

    DoTrace(LEVEL_INFO, TFLAG_IO, ("+<<<< -- %s to %s state -- >>>>",
            OldState == GET_PKT_TYPE    ? "Type"    :
            OldState == GET_PKT_HEADER  ? "Header"  :
            OldState == GET_PKT_PAYLOAD ? "Payload" : "Unknown",
            _NewState == GET_PKT_TYPE    ? "Type"    :
            _NewState == GET_PKT_HEADER  ? "Header"  :
            _NewState == GET_PKT_PAYLOAD ? "Payload" : "Unknown" ));

    // Validate the state transition
    switch (_NewState)
    {
        case GET_PKT_TYPE:
            // Intialize the context for a new packet
            _ReadContext->BytesReadNextSegment = 0;
            _ReadContext->H4Packet.Type = 0;
            _ReadContext->BytesToRead4FullPacket = 0;
            RtlZeroMemory(_ReadContext->H4Packet.Packet.Raw, HCI_ACLDATA_HEADER_LEN);
            break;
        case GET_PKT_HEADER:
        case GET_PKT_PAYLOAD:
            // Reset segment count
            _ReadContext->BytesReadNextSegment = 0;
            break;
    }

    _ReadContext->ReadSegmentState = _NewState;
}

                        // Full packet: match to a Request and complete it.
NTSTATUS
ReadH4PacketComplete(
    PFDO_EXTENSION _FdoExtension,
    UCHAR  _Type,
    _In_reads_bytes_(_BufferLength) PUCHAR _Buffer,
    ULONG  _BufferLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DoTrace(LEVEL_INFO, TFLAG_IO, ("+ReadH4PacketComplete %S Packet Length %d",
        _Type == (UCHAR) HciPacketEvent ? L"Event" : L"AclData", _BufferLength ));

#if DBG
    // Tracking last completed packet
    RtlCopyMemory(_FdoExtension->LastPacket, _Buffer, _BufferLength);
    _FdoExtension->LastPacketLength = _BufferLength;
#endif

    if (_Type == (UCHAR) HciPacketEvent)
    {
        ReadRequestComplete(_FdoExtension,
                            HciPacketEvent,
                            _BufferLength,
                            _Buffer,
                            _FdoExtension->ReadEventQueue,
                            &_FdoExtension->EventQueueCount,
                            &_FdoExtension->ReadEventList,
                            &_FdoExtension->EventListCount);
    }
    else
    {
        ReadRequestComplete(_FdoExtension,
                            HciPacketAclData,
                            _BufferLength,
                            _Buffer,
                            _FdoExtension->ReadDataQueue,
                           &_FdoExtension->DataQueueCount,
                           &_FdoExtension->ReadDataList,
                           &_FdoExtension->DataListCount);
    }

    DoTrace(LEVEL_INFO, TFLAG_IO, ("-ReadH4PacketComplete %!STATUS!", Status));

    return Status;
}

NTSTATUS
ReadH4PacketReassemble(
    _Inout_  PUART_READ_CONTEXT _ReadContext,
    _In_  ULONG  _BytesRead,
    _In_reads_bytes_(_BytesRead) PUCHAR _Buffer
    )
/*++

Routine Description:

    A function enforce a state machine to process reading data to form a
    complete HCI packet.

Arguments:

    _ReadContext - read context
    _BytesRead - bytes of data read and is in the output buffer
    _OutBuffer - Buffer that contain the data

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesRemained = _BytesRead;
    PUCHAR Buffer = _Buffer;
    PFDO_EXTENSION FdoExtension = _ReadContext->FdoExtension;
    PH4_PACKET H4Packet;
    ULONG PacketLen;
    ULONG BytesToRead;

    DoTrace(LEVEL_INFO, TFLAG_IO, ("+ReadH4PacketReassemble: %d _BytesRead, ReadSegmentState %d",
        _BytesRead, _ReadContext->ReadSegmentState));

    //
    // By design, it will take two reads to complete an H4 packets.
    //
    // First Read (5 bytes = 1 + 4 = Type + Larger of (ACLDataHeader:4, EvetnHeader:2))
    //
    //   - Event
    //       Complete (1 + 2    ), this is an Event packet without any param.
    //       Complete (1 + 2 + 1), event with 1 param
    //         * These two outcome requires interval timeout to complete the read (ask for 5).
    //       Complete (1 + 2 + 2), event with 2 params
    //         * if completed with one read, do the First read again.
    //
    //       Partial  (1 + 2 + 2 + ParamCount-2), this will complete in next read
    //            BytesToRead = ParamCount - 2
    //
    //   - ACL Data
    //       Partial (1 + 4 + DataLength), this packet will be complete in next read
    //            ByteToRead = DataLength
    // Second read
    //   - Event/AclData
    //       Complete (5 + BytesToRead)
    //

    while (NT_SUCCESS(Status) && BytesRemained > 0) {

        // Process read buffer based on its read state
        switch (_ReadContext->ReadSegmentState) {
        case GET_PKT_TYPE:
            H4Packet = (PH4_PACKET) Buffer;
            BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, 1);

            if (H4Packet->Type == (UCHAR) HciPacketEvent) {
                DoTrace(LEVEL_INFO, TFLAG_IO, (" [Event] ---------- "));
                 _ReadContext->BytesToRead4FullPacket = HCI_EVENT_HEADER_SIZE;
            }
            else if (H4Packet->Type == (UCHAR) HciPacketAclData) {
                DoTrace(LEVEL_INFO, TFLAG_IO, (" [AclData] ---------- "));
                 _ReadContext->BytesToRead4FullPacket = HCI_ACL_HEADER_SIZE;
            }
            else {
                //
                // Abort the read operation here but can consider to traverse the data
                // until a valid packet type is found.
                //
                Status = STATUS_INVALID_PARAMETER;  // discard and read again
                DoTrace(LEVEL_ERROR, TFLAG_IO, (" Unexpected PacketType %d", H4Packet->Type));
                NT_ASSERT(FALSE && L"Detected unknown packet type");
                goto OutOfSync;
            }

            // Proceed to read packet header
            _ReadContext->H4Packet.Type = H4Packet->Type;    // Valid packet type is cached.
            ReadSegmentStateSet(_ReadContext, GET_PKT_HEADER);
            break;

        case GET_PKT_HEADER:
            if (_ReadContext->H4Packet.Type == (UCHAR) HciPacketEvent) {
                if (_ReadContext->BytesReadNextSegment == 0 && BytesRemained) {
                    _ReadContext->H4Packet.Packet.Event.EventCode = *Buffer;
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [Event] Code 0x%x", _ReadContext->H4Packet.Packet.Event.EventCode));
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, 1);
                    _ReadContext->BytesToRead4FullPacket = 1;  // Read the ParamsCount if needed
                }

                if (_ReadContext->BytesReadNextSegment == 1 && BytesRemained) {
                    _ReadContext->H4Packet.Packet.Event.ParamsCount = *Buffer;
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [Event] ParamsCount 0x%x", _ReadContext->H4Packet.Packet.Event.ParamsCount));
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, 1);

                    if (_ReadContext->H4Packet.Packet.Event.ParamsCount == 0) {
                        // Full packet: match to a Request and complete it.
                        PacketLen = HCI_EVENT_HEADER_LEN + _ReadContext->H4Packet.Packet.Event.ParamsCount;
                        DoTrace(LEVEL_INFO, TFLAG_DATA, (" [Event completed] PacketLen %d", PacketLen));
                        Status = ReadH4PacketComplete(FdoExtension,
                                                      _ReadContext->H4Packet.Type,
                                                      (PUCHAR) &_ReadContext->H4Packet.Packet.Event,
                                                      PacketLen);
                        // Read next packet
                        ReadSegmentStateSet(_ReadContext, GET_PKT_TYPE);
                    }
                    // Read the remainder of a full (Event) packet
                    else {
                        if (BytesRemained < _ReadContext->H4Packet.Packet.Event.ParamsCount) {
                          _ReadContext->BytesToRead4FullPacket =
                                _ReadContext->H4Packet.Packet.Event.ParamsCount - BytesRemained;
                        }

                        // Process to read packet payload
                        ReadSegmentStateSet(_ReadContext, GET_PKT_PAYLOAD);
                    }
                }
            }
            else {

                if (_ReadContext->BytesReadNextSegment == 0 && BytesRemained) {
                    _ReadContext->H4Packet.Packet.Raw[_ReadContext->BytesReadNextSegment] = *Buffer;
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [AclData] Header[0] 0x%x", _ReadContext->H4Packet.Packet.Raw[_ReadContext->BytesReadNextSegment]));
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, 1);
                    _ReadContext->BytesToRead4FullPacket = 3;  // Read the remaining Dta header if needed
                }

                if (_ReadContext->BytesReadNextSegment == 1 && BytesRemained) {
                    _ReadContext->H4Packet.Packet.Raw[_ReadContext->BytesReadNextSegment] = *Buffer;
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [AclData] Header[1] 0x%x", _ReadContext->H4Packet.Packet.Raw[_ReadContext->BytesReadNextSegment]));
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, 1);
                    _ReadContext->BytesToRead4FullPacket = 2;  // Read the remaining Dta header if needed
                }

                if (_ReadContext->BytesReadNextSegment == 2 && BytesRemained) {
                    _ReadContext->H4Packet.Packet.Raw[_ReadContext->BytesReadNextSegment] = *Buffer;
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [AclData] Header[2] 0x%x", _ReadContext->H4Packet.Packet.Raw[_ReadContext->BytesReadNextSegment]));
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, 1);
                    _ReadContext->BytesToRead4FullPacket = 1;  // Read the remaining Dta header if needed
                }

                if (_ReadContext->BytesReadNextSegment == 3 && BytesRemained) {
                    _ReadContext->H4Packet.Packet.Raw[_ReadContext->BytesReadNextSegment] = *Buffer;
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [AclData] Header[3] 0x%x", _ReadContext->H4Packet.Packet.Raw[_ReadContext->BytesReadNextSegment]));
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, 1);

                    // Read the reamainder of a full (Data) packet
                    if (BytesRemained < _ReadContext->H4Packet.Packet.AclData.DataLength) {
                        _ReadContext->BytesToRead4FullPacket =
                            _ReadContext->H4Packet.Packet.AclData.DataLength - BytesRemained;
                    }

                    // Process to read packet payload
                    ReadSegmentStateSet(_ReadContext, GET_PKT_PAYLOAD);
                }
            }
            break;

        case GET_PKT_PAYLOAD:
            if (_ReadContext->H4Packet.Type == (UCHAR) HciPacketEvent) {

                BytesToRead = _ReadContext->H4Packet.Packet.Event.ParamsCount - _ReadContext->BytesReadNextSegment;

                if (BytesRemained >= BytesToRead) {
                    // Full packet
                    RtlCopyMemory(&_ReadContext->H4Packet.Packet.Event.Params[_ReadContext->BytesReadNextSegment],
                                  Buffer,
                                  BytesToRead);
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [Event] Payload[%d + %d] = FULL",
                            _ReadContext->BytesReadNextSegment,
                            BytesToRead));
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, BytesToRead);

                    // Full packet: match to a Request and complete it.
                    PacketLen = HCI_EVENT_HEADER_LEN + _ReadContext->H4Packet.Packet.Event.ParamsCount;
                    Status = ReadH4PacketComplete(FdoExtension,
                                                  _ReadContext->H4Packet.Type,
                                                  (PUCHAR) &_ReadContext->H4Packet.Packet.Event,
                                                  PacketLen);
                    // Read next packet
                    ReadSegmentStateSet(_ReadContext, GET_PKT_TYPE);
                }
                else {
                    // Partial packet
                    RtlCopyMemory(&_ReadContext->H4Packet.Packet.Event.Params[_ReadContext->BytesReadNextSegment],
                                  Buffer,
                                  BytesRemained);
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [Event] Payload[%d + %d] = Partial; %d to read",
                            _ReadContext->BytesReadNextSegment,
                            BytesRemained,
                            BytesToRead - BytesRemained));
                    _ReadContext->BytesReadNextSegment += BytesRemained;
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, BytesRemained);

                    // Remaining event params to read
                    _ReadContext->BytesToRead4FullPacket =
                        _ReadContext->H4Packet.Packet.Event.ParamsCount - _ReadContext->BytesReadNextSegment;
                }
            }
            else {

                if (_ReadContext->H4Packet.Packet.AclData.DataLength > HCI_MAX_ACL_PAYLOAD_SIZE) {
                    Status = STATUS_INVALID_PARAMETER;  // discard and read again
                    DoTrace(LEVEL_ERROR, TFLAG_IO, (" Unexpected ACL DataLength %d > Presetted maximum size %d",
                            _ReadContext->H4Packet.Packet.AclData.DataLength,
                            HCI_MAX_ACL_PAYLOAD_SIZE));
                    NT_ASSERT(FALSE && L"Max ACL DataLength exceeded the presetted Max");
                    goto OutOfSync;
                }

                BytesToRead = _ReadContext->H4Packet.Packet.AclData.DataLength - _ReadContext->BytesReadNextSegment;

                if (BytesRemained >= BytesToRead) {
                    // Process full packet
                    RtlCopyMemory(&_ReadContext->H4Packet.Packet.AclData.Data[_ReadContext->BytesReadNextSegment],
                                  Buffer,
                                  BytesToRead);
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [AclData] Payload[%d + %d] = FULL",
                            _ReadContext->BytesReadNextSegment,
                            BytesToRead));
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, BytesToRead);

                    // Full packet: try match to a Request in queue (if any) and complete it.
                    PacketLen = HCI_ACLDATA_HEADER_LEN + _ReadContext->H4Packet.Packet.AclData.DataLength;
                    Status = ReadH4PacketComplete(FdoExtension,
                                                  _ReadContext->H4Packet.Type,
                                                  (PUCHAR) &_ReadContext->H4Packet.Packet.AclData,
                                                  PacketLen);
                    // Next packet
                    ReadSegmentStateSet(_ReadContext, GET_PKT_TYPE);
                }
                else {
                    // Process partial packet
                    RtlCopyMemory(&_ReadContext->H4Packet.Packet.AclData.Data[_ReadContext->BytesReadNextSegment],
                                  Buffer,
                                  BytesRemained);
                    DoTrace(LEVEL_INFO, TFLAG_IO, (" [AclData] Payload[%d + %d] = Partial; %d to read",
                            _ReadContext->BytesReadNextSegment,
                            BytesRemained,
                            BytesToRead - BytesRemained));
                    _ReadContext->BytesReadNextSegment += BytesRemained;
                    BUFFER_AND_SIZE_ADJUSTED(Buffer, BytesRemained, _ReadContext->BytesReadNextSegment, BytesRemained);

                    // Remaining data to read
                    _ReadContext->BytesToRead4FullPacket =
                        _ReadContext->H4Packet.Packet.AclData.DataLength - _ReadContext->BytesReadNextSegment;
                }
            }
            break;

        default:
            DoTrace(LEVEL_ERROR, TFLAG_IO, (" Unknown ReadSegmentState"));
            break;
        }
    }

    return Status;

OutOfSync:

    DoTrace(LEVEL_ERROR, TFLAG_IO, (" Out-of-sync error detected in ProcessReadBuffer() %!STATUS!", Status));

    return Status;
}

VOID
ReadH4PacketCompletionRoutine(
    _In_  WDFREQUEST   _Request,
    _In_  WDFIOTARGET  _Target,
    _In_  PWDF_REQUEST_COMPLETION_PARAMS  _Params,
    _In_  WDFCONTEXT  _Context
    )
/*++

Routine Description:

    This is CR function for reading data from device.  It process the data read and
    send down another request unless there is an error or the request is being
    canceled.

Arguments:

    _Request - a caller allocated WDF Request
    _Target - WDF IO Target
    _Params - Completion parameters
    _Context - Context of this request

Return Value:

    none

--*/
{
    NTSTATUS Status;
    PUART_READ_CONTEXT ReadContext;
    PFDO_EXTENSION FdoExtension;
    ULONG BytesRead;
    WDFMEMORY ReadMemory;
    PUCHAR  OutBuffer;
    size_t  OutBufferSize;
    READ_REQUEST_STATE PreviousState;

    UNREFERENCED_PARAMETER(_Request);
    UNREFERENCED_PARAMETER(_Target);

    // Operation result
    Status = _Params->IoStatus.Status;
    BytesRead =  (ULONG) _Params->Parameters.Read.Length;

    ReadContext = (PUART_READ_CONTEXT) _Context;
    ReadContext->Status = Status;

    // Set to REQUEST_COMPLETE if skip REQUEST_PENDING state.
    PreviousState = InterlockedCompareExchange((PLONG)&ReadContext->RequestState,
                                               REQUEST_COMPLETE,
                                               REQUEST_SENT);

    DoTrace(LEVEL_WARNING, TFLAG_DATA, ("+ReadH4PacketCompletionRoutine %!STATUS! %d BytesRead %S)",
            Status, BytesRead, PreviousState == REQUEST_PENDING ? L"Async" : L"*Sync*"));

    FdoExtension = (PFDO_EXTENSION) ReadContext->FdoExtension;

    //
    // The return status can either be
    //      - successful (buffer completely filled),
    //      - timeout (buffer not completed filled prior to interval timeout expired
    //      - cancellation
    //      - failure
    //
    if (NT_SUCCESS(Status) || Status == STATUS_IO_TIMEOUT || Status == STATUS_TIMEOUT) {
        // Continue to process
    }
    else  {
        DoTrace(LEVEL_ERROR, TFLAG_IO, (" ReadH4PacketCompletionRoutine failed %!STATUS!", Status));
        if (Status == STATUS_CANCELLED) {
            //
            // Under regualr operational state, IO Target will only cancel a request
            // when it is ready to abort (e.g. device removal).
            //
        }

        goto Exit;
    }

    ReadMemory = _Params->Parameters.Read.Buffer;
    OutBuffer = (PUCHAR) WdfMemoryGetBuffer(ReadMemory, &OutBufferSize);
    NT_ASSERT(OutBufferSize >= BytesRead);
    DoTrace(LEVEL_INFO, TFLAG_IO, (" ReadH4PacketCompletionRoutine %d BytesRead pBuffer %p", BytesRead, OutBuffer));

    //
    // Process a read buffer if there is data
    //
    if (OutBuffer && BytesRead)
    {
        //
        // Process the incoming data to form partial or full H4 packet
        //
        Status = ReadH4PacketReassemble(ReadContext,
                                        BytesRead,
                                        OutBuffer);

        // If data stream error, ignore the packet and start over.
        if (!NT_SUCCESS(Status))
        {
            FdoExtension->OutOfSyncErrorCount++;
            DoTrace(LEVEL_ERROR, TFLAG_IO, (" ====> [%d] 0x%x  <=====",
                    FdoExtension->OutOfSyncErrorCount,
                    *OutBuffer));
            NT_ASSERT(NT_SUCCESS(Status) && L"Encountered an out-of-sync condition!");

            // Prepare to read next data packet, starting with packet type.
            ReadSegmentStateSet(ReadContext, GET_PKT_TYPE);

            // Log(Error): log statistic of the read pump until this error

            //
            // If there is a (knonw) hardware error or if we have exceeded maximun hardware count,
            // the link is no longer reliable.  Need to report to the upper layer via a read request.
            //
            if (FdoExtension->HardwareErrorDetected && FdoExtension->OutOfSyncErrorCount > MAX_HARDWARE_ERROR_COUNT)
            {
                //
                // Complete an event or read data request with STATUS_DEVICE_DATA_ERROR error to trigger
                // BthMini/BthPort to handle the situation.  IT can perform HCI_RESET to restore the
                // data channel.
                //
#ifdef REPORT_HARDWARE_ERROR
                WDFREQUEST Request;

                DoTrace(LEVEL_ERROR, TFLAG_IO, (" ++++ Report a hardware error; OutOfSyncCount %d",  FdoExtension->OutOfSyncErrorCount));

                WdfSpinLockAcquire(FdoExtension->QueueAccessLock);
                    // Complete a read (event or data) request with a specific error to indicate hardware error.
                    Status = WdfIoQueueRetrieveNextRequest(FdoExtension->ReadEventQueue, &Request);

                    // if there is no event request, find a read data request.
                    if (Status == STATUS_NO_MORE_ENTRIES)
                    {
                        Status = WdfIoQueueRetrieveNextRequest(FdoExtension->ReadDataQueue, &Request);
                    }
                WdfSpinLockRelease(FdoExtension->QueueAccessLock);

                if (NT_SUCCESS(Status))
                {
                    DoTrace(LEVEL_ERROR, TFLAG_IO, (" Complete a request with STATUS_DEVICE_DATA_ERROR"));
                    WdfRequestComplete(Request, STATUS_DEVICE_DATA_ERROR);
                }
#endif  // REPORT_HARDWARE_ERROR
                Status = STATUS_DEVICE_DATA_ERROR;

                // abort and stop read pump
                goto Exit;

            }
            else
            {

                DoTrace(LEVEL_ERROR, TFLAG_IO, (" Detect out-of-sync error but read ahead..."));

                // Reset hardware error.
                FdoExtension->HardwareErrorDetected = FALSE;

                // try next
                goto ReadNext;
            }
        }
    }
    else
    {
        NT_ASSERT(Status == STATUS_TIMEOUT);
    }

ReadNext:

    if (PreviousState == REQUEST_PENDING)
    {
        ULONG BytesToRead;

        //
        // Determine what is the size of the buffer to send down.
        //
        BytesToRead = (ReadContext->ReadSegmentState == GET_PKT_TYPE ? INITIAL_H4_READ_SIZE :
                       ReadContext->BytesToRead4FullPacket ? ReadContext->BytesToRead4FullPacket :
                       sizeof(FdoExtension->ReadBuffer));

        DoTrace(LEVEL_INFO, TFLAG_IO, (" ReadH4Packet(Read Buffer Size %d bytes)", BytesToRead));

        // Issue next read here since this request was complete asychronously
        // i.e. pending first and then this completion routein is invoked.
        ReadH4Packet(ReadContext,
                     FdoExtension->ReadRequest,
                     FdoExtension->ReadMemory,
                     FdoExtension->ReadBuffer,
                     BytesToRead);
    }
    else
    {
        // Fall through and leave this fucntion if this request was completed synchronously;
        // i.e. this function is invoked first and then return to the RequestSent function.
    }

    DoTrace(LEVEL_INFO, TFLAG_IO, ("-CR_ReadReadIO (fall though)"));

    return;

Exit:

    if (!NT_SUCCESS(Status))
    {
        NT_ASSERT(Status == STATUS_CANCELLED);
        FdoExtension->ReadPumpRunning = FALSE;
        DoTrace(LEVEL_WARNING, TFLAG_IO, (" Pump has stopped!"));
    }

    DoTrace(LEVEL_INFO, TFLAG_IO, ("-CR_ReadReadIO (error)"));
}

NTSTATUS
ReadH4Packet(
    _In_  PUART_READ_CONTEXT _ReadContext,
    _In_  WDFREQUEST         _WdfRequest,
    _In_  WDFMEMORY          _WdfMemory,
    _Pre_notnull_ _Pre_writable_byte_size_ (_BufferLen) PVOID _Buffer,
    _In_  ULONG              _BufferLen
    )
/*++

Routine Description:

    Initiate the reading of an HCI packet (event or data) by sending down a read request.

Arguments:

    _ReadContext - Context used for reading data from target UART device

Return Value:

    NTSTATUS

--*/
{
    PFDO_EXTENSION   FdoExtension;
    WDF_REQUEST_REUSE_PARAMS RequestReuseParams;
    NTSTATUS Status;

    DoTrace(LEVEL_INFO, TFLAG_IO, ("+ReadH4Packet"));

    FdoExtension = _ReadContext->FdoExtension;

    if (0 == _BufferLen) {
        DoTrace(LEVEL_ERROR, TFLAG_IO, (" ReadH4Packet: _BufferLen cannot be 0"));
        Status = STATUS_INVALID_PARAMETER;
        goto Done;
    }

    while (TRUE) {

        DoTrace(LEVEL_INFO, TFLAG_IO, (" ReadH4Packet - <start>"));
        NT_ASSERT(_ReadContext->RequestState != REQUEST_SENT);

        if (!IsDeviceInitialized(FdoExtension)) {
            Status = STATUS_DEVICE_NOT_READY;
            DoTrace(LEVEL_ERROR, TFLAG_IO, (" ReadH4Packet: cannot attach IO %!STATUS!", Status));
            goto Done;
        }

        //
        // Issue a read event request
        //
        WDF_REQUEST_REUSE_PARAMS_INIT(&RequestReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
        Status = WdfRequestReuse(_WdfRequest, &RequestReuseParams);
        if (!NT_SUCCESS(Status)) {
            DoTrace(LEVEL_ERROR, TFLAG_IO, (" WdfRequestReuse failed %!STATUS!", Status));
            goto Done;
        }

        Status = WdfMemoryAssignBuffer(_WdfMemory, _Buffer, _BufferLen);
        if (!NT_SUCCESS(Status)) {
            DoTrace(LEVEL_ERROR, TFLAG_IO, (" WdfMemoryAssignBuffer failed %!STATUS!", Status));
            goto Done;
        }

        Status = WdfIoTargetFormatRequestForRead(FdoExtension->IoTargetSerial,
                                                 _WdfRequest,
                                                 _WdfMemory,
                                                 NULL, NULL);

        if (!NT_SUCCESS(Status)) {
            DoTrace(LEVEL_ERROR, TFLAG_IO, (" WdfIoTargetFormatRequestForRead failed %!STATUS!", Status));
            goto Done;
        }

        // Note: This request is sent to UART driver so it cannot be marked cancellable.
        // But it can be canceled by issuing WdfRequestCancelSentRequest().

        WdfRequestSetCompletionRoutine(_WdfRequest,
                                       ReadH4PacketCompletionRoutine,
                                       _ReadContext);

        InterlockedExchange((PLONG)&_ReadContext->RequestState, REQUEST_SENT);

        if (FALSE == WdfRequestSend(_WdfRequest,
                                    FdoExtension->IoTargetSerial,
                                    WDF_NO_SEND_OPTIONS))
        {
            Status = WdfRequestGetStatus(_WdfRequest);
            DoTrace(LEVEL_ERROR, TFLAG_IO, (" WdfRequestSend failed %!STATUS!", Status));

            // Not much we can do if cannot send this request; data pump will be stopped!
            goto Done;
        }
        else
        {
            READ_REQUEST_STATE PreviousState;

            // Set to REQUEST_PENDING if it is in the REQUEST_SENT state.
            PreviousState = InterlockedCompareExchange((PLONG) &_ReadContext->RequestState,
                                                       REQUEST_PENDING,
                                                       REQUEST_SENT);

            DoTrace(LEVEL_WARNING, TFLAG_IO, (" WdfRequestSend ReqState: %d -> %d",
                    PreviousState, _ReadContext->RequestState));

            if (PreviousState == REQUEST_SENT)
            {
                // Request is still pending, and will be completed asychronously in the
                // completion routine where it can issue next read.
                Status = STATUS_PENDING;
                break;
            }
            else
            {
                Status = FdoExtension->ReadContext.Status;
                if (NT_SUCCESS(Status))
                {
                    // Previous request has been complete synchronously in the
                    // completion routine; do next read in this function.
                }
                else
                {
                    // No tolerance for error
                    break;
                }
            }
        }
    }

Done:

    if (!NT_SUCCESS(Status))
    {
        NT_ASSERT(Status == STATUS_CANCELLED);
        FdoExtension->ReadPumpRunning = FALSE;
    }

    DoTrace(LEVEL_INFO, TFLAG_IO, ("-ReadH4Packet %!STATUS!", Status));

    return Status;
}

__inline
PHCI_PACKET_ENTRY
HLP_CreatePacketEntry(
    _In_ ULONG  _PacketLength,
    _In_reads_bytes_(_PacketLength) PUCHAR _Packet
    )
{
    PHCI_PACKET_ENTRY  PacketEntry = NULL;

    PacketEntry = (PHCI_PACKET_ENTRY)ExAllocatePool(NonPagedPoolNx, sizeof(HCI_PACKET_ENTRY) + _PacketLength);
    if (PacketEntry != NULL) {
        InitializeListHead(&PacketEntry->DataEntry);
        RtlCopyMemory(PacketEntry->Packet, _Packet, _PacketLength);
        PacketEntry->PacketLen = _PacketLength;
    }

    return PacketEntry;
}

NTSTATUS
ReadRequestComplete(
    _In_ PFDO_EXTENSION _FdoExtension,
    _In_ UCHAR          _PacketType,
    _In_ ULONG          _PacketLength,
    _In_reads_bytes_opt_(_PacketLength) PUCHAR _Packet,
    _Inout_ WDFQUEUE    _Queue,
    _Inout_ PLONG       _QueueCount,
    _Inout_ PLIST_ENTRY _ListHead,
    _Inout_ PLONG       _ListCount
    )
/*++
Routine Description:

    This helper function processes both complete HCI Data packet from the device to find
    a pending Request, or find a completed HCI packet in a list to complete a Request.

Arguments:

    _FdoExtension - Device context
    _PacketType - HCI packet type (either Event or Data for incoming data)
    _ListHead - List where to retrieve completed HCI packet
    _Request - Request that is used to complete a read if a corresponding HCI packet is available.

Return Value:

    NTSTATUS - STATUS_SUCCESS Or STATUS_INSUFFICIENT_RESOURCE

--*/
{
    WDFREQUEST  Request = NULL;
    NTSTATUS    Status = STATUS_SUCCESS;
    PHCI_PACKET_ENTRY  PacketEntry = NULL;
    WDFMEMORY ReqOutMemory;
    size_t BufferSize = 0, BytesToReturn;
    PBTHX_HCI_READ_WRITE_CONTEXT HCIContext;
    BOOLEAN CompleteRequest = FALSE;

    DoTrace(LEVEL_INFO, TFLAG_IO, ("+ReadRequestComplete"));

    //
    //      (ReqQueue, PktList)
    //  C0. ( empty,   empty) -> Add packet to list
    //  C1. ( empty,  !empty) -> Add packet to list
    //  C2. (!empty,   empty) -> DequeueAndCompletRequest(Packet)
    //  C3. (!empty,  !empty) -> Error! Cannot both empty at this function entry.
    //
    WdfSpinLockAcquire(_FdoExtension->QueueAccessLock);

    if (_Packet) {

        Status = WdfIoQueueRetrieveNextRequest(_Queue, &Request);
        if (Status == STATUS_SUCCESS) {
            // Case 2: Typical code path
            InterlockedDecrement(_QueueCount);
            DoTrace(LEVEL_INFO, TFLAG_IO, (" (C2) Complete a request %p, _Packet %p, _PacketLength %d",
                Request, _Packet, _PacketLength));

            CompleteRequest = TRUE;

            // Case 3: An error condition if List is not empty
            NT_ASSERT(IsListEmpty(_ListHead));
        }
        else {
            // Case 0:
            PacketEntry = HLP_CreatePacketEntry(_PacketLength, _Packet);
            if (PacketEntry == NULL) {
                // Error condition
                Status = STATUS_INSUFFICIENT_RESOURCES;
                DoTrace(LEVEL_ERROR, TFLAG_IO, (" (C0/Error) Could not allocate HCI_PACKET_ENTRY %!STATUS!", Status));
                // This packet will be dropped; but nothing we can do as system resource is depleted!
            }
            else {
                // Cache this packet to Packet List
                InsertTailList(_ListHead, &PacketEntry->DataEntry);
                InterlockedIncrement(_ListCount);
                DoTrace(LEVEL_INFO, TFLAG_IO, (" (C0) Queuing packet with list count %d", *_ListCount));
            }
        }
    }
    else {
        if (!IsListEmpty(_ListHead)) {
            Status = WdfIoQueueRetrieveNextRequest(_Queue, &Request);
            if (Status == STATUS_SUCCESS) {
                // Case 2: Has Packet in the list while a new request arrives
                InterlockedDecrement(_QueueCount);

                PacketEntry = (PHCI_PACKET_ENTRY) RemoveHeadList(_ListHead);
                _Packet = PacketEntry->Packet;
                _PacketLength = PacketEntry->PacketLen;
                InterlockedDecrement(_ListCount);

                DoTrace(LEVEL_INFO, TFLAG_IO, (" (C2) Complete a request %p, _Packet %p, _PacketLength %d",
                    Request, _Packet, _PacketLength));

                CompleteRequest = TRUE;
            }
            else {
                NT_ASSERT(FALSE && L"Failed to retrieve a request just queued!");
            }
        }
        else {
            // Case 1: Request is pre-pening and queued.
            Status = STATUS_PENDING;
            DoTrace(LEVEL_INFO, TFLAG_IO, (" (C1) Read request is queued"));
        }
   }

    WdfSpinLockRelease(_FdoExtension->QueueAccessLock);

    if (!CompleteRequest) {
        goto Done;
    }

    // Complete this request
    Status = WdfRequestRetrieveOutputMemory(Request, &ReqOutMemory);
    if (Status != STATUS_SUCCESS) {
        DoTrace(LEVEL_ERROR, TFLAG_IO, (" Could not retrieve output buffer"));
        WdfRequestCompleteWithInformation(Request, Status, (ULONG_PTR)0);
        goto Done;
    }

    HCIContext = WdfMemoryGetBuffer(ReqOutMemory, &BufferSize);
    BytesToReturn = FIELD_OFFSET(BTHX_HCI_READ_WRITE_CONTEXT, Data) + _PacketLength;

    // This should not happen because BthMini should have sent down largest buffer according to device's capability.
    NT_ASSERT(BytesToReturn <= BufferSize);

    // Transfer data to Request's output buffer
    HCIContext->Type    = _PacketType;
    HCIContext->DataLen = _PacketLength;
    if (BytesToReturn <= BufferSize) {
        RtlCopyMemory(&HCIContext->Data, _Packet, _PacketLength);
    }
    else {
        Status = STATUS_BUFFER_TOO_SMALL;
        BytesToReturn = 0;
    }

    // Validate and print out (WPP) HCI packet info
    HCIContextValidate(HCIContext->Type == (UCHAR) HciPacketEvent ?
                       _FdoExtension->CntEventCompleted : _FdoExtension->CntReadDataCompleted,
                       HCIContext);

    //
    // Release memory allocated for a completed packet entry; it was not removed from the packet list.
    //
    if (PacketEntry) {
        ExFreePool(PacketEntry);
    }

    if (HCIContext->Type == (UCHAR) HciPacketEvent) {
        InterlockedIncrement(&_FdoExtension->CntEventCompleted);
        DoTrace(LEVEL_INFO, TFLAG_DATA, (" [%d] HciPacketEvent completing %!STATUS!, %d BytesToReturn",
                _FdoExtension->CntEventCompleted, Status, (ULONG) BytesToReturn));
    }
    else if (HCIContext->Type == (UCHAR) HciPacketAclData) {
        InterlockedIncrement(&_FdoExtension->CntReadDataCompleted);
        DoTrace(LEVEL_INFO, TFLAG_DATA, (" [%d] HciPacketAclData completing %!STATUS!, %d BytesToReturn",
                _FdoExtension->CntReadDataCompleted, Status, (ULONG) BytesToReturn));
    }

    DoTrace(LEVEL_INFO, TFLAG_IO, (" Completing Request(%p) %!STATUS!, %d BytesToReturn",
            Request, Status, (ULONG) BytesToReturn));

    //
    // return only the actual data read, not including BTHX_HCI_READ_WRITE_CONTEXT
    //
    WdfRequestCompleteWithInformation(Request, Status, BytesToReturn);

Done:

    DoTrace(LEVEL_INFO, TFLAG_IO, ("-ReadRequestComplete: %!STATUS!", Status));

    return Status;
}

VOID
ReadResourcesFree(
    _In_  WDFDEVICE _Device
)
/*++
Routine Description:

    This helper function free resource allocated in its corresponding allocation
    function.

Arguments:

    _Device - WDF Device object

Return

    VOID

--*/
{
    PFDO_EXTENSION     FdoExtension;

    DoTrace(LEVEL_INFO, TFLAG_IO,("+ReadResourcesFree"));

    FdoExtension = FdoGetExtension(_Device);

    //
    // Note: The Request(s) in WDFQUEUE (Event and ReadData) WDFQUEUEs
    // are managed by WDF, which will dequeue and cancel them for us.
    // WdfIoQueueRetrieveNextRequest() returns STATUS_WDF_PAUSED since this
    // function is invoked after entered D0.
    //

    //
    // Free resources allocated earlier
    //

    while(!IsListEmpty(&FdoExtension->ReadEventList))
    {
        PHCI_PACKET_ENTRY  PacketEntry;

        WdfSpinLockAcquire(FdoExtension->QueueAccessLock);
            PacketEntry = (PHCI_PACKET_ENTRY)RemoveHeadList(&FdoExtension->ReadEventList);
            InterlockedDecrement(&FdoExtension->EventListCount);
        WdfSpinLockRelease(FdoExtension->QueueAccessLock);

        if (PacketEntry)
        {
            ExFreePool(PacketEntry);
            PacketEntry = NULL;
        }
    }
    NT_ASSERT(FdoExtension->EventListCount == 0);

    while(!IsListEmpty(&FdoExtension->ReadDataList))
    {
        PHCI_PACKET_ENTRY  PacketEntry;

        WdfSpinLockAcquire(FdoExtension->QueueAccessLock);
            PacketEntry = (PHCI_PACKET_ENTRY)RemoveHeadList(&FdoExtension->ReadDataList);
            InterlockedDecrement(&FdoExtension->DataListCount);
        WdfSpinLockRelease(FdoExtension->QueueAccessLock);

        if (PacketEntry)
        {
            ExFreePool(PacketEntry);
            PacketEntry = NULL;
        }
    }
    NT_ASSERT(FdoExtension->DataListCount == 0);

    if (FdoExtension->ReadRequest)
    {
        WdfObjectDelete(FdoExtension->ReadRequest);
        FdoExtension->ReadRequest = NULL;
    }
}

NTSTATUS
ReadResourcesAllocate(
    _In_  WDFDEVICE _Device
)
/*++
Routine Description:

    This helper function allocates resource (queues and lists) for managing read IOs
    Request from upper layer or for data pump with the device.

Arguments:

    _Device - WDF Device object

Return Value:

    NTSTATUS - STATUS_SUCCESS Or STATUS_INSUFFICIENT_RESOURCE

--*/
{
    NTSTATUS  Status;
    PFDO_EXTENSION   FdoExtension;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    WDF_OBJECT_ATTRIBUTES ObjAttributes;

    DoTrace(LEVEL_INFO, TFLAG_IO,("+ReadResourcesAllocate"));

    FdoExtension = FdoGetExtension(_Device);

    // HCI_EVENT
    //  Create WDF Queue for pending Read Event Request(s), and
    //  Initialize a List for pre-fetched Event
    WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig,
                             WdfIoQueueDispatchManual);

    Status = WdfIoQueueCreate(_Device,
                              &QueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &FdoExtension->ReadEventQueue);

    if (!NT_SUCCESS(Status))
    {
        DoTrace(LEVEL_ERROR, TFLAG_IO, (" WdfIoQueueCreate(Event) %!STATUS!", Status));
        goto Done;
    }

    InitializeListHead(&FdoExtension->ReadEventList);

    FdoExtension->EventListCount = 0;
    FdoExtension->EventQueueCount  = 0;

    // HCI_DATA
    //  Create WDF Queue for pending Read Data Request(s), and
    //  Initialize a List for pre-fetched Data
    Status = WdfIoQueueCreate(_Device,
                              &QueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &FdoExtension->ReadDataQueue);

    if (!NT_SUCCESS(Status))
    {
        DoTrace(LEVEL_ERROR, TFLAG_IO, (" WdfIoQueueCreate(Data) %!STATUS!", Status));
        goto Done;
    }

    InitializeListHead(&FdoExtension->ReadDataList);

    FdoExtension->DataListCount = 0;
    FdoExtension->DataQueueCount = 0;

    // Track request from top and HCI packets from device
    FdoExtension->CntCommandReq         = 0;
    FdoExtension->CntCommandCompleted   = 0;

    FdoExtension->CntEventReq           = 0;
    FdoExtension->CntEventCompleted     = 0;

    FdoExtension->CntWriteDataReq       = 0;
    FdoExtension->CntWriteDataCompleted = 0;

    FdoExtension->CntReadDataReq        = 0;
    FdoExtension->CntReadDataCompleted  = 0;

    // Create a WDF Request
    WDF_OBJECT_ATTRIBUTES_INIT(&ObjAttributes);
    ObjAttributes.ParentObject = _Device;

    Status = WdfRequestCreate(&ObjAttributes,
                              FdoExtension->IoTargetSerial,
                              &FdoExtension->ReadRequest);

    if (!NT_SUCCESS(Status))
    {
        DoTrace(LEVEL_ERROR, TFLAG_IO, (" WdfRequestCreate(ReadRequest) failed %!STATUS!", Status));
        goto Done;
    }

    // Initialize the ReadContext and its initial ReadSegmentState
    RtlZeroMemory(&FdoExtension->ReadContext, sizeof(UART_READ_CONTEXT));
    FdoExtension->ReadContext.FdoExtension = FdoExtension;
    ReadSegmentStateSet(&FdoExtension->ReadContext, GET_PKT_TYPE);

    Status = WdfMemoryCreatePreallocated(&ObjAttributes,
                                         &FdoExtension->ReadBuffer,
                                         sizeof(FdoExtension->ReadBuffer),
                                         &FdoExtension->ReadMemory);

    if (!NT_SUCCESS(Status))
    {
        DoTrace(LEVEL_ERROR, TFLAG_IO, (" WdfMemoryCreatePreallocated(ReadMemory) failed %!STATUS!", Status));
        goto Done;
    }

Done:

    DoTrace(LEVEL_INFO, TFLAG_IO,("-ReadResourcesAllocate %!STATUS!", Status));
    if (!NT_SUCCESS(Status))
    {
        ReadResourcesFree(_Device);
    }

    return Status;
}

