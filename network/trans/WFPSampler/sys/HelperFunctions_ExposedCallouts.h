////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_ExposedCallouts.h
//
//   Abstract:
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   - Add support for ADVANCED_PACKET_INJECTION, 
//                                             FLOW_ASSOCIATION, and PEND_ENDPOINT_CLOSURE callouts
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#if DBG

typedef struct INJECTION_COUNTERS_
{
   INT64 inboundNetwork_IPv4;
   INT64 inboundNetwork_IPv6;
   INT64 outboundNetwork_IPv4;
   INT64 outboundNetwork_IPv6;
   INT64 inboundForward_IPv4;
   INT64 inboundForward_IPv6;
   INT64 outboundForward_IPv4;
   INT64 outboundForward_IPv6;
   INT64 inboundTransport_IPv4;
   INT64 inboundTransport_IPv6;
   INT64 outboundTransport_IPv4;
   INT64 outboundTransport_IPv6;
   INT64 inboundStream_IPv4;
   INT64 inboundStream_IPv6;
   INT64 outboundStream_IPv4;
   INT64 outboundStream_IPv6;

#if(NTDDI_VERSION >= NTDDI_WIN8)

   INT64 inboundMAC_IPv4;
   INT64 inboundMAC_IPv6;
   INT64 inboundMAC_Unknown;
   INT64 outboundMAC_IPv4;
   INT64 outboundMAC_IPv6;
   INT64 outboundMAC_Unknown;
   INT64 ingressVSwitch_IPv4;
   INT64 ingressVSwitch_IPv6;
   INT64 ingressVSwitch_Unknown;
   INT64 egressVSwitch_IPv4;
   INT64 egressVSwitch_IPv6;
   INT64 egressVSwitch_Unknown;

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

}INJECTION_COUNTERS, *PINJECTION_COUNTERS;

#endif /// DBG

static const GUID* const ppAdvancedPacketInjection[]   = {&WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_IPFORWARD_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_IPFORWARD_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_DATAGRAM_DATA_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_DATAGRAM_DATA_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V6,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V6,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_STREAM_PACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_INGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_ADVANCED_PACKET_INJECTION_AT_EGRESS_VSWITCH_ETHERNET,

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

                                                         };
static const GUID* const ppBasicActionBlock[]          = {&WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_IPFORWARD_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_IPFORWARD_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_STREAM_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_STREAM_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_DATAGRAM_DATA_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_DATAGRAM_DATA_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_ASSIGNMENT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_ASSIGNMENT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_LISTEN_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_LISTEN_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_AUTH_CONNECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_FLOW_ESTABLISHED_V6,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_RELEASE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_RESOURCE_RELEASE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_ENDPOINT_CLOSURE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_ENDPOINT_CLOSURE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_CONNECT_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_CONNECT_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_BIND_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_ALE_BIND_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_STREAM_PACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_OUTBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_EGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_INGRESS_VSWITCH_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_EGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_BLOCK_AT_EGRESS_VSWITCH_TRANSPORT_V6,

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

                                                         };
static const GUID* const ppBasicActionContinue[]       = {&WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_IPFORWARD_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_IPFORWARD_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_STREAM_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_STREAM_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_DATAGRAM_DATA_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_DATAGRAM_DATA_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_ASSIGNMENT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_ASSIGNMENT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_LISTEN_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_LISTEN_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_AUTH_CONNECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_FLOW_ESTABLISHED_V6,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_RELEASE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_RESOURCE_RELEASE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_ENDPOINT_CLOSURE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_ENDPOINT_CLOSURE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_CONNECT_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_CONNECT_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_BIND_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_ALE_BIND_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_STREAM_PACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_OUTBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_EGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_INGRESS_VSWITCH_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_EGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_CONTINUE_AT_EGRESS_VSWITCH_TRANSPORT_V6,
                                                          
#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)
                                                          
                                                         };
static const GUID* const ppBasicActionPermit[]         = {&WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_IPFORWARD_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_IPFORWARD_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_STREAM_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_STREAM_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_DATAGRAM_DATA_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_DATAGRAM_DATA_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_ASSIGNMENT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_ASSIGNMENT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_LISTEN_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_LISTEN_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_AUTH_CONNECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_FLOW_ESTABLISHED_V6,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_RELEASE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_RESOURCE_RELEASE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_ENDPOINT_CLOSURE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_ENDPOINT_CLOSURE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_CONNECT_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_CONNECT_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_BIND_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_ALE_BIND_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_STREAM_PACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_OUTBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_EGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_INGRESS_VSWITCH_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_EGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_PERMIT_AT_EGRESS_VSWITCH_TRANSPORT_V6,

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

                                                         };
static const GUID* const ppBasicActionRandom[]         = {&WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_IPFORWARD_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_IPFORWARD_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_STREAM_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_STREAM_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_DATAGRAM_DATA_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_DATAGRAM_DATA_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_ASSIGNMENT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_ASSIGNMENT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_LISTEN_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_LISTEN_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_AUTH_CONNECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_FLOW_ESTABLISHED_V6,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_RELEASE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_RESOURCE_RELEASE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_ENDPOINT_CLOSURE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_ENDPOINT_CLOSURE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_CONNECT_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_CONNECT_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_BIND_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_ALE_BIND_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_STREAM_PACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_OUTBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_EGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_INGRESS_VSWITCH_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_EGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_ACTION_RANDOM_AT_EGRESS_VSWITCH_TRANSPORT_V6,

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

                                                         };
static const GUID* const ppBasicPacketExamination[]    = {&WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_IPPACKET_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_IPPACKET_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_IPPACKET_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_IPPACKET_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_IPFORWARD_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_IPFORWARD_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_IPFORWARD_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_IPFORWARD_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_TRANSPORT_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_TRANSPORT_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_TRANSPORT_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_TRANSPORT_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_STREAM_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_STREAM_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_STREAM_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_STREAM_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_DATAGRAM_DATA_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_DATAGRAM_DATA_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_DATAGRAM_DATA_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_DATAGRAM_DATA_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_ICMP_ERROR_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_ICMP_ERROR_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_ICMP_ERROR_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_ICMP_ERROR_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_RESOURCE_ASSIGNMENT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_RESOURCE_ASSIGNMENT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_LISTEN_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_LISTEN_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_LISTEN_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_LISTEN_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_RECV_ACCEPT_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_RECV_ACCEPT_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_CONNECT_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_CONNECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_AUTH_CONNECT_V6_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_FLOW_ESTABLISHED_V4_DISCARD,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_FLOW_ESTABLISHED_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_FLOW_ESTABLISHED_V6_DISCARD,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_RESOURCE_RELEASE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_RESOURCE_RELEASE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_ENDPOINT_CLOSURE_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_ENDPOINT_CLOSURE_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_CONNECT_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_CONNECT_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_BIND_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_ALE_BIND_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_STREAM_PACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_OUTBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_EGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_INGRESS_VSWITCH_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_EGRESS_VSWITCH_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_EXAMINATION_AT_EGRESS_VSWITCH_TRANSPORT_V6,

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

                                                         };
static const GUID* const ppBasicPacketInjection[]      = {&WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_IPFORWARD_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_IPFORWARD_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_DATAGRAM_DATA_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_DATAGRAM_DATA_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V6,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_STREAM_PACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_INGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_INJECTION_AT_EGRESS_VSWITCH_ETHERNET,

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

                                                         };
static const GUID* const ppBasicPacketModification[]   = {&WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_IPFORWARD_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_IPFORWARD_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_DATAGRAM_DATA_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_DATAGRAM_DATA_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_AUTH_CONNECT_V6,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_ALE_FLOW_ESTABLISHED_V6,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_STREAM_PACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_OUTBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_INGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_BASIC_PACKET_MODIFICATION_AT_EGRESS_VSWITCH_ETHERNET,

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

                                                         };
static const GUID* const ppBasicStreamInjection[]      = {&WFPSAMPLER_CALLOUT_BASIC_STREAM_INJECTION_AT_STREAM_V4,
                                                          &WFPSAMPLER_CALLOUT_BASIC_STREAM_INJECTION_AT_STREAM_V6,
                                                         };
static const GUID* const ppFastPacketInjection[]       = {&WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_IPFORWARD_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_IPFORWARD_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_TRANSPORT_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_DATAGRAM_DATA_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_DATAGRAM_DATA_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_ICMP_ERROR_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_AUTH_CONNECT_V6,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_ALE_FLOW_ESTABLISHED_V6,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_STREAM_PACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_OUTBOUND_MAC_FRAME_NATIVE,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_INGRESS_VSWITCH_ETHERNET,
                                                          &WFPSAMPLER_CALLOUT_FAST_PACKET_INJECTION_AT_EGRESS_VSWITCH_ETHERNET,

#endif // (NTDDI_VERSION >= NTDDI_WIN8)
#endif // (NTDDI_VERSION >= NTDDI_WIN7)

                                                         };
static const GUID* const ppFastStreamInjection[]       = {&WFPSAMPLER_CALLOUT_FAST_STREAM_INJECTION_AT_STREAM_V4,
                                                          &WFPSAMPLER_CALLOUT_FAST_STREAM_INJECTION_AT_STREAM_V6,
                                                         };
static const GUID* const ppFlowAssociation[]           = {&WFPSAMPLER_CALLOUT_FLOW_ASSOCIATION_AT_ALE_FLOW_ESTABLISHED_V4,
                                                          &WFPSAMPLER_CALLOUT_FLOW_ASSOCIATION_AT_ALE_FLOW_ESTABLISHED_V6,
                                                         };
static const GUID* const ppPendAuthorization[]         = {&WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_RESOURCE_ASSIGNMENT_V4,
                                                          &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_RESOURCE_ASSIGNMENT_V6,
                                                          &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_LISTEN_V4,
                                                          &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_LISTEN_V6,
                                                          &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_RECV_ACCEPT_V4,
                                                          &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_RECV_ACCEPT_V6,
                                                          &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_CONNECT_V4,
                                                          &WFPSAMPLER_CALLOUT_PEND_AUTHORIZATION_AT_ALE_AUTH_CONNECT_V6,
                                                         };
static const GUID* const ppPendEndpointClosure[]       = {&WFPSAMPLER_CALLOUT_PEND_ENDPOINT_CLOSURE_AT_ALE_ENDPOINT_CLOSURE_V4,
                                                          &WFPSAMPLER_CALLOUT_PEND_ENDPOINT_CLOSURE_AT_ALE_ENDPOINT_CLOSURE_V6,
                                                         };
static const GUID* const ppProxyByALERedirect[]        = {&WFPSAMPLER_CALLOUT_PROXY_BY_ALE_AT_CONNECT_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_PROXY_BY_ALE_AT_CONNECT_REDIRECT_V6,
                                                          &WFPSAMPLER_CALLOUT_PROXY_BY_ALE_AT_BIND_REDIRECT_V4,
                                                          &WFPSAMPLER_CALLOUT_PROXY_BY_ALE_AT_BIND_REDIRECT_V6,
                                                         };
static const GUID* const ppProxyByInjection[]          = {&WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION_AT_INBOUND_IPPACKET_V4,
                                                          &WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION_AT_INBOUND_IPPACKET_V6,
                                                          &WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION_AT_OUTBOUND_TRANSPORT_V4,
                                                          &WFPSAMPLER_CALLOUT_PROXY_BY_INJECTION_AT_OUTBOUND_TRANSPORT_V6,
                                                         };


const UINT32 ADVANCED_PACKET_INJECTION_COUNT = RTL_NUMBER_OF(ppAdvancedPacketInjection);
const UINT32 BASIC_ACTION_BLOCK_COUNT        = RTL_NUMBER_OF(ppBasicActionBlock);
const UINT32 BASIC_ACTION_CONTINUE_COUNT     = RTL_NUMBER_OF(ppBasicActionContinue);
const UINT32 BASIC_ACTION_PERMIT_COUNT       = RTL_NUMBER_OF(ppBasicActionPermit);
const UINT32 BASIC_ACTION_RANDOM_COUNT       = RTL_NUMBER_OF(ppBasicActionRandom);
const UINT32 BASIC_PACKET_EXAMINATION_COUNT  = RTL_NUMBER_OF(ppBasicPacketExamination);
const UINT32 BASIC_PACKET_INJECTION_COUNT    = RTL_NUMBER_OF(ppBasicPacketInjection);
const UINT32 BASIC_PACKET_MODIFICATION_COUNT = RTL_NUMBER_OF(ppBasicPacketModification);
const UINT32 BASIC_STREAM_INJECTION_COUNT    = RTL_NUMBER_OF(ppBasicStreamInjection);
const UINT32 FAST_PACKET_INJECTION_COUNT     = RTL_NUMBER_OF(ppFastPacketInjection);
const UINT32 FAST_STREAM_INJECTION_COUNT     = RTL_NUMBER_OF(ppFastStreamInjection);
const UINT32 FLOW_ASSOCIATION_COUNT          = RTL_NUMBER_OF(ppFlowAssociation);
const UINT32 PEND_AUTHORIZATION_COUNT        = RTL_NUMBER_OF(ppPendAuthorization);
const UINT32 PEND_ENDPOINT_CLOSURE_COUNT     = RTL_NUMBER_OF(ppPendEndpointClosure);
const UINT32 PROXY_BY_ALE_REDIRECT_COUNT     = RTL_NUMBER_OF(ppProxyByALERedirect);
const UINT32 PROXY_BY_INJECTION_COUNT        = RTL_NUMBER_OF(ppProxyByInjection);

const UINT32 EXPOSED_CALLOUT_COUNT = ADVANCED_PACKET_INJECTION_COUNT +
                                     BASIC_ACTION_BLOCK_COUNT +
                                     BASIC_ACTION_CONTINUE_COUNT +
                                     BASIC_ACTION_PERMIT_COUNT +
                                     BASIC_ACTION_RANDOM_COUNT +
                                     BASIC_PACKET_EXAMINATION_COUNT +
                                     BASIC_PACKET_INJECTION_COUNT +
                                     BASIC_PACKET_MODIFICATION_COUNT +
                                     BASIC_STREAM_INJECTION_COUNT +
                                     FAST_PACKET_INJECTION_COUNT +
                                     FAST_STREAM_INJECTION_COUNT +
                                     FLOW_ASSOCIATION_COUNT +
                                     PEND_AUTHORIZATION_COUNT +
                                     PEND_ENDPOINT_CLOSURE_COUNT +
                                     PROXY_BY_ALE_REDIRECT_COUNT +
                                     PROXY_BY_INJECTION_COUNT;

/// Indices of where each group of callouts begins in an uber array (created in HelperFunctions_ExposedCallouts.cpp)
const UINT32 BASE_INDEX_API = 0;
const UINT32 BASE_INDEX_BAB = BASE_INDEX_API + ADVANCED_PACKET_INJECTION_COUNT;
const UINT32 BASE_INDEX_BAC = BASE_INDEX_BAB + BASIC_ACTION_BLOCK_COUNT;
const UINT32 BASE_INDEX_BAP = BASE_INDEX_BAC + BASIC_ACTION_CONTINUE_COUNT;
const UINT32 BASE_INDEX_BAR = BASE_INDEX_BAP + BASIC_ACTION_PERMIT_COUNT;
const UINT32 BASE_INDEX_BPE = BASE_INDEX_BAR + BASIC_ACTION_RANDOM_COUNT;
const UINT32 BASE_INDEX_BPI = BASE_INDEX_BPE + BASIC_PACKET_EXAMINATION_COUNT;
const UINT32 BASE_INDEX_BPM = BASE_INDEX_BPI + BASIC_PACKET_INJECTION_COUNT;
const UINT32 BASE_INDEX_BSI = BASE_INDEX_BPM + BASIC_PACKET_MODIFICATION_COUNT;
const UINT32 BASE_INDEX_FPI = BASE_INDEX_BSI + BASIC_STREAM_INJECTION_COUNT;
const UINT32 BASE_INDEX_FSI = BASE_INDEX_FPI + FAST_PACKET_INJECTION_COUNT;
const UINT32 BASE_INDEX_FA  = BASE_INDEX_FSI + FAST_STREAM_INJECTION_COUNT;
const UINT32 BASE_INDEX_PA  = BASE_INDEX_FA  + FLOW_ASSOCIATION_COUNT;
const UINT32 BASE_INDEX_PEC = BASE_INDEX_PA  + PEND_AUTHORIZATION_COUNT;
const UINT32 BASE_INDEX_PBA = BASE_INDEX_PEC + PEND_ENDPOINT_CLOSURE_COUNT;
const UINT32 BASE_INDEX_PBI = BASE_INDEX_PBA + PROXY_BY_ALE_REDIRECT_COUNT;

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprExposedCalloutsUnregister();

_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprExposedCalloutsRegister();

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
PSTR KrnlHlprExposedCalloutToString(_In_ const GUID* pCalloutKey);
