#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Indication.tmh"
#endif

#include "P2P_Internal.h"
#include "P2P_Indication.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
static
VOID
p2p_IndicateEvent(
	IN  P2P_INFO				*pP2PInfo,
	IN  u4Byte					eventId,
	IN  const PP2P_EVENT_DATA 	pEventData
	)
{
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		MEMORY_BUFFER 			mbObject = {NULL, 0};

		mbObject.Buffer = (pu1Byte) pEventData;
		mbObject.Length = sizeof(P2P_EVENT_DATA);
		
		PlatformIndicateP2PEvent(pP2PInfo, eventId, &mbObject);
	}
	else
	{
		// Not implemented
	}

	return;
}

static
VOID
p2p_IndicateActionFrameReceivedWithToken(
	IN  P2P_INFO				*pP2PInfo,
	IN  u4Byte					eventId,
	IN  RT_STATUS				rtStatus,
	IN  u1Byte					*pFrameBuf,
	IN  u4Byte					frameLen,
	IN  u1Byte					token
	)
{
	u1Byte						origToken = GET_P2P_PUB_ACT_FRAME_DIALOG_TOKEN(pFrameBuf);
	
	SET_P2P_PUB_ACT_FRAME_DIALOG_TOKEN(pFrameBuf, token);
	
	p2p_IndicateActionFrameReceived(pP2PInfo, eventId, rtStatus, pFrameBuf, frameLen);

	SET_P2P_PUB_ACT_FRAME_DIALOG_TOKEN(pFrameBuf, origToken);

	return;
}


//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

VOID
p2p_IndicateActionFrameSendComplete(
	IN  P2P_INFO				*pP2PInfo,
	IN  u4Byte					eventId,
	IN  RT_STATUS				rtStatus,
	IN  u1Byte					*pFrameBuf,
	IN  u4Byte					frameLen
	)
{
	P2P_EVENT_DATA			eventData;

	PlatformZeroMemory(&eventData, sizeof(P2P_EVENT_DATA));

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		eventData.Packet.Buffer = pFrameBuf;
		eventData.Packet.Length = frameLen;
	}
	else
	{
		eventData.rtStatus = rtStatus;
	}

	p2p_IndicateEvent(pP2PInfo, eventId, &eventData);

	return;
}

VOID
p2p_IndicateActionFrameReceived(
	IN  P2P_INFO				*pP2PInfo,
	IN  u4Byte					eventId,
	IN  RT_STATUS				rtStatus,
	IN  u1Byte					*pFrameBuf,
	IN  u4Byte					frameLen
	)
{
	P2P_EVENT_DATA			eventData;

	PlatformZeroMemory(&eventData, sizeof(P2P_EVENT_DATA));

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		eventData.Packet.Buffer = pFrameBuf;
		eventData.Packet.Length = frameLen;
	}
	else
	{
		eventData.rtStatus = rtStatus;
	}

	p2p_IndicateEvent(pP2PInfo, eventId, &eventData);

	return;
}

VOID
p2p_IndicateActionFrameSendCompleteWithToken(
	IN  P2P_INFO				*pP2PInfo,
	IN  u4Byte					eventId,
	IN  RT_STATUS				rtStatus,
	IN  u1Byte					*pFrameBuf,
	IN  u4Byte					frameLen,
	IN  u1Byte					token
	)
{
	u1Byte						origToken = 0;

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		origToken = GET_P2P_PUB_ACT_FRAME_DIALOG_TOKEN(pFrameBuf);
		SET_P2P_PUB_ACT_FRAME_DIALOG_TOKEN(pFrameBuf, token);
	}
	
	p2p_IndicateActionFrameSendComplete(pP2PInfo, eventId, rtStatus, pFrameBuf, frameLen);

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		SET_P2P_PUB_ACT_FRAME_DIALOG_TOKEN(pFrameBuf, origToken);
	}
	
	return;
}

VOID
p2p_IndicatePdReqSent(
	IN  P2P_DEV_LIST_ENTRY		*pDev,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		// Change dialog token to make sure the token is the same as the previous query.
		p2p_IndicateActionFrameSendCompleteWithToken(pP2PInfo, 
			P2P_EVENT_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE, 
			RT_STATUS_SUCCESS, 
			pDev->txFrames[P2P_FID_PD_REQ]->frame, 
			pDev->txFrames[P2P_FID_PD_REQ]->frameLen,
			pP2PInfo->oidDialogToken);
	}
	else
	{
		p2p_IndicateActionFrameSendComplete(pP2PInfo, 
			P2P_EVENT_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE, 
			RT_STATUS_SUCCESS, 
			pDev->txFrames[P2P_FID_PD_REQ]->frame, 
			pDev->txFrames[P2P_FID_PD_REQ]->frameLen);
	}
}

VOID
p2p_IndicatePdRspReceived(
	IN  P2P_DEV_LIST_ENTRY		*pDev,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	u1Byte						*rxFrame = NULL;
	u2Byte						rxFrameLen = 0;
	
	if(pDev->rxFrames[P2P_FID_PD_RSP])
	{
		RT_ASSERT(pDev->txFrames[P2P_FID_PD_REQ], ("%s(): no corresponding tx frame\n", __FUNCTION__));
		
		rxFrame = pDev->rxFrames[P2P_FID_PD_RSP]->frame;
		rxFrameLen = pDev->rxFrames[P2P_FID_PD_RSP]->frameLen;
	}
	else
	{// failed to rx rsp frame
		status = RT_STATUS_FAILURE;
	}
	
	// Handle PD req dialog token
	// Note that if status is not success, frame and frameLen could be 0
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) 
		&& RT_STATUS_SUCCESS == status
		)
	{
		// Change dialog token to make sure the token is the same as the previous query.
		p2p_IndicateActionFrameReceivedWithToken(pP2PInfo, 
			P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE, 
			status, 
			rxFrame, 
			rxFrameLen,
			pP2PInfo->oidDialogToken);
	}
	else
	{
		p2p_IndicateActionFrameReceived(pP2PInfo, 
			P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE, 
			status, 
			rxFrame, 
			rxFrameLen);
	}

	return;
}

//
// Description:
//	Indiacate the fake provision discovery response to the upper layer.
// Arguments:
//	[in] pDev -
//		The device entry which is considered as the respondor for the provision discovery.
//	[in] pP2PInfo -
//		P2P information context.
// Return:
//	Return RT_STATUS_SUCCESS if the indication of fake reponse succeeds.
// By Bruce, 2015-02-17.
//
RT_STATUS
p2p_IndicateFakePdRspReceived(
	IN  P2P_DEV_LIST_ENTRY		*pDev,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	PADAPTER				pAdapter = pP2PInfo->pAdapter;
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	pu1Byte					pRxFrame = NULL;
	FRAME_BUF				fbuf;
	PRT_GEN_TEMP_BUFFER 	pGenBuf = NULL;

	FunctionIn(COMP_P2P);

	do
	{
		if(NULL == (pGenBuf = GetGenTempBuffer(pAdapter, GEN_TEMP_BUFFER_SIZE)))
		{
			RT_TRACE_F(COMP_P2P, DBG_SERIOUS, ("[ERROR] Memory allocation failed!\n"));
			rtStatus = RT_STATUS_RESOURCE;
			break;
		}

		FrameBuf_Init(pAdapter->MAX_TRANSMIT_BUFFER_SIZE, 0, (pu1Byte)(pGenBuf->Buffer.Ptr), &fbuf);
		FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);

		if(RT_STATUS_SUCCESS != (rtStatus = p2p_Construct_FakePDRsp(
												pP2PInfo,
												pDev,
												pP2PInfo->DeviceAddress,
												&fbuf
												)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Failed (0x%08X) from p2p_Construct_FakePDRsp()\n", rtStatus));
			break;
		}

		RT_PRINT_DATA(COMP_P2P, DBG_WARNING, "Fake provision response frame:\n", fbuf.os.Octet, fbuf.os.Length);

		// Handle PD req dialog token
		// Note that if status is not success, frame and frameLen could be 0
		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			// Change dialog token to make sure the token is the same as the previous query.
			p2p_IndicateActionFrameReceivedWithToken(pP2PInfo, 
				P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE, 
				rtStatus, 
				fbuf.os.Octet, 
				fbuf.os.Length,
				pP2PInfo->oidDialogToken);
		}
		else
		{
			p2p_IndicateActionFrameReceived(pP2PInfo, 
				P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE, 
				rtStatus, 
				fbuf.os.Octet, 
				fbuf.os.Length);
		}
	}while(FALSE);

	
	if(pGenBuf)
	{
		ReturnGenTempBuffer (pAdapter, pGenBuf);
		pGenBuf = NULL;
	}

	FunctionOut(COMP_P2P);

	return rtStatus;
}

#endif
