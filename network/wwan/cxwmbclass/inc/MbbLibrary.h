/***************************************************************************

Copyright (c) 2010 Microsoft Corporation

Module Name:

    MbbLibrary.h

Abstract:

    This header files contains the interfaces to the pack / unpack and
    fragment / reassembly functions provided by the Mbb library.
    The class driver Protocol layer calls the libary to convert payloads
    to messages as defined in the MBB Class Driver Hardware Interface
    document before sending it to the Bus Layer.

Environment:

    User and kernel mode only

Notes:

     This library can be used both in user and kernel mode because it does not
     to make any memory allocations.

Revision History:

    2/7/2010 : created

Authors:

    TriRoy

****************************************************************************/
EXTERN_C
NTSTATUS
MbbLibForamtBufferAsOpenMessage(__out_bcount_opt(*BufferLength) PVOID MessageBuffer, __inout PULONG BufferLength, __in ULONG MaximumControlTransfer);
/*
    Description
        Formats an input buffer as a MBB Open Message.

    Parameters
        __out_opt_bcount(*BufferLength) PVOID MessageBuffer
            The input message buffer. This parameter may be NULL
            if BufferLength is 0. This parameter should not be NULL
            when BufferLength is non-zero.

        __inout PULONG  BufferLength
            This provides the length of the memory location described by MessageBuffer.
            If BufferLength is 0 then the function will return the required buffer length
            in this parameter with STATUS_BUFFER_OVERFLOW. On success, the amount of
            buffer space used by the routine is returned to the caller in BufferLength.

        __in ULONG MaximumControlTransfer
            MaximumControlTransfer field of the Open Message.

    Return Value
        STATUS_BUFFER_OVERFLOW
            The buffer passed in is not sufficient for formatting the message.

        STATUS_INVALID_PARAMETER
            One of the required parameters is missing.

        STATUS_SUCCESS
            The input buffer was formatted successfully and the amount of buffer
            used is returned in BufferLength.
*/
EXTERN_C
NTSTATUS
MbbLibForamtBufferAsCloseMessage(__out_bcount_opt(*BufferLength) PVOID MessageBuffer, __inout PULONG BufferLength);
/*
    Description
        Formats an input buffer as a MBB Close Message.

    Parameters
        __out_opt_bcount(*BufferLength) PVOID MessageBuffer
            The input message buffer. This parameter may be NULL
            if BufferLength is 0. This parameter should not be NULL
            when BufferLength is non-zero.

        __inout PULONG  BufferLength
            This provides the length of the memory location described by MessageBuffer.
            If BufferLength is 0 then the function will return the required buffer length
            in this parameter with STATUS_BUFFER_OVERFLOW. On success, the amount of
            buffer space used by the routine is returned to the caller in BufferLength.

    Return Value
        STATUS_BUFFER_OVERFLOW
            The buffer passed in is not sufficient for formatting the message.

        STATUS_INVALID_PARAMETER
            One of the required parameters is missing.

        STATUS_SUCCESS
            The input buffer was formatted successfully and the amount of buffer
            used is returned in BufferLength.
*/
EXTERN_C
NTSTATUS
MbbLibFragmentBufferToCommandMessages(
    __in PMBB_COMMAND Command,
    __in MBB_COMMAND_TYPE CommandType,
    __in_bcount_opt(PayloadLength) PCHAR Payload,
    __in ULONG PayloadLength,
    __inout_opt PULONG TransactionId,
    __in ULONG CurrentFragment,
    __inout PULONG FragmentBufferCount,
    __inout PULONG FragmentLength,
    __out_ecount_opt(*FragmentBufferCount) PCHAR* FragmentBuffers);
/*
    Description
        Given the Payload the routine fragments the payload
        in to the FragmentBuffers.

    Parameters
        __in MBB_COMMAND Command
            Command contains the GUID and Cid.
            The GUID identifying the service for this message.
            The command identifier for the given service.

        __in MBB_COMMAND_TYPE CommandType
            Identifies whether the command is a Set or Query.

        __in_bcount(PayloadLength) PCHAR Payload
            The Payload is a byte array that needs to be fragmented.
            If Payload length is not 0 Payload should be provided.

        __in ULONG PayloadLength
            Length of the payload. If there is no Payload for this command
            Payload is set to 0.

        __inout PULONG TransactionId
            If non-zero then this is the transaction id used for the fragments.
            If zero then a new transcation id is generated and returned to the caller.

        __in ULONG CurrentFragment
            If non-zero then Fragments are constructed starting this Fragment.
            If zero then Fragments are constructed from the beginning.

        __inout ULONG FragmentBufferCount
            Number of input fragments i.e. numberof buffers described by
            FragmentBuffers array. If the caller is querying for the
            number of fragments required then this should be set to 0.
            The function will return the number of fragment required
            with STATUS_BUFFER_OVERFLOW.

        __in ULONG FragmentLength
            Length of each output fragment buffers. All fragment
            buffers needs to be of the same size.
            The length of the last fragment is returned on STATUS_SUCCESS.
            All but the last fragment are of FragmentLength size.
            The last fragment can be lesser than the FragmentLength size.

        __out_ecount_opt(*FragmentBufferCount) PCHAR* FragmentBuffers
            Array of input fragment buffers. If FragmentCount is 0
            FragmentBuffers can be NULL. If FragmentCount is non 0
            FragmentBuffers should be provided.

    Return Value
        STATUS_INVALID_PARAMETER
            One of the required parameters is missing.

        STATUS_SUCCESS
            The Payload was fragmented successfully in to
            FragmentBuffers and the number of fragments used is
            returned in FragmentCount.

        STATUS_MORE_PROCESSING_REQUIRED
            The complete Payload is not fragmented yet.
            This is not a failure code but an indication to the caller that
            more calls are needed with more fragment buffers.
*/

ULONG
MbbLibraryGetNextTransactionId();
