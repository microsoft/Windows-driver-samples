/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   This file implements the utility/helper functions for use by the classify
   functions and worker thread of the Transport Inspect sample.

Environment:

    Kernel mode

--*/

#define POOL_ZERO_DOWN_LEVEL_SUPPORT
#include <ntddk.h>
#include <wdf.h>

#pragma warning(push)
#pragma warning(disable:4201)       // unnamed struct/union

#include <fwpsk.h>

#pragma warning(pop)

#include <fwpmk.h>

#include "inspect.h"
#include "utils.h"


BOOLEAN IsAleReauthorize(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues
   )
{
   UINT flagsIndex;

   GetFlagsIndexesForLayer(
      inFixedValues->layerId,
      &flagsIndex
      );

   if((flagsIndex != UINT_MAX) && ((inFixedValues->incomingValue\
      [flagsIndex].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE) != 0))
   {
      return TRUE;
   }

   return FALSE;
}

BOOLEAN IsSecureConnection(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues
   )
{
   UINT flagsIndex;

   GetFlagsIndexesForLayer(
      inFixedValues->layerId,
      &flagsIndex
      );

   if ((flagsIndex != UINT_MAX) && ((inFixedValues->incomingValue\
       [flagsIndex].value.uint32 & FWP_CONDITION_FLAG_IS_IPSEC_SECURED) != 0))
   {
      return TRUE;
   }

   return FALSE;
}

BOOLEAN
IsAleClassifyRequired(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues
   )
{
   //
   // Note that use of FWP_CONDITION_FLAG_REQUIRES_ALE_CLASSIFY has been
   // deprecated in Vista SP1 and Windows Server 2008.
   //
   UNREFERENCED_PARAMETER(inFixedValues);
   return FWPS_IS_METADATA_FIELD_PRESENT(
             inMetaValues,
             FWPS_METADATA_FIELD_ALE_CLASSIFY_REQUIRED
             );
}

BOOLEAN
IsMatchingConnectPacket(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ ADDRESS_FAMILY addressFamily,
   _In_ FWP_DIRECTION direction,
   _Inout_ TL_INSPECT_PENDED_PACKET* pendedPacket
   )
{
   UINT localAddrIndex;
   UINT remoteAddrIndex;
   UINT localPortIndex;
   UINT remotePortIndex;
   UINT protocolIndex;

   NT_ASSERT(pendedPacket->type == TL_INSPECT_CONNECT_PACKET);

   GetNetwork5TupleIndexesForLayer(
      inFixedValues->layerId,
      &localAddrIndex,
      &remoteAddrIndex,
      &localPortIndex,
      &remotePortIndex,
      &protocolIndex
      );

   if(localAddrIndex == UINT_MAX)
   {
      return FALSE;
   }

   if (addressFamily != pendedPacket->addressFamily)
   {
      return FALSE;
   }

   if (direction != pendedPacket->direction)
   {
      return FALSE;
   }

   if (inFixedValues->incomingValue[protocolIndex].value.uint8 !=
       pendedPacket->protocol)
   {
      return FALSE;
   }

   if (RtlUshortByteSwap(
         inFixedValues->incomingValue[localPortIndex].value.uint16
         ) != pendedPacket->localPort)
   {
      return FALSE;
   }

   if (RtlUshortByteSwap(
         inFixedValues->incomingValue[remotePortIndex].value.uint16
         ) != pendedPacket->remotePort)
   {
      return FALSE;
   }

   if (addressFamily == AF_INET)
   {
      UINT32 ipv4LocalAddr =
         RtlUlongByteSwap(
            inFixedValues->incomingValue[localAddrIndex].value.uint32
            );
      UINT32 ipv4RemoteAddr =
      // Prefast thinks we are ignoring this return value.
      // If driver is unloading, we give up and ignore it on purpose.
      // Otherwise, we put the pointer onto the list, but we make it opaque
      // by casting it as a UINT64, and this tricks Prefast.
         RtlUlongByteSwap( /* host-order -> network-order conversion */
            inFixedValues->incomingValue[remoteAddrIndex].value.uint32
            );
      if (ipv4LocalAddr != pendedPacket->ipv4LocalAddr)
      {
         return FALSE;
      }

      if (ipv4RemoteAddr != pendedPacket->ipv4RemoteAddr)
      {
         return FALSE;
      }
   }
   else
   {
      if (RtlCompareMemory(
            inFixedValues->incomingValue[localAddrIndex].value.byteArray16,
            &pendedPacket->localAddr,
            sizeof(FWP_BYTE_ARRAY16)) !=  sizeof(FWP_BYTE_ARRAY16))
      {
         return FALSE;
      }

      if (RtlCompareMemory(
            inFixedValues->incomingValue[remoteAddrIndex].value.byteArray16,
            &pendedPacket->remoteAddr,
            sizeof(FWP_BYTE_ARRAY16)) !=  sizeof(FWP_BYTE_ARRAY16))
      {
         return FALSE;
      }
   }

   return TRUE;
}

void
FillNetwork5Tuple(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ ADDRESS_FAMILY addressFamily,
   _Inout_ TL_INSPECT_PENDED_PACKET* packet
   )
{
   UINT localAddrIndex;
   UINT remoteAddrIndex;
   UINT localPortIndex;
   UINT remotePortIndex;
   UINT protocolIndex;

   GetNetwork5TupleIndexesForLayer(
      inFixedValues->layerId,
      &localAddrIndex,
      &remoteAddrIndex,
      &localPortIndex,
      &remotePortIndex,
      &protocolIndex
      );

   if (addressFamily == AF_INET)
   {
      packet->ipv4LocalAddr =
         RtlUlongByteSwap( /* host-order -> network-order conversion */
            inFixedValues->incomingValue[localAddrIndex].value.uint32
            );
      packet->ipv4RemoteAddr =
         RtlUlongByteSwap( /* host-order -> network-order conversion */
            inFixedValues->incomingValue[remoteAddrIndex].value.uint32
            );
   }
   else
   {
      RtlCopyMemory(
         (UINT8*)&packet->localAddr,
         inFixedValues->incomingValue[localAddrIndex].value.byteArray16,
         sizeof(FWP_BYTE_ARRAY16)
         );
      RtlCopyMemory(
         (UINT8*)&packet->remoteAddr,
         inFixedValues->incomingValue[remoteAddrIndex].value.byteArray16,
         sizeof(FWP_BYTE_ARRAY16)
         );
   }

   packet->localPort =
      RtlUshortByteSwap(
         inFixedValues->incomingValue[localPortIndex].value.uint16
         );
   packet->remotePort =
      RtlUshortByteSwap(
         inFixedValues->incomingValue[remotePortIndex].value.uint16
         );

   packet->protocol = inFixedValues->incomingValue[protocolIndex].value.uint8;

   return;
}

void
FreePendedPacket(
   _Inout_ __drv_freesMem(Mem) TL_INSPECT_PENDED_PACKET* packet
   )
{
   if (packet->netBufferList != NULL)
   {
      FwpsDereferenceNetBufferList(packet->netBufferList, FALSE);
   }
   if (packet->controlData != NULL)
   {
      ExFreePoolWithTag(packet->controlData, TL_INSPECT_CONTROL_DATA_POOL_TAG);
   }
   if (packet->completionContext != NULL)
   {
      NT_ASSERT(packet->type == TL_INSPECT_CONNECT_PACKET);
      NT_ASSERT(packet->direction == FWP_DIRECTION_INBOUND); // complete for ALE connect
                                                          // is done prior to freeing
                                                          // of the packet.
      FwpsCompleteOperation(packet->completionContext, NULL);
   }
   ExFreePoolWithTag(packet, TL_INSPECT_PENDED_PACKET_POOL_TAG);
}

__drv_allocatesMem(Mem)
TL_INSPECT_PENDED_PACKET*
AllocateAndInitializePendedPacket(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _In_ ADDRESS_FAMILY addressFamily,
   _Inout_opt_ void* layerData,
   _In_ TL_INSPECT_PACKET_TYPE packetType,
   _In_ FWP_DIRECTION packetDirection
   )
{
   TL_INSPECT_PENDED_PACKET* pendedPacket;

   pendedPacket = ExAllocatePool2(
                        POOL_FLAG_NON_PAGED,
                        sizeof(TL_INSPECT_PENDED_PACKET),
                        TL_INSPECT_PENDED_PACKET_POOL_TAG
                        );

   if (pendedPacket == NULL)
   {
      return NULL;
   }

   pendedPacket->type = packetType;
   pendedPacket->direction = packetDirection;

   pendedPacket->addressFamily = addressFamily;

   FillNetwork5Tuple(
      inFixedValues,
      addressFamily,
      pendedPacket
      );

   if (layerData != NULL)
   {
      pendedPacket->netBufferList = layerData;

      //
      // Reference the net buffer list to make it accessible outside of
      // classifyFn.
      //
      FwpsReferenceNetBufferList(pendedPacket->netBufferList, TRUE);
   }

   NT_ASSERT(FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues,
                                         FWPS_METADATA_FIELD_COMPARTMENT_ID));
   pendedPacket->compartmentId = inMetaValues->compartmentId;

   if ((pendedPacket->direction == FWP_DIRECTION_OUTBOUND) &&
       (layerData != NULL))
   {
      NT_ASSERT(FWPS_IS_METADATA_FIELD_PRESENT(
                  inMetaValues,
                  FWPS_METADATA_FIELD_TRANSPORT_ENDPOINT_HANDLE));
      pendedPacket->endpointHandle = inMetaValues->transportEndpointHandle;

      pendedPacket->remoteScopeId = inMetaValues->remoteScopeId;

      if (FWPS_IS_METADATA_FIELD_PRESENT(
            inMetaValues,
            FWPS_METADATA_FIELD_TRANSPORT_CONTROL_DATA))
      {
         NT_ASSERT(inMetaValues->controlDataLength > 0);

         pendedPacket->controlData = ExAllocatePool2(
                                       POOL_FLAG_NON_PAGED,
                                       inMetaValues->controlDataLength,
                                       TL_INSPECT_CONTROL_DATA_POOL_TAG
                                       );
         if (pendedPacket->controlData == NULL)
         {
            goto Exit;
         }

         RtlCopyMemory(
            pendedPacket->controlData,
            inMetaValues->controlData,
            inMetaValues->controlDataLength
            );

         pendedPacket->controlDataLength =  inMetaValues->controlDataLength;
      }
   }
   else if (pendedPacket->direction == FWP_DIRECTION_INBOUND)
   {
      UINT interfaceIndexIndex = 0;
      UINT subInterfaceIndexIndex = 0;

      GetDeliveryInterfaceIndexesForLayer(
         inFixedValues->layerId,
         &interfaceIndexIndex,
         &subInterfaceIndexIndex
         );

      pendedPacket->interfaceIndex =
         inFixedValues->incomingValue[interfaceIndexIndex].value.uint32;
      pendedPacket->subInterfaceIndex =
         inFixedValues->incomingValue[subInterfaceIndexIndex].value.uint32;

      NT_ASSERT(FWPS_IS_METADATA_FIELD_PRESENT(
               inMetaValues,
               FWPS_METADATA_FIELD_IP_HEADER_SIZE));
      NT_ASSERT(FWPS_IS_METADATA_FIELD_PRESENT(
               inMetaValues,
               FWPS_METADATA_FIELD_TRANSPORT_HEADER_SIZE));
      pendedPacket->ipHeaderSize = inMetaValues->ipHeaderSize;
      pendedPacket->transportHeaderSize = inMetaValues->transportHeaderSize;

      if (pendedPacket->netBufferList != NULL)
      {
         FWPS_PACKET_LIST_INFORMATION packetInfo = {0};
         FwpsGetPacketListSecurityInformation(
            pendedPacket->netBufferList,
            FWPS_PACKET_LIST_INFORMATION_QUERY_IPSEC |
            FWPS_PACKET_LIST_INFORMATION_QUERY_INBOUND,
            &packetInfo
            );

         pendedPacket->ipSecProtected =
            (BOOLEAN)packetInfo.ipsecInformation.inbound.isSecure;

         pendedPacket->nblOffset =
            NET_BUFFER_DATA_OFFSET(\
               NET_BUFFER_LIST_FIRST_NB(pendedPacket->netBufferList));
      }
   }

   return pendedPacket;

Exit:

   if (pendedPacket != NULL)
   {
      FreePendedPacket(pendedPacket);
   }

   return NULL;
}

extern WDFKEY gParametersKey;

BOOLEAN
IsTrafficPermitted(void)
{
   NTSTATUS status;
   BOOLEAN permitTraffic = TRUE;
   DECLARE_CONST_UNICODE_STRING(valueName, L"BlockTraffic");
   ULONG result;

   status = WdfRegistryQueryULong(
               gParametersKey,
               &valueName,
               &result
               );

   if (NT_SUCCESS(status) && result != 0)
   {
      permitTraffic = FALSE;
   }

   return permitTraffic;
}


