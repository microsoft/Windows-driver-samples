////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_FwpmLayer.cpp
//
//   Abstract:
//      This module contains functions which assist in actions pertaining to FWPM_LAYER objects.
//
//   Naming Convention:
//
//      <Scope><Module><Object><Action><Modifier>
//  
//      i.e.
//
//       <Scope>
//          {
//                      - Function is likely visible to other modules.
//          }
//       <Module>
//          {
//            Hlpr      - Function is from HelperFunctions_* Modules.
//          }
//       <Object>
//          {
//            FwpmLayer - Function pertains to FWPM_LAYER objects.
//          }
//       <Action>
//          {
//            Get       - Function retrieves a value.
//            Is        - Function determines capabilities.
//          }
//       <Modifier>
//          {
//            ByID      - Function takes a runtime ID.
//            ByKey     - Function takes a GUID.
//            ByString  - Function takes a null terminated wide character string and returns the 
//                           LayerKey.
//            ID        - Function acts on the objects ID.
//            IPv4      - Function acts on IP version 4 objects.
//            IPv6      - Function acts on IP version 6 objects.
//          }
//   Private Functions:
//
//   Public Functions:
//      HlprFwpmLayerGetByID(),
//      HlprFwpmLayerGetByString(),
//      HlprFwpmLayerGetIDByKey(),
//      HlprFwpmLayerGetIDByString(),
//      HlprFwpmLayerIsIPv4(),
//      HlprFwpmLayerIsIPv6(),
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add basic support for WinBlue Fast layers
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HelperFunctions_Include.h" /// .

_At_(*pppFilterConditionArray, _Pre_ _Null_)
_When_(return != NO_ERROR, _At_(*pppFilterConditionArray, _Post_ _Null_))
_When_(return == NO_ERROR, _At_(*pppFilterConditionArray, _Post_ _Maybenull_))
_Success_(return == NO_ERROR)
UINT32 HlprFwpmLayerGetFilterConditionArrayByKey(_In_ const GUID* pLayerKey,
                                                 _Outptr_result_buffer_maybenull_(*pConditionArrayCount) GUID*** pppFilterConditionArray,
                                                 _Out_ UINT16* pConditionArrayCount)
{
   ASSERT(pLayerKey);
   ASSERT(pppFilterConditionArray);
   ASSERT(pConditionArrayCount);

   UINT32 status = NO_ERROR;

   if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_IPPACKET_V4,
                        pLayerKey) ||
      HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD,
                        pLayerKey) ||
      HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_IPPACKET_V6,
                        pLayerKey) ||
      HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD,
                        pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionInboundIPPacket;

      *pConditionArrayCount = INBOUND_IPPACKET_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_IPPACKET_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_IPPACKET_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionOutboundIPPacket;

      *pConditionArrayCount = OUTBOUND_IPPACKET_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_IPFORWARD_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_IPFORWARD_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_IPFORWARD_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_IPFORWARD_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionIPForward;

      *pConditionArrayCount = IPFORWARD_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionInboundTransport;

      *pConditionArrayCount = INBOUND_TRANSPORT_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionOutboundTransport;

      *pConditionArrayCount = OUTBOUND_TRANSPORT_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionStream;

      *pConditionArrayCount = STREAM_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_DATAGRAM_DATA_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_DATAGRAM_DATA_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionDatagramData;

      *pConditionArrayCount = DATAGRAM_DATA_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_ICMP_ERROR_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_ICMP_ERROR_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionInboundICMPError;

      *pConditionArrayCount = INBOUND_ICMP_ERROR_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionOutboundICMPError;

      *pConditionArrayCount = OUTBOUND_ICMP_ERROR_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionALEResourceAssignment;

      *pConditionArrayCount = ALE_RESOURCE_ASSIGNMENT_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_LISTEN_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_LISTEN_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionALEAuthListen;

      *pConditionArrayCount = ALE_AUTH_LISTEN_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionALEAuthRecvAccept;

      *pConditionArrayCount = ALE_AUTH_RECV_ACCEPT_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionALEAuthConnect;

      *pConditionArrayCount = ALE_AUTH_CONNECT_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionALEFlowEstablished;

      *pConditionArrayCount = ALE_FLOW_ESTABLISHED_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_IPSEC_KM_DEMUX_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_IPSEC_KM_DEMUX_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionIPsecKMDemux;

      *pConditionArrayCount = IPSEC_KM_DEMUX_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_IPSEC_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_IPSEC_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionIPsec;

      *pConditionArrayCount = IPSEC_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_IKEEXT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_IKEEXT_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionIKEExt;

      *pConditionArrayCount = IKEEXT_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_RPC_UM,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionRPCUM;

      *pConditionArrayCount = RPC_UM_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_RPC_EPMAP,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionRPCEPMap;

      *pConditionArrayCount = RPC_EPMAP_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_RPC_EP_ADD,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionRPCEPAdd;

      *pConditionArrayCount = RPC_EP_ADD_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_RPC_UM,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionRPCProxyConn;

      *pConditionArrayCount = RPC_PROXY_CONN_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_RPC_PROXY_IF,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionRPCProxyIf;

      *pConditionArrayCount = RPC_PROXY_IF_CONDITIONS_COUNT;
   }

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(HlprGUIDsAreEqual(&FWPM_LAYER_NAME_RESOLUTION_CACHE_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_NAME_RESOLUTION_CACHE_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionNameResolutionCache;

      *pConditionArrayCount = NAME_RESOLUTION_CACHE_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_RELEASE_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_RELEASE_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionALEResourceRelease;

      *pConditionArrayCount = ALE_RESOURCE_RELEASE_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionALEEndpointClosure;

      *pConditionArrayCount = ALE_ENDPOINT_CLOSURE_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_CONNECT_REDIRECT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_CONNECT_REDIRECT_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionALEConnectRedirect;

      *pConditionArrayCount = ALE_CONNECT_REDIRECT_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_BIND_REDIRECT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_ALE_BIND_REDIRECT_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionALEBindRedirect;

      *pConditionArrayCount = ALE_BIND_REDIRECT_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_PACKET_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_PACKET_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionStreamPacket;

      *pConditionArrayCount = STREAM_PACKET_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_KM_AUTHORIZATION,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionKMAuthorization;

      *pConditionArrayCount = KM_AUTHORIZATION_CONDITIONS_COUNT;
   }

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionInboundMACFrameEthernet;

      *pConditionArrayCount = INBOUND_MAC_FRAME_ETHERNET_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionOutboundMACFrameEthernet;

      *pConditionArrayCount = OUTBOUND_MAC_FRAME_ETHERNET_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionInboundMACFrameNative;

      *pConditionArrayCount = INBOUND_MAC_FRAME_NATIVE_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionOutboundMACFrameNative;

      *pConditionArrayCount = OUTBOUND_MAC_FRAME_NATIVE_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INGRESS_VSWITCH_ETHERNET,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionIngressVSwitchEthernet;

      *pConditionArrayCount = INGRESS_VSWITCH_ETHERNET_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_EGRESS_VSWITCH_ETHERNET,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionEgressVSwitchEthernet;

      *pConditionArrayCount = EGRESS_VSWITCH_ETHERNET_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionIngressVSwitchTransport;

      *pConditionArrayCount = INGRESS_VSWITCH_TRANSPORT_CONDITIONS_COUNT;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4,
                             pLayerKey) ||
           HlprGUIDsAreEqual(&FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6,
                             pLayerKey))
   {
      *pppFilterConditionArray = (GUID**)ppConditionEgressVSwitchTransport;

      *pConditionArrayCount = EGRESS_VSWITCH_TRANSPORT_CONDITIONS_COUNT;
   }

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_FAST,
                             pLayerKey))
   {
      *pppFilterConditionArray = 0;

      *pConditionArrayCount = 0;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_FAST,
                             pLayerKey))
   {
      *pppFilterConditionArray = 0;

      *pConditionArrayCount = 0;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST,
                             pLayerKey))
   {
      *pppFilterConditionArray = 0;

      *pConditionArrayCount = 0;
   }
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST,
                             pLayerKey))
   {
      *pppFilterConditionArray = 0;

      *pConditionArrayCount = 0;
   }

#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

   else
   {
      status = (UINT32)FWP_E_LAYER_NOT_FOUND;

      *pppFilterConditionArray = 0;

      *pConditionArrayCount = 0;

      HlprLogError(L"HlprFwpmLayerGetFilterConditionArrayByKey() [status: %#x]",
                   status);

   }

   return status;
}

/**
 @helper_function="HlprFwpmLayerGetIDByKey"
 
   Purpose:  Return the runtime ID of the layer provided the layer's key.                       <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA366492.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/FF570731.aspx              <br>
*/
_Success_(return < FWPS_BUILTIN_LAYER_MAX)
UINT8 HlprFwpmLayerGetIDByKey(_In_ const GUID* pLayerKey)
{
   ASSERT(pLayerKey);

   UINT8 layerID = FWPS_BUILTIN_LAYER_MAX;

   if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_IPPACKET_V4,
                        pLayerKey))
      layerID = FWPS_LAYER_INBOUND_IPPACKET_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_IPPACKET_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_IPPACKET_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_IPPACKET_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_IPPACKET_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_IPPACKET_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_IPPACKET_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_IPFORWARD_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_IPFORWARD_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_IPFORWARD_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_IPFORWARD_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_IPFORWARD_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_IPFORWARD_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_IPFORWARD_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_IPFORWARD_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_STREAM_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_STREAM_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_STREAM_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_STREAM_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_DATAGRAM_DATA_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_DATAGRAM_DATA_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_DATAGRAM_DATA_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_DATAGRAM_DATA_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_ICMP_ERROR_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_ICMP_ERROR_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_ICMP_ERROR_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_ICMP_ERROR_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_LISTEN_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_LISTEN_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_LISTEN_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_LISTEN_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_CONNECT_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_CONNECT_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_CONNECT_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(HlprGUIDsAreEqual(&FWPM_LAYER_NAME_RESOLUTION_CACHE_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_NAME_RESOLUTION_CACHE_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_NAME_RESOLUTION_CACHE_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_NAME_RESOLUTION_CACHE_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_RELEASE_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_RESOURCE_RELEASE_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_RESOURCE_RELEASE_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_RESOURCE_RELEASE_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_CONNECT_REDIRECT_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_CONNECT_REDIRECT_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_CONNECT_REDIRECT_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_CONNECT_REDIRECT_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_BIND_REDIRECT_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_BIND_REDIRECT_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_ALE_BIND_REDIRECT_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_ALE_BIND_REDIRECT_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_PACKET_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_STREAM_PACKET_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_STREAM_PACKET_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_STREAM_PACKET_V6;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INGRESS_VSWITCH_ETHERNET,
                             pLayerKey))
      layerID = FWPS_LAYER_INGRESS_VSWITCH_ETHERNET;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_EGRESS_VSWITCH_ETHERNET,
                             pLayerKey))
      layerID = FWPS_LAYER_EGRESS_VSWITCH_ETHERNET;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4,
                             pLayerKey))
      layerID = FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6,
                             pLayerKey))
      layerID = FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6;

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_TRANSPORT_FAST,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_FAST;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_TRANSPORT_FAST,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_FAST;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST,
                             pLayerKey))
      layerID = FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST;
   else if(HlprGUIDsAreEqual(&FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST,
                             pLayerKey))
      layerID = FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST;

#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

   return layerID;
}

/**
 @helper_function="HlprFwpmLayerGetID"
 
   Purpose:  Return the runtime ID of the layer provided a string representation of the layer's 
             name.                                                                              <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA366492.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/FF570731.aspx              <br>
*/
_Success_(return < FWPS_BUILTIN_LAYER_MAX)
UINT8 HlprFwpmLayerGetIDByString(_In_ PCWSTR pLayerString)
{
   ASSERT(pLayerString);

   UINT8 layerID = FWPS_BUILTIN_LAYER_MAX;

   if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_IPPACKET_V4",
                          pLayerString))
      layerID = FWPS_LAYER_INBOUND_IPPACKET_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_IPPACKET_V6",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_IPPACKET_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_IPPACKET_V4",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_IPPACKET_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_IPPACKET_V6",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_IPPACKET_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_IPFORWARD_V4",
                               pLayerString))
      layerID = FWPS_LAYER_IPFORWARD_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_IPFORWARD_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_IPFORWARD_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_IPFORWARD_V6",
                               pLayerString))
      layerID = FWPS_LAYER_IPFORWARD_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_IPFORWARD_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_IPFORWARD_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_V4",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_V6",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_V4",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_V6",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_V4",
                               pLayerString))
      layerID = FWPS_LAYER_STREAM_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_STREAM_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_V6",
                               pLayerString))
      layerID = FWPS_LAYER_STREAM_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_STREAM_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_DATAGRAM_DATA_V4",
                               pLayerString))
      layerID = FWPS_LAYER_DATAGRAM_DATA_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_DATAGRAM_DATA_V6",
                               pLayerString))
      layerID = FWPS_LAYER_DATAGRAM_DATA_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_ICMP_ERROR_V4",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_ICMP_ERROR_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_ICMP_ERROR_V6",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_ICMP_ERROR_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_LISTEN_V4",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_LISTEN_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_LISTEN_V6",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_LISTEN_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_CONNECT_V4",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_CONNECT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_CONNECT_V6",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_CONNECT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_NAME_RESOLUTION_CACHE_V4",
                               pLayerString))
      layerID = FWPS_LAYER_NAME_RESOLUTION_CACHE_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_NAME_RESOLUTION_CACHE_V6",
                               pLayerString))
      layerID = FWPS_LAYER_NAME_RESOLUTION_CACHE_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_RELEASE_V4",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_RESOURCE_RELEASE_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_RELEASE_V6",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_RESOURCE_RELEASE_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_CONNECT_REDIRECT_V4",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_CONNECT_REDIRECT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_CONNECT_REDIRECT_V6",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_CONNECT_REDIRECT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_BIND_REDIRECT_V4",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_BIND_REDIRECT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_BIND_REDIRECT_V6",
                               pLayerString))
      layerID = FWPS_LAYER_ALE_BIND_REDIRECT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_PACKET_V4",
                               pLayerString))
      layerID = FWPS_LAYER_STREAM_PACKET_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_PACKET_V6",
                               pLayerString))
      layerID = FWPS_LAYER_STREAM_PACKET_V6;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INGRESS_VSWITCH_ETHERNET",
                               pLayerString))
      layerID = FWPS_LAYER_INGRESS_VSWITCH_ETHERNET;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_EGRESS_VSWITCH_ETHERNET",
                               pLayerString))
      layerID = FWPS_LAYER_EGRESS_VSWITCH_ETHERNET;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4",
                               pLayerString))
      layerID = FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6",
                               pLayerString))
      layerID = FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4",
                               pLayerString))
      layerID = FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6",
                               pLayerString))
      layerID = FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6;

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_FAST",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_TRANSPORT_FAST;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_FAST",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_TRANSPORT_FAST;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST",
                               pLayerString))
      layerID = FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST",
                               pLayerString))
      layerID = FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST;

#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

   return layerID;
}

/**
 @helper_function="HlprFwpmLayerGetByID"
 
   Purpose:  Return a pointer to the GUID of the layer represented by the provider layer's 
             runtime ID.                                                                        <br>
                                                                                                <br>
   Notes:    Calling function must check for NULL pointer.                                      <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA366492.aspx              <br>
             HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/FF570731.aspx              <br>
*/
_Success_(return != 0)
const GUID* HlprFwpmLayerGetByID(_In_ const UINT32 layerID)
{
   const GUID* pLayerKey = 0;

   if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V4)
      pLayerKey = &FWPM_LAYER_INBOUND_IPPACKET_V4;
   else if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD;
   else if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V6)
      pLayerKey = &FWPM_LAYER_INBOUND_IPPACKET_V6;
   else if(layerID == FWPS_LAYER_INBOUND_IPPACKET_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD;
   else if(layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V4)
      pLayerKey = &FWPM_LAYER_OUTBOUND_IPPACKET_V4;
   else if(layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD;
   else if(layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
      pLayerKey = &FWPM_LAYER_OUTBOUND_IPPACKET_V6;
   else if(layerID == FWPS_LAYER_OUTBOUND_IPPACKET_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD;
   else if(layerID == FWPS_LAYER_IPFORWARD_V4)
      pLayerKey = &FWPM_LAYER_IPFORWARD_V4;
   else if(layerID == FWPS_LAYER_IPFORWARD_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_IPFORWARD_V4_DISCARD;
   else if(layerID == FWPS_LAYER_IPFORWARD_V6)
      pLayerKey = &FWPM_LAYER_IPFORWARD_V6;
   else if(layerID == FWPS_LAYER_IPFORWARD_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_IPFORWARD_V6_DISCARD;
   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_V4)
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_V4;
   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD;
   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_V6)
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_V6;
   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD;
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V4)
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD;
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V6)
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_V6;
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD;
   else if(layerID == FWPS_LAYER_STREAM_V4)
      pLayerKey = &FWPM_LAYER_STREAM_V4;
   else if(layerID == FWPS_LAYER_STREAM_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_STREAM_V4_DISCARD;
   else if(layerID == FWPS_LAYER_STREAM_V6)
      pLayerKey = &FWPM_LAYER_STREAM_V6;
   else if(layerID == FWPS_LAYER_STREAM_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_STREAM_V6_DISCARD;
   else if(layerID == FWPS_LAYER_DATAGRAM_DATA_V4)
      pLayerKey = &FWPM_LAYER_DATAGRAM_DATA_V4;
   else if(layerID == FWPS_LAYER_DATAGRAM_DATA_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD;
   else if(layerID == FWPS_LAYER_DATAGRAM_DATA_V6)
      pLayerKey = &FWPM_LAYER_DATAGRAM_DATA_V6;
   else if(layerID == FWPS_LAYER_DATAGRAM_DATA_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD;
   else if(layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V4)
      pLayerKey = &FWPM_LAYER_INBOUND_ICMP_ERROR_V4;
   else if(layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD;
   else if(layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V6)
      pLayerKey = &FWPM_LAYER_INBOUND_ICMP_ERROR_V6;
   else if(layerID == FWPS_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD;
   else if(layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4)
      pLayerKey = &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4;
   else if(layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD;
   else if(layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6)
      pLayerKey = &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6;
   else if(layerID == FWPS_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4)
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4;
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6)
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6;
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V4)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_LISTEN_V4;
   else if(layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V6)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_LISTEN_V6;
   else if(layerID == FWPS_LAYER_ALE_AUTH_LISTEN_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
   else if(layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
   else if(layerID == FWPS_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V4)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_CONNECT_V4;
   else if(layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V6)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_CONNECT_V6;
   else if(layerID == FWPS_LAYER_ALE_AUTH_CONNECT_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4)
      pLayerKey = &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4;
   else if(layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD;
   else if(layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)
      pLayerKey = &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6;
   else if(layerID == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD)
      pLayerKey = &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(layerID == FWPS_LAYER_NAME_RESOLUTION_CACHE_V4)
      pLayerKey = &FWPM_LAYER_NAME_RESOLUTION_CACHE_V4;
   else if(layerID == FWPS_LAYER_NAME_RESOLUTION_CACHE_V6)
      pLayerKey = &FWPM_LAYER_NAME_RESOLUTION_CACHE_V6;
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_RELEASE_V4)
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_RELEASE_V4;
   else if(layerID == FWPS_LAYER_ALE_RESOURCE_RELEASE_V6)
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_RELEASE_V6;
   else if(layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V4)
      pLayerKey = &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4;
   else if(layerID == FWPS_LAYER_ALE_ENDPOINT_CLOSURE_V6)
      pLayerKey = &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6;
   else if(layerID == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4)
      pLayerKey = &FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
   else if(layerID == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6)
      pLayerKey = &FWPM_LAYER_ALE_CONNECT_REDIRECT_V6;
   else if(layerID == FWPS_LAYER_ALE_BIND_REDIRECT_V4)
      pLayerKey = &FWPM_LAYER_ALE_BIND_REDIRECT_V4;
   else if(layerID == FWPS_LAYER_ALE_BIND_REDIRECT_V6)
      pLayerKey = &FWPM_LAYER_ALE_BIND_REDIRECT_V6;
   else if(layerID == FWPS_LAYER_STREAM_PACKET_V4)
      pLayerKey = &FWPM_LAYER_STREAM_PACKET_V4;
   else if(layerID == FWPS_LAYER_STREAM_PACKET_V6)
      pLayerKey = &FWPM_LAYER_STREAM_PACKET_V6;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(layerID == FWPS_LAYER_INBOUND_MAC_FRAME_ETHERNET)
      pLayerKey = &FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET;
   else if(layerID == FWPS_LAYER_OUTBOUND_MAC_FRAME_ETHERNET)
      pLayerKey = &FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET;
   else if(layerID == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE)
      pLayerKey = &FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE;
   else if(layerID == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE)
      pLayerKey = &FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE;
   else if(layerID == FWPS_LAYER_INGRESS_VSWITCH_ETHERNET)
      pLayerKey = &FWPM_LAYER_INGRESS_VSWITCH_ETHERNET;
   else if(layerID == FWPS_LAYER_EGRESS_VSWITCH_ETHERNET)
      pLayerKey = &FWPM_LAYER_EGRESS_VSWITCH_ETHERNET;
   else if(layerID == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V4)
      pLayerKey = &FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4;
   else if(layerID == FWPS_LAYER_INGRESS_VSWITCH_TRANSPORT_V6)
      pLayerKey = &FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6;
   else if(layerID == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V4)
      pLayerKey = &FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4;
   else if(layerID == FWPS_LAYER_EGRESS_VSWITCH_TRANSPORT_V6)
      pLayerKey = &FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6;

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

   else if(layerID == FWPS_LAYER_INBOUND_TRANSPORT_FAST)
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_FAST;
   else if(layerID == FWPS_LAYER_OUTBOUND_TRANSPORT_FAST)
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_FAST;
   else if(layerID == FWPS_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST)
      pLayerKey = &FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST;
   else if(layerID == FWPS_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST)
      pLayerKey = &FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST;

#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

   return pLayerKey;
}

/**
 @helper_function="HlprFwpmLayerGetByString"
 
   Purpose:  Return a pointer to the GUID of the layer represented by the provided string.      <br>
                                                                                                <br>
   Notes:    Calling function must check for NULL pointer.                                      <br>
                                                                                                <br>
   MSDN_Ref: HTTP://MSDN.Microsoft.com/En-US/Library/Windows/Desktop/AA366492.aspx              <br>
*/
_Success_(return != 0)
const GUID* HlprFwpmLayerGetByString(_In_ PCWSTR pLayerString)
{
   ASSERT(pLayerString);

   const GUID* pLayerKey = 0;

   if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_IPPACKET_V4",
                          pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_IPPACKET_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_IPPACKET_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_IPPACKET_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_IPPACKET_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_IPPACKET_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_IPPACKET_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_IPPACKET_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_IPFORWARD_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_IPFORWARD_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_IPFORWARD_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_IPFORWARD_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_IPFORWARD_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_IPFORWARD_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_IPFORWARD_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_IPFORWARD_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_STREAM_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_STREAM_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_STREAM_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_STREAM_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_DATAGRAM_DATA_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_DATAGRAM_DATA_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_DATAGRAM_DATA_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_DATAGRAM_DATA_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_ICMP_ERROR_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_ICMP_ERROR_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_ICMP_ERROR_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_ICMP_ERROR_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_LISTEN_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_LISTEN_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_LISTEN_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_LISTEN_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_CONNECT_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_CONNECT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_CONNECT_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_CONNECT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD;

#if(NTDDI_VERSION >= NTDDI_WIN7)

   else if(HlprStringsAreEqual(L"FWPM_LAYER_NAME_RESOLUTION_CACHE_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_NAME_RESOLUTION_CACHE_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_NAME_RESOLUTION_CACHE_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_NAME_RESOLUTION_CACHE_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_RELEASE_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_RELEASE_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_RESOURCE_RELEASE_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_RESOURCE_RELEASE_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_CONNECT_REDIRECT_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_CONNECT_REDIRECT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_CONNECT_REDIRECT_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_CONNECT_REDIRECT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_BIND_REDIRECT_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_BIND_REDIRECT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_ALE_BIND_REDIRECT_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_ALE_BIND_REDIRECT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_PACKET_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_STREAM_PACKET_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_STREAM_PACKET_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_STREAM_PACKET_V6;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INGRESS_VSWITCH_ETHERNET",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INGRESS_VSWITCH_ETHERNET;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_EGRESS_VSWITCH_ETHERNET",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_EGRESS_VSWITCH_ETHERNET;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6;

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_TRANSPORT_FAST",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_TRANSPORT_FAST;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_TRANSPORT_FAST",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_TRANSPORT_FAST;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST;
   else if(HlprStringsAreEqual(L"FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST",
                               pLayerString))
      pLayerKey = &FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST;

#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

   return pLayerKey;
}

/**
 @helper function="HlprFwpmLayerIsUserMode"
 
   Purpose: Determine if the layer is a user-mode layer.                                        <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
BOOLEAN HlprFwpmLayerIsUserMode(_In_ const GUID* pLayerKey)
{
   ASSERT(pLayerKey);

   BOOLEAN isUserMode = FALSE;
   
   for(UINT32 layerIndex = 0;
       layerIndex < TOTAL_USER_MODE_LAYER_COUNT &&
       isUserMode == FALSE;
       layerIndex++)
   {
      if(pLayerKey == ppUserModeLayerKeyArray[layerIndex])
        isUserMode = TRUE;
   }

   return isUserMode;
}

/**
 @helper function="HlprFwpmLayerIsKernelMode"
 
   Purpose: Determine if the layer is a kernel-mode layer.                                      <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
BOOLEAN HlprFwpmLayerIsKernelMode(_In_ const GUID* pLayerKey)
{
   ASSERT(pLayerKey);

   return !(HlprFwpmLayerIsUserMode(pLayerKey));
}

/**
 @helper function="HlprFwpmLayerIsIPv4"
 
   Purpose: Determine if the layer is an IPv4 layer.                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
BOOLEAN HlprFwpmLayerIsIPv4(_In_ const GUID* pLayerKey)
{
   ASSERT(pLayerKey);

   BOOLEAN isIPv4 = FALSE;

   if(*pLayerKey == FWPM_LAYER_INBOUND_IPPACKET_V4 ||
      *pLayerKey == FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V4 ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_IPFORWARD_V4 ||
      *pLayerKey == FWPM_LAYER_IPFORWARD_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_INBOUND_TRANSPORT_V4 ||
      *pLayerKey == FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4 ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_STREAM_V4 ||
      *pLayerKey == FWPM_LAYER_STREAM_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_DATAGRAM_DATA_V4 ||
      *pLayerKey == FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V4 ||
      *pLayerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4 ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4 ||
      *pLayerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V4 ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V4 ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4 ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4 ||
      *pLayerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD ||

#if(NTDDI_VERSION >= NTDDI_WIN7)

      *pLayerKey == FWPM_LAYER_NAME_RESOLUTION_CACHE_V4 ||
      *pLayerKey == FWPM_LAYER_ALE_RESOURCE_RELEASE_V4 ||
      *pLayerKey == FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4 ||
      *pLayerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V4 ||
      *pLayerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V4 ||
      *pLayerKey == FWPM_LAYER_STREAM_PACKET_V4 ||

#if(NTDDI_VERSION >= NTDDI_WIN8)

      *pLayerKey == FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4 ||
      *pLayerKey == FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4 ||

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

      *pLayerKey == FWPM_LAYER_IPSEC_KM_DEMUX_V4 ||
      *pLayerKey == FWPM_LAYER_IPSEC_V4 ||
      *pLayerKey == FWPM_LAYER_IKEEXT_V4)
      isIPv4 = TRUE;

   return isIPv4;
}

/**
 @helper function="HlprFwpmLayerIsIPv6"
 
   Purpose: Determine if the layer is an IPv6 layer.                                            <br>
                                                                                                <br>
   Notes:                                                                                       <br>
                                                                                                <br>
   MSDN_Ref:                                                                                    <br>
*/
BOOLEAN HlprFwpmLayerIsIPv6(_In_ const GUID* pLayerKey)
{
   ASSERT(pLayerKey);

   BOOLEAN isIPv6 = FALSE;

   if(*pLayerKey == FWPM_LAYER_INBOUND_IPPACKET_V6 ||
      *pLayerKey == FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V6 ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_IPFORWARD_V6 ||
      *pLayerKey == FWPM_LAYER_IPFORWARD_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_INBOUND_TRANSPORT_V6 ||
      *pLayerKey == FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6 ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_STREAM_V6 ||
      *pLayerKey == FWPM_LAYER_STREAM_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_DATAGRAM_DATA_V6 ||
      *pLayerKey == FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V6 ||
      *pLayerKey == FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6 ||
      *pLayerKey == FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6 ||
      *pLayerKey == FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V6 ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V6 ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6 ||
      *pLayerKey == FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD ||
      *pLayerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6 ||
      *pLayerKey == FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD ||

#if(NTDDI_VERSION >= NTDDI_WIN7)

      *pLayerKey == FWPM_LAYER_NAME_RESOLUTION_CACHE_V6 ||
      *pLayerKey == FWPM_LAYER_ALE_RESOURCE_RELEASE_V6 ||
      *pLayerKey == FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6 ||
      *pLayerKey == FWPM_LAYER_ALE_CONNECT_REDIRECT_V6 ||
      *pLayerKey == FWPM_LAYER_ALE_BIND_REDIRECT_V6 ||
      *pLayerKey == FWPM_LAYER_STREAM_PACKET_V6 ||

#if(NTDDI_VERSION >= NTDDI_WIN8)

      *pLayerKey == FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6 ||
      *pLayerKey == FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6 ||

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

      *pLayerKey == FWPM_LAYER_IPSEC_KM_DEMUX_V6 ||
      *pLayerKey == FWPM_LAYER_IPSEC_V6 ||
      *pLayerKey == FWPM_LAYER_IKEEXT_V6)
      isIPv6 = TRUE;

   return isIPv6;
}
