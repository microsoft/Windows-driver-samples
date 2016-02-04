////////////////////////////////////////////////////////////////////////////////
//
//	File name:		WDI_SendAction.c
//	Description:	
//
//	Author:			haich
//
////////////////////////////////////////////////////////////////////////////////
#include "Mp_Precomp.h"
#include "WDI_SendAction.h"

#if WPP_SOFTWARE_TRACE
#include "WDI_SendAction.tmh"
#endif

#include "P2P_Internal.h"

#define WDI_SEND_ACTION_POST_ACK_DWELL_TIME_MS			300
#define WDI_SEND_ACTION_RETRY_COUNT						15
#define WDI_SEND_ACTION_RETRY_INTERMITTENT_TIME_MS		300
#define WDI_SEND_ACTION_RETRY_INTERMITTENT_TIME_HCT_MS	0

//-----------------------------------------------------------------------------
// Local: helper
//-----------------------------------------------------------------------------

static
P2P_FRAME_TYPE
wdi_SendAction_XlatFrameType(
	IN  WDI_P2P_ACTION_FRAME_TYPE wdiFType
	)
{
	if(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST == wdiFType)
		return P2P_FID_GO_NEG_REQ;

	if(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE == wdiFType)
		return P2P_FID_GO_NEG_RSP;

	if(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM == wdiFType)
		return P2P_FID_GO_NEG_CONF;

	if(WDI_P2P_ACTION_FRAME_INVITATION_REQUEST == wdiFType)
		return P2P_FID_INV_REQ;

	if(WDI_P2P_ACTION_FRAME_INVITATION_RESPONSE == wdiFType)
		return P2P_FID_INV_RSP;

	if(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST == wdiFType)
		return P2P_FID_PD_REQ;

	if(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == wdiFType)
		return P2P_FID_PD_RSP;

	return P2P_FID_MAX;
}

static
VOID
wdi_SendAction_InitCtx(
	IN  WDI_SEND_ACTION_CTX		*ctx,
	IN  WDI_P2P_ACTION_FRAME_TYPE reqType,
	IN  WDI_P2P_ACTION_FRAME_TYPE rspType,
	IN  ADAPTER					*pAdapter,
	IN  const u1Byte			*da,
	IN  BOOLEAN					bGo,
	IN  u1Byte					token,
	IN  u1Byte					oldToken,
	IN  ULONG					timeout,
	IN  VOID					*offChnlTxReq,
	IN  u1Byte					compEvent
	)
{
	ctx->reqType = reqType;
	ctx->rspType = rspType;
	ctx->pAdapter = pAdapter;
	cpMacAddr(ctx->da, da);
	ctx->bGo = bGo;
	ctx->token = token;
	ctx->oldToken = oldToken;
	ctx->timeout = timeout;
	ctx->req = offChnlTxReq;
	ctx->compEvent = compEvent;
}

static
VOID *
wdi_SendAction_PrepareReq(
	IN  ADAPTER					*pAdapter,
	IN  const u1Byte			*txMac,
	IN  u1Byte					chnl,
	IN  u4Byte					timeout
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	FRAME_BUF					*probe = NULL;

	// off chnl tx req
	if(NULL == (req = OffChnlTx_AllocReq(pAdapter, txMac, NULL, NULL, sizeof(*ctx))))
	{
		return NULL;
	}

	// construct probe req
	probe = OffChnlTx_GetProbeReqBuf(req);
	p2p_Construct_ProbeReq(probe, info);

	// data rate for probe and tx frame
	OffChnlTx_SetDataRate(req, MGN_6M);

	// prepare custom scan req
	OffChnlTx_SetTimeBound(req, timeout);
	OffChnlTx_SetPostAckDwellTime(req, WDI_SEND_ACTION_POST_ACK_DWELL_TIME_MS);
	OffChnlTx_SetProbeEnabled(req, TRUE);
	OffChnlTx_SetTxChnl(req, chnl);
	OffChnlTx_SetTxAttemptCount(req, WDI_SEND_ACTION_RETRY_COUNT);
	OffChnlTx_SetRetryIntermittentTime(req, 
		pAdapter->bInHctTest 
			? WDI_SEND_ACTION_RETRY_INTERMITTENT_TIME_HCT_MS 
			: WDI_SEND_ACTION_RETRY_INTERMITTENT_TIME_MS
		);
	
	return req;
}

static VOID
add_MgntFrameMacHdr(
	IN  FRAME_BUF				*pBuf,
	IN  TYPE_SUBTYPE			typeSubtype,
	IN  const u1Byte			*dest,
	IN  const u1Byte			*src,
	IN  const u1Byte			*bssid
	)
{
	pu1Byte						buf = NULL;
	
	if(NULL == (buf = FrameBuf_Add(pBuf, 24))) return;
	
	SET_80211_HDR_FRAME_CONTROL(buf, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(buf, typeSubtype);
	SET_80211_HDR_DURATION(buf, 0);
	
	SET_80211_HDR_FRAGMENT_SEQUENCE(buf, 0);
	SET_80211_HDR_ADDRESS1(buf, dest);
	SET_80211_HDR_ADDRESS2(buf, src);
	SET_80211_HDR_ADDRESS3(buf, bssid);
	SET_80211_HDR_FRAGMENT_SEQUENCE(buf, 0);

	return;
}

static
VOID *
wdi_SendAction_PrepareReqEx(
	IN  ADAPTER					*pAdapter,
	IN  const u1Byte			*txMac,
	IN  u1Byte					chnl,
	IN  u4Byte					timeout,
	IN  UINT32					PostACKDwellTime
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	FRAME_BUF					*probe = NULL;

	DbgPrintEx(0, 0, "wdi_SendAction_PrepareReqEx Timeout:%d PostAckDwellTime:%d Chan:%d TxAttemptCount:%d RetryintermittentTime:%d\n",
		timeout,
		WDI_SEND_ACTION_POST_ACK_DWELL_TIME_MS,
		chnl,
		WDI_SEND_ACTION_RETRY_COUNT,
		WDI_SEND_ACTION_RETRY_INTERMITTENT_TIME_MS);

	// off chnl tx req
	if(NULL == (req = OffChnlTx_AllocReq(pAdapter, txMac, NULL, NULL, sizeof(*ctx))))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("wdi_SendAction_PrepareReqEx failed to off channel req\n"));
		return NULL;
	}

	// TODO: double check data rate here
	OffChnlTx_SetDataRate(req, MGN_6M);

	// TODO: double check here
	OffChnlTx_SetTimeBound(req, timeout);
	OffChnlTx_SetPostAckDwellTime(req, PostACKDwellTime);
	OffChnlTx_SetProbeEnabled(req, TRUE);
	OffChnlTx_SetTxChnl(req, chnl);
	OffChnlTx_SetTxAttemptCount(req, WDI_SEND_ACTION_RETRY_COUNT);
	OffChnlTx_SetRetryIntermittentTime(req, WDI_SEND_ACTION_RETRY_INTERMITTENT_TIME_MS);
	
	return req;
}

static
RT_STATUS
wdi_SendAction_IssueReq(
	IN  ADAPTER					*pAdapter,
	IN  VOID					*req,
	IN  const char				*typeInfo
	)
{
	return OffChnlTx_IssueReq(pAdapter, req, typeInfo);
}

static
u1Byte
wdi_SendAction_GetTxChnl(
	IN  P2P_INFO				*info,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE 			dtype,
	IN  P2P_FRAME_TYPE			ftype
	)
{
	const P2P_DEV_LIST_ENTRY	*pDev = NULL;
	u1Byte						txChnl = 0;

	p2p_DevList_Lock(&info->devList);

	do
	{
		// retrieve peer channel
		if(NULL == (pDev = p2p_DevList_Find(&info->devList, mac, dtype)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("can't find peer in dev list\n"));
			return 0;
		}

		if(NULL == pDev->rxFrames[ftype])
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("no frame type: %u frame from the device\n", ftype));
			return 0;
		}

		txChnl = pDev->rxFrames[ftype]->channel;
	}while(FALSE);

	p2p_DevList_Unlock(&info->devList);

	return txChnl;
}

static
u1Byte
wdi_SendAction_GetRxActionFrameToken(
	IN  P2P_INFO				*info,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE 			type,
	IN  P2P_FRAME_TYPE			ftype
	)
{
	const P2P_DEV_LIST_ENTRY	*pDev = NULL;
	u1Byte						token = 0;

	p2p_DevList_Lock(&info->devList);

	do
	{
		// retrieve peer channel
		if(NULL == (pDev = p2p_DevList_Find(&info->devList, mac, type)))
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("can't find peer in dev list\n"));
			break;
		}

		if(NULL == pDev->rxFrames[ftype])
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("no probe rsp frame from the device\n"));
			break;
		}

		token = pDev->rxFrames[ftype]->token;
	}while(FALSE);

	p2p_DevList_Unlock(&info->devList);

	return token;
}

//-----------------------------------------------------------------------------
// Local: prepare data
//-----------------------------------------------------------------------------

static
NDIS_STATUS
wdi_SendAction_PdReq_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const P2P_DEV_LIST_ENTRY	*pDev = NULL;

	if(param->ProvisionDiscoveryRequestInfo.Optional.GroupID_IsPresent)
	{
		pDev = p2p_DevList_GetGo(&info->devList, param->RequestParams.PeerDeviceAddress.Address);
		RT_PRINT_ADDR(COMP_OID_SET, DBG_LOUD, "peer GO bssid:\n", pDev->mac);
	}
	else
	{
		pDev = p2p_DevList_Get(&info->devList, param->RequestParams.PeerDeviceAddress.Address, P2P_DEV_TYPE_DEV);
	}

	if(NULL == pDev)
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("can't find peer in dev list\n"));
		return NDIS_STATUS_INVALID_STATE;
	}
	
	info->ProvisionRequestGroupCapability = param->ProvisionDiscoveryRequestInfo.RequestParams.GroupCapability;

	if(param->ProvisionDiscoveryRequestInfo.Optional.GroupID_IsPresent)
	{
		info->bProvisionRequestUseGroupID = TRUE;

		// dev addr
		cpMacAddr(info->ProvisionRequestGroupIDDeviceAddress, param->ProvisionDiscoveryRequestInfo.GroupID.DeviceAddress.Address);

		// ssid
		info->uProvisionRequestGroupIDSSIDLength = (u1Byte)param->ProvisionDiscoveryRequestInfo.GroupID.GroupSSID.ElementCount;
		PlatformMoveMemory(
			info->ProvisionRequestGroupIDSSID, 
			param->ProvisionDiscoveryRequestInfo.GroupID.GroupSSID.pElements, 
			param->ProvisionDiscoveryRequestInfo.GroupID.GroupSSID.ElementCount
			);
	}
	else
	{
		info->bProvisionRequestUseGroupID = FALSE;
	}

	if(param->VendorIEs.ElementCount)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_PROVISION_DISCOVERY_REQUEST,
			param->VendorIEs.ElementCount,
			param->VendorIEs.pElements
			);
	}

	// the ProvisionDiscoveryContext
	cpMacAddr(info->ProvisionDiscoveryContext.devAddr, param->RequestParams.PeerDeviceAddress.Address);
	if(param->ProvisionDiscoveryRequestInfo.Optional.GroupID_IsPresent)
	{
		info->ProvisionDiscoveryContext.go = TRUE;
		cpMacAddr(info->ProvisionDiscoveryContext.goBssid, pDev->mac);
	}
	else
	{
		info->ProvisionDiscoveryContext.go = FALSE;
	}

	// token
	info->DialogToken = (u1Byte)param->RequestParams.DialogToken;

	// clear scan dev id so the req won't be terminated in P2P_OnProbeRsp
	P2PClearScanDeviceID(&info->ScanDeviceIDs);

	return status;
}

static
NDIS_STATUS
wdi_SendAction_PdRsp_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);

	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE
	//

	// addr
	cpMacAddr(info->ProvisionResponseReceiverDeviceAddress, param->ResponseParams.PeerDeviceAddress.Address);

	// token
	info->ProvisionResponseDialogToken = param->ResponseParams.DialogToken;
	
	// add ie
	if(param->VendorIEs.ElementCount)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_PROVISION_DISCOVERY_RESPONSE,
			param->VendorIEs.ElementCount,
			param->VendorIEs.pElements
			);
	}

	return status;
}

static
NDIS_STATUS
wdi_SendAction_InvitationReq_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	P2P_LIB_INVITATION_REQ_CONTEXT invParam = {0};

	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_INVITATION_REQUEST
	// ref: Wdi_Xlat_AllocSendInvitationReqOid
	//

	info->InvitationContext.InvitorRole = (param->InvitationRequestInfo.RequestParams.IsLocalGO) ? P2P_GO : P2P_DEVICE;
	info->InvitationContext.bPersistentInvitation = TEST_FLAG(param->InvitationRequestInfo.RequestParams.InvitationFlags, BIT0);
	info->InvitationContext.DialogToken = param->RequestParams.DialogToken;
	//info->InvitationContext.InvitedDevice
	info->InvitationContext.bToSendInvitationReqOnProbe = TRUE;
	info->InvitationContext.bToUseDeviceDiscoverability = FALSE;
	info->InvitationContext.Status = 0;
	info->InvitationContext.bInvitor = TRUE;
	PlatformMoveMemory(
		info->InvitationContext.SsidBuf, 
		param->InvitationRequestInfo.GroupID.GroupSSID.pElements,
		param->InvitationRequestInfo.GroupID.GroupSSID.ElementCount
		);
	info->InvitationContext.SsidLen = (u1Byte)param->InvitationRequestInfo.GroupID.GroupSSID.ElementCount;
	if(param->InvitationRequestInfo.Optional.GroupBSSID_IsPresent)
	{
		cpMacAddr(info->InvitationContext.GroupBssid, param->InvitationRequestInfo.GroupBSSID.Address);
	}
	cpMacAddr(info->InvitationContext.GODeviceAddress, param->InvitationRequestInfo.GroupID.DeviceAddress.Address);
	if(param->InvitationRequestInfo.Optional.OperatingChannel_IsPresent)
	{
		info->InvitationContext.OpChannel = (u1Byte)param->InvitationRequestInfo.OperatingChannel.ChannelNumber;
	}
	else
	{
		info->InvitationContext.OpChannel = 0;
	}
	/////////////////////////

	// config timeout
	info->GOConfigurationTimeout = (u1Byte)param->InvitationRequestInfo.RequestParams.GOConfigTimeout;
	info->ClientConfigurationTimeout = (u1Byte)param->InvitationRequestInfo.RequestParams.ClientConfigTimeout;

	// grp bssid
	if(param->InvitationRequestInfo.Optional.GroupBSSID_IsPresent)
	{
		info->bInvitationRequestUseGroupBSSID = TRUE;
		cpMacAddr(info->InvitationRequestGroupBSSID, param->InvitationRequestInfo.GroupBSSID.Address);
	}
	else
	{
		info->bInvitationRequestUseGroupBSSID = FALSE;
	}

	// op chnl
	// NOTE: We currently only use our pP2PInfo->OperatingChannel instead of the following OS-given value
	if(param->InvitationRequestInfo.Optional.OperatingChannel_IsPresent)
	{
		info->bInvitationRequestUseSpecifiedOperatingChannel = TRUE;
		info->uInvitationRequestOperatingChannelNumber = (u1Byte)param->InvitationRequestInfo.OperatingChannel.ChannelNumber;	
	}
	else
	{
		info->bInvitationRequestUseSpecifiedOperatingChannel = FALSE;
		info->uInvitationRequestOperatingChannelNumber = 0;
	}

	// local go
	info->bInvitationRequestLocalGO = param->InvitationRequestInfo.RequestParams.IsLocalGO;

	// token
	info->DialogToken = param->RequestParams.DialogToken;

	// clear scan dev id so the req won't be terminated in P2P_OnProbeRsp
	P2PClearScanDeviceID(&info->ScanDeviceIDs);

	return status;
}

static
NDIS_STATUS
wdi_SendAction_InvitationRsp_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);

	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE
	//

	// addr
	cpMacAddr(info->InvitationResponseReceiverDeviceAddress, param->ResponseParams.PeerDeviceAddress.Address);

	// token
	info->InvitationResponseDialogToken = param->ResponseParams.DialogToken;

	// config timeout
	info->GOConfigurationTimeout = (u1Byte)param->GONegotiationResponseInfo.ResponseParams.GOConfigTimeout;
	info->ClientConfigurationTimeout = (u1Byte)param->GONegotiationResponseInfo.ResponseParams.ClientConfigTimeout;

	// status
	info->InvitationResponseStatus = param->InvitationResponseInfo.ResponseParams.StatusCode;

	// grp bssid
	if(param->InvitationResponseInfo.Optional.GroupBSSID_IsPresent)
	{
		info->bInvitationResponseUseGroupBSSID = TRUE;
		cpMacAddr(info->InvitationResponseGroupBSSID, param->InvitationResponseInfo.GroupBSSID.Address);
	}
	else
	{
		info->bInvitationResponseUseGroupBSSID = FALSE;
	}

	// op chnl
	if(param->InvitationResponseInfo.Optional.OperatingChannel_IsPresent)
	{
		info->bInvitationResponseUseSpecifiedOperatingChannel = TRUE;
		info->uInvitationResponseOperatingChannelNumber = (u1Byte)param->InvitationResponseInfo.OperatingChannel.ChannelNumber;	
	}
	else
	{
		info->bInvitationResponseUseSpecifiedOperatingChannel = FALSE;
		info->uInvitationResponseOperatingChannelNumber = 0;
	}

	// add ie
	if(param->VendorIEs.ElementCount)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_INVITATION_RESPONSE,
			param->VendorIEs.ElementCount,
			param->VendorIEs.pElements
			);
	}

	return status;
}

static
NDIS_STATUS
wdi_SendAction_GoNegReq_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	OCTET_STRING				osWpsAttr, osDpid;

	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST
	//
	
	// config timeout
	info->GOConfigurationTimeout = (u1Byte)param->GONegotiationRequestInfo.RequestParams.GOConfigTimeout;
	info->ClientConfigurationTimeout = (u1Byte)param->GONegotiationRequestInfo.RequestParams.ClientConfigTimeout;
	
	// GroupCapability
	info->NegotiationRequestGroupCapability = param->GONegotiationRequestInfo.RequestParams.GroupCapability & ((u1Byte)(~gcP2PGroupOwner));		// Workaround since the pParameters->GroupCapability is wrong

	// go intent
	info->GOIntent = 
		((param->GONegotiationRequestInfo.RequestParams.GOIntent << 1) 
		| param->GONegotiationRequestInfo.RequestParams.TieBreaker);

	// intf addr
	cpMacAddr(info->InterfaceAddress, param->GONegotiationRequestInfo.RequestParams.IntendedInterfaceAddress.Address);

	// add ie
	if(param->VendorIEs.ElementCount)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_GO_NEGOTIATION_REQUEST,
			param->VendorIEs.ElementCount,
			param->VendorIEs.pElements
			);
	}

	// Update the internal WPS Device Password ID attribute ----------------------------------------------------------
	osWpsAttr.Octet = (u1Byte *)param->VendorIEs.pElements + 6;	// Skip (0xDD, 0x??, 0x00, 0x50, 0xF2, 0x04) 6-Byte Header
	osWpsAttr.Length = (u2Byte)param->VendorIEs.ElementCount - 6;	// Skip (0xDD, 0x??, 0x00, 0x50, 0xF2, 0x04) 6-Byte Header
	
	osDpid = P2PWpsIEGetAttribute(osWpsAttr, TRUE, P2P_WPS_ATTR_TAG_DEVICE_PASSWORD_ID);

	if(osDpid.Length == 2) 
	{
		info->WpsDevPasswdId = (WPS_DEVICE_PASSWD_ID) N2H2BYTE(*((pu2Byte)(osDpid.Octet)));
	}

	// connection context
	info->ConnectionContext.Status = P2P_STATUS_SUCCESS;
	info->ConnectionContext.DialogToken = param->RequestParams.DialogToken;
	info->ConnectionContext.bGoingToBeGO = FALSE;

	// token
	info->ConnectionContext.DialogToken = param->RequestParams.DialogToken;

	// clear scan dev id so the req won't be terminated in P2P_OnProbeRsp
	P2PClearScanDeviceID(&info->ScanDeviceIDs);

	return status;
}

static
NDIS_STATUS
wdi_SendAction_GoNegRsp_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);

	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE
	//

	// config timeout
	info->GOConfigurationTimeout = (u1Byte)param->GONegotiationResponseInfo.ResponseParams.GOConfigTimeout;
	info->ClientConfigurationTimeout = (u1Byte)param->GONegotiationResponseInfo.ResponseParams.ClientConfigTimeout;

	// addr
	cpMacAddr(info->NegotiationResponsePeerDeviceAddress, param->ResponseParams.PeerDeviceAddress.Address);

	// token
	info->NegotiationResponseDialogToken = param->ResponseParams.DialogToken;

	// status
	info->NegotiationResponseStatus = param->GONegotiationResponseInfo.ResponseParams.StatusCode;

	// go intent
	info->GOIntent = 
		((param->GONegotiationResponseInfo.ResponseParams.GOIntent << 1) 
		| param->GONegotiationResponseInfo.ResponseParams.TieBreaker);

	// intf addr
	cpMacAddr(info->InterfaceAddress, param->GONegotiationResponseInfo.ResponseParams.IntendedInterfaceAddress.Address);

	// gc
	info->NegotiationResponseGroupCapability = param->GONegotiationResponseInfo.ResponseParams.GroupCapability & ((u1Byte)(~gcP2PGroupOwner)); // Workaround since the pParameters->GroupCapability is wrong

	// grp id
	if(param->GONegotiationResponseInfo.Optional.GroupID_IsPresent)
	{
		info->bNegotiationResponseUseGroupID = TRUE;

		// dev addr
		cpMacAddr(info->NegotiationResponseGroupIDDeviceAddress, param->GONegotiationResponseInfo.GroupID.DeviceAddress.Address);

		// ssid
		info->uNegotiationResponseGroupIDSSIDLength = (u1Byte)param->GONegotiationResponseInfo.GroupID.GroupSSID.ElementCount;
		PlatformMoveMemory(
			info->NegotiationResponseGroupIDSSID, 
			param->GONegotiationResponseInfo.GroupID.GroupSSID.pElements,
			param->GONegotiationResponseInfo.GroupID.GroupSSID.ElementCount
		);
	}
	else
	{
		info->bNegotiationResponseUseGroupID = FALSE;
	}
	
	// add ie
	if(param->VendorIEs.ElementCount)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_GO_NEGOTIATION_RESPONSE,
			param->VendorIEs.ElementCount,
			param->VendorIEs.pElements
			);
	}

	// Update the state machine immediately 
	if(P2P_SC_SUCCESS == param->GONegotiationResponseInfo.ResponseParams.StatusCode)
	{
		info->State = P2P_STATE_GO_NEGO_RSP_SEND;
	}
	else
	{
		info->State = P2P_STATE_GO_NEGO_COMPLETE;
	}

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("update P2P state to 0x%x\n", info->State));
	PlatformCancelTimer(info->pAdapter, &info->P2PMgntTimer);
	PlatformSetTimer(info->pAdapter, &info->P2PMgntTimer, 0); 

	return status;
}

static
NDIS_STATUS
wdi_SendAction_GoNegConfirm_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	OCTET_STRING				osWpsAttr, osDpid;

	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRM
	//

	cpMacAddr(info->NegotiationConfirmPeerDeviceAddress, param->ResponseParams.PeerDeviceAddress.Address);
	info->NegotiationConfirmDialogToken = param->ResponseParams.DialogToken;
	info->NegotiationConfirmStatus = param->GONegotiationConfirmationInfo.ConfirmationParams.StatusCode;

	info->NegotiationConfirmGroupCapability = param->GONegotiationConfirmationInfo.ConfirmationParams.GroupCapability;
	info->NegotiationConfirmGroupCapability = param->GONegotiationConfirmationInfo.ConfirmationParams.GroupCapability & ((u1Byte)(~gcP2PGroupOwner));		// Workaround since the pParameters->GroupCapability is wrong

	if(param->GONegotiationConfirmationInfo.Optional.GroupID_IsPresent)
	{
		cpMacAddr(info->NegotiationConfirmGroupIDDeviceAddress, param->GONegotiationConfirmationInfo.GroupID.DeviceAddress.Address);
		info->uNegotiationConfirmGroupIDSSIDLength = (u1Byte)param->GONegotiationConfirmationInfo.GroupID.GroupSSID.ElementCount;
		PlatformMoveMemory(
				info->NegotiationConfirmGroupIDSSID, 
				param->GONegotiationConfirmationInfo.GroupID.GroupSSID.pElements, 
				info->uNegotiationConfirmGroupIDSSIDLength
			);
	}

	info->bNegotiationConfirmUseGroupID = (TRUE == param->GONegotiationConfirmationInfo.Optional.GroupID_IsPresent);

	// add ie
	if(param->VendorIEs.ElementCount)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_GO_NEGOTIATION_CONFIRM,
			param->VendorIEs.ElementCount,
			param->VendorIEs.pElements
			);
	}

	// token
	info->DialogToken = (u1Byte)param->ResponseParams.DialogToken;

	return status;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

NDIS_STATUS
WDI_SendAction_TerminateReq(
	IN  ADAPTER					*pAdapter
	)
{
	return OffChnlTx_AbortReq(pAdapter);
}

u1Byte
WDI_SendAction_UpdateTxFrameDialogToken(
	IN  VOID 					*req,
	IN  ADAPTER					*pAdapter,
	IN  WDI_P2P_ACTION_FRAME_TYPE frameType
	)
{
	FRAME_BUF					*frame = OffChnlTx_GetTxFrameBuf(req);
	u1Byte						token = 0;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	u1Byte						*devAddr = NULL;
	P2P_DEV_LIST_ENTRY 			*pDev = NULL;
	u1Byte						*mac = NULL;
	P2P_DEV_TYPE				devType = P2P_DEV_TYPE_LEGACY;

	RT_ASSERT(frame, ("%s(): buf is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN + 1 <= FrameBuf_Length(frame), 
		("%s(): invalid frame length: %u\n", __FUNCTION__, FrameBuf_Length(frame)));

	// determine dev type
	if(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST == frameType)
	{
		devType = info->bProvisionRequestUseGroupID ? P2P_DEV_TYPE_GO : P2P_DEV_TYPE_DEV;
	}
	else
	{
		devType = P2P_DEV_TYPE_DEV;
	}

	// get the previouse token
	token = *(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);

	// increase token
	token++;
	if(0 == token)
		token = 1;

	// write to the sending frame
	WriteEF1Byte(FrameBuf_MHead(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN, token);

	// update to the p2p dev list
	devAddr = Frame_Addr1(frame->os);
	
	if(P2P_DEV_TYPE_GO == devType && NULL != (pDev = p2p_DevList_GetGo(&info->devList, devAddr)))
	{
		mac = pDev->mac;
	}
	else
	{
		if(P2P_DEV_TYPE_GO == devType)
		{
			RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("bProvisionRequestUseGroupID is set but can't find the GO in dev list\n"));
		}
		
		mac = devAddr;
	}

	p2p_DevList_TxUpdate(&info->devList, 
			wdi_SendAction_XlatFrameType(frameType), 
			mac, 
			devType, 
			frame, 
			token, 
			OffChnlTx_GetTxChnl(req));

	info->DialogToken = token;

	return token;
}

RT_STATUS
WDI_SendAction_SkipProbeTemporarilly(
	IN	VOID					*req
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	
	RT_ASSERT(req, ("%s(): req is NULL!!!\n", __FUNCTION__));
	
	status = OffChnlTx_SkipProbeTemporarilly(req);

	return status;
}

VOID *
WDI_SendAction_PdReq(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txChnl = 0;
	P2P_DEV_TYPE				devType = P2P_DEV_TYPE_DEV;
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	PP2P_DEVICE_LIST_ENTRY 		pP2PDeviceListEntry = NULL;
	u1Byte						oldToken = info->DialogToken;

	RT_ASSERT(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST == param->RequestParams.RequestFrameType, ("invalid frame type: %u\n", param->RequestParams.RequestFrameType));

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = wdi_SendAction_PdReq_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(param->ProvisionDiscoveryRequestInfo.Optional.GroupID_IsPresent)
	{
		txMac = param->DeviceDescriptor.BSSID.Address;
		txChnl = (u1Byte)param->DeviceDescriptor.ChannelInfo.ChannelNumber;
		devType = P2P_DEV_TYPE_GO;
	}
	else
	{
		txMac = param->RequestParams.PeerDeviceAddress.Address;
		txChnl = (u1Byte)param->DeviceDescriptor.ChannelInfo.ChannelNumber;
		devType = P2P_DEV_TYPE_DEV;
	}
	
	if(NULL == (req = wdi_SendAction_PrepareReq(pAdapter, txMac, txChnl, param->RequestParams.SendTimeout)))
	{
		return NULL;
	}

	pP2PDeviceListEntry = P2PDeviceListFind(&info->DeviceList, param->RequestParams.PeerDeviceAddress.Address);
	if(pP2PDeviceListEntry)
	{
		u1Byte	BoostInitGainValue = (u1Byte)(pP2PDeviceListEntry->RecvSignalPower+110);
		u2Byte	boostDelaySec = WPS_HANDSHAKE_TIMEOUT_SEC;
		
		McDynamicMachanismSet(pAdapter, MC_DM_INIT_GAIN_BOOST_START, &BoostInitGainValue, sizeof(u1Byte));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS]: SignalStrength(%#x)\n", pP2PDeviceListEntry->RecvSignalPower));
		McDynamicMachanismSet(pAdapter, MC_DM_INIT_GAIN_BOOST_END_DELAY_SEC, &boostDelaySec, sizeof(u2Byte));
	}

	// setup cb
	ctx = (WDI_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	wdi_SendAction_InitCtx(ctx, 
		WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST, 
		WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE, 
		pAdapter, 
		param->RequestParams.PeerDeviceAddress.Address, 
		(P2P_DEV_TYPE_GO == devType) ? TRUE : FALSE,
		param->RequestParams.DialogToken, 
		oldToken,
		param->RequestParams.SendTimeout, 
		req, 
		P2P_EVENT_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE
		);

	// flush action frames in dev list so we don't drop frames because of dialog token mismatch
	p2p_DevList_FlushActionFrames(&info->devList, txMac, devType);
	
	// construct pd req
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_PDReq(info, frame, param->RequestParams.PeerDeviceAddress.Address, param->RequestParams.DialogToken, 0);

	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_PD_REQ, 
			txMac, 
			devType, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN),
			txChnl);	

	// issue req
	wdi_SendAction_IssueReq(pAdapter, req, "wdi-send-p2p-action-frame-pd-req");
	
	return req;
}

VOID *
WDI_SendAction_PdRsp(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	RT_ASSERT(WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == param->ResponseParams.ResponseFrameType, ("invalid frame type: %u\n", param->ResponseParams.ResponseFrameType));

	// retrieve peer mac/channel
	txMac = param->ResponseParams.PeerDeviceAddress.Address;
	if(0 == (txChnl = wdi_SendAction_GetTxChnl(info, txMac, P2P_DEV_TYPE_DEV, P2P_FID_PD_REQ)))
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = wdi_SendAction_PdRsp_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = wdi_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->ResponseParams.SendTimeout)))
	{
		return NULL;
	}
	OffChnlTx_SetPostAckDwellTime(req, 100);
	OffChnlTx_SetProbeEnabled(req, FALSE);
	OffChnlTx_SetTxAttemptCount(req, 1);

	// setup cb
	ctx = (WDI_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	wdi_SendAction_InitCtx(ctx, 
		WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE, 
		WDI_P2P_ACTION_FRAME_MAX_VALUE, 
		pAdapter, 
		param->ResponseParams.PeerDeviceAddress.Address, 
		FALSE,
		param->ResponseParams.DialogToken, 
		oldToken,
		param->ResponseParams.SendTimeout, 
		req, 
		P2P_EVENT_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE
		);
	
	// construct confirm
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_PDRsp(info, param->ResponseParams.DialogToken, NULL, 0, frame, txMac);
	
	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_PD_RSP, 
			txMac, 
			P2P_DEV_TYPE_DEV, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	wdi_SendAction_IssueReq(pAdapter, req, "wdi-send-p2p-action-pd-rsp");
	
	return req;
}

VOID *
WDI_SendAction_InvitationReq(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	PP2P_DEVICE_LIST_ENTRY 		pP2PDeviceListEntry = NULL;
	u1Byte						oldToken = info->DialogToken;

	RT_ASSERT(WDI_P2P_ACTION_FRAME_INVITATION_REQUEST == param->RequestParams.RequestFrameType, ("invalid frame type: %u\n", param->RequestParams.RequestFrameType));

	// retrieve peer mac/channel
	txMac = param->RequestParams.PeerDeviceAddress.Address;
	txChnl = (u1Byte)param->DeviceDescriptor.ChannelInfo.ChannelNumber;

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = wdi_SendAction_InvitationReq_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = wdi_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->RequestParams.SendTimeout)))
	{
		return NULL;
	}

	pP2PDeviceListEntry = P2PDeviceListFind(&info->DeviceList, param->RequestParams.PeerDeviceAddress.Address);
	if(pP2PDeviceListEntry)
	{
		u1Byte	BoostInitGainValue = (u1Byte)(pP2PDeviceListEntry->RecvSignalPower+110);
		u2Byte	boostDelaySec = ASSOC_HANDSHAKE_DHCP_DELAY_SEC;
		
		McDynamicMachanismSet(pAdapter, MC_DM_INIT_GAIN_BOOST_START, &BoostInitGainValue, sizeof(u1Byte));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS]: SignalStrength(%#x)\n", pP2PDeviceListEntry->RecvSignalPower));
		McDynamicMachanismSet(pAdapter, MC_DM_INIT_GAIN_BOOST_END_DELAY_SEC, &boostDelaySec, sizeof(u2Byte));
	}

	// setup cb
	ctx = (WDI_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	wdi_SendAction_InitCtx(ctx, 
		WDI_P2P_ACTION_FRAME_INVITATION_REQUEST, 
		WDI_P2P_ACTION_FRAME_INVITATION_RESPONSE, 
		pAdapter, 
		param->RequestParams.PeerDeviceAddress.Address, 
		param->ProvisionDiscoveryRequestInfo.Optional.GroupID_IsPresent ? TRUE : FALSE,
		param->RequestParams.DialogToken, 
		oldToken,
		param->RequestParams.SendTimeout, 
		req, 
		P2P_EVENT_INVITATION_REQUEST_SEND_COMPLETE
		);
	
	// construct invitation req
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_InvitationReq(info, frame, txMac, param->RequestParams.DialogToken);

	// flush action frames in dev list so we don't remember old frames from previous connections
	p2p_DevList_FlushActionFrames(&info->devList, txMac, P2P_DEV_TYPE_DEV);
	
	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_INV_REQ, 
			txMac, 
			P2P_DEV_TYPE_DEV, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	wdi_SendAction_IssueReq(pAdapter, req, "wdi-send-p2p-action-frame-invitation-req");
	
	return req;
}

VOID *
WDI_SendAction_InvitationRsp(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	RT_ASSERT(WDI_P2P_ACTION_FRAME_INVITATION_RESPONSE == param->ResponseParams.ResponseFrameType, ("invalid frame type: %u\n", param->ResponseParams.ResponseFrameType));

	// retrieve peer mac/channel
	txMac = param->ResponseParams.PeerDeviceAddress.Address;
	if(0 == (txChnl = wdi_SendAction_GetTxChnl(info, txMac, P2P_DEV_TYPE_DEV, P2P_FID_INV_REQ)))
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = wdi_SendAction_InvitationRsp_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = wdi_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->ResponseParams.SendTimeout)))
	{
		return NULL;
	}
	OffChnlTx_SetPostAckDwellTime(req, 100);
	OffChnlTx_SetProbeEnabled(req, FALSE);
	OffChnlTx_SetTxAttemptCount(req, 1);
	OffChnlTx_SetShtPostAckDwellOnPositivelyAcked(req, TRUE);

	// setup cb
	ctx = (WDI_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	wdi_SendAction_InitCtx(ctx, 
		WDI_P2P_ACTION_FRAME_INVITATION_RESPONSE, 
		WDI_P2P_ACTION_FRAME_MAX_VALUE, 
		pAdapter, 
		param->ResponseParams.PeerDeviceAddress.Address, 
		FALSE,
		param->ResponseParams.DialogToken, 
		oldToken,
		param->ResponseParams.SendTimeout, 
		req, 
		P2P_EVENT_INVITATION_RESPONSE_SEND_COMPLETE
		);
	
	// construct confirm
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_InvitationRsp(info, param->ResponseParams.DialogToken, frame, txMac);

	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_INV_RSP, 
			txMac, 
			P2P_DEV_TYPE_DEV,
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	wdi_SendAction_IssueReq(pAdapter, req, "wdi-send-p2p-action-frame-invitation-rsp");
	
	return req;
}

VOID *
WDI_SendAction_GoNegReq(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	PP2P_DEVICE_LIST_ENTRY 		pP2PDeviceListEntry = NULL;
	u1Byte						oldToken = info->DialogToken;

	//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Op Chnl = %d\n", info->OperatingChannel));

	RT_ASSERT(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST == param->RequestParams.RequestFrameType, ("invalid frame type: %u\n", param->RequestParams.RequestFrameType));

	// retrieve peer mac/channel
	txMac = param->RequestParams.PeerDeviceAddress.Address;
	txChnl = (u1Byte)param->DeviceDescriptor.ChannelInfo.ChannelNumber;

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = wdi_SendAction_GoNegReq_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = wdi_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->RequestParams.SendTimeout)))
	{
		return NULL;
	}

	pP2PDeviceListEntry = P2PDeviceListFind(&info->DeviceList, param->RequestParams.PeerDeviceAddress.Address);
	if(pP2PDeviceListEntry)
	{
		u1Byte	BoostInitGainValue = (u1Byte)(pP2PDeviceListEntry->RecvSignalPower+110);
		u2Byte	boostDelaySec = WPS_HANDSHAKE_TIMEOUT_SEC;
		
		McDynamicMachanismSet(pAdapter, MC_DM_INIT_GAIN_BOOST_START, &BoostInitGainValue, sizeof(u1Byte));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS]: SignalStrength(%#x)\n", pP2PDeviceListEntry->RecvSignalPower));
		McDynamicMachanismSet(pAdapter, MC_DM_INIT_GAIN_BOOST_END_DELAY_SEC, &boostDelaySec, sizeof(u2Byte));
	}

	// setup cb
	ctx = (WDI_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	wdi_SendAction_InitCtx(ctx, 
		WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST, 
		WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE, 
		pAdapter, 
		param->RequestParams.PeerDeviceAddress.Address, 
		FALSE,
		param->RequestParams.DialogToken, 
		oldToken,
		param->RequestParams.SendTimeout, 
		req, 
		P2P_EVENT_GO_NEGOTIATION_REQUEST_SEND_COMPLETE
		);

	// construct pd req
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_GoNegReq(info, frame, txMac, param->RequestParams.DialogToken);
	
	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_GO_NEG_REQ, 
			txMac, 
			P2P_DEV_TYPE_DEV, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	wdi_SendAction_IssueReq(pAdapter, req, "wdi-send-p2p-action-frame-go-neg-req");
	
	return req;
}

VOID *
WDI_SendAction_GoNegRsp(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	RT_ASSERT(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE == param->ResponseParams.ResponseFrameType, ("invalid frame type: %u\n", param->ResponseParams.ResponseFrameType));

	// retrieve peer mac/channel
	txMac = param->ResponseParams.PeerDeviceAddress.Address;
	if(0 == (txChnl = wdi_SendAction_GetTxChnl(info, txMac, P2P_DEV_TYPE_DEV, P2P_FID_GO_NEG_REQ)))
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = wdi_SendAction_GoNegRsp_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = wdi_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->ResponseParams.SendTimeout)))
	{
		return NULL;
	}
	OffChnlTx_SetPostAckDwellTime(req, 100);
	OffChnlTx_SetProbeEnabled(req, FALSE);
	OffChnlTx_SetTxAttemptCount(req, 1);

	// setup cb
	ctx = (WDI_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	wdi_SendAction_InitCtx(ctx, 
		WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE, 
		WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM, 
		pAdapter, 
		param->ResponseParams.PeerDeviceAddress.Address, 
		FALSE,
		param->ResponseParams.DialogToken, 
		oldToken,
		param->ResponseParams.SendTimeout, 
		req, 
		P2P_EVENT_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE
		);
	
	// construct confirm
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_GoNegRsp(info, param->ResponseParams.DialogToken, frame, txMac);
	
	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_GO_NEG_RSP, 
			txMac, 
			P2P_DEV_TYPE_DEV, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	wdi_SendAction_IssueReq(pAdapter, req, "wdi-send-p2p-action-frame-go-rsp");
	
	return req;
}

VOID *
WDI_SendAction_GoNegConfirm(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txToken = 0;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	RT_ASSERT(WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM == param->ResponseParams.ResponseFrameType, ("invalid frame type: %u\n", param->ResponseParams.ResponseFrameType));

	// retrieve peer mac/channel
	txMac = param->ResponseParams.PeerDeviceAddress.Address;
	if(0 == (txChnl = wdi_SendAction_GetTxChnl(info, txMac, P2P_DEV_TYPE_DEV, P2P_FID_GO_NEG_RSP)))
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// get go neg rsp token, to be used in the go neg conf
	if(0 == (txToken = wdi_SendAction_GetRxActionFrameToken(info, param->ResponseParams.PeerDeviceAddress.Address, P2P_DEV_TYPE_DEV, P2P_FID_GO_NEG_RSP)))
	{
		RT_TRACE_F(COMP_OID_SET, DBG_WARNING, ("failed to get go neg rsp token\n"));
		return NULL;
	}

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = wdi_SendAction_GoNegConfirm_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = wdi_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->ResponseParams.SendTimeout)))
	{
		return NULL;
	}
	OffChnlTx_SetPostAckDwellTime(req, 100);
	OffChnlTx_SetProbeEnabled(req, FALSE);
	OffChnlTx_SetTxAttemptCount(req, 1);
	OffChnlTx_SetShtPostAckDwellOnPositivelyAcked(req, TRUE);

	// setup cb
	ctx = (WDI_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	wdi_SendAction_InitCtx(ctx, 
		WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM, 
		WDI_P2P_ACTION_FRAME_MAX_VALUE, 
		pAdapter, 
		param->ResponseParams.PeerDeviceAddress.Address, 
		FALSE,
		param->ResponseParams.DialogToken, 
		oldToken,
		param->ResponseParams.SendTimeout, 
		req, 
		P2P_EVENT_GO_NEGOTIATION_CONFIRM_SEND_COMPLETE
		);
	
	// construct confirm
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_GoNegConf(info, txToken, frame, txMac);
	
	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_GO_NEG_CONF, 
			txMac, 
			P2P_DEV_TYPE_DEV, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	wdi_SendAction_IssueReq(pAdapter, req, "wdi-send-p2p-action-frame-go-confirm");
	
	return req;
}

VOID
ConstructGASActionFrame(
	IN FRAME_BUF	*frame,
	IN const u1Byte *dest,
	IN const u1Byte	*src
	)
{
	pu1Byte	buf = NULL;
	
	if(NULL == (buf = FrameBuf_Add(frame, 24)))
	{
		return;
	}
	
	SET_80211_HDR_FRAME_CONTROL(buf, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(buf, Type_Action);
	SET_80211_HDR_DURATION(buf, 0);
	SET_80211_HDR_ADDRESS1(buf, dest);
	SET_80211_HDR_ADDRESS2(buf, src);
	SET_80211_HDR_ADDRESS3(buf, dest);
	SET_80211_HDR_FRAGMENT_SEQUENCE(buf, 0);

	// LE only take care of MAC header, UE will send GAS payload
#if 0
	FrameBuf_Add_u1(pBuf, WLAN_ACTION_PUBLIC);
	FrameBuf_Add_u1(pBuf, WLAN_PA_GAS_INITIAL_REQ);
	FrameBuf_Add_be_u4(pBuf, P2P_IE_VENDOR_TYPE);
	FrameBuf_Add_u1(pBuf, subtype);
	FrameBuf_Add_u1(pBuf, dialogToken);
#endif
}


#define ANQP_EVENT_REQUEST_SEND_COMPLETE		0x01
#define ANQP_EVENT_RESPONSE_SEND_COMPLETE		0x02

VOID *
WDI_SendAction_Req(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;

	//UE set this timeout=200ms, sometimes seems this time may not large enough.
	//LE can further tune this value later.
	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("WDI_SendAction_Req force set timeout=1000ms\n"));
	param->RequestParams.SendTimeout = 1000;
	
	// prepare off chnl tx req
	// Note: Use wdi_SendAction_PrepareReqEx to make PostACKDwellTime from UE take effect
	if(NULL == (req = wdi_SendAction_PrepareReqEx(pAdapter, 
						(const u1Byte *)param->RequestParams.DestinationAddress.Address, 
						(u1Byte)param->RequestParams.ChannelNumber, 
						param->RequestParams.SendTimeout,
						param->RequestParams.PostACKDwellTime)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("WDI_SendAction_Req failed to prepare request!\n"));
		return NULL;
	}

	RT_TRACE(COMP_OID_SET, DBG_TRACE, ("WDI_SendAction_Req prepare OK! Chan:%d PostACKDwellTime:%d DA:%x-%x-%x-%x-%x-%x\n", 
		param->RequestParams.ChannelNumber,
		param->RequestParams.PostACKDwellTime,
		param->RequestParams.DestinationAddress.Address[0],
		param->RequestParams.DestinationAddress.Address[1],
		param->RequestParams.DestinationAddress.Address[2],
		param->RequestParams.DestinationAddress.Address[3],
		param->RequestParams.DestinationAddress.Address[4],
		param->RequestParams.DestinationAddress.Address[5]));
	
	ctx = (WDI_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	// TODO: double check request/response type parameter
	wdi_SendAction_InitCtx(ctx, 
		0,//WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST, 
		1,//WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE, 
		pAdapter, 
		(const u1Byte *)param->RequestParams.DestinationAddress.Address, 
		FALSE,
		0,//param->RequestParams.DialogToken, 
		0,
		param->RequestParams.SendTimeout, 
		req, 
		ANQP_EVENT_REQUEST_SEND_COMPLETE//P2P_EVENT_GO_NEGOTIATION_REQUEST_SEND_COMPLETE
		);

	//txReq = (OFF_CHNL_TX_REQ *)req;
	//frame = &txReq->frameBuf;
	frame = OffChnlTx_GetTxFrameBuf(req);
	ConstructGASActionFrame(frame, (const u1Byte *)param->RequestParams.DestinationAddress.Address, (const u1Byte *)pAdapter->CurrentAddress);
	// TODO: double check there
	PlatformMoveMemory(frame->os.Octet+frame->os.Length, param->ActionFrameBody.pElements, param->ActionFrameBody.ElementCount);
	frame->os.Length += (u2Byte)param->ActionFrameBody.ElementCount;

	DbgPrintEx(0, 0, "\nDump header of GAS Request Frame:\n");
	DbgPrintEx(0, 0, "0-7 0x%x-%x-%x-%x-%x-%x-%x-%x\n", 
		frame->os.Octet[0], 
		frame->os.Octet[1], 
		frame->os.Octet[2], 
		frame->os.Octet[3], 
		frame->os.Octet[4],
		frame->os.Octet[5],
		frame->os.Octet[6],
		frame->os.Octet[7]);
	DbgPrintEx(0, 0, "8-15 0x%x-%x-%x-%x-%x-%x-%x-%x\n",
		frame->os.Octet[8],
		frame->os.Octet[9],
		frame->os.Octet[10],
		frame->os.Octet[11],
		frame->os.Octet[12],
		frame->os.Octet[13],
		frame->os.Octet[14],
		frame->os.Octet[15]);
	DbgPrintEx(0, 0, "16-23 0x%x-%x-%x-%x-%x-%x-%x-%x\n",
		frame->os.Octet[16],
		frame->os.Octet[17],
		frame->os.Octet[18],
		frame->os.Octet[19],
		frame->os.Octet[20],
		frame->os.Octet[21],
		frame->os.Octet[22],
		frame->os.Octet[23]);

	DbgPrintEx(0, 0, "/n GAS request frame Payload from UE 0-7: 0x%x-%x-%x-%x-%x-%x-%x-%x\n",
		param->ActionFrameBody.pElements[0],
		param->ActionFrameBody.pElements[1],
		param->ActionFrameBody.pElements[2],
		param->ActionFrameBody.pElements[3],
		param->ActionFrameBody.pElements[4],
		param->ActionFrameBody.pElements[5],
		param->ActionFrameBody.pElements[6],
		param->ActionFrameBody.pElements[7]);
		
	wdi_SendAction_IssueReq(pAdapter, req, "wdi-send-action-frame-req");
	
	return req;
}

VOID *
WDI_SendAction_Rsp(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	VOID 						*req = NULL;
	WDI_SEND_ACTION_CTX			*ctx = NULL;
	FRAME_BUF					*frame = NULL;

	if(NULL == (req = wdi_SendAction_PrepareReqEx(pAdapter, 
						(const u1Byte *)param->ResponseParams.DestinationAddress.Address, 
						(u1Byte)param->ResponseParams.ChannelNumber, 
						param->ResponseParams.SendTimeout,
						param->ResponseParams.PostACKDwellTime)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("WDI_SendAction_Rsp failed to prepare request!\n"));
		return NULL;
	}

	ctx = (WDI_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	// TODO: double check request/response type parameter
	wdi_SendAction_InitCtx(ctx, 
		0,//WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST, 
		1,//WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE, 
		pAdapter, 
		(const u1Byte *)param->ResponseParams.DestinationAddress.Address, 
		FALSE,
		0,//param->RequestParams.DialogToken, 
		0,
		param->ResponseParams.SendTimeout,
		req,
		ANQP_EVENT_RESPONSE_SEND_COMPLETE//P2P_EVENT_GO_NEGOTIATION_REQUEST_SEND_COMPLETE
		);

	//txReq = (OFF_CHNL_TX_REQ *)req;
	//frame = &txReq->frameBuf;
	frame = OffChnlTx_GetTxFrameBuf(req);
	ConstructGASActionFrame(frame, (const u1Byte *)param->ResponseParams.DestinationAddress.Address, (const u1Byte *)pAdapter->CurrentAddress);
	// TODO: double check here
	PlatformMoveMemory(frame->os.Octet, param->ActionFrameBody.pElements, param->ActionFrameBody.ElementCount);
	frame->os.Length += (u2Byte)param->ActionFrameBody.ElementCount;

	wdi_SendAction_IssueReq(pAdapter, req, "wdi-send-action-frame-rsp");
	
	return req;
}

