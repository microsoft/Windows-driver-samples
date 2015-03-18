/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

    Stream Edit Callout Driver Sample.
    
    This sample demonstrates Out-of-band (OOB) stream inspection/editing 
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

#define STREAM_EDITOR_OUTGOING_DATA_TAG 'doeS'
#define STREAM_EDITOR_MDL_DATA_TAG 'dmeS'

void* gThreadObj;

KSTART_ROUTINE StreamOobEditWorker;

NTSTATUS
OobEditInit(
   _Out_ STREAM_EDITOR* streamEditor
   )
{
   NTSTATUS status = STATUS_SUCCESS;

   HANDLE threadHandle;
   
   streamEditor->editInline = FALSE;

   KeInitializeSpinLock(&streamEditor->oobEditInfo.editLock);   

   KeInitializeEvent(
      &streamEditor->oobEditInfo.editEvent,
      NotificationEvent,
      FALSE
      );

  streamEditor->oobEditInfo.busyThreshold = 32 * 1024;
  streamEditor->oobEditInfo.editState = OOB_EDIT_IDLE;

  InitializeListHead(&streamEditor->oobEditInfo.outgoingDataQueue);


   status = PsCreateSystemThread(
               &threadHandle,
               THREAD_ALL_ACCESS,
               NULL,
               NULL,
               NULL,
               StreamOobEditWorker,
               &gStreamEditor
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   status = ObReferenceObjectByHandle(
               threadHandle,
               0,
               NULL,
               KernelMode,
               &gThreadObj,
               NULL
               );
   NT_ASSERT(NT_SUCCESS(status));

   ZwClose(threadHandle);

Exit:

   return status;
}

void
OobEditShutdown(
   _Out_ STREAM_EDITOR* streamEditor
   )
{
   KLOCK_QUEUE_HANDLE editLockHandle;

   KeAcquireInStackQueuedSpinLock(
      &streamEditor->oobEditInfo.editLock,
      &editLockHandle
      );

   streamEditor->oobEditInfo.shuttingDown = TRUE;

   switch (streamEditor->oobEditInfo.editState)
   {
      case OOB_EDIT_IDLE:
      {
         streamEditor->oobEditInfo.editState = OOB_EDIT_SHUT_DOWN;

         KeSetEvent(
            &gStreamEditor.oobEditInfo.editEvent,
            IO_NO_INCREMENT, 
            FALSE
            );

         break;
      }
      default:
         break;
   };

   KeReleaseInStackQueuedSpinLock(&editLockHandle);

   NT_ASSERT(gThreadObj != NULL);

   KeWaitForSingleObject(
      gThreadObj,
      Executive,
      KernelMode,
      FALSE,
      NULL
      );

   ObDereferenceObject(gThreadObj);
}

__inline
NET_BUFFER_LIST* 
TailOfNetBufferListChain(
   _In_ NET_BUFFER_LIST* netBufferListChain
   )
{
   NT_ASSERT(netBufferListChain != NULL);

   while (netBufferListChain->Next != NULL)
   {
      netBufferListChain = netBufferListChain->Next;
   }

   return netBufferListChain;
}

void 
NTAPI 
StreamOobInjectCompletionFn(
   _Inout_ void* context,
   _Inout_ NET_BUFFER_LIST* netBufferList,
   BOOLEAN dispatchLevel
   )
/* ++

   Injection completion function for injecting an NBL created using 
   FwpsAllocateNetBufferAndNetBufferList. This function frees up 
   resources allocated during StreamOobReinjectData().

-- */
{
   MDL* mdl = (MDL*)context;

   UNREFERENCED_PARAMETER(dispatchLevel);

   FwpsFreeNetBufferList(netBufferList);

   if (mdl != NULL)
   {
      IoFreeMdl(mdl);

      //
      // The MDL mapped over a pool alloc which we need to free here.
      //

      ExFreePoolWithTag(
         mdl->MappedSystemVa, 
         STREAM_EDITOR_MDL_DATA_TAG
         );
   }
}

void 
NTAPI StreamOobInjectCloneCompletionFn(
   _Inout_ void* context,
   _Inout_ NET_BUFFER_LIST* netBufferList,
   BOOLEAN dispatchLevel
   )
/* ++

   Injection completion function for injecting one of the NBLs cloned
   via FwpsCloneStreamData.
   
   FwpsCloneStreamData can return a chain of cloned NBLs; each NBL will 
   complete separately.

-- */
{
   UNREFERENCED_PARAMETER(context);
   UNREFERENCED_PARAMETER(dispatchLevel);

   FwpsFreeCloneNetBufferList(netBufferList, 0);
}

NTSTATUS
StreamOobQueueUpIncomingData(
   _Inout_ STREAM_EDITOR* streamEditor,
   _Inout_ FWPS_STREAM_DATA* streamData
   )
/* ++

   This function clones the indicated stream data into a NBL chain and 
   appends the chain at the end of the existing chain (if exists) inside
   the streamEditor.
   
   This function assumes that the oobEditInfo lock inside streamEditor is
   being held.

-- */
{
   NTSTATUS status;

   NET_BUFFER_LIST* clonedNetBufferListChain;

   status = FwpsCloneStreamData(
               streamData,
               NULL,
               NULL,
               0,
               &clonedNetBufferListChain
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   //
   // TCP Fin (EOF) is indicated by an empty NBL with disconnect flag
   // set, since it does not contain any data we queue it up separately.
   //

   if ((streamData->flags & FWPS_STREAM_FLAG_SEND_DISCONNECT) || 
       (streamData->flags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT))
   {
      NT_ASSERT(streamEditor->oobEditInfo.noMoreData);
      NT_ASSERT(streamEditor->oobEditInfo.nblEof == NULL);
      NT_ASSERT(streamData->dataLength == 0);

      streamEditor->oobEditInfo.nblEof = clonedNetBufferListChain;

      status = STATUS_SUCCESS;
      goto Exit;
   }

   if (streamEditor->oobEditInfo.nblTail != NULL)
   {
      NT_ASSERT(streamEditor->oobEditInfo.nblHead != NULL);
      NT_ASSERT(streamEditor->oobEditInfo.nblTail->Next == NULL);

      streamEditor->oobEditInfo.nblTail->Next = clonedNetBufferListChain;
   }
   else
   {
      NT_ASSERT(streamEditor->oobEditInfo.nblHead == NULL);

      streamEditor->oobEditInfo.nblHead = clonedNetBufferListChain;
   }

   streamEditor->oobEditInfo.nblTail = 
      TailOfNetBufferListChain(clonedNetBufferListChain);

   streamEditor->oobEditInfo.totalDataLength += streamData->dataLength;

   streamEditor->oobEditInfo.streamFlags = streamData->flags;

Exit:

   return status;
}

void
StreamOobEdit(
   _Inout_ STREAM_EDITOR* streamEditor,
   const FWPS_INCOMING_VALUES* inFixedValues,
   const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   const FWPS_FILTER* filter,
   _Inout_ FWPS_STREAM_DATA* streamData,
   _Inout_ FWPS_STREAM_CALLOUT_IO_PACKET* ioPacket,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )
/* ++

   This function queues up incoming data and notifies the worker thread
   to process them. The incoming data is blocked and removed from the
   stream while data is pending.

   If the editor is shutdown (e.g. during driverUnload) as indicated 
   by OOB_EDIT_SHUT_DOWN state, it permits the indicated data inline after 
   flushing all pended data (to be carried out by the caller).

-- */

{
   NTSTATUS status;

   KLOCK_QUEUE_HANDLE editLockHandle;

   KeAcquireInStackQueuedSpinLock(
      &streamEditor->oobEditInfo.editLock,
      &editLockHandle
      );

   if (streamEditor->oobEditInfo.nblEof != NULL)
   {
      //
      // A new flow arrives before we finish processing an earlier flow. Production
      // code should create 1:1 between streamEditor and flow to handle this
      // condition. See the "MSN Monitor sample" for how that can be implemented. 
      //

      ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
      classifyOut->actionType = FWP_ACTION_NONE;

      goto Exit;
   }

   if (classifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA)
   {
      NT_ASSERT(streamEditor->oobEditInfo.nblEof == NULL);

      streamEditor->oobEditInfo.noMoreData = TRUE;
   }

   // 
   // Record needed flow information etc for data (re-)injection.
   //

   streamEditor->oobEditInfo.calloutId = filter->action.calloutId;
   streamEditor->oobEditInfo.flowId = inMetaValues->flowHandle;
   streamEditor->oobEditInfo.layerId = inFixedValues->layerId;

   switch (streamEditor->oobEditInfo.editState)
   {
      case OOB_EDIT_PROCESSING:
      {
         if ((streamEditor->oobEditInfo.totalDataLength + streamData->dataLength) > 
            streamEditor->oobEditInfo.busyThreshold)
         {
            ioPacket->streamAction = FWPS_STREAM_ACTION_DEFER;
            classifyOut->actionType = FWP_ACTION_NONE;

            streamEditor->oobEditInfo.editState = OOB_EDIT_BUSY;
         }
         else
         {
            status = StreamOobQueueUpIncomingData(
                        streamEditor,
                        streamData
                        );

            if (!NT_SUCCESS(status))
            {
               streamEditor->oobEditInfo.editState = OOB_EDIT_ERROR;

               ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
               classifyOut->actionType = FWP_ACTION_NONE;
            }
            else
            {
               //
               // State remains at OOB_EDIT_PROCESSING state. Since the worker thread
               // is active there is no need to set the event (to wake it up)
               //

               ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
               ioPacket->countBytesEnforced = 0;
               classifyOut->actionType = FWP_ACTION_BLOCK;
               classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
            }
         }

         break;
      }
      case OOB_EDIT_IDLE:
      {
         status = StreamOobQueueUpIncomingData(
                     streamEditor,
                     streamData
                     );

         if (!NT_SUCCESS(status))
         {
            streamEditor->oobEditInfo.editState = OOB_EDIT_ERROR;

            ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
            classifyOut->actionType = FWP_ACTION_NONE;
         }
         else
         {
            streamEditor->oobEditInfo.editState = OOB_EDIT_PROCESSING;

            //
            // The worker thread is idle waiting for more work, now wake it up.
            //
            KeSetEvent(
               &streamEditor->oobEditInfo.editEvent, 
               0, 
               FALSE
               );

            ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
            ioPacket->countBytesEnforced = 0;
            classifyOut->actionType = FWP_ACTION_BLOCK;
            classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         }

         break;
      }
      case OOB_EDIT_SHUT_DOWN:
      {
         ioPacket->streamAction = FWPS_STREAM_ACTION_NONE;
         ioPacket->countBytesEnforced = 0;
         classifyOut->actionType = FWP_ACTION_PERMIT;

         if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
         {
            classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         }

         break;
      }
      case OOB_EDIT_ERROR:
      {
         ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
         classifyOut->actionType = FWP_ACTION_NONE;
         break;
      }
      default:
         NT_ASSERT(FALSE);
   };

Exit:

   KeReleaseInStackQueuedSpinLock(&editLockHandle);
}

NTSTATUS
StreamOobQueueUpOutgoingData(
   _Inout_ STREAM_EDITOR* streamEditor,
   _Inout_ NET_BUFFER_LIST* netBufferList,
   BOOLEAN isClone,
   size_t dataLength,
   DWORD streamFlags,
   _In_opt_ MDL* mdl
   )
/* ++

   This function queues up processed data (either sections of the indicated
   data or newly created data) such that they can be (re-)injected back to
   the data stream during the following context.

      1. Before FWP_ACTION_BLOCK is returned from the ClassifyFn, or
      2. After EOF is indicated.

   Under the conditions above, the incoming data (which we pend) and the 
   outgoing data (which we (re-)inject) can be synchronized properly).

-- */
{
   NTSTATUS status = STATUS_SUCCESS;
   KLOCK_QUEUE_HANDLE editLockHandle;
   OUTGOING_STREAM_DATA* outgoingStreamData;

   outgoingStreamData = (OUTGOING_STREAM_DATA*) ExAllocatePoolWithTag(
                                                   NonPagedPool,
                                                   sizeof(OUTGOING_STREAM_DATA),
                                                   STREAM_EDITOR_OUTGOING_DATA_TAG
                                                   );

   if (outgoingStreamData == NULL)
   {
      status = STATUS_NO_MEMORY;
      return status;
   }

   RtlZeroMemory(outgoingStreamData, sizeof(OUTGOING_STREAM_DATA));

   outgoingStreamData->netBufferList = netBufferList;
   outgoingStreamData->isClone = isClone;
   outgoingStreamData->dataLength = dataLength;
   outgoingStreamData->streamFlags = streamFlags;
   outgoingStreamData->mdl = mdl;

   KeAcquireInStackQueuedSpinLock(
      &streamEditor->oobEditInfo.editLock,
      &editLockHandle
      );

   InsertTailList(
      &streamEditor->oobEditInfo.outgoingDataQueue,
      &outgoingStreamData->listEntry
      );

   KeReleaseInStackQueuedSpinLock(&editLockHandle);

   return status;
}

NTSTATUS
StreamOobFlushOutgoingData(
   _Inout_ STREAM_EDITOR* streamEditor
   )
{
   NTSTATUS status = STATUS_SUCCESS;

   KLOCK_QUEUE_HANDLE editLockHandle;
   OUTGOING_STREAM_DATA* outgoingStreamData = NULL;

   for(;;)
   {
      KeAcquireInStackQueuedSpinLock(
         &streamEditor->oobEditInfo.editLock,
         &editLockHandle
         );

      if (!IsListEmpty(&streamEditor->oobEditInfo.outgoingDataQueue))
      {
         LIST_ENTRY* listEntry = 
            RemoveHeadList(&streamEditor->oobEditInfo.outgoingDataQueue);
         
         outgoingStreamData = CONTAINING_RECORD(
                                 listEntry, 
                                 OUTGOING_STREAM_DATA,
                                 listEntry
                                 );
      }

      KeReleaseInStackQueuedSpinLock(&editLockHandle);

      if (outgoingStreamData == NULL)
      {
         break;
      }

      status = FwpsStreamInjectAsync(
                  gInjectionHandle,
                  NULL,
                  0,
                  streamEditor->oobEditInfo.flowId,
                  streamEditor->oobEditInfo.calloutId,
                  streamEditor->oobEditInfo.layerId,
                  outgoingStreamData->streamFlags,
                  outgoingStreamData->netBufferList,
                  outgoingStreamData->dataLength,
                  outgoingStreamData->isClone ? StreamOobInjectCloneCompletionFn :
                                                StreamOobInjectCompletionFn,
                  outgoingStreamData->mdl
                  );

      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }

      ExFreePoolWithTag(
         outgoingStreamData,
         STREAM_EDITOR_OUTGOING_DATA_TAG
         );

      outgoingStreamData = NULL;
   }
  
Exit:

   if (outgoingStreamData != NULL)
   {
      NT_ASSERT(!NT_SUCCESS(status));

      if (outgoingStreamData->isClone)
      {
         FwpsDiscardClonedStreamData(
            outgoingStreamData->netBufferList,
            0,
            FALSE
            );
      }
      else
      {
         FwpsFreeNetBufferList(outgoingStreamData->netBufferList);

         if (outgoingStreamData->mdl != NULL)
         {
            IoFreeMdl(outgoingStreamData->mdl);

            ExFreePoolWithTag(
               outgoingStreamData->mdl->MappedSystemVa, 
               STREAM_EDITOR_MDL_DATA_TAG
               );
         }
      }

      ExFreePoolWithTag(
         outgoingStreamData,
         STREAM_EDITOR_OUTGOING_DATA_TAG
         );
   }

   return status;
}


NTSTATUS
StreamOobReinjectData(
   _Inout_ STREAM_EDITOR* streamEditor,
   UINT32 streamFlags,
   const void* data,
   size_t length
   )
/* ++

   This function injects a section of the original indicated data back
   to the data stream.

   An MDL is allocated to describe the data section.

-- */
{
   NTSTATUS status;

   void* dataCopy = NULL;
   MDL* mdl = NULL;
   NET_BUFFER_LIST* netBufferList = NULL;

   dataCopy = ExAllocatePoolWithTag(
                  NonPagedPool,
                  length,
                  STREAM_EDITOR_MDL_DATA_TAG
                  );

   if (dataCopy == NULL)
   {
      status = STATUS_NO_MEMORY;
      goto Exit;
   }

   RtlCopyMemory(dataCopy, data, length);

   mdl = IoAllocateMdl(
            dataCopy,
            (ULONG)length,
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
                  length,
                  &netBufferList
                  );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   NT_ASSERT(!(streamFlags & FWPS_STREAM_FLAG_SEND_DISCONNECT) && 
          !(streamFlags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT));

   status = StreamOobQueueUpOutgoingData(
               streamEditor,
               netBufferList,
               FALSE,
               length,
               streamFlags,
               mdl
               );
               
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   dataCopy = NULL;
   mdl = NULL;
   netBufferList = NULL;

Exit:

   if (netBufferList != NULL)
   {
      FwpsFreeNetBufferList(netBufferList);
   }
   if (mdl != NULL)
   {
      IoFreeMdl(mdl);
   }
   if (dataCopy != NULL)
   {
      ExFreePoolWithTag(
         dataCopy, 
         STREAM_EDITOR_MDL_DATA_TAG
         );
   }
   return status;
}

NTSTATUS
StreamOobInjectReplacement(
   _Inout_ STREAM_EDITOR* streamEditor,
   UINT32 streamFlags,
   _In_opt_ MDL* data,
   size_t length
   )
/* ++

   This function injects a section of replacement data (in place of data
   removed from the stream) into the data stream.

   The MDL describes the replacement data is allocated during DriverEntry
   and does not need to be freed during injection completion.

-- */
{
   NTSTATUS status;

   NET_BUFFER_LIST* netBufferList = NULL;

   status = FwpsAllocateNetBufferAndNetBufferList(
               gNetBufferListPool,
               0,
               0,
               data,
               0,
               length,
               &netBufferList
               );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   NT_ASSERT(!(streamFlags & FWPS_STREAM_FLAG_SEND_DISCONNECT) && 
          !(streamFlags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT));

   status = StreamOobQueueUpOutgoingData(
               streamEditor,
               netBufferList,
               FALSE,
               length,
               streamFlags,
               NULL
               );

   if (!NT_SUCCESS(status))
   {
      FwpsFreeNetBufferList(netBufferList);
      goto Exit;
   }

   netBufferList = NULL;

Exit:

   if (netBufferList != NULL)
   {
      FwpsFreeNetBufferList(netBufferList);
   }

   return status;
}

NTSTATUS
StreamOobCopyDataToFlatBuffer(
   _Inout_ STREAM_EDITOR* streamEditor,
   _Inout_ NET_BUFFER_LIST* netBufferListChain,
   size_t totalDataLength,
   DWORD streamFlags
   )
/* ++

   This function copies the data described by NBL(s) into a flat buffer. 
   
   It reuses the FwpsCopyStreamDataToBuffer API (via StreamCopyDataForInspection)
   by creating a FWPS_STREAM_DATA struct.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   FWPS_STREAM_DATA streamData = {0};

   if (totalDataLength > 0)
   {
      streamData.netBufferListChain = netBufferListChain;
      streamData.dataLength =  totalDataLength;
      streamData.flags = streamFlags;

      streamData.dataOffset.netBufferList = netBufferListChain;
      streamData.dataOffset.netBuffer = 
         NET_BUFFER_LIST_FIRST_NB(streamData.dataOffset.netBufferList);
      streamData.dataOffset.mdl = 
         NET_BUFFER_CURRENT_MDL(streamData.dataOffset.netBuffer);
      streamData.dataOffset.mdlOffset = 
         NET_BUFFER_CURRENT_MDL_OFFSET(streamData.dataOffset.netBuffer);

      if (StreamCopyDataForInspection(
            streamEditor,
            &streamData
            ) == FALSE)
      {
         status = STATUS_NO_MEMORY;
      }
   }

   return status;
}

NTSTATUS
StreamOobEditData(
   _Inout_ STREAM_EDITOR* streamEditor,
   _Inout_ NET_BUFFER_LIST* netBufferListChain,
   size_t totalDataLength,
   DWORD streamFlags
   )
/* ++

   This function first copies the stream data into a flat inspection buffer;
   it then parses the buffer looking for the matching pattern. For
   non-matching sections it re-injects the data back; for a match it skips 
   over and injects an replacement section.

   If a match can not be determined due to lack of data, it injects the
   non-matching section back and moves the potential match to the beginning
   of the inspection buffer.

   When an EOF is reached, it flushes all processed stream sections back
   and re-injects the FIN back to end the stream.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   UINT i = 0;
   BOOLEAN streamModified = FALSE;
   BOOLEAN potentialMatch = FALSE;

   BYTE* dataStart;

   UINT findLength = (UINT) strlen(configStringToFind);

   status = StreamOobCopyDataToFlatBuffer(
               streamEditor,
               netBufferListChain,
               totalDataLength,
               streamFlags
               );
   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }
    
   dataStart =  (BYTE*)streamEditor->scratchBuffer + streamEditor->dataOffset;

   for (; i < streamEditor->dataLength; ++i)
   {
      if (i + findLength <= streamEditor->dataLength)
      {
         if (RtlCompareMemory(
               dataStart + i,
               configStringToFind,
               findLength
               ) == findLength)
         {
            if (i != 0)
            {
               status = StreamOobReinjectData(
                              streamEditor,
                              streamFlags, 
                              dataStart,
                              i
                              );

               if (!NT_SUCCESS(status))
               {
                  goto Exit;
               }

               streamEditor->dataOffset += i;
               streamEditor->dataLength -= i;

               i = 0;
            }

            status = StreamOobInjectReplacement(
                        streamEditor,
                        streamFlags, 
                        gStringToReplaceMdl,
                        strlen(configStringToReplace)
                        );

            if (!NT_SUCCESS(status))
            {
               goto Exit;
            }

            streamEditor->dataOffset += findLength;
            streamEditor->dataLength -= findLength;

            streamModified = TRUE;

            if (streamEditor->dataLength > 0)
            {
               dataStart =  (BYTE*)streamEditor->scratchBuffer + streamEditor->dataOffset;

               --i;               

               continue;
            }
            else
            {
               streamEditor->dataOffset = 0;
            }
         }
      }
      else
      {
         if (streamEditor->oobEditInfo.noMoreData)
         {
            break;
         }

         if (RtlCompareMemory(
               dataStart + i,
               configStringToFind,
               streamEditor->dataLength - i
               ) == streamEditor->dataLength - i)
         {
            potentialMatch = TRUE;  // this is a partial find

            status = StreamOobReinjectData(
                           streamEditor,
                           streamFlags, 
                           dataStart,
                           i
                           );

            if (!NT_SUCCESS(status))
            {
               goto Exit;
            }

            RtlMoveMemory(
               (BYTE*)streamEditor->scratchBuffer,
               dataStart + i,
               streamEditor->dataLength - i
               );

            streamEditor->dataOffset = 0;
            streamEditor->dataLength = streamEditor->dataLength - i;

            break;
         }
      }
   }

   if (streamModified && streamEditor->dataLength > 0)
   {
      status = StreamOobReinjectData(
                     streamEditor,
                     streamFlags, 
                     dataStart,
                     streamEditor->dataLength
                     );

      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }

      streamEditor->dataOffset = 0;
      streamEditor->dataLength = 0;
   }

   if (!streamModified && !potentialMatch)
   {
      if (totalDataLength > 0)
      {
         NT_ASSERT(!(streamFlags & FWPS_STREAM_FLAG_SEND_DISCONNECT) && 
                !(streamFlags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT));

         status = StreamOobQueueUpOutgoingData(
                     streamEditor,
                     netBufferListChain,
                     TRUE,
                     totalDataLength,
                     streamFlags,
                     NULL
                     );

         if (!NT_SUCCESS(status))
         {
            goto Exit;
         }

         netBufferListChain = NULL;
      }
      else if (streamEditor->dataLength > 0)
      {
         status = StreamOobReinjectData(
                        streamEditor,
                        streamFlags, 
                        dataStart,
                        streamEditor->dataLength
                        );

         if (!NT_SUCCESS(status))
         {
            goto Exit;
         }
      }

      streamEditor->dataOffset = 0;
      streamEditor->dataLength = 0;
   }

   if (streamEditor->oobEditInfo.nblEof != NULL)
   {
      status = StreamOobFlushOutgoingData(streamEditor);

      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }

      status = FwpsStreamInjectAsync(
                  gInjectionHandle,
                  NULL,
                  0,
                  streamEditor->oobEditInfo.flowId,
                  streamEditor->oobEditInfo.calloutId,
                  streamEditor->oobEditInfo.layerId,
                  streamFlags | (configInspectionOutbound ? FWPS_STREAM_FLAG_SEND_DISCONNECT :
                                                            FWPS_STREAM_FLAG_RECEIVE_DISCONNECT),
                  streamEditor->oobEditInfo.nblEof,
                  0,
                  StreamOobInjectCompletionFn,
                  NULL
                  );

      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }

      streamEditor->oobEditInfo.nblEof = NULL;
      streamEditor->oobEditInfo.noMoreData = FALSE;     
   }

Exit:

   if (netBufferListChain != NULL)
   {
      FwpsDiscardClonedStreamData(
         netBufferListChain,
         0,
         FALSE
         );
   }

   if (streamEditor->oobEditInfo.nblEof != NULL)
   {
      FwpsDiscardClonedStreamData(
         streamEditor->oobEditInfo.nblEof,
         0,
         FALSE
         );

      streamEditor->oobEditInfo.nblEof = NULL;
   }

   return status;
}

_IRQL_requires_same_
_Function_class_(KSTART_ROUTINE)
void
StreamOobEditWorker(
   _In_ void* StartContext
   )
/* ++

   This function waits for an event which gets signalled when there is data
   waiting to be inspected.

   Once awaken, the worker thread edits the stream until all stream data is
   processed (and then it waits for more work again).

   When requested to shutdown, it will finish the editing task and enters
   "shutdown" state.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   NET_BUFFER_LIST* netBufferListChain = NULL;
   size_t totalDataLength;
   STREAM_EDITOR* streamEditor = (STREAM_EDITOR*)StartContext;
   DWORD streamFlags;

   for(;;)
   {
      KLOCK_QUEUE_HANDLE editLockHandle;

      KeWaitForSingleObject(
         &streamEditor->oobEditInfo.editEvent,
         Executive, 
         KernelMode, 
         FALSE, 
         NULL
         );

      if (streamEditor->oobEditInfo.editState == OOB_EDIT_ERROR ||
          streamEditor->oobEditInfo.editState == OOB_EDIT_SHUT_DOWN)
      {
         break;
      }

      KeAcquireInStackQueuedSpinLock(
         &streamEditor->oobEditInfo.editLock,
         &editLockHandle
         );

      NT_ASSERT(streamEditor->oobEditInfo.editState == OOB_EDIT_PROCESSING ||
             streamEditor->oobEditInfo.editState == OOB_EDIT_BUSY);

      netBufferListChain = streamEditor->oobEditInfo.nblHead;
      totalDataLength = streamEditor->oobEditInfo.totalDataLength;
      streamFlags = streamEditor->oobEditInfo.streamFlags;

      streamEditor->oobEditInfo.nblHead = NULL;
      streamEditor->oobEditInfo.nblTail = NULL;
      streamEditor->oobEditInfo.totalDataLength = 0;

      KeReleaseInStackQueuedSpinLock(&editLockHandle);

      _Analysis_assume_(netBufferListChain != NULL);
      status = StreamOobEditData(
                  streamEditor,
                  netBufferListChain,
                  totalDataLength,
                  streamFlags
                  );

      if (!NT_SUCCESS(status))
      {
         streamEditor->oobEditInfo.editState = OOB_EDIT_ERROR;
         break;
      }

      if (streamEditor->oobEditInfo.editState == OOB_EDIT_BUSY)
      {
         NTSTATUS streamContinueStatus;
         streamEditor->oobEditInfo.editState = OOB_EDIT_PROCESSING;
         
         streamContinueStatus = FwpsStreamContinue(
                                   streamEditor->oobEditInfo.flowId, 
                                   streamEditor->oobEditInfo.calloutId,
                                   streamEditor->oobEditInfo.layerId,
                                   streamEditor->oobEditInfo.streamFlags
                                   );

         if (!NT_SUCCESS(streamContinueStatus))
         {
            streamEditor->oobEditInfo.editState = OOB_EDIT_ERROR;
            break;
         }
      }

      KeAcquireInStackQueuedSpinLock(
         &streamEditor->oobEditInfo.editLock,
         &editLockHandle
         );

      if (streamEditor->oobEditInfo.nblHead == NULL)
      {
         if (!streamEditor->oobEditInfo.shuttingDown)
         {
            streamEditor->oobEditInfo.editState = OOB_EDIT_IDLE;
            KeClearEvent(&streamEditor->oobEditInfo.editEvent);
         }
         else
         {
            streamEditor->oobEditInfo.editState = OOB_EDIT_SHUT_DOWN;
         }
      }

      KeReleaseInStackQueuedSpinLock(&editLockHandle);
   }

   PsTerminateSystemThread(status);
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

void 
NTAPI
StreamOobEditClassify(
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
StreamOobEditClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
/* ++

   This is the ClassifyFn function registered by the OOB stream edit callout.

   An OOB stream modification callout blocks all indicated data after cloning
   them for processing by a kernel mode worker thread (or marshalling the data
   to user mode for inspection); the resultant/edited data will then be put
   back to the stream via the stream injection API.

   For such a callout, the processed data must be (re-)injected back to the 
   stream from within the ClassifyFn.

-- */
{
   FWPS_STREAM_CALLOUT_IO_PACKET* ioPacket;
   FWPS_STREAM_DATA* streamData;

   UINT findLength = (UINT) strlen(configStringToFind);

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

   if ((streamData->dataLength < findLength) && 
       !(classifyOut->flags & FWPS_CLASSIFY_OUT_FLAG_NO_MORE_DATA))
   {
      ioPacket->streamAction = FWPS_STREAM_ACTION_NEED_MORE_DATA;
      ioPacket->countBytesRequired = findLength;

      classifyOut->actionType = FWP_ACTION_NONE;
      goto Exit;
   }

   StreamOobEdit(
      &gStreamEditor,
      inFixedValues,
      inMetaValues,
      filter,
      streamData,
      ioPacket,
      classifyOut
      );

   if (classifyOut->actionType == FWP_ACTION_BLOCK || 
       classifyOut->actionType == FWP_ACTION_PERMIT)
   {
      if (!(streamData->flags & FWPS_STREAM_FLAG_SEND_DISCONNECT) && 
          !(streamData->flags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT))
      {
         NTSTATUS status = StreamOobFlushOutgoingData( &gStreamEditor);
         if (!NT_SUCCESS(status))
         {
            ioPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
            classifyOut->actionType = FWP_ACTION_NONE;
         }
      }
   }

Exit:

   return;
}
