#include "Mp_Precomp.h"
#include "P2P_Internal.h"

VOID
N63CIndicateReceivedProvisionDiscoveryResponse(
	PP2P_INFO pP2PInfo,
	const N63C_SEND_ACTION_CTX *ctx,
	u4Byte frameLen,
	u1Byte *frameBuf
	);

VOID
N63CIndicateReceivedGONegotiationResponse(
	PP2P_INFO pP2PInfo,
	const N63C_SEND_ACTION_CTX *ctx,
	u4Byte frameLen,
	u1Byte *frameBuf
	);

VOID
N63CIndicateReceivedGONegotiationConfirm(
	PP2P_INFO pP2PInfo,
	const N63C_SEND_ACTION_CTX *ctx,
	u4Byte frameLen,
	u1Byte *frameBuf
	);

VOID
N63CIndicateReceivedInvitationResponse(
	PP2P_INFO pP2PInfo,
	const N63C_SEND_ACTION_CTX *ctx,
	u4Byte frameLen,
	u1Byte *frameBuf
	);

static u1Byte InfoBuf[1024];
static ULONG InfoBufLen;

//======================================================================================
// Local function for OffChnlTx
//======================================================================================

static
BOOLEAN
n63c_SendP2pActionFrameComplete(
	IN  PADAPTER				pAdapter,
	IN  const N63C_SEND_ACTION_CTX *ctx
	)
{
	P2P_INFO				*info = GET_P2P_INFO(pAdapter);
	OFF_CHNL_TX_STATUS_FLAG txStatusFlag;
	NDIS_STATUS				status = NDIS_STATUS_SUCCESS;
	FRAME_BUF				*buf = OffChnlTx_GetTxFrameBuf(ctx->req);
	
	FunctionIn(COMP_OID_SET);
	
	txStatusFlag = OffChnlTx_GetTxStatusFlag(ctx->req);

	status = (TEST_FLAG(txStatusFlag.perTxAttemptFlag, OFF_CHNL_TX_PER_TX_ATTEMPT_FLAG_FRAME_SENT) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);

	RT_ASSERT(buf, ("%s(): buf is NULL!!!n", __FUNCTION__));

	RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("indicate frame sent with token: %u\n", ctx->token));
		
	// Change dialog token to make sure the token is the same as the previous query.
	p2p_IndicateActionFrameSendCompleteWithToken(info, 
		ctx->compEvent, 
		(NDIS_STATUS_SUCCESS == status) ? RT_STATUS_SUCCESS : RT_STATUS_FAILURE, 
		FrameBuf_MHead(buf), 
		FrameBuf_Length(buf),
		ctx->token);

	FunctionOut(COMP_OID_SET);

	return TRUE;
}

static
BOOLEAN
n63c_Construct_FakeInvitationRsp(
	IN  ADAPTER						*pAdapter,
	IN  const N63C_SEND_ACTION_CTX 	*ctx,
	IN  const P2P_DEV_LIST_ENTRY 	*dev,
	OUT FRAME_BUF					*fbuf
	)
{
	P2P_INFO				*info = GET_P2P_INFO(pAdapter);
	OCTET_STRING			osWFDIE, osProbeRspPkt;

	FunctionIn(COMP_OID_SET);
	
	// MAC Header
	p2p_add_ActionFrameMacHdr(fbuf, info->DeviceAddress, dev->mac, info->DeviceAddress);

	// Action Header
	p2p_add_P2PPublicActionHdr(fbuf, P2P_INVITATION_RSP, ctx->token);

	// P2P IE
	p2p_build_FakeInvitationRspIe(fbuf, info, dev->mac);

	// WFD IE, from probe rsp
	if(dev->rxFrames[P2P_FID_PROBE_RSP] && dev->rxFrames[P2P_FID_PROBE_RSP]->frameLen)
	{
		FillOctetString(osProbeRspPkt, 
			dev->rxFrames[P2P_FID_PROBE_RSP]->frame, 
			dev->rxFrames[P2P_FID_PROBE_RSP]->frameLen
			);
		
		osWFDIE = PacketGetElement(osProbeRspPkt, EID_Vendor, OUI_SUB_WIFI_DISPLAY, OUI_SUB_DONT_CARE);

		if(osWFDIE.Length > 0)
		{
			PacketMakeElement(&fbuf->os, EID_Vendor, osWFDIE);
		}
	}

	RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("a fake invitation rsp constructed\n"));
	FrameBuf_Dump(fbuf, 0, DBG_LOUD, __FUNCTION__);

	N6CWriteEventLogEntry(pAdapter, NDIS_STATUS_SUCCESS, 
		FrameBuf_Length(fbuf), FrameBuf_MHead(fbuf), 
		L"A fake invitation rsp constructed"
		);

	FunctionOut(COMP_OID_SET);
	
	return TRUE;
}

static
BOOLEAN
n63c_Construct_FakePdRsp(
	IN  ADAPTER						*pAdapter,
	IN  const N63C_SEND_ACTION_CTX 	*ctx,
	IN  const P2P_DEV_LIST_ENTRY 	*dev,
	OUT FRAME_BUF					*fbuf
	)
{
	P2P_INFO				*info = GET_P2P_INFO(pAdapter);
	OCTET_STRING			osPdReq, osTmpIe;
	
	FunctionIn(COMP_OID_SET);

	if(dev->txFrames[P2P_FID_PD_REQ] && dev->txFrames[P2P_FID_PD_REQ]->frameLen)
	{
		FillOctetString(osPdReq, 
			dev->txFrames[P2P_FID_PD_REQ]->frame, 
			dev->txFrames[P2P_FID_PD_REQ]->frameLen
			);
	}
	else
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("no corresp. pd req\n"));
		FillOctetString(osPdReq, NULL, 0);
	}
	
	// MAC Header
	p2p_add_ActionFrameMacHdr(fbuf, info->DeviceAddress, dev->mac, dev->mac);

	// Action Header
	p2p_add_P2PPublicActionHdr(fbuf, P2P_PROV_DISC_RSP, ctx->token);

	// WPS IE, getting from the original PD request
	if(osPdReq.Length)
	{	
		osTmpIe = PacketGetElement(osPdReq, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUB_DONT_CARE);
		if(osTmpIe.Length)
		{
			PacketMakeElement(&fbuf->os, EID_Vendor, osTmpIe);
		}
	}
	
	// WFD IE
	if(osPdReq.Length)
	{
		osTmpIe = PacketGetElement(osPdReq, EID_Vendor, OUI_SUB_WIFI_DISPLAY, OUI_SUB_DONT_CARE);
		if(osTmpIe.Length)
		{
			PacketMakeElement(&fbuf->os, EID_Vendor, osTmpIe);
		}			
	}
	
	RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("a fake pd rsp constructed\n"));
	FrameBuf_Dump(fbuf, 0, DBG_LOUD, __FUNCTION__);

	N6CWriteEventLogEntry(pAdapter, NDIS_STATUS_SUCCESS, 
		FrameBuf_Length(fbuf), FrameBuf_MHead(fbuf), 
		L"A fake pd rsp constructed"
		);


	FunctionOut(COMP_OID_SET);

	return TRUE;
}

static
BOOLEAN
n63c_IndicateRxP2pRspActionFrame(
	IN  ADAPTER					*pAdapter,
	IN  const N63C_SEND_ACTION_CTX *ctx
	)
{
	typedef VOID (*N63C_INDIC_RX_P2P_RSP_ACTION_FRAME_PFN)(P2P_INFO *, const N63C_SEND_ACTION_CTX *, u4Byte, u1Byte *);
	
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	N63C_INDIC_RX_P2P_RSP_ACTION_FRAME_PFN pfn = NULL;					
	u4Byte						eid = P2P_EVENT_NONE;
	P2P_FRAME_TYPE				ft = P2P_FID_MAX;

	RT_GEN_TEMP_BUFFER			*pGenBuf = NULL;
	FRAME_BUF					fbuf;
	
	P2P_DEV_LIST_ENTRY			*dev = NULL;
	u4Byte						ieOffset = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;

	BOOLEAN						bIndicate = FALSE;

	p2p_DevList_Lock(&info->devList);

	do
	{
		if(NULL == (pGenBuf = GetGenTempBuffer(pAdapter, GEN_TEMP_BUFFER_SIZE)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_SERIOUS, ("[ERROR] Memory allocation failed!\n"));
			break;
		}

		FrameBuf_Init(GEN_TEMP_BUFFER_SIZE, 0, (u1Byte *)pGenBuf->Buffer.Ptr, &fbuf);
		
		// get corresp. types
		if(N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == ctx->rspType)
		{
			pfn = N63CIndicateReceivedProvisionDiscoveryResponse;
			eid = P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE;
			ft = P2P_FID_PD_RSP;
		}
		else if(N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE == ctx->rspType)
		{
			pfn = N63CIndicateReceivedGONegotiationResponse;
			eid = P2P_EVENT_RECEIVED_GO_NEGOTIATION_RESPONSE;
			ft = P2P_FID_GO_NEG_RSP;
		}
		else if(N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM == ctx->rspType)
		{
			pfn = N63CIndicateReceivedGONegotiationConfirm;
			eid = P2P_EVENT_RECEIVED_GO_NEGOTIATION_CONFIRM;
			ft = P2P_FID_GO_NEG_CONF;
		}
		else if(N63C_P2P_ACTION_FRAME_INVITATION_RESPONSE == ctx->rspType)
		{
			pfn = N63CIndicateReceivedInvitationResponse;
			eid = P2P_EVENT_RECEIVED_INVITATION_RESPONSE;
			ft = P2P_FID_INV_RSP;
		}
		else
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("invalid response type: %u\n", ctx->rspType));
			break;
		}

		// get dev
		if(NULL == (dev = p2p_DevList_Get(&info->devList, ctx->da, ctx->bGo ? P2P_DEV_TYPE_GO : P2P_DEV_TYPE_DEV)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("dev not found\n"));
			break;
		}

		if(dev->rxFrames[ft])
		{
			if(dev->rxFrames[ft]->frameLen < ieOffset + 1)
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("invalid received rsp action frame len: %u\n", dev->rxFrames[ft]->frameLen));
				break;
			}

			FrameBuf_Add_Data(&fbuf, dev->rxFrames[ft]->frame, dev->rxFrames[ft]->frameLen);
		}
		else
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("no coresp. received rsp action frame, rsp type: %u\n", ft));

			if(N63C_P2P_ACTION_FRAME_INVITATION_RESPONSE == ctx->rspType)
			{
				if(!n63c_Construct_FakeInvitationRsp(pAdapter, ctx, dev, &fbuf))
					break;
			}
			else if(N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == ctx->rspType)
			{
				if(!n63c_Construct_FakePdRsp(pAdapter, ctx, dev, &fbuf))
					break;
			}
			else
			{
				break;
			}		
		}

		bIndicate = TRUE;
	}while(FALSE);

	p2p_DevList_Unlock(&info->devList);

	if(bIndicate)
	{// to indicate w/o lock acquired
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("to indicate rx action frame with token: %u\n", ctx->token));
		(pfn)(info, ctx, FrameBuf_Length(&fbuf), FrameBuf_MHead(&fbuf));
	}

	if(pGenBuf)
	{
		ReturnGenTempBuffer(pAdapter, pGenBuf);
	}

	FunctionOut(COMP_OID_SET);

	return bIndicate;
}

static
BOOLEAN
n63c_SendP2pActionFrameStateCb(
	IN	VOID					*req,
	IN	int 					state,
	IN	VOID					*pCtx
	)
{
	BOOLEAN						bHandled = FALSE;

	N63C_SEND_ACTION_CTX		*ctx = (N63C_SEND_ACTION_CTX *)pCtx;
	ADAPTER						*pAdapter = ctx->pAdapter;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);

	do
	{
		if(OFF_CHNL_TX_STATE_CHNL_HIT == state)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("on chnl\n"));		
			bHandled = TRUE;
			break;
		}

		if(OFF_CHNL_TX_STATE_COMPLETE == state)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("complete\n"));	
			bHandled = TRUE;
			break;
		}
		
		if(OFF_CHNL_TX_STATE_FRAME_SENT == state)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("frame sent\n"));
			bHandled = TRUE;
			break;
		}
	}while(FALSE);

	return bHandled;
}

static
VOID
n63c_SendP2pRspActionFrameStateCb(
	IN  VOID					*req,
	IN  int						state,
	IN  VOID					*pCtx
	)
{
	N63C_SEND_ACTION_CTX		*ctx = (N63C_SEND_ACTION_CTX *)pCtx;
	ADAPTER						*pAdapter = ctx->pAdapter;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	
	n63c_SendP2pActionFrameStateCb(req, state, pCtx);
	
	if(OFF_CHNL_TX_STATE_COMPLETE == state)
	{
		n63c_SendP2pActionFrameComplete(pAdapter, ctx);

		// restore Dialog Token in P2PInfo since we are sending 
		// response action frame, self dialog token shall not be 
		// modified
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("restore token from %u to %u\n", info->DialogToken, ctx->oldToken));
		info->DialogToken = ctx->oldToken;
	}
	else if(OFF_CHNL_TX_STATE_PRE_DESTROY == state)
	{
		if(N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM == ctx->rspType)
		{
			n63c_IndicateRxP2pRspActionFrame(pAdapter, ctx);
		}

	}
	else if(OFF_CHNL_TX_STATE_RETRY == state)
	{
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("retry with token: %u, reqType: %u\n", ctx->token, ctx->reqType));
	}
	
	return;
}

static
VOID
n63c_SendP2pReqActionFrameStateCb(
	IN  VOID					*req,
	IN  int						state,
	IN  VOID					*pCtx
	)
{
	N63C_SEND_ACTION_CTX		*ctx = (N63C_SEND_ACTION_CTX *)pCtx;
	ADAPTER						*pAdapter = ctx->pAdapter;
	
	n63c_SendP2pActionFrameStateCb(req, state, pCtx);
	
	if(OFF_CHNL_TX_STATE_COMPLETE == state)
	{
		P2P_INFO				*info = GET_P2P_INFO(pAdapter);
		
		n63c_SendP2pActionFrameComplete(pAdapter, ctx);

		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("restore StateBeforeScan to initialized\n"));
		info->StateBeforeScan = P2P_STATE_INITIALIZED;
	}
	else if(OFF_CHNL_TX_STATE_PRE_DESTROY == state)
	{
		if(N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == ctx->rspType
			|| N63C_P2P_ACTION_FRAME_INVITATION_RESPONSE == ctx->rspType
			)
		{
			n63c_IndicateRxP2pRspActionFrame(pAdapter, ctx);
		}


		if(N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE == ctx->rspType)
		{
			InfoBufLen = 0;
			n63c_IndicateRxP2pRspActionFrame(pAdapter, ctx);
			if(InfoBufLen)
			{
				if(NULL == N63C_SendAction_GoNegConfirm(pAdapter, (DOT11_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS *)InfoBuf, n63c_SendP2pRspActionFrameStateCb))
				{
					RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("N63C_SendAction_GoNegConfirm failed\n"));
					p2p_IndicateActionFrameSendCompleteWithToken(GET_P2P_INFO(pAdapter), P2P_EVENT_GO_NEGOTIATION_CONFIRM_SEND_COMPLETE, RT_STATUS_FAILURE, NULL, 0, 0);
				}
				InfoBufLen = 0;
			}
		}
	}
	else if(OFF_CHNL_TX_STATE_RETRY == state)
	{
		u1Byte					newToken = 0;
		static BOOLEAN			bSkip = TRUE;

		newToken = N63C_SendAction_UpdateTxFrameDialogToken(req, pAdapter, ctx->reqType);

		if(pAdapter->bInHctTest)
		{
			// Not to skip probing under HCT test, because it wastes time 
			// sending action frame to peer on its Listen Channel when peer
			// is not doing extended listen
			bSkip = FALSE;
		}
		
		if(bSkip)
		{
			N63C_SendAction_SkipProbeTemporarilly(req);
		}
		
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("retry with token: %u, reqType: %u, skip probe: %u\n", newToken, ctx->reqType, bSkip));

		bSkip = !bSkip;
	}
	
	return;
}

//======================================================================================
// Put some temp codes for Windows 8
//======================================================================================

VOID
N63CValidateRoleOfWifiDirectPorts(
	PADAPTER pAdapter
)
{
	PADAPTER pTargetAdapter = GetDefaultAdapter(pAdapter);
	u1Byte uNumberOfDevicePort = 0;
	u1Byte uNumberOfGoPort = 0;
	u1Byte uNumberOfClientPort = 0;

	while(pTargetAdapter)
	{
		if(P2P_ENABLED(GET_P2P_INFO(pTargetAdapter)))
		{
			switch(GET_P2P_INFO(pTargetAdapter)->Role)
			{

				case P2P_DEVICE: uNumberOfDevicePort++;	break;
				case P2P_CLIENT: uNumberOfClientPort++;	break;
				case P2P_GO: 	uNumberOfGoPort++;		break;
				default:
					break;
			}
		}


		pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
	}

	if(uNumberOfDevicePort > 2)	
	{
		RT_TRACE((COMP_OID_QUERY | COMP_OID_SET | COMP_P2P), DBG_SERIOUS, 
				("Wifi-Direct Port Failure: %s: uNumberOfDevicePort > 2\n", __FUNCTION__)
			);
	}

	if(uNumberOfGoPort > 2)	
	{
		RT_TRACE((COMP_OID_QUERY | COMP_OID_SET | COMP_P2P), DBG_SERIOUS, 
				("Wifi-Direct Port Failure: %s: uNumberOfGoPort > 2)\n", __FUNCTION__)
			);
	}
	
	if(uNumberOfClientPort > 1)	
	{
		RT_TRACE((COMP_OID_QUERY | COMP_OID_SET | COMP_P2P), DBG_SERIOUS, 
				("Wifi-Direct Port Failure: %s: uNumberOfClientPort > 1\n", __FUNCTION__)
			);
	}
}

VOID
N63CResetNetworkListOffload(
	PADAPTER pAdapter
)
{
	PRT_NLO_INFO		pNLOInfo = &(pAdapter->MgntInfo.NLOInfo);

	RT_TRACE((COMP_OID_QUERY | COMP_OID_SET), DBG_LOUD, ("--- Reset Network List Offload --- \n"));

	pNLOInfo->NumDot11OffloadNetwork = 0;
	pNLOInfo->FastScanIterations = 0;
	pNLOInfo->FastScanPeriod = 0;
	pNLOInfo->SlowScanPeriod = 0;
}


NDIS_STATUS
N63CResetWifiDirectPorts(
	PADAPTER pAdapter,
	P2P_ROLE PortRole,
	RESET_LEVEL ResetLevel
)
{
	PRT_NDIS62_COMMON pNdis62Common = pAdapter->pNdis62Common;
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PP2P_INFO pP2PInfo = (PP2P_INFO)(pAdapter->MgntInfo.pP2PInfo);
	PP2P_INFO pP2PDevInfo = NULL;
	u1Byte uListenChannel = 0;
	u1Byte uOpChannel = 0;
	u4Byte	BoostInitGainValue = 0;
	u1Byte	uAutoChnl = P2P_DEFAULT_OPERATING_CHANNEL;
	
	PADAPTER pDevicePort = NULL;

	FunctionIn(COMP_P2P);

#if (MULTICHANNEL_SUPPORT == 1)
	MultichannelHandlePacketDuringScan(pAdapter,FALSE);
#endif

	// Set the default OpChannel and ListenChannel ----------------------------------------------------------------------------
	uListenChannel = P2P_DEFAULT_LISTEN_CHANNEL;
	uOpChannel = uListenChannel;


	if(pDefaultAdapter->MgntInfo.WFDOpChannel !=0)
		uOpChannel = pDefaultAdapter->MgntInfo.WFDOpChannel;
	else if(MgntLinkStatusQuery(pDefaultAdapter) == RT_MEDIA_CONNECT)
	{
		uOpChannel = RT_GetChannelNumber(pDefaultAdapter);
		RT_TRACE(COMP_P2P, DBG_LOUD, ("--- Use Default Port Channel: OpChannel %d ---\n", uOpChannel));
	}
	else if(GetFirstGOPort(pAdapter))
	{
		pP2PDevInfo = GET_P2P_INFO(GetFirstGOPort(pAdapter));
		uOpChannel = pP2PDevInfo->OperatingChannel;
	}
	else if(GetFirstClientPort(pAdapter))
	{
		pP2PDevInfo = GET_P2P_INFO(GetFirstClientPort(pAdapter));
		uOpChannel = pP2PDevInfo->OperatingChannel;
	}
	else if(GetFirstDevicePort(pAdapter))
	{
		pP2PDevInfo = GET_P2P_INFO(GetFirstDevicePort(pAdapter));
		uOpChannel = pP2PDevInfo->OperatingChannel;
	}
	else if(PortRole == P2P_DEVICE)
	{
#if (AUTO_CHNL_SEL_NHM ==1)		
		if(IS_AUTO_CHNL_SUPPORT(pAdapter))
		{
			pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_AUTO_CHNL_SEL, (pu1Byte)(&uAutoChnl));
			uOpChannel = P2PIsSocialChannel(uAutoChnl)? uAutoChnl : P2P_DEFAULT_OPERATING_CHANNEL;			
			pDefaultAdapter->MgntInfo.AutoChnlSel.AutoChnlNumberSelected= uAutoChnl;		
			RT_TRACE(COMP_P2P, DBG_LOUD, ("===> [ACS] N63CResetWifiDirectPorts(): uOpChannel(%d)\n", uOpChannel));
		}		
#endif		
	}	

	if(P2PIsSocialChannel(uOpChannel))
	{		
		uListenChannel = uOpChannel;
		if(GetFirstDevicePort(pAdapter))
		{
			GET_P2P_INFO(GetFirstDevicePort(pAdapter))->ListenChannel = uListenChannel;
		}
	}

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Set from Role=%d to Role=%d, listenChnl = %d, OpChnl = %d\n", pP2PInfo->Role, PortRole, uListenChannel, uOpChannel));
		
	if(PortRole == P2P_DEVICE)
	{	
		RT_TRACE((COMP_OID_QUERY | COMP_OID_SET), DBG_LOUD, ("(Win8 Reset to WFD Device Port)\n"));

		// Win8: Let the device port use the locally-adminitered MAC address ----------------------------------------------
		cpMacAddr(pAdapter->CurrentAddress, pAdapter->PermanentAddress);
		pAdapter->CurrentAddress[0] |= BIT1;
		RT_PRINT_ADDR((COMP_OID_QUERY | COMP_OID_SET), DBG_LOUD, "Device Port Address: ", pAdapter->CurrentAddress);
		// -------------------------------------------------------------------------------------------------------
			
		pNdis62Common->CurrentOpState = INIT_STATE;

		if(ResetLevel == RESET_LEVEL_FULL)
		{
			MgntActSet_ApType(pAdapter, FALSE);
			N62CResetAPVariables(pAdapter, FALSE);
		}
		
		// Stop the Device Port mode: Last 3 parameters are don't care
		MgntActSet_P2PMode(pAdapter, FALSE, FALSE, 0, 0, 0);
		// Start the Device Port mode
		MgntActSet_P2PMode(pAdapter, TRUE, FALSE, uListenChannel, uOpChannel, P2P_DEFAULT_GO_INTENT);
		
		// Win8: Let the device port do not conduct the extended listening.
		//	+ When the OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY is issued, start listening.
		pP2PInfo->ExtListenTimingPeriod = 0;
		pP2PInfo->ExtListenTimingDuration = 0;
		RT_TRACE((COMP_OID_QUERY | COMP_OID_SET), DBG_LOUD, ("In Win8, the device port and the extended listening are not started !\n"));

	}
	else if(PortRole == P2P_GO || PortRole == P2P_CLIENT)
	{
		pDevicePort = GetFirstDevicePort(pAdapter);
		if(pDevicePort == NULL)
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: pDevicePort == NULL \n", __FUNCTION__));
			return NDIS_STATUS_FAILURE;
		}


		// Wifi Direct Role Port is Open --------------------------------------------
		//DM_DIG_WifiDirectBoostStart(GetDefaultAdapter(pAdapter));
		// --------------------------------------------------------------------

		if(PortRole == P2P_GO)
		{
			pP2PDevInfo = (PP2P_INFO)(pDevicePort->MgntInfo.pP2PInfo);
			
			if(pP2PDevInfo != NULL)				
			{
				RT_TRACE((COMP_OID_QUERY | COMP_OID_SET|COMP_P2P), DBG_LOUD, ("(Win8 Reset to WFD Group Owner Port): Device port State(%d)\n", pP2PDevInfo->State));

				if((pP2PDevInfo->State == P2P_STATE_INVITATION_REQ_SEND) &&
					(pP2PDevInfo->State != P2P_STATE_INVITATION_RSP_WAIT))
				{
					if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
					{
						//<Roger_TODO>: Should we need to resume DIG here?
					}
				}
			}


			// Reset the AP parameters 
			pNdis62Common->CurrentOpState = INIT_STATE;

			if(ResetLevel == RESET_LEVEL_FULL)
			{
				N62CResetAPVariables(pAdapter, TRUE);
			}
			{
					HAL_AP_IBSS_INT_MODE	intMode = HAL_AP_IBSS_INT_DISABLE;
					pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_AP_IBSS_INTERRUPT, (pu1Byte)&intMode);
			}
			// Restart the Role Port mode: Last 3 parameters are don't care
			MgntActSet_P2PMode(pAdapter, FALSE, FALSE, 0, 0, 0);
			MgntActSet_P2PMode(pAdapter, TRUE, FALSE, uListenChannel, uOpChannel, P2P_DEFAULT_GO_INTENT);	

			pP2PInfo->Role = P2P_GO;
			pP2PInfo->State = P2P_STATE_OPERATING;
	
			// Clause 3.1.4.3: Group Formation bit in the P2P Group Cap shall be set to 1 until Provisioning succeeds. 
			pP2PInfo->GroupCapability |= gcGroupFormation;
		}
		else if(PortRole == P2P_CLIENT)
		{
			RT_TRACE((COMP_OID_QUERY | COMP_OID_SET), DBG_LOUD, ("(Win8 Reset to WFD Client Port)\n"));

			// Reset the STA parameters
			pNdis62Common->CurrentOpState=INIT_STATE;

			if(ResetLevel == RESET_LEVEL_FULL)
			{
				MgntActSet_ApType(pAdapter, FALSE);
				N62CResetAPVariables(pAdapter, FALSE);
			}

			// Restart the Role Port mode: Last 3 parameters are don't care
			MgntActSet_P2PMode(pAdapter, FALSE, FALSE, 0, 0, 0);
			MgntActSet_P2PMode(pAdapter, TRUE, FALSE, uListenChannel, uOpChannel, P2P_DEFAULT_GO_INTENT);
				
			pP2PInfo->Role = P2P_CLIENT;
			pP2PInfo->State = P2P_STATE_OPERATING;
		}

		// Copy the scan list from the device port
		// Prefast warning C28182: Dereferencing NULL pointer.
		if (GET_P2P_INFO(pDevicePort) != NULL)
		{
			P2PScanListCopy(
				pP2PInfo->ScanList,
				&pP2PInfo->ScanListSize,
				GET_P2P_INFO(pDevicePort)->ScanList,
				GET_P2P_INFO(pDevicePort)->ScanListSize
				);
		}
		
	}
	else
	{
		RT_TRACE_F((COMP_OID_QUERY | COMP_OID_SET | COMP_P2P), DBG_LOUD, ("[WARNING] Unknown P2P_ROLE: PortRole: %d\n", PortRole));
		return NDIS_STATUS_FAILURE;
	}

	FunctionOut(COMP_P2P);
	
	return NDIS_STATUS_SUCCESS;
}

//======================================================================================
// Private Functions
//======================================================================================

// Compute the required memory for representing the device list in the form of DOT11_WFD_DEVICE_ENTRY
static u4Byte
N63CComputeRequiredDeviceEntriesSize(
	IN PP2P_INFO pP2PInfo
)
{
	u4Byte i = 0;
	u4Byte requiredMemory = 0;
	
	// Skip the MAC header and the first three fields (non-IE) in the beacon and the probe response
	u4Byte skippedBytes = sMacHdrLng + 12;	
	
	for(i = 0; i < pP2PInfo->DeviceListForQuery.uNumberOfDevices; i++)
	{
		requiredMemory += sizeof(DOT11_WFD_DEVICE_ENTRY);	// Entry Size

		// Only consider the valid packet : Beacon
		if(pP2PInfo->DeviceListForQuery.DeviceEntry[i].BeaconPacket.Length >= skippedBytes)
		{
			requiredMemory += pP2PInfo->DeviceListForQuery.DeviceEntry[i].BeaconPacket.Length - skippedBytes;
		}

		// Only consider the valid packet : ProbeResponse
		if(pP2PInfo->DeviceListForQuery.DeviceEntry[i].ProbeResponsePacket.Length >= skippedBytes)
		{
			requiredMemory += pP2PInfo->DeviceListForQuery.DeviceEntry[i].ProbeResponsePacket.Length - skippedBytes;
		}
	}

	RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Required Memory Size %d\n", __FUNCTION__, requiredMemory));

	return requiredMemory;
}

static VOID
N63CConstruct_DOT11_WFD_DEVICE_ENTRY(
	IN PP2P_INFO pP2PInfo,
	IN	PP2P_DEVICE_LIST_ENTRY p2pEntry,
	OUT PDOT11_WFD_DEVICE_ENTRY  wfdEntry,
	OUT pu4Byte	bytesWritten
)
{
#if 0    
    typedef struct _DOT11_WFD_DEVICE_ENTRY 
    {
        ULONG uPhyId;
        DOT11_BSS_ENTRY_PHY_SPECIFIC_INFO PhySpecificInfo;
        DOT11_MAC_ADDRESS dot11BSSID;
        DOT11_BSS_TYPE dot11BSSType;
        DOT11_MAC_ADDRESS TransmitterAddress;
        LONG lRSSI;
        ULONG uLinkQuality;
        USHORT usBeaconPeriod;
        ULONGLONG ullTimestamp;
        ULONGLONG ullBeaconHostTimestamp;
        ULONGLONG ullProbeResponseHostTimestamp;
        USHORT usCapabilityInformation;
        ULONG uBeaconIEsOffset;
        ULONG uBeaconIEsLength;	// Can be 0
        ULONG uProbeResponseIEsOffset;
        ULONG uProbeResponseIEsLength;	// Can be 0
    } DOT11_WFD_DEVICE_ENTRY, *PDOT11_WFD_DEVICE_ENTRY;
#endif

	pu1Byte header = NULL;
	PADAPTER pAdapter = pP2PInfo->pAdapter;

	// Skip the MAC header and the first three fields (non-IE) in the beacon and the probe response
	u4Byte skippedBytes = sMacHdrLng + 12;

	// Select the most recent packet
	if(p2pEntry->ProbeResponseHostTimestamp > p2pEntry->BeaconHostTimestamp)
	{
		header = p2pEntry->ProbeResponsePacket.Buffer;
	}
	else
	{
		header = p2pEntry->BeaconPacket.Buffer;
	}

	if(header == NULL) 
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s() : Not a Good Header!\n", __FUNCTION__));
		return;
	}

	// Clear the structure
	PlatformZeroMemory(wfdEntry, sizeof(DOT11_WFD_DEVICE_ENTRY));

	wfdEntry->uPhyId = N6CQuery_DOT11_OPERATING_PHYID(GetDefaultAdapter(pP2PInfo->pAdapter));

	wfdEntry->PhySpecificInfo.uChCenterFrequency = MgntGetChannelFrequency(p2pEntry->ChannelNumber);

	GET_80211_HDR_ADDRESS3(header, wfdEntry->dot11BSSID);
	
	wfdEntry->dot11BSSType = dot11_BSS_type_infrastructure;

	GET_80211_HDR_ADDRESS2(header, wfdEntry->TransmitterAddress);

	wfdEntry->lRSSI = ( (p2pEntry->SignalStrength+1)>>1 ) - 95;	// Trick from N6CTranslateRtBssToDot11Bss

	wfdEntry->uLinkQuality = p2pEntry->SignalStrength;			// Trick from N6CTranslateRtBssToDot11Bss

	wfdEntry->usBeaconPeriod = GET_BEACON_PROBE_RSP_BEACON_INTERVAL(header);

	wfdEntry->ullTimestamp = ((u8Byte) GET_BEACON_PROBE_RSP_TIME_STAMP_HIGH(header) << 32) +  GET_BEACON_PROBE_RSP_TIME_STAMP_LOW(header);

	wfdEntry->usCapabilityInformation = GET_BEACON_PROBE_RSP_CAPABILITY_INFO(header);


	//
	// <Roger_Notes>
	// We have to take care of ullBeaconHostTimestamp and ullProbeResponseHostTimestamp in DOT11_WFD_DEVICE_ENTRY structure
	// for OID_DOT11_WFD_ENUM_DEVICE_LIST request. Micfosoft would check this indicated probe response/beacon timestamp from ver9477.
	// The unit in ullBeaconHostTimestamp and ullProbeResponseHostTimestamp must be 100-nanosecond, 2013.08.21.
	//
	
	// Beacon Related

	if(RUNTIME_OS_WIN_FROM_WINBLUE(pAdapter))
	wfdEntry->ullBeaconHostTimestamp = p2pEntry->BeaconHostTimestamp*10;// Convert to 100-nanosecond units
	else
		wfdEntry->ullBeaconHostTimestamp = p2pEntry->BeaconHostTimestamp;
	wfdEntry->uBeaconIEsOffset = sizeof(DOT11_WFD_DEVICE_ENTRY);

	// Only consider the valid packet 
	if(p2pEntry->BeaconPacket.Length >= skippedBytes)
	{
		wfdEntry->uBeaconIEsLength = p2pEntry->BeaconPacket.Length - skippedBytes;

		PlatformMoveMemory(
			(pu1Byte) wfdEntry + wfdEntry->uBeaconIEsOffset, 
			(pu1Byte) p2pEntry->BeaconPacket.Buffer + skippedBytes,
			wfdEntry->uBeaconIEsLength
		);
	}
	else
		wfdEntry->uBeaconIEsOffset = 0;

	// Probe Response Related
	
	if(RUNTIME_OS_WIN_FROM_WINBLUE(pAdapter))
	wfdEntry->ullProbeResponseHostTimestamp = p2pEntry->ProbeResponseHostTimestamp*10;// Convert to 100-nanosecond units
	else
		wfdEntry->ullProbeResponseHostTimestamp = p2pEntry->ProbeResponseHostTimestamp;	
	wfdEntry->uProbeResponseIEsOffset = sizeof(DOT11_WFD_DEVICE_ENTRY) + wfdEntry->uBeaconIEsLength;
      		
	// Only consider the valid packet 
	if(p2pEntry->ProbeResponsePacket.Length  >= skippedBytes)
	{
		wfdEntry->uProbeResponseIEsLength =p2pEntry->ProbeResponsePacket.Length - skippedBytes;

		PlatformMoveMemory(
			(pu1Byte) wfdEntry + wfdEntry->uProbeResponseIEsOffset, 
			(pu1Byte) p2pEntry->ProbeResponsePacket.Buffer + skippedBytes,
			wfdEntry->uProbeResponseIEsLength
		);

	}	
	else
		wfdEntry->uProbeResponseIEsOffset = 0;

	// Return the size of the total DOT11_WFD_DEVICE_ENTRY
	*bytesWritten = sizeof(DOT11_WFD_DEVICE_ENTRY) + wfdEntry->uBeaconIEsLength + wfdEntry->uProbeResponseIEsLength;
	
}


static VOID
DumpN63CConstruct_DOT11_WFD_DEVICE_ENTRY(
	PDOT11_WFD_DEVICE_ENTRY  wfdEntry,
	u4Byte length
)
{
	
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "WFD_DEVICE_ENTRY:===========================================\n",
			wfdEntry, sizeof(DOT11_WFD_DEVICE_ENTRY)
		);

	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "wfdEntry->dot11BSSID:", wfdEntry->dot11BSSID);
	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "wfdEntry->TransmitterAddress:", wfdEntry->TransmitterAddress);		
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("(wfdEntry->uBeaconIEsOffset, wfdEntry->uBeaconIEsLength) = (0x%X, 0x%X)\n", wfdEntry->uBeaconIEsOffset, wfdEntry->uBeaconIEsLength));
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Beacon IE:-----------\n", ((pu1Byte)wfdEntry) + wfdEntry->uBeaconIEsOffset, wfdEntry->uBeaconIEsLength);

	RT_TRACE(COMP_P2P, DBG_LOUD, ("(wfdEntry->uProbeResponseIEsOffset, wfdEntry->uProbeResponseIEsLength) = (0x%X, 0x%X)\n", wfdEntry->uProbeResponseIEsOffset, wfdEntry->uProbeResponseIEsLength));
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ProbeResponse IE:-----------\n", ((pu1Byte)wfdEntry) + wfdEntry->uProbeResponseIEsOffset, wfdEntry->uProbeResponseIEsLength);
}
	
	
static VOID
N63CIndicateDeviceDiscoveryComplete(
	PP2P_INFO pP2PInfo
)
{

#if 0
    #define DOT11_WFD_DISCOVER_COMPLETE_PARAMETERS_REVISION_1    1
    #define DOT11_WFD_DISCOVER_COMPLETE_MAX_LIST_SIZE   128

    typedef struct _DOT11_WFD_DISCOVER_COMPLETE_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        NDIS_STATUS Status;
        ULONG uNumOfEntries;
        ULONG uTotalNumOfEntries;
        ULONG uListOffset;
        ULONG uListLength;
    } DOT11_WFD_DISCOVER_COMPLETE_PARAMETERS, * PDOT11_WFD_DISCOVER_COMPLETE_PARAMETERS;
#endif

	u4Byte i = 0;
	u4Byte bytesWritten = 0;
	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_WFD_DISCOVER_COMPLETE_PARAMETERS	pParameters = NULL;
	PDOT11_WFD_DEVICE_ENTRY  wfdEntry = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
		
	// Compute the requried memory length ------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_WFD_DISCOVER_COMPLETE_PARAMETERS); 		// Parameter Structure Size
	mbObject.Length += N63CComputeRequiredDeviceEntriesSize(pP2PInfo);
	//-------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_WFD_DISCOVER_COMPLETE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_WFD_DISCOVER_COMPLETE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_WFD_DISCOVER_COMPLETE_PARAMETERS_REVISION_1,
			sizeof(DOT11_WFD_DISCOVER_COMPLETE_PARAMETERS)
		);

	pParameters->Status = NDIS_STATUS_SUCCESS;
	pParameters->uNumOfEntries = pP2PInfo->DeviceListForQuery.uNumberOfDevices;
	pParameters->uTotalNumOfEntries = pP2PInfo->DeviceListForQuery.uNumberOfDevices;

	pParameters->uListOffset = sizeof(DOT11_WFD_DISCOVER_COMPLETE_PARAMETERS);

	bytesWritten = 0;
	wfdEntry = (PDOT11_WFD_DEVICE_ENTRY)((pu1Byte) mbObject.Buffer + pParameters->uListOffset);

	for(i = 0; i < pP2PInfo->DeviceListForQuery.uNumberOfDevices; i++)
	{
		wfdEntry = (PDOT11_WFD_DEVICE_ENTRY)((pu1Byte) wfdEntry + bytesWritten);

		N63CConstruct_DOT11_WFD_DEVICE_ENTRY(	
				pP2PInfo,
				&pP2PInfo->DeviceListForQuery.DeviceEntry[i],
				wfdEntry,
				&bytesWritten
			);

		// This one may have crashed some mark this temporailiy
		//DumpN63CConstruct_DOT11_WFD_DEVICE_ENTRY(
		//		wfdEntry,
		//		bytesWritten
		//	);

			pParameters->uListLength += bytesWritten;
		}

	// Assertion --------------------------------------------------------------------------------------------
	if(pParameters->uListLength != N63CComputeRequiredDeviceEntriesSize(pP2PInfo))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: N63CComputeRequiredMemoryForDeviceList value mismatch!\n", __FUNCTION__));
	}
	//-----------------------------------------------------------------------------------------------------
	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_DISCOVER_COMPLETE, 
			mbObject.Buffer, 
			mbObject.Length
		);

	//DumpDOT11_WFD_DISCOVER_COMPLETE_PARAMETERS(mbObject);


	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}


static VOID
N63CIndicateInvitationRequestSendComplete(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0
    #define DOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS_REVISION_1    1
	
    typedef struct _DOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS PeerDeviceAddress;
        DOT11_MAC_ADDRESS ReceiverAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        NDIS_STATUS Status;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS, * PDOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS;
 #endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS); 	// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS_REVISION_1,
			sizeof(DOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS)
		);

	if(pEventData->rtStatus == RT_STATUS_FAILURE)
	{		
		pParameters->Status = NDIS_STATUS_FAILURE;
	}
	else
	{
		header = pEventData->Packet.Buffer;

		GET_80211_HDR_ADDRESS1(header, pParameters->PeerDeviceAddress);

		GET_80211_HDR_ADDRESS1(header, pParameters->ReceiverAddress);
		
		pParameters->DialogToken = *(header + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);
		
		pParameters->Status = NDIS_STATUS_SUCCESS;

		pParameters->uIEsOffset = sizeof(DOT11_INVITATION_REQUEST_SEND_COMPLETE_PARAMETERS);

		// Only the valid packet 
		if(pEventData->Packet.Length >= skippedBytes)
		{
			pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
		}

		PlatformMoveMemory(
				(pu1Byte) pParameters + pParameters->uIEsOffset, 
				header + skippedBytes, 
				pParameters->uIEsLength
			);
	}

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_INVITATION_REQUEST_SEND_COMPLETE, 
			mbObject.Buffer, 
			mbObject.Length
		);

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "mbObject.Buffer\n", mbObject.Buffer, mbObject.Length);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}

static VOID
N63CIndicateReceivedInvitationRequest(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0
    #define DOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS_REVISION_1    1
    typedef struct _DOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS TransmitterDeviceAddress;
        DOT11_MAC_ADDRESS BSSID;
        DOT11_DIALOG_TOKEN DialogToken;
        PVOID RequestContext;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS, * PDOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS;
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS_REVISION_1,
			sizeof(DOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS)
		);

	header = pEventData->Packet.Buffer;
		
	GET_80211_HDR_ADDRESS2(header, pParameters->TransmitterDeviceAddress);
	
	GET_80211_HDR_ADDRESS3(header, pParameters->BSSID);

	pParameters->DialogToken = *(header + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);

	pParameters->RequestContext = NULL;
	  
	pParameters->uIEsOffset = sizeof(DOT11_RECEIVED_INVITATION_REQUEST_PARAMETERS);

	// Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
	}	
		
	PlatformMoveMemory(
			(pu1Byte) pParameters + pParameters->uIEsOffset, 
			header + skippedBytes, 
			pParameters->uIEsLength
		);
	
	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_RECEIVED_INVITATION_REQUEST, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}


static VOID
N63CIndicateInvitationResponseSendComplete(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0
    #define DOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS_REVISION_1    1
    typedef struct _DOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS ReceiverDeviceAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        NDIS_STATUS Status;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS, * PDOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS;
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS_REVISION_1,
			sizeof(DOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS)
		);

	if(pEventData->rtStatus == RT_STATUS_FAILURE)
	{		
		pParameters->Status = NDIS_STATUS_FAILURE;
	}
	else
	{
		header = pEventData->Packet.Buffer;


		GET_80211_HDR_ADDRESS1(header, pParameters->ReceiverDeviceAddress);

		pParameters->DialogToken = pP2PInfo->ProvisionResponseDialogToken;
	
		pParameters->Status = NDIS_STATUS_SUCCESS;

		pParameters->uIEsOffset = sizeof(DOT11_INVITATION_RESPONSE_SEND_COMPLETE_PARAMETERS);

		// Only the valid packet 
		if(pEventData->Packet.Length >= skippedBytes)
		{
			pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
		}
		
		PlatformMoveMemory(
				(pu1Byte) pParameters + pParameters->uIEsOffset, 
				header + skippedBytes, 
				pParameters->uIEsLength
			);
	
	}

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_INVITATION_RESPONSE_SEND_COMPLETE, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}


static VOID
N63CIndicateReceivedInvitationResponse(
	PP2P_INFO pP2PInfo,
	const N63C_SEND_ACTION_CTX *ctx,
	u4Byte frameLen,
	u1Byte *frameBuf
)
{
#if 0
    #define DOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS_REVISION_1    1
    typedef struct _DOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS TransmitterDeviceAddress;
        DOT11_MAC_ADDRESS BSSID;
        DOT11_DIALOG_TOKEN DialogToken;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS, * PDOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS;
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS	 pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;

	FunctionIn(COMP_OID_SET);
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(frameLen >= skippedBytes)
	{
		mbObject.Length += frameLen - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS_REVISION_1,
			sizeof(DOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS)
		);

	header = frameBuf;
		
	GET_80211_HDR_ADDRESS2(header, pParameters->TransmitterDeviceAddress);
	
	GET_80211_HDR_ADDRESS3(header, pParameters->BSSID);

	pParameters->DialogToken = ctx->token;
	  
	pParameters->uIEsOffset = sizeof(DOT11_RECEIVED_INVITATION_RESPONSE_PARAMETERS);

	// Only the valid packet 
	if(frameLen >= skippedBytes)
	{
		pParameters->uIEsLength = frameLen - skippedBytes;
	}	
		
	PlatformMoveMemory(
			(pu1Byte) pParameters + pParameters->uIEsOffset, 
			header + skippedBytes, 
			pParameters->uIEsLength
		);

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_RECEIVED_INVITATION_RESPONSE, 
			mbObject.Buffer, 
			mbObject.Length
		);

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "mbObject.Buffer\n", mbObject.Buffer, mbObject.Length);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

	FunctionOut(COMP_OID_SET);

}

static VOID
N63CIndicateProvisionDiscoveryRequestSendComplete(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0
    #define DOT11_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS_REVISION_1    1

    
    typedef struct _DOT11_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS PeerDeviceAddress;
        DOT11_MAC_ADDRESS ReceiverAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        NDIS_STATUS Status;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS, * PDOT11_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS;
 #endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += DOT11_SIZEOF_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS_REVISION_1;  		// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!: %d\n", __FUNCTION__, mbObject.Length));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, DOT11_SIZEOF_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS_REVISION_1);
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS_REVISION_1,
			DOT11_SIZEOF_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS_REVISION_1
		);

	if(pEventData->rtStatus == RT_STATUS_FAILURE)
	{		
		pParameters->Status = NDIS_STATUS_FAILURE;
	}
	else
	{
		header = pEventData->Packet.Buffer;

		GET_80211_HDR_ADDRESS1(header, pParameters->PeerDeviceAddress);

		GET_80211_HDR_ADDRESS1(header, pParameters->ReceiverAddress);
		
		pParameters->DialogToken = *(header + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);
		
		pParameters->Status = NDIS_STATUS_SUCCESS;

		pParameters->uIEsOffset = sizeof(DOT11_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE_PARAMETERS);

		// Only the valid packet 
		if(pEventData->Packet.Length >= skippedBytes)
		{
			pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
		}

		PlatformMoveMemory(
				(pu1Byte) pParameters + pParameters->uIEsOffset, 
				header + skippedBytes, 
				pParameters->uIEsLength
			);
	}

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE, 
			mbObject.Buffer, 
			mbObject.Length
		);

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "mbObject.Buffer\n", mbObject.Buffer, mbObject.Length);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}


static VOID
N63CIndicateReceivedProvisionDiscoveryRequest(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0
    #define DOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS_REVISION_1    1
    
    typedef struct _DOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS TransmitterDeviceAddress;
        DOT11_MAC_ADDRESS BSSID;
        DOT11_DIALOG_TOKEN DialogToken;
        PVOID RequestContext;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS, * PDOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS;
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS_REVISION_1,
			sizeof(DOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS)
		);

	header = pEventData->Packet.Buffer;
		
	GET_80211_HDR_ADDRESS2(header, pParameters->TransmitterDeviceAddress);
	
	GET_80211_HDR_ADDRESS3(header, pParameters->BSSID);

	pParameters->DialogToken = *(header + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);

	pParameters->RequestContext = NULL;
	  
	pParameters->uIEsOffset = sizeof(DOT11_RECEIVED_PROVISION_DISCOVERY_REQUEST_PARAMETERS);

	// Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
	}	
		
	PlatformMoveMemory(
			(pu1Byte) pParameters + pParameters->uIEsOffset, 
			header + skippedBytes, 
			pParameters->uIEsLength
		);
	
	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_RECEIVED_PROVISION_DISCOVERY_REQUEST, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}

static VOID
N63CIndicateProvisionDiscoveryResponseSendComplete(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0
    #define DOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS_REVISION_1    1

    typedef struct _DOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS ReceiverDeviceAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        NDIS_STATUS Status;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS, * PDOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS;    
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS_REVISION_1,
			sizeof(DOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS)
		);

	if(pEventData->rtStatus == RT_STATUS_FAILURE)
	{		
		pParameters->Status = NDIS_STATUS_FAILURE;
	}
	else
	{
		header = pEventData->Packet.Buffer;


		GET_80211_HDR_ADDRESS1(header, pParameters->ReceiverDeviceAddress);

		pParameters->DialogToken = pP2PInfo->ProvisionResponseDialogToken;
	
		pParameters->Status = NDIS_STATUS_SUCCESS;

		pParameters->uIEsOffset = sizeof(DOT11_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE_PARAMETERS);

		// Only the valid packet 
		if(pEventData->Packet.Length >= skippedBytes)
		{
			pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
		}
		
		PlatformMoveMemory(
				(pu1Byte) pParameters + pParameters->uIEsOffset, 
				header + skippedBytes, 
				pParameters->uIEsLength
			);
	
	}

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}

static VOID
N63CIndicateReceivedProvisionDiscoveryResponse(
	PP2P_INFO pP2PInfo,
	const N63C_SEND_ACTION_CTX *ctx,
	u4Byte frameLen,
	u1Byte *frameBuf
	)
{
#if 0
    #define DOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS_REVISION_1    1
    typedef struct _DOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS TransmitterDeviceAddress;
        DOT11_MAC_ADDRESS BSSID;
        DOT11_DIALOG_TOKEN DialogToken;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS, * PDOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS;    
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;

	FunctionIn(COMP_OID_SET);
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(frameLen >= skippedBytes)
	{
		mbObject.Length += frameLen - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS_REVISION_1,
			sizeof(DOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS)
		);

	header = frameBuf;
		
	GET_80211_HDR_ADDRESS2(header, pParameters->TransmitterDeviceAddress);
	
	GET_80211_HDR_ADDRESS3(header, pParameters->BSSID);

	pParameters->DialogToken = ctx->token;
	  
	pParameters->uIEsOffset = sizeof(DOT11_RECEIVED_PROVISION_DISCOVERY_RESPONSE_PARAMETERS);

	// Only the valid packet 
	if(frameLen >= skippedBytes)
	{
		pParameters->uIEsLength = frameLen - skippedBytes;
	}	
		
	PlatformMoveMemory(
			(pu1Byte) pParameters + pParameters->uIEsOffset, 
			header + skippedBytes, 
			pParameters->uIEsLength
		);

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_RECEIVED_PROVISION_DISCOVERY_RESPONSE, 
			mbObject.Buffer, 
			mbObject.Length
		);

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "mbObject.Buffer\n", mbObject.Buffer, mbObject.Length);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

	FunctionOut(COMP_OID_SET);

}


static VOID
N63CIndicateNegotiationRequestSendComplete(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0
    #define DOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS_REVISION_1    1
    typedef struct _DOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS PeerDeviceAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        NDIS_STATUS Status;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS, * PDOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS;
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successful ------------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!: %u\n", __FUNCTION__, mbObject.Length));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS_REVISION_1,
			sizeof(DOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS)
		);

	if(pEventData->rtStatus == RT_STATUS_FAILURE)
	{		
		pParameters->Status = NDIS_STATUS_FAILURE;
	}
	else
	{
		header = pEventData->Packet.Buffer;


		GET_80211_HDR_ADDRESS1(header, pParameters->PeerDeviceAddress);
		
		pParameters->DialogToken = *(header + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);
	
		pParameters->Status = NDIS_STATUS_SUCCESS;

		pParameters->uIEsOffset = sizeof(DOT11_GO_NEGOTIATION_REQUEST_SEND_COMPLETE_PARAMETERS);

		// Only the valid packet 
		if(pEventData->Packet.Length >= skippedBytes)
		{
			pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
		}
		
		PlatformMoveMemory(
				(pu1Byte) pParameters + pParameters->uIEsOffset, 
				header + skippedBytes, 
				pParameters->uIEsLength
			);
	
	}

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_GO_NEGOTIATION_REQUEST_SEND_COMPLETE, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}

static VOID
N63CIndicateReceivedGONegotiationRequest(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0
    #define DOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS_REVISION_1    1
    typedef struct _DOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS PeerDeviceAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        PVOID RequestContext;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS, * PDOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS;    
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS_REVISION_1,
			sizeof(DOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS)
		);

	header = pEventData->Packet.Buffer;
		
	GET_80211_HDR_ADDRESS2(header, pParameters->PeerDeviceAddress);

	pParameters->DialogToken = *(header + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);
	  
	pParameters->uIEsOffset = sizeof(DOT11_RECEIVED_GO_NEGOTIATION_REQUEST_PARAMETERS);

	// Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
	}	
		
	PlatformMoveMemory(
			(pu1Byte) pParameters + pParameters->uIEsOffset, 
			header + skippedBytes, 
			pParameters->uIEsLength
		);

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_RECEIVED_GO_NEGOTIATION_REQUEST, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}


static VOID
N63CIndicateNegotiationResponseSendComplete(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0 
    #define DOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS_REVISION_1    1
    
    typedef struct _DOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS PeerDeviceAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        NDIS_STATUS Status;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS, * PDOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS;
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successful ------------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS_REVISION_1,
			sizeof(DOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS)
		);

	if(pEventData->rtStatus == RT_STATUS_FAILURE)
	{		
		pParameters->Status = NDIS_STATUS_FAILURE;
	}
	else
	{
		header = pEventData->Packet.Buffer;


		GET_80211_HDR_ADDRESS1(header, pParameters->PeerDeviceAddress);
		
		pParameters->DialogToken = *(header + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);
	
		pParameters->Status = NDIS_STATUS_SUCCESS;

		pParameters->uIEsOffset = sizeof(DOT11_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE_PARAMETERS);

		// Only the valid packet 
		if(pEventData->Packet.Length >= skippedBytes)
		{
			pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
		}
		
		PlatformMoveMemory(
				(pu1Byte) pParameters + pParameters->uIEsOffset, 
				header + skippedBytes, 
				pParameters->uIEsLength
			);
	
	}

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}


static VOID
N63CIndicateReceivedGONegotiationResponse(
	PP2P_INFO pP2PInfo,
	const N63C_SEND_ACTION_CTX *ctx,
	u4Byte frameLen,
	u1Byte *frameBuf
)
{
#if 0
    #define DOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS_REVISION_1    1

    typedef struct _DOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS PeerDeviceAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        PVOID ResponseContext;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS, * PDOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS;
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;

	FunctionIn(COMP_OID_SET);
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(frameLen >= skippedBytes)
	{
		mbObject.Length += frameLen - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successfull -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS_REVISION_1,
			sizeof(DOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS)
		);

	header = frameBuf;
		
	GET_80211_HDR_ADDRESS2(header, pParameters->PeerDeviceAddress);

	pParameters->DialogToken = ctx->token;
	  
	pParameters->uIEsOffset = sizeof(DOT11_RECEIVED_GO_NEGOTIATION_RESPONSE_PARAMETERS);

	// Only the valid packet 
	if(frameLen >= skippedBytes)
	{
		pParameters->uIEsLength = frameLen - skippedBytes;
	}	
		
	PlatformMoveMemory(
			(pu1Byte) pParameters + pParameters->uIEsOffset, 
			header + skippedBytes, 
			pParameters->uIEsLength
		);

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_RECEIVED_GO_NEGOTIATION_RESPONSE, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

	FunctionOut(COMP_OID_SET);

}


static VOID
N63CIndicateNegotiationConfirmSendComplete(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
#if 0
    #define DOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS_REVISION_1    1
    
    typedef struct _DOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS PeerDeviceAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        NDIS_STATUS Status;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS, * PDOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS;
#endif

	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	PP2P_EVENT_DATA pEventData = (PP2P_EVENT_DATA) pInformation->Buffer;
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(pEventData->Packet.Length >= skippedBytes)
	{
		mbObject.Length += pEventData->Packet.Length - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successful ------------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS_REVISION_1,
			sizeof(DOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS)
		);

	if(pEventData->rtStatus == RT_STATUS_FAILURE)
	{		
		pParameters->Status = NDIS_STATUS_FAILURE;
	}
	else
	{
		header = pEventData->Packet.Buffer;


		GET_80211_HDR_ADDRESS1(header, pParameters->PeerDeviceAddress);
		
		pParameters->DialogToken = *(header + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);
	
		pParameters->Status = NDIS_STATUS_SUCCESS;

		pParameters->uIEsOffset = sizeof(DOT11_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE_PARAMETERS);

		// Only the valid packet 
		if(pEventData->Packet.Length >= skippedBytes)
		{
			pParameters->uIEsLength = pEventData->Packet.Length - skippedBytes;
		}
		
		PlatformMoveMemory(
				(pu1Byte) pParameters + pParameters->uIEsOffset, 
				header + skippedBytes, 
				pParameters->uIEsLength
			);
	
	}

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_GO_NEGOTIATION_CONFIRMATION_SEND_COMPLETE, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}


static VOID
N63CIndicateReceivedGONegotiationConfirm(
	PP2P_INFO pP2PInfo,
	const N63C_SEND_ACTION_CTX *ctx,
	u4Byte frameLen,
	u1Byte *frameBuf
)
{
#if 0
    #define DOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS_REVISION_1    1    
    typedef struct _DOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS 
    {
        NDIS_OBJECT_HEADER Header;
        DOT11_MAC_ADDRESS PeerDeviceAddress;
        DOT11_DIALOG_TOKEN DialogToken;
        ULONG uIEsOffset;
        ULONG uIEsLength;
    } DOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS, * PDOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS;
#endif

    	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS	pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	pu1Byte header = NULL;
	
	// Skip the mac header (24) and the fields from Category to Dialog Token (8)
	u2Byte skippedBytes = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;

	FunctionIn(COMP_OID_SET);
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS);  		// Parameter Structure Size

	// + Only the valid packet 
	if(frameLen >= skippedBytes)
	{
		mbObject.Length += frameLen - skippedBytes;
	}
	//----------------------------------------------------------------------------------------------------------
	
	// This must be successful -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
			pParameters->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS_REVISION_1,
			sizeof(DOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS)
		);

	header = frameBuf;
		
	GET_80211_HDR_ADDRESS2(header, pParameters->PeerDeviceAddress);

	pParameters->DialogToken = ctx->token;
	  
	pParameters->uIEsOffset = sizeof(DOT11_RECEIVED_GO_NEGOTIATION_CONFIRMATION_PARAMETERS);

	// Only the valid packet 
	if(frameLen >= skippedBytes)
	{
		pParameters->uIEsLength = frameLen - skippedBytes;
	}	
		
	PlatformMoveMemory(
			(pu1Byte) pParameters + pParameters->uIEsOffset, 
			header + skippedBytes, 
			pParameters->uIEsLength
		);

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_RECEIVED_GO_NEGOTIATION_CONFIRMATION, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

	FunctionOut(COMP_OID_SET);

}

static VOID
N63CIndicateGOOperatingChannel(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pInformation
)
{
	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PDOT11_WFD_CHANNEL pParameters = NULL;
	MEMORY_BUFFER mbObject = {NULL, 0};
	
	// Compute the requried memory length --------------------------------------------------------------------------
	mbObject.Length += sizeof(DOT11_WFD_CHANNEL); 	// Parameter Structure Size

	// This must be successful -----------------------------------------------------------------------
	rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &mbObject.Buffer, mbObject.Length);
	if(rtStatus == RT_STATUS_FAILURE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		return;
	}
	//--------------------------------------------------------------------------------------------

	pParameters = (PDOT11_WFD_CHANNEL) mbObject.Buffer;
	PlatformZeroMemory(pParameters, sizeof(DOT11_WFD_CHANNEL));
	
	PlatformMoveMemory((pu1Byte)pParameters->CountryRegionString, pP2PInfo->CountryString, 3);
	pParameters->OperatingClass = pP2PInfo->RegulatoryClass;
	pParameters->ChannelNumber = pP2PInfo->OperatingChannel;

	// Indicate the status to the OS -------------------------------------------------------------	
	N6IndicateStatus(
			pP2PInfo->pAdapter, 
			NDIS_STATUS_DOT11_WFD_GROUP_OPERATING_CHANNEL, 
			mbObject.Buffer, 
			mbObject.Length
		);
	//---------------------------------------------------------------------------------------

	PlatformFreeMemory(mbObject.Buffer, mbObject.Length);

}

static NDIS_STATUS
N63CValidateOIDCorrectnessGetReturnStatus(
	OID_CORRECT_STATE	correctState, 
	MP_PORT_TYPE		portType,
	P2P_ROLE			portRole,
	MP_PORT_OP_STATE	portState
)
{
// 
// Return Value:
//		NDIS_STATUS_INVALID_STATE: 		That port can process that OID in other state
//		NDIS_STATUS_SUCCESS:			Correct port in the correct state
//

	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_STATE; 

	if(portType == EXT_P2P_DEVICE_PORT)
	{
		if(portState == INIT_STATE) 
		{
			if(correctState & STATE_DEVICE_INIT)
			{
				ndisStatus = NDIS_STATUS_SUCCESS;
			}
		}
	}
	else if(portType == EXT_P2P_ROLE_PORT)
	{
		if(portRole == P2P_GO)
		{
			if(portState == INIT_STATE)
			{
				if(correctState & STATE_GO_INIT)
				{
					ndisStatus = NDIS_STATUS_SUCCESS;
				}
				else if(correctState & STATE_GO_OP)
				{
					ndisStatus = NDIS_STATUS_INVALID_STATE;
				}
			}
			else if(portState == OP_STATE)
			{
				if(correctState & STATE_GO_OP)
				{
					ndisStatus = NDIS_STATUS_SUCCESS;
				}
				else if(correctState & STATE_GO_INIT)
				{
					ndisStatus = NDIS_STATUS_INVALID_STATE;
				}
			}
		}
		else if(portRole == P2P_CLIENT)
		{
			if(portState == INIT_STATE)
			{
				if(correctState & STATE_CLIENT_INIT)
				{
					ndisStatus = NDIS_STATUS_SUCCESS;
				}
				else if(correctState & STATE_CLIENT_OP)
				{
					ndisStatus = NDIS_STATUS_INVALID_STATE;
				}
			}
			else if(portState == OP_STATE)
			{
				if(correctState & STATE_CLIENT_OP)
				{
					ndisStatus = NDIS_STATUS_SUCCESS;
				}
				else if(correctState & STATE_CLIENT_INIT)
				{
					ndisStatus = NDIS_STATUS_INVALID_STATE;
				}
			}
		}
	}

	return ndisStatus;
}

static NDIS_STATUS
N63CValidateQueryOIDCorrectness(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS 		ndisStatus = NDIS_STATUS_INVALID_STATE; 
	NDIS_OID 			oid = NdisRequest->DATA.QUERY_INFORMATION.Oid;
	PADAPTER 			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER 			pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);
	PP2P_INFO			pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	
	MP_PORT_TYPE		portType = pTargetAdapter->pNdis62Common->PortType;
	P2P_ROLE			portRole	= pP2PInfo->Role;	
	MP_PORT_OP_STATE	portState = pTargetAdapter->pNdis62Common->CurrentOpState;
	
	switch (oid)
	{
		case OID_DOT11_WFD_ENUM_DEVICE_LIST:
		case OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY:
		case OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL:
		case OID_DOT11_WFD_GET_DIALOG_TOKEN:
			ndisStatus = N63CValidateOIDCorrectnessGetReturnStatus(
					STATE_DEVICE_INIT,
					portType, 
					portRole, 
					portState
				);
			break;
			
		default:
			ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}

	return ndisStatus;
}

static NDIS_STATUS
N63CValidateSetOIDCorrectness(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS 		ndisStatus = NDIS_STATUS_INVALID_STATE; 
	NDIS_OID 			oid = NdisRequest->DATA.QUERY_INFORMATION.Oid;
	PADAPTER 			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER 			pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);
	PP2P_INFO			pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	
	MP_PORT_TYPE		portType = pTargetAdapter->pNdis62Common->PortType;
	P2P_ROLE			portRole	= pP2PInfo->Role;	
	MP_PORT_OP_STATE	portState = pTargetAdapter->pNdis62Common->CurrentOpState;
	
	switch (oid)
	{
		case OID_DOT11_WFD_DEVICE_CAPABILITY:
		case OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST:
    		case OID_DOT11_WFD_ADDITIONAL_IE:
			ndisStatus = N63CValidateOIDCorrectnessGetReturnStatus(
					STATE_DEVICE_INIT | STATE_GO_INIT | STATE_GO_OP | STATE_CLIENT_INIT | STATE_CLIENT_OP,
					portType, 
					portRole, 
					portState
				);
			break;
			
		case OID_DOT11_WFD_GROUP_OWNER_CAPABILITY:
			ndisStatus = N63CValidateOIDCorrectnessGetReturnStatus(
					STATE_GO_INIT | STATE_GO_OP | STATE_DEVICE_INIT,
					portType, 
					portRole, 
					portState
				);
			break;
			
    		case OID_DOT11_WFD_DEVICE_INFO:
			ndisStatus = N63CValidateOIDCorrectnessGetReturnStatus(
					STATE_DEVICE_INIT | STATE_GO_INIT | STATE_CLIENT_INIT,
					portType, 
					portRole, 
					portState
				);
			break;

		case OID_DOT11_WFD_DISCOVER_REQUEST:
		case OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY:
		case OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL:
		case OID_DOT11_WFD_FLUSH_DEVICE_LIST:
		case OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST:
		case OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE:
    		case OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRMATION:
		case OID_DOT11_WFD_SEND_INVITATION_REQUEST:
		case OID_DOT11_WFD_SEND_INVITATION_RESPONSE:
		case OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST:
    		case OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE:
		case OID_DOT11_WFD_STOP_DISCOVERY:
			ndisStatus = N63CValidateOIDCorrectnessGetReturnStatus(
					STATE_DEVICE_INIT,
					portType, 
					portRole, 
					portState
				);
			break;
			
		case OID_DOT11_WFD_DESIRED_GROUP_ID:
			ndisStatus = N63CValidateOIDCorrectnessGetReturnStatus(
					STATE_GO_INIT | STATE_CLIENT_INIT,
					portType, 
					portRole, 
					portState
				);
			break;
				
		case OID_DOT11_WFD_START_GO_REQUEST:
		case OID_DOT11_WFD_GROUP_START_PARAMETERS:
			ndisStatus = N63CValidateOIDCorrectnessGetReturnStatus(
					STATE_GO_INIT,
					portType, 
					portRole, 
					portState
				);
			break;
			
    		case OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST:
		case OID_DOT11_WFD_GROUP_JOIN_PARAMETERS:
			ndisStatus = N63CValidateOIDCorrectnessGetReturnStatus(
					STATE_CLIENT_INIT,
					portType, 
					portRole, 
					portState
				);
			break;
			
		case OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST:
			ndisStatus = N63CValidateOIDCorrectnessGetReturnStatus(
					STATE_CLIENT_OP,
					portType, 
					portRole, 
					portState
				);
			break;

		default:
			ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
			break;
	}

	return ndisStatus;
}

static NDIS_STATUS
N63CValidateMethodOIDCorrectness(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	// Currently, the driver does not accept the query call based on the Microsoft Wi-Fi Direct Spec v1.4
	UNREFERENCED_PARAMETER(pAdapter);
	UNREFERENCED_PARAMETER(NdisRequest);

	return NDIS_STATUS_NOT_SUPPORTED;
}

// Use this function to check the correctness of each OID issue based on the Mircosoft Wi-Fi Direct Spec v1.1
static NDIS_STATUS
N63CValidateOIDCorrectness(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS 		ndisStatus = NDIS_STATUS_INVALID_STATE; 
	PADAPTER 			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER 			pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);
	PP2P_INFO			pP2PInfo = GET_P2P_INFO(pTargetAdapter);

	MP_PORT_TYPE		portType = pTargetAdapter->pNdis62Common->PortType;
	P2P_ROLE			portRole	= pP2PInfo->Role;	
	MP_PORT_OP_STATE	portState = pTargetAdapter->pNdis62Common->CurrentOpState;

	RT_TRACE(COMP_OID_SET | COMP_OID_QUERY | COMP_P2P, DBG_LOUD, 
		("MP_PORT_TYPE: %d, P2P_ROLE: %d, MP_PORT_OP_STATE: %d\n", portType, portRole, portState));	
	
	do
	{
		// Currently, only handle these OIDs in the Wi-Fi Driect ports (Device Port or Role Port) since these ports are the upmost port type
		if(portType != EXT_P2P_DEVICE_PORT && portType != EXT_P2P_ROLE_PORT)
		{
			ndisStatus = NDIS_STATUS_INVALID_STATE;
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N63CValidateQueryOIDCorrectness(pAdapter, NdisRequest);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63CValidateSetOIDCorrectness(pAdapter, NdisRequest);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = N63CValidateMethodOIDCorrectness(pAdapter, NdisRequest);
				break;
	
			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
		}
	}while(FALSE);

	return ndisStatus;
}


// Set operation for OID_DOT11_WFD_DEVICE_CAPABILITY
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_DEVICE_CAPABILITY(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);

	// OS-given structure
	PDOT11_WFD_DEVICE_CAPABILITY_CONFIG deviceCapability = 
		(PDOT11_WFD_DEVICE_CAPABILITY_CONFIG) InformationBuffer;

	#if 0	// Implementation Status
	typedef struct _DOT11_WFD_DEVICE_CAPABILITY_CONFIG 
	{
    		NDIS_OBJECT_HEADER Header;						// SKIP
    		BOOLEAN bServiceDiscoveryEnabled;					// OK
    		BOOLEAN bClientDiscoverabilityEnabled;				// OK
    		BOOLEAN bConcurrentOperationSupported;				// OK
    		BOOLEAN bInfrastructureManagementEnabled;			// OK
    		BOOLEAN bDeviceLimitReached;						// OK
    		BOOLEAN bInvitationProcedureEnabled;				// OK
    		ULONG WPSVersionsEnabled;							// TODO
    	} DOT11_WFD_DEVICE_CAPABILITY_CONFIG, * PDOT11_WFD_DEVICE_CAPABILITY_CONFIG;
	#endif

	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;
	
	// Set the device capability
	pP2PInfo->DeviceCapability = 
		0x00 | 
		(deviceCapability->bServiceDiscoveryEnabled 			? dcServiceDiscovery : 0) 			|
		(deviceCapability->bClientDiscoverabilityEnabled 		? dcP2PClientDiscoverability : 0) 		|
    		(deviceCapability->bConcurrentOperationSupported 		? dcConcurrentOperation : 0) 		|
    		(deviceCapability->bInfrastructureManagementEnabled 	? dcP2PInfrastructureManaged : 0)	|
    		(deviceCapability->bDeviceLimitReached 				? dcP2PDeviceLimit : 0)				|
    		(deviceCapability->bInvitationProcedureEnabled 		? dcP2PInvitationProcedure : 0)		;


	// Currently, do not know how to implement this -------
	// 	Represent the WPS versions that are currently enabled for the Wi-Fi Direct Device
	RT_TRACE(COMP_P2P, DBG_LOUD, ("deviceCapability->WPSVersionsEnabled: %d\n", deviceCapability->WPSVersionsEnabled));
	
	#if 0	// Possible Solution
	Currently, this may be ignored since our driver does not necessarily process the WPS IE.
	#endif
	// ---------------------------------------------
		
	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_GROUP_OWNER_CAPABILITY
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_GROUP_OWNER_CAPABILITY(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	P2P_ROLE		portRole	= pP2PInfo->Role;
	
	// OS-given structure
	PDOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG groupCapability = 
			(PDOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG) InformationBuffer;

	#if 0	// Implementation Status
    	typedef struct _DOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG 
   	{
    		NDIS_OBJECT_HEADER Header;				// SKIP
      		BOOLEAN bPersistentGroupEnabled;			// OK
    		BOOLEAN bIntraBSSDistributionSupported;		// OK
    		BOOLEAN bCrossConnectionSupported;		// OK
    		BOOLEAN bPersistentReconnectSupported;		// OK
    		BOOLEAN bGroupFormationEnabled;			// OK
    		ULONG uMaximumGroupLimit;					// TODO
    	} DOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG, *PDOT11_WFD_GROUP_OWNER_CAPABILITY_CONFIG;
	#endif
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	// Set the group capability
	pP2PInfo->GroupCapability = 
		0x00 | 
		(portRole == P2P_GO 											? gcP2PGroupOwner : 0) 		|
		(groupCapability->bPersistentGroupEnabled 						? gcPersistentP2PGroup : 0) 		|
    		(P2PClientInfoGetCount(pP2PInfo) >= P2P_MAX_P2P_CLIENT 		? gcP2PGroupLimit : 0) 			|
    		(groupCapability->bIntraBSSDistributionSupported 					? gcIntraBSSDistribution : 0)		|
    		(groupCapability->bCrossConnectionSupported 						? gcCrossConnection : 0)		|
    		(groupCapability->bPersistentReconnectSupported 					? gcPersistentReconnect : 0)	|
		(groupCapability->bGroupFormationEnabled 						? gcGroupFormation : 0)		;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: pP2PInfo->GroupCapability: 0x%0X\n", __FUNCTION__, pP2PInfo->GroupCapability));
	
	// Currently, do not know how to implement this -------
	//	We use the P2P_MAX_P2P_CLIENT macro, while the OS uses the following statement
	groupCapability->uMaximumGroupLimit;

	#if 0	// Possible Solution
	Since our driver uses the macro P2P_MAX_P2P_CLIENT to set the maximum value, we may be able to omit this setting.
	The default value will be set in the DOT11_WFD_ATTRIBUTES.uGORoleClientTableSize during the initialization.
	#endif
	
	// ---------------------------------------------
		
	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_DEVICE_INFO
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_DEVICE_INFO(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	
	// OS-given structure
	PDOT11_WFD_DEVICE_INFO deviceInfo = 	
		(PDOT11_WFD_DEVICE_INFO) InformationBuffer;

	#if 0	// Implementation Status
	typedef struct _DOT11_WFD_DEVICE_INFO 
    	{
      		NDIS_OBJECT_HEADER Header;					// SKIP
       	DOT11_MAC_ADDRESS DeviceAddress;			// OK
       	USHORT ConfigMethods;						// OK
       	DOT11_WFD_DEVICE_TYPE PrimaryDeviceType;		// OK
       	DOT11_WPS_DEVICE_NAME DeviceName;			// OK
	} DOT11_WFD_DEVICE_INFO, *PDOT11_WFD_DEVICE_INFO;
	#endif
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	// Start to set the device information
	
	// Set the device address: Should we set the interface address based on the role of the port?
	cpMacAddr(pP2PInfo->DeviceAddress, deviceInfo->DeviceAddress);
	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Set pP2PInfo->DeviceAddress: ", pP2PInfo->DeviceAddress);
		
	// Set the config method for WPS
	pP2PInfo->WpsAttributes.ConfigMethod = deviceInfo->ConfigMethods;
	RT_TRACE(COMP_P2P, DBG_LOUD, ("pP2PInfo->WpsAttributes.ConfigMethod: 0x%X\n", pP2PInfo->WpsAttributes.ConfigMethod));
	
	// Set the primary device type for WPS	
	pP2PInfo->WpsAttributes.PrimaryDeviceType.CategoryId 		= deviceInfo->PrimaryDeviceType.CategoryID;
	RT_TRACE(COMP_P2P, DBG_LOUD, ("pP2PInfo->WpsAttributes.PrimaryDeviceType.CategoryId : %d\n", pP2PInfo->WpsAttributes.PrimaryDeviceType.CategoryId ));
	
	pP2PInfo->WpsAttributes.PrimaryDeviceType.SubCategoryId 	= deviceInfo->PrimaryDeviceType.SubCategoryID;
	RT_TRACE(COMP_P2P, DBG_LOUD, ("pP2PInfo->WpsAttributes.PrimaryDeviceType.SubCategoryId: %d\n", pP2PInfo->WpsAttributes.PrimaryDeviceType.SubCategoryId));
	
	PlatformMoveMemory(
			pP2PInfo->WpsAttributes.PrimaryDeviceType.Oui, 
			deviceInfo->PrimaryDeviceType.OUI, 
			4
		);	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("pP2PInfo->WpsAttributes.PrimaryDeviceType.Oui: %02X-%02X-%02X-%02X\n", 
			pP2PInfo->WpsAttributes.PrimaryDeviceType.Oui[0],
			pP2PInfo->WpsAttributes.PrimaryDeviceType.Oui[1],
			pP2PInfo->WpsAttributes.PrimaryDeviceType.Oui[2],
			pP2PInfo->WpsAttributes.PrimaryDeviceType.Oui[3])
		);
	
	// Set the device name for WPS
	pP2PInfo->WpsAttributes.DeviceNameLength = (u1Byte) deviceInfo->DeviceName.uDeviceNameLength;
	PlatformMoveMemory(
			pP2PInfo->WpsAttributes.DeviceName, 
			deviceInfo->DeviceName.ucDeviceName, 
			deviceInfo->DeviceName.uDeviceNameLength
		);
	RT_PRINT_STR(COMP_P2P, DBG_LOUD, "DeviceName: ", pP2PInfo->WpsAttributes.DeviceName, pP2PInfo->WpsAttributes.DeviceNameLength);

	P2P_CorrectDeviceCategory(pTargetAdapter);
		
	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);

	// Loop iteration variables
	ULONG			i = 0;
	
	// OS-given structure
	PDOT11_WFD_SECONDARY_DEVICE_TYPE_LIST secondaryDeviceTypeList = 	
		(PDOT11_WFD_SECONDARY_DEVICE_TYPE_LIST) InformationBuffer;

	#if 0	// Implementation Status
    	typedef struct _DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST {	
        	NDIS_OBJECT_HEADER Header;						// SKIP
        	ULONG uNumOfEntries;								// OK
        	ULONG uTotalNumOfEntries;							// TODO
      		DOT11_WFD_DEVICE_TYPE SecondaryDeviceTypes[1];	// OK
    	} DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST, * PDOT11_WFD_SECONDARY_DEVICE_TYPE_LIST;
	#endif
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	// Start to set the secondary device type list

	// Set the number of secondary device type list entries
	pP2PInfo->WpsAttributes.SecondaryDeviceTypeLength = 
		(u1Byte) secondaryDeviceTypeList->uNumOfEntries;

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("secondaryDeviceType number = %d\n", secondaryDeviceTypeList->uNumOfEntries));

	// Set each secondary device type from the OS-given structure
	for(i = 0; i < secondaryDeviceTypeList->uNumOfEntries; i++)
	{
		pP2PInfo->WpsAttributes.SecondaryDeviceTypeList[i].CategoryId = 
			secondaryDeviceTypeList->SecondaryDeviceTypes[i].CategoryID;
		
		pP2PInfo->WpsAttributes.SecondaryDeviceTypeList[i].SubCategoryId = 
			secondaryDeviceTypeList->SecondaryDeviceTypes[i].SubCategoryID;

		PlatformMoveMemory(
				pP2PInfo->WpsAttributes.SecondaryDeviceTypeList[i].Oui, 
				secondaryDeviceTypeList->SecondaryDeviceTypes[i].OUI, 
				4
			);

		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "N63C_SET_OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST:\n",
			&(pP2PInfo->WpsAttributes.SecondaryDeviceTypeList[i]), sizeof(P2P_WPS_ATTRIBUTES_DEVICE_TYPE));
	}

	// Currently, do not know how to implement this -------
	//	The maximum number of entries that the SecondaryDeviceTypes array can contain. 
	//	The default value is 0.
	secondaryDeviceTypeList->uTotalNumOfEntries;

	#if 0	// Possible Solution
	This variable may be used to limit the maximum number of elements in SecondaryDeviceTypeList.
	Currently, the struct _P2P_WPS_ATTRIBUTES accepts 8 emements for SecondaryDeviceTypeList.
	#endif
	
	// ---------------------------------------------
	
	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_ADDITIONAL_IE
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_ADDITIONAL_IE(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	RT_STATUS 		rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	MEMORY_BUFFER	mbOidIe = {NULL, 0};
		
	// OS-given structure
	PDOT11_WFD_ADDITIONAL_IE additionalIEs = 	
		(PDOT11_WFD_ADDITIONAL_IE) InformationBuffer;

	#if 0	// Implementation Status
	typedef struct _DOT11_WFD_ADDITIONAL_IE 
    	{
       	NDIS_OBJECT_HEADER Header;		// SKIP
        	ULONG uBeaconIEsOffset;			// OK
        	ULONG uBeaconIEsLength;			// OK
        	ULONG uProbeResponseIEsOffset;		// OK
        	ULONG uProbeResponseIEsLength;	// OK
        	ULONG uDefaultRequestIEsOffset;	// OK
        	ULONG uDefaultRequestIEsLength;	// OK
    	} DOT11_WFD_ADDITIONAL_IE, *PDOT11_WFD_ADDITIONAL_IE;
	#endif
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;
	
	// Additional Beacon IEs ------------------------------------------------------
	mbOidIe.Buffer = (PVOID) ((pu1Byte) additionalIEs + additionalIEs->uBeaconIEsOffset);
	mbOidIe.Length = additionalIEs->uBeaconIEsLength;

	P2P_AddIe_Set(
			&pP2PInfo->AdditionalIEs, 
			P2P_ADD_IE_BEACON, 
			additionalIEs->uBeaconIEsLength, 
			(u1Byte *) additionalIEs + additionalIEs->uBeaconIEsOffset
		);

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Beacon Additional IE: ====\n", mbOidIe.Buffer, mbOidIe.Length);
	//-------------------------------------------------------------------------

	// Additional Probe Response IEs ------------------------------------------------------
	mbOidIe.Buffer = (PVOID) ((pu1Byte) additionalIEs + additionalIEs->uProbeResponseIEsOffset);
	mbOidIe.Length = additionalIEs->uProbeResponseIEsLength;

	P2P_AddIe_Set(
			&pP2PInfo->AdditionalIEs, 
			P2P_ADD_IE_PROBE_RESPONSE,
			additionalIEs->uProbeResponseIEsLength,
			(u1Byte *) additionalIEs + additionalIEs->uProbeResponseIEsOffset
		);

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ProbeResponse Additional IE: ====\n", mbOidIe.Buffer, mbOidIe.Length);
	
	// -------------------------------------------------------------------------------
	
	// Additional Default Probe Request IEs --------------------------------------------------
	mbOidIe.Buffer = (PVOID) ((pu1Byte) additionalIEs + additionalIEs->uDefaultRequestIEsOffset);
	mbOidIe.Length = additionalIEs->uDefaultRequestIEsLength;

	P2P_AddIe_Set(
			&pP2PInfo->AdditionalIEs, 
			P2P_ADD_IE_DEFAULT_REQUEST,
			additionalIEs->uDefaultRequestIEsLength,
			(u1Byte *) additionalIEs + additionalIEs->uDefaultRequestIEsOffset
		);

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "DefaultRequest Additional IE: ====\n", mbOidIe.Buffer, mbOidIe.Length);
	//----------------------------------------------------------------------------------

	P2P_CorrectDeviceCategory(pTargetAdapter);
	
	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER 		pDefaultAdapter = GetDefaultAdapter(pTargetAdapter);
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	BOOLEAN			bFilterOutNonAssociatedBSSID = FALSE;
			
	// OS-given data
	ULONG			deviceDiscoverability = *((PULONG) InformationBuffer);

	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	#if 0
		#define DOT11_WFD_DEVICE_NOT_DISCOVERABLE   0
		#define DOT11_WFD_DEVICE_AUTO_AVAILABILITY  16
		#define DOT11_WFD_DEVICE_HIGH_AVAILABILITY  24
	#endif

	FunctionIn(COMP_P2P);
	
	// Set the the corresponding variable in the structure
	pP2PInfo->uListenStateDiscoverability = deviceDiscoverability;
	RT_TRACE(COMP_OID_SET|COMP_P2P, DBG_LOUD, ("pP2PInfo->uListenStateDiscoverability: %d\n", pP2PInfo->uListenStateDiscoverability));

	switch(pP2PInfo->uListenStateDiscoverability)
	{
		case DOT11_WFD_DEVICE_NOT_DISCOVERABLE:
		{
			pP2PInfo->ExtListenTimingPeriod = 0;
			pP2PInfo->ExtListenTimingDuration = 0;
		}
		break;

		case DOT11_WFD_DEVICE_AUTO_AVAILABILITY:
		case DOT11_WFD_DEVICE_HIGH_AVAILABILITY:
		default: 
		{
			pP2PInfo->ExtListenTimingPeriod = 3 * P2P_MGNT_PERIOD;	// In the unit of P2P_MGNT_PERIOD
			pP2PInfo->ExtListenTimingDuration = 150;
			pP2PInfo->ExtListenTimingPeriodSlotCount = (u4Byte)P2P_EXT_LISTEN_TIMING_PERIOD_SC;

			// Do not check BSSID since the device port need to receive packets -----------------------------------------------------
			bFilterOutNonAssociatedBSSID = FALSE;
			pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
			// -------------------------------------------------------------------------------------------------------------
			
			// Device Port is Open --------------------------------------------
			//DM_DIG_WifiDirectBoostStart(GetDefaultAdapter(pTargetAdapter));
			// -------------------------------------------------------------

			if(P2P_STATE_INITIALIZED == pP2PInfo->State)
			{
				PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
				PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0); 
				RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("start extended listen immediately\n"));
			}
		}
		break;
	}
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("pP2PInfo->ExtListenTimingPeriod: %d\n", pP2PInfo->ExtListenTimingPeriod));
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("pP2PInfo->ExtListenTimingDuration: %d\n", pP2PInfo->ExtListenTimingDuration));
		
	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	
	// OS-given data
	PDOT11_WFD_DEVICE_LISTEN_CHANNEL	pListenChannel = (PDOT11_WFD_DEVICE_LISTEN_CHANNEL) InformationBuffer;

	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	#if 0
		// OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL
		#define OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL NWF_DEFINE_OID(0x13,NWF_WFD_DEVICE_OID,NWF_MANDATORY_OID)
		#define DOT11_WFD_DEVICE_LISTEN_CHANNEL_REVISION_1
		#define DOT11_SIZEOF_WFD_DEVICE_LISTEN_CHANNEL_REVISION_1

		typedef
		    struct _DOT11_WFD_DEVICE_LISTEN_CHANNEL
		    {
		        NDIS_OBJECT_HEADER Header;
		        UCHAR ChannelNumber;
		    } DOT11_WFD_DEVICE_LISTEN_CHANNEL, *PDOT11_WFD_DEVICE_LISTEN_CHANNEL;
	#endif

	// Set the the corresponding variable in the structure -------------------------------------------
	pP2PInfo->ListenChannel = pListenChannel->ChannelNumber;
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("pP2PInfo->ListenChannel: %d\n", pP2PInfo->ListenChannel));
	// --------------------------------------------------------------------------------------
	
	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_DISCOVER_REQUEST
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_DISCOVER_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	BOOLEAN			bStatus = FALSE;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	MEMORY_BUFFER	mbOidIe = {NULL, 0};
	OCTET_STRING 	osSsid = {NULL, 0};
	PDOT11_SSID 	pDot11Ssid = NULL;
	BOOLEAN			bStartDiscovery = TRUE;
	PP2P_DEVICE_DISCRIPTOR pP2PDeviceDesc = NULL;
	PP2P_DEVICE_LIST_ENTRY pP2PDeviceListEntry = NULL;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pTargetAdapter);	
	// Loop iteration variables
	ULONG 			i = 0;
	u8Byte			curTime = PlatformGetCurrentTime();
	
	// OS-given structure
	PDOT11_WFD_DISCOVER_REQUEST discoverRequest = 	
		(PDOT11_WFD_DISCOVER_REQUEST) InformationBuffer;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);
	
	// Save the Oid Issue time for speeding up the Go Negotiation ----------------
	pP2PInfo->LastDeviceDiscoveryOidIssueTime = PlatformGetCurrentTime();
	// ------------------------------------------------------------------
		
	RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: uDiscoverTimeout: %d\n", __FUNCTION__, discoverRequest->uDiscoverTimeout));

	// Discover Type
	switch(discoverRequest->DiscoverType)
	{
		case dot11_wfd_discover_type_scan_only:
			pP2PInfo->DiscoverSequence = P2P_DISCOVERY_SCAN_PHASE;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("discoverRequest->DiscoverType: dot11_wfd_discover_type_scan_only\n"));
			break;

		case dot11_wfd_discover_type_find_only:
			pP2PInfo->DiscoverSequence = P2P_DISCOVERY_FIND_PHASE;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("discoverRequest->DiscoverType: dot11_wfd_discover_type_find_only\n"));
			break;

		case dot11_wfd_discover_type_auto:
			pP2PInfo->DiscoverSequence = P2P_DISCOVERY_FIND_PHASE;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("discoverRequest->DiscoverType: dot11_wfd_discover_type_auto\n"));
			break;
	
		case dot11_wfd_discover_type_scan_social_channels:
			pP2PInfo->DiscoverSequence = P2P_DISCOVERY_FIND_PHASE;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("discoverRequest->DiscoverType: dot11_wfd_discover_type_auto\n"));
			break;

        	case dot11_wfd_discover_type_forced:
		default:
			pP2PInfo->DiscoverSequence = P2P_DISCOVERY_SCAN_PHASE | P2P_DISCOVERY_FIND_PHASE;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("discoverRequest->DiscoverType: dot11_wfd_discover_type_forced\n"));
			break;
	}

	// Scan Type
	switch(discoverRequest->ScanType)
	{
		case dot11_wfd_scan_type_auto:
			pP2PInfo->ScanType = SCAN_ACTIVE;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("discoverRequest->ScanType: dot11_wfd_scan_type_auto\n"));
			break;
			
		case dot11_wfd_scan_type_active:
			pP2PInfo->ScanType = SCAN_ACTIVE;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("discoverRequest->ScanType: dot11_wfd_scan_type_active\n"));
			break;
			
		case dot11_wfd_scan_type_passive:
		default:
			pP2PInfo->ScanType = SCAN_PASSIVE;
			RT_TRACE(COMP_P2P, DBG_LOUD, ("discoverRequest->ScanType: dot11_wfd_scan_type_passive\n"));
			break;
	}

	// Set the device IDs for device discovery ------------------------------------------
	P2PClearScanDeviceID(&pP2PInfo->ScanDeviceIDs);

	for(i = 0; i < discoverRequest->uNumDeviceFilters; i++)
	{
		bStatus = P2PAddScanDeviceID(
				&pP2PInfo->ScanDeviceIDs,
				((pu1Byte) discoverRequest + discoverRequest->uDeviceFilterListOffset + i * sizeof(DOT11_WFD_DISCOVER_DEVICE_FILTER))
			);
		RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Device ID List Entry:",
			((pu1Byte) discoverRequest + discoverRequest->uDeviceFilterListOffset + i * sizeof(DOT11_WFD_DISCOVER_DEVICE_FILTER)));

		if(bStatus != TRUE)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: P2PInfo.ScanDeviceIDs is too small!\n", __FUNCTION__));
		}
	}

	if(discoverRequest->uNumDeviceFilters == 1)	
	{// specific peer search
		P2P_DEV_LIST_ENTRY 		*dev = NULL;
		u1Byte					chnl = 0;
		
		p2p_DevList_Lock(&pP2PInfo->devList);

		pP2PInfo->uNumberOfDiscoverForSpecificChannels = 0;
		
		if(NULL != (dev = p2p_DevList_GetGo(&pP2PInfo->devList, pP2PInfo->ScanDeviceIDs.DeviceIDs[0])))
		{// go
			if(0 != (chnl = p2p_DevList_GetDevChnl(dev)))
			{
				pP2PInfo->DiscoverForSpecificChannels[pP2PInfo->uNumberOfDiscoverForSpecificChannels++] = chnl;
			}
		}

		if(NULL != (dev = p2p_DevList_Get(&pP2PInfo->devList, pP2PInfo->ScanDeviceIDs.DeviceIDs[0], P2P_DEV_TYPE_DEV)))
		{// dev
			if(0 != (chnl = p2p_DevList_GetDevChnl(dev)))
			{
				pP2PInfo->DiscoverForSpecificChannels[pP2PInfo->uNumberOfDiscoverForSpecificChannels++] = chnl;
				pP2PInfo->DiscoverForSpecificChannels[pP2PInfo->uNumberOfDiscoverForSpecificChannels++] = pP2PInfo->ListenChannel; // For Peer to Find us in GO Negotiation Procedure
				pP2PInfo->DiscoverForSpecificChannels[pP2PInfo->uNumberOfDiscoverForSpecificChannels++] = pP2PInfo->ListenChannel; // For Peer to Find us in GO Negotiation Procedure
			}
			else
			{
				// For case we have only received PD req from peer, since PD does not give info about peer Listen Channel,
				// we add all social channels, otherwise we will do scan phase which wastes time
				if(0 == chnl)
				{
					if(dev->rxFrames[P2P_FID_PROBE_REQ] && dev->rxFrames[P2P_FID_PROBE_REQ]->msg->listenChannel)
					{
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("add listen channel %u from ProbeReq\n", dev->rxFrames[P2P_FID_PROBE_REQ]->msg->listenChannel));
						pP2PInfo->DiscoverForSpecificChannels[pP2PInfo->uNumberOfDiscoverForSpecificChannels++] = dev->rxFrames[P2P_FID_PROBE_REQ]->msg->listenChannel;
					}
					else if(dev->rxFrames[P2P_FID_PD_REQ])
					{
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("only have peer's PD req, add all social channel\n"));
						pP2PInfo->DiscoverForSpecificChannels[pP2PInfo->uNumberOfDiscoverForSpecificChannels++] = 1;
						pP2PInfo->DiscoverForSpecificChannels[pP2PInfo->uNumberOfDiscoverForSpecificChannels++] = 6;
						pP2PInfo->DiscoverForSpecificChannels[pP2PInfo->uNumberOfDiscoverForSpecificChannels++] = 11;
					}
				}
			}
		}

		if(pP2PInfo->uNumberOfDiscoverForSpecificChannels)
		{
			pP2PInfo->bDiscoverForSpecificChannels = TRUE;
			pP2PInfo->uNumberOfDiscoverRounds = 10;
			RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, "to scan following channels to find specific peer:\n", 
				pP2PInfo->DiscoverForSpecificChannels, pP2PInfo->uNumberOfDiscoverForSpecificChannels);

			if(	pP2PInfo->DeviceListForQuery.uNumberOfDevices != 0 &&	// If empty device list, force to scan
				PlatformGetCurrentTime() - pP2PInfo->LastDeviceDiscoveryIndicatedTime < 300 * 1000)  // Do not discover for that short time 300 ms
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Discover Too Frequently, Complete Directly\n", __FUNCTION__));
				bStartDiscovery = FALSE;
			}
		}
		else
		{
			pP2PInfo->bDiscoverForSpecificChannels = FALSE;
			pP2PInfo->uNumberOfDiscoverRounds = 0;
			
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("device entry not found\n"));
			
			P2PDeviceListActionInterface(
					pP2PInfo, 
					P2P_DEVICE_LIST_ACTION_DUMP,
					&pP2PInfo->DeviceList, 
					NULL, NULL
				);
			
			P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
		}

		p2p_DevList_Unlock(&pP2PInfo->devList);
	}
	else
	{		
		RT_TRACE(COMP_P2P, DBG_LOUD, ("devicelist empty\n"));

		if(curTime < MultiportGetLastConnectionActionTime(pTargetAdapter) + P2P_BLOCK_NORMAL_SCAN_PERIOD)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Last Connection time - current time = %d us, skip this discovery\n", (u4Byte)(MultiportGetLastConnectionActionTime(pTargetAdapter) + P2P_BLOCK_NORMAL_SCAN_PERIOD - curTime)));
			bStartDiscovery = FALSE;
		}
	}
	
	//----------------------------------------------------------------------------

	
	// Additional Probe Request IEs ---------------------------------------------------
	mbOidIe.Buffer = (PVOID) ((pu1Byte) discoverRequest + discoverRequest->uIEsOffset);
	mbOidIe.Length = discoverRequest->uIEsLength;

	P2P_AddIe_Set(
			&pP2PInfo->AdditionalIEs, 
			P2P_ADD_IE_PROBE_REQUEST,
			discoverRequest->uIEsLength,
			(u1Byte *) discoverRequest + discoverRequest->uIEsOffset
		);

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ProbeRequest Additional IE:\n", mbOidIe.Buffer, mbOidIe.Length);
	//----------------------------------------------------------------------------

	
	// Set the legacy network scanning ------------------------------------------------
	pP2PInfo->bForceScanLegacyNetworks = discoverRequest->bForceScanLegacyNetworks;
	RT_TRACE(COMP_OID_SET | COMP_P2P, DBG_LOUD, ("%s: pP2PInfo->bForceScanLegacyNetworks: %d\n", __FUNCTION__, pP2PInfo->bForceScanLegacyNetworks));
	//----------------------------------------------------------------------------
	
	if(bStartDiscovery)
	{
		// Win8: Guarantee the packet transmission in the right channel since the this OID will switch channel --------------------------------------------------------------------------------------------
		//RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Delay 1000 ms to guranatee the packet transmission in the right channel and also update the P2P state machine!\n"));
		//delay_ms(500);
		// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//		pP2PInfo->bDeviceDiscoveryInProgress = TRUE;

		//
		// <Roger_Notes>
		// Leave LPS mode prior to the process of P2P device discovery to improve discovery capability in DC power mode.
		// 2014.03.25.
		//
		LeisurePSLeave(pDefaultAdapter, LPS_DISABLE_DEVICE_DISCOVERY);// Leave LPS mode for P2P device discovery, added by Roger, 2014.03.25.

		RT_TRACE(COMP_OID_SET | COMP_P2P, DBG_LOUD, ("Delay 50 ms to guranatee the action frame transmission in the right channel!\n"));
		delay_ms(50);
		
		for(i = 0; MgntScanInProgress(&pP2PInfo->pAdapter->MgntInfo); i++)
		{
			RT_TRACE(COMP_OID_SET | COMP_P2P, DBG_LOUD, ("Delay 10 ms to guranatee the packet transmission in the right channel!\n"));
			delay_ms(10);

			if(i == 2) // Wait for 30 ms
			{	
				// Stop scan imediately to wait for new action
				P2PScanListCeaseScan(pP2PInfo);
				break;
			}
		}
	
		for(i = 0; pP2PInfo->State != P2P_STATE_INITIALIZED; i++)
		{
			RT_TRACE(COMP_OID_SET | COMP_P2P, DBG_LOUD, ("Delay 10 ms to update the P2P state machine!\n"));
			delay_ms(10);
			
			if(i == 50) break;
		}

		// Clear the device list
		P2PDeviceListActionInterface(
			pP2PInfo,
			P2P_DEVICE_LIST_ACTION_CLEAR,
			&pP2PInfo->DeviceList,
			NULL, NULL
		);
		
		// Clear the device list for query
		P2PDeviceListActionInterface(
			pP2PInfo,
			P2P_DEVICE_LIST_ACTION_CLEAR,
			&pP2PInfo->DeviceListForQuery,
			NULL, NULL
		);		
		
		// Simply do the device discovery since the device discovery may be used as the method to guarantee the common channel 
		P2PResetCommonChannelArrivingProcess(pP2PInfo);

		// Start the device discovery
		if(pP2PInfo->pAdapter->bInHctTest)
			P2PDeviceDiscovery(pP2PInfo, 2);
		else
			P2PDeviceDiscovery(pP2PInfo, 10);

		// Win8: The discover time is very limited so we delay the OID and do the discovery in the same time. --------------------------
		//RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Delay 3000 ms to get additional time to scan!\n"));
		//delay_ms(3000);
		// -------------------------------------------------------------------------------------------------------------
		
		// Set the indication to OS when the discovery completes
		pP2PInfo->bDeviceDiscoveryIndicateToOS = TRUE;
	}
	else
	{
		// Win8: Guarantee the packet transmission in the right channel since the this OID will switch channel --------------------------------------------------------------------------------------------
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Delay 50 ms to guranatee the packet transmission in the right channel and also update the P2P state machine!\n"));
		delay_ms(50);
		// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		// Let this OID routine run faster than the P2POidPostProcessWorkItemCallback ------
		pP2PInfo->bPostoneP2POidPostProcessWorkItem = TRUE;
		// ----------------------------------------------------------------------
		
		// Scheduling the sending of the provision discovery response --------------------------------------
		pP2PInfo->OidOperation = OID_OPERATION_INDICATE_DISCOVERY_COMPLETE;
#if 1
		PlatformSetTimer(pTargetAdapter, &(pP2PInfo->P2POidPostProcessTimer), 10);
#else
		PlatformScheduleWorkItem(&pP2PInfo->P2POidPostProcessWorkItem);
#endif
		//----------------------------------------------------------------------------------------
	}
	
	// OS needs this status code
	ndisStatus = NDIS_STATUS_INDICATION_REQUIRED;

	// Let the P2POidPostProcessWorkItem Run
	pP2PInfo->bPostoneP2POidPostProcessWorkItem = FALSE;
	
	FunctionOut(COMP_OID_SET);

	return ndisStatus;
}


// Query operation for OID_DOT11_WFD_GET_DIALOG_TOKEN
static NDIS_STATUS
N63C_QUERY_OID_DOT11_WFD_GET_DIALOG_TOKEN(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	u1Byte 			nextToken = 0;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);

	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	// Preidict the next token generated by the marco of IncreaseDialogToken(pP2PInfo->DialogToken) : Non-Zero ----------
	nextToken = (pP2PInfo->DialogToken + 1 == 0) ? (pP2PInfo->DialogToken + 2) : (pP2PInfo->DialogToken + 1);
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: nextToken: %d\n", __FUNCTION__, nextToken));
	// --------------------------------------------------------------------------------------------------------
	
	*BytesWritten = 1;
	*((pu1Byte) InformationBuffer) = nextToken;

	pP2PInfo->oidDialogToken = nextToken;

	return NDIS_STATUS_SUCCESS;
}


// Query operation for OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY
static NDIS_STATUS
N63C_QUERY_OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);

	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	*BytesWritten = sizeof(pP2PInfo->uListenStateDiscoverability);
	*((PULONG) InformationBuffer) = pP2PInfo->uListenStateDiscoverability;
	RT_TRACE(COMP_P2P, DBG_LOUD, ("pP2PInfo->uListenStateDiscoverability: %d\n", pP2PInfo->uListenStateDiscoverability));
	
	return NDIS_STATUS_SUCCESS;
}

// Query operation for OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL
static NDIS_STATUS
N63C_QUERY_OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	PP2P_INFO	pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	PDOT11_WFD_DEVICE_LISTEN_CHANNEL 
		pListenChannel = (PDOT11_WFD_DEVICE_LISTEN_CHANNEL) InformationBuffer;

	#if 0
		#define DOT11_WFD_DEVICE_LISTEN_CHANNEL_REVISION_1 
		#define DOT11_SIZEOF_WFD_DEVICE_LISTEN_CHANNEL_REVISION_1 

		typedef struct _DOT11_WFD_DEVICE_LISTEN_CHANNEL { 
		 NDIS_OBJECT_HEADER Header; 
		 UCHAR ChannelNumber; 
		} DOT11_WFD_DEVICE_LISTEN_CHANNEL, *PDOT11_WFD_DEVICE_LISTEN_CHANNEL; 
	#endif

	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	N6_ASSIGN_OBJECT_HEADER(
			pListenChannel->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_WFD_DEVICE_LISTEN_CHANNEL_REVISION_1,
			sizeof(DOT11_WFD_DEVICE_LISTEN_CHANNEL)
		);
	
	pListenChannel->ChannelNumber = pP2PInfo->ListenChannel;

	*BytesWritten = sizeof(DOT11_WFD_DEVICE_LISTEN_CHANNEL);
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("pP2PInfo->ListenChannel: %d\n",  pP2PInfo->ListenChannel));

	return NDIS_STATUS_SUCCESS;
}

// Query operation for OID_DOT11_WFD_ENUM_DEVICE_LIST
static NDIS_STATUS
N63C_QUERY_OID_DOT11_WFD_ENUM_DEVICE_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
#if 0
	typedef struct DOT11_BYTE_ARRAY {
  		NDIS_OBJECT_HEADER Header;
  		ULONG              uNumOfBytes;
  		ULONG              uTotalNumOfBytes;
  		UCHAR              ucBuffer[1];
	} DOT11_BYTE_ARRAY, *PDOT11_BYTE_ARRAY;
#endif

	u4Byte			i = 0;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	MEMORY_BUFFER	mbObject = {NULL, 0};
	PDOT11_WFD_DEVICE_ENTRY wfdEntry = NULL;
	u4Byte			usedBytes = 0;
		
	// Skip the MAC header and the first three fields (non-IE) in the beacon and the probe response
	u4Byte skippedBytes = sMacHdrLng + 12;
	
	// OS-given data
	PDOT11_BYTE_ARRAY pByteArray = (PDOT11_BYTE_ARRAY) InformationBuffer;
	
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	pByteArray->uNumOfBytes = 0;
	pByteArray->uTotalNumOfBytes = 0;
	PlatformZeroMemory(pByteArray, sizeof(DOT11_BYTE_ARRAY));
	//-------------------------------------------------------


	// Update the device list for the OS query ----------------------
	P2PDeviceListActionInterface(
			pP2PInfo, 
			P2P_DEVICE_LIST_ACTION_COPY_TO_QUERY_LIST,
			NULL, NULL, NULL
		);
	// -------------------------------------------------------
	
	
	N6_ASSIGN_OBJECT_HEADER(
			pByteArray->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_DEVICE_ENTRY_BYTE_ARRAY_REVISION_1,
			sizeof(DOT11_BYTE_ARRAY)
		);
		
	// Compute the requried memory length ------------------------------------------------------------------
	*BytesNeeded += FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer);
	*BytesNeeded += N63CComputeRequiredDeviceEntriesSize(pP2PInfo);
	//-------------------------------------------------------------------------------------------------

	// Output buffer length checking ---------------
	if(*BytesNeeded > InformationBufferLength)
	{
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}
	//-----------------------------------------
	
	// Fill the device entries -------------------------------------------------------------------------------
	wfdEntry = (PDOT11_WFD_DEVICE_ENTRY)((pu1Byte) pByteArray + FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer));

	for(i = 0; i < pP2PInfo->DeviceListForQuery.uNumberOfDevices; i++)
	{
		wfdEntry = (PDOT11_WFD_DEVICE_ENTRY)((pu1Byte) wfdEntry + usedBytes);

		N63CConstruct_DOT11_WFD_DEVICE_ENTRY(	
				pP2PInfo,
				&pP2PInfo->DeviceListForQuery.DeviceEntry[i],
				wfdEntry,
				&usedBytes
			);
		
		pByteArray->uNumOfBytes += usedBytes;
	}
	//--------------------------------------------------------------------------------------------------
	
	// Assertion --------------------------------------------------------------------------------------------
	if(pByteArray->uNumOfBytes != N63CComputeRequiredDeviceEntriesSize(pP2PInfo))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, (__FUNCTION__": N63CComputeRequiredMemoryForDeviceList value mismatch!\n"));
	}
	//-----------------------------------------------------------------------------------------------------

	// Post settings for the successfull execution -----------------------------------
	pByteArray->uTotalNumOfBytes = pByteArray->uNumOfBytes;
	*BytesWritten = *BytesNeeded;
	*BytesNeeded = 0;

	return NDIS_STATUS_SUCCESS;
	// -----------------------------------------------------------------------
}


// Set operation for OID_DOT11_WFD_FLUSH_DEVICE_LIST
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_FLUSH_DEVICE_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	PMGNT_INFO		pTargetMgntInfo = &pTargetAdapter->MgntInfo;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	// Clear the scan list
	PlatformZeroMemory( pTargetMgntInfo->bssDesc, sizeof(RT_WLAN_BSS)*MAX_BSS_DESC );
	pTargetMgntInfo->NumBssDesc = 0;	
	
	// Clear the device list
	P2PDeviceListActionInterface(
			pP2PInfo,
			P2P_DEVICE_LIST_ACTION_CLEAR,
			&pP2PInfo->DeviceList,
			NULL, NULL
		);
	
	// Clear the device list for Query
	P2PDeviceListActionInterface(
			pP2PInfo,
			P2P_DEVICE_LIST_ACTION_CLEAR,
			&pP2PInfo->DeviceListForQuery,
			NULL, NULL
		);	
	
	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_STOP_DISCOVERY
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_STOP_DISCOVERY(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
	{
		P2PScanListCeaseScan(pP2PInfo);
		P2PDeviceDiscoveryComplete(pP2PInfo, TRUE);
	}
	
	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_SEND_INVITATION_REQUEST
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_SEND_INVITATION_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	VOID 				*req = NULL;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;

	do
	{
		DOT11_SEND_INVITATION_REQUEST_PARAMETERS *param = (DOT11_SEND_INVITATION_REQUEST_PARAMETERS *)InformationBuffer;
		
		if(NULL == (req = N63C_SendAction_InvitationReq(pTargetAdapter, param, n63c_SendP2pReqActionFrameStateCb)))
		{
			status = NDIS_STATUS_FAILURE;
			break;
		}

		status = NDIS_STATUS_INDICATION_REQUIRED;
	}while(FALSE);

	FunctionOut(COMP_OID_SET);
	
	return status;
}

// Set operation for OID_DOT11_WFD_SEND_INVITATION_RESPONSE
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_SEND_INVITATION_RESPONSE(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;

	do
	{
		PlatformMoveMemory(InfoBuf, InformationBuffer, InformationBufferLength);
		InfoBufLen = InformationBufferLength;

		status = NDIS_STATUS_INDICATION_REQUIRED;
	}while(FALSE);

	FunctionOut(COMP_OID_SET);
	
	return status;
}

// Set operation for OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	VOID 				*req = NULL;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;

	do
	{
		DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS *param = (DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS *)InformationBuffer;
	
		if(NULL == (req = N63C_SendAction_PdReq(pTargetAdapter, param, n63c_SendP2pReqActionFrameStateCb)))
		{
			status = NDIS_STATUS_FAILURE;
			break;
		}

		status = NDIS_STATUS_INDICATION_REQUIRED;
	}while(FALSE);
	
	FunctionOut(COMP_OID_SET);
	
	return status;
}


// Set operation for OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;

	do
	{
		PlatformMoveMemory(InfoBuf, InformationBuffer, InformationBufferLength);
		InfoBufLen = InformationBufferLength;

		status = NDIS_STATUS_INDICATION_REQUIRED;
	}while(FALSE);
	
	FunctionOut(COMP_OID_SET);
	
	return status;
}

// Set operation for OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	VOID 				*req = NULL;
	
	PP2P_INFO	pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	PADAPTER 	pDefaultAdapter = GetDefaultAdapter(pTargetAdapter);
	
	// OS-given structure
	PDOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS pParameters = 
		(PDOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS) InformationBuffer;

	// Clear output variables -----
	*BytesRead = 0;
	*BytesNeeded = 0;
	//------------------------

	FunctionIn(COMP_OID_SET);

#if (MULTICHANNEL_SUPPORT == 1)
	// Consider the default port's link state -----------------------------------------------------------------------------------------------------------------
	if(MgntLinkStatusQuery(GetDefaultAdapter(pP2PInfo->pAdapter)) == RT_MEDIA_CONNECT)
	{
		pP2PInfo->OperatingChannel = MultiChannelGetPortConnected20MhzChannel(GetDefaultAdapter(pP2PInfo->pAdapter));
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Change op ch to default port connected channel: %d\n", pP2PInfo->OperatingChannel));	
	}
	else
	{
		if(pDefaultAdapter->MgntInfo.WFDOpChannel !=0)
			pP2PInfo->OperatingChannel = pDefaultAdapter->MgntInfo.WFDOpChannel;
		else
		{
// Removed by Bruce, 2015-6-28.
// Shall not change OP channel after the GO has started or the channels for all P2P roles have been defined.
#if 0 // (AUTO_CHNL_SEL_NHM ==1)
			if(IS_AUTO_CHNL_SUPPORT(pDefaultAdapter))
				pP2PInfo->OperatingChannel =  P2PIsSocialChannel(GET_AUTO_CHNL_SELECTED_NUM(pDefaultAdapter)) ? 
					GET_AUTO_CHNL_SELECTED_NUM(pDefaultAdapter) : P2P_DEFAULT_OPERATING_CHANNEL;
			else
#endif
			{
				if(0 == pP2PInfo->OperatingChannel)
					pP2PInfo->OperatingChannel = P2P_DEFAULT_OPERATING_CHANNEL;
			}
		}
		RT_TRACE_F(COMP_P2P, DBG_LOUD, (" [ACS] Change op ch to P2P_DEFAULT_OPERATING_CHANNEL channel: %d\n", pP2PInfo->OperatingChannel));	
	}
	// -----------------------------------------------------------------------------------------------------------------------------------------------
#endif

	do
	{
		DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS *param = (DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS *)InformationBuffer;
	
		if(NULL == (req = N63C_SendAction_GoNegReq(pTargetAdapter, param, n63c_SendP2pReqActionFrameStateCb)))
		{
			status = NDIS_STATUS_FAILURE;
			break;
		}

		status = NDIS_STATUS_INDICATION_REQUIRED;
	}while(FALSE);
		
	FunctionOut(COMP_OID_SET);
	
	return status;
}

// Set operation for OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;

	do
	{
		PlatformMoveMemory(InfoBuf, InformationBuffer, InformationBufferLength);
		InfoBufLen = InformationBufferLength;

		status = NDIS_STATUS_INDICATION_REQUIRED;
	}while(FALSE);
	
	FunctionOut(COMP_OID_SET);
	
	return status;
}



// Set operation for OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRM
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRM(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	VOID 				*req = NULL;

	FunctionIn(COMP_OID_SET);

	*BytesRead = 0;
	*BytesNeeded = 0;
	
	do
    {
		PlatformMoveMemory(InfoBuf, InformationBuffer, InformationBufferLength);
		InfoBufLen = InformationBufferLength;

		status = NDIS_STATUS_INDICATION_REQUIRED;
	}while(FALSE);

	FunctionOut(COMP_OID_SET);

	return status;
}


// Set operation for OID_DOT11_WFD_START_GO_REQUEST
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_START_GO_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	PRT_NDIS62_COMMON 	pNdis62Common = pTargetAdapter->pNdis62Common;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pTargetAdapter);
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	PADAPTER 		pDevicePort = GetFirstDevicePort(pDefaultAdapter);
	PP2P_INFO		pDeviceP2PInfo = GET_P2P_INFO(pDevicePort);
	
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY | COMP_P2P);

	// Check the AP state
	if(GetAPState(pTargetAdapter) != AP_STATE_STOPPED)
	{
		RT_TRACE((COMP_OID_SET | COMP_OID_QUERY | COMP_INIT | COMP_INDIC), DBG_LOUD, ("AP is not in INIT STATE\n"));

		return NDIS_STATUS_INVALID_STATE;
	}

	// Radio is in the OFF state
	if(N6CQuery_DOT11_NIC_POWER_STATE(GetDefaultAdapter(pTargetAdapter)) == FALSE)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("RF is OFF, return NDIS_STATUS_DOT11_POWER_STATE_INVALID.\n"));

		return NDIS_STATUS_DOT11_POWER_STATE_INVALID;
	}


#if 1 // The Extended Listening will let the AP's Channel be wrong when during extended listening

	//if(MgntScanInProgress(&pDefaultAdapter->MgntInfo))
	//{
		// Postpone the extended listening for a while
	//	P2PExtendedListenResetCounter(pDeviceP2PInfo);
			
		// Stop potential device port extended listening 
	//	P2PScanListCeaseScan(pDeviceP2PInfo);

		// Delay for a while for running ScanTimer callback by context switch
		//	+ Otherwise, this scan will be skipped due to MgntScanInProgress flag
	//	delay_ms(20);
	//}

	if(P2P_DOING_DEVICE_DISCOVERY(pDeviceP2PInfo))
	{// Doing P2P Device Discovery
		//msDelayStart += 10; // if scan in progress, we have to wait until ScanComplete() finishes
		P2PScanListCeaseScan(pDeviceP2PInfo);
		P2PDeviceDiscoveryComplete(pDeviceP2PInfo, TRUE); // P2P State is restored in this function.
	}
	else if(pDeviceP2PInfo->bExtendedListening)
	{// Doing extended listening
		P2PScanListCeaseScan(pDeviceP2PInfo);
		P2PExtendedListenComplete(pDeviceP2PInfo);
	}
	else if(MgntScanInProgress(&pDefaultAdapter->MgntInfo)) 
	{// Doing normal scan
		//msDelayStart += 10; // if scan in progress, we have to wait until ScanComplete() finishes
		P2PScanListCeaseScan(pDeviceP2PInfo);
	}

	delay_ms(20);

#endif


#if (MULTICHANNEL_SUPPORT == 1)
{
	if(MultiChannelSwitchNeeded(pDefaultAdapter))
	{
		// Stop MultiChannel switch for initializing the GO Tsf -----------
		MultiChannelStopChannelSwitchScheduler(pDefaultAdapter,TRUE);
		// -----------------------------------------------------
	}
}
#endif

	// Change the last scan complete time to block scan request for a while from the default port.
	// If the scan process is executed, the next handshake may fail because this port is not in the
	// correct channel now.
	MultiportRecordLastScanTime(pDefaultAdapter);

	if(0 == pP2PInfo->OperatingChannel)
		pP2PInfo->OperatingChannel = P2P_DEFAULT_OPERATING_CHANNEL;
	
	// Note:
	// Do not change operating channel here becuase the OP channel has been negotiated from the previous
	// handshake or set by OS.
#if 0


	// Consider the default port's link state
	if(MgntLinkStatusQuery(pDefaultAdapter) == RT_MEDIA_CONNECT)
	{
		#if (MULTICHANNEL_SUPPORT == 1)	// For Compilation: Only PCI-E Support MultiChannel Now

		pP2PInfo->OperatingChannel = MultiChannelGetPortConnected20MhzChannel(pDefaultAdapter);

		#endif
		
		RT_TRACE((COMP_OID_QUERY | COMP_OID_SET), DBG_LOUD, ("Use Default Port Channel: Channel %d\n", pP2PInfo->OperatingChannel));
	}
	else
	{
		if(pDefaultAdapter->MgntInfo.WFDOpChannel != 0)
			pP2PInfo->OperatingChannel = pDefaultAdapter->MgntInfo.WFDOpChannel;
		else
		{
#if (AUTO_CHNL_SEL_NHM ==1)
			if(IS_AUTO_CHNL_SUPPORT(pDefaultAdapter))
				pP2PInfo->OperatingChannel =  P2PIsSocialChannel(GET_AUTO_CHNL_SELECTED_NUM(pDefaultAdapter)) ?
					GET_AUTO_CHNL_SELECTED_NUM(pDefaultAdapter) : P2P_DEFAULT_OPERATING_CHANNEL;
			else
#endif
				pP2PInfo->OperatingChannel = P2P_DEFAULT_OPERATING_CHANNEL;
		}
		RT_TRACE((COMP_OID_QUERY | COMP_OID_SET), DBG_LOUD, ("[ACS] Start GO at Channel %d\n", pP2PInfo->OperatingChannel));
	}
#endif


	// Update Device Port's Operating Channel Number (Win8 Action Frame is Processed by Only Device Port)
	{
		PADAPTER pDevicePort = GetFirstDevicePort(pDefaultAdapter);
		PP2P_INFO pDeviceP2PInfo = GET_P2P_INFO(pDevicePort);
		
		pDeviceP2PInfo->OperatingChannel = pP2PInfo->OperatingChannel;
	}

	RT_TRACE_F(COMP_MLME, DBG_LOUD, ("ChangeWirelessModeHandler\n"));
	HalChangeWirelessMode(pTargetAdapter, pP2PInfo->OperatingChannel);

	RT_TRACE_F(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_CHANNEL_AND_BANDWIDTH\n"));

	if(pDefaultAdapter->MgntInfo.mAssoc)
	{		
		HalChangeWirelessMode(pDefaultAdapter, pP2PInfo->OperatingChannel);
		// Set the operating channel
		MgntActSet_802_11_CHANNEL_AND_BANDWIDTH(
			pTargetAdapter, 
			pP2PInfo->OperatingChannel, 
			pDefaultAdapter->MgntInfo.pChannelInfo->CurrentChannelBandWidth, 
			pDefaultAdapter->MgntInfo.pChannelInfo->Ext20MHzChnlOffsetOf40MHz, 
			pDefaultAdapter->MgntInfo.pChannelInfo->Ext40MHzChnlOffsetOf80MHz, 
			0
			);
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Default port is conencted, switch to channel (%d), Bandwidth (%d), 20/40M_Offset (%d), 40/80M_Offset (%d)\n",
			pDefaultAdapter->MgntInfo.dot11CurrentChannelNumber, 
			pDefaultAdapter->MgntInfo.pChannelInfo->CurrentChannelBandWidth, 
			pDefaultAdapter->MgntInfo.pChannelInfo->Ext20MHzChnlOffsetOf40MHz, 
			pDefaultAdapter->MgntInfo.pChannelInfo->Ext40MHzChnlOffsetOf80MHz));
	}
	else
	{
		// Set the operating channel		
		MgntActSet_802_11_CHANNEL_AND_BANDWIDTH(
				pTargetAdapter, 
				pP2PInfo->OperatingChannel, 
				CHANNEL_WIDTH_20, 
				EXTCHNL_OFFSET_NO_EXT, 
				EXTCHNL_OFFSET_NO_EXT, 
				0
			);
	}
	

	// Resolve the Wrong GO active channel -----------------------------------------------------------------------
	{
		PADAPTER	pLoopAdapter = GetDefaultAdapter(pTargetAdapter);

		RT_TRACE(COMP_P2P, DBG_LOUD, ("Set All SettingBeforeScan.ChannelNumber = %d\n", pP2PInfo->OperatingChannel));
		
		while(pLoopAdapter !=NULL)
		{
			pLoopAdapter->MgntInfo.dot11CurrentChannelNumber = pP2PInfo->OperatingChannel;
			pLoopAdapter->MgntInfo.SettingBeforeScan.ChannelNumber = pP2PInfo->OperatingChannel;
			pLoopAdapter->MgntInfo.SettingBeforeScan.CenterFrequencyIndex1 = CHNL_GetCenterFrequency(pP2PInfo->OperatingChannel,pLoopAdapter->MgntInfo.SettingBeforeScan.ChannelBandwidth, pLoopAdapter->MgntInfo.SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz);
			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
		
		pLoopAdapter = GetDefaultAdapter(pTargetAdapter);
		// Prefast warning 6011 Dereferencing NULL pointer 'pLoopAdapter'.
		if (pLoopAdapter != NULL)
		{
			RT_TRACE_F(COMP_P2P | COMP_HT, DBG_LOUD,
				("SettingBeforeScan() chnl: %u, CenterFrequencyIndex1: %u\n",
					pLoopAdapter->MgntInfo.SettingBeforeScan.ChannelNumber,
					pLoopAdapter->MgntInfo.SettingBeforeScan.CenterFrequencyIndex1));
		}
		
	}
	// --------------------------------------------------------------------------------------------------------

	// Start AP mode
	ndisStatus=N62CStartApMode(pTargetAdapter);
	pNdis62Common->CurrentOpState = OP_STATE;


	// Debug Message
	{
		PADAPTER pAdapter = GetDefaultAdapter(pTargetAdapter);

		while(pAdapter != NULL)
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("pAdapter->pNdis62Common->PortNumber: %d\n", pAdapter->pNdis62Common->PortNumber));
			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "pAdapter->MgntInfo.Bssid: ", pAdapter->MgntInfo.Bssid);
								
			pAdapter = GetNextExtAdapter(pAdapter);
		}
	}
	
	if(ndisStatus == NDIS_STATUS_SUCCESS)
	{
		PlatformIndicateP2PEvent(pP2PInfo, P2P_EVENT_GO_OPERATING_CHANNEL, NULL);
	}

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY | COMP_P2P);

	return ndisStatus;
}

// Set operation for OID_DOT11_WFD_GROUP_START_PARAMETERS
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_GROUP_START_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
#if 0
    #define DOT11_WFD_GROUP_START_PARAMETERS_REVISION_1     1
    
    typedef struct _DOT11_WFD_GROUP_START_PARAMETERS {	
        NDIS_OBJECT_HEADER Header;
        DOT11_WFD_CHANNEL AdvertisedOperatingChannel;
    } DOT11_WFD_GROUP_START_PARAMETERS, * PDOT11_WFD_GROUP_START_PARAMETERS;

    typedef struct _DOT11_WFD_CHANNEL 
    {
        DOT11_COUNTRY_OR_REGION_STRING CountryRegionString;
        UCHAR OperatingClass;
        UCHAR ChannelNumber;
    } DOT11_WFD_CHANNEL, * PDOT11_WFD_CHANNEL;
#endif

	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);

	// OS-given structure
	PDOT11_WFD_GROUP_START_PARAMETERS pParameters = 
		(PDOT11_WFD_GROUP_START_PARAMETERS) InformationBuffer;
		
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;
	
	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);
	

	// Only consider the channel number
	pP2PInfo->OperatingChannel = pParameters->AdvertisedOperatingChannel.ChannelNumber; 
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("pParameters->AdvertisedOperatingChannel.ChannelNumber: %d\n", pParameters->AdvertisedOperatingChannel.ChannelNumber));
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("pP2PInfo->OperatingChannel: %d\n", pP2PInfo->OperatingChannel));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return NDIS_STATUS_SUCCESS;

}
	
// Set operation for OID_DOT11_WFD_GROUP_JOIN_PARAMETERS
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_GROUP_JOIN_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
#if 0
    typedef 
    struct _DOT11_WFD_GROUP_JOIN_PARAMETERS {	
        NDIS_OBJECT_HEADER Header;
        DOT11_WFD_CHANNEL GOOperatingChannel;
        ULONG GOConfigTime;
        BOOLEAN bInGroupFormation;					// TODO
        BOOLEAN bWaitForWPSReady;					// TODO
    } DOT11_WFD_GROUP_JOIN_PARAMETERS, * PDOT11_WFD_GROUP_JOIN_PARAMETERS;
#endif

	PADAPTER 	pDevicePort = GetFirstDevicePort(pTargetAdapter);
	PP2P_INFO	pDeviceP2PInfo = GET_P2P_INFO(pDevicePort);
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	PP2P_DEVICE_DISCRIPTOR pP2PDeviceDesc = NULL;
	PP2P_DEVICE_LIST_ENTRY pP2PDeviceListEntry = NULL;

	// OS-given structure
	PDOT11_WFD_GROUP_JOIN_PARAMETERS pParameters = 
		(PDOT11_WFD_GROUP_JOIN_PARAMETERS) InformationBuffer;
		
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;
	
	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);
	
	// Only consider the channel number
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("pParameters->GOConfigTime: %d\n", pParameters->GOConfigTime));

	pP2PInfo->OperatingChannel = pParameters->GOOperatingChannel.ChannelNumber; 
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("pP2PInfo->OperatingChannel: %d\n", pP2PInfo->OperatingChannel));

	pP2PInfo->ClientJoinGroupContext.bInGroupFormation = pParameters->bInGroupFormation;
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("pP2PInfo->ClientJoinGroupContext.bInGroupFormation: %d\n", pP2PInfo->ClientJoinGroupContext.bInGroupFormation));
	
	pP2PInfo->ClientJoinGroupContext.WpsState = (pParameters->bWaitForWPSReady) ? P2P_CLIETN_JOIN_GROUP_WPS_STATE_SCANNING : P2P_CLIETN_JOIN_GROUP_WPS_STATE_NONE;
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("pP2PInfo->ClientJoinGroupContext.WpsState: %d\n", pP2PInfo->ClientJoinGroupContext.WpsState));


	// Select the Target Channel in AutoGO mode ----------------------------------------------------------------------------------
	if(pP2PInfo->OperatingChannel == 0)
	{
		RT_PRINT_ADDR(COMP_OID_SET, DBG_LOUD, "pP2PInfo->DesiredTargetMacAddress: ", pP2PInfo->DesiredTargetMacAddress);
	
		// From Win8 Specific Device Information Pool	
		pP2PDeviceListEntry = P2PDeviceListFind(&pDeviceP2PInfo->DeviceList, pP2PInfo->DesiredTargetMacAddress);
		
		// From Common Device Information Pool: Use Interface Address
		pP2PDeviceDesc = P2PScanListFind(pDeviceP2PInfo->ScanList, pDeviceP2PInfo->ScanListSize, NULL, pP2PInfo->DesiredTargetMacAddress, NULL);

		if(pP2PDeviceListEntry)
		{
			pP2PInfo->OperatingChannel = pP2PDeviceListEntry->ChannelNumber;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Win8 Pool: Revised for AutoGO: pP2PInfo->OperatingChannel: %d\n", pP2PInfo->OperatingChannel));
		}
		else if(pP2PDeviceDesc && pP2PDeviceDesc->OperatingChannel != 0)
		{
			pP2PInfo->OperatingChannel = pP2PDeviceDesc->OperatingChannel;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Common Pool: Revised for AutoGO: pP2PInfo->OperatingChannel: %d\n", pP2PInfo->OperatingChannel));
		}
		else
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("No Peer Observed: pP2PInfo->OperatingChannel: %d\n", pP2PInfo->OperatingChannel));

			P2PDeviceListActionInterface(
						pDeviceP2PInfo, 
						P2P_DEVICE_LIST_ACTION_DUMP,
						&pDeviceP2PInfo->DeviceList, 
						NULL, NULL
				);

			P2PDumpScanList(pDeviceP2PInfo->ScanList, pDeviceP2PInfo->ScanListSize);
		}
	}
	// ----------------------------------------------------------------------------------------------------------------------


	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return NDIS_STATUS_SUCCESS;

}


// Set operation for OID_DOT11_WFD_DESIRED_GROUP_ID
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_DESIRED_GROUP_ID(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
#if 0
	typedef struct _DOT11_WFD_GROUP_ID 
	{
    		DOT11_MAC_ADDRESS DeviceAddress;
    		DOT11_SSID SSID;	
	} DOT11_WFD_GROUP_ID, * PDOT11_WFD_GROUP_ID;

	typedef struct _DOT11_SSID
	{
  		ULONG uSSIDLength;
  		UCHAR ucSSID[DOT11_SSID_MAX_LENGTH];
	} DOT11_SSID, *PDOT11_SSID;

	typedef struct DOT11_SSID_LIST {
         NDIS_OBJECT_HEADER Header;
         ULONG uNumOfEntries;
         ULONG uTotalNumOfEntries;
         DOT11_SSID SSIDs[1];
    	} DOT11_SSID_LIST, *PDOT11_SSID_LIST;
#endif

	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	PRT_NDIS6_COMMON	pNdisCommon = pTargetAdapter->pNdisCommon;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	DOT11_SSID_LIST	SsidList;

	// OS-given structure
	PDOT11_WFD_GROUP_ID pParameters = (PDOT11_WFD_GROUP_ID) InformationBuffer;
		
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;
	
	FunctionIn(COMP_OID_SET);

	RT_PRINT_ADDR(COMP_OID_SET, DBG_LOUD, "Target GO MAC Address: ", pParameters->DeviceAddress);
	PlatformMoveMemory(pP2PInfo->DesiredTargetMacAddress, pParameters->DeviceAddress, 6);

	RT_PRINT_STR(COMP_OID_SET, DBG_LOUD, "Target GO SSID: ", pParameters->SSID.ucSSID,  pParameters->SSID.uSSIDLength);
	pP2PInfo->uGroupTargetSSIDLength = pParameters->SSID.uSSIDLength;
	PlatformMoveMemory(pP2PInfo->GroupTargetSSID, pParameters->SSID.ucSSID, pParameters->SSID.uSSIDLength);

	// NdisTest: If no SSID exists
	if(pParameters->SSID.uSSIDLength == 0)
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}
	

	N6_ASSIGN_OBJECT_HEADER(
			SsidList.Header,
			NDIS_OBJECT_TYPE_DEFAULT, 
			DOT11_SSID_LIST_REVISION_1, 
			sizeof(DOT11_SSID_LIST)
		);
	
	SsidList.uNumOfEntries = 1;
	SsidList.SSIDs[0] = pParameters->SSID;

	pNdisCommon->dot11DesiredSSIDList = SsidList;
		
	
	FunctionOut(COMP_OID_SET);
	
	return ndisStatus;

}

// Set operation for OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{

	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER 	pDevicePort = GetFirstDevicePort(pTargetAdapter);
	PP2P_INFO	pDeviceP2PInfo = GET_P2P_INFO(pDevicePort);
	PP2P_DEVICE_DISCRIPTOR pP2PDeviceDesc = NULL;
	PP2P_DEVICE_LIST_ENTRY pP2PDeviceListEntry = NULL;	
	u1Byte i = 0;
		
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	// Change the last scan complete time to block scan request for a while from the default port.
	// If the scan process is executed, the next handshake may fail because this port is not in the
	// correct channel now.
	MultiportRecordLastScanTime(pTargetAdapter);

	if(pP2PInfo->OperatingChannel == 0)
	{
		RT_PRINT_ADDR(COMP_OID_SET, DBG_LOUD, "pP2PInfo->DesiredTargetMacAddress: ", pP2PInfo->DesiredTargetMacAddress);

		// From Win8 Specific Device Information Pool	
		pP2PDeviceListEntry = P2PDeviceListFind(&pDeviceP2PInfo->DeviceList, pP2PInfo->DesiredTargetMacAddress);
		
		// From Common Device Information Pool: Use Interface Address
		pP2PDeviceDesc = P2PScanListFind(pDeviceP2PInfo->ScanList, pDeviceP2PInfo->ScanListSize, NULL, pP2PInfo->DesiredTargetMacAddress, NULL);

		if(pP2PDeviceListEntry)
		{
			pP2PInfo->OperatingChannel = pP2PDeviceListEntry->ChannelNumber;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Win8 Pool: Revised for AutoGO: pP2PInfo->OperatingChannel: %d\n", pP2PInfo->OperatingChannel));
		}
		else if(pP2PDeviceDesc && pP2PDeviceDesc->OperatingChannel != 0)
		{
			pP2PInfo->OperatingChannel = pP2PDeviceDesc->OperatingChannel;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Common Pool: Revised for AutoGO: pP2PInfo->OperatingChannel: %d\n", pP2PInfo->OperatingChannel));
		}
		else
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("No Peer Observed: pP2PInfo->OperatingChannel: %d\n", pP2PInfo->OperatingChannel));

			P2PDeviceListActionInterface(
						pDeviceP2PInfo, 
						P2P_DEVICE_LIST_ACTION_DUMP,
						&pDeviceP2PInfo->DeviceList, 
						NULL, NULL
				);

			P2PDumpScanList(pDeviceP2PInfo->ScanList, pDeviceP2PInfo->ScanListSize);
		}
	}

#if 0
	// Switch band
	if(IS_DUAL_BAND_SUPPORT(pTargetAdapter))
	{
		WIRELESS_MODE wirelessmode;
		
		if(IS_WIRELESS_MODE_5G(pTargetAdapter))
		{
			if(pP2PInfo->OperatingChannel < 14)
			{
				wirelessmode = WIRELESS_MODE_N_24G;
				RT_TRACE(COMP_MLME, DBG_LOUD,("Change to right 2G wirlessmode.\n"));
				pTargetAdapter->HalFunc.SetWirelessModeHandler(pTargetAdapter, (u1Byte)(wirelessmode));
			}
		}
		else if(IS_WIRELESS_MODE_24G(pTargetAdapter))
		{
			if(pP2PInfo->OperatingChannel > 14)
			{
				wirelessmode = WIRELESS_MODE_N_5G;
				RT_TRACE(COMP_MLME, DBG_LOUD,("Change to right 5G wirlessmode .\n"));
				pTargetAdapter->HalFunc.SetWirelessModeHandler(pTargetAdapter, (u1Byte)(wirelessmode));
			}
		}
	}
#else	
	RT_TRACE_F(COMP_MLME, DBG_LOUD, ("ChangeWirelessModeHandler\n"));
	HalChangeWirelessMode(pTargetAdapter, pP2PInfo->OperatingChannel);

//	SetupWirelessMode(pTargetAdapter, pP2PInfo->OperatingChannel); 
#endif
	// Swith to the Right Target Channel specified in OID_DOT11_WFD_GROUP_JOIN_PARAMETERS --
	MgntActSet_802_11_CHANNEL_AND_BANDWIDTH(
			pTargetAdapter, 
			pP2PInfo->OperatingChannel, 
			CHANNEL_WIDTH_20, 
			EXTCHNL_OFFSET_NO_EXT, 
			EXTCHNL_OFFSET_NO_EXT, 
			0
		);	
	// -------------------------------------------------------------------------------

	if(P2P_CLIETN_JOIN_GROUP_WPS_STATE_NONE == pP2PInfo->ClientJoinGroupContext.WpsState)
	{
	pTargetAdapter->pNdis62Common->CurrentOpState = OP_STATE;

	ndisStatus = N6CSet_DOT11_CONNECT_REQUEST(
			pTargetAdapter,
			InformationBuffer,
			InformationBufferLength,
			BytesRead,
			BytesNeeded
		);
	}
	else
	{
		VOID *customScanReq = NULL;
		VOID *customScanInfo = GET_CUSTOM_SCAN_INFO(pTargetAdapter);
		ADAPTER *pDevAdapter = NULL;
		P2P_INFO *pDevP2PInfo = NULL;
		
		pP2PInfo->ClientJoinGroupContext.uWaitForWpsSlotCount = 50;	// Wait for Connection : 5 sec
		PlatformSetTimer(pTargetAdapter, &pP2PInfo->ClientJoinGroupContext.P2PWaitForWpsReadyTimer, 120);

		if(NULL != (pDevAdapter = GetFirstDevicePort(pTargetAdapter))
			&& (NULL != (pDevP2PInfo = GET_P2P_INFO(pDevAdapter)))
			&& NULL != (customScanReq = CustomScan_AllocReq(customScanInfo, NULL, NULL))
			)
		{
			CustomScan_AddScanChnl(customScanReq, pDevP2PInfo->ListenChannel, 
				1, SCAN_PASSIVE, 100, P2P_LOWEST_RATE, NULL);
			
			CustomScan_IssueReq(customScanInfo, customScanReq, CUSTOM_SCAN_SRC_TYPE_P2P, "make cli dev discoverable by GO");
		}
	}

	FunctionOut(COMP_OID_SET);
	
	return ndisStatus;

}

	
// Set operation for OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST
static NDIS_STATUS	
N63C_SET_OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{

	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pTargetAdapter);
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	
	// OS-given structure
	//PDOT11_WFD_GROUP_ID pParameters = (PDOT11_WFD_GROUP_ID) InformationBuffer;
		
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	pTargetAdapter->pNdis62Common->CurrentOpState = INIT_STATE;

	ndisStatus = N6CSet_DOT11_DISCONNECT_REQUEST(
			pTargetAdapter,
			InformationBuffer,
			InformationBufferLength,
			BytesRead,
			BytesNeeded
		);

	// Reset the variable in OID_DOT11_WFD_GROUP_START_PARAMETERS ------------------
	P2PResetClientJoinGroupContext(pP2PInfo);
	// ---------------------------------------------------------------------------
	
	FunctionOut(COMP_OID_SET);
	
	return ndisStatus;

}

NDIS_STATUS
N63C_QUERY_OID_PACKET_COALESCING_FILTER_MATCH_COUNT(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	return ndisStatus;
}

//======================================================================================
// Public Functions 
//======================================================================================

VOID
N63CIndicateP2PEvent(
	PVOID pvP2PInfo, 
	u4Byte EventID,
	PMEMORY_BUFFER pInformation
) 
{
	PP2P_INFO	pP2PInfo = (PP2P_INFO) pvP2PInfo;
	PADAPTER	pAdapter = pP2PInfo->pAdapter;

	if(OS_SUPPORT_WDI(pP2PInfo->pAdapter))
	{
		WDI_IndicateP2PEvent(pvP2PInfo, EventID, pInformation);
		return;
	}

	// rx req action
	if(P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_REQUEST == EventID
		|| P2P_EVENT_RECEIVED_INVITATION_REQUEST == EventID
		)
	{
		N63C_SendAction_TerminateReq(pP2PInfo->pAdapter);

	}

	// rx rsp action
	if(P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE == EventID
		|| P2P_EVENT_RECEIVED_INVITATION_RESPONSE == EventID
		|| P2P_EVENT_RECEIVED_GO_NEGOTIATION_RESPONSE == EventID
		|| P2P_EVENT_RECEIVED_GO_NEGOTIATION_CONFIRM == EventID
		)
	{
		N63C_SendAction_TerminateReq(pP2PInfo->pAdapter);

	}

	switch(EventID)
	{
		case P2P_EVENT_DEVICE_DISCOVERY_COMPLETE:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_DEVICE_DISCOVERY_COMPLETE\n"));
			N63CIndicateDeviceDiscoveryComplete(pP2PInfo);
			break;
			
		case P2P_EVENT_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE\n"));
			N63CIndicateProvisionDiscoveryRequestSendComplete(pP2PInfo, pInformation);
			break;
			
		case P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_REQUEST:
			{
				RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_REQUEST\n"));
				InfoBufLen = 0;
				N63CIndicateReceivedProvisionDiscoveryRequest(pP2PInfo, pInformation);
				if(InfoBufLen)
				{
					if(NULL == N63C_SendAction_PdRsp(pAdapter, (DOT11_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS *)InfoBuf, n63c_SendP2pRspActionFrameStateCb))
					{
						RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("N63C_SendAction_PdRsp failed\n"));
						p2p_IndicateActionFrameSendCompleteWithToken(pP2PInfo, P2P_EVENT_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE, RT_STATUS_FAILURE, NULL, 0, 0);
					}
					InfoBufLen = 0;
				}
			}
			break;
			
		case P2P_EVENT_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE\n"));
			N63CIndicateProvisionDiscoveryResponseSendComplete(pP2PInfo, pInformation);
			break;
			
		case P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE\n"));
			//N63CIndicateReceivedProvisionDiscoveryResponse(pP2PInfo, pInformation);
			break;
			
		case P2P_EVENT_GO_NEGOTIATION_REQUEST_SEND_COMPLETE:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_GO_NEGOTIATION_REQUEST_SEND_COMPLETE\n"));
			N63CIndicateNegotiationRequestSendComplete(pP2PInfo, pInformation);
			break;

		case P2P_EVENT_RECEIVED_GO_NEGOTIATION_REQUEST:
			{
				RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_RECEIVED_GO_NEGOTIATION_REQUEST\n"));
				InfoBufLen = 0;
				N63CIndicateReceivedGONegotiationRequest(pP2PInfo, pInformation);
				if(InfoBufLen)
				{
					N63C_SendAction_TerminateReq(pP2PInfo->pAdapter);

					if(NULL == N63C_SendAction_GoNegRsp(pAdapter, (DOT11_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS *)InfoBuf, n63c_SendP2pRspActionFrameStateCb))
					{
						RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("N63C_SendAction_GoNegRsp failed\n"));
						p2p_IndicateActionFrameSendCompleteWithToken(pP2PInfo, P2P_EVENT_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE, RT_STATUS_FAILURE, NULL, 0, 0);
					}
					InfoBufLen = 0;
				}
			}
			break;
			
		case P2P_EVENT_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE:
			{
				RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE\n"));
				N63CIndicateNegotiationResponseSendComplete(pP2PInfo, pInformation);
			}
			break;

		case P2P_EVENT_RECEIVED_GO_NEGOTIATION_RESPONSE:			
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_RECEIVED_GO_NEGOTIATION_RESPONSE\n"));
			//N63CIndicateReceivedGONegotiationResponse(pP2PInfo, pInformation);
			break;

		case P2P_EVENT_GO_NEGOTIATION_CONFIRM_SEND_COMPLETE:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_GO_NEGOTIATION_CONFIRM_SEND_COMPLETE\n"));
			N63CIndicateNegotiationConfirmSendComplete(pP2PInfo, pInformation);
			break;

		case P2P_EVENT_RECEIVED_GO_NEGOTIATION_CONFIRM:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_RECEIVED_GO_NEGOTIATION_CONFIRM\n"));
			//N63CIndicateReceivedGONegotiationConfirm(pP2PInfo, pInformation);
			break;

		case P2P_EVENT_INVITATION_REQUEST_SEND_COMPLETE:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_INVITATION_REQUEST_SEND_COMPLETE\n"));
			N63CIndicateInvitationRequestSendComplete(pP2PInfo, pInformation);
			break;

		case P2P_EVENT_RECEIVED_INVITATION_REQUEST:
			{
				RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_RECEIVED_INVITATION_REQUEST\n"));
				InfoBufLen = 0;
				N63CIndicateReceivedInvitationRequest(pP2PInfo, pInformation);
				if(InfoBufLen)
				{
					if(NULL == N63C_SendAction_InvitationRsp(pAdapter, (DOT11_SEND_INVITATION_RESPONSE_PARAMETERS *)InfoBuf, n63c_SendP2pRspActionFrameStateCb))
					{
						RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("N63C_SendAction_InvitationRsp failed\n"));
						p2p_IndicateActionFrameSendCompleteWithToken(pP2PInfo, P2P_EVENT_INVITATION_RESPONSE_SEND_COMPLETE, RT_STATUS_FAILURE, NULL, 0, 0);
					}
					InfoBufLen = 0;
				}
			}
			break;

		case P2P_EVENT_INVITATION_RESPONSE_SEND_COMPLETE:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_INVITATION_RESPONSE_SEND_COMPLETE\n"));
			N63CIndicateInvitationResponseSendComplete(pP2PInfo, pInformation);
			break;

		case P2P_EVENT_RECEIVED_INVITATION_RESPONSE:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_RECEIVED_INVITATION_RESPONSE\n"));
			//N63CIndicateReceivedInvitationResponse(pP2PInfo, pInformation);
			break;
			
		case P2P_EVENT_GO_OPERATING_CHANNEL:
			RT_TRACE_F(COMP_INDIC | COMP_P2P, DBG_LOUD, ("P2P_EVENT_GO_OPERATING_CHANNEL\n"));
			N63CIndicateGOOperatingChannel(pP2PInfo, pInformation);
			break;
			
		case P2P_EVENT_NONE:
		default:
			RT_TRACE_F(COMP_P2P, DBG_TRACE, ("EventID (0x%08X) Unrecognized!\n", EventID));
			break;
	}
}


NDIS_STATUS
N63C_OID_DOT11_WFD_DEVICE_CAPABILITY(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_DEVICE_CAPABILITY(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_GROUP_OWNER_CAPABILITY(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_GROUP_OWNER_CAPABILITY(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_DEVICE_INFO(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_DEVICE_INFO(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_DISCOVER_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_DISCOVER_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_GET_DIALOG_TOKEN(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N63C_QUERY_OID_DOT11_WFD_GET_DIALOG_TOKEN(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_ENUM_DEVICE_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N63C_QUERY_OID_DOT11_WFD_ENUM_DEVICE_LIST(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N63C_QUERY_OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
		
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N63C_QUERY_OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
		
	return ndisStatus;
}

	
NDIS_STATUS
N63C_OID_DOT11_WFD_ADDITIONAL_IE(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_ADDITIONAL_IE(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
		
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_FLUSH_DEVICE_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_FLUSH_DEVICE_LIST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
		
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_STOP_DISCOVERY(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_STOP_DISCOVERY(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
		
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);
	
	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
		
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRMATION(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus =N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRM(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
		
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_SEND_INVITATION_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_SEND_INVITATION_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_SEND_INVITATION_RESPONSE(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_SEND_INVITATION_RESPONSE(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
		
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);
		
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_DESIRED_GROUP_ID(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_DESIRED_GROUP_ID(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
}


NDIS_STATUS
N63C_OID_DOT11_WFD_START_GO_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_START_GO_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));
	
	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_GROUP_START_PARAMETERS(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_GROUP_START_PARAMETERS(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));			
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_DISCONNECT_FROM_GROUP_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_WFD_GROUP_JOIN_PARAMETERS(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));			
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_WFD_GROUP_JOIN_PARAMETERS(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
}

//
//  Note : This OID not define in NDIS_63 .
//		 It may support in Vista or Win7 futher. 
//		So it may remove to N6CSet_ , if N6 support !!
//
//
static NDIS_STATUS
N63C_SET_OID_DOT11_OFFLOAD_NETWORK_LIST(
	IN	PADAPTER			pTargetAdapter,
	IN	NDIS_OID			Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	PDOT11_OFFLOAD_NETWORK_LIST_INFO pONLInfo = NULL;
	u4Byte							Index = 0;
	PADAPTER 						pDefaultAdapter = GetDefaultAdapter(pTargetAdapter);
	PMGNT_INFO						pMgntInfo = &( pDefaultAdapter->MgntInfo);
	PRT_NDIS6_COMMON				pNdisCommon = pDefaultAdapter->pNdisCommon;
	PRT_NLO_INFO					pNLOInfo = &(pMgntInfo->NLOInfo);
	PDOT11_OFFLOAD_NETWORK			pONList;
	u4Byte							ChannelIndex = 0;
	NDIS_STATUS 						ndisStatus = NDIS_STATUS_SUCCESS;
	
	pONLInfo = (PDOT11_OFFLOAD_NETWORK_LIST_INFO) InformationBuffer;

	RT_TRACE( (COMP_MLME|COMP_OID_SET), DBG_LOUD , (" ===>N63CSet_OID_DOT11_OFFLOAD_NETWORK_LIST\n") );

	if( InformationBufferLength <  (ULONG)DOT11_MIN_SIZEOF_OFFLOAD_NETWORK_LIST_INFO_REVISION_1)
	{
		ndisStatus = NDIS_STATUS_INVALID_LENGTH;
		*BytesNeeded = sizeof(DOT11_OFFLOAD_NETWORK_LIST_INFO);
		RT_TRACE_F( (COMP_MLME|COMP_OID_SET), DBG_LOUD , ("ndisStatus = NDIS_STATUS_INVALID_LENGTH\n") );
		return ndisStatus;
	}

	switch(pONLInfo->ulFlags)
	{
		case DOT11_NLO_FLAG_STOP_NLO_INDICATION:
			RT_TRACE( (COMP_MLME|COMP_OID_SET), DBG_LOUD, ("Set DOT11_NLO_FLAG_STOP_NLO_INDICATION\n"));
			break;
		case DOT11_NLO_FLAG_SCAN_ON_AOAC_PLATFORM:
			RT_TRACE( (COMP_MLME|COMP_OID_SET), DBG_LOUD, ("Set DOT11_NLO_FLAG_SCAN_ON_AOAC_PLATFORM\n"));
			break;
		case DOT11_NLO_FLAG_SCAN_AT_SYSTEM_RESUME:
			RT_TRACE( (COMP_MLME|COMP_OID_SET), DBG_LOUD, ("Set DOT11_NLO_FLAG_SCAN_AT_SYSTEM_RESUME\n"));
			break;
		default:
			break;
	}

	pNdisCommon->Oidcounter++;
	if( pONLInfo->uNumOfEntries == 0 )
	{
		// Stop NLO mode !!
		pNLOInfo->NumDot11OffloadNetwork = 0;
		pNLOInfo->FastScanIterations = 0;
		pNLOInfo->FastScanPeriod = 0;
		pNLOInfo->SlowScanPeriod = 0;
		pNdisCommon->ScanPeriod = 0;
		
		RT_TRACE( (COMP_MLME|COMP_OID_SET), DBG_LOUD , (" ===>pONLInfo->uNumOfEntries == 0 \n") );
		return NDIS_STATUS_SUCCESS;
	}
	else
	{
		RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, "NLO INFO: \n", InformationBuffer, sizeof(DOT11_OFFLOAD_NETWORK_LIST_INFO));
		pNLOInfo->NumDot11OffloadNetwork =  pONLInfo->uNumOfEntries;
		pNLOInfo->FastScanIterations = pONLInfo->FastScanIterations;
		pNLOInfo->FastScanPeriod = pONLInfo->FastScanPeriod ;
		pNLOInfo->SlowScanPeriod = pONLInfo->SlowScanPeriod ;


		RT_TRACE(COMP_MLME , DBG_LOUD , (" NumDot11OffloadNetwork : %d\n", pNLOInfo->NumDot11OffloadNetwork) );
		RT_TRACE(COMP_MLME , DBG_LOUD , (" FastScanIterations : %d\n", pNLOInfo->FastScanIterations) );
		RT_TRACE(COMP_MLME , DBG_LOUD , (" FastScanPeriod : %d sec\n", pNLOInfo->FastScanPeriod) );
		RT_TRACE(COMP_MLME , DBG_LOUD , (" SlowScanPeriod : %d sec\n", pNLOInfo->SlowScanPeriod) );

		pNdisCommon->bNLOActiveScan 	= TRUE;
		pNdisCommon->bFilterHiddenAP 	= FALSE;

		// Start the NLO scan when the first PlatformHandleNLOnScanRequest is called
		pNdisCommon->ScanPeriod = pNLOInfo->FastScanPeriod;
	}


	if( pONLInfo->uNumOfEntries >  NATIVE_802_11_MAX_NETWORKOFFLOAD_SIZE)
	{
		ndisStatus = NDIS_STATUS_INVALID_LENGTH;
		return ndisStatus;
	}


	pONList = pONLInfo->offloadNetworkList;
	
	for( Index = 0 ; Index <  pONLInfo->uNumOfEntries ; Index++ )
	{
		// Copy SSID 
		CopySsid( pNLOInfo->dDot11OffloadNetworkList[Index].ssidbuf , 
				 pNLOInfo->dDot11OffloadNetworkList[Index].ssidlen,
				 pONList[Index].Ssid.ucSSID ,
				 pONList[Index].Ssid.uSSIDLength);

		RT_PRINT_STR(COMP_MLME, DBG_LOUD , ("N63CSet_OID_DOT11_OFFLOAD_NETWORK_LIST : SSID\n"), pNLOInfo->dDot11OffloadNetworkList[Index].ssidbuf, pNLOInfo->dDot11OffloadNetworkList[Index].ssidlen);

		// Set chuiper !!
		switch(  pONList[Index].UnicastCipher )
		{
			case DOT11_CIPHER_ALGO_NONE :
				pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_NONE;
				pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
				pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= 0;
				break;
			case DOT11_CIPHER_ALGO_WEP40 :
			case DOT11_CIPHER_ALGO_WEP104 :
			case DOT11_CIPHER_ALGO_WEP:
				pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_NONE;
				pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
				pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= 0x0;
				break;
			case DOT11_CIPHER_ALGO_TKIP :  
				if(	pONList[Index].AuthAlgo == DOT11_AUTH_ALGO_WPA || 
				     	pONList[Index].AuthAlgo == DOT11_AUTH_ALGO_WPA_PSK )
				{
					pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA;
					pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
					pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA_TKIP;
				}
				else if( pONList[Index].AuthAlgo == DOT11_AUTH_ALGO_RSNA || 
				     	    pONList[Index].AuthAlgo == DOT11_AUTH_ALGO_RSNA_PSK )
				{
					pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA2;
					pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
					pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA2_TKIP;
				}else
				{
					pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA;
					pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
					pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA_TKIP;
					RT_TRACE( (COMP_MLME|COMP_OID_SET), DBG_LOUD , (" ===>NLO Not define TKIP\n") );
				}
				break;
			case DOT11_CIPHER_ALGO_CCMP :
				if(	pONList[Index].AuthAlgo == DOT11_AUTH_ALGO_WPA || 
				     	pONList[Index].AuthAlgo == DOT11_AUTH_ALGO_WPA_PSK )
				{
					pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA;
					pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
					pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA_AES;
				}
				else if( pONList[Index].AuthAlgo == DOT11_AUTH_ALGO_RSNA || 
				     	    pONList[Index].AuthAlgo == DOT11_AUTH_ALGO_RSNA_PSK )
				{
					pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA2;
					pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
					pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA2_AES;
				}
				else
				{
					pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA2;
					pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
					pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA2_AES;
					RT_TRACE( (COMP_MLME|COMP_OID_SET), DBG_LOUD , (" ===>NLO Not define AES\n") );
				}
				break;
			default :
				pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_NONE;
				pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
				pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= 0;
				RT_TRACE( (COMP_MLME|COMP_OID_SET), DBG_LOUD , (" ===> Unknow UnicastCipher : %x\n",pONList[Index].UnicastCipher) );
				break;
		}

		RT_TRACE(COMP_MLME , DBG_LOUD , (" Seclvl : %d\n", pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl) );
		RT_TRACE(COMP_MLME , DBG_LOUD , (" bPrivacy : %d\n", pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy) );
		RT_TRACE(COMP_MLME , DBG_LOUD , (" chiper : %d\n", pNLOInfo->dDot11OffloadNetworkList[Index].chiper) );

		// Save channel info 
		// DOT11_MAX_CHANNEL_HINTS 4
		for(  ChannelIndex = 0 ;  ChannelIndex < 4 ;  ChannelIndex++ )
		{
			pNLOInfo->dDot11OffloadNetworkList[Index].channelNumberHit[ChannelIndex] = pONList[Index].Dot11ChannelHints[ChannelIndex].uChannelNumber;
		}
		
		// Show Entry information !! 
		RT_TRACE( (COMP_MLME|COMP_OID_SET), DBG_LOUD , (" ===> Show NLO info Index (%x):\n",Index) );
		RT_PRINT_STR((COMP_MLME|COMP_OID_SET), DBG_LOUD, (" SSID :\n"), pNLOInfo->dDot11OffloadNetworkList[Index].ssidbuf, pNLOInfo->dDot11OffloadNetworkList[Index].ssidlen);
		RT_TRACE( (COMP_MLME|COMP_OID_SET) , DBG_LOUD , ( "chiper : %x  \n", pNLOInfo->dDot11OffloadNetworkList[Index].chiper) )
		
	}

	return ndisStatus;
}

NDIS_STATUS
N63C_OID_DOT11_OFFLOAD_NETWORK_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		//ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		//if(ndisStatus != NDIS_STATUS_SUCCESS) {
		//	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));			
		//	break;
		//}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_OFFLOAD_NETWORK_LIST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
}

static NDIS_STATUS
N63C_SET_OID_DOT11_POWER_MGMT_MODE_AUTO_ENABLED(
	IN	PADAPTER			pTargetAdapter,
	IN	NDIS_OID			Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	/*
	#define DOT11_POWER_MGMT_AUTO_MODE_ENABLED_REVISION_1 
	#define DOT11_SIZEOF_POWER_MGMT_AUTO_MODE_ENABLED_REVISION_1 
	typedef struct _ DOT11_POWER_MGMT_MODE_AUTO_INFO { 
	 		NDIS_OBJECT_HEADER Header; 
			BOOLEAN bEnabled; 
	} DOT11_POWER_MGMT_AUTO_MODE_ENABLED_INFO, * PDOT11_POWER_MGMT_AUTO_MODE_ENABLED_INFO; 
*/
	PDOT11_POWER_MGMT_AUTO_MODE_ENABLED_INFO	pPMGAuModInf;
	PADAPTER 									pDefaultAdapter = GetDefaultAdapter(pTargetAdapter);
	PMGNT_INFO									pMgntInfo = &( pDefaultAdapter->MgntInfo);


	if( InformationBufferLength  < DOT11_SIZEOF_POWER_MGMT_AUTO_MODE_ENABLE_INFO_REVISION_1  )
	{
		*BytesNeeded = DOT11_SIZEOF_POWER_MGMT_AUTO_MODE_ENABLE_INFO_REVISION_1;
		return NDIS_STATUS_INVALID_LENGTH;
		
	}
	
	pPMGAuModInf = (PDOT11_POWER_MGMT_AUTO_MODE_ENABLED_INFO)InformationBuffer;


	pMgntInfo->bInAutoPowerSavemode = pPMGAuModInf->bEnabled;

	RT_TRACE( COMP_TEST, DBG_LOUD , ("===>OID_DOT11_POWERMGMT_MODE_AUTO_ENABLED =  %d \n",pMgntInfo->bInAutoPowerSavemode) );
	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
N63C_OID_DOT11_POWER_MGMT_MODE_AUTO_ENABLED(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		//ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		//if(ndisStatus != NDIS_STATUS_SUCCESS) {
		//	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));			
		//	break;
		//}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N63C_SET_OID_DOT11_POWER_MGMT_MODE_AUTO_ENABLED(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
}


static NDIS_STATUS
N63C_QUERY_OID_DOT11_POWER_MGMT_MODE_STATUS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PDOT11_POWER_MGMT_MODE_STATUSINFO		pDot11PMStaInfo = NULL;
	PMGNT_INFO	pMgntInfo = &pTargetAdapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

				
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------



	// This has errors. Please contact CCW. 
	return NDIS_STATUS_FAILURE;


		
	if( !pMgntInfo->bInAutoPowerSavemode )
	{
		pDot11PMStaInfo->PowerSaveMode 	= dot11_power_mode_active;
		pDot11PMStaInfo->uPowerSaveLevel 	= DOT11_POWER_SAVING_NO_POWER_SAVING;
		pDot11PMStaInfo->Reason			= dot11_power_mode_reason_no_change;
	}
	else 
	{
		if(pMgntInfo->dot11PowerSaveMode == eActive)
		//if( pPSC->PowerSaveLevel == POWER_SAVING_NO_POWER_SAVING)
		{
			pDot11PMStaInfo->PowerSaveMode 	= dot11_power_mode_active;
			pDot11PMStaInfo->uPowerSaveLevel 	= DOT11_POWER_SAVING_NO_POWER_SAVING;
			pDot11PMStaInfo->Reason			= dot11_power_mode_reason_no_change;
		}
		else if( pMgntInfo->dot11PowerSaveMode == eMaxPs )
		//else if( pPSC->PowerSaveLevel == POWER_SAVING_MAX_PSP )
		{
			pDot11PMStaInfo->PowerSaveMode 	= dot11_power_mode_powersave;
			pDot11PMStaInfo->uPowerSaveLevel 	= DOT11_POWER_SAVING_MAX_PSP;
			pDot11PMStaInfo->Reason			= dot11_power_mode_reason_compliant_AP;
		}
		else if( pMgntInfo->dot11PowerSaveMode == eFastPs )
		//else if( pPSC->PowerSaveLevel == POWER_SAVING_FAST_PSP  )
		{
			pDot11PMStaInfo->PowerSaveMode 	= dot11_power_mode_powersave;
			pDot11PMStaInfo->uPowerSaveLevel 	= DOT11_POWER_SAVING_FAST_PSP;
			pDot11PMStaInfo->Reason			= dot11_power_mode_reason_compliant_AP;
		}
		//else if(pPSC->PowerSaveLevel == POWER_SAVING_MAXIMUM_LEVEL )
		//{
		//	pDot11PMStaInfo->PowerSaveMode 	= dot11_power_mode_powersave;
		//	pDot11PMStaInfo->uPowerSaveLevel 	= DOT11_POWER_SAVING_MAXIMUM_LEVEL;
		//	pDot11PMStaInfo->Reason			= dot11_power_mode_reason_compliant_AP;
		//}
		else 
		{
			pDot11PMStaInfo->PowerSaveMode 	= dot11_power_mode_powersave;
			pDot11PMStaInfo->uPowerSaveLevel 	= DOT11_POWER_SAVING_FAST_PSP;
			pDot11PMStaInfo->Reason			= dot11_power_mode_reason_compliant_AP;
		}
	}

	RT_TRACE( COMP_TEST, DBG_LOUD , ("===>_DOT11_POWER_MGMT_MODE_STATUS  \n") );
	
	return ndisStatus;
}


NDIS_STATUS
N63C_OID_DOT11_POWER_MGMT_MODE_STATUS(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		//ndisStatus = N63CValidateOIDCorrectness(pAdapter, NdisRequest);
		//if(ndisStatus != NDIS_STATUS_SUCCESS) {
		//	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));			
		//	break;
		//}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N63C_QUERY_OID_DOT11_POWER_MGMT_MODE_STATUS(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
}

NDIS_STATUS
N63C_OID_PACKET_COALESCING_FILTER_MATCH_COUNT(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	FunctionIn(COMP_OID_SET | COMP_OID_QUERY);

	do
	{
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N63C_QUERY_OID_PACKET_COALESCING_FILTER_MATCH_COUNT(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("ndisStatus: %d\n", ndisStatus));

	FunctionOut(COMP_OID_SET | COMP_OID_QUERY);
	
	return ndisStatus;
	
}
