/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:
    Stream Edit Callout Driver Sample.
    
    This sample demonstrates inline stream inspection/editing 
    via the WFP stream API.

Environment:
    Kernel mode
--*/

#define POOL_ZERO_DOWN_LEVEL_SUPPORT
#include "Trace.h"
#include "StreamEdit.h"
#include "InlineEdit.tmh"

#if defined _MODULE_ID
#undef _MODULE_ID
#endif
#define _MODULE_ID  'I'

NTSTATUS
InlineEditFlushData(
    _In_ STREAM_FLOW_CONTEXT *pFlowContext,
    _In_ ULONG DataLength,
    _In_ UINT StreamFlags
    )
/*
    This function re-injects buffered data back to the data stream.
    The data was buffered because it was not big enough (size wise)
    to make an editing decision.
*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PVOID Buffer = NULL;
    MDL* mdl = NULL;
    NET_BUFFER_LIST* NetBufferList = NULL;

    NT_ASSERT(!(StreamFlags & FWPS_STREAM_FLAG_SEND_DISCONNECT) &&
              !(StreamFlags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT));

    if (DataLength == 0)
	{
		DataLength = (ULONG)pFlowContext->ScratchDataLength;
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
        "--> %!FUNC!: FlowCtx %p, Flushing %u bytes", pFlowContext, DataLength);
    do
    {
        if (DataLength == 0)
            break;

        Buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, DataLength, STMEDIT_TAG_MDL_DATA);
        if (Buffer == NULL)
		{
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "Failed to allocate Buffer to flush data!");
            break;
        }

        // Copy the contents that need to be flushed.
        RtlMoveMemory(Buffer, pFlowContext->ScratchBuffer, DataLength);

        mdl = IoAllocateMdl(
                    Buffer,
					DataLength,
                    FALSE,
                    FALSE,
                    NULL);

        if (mdl == NULL)
		{
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "Failed to allocate MDL");
            break;
        }

        MmBuildMdlForNonPagedPool(mdl);

        Status = FwpsAllocateNetBufferAndNetBufferList(
                        Globals.NetBufferListPool,
                        0,
                        0,
                        mdl,
                        0,
						DataLength,
                        &NetBufferList);

        if (!NT_SUCCESS(Status))
        {
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "FwpsAllocateNetBufferAndNetBufferList Failed with %!STATUS!", Status);
            break;
        }

        Status = FwpsStreamInjectAsync(
                        Globals.InjectionHandle,
                        NULL,
                        0,
                        pFlowContext->FlowHandle,
                        pFlowContext->CalloutId,
                        pFlowContext->LayerId,
                        StreamFlags,
                        NetBufferList,
						DataLength,
                        StreamEditInjectCompletionFn,
                        mdl);

        if (!NT_SUCCESS(Status))
        {
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL, "FwpsStreamInjectAsync failed with %!STATUS!", Status);
            break;
        }

        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p, Flushed %lu bytes via NBL %p, MDL %p",
                            pFlowContext, DataLength, NetBufferList, mdl);

        // Control transferred to WFP.
        mdl = NULL;
        NetBufferList = NULL;
        Buffer = NULL;

    } while (FALSE);


    if (!NT_SUCCESS(Status))
    {
        if (Buffer != NULL)
		{
            ExFreePoolWithTag(Buffer, STMEDIT_TAG_MDL_DATA);
        }

        if (mdl != NULL) 
		{
            IoFreeMdl(mdl);
        }

        if (NetBufferList != NULL) 
		{
            FwpsFreeNetBufferList(NetBufferList);
        }
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT, "<-- %!FUNC!: FlowCtx %p, %!STATUS!", pFlowContext, Status);
    return Status;
}

#define PermitBytes(_l)\
{\
    ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;\
    ioPacket->countBytesEnforced = (_l);\
    ClassifyOut->actionType = FWP_ACTION_PERMIT;\
}

NTSTATUS
InlineInjectToken(
    PSTREAM_FLOW_CONTEXT FlowContext,
    UINT32 StreamFlags
    )
/*
    Inject a replacement token into the data stream!
*/
{
    NTSTATUS Status;
    NET_BUFFER_LIST* NetBufferList;
    
    do
    {
        Status = FwpsAllocateNetBufferAndNetBufferList(
                        Globals.NetBufferListPool,
                        0,
                        0,
                        Globals.StringToReplaceMdl,
                        0,
                        Globals.StringToReplaceLength,
                        &NetBufferList
                        );

        if (!NT_SUCCESS(Status))
        {
            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL,
                "FlowCtx %p, FwpsAllocateNetBufferAndNetBufferList failed with %#x, dropping connection",
                    FlowContext, Status);
            break;
        }

        Status = FwpsStreamInjectAsync(
                        Globals.InjectionHandle,
                        NULL,
                        0,
                        FlowContext->FlowHandle,
                        FlowContext->CalloutId,
                        FlowContext->LayerId,
                        StreamFlags,
                        NetBufferList,
                        Globals.StringToReplaceLength,
                        StreamEditInjectCompletionFn,
                        NULL
                        );

        if (!NT_SUCCESS(Status))
        {
            FwpsFreeNetBufferList(NetBufferList);

            DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL,
                    "FlowCtx %p, FwpsStreamInjectAsync failed with %!STATUS!, dropping connection",
                    FlowContext, Status);
            break;
        }

        FlowContext->ScratchDataOffset += Globals.StringXLength;
        FlowContext->ScratchDataLength -= Globals.StringXLength;

        if (FlowContext->ScratchDataLength > 0) 
		{
            FlowContext->InlineEditState = INLINE_EDIT_SCANNING;
        }
        else 
		{
            FlowContext->ScratchDataOffset = 0;
            FlowContext->InlineEditState = INLINE_EDIT_IDLE;
        }
    } while (FALSE);
    
    return Status;
}


VOID 
NTAPI
InlineEditClassify(
   _In_ const FWPS_INCOMING_VALUES* InFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* InMetaValues,
   _Inout_ VOID* LayerData,
   _In_ const FWPS_FILTER* Filter,
   _In_ UINT64 InFlowContext,
   _Inout_ FWPS_CLASSIFY_OUT* ClassifyOut
    )
/*
    This is the ClassifyFn function registered by the inline stream edit callout.

    An inline  stream  modification  callout  performs  editing  from  within the
    classifyFn call by permitting  sections of  the content  and  replacing other
    sections by removing them and injecting new content.

    Here we implement the  state machine that scans  the content and computes the
    number of bytes to permit,  bytes to block,  and performs stream injection to
    replace the blocked data.

    @ Inline Stream Inspection
    @ https://msdn.microsoft.com/en-us/library/windows/hardware/ff570891.aspx

    To replace a pattern Found in the middle of an indicated segment (for example,
    n bytes followed by a pattern of  p  bytes followed by  m  bytes), the callout
    would follow these steps:

    1. The callout's classifyFn function is invoked with 'n + p + m' bytes.
    2. The callout returns FWP_ACTION_PERMIT with countBytesEnforced set to n.
    3. The callout's classifyFn function is called again with p + m bytes.
        (WFP will call classifyFn again if countBytesEnforced is less than the
        indicated amount.)
    4. From the classifyFn function, the callout calls the FwpStreamInjectAsync0
        function to inject the replacement pattern  for  p. The callout then
        returns FWP_ACTION_BLOCK with countBytesEnforced set to p.
    5. The callout's classifyFn function is called again with m bytes.
    6. The callout returns FWP_ACTION_PERMIT with countBytesEnforced set to m.

    If the indicated data is insufficient  for the callout to  make an inspection
    decision,  it  can  set  FWPS_STREAM_CALLOUT_IO_PACKET->streamAction  to
    FWPS_STREAM_ACTION_NEED_MORE_DATA and set the countBytesRequired member to the
    minimal amount WFP should accumulate before the data is indicated again. When
    streamAction is set, the callout should return FWP_ACTION_NONE from the
    classifyFn function.
*/
{
    FWPS_STREAM_CALLOUT_IO_PACKET* ioPacket;
    FWPS_STREAM_DATA* streamData;
    PSTREAM_FLOW_CONTEXT FlowContext = (PSTREAM_FLOW_CONTEXT)(ULONG_PTR)InFlowContext;
    ULONG PartialLength = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(InFixedValues);
    UNREFERENCED_PARAMETER(InMetaValues);

    ioPacket = (FWPS_STREAM_CALLOUT_IO_PACKET*)LayerData;
    streamData = ioPacket->streamData;

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
        "--> %!FUNC!: FlowCtx %p, sFlags %#x, Length %Iu, LayerId %hu, CalloutId %u, "
        "FlowId %I64u, cFlags %#x, Flow State %!InlineState!, Scratch:%Iu/%Iu",
                FlowContext,
                streamData->flags,
                streamData->dataLength,
                InFixedValues->layerId,
                Filter->action.calloutId,
                InMetaValues->flowHandle,
                ClassifyOut->flags,
				FlowContext->InlineEditState,
				FlowContext->ScratchDataLength,
		        FlowContext->ScratchDataOffset);


    // The following checks if the classifyFn is invoked
    // simultaneously on multiple processors for the same flow.
    // This should NOT happen!
    {
        ULONG Processor = KeGetCurrentProcessorIndex();
        Processor = InterlockedExchange(&FlowContext->CurrentProcessor, Processor);
        NT_ASSERT(Processor == INVALID_PROC_NUMBER);
    }

    // If a FIN/RST has been classified, flush any data and permit the FIN/RST.
    //
    if ((!FlowContext->bFlowActive) ||
        (streamData->flags & FWPS_STREAM_FLAG_SEND_DISCONNECT) || // must also handle FWPS_STREAM_FLAG_SEND_ABORT
        (streamData->flags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT)) // must also handle FWPS_STREAM_FLAG_RECEIVE_ABORT
    {
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p, FIN/RST classified (Flow Active %d)!", FlowContext, FlowContext->bFlowActive);

        if (FlowContext->ScratchDataLength > 0)
        {
            InlineEditFlushData(FlowContext, 0, FlowContext->PartialSFlags);
            FlowContext->ScratchDataLength = 0;
            FlowContext->ScratchDataOffset = 0;
        }

        NT_ASSERT(FlowContext->InlineEditState == INLINE_EDIT_IDLE);

        PermitBytes(0);

        goto Exit;
    }

    if (streamData->dataLength == 0) 
	{

        PermitBytes(0);
        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p -> DataLength = 0, Permit!", FlowContext);
        goto Exit;
    }

    if (FlowContext->InlineEditState != INLINE_EDIT_SKIPPING)
    {
        if ((streamData->dataLength < Globals.StringXLength) &&
            !(ClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA))
        {
            ioPacket->streamAction = FWPS_STREAM_ACTION_NEED_MORE_DATA;
            ioPacket->countBytesRequired = (UINT32)Globals.StringXLength;

            ClassifyOut->actionType = FWP_ACTION_NONE;

            DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p - need more data!", FlowContext);
            goto Exit;
        }
    }

    switch (FlowContext->InlineEditState)
    {
        case INLINE_EDIT_IDLE:
        {
            NT_ASSERT(FlowContext->ScratchDataOffset == 0);

            PartialLength = (ULONG)FlowContext->ScratchDataLength;
            if (PartialLength) 
			{
                NT_ASSERT(FlowContext->PartialSFlags);
                DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p, Partial Data Found. Length %u", FlowContext, PartialLength);
            }

            if (FALSE == StreamEditCopyDataForInspection(FlowContext, streamData, streamData->dataLength))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
                ClassifyOut->actionType = FWP_ACTION_NONE;

                DoTraceLevelMessage(TRACE_LEVEL_ERROR, CO_GENERAL,
                            "FlowCtx %p, failed to copy data, drop connection", FlowContext);
                goto Exit;
            }

            //
            // Fall-thru to scanning
            //
        }

        case INLINE_EDIT_SCANNING:
        {
            UINT i;
            BYTE* DataStart = (BYTE*)FlowContext->ScratchBuffer + FlowContext->ScratchDataOffset;
            BOOLEAN Found = FALSE;

            for (i = 0; i < FlowContext->ScratchDataLength; ++i)
            {
                // Look for a Full match if we have enough data to scan
                //
                if (i + Globals.StringXLength <= FlowContext->ScratchDataLength)
                {
                    if (RtlCompareMemory(DataStart + i, Globals.StringX, Globals.StringXLength ) == Globals.StringXLength)
                    {
                        // We Found a pattern match
                        Found = TRUE;

                        FlowContext->InlineEditState = INLINE_EDIT_MODIFYING;
                        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL, "FlowCtx %p, Found match @ %lu", FlowContext, i-PartialLength);

                        // If the match is in the middle of data packet, permit the data
                        // before match from (n + p + m), permit n. We'll be reclassified
                        // at beginning of the match (with p + m).
                        //
                        if (i != 0)
                        {
                            // Flush any left over partial match data on the scratch buffer.
                            if (PartialLength) 
							{
                                InlineEditFlushData(FlowContext, PartialLength, FlowContext->PartialSFlags);
                            }

                            PermitBytes(i - PartialLength);

                            FlowContext->ScratchDataOffset += i;
                            FlowContext->ScratchDataLength -= i;

                            break;
                        }
                        else
                        {
                            Status = InlineInjectToken(FlowContext, streamData->flags);
                            if (NT_SUCCESS(Status))
                            {
                                // Block the segment for which we injected a replacement
                                //
                                ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
                                ioPacket->countBytesEnforced = Globals.StringXLength - PartialLength;
                                ClassifyOut->actionType = FWP_ACTION_BLOCK;
                            }
                            break; // out of for loop.
                        }
                    } // if (RtlCompareMemory ...)
                }

                //
                // If we do not have enough data, try to look for a partial pattern match
                //
                else
                {
                    // If we do not expect more data to come in, quit now!
                    if (ClassifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA)
                    {
                        PermitBytes(0);
                        break;
                    }

                    if (RtlCompareMemory(DataStart + i, Globals.StringX, FlowContext->ScratchDataLength - i )
                                == FlowContext->ScratchDataLength - i)
                    {
                        Found = TRUE;  // This is a partial match
                        FlowContext->InlineEditState = INLINE_EDIT_SKIPPING;

                        // Permit data before partial match. When we get more data, we'll look for a complete match.
                        //
                        if (PartialLength) 
						{
                            InlineEditFlushData(FlowContext, PartialLength, FlowContext->PartialSFlags);
                        }

                        PermitBytes(i - PartialLength);

                        // Move partial matching data to Scratch Buffer.
                        //
                        RtlMoveMemory(FlowContext->ScratchBuffer, DataStart + i, FlowContext->ScratchDataLength - i );

                        FlowContext->PartialSFlags = streamData->flags;

                        FlowContext->ScratchDataOffset = 0;
                        FlowContext->ScratchDataLength -= i;

                        DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_GENERAL,
                                            "FlowCtx %p, Found partial match of %Iu byte(s) @ %lu",
                                                FlowContext, FlowContext->ScratchDataLength, i - PartialLength);
                        break;
                    }
                }
            } // for ( i = 0; i < ScratchDataLength...

            // If no match is Found, inject the whole chunk back into the stream
            //
            if (!Found)
            {
                FlowContext->InlineEditState = INLINE_EDIT_IDLE;

                if (PartialLength) 
				{
                    InlineEditFlushData(FlowContext, PartialLength, FlowContext->PartialSFlags );
                }

                PermitBytes(0);

                FlowContext->ScratchDataOffset = 0;
                FlowContext->ScratchDataLength = 0;
            }

            break;
        }

        // No new data will be classified until we consume all the data originally
        // indicated. We block the partially matching data (left in scratch-buffer),
        // and continue search for a full match when more data in indicated.
        //
        case INLINE_EDIT_SKIPPING:
        {
            NT_ASSERT(FWPS_STREAM_ACTION_NONE == ioPacket->streamAction);
            ioPacket->countBytesEnforced = 0;
            ClassifyOut->actionType = FWP_ACTION_BLOCK;

            FlowContext->InlineEditState = INLINE_EDIT_IDLE;
            DoTraceLevelMessage(TRACE_LEVEL_VERBOSE, CO_GENERAL, "Contents of Blocked buffer : %!HEXDUMP!",
                log_xstr(FlowContext->ScratchBuffer, (USHORT)streamData->dataLength));
            break;
        }

        // Injecting a replacement pattern when a full match is
        // Found at beginning of the scan buffer
        case INLINE_EDIT_MODIFYING:
        {
            Status = InlineInjectToken(FlowContext, streamData->flags);
            if (NT_SUCCESS(Status))
            {
                // Block the segment for which we injected a replacement
                //
                ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
                ioPacket->countBytesEnforced = Globals.StringXLength - PartialLength;
                ClassifyOut->actionType = FWP_ACTION_BLOCK;
            }
            break;
        }

        default:
            NT_ASSERT(FALSE);
            break;
    }; // switch

Exit:

    if (!NT_SUCCESS (Status))
    {
        ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
        ClassifyOut->actionType = FWP_ACTION_NONE;
    }

    DoTraceLevelMessage(TRACE_LEVEL_INFORMATION, CO_ENTER_EXIT,
        "<-- %!FUNC!: FlowCtx %p, cOut->Action %#x, sAction %#x, #Enforced %Iu, Scratch: %Iu/%Iu, New State %!InlineState!",
                    FlowContext,
                    ClassifyOut->actionType,
                    ioPacket->streamAction,
                    ioPacket->countBytesEnforced,
                    FlowContext->ScratchDataLength,
                    FlowContext->ScratchDataOffset,
                    FlowContext->InlineEditState);

    InterlockedExchange(&FlowContext->CurrentProcessor, INVALID_PROC_NUMBER);

    return;
}
