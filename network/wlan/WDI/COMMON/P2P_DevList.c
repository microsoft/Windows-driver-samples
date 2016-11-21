#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_DevList.tmh"
#endif

#include "P2P_DevList.h"
#include "P2P_Internal.h"

#if (P2P_SUPPORT == 1)

static const char *signature = "P2P_DevList_Signature";

#define P2P_DEV_LIST_DEV_ENTRY_LIFETIME_MS (10 * 60 * 1000)

//-----------------------------------------------------------------------------
// Foreward declaration
//-----------------------------------------------------------------------------
static
VOID
p2p_devlist_FreeDev(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_DEV_LIST_ENTRY		*pDev
	);
	
static
const char *
p2p_devlist_DevTypeStr(
	IN  P2P_DEV_TYPE			type	
	);

static
u8Byte
p2p_devlist_RxFrameElapsedTimeMs(
	IN  const P2P_FRAME_INFO 	*finfo
	);

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

static
u4Byte
p2p_devlist_RemoveDatedDev(
	IN  P2P_DEV_LIST			*dlist
	)
{
	RT_LIST_ENTRY				*pEntry, *pn = NULL;
	u4Byte						nFreedDev = 0;

	RtEntryListForEachSafe(&dlist->list, pEntry, pn)
	{
		P2P_DEV_LIST_ENTRY		*pDev = (P2P_DEV_LIST_ENTRY *)pEntry;

		if(RTIsListNotEmpty(&pDev->rxFrameQ))
		{
			const P2P_FRAME_INFO 	*lastRxFrame = (P2P_FRAME_INFO *)RTGetTailList(&pDev->rxFrameQ);
			u8Byte					lastRxElapsed = p2p_devlist_RxFrameElapsedTimeMs(lastRxFrame);

			if(lastRxElapsed <= P2P_DEV_LIST_DEV_ENTRY_LIFETIME_MS)
				continue;

			//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Freeing "MACSTR", type: %s for no rx for %llu ms\n",
				//MAC2STR(pDev->mac), p2p_devlist_DevTypeStr(pDev->type), lastRxElapsed));
			
			p2p_devlist_FreeDev(dlist, pDev);
			nFreedDev++;
		}
	}

	return nFreedDev;
}

static
u4Byte
p2p_devlist_RemoveProbeReqOnlyDev(
	IN  P2P_DEV_LIST			*dlist
	)
{
	RT_LIST_ENTRY				*pEntry, *pn = NULL;
	u4Byte						nFreedDev = 0;
	
	RtEntryListForEachSafe(&dlist->list, pEntry, pn)
	{
		P2P_DEV_LIST_ENTRY		*pDev = (P2P_DEV_LIST_ENTRY *)pEntry;
		P2P_FRAME_INFO			*pFrame = NULL;
		u4Byte					rxFrames = 0;

		RtEntryListForEach(&pDev->rxFrameQ, pEntry)
		{
			rxFrames++;
		}

		if(1 == rxFrames && pDev->rxFrames[P2P_FID_PROBE_REQ])
		{
			//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Freeing "MACSTR", type: %s for we've rx only probe req from it\n",
				//MAC2STR(pDev->mac), p2p_devlist_DevTypeStr(pDev->type)));
			
			p2p_devlist_FreeDev(dlist, pDev);
			nFreedDev++;
		}
	}

	return nFreedDev;
}

static
u4Byte
p2p_devlist_AgeFunction(
	IN  P2P_DEV_LIST			*dlist
	)
{
	u4Byte						nFreedDev = 0;
	
	nFreedDev += p2p_devlist_RemoveDatedDev(dlist);
	nFreedDev += p2p_devlist_RemoveProbeReqOnlyDev(dlist);

	return nFreedDev;
}
	
static
P2P_DEV_LIST_ENTRY *
p2p_devlist_NewDev(
	IN  P2P_DEV_LIST			*dlist,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE 			type
	)
{
	P2P_DEV_LIST_ENTRY			*pDev = NULL;
	P2P_DEV_INFO				*pInfo = NULL;
	
	if(NULL == (pDev = (P2P_DEV_LIST_ENTRY *)Pool_Acquire(&dlist->entryPool)))
	{
		if(!p2p_devlist_AgeFunction(dlist))
		{
			return NULL;
		}
		
		pDev = (P2P_DEV_LIST_ENTRY *)Pool_Acquire(&dlist->entryPool);
	}

	if(P2P_DEV_TYPE_LEGACY != type)
	{
		if(NULL == (pInfo = (P2P_DEV_INFO *)Pool_Acquire(&dlist->p2pDevInfoPool)))
		{
			Pool_Release(&dlist->entryPool, pDev);
			return NULL;
		}

		PlatformZeroMemory(pInfo, sizeof(*pInfo));
	}

	PlatformZeroMemory(pDev, sizeof(*pDev));

	RTInitializeListHead(&pDev->rxFrameQ);
	cpMacAddr(pDev->mac, mac);
	pDev->type = type;
	pDev->p2p = pInfo;

	RTInsertTailListWithCnt(&dlist->list, &pDev->list, &dlist->count);

	return pDev;
}

static
VOID
p2p_devlist_FreeRxFrames(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_DEV_LIST_ENTRY		*pDev,
	IN  BOOLEAN					bActionOnly
	)
{
	u4Byte						it = 0;
	u4Byte						start = bActionOnly ? P2P_FID_ACTION_START : 0;
	
	for(it = start; it < P2P_FID_MAX; it++)
	{
		if(pDev->rxFrames[it])
		{
			RTRemoveEntryList(&pDev->rxFrames[it]->list);
			if(pDev->rxFrames[it]->msg)
			{
				p2p_parse_FreeMessage(pDev->rxFrames[it]->msg);
				Pool_Release(&dlist->rxMsgPool, pDev->rxFrames[it]->msg);
				pDev->rxFrames[it]->msg = NULL;
			}
			Pool_Release(&dlist->framePool, pDev->rxFrames[it]);
			pDev->rxFrames[it] = NULL;
		}
	}

	return;
}

static
VOID
p2p_devlist_FreeTxFrames(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_DEV_LIST_ENTRY		*pDev,
	IN  BOOLEAN					bActionOnly
	)
{
	u4Byte						it = 0;
	u4Byte						start = bActionOnly ? P2P_FID_ACTION_START : 0;
	
	for(it = start; it < P2P_FID_MAX; it++)
	{
		if(pDev->txFrames[it])
		{
			Pool_Release(&dlist->framePool, pDev->txFrames[it]);
			pDev->txFrames[it] = NULL;
		}
	}

	return;
}

static
VOID
p2p_devlist_FreeDev(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_DEV_LIST_ENTRY		*pDev
	)
{
	RTRemoveEntryList(&pDev->list);

	p2p_devlist_FreeRxFrames(dlist, pDev, FALSE);
	p2p_devlist_FreeTxFrames(dlist, pDev, FALSE);

	if(pDev->p2p)
	{
		Pool_Release(&dlist->p2pDevInfoPool, pDev->p2p);
		pDev->p2p = NULL;
	}

	PlatformZeroMemory(pDev, sizeof(*pDev));

	Pool_Release(&dlist->entryPool, pDev);

	return;
}

static
RT_STATUS
p2p_devlist_ParseFrame(
	IN  P2P_FRAME_TYPE			frameType,
	IN  u2Byte					frameLen,
	IN  u1Byte					*frame,
	OUT P2P_MESSAGE				*msg
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	const OCTET_STRING			mpdu = {frame, frameLen};
	
	if(P2P_FID_ACTION_START <= frameType)
		status = p2p_parse_Action(&mpdu, DBG_LOUD, msg);
	else
		status = p2p_parse_Ies(&mpdu, DBG_TRACE, msg);

	return status;
}

static
RT_STATUS
p2p_devlist_UpdateFrameInfo(
	IN  P2P_DEV_LIST	*dlist,
	IN  u2Byte			frameLen,
	IN  const u1Byte	*frame,
	IN  P2P_FRAME_TYPE	type,
	IN  u1Byte			token,
	IN  P2P_MESSAGE		*msg,
	IN  u8Byte			time,
	IN  u1Byte			channel,
	IN  s4Byte			sigPower,
	IN  u1Byte			sigStrength,
	OUT P2P_FRAME_INFO	*finfo
	)
{
	if(sizeof(finfo->frame) < frameLen)
	{
		// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
		RT_TRACE_F(COMP_P2P, DBG_WARNING, 
			("Unable to copy frame, frame size: %u, buf size: %u\n", 
			frameLen, sizeof(finfo->frame)));
			
		return RT_STATUS_BUFFER_TOO_SHORT;
	}
		
	p2p_MoveMemory(finfo->frame, frame, frameLen);
	finfo->frameLen = frameLen;
	finfo->type = type;
	finfo->token = token;
	if(msg)
	{
		if(finfo->msg)
		{
			p2p_parse_FreeMessage(finfo->msg);
			Pool_Release(&dlist->rxMsgPool, finfo->msg);
		}
		finfo->msg = msg;
	}
	finfo->time = time;
	finfo->channel = channel;
	finfo->sigPower = sigPower;
	finfo->sigStrength = sigStrength;

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_devlist_RxUpdateFrameInfo(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_DEV_LIST_ENTRY		*pEntry,
	IN  const RT_RFD			*rfd,
	IN  u1Byte					rxChnl,
	IN  P2P_FRAME_TYPE			type,
	IN  P2P_MESSAGE				*msg
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	
	if(!pEntry->rxFrames[type])
	{
		if(!(pEntry->rxFrames[type] = Pool_Acquire(&dlist->framePool)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Unable to acquire from frame pool\n"));
			return RT_STATUS_RESOURCE; 
		}
	}
	
	status = p2p_devlist_UpdateFrameInfo(dlist,
		rfd->PacketLength, 
		rfd->Buffer.VirtualAddress, 
		type, 
		msg->dialogToken, 
		msg,
		PlatformGetCurrentTime(), 
		(msg->_dsParam) ? (msg->dsParam) : (rxChnl), 
		rfd->Status.RecvSignalPower, 
		rfd->Status.SignalStrength, 
		pEntry->rxFrames[type]);

	if(RT_STATUS_SUCCESS != status)
	{
		Pool_Release(&dlist->framePool, pEntry->rxFrames[type]);
		pEntry->rxFrames[type] = NULL;
		return status;
	}

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_devlist_TxUpdateFrameInfo(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_DEV_LIST_ENTRY		*pEntry,
	IN  const FRAME_BUF			*buf,
	IN  P2P_FRAME_TYPE			type,
	IN  u1Byte					token,
	IN  u1Byte					channel
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;

	if(!pEntry->txFrames[type])
	{
		if(!(pEntry->txFrames[type] = Pool_Acquire(&dlist->framePool)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Unable to acquire from frame pool\n"));
			return RT_STATUS_RESOURCE; 
		}
	}

	status = p2p_devlist_UpdateFrameInfo(dlist,
		FrameBuf_Length(buf), 
		FrameBuf_Head(buf), 
		type, 
		token, 
		NULL,
		PlatformGetCurrentTime(), 
		channel, 
		0, 
		0, 
		pEntry->txFrames[type]);

	if(RT_STATUS_SUCCESS != status)
	{
		Pool_Release(&dlist->framePool, pEntry->txFrames[type]);
		pEntry->txFrames[type] = NULL;
		return status;
	}

	return RT_STATUS_SUCCESS;
}

static
P2P_DEV_LIST_ENTRY *
p2p_devlist_Get(
	IN  const P2P_DEV_LIST		*dlist,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE 			type
	)
{
	RT_LIST_ENTRY				*pEntry = NULL;
	
	RtEntryListForEach(&dlist->list, pEntry)
	{
		P2P_DEV_LIST_ENTRY		*pDev = (P2P_DEV_LIST_ENTRY *)pEntry;

		if(pDev->type != type)
			continue;

		if(!eqMacAddr(pDev->mac, mac))
			continue;

		return pDev;
	}
	
	return NULL;
}

static
P2P_DEV_LIST_ENTRY *
p2p_devlist_GetGo(
	IN  const P2P_DEV_LIST		*dlist,
	IN  const u1Byte			*devAddr
	)
{
	RT_LIST_ENTRY				*pEntry = NULL;
	
	RtEntryListForEach(&dlist->list, pEntry)
	{
		P2P_DEV_LIST_ENTRY		*pDev = (P2P_DEV_LIST_ENTRY *)pEntry;

		if(P2P_DEV_TYPE_GO != pDev->type)
			continue;

		if(pDev->rxFrames[P2P_FID_BEACON]
			&& pDev->rxFrames[P2P_FID_BEACON]->msg->devIdDevAddr
			&& eqMacAddr(pDev->rxFrames[P2P_FID_BEACON]->msg->devIdDevAddr, devAddr)
			)
		{
			return pDev;
		}

		if(pDev->rxFrames[P2P_FID_PROBE_RSP]
			&& pDev->rxFrames[P2P_FID_PROBE_RSP]->msg->devInfoDevAddr
			&& eqMacAddr(pDev->rxFrames[P2P_FID_PROBE_RSP]->msg->devInfoDevAddr, devAddr)
			)
		{
			return pDev;
		}
	}
	
	return NULL;
}

static
VOID
p2p_devlist_Flush(
	IN  P2P_DEV_LIST			*dlist
	)
{
	while(RTIsListNotEmpty(&dlist->list))
	{
		p2p_devlist_FreeDev(dlist, (P2P_DEV_LIST_ENTRY *)RTGetHeadList(&dlist->list));
	}

	return;
}

static
u8Byte
p2p_devlist_RxFrameElapsedTimeMs(
	IN  const P2P_FRAME_INFO 	*finfo
	)
{
	return (PlatformGetCurrentTime() - finfo->time) / 1000; 
}

static
const char *
p2p_devlist_FrameTypeStr(
	IN  P2P_FRAME_TYPE 			type
	)
{
	const char 					*typeStr = NULL;
	
	if(P2P_FID_BEACON == type)
		typeStr = "BEACON";
	else if(P2P_FID_PROBE_REQ == type)
		typeStr = "ProbeReq";
	else if(P2P_FID_PROBE_RSP == type)
		typeStr = "ProbeRsp";
	else if(P2P_FID_GO_NEG_REQ == type)
		typeStr = "GoNegReq";
	else if(P2P_FID_GO_NEG_RSP == type)
		typeStr = "GoNegRsp";
	else if(P2P_FID_GO_NEG_CONF == type)
		typeStr = "GoNegConf";
	else if(P2P_FID_INV_REQ == type)
		typeStr = "InvReq";
	else if(P2P_FID_INV_RSP == type)
		typeStr = "InvRsp";
	else if(P2P_FID_PD_REQ == type)
		typeStr = "PdReq";
	else if(P2P_FID_PD_RSP == type)
		typeStr = "PdRsp";
	else 
		typeStr = "unknown";

	return typeStr;
}

static
const char *
p2p_devlist_DevTypeStr(
	IN  P2P_DEV_TYPE			type	
	)
{
	const char 					*typeStr = NULL;
	
	if(P2P_DEV_TYPE_LEGACY == type)
		typeStr = "Legacy";
	else if(P2P_DEV_TYPE_GO == type)
		typeStr = "GO";
	else if(P2P_DEV_TYPE_DEV == type)
		typeStr = "Device";
	else 
		typeStr = "unknown";

	return typeStr;
}

static
VOID
p2p_devlist_DumpFrameInfo(
	IN  const P2P_FRAME_INFO 	*finfo
	)
{
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("***\n"));
	
	// Type
	RT_TRACE(COMP_P2P, DBG_LOUD, ("type: %s\n", p2p_devlist_FrameTypeStr(finfo->type)));

	// Time
	RT_TRACE(COMP_P2P, DBG_LOUD, ("elapsed time: %u (ms)\n", (u4Byte)(PlatformGetCurrentTime() - finfo->time) / 1000));

	// Channel
	RT_TRACE(COMP_P2P, DBG_LOUD, ("channel: %u\n", finfo->channel));

	// Signal power
	RT_TRACE(COMP_P2P, DBG_LOUD, ("sig pow: %d\n", finfo->sigPower));

	// Signal strength
	RT_TRACE(COMP_P2P, DBG_LOUD, ("sig strength: %u\n", finfo->sigStrength));
	
}

static
VOID
p2p_devlist_DumpDev(
	IN  const P2P_DEV_LIST_ENTRY *pDev
	)
{
	RT_LIST_ENTRY				*pRxFrame = NULL;
	u4Byte						it = 0;

	if(P2P_DEV_TYPE_LEGACY == pDev->type)
	{
		if(!(pDev->rxFrames[P2P_FID_BEACON] && pDev->rxFrames[P2P_FID_PD_RSP]))
			return;
	}

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("---\n"));

	// MAC address
	//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("mac: "MACSTR"\n", MAC2STR(pDev->mac)));

	// Type
	RT_TRACE(COMP_P2P, DBG_LOUD, ("dev type: %s\n", p2p_devlist_DevTypeStr(pDev->type)));

	// Dev name
	for(it = 0; it < P2P_FID_MAX; it++)
	{
		if(pDev->rxFrames[it] && pDev->rxFrames[it]->msg->_wpsDevName)
		{
			RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Dev name: ", pDev->rxFrames[it]->msg->_wpsDevName, pDev->rxFrames[it]->msg->wpsDevNameLen);
			break;
		}
	}

	// Manufacturer
	for(it = 0; it < P2P_FID_MAX; it++)
	{
		if(pDev->rxFrames[it] && pDev->rxFrames[it]->msg->_wpsManufacturer)
		{
			RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Manufacturer: ", pDev->rxFrames[it]->msg->_wpsManufacturer, pDev->rxFrames[it]->msg->wpsManufacturerLen);
			break;
		}
	}

	// Rx frames
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Rx frames in chronological order\n"));
	
	RtEntryListForEach(&pDev->rxFrameQ, pRxFrame)
	{
		p2p_devlist_DumpFrameInfo((P2P_FRAME_INFO *)pRxFrame);
	}

	// Tx frames
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Tx frames\n"));
	
	for(it = 0; it < P2P_FID_MAX; it++)
	{
		if(!pDev->txFrames[it])
			continue;
		p2p_devlist_DumpFrameInfo(pDev->txFrames[it]);
	}

	// PD config method
	RT_TRACE(COMP_P2P, DBG_LOUD, ("pdConfigMethod: 0x%02X\n", pDev->p2p->pdConfigMethod));
}

static
P2P_FRAME_TYPE
p2p_devlist_GetCorrespReqActionCode(
	IN  P2P_FRAME_TYPE			frameType
	)
{
	if(P2P_FID_GO_NEG_RSP == frameType)
		return P2P_FID_GO_NEG_REQ;

	if(P2P_FID_GO_NEG_CONF == frameType)
		return P2P_FID_GO_NEG_RSP;

	if(P2P_FID_INV_RSP == frameType)
		return P2P_FID_INV_REQ;

	if(P2P_FID_PD_RSP == frameType)
		return P2P_FID_PD_REQ;
	
	return P2P_FID_MAX;
}


//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

RT_STATUS
p2p_DevList_Init(
	IN  P2P_DEV_LIST			*dlist
	)
{
	PlatformZeroMemory(dlist, sizeof(*dlist));

	p2p_InitLock(&dlist->lock);

	dlist->count = 0;
	RTInitializeListHead(&dlist->list);
	dlist->sig = signature;
	
	Pool_Init(&dlist->entryPool, "DevPool", sizeof(dlist->entryPoolRsvd), dlist->entryPoolRsvd, sizeof(dlist->entryPoolRsvd[0]), 0, DBG_LOUD);
	Pool_Init(&dlist->p2pDevInfoPool, "P2PDevInfoPool", sizeof(dlist->p2pDevInfoPoolRsvd), dlist->p2pDevInfoPoolRsvd, sizeof(dlist->p2pDevInfoPoolRsvd[0]), 0, DBG_LOUD);
	Pool_Init(&dlist->framePool, "FramePool", sizeof(dlist->framePoolRsvd), dlist->framePoolRsvd, sizeof(dlist->framePoolRsvd[0]), 0, DBG_LOUD);
	Pool_Init(&dlist->rxMsgPool, "RxMsgPool", sizeof(dlist->rxMsgPoolRsvd), dlist->rxMsgPoolRsvd, sizeof(dlist->rxMsgPoolRsvd[0]), 0, DBG_LOUD);

	return RT_STATUS_SUCCESS;
}

RT_STATUS
p2p_DevList_Flush(
	IN  P2P_DEV_LIST			*dlist
	)
{
	RT_LIST_ENTRY				*pEntry = NULL;
	
	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));

	FunctionIn(COMP_P2P);
	
	p2p_AcquireLock(&dlist->lock);
	
	RtEntryListForEach(&dlist->list, pEntry)
	{
		P2P_DEV_LIST_ENTRY		*pDev = (P2P_DEV_LIST_ENTRY *)pEntry;

		pDev->bDirty = FALSE;
	}

	p2p_ReleaseLock(&dlist->lock);

	FunctionOut(COMP_P2P);

	return RT_STATUS_SUCCESS;
}

RT_STATUS
p2p_DevList_Free(
	IN  P2P_DEV_LIST			*dlist
	)
{
	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));
	
	p2p_AcquireLock(&dlist->lock);
	
	p2p_devlist_Flush(dlist);

	dlist->sig = NULL;

	p2p_ReleaseLock(&dlist->lock);

	p2p_FreeLock(&dlist->lock);

	Pool_Dump(&dlist->entryPool);
	Pool_Dump(&dlist->p2pDevInfoPool);
	Pool_Dump(&dlist->framePool);
	Pool_Dump(&dlist->rxMsgPool);

	return RT_STATUS_SUCCESS;
}

VOID
p2p_DevList_Lock(
	IN  P2P_DEV_LIST			*dlist
	)
{
	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));
	p2p_AcquireLock(&dlist->lock);
}

VOID
p2p_DevList_Unlock(
	IN  P2P_DEV_LIST			*dlist
	)
{
	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));
	p2p_ReleaseLock(&dlist->lock);
}

RT_STATUS
p2p_DevList_RxUpdate(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_FRAME_TYPE			frameType,
	IN  const RT_RFD			*rfd,
	IN  u1Byte					*mac,
	IN  u1Byte					rxChnl,
	OUT P2P_DEV_LIST_ENTRY 		**ppDev
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	P2P_MESSAGE					*msg = NULL;
	P2P_DEV_LIST_ENTRY			*pDev = NULL;
	P2P_DEV_TYPE				type;
	P2P_FRAME_TYPE				reqFrameType = P2P_FID_MAX;

	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));

	if(ppDev)
		*ppDev = NULL;

	if(NULL == (msg = (P2P_MESSAGE *)Pool_Acquire(&dlist->rxMsgPool)))
	{
		if(!p2p_devlist_AgeFunction(dlist))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Failed to acquire new msg from the rx msg pool\n"));
			return RT_STATUS_RESOURCE;
		}

		msg = (P2P_MESSAGE *)Pool_Acquire(&dlist->rxMsgPool);
	}

	if(RT_STATUS_SUCCESS != (status = p2p_devlist_ParseFrame(frameType, rfd->PacketLength, rfd->Buffer.VirtualAddress, msg)))
	{
		Pool_Release(&dlist->rxMsgPool, msg);
		return status;
	}

	if(P2P_DEV_TYPE_LEGACY == (type = p2p_MessageSrcType(msg, frameType)))
	{
		p2p_parse_FreeMessage(msg);
		Pool_Release(&dlist->rxMsgPool, msg);
		return RT_STATUS_NOT_RECOGNIZED;
	}

	if(NULL != (pDev = p2p_devlist_Get(dlist, mac, P2P_DEV_TYPE_GO)) 
		&& P2P_FID_PD_RSP == frameType
		&& pDev->txFrames[P2P_FID_PD_REQ]
		)
	{// the peer has dual role, and we are doing PD with it, choose the GO
		type = P2P_DEV_TYPE_GO;
	}

	reqFrameType = p2p_devlist_GetCorrespReqActionCode(frameType);
	
	do
	{
		// Get existing/new device
		if(NULL == (pDev = p2p_devlist_Get(dlist, mac, type)))
		{
			if(P2P_FID_MAX != reqFrameType)
			{
				// Shall have corresponding req frame, but there's even no
				// device entry for the peer, this port is not the one which
				// sent the corresponding req frame.
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid ctx, mac: %02X:%02X:%02X:%02X:%02X:%02X, type: %u\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], type));
				status = RT_STATUS_INVALID_CONTEXT;
				break;
			}
			
			if(NULL == (pDev = p2p_devlist_NewDev(dlist, mac, type)))
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Failed to acquire new dev entry from the pool\n"));
				status = RT_STATUS_RESOURCE;
				break;
			}

			//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Add "MACSTR", type: %s for rx: %s\n", 
				//MAC2STR(mac), 
				//p2p_devlist_DevTypeStr(type),
				//p2p_devlist_FrameTypeStr(frameType)));
		}

		// Check dialog token
		if(P2P_FID_ACTION_START <= frameType
			&& pDev->rxFrames[frameType]
			)
		{// updating an action 
			if(msg->dialogToken == pDev->rxFrames[frameType]->msg->dialogToken)
			{
				//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("rx duplicated %s from "MACSTR", drop it\n", 
				//	p2p_devlist_FrameTypeStr(frameType),
				//	MAC2STR(mac)));
				status = RT_STATUS_PKT_DROP;
				break;
			}
		}

		if(P2P_FID_MAX != reqFrameType)
		{// this is a rsp action
			if(!pDev->txFrames[reqFrameType])
			{// this port didn't send the req action
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("no corresp tx frame %u, mac: %02X:%02X:%02X:%02X:%02X:%02X, type: %u\n", reqFrameType, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], type));
				status = RT_STATUS_INVALID_CONTEXT;
				break;
			}

			if(msg->dialogToken != pDev->txFrames[reqFrameType]->token)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("token mismatch, drop frame, req token: %u, rsp token: %u\n", pDev->txFrames[reqFrameType]->token, msg->dialogToken));
				status = RT_STATUS_PKT_DROP;
				break;
			}
		}

		// If already rx this type of rame, remove it from rxFrameQ
		if(pDev->rxFrames[frameType])
		{
			RTRemoveEntryList(&pDev->rxFrames[frameType]->list);
		}

		// Update rx frame
		if(RT_STATUS_SUCCESS != (status = p2p_devlist_RxUpdateFrameInfo(dlist, pDev, rfd, rxChnl, frameType, msg)))
		{
			break;
		}

		// Channel list
		if(P2P_DEV_TYPE_LEGACY != type && msg->_channelList)
		{
			p2p_Channel_Reset(&pDev->p2p->channels);
			p2p_parse_ChannelEntryList(msg->channelEntryListLen, msg->channelEntryList, &pDev->p2p->channels);
		}

		// Instert to the tail of the rxFrameQ
		if(pDev->rxFrames[frameType] != NULL)
			RTInsertTailList(&pDev->rxFrameQ, &pDev->rxFrames[frameType]->list);

		pDev->bDirty = TRUE;

		if(P2P_FID_ACTION_START <= frameType)
			p2p_devlist_DumpDev(pDev);
	}while(FALSE);

	if(RT_STATUS_SUCCESS != status && msg)
	{
		p2p_parse_FreeMessage(msg);
		Pool_Release(&dlist->rxMsgPool, msg);
	}

	if(ppDev)
		*ppDev = pDev;
	
	return status;
}

RT_STATUS
p2p_DevList_FlushActionFrames(
	IN  P2P_DEV_LIST			*dlist,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE			devType
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	P2P_DEV_LIST_ENTRY			*pDev = NULL;

	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));

	if(NULL != (pDev = p2p_devlist_Get(dlist, mac, devType)))
	{
		p2p_devlist_FreeRxFrames(dlist, pDev, TRUE);
		p2p_devlist_FreeTxFrames(dlist, pDev, TRUE);
	}

	return status;
}

RT_STATUS
p2p_DevList_TxUpdate(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_FRAME_TYPE			frameType,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE			devType,
	IN  const FRAME_BUF 		*buf,
	IN  u1Byte					token,
	IN  u1Byte					channel
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	P2P_DEV_LIST_ENTRY			*pDev = NULL;

	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));
	
	do
	{
		// Get existing/new device
		if(NULL == (pDev = p2p_devlist_Get(dlist, mac, devType)))
		{
			if(NULL == (pDev = p2p_devlist_NewDev(dlist, mac, devType)))
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Failed to acquire new dev entry from the pool\n"));
				status = RT_STATUS_RESOURCE;
				break;
			}

			//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Add "MACSTR", type: %s for tx: %s", 
				//MAC2STR(mac), 
				//p2p_devlist_DevTypeStr(devType),
				//p2p_devlist_FrameTypeStr(frameType)));
		}

		// Update tx frame
		if(frameType < P2P_FID_ACTION_START && token)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("token (%u) specified when updating a mgnt frame, reset it to 0\n", token));
			token = 0;
		}
		
		if(RT_STATUS_SUCCESS != (status = p2p_devlist_TxUpdateFrameInfo(dlist, pDev, buf, frameType, token, channel)))
		{
			break;
		}

		// set dialog token for action frame tx to the one actually set
		pDev->p2p->dialogToken = token;
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("token updated to: %u\n", token));

	}while(FALSE);
	
	return status;
}

VOID
p2p_DevList_DialogTokenUpdate(
	IN  P2P_DEV_LIST_ENTRY 		*pDev
	)
{
	pDev->p2p->dialogToken++;
	if(0 == pDev->p2p->dialogToken)
		pDev->p2p->dialogToken = 1;
	return;
}

const P2P_DEV_LIST_ENTRY *
p2p_DevList_Find(
	IN  const P2P_DEV_LIST		*dlist,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE 			type
	)
{
	const P2P_DEV_LIST_ENTRY	*pDev = NULL;

	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));

	pDev = p2p_devlist_Get(dlist, mac, type);
	
	return pDev;
}

P2P_DEV_LIST_ENTRY *
p2p_DevList_Get(
	IN  P2P_DEV_LIST			*dlist,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE 			type
	)
{
	P2P_DEV_LIST_ENTRY	*pDev = NULL;

	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));

	pDev = p2p_devlist_Get(dlist, mac, type);

	if(pDev && !pDev->bDirty)
	{// no rx frame since last flush
		return NULL;
	}
	
	return pDev;
}

P2P_DEV_LIST_ENTRY *
p2p_DevList_GetGo(
	IN  P2P_DEV_LIST			*dlist,
	IN  const u1Byte			*devAddr
	)
{
	return p2p_devlist_GetGo(dlist, devAddr);
}

RT_STATUS
p2p_DevList_Translate(
	IN  P2P_DEV_LIST			*dlist,
	IN  u4Byte					maxDevices,
	OUT u4Byte					*pNumDevCopied,
	OUT P2P_DEVICE_DISCRIPTOR	*descList
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	RT_LIST_ENTRY				*pEntry = NULL;
	u4Byte						nCopied = 0;

	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));

	*pNumDevCopied = 0;

	do
	{
		RtEntryListForEach(&dlist->list, pEntry)
		{
			P2P_DEV_LIST_ENTRY	*pDev = (P2P_DEV_LIST_ENTRY *)pEntry;

			if(P2P_DEV_TYPE_LEGACY == pDev->type)
				continue;

			if(!pDev->bDirty)
			{
				//RT_TRACE_F(COMP_P2P, DBG_TRACE, 
					//("skip "MACSTR" for no rx after previous flush\n", MAC2STR(pDev->mac)));
				continue;
			}

			if(!pDev->rxFrames[P2P_FID_PROBE_RSP]
				&& !pDev->rxFrames[P2P_FID_GO_NEG_REQ]
				&& !pDev->rxFrames[P2P_FID_GO_NEG_RSP]
				&& !pDev->rxFrames[P2P_FID_INV_REQ]
				&& !pDev->rxFrames[P2P_FID_PD_REQ]
				)
			{
				//RT_TRACE_F(COMP_P2P, DBG_LOUD, 
					//("skip "MACSTR" for not rx frames with dev info (no dev name)\n", MAC2STR(pDev->mac)));
				continue;
			}
			
			if(maxDevices <= nCopied)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, 
					("Unable to copy all descriptors: max: %u, total: %u\n", maxDevices, dlist->count));
				status = RT_STATUS_RESOURCE;
				break;
			}

			p2p_DevList_TranslateDev(pDev, descList + nCopied);

			nCopied++;
		}

		*pNumDevCopied = nCopied;
		
	}while(FALSE);

	return status;
}

VOID
p2p_DevList_TranslateDev(
	IN  const P2P_DEV_LIST_ENTRY *pDev,
	OUT P2P_DEVICE_DISCRIPTOR	*desc
	)
{
	RT_LIST_ENTRY				*pRxFrame = NULL;

	PlatformZeroMemory(desc, sizeof(*desc));

	RtEntryListForEach(&pDev->rxFrameQ, pRxFrame)
	{
		P2P_FRAME_INFO * finfo = (P2P_FRAME_INFO *)pRxFrame;
				
		p2p_parse_UpdateDevDesc(finfo->frame, 
			finfo->msg, 
			finfo->sigStrength, 
			desc);
	}

	return;
}

VOID
p2p_DevList_Dump(
	IN  P2P_DEV_LIST			*dlist
	)
{
	RT_LIST_ENTRY				*pEntry = NULL;

	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));

	RT_TRACE(COMP_P2P, DBG_LOUD, ("@@@\n"));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("count: %u\n", dlist->count));

	do
	{
		RtEntryListForEach(&dlist->list, pEntry)
		{
			p2p_devlist_DumpDev((P2P_DEV_LIST_ENTRY *)pEntry);
		}
		
	}while(FALSE);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("@@@\n"));

	return;
}

VOID
p2p_DevList_DumpDev(
	IN  const P2P_DEV_LIST_ENTRY *pDev
	)
{
	p2p_devlist_DumpDev(pDev);
	return;
}

u8Byte
p2p_DevList_RxFrameElapsedTimeMs(
	IN  P2P_DEV_LIST			*dlist,
	IN  const P2P_DEV_LIST_ENTRY *pDev,
	IN  P2P_FRAME_TYPE			frameType
	)
{
	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));
	
	if(!pDev)
		return 0;

	if(NULL == pDev->rxFrames[frameType])
		return 0;

	return p2p_devlist_RxFrameElapsedTimeMs(pDev->rxFrames[frameType]); 
}

BOOLEAN
p2p_DevList_ActionProcessed(
	IN  const OCTET_STRING		*posMpdu,
	IN  P2P_DEV_LIST			*dlist,
	IN  const P2P_DEV_LIST_ENTRY *pDev,
	IN  P2P_FRAME_TYPE			frameType
	)
{
	RT_ASSERT(signature == dlist->sig, ("Uninitialized list\n"));
	RT_ASSERT(P2P_FID_ACTION_START <= frameType, ("%s(): not an action frame: %u\n", __FUNCTION__, frameType));
	
	if(!pDev)
		return FALSE;

	if(NULL == pDev->rxFrames[frameType])
		return FALSE;

	if(pDev->rxFrames[frameType]->token == *(posMpdu->Octet + sMacHdrLng + 1 + 1 + 3 + 1 + 1))
		return TRUE;
	else
		return FALSE;
}

u1Byte
p2p_DevList_GetDevChnl(
	IN  P2P_DEV_LIST_ENTRY		*dev
	)
{
	u1Byte						chnl = 0;
	RT_LIST_ENTRY				*entry = RTGetTailList(&dev->rxFrameQ);

	while(entry != &dev->rxFrameQ)
	{
		P2P_FRAME_INFO			*finfo = (P2P_FRAME_INFO *)entry;

		// beacon/probe
		if(P2P_FID_BEACON == finfo->type
			|| P2P_FID_PROBE_RSP == finfo->type
			)
		{
			if(finfo->msg->_dsParam)
				chnl = finfo->msg->dsParamBuf[0];
			else
				chnl = finfo->channel;
			
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("get from beacon/probe, dsParam exist: %u, chnl: %u\n", finfo->msg->_dsParam ? 1 : 0, chnl));
			break;
		}

		// action
		if(!finfo->msg)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("a frame in rx frame queue does not have corresp. parsed msg\n"));
			p2p_DevList_DumpDev(dev);
		}
		else if(P2P_FID_INV_REQ == finfo->type)
		{
			if(finfo->msg->_grpId
				&& eqMacAddr(finfo->msg->grpDevAddr, finfo->msg->sa)
				&& finfo->msg->_opChannel
				)
			{
				chnl = finfo->msg->opChannel;
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("get from invReq sent by go, chnl: %u\n", chnl));
				break;
			}
		}
		else if(P2P_FID_INV_RSP == finfo->type)
		{
			P2P_FRAME_INFO		*req = dev->txFrames[P2P_FID_INV_REQ];
			
			if(req)
			{
				chnl = req->channel;
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("get from the chnl on which we sent inv req, chnl: %u\n", chnl));
				break;
			}
		}
		else if(P2P_FID_GO_NEG_RSP == finfo->type)
		{	
			if(finfo->msg->_status && P2P_ATTR_STATUS == finfo->msg->status
				&& finfo->msg->_grpId && !eqMacAddr(finfo->msg->grpDevAddr, finfo->msg->da) // i'm not the go, i.e., peer is the go
				&& finfo->msg->_opChannel
				)
			{
				chnl = finfo->msg->opChannel;
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("get from go neg rsp sent by go, chnl: %u\n", chnl));
				break;
			}
		}
		else if(P2P_FID_GO_NEG_CONF == finfo->type)
		{
			if(finfo->msg->_status && P2P_ATTR_STATUS == finfo->msg->status
				&& finfo->msg->_grpId && !eqMacAddr(finfo->msg->grpDevAddr, finfo->msg->da) // i'm not the go, i.e., peer is the go
				&& finfo->msg->_opChannel
				)
			{
				chnl = finfo->msg->opChannel;
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("get from go neg conf sent by go, chnl: %u\n", chnl));
				break;
			}
		}

		entry = RTForeEntryList(entry);
	}

	return chnl;
}

#endif
