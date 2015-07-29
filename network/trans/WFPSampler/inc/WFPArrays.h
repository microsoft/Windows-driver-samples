////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      WFPArrays.h
//
//   Abstract:
//      This module contains global variables and definitions of WFP specific data for the 
//         WFPSampler project
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WFP_SAMPLER_WFP_ARRAYS_H
#define WFP_SAMPLER_WFP_ARRAYS_H

#if(NTDDI_VERSION >= NTDDI_VISTA)

const GUID* const ppLayerKeyArray[]         = {&FWPM_LAYER_INBOUND_IPPACKET_V4,               ///  0
                                               &FWPM_LAYER_INBOUND_IPPACKET_V4_DISCARD,
                                               &FWPM_LAYER_INBOUND_IPPACKET_V6,
                                               &FWPM_LAYER_INBOUND_IPPACKET_V6_DISCARD,
                                               &FWPM_LAYER_OUTBOUND_IPPACKET_V4,
                                               &FWPM_LAYER_OUTBOUND_IPPACKET_V4_DISCARD,
                                               &FWPM_LAYER_OUTBOUND_IPPACKET_V6,
                                               &FWPM_LAYER_OUTBOUND_IPPACKET_V6_DISCARD,
                                               &FWPM_LAYER_IPFORWARD_V4,
                                               &FWPM_LAYER_IPFORWARD_V4_DISCARD,
                                               &FWPM_LAYER_IPFORWARD_V6,                      /// 10
                                               &FWPM_LAYER_IPFORWARD_V6_DISCARD,
                                               &FWPM_LAYER_INBOUND_TRANSPORT_V4,
                                               &FWPM_LAYER_INBOUND_TRANSPORT_V4_DISCARD,
                                               &FWPM_LAYER_INBOUND_TRANSPORT_V6,
                                               &FWPM_LAYER_INBOUND_TRANSPORT_V6_DISCARD,
                                               &FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
                                               &FWPM_LAYER_OUTBOUND_TRANSPORT_V4_DISCARD,
                                               &FWPM_LAYER_OUTBOUND_TRANSPORT_V6,
                                               &FWPM_LAYER_OUTBOUND_TRANSPORT_V6_DISCARD,
                                               &FWPM_LAYER_STREAM_V4,                         /// 20
                                               &FWPM_LAYER_STREAM_V4_DISCARD,
                                               &FWPM_LAYER_STREAM_V6,
                                               &FWPM_LAYER_STREAM_V6_DISCARD,
                                               &FWPM_LAYER_DATAGRAM_DATA_V4,
                                               &FWPM_LAYER_DATAGRAM_DATA_V4_DISCARD,
                                               &FWPM_LAYER_DATAGRAM_DATA_V6,
                                               &FWPM_LAYER_DATAGRAM_DATA_V6_DISCARD,
                                               &FWPM_LAYER_INBOUND_ICMP_ERROR_V4,
                                               &FWPM_LAYER_INBOUND_ICMP_ERROR_V4_DISCARD,
                                               &FWPM_LAYER_INBOUND_ICMP_ERROR_V6,             /// 30
                                               &FWPM_LAYER_INBOUND_ICMP_ERROR_V6_DISCARD,
                                               &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4,
                                               &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V4_DISCARD,
                                               &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6,
                                               &FWPM_LAYER_OUTBOUND_ICMP_ERROR_V6_DISCARD,
                                               &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4,
                                               &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V4_DISCARD,
                                               &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6,
                                               &FWPM_LAYER_ALE_RESOURCE_ASSIGNMENT_V6_DISCARD,
                                               &FWPM_LAYER_ALE_AUTH_LISTEN_V4,                /// 40
                                               &FWPM_LAYER_ALE_AUTH_LISTEN_V4_DISCARD,
                                               &FWPM_LAYER_ALE_AUTH_LISTEN_V6,
                                               &FWPM_LAYER_ALE_AUTH_LISTEN_V6_DISCARD,
                                               &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
                                               &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4_DISCARD,
                                               &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6,
                                               &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V6_DISCARD,
                                               &FWPM_LAYER_ALE_AUTH_CONNECT_V4,
                                               &FWPM_LAYER_ALE_AUTH_CONNECT_V4_DISCARD,
                                               &FWPM_LAYER_ALE_AUTH_CONNECT_V6,               /// 50
                                               &FWPM_LAYER_ALE_AUTH_CONNECT_V6_DISCARD,
                                               &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4,
                                               &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4_DISCARD,
                                               &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6,
                                               &FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6_DISCARD,
#if(NTDDI_VERSION >= NTDDI_WIN7)

                                               &FWPM_LAYER_NAME_RESOLUTION_CACHE_V4,          /// 60
                                               &FWPM_LAYER_NAME_RESOLUTION_CACHE_V6,
                                               &FWPM_LAYER_ALE_RESOURCE_RELEASE_V4,
                                               &FWPM_LAYER_ALE_RESOURCE_RELEASE_V6,
                                               &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4,
                                               &FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6,
                                               &FWPM_LAYER_ALE_CONNECT_REDIRECT_V4,
                                               &FWPM_LAYER_ALE_CONNECT_REDIRECT_V6,
                                               &FWPM_LAYER_ALE_BIND_REDIRECT_V4,
                                               &FWPM_LAYER_ALE_BIND_REDIRECT_V6,
                                               &FWPM_LAYER_STREAM_PACKET_V4,                  /// 70
                                               &FWPM_LAYER_STREAM_PACKET_V6,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                               &FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET,        /// 56
                                               &FWPM_LAYER_OUTBOUND_MAC_FRAME_ETHERNET,       /// 57
                                               &FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE,          /// 58
                                               &FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE,         /// 59
                                               &FWPM_LAYER_INGRESS_VSWITCH_ETHERNET,          /// 72
                                               &FWPM_LAYER_EGRESS_VSWITCH_ETHERNET,
                                               &FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V4,
                                               &FWPM_LAYER_INGRESS_VSWITCH_TRANSPORT_V6,
                                               &FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V4,
                                               &FWPM_LAYER_EGRESS_VSWITCH_TRANSPORT_V6,

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

                                               &FWPM_LAYER_INBOUND_TRANSPORT_FAST,            /// 78
                                               &FWPM_LAYER_OUTBOUND_TRANSPORT_FAST,
                                               &FWPM_LAYER_INBOUND_MAC_FRAME_NATIVE_FAST,     /// 80
                                               &FWPM_LAYER_OUTBOUND_MAC_FRAME_NATIVE_FAST,


#endif /// (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

                                               &FWPM_LAYER_IPSEC_KM_DEMUX_V4,
                                               &FWPM_LAYER_IPSEC_KM_DEMUX_V6,
                                               &FWPM_LAYER_IPSEC_V4,
                                               &FWPM_LAYER_IPSEC_V6,
                                               &FWPM_LAYER_IKEEXT_V4,
                                               &FWPM_LAYER_IKEEXT_V6,
                                               &FWPM_LAYER_RPC_UM,
                                               &FWPM_LAYER_RPC_EPMAP,
                                               &FWPM_LAYER_RPC_EP_ADD,                        /// 90
                                               &FWPM_LAYER_RPC_PROXY_CONN,
                                               &FWPM_LAYER_RPC_PROXY_IF,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                               &FWPM_LAYER_KM_AUTHORIZATION,                  /// 93

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

                                              };
const GUID* const ppUserModeLayerKeyArray[] = {&FWPM_LAYER_IPSEC_KM_DEMUX_V4,                 ///  0
                                               &FWPM_LAYER_IPSEC_KM_DEMUX_V6,
                                               &FWPM_LAYER_IPSEC_V4,
                                               &FWPM_LAYER_IPSEC_V6,
                                               &FWPM_LAYER_IKEEXT_V4,
                                               &FWPM_LAYER_IKEEXT_V6,
                                               &FWPM_LAYER_RPC_UM,
                                               &FWPM_LAYER_RPC_EPMAP,
                                               &FWPM_LAYER_RPC_EP_ADD,
                                               &FWPM_LAYER_RPC_PROXY_CONN,
                                               &FWPM_LAYER_RPC_PROXY_IF,                      /// 10
#if(NTDDI_VERSION >= NTDDI_WIN7)

                                               &FWPM_LAYER_KM_AUTHORIZATION,

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

                                              };

const UINT32 TOTAL_LAYER_COUNT             = RTL_NUMBER_OF(ppLayerKeyArray);
const UINT32 TOTAL_USER_MODE_LAYER_COUNT   = RTL_NUMBER_OF(ppUserModeLayerKeyArray);
const UINT32 TOTAL_KERNEL_MODE_LAYER_COUNT = TOTAL_LAYER_COUNT - TOTAL_USER_MODE_LAYER_COUNT;

const GUID* const ppConditionInboundIPPacket[]            = {&FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE 
                                                            };
const GUID* const ppConditionOutboundIPPacket[]           = {&FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE
                                                            };
const GUID* const ppConditionIPForward[]                  = {&FWPM_CONDITION_IP_SOURCE_ADDRESS,
                                                             &FWPM_CONDITION_IP_DESTINATION_ADDRESS,
                                                             &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_IP_FORWARD_INTERFACE,
                                                             &FWPM_CONDITION_SOURCE_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_SOURCE_SUB_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_DESTINATION_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_DESTINATION_SUB_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_FLAGS,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_IP_PHYSICAL_ARRIVAL_INTERFACE,
                                                             &FWPM_CONDITION_ARRIVAL_INTERFACE_PROFILE_ID,
                                                             &FWPM_CONDITION_IP_PHYSICAL_NEXTHOP_INTERFACE,
                                                             &FWPM_CONDITION_NEXTHOP_INTERFACE_PROFILE_ID,

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

                                                            };
const GUID* const ppConditionInboundTransport[]           = {&FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_CURRENT_PROFILE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

                                                             &FWPM_CONDITION_IPSEC_SECURITY_REALM_ID,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

                                                            };
const GUID* const ppConditionOutboundTransport[]          = {&FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX
                                                             &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_CURRENT_PROFILE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

                                                             &FWPM_CONDITION_IPSEC_SECURITY_REALM_ID,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

                                                            };
const GUID* const ppConditionStream[]                     = {&FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,
                                                             &FWPM_CONDITION_DIRECTION,

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

                                                             &FWPM_CONDITION_FLAGS,

#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

                                                            };
const GUID* const ppConditionDatagramData[]               = {&FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX
                                                             &FWPM_CONDITION_DIRECTION,
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE
                                                            };
const GUID* const ppConditionInboundICMPError[]           = {&FWPM_CONDITION_EMBEDDED_PROTOCOL,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_EMBEDDED_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_EMBEDDED_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_EMBEDDED_LOCAL_PORT,
                                                             &FWPM_CONDITION_EMBEDDED_REMOTE_PORT,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_ICMP_TYPE,
                                                             &FWPM_CONDITION_ICMP_CODE,
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

                                                             &FWPM_CONDITION_IP_ARRIVAL_INTERFACE,
                                                             &FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE,
                                                             &FWPM_CONDITION_FLAGS,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_ARRIVAL_INTERFACE_PROFILE_ID,
                                                             &FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH,

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

                                                            };
const GUID* const ppConditionOutboundICMPError[]          = {&FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_ICMP_TYPE,
                                                             &FWPM_CONDITION_ICMP_CODE,
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

                                                             &FWPM_CONDITION_FLAGS,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_NEXTHOP_INTERFACE_PROFILE_ID,
                                                             &FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH,

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

                                                            };
const GUID* const ppConditionALEResourceAssignment[]      = {&FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_ALE_PROMISCUOUS_MODE,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_LOCAL_INTERFACE_PROFILE_ID,
                                                             &FWPM_CONDITION_ALE_SIO_FIREWALL_SYSTEM_PORT,     /// &FWPM_CONDITION_ALE_SIO_FIREWALL_SOCKET_PROPERTY

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_ALE_PACKAGE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
    
                                                             &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

                                                            };
const GUID* const ppConditionALEAuthListen[]              = {&FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_LOCAL_INTERFACE_PROFILE_ID,
                                                             &FWPM_CONDITION_ALE_SIO_FIREWALL_SYSTEM_PORT,     /// &FWPM_CONDITION_ALE_SIO_FIREWALL_SOCKET_PROPERTY

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_ALE_PACKAGE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

                                                             &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)

                                                            };
const GUID* const ppConditionALEAuthRecvAccept[]          = {&FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_ALE_REMOTE_USER_ID,
                                                             &FWPM_CONDITION_ALE_REMOTE_MACHINE_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_ALE_SIO_FIREWALL_SYSTEM_PORT,     /// &FWPM_CONDITION_ALE_SIO_FIREWALL_SOCKET_PROPERTY
                                                             &FWPM_CONDITION_ALE_NAP_CONTEXT,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

                                                             &FWPM_CONDITION_IP_ARRIVAL_INTERFACE,
                                                             &FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE,
                                                             &FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX,

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_NEXTHOP_SUB_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_IP_NEXTHOP_INTERFACE,
                                                             &FWPM_CONDITION_NEXTHOP_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_NEXTHOP_TUNNEL_TYPE,
                                                             &FWPM_CONDITION_NEXTHOP_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_ORIGINAL_PROFILE_ID,
                                                             &FWPM_CONDITION_CURRENT_PROFILE_ID,
                                                             &FWPM_CONDITION_REAUTHORIZE_REASON,
                                                             &FWPM_CONDITION_ORIGINAL_ICMP_TYPE,
                                                             &FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_ALE_PACKAGE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

                                                             &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

                                                            };
const GUID* const ppConditionALEAuthConnect[]             = {&FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,                /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_ALE_REMOTE_USER_ID,
                                                             &FWPM_CONDITION_ALE_REMOTE_MACHINE_ID,
                                                             &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_IP_ARRIVAL_INTERFACE,
                                                             &FWPM_CONDITION_ARRIVAL_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_ARRIVAL_TUNNEL_TYPE,
                                                             &FWPM_CONDITION_ARRIVAL_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_NEXTHOP_SUB_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_IP_NEXTHOP_INTERFACE,
                                                             &FWPM_CONDITION_NEXTHOP_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_NEXTHOP_TUNNEL_TYPE,
                                                             &FWPM_CONDITION_NEXTHOP_INTERFACE_INDEX,
                                                             &FWPM_CONDITION_ORIGINAL_PROFILE_ID,
                                                             &FWPM_CONDITION_CURRENT_PROFILE_ID,
                                                             &FWPM_CONDITION_REAUTHORIZE_REASON,
                                                             &FWPM_CONDITION_PEER_NAME,
                                                             &FWPM_CONDITION_ORIGINAL_ICMP_TYPE,
                                                             &FWPM_CONDITION_INTERFACE_QUARANTINE_EPOCH,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_ALE_ORIGINAL_APP_ID,
                                                             &FWPM_CONDITION_ALE_PACKAGE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

                                                             &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

                                                            };
const GUID* const ppConditionALEFlowEstablished[]         = {&FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_ALE_REMOTE_USER_ID,
                                                             &FWPM_CONDITION_ALE_REMOTE_MACHINE_ID,
                                                             &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_DIRECTION,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE

#if(NTDDI_VERSION >= NTDDI_VISTASP1)

                                                             &FWPM_CONDITION_FLAGS,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_ALE_ORIGINAL_APP_ID,
                                                             &FWPM_CONDITION_ALE_PACKAGE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

                                                             &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_VISTASP1)

                                                            };
const GUID* const ppConditionIPsecKMDemux[]               = {&FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_QM_MODE,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,
                                                             &FWPM_CONDITION_CURRENT_PROFILE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                                                           &FWPM_CONDITION_IPSEC_SECURITY_REALM_ID
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)


                                                            };
const GUID* const ppConditionIPsec[]                      = {&FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_CURRENT_PROFILE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                                                           &FWPM_CONDITION_IPSEC_SECURITY_REALM_ID
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)


                                                            };
const GUID* const ppConditionIKEExt[]                     = {&FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE

#if(NTDDI_VERSION >= NTDDI_WIN7)

                                                             &FWPM_CONDITION_CURRENT_PROFILE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                                                           &FWPM_CONDITION_IPSEC_SECURITY_REALM_ID
#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

#endif /// (NTDDI_VERSION >= NTDDI_WIN7)


                                                            };
const GUID* const ppConditionRPCUM[]                      = {&FWPM_CONDITION_REMOTE_USER_TOKEN,
                                                             &FWPM_CONDITION_RPC_IF_UUID,
                                                             &FWPM_CONDITION_RPC_IF_VERSION,
                                                             &FWPM_CONDITION_RPC_IF_FLAG,
                                                             &FWPM_CONDITION_DCOM_APP_ID,
                                                             &FWPM_CONDITION_IMAGE_NAME,
                                                             &FWPM_CONDITION_RPC_PROTOCOL,
                                                             &FWPM_CONDITION_RPC_AUTH_TYPE,
                                                             &FWPM_CONDITION_RPC_AUTH_LEVEL,
                                                             &FWPM_CONDITION_SEC_ENCRYPT_ALGORITHM,
                                                             &FWPM_CONDITION_SEC_KEY_SIZE,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_V4,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_V6,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_CODEE
                                                             &FWPM_CONDITION_PIPE,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS_V4,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS_V6,
                                                            };
const GUID* const ppConditionRPCEPMap[]                   = {&FWPM_CONDITION_REMOTE_USER_TOKEN,
                                                             &FWPM_CONDITION_RPC_IF_UUID,
                                                             &FWPM_CONDITION_RPC_IF_VERSION,
                                                             &FWPM_CONDITION_RPC_PROTOCOL,
                                                             &FWPM_CONDITION_RPC_AUTH_TYPE,
                                                             &FWPM_CONDITION_RPC_AUTH_LEVEL,
                                                             &FWPM_CONDITION_SEC_ENCRYPT_ALGORITHM,
                                                             &FWPM_CONDITION_SEC_KEY_SIZE,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_V4,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_V6,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_PIPE,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS_V4,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS_V6,
                                                            };
const GUID* const ppConditionRPCEPAdd[]                   = {&FWPM_CONDITION_PROCESS_WITH_RPC_IF_UUID,
                                                             &FWPM_CONDITION_RPC_PROTOCOL,
                                                             &FWPM_CONDITION_RPC_EP_VALUE,
                                                             &FWPM_CONDITION_RPC_EP_FLAGS,
                                                            };
const GUID* const ppConditionRPCProxyConn[]               = {&FWPM_CONDITION_CLIENT_TOKEN,
                                                             &FWPM_CONDITION_RPC_SERVER_NAME,
                                                             &FWPM_CONDITION_RPC_SERVER_PORT,
                                                             &FWPM_CONDITION_RPC_PROXY_AUTH_TYPE,
                                                             &FWPM_CONDITION_CLIENT_CERT_KEY_LENGTH,
                                                             &FWPM_CONDITION_CLIENT_CERT_OID,
                                                            };
const GUID* const ppConditionRPCProxyIf[]                 = {&FWPM_CONDITION_CLIENT_TOKEN,
                                                             &FWPM_CONDITION_RPC_IF_UUID,
                                                             &FWPM_CONDITION_RPC_IF_VERSION,
                                                             &FWPM_CONDITION_RPC_SERVER_NAME,
                                                             &FWPM_CONDITION_RPC_SERVER_PORT,
                                                             &FWPM_CONDITION_RPC_PROXY_AUTH_TYPE,
                                                             &FWPM_CONDITION_CLIENT_CERT_KEY_LENGTH,
                                                             &FWPM_CONDITION_CLIENT_CERT_OID,
                                                            };

#if(NTDDI_VERSION >= NTDDI_WIN7)

const GUID* const ppConditionNameResolutionCache[]        = {&FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_PEER_NAME,
                                                            };
const GUID* const ppConditionALEResourceRelease[]         = {&FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_FLAGS,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_ALE_PACKAGE_ID,

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

                                                            };
const GUID* const ppConditionALEEndpointClosure[]         = {&FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_FLAGS,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_ALE_PACKAGE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

                                                             &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

                                                            };
const GUID* const ppConditionALEConnectRedirect[]         = {&FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_DESTINATION_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_FLAGS,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_ALE_ORIGINAL_APP_ID,
                                                             &FWPM_CONDITION_ALE_PACKAGE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

                                                             &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

                                                            };
const GUID* const ppConditionALEBindRedirect[]            = {&FWPM_CONDITION_ALE_APP_ID,
                                                             &FWPM_CONDITION_ALE_USER_ID,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,                    /// &FWPM_CONDITION_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_FLAGS,

#if(NTDDI_VERSION >= NTDDI_WIN8)

                                                             &FWPM_CONDITION_ALE_PACKAGE_ID,

#if(NTDDI_VERSION >= NTDDI_WINTHRESHOLD)

                                                             &FWPM_CONDITION_ALE_SECURITY_ATTRIBUTE_FQBN_VALUE,

#endif /// (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)

                                                            };
const GUID* const ppConditionStreamPacket[]               = {&FWPM_CONDITION_IP_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_IP_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_IP_LOCAL_PORT,
                                                             &FWPM_CONDITION_IP_REMOTE_PORT,                   /// &FWPM_CONDITION_ICMP_CODE
                                                             &FWPM_CONDITION_IP_LOCAL_INTERFACE,               /// &FWPM_CONDITION_INTERFACE
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_SUB_INTERFACE_INDEX,              /// &FWPM_CONDITION_ARRIVAL_SUB_INTERFACE_INDEX
                                                             &FWPM_CONDITION_DIRECTION,
                                                             &FWPM_CONDITION_FLAGS,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_TUNNEL_TYPE,                      /// &FWPM_CONDITION_LOCAL_TUNNEL_TYPE
                                                            };
const GUID* const ppConditionKMAuthorization[]            = {&FWPM_CONDITION_REMOTE_ID,
                                                             &FWPM_CONDITION_AUTHENTICATION_TYPE,
                                                             &FWPM_CONDITION_KM_TYPE,
                                                             &FWPM_CONDITION_DIRECTION,
                                                             &FWPM_CONDITION_KM_MODE,
                                                             &FWPM_CONDITION_IPSEC_POLICY_KEY,
                                                             &FWPM_CONDITION_KM_AUTH_NAP_CONTEXT,
                                                            };

#if(NTDDI_VERSION >= NTDDI_WIN8)

const GUID* const ppConditionInboundMACFrameEthernet[]    = {&FWPM_CONDITION_INTERFACE_MAC_ADDRESS,
                                                             &FWPM_CONDITION_MAC_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_MAC_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_MAC_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_MAC_REMOTE_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_ETHER_TYPE,
                                                             &FWPM_CONDITION_VLAN_ID,
                                                             &FWPM_CONDITION_INTERFACE,
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_NDIS_PORT,
                                                             &FWPM_CONDITION_L2_FLAGS,
                                                            };
const GUID* const ppConditionOutboundMACFrameEthernet[]   = {&FWPM_CONDITION_INTERFACE_MAC_ADDRESS,
                                                             &FWPM_CONDITION_MAC_LOCAL_ADDRESS,
                                                             &FWPM_CONDITION_MAC_REMOTE_ADDRESS,
                                                             &FWPM_CONDITION_MAC_LOCAL_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_MAC_REMOTE_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_ETHER_TYPE,
                                                             &FWPM_CONDITION_VLAN_ID,
                                                             &FWPM_CONDITION_INTERFACE,
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_NDIS_PORT,
                                                             &FWPM_CONDITION_L2_FLAGS,
                                                            };
const GUID* const ppConditionInboundMACFrameNative[]      = {&FWPM_CONDITION_NDIS_MEDIA_TYPE,
                                                             &FWPM_CONDITION_NDIS_PHYSICAL_MEDIA_TYPE,
                                                             &FWPM_CONDITION_INTERFACE,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_NDIS_PORT,
                                                             &FWPM_CONDITION_L2_FLAGS,
                                                            };
const GUID* const ppConditionOutboundMACFrameNative[]     = {&FWPM_CONDITION_NDIS_MEDIA_TYPE,
                                                             &FWPM_CONDITION_NDIS_PHYSICAL_MEDIA_TYPE,
                                                             &FWPM_CONDITION_INTERFACE,
                                                             &FWPM_CONDITION_INTERFACE_TYPE,                   /// &FWPM_CONDITION_LOCAL_INTERFACE_TYPE
                                                             &FWPM_CONDITION_INTERFACE_INDEX,                  /// &FWPM_CONDITION_LOCAL_INTERFACE_INDEX 
                                                             &FWPM_CONDITION_NDIS_PORT,
                                                             &FWPM_CONDITION_L2_FLAGS,
                                                            };
const GUID* const ppConditionIngressVSwitchEthernet[]     = {&FWPM_CONDITION_MAC_SOURCE_ADDRESS,
                                                             &FWPM_CONDITION_MAC_SOURCE_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_MAC_DESTINATION_ADDRESS,
                                                             &FWPM_CONDITION_MAC_DESTINATION_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_ETHER_TYPE,
                                                             &FWPM_CONDITION_VLAN_ID,
                                                             &FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID,
                                                             &FWPM_CONDITION_VSWITCH_ID,
                                                             &FWPM_CONDITION_VSWITCH_NETWORK_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_VM_ID,
                                                             &FWPM_CONDITION_L2_FLAGS,
                                                            };
const GUID* const ppConditionEgressVSwitchEthernet[]      = {&FWPM_CONDITION_MAC_SOURCE_ADDRESS,
                                                             &FWPM_CONDITION_MAC_SOURCE_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_MAC_DESTINATION_ADDRESS,
                                                             &FWPM_CONDITION_MAC_DESTINATION_ADDRESS_TYPE,
                                                             &FWPM_CONDITION_ETHER_TYPE,
                                                             &FWPM_CONDITION_VLAN_ID,
                                                             &FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID,
                                                             &FWPM_CONDITION_VSWITCH_ID,
                                                             &FWPM_CONDITION_VSWITCH_NETWORK_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_VM_ID,
                                                             &FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_ID,
                                                             &FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_DESTINATION_VM_ID,
                                                             &FWPM_CONDITION_L2_FLAGS,
                                                            };
const GUID* const ppConditionIngressVSwitchTransport[]    = {&FWPM_CONDITION_IP_SOURCE_ADDRESS,
                                                             &FWPM_CONDITION_IP_DESTINATION_ADDRESS,
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_SOURCE_PORT,                   /// &FWPM_CONDITION_VSWITCH_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_DESTINATION_PORT,              /// &FWPM_CONDITION_VSWITCH_ICMP_CODE
                                                             &FWPM_CONDITION_VLAN_ID,
                                                             &FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID,
                                                             &FWPM_CONDITION_VSWITCH_ID,
                                                             &FWPM_CONDITION_VSWITCH_NETWORK_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_VM_ID,
                                                             &FWPM_CONDITION_L2_FLAGS
                                                            };
const GUID* const ppConditionEgressVSwitchTransport[]     = {&FWPM_CONDITION_IP_SOURCE_ADDRESS,
                                                             &FWPM_CONDITION_IP_DESTINATION_ADDRESS,
                                                             &FWPM_CONDITION_IP_PROTOCOL,
                                                             &FWPM_CONDITION_IP_SOURCE_PORT,                   /// &FWPM_CONDITION_VSWITCH_ICMP_TYPE
                                                             &FWPM_CONDITION_IP_DESTINATION_PORT,              /// &FWPM_CONDITION_VSWITCH_ICMP_CODE
                                                             &FWPM_CONDITION_VLAN_ID,
                                                             &FWPM_CONDITION_VSWITCH_TENANT_NETWORK_ID,
                                                             &FWPM_CONDITION_VSWITCH_ID,
                                                             &FWPM_CONDITION_VSWITCH_NETWORK_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_ID,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_SOURCE_VM_ID,
                                                             &FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_ID,
                                                             &FWPM_CONDITION_VSWITCH_DESTINATION_INTERFACE_TYPE,
                                                             &FWPM_CONDITION_VSWITCH_DESTINATION_VM_ID,
                                                             &FWPM_CONDITION_L2_FLAGS
                                                            };

#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTA)

#if(NTDDI_VERSION >= NTDDI_VISTA)

const UINT16 INBOUND_IPPACKET_CONDITIONS_COUNT               = RTL_NUMBER_OF(ppConditionInboundIPPacket);
const UINT16 OUTBOUND_IPPACKET_CONDITIONS_COUNT              = RTL_NUMBER_OF(ppConditionOutboundIPPacket);
const UINT16 IPFORWARD_CONDITIONS_COUNT                      = RTL_NUMBER_OF(ppConditionIPForward);
const UINT16 INBOUND_TRANSPORT_CONDITIONS_COUNT              = RTL_NUMBER_OF(ppConditionInboundTransport);
const UINT16 OUTBOUND_TRANSPORT_CONDITIONS_COUNT             = RTL_NUMBER_OF(ppConditionOutboundTransport);
const UINT16 STREAM_CONDITIONS_COUNT                         = RTL_NUMBER_OF(ppConditionStream);
const UINT16 DATAGRAM_DATA_CONDITIONS_COUNT                  = RTL_NUMBER_OF(ppConditionDatagramData);
const UINT16 INBOUND_ICMP_ERROR_CONDITIONS_COUNT             = RTL_NUMBER_OF(ppConditionInboundICMPError);
const UINT16 OUTBOUND_ICMP_ERROR_CONDITIONS_COUNT            = RTL_NUMBER_OF(ppConditionOutboundICMPError);
const UINT16 ALE_RESOURCE_ASSIGNMENT_CONDITIONS_COUNT        = RTL_NUMBER_OF(ppConditionALEResourceAssignment);
const UINT16 ALE_AUTH_LISTEN_CONDITIONS_COUNT                = RTL_NUMBER_OF(ppConditionALEAuthListen);
const UINT16 ALE_AUTH_RECV_ACCEPT_CONDITIONS_COUNT           = RTL_NUMBER_OF(ppConditionALEAuthRecvAccept);
const UINT16 ALE_AUTH_CONNECT_CONDITIONS_COUNT               = RTL_NUMBER_OF(ppConditionALEAuthConnect);
const UINT16 ALE_FLOW_ESTABLISHED_CONDITIONS_COUNT           = RTL_NUMBER_OF(ppConditionALEFlowEstablished);
const UINT16 IPSEC_KM_DEMUX_CONDITIONS_COUNT                 = RTL_NUMBER_OF(ppConditionIPsecKMDemux);
const UINT16 IPSEC_CONDITIONS_COUNT                          = RTL_NUMBER_OF(ppConditionIPsec);
const UINT16 IKEEXT_CONDITIONS_COUNT                         = RTL_NUMBER_OF(ppConditionIKEExt);
const UINT16 RPC_UM_CONDITIONS_COUNT                         = RTL_NUMBER_OF(ppConditionRPCUM);
const UINT16 RPC_EPMAP_CONDITIONS_COUNT                      = RTL_NUMBER_OF(ppConditionRPCEPMap);
const UINT16 RPC_EP_ADD_CONDITIONS_COUNT                     = RTL_NUMBER_OF(ppConditionRPCEPAdd);
const UINT16 RPC_PROXY_CONN_CONDITIONS_COUNT                 = RTL_NUMBER_OF(ppConditionRPCProxyConn);
const UINT16 RPC_PROXY_IF_CONDITIONS_COUNT                   = RTL_NUMBER_OF(ppConditionRPCProxyIf);

#if(NTDDI_VERSION >= NTDDI_WIN7)

const UINT16 NAME_RESOLUTION_CACHE_CONDITIONS_COUNT          = RTL_NUMBER_OF(ppConditionNameResolutionCache);
const UINT16 ALE_RESOURCE_RELEASE_CONDITIONS_COUNT           = RTL_NUMBER_OF(ppConditionALEResourceRelease);
const UINT16 ALE_ENDPOINT_CLOSURE_CONDITIONS_COUNT           = RTL_NUMBER_OF(ppConditionALEEndpointClosure);
const UINT16 ALE_CONNECT_REDIRECT_CONDITIONS_COUNT           = RTL_NUMBER_OF(ppConditionALEConnectRedirect);
const UINT16 ALE_BIND_REDIRECT_CONDITIONS_COUNT              = RTL_NUMBER_OF(ppConditionALEBindRedirect);
const UINT16 STREAM_PACKET_CONDITIONS_COUNT                  = RTL_NUMBER_OF(ppConditionStreamPacket);
const UINT16 KM_AUTHORIZATION_CONDITIONS_COUNT               = RTL_NUMBER_OF(ppConditionKMAuthorization);

#if(NTDDI_VERSION >= NTDDI_WIN8)

const UINT16 INBOUND_MAC_FRAME_ETHERNET_CONDITIONS_COUNT     = RTL_NUMBER_OF(ppConditionInboundMACFrameEthernet);
const UINT16 OUTBOUND_MAC_FRAME_ETHERNET_CONDITIONS_COUNT    = RTL_NUMBER_OF(ppConditionOutboundMACFrameEthernet);
const UINT16 INBOUND_MAC_FRAME_NATIVE_CONDITIONS_COUNT       = RTL_NUMBER_OF(ppConditionInboundMACFrameNative);
const UINT16 OUTBOUND_MAC_FRAME_NATIVE_CONDITIONS_COUNT      = RTL_NUMBER_OF(ppConditionOutboundMACFrameNative);
const UINT16 INGRESS_VSWITCH_ETHERNET_CONDITIONS_COUNT       = RTL_NUMBER_OF(ppConditionIngressVSwitchEthernet);
const UINT16 EGRESS_VSWITCH_ETHERNET_CONDITIONS_COUNT        = RTL_NUMBER_OF(ppConditionEgressVSwitchEthernet);
const UINT16 INGRESS_VSWITCH_TRANSPORT_CONDITIONS_COUNT      = RTL_NUMBER_OF(ppConditionIngressVSwitchTransport);
const UINT16 EGRESS_VSWITCH_TRANSPORT_CONDITIONS_COUNT       = RTL_NUMBER_OF(ppConditionEgressVSwitchTransport);

#if(NTDDI_VERSION >= NTDDI_WINBLUE)

const UINT16 INBOUND_TRANSPORT_FAST_CONDITIONS_COUNT         = 0;
const UINT16 OUTBOUND_TRANSPORT_FAST_CONDITIONS_COUNT        = 0;
const UINT16 INBOUND_MAC_FRAME_NATIVE_FAST_CONDITIONS_COUNT  = 0;
const UINT16 OUTBOUND_MAC_FRAME_NATIVE_FAST_CONDITIONS_COUNT = 0;

#endif /// (NTDDI_VERSION >= NTDDI_WINBLUE)
#endif /// (NTDDI_VERSION >= NTDDI_WIN8)
#endif /// (NTDDI_VERSION >= NTDDI_WIN7)
#endif /// (NTDDI_VERSION >= NTDDI_VISTA)

#endif /// WFP_SAMPLER_WFP_ARRAYS_H
