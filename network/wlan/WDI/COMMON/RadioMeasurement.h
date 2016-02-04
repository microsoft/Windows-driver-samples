#ifndef __INC_RADIOMEASUREMENT_H
#define __INC_RADIOMEASUREMENT_H

#define	GET_RM_INFO(_pMgntInfo)	((PRT_RM_INFO)(_pMgntInfo->pRmInfo))
#define	RT_LOCAL_RM_TOKEN		0xFE

// RM action status
typedef	enum _RT_RM_ACTION_STATUS{
	RT_RM_ACTION_STATUS_SUCCESS = 0,	// The rm is performed successfully and the report is valid.
	RT_RM_ACTION_STATUS_FAIL = 1,		// The rm had been tried but it failed.
	RT_RM_ACTION_STATUS_GOING = 2,		// The rm is still going.
}RT_RM_ACTION_STATUS, *PRT_RM_ACTION_STATUS;


// RM action status
typedef	enum _RT_RM_HASH_TABLE{
	RT_RM_TABLE_FRAME_REPORT = 0,				// 
	RT_RM_TABLE_NEIGHBOR = 1,					// 
	RT_RM_TABLE_MAX,		// The rm is still going.
}_RT_RM_HASH_TABLE, *P_RT_RM_HASH_TABLE;


VOID
HandleRmRequests(
	IN	PADAPTER		Adapter
);

BOOLEAN
IsCcxRmRequestPacket(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	posMPDU
);

u4Byte
OnCcxRmRequestPacket(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd
	);

VOID
RmMonitorSignalStrength(
	IN PADAPTER			Adapter,
	IN PRT_RFD			pRfd
	);

BOOLEAN
StopCcxTsm(
	IN PADAPTER			Adapter,
	IN u1Byte			TSID
	);

BOOLEAN
StartCcxTsm(
	IN PADAPTER			Adapter,
	IN u1Byte			TSID, 
	IN u2Byte			MeasurementInterval // in TU
	);

VOID
ResetCcxTsmMechanism(
	IN PADAPTER			Adapter
	);


VOID
TsmTimerCallback(
	IN	PRT_TIMER		pTimer
	);

VOID
OnCcxTsmIE(
	IN PADAPTER			Adapter,
	IN POCTET_STRING	posCcxTsmIE	
	);

VOID
HandleCCXRmRequests(
	IN	PADAPTER			Adapter,
	IN	PRT_RM_REQUESTS 	pRmRequests,
	OUT PRT_RM_REPORTS		pRmReports
	);

VOID
HandleDot11kRmRequests(
	IN	PADAPTER			Adapter,
	IN	PRT_RM_REQUESTS 	pRmRequests,
	OUT PRT_RM_REPORTS		pRmReports
	);


VOID
RmWorkitemCallback(
	IN PVOID			pContext
	);

BOOLEAN
RmSetChannelLoad(
	IN	PADAPTER		Adapter,
	IN	u1Byte			Channel,
	IN	u2Byte			Duration
	);

RT_RM_ACTION_STATUS
RmQueryChannelLoad(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		pChLoad
	);

BOOLEAN
RmSetNoiseHistogram(
	IN	PADAPTER		Adapter,
	IN	u1Byte			Channel,
	IN	u2Byte			Duration
	);

RT_RM_ACTION_STATUS
RmQueryNoiseHistogram(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		pIPI,
	IN	u1Byte		IPILen,
	IN	pu1Byte		pANPI
	);

VOID
RM_SetHashTable(
	IN	PADAPTER				Adapter,
	IN	u1Byte					TableType,
	IN	RT_HASH_TABLE_HANDLE	Table
	);

#if (CCX_SUPPORT == 1)

RT_STATUS
OnDot11kRmRequest(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

#else // #if (CCX_SUPPORT != 1)

#define OnDot11kRmRequest			NULL

#endif // #if (CCX_SUPPORT == 1)
#endif // #ifndef __INC_RADIOMEASUREMENT_H
