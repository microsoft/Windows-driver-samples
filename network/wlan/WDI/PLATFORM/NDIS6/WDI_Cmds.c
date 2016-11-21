#include "MP_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "WDI_Cmds.tmh"
#endif

#include "P2P_Internal.h"

#include "WDI_Cmds.h"
#include "WDI_Xlat.h"


static u1Byte	BroadcastAddress[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static
VOID
wdi_task_WriteResult(
	IN  RT_OID_HANDLER			*hTask,
	IN  UINT8					*tlv,
	IN  ULONG					tlvlen,
	IN  NDIS_STATUS				status
	)
{
	WDI_MESSAGE_HEADER			*hdr = (WDI_MESSAGE_HEADER *)hTask->pInputBuffer;

	hdr->Status = status;
	if(tlvlen)
	{
		NdisMoveMemory((u1Byte *)hdr + sizeof(*hdr), tlv, tlvlen);
	}
	
	hTask->OutputBufferLength = tlvlen;
}

static
VOID
wdi_ScanCb(
	IN  CUSTOM_SCAN_STATE		state,
	IN  VOID					*pCtx
	)
{
	ADAPTER						*pAdapter = (ADAPTER *)pCtx;
		
	RT_TRACE_F(COMP_MLME, DBG_LOUD, ("%s\n", CustomScan_ScanStateTxt(state)));

	if(CUSTOM_SCAN_STATE_PRE_DESTROY == state)
	{
		WDI_IndicateScanComplete(pAdapter, RT_STATUS_SUCCESS);
	}
}

static
VOID
wdi_P2PDiscoveryCb(
	IN  CUSTOM_SCAN_STATE		state,
	IN  VOID					*pCtx
	)
{
	ADAPTER						*pAdapter = (ADAPTER *)pCtx;
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("%s\n", CustomScan_ScanStateTxt(state)));

	if(CUSTOM_SCAN_STATE_COMPLETED == state)
	{
		WDI_TASK_P2P_DISCOVER_PARAMETERS *param = hTask->tlvParser.parsedTlv.paramP2pDiscover;
		
		MGNT_INFO				*mgnt = &pAdapter->MgntInfo;
		u4Byte					it = 0;

		// copy to 4Q
		mgnt->NumBssDesc4Query = mgnt->NumBssDesc;
		for(it = 0; it < mgnt->NumBssDesc4Query; it++)
		{
			CopyWlanBss(mgnt->bssDesc4Query + it, mgnt->bssDesc + it);
		}

		// copy to cli
		if(param && param->Optional.SSIDList_IsPresent)
		{// specific peer disocvery
			ADAPTER				*cli = GetFirstClientPort(pAdapter);

			if(cli && GetDefaultAdapter(pAdapter) != cli)
			{
				MGNT_INFO		*cliMgnt = &cli->MgntInfo;

				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("copy %u bss(s) from port %u to cli port\n", mgnt->NumBssDesc4Query, GET_PORT_NUMBER(pAdapter)));

				if(cliMgnt->NumBssDesc4Query < mgnt->NumBssDesc4Query)
				{
					cliMgnt->NumBssDesc4Query = mgnt->NumBssDesc4Query;
					for(it = 0; it < mgnt->NumBssDesc4Query; it++)
					{
						CopyWlanBss(cliMgnt->bssDesc4Query + it, mgnt->bssDesc4Query + it);
					}
				}
			}
		}
	}
	else if(CUSTOM_SCAN_STATE_PRE_DESTROY == state)
	{
		WDI_CMD_P2PDiscoverComplete(pAdapter, NDIS_STATUS_SUCCESS);
	}

	return;
}

static
VOID
wdi_ConstructScanReq(
	IN  ADAPTER				*pAdapter,
	IN  WDI_SCAN_PARAMETERS	*param,
	OUT VOID				*req
	)
{
	RT_SCAN_TYPE			scanType = WDI_SCAN_TYPE_PASSIVE_ONLY == param->ScanModeParameters.AllowedScanType ? SCAN_PASSIVE : SCAN_ACTIVE;
	u2Byte					duration = (u2Byte)(SCAN_ACTIVE == scanType ? param->DwellTime.ActiveChannelDwellTime : param->DwellTime.PassiveChannelDwellTime);

	FunctionIn(COMP_SCAN);
	
	if(param->Optional.BandChannelList_IsPresent)
	{
		u4Byte				itBand = 0;
		u4Byte				itChnl = 0;
		
		for(itBand = 0; itBand < param->BandChannelList.ElementCount; itBand++)
		{
			for(itChnl = 0; itChnl < param->BandChannelList.pElements[itBand].ChannelList.ElementCount; itChnl++)
			{
				u1Byte		chnl = (u1Byte)param->BandChannelList.pElements[itBand].ChannelList.pElements[itChnl];
				
				CustomScan_AddScanChnl(req, chnl, 1, scanType, duration, 0, NULL);
			}
		}
	}
	else
	{
		RT_CHANNEL_DOMAIN	domain = GetDefaultAdapter(pAdapter)->MgntInfo.ChannelPlan;
		RT_CHANNEL_PLAN		*plan = NULL;
		
		RtActChannelList(pAdapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_PLAN, &domain, &plan);

		CustomScan_AddChnlPlanChnls(req, plan, scanType, duration, 0, NULL);
	}

	if(WDI_SCAN_TYPE_PASSIVE_ONLY == param->ScanModeParameters.AllowedScanType)
		CustomScan_ForcePassiveScan(req);

	FunctionOut(COMP_SCAN);

	return;
}

static
VOID
wdi_ConstructP2PSpecificChnlReq(
	IN  ADAPTER				*pAdapter,
	IN  WDI_TASK_P2P_DISCOVER_PARAMETERS *param,
	IN  FRAME_BUF			*probeReq,
	OUT VOID				*req
	)
{
	RT_SCAN_TYPE			scanType = WDI_P2P_SCAN_TYPE_PASSIVE == param->DiscoverMode.ScanType ? SCAN_PASSIVE : SCAN_ACTIVE;
	u2Byte					duration = (u2Byte)(SCAN_ACTIVE == scanType ? param->DwellTime.ActiveChannelDwellTime : param->DwellTime.PassiveChannelDwellTime);
	
	if(param->Optional.DiscoveryChannelSettings_IsPresent)
	{
		u4Byte				itSettings = 0;
		u4Byte				itBand = 0;
		u4Byte				itChnl = 0;

		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("channel setting present\n"));
		
		for(itSettings = 0; itSettings < param->DiscoveryChannelSettings.ElementCount; itSettings++)
		{
			WDI_P2P_DISCOVERY_CHANNEL_SETTINGS_CONTAINER *setting = param->DiscoveryChannelSettings.pElements + itSettings;
			
			for(itBand = 0; itBand < setting->BandChannelList.ElementCount; itBand++)
			{
				WDI_BAND_CHANNEL_LIST_CONTAINER *band = setting->BandChannelList.pElements + itBand;
				
				for(itChnl = 0; itChnl < band->ChannelList.ElementCount; itChnl++)
				{
					WDI_CHANNEL_NUMBER *chnl = band->ChannelList.pElements + itChnl;

					RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("adding ch: %u for %u ms\n", *chnl, duration));
					CustomScan_AddScanChnl(req, (u1Byte)*chnl, 16, scanType, duration, MGN_6M, probeReq);
				}
			}
		}
	}

	return;
}

static
VOID
wdi_ConstructP2PScanPhaseReq(
	IN  ADAPTER				*pAdapter,
	IN  WDI_TASK_P2P_DISCOVER_PARAMETERS *param,
	OUT VOID				*req
	)
{
	RT_SCAN_TYPE			scanType = WDI_P2P_SCAN_TYPE_PASSIVE == param->DiscoverMode.ScanType ? SCAN_PASSIVE : SCAN_ACTIVE;
	u2Byte					duration = (u2Byte)(SCAN_ACTIVE == scanType ? param->DwellTime.ActiveChannelDwellTime : param->DwellTime.PassiveChannelDwellTime);
	RT_CHANNEL_DOMAIN		domain = GetDefaultAdapter(pAdapter)->MgntInfo.ChannelPlan;
	RT_CHANNEL_PLAN			*plan = NULL;
	
	RtActChannelList(pAdapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_PLAN, &domain, &plan);
	CustomScan_AddChnlPlanChnls(req, plan, scanType, duration, MGN_6M, NULL);

	return;
}

static
VOID
wdi_ConstructP2PFindPhaseReq(
	IN  ADAPTER				*pAdapter,
	IN  WDI_TASK_P2P_DISCOVER_PARAMETERS *param,
	IN  FRAME_BUF			*probeReq,
	OUT VOID				*req
	)
{
	P2P_INFO				*info = GET_P2P_INFO(pAdapter);
	u2Byte					rand = (u2Byte)GetRandomNumber(0, 3); // [0, 1, 2]

	do
	{
		// listen
		CustomScan_AddScanChnl(req, info->ListenChannel, 1, SCAN_PASSIVE, 100 * (rand + 1), 0, NULL);

		// search
		CustomScan_AddScanChnl(req, 1, 1, SCAN_ACTIVE, 100, MGN_6M, probeReq);
		CustomScan_AddScanChnl(req, 6, 1, SCAN_ACTIVE, 100, MGN_6M, probeReq);
		CustomScan_AddScanChnl(req, 11, 1, SCAN_ACTIVE, 100, MGN_6M, probeReq);

		rand = (rand + 1) % 3;
	}while(CustomScan_NumAddedChnl(req) < MAX_SCAN_CHANNEL_NUM);

	return;
}

static
BOOLEAN
wdi_Construct_FakeInvitationRsp(
	IN  ADAPTER						*pAdapter,
	IN  const WDI_SEND_ACTION_CTX 	*ctx,
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

	FunctionOut(COMP_OID_SET);
	
	return TRUE;
}

static
BOOLEAN
wdi_Construct_FakePdRsp(
	IN  ADAPTER						*pAdapter,
	IN  const WDI_SEND_ACTION_CTX 	*ctx,
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

	FunctionOut(COMP_OID_SET);

	return TRUE;
}

static
VOID
wdi_IndicateRxP2pRspActionFrame(
	IN  ADAPTER					*pAdapter,
	IN  const WDI_SEND_ACTION_CTX *ctx
	)
{
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	P2P_DEV_LIST_ENTRY			*dev = NULL;
	P2P_FRAME_TYPE				ft = P2P_FID_MAX;
	RT_GEN_TEMP_BUFFER			*pGenBuf = NULL;
	FRAME_BUF					fbuf;
	u4Byte						ieOffset = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;

	u1Byte						*buf = NULL;
	ULONG						buflen = 0;
	
	NDIS_STATUS_INDICATION		*indic = NULL;
	WDI_MESSAGE_HEADER			*wdiHdr = NULL;

	WDI_INDICATION_P2P_ACTION_FRAME_RECEIVED_PARAMETERS param = {0};

	FunctionIn(COMP_OID_SET);

	p2p_DevList_Lock(&info->devList);

	do
	{
		if(NULL == (pGenBuf = GetGenTempBuffer(pAdapter, GEN_TEMP_BUFFER_SIZE)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_SERIOUS, ("[ERROR] Memory allocation failed!\n"));
			break;
		}

		FrameBuf_Init(GEN_TEMP_BUFFER_SIZE, 0, (u1Byte *)pGenBuf->Buffer.Ptr, &fbuf);
		
		if(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == ctx->rspType)
			ft = P2P_FID_PD_RSP;
		else if(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE == ctx->rspType)
			ft = P2P_FID_GO_NEG_RSP;
		else if(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM == ctx->rspType)
			ft = P2P_FID_GO_NEG_CONF;
		else if(WDI_P2P_ACTION_FRAME_INVITATION_RESPONSE == ctx->rspType)
			ft = P2P_FID_INV_RSP;
		else
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("invalid rsp frame type specified: %u\n", ctx->rspType));
			break;
		}

		if(ctx->bGo)
		{
			dev = p2p_DevList_GetGo(&info->devList, ctx->da);
		}
		else
		{
			dev = p2p_DevList_Get(&info->devList, ctx->da, P2P_DEV_TYPE_DEV);
		}
		
		if(NULL == dev)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, 
				("dev not found: bGo: %u, mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
				ctx->bGo, ctx->da[0], ctx->da[1], ctx->da[2], ctx->da[3], ctx->da[4], ctx->da[5]
				));
			p2p_DevList_Dump(&info->devList);
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

			if(WDI_P2P_ACTION_FRAME_INVITATION_RESPONSE == ctx->rspType)
			{
				if(!wdi_Construct_FakeInvitationRsp(pAdapter, ctx, dev, &fbuf))
					break;
			}
			else if(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == ctx->rspType)
			{
				if(!wdi_Construct_FakePdRsp(pAdapter, ctx, dev, &fbuf))
					break;
			}
			else
			{
				break;
			}
		}

		// fill param
		param.FrameInfo.Optional.DeviceContext_IsPresent = FALSE;
		param.FrameInfo.FrameParams.ActionFrameType = ctx->rspType;
		cpMacAddr(param.FrameInfo.FrameParams.PeerDeviceAddress.Address, FrameBuf_MHead(&fbuf) + FRAME_OFFSET_ADDRESS2);
		param.FrameInfo.FrameParams.DialogToken = ctx->token;
		param.FrameInfo.FrameIEs.ElementCount = FrameBuf_Length(&fbuf) - ieOffset;
		param.FrameInfo.FrameIEs.pElements = FrameBuf_MHead(&fbuf) + ieOffset;
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("token: %u\n", param.FrameInfo.FrameParams.DialogToken));

		// gen TLV
		if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationP2pActionFrameReceived(
			&param, 
			sizeof(*indic) + sizeof(*wdiHdr), 
			&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
			&buflen, &((UINT8 *)buf)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("gen tlv failed\n"));
			break;
		}

		// fill ptrs
		indic = (NDIS_STATUS_INDICATION *)buf;
		wdiHdr = (WDI_MESSAGE_HEADER *)(buf + sizeof(*indic));
		
		// indication data
		PlatformZeroMemory(indic, sizeof(*indic));
		indic->Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
		indic->Header.Size = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
		indic->Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
		indic->SourceHandle = pAdapter;
		indic->PortNumber = 0;
		indic->StatusCode = NDIS_STATUS_WDI_INDICATION_P2P_ACTION_FRAME_RECEIVED;
		indic->RequestId = 0; // ?
		indic->StatusBuffer = wdiHdr;
		indic->StatusBufferSize = buflen - sizeof(*indic);

		wdiHdr->PortId = (WDI_PORT_ID)(GET_PORT_NUMBER(pAdapter));
		wdiHdr->Reserved = 0;
		wdiHdr->Status = NDIS_STATUS_SUCCESS;
		wdiHdr->TransactionId = 0;
		wdiHdr->IhvSpecificId = 0;

		// indicate
		NdisMIndicateStatusEx(pAdapter->pNdisCommon->hNdisAdapter, indic);
		
		// cleanup
		FreeGenerated((UINT8 *)buf);

	}while(FALSE);

	if(pGenBuf)
	{
		ReturnGenTempBuffer(pAdapter, pGenBuf);
	}

	p2p_DevList_Unlock(&info->devList);
	
	FunctionOut(COMP_OID_SET);

	return;
}

static
BOOLEAN
wdi_SendP2pActionFrameStateCb(
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
wdi_SendP2pReqActionFrameStateCb(
	IN	VOID					*req,
	IN	int 					state,
	IN	VOID					*pCtx
	)
{
	WDI_SEND_ACTION_CTX			*ctx = (WDI_SEND_ACTION_CTX *)pCtx;
	ADAPTER						*pAdapter = ctx->pAdapter;

	wdi_SendP2pActionFrameStateCb(req, state, pCtx);
	
	if(OFF_CHNL_TX_STATE_COMPLETE == state)
	{
		P2P_INFO				*info = GET_P2P_INFO(pAdapter);
		
		WDI_CMD_P2pSendRequestActionFrameComplete(pAdapter, ctx);

		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("restore StateBeforeScan to initialized\n"));
		info->StateBeforeScan = P2P_STATE_INITIALIZED;
	}
	else if(OFF_CHNL_TX_STATE_PRE_DESTROY == state)
	{
		if(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == ctx->rspType
			|| WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE == ctx->rspType
			|| WDI_P2P_ACTION_FRAME_INVITATION_RESPONSE == ctx->rspType
			)
		{
			wdi_IndicateRxP2pRspActionFrame(pAdapter, ctx);
		}	
	}
	else if(OFF_CHNL_TX_STATE_RETRY == state)
	{
		u1Byte					newToken = 0;
		static BOOLEAN			bSkip = TRUE;

		newToken = WDI_SendAction_UpdateTxFrameDialogToken(req, pAdapter, ctx->reqType);

		if(pAdapter->bInHctTest)
		{
			// Not to skip probing under HCT test, because it wastes time 
			// sending action frame to peer on its Listen Channel when peer
			// is not doing extended listen
			bSkip = FALSE;
		}
		
		if(bSkip)
		{
			WDI_SendAction_SkipProbeTemporarilly(req);
		}
		
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("retry with token: %u, reqType: %u, skip probe: %u\n", newToken, ctx->reqType, bSkip));

		bSkip = !bSkip;
	}

	return;
}

static
VOID
wdi_SendP2pRspActionFrameStateCb(
	IN	VOID					*req,
	IN	int 					state,
	IN	VOID					*pCtx
	)
{
	WDI_SEND_ACTION_CTX			*ctx = (WDI_SEND_ACTION_CTX *)pCtx;
	ADAPTER						*pAdapter = ctx->pAdapter;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("state = %d\n", state));

	wdi_SendP2pActionFrameStateCb(req, state, pCtx);
	
	if(OFF_CHNL_TX_STATE_COMPLETE == state)
	{
		WDI_CMD_P2pSendResponseActionFrameComplete(pAdapter, ctx);

		// restore Dialog Token in P2PInfo since we are sending 
		// response action frame, self dialog token shall not be 
		// modified
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("restore token from %u to %u\n", info->DialogToken, ctx->oldToken));
		info->DialogToken = ctx->oldToken;
	}
	else if(OFF_CHNL_TX_STATE_PRE_DESTROY == state)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("ctx->rspType = %d\n", ctx->rspType));
		if(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM == ctx->rspType)
		{
			wdi_IndicateRxP2pRspActionFrame(pAdapter, ctx);
		}	
	}
	else if(OFF_CHNL_TX_STATE_RETRY == state)
	{
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("retry with token: %u, reqType: %u\n", ctx->token, ctx->reqType));
	}

	return;
}

static
BOOLEAN
wdi_SendActionFrameStateCb(
	IN	VOID					*req,
	IN	int 					state,
	IN	VOID					*pCtx
	)
{
	BOOLEAN						bHandled = FALSE;
	N63C_SEND_ACTION_CTX		*ctx = (N63C_SEND_ACTION_CTX *)pCtx;
	ADAPTER						*pAdapter = ctx->pAdapter;
	
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("wdi_SendActionFrameStateCb (0x%x)\n", state));

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

		if(OFF_CHNL_TX_STATE_RETRY == state)
		{
			u1Byte					newToken = 0;
			static BOOLEAN			bSkip = TRUE;

			//newToken = WDI_SendAction_UpdateTxFrameDialogToken(req, pAdapter, ctx->reqType);
			if(bSkip)
			{
				WDI_SendAction_SkipProbeTemporarilly(req);
			}
			
			RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("retry with token: %u, reqType: %u, skip probe: %u\n", newToken, ctx->reqType, bSkip));

			bSkip = !bSkip;

			bHandled = TRUE;
			break;
		}
	}while(FALSE);

	return bHandled;
}


static
VOID
wdi_SendReqActionFrameStateCb(
	IN	VOID					*req,
	IN	int 					state,
	IN	VOID					*pCtx
	)
{
	WDI_SEND_ACTION_CTX			*ctx = (WDI_SEND_ACTION_CTX *)pCtx;
	ADAPTER						*pAdapter = ctx->pAdapter;

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("wdi_SendReqActionFrameStateCb (0x%x)\n", state));

	wdi_SendActionFrameStateCb(req, state, pCtx);
	
	if(OFF_CHNL_TX_STATE_COMPLETE == state)
	{
		WDI_CMD_SendReqActionFrameComplete(pAdapter, ctx);
	}
	else if(OFF_CHNL_TX_STATE_PRE_DESTROY == state)
	{
		// Do nothing here.
		RT_TRACE(COMP_OID_SET, DBG_TRACE, ("wdi_SendReqActionFrameStateCb Pre_Destroy do nothing here\n"));
	}
}

static
VOID
wdi_SendRspActionFrameStateCb(
	IN	VOID					*req,
	IN	int 					state,
	IN	VOID					*pCtx
	)
{
	WDI_SEND_ACTION_CTX			*ctx = (WDI_SEND_ACTION_CTX *)pCtx;
	ADAPTER						*pAdapter = ctx->pAdapter;
	
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("wdi_SendRspActionFrameStateCb (0x%x)\n", state));

	wdi_SendActionFrameStateCb(req, state, pCtx);
	
	if(OFF_CHNL_TX_STATE_COMPLETE == state)
	{
		WDI_CMD_SendRspActionFrameComplete(pAdapter, ctx);
	}
}

NDIS_STATUS
Wdi_Task_Scan(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS				status = NDIS_STATUS_SUCCESS;
	DOT11_SCAN_REQUEST_V2	ScanRequest = {0};
	ULONG					BytesRead = 0, BytesNeeded = 0;
	WDI_SCAN_PARAMETERS	*param = pOidHandle->tlvParser.parsedTlv.paramScan;
	u4Byte					itSsidl = 0;
	WDI_SSID				WdiSsid = {0};
	OCTET_STRING			Ssid = {0,0};

	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	VOID *					req = NULL;
	BOOLEAN					bScanByRoam = FALSE;
	PMGNT_INFO				pMgntInfo =&(pAdapter->MgntInfo);

	RT_TRACE(COMP_OID_SET, DBG_LOUD, 
		("==> Wdi_Task_Scan(): Input InformationBuffer (%d)\n", 
		pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength));

	ScanRequest.dot11BSSType = dot11_BSS_type_infrastructure;
	ScanRequest.uNumOfdot11SSIDs = 0;
	ScanRequest.udot11SSIDsOffset = 0;
	ScanRequest.uRequestIDsOffset = 0;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("WIFI_TLV_BSSID\n"));
	CopyMem(ScanRequest.dot11BSSID, param->BSSID.Address, ETHERNET_ADDRESS_LENGTH);
	RT_PRINT_ADDR(COMP_SCAN, DBG_LOUD, ("ScanRequest \n"), ScanRequest.dot11BSSID);

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("WIFI_TLV_SSID\n"));	

	MgntClearSsidsToScan(pAdapter);
		
	for(itSsidl = 0; itSsidl < param->SSIDList.ElementCount; itSsidl++)
	{
		WdiSsid = param->SSIDList.pElements[itSsidl];
	
		RT_PRINT_STR(COMP_MLME, DBG_LOUD, "===> Wdi_Task_Scan(): ", WdiSsid.pElements, WdiSsid.ElementCount);
				
		Ssid.Octet = WdiSsid.pElements;
		Ssid.Length = (u2Byte)WdiSsid.ElementCount; 
	
		RT_PRINT_STR(COMP_MLME, DBG_LOUD, "===> Wdi_Task_Scan(): SSid ", Ssid.Octet, Ssid.Length);
	
		MgntAddSsidsToScan(pAdapter, Ssid);
	}
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Scan Type\n"));
	if( ((param->ScanModeParameters.ScanTrigger == WDI_SCAN_TRIGGER_ROAM) ||
		(param->ScanModeParameters.ScanTrigger == WDI_SCAN_TRIGGER_FAST_ROAM)) &&
		(pMgntInfo->PrepareRoamState ==RT_PREPARE_ROAM_NORMAL_ROAM_BETTER_AP) )
		bScanByRoam = TRUE;
	else
		bScanByRoam = FALSE;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("WIFI_TLV_SCAN_MODE\n"));
	ScanRequest.dot11ScanType = (DOT11_SCAN_TYPE)param->ScanModeParameters.AllowedScanType;

	ScanRequest.uIEsOffset = 0;
	ScanRequest.uIEsLength = 0;

	if(param->Optional.VendorIEs_IsPresent)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("WIFI_TLV_VENDORIE\n"));
	
		MgntActSet_AdditionalProbeReqIE(pAdapter, 
			(u1Byte *)param->VendorIEs.pElements,
			(u4Byte)param->VendorIEs.ElementCount);
	}	

	// Sinda, 20150512
	// Skip all scan triggered by Roam.
	if( (bScanByRoam == TRUE) )
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Roam trigger scan, so skip it\n"));
		OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
	}
	else if(TRUE == pAdapter->pPortCommonInfo->WdiData.bRFOffSwitchProgress)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set RF in progress, skip scan\n"));
		OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
	}
	else
	{
		status = N6CSet_DOT11_SCAN_REQUEST(pAdapter,
									&ScanRequest,
									(ULONG)sizeof(ScanRequest),
									&BytesRead,
									&BytesNeeded);

		if(pNdisCommon->bToIndicateScanComplete)
		{
			if(param->Optional.VendorIEs_IsPresent)
			{
				MgntActSet_AdditionalProbeReqIE(pAdapter, 
					(u1Byte *)param->VendorIEs.pElements,
					(u4Byte)param->VendorIEs.ElementCount);
			}

			if(NULL != (req = CustomScan_AllocReq(GET_CUSTOM_SCAN_INFO(pAdapter), NULL, NULL)))
			{
				wdi_ConstructScanReq(pAdapter, param, req);
				
				CustomScan_SetTimeBound(req, param->DwellTime.MaximumScanTime);
				CustomScan_SetupCbCtx(req, wdi_ScanCb, pAdapter);
				CustomScan_SetRepeatCount(req, param->ScanModeParameters.ScanRepeatCount);
				
				CustomScan_IssueReq(GET_CUSTOM_SCAN_INFO(pAdapter), req, CUSTOM_SCAN_SRC_TYPE_SYS, "sys");
			}
		}
		else
		{
			WDI_IndicateScanComplete(pAdapter, RT_STATUS_FAILURE);
		}
	}
					
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== Wdi_Task_Scan()\n"));
	return status;
}


NDIS_STATUS
Wdi_Task_Connect(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	ULONG					InputBufferLength = pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
	PRT_SECURITY_T			pSecInfo = &(pAdapter->MgntInfo.SecurityInfo);
	WDI_TASK_CONNECT_PARAMETERS	*Params = pOidHandle->tlvParser.parsedTlv.paramConnect;
	PWDI_CONNECT_BSS_ENTRY_CONTAINER connectBssEntry = NULL;
	PMGNT_INFO			pMgntInfo=&pAdapter->MgntInfo;
	
	PWDI_MESSAGE_HEADER	pWdiHeader 
		= (PWDI_MESSAGE_HEADER)pOidHandle->pInputBuffer;

	//Save PMKID			
	u4Byte					ulIndex, i, j;
	BOOLEAN					blInserted;
	u1Byte 					bssidcount=0;
	int 					NumPreferredBSSID = Params->PreferredBSSEntryList.ElementCount;	

	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> Wdi_Task_Connect()\n"));

	pMgntInfo->RegFakeRoamSignal[0] = 0;

	if(NDIS_STATUS_SUCCESS == ndisStatus)
	{
		DOT11_SSID_LIST	SsidList;
		u1Byte			i = 0;

		N6_ASSIGN_OBJECT_HEADER(
			SsidList.Header,
			NDIS_OBJECT_TYPE_DEFAULT, 
			DOT11_SSID_LIST_REVISION_1, 
			sizeof(DOT11_SSID_LIST)
		);

		// Clear all entries 
		for( ulIndex=0; ulIndex<NUM_PMKID_CACHE; ulIndex++ )
		{
			pSecInfo->PMKIDList[ulIndex].bUsed = FALSE;
		}
		pSecInfo->PMKIDCount = 0;
		
		FtResetEntryList(pAdapter);		

		// TODO: Choose corresponding adapter before assigning parameters
		for(bssidcount=0;bssidcount<NumPreferredBSSID;bssidcount++)
		{
			connectBssEntry = &Params->PreferredBSSEntryList.pElements[bssidcount];
			if( connectBssEntry->Optional.PMKID_IsPresent == TRUE )
			{															
				// 2. Insert or cover with new PMKID.
				blInserted = FALSE;
				for(j=0 ; j<NUM_PMKID_CACHE; j++)
				{
					if( pSecInfo->PMKIDList[j].bUsed && eqMacAddr(pSecInfo->PMKIDList[j].Bssid, connectBssEntry->BSSID.Address) )
					{ // BSSID is matched, the same AP => rewrite with new PMKID.
						if( connectBssEntry->PMKID.ElementCount )
						{
							CopyMem(pSecInfo->PMKIDList[j].PMKID, connectBssEntry->PMKID.pElements, connectBssEntry->PMKID.ElementCount);
							blInserted = TRUE;
							break;
						}
					}
				}

				if(!blInserted)
				{
					// Find a new entry
					for( j=0 ; j<NUM_PMKID_CACHE; j++ )
					{
						if( (pSecInfo->PMKIDList[j].bUsed == FALSE) && connectBssEntry->PMKID.ElementCount )
						{
							pSecInfo->PMKIDList[j].bUsed = TRUE;
							CopyMem(pSecInfo->PMKIDList[j].Bssid, connectBssEntry->BSSID.Address, 6);
							CopyMem(pSecInfo->PMKIDList[j].PMKID, connectBssEntry->PMKID.pElements, PMKID_LEN);
							CopyMem(pSecInfo->PMKIDList[j].SsidBuf, pMgntInfo->SsidBuf, pMgntInfo->Ssid.Length);
							pSecInfo->PMKIDList[j].Ssid.Length= pMgntInfo->Ssid.Length;
							pSecInfo->PMKIDCount ++;
							break;
						}
					}
				}				
			}
			
			if(connectBssEntry->Optional.FTInitialAssocParameters_IsPresent)
			{
				FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_MDE, connectBssEntry->BSSID.Address,
					connectBssEntry->FTInitialAssocParameters.MDE.pElements, connectBssEntry->FTInitialAssocParameters.MDE.ElementCount);
				RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FTInitialAssocParameters, MDE:\n",
					connectBssEntry->FTInitialAssocParameters.MDE.pElements, connectBssEntry->FTInitialAssocParameters.MDE.ElementCount);
			}

			if(connectBssEntry->Optional.FTReAssocParameters_IsPresent)
			{
				FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_MDE, connectBssEntry->BSSID.Address,
					connectBssEntry->FTReAssocParameters.MDE.pElements, connectBssEntry->FTReAssocParameters.MDE.ElementCount);
				RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FTReAssocParameters MDE:\n",
					connectBssEntry->FTReAssocParameters.MDE.pElements, connectBssEntry->FTReAssocParameters.MDE.ElementCount);
				
				FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_FTE, connectBssEntry->BSSID.Address,
					connectBssEntry->FTReAssocParameters.FTE.pElements, connectBssEntry->FTReAssocParameters.FTE.ElementCount);
				RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FTReAssocParameters FTE:\n",
					connectBssEntry->FTReAssocParameters.FTE.pElements, connectBssEntry->FTReAssocParameters.FTE.ElementCount);
				
				FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_PMKR0_NAME, connectBssEntry->BSSID.Address,
					connectBssEntry->FTReAssocParameters.PMKR0Name.pmkname.Name, sizeof(WDI_TYPE_PMK_NAME));
				RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FTReAssocParameters PMKR0:\n",
					connectBssEntry->FTReAssocParameters.PMKR0Name.pmkname.Name, sizeof(WDI_TYPE_PMK_NAME));
			}
		}

		for( ulIndex=0; ulIndex<NUM_PMKID_CACHE ; ulIndex++ )
		{
			if(pSecInfo->PMKIDList[ulIndex].bUsed)
			{
				// For debug purpose: Check current PMKID list.
				RT_TRACE( COMP_MLME, DBG_LOUD, ("------------------------------------------\n") );
				RT_TRACE( COMP_MLME, DBG_LOUD, ("[PMKID %d]\n", ulIndex ) );
				RT_TRACE( COMP_MLME, DBG_LOUD, ("------------------------------------------\n") );
				RT_TRACE( COMP_MLME, DBG_LOUD, ("SecSetPMKID(): PMKIDList[%d].bUsed is TRUE\n", ulIndex) );
				RT_PRINT_STR( COMP_MLME, DBG_LOUD, "SSID: ", pSecInfo->PMKIDList[ulIndex].Ssid.Octet, pSecInfo->PMKIDList[ulIndex].Ssid.Length);
				RT_PRINT_DATA( COMP_MLME, DBG_LOUD, "BSSID: ", pSecInfo->PMKIDList[ulIndex].Bssid, 6);
				RT_PRINT_DATA( COMP_MLME, DBG_LOUD, "PMKID: ", pSecInfo->PMKIDList[ulIndex].PMKID, connectBssEntry->PMKID.ElementCount);
			}
		}

		pAdapter->MgntInfo.bExcludeUnencrypted = Params->ConnectParameters.ConnectionSettings.ExcludeUnencrypted;

		// TODO: HiddenNetwork / MFPEnabled / HostFIPSModeEnabled
		pMgntInfo->bRoamRequest = Params->ConnectParameters.ConnectionSettings.RoamRequest;
		RT_TRACE( COMP_MLME, DBG_LOUD, ("bRoamRequest %d\n", pMgntInfo->bRoamRequest) );
		
		pMgntInfo->SafeModeEnabled = Params->ConnectParameters.ConnectionSettings.HostFIPSModeEnabled;
		
		if(pMgntInfo->SafeModeEnabled && !pMgntInfo->pHTInfo->bEnableHT)
			pMgntInfo->pStaQos->QosCapability = QOS_DISABLE;								
		else
			pMgntInfo->pStaQos->QosCapability = pMgntInfo->pStaQos->QosCapabilityBackup;	
		
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Wdi_task_connect(): bExcludeUnencrypted %d HostFIPSModeEnabled %d QosCapability 0x%x QosCapabilityBackup 0x%x enableHT %d\n", pMgntInfo->bExcludeUnencrypted, pMgntInfo->SafeModeEnabled, pMgntInfo->pStaQos->QosCapability, pMgntInfo->pStaQos->QosCapabilityBackup, pMgntInfo->pHTInfo->bEnableHT));

		//3 DesiredSSIDList
		SsidList.uNumOfEntries = Params->ConnectParameters.SSIDList.ElementCount;

		for( i=0; i< SsidList.uNumOfEntries; i++)
		{
			SsidList.SSIDs[i].uSSIDLength = Params->ConnectParameters.SSIDList.pElements[i].ElementCount;
			CopyMem(SsidList.SSIDs[i].ucSSID, Params->ConnectParameters.SSIDList.pElements[i].pElements, SsidList.SSIDs[i].uSSIDLength);

			RT_PRINT_STR(COMP_MLME, DBG_LOUD, ("Desired SSID \n"), SsidList.SSIDs[i].ucSSID, SsidList.SSIDs[i].uSSIDLength);
		}
		pNdisCommon->dot11DesiredSSIDList = SsidList;
		
		//3 Authentication Algorithm
		N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(pAdapter, Params->ConnectParameters.AuthenticationAlgorithms.pElements[0]);		
		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("AuthenticationAlgorithms count = %d\n", Params->ConnectParameters.AuthenticationAlgorithms.ElementCount));

		//3 Unicast Cipher Algorithm
		N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM(pAdapter, Params->ConnectParameters.UnicastCipherAlgorithms.pElements[0]);
		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("UnicastCipherAlgorithms count = %d\n", Params->ConnectParameters.UnicastCipherAlgorithms.ElementCount));

		//3 Multicast Cipher Algorithm
		N6CSet_DOT11_MULTICAST_CIPHER_ALGORITHM(pAdapter, Params->ConnectParameters.MulticastCipherAlgorithms.pElements[0]);
		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("MulticastCipherAlgorithms count = %d\n", Params->ConnectParameters.MulticastCipherAlgorithms.ElementCount));
		
		
		//3 Additional Association Request IEs
		if(Params->ConnectParameters.Optional.AssociationRequestVendorIE_IsPresent)
		{
			if(Params->ConnectParameters.AssociationRequestVendorIE.ElementCount > 0)
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("Wdi_Task_Connect(): set association request additional IE\n"));
				MgntActSet_AdditionalAssocReqIE(pAdapter, 
					Params->ConnectParameters.AssociationRequestVendorIE.pElements, 
					Params->ConnectParameters.AssociationRequestVendorIE.ElementCount
					);
				
				RT_PRINT_DATA(COMP_MLME, DBG_TRACE, ("Wdi_Task_Connect(): AssociationRequestVendorIE\n"), 
									Params->ConnectParameters.AssociationRequestVendorIE.pElements,
									Params->ConnectParameters.AssociationRequestVendorIE.ElementCount);
			}
		}

		//3 ActivePhyTypeList
		if(Params->ConnectParameters.Optional.ActivePhyTypeList_IsPresent)
		{
			// TODO: connection can be made only at specified PhyType
		}

		//3 DisallowedBSSIDs_IsPresent
		if(Params->ConnectParameters.Optional.DisallowedBSSIDs_IsPresent)
		{
			if(Params->ConnectParameters.DisallowedBSSIDs.ElementCount > 0)
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("Wdi_Task_Connect(): set disallowed BSSIDs\n"));
				MgntActSet_ExcludedMacAddressList(
					pAdapter, 
					(pu1Byte)Params->ConnectParameters.DisallowedBSSIDs.pElements, 
					Params->ConnectParameters.DisallowedBSSIDs.ElementCount
					);
				
				for(; i < Params->ConnectParameters.DisallowedBSSIDs.ElementCount; i++)
				{
					RT_PRINT_ADDR(COMP_MLME, DBG_TRACE, ("Wdi_Task_Connect(): disallowed BSSID:"),
									Params->ConnectParameters.DisallowedBSSIDs.pElements[i].Address);
				}
			}
		}

		//3 AllowedBSSID
		if(Params->ConnectParameters.Optional.AllowedBSSIDs_IsPresent)
		{
			if(Params->ConnectParameters.AllowedBSSIDs.ElementCount > 0)
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("Wdi_Task_Connect(): set allowed BSSIDs\n"));
				PlatformMoveMemory(
					pNdisCommon->dot11DesiredBSSIDList,
					Params->ConnectParameters.AllowedBSSIDs.pElements,
					Params->ConnectParameters.AllowedBSSIDs.ElementCount*sizeof(DOT11_MAC_ADDRESS));

				for(; i < Params->ConnectParameters.AllowedBSSIDs.ElementCount; i++)
				{
					RT_PRINT_ADDR(COMP_MLME, DBG_TRACE, "Wdi_Task_Connect(): allowed BSSID:", pNdisCommon->dot11DesiredBSSIDList[i]);
				}
			}
		}
	}
	else	// parse TLV manually
	{
		PTLV_WDI_STRUCT		pTlvData = NULL;
		ULONG					BytesProcessed = 0;
		ULONG					Prefix = sizeof(WDI_MESSAGE_HEADER);
		
		pTlvData = (PTLV_WDI_STRUCT)((pu1Byte)pOidHandle->pInputBuffer + sizeof(WDI_MESSAGE_HEADER));

		// WDI_CONNECT_BSS_ENTRY
		//RT_TRACE(COMP_MLME, DBG_LOUD, ("PreferredBSSEntryList count = %d\n", Params.PreferredBSSEntryList.ElementCount));
		if(WDI_TLV_CONNECT_PARAMETERS == pTlvData->Type)
		{
			DOT11_SSID_LIST	SsidList;
			u1Byte			NumSsid = 0;
			PRT_SECURITY_T	pSecInfo = &(pAdapter->MgntInfo.SecurityInfo);
			
			InputBufferLength = pTlvData->Length;

			N6_ASSIGN_OBJECT_HEADER(
				SsidList.Header,
				NDIS_OBJECT_TYPE_DEFAULT, 
				DOT11_SSID_LIST_REVISION_1, 
				sizeof(DOT11_SSID_LIST)
			);

			SsidList.uNumOfEntries = NumSsid;

			pTlvData = (PTLV_WDI_STRUCT)((pu1Byte)pTlvData->Value);
			
			while(BytesProcessed < InputBufferLength)
			{
				// Parse Type
				switch(pTlvData->Type)
				{
					default:
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("unknown TLV 0x%x\n", pTlvData->Type));
					}
					break;
					
					case WDI_TLV_CONNECTION_SETTINGS:
					{
						u1Byte	RoamRequest = 0;
						u1Byte	HiddenNetwork = 0;
						u1Byte	ExcludeUnencrypted = 0;
						u1Byte	MFPEnabled = 0;
						u1Byte	HostFIPSModeEnabled = 0;

						RoamRequest = *(pu1Byte)(pTlvData->Value);
						HiddenNetwork = *(pu1Byte)(pTlvData->Value + 1);
						ExcludeUnencrypted = *(pu1Byte)(pTlvData->Value + 2);
						MFPEnabled = *(pu1Byte)(pTlvData->Value + 3);
						HostFIPSModeEnabled = *(pu1Byte)(pTlvData->Value + 4);
						
						RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_TLV_CONNECTION_SETTINGS\n"));
						RT_TRACE(COMP_MLME, DBG_LOUD, ("RoamRequest = %d, HiddenNetwork = %d, ExcludeUnencrypted = %d, MFPEnabled = %d, HostFIPSModeEnabled = %d\n",
															RoamRequest, HiddenNetwork, ExcludeUnencrypted, MFPEnabled, HostFIPSModeEnabled));

						// Handle RoamRequest

						// Handle HiddenNetwork

						// Handle ExcludeUnencrypted
						pAdapter->MgntInfo.bExcludeUnencrypted = (ExcludeUnencrypted == 1)?TRUE:FALSE;

						// Handle MFPEnabled

						// Handle HostFIPSModeEnabled
					}
					break;

					case WDI_TLV_SSID:
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_TLV_SSID\n"));
						SsidList.SSIDs[NumSsid].uSSIDLength = pTlvData->Length;
						CopyMem(SsidList.SSIDs[NumSsid].ucSSID, pTlvData->Value, pTlvData->Length);
						NumSsid++;
						SsidList.uNumOfEntries = NumSsid;
					}
					break;

					case WDI_TLV_AUTH_ALGO_LIST:
					{
						u4Byte	AlgorithmId = (*((pu4Byte)(pTlvData->Value)));
						RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_TLV_AUTH_ALGO_LIST\n"));

						N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(pAdapter, AlgorithmId);
					}
					break;

					case WDI_TLV_MULTICAST_CIPHER_ALGO_LIST:
					{
						u4Byte	AlgorithmId = (*((pu4Byte)(pTlvData->Value)));
						
						RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_TLV_MULTICAST_CIPHER_ALGO_LIST\n"));

						switch( AlgorithmId )
						{
							case WDI_CIPHER_ALGO_NONE:
								pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
								RT_TRACE( COMP_SEC, DBG_LOUD, ("Wdi_Task_Connect(): DOT11_CIPHER_ALGO_NONE\n") );
								break;

							case WDI_CIPHER_ALGO_WEP:
								pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_WEP40;
								RT_TRACE( COMP_SEC, DBG_LOUD, ("Wdi_Task_Connect(): DOT11_CIPHER_ALGO_WEP\n") );
								break;
								
							case WDI_CIPHER_ALGO_WEP40:
								pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_WEP40;
								RT_TRACE( COMP_SEC, DBG_LOUD, ("Wdi_Task_Connect(): DOT11_CIPHER_ALGO_WEP40\n") );
								break;

							case WDI_CIPHER_ALGO_TKIP:
								pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_TKIP;
								RT_TRACE( COMP_SEC, DBG_LOUD, ("Wdi_Task_Connect(): DOT11_CIPHER_ALGO_TKIP\n") );
								break;

							case WDI_CIPHER_ALGO_CCMP:
								pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_AESCCMP;
								RT_TRACE( COMP_SEC, DBG_LOUD, ("Wdi_Task_Connect(): DOT11_CIPHER_ALGO_CCMP\n") );
								break;

							case DOT11_CIPHER_ALGO_WAPI_SMS4: //For WAPI IHV service support add  by ylb 20111114
								pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_SMS4;		
								RT_TRACE( COMP_SEC, DBG_LOUD, ("Wdi_Task_Connect(): DOT11_CIPHER_ALGO_WAPI_SMS4\n") );			
								break;
							case WDI_CIPHER_ALGO_WEP104:
								pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_WEP104;
								RT_TRACE( COMP_SEC, DBG_LOUD, ("Wdi_Task_Connect(): DOT11_CIPHER_ALGO_WEP104\n") );
								break;

							default:
								pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
								RT_TRACE( COMP_SEC, DBG_WARNING, ("Wdi_Task_Connect(): Unknown AlgorithmId 0x%X (WARNING!!!)\n", AlgorithmId) );
								break;
						}

						// Save Multicast Alg
						pNdisCommon->RegGroupALg = AlgorithmId;

						SecConstructRSNIE(pAdapter);
					}
					break;

					case WDI_TLV_UNICAST_CIPHER_ALGO_LIST:
					{
						u4Byte	AlgorithmId = (*((pu4Byte)(pTlvData->Value)));
						
						RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_TLV_UNICAST_CIPHER_ALGO_LIST\n"));

						N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM(pAdapter, (DOT11_CIPHER_ALGORITHM)AlgorithmId);
					}
					break;

					case WDI_TLV_EXTRA_ASSOCIATION_REQUEST_IES:
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_TLV_EXTRA_ASSOCIATION_REQUEST_IES\n"));
					}
					break;

					case WDI_TLV_PHY_TYPE_LIST:
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_TLV_PHY_TYPE_LIST\n"));
					}
					break;

					case WDI_TLV_DISALLOWED_BSSIDS_LIST:
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_TLV_DISALLOWED_BSSIDS_LIST\n"));
					}
					break;

					case WDI_TLV_ALLOWED_BSSIDS_LIST:
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_TLV_ALLOWED_BSSIDS_LIST\n"));
					}
					break;
					
				}

				// Parse Length
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Length = %d\n", pTlvData->Length));

				// Parse Content

				BytesProcessed += (4 + pTlvData->Length);
				
				pTlvData = (PTLV_WDI_STRUCT)((pu1Byte)pOidHandle->pInputBuffer + Prefix + 4 + BytesProcessed);
			}

			pNdisCommon->dot11DesiredSSIDList = SsidList;
		}
	}

	pAdapter->pNdis62Common->CurrentOpState = OP_STATE;

	if( pMgntInfo->bRoamRequest == FALSE )
	{
		ndisStatus = N6CSet_DOT11_CONNECT_REQUEST(
							pAdapter,
							NULL, 0, 0, 0);
	}
	else
	{
		pMgntInfo->Ssid.Octet = pNdisCommon->dot11DesiredSSIDList.SSIDs[0].ucSSID;
		pMgntInfo->Ssid.Length = (u2Byte)pNdisCommon->dot11DesiredSSIDList.SSIDs[0].uSSIDLength;
		
		pMgntInfo->bDisconnectRequest = FALSE;
		RT_TRACE(COMP_MLME, DBG_LOUD, ("This is roam request, go to roam flow\n"));
		MgntLinkStatusSetRoamingState(pAdapter, 0, RT_ROAMING_BY_DEAUTH, ROAMINGSTATE_SCANNING);
		DrvIFIndicateRoamingStart(pAdapter);
	
		if( MgntRoamRetry(pAdapter, FALSE) == FALSE )
			ndisStatus = NDIS_STATUS_FAILURE;
		else
			ndisStatus = NDIS_STATUS_SUCCESS;
	}

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== Wdi_Task_Connect()\n"));
	return ndisStatus;
}


NDIS_STATUS
Wdi_Task_Disconnect(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_TASK_DISCONNECT_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramDisconnect;

	NDIS_OID_REQUEST			*req = NULL;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		if(NDIS_STATUS_SUCCESS != (status = N6CSet_DOT11_DISCONNECT_REQUEST(pAdapter, NULL, 0, 0, 0)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N6CSet_DOT11_DISCONNECT_REQUEST returns: 0x%08X\n", status));
			break;
		}	
	}while(FALSE);

	// M4 indication is not required since bWaitComplete in RT_SUPPORT_TASKs is FALSE

	FunctionOut(COMP_OID_SET);

	return status;
}

//
// Description:
//		Create port according to arguments specified by WDI.
//		The completion counterpart of this function shall be
//		called when create port is done.
//
// Return:
//		NDIS_STATUS_SUCCESS if done synchronousely,
//		NDIS_STATUS_PENDING if done asynchronously
//
static
NDIS_STATUS
Wdi_Task_CreatePort(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS 				status = NDIS_STATUS_SUCCESS;
	NDIS_OID_REQUEST 			*req = NULL;

	WDI_TASK_CREATE_PORT_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramCreatePort;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("NdisPortNumber:%d OpModeMask: 0x%08X\n", 
			param->CreatePortParameters.NdisPortNumber,
			param->CreatePortParameters.OpModeMask));

		if(NULL == (req = Wdi_Xlat_AllocCreateMacOid(pOidHandle, param)))
		{
			status = NDIS_STATUS_RESOURCES;
			break;
		}

		// create mac
		if(WDI_OPERATION_MODE_P2P_DEVICE == param->CreatePortParameters.OpModeMask
			|| WDI_OPERATION_MODE_P2P_CLIENT == param->CreatePortParameters.OpModeMask
			|| WDI_OPERATION_MODE_P2P_GO == param->CreatePortParameters.OpModeMask
			|| (WDI_OPERATION_MODE_P2P_CLIENT | WDI_OPERATION_MODE_P2P_GO)== param->CreatePortParameters.OpModeMask
			) // TODO: OpModeMask is a bitmap with combination of possible op modes
		{
			status = N62C_OID_DOT11_CREATE_MAC(pAdapter, req);
			
			if(NDIS_STATUS_SUCCESS != status && NDIS_STATUS_PENDING != status)
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N62C_OID_DOT11_CREATE_MAC returns error: 0x%08X\n", status));
				Wdi_Xlat_FreeOid(req);
			}
		
			break;
		}
		else
		{// cases which we don't need to create mac
			DOT11_MAC_INFO		*macInfo = req->DATA.METHOD_INFORMATION.InformationBuffer;
			
			status = NDIS_STATUS_SUCCESS;

			macInfo->uNdisPortNumber = 0;
			cpMacAddr(macInfo->MacAddr, pAdapter->CurrentAddress);
			req->DATA.METHOD_INFORMATION.BytesWritten = sizeof(*macInfo);

			WDI_CMD_CreatePortComplete(pAdapter, req, status);
			break;
		}

	}while(FALSE);
	
	FunctionOut(COMP_OID_SET);
	return status;
}

static
NDIS_STATUS
Wdi_Task_DeletePort(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS 		status = NDIS_STATUS_SUCCESS;
	NDIS_OID_REQUEST 	*req = NULL;
	PADAPTER			pTargetAdapter = NULL;
	WDI_TASK_DELETE_PORT_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramDeletePort;
	
	FunctionIn(COMP_OID_SET);
	
	do
	{
		if(NULL == (req = Wdi_Xlat_AllocDeleteMacOid(pOidHandle, param)))
		{
			status = NDIS_STATUS_RESOURCES;
			break;
		}
		
		// delete mac
		if(0 != param->DeletePortParameters.PortNumber)
		{
			//Delete group peer.
			WDI_DeleteGroupPeer(pAdapter, param->DeletePortParameters.PortNumber);
			
			status = N62C_OID_DOT11_DELETE_MAC(pAdapter, req);
			
			if(NDIS_STATUS_SUCCESS != status && NDIS_STATUS_PENDING != status)
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N62C_OID_DOT11_DELETE_MAC returns error: 0x%08X\n", status));
				Wdi_Xlat_FreeOid(req);
			}
			
			break;
		}
		else
		{// cases which we don't need to delete mac
			status = NDIS_STATUS_SUCCESS;
			
			//Delete group peer.
			WDI_DeleteGroupPeer(pAdapter, param->DeletePortParameters.PortNumber);
			
			WDI_CMD_DeletePortComplete(pAdapter, req, status);
			break;
		}
	
	}while(FALSE);
	
	FunctionOut(COMP_OID_SET);
	return status;
}

static
NDIS_STATUS
Wdi_Task_SetApCipherKeyMappingKey(
	IN  ADAPTER					*pAdapter,
	IN  u1Byte					*addr,
	IN  ULONG					cipherAlgo,
	IN  u4Byte					keyLen,
	IN  u1Byte					*pKeyMaterial
	)
{
	MGNT_INFO					*pMgntInfo = &pAdapter->MgntInfo;
	RT_WLAN_STA					*pEntry = AsocEntry_GetEntry(pMgntInfo, addr);

	u4Byte						ucIndex = 0;

	if(!pEntry)
		return	NDIS_STATUS_INVALID_DATA;
		
	CopyMem(pEntry->perSTAKeyInfo.PTK, pKeyMaterial, keyLen); // Added by Annie, 2005-07-12.
	
	if(WDI_CIPHER_ALGO_TKIP == cipherAlgo)
	{
		pEntry->perSTAKeyInfo.TempEncKey = pEntry->perSTAKeyInfo.PTK+TKIP_ENC_KEY_POS;
		pEntry->perSTAKeyInfo.TxMICKey = pEntry->perSTAKeyInfo.PTK+(TKIP_MIC_KEY_POS);	
		pEntry->perSTAKeyInfo.RxMICKey = pEntry->perSTAKeyInfo.PTK+(TKIP_MIC_KEY_POS+TKIP_MIC_KEY_LEN);

		//Add for AP mode HW enc,by CCW 	
		ucIndex = AP_FindFreeEntry(pAdapter , pEntry->MacAddr);
		if(ucIndex == TOTAL_CAM_ENTRY)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("[Warning]: Cam Entry is FULL!!!\n"));
			return NDIS_STATUS_INVALID_DATA;
		}

		//set key
		AP_Setkey(pAdapter , 
			  pEntry->perSTAKeyInfo.pWLanSTA->MacAddr,
			  ucIndex,	// Entey  index 
			  CAM_TKIP,
			  0,  		// Parise key 
			  pEntry->perSTAKeyInfo.TempEncKey);	

		pEntry->keyindex  = ucIndex;
	}
	else if(WDI_CIPHER_ALGO_CCMP == cipherAlgo)
	{  // AES mode AP-WPA AES,CCW
		AESCCMP_BLOCK		blockKey;
		AUTH_PKEY_MGNT_TAG *pKeyMgnt = &pEntry->perSTAKeyInfo;
		
		RT_TRACE(COMP_SEC, DBG_LOUD, ("===>CCMP Set Station Key.\n"));
		RT_PRINT_ADDR(COMP_SEC, DBG_LOUD, "===> Peer Address :  ", addr);
		
		//Add for AP mode HW enc,by CCW 	
		ucIndex = AP_FindFreeEntry(pAdapter, pEntry->MacAddr);
		if(ucIndex == TOTAL_CAM_ENTRY)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("[Warning]: Cam Entry is FULL!!!\n"));
			return NDIS_STATUS_INVALID_DATA;
		}
		
		//Set Key 
		PlatformMoveMemory(blockKey.x , pEntry->perSTAKeyInfo.PTK, 16);
		AES_SetKey(blockKey.x, AESCCMP_BLK_SIZE * 8, (u4Byte *)pEntry->perSTAKeyInfo.AESKeyBuf);
		//set hw key
		AP_Setkey(pAdapter , 
			pEntry->perSTAKeyInfo.pWLanSTA->MacAddr,
			ucIndex,	// Entey  index 
			CAM_AES,
			0,  		// Parise key 
			pEntry->perSTAKeyInfo.PTK);	
		pEntry->keyindex = ucIndex;
	}

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
Wdi_Task_SetApCipherDefaultKey(
	IN  ADAPTER					*pAdapter,
	IN  ULONG					cipherAlgo,
	IN  UINT32					keyId,
	IN  u4Byte					keyLen,
	IN  u1Byte					*pKeyMaterial
	)
{
	MGNT_INFO					*pMgntInfo = &pAdapter->MgntInfo;
	PAUTH_GLOBAL_KEY_TAG		pGlInfo = &pMgntInfo->globalKeyInfo;
	u1Byte						CAM_CONST_BROAD[6]	= {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	// Refer to N6CSet_DOT11_CIPHER_DEFAULT_KEY

	PlatformZeroMemory(pMgntInfo->globalKeyInfo.GTK, GTK_LEN );
	PlatformMoveMemory(pMgntInfo->globalKeyInfo.GTK, pKeyMaterial, keyLen);

	if(WDI_CIPHER_ALGO_TKIP == cipherAlgo)
	{
		pGlInfo->TxMICKey = pGlInfo->GTK + GTK_MIC_TX_POS;
		pGlInfo->RxMICKey = pGlInfo->GTK + GTK_MIC_RX_POS;
		pMgntInfo->SecurityInfo.GroupTransmitKeyIdx = (u1Byte)keyId;
	}
	else if(WDI_CIPHER_ALGO_CCMP == cipherAlgo)
	{
		AESCCMP_BLOCK		blockKey;
		
		PlatformMoveMemory(blockKey.x, pGlInfo->GTK, 16);
		pMgntInfo->SecurityInfo.GroupTransmitKeyIdx = (u1Byte)keyId;
		AES_SetKey(blockKey.x, AESCCMP_BLK_SIZE * 8, (u4Byte *)pGlInfo->AESGTK);
	}

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
Wdi_Task_Set_Radio_State(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	ULONG				InputBufferLength = pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
	WDI_SET_RADIO_STATE_PARAMETERS	*Params = pOidHandle->tlvParser.parsedTlv.paramSetRadioState;
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> Wdi_Task_Set_Radio_State()\n"));

	if( Params->SoftwareRadioState == TRUE )
	{
		pNdisCommon->eRfPowerStateToSet = eRfOn;
		//the task is used for Adapter, so pAdapter is default adapter.
		pAdapter->MgntInfo.RfRequestFromUplayer = FALSE;
	}
	else
	{
		pNdisCommon->eRfPowerStateToSet = eRfOff;
		//the task is used for Adapter, so pAdapter is default adapter.
		pAdapter->MgntInfo.RfRequestFromUplayer = TRUE;
	}
	SetRFPowerStateWorkItemCallback((PVOID)pAdapter);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== Wdi_Task_Set_Radio_State(%x)\n", ndisStatus));
	return ndisStatus;
}

NDIS_STATUS
Wdi_Task_Roam(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS 						ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_NDIS6_COMMON					pNdisCommon = pAdapter->pNdisCommon;
	ULONG								InputBufferLength = pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
	PRT_SECURITY_T						pSecInfo = &(pAdapter->MgntInfo.SecurityInfo);
	WDI_TASK_ROAM_PARAMETERS			*Params = pOidHandle->tlvParser.parsedTlv.paramRoam;
	PWDI_CONNECT_BSS_ENTRY_CONTAINER 	connectBssEntry = NULL;
	PMGNT_INFO							pMgntInfo=&pAdapter->MgntInfo;
	WDI_ASSOC_STATUS					AssocStatus = 0;
	PWDI_MESSAGE_HEADER					pWdiHeader = (PWDI_MESSAGE_HEADER)pOidHandle->pInputBuffer;
	u4Byte								ulIndex, i, j;
	BOOLEAN								blInserted;
	u1Byte 								bssidcount=0;
	int 								NumPreferredBSSID = Params->PreferredBSSEntryList.ElementCount;	
	

	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> Wdi_Task_Roam()\n"));

	pMgntInfo->RegFakeRoamSignal[0] = 0;

	if(NDIS_STATUS_SUCCESS == ndisStatus)
	{
		DOT11_SSID_LIST	SsidList;
		u1Byte			i = 0;

		N6_ASSIGN_OBJECT_HEADER(
			SsidList.Header,
			NDIS_OBJECT_TYPE_DEFAULT, 
			DOT11_SSID_LIST_REVISION_1, 
			sizeof(DOT11_SSID_LIST)
		);

		// TODO: Choose corresponding adapter before assigning parameters
		
		// Clear all entries 
		for( ulIndex=0; ulIndex<NUM_PMKID_CACHE; ulIndex++ )
		{
			pSecInfo->PMKIDList[ulIndex].bUsed = FALSE;
		}
		pSecInfo->PMKIDCount = 0;
		
		FtResetEntryList(pAdapter);		

		// TODO: Choose corresponding adapter before assigning parameters
		for(bssidcount=0;bssidcount<NumPreferredBSSID;bssidcount++)
		{
			connectBssEntry = &Params->PreferredBSSEntryList.pElements[bssidcount];
			if( connectBssEntry->Optional.PMKID_IsPresent == TRUE )
			{															
				// 2. Insert or cover with new PMKID.
				blInserted = FALSE;
				for(j=0 ; j<NUM_PMKID_CACHE; j++)
				{
					if( pSecInfo->PMKIDList[j].bUsed && eqMacAddr(pSecInfo->PMKIDList[j].Bssid, connectBssEntry->BSSID.Address) )
					{ // BSSID is matched, the same AP => rewrite with new PMKID.
						if( connectBssEntry->PMKID.ElementCount )
						{
							CopyMem(pSecInfo->PMKIDList[j].PMKID, connectBssEntry->PMKID.pElements, connectBssEntry->PMKID.ElementCount);
							blInserted = TRUE;
							break;
						}
					}
				}

				if(!blInserted)
				{
					// Find a new entry
					for( j=0 ; j<NUM_PMKID_CACHE; j++ )
					{
						if( (pSecInfo->PMKIDList[j].bUsed == FALSE) && connectBssEntry->PMKID.ElementCount )
						{
							pSecInfo->PMKIDList[j].bUsed = TRUE;
							CopyMem(pSecInfo->PMKIDList[j].Bssid, connectBssEntry->BSSID.Address, 6);
							CopyMem(pSecInfo->PMKIDList[j].PMKID, connectBssEntry->PMKID.pElements, PMKID_LEN);
							CopyMem(pSecInfo->PMKIDList[j].SsidBuf, pMgntInfo->SsidBuf, pMgntInfo->Ssid.Length);
							pSecInfo->PMKIDList[j].Ssid.Length= pMgntInfo->Ssid.Length;
							pSecInfo->PMKIDCount ++;
							break;
						}
					}
				}				
			}
			
			if(connectBssEntry->Optional.FTInitialAssocParameters_IsPresent)
			{
				FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_MDE, connectBssEntry->BSSID.Address,
					connectBssEntry->FTInitialAssocParameters.MDE.pElements, connectBssEntry->FTInitialAssocParameters.MDE.ElementCount);
				RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FTInitialAssocParameters MDE:\n",
					connectBssEntry->FTInitialAssocParameters.MDE.pElements, connectBssEntry->FTInitialAssocParameters.MDE.ElementCount);
			}

			if(connectBssEntry->Optional.FTReAssocParameters_IsPresent)
			{
				FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_MDE, connectBssEntry->BSSID.Address,
					connectBssEntry->FTReAssocParameters.MDE.pElements, connectBssEntry->FTReAssocParameters.MDE.ElementCount);
				RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FTReAssocParameters MDE:\n",
					connectBssEntry->FTReAssocParameters.MDE.pElements, connectBssEntry->FTReAssocParameters.MDE.ElementCount);
				
				FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_FTE, connectBssEntry->BSSID.Address,
					connectBssEntry->FTReAssocParameters.FTE.pElements, connectBssEntry->FTReAssocParameters.FTE.ElementCount);
				RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FTReAssocParameters FTE:\n",
					connectBssEntry->FTReAssocParameters.FTE.pElements, connectBssEntry->FTReAssocParameters.FTE.ElementCount);
				
				FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_PMKR0_NAME, connectBssEntry->BSSID.Address,
					connectBssEntry->FTReAssocParameters.PMKR0Name.pmkname.Name, sizeof(WDI_TYPE_PMK_NAME));
				RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FTReAssocParameters PMKR0:\n",
					connectBssEntry->FTReAssocParameters.PMKR0Name.pmkname.Name, sizeof(WDI_TYPE_PMK_NAME));
			}
		}

		for( ulIndex=0; ulIndex<NUM_PMKID_CACHE ; ulIndex++ )
		{
			if(pSecInfo->PMKIDList[ulIndex].bUsed)
			{
				// For debug purpose: Check current PMKID list.
				RT_TRACE( COMP_MLME, DBG_LOUD, ("------------------------------------------\n") );
				RT_TRACE( COMP_MLME, DBG_LOUD, ("[PMKID %d]\n", ulIndex ) );
				RT_TRACE( COMP_MLME, DBG_LOUD, ("------------------------------------------\n") );
				RT_TRACE( COMP_MLME, DBG_LOUD, ("SecSetPMKID(): PMKIDList[%d].bUsed is TRUE\n", ulIndex) );
				RT_PRINT_STR( COMP_MLME, DBG_LOUD, "SSID: ", pSecInfo->PMKIDList[ulIndex].Ssid.Octet, pSecInfo->PMKIDList[ulIndex].Ssid.Length);
				RT_PRINT_DATA( COMP_MLME, DBG_LOUD, "BSSID: ", pSecInfo->PMKIDList[ulIndex].Bssid, 6);
				RT_PRINT_DATA( COMP_MLME, DBG_LOUD, "PMKID: ", pSecInfo->PMKIDList[ulIndex].PMKID, connectBssEntry->PMKID.ElementCount);
			}
		}
		
		pAdapter->MgntInfo.bExcludeUnencrypted = Params->ConnectParameters.ConnectionSettings.ExcludeUnencrypted;

		// TODO: HiddenNetwork / MFPEnabled / HostFIPSModeEnabled
		pMgntInfo->SafeModeEnabled = Params->ConnectParameters.ConnectionSettings.HostFIPSModeEnabled;
		
		if(pMgntInfo->SafeModeEnabled && !pMgntInfo->pHTInfo->bEnableHT)
			pMgntInfo->pStaQos->QosCapability = QOS_DISABLE;								
		else
			pMgntInfo->pStaQos->QosCapability = pMgntInfo->pStaQos->QosCapabilityBackup;	
		
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Wdi_task_roam(): bExcludeUnencrypted %d HostFIPSModeEnabled %d QosCapability 0x%x\n", pMgntInfo->bExcludeUnencrypted, pMgntInfo->SafeModeEnabled, pMgntInfo->pStaQos->QosCapability));
		
		//3 RoamNeededReason
		if( Params->ConnectParameters.ConnectionSettings.RoamRequest == TRUE )
		{
			AssocStatus = Params->ConnectParameters.ConnectionSettings.RoamNeededReason;
			RT_TRACE_F(COMP_MLME, DBG_LOUD, ("RoamNeededReason %d\n", AssocStatus));
		}
		
		//3 DesiredSSIDList
		SsidList.uNumOfEntries = Params->ConnectParameters.SSIDList.ElementCount;

		for( i=0; i< SsidList.uNumOfEntries; i++)
		{
			SsidList.SSIDs[i].uSSIDLength = Params->ConnectParameters.SSIDList.pElements[i].ElementCount;
			CopyMem(SsidList.SSIDs[i].ucSSID, Params->ConnectParameters.SSIDList.pElements[i].pElements, SsidList.SSIDs[i].uSSIDLength);

			RT_PRINT_STR(COMP_MLME, DBG_LOUD, ("Desired SSID \n"), SsidList.SSIDs[i].ucSSID, SsidList.SSIDs[i].uSSIDLength);
		}
		pNdisCommon->dot11DesiredSSIDList = SsidList;

		//3 Authentication Algorithm
		N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(pAdapter, Params->ConnectParameters.AuthenticationAlgorithms.pElements[0]);

		//3 Unicast Cipher Algorithm
		N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM(pAdapter, Params->ConnectParameters.UnicastCipherAlgorithms.pElements[0]);

		//3 Multicast Cipher Algorithm
		N6CSet_DOT11_MULTICAST_CIPHER_ALGORITHM(pAdapter, Params->ConnectParameters.MulticastCipherAlgorithms.pElements[0]);

		
		//3 Additional Association Request IEs
		if(Params->ConnectParameters.Optional.AssociationRequestVendorIE_IsPresent)
		{
			if(Params->ConnectParameters.AssociationRequestVendorIE.ElementCount > 0)
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("Wdi_Task_Roam(): set association request additional IE\n"));
				MgntActSet_AdditionalAssocReqIE(pAdapter, 
					Params->ConnectParameters.AssociationRequestVendorIE.pElements, 
					Params->ConnectParameters.AssociationRequestVendorIE.ElementCount
					);
				
				RT_PRINT_DATA(COMP_MLME, DBG_TRACE, ("Wdi_Task_Roam(): AssociationRequestVendorIE\n"), 
									Params->ConnectParameters.AssociationRequestVendorIE.pElements,
									Params->ConnectParameters.AssociationRequestVendorIE.ElementCount);
			}
		}

		//3 ActivePhyTypeList
		if(Params->ConnectParameters.Optional.ActivePhyTypeList_IsPresent)
		{
			// TODO: connection can be made only at specified PhyType
		}

		//3 DisallowedBSSIDs_IsPresent
		if(Params->ConnectParameters.Optional.DisallowedBSSIDs_IsPresent)
		{
			if(Params->ConnectParameters.DisallowedBSSIDs.ElementCount > 0)
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("Wdi_Task_Roam(): set disallowed BSSIDs\n"));
				MgntActSet_ExcludedMacAddressList(
					pAdapter, 
					(pu1Byte)Params->ConnectParameters.DisallowedBSSIDs.pElements, 
					Params->ConnectParameters.DisallowedBSSIDs.ElementCount
					);
				
				for(; i < Params->ConnectParameters.DisallowedBSSIDs.ElementCount; i++)
				{
					RT_PRINT_ADDR(COMP_MLME, DBG_TRACE, ("Wdi_Task_Roam(): disallowed BSSID:"),
									Params->ConnectParameters.DisallowedBSSIDs.pElements[i].Address);
				}
			}
		}

		//3 AllowedBSSID
		if(Params->ConnectParameters.Optional.AllowedBSSIDs_IsPresent)
		{
			if(Params->ConnectParameters.AllowedBSSIDs.ElementCount > 0)
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("Wdi_Task_Roam(): set allowed BSSIDs\n"));
				PlatformMoveMemory(
					pNdisCommon->dot11DesiredBSSIDList,
					Params->ConnectParameters.AllowedBSSIDs.pElements,
					Params->ConnectParameters.AllowedBSSIDs.ElementCount*sizeof(DOT11_MAC_ADDRESS));

				for(; i < Params->ConnectParameters.AllowedBSSIDs.ElementCount; i++)
				{
					RT_PRINT_ADDR(COMP_MLME, DBG_TRACE, "Wdi_Task_Roam(): allowed BSSID:", pNdisCommon->dot11DesiredBSSIDList[i]);
				}
			}
		}
	}
	
	if( pMgntInfo->bMediaConnect || pMgntInfo->bIbssStarter)
	{
		if( pMgntInfo->mIbss ){
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Wdi_Task_Roam() => MgntDisconnectIBSS\n") );
			MgntDisconnectIBSS( pAdapter );
		}

		if( pMgntInfo->mAssoc)
		{
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Wdi_Task_Roam() => MgntDisconnectAP\n") );
			MgntDisconnectAP(pAdapter, disas_lv_ss);
		}
	}
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("pMgntInfo->PrepareRoamState %d\n", pMgntInfo->PrepareRoamState));
	switch(pMgntInfo->PrepareRoamState)
	{
		case RT_PREPARE_ROAM_NORMAL_ROAM_POOR_LINK:
			MgntLinkStatusSetRoamingState(pAdapter, 0, RT_ROAMING_BY_DISCONNECT_POOR_LINK, ROAMINGSTATE_SCANNING);
			break;
		case RT_PREPARE_ROAM_DEAUTH_DISASSOC:
			MgntLinkStatusSetRoamingState(pAdapter, 0, RT_ROAMING_BY_DEAUTH, ROAMINGSTATE_SCANNING);
			break;
		case RT_PREPARE_ROAM_UNSPECIFIED:
		case RT_PREPARE_ROAM_NORMAL_ROAM_LOAD_BALANCING:
		case RT_PREPARE_ROAM_INSUFFICIENT_CAPACITY:
		case RT_PREPARE_ROAM_DIRECT_ROAM:
		case RT_PREPARE_ROAM_FIRST_ASSOCIATION:
		case RT_PREPARE_ROAM_ROAM_FROM_WAN:
		case RT_PREPARE_ROAM_ROAM_TO_WAN:
		case RT_PREPARE_ROAM_NORMAL_ROAM_BETTER_AP:
			MgntLinkStatusSetRoamingState(pAdapter, 0, RT_ROAMING_NORMAL, ROAMINGSTATE_SCANNING);
			break;
		default:
			RT_TRACE(COMP_MLME, DBG_WARNING, ("No matched roaming state\n"));
			break;
	}
	
	pMgntInfo->bPrepareRoaming = FALSE;
	pMgntInfo->PrepareRoamState = RT_PREPARE_ROAM_NONE;
	pMgntInfo->PrepareRoamingCount = 0;
	
	DrvIFIndicateRoamingStart(pAdapter);
	
	if( MgntRoamRetry(pAdapter, FALSE) == FALSE )
		ndisStatus = NDIS_STATUS_FAILURE;
	else
		ndisStatus = NDIS_STATUS_SUCCESS;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== Wdi_Task_Roam()\n"));
	return ndisStatus;
}


NDIS_STATUS
WdiSimpleSetProperty(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS 		ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG				BytesRead = 0, BytesNeeded = 0;
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;

	PWDI_MESSAGE_HEADER pWdiHeader 
		= (PWDI_MESSAGE_HEADER)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	// We complete the OID. Need to populate some fields
	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);

	switch(pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.Oid)
	{
		case OID_WDI_SET_ADAPTER_CONFIGURATION:
			N6InitializeNative80211MIBs(pAdapter);
			pMgntInfo->NdisVersion = MgntTranslateNdisVersionToRtNdisVersion(pNdisCommon->NdisVersion);
			break;
	}

	return ndisStatus;
}

// 
// Translate from RT_PS_MODE to WDI_POWER_SAVE_LEVEL
// 2005.02.15, by rcnjko.
//
int 
TranslatePsToWdiPs(
	IN	POWER_LEVEL_CONFIG PowerMode
	)
{
	WDI_POWER_SAVE_LEVEL WdiPsMode;

	switch(PowerMode)
	{
		case POWER_SAVING_NO_POWER_SAVING:
			WdiPsMode = WDI_POWER_SAVE_LEVEL_NO_POWER_SAVE; 
			break;	
		case POWER_SAVING_FAST_PSP:
			WdiPsMode = WDI_POWER_SAVE_LEVEL_FAST_PSP; 
			break;	
		case POWER_SAVING_MAX_PSP:
			WdiPsMode = WDI_POWER_SAVE_LEVEL_MAX_PSP; 
			break;	
		case POWER_SAVING_MAXIMUM_LEVEL:
			WdiPsMode = WDI_POWER_SAVE_LEVEL_MAXIMUM_LEVEL; 
			break;				
		default:
			RT_TRACE(COMP_DBG, DBG_SERIOUS, ("TranslatePsToWdiPs(): Unknown PowerMode: 0x%X !!!\n", PowerMode));
			WdiPsMode = WDI_POWER_SAVE_LEVEL_NO_POWER_SAVE; 
			break;
	}

	return WdiPsMode;
}


NDIS_STATUS
Wdi_Set_Add_Cipher_Keys(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	PTLV_WDI_STRUCT			pTlvData = NULL;
	ULONG					BytesProcessed = 0;
	ULONG					InputBufferLength = 0;
	ULONG					Prefix = sizeof(WDI_MESSAGE_HEADER);
	PRT_SECURITY_T			pSecInfo = &(pAdapter->MgntInfo.SecurityInfo);
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	WDI_SET_ADD_CIPHER_KEYS_PARAMETERS	*Params = pOidHandle->tlvParser.parsedTlv.paramSetAddCipherKeys;
	WDI_SET_ADD_CIPHER_KEYS_CONTAINER 	CipherKey = {0};

	PWDI_MESSAGE_HEADER	pWdiHeader 	= (PWDI_MESSAGE_HEADER)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	BOOLEAN 	bDefaultKey = TRUE;
	u1Byte	MacAddress[ETHERNET_ADDRESS_LENGTH] = {0x00,0x00,0x00,0x00,0x00,0x00};
	RT_ENC_ALG	rtEncAlgo = RT_ENC_ALG_NO_CIPHER;
	ULONG	cipherAlgo = 0;
	ULONG	direction = WDI_CIPHER_KEY_DIRECTION_BOTH;
	u1Byte	static_key = TRUE;
	ULONG	cipherType = WDI_CIPHER_KEY_TYPE_PAIRWISE_KEY;
	u8Byte	KeyRSC = 0;
	u4Byte	KeyLen = 0;
	pu1Byte pKeyMaterial = NULL;
	u1Byte	pTkipKeyMaterial[TKIP_KEY_LEN];
	BOOLEAN bValidKeyRSC = FALSE;
	u4Byte	KeyIndex = 0;
	u4Byte					i = 0;

	FunctionIn(COMP_OID_SET);

	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);

	InputBufferLength = pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
	pTlvData = (PTLV_WDI_STRUCT)((pu1Byte)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer + Prefix);

	if( Params )
	{
		for(i = 0; i < Params->SetCipherKey.ElementCount; i++)
		{
			CipherKey = Params->SetCipherKey.pElements[i];
			if(CipherKey.Optional.PeerMacAddress_IsPresent)
			{
				cpMacAddr(MacAddress, CipherKey.PeerMacAddress.Address);
				bDefaultKey = FALSE;

				RT_PRINT_ADDR(COMP_SCAN, DBG_LOUD, "WDI_TLV_PEER_MAC_ADDRESS(): ", MacAddress);				
			}

			if(CipherKey.Optional.CipherKeyID_IsPresent)
			{
				KeyIndex = CipherKey.CipherKeyID.CipherKeyID;
				RT_TRACE(COMP_SEC, DBG_LOUD, ("WDI_TLV_CIPHER_KEY_ID 0x%x\n", KeyIndex));							
			}
			
			{
				cipherAlgo = CipherKey.CipherKeyTypeInfo.CipherAlgorithm;
				direction = CipherKey.CipherKeyTypeInfo.Direction;
				static_key = CipherKey.CipherKeyTypeInfo.Static;
				cipherType = CipherKey.CipherKeyTypeInfo.KeyType;		// TODO: [WDI] unknown setting

				RT_TRACE(COMP_SEC, DBG_LOUD, ("WDI_TLV_CIPHER_KEY_TYPE_INFO cipherAlgo 0x%x direction 0x%x static_key 0x%x cipherType 0x%x \n", cipherAlgo, direction, static_key, cipherType));

				switch(cipherAlgo)
				{
					case WDI_CIPHER_ALGO_WEP:
						rtEncAlgo = RT_ENC_ALG_WEP;						
						break;

					case WDI_CIPHER_ALGO_WEP40:
						rtEncAlgo = RT_ENC_ALG_WEP40;
						break;

					case WDI_CIPHER_ALGO_WEP104:
						rtEncAlgo = RT_ENC_ALG_WEP104;
						break;			
						
					case WDI_CIPHER_ALGO_TKIP:					
						rtEncAlgo = RT_ENC_ALG_TKIP;
						break;

					case WDI_CIPHER_ALGO_CCMP:						
						rtEncAlgo = RT_ENC_ALG_AESCCMP;						
						break;

					case WDI_CIPHER_ALGO_BIP:											
						break;

					default:
						break;
				}				
			}			

			if(CipherKey.Optional.ReceiveSequenceCount_IsPresent)			
			{
				PlatformMoveMemory(&KeyRSC, CipherKey.ReceiveSequenceCount.ReceiveSequenceCount, 6);
//				RT_TRACE(COMP_SEC, DBG_LOUD, ("WIFI_TLV_CIPHER_KEY_RECEIVE_SEQUENCE_COUNT 0x%"i64fmt"x\n", KeyRSC));							
			}

			if(CipherKey.Optional.CCMPKey_IsPresent)
			{			
				KeyLen = CipherKey.CCMPKey.ElementCount;
				pKeyMaterial = CipherKey.CCMPKey.pElements;
				bValidKeyRSC = TRUE;				

				RT_PRINT_DATA(COMP_SEC, DBG_LOUD,("WDI_TLV_CIPHER_KEY_CCMP_KEY\n"),pKeyMaterial,KeyLen);
			}

			if(CipherKey.Optional.TKIPInfo_IsPresent)
			{			
				// Tkip Enc Key(16) + STA Rx MIC Key(8) + STA Tx MIC Key(8).
				RT_PRINT_DATA(COMP_SEC, DBG_LOUD, ("TKIPKey \n"), CipherKey.TKIPInfo.TKIPKey.pElements, CipherKey.TKIPInfo.TKIPKey.ElementCount);
				RT_PRINT_DATA(COMP_SEC, DBG_LOUD, ("TKIPMIC \n"), CipherKey.TKIPInfo.TKIPMIC.pElements, CipherKey.TKIPInfo.TKIPMIC.ElementCount);
				
				PlatformMoveMemory(pTkipKeyMaterial, CipherKey.TKIPInfo.TKIPKey.pElements, CipherKey.TKIPInfo.TKIPKey.ElementCount);
				PlatformMoveMemory(pTkipKeyMaterial+CipherKey.TKIPInfo.TKIPKey.ElementCount, CipherKey.TKIPInfo.TKIPMIC.pElements, CipherKey.TKIPInfo.TKIPMIC.ElementCount);				

				RT_PRINT_DATA(COMP_SEC, DBG_LOUD, ("pTkipKeyMaterial \n"), pTkipKeyMaterial, TKIP_KEY_LEN);				
				
				pKeyMaterial = pTkipKeyMaterial; 
				KeyLen = CipherKey.TKIPInfo.TKIPKey.ElementCount + CipherKey.TKIPInfo.TKIPMIC.ElementCount; 
				
				bValidKeyRSC = TRUE;				
				RT_PRINT_DATA(COMP_SEC, DBG_LOUD,("WDI_TLV_CIPHER_KEY_TKIP_KEY_INFO\n"),pKeyMaterial,KeyLen);				
			}

			if(CipherKey.Optional.BIPKey_IsPresent)
			{		
				if( CipherKey.BIPKey.ElementCount == BIP_KEY_LEN)
				{
					PlatformMoveMemory( pSecInfo->BIPKeyBuffer ,CipherKey.BIPKey.pElements, CipherKey.BIPKey.ElementCount);
					RT_TRACE_F(COMP_SEC, DBG_LOUD, ("BIP key length %d\n", CipherKey.BIPKey.ElementCount));
					RT_PRINT_DATA(COMP_SEC, DBG_LOUD,("WDI_TLV_CIPHER_KEY_BIP_KEY\n"),CipherKey.BIPKey.pElements,CipherKey.BIPKey.ElementCount);
					PlatformMoveMemory(pSecInfo->IPN, &KeyRSC, 6);
					RT_PRINT_DATA(COMP_SEC, DBG_LOUD,("Wdi_Set_Add_Cipher_Keys(): BIP IPN:\n"), pSecInfo->IPN, 6);
				}
				else
				{
					ndisStatus = NDIS_STATUS_INVALID_DATA;
				}
				
				return ndisStatus;				
			}			
			
			if(CipherKey.Optional.WEPKey_IsPresent)
			{			
				if(rtEncAlgo == RT_ENC_ALG_WEP)	
				{
					if (CipherKey.WEPKey.ElementCount == NATIVE_802_11_WEP40_KEY_LENGTH)
					{
						rtEncAlgo = RT_ENC_ALG_WEP40;
						RT_TRACE( COMP_SEC, DBG_LOUD, ("Wdi_Set_Add_Cipher_Keys(): DOT11_CIPHER_ALGO_WEP40\n") );
					}
					else if (CipherKey.WEPKey.ElementCount == NATIVE_802_11_WEP104_KEY_LENGTH)
					{
						rtEncAlgo = RT_ENC_ALG_WEP104;
						RT_TRACE( COMP_SEC, DBG_LOUD, ("Wdi_Set_Add_Cipher_Keys(): DOT11_CIPHER_ALGO_WEP104\n") );
					}
				}
								
				KeyLen = CipherKey.WEPKey.ElementCount;
				pKeyMaterial = CipherKey.WEPKey.pElements;

				RT_PRINT_DATA(COMP_SEC, DBG_LOUD,("WDI_TLV_CIPHER_KEY_WEP_KEY\n"),pKeyMaterial,KeyLen);								
			}			
			
			RT_TRACE(COMP_SEC, DBG_LOUD,("==>CipherType %s\n", (WDI_CIPHER_KEY_TYPE_PAIRWISE_KEY == cipherType)?"WDI_CIPHER_KEY_TYPE_PAIRWISE_KEY":( (WDI_CIPHER_KEY_TYPE_GROUP_KEY == cipherType)?"WDI_CIPHER_KEY_TYPE_GROUP_KEY":"WDI_CIPHER_KEY_TYPE_IGTK")));
			if(0 == i && !ACTING_AS_AP(pAdapter))
			{
				if(WDI_CIPHER_KEY_TYPE_PAIRWISE_KEY == cipherType)
				{
					pSecInfo->PairwiseEncAlgorithm = rtEncAlgo;
				}
				else if(WDI_CIPHER_KEY_TYPE_GROUP_KEY == cipherType)
				{				
					pSecInfo->GroupEncAlgorithm = rtEncAlgo;				
				}
				RT_TRACE(COMP_SEC, DBG_LOUD,("==> pSecInfo->PairwiseEncAlgorithm=0x%x\n",pSecInfo->PairwiseEncAlgorithm));
				
			}					
			{
				if( ACTING_AS_AP(pAdapter))
				{
					if(bDefaultKey)
						ndisStatus = Wdi_Task_SetApCipherDefaultKey(pAdapter, cipherAlgo, KeyIndex, KeyLen, pKeyMaterial);
					else
						ndisStatus = Wdi_Task_SetApCipherKeyMappingKey(pAdapter, MacAddress, cipherAlgo, KeyLen, pKeyMaterial);
				}
				else if( !pAdapter->MgntInfo.bRSNAPSKMode )
				{
					if(!bDefaultKey)
					{
						if( !(rtEncAlgo == RT_ENC_ALG_WEP40 || rtEncAlgo == RT_ENC_ALG_WEP104 )  )
						{
							MgntActSet_802_11_ADD_KEY(
								pAdapter,
								rtEncAlgo,
								(PAIRWISE_KEYIDX | ((bValidKeyRSC) ? ADD_KEY_IDX_RSC: 0) ),
								KeyLen,
								pKeyMaterial,
								MacAddress,
								FALSE, // IsGroupTransmitKey.
								FALSE, // IsGroup.
								KeyRSC);
						}
						else
						{							
							for(i=0;i<4;i++){
								// Save for Reconnent for Ad-hoc ,by CCW
//								PlatformMoveMemory( pNdisCommon->RegDefaultKeyBuf[i], pKeyMaterial, KeyLen);
//								pNdisCommon->RegDefaultKey[i].Length  = (u2Byte)KeyLen;
								MgntActSet_802_11_ADD_KEY(
									pAdapter,
									rtEncAlgo,
									i,
									KeyLen,
									pKeyMaterial,
									MacAddress,
									FALSE, // IsGroupTransmitKey.
									FALSE, // IsGroup.
									KeyRSC);

								if( rtEncAlgo == RT_ENC_ALG_WEP40 )
								{
									PlatformMoveMemory( pNdisCommon->RegDefaultKeyBuf[i], pKeyMaterial, KeyLen);
									pNdisCommon->RegDefaultKey[i].Length = (u2Byte)KeyLen;
								}
								else if(rtEncAlgo == RT_ENC_ALG_WEP104) 
								{
									PlatformMoveMemory( pNdisCommon->RegDefaultKeyWBuf[i], pKeyMaterial, KeyLen);
									pNdisCommon->RegDefaultKeyW[i].Length =(u2Byte)KeyLen;
								}

							}							
						}
					}
					else
					{
						pSecInfo->GroupTransmitKeyIdx = (u1Byte)KeyIndex; //by tynli. Suggested by CCW.

						RT_TRACE( COMP_SEC ,DBG_LOUD,( "====> pSecInfo->GroupTransmitKeyIdx  = %d\n ",KeyIndex));
						MgntActSet_802_11_ADD_KEY(
							pAdapter,
							rtEncAlgo,
							( KeyIndex | ((bValidKeyRSC) ? ADD_KEY_IDX_RSC: 0) ),
							KeyLen,
							pKeyMaterial,
							MacAddress, // Only be a valid unicast address in RSNA IBSS case.
							FALSE, // IsGroupTransmitKey, since DefaultTransmitKeyIdx and GroupTransmitKeyIdx is determined by OID_DOT11_CIPHER_DEFAULT_KEY_ID, we just set it to FALSE.
							TRUE, // IsGroup.
							KeyRSC);

							i = (KeyIndex & 0x00000003);
							if( rtEncAlgo == RT_ENC_ALG_WEP40 )
							{
								PlatformMoveMemory( pNdisCommon->RegDefaultKeyBuf[i], pKeyMaterial, KeyLen);
								pNdisCommon->RegDefaultKey[i].Length = (u2Byte)KeyLen;
							}
							else if(rtEncAlgo == RT_ENC_ALG_WEP104) 
							{
								PlatformMoveMemory( pNdisCommon->RegDefaultKeyWBuf[i], pKeyMaterial, KeyLen);
								pNdisCommon->RegDefaultKeyW[i].Length =(u2Byte)KeyLen;
							}
					}
					PlatformRequestPreAuthentication( pAdapter, PRE_AUTH_INDICATION_REASON_ASSOCIATION );
					
				}
				else
				{
					// TODO: [WDI] security for Ad-hoc mode
					
				}
			}				
		}

	}
	else
	{
		RT_TRACE(COMP_SEC, DBG_LOUD, ("ParseWdiAddCipherkey FAIL!! \n"));
	}

	return ndisStatus;
}


NDIS_STATUS
Wdi_Set_Delete_Cipher_Keys(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	PTLV_WDI_STRUCT			pTlvData = NULL;
	ULONG					BytesProcessed = 0;
	ULONG					InputBufferLength = 0;
	ULONG					Prefix = sizeof(WDI_MESSAGE_HEADER);
	PRT_SECURITY_T			pSecInfo = &(pAdapter->MgntInfo.SecurityInfo);
	WDI_SET_DELETE_CIPHER_KEYS_PARAMETERS	*Params = pOidHandle->tlvParser.parsedTlv.paramSetDeleteDefaultCipherKeys;
	WDI_SET_DELETE_CIPHER_KEYS_CONTAINER 	CipherKeyInfo = {0};

	PWDI_MESSAGE_HEADER	pWdiHeader 
		= (PWDI_MESSAGE_HEADER)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	BOOLEAN		bDefaultKey = TRUE;
	u1Byte	MacAddress[ETHERNET_ADDRESS_LENGTH] = {0x00,0x00,0x00,0x00,0x00,0x00};
	RT_ENC_ALG	rtEncAlgo = RT_ENC_ALG_NO_CIPHER;
	u4Byte	KeyIndex = 0;
	ULONG	cipherAlgo = 0;
	ULONG	direction = WDI_CIPHER_KEY_DIRECTION_BOTH;
	u1Byte	static_key = TRUE;
	ULONG	cipherType = WDI_CIPHER_KEY_TYPE_PAIRWISE_KEY;	

	u4Byte					i = 0;

	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);

	InputBufferLength = pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;

	pTlvData = (PTLV_WDI_STRUCT)((pu1Byte)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer + Prefix);

	if( Params )
	{
		for(i = 0; i < Params->CipherKeyInfo.ElementCount; i++)
		{
			CipherKeyInfo = Params->CipherKeyInfo.pElements[i];
			if(CipherKeyInfo.Optional.PeerMacAddress_IsPresent)
			{
				RT_TRACE(COMP_SEC, DBG_LOUD, ("WDI_TLV_PEER_MAC_ADDRESS\n"));			
				cpMacAddr(MacAddress, CipherKeyInfo.PeerMacAddress.Address);
				bDefaultKey = FALSE;
			}

			if(CipherKeyInfo.Optional.CipherKeyID_IsPresent)
			{
				RT_TRACE(COMP_SEC, DBG_LOUD, ("WDI_TLV_CIPHER_KEY_ID\n"));			
				KeyIndex = CipherKeyInfo.CipherKeyID.CipherKeyID;
			}
			
			{
				cipherAlgo = CipherKeyInfo.CipherKeyTypeInfo.CipherAlgorithm;
				direction = CipherKeyInfo.CipherKeyTypeInfo.Direction;
				static_key = CipherKeyInfo.CipherKeyTypeInfo.Static;
				cipherType = CipherKeyInfo.CipherKeyTypeInfo.KeyType;		// TODO: [WDI] unknown setting

				RT_TRACE_F(COMP_SEC, DBG_LOUD, ("cipherAlgo 0x%x\n", cipherAlgo));

				switch(cipherAlgo)
				{
					case WDI_CIPHER_ALGO_WEP:
						rtEncAlgo = RT_ENC_ALG_WEP;						
						break;

					case WDI_CIPHER_ALGO_WEP40:
						rtEncAlgo = RT_ENC_ALG_WEP40;
						break;

					case WDI_CIPHER_ALGO_WEP104:
						rtEncAlgo = RT_ENC_ALG_WEP104;
						break;			
						
					case WDI_CIPHER_ALGO_TKIP:					
						rtEncAlgo = RT_ENC_ALG_TKIP;
						break;

					case WDI_CIPHER_ALGO_CCMP:						
						rtEncAlgo = RT_ENC_ALG_AESCCMP;						
						break;

					case WDI_CIPHER_ALGO_BIP:											
						break;

					default:
						break;
				}				
			}									

			RT_TRACE(COMP_SEC, DBG_LOUD, ("[WDI] [Property] WDI_SET_DELETE_CIPHER_KEYS: delete Key\n"));

			{
				//
				// 061028, rcnjko: WDK said we shall ignore AlgorithmId if bDelete is set, 
				// so we use GroupEncAlgorithm instead. 
				//
				if( KeyIndex > 3 && KeyIndex < 6 )
				{  // 802.11w BIP key Key index 5 or 6
				
				    PlatformZeroMemory(pSecInfo->BIPKeyBuffer, MAX_KEY_LEN);
				}
				else if( !pAdapter->MgntInfo.bRSNAPSKMode)
				{
					MgntActSet_802_11_REMOVE_KEY(
						pAdapter, 
						rtEncAlgo,
						KeyIndex,
						MacAddress,
						TRUE); // IsGroup.
				}
				else
				{
					if(!MgntActSet_RSNA_REMOVE_DEAULT_KEY(
							pAdapter,
							KeyIndex,
							MacAddress))
					{
						RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [WDI] Property WIFI_SET_DELETE_CIPHER_KEYS, MgntActSet_RSNA_REMOVE_DEAULT_KEY return FALSE !!!\n"));
						return NDIS_STATUS_INVALID_DATA;
					}
				}		
			}
		}		

	}
	else if(WDI_TLV_DELETE_CIPHER_KEY_INFO== pTlvData->Type)
	{		
		InputBufferLength = pTlvData->Length;

		pTlvData = (PTLV_WDI_STRUCT)((pu1Byte)pTlvData->Value);
		Prefix += 4;
		
		while(BytesProcessed < InputBufferLength)
		{
			// Parse Type
			switch(pTlvData->Type)
			{
				default:
				{
					RT_TRACE(COMP_SEC, DBG_LOUD, ("unknown TLV 0x%x\n", pTlvData->Type));
				}
				break;
				
				case WDI_TLV_PEER_MAC_ADDRESS:
				{
					RT_TRACE(COMP_SEC, DBG_LOUD, ("WDI_TLV_PEER_MAC_ADDRESS\n"));
					cpMacAddr(MacAddress, pTlvData->Value);
					bDefaultKey = FALSE;
				}
				break;

				case WDI_TLV_CIPHER_KEY_ID:
				{
					RT_TRACE(COMP_SEC, DBG_LOUD, ("WDI_TLV_CIPHER_KEY_ID\n"));
					KeyIndex = (*((pu4Byte)(pTlvData->Value)));
				}
				break;

				case WDI_TLV_CIPHER_KEY_TYPE_INFO:
				{
					RT_TRACE(COMP_SEC, DBG_LOUD, ("WDI_TLV_CIPHER_KEY_TYPE_INFO\n"));
					cipherAlgo = (*((pu4Byte)(pTlvData->Value)));
					direction = (*((pu4Byte)(pTlvData->Value + 4)));
					static_key = (*(pu1Byte)(pTlvData->Value + 8));
					cipherType = (*((pu4Byte)(pTlvData->Value + 9)));		// TODO: [WDI] unknown setting

					if(WDI_CIPHER_ALGO_CCMP == cipherAlgo)
						rtEncAlgo = RT_ENC_ALG_AESCCMP;
					else
						rtEncAlgo = RT_ENC_ALG_NO_CIPHER;
				}
				break;
			}
			// Parse Length
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Length = %d\n", pTlvData->Length));

			// Parse Content

			BytesProcessed += (4 + pTlvData->Length);
			
			pTlvData = (PTLV_WDI_STRUCT)((pu1Byte)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer + Prefix + BytesProcessed);
		}

		RT_TRACE(COMP_SEC, DBG_LOUD, ("[WDI] [Property] WDI_SET_DELETE_CIPHER_KEYS: delete Key\n"));

		//
		// 061028, rcnjko: WDK said we shall ignore AlgorithmId if bDelete is set, 
		// so we use GroupEncAlgorithm instead. 
		//
		rtEncAlgo = pSecInfo->GroupEncAlgorithm;
		if( KeyIndex > 3 && KeyIndex < 6 )
		{  // 802.11w BIP key Key index 5 or 6
		
		    PlatformZeroMemory(pSecInfo->BIPKeyBuffer, MAX_KEY_LEN);
		}
		else if( !pAdapter->MgntInfo.bRSNAPSKMode)
		{
			MgntActSet_802_11_REMOVE_KEY(
				pAdapter, 
				rtEncAlgo,
				KeyIndex,
				MacAddress,
				TRUE); // IsGroup.
		}
		else
		{
			if(!MgntActSet_RSNA_REMOVE_DEAULT_KEY(
					pAdapter,
					KeyIndex,
					MacAddress))
			{
				RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [WDI] Property WDI_SET_DELETE_CIPHER_KEYS, MgntActSet_RSNA_REMOVE_DEAULT_KEY return FALSE !!!\n"));
				return NDIS_STATUS_INVALID_DATA;
			}
		}
	}
	return ndisStatus;
}

NDIS_STATUS
Wdi_Set_Default_Key_ID(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG					InputBufferLength = 0;
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PRT_SECURITY_T			pSecInfo = &(pAdapter->MgntInfo.SecurityInfo);
	WDI_SET_DEFAULT_KEY_ID_PARAMETERS	*Params = pOidHandle->tlvParser.parsedTlv.paramSetDefaultKeyId;
	InputBufferLength = pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
		u2Byte KeyId;

	if( Params )
		{
			// Get default key id to set.
			KeyId = (u2Byte)Params->DefaultKeyIdParameters.KeyID;

			// Set default key id: 0 ~ 3.
			KeyId &= 0x3;
			// The following code will cause 8187 failed to link to AP.	
			pSecInfo->DefaultTransmitKeyIdx = (u1Byte)KeyId;
			pNdisCommon->RegDefaultKeyId = (u1Byte)KeyId;

			RT_TRACE_F(COMP_SEC, DBG_LOUD, ("KeyId 0x%x\n", KeyId));
		}
		return ndisStatus;
	}


NDIS_STATUS
Wdi_Set_Receive_Packet_Filter(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG					BytesRead = 0, BytesNeeded = 0;

	PWDI_MESSAGE_HEADER	pWdiHeader 
		= (PWDI_MESSAGE_HEADER)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	ULONG					InputBufferLength 
		= pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;;
	WDI_SET_RECEIVE_PACKET_FILTER_PARAMETERS	*Params = pOidHandle->tlvParser.parsedTlv.paramSetReceivePacketFilter;

	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);

	// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
	RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("sizeof(WDI_MESSAGE_HEADER) %d, InputBufferLength: %d\n", sizeof(WDI_MESSAGE_HEADER), pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength));
	RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, "Information buffer:\n", pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer, pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength);

	ndisStatus = N6C_SET_OID_GEN_CURRENT_PACKET_FILTER(pAdapter,
				pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.Oid,
				pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer,
				pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength,
				&BytesRead,
				&BytesNeeded);
	
	return ndisStatus;
}

NDIS_STATUS
Wdi_Set_Network_List_Offload(
	IN  PADAPTER			pAdapter,
	IN	PRT_OID_HANDLER		pOidHandle
	)
{
	WDI_NETWORK_LIST_OFFLOAD_PARAMETERS *Params 		= pOidHandle->tlvParser.parsedTlv.paramNetworkListOffload;
	PWDI_NETWORK_LIST_OFFLOAD_INFO		pWdiNLO			= NULL;
	PWDI_SSID_OFFLOAD_CONTAINER			pOffloadItem	= NULL;
	PWDI_CHANNEL_MAPPING_ENTRY			pChannelItem	= NULL;
	PWDI_ALGO_PAIRS						pAlgorithmItem	= NULL;

	NDIS_STATUS			ndisStatus 		= NDIS_STATUS_SUCCESS;
	PADAPTER			pDefaultAdapter	= GetDefaultAdapter(pAdapter);
	PRT_NDIS6_COMMON	pNdisCommon 	= pDefaultAdapter->pNdisCommon;
	PRT_NLO_INFO		pNLOInfo 		= &(pDefaultAdapter->MgntInfo.NLOInfo);	
	u4Byte				ChannelIndex 	= 0;
	u4Byte				Index			= 0;
	u4Byte				ItemCnt			= 0;
	u1Byte				i				= 0;

	RT_TRACE( COMP_POWER, DBG_LOUD , (" ===>Wdi_Set_Network_List_Offload\n") );

	if( Params )
	{
		pWdiNLO = &(Params->NetworkListOffload);

		if( pWdiNLO->Optional.SsidOffload_IsPresent == 0 )
		{
			// Stop NLO mode !!
			pNLOInfo->NumDot11OffloadNetwork = 0;
			pNLOInfo->FastScanIterations = 0;
			pNLOInfo->FastScanPeriod = 0;
			pNLOInfo->SlowScanPeriod = 0;
			pNdisCommon->ScanPeriod = 0;
			
			RT_TRACE( COMP_POWER, DBG_LOUD , (" ===>SsidOffload_IsPresent == 0 \n") );
			return NDIS_STATUS_SUCCESS;
		}
		else
		{
//			RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, "NLO INFO: \n", InformationBuffer, sizeof(DOT11_OFFLOAD_NETWORK_LIST_INFO));
			pNLOInfo->NumDot11OffloadNetwork = pWdiNLO->SsidOffload.ElementCount;
			pNLOInfo->FastScanIterations = pWdiNLO->NetworkListOffloadConfig.FastScanIterations;
			pNLOInfo->FastScanPeriod = pWdiNLO->NetworkListOffloadConfig.FastScanPeriodinSeconds ;
			pNLOInfo->SlowScanPeriod = pWdiNLO->NetworkListOffloadConfig.SlowScanPeriodinSeconds ;


			RT_TRACE(COMP_POWER , DBG_LOUD , (" NumDot11OffloadNetwork : %d\n", pNLOInfo->NumDot11OffloadNetwork) );
			RT_TRACE(COMP_POWER , DBG_LOUD , (" FastScanIterations : %d\n", pNLOInfo->FastScanIterations) );
			RT_TRACE(COMP_POWER , DBG_LOUD , (" FastScanPeriod : %d sec\n", pNLOInfo->FastScanPeriod) );
			RT_TRACE(COMP_POWER , DBG_LOUD , (" SlowScanPeriod : %d sec\n", pNLOInfo->SlowScanPeriod) );

			pNdisCommon->bNLOActiveScan 	= TRUE;
			pNdisCommon->bFilterHiddenAP 	= FALSE;

			// Start the NLO scan when the first PlatformHandleNLOnScanRequest is called
			pNdisCommon->ScanPeriod = pNLOInfo->FastScanPeriod;
		}

		for( ItemCnt=0, Index=0 ; ItemCnt<pWdiNLO->SsidOffload.ElementCount ; ++ItemCnt)
		{
			pOffloadItem = &(pWdiNLO->SsidOffload.pElements[ItemCnt]);
			pChannelItem = pOffloadItem->ChannellHintList.pElements;

			for( i=0 ; i<pOffloadItem->UnicastAlgorithms.ElementCount ; ++i, ++Index)
			{
				// Copy SSID 
				CopySsid(	pNLOInfo->dDot11OffloadNetworkList[Index].ssidbuf, 
							pNLOInfo->dDot11OffloadNetworkList[Index].ssidlen,
					 		pOffloadItem->SsidToScan.pElements,
					 		pOffloadItem->SsidToScan.ElementCount);
		
				RT_TRACE(COMP_POWER, DBG_LOUD, ("The %u OFFLOAD NETWORK SSID: %s\n", Index+1, pNLOInfo->dDot11OffloadNetworkList[Index].ssidbuf));

				pAlgorithmItem = &(pOffloadItem->UnicastAlgorithms.pElements[i]);

				// Set chuiper !!
				switch(  pAlgorithmItem->CipherAlgorithm )
				{
					case WDI_CIPHER_ALGO_NONE :
						pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_NONE;
						pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 		= TRUE;
						pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= 0;
						break;
					case WDI_CIPHER_ALGO_WEP40 :
					case WDI_CIPHER_ALGO_WEP104 :
					case WDI_CIPHER_ALGO_WEP:
						pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_NONE;
						pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 		= TRUE;
						pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= 0x0;
						break;
					case WDI_CIPHER_ALGO_TKIP :  
						if(	pAlgorithmItem->AuthAlgorithm == WDI_AUTH_ALGO_WPA || 
						     	pAlgorithmItem->AuthAlgorithm == WDI_AUTH_ALGO_WPA_PSK )
						{
							pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA;
							pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
							pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA_TKIP;
						}
						else if( pAlgorithmItem->AuthAlgorithm == WDI_AUTH_ALGO_RSNA || 
						     	    pAlgorithmItem->AuthAlgorithm == WDI_AUTH_ALGO_RSNA_PSK )
						{
							pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA2;
							pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
							pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA2_TKIP;
						}
						else
						{
							pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA;
							pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
							pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA_TKIP;
							RT_TRACE( COMP_POWER, DBG_LOUD , (" ===>NLO Not define TKIP\n") );
						}
						break;
					case WDI_CIPHER_ALGO_CCMP :
						if(	pAlgorithmItem->AuthAlgorithm == WDI_AUTH_ALGO_WPA || 
						     	pAlgorithmItem->AuthAlgorithm == WDI_AUTH_ALGO_WPA_PSK )
						{
							pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_WPA;
							pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
							pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= WPA_AES;
						}
						else if( pAlgorithmItem->AuthAlgorithm == WDI_AUTH_ALGO_RSNA || 
						     	    pAlgorithmItem->AuthAlgorithm == WDI_AUTH_ALGO_RSNA_PSK )
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
							RT_TRACE( COMP_POWER, DBG_LOUD , (" ===>NLO Not define AES\n") );
						}
						break;
					default :
						pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl    	= RT_SEC_LVL_NONE;
						pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy 	= TRUE;
						pNLOInfo->dDot11OffloadNetworkList[Index].chiper	 	= 0;
						RT_TRACE( COMP_POWER, DBG_LOUD , (" ===> Unknow UnicastCipher : %x\n", pAlgorithmItem->CipherAlgorithm) );
						break;
				}
				
				RT_TRACE(COMP_POWER , DBG_LOUD , (" Seclvl : %d\n", pNLOInfo->dDot11OffloadNetworkList[Index].SecLvl) );
				RT_TRACE(COMP_POWER , DBG_LOUD , (" bPrivacy : %d\n", pNLOInfo->dDot11OffloadNetworkList[Index].bPrivacy) );
				RT_TRACE(COMP_POWER , DBG_LOUD , (" chiper : %d\n", pNLOInfo->dDot11OffloadNetworkList[Index].chiper) );

				RT_TRACE( COMP_POWER, DBG_LOUD, (" Channel Hint:\n"));
				
				// Save channel info 
				// DOT11_MAX_CHANNEL_HINTS 4
				for(  ChannelIndex = 0 ;  ChannelIndex < pOffloadItem->ChannellHintList.ElementCount ;  ChannelIndex++ )
				{
					pNLOInfo->dDot11OffloadNetworkList[Index].channelNumberHit[ChannelIndex] = pChannelItem[ChannelIndex].ChannelNumber;
					RT_TRACE( COMP_POWER, DBG_LOUD, (" %u", pNLOInfo->dDot11OffloadNetworkList[Index].channelNumberHit[ChannelIndex]));
				}
				
				RT_TRACE( COMP_POWER, DBG_LOUD, ("\n"));
				
				// Show Entry information !! 
				RT_TRACE( COMP_POWER, DBG_LOUD , (" ===> Show NLO info Index (%x):\n", Index) );
				RT_PRINT_STR(COMP_POWER, DBG_LOUD, (" SSID"), pNLOInfo->dDot11OffloadNetworkList[Index].ssidbuf, pNLOInfo->dDot11OffloadNetworkList[Index].ssidlen);
				RT_TRACE( COMP_POWER , DBG_LOUD , (" chiper : %x	\n", pNLOInfo->dDot11OffloadNetworkList[Index].chiper) );
			}
		}		
	}
	else
	{
		RT_TRACE(COMP_POWER, DBG_SERIOUS, ("Wdi_Set_Network_List_Offload(): Tlv Params is NULL\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
	}

	RT_TRACE( COMP_POWER, DBG_LOUD , (" <===Wdi_Set_Network_List_Offload\n") );

	return ndisStatus;
}


NDIS_STATUS
Wdi_Set_Privacy_Exemption_List(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG					BytesRead = 0, BytesNeeded = 0;
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PNDIS_OID_REQUEST  pNdisRequest = pOidHandle->pNdisRequest;
	PWDI_MESSAGE_HEADER		pWdiHeader 
		= (PWDI_MESSAGE_HEADER)pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	ULONG					InputBufferLength 
		= pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;
	WDI_SET_PRIVACY_EXEMPTION_LIST_PARAMETERS	*Params = pOidHandle->tlvParser.parsedTlv.paramSetPrivacyExemptionList;
	WDI_PRIVACY_EXEMPTION_LIST_CONTAINER	PrivacyList = {0};
	u4Byte		i = 0;

	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);

	// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("sizeof(WDI_MESSAGE_HEADER) %d, InputBufferLength: %d\n", sizeof(WDI_MESSAGE_HEADER), pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength));
	RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, "Information buffer:\n", pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer, pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength);


	if( Params && Params->Optional.PrivacyExemptionEntry_IsPresent )
	{
		pNdisCommon->PrivacyExemptionEntrieNum = Params->PrivacyExemptionEntry.ElementCount;
		for(i = 0; i < Params->PrivacyExemptionEntry.ElementCount; i++)
		{
			PrivacyList = Params->PrivacyExemptionEntry.pElements[i];
				pNdisCommon->PrivacyExemptionEntries[i].usEtherType = 
					PrivacyList.EtherType;
				pNdisCommon->PrivacyExemptionEntries[i].usExemptionActionType = 
					PrivacyList.ExemptionActionType;	
				pNdisCommon->PrivacyExemptionEntries[i].usExemptionPacketType = 
					PrivacyList.ExemptionPacketType;									
			}

			RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, 
				"Privacy exempt list:", 
				pNdisCommon->PrivacyExemptionEntries, 
				(int)(pNdisCommon->PrivacyExemptionEntrieNum * sizeof(DOT11_PRIVACY_EXEMPTION)));			


	}
	
	return ndisStatus;
}


NDIS_STATUS
Wdi_Set_Power_State(	
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	
	ndisStatus = N6SdioWdi_Mgnt_SetPower(pAdapter, pOidHandle);

	return ndisStatus;
}


NDIS_STATUS
Wdi_Abort_Task(	
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	PWDI_MESSAGE_HEADER	pWdiHeader 
		= (PWDI_MESSAGE_HEADER) pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	ULONG					InputBufferLength 
		=  pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;

	WDI_TASK_ABORT_PARAMETERS		*Params = pOidHandle->tlvParser.parsedTlv.paramAbort;

	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);


	if( Params )
	{
		if(RT_STATUS_SUCCESS != OidHandle_Cancel(pAdapter, Params->CancelParameters.OriginalTaskOID, Params->CancelParameters.OriginalTransactionId, Params->CancelParameters.OriginalPortId))
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Wdi_Abort_Task(): failed to abort this task\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
		}

		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Wdi_Abort_Task(): Cancel Oid:%04x, TransactionId:%04x, PortId:%d\n", Params->CancelParameters.OriginalTaskOID, Params->CancelParameters.OriginalTransactionId, Params->CancelParameters.OriginalPortId));
	}
	else
	{
		ndisStatus = NDIS_STATUS_FAILURE;
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("Wdi_Abort_Task(): failed to ParseWdiAbortTask\n"));
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== Wdi_Abort_Task()\n"));
	return ndisStatus;
}

NDIS_STATUS
Wdi_Set_Add_Wol_Pattern(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS			ndisStatus		= NDIS_STATUS_SUCCESS;
	PWDI_MESSAGE_HEADER	pWdiHeader		= (PWDI_MESSAGE_HEADER) pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	WDI_SET_ADD_WOL_PATTERN_PARAMETERS	*Params	= pOidHandle->tlvParser.parsedTlv.paramSetAddWolPattern;
	WDI_PACKET_PATTERN_CONTAINER		*pPatternElements;
	WDI_IPv4_TCP_SYNC_CONTAINER			*pIPv4Elements;
	WDI_IPv6TCP_SYNC_CONTAINER			*pIPv6Elements;

	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO	pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);

	u1Byte	WoLBitMapPatternMask[MAX_WOL_BIT_MASK_SIZE];
	u1Byte	WoLBitMapPatternContent[MAX_WOL_PATTERN_SIZE];
	u1Byte	WoLBitMapPattern[MAX_WOL_PATTERN_SIZE];
	u1Byte	Index, ptnIndex;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> Wdi_Set_Add_Wol_Pattern()\n"));
	
	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);

	pPSC->bSupportWakeUp = TRUE;

	PlatformZeroMemory(WoLBitMapPatternMask, sizeof(WoLBitMapPatternMask));
	PlatformZeroMemory(WoLBitMapPatternContent, sizeof(WoLBitMapPatternContent));

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Pattern[%u]\n", 					Params->Optional.WakePacketPattern_IsPresent));
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("MagicPacketPattern[%u]\n", 		Params->Optional.WakePacketMagicPacketPatternId_IsPresent));
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Ipv4TcpSync[%u]\n", 				Params->Optional.WakePacketIpv4TcpSync_IsPresent));
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Ipv6TcpSync[%u]\n", 				Params->Optional.WakePacketIpv6TcpSync_IsPresent));
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("EapolRequestIdPatternId[%u]\n", 	Params->Optional.WakePacketEapolRequestIdPatternId_IsPresent));

	//2 Wake Packet Pattern
	pPatternElements = Params->WakePacketPattern.pElements;
	for(Index = 0; Index < Params->WakePacketPattern.ElementCount; Index++, pPatternElements++)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN[%d]:\n", Index));
			
		if(pPatternElements->PacketPattern.ElementCount <= MAX_WOL_PATTERN_SIZE)
		{
			PlatformMoveMemory(WoLBitMapPatternContent, pPatternElements->PacketPattern.pElements, pPatternElements->PacketPattern.ElementCount);
			RT_PRINT_DATA( COMP_OID_SET | COMP_POWER, DBG_LOUD, ("WoLBitMapPatternContent...\n"), 
				WoLBitMapPatternContent, sizeof(WoLBitMapPatternContent));
		}
		if(pPatternElements->PacketPatternMask.ElementCount <= MAX_WOL_BIT_MASK_SIZE)
		{
			PlatformMoveMemory(WoLBitMapPatternMask, pPatternElements->PacketPatternMask.pElements, pPatternElements->PacketPatternMask.ElementCount);						
			RT_PRINT_DATA( COMP_OID_SET | COMP_POWER, DBG_LOUD, ("WoLBitMapPatternMask...\n"), 
				WoLBitMapPatternMask, sizeof(WoLBitMapPatternMask));
		}
				
		//Find the index of the first empty entry.
		for(Index = 0; Index < MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter); Index++)
		{
			if(pPmWoLPatternInfo[Index].PatternId == pPatternElements->WakePacketPatternId)
			{
				Index = MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter);
				break;
			}

			if(pPmWoLPatternInfo[Index].PatternId == 0)
				break;
		}
					
		if(Index >= MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter))
		{
			ndisStatus = NDIS_STATUS_RESOURCES;
			RT_TRACE(COMP_POWER, DBG_LOUD,
				("SET OID_PNP_ADD_WOL_PATTERN: Return status(%#X). The number of wake up pattern is more than %d or the pattern Id is exist.\n", 
				ndisStatus, MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter)));
			return ndisStatus;
		}
		else
		{
			u1Byte	BROADCAST_ADDR[6]	= {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
			u1Byte	DONTCARE_ADDR[6]	= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			u1Byte	MULTICAST_ADDR[2]	= {0x33, 0x33};

			pPmWoLPatternInfo[Index].PatternType = eUnknownType;
			pPmWoLPatternInfo[Index].PatternId = pPatternElements->WakePacketPatternId;
			pPmWoLPatternInfo[Index].IsUserDefined = 0;

			//packet type.
			if(PlatformCompareMemory(WoLBitMapPatternContent, pAdapter->PermanentAddress, sizeof(pAdapter->PermanentAddress)) == 0 )
			{	//unicast
				//Skip IPv4/IPv6 TCP SYN pattern because it will hit HW bug for the CRC.
				// We offload this pattern to FW.
				pPmWoLPatternInfo[Index].PatternType = eUnicastPattern;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, 
						("OID_PM_ADD_WOL_PATTERN: IPv4/IPv6 TCP SYN pattern.\n"));
				
			}
			else if(PlatformCompareMemory(WoLBitMapPatternContent, MULTICAST_ADDR, sizeof(MULTICAST_ADDR)) == 0)
			{	//Multicast
				pPmWoLPatternInfo[Index].PatternType = eMulticastPattern;
			}
			else if(PlatformCompareMemory(WoLBitMapPatternContent, BROADCAST_ADDR, sizeof(BROADCAST_ADDR)) == 0 )
			{	//broadcast
				pPmWoLPatternInfo[Index].PatternType = eBroadcastPattern;
			}
			else if(PlatformCompareMemory(WoLBitMapPatternContent, DONTCARE_ADDR, sizeof(DONTCARE_ADDR)) == 0 )
			{
				pPmWoLPatternInfo[Index].PatternType = eDontCareDA;
			}
				
			{
				GetWOLWakeUpPattern(pAdapter, 
						(pu1Byte)&WoLBitMapPatternMask, 
						pPatternElements->PacketPatternMask.ElementCount, 
						(pu1Byte)&WoLBitMapPatternContent, 
						pPatternElements->PacketPattern.ElementCount,
						Index,
						FALSE
					);

				pPSC->WoLPatternNum++;
			}
	  	}
	
	}

	//2 Wake Packet Magic Packet
	if(Params->WakePacketMagicPacketPatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: Support magic packet.\n"));

		if(!pPSC->MagicPacketPatternId)
		{
			pPSC->MagicPacketPatternId = Params->WakePacketMagicPacketPatternId;
			pPSC->WoLPktNoPtnNum++;
		}
	}

	//2 Wake Packet Ipv4 Tcp Sync: Not verify
	pIPv4Elements = Params->WakePacketIpv4TcpSync.pElements;
	for(Index = 0; Index < Params->WakePacketIpv4TcpSync.ElementCount; Index++, pIPv4Elements++)
	{
		if(!pPSC->IPv4TcpSynPatternId)
		{
			pPSC->IPv4TcpSynPatternId = pIPv4Elements->PatternId;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("IPv4 PatternId = %d\n", pIPv4Elements->PatternId));
			pPSC->WoLPktNoPtnNum++;
		}
	}

	//2 Wake Packet Ipv6 Tcp Sync: Not verify
	pIPv6Elements = Params->WakePacketIpv6TcpSync.pElements;
	for(Index = 0; Index < Params->WakePacketIpv6TcpSync.ElementCount; Index++, pIPv6Elements++)
	{
		if(!pPSC->IPv6TcpSynPatternId)
		{
			pPSC->IPv6TcpSynPatternId = pIPv6Elements->PatternId;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("IPv6 PatternId = %d\n", pIPv6Elements->PatternId));
			pPSC->WoLPktNoPtnNum++;
		}
	}

	//2 Wake Packet Eapol Request
	if(Params->WakePacketEapolRequestIdPatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: Support EapolRequestIdMessage.\n"));
		
		if(!pPSC->EapolRequestIdMessagePatternId)
		{
			pPSC->EapolRequestIdMessagePatternId = Params->WakePacketEapolRequestIdPatternId;
			pPSC->WoLPktNoPtnNum++;
		}
	}



	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== Wdi_Set_Add_Wol_Pattern()\n"));
	return ndisStatus;
}


NDIS_STATUS
Wdi_Set_Remove_Wol_Pattern(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS			ndisStatus			= NDIS_STATUS_SUCCESS;
	PWDI_MESSAGE_HEADER	pWdiHeader			= (PWDI_MESSAGE_HEADER) pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	WDI_SET_REMOVE_WOL_PATTERN_PARAMETERS	*Params	= pOidHandle->tlvParser.parsedTlv.paramSetRemoveWolPattern;

	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO	pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	
	u4Byte	RemovePatternId = Params->PatternId;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> Wdi_Set_Remove_Wol_Pattern()\n"));

	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);


	if(RemovePatternId == pPSC->MagicPacketPatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET_OID_PM_REMOVE_WOL_PATTERN: Remove MagicPacket.\n"));
		
		pPSC->MagicPacketPatternId = 0;
		pPSC->WoLPktNoPtnNum--;
	}
	else if(RemovePatternId == pPSC->EapolRequestIdMessagePatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET_OID_PM_REMOVE_WOL_PATTERN: Remove EapolRequestIdMessage.\n"));
		
		pPSC->EapolRequestIdMessagePatternId = 0;
		pPSC->WoLPktNoPtnNum--;
	}
	else if(RemovePatternId == pPSC->IPv4TcpSynPatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET_OID_PM_REMOVE_WOL_PATTERN: Remove IPv4TcpSyn.\n"));
		
		pPSC->IPv4TcpSynPatternId = 0;
		pPSC->WoLPktNoPtnNum--;
	}
	else if(RemovePatternId == pPSC->IPv6TcpSynPatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET_OID_PM_REMOVE_WOL_PATTERN: Remove IPv6TcpSyn.\n"));
		
		pPSC->IPv6TcpSynPatternId = 0;
		pPSC->WoLPktNoPtnNum--;
	}
	else
	{
		u1Byte	Index;
		
		for(Index = 0; Index < MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter); Index++)
		{
			if(pPmWoLPatternInfo[Index].PatternId == RemovePatternId)
				break;
		}
		
		if(Index >= MAX_SUPPORT_WOL_PATTERN_NUM(pAdapter))
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD,("SET OID_PM_REMOVE_WOL_PATTERN: Cannot find the wake up pattern Id(%08X).\n", RemovePatternId));
		}
		else
		{
			//Reset the structure and set WFCRC register to non-zero value.
			pPmWoLPatternInfo[Index].PatternId = 0;
			PlatformZeroMemory(pPmWoLPatternInfo[Index].Mask, sizeof(pPmWoLPatternInfo[Index].Mask));
			pPmWoLPatternInfo[Index].CrcRemainder = 0xffff;
			pPmWoLPatternInfo[Index].IsPatternMatch = 0;
			pPmWoLPatternInfo[Index].IsSupportedByFW = 0;
						
			pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_WF_MASK, (pu1Byte)(&Index)); 
			pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_WF_CRC, (pu1Byte)(&Index)); 
			pPmWoLPatternInfo[Index].HwWFMIndex = 0xff; // reset the value after clear HW/CAM entry.

			pPSC->WoLPatternNum--;
		}
	}

	// Check WoWLAN support
	if(pPSC->WoLPktNoPtnNum + pPSC->WoLPatternNum)
		pPSC->bSupportWakeUp = TRUE;
	else
		pPSC->bSupportWakeUp = FALSE;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("WoLPktNoPtnNum = %d.\n", pPSC->WoLPktNoPtnNum));
	RT_TRACE(COMP_POWER, DBG_LOUD, ("WoLPatternNum = %d.\n", pPSC->WoLPatternNum));


	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== Wdi_Set_Remove_Wol_Pattern()\n"));
	return ndisStatus;
}

NDIS_STATUS
Wdi_Set_Multicast_List(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{

	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG					BytesRead = 0, BytesNeeded = 0;
	PWDI_MESSAGE_HEADER	pWdiHeader 	= (PWDI_MESSAGE_HEADER)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	ULONG					InputBufferLength = pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;	
	WDI_SET_MULTICAST_LIST_PARAMETERS	*param = pOidHandle->tlvParser.parsedTlv.paramSetMulticastList;
	WDI_MAC_ADDRESS		Address = {0};
	DOT11_MAC_ADDRESS		MulticastList[MAX_MCAST_LIST_NUM];
	u4Byte		i;

	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);

	if( param )
	{
		for(i=0; i< param->MulticastList.ElementCount; i++)
		{
		 	Address = param->MulticastList.pElements[i];
			CopyMem(MulticastList[i], Address.Address, sizeof(DOT11_MAC_ADDRESS));
			RT_PRINT_ADDR(COMP_OID_SET, DBG_TRACE, ("Wdi_Set_Multicast_List MulticastList\n"), MulticastList[i]);
			RT_PRINT_ADDR(COMP_OID_SET, DBG_TRACE, ("Wdi_Set_Multicast_List Address.Address\n"), Address.Address);	
		}

		ndisStatus = N6CSet_DOT11_MULTICAST_LIST(pAdapter,
					MulticastList,
					sizeof(DOT11_MAC_ADDRESS)*param->MulticastList.ElementCount,
					&BytesRead,
					&BytesNeeded);
	}
	else
	{
		// Shall we need to clear multicast list since parameter was empty?
	}
	
	return ndisStatus;
}

NDIS_STATUS
Wdi_Set_Add_Pm_Protocol_Offload(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS				ndisStatus	= NDIS_STATUS_SUCCESS;
	PMGNT_INFO				pMgntInfo	= &pAdapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC 		= GET_POWER_SAVE_CONTROL(pMgntInfo);
	PWDI_MESSAGE_HEADER		pWdiHeader	= (PWDI_MESSAGE_HEADER) pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	WDI_SET_ADD_PM_PROTOCOL_OFFLOAD_PARAMETERS_PARAMETERS *Params	= pOidHandle->tlvParser.parsedTlv.paramSetAddProtocolOffload;
		
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> Wdi_Set_Add_Pm_Protocol_Offload()\n"));
	
	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);


	//2 DOT11 RSN REKey Offload
	if(Params->DOT11RSNREKeyOffload.ProtocolOffloadId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("ProtocolOffloadId = %d\n", 
			Params->DOT11RSNREKeyOffload.ProtocolOffloadId));
		
		pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eGTKOffloadIdx] 
			= Params->DOT11RSNREKeyOffload.ProtocolOffloadId;
			
		//Copy kck, kek
		PlatformMoveMemory(&(pMgntInfo->PMDot11RSNRekeyPara.KCK), 
			Params->DOT11RSNREKeyOffload.KCK_CONTENT, 32); 

		RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("KCK_CONTENT:\n"), 
			pMgntInfo->PMDot11RSNRekeyPara.KCK, 16);
		RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("KEK_CONTENT:\n"), 
			pMgntInfo->PMDot11RSNRekeyPara.KEK, 16);
		
		pMgntInfo->PMDot11RSNRekeyPara.KeyReplayCounter = Params->DOT11RSNREKeyOffload.ReplayCounter;

		if(pPSC->RegGTKOffloadEnable)
			pPSC->GTKOffloadEnable = TRUE;
	}


	//2 ipv4 ARP Offload
	if(Params->ipv4ARPOffload.ProtocolOffloadId)
	{
		pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eARPOffloadIdx] 
			= Params->ipv4ARPOffload.ProtocolOffloadId;

		//Copy RemoteIPv4Address, HostIPv4Address & MacAddress
		PlatformMoveMemory(&(pMgntInfo->PMIPV4ARPPara.RemoteIPv4Address), 
			Params->ipv4ARPOffload.RemoteIPV4Address, 14); 
		
		RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("RemoteIPv4Address:\n"), 
			pMgntInfo->PMIPV4ARPPara.RemoteIPv4Address, 4);
		RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("HostIPv4Address:\n"), 
			pMgntInfo->PMIPV4ARPPara.HostIPv4Address, 4);
		RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("MacAddress:\n"), 
			pMgntInfo->PMIPV4ARPPara.MacAddress, 6);

		if(pPSC->RegARPOffloadEnable)
			pPSC->ARPOffloadEnable = TRUE;
	}


	//2 ipv6 ARP Offload
	if(Params->ipv6ARPOffload.ProtocolOffloadId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_PROTOCOL_OFFLOAD. (NS IPv6)\n"));
		
		if((pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] != 0) &&
			(pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx2] != 0))
		{
			ndisStatus = NDIS_STATUS_RESOURCES;
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Insufficient NS offload number!! Status(%#X) <==\n", ndisStatus));
		}
		else
		{
			u1Byte						IPv6NSIndex = 0;
			RT_PM_IPV6_NS_PARAMETERS	*NsParams;
		
			if( pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] ==  0)
			{
				pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] 
					= Params->ipv6ARPOffload.ProtocolOffloadId;
				IPv6NSIndex = 0;
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==> eNSOffloadIdx1: \n"));
			}
			else
			{
				pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx2] 
					= Params->ipv6ARPOffload.ProtocolOffloadId;
				IPv6NSIndex = 1;
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==> eNSOffloadIdx2: \n"));
			}

			NsParams = &(pMgntInfo->PMIPV6NSPara[IPv6NSIndex]);
			
			PlatformMoveMemory(NsParams->RemoteIPv6Address, 
				Params->ipv6ARPOffload.RemoteIPV6Address, 16);
			PlatformMoveMemory(NsParams->SolicitedNodeIPv6Address, 
				Params->ipv6ARPOffload.SolicitdNodeIPv6Address, 16);
			PlatformMoveMemory(NsParams->MacAddress, 
				Params->ipv6ARPOffload.MacAddress.Address, 6);
			PlatformMoveMemory(NsParams->TargetIPv6Addresses[0], 
				Params->ipv6ARPOffload.TargetIPV6Address1, 16);
			PlatformMoveMemory(NsParams->TargetIPv6Addresses[1], 
				Params->ipv6ARPOffload.TargetIPV6Address2, 16);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM RemoteIPv6Address: \n"), 
				NsParams->RemoteIPv6Address, 16);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM SolicitedNodeIPv6Address: \n"), 
				NsParams->SolicitedNodeIPv6Address, 16);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM MacAddress: \n"), 
				NsParams->MacAddress, 6);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM TargetIPv6Addresses[0]: \n"), 
				NsParams->TargetIPv6Addresses[0], 16);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM TargetIPv6Addresses[1]: \n"), 
				NsParams->TargetIPv6Addresses[1], 16);

			if(pPSC->RegNSOffloadEnable)
				pPSC->NSOffloadEnable = TRUE;
		}
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== Wdi_Set_Add_Pm_Protocol_Offload()\n"));
	
	return ndisStatus;
}

NDIS_STATUS
Wdi_Set_Remove_Pm_Protocol_Offload(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS				ndisStatus	= NDIS_STATUS_SUCCESS;
	PMGNT_INFO 				pMgntInfo	= &pAdapter->MgntInfo;
	PWDI_MESSAGE_HEADER		pWdiHeader	= (PWDI_MESSAGE_HEADER) pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	WDI_SET_REMOVE_PM_PROTOCOL_OFFLOAD_PARAMETERS *Params	= pOidHandle->tlvParser.parsedTlv.paramSetRemoveProtocolOffload;

	u4Byte	MatchId, ProtocolOffloadId  = Params->RemovePMOffload;
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> Wdi_Set_Remove_Pm_Protocol_Offload()\n"));
	
	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);

	for(MatchId = 0; MatchId < 4; MatchId++)
	{
		if(ProtocolOffloadId == pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[MatchId])
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Find match protocol offload ID idx (%d)\n", MatchId));
			
			if(MatchId == eGTKOffloadIdx)
			{
				PlatformZeroMemory(&(pMgntInfo->PMDot11RSNRekeyPara), sizeof(RT_PM_DOT11_RSN_REKEY_PARAMETERS)); 
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==>REMOVE eGTKOffloadIdx\n"));
			}
			else if(MatchId == eARPOffloadIdx)
			{
				PlatformZeroMemory(&(pMgntInfo->PMIPV4ARPPara), sizeof(RT_PM_IPV4_ARP_PARAMETERS)); 
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==>REMOVE eARPOffloadIdx\n"));
			}
			else if(MatchId == eNSOffloadIdx1 )
			{	
				// Disable eNSOffloadIdx1 , set ID = 0 !!
				pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] = 0;
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==>REMOVE eNSOffloadIdx1\n"));
			}
			else if(MatchId == eNSOffloadIdx2)
			{
				// Disable eNSOffloadIdx2 , set ID = 0 !!
				pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx2] = 0;
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==>REMOVE eNSOffloadIdx2\n"));
			}
		}
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== Wdi_Set_Remove_Pm_Protocol_Offload()\n"));
	
	return ndisStatus;
}

NDIS_STATUS
Wdi_Set_Connection_Quality(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG					BytesRead = 0, BytesNeeded = 0;
	PMGNT_INFO				pMgntInfo=&(pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PWDI_MESSAGE_HEADER		pWdiHeader 
		= (PWDI_MESSAGE_HEADER)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
	ULONG					InputBufferLength 
		= pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength;	
	WDI_SET_CONNECTION_QUALITY_PARAMETERS	*param = pOidHandle->tlvParser.parsedTlv.paramSetConnectionQuality;

	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);

	// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
	RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("sizeof(WDI_MESSAGE_HEADER) %d, InputBufferLength: %d\n", sizeof(WDI_MESSAGE_HEADER), pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength));
	RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, "Information buffer:\n", pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer, pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InputBufferLength);


	if( param)
	{
		pPSC->ConnectionQuality = param->Quality;

		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Wdi_Set_Connection_Quality ConnectionQuality 0x%x\n", pPSC->ConnectionQuality));
		
		if(param->Optional.LowLatencyParameters_IsPresent)
		{
			pPSC->MaximumOffChannelOperationTime = param->LowLatencyParameters.MaximumOffChannelOperationTime;
			pPSC->RoamingNeededLinkQualityThreshold = param->LowLatencyParameters.RoamingNeededLinkQualityThreshold;	

			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Wdi_Set_Connection_Quality MaximumOffChannelOperationTime 0x%x\n", pPSC->MaximumOffChannelOperationTime));
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Wdi_Set_Connection_Quality RoamingNeededLinkQualityThreshold 0x%x\n", pPSC->RoamingNeededLinkQualityThreshold));			
		}
	}
	
	
	return ndisStatus;
}

NDIS_STATUS
Wdi_Get_Auto_Power_Save(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER		pDefAdapter = GetDefaultAdapter(pAdapter);
	PMGNT_INFO		pMgntInfo=&(pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	WDI_GET_AUTO_POWER_SAVE_RESULTS	adapterPowerSave = {0};
	PUCHAR			pOutput = NULL;
	ULONG			length = 0;
    
	PWDI_MESSAGE_HEADER pOidHeader 
		= (PWDI_MESSAGE_HEADER)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	adapterPowerSave.AutoPowerSaveParameters.EnableAutoPSM = 
		pMgntInfo->bInAutoPowerSavemode;

	adapterPowerSave.AutoPowerSaveParameters.BeaconInterval = 
	pMgntInfo->dot11BeaconPeriod;

	adapterPowerSave.AutoPowerSaveParameters.ListenInterval = 
	(UINT8)pMgntInfo->ListenInterval;

	if(pMgntInfo->ListenIntervalInDX)
	{
		adapterPowerSave.AutoPowerSaveParameters.ListenIntervalInDx = 
			(UINT8)pMgntInfo->ListenIntervalInDX;		
	}
	else
	{
		adapterPowerSave.AutoPowerSaveParameters.ListenIntervalInDx = 0xFF;
	}

	//2 Todo
	if(pPSC->ConnectionQuality == WDI_CONNECTION_QUALITY_AUTO_POWER_SAVE ||
		pPSC->ConnectionQuality == WDI_CONNECTION_QUALITY_LOW_LATENCY)
	{
		adapterPowerSave.AutoPowerSaveParameters.PowerMode = TranslatePsToWdiPs(pPSC->PowerMode);
		adapterPowerSave.AutoPowerSaveParameters.PowerModeinDx = TranslatePsToWdiPs(pPSC->PowerMode);
	}
	else
	{
		adapterPowerSave.AutoPowerSaveParameters.PowerMode = WDI_POWER_SAVE_LEVEL_NO_POWER_SAVE;
		adapterPowerSave.AutoPowerSaveParameters.PowerModeinDx = WDI_POWER_SAVE_LEVEL_NO_POWER_SAVE;	
	}

	//2 Todo
	adapterPowerSave.AutoPowerSaveParameters.Reason = 0;
	
	adapterPowerSave.AutoPowerSaveParameters.MillisecondsSinceStart = 
		(PlatformGetCurrentTime() - pDefAdapter->DriverUpTime) / 1000;

	if (pPSC->bLeisurePs && pMgntInfo->dot11PowerSaveMode != eActive)
	{
		adapterPowerSave.AutoPowerSaveParameters.MillisecondsInPowerSave =
			(PlatformGetCurrentTime() - pPSC->LastLPSEnterTime) / 1000;
	}
	else if (pPSC->LastLPSEnterTime)
	{		
		adapterPowerSave.AutoPowerSaveParameters.MillisecondsInPowerSave =
			(pPSC->LastLPSLeaveTime - pPSC->LastLPSEnterTime) / 1000;
	}	
	else
	{
		adapterPowerSave.AutoPowerSaveParameters.MillisecondsInPowerSave = 0;
	}

	//2 Todo
	adapterPowerSave.AutoPowerSaveParameters.ReceivedMulticastPackets = 
		adapterPowerSave.AutoPowerSaveParameters.SentMulticastPackets = 
		adapterPowerSave.AutoPowerSaveParameters.ReceivedUnicastPackets = 
		adapterPowerSave.AutoPowerSaveParameters.SentUnicastPacket = 0;
	
	ndisStatus = GenerateWdiGetAutoPowerSave(
			//pAdapterCapabilities,
			&adapterPowerSave,
			0,
			&pAdapter->pPortCommonInfo->WdiData.TlvContext,
			&length,
			&pOutput );

//	CleanupParsedWdiGetAutoPowerSave(&adapterPowerSave);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("GenerateWdiGetAutoPowerSave() return length = %d\n", length));
	RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("buffer\n"), pOutput, length);

	if(NDIS_STATUS_SUCCESS != ndisStatus)
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("GenerateWdiGetAutoPowerSave failed for status: 0x%08X\n", ndisStatus));
		return ndisStatus;
	}

	if (pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength >= 
		(length + sizeof(WDI_MESSAGE_HEADER)))
	{
		PlatformMoveMemory(
			((UCHAR *)pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer) + sizeof(WDI_MESSAGE_HEADER),
			pOutput,
			length);
		pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten += length;
	}
	else
	{
		ndisStatus = NDIS_STATUS_BUFFER_TOO_SHORT;
		pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesNeeded += (length + sizeof(WDI_MESSAGE_HEADER));
 		RT_TRACE(COMP_INIT, DBG_WARNING, ("[Error] GenerateWdiGetAdapterCapabilities() failed with status(%x)\n", ndisStatus));       
	}
	
	if(pOutput)
	{
		FreeGenerated(pOutput);
		pOutput = NULL;
	}

	pOidHeader->Status = ndisStatus;
	
	return ndisStatus;
}

NDIS_STATUS
Wdi_IHV_Request(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	PTLV_WDI_STRUCT		pTlvData = NULL;
	ULONG					BytesWritten = 0, BytesRead = 0, BytesNeeded = 0;
	PNDIS_OID_REQUEST  pNdisRequest = pOidHandle->pNdisRequest;
	PWDI_MESSAGE_HEADER	pWdiHeader 
		= (PWDI_MESSAGE_HEADER)pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;
//20150915-KenSun
//Temp Workaround for setting a private OID 
#if 	1
	PCCX_NIC_SPECIFIC_EXTENSION	ccxData;
	PIHV_CCX_TLV				pTlv = NULL;
	u1Byte i = 0;
#endif
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> Wdi_IHV_Request()\n"));
	pWdiHeader->Status = NDIS_STATUS_SUCCESS;
	pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(WDI_MESSAGE_HEADER);


	pTlvData = (PTLV_WDI_STRUCT)((pu1Byte)pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer + sizeof(WDI_MESSAGE_HEADER));

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Type = 0x%x, Length = %d\n", pTlvData->Type, pTlvData->Length));
	if(WDI_TLV_IHV_DATA == pTlvData->Type)
	{

		ndisStatus = WAPI_EventQuerySetInformation(
						pAdapter,
						OID_DOT11_NIC_SPECIFIC_EXTENSION,
						pTlvData->Value,
						(ULONG)pTlvData->Length,
						(ULONG)pTlvData->Length,	// this correct?
						(ULONG)pTlvData->Length,
						(PULONG)&BytesWritten,
						(PULONG)&BytesRead,
						(PULONG)&BytesNeeded
						);
		if(NDIS_STATUS_INVALID_OID  != ndisStatus)
		{
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("WAPI_EventQuerySetInformation BytesWritten = %d\n", BytesWritten));
			pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten += BytesWritten;
			pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten += 4;	// type + length
			pNdisRequest->DATA.METHOD_INFORMATION.BytesNeeded += BytesWritten;
			pNdisRequest->DATA.METHOD_INFORMATION.BytesNeeded += 4;	// type + length

			RT_PRINT_DATA(COMP_OID_SET, DBG_TRACE,("WAPI_EventQuerySetInformation Output buffer\n"), pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer,
										pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten);
			return ndisStatus;
		}

		ndisStatus = NDIS_STATUS_SUCCESS;
		if(NDIS_STATUS_SUCCESS == ndisStatus)
		{
			RT_TRACE(COMP_OID_SET, DBG_TRACE, ("BytesWritten = %d\n", BytesWritten));
			pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten += BytesWritten;
			pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten += 4;	// type + length
			pNdisRequest->DATA.METHOD_INFORMATION.BytesNeeded += BytesWritten;
			pNdisRequest->DATA.METHOD_INFORMATION.BytesNeeded += 4;	// type + length
//20150921-KenSun
//Workaround for set method for RTK private OIDs
//Because WDI would always pass 2046 in pNdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength
//which is not the same value that passed in RTK Appilcations. (All RTK appilcation pass 0 outputbufferlength when issuing a SET OID request)
//Therefore, we cannot use it as refence.
//For interfacing with WDI, we have to make sure BytesWriiten is sizeof(WDI_MESSAGE_HEADER) when dealing with a SET method request. 
//
#if 1			
			ccxData = (PCCX_NIC_SPECIFIC_EXTENSION)pTlvData->Value;
			if(ccxData->event == CCX_EVENT_OID)
			{
				pTlv = &ccxData->tlvData.tlv[0];
				for(i = 0; pTlv && i < ccxData->tlvData.tlvCount; i++)
				{

			              if(pTlv->type == CCX_TLV_SET_DATA)
			              {
					  	pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten =  sizeof(WDI_MESSAGE_HEADER);
						RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> For set method request, we should set BytesWritten to be sizeof(WDI_MESSAGE_HEADER), BytesWritten = %d\n",pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten));
			              }
				}
			}
#endif
			RT_PRINT_DATA(COMP_OID_SET, DBG_TRACE, ("Output buffer\n"), pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer,
										pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten);
		}
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<== Wdi_IHV_Request()\n"));
	return ndisStatus;
}


NDIS_STATUS
Wdi_Get_Statistics(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	NDIS_STATUS						ndisStatus = NDIS_STATUS_SUCCESS;
	WDI_GET_STATISTICS_PARAMETERS	Params = {0};
	PUCHAR							pOutput = NULL;
	ULONG							length = 0;
	PMGNT_INFO						pMgntInfo = &(pAdapter->MgntInfo);
	WDI_MAC_STATISTICS_CONTAINER	MacStatistics[2];
	WDI_PHY_STATISTICS_CONTAINER	PhyStatistics[WDI_802_11_MAX_NUM_PHY_TYPES];
    	PNDIS_OID_REQUEST  pNdisRequest = pOidHandle->pNdisRequest;
	PWDI_MESSAGE_HEADER 			pOidHeader 
		= (PWDI_MESSAGE_HEADER)pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer;

	FunctionIn(COMP_OID_QUERY);


	if( pMgntInfo->mAssoc == TRUE )
	{
		PlatformZeroMemory(&MacStatistics[0], sizeof(WDI_MAC_STATISTICS_CONTAINER)*2);

		// MacStatistics[0] BSSID
		cpMacAddr(MacStatistics[0].MACAddress.Address, pMgntInfo->Bssid);
		MacStatistics[0].TransmittedFrameCount = pAdapter->TxStats.NumTxUnicast;
		MacStatistics[0].ReceivedFrameCount = pAdapter->RxStats.NumRxUnicast;
		MacStatistics[0].WEPExcludedCount = pAdapter->RxStats.NumRxExcludeUnencryptedUnicast;
		MacStatistics[0].TKIPLocalMICFailures = pAdapter->RxStats.NumRxTKIPLocalMICFailuresUnicast;
		MacStatistics[0].TKIPReplays = pAdapter->RxStats.NumRxTKIPReplayUnicast;
		MacStatistics[0].TKIPICVErrorCount = pAdapter->RxStats.NumRxTKIPICVErrorUnicast;
		MacStatistics[0].CCMPFormatErrors = pAdapter->RxStats.NumRxCCMPDecryptErrorsUnicast;
		MacStatistics[0].CCMPReplays = pAdapter->RxStats.NumRxCCMPReplayUnicast;
		MacStatistics[0].CCMPDecryptErrors = pAdapter->RxStats.NumRxCCMPDecryptErrorsUnicast;
		MacStatistics[0].WEPUndecryptableCount = pAdapter->RxStats.NumRxWEPUndecryptableUnicast; 
		MacStatistics[0].WEPICVErrorCount = pAdapter->RxStats.NumRxWEPICVErrorUnicast;
		MacStatistics[0].DecryptSuccessCount = pAdapter->RxStats.NumRxDecryptSuccessUnicast;
		MacStatistics[0].DecryptFailureCount = pAdapter->RxStats.NumRxDecryptFailureUnicast;

		// MacStatistics[1] Multicast and Broadcast
		cpMacAddr(MacStatistics[1].MACAddress.Address, BroadcastAddress);
		MacStatistics[1].TransmittedFrameCount = pAdapter->TxStats.NumTxMulticast + pAdapter->TxStats.NumTxBroadcast;
		MacStatistics[1].ReceivedFrameCount = pAdapter->RxStats.NumRxMulticast + pAdapter->RxStats.NumRxBroadcast;
		MacStatistics[1].WEPExcludedCount = pAdapter->RxStats.NumRxExcludeUnencryptedMulticast + pAdapter->RxStats.NumRxExcludeUnencryptedBroadcast;
		MacStatistics[1].TKIPLocalMICFailures = pAdapter->RxStats.NumRxTKIPLocalMICFailuresMulticast + pAdapter->RxStats.NumRxTKIPLocalMICFailuresBroadcast;
		MacStatistics[1].TKIPReplays = pAdapter->RxStats.NumRxTKIPReplayMulticast;
		MacStatistics[1].TKIPICVErrorCount = pAdapter->RxStats.NumRxTKIPICVErrorMulticast + pAdapter->RxStats.NumRxTKIPICVErrorBroadcast;
		MacStatistics[1].CCMPFormatErrors = pAdapter->RxStats.NumRxCCMPDecryptErrorsMulticast + pAdapter->RxStats.NumRxCCMPDecryptErrorsBroadcast;
		MacStatistics[1].CCMPReplays = pAdapter->RxStats.NumRxCCMPReplayMulticast;
		MacStatistics[1].CCMPDecryptErrors = pAdapter->RxStats.NumRxCCMPDecryptErrorsMulticast + pAdapter->RxStats.NumRxCCMPDecryptErrorsBroadcast;
		MacStatistics[1].WEPUndecryptableCount = pAdapter->RxStats.NumRxWEPUndecryptableMulticast + pAdapter->RxStats.NumRxWEPUndecryptableBroadcast; 
		MacStatistics[1].WEPICVErrorCount = pAdapter->RxStats.NumRxWEPICVErrorMulticast + pAdapter->RxStats.NumRxWEPICVErrorBroadcast;
		MacStatistics[1].DecryptSuccessCount = pAdapter->RxStats.NumRxDecryptSuccessMulticast + pAdapter->RxStats.NumRxDecryptSuccessBroadcast;
		MacStatistics[1].DecryptFailureCount = pAdapter->RxStats.NumRxDecryptFailureMulticast + pAdapter->RxStats.NumRxDecryptFailureBroadcast;
		
		Params.PeerMACStatistics.ElementCount = 2;
		Params.PeerMACStatistics.pElements = MacStatistics;

		PlatformZeroMemory(&PhyStatistics[0], sizeof(WDI_PHY_STATISTICS_CONTAINER)*WDI_802_11_MAX_NUM_PHY_TYPES);
		switch (pMgntInfo->dot11CurrentWirelessMode)
		{
			default:
				PhyStatistics[0].PhyType = WDI_PHY_TYPE_OFDM;
				break;

			case WIRELESS_MODE_A:
			case WIRELESS_MODE_G:
				PhyStatistics[0].PhyType = WDI_PHY_TYPE_OFDM;
				break;

			case WIRELESS_MODE_B:
				PhyStatistics[0].PhyType = WDI_PHY_TYPE_DSSS;
				break;

			case WIRELESS_MODE_N_24G:
			case WIRELESS_MODE_N_5G:
			case WIRELESS_MODE_AUTO:
				PhyStatistics[0].PhyType = WDI_PHY_TYPE_HT;
				break;

			case WIRELESS_MODE_AC_5G:
			case WIRELESS_MODE_AC_ONLY:
			case WIRELESS_MODE_AC_24G:
				PhyStatistics[0].PhyType = WDI_PHY_TYPE_VHT;
				break;

				
		}
		PhyStatistics[0].TransmittedFrameCount = pAdapter->TxStats.NumTxOkTotal;
		PhyStatistics[0].GroupTransmittedFrameCount = pAdapter->TxStats.NumTxMulticast + pAdapter->TxStats.NumTxBroadcast;
		PhyStatistics[0].FailedCount = pAdapter->TxStats.NumTxErrTotal; 
		PhyStatistics[0].RetryCount = pAdapter->TxStats.NumTxRetryCount; 
		PhyStatistics[0].MultipleRetryCount = 0;				// TODO
		PhyStatistics[0].MaxTXLifetimeExceededCount = 0;		// TODO
		PhyStatistics[0].TransmittedFragmentCount = pAdapter->TxStats.NumTxOkTotal; 
		PhyStatistics[0].RTSSuccessCount = 0;				// TODO
		PhyStatistics[0].RTSFailureCount = 0;				// TODO
		PhyStatistics[0].ACKFailureCount = 0;				// TODO
		PhyStatistics[0].ReceivedFrameCount = pAdapter->RxStats.NumRxFramgment;
		PhyStatistics[0].GroupReceivedFrameCount = pAdapter->RxStats.NumRxFramgment; 
		PhyStatistics[0].PromiscuousReceivedFrameCount = 0;	// TODO
		PhyStatistics[0].MaxRXLifetimeExceededCount = 0;	// TODO
		PhyStatistics[0].FrameDuplicateCount = 0;			// TODO
		PhyStatistics[0].ReceivedFragmentCount = pAdapter->RxStats.NumRxFramgment; 
		PhyStatistics[0].PromiscuousReceivedFragmentCount = 0;	// TODO
		PhyStatistics[0].FCSErrorCount = pAdapter->RxStats.NumRxErrTotalUnicast + pAdapter->RxStats.NumRxErrTotalMulticast;
		Params.PhyStatistics.ElementCount = 1;
		Params.PhyStatistics.pElements = PhyStatistics;
	}
	else
	{
		u1Byte	count = 0;
		
		PlatformZeroMemory(&MacStatistics[0], sizeof(WDI_MAC_STATISTICS_CONTAINER)*2);
		cpMacAddr(MacStatistics[0].MACAddress.Address, BroadcastAddress);
		Params.PeerMACStatistics.ElementCount = 1;
		Params.PeerMACStatistics.pElements = MacStatistics;

		PlatformZeroMemory(&PhyStatistics, sizeof(WDI_PHY_STATISTICS_CONTAINER)*WDI_802_11_MAX_NUM_PHY_TYPES);
		for (count = 0; count < pAdapter->pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries; count ++)
		{
			PhyStatistics[count].PhyType = pAdapter->pNdisCommon->pDot11PhyMIBs[count].PhyType;
		}
		Params.PhyStatistics.ElementCount = pAdapter->pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries;
		Params.PhyStatistics.pElements = PhyStatistics;
	}

	ndisStatus = GenerateWdiGetStatistics(
			&Params,
			0,
			&pAdapter->pPortCommonInfo->WdiData.TlvContext,
			&length,
			&pOutput );

	RT_TRACE(COMP_INIT, DBG_LOUD, ("GenerateWdiGetStatistics() return length = %d\n", length));
	RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("buffer\n"), pOutput, length);

	if(NDIS_STATUS_SUCCESS != ndisStatus)
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("GenerateWdiGetStatistics failed for status: 0x%08X\n", ndisStatus));
		return ndisStatus;
	}

	if (pNdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength >= 
		(length + sizeof(WDI_MESSAGE_HEADER)))
	{
		PlatformMoveMemory(
			((UCHAR *)pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer) + sizeof(WDI_MESSAGE_HEADER),
			pOutput,
			length);
		pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten += length;
	}
	else
	{
		ndisStatus = NDIS_STATUS_BUFFER_TOO_SHORT;
		pNdisRequest->DATA.METHOD_INFORMATION.BytesNeeded += (length + sizeof(WDI_MESSAGE_HEADER));
 		RT_TRACE(COMP_INIT, DBG_WARNING, ("[Error] GenerateWdiGetStatistics() failed with status(%x)\n", ndisStatus));       
	}
	
	if(pOutput)
	{
		FreeGenerated(pOutput);
		pOutput = NULL;
	}

	pOidHeader->Status = ndisStatus;
	
	FunctionOut(COMP_OID_QUERY);
	
	return ndisStatus;
}


NDIS_STATUS
WDI_OID_TASK_OPEN(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Not Support WDI_OID_TASK_OPEN\n"));

	return ndisStatus;
}


NDIS_STATUS
WDI_OID_TASK_CLOSE(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{

	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Not Support WDI_OID_TASK_CLOSE\n"));

	return ndisStatus;
}


NDIS_STATUS
WDI_OID_TASK_SCAN(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	return Wdi_Task_Scan(pAdapter, pOidHandle);
}

NDIS_STATUS
WDI_OID_TASK_P2P_DISCOVER(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_TASK_P2P_DISCOVER_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramP2pDiscover;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		VOID					*req = NULL;

		P2P_INFO				*info = GET_P2P_INFO(pAdapter);
		FRAME_BUF 				*buf = NULL;
		
		// additional ie
		if(param->Optional.VendorIEs_IsPresent)
		{
			P2P_AddIe_Set(
				&info->AdditionalIEs, P2P_ADD_IE_PROBE_REQUEST,
				param->VendorIEs.ElementCount, param->VendorIEs.pElements
				);
		}

		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("DiscoveryType = %d, ScanType = %d\n", 
			param->DiscoverMode.DiscoveryType, param->DiscoverMode.ScanType));

		// construct req
		if(NULL != (req = CustomScan_AllocReq(GET_CUSTOM_SCAN_INFO(pAdapter), NULL, NULL)))
		{
			static u1Byte bcst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
			
			buf = CustomScan_GetProbeReqBuf(req);

			if(param->SSIDList.ElementCount)
			{
				WDI_SSID *ssid = &param->SSIDList.pElements[0];
				
				RT_PRINT_STR(COMP_OID_SET, DBG_LOUD, "to probe with ssid", ssid->pElements, ssid->ElementCount);
				P2P_Construct_ProbeReqEx(buf, info, bcst, (u1Byte)ssid->ElementCount, (u1Byte *)ssid->pElements);
			}
			else
			{
				P2P_Construct_ProbeReq(buf, info);
			}

			if(param->Optional.DiscoveryChannelSettings_IsPresent)
			{
				wdi_ConstructP2PSpecificChnlReq(pAdapter, param, buf, req);
			}
			else
			{
				wdi_ConstructP2PScanPhaseReq(pAdapter, param, req);
				wdi_ConstructP2PFindPhaseReq(pAdapter, param, buf, req);
			}

			CustomScan_SetRepeatCount(req, 16); // use time bound to control overall time

			if(WDI_P2P_SCAN_TYPE_PASSIVE == param->DiscoverMode.ScanType)
				CustomScan_ForcePassiveScan(req);
			
			CustomScan_SetTimeBound(req, param->DwellTime.MaximumScanTime);
			CustomScan_SetupCbCtx(req, wdi_P2PDiscoveryCb, pAdapter);
			
			CustomScan_IssueReq(GET_CUSTOM_SCAN_INFO(pAdapter), req, CUSTOM_SCAN_SRC_TYPE_P2P, "wdi-p2p-discovery");
		}
	}while(FALSE);

	FunctionOut(COMP_OID_SET);

	return status;
}


NDIS_STATUS
WDI_OID_TASK_CONNECT(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Task_Connect(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_TASK_ROAM(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Task_Roam(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_TASK_DOT11_RESET(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_TASK_DOT11_RESET_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramDot11Reset;

	NDIS_OID_REQUEST			*req = NULL;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		if(NULL == (req = Wdi_Xlat_AllocResetOid(pOidHandle, param)))
		{
			status = NDIS_STATUS_RESOURCES;
			break;
		}

		if(NDIS_STATUS_SUCCESS != (status = N6C_OID_DOT11_RESET_REQUEST(pAdapter, req)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N6C_OID_DOT11_RESET_REQUEST returns: 0x%08X\n", status));
			break;
		}

		// Indicate completed successfully
		WDI_CMD_ResetComplete(pAdapter, req, status);
	}while(FALSE);

	N63CResetNetworkListOffload(pAdapter);

	// MAC Address Randomization 
	if(param->Optional.ResetMACAddress_IsPresent && pAdapter->MgntInfo.RegSupportMACRandom)
	{
		cpMacAddr( pAdapter->CurrentAddress, param->ResetMACAddress.Address);
		pAdapter->HalFunc.SetHwRegHandler( pAdapter , HW_VAR_MAC_ADDR_RANDOM , pAdapter->CurrentAddress);
		RT_TRACE(COMP_MLME, DBG_LOUD, ("ResetMACAddress : 0x%x : 0x%x : 0x%x : 0x%x : 0x%x : 0x%x ",param->ResetMACAddress.Address[0],param->ResetMACAddress.Address[1],param->ResetMACAddress.Address[2],param->ResetMACAddress.Address[3],param->ResetMACAddress.Address[4],param->ResetMACAddress.Address[5]));
	}

	// cleanup
	if(req && NDIS_STATUS_SUCCESS != status) 
		Wdi_Xlat_FreeOid(req);

	FunctionOut(COMP_OID_SET);

	return status;
}


NDIS_STATUS
WDI_OID_TASK_DISCONNECT(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Task_Disconnect(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_TASK_P2P_SEND_REQUEST_ACTION_FRAME(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramP2pSendRequestActionFrame;
	VOID 						*req = NULL;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		if(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST == param->RequestParams.RequestFrameType)
		{
			req = WDI_SendAction_GoNegReq(pAdapter, param, wdi_SendP2pReqActionFrameStateCb);
		}
		else if(WDI_P2P_ACTION_FRAME_INVITATION_REQUEST == param->RequestParams.RequestFrameType)
		{
			req = WDI_SendAction_InvitationReq(pAdapter, param, wdi_SendP2pReqActionFrameStateCb);
		}
		else if(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST== param->RequestParams.RequestFrameType)
		{
			req = WDI_SendAction_PdReq(pAdapter, param, wdi_SendP2pReqActionFrameStateCb);
		}
		else
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("unknown frame type: %u\n", param->RequestParams.RequestFrameType));
			req = NULL;
		}

		if(!req)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("send p2p req action frame failed\n"));
			status = NDIS_STATUS_FAILURE;
			break;
		}
	}while(FALSE);
	
	FunctionOut(COMP_OID_SET);

	return status;
}


NDIS_STATUS
WDI_OID_TASK_P2P_SEND_RESPONSE_ACTION_FRAME(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	NDIS_STATUS 				status = NDIS_STATUS_SUCCESS;
	WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramP2pSendResponseActionFrame;
	VOID 						*req = NULL;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		// termincate any ongoing scan for sending rsp 
		WDI_SendAction_TerminateReq(pAdapter);

		if(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE == param->ResponseParams.ResponseFrameType)
		{
			req = WDI_SendAction_GoNegRsp(pAdapter, param, wdi_SendP2pRspActionFrameStateCb);
		}
		else if(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM == param->ResponseParams.ResponseFrameType)
		{
			req = WDI_SendAction_GoNegConfirm(pAdapter, param, wdi_SendP2pRspActionFrameStateCb);
		}
		else if(WDI_P2P_ACTION_FRAME_INVITATION_RESPONSE == param->ResponseParams.ResponseFrameType)
		{
			req = WDI_SendAction_InvitationRsp(pAdapter, param, wdi_SendP2pRspActionFrameStateCb);
		}
		else if(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == param->ResponseParams.ResponseFrameType)
		{
			req = WDI_SendAction_PdRsp(pAdapter, param, wdi_SendP2pRspActionFrameStateCb);
		}
		else
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("unknown frame type: %u\n", param->ResponseParams.ResponseFrameType));
			req = NULL;
		}

		if(!req)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("send p2p rsp action frame failed\n"));
			status = NDIS_STATUS_FAILURE;
			break;
		}
	}while(FALSE);
	
	FunctionOut(COMP_OID_SET);

	return status;
}

NDIS_STATUS
WDI_OID_TASK_SET_RADIO_STATE(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	return Wdi_Task_Set_Radio_State(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_TASK_CREATE_PORT(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	PORT_HELPER					*pPortHelper = pAdapter->pPortCommonInfo->pPortHelper;
	
	status = Wdi_Task_CreatePort(pAdapter, pOidHandle);

	if(NDIS_STATUS_PENDING == status)
	{// WDI always return success regardless of our status here, so we need to record pended req here
		PlatformAcquireSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		pAdapter->pNdisCommon->PendedRequest = pPortHelper->pCreateDeleteOID;
		PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		status = NDIS_STATUS_SUCCESS;
	}
	
	return status;
}


NDIS_STATUS
WDI_OID_TASK_DELETE_PORT(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	PORT_HELPER					*pPortHelper = pAdapter->pPortCommonInfo->pPortHelper;

	status = Wdi_Task_DeletePort(pAdapter, pOidHandle);
	
	if(NDIS_STATUS_PENDING == status)
	{// WDI always return success regardless of our status here, so we need to record pended req here
		PlatformAcquireSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		pAdapter->pNdisCommon->PendedRequest = pPortHelper->pCreateDeleteOID;
		PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		status = NDIS_STATUS_SUCCESS;
	}

	return status;
}


NDIS_STATUS
WDI_OID_TASK_START_AP(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_TASK_START_AP_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramStartAp;

	NDIS_OID_REQUEST			*req = NULL;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		// SSID list
		if(NULL == (req = Wdi_Xlat_AllocSetDesiredSsidListOid(pOidHandle, &param->DesiredSSID)))
		{
			status = NDIS_STATUS_RESOURCES;
			break;
		}

		if(NDIS_STATUS_SUCCESS != (status = N6C_OID_DOT11_DESIRED_SSID_LIST(pAdapter, req)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N6C_OID_DOT11_DESIRED_SSID_LIST returns: 0x%08X\n", status));
			break;
		}

		Wdi_Xlat_FreeOid(req);
		req = NULL;
		
		// Start AP
		if(NULL == (req = Wdi_Xlat_AllocStartApReqOid(pOidHandle, param)))
		{
			break;
		}

		N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(pAdapter, Wdi_Xlat_AuthMode(*param->AuthenticationAlgorithms.pElements));
		N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM(pAdapter, Wdi_Xlat_CipherAlgo(*param->UnicastCipherAlgorithms.pElements));
		N6CSet_DOT11_MULTICAST_CIPHER_ALGORITHM(pAdapter, Wdi_Xlat_CipherAlgo(*param->MulticastCipherAlgorithms.pElements));

		if(NDIS_STATUS_SUCCESS != (status = N63C_OID_DOT11_WFD_START_GO_REQUEST(pAdapter, req)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N63C_OID_DOT11_WFD_START_GO_REQUEST returns: 0x%08X\n", status));
			break;
		}
			
	}while(FALSE);

	// M4 indication is not required since bWaitComplete in RT_SUPPORT_TASKs is FALSE

	// cleanup
	if(req) 
		Wdi_Xlat_FreeOid(req);

	FunctionOut(COMP_OID_SET);

	return status;
}


NDIS_STATUS
WDI_OID_TASK_STOP_AP(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	NDIS_STATUS 				status = NDIS_STATUS_SUCCESS;
	
	FunctionIn(COMP_OID_SET);

	if(RT_AP_TYPE_VWIFI_AP == MgntActQuery_ApType(pAdapter))
	{
		status = N6CSet_DOT11_DISCONNECT_REQUEST(pAdapter, NULL, 0, 0, 0);
		
		if(NDIS_STATUS_SUCCESS != status)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N6CSet_DOT11_DISCONNECT_REQUEST returns: 0x%08X\n", status));
		}	
	}
	else
	{
		status = NDIS_STATUS_INVALID_STATE;
	}

	FunctionOut(COMP_OID_SET);

	return status;
}


NDIS_STATUS
WDI_OID_TASK_SEND_AP_ASSOCIATION_RESPONSE(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_TASK_SEND_AP_ASSOCIATION_RESPONSE_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramSendApAssociationResponse;

	NDIS_OID_REQUEST			*req = NULL;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		MGNT_INFO				*pMgntInfo = NULL;
		RT_WLAN_STA 			*pEntry = NULL;
		
		pMgntInfo = &pAdapter->MgntInfo;

		if(NULL == (req = Wdi_Xlat_AllocIncomingAssociationDecisionOid(pOidHandle, param)))
		{
			status = NDIS_STATUS_RESOURCES;
			break;
		}

		if(NULL == (pEntry = AsocEntry_GetEntry(pMgntInfo, param->IncomingRequestInfo.AssocRequestParams.PeerMacAddress.Address)))
		{
			status = NDIS_STATUS_INVALID_STATE;
			break;
		}
		
		if(NDIS_STATUS_SUCCESS != (status = N62C_OID_DOT11_INCOMING_ASSOCIATION_DECISION(pAdapter, req)))
		{
			RT_TRACE_F(COMP_MLME, DBG_WARNING, ("N62C_OID_DOT11_INCOMING_ASSOCIATION_DECISION returns: 0x%08X\n", status));
			break;
		}

		pOidHandle->pvCtx = req;

		AP_SendAsocRsp(
			pAdapter, 
			pEntry, 
			TRUE == param->IncomingRequestInfo.AssocRequestParams.IsReassociationRequest, 
			FALSE, 
			param->AssocResponseParameters.ReasonCode
			);

		// If succeeded, WDI_CMD_SendApAssociationResponseComplete is called from DrvIFIndicateIncommingAssociationComplete
		
	}while(FALSE);

	// cleanup
	if(NDIS_STATUS_SUCCESS != status)
	{
		if(req) 
			Wdi_Xlat_FreeOid(req);
	}

	FunctionOut(COMP_OID_SET);

	return status;

}


NDIS_STATUS
WDI_OID_TASK_CHANGE_OPERATION_MODE(	
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_TASK_CHANGE_OPERATION_MODE_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramChangeOperationMode;

	NDIS_OID_REQUEST			*req = NULL;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		if(NULL == (req = Wdi_Xlat_AllocChangeOpModeOid(pOidHandle, param)))
		{
			status = NDIS_STATUS_RESOURCES;
			break;
		}
		
		if(NDIS_STATUS_SUCCESS != (status = N6C_OID_DOT11_CURRENT_OPERATION_MODE(pAdapter, req)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N6C_OID_DOT11_CURRENT_OPERATION_MODE returns: 0x%08X\n", status));
			break;
		}

		// Indicate completed successfully
		WDI_CMD_ChangeOpModeComplete(pAdapter, req, status);
			
	}while(FALSE);

	// cleanup
	if(req && NDIS_STATUS_SUCCESS != status) 
		Wdi_Xlat_FreeOid(req);

	FunctionOut(COMP_OID_SET);

	return status;
}

NDIS_STATUS
WDI_OID_TASK_SEND_REQUEST_ACTION_FRAME(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_TASK_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramSendRequestActionFrame;
	VOID 						*req = NULL;
	
	FunctionIn(COMP_OID_SET);
	
	do
	{
		req = WDI_SendAction_Req(pAdapter, param, wdi_SendReqActionFrameStateCb);

		if(!req)
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("Send Action Req frame failed\n"));
			status = NDIS_STATUS_FAILURE;
		}
	}while(FALSE);
	
	FunctionOut(COMP_OID_SET);

	return status;
}

NDIS_STATUS
WDI_OID_TASK_SEND_RESPONSE_ACTION_FRAME(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_TASK_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramSendResponseActionFrame;
	VOID 						*req = NULL;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		req = WDI_SendAction_Rsp(pAdapter, param, wdi_SendRspActionFrameStateCb);

		if(!req)
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("Send Action Response frame failed\n"));
			status = NDIS_STATUS_FAILURE;
		}
	}while(FALSE);
	
	FunctionOut(COMP_OID_SET);

	return status;
}

NDIS_STATUS
WDI_OID_SET_P2P_LISTEN_STATE(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS 				status = NDIS_STATUS_SUCCESS;
	WDI_SET_P2P_LISTEN_STATE_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramSetP2pListenState;
	NDIS_OID_REQUEST			*req = NULL;
	
	FunctionIn(COMP_OID_SET);

	if( param == NULL )
		return NDIS_STATUS_FAILURE;

	do
	{
		P2P_INFO				*info = NULL;

		info = GET_P2P_INFO(pAdapter);

		// listen state
		if(NULL == (req = Wdi_Xlat_AllocListenStateDiscoverabilityOid(pOidHandle, &param->ListenState)))
		{
			break;
		}

		if(NDIS_STATUS_SUCCESS != (status = N63C_OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY(pAdapter, req)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N63C_OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY returns: 0x%08X\n", status));
			break;
		}

		Wdi_Xlat_FreeOid(req);
		req = NULL;

		// listen channel
		if(param->Optional.ListenChannel_IsPresent)
		{
			if(NULL == (req = Wdi_Xlat_AllocListenChannelOid(pOidHandle, &param->ListenChannel)))
			{
				break;
			}

			if(NDIS_STATUS_SUCCESS != (status = N63C_OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL(pAdapter, req)))
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N63C_OID_DOT11_WFD_DEVICE_LISTEN_CHANNEL returns: 0x%08X\n", status));
				break;
			}

			Wdi_Xlat_FreeOid(req);
			req = NULL;
		}

		// listen duration
		// TODO:
	}while(FALSE);

	if(req) 
		Wdi_Xlat_FreeOid(req);

	FunctionOut(COMP_OID_SET);

	return status;
}


NDIS_STATUS
WDI_OID_SET_P2P_ADDITIONAL_IE(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return WdiSimpleSetProperty(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_ADD_CIPHER_KEYS(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Add_Cipher_Keys(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_DELETE_CIPHER_KEYS(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Delete_Cipher_Keys(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_DEFAULT_KEY_ID(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Default_Key_ID(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_CONNECTION_QUALITY(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Connection_Quality(pAdapter, pOidHandle);
}

NDIS_STATUS
WDI_OID_GET_AUTO_POWER_SAVE(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	return Wdi_Get_Auto_Power_Save(pAdapter, pOidHandle);
}

NDIS_STATUS
WDI_OID_SET_RECEIVE_PACKET_FILTER(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Receive_Packet_Filter(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_GET_ADAPTER_CAPABILITIES(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	ndisStatus = N6SdioWdi_AdapterCapabilities(
                    pAdapter,
						pOidHandle
                    );

	return ndisStatus;
}


NDIS_STATUS
WDI_OID_SET_NETWORK_LIST_OFFLOAD(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Network_List_Offload(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_RECEIVE_COALESCING(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return WdiSimpleSetProperty(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_PRIVACY_EXEMPTION_LIST(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Privacy_Exemption_List(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_CURRENT_CHANNEL(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return WdiSimpleSetProperty(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_POWER_STATE(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Power_State(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_ABORT_TASK(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Abort_Task(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_ADD_WOL_PATTERN(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Add_Wol_Pattern(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_REMOVE_WOL_PATTERN(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Remove_Wol_Pattern(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_MULTICAST_LIST(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Multicast_List(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_WDI_SET_ADD_PM_PROTOCOL_OFFLOAD(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Add_Pm_Protocol_Offload(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_WDI_SET_REMOVE_PM_PROTOCOL_OFFLOAD(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_Set_Remove_Pm_Protocol_Offload(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_ADAPTER_CONFIGURATION(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return WdiSimpleSetProperty(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_GET_RECEIVE_COALESCING_MATCH_COUNT(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return WdiSimpleSetProperty(pAdapter, pOidHandle);
}


NDIS_STATUS
WDI_OID_SET_ADVERTISEMENT_INFORMATION(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	WDI_SET_ADVERTISEMENT_INFORMATION_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramSetAdvertisementInformation;
	NDIS_OID_REQUEST			*req = NULL;
	
	if( param == NULL )
		return NDIS_STATUS_FAILURE;

	FunctionIn(COMP_OID_SET);

	do
	{
		P2P_INFO				*info = NULL;

		info = GET_P2P_INFO(pAdapter);

		// additional ie
		if(param->Optional.AdditionalIEs_IsPresent)
		{
			if(param->AdditionalIEs.AdditionalBeaconIEs.ElementCount)
			{
				P2P_AddIe_Set(
					&info->AdditionalIEs, P2P_ADD_IE_BEACON, 
					param->AdditionalIEs.AdditionalBeaconIEs.ElementCount, 
					param->AdditionalIEs.AdditionalBeaconIEs.pElements);
				RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "WDI_OID_SET_ADVERTISEMENT_INFORMATION(): AdditionalBeaconIEs\n",
					param->AdditionalIEs.AdditionalBeaconIEs.pElements, param->AdditionalIEs.AdditionalBeaconIEs.ElementCount);
			}

			if(param->AdditionalIEs.AdditionalProbeResponseIEs.ElementCount)
			{
				P2P_AddIe_Set(
					&info->AdditionalIEs, P2P_ADD_IE_PROBE_RESPONSE, 
					param->AdditionalIEs.AdditionalProbeResponseIEs.ElementCount, 
					param->AdditionalIEs.AdditionalProbeResponseIEs.pElements);
				RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "WDI_OID_SET_ADVERTISEMENT_INFORMATION(): AdditionalProbeResponseIEs\n",
					param->AdditionalIEs.AdditionalProbeResponseIEs.pElements, param->AdditionalIEs.AdditionalProbeResponseIEs.ElementCount);

				P2P_CorrectDeviceCategory(pAdapter);
			}

			if(param->AdditionalIEs.AdditionalProbeRequestDefaultIEs.ElementCount)
			{
				P2P_AddIe_Set(
					&info->AdditionalIEs, P2P_ADD_IE_DEFAULT_REQUEST, 
					param->AdditionalIEs.AdditionalProbeRequestDefaultIEs.ElementCount, 
					param->AdditionalIEs.AdditionalProbeRequestDefaultIEs.pElements);
				RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "WDI_OID_SET_ADVERTISEMENT_INFORMATION(): AdditionalProbeRequestDefaultIEs\n",
					param->AdditionalIEs.AdditionalProbeRequestDefaultIEs.pElements, param->AdditionalIEs.AdditionalProbeRequestDefaultIEs.ElementCount);
			}
		}
		
		// device info
		if(param->Optional.DeviceInformation_IsPresent)
		{
			if(NULL == (req = Wdi_Xlat_AllocDevInfoOid(pOidHandle, &param->DeviceInformation)))
			{
				break;
			}

			if(NDIS_STATUS_SUCCESS != (status = N63C_OID_DOT11_WFD_DEVICE_INFO(pAdapter, req)))
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N63C_OID_DOT11_WFD_DEVICE_INFO returns: 0x%08X\n", status));
				break;
			}

			Wdi_Xlat_FreeOid(req);
			req = NULL;
		}
		
		// dev cap
		if(param->Optional.DeviceCapability_IsPresent)
		{
			if(NULL == (req = Wdi_Xlat_AllocDevCapOid(pOidHandle, &param->DeviceCapability)))
			{
				break;
			}

			if(NDIS_STATUS_SUCCESS != (status = N63C_OID_DOT11_WFD_DEVICE_CAPABILITY(pAdapter, req)))
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N63C_OID_DOT11_WFD_DEVICE_CAPABILITY returns: 0x%08X\n", status));
				break;
			}

			Wdi_Xlat_FreeOid(req);
			req = NULL;
		}

		// grp cap
		if(param->Optional.GroupOwnerCapability_IsPresent)
		{
			if(NULL == (req = Wdi_Xlat_AllocGrpCapOid(pOidHandle, &param->GroupOwnerCapability)))
			{
				break;
			}

			if(NDIS_STATUS_SUCCESS != (status = N63C_OID_DOT11_WFD_GROUP_OWNER_CAPABILITY(pAdapter, req)))
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N63C_OID_DOT11_WFD_GROUP_OWNER_CAPABILITY returns: 0x%08X\n", status));
				break;
			}

			Wdi_Xlat_FreeOid(req);
			req = NULL;
		}

		// sec dev type
		if(param->Optional.SecondaryDeviceTypeList_IsPresent)
		{
			if(NULL == (req = Wdi_Xlat_AllocSecDevTypeOid(pOidHandle, &param->SecondaryDeviceTypeList)))
			{
				break;
			}

			if(NDIS_STATUS_SUCCESS != (status = N63C_OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST(pAdapter, req)))
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N63C_OID_DOT11_WFD_SECONDARY_DEVICE_TYPE_LIST returns: 0x%08X\n", status));
				break;
			}

			Wdi_Xlat_FreeOid(req);
			req = NULL;
		}
		

		// TODO: adv svc
	}while(FALSE);

	if(req) 
		Wdi_Xlat_FreeOid(req);

	FunctionOut(COMP_OID_SET);

	return status;

}


NDIS_STATUS
WDI_OID_IHV_REQUEST(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	return Wdi_IHV_Request(pAdapter, pOidHandle);
}

NDIS_STATUS
WDI_OID_GET_NEXT_ACTION_FRAME_DIALOG_TOKEN(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS 				status = NDIS_STATUS_SUCCESS;
	MP_PORT_TYPE				portType;
	WDI_GET_NEXT_ACTION_FRAME_DIALOG_TOKEN_PARAMETERS param;
	PNDIS_OID_REQUEST  pNdisRequest = pOidHandle->pNdisRequest;
	NDIS_OID_REQUEST			*req = NULL;

	ULONG						tlvlen = 0;
	UINT8						*tlv = NULL;
	
	FunctionIn(COMP_OID_SET);

	do
	{
		if( pAdapter->pNdis62Common != NULL )
		{
			portType = pAdapter->pNdis62Common->PortType;
		}
		else
		{
			status = NDIS_STATUS_ADAPTER_NOT_READY;
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N63C_OID_DOT11_WFD_GET_DIALOG_TOKEN returns: 0x%08X\n", status));
			break;
		}

		if( portType != EXT_P2P_DEVICE_PORT && portType != EXT_P2P_ROLE_PORT )
		{
			param.NextDialogToken = IncreaseDialogToken(pAdapter->pNdisCommon->DialogToken);
		}
		else
		{
			P2P_INFO				*info = NULL;

			info = GET_P2P_INFO(pAdapter);

			// listen state
			if(NULL == (req = Wdi_Xlat_AllocGetDialogTokenOid(pOidHandle, &param)))
			{
				break;
			}

			if(NDIS_STATUS_SUCCESS != (status = N63C_OID_DOT11_WFD_GET_DIALOG_TOKEN(pAdapter, req)))
			{
				RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N63C_OID_DOT11_WFD_GET_DIALOG_TOKEN returns: 0x%08X\n", status));
				break;
			}

			param.NextDialogToken = *(UINT8 *)req->DATA.QUERY_INFORMATION.InformationBuffer;
		}

		GenerateWdiGetNextActionFrameDialogToken(
			&param, 0, 
			&pAdapter->pPortCommonInfo->WdiData.TlvContext,
			&tlvlen, &tlv);

		if(pNdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength < sizeof(WDI_MESSAGE_HEADER) + tlvlen)
		{
			pOidHandle->pNdisRequest->DATA.METHOD_INFORMATION.BytesNeeded += (tlvlen + sizeof(WDI_MESSAGE_HEADER));
			status = NDIS_STATUS_BUFFER_TOO_SHORT;
			break;
		}

		PlatformMoveMemory(
			(UCHAR *)pNdisRequest->DATA.METHOD_INFORMATION.InformationBuffer + sizeof(WDI_MESSAGE_HEADER),
			tlv, tlvlen);

		pNdisRequest->DATA.METHOD_INFORMATION.BytesWritten += tlvlen;
		
	}while(FALSE);

	// cleanup;
	if(req) 
		Wdi_Xlat_FreeOid(req);

	if(tlv)
		FreeGenerated(tlv);

	FunctionOut(COMP_OID_SET);

	return status;

}

NDIS_STATUS
WDI_OID_SET_P2P_WPS_ENABLED(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{

	NDIS_STATUS 				status = NDIS_STATUS_SUCCESS;
	WDI_SET_P2P_WPS_ENABLED_PARAMETERS 	*param = pOidHandle->tlvParser.parsedTlv.paramSetP2pWpsEnabled;
	NDIS_OID_REQUEST			*req = NULL;
	
	if( param == NULL )
		return NDIS_STATUS_FAILURE;

	do
	{
		if(NULL == (req = Wdi_Xlat_AllocWpsEnabledOid(pOidHandle, param)))
		{
			break;
		}

		if(NDIS_STATUS_SUCCESS != (status = N62C_OID_DOT11_WPS_ENABLED(pAdapter, req)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("N62C_OID_DOT11_WPS_ENABLED returns: 0x%08X\n", status));
			break;
		}
		
	}while(FALSE);

	if(req) 
		Wdi_Xlat_FreeOid(req);

	FunctionOut(COMP_OID_SET);

	return status;

}

NDIS_STATUS
WDI_OID_GET_BSS_ENTRY_LIST(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	NDIS_STATUS 				status = NDIS_STATUS_SUCCESS;
	WDI_GET_BSS_ENTRY_LIST_UPDATE_PARAMETERS *param = pOidHandle->tlvParser.parsedTlv.paramGetBSSEntryListUpdate;
	NDIS_OID_REQUEST			*req = NULL;

	if( param == NULL )
		return NDIS_STATUS_FAILURE;

	WDI_IndicateBssListBySSID(pAdapter, &(param->SSID));

	return status;
}

NDIS_STATUS
WDI_OID_SET_FAST_BSS_TRANSITION_PARAMETERS(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER			pOidHandle
	)
{
	RT_STATUS					rtStatus = RT_STATUS_SUCCESS;
	NDIS_STATUS 				ndisStatus = NDIS_STATUS_SUCCESS;
	WDI_SET_FAST_BSS_TRANSITION_PARAMETERS_COMMAND 	*param = pOidHandle->tlvParser.parsedTlv.paramSetFastBssTransitionParametersCommand;
	u1Byte						targetAddr[6] = {0};
	
	if( param == NULL )
		return NDIS_STATUS_FAILURE;



	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = FtGetWaitOSDecisionAddr(pAdapter, targetAddr)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("Fail (0x%08X) to get addr from FtGetWaitOSDecisionAddr()\n", rtStatus));
			break;
		}

		RT_PRINT_ADDR(COMP_OID_SET, DBG_LOUD, "Addr = ", targetAddr);

		if(param->Optional.RSNIE_IsPresent)
		{
			FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_RSNE, targetAddr,
				param->RSNIE.pElements, param->RSNIE.ElementCount);
			RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "RSNIE:\n",
				param->RSNIE.pElements, param->RSNIE.ElementCount);
		}
			
		if(param->Optional.MDE_IsPresent)
		{
			FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_MDE, targetAddr,
				param->MDE.pElements, param->MDE.ElementCount);
			RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "MDE:\n",
				param->MDE.pElements, param->MDE.ElementCount);
		}

		if(param->Optional.FTE_IsPresent)
		{
			FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_FTE, targetAddr,
				param->FTE.pElements, param->FTE.ElementCount);
			RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FTE:\n",
				param->FTE.pElements, param->FTE.ElementCount);
		}

		FtUpdateEntryInfo(pAdapter, FT_ENTRY_ACTION_UPDATE_ASSOC_INFO_STATUS, targetAddr,
				&(param->status), sizeof(WDI_STATUS_CONTAINER));
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("status = 0x%08X\n", param->status));

		FtUpdateEntryInfo(pAdapter, FT_OS_DECISION_MADE, targetAddr, NULL, 0);

		MlmeAuthenticateRequest_Confirm(pAdapter, (NDIS_STATUS_SUCCESS == param->status) ? MlmeStatus_success : MlmeStatus_invalid);
	}while(FALSE);


	FunctionOut(COMP_OID_SET);

	return ndisStatus;

}

//-----------------------------------------------------------------------------
// Completion routine
//-----------------------------------------------------------------------------

//
// Description:
//		Prepare indication data for WDI create port.
//		Also add data path (via AddDataPathPeer()) as 
//		it was copied from the original code in 
//		WdiSimpleTask().
//
// Note:
//		pPortHelper->pCreateDot11MacInfo points to the
//		OID info buffer and is filled in N62C_OID_DOT11_CREATE_MAC
//		and N62CCreateMac.
//
BOOLEAN
WDI_CMD_CreatePortComplete(
	IN  PADAPTER				pAdapter,
	IN  NDIS_OID_REQUEST		*req,
	IN  NDIS_STATUS				status
	)
{
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	DOT11_MAC_INFO				*macInfo = NULL;
	
	UINT8						*tlv = NULL;
	ULONG 						len = 0;

	WDI_INDICATION_CREATE_PORT_COMPLETE_PARAMETERS param = {0};
	
	macInfo = (DOT11_MAC_INFO *)req->DATA.METHOD_INFORMATION.InformationBuffer;

	FunctionIn(COMP_OID_SET);

	// Win8: Let the device port use the locally-adminitered MAC address
	if(WDI_OPERATION_MODE_P2P_DEVICE == hTask->tlvParser.parsedTlv.paramCreatePort->CreatePortParameters.OpModeMask)
	{
		ADAPTER *pNewAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)macInfo->uNdisPortNumber);

		macInfo->MacAddr[0] |= BIT1;
		cpMacAddr(pNewAdapter->CurrentAddress, macInfo->MacAddr);
	}

	// prepare completion param
	cpMacAddr(param.PortAttributes.MacAddress.Address, macInfo->MacAddr);
	param.PortAttributes.PortNumber = (UINT16)macInfo->uNdisPortNumber;

	// generate TLV
	GenerateWdiIndicationCreatePortComplete(&param, 0, &pAdapter->pPortCommonInfo->WdiData.TlvContext, &len, &tlv);

	// copy tlv
	wdi_task_WriteResult(hTask, tlv, len, status);

	// cleanup
	FreeGenerated(tlv);

	// Free the OID
	Wdi_Xlat_FreeOid(req);

	// Trigger M4 indication
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	// Free pended oid
	PlatformAcquireSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
	pAdapter->pNdisCommon->PendedRequest = NULL;
	PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
	
	FunctionOut(COMP_OID_SET);

	return TRUE;
}

BOOLEAN
WDI_CMD_DeletePortComplete(
	IN  PADAPTER				pAdapter,
	IN  NDIS_OID_REQUEST		*req,
	IN  NDIS_STATUS				status
	)
{
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	
	UINT8						*tlv = NULL;
	ULONG 						len = 0;

	WDI_INDICATION_DELETE_PORT_COMPLETE_PARAMETERS param = {0};

	FunctionIn(COMP_OID_SET);

	// prepare structure
	param._Reserved = 0;

	// generate TLV
	//GenerateWdiIndicationDeletePortComplete(&param, 0, &pAdapter->pPortCommonInfo->WdiData.TlvContext, &len, &tlv);

	// copy tlv
	wdi_task_WriteResult(hTask, tlv, len, status);

	// cleanup
	Wdi_Xlat_FreeOid(req);
	//FreeGenerated(tlv);

	// Trigger M4 indication
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	// Free pended oid
	PlatformAcquireSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
	pAdapter->pNdisCommon->PendedRequest = NULL;
	PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);	

	FunctionOut(COMP_OID_SET);

	return TRUE;
}

BOOLEAN
WDI_CMD_ResetComplete(
	IN  PADAPTER				pAdapter,
	IN  NDIS_OID_REQUEST		*req,
	IN  NDIS_STATUS				status
	)
{
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	
	UINT8						*tlv = NULL;
	ULONG 						len = 0;

	WDI_INDICATION_DOT11_RESET_COMPLETE_PARAMETERS param = {0};

	FunctionIn(COMP_OID_SET);

	// prepare structure
	param._Reserved = 0;

	// generate TLV
	//GenerateWdiIndicationDot11ResetComplete(&param, 0, &pAdapter->pPortCommonInfo->WdiData.TlvContext, &len, &tlv);
	
	// copy tlv
	wdi_task_WriteResult(hTask, tlv, len, status);

	// cleanup
	Wdi_Xlat_FreeOid(req);
	//FreeGenerated(tlv);

	// Trigger M4 indication
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	FunctionOut(COMP_OID_SET);

	return TRUE;
}

BOOLEAN
WDI_CMD_ChangeOpModeComplete(
	IN  PADAPTER				pAdapter,
	IN  NDIS_OID_REQUEST		*req,
	IN  NDIS_STATUS				status
	)
{
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	
	UINT8						*tlv = NULL;
	ULONG 						len = 0;

	WDI_INDICATION_CHANGE_OPERATION_MODE_COMPLETE_PARAMETERS param = {0};

	FunctionIn(COMP_OID_SET);

	// prepare structure
	param._Reserved = 0;

	// generate TLV
	//GenerateWdiIndicationChangeOperationModeComplete(&param, 0, &pAdapter->pPortCommonInfo->WdiData.TlvContext, &len, &tlv);

	// copy tlv
	wdi_task_WriteResult(hTask, tlv, len, status);

	// cleanup
	Wdi_Xlat_FreeOid(req);
	//FreeGenerated(tlv);

	// Trigger M4 indication
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	FunctionOut(COMP_OID_SET);

	return TRUE;
}

BOOLEAN
WDI_CMD_P2PDiscoverComplete(
	IN  PADAPTER				pAdapter,
	IN  NDIS_STATUS				status
	)
{
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	
	UINT8						*tlv = NULL;
	ULONG 						len = 0;

	WDI_INDICATION_DOT11_RESET_COMPLETE_PARAMETERS param = {0};

	FunctionIn(COMP_OID_SET);

	RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("complete via port: %u\n", pAdapter->pNdis62Common->PortNumber));

	if(FALSE == OidHandle_VerifyTask(pAdapter, OID_WDI_TASK_P2P_DISCOVER))
		return FALSE;

	// prepare structure
	param._Reserved = 0;

	// generate TLV
	//GenerateWdiIndicationDot11ResetComplete(&param, 0, &pAdapter->pPortCommonInfo->WdiData.TlvContext, &len, &tlv);
	
	// copy tlv
	wdi_task_WriteResult(hTask, tlv, len, status);

	// cleanup
	//FreeGenerated(tlv);

	// Trigger M4 indication
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	FunctionOut(COMP_OID_SET);

	return TRUE;
}

BOOLEAN
WDI_CMD_P2pSendRequestActionFrameComplete(
	IN  ADAPTER					*pAdapter,
	IN  const WDI_SEND_ACTION_CTX *ctx
	)
{
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	OFF_CHNL_TX_STATUS_FLAG		txStatusFlag;
	
	u4Byte						ieOffset = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	
	UINT8						*tlv = NULL;
	ULONG 						len = 0;

	// N63CIndicateInvitationRequestSendComplete

	WDI_INDICATION_P2P_SEND_REQUEST_ACTION_FRAME_COMPLETE_PARAMETERS param = {0};

	txStatusFlag = OffChnlTx_GetTxStatusFlag(ctx->req);

	status = (TEST_FLAG(txStatusFlag.perTxAttemptFlag, OFF_CHNL_TX_PER_TX_ATTEMPT_FLAG_FRAME_SENT) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);

	FunctionIn(COMP_OID_SET);

	if(NDIS_STATUS_SUCCESS == status)
	{
		FRAME_BUF				*buf = OffChnlTx_GetTxFrameBuf(ctx->req);

		RT_ASSERT(buf, ("%s(): buf is NULL!!!n", __FUNCTION__));
		
		cpMacAddr(param.SendActionFrameResult.FrameParameters.PeerDeviceAddress.Address, (FrameBuf_Head(buf) + FRAME_OFFSET_ADDRESS1));
		param.SendActionFrameResult.FrameParameters.DialogToken = ctx->token;
		param.SendActionFrameResult.FrameIEs.ElementCount = FrameBuf_Length(buf) - ieOffset;
		param.SendActionFrameResult.FrameIEs.pElements = FrameBuf_MHead(buf) + ieOffset;

		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("token: %u\n", param.SendActionFrameResult.FrameParameters.DialogToken));
	}
	
	// generate TLV
	GenerateWdiIndicationP2pSendRequestActionFrameComplete(
		&param, 0, 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&len, &tlv);
	
	// copy tlv
	wdi_task_WriteResult(hTask, tlv, len, status);

	// cleanup
	FreeGenerated(tlv);

	// Trigger M4 indication
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	FunctionOut(COMP_OID_SET);

	return TRUE;
}

BOOLEAN
WDI_CMD_P2pSendResponseActionFrameComplete(
	IN  PADAPTER				pAdapter,
	IN  const WDI_SEND_ACTION_CTX *ctx
	)
{
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	OFF_CHNL_TX_STATUS_FLAG		txStatusFlag;
	
	u4Byte						ieOffset = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	
	UINT8						*tlv = NULL;
	ULONG 						len = 0;

	WDI_INDICATION_P2P_SEND_RESPONSE_ACTION_FRAME_COMPLETE_PARAMETERS param = {0};

	txStatusFlag = OffChnlTx_GetTxStatusFlag(ctx->req);

	status = (TEST_FLAG(txStatusFlag.perTxAttemptFlag, OFF_CHNL_TX_PER_TX_ATTEMPT_FLAG_FRAME_SENT) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);

	FunctionIn(COMP_OID_SET);
	
	if(NDIS_STATUS_SUCCESS == status)
	{
		FRAME_BUF				*buf = OffChnlTx_GetTxFrameBuf(ctx->req);

		RT_ASSERT(buf, ("%s(): buf is NULL!!!n", __FUNCTION__));
		
		cpMacAddr(param.SendActionFrameResult.FrameParameters.PeerDeviceAddress.Address, (FrameBuf_Head(buf) + FRAME_OFFSET_ADDRESS1));
		param.SendActionFrameResult.FrameParameters.DialogToken = ctx->token;
		param.SendActionFrameResult.FrameIEs.ElementCount = FrameBuf_Length(buf) - ieOffset;
		param.SendActionFrameResult.FrameIEs.pElements = FrameBuf_MHead(buf) + ieOffset;

		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("token: %u\n", param.SendActionFrameResult.FrameParameters.DialogToken));
	}
	
	// generate TLV
	GenerateWdiIndicationP2pSendResponseActionFrameComplete(
		&param, 0, 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&len, &tlv);
	
	// copy tlv
	wdi_task_WriteResult(hTask, tlv, len, status);

	// cleanup
	FreeGenerated(tlv);

	// Trigger M4 indication
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	FunctionOut(COMP_OID_SET);

	return TRUE;
}

BOOLEAN
WDI_CMD_SendReqActionFrameComplete(
	IN  PADAPTER				pAdapter,
	IN  const WDI_SEND_ACTION_CTX *ctx
	)
{	
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	OFF_CHNL_TX_STATUS_FLAG		txStatusFlag = {0};
	
	UINT8						*tlv = NULL;
	ULONG 						len = 0;

	WDI_INDICATION_SEND_REQUEST_ACTION_FRAME_COMPLETE_PARAMETERS param = {0};

	txStatusFlag = OffChnlTx_GetTxStatusFlag(ctx->req);

	// TODO: update status in wdi header
	status = (TEST_FLAG(txStatusFlag.perTxAttemptFlag, OFF_CHNL_TX_PER_TX_ATTEMPT_FLAG_FRAME_SENT) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);

	if(NDIS_STATUS_SUCCESS == status)
	{
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("Action Req frame send failure.\n"));
	}
	
#if 0
	//GenerateWdiIndicationSendRequestActionFrameComplete(
	//	&param, 0, 
	//	&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
	//	&len, &tlv);
	
	wdi_task_WriteResult(hTask, tlv, len, status);

	FreeGenerated(tlv);
#endif
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	return TRUE;
}

BOOLEAN
WDI_CMD_SendRspActionFrameComplete(
	IN  PADAPTER				pAdapter,
	IN  const WDI_SEND_ACTION_CTX *ctx
	)
{	
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	RT_OID_HANDLER				*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	OFF_CHNL_TX_STATUS_FLAG		txStatusFlag = {0};
		
	UINT8						*tlv = NULL;
	ULONG 						len = 0;

	WDI_INDICATION_SEND_RESPONSE_ACTION_FRAME_COMPLETE_PARAMETERS param = {0};

	txStatusFlag = OffChnlTx_GetTxStatusFlag(ctx->req);

	// TODO: update status in wdi header
	status = (TEST_FLAG(txStatusFlag.perTxAttemptFlag, OFF_CHNL_TX_PER_TX_ATTEMPT_FLAG_FRAME_SENT) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);

	if(NDIS_STATUS_SUCCESS != status)
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("Action Rsp frame send failure.\n"));
	}
	
#if 0
	GenerateWdiIndicationSendRequestActionFrameComplete(
		&param, 0, 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&len, &tlv);
	
	wdi_task_WriteResult(hTask, tlv, len, status);

	FreeGenerated(tlv);
#endif
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	return TRUE;
}

BOOLEAN
WDI_CMD_SendApAssociationResponseComplete(
	IN  PADAPTER				pAdapter,
	IN  NDIS_OID_REQUEST		*req,
	IN  RT_STATUS				status
	)
{
	RT_OID_HANDLER	*hTask = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	RT_WLAN_STA	*sta = pAdapter->MgntInfo.pCurrentSta;
	UINT8			*tlv = NULL;
	ULONG			len = 0;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_FAILURE;

	WDI_INDICATION_SEND_AP_ASSOCIATION_RESPONSE_COMPLETE_PARAMETERS param = {0};

	FunctionIn(COMP_MLME);

	// prepare structure, ref N62CAPIndicateIncomAssocComplete
	param.Optional.ActivePhyList_IsPresent = FALSE;
	cpMacAddr(param.AssocResponseResult.PeerMACAddress.Address, sta->MacAddr);
	param.AssocResponseResult.IsReassociationRequest = 
		Type_Reasoc_Rsp == GET_80211_HDR_TYPE(sta->AP_SendAsocResp) ? TRUE : FALSE;
	param.AssocResponseResult.IsReAssociationResponse = 
		param.AssocResponseResult.IsReassociationRequest;
	if(RT_802_11AuthModeOpen == sta->AuthMode)
		param.AssocResponseResult.AuthAlgorithm = WDI_AUTH_ALGO_80211_OPEN;
	else if(RT_802_11AuthModeShared == sta->AuthMode)
		param.AssocResponseResult.AuthAlgorithm = WDI_AUTH_ALGO_80211_SHARED_KEY;
	else if(RT_802_11AuthModeAutoSwitch == sta->AuthMode)
		param.AssocResponseResult.AuthAlgorithm = WDI_AUTH_ALGO_80211_OPEN;
	else if(RT_802_11AuthModeWPA == sta->AuthMode)
		param.AssocResponseResult.AuthAlgorithm = WDI_AUTH_ALGO_WPA_NONE;
	else if(RT_802_11AuthModeWPAPSK == sta->AuthMode)
		param.AssocResponseResult.AuthAlgorithm = WDI_AUTH_ALGO_WPA_PSK;
	else if(RT_802_11AuthModeWPANone == sta->AuthMode)
		param.AssocResponseResult.AuthAlgorithm = WDI_AUTH_ALGO_WPA_NONE;
	else if(RT_802_11AuthModeWPA2 == sta->AuthMode)
		param.AssocResponseResult.AuthAlgorithm = WDI_AUTH_ALGO_WPA_NONE;
	else if(RT_802_11AuthModeWPA2PSK == sta->AuthMode)
		param.AssocResponseResult.AuthAlgorithm = WDI_AUTH_ALGO_WPA_PSK;
	else 
		param.AssocResponseResult.AuthAlgorithm = WDI_AUTH_ALGO_80211_OPEN;

	if(RT_ENC_ALG_NO_CIPHER == sta->perSTAKeyInfo.PairwiseCipherSuite[0])
		param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_NONE;
	else if(RT_ENC_ALG_WEP40 == sta->perSTAKeyInfo.PairwiseCipherSuite[0])
		param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_WEP40;
	else if(RT_ENC_ALG_TKIP == sta->perSTAKeyInfo.PairwiseCipherSuite[0])
			param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_TKIP;
	else if(RT_ENC_ALG_AESCCMP == sta->perSTAKeyInfo.PairwiseCipherSuite[0])
			param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_CCMP;
	else if(RT_ENC_ALG_WEP104 == sta->perSTAKeyInfo.PairwiseCipherSuite[0])
			param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_WEP104;
	else if(RT_ENC_ALG_WEP == sta->perSTAKeyInfo.PairwiseCipherSuite[0])
			param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_WEP;
	else
		param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_NONE;

	if(RT_ENC_ALG_NO_CIPHER == sta->perSTAKeyInfo.GroupCipherSuite)
		param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_NONE;
	else if(RT_ENC_ALG_WEP40 == sta->perSTAKeyInfo.GroupCipherSuite)
		param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_WEP40;
	else if(RT_ENC_ALG_TKIP == sta->perSTAKeyInfo.GroupCipherSuite)
			param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_TKIP;
	else if(RT_ENC_ALG_AESCCMP == sta->perSTAKeyInfo.GroupCipherSuite)
			param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_CCMP;
	else if(RT_ENC_ALG_WEP104 == sta->perSTAKeyInfo.GroupCipherSuite)
			param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_WEP104;
	else if(RT_ENC_ALG_WEP == sta->perSTAKeyInfo.GroupCipherSuite)
			param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_WEP;
	else
		param.AssocResponseResult.UnicastCipherAlgorithm = WDI_CIPHER_ALGO_NONE;

	param.AssocResponseFrame.ElementCount = MMPDU_BODY_LEN(sta->AP_SendAsocRespLength);
	param.AssocResponseFrame.pElements = MMPDU_BODY(sta->AP_SendAsocResp);	

	if(pAdapter->MgntInfo.beaconframe.Length)
	{
		param.BeaconIEs.ElementCount = MMPDU_BODY_LEN(pAdapter->MgntInfo.beaconframe.Length);
		param.BeaconIEs.pElements = MMPDU_BODY(pAdapter->MgntInfo.beaconframe.Octet);
	}
	else
	{
		param.BeaconIEs.ElementCount = 0;
		param.BeaconIEs.pElements = NULL;
	}
	
	// generate TLV
	GenerateWdiIndicationSendApAssociationResponseComplete(
		&param, 0, 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&len, &tlv);
	
	// copy tlv
	wdi_task_WriteResult(hTask, tlv, len, status);

	// cleanup
	FreeGenerated(tlv);

	ndisStatus = WDI_AddDatapathPeer(pAdapter, sta->MacAddr);
	
	// Free the OID
	Wdi_Xlat_FreeOid(req);

	// Trigger M4 indication
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);

	if( ndisStatus != NDIS_STATUS_SUCCESS )
	{		
		RT_TRACE(COMP_MLME, DBG_WARNING, ("WDI_CMD_SendApAssociationResponseComplete: fail to create peer, so disconnect with this AP[%2x:%2x:%2x:%2x:%2x:%2x]\n", sta->MacAddr[0], sta->MacAddr[1], sta->MacAddr[2], sta->MacAddr[3], sta->MacAddr[4], sta->MacAddr[5]));

		// 1. Send disassoc frame to the STA.
		SendDisassociation(pAdapter, sta->MacAddr, class3_err );

		// 3. Update its timestamp.
		AsocEntry_UpdateTimeStamp(sta);

		// 4. Mark the STA as disassoicated and related.
		AsocEntry_BecomeDisassoc(pAdapter, sta);

		// 5. To initialize WPA-PSK key mgnt state machine. Added by Annie, 2005-07-15.
		if( pAdapter->MgntInfo.SecurityInfo.AuthMode == RT_802_11AuthModeWPA )
		{
			Authenticator_StateDISCONNECTED(pAdapter, sta);
		}
	}

	FunctionOut(COMP_MLME);

	return TRUE;
}

VOID
WDI_CMD_PreIndicateRxP2pActionFrameCb(
	IN  VOID 					*pvP2PInfo,
	IN  u4Byte 					EventID,
	IN  MEMORY_BUFFER			*pInformation
	)
{
	P2P_INFO					*pP2PInfo = (P2P_INFO *)pvP2PInfo;
	ADAPTER						*pAdapter = pP2PInfo->pAdapter;

	FunctionIn(COMP_OID_SET);
	
	if(P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE == EventID
		|| P2P_EVENT_RECEIVED_INVITATION_RESPONSE == EventID
		|| P2P_EVENT_RECEIVED_GO_NEGOTIATION_RESPONSE == EventID
		|| P2P_EVENT_RECEIVED_GO_NEGOTIATION_CONFIRM == EventID
		)
	{
		WDI_SendAction_TerminateReq(pAdapter);
	}
	
	FunctionOut(COMP_OID_SET);

	return;
}

VOID
WDI_CMD_PostIndicateRxP2pActionFrameCb(
	IN  VOID 					*pvP2PInfo,
	IN  u4Byte 					EventID,
	IN  MEMORY_BUFFER			*pInformation
	)
{
	return;
}

NDIS_STATUS
WDI_OID_GET_STATISTICS(
	IN  PADAPTER			pAdapter,
	IN  PRT_OID_HANDLER	pOidHandle
	)
{
	return Wdi_Get_Statistics(pAdapter, pOidHandle);
}

NDIS_STATUS
WDI_CANCEL_OID_TASK_SCAN(
	IN  PADAPTER			pAdapter
	)
{
	CustomScan_TermReq(GET_CUSTOM_SCAN_INFO(pAdapter), FALSE);
	wdi_task_WriteResult(&pAdapter->pPortCommonInfo->WdiData.TaskHandle, NULL, 0, NDIS_STATUS_REQUEST_ABORTED);

	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
WDI_CANCEL_OID_TASK_P2P_DISCOVER(
	IN  PADAPTER			pAdapter
	)
{
	CustomScan_TermReq(GET_CUSTOM_SCAN_INFO(pAdapter), FALSE);

	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
WDI_CANCEL_OID_TASK_CONNECT(
	IN  PADAPTER			pAdapter
	)
{
	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
WDI_CANCEL_OID_TASK_P2P_SEND_REQUEST_ACTION_FRAME(
	IN  PADAPTER			pAdapter
	)
{
	RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("cancel sending p2p request action frame!!!\n"));

	return WDI_SendAction_TerminateReq(pAdapter);
}


NDIS_STATUS
WDI_CANCEL_OID_TASK_P2P_SEND_RESPONSE_ACTION_FRAME(
	IN  PADAPTER			pAdapter
	)
{
	RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("[TODO]: cancel sending p2p response action frame!!!\n"));
	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
WDI_CANCEL_OID_TASK_START_AP(
	IN  PADAPTER			pAdapter
	)
{
	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
WDI_CANCEL_OID_TASK_SEND_AP_ASSOCIATION_RESPONSE(
	IN  PADAPTER			pAdapter
	)
{
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
WDI_CANCEL_OID_TASK_SEND_REQUEST_ACTION_FRAME(
	IN  PADAPTER			pAdapter
	)
{
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
WDI_CANCEL_OID_TASK_SEND_RESPONSE_ACTION_FRAME(
	IN  PADAPTER			pAdapter
	)
{
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS 
WDI_PRE_M4_OID_TASK_SCAN(
	IN  PADAPTER 			pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	)
{
	WDI_IndicateBssList(pAdapter, RT_STATUS_SUCCESS);
	return NDIS_STATUS_SUCCESS;
}

