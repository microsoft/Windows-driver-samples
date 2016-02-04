#ifndef __INC_MGNTLINK_H
#define __INC_MGNTLINK_H

#define MAX_CHANNEL_PLAN	8

//
// Components in decreasing weight to select a BSS:
// - SSID matched: its weight should > sum of components below.
// - BSSID matched: its weight should > sum of components below.
// - From Probe Response: for Vista DTM, its weight should > sum of components below.
// - From Neighbor List: for CCX test plan v4.59 6.2.2.4, its weight should > sum of components below.
// - RSSI: 0-100.
// - channel load: 0-31, for CCX test plan v4.59 6.2.1.1 and 6.2.1.2.
//
#define	WEIGHTING_SAME_SSID			0x8000    // This is for hidden AP (Hidden will not be given this weight)
#define	WEIGHTING_BEST_RSSI			0x5000
#define	WEIGHTING_SAME_BSSID		0x4000
#define	WEIGHTING_PREFER_BAND		0x2000
#define	WEIGHTING_PROBE_RSP			0x1000
#define	WEIGHTING_NEIGHBOR_LIST		0x0800
#define	WEIGHTING_DEFAULT_QBSS_LOAD	0x000F	// half of 32.



#define	JOIN_TIMEOUT					1000		//Timeout time to detect beacon after join an BSS/IBSS

#define	AUTH_REQ_TIMEOUT				1000		//Timeout time to detect beacon after join an BSS/IBSS
#define	ASSOC_REQ_TIMEOUT				1000		//Timeout time to detect beacon after join an BSS/IBSS
#define	ADDTS_TIMEOUT					1000		// Tmeout time to add the ts.


#define	SWRF_TIMEOUT					50
#define	SWRF_TIMEOUT_ANT_DET					2000


#define SW_BEACON_INTERVAL			1000	// Interval of SW beacons.
#define KEY_MGNT_INTERVAL				1000	// Interval of key management for WPA-PSK.

#define	FAST_RESUME_BEACON_TIME			1000
#define	DELAY_START_BEACON			20000

#define	RT_AUTH_RETRY_LIMIT			3//5
#define	RT_ASOC_RETRY_LIMIT			3//5

#define	RT_CONNECT_ROAM_INDICATE_TIME_LIMIT		90	//10sec, in 0.1 sec

#define	RECONNECT_SLOT_COUNT			3
#define	ROAM_RETRY_LIMIT				2

//
// 061207, Roger: 
// Turbo mode related parameter.
//
#define TCA_CHECK_PERIOD			1000	// in ms. 
#define TCA_DEF_CHECK_INTERVAL		2		// in # of TCA_CHECK_PERIOD.
#define TCA_NG_EXTRA_CHECK_INTERVAL	2		// in# of TCA_CHECK_PERIOD, extra penalty when TCA is NG.
#define TCA_OK_CHECK_INTERVAL		1		// in# of TCA_CHECK_PERIOD.

// Justin, 2011.06.22
// Define a packets count threadhold for the dection of running High Throughput testing
#define BUSY_TRAFFIC_THREADHOLD					100			// Tx/Rx packets in a certain period
#define BUSY_TRAFFIC_TP								10			// Tx/Rx packets in a certain period


//#define BUSY_TRAFFIC_HIGH_THREADHOLD			4000		// Tx/Rx packets in a certain period
#define BUSY_TRAFFIC_HIGH_THREADHOLD			2000		// Tx/Rx packets in a certain period, Justin: make this value smaller, so can adjust the USB aggregation faster
#define BUSY_TRAFFIC_HIGH_TP							80		// Tx/Rx packets in a certain period, Justin: make this value smaller, so can adjust the USB aggregation faster

// 2014/09/25 MH Add for very high speed
#define BUSY_TRAFFIC_VERY_HIGH_THREADHOLD			10000		
#define BUSY_TRAFFIC_VERY_HIGH_TP						250		

#define	SCAN_DELAY_TIME_MS_LONG						1000
#define	SCAN_DELAY_TIME_MS_SHORT					50

#define	ASSOC_HANDSHAKE_DHCP_DELAY_SEC				20

#if 0
#define 	MgntIsLinkInProgress(_pMgntInfo)								\
	(																	\
		((_pMgntInfo)->bScanInProgress && !(_pMgntInfo)->bScanOnly)	||	\
		((_pMgntInfo)->JoinTimer.Status&RT_TIMER_STATUS_SET)			||	\
		((_pMgntInfo)->JoinConfirmTimer.Status&RT_TIMER_STATUS_SET)			||	\
		((_pMgntInfo)->JoinProbeReqTimer.Status&RT_TIMER_STATUS_SET)			||	\
		((_pMgntInfo)->AuthTimer.Status&RT_TIMER_STATUS_SET)			||	\
		((_pMgntInfo)->AsocTimer.Status&RT_TIMER_STATUS_SET)			||	\
		((_pMgntInfo)->bJoinInProgress)								\
	)
#endif	

#define	MgntLinkStatusUpdateRxBeacon(_Adapter)		\
			_Adapter->MgntInfo.LinkDetectInfo.NumRecvBcnInPeriod ++;
#define	MgntLinkStatusResetRxBeacon(_Adapter)		\
			_Adapter->MgntInfo.LinkDetectInfo.NumRecvBcnInPeriod = 0;

#define	MgntLinkStatusUpdateRxData(_Adapter,_pSaddr)	\
			_Adapter->MgntInfo.LinkDetectInfo.NumRecvDataInPeriod ++;

#define MgntLinkStatusSetRoamingState(_Adapter, _RoamingFailCount, _RoamingType, _RoamingState) \
			_Adapter->MgntInfo.RoamingFailCount = (_RoamingFailCount);	\
			_Adapter->MgntInfo.RoamingType = (_RoamingType);	\
			_Adapter->MgntInfo.RoamingState  = (_RoamingState);

//
// Determine if the roaming is going now. By Bruce, 2008-05-16.
//
#define	MgntRoamingInProgress(_pMgntInfo)			\
	(!((_pMgntInfo)->RoamingType == RT_ROAMING_NONE))

#define	MgntResetOrPnPInProgress(_Adapter)			\
	((_Adapter)->MgntInfo.bDriverIsGoingToSleep) ||	\
	((_Adapter)->MgntInfo.bResetInProgress)

//
// Reset the status of all variables about roaming in MGNT_INFO, by Bruce, 2008-05-16.
//
#define	MgntResetRoamingState(_pMgntInfo)			\
{													\
	(_pMgntInfo)->RoamingType = RT_ROAMING_NONE;	\
	(_pMgntInfo)->RoamingState = ROAMINGSTATE_IDLE;	\
	(_pMgntInfo)->RoamingFailCount = 0;				\
}

//
// Reset the counters of all variables about join actions in MGNT_INFO, by Bruce, 2008-05-16.
//
#define	MgntResetJoinCounter(_pMgntInfo)			\
{													\
	(_pMgntInfo)->JoinRetryCount = 0;				\
	(_pMgntInfo)->AuthRetryCount = 0;				\
	(_pMgntInfo)->AsocRetryCount = 0;				\
}

#define	MgntInitAdapterInProgress(_pMgntInfo)									\
		((_pMgntInfo)->init_adpt_in_progress)

#define IN_SEND_BEACON_MODE(_Adapter) \
			(( _Adapter->MgntInfo.OpMode == RT_OP_MODE_AP || \
			_Adapter->MgntInfo.OpMode == RT_OP_MODE_IBSS)?TRUE:FALSE)

//----------------------------------------------------------------------------
//  Authenticate
//----------------------------------------------------------------------------
enum	_MlmeAuthenticateRequest_State{
	STATE_Auth_Req_Idle      =  0,
	STATE_Wait_Auth_Seq_2  =  2,
	STATE_Wait_Auth_Seq_4  =  4
};


//----------------------------------------------------------------------------
//  Association
//----------------------------------------------------------------------------
enum	_MlmeAssociateRequest_State{
	STATE_Asoc_Idle,
	STATE_Wait_Asoc_Response,
	STATE_Wait_Reasoc_Response
};

//
// Used for MlmeAssociateConfirm() only.
//
enum MlmeAssocConfirmState
{
	AssocSuccess = 0, // Association Success
	AssocUnspecFail, // Association Fail and select another one to join or indicate disconnected
	AssocRetry, // Retry association again.
};



//----------------------------------------------------------------------------
//  Management status. parm of Mlme operation confirm
//----------------------------------------------------------------------------
enum MlmeStatus {
	MlmeStatus_success        = 0,
	MlmeStatus_invalid          = 1,
	MlmeStatus_timeout         = 2,
	MlmeStatus_refused         = 3,
	MlmeStatus_tomany_req  = 4,
	MlmeStatus_already_bss  = 5
};


//----------------------------------------------------------------------------
//      802.11 Management frame Status Code field
//----------------------------------------------------------------------------
typedef	enum	_AUTH_STATUS{
	AUTH_STATUS_IDLE,			//initialization state
	AUTH_STATUS_IN_PROGRESS,	//authentication packet exchange in progress
	AUTH_STATUS_FAILED,			//last authentication failed
	AUTH_STATUS_SUCCESSFUL,		//last authentication successed
	AUTH_STATUS_METHOD_NOT_MATCH,//last authentication authenation did not started because of method does not match(open vs shared)
}AUTH_STATUS;


//----------------------------------------------------------------------------
//      802.11 Management frame Status Code field
//----------------------------------------------------------------------------
enum StatusCode
{
	StatusCode_success                      = 0,
	StatusCode_Unspecified_failure   = 1,

	// For 802.11z
	StatusCode_TDLS_alternative_wakeup_schedule	= 2,
	StatusCode_TDLS_wakeup_schedule_reject	= 3,
	StatusCode_notallowed_by_BSS	= 4,
	StatusCode_security_disabled		= 5,
	StatusCode_unacceptable_lifetime	= 6,
	StatusCode_notsame_BSS			= 7,
	
	StatusCode_notsupport_cap         = 10,
	StatusCode_reassoc_denied         = 11,
	StatusCode_assoc_denied             = 12,
	StatusCode_notsupport_authalg   = 13,
	StatusCode_error_seqnum            = 14,
	StatusCode_challenge_failure       = 15,
	StatusCode_auth_timeout              = 16,
	StatusCode_assoc_deniedbyap      = 17,
	StatusCode_assoc_deniedbyrate   = 18,

	// For RTL8185
	StatusCode_assoc_notsupport_shortslottime   = 25,
	StatusCode_assoc_notsupport_DSSS_OFDM   = 26,

	StatusCode_request_declined		= 37,
	StatusCode_invalid_ie				= 40,
	StatusCode_invalid_pairwise_cipher	= 42,
	StatusCode_invalid_AKMP			= 43,
	StatusCode_invalid_RSN_cap		= 45,
	StatusCode_invalid_FTIE			= 55,	// Defined in 802.11r
};




//----------------------------------------------------------------------------
//      802.11 Management frame Reason Code field
//----------------------------------------------------------------------------
enum	_ReasonCode{
	unspec_reason	= 0x1,
	auth_not_valid	= 0x2,
	deauth_lv_ss	= 0x3, 
	inactivity		= 0x4,
	ap_overload		= 0x5, 
	class2_err		= 0x6,
	class3_err		= 0x7, 
	disas_lv_ss		= 0x8,
	asoc_not_auth	= 0x9,

	//----MIC_CHECK
	mic_failure		= 0xe,
	//----END MIC_CHECK

	// Reason code defined in 802.11i D10.0 p.28.
	invalid_IE		= 0x0d,
	four_way_tmout	= 0x0f,
	two_way_tmout	= 0x10,
	IE_dismatch		= 0x11,
	invalid_Gcipher	= 0x12,
	invalid_Pcipher	= 0x13,
	invalid_AKMP	= 0x14,
	unsup_RSNIEver = 0x15,
	invalid_RSNIE	= 0x16,
	auth_802_1x_fail= 0x17,
	ciper_reject		= 0x18,

	unreachable_TDLS_peer	= 0x19,	//25 Defined in 802.11z
	teardown_unspec_reacon	= 0x1a,	//26 Defined in 802.11z

	// Reason code defined in 7.3.1.7, 802.1e D13.0, p.42. Added by Annie, 2005-11-15.
	QoS_unspec		= 0x20,	// 32
	QAP_bandwidth	= 0x21,	// 33
	poor_condition	= 0x22,	// 34
	no_facility		= 0x23,	// 35
							// Where is 36???
	req_declined	= 0x25,	// 37
	invalid_param	= 0x26,	// 38
	req_not_honored= 0x27,	// 39
	TS_not_created	= 0x2F,	// 47
	DL_not_allowed	= 0x30,	// 48
	dest_not_exist	= 0x31,	// 49
	dest_not_QSTA	= 0x32,	// 50
	dest_unreachable  = 0x33,	// 51 for win8 dtm
};



//----------------------------------------------------------------------------
//      802.11 Preamble Mode
//----------------------------------------------------------------------------
enum	_REG_PREAMBLE_MODE{
	PREAMBLE_LONG	= 1,
	PREAMBLE_AUTO	= 2,
	PREAMBLE_SHORT	= 3,
};


extern u4Byte DSSS_Freq_Channel[];

extern BOOLEAN		bDebugFixBssid;
extern u1Byte		debug_fixed_bssid[];

BOOLEAN 
AssembleFragmentWcnIeFromMmpdu(
	IN	PADAPTER		Adapter,
	IN    PSIMPLE_CONFIG_T	pSimpleConfig,
	IN OUT	POCTET_STRING posMmpdu
);

PRT_WLAN_BSS 
BssDescDupByDesc(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pRtBSS
	);

PRT_WLAN_BSS 
BssDescDupByBssid(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pBssid
	);

PRT_WLAN_RSSI
BssDescDupByBssid4Rssi(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pBssid
	);

u1Byte
ToLegalChannel(
	PADAPTER		Adapter,
	u8Byte			freq_channel
);


BOOLEAN
GetValueFromBeaconOrProbeRsp(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	OUT	RT_WLAN_BSS		*bssDesc,
	IN	BOOLEAN			bUpdate
	);

VOID
GetScanInfo(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
);

VOID
FilterSupportRate(
	OCTET_STRING	RegRate,
	POCTET_STRING	pSupRate,
	BOOLEAN			bFilterOutCCKRate
	);


VOID
SelectRateSet(
	PADAPTER		Adapter,
	OCTET_STRING	SuppRateSet
);


WIRELESS_MODE
SetupJoinWirelessMode(
	PADAPTER		Adapter,
	BOOLEAN			bSupportNmode,
	WIRELESS_MODE	bssWirelessMode
);



WIRELESS_MODE
SetupStarWirelessMode(
	PADAPTER		Adapter,
	BOOLEAN			bSupportNmode
);


BOOLEAN
IncludedInSupportedRates(
	IN	PADAPTER	Adapter,
	IN	u1Byte		TxRate
);

VOID
ActUpdate_ProtectionMode(
	PADAPTER		Adapter,
	BOOLEAN		bUseProtection
);


VOID
ActUpdate_mCapInfo(
	PADAPTER		Adapter,
	u2Byte			updateCap
);

VOID
ActUpdate_ERPInfo(
	PADAPTER		Adapter,
	u1Byte			ERPInfo
);

RT_JOIN_ACTION
SelectNetworkBySSID(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	*ssid2match,
	IN	BOOLEAN			bRoaming,
	OUT	PRT_WLAN_BSS	pRtBss
	);

BOOLEAN
MlmeAssociateRequest(
	PADAPTER		Adapter,
	pu1Byte			asocStaAddr,
	u4Byte			asocTmot,
	u2Byte			asCap,
	u2Byte			asListenInterval,
	BOOLEAN			Reassociate	
);

RT_STATUS
MlmeAssociateConfirm(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu,
	IN	BOOLEAN			bReassociate,
	IN	u2Byte			result
	);

BOOLEAN
MlmeAuthenticateRequest(
	PADAPTER		Adapter,
	pu1Byte			auStaAddr,
	u1Byte			auAlg,
	u4Byte			auTmot
);

BOOLEAN
MlmeAuthenticateRequest_Confirm(
	PADAPTER		Adapter,
	u2Byte			result
	);

VOID
JoinConfirm(
	PRT_TIMER	pTimer
);

VOID
JoinProbeReq(
	PRT_TIMER	pTimer
);


VOID
MgntRoamComplete(
	IN	PADAPTER		pAdapter,
	IN	u2Byte			Result
	);

VOID
JoinProbeReq(
	PRT_TIMER	pTimer
);

void
MgntDisconnectAP(
        PADAPTER		Adapter,
	u1Byte                  asRsn
);


void
MgntDisconnectIBSS(
	PADAPTER		Adapter
) ;


#define	GENERAL_INDICATE		0x00
#define	FORCE_INDICATE		0x01
#define	FORCE_NO_INDICATE		0x02


void 
MgntIndicateMediaStatus(
	PADAPTER			Adapter,
	RT_MEDIA_STATUS	mstatus,
	u1Byte				Indicatemode
);

void
AsocTimeout(
	PRT_TIMER		pTimer
);

void
AuthTimeout(
	PRT_TIMER		pTimer
);

void
JoinTimeout(
	PRT_TIMER		pTimer
	);

VOID
JoinRequest(
	PADAPTER		Adapter,
	RT_JOIN_ACTION		JoinAction,
	RT_WLAN_BSS		*bssDesc
);

VOID
ScanByTimer(
	PADAPTER	Adapter,
	BOOLEAN		bActiveScan,
	BOOLEAN		bScanOnly
	);

VOID
ScanComplete(
	PADAPTER	Adapter
	);

VOID
ScanCallback(
	PRT_TIMER		pTimer
	);

VOID
WatchDogTimerCallback(
	IN	PRT_TIMER		pTimer
);

VOID
SendDataFrameQueued(
	PADAPTER	Adapter
	);				
// -------------------------------------------
VOID
MgntResetScanProcess(
	PADAPTER		pAdapter
);

VOID
MgntResetLinkProcess(
	PADAPTER		Adapter
);

VOID
MgntResetJoinProcess(
	PADAPTER		Adapter
);

BOOLEAN
MgntLinkRetry(
	PADAPTER		Adapter,
	BOOLEAN			bForceRetry
	);

BOOLEAN
MgntRoamRetry(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN 		bForceRetry
	);

BOOLEAN
MgntLinkRequest(
	PADAPTER		Adapter,
	BOOLEAN        	bScanOnly,
	BOOLEAN         	bActiveScan,
	BOOLEAN        	bFilterHiddenAP,
	BOOLEAN			bUpdateParms,
	POCTET_STRING	pSsid2scan,
	u1Byte          		NetworkType,
	u1Byte          		ChannelNumber,
	u2Byte          		BcnPeriod,
	u1Byte          		DtimPeriod,
	u2Byte          		mCap,
	POCTET_STRING	pSuppRateSet,
	PIbssParms       	pIbpm
	);

 VOID
MgntLinkHandleIPS(
 PADAPTER Adapter
);

  VOID
MgntLinkHandleLPS(
 PADAPTER Adapter
);

VOID
MgntLinkStatusWatchdogForSystemWide(
	PADAPTER 		Adapter
);

BOOLEAN
MgntLinkStatusIsWifiBusy(
	IN	PADAPTER	Adapter
	);

VOID
MgntLinkMultiPortStatusWatchdog(
	PADAPTER 		Adapter
);

VOID
MgntLinkStatusWatchdogForEachPortSpecific(
	PADAPTER		Adapter
);

VOID
MgntResetLinkStatusWatchdogState(
	PADAPTER		Adapter
	);

RT_MEDIA_STATUS
MgntLinkStatusQuery(
	PADAPTER		Adapter
	);

VOID
SwBeaconCallback(
	PRT_TIMER		pTimer
);

VOID
IbssStartRequestCallback(
	IN	PVOID	Context
);

VOID
IbssAgeFunction(
	IN	PADAPTER		Adapter
);

u2Byte
GetRandomU2ByteNumber(
	IN	u2Byte	MinIndex,
	IN	u2Byte	MaxIndex
);

int
GetRandomNumber(
	IN	int	MinIndex,
	IN	int	MaxIndex
);

VOID
GetRandomBuffer(
	OUT	pu1Byte	pHashed
);

VOID
MgntAddRejectedAsocAP(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		AddrOfAP
);

VOID
MgntClearRejectedAsocAP(
	IN	PADAPTER	Adapter
);

u4Byte
MgntGetRejectedAPIndex(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		AddrOfAP
);


VOID
InitTurboChannelAccess(
	PADAPTER	Adapter
);

VOID
DeInitTurboChannelAccess(
	PADAPTER	Adapter
);


VOID
TcaCheckTimerCallback(
	PRT_TIMER		pTimer
);

// For Parsing Realtek information element (sent from 8186). Added by Roger, 2006.12.07.
VOID
GetRealtekIEContentForTurboMode(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pBssDesc,
	IN	POCTET_STRING	pRealtekIE
);

PRT_WLAN_BSS
MgntHasOtherAPwithSameSSID(
	IN	PADAPTER	Adapter
);

BOOLEAN
MgntIsInRejectedAPList(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		AddrOfAP
);

BOOLEAN
MgntTryToRoam(
	IN	PADAPTER	Adapter
);
BOOLEAN
MgntDisconnect(
	IN	PADAPTER		Adapter,
	IN	u1Byte			asRsn
); 


VOID
WaitingKeyTimerCallback(
	PRT_TIMER		pTimer
);


//Keep Alive Mechanism, Isaiah 2008-08-16
//VOID
//MgntLinkKeepAlive(
//	IN	PADAPTER		pAdapter
//);

BOOLEAN
IsInDesiredBSSIDList(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		MacAddr
	);

BOOLEAN
GetDesiredSSIDList(
	IN	PADAPTER	Adapter
	);

VOID
PnPWakeUpJoinTimerCallback(
	IN PRT_TIMER		pTimer
	);

BOOLEAN
CheckBSSSetting(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pRtBss
);


VOID
MgntResumeBeacon(
	IN		PADAPTER	pAdapter
	);

VOID
MgntStopBeacon(
	IN		PADAPTER	pAdapter
	);

BOOLEAN
MgntScanInProgress(
	PMGNT_INFO	pMgntInfo
);

BOOLEAN 
MgntIsLinkInProgress(
	PMGNT_INFO		pMgntInfo
);

BOOLEAN 
MgntIsTimeOutForIndication(
	PMGNT_INFO		pMgntInfo
);

u1Byte
AutoSelectChannel(
	PADAPTER		Adapter, 
	PRT_WLAN_BSS	pBssList,
	int				nNumBss
);

BOOLEAN
IsSendingBeacon(
	PADAPTER	Adapter
);

VOID
Mgnt_SwChnl(
	IN	PADAPTER	Adapter,
	IN	u1Byte		Channel,
	IN	u1Byte		SwitchChannelMethod
);
VOID
Mgnt_BackupVarBeforeScan(
	IN	PADAPTER			Adapter,
	IN	u1Byte				wirelessMode,
	IN	u1Byte				chnl,
	IN	CHANNEL_WIDTH		bw,
	IN	EXTCHNL_OFFSET		chExt
	);

VOID
MgntScanTxFeedbackCallback(
	IN	PADAPTER		pAdapter,
	IN	PVOID			pContext
	);

extern VOID
Hal_PauseTx(
	IN		PADAPTER	Adapter,
	u1Byte	type
	);

VOID
MgntRecoverFWOffloadMechanism(
	IN	PADAPTER			Adapter
	);

VOID
FtResetEntryList(
	IN	PADAPTER	pAdapter
	);

PFT_INFO_ENTRY
FtGetNewEntry(
	IN	PADAPTER			pAdapter,
	IN	pu1Byte				pTargetAddr
	);

PFT_INFO_ENTRY
FtGetEntry(
	IN	PADAPTER			pAdapter,
	IN	pu1Byte				pTargetAddr
	);

RT_STATUS
FtUpdateEntryInfo(
	IN	PADAPTER			pAdapter,
	IN	FT_ENTRY_ACTION		action,
	IN	pu1Byte				pTargetAddr,
	IN	PVOID				pInfoBuffer,
	IN	u4Byte				BufLen	
	);

BOOLEAN
FtIsFtAuthReady(
	IN	PADAPTER			pAdapter,
	IN	pu1Byte				pTargetAddr
	);

BOOLEAN
FtIsFtAssocReqReady(
	IN	PADAPTER			pAdapter,
	IN	pu1Byte				pTargetAddr
	);

RT_STATUS
FtGetWaitOSDecisionAddr(
	IN	PADAPTER	pAdapter,
	OUT	pu1Byte		pAddr
	);

#endif // #ifndef __INC_MGNTLINK_H

