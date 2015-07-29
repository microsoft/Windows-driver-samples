///////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2012 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      ScenarioData.h
//
//   Abstract:
//      This module contains global definitions of Scenario specific data for the WFPSampler project
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//
///////////////////////////////////////////////////////////////////////////////

#ifndef WFP_SAMPLER_SCENARIO_DATA_H
#define WFP_SAMPLER_SCENARIO_DATA_H

#define  IEEE_802_ADDRESS_LENGTH         6
#define  IEEE_802_ADDRESS_STRING_BUFFER 17

/// FRAME_DATA Flags
#define FDF_INTERFACE_MAC_ADDRESS_SET        0x00000001
#define FDF_LOCAL_MAC_ADDRESS_SET            0x00000002
#define FDF_REMOTE_MAC_ADDRESS_SET           0x00000004
#define FDF_ETHER_TYPE_SET                   0x00000008
#define FDF_VLAN_ID_SET                      0x00000010
#define FDF_NDIS_PORT_SET                    0x00000020
#define FDF_NDIS_MEDIA_TYPE_SET              0x00000040
#define FDF_NDIS_PHYSICAL_MEDIA_TYPE_SET     0x00000080
#define FDF_L2_FLAGS_SET                     0x00000100
#define FDF_LOCAL_MAC_ADDRESS_TYPE_SET       0x00000200
#define FDF_REMOTE_MAC_ADDRESS_TYPE_SET      0x00000400
#define FDF_SOURCE_MAC_ADDRESS_SET           0x00000800
#define FDF_DESTINATION_MAC_ADDRESS_SET      0x00001000
#define FDF_SOURCE_MAC_ADDRESS_TYPE_SET      0x00002000
#define FDF_DESTINATION_MAC_ADDRESS_TYPE_SET 0x00004000

/// VSWITCH_DATA Flags
#define VSDF_ID_SET                         0x00000001
#define VSDF_NETWORK_TYPE_SET               0x00000002
#define VSDF_SOURCE_INTERFACE_ID_SET        0x00000004
#define VSDF_DESTINATION_INTERFACE_ID_SET   0x00000008
#define VSDF_SOURCE_INTERFACE_TYPE_SET      0x00000010
#define VSDF_DESTINATION_INTERFACE_TYPE_SET 0x00000020
#define VSDF_SOURCE_VM_ID_SET               0x00000040
#define VSDF_DESTINATION_VM_ID_SET          0x00000080

/// PACKET_DATA Flags
#define PDF_VERSION_SET                     0x00000001
#define PDF_PROTOCOL_SET                    0x00000002
#define PDF_LOCAL_ADDRESS_SET               0x00000004
#define PDF_LOCAL_PORT_SET                  0x00000008
#define PDF_REMOTE_ADDRESS_SET              0x00000010
#define PDF_REMOTE_PORT_SET                 0x00000020
#define PDF_LOCAL_ADDRESS_TYPE_SET          0x00000040
#define PDF_DESTINATION_ADDRESS_TYPE_SET    0x00000080
#define PDF_DIRECTION_SET                   0x00000100
#define PDF_FLAGS_SET                       0x00000200
#define PDF_NEXTHOP_ADDRESS_SET             0x00000400
#define PDF_ORIGINAL_ICMP_TYPE_SET          0x00000800
#define PDF_EMBEDDED_LOCAL_ADDRESS_TYPE_SET 0x00001000
#define PDF_EMBEDDED_REMOTE_ADDRESS_SET     0x00002000
#define PDF_EMBEDDED_PROTOCOL_SET           0x00004000
#define PDF_EMBEDDED_LOCAL_PORT_SET         0x00008000
#define PDF_EMBEDDED_REMOTE_PORT_SET        0x00010000

#define PDF_ICMP_TYPE_SET PDF_LOCAL_PORT_SET
#define PDF_ICMP_CODE_SET PDF_REMOTE_PORT_SET

#endif /// WFP_SAMPLER_SCENARIO_DATA_H