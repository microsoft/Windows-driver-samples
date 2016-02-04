#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_SendAction.tmh"
#endif

#include "P2P_Internal.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
static
VOID
p2p_Send_DumpTxReport(
	IN	const RT_TX_FEEDBACK_INFO_TX_PROFILE * const pTxProfile
	)
{
	if(NULL == pTxProfile)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("pTxProfile: is NULL!!!\n"));
		return;
	}
	
	if(pTxProfile->bPktOk.bVaild) 
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bPktOk: %u\n", pTxProfile->bPktOk.Data));
	}
	else 
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bPktOk: N/A\n"));
	}
	
	if(pTxProfile->bRetryOver.bVaild)
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bRetryOver: %u\n", pTxProfile->bRetryOver.Data));}
	else 
	{	
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bRetryOver: N/A\n"));
	}
	
	if(pTxProfile->bLifeTimeOver.bVaild)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bLifeTimeOver: %u\n", pTxProfile->bLifeTimeOver.Data));
	}
	else
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bLifeTimeOver: N/A\n"));
	}
	
	if(pTxProfile->bUnicast.bVaild)
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bUnicast: %u\n", pTxProfile->bUnicast.Data));
	}
	else
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bUnicast: N/A\n"));
	}
	
	if(pTxProfile->uQueueID.bVaild)
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uQueueID: %u\n", pTxProfile->uQueueID.Data));
	}
	else
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uQueueID: N/A\n"));
	}
	
	if(pTxProfile->uMacID.bVaild)
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uMacID: %u\n", pTxProfile->uMacID.Data));
	}
	else
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uMacID: N/A\n"));
	}
	
	if(pTxProfile->uDataRetryCount.bVaild)
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uDataRetryCount: %u\n", pTxProfile->uDataRetryCount.Data));
	}
	else
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uDataRetryCount: N/A\n"));
	}
	
	if(pTxProfile->uQueueTimeUs.bVaild)
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uQueueTimeUs: %u\n", pTxProfile->uQueueTimeUs.Data));
	}
	else
	{ 
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uQueueTimeUs: N/A\n"));
	}
	
	if(pTxProfile->uFinalDataRateIndex.bVaild) 
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uFinalDataRateIndex: %u\n", pTxProfile->uFinalDataRateIndex.Data));
	}
	else 
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("uFinalDataRateIndex: N/A\n"));
	}

	return;
}

static
BOOLEAN
p2p_GetSentFrame(
	IN  P2P_INFO				*info,
	IN  u1Byte					*devAddr,
	IN  P2P_DEV_TYPE 			devType,
	IN  P2P_FRAME_TYPE			frameType,
	OUT FRAME_BUF				*buf
	)
{
	BOOLEAN						bFound = FALSE;
	
	do
	{
		P2P_DEV_LIST_ENTRY *dev = NULL;

		FrameBuf_Init(0, 0, NULL, buf);
		
		if(NULL == (dev = p2p_DevList_Get(&info->devList, devAddr, devType)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("dev not found: %02X:%02X:%02X:%02X:%02X:%02X\n", 
				devAddr[0], devAddr[1], devAddr[2], devAddr[3], devAddr[4], devAddr[5]));
			break;
		}

		if(NULL == dev->txFrames[frameType]->frame)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("no tx frame recorded: %02X:%02X:%02X:%02X:%02X:%02X\n", 
				devAddr[0], devAddr[1], devAddr[2], devAddr[3], devAddr[4], devAddr[5]));
			break;
		}

		FrameBuf_Init(dev->txFrames[frameType]->frameLen, dev->txFrames[frameType]->frameLen, dev->txFrames[frameType]->frame, buf);

		bFound = TRUE;
	}while(FALSE);

	return bFound;
}

static
VOID
p2p_IndicateFrameSent(
	IN  P2P_INFO				*info,
	IN  u1Byte					*devAddr,
	IN  P2P_DEV_TYPE 			devType,
	IN  P2P_FRAME_TYPE			frameType,
	IN  u4Byte					eventId,
	IN  RT_STATUS				rtStatus
	)
{
	p2p_DevList_Lock(&info->devList);
	
	do
	{
		FRAME_BUF buf;
		
		if(!p2p_GetSentFrame(info, devAddr, devType, frameType, &buf))
		{
			break;
		}
		
		p2p_IndicateActionFrameSendComplete(info, 
			eventId, 
			rtStatus, 
			buf.os.Octet, 
			buf.os.Length);
		
	}while(FALSE);
	
	p2p_DevList_Unlock(&info->devList);

	return;
}

static
VOID
p2p_GoNegConfirmSent(
	IN  PADAPTER				pAdapter,
	IN  const RT_TX_FEEDBACK_INFO * const pTxFeedbackInfo
	)
{
	PP2P_INFO 					pP2PInfo = (pTxFeedbackInfo) ? 
											(pTxFeedbackInfo->pContext) : (NULL);
	u1Byte						dataRate = P2P_LOWEST_RATE;

	if(NULL == pTxFeedbackInfo) return;

	// This tx packet feedback reason doen't match.
	if(!(pTxFeedbackInfo->Reason & RT_TX_FEEDBACK_ID_P2P_NEGO_TX)) return;

	FunctionIn(COMP_P2P);

	do
	{
		if(pTxFeedbackInfo->TxProfile.bPktOk.bVaild && pTxFeedbackInfo->TxProfile.bPktOk.Data)
		{
			pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
			pP2PInfo->ConnectionContext.Status = P2P_STATUS_SUCCESS;
			PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("sent OK\n"));
		}
		else if(P2P_STATE_GO_NEGO_CONFIRM_SENT_WAIT == pP2PInfo->State)
		{
			BOOLEAN bSupportTxReport = FALSE;

			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("failed, try again\n"));		
			p2p_Send_DumpTxReport(&pTxFeedbackInfo->TxProfile);

			p2p_DevList_Lock(&pP2PInfo->devList);

			p2p_Send_GoNegConfirm(pP2PInfo, 
				pP2PInfo->ConnectionContext.ConnectingDevice.DeviceAddress, 
				pP2PInfo->ConnectionContext.DialogToken, 
				&bSupportTxReport);

			p2p_DevList_Unlock(&pP2PInfo->devList);
		}
	}while(FALSE);

	FunctionOut(COMP_P2P);

	return;
	
}

VOID
p2p_PDRspSent(
	IN	PADAPTER						pAdapter,
	const RT_TX_FEEDBACK_INFO * const 	pTxFeedbackInfo
	)
{
	PP2P_INFO 				pP2PInfo = (pTxFeedbackInfo) ? (pTxFeedbackInfo->pContext) : (NULL);
	BOOLEAN					bSendOk = FALSE;

	if(NULL == pTxFeedbackInfo) return;

	// This tx packet feedback reason doen't match.
	if(!(pTxFeedbackInfo->Reason & RT_TX_FEEDBACK_ID_P2P_PD_RSP_TX)) return;

	FunctionIn(COMP_P2P);

	do
	{
		p2p_Send_DumpTxReport(&pTxFeedbackInfo->TxProfile);

		bSendOk = (pTxFeedbackInfo->TxProfile.bPktOk.bVaild && pTxFeedbackInfo->TxProfile.bPktOk.Data);

		if(bSendOk)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("send OK\n"));
		}
		else
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("send failed\n"));		
		}

		P2PSvc_OnPDRspSent(pP2PInfo->pP2PSvcInfo, bSendOk);
		
	}while(FALSE);

	FunctionOut(COMP_P2P);
	
	return;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

VOID
p2p_Send_GoNegReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  BOOLEAN					bSend
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb = NULL;
	PRT_TX_LOCAL_BUFFER 		pBuf = NULL;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;
		u1Byte					dialogToken = pP2PInfo->ConnectionContext.DialogToken;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_GoNegReq(pP2PInfo, &fbuf, da, dialogToken);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		p2p_IndicateActionFrameSendComplete(pP2PInfo, 
			P2P_EVENT_GO_NEGOTIATION_REQUEST_SEND_COMPLETE, 
			RT_STATUS_SUCCESS, 
			pBuf->Buffer.VirtualAddress, 
			pTcb->PacketLength);
	
		if(bSend)
		{
			p2p_DevList_TxUpdate(&pP2PInfo->devList, P2P_FID_GO_NEG_REQ, 
				da, P2P_DEV_TYPE_DEV, &fbuf,
				dialogToken, RT_GetChannelNumber(pP2PInfo->pAdapter));

			MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
		}		
		else
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("update dev list only\n"));
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

	return;
}

VOID
p2p_Send_GoNegRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);
		
		p2p_Construct_GoNegRsp(pP2PInfo, dialogToken, &fbuf, da);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		p2p_IndicateActionFrameSendComplete(pP2PInfo, 
			P2P_EVENT_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE, 
			RT_STATUS_SUCCESS, 
			pBuf->Buffer.VirtualAddress, 
			pTcb->PacketLength);

		p2p_DevList_TxUpdate(&pP2PInfo->devList, P2P_FID_GO_NEG_RSP, 
			da, P2P_DEV_TYPE_DEV, &fbuf, 
			dialogToken, RT_GetChannelNumber(pP2PInfo->pAdapter));
		
		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_GoNegConfirm(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken,
	OUT PBOOLEAN 				pbSupportTxReport
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_GoNegConf(pP2PInfo, dialogToken, &fbuf, da);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		p2p_IndicateActionFrameSendComplete(pP2PInfo, 
			P2P_EVENT_GO_NEGOTIATION_CONFIRM_SEND_COMPLETE, 
			RT_STATUS_SUCCESS, 
			pBuf->Buffer.VirtualAddress, 
			pTcb->PacketLength);

		*pbSupportTxReport = TxFeedbackInstallTxFeedbackInfoForTcb(pAdapter, pTcb);
		
		if(*pbSupportTxReport)
		{
			TxFeedbackFillTxFeedbackInfoUserConfiguration(
						pTcb, 
						RT_TX_FEEDBACK_ID_P2P_NEGO_TX, 
						pAdapter, 
						p2p_GoNegConfirmSent,
						pP2PInfo
					);
		}

		p2p_DevList_TxUpdate(&pP2PInfo->devList, P2P_FID_GO_NEG_CONF, 
			da, P2P_DEV_TYPE_DEV, &fbuf,
			dialogToken, RT_GetChannelNumber(pP2PInfo->pAdapter));
		
		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_InvitationReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;
		u1Byte					dialogToken = pP2PInfo->InvitationContext.DialogToken;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_InvitationReq(pP2PInfo, &fbuf, da, dialogToken);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		p2p_IndicateActionFrameSendComplete(pP2PInfo, 
			P2P_EVENT_INVITATION_REQUEST_SEND_COMPLETE, 
			RT_STATUS_SUCCESS, 
			pBuf->Buffer.VirtualAddress, 
			pTcb->PacketLength);

		p2p_DevList_TxUpdate(&pP2PInfo->devList, P2P_FID_INV_REQ, 
			da, P2P_DEV_TYPE_DEV, &fbuf, 
			dialogToken, RT_GetChannelNumber(pP2PInfo->pAdapter));

		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_InvitationRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_InvitationRsp(pP2PInfo, dialogToken, &fbuf, da);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		p2p_IndicateActionFrameSendComplete(pP2PInfo, 
			P2P_EVENT_INVITATION_RESPONSE_SEND_COMPLETE, 
			RT_STATUS_SUCCESS, 
			pBuf->Buffer.VirtualAddress, 
			pTcb->PacketLength);

		p2p_DevList_TxUpdate(&pP2PInfo->devList, P2P_FID_INV_RSP, 
			da, P2P_DEV_TYPE_DEV, &fbuf,
			dialogToken, RT_GetChannelNumber(pP2PInfo->pAdapter));
	
		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_DevDiscReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*bssid,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_DevDiscReq(pP2PInfo, dialogToken, &fbuf, da, bssid);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_DevDiscRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken,
	IN  u1Byte					status
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						DataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_DevDiscRsp(pP2PInfo, dialogToken, &fbuf, da, status);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Send Packet! status = %d\n", status));
		
		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_PDReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*mac,
	IN  BOOLEAN					bGo,
	IN  u1Byte					dialogToken,
	IN  u2Byte					configMethod
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		// Increase the dialog token.
		IncreaseDialogToken(pP2PInfo->DialogToken);
		
		p2p_Construct_PDReq(pP2PInfo, &fbuf, mac, dialogToken, configMethod);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		p2p_DevList_TxUpdate(&pP2PInfo->devList, P2P_FID_PD_REQ, 
			mac, bGo ? P2P_DEV_TYPE_GO : P2P_DEV_TYPE_DEV, &fbuf,
			dialogToken, RT_GetChannelNumber(pP2PInfo->pAdapter));
		
		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_PDRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  OCTET_STRING 			*posP2PAttrs,
	IN  u2Byte 					configMethod,
	OUT PBOOLEAN 				pbSupportTxReport
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;
	BOOLEAN						bSupportTxReport = FALSE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(pbSupportTxReport) *pbSupportTxReport = FALSE;

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_PDRsp(pP2PInfo, dialogToken, posP2PAttrs, configMethod, &fbuf, da);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		p2p_IndicateActionFrameSendComplete(pP2PInfo, 
			P2P_EVENT_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE, 
			RT_STATUS_SUCCESS, 
			pBuf->Buffer.VirtualAddress, 
			pTcb->PacketLength);

		if(TRUE == (bSupportTxReport = TxFeedbackInstallTxFeedbackInfoForTcb(pAdapter, pTcb)))
		{
			TxFeedbackFillTxFeedbackInfoUserConfiguration(
						pTcb, 
						RT_TX_FEEDBACK_ID_P2P_PD_RSP_TX, 
						pAdapter, 
						p2p_PDRspSent,
						(PVOID)(pP2PInfo)
					);
		}

		if(pbSupportTxReport) *pbSupportTxReport = bSupportTxReport;

		p2p_DevList_TxUpdate(&pP2PInfo->devList, P2P_FID_PD_RSP, 
			da, P2P_DEV_TYPE_DEV, &fbuf,
			dialogToken, RT_GetChannelNumber(pP2PInfo->pAdapter));

		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_PresenceReq(
	IN  P2P_INFO				*pP2PInfo,
	IN  const P2P_POWERSAVE_SET	*pPs,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_PresenceReq(pP2PInfo, pPs, da, dialogToken, &fbuf);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_PresenceRsp(
	IN  P2P_INFO				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken,
	IN  u1Byte					status
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_PresenceRsp(pP2PInfo, da, dialogToken, status, &fbuf);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_GoDiscoverabilityReq(
	IN  P2P_INFO				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	)
{
	PADAPTER					pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 					pTcb;
	PRT_TX_LOCAL_BUFFER 		pBuf;
	u1Byte						dataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF				fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Copnstruct_GoDiscoverabilityReq(pP2PInfo, da, dialogToken, &fbuf);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);

		MgntSendPacket(pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, dataRate);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

VOID
p2p_Send_SDReq(
	IN  const P2P_INFO			*pP2PInfo,
	IN  const u1Byte 			*da
	)
{
	PADAPTER				pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 				pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u1Byte					DataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pP2PInfo->pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF			fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_SDReq(pP2PInfo, &fbuf, da);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);
		
		MgntSendPacket(pP2PInfo->pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}
		
	PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);

	return;
}

VOID
p2p_Send_SDRsp(
	IN  const P2P_INFO			*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  P2P_SD_STATUS_CODE		status,
	IN  BOOLEAN					bFrag
	)
{
	PADAPTER				pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 				pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u1Byte					DataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pP2PInfo->pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF			fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);
		
		p2p_Construct_SDRsp(pP2PInfo, &fbuf, da, dialogToken, status, bFrag);

		pTcb->PacketLength = FrameBuf_Length(&fbuf);
		
		MgntSendPacket(pP2PInfo->pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}
	
	PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);

	return;
}

VOID
p2p_Send_SDComebackReq(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken
	)
{
	PADAPTER				pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 				pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u1Byte					DataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pP2PInfo->pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF			fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_SDComebackReq(pP2PInfo, &fbuf, da, dialogToken);

		MgntSendPacket(pP2PInfo->pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}
	
	PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);

	return;
}

VOID
p2p_Send_SDComebackRsp(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  u2Byte					bytesToCopy,
	IN  BOOLEAN					bMoreData
	)
{
	PADAPTER				pAdapter = pP2PInfo->pAdapter;
	PRT_TCB 				pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u1Byte					DataRate = P2P_LOWEST_RATE;

	PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pP2PInfo->pAdapter, &pTcb, &pBuf))
	{
		FRAME_BUF			fbuf;

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, pBuf->Buffer.VirtualAddress, &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		p2p_Construct_SDComebackRsp(pP2PInfo, &fbuf, da, dialogToken, bytesToCopy, bMoreData);

		MgntSendPacket(pP2PInfo->pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}
	
	PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_TX_SPINLOCK);

	return;
}

#endif
