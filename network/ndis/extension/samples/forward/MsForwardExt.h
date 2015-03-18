/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

   MsForwardExt.h

Abstract:

    This file contains structures and function definitions
    necessary for MsForwardExt.


--*/


#define MSFORWARD_MAC_LENGTH    6

//
// MSFORWARD_CONTEXT
// The context allocated per switch.
//
typedef struct _MSFORWARD_CONTEXT
{
    BOOLEAN                 IsActive;

    NDIS_SWITCH_PORT_ID     ExternalPortId;
    NDIS_SWITCH_NIC_INDEX   ExternalNicIndex;
    BOOLEAN                 ExternalNicConnected;
    
    //
    // This sample uses linked lists for the NICs and property
    // lookup. THIS IS NOT RECOMMENDED.
    //
    LIST_ENTRY              NicList;
    LIST_ENTRY              PropertyList;
    PNDIS_RW_LOCK_EX        DispatchLock;
    
    UINT32                  NumDestinations;
    BOOLEAN                 IsInitialRestart;
} MSFORWARD_CONTEXT, *PMSFORWARD_CONTEXT;

//
// MSFORWARD_NIC_LIST_ENTRY
// The context allocated per NIC.
//
typedef struct _MSFORWARD_NIC_LIST_ENTRY
{
    LIST_ENTRY                           ListEntry;
    UINT8                                MacAddress[MSFORWARD_MAC_LENGTH];
    NDIS_SWITCH_PORT_ID                  PortId;
    NDIS_SWITCH_NIC_INDEX                NicIndex;
    NDIS_SWITCH_NIC_TYPE                 NicType;
    BOOLEAN                              AllowSends;
    BOOLEAN                              Connected;
} MSFORWARD_NIC_LIST_ENTRY, *PMSFORWARD_NIC_LIST_ENTRY;

//
// MSFORWARD_MAC_POLICY_LIST_ENTRY
// The context allocated per switch policy.
//
typedef struct _MSFORWARD_MAC_POLICY_LIST_ENTRY
{
    LIST_ENTRY                      ListEntry;
    UINT8                           MacAddress[MSFORWARD_MAC_LENGTH];
    NDIS_SWITCH_OBJECT_INSTANCE_ID  PropertyInstanceId;
} MSFORWARD_MAC_POLICY_LIST_ENTRY, *PMSFORWARD_MAC_POLICY_LIST_ENTRY;

//
// MSFORWARD_MAC_ADDRESS_POLICY
// The serialization of the policy structure found
// in MsForwardExtPolicy.mof
//
typedef struct _MSFORWARD_MAC_ADDRESS_POLICY
{
    UINT32  MacAddressLength;
    UINT8   MacAddress[MSFORWARD_MAC_LENGTH];
} MSFORWARD_MAC_ADDRESS_POLICY, *PMSFORWARD_MAC_ADDRESS_POLICY;

//
// MSFORWARD_MAC_ADDRESS_POLICY_STATUS
// The serialization of the policy status structure
// found in MsForwardExtPolicyStatus.mof
//
typedef struct _MSFORWARD_MAC_ADDRESS_POLICY_STATUS
{
    UINT32  PortArrayLength;
    UINT32  PortArrayOffset;
} MSFORWARD_MAC_ADDRESS_POLICY_STATUS, *PMSFORWARD_MAC_ADDRESS_POLICY_STATUS;

//
// MSFORWARD_ETHERNET_HEADER
// Ethernet header definition.
//
typedef struct _MSFORWARD_ETHERNET_HEADER
{
    UINT8       Destination[MSFORWARD_MAC_LENGTH];
    UINT8       Source[MSFORWARD_MAC_LENGTH];
    UINT16      Type;
} MSFORWARD_ETHERNET_HEADER, *PMSFORWARD_ETHERNET_HEADER;

//
// MacAddressPolicyGuid
//
// The GUID representing the switch policy owned
// by MsForwardExt.
//
extern const NDIS_SWITCH_OBJECT_ID MacAddressPolicyGuid;

//
// MacAddressPolicyStatusGuid
//
// The GUID representing the switch policy status
// indicated for the status of the switch policy
// owned by MsForwardExt.
//
extern const NDIS_SWITCH_OBJECT_ID MacAddressPolicyStatusGuid;

//
// Switch Property Macros
//                             
#define MAC_ADDRESS_POLICY_VERSION  0x0100
#define MAC_ADDRESS_POLICY_SERIALIZATION_VERSION  NDIS_SWITCH_OBJECT_SERIALIZATION_VERSION_1

//
// Switch Property Status Macros
//
#define MAC_ADDRESS_POLICY_STATUS_VERSION  0x0100
#define MAC_ADDRESS_POLICY_STATUS_SERIALIZATION_VERSION  NDIS_SWITCH_OBJECT_SERIALIZATION_VERSION_1


//
// Private functions used by MsForwardExt
//
NDIS_STATUS
MsForwardAddNicUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_reads_bytes_(6) PUCHAR MacAddress,
    _In_ NDIS_SWITCH_PORT_ID PortId,
    _In_ NDIS_SWITCH_NIC_INDEX NicIndex,
    _In_ NDIS_SWITCH_NIC_TYPE NicType,
    _In_ BOOLEAN Connected
    );
    
NDIS_STATUS
MsForwardDeleteNicUnsafe(
   _In_ PMSFORWARD_CONTEXT SwitchContext,
   _In_ NDIS_SWITCH_PORT_ID PortId,
   _In_ NDIS_SWITCH_NIC_INDEX NicIndex
   );
   
VOID
MsForwardClearNicListUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext
    );
    
PMSFORWARD_NIC_LIST_ENTRY
MsForwardFindNicByPortIdUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ NDIS_SWITCH_PORT_ID PortId,
    _In_ NDIS_SWITCH_NIC_INDEX NicIndex
    );
    
PMSFORWARD_NIC_LIST_ENTRY
MsForwardFindNicByMacAddressUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_reads_bytes_(6) PUCHAR MacAddress
    );

NDIS_STATUS
MsForwardAddMacPolicyUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ PMSFORWARD_MAC_ADDRESS_POLICY MacPolicyBuffer,
    _In_ PNDIS_SWITCH_OBJECT_INSTANCE_ID PropertyInstanceId
    );
    
VOID
MsForwardDeleteMacPolicyUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ PNDIS_SWITCH_OBJECT_INSTANCE_ID PropertyInstanceId
    );

VOID
MsForwardClearPropertyListUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext
    );
    
PMSFORWARD_MAC_POLICY_LIST_ENTRY
MsForwardFindPolicyByMacAddressUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_reads_bytes_(6) PUCHAR MacAddress
    );
    
PMSFORWARD_MAC_POLICY_LIST_ENTRY
MsForwardFindPolicyByPropertyInstanceIdUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ PNDIS_SWITCH_OBJECT_INSTANCE_ID PropertyInstanceId
    );
    
BOOLEAN
MsForwardNicHasPolicy(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_reads_bytes_(6) PUCHAR MacAddress
    );
    
VOID
MsForwardMakeBroadcastArrayUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ PNDIS_SWITCH_FORWARDING_DESTINATION_ARRAY BroadcastArray,
    _In_ NDIS_SWITCH_PORT_ID SourcePortId,
    _In_ NDIS_SWITCH_NIC_INDEX SourceNicIndex
    );
    
NDIS_STATUS
MsForwardInitSwitch(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ PMSFORWARD_CONTEXT SwitchContext
    );
