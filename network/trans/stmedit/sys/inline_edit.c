/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Stream Edit Callout Driver Sample.
    
    This sample demonstrates inline stream inspection/editing 
    via the WFP stream API.

Environment:

    Kernel mode

--*/

#include <ntddk.h>

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union

#include <fwpsk.h>

#pragma warning(pop)

#include <fwpmk.h>

#include "inline_edit.h"
#include "oob_edit.h"
#include "stream_callout.h"

void
InlineEditInit(
   _Out_ STREAM_EDITOR* streamEditor
   )
{
   streamEditor->editInline = TRUE;
   streamEditor->inlineEditState = INLINE_EDIT_WAITING_FOR_DATA;
}

void 
NTAPI StreamInjectCompletionFn(
   _Inout_ void* context,
   _Inout_ NET_BUFFER_LIST* netBufferList,
   _In_ BOOLEAN dispatchLevel
   )
{
   MDL* mdl = (MDL*)context;

   UNREFERENCED_PARAMETER(dispatchLevel);

   if (mdl != NULL)
   {
      IoFreeMdl(mdl);
   }

   FwpsFreeNetBufferList(netBufferList);
}

NTSTATUS
StreamEditFlushData(
   _Inout_ STREAM_EDITOR* streamEditor,
   UINT64 flowId,
   UINT32 calloutId,
   UINT16 layerId,
   UINT32 streamFlags
   )
/* ++

   This function re-injects buffered data back to the data stream upon
   receiving a FIN. The data was buffered because it was not big enough
   (size wise) to make an editing decision.

-- */
{
   NTSTATUS status;

   MDL* mdl = NULL;
   NET_BUFFER_LIST* netBufferList = NULL;

   NT_ASSERT(streamEditor->dataOffset == 0);

   mdl = IoAllocateMdl(
            streamEditor->scratchBuffer,
            (ULONG)(streamEditor->dataLength),
            FALSE,
            FALSE,
            NULL
            );

   if (mdl == NULL)
   {
      status = STATUS_NO_MEMORY;
      goto Exit;
   }

   MmBuildMdlForNonPagedPool(mdl);

   status = FwpsAllocateNetBufferAndNetBufferList(
                  gNetBufferListPool,
                  0,
                  0,
                  mdl,
                  0,
                  streamEditor->dataLength,
                  &netBufferList
                  );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   streamFlags &= ~(FWPS_STREAM_FLAG_SEND_DISCONNECT | FWPS_STREAM_FLAG_RECEIVE_DISCONNECT);

   status = FwpsStreamInjectAsync(
               gInjectionHandle,
               NULL,
               0,
               flowId,
               calloutId,
               layerId,
               streamFlags, 
               netBufferList,
               streamEditor->dataLength,
               StreamInjectCompletionFn,
               mdl
               );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   mdl = NULL;
   netBufferList = NULL;

Exit:

   if (mdl != NULL)
   {
      IoFreeMdl(mdl);
   }
   if (netBufferList != NULL)
   {
      FwpsFreeNetBufferList(netBufferList);
   }

   return status;
}

void
StreamInlineEdit(
   _Inout_ STREAM_EDITOR* streamEditor,
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _In_ const FWPS_FILTER* filter,
   _In_ const FWPS_STREAM_DATA* streamData,
   _Inout_ FWPS_STREAM_CALLOUT_IO_PACKET* ioPacket,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )
/* ++

   This function implements the state machine that scans the content 
   and computes the number of bytes to permit, bytes to block, and 
   performs stream injection to replace the blocked data.

-- */
{
   UINT findLength = (UINT) strlen(configStringToFind);
   UINT replaceLength = (UINT) strlen(configStringToReplace);

   if ((streamData->flags & FWPS_STREAM_FLAG_SEND_DISCONNECT) || 
       (streamData->flags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT))
   {
      if (streamEditor->dataLength > 0)
      {
         StreamEditFlushData(
            streamEditor,
            inMetaValues->flowHandle,
            filter->action.calloutId,
            inFixedValues->layerId,
            streamData->flags
            );

         streamEditor->dataLength = 0;
         streamEditor->dataOffset = 0;
      }

      NT_ASSERT(streamEditor->inlineEditState == INLINE_EDIT_WAITING_FOR_DATA);

      ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
      classifyOut->actionType = FWP_ACTION_PERMIT;

      if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
      {
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      }

      goto Exit;
   }

   if (streamData->dataLength == 0)
   {
      ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
      classifyOut->actionType = FWP_ACTION_PERMIT;

      if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
      {
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      }

      goto Exit;
   }

   if (streamEditor->inlineEditState != INLINE_EDIT_SKIPPING)
   {
      if ((streamData->dataLength < findLength) && 
          !(classifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA))
      {
         ioPacket->streamAction = FWPS_STREAM_ACTION_NEED_MORE_DATA;
         ioPacket->countBytesRequired = findLength;

         classifyOut->actionType = FWP_ACTION_NONE;
         goto Exit;
      }
   }

   switch (streamEditor->inlineEditState)
   {
      case INLINE_EDIT_WAITING_FOR_DATA:
      {
         if (StreamCopyDataForInspection(
               streamEditor,
               streamData
               ) == FALSE)
         {
            ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
            classifyOut->actionType = FWP_ACTION_NONE;
            goto Exit;
         }

         //
         // Pass-thru to scanning
         //
      }
      case INLINE_EDIT_SCANNING:
      {
         UINT i;
         BYTE* dataStart =  (BYTE*)streamEditor->scratchBuffer + streamEditor->dataOffset;
         BOOLEAN found = FALSE;

         for (i = 0; i < streamEditor->dataLength; ++i)
         {
            if (i + findLength <= streamEditor->dataLength)
            {
               if (RtlCompareMemory(
                     dataStart + i,
                     configStringToFind,
                     findLength
                     ) == findLength)
               {
                  found = TRUE;

                  streamEditor->inlineEditState = INLINE_EDIT_MODIFYING;

                  if (i != 0)
                  {
                     ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
                     ioPacket->countBytesEnforced = i;

                     classifyOut->actionType = FWP_ACTION_PERMIT;

                     if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
                     {
                        classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
                     }

                     streamEditor->dataOffset += i;
                     streamEditor->dataLength -= i;

                     break;
                  }
                  else
                  {
                     goto modify_data;
                  }
               }
            }
            else
            {
               if (classifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA)
               {
                  break;
               }

               if (RtlCompareMemory(
                     dataStart + i,
                     configStringToFind,
                     streamEditor->dataLength - i
                     ) == streamEditor->dataLength - i)
               {
                  found = TRUE;  // this is a partial find
   
                  ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
                  ioPacket->countBytesEnforced = i;

                  classifyOut->actionType = FWP_ACTION_PERMIT;

                  if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
                  {
                     classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
                  }

                  RtlMoveMemory(
                     streamEditor->scratchBuffer,
                     dataStart + i,
                     streamEditor->dataLength - i
                     );

                  streamEditor->dataOffset = 0;
                  streamEditor->dataLength = streamEditor->dataLength - i;

                  streamEditor->inlineEditState = INLINE_EDIT_SKIPPING;
   
                  break;
               }
            }
         }

         if (!found)
         {
            ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
            ioPacket->countBytesEnforced = 0;

            classifyOut->actionType = FWP_ACTION_PERMIT;

            if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
            {
               classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
            }

            streamEditor->dataOffset = 0;
            streamEditor->dataLength = 0;

            streamEditor->inlineEditState = INLINE_EDIT_WAITING_FOR_DATA;
         }

         break;
      }
      case INLINE_EDIT_SKIPPING:
      {
         ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
         ioPacket->countBytesEnforced = 0;

         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

         streamEditor->inlineEditState = INLINE_EDIT_WAITING_FOR_DATA;

         break;
      }
      case INLINE_EDIT_MODIFYING:

modify_data:
      
      {
         NTSTATUS status;
         NET_BUFFER_LIST* netBufferList;

         status = FwpsAllocateNetBufferAndNetBufferList(
                     gNetBufferListPool,
                     0,
                     0,
                     gStringToReplaceMdl,
                     0,
                     replaceLength,
                     &netBufferList
                     );

         if (!NT_SUCCESS(status))
         {
            ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
            classifyOut->actionType = FWP_ACTION_NONE;
            goto Exit;
         }

         status = FwpsStreamInjectAsync(
                     gInjectionHandle,
                     NULL,
                     0,
                     inMetaValues->flowHandle,
                     filter->action.calloutId,
                     inFixedValues->layerId,
                     streamData->flags,
                     netBufferList,
                     replaceLength,
                     StreamInjectCompletionFn,
                     NULL
                     );

         if (!NT_SUCCESS(status))
         {
            FwpsFreeNetBufferList(netBufferList);

            ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
            classifyOut->actionType = FWP_ACTION_NONE;
            goto Exit;
         }

         ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
         ioPacket->countBytesEnforced = findLength;

         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

         streamEditor->dataOffset += findLength;
         streamEditor->dataLength -= findLength;

         if (streamEditor->dataLength > 0)
         {
            streamEditor->inlineEditState = INLINE_EDIT_SCANNING;
         }
         else
         {
            streamEditor->dataOffset = 0;

            streamEditor->inlineEditState = INLINE_EDIT_WAITING_FOR_DATA;
         }

         break;
      }
      default:
         NT_ASSERT(FALSE);
         break;
   };

Exit:

   return;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

void 
NTAPI
StreamInlineEditClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_ void* layerData,
   _In_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )

#else

void 
NTAPI
StreamInlineEditClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Out_ FWPS_CLASSIFY_OUT* classifyOut
   )

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

/* ++

   This is the ClassifyFn function registered by the inline stream edit callout.

   An inline stream modification callout performs editing from within the
   ClassifyFn call by permitting sections of the content and replacing other
   sections by removing them and injecting new content.

-- */
{
   FWPS_STREAM_CALLOUT_IO_PACKET* ioPacket;
   FWPS_STREAM_DATA* streamData;

   ioPacket = (FWPS_STREAM_CALLOUT_IO_PACKET*)layerData;
   NT_ASSERT(ioPacket != NULL);

   streamData = ioPacket->streamData;
   NT_ASSERT(streamData != NULL);

#if(NTDDI_VERSION >= NTDDI_WIN7)
   UNREFERENCED_PARAMETER(classifyContext);
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
   UNREFERENCED_PARAMETER(flowContext);

   RtlZeroMemory(classifyOut, sizeof(FWPS_CLASSIFY_OUT));

   //
   // Let go the traffic that the editor does not care about.
   //

   if ((configInspectionOutbound  && (streamData->flags & FWPS_STREAM_FLAG_RECEIVE)) ||
       (!configInspectionOutbound && (streamData->flags & FWPS_STREAM_FLAG_SEND)))
   {
      ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
      classifyOut->actionType = FWP_ACTION_PERMIT;

      if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
      {
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      }

      goto Exit;
   }

   //
   // In this sample we don't edit TCP urgent data
   //

   if ((streamData->flags & FWPS_STREAM_FLAG_SEND_EXPEDITED) ||
       (streamData->flags & FWPS_STREAM_FLAG_RECEIVE_EXPEDITED))
   {
      ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
      classifyOut->actionType = FWP_ACTION_PERMIT;

      if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
      {
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      }

      goto Exit;
   }

   StreamInlineEdit(
      &gStreamEditor,
      inFixedValues,
      inMetaValues,
      filter,
      streamData,
      ioPacket,
      classifyOut
      );

Exit:

   return;
}
