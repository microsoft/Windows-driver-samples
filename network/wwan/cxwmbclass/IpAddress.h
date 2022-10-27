//
//    Copyright (C) Microsoft.  All rights reserved.
//

////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////
#define MBB_IP_IS_IPV4_AVAILABLE(_X_) (((_X_)->IPv4Flags & MbbIpFlagsAddressAvailable) == MbbIpFlagsAddressAvailable)
#define MBB_IP_IS_IPV6_AVAILABLE(_X_) (((_X_)->IPv6Flags & MbbIpFlagsAddressAvailable) == MbbIpFlagsAddressAvailable)
#define MBB_IP_IS_GWV4_AVAILABLE(_X_) (((_X_)->IPv4Flags & MbbIpFlagsGatewayAvailable) == MbbIpFlagsGatewayAvailable)
#define MBB_IP_IS_GWV6_AVAILABLE(_X_) (((_X_)->IPv6Flags & MbbIpFlagsGatewayAvailable) == MbbIpFlagsGatewayAvailable)
#define MBB_IP_IS_MTUV4_AVAILABLE(_X_) (((_X_)->IPv4Flags & MbbIpFlagsMTUAvailable) == MbbIpFlagsMTUAvailable)
#define MBB_IP_IS_MTUV6_AVAILABLE(_X_) (((_X_)->IPv6Flags & MbbIpFlagsMTUAvailable) == MbbIpFlagsMTUAvailable)
#define MBB_IP_IS_DSV4_AVAILABLE(_X_) (((_X_)->IPv4Flags & MbbIpFlagsDnsServerAvailable) == MbbIpFlagsDnsServerAvailable)
#define MBB_IP_IS_DSV6_AVAILABLE(_X_) (((_X_)->IPv6Flags & MbbIpFlagsDnsServerAvailable) == MbbIpFlagsDnsServerAvailable)

#define MBB_IP_GET_IPV4TABLE(_X_) (PMBB_IPV4_ADDRESS)(MBB_IP_IS_IPV4_AVAILABLE(_X_)? (((PCHAR)(_X_)) + (_X_)->IPv4AddressOffset): NULL)
#define MBB_IP_GET_IPV6TABLE(_X_) (PMBB_IPV6_ADDRESS)(MBB_IP_IS_IPV6_AVAILABLE(_X_)? (((PCHAR)(_X_)) + (_X_)->IPv6AddressOffset): NULL)
#define MBB_IP_GET_GWV4TABLE(_X_) (IN_ADDR*) (MBB_IP_IS_GWV4_AVAILABLE(_X_)? (((PCHAR)(_X_)) + (_X_)->IPv4GatewayOffset): NULL)
#define MBB_IP_GET_GWV6TABLE(_X_) (IN6_ADDR*)(MBB_IP_IS_GWV6_AVAILABLE(_X_)? (((PCHAR)(_X_)) + (_X_)->IPv6GatewayOffset): NULL)
#define MBB_IP_GET_DSV4TABLE(_X_) (IN_ADDR*) (MBB_IP_IS_DSV4_AVAILABLE(_X_)? (((PCHAR)(_X_)) + (_X_)->IPv4DnsServerOffset): NULL)
#define MBB_IP_GET_DSV6TABLE(_X_) (IN6_ADDR*)(MBB_IP_IS_DSV6_AVAILABLE(_X_)? (((PCHAR)(_X_)) + (_X_)->IPv6DnsServerOffset): NULL)

#define MBB_IP_GET_IPV4COUNT(_X_) (MBB_IP_IS_IPV4_AVAILABLE(_X_)? (_X_)->IPv4AddressCount: 0)
#define MBB_IP_GET_IPV6COUNT(_X_) (MBB_IP_IS_IPV6_AVAILABLE(_X_)? (_X_)->IPv6AddressCount: 0)
#define MBB_IP_GET_DSV4COUNT(_X_) (MBB_IP_IS_DSV4_AVAILABLE(_X_)? (_X_)->IPv4DnsServerCount: 0)
#define MBB_IP_GET_DSV6COUNT(_X_) (MBB_IP_IS_DSV6_AVAILABLE(_X_)? (_X_)->IPv6DnsServerCount: 0)
#define MBB_IP_GET_GWV4COUNT(_X_) (MBB_IP_IS_GWV4_AVAILABLE(_X_)? 1: 0)
#define MBB_IP_GET_GWV6COUNT(_X_) (MBB_IP_IS_GWV6_AVAILABLE(_X_)? 1: 0)



////////////////////////////////////////////////////////////////////////////////
//
//  TYPEDEFS
//
////////////////////////////////////////////////////////////////////////////////

typedef enum
{
    MbbIPChangeTypeNone = 0,
    MbbIPChangeTypeDeviceConfiguration,
    MbbIPChangeTypeDeviceConfigurationChange,
    MbbIPChangeTypeHostInterfaceChange,
    MbbIPChangeTypeHostIPChange,
    MbbIPChangeTypeHostRouteChange,

} MBB_IP_CHANGE_TYPE;

////////////////////////////////////////////////////////////////////////////////
//
//  PROTOTYPES
//
////////////////////////////////////////////////////////////////////////////////
NDIS_STATUS
MbbIpInitialize(
    __in PMINIPORT_DRIVER_CONTEXT Driver
    );

VOID
MbbIpCleanup(
    __in PMINIPORT_DRIVER_CONTEXT Driver
    );

NDIS_STATUS
MbbIpAddressStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );
