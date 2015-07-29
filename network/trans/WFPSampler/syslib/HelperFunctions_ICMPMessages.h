////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      HelperFunctions_ICMPMessages.h
//
//   Abstract:
//      This module contains prototypes for kernel helper functions that assist with ICMP Message 
//         packets.
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

#ifndef HELPERFUNCTIONS_ICMP_MESSAGES_H
#define HELPERFUNCTIONS_ICMP_MESSAGES_H

/*
   ICMP V4 Echo Message                                                                         <br>
                                                                                                <br>
      ICMP Type 0 = Echo Request                                                                <br>
                8 = Echo Reply                                                                  <br>
                                                                                                <br>
      ICMP Code 0                                                                               <br>
*/
typedef struct _ECHO_MESSAGE_
{
   UINT16 identifier;
   UINT16 sequence;
   BYTE   pStartOfData[1];
}ECHO_MESSAGE, *PECHO_MESSAGE;

/*
   ICMP V4 Destination Unreachable Message                                                      <br>
                                                                                                <br>
      ICMP Type 3                                                                               <br>
                                                                                                <br>
      ICMP Code 0 = Network Unreachable                                                         <br>
                1 = Host Unreachable                                                            <br>
                2 = Protocol Unreachable                                                        <br>
                3 = Port Unreachable                                                            <br>
                4 = Fragmentation needed but IP Header's Don't Fragment Flag is set             <br>
                5 = Source Route Failed                                                         <br>

*/
typedef struct _DESTINATION_UNREACHABLE_MESSAGE_
{
   UINT32      unused;
   IPV4_HEADER originalIPHeader;
   BYTE        pOriginalData[64];
}DESTINATION_UNREACHABLE_MESSAGE, *PDESTINATION_UNREACHABLE_MESSAGE;

/*
   ICMP V4 Source Quench Message                                                                <br>
                                                                                                <br>
      ICMP Type 4                                                                               <br>
                                                                                                <br>
      ICMP Code 0                                                                               <br>
*/
typedef struct _SOURCE_QUENCH_MESSAGE_
{
   UINT32      unused;
   IPV4_HEADER originalIPHeader;
   BYTE        pOriginalData[64];
}SOURCE_QUENCH_MESSAGE, *PSOURCE_QUENCH_MESSAGE;

/*
   ICMP V4 Redirect Message                                                                     <br>
                                                                                                <br>
      ICMP Type 5                                                                               <br>
                                                                                                <br>
      ICMP Code 0 = Redrect datagrams for the Network                                           <br>
                1 = Redrect datagrams for the Host                                              <br>
                2 = Redrect datagrams for the Type of Service and Network                       <br>
                3 = Redrect datagrams for the Type of Service and Host                          <br>
*/
typedef struct _REDIRECT_MESSAGE_
{
   UINT32      gatewayInternetAddress;
   IPV4_HEADER originalIPHeader;
   BYTE        pOriginalData[64];
}REDIRECT_MESSAGE, *PREDIRECT_MESSAGE;

/*
   ICMP V4 Time Exceeded Message                                                                <br>
                                                                                                <br>
      ICMP Type 11                                                                              <br>
                                                                                                <br>
      ICMP Code 0 = Time To Live exceeded in transit                                            <br>
                1 = Fragment reassembly time exceeded                                           <br>
*/
typedef struct _TIME_EXCEEDED_MESSAGE_
{
   UINT32      unused;
   IPV4_HEADER originalIPHeader;
   BYTE        pOriginalData[64];
}TIME_EXCEEDED_MESSAGE, *PTIME_EXCEEDED_MESSAGE;

/*
   ICMP V4 Parameter Problem Message                                                            <br>
                                                                                                <br>
      ICMP Type 12                                                                              <br>
                                                                                                <br>
      ICMP Code 0 = Pointer indicates the error                                                 <br>
*/
typedef struct _PARAMETER_PROBLEM_MESSAGE_
{
   UINT8       pointer;
   BYTE        pUnused[3];
   IPV4_HEADER originalIPHeader;
   BYTE        pOriginalData[64];
}PARAMETER_PROBLEM_MESSAGE, *PPARAMETER_PROBLEM_MESSAGE;

/*
   ICMP V4 Timestamp Message                                                                    <br>
                                                                                                <br>
      ICMP Type 13 = Timestamp Request                                                          <br>
                14 = Timestamp Reply                                                            <br>
                                                                                                <br>
      ICMP Code 0                                                                               <br>
*/
typedef struct _TIMESTAMP_MESSAGE_
{
   UINT16 identifier;
   UINT16 sequence;
   UINT32 originateTimestamp;
   UINT32 receiveTimestamp;
   UINT32 transmitTimestamp;
}TIMESTAMP_MESSAGE, *PTIMESTAMP_MESSAGE;

/*
   ICMP V4 Information Message                                                                  <br>
                                                                                                <br>
      ICMP Type 15 = Information Request                                                        <br>
                16 = Information Reply                                                          <br>
                                                                                                <br>
      ICMP Code 0                                                                               <br>
*/
typedef struct _INFORMATION_MESSAGE_
{
   UINT16 identifier;
   UINT16 sequence;
}INFORMATION_MESSAGE, *PINFORMATION_MESSAGE;

#endif /// HELPERFUNCTIONS_ICMP_MESSAGES_H