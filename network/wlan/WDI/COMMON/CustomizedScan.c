#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "CustomizedScan.tmh"
#endif

#include "CustomizedScan.h"

static const char *info_sig = "CUSTOM_SCAN";
static const char *req_sig = "CUSTOM_SCAN_REQ";

#define CUSTOM_SCAN_MAX_TIME_WAIT_SCAN_STARTED_MS 300
#define CUSTOM_SCAN_WAIT_LINK_DONE_MS 500

#define CUSTOM_SCAN_PROCESSING_REQ(__info) (NULL != (__info)->pCurReq && !TEST_FLAG((__info)->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_TO_BE_FREED))

#define CUSTOM_SCAN_REQ_FLAG_REQUEUED					BIT0
#define CUSTOM_SCAN_REQ_FLAG_RESCHEDULED				BIT2
#define CUSTOM_SCAN_REQ_FLAG_RESET						BIT3
#define CUSTOM_SCAN_REQ_FLAG_TO_BE_FREED				BIT4
#define CUSTOM_SCAN_REQ_FLAG_POTENTIAL_REPEATE_COUNT_UNDERFLOW BIT5
#define CUSTOM_SCAN_REQ_FLAG_STARTED					BIT6
#define CUSTOM_SCAN_REQ_FLAG_TERMINATE					BIT7
#define CUSTOM_SCAN_REQ_FLAG_FLUSH						BIT8

#define CUSTOM_SCAN_TERM_SCAN_BUF_MS					20
#define CUSTOM_SCAN_ELAPSE_TIME(__req) (PlatformGetCurrentTime() / 1000 - (__req)->issueTime)
#define CUSTOM_SCAN_ALLOW_TIME(__req) (\
				CUSTOM_SCAN_TERM_SCAN_BUF_MS < (__req)->timeBound\
				? (__req)->timeBound - CUSTOM_SCAN_TERM_SCAN_BUF_MS\
				: (__req)->timeBound\
				)
				
#define CUSTOM_SCAN_TIME_DUE(__req) (0 < (__req)->timeBound && CUSTOM_SCAN_ALLOW_TIME(__req) <= CUSTOM_SCAN_ELAPSE_TIME(__req))

#define CHNL_ENTRY_EXT_FLAG_CHNL_SHUT					BIT0

typedef struct _CHNL_ENTRY_EXT
{
	RT_LIST_ENTRY				list;
	
	// super
	RT_CHNL_LIST_ENTRY			super;

	// ext
	FRAME_BUF					*probeReq;
	u1Byte						dataRate;
	u4Byte						flag; // init in customscan_InitChnlEntryExt and reset when requeued
}CHNL_ENTRY_EXT;

typedef struct _CUSTOM_SCAN_REQ
{
	RT_LIST_ENTRY				list;

	const char					*sig;

	u4Byte						flag;

	// source type
	CUSTOM_SCAN_SRC_TYPE		type;
	const char					*typeInfo;

	// state
	CUSTOM_SCAN_STATE			state;
	u8Byte						stateTime;

	// force passive scan 
	BOOLEAN						bForcePassive;
	
	// probe req
	FRAME_BUF					probeReqBuf;
	u1Byte						probeReqRsvd[CUSTOMIZED_SCAN_MAX_FRAME_LEN];

	// channels to scan, cnt is 0 if default scan
	RT_LIST_ENTRY				chnlListQ;
	u4Byte						chnlListCnt;

	// chnl entry pool
	POOL						chnlEntryPool;
	CHNL_ENTRY_EXT				chnlEntryPoolRsvd[MAX_SCAN_CHANNEL_NUM * 2];

	// status callback
	VOID						*cbCtx;
	CUSTOM_SCAN_STATE_CB		cb;

	// dbg
	u8Byte						dbgComp;
	u4Byte						dbgLevel;

	// extended dwell time, reset after applied
	u2Byte						extDwellTime;

	// info of the channel we are on, n/a if default scan
	CHNL_ENTRY_EXT				*curChnl;

	// time bound, 0 if not bounded
	u4Byte						timeBound;

	// time stamps
	u8Byte						issueTime;

	// time in ms to delay before the worker process it
	u4Byte						delayStartMs;

	// time in ms to delay before repeat a req
	u4Byte						repeatIntermittentMs;

	// repeat count
	u4Byte						repeatCount;

	// on probe rsp cb
	CUSTOM_SCAN_ON_PROBE_RSP_CB	onProbeRspCb;
	VOID						*onProbeRspCtx;

}CUSTOM_SCAN_REQ;

typedef struct _CUSTOM_SCAN_INFO
{
	DECLARE_RT_OBJECT(_CUSTOM_SCAN_INFO);

	const char					*sig;

	// the corresp. adapter
	ADAPTER						*pAdapter;

	// request queue
	RT_LIST_ENTRY				reqQ;
	u4Byte						reqCnt;

	// request pool
	POOL						reqPool;
	CUSTOM_SCAN_REQ				reqPoolRsvd[CUSTOMIZED_SCAN_MAX_REQ_NUM];

	// the currently processing req
	CUSTOM_SCAN_REQ				*pCurReq;

	// dbg
	u8Byte						dbgComp;
	u4Byte						dbgLevel;

	// the worker thread
	RT_THREAD					worker;

	// temporary field for constructing scan list
	RT_CHNL_LIST_ENTRY			*scanChnlList[MAX_SCAN_CHANNEL_NUM];
	u1Byte						scanChnlListLen;

	BOOLEAN						bWorkerExited;
	
}CUSTOM_SCAN_INFO;

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

static
VOID
customscan_Lock(
	IN  CUSTOM_SCAN_INFO		*info
	)
{
	PlatformAcquireSpinLock(info->pAdapter, RT_CUSTOM_SCAN_SPINLOCK);
}

static
VOID
customscan_Unlock(
	IN  CUSTOM_SCAN_INFO		*info
	)
{
	PlatformReleaseSpinLock(info->pAdapter, RT_CUSTOM_SCAN_SPINLOCK);
}

static
VOID
customscan_SetReqState(
	IN  CUSTOM_SCAN_INFO		*info,
	IN  CUSTOM_SCAN_REQ			*req,
	IN  CUSTOM_SCAN_STATE		state
	)
{
	req->state = state;
	req->stateTime = PlatformGetCurrentTime();
	if(req->cb)
	{
		(req->cb)(state, req->cbCtx);
	}
}

static
VOID
customscan_InitChnlEntryExt(
	IN  CHNL_ENTRY_EXT			*pExt,
	IN  const RT_CHNL_LIST_ENTRY *pChnl,
	IN  FRAME_BUF				*probeReq,
	IN  u1Byte					dataRate
	)
{
	RTInitializeListHead(&pExt->list);
	pExt->super = *pChnl;
	pExt->probeReq = probeReq;
	pExt->dataRate = dataRate;
	pExt->flag = 0;

	return;
}

static
VOID
customscan_InitReq(
	IN  CUSTOM_SCAN_INFO		*info,
	IN  CUSTOM_SCAN_REQ			*req
	)
{
	PlatformZeroMemory(req, sizeof(*req));
	
	RTInitializeListHead(&req->list);
	req->sig = req_sig;
	req->flag = 0;
	req->type = CUSTOM_SCAN_SRC_TYPE_UNSPECIFIED;
	req->state = CUSTOM_SCAN_STATE_IDLE;
	FrameBuf_Init(sizeof(req->probeReqRsvd), 0, req->probeReqRsvd, &req->probeReqBuf);
	
	RTInitializeListHead(&req->chnlListQ);
	req->chnlListCnt = 0;

	Pool_Init(&req->chnlEntryPool, "chnl entry pool", sizeof(req->chnlEntryPoolRsvd), 
		req->chnlEntryPoolRsvd, sizeof(req->chnlEntryPoolRsvd[0]), 
		BIT45, DBG_LOUD);

	req->cbCtx = NULL;
	req->cb = NULL;

	req->dbgComp = BIT45;
	req->dbgLevel = DBG_LOUD;

	req->extDwellTime = 0;
	req->curChnl = NULL;

	req->repeatCount = 1;
	
	return;
}

static
VOID
customscan_DescructReq(
	IN  CUSTOM_SCAN_INFO		*info,
	IN  CUSTOM_SCAN_REQ			*req
	)
{
	while(RTIsListNotEmpty(&req->chnlListQ))
	{
		RT_LIST_ENTRY *pEntry = RTRemoveTailListWithCnt(&req->chnlListQ, &req->chnlListCnt);

		Pool_Release(&req->chnlEntryPool, pEntry);
	}

	return;
}

static
VOID
customscan_FreeReq(
	IN	CUSTOM_SCAN_INFO		*info,
	IN	CUSTOM_SCAN_REQ 		*req
	)
{
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("freeing: %s\n", req->typeInfo));
	customscan_DescructReq(info, req);
	Pool_Release(&info->reqPool, req);

	Pool_Dump(&info->reqPool);

	return;
}

static
VOID
customscan_FreeCurReq(
	IN	CUSTOM_SCAN_INFO		*info
	)
{
	customscan_FreeReq(info, info->pCurReq);
	info->pCurReq = NULL;

	return;
}

static
VOID
customscan_FlushReqQ(
	IN  CUSTOM_SCAN_INFO		*info
	)
{
	RT_LIST_ENTRY				*pEntry = NULL;

	while(RTIsListNotEmpty(&info->reqQ))
	{
		pEntry = RTRemoveTailListWithCnt(&info->reqQ, &info->reqCnt);
		customscan_Unlock(info);
		customscan_SetReqState(info, (CUSTOM_SCAN_REQ *)pEntry, CUSTOM_SCAN_STATE_PRE_DESTROY); // so client has a chance to cleanup
		customscan_Lock(info);
		SET_FLAG(((CUSTOM_SCAN_REQ *)pEntry)->flag, CUSTOM_SCAN_REQ_FLAG_FLUSH);
		customscan_FreeReq(info, (CUSTOM_SCAN_REQ *)pEntry);
	}

	return;
}

static
RT_CHNL_LIST_ENTRY *
customscan_GetScanChnlInDrvChnlList(
	IN  RT_CHNL_LIST_ENTRY		**pChnlList,
	IN  u1Byte					chnlListLen,
	IN  u1Byte					chnl
	)
{
	u4Byte						itChnl = 0;
	
	for(itChnl = 0; itChnl < chnlListLen; itChnl++)
	{
		if(pChnlList[itChnl]->ChannelNum == chnl)
			return pChnlList[itChnl];
	}

	return NULL;
}

static
const char *
customscan_ScanStateTxt(
	IN  CUSTOM_SCAN_STATE		state
	)
{
	switch (state) 
	{
	case CUSTOM_SCAN_STATE_IDLE:
		return "idle";
	case CUSTOM_SCAN_STATE_WAITING:
		return "waiting";
	case CUSTOM_SCAN_STATE_STARTING:
		return "starting";
	case CUSTOM_SCAN_STATE_STARTED:
		return "started";
	case CUSTOM_SCAN_STATE_SW_CHNL:
		return "swChnl";
	case CUSTOM_SCAN_STATE_DWELL:
		return "dwell";
	case CUSTOM_SCAN_STATE_COMPLETED:
		return "completed";
	case CUSTOM_SCAN_STATE_PRE_DESTROY:
		return "pre-destroy";
	case CUSTOM_SCAN_STATE_DEFERRED:
		return "deferred";
	case CUSTOM_SCAN_STATE_TERMINATING:
		return "terminating";
	default:
		return "Unknown?!";
	}
}

static
const char *
customscan_SrcTypeTxt(
	IN  CUSTOM_SCAN_SRC_TYPE		type
	)
{
	switch (type) 
	{
	case CUSTOM_SCAN_SRC_TYPE_SYS:
		return "sys";
	case CUSTOM_SCAN_SRC_TYPE_BT:
		return "bt";
	case CUSTOM_SCAN_SRC_TYPE_P2P:
		return "p2p";
	case CUSTOM_SCAN_SRC_TYPE_UNSPECIFIED:
		return "unspecified";
	default:
		return "Unknown?!";
	}
}

static
VOID
customscan_DumpChnlExt(
	IN  const CHNL_ENTRY_EXT	*chnlExt,
	IN  u8Byte					dbgComp,
	IN  u4Byte					dbgLevel
	)
{
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("chnl: %u, active: %u, duration: %u, flag: 0x%08X\n", 
		chnlExt->super.ChannelNum, 
		SCAN_ACTIVE == chnlExt->super.ScanType, 
		chnlExt->super.ScanPeriod,
		chnlExt->flag
		));
}

static
VOID
customscan_DumpReq(
	IN  const CUSTOM_SCAN_REQ	*req
	)
{
	RT_LIST_ENTRY				*pEntry = NULL;

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("type: %s\n", customscan_SrcTypeTxt(req->type)));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("type info: %s\n", req->typeInfo));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("state: %s\n", customscan_ScanStateTxt(req->state)));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("sig: %s\n", req->sig));
	FrameBuf_Dump(&req->probeReqBuf, BIT45, DBG_LOUD, "ProbeReqBuf");
	RT_TRACE(COMP_SCAN, req->dbgLevel, ("chnl list:\n"));
	RtEntryListForEach(&req->chnlListQ, pEntry)
	{
		customscan_DumpChnlExt((CHNL_ENTRY_EXT *)pEntry, BIT45, DBG_LOUD);
	}
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("timeBound: %u\n", req->timeBound));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("delayStartMs: %u\n", req->delayStartMs));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("repeatIntermittentMs: %u\n", req->repeatIntermittentMs));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("repeatCount: %u\n", req->repeatCount));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("cb: %p\n", req->cb));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("ctx: %p\n", req->cbCtx));

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("flag: 0x%08X\n", req->flag));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("state: %s\n", customscan_ScanStateTxt(req->state)));
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("time in state: %u\n", (u4Byte)(PlatformGetCurrentTime() - req->stateTime)));

	return;
}

static
VOID
customscan_DumpReqQ(
	IN  const CUSTOM_SCAN_INFO	*info
	)
{
	RT_LIST_ENTRY				*pEntry = NULL;

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("reqQ: %u\n", info->reqCnt));
	
	RtEntryListForEach(&info->reqQ, pEntry)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("req:\n"));
		customscan_DumpReq((CUSTOM_SCAN_REQ *)pEntry);
	}

	return;
}

static
VOID
customscan_DumpReqPool(
	IN  const CUSTOM_SCAN_INFO	*info
	)
{
	const CUSTOM_SCAN_REQ		*pEntry = NULL;
	size_t						poolSize = sizeof(info->reqPoolRsvd) / sizeof(info->reqPoolRsvd[0]);
	size_t						it = 0;

	// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("req pool size: %u\n", poolSize));

	for(it = 0; it < poolSize; it++)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("req:\n"));
		pEntry = &info->reqPoolRsvd[it];
		customscan_DumpReq(pEntry);
	}

	return;
}

static
RT_STATUS
customscan_TerminateCurrentChnl(
	IN  CUSTOM_SCAN_INFO		*info
	)
{
	ADAPTER						*pAdapter = info->pAdapter;

	if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		return RT_STATUS_INVALID_STATE;

	RT_TRACE(COMP_SCAN, DBG_LOUD, 
		("req: %s, state: %s\n", 
		info->pCurReq->typeInfo,
		customscan_ScanStateTxt(info->pCurReq->state)));
	
	if(CUSTOM_SCAN_STATE_DWELL == info->pCurReq->state)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("terminating cur req on chnl\n"));

		customscan_Unlock(info);
		if(IS_DUAL_BAND_SUPPORT(pAdapter))
			pAdapter->MgntInfo.bDualModeScanStep = 2;
		PlatformCancelTimer(pAdapter, &pAdapter->MgntInfo.ScanTimer);
		PlatformSetTimer(pAdapter, &pAdapter->MgntInfo.ScanTimer, 0);
		customscan_Lock(info);
	}

	return RT_STATUS_SUCCESS;
}

static
BOOLEAN
customscan_FreeDanglingReq(
	IN	CUSTOM_SCAN_INFO		*info
	)
{
	BOOLEAN						bFreed = FALSE;

	do
	{
		if(!info->pCurReq)
			break;

		if(!TEST_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_RESET))
			break;

		// the scan is reset, ScanComplete for the req may/may not entered
		// however if ScanComplete entered, we can't get here since the req wold have been freed
		RT_TRACE(COMP_SCAN, DBG_WARNING, ("freeing dangling req: %s got reset and ScanComplete not enter, free it and continue\n", info->pCurReq->typeInfo));
		customscan_Unlock(info);
		customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_PRE_DESTROY);
		customscan_Lock(info);
		customscan_FreeCurReq(info);

		bFreed = TRUE;
	}while(FALSE);
	
	return bFreed;
}

static
RT_STATUS
customscan_TerminateCurrentReq(
	IN  CUSTOM_SCAN_INFO		*info,
	IN  BOOLEAN					bStopOnCurChnl
	)
{
	ADAPTER						*pAdapter = info->pAdapter;
	RT_CHNL_LIST_ENTRY 			*pChnlEntry = NULL;

	if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		return RT_STATUS_INVALID_STATE;

	RT_TRACE(COMP_SCAN, DBG_LOUD, 
		("req: %s, state: %s, ScanStep: %u\n", 
			info->pCurReq->typeInfo,
			customscan_ScanStateTxt(info->pCurReq->state),
			pAdapter->MgntInfo.ScanStep
		)
	);
	
	if(CUSTOM_SCAN_STATE_SW_CHNL == info->pCurReq->state
		|| CUSTOM_SCAN_STATE_DWELL == info->pCurReq->state
		|| CUSTOM_SCAN_STATE_STARTED == info->pCurReq->state
		|| CUSTOM_SCAN_STATE_STARTING == info->pCurReq->state
		)
	{// doing scan callback
		while(RtActChannelList(pAdapter, RT_CHNL_LIST_ACTION_POP_SCAN_CHANNEL, NULL, &pChnlEntry)){}

		if(bStopOnCurChnl)
		{
			ADAPTER 			*pda = GetDefaultAdapter(pAdapter);
			ADAPTER 			*pla = pda;
			u1Byte				curChnl = RT_GetChannelNumber(pla);				
			EXTCHNL_OFFSET		hwBW40MOffset = EXTCHNL_OFFSET_NO_EXT;
			EXTCHNL_OFFSET		hwBW80MOffset = EXTCHNL_OFFSET_NO_EXT;
	
			pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_BW40MHZ_EXTCHNL, (pu1Byte)(&hwBW40MOffset));
			pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_BW80MHZ_EXTCHNL, (pu1Byte)(&hwBW80MOffset));

			while(pla)
			{
				pla->MgntInfo.SettingBeforeScan.ChannelNumber = curChnl;

				pda->MgntInfo.SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz = hwBW40MOffset;
				pda->MgntInfo.SettingBeforeScan.Ext40MHzChnlOffsetOf80MHz = hwBW80MOffset;

				if(EXTCHNL_OFFSET_NO_EXT != hwBW80MOffset)
				{
					pda->MgntInfo.SettingBeforeScan.ChannelBandwidth = CHANNEL_WIDTH_80;
				}
				else if(EXTCHNL_OFFSET_NO_EXT != hwBW40MOffset)
				{
					pda->MgntInfo.SettingBeforeScan.ChannelBandwidth = CHANNEL_WIDTH_40;
				}
				else
				{
					pda->MgntInfo.SettingBeforeScan.ChannelBandwidth = CHANNEL_WIDTH_20;
				}
				
				pla->MgntInfo.SettingBeforeScan.CenterFrequencyIndex1 = CHNL_GetCenterFrequency(
							pda->MgntInfo.SettingBeforeScan.ChannelNumber,
							pda->MgntInfo.SettingBeforeScan.ChannelBandwidth,
							pda->MgntInfo.SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz);			
				pla = GetNextExtAdapter(pla);
			}
		
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("to stop on chnl: %u\n", curChnl));
		}

		customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_TERMINATING);
		SET_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_TERMINATE);

		RT_TRACE(COMP_SCAN, DBG_LOUD, ("cur req terminated\n"));

		customscan_Unlock(info);
		pAdapter->MgntInfo.ScanStep = 0;
		if(IS_DUAL_BAND_SUPPORT(pAdapter))
			pAdapter->MgntInfo.bDualModeScanStep = 2;
		PlatformCancelTimer(pAdapter, &pAdapter->MgntInfo.ScanTimer);
		PlatformSetTimer(pAdapter, &pAdapter->MgntInfo.ScanTimer, 0);
		customscan_Lock(info);
	}
	else if(CUSTOM_SCAN_STATE_COMPLETED == info->pCurReq->state)
	{// doing scan complete
	}
	else if(CUSTOM_SCAN_STATE_TERMINATING == info->pCurReq->state
		&& TEST_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_RESET)
		)
	{
		// Previously in terminating state because of scan reset 
		// and now the req is to be terminated again (aborted by OS) 
		// before ScanComplete is invoked.
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("%s(): req %s is in terminating state and reset flag set, free it\n", __FUNCTION__, info->pCurReq->typeInfo));
		customscan_FreeDanglingReq(info);
	}
	
	else
	{
		RT_ASSERT(TRUE, ("invalid req state: %s\n", customscan_ScanStateTxt(info->pCurReq->state)));
		return RT_STATUS_FAILURE;
	}

	return RT_STATUS_SUCCESS;
}

static
VOID
customscan_RequeueCurReq(
	IN  CUSTOM_SCAN_INFO		*info
	)
{
	size_t						it = 0;
	
	FunctionIn(COMP_SCAN);
	
	customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_DEFERRED);
	SET_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_REQUEUED);
	for(it = 0; 
		it < sizeof(info->pCurReq->chnlEntryPoolRsvd) / sizeof(info->pCurReq->chnlEntryPoolRsvd[0]); 
		it++
		)
	{
		info->pCurReq->chnlEntryPoolRsvd[it].flag = 0;
	}
	info->pCurReq->curChnl = NULL;
	
	RTInsertHeadListWithCnt(&info->reqQ, &info->pCurReq->list, &info->reqCnt);
	info->pCurReq = NULL;

	FunctionOut(COMP_SCAN);
}

static
BOOLEAN
customscan_LinkInProgress(
	IN  CUSTOM_SCAN_INFO		*info
	)
{
	BOOLEAN						bLinkInProgress = FALSE;
	ADAPTER 					*pla = NULL;

	for(pla = GetDefaultAdapter(info->pAdapter);
		NULL != pla;
		pla = GetNextExtAdapter(pla)
		)
	{
		if(MgntIsLinkInProgress(&pla->MgntInfo))
		{
			bLinkInProgress = TRUE;
			break;
		}
	}

	return bLinkInProgress;
}

static
BOOLEAN
customscan_InvokeMgntLinkReq(
	IN  CUSTOM_SCAN_INFO		*info,
	IN  BOOLEAN					bForcePassive
	)
{
	BOOLEAN						bScanStarted = FALSE;
	
	//
	// Formerly, system scan comes from N6CSet_DOT11_SCAN_REQUEST and may
	// be called directly down to ScanCallback if timeout specified is 0,
	// and if scan is rejected by MgntLinkRequest (or the function it calls)
	// N6CSet_DOT11_SCAN_REQUEST does not try to handle this.
	// So for N6CSet_DOT11_SCAN_REQUEST, it is safe to simply call 
	// MgntLinkRequest here and don't care about indicate scan complete at 
	// all.
	// 
	
	MgntLinkRequest(
			info->pAdapter,
			TRUE,		//bScanOnly
			bForcePassive ? FALSE : TRUE, //bActiveScan,
			FALSE,		//FilterHiddenAP // Asked by Netgear's Lancelot for 8187 should look like their damn wg111v1, 2005.02.01, by rcnjko.
			FALSE,		// Update parameters
			NULL,		//ssid2scan
			0,			//NetworkType,
			0,			//ChannelNumber,
			0,			//BcnPeriod,
			0,			//DtimPeriod,
			0,			//mCap,
			NULL,		//SuppRateSet,
			NULL		//yIbpm,
			);

	//
	// Note: the following code handles the cases scan is not actually started
	// 	and we need to free the custom scan req.
	// 
	// pCurReq may be NULL:
	// 	After MgntLinkRequest, pCurReq may have been freed by the scan time check
	// 	mechanism (pMgntInfo->bCheckScanTime). In this case, the req is safely 
	// 	freed (or requeued if repeat count is not 0) in CustomScan_ScanCompleteCb.
	//
	// MgntLinkRequest rescheduled:
	// 	If MgntLinkRequest is rescheduled via MgntLinkRequestReschedule, it 
	// 	will be called again when RF is turned on in InactivePsTimerCallback.
	// 	In this case, we leave the custom scan req as the cur req and it will 
	// 	be activated again when RF is on.
	//
	// If scan request is rejected by MgntLinkRequest non rescheduled cases or
	// ScanByTimer, we need to free the request here.
	// 
	// If scan request is rejected by ScanCallback (and note that the scan timer
	// is not set again when returned), such as MgntResetOrPnPInProgress, we are
	// not able to detect that here, and the state of the custom scan req is left
	// as starting and will be recycled by the worker if the state time exceeds 
	// CUSTOM_SCAN_MAX_TIME_WAIT_SCAN_STARTED_MS.
	//

	customscan_Lock(info);
	if(info->pCurReq)
	{
		if(CUSTOM_SCAN_STATE_STARTED <= info->pCurReq->state)
		{
			bScanStarted = TRUE;
		}
		else
		{// ScanByTimer or its callers reject the scan request
			if(TEST_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_RESCHEDULED))
			{// MgntLinkRequest shall be called again when RF is turned on
				// in this case, scan is "scheduled" to be started when RF is on, so return true
				bScanStarted = TRUE;
			}
			else
			{// other cases such as hw/sw radio off
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("scan did not started and not rescheduled\n"));				customscan_Unlock(info);
				customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_PRE_DESTROY);
				customscan_Lock(info);
				customscan_FreeCurReq(info);
			}
		}
	}
	customscan_Unlock(info);

	return bScanStarted;
}

static
VOID
customscan_WorkerCb(
	IN  VOID 					*pCtx
	)
{
	RT_THREAD					*threadCtx = (RT_THREAD *)pCtx;
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)threadCtx->pContext;
	u4Byte						bDelayMs = 0;
	BOOLEAN						bForcePassive = FALSE;

	FunctionIn(COMP_SCAN);

	RT_ASSERT(pCtx, ("%s(): pCtx is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	if(MgntScanInProgress(&info->pAdapter->MgntInfo))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("skip for there's ongoing req\n"));
		FunctionOut(COMP_SCAN);
		return;
	}

	if(customscan_LinkInProgress(info))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, 
			("skip and wait %u ms for link in progress\n", 
			CUSTOM_SCAN_WAIT_LINK_DONE_MS));
		PlatformSleepUs(CUSTOM_SCAN_WAIT_LINK_DONE_MS * 1000);
		PlatformSetEventToTrigerThread(info->pAdapter, &info->worker);
		FunctionOut(COMP_SCAN);
		return;
	}

	customscan_Lock(info);

	if(info->pCurReq)
	{
		if(TEST_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_RESET))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("%s(): req %s is in terminating state and reset flag set, free it\n", __FUNCTION__, info->pCurReq->typeInfo));

			customscan_FreeDanglingReq(info);
			// to continue processing req in queue
		}
		else
		{
			if(CUSTOM_SCAN_STATE_STARTING == info->pCurReq->state)
			{
				if(CUSTOM_SCAN_MAX_TIME_WAIT_SCAN_STARTED_MS < PlatformGetCurrentTime() - info->pCurReq->stateTime)
				{
					RT_TRACE(COMP_SCAN, DBG_WARNING, ("requeue req: %s for failed to start\n", info->pCurReq->typeInfo));
					customscan_FreeCurReq(info);
				}
				customscan_Unlock(info);
				FunctionOut(COMP_SCAN);
				return;
			}

			if(CUSTOM_SCAN_STATE_PRE_DESTROY == info->pCurReq->state)
			{
				RT_TRACE(COMP_SCAN, DBG_WARNING, ("cur req: %s is in its pre destroy state\n", info->pCurReq->typeInfo));
				customscan_Unlock(info);
				FunctionOut(COMP_SCAN);
				return;
			}	
		}
	}

	// There's ongoing custom req when scan in progress is false
	if(info->pCurReq)
	{
		if(TEST_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_TERMINATE))
		{// to wait until it is freed
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("req %s is being terminated, not to start another one\n", info->pCurReq->typeInfo));
		}
		else
		{
			RT_ASSERT(NULL == info->pCurReq, 
				("%s(): there's ongoing custom req %s when scan in progress is false, state: %s\n", 
				__FUNCTION__, info->pCurReq->typeInfo, customscan_ScanStateTxt(info->pCurReq->state)));
		}

		customscan_Unlock(info);
		FunctionOut(COMP_SCAN);
		return;
	}
	
	// Check if any request queued
	if(!info->reqCnt)
	{
		customscan_Unlock(info);
		FunctionOut(COMP_SCAN);
		return;
	}

	// Make curReq ready so it is available in the callbacks
	info->pCurReq = (CUSTOM_SCAN_REQ *)RTRemoveHeadListWithCnt(&info->reqQ, &info->reqCnt);
	RT_ASSERT(CUSTOM_SCAN_STATE_WAITING == info->pCurReq->state || CUSTOM_SCAN_STATE_DEFERRED == info->pCurReq->state, 
		("%s(): invalid req state: %s\n", __FUNCTION__, customscan_ScanStateTxt(info->pCurReq->state)));
	customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_STARTING);

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("starting req: %s\n", info->pCurReq->typeInfo));

	if(!TEST_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_REQUEUED))
	{
		if(info->pCurReq->delayStartMs)
			bDelayMs = info->pCurReq->delayStartMs;
	}
	else
	{
		if(info->pCurReq->repeatIntermittentMs)
			bDelayMs = info->pCurReq->repeatIntermittentMs;
	}

	bForcePassive = info->pCurReq->bForcePassive;
	
	customscan_Unlock(info);

	if(bDelayMs)
		PlatformSleepUs(bDelayMs * 1000);

	customscan_InvokeMgntLinkReq(info, bForcePassive);

	FunctionOut(COMP_SCAN);
	
	return;
}

static
VOID
customscan_PreWorkerExitCb(
	IN  VOID 					*pCtx
	)
{
	RT_THREAD					*threadCtx = (RT_THREAD *)pCtx;
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)threadCtx->pContext;

	RT_ASSERT(pCtx, ("%s(): pCtx is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	info->bWorkerExited = TRUE;
	
	return;
}

//-----------------------------------------------------------------------------
// Driver callback
//-----------------------------------------------------------------------------

//
// Description:
//		This function handles reset request when scan in progress is not set.
//
VOID
CustomScan_ResetReqNoScanCb(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	ADAPTER						*pAdapter = info->pAdapter;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));
	
	if(MgntScanInProgress(&pAdapter->MgntInfo))
	{
		return;
	}

	customscan_Lock(info);

	// since we are doing reset, flush all queued reqs
	customscan_FlushReqQ(info);
	
	if(CUSTOM_SCAN_PROCESSING_REQ(info) 
		&& CUSTOM_SCAN_STATE_STARTING == info->pCurReq->state
		&& TEST_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_RESCHEDULED)
		)
	{
		// A rescheduled scan req still waiting for RF on, we free it here, otherwise:
		// 	* If there's a subsequent connect request, the return point would be changed to 
		//	  IPS_CALLBACK_JOIN_REQUEST and when subsequent scan is issued, CustomScan_IssueSysScan
		// 	  would think that the driver is doing custom scan and return media busy. In addition,
		//    JoinRequest may issue scan and that scan would be associated with the rescheduled 
		//    custom scan request and that is wrong.
		// *  If there's no connect request, the scan would continue when RF is on and that's not
		//    desiarble since we shall stop scan when receive a reset request.
		RT_TRACE(COMP_SCAN, DBG_WARNING, ("rescheduled scan %s got reset\n", info->pCurReq->typeInfo));
		customscan_Unlock(info);
		customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_PRE_DESTROY);
		customscan_Lock(info);
		customscan_FreeCurReq(info);
	}
	
	customscan_Unlock(info);
	
	return;
}

VOID
CustomScan_ScanResetCb(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));


	customscan_Lock(info);

	RT_TRACE(COMP_SCAN, DBG_WARNING, ("scan got reset\n"));

	customscan_DumpReqQ(info);
	if(CUSTOM_SCAN_PROCESSING_REQ(info))
	{
		RT_TRACE(COMP_SCAN, DBG_WARNING, ("cur req:\n"));
		customscan_DumpReq(info->pCurReq);
	}
	
	do
	{
		// since we are doing reset, flush all queued reqs
		customscan_FlushReqQ(info);

		// mark cur req as reset
		if(CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			SET_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_RESET);
			
			if(CUSTOM_SCAN_STATE_STARTING == info->pCurReq->state
				|| CUSTOM_SCAN_STATE_STARTED == info->pCurReq->state
				|| CUSTOM_SCAN_STATE_SW_CHNL == info->pCurReq->state
				|| CUSTOM_SCAN_STATE_DWELL == info->pCurReq->state
				)
			{// can't guarantee that ScanComplete for this req will be invoked
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("req %s got reset\n", info->pCurReq->typeInfo));
				customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_TERMINATING);
			}
		}		
	}while(FALSE);
	
	customscan_Unlock(info);
	
	return;
}

VOID
CustomScan_ScanByTimerCb(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));


	customscan_Lock(info);

	do
	{
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("pCurReq is NULL!!! return\n"));
			break;
		}

		if(CUSTOM_SCAN_STATE_STARTING == info->pCurReq->state)
		{
			customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_STARTED);
			SET_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_STARTED);
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("req issued: %s\n", info->pCurReq->typeInfo));
		}
	}while(FALSE);
	
	customscan_Unlock(info);
	
	return;
}

BOOLEAN
CustomScan_ConstructScanListCb(
	IN  VOID					*pInfo,
	IN  RT_CHANNEL_LIST			*pChnlList
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	u4Byte						itChnl = 0;
	RT_LIST_ENTRY				*pEntry = NULL;
	BOOLEAN						bSysScan = FALSE;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	// Not to acquire lock because
	// 	1. it is not necessary since constructing scan list is already an exclusive action
	// 	2. it may cause deadlock  
	//customscan_Lock(info);

	do
	{
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("pCurReq is NULL!!! return\n"));
			bSysScan = TRUE;
			break;
		}

		// Make a copy of the channel list
		info->scanChnlListLen = pChnlList->ScanChannelListLength;
		for(itChnl = 0; itChnl < pChnlList->ScanChannelListLength; itChnl++)
			info->scanChnlList[itChnl] = pChnlList->ScanChannelList[itChnl];

		// Reset driver channel list
		pChnlList->ScanChannelListLength = 0;

		// Reconstruct the channel list
		RtEntryListForEach(&info->pCurReq->chnlListQ, pEntry)
		{
			CHNL_ENTRY_EXT			*chnl = (CHNL_ENTRY_EXT *)pEntry;
			RT_CHNL_LIST_ENTRY		*chnlEntry = NULL;

			chnlEntry = customscan_GetScanChnlInDrvChnlList(info->scanChnlList, info->scanChnlListLen, chnl->super.ChannelNum);

			if(!chnlEntry)
				continue;

			// if client specifies mix, follow driver
			if(SCAN_MIX == chnl->super.ScanType)
				chnl->super.ScanType = chnlEntry->ScanType;

			// if driver specifies passive, we shall use passive
			if(SCAN_PASSIVE == chnlEntry->ScanType)
				chnl->super.ScanType = SCAN_PASSIVE;

			if(0 == chnl->super.ScanPeriod)
				chnl->super.ScanPeriod = chnlEntry->ScanPeriod;

			chnl->super.MaxTxPwrDbm = chnlEntry->MaxTxPwrDbm;
			chnl->super.ExInfo = chnlEntry->ExInfo;

			if(pChnlList->ScanChannelListLength < MAX_SCAN_CHANNEL_NUM)
			{
				pChnlList->ScanChannelList[pChnlList->ScanChannelListLength++] = &chnl->super;
			}
			else
			{
				RT_TRACE(COMP_SCAN, DBG_WARNING, ("unable to insert ch %u for scan\n", chnl->super.ChannelNum));
			}
		}

		bSysScan = (CUSTOM_SCAN_SRC_TYPE_SYS == info->pCurReq->type);
		
	}while(FALSE);

	//customscan_Unlock(info);

	return bSysScan;
}

u2Byte
CustomScan_PreSwChnlCb(
	IN  VOID					*pInfo,
	IN  RT_CHNL_LIST_ENTRY		*chnl
	)
{
	u2Byte						extDwellTime = 0;
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	customscan_Lock(info);
	
	do
	{
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("pCurReq is NULL!!! return\n"));
			break;
		}

		if(info->pCurReq->extDwellTime)
		{
			extDwellTime = info->pCurReq->extDwellTime;
			info->pCurReq->extDwellTime = 0;
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("dwell time extended to: %u\n", extDwellTime));
		}
		customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_SW_CHNL);

		RT_TRACE(COMP_SCAN, DBG_LOUD, 
		("%s: to sw from: %u to %u\n", 
		info->pCurReq->typeInfo,
			info->pCurReq->curChnl ? info->pCurReq->curChnl->super.ChannelNum : 0, 
			chnl->ChannelNum));

		info->pCurReq->curChnl = (CHNL_ENTRY_EXT *)CONTAINING_RECORD(chnl, CHNL_ENTRY_EXT, super);
		
	}while(FALSE);
	customscan_Unlock(info);

	return extDwellTime;
}

VOID
CustomScan_OnChnlCb(
	IN  VOID					*pInfo,
	IN  RT_CHNL_LIST_ENTRY		*chnl
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	customscan_Lock(info);	

	do
	{
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("pCurReq is NULL!!! return\n"));
			break;
		}

		RT_TRACE(COMP_SCAN, DBG_TRACE, ("on chnl for req %s: %u for %u ms\n", info->pCurReq->typeInfo, chnl->ChannelNum, chnl->ScanPeriod));

		customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_DWELL);

		if(info->pCurReq->curChnl)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("flag: %u\n", info->pCurReq->curChnl->flag));
		}
		
		if(CUSTOM_SCAN_TIME_DUE(info->pCurReq))
		{
			customscan_TerminateCurrentReq(info, FALSE);
		}
	}while(FALSE);
	
	customscan_Unlock(info);

	return;
}

BOOLEAN
CustomScan_PreSetDwellTimerCb(
	IN  VOID					*pInfo,
	IN  RT_CHNL_LIST_ENTRY		*chnl
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	BOOLEAN						bSetDwellTimer = TRUE;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	customscan_Lock(info);	

	do
	{
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("pCurReq is NULL!!! return\n"));
			break;
		}

		if(info->pCurReq->curChnl && TEST_FLAG(info->pCurReq->curChnl->flag, CHNL_ENTRY_EXT_FLAG_CHNL_SHUT))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("shutting cur chnl: %u, not to set dwell timer\n", info->pCurReq->curChnl->super.ChannelNum));
			bSetDwellTimer = FALSE;
		}

	}while(FALSE);
	
	customscan_Unlock(info);

	return bSetDwellTimer;
}

BOOLEAN
CustomScan_SendProbeCb(
	IN  VOID					*pInfo,
	IN  RT_CHNL_LIST_ENTRY		*chnl
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	CHNL_ENTRY_EXT				*chnlExt = NULL;
	ADAPTER						*pAdapter = NULL;

	RT_TCB						*pTcb = NULL;
	RT_TX_LOCAL_BUFFER 			*pBuf = NULL;

	BOOLEAN						bContinue = TRUE;
	BOOLEAN						bSendProbe = FALSE;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	customscan_Lock(info);

	do
	{
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			RT_TRACE(COMP_SCAN, DBG_TRACE, ("pCurReq is NULL!!! return\n"));
			break;
		}

		RT_ASSERT((VOID *)info->pCurReq->chnlEntryPool.start <= (VOID *)chnl 
			&& (VOID *)chnl < (VOID *)info->pCurReq->chnlEntryPool.end, 
			("%s(): chnl is not the customized one\n", __FUNCTION__));

		chnlExt = (CHNL_ENTRY_EXT *)CONTAINING_RECORD(chnl, CHNL_ENTRY_EXT, super);
		pAdapter = info->pAdapter;

		if(!chnlExt->probeReq) // didn't specify probe to send, use default
		{
			RT_TRACE(COMP_SCAN, DBG_TRACE, ("chnlExt->probeReq is NULL!!! return\n"));
			break;
		}

		bContinue = FALSE;

		if(0 == FrameBuf_Length(chnlExt->probeReq)) // probe specified but length is 0, to send nothing
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("0 == FrameBuf_Length(chnlExt->probeReq)!!! return\n"));
			break;
		}

		bSendProbe = TRUE;
	}while(FALSE);

	customscan_Unlock(info);

	if(bSendProbe)
	{// send probe w/o having custom scan lock acquired to prevent deadlock
		PlatformAcquireSpinLock(info->pAdapter, RT_TX_SPINLOCK);

		if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
		{
			PlatformMoveMemory(pBuf->Buffer.VirtualAddress, 
				FrameBuf_MHead(chnlExt->probeReq), 
				FrameBuf_Length(chnlExt->probeReq));
			pTcb->PacketLength = FrameBuf_Length(chnlExt->probeReq);
				
			MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, chnlExt->dataRate);
		}

		PlatformReleaseSpinLock(info->pAdapter, RT_TX_SPINLOCK);	
	}
	
	return bContinue;
}

BOOLEAN
CustomScan_DualBandScanCb(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	BOOLEAN						terminated = FALSE;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));


	customscan_Lock(info);

	do{
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("pCurReq is NULL!!! return\n"));
			break;
		}

		RT_TRACE(COMP_SCAN, DBG_LOUD, ("state: %s\n", customscan_ScanStateTxt(info->pCurReq->state)));
		
		if(CUSTOM_SCAN_STATE_TERMINATING == info->pCurReq->state)
		{
			RT_CHNL_LIST_ENTRY 		*pChnlEntry = NULL;		
			
			while(RtActChannelList(info->pAdapter, RT_CHNL_LIST_ACTION_POP_SCAN_CHANNEL, NULL, &pChnlEntry)){}
			terminated = TRUE;
		}

		if(CUSTOM_SCAN_TIME_DUE(info->pCurReq))
		{
			customscan_TerminateCurrentReq(info, FALSE);
		}
	}while(FALSE);
	
	customscan_Unlock(info);

	return terminated;
}

VOID
CustomScan_PreScanCompleteCb(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("\n"));

	if(!CUSTOM_SCAN_PROCESSING_REQ(info))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("pCurReq is NULL!!! return\n"));
		return;
	}

	return;
}

VOID
CustomScan_ScanCompleteCb(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("\n"));

	customscan_Lock(info);

	if(CUSTOM_SCAN_PROCESSING_REQ(info))
	{// Note that scan spinlock is acquired
		if(info->pCurReq->repeatCount)
		{
			info->pCurReq->repeatCount--;

			RT_TRACE(COMP_SCAN, DBG_LOUD, 
				("repeatCount: %u, state: %s\n", 
				info->pCurReq->repeatCount, customscan_ScanStateTxt(info->pCurReq->state)));
			
			if(info->pCurReq->repeatCount
				&& CUSTOM_SCAN_STATE_TERMINATING != info->pCurReq->state
				)
			{
				customscan_RequeueCurReq(info);
			}
			else
			{
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("sending completed event to: %s\n", info->pCurReq->typeInfo));
				customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_COMPLETED);
			}
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("scan complete: sw back to chnl: %u\n", info->pAdapter->MgntInfo.SettingBeforeScan.ChannelNumber));
		}
		else
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, 
				("current req's repeat count reaches 0 already and it is not freed and scan complete is invoked again\n"));
			SET_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_POTENTIAL_REPEATE_COUNT_UNDERFLOW);
		}
	}

	customscan_Unlock(info);
	
	return;
}

VOID
CustomScan_ScanCompleteReturnCb(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	BOOLEAN						bProcessingReq = FALSE;
	CUSTOM_SCAN_REQ				*curReq = NULL;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	FunctionIn(COMP_SCAN);
	
	customscan_Lock(info);
	if(CUSTOM_SCAN_PROCESSING_REQ(info))
	{
		bProcessingReq = TRUE;
		curReq = info->pCurReq;
		SET_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_TO_BE_FREED);
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("going to free: %s\n", curReq->typeInfo));
	}
	customscan_Unlock(info);

	// Note that pCurReq may be changed by the worker after unlock,
	// this happens when direct OID is used

	if(bProcessingReq)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("sending pre-destroy event to: %s\n", curReq->typeInfo));
		customscan_SetReqState(info, curReq, CUSTOM_SCAN_STATE_PRE_DESTROY);
	}

	customscan_Lock(info);

	if(bProcessingReq)
	{
		if(info->pCurReq == curReq)
			customscan_FreeCurReq(info);
		else
			customscan_FreeReq(info, curReq);
	}

	// Trigger thread when ScanComplete has done everything it needs to do
	if(info->reqCnt)
	{
		PlatformSetEventToTrigerThread(info->pAdapter, &info->worker);	
	}

	customscan_Unlock(info);
	
	FunctionOut(COMP_SCAN);
	
	return;
}

VOID
CustomScan_RescheduleCb(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	customscan_Lock(info);
	
	do
	{
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("pCurReq is NULL!!! return\n"));
			break;
		}

		RT_TRACE(COMP_SCAN, DBG_LOUD, ("\n"));


		SET_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_RESCHEDULED);
	}while(FALSE);
	customscan_Unlock(info);
	
	return;
}

VOID
CustomScan_WatchDogCb(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	if(NULL == info || info_sig != info->sig)
	{// not initialized yet
		return;
	}
	
	if(info->bWorkerExited)
	{
		info->bWorkerExited = FALSE;

		// Note that this shall be done in passive level
		PlatformSetEventTrigerThread(info->pAdapter, &info->worker, PASSIVE_LEVEL, pInfo);
	}
}

VOID
CustomScan_OnProbeRspCb(
	IN  VOID					*pInfo,
	IN	RT_RFD					*pRfd
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	customscan_Lock(info);

	if(CUSTOM_SCAN_PROCESSING_REQ(info))
	{
		if(info->pCurReq->onProbeRspCb)
			info->pCurReq->onProbeRspCb(
				info->pCurReq->state,
				info->pCurReq->onProbeRspCtx,
				pRfd
				);
	}

	customscan_Unlock(info);
	
	return;
}

//-----------------------------------------------------------------------------
// Public
//-----------------------------------------------------------------------------
VOID *
CustomScan_AllocInfo(
	IN  ADAPTER					*pAdapter
	)
{
	CUSTOM_SCAN_INFO			*info = NULL;
	MGNT_INFO					*pMgntInfo = NULL;

	RT_ASSERT(pAdapter, ("%s(): pAdapter is NULL!!!\n", __FUNCTION__));

	FunctionIn(COMP_SCAN);

	pMgntInfo = &pAdapter->MgntInfo;
	pMgntInfo->pCustomScanInfo = NULL;
	
	if(RT_STATUS_SUCCESS != PlatformAllocateMemory(pAdapter, (PVOID *)&info, sizeof(*info)))
	{
		return NULL;
	}

	pMgntInfo->pCustomScanInfo = info;

	return info;
}

VOID
CustomScan_FreeInfo(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	PlatformFreeMemory(info, sizeof(*info));

	return;
}

BOOLEAN
CustomScan_InProgress(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		return FALSE;

	if(CUSTOM_SCAN_PROCESSING_REQ(info) && CUSTOM_SCAN_SRC_TYPE_SYS != info->pCurReq->type)
		return TRUE;

	return FALSE;
}

VOID
CustomScan_Init(
	IN  VOID					*pInfo,
	IN  ADAPTER					*pScanAdapter,
	IN  u8Byte					dbgComp,
	IN  u4Byte					dbgLevel
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(pScanAdapter, ("%s(): pScanAdapter is NULL!!!\n", __FUNCTION__));

	FunctionIn(COMP_SCAN);

	PlatformZeroMemory(info, sizeof(*info));
 
	info->sig = info_sig;
	info->pAdapter = pScanAdapter;
	RTInitializeListHead(&info->reqQ);
	info->reqCnt = 0;
	Pool_Init(&info->reqPool, "req pool", sizeof(info->reqPoolRsvd), 
		info->reqPoolRsvd, sizeof(info->reqPoolRsvd[0]), 
		dbgComp, dbgLevel);
	info->pCurReq = NULL;
	info->dbgComp = dbgComp;
	info->dbgLevel = dbgLevel;

	PlatformInitializeSpinLock(pScanAdapter, RT_CUSTOM_SCAN_SPINLOCK);

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("ctx: %p\n", pInfo));

	info->bWorkerExited = FALSE;

	FunctionOut(COMP_SCAN);

	return;
}

VOID
CustomScan_Start(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	ADAPTER						*pScanAdapter = info->pAdapter;
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));

	FunctionIn(COMP_SCAN);

	PlatformInitializeThreadEx(pScanAdapter, &info->worker, customscan_WorkerCb, customscan_PreWorkerExitCb, "custom scan worker thread", TRUE, 1000, pInfo);
	PlatformSetEventTrigerThread(pScanAdapter, &info->worker, PASSIVE_LEVEL, pInfo);
	
	FunctionOut(COMP_SCAN);

	return;
}

VOID
CustomScan_Stop(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));

	FunctionIn(COMP_SCAN);

	// Free all requests in the queue
	// Note that at this time, scan spinlock has been freed already
	customscan_Lock(info);
	customscan_FlushReqQ(info);
	customscan_Unlock(info);

	// Terminate the worker thread
	PlatformWaitThreadEnd(info->pAdapter, &info->worker);
	PlatformCancelThread(info->pAdapter, &info->worker);
	PlatformReleaseThread(info->pAdapter, &info->worker);

	FunctionOut(COMP_SCAN);

	return;
}

VOID
CustomScan_Deinit(
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	PlatformFreeSpinLock(info->pAdapter, RT_CUSTOM_SCAN_SPINLOCK);

	return;
}

VOID *
CustomScan_AllocReq(
	IN  VOID					*pInfo,
	IN  CUSTOM_SCAN_STATE_CB	cb,
	IN  VOID					*ctx
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	CUSTOM_SCAN_REQ				*req = NULL;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	customscan_Lock(info);

	do
	{
		if(NULL == (req = (CUSTOM_SCAN_REQ *)Pool_Acquire(&info->reqPool)))
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, ("Failed to acquire req from pool\n"));
			customscan_DumpReqPool(info);
			break;
		}

		customscan_InitReq(info, req);
	}while(FALSE);

	customscan_Unlock(info);

	return req;
}

FRAME_BUF *
CustomScan_GetProbeReqBuf(
	IN  VOID					*scanReq
	)
{
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;
	
	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));
	RT_ASSERT(CUSTOM_SCAN_STATE_IDLE == req->state, ("%s(): invalid req state: %s\n", __FUNCTION__, customscan_ScanStateTxt(req->state)));

	return &req->probeReqBuf;
}

VOID
CustomScan_SetupCbCtx(
	IN  VOID					*scanReq,
	IN  CUSTOM_SCAN_STATE_CB	cb,
	IN  VOID					*ctx
	)
{
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;

	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));
	RT_ASSERT(CUSTOM_SCAN_STATE_IDLE == req->state, ("%s(): invalid req state: %s\n", __FUNCTION__, customscan_ScanStateTxt(req->state)));

	req->cb = cb;
	req->cbCtx = ctx;
	
	return;
}

RT_STATUS
CustomScan_TermReq(
	IN  VOID					*pInfo,
	IN  BOOLEAN					bStopOnCurChnl
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	customscan_Lock(info);

	RT_TRACE(COMP_SCAN, DBG_WARNING, ("to terminate scan\n"));
	customscan_DumpReqQ(info);
	if(CUSTOM_SCAN_PROCESSING_REQ(info))
	{
		RT_TRACE(COMP_SCAN, DBG_WARNING, ("cur req:\n"));
		customscan_DumpReq(info->pCurReq);
	}

	do
	{
		customscan_FlushReqQ(info);
		
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
			break;

		info->pAdapter->MgntInfo.bCompleteScan = TRUE;
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("CustomScan_TermReq(): info bCompleteScan %d\n", info->pAdapter->MgntInfo.bCompleteScan));
				
		if(RT_STATUS_SUCCESS != (status = customscan_TerminateCurrentReq(info, bStopOnCurChnl)))
			break;
		
	}while(FALSE);
	
	customscan_Unlock(info);

	return status;
}

RT_STATUS
CustomScan_AddScanChnl(
	IN  VOID					*scanReq,
	IN  u1Byte					chnl,
	IN  u1Byte					count,
	IN  RT_SCAN_TYPE			scanType,
	IN  u2Byte					duration,
	IN  u1Byte					dataRate,
	IN  FRAME_BUF				*probeReqBuf
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;
	RT_CHNL_LIST_ENTRY			chnlEntry;
	u4Byte						itChnl = 0;
	
	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));
	RT_ASSERT(CUSTOM_SCAN_STATE_IDLE == req->state, ("%s(): invalid req state: %s\n", __FUNCTION__, customscan_ScanStateTxt(req->state)));

	if(MAX_SCAN_CHANNEL_NUM <= req->chnlListCnt)
	{
		RT_TRACE(COMP_SCAN, DBG_WARNING, ("chnlListCnt (%u) exceeds MAX_SCAN_CHANNEL_NUM\n", req->chnlListCnt));
		return RT_STATUS_BUFFER_TOO_SHORT;
	}

	chnlEntry.ChannelNum = chnl;
	chnlEntry.ScanType = scanType;
	chnlEntry.ScanPeriod = duration;
	chnlEntry.MaxTxPwrDbm = UNSPECIFIED_PWR_DBM;
	chnlEntry.ExInfo = 0;

	for(itChnl = 0; itChnl < count; itChnl++)
	{
		CHNL_ENTRY_EXT			*pExt = (CHNL_ENTRY_EXT *)Pool_Acquire(&req->chnlEntryPool);
		
		if(!pExt)
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, ("Failed to acquire chnl ext from pool\n"));
			status = RT_STATUS_RESOURCE;
			break;
		}

		customscan_InitChnlEntryExt(pExt, &chnlEntry, probeReqBuf, dataRate);
		RTInsertTailListWithCnt(&req->chnlListQ, &pExt->list, &req->chnlListCnt);
	}

	return status;
}

RT_STATUS
CustomScan_AddChnlPlanChnls(
	IN  VOID					*scanReq,
	IN  const RT_CHANNEL_PLAN	*plan,
	IN  RT_SCAN_TYPE			scanType,
	IN  u2Byte					duration,
	IN  u1Byte					dataRate,
	IN  FRAME_BUF				*probeReqBuf
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	u4Byte						itChnl = 0;

	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(plan, ("%s(): plan is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(plan->Len || plan->Len2_4G || plan->Len5G, ("%s(): plan->Len and plan->Len2_4G and plan->Len5G are all 0\n", __FUNCTION__));

	if(plan->Len)
	{// old definition
		// add all channels
		for(itChnl = 0; itChnl < plan->Len; itChnl++)
		{
			CustomScan_AddScanChnl(
				scanReq,
				plan->Channel[itChnl],
				1,
				scanType,
				duration,
				dataRate,
				probeReqBuf
				);
		}	
	}
	else
	{// new definition, see DefaultChannelPlan
		// add 2.4g channels
		for(itChnl = 0; itChnl < plan->Len2_4G; itChnl++)
		{
			CustomScan_AddScanChnl(
				scanReq,
				plan->Channel2_4G[itChnl],
				1,
				scanType,
				duration,
				dataRate,
				probeReqBuf
				);
		}

		// add 5g channels
		for(itChnl = 0; itChnl < plan->Len5G; itChnl++)
		{
			CustomScan_AddScanChnl(
				scanReq,
				plan->Channel5G[itChnl],
				1,
				scanType,
				duration,
				dataRate,
				probeReqBuf
				);
		}	

	}

	return status;
}

RT_STATUS
CustomScan_SetTimeBound(
	IN  VOID					*scanReq,
	IN  u4Byte					timeBound
	)
{
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;

	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));

	if(CUSTOM_SCAN_STATE_IDLE != req->state)
	{
		return RT_STATUS_INVALID_STATE;
	}

	req->timeBound = timeBound;

	return RT_STATUS_SUCCESS;
}

RT_STATUS
CustomScan_ForcePassiveScan(
	IN  VOID					*scanReq
	)
{
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;

	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));

	if(CUSTOM_SCAN_STATE_IDLE != req->state)
	{
		return RT_STATUS_INVALID_STATE;
	}

	req->bForcePassive = TRUE;

	return RT_STATUS_SUCCESS;
}

RT_STATUS
CustomScan_SetDelayStart(
	IN  VOID					*scanReq,
	IN  u4Byte					delayMs
	)
{
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;

	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));

	if(CUSTOM_SCAN_STATE_IDLE != req->state)
	{
		return RT_STATUS_INVALID_STATE;
	}

	req->delayStartMs = delayMs;

	return RT_STATUS_SUCCESS;
}

RT_STATUS
CustomScan_SetRepeatIntermittent(
	IN  VOID					*scanReq,
	IN  u4Byte					repeatIntermittentMs
	)
{
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;

	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));

	if(CUSTOM_SCAN_STATE_IDLE != req->state)
	{
		return RT_STATUS_INVALID_STATE;
	}

	req->repeatIntermittentMs = repeatIntermittentMs;

	return RT_STATUS_SUCCESS;
}


RT_STATUS
CustomScan_SetRepeatCount(
	IN  VOID					*scanReq,
	IN  u4Byte					count
	)
{
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;

	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));

	if(CUSTOM_SCAN_STATE_IDLE != req->state)
	{
		return RT_STATUS_INVALID_STATE;
	}

	req->repeatCount = count;

	return RT_STATUS_SUCCESS;
}

RT_STATUS
CustomScan_SetProbeRspCb(
	IN  VOID					*scanReq,
	IN  CUSTOM_SCAN_ON_PROBE_RSP_CB onProbeRspCb,
	IN  VOID					*ctx
	)
{
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;

	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));

	if(CUSTOM_SCAN_STATE_IDLE != req->state)
	{
		return RT_STATUS_INVALID_STATE;
	}

	req->onProbeRspCb = onProbeRspCb;
	req->onProbeRspCtx = ctx;

	return RT_STATUS_SUCCESS;
}

u4Byte
CustomScan_NumAddedChnl(
	IN  VOID					*scanReq
	)
{
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;
	
	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));

	return req->chnlListCnt;
}

RT_STATUS
CustomScan_IssueReq(
	IN  VOID					*pInfo,
	IN  VOID					*scanReq,
	IN  CUSTOM_SCAN_SRC_TYPE	type,
	IN  const char				*typeInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));
	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));
	RT_ASSERT(CUSTOM_SCAN_STATE_IDLE == req->state, ("%s(): invalid req state: %s\n", __FUNCTION__, customscan_ScanStateTxt(req->state)));
	RT_ASSERT(typeInfo, ("%s(): typeInfo is NULL!!!\n", __FUNCTION__));
	
	FunctionIn(COMP_SCAN);

	req->type = type;
	req->typeInfo = typeInfo;
	req->issueTime = PlatformGetCurrentTime() / 1000;

	if(0 == req->repeatCount)
	{
		customscan_Lock(info);
		customscan_FreeReq(info, req);
		customscan_Unlock(info);
		return RT_STATUS_SUCCESS;
	}
	
	customscan_Lock(info);
	if(CUSTOM_SCAN_SRC_TYPE_SYS == type)
	{// give system scan the highest priority
		RTInsertHeadListWithCnt(&info->reqQ, &req->list, &info->reqCnt);
	}
	else
	{
		RTInsertTailListWithCnt(&info->reqQ, &req->list, &info->reqCnt);
	}
	customscan_SetReqState(info, req, CUSTOM_SCAN_STATE_WAITING);

	customscan_DumpReq(req);
	
	customscan_Unlock(info);

	if(RT_DRIVER_STOP(info->pAdapter))
	{
		// In this case, the request has been inserted to the reqQ and will be freed automatically in CustomScan_Deinit
		return RT_STATUS_INVALID_STATE;
	}


	PlatformSetEventToTrigerThread(info->pAdapter, &info->worker);
	
	return RT_STATUS_SUCCESS;
}

RT_STATUS
CustomScan_IssueSysScan(
	IN  VOID					*pInfo,
	IN  BOOLEAN					bActiveScan
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	CUSTOM_SCAN_REQ				*req = NULL;
	RT_CHANNEL_PLAN				*plan = NULL;
	
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	FunctionIn(COMP_SCAN);

	do
	{
		if(MgntScanInProgress(&info->pAdapter->MgntInfo))
		{
			status = RT_STATUS_MEDIA_BUSY;
			break;
		}
		
		if(NULL == (req = (CUSTOM_SCAN_REQ *)CustomScan_AllocReq(pInfo, NULL, NULL)))
		{
			status = RT_STATUS_RESOURCE;
			break;
		}

		if(CUSTOM_SCAN_PROCESSING_REQ(info) && TEST_FLAG(info->pCurReq->flag, CUSTOM_SCAN_REQ_FLAG_RESET))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("%s(): req %s is in terminating state and reset flag set, free it\n", __FUNCTION__, info->pCurReq->typeInfo));
			customscan_Lock(info);
			customscan_FreeDanglingReq(info);
			customscan_Unlock(info);
		}

		// set to cur req ASAP
		if(!CUSTOM_SCAN_PROCESSING_REQ(info))
		{
			customscan_Lock(info);
			info->pCurReq = req;
			customscan_Unlock(info);
		}
		else
		{
			status = RT_STATUS_MEDIA_BUSY;
			break;
		}

		// prepare chnls
		RtActChannelList(info->pAdapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_PLAN, &info->pAdapter->MgntInfo.ChannelPlan, &plan);
		CustomScan_AddChnlPlanChnls(req, plan, bActiveScan ? SCAN_ACTIVE : SCAN_PASSIVE, 0, 0, NULL);

		if(!bActiveScan)
		{
			CustomScan_ForcePassiveScan(req);
		}

		// store info
		req->type = CUSTOM_SCAN_SRC_TYPE_SYS;
		req->typeInfo = "sys";
		req->issueTime = PlatformGetCurrentTime() / 1000;
		
		// set state
		customscan_Lock(info);
		customscan_SetReqState(info, info->pCurReq, CUSTOM_SCAN_STATE_STARTING);
		customscan_Unlock(info);
		
		// issue scan
		if(!customscan_InvokeMgntLinkReq(info, !bActiveScan))
		{
			status = RT_STATUS_FAILURE;
			break;
		}
	}while(FALSE);

	// cleanup
	if(req && RT_STATUS_SUCCESS != status)
	{
		customscan_Lock(info);
		if(req == info->pCurReq)
			info->pCurReq = NULL;
		
		customscan_FreeReq(info, req);

		customscan_Unlock(info);
	}

	FunctionOut(COMP_SCAN);

	return status;
}

RT_STATUS
CustomScan_ExtendDwellTime(
	IN  VOID					*pInfo,
	IN  u2Byte					dwellTime
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	customscan_Lock(info);

	do
	{
		if(!CustomScan_InProgress(pInfo))
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, ("no custom scan in progress\n"));
			status = RT_STATUS_INVALID_STATE;
			break;
		}
		
		if(CUSTOM_SCAN_STATE_DWELL != info->pCurReq->state)
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, 
				("Failed to extend dwell time: state is: %s\n", customscan_ScanStateTxt(info->pCurReq->state)));
			status = RT_STATUS_INVALID_STATE;
			break;
		}
		
		info->pCurReq->extDwellTime = dwellTime;
	}while(FALSE);

	customscan_Unlock(info);
	
	return status;

}

RT_STATUS
CustomScan_ShutDwellTime(
	IN  VOID					*pInfo,
	IN  VOID					*req
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	CUSTOM_SCAN_REQ				*reqIn = (CUSTOM_SCAN_REQ *)req;

	FunctionIn(COMP_SCAN);
	
	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));

	do
	{
		if(!CustomScan_InProgress(pInfo))
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, ("no custom scan in progress\n"));
			status = RT_STATUS_INVALID_STATE;
			break;
		}

		if(reqIn != info->pCurReq)
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, ("not current req\n"));
			status = RT_STATUS_INVALID_STATE;
			break;
		}
		
		if(CUSTOM_SCAN_STATE_DWELL != reqIn->state)
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, 
				("Failed to shut dwell time: state is: %s\n", customscan_ScanStateTxt(reqIn->state)));
			status = RT_STATUS_INVALID_STATE;
			break;
		}
		
		RT_TRACE(COMP_SCAN, DBG_WARNING, 
				("shutting off scan req %s dwell time\n", reqIn->typeInfo));
		
		if(RT_STATUS_SUCCESS != (status = customscan_TerminateCurrentChnl(info)))
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, 
				("customscan_TerminateCurrentChnl failed: 0x%08X\n", status));
			break;
		}

		if(reqIn->curChnl)
		{
			SET_FLAG(reqIn->curChnl->flag, CHNL_ENTRY_EXT_FLAG_CHNL_SHUT);
		}
	}while(FALSE);

	FunctionOut(COMP_SCAN);
	
	return status;

}

const char *
CustomScan_ScanStateTxt(
	IN  CUSTOM_SCAN_STATE		state
	)
{
	return customscan_ScanStateTxt(state);
}

BOOLEAN
CustomScan_AcquireCurCtx(
	IN  VOID					*pInfo,
	IN  VOID					*scanReq,
	OUT VOID					**ppCtx
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;
	CUSTOM_SCAN_REQ				*req = (CUSTOM_SCAN_REQ *)scanReq;
	BOOLEAN						bCurReq = FALSE;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));
	RT_ASSERT(scanReq, ("%s(): scanReq is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(req_sig == req->sig, ("%s(): invalid scanReq\n", __FUNCTION__));

	customscan_Lock(info);

	do
	{
		if(req != info->pCurReq)
		{
			break;
		}

		bCurReq = TRUE;
		*ppCtx = req->cbCtx;
	}while(FALSE);

	if(!bCurReq)
	{
		customscan_Unlock(info);
	}

	return bCurReq;
}

VOID
CustomScan_ReleaseCurCtx(	
	IN  VOID					*pInfo
	)
{
	CUSTOM_SCAN_INFO			*info = (CUSTOM_SCAN_INFO *)pInfo;

	RT_ASSERT(pInfo, ("%s(): pInfo is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(info_sig == info->sig, ("%s(): invalid info\n", __FUNCTION__));
	
	customscan_Unlock(info);
}

