////////////////////////////////////////////////////////////////////////////////
//
//	File name:		N5C_Req.c
//	Description:	NDIS5.x common OID handler.
//
//	Author:			rcnjko
//
////////////////////////////////////////////////////////////////////////////////
#include "Mp_Precomp.h"
#include "802_11_OID.h"
#include "CustomOid.h"


#if WPP_SOFTWARE_TRACE
#include "N6C_Req.tmh"
#endif


RT_OID_ENTRY RT_SUPPORT_OIDs[]=
{
// NDIS 6.30 OID
	// Native Wi-Fi Direct OIDs
	{
		OID_DOT11_WFD_DEVICE_CAPABILITY,
		OID_STR_WRAPPER("OID_DOT11_WFD_DEVICE_CAPABILITY"),
		N63C_OID_DOT11_WFD_DEVICE_CAPABILITY
	},
	{
		OID_DOT11_WFD_GROUP_OWNER_CAPABILITY,
		OID_STR_WRAPPER("OID_DOT11_WFD_GROUP_OWNER_CAPABILITY"),
		N63C_OID_DOT11_WFD_GROUP_OWNER_CAPABILITY
	},
	{
		OID_DOT11_WFD_DEVICE_INFO,
		OID_STR_WRAPPER("OID_DOT11_WFD_DEVICE_INFO"),
		N63C_OID_DOT11_WFD_DEVICE_INFO
	},
	{
		OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST,
		OID_STR_WRAPPER("OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST"),
		N63C_OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST
	},
	{
		OID_DOT11_WFD_DISCOVER_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_WFD_DISCOVER_REQUEST"),
		N63C_OID_DOT11_WFD_DISCOVER_REQUEST
	},
	{
		OID_DOT11_WFD_ENUM_DEVICE_LIST,
		OID_STR_WRAPPER("OID_DOT11_WFD_ENUM_DEVICE_LIST"),
		N63C_OID_DOT11_WFD_ENUM_DEVICE_LIST
	},
	{
		OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY,
		OID_STR_WRAPPER("OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY"),
		N63C_OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY
	},
	{
		OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL,
		OID_STR_WRAPPER("OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL"),
		N63C_OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL
	},
	{
		OID_DOT11_WFD_ADDITIONAL_IE,
		OID_STR_WRAPPER("OID_DOT11_WFD_ADDITIONAL_IE"),
		N63C_OID_DOT11_WFD_ADDITIONAL_IE
	},
	{
		OID_DOT11_WFD_FLUSH_DEVICE_LIST,
		OID_STR_WRAPPER("OID_DOT11_WFD_FLUSH_DEVICE_LIST"),
		N63C_OID_DOT11_WFD_FLUSH_DEVICE_LIST
	},
	{
		OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST"),
		N63C_OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST
	},
	{
		OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE,
		OID_STR_WRAPPER("OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE"),
		N63C_OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE
	},
	{
		OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRMATION,
		OID_STR_WRAPPER("OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRMATION"),
		N63C_OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRMATION
	},
	{
		OID_DOT11_WFD_SEND_INVITATION_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_WFD_SEND_INVITATION_REQUEST"),
		N63C_OID_DOT11_WFD_SEND_INVITATION_REQUEST
	},
	{
		OID_DOT11_WFD_SEND_INVITATION_RESPONSE,
		OID_STR_WRAPPER("OID_DOT11_WFD_SEND_INVITATION_RESPONSE"),
		N63C_OID_DOT11_WFD_SEND_INVITATION_RESPONSE
	},
	{
		OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST"),
		N63C_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST
	},
	{
		OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE,
		OID_STR_WRAPPER("OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE"),
		N63C_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE
	},
	{
		OID_DOT11_WFD_STOP_DISCOVERY,
		OID_STR_WRAPPER("OID_DOT11_WFD_STOP_DISCOVERY"),
		N63C_OID_DOT11_WFD_STOP_DISCOVERY
	},
	{
		OID_DOT11_WFD_DESIRED_GROUP_ID,
		OID_STR_WRAPPER("OID_DOT11_WFD_DESIRED_GROUP_ID"),
		N63C_OID_DOT11_WFD_DESIRED_GROUP_ID
	},
	{
		OID_DOT11_WFD_START_GO_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_WFD_START_GO_REQUEST"),
		N63C_OID_DOT11_WFD_START_GO_REQUEST
	},
	{
		OID_DOT11_WFD_GROUP_START_PARAMETERS,
		OID_STR_WRAPPER("OID_DOT11_WFD_GROUP_START_PARAMETERS"),
		N63C_OID_DOT11_WFD_GROUP_START_PARAMETERS
	},
	{
		OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST"),
		N63C_OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST
	},
	{
		OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST"),
		N63C_OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST
	},
	{
		OID_DOT11_WFD_GROUP_JOIN_PARAMETERS,
		OID_STR_WRAPPER("OID_DOT11_WFD_GROUP_JOIN_PARAMETERS"),
		N63C_OID_DOT11_WFD_GROUP_JOIN_PARAMETERS
	},
	{
		OID_DOT11_WFD_GET_DIALOG_TOKEN,
		OID_STR_WRAPPER("OID_DOT11_WFD_GET_DIALOG_TOKEN"),
		N63C_OID_DOT11_WFD_GET_DIALOG_TOKEN
	},

	// Others
	{
		OID_DOT11_OFFLOAD_NETWORK_LIST,
		OID_STR_WRAPPER("OID_DOT11_OFFLOAD_NETWORK_LIST"),
		N63C_OID_DOT11_OFFLOAD_NETWORK_LIST
	},
	{
		OID_DOT11_POWER_MGMT_MODE_AUTO_ENABLED,
		OID_STR_WRAPPER("OID_DOT11_POWER_MGMT_MODE_AUTO_ENABLED"),
		N63C_OID_DOT11_POWER_MGMT_MODE_AUTO_ENABLED
	},
	{
		OID_DOT11_POWER_MGMT_MODE_STATUS,
		OID_STR_WRAPPER("OID_DOT11_POWER_MGMT_MODE_STATUS"),
		N63C_OID_DOT11_POWER_MGMT_MODE_STATUS
	},
	{
		OID_PACKET_COALESCING_FILTER_MATCH_COUNT,   // Query 
		OID_STR_WRAPPER("OID_PACKET_COALESCING_FILTER_MATCH_COUNT"),
		N63C_OID_PACKET_COALESCING_FILTER_MATCH_COUNT
	},

// NDIS 6.20 OID
	// Native 802.11 Virtual WiFi OIDs
	{
		OID_DOT11_CREATE_MAC,
		OID_STR_WRAPPER("OID_DOT11_CREATE_MAC"),
		N62C_OID_DOT11_CREATE_MAC
	},
	{
		OID_DOT11_DELETE_MAC,
		OID_STR_WRAPPER("OID_DOT11_DELETE_MAC"),
		N62C_OID_DOT11_DELETE_MAC
	},

	// Native 802.11 Extensible AP OIDs 
	{
		OID_DOT11_INCOMING_ASSOCIATION_DECISION,
		OID_STR_WRAPPER("OID_DOT11_INCOMING_ASSOCIATION_DECISION"),
		N62C_OID_DOT11_INCOMING_ASSOCIATION_DECISION
	},
	{
		OID_DOT11_WPS_ENABLED,
		OID_STR_WRAPPER("OID_DOT11_WPS_ENABLED"),
		N62C_OID_DOT11_WPS_ENABLED
	},
	{
		OID_DOT11_ADDITIONAL_IE,
		OID_STR_WRAPPER("OID_DOT11_ADDITIONAL_IE"),
		N62C_OID_DOT11_ADDITIONAL_IE
	},
	{
		OID_DOT11_ENUM_PEER_INFO,
		OID_STR_WRAPPER("OID_DOT11_ENUM_PEER_INFO"),
		N62C_OID_DOT11_ENUM_PEER_INFO
	},
	{
		OID_DOT11_AVAILABLE_CHANNEL_LIST,
		OID_STR_WRAPPER("OID_DOT11_AVAILABLE_CHANNEL_LIST"),
		N62C_OID_DOT11_AVAILABLE_CHANNEL_LIST
	},
	{
		OID_DOT11_AVAILABLE_FREQUENCY_LIST,
		OID_STR_WRAPPER("OID_DOT11_AVAILABLE_FREQUENCY_LIST"),
		N62C_OID_DOT11_AVAILABLE_FREQUENCY_LIST
	},
	{
		OID_DOT11_START_AP_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_START_AP_REQUEST"),
		N62C_OID_DOT11_START_AP_REQUEST
	},
	{
		OID_DOT11_DISASSOCIATE_PEER_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_DISASSOCIATE_PEER_REQUEST"),
		N62C_OID_DOT11_DISASSOCIATE_PEER_REQUEST
	},

	// Station Specific
	{
		OID_DOT11_ASSOCIATION_PARAMS,
		OID_STR_WRAPPER("OID_DOT11_ASSOCIATION_PARAMS"),
		N62C_OID_DOT11_ASSOCIATION_PARAMS
	},
		
	// Power Management OID
	{
		OID_PM_GET_PROTOCOL_OFFLOAD,
		OID_STR_WRAPPER("OID_PM_GET_PROTOCOL_OFFLOAD"),
		N62C_OID_PM_GET_PROTOCOL_OFFLOAD
	},
	{
		OID_PM_ADD_WOL_PATTERN,
		OID_STR_WRAPPER("OID_PM_ADD_WOL_PATTERN"),
		N62C_OID_PM_ADD_WOL_PATTERN
	},
	{
		OID_PM_REMOVE_WOL_PATTERN,
		OID_STR_WRAPPER("OID_PM_REMOVE_WOL_PATTERN"),
		N62C_OID_PM_REMOVE_WOL_PATTERN
	},
	{
		OID_PM_PARAMETERS,
		OID_STR_WRAPPER("OID_PM_PARAMETERS"),
		N62C_OID_PM_PARAMETERS
	},
	{
		OID_PM_ADD_PROTOCOL_OFFLOAD,
		OID_STR_WRAPPER("OID_PM_ADD_PROTOCOL_OFFLOAD"),
		N62C_OID_PM_ADD_PROTOCOL_OFFLOAD
	},
	{
		OID_PM_REMOVE_PROTOCOL_OFFLOAD,
		OID_STR_WRAPPER("OID_PM_REMOVE_PROTOCOL_OFFLOAD"),
		N62C_OID_PM_REMOVE_PROTOCOL_OFFLOAD
	},

	// Packet filter OID
	{
		OID_RECEIVE_FILTER_SET_FILTER,
		OID_STR_WRAPPER("OID_RECEIVE_FILTER_SET_FILTER"),
		N62C_OID_RECEIVE_FILTER_SET_FILTER
	},
	{
		OID_RECEIVE_FILTER_CLEAR_FILTER,   // Set 
		OID_STR_WRAPPER("OID_RECEIVE_FILTER_CLEAR_FILTER"),
		N62C_OID_RECEIVE_FILTER_CLEAR_FILTER
	},
	{
		OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES,   // Query 
		OID_STR_WRAPPER("OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES"),
		N62C_OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES
	},
	{
		OID_RECEIVE_FILTER_CURRENT_CAPABILITIES,   // Query 
		OID_STR_WRAPPER("OID_RECEIVE_FILTER_CURRENT_CAPABILITIES"),
		N62C_OID_RECEIVE_FILTER_SET_FILTER
	}, 
	/*
	{
		OID_PACKET_COALESCING_FILTER_MATCH_COUNT,   // Query 
		OID_STR_WRAPPER("OID_PACKET_COALESCING_FILTER_MATCH_COUNT"),
		N62C_OID_RECEIVE_FILTER_SET_FILTER
	},
	*/
	//====================
	/*  Need to check support or Not 
	{
		OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES,   // Query 
		OID_STR_WRAPPER("OID_PACKET_COALESCING_FILTER_MATCH_COUNT"),
		N62C_OID_RECEIVE_FILTER_SET_FILTER
	},
	{
		OID_RECEIVE_FILTER_ENUM_FILTERS,   // method 
		OID_STR_WRAPPER("OID_PACKET_COALESCING_FILTER_MATCH_COUNT"),
		N62C_OID_RECEIVE_FILTER_SET_FILTER
	},
	{
		OID_RECEIVE_FILTER_PARAMETERS,   // method 
		OID_STR_WRAPPER("OID_PACKET_COALESCING_FILTER_MATCH_COUNT"),
		N62C_OID_RECEIVE_FILTER_SET_FILTER
	},
	{
		OID_RECEIVE_FILTER_CURRENT_CAPABILITIES,   // Query 
		OID_STR_WRAPPER("OID_PACKET_COALESCING_FILTER_MATCH_COUNT"),
		N62C_OID_RECEIVE_FILTER_SET_FILTER
	}, 
	*/

	// Others
	{
		OID_GEN_INTERRUPT_MODERATION,
		OID_STR_WRAPPER("OID_GEN_INTERRUPT_MODERATION"),
		N62C_OID_GEN_INTERRUPT_MODERATION
	},


// NDIS 6.0-6.1 OID
	{
		OID_DOT11_CURRENT_OPERATION_MODE, 
		OID_STR_WRAPPER("OID_DOT11_CURRENT_OPERATION_MODE"), 
		N6C_OID_DOT11_CURRENT_OPERATION_MODE	
	},
	{
		OID_DOT11_FLUSH_BSS_LIST,
		OID_STR_WRAPPER("OID_DOT11_FLUSH_BSS_LIST"),
		N6C_OID_DOT11_FLUSH_BSS_LIST
	},
	{
		OID_DOT11_SCAN_REQUEST,
		OID_STR_WRAPPER("OID_DOT11_SCAN_REQUEST"),
		N6C_OID_DOT11_SCAN_REQUEST	// Only set default scan
	},
	{
		OID_DOT11_ENUM_BSS_LIST,		// only get from default port. ?? or need to check
		OID_STR_WRAPPER("OID_DOT11_ENUM_BSS_LIST"),
		N6C_OID_DOT11_ENUM_BSS_LIST
	},
	{
		OID_DOT11_NIC_POWER_STATE,
		OID_STR_WRAPPER("OID_DOT11_NIC_POWER_STATE"),
		N6C_OID_DOT11_NIC_POWER_STATE
	},
	{
		OID_DOT11_RESET_REQUEST, 
		OID_STR_WRAPPER("OID_DOT11_RESET_REQUEST"),
		N6C_OID_DOT11_RESET_REQUEST
	},
	{
		OID_DOT11_DESIRED_PHY_LIST, 
		OID_STR_WRAPPER("OID_DOT11_DESIRED_PHY_LIST"),
		N6C_OID_DOT11_DESIRED_PHY_LIST
	},
	{
		OID_DOT11_AUTO_CONFIG_ENABLED, 
		OID_STR_WRAPPER("OID_DOT11_AUTO_CONFIG_ENABLED"),
		N6C_OID_DOT11_AUTO_CONFIG_ENABLED
	},
	{
		OID_DOT11_BEACON_PERIOD, 
		OID_STR_WRAPPER("OID_DOT11_BEACON_PERIOD"),
		N6C_OID_DOT11_BEACON_PERIOD
	},
	{
		OID_DOT11_DTIM_PERIOD, 
		OID_STR_WRAPPER("OID_DOT11_DTIM_PERIOD"),
		N6C_OID_DOT11_DTIM_PERIOD
	},
	{
		OID_DOT11_DESIRED_SSID_LIST, 
		OID_STR_WRAPPER("OID_DOT11_DESIRED_SSID_LIST"),
		N6C_OID_DOT11_DESIRED_SSID_LIST
	},
	{
		OID_GEN_CURRENT_PACKET_FILTER, 
		OID_STR_WRAPPER("OID_GEN_CURRENT_PACKET_FILTER"),
		N6C_OID_GEN_CURRENT_PACKET_FILTER
	},
	{
		OID_DOT11_CURRENT_CHANNEL, 
		OID_STR_WRAPPER("OID_DOT11_CURRENT_CHANNEL"),
		N6C_OID_DOT11_CURRENT_CHANNEL
	},
	{
		OID_DOT11_DISCONNECT_REQUEST, 
		OID_STR_WRAPPER("OID_DOT11_DISCONNECT_REQUEST"),
		N6C_OID_DOT11_DISCONNECT_REQUEST
	},
	{
		OID_DOT11_CONNECT_REQUEST, 
		OID_STR_WRAPPER("OID_DOT11_CONNECT_REQUEST"),
		N6C_OID_DOT11_CONNECT_REQUEST
	},
	{
		OID_GEN_LINK_PARAMETERS, 
		OID_STR_WRAPPER("OID_GEN_LINK_PARAMETERS"),
		N6C_OID_GEN_LINK_PARAMETERS
	},
	{
		OID_DOT11_SAFE_MODE_ENABLED, 
		OID_STR_WRAPPER("OID_DOT11_SAFE_MODE_ENABLED"),
		N6C_OID_DOT11_SAFE_MODE_ENABLED
	},
	{
		OID_DOT11_CURRENT_PHY_ID, 
		OID_STR_WRAPPER("OID_DOT11_CURRENT_PHY_ID"),
		N6C_OID_DOT11_CURRENT_PHY_ID
	},
	{
		OID_GEN_SUPPORTED_LIST, 
		OID_STR_WRAPPER("OID_GEN_SUPPORTED_LIST"),
		N6C_OID_GEN_SUPPORTED_LIST
	},
	{
		OID_GEN_VENDOR_DRIVER_VERSION, 
		OID_STR_WRAPPER("OID_GEN_VENDOR_DRIVER_VERSION"),
		N6C_OID_GEN_VENDOR_DRIVER_VERSION
	},
	{
		OID_PNP_CAPABILITIES, 
		OID_STR_WRAPPER("OID_PNP_CAPABILITIES"),
		N6C_OID_PNP_CAPABILITIES
	},
	{
		OID_PNP_QUERY_POWER, 
		OID_STR_WRAPPER("OID_PNP_QUERY_POWER"),
		N6C_OID_PNP_QUERY_POWER
	},
	{
		OID_PNP_ENABLE_WAKE_UP, 
		OID_STR_WRAPPER("OID_PNP_ENABLE_WAKE_UP"),
		N6C_OID_PNP_ENABLE_WAKE_UP
	},
	{
		OID_DOT11_CURRENT_TX_ANTENNA, 
		OID_STR_WRAPPER("OID_DOT11_CURRENT_TX_ANTENNA"),
		N6C_OID_DOT11_CURRENT_TX_ANTENNA
	},
	{
		OID_DOT11_CURRENT_RX_ANTENNA, 
		OID_STR_WRAPPER("OID_DOT11_CURRENT_RX_ANTENNA"),
		N6C_OID_DOT11_CURRENT_RX_ANTENNA
	},
	{
		OID_PNP_ADD_WAKE_UP_PATTERN, 
		OID_STR_WRAPPER("OID_PNP_ADD_WAKE_UP_PATTERN"),
		N6C_OID_PNP_ADD_WAKE_UP_PATTERN
	},
	{
		OID_PNP_REMOVE_WAKE_UP_PATTERN, 
		OID_STR_WRAPPER("OID_PNP_REMOVE_WAKE_UP_PATTERN"),
		N6C_OID_PNP_REMOVE_WAKE_UP_PATTERN
	},
	{
		OID_PNP_SET_POWER, 
		OID_STR_WRAPPER("OID_PNP_SET_POWER"),
		N6C_OID_PNP_SET_POWER
	}
};


//
//	Description:
//		Translate PRT_WLAN_BSS into PNDIS_WLAN_BSSID_EX.
//
VOID
TranslateRtBssToNdisBss(
	PADAPTER			Adapter,
	pu1Byte				pNBss,
	PRT_WLAN_BSS		pRtBss
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PNDIS_WLAN_BSSID_EX	pNdisBss = (PNDIS_WLAN_BSSID_EX)pNBss;
	
	PlatformZeroMemory(pNdisBss, sizeof(NDIS_WLAN_BSSID_EX));

	CopyMem(pNdisBss->MacAddress, pRtBss->bdBssIdBuf, 6);
	if(BeHiddenSsid(pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen))
	{
		pNdisBss->Ssid.Ssid[0] = 0x0;
		pNdisBss->Ssid.SsidLength = 1;
	}
	else
	{
		CopySsid(pNdisBss->Ssid.Ssid, pNdisBss->Ssid.SsidLength, pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen);
	}
	pNdisBss->Privacy = (pRtBss->bdCap & cPrivacy) ? TRUE : FALSE;

	pNdisBss->Rssi = ( (pRtBss->RSSI+1)>>1 ) - 95;
	
	//HCT12.0 NDTest
	switch(pRtBss->wirelessmode)
	{
	case WIRELESS_MODE_B:
		RT_TRACE(COMP_DBG, DBG_LOUD, ("TranslateRtBssToNdisBss setB Ndis802_11DS \n"));
		pNdisBss->NetworkTypeInUse = Ndis802_11DS;
		break;
	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
	case WIRELESS_MODE_AC_24G:
		RT_TRACE(COMP_DBG, DBG_LOUD, ("TranslateRtBssToNdisBss setG Ndis802_11OFDM24 \n"));
		pNdisBss->NetworkTypeInUse = Ndis802_11OFDM24;
		break;
	case WIRELESS_MODE_A:
	case WIRELESS_MODE_N_5G:
	case WIRELESS_MODE_AC_5G:
		RT_TRACE(COMP_DBG, DBG_LOUD, ("TranslateRtBssToNdisBss setG Ndis802_11OFDM5 \n"));
		pNdisBss->NetworkTypeInUse = Ndis802_11OFDM5;
		break;
	default:
		RT_TRACE(COMP_DBG, DBG_LOUD, ("TranslateRtBssToNdisBss default Ndis802_11DS \n"));
		pNdisBss->NetworkTypeInUse = Ndis802_11DS;
		break;			
	}
	
	pNdisBss->Configuration.Length = sizeof(NDIS_802_11_CONFIGURATION);
	pNdisBss->Configuration.BeaconPeriod = pRtBss->bdBcnPer;
	pNdisBss->Configuration.ATIMWindow = pRtBss->bdIbssParms.atimWin;

	//HCT12.0 NDTest
	pNdisBss->Configuration.DSConfig = 1000 * (MgntGetChannelFrequency(pRtBss->ChannelNumber));

	if(pRtBss->bdCap & cESS)
		pNdisBss->InfrastructureMode = Ndis802_11Infrastructure;
	else if(pRtBss->bdCap & cIBSS)
		pNdisBss->InfrastructureMode = Ndis802_11IBSS;
	else
		pNdisBss->InfrastructureMode = Ndis802_11AutoUnknown;

	CopyMem( &pNdisBss->SupportedRates[0], &pRtBss->bdSupportRateEXBuf[0], pRtBss->bdSupportRateEXLen);

	pNdisBss->IELength = pRtBss->IE.Length;
	CopyMem(pNdisBss->IEs, pRtBss->IE.Octet, pRtBss->IE.Length);

	if(pNdisBss->IELength > 0)
	{
		pNdisBss->Length = sizeof(NDIS_WLAN_BSSID_EX)-1+pNdisBss->IELength;
	}
	else
	{
		pNdisBss->Length = sizeof(NDIS_WLAN_BSSID_EX);
	}
	
	// <RJ_TODO> This is a 4-byte alignment, shall we align it to 8 byte for X64???
	pNdisBss->Length = (pNdisBss->Length+3)&0xfffffffc; 
}


//
// Added for WPA2-PSK, by Annie, 2005-09-20.
//
BOOLEAN
Query_802_11_CAPABILITY(
	PADAPTER		Adapter,
	pu1Byte			pucBuf,
	pu4Byte			pulOutLen
)
{
	static NDIS_802_11_AUTHENTICATION_ENCRYPTION szAuthEnc[] = 
	{
		{Ndis802_11AuthModeOpen, Ndis802_11EncryptionDisabled}, 
		{Ndis802_11AuthModeOpen, Ndis802_11Encryption1Enabled},
		{Ndis802_11AuthModeShared, Ndis802_11EncryptionDisabled}, 
		{Ndis802_11AuthModeShared, Ndis802_11Encryption1Enabled},
		{Ndis802_11AuthModeWPA, Ndis802_11Encryption2Enabled}, 
		{Ndis802_11AuthModeWPA, Ndis802_11Encryption3Enabled},
		{Ndis802_11AuthModeWPAPSK, Ndis802_11Encryption2Enabled}, 
		{Ndis802_11AuthModeWPAPSK, Ndis802_11Encryption3Enabled},
		{Ndis802_11AuthModeWPANone, Ndis802_11Encryption2Enabled}, 
		{Ndis802_11AuthModeWPANone, Ndis802_11Encryption3Enabled},
		{Ndis802_11AuthModeWPA2, Ndis802_11Encryption2Enabled}, 
		{Ndis802_11AuthModeWPA2, Ndis802_11Encryption3Enabled},
		{Ndis802_11AuthModeWPA2PSK, Ndis802_11Encryption2Enabled}, 
		{Ndis802_11AuthModeWPA2PSK, Ndis802_11Encryption3Enabled}
	};	
	static ULONG	ulNumOfPairSupported = sizeof(szAuthEnc)/sizeof(NDIS_802_11_AUTHENTICATION_ENCRYPTION);
	NDIS_802_11_CAPABILITY * pCap = (NDIS_802_11_CAPABILITY *)pucBuf;
	pu1Byte	pucAuthEncryptionSupported = (pu1Byte)pCap->AuthenticationEncryptionSupported;


	pCap->Length = sizeof(NDIS_802_11_CAPABILITY);
	if(ulNumOfPairSupported > 1 )
		pCap->Length += 	(ulNumOfPairSupported-1) * sizeof(NDIS_802_11_AUTHENTICATION_ENCRYPTION);
	
	pCap->Version = 2;	
	pCap->NoOfPMKIDs = NUM_PMKID_CACHE;	// It MUST be set between 3~16, or WZC won't show WPA2 related option in profile.
	pCap->NoOfAuthEncryptPairsSupported = ulNumOfPairSupported;

	if( sizeof (szAuthEnc) <= 240 )		// 240 = 256 - 4*4	// SecurityInfo.szCapability: only 256 bytes in size.
	{
		CopyMem( pucAuthEncryptionSupported, (pu1Byte)szAuthEnc,  sizeof (szAuthEnc) );
		*pulOutLen = pCap->Length;
		return TRUE;
	}
	else
	{
		*pulOutLen = 0;
		RT_ASSERT( FALSE, ("Query_802_11_CAPABILITY(): szAuthEnc size is too large.\n"));
		return FALSE;
	}
}



VOID
ResetHistogramCounter(
	IN	PADAPTER		Adapter)
{
	Adapter->RxStats.NumRxRetrySmallPacket = 0;
	Adapter->RxStats.NumRxRetryMiddlePacket = 0;
	Adapter->RxStats.NumRxRetryLargePacket = 0;
	Adapter->RxStats.NumRxOKSmallPacket = 0;
	Adapter->RxStats.NumRxOKMiddlePacket = 0;
	Adapter->RxStats.NumRxOKLargePacket = 0;
}

// This is to offload the N6CQueryInformation() since the huge size of the funcion stack causes the BSOD when DbgPrint.
static NDIS_STATUS
N6CQueryInformationHandleCustomizedWifiDirectOids(
	IN	PADAPTER		pAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
	)
{
	PMGNT_INFO      		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	UCHAR					ucInfo = 0;
	USHORT					usInfo = 0;
	ULONG					ulInfo = 0;
	LONG				lInfo = 0;
	ULONGLONG				ul64Info = 0;

	PVOID					pInfo = (PVOID) &ulInfo;
	ULONG					ulInfoLen = sizeof(ulInfo);
	BOOLEAN					bInformationCopied = FALSE;

	FunctionIn(COMP_OID_QUERY);

	*BytesWritten = 0;
	*BytesNeeded = 0;

	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;

#if (P2P_SUPPORT == 1)
	case OID_RT_P2P_VERSION:
		{
			if(pMgntInfo->bDisableRtkSupportedP2P)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, 
					("Ignore query of 0x%08X because bDisableRtkSupportedP2P is turned on\n", Oid));
				Status = NDIS_STATUS_NOT_SUPPORTED;
				break;
			}
			
			if(InformationBufferLength < sizeof(u4Byte))
			{
				*BytesNeeded = sizeof(u4Byte);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			*((pu4Byte)InformationBuffer) = (GET_P2P_INFO(pAdapter))->P2PVersion; //(u4Byte)P2P_VERSION;
			*BytesWritten = sizeof(u4Byte);
			ulInfoLen = *BytesWritten;
			bInformationCopied = TRUE;
		}
		break;
	
	case OID_RT_P2P_MAX_VERSION:
		{
			if(pMgntInfo->bDisableRtkSupportedP2P)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, 
					("Ignore query of 0x%08X because bDisableRtkSupportedP2P is turned on\n", Oid));
				Status = NDIS_STATUS_NOT_SUPPORTED;
				break;
			}
			
			if(InformationBufferLength < sizeof(u4Byte))
			{
				*BytesNeeded = sizeof(u4Byte);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			*((pu4Byte)InformationBuffer) = (u4Byte)P2P_VERSION;
			*BytesWritten = sizeof(u4Byte);
			ulInfoLen = *BytesWritten;
			bInformationCopied = TRUE;
		}
		break;

	case OID_RT_P2P_MODE:
		{
			if(pMgntInfo->bDisableRtkSupportedP2P)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, 
					("Ignore query of 0x%08X because bDisableRtkSupportedP2P is turned on\n", Oid));
				Status = NDIS_STATUS_NOT_SUPPORTED;
				break;
			}
				
			ulInfo = MgntActQuery_P2PMode(pAdapter);
			//RT_TRACE(COMP_P2P, DBG_LOUD, ("OID_RT_P2P_MODE: %u\n", ulInfo));
		}
		break;
	case OID_RT_P2P_ENUM_SCAN_LIST:
		{
			u4Byte ScanListSize = InformationBufferLength;
			if(!MgntActQuery_P2PScanList(pAdapter, InformationBuffer, &ScanListSize))
			{
				RT_TRACE(COMP_P2P, DBG_WARNING, ("Error: OID_RT_P2P_ENUM_SCAN_LIST buffer too short!!!\n"));
				*BytesNeeded = ScanListSize;
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
			}

			*BytesWritten = ScanListSize;
			ulInfoLen = ScanListSize;
			bInformationCopied = TRUE;
		}
		break;

	case OID_RT_P2P_GO_INTENT:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			
			if( InformationBufferLength < sizeof(ucInfo) )
			{
				*BytesNeeded = sizeof(ucInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			ucInfo = pP2PInfo->GOIntent >> 1;
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("Get OID_RT_P2P_GO_INTENT: %u\n", 
				ucInfo));
		}
		break;
	case OID_RT_P2P_CAPABILITY:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			pu1Byte pDevCap = (pu1Byte)InformationBuffer + 0;
			pu1Byte pGrpCap = (pu1Byte)InformationBuffer + 1;

			if(InformationBufferLength < sizeof(u1Byte) * 2)
			{
				*BytesNeeded = sizeof(u1Byte) * 2;
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
			}

			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("OID_RT_P2P_CAPABILITY: current DC=%u, GC=%u\n", 
				pP2PInfo->DeviceCapability, pP2PInfo->GroupCapability));

			*pDevCap = pP2PInfo->DeviceCapability;
			*pGrpCap = pP2PInfo->GroupCapability;

			P2PDumpDeviceCapability(*pDevCap);
			P2PDumpGroupCapability(*pGrpCap);

			*BytesWritten = sizeof(u1Byte) * 2;
			ulInfoLen = sizeof(u1Byte) * 2;
			bInformationCopied = TRUE;
		}
		break;
	case OID_RT_P2P_GO_SSID:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			pu1Byte pSsidLen = (pu1Byte)InformationBuffer + 0;
			pu1Byte SsidBuf = (pu1Byte)InformationBuffer + 1;

			if(InformationBufferLength < sizeof(u1Byte) + pP2PInfo->SSIDLen)
			{
				*BytesNeeded = sizeof(u1Byte) + pP2PInfo->SSIDLen;
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
			}

			if(pP2PInfo->Role != P2P_GO)
			{
				*BytesWritten = 0;
			}
			else
			{
			//
			// When query,  return the entire (Including DIRECT-xx) SSID of the GO.
			// However, when set, only postfix can be set.
			//
				u2Byte SsidLen = 0;
				MgntActQuery_802_11_SSID(GetFirstExtAdapter(pAdapter), SsidBuf, &SsidLen);
				*pSsidLen = (u1Byte)SsidLen;
			}
			ulInfoLen = *BytesWritten;
			bInformationCopied = TRUE;
		}
		break;
	case OID_RT_P2P_GO_SSID_POSTFIX:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			pu1Byte pSsidLen = (pu1Byte)InformationBuffer + 0;
			pu1Byte SsidBuf = (pu1Byte)InformationBuffer + 1;

			if(InformationBufferLength < sizeof(u1Byte) + pP2PInfo->SSIDLen)
			{
				*BytesNeeded = sizeof(u1Byte) + pP2PInfo->SSIDLen;
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
			}
			
			CopySsid(SsidBuf, *pSsidLen, pP2PInfo->SSIDPostfixBuf, pP2PInfo->SSIDPostfixLen);

			RT_PRINT_STR(COMP_P2P, DBG_LOUD, "OID_RT_P2P_GO_SSID\n", SsidBuf, *pSsidLen);

			*BytesWritten = *pSsidLen + 1;
			
			ulInfoLen = *BytesWritten;
			bInformationCopied = TRUE;
		}
		break;
	case OID_RT_P2P_SELF_DEV_DESC:
		{
			if(GET_P2P_INFO(pAdapter)->P2PVersion == 0 && InformationBufferLength < sizeof(P2P_DEVICE_DESCRIPTOR_V0))
			{
				*BytesNeeded = sizeof(P2P_DEVICE_DESCRIPTOR_V0);
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
			}
			else if(GET_P2P_INFO(pAdapter)->P2PVersion == 1 && InformationBufferLength < sizeof(P2P_DEVICE_DESCRIPTOR_V1))
			{
				*BytesNeeded = sizeof(P2P_DEVICE_DESCRIPTOR_V1);
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
			}
			else if((GET_P2P_INFO(pAdapter)->P2PVersion == 2 || GET_P2P_INFO(pAdapter)->P2PVersion == 3) 
				&& InformationBufferLength < sizeof(P2P_DEVICE_DESCRIPTOR_V2))
			{
				*BytesNeeded = sizeof(P2P_DEVICE_DESCRIPTOR_V2);
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
			}
			
			*BytesWritten = MgntActQuery_P2PSelfDeviceDescriptor(pAdapter, InformationBuffer);
			bInformationCopied = TRUE;

		}
		break;
	case OID_RT_P2P_INTERFACE_ADDRESS:
		{
			if(InformationBufferLength < 6)
			{
				*BytesNeeded = 6;
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
			}
			PlatformMoveMemory(InformationBuffer, (GET_P2P_INFO(pAdapter))->InterfaceAddress, 6);

			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Get OID_RT_P2P_INTERFACE_ADDRESS: ", InformationBuffer);
			
			*BytesWritten = 6;
			bInformationCopied = TRUE;
		}
		break;
	case OID_RT_P2P_DEVICE_ADDRESS:
		{
			if(InformationBufferLength < 6)
			{
				*BytesNeeded = 6;
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				break;
			}
			PlatformMoveMemory(InformationBuffer, (GET_P2P_INFO(pAdapter))->DeviceAddress, 6);

			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Get OID_RT_P2P_DEVICE_ADDRESS: ", InformationBuffer);
			
			*BytesWritten = 6;
			bInformationCopied = TRUE;
		}
		break;
	case OID_RT_P2P_OP_CHANNEL:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			
			if( InformationBufferLength < sizeof(ucInfo) )
			{
				*BytesNeeded = sizeof(ucInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			ucInfo = pP2PInfo->OperatingChannel;
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("Get OID_RT_P2P_OP_CHANNEL: %u\n", 
				ucInfo));
		}
		break;
	case OID_RT_P2P_LISTEN_CHANNEL:
		{
			if( InformationBufferLength < sizeof(ucInfo) )
			{
				*BytesNeeded = sizeof(ucInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			MgntActQuery_P2PListenChannel(pAdapter, &ucInfo);
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("Get OID_RT_P2P_LISTEN_CHANNEL: %u\n", 
				ucInfo));
		}
		break;
	case OID_RT_P2P_EXTENDED_LISTEN_TIMING:
		{
			if( InformationBufferLength < sizeof(u2Byte) * 2)
			{
				*BytesNeeded = sizeof(u2Byte) * 2;
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			((pu2Byte)InformationBuffer)[0] = (u2Byte)(GET_P2P_INFO(pAdapter))->ExtListenTimingPeriod;
			((pu2Byte)InformationBuffer)[1] = (u2Byte)(GET_P2P_INFO(pAdapter))->ExtListenTimingDuration;
			
			*BytesWritten = sizeof(u2Byte) * 2;
			bInformationCopied = TRUE;
		}
		break;
	case OID_RT_P2P_GO_NEGO_RESULT:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			
			if( InformationBufferLength < sizeof(ucInfo) )
			{
				*BytesNeeded = sizeof(ucInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			ucInfo = pP2PInfo->PreviousGONegoResult;
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("Get OID_RT_P2P_GO_NEGO_RESULT: %u\n", 
				ucInfo));
		}
		break;
	case OID_RT_P2P_CHANNEL_LIST:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_P2P_CHANNEL_LIST\n") );
		{
			if(MgntActQuery_P2PChannelList(pAdapter, 
				(u4Byte)InformationBufferLength, 
				(pu1Byte)InformationBuffer, 
				(pu1Byte)InformationBuffer + 1) == RT_STATUS_SUCCESS)
			{
				*BytesWritten = *((pu1Byte)InformationBuffer) + 1;
				bInformationCopied = TRUE;
			}
			else
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			}
		}
		break;

	case OID_RT_P2P_SERVICE_FRAG_THRESHOLD:
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("Query OID_RT_P2P_SERVICE_FRAG_THRESHOLD\n"));

			if(InformationBufferLength < sizeof(u2Byte))
			{
				*BytesNeeded = sizeof(u2Byte);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 
			
			if(RT_STATUS_SUCCESS == P2PGetServiceFragThreshold(pAdapter, (pu2Byte)InformationBuffer))
			{
				Status = NDIS_STATUS_FAILURE;
			}
		}
		break;

#endif	// #if (P2P_SUPPORT == 1)
	case OID_RT_P2P_FULL_SSID:
		{
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_P2P_FULL_SSID\n"));
			if(NDIS_STATUS_SUCCESS == NdisStatusFromRtStatus(P2P_GetP2PGoFullSSID(pAdapter, InformationBufferLength, InformationBuffer, BytesNeeded)))
			{
				*BytesWritten = *BytesNeeded;
				bInformationCopied = TRUE;
			}
		}
		break;
		
	}

	if(Status == NDIS_STATUS_SUCCESS)
	{        
		if(ulInfoLen <= InformationBufferLength)
		{
			// Copy result into InformationBuffer
			if(ulInfoLen && !bInformationCopied)
			{
				*BytesWritten = ulInfoLen;
				NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
			}
		}
		else if(!bInformationCopied)
		{
			// Buffer too short
			*BytesNeeded = ulInfoLen;
			Status = NDIS_STATUS_BUFFER_TOO_SHORT;
		}
	}

	return Status;
}




// This is to offload the N6CQueryInformation() since the huge size of the funcion stack causes the BSOD when DbgPrint.
static NDIS_STATUS
N6CQueryInformationHandleCustomized11nOids(
	IN	PADAPTER		pAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
	)
{
	PMGNT_INFO      		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	UCHAR					ucInfo = 0;
	USHORT					usInfo = 0;
	ULONG					ulInfo = 0;
	LONG				lInfo = 0;
	ULONGLONG				ul64Info = 0;

	PVOID					pInfo = (PVOID) &ulInfo;
	ULONG					ulInfoLen = sizeof(ulInfo);
	BOOLEAN					bInformationCopied = FALSE;

	FunctionIn(COMP_OID_QUERY);

	*BytesWritten = 0;
	*BytesNeeded = 0;

	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;

	case OID_RT_GET_11N_MIMPO_RSSI:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_11N_MIMPO_RSSI\n"));
		//DbgPrint("Query OID_RT_GET_11N_MIMPO_RSSI\n");
		{	
			MIMO_RSSI	reportRSSI;
			
			if(InformationBufferLength < sizeof(MIMO_RSSI))
			{
				*BytesNeeded = sizeof(MIMO_RSSI);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			
			reportRSSI.EnableAntenna = 0x0f;
			reportRSSI.AntennaA = pAdapter->RxStats.RxRSSIPercentage[ODM_RF_PATH_A];
			reportRSSI.AntennaB = pAdapter->RxStats.RxRSSIPercentage[ODM_RF_PATH_B];
			reportRSSI.AntennaC = pAdapter->RxStats.RxRSSIPercentage[ODM_RF_PATH_C];
			reportRSSI.AntennaD = pAdapter->RxStats.RxRSSIPercentage[ODM_RF_PATH_D];
			reportRSSI.Average = GET_UNDECORATED_AVERAGE_RSSI(pAdapter);
						
			PlatformMoveMemory( InformationBuffer, 
								&(reportRSSI), sizeof(MIMO_RSSI));			
			*BytesWritten = sizeof(MIMO_RSSI);
		}
		return Status;

	case OID_RT_MOTOR_BT_802_11_PAL:
		{
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_MOTOR_BT_802_11_PAL\n"));
			RT_DISP(FIOCTL, IOCTL_STATE, ("[BT]Query OID_RT_MOTOR_BT_802_11_PAL\n"));
			return Status;
		}
		
		//break;
	case OID_RT_GET_11N_MIMPO_EVM:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_11N_MIMPO_EVM\n"));
		{	

			MIMO_EVM	reportEVM;

			if(InformationBufferLength < sizeof(MIMO_EVM))
			{
				*BytesNeeded = sizeof(MIMO_EVM);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			reportEVM.EVM1 = pAdapter->RxStats.RxEVMPercentage[0];
			reportEVM.EVM2 = pAdapter->RxStats.RxEVMPercentage[1];

			PlatformMoveMemory( InformationBuffer, 
								&(reportEVM), sizeof(MIMO_EVM));			
			*BytesWritten = sizeof(MIMO_EVM);

		}
		return Status;

	case OID_RT_11N_INITIAL_TX_RATE: //added by vivi, 20080604
		{
			if(pAdapter->TxStats.CurrentInitTxRate > 0x80)
				ulInfo = pAdapter->TxStats.CurrentInitTxRate*10000/2;
			else
				ulInfo = HTMcsToDataRate(pAdapter, pAdapter->TxStats.CurrentInitTxRate)*10000/2;
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, MgntActQuery_RT_11N_INITIAL_TX_RATES:%d Mbps\n",ulInfo/10000));
		}
		break;
		
	case OID_RT_11N_TX_RETRY_COUNT://added by vivi, 20080604
		ulInfo =(u4Byte) pAdapter->TxStats.NumTxRetryCount;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_RT_11N_TX_RETRY_COUNT:%d Mbps\n",ulInfo));
		break;

	case OID_RT_11N_UI_SHOW_TX_RATE:        //Add by Jacken 2008/04/14
		ulInfo = (MgntActQuery_RT_11N_USER_SHOW_RATES(pAdapter,FALSE, FALSE)/2)*10000;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, MgntActQuery_RT_11N_USER_SHOW_RATES-TX:%d Mbps\n",ulInfo/10000));
		break;	

	case OID_RT_11N_UI_SHOW_RX_RATE:        //Add by Jacken 2008/05/12
		ulInfo = (MgntActQuery_RT_11N_USER_SHOW_RATES(pAdapter,TRUE, FALSE)/2)*10000;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, MgntActQuery_RT_11N_USER_SHOW_RATES-RX:%d Mbps\n",ulInfo/10000));
		break;	

	case OID_RT_CURRENT_BANDWIDTH:	
		ulInfo = pMgntInfo->dot11CurrentChannelBandWidth;
		break;

	case OID_RT_CURRENT_CHANNEL_INFO:
		{
			u4Byte	ChnlLen  = InformationBufferLength;
			if(CHNL_GetCurrnetChnlInfo(pAdapter, InformationBuffer, &ChnlLen) == FALSE)
			{
				*BytesNeeded = ChnlLen;
				 Status =  NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			else
			{
				ulInfoLen = *BytesWritten = ChnlLen;
				bInformationCopied = TRUE;
			}	
		}
		break;

	case OID_RT_11N_FIRMWARE_VERSION:
		ulInfo = (ULONG)HalGetFirmwareVerison(pAdapter);
		ulInfoLen = sizeof(u2Byte);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, Firmware Version = %d\n",ucInfo));
		break;
	case OID_RT_ADCSMP_TRIG:
		{
			Status = ADCSmp_Query(pAdapter, InformationBufferLength, InformationBuffer, BytesWritten);
			if(Status != RT_STATUS_SUCCESS)
				return Status;

			bInformationCopied = TRUE;
		}
		break;
	
	case OID_RT_11N_FORCED_LDPC:
		{
			PRT_NDIS_COMMON			pNdisCommon = pAdapter->pNdisCommon;
			ulInfo = pNdisCommon->RegLdpc;
			ulInfoLen = 1;
		}
		break;
	
	case OID_RT_11N_FORCED_STBC:
		{
			PRT_NDIS_COMMON			pNdisCommon = pAdapter->pNdisCommon;
			ulInfo = pNdisCommon->RegStbc;
			ulInfoLen = 1;
		}
		break;
		
	case OID_RT_BAND_SELECT:
		{
			ulInfo = pMgntInfo->RegPreferBand;		
			RT_TRACE(COMP_OID_SET,DBG_LOUD,("Qyery PreferBand=%d\n", pMgntInfo->RegPreferBand));
			ulInfoLen = 4;
		}
		break;
		
	}

	if(Status == NDIS_STATUS_SUCCESS)
	{        
		if(ulInfoLen <= InformationBufferLength)
		{
			// Copy result into InformationBuffer
			if(ulInfoLen && !bInformationCopied)
			{
				*BytesWritten = ulInfoLen;
				NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
			}
		}
		else if(!bInformationCopied)
		{
			// Buffer too short
			*BytesNeeded = ulInfoLen;
			Status = NDIS_STATUS_BUFFER_TOO_SHORT;
		}
	}

	return Status;
}


// This is to offload the N6CQueryInformation() since the huge size of the funcion stack causes the BSOD when DbgPrint.
static NDIS_STATUS
N6CQueryInformationHandleCustomizedSecurityOids(
	IN	PADAPTER		pAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
	)
{
	PMGNT_INFO      		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	UCHAR					ucInfo = 0;
	USHORT					usInfo = 0;
	ULONG					ulInfo = 0;
	LONG				lInfo = 0;
	ULONGLONG				ul64Info = 0;

	PVOID					pInfo = (PVOID) &ulInfo;
	ULONG					ulInfoLen = sizeof(ulInfo);
	BOOLEAN					bInformationCopied = FALSE;

	FunctionIn(COMP_OID_QUERY);

	*BytesWritten = 0;
	*BytesNeeded = 0;

	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;

	case OID_802_11_CAPABILITY:
	case OID_RT_802_11_CAPABILITY:
		pInfo = &(pMgntInfo->SecurityInfo.szCapability[0]);
		Query_802_11_CAPABILITY( pAdapter, (pu1Byte)pInfo, &ulInfoLen );
		RT_TRACE( COMP_OID_QUERY,  DBG_WARNING, ("OID_802_11_CAPABILITY Querying (Warning: Old OID in NDIS5, do nothing)\n") );
		break;

	case OID_802_11_PMKID:
	case OID_RT_802_11_PMKID:
		RT_TRACE( COMP_OID_QUERY,  DBG_WARNING, ("OID_802_11_PMKID Querying (Warning: Old OID in NDIS5, do nothing)\n") );
		break;

	case OID_802_11_AUTHENTICATION_MODE:
	case OID_RT_802_11_AUTHENTICATION_MODE:
		RT_TRACE( COMP_OID_QUERY,  DBG_WARNING, ("OID_802_11_AUTHENTICATION_MODE Querying (Warning: Old OID in NDIS5, do nothing)\n") );
		{
			RT_AUTH_MODE	authmode;

			MgntActQuery_802_11_AUTHENTICATION_MODE( pAdapter, &authmode );
			switch( authmode )
			{
			case RT_802_11AuthModeOpen:
				ulInfo = Ndis802_11AuthModeOpen;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: Ndis802_11AuthModeOpen\n"));
				break;
			case RT_802_11AuthModeShared:
				ulInfo = Ndis802_11AuthModeShared;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: Ndis802_11AuthModeShared\n"));
				break;
			case RT_802_11AuthModeAutoSwitch:
				ulInfo = Ndis802_11AuthModeAutoSwitch;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: RT_802_11AuthModeAutoSwitch\n"));
				break;
			case RT_802_11AuthModeWPA:
				ulInfo = Ndis802_11AuthModeWPA;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: RT_802_11AuthModeWPA\n"));
				break;
			case RT_802_11AuthModeWPAPSK:
				ulInfo = Ndis802_11AuthModeWPAPSK;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: RT_802_11AuthModeWPAPSK\n"));
				break;
			case RT_802_11AuthModeWPANone:
				ulInfo = Ndis802_11AuthModeWPANone;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: RT_802_11AuthModeWPANone\n"));
				break;
			case RT_802_11AuthModeWPA2:
				ulInfo = Ndis802_11AuthModeWPA2;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: RT_802_11AuthModeWPA2\n"));
				break;
			case RT_802_11AuthModeWPA2PSK:
				ulInfo = Ndis802_11AuthModeWPA2PSK;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: RT_802_11AuthModeWPA2PSK\n"));
				break;
			case RT_802_11AuthModeWAPI:
				ulInfo = 8;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: Ndis802_11AuthModeWAPIPSK\n"));
				break;
			case RT_802_11AuthModeCertificateWAPI:
				ulInfo = 9;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: Ndis802_11AuthModeWAPICERTIFICATE\n"));
				break;
			case RT_802_11AuthModeMax:
				ulInfo = Ndis802_11AuthModeMax;           // Not a real mode,
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: Ndis802_11AuthModeMax \n"));
				break;
			default:
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_AUTHENTICATION_MODE: unknow authmode: 0x%X\n", authmode));
				break;
			}
		}
		break;

	case OID_802_11_ASSOCIATION_INFORMATION:
	case OID_RT_802_11_ASSOCIATION_INFORMATION:
		RT_TRACE( COMP_OID_QUERY,  DBG_WARNING, ("OID_802_11_ASSOCIATION_INFORMATION Querying (Warning: Old OID in NDIS5, do nothing)\n") );
		{
			PNDIS_802_11_ASSOCIATION_INFORMATION pAssocInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION)(pMgntInfo->SecurityInfo.AssocInfo);

			MgntActQuery_802_11_ASSOCIATION_INFORMATION(pAdapter, pAssocInfo);
	
			pInfo = pAssocInfo;
			ulInfoLen = sizeof(NDIS_802_11_ASSOCIATION_INFORMATION) +	\
											pAssocInfo->RequestIELength + 				\
											pAssocInfo->ResponseIELength;
	
			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_ASSOCIATION_INFORMATION, length=%d\n", ulInfoLen ) );
			RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "pInfo", pInfo, ulInfoLen );
		}
		break;

	//case OID_802_11_WEP_STATUS:
	case OID_802_11_ENCRYPTION_STATUS:
	case OID_RT_802_11_ENCRYPTION_STATUS:
		switch(pMgntInfo->SecurityInfo.EncryptionStatus)
		{
			case RT802_11Encryption1Enabled:
				if(pMgntInfo->SecurityInfo.KeyLen[pMgntInfo->SecurityInfo.DefaultTransmitKeyIdx] == 0)
					ulInfo = RT802_11Encryption1KeyAbsent;
				else
					ulInfo = pMgntInfo->SecurityInfo.EncryptionStatus;
				break;
				
			case RT802_11Encryption2Enabled:
				if( pMgntInfo->SecurityInfo.KeyLen[PAIRWISE_KEYIDX] == 0)
					ulInfo = RT802_11Encryption2KeyAbsent;
				else
					ulInfo = pMgntInfo->SecurityInfo.EncryptionStatus;
				break;
				
			case RT802_11Encryption3Enabled:
				// For AES, Added by Annie, 2005-09-15.
				if( pMgntInfo->SecurityInfo.KeyLen[PAIRWISE_KEYIDX] == 0)	// Should I check pMgntInfo->SecurityInfo.AESKeyBuf len???
					ulInfo = RT802_11Encryption3KeyAbsent;
				else
					ulInfo = pMgntInfo->SecurityInfo.EncryptionStatus;
				break;

			case Wapi_Encryption:
				  ulInfo = pMgntInfo->SecurityInfo.EncryptionStatus;
				  break;
			case Wapi_Certificate:
				  ulInfo = pMgntInfo->SecurityInfo.EncryptionStatus;
				  break;

			default:
				ulInfo = pMgntInfo->SecurityInfo.EncryptionStatus;
				break;
		}
		RT_TRACE( COMP_OID_QUERY,  DBG_WARNING, ("OID_802_11_ENCRYPTION_STATUS Querying (Warning: Old OID in NDIS5, do nothing)\n") );
		break;

	// by Owen on 03/07/03 for Windows ME HCT 9.6
	case OID_802_11_ADD_WEP:
	case OID_802_11_REMOVE_WEP:
	case OID_802_11_DISASSOCIATE:
	case OID_802_11_BSSID_LIST_SCAN:
	case OID_802_11_RELOAD_DEFAULTS:
	case OID_RT_802_11_ADD_WEP:
	case OID_RT_802_11_REMOVE_WEP:
	case OID_RT_802_11_DISASSOCIATE:
	case OID_RT_802_11_BSSID_LIST_SCAN:
	case OID_RT_802_11_RELOAD_DEFAULTS:
		ulInfo = 1;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query some OIDs for Windows ME HCT 9.6.\n"));
		break;

	}

	if(Status == NDIS_STATUS_SUCCESS)
	{        
		if(ulInfoLen <= InformationBufferLength)
		{
			// Copy result into InformationBuffer
			if(ulInfoLen && !bInformationCopied)
			{
				*BytesWritten = ulInfoLen;
				NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
			}
		}
		else if(!bInformationCopied)
		{
			// Buffer too short
			*BytesNeeded = ulInfoLen;
			Status = NDIS_STATUS_BUFFER_TOO_SHORT;
		}
	}

	return Status;
}


// This is to offload the N6CQueryInformation() since the huge size of the funcion stack causes the BSOD when DbgPrint.
static NDIS_STATUS
N6CQueryInformationHandleCustomizedOids(
	IN	PADAPTER		pAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
	)
{
	PMGNT_INFO      		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	UCHAR					ucInfo = 0;
	USHORT					usInfo = 0;
	ULONG					ulInfo = 0;
	LONG				lInfo = 0;
	ULONGLONG				ul64Info = 0;

	PVOID					pInfo = (PVOID) &ulInfo;
	ULONG					ulInfoLen = sizeof(ulInfo);
	BOOLEAN					bInformationCopied = FALSE;
	u4Byte					i;

	FunctionIn(COMP_OID_QUERY);

	*BytesWritten = 0;
	*BytesNeeded = 0;

	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;

	//-------- Original 802.11 OID--------------
	case OID_802_11_BSSID:
	case OID_RT_802_11_BSSID:
		if( (pMgntInfo->mIbss==TRUE) || (pMgntInfo->mAssoc==TRUE) )
		{
			pInfo = &pMgntInfo->Bssid[0];
			ulInfoLen = 6 ;
		}
		else
		{
			Status = NDIS_STATUS_ADAPTER_NOT_READY;
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n", 
			pMgntInfo->Bssid[0], pMgntInfo->Bssid[1], pMgntInfo->Bssid[2], pMgntInfo->Bssid[3], pMgntInfo->Bssid[4], pMgntInfo->Bssid[5]));
		break;

	case OID_802_11_SSID:
	case OID_RT_802_11_SSID:
		if( (pMgntInfo->mIbss==1) || (pMgntInfo->mAssoc==1) ) // 2005.02.01, by rcnjko.
		{
			u2Byte    len;

			PlatformZeroMemory( &(pNdisCommon->TmpNdisSsid.Ssid[0]), 32 );
			MgntActQuery_802_11_SSID( pAdapter, &(pNdisCommon->TmpNdisSsid.Ssid[0]), &len );
			if( len <= 32 )
			{
				pNdisCommon->TmpNdisSsid.SsidLength = len;
				//RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_SSID: <%s>\n",pNdisCommon->TmpNdisSsid.Ssid));
				RT_PRINT_STR( COMP_OID_QUERY, DBG_LOUD, "Query OID_802_11_SSID", pNdisCommon->TmpNdisSsid.Ssid, pNdisCommon->TmpNdisSsid.SsidLength );
			}
			else
			{
				pNdisCommon->TmpNdisSsid.SsidLength = 0;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_SSID: NDIS_STATUS_ADAPTER_NOT_READY\n"));
			}
		}
		else
		{
			pNdisCommon->TmpNdisSsid.SsidLength = 0;
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_SSID: NDIS_STATUS_ADAPTER_NOT_READY\n"));
		}

		pInfo = &(pNdisCommon->TmpNdisSsid);
		ulInfoLen = pNdisCommon->TmpNdisSsid.SsidLength + FIELD_OFFSET(NDIS_802_11_SSID, Ssid); // Use FIELD_OFFSET for x64 safe, 2005.10.12, by rcnjko.
		break;

	case OID_802_11_NETWORK_TYPES_SUPPORTED:
	case OID_RT_802_11_NETWORK_TYPES_SUPPORTED:
		{
			u2Byte SupportedBand = pAdapter->HalFunc.GetSupportedWirelessModeHandler(pAdapter);
			u1Byte index, num=0;

			struct{WIRELESS_MODE wm; NDIS_802_11_NETWORK_TYPE nt; } BandSetList[3] = 
				{	{WIRELESS_MODE_B, Ndis802_11DS},
					{(WIRELESS_MODE_N_5G|WIRELESS_MODE_A), Ndis802_11OFDM5},
					{(WIRELESS_MODE_AC_24G|WIRELESS_MODE_N_24G|WIRELESS_MODE_G), Ndis802_11OFDM24}	};

			for(index=0; index<3; index++)
			{
				if(SupportedBand & BandSetList[index].wm)
				{
					pNdisCommon->TmpNdisNetworkTypeList.NetworkType[num] = BandSetList[index].nt;
					num++;
				}
			}
			RT_ASSERT((num!=0), ("N5CQueryInformation(): OID_802_11_NETWORK_TYPES_SUPPORTED no value return!!\n"));
			pNdisCommon->TmpNdisNetworkTypeList.NumberOfItems = num;

			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N5CQueryInformation(): Number of Network Type:%d", num));


			pInfo = &(pNdisCommon->TmpNdisNetworkTypeList);
			ulInfoLen = sizeof(NDIS_802_11_NETWORK_TYPE_LIST);
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_NETWORK_TYPES_SUPPORTED: \n"));
		}
		break;

	case OID_802_11_NETWORK_TYPE_IN_USE:
	case OID_RT_802_11_NETWORK_TYPE_IN_USE:
		if (pNdisCommon->BeSetNetworkTypeByNDTest)
		{
			ulInfo = pNdisCommon->BeSetNetworkTypeByNDTest;
			RT_TRACE(COMP_DBG, DBG_LOUD, ("Query OID_802_11_NETWORK_TYPE_IN_USE: BeSetNetworkTypeByNDTest 0x%0X\n", ulInfo));
			break;
		}
		switch(pMgntInfo->dot11CurrentWirelessMode)
		{
			case WIRELESS_MODE_B:
				ulInfo = Ndis802_11DS;
				RT_TRACE(COMP_DBG, DBG_LOUD, ("Query OID_802_11_NETWORK_TYPE_IN_USE: B Ndis802_11DS\n"));
				break;
			case WIRELESS_MODE_G:
			case WIRELESS_MODE_N_24G:
			case WIRELESS_MODE_AC_24G:
				ulInfo = Ndis802_11OFDM24;
				RT_TRACE(COMP_DBG, DBG_LOUD, ("Query OID_802_11_NETWORK_TYPE_IN_USE: G Ndis802_11OFDM24\n"));
				break;
			case WIRELESS_MODE_A:
			case WIRELESS_MODE_N_5G:
				ulInfo = Ndis802_11OFDM5;
				RT_TRACE(COMP_DBG, DBG_LOUD, ("Query OID_802_11_NETWORK_TYPE_IN_USE: A Ndis802_11OFDM5\n"));
				break;
			default:
				ulInfo = Ndis802_11DS;
				RT_TRACE(COMP_DBG, DBG_LOUD, ("Query OID_802_11_NETWORK_TYPE_IN_USE: default Ndis802_11DS\n"));
				break;
		}
		break;

	case OID_802_11_TX_POWER_LEVEL:
	case OID_RT_802_11_TX_POWER_LEVEL:
		// <RJ_TODO> We shall return the value of tx power in mW.
		ulInfo = 100;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_TX_POWER_LEVEL: \n"));
		break;


	case OID_RT_TX_POWER:
		{
			if( InformationBufferLength < sizeof(s4Byte) )
			{
				*BytesNeeded = sizeof(s4Byte);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 
			
			*((ps4Byte)InformationBuffer) = MgntActQuery_TX_POWER_DBM(pAdapter);

			ulInfoLen = sizeof(s4Byte);
			*BytesWritten = ulInfoLen; 
			bInformationCopied = TRUE;

			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_TX_POWER: %d dBm\n", *((ps4Byte)InformationBuffer)));
		}
		break;

	case OID_RT_TX_POWER_RANGE:
		{
			// 2 Bytes to store min (4Byte) and max (4Byte)
			if(InformationBufferLength < 2 * sizeof(s4Byte))
			{
				*BytesNeeded = 2 * sizeof(s4Byte);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			PlatformZeroMemory(InformationBuffer, 2 * sizeof(s4Byte));

			pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_MIN_TX_POWER_DBM, (pu1Byte)InformationBuffer);
			pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_MAX_TX_POWER_DBM, ((pu1Byte)InformationBuffer + 4));

			ulInfoLen = 2 * sizeof(s4Byte);
			*BytesWritten = ulInfoLen; 
			bInformationCopied = TRUE;
			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_TX_POWER_RANGE: min = %d dBm, max = %d\n", *((ps4Byte)InformationBuffer), *(((ps4Byte)InformationBuffer + 1))));
		}
		break;

	case OID_802_11_RSSI:
	case OID_RT_802_11_RSSI:
		// It had already translated to dBm (x=0.5y-95).
		if(pMgntInfo->AntennaTest == 1)
		{
			u4Byte	diff=0, pwdball_diff=0;
			diff = pAdapter->RxStats.RssiCalculateCnt - pAdapter->RxStats.RssiOldCalculateCnt;
			pAdapter->RxStats.RssiOldCalculateCnt = pAdapter->RxStats.RssiCalculateCnt;
			pwdball_diff = pAdapter->RxStats.PWDBAllCnt - pAdapter->RxStats.PWDBAllOldCnt;

			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_RSSI: PWDBAllCnt  %d PWDBAllOLDCnt %d\n", pAdapter->RxStats.PWDBAllCnt, pAdapter->RxStats.PWDBAllOldCnt));
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_RSSI: diff  %d pwdbdiff %d\n", diff, pwdball_diff));
			
			pAdapter->RxStats.PWDBAllOldCnt = pAdapter->RxStats.PWDBAllCnt;
			
			if(diff >= 3)	// calculate more than 3 times of rssi
			{
				lInfo = (((pwdball_diff/diff)+1)>>1)-95;
			}
			else
			{
				lInfo = -80+(u4Byte)diff;	// random number.
			}

		}
		else
			lInfo = pAdapter->RxStats.SignalStrength;
		pInfo = &lInfo;
		ulInfoLen = sizeof(lInfo);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_RSSI: %d\n", lInfo));
		break;

	case OID_802_11_RSSI_TRIGGER:
	case OID_RT_802_11_RSSI_TRIGGER:
		ulInfo = pNdisCommon->NdisRssiTrigger;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_RSSI_TRIGGER: %d\n", ulInfo));
		break;

	case OID_802_11_INFRASTRUCTURE_MODE:
	case OID_RT_802_11_INFRASTRUCTURE_MODE:
		{
			RT_JOIN_NETWORKTYPE	networktype = MgntActQuery_802_11_INFRASTRUCTURE_MODE(pAdapter);

			switch( networktype )
			{
			case RT_JOIN_NETWORKTYPE_INFRA:
				ulInfo = Ndis802_11Infrastructure;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_INFRASTRUCTURE_MODE: Ndis802_11INFRA\n"));
				break;
			case RT_JOIN_NETWORKTYPE_ADHOC:
				ulInfo = Ndis802_11IBSS;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_INFRASTRUCTURE_MODE: Ndis802_11IBSS\n"));
				break;
			case RT_JOIN_NETWORKTYPE_AUTO:
				ulInfo = Ndis802_11IBSS;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_INFRASTRUCTURE_MODE: Ndis802_11AUTO <Ndis802_11IBSS>\n"));
				break;
			default:
				ulInfo = Ndis802_11AutoUnknown;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_INFRASTRUCTURE_MODE: Ndis802_11unknown\n"));
				break;
			}
		}
		break;

	case OID_802_11_FRAGMENTATION_THRESHOLD:
	case OID_RT_802_11_FRAGMENTATION_THRESHOLD:
		ulInfo = (ULONG)MgntActQuery_802_11_FRAGMENTATION_THRESHOLD(pAdapter);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_FRAGMENTATION_THRESHOLD: %d\n", ulInfo));
		break;

	case OID_802_11_RTS_THRESHOLD:
	case OID_RT_802_11_RTS_THRESHOLD:
		ulInfo = (ULONG)MgntActQuery_802_11_RTS_THRESHOLD(pAdapter); 
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_RTS_THRESHOLD: %d\n", ulInfo));
		break;

	case OID_802_11_SUPPORTED_RATES:
	case OID_RT_802_11_SUPPORTED_RATES:
		//TODO:
		ulInfo = 1;

		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_SUPPORTED_RATES: \n"));
		break;

	case OID_802_11_DESIRED_RATES:
	case OID_RT_802_11_DESIRED_RATES:
		//TODO:
		ulInfo = 1;

		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_DESIRED_RATES: \n"));
		break;

	case OID_802_11_CONFIGURATION:
	case OID_RT_802_11_CONFIGURATION:
		{
			pNdisCommon->TmpConfig80211.Length = sizeof(NDIS_802_11_CONFIGURATION);

			if( (pMgntInfo->mAssoc) || (pMgntInfo->mIbss) )
			{
				pNdisCommon->TmpConfig80211.BeaconPeriod = pMgntInfo->Regdot11BeaconPeriod;
			}
			else
			{
				pNdisCommon->TmpConfig80211.BeaconPeriod = 0;
			}

			pNdisCommon->TmpConfig80211.ATIMWindow = pMgntInfo->mIbssParms.atimWin;

			if(GET_HAL_DATA(pAdapter)->CurrentBandType == BAND_ON_2_4G)
				pNdisCommon->TmpConfig80211.DSConfig = DSSS_Freq_Channel[pMgntInfo->dot11CurrentChannelNumber]*1000;
			else if(GET_HAL_DATA(pAdapter)->CurrentBandType == BAND_ON_5G)
				pNdisCommon->TmpConfig80211.DSConfig = (5000+5*pMgntInfo->dot11CurrentChannelNumber)*1000;

			pNdisCommon->TmpConfig80211.FHConfig.Length = sizeof(NDIS_802_11_CONFIGURATION_FH);
			pNdisCommon->TmpConfig80211.FHConfig.HopPattern = 0;
			pNdisCommon->TmpConfig80211.FHConfig.HopSet = 0;
			pNdisCommon->TmpConfig80211.FHConfig.DwellTime = 0;

			pInfo = &(pNdisCommon->TmpConfig80211);
			ulInfoLen = pNdisCommon->TmpConfig80211.Length;	
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_CONFIGURATION: \n"));
		break;

	case OID_802_11_STATISTICS:
	case OID_RT_802_11_STATISTICS:
		//TODO:
		ulInfo = 1;
		Status = NDIS_STATUS_NOT_SUPPORTED; //Added by Jay 0523
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_STATISTICS: \n"));
		break;

	case OID_802_11_POWER_MODE:
	case OID_RT_802_11_POWER_MODE:
		ulInfo = pNdisCommon->RegPowerSaveMode;
		if(Oid == OID_RT_802_11_POWER_MODE && InformationBufferLength >= 8)
		{
			*((pu4Byte)InformationBuffer) = pNdisCommon->RegPowerSaveMode;
			*((pu4Byte)((pu1Byte)InformationBuffer + 4)) = (u4Byte)pMgntInfo->PsPollType;
			// Setup variables to return. 
			*BytesWritten = 8;
			bInformationCopied = TRUE;
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_11_POWER_MODE: 0x%X\n", ulInfo));
		break;

	case OID_802_11_PRIVACY_FILTER:
	case OID_RT_802_11_PRIVACY_FILTER:
		//TODO:
		ulInfo = Ndis802_11PrivFilterAcceptAll;
		RT_TRACE( COMP_OID_QUERY,  DBG_WARNING, ("OID_802_11_PRIVACY_FILTER Querying (Warning: Old OID in NDIS5, do nothing)\n") );
		break;	

	case OID_RT_GET_SCAN_IN_PROGRESS:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_GET_SCAN_IN_PROGRESS.\n"));
		ulInfo = 0xAA;					// 0xAA is fixed pattern
		if((pMgntInfo->bScanInProgress == TRUE) || (pMgntInfo->PowerSaveControl.ReturnPoint == IPS_CALLBACK_MGNT_LINK_REQUEST))
			ulInfo |= 0x0100;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_GET_SCAN_IN_PROGRESS: %x.\n", ulInfo));
		break;

	case OID_RT_GET_SIGNAL_QUALITY:
		if(pMgntInfo->mAssoc || pMgntInfo->mIbss)
		{
			// 2013/09/02 MH Add to enhance more SQ value.
			if (pAdapter->RxStats.SignalQuality >= 80)
			{
				ulInfo = 100;
			}			
			else if (pAdapter->RxStats.SignalQuality <= 79)
			{
				ulInfo = pAdapter->RxStats.SignalQuality+20;
			}

			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("OID_RT_GET_SIGNAL_QUALITY %x.\n", ulInfo));	

		}
		else
		{
			ulInfo = 0xffffffff; // It stands for -1 in 4-byte integer.
		}
		break;

	case OID_RT_GET_CONNECT_STATE:
		// Rearrange the order to let the UI still shows connection when scan is in progress
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_GET_CONNECT_STATE.\n"));
		if(pMgntInfo->mAssoc)
			ulInfo = 1;
		else if(pMgntInfo->mIbss)
			ulInfo = 2;
		else if(pMgntInfo->bScanInProgress)
			ulInfo = 0;
		else
			ulInfo = 3;
		ulInfoLen = sizeof(ULONG);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_GET_CONNECT_STATE: %d\n", ulInfo));
		break;

	case OID_RT_GET_CHANNELPLAN:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_GET_CHANNELPLAN.\n"));
		ulInfo = (ULONG)pMgntInfo->ChannelPlan;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_GET_CHANNELPLAN: %d\n", ulInfo));
		break;

	case OID_RT_GET_CHANNEL:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_GET_CHANNEL.\n"));
		// For damn Netgear WG111V2.
		if(pMgntInfo->mAssoc || pMgntInfo->mIbss)
		{
			ulInfo = pMgntInfo->dot11CurrentChannelNumber;
		}
		else if(ACTING_AS_AP(pAdapter) ||IsExtAPModeExist(pAdapter))
		{
			//sherry added for ui show correct channel for AP mode 20110517		
			ulInfo = pMgntInfo->dot11CurrentChannelNumber;
		}

		else
		{
			ulInfo = RT_GetChannelNumber(pAdapter);
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_GET_CHANNEL: %d\n", ulInfo));
		break;

		
	case OID_RT_GET_CHANNEL_LIST:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_GET_CHANNEL_LIST.\n"));
		{
			pu1Byte pList;
			PRT_CHANNEL_LIST	pChannelList = NULL;
			u1Byte i, chnlcount=0;
			u4Byte cbBufSizeRequired;
			RT_CHNL_LIST_ENTRY		ChnlListEntryArray[MAX_CHANNEL_NUM] = {0};
			u1Byte		ChannelLen;
			BOOLEAN		bIndicateToUI = FALSE;
			

			
			if(IS_DUAL_BAND_SUPPORT(pAdapter))
			{
				ChannelLen = RtGetDualBandChannel(pAdapter, ChnlListEntryArray);
				if(ChannelLen == 0)
				{
					return NDIS_STATUS_ADAPTER_NOT_READY;
				}
			}
			else
			{
				pChannelList = MgntActQuery_ChannelList(pAdapter);

				if(!pChannelList)
				{
					return NDIS_STATUS_ADAPTER_NOT_READY;
				}

				for(i=0;i<pChannelList->ChannelLen;i++)
				{
					PlatformMoveMemory(&ChnlListEntryArray[i], &pChannelList->ChnlListEntry[i], sizeof(RT_CHNL_LIST_ENTRY));
					RT_TRACE(COMP_INIT,DBG_LOUD,(("ChnlListEntryArray[i]ChannelNum 00  %d \n"),ChnlListEntryArray[i].ChannelNum));

				}

				ChannelLen =  pChannelList->ChannelLen;
			}
		
			

			RT_TRACE(COMP_OID_QUERY, DBG_LOUD,("OID_RT_EASY_COCURRENT_GET_CHANNEL_INFO channelLen 2 %d \n",ChannelLen));

			if(P2P_ENABLED(GET_P2P_INFO(pAdapter)) && MgntScanInProgress(&pAdapter->MgntInfo))
			{
				*BytesWritten = 0;
				return NDIS_STATUS_MEDIA_BUSY;
			}

			cbBufSizeRequired = 2 + ChannelLen;
				
			// Check output buffer length. 
			if(InformationBufferLength < cbBufSizeRequired)
			{
				*BytesNeeded = cbBufSizeRequired;
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			pList = (pu1Byte)InformationBuffer;
			// A tag identified by UI. 
			pList[0] = 0xAA;	
			
			for(i = 0; i < ChannelLen; i++)
			{
				bIndicateToUI = FALSE;
				if( (ChnlListEntryArray[i].ScanType == SCAN_ACTIVE) ||
				(DFS_5G_RADAR_CHANNEL(ChnlListEntryArray[i].ChannelNum)) )
				{
					bIndicateToUI = TRUE;
				}

				//according to derzheng, temply do this, vivi, 20100114
				// 2013/08/12 MH Revise for DFS channel can not indicate to UI.
				if(DFS_5G_RADAR_CHANNEL(ChnlListEntryArray[i].ChannelNum))
				{
					bIndicateToUI = FALSE;
				}

				if(bIndicateToUI)
				{
					if(IS_DUAL_BAND_SUPPORT(pAdapter))
						pList[chnlcount+2] = ChnlListEntryArray[i].ChannelNum;
					else
						pList[chnlcount+2] = pChannelList->ChnlListEntry[i].ChannelNum;
					chnlcount++;
				}

			}

			// Length of channel list.
			pList[1] = chnlcount; 
			cbBufSizeRequired = chnlcount + 2;
			// Setup variables to return. 
			*BytesWritten = cbBufSizeRequired;
			bInformationCopied = TRUE;
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_GET_CHANNEL_LIST.\n"));
		break;


	 	case OID_RT_EASY_COCURRENT_GET_CHANNEL_INFO:
		RT_TRACE(COMP_INIT,DBG_LOUD,(" ====>OID_RT_EASY_COCURRENT_GET_CHANNEL_INFO\n"));
		{
			pu1Byte pList;
			PRT_CHANNEL_LIST	pChannelList = NULL;
			u1Byte i, chnlcount=0;
			u4Byte cbBufSizeRequired;
			RT_CHNL_LIST_ENTRY		ChnlListEntryArray[MAX_CHANNEL_NUM] = {0};
			u1Byte		ChannelLen;
			BOOLEAN		bAP = FALSE,bIndicateToUI = FALSE;

			{
				pChannelList = MgntActQuery_ChannelList(pAdapter);

				if(!pChannelList)
				{
					return NDIS_STATUS_ADAPTER_NOT_READY;
				}

				for(i=0;i<pChannelList->ChannelLen;i++)
				{
					PlatformMoveMemory(&ChnlListEntryArray[i], &pChannelList->ChnlListEntry[i], sizeof(RT_CHNL_LIST_ENTRY));
					RT_TRACE(COMP_INIT,DBG_LOUD,(("ChnlListEntryArray[i]ChannelNum 00  %d \n"),ChnlListEntryArray[i].ChannelNum));

				}

				ChannelLen =  pChannelList->ChannelLen;
			}
			

			RT_TRACE(COMP_EASY_CONCURRENT,DBG_LOUD,("OID_RT_EASY_COCURRENT_GET_CHANNEL_INFO channelLen 2 %d \n",ChannelLen));

			cbBufSizeRequired = 2 + ChannelLen;
				
			// Check output buffer length. 
			if(InformationBufferLength < cbBufSizeRequired)
			{
				*BytesNeeded = cbBufSizeRequired;
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			pList = (pu1Byte)InformationBuffer;
			// A tag identified by UI. 
			pList[0] = 0xAA;	

			{
				for(i = 0; i < ChannelLen; i++)
				{
					bIndicateToUI = FALSE;
					if( (ChnlListEntryArray[i].ScanType == SCAN_ACTIVE) ||
					(DFS_5G_RADAR_CHANNEL(ChnlListEntryArray[i].ChannelNum)) )
					{
						bIndicateToUI = TRUE;
					}

					//according to derzheng, temply do this, vivi, 20100114
					if(ChnlListEntryArray[i].ChannelNum == 52||
						ChnlListEntryArray[i].ChannelNum == 56||
						ChnlListEntryArray[i].ChannelNum == 60||
						ChnlListEntryArray[i].ChannelNum == 64)
					{
						bIndicateToUI = FALSE;
					}

					if(bIndicateToUI)
					{
//change by ylb  for UI get channel num error 20110906	
							pList[chnlcount+2] = pChannelList->ChnlListEntry[i].ChannelNum;
						chnlcount++;
					}

				}

			}

			// Length of channel list.
			pList[1] = chnlcount; 

			cbBufSizeRequired = chnlcount + 2;
			// Setup variables to return. 
			*BytesWritten = cbBufSizeRequired;
			bInformationCopied = TRUE;
		}
		RT_TRACE(COMP_EASY_CONCURRENT,DBG_LOUD,(" <==== OID_RT_EASY_COCURRENT_GET_CHANNEL_INFO\n"));
	 	break;		

	case OID_RT_GET_TOTAL_TX_BYTES:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_GET_TOTAL_TX_BYTES.\n"));
		pInfo = &ul64Info;
		ulInfoLen = sizeof(ul64Info);
		ul64Info = pAdapter->TxStats.NumTxOkBytesTotal;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_GET_TOTAL_TX_BYTES.\n"));
		break;

	case OID_RT_GET_TOTAL_RX_BYTES:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_GET_TOTAL_RX_BYTES.\n"));
		pInfo = &ul64Info;
		ulInfoLen = sizeof(ul64Info);
		ul64Info = pAdapter->RxStats.NumRxOkBytesTotal;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_GET_TOTAL_RX_BYTES.\n"));
		break;

	case OID_RT_QUERY_ONE_BYTE_ALIGNMENT:
		ulInfo = IS_ONE_BYTE_ALIGNED_SERIES(pAdapter);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_QUERY_ONE_BYTE_ALIGNMENT: %d\n", ulInfo));
		break;

	case OID_RT_SUPPORTED_WIRELESS_MODE:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_SUPPORTED_WIRELESS_MODE.\n"));
		{
			u2Byte btSupportedWirelessMode;

			// Initialize return parameter:
			// in low byte of a WORD, 0xAA is fixed pattern.
			ulInfo = 0xAA;

			// Get supported wireless mode.
			btSupportedWirelessMode = pAdapter->HalFunc.GetSupportedWirelessModeHandler(pAdapter);

			// Fill up return parameter according to supported wireless mode. 
 			// In high byte of a WORD, e.g. 0x07 means a(bit 2),g(bit 1),b(bit 0).
			if(btSupportedWirelessMode & WIRELESS_MODE_B)
			{
				ulInfo |= 0x0100; 
			}
			if(btSupportedWirelessMode & WIRELESS_MODE_G)
			{
				ulInfo |= 0x0200; 
			}
			if(btSupportedWirelessMode & WIRELESS_MODE_A)
			{
				ulInfo |= 0x0400; 
			}
			if(btSupportedWirelessMode & WIRELESS_MODE_N_24G)
			{
				ulInfo |= 0x0800; 
			}
			if(btSupportedWirelessMode & WIRELESS_MODE_N_5G)
			{
				ulInfo |= 0x1000; 
			}
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_SUPPORTED_WIRELESS_MODE: %x\n", ulInfo));
		break;

	case OID_RT_WIRELESS_MODE:
		{
			WIRELESS_MODE WirelessMode;

			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_WIRELESS_MODE.\n"));
			
			// Check output buffer length. 
			if(InformationBufferLength < sizeof(WIRELESS_MODE))
			{
				*BytesNeeded = sizeof(WIRELESS_MODE);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
	
			// Get wireless mode.
			WirelessMode = MgntActQuery_802_11_WIRELESS_MODE(pAdapter);

			// Fill up paramters to return. 
			*((WIRELESS_MODE*)InformationBuffer) = WirelessMode;
			*BytesWritten = sizeof(WIRELESS_MODE);
			bInformationCopied = TRUE;
	
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_WIRELESS_MODE: %d\n", WirelessMode));
		}
		break; 

	case OID_RT_CURRENT_WLRELESSMODE:
		{
			WIRELESS_MODE WirelessMode;

			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_CURRENT_WLRELESSMODE.\n"));
			
			// Check output buffer length. 
			if(InformationBufferLength < sizeof(WIRELESS_MODE))
			{
				*BytesNeeded = sizeof(WIRELESS_MODE);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
	
			// Get wireless mode.
			WirelessMode = pAdapter->MgntInfo.dot11CurrentWirelessMode;

			// Fill up paramters to return. 
			*((WIRELESS_MODE*)InformationBuffer) = WirelessMode;
			*BytesWritten = sizeof(WIRELESS_MODE);
			bInformationCopied = TRUE;
	
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_CURRENT_WLRELESSMODE: %d\n", WirelessMode));
		}

	case OID_RT_GET_DRIVER_UP_DELTA_TIME:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_GET_DRIVER_UP_DELTA_TIME.\n"));
		{	
			u8Byte now_time;

			now_time = PlatformGetCurrentTime(); // in micro-second.
			ulInfo = (ULONG)( (now_time - pAdapter->DriverUpTime) / 100000 ); // in 0.1 second.
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_GET_DRIVER_UP_DELTA_TIME: %u (0.1 Sec)\n", ulInfo));
		break;

	case OID_RT_GET_TX_RETRY:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->TxStats.NumTxRetryCount;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->TxStats.NumTxRetryCount;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_RT_GET_TX_BEACON_OK:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->TxStats.NumTxBeaconOk;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->TxStats.NumTxBeaconOk;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_RT_GET_TX_BEACON_ERR:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->TxStats.NumTxBeaconErr;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->TxStats.NumTxBeaconErr;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_RT_GET_RX_RETRY:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->RxStats.NumRxRetryCount;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->RxStats.NumRxRetryCount;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_RT_GET_RX_TOTAL_PACKET:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->RxStats.NumRxOkTotal + (pAdapter->RxStats.NumRxErrTotalUnicast + pAdapter->RxStats.NumRxErrTotalMulticast);
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)(pAdapter->RxStats.NumRxOkTotal + (pAdapter->RxStats.NumRxErrTotalUnicast + pAdapter->RxStats.NumRxErrTotalMulticast));
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_RT_GET_SMALL_PACKET_CRC:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->RxStats.NumRxCrcErrSmallPacket;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->RxStats.NumRxCrcErrSmallPacket;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_RT_GET_MIDDLE_PACKET_CRC:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->RxStats.NumRxCrcErrMiddlePacket;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->RxStats.NumRxCrcErrMiddlePacket;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_RT_GET_LARGE_PACKET_CRC:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->RxStats.NumRxCrcErrLargePacket;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->RxStats.NumRxCrcErrLargePacket;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_RT_GET_RX_ICV_ERR:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->RxStats.NumRxIcvErr;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->RxStats.NumRxIcvErr;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_RT_GET_BSS_WIRELESS_MODE:
		{
			WIRELESS_MODE WirelessMode;

			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_GET_BSS_WIRELESS_MODE.\n"));
			
			// Check output buffer length. 
			if(InformationBufferLength < sizeof(WIRELESS_MODE))
			{
				*BytesNeeded = sizeof(WIRELESS_MODE);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
	
			// Get wireless mode of current BSS.
			if(pMgntInfo->mAssoc || pMgntInfo->mIbss)
			{
				WirelessMode = pMgntInfo->CurrentBssWirelessMode;
			}
			else
			{
				WirelessMode = WIRELESS_MODE_UNKNOWN; 
			}

			// Fill up paramters to return. 
			*((WIRELESS_MODE*)InformationBuffer) = WirelessMode;
			*BytesWritten = sizeof(WIRELESS_MODE);
			bInformationCopied = TRUE;
	
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_GET_BSS_WIRELESS_MODE: %d\n", WirelessMode));
		}
		break; 

	case OID_RT_AP_GET_CURRENT_TIME_STAMP:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_AP_GET_CURRENT_TIME_STAMP.\n"));
		{
			PADAPTER pTargetAdapter = GetDefaultAdapter(pAdapter); // get the first AP mode. Neo Test 123, But this oid need to be modified for more APs.
			
			//
			// <Roger_Notes> We shall redirect to proper adapter for UI query. 
			// Otherwise, UI would retrieve abnormal value. 
			// Added by Roger, 2009.10.07.
			//
			while(pTargetAdapter != NULL)
			{
				if(ACTING_AS_AP(pTargetAdapter))
					break;
				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}

			if(pTargetAdapter == NULL)
				pTargetAdapter = GetDefaultAdapter(pAdapter);

			if(ACTING_AS_AP(pTargetAdapter))
			{
				LARGE_INTEGER ts;

				pInfo = &ul64Info;
				ulInfoLen = sizeof(ul64Info);

				NdisGetCurrentSystemTime(&ts);
				ul64Info = ts.QuadPart;
			}
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_AP_GET_CURRENT_TIME_STAMP.\n"));
		break;

	case OID_RT_AP_GET_ASSOCIATED_STATION_LIST:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_AP_GET_ASSOCIATED_STATION_LIST.\n"));
		{
			ULONG	ulBufRequired = 2*sizeof(ULONG) + ASSOCIATE_ENTRY_NUM*sizeof(RtlLibAssociateEntry);
			int		i;
			PRT_WLAN_STA			CurrAsocEntry;
			PRtlLibAssociateEntry	pAsocEntry;
			PADAPTER pTargetAdapter = GetDefaultAdapter(pAdapter);

			//
			// <Roger_Notes> We shall redirect to proper adapter for UI query. 
			// Otherwise, UI would retrieve abnormal value. 
			// Added by Roger, 2009.10.07.
			//

			// Determine size of buffer needed.
			if(InformationBufferLength < ulBufRequired) 
			{
				*BytesNeeded = ulBufRequired;
				*BytesWritten = 0;
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_AP_GET_ASSOCIATED_STATION_LIST: BytesNeeded: %d !!!\n", ulBufRequired));
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			while(pTargetAdapter != NULL)
			{
				if(ACTING_AS_AP(pTargetAdapter))
				{
					pMgntInfo = &(pTargetAdapter->MgntInfo);
					break;
				}
				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}

			// Initialize output buffer. 
			PlatformZeroMemory(InformationBuffer, ulBufRequired);

			// Fill up total size and size of each entry.
			*((PULONG)InformationBuffer) = ASSOCIATE_ENTRY_NUM*sizeof(RtlLibAssociateEntry); // Total size.
			*((PULONG)InformationBuffer + 1) = sizeof(RtlLibAssociateEntry); // Size of each entry.

			// Fill up each entry.
			pAsocEntry = (PRtlLibAssociateEntry)((PULONG)InformationBuffer + 2);
			CurrAsocEntry = pMgntInfo->AsocEntry;
			for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
			{
				if(CurrAsocEntry[i].bUsed && CurrAsocEntry[i].bAssociated)
				{
					pAsocEntry[i].bUsed = TRUE;
					pAsocEntry[i].Sum = CurrAsocEntry[i].Sum;
					PlatformMoveMemory(pAsocEntry[i].MacAddr, CurrAsocEntry[i].MacAddr, 6);
					pAsocEntry[i].AuthAlg = (CurrAsocEntry[i].AuthAlg==OPEN_SYSTEM) ? 0 : 1; // 0:Open, 1:Shared
					pAsocEntry[i].AuthPassSeq = CurrAsocEntry[i].AuthPassSeq;
					pAsocEntry[i].bAssociated = TRUE;
					pAsocEntry[i].AID = CurrAsocEntry[i].AID;
					pAsocEntry[i].LastActiveTime.QuadPart = CurrAsocEntry[i].LastActiveTime*10;
					RT_PRINT_ADDR(COMP_AP, DBG_TRACE, "OID_RT_AP_GET_ASSOCIATED_STATION_LIST: AsocEntry_GetEntry: \n", CurrAsocEntry[i].MacAddr);
				}
			}

			*BytesWritten = ulBufRequired; 
			bInformationCopied = TRUE;
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_AP_GET_ASSOCIATED_STATION_LIST.\n"));
		break;

	case OID_RT_SET_SCAN_OPERATION:
		{
			ulInfo = pMgntInfo->bDisableScanByOID;
		}
		break;
	
	case OID_RT_MH_VENDER_ID:	// Added by Annie, 2005-07-20.
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("OID_RT_MH_VENDER_ID(): Query OID_RT_MH_VENDER_ID. (MeetingHouse)\n"));
		{
			ulInfo = REALTEK_VENDER_ID;
		}
		break;

	case OID_RT_RF_OFF:
		{
			RT_RF_POWER_STATE eRfPowerState;

			pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_RF_STATE, (pu1Byte)(&eRfPowerState));
			if(eRfPowerState == eRfOff)
			{
				if (pMgntInfo->RfOffReason >= RF_CHANGE_BY_HW)
				{
					ulInfo = 1;
				}
				else
				{
					ulInfo = 0;
				}
			}
			else
			{
				ulInfo = 0;
			}
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("OID_RT_RF_OFF: eRfPowerState: 0x%X,  RFOff: 0x%X\n", eRfPowerState, ulInfo));
		}
		break;		

	case OID_RT_DBG_COMPONENT:
		ul64Info = GlobalDebugComponents;
		ulInfoLen = sizeof(GlobalDebugComponents);
		pInfo = &ul64Info;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_DBG_COMPONENT: %#X\n", ulInfo ));
		break;

	case OID_RT_DBG_LEVEL:
		ulInfo = GlobalDebugLevel;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_DBG_LEVEL: %#X\n", ulInfo));
		break;

	case OID_RT_AUTO_SELECT_CHANNEL:
		ulInfo = (ULONG)(pMgntInfo->bAutoSelChnl);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_AUTO_SELECT_CHANNEL: %d\n", ulInfo));
		break;

	case OID_RT_HIDDEN_SSID:
		ulInfo = (ULONG)(pMgntInfo->bHiddenSSID);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_HIDDEN_SSID: %d\n", ulInfo));
		break;

	case OID_RT_FILTER_DEFAULT_PERMITED:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_FILTER_DEFAULT_PERMITED\n"));
		{
			ulInfo = (ULONG)(pMgntInfo->bDefaultPermited);
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_FILTER_DEFAULT_PERMITED:  %d\n", ulInfo));			
		}
		break;
		
	case OID_RT_MAC_FILTER_TYPE:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_MAC_FILTER_TYPE\n"));
		{
			ulInfo = (ULONG)(pMgntInfo->LockType);
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_MAC_FILTER_TYPE:  %d\n", ulInfo));			
		}
		break;

	case OID_RT_LOCKED_STA_ADDRESS:
	case OID_RT_FILTER_STA_ADDRESS:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_LOCKED_STA_ADDRESS\n"));
		{
			pu4Byte	pNumOfLockedAddr = (pu4Byte)InformationBuffer;
			pu1Byte	pLockListBuffer = (pu1Byte)InformationBuffer + 4;
			u4Byte	LockListSize;
			if(pMgntInfo->LockType == MAC_FILTER_ACCEPT)
				LockListSize = sizeof(u4Byte) + pMgntInfo->LockedSTACount*6;
			else if(pMgntInfo->LockType ==MAC_FILTER_REJECT)
				LockListSize = sizeof(u4Byte) + pMgntInfo->LockedSTACount_Reject*6;
			
			if( InformationBufferLength < LockListSize )
			{
				*BytesNeeded = LockListSize;
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
							
			if(pMgntInfo->LockType == MAC_FILTER_ACCEPT)
			{
				*pNumOfLockedAddr = pMgntInfo->LockedSTACount;
				PlatformMoveMemory( pLockListBuffer, pMgntInfo->LockedSTAList, pMgntInfo->LockedSTACount*6  );
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_LOCKED_STA_ADDRESS: LockedSTACount=%d\n", pMgntInfo->LockedSTACount ));
			}
			else if(pMgntInfo->LockType ==MAC_FILTER_REJECT)
			{
				*pNumOfLockedAddr = pMgntInfo->LockedSTACount_Reject;
				PlatformMoveMemory( pLockListBuffer, pMgntInfo->LockedSTAList_Reject, pMgntInfo->LockedSTACount_Reject*6  );
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_LOCKED_STA_ADDRESS: LockedSTACount_Reject=%d\n", pMgntInfo->LockedSTACount_Reject));
			}
			
			*BytesWritten = LockListSize;
			bInformationCopied = TRUE;

			
//			RT_PRINT_ADDRS(COMP_OID_QUERY, DBG_LOUD, ("Locked STA Address"), LockListBuffer, pMgntInfo->LockedSTACount );
			RT_PRINT_DATA(COMP_OID_QUERY, DBG_LOUD, ("InformationBuffer"), InformationBuffer, InformationBufferLength );
		}
		break;
		
	case OID_RT_GET_DEFAULT_KEY_ID:                                             	
		{
			if( InformationBufferLength < sizeof(usInfo) )
			{
				*BytesNeeded = sizeof(usInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			usInfo = (USHORT) pMgntInfo->SecurityInfo.DefaultTransmitKeyIdx;
			pInfo = (PVOID) &usInfo;
			ulInfoLen = sizeof(usInfo);
			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_DEFAULT_KEY_ID: %d\n", usInfo));
		}
		break;

	case OID_RT_GET_BCN_INTVL:
		{
			if( InformationBufferLength < sizeof(usInfo) )
			{
				*BytesNeeded = sizeof(usInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			usInfo = pMgntInfo->dot11BeaconPeriod;
			pInfo = (PVOID) &usInfo;
			ulInfoLen = sizeof(usInfo);
			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query ID_RT_GET_BCN_INTVL: %d\n", usInfo));
		}
		break;

	case OID_RT_AP_GET_DTIM_PERIOD:
		{
			if( InformationBufferLength < sizeof(usInfo) )
			{
				*BytesNeeded = sizeof(usInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			usInfo = (USHORT)pMgntInfo->dot11DtimPeriod;
			pInfo = (PVOID) &usInfo;
			ulInfoLen = sizeof(usInfo);
			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_AP_GET_DTIM_PERIOD: %d\n", usInfo));
		}
		break;

	case OID_RT_GET_PREAMBLE_MODE:
		{
			if( InformationBufferLength < sizeof(ucInfo) )
			{
				*BytesNeeded = sizeof(ucInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			switch(pMgntInfo->dot11CurrentPreambleMode)
			{
			case PREAMBLE_LONG:
				ucInfo = 0;
				break;

			case PREAMBLE_AUTO:
				ucInfo = 1;
				break;

			case PREAMBLE_SHORT:
				ucInfo = 2;
				break;

			default:
				RT_TRACE(COMP_OID_QUERY, DBG_WARNING, ("Invalid preamble mode, dot11CurrentPreambleMode: %d...\n", pMgntInfo->dot11CurrentPreambleMode));
				ucInfo = 0;
				break;
			}

			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_PREAMBLE_MODE: %d\n", ucInfo));
		}
		break;

	case OID_RT_AP_SWITCH_INTO_AP_MODE:
		{
			if( InformationBufferLength < sizeof(ucInfo) )
			{
				*BytesNeeded = sizeof(ucInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			ucInfo = (ACTING_AS_AP(pAdapter)  == TRUE) ? 1 : 0;
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_AP_SWITCH_INTO_AP_MODE: %d\n", ucInfo));
		}
		break;
	case OID_RT_AP_GET_ASSOC_STA_COUNT:
		{
			int		i;
			UCHAR count_temp = 0;
			PRT_WLAN_STA			CurrAsocEntry =pMgntInfo->AsocEntry ;
			
			if( InformationBufferLength < sizeof(ucInfo) )
			{
				*BytesNeeded = sizeof(ucInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 
			for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
			{
				if( CurrAsocEntry[i].bUsed &&  CurrAsocEntry[i].bAssociated)
					count_temp++;
			}
			pMgntInfo->StaCount = count_temp;
			ucInfo =count_temp;
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_AP_GET_ASSOC_STA_COUNT: %d\n", ucInfo));
		}
		break;
	case OID_RT_AP_GET_CLOUD_KEY_EX:
		{
			u1Byte length_temp = 0;
			u1Byte offset = pMgntInfo->cloud_key_offset;
			length_temp = EEPROM_CLOUD_KEY_LENGTH_EX - offset; //make sure offset is not larger then 14

			if( (InformationBufferLength-1) > length_temp || (InformationBufferLength-1) ==0)
				InformationBufferLength = length_temp+1;
			
			((pu1Byte)InformationBuffer)[0] = (UCHAR)InformationBufferLength-1;
			HAL_ReadCloudKey_Ex(pAdapter,(UCHAR)InformationBufferLength-1,offset,(pu1Byte)InformationBuffer+1);

			ulInfoLen = InformationBufferLength;
			*BytesWritten = InformationBufferLength; 
			
			bInformationCopied = TRUE;
		
			RT_PRINT_DATA( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_AP_GET_CLOUD_KEY_EX: "), 
				InformationBuffer, ulInfoLen );
		}
		break;
	case OID_RT_AP_SET_BEACON_START:
		{
			PMGNT_INFO 	pDefMgntInfo = GetDefaultMgntInfo(pAdapter);
			
			if( InformationBufferLength < sizeof(ucInfo) )
			{
				*BytesNeeded = sizeof(ucInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			} 

			//2014/02/22 MH We can not read USB reg here otherwise the TX pause will be incorrect. 
			pDefMgntInfo->DelayApBeaconCnt = (u1Byte)pDefMgntInfo->Regbcndelay;

			ucInfo =TRUE;
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_AP_SET_BEACON_START: %d\n", ucInfo));
		}
		break;
	case OID_RT_GET_HARDWARE_RADIO_OFF:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_HARDWARE_RADIO_OFF:\n"));
		{
			BOOLEAN bRfOffByHw = FALSE;

			pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_RF_OFF_BY_HW, (pu1Byte)(&bRfOffByHw));
			ulInfo = bRfOffByHw;
		}
		break;

	case OID_RT_AP_WDS_MODE:
		{
			if( InformationBufferLength < sizeof(pMgntInfo->WdsMode) )
			{
				*BytesNeeded = sizeof(pMgntInfo->WdsMode);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			ucInfo = pMgntInfo->WdsMode;
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_AP_WDS_MODE: %d\n", ucInfo));
		}
		break;

	case OID_RT_AP_WDS_AP_LIST:
		{
			pu4Byte	pNumOfWdsAp = (pu4Byte)InformationBuffer;
			pu1Byte	pWdsApList = (pu1Byte)InformationBuffer + sizeof(u4Byte);
			u4Byte	ListSize = sizeof(u4Byte) + 1*6;

			if( InformationBufferLength < ListSize ) 
			{
				*BytesNeeded = ListSize;
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			// Note that, in current architecture, we only support 1 WDS AP.
			// For more than one WDS AP to support, we must implment "learning" 
			// and "802.1d spanning tree". 2006.06.12, by rcnjko.
			*pNumOfWdsAp = 1;
			PlatformMoveMemory( pWdsApList, pMgntInfo->WdsApAddr, 1*6 );
			*BytesWritten = ListSize;
			bInformationCopied = TRUE;

			RT_PRINT_DATA(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_AP_WDS_AP_LIST: "), 
				InformationBuffer, ListSize );
		}
		break;
		
	case OID_RT_GET_PSP_XLINK_STATUS:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_PSP_XLINK_STATUS:\n"));
		{
			if ( InformationBufferLength < sizeof(ulInfo) )
			{
				*BytesNeeded = sizeof(ulInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			
			ucInfo = pMgntInfo->bDefaultPSPXlinkMode; // return default settings ONLY which was set by UI or registry.
			pInfo = &ucInfo;
			ulInfoLen = sizeof(ucInfo);

		}
		break;
		
	case OID_RT_GET_WMM_ENABLE:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_WMM_ENABLE:\n"));
		{
			if ( InformationBufferLength < sizeof(ulInfo) )
			{
				*BytesNeeded = sizeof(ulInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			
			if (pMgntInfo->pStaQos->QosCapability & QOS_WMM)
			{
				ulInfo = TRUE;
			}
			else
			{
				ulInfo = FALSE;
			}
		}
		break;


	case OID_RT_GET_WMM_UAPSD_ENABLE:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_WMM_UAPSD_ENABLE:\n"));
		{
			if ( InformationBufferLength < sizeof(ulInfo) )
			{
				*BytesNeeded = sizeof(ulInfo);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			
			ulInfo = pMgntInfo->pStaQos->b4ac_Uapsd & 0x0F;
		}
		break;
	
	case OID_RT_DOT11D:
		{
			ucInfo = IS_DOT11D_ENABLE(pMgntInfo);
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_DOT11D: %d\n", ucInfo));
		}
		break;

	case OID_RT_GET_LOGV2_TYPE_LIST:
		{
			if( !MgntActQuery_DrvLogTypeList(
					pAdapter,
					InformationBufferLength,
					(PDRV_LOG_TYPE_LIST_T)InformationBuffer, 
					BytesWritten,
					BytesNeeded))
			{
				RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_LOGV2_TYPE_LIST failed!!! BytesNeeded: %d\n", *BytesNeeded));
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			ulInfoLen = *BytesWritten;
			bInformationCopied = TRUE;
		}
		break;

	case OID_RT_GET_LOGV2_ATTR_LIST:
		{
			if( !MgntActQuery_DrvLogAttrList(
					pAdapter,
					InformationBufferLength,
					(PDRV_LOG_ATTR_LIST_T)InformationBuffer, 
					BytesWritten,
					BytesNeeded))
			{
				RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_LOGV2_ATTR_LIST failed!!! BytesNeeded: %d\n", *BytesNeeded));
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			ulInfoLen = *BytesWritten;
			bInformationCopied = TRUE;
		}
		break;

	case OID_RT_GET_LOGV2_DATA_LIST:
		{
			pu4Byte pLogType;

			if( InformationBufferLength < sizeof(u4Byte) )
			{
				return NDIS_STATUS_INVALID_DATA;
			}
			pLogType = (pu4Byte)InformationBuffer;

			if( !MgntActQuery_DrvLogDataList(
					pAdapter,
					*pLogType,
					InformationBufferLength,
					(PDRV_LOG_DATA_LIST_T)InformationBuffer, 
					BytesWritten,
					BytesNeeded))
			{
				RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_GET_LOGV2_DATA_LIST failed!!! BytesNeeded: %d\n", *BytesNeeded));
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			ulInfoLen = *BytesWritten;
			bInformationCopied = TRUE;
		}
		break;


	//For turbo mode mechanism, added by Roger. 2007.02.06.
	case OID_RT_TURBOMODE:
		{
			PTURBOMODE_TYPE	pForceType = (PTURBOMODE_TYPE)pInfo;
			pForceType->charData = 0;	// Clear all
			pForceType->field.SupportTurboMode = pMgntInfo->bSupportTurboMode;
			pForceType->field.AutoTurboBy8186 = pMgntInfo->bAutoTurboBy8186;
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_TURBOMODE: %d\n", ulInfo));
		}
		break;

#if (WPS_SUPPORT == 1)
	case OID_RT_WPS_RECIEVE_PACKET:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_WPS_RECIEVE_PACKET:\n"));
		{
			PSIMPLE_CONFIG_T pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);
			RT_TRACE(COMP_WPS, DBG_LOUD, ("WPS Check Recieve Buffer:\n"));
			if( InformationBufferLength < pSimpleConfig->RecieveLength)
			{
				*BytesNeeded = pSimpleConfig->RecieveLength;
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			if(!pSimpleConfig->bRecievePacket){
				*BytesNeeded =0;
				//*BytesWritten = 0;
			}else{
				RT_PRINT_DATA(COMP_WPS, DBG_LOUD, "Driver report Info Buffer before copy:", InformationBuffer, 16);
				(ULONG)InformationBufferLength = pSimpleConfig->RecieveLength;//Set For packet length
				PlatformMoveMemory( InformationBuffer, pSimpleConfig->RecieveBuf, pSimpleConfig->RecieveLength);
				RT_PRINT_DATA(COMP_WPS, DBG_LOUD, "Driver report Info Buffer After copy:", InformationBuffer, 16);
				*BytesWritten = pSimpleConfig->RecieveLength;
				pSimpleConfig->bRecievePacket = FALSE;
				RT_PRINT_DATA(COMP_WPS, DBG_LOUD, "Driver report EAP Packet Content:", pSimpleConfig->RecieveBuf, pSimpleConfig->RecieveLength);
			}
			return NDIS_STATUS_SUCCESS;
		}
		break;
#endif

	//added by vivi, adviced by xinpin, 20080424
	case OID_RT_WPS_CUSTOMIZED_LED:
		ulInfo = 1;
		break;

	case	OID_RT_WPS_HWGET_PBC_PRESSED:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_WPS_HWGET_PBC_PRESSED\n"));
		{
			*BytesNeeded = sizeof(BOOLEAN);
			if(InformationBufferLength < *BytesNeeded)
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			}
			ulInfo = (ULONG)pMgntInfo->bPbcPressed;

			if(pMgntInfo->bPbcPressed)
			{
				// 2011/11/03 MH Add for netgear special requirement. We need to count PBC press time.
				if (pMgntInfo->bRegTimerGPIO != TRUE)
					pMgntInfo->bPbcPressed = FALSE;
			}
		}
		break;

	case OID_RT_SET_WMM_UAPSD_ENABLE:
		{
			if( InformationBufferLength < sizeof(AC_UAPSD) )
			{
				*BytesNeeded = sizeof(AC_UAPSD);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			ucInfo = pMgntInfo->pStaQos->b4ac_Uapsd & 0xF;
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_SET_WMM_UAPSD_ENABLE: %X\n", ucInfo));
		}
		break;

	case OID_RT_GET_ROAM_COUNT:
		{
			
			pMgntInfo->RoamingCount=256;
			usInfo=pMgntInfo->RoamingCount;
			pInfo=(PVOID )&usInfo;
			ulInfoLen=sizeof(usInfo);
			
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Query OID_RT_GET_ROAM_COUNT: \n"));
			break;
		}
	case OID_RT_GET_DISASSOC_COUNT:
		{
			pMgntInfo->DisconnectCount=65500;
			usInfo=pMgntInfo->DisconnectCount;
			pInfo=(PVOID )&usInfo;
			ulInfoLen=sizeof(usInfo);	

			RT_TRACE(COMP_POWER, DBG_LOUD, ("Query OID_RT_GET_DISASSOC_COUNT: \n"));
			break;
		}	

	case OID_RT_HOST_SUSPEND_STATUS:
		pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_FPGA_SUSPEND_STATUS, (pu1Byte)(&ulInfo));		
		break;

	case OID_RT_AP_GET_VWIFI_STATUS:
		{
			*BytesNeeded = sizeof(BOOLEAN);
			ulInfo = (MgntActQuery_ApType(GetFirstExtAdapter(pAdapter)) == RT_AP_TYPE_VWIFI_AP)?TRUE:FALSE ;
			break;
		}

        case OID_RT_ANTENNA_DETECTED_INFO:
		{	
			#if(defined(CONFIG_ANT_DETECTION))
			if(InformationBufferLength < sizeof(ANT_DETECTED_INFO))
			{
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				*BytesNeeded = sizeof(ANT_DETECTED_INFO);
				*BytesWritten = 0;
				return Status;
			}
			
			PlatformZeroMemory(InformationBuffer, sizeof(ANT_DETECTED_INFO));

			// Retrieve previous antenna detection information in initialization process, added by Roger, 2012.11.27.
			pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_ANTENNA_DETECTED_INFO , (pu1Byte)InformationBuffer);
			
			ulInfoLen = sizeof(ANT_DETECTED_INFO);
			*BytesWritten = ulInfoLen; 
			bInformationCopied = TRUE;
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_ANTENNA_DETECTED_INFO: \n"));			
			#endif
			break;
		}

	case OID_RT_LC_STOP_SCAN:
		{
			if(InformationBufferLength < sizeof(ULONG))
			{
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				*BytesNeeded = sizeof(ULONG);
				*BytesWritten = 0;
				return Status;
			}
			ulInfo = (u4Byte)pMgntInfo->bStopScan;
		}
		break;
		
	case OID_RT_FORCED_DATA_RATE:
		{
			*BytesNeeded = sizeof(u1Byte);
			ucInfo = (u1Byte)pMgntInfo->ForcedDataRate;
			pInfo = (PVOID) &ucInfo;
			ulInfoLen = sizeof(ucInfo);
			break;
		}

	case OID_RT_WPS_INFORMATION:
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OID_RT_WPS_INFORMATION\n"));
#if ( WPS_SUPPORT == 1 )
			if((ulInfoLen = MgntActQuery_WPS_Information(pAdapter, InformationBuffer, InformationBufferLength)) == 0)
#endif
			{
				Status = NDIS_STATUS_NOT_SUPPORTED;
				return Status;					
			}
			*BytesWritten = ulInfoLen;
			bInformationCopied = TRUE;
		}
		break;

	case OID_RT_GET_CUSTOMIZE_BSS_LIST:
		{
			PRT_802_11_BSSID_LIST_Customized pCusBSSIDList = (PRT_802_11_BSSID_LIST_Customized)InformationBuffer;
			pCusBSSIDList->NumberOfItems = (pMgntInfo->NumBssDesc4Query<=1024) ? pMgntInfo->NumBssDesc4Query: 1024;
			*BytesWritten = 4;
			for(i=0;i<pCusBSSIDList->NumberOfItems;i++)
			{
				
				NdisMoveMemory(pCusBSSIDList->bssidentry[i].SsIdBuf,pMgntInfo->bssDesc4Query[i].bdSsIdBuf,pMgntInfo->bssDesc4Query[i].bdSsIdLen);
				pCusBSSIDList->bssidentry[i].SsIdLen = pMgntInfo->bssDesc4Query[i].bdSsIdLen;
				NdisMoveMemory(pCusBSSIDList->bssidentry[i].BssIdBuf,pMgntInfo->bssDesc4Query[i].bdBssIdBuf,6);
				pCusBSSIDList->bssidentry[i].RecvSignalPower = pMgntInfo->bssDesc4Query[i].RecvSignalPower;
				pCusBSSIDList->bssidentry[i].BssPacketType = pMgntInfo->bssDesc4Query[i].BssPacketType;
				pCusBSSIDList->bssidentry[i].ChannelNumber =  pMgntInfo->bssDesc4Query[i].ChannelNumber;
				pCusBSSIDList->bssidentry[i].TimeStamp = pMgntInfo->bssDesc4Query[i].bdTstamp;
				pCusBSSIDList->bssidentry[i].Noise = pMgntInfo->bssDesc4Query[i].Noise;
				*BytesWritten += sizeof(RT_WLAN_BSS_Customized);
			}
			bInformationCopied = TRUE;
		}
		break;

	}

	if(Status == NDIS_STATUS_SUCCESS)
	{        
		if(ulInfoLen <= InformationBufferLength)
		{
			// Copy result into InformationBuffer
			if(ulInfoLen && !bInformationCopied)
			{
				*BytesWritten = ulInfoLen;
				NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
			}
		}
		else if(!bInformationCopied)
		{
			// Buffer too short
			*BytesNeeded = ulInfoLen;
			Status = NDIS_STATUS_BUFFER_TOO_SHORT;
		}
	}

	return Status;
}

static NDIS_STATUS
N6CQueryInformationHandleCustomizedOriginalMPQueryOid(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;
	PADAPTER	Adapter = (PADAPTER) MiniportAdapterContext;

	USHORT					usInfo = 0;
	ULONG					ulInfo = 0;
	long						lInfo = 0;
	ULONGLONG				ul64Info = 0;

	PVOID					pInfo = (PVOID) &ulInfo;
	ULONG					ulInfoLen = sizeof(ulInfo);
	ULONG					ulBytesAvailable = ulInfoLen;
	BOOLEAN					bInformationCopied = FALSE;
	
	FunctionIn(COMP_OID_QUERY);
	
	*BytesWritten = 0;
	*BytesNeeded = 0;

	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;

	case OID_RT_PRO_GET_PHY_PARAM_VERSION:
	case OID_RT_DEVICE_ID_INFO:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_DEVICE_ID_INFO: \n"));	
		{
			PRT_SDIO_ID_INFO	pSdioIdInfo = (PRT_SDIO_ID_INFO)InformationBuffer;

			if(InformationBufferLength < sizeof(RT_SDIO_ID_INFO))
			{
				*BytesNeeded = sizeof(RT_SDIO_ID_INFO);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			// Fill up member of RT_DEVICE_ID_HEADER.
			pSdioIdInfo->DevIDHeader.RtWlanDevTag = RT_DEVICE_ID_INFO_TAG;
			
			pSdioIdInfo->DevIDHeader.ChipID	= GET_CHIP_ID(Adapter);
			pSdioIdInfo->DevIDHeader.ChipVer = GET_CHIP_VERSION(Adapter);
			
			pSdioIdInfo->DevIDHeader.BusType = RT_DEVICE_ID_SDIO;

			RT_TRACE(COMP_INIT, DBG_LOUD, ("chip id 0x%x chipver 0x%x\n", pSdioIdInfo->DevIDHeader.ChipID, pSdioIdInfo->DevIDHeader.ChipVer));

			//
			// <Roger_TODO> We should fill up the member of RT_SDIO_ID_INFO.
			// 2011.01.12.
			//
			pSdioIdInfo->VID = 0x024c;
			pSdioIdInfo->PID = 0x8723;
			pSdioIdInfo->RevID = 0;
			pSdioIdInfo->InterfaceIdx = 0;	

			*BytesWritten = sizeof(RT_SDIO_ID_INFO);
			bInformationCopied = TRUE;

			RT_TRACE(COMP_MP,DBG_LOUD,("==>OID_RT_DEVICE_ID_INFO, (ChipID, ChipVer) = (0x%X, 0x%X)\n", 
                        pSdioIdInfo->DevIDHeader.ChipID, pSdioIdInfo->DevIDHeader.ChipVer));  
			RT_TRACE(COMP_MP, DBG_LOUD, ("===> OID_RT_DEVICE_ID_INFO: Adapter->HardwareType = %d\n", Adapter->HardwareType));
		}
		break;

	case OID_RT_PRO8187_WI_POLL:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_PRO8187_WI_POLL: \n"));	
		{
			PRT8187DBGWIPARAM pWIParam = (PRT8187DBGWIPARAM)InformationBuffer;
			PRT_NDIS_DBG_CONTEXT pDbgCtx = &(Adapter->ndisDbgCtx);
			PMGNT_INFO      	pMgntInfo = &(Adapter->MgntInfo);

			// Redirect to proper adapter to handle workitem done polling. added by Roger, 2009.10.07.
			if(pMgntInfo->NdisVersion >= RT_NDIS_VERSION_6_20)
			{
				pDbgCtx = &(GetDefaultAdapter(Adapter)->ndisDbgCtx);
			}
	
			if(InformationBufferLength < sizeof(ULONG)*4)
			{
				*BytesNeeded = sizeof(ULONG)*4;
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
	
			if(pDbgCtx->bDbgWorkItemInProgress)
			{ // DbgWorkItem is in progress.
				pWIParam->bDbgActCompleted = FALSE;
				pWIParam->IoValue = 0;
			}
			else
			{ // DbgWorkItem is completed.
				pWIParam->bDbgActCompleted = TRUE;
				pWIParam->IoValue = pDbgCtx->DbgIoValue;
				
				pWIParam->outLen = pDbgCtx->DbgOutLen;
				if((InformationBufferLength>=sizeof(RT8187DBGWIPARAM)) && (pWIParam->outLen>0))
				{
					PlatformMoveMemory(&pWIParam->outBuf[0], &pDbgCtx->DbgOutBuf[0],pWIParam->outLen);
					RT_DISP_DATA(FBT, BT_DBG_CONTENT, ("[BTDBG], DLL output for RT_PRO8187_WI_POLL: \n"), &pWIParam->outBuf[0], pWIParam->outLen);
				}
			}
			pWIParam->eDbgActType = pDbgCtx->DbgActType;
			pWIParam->IoOffset = pDbgCtx->DbgIoOffset;
			*BytesWritten = sizeof(RT8187DBGWIPARAM);
			bInformationCopied = TRUE;
		}
		break;

	case OID_RT_PRO_GET_CUT_VERSION:
		{
			ulInfo = HAL_GetCutVersion(Adapter);
			ulInfoLen = 4;
		}
		break;		


	case OID_RT_PRO_GET_EFUSE_UTILIZE:	
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_PRO_GET_EFUSE_UTILIZE\n"));
		Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_EFUSE_USAGE, (pu1Byte)&ulInfo);		
		break;

	case OID_RT_PRO_CHK_AUTOLOAD:	
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_PRO_CHK_AUTOLOAD\n"));
		Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_AUTOLOAD_STATUS, (pu1Byte)&ulInfo );		
		break;

	case OID_RT_PRO_READ_MAC_ADDRESS:
		// Modified from PermanentAddress to CurrentAddress.
		pInfo = Adapter->CurrentAddress;
		// Modified by Bruce, 2007-1-19.
		ulBytesAvailable = ulInfoLen = ETH_LENGTH_OF_ADDRESS;
		break;

	// Added by Bruce, 2007-1-31.
	case OID_RT_PRO_READ_REGISTRY:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_PRO_READ_REGISTRY.\n"));

		Status = OIDQ_RTKReadReg(
					Adapter, 
					InformationBuffer, 
					InformationBufferLength, 
					BytesWritten, 
					BytesNeeded,
					&ulInfo);

		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_PRO_READ_REGISTRY.\n"));
		if(Status != NDIS_STATUS_SUCCESS)
			return Status;
		break;

	case OID_RT_PRO_READ_REGISTRY_SIC:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_PRO_READ_REGISTRY_SIC.\n"));

		Status = OIDQ_RTKReadRegSIC(
					Adapter, 
					InformationBuffer, 
					InformationBufferLength, 
					BytesWritten, 
					BytesNeeded,
					&ulInfo);

		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_PRO_READ_REGISTRY_SIC.\n"));
		if(Status != NDIS_STATUS_SUCCESS)
			return Status;
		break;

	case OID_RT_PRO_READ_BB_REG:
		// 
		// Ported from 85B MP, by Bruce, 2007-1-19.
		//
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_PRO_READ_BB_REG.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulBeOFDM;
			ULONG	ulRegValue;
			u1Byte	uData;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*3)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*3;
				return Status;
			}
			// Get offset, and type of BB register to read.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulBeOFDM = ((*((ULONG*)InformationBuffer+1)) == TRUE)?0:1;
			pInfo = ((ULONG*)InformationBuffer)+2;
			
			ulRegValue = 0x00000000 | (ulBeOFDM << 24); // OFDM or CCK read.
			ulRegValue |= (ulRegOffset & 0x7f); // Offset.
			PlatformEFIOWrite4Byte(Adapter, BBAddr, ulRegValue);			
			uData = PlatformEFIORead1Byte(Adapter, PhyDataR);
			*((ULONG*)pInfo) = uData;
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_PRO_READ_BB_REG.\n"));
		break;

	//
	// <Roger_Notes> To backward compatible earlier DLL exported function.
	// Revised by Roger, 2008.11.10.
	//
	case OID_RT_PRO_QUERY_EEPROM_TYPE:
		switch(Adapter->EepromAddressSize)
		{
			case 4: 
				ulInfo = 3; // For E-Fuse. added by Roger, 2008.11.10.
				break;
			case 6:
				ulInfo = 1; // 93C46
				break;
			case 8:
				ulInfo = 2; //93C56
				break;
			default:
				ulInfo = 1;
				break;
		}
		break;

	// Added by Bruce, 2007-03-01.
	case OID_RT_PRO_READ_EEPROM:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_PRO_READ_EEPROM.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*2;				
				return Status;
			}
			// Get offset and data width.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			
			switch(ulRegDataWidth)
			{
			case 1:			
				ulInfo = ReadEEprom(Adapter, (u2Byte)(ulRegOffset >> 1));
				if(ulRegOffset % 2 == 1)
					ulInfo >>= 8;
				ulInfo &= 0xFF;
				break;
				
			case 2:
				ulInfo = ReadEEprom(Adapter, (u2Byte)(ulRegOffset >> 1));
				ulInfo &= 0xFFFF;
				break;

			default:
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_PRO_READ_EEPROM.\n"));
		break;
		
	case OID_RT_PRO_READ_EFUSE:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_PRO_READ_EFUSE.\n"));
		{
			ULONG	ulRegOffset;
			u2Byte	value;
			ULONG	ulRegDataWidth;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*2;				
				return Status;
			}
			// Get offset and data width.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);

			// 2008/11/12 MH We only read efuse shadow modify map!!!
			// We only support 1,2 and 4byte data lengths.
			switch(ulRegDataWidth)
			{
			case 1:
				EFUSE_MaskedShadowRead(Adapter, 1, (u2Byte)(ulRegOffset), (UINT32 *)&value);
				ulInfo = (u1Byte)value;
				break;
				
			case 2:
				EFUSE_MaskedShadowRead(Adapter, 2, (u2Byte)(ulRegOffset), (UINT32 *)&value);
				ulInfo = (u2Byte)value;
				break;

			case 4:
				EFUSE_MaskedShadowRead(Adapter, 4, (u2Byte)(ulRegOffset), (UINT32 *)&value);
				ulInfo = (u4Byte)value;
				break;

			default:
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_PRO_READ_EFUSE.\n"));
		break;

	case OID_RT_PRO_READ_EFUSE_BT:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_PRO_READ_EFUSE_BT.\n"));
		{
			ULONG	ulRegOffset;
			u2Byte	value;
			ULONG	ulRegDataWidth;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(ULONG)*2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG)*2;				
				return Status;
			}
			// Get offset and data width.
			ulRegOffset = *((ULONG*)InformationBuffer);
			ulRegDataWidth = *((ULONG*)InformationBuffer+1);

			// 2008/11/12 MH We only read efuse shadow modify map!!!
			// We only support 1,2 and 4byte data lengths.
			switch(ulRegDataWidth)
			{
			case 1:
				EFUSE_ShadowReadBT(Adapter, 1, (u2Byte)(ulRegOffset), (UINT32 *)&value);
				ulInfo = (u1Byte)value;
				break;
				
			case 2:
				EFUSE_ShadowReadBT(Adapter, 2, (u2Byte)(ulRegOffset), (UINT32 *)&value);
				ulInfo = (u2Byte)value;
				break;

			case 4:
				EFUSE_ShadowReadBT(Adapter, 4, (u2Byte)(ulRegOffset), (UINT32 *)&value);
				ulInfo = (u4Byte)value;
				break;

			default:
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<=== Query OID_RT_PRO_READ_EFUSE_BT.\n"));
		break;

	case	OID_RT_PRO_GET_EFUSE_UTILIZE_BT:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_PRO_GET_EFUSE_UTILIZE_BT\n"));
		break;

	// Added by Bruce, 2007-03-06.
	case OID_RT_PRO_RF_READ_REGISTRY:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("===> Query OID_RT_PRO_RF_READ_REGISTRY.\n"));
		{
			ULONG	ulRegOffset;
			ULONG	ulRegDataWidth;
			ULONG	RF_PATH;	
			
			//
			// Add in the future. Since we cannot call HAL related function in \PLATFORM
			//
			// Get offset and data width.
			ulRegOffset = *((ULONG*)InformationBuffer);
 			ulRegDataWidth = *((ULONG*)InformationBuffer+1);
			RF_PATH = (ulRegDataWidth >> 4);
			ulRegOffset = ulRegOffset & bRFRegOffsetMask;
			// Read RF register synchronously.

			// Use this temporarily
			ulInfo = PHY_QueryRFReg(Adapter, (u1Byte)RF_PATH, ulRegOffset, bRFRegOffsetMask);
		}
		RT_TRACE(COMP_MP, DBG_LOUD, ("<=== Query OID_RT_PRO_RF_READ_REGISTRY.\n"));
		break;
		
	case OID_RT_INACTIVE_PS:
		{
			//
			// Get current IPS status, by Bruce, 2007-12-12.
			// The IPS actions includes two flags "pMgntInfo->bInactivePs" and "pMgntInfo->bIPSModeBackup",
			// and the returned status by this OID is copied from "pMgntInfo->bInactivePs".
			// The status returned "FALSE" does not indicate the driver never enters IPS.
			//

			// Verify size
			if(InformationBufferLength < sizeof(BOOLEAN))
			{
				*BytesNeeded = sizeof(BOOLEAN);
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}
			// Get IPS state.
			ulInfo = (ULONG)(Adapter->MgntInfo.PowerSaveControl.bInactivePs);

			RT_TRACE(COMP_POWER, DBG_LOUD, ("Query OID_RT_INACTIVE_PS: %u\n", ulInfo));
		}
		break;
		
	case OID_RT_CUSTOMER_ID_INFO:
		{
			if( InformationBufferLength < sizeof(RT_CUSTOMER_ID) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(RT_CUSTOMER_ID);
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_RT_CUSTOMER_ID_INFO: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;
			}
			ulInfo = Adapter->MgntInfo.CustomerID;
		}
	break;

	}


	if(Status == NDIS_STATUS_SUCCESS)
	{        
		if(ulInfoLen <= InformationBufferLength)
		{
			// Copy result into InformationBuffer
			if(ulInfoLen && !bInformationCopied)
			{
				*BytesWritten = ulInfoLen;
				PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
			}
		}
		else if(!bInformationCopied)
		{
			// Buffer too short
			*BytesNeeded = ulInfoLen;
			Status = NDIS_STATUS_BUFFER_TOO_SHORT;
		}
	}

	return(Status);
}


//
//	Description:
//		Handler for querying OIDs whcih are common to PCI and USB devices.
//
NDIS_STATUS
N6CQueryInformation(
	IN	PADAPTER		pAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
	)
{
	PMGNT_INFO      		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	UCHAR					ucInfo = 0;
	USHORT					usInfo = 0;
	ULONG					ulInfo = 0;
	LONG				lInfo = 0;
	ULONGLONG				ul64Info = 0;

	PVOID					pInfo = (PVOID) &ulInfo;
	ULONG					ulInfoLen = sizeof(ulInfo);
	BOOLEAN					bInformationCopied = FALSE;

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("====> N6CQueryInformation, OID=0x%08x\n", Oid ));


	// Moved From Orginal MPQueryInformation Query OID ----------------------------------------------------------------------------------------------
	Status = N6CQueryInformationHandleCustomizedOriginalMPQueryOid(
 			pAdapter, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesWritten, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------


	// CCX Handle Query OID ----------------------------------------------------------------------------------------------
	Status = NDIS_STATUS_NOT_RECOGNIZED;

	if (Status != NDIS_STATUS_NOT_RECOGNIZED)
	{
		//
		// This Oid has been responded to by the MpEventQueryInformation function
		// We will not handle this Oid and return to the OS.
		//
		RT_TRACE(COMP_CCX, DBG_TRACE, ("N6CQueryInformation intercepted by MpEventQueryInformation! Status 0x%08x\n", Status));
		return	Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// -----------------------------------------------------------------------------------------------------------------


	// Handle the Customized Wapi Oids --------------------------------------------------------------------------
	Status = WAPI_OidHandler(
			pAdapter, 
			1, //N6 Platform
			0, //Query OID
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesWritten, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------



	// Handle the Customized WifiDirect Oids --------------------------------------------------------------------------
	Status = N6CQueryInformationHandleCustomizedWifiDirectOids(
			pAdapter, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesWritten, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------



	// Handle the Customized 11n Oids --------------------------------------------------------------------------
	Status = N6CQueryInformationHandleCustomized11nOids(
			pAdapter, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesWritten, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------

	

	// Handle the Customized Security Oids --------------------------------------------------------------------
	Status = N6CQueryInformationHandleCustomizedSecurityOids(
			pAdapter, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesWritten, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------



	
	// Handle the Customized Oids --------------------------------------------------------------------------
	Status = N6CQueryInformationHandleCustomizedOids(
			pAdapter, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesWritten, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------



	// ============================================================================================
	// The following are the OIDs from the NDIS 6.x, including the Customized OIDs with the same meanings. Please be organized. 
	// ============================================================================================


	*BytesWritten = 0;
	*BytesNeeded = 0;

	switch(Oid)
	{
	case OID_GEN_HARDWARE_STATUS:
		ulInfo = NdisHardwareStatusReady;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD,("Query Information, OID_GEN_HARDWARE_STATUS: 0x%08X\n", ulInfo));
		break;

	case OID_GEN_MEDIA_SUPPORTED:
	case OID_GEN_MEDIA_IN_USE:
		ulInfo = NATIVE_802_11_MEDIA_TYPE;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_MEDIA_SUPPORTED or OID_GEN_MEDIA_IN_USE: %08x\n", ulInfo));
		break;

	case OID_GEN_PHYSICAL_MEDIUM:
		ulInfo = NATIVE_802_11_PHYSICAL_MEDIA_TYPE;

		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_PHYSICAL_MEDIUM:%d(NdisPhysicalMediumWirelessLan)\n", ulInfo));
		break;

	case OID_GEN_MEDIA_CAPABILITIES:
		ulInfo = NDIS_MEDIA_CAP_TRANSMIT | NDIS_MEDIA_CAP_RECEIVE;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_MEDIA_CAPABILITIES: %d\n",ulInfo));        	
		break;

	case OID_GEN_VENDOR_ID:
		PlatformMoveMemory(&ulInfo, pAdapter->PermanentAddress, 3);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_VENDOR_ID:%02x:%02x:%02x\n", pAdapter->PermanentAddress[0], pAdapter->PermanentAddress[1], pAdapter->PermanentAddress[2]));
		break;

	case OID_GEN_VENDOR_DESCRIPTION:
		pInfo = (PVOID)NicIFGetVenderDescription(pAdapter);
		ulInfoLen = strlen((const char*)NicIFGetVenderDescription(pAdapter))+1;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_VENDOR_DESCRIPTION:%s\n",(char *)pInfo));
		break;

	case OID_GEN_DRIVER_VERSION:
		usInfo = (USHORT) NIC_DRIVER_VERSION60;
		pInfo = (PVOID) &usInfo;
		ulInfoLen = sizeof(USHORT);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_DRIVER_VERSION:%08x\n",usInfo));            
		break;

	case OID_GEN_CURRENT_LOOKAHEAD:
	case OID_GEN_MAXIMUM_LOOKAHEAD:
		if(pNdisCommon->ulLookAhead == 0)
			pNdisCommon->ulLookAhead = pNdisCommon->MaxPktSize - NIC_HEADER_SIZE;
		ulInfo = pNdisCommon->ulLookAhead;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_CURRENT_LOOKAHEAD or OID_GEN_MAXIMUM_LOOKAHEAD: %d\n",ulInfo));            
		break;      

	case OID_GEN_MAXIMUM_FRAME_SIZE:
		ulInfo = pNdisCommon->MaxPktSize - NIC_HEADER_SIZE; 
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_MAXIMUM_FRAME_SIZE:%d\n",ulInfo));
		break;

	case OID_GEN_MAXIMUM_TOTAL_SIZE:
		ulInfo = pNdisCommon->MaxPktSize; 
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_MAXIMUM_TOTAL_SIZE:%d\n",ulInfo));
		break;

	case OID_GEN_TRANSMIT_BLOCK_SIZE:
	case OID_GEN_RECEIVE_BLOCK_SIZE:
		ulInfo = pNdisCommon->MaxPktSize;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_TRANSMIT_BLOCK_SIZE or OID_GEN_RECEIVE_BLOCK_SIZE:%d\n",ulInfo));
		break;

	case OID_GEN_TRANSMIT_BUFFER_SPACE:
		// TODO: We should change it
		// 2004.06.03, by rcnjko.
		ulInfo = pNdisCommon->MaxPktSize * 10 * 2;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_TRANSMIT_BUFFER_SPACE:%d\n",ulInfo));
		break;

	case OID_GEN_RECEIVE_BUFFER_SPACE:
		// TODO: We should change it
		// 2004.06.03, by rcnjko.
		ulInfo = pNdisCommon->MaxPktSize * 10;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_RECEIVE_BUFFER_SPACE:%d\n",ulInfo));
		break;

	case OID_GEN_MAC_OPTIONS:
		ulInfo =	NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
// 20100309 Joseph: Modified for software "Chinasec" BSOD issue.
// If we do not indicate NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA flag, Chinasec BSOD in higher layer 
//when sending Tx buffer to do NDIS miniport driver after connection.
//#if COALESCE_RECEIVED_PACKET
					NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | 
//#endif
					NDIS_MAC_OPTION_NO_LOOPBACK;

		// Qos support. 2007.01.05, by shien chang.
		if (pMgntInfo->pStaQos->QosCapability > QOS_DISABLE)
		{
			ulInfo |= NDIS_MAC_OPTION_8021P_PRIORITY;
		}

		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_MAC_OPTIONS:%08x\n", ulInfo));                    
		break;

	case OID_GEN_LINK_SPEED:		
		ulInfo = (MgntActQuery_RT_11N_USER_SHOW_RATES(pAdapter , pMgntInfo->bForcedShowRxRate, FALSE)/2)*10000;

		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_LINK_SPEED:%d Mbps\n",ulInfo/10000));
		break;

	case OID_GEN_MEDIA_CONNECT_STATUS: 
       	 //
       	 // We will always report connected. The nwifi IM driver
       	 // takes care of the logic of this for the miniport.
       	 //
		//ulInfo = NdisMediaStateConnected;
		if( pMgntInfo->bMediaConnect )
		{
			ulInfo = NdisMediaStateConnected;
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_MEDIA_CONNECT_STATUS: NdisMediaStateConnected\n"));
		}
		else
		{
			ulInfo = NdisMediaStateDisconnected;
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_MEDIA_CONNECT_STATUS: NdisMediaStateDisconnected\n"));
		}
		
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_MEDIA_CONNECT_STATUS: NdisMediaStateConnected\n"));
		break;

	case OID_GEN_XMIT_OK:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->TxStats.NumTxOkTotal;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->TxStats.NumTxOkTotal;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_GEN_XMIT_ERROR:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->TxStats.NumTxErrTotal;
			ulInfoLen = sizeof(ul64Info);
			pInfo = &ul64Info;
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->TxStats.NumTxErrTotal;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_GEN_RCV_OK:
		if(InformationBufferLength >= 8)
		{
			ul64Info = pAdapter->RxStats.NumRxOkTotal;
			pInfo = &ul64Info;
			ulInfoLen = sizeof(ul64Info);
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)pAdapter->RxStats.NumRxOkTotal;
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_GEN_RCV_ERROR:
		if(InformationBufferLength >= 8)
		{
			ul64Info = (pAdapter->RxStats.NumRxErrTotalUnicast + pAdapter->RxStats.NumRxErrTotalMulticast);
			pInfo = &ul64Info;
			ulInfoLen = sizeof(ul64Info);
		}
		else if(InformationBufferLength == 4)
		{
			ulInfo = (u4Byte)(pAdapter->RxStats.NumRxErrTotalUnicast + pAdapter->RxStats.NumRxErrTotalMulticast);
		}
		else
		{
			ulInfoLen = sizeof(ul64Info);
		}
		break;

	case OID_GEN_RCV_NO_BUFFER:
		ulInfoLen = sizeof(ul64Info);
		pInfo = &ul64Info;
		// TODO: Set counter to a valid value
		break;

	case OID_FFP_SUPPORT:
		Status = NDIS_STATUS_NOT_SUPPORTED;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_FFP_SUPPORT: (not supported)\n"));
		break;
	
	case OID_GEN_SUPPORTED_GUIDS:
		Status = NDIS_STATUS_NOT_SUPPORTED;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_SUPPORTED_GUIDS: (not supported)\n"));
		break;

	case OID_GEN_MAXIMUM_SEND_PACKETS:
		ulInfo = NIC_MAX_SEND_PACKETS;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_GEN_MAXIMUM_SEND_PACKETS:%d\n",ulInfo));                        
		break;

	case OID_802_3_PERMANENT_ADDRESS:
		pInfo = pAdapter->PermanentAddress;
		ulInfoLen = ETH_LENGTH_OF_ADDRESS;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_802_3_PERMANENT_ADDRESS:%02x:%02x:%02x:%02x:%02x:%02x\n", pAdapter->PermanentAddress[0], pAdapter->PermanentAddress[1], pAdapter->PermanentAddress[2], pAdapter->PermanentAddress[3], pAdapter->PermanentAddress[4], pAdapter->PermanentAddress[5]));
		break;

	case OID_802_3_CURRENT_ADDRESS:
		pInfo = pAdapter->CurrentAddress;		
		ulInfoLen = ETH_LENGTH_OF_ADDRESS;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_802_3_CURRENT_ADDRESS:%02x:%02x:%02x:%02x:%02x:%02x\n", pAdapter->CurrentAddress[0], pAdapter->CurrentAddress[1], pAdapter->CurrentAddress[2], pAdapter->CurrentAddress[3], pAdapter->CurrentAddress[4], pAdapter->CurrentAddress[5]));            
		break;

	case OID_802_3_MAXIMUM_LIST_SIZE:
		ulInfo = MAX_MCAST_LIST_NUM;
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query Information, OID_802_3_MAXIMUM_LIST_SIZE:%d\n",ulInfo));                        
		break;

	case OID_802_3_MULTICAST_LIST:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_802_3_MULTICAST_LIST:\n"));
		{
			u4Byte MCListSize = pAdapter->MCAddrCount*6;
			if( InformationBufferLength < MCListSize )
			{
				*BytesNeeded = MCListSize;
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			PlatformMoveMemory( InformationBuffer, pAdapter->MCList, MCListSize );
			*BytesWritten = MCListSize;
			bInformationCopied = TRUE;
		}
		break;
		
	// 2006.10.26, by shien chang.
	case OID_DOT11_OPERATION_MODE_CAPABILITY:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_OPERATION_MODE_CAPABILITY:\n"));
		{
			return N6CQuery_DOT11_OPERATION_MODE_CAPABILITY(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesWritten,
					BytesNeeded);
		}
		break;

	// 2006.10.26, by shien chang.
	case OID_DOT11_CURRENT_OPERATION_MODE:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_CURRENT_OPERATION_MODE:\n"));
		{
			return N6CQuery_DOT11_CURRENT_OPERATION_MODE(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesWritten,
					BytesNeeded);
		}
		break;

	case OID_DOT11_STATISTICS:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_STATISTICS:\n"));
		{		
			return N6CQuery_DOT11_STATISTICS(
				pAdapter,
				InformationBuffer,
				InformationBufferLength,
				BytesWritten,
				BytesNeeded);
		}
		break;

	case OID_DOT11_DESIRED_BSS_TYPE:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_DESIRED_BSS_TYPE:\n"));
		{
			PDOT11_BSS_TYPE pBssType = (PDOT11_BSS_TYPE)InformationBuffer;
			u4Byte len = sizeof(DOT11_BSS_TYPE);
			if ( InformationBufferLength < len )
			{
				*BytesNeeded = len;
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_TOO_SHORT;			
			}

			Status = N6CQuery_DOT11_DESIRED_BSS_TYPE(pAdapter, pBssType);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = len;
				bInformationCopied = TRUE;
			}
		}
		break;
		
	case OID_DOT11_MEDIA_STREAMING_ENABLED:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_MEDIA_STREAMING_ENABLED:\n"));
		{
			if ( InformationBufferLength < sizeof(BOOLEAN) )
			{
				*BytesWritten = 0;
				*BytesNeeded = sizeof(BOOLEAN);
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			ulInfo = pNdisCommon->bDot11MediaStreamingEnabled;
			ulInfoLen = sizeof(BOOLEAN);
		}
		break;

	case OID_DOT11_EXCLUDE_UNENCRYPTED:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_EXCLUDE_UNENCRYPTED:\n"));
		{
			if ( InformationBufferLength < sizeof(BOOLEAN) )
			{
				*BytesWritten = 0;
				*BytesNeeded = sizeof(BOOLEAN);
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			ulInfo = pMgntInfo->bExcludeUnencrypted;
			ulInfoLen = sizeof(BOOLEAN);
		}
		break;

	case OID_DOT11_PRIVACY_EXEMPTION_LIST:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_PRIVACY_EXEMPTION_LIST:\n"));
		{
			return N6CQuery_DOT11_PRIVACY_EXEMPTION_LIST(
				pAdapter,
				InformationBuffer,
				InformationBufferLength,
				BytesWritten,
				BytesNeeded);
		}
		break;
		
	case OID_DOT11_IBSS_PARAMS:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_IBSS_PARAMS:\n"));
		{
			return N6CQuery_DOT11_IBSS_PARAMS(
				pAdapter,
				InformationBuffer,
				InformationBufferLength,
				BytesWritten,
				BytesNeeded);
		}
		break;
		
	case OID_DOT11_PERMANENT_ADDRESS:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_PERMANENT_ADDRESS:\n"));
		{
			if ( InformationBufferLength < ETH_LENGTH_OF_ADDRESS )
			{
				*BytesWritten = 0;
				*BytesNeeded = ETH_LENGTH_OF_ADDRESS;
				return NDIS_STATUS_INVALID_LENGTH;
			}

			PlatformMoveMemory(
				InformationBuffer,
				pAdapter->PermanentAddress,
				ETH_LENGTH_OF_ADDRESS);
			*BytesWritten = ETH_LENGTH_OF_ADDRESS;
			bInformationCopied = TRUE;
		}
		break;
		
	case OID_DOT11_CURRENT_ADDRESS:
	case OID_DOT11_MAC_ADDRESS:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_CURRENT_ADDRESS or OID_DOT11_MAC_ADDRESS:\n"));
		{
			if ( InformationBufferLength < ETH_LENGTH_OF_ADDRESS )
			{
				*BytesWritten = 0;
				*BytesNeeded = ETH_LENGTH_OF_ADDRESS;
				RT_TRACE(COMP_OID_QUERY, DBG_WARNING, ("Query OID_DOT11_CURRENT_ADDRESS or OID_DOT11_MAC_ADDRESS: Invalid length (%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return NDIS_STATUS_INVALID_LENGTH;
			}

			PlatformMoveMemory(
				InformationBuffer,
				pAdapter->CurrentAddress,
				ETH_LENGTH_OF_ADDRESS);
			*BytesWritten = ETH_LENGTH_OF_ADDRESS;
			bInformationCopied = TRUE;
		}
		break;
		
	// 2006.11.01, by shien chang.
	case OID_DOT11_STATION_ID:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_STATION_ID:\n"));
		{
			*BytesNeeded = sizeof(DOT11_MAC_ADDRESS);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			PlatformMoveMemory(
				InformationBuffer, 
				pAdapter->CurrentAddress,
				*BytesNeeded);
			bInformationCopied = TRUE;
		}
		break;

	// 2006.11.01, by shien chang.
	case OID_DOT11_CURRENT_REG_DOMAIN:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_CURRENT_REG_DOMAIN:\n"));
		{
			*BytesNeeded = sizeof(ULONG);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_BUFFER_OVERFLOW;
			}

			ulInfo = DOT11_REG_DOMAIN_FCC;
			ulInfoLen = *BytesNeeded;
		}
		break;

	// 2006.11.01, by shien chang.
	case OID_DOT11_REG_DOMAINS_SUPPORT_VALUE:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_REG_DOMAINS_SUPPORT_VALUE:\n"));
		{
			PDOT11_REG_DOMAINS_SUPPORT_VALUE pValue = 
				(PDOT11_REG_DOMAINS_SUPPORT_VALUE)InformationBuffer;
			
			*BytesNeeded = sizeof(DOT11_REG_DOMAINS_SUPPORT_VALUE);
			if ( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				return NDIS_STATUS_BUFFER_OVERFLOW;
			}

			//<SC_TODO: reimplement it>
			pValue->uNumOfEntries = 1;
			pValue->uTotalNumOfEntries = 1;
			pValue->dot11RegDomainValue[0].uRegDomainsSupportIndex = 0;
			pValue->dot11RegDomainValue[0].uRegDomainsSupportValue = DOT11_REG_DOMAIN_FCC;
			*BytesWritten = *BytesNeeded;
			bInformationCopied = TRUE;
		}
		break;

	// 2006.11.01, by shien chang.
	case OID_DOT11_DESIRED_BSSID_LIST:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_DESIRED_BSSID_LIST:\n"));
		{
			return N6CQuery_DOT11_DESIRED_BSSID_LIST(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesWritten,
					BytesNeeded);
		}
		break;

	// 2006.11.02, by shien chang.
	case OID_DOT11_UNREACHABLE_DETECTION_THRESHOLD:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_UNREACHABLE_DETECTION_THRESHOLD:\n"));
		{
			*BytesNeeded = sizeof(ULONG);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			ulInfo = pNdisCommon->dot11UnreachableDetectionThreshold;
			ulInfoLen = *BytesNeeded;
		}
		break;
	
	case OID_DOT11_UNICAST_USE_GROUP_ENABLED:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_UNICAST_USE_GROUP_ENABLED:\n"));
		{
			*BytesNeeded = sizeof(BOOLEAN);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			ulInfo = pNdisCommon->dot11UnicastUseGroupEnabled;
			ulInfoLen = *BytesNeeded;
		}
		break;

	case OID_DOT11_ACTIVE_PHY_LIST:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_ACTIVE_PHY_LIST:\n"));
		{
			return N6CQuery_DOT11_ACTIVE_PHY_LIST(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesWritten,
					BytesNeeded);
		}
		break;

	case OID_DOT11_ENUM_ASSOCIATION_INFO:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_ENUM_ASSOCIATION_INFO:\n"));
		{
			return N6CQuery_DOT11_ENUM_ASSOCIATION_INFO(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesWritten,
					BytesNeeded);
		}
		break;

	case OID_DOT11_HIDDEN_NETWORK_ENABLED:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_HIDDEN_NETWORK_ENABLED:\n"));
		{
			*BytesNeeded = sizeof(BOOLEAN);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			ulInfo = pMgntInfo->bHiddenSSIDEnable;
			ulInfoLen = *BytesNeeded;
		}
		break;

	case  OID_DOT11_CURRENT_FREQUENCY:
	case  OID_DOT11_CURRENT_CHANNEL: 
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_CURRENT_CHANNEL:\n"));
		{
			*BytesNeeded = sizeof(ULONG);
			if ( InformationBufferLength < *BytesNeeded )
			{
				//return NDIS_STATUS_INVALID_LENGTH;
				return NDIS_STATUS_BUFFER_OVERFLOW;
			}

			if(pMgntInfo->mAssoc || pMgntInfo->mIbss)
			{
				
			#if (MULTICHANNEL_SUPPORT == 1)
				if(MultiChannelSwitchNeeded(pAdapter))
				{
					if (pNdisCommon->RegWfdChnl == 0)
						ulInfo = MultiChannelGetPortConnected20MhzChannel(pAdapter);
					else if (pNdisCommon->RegWfdChnl != 0)
						ulInfo = pNdisCommon->RegWfdChnl;
					RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("MultiChannelGetPortConnected20MhzChannel: %d\n", ulInfo));
				}
				else
			#endif				
				{
					ulInfo = pMgntInfo->dot11CurrentChannelNumber;
				}

				
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("A: channel: %u:\n", ulInfo));
			}
			else
			{
				//
				//Report the default channel to os, Or it will fail when in SoftAP_connectOID test.
				//receive non probe rsp. By Maddest 06052009.
				//
				//sherry added for Softap_wps error
				//20110927
				if(pAdapter->bInHctTest)
				{
					if(!IsDefaultAdapter(pAdapter))
						ulInfo = pMgntInfo->dot11CurrentChannelNumber;
					else
						ulInfo = RT_GetChannelNumber(GetDefaultAdapter(pAdapter));
				}
				else
				{
					ulInfo = RT_GetChannelNumber(GetDefaultAdapter(pAdapter));
				}
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("B: channel: %u:\n", ulInfo));
			}
			ulInfoLen = sizeof(ULONG);

			
		}
		break;

	case OID_DOT11_DATA_RATE_MAPPING_TABLE:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_DATA_RATE_MAPPING_TABLE:\n"));
		{
			return N6CQuery_DOT11_DataRateMappingTable(
                    pAdapter,
                    InformationBuffer,
                    InformationBufferLength,
                    BytesWritten,
                    BytesNeeded
                    );
		}
		break;
		
	case OID_DOT11_SUPPORTED_DATA_RATES_VALUE: 
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_SUPPORTED_DATA_RATES_VALUE:\n"));
		{
			return N6CQuery_DOT11_Supported_DataRates(
					pAdapter,
                    InformationBuffer,
                    InformationBufferLength,
                    BytesWritten,
                    BytesNeeded
                    );
		}
		break;
		
	case OID_DOT11_MULTI_DOMAIN_CAPABILITY_IMPLEMENTED:
	case OID_DOT11_MULTI_DOMAIN_CAPABILITY_ENABLED:
	case OID_DOT11_COUNTRY_STRING:
		Status = NDIS_STATUS_NOT_SUPPORTED;
		break;
		
	case OID_DOT11_SUPPORTED_TX_ANTENNA:
		{		
			return N6CQuery_DOT11_Tx_Antenna(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesWritten,
					BytesNeeded);
		}	
		break;

	// Added by Annie, 2006-10-09.
	case OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM:\n") );
		{
			return N6CQuery_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(
						pAdapter,
						InformationBuffer,
						InformationBufferLength,
						BytesWritten,
						BytesNeeded
						);
		}
		break;

	// Added by Annie, 2006-10-09.
	case OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM, 0x%08x\n", Oid) );
		{
			return N6CQuery_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(
						pAdapter,
						InformationBuffer,
						InformationBufferLength,
						BytesWritten,
						BytesNeeded
						);
		}
		break;

	// Added by Annie, 2006-10-09.
	case OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM, 0x%08x\n", Oid) );
		{
			return N6CQuery_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM(
						pAdapter,
						InformationBuffer,
						InformationBufferLength,
						BytesWritten,
						BytesNeeded
						);
		}
		break;


	// Added by Annie, 2006-10-13.
	case OID_DOT11_CIPHER_DEFAULT_KEY_ID:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_CIPHER_DEFAULT_KEY_ID, 0x%08x\n", Oid) );
		{		
			PRT_SECURITY_T		pSecInfo = &(pAdapter->MgntInfo.SecurityInfo);
			
			if (InformationBufferLength < sizeof(u4Byte))
			{
				*BytesNeeded = sizeof(u4Byte);
				return NDIS_STATUS_INVALID_LENGTH;
			}

			ulInfo = pSecInfo->DefaultTransmitKeyIdx;
			ulInfoLen = sizeof(u4Byte);
		}
		break;

	// Added by Annie, 2006-10-12.
	case OID_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR:\n") );
		{
			return N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
						pAdapter,
						InformationBuffer,
						InformationBufferLength,
						BytesWritten,
						BytesNeeded
						);
		}
		break;

	// Added by Annie, 2006-10-12.
	case OID_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR:\n") );
		{
			return N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
						pAdapter,
						InformationBuffer,			
						InformationBufferLength,
						BytesWritten,
						BytesNeeded
						);
		}
		break;

	// Added by Annie, 2006-10-12.
	case OID_DOT11_PMKID_LIST:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_PMKID_LIST, 0x%08x\n", Oid) );
		{
			PDOT11_PMKID_LIST    pPMKIDList = NULL;

			if (InformationBufferLength < (ULONG)FIELD_OFFSET(DOT11_PMKID_LIST, PMKIDs))
			{
				Status = NDIS_STATUS_BUFFER_OVERFLOW;
				*BytesNeeded = FIELD_OFFSET(DOT11_PMKID_LIST, PMKIDs);
				return Status;
			}

			pPMKIDList = (PDOT11_PMKID_LIST)InformationBuffer;
			Status = N6CQuery_DOT11_PMKID_LIST(
						pAdapter,
						pPMKIDList,
						InformationBufferLength
						);
			
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = pPMKIDList->uNumOfEntries * sizeof(DOT11_PMKID_ENTRY) + 
								FIELD_OFFSET(DOT11_PMKID_LIST, PMKIDs);
				bInformationCopied = TRUE;
			}
		}
		break;

	// Added by Annie, 2006-10-12.
	case OID_DOT11_EXTSTA_CAPABILITY:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_EXTSTA_CAPABILITY:\n") );
		{
			return N6CQuery_DOT11_EXTSTA_CAPABILITY(
						pAdapter,
						InformationBuffer,
						InformationBufferLength,
						BytesWritten,
						BytesNeeded
						);
		}
		break;

	// Added by Annie, 2006-10-16.
	case OID_RT_POWER_MGMT_REQUEST:
	case OID_DOT11_POWER_MGMT_REQUEST:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_POWER_MGMT_REQUEST, 0x%08x\n", Oid) );
		{
			if (InformationBufferLength < sizeof(ULONG))
			{
				Status = NDIS_STATUS_BUFFER_OVERFLOW;
				*BytesNeeded = sizeof(ULONG);
				return Status;
			}

			N6CQuery_DOT11_POWER_MGMT_REQUEST(
						pAdapter,
						(pu4Byte)InformationBuffer
						);
			*BytesWritten = sizeof(ULONG);
			bInformationCopied = TRUE;
		}
		break;

	case OID_DOT11_EXCLUDED_MAC_ADDRESS_LIST:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_EXCLUDED_MAC_ADDRESS_LIST:\n") );
		{
			return N6CQuery_DOT11_EXCLUDED_MAC_ADDRESS_LIST(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesWritten,
					BytesNeeded);
		}
		break;

	// 061012, by rcnjko. 
	case OID_DOT11_MPDU_MAX_LENGTH:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_MPDU_MAX_LENGTH\n") );
		{ 
			*BytesNeeded = sizeof(u4Byte);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			}

			ulInfo = sMaxMpduLng;
			ulInfoLen = sizeof(u4Byte);
		}
		break;

	// 061012, by rcnjko. 
	case OID_DOT11_ATIM_WINDOW:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_ATIM_WINDOW\n") );
		{ 
			*BytesNeeded = sizeof(ULONG);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			}

			Status = N6CQuery_DOT11_ATIM_WINDOW(pAdapter, InformationBuffer);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = *BytesNeeded;
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
		}
		break;

#if 0	// Move to OID Protal - 2011.09.22
	// 061012, by rcnjko. 
	case OID_DOT11_NIC_POWER_STATE:
		{ 
			RT_RF_POWER_STATE rfState;

			*BytesNeeded = sizeof(BOOLEAN);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_BUFFER_OVERFLOW;
				break;
			} 

			pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));
			if (rfState == eRfOff)
			{
				if (pMgntInfo->RfOffReason >= RF_CHANGE_BY_HW) 
				{
					ulInfo = FALSE;
				}
				else
				{
					ulInfo = TRUE;
				}
			}
			else
			{
				ulInfo = TRUE;
			}
			ulInfoLen = *BytesNeeded;
		}
		RT_TRACE( COMP_OID_QUERY|COMP_RF, DBG_LOUD, ("Query OID_DOT11_NIC_POWER_STATE rf %d\n", ulInfo) );		
		break;
#endif

	// 2006.11.02, by shien chang.
	case OID_DOT11_HARDWARE_PHY_STATE:
		{ 
			RT_RF_POWER_STATE rfState;
			
			*BytesNeeded = sizeof(BOOLEAN);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));
			if (rfState == eRfOff)
			{
				if (pMgntInfo->RfOffReason & RF_CHANGE_BY_HW)
				{
					ulInfo = FALSE;
				}
				else
				{
					ulInfo = TRUE;
				}
			}
			else
			{
				ulInfo = TRUE;
			}
			ulInfoLen = *BytesNeeded;
		}
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_HARDWARE_PHY_STATE: rf %d\n", ulInfo));
		break;

		
	// 061014, by rcnjko. 
	case OID_DOT11_OPTIONAL_CAPABILITY:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_OPTIONAL_CAPABILITY\n") );
		{ 
			*BytesNeeded = sizeof(DOT11_OPTIONAL_CAPABILITY);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			Status = N6CQuery_DOT11_OPTIONAL_CAPABILITY(pAdapter, InformationBuffer);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = *BytesNeeded;
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
		}
		break;

	// 061014, by rcnjko. 
	case OID_DOT11_CURRENT_OPTIONAL_CAPABILITY:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_CURRENT_OPTIONAL_CAPABILITY\n") );
		{ 
			*BytesNeeded = sizeof(DOT11_OPTIONAL_CAPABILITY);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			Status = N6CQuery_DOT11_CURRENT_OPTIONAL_CAPABILITY(pAdapter, InformationBuffer);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = *BytesNeeded;
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
		}
		break;

	// 061014, by rcnjko. 
	case OID_DOT11_CF_POLLABLE:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_CF_POLLABLE\n") );
		{ 
			*BytesNeeded = sizeof(BOOLEAN);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			Status = N6CQuery_DOT11_CF_POLLABLE(pAdapter, InformationBuffer);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = *BytesNeeded;
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
		}
		break;

	// 061014, by rcnjko. 
	case OID_RT_OPERATIONAL_RATE_SET:
	case OID_DOT11_OPERATIONAL_RATE_SET:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_OPERATIONAL_RATE_SET\n") );
		{ 
			return N6CQuery_DOT11_OPERATIONAL_RATE_SET(
					pAdapter, 
					InformationBuffer,
					InformationBufferLength,
					BytesWritten,
					BytesNeeded);
		}
		break;

	// 061014, by rcnjko. 
	case OID_RT_RTS_THRESHOLD:
	case OID_DOT11_RTS_THRESHOLD:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_RTS_THRESHOLD\n") );
		{ 
			*BytesNeeded = sizeof(ULONG);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_BUFFER_OVERFLOW;
				break;
			} 

			ulInfo = MgntActQuery_802_11_RTS_THRESHOLD(pAdapter);
			ulInfoLen = sizeof(ULONG);
		}
		break;
		
	// 061014, by rcnjko. 
	case OID_DOT11_SHORT_RETRY_LIMIT:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_SHORT_RETRY_LIMIT\n") );
		{ 
			*BytesNeeded = sizeof(ULONG);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			Status = N6CQuery_DOT11_SHORT_RETRY_LIMIT(pAdapter, InformationBuffer);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = *BytesNeeded;
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
		}
		break;

	// 061014, by rcnjko. 
	case OID_DOT11_LONG_RETRY_LIMIT:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_LONG_RETRY_LIMIT\n") );
		{ 
			*BytesNeeded = sizeof(ULONG);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			Status = N6CQuery_DOT11_LONG_RETRY_LIMIT(pAdapter, InformationBuffer);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = *BytesNeeded;
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
		}
		break;

	// 061014, by rcnjko. 
	case OID_RT_FRAGMENTATION_THRESHOLD:
	case OID_DOT11_FRAGMENTATION_THRESHOLD:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_FRAGMENTATION_THRESHOLD\n") );
		{ 
			*BytesNeeded = sizeof(ULONG);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_BUFFER_OVERFLOW;
				break;
			} 

			ulInfo = MgntActQuery_802_11_FRAGMENTATION_THRESHOLD(pAdapter);
			ulInfoLen = *BytesNeeded;
		}
		break;

	// 061014, by rcnjko. 
	case OID_DOT11_MAX_TRANSMIT_MSDU_LIFETIME:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_MAX_TRANSMIT_MSDU_LIFETIME\n") );
		{ 
			*BytesNeeded = sizeof(ULONG);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			Status = N6CQuery_DOT11_MAX_TRANSMIT_MSDU_LIFETIME(pAdapter, InformationBuffer);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = *BytesNeeded;
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
		}
		break;

	// 061014, by rcnjko. 
	case OID_DOT11_MAX_RECEIVE_LIFETIME:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_MAX_RECEIVE_LIFETIME\n") );
		{ 
			*BytesNeeded = sizeof(ULONG);
			if( InformationBufferLength < *BytesNeeded )
			{
				*BytesWritten = 0;
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			Status = N6CQuery_DOT11_MAX_RECEIVE_LIFETIME(pAdapter, InformationBuffer);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesWritten = *BytesNeeded;
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
		}
		break;

	// 061014, by rcnjko. 
	case OID_DOT11_SUPPORTED_PHY_TYPES:
		RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_SUPPORTED_PHY_TYPES\n") );
		{ 
			Status = N6CQuery_DOT11_SUPPORTED_PHY_TYPES(
						pAdapter, 
						InformationBuffer, 
						InformationBufferLength,
						BytesWritten,
						BytesNeeded);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
		}
		break;

	case OID_DOT11_MULTICAST_LIST:
		{
			Status = N6CQuery_DOT11_MULTICAST_LIST(
						pAdapter, 
						InformationBuffer, 
						InformationBufferLength,
						BytesWritten,
						BytesNeeded);
			
			if (Status == NDIS_STATUS_SUCCESS)
			{
				ulInfoLen = *BytesWritten;
				bInformationCopied = TRUE;
			}
			RT_TRACE( COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_MULTICAST_LIST num of multicast address:%d\n", pAdapter->MCAddrCount));
		}
		break;
		
	case OID_RT_QUERY_IS_MP_CHIP:
		{
			PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(pAdapter);
			ulInfo = pHalData->bIsMPChip;
			ulInfoLen = 1;
		}
		break;

	case OID_DOT11_CURRENT_PHY_ID:
	{
		if(InformationBufferLength < sizeof(ULONG))
		{
			*BytesNeeded = sizeof(ULONG);
			*BytesWritten = 0;
			return NDIS_STATUS_BUFFER_TOO_SHORT;
		}

		ulInfo = pAdapter->pNdisCommon->dot11SelectedPhyId;
		PlatformMoveMemory(InformationBuffer, &ulInfo, sizeof(ULONG));
		*BytesWritten = sizeof(ULONG);	

		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query OID_DOT11_CURRENT_PHY_ID: phyid%d\n", ulInfo));
		break;
	}

	case OID_DOT11_SUPPORTED_POWER_LEVELS:
	{
		if(InformationBufferLength < sizeof(ULONG)*9 )
		{
			*BytesNeeded = sizeof(ULONG);
			*BytesWritten = 0;
			return NDIS_STATUS_BUFFER_TOO_SHORT;
		}
		
		DbgPrint( "Query OID_DOT11_SUPPORTED_POWER_LEVELS\n" );
		*BytesWritten = sizeof(ULONG) * 9;
		PlatformZeroMemory( InformationBuffer, sizeof(ULONG) * 9 );
		break;
	}

	case OID_DOT11_BEACON_PERIOD:
	{
		PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
		
		// Clean output variables -----------------------------------
		*BytesWritten = 0;
		*BytesNeeded = 0;
		//-------------------------------------------------------

		FunctionIn(COMP_OID_QUERY);

		if(!pMgntInfo->mAssoc && pMgntInfo->mIbss && ACTING_AS_AP(pAdapter))
		{
			*BytesWritten = 0;
			Status = NDIS_STATUS_INVALID_STATE;
			return Status;
		}
				
		*BytesNeeded = sizeof(ULONG);
		if( InformationBufferLength < *BytesNeeded )
		{
			*BytesWritten = 0;
			Status = NDIS_STATUS_INVALID_LENGTH;
			return Status;
		} 
		
		Status = N6CQuery_DOT11_BEACON_PERIOD(pAdapter, (PULONG) InformationBuffer );

		if (Status == NDIS_STATUS_SUCCESS)
		{
			*BytesWritten = *BytesNeeded;
			return Status;
		}
		break;
	}

	// <LAST_OID_QUERY>
	
	default:
		// This OID is not common one handled by USB and PCI. 
		Status = NDIS_STATUS_INVALID_OID;
		break;
	}

	if(Status == NDIS_STATUS_SUCCESS)
	{        
		if(ulInfoLen <= InformationBufferLength)
		{
			// Copy result into InformationBuffer
			if(ulInfoLen && !bInformationCopied)
			{
				*BytesWritten = ulInfoLen;
				NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
			}
		}
		else if(!bInformationCopied)
		{
			// Buffer too short
			*BytesNeeded = ulInfoLen;
			Status = NDIS_STATUS_BUFFER_TOO_SHORT;
		}
	}

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<==== N6CQueryInformation, OID=0x%08x, Status=0x%X\n", Oid, Status));

	return Status;
}

// This is to offload the N6CSetInformation() since the huge size of the funcion stack causes the BSOD when DbgPrint.
static NDIS_STATUS
N6CSetInformationHandleCustomizedWifiDirectOids(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
	)
{
	PADAPTER				pAdapter = (PADAPTER)MiniportAdapterContext;
	PMGNT_INFO      			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;
	
	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;

#if (P2P_SUPPORT == 1)
	case OID_RT_P2P_ACCEPT_INVITATION_REQ:
		{
			if(InformationBufferLength < 7)
			{
				*BytesNeeded = 7;
				return	NDIS_STATUS_INVALID_LENGTH;
			}
			
			(GET_P2P_INFO(pAdapter))->bAcceptInvitation = *((PBOOLEAN)((pu1Byte)InformationBuffer + 0));
			PlatformMoveMemory((GET_P2P_INFO(pAdapter))->AccpetInvitationDeviceAddress, 
				(pu1Byte)InformationBuffer + 1, 6);
		}
		break;
	case OID_RT_P2P_VERSION:
		{
			u4Byte verToSet = 0;

			if(pMgntInfo->bDisableRtkSupportedP2P)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, 
					("Ignore set of 0x%08X because bDisableRtkSupportedP2P is turned on\n", Oid));
				Status = NDIS_STATUS_NOT_SUPPORTED;
				break;
			}
			
			if(InformationBufferLength < sizeof(u4Byte))
			{
				*BytesNeeded = sizeof(u4Byte);
				return NDIS_STATUS_INVALID_LENGTH;
			}

			verToSet = *((pu4Byte)InformationBuffer);

			if(P2P_VERSION < verToSet)
			{
				return NDIS_STATUS_NOT_SUPPORTED;
			}
			
			(GET_P2P_INFO(pAdapter))->P2PVersion = verToSet;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_VERSION to %u\n", verToSet));
		}
		break;	
	case OID_RT_P2P_MODE:
		{
			u4Byte index = 0;
			BOOLEAN bP2PMode;
			BOOLEAN bGO;
			u1Byte ListenChannel;
			u1Byte IntendedOpChannel;
			u1Byte GOIntent;

			if(pMgntInfo->bDisableRtkSupportedP2P)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, 
					("Ignore set of 0x%08X because bDisableRtkSupportedP2P is turned on\n", Oid));
				Status = NDIS_STATUS_NOT_SUPPORTED;
				break;
			}

			if(InformationBufferLength < sizeof(BOOLEAN) * 2 + sizeof(u1Byte) * 3)
			{
				*BytesNeeded = sizeof(BOOLEAN) * 2 + sizeof(u1Byte) * 3;
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_MODE: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}

			bP2PMode = *((PBOOLEAN)((pu1Byte)InformationBuffer + index));
			index += sizeof(BOOLEAN);
			
			bGO = *((PBOOLEAN)((pu1Byte)InformationBuffer + index));
			index += sizeof(BOOLEAN);
			
			ListenChannel = *((pu1Byte)((pu1Byte)InformationBuffer + index));
			index += sizeof(u1Byte);

			IntendedOpChannel = *((pu1Byte)((pu1Byte)InformationBuffer + index));
			index += sizeof(u1Byte);

			GOIntent = *((pu1Byte)((pu1Byte)InformationBuffer + index));
			index += sizeof(u1Byte);
				
			MgntActSet_P2PMode(pAdapter, 
				bP2PMode, 
				bGO, 
				ListenChannel,
				IntendedOpChannel,
				GOIntent);
		}
		break;

	case OID_RT_ROAM_FAKE_SIGNAL:
		{
			if(InformationBufferLength < sizeof(u2Byte))
			{
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_OID_SET, DBG_WARNING,
					("Set OID_RT_ROAM_FAKE_SIGNAL: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}			
			pMgntInfo->RegFakeRoamSignal[0] = *((pu1Byte)InformationBuffer);
			pMgntInfo->RegFakeRoamSignal[1] = *((pu1Byte)InformationBuffer + 1);
			RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("RegFakeRoamSignal[0] = %d, RegFakeRoamSignal[1] = %d\n", pMgntInfo->RegFakeRoamSignal[0], pMgntInfo->RegFakeRoamSignal[1]));
		}
		break;
		
	case OID_RT_P2P_PROVISION_IE: 
		{
			if(InformationBufferLength < sizeof(u1Byte))
			{
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_PROVISION_IE: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}
			
			MgntActSet_P2PProvisionIE(pAdapter, InformationBuffer, (u1Byte)InformationBufferLength);
		}
		break;
	case OID_RT_P2P_FLUSH_SCAN_LIST: 
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD,
				("Set OID_RT_P2P_FLUSH_SCAN_LIST:\n"));
			MgntActSet_P2PFlushScanList(pAdapter);
		}
		break;
	case OID_RT_P2P_PROVISIONING_RESULT:
		{
			if(InformationBufferLength < sizeof(P2P_PROVISIONING_RESULT))
			{
				*BytesNeeded = sizeof(P2P_PROVISIONING_RESULT);
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_PROVISIONING_RESULT: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}

			RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_PROVISIONING_RESULT: %u\n", 
					*(PP2P_PROVISIONING_RESULT)InformationBuffer));
			MgntActSet_P2PProvisioningResult(pAdapter, *(PP2P_PROVISIONING_RESULT)InformationBuffer);
		}
		break;
	case OID_RT_P2P_DEVICE_DISCOVERY:
		{
			PP2P_INFO pP2PInfo;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("OID_RT_P2P_DEVICE_DISCOVERY\n"));
			
			if(InformationBufferLength < sizeof(u1Byte))
			{
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_DEVICE_DISCOVERY: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}
			
			pP2PInfo = GET_P2P_INFO(pAdapter);

			P2PResetCommonChannelArrivingProcess(pP2PInfo);
			if(!P2PDeviceDiscovery(pP2PInfo, (*(pu1Byte)InformationBuffer)))
			{
				return NDIS_STATUS_MEDIA_BUSY;
			}
		}
		break;
	case OID_RT_P2P_CONNECT_REQUEST:
		{
			*BytesNeeded = sizeof(u1Byte) * 6;
			if(InformationBufferLength < *BytesNeeded)
			{
				
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_CONNECT_REQUEST: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}
			
			if(!P2PConnect(GET_P2P_INFO(pAdapter), (pu1Byte)InformationBuffer))
			{
				return NDIS_STATUS_FAILURE;
			}
		}
		break;
	case OID_RT_P2P_DISCONNECT_REQUEST:
		{
			P2PDisconnect(GET_P2P_INFO(pAdapter));
		}
		break;
	case OID_RT_P2P_INVITE_PEER:
		{
			if(InformationBufferLength < sizeof(P2P_LIB_INVITATION_REQ_CONTEXT))
			{
				*BytesNeeded = sizeof(P2P_LIB_INVITATION_REQ_CONTEXT);
				RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_INVITE_PEER: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}

			P2PInvitePeerStart(GET_P2P_INFO(pAdapter), (PP2P_LIB_INVITATION_REQ_CONTEXT)InformationBuffer);
		}
		break;
	case OID_RT_P2P_GO_INTENT:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			BOOLEAN bTieBreaker = pP2PInfo->GOIntent & 0x01;

			if(InformationBufferLength < sizeof(u1Byte))
			{
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_GO_INTENT: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}
			
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("Set OID_RT_P2P_GO_INTENT: from %u to %u with TieBreaker: %u unchanged\n", 
				pP2PInfo->GOIntent >> 1, *((pu1Byte)InformationBuffer), bTieBreaker));

			pP2PInfo->GOIntent = ((*((pu1Byte)InformationBuffer) << 1) | bTieBreaker);
		}
		break;
	case OID_RT_P2P_DEVICE_DISCOVERABILITY_REQ:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);

			if(InformationBufferLength < sizeof(u1Byte) * 6)
			{
				*BytesNeeded = sizeof(u1Byte) * 6;
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_DEVICE_DISCOVERABILITY_REQ: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}

			P2PDeviceDiscoverabilityReq(pP2PInfo, (pu1Byte)InformationBuffer, FALSE);
		}
		break;
	case OID_RT_P2P_PROVISION_DISCOVERY:
		{
			if(InformationBufferLength < 6 + sizeof(u2Byte))
			{
				*BytesNeeded = 6 + sizeof(u2Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_PROVISION_DISCOVERY: invalid length(%d < %d)\n", 
					InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}

			GET_P2P_INFO(pAdapter)->ProvisionReqRetryCnt = 0;
			if(!P2PProvisionDiscovery(GET_P2P_INFO(pAdapter), 
				(pu1Byte)InformationBuffer, 
				*(pu2Byte)((pu1Byte)InformationBuffer + 6)))
			{
				return NDIS_STATUS_FAILURE;
			}
		}
		break;
	case OID_RT_P2P_CAPABILITY:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			u1Byte DevCap;
			u1Byte GrpCap;

			if(InformationBufferLength < sizeof(u1Byte) * 2)
			{
				*BytesNeeded = sizeof(u1Byte) * 2;
				RT_TRACE(COMP_P2P, DBG_LOUD,("Set OID_RT_P2P_CAPABILITY: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}

			DevCap = *((pu1Byte)InformationBuffer + 0);
			GrpCap = *((pu1Byte)InformationBuffer + 1);

			pP2PInfo->DeviceCapability = DevCap;
			pP2PInfo->GroupCapability = GrpCap;

			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("OID_RT_P2P_CAPABILITY: set DC=%u, GC=%u\n", 
				pP2PInfo->DeviceCapability, pP2PInfo->GroupCapability));

			//P2PDumpDeviceCapability(pP2PInfo->DeviceCapability);
			//P2PDumpGroupCapability(pP2PInfo->GroupCapability);
		}
		break;
	case OID_RT_P2P_GO_SSID:
	case OID_RT_P2P_GO_SSID_POSTFIX:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			u1Byte SsidLen = 0;
			pu1Byte SsidBuf = NULL;

			if(InformationBufferLength < sizeof(u1Byte))
			{
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_GO_SSID: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return NDIS_STATUS_INVALID_LENGTH;
			}

			SsidLen = *((pu1Byte)InformationBuffer + 0);

			if(InformationBufferLength < sizeof(u1Byte) + SsidLen)
			{
				*BytesNeeded = sizeof(u1Byte) + SsidLen;
				RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_GO_SSID: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return NDIS_STATUS_INVALID_LENGTH;
			}

			//
			// To append the post fix to DIRECT-xx which has length 9
			//
			if(SsidLen > 32 - 9)
			{
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_GO_SSID: invalid SSID postfixlength(%d < %d)\n", SsidLen, 32 - 9));
				return NDIS_STATUS_INVALID_LENGTH;
			}

			SsidBuf = ((pu1Byte)InformationBuffer + 1);

			//
			// If this OID is called, GO SSID will not be changed until this OID is set again or
			// P2PInitialize is called.
			//

			CopySsid(pP2PInfo->SSIDPostfixBuf, pP2PInfo->SSIDPostfixLen, SsidBuf, SsidLen);

			RT_PRINT_STR(COMP_P2P, DBG_LOUD, "OID_RT_P2P_GO_SSID\n", pP2PInfo->SSIDPostfixBuf, pP2PInfo->SSIDPostfixLen);
		}
		break;
	case OID_RT_P2P_INTERFACE_ADDRESS:
		{
			if(InformationBufferLength < 6)
			{
				*BytesNeeded = 6;
				return NDIS_STATUS_INVALID_LENGTH;
			}
			PlatformMoveMemory((GET_P2P_INFO(pAdapter))->InterfaceAddress, InformationBuffer, 6);

			if(!eqMacAddr(pAdapter->CurrentAddress, (pu1Byte)InformationBuffer))
			{
				//NicIFSetMacAddress1(pAdapter, (pu1Byte)InformationBuffer);
			}

			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Set OID_RT_P2P_INTERFACE_ADDRESS: ", InformationBuffer);
		}
		break;
	case OID_RT_P2P_DEVICE_ADDRESS:
		{
			if(InformationBufferLength < 6)
			{
				*BytesNeeded = 6;
				return NDIS_STATUS_INVALID_LENGTH;
			}
			PlatformMoveMemory((GET_P2P_INFO(pAdapter))->DeviceAddress, InformationBuffer, 6);

			if(!eqMacAddr(pAdapter->CurrentAddress, (pu1Byte)InformationBuffer))
			{
				//NicIFSetMacAddress1(pAdapter, (pu1Byte)InformationBuffer);
			}

			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Set OID_RT_P2P_DEVICE_ADDRESS: ", InformationBuffer);
		}
		break;
	case OID_RT_P2P_POWER_SAVE:
		{
			PP2P_POWERSAVE_SET	pP2pPs;
			
			*BytesNeeded = sizeof(P2P_POWERSAVE_SET);
			if(InformationBufferLength < *BytesNeeded)
				return NDIS_STATUS_INVALID_LENGTH;

			pP2pPs = (PP2P_POWERSAVE_SET)InformationBuffer;
			RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Set OID: OID_RT_P2P_POWER_SAVE:\n", pP2pPs, sizeof(P2P_POWERSAVE_SET));
			P2PSetPowerSaveMode(GET_P2P_INFO(pAdapter), pP2pPs, 0, P2P_PS_UPDATE_BY_USER);
		}
		break;
	case OID_RT_P2P_OP_CHANNEL:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);

			if(InformationBufferLength < sizeof(u1Byte))
			{
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD,("Set OID_RT_P2P_OP_CHANNEL: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}

			if(pP2PInfo->Role > P2P_DEVICE)
			{
				RT_TRACE(COMP_P2P, DBG_LOUD,
					("Set OID_RT_P2P_OP_CHANNEL: in operating state: %u\n",
					pP2PInfo->State));
				return NDIS_STATUS_FAILURE;
			}
			
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("Set OID_RT_P2P_OP_CHANNEL: from %u to %u\n", 
				pP2PInfo->OperatingChannel, *(pu1Byte)InformationBuffer));

			pP2PInfo->OperatingChannel = *(pu1Byte)InformationBuffer;
		}
		break;
	case OID_RT_P2P_LISTEN_CHANNEL:
		{
			if(InformationBufferLength < sizeof(u1Byte))
			{
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD,("Set OID_RT_P2P_LISTEN_CHANNEL: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}

			MgntActSet_P2PListenChannel(pAdapter, *(pu1Byte)InformationBuffer);
		}
		break;
	case OID_RT_P2P_EXTENDED_LISTEN_TIMING:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
			u4Byte index;
			BOOLEAN bOn;
			u2Byte Period;
			u2Byte Duration;

			if(InformationBufferLength < sizeof(BOOLEAN) + 2 * sizeof(u2Byte))
			{
				*BytesNeeded = sizeof(BOOLEAN);
				RT_TRACE(COMP_P2P, DBG_LOUD,("Set OID_RT_P2P_EXTENDED_LISTEN_TIMING: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return NDIS_STATUS_INVALID_LENGTH;
			}

			if(!P2P_ENABLED(pP2PInfo))
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, 
				("Set OID_RT_P2P_EXTENDED_LISTEN_TIMING: P2P NOT enabled\n"));
				break;
			}

			index = 0;
			bOn = *((PBOOLEAN)((pu1Byte)InformationBuffer + index));
			index += sizeof(BOOLEAN);
			Period = *((pu2Byte)((pu1Byte)InformationBuffer + index));
			index += sizeof(u2Byte);
			Duration= *((pu2Byte)((pu1Byte)InformationBuffer + index));
			index += sizeof(u2Byte);

			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("Set OID_RT_P2P_EXTENDED_LISTEN_TIMING: bOn: %u, period: %u(ms), duration: %u(ms)\n", 
				bOn, Period, Duration));

			pP2PInfo->bSendProbeReqInExtendedListen = bOn;
			pP2PInfo->ExtListenTimingPeriod = Period;
			pP2PInfo->ExtListenTimingDuration = Duration;
		}
		break;		

	case OID_RT_P2P_SERVICE_DISCOVERY_REQ:
		{
			PP2P_SD_REQ_CONTEXT pServiceQueryContent = (PP2P_SD_REQ_CONTEXT)InformationBuffer;
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);

			if(InformationBufferLength < (ULONG)FIELD_OFFSET(P2P_SD_REQ_CONTEXT, ServiceReqTLVList))
			{
				*BytesNeeded = FIELD_OFFSET(P2P_SD_REQ_CONTEXT, ServiceReqTLVList);
				return NDIS_STATUS_INVALID_LENGTH;
			}

			if(InformationBufferLength < 
				FIELD_OFFSET(P2P_SD_REQ_CONTEXT, ServiceReqTLVList) + 
				sizeof(P2P_SERVICE_REQ_TLV) * pServiceQueryContent->ServiceReqTLVSize)
			{
				*BytesNeeded = FIELD_OFFSET(P2P_SD_REQ_CONTEXT, ServiceReqTLVList) + 
					sizeof(P2P_SERVICE_REQ_TLV) * pServiceQueryContent->ServiceReqTLVSize;
				return NDIS_STATUS_INVALID_LENGTH;
			}

			RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_SERVICE_DISCOVERY_REQ\n"));

			P2PServiceDiscoveryReq(pP2PInfo, pServiceQueryContent);
		}
		break;
	case OID_RT_P2P_SERVICE_DISCOVERY_RSP:	
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_SERVICE_DISCOVERY_RSP\n"));
			if(InformationBufferLength < (ULONG)FIELD_OFFSET(P2P_SD_RSP_CONTEXT, ServiceRspTLVList))
			{
				*BytesNeeded = FIELD_OFFSET(P2P_SD_RSP_CONTEXT, ServiceRspTLVList);
				return NDIS_STATUS_INVALID_LENGTH;
			}
			
			P2PServiceDiscoveryRsp(GET_P2P_INFO(pAdapter), (PP2P_SD_RSP_CONTEXT)InformationBuffer);
		}
		break;

	case OID_RT_P2P_SERVICE_FRAG_THRESHOLD:
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_SERVICE_FRAG_THRESHOLD\n"));
			if(InformationBufferLength < sizeof(u2Byte))
			{
				*BytesNeeded = sizeof(u2Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD,("Set OID_RT_P2P_SERVICE_FRAG_THRESHOLD: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return NDIS_STATUS_INVALID_LENGTH;
			}
			if(RT_STATUS_SUCCESS == P2PSetServiceFragThreshold(pAdapter, *((pu2Byte)InformationBuffer)))
			{
				Status = NDIS_STATUS_FAILURE;
			}
		}
		break;		
	case OID_RT_P2P_GO_BEACON_INTERVAL:
			{
				PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
		
				if(InformationBufferLength < sizeof(u2Byte))
				{
					*BytesNeeded = sizeof(u2Byte);
					RT_TRACE(COMP_P2P, DBG_LOUD,("Set OID_RT_P2P_GO_BEACON_INTERVAL: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
					return NDIS_STATUS_INVALID_LENGTH;
				}
		
				P2PGOSetBeaconInterval(pP2PInfo, *(pu2Byte)InformationBuffer);
			}
			break;

	case OID_RT_P2P_GO_NEGO_RESULT:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);

			if(InformationBufferLength < sizeof(u1Byte))
			{
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD,("Set OID_RT_P2P_GO_NEGO_RESULT: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}
			
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("Set OID_RT_P2P_GO_NEGO_RESULT: from %u to %u \n", 
				pP2PInfo->PreviousGONegoResult, *(pu1Byte)InformationBuffer));

			pP2PInfo->PreviousGONegoResult = *(pu1Byte)InformationBuffer;
		}
		break;
	case OID_RT_P2P_GO_PREPROVISIONING:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);

			u4Byte SsidLen = 0;

			if(InformationBufferLength < sizeof(u4Byte))
			{
				*BytesNeeded = sizeof(u4Byte);
				RT_TRACE(COMP_P2P, DBG_LOUD,("Set OID_RT_P2P_GO_PREPROVISIONING: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return	NDIS_STATUS_INVALID_LENGTH;
			}
			
			SsidLen = *(pu4Byte)InformationBuffer;

			if(InformationBufferLength < sizeof(u4Byte) + SsidLen)
			{
				*BytesNeeded = sizeof(u4Byte) + SsidLen;
				RT_TRACE(COMP_P2P, DBG_LOUD,("Set OID_RT_P2P_GO_PREPROVISIONING: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return NDIS_STATUS_INVALID_LENGTH;
			}

			pP2PInfo->SSIDLen = (u1Byte)SsidLen;
			PlatformMoveMemory(pP2PInfo->SSIDBuf, ((pu1Byte)InformationBuffer) + sizeof(u4Byte), SsidLen);

			RT_PRINT_STR(COMP_P2P, DBG_LOUD, "Set OID_RT_P2P_GO_PREPROVISIONING: SSID:", pP2PInfo->SSIDBuf, pP2PInfo->SSIDLen);

			P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
			
			pP2PInfo->ConnectionContext.bGoingToBeGO = TRUE;
			pP2PInfo->State = P2P_STATE_PRE_PROVISIONING;
			PlatformCancelTimer(pAdapter, &pP2PInfo->P2PMgntTimer);
			PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
		}
		break;
	case OID_RT_P2P_CLIENT_PREPROVISIONING:
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);

			P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);

			pP2PInfo->ConnectionContext.bGoingToBeGO = FALSE;
			pP2PInfo->State = P2P_STATE_PRE_PROVISIONING;
			PlatformCancelTimer(pAdapter, &pP2PInfo->P2PMgntTimer);
			PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
		}
		break;
	case OID_RT_P2P_CHANNEL_LIST:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_P2P_CHANNEL_LIST\n"));
		{
			MgntActSet_P2PChannelList(pAdapter, *(pu1Byte)InformationBuffer, (pu1Byte)InformationBuffer + 1);
		}
		break;

	case OID_RT_P2P_PROFILE_LIST:
		{
			PP2P_PROFILE_LIST pProfileList = NULL;

			RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_PROFILE_LIST\n"));
			
			if(InformationBufferLength < (ULONG)FIELD_OFFSET(P2P_PROFILE_LIST, profileList))
			{
				*BytesNeeded = FIELD_OFFSET(P2P_PROFILE_LIST, profileList);
				RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_PROFILE_LIST: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return NDIS_STATUS_INVALID_LENGTH;
			}

			pProfileList = (PP2P_PROFILE_LIST)InformationBuffer;

			if(InformationBufferLength < FIELD_OFFSET(P2P_PROFILE_LIST, profileList) + pProfileList->nProfiles * sizeof(P2P_PROFILE_LIST_ENTRY))
			{
				*BytesNeeded = FIELD_OFFSET(P2P_PROFILE_LIST, profileList) + pProfileList->nProfiles * sizeof(P2P_PROFILE_LIST_ENTRY);
				RT_TRACE(COMP_P2P, DBG_LOUD, ("Set OID_RT_P2P_PROFILE_LIST: invalid length(%d < %d)\n", InformationBufferLength, *BytesNeeded));
				return NDIS_STATUS_INVALID_LENGTH;
			}

			if(RT_STATUS_SUCCESS != P2PSetProfileList(pAdapter, pProfileList))
			{
				Status = NDIS_STATUS_FAILURE;
			}
		}
		break;

	#endif // #if (P2P_SUPPORT == 1)

	case OID_RT_P2P_FULL_SSID:
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_P2P_FULL_SSID\n"));
			Status = NdisStatusFromRtStatus(P2P_SetP2PGoFullSSID(pAdapter, (pu1Byte)InformationBuffer, InformationBufferLength));
		}
		break;
	
	}

	return Status;
}


// This is to offload the N6CSetInformation() since the huge size of the funcion stack causes the BSOD when DbgPrint.
static NDIS_STATUS
N6CSetInformationHandleCustomized11nOids(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
	)
{
	PADAPTER				pAdapter = (PADAPTER)MiniportAdapterContext;
	PMGNT_INFO      			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;
	
	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;

		// Joseph test
	case OID_RT_11N_SILENT_RESET:
		{

		//Keep for debug usb3.0 20120907
#if (HAL_8812A_USB == 1)			
			PRT_USB_DEVICE		pDevice;
			u1Byte direction;

			pDevice = GET_RT_USB_DEVICE(pAdapter);

			direction = *((u1Byte*)InformationBuffer);
			if(direction == 0)  //out pipe need to be reset
				PlatformScheduleWorkItem(&(pDevice->FixOutPipeErrorWorkItem));
			else if(direction == 1) //in pipe need to be reset
				PlatformScheduleWorkItem(&(pDevice->FixInPipeErrorWorkItem));
			else
				RT_TRACE(COMP_OID_SET, DBG_LOUD,( "Wrong parameters!\n" ));

			RT_TRACE(COMP_OID_SET, DBG_LOUD,( "Set pipe reset!\n" ));
#endif				
		}
		break;

	case OID_RT_AMPDU_BURST_MODE:
		{
			HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
			pHalData->AMPDUBurstMode =  (*(pu1Byte)InformationBuffer);
			DbgPrint("OID_RT_AMPDU_BURST_MODE: %d\n", pHalData->AMPDUBurstMode);
		}	
		break;

	case OID_RT_AMPDU_BURST_NUM:
		{
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
			pHalData->AMPDUBurstNum = (*(pu1Byte)InformationBuffer);
			DbgPrint("OID_RT_AMPDU_BURST_NUM: %d\n", pHalData->AMPDUBurstNum);
		}	
		break;

	case OID_RT_AUTO_AMPDU_BURST_ENABLE:
		{
			HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
			pHalData->bAutoAMPDUBurstMode =  (BOOLEAN)(*(pu1Byte)InformationBuffer);
			DbgPrint("OID_RT_AUTO_AMPDU_BURST_ENABLE:  bAutoAMPDUBurstMode %d\n", pHalData->bAutoAMPDUBurstMode);
		}
		break;

	case OID_RT_AUTO_AMPDU_BURST_THRESHOLD:
		{
			HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
			pNdisCommon->RegAutoAMPDUBurstModeThreshold = (u2Byte)(*(pu1Byte)InformationBuffer);
			pHalData->AutoAMPDUBurstModeThreshold = pNdisCommon->RegAutoAMPDUBurstModeThreshold ;
			DbgPrint("OID_RT_AUTO_AMPDU_BURST_THRESHOLD:  RegAutoAMPDUBurstModeThreshold %d, AutoAMPDUBurstModeThreshold %d\n",pNdisCommon->RegAutoAMPDUBurstModeThreshold,pHalData->AutoAMPDUBurstModeThreshold);
		}
		break;

	case OID_RT_TX_CHECK_TP_THRESHOLD:
		{
			HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
			pNdisCommon->RegTxHignTPThreshold = (u2Byte)(*(pu1Byte)InformationBuffer);
			pHalData->TxHignTPThreshold = pNdisCommon->RegTxHignTPThreshold;
		}
		break;

	case OID_RT_RX_CHECK_TP_THRESHOLD:
		{
			HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
			pNdisCommon->RegRxHignTPThreshold = (u2Byte)(*(pu1Byte)InformationBuffer);
			pHalData->RxHignTPThreshold = pNdisCommon->RegRxHignTPThreshold;
		}
		break;		

	case OID_RT_BEAMFORMING_START:
		{
			u1Byte RA[6];
			u1Byte AID, Mode, Rate;
			CHANNEL_WIDTH 	BW;

			if(InformationBufferLength < 8)
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("[WARNING] OID_RT_BEAMFORMING_START, invalid buffer length (%d)!\n", InformationBufferLength));
				*BytesNeeded = 8;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}	
			cpMacAddr(RA, (pu1Byte)InformationBuffer+3);
			Mode = (*((pu1Byte)InformationBuffer)  >>  4) & 0xf;
			BW = *((pu1Byte)(InformationBuffer)) & 0xf;
			AID = *((pu1Byte)InformationBuffer+1);
			Rate = *((pu1Byte)InformationBuffer+2);
		}
		break;

	case OID_RT_BEAMFORMING_END:
		{
			HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
		}
		break;

	case OID_RT_BEAMFORMING_PERIOD:
		{
			u2Byte Period;
			u1Byte Idx, Mode;
			CHANNEL_WIDTH 	BW;
			HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(pAdapter);
			PDM_ODM_T				pDM_Odm = &pHalData->DM_OutSrc;

			if(InformationBufferLength < 4)
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("[WARNING] OID_RT_BEAMFORMING_PERIOD, invalid buffer length (%d)!\n", InformationBufferLength));
				*BytesNeeded = 4;
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}	
			
			Mode = (*((pu1Byte)InformationBuffer)  >>  4) & 0xf;
			BW = *((pu1Byte)(InformationBuffer)) & 0xf;
			Idx = *((pu1Byte)InformationBuffer+1);
			Period =( (*((pu1Byte)InformationBuffer+2)) << 8) | (*((pu1Byte)InformationBuffer+3));
		}
		break;

	

	case OID_RT_ADCSMP_TRIG:
		{
			u2Byte	PollingTime;
			u1Byte	TrigSel, TrigSigSel, DmaDataSigSel, TriggerTime;

			TrigSel = (*((pu1Byte)(InformationBuffer)) ) >> 7;
			TrigSigSel = *((pu1Byte)(InformationBuffer)) & 0x1f;
			DmaDataSigSel =*((pu1Byte)InformationBuffer+1)  & 0xf;

			TriggerTime = *((pu1Byte)InformationBuffer+2);
			PollingTime =( (*((pu1Byte)InformationBuffer+5)) << 8) | (*((pu1Byte)InformationBuffer+4));

			ADCSmp_Set(pAdapter, TrigSel, TrigSigSel, DmaDataSigSel, TriggerTime, PollingTime);
		}	
		break;

	case OID_RT_ADCSMP_STOP:
		ADCSmp_Stop(pAdapter);
		break;
		
	case OID_RT_11N_TX_RATE_DISPLAY:
		pMgntInfo->OSTxRateDisplayType = *((pu1Byte)InformationBuffer);
		break;
		
	case OID_RT_11N_FORCED_ADDBA:
		{
			PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
			pHTInfo->bAcceptAddbaReq  = *((pu1Byte)InformationBuffer);
			RT_TRACE(COMP_OID_SET, DBG_LOUD,( "Force addba %x\n", pHTInfo->bAcceptAddbaReq));
		}
		break;

	case OID_RT_11N_FORCED_AMPDU:
			{
			u1Byte 					nAMPDU_Ability;
			PADAPTER				pExtAdapter = GetFirstExtAdapter(pAdapter);
			PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
			PRT_HIGH_THROUGHPUT	pExtHTInfo = NULL;

			if(pExtAdapter != NULL)
				pExtHTInfo = pExtAdapter->MgntInfo.pHTInfo;
			
			pHTInfo->ForcedAMPDUMode = *((pu1Byte)InformationBuffer);
			if(pExtHTInfo != NULL)
				pExtHTInfo->ForcedAMPDUFactor = pHTInfo->ForcedAMPDUMode;

			switch(pHTInfo->ForcedAMPDUMode)
			{					
				case HT_AMPDU_AUTO:
				case HT_AMPDU_DISABLE:
					// Do nothing
					break;
					
				case HT_AMPDU_ENABLE:
					// Verify input paramter.
					if(InformationBufferLength < 2)
					{
						Status = NDIS_STATUS_INVALID_LENGTH;
						*BytesNeeded = 2;
						return Status;
					}

					nAMPDU_Ability = *((pu1Byte)InformationBuffer+1);

					if(nAMPDU_Ability > 0x77)
					{
						Status = NDIS_STATUS_INVALID_DATA;
						return Status;
					}
					
					pHTInfo->ForcedAMPDUFactor = nAMPDU_Ability&0x07;
					pHTInfo->ForcedMPDUDensity = (nAMPDU_Ability>>4)&0x07;
					if(pExtHTInfo != NULL)
					{
						pExtHTInfo->ForcedAMPDUFactor = pHTInfo->ForcedAMPDUFactor ;
						pExtHTInfo->ForcedMPDUDensity = pHTInfo->ForcedMPDUDensity;
					}	
					break;
			}
		}
		break;

	case OID_RT_11N_FORCED_AMSDU:
		{
			PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
			u2Byte					nAMSDU_MaxSize = 0;
			u1Byte 					nAMSDU_SizeType = 0, nAMSDU_MaxNum = 0;

			HT_MGNT_SET_AMSDU(pAdapter, (HT_AMSDU_MODE_E)*((pu1Byte)InformationBuffer));
			switch(pHTInfo->ForcedAMSDUMode)
			{					
				case HT_AMSDU_AUTO:
				case HT_AMSDU_DISABLE:
					{
						u1Byte 		QueueId;
						PRT_TCB		pTcb;

						PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

						for(QueueId = 0; QueueId < MAX_TX_QUEUE; QueueId++)
						{
							while(!RTIsListEmpty(&pAdapter->TcbAggrQueue[QueueId]))
							{
								pTcb = (PRT_TCB)RTRemoveHeadList(&pAdapter->TcbAggrQueue[QueueId]);
								pAdapter->TcbCountInAggrQueue[QueueId]--;
								PreTransmitTCB(pAdapter, pTcb);
							}
						}
						PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
					}
					break;

				
				case HT_AMSDU_ENABLE:
				case HT_AMSDU_WITHIN_AMPDU:
					if(InformationBufferLength < 5)
					{
						Status = NDIS_STATUS_INVALID_LENGTH;
						*BytesNeeded = 5;
						return Status;
					}

					nAMSDU_SizeType = *((pu1Byte)InformationBuffer+1);

					if(nAMSDU_SizeType > 6)
					{
						Status = NDIS_STATUS_INVALID_DATA;
						return Status;					
					}

					nAMSDU_MaxSize= ( (*((pu1Byte)InformationBuffer+2)) << 8) | (*((pu1Byte)InformationBuffer+3));

					if(nAMSDU_SizeType == 0 && nAMSDU_MaxSize == 0 )
					{
						Status = NDIS_STATUS_INVALID_DATA;
						return Status;					
					}

					switch(nAMSDU_SizeType)
					{
						case 0:
						pHTInfo->ForcedAMSDUMaxSize = nAMSDU_MaxSize;
						break;
						case 1:
						pHTInfo->ForcedAMSDUMaxSize = HT_AMSDU_SIZE_4K;
						break;
						case 2:
						pHTInfo->ForcedAMSDUMaxSize = HT_AMSDU_SIZE_8K;
						break;
						case 3:
						pHTInfo->ForcedAMSDUMaxSize = VHT_AMSDU_SIZE_4K;
						break;
						case 4:
						pHTInfo->ForcedAMSDUMaxSize = VHT_AMSDU_SIZE_8K;
						break;
						case 5:
						pHTInfo->ForcedAMSDUMaxSize = VHT_AMSDU_SIZE_11K;
						break;
						default:
						pHTInfo->ForcedAMSDUMaxSize = AMSDU_SIZE_UNSPECIFED;
						break;				
					}
					
					nAMSDU_MaxNum = *((pu1Byte)InformationBuffer+4);

					if(pHTInfo->ForcedAMSDUMode == HT_AMSDU_WITHIN_AMPDU && nAMSDU_MaxNum < 1 )
					{
						Status = NDIS_STATUS_INVALID_DATA;
						return Status;					
					}
					
					pHTInfo->ForcedAMSDUMaxNum =nAMSDU_MaxNum;
					break;
			}

			//DbgPrint(	"AMSDU mode %d AMSDU size %d AMSDU Num %d\n", 
			//			pHTInfo->ForcedAMSDUMode, pHTInfo->ForcedAMSDUMaxSize, pHTInfo->ForcedAMSDUMaxNum);
		}
		break;

	case OID_RT_11N_FORCED_SHORTGI:
		{
			PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
				
			if(*((pu1Byte)InformationBuffer) > 2)
			{
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;					
			}
			
			pHTInfo->ForcedShortGI = (*((pu1Byte)InformationBuffer));
				
		}
		break;

	case OID_RT_11N_FORCED_LDPC:
		{
			if(RT_STATUS_SUCCESS !=
				(rtStatus = MgntActSet_802_11_LDPC_MODE(pAdapter, InformationBuffer, InformationBufferLength)))
			{
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;					
			}
			
		}
		break;	

	case OID_RT_11N_FORCED_STBC:
		{
			if(RT_STATUS_SUCCESS !=
				(rtStatus = MgntActSet_802_11_STBC_MODE(pAdapter, InformationBuffer, InformationBufferLength)))
			{
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;					
			}
			
		}
		break;	

	case OID_RT_11N_MIMOPS_MODE:
		{
			u1Byte	NewMimoPsMode = *((u1Byte*)InformationBuffer);
			PADAPTER	Adapter = GetDefaultAdapter(pAdapter);
			if((NewMimoPsMode == 2) || (NewMimoPsMode > 3))
			{
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;				
			}

			// get first Adapter with AP mode.
			while(Adapter != NULL)
			{
				if(ACTING_AS_AP(Adapter))
					break;
				Adapter = GetNextExtAdapter(Adapter);
			}

			if(Adapter == NULL)
				Adapter = GetDefaultAdapter(pAdapter);

			if(ACTING_AS_AP(Adapter))
			{
				PRT_WLAN_STA pEntry;
				u1Byte i;
				u1Byte		multicast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

				SendMimoPsFrame(Adapter, multicast, NewMimoPsMode);

				for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
				{
					pEntry = AsocEntry_EnumStation(Adapter, i);
					if(pEntry!=NULL)
					{
						if(pEntry->bAssociated)
							SendMimoPsFrame(Adapter, pEntry->MacAddr, NewMimoPsMode);
					}
					else
					{
						break;
					}
				}
				SetSelfMimoPsMode(Adapter, NewMimoPsMode);
			}
			else
			{
				SendMimoPsFrame(pAdapter, pMgntInfo->Bssid, NewMimoPsMode);
				SetSelfMimoPsMode(pAdapter, NewMimoPsMode);
			}
		}
		break;


		case OID_RT_11N_RESET_HISTOGRAM:
			ResetHistogramCounter(pAdapter);
			break;

		case OID_RT_11N_RX_REORDER_CONTROL:
			{
				BOOLEAN 	bRxReorderEnable;
				u1Byte		RxReorderWinSize;
				u1Byte		RxReorderPendingTime; // ms
				
				// Get forced data rate to set.
				bRxReorderEnable = (*((u1Byte*)InformationBuffer)==1)?TRUE:FALSE;

				if(bRxReorderEnable)
				{
					// Verify input paramter.
					if(InformationBufferLength < 3)
					{
						Status = NDIS_STATUS_INVALID_LENGTH;
						*BytesNeeded = 3;
						return Status;
					}

					RxReorderWinSize = *((pu1Byte)InformationBuffer+1);
					if(RxReorderWinSize > 64)
					{
						Status = NDIS_STATUS_INVALID_DATA;
						return Status;
					}

					RxReorderPendingTime = *((pu1Byte)InformationBuffer+2);
					if(RxReorderWinSize == 0)
					{
						Status = NDIS_STATUS_INVALID_DATA;
						return Status;
					}
					
					pMgntInfo->pHTInfo->bCurRxReorderEnable = TRUE;
					pMgntInfo->pHTInfo->RxReorderWinSize = RxReorderWinSize;
					pMgntInfo->pHTInfo->RxReorderPendingTime = RxReorderPendingTime;
				}
				else
				{
					PRX_TS_RECORD pRxTS = (PRX_TS_RECORD) RTGetHeadList(&pMgntInfo->Rx_TS_Admit_List);
					PRX_REORDER_ENTRY pRxReorderEntry;

					PlatformAcquireSpinLock(pAdapter, RT_RX_SPINLOCK);
		
					while(&pRxTS->TsCommonInfo.List != &pMgntInfo->Rx_TS_Admit_List)
					{
						PlatformCancelTimer(pAdapter, &pRxTS->RxPktPendingTimer);
						pRxTS->RxIndicateState = 0;
						while( RTIsListNotEmpty(&pRxTS->RxPendingPktList))
						{
							pRxReorderEntry = (PRX_REORDER_ENTRY)RTRemoveHeadList(&pRxTS->RxPendingPktList);
							pRxTS->RxBatchCount--;
							CountRxStatistics(pAdapter, pRxReorderEntry->pRfd);
							/*if(pAdapter->bInHctTest && 
								pRxReorderEntry->pRfd->FragLength == pRxReorderEntry->pRfd->PacketLength&&
								pAdapter->ForHctTest < 25 )
							{
								pAdapter->ForHctTest++;
								CountRxStatistics(pAdapter, pRxReorderEntry->pRfd);
								DrvIFIndicatePacket(pAdapter, pRxReorderEntry->pRfd);
							}*/ // temp mark if pass DTM
							DrvIFIndicatePacket(pAdapter, pRxReorderEntry->pRfd);
						}
						pRxTS = (PRX_TS_RECORD)RTNextEntryList(&pRxTS->TsCommonInfo.List);
					}

					pMgntInfo->pHTInfo->bCurRxReorderEnable = FALSE;
					PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);
				}
			}
			break;
	
	case OID_RT_11N_DYNAMIC_TX_POWER_CONTROL:       //Dynamic Tx power for near/far range enable/Disable  , by Jacken , 2008-03-06
			if( *((pu1Byte)InformationBuffer) == 0 )
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_11N_DYNAMIC_TX_POWER_CONTROL: disable\n"));
				pMgntInfo->bDynamicTxPowerEnable  = FALSE;
			}
			else
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_11N_DYNAMIC_TX_POWER_CONTROL: enable\n"));
				pMgntInfo->bDynamicTxPowerEnable = TRUE;
			}
		break;	
      case   OID_RT_11N_TX_POWER_TRAINING:
	  		if( *((pu1Byte)InformationBuffer) == 0 )
			{
				pMgntInfo->bDisableTXPowerTraining = TRUE;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_11N_TX_POWER_TRAINING: disable \n"));
			}
			else
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_11N_TX_POWER_TRAINING: enable\n"));
				pMgntInfo->bDisableTXPowerTraining = FALSE;
			}
			//This oid only valid when tx power training function excuted by fw
			// when media connect ,set bSetTXPowerTrainingByOid TRUE ,and dm_RefreshRateAdaptiveMask() 
			//will update H2CCMD TO FW to disable or enable tx power training, 
                     pMgntInfo->bSetTXPowerTrainingByOid = TRUE;			
		break;	

	case   OID_RT_11N_DISABLE_TX_POWER_BY_RATE:
			if( *((pu1Byte)InformationBuffer) == 1 )
			{
				pMgntInfo->bDisableTXPowerByRate= TRUE;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_11N_DISABLE_TX_POWER_BY_RATE: disable \n"));
			}
			else
			{
				pMgntInfo->bDisableTXPowerByRate = FALSE;
			}
	
		 break; 


		
	//added just for debug. Dump all regs' value. by wl.
	case OID_RT_11N_DUMP_REGS:
		{
			if(InformationBufferLength < sizeof(u1Byte))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_11N_DUMP_REGS: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;
			}

			if( *((pu1Byte)InformationBuffer) == 1 )
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Dump regs' value:::\n"));				
				pMgntInfo->bDumpRegs= TRUE;
			}
			
			*BytesRead = InformationBufferLength;
		}
		break;	
        // add for 92d rx batch indicate	
	case OID_RT_11N_SET_RX_FAST_BATCH_NUM:
		{
			if(InformationBufferLength < sizeof(u1Byte))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_11N_DUMP_REGS: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;
			}
		
			pMgntInfo->NumforRxFastbatch= *((pu1Byte)InformationBuffer);
			if( pMgntInfo->NumforRxFastbatch<1 )
			{
				
				pMgntInfo->NumforRxFastbatch = 1;
			}
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set RX fast batch Indication num:%d.\n",pMgntInfo->NumforRxFastbatch));				
			
			*BytesRead = InformationBufferLength;
		}
		break;
			
	case OID_RT_11N_TDLS_ENABLE:
		{
			TDLS_SetConfiguration(pAdapter, InformationBuffer, (u1Byte)InformationBufferLength);
		}
		break;

	case OID_RT_11N_SIGMA_CONFIG:
		{
			MgntActSet_802_11_Sigma_Capability(pAdapter, InformationBuffer, (u1Byte)InformationBufferLength);
		}
		break;
	case OID_RT_BAND_SELECT:
		{
			pMgntInfo->RegPreferBand = *((pu2Byte)InformationBuffer);
			RT_TRACE(COMP_OID_SET,DBG_LOUD,("RegPreferBand=%d\n", pMgntInfo->RegPreferBand));
		}
		break;	
	
	case OID_RT_CUSTOMER_WOW_S5_INFO:
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD,("===> OID_RT_CUSTOMER_WOW_S5_INFO \n"));
			if( InformationBufferLength < sizeof(RT_S5WakeUPInfo) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(RT_S5WakeUPInfo);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_CUSTOMER_WOW_S5_INFO: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;
			}
			MgntActSet_S5_WAKEUP_INFO(pAdapter, (pu1Byte)InformationBuffer , InformationBufferLength);
		}
	break;
	
	case OID_RT_CUSTOMER_WOW_S5_SUPPORT :
		{
			ULONG		bEnableS5;
			if( InformationBufferLength < sizeof(ULONG) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(ULONG);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_CUSTOMER_WOW_S5_SUPPORT: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;
			}

			bEnableS5 =*((PULONG)InformationBuffer);

			if( bEnableS5 == 1 )
			{
				pMgntInfo->bReceiveSystemPSOID = TRUE;
				RT_TRACE(COMP_TEST, DBG_LOUD, ("OID_RT_CUSTOMER_WOW_S5_SUPPORT enable S5\n"));
			}
			else
			{
				pMgntInfo->bReceiveSystemPSOID = FALSE;
				RT_TRACE(COMP_TEST, DBG_LOUD, ("OID_RT_CUSTOMER_WOW_S5_SUPPORT disenable S5\n"));
			}
			
		}
	break;
	
	}

	return Status;
}

// This is to offload the N6CSetInformation() since the huge size of the funcion stack causes the BSOD when DbgPrint.
static NDIS_STATUS
N6CSetInformationHandleCustomizedSecurityOids(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
	)
{
	PADAPTER				pAdapter = (PADAPTER)MiniportAdapterContext;
	PMGNT_INFO      			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;
	
	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;

	case OID_802_11_ADD_KEY:
	case OID_RT_802_11_ADD_KEY:
		RT_TRACE( COMP_OID_SET,  DBG_WARNING, ("OID_802_11_ADD_KEY Setting (Warning: Old OID in NDIS5, do nothing)\n") );
		{
			PNDIS_802_11_KEY	key = (PNDIS_802_11_KEY)InformationBuffer;
			RT_ENC_ALG			EncAlgorithm;
			BOOLEAN				IsGroup = FALSE;
			BOOLEAN				IsGroupTransmitKey = FALSE;
			pu1Byte				pKeyRSC = (pu1Byte)&key->KeyRSC;
	
			RT_TRACE(COMP_OID_SET, DBG_LOUD,("===> Set OID_802_11_ADD_KEY, key->Length=%d\n", key->Length ));
			RT_PRINT_DATA(COMP_OID_SET, DBG_TRACE, "OID_802_11_ADD_KEY InformationBuffer", InformationBuffer, key->Length );

			// For WPA Verify.
			if(key->KeyIndex == 0xc0000001)
			{
				RT_TRACE(COMP_SEC, DBG_LOUD,("[Note] KeyIndex is 0xc0000001\n"));
			
				pNdisCommon->RegWepEncStatus = pNdisCommon->WepEncStatusBeforeWpaVerify;
				pNdisCommon->RegEncAlgorithm = pNdisCommon->EncAlgorithmBeforeWpaVerify;
				pNdisCommon->RegAuthentAlg = pNdisCommon->AuthentAlgBeforeWpaVerify;
	
				N6RestoreLastInitSetting(pAdapter);
			}

			if(key->KeyIndex & 0x40000000)
			{ // Pairwise key
				RT_TRACE(COMP_SEC, DBG_LOUD,("OID_802_11_ADD_KEY: +++++ Pairwise key +++++\n"));

				if( !(key->KeyIndex & 0x80000000))
				{ //2004/08/03, kcwu, refer to MS design note
					RT_TRACE(COMP_SEC, DBG_LOUD,("<=== SetInfo, OID_802_11_ADD_KEY: !(key->KeyIndex & 0x80000000)\n"));
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;
				}
				
				if(key->KeyIndex & 0x0fffffff)
				{
					//kcwu:
					//WinXP Design Note: The key index is specified in the lower 8 bits by values of zero to 255.
					//The key index should be set to zero for a Pairwise key, and the driver should fail with
					//NDIS_STATUS_INVALID_DATA if the lower 8 bits is not zero
					RT_TRACE(COMP_SEC, DBG_LOUD,("<=== SetInfo, OID_802_11_ADD_KEY: key->KeyIndex & 0x0fffffff\n"));
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;
				}

				if(MacAddr_isBcst(key->BSSID))
				{
					RT_TRACE(COMP_SEC, DBG_LOUD,("<=== SetInfo, OID_802_11_ADD_KEY: MacAddr_isBcst(key->BSSID)\n"));
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;
				}

				// Check key length for TKIP.
				if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_TKIP && key->KeyLength != 32)
				{
					RT_TRACE(COMP_SEC, DBG_LOUD,("<=== SetInfo, OID_802_11_ADD_KEY: TKIP KeyLength:%d != 32\n", key->KeyLength));
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;
				}

				// Check key length for AES.
				if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP && key->KeyLength != 16)
				{
					// For our supplicant, EAPPkt9x.vxd, cannot differentiate TKIP and AES case.
					// 2005.10.17, by rcnjko.
					if(key->KeyLength == 32)
					{
						key->KeyLength = 16; 
					}
					else
					{
						Status = NDIS_STATUS_INVALID_DATA;
						return Status;
					}
				}

				// Check key length for WEP. For NDTEST, 2005.01.27, by rcnjko.
				if(	
					(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 && key->KeyLength != 5) || 
					(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104 && key->KeyLength != 13)
				)
				{
					RT_TRACE(COMP_SEC, DBG_LOUD,("<=== SetInfo, OID_802_11_ADD_KEY: WEP KeyLength:%d != 5 or 13\n", key->KeyLength));
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;
				}

				EncAlgorithm = pMgntInfo->SecurityInfo.PairwiseEncAlgorithm;
				IsGroup = FALSE;

				// Check the pairwise key. Added by Annie, 2005-07-06.
				{
					RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("[Pairwise Key set]\n") );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("key index: 0x%x\n", key->KeyIndex) );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("key Length: 0x%x\n", key->KeyLength) );
					RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "Key Material:", key->KeyMaterial, key->KeyLength );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
				}
			}
			else
			{ // Group key
				RT_TRACE(COMP_SEC, DBG_LOUD,("OID_802_11_ADD_KEY: +++++ Group key +++++\n"));
				
				if(pMgntInfo->mIbss && !MacAddr_isBcst(key->BSSID))
				{
					RT_TRACE(COMP_SEC, DBG_LOUD,("<=== SetInfo, OID_802_11_ADD_KEY: pMgntInfo.mIbss && !MacAddr_isBcst(key->BSSID)\n"));
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;
				}

				// Check key length for TKIP
				if(pMgntInfo->SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_TKIP && key->KeyLength != 32)
				{
					if( pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_NONE )
					{
						// [Note] OID oder of Window Zero-Config for WEP setting is as follows,
						//	Step 0. 0xc0000001 and RestoreLastInitSetting().
						//	Step 1. OID_802_11_INFRASTRUCTURE_MODE
						//	Step 2. OID_802_11_AUTHENTICATION_MODE
						//	Step 3. OID_802_11_ADD_KEY
						//	Step 4. OID_802_11_ENCRYPTION_STATUS
						//	Step 5. OID_802_11_SSID
						//
						// If we are connecting to WPA or WPA2 AP by UI,
						// and then use WZC to connect to a WEP AP,
						// we'll add key with AuthMode Open and EncryptionStatus TKIP/AES at step 3.
						// We should not reject the WEP key with length 5 or 13 in this case.
						//
						// - Added by Annie, 2006-05-10. -
						
						RT_TRACE( COMP_SEC, DBG_LOUD, ("OID_802_11_ADD_KEY[Group]: Not to Check key length 32 for TKIP because we'e in RT_SEC_LVL_NONE.\n") );
					}
					else
					{
						RT_TRACE(COMP_SEC, DBG_LOUD,("<=== SetInfo, OID_802_11_ADD_KEY: TKIP GTK KeyLength:%d != 32\n", key->KeyLength));
						Status = NDIS_STATUS_INVALID_DATA;
						return Status;
					}
				}

				// Check key length for AES
				// For NDTEST, we allow keylen=32 in this case. 2005.01.27, by rcnjko.
				else if(pMgntInfo->SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_AESCCMP  && 
						(key->KeyLength != 16 && key->KeyLength != 32) ) 
				{
					if( pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_NONE )
					{
						RT_TRACE( COMP_SEC, DBG_LOUD, ("OID_802_11_ADD_KEY[Group]: Not to Check key length 16 or 32 for AES because we'e in RT_SEC_LVL_NONE.\n") );
					}
					else
					{
						RT_TRACE(COMP_SEC, DBG_LOUD,("<=== SetInfo, OID_802_11_ADD_KEY: AES GTK KeyLength:%d != 16 or 32\n", key->KeyLength));
						Status = NDIS_STATUS_INVALID_DATA;
						return Status;
					}
				}

				if(key->KeyIndex & 0x8000000)
				{
					IsGroupTransmitKey = TRUE;
				}

				if(pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC)
				{
					IsGroupTransmitKey = TRUE;
				}

				EncAlgorithm = pMgntInfo->SecurityInfo.GroupEncAlgorithm;
				IsGroup = TRUE;

				// Added for WPA2-PSK. added by Annie, 2005-09-23.
				// Moved by Bruce to pass PMK of WMM WPA2 test, by Bruce, 2007-11-07. 
				PlatformRequestPreAuthentication( pAdapter, PRE_AUTH_INDICATION_REASON_ASSOCIATION );

				// Check the group key. Added by Annie, 2005-07-06.
				{
					RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("[Group Key set]\n") );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("key index: 0x%x\n", key->KeyIndex) );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("key Length: 0x%x\n", key->KeyLength) );
					RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "Key Material:", key->KeyMaterial, key->KeyLength );
					RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
				}
			}

			if(!MgntActSet_802_11_ADD_KEY(
						pAdapter,
						EncAlgorithm,
						key->KeyIndex,
						key->KeyLength,
						key->KeyMaterial,
						key->BSSID,
						IsGroupTransmitKey,
						IsGroup,
						key->KeyRSC))
			{
				Status = NDIS_STATUS_INVALID_DATA;
			}
			
			return Status;
		}
		break;

	case OID_802_11_ADD_WEP:
	case OID_RT_802_11_ADD_WEP:
		RT_TRACE( COMP_OID_SET,  DBG_WARNING, ("OID_802_11_ADD_WEP Setting (Warning: Old OID in NDIS5, do nothing)\n") );
		{
			NDIS_802_11_WEP	*key;
			RT_ENC_ALG		EncAlgorithm;
			u4Byte			KeyIndex;
			u4Byte			KeyLength;
			BOOLEAN			IsDefaultKey;
			pu1Byte			KeyMaterial;
			BOOLEAN			IsDefaultKeyId;

			RT_TRACE(COMP_SEC, DBG_LOUD, ("Set OID_802_11_ADD_WEP\n"));
			
			key = (NDIS_802_11_WEP *)InformationBuffer;
			KeyIndex = key->KeyIndex;
			KeyLength = key->KeyLength;
			KeyMaterial = key->KeyMaterial;
			//PlatformZeroMemory(KeyMaterial+KeyLength, 32-KeyLength);
			IsDefaultKey = ( KeyIndex & 0x80000000 ) ? TRUE : FALSE;
			KeyIndex &= 0x7fffffff;

			RT_TRACE( COMP_SEC, DBG_TRACE, ("\tKeyIndex = %d\n", KeyIndex) );
			RT_TRACE( COMP_SEC, DBG_TRACE, ("\tKeyLength = %d\n", KeyLength) );
			RT_TRACE( COMP_SEC, DBG_TRACE, ("\tKeyMaterial = %13s\n", KeyMaterial) );
			RT_TRACE( COMP_SEC, DBG_TRACE, ("\tIsDefaultKey = %d\n", IsDefaultKey) );
			
			if( (KeyLength == 5) && (KeyIndex < 4) )
			{
				EncAlgorithm = RT_ENC_ALG_WEP40;
				// For WPA Verify.
				pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_WEP64; 
				PlatformMoveMemory((PVOID)&(pNdisCommon->RegDefaultKeyBuf[KeyIndex]), (PVOID)KeyMaterial, KeyLength);
			}
			else if( (KeyLength == 13) && (KeyIndex < 4) )
			{
				EncAlgorithm = RT_ENC_ALG_WEP104;
				// For WPA Verify.
				pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_WEP128; 
				PlatformMoveMemory((PVOID)&(pNdisCommon->RegDefaultKeyWBuf[KeyIndex]), (PVOID)KeyMaterial, KeyLength);
			}
			else
			{
				Status = NDIS_STATUS_INVALID_DATA;
				RT_TRACE( COMP_SEC, DBG_LOUD, ("[WARNING!!!] OID_802_11_ADD_WEP: return NDIS_STATUS_INVALID_DATA!! KeyIndex=%d, KeyLength=%d\n", KeyIndex, KeyLength) );
				return Status;
			}

			IsDefaultKeyId =( IsDefaultKey && (KeyIndex < 4) ) ?  TRUE : FALSE;
			MgntActSet_802_11_ADD_WEP(
					pAdapter,
					EncAlgorithm,
					KeyIndex,
					KeyLength,
					KeyMaterial,
					IsDefaultKeyId
			);

			*BytesRead = InformationBufferLength;
		}
		break;

	case OID_802_11_REMOVE_WEP:
	case OID_RT_802_11_REMOVE_WEP:
		RT_TRACE( COMP_OID_SET,  DBG_WARNING, ("OID_802_11_REMOVE_WEP Setting (Warning: Old OID in NDIS5, do nothing)\n") );
		{
			RT_ENC_ALG		EncAlgorithm;
			u4Byte			*KeyIndex;
			u4Byte			KeyLength;

			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_REMOVE_WEP\n"));

			KeyIndex = (u4Byte *)InformationBuffer;
			(*KeyIndex) &= 0x3fffffff;	// clear default ket bit
			EncAlgorithm = pMgntInfo->SecurityInfo.PairwiseEncAlgorithm;
			KeyLength = ( EncAlgorithm == RT_ENC_ALG_WEP40 ) ? 5 : 13;

			MgntActSet_802_11_REMOVE_WEP(
					pAdapter,
					EncAlgorithm,
					*KeyIndex,
					KeyLength
			);

			*BytesRead = sizeof(u4Byte);
		}
		break;

	case OID_802_11_REMOVE_KEY:
	case OID_RT_802_11_REMOVE_KEY:
		RT_TRACE( COMP_OID_SET,  DBG_WARNING, ("OID_802_11_REMOVE_KEY Setting (Warning: Old OID in NDIS5, do nothing)\n") );
		{
			PNDIS_802_11_REMOVE_KEY	key = (PNDIS_802_11_REMOVE_KEY)InformationBuffer;
			BOOLEAN					IsGroup = (key->KeyIndex & 0x4000000)?FALSE:TRUE;
			RT_ENC_ALG				EncAlgorithm = (IsGroup)?pMgntInfo->SecurityInfo.GroupEncAlgorithm:pMgntInfo->SecurityInfo.PairwiseEncAlgorithm;
			
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_REMOVE_KEY\n"));


			//if(key->KeyIndex & 0xbfffff00){
			// Lower 8 bits => 0~255
			// BIT30 => 1: Pairwise
			//                0: Group
			// Other bits => 0
			
			if(key->KeyIndex & 0xbffffffc)
			{
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;
			}
			MgntActSet_802_11_REMOVE_KEY(
						pAdapter,
						EncAlgorithm,
						key->KeyIndex,
						key->BSSID,
						IsGroup);
		}
		break;

	//case OID_802_11_WEP_STATUS:
	case OID_802_11_ENCRYPTION_STATUS:
	case OID_RT_802_11_ENCRYPTION_STATUS:
		RT_TRACE( COMP_OID_SET,  DBG_WARNING, ("OID_802_11_ENCRYPTION_STATUS Setting (Warning: Old OID in NDIS5, do nothing)\n") );
		if( (InformationBuffer == 0) || (InformationBufferLength < sizeof(NDIS_802_11_WEP_STATUS)) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			*BytesNeeded = sizeof(NDIS_802_11_WEP_STATUS);
			break;
		}

		// NDIS_802_11_ENCRYPTION_STATUS should be the same as RT_802_11_ENCRYPTION_STATUS
		// which is defined by KcWu. 2004.09.23, by rcnjko. 
		pMgntInfo->SecurityInfo.EncryptionStatus = *(RT_802_11_ENCRYPTION_STATUS*)InformationBuffer;
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_ENCRYPTION_STATUS: %d\n", *(PNDIS_802_11_ENCRYPTION_STATUS)InformationBuffer));

		// For WPA Verify.
		pNdisCommon->WepEncStatusBeforeWpaVerify = pNdisCommon->RegWepEncStatus;
		pNdisCommon->EncAlgorithmBeforeWpaVerify = pNdisCommon->RegEncAlgorithm;

		// For CKIP which can only use software encryption/decryption. Added by Annie, 2006-08-14.
		if( IsCkipEnabled(pMgntInfo) )
		{ // CKIP case.
			SecSetSwEncryptionDecryption(pAdapter, TRUE, TRUE);
		}
		else
		{ // WEP or other case.
			SecSetSwEncryptionDecryption(pAdapter, FALSE, FALSE);
		}
		//

		switch(*(PNDIS_802_11_ENCRYPTION_STATUS)InformationBuffer)
		{
		case Ndis802_11EncryptionDisabled:
			RT_TRACE(COMP_SEC, DBG_LOUD, ("\tNdis802_11EncryptionDisabled\n"));
			//MgntActSet_802_11_REMOVE_WEP( Adapter, RT_ENC_ALG_NO_CIPHER, 0);
			pMgntInfo->SecurityInfo.PairwiseEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
			pMgntInfo->SecurityInfo.GroupEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
			pMgntInfo->SecurityInfo.UseDefaultKey = FALSE;
			pMgntInfo->SecurityInfo.EncryptionHeadOverhead = 0;
			pMgntInfo->SecurityInfo.EncryptionTailOverhead = 0;
			// For WPA Verify.
			pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_NO_WEP;
			pNdisCommon->RegEncAlgorithm = REG_NONE_Encryption;
			break;

		case Ndis802_11Encryption1Enabled:
			//TODO: Enable WEP
			RT_TRACE(COMP_SEC, DBG_LOUD, ("\tNdis802_11Encryption1Enabled\n"));
			pMgntInfo->SecurityInfo.PairwiseEncAlgorithm = RT_ENC_ALG_WEP40;
			pMgntInfo->SecurityInfo.GroupEncAlgorithm = RT_ENC_ALG_WEP40;
			pMgntInfo->SecurityInfo.UseDefaultKey = TRUE;
			pMgntInfo->SecurityInfo.EncryptionHeadOverhead = WEP_IV_LEN;
			pMgntInfo->SecurityInfo.EncryptionTailOverhead = WEP_ICV_LEN;
			// For WPA Verify.
			// WEP64 or WEP128 should be determined in OID_802_11_ADD_WEP later. 
			// 2004.11.24, by rcnjko.
			pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_WEP64; 
			pNdisCommon->RegEncAlgorithm = REG_WEP_Encryption;

			break;

		case Ndis802_11Encryption2Enabled:
			//TODO: Enable WPA
			RT_TRACE(COMP_SEC, DBG_LOUD, ("\tNdis802_11Encryption2Enabled\n"));
			pMgntInfo->SecurityInfo.PairwiseEncAlgorithm = RT_ENC_ALG_TKIP;
			pMgntInfo->SecurityInfo.GroupEncAlgorithm = RT_ENC_ALG_TKIP;
			//2004/08/13, kcwu, I think it should be correct
			pMgntInfo->SecurityInfo.UseDefaultKey = (pMgntInfo->Regdot11networktype==RT_JOIN_NETWORKTYPE_ADHOC && !(ACTING_AS_AP(pAdapter) ))?TRUE:FALSE;
			pMgntInfo->SecurityInfo.EncryptionHeadOverhead = EXT_IV_LEN;
			//2004/06/25, kcwu_comment, The EncryptionTailOverhead only includes WEP IV, and doesn't include TKIP MIC
			pMgntInfo->SecurityInfo.EncryptionTailOverhead = WEP_ICV_LEN;
			// For WPA Verify.
			pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_NO_WEP; 
			pNdisCommon->RegEncAlgorithm = REG_TKIPv2_Encryption;
			// Emily. 
			// [AnnieWorkaround] Default value changed from CAM_AES to CAM_TKIP by Annie for solving WPA-none problem.
			// In Ad-Hoc mode, use encryption TKIP (authmode=WPA-None).
			// If upper layer is windows zero-config, OID_802_11_ADD_KEY is called before OID_802_11_ENCRYPTION_STATUS.
			// In this case, we'll first execute RestoreLastInitSetting() in OID_802_11_ADD_KEY and EncAlgo will be RT_ENC_ALG_NO_CIPHER.
			// At least, If we go into this case, we set default EncAlgo as CAM_TKIP to config correct CAM.
			// - Annie, 2005-08-16.
			if( (pNdisCommon->EncAlgorithmBeforeWpaVerify!=REG_WAPI_PSK) || FALSE == WAPI_QuerySetVariable(pAdapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0))
			{//Temply add to  solve UI Switch problem which cause wapi msk tx key invalid   zhiyuan 2009/12/22				

			        if(pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC && !(ACTING_AS_AP(pAdapter) ))
			        {
				        u1Byte CAM_CONST_BROAD[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
				        pAdapter->HalFunc.SetKeyHandler(pAdapter, 
												0,					//KeyIndex,
												CAM_CONST_BROAD,
												TRUE,				//IsGroup,
												RT_ENC_ALG_TKIP,
												FALSE,
												FALSE);
			        }
			}

			break;

		case Ndis802_11Encryption3Enabled:
#if 1	// Opened for AES, by Annie, 2005-09-14.
			RT_TRACE(COMP_SEC, DBG_LOUD, ("\tNdis802_11Encryption3Enabled\n"));
			pMgntInfo->SecurityInfo.PairwiseEncAlgorithm = RT_ENC_ALG_AESCCMP;
			pMgntInfo->SecurityInfo.GroupEncAlgorithm = RT_ENC_ALG_AESCCMP;
			pMgntInfo->SecurityInfo.AESCCMPMicLen = 8;
			//2004/09/22, kcwu
			pMgntInfo->SecurityInfo.UseDefaultKey = (pMgntInfo->Regdot11networktype==RT_JOIN_NETWORKTYPE_ADHOC&& !(ACTING_AS_AP(pAdapter) ))?TRUE:FALSE;
			pMgntInfo->SecurityInfo.EncryptionHeadOverhead = EXT_IV_LEN;
			pMgntInfo->SecurityInfo.EncryptionTailOverhead = AES_MIC_LEN;
			// For WPA Verify.
			pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_NO_WEP; 
			pNdisCommon->RegEncAlgorithm = REG_AESCCMP_Encryption;
#else
			// For HCT 12.1 test if we have not yet implemented AES, 2005.07.11, by rcnjko.
			Status = NDIS_STATUS_NOT_SUPPORTED;
			return Status;
#endif
			// Emily. 
			// [AnnieWorkaround] Default value changed from CAM_AES to CAM_TKIP by Annie for solving WPA-none problem.
			// In Ad-Hoc mode, use encryption TKIP (authmode=WPA-None).
			// If upper layer is windows zero-config, OID_802_11_ADD_KEY is called before OID_802_11_ENCRYPTION_STATUS.
			// In this case, we'll first execute RestoreLastInitSetting() in OID_802_11_ADD_KEY and EncAlgo will be RT_ENC_ALG_NO_CIPHER.
			// At least, If we go into this case, we set default EncAlgo as CAM_TKIP to config correct CAM.
			// - Annie, 2005-08-16.
			if( (pNdisCommon->EncAlgorithmBeforeWpaVerify!=REG_WAPI_PSK) || FALSE == WAPI_QuerySetVariable(pAdapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0))
			{//Temply add to  solve UI Switch problem which cause wapi msk tx key invalid   zhiyuan 2009/12/22				
			        if(pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC && !(ACTING_AS_AP(pAdapter) ))
			        {
				        u1Byte CAM_CONST_BROAD[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
				        pAdapter->HalFunc.SetKeyHandler(pAdapter, 
												0,					//KeyIndex,
												CAM_CONST_BROAD,
												TRUE,				//IsGroup,
												RT_ENC_ALG_AESCCMP,
												FALSE,
												FALSE);
			        }
                        }
			break;

		case Wapi_Encryption:
		case Wapi_Certificate:
			WAPI_SecFuncHandler(WAPI_SETAUTHENCRYPTSTATE,pAdapter,(PVOID)&Oid,InformationBuffer);
			break;
			
		default:
			RT_TRACE(COMP_SEC, DBG_LOUD, ("\tdefault\n"));
			pMgntInfo->SecurityInfo.PairwiseEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
			pMgntInfo->SecurityInfo.GroupEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
			pMgntInfo->SecurityInfo.UseDefaultKey = FALSE;
			pMgntInfo->SecurityInfo.EncryptionHeadOverhead = 0;
			pMgntInfo->SecurityInfo.EncryptionTailOverhead = 0;
			// For WPA Verify.
			pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_NO_WEP;
			pNdisCommon->RegEncAlgorithm = REG_NONE_Encryption;
			break;
		}

		// Switch HW encryption/decryption mode.
		if( pAdapter->bInHctTest == 1 )
		{
			pAdapter->HalFunc.DisableHWSecCfgHandler(pAdapter);
			SecSetSwEncryptionDecryption( pAdapter , TRUE , TRUE );
		}
		else
		{
			if( pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC &&
			     MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_NONE )
			{
				RT_TRACE(COMP_SEC , DBG_LOUD , ("==>Win7 we used SW in AD-HOT \n"));			
				SecSetSwEncryptionDecryption( pAdapter , TRUE , TRUE );
			}
			else if( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104 ||
				     pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40)
			{
				RT_TRACE(COMP_SEC , DBG_LOUD , ("==>Win7 we used SW in WEP \n"));
				SecSetSwEncryptionDecryption( pAdapter , TRUE , TRUE );
			}
			else
			{
				RT_TRACE(COMP_SEC , DBG_LOUD , ("==>Win7 we used HW in default !! \n"));
				SecSetSwEncryptionDecryption( pAdapter , FALSE , FALSE );
			}
				
	 		pAdapter->HalFunc.EnableHWSecCfgHandler(pAdapter);
		}

		// Update mCap.
		if( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm != RT_ENC_ALG_NO_CIPHER )
		{
			pMgntInfo->mCap |= cPrivacy;
		}
		else
		{
			pMgntInfo->mCap &= ~cPrivacy;
		}

		if(pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC
			&& pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPANone
			&& pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40)
		{

			MgntIndicateMediaStatus( pAdapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );
			break;
		}

		// Update STA's RSNIE.
		if( pMgntInfo->SecurityInfo.EncryptionStatus !=Wapi_Encryption && pMgntInfo->SecurityInfo.EncryptionStatus != Wapi_Certificate) 
			SecConstructRSNIE(pAdapter);

		*BytesRead = InformationBufferLength;
		break;

	case OID_802_11_ASSOCIATION_INFORMATION:
	case OID_RT_802_11_ASSOCIATION_INFORMATION:
		RT_TRACE( COMP_OID_SET,  DBG_WARNING, ("OID_802_11_ASSOCIATION_INFORMATION Setting (Warning: Old OID in NDIS5, do nothing)\n") );
		if( (InformationBuffer == 0) || (InformationBufferLength < 4) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			return Status;
		}
		else
		{
			PRT_SECURITY_T pSecInfo = &(pMgntInfo->SecurityInfo);
			PNDIS_802_11_ASSOCIATION_INFORMATION pAssocInfo = (PNDIS_802_11_ASSOCIATION_INFORMATION)InformationBuffer;
	
			if( pAssocInfo->RequestIELength != 0)
			{
				// do not copy field [Element id] and [Length] 
				PlatformMoveMemory(
						pSecInfo->RSNIE.Octet, 
						(pu1Byte)InformationBuffer + pAssocInfo->OffsetRequestIEs + 2,
						pAssocInfo->RequestIELength - 2);
				pSecInfo->RSNIE.Length = (u1Byte)pAssocInfo->RequestIELength - 2;
				RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "OID RSNIE Data:\n", pSecInfo->RSNIE.Octet, pSecInfo->RSNIE.Length);
			}
		}
		*BytesRead = InformationBufferLength;
		break;

	case OID_802_11_PMKID:
	case OID_RT_802_11_PMKID:
		RT_TRACE( COMP_OID_SET,  DBG_WARNING, ("OID_802_11_PMKID Setting (Warning: Old OID in NDIS5, do nothing)\n") );
		if( (InformationBuffer == 0) || (InformationBufferLength < 4) )
		{
			return NDIS_STATUS_INVALID_LENGTH;
		}
		else
		{
			MgntActSet_802_11_PMKID( pAdapter, (pu1Byte)InformationBuffer, InformationBufferLength );
		}
		break;

    case OID_802_11_AUTHENTICATION_MODE:
	case OID_RT_802_11_AUTHENTICATION_MODE:
		RT_TRACE( COMP_OID_SET,  DBG_WARNING, ("OID_802_11_AUTHENTICATION_MODE Setting (Warning: Old OID in NDIS5, do nothing)\n") );
		{
			RT_AUTH_MODE	authmode;

			if( (InformationBuffer == 0) || (InformationBufferLength < sizeof(NDIS_802_11_AUTHENTICATION_MODE )) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(NDIS_802_11_AUTHENTICATION_MODE );
				return Status;
			}
			
			authmode = *(RT_AUTH_MODE*)InformationBuffer;

			// For WPA Verify.
			pNdisCommon->AuthentAlgBeforeWpaVerify = pNdisCommon->RegAuthentAlg;
			pNdisCommon->RegAuthentAlg = (int)authmode;

			if((authmode != RT_802_11AuthModeWAPI) && (authmode != RT_802_11AuthModeCertificateWAPI))
			{
				SecSetSwEncryptionDecryption(pAdapter,FALSE,FALSE);
			}

			switch(authmode)
			{
			case RT_802_11AuthModeOpen:
			case RT_802_11AuthModeShared:
					pMgntInfo->SecurityInfo.SecLvl = RT_SEC_LVL_NONE;
//					pMgntInfo->SecurityInfo.PairwiseEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
//					SecClearAllKeys(pAdapter);
					RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_AUTHENTICATION_MODE: %d (0:Open, 1:Shared-Key)\n", authmode ));
					break;

			case RT_802_11AuthModeAutoSwitch:
					// Do the same setting as Open and Shared. 2005.03.08, by rcnjko.
					pMgntInfo->SecurityInfo.SecLvl = RT_SEC_LVL_NONE;
//					pMgntInfo->SecurityInfo.PairwiseEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
//					SecClearAllKeys(pAdapter);
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_AUTHENTICATION_MODE: Ndis802_11AuthModeAutoSwitch\n"));
					break;

			case RT_802_11AuthModeWPA:
			case RT_802_11AuthModeWPAPSK:
			case RT_802_11AuthModeWPANone:
					pMgntInfo->SecurityInfo.SecLvl = RT_SEC_LVL_WPA;
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_AUTHENTICATION_MODE: %d: RT_SEC_LVL_WPA\n", authmode ));
					break;

			case RT_802_11AuthModeWPA2:
			case RT_802_11AuthModeWPA2PSK:
					pMgntInfo->SecurityInfo.SecLvl = RT_SEC_LVL_WPA2;
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_AUTHENTICATION_MODE: %d: RT_SEC_LVL_WPA2\n", authmode ));
					break;

			case RT_802_11AuthModeWAPI:
			case  RT_802_11AuthModeCertificateWAPI:
					WAPI_SecFuncHandler(WAPI_SETAUTHENCRYPTSTATE,pAdapter,(PVOID)&Oid,InformationBuffer);
					break;
			default:
					Status = NDIS_STATUS_INVALID_DATA;
					break;
			}

			if(Status==NDIS_STATUS_SUCCESS)
				MgntActSet_802_11_AUTHENTICATION_MODE( pAdapter, authmode );

			if(authmode != RT_802_11AuthModeWAPI && authmode != RT_802_11AuthModeCertificateWAPI)
				SecConstructRSNIE(pAdapter);

			*BytesRead = sizeof(NDIS_802_11_AUTHENTICATION_MODE );
		}
		break;

	case OID_802_11_RELOAD_DEFAULTS:
	case OID_RT_802_11_RELOAD_DEFAULTS:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_RELOAD_DEFAULTS: \n"));
		if( (InformationBuffer == 0) || (InformationBufferLength < sizeof(NDIS_802_11_RELOAD_DEFAULTS)) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			*BytesNeeded = sizeof(NDIS_802_11_RELOAD_DEFAULTS);
			break;
		}
		else
		{
			RT_802_11_RELOAD_DEFAULTS NdisReloadDefault = *(PRT_802_11_RELOAD_DEFAULTS)InformationBuffer;

			RT_TRACE(COMP_OID_SET, DBG_LOUD,("OID_RELOAD_DEFAULT\n"));

			SecClearAllKeys(pAdapter);

			switch( NdisReloadDefault)
			{
			case RT802_11ReloadWEPKeys:
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SetInfo, OID_802_11_RELOAD_DEFAULTS:Ndis802_11ReloadWEPKeys\n"));
				break;
			}
		}
		*BytesRead = sizeof(RT_802_11_RELOAD_DEFAULTS);
		break;

	case OID_RT_SET_DEFAULT_KEY_ID:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SET_DEFAULT_KEY_ID.\n"));
		{
			u2Byte KeyId;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(KeyId))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(KeyId);
				return Status;
			}
	
			// Get default key id to set.
			KeyId = *((u2Byte*)InformationBuffer);

			// Set default key id: 0 ~ 3.
			KeyId &= 0x3;
			// The following code will cause 8187 failed to link to AP.	
			pMgntInfo->SecurityInfo.DefaultTransmitKeyIdx = (u1Byte)KeyId;
			pNdisCommon->RegDefaultKeyId = (u1Byte)KeyId;
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SET_DEFAULT_KEY_ID.\n"));
		break;
		

	}
	return Status;
}

static NDIS_STATUS
N6CSetInformationHandleCustomizedNANOids(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
	)
{
	PADAPTER				pAdapter = (PADAPTER)MiniportAdapterContext;
	PMGNT_INFO      		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;
	
	switch(Oid)
	{		
#if (NAN_SUPPORT == 1)	
	case OID_RT_NAN_TEST: 
		{

			u4Byte cmd = 0;
			u4Byte ID = 0, Port = 0, cmdid = 0, Para = 0;
			RT_TRACE(COMP_NAN, DBG_LOUD, ("===> Set OID_RT_NAN_TEST CMD = 0x%X, len = %d\n", *((pu4Byte)InformationBuffer), InformationBufferLength));

			if (InformationBufferLength == sizeof(u4Byte)) { 
				cmd = *((pu4Byte)InformationBuffer);
				ID = cmd & 0xFF;
				Port = (cmd & 0xFF00) >> 8;
				cmdid = (cmd & 0xFF0000) >> 16;
				Para = (cmd & 0xFF000000) >> 24;

				RT_TRACE(COMP_NAN, DBG_LOUD, ("===> ID=0x%X, Port=0x%X, cmdid=0x%X, Para=0x%X\n", ID,Port,cmdid,Para));
				
				if (ID == 0x29)
					NAN_DbgCtrlOperation(pAdapter, Port, cmdid, Para);
				else if (ID == 0x30)
					NAN_DbgCtrlState(pAdapter, Para);
				else if (ID == 0x31)
					NAN_DbgCtrlVariable(pAdapter, Port, cmdid, Para);

			} else {
				return NDIS_STATUS_SUCCESS;
			}

			RT_TRACE(COMP_NAN, DBG_LOUD, ("<=== Set OID_RT_NAN_TEST CMD = 0x%X\n", cmd));
		}
		break;
		
	case OID_RT_NAN_SDF: 
		{
			RT_TRACE(COMP_NAN, DBG_LOUD, ("===> Set OID_RT_NAN_SDF, len = %d\n", InformationBufferLength));
			{
				PNAN_INFO 		 pNanInfo = GET_NAN_INFO(pAdapter);
				u2Byte			 idx, u2bPacketLength, u2bTxNumber;
				u1Byte			 u1bTxRate;
				OCTET_STRING	 ocPacketToSend;
				NAN_SDF_CONTEXT 		FakeSDFCtx;
				
				if (InformationBufferLength < 4){
					u1Byte Dest[6] = {0x00, 0xE0, 0x4c, 0x81, 0x88, 0xbf};
					u1Byte Dest_b[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
					u1Byte ServiceID[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
					u1Byte tmp = *((pu1Byte)InformationBuffer);
					RT_TRACE(COMP_NAN, DBG_LOUD, ("Make a Fake SDF Frame to Test\n"));

					PlatformZeroMemory(&pNanInfo->SDFCtx.SDAttibute,sizeof(NAN_SD_ATTRIBUTE));
					PlatformZeroMemory(&pNanInfo->SDFCtx.ConfigPara,sizeof(NAN_SDF_CONFIG_PARA));

					if (tmp != 0xff){
						Dest[5] = tmp;
						PlatformMoveMemory(pNanInfo->SDFCtx.PeerAddr, Dest, 6);
					}
					else
						PlatformMoveMemory(pNanInfo->SDFCtx.PeerAddr, Dest_b, 6);
					
					PlatformMoveMemory(pNanInfo->SDFCtx.SDAttibute.ServiceID, ServiceID, 6);
					pNanInfo->SDFCtx.SDAttibute.InstanceID = 0x88;
					pNanInfo->SDFCtx.ConfigPara.TimeToLive = 0xFFFFFFFF;
					goto test_SDF;
				}
				
				if( InformationBufferLength < 16 )	// 16 = 6(peer addr) + 9(SDA Mandatory) + 1(Configuration Parameters)
				{
					Status = NDIS_STATUS_INVALID_LENGTH;
					return Status;
				}
				
				u2bPacketLength = sizeof(InformationBuffer);
				if( InformationBufferLength < (ULONG)(u2bPacketLength)-1)
				{
					Status = NDIS_STATUS_INVALID_LENGTH;
					*BytesNeeded = u2bPacketLength;
					return Status;
				}
		
				PlatformMoveMemory(pNanInfo->SDFCtx.PeerAddr, InformationBuffer, sizeof(u1Byte)*6);
				PlatformMoveMemory(&pNanInfo->SDFCtx.SDAttibute,((u1Byte*)(InformationBuffer)+ sizeof(u1Byte)*6), sizeof(NAN_SD_ATTRIBUTE));
				PlatformMoveMemory(&pNanInfo->SDFCtx.ConfigPara,((u1Byte*)(InformationBuffer)+ sizeof(u1Byte)*6 + sizeof(NAN_SD_ATTRIBUTE)), sizeof(NAN_SDF_CONFIG_PARA));
test_SDF:

				pNanInfo->SDFCtx.bIsContentUpdated = TRUE;
				//pNanInfo->SDFCtx.bIsContentUpdatedUnicast = TRUE;
#if (WPP_SOFTWARE_TRACE == 0)				
				RT_PRINT_ADDR(COMP_NAN, DBG_LOUD, "OID_RT_NAN_SDF: Addr = ", pNanInfo->SDFCtx.PeerAddr);
				RT_PRINT_DATA(COMP_NAN, DBG_LOUD, ("ServiceID = "), pNanInfo->SDFCtx.SDAttibute.ServiceID, 6);
				RT_TRACE(COMP_NAN, DBG_LOUD, ("InstanceID = %d, ReqID = %d, ServiceCtrl = 0x%02X\n", pNanInfo->SDFCtx.SDAttibute.InstanceID, pNanInfo->SDFCtx.SDAttibute.RequestorID, pNanInfo->SDFCtx.SDAttibute.ServiceControl));
#endif
				if(pNanInfo->SDFCtx.SDAttibute.MatchingFilterLength > 0)
				{
					for(idx = 0; idx < MAX_M_FILTER_PAIR_NUM; idx ++)
					{
						if(pNanInfo->SDFCtx.SDAttibute.MatchingFilter[idx].Length > 0)
						{
							RT_TRACE(COMP_NAN, DBG_LOUD, ("Matching Filter[%d]: Len = %d", idx, pNanInfo->SDFCtx.SDAttibute.MatchingFilter[idx].Length));
							RT_PRINT_DATA(COMP_NAN, DBG_LOUD, ("Content = \n"), pNanInfo->SDFCtx.SDAttibute.MatchingFilter[idx].FilterValue, pNanInfo->SDFCtx.SDAttibute.MatchingFilter[idx].Length);
						}
					}
				}
				RT_TRACE(COMP_NAN, DBG_LOUD, ("DiscoveryRange = 0x%08X, TimeToLive = %d\n"
						"SubscribeType = %d, QueryPeriod = %d\n"
						"PublishType = %d, SolicitedTransmitType = %d, AnnouncementPeriod = %d\n"
						"TransmissionPriority = %d\n, SDFDelay = %d\n",
					pNanInfo->SDFCtx.ConfigPara.DiscoveryRange, pNanInfo->SDFCtx.ConfigPara.TimeToLive,
					pNanInfo->SDFCtx.ConfigPara.SubscribeType, pNanInfo->SDFCtx.ConfigPara.QueryPeriod,
					pNanInfo->SDFCtx.ConfigPara.PublishType, pNanInfo->SDFCtx.ConfigPara.SolicitedTransmitType, pNanInfo->SDFCtx.ConfigPara.AnnouncementPeriod,
					pNanInfo->SDFCtx.ConfigPara.TransmissionPriority, pNanInfo->SDFCtx.ConfigPara.SDFDelay));
				RT_PRINT_ADDR(COMP_NAN, DBG_LOUD, "Dest Addr = ", pNanInfo->SDFCtx.ConfigPara.DestAddr);

				RT_TRACE(COMP_NAN, DBG_LOUD, ("bIsTimeToSend = %d\n", pNanInfo->SDFCtx.bIsTimeToSend));	
			}
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SEND_SPECIFIC_PACKET.\n"));
		}
		break;

	case OID_RT_NAN_INIT_TSF:
		{
			Status = NdisStatusFromRtStatus(NAN_UpdateDefaultSetting(pAdapter, NAN_REG_INIT_TSF, InformationBuffer, InformationBufferLength));
		}
		break;

	case OID_RT_NAN_BEACON_SEND_TYPE:
		{
			Status = NdisStatusFromRtStatus(NAN_UpdateDefaultSetting(pAdapter, NAN_REG_BEACON_SEND_CAP, InformationBuffer, InformationBufferLength));
		}
		break;

	case OID_RT_NAN_OPER_CHNL_2G:
		{
			Status = NdisStatusFromRtStatus(NAN_UpdateDefaultSetting(pAdapter, NAN_REG_OPER_CHNL_2G, InformationBuffer, InformationBufferLength));
		}
		break;

	case OID_RT_NAN_OPER_CHNL_5G:
		{
			Status = NdisStatusFromRtStatus(NAN_UpdateDefaultSetting(pAdapter, NAN_REG_OPER_CHNL_5G, InformationBuffer, InformationBufferLength));
		}
		break;

	case OID_RT_NAN_FURTHER_AVAILABILITY_TX:
		{
			PNAN_INFO 		 pNanInfo = GET_NAN_INFO(pAdapter);
			Status = NDIS_STATUS_SUCCESS;

			if(sizeof(u1Byte) != InformationBufferLength)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				RT_TRACE_F(COMP_NAN,DBG_LOUD,("Require Length(%d) != InformationBufferLength(%d)\n",sizeof(u1Byte),InformationBufferLength));
				break;
			}

			if(!pNanInfo->bAppendFurtherAvailability)
			{	//NAN_TO_DO_YP
				//if concurrent, not support further availability
				pNanInfo->bAppendFurtherAvailability = TRUE;
			}
		}
		break;	

	case OID_RT_NAN_FURTHER_AVAILABILITY_RX:
		{
			PNAN_INFO 		 pNanInfo = GET_NAN_INFO(pAdapter);
			Status = NDIS_STATUS_SUCCESS;

			if(sizeof(u1Byte) != InformationBufferLength)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				RT_TRACE_F(COMP_NAN,DBG_LOUD,("Require Length(%d) != InformationBufferLength(%d)\n",sizeof(u1Byte),InformationBufferLength));
				break;
			}

			if(!pNanInfo->bTestbedAvailabilityTest)
			{
				//NAN_TO_DO_YP
				//if concurrent, not support further availability
				pNanInfo->bTestbedAvailabilityTest = TRUE;
			}
		}
		break;
	case OID_RT_NAN_GET_AVAILABILITY_INFO:
		{

		Status = NdisStatusFromRtStatus(NAN_GetAvailabilityInfo(pAdapter, InformationBuffer, InformationBufferLength));
		}
		break;

	case OID_RT_NAN_BEACON_AGE_TIMEOUT:
		{
			Status = NdisStatusFromRtStatus(NAN_UpdateDefaultSetting(pAdapter, NAN_REG_BEACON_AGE_TIMEOUT, InformationBuffer, InformationBufferLength));
		}
		break;
#endif

	default:
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;
	}
	return Status;
}

// This is to offload the N6CSetInformation() since the huge size of the funcion stack causes the BSOD when DbgPrint.
static NDIS_STATUS
N6CSetInformationHandleCustomizedOids(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
	)
{
	PADAPTER				pAdapter = (PADAPTER)MiniportAdapterContext;
	PADAPTER	              	pExtAdapter = GetFirstExtAdapter(pAdapter);
	PMGNT_INFO      			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;

//	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;
	
	switch(Oid)
	{
	default:
		// Can not find the OID specified
		Status = NDIS_STATUS_NOT_RECOGNIZED;
		break;
		
	case OID_802_11_BSSID:
	case OID_RT_802_11_BSSID:
		if(InformationBufferLength < 6)
		{
			*BytesNeeded = 6;
			Status = NDIS_STATUS_INVALID_LENGTH;
			return Status;
		}
		else
		{
			MgntActSet_802_11_BSSID( pAdapter, (pu1Byte)InformationBuffer );
			*BytesRead = 6;
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_BSSID: \n"));
		break;

	case OID_802_11_SSID:
	case OID_RT_802_11_SSID:
		{
			PNDIS_802_11_SSID    		ssid;

			if( (InformationBuffer == 0) || (InformationBufferLength == 0) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}
			
			//2004/08/05, kcwu, for NDTEST 1c_wlan_bssidlist
			//Driver must set NDIS_802_11_BSSID_LIST_EX->NumberOfItems to 0 
			//when a scan is performed while the radio is off
			//Notice: It should be set to FALSE in OID_802_11_INFRASTRUCTURE_MODE,
			//           But we make it change here
			pNdisCommon->KeepDisconnectFlag = FALSE;
			//==
			
			ssid = (PNDIS_802_11_SSID)InformationBuffer;
			if( ssid->SsidLength > 32 )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}
	
#if RTL819X_NO_SCAN_AFTER_LINK
			if(!pMgntInfo->bMediaConnect){
				RT_TRACE(COMP_SEND, DBG_LOUD, ("==m==>Scan <==m==\n"));
				MgntActSet_802_11_SSID( pAdapter, ssid->Ssid, (u2Byte)ssid->SsidLength, TRUE );
			}
#else
			MgntActSet_802_11_SSID( pAdapter, ssid->Ssid, (u2Byte)ssid->SsidLength, TRUE );
#endif

			RT_PRINT_STR( COMP_OID_SET, DBG_LOUD, "Set OID_802_11_SSID", ssid->Ssid, ssid->SsidLength );
			RT_PRINT_STR( COMP_QOS, DBG_LOUD, "Set OID_802_11_SSID", ssid->Ssid, ssid->SsidLength );
		}
		break;

    case OID_802_11_NETWORK_TYPE_IN_USE:
	case OID_RT_802_11_NETWORK_TYPE_IN_USE:
		if( (InformationBuffer == 0) || (InformationBufferLength < 4) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			*BytesNeeded = sizeof(ULONG);
			return Status;
		}

		//HCT12.0 NDTest
		if(	(*(PULONG)InformationBuffer == Ndis802_11OFDM24) ||
			(*(PULONG)InformationBuffer == Ndis802_11OFDM5) ||
			(*(PULONG)InformationBuffer == Ndis802_11DS) )
		{
			pNdisCommon->BeSetNetworkTypeByNDTest = (NDIS_802_11_NETWORK_TYPE)(*(PULONG)InformationBuffer);
		}
		else
		{
			pNdisCommon->BeSetNetworkTypeByNDTest = (NDIS_802_11_NETWORK_TYPE)0;
			Status = NDIS_STATUS_INVALID_DATA;
		}

		*BytesRead = 4;
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_NETWORK_TYPE_IN_USE: \n"));
		break;

	case OID_802_11_TX_POWER_LEVEL:
	case OID_RT_802_11_TX_POWER_LEVEL:
		if( (InformationBuffer == 0) || (InformationBufferLength < 4) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			return Status;
		}

		// <RJ_TODO> MgntActSet_802_11_TX_POWER_LEVEL() does not set tx power in mW. 
		MgntActSet_802_11_TX_POWER_LEVEL( pAdapter, (*(pu4Byte)InformationBuffer) );
		*BytesRead = 4;

		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_TX_POWER_LEVEL: \n"));
		break;

	case OID_RT_TX_POWER_LEVEL:
		{	
			s4Byte	powerLevel =0;
			if( InformationBufferLength < sizeof(s4Byte) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(s4Byte);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_TX_POWER_LEVEL: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;
			} 
			powerLevel = *((s4Byte*)InformationBuffer);
			pNdisCommon->RegTxPowerLevel = powerLevel;
			pMgntInfo->TxPowerLevel = pNdisCommon->RegTxPowerLevel;
			MgntActSet_TX_POWER_LEVEL(pAdapter, powerLevel);
		}
		break;

	case OID_RT_TX_POWER:
		{
			s4Byte powerInDbm;

			if( InformationBufferLength < sizeof(s4Byte) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(s4Byte);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_TX_POWER: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;

			} 

			powerInDbm = *((s4Byte*)InformationBuffer);
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_TX_POWER: %d dBm\n", powerInDbm));
			// Save the user's power.
			pMgntInfo->ClientConfigPwrInDbm = powerInDbm;
			MgntActSet_TX_POWER_DBM(pAdapter, powerInDbm);	

			*BytesRead = InformationBufferLength;
		}
		break;

	case OID_802_11_RSSI_TRIGGER:
	case OID_RT_802_11_RSSI_TRIGGER:
		if( (InformationBuffer == 0) || (InformationBufferLength < 4) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			return Status;
		}
		pNdisCommon->NdisRssiTrigger = *((ULONG*)InformationBuffer);
		*BytesRead = 4;
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_RSSI_TRIGGER\n"));
		break;

	case OID_802_11_INFRASTRUCTURE_MODE:
	case OID_RT_802_11_INFRASTRUCTURE_MODE:
		{
			RT_JOIN_NETWORKTYPE	networktype = RT_JOIN_NETWORKTYPE_AUTO;
			if( (InformationBuffer == 0) || (InformationBufferLength < sizeof(NDIS_802_11_NETWORK_INFRASTRUCTURE )) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(NDIS_802_11_NETWORK_INFRASTRUCTURE);
				return Status;
			}

			switch( *(PNDIS_802_11_NETWORK_INFRASTRUCTURE)InformationBuffer )
			{
			case Ndis802_11IBSS:
				networktype = RT_JOIN_NETWORKTYPE_ADHOC;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_INFRASTRUCTURE_MODE: RT_JOIN_NETWORKTYPE_ADHOC\n"));
				break;
			case Ndis802_11Infrastructure:
				networktype = RT_JOIN_NETWORKTYPE_INFRA;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_INFRASTRUCTURE_MODE: RT_JOIN_NETWORKTYPE_INFRA\n"));
				break;
			case Ndis802_11AutoUnknown:
				networktype = RT_JOIN_NETWORKTYPE_AUTO;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_INFRASTRUCTURE_MODE: RT_JOIN_NETWORKTYPE_AUTO\n"));
				break;// auto mode
			default:
				RT_TRACE(COMP_OID_SET, DBG_WARNING, ("Set OID_802_11_INFRASTRUCTURE_MODE: unknown type: 0x%X\n", *(PNDIS_802_11_NETWORK_INFRASTRUCTURE)InformationBuffer));
				break;
			}
	
			if(ACTING_AS_AP(pAdapter) )
			{ // 070125, rcnjko.
				pMgntInfo->Regdot11networktype = RT_JOIN_NETWORKTYPE_INFRA;
				return NDIS_STATUS_SUCCESS;
			}

			MgntActSet_802_11_INFRASTRUCTURE_MODE( pAdapter, networktype );
			*BytesRead = sizeof(NDIS_802_11_NETWORK_INFRASTRUCTURE);
		}
		break;

	case OID_802_11_FRAGMENTATION_THRESHOLD:
	case OID_RT_802_11_FRAGMENTATION_THRESHOLD:
		// 2004.06.01, by rcnjko.
		if( (InformationBuffer == 0) || (InformationBufferLength < 4) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			return Status;
		}
		MgntActSet_802_11_FRAGMENTATION_THRESHOLD( pAdapter, *((pu2Byte)InformationBuffer) );
		*BytesRead = sizeof(ULONG);
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_FRAGMENTATION_THRESHOLD: \n"));
		break;

	case OID_802_11_RTS_THRESHOLD:
	case OID_RT_802_11_RTS_THRESHOLD:
		if( (InformationBuffer == 0) || (InformationBufferLength < 4) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			return Status;
		}
		MgntActSet_802_11_RTS_THRESHOLD( pAdapter, *(pu2Byte)InformationBuffer );
		*BytesRead = sizeof(ULONG);
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_RTS_THRESHOLD: \n"));
		break;


	case OID_802_11_DESIRED_RATES:
	case OID_RT_802_11_DESIRED_RATES:
#if 0
		if( (InformationBuffer == 0) || (InformationBufferLength < 4) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			goto set_oid_exit;
		}
		
		if(1){
			u2Byte ratelen = (u2Byte)InformationBufferLength;
			
			ratelen = (ratelen<8) ? ratelen : 8;
			//ShuChen TODO: save desired rate
			//NdisMoveMemory(&Adapter->NdisDesiredRate[0], InformationBuffer, ratelen);
			*BytesRead = ratelen;
		}
//		OID_USED_ChangeDataRate(Adapter);
#endif
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_DESIRED_RATES: \n"));
		break;

	case OID_802_11_CONFIGURATION:
	case OID_RT_802_11_CONFIGURATION:
		if( (InformationBuffer == 0) || (InformationBufferLength < sizeof(NDIS_802_11_CONFIGURATION)) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			*BytesNeeded = sizeof(NDIS_802_11_CONFIGURATION);
			return Status;
		}
		else
		{
			NDIS_802_11_CONFIGURATION *pConfigToSet;
			u1Byte ChnlNum;

			pConfigToSet = (NDIS_802_11_CONFIGURATION *)InformationBuffer;
			*BytesRead = sizeof(NDIS_802_11_CONFIGURATION);

			// Translate from kHz to channel number.
			ChnlNum = CHNL_IsLegalChannel(pAdapter, pConfigToSet->DSConfig);
			if(ChnlNum > 0)
			{
				MgntActSet_802_11_CONFIGURATION(
					pAdapter, 
					(u2Byte)((pNdisCommon->RegBeaconPeriod != 0) ? pMgntInfo->Regdot11BeaconPeriod : (u2Byte)pConfigToSet->BeaconPeriod), // Do not change beacon interval if user had specified beacon interval for debug purpose. 2005.04.18, by rcnjko.
					(u1Byte)ChnlNum);
			}
			else
			{
				RT_ASSERT(FALSE, ("Set OID_802_11_CONFIGURATION: invalid frequency: %d\n", pConfigToSet->DSConfig));
			}
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_CONFIGURATION: \n"));
		}
		break;

	case OID_802_11_DISASSOCIATE:
	case OID_RT_802_11_DISASSOCIATE:
		MgntActSet_802_11_DISASSOCIATE( pAdapter, disas_lv_ss );
		if(pAdapter->bInHctTest)
		{
			//2004/08/23, kcwu
			pNdisCommon->KeepDisconnectFlag = TRUE;
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_DISASSOCIATE: \n"));
		break;

	case OID_802_11_POWER_MODE:
	case OID_RT_802_11_POWER_MODE:
		{
			NDIS_802_11_POWER_MODE ndisPsModeToSet;
			RT_PS_MODE rtPsModeToSet;
			PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

			if( (InformationBuffer == NULL) || 
				(InformationBufferLength < sizeof(NDIS_802_11_POWER_MODE)) )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(NDIS_802_11_POWER_MODE );
				return Status;
			}

			ndisPsModeToSet = *((NDIS_802_11_POWER_MODE*)InformationBuffer);
			rtPsModeToSet = (RT_PS_MODE)TranslateNdisPsToRtPs(ndisPsModeToSet);

			if(InformationBufferLength >= sizeof(NDIS_802_11_POWER_MODE) + 4)
			{
				pMgntInfo->PsPollType = *((pu1Byte)InformationBuffer + 4);
			}

			if(pNdisCommon->RegPowerSaveMode != ndisPsModeToSet)
			{
				pNdisCommon->RegPowerSaveMode = ndisPsModeToSet;
				pPSC->RegPowerSaveMode = pNdisCommon->RegPowerSaveMode;
				//if( (pNdisCommon->RegLeisurePsMode) && (rtPsModeToSet == eActive) && (!pAdapter->bInHctTest ))
				if( (pNdisCommon->RegLeisurePsMode) && (!pAdapter->bInHctTest ))
					pPSC->bLeisurePs= TRUE;
				else
					pPSC->bLeisurePs= FALSE;
				MgntActSet_802_11_PowerSaveMode(pAdapter, rtPsModeToSet);
				//2008.08.25
				if( (pNdisCommon->RegLeisurePsMode) && (rtPsModeToSet == eActive) 
					&& (! pAdapter->bInHctTest ) )
				{
					// Auto LeisurePs based on AC or Battery.
					if(pNdisCommon->RegLeisurePsMode == eLpsAuto)
					{
						if(pPSC->PowerProfile == NdisPowerProfileBattery  )
							pPSC->bLeisurePs= TRUE;
						else if(pPSC->PowerProfile == NdisPowerProfileAcOnLine )
							pPSC->bLeisurePs= FALSE;
						
						// NdisDevicePnPEventSurpriseRemoved isn't called. 
						else
							pPSC->bLeisurePs= TRUE;
					}
					//Always turn on LeisurePS.
					else if(pNdisCommon->RegLeisurePsMode == eLpsOn)
					{
						pPSC->bLeisurePs= TRUE;
					}
				}
				else
					pPSC->bLeisurePs= FALSE;
			}

			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_POWER_MODE: 0x%X\n", ndisPsModeToSet));
		}
		break;


	case OID_802_11_PRIVACY_FILTER:
	case OID_RT_802_11_PRIVACY_FILTER:
		//TODO:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_11_PRIVACY_FILTER: \n"));
		break;

	case OID_RT_SCAN_AVAILABLE_BSSID:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SCAN_AVAILABLE_BSSID: \n"));
	case OID_802_11_BSSID_LIST_SCAN:
	case OID_RT_802_11_BSSID_LIST_SCAN:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_802_11_BSSID_LIST_SCAN.\n"));
		if(pMgntInfo->SnifferTurnOnFlag)
			break;
#if RTL819X_NO_SCAN_AFTER_LINK		
		if(!pMgntInfo->bMediaConnect){
			RT_TRACE(COMP_SEND, DBG_LOUD, ("==m==> Scan <==m==\n"));
			MgntActSet_802_11_BSSID_LIST_SCAN(pAdapter);
		}
#else
		MgntActSet_802_11_BSSID_LIST_SCAN(pAdapter);
#endif
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_802_11_BSSID_LIST_SCAN.\n"));
		break;

	case OID_802_11_TEST:			//O
	case OID_RT_802_11_TEST:
		if( (InformationBuffer == 0) || (InformationBufferLength < 4) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			return Status;
		}
		else 
		{
			NDIS_802_11_TEST* Test = (PNDIS_802_11_TEST)InformationBuffer;

			switch(Test->Type)
			{
			case 1:
				N6IndicateStatus(
						pAdapter,
						NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
						(PVOID)&Test->AuthenticationEvent,
						sizeof(NDIS_802_11_STATUS_TYPE) + Test->Length);
				break;

			case 2:
				N6IndicateStatus(
						pAdapter,
						NDIS_STATUS_MEDIA_SPECIFIC_INDICATION,
						(PVOID)&Test->RssiTrigger,
						sizeof(NDIS_802_11_RSSI ));
				break;

			default:
				Status = NDIS_STATUS_INVALID_DATA;
				break;
			}
		}
		*BytesRead = InformationBufferLength;
		break;

	case OID_RT_SET_CHANNEL:
		{
			u1Byte btChannel;
			
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SET_CHANNEL\n"));

			// Verify input paramter.
			if(InformationBufferLength < sizeof(u1Byte))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(u1Byte);
				return Status;
			}

			// Get channel number to set.
			btChannel = *((u1Byte*)InformationBuffer);

			//
			// 2012/06/08 MH Dirty fix for adhoc set channel under adhoc mode? 
			// Before RT UI set channel, the scan process will be called at first and then
			// the set channel process will be aborted. Need to investigate later!?
			// 			
			{
				u1Byte		delay = 0;
				PMGNT_INFO	pMgntInfo = &pAdapter->MgntInfo;
				while (pMgntInfo->bScanInProgress && delay < 20 && pMgntInfo->mIbss)
				{
					delay_ms(100);
					delay++;
					//DbgPrint("scan in progress we can nnot set channel otherwise this will be aborted delay=%d\n", delay);
				}
			}
			
			// Switch channel.
			if(MgntActSet_802_11_REG_20MHZ_CHANNEL_AND_SWITCH(pAdapter, btChannel) != TRUE)
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("!!! Failed to switch to the channel, %d\n", btChannel));
			}
			
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SET_CHANNEL: %d\n",btChannel));
		}
		break;

	case OID_RT_SET_PREAMBLE_MODE:
		{
			UCHAR PreambleMode;

			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SET_PREAMBLE_MODE.\n"));

			// Verify input paramter.
			if(InformationBufferLength < sizeof(PreambleMode))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(PreambleMode);
				return Status;
			}

			PreambleMode = *((UCHAR*)InformationBuffer);
			// 0:Long, 1:Auto, 2:Short.
			switch(PreambleMode)
			{
			case 0:
				MgntActSet_802_11_PREAMBLE_MODE(pAdapter, PREAMBLE_LONG);
				break;
			case 1:
				MgntActSet_802_11_PREAMBLE_MODE(pAdapter, PREAMBLE_AUTO);
				break;
			case 2:
				MgntActSet_802_11_PREAMBLE_MODE(pAdapter, PREAMBLE_SHORT);
				break;
			default:
				RT_ASSERT(FALSE, ("Invalid preamble mode\n"));
				break;
			}
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SET_PREAMBLE_MODE: %d\n", PreambleMode));
		}
		break;

	case OID_RT_WIRELESS_MODE:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_WIRELESS_MODE.\n"));
		{
			u1Byte btWirelessModeToSet;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(u1Byte))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(u1Byte);
				return Status;
			}

			// Get wireless mode to switch to.
			btWirelessModeToSet = *((u1Byte*)InformationBuffer);

			if(FALSE == MgntActSet_802_11_WIRELESS_MODE(pAdapter, (WIRELESS_MODE)btWirelessModeToSet))
			{
				Status = NDIS_STATUS_INVALID_DATA;
				*BytesNeeded = sizeof(btWirelessModeToSet);
				return Status;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_WIRELESS_MODE.\n"));
		break;

	// Annie, 2004-12-27
	case OID_RT_SET_CHANNELPLAN:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SET_CHANNELPLAN\n"));
		{
			u2Byte ChannelPlan;
			
			// Verify input paramter
			if(InformationBufferLength < sizeof(u2Byte))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(u2Byte);
				return Status;
			}

			// Get channel plan to set
			ChannelPlan = *((u2Byte*)InformationBuffer);

			// Set channel plan
			if(MgntActSet_802_11_CHANNELPLAN(pAdapter, ChannelPlan) != TRUE)
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("!!! Failed to set the channelplan, %d\n", ChannelPlan));
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SET_CHANNELPLAN.\n"));
		break;

	case OID_RT_FORCED_DATA_RATE:
		{
			u1Byte		DataRate;
			PADAPTER 	pExtAdapter = NULL;
	
			// Verify input paramter.
			if(InformationBufferLength < sizeof(DataRate))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(DataRate);
				return Status;
			}

			// Get forced data rate to set.
			DataRate = *((u1Byte*)InformationBuffer);

			// Check if it is valid for current wireless mode.
			if(!MgntIsRateValidForWirelessMode(DataRate, pMgntInfo->dot11CurrentWirelessMode) && 
				DataRate != 0 ) // 0 stands for Auto.
			{
				Status = NDIS_STATUS_INVALID_DATA;
				*BytesNeeded = sizeof(DataRate);
				return Status;
			}

			// Set data rate forced.
			pExtAdapter = GetDefaultAdapter(pAdapter);		
			while(pExtAdapter !=NULL){
				pExtAdapter->MgntInfo.ForcedDataRate = (u2Byte)DataRate;
				pExtAdapter = GetNextExtAdapter(pExtAdapter);
			}
			pNdisCommon->RegForcedDataRate = (u2Byte)DataRate;


			if(InformationBufferLength > 1)
			{
				pExtAdapter = GetDefaultAdapter(pAdapter);		
				while(pExtAdapter !=NULL){
					pExtAdapter->MgntInfo.ForcedTxDisableRateFallBack = *((u1Byte*)InformationBuffer+1);
					pExtAdapter = GetNextExtAdapter(pExtAdapter);
				}
			}
		}
		break;

	case OID_RT_WIRELESS_MODE_FOR_SCAN_LIST:
		//
		// It changes the behavior when we report scan result to upper layer. 
		// See OID_802_11_BSSID_LIST for details.  2005.01.13, by rcnjko.
		//
		{
			u1Byte WirelessMode;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(WirelessMode))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(WirelessMode);
				return Status;
			}

			// Get wireless mode for scan list.
			WirelessMode = *((u1Byte*)InformationBuffer);

			// Set wireless mode for scan list.
			pNdisCommon->RegWirelessMode4ScanList = (int)WirelessMode;
		}
		break;

	case OID_RT_SCAN_WITH_MAGIC_PACKET:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SCAN_WITH_MAGIC_PACKET.\n"));
		{
			pu1Byte	pDstAddr;

			// Verify input paramter.
			if(InformationBufferLength < sizeof(UCHAR)*6)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(UCHAR)*6;
				return Status;
			}

			// Get offset, data width, and value to write.
			pDstAddr = (pu1Byte)InformationBuffer;

			// Send magic packet to each channle.
			MgntActSet_802_11_ScanWithMagicPacket(pAdapter, pDstAddr);
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SCAN_WITH_MAGIC_PACKET.\n"));
		break;

	case OID_RT_SET_SCAN_OPERATION:
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("===> Set OID_RT_SET_SCAN_OPERATION.\n"));
		{
			PADAPTER pDefAdapter =  GetDefaultAdapter(pAdapter);
			PMGNT_INFO pDefMgntInfo = &(pDefAdapter->MgntInfo);
			
			*BytesRead = sizeof(BOOLEAN);
			pMgntInfo->bDisableScanByOID = *((BOOLEAN*)InformationBuffer);
			pDefMgntInfo->bDisableScanByOID = *((BOOLEAN*)InformationBuffer);
			
			if(pMgntInfo->bDisableScanByOID)
			{
				if(MgntScanInProgress(pDefMgntInfo) == TRUE && MgntIsLinkInProgress(pDefMgntInfo) == FALSE)
					MgntResetScanProcess(pDefAdapter);
			}
		}
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("<=== Set OID_RT_SET_SCAN_OPERATION %d.\n", pMgntInfo->bDisableScanByOID));
		break;
	

	case OID_RT_AP_SWITCH_INTO_AP_MODE:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_AP_SWITCH_INTO_AP_MODE.\n"));
		{
			switch(*(ULONG *)InformationBuffer)
			{
			case 0:
				// Switch to STA mode.
				MgntActSet_ApMode(pAdapter, FALSE);
				break;

			case 1:
				// Switch to AP mode.
				MgntActSet_ApMode(pAdapter, TRUE);
				break;
			}	
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_AP_SWITCH_INTO_AP_MODE.\n"));
		break;

	case OID_RT_AP_SET_DTIM_PERIOD:
		{
			u1Byte u1DtimPeriod; 

			// Verify input paramter.
			if(InformationBufferLength < sizeof(u1Byte))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(u1Byte);
				return Status;
			}

			u1DtimPeriod = *((pu1Byte)InformationBuffer);
			pNdisCommon->RegDtimPeriod = u1DtimPeriod;
			MgntActSet_802_11_DtimPeriod(pAdapter, u1DtimPeriod);
		}
		break;

	case OID_RT_SET_BCN_INTVL:
		{
			u2Byte u2BcnIntv; 

			// Verify input paramter.
			if(InformationBufferLength < sizeof(u2Byte))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(u2Byte);
				return Status;
			}

			u2BcnIntv = *((pu2Byte)InformationBuffer);
			pNdisCommon->RegBeaconPeriod = u2BcnIntv;
			MgntActSet_802_11_BeaconInterval(pAdapter, u2BcnIntv);
		}
		break;

	case OID_RT_AP_SET_PASSPHRASE:
		{
			PRtlLibPassphrase	pRtPassphrase;
			OCTET_STRING		osPassphrase;

			// Verify input paramter.
			if(InformationBuffer == NULL)
			{
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;
			}
			if(InformationBufferLength < sizeof(RtlLibPassphrase))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(RtlLibPassphrase);
				return Status;
			}

			pRtPassphrase = (PRtlLibPassphrase)InformationBuffer;
			if(pRtPassphrase->Length > 64 || pRtPassphrase->Length < 8) // Modify 63 to 64 For A mode 64-Hex mode !!
			{
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;
			}
			
			FillOctetString(osPassphrase, pRtPassphrase->Passphrase, ((u2Byte)pRtPassphrase->Length));
			//PRINT_DATA(("Set OID_RT_AP_SET_PASSPHRASE: "), osPassphrase.Octet, osPassphrase.Length);

			MgntActSet_Passphrase(pAdapter, &osPassphrase);
		}
		break;

	case OID_RT_SEND_SPECIFIC_PACKET:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SEND_SPECIFIC_PACKET.\n"));
		{
			u2Byte			idx, u2bPacketLength, u2bTxNumber;
			u1Byte			u1bTxRate;
			OCTET_STRING	ocPacketToSend;

			if( InformationBufferLength < 16 )	// 16 = 2(Length) + 1(DataRate) + 2(Number) + 11(Reserved).
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = 16;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SEND_SPECIFIC_PACKET  NDIS_STATUS_INVALID_LENGTH.\n"));
				return Status;
			}
			
			// Get packet lengh, in unit of byte.
			u2bPacketLength = *((pu2Byte)InformationBuffer);
			if( InformationBufferLength < (ULONG)(16 + u2bPacketLength))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = (16 + u2bPacketLength);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SEND_SPECIFIC_PACKET  NDIS_STATUS_INVALID_LENGTH2 %ld.\n",*BytesNeeded));
				return Status;
			}
			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] u2bPacketLength = %d.\n", u2bPacketLength));
			
			// Get data rate, in unit of 0.5 Mbps.
			u1bTxRate = *((pu1Byte)InformationBuffer + 2);
			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] u1bTxRate = %d.\n", u1bTxRate));
			
			// Get number of sending packet.
			u2bTxNumber = *(pu2Byte)( (pu1Byte)InformationBuffer+3 );
			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] u2bTxNumber = %d.\n", u2bTxNumber));
			
			// Get packet to send, it should be an 802.11 frame with CRC append.
			FillOctetString(ocPacketToSend, ((pu1Byte)InformationBuffer + 16), u2bPacketLength);
			RT_PRINT_DATA(COMP_MP, DBG_LOUD, "[Debug] ocPacketToSend:\n", ((pu1Byte)InformationBuffer + 16), u2bPacketLength);
			
			// Send the specific packet.
			if( u2bTxNumber == 0 )
				u2bTxNumber = 1;		// At least send one packet. Added by Annie, 2006-03-27.
			
			for( idx=0; idx<u2bTxNumber; idx++ )
			{
				if( !MgntSendSpecificPacket(pAdapter, &ocPacketToSend, u1bTxRate) )
				{
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;
				}
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SEND_SPECIFIC_PACKET.\n"));
		break;

	case OID_RT_SEND_SPECIFIC_PACKET_MP:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SEND_SPECIFIC_PACKET_MP.\n"));
		RT_TRACE(COMP_MP, DBG_LOUD, ("===> Set OID_RT_SEND_SPECIFIC_PACKET_MP.\n"));
		{
			u2Byte			idx, u2bPacketLength, u2bTxNumber;
			u1Byte			u1bTxRate;
			OCTET_STRING	ocPacketToSend;

			u4Byte			MptTag = 0;
			u1Byte 			MptTagBuf[4];
			u1Byte			i = 0;


			if( InformationBufferLength < 16 )	// 16 = 2(Length) + 1(DataRate) + 2(Number) + 11(Reserved).
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = 16;
				return Status;
			}
			
			// Get packet length, in unit of byte.
			u2bPacketLength = *((pu2Byte)InformationBuffer);
			if( InformationBufferLength < (ULONG)(16 + u2bPacketLength))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = (16 + u2bPacketLength);
				return Status;
			}
			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] u2bPacketLength = %d.\n", u2bPacketLength));
			// Get data rate, in unit of 0.5 Mbps.
			u1bTxRate = *((pu1Byte)InformationBuffer + 2);

			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] u1bTxRate = %d.\n", u1bTxRate));

			// Get number of sending packet.
			u2bTxNumber = *(pu2Byte)( (pu1Byte)InformationBuffer+3 );

			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] u2bTxNumber = %d.\n", u2bTxNumber));
			
			// Get packet to send, it should be an 802.11 frame (start with 24 bytes header).
			FillOctetString(ocPacketToSend, ((pu1Byte)InformationBuffer + 16), u2bPacketLength);

			RT_PRINT_DATA(COMP_MP, DBG_LOUD, "[Debug] ocPacketToSend:\n", ((pu1Byte)InformationBuffer + 16), u2bPacketLength);

			// Send the specific packet.
			if( u2bTxNumber == 0 )
				u2bTxNumber = 1;		// At least send one packet. Added by Annie, 2006-03-27.
			
			//RT_PRINT_DATA(COMP_MP, DBG_LOUD, "[Debug] CurrentAddress:\n", pAdapter->CurrentAddress, 6);
			//RT_PRINT_DATA(COMP_MP, DBG_LOUD, "[Debug] ocPacketToSend::\n", ocPacketToSend.Octet, ocPacketToSend.Length); // For debug purpose

			for (i = 0; i < 4; i++)
				MptTagBuf[i] = *((pu1Byte)InformationBuffer + 16 + 24 + i);

			MptTag = *((u4Byte*)MptTagBuf);
			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] MptTag = 0x%0X.\n", MptTag));
			if (MptTag != 0x10ec8139){
				RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] return FALSE for MptTag != 0x10ec8139.\n"));
			}

			
			
			for( idx=0; idx<u2bTxNumber; idx++ )
			{
				if( !MgntSendSpecificPacket(pAdapter, &ocPacketToSend, u1bTxRate) )
				{
					Status = NDIS_STATUS_INVALID_DATA;
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SEND_SPECIFIC_PACKET  NDIS_STATUS_INVALID_DATA.\n"));
					return Status;
				}
			}
			
		}
		RT_TRACE(COMP_MP, DBG_LOUD, ("<=== Set OID_RT_SEND_SPECIFIC_PACKET_MP.\n"));
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SEND_SPECIFIC_PACKET_MP.\n"));
		break;

	case OID_RT_SEND_ONE_PACKET_MP:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_SEND_ONE_PACKET_MP.\n"));
		RT_TRACE(COMP_MP, DBG_LOUD, ("===> Set OID_RT_SEND_ONE_PACKET_MP.\n"));
		{
			u2Byte			idx, u2bPacketLength, u2bTxNumber;
			u1Byte			u1bTxRate;
			OCTET_STRING	ocPacketToSend;

			u4Byte			MptTag = 0;
			u1Byte 			MptTagBuf[4];
			u1Byte			i = 0;

			// TODO:  Construct Packet in driver, Not yet ready.

			if( InformationBufferLength < 16 )	// 16 = 2(Length) + 1(DataRate) + 2(Number) + 11(Reserved).
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = 16;
				return Status;
			}
			
			// Get packet length, in unit of byte.
			u2bPacketLength = *((pu2Byte)InformationBuffer);
			if( InformationBufferLength < (ULONG)(16 + u2bPacketLength))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = (16 + u2bPacketLength);
				return Status;
			}
			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] u2bPacketLength = %d.\n", u2bPacketLength));
			// Get data rate, in unit of 0.5 Mbps.
			u1bTxRate = *((pu1Byte)InformationBuffer + 2);

			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] u1bTxRate = %d.\n", u1bTxRate));

			// Get number of sending packet.
			u2bTxNumber = *(pu2Byte)( (pu1Byte)InformationBuffer+3 );

			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] u2bTxNumber = %d.\n", u2bTxNumber));
			
			// Get packet to send, it should be an 802.11 frame (start with 24 bytes header).
			FillOctetString(ocPacketToSend, ((pu1Byte)InformationBuffer + 16), u2bPacketLength);

			RT_PRINT_DATA(COMP_MP, DBG_LOUD, "[Debug] ocPacketToSend:\n", ((pu1Byte)InformationBuffer + 16), u2bPacketLength);

			// Send the specific packet.
			if( u2bTxNumber == 0 )
				u2bTxNumber = 1;		// At least send one packet. Added by Annie, 2006-03-27.
			
			//RT_PRINT_DATA(COMP_MP, DBG_LOUD, "[Debug] CurrentAddress:\n", pAdapter->CurrentAddress, 6);
			//RT_PRINT_DATA(COMP_MP, DBG_LOUD, "[Debug] ocPacketToSend::\n", ocPacketToSend.Octet, ocPacketToSend.Length); // For debug purpose

			for (i = 0; i < 4; i++)
				MptTagBuf[i] = *((pu1Byte)InformationBuffer + 16 + 24 + i);

			MptTag = *((u4Byte*)MptTagBuf);
			RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] MptTag = 0x%0X.\n", MptTag));
			if (MptTag != 0x10ec8139){
				RT_TRACE(COMP_MP, DBG_LOUD, ("[Debug] return FALSE for MptTag != 0x10ec8139.\n"));
			}

			
			
			for( idx=0; idx<u2bTxNumber; idx++ )
			{
				if( !MgntSendSpecificPacket(pAdapter, &ocPacketToSend, u1bTxRate) )
				{
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;
				}
			}
			
		}
		RT_TRACE(COMP_MP, DBG_LOUD, ("<=== Set OID_RT_SEND_ONE_PACKET_MP.\n"));
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_SEND_ONE_PACKET_MP.\n"));
		break;




	case OID_RT_PRO_SEND_RAW_DATA:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_SEND_RAW_DATA.\n"));
		{
			u2Byte			idx, u2bPacketLength, u2bTxNumber;
			u1Byte			u1bTxRate;
			OCTET_STRING	ocPacketToSend;

			if( InformationBufferLength < 16 )	// 16 = 2(Length) + 1(DataRate) + 2(Number) + 11(Reserved).
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = 16;
				return Status;
			}
			
			// Get packet lengh, in unit of byte.
			u2bPacketLength = *((pu2Byte)InformationBuffer);
			if( InformationBufferLength < (ULONG)(16 + u2bPacketLength))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = (16 + u2bPacketLength);
				return Status;
			}

			// Get data rate, in unit of 0.5 Mbps.
			u1bTxRate = *((pu1Byte)InformationBuffer + 2);

			// Get number of sending packet.
			u2bTxNumber = *(pu2Byte)( (pu1Byte)InformationBuffer+3 );
			
			// Get packet to send, it should be an 802.11 frame with CRC append.
			FillOctetString(ocPacketToSend, ((pu1Byte)InformationBuffer + 16), u2bPacketLength);
			//PRINT_DATA("ocPacketToSend", ocPacketToSend.Octet, ocPacketToSend.Length); // For debug purpose, 2005.12.23, by rcnjko.

			// Send the specific packet.
			if( u2bTxNumber == 0 )
				u2bTxNumber = 1;		// At least send one packet. Added by Annie, 2006-03-27.
			
			for( idx=0; idx<u2bTxNumber; idx++ )
			{
				if( !MgntSendRawPacket(pAdapter, &ocPacketToSend, u1bTxRate) )
				{
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;
				}
			}
		}
		break;

	case OID_RT_DBG_COMPONENT:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_DBG_COMPONENT.\n"));
		{
			u8Byte	InfoComp = 0;
			if( InformationBufferLength < 1 )	// At least 1 byte.
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = 1;
				return Status;
			}

			CopyMem( &InfoComp, InformationBuffer, InformationBufferLength );
			GlobalDebugComponents = InfoComp;
		}
		//RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_DBG_COMPONENT: GlobalDebugComponents: %#X\n", GlobalDebugComponents));
		break;

	case OID_RT_DBG_LEVEL:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_DBG_LEVEL.\n"));
		{
			u4Byte	InfoLevel = 0;
			if( InformationBufferLength < 1 )	// At least 1 byte.
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = 1;
				return Status;
			}

			CopyMem( &InfoLevel, InformationBuffer, InformationBufferLength );
			GlobalDebugLevel = InfoLevel;
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_DBG_LEVEL: GlobalDebugLevel: %#X\n", GlobalDebugLevel));
		break;

	case OID_RT_AUTO_SELECT_CHANNEL:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_AUTO_SELECT_CHANNEL.\n"));
		{
			if( InformationBufferLength < sizeof(BOOLEAN))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(BOOLEAN);
				return Status;
			}

			pMgntInfo->bAutoSelChnl = *((PBOOLEAN)InformationBuffer);
			*BytesRead = InformationBufferLength;
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_AUTO_SELECT_CHANNEL: %d\n", pMgntInfo->bAutoSelChnl));
		break;

	case OID_RT_HIDDEN_SSID:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_HIDDEN_SSID.\n"));
		{
			if( InformationBufferLength < sizeof(BOOLEAN))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(BOOLEAN);
				return Status;
			}

			pMgntInfo->bHiddenSSID = *((PBOOLEAN)InformationBuffer);
			*BytesRead = InformationBufferLength;
		}
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_HIDDEN_SSID: %d\n", pMgntInfo->bHiddenSSID) );
		break;
		
	case OID_RT_AP_GET_CLOUD_KEY_EX:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_AP_GET_CLOUD_KEY_EX.\n"));
		{
			u1Byte offset = 0;
			if((InformationBufferLength == 0)||(InformationBuffer == 0))
			{
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_MAC_FILTER_TYPE: Invalid Length!\n") );
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}

			offset = *( (pu1Byte)InformationBuffer );	// The first double word
			if(offset > EEPROM_CLOUD_KEY_LENGTH_EX || offset <0)
			{
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_AP_GET_CLOUD_KEY_EX: Invalid Data!\n") );
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;
			}
			pMgntInfo->cloud_key_offset = offset;
			*BytesRead = InformationBufferLength;
		}
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_AP_GET_CLOUD_KEY_EX\n") );
		break;
		
	case OID_RT_FILTER_DEFAULT_PERMITED:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_FILTER_DEFAULT_PERMITED.\n"));
		{
			BOOLEAN bPermited;
				
			if((InformationBufferLength == 0)||(InformationBuffer == 0))
			{
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_FILTER_DEFAULT_PERMITED: Invalid Length!\n") );
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}

			bPermited = (BOOLEAN)*( (pu1Byte)InformationBuffer );	// The first double word
			
			pMgntInfo->bDefaultPermited = bPermited;
			
			if(pMgntInfo->bDefaultPermited)
				pMgntInfo->LockType = MAC_FILTER_ACCEPT;
			else
				pMgntInfo->LockType = MAC_FILTER_REJECT;
			
			pMgntInfo->LockedSTACount = 0;
			pMgntInfo->LockedSTACount_Reject= 0;
			
			*BytesRead = InformationBufferLength;
		}
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_FILTER_DEFAULT_PERMITED\n") );
		break;	
		
	case OID_RT_MAC_FILTER_TYPE:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_MAC_FILTER_TYPE.\n"));
		{
			u1Byte	MacFilterType;
				
			if((InformationBufferLength == 0)||(InformationBuffer == 0))
			{
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_MAC_FILTER_TYPE: Invalid Length!\n") );
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}

			MacFilterType = *( (pu1Byte)InformationBuffer );	// The first double word
			
			if(MacFilterType > 2)
			{
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_MAC_FILTER_TYPE: Invalid Data!\n") );
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;
			}
			
			pMgntInfo->LockType = MacFilterType;
			if(pMgntInfo->LockType == MAC_FILTER_DISABLE)
			{
				pMgntInfo->LockedSTACount = 0;
				pMgntInfo->LockedSTACount_Reject= 0;
				PlatformZeroMemory( pMgntInfo->LockedSTAList, MAX_LOCKED_STA_LIST_NUM*6 );
				PlatformZeroMemory( pMgntInfo->LockedSTAList_Reject, MAX_LOCKED_STA_LIST_NUM*6 );
			}
			
			*BytesRead = InformationBufferLength;
		}
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_MAC_FILTER_TYPE\n") );
		
		break;	

	case OID_RT_LOCKED_STA_ADDRESS:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_LOCKED_STA_ADDRESS.\n"));
		{
			u4Byte	NumOfLockedAddr;
				
			if(	(InformationBufferLength == 0)			||
				(InformationBuffer == 0)					||
				( (InformationBufferLength-4)%6 != 0 )	||
				(InformationBufferLength > (4 + 6*MAX_LOCKED_STA_LIST_NUM) )
				)
			{
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_LOCKED_STA_ADDRESS: Invalid Length!\n") );
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}

			NumOfLockedAddr = *( (pu4Byte)InformationBuffer );	// The first double word
			
			if(	(NumOfLockedAddr > MAX_LOCKED_STA_LIST_NUM)	||
				((NumOfLockedAddr*6+4) != InformationBufferLength)
				)
			{
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_LOCKED_STA_ADDRESS: Invalid Data!\n") );
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;
			}

			MgntActSet_Locked_STA_Address( pAdapter, (pu1Byte)InformationBuffer+4, NumOfLockedAddr );
			*BytesRead = InformationBufferLength;
		}
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_LOCKED_STA_ADDRESS\n") );
		break;

	case OID_RT_FILTER_STA_ADDRESS:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_FILTER_STA_ADDRESS.\n"));
		{
			u4Byte	NumOfLockedAddr;
				
			if(	(InformationBufferLength == 0)			||
				(InformationBuffer == 0)					||
				( (InformationBufferLength-4)%6 != 0 )	||
				(InformationBufferLength > (4 + 6*MAX_LOCKED_STA_LIST_NUM) )
				)
			{
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_FILTER_STA_ADDRESS: Invalid Length!\n") );
				Status = NDIS_STATUS_INVALID_LENGTH;
				return Status;
			}

			NumOfLockedAddr = *( (pu4Byte)InformationBuffer );	// The first double word
			
			if(	(NumOfLockedAddr > MAX_LOCKED_STA_LIST_NUM)	||
				((NumOfLockedAddr*6+4) != InformationBufferLength)
				)
			{
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_FILTER_STA_ADDRESS: Invalid Data!\n") );
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;
			}

			if(pMgntInfo->LockType == MAC_FILTER_ACCEPT)
				MgntActSet_Accepted_STA_Address( pAdapter, (pu1Byte)InformationBuffer+4, NumOfLockedAddr );
			else if(pMgntInfo->LockType == MAC_FILTER_REJECT)
				MgntActSet_Rejected_STA_Address( pAdapter, (pu1Byte)InformationBuffer+4, NumOfLockedAddr );
			
			*BytesRead = InformationBufferLength;
		}
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_FILTER_STA_ADDRESS\n") );
		break;

	case OID_RT_AP_WDS_MODE:
		if( InformationBufferLength < sizeof(pMgntInfo->WdsMode) )
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			*BytesNeeded = sizeof(pMgntInfo->WdsMode);
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_AP_WDS_MODE: inavalid InformationBufferLength(%d), BytesNeeded is %d\n", InformationBufferLength, *BytesNeeded) );
			return Status;
		}

		pMgntInfo->WdsMode = *((pu1Byte)InformationBuffer);	
		*BytesRead = InformationBufferLength;
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_AP_WDS_MODE: %d\n", pMgntInfo->WdsMode));
		break;

	case OID_RT_AP_WDS_AP_LIST:
		{
			pu1Byte	pWdsApList;
			u4Byte	ListSize = sizeof(u4Byte) + 1*6;

			if( InformationBufferLength < ListSize )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = ListSize;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_AP_WDS_AP_LIST: inavalid InformationBufferLength(%d), BytesNeeded is %d\n", InformationBufferLength, *BytesNeeded) );
				return Status;
			}

			pWdsApList = (pu1Byte)InformationBuffer + sizeof(u4Byte);
			PlatformMoveMemory(pMgntInfo->WdsApAddr, pWdsApList, 6);
			*BytesRead = InformationBufferLength;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, 
				("Set OID_RT_AP_WDS_AP_LIST: %02X-%02X-%02X-%02X-%02X-%02X\n", 
				pMgntInfo->WdsApAddr[0], pMgntInfo->WdsApAddr[1], pMgntInfo->WdsApAddr[2], pMgntInfo->WdsApAddr[3], pMgntInfo->WdsApAddr[4], pMgntInfo->WdsApAddr[5] ));
		}
		break;
	
	case OID_RT_SET_PSP_XLINK_STATUS:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SET_PSP_XLINK_STATUS:\n"));
		{
			if ( InformationBufferLength < sizeof(u4Byte) )
			{
				*BytesNeeded = sizeof(u4Byte);
				return NDIS_STATUS_INVALID_LENGTH;
			}

			pMgntInfo->bDefaultPSPXlinkMode = *((PBOOLEAN)InformationBuffer);
		}
		break;

	case OID_RT_SET_WMM_ENABLE:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SET_WMM_ENABLE:\n"));
		{
			if ( InformationBufferLength < sizeof(u4Byte) )
			{
				*BytesNeeded = sizeof(u4Byte);
				return NDIS_STATUS_INVALID_LENGTH;
			}

			MgntActSet_802_11_WMM_MODE(pAdapter, *((PBOOLEAN)InformationBuffer));
		}
		break;
		
	case OID_RT_SET_WMM_UAPSD_ENABLE:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SET_WMM_UAPSD_ENABLE:\n"));
		{	
			if ( InformationBufferLength < sizeof(AC_UAPSD) )
			{
				*BytesNeeded = sizeof(AC_UAPSD);
				return NDIS_STATUS_INVALID_LENGTH;
			}
			
			MgntActSet_802_11_WMM_UAPSD(GetDefaultAdapter(pAdapter), (AC_UAPSD)(*((pu1Byte)InformationBuffer)));

		}
		break;

	case OID_RT_DOT11D:
		{
			PRT_DOT11D_INFO pDot11Info = GET_DOT11D_INFO(pMgntInfo);

			if(InformationBufferLength < sizeof(u1Byte))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(u1Byte);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_DOT11D: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;
			}

			if( *((pu1Byte)InformationBuffer) == 0 )
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_DOT11D: disable\n"));
				pDot11Info->bEnabled = FALSE;
			}
			else
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_DOT11D: enable\n"));
				pDot11Info->bEnabled = TRUE;
			}
			*BytesRead = InformationBufferLength;
		}
		break;
	case OID_RT_MOTOR_BT_802_11_PAL:
		{
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Set OID_RT_MOTOR_BT_802_11_PAL\n"));
			RT_DISP(FIOCTL, IOCTL_STATE, ("[BT]Query OID_RT_MOTOR_BT_802_11_PAL\n"));
			return Status;
		}

	case OID_RT_FORCED_PROTECTION:
		{
			BOOLEAN 	bProtection_Enable;
			u1Byte		Protection_Rate;
			u1Byte		Protection_CtsFrame, Protection_RtsFrame;
			u1Byte		Protection_BW, Protection_SC, Protection_CCA;

			bProtection_Enable = *((u1Byte*)InformationBuffer) >> 4;

			if(bProtection_Enable == PROTECTION_MODE_FORCE_ENABLE)
			{
				// Verify input paramter.
				if(InformationBufferLength < 4)
				{
					Status = NDIS_STATUS_INVALID_LENGTH;
					*BytesNeeded = 4;
					return Status;
				}

				Protection_CCA	 	= *((u1Byte*)InformationBuffer) & 0x0f;
					
				Protection_Rate 		= *((pu1Byte)InformationBuffer+1);

				Protection_RtsFrame     = *((pu1Byte)InformationBuffer+2) >> 4;
				Protection_CtsFrame 	= *((pu1Byte)InformationBuffer+2) & 0x0f;

				
				Protection_BW 		= *((pu1Byte)InformationBuffer+3) >> 4;
				Protection_SC 		= *((pu1Byte)InformationBuffer+3) & 0x0f;

				if(	Protection_CtsFrame > 1 || Protection_RtsFrame > 1 || 
					Protection_BW > 3 || Protection_SC > 3 || Protection_CCA > 2)
				{
					Status = NDIS_STATUS_INVALID_DATA;
					return Status;					
				}

				pMgntInfo->ForcedProtectionMode = PROTECTION_MODE_FORCE_ENABLE;
				pMgntInfo->ForcedProtectRate = Protection_Rate;
				pMgntInfo->bForcedProtectRTSFrame = (Protection_RtsFrame==1)?TRUE:FALSE;
				pMgntInfo->bForcedProtectCTSFrame	= (Protection_CtsFrame==1)?TRUE:FALSE;
				pMgntInfo->ForcedProtectBW 	= Protection_BW;
				pMgntInfo->ForcedProtectSC 	= Protection_SC;
				pMgntInfo->ForcedProtectCCA	= Protection_CCA;
//				DbgPrint("Protection Mode %d Rate %d RTS %d CTS %d BW %d SC %d CCA %d\n",
//					pMgntInfo->ForcedProtectionMode, pMgntInfo->ForcedProtectRate>>1, pMgntInfo->bForcedProtectRTSFrame, 
//					pMgntInfo->bForcedProtectCTSFrame, pMgntInfo->ForcedProtectBW, pMgntInfo->ForcedProtectSC, pMgntInfo->ForcedProtectCCA);
			}
			else
			{
				pMgntInfo->ForcedProtectionMode = bProtection_Enable;
//				DbgPrint("Protection Mode %d\n", pMgntInfo->ForcedProtectionMode);
			}
		}
		break;
		
	case OID_RT_RF_OFF:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_RF_OFF\n") );
		{ 
			BOOLEAN bRfOff = FALSE;

			*BytesNeeded = sizeof(BOOLEAN);
			if( InformationBufferLength < *BytesNeeded )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 
			bRfOff = *((PBOOLEAN)InformationBuffer);
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OID_RT_RF_OFF: bRfOff(%d)\n", bRfOff));

			Status = N6CSet_DOT11_NIC_POWER_STATE(pAdapter, pNdisCommon->PendedRequest, !bRfOff);
			*BytesRead = *BytesNeeded;
		}
		break;

	//Turbo mode mechanism, added by Roger. 2007.02.06.
	case OID_RT_TURBOMODE:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_TURBOMODE\n"));
		{	
			// Verify input.
			if( (InformationBuffer == NULL) || (InformationBufferLength == 0) )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			// Set mgturbo mode type.
			MgntActSet_RT_TurboModeType( pAdapter, (pu1Byte)InformationBuffer );
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_TURBOMODE.\n"));
		*BytesRead = InformationBufferLength;
		break;
	
#if (WPS_SUPPORT == 1)		
	// SimpleConfig ,By CCW copy from 818x
	case OID_RT_SimpleConfScan:
		{
			PSIMPLE_CONFIG_T pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);

			RT_PRINT_DATA(COMP_MLME, DBG_LOUD, 
				"Set OID_RT_SimpleConfScan\n", 
				InformationBuffer, InformationBufferLength);

			//
			// 2011-05-13 modify by hpfan, for WSC IE support fragment
			// 
 
			// buffer format:
			// 1-byte: switch of simple config mode [BIT0] + IE length high byte [BIT7 ~ BIT1]
			// 1-byte: length of IE low byte
			// n-byte: content of IE.
			//

			// 
			// buffer format: [Support 1byte IE length]
			// 1-byte: switch of simple config mode.
			// 1-byte: length of IE.
			// n-byte: content of IE.
			//

			//RT_TRACE(COMP_RSNA, DBG_LOUD, ("<=== OID_RT_SimpleConfScan ", Adapter->MgntInfo.bCCX8021xResultenable));
			if(InformationBufferLength < 1)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				pSimpleConfig->bEnabled = FALSE;
				*BytesNeeded = 1;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SimpleConfScan: inavalid InformationBufferLength(%d), BytesNeeded is %d\n", InformationBufferLength, *BytesNeeded) );
				return Status;
			}

			if( (*(PUCHAR)InformationBuffer & BIT0) == 0 )
			{ // Simple config is OFF.
				u2Byte IELen;
				pSimpleConfig->bEnabled = FALSE;
				*BytesRead = InformationBufferLength;

				//
				// Get IE length.
				//
				if(InformationBufferLength < 2)
				{
					Status = NDIS_STATUS_INVALID_LENGTH;
					*BytesNeeded = 2;
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SimpleConfScan: inavalid InformationBufferLength(%d), BytesNeeded is %d\n", InformationBufferLength, *BytesNeeded) );
					pSimpleConfig->bEnabled = FALSE;
					pSimpleConfig->bAPEnabled = FALSE;
					pSimpleConfig->IELen = 0;
					PlatformFillMemory(pSimpleConfig->IEBuf,MAX_SIMPLE_CONFIG_IE_LEN_V2,0);
					return Status;
				}
				IELen = *((u1Byte *)InformationBuffer + 1);

				if( *(PUCHAR)InformationBuffer > BIT0 )
				{
					IELen += ( ((*(PUCHAR)InformationBuffer) >> 1) << 8 );
				}
				
				//
				// Validate IE length. 35 is mimmum IE size.
				//
				if( (InformationBufferLength < (ULONG)(IELen)) ||  // IELen+1 will cause all IE data drop,Because IElen = InformationBufferLength ;			
					IELen < 35 || IELen > MAX_SIMPLE_CONFIG_IE_LEN_V2)
				{
					Status = NDIS_STATUS_INVALID_LENGTH;
					*BytesNeeded = 2 + 35;
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SimpleConfScan: inavalid InformationBufferLength(%d), BytesNeeded is %d\n", InformationBufferLength, *BytesNeeded) );
					pSimpleConfig->bEnabled = FALSE;
					pSimpleConfig->bAPEnabled = FALSE;
					return Status;
				}

				// Verify if AP mode exist
				if(IsExtAPModeExist(pAdapter))
				{
					pSimpleConfig->bAPEnabled = TRUE;
				}
				
				//
				// Enable Simple Config and update IE.
				//
				pSimpleConfig->IELen = IELen;
				PlatformFillMemory(pSimpleConfig->IEBuf,MAX_SIMPLE_CONFIG_IE_LEN_V2 ,0);
				CopyMem(pSimpleConfig->IEBuf, 
						((u1Byte *)InformationBuffer + 2), 
						pSimpleConfig->IELen);
				
				*BytesRead = InformationBufferLength;
			}
			else
			{ // Simple config is ON.
				u2Byte IELen;

				//
				// Get IE length.
				//
				if(InformationBufferLength < 2)
				{
					Status = NDIS_STATUS_INVALID_LENGTH;
					*BytesNeeded = 2;
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SimpleConfScan: inavalid InformationBufferLength(%d), BytesNeeded is %d\n", InformationBufferLength, *BytesNeeded) );
					pSimpleConfig->bEnabled = FALSE;
					return Status;
				}
				IELen = *((u1Byte *)InformationBuffer + 1);

				if( *(PUCHAR)InformationBuffer > BIT0 )
				{
					IELen += ( ((*(PUCHAR)InformationBuffer) >> 1) << 8 );
				}
				
				//
				// Validate IE length. 35 is mimmum IE size.
				//
				if( (InformationBufferLength < (ULONG)(IELen)) ||  // IELen+1 will cause all IE data drop,Because IElen = InformationBufferLength ;			
					IELen < 35 || IELen > MAX_SIMPLE_CONFIG_IE_LEN_V2 )
				{
					Status = NDIS_STATUS_INVALID_LENGTH;
					*BytesNeeded = 2 + 35;
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SimpleConfScan: inavalid InformationBufferLength(%d), BytesNeeded is %d\n", InformationBufferLength, *BytesNeeded) );
					pSimpleConfig->bEnabled = FALSE;
					return Status;
				}
				
				// Verify if AP mode exist
				if(IsExtAPModeExist(pAdapter))
				{
					pSimpleConfig->bAPEnabled = TRUE;
				}
				
				//
				// Enable Simple Config and update IE.
				//
				if(ACTING_AS_IBSS(pAdapter))
				    MgntDisconnect(pAdapter, disas_lv_ss);
				pSimpleConfig->bEnabled = TRUE;
				pSimpleConfig->IELen = IELen;
				PlatformFillMemory(pSimpleConfig->IEBuf,MAX_SIMPLE_CONFIG_IE_LEN_V2,0);
				CopyMem(pSimpleConfig->IEBuf, 
						((u1Byte *)InformationBuffer + 2), 
						pSimpleConfig->IELen);
				
				*BytesRead = InformationBufferLength;
			}
		}
		break;
	case OID_RT_WPS_SET_IE_FRAGMENT:
		{
			PSIMPLE_CONFIG_T pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);
			if( *((pu1Byte)InformationBuffer) == 0 )
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_SET_IE_FRAGMENT: disable\n"));
				pSimpleConfig->bFragmentIE = FALSE;
			}
			else
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_SET_IE_FRAGMENT: enable\n"));
				pSimpleConfig->bFragmentIE = TRUE;
			}
		}
		break;
#endif	// #if (WPS_SUPPORT == 1)

	case	OID_RT_WPS_HWSET_PBC_PRESSED:
		{
			if( InformationBufferLength < sizeof(BOOLEAN))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(BOOLEAN);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_HWSET_PBC_PRESSED: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;
			}

			// 2011/11/03 MH Add for netgear special requirement. We need to count PBC press time.
			if (pMgntInfo->bRegTimerGPIO != TRUE)
				pMgntInfo->bPbcPressed= FALSE;
			*BytesRead = InformationBufferLength;
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_HWSET_PBC_PRESSED: %d\n", pMgntInfo->bPbcPressed) );
		}
		break;

	case OID_RT_WPS_LED_CTL_START:
		{
			
			if( InformationBufferLength < sizeof(u1Byte))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = sizeof(BOOLEAN);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_LED_CTL_START: invalid length(%d), BytesNeeded: %d !!!\n", InformationBufferLength, *BytesNeeded));
				return Status;
			}
			
			if( *((pu1Byte)InformationBuffer) == 0 )
			{
				pAdapter->HalFunc.LedControlHandler(pAdapter, LED_CTL_STOP_WPS);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_LED_CTL_START: stop LED blinking SUCCESS\n"));				
			}
			else if( *((pu1Byte)InformationBuffer) == 1 )
			{
				pAdapter->HalFunc.LedControlHandler(pAdapter, LED_CTL_START_WPS);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_LED_CTL_START: start LED blinking 0.3s\n"));
			}
			else if( *((pu1Byte)InformationBuffer) == 2 )
			{				
				pAdapter->HalFunc.LedControlHandler(pAdapter, LED_CTL_START_WPS_BOTTON);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_LED_CTL_START_BOTTON: start LED blinking 0.3s\n"));
			}
			else if( *((pu1Byte)InformationBuffer) == 3 )
			{
				pAdapter->HalFunc.LedControlHandler(pAdapter, LED_CTL_STOP_WPS_FAIL);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_LED_CTL_START_BOTTON: stop LED blinking fail\n"));
			}
			else if ( *((pu1Byte)InformationBuffer) == 4 )
			{
				pAdapter->HalFunc.LedControlHandler(pAdapter, LED_CTL_STOP_WPS_FAIL_OVERLAP);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_LED_CTL_START_BOTTON: stop LED blinking fail session overlap\n"));			
			}
			else
			{				
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_WPS_LED_CTL_START_BOTTON: WRONG parameter\n"));			
			}
			
			*BytesRead = InformationBufferLength;
		}
		break;

	case OID_RT_FPGA_PHY_RDY:
		{
			if(*((pu1Byte)InformationBuffer))
			{
				RT_TRACE(COMP_INIT, DBG_LOUD, ("OID_RT_FPGA_PHY_RDY, set Enabled\n"));
				RT_ENABLE_FUNC(pAdapter, DF_TX_BIT);
			}
			else
			{
				RT_TRACE(COMP_INIT, DBG_LOUD, ("OID_RT_FPGA_PHY_RDY, set Disabled\n"));
				RT_DISABLE_FUNC(pAdapter, DF_TX_BIT);
			}

			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_FPGA_PHY_RDY: PHY Enabled(%d)\n",  *((pu1Byte)InformationBuffer)));
		}
		break;

	case OID_RT_CUSTOMIZED_SCAN:
		{
			RT_TRACE(COMP_OID_SET , DBG_LOUD, ("Set OID_RT_CUSTOMIZED_SCAN:\n"));
			N6CSet_DOT11_CUSTOMIZED_SCAN_REQUEST(GetDefaultAdapter(pAdapter), 
				InformationBuffer,
				InformationBufferLength,
				BytesRead,
				BytesNeeded);
		}
		break;

	case OID_RT_CONTROL_LPS:
		{
			BOOLEAN	bEnableLPS;
			PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
			PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

			bEnableLPS = (*(PBOOLEAN *)InformationBuffer) ? (TRUE):(FALSE);

			if(bEnableLPS)
			{
				pPSC->bDisableLPSByOID = FALSE;
				// Return LPS by UpdateLPSStatus(). by tynli. 2011.01.04.
				pAdapter->HalFunc.UpdateLPSStatusHandler(
						pAdapter, 
						pPSC->RegLeisurePsMode, 
						pPSC->RegPowerSaveMode);
				RT_TRACE(COMP_OID_SET, DBG_LOUD,("Set OID_RT_CONTROL_LPS: Recover LPS. bLeisurePs=%d\n", pPSC->bLeisurePs));
			}
			else
			{
				pPSC->bDisableLPSByOID = TRUE;
				LeisurePSLeave(pAdapter, LPS_DISABLE_OID_SET);
				pPSC->bLeisurePs = FALSE;
				RT_TRACE(COMP_OID_SET, DBG_LOUD,("Set OID_RT_CONTROL_LPS: Disable LPS. bLeisurePs=%d\n", pPSC->bLeisurePs));
			}
		}
		break;

	case OID_RT_CONTROL_IPS:
		{
			BOOLEAN	bEnableIPS;
			PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
			PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

			bEnableIPS = (*(PBOOLEAN *)InformationBuffer) ? (TRUE):(FALSE);

			if(bEnableIPS)
			{
				IPSReturn(pAdapter, IPS_DISABLE_BY_OID);
				RT_TRACE(COMP_OID_SET, DBG_LOUD,("Set OID_RT_CONTROL_IPS: IPS Return\n"));
			}
			else
			{
				IPSDisable(pAdapter, FALSE, IPS_DISABLE_BY_OID);
				RT_TRACE(COMP_OID_SET, DBG_LOUD,("Set OID_RT_CONTROL_IPS: IPS Disable\n"));
			}
		}

		break;

	case OID_RT_CLEAR_ANTENNA_TEST_VAL:
		{
			pMgntInfo->AntennaTest = 0;
			RT_TRACE(COMP_OID_SET, DBG_LOUD,("Set OID_RT_CLEAR_ANTENNA_TEST_VAL\n"));
		}
		break;
			
	case OID_RT_11N_TDLS_ENABLE:
		{
			TDLS_SetConfiguration(pAdapter, InformationBuffer, InformationBufferLength);
		}
		break;

		case OID_RT_CUSTOMIZED_ASSOCIATION_PARAM:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> SET OID_RT_CUSTOMIZED_ASSOCIATION_PARAM\n"));
		{
			if(InformationBufferLength < 2)
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = 2;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_RT_CUSTOMIZED_ASSOCIATION_PARAM invalid length\n"));
				return Status;
			}
			MgntActSet_802_11_CustomizedAsocIE(pAdapter, InformationBuffer, (u1Byte)InformationBufferLength);
		}
		break;	

		case OID_RT_LC_STOP_SCAN:
			{
				
				if(InformationBufferLength < sizeof(ULONG))
				{
					Status = NDIS_STATUS_INVALID_LENGTH;
					*BytesNeeded = sizeof(ULONG);
					return Status;
				}
				switch(*(ULONG *)InformationBuffer)
				{
					case 0:
						pMgntInfo->bStopScan = FALSE;
						break;

					case 1:
						pMgntInfo->bStopScan = TRUE;
						// 
						MgntDisconnect(pAdapter, unspec_reason);

						break;
					default :
						pMgntInfo->bStopScan = FALSE;
						break;
				}
			}
			break;		

		case OID_RT_PNP_POWER:
		{
			Status = N6C_SET_OID_PNP_SET_POWER(
					pAdapter,
					OID_RT_PNP_POWER,
					InformationBuffer,
					InformationBufferLength,
					BytesRead,
					BytesNeeded);
		}
		break;

		case OID_RT_WPS_INFORMATION:
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OID_RT_WPS_INFORMATION\n"));
#if ( WPS_SUPPORT == 1 )
			if(RT_STATUS_SUCCESS !=
				(rtStatus = MgntActSet_WPS_Information(pAdapter, InformationBuffer, (u2Byte)InformationBufferLength)))
			{
				Status = NDIS_STATUS_INVALID_DATA;
				return Status;					
			}
#else
			return NDIS_STATUS_FAILURE;
#endif
		}
		break;

	case OID_RT_SYSTEM_POWER_STATE_SHUTDOWN:
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_RT_SYSTEM_POWER_STATE_SHUTDOWN ===>\n"));
			pMgntInfo->bReceiveSystemPSOID = TRUE;
		}
		break;


	case OID_RT_FORCED_BUG_CHECK:
		{
			//
			// <Roger_Notes> Add forced bug check operation(OID_RT_FORCED_BUG_CHECK) to observe some specific HW behavior, 
			// e.g., to check whether SDIO device will be recognized by device management after unexpected system restart
			// 2014.07.05.
			//
			KeBugCheckEx(
				OID_RT_FORCED_BUG_CHECK, 
				(ULONG) GET_RT_SDIO_DEVICE(pAdapter)->CurrentPowerState, 
				(ULONG)pDefaultAdapter->bSurpriseRemoved,				 
				(ULONG)pDefaultAdapter->bDriverStopped,
				(ULONG)pDefaultAdapter->bEnterPnpSleep);
		}
		break;

	}
	return Status;
}

//
//	Description:
//		Handler for setting OIDs whcih are common to PCI and USB devices.
//
NDIS_STATUS
N6CSetInformation(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
	)
{
	PADAPTER				pAdapter = (PADAPTER)MiniportAdapterContext;
	PMGNT_INFO      			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("====> N6CSetInformation, OID=0x%08x\n", Oid));

	// Moved From Orginal MPSetInformation Set OID (Too Many Interface Dependent OIDs; therefore, separate them into different files) ---
	Status = InterfaceSetInformationHandleCustomizedOriginalMPSetOid(
			MiniportAdapterContext, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesRead, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized Wifi-Direct OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// ----------------------------------------------------------------------------------------------------------------


	// CCX Handle Set OID --------------------------------------------------------------------------------
	Status = NDIS_STATUS_NOT_RECOGNIZED;

	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{
		// This Oid has been responded to by the MpEventQueryInformation function
        	// We will not handle this Oid and return to the OS.
       
		RT_TRACE(COMP_CCX, DBG_TRACE,  ("Oid intercepted by CCX EventSetInformation! Status 0x%08x\n", Status));
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------


	// Handle the Wifi-Direct Customized Oids -----------------------------------------------------------------
	Status = N6CSetInformationHandleCustomizedWifiDirectOids(
			MiniportAdapterContext, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesRead, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized Wifi-Direct OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------


	// Handle the WAPI Customized Oids ---------------------------------------------------------------------
	Status = WAPI_OidHandler(
			pAdapter, 
			1, //N6 Platform
			1, //Set OID
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesRead, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized WAPI OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------


	// Handle the Customized 11n Oids -----------------------------------------------------------------------
	Status = N6CSetInformationHandleCustomized11nOids(
			MiniportAdapterContext, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesRead, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized 11n OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------


	// Handle the Customized Security Oids --------------------------------------------------------------------------
	Status = N6CSetInformationHandleCustomizedSecurityOids(
			MiniportAdapterContext, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesRead, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------
	

	// Handle the Customized NAN Oids --------------------------------------------------------------------------
	Status = N6CSetInformationHandleCustomizedNANOids(
			MiniportAdapterContext, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesRead, 
			BytesNeeded
		);

	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized NAN OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------

	// Handle the Customized Oids --------------------------------------------------------------------------
	Status = N6CSetInformationHandleCustomizedOids(
			MiniportAdapterContext, 
			Oid, 
			InformationBuffer,
			InformationBufferLength, 
			BytesRead, 
			BytesNeeded
		);
	
	if(Status != NDIS_STATUS_NOT_RECOGNIZED)
	{	
		// The Customized OID has been handled.
		return Status;
	}
	else Status = NDIS_STATUS_SUCCESS;
	// --------------------------------------------------------------------------------------------------



	// ============================================================================================
	// The following are the OIDs from the NDIS 6.x, including the Customized OIDs with the same meanings. Please be organized. 
	// ============================================================================================
	
	*BytesRead = 0;
	*BytesNeeded = 0;

	switch(Oid)
	{
	case OID_GEN_CURRENT_LOOKAHEAD:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_GEN_CURRENT_LOOKAHEAD: \n"));
		break;

	case OID_GEN_PROTOCOL_OPTIONS:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_GEN_PROTOCOL_OPTIONS: \n"));
		break;

	case OID_802_3_MULTICAST_LIST:
		if( (InformationBuffer == 0) || (InformationBufferLength == 0) || (InformationBufferLength%6 != 0))
		{
			Status = NDIS_STATUS_INVALID_LENGTH;
			return Status;
		}

		MgntActSet_802_3_MULTICAST_LIST(
			pAdapter,
			(pu1Byte)InformationBuffer,
			(u4Byte)InformationBufferLength,
			(BOOLEAN)(pNdisCommon->NdisPacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST));

		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_802_3_MULTICAST_LIST: \n"));
		break;
		
	case OID_DOT11_DESIRED_BSS_TYPE:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_DESIRED_BSS_TYPE:\n"));
		{
			PDOT11_BSS_TYPE pBssType = (PDOT11_BSS_TYPE)InformationBuffer;
			u4Byte len = sizeof(DOT11_BSS_TYPE);
			if ( InformationBufferLength < len )
			{
				Status = NDIS_STATUS_BUFFER_TOO_SHORT;
				*BytesNeeded = len;
				return Status;
			}

			Status = N6CSet_DOT11_DESIRED_BSS_TYPE(pAdapter, pBssType);
			if (Status == NDIS_STATUS_SUCCESS)
			{
				*BytesRead = len;
			}
		}
		break;
		
	case OID_DOT11_MEDIA_STREAMING_ENABLED:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_MEDIA_STREAMING_ENABLED::\n"));
		{
			BOOLEAN		bStreamingEnabled = *((PBOOLEAN)InformationBuffer);

			if ( InformationBufferLength < sizeof(BOOLEAN) )
			{
				*BytesRead = 0;
				*BytesNeeded = sizeof(BOOLEAN);
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			*BytesRead = sizeof(BOOLEAN);
			pNdisCommon->bDot11MediaStreamingEnabled = bStreamingEnabled;
		}
		break;

	case OID_DOT11_EXCLUDE_UNENCRYPTED:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_EXCLUDE_UNENCRYPTED:\n"));
		{
			if ( InformationBufferLength < sizeof(BOOLEAN) )
			{
				*BytesRead = 0;
				*BytesNeeded = sizeof(BOOLEAN);
				return NDIS_STATUS_BUFFER_TOO_SHORT;
			}

			pMgntInfo->bExcludeUnencrypted = *((PBOOLEAN)InformationBuffer);
			*BytesRead = sizeof(BOOLEAN);
		}
		break;

	case OID_DOT11_PRIVACY_EXEMPTION_LIST:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_PRIVACY_EXEMPTION_LIST:\n"));
		{
			return N6CSet_DOT11_PRIVACY_EXEMPTION_LIST(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesRead,
					BytesNeeded);
		}
		break;
		
	case OID_DOT11_IBSS_PARAMS:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_IBSS_PARAMS:\n"));
		{
			return N6CSet_DOT11_IBSS_PARAMS(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesRead,
					BytesNeeded);
		}
		break;
		
	// 2006.10.31, by shien chang.
	case OID_RT_FRAGMENTATION_THRESHOLD:
	case OID_DOT11_FRAGMENTATION_THRESHOLD:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_FRAGMENTATION_THRESHOLD:\n"));
		{
			if ( InformationBufferLength < sizeof(ULONG) )
			{
				*BytesNeeded = sizeof(ULONG);
				return NDIS_STATUS_INVALID_LENGTH;
			}

			MgntActSet_802_11_FRAGMENTATION_THRESHOLD( pAdapter, *((pu2Byte)InformationBuffer) );
			*BytesRead = sizeof(ULONG);	
		}
		break;

	// 2006.10.31, by shien chang.
	case OID_RT_RTS_THRESHOLD:
	case OID_DOT11_RTS_THRESHOLD:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_RTS_THRESHOLD:\n"));
		{
			if ( InformationBufferLength < sizeof(ULONG) )
			{
				*BytesNeeded = sizeof(ULONG);
				return NDIS_STATUS_BUFFER_OVERFLOW;
			}

			MgntActSet_802_11_RTS_THRESHOLD( pAdapter, *(pu2Byte)InformationBuffer );
			*BytesRead = sizeof(ULONG);
		}
		break;

	// 2006.11.01, by shien chang.
	case OID_DOT11_FLUSH_BSS_LIST:
		RT_TRACE(COMP_OID_SET , DBG_LOUD, ("Set OID_DOT11_FLUSH_BSS_LIST:\n"));
		{
			// Mark the scan list has been flushed by OS.
			pMgntInfo->bFlushScanList = TRUE;

			// Clear content of bssDesc4Query[].
			PlatformZeroMemory( pMgntInfo->bssDesc4Query, sizeof(RT_WLAN_BSS)*MAX_BSS_DESC);
			pMgntInfo->NumBssDesc4Query = 0;
			RT_TRACE(COMP_SCAN, DBG_LOUD, 
				("[REDX]: OID_DOT11_FLUSH_BSS_LIST(), clear NumBssDesc4Query\n"));
			MgntClearRejectedAsocAP(pAdapter);
			
			*BytesRead = 0;
		}
		break;

	case OID_DOT11_CURRENT_REG_DOMAIN:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Set OID_DOT11_CURRENT_REG_DOMAIN:\n"));
		{
			*BytesNeeded = sizeof(ULONG);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			//<SC_TODO: do something>
		}
		break;

	case OID_DOT11_DESIRED_BSSID_LIST:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_DESIRED_BSSID_LIST:\n"));
		{
			return N6CSet_DOT11_DESIRED_BSSID_LIST(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesRead,
					BytesNeeded);
		}
		break;
		
	case OID_DOT11_DATA_RATE_MAPPING_TABLE:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_DATA_RATE_MAPPING_TABLE:\n"));
		{

		}
		break;
		
	case OID_DOT11_MULTI_DOMAIN_CAPABILITY_ENABLED:
	case OID_DOT11_COUNTRY_STRING:
		Status = NDIS_STATUS_NOT_SUPPORTED;
		break;


	// Added by Annie, 2006-10-09.
	case OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM\n") );
		{
			return N6CSet_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(
					pAdapter,
					InformationBuffer, 
					InformationBufferLength,
					BytesRead,
					BytesNeeded);
		}
		break;

	// Added by Annie, 2006-10-09.
	case OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM\n") );
		{
			return N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(
					pAdapter, 
					InformationBuffer,
					InformationBufferLength,
					BytesRead,
					BytesNeeded
					);
		}
		break;

	// Added by Annie, 2006-10-12.
	case OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM, 0x%08x\n", Oid) );
		{
			return N6CSet_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM( 
					pAdapter,
					InformationBuffer, 
					InformationBufferLength,
					BytesRead,
					BytesNeeded
					);
		}
		break;

	// Added by Annie, 2006-10-13.
	case OID_DOT11_CIPHER_DEFAULT_KEY:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_CIPHER_DEFAULT_KEY, 0x%08x\n", Oid) );
		{
			KIRQL CurrIrql = KeGetCurrentIrql();
			if(CurrIrql < DISPATCH_LEVEL)
			{
				KeRaiseIrql(DISPATCH_LEVEL, &CurrIrql);
			
				Status = N6CSet_DOT11_CIPHER_DEFAULT_KEY(
							pAdapter, 
							InformationBuffer,
							InformationBufferLength,
							BytesRead,
							BytesNeeded);
				
				KeLowerIrql(CurrIrql);
			}
		}
		break;

	// Added by Annie, 2006-10-09.
	case OID_DOT11_CIPHER_DEFAULT_KEY_ID:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_CIPHER_DEFAULT_KEY_ID, 0x%08x\n", Oid) );
		{
			PRT_SECURITY_T		pSecInfo = &(pAdapter->MgntInfo.SecurityInfo);
			u4Byte				ulKey;
			
			if (InformationBufferLength < sizeof(u4Byte))
			{
				*BytesNeeded = sizeof(u4Byte);
				return NDIS_STATUS_INVALID_LENGTH;
			}

			ulKey = *((pu4Byte)InformationBuffer);

			if( ulKey >=  NATIVE_802_11_MAX_DEFAULT_KEY_ENTRY)
			{
				return NDIS_STATUS_INVALID_DATA;
			}
			
			if(WAPI_QuerySetVariable(pAdapter, WAPI_QUERY, WAPI_VAR_WAPIENABLE, 0))
				return NDIS_STATUS_SUCCESS;

			pSecInfo->DefaultTransmitKeyIdx = (u1Byte)ulKey;
			RT_TRACE_F(COMP_SEC, DBG_LOUD, ("OID_DOT11_CIPHER_DEFAULT_KEY_ID = %d\n", pSecInfo->DefaultTransmitKeyIdx));
			pNdisCommon->RegDefaultKeyId = ulKey;
			*BytesRead = sizeof(u4Byte);
		}
		break;

	// Added by Annie, 2006-10-19.
	case OID_DOT11_CIPHER_KEY_MAPPING_KEY:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_CIPHER_KEY_MAPPING_KEY, 0x%08x\n", Oid) );
		{
			KIRQL CurrIrql = KeGetCurrentIrql();
			if(CurrIrql < DISPATCH_LEVEL)
			{
				KeRaiseIrql(DISPATCH_LEVEL, &CurrIrql);

				Status = N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(
						pAdapter, 
						InformationBuffer,
						InformationBufferLength,
						BytesRead,
						BytesNeeded);
				
				KeLowerIrql(CurrIrql);
			}
		}
		break;
		
	// Added by Annie, 2006-10-09.
	case OID_DOT11_PMKID_LIST:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_PMKID_LIST, 0x%08x\n", Oid) );
		{
			PDOT11_PMKID_LIST	pPMKIDList;

			if (InformationBufferLength < (ULONG)FIELD_OFFSET(DOT11_PMKID_LIST, PMKIDs))
			{
				*BytesNeeded = FIELD_OFFSET(DOT11_PMKID_LIST, PMKIDs);
				Status = NDIS_STATUS_BUFFER_OVERFLOW;
				return Status;
			}

			pPMKIDList = (PDOT11_PMKID_LIST)InformationBuffer;
			Status = N6CSet_DOT11_PMKID_LIST( pAdapter, pPMKIDList, InformationBufferLength );
		}
		break;

	case OID_RT_DISCONNECT_REQUEST:
		RT_TRACE( COMP_WPS, DBG_LOUD, ("WPS Set DISCONEECT REQUEST\n") );		
		{
			Status = N6CSet_DOT11_DISCONNECT_REQUEST(
						pAdapter,
						InformationBuffer,
						InformationBufferLength,
						BytesRead,
						BytesNeeded);
		}
		break;

	// Added by Annie, 2006-10-16.
	case OID_RT_POWER_MGMT_REQUEST:
	case OID_DOT11_POWER_MGMT_REQUEST:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_POWER_MGMT_REQUEST, 0x%08x\n", Oid) );
		{
			*BytesNeeded = sizeof(ULONG);
			if( InformationBufferLength < *BytesNeeded )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			Status = N6CSet_DOT11_POWER_MGMT_REQUEST(pAdapter, InformationBuffer);
			*BytesRead = *BytesNeeded;
		}
		break;


	// Added by Annie, 2006-10-18.
	case OID_DOT11_EXCLUDED_MAC_ADDRESS_LIST:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_EXCLUDED_MAC_ADDRESS_LIST, 0x%08x\n", Oid) );
		{
			return N6CSet_DOT11_EXCLUDED_MAC_ADDRESS_LIST(
					pAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesRead,
					BytesNeeded);
		}
		break;
	//

	// 061012, by rcnjko. 
	case OID_DOT11_ATIM_WINDOW:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_ATIM_WINDOW\n") );
		{ 
			*BytesNeeded = sizeof(ULONG);
			if( InformationBufferLength < *BytesNeeded )
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				break;
			} 

			Status = N6CSet_DOT11_ATIM_WINDOW(pAdapter, InformationBuffer);
			*BytesRead = *BytesNeeded;
		}
		break;

	// 061014, by rcnjko. 
	case OID_RT_OPERATIONAL_RATE_SET:
	case OID_DOT11_OPERATIONAL_RATE_SET:
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_OPERATIONAL_RATE_SET\n") );
		{ 
			return N6CSet_DOT11_OPERATIONAL_RATE_SET(
						pAdapter, 
						InformationBuffer,
						InformationBufferLength,
						BytesRead,
						BytesNeeded);
		}
		break;

	// 2006.11.02, by shien chang.
	case OID_DOT11_UNREACHABLE_DETECTION_THRESHOLD:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_UNREACHABLE_DETECTION_THRESHOLD:\n"));
		{
			*BytesNeeded = sizeof(ULONG);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			pNdisCommon->dot11UnreachableDetectionThreshold = *((PULONG)InformationBuffer);
			*BytesRead = *BytesNeeded;
		}
		break;

	case OID_DOT11_UNICAST_USE_GROUP_ENABLED:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_UNICAST_USE_GROUP_ENABLED:\n"));
		{
			*BytesNeeded = sizeof(BOOLEAN);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			pNdisCommon->dot11UnicastUseGroupEnabled = *((PBOOLEAN)InformationBuffer);
			*BytesRead = *BytesNeeded;
		}
		break;

	case OID_DOT11_HIDDEN_NETWORK_ENABLED:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_HIDDEN_NETWORK_ENABLED:\n"));
		{
			*BytesNeeded = sizeof(BOOLEAN);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			pMgntInfo->bHiddenSSIDEnable = *((PBOOLEAN)InformationBuffer);
			*BytesRead = *BytesNeeded;
		}
		break;
		
	case  OID_DOT11_CURRENT_CHANNEL:
	case  OID_DOT11_CURRENT_FREQUENCY:	
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_CURRENT_CHANNEL:\n"));
		{
			u1Byte btChannel;
			
			*BytesNeeded = sizeof(ULONG);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_INVALID_LENGTH;
			}

			btChannel = (u1Byte)(*(PULONG)InformationBuffer);
			if (MgntActSet_802_11_REG_20MHZ_CHANNEL_AND_SWITCH(pAdapter, btChannel) != TRUE)
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("!!! Failed to switch to the channel, %d\n", btChannel));
			}
		}
		break;
		
	case OID_DOT11_MULTICAST_LIST:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_MULTICAST_LIST:\n"));
		{
			Status = N6CSet_DOT11_MULTICAST_LIST(
						pAdapter,
						(pu1Byte)InformationBuffer,
						(u4Byte)InformationBufferLength,
						BytesRead,
						BytesNeeded);
		}
		break;

	case OID_RT_PRO_SET_TX_POWER_FOR_ALL_RATE:
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> Set OID_RT_PRO_SET_TX_POWER_FOR_ALL_RATE.\n"));
		{
			ULONG	ulTxPowerData;

			// Verify input paramter.
			if(InformationBufferLength < 3*sizeof(UCHAR))
			{
				Status = NDIS_STATUS_INVALID_LENGTH;
				*BytesNeeded = 3*sizeof(UCHAR);
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_SET_TX_POWER_FOR_ALL_RATE. return!!\n"));
				return Status;
			}
			// Get Type/Path/TxPower.
			ulTxPowerData = *((ULONG*)InformationBuffer);

			// Caller should use OID_RT_PRO8187_WI_POLL to get result.
			if(!DbgSetTxPowerForAllRate(GetDefaultAdapter(pAdapter), ulTxPowerData))
			{
				Status = NDIS_STATUS_NOT_ACCEPTED;
			}
		}
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_RT_PRO_SET_TX_POWER_FOR_ALL_RATE.\n"));
		break;

	//Add just for test. by luke
	case	OID_RT_11N_IQK_TRIGGER:
		{
			PHAL_DATA_TYPE pHalData = GET_HAL_DATA(pAdapter);

			RT_TRACE(COMP_OID_SET,DBG_LOUD,("set oid OID_RT_11N_IQK_TRIGGER\n"));
			PlatformScheduleWorkItem(&(pHalData->IQKTriggerWorkItem));
		}
		break;	

	case	OID_RT_PRO_TRIGGER_LCK:
		{
			PHAL_DATA_TYPE pHalData = GET_HAL_DATA(pAdapter);

			RT_TRACE(COMP_OID_SET,DBG_LOUD,("set oid OID_RT_PRO_TRIGGER_LCK\n"));
			PlatformScheduleWorkItem(&(pHalData->LCKTriggerWorkItem));
		}
		break;		

	case	OID_RT_PRO_TRIGGER_DPK:
		{
			PHAL_DATA_TYPE pHalData = GET_HAL_DATA(pAdapter);
			RT_TRACE(COMP_OID_SET,DBG_LOUD,("set oid OID_RT_PRO_TRIGGER_DPK\n"));
			PlatformScheduleWorkItem(&(pHalData->DPKTriggerWorkItem));
		}
		break;			

		// This is added for nested OID from CcxHandleNicSpecificExtension()
		case OID_DOT11_CURRENT_PHY_ID:
		{
			ULONG PhyId = *(PULONG)InformationBuffer;
			
			*BytesNeeded = sizeof(ULONG);
			if ( InformationBufferLength < *BytesNeeded )
			{
				return NDIS_STATUS_BUFFER_OVERFLOW;
			}

			if ( PhyId >= pAdapter->pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries )
			{
				*BytesRead = 0;
				return NDIS_STATUS_INVALID_DATA;
			}
			else
			{
				pAdapter->pNdisCommon->dot11SelectedPhyId = PhyId;
				pAdapter->pNdisCommon->pDot11SelectedPhyMIB = pAdapter->pNdisCommon->pDot11PhyMIBs + PhyId;
				*BytesRead = *BytesNeeded;
			}
		}
		break;

		// This is added for nested OID from CcxHandleNicSpecificExtension()
		case OID_DOT11_DESIRED_PHY_LIST:
		{
			Status = N6CSet_DOT11_DESIRED_PHY_LIST(
				pAdapter,
				InformationBuffer,
				InformationBufferLength,
				BytesRead,
				BytesNeeded
			);
		}
		break;

		// This is added for nested OID from CcxHandleNicSpecificExtension()
		case OID_DOT11_AUTO_CONFIG_ENABLED:
		{
			u4Byte	tempAutoConfigEnable = 0;
			*BytesNeeded = sizeof(ULONG);

			if ( InformationBufferLength < *BytesNeeded )
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_AUTO_CONFIG_ENABLED: Invalid length\n"));
				return NDIS_STATUS_INVALID_LENGTH;
			}

			tempAutoConfigEnable=ALLOWED_AUTO_CONFIG_FLAGS & *((PULONG)InformationBuffer);
			pAdapter->pNdisCommon->dot11AutoConfigEnabled = tempAutoConfigEnable;
				
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<===Set OID_DOT11_AUTO_CONFIG_ENABLED:\n"));
		}
		break;

	default:
		Status = NDIS_STATUS_INVALID_OID;
		break;
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<==== N6CSetInformation, OID=0x%08x, Status=0x%X\n", Oid, Status));
	return Status;
}


NDIS_STATUS
N6CQuerySetInformation(
	IN	PADAPTER	Adapter,
	IN	NDIS_OID	Oid,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	IN	ULONG		OutputBufferLength,
	IN	ULONG		MethodId,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
	)
{
	NDIS_STATUS		ndisStatus;
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	*BytesWritten = 0;
	*BytesRead = 0;
	*BytesNeeded = 0;

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("====> N6CQuerySetInformation, OID=0x%08x\n", Oid ));


	ndisStatus = WAPI_EventQuerySetInformation(
       	         Adapter,
			  Oid,
                     InformationBuffer,
                     InputBufferLength,
                     OutputBufferLength,
                     MethodId,
                     BytesWritten,
                     BytesRead,
                     BytesNeeded
                     );
	if (ndisStatus != NDIS_STATUS_INVALID_OID)
	{
       	 //
       	 // This Oid has been responded to by the MpEventQueryInformation function 
       	 // We will not handle this Oid and return to the OS.
       	 //
       	RT_TRACE(COMP_OID_QUERY, DBG_LOUD,  ("==>Oid intercepted by WAPI_EventQuerySetInformation! Status 0x%08x\n", ndisStatus));
		return ndisStatus;
	}
	
	//CCX Add for process IHV DLL Set OID down to Driver	
	ndisStatus = NDIS_STATUS_SUCCESS;
	if (ndisStatus != NDIS_STATUS_INVALID_OID)
	{
       	 //
       	 // This Oid has been responded to by the MpEventQueryInformation function
       	 // We will not handle this Oid and return to the OS.
       	 //
       	RT_TRACE(COMP_CCX, DBG_TRACE,  ("Oid intercepted by MpEventQuerySetInformation! Status 0x%08x\n", ndisStatus));
		return ndisStatus;
	}

	switch (Oid)
	{
	default:
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query/Set OID_Unknown: %08X\n", Oid));
		ndisStatus = NDIS_STATUS_INVALID_OID;
		break;
	
	case OID_RT_WFD_REQUEST:
		rtStatus = WFD_Reqeust(
					Adapter,
					InformationBuffer,
					InputBufferLength,
					OutputBufferLength,
					BytesWritten,
					BytesRead,
					BytesNeeded);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("OID_RT_WFD_REQUEST: rtStatus = %d\n", rtStatus));
		ndisStatus = NdisStatusFromRtStatus(rtStatus);
		break;

	case OID_RT_P2P_SERVICE_REQUEST:
		rtStatus = P2PSvc_Request(
					Adapter,
					InformationBuffer,
					InputBufferLength,
					OutputBufferLength,
					BytesWritten,
					BytesRead,
					BytesNeeded);
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("OID_RT_P2P_SERVICE_REQUEST: rtStatus = %d\n", rtStatus));
		ndisStatus = NdisStatusFromRtStatus(rtStatus);
		break;
	}

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("<==== N6CQuerySetInformation, OID=0x%08x, Status=0x%X\n", Oid, ndisStatus));
	return ndisStatus;
}

//
// Description:
//	Complete the pended OID.
// Arguments:
//	Adapter -
//		NIC context
//	OidType -
//		The OidType is verified if the pended OID is which we want to complete.
//	ndisStatus -
//		The ndis status included in the completion.
// By Bruce, 2008-10-29.
//
BOOLEAN
N6CompletePendedOID(
	IN 	PADAPTER 			Adapter,
	IN	RT_PENDED_OID_TYPE	OidType,
	IN	NDIS_STATUS			ndisStatus
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	BOOLEAN				bComplete = FALSE;
	NDIS_OID			Oid;
	PNDIS_OID_REQUEST   tmpPendedRequest = NULL;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===>N6CompletePendedOID  \n" ));
	PlatformAcquireSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);
	if(pNdisCommon->PendedRequest == NULL)
	{
		PlatformReleaseSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<===N6CompletePendedOID  \n" ));
		return FALSE;
	}
	
	//RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===>N6CompletePendedOID   PendedRequest %08X \n",pNdisCommon->PendedRequest  ));
	Oid = pNdisCommon->PendedRequest->DATA.METHOD_INFORMATION.Oid;

	//
	// Make sure we never complete the OID which is not really pended and never complete the one
	// that the procedure is not done.
	//
	switch(OidType)
	{
		case RT_PENDED_OID_DONT_CARE:
			bComplete = TRUE;
			break;

		case RT_PENDED_OID_RF_STATE:
			//if(Oid == (OID_RT_RF_OFF || OID_DOT11_NIC_POWER_STATE))
			if(Oid == OID_DOT11_NIC_POWER_STATE)
				bComplete = TRUE;
			break;

		case RT_PENDED_OID_PNP:
			if(Oid == OID_PNP_SET_POWER)
				bComplete = TRUE;
			break;
			
		case RT_PENDED_OID_CREATE_DELETE_MAC:
			if(Oid == OID_DOT11_CREATE_MAC || Oid == OID_DOT11_DELETE_MAC)
				bComplete = TRUE;
			break;

	default:
		break;
	}

	if(bComplete)
	{
		tmpPendedRequest = pNdisCommon->PendedRequest;
		pNdisCommon->PendedRequest = NULL;
	}
	
	PlatformReleaseSpinLock(Adapter, RT_PENDED_OID_SPINLOCK);

	if(bComplete)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, 
			("N6CompletePendedOID(): Complete pended OID(0x%08X)\n", Oid));

		N6C_DOT11_DUMP_OID(Oid);

		RT_CLEAR_DRV_STATE(Adapter, DrvStateNdisReqPended);
		
		NdisOIDHistoryUpdate(Adapter, tmpPendedRequest, RT_OID_HISTORY_COMPLETE);
			
		NdisMOidRequestComplete(
			pNdisCommon->hNdisAdapter,
			tmpPendedRequest,
			ndisStatus);

		return TRUE;
	}
	else
	{
		RT_TRACE(COMP_OID_QUERY, DBG_LOUD, 
			("N6CompletePendedOID(): Pended OID(0x%08X) does not match input OID type(%d)\n",
			Oid, OidType));
		return FALSE;
	}
}

NDIS_STATUS
N6CProcessOidRequest(
	IN  PADAPTER	pAdapter,
	IN  PNDIS_OID_REQUEST   NdisRequest,
	IN  BOOLEAN bSelfMadeNdisRequest	
)
{
	PADAPTER		pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);
	NDIS_STATUS		ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
	u2Byte			i = 0;
	u4Byte			portType = 0;
		
	// OID callback functions ------------------------------------------------------------
	// 	Hope: Let all NDIS OIDs be handled here with one entry point
	portType = pTargetAdapter->pNdis62Common->PortType;
		
	for(i = 0 ; i < sizeof(RT_SUPPORT_OIDs)/sizeof(RT_OID_ENTRY); i++)
	{
		if(NdisRequest->DATA.QUERY_INFORMATION.Oid == RT_SUPPORT_OIDs[i].Oid)
		{
			if(	(NdisRequest->RequestType == NdisRequestQueryInformation)||
				(NdisRequest->RequestType == NdisRequestQueryStatistics) )
			{
				RT_TRACE(COMP_OID_QUERY, DBG_LOUD,
					("[OID], %s to Port(%d) PortType(%d)\n", RT_SUPPORT_OIDs[i].szID, NdisRequest->PortNumber, portType));
			}
			else
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD,
					("[OID], %s to Port(%d) PortType(%d)\n", RT_SUPPORT_OIDs[i].szID, NdisRequest->PortNumber, portType));
			}
			
			ndisStatus = RT_SUPPORT_OIDs[i].Func(pAdapter, NdisRequest);
		
			return ndisStatus;
		}
	}
	// -------------------------------------------------------------------------------



	// ===============================================================
	// Hope: Let the following be only customized oids: However, it is not now: 2012.06.08
	// ===============================================================

		
	switch (NdisRequest->RequestType)
	{
		case NdisRequestQueryInformation:
		case NdisRequestQueryStatistics:
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CProcessOidRequest :: Query OID: 0x%08x\n", NdisRequest->DATA.QUERY_INFORMATION.Oid));
			ndisStatus = N6CQueryInformation(
						pTargetAdapter,
					NdisRequest->DATA.QUERY_INFORMATION.Oid,
					NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
					NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
					(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
					(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded
					);
			break;

		case NdisRequestSetInformation:
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CProcessOidRequest :: Set OID: 0x%08x\n", NdisRequest->DATA.SET_INFORMATION.Oid));
			ndisStatus = N6CSetInformation(
						pTargetAdapter,
					NdisRequest->DATA.SET_INFORMATION.Oid,
					NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
					NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
					(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
					(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
			break;

		case NdisRequestMethod:
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("N6CProcessOidRequest :: QSset  OID: 0x%08x\n", NdisRequest->DATA.METHOD_INFORMATION.Oid));
			ndisStatus = N6CQuerySetInformation(
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

	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_FLUSH_BSS_LIST(
	PADAPTER	pAdapter,
	PNDIS_OID_REQUEST  NdisRequest
	)
{	
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PMGNT_INFO		pDefaultMgntInfo = &pDefaultAdapter->MgntInfo;
	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;

	if(NdisRequest->RequestType == NdisRequestQueryInformation ||
		NdisRequest->RequestType == NdisRequestQueryStatistics ||
		NdisRequest->RequestType == NdisRequestMethod)
		return NDIS_STATUS_NOT_SUPPORTED;

	RT_TRACE(COMP_MLME,DBG_LOUD, ("====>N6C_OID_DOT11_FLUSH_BSS_LIST\n"));
	
	// when flush, should set this variable.
	pMgntInfo->bFlushScanList = TRUE;

	//
	// Clear content of bssDesc[].
	//
	PlatformZeroMemory( pDefaultMgntInfo->bssDesc, sizeof(RT_WLAN_BSS)*MAX_BSS_DESC );
	pDefaultMgntInfo->NumBssDesc = 0;
	//
	// Clear content of bssDesc4Query[].
	//
	PlatformZeroMemory( pDefaultMgntInfo->bssDesc4Query, sizeof(RT_WLAN_BSS)*MAX_BSS_DESC);
	pDefaultMgntInfo->NumBssDesc4Query = 0;
	RT_TRACE(COMP_SCAN, DBG_LOUD, 
		("[REDX]: N6C_OID_DOT11_FLUSH_BSS_LIST(), clear NumBssDesc4Query\n"));
	MgntClearRejectedAsocAP(pDefaultAdapter);

	if(pDefaultAdapter != pAdapter)
	{
		PlatformZeroMemory( pMgntInfo->bssDesc4Query, sizeof(RT_WLAN_BSS)*MAX_BSS_DESC);
		pMgntInfo->NumBssDesc4Query = 0;
		RT_TRACE(COMP_SCAN, DBG_LOUD, 
			("[REDX]: N6C_OID_DOT11_FLUSH_BSS_LIST(), clear NumBssDesc4Query Non def adapter\n"));
		PlatformZeroMemory( pMgntInfo->bssDesc, sizeof(RT_WLAN_BSS)*MAX_BSS_DESC );
		pMgntInfo->NumBssDesc = 0;
		MgntClearRejectedAsocAP(pAdapter);
	}

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6C_OID_DOT11_SCAN_REQUEST(
	PADAPTER	pAdapter,
	PNDIS_OID_REQUEST  NdisRequest
	)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER		pDevicePort = NULL;
	PADAPTER 		pClientPort = NULL;
	PADAPTER		pTargetAdapter = NULL;
	PP2P_INFO		pP2PInfo = NULL;
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6C_OID_DOT11_SCAN_REQUEST(): Oid(%#x)--->\n", NdisRequest->DATA.METHOD_INFORMATION.Oid));
	N6C_DOT11_DUMP_OID(NdisRequest->DATA.METHOD_INFORMATION.Oid);
	
	//
	// 2013/02/05 MH Add for NEC special request in Vista. When stram mode is 
	// enabled do not scan.
	//

	if(!N6_INIT_READY(pAdapter))
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6C_OID_DOT11_SCAN_REQUEST(): return by !N6_INIT_READY\n"));
		return NDIS_STATUS_DOT11_MEDIA_IN_USE;
	}

	if(NdisRequest->RequestType == NdisRequestQueryInformation ||
		NdisRequest->RequestType == NdisRequestQueryStatistics ||
			NdisRequest->RequestType == NdisRequestMethod)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6C_OID_DOT11_SCAN_REQUEST(): return by NDIS_STATUS_NOT_SUPPORTED\n"));
		return NDIS_STATUS_NOT_SUPPORTED;
	}
	
{
	// Reject the scan request to the Wi-Fi direct port -----------------------------
	pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte) NdisRequest->PortNumber);

	switch(pTargetAdapter->pNdis62Common->PortType)
	{
		case EXT_P2P_DEVICE_PORT:
		case EXT_P2P_ROLE_PORT:
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: NDIS_STATUS_INVALID_OID\n", __FUNCTION__));
			return NDIS_STATUS_INVALID_OID;
	}
	// ----------------------------------------------------------------------

	if(NdisRequest->PortNumber != 0)
	{
		RT_TRACE(COMP_OID_SET , DBG_LOUD, ("%s: NDIS_STATUS_INVALID_OID because of not port 0.\n", __FUNCTION__));
		return NDIS_STATUS_INVALID_OID;
	}
	
	// Block scan for a while if P2P handshake is going now.
	if(GetFirstClientPort(pAdapter) || GetFirstGOPort(pAdapter))
	{
		if(pDefaultAdapter->MgntInfo.NumBssDesc4Query == 0)
		{
			// OID_DOT11_FLUSH_BSS_LIST may be issued for obtaining all-new scan list.
			// No Bss in the BssList: Do the scanning if possible.
		}
		else if(0 != pTargetAdapter->LastScanCompleteTime && PlatformGetCurrentTime() < pTargetAdapter->LastScanCompleteTime + P2P_BLOCK_NORMAL_SCAN_PERIOD)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("[WARNING] Last scan complete time less than %d us, return NDIS_STATUS_DOT11_MEDIA_IN_USE!\n", P2P_BLOCK_NORMAL_SCAN_PERIOD));		
			
			return NDIS_STATUS_DOT11_MEDIA_IN_USE;
		}
	}
	
	// Only scan in the idle device port ----------------------------
	pDevicePort = GetFirstDevicePort(pAdapter);
	
	if(pDevicePort != NULL)
	{
		pP2PInfo = GET_P2P_INFO(pDevicePort);
		
		if(pP2PInfo->State != P2P_STATE_INITIALIZED /*&& pP2PInfo->State != P2P_STATE_DISABLED*/)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: NDIS_STATUS_DOT11_MEDIA_IN_USE\n", __FUNCTION__));
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: pP2PInfo->State: %d portnumber %d porttype %d portrole %d\n", __FUNCTION__, pP2PInfo->State, pDevicePort->pNdis62Common->PortNumber, pDevicePort->pNdis62Common->PortType, pP2PInfo->Role));
			return NDIS_STATUS_DOT11_MEDIA_IN_USE;
		}

//		if(MgntScanInProgress(&pDefaultAdapter->MgntInfo))
		if(!pDefaultAdapter->MgntInfo.bScanInProgress && pP2PInfo->pAdapter->MgntInfo.bScanInProgress)
		{
			// Postpone the extended listening for a while
			P2PExtendedListenResetCounter(pP2PInfo);
			
			// Stop potential device port extended listening 
			P2PScanListCeaseScan(pP2PInfo);

			// Delay for a while for running ScanTimer callback by context switch
			//	+ Otherwise, this scan will be skipped due to MgntScanInProgress flag
			delay_ms(20);
		}
	}

	// Do not scan in Waiting Wps Ready
	pClientPort = GetFirstClientPort(pAdapter);
	if(pClientPort != NULL)
	{
		pP2PInfo = GET_P2P_INFO(pClientPort);
		
		if(P2P_CLIETN_JOIN_GROUP_WPS_STATE_NONE != pP2PInfo->ClientJoinGroupContext.WpsState)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("NDIS_STATUS_DOT11_MEDIA_IN_USE\n"));
			RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("pP2PInfo->ClientJoinGroupContext.WpsState = %d\n", pP2PInfo->ClientJoinGroupContext.WpsState));
			//DbgPrint("<===N6C_OID_DOT11_SCAN_REQUEST(), !NDIS_STATUS_DOT11_MEDIA_IN_USE   111\n");
			return NDIS_STATUS_DOT11_MEDIA_IN_USE;
		}
	}
	// -------------------------------------------------------
}



	N6WriteEventLog(pAdapter, RT_SCAN_START, 0);

	ndisStatus = N6CSet_DOT11_SCAN_REQUEST(pDefaultAdapter, 
		NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
		NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
		(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
		(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded);

	//DbgPrint("<===N6C_OID_DOT11_SCAN_REQUEST(), !ndisStatus   %x\n",ndisStatus);
	
	return ndisStatus;

}

NDIS_STATUS
N6C_OID_DOT11_ENUM_BSS_LIST(
	PADAPTER	pAdapter,
	PNDIS_OID_REQUEST	NdisRequest)
{
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("[REDX]: N6C_OID_DOT11_ENUM_BSS_LIST() ===>\n"));
	if(NdisRequest->RequestType == NdisRequestSetInformation ||
		NdisRequest->RequestType == NdisRequestQueryInformation ||
			NdisRequest->RequestType == NdisRequestQueryStatistics)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("[REDX]: N6C_OID_DOT11_ENUM_BSS_LIST() <===, return by NOT_SUPPORTED\n"));
		return NDIS_STATUS_NOT_SUPPORTED;
	}

	return N6CQuerySet_DOT11_ENUM_BSS_LIST(
		GetDefaultAdapter(pAdapter),
		NdisRequest->DATA.METHOD_INFORMATION.InformationBuffer,
		NdisRequest->DATA.METHOD_INFORMATION.InputBufferLength,
		NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength,
		(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesWritten,
		(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesNeeded);
}


