//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_CUSTOMIZED_SCAN_H
#define __INC_CUSTOMIZED_SCAN_H


#define CUSTOMIZED_SCAN_MAX_FRAME_LEN	512
#define CUSTOMIZED_SCAN_MAX_REQ_NUM		8

#define GET_CUSTOM_SCAN_INFO(__pAdapter) (GetDefaultMgntInfo((__pAdapter))->pCustomScanInfo)

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------

typedef enum _CUSTOM_SCAN_STATE
{
	CUSTOM_SCAN_STATE_IDLE = 0,		// req allocated
	CUSTOM_SCAN_STATE_WAITING,		// waiting in the request queue
	CUSTOM_SCAN_STATE_STARTING,
	CUSTOM_SCAN_STATE_STARTED,		// MgntLinkRequest called for the request
	CUSTOM_SCAN_STATE_SW_CHNL,		// switching channel in ScanCallback
	CUSTOM_SCAN_STATE_DWELL,		// dwelling on a specific channel
	CUSTOM_SCAN_STATE_COMPLETED,	// completed
	CUSTOM_SCAN_STATE_PRE_DESTROY,
	CUSTOM_SCAN_STATE_DEFERRED,		// deferred for some reason
	CUSTOM_SCAN_STATE_TERMINATING
}CUSTOM_SCAN_STATE;

typedef
VOID
(*CUSTOM_SCAN_STATE_CB)
(
	IN  CUSTOM_SCAN_STATE		state,
	IN  VOID					*pCtx
);

typedef enum _CUSTOM_SCAN_SRC_TYPE
{
	CUSTOM_SCAN_SRC_TYPE_SYS,
	CUSTOM_SCAN_SRC_TYPE_BT,
	CUSTOM_SCAN_SRC_TYPE_P2P,
	CUSTOM_SCAN_SRC_TYPE_UNSPECIFIED
}CUSTOM_SCAN_SRC_TYPE;

typedef
VOID
(*CUSTOM_SCAN_ON_PROBE_RSP_CB)
(
	IN  CUSTOM_SCAN_STATE		state,
	IN  VOID					*pCtx,
	IN  RT_RFD					*pRfd
);

//-----------------------------------------------------------------------------
// Driver callback
//-----------------------------------------------------------------------------

VOID
CustomScan_ResetReqNoScanCb(
	IN  VOID					*pInfo
	);

VOID
CustomScan_ScanResetCb(
	IN  VOID					*pInfo
	);

VOID
CustomScan_ScanByTimerCb(
	IN  VOID					*pInfo
	);
	
BOOLEAN
CustomScan_ConstructScanListCb(
	IN  VOID					*pInfo,
	IN  RT_CHANNEL_LIST			*pChnlList
	);

u2Byte
CustomScan_PreSwChnlCb(
	IN  VOID					*pInfo,
	IN  RT_CHNL_LIST_ENTRY		*chnl
	);

VOID
CustomScan_OnChnlCb(
	IN  VOID					*pInfo,
	IN  RT_CHNL_LIST_ENTRY		*chnl
	);

BOOLEAN
CustomScan_PreSetDwellTimerCb(
	IN  VOID					*pInfo,
	IN  RT_CHNL_LIST_ENTRY		*chnl
	);

BOOLEAN
CustomScan_SendProbeCb(
	IN  VOID					*pInfo,
	IN  RT_CHNL_LIST_ENTRY		*chnl
	);

BOOLEAN
CustomScan_DualBandScanCb(
	IN  VOID					*pInfo
	);

VOID
CustomScan_PreScanCompleteCb(
	IN  VOID					*pInfo
	);

VOID
CustomScan_ScanCompleteCb(
	IN  VOID					*pInfo
	);

VOID
CustomScan_ScanCompleteReturnCb(
	IN  VOID					*pInfo
	);

VOID
CustomScan_RescheduleCb(
	IN  VOID					*pInfo
	);

VOID
CustomScan_WatchDogCb(
	IN  VOID					*pInfo
	);

VOID
CustomScan_OnProbeRspCb(
	IN  VOID					*pInfo,
	IN	RT_RFD					*pRfd
	);

//-----------------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------------

VOID *
CustomScan_AllocInfo(
	IN  ADAPTER					*pAdapter
	);

VOID
CustomScan_FreeInfo(
	IN  VOID					*pInfo
	);

BOOLEAN
CustomScan_InProgress(
	IN  VOID					*pInfo
	);

VOID
CustomScan_Init(
	IN  VOID					*pInfo,
	IN  ADAPTER					*pScanAdapter,
	IN  u8Byte					dbgComp,
	IN  u4Byte					dbgLevel
	);

VOID
CustomScan_Start(
	IN  VOID					*pInfo
	);

VOID
CustomScan_Stop(
	IN  VOID					*pInfo
	);

VOID
CustomScan_Deinit(
	IN  VOID					*pInfo
	);

VOID *
CustomScan_AllocReq(
	IN  VOID					*pInfo,
	IN  CUSTOM_SCAN_STATE_CB	cb,
	IN  VOID					*ctx
	);

FRAME_BUF *
CustomScan_GetProbeReqBuf(
	IN  VOID					*scanReq
	);

VOID
CustomScan_SetupCbCtx(
	IN  VOID					*scanReq,
	IN  CUSTOM_SCAN_STATE_CB	cb,
	IN  VOID					*ctx
	);

RT_STATUS
CustomScan_TermReq(
	IN  VOID					*pInfo,
	IN  BOOLEAN					bStopOnCurChnl
	);

RT_STATUS
CustomScan_AddScanChnl(
	IN  VOID					*scanReq,
	IN  u1Byte					chnl,
	IN  u1Byte					count,
	IN  RT_SCAN_TYPE			scanType,
	IN  u2Byte					duration,
	IN  u1Byte					dataRate,
	IN  FRAME_BUF				*probeReqBuf
	);

RT_STATUS
CustomScan_AddChnlPlanChnls(
	IN  VOID					*scanReq,
	IN  const RT_CHANNEL_PLAN	*plan,
	IN  RT_SCAN_TYPE			scanType,
	IN  u2Byte					duration,
	IN  u1Byte					dataRate,
	IN  FRAME_BUF				*probeReqBuf
	);

RT_STATUS
CustomScan_SetTimeBound(
	IN  VOID					*scanReq,
	IN  u4Byte					timeBound
	);

RT_STATUS
CustomScan_ForcePassiveScan(
	IN  VOID					*scanReq
	);

RT_STATUS
CustomScan_SetDelayStart(
	IN  VOID					*scanReq,
	IN  u4Byte					delayMs
	);

RT_STATUS
CustomScan_SetRepeatIntermittent(
	IN  VOID					*scanReq,
	IN  u4Byte					repeatIntermittentMs
	);

RT_STATUS
CustomScan_SetRepeatCount(
	IN  VOID					*scanReq,
	IN  u4Byte					count
	);

RT_STATUS
CustomScan_SetProbeRspCb(
	IN  VOID					*scanReq,
	IN  CUSTOM_SCAN_ON_PROBE_RSP_CB onProbeRspCb,
	IN  VOID					*ctx
	);

u4Byte
CustomScan_NumAddedChnl(
	IN  VOID					*scanReq
	);

RT_STATUS
CustomScan_IssueReq(
	IN  VOID					*pInfo,
	IN  VOID					*scanReq,
	IN  CUSTOM_SCAN_SRC_TYPE	type,
	IN  const char				*typeInfo
	);

RT_STATUS
CustomScan_IssueSysScan(
	IN  VOID					*pInfo,
	IN  BOOLEAN					bActiveScan
	);

RT_STATUS
CustomScan_ExtendDwellTime(
	IN  VOID					*pInfo,
	IN  u2Byte					dwellTime
	);

RT_STATUS
CustomScan_ShutDwellTime(
	IN  VOID					*pInfo,
	IN  VOID					*req
	);

const char *
CustomScan_ScanStateTxt(
	IN  CUSTOM_SCAN_STATE		state
	);

BOOLEAN
CustomScan_AcquireCurCtx(
	IN  VOID					*pInfo,
	IN  VOID					*scanReq,
	OUT VOID					**ppCtx
	);

VOID
CustomScan_ReleaseCurCtx(	
	IN  VOID					*pInfo
	);

#endif	// #ifndef __INC_CUSTOMIZED_SCAN_H
