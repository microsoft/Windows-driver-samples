/*++
Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    miniport.c

Abstract:

    NDIS Miniport Entry points and utility functions for the NDIS
    MUX Intermediate Miniport sample. The driver exposes zero or more
    Virtual Ethernet LANs (VELANs) as NDIS miniport instances over
    each lower (protocol-edge) binding to an underlying adapter.

Environment:

    Kernel mode.

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define MODULE_NUMBER           MODULE_MINI

NDIS_OID VElanSupportedOids[] =
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_TRANSMIT_QUEUE_LENGTH,
    OID_GEN_STATISTICS,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_802_3_XMIT_DEFERRED,
    OID_802_3_XMIT_MAX_COLLISIONS,
    OID_802_3_RCV_OVERRUN,
    OID_802_3_XMIT_UNDERRUN,
    OID_802_3_XMIT_HEARTBEAT_FAILURE,
    OID_802_3_XMIT_TIMES_CRS_LOST,
    OID_802_3_XMIT_LATE_COLLISIONS,
    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ADD_WAKE_UP_PATTERN,
    OID_PNP_REMOVE_WAKE_UP_PATTERN,
#if IEEE_VLAN_SUPPORT
    OID_GEN_VLAN_ID,
#endif    
    OID_PNP_ENABLE_WAKE_UP
};


NDIS_STATUS
MPInitialize(
    IN  NDIS_HANDLE                     MiniportAdapterHandle,
    IN  NDIS_HANDLE                     MiniportDriverContext,
    IN  PNDIS_MINIPORT_INIT_PARAMETERS  MiniportInitParameters
    )
/*++

Routine Description:

    This is the Miniport Initialize routine which gets called as a
    result of our call to NdisIMInitializeDeviceInstanceEx.
    The context parameter which we pass there is the VELan structure
    which we retrieve here.

Arguments:

    MiniportAdapterHandle       NDIS handle for this miniport
    MiniportDriverContext       Handle passed to NDIS when we registered the driver
    MiniportInitParameters      Miniport initialization parameters such
                                as our device context, resources, etc.

Return Value:

    NDIS_STATUS_SUCCESS unless something goes wrong

--*/
{
    PVELAN                          pVElan;
    UINT                            i;
    NDIS_STATUS                     Status = NDIS_STATUS_FAILURE;
    NDIS_HANDLE                     ConfigurationHandle;
    PVOID                           NetworkAddress;
    LOCK_STATE                      LockState;
    NET_IFINDEX                     HigherLayerIfIndex, LowerLayerIfIndex;
    NDIS_MINIPORT_ADAPTER_ATTRIBUTES MiniportAttributesContent;
    const PNDIS_MINIPORT_ADAPTER_ATTRIBUTES  MiniportAttributes = &MiniportAttributesContent;
    
 

#if IEEE_VLAN_SUPPORT
    NDIS_STRING                     strVlanId = NDIS_STRING_CONST("VlanID");
    PNDIS_CONFIGURATION_PARAMETER   Params;
#endif


    //NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES   RegistrationAttributes;
    //NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES        GeneralAttributes;
    NDIS_CONFIGURATION_OBJECT                       ConfigObject;


    UNREFERENCED_PARAMETER(MiniportDriverContext);
    
    //
    // Start off by retrieving our virtual miniport context (VELAN) and 
    // storing the Miniport handle in it.
    //

    pVElan = MiniportInitParameters->IMDeviceInstanceContext;

    DBGPRINT(MUX_LOUD, ("==> MPInitialize: VELAN %p\n", pVElan));

    ASSERT(pVElan != NULL);
    ASSERT(pVElan->pAdapt != NULL);
    NdisZeroMemory(MiniportAttributes,sizeof(NDIS_MINIPORT_ADAPTER_ATTRIBUTES));

    do
    {
        pVElan->MiniportAdapterHandle = MiniportAdapterHandle;
        //
        // Create an ioctl interface
        //
        (VOID)PtRegisterDevice();

        //
        // register this miniport with NDIS
        //

        //NdisZeroMemory(&RegistrationAttributes, sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES));
        //NdisZeroMemory(&GeneralAttributes, sizeof(NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES));

        //
        // setting registration attributes
        //
        MiniportAttributesContent.RegistrationAttributes.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
        MiniportAttributesContent.RegistrationAttributes.Header.Revision = NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;
        MiniportAttributesContent.RegistrationAttributes.Header.Size = sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES);

        MiniportAttributesContent.RegistrationAttributes.MiniportAdapterContext = (NDIS_HANDLE)pVElan;


        MiniportAttributesContent.RegistrationAttributes.AttributeFlags = NDIS_MINIPORT_ATTRIBUTES_NO_HALT_ON_SUSPEND;

        MiniportAttributesContent.RegistrationAttributes.CheckForHangTimeInSeconds = 0;
        MiniportAttributesContent.RegistrationAttributes.InterfaceType = 0;
        

        
        NDIS_DECLARE_MINIPORT_ADAPTER_CONTEXT(VELAN);
        Status = NdisMSetMiniportAttributes(MiniportAdapterHandle,
                                            MiniportAttributes);

        

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
        

        //
        // Access configuration parameters for this miniport.
        //
        ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
        ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
        ConfigObject.Header.Size = sizeof(NDIS_CONFIGURATION_OBJECT);
        ConfigObject.NdisHandle = pVElan->MiniportAdapterHandle;
        ConfigObject.Flags = 0;

        Status = NdisOpenConfigurationEx(
                    &ConfigObject,
                    &ConfigurationHandle);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }


        NdisReadNetworkAddress(
            &Status,
            &NetworkAddress,
            &i,
            ConfigurationHandle);

        //
        // If there is a NetworkAddress override, use it 
        //
        if (((Status == NDIS_STATUS_SUCCESS) 
                && (i == ETH_LENGTH_OF_ADDRESS))
                && ((!ETH_IS_MULTICAST(NetworkAddress)) 
                && (ETH_IS_LOCALLY_ADMINISTERED (NetworkAddress))))
        {
            
            ETH_COPY_NETWORK_ADDRESS(
                        pVElan->CurrentAddress,
                        NetworkAddress);
        }
        else
        {
            MPGenerateMacAddr(pVElan);
        }

        //
        // ignore error reading the network address
        //
        Status = NDIS_STATUS_SUCCESS;
   
#if IEEE_VLAN_SUPPORT
        //
        // Read VLAN ID
        //
        NdisReadConfiguration(
                &Status,
                &Params,
                ConfigurationHandle,
                &strVlanId,
                NdisParameterInteger);
        if (Status == NDIS_STATUS_SUCCESS)
        {
            //
            // Check for out of bound
            //
            if (Params->ParameterData.IntegerData > VLAN_ID_MAX)
            {
                pVElan->VlanId = VLANID_DEFAULT;
            }
            else
            {
                pVElan->VlanId = Params->ParameterData.IntegerData;
            }
        }

        else
        {
            
            pVElan->VlanId = VLANID_DEFAULT;
            Status = NDIS_STATUS_SUCCESS;
        }
#endif    


        NdisCloseConfiguration(ConfigurationHandle);



        //
        // set up generic attributes
        //

        MiniportAttributesContent.GeneralAttributes.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES;
        MiniportAttributesContent.GeneralAttributes.Header.Revision = NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES_REVISION_1;
        MiniportAttributesContent.GeneralAttributes.Header.Size = sizeof(NDIS_MINIPORT_ADAPTER_GENERAL_ATTRIBUTES);   

        MiniportAttributesContent.GeneralAttributes.MediaType = VELAN_MEDIA_TYPE;
        MiniportAttributesContent.GeneralAttributes.MtuSize = pVElan->pAdapt->BindParameters.MtuSize;
        MiniportAttributesContent.GeneralAttributes.MaxXmitLinkSpeed = pVElan->pAdapt->BindParameters.MaxXmitLinkSpeed;
        MiniportAttributesContent.GeneralAttributes.MaxRcvLinkSpeed = pVElan->pAdapt->BindParameters.MaxRcvLinkSpeed;
        MiniportAttributesContent.GeneralAttributes.XmitLinkSpeed = pVElan->pAdapt->BindParameters.XmitLinkSpeed;
        MiniportAttributesContent.GeneralAttributes.RcvLinkSpeed = pVElan->pAdapt->BindParameters.RcvLinkSpeed;
        

        MUX_ACQUIRE_ADAPT_READ_LOCK(pVElan->pAdapt, &LockState);
        
        //
        // Miniport below has indicated some status indication
        //
        MiniportAttributesContent.GeneralAttributes.MediaConnectState = pVElan->pAdapt->LastIndicatedLinkState.MediaConnectState;
        MiniportAttributesContent.GeneralAttributes.MediaDuplexState = pVElan->pAdapt->LastIndicatedLinkState.MediaDuplexState;
        MiniportAttributesContent.GeneralAttributes.XmitLinkSpeed = pVElan->pAdapt->LastIndicatedLinkState.XmitLinkSpeed;
        MiniportAttributesContent.GeneralAttributes.RcvLinkSpeed = pVElan->pAdapt->LastIndicatedLinkState.RcvLinkSpeed;
        
        pVElan->LastIndicatedStatus = NDIS_STATUS_LINK_STATE;

        pVElan->LastIndicatedLinkState = pVElan->pAdapt->LastIndicatedLinkState;

        
        MiniportAttributesContent.GeneralAttributes.LookaheadSize = pVElan->pAdapt->BindParameters.LookaheadSize;
        MiniportAttributesContent.GeneralAttributes.MaxMulticastListSize = pVElan->pAdapt->BindParameters.MaxMulticastListSize;
        MiniportAttributesContent.GeneralAttributes.MacAddressLength = pVElan->pAdapt->BindParameters.MacAddressLength;
        
        MiniportAttributesContent.GeneralAttributes.PhysicalMediumType = pVElan->pAdapt->BindParameters.PhysicalMediumType ;
        MiniportAttributesContent.GeneralAttributes.AccessType = pVElan->pAdapt->BindParameters.AccessType ; 
        MiniportAttributesContent.GeneralAttributes.DirectionType = pVElan->pAdapt->BindParameters.DirectionType;         
        MiniportAttributesContent.GeneralAttributes.ConnectionType = pVElan->pAdapt->BindParameters.ConnectionType ; 
        MiniportAttributesContent.GeneralAttributes.IfType = pVElan->pAdapt->BindParameters.IfType ; 
        MiniportAttributesContent.GeneralAttributes.IfConnectorPresent = FALSE; // RFC 2665 TRUE if physical adapter

        if (pVElan->pAdapt->BindParameters.RcvScaleCapabilities)
        {
            MiniportAttributesContent.GeneralAttributes.RecvScaleCapabilities = pVElan->pAdapt->BindParameters.RcvScaleCapabilities;
        }
        else
        {
            MiniportAttributesContent.GeneralAttributes.RecvScaleCapabilities = NULL;
        }
        
        MiniportAttributesContent.GeneralAttributes.MacOptions = NDIS_MAC_OPTION_NO_LOOPBACK;


#if IEEE_VLAN_SUPPORT
        MiniportAttributesContent.GeneralAttributes.MacOptions |= (NDIS_MAC_OPTION_8021P_PRIORITY |
                                        NDIS_MAC_OPTION_8021Q_VLAN);

#endif
        
        MiniportAttributesContent.GeneralAttributes.SupportedPacketFilters = pVElan->pAdapt->BindParameters.SupportedPacketFilters;

        MiniportAttributesContent.GeneralAttributes.SupportedStatistics = NDIS_STATISTICS_XMIT_OK_SUPPORTED |
                                                NDIS_STATISTICS_RCV_OK_SUPPORTED |
                                                NDIS_STATISTICS_XMIT_ERROR_SUPPORTED |
                                                NDIS_STATISTICS_RCV_ERROR_SUPPORTED |
                                                NDIS_STATISTICS_RCV_CRC_ERROR_SUPPORTED |
                                                NDIS_STATISTICS_RCV_NO_BUFFER_SUPPORTED |
                                                NDIS_STATISTICS_TRANSMIT_QUEUE_LENGTH_SUPPORTED |
                                                NDIS_STATISTICS_GEN_STATISTICS_SUPPORTED;

        
        NdisMoveMemory(&MiniportAttributesContent.GeneralAttributes.CurrentMacAddress,
                       &pVElan->CurrentAddress,
                       ETH_LENGTH_OF_ADDRESS);

        NdisMoveMemory(&MiniportAttributesContent.GeneralAttributes.PermanentMacAddress,
                       &pVElan->PermanentAddress,
                       ETH_LENGTH_OF_ADDRESS);

        MiniportAttributesContent.GeneralAttributes.PowerManagementCapabilities = &pVElan->pAdapt->PowerManagementCapabilities;
        MiniportAttributesContent.GeneralAttributes.SupportedOidList = VElanSupportedOids;
        MiniportAttributesContent.GeneralAttributes.SupportedOidListLength = sizeof(VElanSupportedOids);
        MUX_RELEASE_ADAPT_READ_LOCK(pVElan->pAdapt, &LockState);
        Status = NdisMSetMiniportAttributes(MiniportAdapterHandle,MiniportAttributes);

        pVElan->MiniportInitPending = FALSE;
    } while (FALSE);

    
    //
    // If we had received an UnbindAdapter notification on the underlying
    // adapter, we would have blocked that thread waiting for the IM Init
    // process to complete. Wake up any such thread.
    //
    // See PtUnbindAdapter for more details.
    //
    //

    if (Status == NDIS_STATUS_SUCCESS)
    {
        //
        // we should set this to FALSE only if we successfully initialized the adapter
        // otherwise the unbind routine will wait forever for this VElan to go away
        //
        pVElan->MiniportInitPending = FALSE;
        //
        // Save the IfIndex for this VELAN
        //
        HigherLayerIfIndex = MiniportInitParameters->IfIndex;
        LowerLayerIfIndex = pVElan->pAdapt->BindParameters.BoundIfIndex;
            
        Status = NdisIfAddIfStackEntry(HigherLayerIfIndex,
                                       LowerLayerIfIndex);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            pVElan->IfIndex = HigherLayerIfIndex;                
        }

        //
        // Ignore if the add fails
        //
        Status = NDIS_STATUS_SUCCESS;

    }
    else
    {        
        pVElan->MiniportAdapterHandle = NULL;
    }

    if (pVElan->MiniportInitPending == TRUE)
    {
        pVElan->MiniportInitPending = FALSE;
    }
    
    // TODO: check to see if we can set the init event in a failure case?
    
    NdisSetEvent(&pVElan->MiniportInitEvent);

    DBGPRINT(MUX_LOUD, ("<== MPInitialize: VELAN %p, Status %x\n", pVElan, Status));

    return Status;
}

NDIS_STATUS
MPQueryInformation(
    IN    PVELAN                    pVElan,
    IN    PNDIS_OID_REQUEST         NdisRequest
    )
/*++

Routine Description:

    This function is called to handle the query request specified by NdisRequest
    All query requests are first handled right here, since this is a virtual
    device (not pass-through).

Arguments:

    MiniportAdapterContext      Pointer to the adapter structure
    NdisRequest                 Specify the query request.

Return Value:

    NDIS_STATUS_SUCCESS         
    NDIS_STATUS_NOT_SUPPORTED
    Return code from the MPForwardOidRequest below.

--*/
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    NDIS_HARDWARE_STATUS        HardwareStatus = NdisHardwareStatusReady;
    NDIS_MEDIUM                 Medium = VELAN_MEDIA_TYPE;
    UCHAR                       VendorDesc[] = VELAN_VENDOR_DESC;
    ULONG                       ulInfo;
    ULONG64                     ulInfo64;
    USHORT                      usInfo;
    PVOID                       pInfo = (PVOID)&ulInfo;
    ULONG                       ulInfoLen = sizeof(ulInfo), NeededLength = 0;
    // Should we forward the request to the miniport below?
    BOOLEAN                     bForwardRequest = FALSE;

    NDIS_OID                    Oid;
    PVOID                       InformationBuffer;
    ULONG                       InformationBufferLength;
    PULONG                      BytesWritten;
    PULONG                      BytesNeeded;
    NDIS_STATISTICS_INFO        StatisticsInfo;


    DBGPRINT(MUX_LOUD, ("==> MPQueryInformation: VElan %p, Request %p\n",pVElan, NdisRequest));
    
    Oid = NdisRequest->DATA.QUERY_INFORMATION.Oid;
    InformationBuffer = NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;
    InformationBufferLength = NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
    BytesWritten = (ULONG*) &(NdisRequest->DATA.QUERY_INFORMATION.BytesWritten);
    BytesNeeded = (ULONG*) &(NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded);
    
    
    // Initialize the result
    *BytesWritten = 0;
    *BytesNeeded = 0;

    switch (Oid)
    {
        case OID_GEN_SUPPORTED_LIST:
            pInfo = (PVOID) VElanSupportedOids;
            ulInfoLen = sizeof(VElanSupportedOids);
            break;

        case OID_GEN_SUPPORTED_GUIDS:
            //
            // Do NOT forward this down, otherwise we will
            // end up with spurious instances of private WMI
            // classes supported by the lower driver(s).
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;

        case OID_GEN_HARDWARE_STATUS:
            pInfo = (PVOID) &HardwareStatus;
            ulInfoLen = sizeof(NDIS_HARDWARE_STATUS);
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
            pInfo = (PVOID) &Medium;
            ulInfoLen = sizeof(NDIS_MEDIUM);
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_MAXIMUM_LOOKAHEAD:         
            ulInfo = pVElan->LookAhead;
            pInfo = (PVOID) &ulInfo;
            break;            
            
        case OID_GEN_MAXIMUM_FRAME_SIZE:
            ulInfo = ETH_MAX_PACKET_SIZE - ETH_HEADER_SIZE;
#if IEEE_VLAN_SUPPORT
            ulInfo -= VLAN_TAG_HEADER_SIZE;
            
#endif
            pInfo = (PVOID) &ulInfo;
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
            ulInfo = (ULONG) ETH_MAX_PACKET_SIZE;
#if IEEE_VLAN_SUPPORT
            ulInfo -= VLAN_TAG_HEADER_SIZE;
#endif    
            pInfo = (PVOID) &ulInfo;
            break;
            
        case OID_GEN_MAC_OPTIONS:
            ulInfo = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | 
                     NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                     NDIS_MAC_OPTION_NO_LOOPBACK;
#if IEEE_VLAN_SUPPORT
            ulInfo |= (NDIS_MAC_OPTION_8021P_PRIORITY |
                       NDIS_MAC_OPTION_8021Q_VLAN);        
#endif
            pInfo = (PVOID) &ulInfo;
            break;

        case OID_GEN_LINK_SPEED:
            bForwardRequest = TRUE;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            ulInfo = ETH_MAX_PACKET_SIZE * pVElan->MaxBusySends;
#if IEEE_VLAN_SUPPORT
            ulInfo -= VLAN_TAG_HEADER_SIZE * pVElan->MaxBusySends;
#endif          
            pInfo = (PVOID) &ulInfo;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            ulInfo = ETH_MAX_PACKET_SIZE * pVElan->MaxBusyRecvs;
#if IEEE_VLAN_SUPPORT
            ulInfo -= VLAN_TAG_HEADER_SIZE * pVElan->MaxBusyRecvs;        
#endif            
             pInfo = (PVOID) &ulInfo;
            break;

        case OID_GEN_VENDOR_ID:
            ulInfo = VELAN_VENDOR_ID;
            pInfo = (PVOID) &ulInfo;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            pInfo = VendorDesc;
            ulInfoLen = sizeof(VendorDesc);
            break;
            
        case OID_GEN_VENDOR_DRIVER_VERSION:
            ulInfo = VELAN_VENDOR_ID;
            pInfo = (PVOID) &ulInfo;
            break;

        case OID_GEN_DRIVER_VERSION:
            usInfo = (USHORT) VELAN_DRIVER_VERSION;
            pInfo = (PVOID) &usInfo;
            ulInfoLen = sizeof(USHORT);
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            pInfo = pVElan->PermanentAddress;
            ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            pInfo = pVElan->CurrentAddress;
            ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            ulInfo = VELAN_MAX_MCAST_LIST;
            pInfo = (PVOID) &ulInfo;
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            ulInfo = VELAN_MAX_SEND_PKTS;
            pInfo = (PVOID) &ulInfo;
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:
            //
            // Get this from the adapter below.
            //
            bForwardRequest = TRUE;
            break;

        case OID_PNP_QUERY_POWER:
            // simply succeed this.
            ulInfoLen = sizeof(ULONG);
            ulInfo = 0;
            break;

        case OID_PNP_CAPABILITIES:
        case OID_PNP_WAKE_UP_PATTERN_LIST:
            //
            // Pass down these power management/PNP OIDs.
            //
            bForwardRequest = TRUE;
            break;

        case OID_GEN_XMIT_OK:
            ulInfo64 = pVElan->GoodTransmits;
            pInfo = &ulInfo64;
            if (InformationBufferLength >= sizeof(ULONG64) ||
                InformationBufferLength == 0)
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }
            NeededLength = sizeof(ulInfo64);

            break;
    
        case OID_GEN_RCV_OK:
            ulInfo64 = pVElan->GoodReceives;
            pInfo = &ulInfo64;
            if (InformationBufferLength >= sizeof(ULONG64) ||
                InformationBufferLength == 0)
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }

            NeededLength = sizeof(ulInfo64);
            
            break;
    
        case OID_GEN_XMIT_ERROR:
            ulInfo = pVElan->TxAbortExcessCollisions +
                pVElan->TxDmaUnderrun +
                pVElan->TxLostCRS +
                pVElan->TxLateCollisions+
                pVElan->TransmitFailuresOther;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_GEN_RCV_ERROR:
            ulInfo = pVElan->RcvCrcErrors +
                pVElan->RcvAlignmentErrors +
                pVElan->RcvDmaOverrunErrors +
                pVElan->RcvRuntErrors;
#if IEEE_VLAN_SUPPORT
            ulInfo +=
                (pVElan->RcvVlanIdErrors +
                pVElan->RcvFormatErrors);           
#endif
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_GEN_RCV_NO_BUFFER:
            ulInfo = pVElan->RcvResourceErrors;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_GEN_RCV_CRC_ERROR:
            ulInfo = pVElan->RcvCrcErrors;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
            ulInfo = pVElan->RegNumTcb;
            pInfo = (PVOID) &ulInfo;
            break;
        
        case OID_GEN_STATISTICS:
            ulInfoLen = sizeof (NDIS_STATISTICS_INFO);
            NdisZeroMemory(&StatisticsInfo, sizeof(NDIS_STATISTICS_INFO));

            StatisticsInfo.Header.Revision = NDIS_OBJECT_REVISION_1;
            StatisticsInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
            StatisticsInfo.Header.Size = sizeof(NDIS_STATISTICS_INFO);
            StatisticsInfo.SupportedStatistics = NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS          |
                                                 NDIS_STATISTICS_FLAGS_VALID_RCV_ERROR             |
                                                 NDIS_STATISTICS_FLAGS_VALID_XMIT_ERROR;
                    
            StatisticsInfo.ifInDiscards =
                (ULONG64)pVElan->RcvCrcErrors +
                (ULONG64)pVElan->RcvAlignmentErrors +
                (ULONG64)pVElan->RcvResourceErrors +
                (ULONG64)pVElan->RcvDmaOverrunErrors +
                (ULONG64)pVElan->RcvRuntErrors;

#if IEEE_VLAN_SUPPORT
            StatisticsInfo.ifInDiscards += ((ULONG64)pVElan->RcvVlanIdErrors + (ULONG64)pVElan->RcvFormatErrors);
#endif
            StatisticsInfo.ifInErrors = StatisticsInfo.ifInDiscards -
                (ULONG64)pVElan->RcvResourceErrors;

            StatisticsInfo.ifOutErrors = (ULONG64)pVElan->TxAbortExcessCollisions +
                (ULONG64)pVElan->TxDmaUnderrun +
                (ULONG64)pVElan->TxLostCRS +
                (ULONG64)pVElan->TxLateCollisions;

            pInfo = &StatisticsInfo;
            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:
            ulInfo = pVElan->RcvAlignmentErrors;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_802_3_XMIT_ONE_COLLISION:
        	ulInfo = pVElan->OneRetry;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_802_3_XMIT_MORE_COLLISIONS:
        	ulInfo = pVElan->MoreThanOneRetry;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_802_3_XMIT_DEFERRED:
        	ulInfo = pVElan->TxOKButDeferred;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_802_3_XMIT_MAX_COLLISIONS:
            ulInfo = pVElan->TxAbortExcessCollisions;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_802_3_RCV_OVERRUN:
            ulInfo = pVElan->RcvDmaOverrunErrors;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_802_3_XMIT_UNDERRUN:
            ulInfo = pVElan->TxDmaUnderrun;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
            ulInfo = pVElan->TxLostCRS;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_802_3_XMIT_TIMES_CRS_LOST:
            ulInfo = pVElan->TxLostCRS;
            pInfo = (PVOID) &ulInfo;
            break;
    
        case OID_802_3_XMIT_LATE_COLLISIONS:
            ulInfo = pVElan->TxLateCollisions;
            pInfo = (PVOID) &ulInfo;
            break;
   
#if IEEE_VLAN_SUPPORT            
        case OID_GEN_VLAN_ID:
            ulInfo = pVElan->VlanId;
            pInfo = (PVOID) &ulInfo;
            break;

#endif

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if (bForwardRequest == FALSE)
    {
        //
        // No need to forward this request down.
        //
        if (Status == NDIS_STATUS_SUCCESS)
        {
            if (ulInfoLen <= InformationBufferLength)
            {
                // Copy result into InformationBuffer
                *BytesWritten = ulInfoLen;
                if(ulInfoLen)
                {
                    NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
                    
                    if (NeededLength > ulInfoLen)
                    {
                        *BytesNeeded = NeededLength;
                    }
                }
             

            }
            else
            {
                // too short
                *BytesNeeded = (NeededLength > ulInfoLen ? NeededLength : ulInfoLen);
             
                Status = NDIS_STATUS_BUFFER_TOO_SHORT;
            }
        }
    }
    else
    {
     

        //
        // Send this request to the binding below.
        //
        Status = MPForwardOidRequest(pVElan,NdisRequest);
    }

    if ((Status != NDIS_STATUS_SUCCESS) &&
        (Status != NDIS_STATUS_PENDING))
    {
        DBGPRINT(MUX_WARN, ("MPQueryInformation VELAN %p, OID 0x%08x, Status = 0x%08x\n",
                    pVElan, Oid, Status));
    }

 

    DBGPRINT(MUX_LOUD, ("<== MPQueryInformation: VElan %p, Request %p returning %08lx\n",pVElan, NdisRequest, Status));
    
    return(Status);

}


NDIS_STATUS
MPSetInformation(
    IN    PVELAN                    pVElan,
    IN    PNDIS_OID_REQUEST         NdisRequest
    )
/*++

Routine Description:

    This is the handler for an set request operation. Relevant
    requests are forwarded down to the lower miniport for handling.

Arguments:

    MiniportAdapterContext      Pointer to the adapter structure
    NdisRequest                 Specify the set request

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_INVALID_LENGTH
    Return code from the MPForwardOidRequest below.

--*/
{
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    ULONG                   PacketFilter;
    NDIS_DEVICE_POWER_STATE NewDeviceState;

    NDIS_OID                Oid;
    PVOID                   InformationBuffer;
    ULONG                   InformationBufferLength;
    PULONG                  BytesRead;
    PULONG                  BytesNeeded;
    
    // Should we forward the request to the miniport below?
    BOOLEAN                 bForwardRequest = FALSE;
    NDIS_STATUS_INDICATION  StatusIndication;

    DBGPRINT(MUX_LOUD, ("==> MPSetInformation: VElan %p, Request %p\n", pVElan, NdisRequest));
    
    NdisZeroMemory(&StatusIndication, sizeof(NDIS_STATUS_INDICATION));
    Oid = NdisRequest->DATA.SET_INFORMATION.Oid;
    InformationBuffer = NdisRequest->DATA.SET_INFORMATION.InformationBuffer;
    InformationBufferLength = NdisRequest->DATA.SET_INFORMATION.InformationBufferLength;
    BytesRead = (ULONG*) &(NdisRequest->DATA.SET_INFORMATION.BytesRead);
    BytesNeeded = (ULONG*) &(NdisRequest->DATA.SET_INFORMATION.BytesNeeded);

    *BytesRead = 0;
    *BytesNeeded = 0;

    switch (Oid)
    {
        //
        // Let the miniport below handle these OIDs:
        //
        case OID_PNP_ADD_WAKE_UP_PATTERN:
        case OID_PNP_REMOVE_WAKE_UP_PATTERN:
        case OID_PNP_ENABLE_WAKE_UP:
            bForwardRequest = TRUE;
            break;

        case OID_PNP_SET_POWER:
            //
            // Store new power state and succeed the request.
            //
            *BytesNeeded = sizeof(NDIS_DEVICE_POWER_STATE);
            if (InformationBufferLength < *BytesNeeded)
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
           
            NewDeviceState = (*(PNDIS_DEVICE_POWER_STATE)InformationBuffer);
            
            //
            // Check if the VELAN adapter goes from lower power state to D0
            // 
            if ((MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState)) 
                    && (!MUX_IS_LOW_POWER_STATE(NewDeviceState)))
            {
                //
                // Indicate the media status is necessary
                // 
                if (pVElan->LastIndicatedStatus != pVElan->LatestUnIndicateStatus)
                {
                    
                    StatusIndication.Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
                    StatusIndication.Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
                    StatusIndication.Header.Size = sizeof(NDIS_STATUS_INDICATION);
                    
                    StatusIndication.SourceHandle = pVElan->MiniportAdapterHandle;
                    StatusIndication.StatusCode = pVElan->LatestUnIndicateStatus;
                    if (pVElan->LatestUnIndicateStatus == NDIS_STATUS_LINK_STATE)
                    {
                        StatusIndication.StatusBuffer = &pVElan->LatestUnIndicateLinkState;
                        StatusIndication.StatusBufferSize = sizeof(NDIS_LINK_STATE);
                            
                    }
                    else
                    {
                        StatusIndication.StatusBuffer = NULL;
                        StatusIndication.StatusBufferSize = 0;
                    }
                    
                    NdisMIndicateStatusEx(pVElan->MiniportAdapterHandle, &StatusIndication);
                                        
                    pVElan->LastIndicatedStatus = pVElan->LatestUnIndicateStatus;

                    if (pVElan->LatestUnIndicateStatus == NDIS_STATUS_LINK_STATE)
                    {
                        pVElan->LastIndicatedLinkState = pVElan->LatestUnIndicateLinkState;
                    }
                        
                }
                else
                {
                    if (pVElan->LastIndicatedStatus == NDIS_STATUS_LINK_STATE)
                    {
                        if (!NdisEqualMemory(&pVElan->LatestUnIndicateLinkState,
                                             &pVElan->LastIndicatedLinkState,
                                             sizeof(NDIS_LINK_STATE)))
                        {
                        
                            StatusIndication.Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
                            StatusIndication.Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
                            StatusIndication.Header.Size = sizeof(NDIS_STATUS_INDICATION);
                    
                            StatusIndication.SourceHandle = pVElan->MiniportAdapterHandle;
                            StatusIndication.StatusCode = pVElan->LatestUnIndicateStatus;
                            StatusIndication.StatusBuffer = &pVElan->LatestUnIndicateLinkState;
                            StatusIndication.StatusBufferSize = sizeof(NDIS_LINK_STATE);
                            
                            NdisMIndicateStatusEx(pVElan->MiniportAdapterHandle, &StatusIndication);

                            pVElan->LastIndicatedStatus = pVElan->LatestUnIndicateStatus;
                            pVElan->LastIndicatedLinkState = pVElan->LatestUnIndicateLinkState;
                        }
                    }
                } 
            }
            //
            // Check if the VELAN adapter goes from D0 to lower power state
            // 
            if ((!MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState)) 
                    && (MUX_IS_LOW_POWER_STATE(NewDeviceState)))
            {
                //
                //  Initialize LastUnIndicateStatus 
                // 
                pVElan->LatestUnIndicateStatus = pVElan->LastIndicatedStatus;

                if (pVElan->LastIndicatedStatus == NDIS_STATUS_LINK_STATE)
                {
                    pVElan->LatestUnIndicateLinkState = pVElan->LastIndicatedLinkState;
                }
            }
            
            NdisMoveMemory(&pVElan->MPDevicePowerState,
                           InformationBuffer,
                           *BytesNeeded);

            DBGPRINT(MUX_INFO, ("SetInfo: VElan %p, new miniport power state --- %d\n",
                    pVElan, pVElan->MPDevicePowerState));

            break;

        case OID_802_3_MULTICAST_LIST:
            Status = MPSetMulticastList(pVElan,
                                        InformationBuffer,
                                        InformationBufferLength,
                                        BytesRead,
                                        BytesNeeded);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            if (InformationBufferLength != sizeof(ULONG))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                *BytesNeeded = sizeof(ULONG);
                break;
            }

            NdisMoveMemory(&PacketFilter, InformationBuffer, sizeof(ULONG));
            *BytesRead = sizeof(ULONG);

            Status = MPSetPacketFilter(pVElan,
                                       PacketFilter);
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            if (InformationBufferLength < sizeof(ULONG))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                *BytesNeeded = sizeof(ULONG);
                break;
            }
#if IEEE_VLAN_SUPPORT
            //
            // In order to simplify parsing and to avoid excessive
            // copying, we need the tag header also to be present in the
            // lookahead buffer. Make sure that the driver below
            // includes that.
            //
            if (*(UNALIGNED PULONG)InformationBuffer < VLAN_TAG_HEADER_SIZE)
            {
                pVElan->RestoreLookaheadSize = TRUE;
                *(UNALIGNED PULONG)InformationBuffer += VLAN_TAG_HEADER_SIZE;
            }
#endif            
            bForwardRequest = TRUE;
            break;
            
#if IEEE_VLAN_SUPPORT
        case OID_GEN_VLAN_ID:
            if (InformationBufferLength == sizeof(ULONG))
            {
                NdisMoveMemory((&pVElan->VlanId), InformationBuffer, sizeof(ULONG));

            } 
            else
            {
                *BytesNeeded = sizeof(ULONG);
                 Status = NDIS_STATUS_INVALID_LENGTH;
            }

            break;
#endif
            
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;

    }
    
    if (bForwardRequest == FALSE)
    {
        if (Status == NDIS_STATUS_SUCCESS)
        {
            *BytesRead = InformationBufferLength;
        }
    }
    else
    {
        //
        // Send this request to the binding below.
        //
        Status = MPForwardOidRequest(pVElan,NdisRequest);
    }

    DBGPRINT(MUX_LOUD, ("<== MPSetInformation: VElan %p, Request %p returning %08lx\n",pVElan, NdisRequest, Status));
    
    return(Status);
}

NDIS_STATUS
MPMethodRequest(
    IN    PVELAN                  pVElan,
    IN    PNDIS_OID_REQUEST       NdisRequest
    )
/*++
Routine Description:

    WMI method request handler

Arguments:

    MiniportAdapterContext          Pointer to the adapter structure
    NdisRequest                     Pointer to the request sent down by NDIS

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
   

--*/
{
    NDIS_OID        Oid;
    ULONG           MethodId;
    PVOID           InformationBuffer;
    ULONG           InputBufferLength;
    ULONG           OutputBufferLength;
    ULONG           BytesNeeded;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pVElan);
    
    DBGPRINT(MUX_LOUD, ("==> MPMethodRequest: VElan %p, Request %p\n", pVElan, NdisRequest));


    Oid = NdisRequest->DATA.METHOD_INFORMATION.Oid;
    InformationBuffer = (PVOID)(NdisRequest->DATA.METHOD_INFORMATION.InformationBuffer);
    InputBufferLength = NdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
    OutputBufferLength = NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength;
    MethodId = NdisRequest->DATA.METHOD_INFORMATION.MethodId;
    UNREFERENCED_PARAMETER(Oid);
    UNREFERENCED_PARAMETER(InformationBuffer);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(MethodId);


    BytesNeeded = 0;

    switch(Oid)
    {
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }


    DBGPRINT(MUX_LOUD, ("<== MPMethodRequest: VElan %p, Request %p returning %08lx\n",pVElan, NdisRequest, Status));

    return Status;

}



NDIS_STATUS
MPOidRequest(
    IN    NDIS_HANDLE             MiniportAdapterContext,
    IN    PNDIS_OID_REQUEST       NdisRequest
    )
/*++
Routine Description:

    MiniportRequest dispatch handler

Arguments:

    MiniportAdapterContext      Pointer to the adapter structure
    NdisRequest                 Pointer to NDIS_OID_REQUEST sent down by NDIS.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_XXX

--*/
{
    PVELAN                  pVElan = (PVELAN)MiniportAdapterContext;
    NDIS_REQUEST_TYPE       RequestType;
    NDIS_STATUS             Status;

    DBGPRINT(MUX_LOUD,("==> MPOidRequest: Request %p\n", NdisRequest));

    RequestType = NdisRequest->RequestType;

    switch(RequestType)
    {
        case NdisRequestMethod:
            Status = MPMethodRequest(pVElan, NdisRequest);
            break;

        case NdisRequestSetInformation:
            Status = MPSetInformation(pVElan, NdisRequest);
            break;

        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
            Status = MPQueryInformation(pVElan, NdisRequest);
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    DBGPRINT(MUX_LOUD,("<== MPOidRequest: Request %p, Status %08lx\n", NdisRequest, Status));

    return Status;
}



VOID
MPHalt(
    IN    NDIS_HANDLE                MiniportAdapterContext,
    IN    NDIS_HALT_ACTION           HaltAction
    )
/*++

Routine Description:

    Halt handler. Add any further clean-up for the VELAN to this
    function.

    We wait for all pending I/O on the VELAN to complete and then
    unlink the VELAN from the adapter.

Arguments:

    MiniportAdapterContext    Pointer to the pVElan
    HaltAction                The reason adapter is being halted 

Return Value:

    None.

--*/
{
    PVELAN            pVElan = (PVELAN)MiniportAdapterContext;
    NET_IFINDEX       LowerLayerIfIndex;

    UNREFERENCED_PARAMETER(HaltAction);
    

    DBGPRINT(MUX_LOUD, ("==> MPHalt: VELAN %p\n", pVElan));

    //
    // Mark the VELAN so that we don't send down any new requests or
    // sends to the adapter below, or new receives/indications to
    // protocols above.
    //
    pVElan->MiniportHalting = TRUE;

    //
    // Update the packet filter on the underlying adapter if needed.
    //
    if (pVElan->PacketFilter != 0)
    {
        MPSetPacketFilter(pVElan, 0);
    }

    //
    // Wait for any outstanding sends or requests to complete.
    //
    while (pVElan->OutstandingSends)
    {
        DBGPRINT(MUX_INFO, ("MPHalt: VELAN %p has %d outstanding sends\n",
                            pVElan, pVElan->OutstandingSends));
        NdisMSleep(20000);
    }

    //
    // Wait for all outstanding indications to be completed and
    // any pended receive packets to be returned to us.
    //
    while (pVElan->OutstandingReceives)
    {
        DBGPRINT(MUX_INFO, ("MPHalt: VELAN %p has %d outstanding receives\n",
                            pVElan, pVElan->OutstandingReceives));
        NdisMSleep(20000);
    }


    //
    // Delete the ioctl interface that was created when the miniport
    // was created.
    //
    (VOID)PtDeregisterDevice();


    //
    // Delete stack entry for this Velan
    //
    if (pVElan->IfIndex != 0)
    {
        LowerLayerIfIndex = pVElan->pAdapt->BindParameters.BoundIfIndex;
        
        NdisIfDeleteIfStackEntry(pVElan->IfIndex,
                                 LowerLayerIfIndex);
        pVElan->IfIndex = 0;
    }
    

    //
    // Unlink the VELAN from its parent ADAPT structure. This will
    // dereference the VELAN.
    //
    pVElan->MiniportAdapterHandle = NULL;
    PtUnlinkVElanFromAdapter(pVElan);
    
    DBGPRINT(MUX_LOUD, ("<== MPHalt: pVElan %p\n", pVElan));
}


NDIS_STATUS
MPForwardOidRequest(
    IN PVELAN                       pVElan,
    IN PNDIS_OID_REQUEST            Request
    )
/*++

Routine Description:

    Utility routine that forwards an NDIS request made on a VELAN to the
    lower binding. Since at most a single request can be pended on a VELAN,
    we use the pre-allocated request structure embedded in the VELAN struct.

Arguments:

    pVElan                Pointer to a VElan Adapter
    Request               Pointer to an NDIS request to be forwarded to the below adapter.   

Return Value:

    NDIS_STATUS_PENDING if a request was sent down.
    NDIS_STATUS_XXX     Otherwise.

--*/
{
    NDIS_STATUS                  Status;
    PMUX_NDIS_REQUEST            pMuxNdisRequest = &pVElan->Request;

    PADAPT                       pAdapt = pVElan->pAdapt;
    

    DBGPRINT(MUX_LOUD, ("==> MPForwardOidRequest: VELAN %p, Request %p\n", pVElan, Request));

    do
    {
        MUX_INCR_PENDING_SENDS(pVElan);

        //
        // If the miniport below is going away, fail the request
        // 
        NdisAcquireSpinLock(&pVElan->Lock);
        if (pVElan->DeInitializing == TRUE)
        {
            NdisReleaseSpinLock(&pVElan->Lock);
            MUX_DECR_PENDING_SENDS(pVElan);
            Status = NDIS_STATUS_FAILURE;
            break;
        }
        NdisReleaseSpinLock(&pVElan->Lock);    

        //
        // If the virtual miniport edge is at a low power
        // state, fail this request.
        //
        if (MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState))
        {
            MUX_DECR_PENDING_SENDS(pVElan);
            Status = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }
        
        NdisAcquireSpinLock(&pVElan->Lock);
        pMuxNdisRequest->Cancelled = FALSE;
        pMuxNdisRequest->OrigRequest = Request;
        pMuxNdisRequest->pCallback = PtCompleteForwardedRequest;
        pMuxNdisRequest->Request.RequestType = Request->RequestType;
        pMuxNdisRequest->Refcount = 1;
        NdisReleaseSpinLock(&pVElan->Lock);    

        pMuxNdisRequest->Request.Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
        pMuxNdisRequest->Request.Header.Revision = NDIS_OID_REQUEST_REVISION_1;
        pMuxNdisRequest->Request.Header.Size = sizeof(NDIS_OID_REQUEST);

        switch (Request->RequestType)
        {
            case NdisRequestQueryInformation:
            case NdisRequestQueryStatistics:
                pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.Oid = Request->DATA.QUERY_INFORMATION.Oid;
                pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.InformationBuffer = 
                                            Request->DATA.QUERY_INFORMATION.InformationBuffer;
                pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.InformationBufferLength = 
                                            Request->DATA.QUERY_INFORMATION.InformationBufferLength;
                break;

            case NdisRequestSetInformation:
                pMuxNdisRequest->Request.DATA.SET_INFORMATION.Oid = Request->DATA.SET_INFORMATION.Oid;
                pMuxNdisRequest->Request.DATA.SET_INFORMATION.InformationBuffer = 
                                            Request->DATA.SET_INFORMATION.InformationBuffer;
                pMuxNdisRequest->Request.DATA.SET_INFORMATION.InformationBufferLength = 
                                            Request->DATA.SET_INFORMATION.InformationBufferLength;
                break;

            case NdisRequestMethod:
            default:
                ASSERT(FALSE);
                break;
        }

        //
        // If the miniport below is going away
        //
        NdisAcquireSpinLock(&pVElan->Lock);
        if (pVElan->DeInitializing == TRUE)
        {
            pMuxNdisRequest->OrigRequest = NULL;
            NdisReleaseSpinLock(&pVElan->Lock);
            MUX_DECR_PENDING_SENDS(pVElan);
            Status = NDIS_STATUS_FAILURE;
            break;
        }
        
        // If the lower binding has been notified of a low
        // power state, queue this request; it will be picked
        // up again when the lower binding returns to D0.
        //
        if (MUX_IS_LOW_POWER_STATE(pVElan->pAdapt->PtDevicePowerState))
        {
            DBGPRINT(MUX_INFO, ("ForwardRequest: VELAN %p, Adapt %p power"
                                " state is %d, queueing OID %x\n",
                                pVElan, pVElan->pAdapt,
                                pVElan->pAdapt->PtDevicePowerState, 
                                Request->DATA.QUERY_INFORMATION.Oid));

            pVElan->QueuedRequest = TRUE;
            NdisReleaseSpinLock(&pVElan->Lock);
            Status = NDIS_STATUS_PENDING;
            break;
        }

        if (pMuxNdisRequest->Cancelled == TRUE)
        {
            NdisReleaseSpinLock(&pVElan->Lock);
            Status = NDIS_STATUS_REQUEST_ABORTED;
            PtRequestComplete(pVElan->pAdapt, &pMuxNdisRequest->Request, Status);
            break;
        }
        NdisReleaseSpinLock(&pVElan->Lock);
        
        NdisAcquireSpinLock(&pAdapt->Lock);

        pAdapt->OutstandingRequests ++;
        
        if ((pAdapt->Flags & MUX_BINDING_CLOSING)== MUX_BINDING_CLOSING)
        {
            NdisReleaseSpinLock(&pAdapt->Lock);        
            Status = NDIS_STATUS_CLOSING;           
        }
        else
        {
            NdisReleaseSpinLock(&pAdapt->Lock);        
            Status  = NdisOidRequest(pVElan->BindingHandle,
                                    &pMuxNdisRequest->Request);
        }
        if (Status != NDIS_STATUS_PENDING)
        {
            PtRequestComplete(pVElan->pAdapt, &pMuxNdisRequest->Request, Status);
            Status = NDIS_STATUS_PENDING;
            break;
        }
    }
    while (FALSE);

    DBGPRINT(MUX_LOUD, ("<== MPForwardOidRequest: VELAN %p, Request %p, Status %8x\n", pVElan, Request, Status));

#if IEEE_VELAN_SUPPORT
    if ((Status != NDIS_STATUS_PENDING)
            && (((Request->RequestType == NdisRequestSetInformation)
            && (Request->DATA.SET_INFORMATION.Oid == OID_GEN_CURRENT_LOOKAHEAD))
            && (pVElan->RestoreLookaheadSize == TRUE)))
    {
        pVElan->RestoreLookaheadSize = FALSE;
        *(UNALIGNED PULONG)(Request->DATA.SET_INFORMATION.InformationBuffer) -= VLAN_TAG_HEADER_SIZE;
    }
#endif
    
    return (Status);
}

NDIS_STATUS
MPSetPacketFilter(
    IN PVELAN               pVElan,
    IN ULONG                PacketFilter
    )
/*++
Routine Description:

    This routine will set up the VELAN so that it accepts packets 
    that match the specified packet filter.  The only filter bits   
    that can truly be toggled are for broadcast and promiscuous.

    The MUX driver always sets the lower binding to promiscuous
    mode, but we do some optimization here to avoid turning on
    receives too soon. That is, we set the packet filter on the lower
    binding to a non-zero value iff at least one of the VELANs
    has a non-zero filter value.
    
    NOTE: setting the lower binding to promiscuous mode can
    impact CPU utilization. The only reason we set the lower binding
    to promiscuous mode in this sample is that we need to be able
    to receive unicast frames directed to MAC address(es) that do not
    match the local adapter's MAC address. If VELAN MAC addresses
    are set to be equal to that of the adapter below, it is sufficient
    to set the lower packet filter to the bitwise OR'ed value of
    packet filter settings on all VELANs.
                                    

Arguments:

    pVElan - pointer to VELAN
    PacketFilter - the new packet filter 
    
Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    
--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PADAPT          pAdapt;
    PVELAN          pTmpVElan;
    PLIST_ENTRY     p;
    ULONG           AdapterFilter;
    BOOLEAN         bSendUpdate = FALSE;
    LOCK_STATE      LockState;

    DBGPRINT(MUX_LOUD, ("==> MPSetPacketFilter VELAN %p, Filter %x\n", pVElan, PacketFilter));
    
    do
    {
        //
        // Any bits not supported?
        //
        if (PacketFilter & ~VELAN_SUPPORTED_FILTERS)
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
    
        AdapterFilter = 0;
        pAdapt = pVElan->pAdapt;

        //
        // Grab a Write lock on the adapter so that this operation
        // does not interfere with any receives that might be accessing
        // filter information.
        //
        MUX_ACQUIRE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

        //
        // Save the new packet filter value
        //
        pVElan->PacketFilter = PacketFilter;

        //
        // Compute the new combined filter for all VELANs on this
        // adapter.
        //
        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            pTmpVElan = CONTAINING_RECORD(p, VELAN, Link);
            AdapterFilter |= pTmpVElan->PacketFilter;
        }

        //
        // If all VELANs have packet filters set to 0, turn off
        // receives on the lower adapter, if not already done.
        //
        if ((AdapterFilter == 0) && (pAdapt->PacketFilter != 0))
        {
            bSendUpdate = TRUE;
            pAdapt->PacketFilter = 0;
        }
        else
        //
        // If receives had been turned off on the lower adapter, and
        // the new filter is non-zero, turn on the lower adapter.
        // We set the adapter to promiscuous mode in this sample
        // so that we are able to receive packets directed to
        // any of the VELAN MAC addresses.
        //
        if ((AdapterFilter != 0) && (pAdapt->PacketFilter == 0))
        {
            bSendUpdate = TRUE;
            pAdapt->PacketFilter = MUX_ADAPTER_PACKET_FILTER;
        }
        
        MUX_RELEASE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

        if (bSendUpdate)
        {
            PtRequestAdapterAsync(
                pAdapt,
                NdisRequestSetInformation,
                OID_GEN_CURRENT_PACKET_FILTER,
                &pAdapt->PacketFilter,
                sizeof(pAdapt->PacketFilter),
                PtDiscardCompletedRequest);
        }

    }
    while (FALSE);

    DBGPRINT(MUX_LOUD, ("<== MPSetPacketFilter VELAN %p, Status %x\n", pVElan, Status));
    
    return(Status);
}


NDIS_STATUS
MPSetMulticastList(
    IN PVELAN                                       pVElan,
    _In_reads_bytes_(InformationBufferLength) IN PVOID   InformationBuffer,
    IN ULONG                                        InformationBufferLength,
    OUT PULONG                                      pBytesRead,
    OUT PULONG                                      pBytesNeeded
    )
/*++

Routine Description:

    Set the multicast list on the specified VELAN miniport.
    We simply validate all information and copy in the multicast
    list.

    We don't forward the multicast list information down since
    we set the lower binding to promisc. mode.

Arguments:

    pVElan - VELAN on which to set the multicast list
    InformationBuffer - pointer to new multicast list
    InformationBufferLength - length in bytes of above list
    pBytesRead - place to return # of bytes read from the above
    pBytesNeeded - place to return expected min # of bytes

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_MULTICAST_FULL

--*/
{
    NDIS_STATUS         Status;
    PADAPT              pAdapt;
    LOCK_STATE          LockState;

    DBGPRINT(MUX_LOUD, ("==> MPSetMulticastList VELAN %p\n", pVElan));
    //
    // Initialize.
    //
    *pBytesNeeded = sizeof(MUX_MAC_ADDRESS);
    *pBytesRead = 0;
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        if (InformationBufferLength % sizeof(MUX_MAC_ADDRESS))
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        if (InformationBufferLength > (VELAN_MAX_MCAST_LIST * sizeof(MUX_MAC_ADDRESS)))
        {
            Status = NDIS_STATUS_MULTICAST_FULL;
            *pBytesNeeded = VELAN_MAX_MCAST_LIST * sizeof(MUX_MAC_ADDRESS);
            break;
        }

        pAdapt = pVElan->pAdapt;

        //
        // Grab a Write lock on the adapter so that this operation
        // does not interfere with any receives that might be accessing
        // multicast list information.
        //
        MUX_ACQUIRE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

        NdisZeroMemory(pVElan->McastAddrs,
                       VELAN_MAX_MCAST_LIST * sizeof(MUX_MAC_ADDRESS));
        
        NdisMoveMemory(&pVElan->McastAddrs[0],
                       InformationBuffer,
                       InformationBufferLength);
        
        pVElan->McastAddrCount = InformationBufferLength / sizeof(MUX_MAC_ADDRESS);
        
        MUX_RELEASE_ADAPT_WRITE_LOCK(pAdapt, &LockState);
    }
    while (FALSE);

    DBGPRINT(MUX_LOUD, ("<== MPSetMulticastList VELAN %p, Status %8x\n", pVElan, Status));
    
    return (Status);
}



PUCHAR
MacAddrToString(
    PVOID In
    )
/*++

Routine Description:

    Careful! Uses static storage for string. Used to simplify DbgPrints
    of MAC addresses.
    
Arguments:

    IN            Pointer to MAC address array

Return Value:

    A string format of the given mac address

--*/    
{
    static UCHAR String[20];
    static PCHAR HexChars = "0123456789abcdef";
    PUCHAR EthAddr = (PUCHAR) In;
    UINT i;
    PUCHAR s;
    
    for (i = 0, s = String; i < 6; i++, EthAddr++)
    {
#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW, "s is bounded by check above");    
        *s++ = HexChars[(*EthAddr) >> 4];
        *s++ = HexChars[(*EthAddr) & 0xf];
    }
    *s = '\0';
    
    return String; 
}


VOID
MPGenerateMacAddr(
    PVELAN                    pVElan
)
/*++

Routine Description:

    Generates a "virtual" MAC address for a VELAN.
    NOTE: this is only a sample implementation of selecting
    a MAC address for the VELAN. Other implementations are possible,
    including using the MAC address of the underlying adapter as
    the MAC address of the VELAN.
    
Arguments:

    pVElan  - Pointer to velan structure

Return Value:

    None

--*/
{
    ETH_COPY_NETWORK_ADDRESS(
            pVElan->CurrentAddress,
            pVElan->PermanentAddress);
    
    DBGPRINT(MUX_LOUD, ("%d CurrentAddress %s\n",
        pVElan->VElanNumber, MacAddrToString(&pVElan->CurrentAddress)));
    DBGPRINT(MUX_LOUD, ("%d PermanentAddress  %s\n",
        pVElan->VElanNumber, MacAddrToString(&pVElan->PermanentAddress)));

}



VOID
MPDevicePnPEvent(
    IN NDIS_HANDLE              MiniportAdapterContext,
    IN PNET_DEVICE_PNP_EVENT    NetDevicePnPEvent
    )
/*++

Routine Description:

    This handler is called to notify us of PnP events directed to
    our miniport device object.

Arguments:

    MiniportAdapterContext - pointer to VELAN structure
    DevicePnPEvent - the event
    InformationBuffer - Points to additional event-specific information
    InformationBufferLength - length of above

Return Value:

    None
--*/
{
    // TBD - add code/comments about processing this.
    //
    
    DBGPRINT(MUX_LOUD, ("==> MPDevicePnPEvent: AdapterContext %08lp, DevicePnPEvent %x\n",MiniportAdapterContext, NetDevicePnPEvent->DevicePnPEvent));

	UNREFERENCED_PARAMETER(MiniportAdapterContext);
    UNREFERENCED_PARAMETER(NetDevicePnPEvent);
 

    DBGPRINT(MUX_LOUD, ("<== MPDevicePnPEvent: AdapterContext %08lp, DevicePnPEvent %x\n",MiniportAdapterContext, NetDevicePnPEvent->DevicePnPEvent));
    
    return;
}


VOID
MPAdapterShutdown(
    IN NDIS_HANDLE              MiniportAdapterContext,
    IN NDIS_SHUTDOWN_ACTION     ShutdownAction
    )
/*++

Routine Description:

    This handler is called to notify us of an impending system shutdown.
    Since this is not a hardware driver, there isn't anything specific
    we need to do about this.

Arguments:

    MiniportAdapterContext     pointer to VELAN structure
    ShutdownAction             Specify the reason to shut down the adapter

Return Value:

    None
--*/
{
    PVELAN      pVElan = (PVELAN)MiniportAdapterContext;

    DBGPRINT(MUX_LOUD,("==> MPAdapterShutdown: VElan %p, ShutdwonAction %x\n", pVElan, ShutdownAction));

    UNREFERENCED_PARAMETER(pVElan);
    UNREFERENCED_PARAMETER(ShutdownAction);

    DBGPRINT(MUX_LOUD,("<== MPAdapterShutdown: VElan %p, ShutdwonAction %x\n", pVElan, ShutdownAction));
    
    return;
}


VOID
MPUnload(
    IN    PDRIVER_OBJECT        DriverObject
    )
/*++

Routine Description:
    This handler is used to unload the miniport

Arguments:
    DriverObject            Pointer to the system's driver object structure 
                            for this driver.

Return Value:
    None


--*/
{
        
#if !DBG
    UNREFERENCED_PARAMETER(DriverObject);
#endif
    
    DBGPRINT(MUX_LOUD, ("==> MPUnload: DriverObj %p\n", DriverObject));
    if (ProtHandle != NULL)
    {
        NdisDeregisterProtocolDriver(ProtHandle);
    }
    NdisMDeregisterMiniportDriver(DriverHandle);

    NdisFreeSpinLock(&GlobalLock);
        
    DBGPRINT(MUX_LOUD, ("<== MPUnload: DriverObj %p\n", DriverObject));     
}

NDIS_STATUS
MPPause(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PNDIS_MINIPORT_PAUSE_PARAMETERS  MiniportPauseParameters
    )
/*++

Routine Description:
    This handler is used to pause the miniport. During which, no NET_BUFFER_LIST
    will be indicated to the upper binding as well as status indications.

Arguments:
    MiniportAdapterContext      Pointer to our VELAN
    MiniportPauseParameters     Specify the pause parameters

Return Value:
    NDIS_STATUS_SUCCESS

--*/
{
    PVELAN             pVElan = (PVELAN)MiniportAdapterContext;
    NDIS_STATUS        Status = NDIS_STATUS_SUCCESS;

    DBGPRINT(MUX_LOUD, ("==> MPPause: VElan %p\n", pVElan));
    
    UNREFERENCED_PARAMETER(MiniportPauseParameters);

    DBGPRINT(MUX_LOUD,("==>MPPause  Adapter %08lp\n",MiniportAdapterContext));

    // Whilst the miniport is being paused, it cannot be restart

    NdisAcquireSpinLock(&pVElan->PauseLock);

    pVElan->Paused = TRUE;

    NdisReleaseSpinLock(&pVElan->PauseLock);

       

    DBGPRINT(MUX_LOUD,("<== MPPause,VElan %p, Status %8x\n", pVElan, Status));

    return Status;
}


NDIS_STATUS
MPRestart(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PNDIS_MINIPORT_RESTART_PARAMETERS  MiniportRestartParameters
    )
/*++

Routine Description:
    This handler is used to restart the miniport.  When the miniport is
    back in the restart state, it can indicate NET_BUFFER_LISTs to the
    upper binding

Arguments:
    MiniportAdapterContext      Pointer to our VELAN
    MiniportRestartParameters

Return Value:
    NDIS_STATUS_SUCCESS

--*/
{
    PVELAN                            pVElan = (PVELAN)MiniportAdapterContext;
    NDIS_STATUS                       Status = NDIS_STATUS_SUCCESS;
    PNDIS_RESTART_ATTRIBUTES          NdisRestartAttributes;
    PNDIS_RESTART_GENERAL_ATTRIBUTES  NdisGeneralAttributes;
    
    UNREFERENCED_PARAMETER(MiniportRestartParameters);

    DBGPRINT(MUX_LOUD,("==> MPRestart  Adapter %p\n",MiniportAdapterContext));

     
    //
    // Here the driver can change its restart attributes 
    //
    NdisRestartAttributes = MiniportRestartParameters->RestartAttributes;

    //
    // If NdisRestartAttributes is not NULL, then miniport can modify generic attributes and add
    // new media specific info attributes at the end. Otherwise, NDIS restarts the miniport because 
    // of other reason, miniport should not try to modify/add attributes
    //
    if (NdisRestartAttributes != NULL)
    {

        ASSERT(NdisRestartAttributes->Oid == OID_GEN_MINIPORT_RESTART_ATTRIBUTES);
        
        NdisGeneralAttributes = (PNDIS_RESTART_GENERAL_ATTRIBUTES)NdisRestartAttributes->Data;
        UNREFERENCED_PARAMETER(NdisGeneralAttributes);
    
        //
        // Check to see if we need to change any attributes, for example, the driver can change the current
        // MAC address here. Or the driver can add media specific info attributes.
        //
    }
   
    NdisAcquireSpinLock(&pVElan->PauseLock);
    pVElan->Paused = FALSE;

    NdisReleaseSpinLock(&pVElan->PauseLock);
 

    DBGPRINT(MUX_LOUD,("<== MPRestart: Adapter %p, Status %8x\n", MiniportAdapterContext, Status));

    return Status;
}


VOID
MPSendNetBufferLists(
    IN NDIS_HANDLE      MiniportAdapterContext,
    IN PNET_BUFFER_LIST NetBufferLists,
    IN NDIS_PORT_NUMBER PortNumber,
    IN ULONG            SendFlags
    )
/*++

Routine Description:

    Send NET_BUFFER_LISTs to the lower binding

Arguments:
    MiniportAdapterContext          Pointer to our VELAN
    NetBufferLists                  Set of NET_BUFFER_LISTs to send
    SendFlags                       Specify the send flags
    DispatchLevel                   TRUE if IRQL == DISPATCH_LEVEL

Return Value:
    None

--*/
{
    PVELAN                      pVElan = (PVELAN)MiniportAdapterContext;
    PADAPT                      pAdapt = pVElan->pAdapt;
    PNET_BUFFER_LIST            CurrentNetBufferList = NULL;
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PIM_NBL_ENTRY               SendContext;
    ULONG                       SendCompleteFlags = 0;
    BOOLEAN                     DispatchLevel = FALSE;
    
    DBGPRINT(MUX_VERY_LOUD,("==> MPSendNetBufferLists: MiniportAdapterContext  %p, NetBufferLists %p\n",MiniportAdapterContext,NetBufferLists));

    DispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags);

    while(NetBufferLists != NULL)
    {
        CurrentNetBufferList = NetBufferLists;
        NetBufferLists = NET_BUFFER_LIST_NEXT_NBL(NetBufferLists);
        NET_BUFFER_LIST_NEXT_NBL(CurrentNetBufferList) = NULL;

        MUX_ACQUIRE_SPIN_LOCK(&pAdapt->Lock, DispatchLevel);
            
        if (pAdapt->BindingState != MuxAdapterBindingRunning)
        {
            Status = NDIS_STATUS_REQUEST_ABORTED;
            MUX_RELEASE_SPIN_LOCK(&pAdapt->Lock, DispatchLevel);
                
            break;
        }
          
        pAdapt->OutstandingSends ++;
            
        MUX_RELEASE_SPIN_LOCK(&pAdapt->Lock, DispatchLevel);
        
        do
        {
            Status = NdisAllocateNetBufferListContext(CurrentNetBufferList,
                                                      sizeof(IM_NBL_ENTRY),
                                                      0,
                                                      MUX_TAG);

            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
            
            SendContext = (PIM_NBL_ENTRY)NET_BUFFER_LIST_CONTEXT_DATA_START(CurrentNetBufferList);
            NdisZeroMemory(SendContext, sizeof(IM_NBL_ENTRY));
            SendContext->PreviousSourceHandle = CurrentNetBufferList->SourceHandle;
            SendContext->pVElan = pVElan;


#ifdef IEEE_VLAN_SUPPORT
            SendContext->Flags = 0;

            Status = MPHandleSendTaggingNB(pVElan, CurrentNetBufferList);

            if (Status != NDIS_STATUS_SUCCESS)
            {
                NdisFreeNetBufferListContext(CurrentNetBufferList,
                                             sizeof(IM_NBL_ENTRY));
                break;
            }

#endif

            CurrentNetBufferList->SourceHandle = pAdapt->BindingHandle;

            MUX_INCR_PENDING_SENDS(pVElan);

            //
            // Remove this flag, so NDIS will not try to loopback the packets to mux
            //
            SendFlags &= ~NDIS_SEND_FLAGS_CHECK_FOR_LOOPBACK;

            NdisSendNetBufferLists(pAdapt->BindingHandle,
                                   CurrentNetBufferList,
                                   PortNumber,
                                   SendFlags);
                                   
        } while(FALSE);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            MUX_ACQUIRE_SPIN_LOCK(&pAdapt->Lock, DispatchLevel);
            pAdapt->OutstandingSends --;
            
            if ((pAdapt->OutstandingSends == 0) && (pAdapt->PauseEvent != NULL))
            {
                NdisSetEvent(pAdapt->PauseEvent);
                pAdapt->PauseEvent = NULL;
            }
            
            MUX_RELEASE_SPIN_LOCK(&pAdapt->Lock, DispatchLevel);
            //
            // Handle failure case
            //
            NET_BUFFER_LIST_STATUS(CurrentNetBufferList) = Status;

            if (NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags))
            {
                NDIS_SET_SEND_COMPLETE_FLAG(SendCompleteFlags, NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL);
            }

            NdisMSendNetBufferListsComplete(pVElan->MiniportAdapterHandle,
                                            CurrentNetBufferList,
                                            SendCompleteFlags);

            Status = NDIS_STATUS_SUCCESS;
        }
    }

    if (Status != NDIS_STATUS_SUCCESS)
    {
        PNET_BUFFER_LIST          TempNetBufferList;

        for (TempNetBufferList = CurrentNetBufferList;
             TempNetBufferList != NULL;
             TempNetBufferList = NET_BUFFER_LIST_NEXT_NBL(TempNetBufferList))
        {
            NET_BUFFER_LIST_STATUS(TempNetBufferList) = Status;
        }
        if (NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags))
        {
            NDIS_SET_SEND_COMPLETE_FLAG(SendCompleteFlags, NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL);
        }

        NdisMSendNetBufferListsComplete(pVElan->MiniportAdapterHandle,
                                        CurrentNetBufferList,
                                        SendCompleteFlags);
    }

    DBGPRINT(MUX_VERY_LOUD,("<== MPSendNetBufferLists, MiniportAdapterContext  %p, NetBufferLists %p\n",MiniportAdapterContext,NetBufferLists));
}

VOID 
MPReturnNetBufferLists(
    IN NDIS_HANDLE      MiniportAdapterContext,
    IN PNET_BUFFER_LIST NetBufferLists,
    IN ULONG            ReturnFlags
    )
/*++

Routine Description:
    NDIS Miniport entry point called whenever protocols are done with
    a packet that we had indicated up and they had queued up for returning
    later.

Arguments:
    MiniportAdapterContext          Pointer to VELAN structure
    NetBufferLists                  NetBufferLists being returned
    Dispatch                        TRUE if IRQL == DISPATCH_LEVEL

Return Value:
    None

--*/
{
    PVELAN                  pVElan = (PVELAN)MiniportAdapterContext;
    PNET_BUFFER_LIST        CurrentNetBufferList = NULL;
    ULONG                   NumberOfNetBufferLists = 0;
#ifdef IEEE_VLAN_SUPPORT  
    NDIS_STATUS             Status;
#endif
    
    DBGPRINT(MUX_VERY_LOUD,("==> MPReturnNetBufferLists: MiniportAdapterContext %p, NetBufferList %p\n",MiniportAdapterContext,NetBufferLists));

    CurrentNetBufferList = NetBufferLists;

    while (CurrentNetBufferList)
    {
        NumberOfNetBufferLists++;

#ifdef IEEE_VLAN_SUPPORT
        //
        // Retreat the NBL before returning it to the miniport
        //
        Status = PtRestoreReceiveNBL(CurrentNetBufferList);

        ASSERT(Status == NDIS_STATUS_SUCCESS);

        //
        // Free the context that was allocated in PtReceiveNBL
        //
        NdisFreeNetBufferListContext(CurrentNetBufferList,
                                     sizeof(RECV_NBL_ENTRY));                    
#endif        

        CurrentNetBufferList = NET_BUFFER_LIST_NEXT_NBL(CurrentNetBufferList);
    }
    
    NdisReturnNetBufferLists(pVElan->BindingHandle,
                             NetBufferLists,
                             ReturnFlags);

    MUX_DECR_MULTIPLE_PENDING_RECEIVES(pVElan, NumberOfNetBufferLists);

    DBGPRINT(MUX_VERY_LOUD,("<== MPReturnNetBufferLists: MiniportAdapterContext %p, NetBufferList %p\n",MiniportAdapterContext,NetBufferLists));
}
    

VOID 
MPCancelSendNetBufferLists(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PVOID       CancelId
    )
/*++

Routine Description:

    The miniport entry point to hanadle cancellation of all send packets that
    match the given CancelId. If we have queued any packets that match this,
    then we should dequeue them and call NdisMSendCompleteNetBufferLists for
    all such packets, with a status of NDIS_STATUS_REQUEST_ABORTED.

    We should also call NdisCancelSendPackets in turn, on each lower binding
    that this adapter corresponds to. This is to let miniports below cancel
    any matching packets.

Arguments:

    MiniportAdapterContext          Pointer to VELAN structure
    CancelID                        ID of NetBufferLists to be cancelled

Return Value:
    None

--*/
{
    PVELAN                      pVElan = (PVELAN)MiniportAdapterContext;

    DBGPRINT(MUX_LOUD,("==> MPCancelSendNetBufferLists: VElan %p, CancelId %p\n", pVElan, CancelId));
    
    NdisCancelSendNetBufferLists(pVElan->pAdapt->BindingHandle,CancelId);
    
    DBGPRINT(MUX_LOUD,("<== MPCancelSendNetBufferLists: VElan %p, CancelId %p\n", pVElan, CancelId));
}

VOID 
MPCancelOidRequest(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PVOID       RequestId
    )
/*++

Routine Description:

    The miniport entry point to hanadle cancellation of a request. This function 
    checks to see if the CancelRequest should be terminated at this level
    or passed down to the next driver. 

Arguments:

    MiniportAdapterContext          Pointer to VELAN structure
    RequestId                       RequestId to be cancelled

Return Value:
    None

--*/
{
    PVELAN                      pVElan = (PVELAN)MiniportAdapterContext;
    PMUX_NDIS_REQUEST           pMuxNdisRequest = &pVElan->Request;
    BOOLEAN                     fCancelRequest = FALSE;
    
    DBGPRINT(MUX_LOUD, ("==> MPCancelOidRequest: VELAN %p, RequestId %p\n", pVElan, RequestId));
        
    NdisAcquireSpinLock(&pVElan->Lock);
    if (pMuxNdisRequest->OrigRequest != NULL)
    {
        if (pMuxNdisRequest->OrigRequest->RequestId == RequestId)
        {
            pMuxNdisRequest->Cancelled = TRUE;
            fCancelRequest = TRUE;
            pMuxNdisRequest->Refcount++;
        }

    }
    
    NdisReleaseSpinLock(&pVElan->Lock);    

    //
    // If we find the request, just send down the cancel, otherwise return because there is only 
    // one request pending from upper layer on the miniport
    //
    if (fCancelRequest)
    {
        NdisCancelOidRequest(pVElan->pAdapt->BindingHandle, &pMuxNdisRequest->Request);

        PtCompleteForwardedRequest(pVElan->pAdapt, 
                                    pMuxNdisRequest, 
                                    NDIS_STATUS_REQUEST_ABORTED);
    }
   
    DBGPRINT(MUX_LOUD, ("<== MPCancelOidRequest: VELAN %p, RequestId %p\n", pVElan, RequestId));
}



#ifdef IEEE_VLAN_SUPPORT

PMDL
MuxAllocateMdl(
    IN OUT PULONG               BufferSize
    )
/*++

Routine Description:
    This function is called by NDIS in order to allocate an MDL and memory when 
    there isn't unused data space in the net buffer when NdisRetreatNetBufferDataStart 
    is called

Arguments:
    BufferSize                      Pointer to allocation size being requested
    
Return Value:

NOTE: This function always returns NULL. This is so that MUX can allocate memory and MDL
      and save the required context about the NetBuffer in the allocated memory

--*/
{
    UNREFERENCED_PARAMETER(BufferSize);

    return NULL;
}

NDIS_STATUS 
MPHandleSendTaggingNB(
    IN PVELAN pVElan,
    IN PNET_BUFFER_LIST NetBufferList
    )
/*++

Routine Description:
    This function is called when the driver supports IEEE 802.1Q taggng. It checks
    the netbuffer to be sent on a VELAN and inserts a tag header if necessary.

Arguments:
    pVElan                          Pointer to VELAN structure
    NetBufferList                   A pointer to a NET_BUFFER_LIST

Return Value:
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_XXX

NOTE: This functio doesn't handle vlan tagging in an efficient way, please wait for the next release
      to get a better implementation.

--*/
{
    NDIS_STATUS             Status;
    NDIS_NET_BUFFER_LIST_8021Q_INFO  NdisPacket8021qInfo;
    PUCHAR                  pEthFrame = NULL;
    PUCHAR                  pEthFrameNew = NULL;
    PUSHORT                 pTpid;
    PVLAN_TAG_HEADER        pTagHeader;
    PIM_NBL_ENTRY           SendContext;
    PNET_BUFFER             CurrentNetBuffer;
    PIM_SEND_NB_ENTRY       pNetBufferContext, LastNetBufferContext;
    PVOID                   pVa;
    PMDL                    Mdl, FirstMdl, SecondMdl, PrevMdl;
    ULONG                   BytesToSkip;
    ULONG                   BufferLength;  
    PVOID                   Storage;
    PNET_BUFFER             MdlAllocatedNetBuffers = NULL;

    DBGPRINT(MUX_LOUD, ("==> MPHandleSendTaggingNB: VELAN %p, NetBufferList %p\n", pVElan, NetBufferList));
    
    NdisPacket8021qInfo.Value = NET_BUFFER_LIST_INFO(NetBufferList, Ieee8021QNetBufferListInfo);
    SendContext = (PIM_NBL_ENTRY)NET_BUFFER_LIST_CONTEXT_DATA_START(NetBufferList);
    
    do
    {
        Status = NDIS_STATUS_SUCCESS;

        // If the vlan ID of the virtual miniport is 0, the miniport should act like it doesn't
        // support VELAN tag processing

        if (MuxRecognizedVlanId(pVElan,0))
        {
            break;
        }

        //
        //  Insert a tag only if we have a configured VLAN ID. Note that we do not
        //  support E-RIF
        //

        if (NdisPacket8021qInfo.TagHeader.CanonicalFormatId)
        {
            //
            // Skip the packet, return NDIS_STATUS_FAILURE
            //
            Status = NDIS_STATUS_INVALID_PACKET;
            break;
        }


        //
        // If the there is a tag header and it doesn't match the VLAN ID
        // then the packet is invalid .. ignore!
        //
        if ((NdisPacket8021qInfo.TagHeader.VlanId)
            && (! MuxRecognizedVlanId(pVElan,NdisPacket8021qInfo.TagHeader.VlanId)))
        {
            Status = NDIS_STATUS_INVALID_PACKET;
            break;
        }

        CurrentNetBuffer = NET_BUFFER_LIST_FIRST_NB(NetBufferList);
        LastNetBufferContext = NULL;

        while(CurrentNetBuffer)
        {
            //
            // Find the start address of the frame
            // 
            Storage = NULL;
            pEthFrame = NdisGetDataBuffer(CurrentNetBuffer,
                                          ETH_HEADER_SIZE,
                                          Storage,
                                          1,
                                          0);

            if (pEthFrame == NULL)
            {
                Status = NDIS_STATUS_INVALID_PACKET;
                break;
            }

            Mdl = NET_BUFFER_CURRENT_MDL(CurrentNetBuffer);
            PrevMdl = NULL;
            
            //
            // Retreat the net buffer list
            //
            Status = NdisRetreatNetBufferDataStart(CurrentNetBuffer,
                                                   VLAN_TAG_HEADER_SIZE,
                                                   0,
                                                   MuxAllocateMdl);

            if (Status == NDIS_STATUS_SUCCESS)
            {
                //
                // If there was a MDL ahead of the current MDL, this could result
                // in the retreat being successful, but the retreated bytes being in
                // a different MDL. But we need to make sure that the ethernet header
                // with the VLAN tag is in contiguous memory
                //
                if (Mdl != NET_BUFFER_CURRENT_MDL(CurrentNetBuffer))
                {
                    //
                    // Advance the NetBuffer so that we can allocate MDLs instead
                    //                    
                    NdisAdvanceNetBufferDataStart(CurrentNetBuffer,
                                                  VLAN_TAG_HEADER_SIZE,
                                                  FALSE,
                                                  NULL);

                    Status = NDIS_STATUS_RESOURCES;
                }
            }
            
            if (Status == NDIS_STATUS_RESOURCES)
            {
                do
                {                        
                    //
                    // There is no more unused data space in the NetBuffer, need to allocate
                    // a new MDL and memory
                    //
                    BytesToSkip = ETH_HEADER_SIZE;
                    Mdl = NET_BUFFER_CURRENT_MDL(CurrentNetBuffer);

                    //
                    // Assume the Ethernet Header is in the first buffer of the packet.
                    // The following loop is to find the start address of the data after
                    // the ethernet header. This may be either in the first MDL
                    // or in the second.
                    // 
                    while (TRUE)
                    {
                        pVa = NULL;
                        NdisQueryMdl(Mdl, &pVa, &BufferLength, NormalPagePriority | MdlMappingNoExecute);

                        if (pVa == NULL)
                        {
                            break;
                        }

                        //
                        // Have we gone far enough into the packet?
                        // 
                        if (BytesToSkip == 0)
                        {
                            break;
                        }

                        //
                        // Does the current buffer contain bytes past the Ethernet
                        // header? If so, stop.
                        // 
                        if (BufferLength > BytesToSkip)
                        {
                            pVa = (PVOID)((PUCHAR)pVa + BytesToSkip);
                            BufferLength -= BytesToSkip;
                            break;
                        }

                        //
                        // We haven't gone past the Ethernet header yet, so go
                        // to the next buffer.
                        //
                        BytesToSkip -= BufferLength;
                        Mdl = NDIS_MDL_LINKAGE(Mdl);                    
                    }

                    if (pVa == NULL)
                    {
                        Status = NDIS_STATUS_RESOURCES;
                        break;
                    }

                    //
                    // AllocateSpace for Ethernet + VLAN tag header + Netbuffer context
                    //
                    pNetBufferContext = (PIM_SEND_NB_ENTRY) NdisAllocateFromNPagedLookasideList(&pVElan->TagLookaside);

                    //
                    // Memory allocation failed
                    //
                    if (pNetBufferContext == NULL)
                    {
                        Status = NDIS_STATUS_RESOURCES;
                        break;
                    }

                    NdisZeroMemory((PVOID)pNetBufferContext, sizeof(IM_SEND_NB_ENTRY));
                    
                    pEthFrameNew = ((PUCHAR) pNetBufferContext) + sizeof(IM_SEND_NB_ENTRY);

                    //
                    // Allocate MDLs for the Ethernet + VLAN tag header and
                    // the data that follow these.
                    //
                    SecondMdl = NdisAllocateMdl(pVElan->MiniportAdapterHandle,
                                                pVa,    // byte following the Eth+tag headers
                                                BufferLength);
                    
                    FirstMdl = NdisAllocateMdl(pVElan->MiniportAdapterHandle,
                                               pEthFrameNew,
                                               ETH_HEADER_SIZE + VLAN_TAG_HEADER_SIZE);

                    if (!FirstMdl || !SecondMdl)
                    {
                        //
                        // One of the buffer allocations failed
                        //
                        if (FirstMdl)
                        {
                            NdisFreeMdl(FirstMdl);
                        }

                        if (SecondMdl)
                        {
                            NdisFreeMdl(SecondMdl);
                        }

                        NdisFreeToNPagedLookasideList(&pVElan->TagLookaside, (PVOID) pNetBufferContext);

                        Status = NDIS_STATUS_RESOURCES;
                        break;
                    }
                    
                    //
                    // All allocations are successful. 
                    // Copy the Ethernet header to the newly allocated memory
                    // Leave space for the VLAN tag
                    //
                    NdisMoveMemory(pEthFrameNew, pEthFrame, 2 * ETH_LENGTH_OF_ADDRESS);
                    
                    NdisMoveMemory(pEthFrameNew + (2 * ETH_LENGTH_OF_ADDRESS) + VLAN_TAG_HEADER_SIZE, 
                                   pEthFrame + (2 * ETH_LENGTH_OF_ADDRESS), 
                                   2);

                    //
                    // Save the context for the NetBuffer
                    //
                    pNetBufferContext->CurrentMdl = NET_BUFFER_CURRENT_MDL(CurrentNetBuffer);

                    //
                    // If the current MDL is not the first on in the chain, we need to adjust the MDL chain
                    //
                    if (NET_BUFFER_FIRST_MDL(CurrentNetBuffer) != NET_BUFFER_CURRENT_MDL(CurrentNetBuffer))
                    {
                        PrevMdl = NET_BUFFER_FIRST_MDL(CurrentNetBuffer);
                        while (NDIS_MDL_LINKAGE(PrevMdl) != NET_BUFFER_CURRENT_MDL(CurrentNetBuffer))
                        {
                            PrevMdl = NDIS_MDL_LINKAGE(PrevMdl);
                        }

                        pNetBufferContext->PrevMdl = PrevMdl;                        
                    }
                    
                    pNetBufferContext->CurrentMdlOffset = NET_BUFFER_CURRENT_MDL_OFFSET(CurrentNetBuffer);

                    //
                    // Link this NB to the NBL context to be restored
                    // This is so that MPRestoreSendNBL can free the MDLs that were allocated
                    // for this NB
                    //
                    if (MdlAllocatedNetBuffers == NULL)
                    {
                        MdlAllocatedNetBuffers = CurrentNetBuffer;
                    }
                    else
                    {
                        ASSERT(LastNetBufferContext);
                        LastNetBufferContext->NextNetBuffer = CurrentNetBuffer;
                    }

                    LastNetBufferContext = pNetBufferContext;

                    //
                    // Adjust the NetBuffer to use the new Mdls
                    //
                    NDIS_MDL_LINKAGE(FirstMdl) = SecondMdl;
                    
                    NDIS_MDL_LINKAGE(SecondMdl) = NDIS_MDL_LINKAGE(Mdl);
                    
                    NET_BUFFER_DATA_OFFSET(CurrentNetBuffer) = NET_BUFFER_DATA_OFFSET(CurrentNetBuffer) - 
                                                               NET_BUFFER_CURRENT_MDL_OFFSET(CurrentNetBuffer);

                    NET_BUFFER_DATA_LENGTH(CurrentNetBuffer) += VLAN_TAG_HEADER_SIZE;

                    NET_BUFFER_CURRENT_MDL_OFFSET(CurrentNetBuffer) = 0;

                    NET_BUFFER_CURRENT_MDL(CurrentNetBuffer) = FirstMdl;

                    //
                    // If there are any MDLs in the MDL chain ahead of the current MDL,
                    // adjust the linkage
                    //
                    if (PrevMdl)
                    {
                        NDIS_MDL_LINKAGE(PrevMdl) = FirstMdl;
                    }
                    else
                    {
                        NET_BUFFER_FIRST_MDL(CurrentNetBuffer) = FirstMdl;
                    }

                    Status = NDIS_STATUS_SUCCESS;
                }
                while (FALSE);
            }
            else if (Status == NDIS_STATUS_SUCCESS)
            {
                //
                // There was enough unused space in the NetBuffer to 
                // accomodate the VLAN tag. 
                // Get new start address of frame
                // 
                Storage = NULL;
                pEthFrameNew = NdisGetDataBuffer(CurrentNetBuffer,
                                                 VLAN_TAG_HEADER_SIZE,
                                                 Storage,
                                                 1,
                                                 0);

                if (pEthFrameNew == NULL)
                {
                    NdisAdvanceNetBufferDataStart(CurrentNetBuffer,
                                                  VLAN_TAG_HEADER_SIZE,
                                                  FALSE,
                                                  NULL);
                    
                    Status = NDIS_STATUS_INVALID_PACKET;
                }
                else
                {
                    //
                    // Adjust the header to insert the VLAN tag in the packet frame
                    //
                    NdisMoveMemory(pEthFrameNew, pEthFrame, 2 * ETH_LENGTH_OF_ADDRESS);
                }
                
            }

            if (Status != NDIS_STATUS_SUCCESS)
            {
                break;
            }
            
            pTpid = (PUSHORT)((PUCHAR)pEthFrameNew + 2 * ETH_LENGTH_OF_ADDRESS);
            *pTpid = TPID;
            pTagHeader = (PVLAN_TAG_HEADER)(pTpid + 1);

            //
            // Write IEEE 802.1Q info to packet frame
            //

            INITIALIZE_TAG_HEADER_TO_ZERO(pTagHeader);

            if (NdisPacket8021qInfo.Value)
            {
                SET_USER_PRIORITY_TO_TAG(pTagHeader, NdisPacket8021qInfo.TagHeader.UserPriority);
            }
            else
            {
                SET_USER_PRIORITY_TO_TAG(pTagHeader, 0);
            }

            SET_CANONICAL_FORMAT_ID_TO_TAG(pTagHeader, 0);

            if (NdisPacket8021qInfo.TagHeader.VlanId)
            {
                SET_VLAN_ID_TO_TAG(pTagHeader, NdisPacket8021qInfo.TagHeader.VlanId);
            }
            else
            {
                SET_VLAN_ID_TO_TAG(pTagHeader, pVElan->VlanId);
            }
            
            CurrentNetBuffer = NET_BUFFER_NEXT_NB(CurrentNetBuffer);
        }

        if(Status == NDIS_STATUS_SUCCESS)
        {
            SendContext->Flags |= MUX_RETREAT_DATA;
            SendContext->MdlAllocatedNetBuffers = MdlAllocatedNetBuffers;
            NET_BUFFER_LIST_INFO(NetBufferList, Ieee8021QNetBufferListInfo) = 0;            
        }
        else
        {
            //
            // In case of failure, restore the NetBuffers to their original state
            // Only the NetBuffers in the NBL upto CurrentNetBuffer needs to be restored
            //
            MPRestoreSendNBL(pVElan, NetBufferList, CurrentNetBuffer, MdlAllocatedNetBuffers);
        }

    } while(FALSE);

    DBGPRINT(MUX_LOUD, ("<== MPHandleSendTaggingNB: VELAN %p, NetBufferList %p, Status %8x\n", pVElan, NetBufferList, Status));
    return Status;
}

VOID 
MPRestoreSendNBL(
    IN PVELAN               pVElan,
    IN PNET_BUFFER_LIST     NetBufferList,
    IN PNET_BUFFER          LastNetBuffer,
    IN PNET_BUFFER          MdlAllocatedNetBuffers
    )
/*++

Routine Description:
    Restore the NBL that was modified during Send

Arguments:
    pVElan                          Pointer to VELAN structure
    NetBufferList                   A pointer to a NET_BUFFER_LIST

Return Value:

--*/
    
{
    PNET_BUFFER         CurrentNetBuffer;
    PNET_BUFFER         CurrentMdlAllocatedNetBuffer, SavedMdlAllocatedNetBuffer;
    PIM_SEND_NB_ENTRY   NetBufferContext;
    PVOID               pVa = NULL;
    ULONG               BufferLength;
    PUCHAR              pFrame = NULL, pDst = NULL; 
    PMDL                FirstMdl, SecondMdl;
    PVOID               Storage;

    CurrentNetBuffer = NET_BUFFER_LIST_FIRST_NB(NetBufferList);
    CurrentMdlAllocatedNetBuffer = MdlAllocatedNetBuffers;
    
    while (CurrentNetBuffer != LastNetBuffer)
    {
        SavedMdlAllocatedNetBuffer = CurrentMdlAllocatedNetBuffer;

        //
        // Free the MDLs and the memory allocated for the NET_BUFFER
        //
        if (CurrentMdlAllocatedNetBuffer)
        {
            NdisQueryMdl(NET_BUFFER_CURRENT_MDL(CurrentMdlAllocatedNetBuffer), 
                         &pVa, 
                         &BufferLength, 
                         NormalPagePriority | MdlMappingNoExecute);
            if( pVa == NULL ){
                //you may do something
            }
            NetBufferContext = (PIM_SEND_NB_ENTRY) ((PUCHAR) pVa - sizeof(IM_SEND_NB_ENTRY));
            ASSERT(NetBufferContext != NULL);

            //
            // Save the MDLs to be freed in temporary variables
            //
            FirstMdl = NET_BUFFER_CURRENT_MDL(CurrentMdlAllocatedNetBuffer);

            SecondMdl = NDIS_MDL_LINKAGE(FirstMdl);

            //
            // Adjust the offsets and length
            //
            if( NetBufferContext == NULL ){
                //check why NetBufferContext is NULL
            }
            else{
                NET_BUFFER_DATA_OFFSET(CurrentMdlAllocatedNetBuffer) = NET_BUFFER_DATA_OFFSET(CurrentMdlAllocatedNetBuffer) + 
                                                                       NetBufferContext->CurrentMdlOffset;
                
                NET_BUFFER_DATA_LENGTH(CurrentMdlAllocatedNetBuffer) -= VLAN_TAG_HEADER_SIZE;   

                NET_BUFFER_CURRENT_MDL_OFFSET(CurrentMdlAllocatedNetBuffer) = NetBufferContext->CurrentMdlOffset;

                NET_BUFFER_CURRENT_MDL(CurrentMdlAllocatedNetBuffer) = NetBufferContext->CurrentMdl;

                if (NetBufferContext->PrevMdl)
                {
                    NDIS_MDL_LINKAGE(NetBufferContext->PrevMdl) = NetBufferContext->CurrentMdl;
                }
                else
                {
                    NET_BUFFER_FIRST_MDL(CurrentMdlAllocatedNetBuffer) = NetBufferContext->CurrentMdl;
                }
                
                CurrentMdlAllocatedNetBuffer = NetBufferContext->NextNetBuffer;                   

                //
                // Free the MDLs and the memory allocated
                //
                NdisFreeMdl(SecondMdl);        

                NdisFreeMdl(FirstMdl);

                NdisFreeToNPagedLookasideList(&pVElan->TagLookaside, (PVOID) NetBufferContext);          
            }
        }

        //
        // Advance the NET_BUFFERs until the NET_BUFFER for which 
        // the MDLs were allocated
        //
        while ((CurrentNetBuffer != SavedMdlAllocatedNetBuffer) &&
               (CurrentNetBuffer != LastNetBuffer))
        {
            Storage = NULL;
            pFrame = NdisGetDataBuffer(CurrentNetBuffer,
                                       (2 * ETH_LENGTH_OF_ADDRESS) + VLAN_TAG_HEADER_SIZE,
                                       Storage,
                                       1,
                                       0);

            if (pFrame == NULL)
            {
                ASSERT(FALSE);
            }
            else
            {
                //
                // Restore the original header
                //
                pDst = pFrame + VLAN_TAG_HEADER_SIZE;

                RtlMoveMemory(pDst, pFrame, (2 * ETH_LENGTH_OF_ADDRESS));
            }
                    
            NdisAdvanceNetBufferDataStart(CurrentNetBuffer,
                                          VLAN_TAG_HEADER_SIZE,
                                          FALSE,
                                          NULL);    
                
            CurrentNetBuffer = NET_BUFFER_NEXT_NB(CurrentNetBuffer);                
        }

        if (SavedMdlAllocatedNetBuffer)
        {
            CurrentNetBuffer = NET_BUFFER_NEXT_NB(SavedMdlAllocatedNetBuffer);
        }
    }
}

#endif


