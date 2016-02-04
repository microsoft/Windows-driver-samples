
#include "MP_Precomp.h"
#include "CustomOid.h"

#if WPP_SOFTWARE_TRACE
#include "N6C_Oids.tmh"
#endif

static NDIS_OID NICSupportedOids[] =
{
    // General Oids.
    OID_GEN_SUPPORTED_LIST,  // OK
    OID_GEN_SUPPORTED_GUIDS,  // OK
    OID_GEN_HARDWARE_STATUS,  // OK
    OID_GEN_MEDIA_SUPPORTED,  // OK
    OID_GEN_MEDIA_IN_USE,  // OK
    OID_GEN_PHYSICAL_MEDIUM,  // OK
    OID_GEN_CURRENT_LOOKAHEAD,  // OK
    OID_GEN_MAXIMUM_LOOKAHEAD,  // OK
    OID_GEN_MAXIMUM_FRAME_SIZE,  // OK
    OID_GEN_CURRENT_PACKET_FILTER,  // OK
    OID_GEN_MAXIMUM_TOTAL_SIZE,  // OK
    OID_GEN_TRANSMIT_BLOCK_SIZE,  // OK
    OID_GEN_RECEIVE_BLOCK_SIZE,  // OK
    OID_GEN_MAC_OPTIONS,  // OK
    OID_GEN_LINK_SPEED,  // OK
    OID_GEN_TRANSMIT_BUFFER_SPACE,  // OK
    OID_GEN_RECEIVE_BUFFER_SPACE,  // OK
    OID_GEN_VENDOR_ID,  // OK
    OID_GEN_VENDOR_DESCRIPTION,  // OK
    OID_GEN_VENDOR_DRIVER_VERSION,  // OK
    OID_GEN_DRIVER_VERSION,  // OK
    OID_GEN_PROTOCOL_OPTIONS,  // OK
    OID_GEN_MEDIA_CONNECT_STATUS,  // OK
    OID_GEN_MAXIMUM_SEND_PACKETS,  // OK
    OID_GEN_XMIT_OK,  // OK
    OID_GEN_RCV_OK,  // OK
    OID_GEN_XMIT_ERROR,  // OK
    OID_GEN_RCV_ERROR,  // OK
    OID_GEN_RCV_NO_BUFFER,  // OK
    OID_GEN_RCV_CRC_ERROR, // OK, no implement
    OID_GEN_TRANSMIT_QUEUE_LENGTH, // OK, no implement
    OID_GEN_LINK_PARAMETERS,  // ndis6 mandatory, TODO
    OID_GEN_NETWORK_LAYER_ADDRESSES, // OK, no implement

    // 802.3 Oids.
    OID_802_3_PERMANENT_ADDRESS,  // OK
    OID_802_3_CURRENT_ADDRESS,  // OK
    OID_802_3_MULTICAST_LIST,  // OK
    OID_802_3_MAXIMUM_LIST_SIZE,  // OK
    OID_802_3_RCV_ERROR_ALIGNMENT,  // OK, no implement
    OID_802_3_XMIT_ONE_COLLISION, // OK, no implement
    OID_802_3_XMIT_MORE_COLLISIONS,  // OK, no implement
    OID_802_3_XMIT_DEFERRED,  // OK, no implement
    OID_802_3_XMIT_MAX_COLLISIONS,  // OK, no implement
    OID_802_3_RCV_OVERRUN,  // OK, no implement
    OID_802_3_XMIT_UNDERRUN,  // OK, no implement
    OID_802_3_XMIT_HEARTBEAT_FAILURE,  // OK, no implement
    OID_802_3_XMIT_TIMES_CRS_LOST,  // OK, no implement
    OID_802_3_XMIT_LATE_COLLISIONS,  // OK, no implement
#if POWER_MAN
    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ADD_WAKE_UP_PATTERN,
    OID_PNP_REMOVE_WAKE_UP_PATTERN,
    OID_PNP_ENABLE_WAKE_UP,

#endif

    // Native 802.11 Oids.
    OID_DOT11_MPDU_MAX_LENGTH,  // OK -- SC-OK
    OID_DOT11_OPERATION_MODE_CAPABILITY,  // OK -- SC-OK
    OID_DOT11_CURRENT_OPERATION_MODE,  // OK -- SC-OK
    OID_DOT11_ATIM_WINDOW,  // OK
    OID_DOT11_NIC_POWER_STATE,  // OK
    OID_DOT11_OPTIONAL_CAPABILITY,  // OK
    OID_DOT11_CURRENT_OPTIONAL_CAPABILITY,  // OK
    OID_DOT11_CF_POLLABLE,  // OK,
    OID_DOT11_OPERATIONAL_RATE_SET,  // OK
    OID_DOT11_BEACON_PERIOD,  // OK
    OID_DOT11_RTS_THRESHOLD,  // OK
    OID_DOT11_SHORT_RETRY_LIMIT,  // OK
    OID_DOT11_LONG_RETRY_LIMIT,  // OK
    OID_DOT11_FRAGMENTATION_THRESHOLD,  // OK
    OID_DOT11_MAX_TRANSMIT_MSDU_LIFETIME,  // OK
    OID_DOT11_MAX_RECEIVE_LIFETIME,  // OK
    OID_DOT11_SUPPORTED_PHY_TYPES,  // OK
    OID_DOT11_CURRENT_REG_DOMAIN,  // OK, SC-- to be reimplement.
    OID_DOT11_TEMP_TYPE,  // Rcnjko
    OID_DOT11_CURRENT_TX_ANTENNA, // Rcnjko
    OID_DOT11_DIVERSITY_SUPPORT,  // Rcnjko
    OID_DOT11_CURRENT_RX_ANTENNA,  // Rcnjko
    OID_DOT11_SUPPORTED_POWER_LEVELS,  // Rcnjko
    OID_DOT11_CURRENT_TX_POWER_LEVEL,  // Rcnjko
    OID_DOT11_PERMANENT_ADDRESS,  // OK -- SC-OK
    OID_DOT11_CURRENT_ADDRESS,  // OK -- SC-OK
    OID_DOT11_MAC_ADDRESS,  // OK -- SC-OK
    OID_DOT11_STATION_ID,  // OK -- SC-OK

    // Added
    //OID_DOT11_MAXIMUM_LIST_SIZE,
    //OID_DOT11_MULTICAST_LIST,
    //OID_DOT11_NIC_SPECIFIC_EXTENSION,
    //OID_DOT11_RECV_SENSITIVITY_LIST,
    //OID_DOT11_MULTI_DOMAIN_CAPABILITY,
    //OID_DOT11_SUPPORTED_COUNTRY_OR_REGION_STRING,
    //OID_DOT11_DESIRED_COUNTRY_OR_REGION_STRING,
    
    // ----- IHV -----
    OID_DOT11_CURRENT_CHANNEL, // SC -- SC-OK
    OID_DOT11_CCA_MODE_SUPPORTED,
    OID_DOT11_CURRENT_CCA_MODE,
    OID_DOT11_ED_THRESHOLD,
    // ----- OFDM -----
    OID_DOT11_CURRENT_FREQUENCY,
    OID_DOT11_FREQUENCY_BANDS_SUPPORTED,
    OID_DOT11_REG_DOMAINS_SUPPORT_VALUE, // SC -- to be reimplement.
    OID_DOT11_SUPPORTED_TX_ANTENNA,  // tbi
    OID_DOT11_SUPPORTED_RX_ANTENNA,  // tbi
    OID_DOT11_DIVERSITY_SELECTION_RX,  // Rcnjko
    OID_DOT11_SUPPORTED_DATA_RATES_VALUE, // Rcnjko 
    OID_DOT11_RF_USAGE,
    OID_DOT11_MAX_MAC_ADDRESS_STATES,
    OID_DOT11_MULTI_DOMAIN_CAPABILITY_IMPLEMENTED,  // return not support
    OID_DOT11_MULTI_DOMAIN_CAPABILITY_ENABLED,  // return not support
    OID_DOT11_COUNTRY_STRING, // return not support
    OID_DOT11_SHORT_PREAMBLE_OPTION_IMPLEMENTED,
    OID_DOT11_PBCC_OPTION_IMPLEMENTED,
    OID_DOT11_ERP_PBCC_OPTION_IMPLEMENTED,
    OID_DOT11_ERP_PBCC_OPTION_ENABLED,
    OID_DOT11_DSSS_OFDM_OPTION_IMPLEMENTED,
    OID_DOT11_DSSS_OFDM_OPTION_ENABLED,
    OID_DOT11_SHORT_SLOT_TIME_OPTION_IMPLEMENTED,
    OID_DOT11_SHORT_SLOT_TIME_OPTION_ENABLED,

    // (get) not simple native wifi ?
    OID_DOT11_AUTO_CONFIG_ENABLED,  // OK -- SC-OK
    OID_DOT11_POWER_MGMT_REQUEST,  // Annie					// OK (not verified)
    OID_DOT11_DESIRED_SSID_LIST,  // OK
    OID_DOT11_EXCLUDED_MAC_ADDRESS_LIST,  // OK -- SC-OK
    OID_DOT11_EXCLUDE_UNENCRYPTED,// OK, mandatory.
    OID_DOT11_DESIRED_BSSID_LIST, // OK -- SC-OK
    OID_DOT11_DESIRED_BSS_TYPE,  // OK
    OID_DOT11_STATISTICS,  // OK
    OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM,  // Annie	-- SC-OK
    OID_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR,  // Annie	-- SC-OK to check infra and adhoc
    OID_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR,  // Annie -- SC-OK to check infra and adhoc
    OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM,  // Annie	-- SC-OK
    OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM,  // Annie -- SC-OK
    OID_DOT11_CIPHER_DEFAULT_KEY_ID,  // Annie	-- SC-OK
    OID_DOT11_ENUM_ASSOCIATION_INFO, // SC -- SC-OK
    OID_DOT11_HARDWARE_PHY_STATE,  // OK -- SC-OK
    OID_DOT11_DESIRED_PHY_LIST,  // OK -- SC-OK
    OID_DOT11_CURRENT_PHY_ID,  // OK
    OID_DOT11_PMKID_LIST,  // Annie								// OK (not verified)
    OID_DOT11_MEDIA_STREAMING_ENABLED,  // OK
    OID_DOT11_UNREACHABLE_DETECTION_THRESHOLD,  // OK -- SC-OK
    OID_DOT11_ACTIVE_PHY_LIST,  // SC -- SC-OK
    OID_DOT11_EXTSTA_CAPABILITY,  // OK -- SC-OK
    OID_DOT11_DATA_RATE_MAPPING_TABLE,
    OID_DOT11_PRIVACY_EXEMPTION_LIST, // OK
    OID_DOT11_IBSS_PARAMS, // OK -- SC-OK
    OID_DOT11_UNICAST_USE_GROUP_ENABLED, // OK -- SC-OK
    OID_DOT11_SAFE_MODE_ENABLED,
    OID_DOT11_HIDDEN_NETWORK_ENABLED, // OK -- SC-OK

    // Set
    OID_DOT11_SCAN_REQUEST,  // OK
    OID_DOT11_RESET_REQUEST,  // OK
	
    // (Set) simple native wifi ?
    OID_DOT11_ENUM_BSS_LIST, // OK
    OID_DOT11_FLUSH_BSS_LIST,  // OK -- SC-OK
    OID_DOT11_CONNECT_REQUEST,  // OK
    OID_DOT11_CIPHER_DEFAULT_KEY,  // Annie  SC-OK
    OID_DOT11_CIPHER_KEY_MAPPING_KEY,  // Annie
    OID_DOT11_DISCONNECT_REQUEST,  // OK

    OID_GEN_INTERRUPT_MODERATION,
    OID_DOT11_CHANNEL_AGILITY_ENABLED,
    OID_DOT11_CHANNEL_AGILITY_PRESENT,

    OID_DOT11_ASSOCIATION_PARAMS,

    // ExtAP specific OIDs
    OID_DOT11_DTIM_PERIOD,                      // dot11DTIMPeriod
    OID_DOT11_AVAILABLE_CHANNEL_LIST,           // msDot11AvailableChannelList
    OID_DOT11_AVAILABLE_FREQUENCY_LIST,         // msDot11AvailableFrequencyList
    OID_DOT11_ENUM_PEER_INFO,                   
    OID_DOT11_DISASSOCIATE_PEER_REQUEST,        
    //OID_DOT11_PORT_STATE_NOTIFICATION,
    OID_DOT11_INCOMING_ASSOCIATION_DECISION,
    OID_DOT11_ADDITIONAL_IE,
    OID_DOT11_WPS_ENABLED,
    OID_DOT11_START_AP_REQUEST,
   // OID_DOT11_AP_BEACON_MODE,
    
    // Virtual WiFi specifc OIDs
    OID_DOT11_CREATE_MAC,
    OID_DOT11_DELETE_MAC,

    //WoWLAN PM OIDs
    OID_PM_ADD_WOL_PATTERN,
    OID_PM_REMOVE_WOL_PATTERN,
    OID_PM_PARAMETERS,
    OID_PM_ADD_PROTOCOL_OFFLOAD,
    OID_PM_GET_PROTOCOL_OFFLOAD,
    OID_PM_REMOVE_PROTOCOL_OFFLOAD,
    OID_PM_PROTOCOL_OFFLOAD_LIST,

    // Packet filter
    OID_RECEIVE_FILTER_SET_FILTER,
    OID_RECEIVE_FILTER_CLEAR_FILTER,

	// Connected standby OIDs
    OID_PACKET_COALESCING_FILTER_MATCH_COUNT,

    // Wifi Direct Support: From <windot11.h>
    // Format: ((0x0E000000U) | ((T) << 16) | ((M) << 8) | (SN))
    // 	T: 
    // 		NWF_WFD_DEVICE_OID or NWF_WFD_ROLE_OID
    //	M: 
    //		NWF_MANDATORY_OID or NWF_OPTIONAL_OID
    
    // Device OIDs: NWF_WFD_DEVICE_OID and NWF_MANDATORY_OID (some will be accepted by the role port)
    OID_DOT11_WFD_DEVICE_CAPABILITY, 
    OID_DOT11_WFD_GROUP_OWNER_CAPABILITY,
    OID_DOT11_WFD_DEVICE_INFO,
    OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST,
    OID_DOT11_WFD_DISCOVER_REQUEST,
    OID_DOT11_WFD_ENUM_DEVICE_LIST,
    OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY,
    OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL,
    OID_DOT11_WFD_ADDITIONAL_IE,
    OID_DOT11_WFD_FLUSH_DEVICE_LIST,
    OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST,
    OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE,
    OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRMATION,
    OID_DOT11_WFD_SEND_INVITATION_REQUEST,
    OID_DOT11_WFD_SEND_INVITATION_RESPONSE,
    OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST,
    OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE,
    OID_DOT11_WFD_STOP_DISCOVERY,

    // Role OIDs: NWF_WFD_ROLE_OID and NWF_MANDATORY_OID
    OID_DOT11_WFD_DESIRED_GROUP_ID,
    OID_DOT11_WFD_START_GO_REQUEST,
    OID_DOT11_WFD_GROUP_START_PARAMETERS,
    OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST,
    OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST,
    OID_DOT11_WFD_GROUP_JOIN_PARAMETERS,
    OID_DOT11_WFD_GET_DIALOG_TOKEN,

	// D0 Coalescing
    OID_PACKET_COALESCING_FILTER_MATCH_COUNT,

    // Connected standby OIDs
    OID_DOT11_POWER_MGMT_MODE_AUTO_ENABLED,
    OID_DOT11_POWER_MGMT_MODE_STATUS,
    OID_DOT11_OFFLOAD_NETWORK_LIST,

    // Realtek internal.
    OID_RT_PRO8187_WI_POLL,
    OID_RT_DEVICE_ID_INFO
};


NDIS_OID*
N6GetSupportedOids()
{
	return NICSupportedOids;
}

ULONG
N6GetSupportedOidsLength()
{
	return sizeof(NICSupportedOids);
}


NDIS_STATUS
N6CDirectOidRequest(
	IN  NDIS_HANDLE			MiniportAdapterContext,
	IN  PNDIS_OID_REQUEST	NdisRequest
	)
/*++

	MiniportDirectOidRequest is an optional function. A miniport driver registers this function 
	if it handles direct OID requests. A driver specifies the MiniportDirectOidRequest entry 
	point when it calls the NdisMRegisterMiniportDriver function. A miniport driver that registers 
	the MiniportCancelDirectOidRequest function must also register MiniportDirectOidRequest.

	NDIS calls the MiniportDirectOidRequest function either on its own behalf or on behalf of 
	a bound protocol driver that called the NdisDirectOidRequest function. Miniport drivers 
	should examine the request that is supplied at the OidRequest parameter and take the 
	action requested.

	Note that NDIS does not validate the OID-specific contents at OidRequest. Therefore, 
	the driver itself must validate these contents. If the driver determines that the value to 
	be set is out of bounds, it should fail the request and return NDIS_STATUS_INVALID_DATA.

	NDIS does not serialize requests that it sends to MiniportDirectOidRequest with other OID 
	requests. The miniport driver must be able to handle multiple calls to MiniportDirectOidRequest 
	when other requests that are sent to MiniportOidRequest or MiniportDirectOidRequest are 
	outstanding.

	NDIS calls MiniportDirectOidRequest at IRQL <= DISPATCH_LEVEL.
	
--*/
{	
	PADAPTER		Adapter = (PADAPTER)MiniportAdapterContext;
	u2Byte			i = 0;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_NOT_SUPPORTED;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> N6PciDirectOidRequest(): to port %u\n", NdisRequest->PortNumber));

	ndisStatus = WDI_HandleOidRequest(
						MiniportAdapterContext,
						NdisRequest
						);

	if(NDIS_STATUS_NOT_RECOGNIZED == ndisStatus)
	{
		ndisStatus = N6CProcessOidRequest(Adapter, NdisRequest, FALSE);
	}
	
	RT_ASSERT(ndisStatus != NDIS_STATUS_PENDING, ("N6CDirectOidRequest(): There is no pending OID handler!! (%x)\n", NdisRequest->DATA.QUERY_INFORMATION.Oid));

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== N6PciDirectOidRequest(): %u.\n", ndisStatus));

	return ndisStatus;
}

VOID
N6CCancelDirectOidRequest(
	IN NDIS_HANDLE 		hMiniportAdapterContext,
	IN PVOID       			RequestId
	)
{
	UNREFERENCED_PARAMETER(hMiniportAdapterContext);
	UNREFERENCED_PARAMETER(RequestId);
}

// Method operation for OID_DOT11_RESET_REQUEST
NDIS_STATUS
N6C_METHOD_OID_DOT11_RESET_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InputBufferLength,
	IN	ULONG			OutputBufferLength,
	IN	ULONG			MethodId,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus 	= NDIS_STATUS_SUCCESS;
	PMGNT_INFO		pMgntInfo 	= &(pTargetAdapter->MgntInfo);
	u4Byte 			id 			= 0;


	PRT_NDIS62_COMMON pTargetNdis62Common = pTargetAdapter->pNdis62Common;
#if (P2P_SUPPORT == 1)
	PP2P_INFO	pP2PInfo = GET_P2P_INFO(pTargetAdapter);
#endif
	P2P_ROLE	currentRole = P2P_NONE;

	RT_TRACE_F(COMP_OID_SET, DBG_LOUD,
		("Port = %d, PortType = %d\n", pTargetAdapter->pNdis62Common->PortNumber, pTargetAdapter->pNdis62Common->PortType));
	
	pTargetNdis62Common->CurrentOpState = INIT_STATE;

{
#if (P2P_SUPPORT == 1)
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pTargetAdapter))
		currentRole = pP2PInfo->Role;
#endif

	if(currentRole != P2P_NONE)
	{ 
		ndisStatus = N63CResetWifiDirectPorts(pTargetAdapter, currentRole, RESET_LEVEL_P2P_ONLY);
	}

	pTargetAdapter->pNdis62Common->bDot11SetPhyIdReady = TRUE;
	N63CResetNetworkListOffload(pTargetAdapter);

	if(pMgntInfo->bSupportPacketCoalescing)
		N6FulshD0CoalescingQueue(pTargetAdapter, TRUE, FALSE);
	
}

	if(pTargetNdis62Common->PortType == EXTAP_PORT || currentRole == P2P_GO)
	{
		ndisStatus = N62CAPResetRequest(
				pTargetAdapter,
				InformationBuffer,
				InputBufferLength,
				OutputBufferLength,
				BytesWritten,
				BytesRead,
				BytesNeeded
			);
	}
	else if(pTargetNdis62Common->PortType == EXTSTA_PORT || currentRole == P2P_DEVICE || currentRole == P2P_CLIENT)
	{
		ndisStatus = N6CQuerySet_DOT11_RESET_REQUEST(
				pTargetAdapter,
				InformationBuffer,
				InputBufferLength,
				OutputBufferLength,
				BytesWritten,
				BytesRead,
				BytesNeeded
			);
	}

	FunctionOut(COMP_OID_SET);

	return ndisStatus;
}	

NDIS_STATUS
N6CValidateOIDCorrectness(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	// Validate N62C Specific OID

	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	ndisStatus = NDIS_STATUS_SUCCESS;
	
	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_RESET_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("=====>N6C_DOT11_RESET_REQUEST()\n"));

	RT_SET_DRV_STATE(pAdapter, DrvStateResetting);
	
	do
	{
		// This should be updated to fit the state 
		
		// Validate if the OID is issued in the correct state and mode
		//ndisStatus = n63cValidateOIDCorrectness(pAdapter, NdisRequest);
		//if(ndisStatus != NDIS_STATUS_SUCCESS) {
		//	break;
		//}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = N6C_METHOD_OID_DOT11_RESET_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.METHOD_INFORMATION.Oid,
						NdisRequest->DATA.METHOD_INFORMATION.InformationBuffer,
						NdisRequest->DATA.METHOD_INFORMATION.InputBufferLength,
						NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength,
						NdisRequest->DATA.METHOD_INFORMATION.MethodId,
						(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesNeeded 
					);
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
	
	RT_CLEAR_DRV_STATE(pAdapter, DrvStateResetting);
	
	RT_TRACE(COMP_MLME, DBG_LOUD,("<====N6C_DOT11_RESET_REQUEST(): status=%d\n", ndisStatus));	

	return ndisStatus;
}


NDIS_STATUS
N6C_OID_DOT11_CURRENT_OPERATION_MODE(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
	)
{
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(pAdapter);
	u1Byte	WaitCnt = 0;
	NDIS_STATUS	ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
	PADAPTER	pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);
	PRT_NDIS62_COMMON pTargetNdis62Common = pTargetAdapter->pNdis62Common;

	//Fixed sometime start ap will file and it shows not available issue, by Maddest 20090731.
	while(MgntScanInProgress(&pDefaultAdapter->MgntInfo))
	{
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("scan in progress\n"));
		delay_ms(50);
		WaitCnt++;
		if(WaitCnt>30)
			break;
		
	}	

	switch(NdisRequest->RequestType)
	{
		case NdisRequestSetInformation:

		if (pTargetNdis62Common->CurrentOpState == INIT_STATE)
		{
			ndisStatus = N6CSet_DOT11_CURRENT_OPERATION_MODE(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
			
			N62CChangePortTypeByOpMode(
				pTargetAdapter, 
				(PDOT11_CURRENT_OPERATION_MODE)NdisRequest->DATA.SET_INFORMATION.InformationBuffer
				);// need to check return status ?? Neo test 123

		}
		else
		{
			ndisStatus = NDIS_STATUS_INVALID_STATE;
		}
		
		break;
		case NdisRequestQueryInformation:
			ndisStatus = N6CQuery_DOT11_CURRENT_OPERATION_MODE(
						pTargetAdapter, 
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded
						);
			break;
		case NdisRequestMethod:
			break;
		default:
			break;
	}
		
	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_DESIRED_PHY_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_DESIRED_PHY_LIST(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_DOT11_DESIRED_PHY_LIST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_AUTO_CONFIG_ENABLED(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);

		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_AUTO_CONFIG_ENABLED(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_DOT11_AUTO_CONFIG_ENABLED(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_BEACON_PERIOD(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_BEACON_PERIOD(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_DOT11_BEACON_PERIOD(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_DTIM_PERIOD(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_DOT11_DTIM_PERIOD(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_DESIRED_SSID_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_DESIRED_SSID_LIST(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_DOT11_DESIRED_SSID_LIST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}


NDIS_STATUS
N6C_OID_GEN_CURRENT_PACKET_FILTER(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_GEN_CURRENT_PACKET_FILTER(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_GEN_CURRENT_PACKET_FILTER(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_CURRENT_CHANNEL(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_CURRENT_CHANNEL(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_DOT11_CURRENT_CHANNEL(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_DISCONNECT_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_DOT11_DISCONNECT_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_CONNECT_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_DOT11_CONNECT_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_GEN_LINK_PARAMETERS(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_GEN_LINK_PARAMETERS(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_GEN_LINK_PARAMETERS(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_SAFE_MODE_ENABLED(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_SAFE_MODE_ENABLED(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
#if 0				
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
#else
				ndisStatus = N6C_SET_OID_DOT11_SAFE_MODE_ENABLED(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
#endif
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_CURRENT_PHY_ID(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_CURRENT_PHY_ID(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_DOT11_CURRENT_PHY_ID(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_GEN_SUPPORTED_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_GEN_SUPPORTED_LIST(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}


NDIS_STATUS
N6C_OID_GEN_VENDOR_DRIVER_VERSION(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_GEN_VENDOR_DRIVER_VERSION(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}


NDIS_STATUS
N6C_OID_PNP_CAPABILITIES(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_PNP_CAPABILITIES(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}


NDIS_STATUS
N6C_OID_PNP_QUERY_POWER(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_PNP_QUERY_POWER(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_PNP_ENABLE_WAKE_UP(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_PNP_ENABLE_WAKE_UP(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_PNP_ENABLE_WAKE_UP(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}


NDIS_STATUS
N6C_OID_DOT11_CURRENT_TX_ANTENNA(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_CURRENT_TX_ANTENNA(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_CURRENT_RX_ANTENNA(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_CURRENT_RX_ANTENNA(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_SUPPORTED_RX_ANTENNA(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N6C_QUERY_OID_DOT11_SUPPORTED_RX_ANTENNA(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_PNP_ADD_WAKE_UP_PATTERN(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_PNP_ADD_WAKE_UP_PATTERN(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}


NDIS_STATUS
N6C_OID_PNP_REMOVE_WAKE_UP_PATTERN(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_PNP_REMOVE_WAKE_UP_PATTERN(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_PNP_SET_POWER(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N6CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N6C_SET_OID_PNP_SET_POWER(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

//
// Description:
//	Dump OID string for further debugging.
//
VOID
N6C_DOT11_DUMP_OID(
	NDIS_OID Oid
	)
{
	LPSTR	StrBuf = "";
	
	switch (Oid) {
		case OID_GEN_STATISTICS : StrBuf = "OID_GEN_STATISTICS"; break;

		case OID_GEN_SUPPORTED_LIST : StrBuf = "OID_GEN_SUPPORTED_LIST"; break;
		case OID_GEN_SUPPORTED_GUIDS : StrBuf = "OID_GEN_SUPPORTED_GUIDS"; break;
		case OID_GEN_HARDWARE_STATUS : StrBuf = "OID_GEN_HARDWARE_STATUS"; break;
		case OID_GEN_MEDIA_SUPPORTED : StrBuf = "OID_GEN_MEDIA_SUPPORTED"; break;
		case OID_GEN_MEDIA_IN_USE : StrBuf = "OID_GEN_MEDIA_IN_USE"; break;
		case OID_GEN_PHYSICAL_MEDIUM : StrBuf = "OID_GEN_PHYSICAL_MEDIUM"; break;
		case OID_GEN_CURRENT_LOOKAHEAD : StrBuf = "OID_GEN_CURRENT_LOOKAHEAD"; break;
		case OID_GEN_MAXIMUM_LOOKAHEAD : StrBuf = "OID_GEN_MAXIMUM_LOOKAHEAD"; break;
		case OID_GEN_MAXIMUM_FRAME_SIZE : StrBuf = "OID_GEN_MAXIMUM_FRAME_SIZE"; break;
		case OID_GEN_CURRENT_PACKET_FILTER : StrBuf = "OID_GEN_CURRENT_PACKET_FILTER"; break;
		case OID_GEN_MAXIMUM_TOTAL_SIZE : StrBuf = "OID_GEN_MAXIMUM_TOTAL_SIZE"; break;
		case OID_GEN_TRANSMIT_BLOCK_SIZE : StrBuf = "OID_GEN_TRANSMIT_BLOCK_SIZE"; break;
		case OID_GEN_RECEIVE_BLOCK_SIZE : StrBuf = "OID_GEN_RECEIVE_BLOCK_SIZE"; break;
		case OID_GEN_MAC_OPTIONS : StrBuf = "OID_GEN_MAC_OPTIONS"; break;
		case OID_GEN_LINK_SPEED : StrBuf = "OID_GEN_LINK_SPEED"; break;
		case OID_GEN_TRANSMIT_BUFFER_SPACE : StrBuf = "OID_GEN_TRANSMIT_BUFFER_SPACE"; break;
		case OID_GEN_RECEIVE_BUFFER_SPACE : StrBuf = "OID_GEN_RECEIVE_BUFFER_SPACE"; break;
		case OID_GEN_VENDOR_ID : StrBuf = "OID_GEN_VENDOR_ID"; break;
		case OID_GEN_VENDOR_DESCRIPTION : StrBuf = "OID_GEN_VENDOR_DESCRIPTION"; break;
		case OID_GEN_VENDOR_DRIVER_VERSION : StrBuf = "OID_GEN_VENDOR_DRIVER_VERSION"; break;
		case OID_GEN_DRIVER_VERSION : StrBuf = "OID_GEN_DRIVER_VERSION"; break;
		case OID_GEN_PROTOCOL_OPTIONS : StrBuf = "OID_GEN_PROTOCOL_OPTIONS"; break;
		case OID_GEN_MEDIA_CONNECT_STATUS : StrBuf = "OID_GEN_MEDIA_CONNECT_STATUS"; break;
		case OID_GEN_MAXIMUM_SEND_PACKETS : StrBuf = "OID_GEN_MAXIMUM_SEND_PACKETS"; break;
		case OID_GEN_XMIT_OK : StrBuf = "OID_GEN_XMIT_OK"; break;
		case OID_GEN_RCV_OK : StrBuf = "OID_GEN_RCV_OK"; break;
		case OID_GEN_XMIT_ERROR : StrBuf = "OID_GEN_XMIT_ERROR"; break;
		case OID_GEN_RCV_ERROR : StrBuf = "OID_GEN_RCV_ERROR"; break;
		case OID_GEN_RCV_NO_BUFFER : StrBuf = "OID_GEN_RCV_NO_BUFFER"; break;
		case OID_GEN_RCV_CRC_ERROR : StrBuf = "OID_GEN_RCV_CRC_ERROR"; break;
		case OID_GEN_TRANSMIT_QUEUE_LENGTH : StrBuf = "OID_GEN_TRANSMIT_QUEUE_LENGTH"; break;
		case OID_GEN_LINK_PARAMETERS : StrBuf = "OID_GEN_LINK_PARAMETERS"; break;
		case OID_GEN_NETWORK_LAYER_ADDRESSES : StrBuf = "OID_GEN_NETWORK_LAYER_ADDRESSES"; break;

		
		case OID_802_3_PERMANENT_ADDRESS : StrBuf = "OID_802_3_PERMANENT_ADDRESS"; break;
		case OID_802_3_CURRENT_ADDRESS : StrBuf = "OID_802_3_CURRENT_ADDRESS"; break;
		case OID_802_3_MULTICAST_LIST : StrBuf = "OID_802_3_MULTICAST_LIST"; break;
		case OID_802_3_MAXIMUM_LIST_SIZE : StrBuf = "OID_802_3_MAXIMUM_LIST_SIZE"; break;
		case OID_802_3_RCV_ERROR_ALIGNMENT : StrBuf = "OID_802_3_RCV_ERROR_ALIGNMENT"; break;
		case OID_802_3_XMIT_ONE_COLLISION : StrBuf = "OID_802_3_XMIT_ONE_COLLISION"; break;
		case OID_802_3_XMIT_MORE_COLLISIONS : StrBuf = "OID_802_3_XMIT_MORE_COLLISIONS"; break;
		case OID_802_3_XMIT_DEFERRED : StrBuf = "OID_802_3_XMIT_DEFERRED"; break;
		case OID_802_3_XMIT_MAX_COLLISIONS : StrBuf = "OID_802_3_XMIT_MAX_COLLISIONS"; break;
		case OID_802_3_RCV_OVERRUN : StrBuf = "OID_802_3_RCV_OVERRUN"; break;
		case OID_802_3_XMIT_UNDERRUN : StrBuf = "OID_802_3_XMIT_UNDERRUN"; break;
		case OID_802_3_XMIT_HEARTBEAT_FAILURE : StrBuf = "OID_802_3_XMIT_HEARTBEAT_FAILURE"; break;
		case OID_802_3_XMIT_TIMES_CRS_LOST : StrBuf = "OID_802_3_XMIT_TIMES_CRS_LOST"; break;
		case OID_802_3_XMIT_LATE_COLLISIONS : StrBuf = "OID_802_3_XMIT_LATE_COLLISIONS"; break;
#if POWER_MAN 
		case OID_PNP_CAPABILITIES : StrBuf = "OID_PNP_CAPABILITIES"; break;
		case OID_PNP_SET_POWER : StrBuf = "OID_PNP_SET_POWER"; break;
		case OID_PNP_QUERY_POWER : StrBuf = "OID_PNP_QUERY_POWER"; break;
		case OID_PNP_ADD_WAKE_UP_PATTERN : StrBuf = "OID_PNP_ADD_WAKE_UP_PATTERN"; break;
		case OID_PNP_REMOVE_WAKE_UP_PATTERN : StrBuf = "OID_PNP_REMOVE_WAKE_UP_PATTERN"; break;
		case OID_PNP_ENABLE_WAKE_UP : StrBuf = "OID_PNP_ENABLE_WAKE_UP"; break;
#endif

		case OID_DOT11_MPDU_MAX_LENGTH : StrBuf = "OID_DOT11_MPDU_MAX_LENGTH"; break;
		case OID_DOT11_OPERATION_MODE_CAPABILITY : StrBuf = "OID_DOT11_OPERATION_MODE_CAPABILITY"; break;
		case OID_DOT11_CURRENT_OPERATION_MODE : StrBuf = "OID_DOT11_CURRENT_OPERATION_MODE"; break;
		case OID_DOT11_ATIM_WINDOW : StrBuf = "OID_DOT11_ATIM_WINDOW"; break;
		case OID_DOT11_NIC_POWER_STATE : StrBuf = "OID_DOT11_NIC_POWER_STATE"; break;
		case OID_DOT11_OPTIONAL_CAPABILITY : StrBuf = "OID_DOT11_OPTIONAL_CAPABILITY"; break;
		case OID_DOT11_CURRENT_OPTIONAL_CAPABILITY : StrBuf = "OID_DOT11_CURRENT_OPTIONAL_CAPABILITY"; break;
		case OID_DOT11_CF_POLLABLE : StrBuf = "OID_DOT11_CF_POLLABLE"; break;
		case OID_DOT11_OPERATIONAL_RATE_SET : StrBuf = "OID_DOT11_OPERATIONAL_RATE_SET"; break;
		case OID_DOT11_BEACON_PERIOD : StrBuf = "OID_DOT11_BEACON_PERIOD"; break;
		case OID_DOT11_RTS_THRESHOLD : StrBuf = "OID_DOT11_RTS_THRESHOLD"; break;
		case OID_DOT11_SHORT_RETRY_LIMIT : StrBuf = "OID_DOT11_SHORT_RETRY_LIMIT"; break;
		case OID_DOT11_LONG_RETRY_LIMIT : StrBuf = "OID_DOT11_LONG_RETRY_LIMIT"; break;
		case OID_DOT11_FRAGMENTATION_THRESHOLD : StrBuf = "OID_DOT11_FRAGMENTATION_THRESHOLD"; break;
		case OID_DOT11_MAX_TRANSMIT_MSDU_LIFETIME : StrBuf = "OID_DOT11_MAX_TRANSMIT_MSDU_LIFETIME"; break;
		case OID_DOT11_MAX_RECEIVE_LIFETIME : StrBuf = "OID_DOT11_MAX_RECEIVE_LIFETIME"; break;
		case OID_DOT11_SUPPORTED_PHY_TYPES : StrBuf = "OID_DOT11_SUPPORTED_PHY_TYPES"; break;
		case OID_DOT11_CURRENT_REG_DOMAIN : StrBuf = "OID_DOT11_CURRENT_REG_DOMAIN"; break;
		case OID_DOT11_TEMP_TYPE : StrBuf = "OID_DOT11_TEMP_TYPE"; break;
		case OID_DOT11_CURRENT_TX_ANTENNA : StrBuf = "OID_DOT11_CURRENT_TX_ANTENNA"; break;
		case OID_DOT11_DIVERSITY_SUPPORT : StrBuf = "OID_DOT11_DIVERSITY_SUPPORT"; break;
		case OID_DOT11_CURRENT_RX_ANTENNA : StrBuf = "OID_DOT11_CURRENT_RX_ANTENNA"; break;
		case OID_DOT11_SUPPORTED_POWER_LEVELS : StrBuf = "OID_DOT11_SUPPORTED_POWER_LEVELS"; break;
		case OID_DOT11_CURRENT_TX_POWER_LEVEL : StrBuf = "OID_DOT11_CURRENT_TX_POWER_LEVEL"; break;
		case OID_DOT11_PERMANENT_ADDRESS : StrBuf = "OID_DOT11_PERMANENT_ADDRESS"; break;
		case OID_DOT11_CURRENT_ADDRESS : StrBuf = "OID_DOT11_CURRENT_ADDRESS"; break;
		case OID_DOT11_MAC_ADDRESS : StrBuf = "OID_DOT11_MAC_ADDRESS"; break;
		case OID_DOT11_STATION_ID : StrBuf = "OID_DOT11_STATION_ID"; break;
		
		case OID_DOT11_CURRENT_CHANNEL : StrBuf = "OID_DOT11_CURRENT_CHANNEL"; break;
		case OID_DOT11_CCA_MODE_SUPPORTED : StrBuf = "OID_DOT11_CCA_MODE_SUPPORTED"; break;
		case OID_DOT11_CURRENT_CCA_MODE : StrBuf = "OID_DOT11_CURRENT_CCA_MODE"; break;
		case OID_DOT11_ED_THRESHOLD : StrBuf = "OID_DOT11_ED_THRESHOLD"; break;
		
		case OID_DOT11_CURRENT_FREQUENCY : StrBuf = "OID_DOT11_CURRENT_FREQUENCY"; break;
		case OID_DOT11_FREQUENCY_BANDS_SUPPORTED : StrBuf = "OID_DOT11_FREQUENCY_BANDS_SUPPORTED"; break;
		case OID_DOT11_REG_DOMAINS_SUPPORT_VALUE : StrBuf = "OID_DOT11_REG_DOMAINS_SUPPORT_VALUE"; break;
		case OID_DOT11_SUPPORTED_TX_ANTENNA : StrBuf = "OID_DOT11_SUPPORTED_TX_ANTENNA"; break;
		case OID_DOT11_SUPPORTED_RX_ANTENNA : StrBuf = "OID_DOT11_SUPPORTED_RX_ANTENNA"; break;
		case OID_DOT11_DIVERSITY_SELECTION_RX : StrBuf = "OID_DOT11_DIVERSITY_SELECTION_RX"; break;
		case OID_DOT11_SUPPORTED_DATA_RATES_VALUE : StrBuf = "OID_DOT11_SUPPORTED_DATA_RATES_VALUE"; break;
		case OID_DOT11_RF_USAGE : StrBuf = "OID_DOT11_RF_USAGE"; break;
		case OID_DOT11_MAX_MAC_ADDRESS_STATES : StrBuf = "OID_DOT11_MAX_MAC_ADDRESS_STATES"; break;
		case OID_DOT11_MULTI_DOMAIN_CAPABILITY_IMPLEMENTED : StrBuf = "OID_DOT11_MULTI_DOMAIN_CAPABILITY_IMPLEMENTED"; break;
		case OID_DOT11_MULTI_DOMAIN_CAPABILITY_ENABLED : StrBuf = "OID_DOT11_MULTI_DOMAIN_CAPABILITY_ENABLED"; break;
		case OID_DOT11_COUNTRY_STRING : StrBuf = "OID_DOT11_COUNTRY_STRING"; break;
		case OID_DOT11_SHORT_PREAMBLE_OPTION_IMPLEMENTED : StrBuf = "OID_DOT11_SHORT_PREAMBLE_OPTION_IMPLEMENTED"; break;
		case OID_DOT11_PBCC_OPTION_IMPLEMENTED : StrBuf = "OID_DOT11_PBCC_OPTION_IMPLEMENTED"; break;
		case OID_DOT11_ERP_PBCC_OPTION_IMPLEMENTED : StrBuf = "OID_DOT11_ERP_PBCC_OPTION_IMPLEMENTED"; break;
		case OID_DOT11_ERP_PBCC_OPTION_ENABLED : StrBuf = "OID_DOT11_ERP_PBCC_OPTION_ENABLED"; break;
		case OID_DOT11_DSSS_OFDM_OPTION_IMPLEMENTED : StrBuf = "OID_DOT11_DSSS_OFDM_OPTION_IMPLEMENTED"; break;
		case OID_DOT11_DSSS_OFDM_OPTION_ENABLED : StrBuf = "OID_DOT11_DSSS_OFDM_OPTION_ENABLED"; break;
		case OID_DOT11_SHORT_SLOT_TIME_OPTION_IMPLEMENTED : StrBuf = "OID_DOT11_SHORT_SLOT_TIME_OPTION_IMPLEMENTED"; break;
		case OID_DOT11_SHORT_SLOT_TIME_OPTION_ENABLED : StrBuf = "OID_DOT11_SHORT_SLOT_TIME_OPTION_ENABLED"; break;
		
		case OID_DOT11_AUTO_CONFIG_ENABLED : StrBuf = "OID_DOT11_AUTO_CONFIG_ENABLED"; break;
		case OID_DOT11_POWER_MGMT_REQUEST : StrBuf = "OID_DOT11_POWER_MGMT_REQUEST"; break;
		case OID_DOT11_DESIRED_SSID_LIST : StrBuf = "OID_DOT11_DESIRED_SSID_LIST"; break;
		case OID_DOT11_EXCLUDED_MAC_ADDRESS_LIST : StrBuf = "OID_DOT11_EXCLUDED_MAC_ADDRESS_LIST"; break;
		case OID_DOT11_EXCLUDE_UNENCRYPTED : StrBuf = "OID_DOT11_EXCLUDE_UNENCRYPTED"; break;
		case OID_DOT11_DESIRED_BSSID_LIST : StrBuf = "OID_DOT11_DESIRED_BSSID_LIST"; break;
		case OID_DOT11_DESIRED_BSS_TYPE : StrBuf = "OID_DOT11_DESIRED_BSS_TYPE"; break;
		case OID_DOT11_STATISTICS : StrBuf = "OID_DOT11_STATISTICS"; break;
		case OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM : StrBuf = "OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM"; break;
		case OID_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR : StrBuf = "OID_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR"; break;
		case OID_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR : StrBuf = "OID_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR"; break;
		case OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM : StrBuf = "OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM"; break;
		case OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM : StrBuf = "OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM"; break;
		case OID_DOT11_CIPHER_DEFAULT_KEY_ID : StrBuf = "OID_DOT11_CIPHER_DEFAULT_KEY_ID"; break;
		case OID_DOT11_ENUM_ASSOCIATION_INFO : StrBuf = "OID_DOT11_ENUM_ASSOCIATION_INFO"; break;
		case OID_DOT11_HARDWARE_PHY_STATE : StrBuf = "OID_DOT11_HARDWARE_PHY_STATE"; break;
		case OID_DOT11_DESIRED_PHY_LIST : StrBuf = "OID_DOT11_DESIRED_PHY_LIST"; break;
		case OID_DOT11_CURRENT_PHY_ID : StrBuf = "OID_DOT11_CURRENT_PHY_ID"; break;
		case OID_DOT11_PMKID_LIST : StrBuf = "OID_DOT11_PMKID_LIST"; break;
		case OID_DOT11_MEDIA_STREAMING_ENABLED : StrBuf = "OID_DOT11_MEDIA_STREAMING_ENABLED"; break;
		case OID_DOT11_UNREACHABLE_DETECTION_THRESHOLD : StrBuf = "OID_DOT11_UNREACHABLE_DETECTION_THRESHOLD"; break;
		case OID_DOT11_ACTIVE_PHY_LIST : StrBuf = "OID_DOT11_ACTIVE_PHY_LIST"; break;
		case OID_DOT11_EXTSTA_CAPABILITY : StrBuf = "OID_DOT11_EXTSTA_CAPABILITY"; break;
		case OID_DOT11_DATA_RATE_MAPPING_TABLE : StrBuf = "OID_DOT11_DATA_RATE_MAPPING_TABLE"; break;
		case OID_DOT11_PRIVACY_EXEMPTION_LIST : StrBuf = "OID_DOT11_PRIVACY_EXEMPTION_LIST"; break;
		case OID_DOT11_IBSS_PARAMS : StrBuf = "OID_DOT11_IBSS_PARAMS"; break;
		case OID_DOT11_UNICAST_USE_GROUP_ENABLED : StrBuf = "OID_DOT11_UNICAST_USE_GROUP_ENABLED"; break;
		case OID_DOT11_SAFE_MODE_ENABLED : StrBuf = "OID_DOT11_SAFE_MODE_ENABLED"; break;
		case OID_DOT11_HIDDEN_NETWORK_ENABLED : StrBuf = "OID_DOT11_HIDDEN_NETWORK_ENABLED"; break;

		case OID_DOT11_SCAN_REQUEST : StrBuf = "OID_DOT11_SCAN_REQUEST"; break;
		case OID_DOT11_RESET_REQUEST : StrBuf = "OID_DOT11_RESET_REQUEST"; break;

		case OID_DOT11_ENUM_BSS_LIST : StrBuf = "OID_DOT11_ENUM_BSS_LIST"; break;
		case OID_DOT11_FLUSH_BSS_LIST : StrBuf = "OID_DOT11_FLUSH_BSS_LIST"; break;
		case OID_DOT11_CONNECT_REQUEST : StrBuf = "OID_DOT11_CONNECT_REQUEST"; break;
		case OID_DOT11_CIPHER_DEFAULT_KEY : StrBuf = "OID_DOT11_CIPHER_DEFAULT_KEY"; break;
		case OID_DOT11_CIPHER_KEY_MAPPING_KEY : StrBuf = "OID_DOT11_CIPHER_KEY_MAPPING_KEY"; break;
		case OID_DOT11_DISCONNECT_REQUEST : StrBuf = "OID_DOT11_DISCONNECT_REQUEST"; break;


		case OID_GEN_INTERRUPT_MODERATION : StrBuf = "OID_GEN_INTERRUPT_MODERATION"; break;
		case OID_DOT11_CHANNEL_AGILITY_ENABLED : StrBuf = "OID_DOT11_CHANNEL_AGILITY_ENABLED"; break;
		case OID_DOT11_CHANNEL_AGILITY_PRESENT : StrBuf = "OID_DOT11_CHANNEL_AGILITY_PRESENT"; break;
		
		case OID_DOT11_ASSOCIATION_PARAMS : StrBuf = "OID_DOT11_ASSOCIATION_PARAMS"; break;
		
		case OID_DOT11_DTIM_PERIOD : StrBuf = "OID_DOT11_DTIM_PERIOD"; break;
		case OID_DOT11_AVAILABLE_CHANNEL_LIST : StrBuf = "OID_DOT11_AVAILABLE_CHANNEL_LIST"; break;
		case OID_DOT11_AVAILABLE_FREQUENCY_LIST : StrBuf = "OID_DOT11_AVAILABLE_FREQUENCY_LIST"; break;
		case OID_DOT11_ENUM_PEER_INFO : StrBuf = "OID_DOT11_ENUM_PEER_INFO"; break;
		case OID_DOT11_DISASSOCIATE_PEER_REQUEST : StrBuf = "OID_DOT11_DISASSOCIATE_PEER_REQUEST"; break;
		case OID_DOT11_INCOMING_ASSOCIATION_DECISION : StrBuf = "OID_DOT11_INCOMING_ASSOCIATION_DECISION"; break;
		case OID_DOT11_ADDITIONAL_IE : StrBuf = "OID_DOT11_ADDITIONAL_IE"; break;
		case OID_DOT11_WPS_ENABLED : StrBuf = "OID_DOT11_WPS_ENABLED"; break;
		case OID_DOT11_START_AP_REQUEST : StrBuf = "OID_DOT11_START_AP_REQUEST"; break;
		
		case OID_DOT11_CREATE_MAC : StrBuf = "OID_DOT11_CREATE_MAC"; break;
		case OID_DOT11_DELETE_MAC : StrBuf = "OID_DOT11_DELETE_MAC"; break;
		
		case OID_PM_ADD_WOL_PATTERN : StrBuf = "OID_PM_ADD_WOL_PATTERN"; break;
		case OID_PM_REMOVE_WOL_PATTERN : StrBuf = "OID_PM_REMOVE_WOL_PATTERN"; break;
		case OID_PM_PARAMETERS : StrBuf = "OID_PM_PARAMETERS"; break;
		case OID_PM_ADD_PROTOCOL_OFFLOAD : StrBuf = "OID_PM_ADD_PROTOCOL_OFFLOAD"; break;
		case OID_PM_GET_PROTOCOL_OFFLOAD : StrBuf = "OID_PM_GET_PROTOCOL_OFFLOAD"; break;
		case OID_PM_REMOVE_PROTOCOL_OFFLOAD : StrBuf = "OID_PM_REMOVE_PROTOCOL_OFFLOAD"; break;		


		case OID_RECEIVE_FILTER_SET_FILTER : StrBuf = "OID_RECEIVE_FILTER_SET_FILTER"; break;
		case OID_RECEIVE_FILTER_CLEAR_FILTER : StrBuf = "OID_RECEIVE_FILTER_CLEAR_FILTER"; break;
		
		case OID_DOT11_WFD_DEVICE_CAPABILITY : StrBuf = "OID_DOT11_WFD_DEVICE_CAPABILITY"; break;
		case OID_DOT11_WFD_GROUP_OWNER_CAPABILITY : StrBuf = "OID_DOT11_WFD_GROUP_OWNER_CAPABILITY"; break;
		case OID_DOT11_WFD_DEVICE_INFO : StrBuf = "OID_DOT11_WFD_DEVICE_INFO"; break;
		case OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST : StrBuf = "OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST"; break;
		case OID_DOT11_WFD_DISCOVER_REQUEST : StrBuf = "OID_DOT11_WFD_DISCOVER_REQUEST"; break;
		case OID_DOT11_WFD_ENUM_DEVICE_LIST : StrBuf = "OID_DOT11_WFD_ENUM_DEVICE_LIST"; break;
		case OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY : StrBuf = "OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY"; break;
		case OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL : StrBuf = "OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL"; break;
		case OID_DOT11_WFD_ADDITIONAL_IE : StrBuf = "OID_DOT11_WFD_ADDITIONAL_IE"; break;
		case OID_DOT11_WFD_FLUSH_DEVICE_LIST : StrBuf = "OID_DOT11_WFD_FLUSH_DEVICE_LIST"; break;
		case OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST : StrBuf = "OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST"; break;
		case OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE : StrBuf = "OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE"; break;
		case OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRMATION : StrBuf = "OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRMATION"; break;
		case OID_DOT11_WFD_SEND_INVITATION_REQUEST : StrBuf = "OID_DOT11_WFD_SEND_INVITATION_REQUEST"; break;
		case OID_DOT11_WFD_SEND_INVITATION_RESPONSE : StrBuf = "OID_DOT11_WFD_SEND_INVITATION_RESPONSE"; break;
		case OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST : StrBuf = "OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST"; break;
		case OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE : StrBuf = "OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE"; break;
		case OID_DOT11_WFD_STOP_DISCOVERY : StrBuf = "OID_DOT11_WFD_STOP_DISCOVERY"; break;
		
		case OID_DOT11_WFD_DESIRED_GROUP_ID : StrBuf = "OID_DOT11_WFD_DESIRED_GROUP_ID"; break;
		case OID_DOT11_WFD_START_GO_REQUEST : StrBuf = "OID_DOT11_WFD_START_GO_REQUEST"; break;
		case OID_DOT11_WFD_GROUP_START_PARAMETERS : StrBuf = "OID_DOT11_WFD_GROUP_START_PARAMETERS"; break;
		case OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST : StrBuf = "OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST"; break;
		case OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST : StrBuf = "OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST"; break;
		case OID_DOT11_WFD_GROUP_JOIN_PARAMETERS : StrBuf = "OID_DOT11_WFD_GROUP_JOIN_PARAMETERS"; break;
		case OID_DOT11_WFD_GET_DIALOG_TOKEN : StrBuf = "OID_DOT11_WFD_GET_DIALOG_TOKEN"; break;
		
		case OID_PACKET_COALESCING_FILTER_MATCH_COUNT : StrBuf = "OID_PACKET_COALESCING_FILTER_MATCH_COUNT"; break;
		
		case OID_DOT11_POWER_MGMT_MODE_AUTO_ENABLED : StrBuf = "OID_DOT11_POWER_MGMT_MODE_AUTO_ENABLED"; break;
		case OID_DOT11_POWER_MGMT_MODE_STATUS : StrBuf = "OID_DOT11_POWER_MGMT_MODE_STATUS"; break;
		case OID_DOT11_OFFLOAD_NETWORK_LIST : StrBuf = "OID_DOT11_OFFLOAD_NETWORK_LIST"; break;

		case OID_RT_PRO8187_WI_POLL : StrBuf = "OID_RT_PRO8187_WI_POLL"; break;
		case OID_RT_DEVICE_ID_INFO : StrBuf = "OID_RT_DEVICE_ID_INFO"; break;

		default:
			StrBuf = "UNKNOWN";
			break;
	}
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("[OID], N6C_DOT11_DUMP_OID: %#x, %s\n", Oid, StrBuf));
}

