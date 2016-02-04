#ifndef __INC_WDI_DEF_H
#define __INC_WDI_DEF_H


#define	RT_CMD_ENTRY_STRING_SIZE			80

typedef enum __RT_COMMAND_ENTRY_TYPE
{
	RT_CMD_ENTRY_TYPE_NONE,
	RT_CMD_ENTRY_TYPE_TASK,
	RT_CMD_ENTRY_TYPE_PROPERTY,
}RT_COMMAND_ENTRY_TYPE;


typedef struct __RT_COMMAND_ENTRY
{
	RT_COMMAND_ENTRY_TYPE		Type;
	u4Byte 						Oid;
	char						szID[RT_CMD_ENTRY_STRING_SIZE];		// The string name of this command
	BOOLEAN						bAdapterObject;						// Refer WDI spec TASK COMMANDS and PROPTERTY COMMANDS

	// func ptr to the tlv parser, set to NULL if not supported
	NDIS_STATUS (__stdcall *TlvParser)(
		_In_ ULONG BufferLength,
		_In_reads_bytes_( BufferLength ) UINT8 const * pBuffer,
		_In_ PCTLV_CONTEXT Context,
		_Out_opt_ VOID *pParsedMessage
		);

 	// func ptr to the tlv cleaner, shall be provided if TlvParser is specified
	void (__stdcall *TlvCleaner)(
		_In_ VOID *pParsedMessage
		);

	// 'sizeof' of the parsed tlv structure, shall be provided if TlvParser is specified
	size_t						parsedTlvSize;
}RT_COMMAND_ENTRY;

typedef union __RT_PARSED_TLV
{
	// general reference
	VOID *param;

	// task
	WDI_TASK_OPEN_PARAMETERS *paramOpen;
	WDI_TASK_CLOSE_PARAMETERS *paramClose;
	WDI_SCAN_PARAMETERS *paramScan;
	WDI_TASK_P2P_DISCOVER_PARAMETERS *paramP2pDiscover;
	WDI_TASK_CONNECT_PARAMETERS *paramConnect;
	WDI_TASK_DOT11_RESET_PARAMETERS *paramDot11Reset;
	WDI_TASK_DISCONNECT_PARAMETERS *paramDisconnect;
	WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *paramP2pSendRequestActionFrame;
	WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *paramP2pSendResponseActionFrame;
	WDI_SET_RADIO_STATE_PARAMETERS *paramSetRadioState;
	WDI_TASK_CREATE_PORT_PARAMETERS *paramCreatePort;
	WDI_TASK_DELETE_PORT_PARAMETERS *paramDeletePort;
	WDI_TASK_START_AP_PARAMETERS *paramStartAp;
	WDI_TASK_STOP_AP_PARAMETERS *paramStopAp;
	WDI_TASK_SEND_AP_ASSOCIATION_RESPONSE_PARAMETERS *paramSendApAssociationResponse;
	WDI_TASK_CHANGE_OPERATION_MODE_PARAMETERS *paramChangeOperationMode;
	WDI_TASK_ROAM_PARAMETERS *paramRoam;
	WDI_TASK_SEND_REQUEST_ACTION_FRAME_PARAMETERS *paramSendRequestActionFrame;
	WDI_TASK_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *paramSendResponseActionFrame;

	// property
	WDI_SET_P2P_LISTEN_STATE_PARAMETERS *paramSetP2pListenState;
	// OID_WDI_SET_P2P_ADDITIONAL_IE, does not have corresp. structure
	WDI_SET_ADD_CIPHER_KEYS_PARAMETERS *paramSetAddCipherKeys;
	WDI_SET_DELETE_CIPHER_KEYS_PARAMETERS *paramSetDeleteDefaultCipherKeys;
	WDI_SET_DEFAULT_KEY_ID_PARAMETERS *paramSetDefaultKeyId;
	WDI_SET_CONNECTION_QUALITY_PARAMETERS *paramSetConnectionQuality;
	WDI_GET_AUTO_POWER_SAVE_PARAMETERS *paramGetAutoPowerSave;
	WDI_GET_STATISTICS_PARAMETERS *paramGetStatistics;
	WDI_SET_RECEIVE_PACKET_FILTER_PARAMETERS *paramSetReceivePacketFilter;
	WDI_GET_ADAPTER_CAPABILITIES_PARAMETERS *paramGetAdapterCapabilities;
	WDI_NETWORK_LIST_OFFLOAD_PARAMETERS *paramNetworkListOffload;
	WDI_SET_RECEIVE_COALESCING_PARAMETERS *paramSetReceiveCoalescing;
	WDI_SET_PRIVACY_EXEMPTION_LIST_PARAMETERS *paramSetPrivacyExemptionList;
	WDI_SET_POWER_PARAMETERS *paramSetPower;
	WDI_TASK_ABORT_PARAMETERS *paramAbort;
	WDI_SET_ADD_WOL_PATTERN_PARAMETERS *paramSetAddWolPattern;
	WDI_SET_REMOVE_WOL_PATTERN_PARAMETERS *paramSetRemoveWolPattern;
	WDI_SET_MULTICAST_LIST_PARAMETERS *paramSetMulticastList;
	WDI_SET_ADD_PM_PROTOCOL_OFFLOAD_PARAMETERS_PARAMETERS *paramSetAddProtocolOffload;
	WDI_SET_REMOVE_PM_PROTOCOL_OFFLOAD_PARAMETERS *paramSetRemoveProtocolOffload;
	WDI_SET_FIRMWARE_CONFIGURATION_PARAMETERS *paramSetFirmwareConfiguration;
	WDI_GET_RECEIVE_COALESCING_MATCH_COUNT_INPUTS *paramGetReceiveColescingMatchCountInput;
	WDI_SET_ADVERTISEMENT_INFORMATION_PARAMETERS *paramSetAdvertisementInformation;
	WDI_IHV_REQUEST_PARAMETERS *paramIhvRequest;
	WDI_GET_NEXT_ACTION_FRAME_DIALOG_TOKEN_INPUTS *paramGetNextActionFrameDialogTokenInputs;
	WDI_SET_P2P_WPS_ENABLED_PARAMETERS *paramSetP2pWpsEnabled;
	WDI_GET_BSS_ENTRY_LIST_UPDATE_PARAMETERS *paramGetBSSEntryListUpdate;
	WDI_SET_FAST_BSS_TRANSITION_PARAMETERS_COMMAND	*paramSetFastBssTransitionParametersCommand;
}RT_PARSED_TLV;

typedef struct __RT_TLV_PARSER
{
	// dbg info
	size_t					nonCleanedup;
	
	// buffer
	u1Byte					*buf;
	size_t					bufSize;

	// parsed tlv
	RT_PARSED_TLV			parsedTlv;
}RT_TLV_PARSER;

typedef struct _RT_OID_HANDLER
{
	// Associate which NdisRequest to process
	PNDIS_OID_REQUEST		pNdisRequest;
	
	// For task command, NdisRequest is returned with NDIS_STATUS_SUCCESS
	// So copy the content of NdisRequest
	NDIS_OID_REQUEST		PendedRequest;

	// InformationBuffer of NdisRequest, length is stored in NdisRequest
	pu1Byte					pInputBuffer;

	ULONG					AllocatedBufferLength;

	// Output length of this NdisRequest
	ULONG					OutputBufferLength;

	// Keep current handler's status
	u1Byte					Status;

	// Protection of status
	NDIS_SPIN_LOCK			Lock;

	// Record Oid start/returned/complete time
	u8Byte					OidStartTime;
	u8Byte					OidExecutionTime;
	u8Byte					OidReturnTime;

	// Event to notify there is a pending NdisRequest waiting for process
	NDIS_EVENT				PendingEvent;

	// Event to notify completion of NdisRequest
	NDIS_EVENT				CompleteEvent;

	// Associated command entry
	RT_COMMAND_ENTRY		*CmdEntry;

	// Parsed TLV structure
	RT_TLV_PARSER			tlvParser;
	
	// Oid returned status
	NDIS_STATUS				OidStatus;

	RT_WORK_ITEM			WDICommandWorkitem;

	VOID					*pvCtx;
}RT_OID_HANDLER, *PRT_OID_HANDLER;

typedef struct __RT_TASK_ENTRY
{
	RT_COMMAND_ENTRY			super;

	NDIS_STATUS					StatusCode;
	BOOLEAN						bWaitComplete;
	u1Byte						TimeoutSec;

	NDIS_STATUS (*Func)(											// The command handler function
		IN  PADAPTER			pAdapter,
		IN  PRT_OID_HANDLER	pOidHandle
		);

	NDIS_STATUS (*AbortFunc)(
		IN  PADAPTER pAdapter
		);

	NDIS_STATUS (*TimeoutFunc)(
		IN  PADAPTER pAdapter
		);

	NDIS_STATUS (*PreM4Cb)(
		IN  PADAPTER			pAdapter,
		IN  PRT_OID_HANDLER	pOidHandle
		);

	NDIS_STATUS (*PostM4Cb)(
		IN  PADAPTER		pAdapter,
		IN  PVOID			pContext
		);
		
}RT_TASK_ENTRY, *PRT_TASK_ENTRY;


typedef struct __RT_PROPERTY_ENTRY
{
	RT_COMMAND_ENTRY			super;
	
	NDIS_STATUS (*Func)(											// The command handler function
		IN  PADAPTER 			pAdapter,
		IN  PRT_OID_HANDLER		pOidHandle
		);

}RT_PROPERTY_ENTRY, *PRT_PROPERTY_ENTRY;


typedef struct __RT_COMPLETE_ENTRY
{
	RT_LIST_ENTRY	List;
	u4Byte			Oid;
	u4Byte			TransactionId;
	u2Byte			PortId;
}RT_COMPLETE_ENTRY, *PRT_COMPLETE_ENTRY;


//--------------------------------------------------------------------
//
// Here we define required format for WDI
//
//--------------------------------------------------------------------

typedef struct _WDI_DATA_STRUCT
{
	NDIS_HANDLE					DataPathHandle;
	NDIS_WDI_DATA_API			WdiDataApi;
	PNDIS_WDI_INIT_PARAMETERS	pWdiInitParameters;
	WDI_PORT_ID					StaPortId;
	RT_THREAD					TaskThread;
	RT_THREAD					PropertyThread;
	RT_OID_HANDLER				TaskHandle;
	RT_OID_HANDLER				PropertyHandle;
	RT_WORK_ITEM				TaskWorkItem;
	RT_WORK_ITEM				PropertyWorkItem;
	PVOID						LatestScanRequestId;		// temporary
	NDIS_SPIN_LOCK				TaskCompleteIndicationLock;
	RT_LIST_ENTRY				TaskCompleteIndicationList;
	RT_COMPLETE_ENTRY			CurrentTask;
	TLV_CONTEXT					TlvContext;
	u4Byte						txPauseReason;
	BOOLEAN						bWdiLEPLDR; // LE Hang detection and recovery, which currently is for PLDR
	BOOLEAN						bRFOffSwitchProgress;
	RT_OID_HANDLER				TaskPostHangHandle;
	BOOLEAN						bCommandHangHappened;
	RT_LIST_ENTRY				TaskPostHangCompleteIndicationList;
	RT_COMPLETE_ENTRY			CurrentTaskPostHang;
} WDI_DATA_STRUCT, *PWDI_DATA_STRUCT;


typedef struct _TLV_WDI_STRUCT
{
	u2Byte		Type;
	u2Byte		Length;
	u1Byte		Value[1];
} TLV_WDI_STRUCT, *PTLV_WDI_STRUCT;


#endif
