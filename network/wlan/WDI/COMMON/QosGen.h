#ifndef __INC_QOSGEN_H
#define __INC_QOSGEN_H

//=============================================================================
//	Prototype function for Debugging Qos.
//=============================================================================
#if DBG
//
//	Description:
//		Dump the TSPEC IE content.
//
VOID
QosParsingDebug_TspecIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer
	);

VOID
QosParsingDebug_TsrsIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer
	);

VOID
QosParsingDebug_MsduLifetimeIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer
	);

#else
	#define	QosParsingDebug_TspecIE(__Adapter, __pOsBuffer)
	#define	QosParsingDebug_TsrsIE(__Adapter, __pOsBuffer)
	#define QosParsingDebug_MsduLifetimeIE(__Adapter, __pOsBuffer)
#endif // #if DBG
//=============================================================================
//	End of Prototype function for Debugging Qos.
//=============================================================================


VOID 
QosInitializeSTA(
	IN	PADAPTER		Adapter
	);

VOID 
QosDeinitializeSTA(
	IN	PADAPTER		Adapter
	);

VOID 
QosInitializeBssDesc(
	IN	PBSS_QOS		pBssQos
	);


VOID 
QosParsingQoSElement(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			bEDCAParms,
	IN	OCTET_STRING	WMMElement,
	OUT	PRT_WLAN_BSS	pBssDesc
	);

VOID 
QosSetLegacyWMMParamWithHT(
	IN	PADAPTER		Adapter,
	OUT	PRT_WLAN_BSS	pBssDesc
	);

VOID
QosSetLegacyACParam(
	IN	PADAPTER	Adapter
	);

VOID
QosOnAssocRsp(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	asocpdu
	);


VOID
QosOnBeaconUpdateParameter(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pBssDesc
	);


VOID
QosFillHeader(
	IN	PADAPTER	Adapter,
	IN	PRT_TCB		pTcb
	);


BOOLEAN
QosDataCheck(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pFrame
	);

u1Byte
QosGetUserPriority(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pFrame
	);

VOID
QosParsingDebug_BssDesc(
	IN	PRT_WLAN_BSS	pBssDesc
	);


VOID
QosParsingDebug_STA(
	IN	PADAPTER		Adapter
	);



VOID
QosParsingDebug_ParaElement(
	IN	pu1Byte		pWMMParaEle
	);


VOID
QosParsingDebug_AcParam(
	IN	pu1Byte		pAcParam
	);


VOID
QosParsingDebug_QosCtrlField(
	IN	pu1Byte		pFrameHeader
	);

VOID
SendQoSNullFunctionData(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			StaAddr,
	IN	u1Byte			AC,
	IN	BOOLEAN			bForcePowerSave
	);

VOID
QosConstructEDCAParamElem(
	IN	PADAPTER		Adapter,
	OUT POCTET_STRING	posBuffer
	);

VOID
QosConstructTSPEC(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pBuffer,
	OUT pu4Byte			pLength,
	IN	u1Byte			TID,
	IN	u1Byte			Direction,
	IN	BOOLEAN			bPSB,
	IN	BOOLEAN			bSchedule,
	IN	u1Byte			AccessPolicy,
	IN	BOOLEAN			bAggregation,
	IN	u1Byte			AckPolicy,
	IN	u1Byte			TrafficType,
	IN	u1Byte			UserPriority,
	IN	u2Byte			NominalMsduSize,
	IN	u2Byte			MaxMsduSize,
	IN	u4Byte			MinServiceItv,
	IN	u4Byte			MaxServiceItv,
	IN	u4Byte			InactivityItv,
	IN	u4Byte			SuspensionItv,
	IN	u4Byte			ServiceStartTime,
	IN	u4Byte			MinDataRate,
	IN	u4Byte			MeanDataRate,
	IN	u4Byte			PeakDataRate,
	IN	u4Byte			MaxBurstSize,
	IN	u4Byte			DelayBound,
	IN	u4Byte			MinPhyRate,
	IN	u2Byte			SurplusBandwithAllow,
	IN	u2Byte			MediumTime
	);

VOID
QosInitTs(
	IN PADAPTER			Adapter,
	IN PQOS_TSTREAM		pTs,
	IN u1Byte			TSID, 
	IN PWMM_TSPEC		pTSpec
	);

VOID
QosFlushTs(
	IN PADAPTER			Adapter,
	IN PQOS_TSTREAM		pTs
	);

PQOS_TSTREAM 
QosAddTs(
	IN PADAPTER			Adapter,
	IN u1Byte			TSID, 
	IN pu1Byte			RA, 
	IN pu1Byte			TA,
	IN PWMM_TSPEC		pTSpec
	);

PQOS_TSTREAM 
QosGetTs(
	IN PADAPTER			Adapter,
	IN u1Byte			TSID, 
	IN pu1Byte			RA, 
	IN pu1Byte			TA
	);

VOID
QosUpdateTs(
	IN PADAPTER				Adapter,
	IN PQOS_TSTREAM			pTs,
	IN PWMM_TSPEC			pTSpec
	);

VOID
QosSendAddTs(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs,
	IN	u4Byte			numTs
	);

VOID
QosSendDelTs(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs,
	IN	u4Byte			numTs
	);


UINT
QosTsHash(
	IN RT_HASH_KEY			Key
	);

VOID
QosAddTsTimerCallback(
	IN PRT_TIMER		pTimer
	);

VOID
QosRemoveTs(
	IN PADAPTER			Adapter,
	IN PQOS_TSTREAM		pTs
	);

VOID
QosResetTs(
	IN PADAPTER			Adapter,
	IN PQOS_TSTREAM		pTs
	);

BOOLEAN 
QosResetAllTs(
	IN PADAPTER			Adapter
	);

VOID
QosRemoveAllTs(
	IN PADAPTER			Adapter
	);

VOID
QosRecvMsduLifetimeIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer,
	IN	QOSIE_SOURCE	source,
	IN	BOOLEAN			rspStatus
	);

VOID
QosParsingTrafficStreamIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer,
	IN	u4Byte			offset,
	IN	QOSIE_SOURCE	source,
	IN	BOOLEAN			rspStatus
	);

VOID
QosRecvTspecIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer,
	IN	QOSIE_SOURCE	source,
	IN	BOOLEAN			rspStatus
	);

VOID
QosRecvTsrsIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer,
	IN	QOSIE_SOURCE	source,
	IN	BOOLEAN			rspStatus
	);


VOID
QosConstructTSRS(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs,
	IN	POCTET_STRING	pOsAddTsPkt
	);

VOID
QosIncAdmittedTime(
	IN	PADAPTER			Adapter,
	IN	PQOS_TSTREAM		pTs
	);

VOID
QosDecAdmittedTime(
	IN	PADAPTER			Adapter,
	IN	PQOS_TSTREAM		pTs
	);

VOID
QosACMTimerCallback(
	IN PRT_TIMER		pTimer
	);

u1Byte
QosGetNPR(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs
	);

VOID
QosSetNPR(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs,
	IN	u1Byte			rate
	);

BOOLEAN
QosAdmissionControl(
	IN PADAPTER			Adapter,
	IN PRT_TCB			pTcb
	);

BOOLEAN
QosCalcUsedTimeAndAdmitPacket(
	IN	PADAPTER			Adapter,
	IN	PRT_TCB				pTcb
	);

VOID
QosReturnAllPendingTxMsdu(
	IN PADAPTER			Adapter
	);

RT_STATUS
OnAddTsReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
OnAddTsRsp(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);


RT_STATUS
OnDelTs(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

BOOLEAN
IsStaQosTriggerFrame(
	IN	POCTET_STRING		pOSMpdu,
	IN	AC_UAPSD			StaUapsd
	);

VOID
QosResetRxTS(
	IN	PADAPTER		Adapter
	);

#endif // #ifndef __INC_QOSGEN_H
