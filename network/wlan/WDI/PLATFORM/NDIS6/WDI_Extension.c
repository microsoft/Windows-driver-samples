////////////////////////////////////////////////////////////////////////////////
//
//	File name:		WDI_Extension.c
//	Description:	WDI common functions.
//
//	Author:			hpfan
//                                Tommy
//
////////////////////////////////////////////////////////////////////////////////
#include "Mp_Precomp.h"


#if WPP_SOFTWARE_TRACE
#include "WDI_Extension.tmh"
#endif


RT_TASK_ENTRY RT_SUPPORT_TASKs[]=
{
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_OPEN, 
			OID_STR_WRAPPER("OID_WDI_TASK_OPEN"),
			TRUE,
			NULL,
			NULL,
			0
		},
		NDIS_STATUS_WDI_INDICATION_OPEN_COMPLETE,
		FALSE,
		28,
		WDI_OID_TASK_OPEN,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_CLOSE, 
			OID_STR_WRAPPER("OID_WDI_TASK_CLOSE"),
			TRUE,
			NULL,
			NULL,
			0
		},
		NDIS_STATUS_WDI_INDICATION_CLOSE_COMPLETE,
		FALSE,
		28,
		WDI_OID_TASK_CLOSE,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_SCAN, 
			OID_STR_WRAPPER("OID_WDI_TASK_SCAN"),
			FALSE,
			ParseWdiTaskScan,
			CleanupParsedWdiTaskScan,
			sizeof(WDI_SCAN_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_SCAN_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_SCAN,
		WDI_CANCEL_OID_TASK_SCAN,
		WDI_CANCEL_OID_TASK_SCAN,
		WDI_PRE_M4_OID_TASK_SCAN,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_P2P_DISCOVER, 
			OID_STR_WRAPPER("OID_WDI_TASK_P2P_DISCOVER"),
			FALSE,
			ParseWdiTaskP2pDiscover,
			CleanupParsedWdiTaskP2pDiscover,
			sizeof(WDI_TASK_P2P_DISCOVER_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_P2P_DISCOVERY_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_P2P_DISCOVER,
		WDI_CANCEL_OID_TASK_P2P_DISCOVER,
		WDI_CANCEL_OID_TASK_P2P_DISCOVER,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_CONNECT, 
			OID_STR_WRAPPER("OID_WDI_TASK_CONNECT"),
			FALSE,
			ParseWdiTaskConnect,
			CleanupParsedWdiTaskConnect,
			sizeof(WDI_TASK_CONNECT_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_CONNECT_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_CONNECT,
		WDI_CANCEL_OID_TASK_CONNECT,
		WDI_CANCEL_OID_TASK_CONNECT,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_DOT11_RESET, 
			OID_STR_WRAPPER("OID_WDI_TASK_DOT11_RESET"),
			FALSE,
			ParseWdiTaskDot11Reset,
			CleanupParsedWdiTaskDot11Reset,
			sizeof(WDI_TASK_DOT11_RESET_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_DOT11_RESET_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_DOT11_RESET,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_DISCONNECT, 
			OID_STR_WRAPPER("OID_WDI_TASK_DISCONNECT"),
			FALSE,
			ParseWdiTaskDisconnect,
			CleanupParsedWdiTaskDisconnect,
			sizeof(WDI_TASK_DISCONNECT_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_DISCONNECT_COMPLETE,
		FALSE,
		28,
		WDI_OID_TASK_DISCONNECT,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME, 
			OID_STR_WRAPPER("OID_WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME"),
			FALSE,
			ParseWdiTaskP2pSendRequestActionFrame,
			CleanupParsedWdiTaskP2pSendRequestActionFrame,
			sizeof(WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_P2P_SEND_REQUEST_ACTION_FRAME_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_P2P_SEND_REQUEST_ACTION_FRAME,
		WDI_CANCEL_OID_TASK_P2P_SEND_REQUEST_ACTION_FRAME,
		WDI_CANCEL_OID_TASK_P2P_SEND_REQUEST_ACTION_FRAME,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME, 
			OID_STR_WRAPPER("OID_WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME"),
			FALSE,
			ParseWdiTaskP2pSendResponseActionFrame,
			CleanupParsedWdiTaskP2pSendResponseActionFrame,
			sizeof(WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_P2P_SEND_RESPONSE_ACTION_FRAME_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_P2P_SEND_RESPONSE_ACTION_FRAME,
		WDI_CANCEL_OID_TASK_P2P_SEND_RESPONSE_ACTION_FRAME,
		WDI_CANCEL_OID_TASK_P2P_SEND_RESPONSE_ACTION_FRAME,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_SET_RADIO_STATE, 
			OID_STR_WRAPPER("OID_WDI_TASK_SET_RADIO_STATE"),
			TRUE,
			ParseWdiTaskSetRadioState,
			CleanupParsedWdiTaskSetRadioState,
			sizeof(WDI_SET_RADIO_STATE_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_SET_RADIO_STATE_COMPLETE,
		FALSE,
		28,
		WDI_OID_TASK_SET_RADIO_STATE,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_CREATE_PORT, 
			OID_STR_WRAPPER("OID_WDI_TASK_CREATE_PORT"),
			TRUE,
			ParseWdiTaskCreatePort,
			CleanupParsedWdiTaskCreatePort,
			sizeof(WDI_TASK_CREATE_PORT_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_CREATE_PORT_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_CREATE_PORT,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_DELETE_PORT, 
			OID_STR_WRAPPER("OID_WDI_TASK_DELETE_PORT"),
			TRUE,
			ParseWdiTaskDeletePort,
			CleanupParsedWdiTaskDeletePort,
			sizeof(WDI_TASK_DELETE_PORT_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_DELETE_PORT_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_DELETE_PORT,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_START_AP, 
			OID_STR_WRAPPER("OID_WDI_TASK_START_AP"),
			FALSE,
			ParseWdiTaskStartAp,
			CleanupParsedWdiTaskStartAp,
			sizeof(WDI_TASK_START_AP_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_START_AP_COMPLETE,
		FALSE,
		28,
		WDI_OID_TASK_START_AP,
		WDI_CANCEL_OID_TASK_START_AP,
		WDI_CANCEL_OID_TASK_START_AP,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_STOP_AP, 
			OID_STR_WRAPPER("OID_WDI_TASK_STOP_AP"),
			FALSE,
			NULL,
			NULL,
			0
		},
		NDIS_STATUS_WDI_INDICATION_STOP_AP_COMPLETE,
		FALSE,
		28,
		WDI_OID_TASK_STOP_AP,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_SEND_AP_ASSOCIATION_RESPONSE, 
			OID_STR_WRAPPER("OID_WDI_TASK_SEND_AP_ASSOCIATION_RESPONSE"),
			FALSE,
			ParseWdiTaskSendApAssociationResponse,
			CleanupParsedWdiTaskSendApAssociationResponse,
			sizeof(WDI_TASK_SEND_AP_ASSOCIATION_RESPONSE_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_SEND_AP_ASSOCIATION_RESPONSE_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_SEND_AP_ASSOCIATION_RESPONSE,
		WDI_CANCEL_OID_TASK_SEND_AP_ASSOCIATION_RESPONSE,
		WDI_CANCEL_OID_TASK_SEND_AP_ASSOCIATION_RESPONSE,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_CHANGE_OPERATION_MODE, 
			OID_STR_WRAPPER("OID_WDI_TASK_CHANGE_OPERATION_MODE"),
			FALSE,
			ParseWdiTaskChangeOperationMode,
			CleanupParsedWdiTaskChangeOperationMode,
			sizeof(WDI_TASK_CHANGE_OPERATION_MODE_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_CHANGE_OPERATION_MODE_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_CHANGE_OPERATION_MODE,
		NULL,
		NULL,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_ROAM, 
			OID_STR_WRAPPER("OID_WDI_TASK_ROAM"),
			FALSE,
			ParseWdiTaskRoam,
			CleanupParsedWdiTaskRoam,
			sizeof(WDI_TASK_ROAM_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_ROAM_COMPLETE,
		TRUE,
		28,
		WDI_OID_TASK_ROAM,
		NULL,
		NULL,
		NULL,
		NULL
	},
	/*------------------------- HotSpot 2.0 ------------------------*/
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_SEND_REQUEST_ACTION_FRAME, 
			OID_STR_WRAPPER("OID_WDI_TASK_SEND_REQUEST_ACTION_FRAME"),
			FALSE,
			ParseWdiTaskSendRequestActionFrame,
			CleanupParsedWdiTaskSendRequestActionFrame,
			sizeof(WDI_TASK_SEND_REQUEST_ACTION_FRAME_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_SEND_REQUEST_ACTION_FRAME_COMPLETE,
		TRUE,	
		28,
		WDI_OID_TASK_SEND_REQUEST_ACTION_FRAME,
		WDI_CANCEL_OID_TASK_SEND_REQUEST_ACTION_FRAME,
		WDI_CANCEL_OID_TASK_SEND_REQUEST_ACTION_FRAME,
		NULL,
		NULL
	},
	{
		{
			RT_CMD_ENTRY_TYPE_TASK,
			OID_WDI_TASK_SEND_RESPONSE_ACTION_FRAME, 
			OID_STR_WRAPPER("OID_WDI_TASK_SEND_RESPONSE_ACTION_FRAME"),
			FALSE,
			ParseWdiTaskSendResponseActionFrame,
			CleanupParsedWdiTaskSendResponseActionFrame,
			sizeof(WDI_TASK_SEND_RESPONSE_ACTION_FRAME_PARAMETERS)
		},
		NDIS_STATUS_WDI_INDICATION_SEND_RESPONSE_ACTION_FRAME_COMPLETE,
		TRUE,	
		28,
		WDI_OID_TASK_SEND_RESPONSE_ACTION_FRAME,
		WDI_CANCEL_OID_TASK_SEND_RESPONSE_ACTION_FRAME,
		WDI_CANCEL_OID_TASK_SEND_RESPONSE_ACTION_FRAME,
		NULL,
		NULL
	}

};

RT_PROPERTY_ENTRY RT_SUPPORT_PROPERTYs[]=
{
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_P2P_LISTEN_STATE, 
			OID_STR_WRAPPER("OID_WDI_SET_P2P_LISTEN_STATE"),
			FALSE,
			ParseWdiSetP2pListenState,
			CleanupParsedWdiSetP2pListenState,
			sizeof(WDI_SET_P2P_LISTEN_STATE_PARAMETERS)
		},
		WDI_OID_SET_P2P_LISTEN_STATE
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_P2P_ADDITIONAL_IE, 
			OID_STR_WRAPPER("OID_WDI_SET_P2P_ADDITIONAL_IE"),
			FALSE,
			NULL,
			NULL,
			0
		},
		WDI_OID_SET_P2P_ADDITIONAL_IE
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_ADD_CIPHER_KEYS, 
			OID_STR_WRAPPER("OID_WDI_SET_ADD_CIPHER_KEYS"),
			FALSE,
			ParseWdiSetAddCipherKeys,
			CleanupParsedWdiSetAddCipherKeys,
			sizeof(WDI_SET_ADD_CIPHER_KEYS_PARAMETERS)
		},
		WDI_OID_SET_ADD_CIPHER_KEYS
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_DELETE_CIPHER_KEYS, 
			OID_STR_WRAPPER("OID_WDI_SET_DELETE_CIPHER_KEYS"),
			FALSE,
			ParseWdiSetDeleteCipherKeys,
			CleanupParsedWdiSetDeleteCipherKeys,
			sizeof(WDI_SET_DELETE_CIPHER_KEYS_PARAMETERS)
		},
		WDI_OID_SET_DELETE_CIPHER_KEYS
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_DEFAULT_KEY_ID, 
			OID_STR_WRAPPER("OID_WDI_SET_DEFAULT_KEY_ID"),
			FALSE,
			ParseWdiSetDefaultKeyId,
			CleanupParsedWdiSetDefaultKeyId,
			sizeof(WDI_SET_DEFAULT_KEY_ID_PARAMETERS)
		},
		WDI_OID_SET_DEFAULT_KEY_ID
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_CONNECTION_QUALITY, 
			OID_STR_WRAPPER("OID_WDI_SET_CONNECTION_QUALITY"),
			FALSE,
			ParseWdiSetConnectionQuality,
			CleanupParsedWdiSetConnectionQuality,
			sizeof(WDI_SET_CONNECTION_QUALITY_PARAMETERS)
		},
		WDI_OID_SET_CONNECTION_QUALITY
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_GET_AUTO_POWER_SAVE, 
			OID_STR_WRAPPER("OID_WDI_GET_AUTO_POWER_SAVE"),
			FALSE,
			ParseWdiGetAutoPowerSave,
			CleanupParsedWdiGetAutoPowerSave,
			sizeof(WDI_GET_AUTO_POWER_SAVE_PARAMETERS)
		},
		WDI_OID_GET_AUTO_POWER_SAVE
	},					
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_GET_STATISTICS, 
			OID_STR_WRAPPER("OID_WDI_GET_STATISTICS"),
			FALSE,
			NULL,
			NULL,
			0
		},
		WDI_OID_GET_STATISTICS
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_RECEIVE_PACKET_FILTER, 
			OID_STR_WRAPPER("OID_WDI_SET_RECEIVE_PACKET_FILTER"),
			FALSE,
			ParseWdiSetReceivePacketFilter,
			CleanupParsedWdiSetReceivePacketFilter,
			sizeof(WDI_SET_RECEIVE_PACKET_FILTER_PARAMETERS)
		},
		WDI_OID_SET_RECEIVE_PACKET_FILTER
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_GET_ADAPTER_CAPABILITIES, 
			OID_STR_WRAPPER("OID_WDI_GET_ADAPTER_CAPABILITIES"),
			TRUE,
			NULL,
			NULL,
			0
		},
		WDI_OID_GET_ADAPTER_CAPABILITIES
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_NETWORK_LIST_OFFLOAD, 
			OID_STR_WRAPPER("OID_WDI_SET_NETWORK_LIST_OFFLOAD"),
			FALSE,
			ParseWdiSetNetworkListOffload,
			CleanupParsedWdiSetNetworkListOffload,
			sizeof(WDI_NETWORK_LIST_OFFLOAD_PARAMETERS)
		},
		WDI_OID_SET_NETWORK_LIST_OFFLOAD
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_RECEIVE_COALESCING, 
			OID_STR_WRAPPER("OID_WDI_SET_RECEIVE_COALESCING"),
			FALSE,
			ParseWdiSetReceiveCoalescing,
			CleanupParsedWdiSetReceiveCoalescing,
			sizeof(WDI_SET_RECEIVE_COALESCING_PARAMETERS)
		},
		WDI_OID_SET_RECEIVE_COALESCING
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_PRIVACY_EXEMPTION_LIST, 
			OID_STR_WRAPPER("OID_WDI_SET_PRIVACY_EXEMPTION_LIST"),
			FALSE,
			ParseWdiSetPrivacyExemptionList,
			CleanupParsedWdiSetPrivacyExemptionList,
			sizeof(WDI_SET_PRIVACY_EXEMPTION_LIST_PARAMETERS)
		},
		WDI_OID_SET_PRIVACY_EXEMPTION_LIST
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_POWER_STATE, 
			OID_STR_WRAPPER("OID_WDI_SET_POWER_STATE"),
			TRUE,
			ParseWdiSetPowerState,
			CleanupParsedWdiSetPowerState,
			sizeof(WDI_SET_POWER_PARAMETERS)
		},
		WDI_OID_SET_POWER_STATE
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_ABORT_TASK,
			OID_STR_WRAPPER("OID_WDI_ABORT_TASK"),
			FALSE,
			ParseWdiAbortTask,
			CleanupParsedWdiAbortTask,
			sizeof(WDI_TASK_ABORT_PARAMETERS)
		},
		WDI_OID_ABORT_TASK
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_ADD_WOL_PATTERN, 
			OID_STR_WRAPPER("OID_WDI_SET_ADD_WOL_PATTERN"),
			FALSE,
			ParseWdiSetAddWolPattern,
			CleanupParsedWdiSetAddWolPattern,
			sizeof(WDI_SET_ADD_WOL_PATTERN_PARAMETERS)
		},
		WDI_OID_SET_ADD_WOL_PATTERN
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_REMOVE_WOL_PATTERN, 
			OID_STR_WRAPPER("OID_WDI_SET_REMOVE_WOL_PATTERN"),
			FALSE,
			ParseWdiSetRemoveWolPattern,
			CleanupParsedWdiSetRemoveWolPattern,
			sizeof(WDI_SET_REMOVE_WOL_PATTERN_PARAMETERS)
		},
		WDI_OID_SET_REMOVE_WOL_PATTERN
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_MULTICAST_LIST, 
			OID_STR_WRAPPER("OID_WDI_SET_MULTICAST_LIST"),
			FALSE,
			ParseWdiSetMulticastList,
			CleanupParsedWdiSetMulticastList,
			sizeof(WDI_SET_MULTICAST_LIST_PARAMETERS)
		},
		WDI_OID_SET_MULTICAST_LIST
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_ADD_PM_PROTOCOL_OFFLOAD,
			OID_STR_WRAPPER("OID_WDI_SET_ADD_PM_PROTOCOL_OFFLOAD"),
			FALSE,
			ParseWdiSetAddPmProtocolOffload,
			CleanupParsedWdiSetAddPmProtocolOffload,
			sizeof(WDI_SET_ADD_PM_PROTOCOL_OFFLOAD_PARAMETERS_PARAMETERS)
		},
		WDI_OID_WDI_SET_ADD_PM_PROTOCOL_OFFLOAD
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_REMOVE_PM_PROTOCOL_OFFLOAD,
			OID_STR_WRAPPER("OID_WDI_SET_REMOVE_PM_PROTOCOL_OFFLOAD"),
			FALSE,
			ParseWdiSetRemovePmProtocolOffload,
			CleanupParsedWdiSetRemovePmProtocolOffload,
			sizeof(WDI_SET_REMOVE_PM_PROTOCOL_OFFLOAD_PARAMETERS)
		},
		WDI_OID_WDI_SET_REMOVE_PM_PROTOCOL_OFFLOAD
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_ADAPTER_CONFIGURATION, 
			OID_STR_WRAPPER("OID_WDI_SET_ADAPTER_CONFIGURATION"),
			TRUE,
			ParseWdiSetAdapterConfiguration,
			CleanupParsedWdiSetAdapterConfiguration,
			sizeof(WDI_SET_FIRMWARE_CONFIGURATION_PARAMETERS)
		},
		WDI_OID_SET_ADAPTER_CONFIGURATION
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_GET_RECEIVE_COALESCING_MATCH_COUNT, 
			OID_STR_WRAPPER("OID_WDI_GET_RECEIVE_COALESCING_MATCH_COUNT"),
			FALSE,
			ParseWdiGetReceiveCoalescingMatchCount,
			CleanupParsedWdiGetReceiveCoalescingMatchCount,
			sizeof(WDI_GET_RECEIVE_COALESCING_MATCH_COUNT_INPUTS)
		},
		WDI_OID_GET_RECEIVE_COALESCING_MATCH_COUNT
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_ADVERTISEMENT_INFORMATION,
			OID_STR_WRAPPER("OID_WDI_SET_ADVERTISEMENT_INFORMATION"),
			FALSE,
			ParseWdiSetAdvertisementInformation,
			CleanupParsedWdiSetAdvertisementInformation,
			sizeof(WDI_SET_ADVERTISEMENT_INFORMATION_PARAMETERS)
		},
		WDI_OID_SET_ADVERTISEMENT_INFORMATION,
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_IHV_REQUEST, 
			OID_STR_WRAPPER("OID_WDI_IHV_REQUEST"),
			FALSE,
			NULL,
			NULL,
			0
		},
		WDI_OID_IHV_REQUEST
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_GET_NEXT_ACTION_FRAME_DIALOG_TOKEN, 
			OID_STR_WRAPPER("OID_WDI_GET_NEXT_ACTION_FRAME_DIALOG_TOKEN"),
			FALSE,
			ParseWdiGetNextActionFrameDialogToken,
			CleanupParsedWdiGetNextActionFrameDialogToken,
			sizeof(WDI_GET_NEXT_ACTION_FRAME_DIALOG_TOKEN_INPUTS)
		},
		WDI_OID_GET_NEXT_ACTION_FRAME_DIALOG_TOKEN
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_P2P_WPS_ENABLED, 
			OID_STR_WRAPPER("OID_WDI_SET_P2P_WPS_ENABLED"),
			FALSE,
			ParseWdiSetP2pWpsEnabled,
			CleanupParsedWdiSetP2pWpsEnabled,
			sizeof(WDI_SET_P2P_WPS_ENABLED_PARAMETERS)
		},
		WDI_OID_SET_P2P_WPS_ENABLED
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_GET_BSS_ENTRY_LIST, 
			OID_STR_WRAPPER("OID_WDI_GET_BSS_ENTRY_LIST"),
			FALSE,
			ParseWdiGetBssEntryList,
			CleanupParsedWdiGetBssEntryList,
			sizeof(WDI_GET_BSS_ENTRY_LIST_UPDATE_PARAMETERS)
		},
		WDI_OID_GET_BSS_ENTRY_LIST
	},
	{
		{
			RT_CMD_ENTRY_TYPE_PROPERTY,
			OID_WDI_SET_FAST_BSS_TRANSITION_PARAMETERS, 
			OID_STR_WRAPPER("OID_WDI_SET_FAST_BSS_TRANSITION_PARAMETERS"),
			TRUE,
			ParseWdiSetFastBssTransitionParameters,
			CleanupParsedWdiSetFastBssTransitionParameters,
			sizeof(WDI_SET_FAST_BSS_TRANSITION_PARAMETERS_COMMAND)
		},
		WDI_OID_SET_FAST_BSS_TRANSITION_PARAMETERS

	}
};

static
RT_COMMAND_ENTRY *
wdi_GetCommandEntry(
	IN  NDIS_OID			oid
	)
{
	u4Byte					it = 0;
	
	for(it = 0 ; it < sizeof(RT_SUPPORT_TASKs) / sizeof(RT_SUPPORT_TASKs[0]); it++)
	{
		if(oid == RT_SUPPORT_TASKs[it].super.Oid)
			return &RT_SUPPORT_TASKs[it].super;
	}

	for(it = 0 ; it < sizeof(RT_SUPPORT_PROPERTYs) / sizeof(RT_SUPPORT_PROPERTYs[0]); it++)
	{
		if(oid == RT_SUPPORT_PROPERTYs[it].super.Oid)
			return &RT_SUPPORT_PROPERTYs[it].super;
	}

	return NULL;
}

static
RT_OID_HANDLER *
wdi_GetOidHandler(
	IN  WDI_DATA_STRUCT			*wdi,
	IN  RT_COMMAND_ENTRY		*cmd
	)
{
	if(RT_CMD_ENTRY_TYPE_TASK == cmd->Type && !wdi->bCommandHangHappened)
	{
		return &wdi->TaskHandle;
	}
	else if(RT_CMD_ENTRY_TYPE_TASK == cmd->Type && wdi->bCommandHangHappened)
	{
		return &wdi->TaskPostHangHandle;
	}
	else if(RT_CMD_ENTRY_TYPE_PROPERTY == cmd->Type)
	{
		return &wdi->PropertyHandle;
	}
	else
	{
		return NULL;
	}
}

static
NDIS_STATUS
wdi_TlvParser_Init(
	IN  RT_TLV_PARSER			*parser,
	IN  ADAPTER					*pAdapter
	)
{
	u4Byte						it = 0;

	parser->nonCleanedup = 0;
	parser->buf = NULL;
	parser->bufSize = 0;
	parser->parsedTlv.param = NULL;
	
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("wdi_TlvParser_Init(): Param Cleaned.\n"));

	for(it = 0 ; it < sizeof(RT_SUPPORT_TASKs) / sizeof(RT_SUPPORT_TASKs[0]); it++)
	{
		if(parser->bufSize < RT_SUPPORT_TASKs[it].super.parsedTlvSize)
			parser->bufSize = RT_SUPPORT_TASKs[it].super.parsedTlvSize;
	}

	for(it = 0 ; it < sizeof(RT_SUPPORT_PROPERTYs) / sizeof(RT_SUPPORT_PROPERTYs[0]); it++)
	{
		if(parser->bufSize < RT_SUPPORT_PROPERTYs[it].super.parsedTlvSize)
			parser->bufSize = RT_SUPPORT_PROPERTYs[it].super.parsedTlvSize;
	}
	
	PlatformAllocateMemory(pAdapter, &parser->buf, parser->bufSize);

	if(!parser->buf)
		return NDIS_STATUS_RESOURCES;

	return NDIS_STATUS_SUCCESS;
}

static
VOID
wdi_TlvParser_Deinit(
	IN  RT_TLV_PARSER			*parser
	)
{
	// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
	RT_ASSERT(0 == parser->nonCleanedup, 
		("%s(): some parsed tls not cleaned up before deinit: %u\n", 
		__FUNCTION__, parser->nonCleanedup));
	
	if(parser->buf)
	{
		PlatformFreeMemory(parser->buf, parser->bufSize);

		parser->buf = NULL;
		parser->bufSize = 0;
		parser->parsedTlv.param = NULL;
		
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("wdi_TlvParser_Deinit(): Param Cleaned.\n"));
	}
	else
	{// wdi_TlvParser_Init failed

	}
}

static
NDIS_STATUS
wdi_TlvParser_Parse(
	IN	RT_TLV_PARSER			*parser,
	IN  ADAPTER					*pAdapter,
	IN  RT_COMMAND_ENTRY		*cmdEntry,
	IN  NDIS_OID_REQUEST		*pNdisRequest
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	
	if(!cmdEntry->TlvParser)
		return NDIS_STATUS_SUCCESS;

	RT_ASSERT(cmdEntry->TlvCleaner, ("%s(): there's a parser but no cleaner\n", __FUNCTION__));
	RT_ASSERT(cmdEntry->parsedTlvSize, ("%s(): there's a parser but no parsed tlv size\n", __FUNCTION__));
	RT_ASSERT(parser->buf, ("%s(): parser buffer is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(parser->parsedTlv.param == NULL, ("%s(): parsedTlv param shall be NULL\n", __FUNCTION__));
	// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
	RT_ASSERT(0 == parser->nonCleanedup, 
		("%s(): there are parsed tlvs not cleanded up: %u\n", 
		__FUNCTION__, parser->nonCleanedup));

	do
	{
		PlatformZeroMemory(parser->buf, parser->bufSize);
		parser->parsedTlv.param = NULL;		
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("wdi_TlvParser_Parse(): Param Cleaned.\n"));

		if(sizeof(WDI_MESSAGE_HEADER) == pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength)
		{// no tlv at all, don't feed to the parser (will return invalid data)
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("wdi_TlvParser_Parse(): no tlv at all\n"));
			status = NDIS_STATUS_SUCCESS;
			break;
		}
		
		if(NDIS_STATUS_SUCCESS != (status = cmdEntry->TlvParser(
			pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength - sizeof(WDI_MESSAGE_HEADER),
			(pu1Byte)pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer + sizeof(WDI_MESSAGE_HEADER),
			&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
			parser->buf)))
		{
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("wdi_TlvParser_Parse(): cmdEntry->TlvParser return %s\n", status?"NOT SUCCESS":"SUCCESS"));
			cmdEntry->TlvCleaner(parser->buf);
			PlatformZeroMemory(parser->buf, parser->bufSize);
			break;
		}

		parser->parsedTlv.param = parser->buf;
		parser->nonCleanedup++;
		
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("wdi_TlvParser_Parse(): Param Assigned.\n"));
	}while(FALSE);

	return status;
}

static
VOID
wdi_TlvParser_Cleanup(
	IN  RT_TLV_PARSER			*parser,
	IN  RT_COMMAND_ENTRY		*cmdEntry
	)
{

	if(!cmdEntry->TlvCleaner)
		return;
	
	RT_ASSERT(cmdEntry->TlvParser, ("%s(): there's a cleaner but no parser\n", __FUNCTION__));

	if(parser->parsedTlv.param)
	{
		RT_ASSERT(parser->parsedTlv.param, ("%s(): no parsed structure, parsedTlv.param is NULL\n", __FUNCTION__));
		
		// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
		RT_ASSERT(1 == parser->nonCleanedup, 
			("%s(): incorrect # of non cleaned up parsed tlvs: %u\n", 
			__FUNCTION__, parser->nonCleanedup));
		
		cmdEntry->TlvCleaner(parser->parsedTlv.param);
		PlatformZeroMemory(parser->buf, parser->bufSize);
		parser->parsedTlv.param = NULL;
		parser->nonCleanedup--;
		
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("wdi_TlvParser_Cleanup(): Param Cleaned.\n"));
	}
	
	return;
}


BOOLEAN
OidHandle_VerifyTask(
	IN  PADAPTER			pAdapter,
	IN  u4Byte				WdiTaskOid
	)
{
	BOOLEAN				bResult = FALSE;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PRT_COMPLETE_ENTRY	pEntry = NULL;
	RT_LIST_ENTRY		*pTaskIndicationList = NULL;

	if(pWdi->bCommandHangHappened)
	{
		pTaskIndicationList = &pWdi->TaskPostHangCompleteIndicationList;
	}
	else
	{
		pTaskIndicationList = &pWdi->TaskCompleteIndicationList;
	}

	if(RTIsListEmpty(pTaskIndicationList))
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s(): No entry in completion list!!! Verify fail!!!\n", __FUNCTION__));
	}
	else
	{
		pEntry= (PRT_COMPLETE_ENTRY)RTGetHeadList(pTaskIndicationList);
		
		if(pEntry->Oid == WdiTaskOid)
		{
			bResult = TRUE;
		}
		else
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s(): Completion entry record mismatch! ", __FUNCTION__));
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("In completion entry: Oid = %04x\n", pEntry->Oid));
		}
	}

	return bResult;
}


RT_STATUS
OidHandle_IndicateM3(
	IN	PADAPTER			pAdapter,
	IN	PRT_OID_HANDLER	pOidHandle
	)
{
	PNDIS_OID_REQUEST		pNdisRequest = pOidHandle->pNdisRequest;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> OidHandle_IndicateM3 (%x)\n", pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.Oid));

	RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, ("Indicate buffer\n"), pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer, pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten);

	NdisOIDHistoryUpdate(pAdapter, pNdisRequest, RT_OID_HISTORY_M3);

	// Should we need to ndicate the actual status for each property command instead? e.g., pOidHandle->OidStatus	
	NdisMOidRequestComplete(
		pAdapter->pNdisCommon->hNdisAdapter,
		pNdisRequest,
		pOidHandle->OidStatus  //NDIS_STATUS_SUCCESS
		);

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== OidHandle_IndicateM3\n"));
	return RT_STATUS_SUCCESS;
}


RT_STATUS
OidHandle_IndicateM4(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle,
	IN  PRT_TASK_ENTRY			pTaskEntry
	)
{
	NDIS_STATUS_INDICATION		statusIndication = {0};
	PMGNT_INFO					pMgntInfo = &pAdapter->MgntInfo;
	BOOLEAN						bDeletePeer = FALSE;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> OidHandle_IndicateM4 (%x)\n", pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.Oid));

	NdisOIDHistoryUpdate(pAdapter, pOidHandle->pNdisRequest, RT_OID_HISTORY_M4);
	
	N6_ASSIGN_OBJECT_HEADER(
		statusIndication.Header,
		NDIS_OBJECT_TYPE_STATUS_INDICATION,
		NDIS_SIZEOF_STATUS_INDICATION_REVISION_1,
		NDIS_STATUS_INDICATION_REVISION_1);

	statusIndication.RequestId = pOidHandle->pNdisRequest->RequestId;
	statusIndication.SourceHandle = pAdapter;
	statusIndication.StatusBuffer = pOidHandle->pInputBuffer;
	statusIndication.StatusBufferSize = sizeof(WDI_MESSAGE_HEADER) + pOidHandle->OutputBufferLength;

	//3 Fill status code for each task
	statusIndication.StatusCode = pTaskEntry->StatusCode;

	RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, ("statusIndication buffer\n"), &statusIndication, sizeof(statusIndication));
	RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, ("buffer content\n"), statusIndication.StatusBuffer, statusIndication.StatusBufferSize);
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== OidHandle_IndicateM4 returns: 0x%08X\n", ((WDI_MESSAGE_HEADER *)pOidHandle->pInputBuffer)->Status));

	// Send the indication
	NdisMIndicateStatusEx(
		pAdapter->pNdisCommon->hNdisAdapter,
		&statusIndication
		);
	
	return RT_STATUS_SUCCESS;
}


RT_STATUS
OidHandle_Cancel(
	IN  PADAPTER				pAdapter,
	IN  u4Byte				NdisOid,
	IN  u4Byte				TransactionId,
	IN  u2Byte				PortId
	)
{
	//
	// Determine if this OID can be aborted or not
	//	Check:
	//		(1) Validate definition of abort from task table
	//		(2) Validate OID/Transaction ID/Port ID
	//
	
	RT_STATUS	RtStatus = RT_STATUS_INVALID_PARAMETER;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);

	do
	{
		PRT_COMPLETE_ENTRY		pEntry = NULL;
		PRT_TASK_ENTRY			pTask = (PRT_TASK_ENTRY)pWdi->TaskHandle.CmdEntry;

		if(NULL == pTask)
		{
			break;
		}

		NdisAcquireSpinLock(&pWdi->TaskCompleteIndicationLock);
		if(RTIsListEmpty(&(pWdi->TaskCompleteIndicationList)))
		{
			NdisReleaseSpinLock(&pWdi->TaskCompleteIndicationLock);
			RtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		pEntry = (PRT_COMPLETE_ENTRY)RTGetHeadList(&(pWdi->TaskCompleteIndicationList));

		if(pEntry->Oid == NdisOid && pEntry->TransactionId == TransactionId && pEntry->PortId == PortId)
		{
			NdisReleaseSpinLock(&pWdi->TaskCompleteIndicationLock);
			RtStatus = RT_STATUS_SUCCESS;

			if(OID_WDI_TASK_P2P_DISCOVER == pEntry->Oid
				|| OID_WDI_TASK_SCAN == pEntry->Oid
				)
			{
				OidHandle_AbortAction(pAdapter, &pWdi->TaskHandle);
			}
			else
			{
				NdisAcquireSpinLock(&pWdi->TaskHandle.Lock);
				pWdi->TaskHandle.Status |= RT_OID_HANDLER_STATUS_CANCELED;
				NdisReleaseSpinLock(&pWdi->TaskHandle.Lock);
				
				OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
			}
			
			break;
		}

		NdisReleaseSpinLock(&pWdi->TaskCompleteIndicationLock);

	}
	while(FALSE);

	return RtStatus;
}


RT_STATUS
OidHandle_AbortAction(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	RT_STATUS			RtStatus = RT_STATUS_SUCCESS;
	PRT_TASK_ENTRY		pTask = (PRT_TASK_ENTRY)pOidHandle->CmdEntry;
	u2Byte				i = 0;
	PADAPTER			pTargetAdapter = pAdapter;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> OidHandle_AbortAction\n"));


	do
	{
		if(NULL == pTask)
		{
			RtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(!pTask->super.bAdapterObject)
		{
			pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)(((PWDI_MESSAGE_HEADER)pOidHandle->pInputBuffer)->PortId));
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Transport TASK TargetAdapter to Port(%d)\n", ((PWDI_MESSAGE_HEADER)pOidHandle->pInputBuffer)->PortId));
		}

		if(NULL != pTask->AbortFunc)
			pTask->AbortFunc(pTargetAdapter);
		
	}
	while(FALSE);
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== OidHandle_AbortAction\n"));
	return RtStatus;
}


NDIS_STATUS
N6WdiAbortOidRequest(
	IN  NDIS_HANDLE			MiniportAdapterContext,
	IN  PNDIS_OID_REQUEST	pNdisRequest
    )
{
	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
WDICommandCleanup(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle,
	IN	BOOLEAN				bAsync
	)
{
	NDIS_STATUS				status = NDIS_STATUS_SUCCESS;
	
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("==> WDICommandCleanup\n"));
	
	do
	{
		if(pOidHandle->Status & RT_OID_HANDLER_STATUS_RELEASED)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OidHandle is already released\n"));
			status = NDIS_STATUS_INVALID_STATE;
			break;
		}

		if(!(pOidHandle->Status & RT_OID_HANDLER_STATUS_SET))
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OidHandle not set\n"));
			status = NDIS_STATUS_INVALID_STATE;
			break;
		}

		if((!(pOidHandle->Status & RT_OID_HANDLER_STATUS_RETURNED)) && 
			(!bAsync) &&
			!(pAdapter->pPortCommonInfo->WdiData.bCommandHangHappened))
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OidHandle not returned\n"));
			status = NDIS_STATUS_INVALID_STATE;
			break;
		}

		wdi_TlvParser_Cleanup(&pOidHandle->tlvParser, pOidHandle->CmdEntry);

		pOidHandle->Status &= ~RT_OID_HANDLER_STATUS_SET;
		pOidHandle->Status &= ~RT_OID_HANDLER_STATUS_RETURNED;
		pOidHandle->Status &= ~RT_OID_HANDLER_STATUS_CANCELED;
		
	}while(FALSE);

	return status;
}


NDIS_STATUS
OidHandle_Complete(
	IN  PADAPTER			pAdapter,
	IN  OIDHANDLE_TYPE		Type
	)
{
	NDIS_STATUS 		status = NDIS_STATUS_SUCCESS;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PRT_OID_HANDLER		pOidHandle = NULL;
	BOOLEAN				bAsync = FALSE;
	PADAPTER			pDefaultAdapter = NULL;

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("==> OidHandle_Complete enter\n"));

	if(OIDHANDLE_TYPE_TASK == Type)
	{
		RT_TASK_ENTRY 		*entry = NULL;//(RT_TASK_ENTRY *)pWdi->TaskHandle.CmdEntry;
		WDI_PORT_ID			PortNumber = 0;
		UINT32				TransactionId = 0;
		WDI_MESSAGE_HEADER	*wdiHdr = NULL;
		//PADAPTER			pTargetAdapter = GetDefaultAdapter(pAdapter);
		RT_LIST_ENTRY		*pTaskIndicationList = NULL;

		if(pWdi->bCommandHangHappened)
		{
			pOidHandle = &pWdi->TaskPostHangHandle;
			pTaskIndicationList = &pWdi->TaskPostHangCompleteIndicationList;
		}
		else
		{
			pOidHandle = &pWdi->TaskHandle;
			pTaskIndicationList = &pWdi->TaskCompleteIndicationList;
		}
		
		entry = (RT_TASK_ENTRY *)pOidHandle->CmdEntry;
		wdiHdr = (WDI_MESSAGE_HEADER *)pOidHandle->pInputBuffer;
		//wdiHdr = (WDI_MESSAGE_HEADER *)pWdi->TaskHandle.pInputBuffer;
		PortNumber = (u2Byte)wdiHdr->PortId;
		TransactionId = wdiHdr->TransactionId;

		if(TRUE == OidHandle_VerifyTask(pAdapter, pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.Oid))
		{
			RTRemoveHeadList(pTaskIndicationList);
		}
		else
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OidHandle_Complete:OidHandle_VerifyTask() return FALSE\n"));
			return NDIS_STATUS_FAILURE;
		}

		pOidHandle->OidExecutionTime = (PlatformGetCurrentTime() - pOidHandle->OidStartTime);
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("TASK CMD execution time:%dus\n", (ULONG)pOidHandle->OidExecutionTime));

		if(pOidHandle->Status & RT_OID_HANDLER_STATUS_CANCELED)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OidHandle_Complete start to abort action!\n"));
			OidHandle_AbortAction(pAdapter, pOidHandle);
		}

		if(entry->bWaitComplete)
		{
			bAsync = TRUE;
		}
		
		if(entry->PreM4Cb != NULL)
		{
			entry->PreM4Cb(pAdapter, pOidHandle);
		}

		RT_TRACE(COMP_OID_SET, DBG_TRACE, 
			("OidHandle_Complete start to indicate M4. Oid:0x%x Port:%d TransactionId:0x%x\n", 
			pWdi->TaskHandle.pNdisRequest->DATA.METHOD_INFORMATION.Oid,
			PortNumber,
			TransactionId));

		pDefaultAdapter = GetDefaultAdapter(pAdapter);

		// TODO: there maybe race condition that when M4 indicated?? new task command may come before WDICommandCleanup
		OidHandle_IndicateM4(pDefaultAdapter, pOidHandle, entry);

		if(entry->PostM4Cb != NULL)
		{
			entry->PostM4Cb(pAdapter, &PortNumber);
		}
		
	}
	else if(OIDHANDLE_TYPE_PROPERTY == Type)
	{
		pOidHandle = &pWdi->PropertyHandle;
		
		pOidHandle->OidExecutionTime = (PlatformGetCurrentTime() - pOidHandle->OidStartTime);
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Property CMD execution time:%dus\n", (ULONG)pOidHandle->OidExecutionTime));

		if(pOidHandle->OidStatus == NDIS_STATUS_PENDING)
		{
			pOidHandle->OidStatus = NDIS_STATUS_SUCCESS;
		}

		pDefaultAdapter = GetDefaultAdapter(pAdapter);
		
		OidHandle_IndicateM3(pDefaultAdapter, &pWdi->PropertyHandle);
	}
	else
	{
		RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("OidHandle_Complete Invalid Type! Should Not be here!\n"));
		return NDIS_STATUS_INVALID_PARAMETER;
	}

	status = WDICommandCleanup(pAdapter, pOidHandle, bAsync);

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("==> OidHandle_Complete status:0x%x\n", status));
	
	return status;
}

NDIS_STATUS
WDICommandHangComplete(
	IN  PADAPTER			pAdapter
	)
{
	NDIS_STATUS 		status = NDIS_STATUS_SUCCESS;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PRT_OID_HANDLER		pOidHandle = NULL;
	RT_TASK_ENTRY 		*entry = (RT_TASK_ENTRY *)pWdi->TaskHandle.CmdEntry;
	WDI_PORT_ID			PortNumber = 0;
	UINT32				TransactionId = 0;
	WDI_MESSAGE_HEADER	*wdiHdr = NULL;
	//PADAPTER			pTargetAdapter = GetDefaultAdapter(pAdapter);
	BOOLEAN				bAsync = FALSE;
	BOOLEAN				bResult = FALSE;
	PRT_COMPLETE_ENTRY	pEntry = NULL;
	PADAPTER			pDefaultAdapter = NULL;
			
	wdiHdr = (WDI_MESSAGE_HEADER *)pWdi->TaskHandle.pInputBuffer;
	PortNumber = (u2Byte)wdiHdr->PortId;
	TransactionId = wdiHdr->TransactionId;

	pOidHandle = &pWdi->TaskHandle;

	if(RTIsListEmpty(&pWdi->TaskCompleteIndicationList))
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s(): No entry in completion list!!! Verify fail!!!\n", __FUNCTION__));
	}
	else
	{
		pEntry= (PRT_COMPLETE_ENTRY)RTGetHeadList(&pWdi->TaskCompleteIndicationList);
		
		if(pEntry->Oid == pWdi->TaskHandle.pNdisRequest->DATA.METHOD_INFORMATION.Oid)
		{
			bResult = TRUE;
		}
		else
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s(): Completion entry record mismatch! ", __FUNCTION__));
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("In completion entry: Oid = %04x\n", pEntry->Oid));
		}
	}

	if(TRUE == bResult)
	{
		DbgPrintEx(0, 0, "Remove hang command from list!\n");
		RTRemoveHeadList(&pWdi->TaskCompleteIndicationList);
	}
	else
	{
		DbgPrintEx(0, 0, "Failed to Remove hang command from list!\n");
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("WDICommandHangComplete:OidHandle_VerifyTask() return FALSE\n"));
		return NDIS_STATUS_FAILURE;
	}

	pOidHandle->OidExecutionTime = (PlatformGetCurrentTime() - pOidHandle->OidStartTime);
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("TASK Hang CMD execution time:%dus\n", (ULONG)pOidHandle->OidExecutionTime));

	if(pOidHandle->Status & RT_OID_HANDLER_STATUS_CANCELED)
	{
		//RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OidHandle_Complete start to abort action!\n"));
		//OidHandle_AbortAction(pAdapter, pOidHandle);
	}

	if(entry->bWaitComplete)
	{
		bAsync = TRUE;
	}
		
	if(entry->PreM4Cb != NULL)
	{
		entry->PreM4Cb(pAdapter, &pWdi->TaskHandle);
	}

	RT_TRACE(COMP_OID_SET, DBG_TRACE, 
		("OidHandle_Complete start to indicate M4. Oid:0x%x Port:%d TransactionId:0x%x\n", 
		pWdi->TaskHandle.pNdisRequest->DATA.METHOD_INFORMATION.Oid,
		PortNumber,
		TransactionId));

	pDefaultAdapter = GetDefaultAdapter(pAdapter);

	DbgPrintEx(0, 0, "complete Hang command M4!\n");
	// TODO: there maybe race condition that when M4 indicated?? new task command may come before WDICommandCleanup
	OidHandle_IndicateM4(pDefaultAdapter, &pWdi->TaskHandle, entry);

	if(entry->PostM4Cb != NULL)
	{
		entry->PostM4Cb(pAdapter, &PortNumber);
	}

	status = WDICommandCleanup(pAdapter, pOidHandle, bAsync);

	return status;
}

NDIS_STATUS
WDICommandHangCleanup(
	IN  PADAPTER			pAdapter
	)
{
	NDIS_STATUS				status = NDIS_STATUS_SUCCESS;
	PWDI_DATA_STRUCT		pWdi = &(pAdapter->pPortCommonInfo->WdiData);

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("==> WDICommandCancelForPLDR\n"));

	if(pWdi->TaskHandle.Status & RT_OID_HANDLER_STATUS_SET)
	{
		PRT_COMPLETE_ENTRY	pEntry = NULL;

		if(RTIsListEmpty(&(pWdi->TaskCompleteIndicationList)))
		{
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("WDICommandCancelForPLDR: No Task Command pending\n"));
			return RT_STATUS_INVALID_STATE;
		}

		pEntry = (PRT_COMPLETE_ENTRY)RTGetHeadList(&(pWdi->TaskCompleteIndicationList));

		//if(pEntry->Oid == NdisOid && pEntry->TransactionId == TransactionId && pEntry->PortId == PortId)
		{
			//If real FW hang happen, LE should just complete the command without touch FW.
			if(OID_WDI_TASK_P2P_DISCOVER == pEntry->Oid || 
				OID_WDI_TASK_SCAN == pEntry->Oid
				)
			{
				//simply complete without abort?
				//OidHandle_AbortAction(pAdapter, &pWdi->TaskHandle);
				//OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK_POST_HANG);
				WDICommandHangComplete(pAdapter);
			}
			else
			{
				pWdi->TaskHandle.Status |= RT_OID_HANDLER_STATUS_CANCELED;
				
				//OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK_POST_HANG);
				WDICommandHangComplete(pAdapter);
			}
		}
	}
	else if(pWdi->PropertyHandle.Status & RT_OID_HANDLER_STATUS_SET)
	{
		//Do nothing here since property cmd will return quickly without touch with FW(need double check) and it should not hang...
		//pWdi->PropertyHandle.Status |= RT_OID_HANDLER_STATUS_CANCELED;
		//OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_PROPERTY);
		
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("WDICommandCancelForPLDR: Should Not happen for Property CMD\n"));
	}

	return status;
}

VOID
WDITaskCommandWorkItemCallback(
	IN PVOID			pContext
	)
{
	NDIS_STATUS 		status = NDIS_STATUS_SUCCESS;
	PADAPTER			pAdapter = (PADAPTER)pContext;
	PADAPTER			pTargetAdapter = pAdapter;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PRT_OID_HANDLER		pOidHandle = &(pWdi->TaskHandle);
	PWDI_MESSAGE_HEADER	pWdiHeader = NULL;
	RT_COMMAND_ENTRY	*entry = NULL;

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("==> WDITaskCommandWorkItemCallback\n"));

	if(pWdi->bCommandHangHappened)
	{
		pOidHandle = &(pWdi->TaskPostHangHandle);
	}

	pWdiHeader = (PWDI_MESSAGE_HEADER)pOidHandle->pInputBuffer;
	entry = pOidHandle->CmdEntry;

	do
	{
		PRT_COMPLETE_ENTRY	pEntry = NULL;
		RT_LIST_ENTRY		*pTaskIndicationList = NULL;

		pOidHandle->pNdisRequest = &(pOidHandle->PendedRequest);

		if(pWdi->bCommandHangHappened)
		{
			pEntry = &pWdi->CurrentTaskPostHang;
			pTaskIndicationList = &pWdi->TaskPostHangCompleteIndicationList;
		}
		else
		{
			pEntry = &pWdi->CurrentTask;
			pTaskIndicationList = &pWdi->TaskCompleteIndicationList;
		}

		if(!RTIsListEmpty(pTaskIndicationList))
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("Task completion list is not empty!!! Check previous task is completed or not!!!\n"));
		}

		//pEntry = &pWdi->CurrentTask;
		pEntry->Oid = pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.Oid;
		pEntry->TransactionId = ((PWDI_MESSAGE_HEADER)(pOidHandle->pInputBuffer))->TransactionId;
		pEntry->PortId = ((PWDI_MESSAGE_HEADER)(pOidHandle->pInputBuffer))->PortId;

		RTInsertTailList(pTaskIndicationList, &pEntry->List);
	}
	while(FALSE);

	if(FALSE == entry->bAdapterObject)
	{
		pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)pWdiHeader->PortId);
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Async:Transport TASK TargetAdapter to Port(%d)\n", pWdiHeader->PortId));
	}
	else
	{
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Async:Task target is adapter\n"));
	}

	// TODO: LE should not touch FW anymore for the commands coming post hangdiagnose
	//if(!pAdapter->pPortCommonInfo->WdiData.bCommandHangHappened)
	{
		status = ((PRT_TASK_ENTRY)(pOidHandle->CmdEntry))->Func(pTargetAdapter, pOidHandle);

		pOidHandle->Status |= RT_OID_HANDLER_STATUS_RETURNED;

		if(NDIS_STATUS_SUCCESS != status)
		{
			pWdiHeader->Status = status;
		}
	}
#if 0
	else
	{
		//for the command coming post hangdiagnose, LE should process them without touch FW.
		pWdiHeader->Status = NDIS_STATUS_SUCCESS;
		OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
	}
#endif
}

VOID
WDIPropertyCommandWorkItemCallback(
	IN PVOID			pContext
	)
{
	NDIS_STATUS 		status = NDIS_STATUS_SUCCESS;
	PADAPTER			pAdapter = (PADAPTER)pContext;
	PADAPTER			pTargetAdapter = pAdapter;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PRT_OID_HANDLER		pOidHandle = &(pWdi->PropertyHandle);
	PWDI_MESSAGE_HEADER	pWdiHeader = NULL;
	RT_COMMAND_ENTRY	*entry = NULL;

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("==> WDIPropertyCommandWorkItemCallback\n"));

	pWdiHeader = (PWDI_MESSAGE_HEADER)pOidHandle->pInputBuffer;
	entry = pOidHandle->CmdEntry;

	if(FALSE == entry->bAdapterObject)
	{
		pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)pWdiHeader->PortId);
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Async:Transport Property TargetAdapter to Port(%d)\n", pWdiHeader->PortId));
	}
	else
	{
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Async:Property target is adapter\n"));
	}

	{
		status = ((RT_PROPERTY_ENTRY *)(pOidHandle->CmdEntry))->Func(pTargetAdapter, pOidHandle);

		pOidHandle->OidStatus = status;
		pOidHandle->Status |= RT_OID_HANDLER_STATUS_RETURNED;

		if(NDIS_STATUS_SUCCESS != status)
		{
			pWdiHeader->Status = status;
		}

		if(0 == pOidHandle->OidReturnTime)
		{
			pOidHandle->OidReturnTime = (PlatformGetCurrentTime() - pOidHandle->OidStartTime);
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Workitem Property return CMD time = %dus\n", (ULONG)pOidHandle->OidReturnTime));
		}

		OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_PROPERTY);
	}
}


NDIS_STATUS
TaskCommandHandle(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS 		status = NDIS_STATUS_SUCCESS;
	PADAPTER			pTargetAdapter = pAdapter;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	//PRT_OID_HANDLER		pOidHandle = &(pWdi->TaskHandle);
	PWDI_MESSAGE_HEADER	pWdiHeader = (PWDI_MESSAGE_HEADER)pOidHandle->pInputBuffer;
	RT_TASK_ENTRY		*entry = (RT_TASK_ENTRY *)pOidHandle->CmdEntry;

	do
	{
		PRT_COMPLETE_ENTRY	pEntry = NULL;
		RT_LIST_ENTRY		*pTaskIndicationList = NULL;

		pOidHandle->pNdisRequest = &(pOidHandle->PendedRequest);

		if(pWdi->bCommandHangHappened)
		{
			pEntry = &pWdi->CurrentTaskPostHang;
			pTaskIndicationList = &pWdi->TaskPostHangCompleteIndicationList;
		}
		else
		{
			pEntry = &pWdi->CurrentTask;
			pTaskIndicationList = &pWdi->TaskCompleteIndicationList;
		}

		if(!RTIsListEmpty(pTaskIndicationList))
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("Task completion list is not empty!!! Check previous task is completed or not!!!\n"));
			return NDIS_STATUS_FAILURE;
		}

		//pEntry = &pWdi->CurrentTask;
		pEntry->Oid = pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.Oid;
		pEntry->TransactionId = ((PWDI_MESSAGE_HEADER)(pOidHandle->pInputBuffer))->TransactionId;
		pEntry->PortId = ((PWDI_MESSAGE_HEADER)(pOidHandle->pInputBuffer))->PortId;

		RTInsertTailList(pTaskIndicationList, &pEntry->List);
	}
	while(FALSE);

	if(FALSE == entry->super.bAdapterObject)
	{
		pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)pWdiHeader->PortId);
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Sync:Transport TASK TargetAdapter to Port(%d)\n", pWdiHeader->PortId));
	}
	else
	{
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Sync:Task target is adapter\n"));
	}
	
	status = entry->Func(pTargetAdapter, pOidHandle);

	pOidHandle->Status |= RT_OID_HANDLER_STATUS_RETURNED;

	if(NDIS_STATUS_SUCCESS != status)
	{
		pWdiHeader->Status = status;
	}

	if(0 == pOidHandle->OidReturnTime)
	{
		pOidHandle->OidReturnTime = (PlatformGetCurrentTime() - pOidHandle->OidStartTime);
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Task return CMD time = %dus\n", (ULONG)pOidHandle->OidReturnTime));
	}

	// TODO: double check
	if(FALSE == entry->bWaitComplete || NDIS_STATUS_SUCCESS != status)
	{
		OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
	}

	return status;
}

NDIS_STATUS
PropertyCommandHandle(
	IN  PADAPTER	pAdapter
	)
{
	NDIS_STATUS 		status = NDIS_STATUS_PENDING;
	PADAPTER			pTargetAdapter = pAdapter;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PRT_OID_HANDLER		pOidHandle = &(pWdi->PropertyHandle);
	PWDI_MESSAGE_HEADER	pWdiHeader = (PWDI_MESSAGE_HEADER)pOidHandle->pInputBuffer;
	RT_PROPERTY_ENTRY	*entry = (RT_PROPERTY_ENTRY *)pOidHandle->CmdEntry;

	if(FALSE == entry->super.bAdapterObject)
	{
		pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)pWdiHeader->PortId);
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Transport PROPERTY TargetAdapter to Port(%d)\n", pWdiHeader->PortId));
	}
	else
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Property target is adapter\n"));
	}
	
	pOidHandle->OidStatus = entry->Func(pTargetAdapter, pOidHandle);

	pOidHandle->Status |= RT_OID_HANDLER_STATUS_RETURNED;

	if(0 == pOidHandle->OidReturnTime)
	{
		pOidHandle->OidReturnTime = (PlatformGetCurrentTime() - pOidHandle->OidStartTime);
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("Property return CMD time = %dus\n", (ULONG)pOidHandle->OidReturnTime));
	}

	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_PROPERTY);

	return status;
}

NDIS_STATUS
WDICommandHandleInitInner(
	IN  PADAPTER			pAdapter,
	IN	PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS 		status = NDIS_STATUS_SUCCESS;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	
	if(NDIS_STATUS_SUCCESS != wdi_TlvParser_Init(&pOidHandle->tlvParser, pAdapter))
	{
		return NDIS_STATUS_FAILURE;
	}
	
	status = PlatformAllocateMemory(
					pAdapter, 
					(PVOID*)&pOidHandle->pInputBuffer, 
					MAX_OID_INPUT_BUFFER_LEN
					);
	if(NDIS_STATUS_SUCCESS != status)
	{
		RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("WDICommandHandleInit Task failed!\n"));
		return status;
	}
	else
	{
		PlatformZeroMemory(pOidHandle->pInputBuffer, MAX_OID_INPUT_BUFFER_LEN);
		pOidHandle->AllocatedBufferLength = MAX_OID_INPUT_BUFFER_LEN;

		// TODO: remove spinlock later if nobody use it
		NdisAllocateSpinLock(&(pOidHandle->Lock));
		//NdisInitializeEvent(&(pOidHandle->CompleteEvent));
		//NdisInitializeEvent(&(pOidHandle->PendingEvent));
		pOidHandle->Status = RT_OID_HANDLER_STATUS_INITIALIZED;
	}

	if(pOidHandle == &pWdi->TaskHandle || pOidHandle == &pWdi->TaskPostHangHandle)
	{
		status = PlatformInitializeWorkItem(pAdapter, 
					&(pOidHandle->WDICommandWorkitem),
					( RT_WORKITEM_CALL_BACK) WDITaskCommandWorkItemCallback,
					(PVOID) pAdapter, 
					"WDITaskCommandWorkitem"
					);
	}

	//Also prepare a workitem for property command if needed
	if(pOidHandle == &pWdi->PropertyHandle)
	{
		status = PlatformInitializeWorkItem(pAdapter, 
					&(pOidHandle->WDICommandWorkitem),
					( RT_WORKITEM_CALL_BACK) WDIPropertyCommandWorkItemCallback,
					(PVOID) pAdapter, 
					"WDIPropertyCommandWorkitem"
					);
	}

	return status;
}

NDIS_STATUS
WDICommandHandleInit(
	IN  PADAPTER pAdapter
	)
{
	NDIS_STATUS			status = RT_STATUS_SUCCESS;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PRT_OID_HANDLER		pOidHandle = NULL;

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("==> WDICommandHandleInit\n"));

	pOidHandle = &(pWdi->PropertyHandle);
	status = WDICommandHandleInitInner(pAdapter, pOidHandle);

	pOidHandle = &(pWdi->TaskHandle);
	status = WDICommandHandleInitInner(pAdapter, pOidHandle);

	//For PLDR: when there are task commands coming post hangdiagnose.
	pOidHandle = &(pWdi->TaskPostHangHandle);
	status = WDICommandHandleInitInner(pAdapter, pOidHandle);

	// TODO: remove spinlock later when nobody use it, currently only abort command will use it.
	NdisAllocateSpinLock(&pAdapter->pPortCommonInfo->WdiData.TaskCompleteIndicationLock);
	RTInitializeListHead(&pAdapter->pPortCommonInfo->WdiData.TaskCompleteIndicationList);
	RTInitializeListHead(&pAdapter->pPortCommonInfo->WdiData.TaskPostHangCompleteIndicationList);

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("==> WDICommandHandleInit status:0x%x\n", status));

	return status;
}


NDIS_STATUS
WDICommandHandleDeinitInner(
	IN  PADAPTER			pAdapter,
	IN	PRT_OID_HANDLER		pOidHandle
	)
{
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	NDIS_STATUS 		status = NDIS_STATUS_SUCCESS;

	if(NULL == pOidHandle)
	{
		RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("WDICommandHandleDeinitEx return. Handle is NULL!\n"));
		return NDIS_STATUS_INVALID_PARAMETER;
	}

	pOidHandle->Status |= RT_OID_HANDLER_STATUS_RELEASED;
	NdisFreeSpinLock(&(pOidHandle->Lock));

	//if(pOidHandle == &pWdi->TaskHandle || pOidHandle == &pWdi->TaskPostHangHandle)
	{
		PlatformFreeWorkItem(&pOidHandle->WDICommandWorkitem);
	}

	if(pOidHandle->pInputBuffer)
	{
		PlatformFreeMemory(pOidHandle->pInputBuffer, pOidHandle->AllocatedBufferLength);
		pOidHandle->pInputBuffer = NULL;
	}

	wdi_TlvParser_Deinit(&pOidHandle->tlvParser);

	return status;
}

NDIS_STATUS
WDICommandHandleDeinit(
	IN  PADAPTER pAdapter
	)
{
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PRT_OID_HANDLER		pOidHandle = NULL;
	NDIS_STATUS 		status = NDIS_STATUS_SUCCESS;

	pOidHandle = &(pWdi->PropertyHandle);
	status = WDICommandHandleDeinitInner(pAdapter, pOidHandle);

	pOidHandle = &(pWdi->TaskHandle);
	status = WDICommandHandleDeinitInner(pAdapter, pOidHandle);

	pOidHandle = &(pWdi->TaskPostHangHandle);
	status = WDICommandHandleDeinitInner(pAdapter, pOidHandle);	

	// TODO: remove spinlock later when nobody use it, currently only abort command will use it.
	NdisFreeSpinLock(&pAdapter->pPortCommonInfo->WdiData.TaskCompleteIndicationLock);

	return status;
}

NDIS_STATUS
PreSetupCommand(
	IN  PADAPTER			pAdapter,
	IN  PNDIS_OID_REQUEST	pNdisRequest,
	IN  PRT_OID_HANDLER		pOidHandle,
	IN  RT_COMMAND_ENTRY	*pCmdEntry
	)
{
	NDIS_STATUS status = NDIS_STATUS_SUCCESS;

	if((!(pOidHandle->Status & RT_OID_HANDLER_STATUS_INITIALIZED)) ||
		(pOidHandle->Status & RT_OID_HANDLER_STATUS_SET))
	{
		RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("PreSetupCommand: OidHandle invalid state! 0x%x\n", pOidHandle->Status));
		return NDIS_STATUS_INVALID_STATE;
	}
	
	pOidHandle->pNdisRequest = pNdisRequest;
	pOidHandle->OutputBufferLength = 0;
	pOidHandle->OidExecutionTime = 0;
	pOidHandle->OidReturnTime = 0;
	pOidHandle->OidStartTime = 0;
	pOidHandle->CmdEntry = pCmdEntry;

	PlatformMoveMemory(&pOidHandle->PendedRequest, pNdisRequest, sizeof(NDIS_OID_REQUEST));

	if(pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength <= pOidHandle->AllocatedBufferLength)
	{
		PlatformMoveMemory(
			pOidHandle->pInputBuffer, 
			pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer,
			pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength);
	}
	else
	{
		PlatformFreeMemory(pOidHandle->pInputBuffer, pOidHandle->AllocatedBufferLength);
		if(RT_STATUS_SUCCESS == (status = PlatformAllocateMemory(pAdapter, (PVOID*)&pOidHandle->pInputBuffer, pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength)))
		{
			pOidHandle->AllocatedBufferLength = pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
			PlatformMoveMemory(
				pOidHandle->pInputBuffer, 
				pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer,
				pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength);
		}
		else
		{
			RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("PreSetupCommand allocate buffer failed!\n"));
			return NDIS_STATUS_RESOURCES;
		}
	}

	if(NDIS_STATUS_SUCCESS == (status = wdi_TlvParser_Parse(&pOidHandle->tlvParser, pAdapter, pCmdEntry, pNdisRequest)))
	{
		pOidHandle->OidStartTime = PlatformGetCurrentTime();
		pOidHandle->Status |= RT_OID_HANDLER_STATUS_SET;
	}
	else
	{
		RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("PreSetupCommand TLV Parser failure!\n"));
	}
	
	return status;
}

BOOLEAN
TimeConsumingPropertyCommand(
	IN PRT_OID_HANDLER		pOidHandle
	)
{
	if(pOidHandle->CmdEntry->Oid == OID_WDI_SET_POWER_STATE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

NDIS_STATUS
WDICommandHandle(
	IN	PADAPTER				pAdapter,
	IN	RT_COMMAND_ENTRY_TYPE	Type
	)
{
	NDIS_STATUS 		ndisStatus = NDIS_STATUS_SUCCESS;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);

	if(Type == RT_CMD_ENTRY_TYPE_PROPERTY)
	{
		PRT_OID_HANDLER		pOidHandle = &pWdi->PropertyHandle;

		//Note: property command suppose to be executed quickly, but there may be exceptions for some NIC solution.
		//Use workitem to handle the time consuming property command, such as set power that may takes about several seconds on 8723BS.
		if(TimeConsumingPropertyCommand(pOidHandle))
		{
			PlatformScheduleWorkItem(&(pOidHandle->WDICommandWorkitem));
			ndisStatus = NDIS_STATUS_PENDING;
		}
		else
		{
		ndisStatus = PropertyCommandHandle(pAdapter);
	}
	}
	else if(Type == RT_CMD_ENTRY_TYPE_TASK)
	{
		PRT_OID_HANDLER		pOidHandle = &pWdi->TaskHandle;
		RT_TASK_ENTRY 		*taskentry = (RT_TASK_ENTRY *)(pOidHandle->CmdEntry);

		if(pWdi->bCommandHangHappened)
		{
			pOidHandle = &pWdi->TaskPostHangHandle;
		}
		
		if(taskentry->bWaitComplete)
		{
			PlatformScheduleWorkItem(&(pOidHandle->WDICommandWorkitem));
		}
		else
		{
			ndisStatus = TaskCommandHandle(pAdapter, pOidHandle);
		}
	}
	else
	{
		RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("WDICommandHandle CommandType Invalid. Should Not be here!\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
	}

	return ndisStatus;
}

//
// Commands from UE are serialized and LE can reduce the effort of synchronization.
// The command synchronization scope is adapter level scope.
// Basic rule is task commands are serialized between M1 and M4, while property serialized between M1 and M3.
//
// Note: 
// 1. A subset of tasks can be aborted after they have been started.
// 2. Data path is not serialized with the command path, expect for some specific cases.
//
// To support PLDR, here use one more slot for command handler.
//
// Please refer to WDI Spec for more details.
//
NDIS_STATUS
N6WdiHandleOidRequest(
	IN  NDIS_HANDLE			MiniportAdapterContext,
	IN  PNDIS_OID_REQUEST	pNdisRequest
    )
{
	NDIS_STATUS			ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
	PADAPTER			pAdapter = (PADAPTER) MiniportAdapterContext;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	RT_COMMAND_ENTRY	*entry = NULL;
	RT_OID_HANDLER		*oidHandle = NULL;
	PWDI_MESSAGE_HEADER pWdiHeader = (PWDI_MESSAGE_HEADER)pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("N6WdiHandleOidRequestEx Oid:0x%x\n", pNdisRequest->DATA.METHOD_INFORMATION.Oid));

#if SOFTWARE_TRACE_LOGGING
	TraceLoggingWrite(
		g_hProvider,
		"WDI Sample HandleOidRequest",
		TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
		TraceLoggingHexUInt32(pNdisRequest->DATA.METHOD_INFORMATION.Oid, "Oid:"));
#endif

	pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);
	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	
	do
	{
		if(NdisRequestMethod != pNdisRequest->RequestType)
		{
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("N6WdiHandleOidRequestEx Not Recgnized Type:0x%x\n", (ULONG)pNdisRequest->RequestType));
			ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
			break;
		}
		
		if(NULL == (entry = wdi_GetCommandEntry(pNdisRequest->DATA.METHOD_INFORMATION.Oid)))
		{
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("N6WdiHandleOidRequestEx Not Recognized Oid:0x%x\n", pNdisRequest->DATA.METHOD_INFORMATION.Oid));
			ndisStatus = NDIS_STATUS_NOT_RECOGNIZED;
			break;
		}

		if(NULL == (oidHandle = wdi_GetOidHandler(pWdi, entry)))
		{	
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("N6WdiHandleOidRequestEx Failed to get valid OidHandle\n"));
			ndisStatus = NDIS_STATUS_INVALID_DATA;
			break;
		}

		if(NDIS_STATUS_SUCCESS == (ndisStatus = PreSetupCommand(pAdapter, pNdisRequest, oidHandle, entry)))
		{
			ndisStatus = WDICommandHandle(pAdapter, entry->Type);
		}

		if (ndisStatus != NDIS_STATUS_PENDING)
		{
			pWdiHeader->Status = ndisStatus;
		}
	} while(0);

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("N6WdiHandleOidRequestEx Oid:0x%x Status:0x%x\n", pNdisRequest->DATA.METHOD_INFORMATION.Oid, ndisStatus));

	return ndisStatus;
}

