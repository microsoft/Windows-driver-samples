/***************************************************************************

Copyright (c) 2010 Microsoft Corporation

Module Name:

    MbbLibrary.c

Abstract:

    This files contains the implementation of the pack / unpack and
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

////////////////////////////////////////////////////////////////////////////////
//
//  INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include <ndis.h>
#include "tmpMbbMessages.h"
#include "MbbLibrary.h"
//#include "MbbLibrary.tmh"

////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////
#define MIN( _X_, _Y_ ) (((_X_) < (_Y_))? (_X_): (_Y_))

////////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION PROTOTYPES
//
////////////////////////////////////////////////////////////////////////////////
ULONG
MbbLibraryGetNextTransactionId( );


////////////////////////////////////////////////////////////////////////////////
//
//  GLOBALS
//
////////////////////////////////////////////////////////////////////////////////
static ULONG GlobalMbbLibraryTransactionId;


////////////////////////////////////////////////////////////////////////////////
//
//  EXPORTED ROUTINES
//
////////////////////////////////////////////////////////////////////////////////
NTSTATUS
MbbLibForamtBufferAsOpenMessage(
    __out_bcount_opt(*BufferLength) PVOID   MessageBuffer,
    __inout                         PULONG  BufferLength,
    __in                            ULONG   MaximumControlTransfer
    )
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
{
    NTSTATUS            Status  = STATUS_SUCCESS;
    PMBB_OPEN_MESSAGE   Open    = (PMBB_OPEN_MESSAGE)(MessageBuffer);

    do
    {
        if( BufferLength == NULL )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        else if( *BufferLength != 0 && Open == NULL )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        //
        // If the caller is querying for the size or
        // there isnt enough space in the buffer,
        // return the size with the correct status.
        //
        if( *BufferLength < sizeof( MBB_OPEN_MESSAGE ) )
        {
            *BufferLength = sizeof( MBB_OPEN_MESSAGE );
            Status = STATUS_BUFFER_OVERFLOW;
            break;
        }
        //
        // Sufficient space available. Format the input
        // buffer as an open message.
        //
        Open->MessageHeader.MessageType             = MBB_MESSAGE_TYPE_OPEN;
        Open->MessageHeader.MessageLength           = sizeof( MBB_OPEN_MESSAGE );
        Open->MessageHeader.MessageTransactionId    = MbbLibraryGetNextTransactionId( );
        Open->MaximumControlTransfer                = MaximumControlTransfer;
        //
        // Update how much buffer is written.
        //
        *BufferLength = sizeof( MBB_OPEN_MESSAGE );

    } while( FALSE );

    return Status;
}

NTSTATUS
MbbLibForamtBufferAsCloseMessage(
    __out_bcount_opt(*BufferLength) PVOID   MessageBuffer,
    __inout                         PULONG  BufferLength
    )
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
{
    NTSTATUS            Status  = STATUS_SUCCESS;
    PMBB_CLOSE_MESSAGE  Close   = (PMBB_CLOSE_MESSAGE)(MessageBuffer);

    do
    {
        if( BufferLength == NULL )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        else if( *BufferLength != 0 && Close == NULL )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        //
        // If the caller is querying for the size or
        // there isnt enough space in the buffer,
        // return the size with the correct status.
        //
        if( *BufferLength < sizeof( MBB_CLOSE_MESSAGE ) )
        {
            *BufferLength = sizeof( MBB_CLOSE_MESSAGE );
            Status = STATUS_BUFFER_OVERFLOW;
            break;
        }
        //
        // Sufficient space available. Format the input
        // buffer as an open message.
        //
        Close->MessageHeader.MessageType             = MBB_MESSAGE_TYPE_CLOSE;
        Close->MessageHeader.MessageLength           = sizeof( MBB_CLOSE_MESSAGE );
        Close->MessageHeader.MessageTransactionId    = MbbLibraryGetNextTransactionId( );
        //
        // Update how much buffer is written.
        //
        *BufferLength = sizeof( MBB_CLOSE_MESSAGE );

    } while( FALSE );

    return Status;
}

NTSTATUS
MbbLibFragmentBufferToCommandMessages(
    __in PMBB_COMMAND                               Command,
    __in MBB_COMMAND_TYPE                           CommandType,
    __in_bcount_opt(PayloadLength) PCHAR            Payload,
    __in ULONG                                      PayloadLength,
    __inout_opt PULONG                              TransactionId,
    __in ULONG                                      CurrentFragment,
    __inout PULONG                                  FragmentBufferCount,
    __inout PULONG                                  FragmentLength,
    __out_ecount_opt(*FragmentBufferCount) PCHAR*   FragmentBuffers
    )
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
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    PCHAR                           FragmentPayload;
    ULONG                           BufferIndex;
    ULONG                           FirstFragmentPayloadSize;
    ULONG                           OtherFragmentPayloadSize;
    ULONG                           FragmentPayloadLength;
    ULONG                           TotalFragments;
    ULONG                           PayloadOffset;
    PMBB_COMMAND_HEADER             CommandHeader;
    PMBB_COMMAND_FRAGMENT_HEADER    CommandFragmentHeader = NULL;

    do
    {
        //
        // Parameter Validation.
        // Check for required parameters.
        //
        if( (Command == NULL) || (FragmentBufferCount == NULL) || (FragmentLength == 0) ||
            (PayloadLength != 0 && Payload == NULL) ||
            (*FragmentBufferCount != 0 && FragmentBuffers == NULL) )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        //
        // The fragments must be atleast the header size.
        //
        if( *FragmentLength < sizeof(MBB_COMMAND_HEADER) )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        //
        // The size of the payload each fragment can carry.
        //
        FirstFragmentPayloadSize = *FragmentLength - sizeof(MBB_COMMAND_HEADER);
        OtherFragmentPayloadSize = *FragmentLength - sizeof(MBB_COMMAND_FRAGMENT_HEADER);
        //
        // Calculate the number of fragments required.
        // Equation to calculate the fragment count.
        // Key : 
        //  CH = CommandHeader = sizeof(MBB_COMMAND_HEADER)
        //  FH = FragmentHeader = sizeof(MBB_COMMAND_FRAGMENT_HEADER)
        //  PL = PayloadLength
        //  FL = FragmentLength
        //  n  = FragmentBufferCount
        //
        // 1. There is 1 CH and (n-1) FH in n Fragments.
        // 2. So the total length of all fragments would be (CH + (n-1)FH + PL).
        // 3. To round off the result add (FL-FH-1).
        //
        // CH + (n-1)FH + PL + (FL-FH-1) / FL = n
        // n = ( (CH+PL-1) + (FL-2FH) ) / (FL-FH)
        //
        TotalFragments  = (sizeof(MBB_COMMAND_HEADER) + PayloadLength - 1);
        TotalFragments += (*FragmentLength - (2* sizeof(MBB_COMMAND_FRAGMENT_HEADER)));
        TotalFragments /= (*FragmentLength - sizeof(MBB_COMMAND_FRAGMENT_HEADER));
        //
        // Starting fragment cannot be larger than the total fragments.
        //
        if( CurrentFragment >= TotalFragments )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        //
        // If the caller is querying for the size
        // return the required fragment count to the caller.
        //
        if( *FragmentBufferCount == 0 )
        {
            *FragmentBufferCount = (TotalFragments-CurrentFragment);
            Status = STATUS_SUCCESS;
            break;
        }
        //
        // Get a transaction Id if not provided.
        // Only get a new TransactionId if we are starting fragmentation.
        // Do not get a new transaction id while in the middle of fragmentation.
        //
        if( TransactionId == NULL )
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        if( *TransactionId == 0 )
        {
            *TransactionId = MbbLibraryGetNextTransactionId( );
        }
        //
        // PayloadOffset for Fragment #m is equal to amount of data 
        // in (m-1) fragments.
        //
        if( CurrentFragment > 0 )
        {
            PayloadOffset  = FirstFragmentPayloadSize;
            PayloadOffset += (CurrentFragment-1) * OtherFragmentPayloadSize;
        }
        else
        {
            PayloadOffset  = 0;
        }
        //
        // Format the fragment buffers.
        //
        for( BufferIndex = 0;
             BufferIndex < *FragmentBufferCount;
             BufferIndex++ )
        {
            //
            // If all the data is filled dont create anymore fragments.
            // This check is for the case where there are more fragments
            // than are required.
            //
            if( PayloadLength != 0 &&
                PayloadOffset >= PayloadLength )
            {
                break;
            }
            //
            // Common header for all fragments
            //
            CommandFragmentHeader = (PMBB_COMMAND_FRAGMENT_HEADER)FragmentBuffers[BufferIndex];

            CommandFragmentHeader->MessageHeader.MessageType            = MBB_MESSAGE_TYPE_COMMAND;
            CommandFragmentHeader->MessageHeader.MessageTransactionId   = *TransactionId;
            CommandFragmentHeader->FragmentHeader.CurrentFragment       = (USHORT)CurrentFragment;
            CommandFragmentHeader->FragmentHeader.TotalFragments        = (USHORT)TotalFragments;
            //
            // Special handling for the first fragment.
            //
            if( CurrentFragment == 0 )
            {
                CommandHeader = (PMBB_COMMAND_HEADER)FragmentBuffers[BufferIndex];

                MBB_UUID_TO_NET(
                    &CommandHeader->Command.ServiceId,
                    &Command->ServiceId
                    );

                CommandHeader->Command.CommandId        = Command->CommandId;
                CommandHeader->CommandType              = CommandType;
                CommandHeader->InformationBufferLength  = PayloadLength;
                
                FragmentPayload = FragmentBuffers[BufferIndex] + sizeof(MBB_COMMAND_HEADER);
                FragmentPayloadLength = MIN( ( PayloadLength - PayloadOffset ), FirstFragmentPayloadSize );
                CommandFragmentHeader->MessageHeader.MessageLength = FragmentPayloadLength + sizeof(MBB_COMMAND_HEADER);
            }
            else
            {
                FragmentPayload = FragmentBuffers[BufferIndex] + sizeof(MBB_COMMAND_FRAGMENT_HEADER);
                FragmentPayloadLength = MIN( ( PayloadLength - PayloadOffset ), OtherFragmentPayloadSize );
                CommandFragmentHeader->MessageHeader.MessageLength = FragmentPayloadLength + sizeof(MBB_COMMAND_FRAGMENT_HEADER);
            }
            //
            // Copy the actual payload.
            //
            memcpy(
                FragmentPayload,
                Payload + PayloadOffset,
                FragmentPayloadLength
                );
            CurrentFragment++;
            PayloadOffset += FragmentPayloadLength;
        }
        //
        // Update how much data is written.
        // FragmentLength is the length of ONLY the last fragment.
        //
        *FragmentBufferCount  = BufferIndex;
        if (CommandFragmentHeader)
        {
            *FragmentLength = CommandFragmentHeader->MessageHeader.MessageLength;
        }
        //
        // If the complete Payload is not fragmented indicate it to the caller.
        //
        if( CurrentFragment < TotalFragments )
            Status = STATUS_MORE_PROCESSING_REQUIRED;
        else
            Status = STATUS_SUCCESS;

    } while( FALSE );

    return Status;
}

////////////////////////////////////////////////////////////////////////////////
//
//  INTERNAL ROUTINES
//
////////////////////////////////////////////////////////////////////////////////

ULONG
MbbLibraryGetNextTransactionId( )
{
    return InterlockedIncrement( &GlobalMbbLibraryTransactionId );
}
