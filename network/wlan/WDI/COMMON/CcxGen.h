#ifndef __INC_CCX_GEN_H
#define __INC_CCX_GEN_H


#if CCX_SUPPORT
//------------------------------------------------------------------------------
// WLAN/CCX interface.
//------------------------------------------------------------------------------
VOID
CCX_Initialize(
	IN PADAPTER			pAdapter
	);

VOID
CCX_OnSetupJoinInfraInfo(
	IN PADAPTER			pAdapter,
	IN PRT_WLAN_BSS		pRtBss
	);

VOID
CCX_OnAssocOk(
	IN PADAPTER			pAdapter,
	IN POCTET_STRING	posAsocRsp,
	IN BOOLEAN			bReassociate
	);

VOID
CCX_OnBssReset(
	IN PADAPTER			pAdapter
	);

VOID
CCX_OnLinkStatusWatchdog(
	IN	PADAPTER		pAdapter
	);

VOID
CCX_OnScanComplete(
	IN	PADAPTER		pAdapter
	);

VOID
CCX_OnRoamOk(
	IN	PADAPTER		pAdapter,
	IN POCTET_STRING	posReAsocRsp
	);

VOID
CCX_OnRoamFailed(
	IN	PADAPTER		pAdapter
	);

RT_STATUS
CCX_OnIAPPPacket(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
	);

u4Byte
CCX_GetPacketType(
	IN	PADAPTER		pAdapter,
	IN	POCTET_STRING	posMpdu
	);

u4Byte
CCX_ParsePacket(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd
	);

//------------------------------------------------------------------------------
// CCX version and capability.
//------------------------------------------------------------------------------
VOID
CCX_AppendAironetIE(
	IN PADAPTER			pAdapter,
	OUT POCTET_STRING	posFrame
	);

VOID
CCX_AppendCcxVerIE(
	IN PADAPTER			pAdapter,
	OUT POCTET_STRING	posFrame
	);

VOID
CCX_AppendCcxCellPowerIE(
	IN PADAPTER			pAdapter,
	OUT POCTET_STRING	posFrame
	);

VOID
CCX_RM_AppendRmCapIE(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	);

VOID
CCX_AppendMHDRIE(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	);

BOOLEAN
CCX_VerifyRxMFP_MHDRIE(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd
	);


//------------------------------------------------------------------------------
// Layer 2 roaming routines.
//------------------------------------------------------------------------------
VOID 
CCX_ClearAdjacentApReport(
	IN PADAPTER		pAdapter
	);

VOID 
CCX_UpdateAdjacentApReport(
	IN PADAPTER		pAdapter
	);

VOID
CCX_SendAdjacentApReport(
	IN PADAPTER		pAdapter 
	);

unsigned int
CCX_NeighborHash(
	IN RT_HASH_KEY			Key
	);

VOID
CCX_ResetNeighborList(
	IN	PADAPTER		pAdapter
	);

u4Byte
CCX_OnNeighborListResponse(
	IN	PADAPTER		pAdapter,
	IN	POCTET_STRING	posMpdu,
	IN	PRT_RFD			pRfd
	);

VOID
CCX_UpdateNeighborList(
	IN	PADAPTER		pAdapter,
	IN	POCTET_STRING	posMpdu,
	IN	u4Byte			ElementStartOffset,
	IN	PBOOLEAN		pbScan
	);


u4Byte
CCX_OnDirectRoam(
	IN	PADAPTER		pAdapter,
	IN	POCTET_STRING	posMpdu,
	IN	PRT_RFD			pRfd
	);

VOID
CCX_TryToRoam(
	IN	PADAPTER		pAdapter,
	IN	PRT_WLAN_BSS	pRtBss
	);

VOID
CCX_SendNeighborPoll(
	IN PADAPTER		pAdapter 
	);
//------------------------------------------------------------------------------
// Link Test. 
//------------------------------------------------------------------------------
u4Byte
CCX_OnLinkTestRequest(
	IN	PADAPTER		pAdapter,
	IN	POCTET_STRING	posMpdu,
	IN	PRT_RFD			pRfd
	);

VOID
CCX_HandleLinkTestRequest(
	IN	PADAPTER				pAdapter,
	IN	POCTET_STRING			posMpdu,
	IN	s4Byte					RecvSignalPower
	);

VOID
CCX_HandleLinkTestReplySentComplete(
	IN	PADAPTER				pAdapter,
	IN	u2Byte					SeqNo,
	IN	u1Byte					RetryCnt
	);

VOID
CCX_CheckLinkTestRequetQueued(
	IN	PADAPTER				pAdapter
	);

VOID
CCX_ResetLinkTestRequestWaitQueue(
	IN	PRT_CCX_INFO			pCcxInfo
	);

VOID
CCX_AddLinkTestRequestWaitQueue(
	IN	PRT_CCX_INFO			pCcxInfo,
	IN	POCTET_STRING			posMpdu,
	IN	s4Byte					RecvSignalPower
	);

BOOLEAN
CCX_RemoveLinkTestRequestWaitQueue(
	IN	PRT_CCX_INFO			pCcxInfo
	);

#define CCX_IsLinkTestRequestWaitQueueEmpty(_pCcxInfo) ((_pCcxInfo)->LtRequestWaitQueueSize == 0)

PRT_LT_REQ
CCX_GetHeadOfTestRequestWaitQueue(
	IN	PRT_CCX_INFO			pCcxInfo
	);

VOID
CCX_ResetLinkTestReplySet(
	IN	PRT_CCX_INFO			pCcxInfo
	);

VOID
CCX_AddLinkTestReply(
	IN	PADAPTER				Adapter,
	IN	u2Byte					SeqNo,
	IN	u2Byte					FrameNumber	
	);

PRT_LT_REP_RETRY
CCX_FindLinkTestReplyBySeqNo(
	IN	PRT_CCX_INFO			pCcxInfo,
	IN	u2Byte					SeqNo
	);

PRT_LT_REP_RETRY
CCX_FindLinkTestReplyByFrameNumber(
	IN	PRT_CCX_INFO			pCcxInfo,
	IN	u2Byte					FrameNumber
	);

BOOLEAN
CCX_UpdateLinkTestReply(
	IN	PRT_CCX_INFO			pCcxInfo,
	IN	u2Byte					SeqNo,
	IN	u1Byte					RetryCnt
	);

PRT_LT_REP_RETRY
CCX_FindLinkTestReplyByPacketID(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					PacketID
	);

PRT_LT_REP_RETRY
CCX_GetHeadLinkTestReply(
	IN	PRT_CCX_INFO			pCcxInfo
	);
//------------------------------------------------------------------------------
// Qos related.
//------------------------------------------------------------------------------

u4Byte
CCX_CAC_GetVoNominalPhyRate(
	IN	PADAPTER		Adapter,
	IN	u1Byte			Rate
	);

u2Byte
CCX_CAC_GetVoSurplusBandwith(
	IN	PADAPTER		Adapter,
	IN	u1Byte			Rate
	);

VOID
CCX_CAC_ConstructVoiceTspec(
	IN	PADAPTER			Adapter,
	IN	WMM_TSPEC			tspec
	);

VOID
CCX_CAC_ConstructSignalTspec(
	IN	PADAPTER		Adapter,
	IN	WMM_TSPEC		tspec
	);

VOID
CCX_CAC_AddTs(
	IN	PADAPTER		Adapter
	);

VOID
CCX_CAC_DelTs(
	IN	PADAPTER		Adapter
	);

BOOLEAN
CCX_CAC_IsVoiceTsExist(
	IN	PADAPTER		Adapter
	);

BOOLEAN
CCX_CAC_IsSignalTsExist(
	IN	PADAPTER		Adapter
	);

VOID
CCX_CAC_RoamOk(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	posReAsocRsp
	);

VOID
CCX_CAC_RoamFailed(
	IN	PADAPTER		Adapter
	);

BOOLEAN
CCX_CallAdmissionControl(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb
	);

VOID
CCX_SessionRetryTimerCallback(
	IN	PRT_TIMER		pTimer
	);

VOID
CCX_SetAssocInfoTimerCallback(
	IN	PRT_TIMER	pTimer
	);

VOID
CCX_FlushAllTs(
	IN	PADAPTER		Adapter
	);


//------------------------------------------------------------------------------
// SSIDL routines.
//------------------------------------------------------------------------------

//
//Description: 
//			Follow an SSIDInfo to Construct BSS
//Input	     :
//			pSSIDInfo  : an SSID entry of SSIDL
//			pExtenBSS : Output BBS
//			pOrgBSS    : The BSS include this SSIDL
//
VOID
CCX_SSIDL_ConstructBSS(
	IN PADAPTER					Adapter,
	IN PCCX_SSIDL_SSID_ELEMENT	pSSIDInfo,   // Exten SSID information
	IN PRT_WLAN_BSS				pExtenBSS,  // Exten BSS
	IN PRT_WLAN_BSS				pOrgBSS    //  Basic BSS
);

VOID
CCX_GetSSIDLToBssList(
	IN PADAPTER					Adapter,
	IN PRT_WLAN_BSS				pOrgBSS
	);


//
// Description: 
//		Constr New Beacon for Indic to WZC to Pass check 
//
// Input	     :
//			pSSIDInfo  : an SSID entry of SSIDL ( Connect SSID name  )
//
VOID
CCX_SSIDL_UpdateAsocBeacon(
	IN PADAPTER					pAdapter,
	IN PCCX_SSIDL_SSID_ELEMENT	pSSIDInfo,   // Exten SSID information
	IN PRT_RFD					pRfd,
	IN PRT_WLAN_BSS				pOrgBSS    //  Basic BSS
);


VOID
CCX_AppendCcxSFAIE(
	IN PADAPTER			pAdapter,
	OUT POCTET_STRING	posFrame
	);

VOID
CCX_AppendCcxDiagReqReasonIE(
	IN	PADAPTER		pAdapter,
	OUT	POCTET_STRING	posFrame
	);

BOOLEAN
CCX_Construct_DiagChnl_AssocReq(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			Buffer,
	OUT	pu4Byte			pLength
	);

pu1Byte
CCX_HashNextSSIDL(
	IN		POCTET_STRING	posIe,
	IN OUT	pu4Byte			pOffset
	);

VOID
CCX_ConstructReAssociateReq(
	IN	PADAPTER		Adapter,
	OUT	POCTET_STRING	ReAsocReq
	);

VOID
CCX_ConstructDeauthenticatePacket(
	IN	PADAPTER		Adapter,
	OUT	pu1Byte			pdeauth,
	OUT POCTET_STRING	Deauth
	);

VOID
CCX_ConstructDisassociatePacket(
	IN	PADAPTER		Adapter,
	OUT	pu1Byte			pdisassoc,
	OUT POCTET_STRING	Disas
	);


VOID
CCX_ConstructQosADDTSPacket(
	IN	PADAPTER		Adapter,
	OUT POCTET_STRING	osAddTs
	);

VOID
CCX_ConstructQosDELTSPacket(
	IN	PADAPTER		Adapter,
	OUT POCTET_STRING	osDelTs
	);

VOID 
CCX_MgntLinkKeepAlive(
	IN	PADAPTER	pAdapter
	);

VOID
CCX_8021xModeChange(
	IN	PADAPTER		pAdapter,
	IN	BOOLEAN			Value
	);

VOID
CCX_MFPModeChange(
	IN	PADAPTER		pAdapter,
	IN	BOOLEAN			Value
	);

VOID
CCX_DrvIFCompletePacket(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb
	);

BOOLEAN
CCX_IndicateAssociationStart(
	IN	PADAPTER		Adapter
	);

VOID
CCX_GPParserCustomHandleUDP2SIP(
	IN	PADAPTER			Adapter,
	IN	RT_SPINLOCK_TYPE	SpinLockType,
	OUT	BOOLEAN				*pbQosData,
	OUT	BOOLEAN				*pbReturn,
	OUT	BOOLEAN				*pbRejectData
	);

VOID
CCX_OnAddTsRspSet(
	IN	PADAPTER			Adapter
	);

BOOLEAN
CCX_QosReturnAllPendingTxMsdu(
	IN	PADAPTER			Adapter
	);
	
BOOLEAN
CCX_ParseRfd(
	IN	PADAPTER			Adapter,
	IN	PRT_RFD				pRfd,
	IN	OCTET_STRING		frame,
	OUT	BOOLEAN				*pbFreeRfd
	);

VOID
CCX_QueryMFPSupport(
	IN	PADAPTER			Adapter,
	OUT	BOOLEAN				*pbCcxMFPEnable,
	OUT	pu1Byte				pMFPtk
	);

VOID
CCX_QueryMFPAESSupport(
	IN	PADAPTER			Adapter,
	OUT	BOOLEAN				*pbCcxMFPEnable,
	OUT	pu1Byte				MFPAESKeyBuf
	);

VOID
CCX_QueryCCKMSupport(
	IN	PADAPTER			Adapter,
	OUT	BOOLEAN				*pbCCX8021xenable,
	OUT	BOOLEAN				*pbAPSuportCCKM
	);

VOID
CCX_QueryCACSupport(
	IN	PADAPTER			Adapter,
	OUT	BOOLEAN				*pbCcxCACEnable
	);

PCCX_TSM_COUNTER
CCX_QueryTSMSupport(
	IN	PADAPTER			Adapter,
	OUT	BOOLEAN				*pbEnableTSM,
	IN	u1Byte				TsIdx
	);

VOID
CCX_QueryIHVSupport(
	IN	PADAPTER			Adapter,
	OUT	BOOLEAN				*pbIhvFrameLogMode
	);

VOID
CCX_QueryVersionNum(
	IN	PADAPTER			Adapter,
	OUT	u1Byte				*pCurrCcxVerNumber
	);

VOID
CCX_TSMTxFeedbackCallback(
	IN	PADAPTER						pAdapter,
	const RT_TX_FEEDBACK_INFO * const 	pTxFeedbackInfo
	);

VOID
CCX_LinkTestTxFeedbackCallback(
	IN	PADAPTER						pAdapter,
	const RT_TX_FEEDBACK_INFO * const 	pTxFeedbackInfo
	);

VOID
CCX_FwC2HTxRpt(
	IN	PADAPTER	Adapter,
	IN	u1Byte		QueueID,
	IN	pu1Byte		tmpBuf
	);

VOID
CCX_QueryTxPower(
	IN	PADAPTER	Adapter,
	OUT	s4Byte		*pPowerlevel
	);

VOID
CCX_SetAssocReason(
	IN	PADAPTER	Adapter,
	IN	CCX_ASOC_REASON	Reason
	);

VOID
CCX_SetCCKMTimeStamp(
	IN	PADAPTER	Adapter,
	IN	u8Byte		TimeStamp
	);

BOOLEAN
CCX_MlmeAssociateRequest(
	IN	PADAPTER	Adapter
	);

#define	CCX_CATASTROPHIC_REASON_UNSPECIFY	0
#define	CCX_CATASTROPHIC_REASON_TX_FAIL		1

u4Byte
CCX_CatastrophicRoaming(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			roamReason
	);

VOID
CCX_CellPowerLimit(
	IN PADAPTER			pAdapter,
	IN u1Byte			channel,
	IN u1Byte			rate,
	IN pu1Byte			pTxPower
	);

VOID
CCX_UpdateUsedTime(
	PADAPTER	pAdapter
	);

RT_STATUS
CCX_SSIDLUpdateJoinBss(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	PRT_WLAN_BSS	pBssDesc
	);

RT_STATUS
CCX_AppendAssocReqCCKMIE(
	IN	PADAPTER	pAdapter,
	OUT	pu1Byte		pBuffer,
	OUT	pu4Byte		pOutputLen
	);

RT_STATUS
CCX_AppendAssocRspCCKMIE(
	IN	PADAPTER	pAdapter,
	OUT	pu1Byte		pBuffer,
	OUT	pu4Byte		pOutputLen
	);

#else // #if CCX_SUPPORT
#define	CCX_Initialize(_pAdapter)
#define CCX_OnSetupJoinInfraInfo(pAdapter, pRtBss)
#define CCX_OnAssocOk(pAdapter, posAsocRsp, bReassociate)
#define CCX_OnBssReset(pAdapter)
#define CCX_OnLinkStatusWatchdog(pAdapter)
#define CCX_OnScanComplete(pAdapter)
#define CCX_OnIAPPPacket												NULL
#define CCX_ParsePacket(pAdapter, pRfd)									(FALSE)
#define CCX_AppendAironetIE(pAdapter, posFrame)
#define CCX_AppendCcxVerIE(pAdapter, posFrame)
#define CCX_AppendCcxCellPowerIE(pAdapter, posFrame)
#define CCX_RM_AppendRmCapIE(pAdapter, posFrame)
#define CCX_AppendMHDRIE(pAdapter, posFrame)
#define CCX_VerifyRxMFP_MHDRIE(pAdapter, pRfd)							(TRUE)
#define CCX_NeighborHash(Key)											(0)
#define CCX_FindLinkTestReplyByFrameNumber(pCcxInfo, FrameNumber)		(NULL)
#define CCX_HandleLinkTestReplySentComplete(pAdapter, SeqNo, RetryCnt)
#define CCX_FindLinkTestReplyBySeqNo(pCcxInfo, SeqNo)					(NULL)
#define CCX_AddLinkTestReply(Adapter, SeqNo, FrameNumber)
#define CCX_FindLinkTestReplyByPacketID(pAdapter, PacketID)				(NULL)
#define CCX_GetHeadLinkTestReply(pCcxInfo)								(NULL)
#define CCX_OnRoamFailed(pAdapter)
#define CCX_SSIDL_UpdateAsocBeacon(pAdapter, pSSIDInfo, pRfd, pOrgBSS)
#define CCX_HashNextSSIDL(posIe, pOffset)
#define CCX_GetSSIDLToBssList(Adapter, pOrgBSS)
#define CCX_CAC_IsVoiceTsExist(Adapter)									(FALSE)
#define CCX_FlushAllTs(Adapter)
#define CCX_CallAdmissionControl(Adapter, pTcb)							(FALSE)
#define CCX_CAC_AddTs(Adapter)
#define CCX_CAC_DelTs(Adapter)
#define CCX_CAC_ConstructSignalTspec(Adapter, tspec)
#define CCX_CAC_ConstructVoiceTspec(Adapter, tspec)
#define CCX_SessionRetryTimerCallback(pTimer)
#define CCX_SetAssocInfoTimerCallback(pTimer)
#define CCX_AppendCcxSFAIE(pAdapter, posFrame)
#define CCX_Construct_DiagChnl_AssocReq(pAdapter, Buffer,pLength)		(FALSE)
#define CCX_CatastrophicRoaming(pAdapter,roamReason)					(0)
#define CCX_ConstructReAssociateReq(Adapter, ReAsocReq)
#define CCX_ConstructDeauthenticatePacket(Adapter, pdeauth, Deauth)
#define CCX_ConstructDisassociatePacket(Adapter, pdisassoc,Disas)
#define CCX_ConstructQosADDTSPacket(Adapter, osAddTs)
#define CCX_ConstructQosDELTSPacket(Adapter,osDelTs)
#define CCX_MgntLinkKeepAlive(pAdapter)
#define CCX_8021xModeChange(pAdapter, Value)
#define CCX_MFPModeChange(pAdapter, Value)
#define CCX_DrvIFCompletePacket(Adapter, pTcb)
#define CCX_IndicateAssociationStart(Adapter)							(FALSE)
#define CCX_GPParserCustomHandleUDP2SIP(Adapter, SpinLockType, pbQosData, pbReturn, pbRejectData)
#define CCX_OnAddTsRspSet(Adapter)
#define CCX_QosReturnAllPendingTxMsdu(Adapter)							(FALSE)
#define CCX_ParseRfd(Adapter, pRfd, frame, pbFreeRfd)					(FALSE)
#define CCX_QueryMFPSupport(Adapter, pbCcxMFPEnable, pMFPtk)
#define CCX_QueryMFPAESSupport(Adapter,pbCcxMFPEnable, MFPAESKeyBuf)
#define CCX_QueryCCKMSupport(Adapter, pbCCX8021xenable, pbAPSuportCCKM)
#define CCX_QueryCACSupport(Adapter, pbCcxCACEnable)
#define CCX_QueryTSMSupport(Adapter, pbEnableTSM, TsIdx)				(NULL)
#define CCX_QueryIHVSupport(Adapter, pbIhvFrameLogMode)
#define CCX_QueryVersionNum(Adapter, pCurrCcxVerNumber)
#define CCX_TSMTxFeedbackCallback(pAdapter, pTxFeedbackInfo)
#define CCX_LinkTestTxFeedbackCallback(pAdapter, pTxFeedbackInfo)
#define CCX_FwC2HTxRpt(Adapter, QueueID, tmpBuf)
#define CCX_C2HCommandHandler(Adapter, QueueID,tmpBuf)
#define CCX_QueryTxPower(Adapter, pPowerlevel)
#define CCX_SetAssocReason(Adapter,Reason)
#define CCX_SetCCKMTimeStamp(Adapter, TimeStamp)
#define CCX_MlmeAssociateRequest(Adapter)								(FALSE)
#define CCX_CellPowerLimit(Adapter, channel, rate, pTxPower)
#define CCX_UpdateUsedTime(Adapter)		
#define	CCX_SSIDLUpdateJoinBss(_pAdapter, _pRfd, pBssDesc)				(RT_STATUS_NOT_SUPPORT)
#define	CCX_AppendAssocReqCCKMIE(_pAdapter, _pBuffer, _pOutputLen)		(RT_STATUS_NOT_SUPPORT)
#define	CCX_AppendAssocRspCCKMIE(_pAdapter, _pBuffer, _pOutputLen)		(RT_STATUS_NOT_SUPPORT)

#endif // #if CCX_SUPPORT
#endif // #ifndef __INC_CCX_GEN_H
