/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   This file implements the classifyFn callout functions for the ALE connect,
   recv-accept, and transport callouts. In addition the system worker thread 
   that performs the actual packet inspection is also implemented here along 
   with the eventing mechanisms shared between the classify function and the
   worker thread.

   connect/Packet inspection is done out-of-band by a threaded DPC
   using the reference-drop-clone-reinject as well as ALE pend/complete 
   mechanism. Therefore the sample can serve as a base in scenarios where 
   filtering decision cannot be made within the classifyFn() callout and 
   instead must be made, for example, by an user-mode application.

Environment:

    Kernel mode

--*/


#include <ntddk.h>

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union

#include <fwpsk.h>

#pragma warning(pop)

#include <fwpmk.h>

#include "inspect.h"
#include "utils.h"

#if(NTDDI_VERSION >= NTDDI_WIN7)

void
TLInspectALEConnectClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_opt_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )

#else

void
TLInspectALEConnectClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

/* ++

   This is the classifyFn function for the ALE connect (v4 and v6) callout.
   For an initial classify (where the FWP_CONDITION_FLAG_IS_REAUTHORIZE flag
   is not set), it is queued to the per-processor connection list for inspection
   by the threaded DPC. For re-auth, we first check if it is triggered by an ealier
   FwpsCompleteOperation call by looking for an pended connect that has been
   inspected. If found, we return the inspection result; otherwise we can
   conclude that the re-auth is triggered by policy change so we queue
   it to the packet queue to be process by the worker thread like any other regular packets.

-- */
{
   NTSTATUS status;

   KLOCK_QUEUE_HANDLE connListLockHandle;
   KLOCK_QUEUE_HANDLE classifiedConnListLockHandle;

   TL_INSPECT_PENDED_PACKET* pendedConnect = NULL;
   TL_INSPECT_PENDED_PACKET* connEntry;
   TL_INSPECT_PENDED_PACKET* pendedPacket = NULL;

   ADDRESS_FAMILY addressFamily;
   FWPS_PACKET_INJECTION_STATE packetState;
   BOOL writesAccess = TRUE;
   ULONG processorIndex = KeGetCurrentProcessorNumber();

#if(NTDDI_VERSION >= NTDDI_WIN7)
   UNREFERENCED_PARAMETER(classifyContext);
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
   UNREFERENCED_PARAMETER(filter);
   UNREFERENCED_PARAMETER(flowContext);

   //
   // We don't have the necessary right to alter the classify, exit.
   //
   if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
   {
      TraceLoggingWrite(
         gTlgHandle,
         "ConnectClassifyFn",
         TraceLoggingWideString(L"No writes access", "Message")
         );

      writesAccess = FALSE;

      //
      // Even if we do not have FWPS_RIGHT_ACTION_WRITE rights, we may still need to
      // re-inject the packet. This is necessary when this is a FwpsCompleteOperation() triggered re-auth,
      // which was invoked by this callout driver.
      //
      if (IsAleReauthorizeDueToClassifyCompletion(inFixedValues))
      {
         TraceLoggingWrite(
            gTlgHandle,
            "ConnectClassifyFn",
            TraceLoggingWideString(L"No writes access, but NOT exiting as we may need to reinject the packet", "Message")
            );
      }
      else
      {
         TraceLoggingWrite(
            gTlgHandle,
            "ConnectClassifyFn",
            TraceLoggingWideString(L"Exiting with no write access", "Message")
            );
         goto Exit;
      }
   }

   if (layerData != NULL)
   {
      //
      // We don't re-inspect packets that we've inspected earlier.
      //
      packetState = FwpsQueryPacketInjectionState(
                     gInjectionHandle,
                     layerData,
                     NULL
                     );

      if ((packetState == FWPS_PACKET_INJECTED_BY_SELF) ||
          (packetState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF))
      {

         TraceLoggingWrite(
            gTlgHandle,
            "ConnectClassifyFn",
            TraceLoggingWideString(L"Permitting due to injected by self", "Message"),
            TraceLoggingUInt32(packetState, "PacketState")
            );

         classifyOut->actionType = FWP_ACTION_PERMIT;
         if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
         {
            classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         }

         goto Exit;
      }
   }

   addressFamily = GetAddressFamilyForLayer(inFixedValues->layerId);

   if (!IsAleReauthorize(inFixedValues))
   {

      TraceLoggingWrite(
         gTlgHandle,
         "ConnectClassifyFn",
         TraceLoggingWideString(L"Non-reauth connection", "Message")
         );

      //
      // If the classify is the initial authorization for a connection, we 
      // queue it to the pended connection list and queue the threaded DPC
      // (if not already queued) for out-of-band processing.
      //
      pendedConnect = AllocateAndInitializePendedPacket(
                           inFixedValues,
                           inMetaValues,
                           addressFamily,
                           layerData,
                           TL_INSPECT_CONNECT_PACKET,
                           FWP_DIRECTION_OUTBOUND
                           );

      if (pendedConnect == NULL)
      {
         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         goto Exit;
      }

      NT_ASSERT(FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues,
                                               FWPS_METADATA_FIELD_COMPLETION_HANDLE));

      //
      // Pend the ALE_AUTH_CONNECT classify.
      //
      status = FwpsPendOperation(
                  inMetaValues->completionHandle,
                  &pendedConnect->completionContext
                  );

      TraceLoggingWrite(
         gTlgHandle,
         "ConnectClassifyFn",
         TraceLoggingWideString(L"FwpsPendOperation() invoked", "Message"),
         TraceLoggingUInt32(status, "FwpsPendOperation::ntStatus"),
         TraceLoggingUInt32((UINT32) processorIndex, "ProcessorIndex")
         );

      if (!NT_SUCCESS(status))
      {
         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         goto Exit;
      }

      KeAcquireInStackQueuedSpinLock(
         &gProcessorQueue[processorIndex].connListLock,
         &connListLockHandle
         );

      InsertTailList(&gProcessorQueue[processorIndex].connList, &pendedConnect->listEntry);
      pendedConnect = NULL; // ownership transferred


      if (gProcessorQueue[processorIndex].isDpcQueued == FALSE)
      {
         gProcessorQueue[processorIndex].isDpcQueued = TRUE;
         KeInsertQueueDpc(
            &gProcessorQueue[processorIndex].kdpc,
            NULL,
            NULL
            );
      }

      KeReleaseInStackQueuedSpinLock(&connListLockHandle);

      classifyOut->actionType = FWP_ACTION_BLOCK;
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

   }
   else // re-auth @ ALE_AUTH_CONNECT
   {
      TraceLoggingWrite(
         gTlgHandle,
         "ConnectClassifyFn",
         TraceLoggingWideString(L"[Reauth] Is re-auth", "Message")
         );

      FWP_DIRECTION packetDirection;

      //
      // The classify is the re-authorization for an existing connection, it 
      // could have been triggered for one of the three cases --
      //
      //    1) The re-auth is triggered by a FwpsCompleteOperation call to
      //       complete a ALE_AUTH_CONNECT classify pended earlier. 
      //    2) The re-auth is triggered by an outbound packet sent immediately
      //       after a policy change at ALE_AUTH_CONNECT layer.
      //    3) The re-auth is triggered by an inbound packet received 
      //       immediately after a policy change at ALE_AUTH_CONNECT layer.
      //

      NT_ASSERT(FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues,
                                               FWPS_METADATA_FIELD_PACKET_DIRECTION));
      packetDirection = inMetaValues->packetDirection;

      if (packetDirection == FWP_DIRECTION_OUTBOUND)
      {
         LIST_ENTRY* listEntry;
         BOOLEAN authComplete = FALSE;

         //
         // We first check whether this is a FwpsCompleteOperation-triggered
         // reauth by looking for a pended connect that has the inspection
         // decision recorded. If found, we return that decision.
         //

         KeAcquireInStackQueuedSpinLock(
            &gProcessorQueue[processorIndex].classifiedConnListLock,
            &classifiedConnListLockHandle
            );

         for (listEntry = gProcessorQueue[processorIndex].classifiedConnList.Flink;
              listEntry != &gProcessorQueue[processorIndex].classifiedConnList;
              listEntry = listEntry->Flink)
         {
            connEntry = CONTAINING_RECORD(
                            listEntry,
                            TL_INSPECT_PENDED_PACKET,
                            listEntry
                            );

            if (IsMatchingConnectPacket(
                     inFixedValues,
                     addressFamily,
                     packetDirection,
                     connEntry
                  ) && (connEntry->authConnectDecision != 0))
            {

               TraceLoggingWrite(
                  gTlgHandle,
                  "ClassifyFn",
                  TraceLoggingWideString(L"[Reauth] Found matching packet", "Message")
                  );

               // We found a match.
               pendedConnect = connEntry;

               NT_ASSERT((pendedConnect->authConnectDecision == FWP_ACTION_PERMIT) ||
                         (pendedConnect->authConnectDecision == FWP_ACTION_BLOCK));

               if (writesAccess)
               {
                  classifyOut->actionType = pendedConnect->authConnectDecision;
                  if (classifyOut->actionType == FWP_ACTION_BLOCK ||
                     filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
                  {
                     classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
                  }
                  TraceLoggingWrite(
                     gTlgHandle,
                     "ConnectClassifyFn",
                     TraceLoggingWideString(L"[Reauth] Setting action based on found packet", "Message")
                     );
               }
               else
               {
                  TraceLoggingWrite(
                     gTlgHandle,
                     "ConnectClassifyFn",
                     TraceLoggingWideString(L"[Reauth] Cannot set action based on found packet as classifyOut can't be modified due to no writes access", "Message")
                     );
               }
               pendedConnect = NULL;
               authComplete = TRUE;
               break;
            }
         }

         KeReleaseInStackQueuedSpinLock(&classifiedConnListLockHandle);

         if (authComplete)
         {
            goto Exit;
         }
      }

      //
      // If we reach here it means either:
      //    1 - This is a policy change triggered re-auth for an pre-existing connection.
      //        For such a packet (inbound or outbound) we queue it to the packet queue and
      //        inspect it just like other regular data packets from TRANSPORT layers.
      //    2 - This is a re-auth triggered by FwpsCompleteOperation() for which we
      //        did not find a previous packet. In this case layerData is NULL, so we
      //        block the packet.
      //

      if (writesAccess == FALSE)
      {
         TraceLoggingWrite(
            gTlgHandle,
            "ConnectClassifyFn",
            TraceLoggingWideString(L"[Reauth] Exiting due to no writes access", "Message")
            );
         goto Exit;
      }

      if (layerData == NULL)
      {
         TraceLoggingWrite(
            gTlgHandle,
            "ConnectClassifyFn",
            TraceLoggingWideString(L"[Reauth] LayerData is NULL. Setting action to BLOCK and exiting", "Action")
            );
         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
         goto Exit;
      }

      //
      // This is a policy change triggered re-auth. Here, we should re-inject the packet
      //

      TraceLoggingWrite(
         gTlgHandle,
         "ConnectClassifyFn",
         TraceLoggingWideString(L"[Reauth] Queuing packet for re-inject", "Message")
         );

      pendedPacket = AllocateAndInitializePendedPacket(
                        inFixedValues,
                        inMetaValues,
                        addressFamily,
                        layerData,
                        TL_INSPECT_REAUTH_PACKET,
                        packetDirection
                        );

      if (pendedPacket == NULL)
      {
         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         goto Exit;
      }

      if (packetDirection == FWP_DIRECTION_INBOUND)
      {
         pendedPacket->ipSecProtected = IsSecureConnection(inFixedValues);
      }

      KeAcquireInStackQueuedSpinLock(
         &gProcessorQueue[processorIndex].connListLock,
         &connListLockHandle
         );

      if (!gDriverUnloading)
      {
         InsertTailList(&gProcessorQueue[processorIndex].connList, &pendedPacket->listEntry);
         pendedPacket = NULL; // ownership transferred

         if (gProcessorQueue[processorIndex].isDpcQueued == FALSE)
         {
            gProcessorQueue[processorIndex].isDpcQueued = TRUE;
            KeInsertQueueDpc(
               &gProcessorQueue[processorIndex].kdpc,
               NULL,
               NULL
               );
         }

         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
      }
      else
      {
         //
         // Driver is being unloaded, permit any connect classify.
         //

         classifyOut->actionType = FWP_ACTION_PERMIT;
         if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
         {
            classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         }
      }

      KeReleaseInStackQueuedSpinLock(&connListLockHandle);
   }

Exit:

   if (pendedPacket != NULL)
   {
      FreePendedPacket(pendedPacket);
   }
   if (pendedConnect != NULL)
   {
      FreePendedPacket(pendedConnect);
   }

   return;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

void
TLInspectALERecvAcceptClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_opt_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )

#else

void
TLInspectALERecvAcceptClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
/* ++

   This is the classifyFn function for the ALE Recv-Accept (v4 and v6) callout.
   For an initial classify (where the FWP_CONDITION_FLAG_IS_REAUTHORIZE flag
   is not set), it is queued to the connection list for inspection by the
   worker thread. For re-auth, it is queued to the packet queue to be process 
   by the worker thread like any other regular packets.

-- */
{
   NTSTATUS status;

   KLOCK_QUEUE_HANDLE connListLockHandle;

   TL_INSPECT_PENDED_PACKET* pendedRecvAccept = NULL;
   TL_INSPECT_PENDED_PACKET* pendedPacket = NULL;

   ADDRESS_FAMILY addressFamily;
   FWPS_PACKET_INJECTION_STATE packetState;
   ULONG processorIndex = KeGetCurrentProcessorNumber();

#if(NTDDI_VERSION >= NTDDI_WIN7)
   UNREFERENCED_PARAMETER(classifyContext);
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
   UNREFERENCED_PARAMETER(filter);
   UNREFERENCED_PARAMETER(flowContext);

   //
   // We don't have the necessary right to alter the classify, exit.
   //
   if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
   {
      TraceLoggingWrite(
         gTlgHandle,
         "RecvAcceptClassifyFn",
         TraceLoggingWideString(L"Exiting due to no writes access", "Message")
         );
      goto Exit;
   }

  NT_ASSERT(layerData != NULL);
  _Analysis_assume_(layerData != NULL);

   //
   // We don't re-inspect packets that we've inspected earlier.
   //
   packetState = FwpsQueryPacketInjectionState(
                     gInjectionHandle,
                     layerData,
                     NULL
                     );

   if ((packetState == FWPS_PACKET_INJECTED_BY_SELF) ||
       (packetState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF))
   {
      classifyOut->actionType = FWP_ACTION_PERMIT;
      if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
      {
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      }

      TraceLoggingWrite(
         gTlgHandle,
         "RecvAcceptClassifyFn",
         TraceLoggingWideString(L"Permitting due to injected by self", "Message"),
         TraceLoggingUInt32(packetState, "PacketState")
         );

      goto Exit;
   }

   addressFamily = GetAddressFamilyForLayer(inFixedValues->layerId);

   if (!IsAleReauthorize(inFixedValues))
   {
      TraceLoggingWrite(
         gTlgHandle,
         "RecvAcceptClassifyFn",
         TraceLoggingWideString(L"Non re-auth", "Message")
         );
      //
      // If the classify is the initial authorization for a connection, we 
      // queue it to the pended connection list and notify the worker thread
      // for out-of-band processing.
      //
      pendedRecvAccept = AllocateAndInitializePendedPacket(
                              inFixedValues,
                              inMetaValues,
                              addressFamily,
                              layerData,
                              TL_INSPECT_CONNECT_PACKET,
                              FWP_DIRECTION_INBOUND
                              );

      if (pendedRecvAccept == NULL)
      {
         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         goto Exit;
      }

      NT_ASSERT(FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, 
                                            FWPS_METADATA_FIELD_COMPLETION_HANDLE));

      //
      // Pend the ALE_AUTH_RECV_ACCEPT classify.
      //
      status = FwpsPendOperation(
                  inMetaValues->completionHandle,
                  &pendedRecvAccept->completionContext
                  );

      TraceLoggingWrite(
         gTlgHandle,
         "RecvAcceptClassifyFn",
         TraceLoggingWideString(L"Invoked FwpsPendOperation()", "Message"),
         TraceLoggingUInt32((UINT32) status, "FwpsPendOperation::ntStatus")
         );

      if (!NT_SUCCESS(status))
      {
         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         goto Exit;
      }

      KeAcquireInStackQueuedSpinLock(
         &gProcessorQueue[processorIndex].connListLock,
         &connListLockHandle
         );

      InsertTailList(&gProcessorQueue[processorIndex].connList, &pendedRecvAccept->listEntry);
      pendedRecvAccept = NULL; // ownership transferred

      if (gProcessorQueue[processorIndex].isDpcQueued == FALSE)
      {
         gProcessorQueue[processorIndex].isDpcQueued = TRUE;
         KeInsertQueueDpc(
            &gProcessorQueue[processorIndex].kdpc,
            NULL,
            NULL
            );
      }

      KeReleaseInStackQueuedSpinLock(&connListLockHandle);

      classifyOut->actionType = FWP_ACTION_BLOCK;
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

   }
   else // re-auth @ ALE_AUTH_RECV_ACCEPT
   {
      TraceLoggingWrite(
         gTlgHandle,
         "RecvAcceptClassifyFn",
         TraceLoggingWideString(L"[Reauth]", "Message")
         );

      FWP_DIRECTION packetDirection;

      //
      // The classify is the re-authorization for a existing connection, it 
      // could have been triggered for one of the two cases --
      //
      //    1) The re-auth is triggered by an outbound packet sent immediately
      //       after a policy change at ALE_AUTH_RECV_ACCEPT layer.
      //    2) The re-auth is triggered by an inbound packet received 
      //       immediately after a policy change at ALE_AUTH_RECV_ACCEPT layer.
      //

      NT_ASSERT(FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, 
                                            FWPS_METADATA_FIELD_PACKET_DIRECTION));
      packetDirection = inMetaValues->packetDirection;

      pendedPacket = AllocateAndInitializePendedPacket(
                        inFixedValues,
                        inMetaValues,
                        addressFamily,
                        layerData,
                        TL_INSPECT_REAUTH_PACKET,
                        packetDirection
                        );

      if (pendedPacket == NULL)
      {
         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         goto Exit;
      }

      if (packetDirection == FWP_DIRECTION_INBOUND)
      {
         pendedPacket->ipSecProtected = IsSecureConnection(inFixedValues);
      }

      KeAcquireInStackQueuedSpinLock(
         &gProcessorQueue[processorIndex].connListLock,
         &connListLockHandle
         );

      if (!gDriverUnloading)
      {
         InsertTailList(&gProcessorQueue[processorIndex].connList, &pendedPacket->listEntry);
         pendedPacket = NULL; // ownership transferred

         if (gProcessorQueue[processorIndex].isDpcQueued == FALSE)
         {
            gProcessorQueue[processorIndex].isDpcQueued = TRUE;
            KeInsertQueueDpc(
               &gProcessorQueue[processorIndex].kdpc,
               NULL,
               NULL
               );
         }

         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
      }
      else
      {
         //
         // Driver is being unloaded, permit any connect classify.
         //

         classifyOut->actionType = FWP_ACTION_PERMIT;
         if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
         {
            classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         }
      }

      KeReleaseInStackQueuedSpinLock(&connListLockHandle);
   }

Exit:

   if (pendedPacket != NULL)
   {
      FreePendedPacket(pendedPacket);
   }
   if (pendedRecvAccept != NULL)
   {
      FreePendedPacket(pendedRecvAccept);
   }

   return;
}

#if(NTDDI_VERSION >= NTDDI_WIN7)

void
TLInspectTransportClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_opt_ const void* classifyContext,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )

#else

void
TLInspectTransportClassify(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _Inout_opt_ void* layerData,
   _In_ const FWPS_FILTER* filter,
   _In_ UINT64 flowContext,
   _Inout_ FWPS_CLASSIFY_OUT* classifyOut
   )

#endif
/* ++

   This is the classifyFn function for the Transport (v4 and v6) callout.
   packets (inbound or outbound) are ueued to the packet queue to be processed 
   by the worker thread.

-- */
{

   KLOCK_QUEUE_HANDLE connListLockHandle;

   TL_INSPECT_PENDED_PACKET* pendedPacket = NULL;
   FWP_DIRECTION packetDirection;

   ADDRESS_FAMILY addressFamily;
   FWPS_PACKET_INJECTION_STATE packetState;
   ULONG processorIndex = KeGetCurrentProcessorNumber();

#if(NTDDI_VERSION >= NTDDI_WIN7)
   UNREFERENCED_PARAMETER(classifyContext);
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
   UNREFERENCED_PARAMETER(filter);
   UNREFERENCED_PARAMETER(flowContext);

   //
   // We don't have the necessary right to alter the classify, exit.
   //
   if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
   {
      TraceLoggingWrite(
         gTlgHandle,
         "TransportClassifyFn",
         TraceLoggingWideString(L"Exiting due to no writes access", "Message")
         );
      goto Exit;
   }

  NT_ASSERT(layerData != NULL);
  _Analysis_assume_(layerData != NULL);

   //
   // We don't re-inspect packets that we've inspected earlier.
   //
   packetState = FwpsQueryPacketInjectionState(
                     gInjectionHandle,
                     layerData,
                     NULL
                     );

   if ((packetState == FWPS_PACKET_INJECTED_BY_SELF) ||
       (packetState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF))
   {
      classifyOut->actionType = FWP_ACTION_PERMIT;
      if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
      {
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      }

      TraceLoggingWrite(
         gTlgHandle,
         "TransportClassifyFn",
         TraceLoggingWideString(L"Permitting due to injected by self", "Message"),
         TraceLoggingUInt32(packetState, "PacketState")
         );

      goto Exit;
   }

   addressFamily = GetAddressFamilyForLayer(inFixedValues->layerId);

   packetDirection = 
      GetPacketDirectionForLayer(inFixedValues->layerId);

   if (packetDirection == FWP_DIRECTION_INBOUND)
   {
      if (IsAleClassifyRequired(inFixedValues, inMetaValues))
      {
         //
         // Inbound transport packets that are destined to ALE Recv-Accept 
         // layers, for initial authorization or reauth, should be inspected 
         // at the ALE layer. We permit it from Tranport here.
         //
         classifyOut->actionType = FWP_ACTION_PERMIT;
         if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
         {
            classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         }

         TraceLoggingWrite(
            gTlgHandle,
            "TransportClassifyFn",
            TraceLoggingWideString(L"Permitting as packet will be inspected at ALE_AUTH_RECV_ACCEPT layer", "Message")
            );

         goto Exit;
      }
      else
      {
         //
         // To be compatible with Vista's IpSec implementation, we must not
         // intercept not-yet-detunneled IpSec traffic.
         //
         FWPS_PACKET_LIST_INFORMATION packetInfo = {0};
         FwpsGetPacketListSecurityInformation(
            layerData,
            FWPS_PACKET_LIST_INFORMATION_QUERY_IPSEC |
            FWPS_PACKET_LIST_INFORMATION_QUERY_INBOUND,
            &packetInfo
            );

         if (packetInfo.ipsecInformation.inbound.isTunnelMode &&
             !packetInfo.ipsecInformation.inbound.isDeTunneled)
         {
            classifyOut->actionType = FWP_ACTION_PERMIT;
            if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
            {
               classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
            }
            goto Exit;
         }
      }
   }

   pendedPacket = AllocateAndInitializePendedPacket(
                     inFixedValues,
                     inMetaValues,
                     addressFamily,
                     layerData,
                     TL_INSPECT_DATA_PACKET,
                     packetDirection
                     );

   if (pendedPacket == NULL)
   {
      classifyOut->actionType = FWP_ACTION_BLOCK;
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      goto Exit;
   }

   KeAcquireInStackQueuedSpinLock(
      &gProcessorQueue[processorIndex].connListLock,
      &connListLockHandle
      );

   if (!gDriverUnloading)
   {
      InsertTailList(&gProcessorQueue[processorIndex].connList, &pendedPacket->listEntry);
      pendedPacket = NULL; // ownership transferred

      if (gProcessorQueue[processorIndex].isDpcQueued == FALSE)
      {
         gProcessorQueue[processorIndex].isDpcQueued = TRUE;
         KeInsertQueueDpc(
            &gProcessorQueue[processorIndex].kdpc,
            NULL,
            NULL
            );
      }

      classifyOut->actionType = FWP_ACTION_BLOCK;
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

      TraceLoggingWrite(
         gTlgHandle,
         "TransportClassifyFn",
         TraceLoggingWideString(L"Blocking packet, but will be re-injected in work item", "Message")
         );
   }
   else
   {
      //
      // Driver is being unloaded, permit any connect classify.
      //
      classifyOut->actionType = FWP_ACTION_PERMIT;
      if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
      {
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      }
   }

   KeReleaseInStackQueuedSpinLock(&connListLockHandle);

Exit:

   if (pendedPacket != NULL)
   {
      FreePendedPacket(pendedPacket);
   }

   return;
}

NTSTATUS
TLInspectALEConnectNotify(
   _In_  FWPS_CALLOUT_NOTIFY_TYPE notifyType,
   _In_ const GUID* filterKey,
   _Inout_ const FWPS_FILTER* filter
   )
{
   UNREFERENCED_PARAMETER(notifyType);
   UNREFERENCED_PARAMETER(filterKey);
   UNREFERENCED_PARAMETER(filter);

   return STATUS_SUCCESS;
}

NTSTATUS
TLInspectALERecvAcceptNotify(
   _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
   _In_ const GUID* filterKey,
   _Inout_ const FWPS_FILTER* filter
   )
{
   UNREFERENCED_PARAMETER(notifyType);
   UNREFERENCED_PARAMETER(filterKey);
   UNREFERENCED_PARAMETER(filter);

   return STATUS_SUCCESS;
}

NTSTATUS
TLInspectTransportNotify(
   _In_ FWPS_CALLOUT_NOTIFY_TYPE notifyType,
   _In_ const GUID* filterKey,
   _Inout_ const FWPS_FILTER* filter
   )
{
   UNREFERENCED_PARAMETER(notifyType);
   UNREFERENCED_PARAMETER(filterKey);
   UNREFERENCED_PARAMETER(filter);

   return STATUS_SUCCESS;
}

void TLInspectInjectComplete(
   _Inout_ void* context,
   _Inout_ NET_BUFFER_LIST* netBufferList,
   _In_ BOOLEAN dispatchLevel
   )
{
   TL_INSPECT_PENDED_PACKET* packet = context;

   UNREFERENCED_PARAMETER(dispatchLevel);   

   FwpsFreeCloneNetBufferList(netBufferList, 0);

   FreePendedPacket(packet);
}

NTSTATUS
TLInspectCloneReinjectOutbound(
   _Inout_ TL_INSPECT_PENDED_PACKET* packet
   )
/* ++

   This function clones the outbound net buffer list and reinject it back.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   NET_BUFFER_LIST* clonedNetBufferList = NULL;
   FWPS_TRANSPORT_SEND_PARAMS sendArgs = {0};

   status = FwpsAllocateCloneNetBufferList(
               packet->netBufferList,
               NULL,
               NULL,
               0,
               &clonedNetBufferList
               );

   TraceLoggingWrite(
      gTlgHandle,
      "WorkerThread",
      TraceLoggingWideString(L"[TLInspectCloneReinjectOutbound] Invoked FwpsAllocateCloneNetBufferList()", "Message"),
      TraceLoggingUInt32(status, "FwpsAllocateCloneNetBufferList::ntStatus")
      );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   sendArgs.remoteAddress = (UINT8*)(&packet->remoteAddr);
   sendArgs.remoteScopeId = packet->remoteScopeId;
   sendArgs.controlData = packet->controlData;
   sendArgs.controlDataLength = packet->controlDataLength;

   //
   // Send-inject the cloned net buffer list.
   //

   status = FwpsInjectTransportSendAsync(
               gInjectionHandle,
               NULL,
               packet->endpointHandle,
               0,
               &sendArgs,
               packet->addressFamily,
               packet->compartmentId,
               clonedNetBufferList,
               TLInspectInjectComplete,
               packet
               );

   TraceLoggingWrite(
      gTlgHandle,
      "WorkerThread",
      TraceLoggingWideString(L"[TLInspectCloneReinjectOutbound] Invoked FwpsInjectTransportSendAsync()", "Message"),
      TraceLoggingUInt32(status, "FwpsInjectTransportSendAsync::ntStatus")
      );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   clonedNetBufferList = NULL; // ownership transferred to the 
                               // completion function.

Exit:

   if (clonedNetBufferList != NULL)
   {
      FwpsFreeCloneNetBufferList(clonedNetBufferList, 0);
   }

   return status;
}

NTSTATUS
TLInspectCloneReinjectInbound(
   _Inout_ TL_INSPECT_PENDED_PACKET* packet
   )
/* ++

   This function clones the inbound net buffer list and, if needed, 
   rebuild the IP header to remove the IpSec headers and receive-injects 
   the clone back to the tcpip stack.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;

   NET_BUFFER_LIST* clonedNetBufferList = NULL;
   NET_BUFFER* netBuffer;
   ULONG nblOffset;
   NDIS_STATUS ndisStatus;

   //
   // For inbound net buffer list, we can assume it contains only one 
   // net buffer.
   //
   netBuffer = NET_BUFFER_LIST_FIRST_NB(packet->netBufferList);
   
   nblOffset = NET_BUFFER_DATA_OFFSET(netBuffer);

   //
   // The TCP/IP stack could have retreated the net buffer list by the 
   // transportHeaderSize amount; detect the condition here to avoid
   // retreating twice.
   //
   if (nblOffset != packet->nblOffset)
   {
      NT_ASSERT(packet->nblOffset - nblOffset == packet->transportHeaderSize);
      packet->transportHeaderSize = 0;
   }

   //
   // Adjust the net buffer list offset to the start of the IP header.
   //
   ndisStatus = NdisRetreatNetBufferDataStart(
      netBuffer,
      packet->ipHeaderSize + packet->transportHeaderSize,
      0,
      NULL
      );
   _Analysis_assume_(ndisStatus == NDIS_STATUS_SUCCESS);

   //
   // Note that the clone will inherit the original net buffer list's offset.
   //

   status = FwpsAllocateCloneNetBufferList(
               packet->netBufferList,
               NULL,
               NULL,
               0,
               &clonedNetBufferList
               );

   TraceLoggingWrite(
      gTlgHandle,
      "WorkerThread",
      TraceLoggingWideString(L"[TLInspectCloneReinjectInbound] Invoked FwpsAllocateCloneNetBufferList()", "Message"),
      TraceLoggingUInt32(status, "FwpsAllocateCloneNetBufferList::ntStatus")
      );

   //
   // Undo the adjustment on the original net buffer list.
   //

   NdisAdvanceNetBufferDataStart(
      netBuffer,
      packet->ipHeaderSize + packet->transportHeaderSize,
      FALSE,
      NULL
      );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   if (packet->ipSecProtected)
   {
      //
      // When an IpSec protected packet is indicated to AUTH_RECV_ACCEPT or 
      // INBOUND_TRANSPORT layers, for performance reasons the tcpip stack
      // does not remove the AH/ESP header from the packet. And such 
      // packets cannot be recv-injected back to the stack w/o removing the
      // AH/ESP header. Therefore before re-injection we need to "re-build"
      // the cloned packet.
      // 
      status = FwpsConstructIpHeaderForTransportPacket(
                  clonedNetBufferList,
                  packet->ipHeaderSize,
                  packet->addressFamily,
                  (UINT8*)&packet->remoteAddr, 
                  (UINT8*)&packet->localAddr,  
                  packet->protocol,
                  0,
                  NULL,
                  0,
                  0,
                  NULL,
                  0,
                  0
                  );

      if (!NT_SUCCESS(status))
      {
         goto Exit;
      }
   }

   if (packet->completionContext != NULL)
   {
      NT_ASSERT(packet->type == TL_INSPECT_CONNECT_PACKET);

      TraceLoggingWrite(
         gTlgHandle,
         "WorkerThread",
         TraceLoggingWideString(L"[TLInspectCloneReinjectInbound] Invoking FwpsCompleteOperation()", "Message")
         );

      FwpsCompleteOperation(
         packet->completionContext,
         clonedNetBufferList
         );

      packet->completionContext = NULL;
   }

   status = FwpsInjectTransportReceiveAsync(
               gInjectionHandle,
               NULL,
               NULL,
               0,
               packet->addressFamily,
               packet->compartmentId,
               packet->interfaceIndex,
               packet->subInterfaceIndex,
               clonedNetBufferList,
               TLInspectInjectComplete,
               packet
               );

   TraceLoggingWrite(
      gTlgHandle,
      "WorkerThread",
      TraceLoggingWideString(L"[TLInspectCloneReinjectInbound] Invoked FwpsInjectTransportSendAsync()", "Message"),
      TraceLoggingUInt32(status, "FwpsInjectTransportSendAsync::ntStatus")
      );

   if (!NT_SUCCESS(status))
   {
      goto Exit;
   }

   clonedNetBufferList = NULL; // ownership transferred to the 
                               // completion function.

Exit:

   if (clonedNetBufferList != NULL)
   {
      FwpsFreeCloneNetBufferList(clonedNetBufferList, 0);
   }

   return status;
}

void
TlInspectCompletePendedConnection(
   _Inout_ TL_INSPECT_PENDED_PACKET* pendedConnect
   )
/* ++

   This function completes the pended connection (inbound or outbound)
   with the inspection result. After invoking FwpsCompleteOperation(),
   if this callout is permitting the connection, it will also clone re-inject the packet.

-- */
{
   NTSTATUS status = STATUS_SUCCESS;
   KLOCK_QUEUE_HANDLE classifiedConnListLockHandle;
   ULONG processorIndex = KeGetCurrentProcessorNumber();
   HANDLE completionContext = pendedConnect->completionContext;

   pendedConnect->authConnectDecision = 
      gIsTrafficPermitted ? FWP_ACTION_PERMIT : FWP_ACTION_BLOCK;

   if (pendedConnect->direction == FWP_DIRECTION_OUTBOUND)
   {
      //
      // For pended ALE_AUTH_CONNECT, FwpsCompleteOperation will trigger
      // a re-auth during which the inspection decision is to be returned.
      // Here we move the pended entry to the classified list such that the
      // re-auth can find it along with the recorded inspection result.
      //
      pendedConnect->completionContext = NULL; // handing ownership to FwpsCompleteOperation

      TraceLoggingWrite(
         gTlgHandle,
         "WorkerThread",
         TraceLoggingWideString(L"[TlInspectCompletePendedConnection:Outbound] Invoking FwpsCompleteOperation()", "Message")
         );

      //
      // Add to the classifiedConnList for the re-auth path to look at to determine action
      //
      KeAcquireInStackQueuedSpinLock(
         &gProcessorQueue[processorIndex].classifiedConnListLock,
         &classifiedConnListLockHandle
         );

      InsertTailList(&gProcessorQueue[processorIndex].classifiedConnList, &pendedConnect->listEntry);

      KeReleaseInStackQueuedSpinLock(&classifiedConnListLockHandle);

      FwpsCompleteOperation(
         completionContext,
         NULL
         );

      //
      // Re-auth has completed, remove it from the classifiedConnList
      //
      KeAcquireInStackQueuedSpinLock(
         &gProcessorQueue[processorIndex].classifiedConnListLock,
         &classifiedConnListLockHandle
         );

      RemoveEntryList(&pendedConnect->listEntry);

      KeReleaseInStackQueuedSpinLock(&classifiedConnListLockHandle);

      //
      // Clone and re-inject the packet
      //
      pendedConnect->type = TL_INSPECT_DATA_PACKET;

      status = TLInspectCloneReinjectOutbound(pendedConnect);

      if (!NT_SUCCESS(status))
      {
         FreePendedPacket(pendedConnect);
      }
   }
   else
   {
      if (pendedConnect->authConnectDecision == FWP_ACTION_PERMIT)
      {
         //
         // TLInspectCloneReinjectInbound will invoke FwpsCompleteOperation() and clone re-inject the packet
         //
         status = TLInspectCloneReinjectInbound(pendedConnect);
         if (!NT_SUCCESS(status))
         {
            FreePendedPacket(pendedConnect);
         }
      }
      else
      {
         TraceLoggingWrite(
            gTlgHandle,
            "WorkerThread",
            TraceLoggingWideString(L"[TlInspectCompletePendedConnection:Inbound] Invoking FwpsCompleteOperation()", "Message")
            );

         //
         // We are dropping the packet, but still must issue a call to FwpsCompleteOperation(),
         // as we have previously pended this connection
         //
         pendedConnect->completionContext = NULL; // handing ownership to FwpsCompleteOperation

         FwpsCompleteOperation(
            completionContext,
            NULL
            );

         FreePendedPacket(pendedConnect);
      }
   }
}

void
TLInspectWorker(
   _In_ KDPC* pDPC,
   _In_opt_ PVOID pContext,
   _In_opt_ PVOID pArg1,
   _In_opt_ PVOID pArg2
   )
/* ++

   This worker DPC will run in a loop to process individual packets, until the
   queue is empty. Based on the direction and packet type, it will either
   invoke FwpsCompleteOperation(), followed by clone-reinject, or it will
   simply clone-reinject the packet.

   This worker DPC operates on the processor queue for which the DPC is running on.

- */
{
   NTSTATUS status = STATUS_SUCCESS;
   TL_INSPECT_PENDED_PACKET* packet = NULL;
   LIST_ENTRY* listEntry = NULL;
   KLOCK_QUEUE_HANDLE connListLockHandle;
   ULONG processorIndex = KeGetCurrentProcessorNumber();

   UNREFERENCED_PARAMETER(pDPC);
   UNREFERENCED_PARAMETER(pContext);
   UNREFERENCED_PARAMETER(pArg1);
   UNREFERENCED_PARAMETER(pArg2);

   for (;;)
   {
      listEntry = NULL;
      packet = NULL;

      TraceLoggingWrite(
         gTlgHandle,
         "WorkerThread",
         TraceLoggingWideString(L"[TLInspectWorker] Executing...", "Message"),
         TraceLoggingUInt32((UINT32) processorIndex, "ProcessorIndex")
         );

      KeAcquireInStackQueuedSpinLock(
         &gProcessorQueue[processorIndex].connListLock,
         &connListLockHandle
         );

      if (!IsListEmpty(&gProcessorQueue[processorIndex].connList))
      {
         listEntry = RemoveHeadList(&gProcessorQueue[processorIndex].connList);

         packet = CONTAINING_RECORD(
                           listEntry,
                           TL_INSPECT_PENDED_PACKET,
                           listEntry
                           );
      }

      if (packet == NULL)
      {
         gProcessorQueue[processorIndex].isDpcQueued = FALSE;
      }

      KeReleaseInStackQueuedSpinLock(&connListLockHandle);

      if (packet == NULL)
      {
         //
         // List empty. terminate thread
         //
         TraceLoggingWrite(
            gTlgHandle,
            "WorkerThread",
            TraceLoggingWideString(L"[TLInspectWorker] Terminating thread as connList is empty", "Message"),
            TraceLoggingUInt32((UINT32) processorIndex, "ProcessorIndex")
            );
         break;
      }

      if (packet->type == TL_INSPECT_CONNECT_PACKET)
      {
         //
         // We issued a call to FwpsPendOperation() for this packet.
         // We must invoke FwpsCompleteOperation() on this, and then clone re-inject the packet (if permitted).
         //
         TlInspectCompletePendedConnection(packet);
      }
      else
      {
         //
         // This packet was not pended, meaning that it is either a transport packet or a reauth packet.
         // Here, we do not need to invoke FwpsCompleteOperation, but we still need to clone re-inject (if permitted)
         //
         if ((gIsTrafficPermitted == TRUE) && (gDriverUnloading == FALSE))
         {
            if (packet->direction == FWP_DIRECTION_OUTBOUND)
            {
               status = TLInspectCloneReinjectOutbound(packet);
            }
            else
            {
               status = TLInspectCloneReinjectInbound(packet);
            }
            if (NT_SUCCESS(status))
            {
               packet = NULL; // ownership transferred.
            }
         }

         //
         // We free the packet if either:
         //    1 - Re-injection failed
         //    2 - We are unloading the driver
         //    3 - We are dropping the packet
         //
         if (packet != NULL)
         {
            FreePendedPacket(packet);
         }
      }
   }
}
