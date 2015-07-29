////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ProviderContexts.h
//
//   Abstract:
//      This module contains global definitions of FWPM_PROVIDER_CONTEXT Data for the WFPSampler 
//         project
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add ADVANCED_PACKET_INJECTION, FLOW_ASSOCIATION, and
//                                              PEND_ENDPOINT_CLOSURE scenarios.   Enhancements for
//                                              BASIC_PACKET_EXAMINATION, BASIC_PACKET_MODIFICATION,
//                                              and PROXY providerContexts.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WFP_SAMPLER_PROVIDER_CONTEXT_H
#define WFP_SAMPLER_PROVIDER_CONTEXT_H

/// ProviderContext ProxyData Flags
#define PCPDF_PROXY_LOCAL_ADDRESS  0x01
#define PCPDF_PROXY_LOCAL_PORT     0x02
#define PCPDF_PROXY_REMOTE_ADDRESS 0x04
#define PCPDF_PROXY_REMOTE_PORT    0x08

/// ProviderContext PacketExamination Flags
#define PCPEF_EXAMINE_UNDER_LOCK               0x00000001
#define PCPEF_EXAMINE_INCOMING_VALUES          0x00000010
#define PCPEF_EXAMINE_INCOMING_METADATA_VALUES 0x00000100
#define PCPEF_EXAMINE_LAYER_DATA               0x00001000
#define PCPEF_EXAMINE_CLASSIFY_CONTEXT         0x00010000
#define PCPEF_EXAMINE_FILTER                   0x00100000
#define PCPEF_EXAMINE_FLOW_CONTEXT             0x01000000
#define PCPEF_EXAMINE_CLASSIFY_OUT             0x10000000

///ProviderContext PacketModificationData Flags
#define PCPMDF_MODIFY_MAC_HEADER                     0x10
#define PCPMDF_MODIFY_MAC_HEADER_SOURCE_ADDRESS      0x01
#define PCPMDF_MODIFY_MAC_HEADER_DESTINATION_ADDRESS 0x02

#define PCPMDF_MODIFY_IP_HEADER                     0x20
#define PCPMDF_MODIFY_IP_HEADER_SOURCE_ADDRESS      0x01
#define PCPMDF_MODIFY_IP_HEADER_DESTINATION_ADDRESS 0x02
#define PCPMDF_MODIFY_IP_HEADER_INTERFACE_INDEX     0x04

#define PCPMDF_MODIFY_TRANSPORT_HEADER                  0x40
#define PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT      0x01
#define PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT 0x02

#define PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_TYPE PCPMDF_MODIFY_TRANSPORT_HEADER_SOURCE_PORT
#define PCPMDF_MODIFY_TRANSPORT_HEADER_ICMP_CODE PCPMDF_MODIFY_TRANSPORT_HEADER_DESTINATION_PORT

#define PCFA_MAX_COUNT 7

typedef struct MAC_DATA_
{
   UINT32 flags;
   BYTE   pReserved[4];
   BYTE   pSourceMACAddress[8];
   BYTE   pDestinationMACAddress[8];
}MAC_DATA;

typedef struct IP_DATA_
{
   UINT32 flags;
   BYTE   pReserved[3];
   UINT8  ipVersion;
   union
   {
      BYTE pIPv4[4];                       /// Network Byte Order
      BYTE pIPv6[16];
      BYTE pBytes[16];
   }sourceAddress;
   union
   {
      BYTE pIPv4[4];                       /// Network Byte Order
      BYTE pIPv6[16];
      BYTE pBytes[16];
   }destinationAddress;
   UINT32 interfaceIndex;
}IP_DATA;

typedef struct TRANSPORT_DATA_
{
   UINT32 flags;
   BYTE   pReserved[3];
   UINT8  protocol;
   union
   {
      UINT8  icmpType;
      UINT16 sourcePort;                   /// Network Byte Order
   };
   union
   {
      UINT8  icmpCode;
      UINT16 destinationPort;              /// Network Byte Order
   };
   BYTE pPadding[4];
} TRANSPORT_DATA;

typedef struct PC_ADVANCED_PACKET_INJECTION_DATA_
{
   BOOLEAN performInline;   /// Inline vs. Out of Band
   BOOLEAN useWorkItems;    /// Work Items vs. Deferred Procedure Calls
   BOOLEAN useThreadedDPC;  /// Threaded DPCs vs Deferred Procedure Calls
   UINT32  additionalBytes;
   BYTE    pReserved[1];
} PC_ADVANCED_PACKET_INJECTION_DATA, *PPC_ADVANCED_PACKET_INJECTION_DATA;

typedef struct PC_BASIC_ACTION_DATA_
{
   UINT8 percentBlock;
   UINT8 percentPermit;
   UINT8 percentContinue;
   BYTE  pReserved[5];
} PC_BASIC_ACTION_DATA, *PPC_BASIC_ACTION_DATA;

typedef struct PC_BASIC_PACKET_INJECTION_DATA_
{
   BOOLEAN performInline;  /// Inline vs. Out of Band
   BOOLEAN useWorkItems;   /// Work Items vs. Deferred Procedure Calls
   BOOLEAN useThreadedDPC; /// Threaded DPCs vs Deferred Procedure Calls
   BYTE    pReserved[5];
} PC_BASIC_PACKET_INJECTION_DATA, *PPC_BASIC_PACKET_INJECTION_DATA;

typedef struct PC_BASIC_PACKET_MODIFICATION_DATA_
{
   UINT32         flags;
   BOOLEAN        performInline;         /// Inline vs. Out of Band
   BOOLEAN        useWorkItems;          /// Work Items vs. Deferred Procedure Calls
   BOOLEAN        useThreadedDPC;        /// Threaded DPCs vs Deferred Procedure Calls
   BYTE           pReserved[1];
   MAC_DATA       macData;
   IP_DATA        ipData;
   TRANSPORT_DATA transportData;
   MAC_DATA       originalMACData;
   IP_DATA        originalIPData;
   TRANSPORT_DATA originalTransportData; /// Used to uniquely identify traffic at the Network layers
}PC_BASIC_PACKET_MODIFICATION_DATA;

typedef struct PC_BASIC_STREAM_INJECTION_DATA_
{
   BOOLEAN performInline;  /// Inline vs. Out of Band
   BOOLEAN useWorkItems;   /// Work Items vs. Deferred Procedure Calls
   BOOLEAN useThreadedDPC; /// Threaded DPCs vs Deferred Procedure Calls
   BYTE    pReserved[5];
} PC_BASIC_STREAM_INJECTION_DATA, *PPC_BASIC_STREAM_INJECTION_DATA;

typedef struct PC_FLOW_ASSOCIATION_DATA_
{
   UINT16 itemCount;
   UINT16 pLayerIDs[PCFA_MAX_COUNT];
   UINT32 pCalloutIDs[PCFA_MAX_COUNT];
}PC_FLOW_ASSOCIATION_DATA,*PPC_FLOW_ASSOCIATION_DATA;

typedef struct PC_PEND_AUTHORIZATION_DATA_
{
   UINT32  flags;
   BOOLEAN useWorkItems;                   /// Work Items vs. Deferred Procedure Calls
   BOOLEAN useThreadedDPC;                 /// Threaded DPCs vs Deferred Procedure Calls
   BYTE    pReserved[2];
   UINT32  finalAction;
   UINT32  delay;
}PC_PEND_AUTHORIZATION_DATA, *PPC_PEND_AUTHORIZATION_DATA;

typedef struct PC_PEND_ENDPOINT_CLOSURE_DATA_
{
   UINT32  flags;
   BOOLEAN useWorkItems;                   /// Work Items vs. Deferred Procedure Calls
   BOOLEAN useThreadedDPC;                 /// Threaded DPCs vs Deferred Procedure Calls
   BYTE    pReserved[2];
   UINT32  delay;
}PC_PEND_ENDPOINT_CLOSURE_DATA, *PPC_PEND_ENDPOINT_CLOSURE_DATA;

typedef struct PC_PROXY_DATA_
{
   UINT32  flags;
   BOOLEAN performInline;                  /// Inline vs. Out of Band
   BOOLEAN useWorkItems;                   /// Work Items vs. Deferred Procedure Calls
   BOOLEAN useThreadedDPC;                 /// Threaded DPCs vs Deferred Procedure Calls
   BOOLEAN proxyToRemoteService;           /// Local vs. Remote Service
   BYTE    pReserved[6];
   UINT8   ipVersion;
   UINT8   ipProtocol;
   union
   {
      BYTE pIPv4[4];                       /// Network Byte Order
      BYTE pIPv6[16];
      BYTE pBytes[16];
   }proxyLocalAddress;
   union
   {
      BYTE pIPv4[4];                       /// Network Byte Order
      BYTE pIPv6[16];
      BYTE pBytes[16];
   }proxyRemoteAddress;
   union
   {
      BYTE pIPv4[4];                       /// Network Byte Order
      BYTE pIPv6[16];
      BYTE pBytes[16];
   }originalLocalAddress;
   union
   {
      BYTE pIPv4[4];                       /// Network Byte Order
      BYTE pIPv6[16];
      BYTE pBytes[16];
   }originalRemoteAddress;
   UINT32  localScopeId;
   UINT32  remoteScopeId;
   UINT16  proxyLocalPort;                 /// Network Byte Order
   UINT16  proxyRemotePort;                /// Network Byte Order
   UINT16  originalLocalPort;              /// Network Byte Order
   UINT16  originalRemotePort;             /// Network Byte Order
   UINT32  targetProcessID;
   UINT64  tcpPortReservationToken;
   UINT64  udpPortReservationToken;
} PC_PROXY_DATA, *PPC_PROXY_DATA;

#endif /// WFP_SAMPLER_PROVIDER_CONTEXT_H
