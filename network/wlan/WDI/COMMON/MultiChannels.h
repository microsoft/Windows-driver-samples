
#ifndef __INC_MULTICHANNELS_H
#define __INC_MULTICHANNELS_H

#define MULTICHANNEL_QUEUE_FOR_EXTSTA		BE_QUEUE
#define MULTICHANNEL_QUEUE_FOR_EXTCLIENT		VI_QUEUE
#define MULTICHANNEL_QUEUE_FOR_EXTGO		BK_QUEUE

#define MULTICHANNEL_ACTION_NONE				0
#define MULTICHANNEL_ACTION_STARTED			1
#define MULTICHANNEL_ACTION_PAUSED			2

#define MULTICHANNEL_FCS_SUPPORT_START		1
#define MULTICHANNEL_FCS_SUPPORT_GO			4

#define FCS_STATE_ACTIVE						BIT0
#define FCS_STATE_CONNECTED					BIT1
#define FCS_STATE_PKT_PAUSED					BIT2
#define FCS_STATE_PKT_DROPPED					BIT3

#define FCS_NOTIFY_TYPE_C2H					1
#define FCS_NOTIFY_TYPE_CHANNEL				2
#define FCS_NOTIFY_TYPE_BT_DISABLE			3

typedef enum _MULTICHANNEL_FCS_COMMON_INFO{
	MULTICHANNEL_FCS_COMMON_NUMCLIENT = 1,
	MULTICHANNEL_FCS_COMMON_DURATION = 2,
	MULTICHANNEL_FCS_PORT_ORDER = 3,
	MULTICHANNEL_FCS_PORT_CHANNEL = 4,
	MULTICHANNEL_FCS_PORT_BANDWIDTH = 5,
	MULTICHANNEL_FCS_PORT_EXTOFFSET_40MHZ = 6,
	MULTICHANNEL_FCS_PORT_EXTOFFSET_80MHZ = 7,
	MULTICHANNEL_FCS_PORT_DURATION = 8,
	MULTICHANNEL_FCS_PORT_RFETYPE = 9,
	MULTICHANNEL_FCS_PORT_CHANNEL_TX_NULL = 10,
	MULTICHANNEL_FCS_PORT_C2H_REPORT = 11,
	MULTICHANNEL_FCS_PORT_CHANNEL_SCAN = 12,	
	MULTICHANNEL_FCS_PORT_ROLE = 13,
	MULTICHANNEL_FCS_COMMON_NOA_DURATION = 14,
	MULTICHANNEL_FCS_COMMON_NOA_STARTTIME = 15,
	MULTICHANNEL_FCS_COMMON_STA_BEACONTIME = 16,
	MULTICHANNEL_FCS_COMMON_QPKT_LEVEL = 17,
	MULTICHANNEL_FCS_PORT_MACID_BITMAP = 18,
}MULTICHANNEL_FCS_COMMON_INFO;

// Multi-channel switch role for each port
typedef enum _FCS_ROLE{
	FCS_ROLE_STA = 0,
	FCS_ROLE_AP = 1,
	FCS_ROLE_GC = 2,
	FCS_ROLE_GO = 3,
}FCS_ROLE;

// Represent Channel Tx Null setting
typedef enum _FCS_CHANNEL_TX_NULL{
	FCS_ENABLE_TX_NULL = 0,
	FCS_DISABLE_TX_NULL = 1,
}FCS_CHANNEL_TX_NULL, *PFCS_CHANNEL_TX_NULL;

// Represent Channel Scan
typedef enum _FCS_CHANNEL_SCAN{
	FCS_CHIDX = 0,
	FCS_SCANCH_RSVD_LOC = 1,
}FCS_CHANNEL_SCAN, *PFCS_CHANNEL_SCAN;

// Represent C2H Report setting
typedef enum _FCS_C2H_REPORT{
	FCS_C2H_REPORT_DISABLE = 0,
	FCS_C2H_REPORT_FAIL_STATUS = 1,
	FCS_C2H_REPORT_ALL_STATUS = 2,
}FCS_C2H_REPORT, *PFCS_C2H_REPORT;

// Represent FW status report of channel switch
typedef enum _FCS_STATUS_RPT{
	FCS_RPT_SUCCESS			= 0,
	FCS_RPT_TXNULL_FAIL		= 1,
	FCS_RPT_STOPFCS			= 2,
	FCS_RPT_READY				= 3,
}FCS_STATUS, *PFCS_STATUS;

// Represent FW status
typedef enum _FCS_FW_STATE{
	FCS_FW_IDLE				= 0,
	FCS_FW_STARTING			= 1,
	FCS_FW_STOPPED			= 2,
}FCS_FW_STATE, *PFCS_FW_STATE;

// Represent channel switch operation mode 
typedef enum _FCS_CHNL_OPMODE{
	FCS_CHNL_OPMODE_NONE			=	0,
	FCS_CHNL_OPMODE_RESET			=	1,
	FCS_CHNL_OPMODE_JOINBSS			=	2,
	FCS_CHNL_OPMODE_DISCONNECT		=	3,
	FCS_CHNL_OPMODE_JOINFAIL			=	4,
	FCS_CHNL_OPMODE_STARTMCC		=	5,
}CHNL_OPMODE, *PCHNL_OPMODE;

// Represent queue packet setting
typedef enum _FCS_QPKT_LEVEL{
	FCS_QPKT_DEFAULT				= 0,
	FCS_QPKT_BY_MACID_SLEEP		= 1,
	FCS_QPKT_BY_DRIVER_DROP		= 2,
	FCS_QPKT_BY_MACID_DROP		= 4,
}FCS_QPKT_LEVEL, *PFCS_QPKT_LEVEL;


//============================================================================
// Multiple Channel Supported 
//============================================================================

// Context for Each Port Specific
typedef struct _MULTICHANNEL_PORT_CONTEXT {

	//==================================================
	// Client-Related Elements
	//==================================================
	
	// This controls the offset for this port to the HW TSF counter --
	BOOLEAN bBssLocalTsfValid;
	s8Byte BssLocalTsfOffset;
	u8Byte BssTimestamp;
	u2Byte BssBeaconInterval;
	// -----------------------------------------------------

	// Channel Information ------------------------------------
	CHANNEL_WIDTH	ConnectedBandWidthMode;
	u1Byte 			ConnectedPrimary20MhzChannel;
	EXTCHNL_OFFSET	ConnectedExtChnlOffsetOf40MHz;
	EXTCHNL_OFFSET	ConnectedExtChnlOffsetOf80MHz;

	RT_WORK_ITEM MultiChannelSwitchChannelWorkItem;
	// -----------------------------------------------------

	// Configuration Time --------------------------------------------------------
	u2Byte ConnectionConfigurationTime;	// For speeding up the WPS or DHCP packets: ms
	// ------------------------------------------------------------------------

	// Order for FW fast channel switch
	u1Byte	FCSOrder;
	u1Byte	FCSPortState;

	u1Byte	FcsDuration;
	u1Byte	Role;
	u2Byte	MacIdBitmap;
	
} MULTICHANNEL_PORT_CONTEXT, *PMULTICHANNEL_PORT_CONTEXT;

#define MULTICHANNEL_SIZE_OF_PORT_CONTEXT 		sizeof(MULTICHANNEL_PORT_CONTEXT)



// Common Context for All Ports (Only Valid in Default Port)
typedef struct _MULTICHANNEL_COMMON_CONTEXT {

	// Channel Switch Timer -------------------------------------
	//	+ Future Goal: Handle all channel swith actions for whole driver
	ACTION_TIMER_HANDLE		MultiChannelActionTimer;
	// -------------------------------------------------------


	
	// HW / SW Timer Switch --------------------------------
	BOOLEAN		bUseHardwareTimer; 	
	// ----------------------------------------------------




	// Channel Switch Scheduler Status --------------------
	BOOLEAN bChannelSwitchSchedulerStarted;
	u1Byte		MultiChannelState;
	PADAPTER pAdapterBeforeSwitch;
	// ------------------------------------------------

	// Used for HwTsf Synchronization ----------------- 
	u1Byte TargetApBssid[6];
	BOOLEAN bSyncHwTsfBeaconUpdateReceived;
	RT_WORK_ITEM MultiChannelSyncHwTsfWorkItem;
	// --------------------------------------------

	// Tx Report Indication --------------------------------
	BOOLEAN bWaitChannelSwitchTxReport;
	// -------------------------------------------------

	// For TxPause Read in Dispatch Level: USB and SDIO ---
	u1Byte		TxPauseCache;
	// ----------------------------------------------

	// Record current client count
	u1Byte		NumCurPort;
	u1Byte		FcsDuration;
	BOOLEAN		bDownloadRSVD;
	BOOLEAN		bStartFwFCS;
	u1Byte		uFCSReport;
	u8Byte		uSyncTsfDiff;
	u1Byte		MCCNoADuration;
	u1Byte		MCCNoAStartTime;
	u1Byte		MCCStaBeaconTime;
	u1Byte		MCCQPktLevel;
	RT_WORK_ITEM	MultiChannelC2HWorkItem;
	RT_WORK_ITEM	MultiChannelAdjustTsfWorkItem;
	// ----------------------------------------------

	u8Byte		IGBoostEndTime;			// us
	u4Byte		IGBoostStateFlag;
} MULTICHANNEL_COMMON_CONTEXT, *PMULTICHANNEL_COMMON_CONTEXT;

#define MULTICHANNEL_SIZE_OF_COMMON_CONTEXT 	sizeof(MULTICHANNEL_COMMON_CONTEXT)

typedef struct _FCS_C2H_NOTIFY{
	u1Byte		status;
	u1Byte		channel;
}FCS_C2H_NOTIFY, *PFCS_C2H_NOTIFY;

#define	MC_DM_INIT_GAIN_BOOST_START				0x00000001
#define	MC_DM_INIT_GAIN_BOOST_END				0x00000002
#define	MC_DM_INIT_GAIN_BOOST_END_DELAY_SEC		0x00000003
#define	MC_DM_INIT_GAIN_BOOST_CHECK				0x00000004


#define	MC_DM_STATE_INIT_GAIN_BOOST_STARTED		BIT31

#define	MC_DM_INIT_GAIN_BOOST_DEFAULT_DELAY_SEC	10 // 10 sec


//-----------------------------------------------------------
// Public Functions
//-----------------------------------------------------------

VOID
MultiChannelWriteTxPauseCache(
	PADAPTER	pAdapter,
	u1Byte TxPauseCommand
);

u1Byte
MultiChannelReadTxPauseCache(
	PADAPTER	pAdapter
);

VOID
MultiChannelAssociateConfirm(
	PADAPTER	pAdapter,
	BOOLEAN		bSuccess
);

VOID
MultiChannelHandleReceivedBeacon(
	PADAPTER pAdapter,
	OCTET_STRING osPacket
);

VOID
MultiChannelSwitchChannelWorkItemCallback(
	IN	PVOID	pContext
);

VOID
MultiChannelHandleChannelSwitchToCurrentPort(
	PADAPTER pAdapterBeforeSwitch, 
	PADAPTER pCurrentPort
);

VOID
MultiChannelSyncHwTsfWorkItemCallback(
	IN	PVOID	pContext
);

VOID
MultiChannelResetApStatus(
	PADAPTER pAdapter,
	BOOLEAN bSet
);

VOID
MultiChannelStatusWatchdog(
	PADAPTER pAdapter
);

VOID
MultiChannelDumpHwQueueStatus(
	PADAPTER pAdapter
);

VOID
MultiChannelDisableEnableExactHwQueue(
	PADAPTER pAdapter,
	BOOLEAN bDisable
);

VOID
MultiChannelStopChannelSwitchScheduler(
	PADAPTER pAdapter,		
	BOOLEAN   bEnableSwitchOperation
);

BOOLEAN
MultiChannelSwitchNeeded(
	PADAPTER pAdapter
);

BOOLEAN
MultiChannelSyncHwTsfNeeded(
	PADAPTER pAdapter
);

u1Byte
MultiChannelRedirectPacketToSpecificQueue(
	PRT_TCB	pTcb
);

VOID
MultiChannelUpdateSettingBeforeScanOnResetRequest(
	PADAPTER pAdapter
);

VOID
MultiChannelInitializeWorkItem(
	PADAPTER pAdapter
);

VOID
MultiChannelReleaseWorkItem(
	PADAPTER pAdapter
);

VOID
MultiChannelInitializeTimer(
	PADAPTER pAdapter
);

VOID
MultiChannelReleaseTimer(
	PADAPTER pAdapter
);

u1Byte
MultiChannelGetPortConnected20MhzChannel(
	PADAPTER	pAdapter
);

VOID
MultichannelHandlePacketDuringScan(
	PADAPTER	pAdapter,
	BOOLEAN 	bDisable
);

VOID
MultichannelEnableGoQueue(
	PADAPTER	pAdapter
);

PADAPTER
MultiChannelGetConnectionAdapter(
	PADAPTER 	pAdapter,
	u1Byte		Order
);

VOID
MultiChannelScanHandler(
	PADAPTER 	pAdapter,
	BOOLEAN		bScan,
	BOOLEAN		bResume
);

VOID
MultiChannelDisconnectClient(
	PADAPTER	pAdapter,
	BOOLEAN		bSendDisassociate
);

VOID
MultiChannelDisconnectGo(
	PADAPTER	pAdapter
);

BOOLEAN
MultiChannelCheckUpdateHwTsf(
	PADAPTER pAdapter
);

VOID
MultiChannelGetFcsCommonInfo(
	PADAPTER 	pAdapter,
	u1Byte		FcsFlag,
	pu1Byte		value
);

VOID
MultiChannelSetFcsCommonInfo(
	PADAPTER 	pAdapter,
	u1Byte		FcsFlag,
	pu1Byte		value
);

BOOLEAN
MultiChannel_IsFCSInProgress(
	PADAPTER			pAdapter
);

VOID
MultiChannel_OnBss(
	PADAPTER			pAdapter,
	u8Byte				uAPTsf,
	u8Byte				uCurrentTsf
);

RT_STATUS
FCS_Notify(
	PADAPTER			pAdapter,
	ULONG				NotifyType,
	pu1Byte				InformationBuffer,
	ULONG				InformationBufferLength
);

#if (MULTICHANNEL_SUPPORT == 1)
u1Byte
MultiChannelGetConnectedChannels(
	IN	PADAPTER	pAdapter,
	OUT	pu1Byte 	pChnlArray,
	IN	u1Byte		maxChnlArrayNum
	);

RT_STATUS
McDynamicMachanismSet(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		ReqID,
	IN	PVOID		pInputBuffer,
	IN	u4Byte		InputBufferLen
	);

#else // #if (MULTICHANNEL_SUPPORT != 1)

#define	MultiChannelGetConnectedChannels(_pAdapter, _pChnlArray, _maxChnlArrayNum) 0
#define	McDynamicMachanismSet(_pAdapter, _ReqID, _pInputBuffer, _InputBufferLen)	RT_STATUS_NOT_SUPPORT
#endif // #if (MULTICHANNEL_SUPPORT == 1)

#endif
