/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:
	Stream Edit Callout Driver Sample.

	This sample demonstrates Out-of-band (OOB) stream inspection/editing
	via the WFP stream API.

Environment:
	Kernel mode

--*/

#define POOL_ZERO_DOWN_LEVEL_SUPPORT
#include "Trace.h"
#include "StreamEdit.h"
#include "OobEdit.tmh"

#if defined _MODULE_ID
#undef _MODULE_ID
#endif
#define _MODULE_ID  'O'

NTSTATUS
StreamEditInitializeWorkitemPool(
    )
{
/*
    This functions initializes the Light Weight Queue (LW_QUEUE) pool.
*/
    NTSTATUS Status;
    ULONG nCount;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!");

    for (nCount = 0; nCount < NUM_WORKITEM_QUEUES; nCount++)
    {
        NT_ASSERT(Globals.WdmDevice);
        Status = LwInitializeQueue(Globals.WdmDevice, &Globals.ProcessingQueues[nCount],  StreamEditOobPoolWorker);

        if (!NT_SUCCESS(Status)) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "Worker Queue Initialization failed with %!STATUS!", Status);
            break;
        }
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!: Status %!STATUS!", Status);
    return Status;
}

VOID
NTAPI
StreamOobInjectCompletionFn(
    _In_    PVOID Context,
    _Inout_ NET_BUFFER_LIST* NetBufferList,
    _In_    BOOLEAN DispatchLevel
    )
/*
   Injection completion function for injecting an NBL created using
   FwpsAllocateNetBufferAndNetBufferList. This function frees up
   resources allocated during StreamOobReinjectData().
*/
{
    MDL* mdl = (MDL *)Context;

    UNREFERENCED_PARAMETER(DispatchLevel);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "-><- %!FUNC!: NBL %p, Status=%!STATUS!, MDL %p",  NetBufferList, NetBufferList->Status, mdl);

    
    // Supress warning 28922: Redundant test against NULL. Pointer is already guaranteed to be non-NULL.
    // Rationale : mdl is not guaranteed to be non-NULL here... 
    
#pragma prefast(push)
#pragma prefast(disable:28922)
 
    if (mdl != NULL)  
	{
        //
        // The MDL mapped over a pool alloc which we need to free here.
        //
        ExFreePoolWithTag(mdl->MappedSystemVa, STMEDIT_TAG_MDL_DATA);

        IoFreeMdl(mdl);
    }
#pragma prefast(pop)

    FwpsFreeNetBufferList(NetBufferList);
}

VOID
NTAPI
StreamOobCloneInjectCompletionFn(
    _In_ VOID* Context,
    _Inout_ NET_BUFFER_LIST* NetBufferList,
    _In_ BOOLEAN DispatchLevel
    )
/*
   Injection completion function for injecting one of the NBLs cloned
   via FwpsCloneStreamData.

   FwpsCloneStreamData can return a chain of cloned NBLs; each NBL will
   complete separately.
*/
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(DispatchLevel);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "-><- %!FUNC!: NBL %p, Status=%!STATUS!",  NetBufferList, NetBufferList->Status);
    FwpsFreeCloneNetBufferList(NetBufferList, 0);
}

_Requires_lock_held_(TaskEntry->FlowCtx->OobInfo.EditLock)
NTSTATUS
StreamOobQueueUpIncomingData(
    _Inout_ PTASK_ENTRY TaskEntry,
    _Inout_ FWPS_STREAM_DATA* streamData,
	_In_	UINT32 flags
    )
/*
   This function clones the indicated stream data into a NBL chain and
   saves it in the task entry, along with other information (Flags etc.)

   This function assumes that the OobInfo lock inside the flow's context
   is being held.
*/
{
    NTSTATUS Status;
    NET_BUFFER_LIST* ClonedNbl;
    PSTREAM_FLOW_CONTEXT FlowCtx = TaskEntry->FlowCtx;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!: FlowCtx %p, Task %p, StreamData %p",
                        FlowCtx,
                        TaskEntry,
                        streamData);

    Status = FwpsCloneStreamData(streamData, NULL, NULL, 0, &ClonedNbl);
    if (NT_SUCCESS(Status))
    {
        PLW_QUEUE Queue;
        ULONG Count;
        TaskEntry->StreamFlags = streamData->flags;
        TaskEntry->NetBufferList = ClonedNbl;
        TaskEntry->DataLength = streamData->dataLength;

        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
                "NBL Chain @ %p was Cloned as %p and added to Task %p",
                    streamData->netBufferListChain, ClonedNbl, TaskEntry);

        Count = InterlockedIncrement(& (LONG)FlowCtx->OobInfo.PendingTasks);
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_REFCOUNT,
                "FlowCtx %p, QueueIncoming : PendingTasks @++ = %lu", FlowCtx, Count);

        // Reference the flow for task
        StmEditReferenceFlow(FlowCtx, _MODULE_ID, __LINE__);

        Queue = &(Globals.ProcessingQueues[FlowCtx->OobInfo.QueueNumber]);
        LwEnqueue(Queue , &TaskEntry->LwQLink);

		if (flags & FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA)
		{
			FlowCtx->bNoMoreData = TRUE;

			DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
				"FlowCtx %p, NO_MORE_DATA flag set, bNoMoreData = TRUE.", FlowCtx);
		}

        //
        // TCP FIN is indicated by an empty NBL with disconnect flag set and
        // does not contain any data.
        //

        if ((streamData->flags & FWPS_STREAM_FLAG_SEND_DISCONNECT) ||
            (streamData->flags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT))
        {
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "Received FIN for FlowCtx %p in Task %p", FlowCtx, TaskEntry);

            NT_ASSERT(FlowCtx->bNoMoreData);
            NT_ASSERT(streamData->dataLength == 0);

            FlowCtx->bFlowTerminating = TRUE;
        }

        InterlockedAdd( (LONG *)&FlowCtx->OobInfo.PendedDataLength, (LONG)streamData->dataLength);
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "<-- %!FUNC!: FlowCtx %p, Pending %Iu, %!STATUS!", FlowCtx, FlowCtx->OobInfo.PendedDataLength, Status);
    return Status;
}

_Requires_lock_not_held_(FlowContext->OobInfo.EditLock)
NTSTATUS
StreamOobQueueUpOutgoingData(
    _Inout_ PSTREAM_FLOW_CONTEXT FlowContext,
    _Inout_ PNET_BUFFER_LIST NetBufferList,
    _In_ BOOLEAN IsClone,
    _In_ size_t DataLength,
    _In_ UINT32 StreamFlags,
    _In_opt_ MDL* Mdl
    )
/*
   This function queues up processed data (either sections of the indicated
   data or newly created data) such that they can be (re-)injected back to
   the data stream during the following context.

   1. Before FWP_ACTION_BLOCK is returned from the ClassifyFn, or
   2. After EOF is indicated.

   Under the conditions above, the incoming data (which we pend) and the
   outgoing data (which we (re-)inject) can be synchronized properly).
*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    KLOCK_QUEUE_HANDLE LockHandle;
    POUTGOING_STREAM_DATA OutgoingData;

    NT_ASSERT(0 != StreamFlags);
    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "--> %!FUNC!: FlowCtx %p, DataLength %Iu, sFlags %#x",
                        FlowContext, DataLength, StreamFlags);

    OutgoingData = (POUTGOING_STREAM_DATA)ExAllocateFromLookasideListEx(&Globals.LookasideList);

    if (OutgoingData == NULL) 
	{
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    OutgoingData->NetBufferList = NetBufferList;
    OutgoingData->isClone = IsClone;
    OutgoingData->DataLength = DataLength;
    OutgoingData->StreamFlags = StreamFlags;
    OutgoingData->Mdl = Mdl;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
            "OutgoingStreamData %p has NBL %p (clone %d) with Length of %Iu, sFlags %#x, mdl %p",
                OutgoingData, NetBufferList, IsClone, DataLength, StreamFlags, Mdl);

    KeAcquireInStackQueuedSpinLock(&FlowContext->OobInfo.EditLock, &LockHandle);
    InsertTailList(&FlowContext->OobInfo.OutgoingDataQueue, &OutgoingData->Link);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,"<-- %!FUNC!: %!STATUS!", Status);
    return Status;
}

NTSTATUS
StreamOobFlushOutgoingData(
    _Inout_ PSTREAM_FLOW_CONTEXT FlowContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    KLOCK_QUEUE_HANDLE LockHandle;
    OUTGOING_STREAM_DATA* OutgoingData = NULL;
    BOOLEAN FlowInError = FALSE;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!: FlowCtx %p", FlowContext);

    for (;;)
    {
        KeAcquireInStackQueuedSpinLock(&FlowContext->OobInfo.EditLock, &LockHandle);

        if (!IsListEmpty(&FlowContext->OobInfo.OutgoingDataQueue)) 
		{
            LIST_ENTRY* Entry = RemoveHeadList(&FlowContext->OobInfo.OutgoingDataQueue);
            OutgoingData = CONTAINING_RECORD(Entry, OUTGOING_STREAM_DATA, Link);

            FlowInError = (OOB_EDIT_ERROR == FlowContext->OobInfo.EditState);
        }
        KeReleaseInStackQueuedSpinLock(&LockHandle);

        if (OutgoingData == NULL || FlowInError)
		{
            break;
        }

        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                "Oob Flush Outgoing: OutgoingStreamData %p, Injecting NBL %p, Data Length %Iu, Stream Flags %#x",
                    OutgoingData,
                    OutgoingData->NetBufferList,
                    OutgoingData->DataLength,
                    OutgoingData->StreamFlags);

        Status = FwpsStreamInjectAsync(
                        Globals.InjectionHandle,
                        NULL,
                        0,
                        FlowContext->FlowHandle,
                        FlowContext->CalloutId,
                        FlowContext->LayerId,
                        OutgoingData->StreamFlags,
                        OutgoingData->NetBufferList,
                        OutgoingData->DataLength,
                        OutgoingData->isClone ? StreamOobCloneInjectCompletionFn : StreamOobInjectCompletionFn,
                        OutgoingData->Mdl
                    );

        if (!NT_SUCCESS(Status)) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL,
                    "OobFlushOutgoingData: FwpsStreamInjectAsync() failed with Status %!STATUS!", Status);
            break;
        }

        ExFreeToLookasideListEx(&Globals.LookasideList, OutgoingData);
        OutgoingData = NULL;
    }

    if (!NT_SUCCESS(Status) || FlowInError)
    {
        while (OutgoingData != NULL)
        {
            if (OutgoingData->isClone) 
			{
                FwpsDiscardClonedStreamData(OutgoingData->NetBufferList, 0, FALSE);
            }
            else
            {
                // Invoke the injection completion routine to free the resources
                StreamOobInjectCompletionFn(OutgoingData->Mdl, OutgoingData->NetBufferList, FALSE);
            }

            ExFreeToLookasideListEx(&Globals.LookasideList, OutgoingData);
            OutgoingData = NULL;

            // Free up any queued data.
            //
            KeAcquireInStackQueuedSpinLock(&FlowContext->OobInfo.EditLock, &LockHandle);

            if (!IsListEmpty(&FlowContext->OobInfo.OutgoingDataQueue)) 
			{
                LIST_ENTRY* Entry = RemoveHeadList(&FlowContext->OobInfo.OutgoingDataQueue);
                OutgoingData = CONTAINING_RECORD(Entry, OUTGOING_STREAM_DATA, Link);
            }
            KeReleaseInStackQueuedSpinLock(&LockHandle);
        }
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "<-- %!FUNC!: FlowCtx %p, FlowInError %d, Status %!STATUS!",
                    FlowContext, FlowInError, Status);
    return Status;
}

_Requires_lock_not_held_(FlowContext->OobInfo.EditLock)
NTSTATUS
StreamOobReinjectData(
    _In_ STREAM_FLOW_CONTEXT* FlowContext,
    _In_ const PVOID Data,
    _In_ size_t Length,
    _In_ UINT32 StreamFlags
    )
/*
   This function injects a section of the original indicated data back
   to the data stream.

   An MDL is allocated to describe the data section.
*/
{
    NTSTATUS Status;

    VOID* DataCopy = NULL;
    MDL* mdl = NULL;
    NET_BUFFER_LIST* NetBufferList = NULL;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "--> %!FUNC!: FlowCtx %p, Length %Iu, sFlags 0x%x", FlowContext, Length, StreamFlags);

    NT_ASSERT(StreamFlags);
    NT_ASSERT(!(StreamFlags & FWPS_STREAM_FLAG_SEND_DISCONNECT) &&
              !(StreamFlags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT));
    NT_ASSERT(Length);

    do
    {
        DataCopy = ExAllocatePoolZero(
                        NonPagedPool,
                        Length,
                        STMEDIT_TAG_MDL_DATA
                        );

        if (DataCopy == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "Failed to allocate memory.");
            break;
        }

        RtlCopyMemory(DataCopy, Data, Length);

        mdl = IoAllocateMdl(
                    DataCopy,
                    (ULONG)Length,
                    FALSE,
                    FALSE,
                    NULL);

        if (mdl == NULL)
		{
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "Failed to allocate MDL for re-injection.");
            break;
        }
        MmBuildMdlForNonPagedPool(mdl);

        Status = FwpsAllocateNetBufferAndNetBufferList(
                        Globals.NetBufferListPool,
                        0,
                        0,
                        mdl,
                        0,
                        Length,
                        &NetBufferList
                        );

        if (!NT_SUCCESS(Status)) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "AllocateNetBufferAndNetBufferList failed with %!STATUS!\n", Status);
            break;
        }

        Status = StreamOobQueueUpOutgoingData(
                        FlowContext,
                        NetBufferList,
                        FALSE,
                        Length,
                        StreamFlags,
                        mdl
                        );

        if (NT_SUCCESS(Status))
        {
            DataCopy = NULL;
            mdl = NULL;
            NetBufferList = NULL;
        }
    } while (FALSE);

    if (!NT_SUCCESS(Status))
    {
        if (NetBufferList != NULL) 
		{
            FwpsFreeNetBufferList(NetBufferList);
        }

        if (mdl != NULL) 
		{
            IoFreeMdl(mdl);
        }

        if (DataCopy != NULL) 
		{
            ExFreePoolWithTag(DataCopy, STMEDIT_TAG_MDL_DATA );
        }
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "<-- %!FUNC!: FlowCtx %p, %!STATUS!",  FlowContext, Status);

    return Status;
}

_Requires_lock_not_held_(FlowContext->OobInfo.EditLock)
NTSTATUS
StreamOobInjectReplacement(
    _In_ STREAM_FLOW_CONTEXT* FlowContext,
    _In_ UINT32 StreamFlags,
    _In_ MDL* Mdl, // Optional
    _In_ size_t Length
    )
/*
   This function injects a section of replacement data (in place of data
   removed from the stream) into the data stream.

   The MDL describing the replacement data is allocated during DriverEntry
   and does not need to be freed during injection completion.
*/
{
    NTSTATUS Status;
    NET_BUFFER_LIST* NetBufferList = NULL;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "--> %!FUNC!: FlowCtx %p, sFlags %#x, Length %Iu",  FlowContext, StreamFlags, Length);

    Status = FwpsAllocateNetBufferAndNetBufferList(
                            Globals.NetBufferListPool,
                            0,
                            0,
                            Mdl,
                            0,
                            Length,
                            &NetBufferList
                            );

    if (NT_SUCCESS(Status))
    {
        NT_ASSERT(!(StreamFlags & FWPS_STREAM_FLAG_SEND_DISCONNECT) &&
                  !(StreamFlags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT));

        Status = StreamOobQueueUpOutgoingData(
                            FlowContext,
                            NetBufferList,
                            FALSE,
                            Length,
                            StreamFlags,
                            NULL
                            );

        if (NT_SUCCESS(Status)) 
		{
            NetBufferList = NULL;
        }
    }

    if (NetBufferList)
    {
        FwpsFreeNetBufferList(NetBufferList);
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!: FlowCtx %p, %!STATUS!",  FlowContext, Status);

    return Status;
}

FORCEINLINE
NTSTATUS
StreamOobCopyDataToFlatBuffer(
    _Inout_ PTASK_ENTRY TaskEntry
    )
/*
   This function copies the data described by NBL(s) into a flat buffer for easy searching.

   It reuses the FwpsCopyStreamDataToBuffer API (via StreamEditCopyDataForInspection)
   by creating a FWPS_STREAM_DATA struct.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    FWPS_STREAM_DATA streamData;

    if (TaskEntry->DataLength > 0)
    {
        streamData.netBufferListChain = TaskEntry->NetBufferList;
        streamData.dataLength = TaskEntry->DataLength;
        streamData.flags = TaskEntry->StreamFlags;

        streamData.dataOffset.netBufferList = TaskEntry->NetBufferList;
        streamData.dataOffset.netBuffer = NET_BUFFER_LIST_FIRST_NB(streamData.dataOffset.netBufferList);
        streamData.dataOffset.mdl = NET_BUFFER_CURRENT_MDL(streamData.dataOffset.netBuffer);
        streamData.dataOffset.mdlOffset = NET_BUFFER_CURRENT_MDL_OFFSET(streamData.dataOffset.netBuffer);

        streamData.dataOffset.netBufferOffset  = 0;
        streamData.dataOffset.streamDataOffset = 0;

        if (!StreamEditCopyDataForInspection(TaskEntry->FlowCtx, &streamData, streamData.dataLength)) 
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return Status;
}

_Requires_lock_not_held_(TaskEntry->FlowCtx->OobInfo.EditLock)
NTSTATUS
StreamOobEditData(
_Inout_ PTASK_ENTRY TaskEntry
)
/*
    This function processes Stream data in "Out of Band" processing, looking
    for pattern  matches, and replacing them.

    It first copies the stream data into a flat inspection buffer;
    it then parses the buffer looking for the matching pattern. For
    non-matching sections it re-injects the data back; for a match, it skips
    over and injects a replacement pattern.

    If a match can not be determined due to lack of data, it injects the
    non-matching section back and moves the potential (or partial) match
    to the beginning of the (flattened) inspection buffer.

    If a FIN is presented by the NetBufferList, it flushes all processed stream
    sections back and re-injects the FIN back at the end the stream.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSTREAM_FLOW_CONTEXT FlowContext = TaskEntry->FlowCtx;
    UINT i;
    BOOLEAN bStreamModified, bPartialMatch;
    BYTE* dataStart;


    size_t BytesToProcess = 0;     // # of bytes to be processed in one loop!
    size_t ProcessedBytes = 0;     // # of bytes of original data actually processed/consumed
    size_t BytesRemaining = TaskEntry->DataLength; // # of bytes of original data remaining to be processed.

    FWPS_STREAM_DATA StreamData;  // Local StreamData struct for keeping track of stream data offset
    PNET_BUFFER_LIST TheNbl;      // The NBL to be processed next in the StreamData's netBufferListChain

    BOOLEAN bIsLastNbl;           // True if last NBL of the chain is being processed
    BOOLEAN bProcessingPartialTask = FALSE; // True if we are only processing a part of current task data

	NT_ASSERT(TaskEntry->NetBufferList != NULL);

	DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
		"--> %!FUNC!: FlowCtx %p, Task %p, NBL Chain @ %p, Length %Iu, sFlags %#x, Pended Data %Iu",
		FlowContext,
		TaskEntry,
		TaskEntry->NetBufferList,
		TaskEntry->DataLength,
		TaskEntry->StreamFlags,
		FlowContext->OobInfo.PendedDataLength);
	
    StreamData.dataLength = TaskEntry->DataLength;
    StreamData.netBufferListChain = TheNbl = TaskEntry->NetBufferList;
    StreamData.flags = TaskEntry->StreamFlags;
    StreamData.dataOffset.streamDataOffset = 0;

    while (BytesRemaining)
    {
		bStreamModified = bPartialMatch = FALSE;

        StreamData.dataOffset.netBufferList = TheNbl;

        StreamData.dataOffset.netBuffer = NET_BUFFER_LIST_FIRST_NB(TheNbl);
        StreamData.dataOffset.netBufferOffset = NET_BUFFER_CURRENT_MDL_OFFSET(StreamData.dataOffset.netBuffer);

        StreamData.dataOffset.mdl = NET_BUFFER_CURRENT_MDL(StreamData.dataOffset.netBuffer);
        StreamData.dataOffset.mdlOffset = NET_BUFFER_CURRENT_MDL_OFFSET(StreamData.dataOffset.netBuffer);

        StreamData.dataOffset.streamDataOffset += BytesToProcess;

        // Presume we are going to process all data.
        BytesToProcess = BytesRemaining;
        bIsLastNbl = TRUE;

        // However, if we have more data than we can handle, lets try to consume it in chunks!
        //
        if (BytesRemaining + FlowContext->ScratchDataLength > Globals.BusyThreshold)
        {

            // Process ONE NBL at a time...
            //
            // It is possible that the length of NBL being processed is more than the
            // BusyThreshold limit (though, very unlikely for a realistic BusyThreshold).
            //
            // The code can be modified to handle such situations -- i.e. to process a
            // partial NBL or to process multiple NBLs (or a combination thereof ) at a
            // time by appropriately setting StreamData.dataOffset.
            //
            // This is left as an exercise for the user.
            //

            BytesToProcess = NetBufferListLength(TheNbl);

            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                    "Partially processing task %p - Now Processing NBL %p with %Iu bytes",
                        TaskEntry, TheNbl, BytesToProcess);

            TheNbl = TheNbl->Next;

            if (TheNbl != NULL)
                bIsLastNbl = FALSE;

            bProcessingPartialTask = TRUE;

            if (BytesToProcess == 0)
            {
                NT_ASSERT(TheNbl != NULL);
                continue;
            }
        }


        if (!StreamEditCopyDataForInspection(TaskEntry->FlowCtx, &StreamData, BytesToProcess))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                    "FlowCtx %p, Task %p - CopyDataToFlatBuffer failed with status %#x",
                            FlowContext, TaskEntry, Status);

            goto Exit;
        }


        /*
            Search for a pattern through the "flattened" buffer.

            If we find a full match, inject the (if any) data before the match into stream,
            followed by the injection of replacement pattern.

            If we find a partial match (at the end of flat buffer), then inject the data
            before the pattern, and move the partial pattern at the beginning of the ScratchBuffer.
            Next time (if) we get more data, we can try for a full match.
        */

        NT_ASSERT(FlowContext->ScratchDataOffset == 0);
        dataStart = (BYTE*)FlowContext->ScratchBuffer;

        ProcessedBytes = FlowContext->ScratchDataLength;

        for (i = 0; i < FlowContext->ScratchDataLength; ++i)
        {
            // If there is enough data on which to perform a search
            //
            if (i + Globals.StringToFindLength <= FlowContext->ScratchDataLength)
            {
                if (RtlCompareMemory(
                            dataStart + i,
                            Globals.StringToFind,
                            Globals.StringToFindLength
                       ) == Globals.StringToFindLength)
                {
                    // If the match is not at the beginning of the data,
                    // inject back the data before the match
                    //
                    if (i != 0)
                    {
                        Status = StreamOobReinjectData(FlowContext, dataStart, i, TaskEntry->StreamFlags);
                        if (!NT_SUCCESS(Status)) 
						{
                            goto Exit;
                        }

                        FlowContext->ScratchDataOffset += i;
                        FlowContext->ScratchDataLength -= i;
                        i = 0;
                    }

                    // Now inject the replacement string in place of the match (Globals.StringToFind)!
                    //
                    Status = StreamOobInjectReplacement(
                                    FlowContext,
                                    TaskEntry->StreamFlags,
                                    Globals.StringXMdl,
                                    Globals.StringXLength
                                    );

                    if (!NT_SUCCESS(Status)) 
					{
                        goto Exit;
                    }

                    FlowContext->ScratchDataOffset += Globals.StringToFindLength;
                    FlowContext->ScratchDataLength -= Globals.StringToFindLength;

                    bStreamModified = TRUE;

                    // Still more data to be searched for the match
                    //
                    if (FlowContext->ScratchDataLength > 0) 
					{
                        dataStart = (BYTE*)FlowContext->ScratchBuffer + FlowContext->ScratchDataOffset;
                        --i;

                        continue;
                    }
                    else {
                        FlowContext->ScratchDataOffset = 0;
                    }
                }
            }
            else // If we do not have enough data to perform a search on
            {
                // If we do not expect more data to come in, get out...
                // 1 == FlowContext->OobInfo.RefCount ==> This is the last
                // (data-processing) Task being processed for the flow
                //
                if (bIsLastNbl && FlowContext->bNoMoreData && (0 == FlowContext->OobInfo.PendingTasks)) 
				{
					DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
						"FlowCtx %p -> giving up on partial match search - offset %lu, scratch length %Iu",
						FlowContext, i, FlowContext->ScratchDataLength);

                    break;
                }

                // Look for a partial match!
                // If we find a partial match, move the partially matching pattern
                // to the beginning of ScratchBuffer... when more data comes in,
                // we'll try a complete match again.
                //
                if (RtlCompareMemory(dataStart + i, Globals.StringToFind, FlowContext->ScratchDataLength - i)
                    == FlowContext->ScratchDataLength - i)
                {
                    bPartialMatch = TRUE;  // This is a partial find

                    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                        "FlowCtx %p -> partial match @ offset %lu, match length %Iu",
                        FlowContext, i, (FlowContext->ScratchDataLength - i));

                    if (i != 0) 
					{
                        // Inject any data before partial match back into the stream
                        //
                        Status = StreamOobReinjectData(
                                        FlowContext,
                                        dataStart,
                                        i,
                                        TaskEntry->StreamFlags
                                     );

                        if (!NT_SUCCESS(Status)) 
						{
                            goto Exit;
                        }
                    }
                    // Move the partially matching bytes to the beginning of scratch buffer
                    //
                    RtlMoveMemory((BYTE*)FlowContext->ScratchBuffer, dataStart + i, FlowContext->ScratchDataLength - i);
                    FlowContext->PartialSFlags = TaskEntry->StreamFlags;

                    FlowContext->ScratchDataOffset = 0;
                    FlowContext->ScratchDataLength -= i;

                    break;
                }
            }
        }

        //
        // At this point we should fall into one of the following :
        // case 1: We found a partial match (irrespective of whether a full match was
        // found before it or not). Data before the partial match has already
        // been injected; we do not need to inject any more data.
        //
        // case 2: We found a full match (but no partial match).
        // Data before the match has been taken care of; if there is any data
        // left after the matching token, inject it.
        //
        // case 3: No full or partial match was found.
        // subcase 1: There is data left in  the  ScratchBuffer  from  a previous
        // partial match:
        // inject "data left from previous match"
        // . . . then the current NBL (subcase 2 below)
        //
        // subcase 2: There is no data left from previous partial match
        // (i.e. all data is what was in the indicated NBL Chain) :
        // inject the indicated NetBufferList Chain...
        //
        // Injecting (original/indicated) NBLs will save us from having to
        // allocate memory and unnecessary processing  to create new NBL.
        //

        if (bPartialMatch)
        {
            // case 1. nothing to be done!
            //
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                "FlowCtx %p: Partial match, nothing to reinject, %Iu bytes left...",
                FlowContext, FlowContext->ScratchDataLength);
        }
        else if (bStreamModified)
        {
            // case 2. We found a match. Reinject data after matching token
            //
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                "FlowCtx %p: Full match, reinjecting %Iu remaining bytes",
                FlowContext, FlowContext->ScratchDataLength);

            if (FlowContext->ScratchDataLength > 0)
            {
                Status = StreamOobReinjectData(
                                FlowContext,
                                dataStart,
                                FlowContext->ScratchDataLength,
                                TaskEntry->StreamFlags
                           );

                if (!NT_SUCCESS(Status)) 
				{
                    goto Exit;
                }

                FlowContext->ScratchDataOffset = FlowContext->ScratchDataLength = 0;
            }
        }
        else
        {
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                        "FlowCtx %p: No match, reinjecting %Iu+%Iu bytes",
                                    FlowContext,
                                    (FlowContext->ScratchDataLength - BytesToProcess),
                                    BytesToProcess);

            if (!bProcessingPartialTask)
            {
                NT_ASSERT(TaskEntry->DataLength == BytesToProcess);

                if (FlowContext->ScratchDataLength > BytesToProcess)
                {
                    // Case 3/subcase 1: We have partial data left in the scratch buffer.
                    //
                    NT_ASSERT(FlowContext->ScratchBuffer == dataStart);
                    NT_ASSERT(FlowContext->ScratchDataOffset == 0);

                    Status = StreamOobReinjectData(
                                    FlowContext,
                                    dataStart,
                                    (FlowContext->ScratchDataLength - BytesToProcess),
                                    FlowContext->PartialSFlags
                                );

                    if (!NT_SUCCESS(Status)) 
					{
                        goto Exit;
                    }

                    FlowContext->ScratchDataLength = BytesToProcess;
                }

                // case 3/subcase 2: Inject the rest of the (original) data.
                //
                if (TaskEntry->DataLength)
                {
                    NT_ASSERT(TaskEntry->DataLength == FlowContext->ScratchDataLength);

                    Status = StreamOobQueueUpOutgoingData(
                                    FlowContext,
                                    TaskEntry->NetBufferList,
                                    TRUE,
                                    TaskEntry->DataLength,
                                    TaskEntry->StreamFlags,
                                    NULL
                                 );

                    if (!NT_SUCCESS(Status)) 
					{
                        goto Exit;
                    }

                    TaskEntry->NetBufferList = NULL;
                }
            }
            else
            {
                NT_ASSERT(FlowContext->ScratchBuffer == dataStart);
                NT_ASSERT(FlowContext->ScratchDataOffset == 0);

                Status = StreamOobReinjectData(
                                FlowContext,
                                FlowContext->ScratchBuffer,
                                FlowContext->ScratchDataLength,
                                TaskEntry->StreamFlags
                            );

                if (!NT_SUCCESS(Status)) 
				{
                    goto Exit;
                }
            }

            FlowContext->ScratchDataOffset = FlowContext->ScratchDataLength = 0;
        }

        ProcessedBytes -= FlowContext->ScratchDataLength;

        NT_ASSERT((signed)ProcessedBytes >= 0);
        InterlockedAdd((LONG *)&FlowContext->OobInfo.PendedDataLength, -(signed)ProcessedBytes);


        BytesRemaining -= BytesToProcess;

		NT_ASSERT(FlowContext->ScratchDataLength < Globals.StringToFindLength);

    } // while (BytesRemaining);

	  // If we received a FIN and there are left overs in ScratchBuffer,
	  // let us inject it before injecting the FIN.
	  //
	if ((TRUE == FlowContext->bFlowTerminating) &&
		(0 == TaskEntry->DataLength))
	{
		NT_ASSERT(FlowContext->OobInfo.PendedDataLength == FlowContext->ScratchDataLength);

		if (FlowContext->OobInfo.PendedDataLength)
		{
			NT_ASSERT(FlowContext->ScratchDataOffset == 0);

			Status = StreamOobReinjectData(FlowContext,
				FlowContext->ScratchBuffer,
				FlowContext->ScratchDataLength,
				FlowContext->PartialSFlags);

			if (!NT_SUCCESS(Status)) 
			{
				goto Exit;
			}

			// InterlockedExchangeSubtract can be used as well!
			//
			InterlockedAdd((LONG *)&FlowContext->OobInfo.PendedDataLength, -(signed)FlowContext->ScratchDataLength);
			FlowContext->ScratchDataOffset = FlowContext->ScratchDataLength = 0;
		}
	}


    //
    // Its a good time to reinject the processed data back into the stream
    //
    Status = StreamOobFlushOutgoingData(FlowContext);

    if (!NT_SUCCESS(Status)) 
	{
        goto Exit;
    }

    // FIN/RST :: inject FIN after flushing the processed stream data
    //
    if ( (TRUE == FlowContext->bFlowTerminating) &&
         (0 == TaskEntry->DataLength) )
    {
        NT_ASSERT(TRUE == FlowContext->bNoMoreData);
        NT_ASSERT(FlowContext->OobInfo.PendedDataLength == 0);

        NT_ASSERT(  TaskEntry->StreamFlags & FWPS_STREAM_FLAG_SEND_DISCONNECT ||
                    TaskEntry->StreamFlags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT);

        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "Now injecting FIN for FlowCtx %p", FlowContext);
        Status = FwpsStreamInjectAsync(
                            Globals.InjectionHandle,
                            NULL,
                            0,
                            FlowContext->FlowHandle,
                            FlowContext->CalloutId,
                            FlowContext->LayerId,
                            (DWORD)TaskEntry->StreamFlags,
                            TaskEntry->NetBufferList,
                            0,
                            StreamOobCloneInjectCompletionFn,
                            NULL
                        );

        if (!NT_SUCCESS(Status)) 
		{
            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FIN injection for FlowCtx %p failed with %!STATUS!", FlowContext, Status);
            goto Exit;
        }

        TaskEntry->NetBufferList = NULL;
    }

Exit:

    if (TaskEntry->NetBufferList != NULL) 
	{
        FwpsDiscardClonedStreamData(TaskEntry->NetBufferList, 0, FALSE);
        TaskEntry->NetBufferList = NULL;
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!: FlowCtx %p, %!STATUS!",  FlowContext, Status);
    return Status;
}

VOID
StreamEditOobProcessTask(
    TASK_ENTRY *TaskEntry
    )
/*
    This function processes a Task Entry data and resume
    a paused stream if conditions are right.
*/
{
    KLOCK_QUEUE_HANDLE LockHandle;
    NTSTATUS Status = STATUS_SUCCESS;
    STREAM_FLOW_CONTEXT* FlowCtx = TaskEntry->FlowCtx;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "--> %!FUNC!: Processing Task %p for FlowCtx %p",  TaskEntry, FlowCtx);

    do
    {
        KeAcquireInStackQueuedSpinLock(&FlowCtx->OobInfo.EditLock, &LockHandle);

        if (OOB_EDIT_ERROR == FlowCtx->OobInfo.EditState)
        {
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "TaskProc: Flow in Error/Shutdown (%d) state", FlowCtx->OobInfo.EditState);
            KeReleaseInStackQueuedSpinLock(&LockHandle);
            break;
        }

        KeReleaseInStackQueuedSpinLock(&LockHandle);

        Status = StreamOobEditData(TaskEntry);

        if (!NT_SUCCESS(Status)) 
		{
            KeAcquireInStackQueuedSpinLock(&FlowCtx->OobInfo.EditLock, &LockHandle);
            FlowCtx->OobInfo.EditState = OOB_EDIT_ERROR;
            KeReleaseInStackQueuedSpinLock(&LockHandle);
            break;
        }

        // Wait till we have some room to process more data
        //
        if (FlowCtx->OobInfo.PendedDataLength <= (Globals.BusyThreshold >> 2))
        {
            BOOLEAN bStreamPaused;

            KeAcquireInStackQueuedSpinLock(&FlowCtx->OobInfo.EditLock, &LockHandle);
            bStreamPaused = (OOB_EDIT_BUSY == FlowCtx->OobInfo.EditState);

            if (FlowCtx->OobInfo.PendedDataLength < Globals.StringToFindLength)
                FlowCtx->OobInfo.EditState = OOB_EDIT_IDLE;
            else
                if (FlowCtx->OobInfo.EditState == OOB_EDIT_BUSY)
                    FlowCtx->OobInfo.EditState = OOB_EDIT_PROCESSING;

            KeReleaseInStackQueuedSpinLock(&LockHandle);

            if (bStreamPaused)
            {
                NTSTATUS ContinueStatus;
                DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                    "TaskProc: Calling FwpsStreamContinue. PendedLength %Iu, new State %!OobState!",
                        FlowCtx->OobInfo.PendedDataLength,
                        FlowCtx->OobInfo.EditState);

                ContinueStatus = FwpsStreamContinue(
                                        FlowCtx->FlowHandle,
                                        FlowCtx->CalloutId,
                                        FlowCtx->LayerId,
                                        FlowCtx->OobInfo.StreamFlags
                                        );

                if (!NT_SUCCESS(ContinueStatus))
                {
                    DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "TaskProc: FwpsStreamContinue() returns with %!STATUS!", ContinueStatus);

                    KeAcquireInStackQueuedSpinLock(&FlowCtx->OobInfo.EditLock, &LockHandle);
                    FlowCtx->OobInfo.EditState = OOB_EDIT_ERROR;
                    KeReleaseInStackQueuedSpinLock(&LockHandle);

                    break;
                }
            }
        }
    } while (FALSE);

    // DeRef the FlowContext for completed task
    StmEditDeReferenceFlow(TaskEntry->FlowCtx, _MODULE_ID, __LINE__);

    KeAcquireInStackQueuedSpinLock(&FlowCtx->OobInfo.EditLock, &LockHandle);
    if (OOB_EDIT_ERROR == FlowCtx->OobInfo.EditState)
    {
        KeReleaseInStackQueuedSpinLock(&LockHandle);

#if (NTDDI_VERSION >= NTDDI_WIN8)
        (VOID)StreamOobFlushOutgoingData(FlowCtx);
        Status = FwpsFlowAbort(FlowCtx->FlowHandle);

        if (!NT_SUCCESS(Status)) 
		{
           (VOID) FwpsFlowRemoveContext(FlowCtx->FlowHandle, FlowCtx->LayerId, FlowCtx->CalloutId);
        }
#else
        (VOID) StreamEditRemoveFlowCtx(FlowCtx);
#endif

    }
    else
    {
        KeReleaseInStackQueuedSpinLock(&LockHandle);
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "<-- %!FUNC!: Task %p for FlowCtx %p (Ref %lu, Pending %lu), State %!OobState!",
            TaskEntry, FlowCtx, FlowCtx->RefCount, FlowCtx->OobInfo.PendingTasks, FlowCtx->OobInfo.EditState);
}


__drv_functionClass(IO_WORKITEM_ROUTINE)
__drv_requiresIRQL(PASSIVE_LEVEL)
__drv_sameIRQL
VOID
StreamEditOobPoolWorker(
    _In_ PDEVICE_OBJECT DevObj,
    _In_opt_ PVOID Context
    )
{
    PLW_ENTRY LinkEntry = (PLW_ENTRY)Context;
    PTASK_ENTRY TaskEntry;
    ULONG Count;

    UNREFERENCED_PARAMETER(DevObj);

    NT_ASSERT(LinkEntry != NULL);

    while (LinkEntry)
    {
        TaskEntry = CONTAINING_RECORD(LinkEntry, TASK_ENTRY, LwQLink);
        NT_ASSERT(TaskEntry);
        LinkEntry = LinkEntry->Next;

        //
        // Process the task.
        //
        Count = InterlockedDecrement(& (LONG)TaskEntry->FlowCtx->OobInfo.PendingTasks);
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_REFCOUNT,
            "FlowCtx %p, ProcessingTask %p: PendingTasks @-- = %lu", TaskEntry->FlowCtx, TaskEntry, Count);

        StreamEditOobProcessTask(TaskEntry);

        //
        // Free the task Memory.
        //
        ExFreeToLookasideListEx(&Globals.LookasideList, TaskEntry);
    }

    return;
}

VOID
OobEditClassify(
    _In_ const FWPS_INCOMING_VALUES* InFixedValues,
    _In_ const FWPS_INCOMING_METADATA_VALUES* InMetaValues,
    _Inout_ VOID* LayerData,
    _In_ const FWPS_FILTER* Filter,
    _In_ UINT64 InFlowContext,
    _Inout_ FWPS_CLASSIFY_OUT* ClassifyOut
    )
/*
   This is the ClassifyFn function registered by the OOB stream edit callout.

   An OOB stream modification callout blocks all indicated data after cloning
   them for processing by a kernel mode worker thread (or marshalling the data
   to user mode for inspection); the resultant/edited data will then be put
   back to the stream via the stream injection API.

*/
{
    FWPS_STREAM_CALLOUT_IO_PACKET* ioPacket;
    FWPS_STREAM_DATA* streamData;

    PTASK_ENTRY TaskEntry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    KLOCK_QUEUE_HANDLE LockHandle;
    STREAM_FLOW_CONTEXT* FlowContext = (STREAM_FLOW_CONTEXT *)(ULONG_PTR)InFlowContext;

    ioPacket = (FWPS_STREAM_CALLOUT_IO_PACKET*)LayerData;
    NT_ASSERT(ioPacket != NULL);

    streamData = ioPacket->streamData;
    NT_ASSERT(streamData != NULL);

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
            "--> %!FUNC!: FlowCtx %p, sFlags %#x, Length %Iu, LayerId %hu, CalloutId %u, FlowId %I64u, cFlags %#x",
                    FlowContext,
                    streamData->flags,
                    streamData->dataLength,
                    InFixedValues->layerId,
                    Filter->action.calloutId,
                    InMetaValues->flowHandle,
                    ClassifyOut->flags);

    // Set default classify action to Block, and stream action to None!
    //
    ClassifyOut->actionType = FWP_ACTION_BLOCK;
    ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;

    // Setting countBytesEnforced to 0 bytes means that action applies
    // to entire stream... We'll consume or reject the entire stream data segment
    //
    ioPacket->countBytesEnforced = 0;

    //
    // If the indicated data is insufficient for the callout to make an inspection
    // decision, it can request for more data. To do so :
    //
    // 1. set IoPacket->streamAction to FWPS_STREAM_ACTION_NEED_MORE_DATA
    // 2. set the countBytesRequired member to the minimal amount WFP should
    //    accumulate before the data is indicated again.
    // 3. return FWP_ACTION_NONE from the classifyFn function
    //

    if ((streamData->dataLength < Globals.StringToFindLength) &&
        !(ClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA))
    {
        ioPacket->streamAction = FWPS_STREAM_ACTION_NEED_MORE_DATA;
        ioPacket->countBytesRequired = (UINT32)Globals.StringToFindLength;

        ClassifyOut->actionType = FWP_ACTION_NONE;

        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
                "<-- %!FUNC!: FlowCtx %p - Need more data.",  FlowContext);
        return;
    }

    if (FlowContext->bFlowTerminating) 
	{
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                "Received data classified after FIN for FlowCtx %p.", FlowContext);
        return;
    }

    KeAcquireInStackQueuedSpinLock(&FlowContext->OobInfo.EditLock, &LockHandle );

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
            "FlowCtx %p: PendedDataLength %Iu, StreamDataLength %Iu, State %!OobState!, sFlags %#x",
                            FlowContext,
                            FlowContext->OobInfo.PendedDataLength,
                            streamData->dataLength,
                            FlowContext->OobInfo.EditState,
                            streamData->flags);

    switch (FlowContext->OobInfo.EditState)
    {
        case OOB_EDIT_PROCESSING:

            // If we are to pause a flow, make sure it is an inbound flow. Currently,
            // WFP Stream layer only  supports pausing  and  resuming  inbound flows.
            //
            // Let's not pause the stream if we receive a DISCONNECT (FIN) packet.
            // For FIN packets, streamData->dataLength == 0, and streamData->flags has
            // one of the following flags set:
            //      FWPS_STREAM_FLAG_SEND_DISCONNECT
            //      FWPS_STREAM_FLAG_RECEIVE_DISCONNECT
            //
            if (( streamData->flags & FWPS_STREAM_FLAG_RECEIVE) &&
                  streamData->dataLength &&
                  FALSE == FlowContext->bNoMoreData )
            {
                // While the stream is paused/deferred, it keeps storing incoming data
                // for the callouts. If resuming a stream causes to give us more data
                // than we can handle, then pause the stream again.
                //
                if (((FlowContext->OobInfo.PendedDataLength + streamData->dataLength) > Globals.BusyThreshold))
                {
                    ioPacket->streamAction = FWPS_STREAM_ACTION_DEFER;
                    ClassifyOut->actionType = FWP_ACTION_NONE;

                    FlowContext->OobInfo.StreamFlags = streamData->flags;
                    FlowContext->OobInfo.EditState = OOB_EDIT_BUSY;

                    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p, Flow is pausing", FlowContext);
                    break;
                }
            }

            // If this is an outbound flow OR inbound flow that has not yet hit
            // threshold limit, keep queueing the data for processing.
            //
            // Fall through to IDLE State.
            //

        case OOB_EDIT_IDLE:

#pragma prefast(push)
#pragma prefast(disable:6014, "Warning Leaking memory 'TaskEntry' : TaskEntry is freed in StreamEditOobPoolWorker")
            TaskEntry = (PTASK_ENTRY) ExAllocateFromLookasideListEx(&Globals.LookasideList);
#pragma prefast(pop)

            if (TaskEntry)
            {
                TaskEntry->FlowCtx = FlowContext;

#pragma prefast(push)
#pragma prefast(disable: 26110, "The lock is being held via KeAcquireInStackQueuedSpinLock")
				Status = StreamOobQueueUpIncomingData(TaskEntry, streamData, ClassifyOut->flags);
#pragma prefast(pop)

                if (NT_SUCCESS(Status))
                {
                    // If the previous state was _PROCESSING, it stays the same...
                    FlowContext->OobInfo.EditState = OOB_EDIT_PROCESSING;

                    // Block original data
                    //
                    break;
                }
                else
                {
                    ExFreeToLookasideListEx(&Globals.LookasideList, TaskEntry);
                    TaskEntry = NULL;
                }
            }

            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL,
                    "OC: FlowCtx %p ERROR. Task %p, %!STATUS!", FlowContext, TaskEntry, Status);

            FlowContext->OobInfo.EditState = OOB_EDIT_ERROR;

            //
            // Fall through for ERROR processing
            //

        case OOB_EDIT_ERROR:
            //
            // An out-of-band inspection module must not arbitrarily inject a FIN
            // (which indicates no more data from the sender) into an (outgoing) data
            // stream. If the module must drop the connection, its classifyFn callout
            // function must set the FWPS_STREAM_CALLOUT_IO_PACKET->streamAction to
            // FWPS_STREAM_ACTION_DROP_CONNECTION.
            //

            ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
            break;

        default:
            NT_ASSERT(FALSE);
			break;
    };

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
        "<-- %!FUNC!: FlowCtx %p, cAction %#x, sAction %#x, #Enforced %Iu, New State %!OobState!",
        FlowContext, ClassifyOut->actionType, ioPacket->streamAction, ioPacket->countBytesEnforced, FlowContext->OobInfo.EditState);

    KeReleaseInStackQueuedSpinLock(&LockHandle);
    return;
}
