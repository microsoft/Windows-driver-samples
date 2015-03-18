/*++

Copyright (c) Microsoft Corporation. All rights reserved

Abstract:

   This file declares the utility/helper functions for use by the classify
   functions and worker thread of the Transport Inspect sample.

Environment:

    Kernel mode

--*/

#include <limits.h>

#ifndef _TL_INSPECT_UTILS_H_
#define _TL_INSPECT_UTILS_H_

__inline
ADDRESS_FAMILY GetAddressFamilyForLayer(
   _In_ UINT16 layerId
   )
{
   ADDRESS_FAMILY addressFamily;

   switch (layerId)
   {
   case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
   case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
   case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
   case FWPS_LAYER_INBOUND_TRANSPORT_V4:
      addressFamily = AF_INET;
      break;
   case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
   case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
   case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
   case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      addressFamily = AF_INET6;
      break;
   default:
      addressFamily = AF_UNSPEC;
      NT_ASSERT(0);
   }

   return addressFamily;
}

__inline
FWP_DIRECTION GetPacketDirectionForLayer(
   _In_ UINT16 layerId
   )
{
   FWP_DIRECTION direction;

   switch (layerId)
   {
   case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
   case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
      direction = FWP_DIRECTION_OUTBOUND;
      break;
   case FWPS_LAYER_INBOUND_TRANSPORT_V4:
   case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      direction = FWP_DIRECTION_INBOUND;
      break;
   default:
      direction = FWP_DIRECTION_MAX;
      NT_ASSERT(0);
   }

   return direction;
}

__inline
void
GetFlagsIndexesForLayer(
   _In_ UINT16 layerId,
   _Out_ UINT* flagsIndex
   )
{
   switch (layerId)
   {
   case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
      *flagsIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_FLAGS;
      break;
   case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
      *flagsIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_FLAGS;
      break;
   case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
      *flagsIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_FLAGS;
      break;
   case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
      *flagsIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_FLAGS;
      break;
   case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
      *flagsIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V4_FLAGS;
      break;
   case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
      *flagsIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V6_FLAGS;
      break;
   case FWPS_LAYER_INBOUND_TRANSPORT_V4:
      *flagsIndex = FWPS_FIELD_INBOUND_TRANSPORT_V4_FLAGS;
      break;
   case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      *flagsIndex = FWPS_FIELD_INBOUND_TRANSPORT_V6_FLAGS;
      break;
   default:
      *flagsIndex = UINT_MAX;
      NT_ASSERT(0);
      break;
   }
}

__inline
void
GetDeliveryInterfaceIndexesForLayer(
   _In_ UINT16 layerId,
   _Out_ UINT* interfaceIndexIndex,
   _Out_ UINT* subInterfaceIndexIndex
   )
{
   *interfaceIndexIndex = 0;

   *subInterfaceIndexIndex = 0;

   switch (layerId)
   {
   case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
      *interfaceIndexIndex = 
         FWPS_FIELD_ALE_AUTH_CONNECT_V4_INTERFACE_INDEX;
      *subInterfaceIndexIndex = 
         FWPS_FIELD_ALE_AUTH_CONNECT_V4_SUB_INTERFACE_INDEX;
      break;
   case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
      *interfaceIndexIndex = 
         FWPS_FIELD_ALE_AUTH_CONNECT_V6_INTERFACE_INDEX;
      *subInterfaceIndexIndex = 
         FWPS_FIELD_ALE_AUTH_CONNECT_V6_SUB_INTERFACE_INDEX;
      break;
   case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
      *interfaceIndexIndex = 
         FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_INTERFACE_INDEX;
      *subInterfaceIndexIndex = 
         FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_SUB_INTERFACE_INDEX;
      break;
   case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
      *interfaceIndexIndex = 
         FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_INTERFACE_INDEX;
      *subInterfaceIndexIndex = 
         FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_SUB_INTERFACE_INDEX;
      break;
   case FWPS_LAYER_INBOUND_TRANSPORT_V4:
      *interfaceIndexIndex = 
         FWPS_FIELD_INBOUND_TRANSPORT_V4_INTERFACE_INDEX;
      *subInterfaceIndexIndex = 
         FWPS_FIELD_INBOUND_TRANSPORT_V4_SUB_INTERFACE_INDEX;
      break;
   case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      *interfaceIndexIndex = 
         FWPS_FIELD_INBOUND_TRANSPORT_V6_INTERFACE_INDEX;
      *subInterfaceIndexIndex = 
         FWPS_FIELD_INBOUND_TRANSPORT_V6_SUB_INTERFACE_INDEX;
      break;
   default:
      NT_ASSERT(0);
      break;
   }
}

__inline
void
GetNetwork5TupleIndexesForLayer(
   _In_ UINT16 layerId,
   _Out_ UINT* localAddressIndex,
   _Out_ UINT* remoteAddressIndex,
   _Out_ UINT* localPortIndex,
   _Out_ UINT* remotePortIndex,
   _Out_ UINT* protocolIndex
   )
{
   switch (layerId)
   {
   case FWPS_LAYER_ALE_AUTH_CONNECT_V4:
      *localAddressIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_ADDRESS;
      *remoteAddressIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_ADDRESS;
      *localPortIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_LOCAL_PORT;
      *remotePortIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_REMOTE_PORT;
      *protocolIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V4_IP_PROTOCOL;
      break;
   case FWPS_LAYER_ALE_AUTH_CONNECT_V6:
      *localAddressIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_ADDRESS;
      *remoteAddressIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_ADDRESS;
      *localPortIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_LOCAL_PORT;
      *remotePortIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_REMOTE_PORT;
      *protocolIndex = FWPS_FIELD_ALE_AUTH_CONNECT_V6_IP_PROTOCOL;
      break;
   case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4:
      *localAddressIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_ADDRESS;
      *remoteAddressIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_ADDRESS;
      *localPortIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_LOCAL_PORT;
      *remotePortIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_REMOTE_PORT;
      *protocolIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V4_IP_PROTOCOL;
      break;
   case FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6:
      *localAddressIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_ADDRESS;
      *remoteAddressIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_ADDRESS;
      *localPortIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_LOCAL_PORT;
      *remotePortIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_REMOTE_PORT;
      *protocolIndex = FWPS_FIELD_ALE_AUTH_RECV_ACCEPT_V6_IP_PROTOCOL;
      break;
   case FWPS_LAYER_OUTBOUND_TRANSPORT_V4:
      *localAddressIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS;
      *remoteAddressIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS;
      *localPortIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_LOCAL_PORT;
      *remotePortIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_REMOTE_PORT;
      *protocolIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V4_IP_PROTOCOL;
      break;
   case FWPS_LAYER_OUTBOUND_TRANSPORT_V6:
      *localAddressIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V6_IP_LOCAL_ADDRESS;
      *remoteAddressIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V6_IP_REMOTE_ADDRESS;
      *localPortIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V6_IP_LOCAL_PORT;
      *remotePortIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V6_IP_REMOTE_PORT;
      *protocolIndex = FWPS_FIELD_OUTBOUND_TRANSPORT_V6_IP_PROTOCOL;
      break;
   case FWPS_LAYER_INBOUND_TRANSPORT_V4:
      *localAddressIndex = FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_ADDRESS;
      *remoteAddressIndex = FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_REMOTE_ADDRESS;
      *localPortIndex = FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_LOCAL_PORT;
      *remotePortIndex = FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_REMOTE_PORT;
      *protocolIndex = FWPS_FIELD_INBOUND_TRANSPORT_V4_IP_PROTOCOL;
      break;
   case FWPS_LAYER_INBOUND_TRANSPORT_V6:
      *localAddressIndex = FWPS_FIELD_INBOUND_TRANSPORT_V6_IP_LOCAL_ADDRESS;
      *remoteAddressIndex = FWPS_FIELD_INBOUND_TRANSPORT_V6_IP_REMOTE_ADDRESS;
      *localPortIndex = FWPS_FIELD_INBOUND_TRANSPORT_V6_IP_LOCAL_PORT;
      *remotePortIndex = FWPS_FIELD_INBOUND_TRANSPORT_V6_IP_REMOTE_PORT;
      *protocolIndex = FWPS_FIELD_INBOUND_TRANSPORT_V6_IP_PROTOCOL;
      break;
   default:
      *localAddressIndex = UINT_MAX;
      *remoteAddressIndex = UINT_MAX;
      *localPortIndex = UINT_MAX;
      *remotePortIndex = UINT_MAX;
      *protocolIndex = UINT_MAX;      
      NT_ASSERT(0);
   }
}

BOOLEAN IsAleReauthorize(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues
   );

BOOLEAN IsSecureConnection(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues
   );

BOOLEAN
IsAleClassifyRequired(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues
   );

void
FillNetwork5Tuple(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ ADDRESS_FAMILY addressFamily,
   _Inout_ TL_INSPECT_PENDED_PACKET* packet
   );

BOOLEAN
IsMatchingConnectPacket(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ ADDRESS_FAMILY addressFamily,
   _In_ FWP_DIRECTION direction,
   _Inout_ TL_INSPECT_PENDED_PACKET* pendedPacket
   );

__drv_allocatesMem(Mem)
TL_INSPECT_PENDED_PACKET*
AllocateAndInitializePendedPacket(
   _In_ const FWPS_INCOMING_VALUES* inFixedValues,
   _In_ const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   _In_ ADDRESS_FAMILY addressFamily,
   _Inout_opt_ void* layerData,
   _In_ TL_INSPECT_PACKET_TYPE packetType,
   _In_ FWP_DIRECTION packetDirection
   );

void
FreePendedPacket(
   _Inout_ __drv_freesMem(Mem) TL_INSPECT_PENDED_PACKET* packet
   );

BOOLEAN
IsTrafficPermitted(void);

#endif // _TL_INSPECT_UTILS_H_
