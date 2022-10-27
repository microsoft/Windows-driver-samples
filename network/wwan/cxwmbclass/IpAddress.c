//
//    Copyright (C) Microsoft.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////
//
//  INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "IpAddress.tmh"


#define MBB_PREALLOC_IP_WORKITEM_COUNT      1


////////////////////////////////////////////////////////////////////////////////
//
//  PROTOTYPES
//
////////////////////////////////////////////////////////////////////////////////

_Requires_lock_held_( Port->Lock )
VOID
MbbIpSnapPortAddressTable(
    __in PMBB_PORT                                          Port,
    __in MBB_ADDRESS_TABLE_ID                               TableId,
    __deref_out_ecount(*AddressCount) PMBB_IPADDRESS_ENTRY* AddressTable,
    __out PULONG                                            AddressCount
    );

_Requires_lock_held_( Port->Lock )
VOID
MbbIpSnapPortIpTable(
    __in PMBB_PORT                                          Port,
    __deref_out_ecount(*AddressCount) PMBB_IPADDRESS_ENTRY* AddressTable,
    __out PULONG                                            AddressCount
    );

__callback
VOID
MbbIpProcessChange(
    __in PVOID  Context1,
    __in PVOID  Context2,
    __in PVOID  Context3,
    __in PVOID  Context4
    );

NDIS_STATUS
MbbIpQueueIPChange(
    __in PMINIPORT_DRIVER_CONTEXT   Driver,
    __in PNET_LUID                  NetLuid,
    __in MBB_IP_CHANGE_TYPE         IPChangeType,
    __in NDIS_PORT_NUMBER           PortNumber
    );

__callback
VOID
MbbIpInterfaceChangeCallback(
    __in PVOID                      CallerContext,
    __in_opt PMIB_IPINTERFACE_ROW   Row,
    __in MIB_NOTIFICATION_TYPE      NotificationType
    );

__callback
VOID
MbbIpRouteChangeCallback(
    _In_ PVOID                      CallerContext,
    _In_opt_ PMIB_IPFORWARD_ROW2    Row,
    _In_ MIB_NOTIFICATION_TYPE      NotificationType
    );

__callback
VOID
MbbIpUnicastAddressChangeCallback(
    _In_ PVOID                          CallerContext,
    _In_opt_ PMIB_UNICASTIPADDRESS_ROW  Row,
    _In_ MIB_NOTIFICATION_TYPE          NotificationType
    );





////////////////////////////////////////////////////////////////////////////////
//
//  IMPLEMENTATION
//
////////////////////////////////////////////////////////////////////////////////
_Requires_lock_held_( Port->Lock )
VOID
MbbIpSnapPortAddressTable(
    __in PMBB_PORT                                          Port,
    __in MBB_ADDRESS_TABLE_ID                               TableId,
    __deref_out_ecount(*AddressCount) PMBB_IPADDRESS_ENTRY* AddressTable,
    __out PULONG                                            AddressCount
    )
{
    ULONG                   AddressIndex;
    ULONG                   V4Index;
    ULONG                   V6Index;
    ULONG                   V4Count = 0;
    ULONG                   V6Count = 0;
    #pragma prefast (suppress:__WARNING_IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC)
    IN_ADDR*                V4Table = NULL;
    IN6_ADDR*               V6Table = NULL;
    ULONG                   LocalAddressCount = 0;
    PMBB_IPADDRESS_ENTRY    LocalAddressTable = NULL;

    do
    {
        if( Port->IpAddressInfo == NULL )
        {
            //
            // We are not connected yet.
            //
            break;
        }

        switch( TableId )
        {
            case MbbAddressTableGateway:
            {
                V4Count = MBB_IP_GET_GWV4COUNT( Port->IpAddressInfo );
                V4Table = MBB_IP_GET_GWV4TABLE( Port->IpAddressInfo );
                V6Count = MBB_IP_GET_GWV6COUNT( Port->IpAddressInfo );
                V6Table = MBB_IP_GET_GWV6TABLE( Port->IpAddressInfo );
            }
            break;

            case MbbAddressTableDns:
            {
                V4Count = MBB_IP_GET_DSV4COUNT( Port->IpAddressInfo );
                V4Table = MBB_IP_GET_DSV4TABLE( Port->IpAddressInfo );
                V6Count = MBB_IP_GET_DSV6COUNT( Port->IpAddressInfo );
                V6Table = MBB_IP_GET_DSV6TABLE( Port->IpAddressInfo );
            }
            break;

            default:
            {
                ASSERT( TableId == MbbAddressTableGateway || TableId == MbbAddressTableDns );
            }
            break;
        }

        if( (LocalAddressCount = V4Count + V6Count) == 0 )
        {
            break;
        }

        if( (LocalAddressTable = ALLOCATE_NONPAGED_POOL( sizeof(MBB_IPADDRESS_ENTRY) * LocalAddressCount )) == NULL )
        {
            LocalAddressCount = 0;
            break;
        }

        for( V4Index = 0, AddressIndex = 0;
             V4Index < V4Count;
             V4Index ++, AddressIndex ++ )
        {
            LocalAddressTable[AddressIndex].IsIpv6       = FALSE;
            LocalAddressTable[AddressIndex].IsReported   = FALSE;

            #pragma prefast (suppress:__WARNING_IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC)
            RtlCopyMemory(
                &LocalAddressTable[AddressIndex].Ipv4.IPV4Address,
                &V4Table[V4Index],
                sizeof(IN_ADDR)
                );
        }

        for( V6Index = 0;
             V6Index < V6Count;
             V6Index ++, AddressIndex ++ )
        {
            LocalAddressTable[AddressIndex].IsIpv6       = TRUE;
            LocalAddressTable[AddressIndex].IsReported   = FALSE;

            RtlCopyMemory(
                &LocalAddressTable[AddressIndex].Ipv6.IPV6Address,
                &V6Table[V6Index],
                sizeof(IN6_ADDR)
                );
        }
    } while( FALSE );

    *AddressCount = LocalAddressCount;
    *AddressTable = LocalAddressTable;
}

_Requires_lock_held_( Port->Lock )
VOID
MbbIpSnapPortIpTable(
    __in PMBB_PORT                                          Port,
    __deref_out_ecount(*AddressCount) PMBB_IPADDRESS_ENTRY* AddressTable,
    __out PULONG                                            AddressCount
    )
{
    ULONG                   AddressIndex;
    ULONG                   V4Index;
    ULONG                   V6Index;
    ULONG                   V4Count = 0;
    ULONG                   V6Count = 0;
    PMBB_IPV4_ADDRESS       V4Table = NULL;
    PMBB_IPV6_ADDRESS       V6Table = NULL;
    ULONG                   LocalAddressCount = 0;
    PMBB_IPADDRESS_ENTRY    LocalAddressTable = NULL;

    do
    {
        if( Port->IpAddressInfo == NULL )
        {
            //
            // We are not connected yet.
            //
            break;
        }

        V4Count = MBB_IP_GET_IPV4COUNT( Port->IpAddressInfo );
        V4Table = MBB_IP_GET_IPV4TABLE( Port->IpAddressInfo );
        V6Count = MBB_IP_GET_IPV6COUNT( Port->IpAddressInfo );
        V6Table = MBB_IP_GET_IPV6TABLE( Port->IpAddressInfo );

        if( (LocalAddressCount = V4Count + V6Count) == 0 )
        {
            break;
        }

        if( (LocalAddressTable = ALLOCATE_NONPAGED_POOL( sizeof(MBB_IPADDRESS_ENTRY) * LocalAddressCount )) == NULL )
        {
            LocalAddressCount = 0;
            break;
        }

        for( V4Index = 0, AddressIndex = 0;
             V4Index < V4Count;
             V4Index ++, AddressIndex ++ )
        {
            LocalAddressTable[AddressIndex].IsIpv6       = FALSE;
            LocalAddressTable[AddressIndex].IsReported   = FALSE;

            RtlCopyMemory(
                &LocalAddressTable[AddressIndex].Ipv4,
                &V4Table[V4Index],
                sizeof(MBB_IPV4_ADDRESS)
                );
        }

        for( V6Index = 0;
             V6Index < V6Count;
             V6Index ++, AddressIndex ++ )
        {
            LocalAddressTable[AddressIndex].IsIpv6       = TRUE;
            LocalAddressTable[AddressIndex].IsReported   = FALSE;

            RtlCopyMemory(
                &LocalAddressTable[AddressIndex].Ipv6,
                &V6Table[V6Index],
                sizeof(MBB_IPV6_ADDRESS)
                );
        }
    } while( FALSE );

    *AddressCount = LocalAddressCount;
    *AddressTable = LocalAddressTable;
}

//
// Ip Address CID
//

NDIS_STATUS
MbbIpAddressStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
/*++
Description:

    The IP Status Handler processes responses to requested IP Info as well as
    unsolicited change indications from the device. Only one instance of this routine
    runs at any time due to the CID Serializer (RequestManager) so there is no race in
    reporting two IP indications from the device on two threads.

    It does the following -

    1. Verify that IP information received is valid.

    2. Copy & cache the information in the adapter object.
        a. Required for validating the IP configured on the adpater in IpChangeCallback.

    3. Enable \ Disable DHCPv4 depending on whether IP Address is available via Layer2.
       DHCPv6 or NDP behavior is not changed.
       The network is required to be configured correctly i.e. not give IPv6 over Layer2 and Layer3.

    4. Report the new DNS, IP, Gateway to IP stack.
       The order in which the information is reported is important.
        a. MTU (Supported starting in Blue)
        b. DNS Servers
        c. IP Address
        d. Routes

    5. Indicate LinkUP to NDIS under the following condition -
        a. DNS Server setting changed. LinkDOWN followed by a LinkUP is required for the stack to pick up the changes.
        b. IP Information was retrieved as a result of a connect (not unsolicited indication).
--*/
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter = Request->RequestManager->AdapterContext;
    PMBB_IP_ADDRESS_INFO        IpAddressInfo = (PMBB_IP_ADDRESS_INFO)InBuffer;
    MBB_CONNECTION_STATE        ConnectionState;
    PMBB_PORT                   Port = NULL;
    NDIS_PORT_NUMBER            PortNumber = DEFAULT_NDIS_PORT_NUMBER;
    USHORT                      EffectiveMaxMtu = MBIM_MAX_MTU_SIZE;
    
    do
    {
        //
        // Only parse the result on sucess
        //
        if( NdisStatus != STATUS_SUCCESS ||
            (MbbStatus != MBB_STATUS_SUCCESS) ||
              IpAddressInfo == NULL )
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: FAILED to get IP Address info, NdisStatus=%!STATUS! MbbStatus=%!MbbStatus!",
                        Adapter->NetLuid.Value,
                        NdisStatus,
                        MbbStatus
                        );
            break;
        }

        //Get the port corresponding to the session id   

        Port = MbbWwanTranslateSessionIdToPort(Adapter, IpAddressInfo->SessionId);        

        if(!Port)
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: Invalid session id. No port corresponding to the session id %lu",
                        Adapter->NetLuid.Value, IpAddressInfo->SessionId
                        );            
            break;
        }

        MBB_ACQUIRE_PORT_LOCK(Port);
        PortNumber = Port->PortNumber;
        MBB_RELEASE_PORT_LOCK(Port);
        
        //
        // Device should only send unsolicited indications after successful connection.
        //
        if( MbbReqMgrIsUnsolicitedIndication( Request ) == TRUE &&          
            MbbPortIsConnected(Port) == FALSE )
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: IGNORING unsolicited notification when port is disconnected",
                        Adapter->NetLuid.Value
                        );
            break;
        }
     
        //
        // Minimum buffer size check
        //
        if( InBufferSize < sizeof(MBB_IP_ADDRESS_INFO) )
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: INSUFFICIENT IP Address info length, expected=%d received=%d",
                        Adapter->NetLuid.Value,
                        sizeof(MBB_IP_ADDRESS_INFO),
                        InBufferSize
                        );
            break;
        }

        //
        // Max MTU is different from initialized value if the device has the Extended descriptor (per Errata).
        //
        if ( Adapter->BusParams.IsErrataDevice )
        {
            EffectiveMaxMtu = Adapter->BusParams.MTU;
        }

        //
        // Minimum and Maximum IPv4 MTU size check, but only require correct
        // value if MTU being set. If not correct, just ignore.
        //
        if( MBB_IP_IS_MTUV4_AVAILABLE( IpAddressInfo ) )
        {
          
            if( (
                    IpAddressInfo->IPv4MTU < MBIM_MIN_MTU_SIZE  
                ) ||
                (
                    (USHORT) IpAddressInfo->IPv4MTU >  EffectiveMaxMtu
                ) ) 
            
            {
                TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: IGNORING Invalid IPv4 MTU value, received=%d min=%d max=%d",
                            Adapter->NetLuid.Value,
                            IpAddressInfo->IPv4MTU,
                            MBIM_MIN_MTU_SIZE,
                            EffectiveMaxMtu
                            );
                // Do *NOT* break so other settings continue to work even if MTU setting is off.
                // Simply set to be ignored when effecting the actual change in MbbIpProcessChange() later on.
                IpAddressInfo->IPv4Flags &= ~MbbIpFlagsMTUAvailable;
            } 
        }

        //
        // Minimum and Maximum IPv6 MTU size check, but only require correct
        // value if MTU being set, otherwise ignore.
        //
        if( MBB_IP_IS_MTUV6_AVAILABLE( IpAddressInfo ) )
        {
          
            if( (
                    IpAddressInfo->IPv6MTU < MBIM_MIN_MTU_SIZE  
                ) ||
                (
                    (USHORT) IpAddressInfo->IPv6MTU >  EffectiveMaxMtu
                ) ) 
            
            {
                TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: IGNORING Invalid IPv6 MTU value, received=%d min=%d max=%d",
                            Adapter->NetLuid.Value,
                            IpAddressInfo->IPv6MTU,
                            MBIM_MIN_MTU_SIZE,
                            EffectiveMaxMtu
                            );
                // Do *NOT* break so other settings continue to work even if MTU setting is off.
                // Simply set to be ignored when effecting the actual change in MbbIpProcessChange() later on.
                IpAddressInfo->IPv6Flags &= ~MbbIpFlagsMTUAvailable;
            }
        }

        //
        // Variable fields check
        //
        #pragma prefast (suppress:__WARNING_IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC)
        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            IpAddressInfo->IPv4GatewayOffset,
            sizeof(IN_ADDR),
            1,
            sizeof(IN_ADDR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: Invalid IPv4 gateway, offset=%d received=%d",
                        Adapter->NetLuid.Value,
                        IpAddressInfo->IPv4GatewayOffset,
                        InBufferSize
                        );

            break;

        }

        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            IpAddressInfo->IPv6GatewayOffset,
            sizeof(IN6_ADDR),
            1,
            sizeof(IN6_ADDR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: Invalid IPv6 gateway, offset=%d received=%d",
                        Adapter->NetLuid.Value,
                        IpAddressInfo->IPv6GatewayOffset,
                        InBufferSize
                        );

            break;

        }


        NdisStatus=MbbIsArrayFieldValid(
            InBufferSize,
            IpAddressInfo->IPv4AddressOffset,
            IpAddressInfo->IPv4AddressCount,
            sizeof(MBB_IPV4_ADDRESS)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: Invalid IPv4 address array, offset=%d, count=%d, received=%d",
                        Adapter->NetLuid.Value,
                        IpAddressInfo->IPv4AddressOffset,
                        IpAddressInfo->IPv4AddressCount,
                        InBufferSize
                        );

            break;

        }

        NdisStatus=MbbIsArrayFieldValid(
            InBufferSize,
            IpAddressInfo->IPv6AddressOffset,
            IpAddressInfo->IPv6AddressCount,
            sizeof(MBB_IPV6_ADDRESS)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: Invalid IPv6 address array, offset=%d, count=%d, received=%d",
                        Adapter->NetLuid.Value,
                        IpAddressInfo->IPv6AddressOffset,
                        IpAddressInfo->IPv6AddressCount,
                        InBufferSize
                        );

            break;

        }

        #pragma prefast (suppress:__WARNING_IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC)
        NdisStatus=MbbIsArrayFieldValid(
            InBufferSize,
            IpAddressInfo->IPv4DnsServerOffset,
            IpAddressInfo->IPv4DnsServerCount,
            sizeof(IN_ADDR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: Invalid IPv4 DNS address array, offset=%d, count=%d, received=%d",
                        Adapter->NetLuid.Value,
                        IpAddressInfo->IPv4DnsServerOffset,
                        IpAddressInfo->IPv4DnsServerCount,
                        InBufferSize
                        );

            break;

        }

        NdisStatus=MbbIsArrayFieldValid(
            InBufferSize,
            IpAddressInfo->IPv6DnsServerOffset,
            IpAddressInfo->IPv6DnsServerCount,
            sizeof(IN6_ADDR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: Invalid IPv6 DNS address array, offset=%d, count=%d, received=%d",
                        Adapter->NetLuid.Value,
                        IpAddressInfo->IPv6DnsServerOffset,
                        IpAddressInfo->IPv6DnsServerCount,
                        InBufferSize
                        );

            break;

        }


        //
        // IP and Gateway should be reported using the same mechanism
        //
        if( MBB_IP_IS_IPV4_AVAILABLE( IpAddressInfo ) != MBB_IP_IS_GWV4_AVAILABLE( IpAddressInfo ) &&
             MBB_IP_IS_IPV6_AVAILABLE( IpAddressInfo ) != MBB_IP_IS_GWV6_AVAILABLE( IpAddressInfo ) )
        {
            ASSERT( FALSE );

            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: IP and Gateway should be configured using same Layer mechanisms. v4[IP=%!bool! GW=%!bool!] v6[IP=%!bool! GW=%!bool!]",
                        Adapter->NetLuid.Value,
                        MBB_IP_IS_IPV4_AVAILABLE( IpAddressInfo ),
                        MBB_IP_IS_GWV4_AVAILABLE( IpAddressInfo ),
                        MBB_IP_IS_IPV6_AVAILABLE( IpAddressInfo ),
                        MBB_IP_IS_GWV6_AVAILABLE( IpAddressInfo )
                        );
            break;
        }
        //
        //  Log a warning when IP is derived from Layer2 mechanisms and DNS is derived from Layer3
        //
        if( (
                MBB_IP_IS_IPV4_AVAILABLE( IpAddressInfo ) &&
               !MBB_IP_IS_DSV4_AVAILABLE( IpAddressInfo )
            ) ||
            (
                MBB_IP_IS_IPV6_AVAILABLE( IpAddressInfo ) &&
               !MBB_IP_IS_DSV6_AVAILABLE( IpAddressInfo )
            ) )
        {
            TraceWarn( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: DNS configured using Layer3 when IP is derived from Layer2. v4[IP=%!bool! DNS=%!bool!] v6[IP=%!bool! DNS=%!bool!]",
                       Adapter->NetLuid.Value,
                       MBB_IP_IS_IPV4_AVAILABLE( IpAddressInfo ),
                       MBB_IP_IS_DSV4_AVAILABLE( IpAddressInfo ),
                       MBB_IP_IS_IPV6_AVAILABLE( IpAddressInfo ),
                       MBB_IP_IS_DSV6_AVAILABLE( IpAddressInfo )
                       );
        }

        if(!IS_ALLOCATED_PORT_NUMBER(PortNumber))
        {
            // Enable/Disable DHCP for default port connection. For non-default
            // port connection the virtual miniport will do it when we send 
            // NDIS_WWAN_IP_ADDRESS_STATE indication
            WwanAdjustDhcpSettings(
                Adapter->NetLuid,
                Adapter->NetCfgId,
                (! MBB_IP_IS_IPV4_AVAILABLE( IpAddressInfo )));
        }

        TraceInfo(  WMBCLASS_IP, "[MbbIP][Luid=0x%I64x][Port=%lu] CID_IP_CONFIGURATION: %02d IPv4 %02d IPv6 %02d DNv4 %02d DNv6 %02d GWv4 %02d GWv6 (IPv4MTU %d IPv6MTU %d)",
                    Adapter->NetLuid.Value,
                    PortNumber,
                    MBB_IP_IS_IPV4_AVAILABLE( IpAddressInfo )?  IpAddressInfo->IPv4AddressCount:    0,
                    MBB_IP_IS_IPV6_AVAILABLE( IpAddressInfo )?  IpAddressInfo->IPv6AddressCount:    0,
                    MBB_IP_IS_DSV4_AVAILABLE( IpAddressInfo )?  IpAddressInfo->IPv4DnsServerCount:  0,
                    MBB_IP_IS_DSV6_AVAILABLE( IpAddressInfo )?  IpAddressInfo->IPv6DnsServerCount:  0,
                    MBB_IP_IS_GWV4_AVAILABLE( IpAddressInfo )?  1: 0,
                    MBB_IP_IS_GWV6_AVAILABLE( IpAddressInfo )?  1: 0,
                    MBB_IP_IS_MTUV4_AVAILABLE( IpAddressInfo ) ? IpAddressInfo->IPv4MTU : 0,
                    MBB_IP_IS_MTUV6_AVAILABLE( IpAddressInfo ) ? IpAddressInfo->IPv6MTU : 0
                    );

        if (MBB_IP_IS_IPV4_AVAILABLE(IpAddressInfo))
        {
            PMBB_IPV4_ADDRESS pIPV4Elem = (PMBB_IPV4_ADDRESS)(InBuffer + IpAddressInfo->IPv4AddressOffset);
            for (unsigned int i = 0; i < IpAddressInfo->IPv4AddressCount; i++, pIPV4Elem++)
            {
                TraceInfo(  WMBCLASS_IP, "CID_IP_CONFIGURATION: IPv4 #%d (subnetAddrLen %d) : %!IPV4ADDR!",
                            i+1, pIPV4Elem->OnLinkPrefixLength, pIPV4Elem->IPV4Address);
            }
        }

        if (MBB_IP_IS_GWV4_AVAILABLE(IpAddressInfo))
        {
            PIN_ADDR pInAddr = (PIN_ADDR)(InBuffer + IpAddressInfo->IPv4GatewayOffset);
            TraceInfo(  WMBCLASS_IP, "CID_IP_CONFIGURATION: GWv4: %d.%d.%d.%d", 
                        pInAddr->S_un.S_un_b.s_b1, pInAddr->S_un.S_un_b.s_b2, pInAddr->S_un.S_un_b.s_b3, pInAddr->S_un.S_un_b.s_b4);
        }

        if (MBB_IP_IS_DSV4_AVAILABLE(IpAddressInfo))
        {
            PIN_ADDR pInAddr = (PIN_ADDR)(InBuffer + IpAddressInfo->IPv4DnsServerOffset);
            for (unsigned int i = 0; i < IpAddressInfo->IPv4DnsServerCount; i++, pInAddr++)
            {
                TraceInfo(  WMBCLASS_IP, "CID_IP_CONFIGURATION: DNSv4 #%d: %d.%d.%d.%d", 
                            i+1, pInAddr->S_un.S_un_b.s_b1, pInAddr->S_un.S_un_b.s_b2, pInAddr->S_un.S_un_b.s_b3, pInAddr->S_un.S_un_b.s_b4);
            }
        }

        if (MBB_IP_IS_IPV6_AVAILABLE(IpAddressInfo))
        {
            PMBB_IPV6_ADDRESS pIPV6Elem = (PMBB_IPV6_ADDRESS)(InBuffer + IpAddressInfo->IPv6AddressOffset);
            for (unsigned int i = 0; i < IpAddressInfo->IPv6AddressCount; i++, pIPV6Elem++)
            {
                TraceInfo(  WMBCLASS_IP, "CID_IP_CONFIGURATION: IPv6 #%d (prefixLen %d): %!IPV6ADDR!",
                            i + 1, pIPV6Elem->OnLinkPrefixLength, pIPV6Elem->IPV6Address);
            }
        }

        if (MBB_IP_IS_GWV6_AVAILABLE(IpAddressInfo))
        {
            PIN6_ADDR pIn6Addr = (PIN6_ADDR)(InBuffer + IpAddressInfo->IPv6GatewayOffset);
            TraceInfo(  WMBCLASS_IP, "CID_IP_CONFIGURATION: GWv6: %!IPV6ADDR!", pIn6Addr->u.Byte);
        }

        if (MBB_IP_IS_DSV6_AVAILABLE(IpAddressInfo))
        {
            PIN6_ADDR pIn6Addr = (PIN6_ADDR)(InBuffer + IpAddressInfo->IPv6DnsServerOffset);
            for (unsigned int i = 0; i < IpAddressInfo->IPv6DnsServerCount; i++, pIn6Addr++)
            {
                TraceInfo(  WMBCLASS_IP, "CID_IP_CONFIGURATION: DNSv6 #%d: %!IPV6ADDR!", i+1, pIn6Addr->u.Byte);
            }
        }

        //
        // Copy the received info.
        //
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "By Design: Allocate IP address info, only one instance exists, released if new IP info comes.")
        if( (IpAddressInfo = ALLOCATE_NONPAGED_POOL( InBufferSize )) == NULL )
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] CID_IP_CONFIGURATION: FAILED to allocate IP Address Info", Adapter->NetLuid.Value );
            break;
        }
        RtlCopyMemory( IpAddressInfo, InBuffer, InBufferSize );

        MBB_ACQUIRE_PORT_LOCK(Port);

        if( Port->IpAddressInfo != NULL )
        {
            FREE_POOL( Port->IpAddressInfo );
        }
        Port->IpAddressInfo = IpAddressInfo;
        
        MBB_RELEASE_PORT_LOCK(Port);

        MbbIpQueueIPChange(
            &GlobalControl,
            &Adapter->NetLuid,
            MbbReqMgrIsUnsolicitedIndication( Request )?
                MbbIPChangeTypeDeviceConfigurationChange:
                MbbIPChangeTypeDeviceConfiguration,
                PortNumber
            );

        
    }
    while( FALSE );

    if(Port != NULL)
    {  
        // Remove the reference added in find
        Dereference(Port);
    }

    return NDIS_STATUS_SUCCESS;
}

__callback
VOID
MbbIpProcessChange(
    __in PVOID  Context1,
    __in PVOID  Context2,
    __in PVOID  Context3,
    __in PVOID  Context4
    )
{
    PMBB_REQUEST_CONTEXT        NextRequest;
//    PMINIPORT_DRIVER_CONTEXT    Driver = (PMINIPORT_DRIVER_CONTEXT)Context1;
    PMINIPORT_ADAPTER_CONTEXT   Adapter= (PMINIPORT_ADAPTER_CONTEXT)Context2;
    MBB_IP_CHANGE_TYPE          IPChangeType = (MBB_IP_CHANGE_TYPE)Context3;
    PMBB_PORT                   Port = (PMBB_PORT)Context4;
    NDIS_PORT_NUMBER            PortNumber = DEFAULT_NDIS_PORT_NUMBER;
    PMBB_IPADDRESS_ENTRY        IpTable = NULL;
    ULONG                       IpCount;
    PMBB_IPADDRESS_ENTRY        GatewayTable = NULL;
    ULONG                       GatewayCount;
    PMBB_IPADDRESS_ENTRY        DnsTable = NULL;
    ULONG                       DnsCount = 0;
    PMBB_IPADDRESS_ENTRY        LastIpTable;
    ULONG                       LastIpCount;
    PMBB_IPADDRESS_ENTRY        LastGatewayTable;
    ULONG                       LastGatewayCount;
    PMBB_IPADDRESS_ENTRY        LastDnsTable;
    ULONG                       LastDnsCount;
    BOOLEAN                     IsUserConfiguredIP = FALSE;
    MBB_CONNECTION_STATE        ConnectionState;
    NDIS_STATUS                 NdisStatus;
    WWAN_IP_ADDRESS_STATE       IpAddressState = {0};
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;

    TraceInfo( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] Processing %!MbbIPChangeType!", Adapter->NetLuid.Value, IPChangeType );

    do
    {       
        if(Port == NULL)
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] IP change queued on invalid ndis port", Adapter->NetLuid.Value);
            break;
        }       

        //
        // Get current IP information
        //

        MBB_ACQUIRE_PORT_LOCK(Port);

        PortNumber = Port->PortNumber;

        MbbIpSnapPortIpTable(
            Port,
            &IpTable,
            &IpCount
            );

        MbbIpSnapPortAddressTable(
            Port,
            MbbAddressTableGateway,
            &GatewayTable,
            &GatewayCount
            );
        //
        // Only evaluate DNS changes when the device reports a change.
        //
        if( IPChangeType == MbbIPChangeTypeDeviceConfiguration ||
            IPChangeType == MbbIPChangeTypeDeviceConfigurationChange )
        {
            MbbIpSnapPortAddressTable(
                Port,
                MbbAddressTableDns,
                &DnsTable,
                &DnsCount
                );
        }

        Status = MbbUtilPopulateWwanIPAddressState(
                    Port->IpAddressInfo,
                    IpTable,
                    GatewayTable,
                    DnsTable,
                    IpCount,
                    GatewayCount,
                    DnsCount,
                    &IpAddressState);

        MBB_RELEASE_PORT_LOCK(Port);

        if(Status != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] IP change queued for disconnected port", Adapter->NetLuid.Value);
            break;
        }
        
        // For DEFAULT_NDIS_PORT ( i.e Port 0 ) update the IP address information 
        // For non default ports we send the indication upto the virtual miniport.
        // The virtual miniport then updates its IP address information in the registry
        
        if(IS_ALLOCATED_PORT_NUMBER(PortNumber))
        { 
            MbbPortSetIpAddressState(
                Adapter,
                &IpAddressState,
                PortNumber);
        }
        else
        {
            // Report the information in order to the IP stack for default port        
            WwanUpdateIPStack(
                Adapter->MiniportAdapterHandle,
                &IpAddressState,
                Adapter->NetLuid,
                Adapter->NetCfgId,
                Adapter->IfIndex,
                (IPChangeType == MbbIPChangeTypeDeviceConfiguration 
                    || IPChangeType == MbbIPChangeTypeDeviceConfigurationChange)
                );
        }
       
        //
        // Indicate LinkUP to NDIS.
        // In case of successful connect request this is done even when IP info was not reported to IP successfully
        // since the CONNECT OID is already completed with success. Worst case is that the adapter will not
        // have an IP info.
        //
        if( IPChangeType == MbbIPChangeTypeDeviceConfiguration )
        {
            ConnectionState.ConnectionUp = MediaConnectStateConnected;

            ConnectionState.UpStreamBitRate     = Adapter->UplinkSpeed;
            ConnectionState.DownStreamBitRate   = Adapter->DownlinkSpeed;

            MbbAdapterSetConnectionState(
                Adapter,
                &ConnectionState,
                PortNumber
                );
        }
    }
    while( FALSE );
    
    if(DnsTable!= NULL)
    {
         FREE_POOL( DnsTable );
         DnsTable = NULL;
    }
        
    if(IpTable!= NULL)
    {
         FREE_POOL( IpTable );
         IpTable = NULL;
    } 
    
    if(GatewayTable!= NULL)
    {
         FREE_POOL( GatewayTable );
         GatewayTable = NULL;
    }

    if(IpAddressState.IpTable)
    {
        FREE_POOL(IpAddressState.IpTable);
        IpAddressState.IpTable = NULL;
    }

    if(IpAddressState.GatewayTable)
    {
        FREE_POOL(IpAddressState.GatewayTable);
        IpAddressState.GatewayTable = NULL;
    }

    if(IpAddressState.DnsTable)
    {
        FREE_POOL(IpAddressState.DnsTable);
        IpAddressState.DnsTable = NULL;
    }

    if(Port != NULL)
    {
        Dereference(Port);
    }
    
    MbbAdapterDeref( Adapter );    
}


__callback
VOID
MbbIpInterfaceChangeCallback(
    __in PVOID                      CallerContext,
    __in_opt PMIB_IPINTERFACE_ROW   Row,
    __in MIB_NOTIFICATION_TYPE      NotificationType
    )
{
    NET_LUID                    NetLuid;
    PMINIPORT_DRIVER_CONTEXT    Driver = (PMINIPORT_DRIVER_CONTEXT)CallerContext;

    switch( NotificationType )
    {
        case MibInitialNotification :
        {
            TraceInfo( WMBCLASS_IP, "[MbbIP] Successfully regsitered for interface change notification" );
        }
        break;

        case MibAddInstance : __fallthrough;
        case MibDeleteInstance : __fallthrough;
        case MibParameterNotification : __fallthrough;
        {
            ASSERT(Row != NULL);

            __analysis_assume(Row != NULL);
            MbbIpQueueIPChange(
                Driver,
                &Row->InterfaceLuid,
                MbbIPChangeTypeHostInterfaceChange,
                DEFAULT_NDIS_PORT_NUMBER // indicate on default NDIS port, the adapter may indicate it to the miniport if needed
                );
        }
        break;
    }
}

__callback
VOID
MbbIpUnicastAddressChangeCallback(
    _In_ PVOID                          CallerContext,
    _In_opt_ PMIB_UNICASTIPADDRESS_ROW  Row,
    _In_ MIB_NOTIFICATION_TYPE          NotificationType
    )
{
    PMINIPORT_DRIVER_CONTEXT    Driver = (PMINIPORT_DRIVER_CONTEXT)CallerContext;

    if (Row != NULL)
    {
        MbbIpQueueIPChange(
            Driver,
            &Row->InterfaceLuid,
            MbbIPChangeTypeHostIPChange,        
            DEFAULT_NDIS_PORT_NUMBER // indicate on default NDIS port, the adapter may indicate it to the miniport if needed        
            );
    }
    else
    {
        ASSERT(NotificationType == MibInitialNotification);
    }
}

__callback
VOID
MbbIpRouteChangeCallback(
    _In_ PVOID                      CallerContext,
    _In_opt_ PMIB_IPFORWARD_ROW2    Row,
    _In_ MIB_NOTIFICATION_TYPE      NotificationType
    )
{
    PMINIPORT_DRIVER_CONTEXT    Driver = (PMINIPORT_DRIVER_CONTEXT)CallerContext;

    if (Row != NULL)
    {
        if (NotificationType != MibDeleteInstance)
        {
            MbbIpQueueIPChange(
                Driver,
                &Row->InterfaceLuid,
                MbbIPChangeTypeHostRouteChange,
                DEFAULT_NDIS_PORT_NUMBER // indicate on default NDIS port, the adapter may indicate it to the miniport if needed
                );
        }
    }
    else
    {
        ASSERT(NotificationType == MibInitialNotification);
    }
}

NDIS_STATUS
MbbIpQueueIPChange(
    __in PMINIPORT_DRIVER_CONTEXT   Driver,
    __in PNET_LUID                  NetLuid,
    __in MBB_IP_CHANGE_TYPE         IPChangeType,
    __in NDIS_PORT_NUMBER           PortNumber
    )
{
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_SUCCESS;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;
    PMBB_PORT                   Port = NULL;
 
    if( (Adapter = MbbDriverFindAdapterByNetLuid(
                        Driver,
                        NetLuid
                        )) != NULL )
    {
        Port = MbbWwanTranslatePortNumberToPort(
                        Adapter,
                        PortNumber);        
       
        if(Port)
        {
            MBB_ACQUIRE_PORT_LOCK(Port);
            
            if( IPChangeType == MbbIPChangeTypeDeviceConfiguration ||
                Port->ConnectionState.ConnectionUp == TRUE )
            {
                // Add reference to the port before queuing it up in the workitem.
                // The workitem routine will cleanup the reference

                // Adapter find reference is reused and cleared insider the worker thread
                Reference(Port);
                if( (NdisStatus = MbbWorkMgrQueueWorkItem(
                        Driver->IpWorkItemManagerHandle,
                        Driver,
                        Adapter,
                        (PVOID)IPChangeType,
                        (PVOID)Port,
                        MbbIpProcessChange
                        )) != NDIS_STATUS_SUCCESS )
                {
                    // Remove the reference added for queuing
                    Dereference(Port);   
                    TraceError( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] FAILED to queue work item to process %!MbbIPChangeType! with status=%!status!.", NetLuid->Value, IPChangeType, NdisStatus );
                }
                else
                {   
                    TraceInfo( WMBCLASS_IP, "[MbbIP][Luid=0x%I64x] Queuing %!MbbIPChangeType!", NetLuid->Value, IPChangeType );
                }
            }
            else
            {
                NdisStatus = NDIS_STATUS_MEDIA_DISCONNECT;
            }

            MBB_RELEASE_PORT_LOCK(Port);
        }
        else
        {
            NdisStatus = NDIS_STATUS_INVALID_PORT;
        }
    }
    else
    {
        NdisStatus = NDIS_STATUS_ADAPTER_NOT_FOUND;
    }

    if(Port!= NULL)
    {
        //Remove the reference on the port due to find
        Dereference(Port);
    }
    
    //
    // If successful, adapter and port would be derefed in MbbIpProcessChange
    //
    if( NdisStatus != NDIS_STATUS_SUCCESS )
    { 
        if( Adapter != NULL )
        {
            MbbAdapterDeref( Adapter );
        }
    }
    return NdisStatus;
}

NDIS_STATUS
MbbIpInitialize(
    __in PMINIPORT_DRIVER_CONTEXT Driver
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    do
    {
        //
        // Work item manager
        //
        if( (Driver->IpWorkItemManagerHandle = MbbWorkMgrInitialize( MBB_PREALLOC_IP_WORKITEM_COUNT )) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbIP] FAILED to initialize work item manager" );
            NtStatus = NDIS_STATUS_FAILURE;
            break;
        }
        //
        // Register with IP Helper for IP change notifications.
        //
        if( (NtStatus = NotifyIpInterfaceChange(
                            AF_UNSPEC,
                            MbbIpInterfaceChangeCallback,
                            Driver,
                            TRUE,
                            &Driver->IpInterfaceNotificationHandle
                            ))!= STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbIP] FAILED to register for Interface change notifications, status=%!STATUS!", NtStatus );
            NtStatus = NDIS_STATUS_FAILURE;
            break;
        }

        if( (NtStatus = NotifyRouteChange2(
                            AF_UNSPEC,
                            MbbIpRouteChangeCallback,
                            Driver,
                            TRUE,
                            &Driver->IpRouteNotificationHandle
                            )) != NO_ERROR )
        {
            TraceError( WMBCLASS_INIT, "[MbbIP] FAILED to register for Route change notifications, status=%!STATUS!", NtStatus );
            NtStatus = NDIS_STATUS_FAILURE;
            break;
        }

        if( (NtStatus = NotifyUnicastIpAddressChange(
                            AF_UNSPEC,
                            MbbIpUnicastAddressChangeCallback,
                            Driver,
                            TRUE,
                            &Driver->IpUnicastAddressNotificationHandle
                            )) != NO_ERROR )
        {
            TraceError( WMBCLASS_INIT, "[MbbIP] FAILED to register for Unicast address change notifications, status=%!STATUS!", NtStatus );
            NtStatus = NDIS_STATUS_FAILURE;
            break;
        }
    }
    while( FALSE );
    //
    // Cleanup on error
    //
    if( NtStatus != NDIS_STATUS_SUCCESS )
    {
        MbbIpCleanup( Driver );
    }
    else
    {
        TraceInfo( WMBCLASS_INIT, "[MbbIP] IP subsystem initialized" );
    }

    return (NDIS_STATUS)NtStatus;
}

VOID
MbbIpCleanup(
    __in PMINIPORT_DRIVER_CONTEXT Driver
    )
{
    TraceInfo( WMBCLASS_INIT, "[MbbIP] IP subsystem clean up" );

    if( Driver->IpUnicastAddressNotificationHandle != NULL )
    {
        CancelMibChangeNotify2( Driver->IpUnicastAddressNotificationHandle );
        Driver->IpUnicastAddressNotificationHandle = NULL;
    }

    if( Driver->IpRouteNotificationHandle != NULL )
    {
        CancelMibChangeNotify2( Driver->IpRouteNotificationHandle );
        Driver->IpRouteNotificationHandle = NULL;
    }

    if( Driver->IpInterfaceNotificationHandle != NULL )
    {
        CancelMibChangeNotify2( Driver->IpInterfaceNotificationHandle );
        Driver->IpInterfaceNotificationHandle = NULL;
    }

    if( Driver->IpWorkItemManagerHandle != NULL )
    {
        MbbWorkMgrCleanup( Driver->IpWorkItemManagerHandle );
        Driver->IpWorkItemManagerHandle = NULL;
    }
}
