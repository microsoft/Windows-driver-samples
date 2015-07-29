////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_Headers.h
//
//   Abstract:
//      This module contains definitions and prototypes of kernel helper functions that assist with  
//         MAC, IP, and Transport header operations.
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Enhance annotations and add
//                                              KrnlHlprIPHeaderGetDestinationAddressField, 
//                                              KrnlHlprIPHeaderGetSourceAddressField,
//                                              KrnlHlprIPHeaderGetVersionField,
//                                              KrnlHlprTransportHeaderGetSourcePortField,
//                                              KrnlHlprTransportHeaderGetDestinationPortField
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef HELPERFUNCTIONS_HEADERS_H
#define HELPERFUNCTIONS_HEADERS_H

#pragma warning(push)
#pragma warning(disable: 4201) /// NAMELESS_STRUCT_UNION

#define ETHERNET_ADDRESS_SIZE 6

#define IPV4_ADDRESS_SIZE  4
#define IPV6_ADDRESS_SIZE 16

#define IPV4 4
#define IPV6 6

#define IPV4_HEADER_MIN_SIZE 20
#define IPV6_HEADER_MIN_SIZE 40
#define ICMP_HEADER_MIN_SIZE  8
#define TCP_HEADER_MIN_SIZE  20
#define UDP_HEADER_MIN_SIZE   8

#define ICMPV4  1
#define TCP     6
#define UDP    17
#define ICMPV6 58

static const UINT8 IPV4_LOOPBACK_ADDRESS[] = {0x01,
                                              0x00,
                                              0x00,
                                              0x7F}; /// 127.0.0.1 ( in network byte order)
static const UINT8 IPV6_LOOPBACK_ADDRESS[] = {0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x00,
                                              0x01}; /// ::1



/*
    RFC 894 - A Standard for the Transmission of     <br>
              IP Datagrams over Ethernet Networks    <br>
                                                     <br>
    0                   1                   2        <br>
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3  <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                                               | <br>
   +            Destination MAC Address            + <br>
   |                                               | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                                               | <br>
   +               Source MAC Address              + <br>
   |                                               | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |              Type             |    Data...    | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
                                                     <br>
   RFC_REF: http://www.faqs.org/rfcs/rfc894.html     <br>
*/
typedef struct _ETHERNET_II_HEADER_
{
   BYTE   pDestinationAddress[6];
   BYTE   pSourceAddress[6];
   UINT16 type;
}ETHERNET_II_HEADER;

/*
    RFC 1042 - A Standard for the Transmission of    <br>
               IP Datagrams over IEEE 802 Networks   <br>
                                                     <br>
    0                   1                   2        <br>
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3  <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                                               | <br>
   +            Destination MAC Address            + <br>
   |                                               | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                                               | <br>
   +               Source MAC Address              + <br>
   |                                               | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |            Length             |      DSAP     | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |      SSAP     | Control Byte  |    OUI ...    > <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   <           OUI (cont.)         |    Type ...   > <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   < Type (cont.)  |            Data ...           | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
                                                     <br>
   RFC_REF: http://www.faqs.org/rfcs/rfc1042.html    <br>
*/
typedef struct _ETHERNET_SNAP_HEADER_
{
   BYTE   pDestinationAddress[6];
   BYTE   pSourceAddress[6];
   UINT16 length;
   UINT8  destinationSAP; /// Destination Subnetwork Access Protocol
   UINT8  sourceSAP;      /// Source  Subnetwork Access Protocol
   UINT8  controlByte;
   UINT8  pOUI[3];        /// Organizationally Unique Identifier
   UINT16 type;
}ETHERNET_SNAP_HEADER;

/*
                     RFC 791 - Internet Protocol                     <br>
                                                                     <br>
    0                   1                   2                   3    <br>
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1  <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |Version|  IHL  |Type of Service|         Total Length          | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |        Identification         |Flags|     Fragment Offset     | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |  Time to Live |     Protocol  |        Header Checksum        | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                        Source Address                         | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                      Destination Address                      | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                    Options                    |    Padding    | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
                                                                     <br>
   RFC_REF: http://www.faqs.org/rfcs/rfc791.html                     <br>
*/
typedef struct _IP_HEADER_V4_
{
   union
   {
      UINT8 versionAndHeaderLength;
      struct
      {
         UINT8 headerLength : 4;
         UINT8 version : 4;
      };
   };
   union
   {
      UINT8  typeOfService;
      UINT8  differentiatedServicesCodePoint;
      struct
      {
         UINT8 explicitCongestionNotification : 2;
         UINT8 typeOfService : 6;
      };
   };
   UINT16 totalLength;
   UINT16 identification;
   union
   {
      UINT16 flagsAndFragmentOffset;
      struct
      {
         UINT16 fragmentOffset : 13;
         UINT16 flags : 3;
      };
   };
   UINT8  timeToLive;
   UINT8  protocol;
   UINT16 checksum;
   BYTE   pSourceAddress[sizeof(UINT32)];
   BYTE   pDestinationAddress[sizeof(UINT32)];
}IP_HEADER_V4, *PIP_HEADER_V4;

/*
      RFC 2460 - Internet Protocol, Version 6 (IPv6) Specification   <br>
                                                                     <br>
    0                   1                   2                   3    <br>
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1  <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |Version| Traffic Class |              Flow Label               | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |        Payload Length         |  Next Header  |   Hop Limit   | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                                                               | <br>
   +                                                               + <br>
   |                                                               | <br>
   +                        Source Address                         + <br>
   |                                                               | <br>
   +                                                               + <br>
   |                                                               | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                                                               | <br>
   +                                                               + <br>
   |                                                               | <br>
   +                      Destination Address                      + <br>
   |                                                               | <br>
   +                                                               + <br>
   |                                                               | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
                                                                     <br>
   RFC_REF: http://www.faqs.org/rfcs/rfc2460.html                    <br>
*/
typedef struct _IP_HEADER_V6_
{
   union
   {
      UINT8 pVersionTrafficClassAndFlowLabel[4];
      struct
      {
       UINT8 r1 : 4;
       UINT8 value : 4;
       UINT8 r2;
       UINT8 r3;
       UINT8 r4;
      }version;
   };
   UINT16 payloadLength;
   UINT8  nextHeader;
   UINT8  hopLimit;
   BYTE   pSourceAddress[16];
   BYTE   pDestinationAddress[16];
}IP_HEADER_V6, *PIP_HEADER_V6;

/*
             RFC 792 - Internet Control Message Protocol             <br>
                                                                     <br>
    0                   1                   2                   3    <br>
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1  <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |     Type      |     Code      |           Checksum            | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |              Variable (Dependent on Type / Code)              | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
*/
typedef struct _ICMP_HEADER_V4_
{
   UINT8  type;
   UINT8  code;
   UINT16 checksum;
/*
   union
   {
      ECHO_MESSAGE                    echo;
      DESTINATION_UNREACHABLE_MESSAGE destinationUnreachable;
      SOURCE_QUENCH_MESSAGE           sourceQuench;
      REDIRECT_MESSAGE                redirect;
      TIME_EXCEEDED_MESSAGE           timeExceeded;
      PARAMETER_PROBLEM_MESSAGE       parameterProblem;
      TIMESTAMP_MESSAGE               timestamp;
      INFORMATION_MESSAGE             information;
   };
*/
}ICMP_HEADER_V4, *PICMP_HEADER_V4;

/*
        RFC 2463 - Internet Control Message Protocol for IPv6        <br>
                                                                     <br>
    0                   1                   2                   3    <br>
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1  <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |     Type      |     Code      |           Checksum            | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |              Variable (Dependent on Type / Code)              | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
*/
typedef struct _ICMP_HEADER_V6_
{
   UINT8  type;
   UINT8  code;
   UINT16 checksum;
/*   union
   {
      ECHO_MESSAGE                    echo;
      DESTINATION_UNREACHABLE_MESSAGE destinationUnreachable;
      SOURCE_QUENCH_MESSAGE           sourceQuench;
      REDIRECT_MESSAGE                redirect;
      TIME_EXCEEDED_MESSAGE           timeExceeded;
      PARAMETER_PROBLEM_MESSAGE       parameterProblem;
      TIMESTAMP_MESSAGE               timestamp;
      INFORMATION_MESSAGE             information;
   };*/
}ICMP_HEADER_V6, *PICMP_HEADER_V6;

/*
               RFC 793 - Transmission Control Protocol               <br>
                                                                     <br>
    0                   1                   2                   3    <br>
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1  <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |          Source Port          |       Destination Port        | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                        Sequence Number                        | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                     Acknowledgment Number                     | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |Offset |Rsvd |N|C|E|U|A|P|R|S|F|            Window             | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |           Checksum            |        Urgent Pointer         | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |                    Options                    |    Padding    | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
                                                                     <br>
   RFC_REF: http://www.faqs.org/rfcs/rfc793.html                     <br>
*/
typedef struct _TCP_HEADER_
{
   UINT16 sourcePort;
   UINT16 destinationPort;
   UINT32 sequenceNumber;
   UINT32 acknowledgementNumber;
   union
   {
      UINT8 dataOffsetReservedAndNS;
      struct
      {
         UINT8 nonceSum : 1;
         UINT8 reserved : 3;
         UINT8 dataOffset : 4;
      }dORNS;
   };
   union
   {
      UINT8 controlBits;
      struct
      {
         UINT8 FIN : 1;
         UINT8 SYN : 1;
         UINT8 RST : 1;
         UINT8 PSH : 1;
         UINT8 ACK : 1;
         UINT8 URG : 1;
         UINT8 ECE : 1;
         UINT8 CWR : 1;
      };
   };
   UINT16 window;
   UINT16 checksum;
   UINT16 urgentPointer;
}TCP_HEADER, *PTCP_HEADER;

/*
                    RFC 768 - User Datagram Protocol                 <br>
                                                                     <br>
    0                   1                   2                   3    <br>
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1  <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |          Source Port          |       Destination Port        | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
   |            Length             |           Checksum            | <br>
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <br>
                                                                     <br>
   RFC_REF: http://www.faqs.org/rfcs/rfc768.html                     <br>
*/
typedef struct _UDP_HEADER_
{
   UINT16 sourcePort;
   UINT16 destinationPort;
   UINT16 length;
   UINT16 checksum;
}UDP_HEADER, *PUDP_HEADER;

#pragma warning(pop) /// NAMELESS_STRUCT_UNION

_At_(*ppMACHeader, _Pre_ _Notnull_)
_At_(*ppMACHeader, _Post_ _Null_)
_Success_(*ppMACHeader == 0)
inline VOID KrnlHlprMACHeaderDestroy(_Inout_ VOID** ppMACHeader);

_When_(return != STATUS_SUCCESS, _At_(*ppMACHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppMACHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprMACHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                              _Outptr_ VOID** ppMACHeader,
                              _Inout_ BOOLEAN* pNeedToFreeMemory,
                              _In_ UINT32 macHeaderSize = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprMACHeaderModifySourceAddress(_In_ const FWP_VALUE* pValue,
                                              _Inout_ NET_BUFFER_LIST* pNetBufferList);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprMACHeaderModifyDestinationAddress(_In_ const FWP_VALUE* pValue,
                                                   _Inout_ NET_BUFFER_LIST* pNetBufferList);

_At_(*ppIPHeader, _Pre_ _Notnull_)
_At_(*ppIPHeader, _Post_ _Null_)
_Success_(*ppIPHeader == 0)
inline VOID KrnlHlprIPHeaderDestroy(_Inout_ VOID** ppIPHeader);

_When_(return != STATUS_SUCCESS, _At_(*ppIPHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppIPHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                             _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                             _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                             _Outptr_result_buffer_(*pIPHeaderSize) VOID** ppIPHeader,
                             _Inout_ BOOLEAN* pNeedToFreeMemory,
                             _Inout_opt_ FWP_DIRECTION* pDirection = 0,
                             _Inout_opt_ UINT32* pIPHeaderSize = 0);
_When_(return != STATUS_SUCCESS, _At_(*ppIPHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppIPHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                             _Outptr_result_buffer_(ipHeaderSize) VOID** ppIPHeader,
                             _Inout_ BOOLEAN* pNeedToFreeMemory,
                             _In_ UINT32 ipHeaderSize = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
BYTE* KrnlHlprIPHeaderGetDestinationAddressField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                                 _In_ ADDRESS_FAMILY addressFamily);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return != 0)
BYTE* KrnlHlprIPHeaderGetSourceAddressField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                            _In_ ADDRESS_FAMILY addressFamily);


_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
IPPROTO KrnlHlprIPHeaderGetProtocolField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                         _In_ ADDRESS_FAMILY addressFamily);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
UINT8 KrnlHlprIPHeaderGetVersionField(_In_ NET_BUFFER_LIST* pNetBufferList);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID KrnlHlprIPHeaderCalculateV4Checksum(_Inout_ NET_BUFFER_LIST* pNetBufferList,
                                         _In_ UINT32 ipHeaderSize = IPV4_HEADER_MIN_SIZE);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderModifySourceAddress(_In_ const FWP_VALUE* pValue,
                                             _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                             _In_ const BOOLEAN recalculateChecksum = TRUE,
                                             _In_ BOOLEAN convertByteOrder = FALSE);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderModifyDestinationAddress(_In_ const FWP_VALUE* pValue,
                                                  _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                                  _In_ const BOOLEAN recalculateChecksum = TRUE,
                                                  _In_ BOOLEAN convertByteOrder = FALSE);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprIPHeaderModifyLoopbackToLocal(_In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                               _In_ const FWP_VALUE* pRemoteAddress,
                                               _In_ const UINT32 ipHeaderSize,
                                               _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                               _In_reads_opt_(controlDataSize) const WSACMSGHDR* pControlData = 0,
                                               _In_ UINT32 controlDataSize = 0);

_At_(*ppTransportHeader, _Pre_ _Notnull_)
_At_(*ppTransportHeader, _Post_ _Null_)
_Success_(*ppTransportHeader == 0)
inline VOID KrnlHlprTransportHeaderDestroy(_Inout_ VOID** ppTransportHeader);

_When_(return != STATUS_SUCCESS, _At_(*ppTransportHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppTransportHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTransportHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                                    _In_ const FWPS_INCOMING_VALUES* pClassifyValues,
                                    _In_ const FWPS_INCOMING_METADATA_VALUES* pMetadata,
                                    _Outptr_result_buffer_(*pTransportHeaderSize) VOID** ppTransportHeader,
                                    _Inout_ BOOLEAN* pNeedToFreeMemory,
                                    _Inout_opt_ IPPROTO* pProtocol = 0,
                                    _Inout_opt_ FWP_DIRECTION* pDirection = 0,
                                    _Inout_opt_ UINT32* pTransportHeaderSize = 0);
_When_(return != STATUS_SUCCESS, _At_(*ppTransportHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppTransportHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTransportHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                                    _Outptr_result_buffer_(transportHeaderSize) VOID** ppTransportHeader,
                                    _Inout_ BOOLEAN* pNeedToFreeMemory,
                                    _In_ UINT32 transportHeaderSize = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
UINT16 KrnlHlprTransportHeaderGetSourcePortField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                                 _In_ IPPROTO protocol);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
UINT16 KrnlHlprTransportHeaderGetDestinationPortField(_In_ NET_BUFFER_LIST* pNetBufferList,
                                                      _In_ IPPROTO protocol);

_When_(return != STATUS_SUCCESS, _At_(*ppICMPv4Header, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppICMPv4Header, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv4HeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                                 _Outptr_result_buffer_(icmpHeaderSize) VOID** ppICMPv4Header,
                                 _Inout_ BOOLEAN* pNeedToFreeMemory,
                                 _In_ UINT32 icmpHeaderSize = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv4HeaderModifyType(_In_ const FWP_VALUE* pValue,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ UINT32 icmpHeaderSize = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv4HeaderModifyCode(_In_ const FWP_VALUE* pValue,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ UINT32 icmpHeaderSize = 0);

_When_(return != STATUS_SUCCESS, _At_(*ppICMPv4Header, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppICMPv4Header, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv6HeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                                 _Outptr_result_buffer_(icmpHeaderSize) VOID** ppICMPv4Header,
                                 _Inout_ BOOLEAN* pNeedToFreeMemory,
                                 _In_ UINT32 icmpHeaderSize = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv6HeaderModifyType(_In_ const FWP_VALUE* pValue,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ UINT32 icmpHeaderSize = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprICMPv6HeaderModifyCode(_In_ const FWP_VALUE* pValue,
                                        _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                        _In_ UINT32 icmpHeaderSize = 0);

_When_(return != STATUS_SUCCESS, _At_(*ppTCPHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppTCPHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTCPHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                             _Outptr_result_buffer_(tcpHeaderSize) VOID** ppTCPHeader,
                             _Inout_ BOOLEAN* pNeedToFreeMemory,
                             _In_ UINT32 tcpHeaderSize = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTCPHeaderModifySourcePort(_In_ const FWP_VALUE* pValue,
                                           _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                           _In_ UINT32 tcpHeaderSize = 0,
                                           _In_ BOOLEAN convertByteOrder = FALSE);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprTCPHeaderModifyDestinationPort(_In_ const FWP_VALUE* pValue,
                                                _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                                _In_ UINT32 tcpHeaderSize = 0,
                                                _In_ BOOLEAN convertByteOrder = FALSE);

_When_(return != STATUS_SUCCESS, _At_(*ppUDPHeader, _Post_ _Null_))
_When_(return == STATUS_SUCCESS, _At_(*ppUDPHeader, _Post_ _Notnull_))
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprUDPHeaderGet(_In_ NET_BUFFER_LIST* pNetBufferList,
                             _Outptr_result_buffer_(udpHeaderSize) VOID** ppUDPHeader,
                             _Inout_ BOOLEAN* pNeedToFreeMemory,
                             _In_ UINT32 udpHeaderSize = 0);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprUDPHeaderModifySourcePort(_In_ const FWP_VALUE* pValue,
                                           _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                           _In_ UINT32 udpHeaderSize = 0,
                                           _In_ BOOLEAN convertByteOrder = FALSE);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
_Check_return_
_Success_(return == STATUS_SUCCESS)
NTSTATUS KrnlHlprUDPHeaderModifyDestinationPort(_In_ const FWP_VALUE* pValue,
                                                _Inout_ NET_BUFFER_LIST* pNetBufferList,
                                                _In_ UINT32 udpheaderSize = 0,
                                                _In_ BOOLEAN convertByteOrder = FALSE);

#endif /// HELPERFUNCTIONS_HEADERS_H
