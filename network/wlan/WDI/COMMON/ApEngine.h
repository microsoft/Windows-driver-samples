

//
// 2011/06/01 MH Add for AP mode switch for code and data memory collection.
//

#define		AP_MODE_SUPPORT				1

//
// Wrapper class of AP mode information.
//
typedef struct _RT_AP_INFO{
	DECLARE_RT_OBJECT(RT_AP_INFO);

	BOOLEAN			bSupportWmm;		// Determine if our AP supports WMM.
	BOOLEAN			bSupportWmmUapsd;  // Determine if our AP supports WMM UAPSD.
	u1Byte			WmmParaCnt;			// WMM parameter Set Count.
	OCTET_STRING	osWmmAcParaIE;
	u1Byte			WmmAcParaBuf[WMM_PARAM_ELEMENT_SIZE];	// Max WMM parameter size is 24 octets.

	BOOLEAN			bSupportCountryIe;	// Determine if our AP supports dot11d.
	OCTET_STRING	osCountryIe;
	u1Byte			CountryIeBuf[MAX_IE_LEN];

	BOOLEAN			bSupportPowerConstraint;	// Determine if our AP supports Power Constraint.
	OCTET_STRING	osPowerConstraintIe;
	u1Byte			PowerConstraintBuf[MAX_DOT11_POWER_CONSTRAINT_IE_LEN];
}RT_AP_INFO, *PRT_AP_INFO;


#define GET_AP_INFO(__pMgntInfo) ( (PRT_AP_INFO)((__pMgntInfo)->pApModeInfo) )

// The timeout value for those packets queued in the AP.
#define	AP_CLIENT_PS_QUEUE_TIMEOUT_SEC			4
#define	AP_CLIENT_PS_QUEUE_TIMEOUT_LOGO_SEC		60

#define	AP_CHNAGE_CLIENT_PS_STATE(__pAdapter, __pEntry, __bPowerSave)	\
{															\
	if(__pEntry->bPowerSave != __bPowerSave)				\
	{														\
		__pEntry->bPowerSave = __bPowerSave;				\
		if(__bPowerSave)									\
			__pAdapter->MgntInfo.PowerSaveStationNum ++;	\
		else												\
			__pAdapter->MgntInfo.PowerSaveStationNum --;	\
		P2PNotifyClientPSChange(GET_P2P_INFO(__pAdapter));	\
	}														\
}

//------------------------------------------------------------------------------
// AssociateEntry related operations.
//------------------------------------------------------------------------------
PRT_WLAN_STA
AsocEntry_GetEntry(
	IN	PMGNT_INFO	pMgntInfo,
	IN	pu1Byte		MacAddr);


PRT_WLAN_STA
AsocEntry_GetEntryByMacId(
	IN	PMGNT_INFO	pMgntInfo,
	IN	u1Byte		MacId
	);

PRT_WLAN_STA
AsocEntry_GetFreeEntry(
	IN	PMGNT_INFO	pMgntInfo,
	IN	pu1Byte		MacAddr);

BOOLEAN
AsocEntry_AddStation(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			MacAddr,
	IN	AUTH_ALGORITHM	AuthAlg);

VOID
AsocEntry_ResetEntry(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry);

VOID
AsocEntry_RemoveStation(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		MacAddr);

VOID
AsocEntry_UpdateTimeStamp(
	IN	PRT_WLAN_STA	pEntry);

BOOLEAN
AsocEntry_AnyStationAssociated(
	IN	PMGNT_INFO	pMgntInfo
	);

BOOLEAN
AsocEntry_IsStationAssociated(
	IN	PMGNT_INFO	pMgntInfo,
	IN	pu1Byte		MacAddr);

VOID
AsocEntry_AgeFunction(
	IN	PADAPTER	Adapter);

VOID
AsocEntry_BecomeDisassoc(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry);

PRT_WLAN_STA
AsocEntry_EnumStation(
	IN	PADAPTER		Adapter,
	IN	ULONG			nIndex);

VOID
AsocEntry_ResetAll(
	IN	PADAPTER		Adapter);

VOID
AsocEntry_UpdateAsocInfo(
	IN	PADAPTER					Adapter,
	IN	pu1Byte						StaAddr,
	IN	pu1Byte						Content,
	IN	u4Byte						ContentLength,
	IN	ASOCENTRY_UPDATE_ASOC_INFO_ACTION	Action
	);

u1Byte
AsocEntry_AssignAvailableAID(
	IN	PMGNT_INFO		pMgntInfo,
	IN	pu1Byte			MacAddr);


VOID
AsocEntry_ResetAvailableAID(
	IN	PMGNT_INFO	pMgntInfo,
	IN	u2Byte		AID);


//------------------------------------------------------------------------------
// AP mode engine.
//------------------------------------------------------------------------------
VOID
AP_Reset(
	IN	PADAPTER		Adapter);


VOID
Ap_SendDisassocWithOldChnlWorkitemCallback(
	IN PVOID			pContext
	);

VOID 
AP_DisconnectAfterSTANewConnected(	
	PADAPTER Adapter
	);

VOID
AP_StatusWatchdog(
	IN	PADAPTER		Adapter
);

VOID
AP_DisassociateAllStation(
	IN	PADAPTER		Adapter,
	IN	u1Byte			asRsn);

VOID
AP_SetupStartApInfo(
	IN	PADAPTER	Adapter);

VOID
AP_StartApRequest(
	IN	PVOID	Context);

BOOLEAN
AP_OnAuthOdd(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	authpdu);

BOOLEAN
AP_OnAsocReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	OCTET_STRING	asocpdu
	);

VOID
AP_SendAsocRsp(
	IN  ADAPTER					*pAdapter,
	IN  RT_WLAN_STA				*pEntry,
	IN  BOOLEAN					bReassoc,
	IN  BOOLEAN					bQosSta,
	IN  u2Byte					status
	);

BOOLEAN
AP_DisassociateStation(
	IN	PADAPTER		Adapter, 
	IN	PRT_WLAN_STA	pSta, 
	IN	u1Byte			asRsn);

VOID
AP_PS_ReturnAllQueuedPackets(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			bMulticastOnly);

PRT_WLAN_STA
AP_PS_UpdateStationPSState(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	posFrame
	);

BOOLEAN
AP_PS_SendPacket(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb);

VOID
AP_PS_FillTim(
	IN	PMGNT_INFO	pMgntInfo);

VOID 
AP_PS_OnPSPoll(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	osMpdu);

BOOLEAN
AP_ForwardPacketWithFromDS(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	BOOLEAN			bNeedCopy);

BOOLEAN
AP_FromWdsToBss(
	IN	PADAPTER		Adapter, 
	IN	PRT_RFD			pRfd,
	IN	BOOLEAN			bNeedCopy);

BOOLEAN
AP_FromBssToWds(
	IN	PADAPTER		Adapter, 
	IN	PRT_RFD			pRfd,
	IN	BOOLEAN			bNeedCopy);

VOID
AP_WdsTx(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb);

BOOLEAN
AP_CheckRSNIE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry,
	IN	OCTET_STRING	asocpdu);

void 
AP_OnEAPOL(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd );



AP_STATE
GetAPState(
	IN	PADAPTER		pAdapter	
	);

VOID
SetAPState(
	IN	PADAPTER		pAdapter,
	IN	AP_STATE		NewSatate
	);


VOID
AP_GetBandWidth(
	IN		PADAPTER	Adapter,
	OUT	 	CHANNEL_WIDTH*	pChnlBW,
	OUT		EXTCHNL_OFFSET* pExtChnlOffset
);



//------------------------------------------------------------------------------
// HT(High Throughput) related operation
//------------------------------------------------------------------------------
VOID
AP_HTResetSTAEntry	(
	PADAPTER			Adapter,
	PRT_WLAN_STA		pSTAEntry
	);

VOID
AP_VHTResetSTAEntry	(
	PADAPTER			Adapter,
	PRT_WLAN_STA		pSTAEntry
	);

VOID
AP_InitRateAdaptive(
	IN	PADAPTER	Adapter	,
	IN	PRT_WLAN_STA  pEntry
	);


u1Byte
AP_CheckBwWidth	(
	PADAPTER	Adapter
	);

VOID
AP_InitRateAdaptiveState(
	IN	PADAPTER	Adapter	,
	IN	PRT_WLAN_STA  pEntry
	);

VOID
DelaySendBeaconTimerCallback(
	IN	PRT_TIMER		pTimer
	);


RT_AP_TYPE
AP_DetermineApType(
	IN PADAPTER pAdapter
	);


BOOLEAN
AP_DetermineAlive(
	IN PADAPTER pAdapter
	);


VOID
AP_InitializeVariables(
	IN PADAPTER 		pAdapter
	);

VOID
Ap_AppendWmmIe(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	);

VOID
Ap_AppendCountryIE(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	);

VOID
Ap_AppendPowerConstraintIE(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	);

VOID
Ap_PsTxFeedbackCallback(
	IN	PADAPTER		pAdapter,
	const RT_TX_FEEDBACK_INFO * const pTxFeedbackInfo
);

VOID 
AP_AllPowerSaveDisable(
	IN PADAPTER 	pAdapter
);

VOID
AP_AllPowerSaveReturn(
	IN PADAPTER		pAdapter
);

VOID
AP_Restart(
	IN	PADAPTER		Adapter
);

VOID
AP_PS_SendAllMcastPkts(
	IN	PADAPTER	pAdapter,
	IN	u1Byte		QueueID
	);

u1Byte
APGetCenterFrequency(
	IN	PADAPTER 	Adapter
);

VOID
AP_SetBandWidth(
	IN	PADAPTER	Adapter
);


