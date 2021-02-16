/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   This file implements the classifyFn callout functions for the ALE connect,
   recv-accept, and transport callouts. In addition the system worker thread 
   that performs the actual packet inspection is also implemented here along 
   with the eventing mechanisms shared between the classify function and the
   worker thread.

   connect/Packet inspection is done out-of-band by a system worker thread 
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
   is not set), it is queued to the connection list for inspection by the
   worker thread. For re-auth, we first check if it is triggered by an ealier
   FwpsCompleteOperation call by looking for an pended connect that has been
   inspected. If found, we remove it from the connect list and return the 
   inspection result; otherwise we can conclude that the re-auth is triggered 
   by policy change so we queue it to the packet queue to be process by the 
   worker thread like any other regular packets.

-- */
{
   NTSTATUS status;

   KLOCK_QUEUE_HANDLE connListLockHandle;
   KLOCK_QUEUE_HANDLE packetQueueLockHandle;

   TL_INSPECT_PENDED_PACKET* pendedConnect = NULL;
   TL_INSPECT_PENDED_PACKET* connEntry;
   TL_INSPECT_PENDED_PACKET* pendedPacket = NULL;

   ADDRESS_FAMILY addressFamily;
   FWPS_PACKET_INJECTION_STATE packetState;
   BOOLEAN signalWorkerThread;

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
      goto Exit;
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
      //
      // If the classify is the initial authorization for a connection, we 
      // queue it to the pended connection list and notify the worker thread
      // for out-of-band processing.
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

      if (!NT_SUCCESS(status))
      {
         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         goto Exit;
      }

      KeAcquireInStackQueuedSpinLock(
         &gConnListLock,
         &connListLockHandle
         );
      KeAcquireInStackQueuedSpinLock(
         &gPacketQueueLock,
         &packetQueueLockHandle
         );

      signalWorkerThread = IsListEmpty(&gConnList) && 
                           IsListEmpty(&gPacketQueue);

      InsertTailList(&gConnList, &pendedConnect->listEntry);
      pendedConnect = NULL; // ownership transferred

      KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);
      KeReleaseInStackQueuedSpinLock(&connListLockHandle);

      classifyOut->actionType = FWP_ACTION_BLOCK;
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

      if (signalWorkerThread)
      {
         KeSetEvent(
            &gWorkerEvent, 
            0, 
            FALSE
            );
      }
   }
   else // re-auth @ ALE_AUTH_CONNECT
   {
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
         // decision recorded. If found, we return that decision and remove
         // the pended connect from the list.
         //

         KeAcquireInStackQueuedSpinLock(
            &gConnListLock,
            &connListLockHandle
            );

         for (listEntry = gConnList.Flink;
              listEntry != &gConnList;
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
               // We found a match.
               pendedConnect = connEntry;

               NT_ASSERT((pendedConnect->authConnectDecision == FWP_ACTION_PERMIT) ||
                      (pendedConnect->authConnectDecision == FWP_ACTION_BLOCK));
               
               classifyOut->actionType = pendedConnect->authConnectDecision;
               if (classifyOut->actionType == FWP_ACTION_BLOCK || 
                     filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
               {
                  classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
               }

               RemoveEntryList(&pendedConnect->listEntry);
               
               if (!gDriverUnloading &&
                   (pendedConnect->netBufferList != NULL) &&
                   (pendedConnect->authConnectDecision == FWP_ACTION_PERMIT))
               {
                  //
                  // Now the outbound connection has been authorized. If the
                  // pended connect has a net buffer list in it, we need it
                  // morph it into a data packet and queue it to the packet
                  // queue for send injecition.
                  //
                  pendedConnect->type = TL_INSPECT_DATA_PACKET;

                  KeAcquireInStackQueuedSpinLock(
                     &gPacketQueueLock,
                     &packetQueueLockHandle
                     );

                  signalWorkerThread = IsListEmpty(&gPacketQueue) &&
                                       IsListEmpty(&gConnList);

                  InsertTailList(&gPacketQueue, &pendedConnect->listEntry);
                  pendedConnect = NULL; // ownership transferred

                  KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);
                  
                  if (signalWorkerThread)
                  {
                     KeSetEvent(
                        &gWorkerEvent, 
                        0, 
                        FALSE
                        );
                  }
               }

               authComplete = TRUE;
               break;
            }
         }

         KeReleaseInStackQueuedSpinLock(&connListLockHandle);

         if (authComplete)
         {
            goto Exit;
         }
      }

      //
      // If we reach here it means this is a policy change triggered re-auth
      // for an pre-existing connection. For such a packet (inbound or 
      // outbound) we queue it to the packet queue and inspect it just like
      // other regular data packets from TRANSPORT layers.
      //

      NT_ASSERT(layerData != NULL);

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
         &gConnListLock,
         &connListLockHandle
         );
      KeAcquireInStackQueuedSpinLock(
         &gPacketQueueLock,
         &packetQueueLockHandle
         );

      if (!gDriverUnloading)
      {
         signalWorkerThread = IsListEmpty(&gPacketQueue) &&
                              IsListEmpty(&gConnList);

         InsertTailList(&gPacketQueue, &pendedPacket->listEntry);
         pendedPacket = NULL; // ownership transferred

         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
      }
      else
      {
         //
         // Driver is being unloaded, permit any connect classify.
         //
         signalWorkerThread = FALSE;

         classifyOut->actionType = FWP_ACTION_PERMIT;
         if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
         {
            classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         }
      }

      KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);
      KeReleaseInStackQueuedSpinLock(&connListLockHandle);

      if (signalWorkerThread)
      {
         KeSetEvent(
            &gWorkerEvent, 
            0, 
            FALSE
            );
      }

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
   KLOCK_QUEUE_HANDLE packetQueueLockHandle;

   TL_INSPECT_PENDED_PACKET* pendedRecvAccept = NULL;
   TL_INSPECT_PENDED_PACKET* pendedPacket = NULL;

   ADDRESS_FAMILY addressFamily;
   FWPS_PACKET_INJECTION_STATE packetState;
   BOOLEAN signalWorkerThread;

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

      goto Exit;
   }

   addressFamily = GetAddressFamilyForLayer(inFixedValues->layerId);

   if (!IsAleReauthorize(inFixedValues))
   {
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

      if (!NT_SUCCESS(status))
      {
         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         goto Exit;
      }

      KeAcquireInStackQueuedSpinLock(
         &gConnListLock,
         &connListLockHandle
         );
      KeAcquireInStackQueuedSpinLock(
         &gPacketQueueLock,
         &packetQueueLockHandle
         );

      signalWorkerThread = IsListEmpty(&gConnList) && 
                           IsListEmpty(&gPacketQueue);

      InsertTailList(&gConnList, &pendedRecvAccept->listEntry);
      pendedRecvAccept = NULL; // ownership transferred

      KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);
      KeReleaseInStackQueuedSpinLock(&connListLockHandle);

      classifyOut->actionType = FWP_ACTION_BLOCK;
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;

      if (signalWorkerThread)
      {
         KeSetEvent(
            &gWorkerEvent, 
            0, 
            FALSE
            );
      }

   }
   else // re-auth @ ALE_AUTH_RECV_ACCEPT
   {
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
         &gConnListLock,
         &connListLockHandle
         );
      KeAcquireInStackQueuedSpinLock(
         &gPacketQueueLock,
         &packetQueueLockHandle
         );

      if (!gDriverUnloading)
      {
         signalWorkerThread = IsListEmpty(&gPacketQueue) &&
                              IsListEmpty(&gConnList);

         InsertTailList(&gPacketQueue, &pendedPacket->listEntry);
         pendedPacket = NULL; // ownership transferred

         classifyOut->actionType = FWP_ACTION_BLOCK;
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
      }
      else
      {
         //
         // Driver is being unloaded, permit any connect classify.
         //
         signalWorkerThread = FALSE;

         classifyOut->actionType = FWP_ACTION_PERMIT;
         if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
         {
            classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
         }
      }

      KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);
      KeReleaseInStackQueuedSpinLock(&connListLockHandle);

      if (signalWorkerThread)
      {
         KeSetEvent(
            &gWorkerEvent, 
            0, 
            FALSE
            );
      }
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
   KLOCK_QUEUE_HANDLE packetQueueLockHandle;

   TL_INSPECT_PENDED_PACKET* pendedPacket = NULL;
   FWP_DIRECTION packetDirection;

   ADDRESS_FAMILY addressFamily;
   FWPS_PACKET_INJECTION_STATE packetState;
   BOOLEAN signalWorkerThread;

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
      &gConnListLock,
      &connListLockHandle
      );
   KeAcquireInStackQueuedSpinLock(
      &gPacketQueueLock,
      &packetQueueLockHandle
      );

   if (!gDriverUnloading)
   {
      signalWorkerThread = IsListEmpty(&gPacketQueue) &&
                           IsListEmpty(&gConnList);

      InsertTailList(&gPacketQueue, &pendedPacket->listEntry);
      pendedPacket = NULL; // ownership transferred

      classifyOut->actionType = FWP_ACTION_BLOCK;
      classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
   }
   else
   {
      //
      // Driver is being unloaded, permit any connect classify.
      //
      signalWorkerThread = FALSE;

      classifyOut->actionType = FWP_ACTION_PERMIT;
      if (filter->flags & FWPS_FILTER_FLAG_CLEAR_ACTION_RIGHT)
      {
         classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
      }
   }

   KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);
   KeReleaseInStackQueuedSpinLock(&connListLockHandle);

   if (signalWorkerThread)
   {
      KeSetEvent(
         &gWorkerEvent, 
         0, 
         FALSE
         );
   }

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
   _Inout_ TL_INSPECT_PENDED_PACKET** pendedConnect,
   _In_ BOOLEAN permitTraffic
   )
/* ++

   This function completes the pended connection (inbound or outbound)
   with the inspection result.

-- */
{

   TL_INSPECT_PENDED_PACKET* pendedConnectLocal = *pendedConnect;

   if (pendedConnectLocal->direction == FWP_DIRECTION_OUTBOUND)
   {
      HANDLE completionContext = pendedConnectLocal->completionContext;

      pendedConnectLocal->authConnectDecision = 
         permitTraffic ? FWP_ACTION_PERMIT : FWP_ACTION_BLOCK;

      //
      // For pended ALE_AUTH_CONNECT, FwpsCompleteOperation will trigger
      // a re-auth during which the inspection decision is to be returned.
      // Here we don't remove the pended entry from the list such that the
      // re-auth can find it along with the recorded inspection result.
      //
      pendedConnectLocal->completionContext = NULL;

      FwpsCompleteOperation(
         completionContext,
         NULL
         );

      *pendedConnect = NULL; // ownership transferred to the re-auth path.
   }
   else
   {
      if (!configPermitTraffic)
      {
         FreePendedPacket(pendedConnectLocal);
         *pendedConnect = NULL;
      }

      //
      // Permitted ALE_RECV_ACCEPT will pass thru and be processed by
      // TLInspectCloneReinjectInbound. FwpsCompleteOperation will be called
      // then when the net buffer list is cloned; after which the clone will
      // be recv-injected.
      //
   }
}

void
TLInspectWorker(
   _In_ void* StartContext
   )
/* ++

   This worker thread waits for the connect and packet queue event when the 
   queues are empty; and it will be woken up when there are connects/packets 
   queued needing to be inspected. Once awaking, It will run in a loop to 
   complete the pended ALE classifies and/or clone-reinject packets back 
   until both queues are exhausted (and it will go to sleep waiting for more 
   work).

   The worker thread will end once it detected the driver is unloading.

-- */
{
   NTSTATUS status;

   TL_INSPECT_PENDED_PACKET* packet = NULL;
   LIST_ENTRY* listEntry;

   KLOCK_QUEUE_HANDLE packetQueueLockHandle;
   KLOCK_QUEUE_HANDLE connListLockHandle;

   BOOLEAN found = FALSE;

   UNREFERENCED_PARAMETER(StartContext);

   for(;;)
   {
      KeWaitForSingleObject(
         &gWorkerEvent,
         Executive, 
         KernelMode, 
         FALSE, 
         NULL
         );

      if (gDriverUnloading)
      {
         break;
      }

      configPermitTraffic = IsTrafficPermitted();

      listEntry = NULL;

      KeAcquireInStackQueuedSpinLock(
         &gConnListLock,
         &connListLockHandle
         );

      if (!IsListEmpty(&gConnList))
      {
         //
         // Skip pended connections in the list, for which the auth decision is already taken.
         // They should not be for inbound connections.
         //
         _Analysis_assume_(gConnList.Flink != NULL);         
         for (listEntry = gConnList.Flink;
              listEntry != &gConnList;
              listEntry = listEntry->Flink)
         {
            packet = CONTAINING_RECORD(
                                listEntry,
                                TL_INSPECT_PENDED_PACKET,
                                listEntry
                                );

            NT_ASSERT((packet->direction != FWP_DIRECTION_INBOUND) ||
                      (packet->authConnectDecision == 0));
        
            if (packet->authConnectDecision == 0)
            {
               found = TRUE;
               break;
            }
         }

         //
         // If not found, reset entry and packet
         //
         if (!found)
         {
            listEntry = NULL;
            packet = NULL;
         }

         //
         // Completing a pended recv_accept auth does not trigger reauth. 
         // So the pended entries for AUTH_RECV_ACCEPT are removed here. 
         //
         if (packet != NULL && packet->direction == FWP_DIRECTION_INBOUND)
         {
            RemoveEntryList(&packet->listEntry);
         }

         //
         // Leave the pended ALE_AUTH_CONNECT in the connection list, it will
         // be processed and removed from the list during re-auth.
         //
      }

      KeReleaseInStackQueuedSpinLock(&connListLockHandle);

      if (listEntry == NULL)
      {
         NT_ASSERT(!IsListEmpty(&gPacketQueue));

         KeAcquireInStackQueuedSpinLock(
            &gPacketQueueLock,
            &packetQueueLockHandle
            );

         listEntry = RemoveHeadList(&gPacketQueue);

         packet = CONTAINING_RECORD(
                           listEntry,
                           TL_INSPECT_PENDED_PACKET,
                           listEntry
                           );

         KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);
      }

      if (packet->type == TL_INSPECT_CONNECT_PACKET)
      {
         TlInspectCompletePendedConnection(
            &packet,
            configPermitTraffic);
      }

      if ((packet != NULL) && configPermitTraffic)
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

      if (packet != NULL)
      {
         FreePendedPacket(packet);
      }

      KeAcquireInStackQueuedSpinLock(
         &gConnListLock,
         &connListLockHandle
         );
      KeAcquireInStackQueuedSpinLock(
         &gPacketQueueLock,
         &packetQueueLockHandle
         );

      if (IsListEmpty(&gConnList) && IsListEmpty(&gPacketQueue) &&
          !gDriverUnloading)
      {
         KeClearEvent(&gWorkerEvent);
      }

      KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);
      KeReleaseInStackQueuedSpinLock(&connListLockHandle);
   }

   NT_ASSERT(gDriverUnloading);

   while (!IsListEmpty(&gConnList))
   {
      packet = NULL;

      KeAcquireInStackQueuedSpinLock(
         &gConnListLock,
         &connListLockHandle
         );

      if (!IsListEmpty(&gConnList))
      {
         listEntry = gConnList.Flink;
         packet = CONTAINING_RECORD(
                           listEntry,
                           TL_INSPECT_PENDED_PACKET,
                           listEntry
                           );
      }

      KeReleaseInStackQueuedSpinLock(&connListLockHandle);

      if (packet != NULL)
      {
         TlInspectCompletePendedConnection(&packet, FALSE);
         NT_ASSERT(packet == NULL);
      }
   }

   //
   // Discard all the pended packets if driver is being unloaded.
   //

   while (!IsListEmpty(&gPacketQueue))
   {
      packet = NULL;

      KeAcquireInStackQueuedSpinLock(
         &gPacketQueueLock,
         &packetQueueLockHandle
         );

      if (!IsListEmpty(&gPacketQueue))
      {
         listEntry = RemoveHeadList(&gPacketQueue);

         packet = CONTAINING_RECORD(
                           listEntry,
                           TL_INSPECT_PENDED_PACKET,
                           listEntry
                           );
      }

      KeReleaseInStackQueuedSpinLock(&packetQueueLockHandle);
      
      if (packet != NULL)
      {
         FreePendedPacket(packet);
      }
   }

   PsTerminateSystemThread(STATUS_SUCCESS);

}
