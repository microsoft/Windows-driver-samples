////////////////////////////////////////////////////////////////////////////////
//
//	File name:		N63C_SendAction.c
//	Description:	
//
//	Author:			haich
//
////////////////////////////////////////////////////////////////////////////////
#include "Mp_Precomp.h"
#include "N63C_SendAction.h"

#if WPP_SOFTWARE_TRACE
#include "N63C_SendAction.tmh"
#endif

#include "N63C_SendAction.h"
#include "P2P_Internal.h"

#define N63C_SEND_ACTION_POST_ACK_DWELL_TIME_MS			300
#define N63C_SEND_ACTION_RETRY_COUNT					15
#define N63C_SEND_ACTION_RETRY_INTERMITTENT_TIME_MS		300
#define N63C_SEND_ACTION_RETRY_INTERMITTENT_TIME_HCT_MS	0

//-----------------------------------------------------------------------------
// Local: helper
//-----------------------------------------------------------------------------

static
P2P_FRAME_TYPE
n63c_SendAction_XlatFrameType(
	IN  N63C_P2P_ACTION_FRAME_TYPE n63cFType
	)
{
	if(N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST == n63cFType)
		return P2P_FID_GO_NEG_REQ;

	if(N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE == n63cFType)
		return P2P_FID_GO_NEG_RSP;

	if(N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM == n63cFType)
		return P2P_FID_GO_NEG_CONF;

	if(N63C_P2P_ACTION_FRAME_INVITATION_REQUEST == n63cFType)
		return P2P_FID_INV_REQ;

	if(N63C_P2P_ACTION_FRAME_INVITATION_RESPONSE == n63cFType)
		return P2P_FID_INV_RSP;

	if(N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST == n63cFType)
		return P2P_FID_PD_REQ;

	if(N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE == n63cFType)
		return P2P_FID_PD_RSP;

	return P2P_FID_MAX;
}

static
VOID
n63c_SendAction_InitCtx(
	IN  N63C_SEND_ACTION_CTX	*ctx,
	IN  N63C_P2P_ACTION_FRAME_TYPE reqType,
	IN  N63C_P2P_ACTION_FRAME_TYPE rspType,
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
n63c_SendAction_PrepareReq(
	IN  ADAPTER					*pAdapter,
	IN  const u1Byte			*txMac,
	IN  u1Byte					chnl,
	IN  u4Byte					timeout
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	VOID 						*req = NULL;
	N63C_SEND_ACTION_CTX		*ctx = NULL;
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
	OffChnlTx_SetPostAckDwellTime(req, N63C_SEND_ACTION_POST_ACK_DWELL_TIME_MS);
	OffChnlTx_SetProbeEnabled(req, TRUE);
	OffChnlTx_SetTxChnl(req, chnl);
	OffChnlTx_SetTxAttemptCount(req, N63C_SEND_ACTION_RETRY_COUNT);
	OffChnlTx_SetRetryIntermittentTime(req, 
		pAdapter->bInHctTest 
			? N63C_SEND_ACTION_RETRY_INTERMITTENT_TIME_HCT_MS 
			: N63C_SEND_ACTION_RETRY_INTERMITTENT_TIME_MS
		);
	
	return req;
}

static
RT_STATUS
n63c_SendAction_IssueReq(
	IN  ADAPTER					*pAdapter,
	IN  VOID					*req,
	IN  const char				*typeInfo
	)
{
	return OffChnlTx_IssueReq(pAdapter, req, typeInfo);
}

static
u1Byte
n63c_SendAction_GetTxChnl(
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
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("can't find peer in dev list\n"));
			return 0;
		}

		if(NULL == pDev->rxFrames[ftype])
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("no frame type: %u frame from the device\n", ftype));
			return 0;
		}

		txChnl = pDev->rxFrames[ftype]->channel;
	}while(FALSE);

	p2p_DevList_Unlock(&info->devList);

	return txChnl;
}

static
u1Byte
n63c_SendAction_GetRxActionFrameToken(
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
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("can't find peer in dev list\n"));
			break;
		}

		if(NULL == pDev->rxFrames[ftype])
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("no probe rsp frame from the device\n"));
			break;
		}

		token = pDev->rxFrames[ftype]->token;
	}while(FALSE);

	p2p_DevList_Unlock(&info->devList);

	return token;
}

//-----------------------------------------------------------------------------
// Exported: prepare data
//-----------------------------------------------------------------------------
static
NDIS_STATUS
n63c_SendAction_PdReq_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const P2P_DEV_LIST_ENTRY	*pDev = NULL;

	if(param->bUseGroupID)
	{
		pDev = p2p_DevList_GetGo(&info->devList, param->PeerDeviceAddress);
		RT_PRINT_ADDR(COMP_OID_SET, DBG_LOUD, "peer GO bssid:\n", pDev->mac);
	}
	else
	{
		pDev = p2p_DevList_Get(&info->devList, param->PeerDeviceAddress, P2P_DEV_TYPE_DEV);
	}

	if(NULL == pDev)
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("can't find peer in dev list\n"));
		return NDIS_STATUS_INVALID_STATE;
	}

	info->ProvisionRequestGroupCapability = param->GroupCapability;

	if(param->bUseGroupID)
	{
		info->bProvisionRequestUseGroupID = TRUE;

		// dev addr
		cpMacAddr(info->ProvisionRequestGroupIDDeviceAddress, param->PeerDeviceAddress);

		// ssid
		info->uProvisionRequestGroupIDSSIDLength = (u1Byte)param->GroupID.SSID.uSSIDLength;
		PlatformMoveMemory(
			info->ProvisionRequestGroupIDSSID, 
			param->GroupID.SSID.ucSSID, 
			param->GroupID.SSID.uSSIDLength
			);
	}
	else
	{
		info->bProvisionRequestUseGroupID = FALSE;
	}

	if(param->uIEsLength)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_PROVISION_DISCOVERY_REQUEST,
			param->uIEsLength,
			(u1Byte *)param + param->uIEsOffset
			);
	}

	// the ProvisionDiscoveryContext
	cpMacAddr(info->ProvisionDiscoveryContext.devAddr, param->PeerDeviceAddress);
	if(param->bUseGroupID)
	{
		info->ProvisionDiscoveryContext.go = TRUE;
		cpMacAddr(info->ProvisionDiscoveryContext.goBssid, pDev->mac);
	}
	else
	{
		info->ProvisionDiscoveryContext.go = FALSE;
	}

	// token
	info->DialogToken = (u1Byte)param->DialogToken;

	// clear scan dev id so the req won't be terminated in P2P_OnProbeRsp
	P2PClearScanDeviceID(&info->ScanDeviceIDs);

	return status;
}

static
NDIS_STATUS
n63c_SendAction_PdRsp_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);

	// addr
	cpMacAddr(info->ProvisionResponseReceiverDeviceAddress, param->ReceiverDeviceAddress);

	// token
	info->ProvisionResponseDialogToken = param->DialogToken;

	// add ie
	if(param->uIEsLength)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_PROVISION_DISCOVERY_RESPONSE,
			param->uIEsLength,
			(u1Byte *)param + param->uIEsOffset
			);
	}

	// token
	info->DialogToken = (u1Byte)param->DialogToken;

	// clear scan dev id so the req won't be terminated in P2P_OnProbeRsp
	P2PClearScanDeviceID(&info->ScanDeviceIDs);

	return status;
}

static
NDIS_STATUS
n63c_SendAction_InvitationReq_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_INVITATION_REQUEST_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	
	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_INVITATION_REQUEST
	// ref: Wdi_Xlat_AllocSendInvitationReqOid
	//
	
	info->InvitationContext.InvitorRole = (param->bLocalGO) ? P2P_GO : P2P_DEVICE;
	info->InvitationContext.bPersistentInvitation = (1 == param->InvitationFlags.InvitationType);
	info->InvitationContext.DialogToken = param->DialogToken;
	//info->InvitationContext.InvitedDevice
	info->InvitationContext.bToSendInvitationReqOnProbe = TRUE;
	info->InvitationContext.bToUseDeviceDiscoverability = FALSE;
	info->InvitationContext.Status = 0;
	info->InvitationContext.bInvitor = TRUE;
	PlatformMoveMemory(
		info->InvitationContext.SsidBuf, 
		param->GroupID.SSID.ucSSID,
		param->GroupID.SSID.uSSIDLength
		);
	info->InvitationContext.SsidLen = (u1Byte)param->GroupID.SSID.uSSIDLength;
	cpMacAddr(info->InvitationContext.GroupBssid, param->GroupBSSID);
	cpMacAddr(info->InvitationContext.GODeviceAddress, param->GroupID.DeviceAddress);
	info->InvitationContext.OpChannel = info->OperatingChannel;
	
	// config timeout
	info->GOConfigurationTimeout = (u1Byte)param->MinimumConfigTimeout.GOTimeout;
	info->ClientConfigurationTimeout = (u1Byte)param->MinimumConfigTimeout.ClientTimeout;

	// grp bssid
	if(param->bUseGroupBSSID)
	{
		info->bInvitationRequestUseGroupBSSID = TRUE;
		cpMacAddr(info->InvitationRequestGroupBSSID, param->GroupBSSID);
	}
	else
	{
		info->bInvitationRequestUseGroupBSSID = FALSE;
	}

	// op chnl
	// NOTE: We currently only use our pP2PInfo->OperatingChannel instead of the following OS-given value
	if(param->bUseSpecifiedOperatingChannel)
	{
		info->bInvitationRequestUseSpecifiedOperatingChannel = TRUE;
		info->uInvitationRequestOperatingChannelNumber = (u1Byte)param->OperatingChannel.ChannelNumber;	
		
		//RT_TRACE(COMP_OID_SET, DBG_LOUD, ("use specific chnl: %u\n", info->uInvitationRequestOperatingChannelNumber));
	}
	else
	{
		info->bInvitationRequestUseSpecifiedOperatingChannel = FALSE;
		info->uInvitationRequestOperatingChannelNumber = 0;
	}

	// local go
	info->bInvitationRequestLocalGO = param->bLocalGO;

	// token
	info->DialogToken = (u1Byte)param->DialogToken;

	// add ie
	if(param->uIEsOffset)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_INVITATION_REQUEST,
			param->uIEsLength,
			(u1Byte *)param + param->uIEsOffset
			);
	}
	
	// clear scan dev id so the req won't be terminated in P2P_OnProbeRsp
	P2PClearScanDeviceID(&info->ScanDeviceIDs);

	return status;
}

static
NDIS_STATUS
n63c_SendAction_InvitationRsp_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_INVITATION_RESPONSE_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);

	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE
	//

	// addr
	cpMacAddr(info->InvitationResponseReceiverDeviceAddress, param->ReceiverDeviceAddress);

	// token
	info->InvitationResponseDialogToken = param->DialogToken;

	// config timeout
	info->GOConfigurationTimeout = (u1Byte)param->MinimumConfigTimeout.GOTimeout;
	info->ClientConfigurationTimeout = (u1Byte)param->MinimumConfigTimeout.ClientTimeout;

	// status
	info->InvitationResponseStatus = param->Status;

	// grp bssid
	if(param->bUseGroupBSSID)
	{
		info->bInvitationResponseUseGroupBSSID = TRUE;
		cpMacAddr(info->InvitationResponseGroupBSSID, param->GroupBSSID);
	}
	else
	{
		info->bInvitationResponseUseGroupBSSID = FALSE;
	}

	// op chnl
	if(param->bUseSpecifiedOperatingChannel)
	{
		info->bInvitationResponseUseSpecifiedOperatingChannel = TRUE;
		info->uInvitationResponseOperatingChannelNumber = (u1Byte)param->OperatingChannel.ChannelNumber;	
	}
	else
	{
		info->bInvitationResponseUseSpecifiedOperatingChannel = FALSE;
		info->uInvitationResponseOperatingChannelNumber = 0;
	}

	// add ie
	if(param->uIEsLength)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_INVITATION_RESPONSE,
			param->uIEsLength,
			(u1Byte *)param + param->uIEsOffset
			);
	}

	return status;
}

static
NDIS_STATUS
n63c_SendAction_GoNegReq_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	OCTET_STRING				osWpsAttr, osDpid;
	
	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST
	// ref: wdi_SendAction_GoNegReq_PrepareData
	//
	
	// config timeout
	info->GOConfigurationTimeout = param->MinimumConfigTimeout.GOTimeout;
	info->ClientConfigurationTimeout = param->MinimumConfigTimeout.ClientTimeout;
	
	// capability
	info->NegotiationRequestGroupCapability = param->GroupCapability;
	info->NegotiationRequestGroupCapability = param->GroupCapability & ((u1Byte)(~gcP2PGroupOwner));		// Workaround since the pParameters->GroupCapability is wrong

	// go intent
	info->GOIntent = 
		((param->GroupOwnerIntent.Intent << 1) 
		| param->GroupOwnerIntent.TieBreaker);

	// intf addr
	cpMacAddr(info->InterfaceAddress, param->IntendedInterfaceAddress);

	// add ie
	if(param->uIEsLength)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_GO_NEGOTIATION_REQUEST,
			param->uIEsLength,
			(u1Byte *)param + param->uIEsOffset
			);
	}

	// Update the internal WPS Device Password ID attribute ----------------------------------------------------------
	osWpsAttr.Octet = (u1Byte *)param + param->uIEsOffset + 6;	// Skip (0xDD, 0x??, 0x00, 0x50, 0xF2, 0x04) 6-Byte Header
	osWpsAttr.Length = (u2Byte)param->uIEsLength - 6;	// Skip (0xDD, 0x??, 0x00, 0x50, 0xF2, 0x04) 6-Byte Header
	
	osDpid = P2PWpsIEGetAttribute(osWpsAttr, TRUE, P2P_WPS_ATTR_TAG_DEVICE_PASSWORD_ID);

	if(osDpid.Length == 2) 
	{
		info->WpsDevPasswdId = (WPS_DEVICE_PASSWD_ID) N2H2BYTE(*((pu2Byte)(osDpid.Octet)));
	}

	// connection context
	info->ConnectionContext.Status = P2P_STATUS_SUCCESS;
	info->ConnectionContext.DialogToken = param->DialogToken;
	info->ConnectionContext.bGoingToBeGO = FALSE;

	// token
	info->DialogToken = (u1Byte)param->DialogToken;

	// clear scan dev id so the req won't be terminated in P2P_OnProbeRsp
	P2PClearScanDeviceID(&info->ScanDeviceIDs);

	return status;
}

static
NDIS_STATUS
n63c_SendAction_GoNegRsp_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);

	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE
	//

	// config timeout
	info->GOConfigurationTimeout = (u1Byte)param->MinimumConfigTimeout.GOTimeout;
	info->ClientConfigurationTimeout = (u1Byte)param->MinimumConfigTimeout.ClientTimeout;

	// addr
	cpMacAddr(info->NegotiationResponsePeerDeviceAddress, param->PeerDeviceAddress);

	// token
	info->NegotiationResponseDialogToken = param->DialogToken;

	// status
	info->NegotiationResponseStatus = param->Status;

	// go intent
	info->GOIntent = 
		((param->GroupOwnerIntent.Intent << 1) 
		| param->GroupOwnerIntent.TieBreaker);

	// intf addr
	cpMacAddr(info->InterfaceAddress, param->IntendedInterfaceAddress);

	// gc
	info->NegotiationResponseGroupCapability = param->GroupCapability & ((u1Byte)(~gcP2PGroupOwner)); // Workaround since the pParameters->GroupCapability is wrong

	// grp id
	if(param->bUseGroupID)
	{
		info->bNegotiationResponseUseGroupID = TRUE;

		// dev addr
		cpMacAddr(info->NegotiationResponseGroupIDDeviceAddress, param->GroupID.DeviceAddress);

		// ssid
		info->uNegotiationResponseGroupIDSSIDLength = (u1Byte)param->GroupID.SSID.uSSIDLength;
		PlatformMoveMemory(
			info->NegotiationResponseGroupIDSSID, 
			param->GroupID.SSID.ucSSID,
			param->GroupID.SSID.uSSIDLength
		);
	}
	else
	{
		info->bNegotiationResponseUseGroupID = FALSE;
	}
	
	// add ie
	if(param->uIEsLength)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_GO_NEGOTIATION_RESPONSE,
			param->uIEsLength,
			(u1Byte *)param + param->uIEsOffset
			);
	}

	// Update the state machine immediately 
	if(P2P_SC_SUCCESS == param->Status)
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
n63c_SendAction_GoNegConfirm_PrepareData(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS *param
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	OCTET_STRING				osWpsAttr, osDpid;
	
	//
	// ref: N63C_SET_OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRM
	//

	cpMacAddr(info->NegotiationConfirmPeerDeviceAddress, param->PeerDeviceAddress);
	info->NegotiationConfirmDialogToken = param->DialogToken;
	info->NegotiationConfirmStatus = param->Status;

	info->NegotiationConfirmGroupCapability = param->GroupCapability;
	info->NegotiationConfirmGroupCapability = param->GroupCapability & ((u1Byte)(~gcP2PGroupOwner));		// Workaround since the pParameters->GroupCapability is wrong

	if(param->bUseGroupID)
	{
		info->bNegotiationConfirmUseGroupID = TRUE;
		cpMacAddr(info->NegotiationConfirmGroupIDDeviceAddress, param->GroupID.DeviceAddress);
		info->uNegotiationConfirmGroupIDSSIDLength = (u1Byte)param->GroupID.SSID.uSSIDLength;
		PlatformMoveMemory(
				info->NegotiationConfirmGroupIDSSID, 
				param->GroupID.SSID.ucSSID, 
				info->uNegotiationConfirmGroupIDSSIDLength
			);
	}
	else
	{
		info->bNegotiationConfirmUseGroupID = FALSE;
	}

	// add ie
	if(param->uIEsLength)
	{
		P2P_AddIe_Set(
			&info->AdditionalIEs, 
			P2P_ADD_IE_GO_NEGOTIATION_CONFIRM,
			param->uIEsLength,
			(u1Byte *)param + param->uIEsOffset
			);
	}

	// token
	info->DialogToken = (u1Byte)param->DialogToken;

	return status;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

NDIS_STATUS
N63C_SendAction_TerminateReq(
	IN  ADAPTER					*pAdapter
	)
{
	return OffChnlTx_AbortReq(pAdapter);
}

u1Byte
N63C_SendAction_UpdateTxFrameDialogToken(
	IN  VOID 					*req,
	IN  ADAPTER					*pAdapter,
	IN  N63C_P2P_ACTION_FRAME_TYPE frameType
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
	if(N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST == frameType)
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
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("bProvisionRequestUseGroupID is set but can't find the GO in dev list\n"));
		}
		
		mac = devAddr;
	}
	
	p2p_DevList_TxUpdate(&info->devList, 
			n63c_SendAction_XlatFrameType(frameType), 
			mac, 
			devType, 
			frame, 
			token, 
			OffChnlTx_GetTxChnl(req));

	info->DialogToken = token;

	return token;
}

RT_STATUS
N63C_SendAction_SkipProbeTemporarilly(
	IN	VOID					*req
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	
	RT_ASSERT(req, ("%s(): req is NULL!!!\n", __FUNCTION__));
	
	status = OffChnlTx_SkipProbeTemporarilly(req);

	return status;
}

VOID *
N63C_SendAction_PdReq(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const P2P_DEV_LIST_ENTRY	*pDev = NULL;
	u1Byte						txChnl = 0;
	P2P_DEV_TYPE				devType = P2P_DEV_TYPE_DEV;
	VOID 						*req = NULL;
	N63C_SEND_ACTION_CTX		*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	if(NDIS_STATUS_SUCCESS != (status = n63c_SendAction_PdReq_PrepareData(pAdapter, param)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to prepare data\n"));
		return NULL;
	}

	// retrieve peer channel
	if(param->bUseGroupID)
	{
		pDev = p2p_DevList_GetGo(&info->devList, param->PeerDeviceAddress);
		devType = P2P_DEV_TYPE_GO;
		RT_PRINT_ADDR(COMP_OID_SET, DBG_LOUD, "peer GO bssid:\n", pDev->mac);
	}
	else
	{
		pDev = p2p_DevList_Get(&info->devList, param->PeerDeviceAddress, P2P_DEV_TYPE_DEV);
		devType = P2P_DEV_TYPE_DEV;
	}

	if(NULL == pDev)
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("can't find peer in dev list\n"));
		return NULL;
	}

	if(NULL == pDev->rxFrames[P2P_FID_PROBE_RSP])
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("no probe rsp frame from the device\n"));
		return NULL;
	}

	txChnl = pDev->rxFrames[P2P_FID_PROBE_RSP]->channel;

	// prepare off chnl tx req
	if(NULL == (req = n63c_SendAction_PrepareReq(pAdapter, pDev->mac, txChnl, param->uSendTimeout)))
	{
		return NULL;
	}

	// setup cb
	ctx = (N63C_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	n63c_SendAction_InitCtx(ctx, 
		N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST, 
		N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE, 
		pAdapter, 
		pDev->mac, 
		(P2P_DEV_TYPE_GO == devType) ? TRUE : FALSE,
		param->DialogToken, 
		oldToken,
		param->uSendTimeout, 
		req, 
		P2P_EVENT_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE
		);

	// flush action frames in dev list so we don't drop frames because of dialog token mismatch
	p2p_DevList_FlushActionFrames(&info->devList, pDev->mac, pDev->type);
	
	// construct pd req
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_PDReq(info, frame, param->PeerDeviceAddress, param->DialogToken, 0);

	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_PD_REQ, 
			pDev->mac, 
			pDev->type, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN),
			txChnl);

	// issue req
	n63c_SendAction_IssueReq(pAdapter, req, "n63c-send-p2p-action-frame-pd-req");
	
	return req;
}

VOID *
N63C_SendAction_PdRsp(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	N63C_SEND_ACTION_CTX		*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	// retrieve peer mac/channel
	txMac = param->ReceiverDeviceAddress;
	if(0 == (txChnl = n63c_SendAction_GetTxChnl(info, txMac, P2P_DEV_TYPE_DEV, P2P_FID_PD_REQ)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = n63c_SendAction_PdRsp_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = n63c_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->uSendTimeout)))
	{
		return NULL;
	}
	OffChnlTx_SetPostAckDwellTime(req, 100);
	OffChnlTx_SetProbeEnabled(req, FALSE);
	OffChnlTx_SetTxAttemptCount(req, 1);
	OffChnlTx_SetShtPostAckDwellOnPositivelyAcked(req, TRUE);

	// setup cb
	ctx = (N63C_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	n63c_SendAction_InitCtx(ctx, 
		N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE, 
		N63C_P2P_ACTION_FRAME_MAX_VALUE, 
		pAdapter, 
		param->ReceiverDeviceAddress, 
		FALSE,
		param->DialogToken, 
		oldToken,
		param->uSendTimeout, 
		req, 
		P2P_EVENT_PROVISION_DISCOVERY_RESPONSE_SEND_COMPLETE
		);
	
	// construct confirm
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_PDRsp(info, param->DialogToken, NULL, 0, frame, txMac);
	
	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_PD_RSP, 
			txMac, 
			P2P_DEV_TYPE_DEV, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	n63c_SendAction_IssueReq(pAdapter, req, "n63c-send-p2p-action-pd-rsp");
	
	return req;
}

VOID *
N63C_SendAction_InvitationReq(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_INVITATION_REQUEST_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	N63C_SEND_ACTION_CTX		*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	if(NDIS_STATUS_SUCCESS != (status = n63c_SendAction_InvitationReq_PrepareData(pAdapter, param)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to prepare data\n"));
		return NULL;
	}

	// retrieve peer channel
	if(0 == (txChnl = n63c_SendAction_GetTxChnl(info, param->PeerDeviceAddress, P2P_DEV_TYPE_DEV, P2P_FID_PROBE_RSP)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = n63c_SendAction_PrepareReq(pAdapter, param->PeerDeviceAddress, txChnl, param->uSendTimeout)))
	{
		return NULL;
	}

	// setup cb
	ctx = (N63C_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	n63c_SendAction_InitCtx(ctx, 
		N63C_P2P_ACTION_FRAME_INVITATION_REQUEST, 
		N63C_P2P_ACTION_FRAME_INVITATION_RESPONSE, 
		pAdapter, 
		param->PeerDeviceAddress, 
		FALSE,
		param->DialogToken, 
		oldToken,
		param->uSendTimeout, 
		req, 
		P2P_EVENT_INVITATION_REQUEST_SEND_COMPLETE
		);
	
	// construct pd req
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_InvitationReq(info, frame, param->PeerDeviceAddress, param->DialogToken);

	// flush action frames in dev list so we don't remember old frames from previous connections
	p2p_DevList_FlushActionFrames(&info->devList, param->PeerDeviceAddress, P2P_DEV_TYPE_DEV);

	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_INV_REQ, 
			param->PeerDeviceAddress, 
			P2P_DEV_TYPE_DEV, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	n63c_SendAction_IssueReq(pAdapter, req, "n63c-send-p2p-action-frame-invitation-req");
	
	return req;
}

VOID *
N63C_SendAction_InvitationRsp(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_INVITATION_RESPONSE_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	N63C_SEND_ACTION_CTX		*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	// retrieve peer mac/channel
	txMac = param->ReceiverDeviceAddress;
	if(0 == (txChnl = n63c_SendAction_GetTxChnl(info, txMac, P2P_DEV_TYPE_DEV, P2P_FID_INV_REQ)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = n63c_SendAction_InvitationRsp_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = n63c_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->uSendTimeout)))
	{
		return NULL;
	}
	OffChnlTx_SetPostAckDwellTime(req, 100);
	OffChnlTx_SetProbeEnabled(req, FALSE);
	OffChnlTx_SetTxAttemptCount(req, 1);
	OffChnlTx_SetShtPostAckDwellOnPositivelyAcked(req, TRUE);

	// setup cb
	ctx = (N63C_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	n63c_SendAction_InitCtx(ctx, 
		N63C_P2P_ACTION_FRAME_INVITATION_RESPONSE, 
		N63C_P2P_ACTION_FRAME_MAX_VALUE, 
		pAdapter, 
		param->ReceiverDeviceAddress, 
		FALSE,
		param->DialogToken, 
		oldToken,
		param->uSendTimeout, 
		req, 
		P2P_EVENT_INVITATION_RESPONSE_SEND_COMPLETE
		);
	
	// construct confirm
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_InvitationRsp(info, param->DialogToken, frame, txMac);

	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_INV_RSP, 
			txMac, 
			P2P_DEV_TYPE_DEV,
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	n63c_SendAction_IssueReq(pAdapter, req, "n63c-send-p2p-action-frame-invitation-rsp");
	
	return req;
}

VOID *
N63C_SendAction_GoNegReq(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	N63C_SEND_ACTION_CTX		*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	if(NDIS_STATUS_SUCCESS != (status = n63c_SendAction_GoNegReq_PrepareData(pAdapter, param)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to prepare data\n"));
		return NULL;
	}

	// retrieve peer channel
	if(0 == (txChnl = n63c_SendAction_GetTxChnl(info, param->PeerDeviceAddress, P2P_DEV_TYPE_DEV, P2P_FID_PROBE_RSP)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = n63c_SendAction_PrepareReq(pAdapter, param->PeerDeviceAddress, txChnl, param->uSendTimeout)))
	{
		return NULL;
	}

	// setup cb
	ctx = (N63C_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	n63c_SendAction_InitCtx(ctx, 
		N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST, 
		N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE, 
		pAdapter, 
		param->PeerDeviceAddress, 
		FALSE,
		param->DialogToken, 
		oldToken,
		param->uSendTimeout, 
		req, 
		P2P_EVENT_GO_NEGOTIATION_REQUEST_SEND_COMPLETE
		);
	
	// construct pd req
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_GoNegReq(info, frame, param->PeerDeviceAddress, param->DialogToken);

	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_GO_NEG_REQ, 
			param->PeerDeviceAddress, 
			P2P_DEV_TYPE_DEV, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN),
			txChnl);

	// issue req
	n63c_SendAction_IssueReq(pAdapter, req, "n63c-send-p2p-action-frame-go-neg-req");
	
	return req;
}

VOID *
N63C_SendAction_GoNegRsp(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	N63C_SEND_ACTION_CTX		*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	// retrieve peer mac/channel
	txMac = param->PeerDeviceAddress;
	if(0 == (txChnl = n63c_SendAction_GetTxChnl(info, txMac, P2P_DEV_TYPE_DEV, P2P_FID_GO_NEG_REQ)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = n63c_SendAction_GoNegRsp_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = n63c_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->uSendTimeout)))
	{
		return NULL;
	}
	OffChnlTx_SetProbeEnabled(req, FALSE);
	OffChnlTx_SetTxAttemptCount(req, 1);

	// setup cb
	ctx = (N63C_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	n63c_SendAction_InitCtx(ctx, 
		N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE, 
		N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM, 
		pAdapter, 
		param->PeerDeviceAddress, 
		FALSE,
		param->DialogToken, 
		oldToken,
		param->uSendTimeout, 
		req, 
		P2P_EVENT_GO_NEGOTIATION_RESPONSE_SEND_COMPLETE
		);
	
	// construct confirm
	frame = OffChnlTx_GetTxFrameBuf(req);
	p2p_Construct_GoNegRsp(info, param->DialogToken, frame, txMac);
	
	// update to the p2p dev list
	p2p_DevList_TxUpdate(&info->devList, P2P_FID_GO_NEG_RSP, 
			txMac, 
			P2P_DEV_TYPE_DEV, 
			frame, 
			ReadEF1Byte(FrameBuf_Head(frame) + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN), 
			txChnl);

	// issue req
	n63c_SendAction_IssueReq(pAdapter, req, "n63c-send-p2p-action-frame-go-rsp");
	
	return req;
}

VOID *
N63C_SendAction_GoNegConfirm(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	)
{
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;
	P2P_INFO					*info = GET_P2P_INFO(pAdapter);
	const u1Byte				*txMac = NULL;
	u1Byte						txToken = 0;
	u1Byte						txChnl = 0;
	VOID 						*req = NULL;
	N63C_SEND_ACTION_CTX		*ctx = NULL;
	FRAME_BUF					*frame = NULL;
	u1Byte						oldToken = info->DialogToken;

	// retrieve peer mac/channel
	txMac = param->PeerDeviceAddress;
	if(0 == (txChnl = n63c_SendAction_GetTxChnl(info, txMac, P2P_DEV_TYPE_DEV, P2P_FID_GO_NEG_RSP)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to get tx chnl\n"));
		return NULL;
	}

	// get go neg rsp token, to be used in the go neg conf
	if(0 == (txToken = n63c_SendAction_GetRxActionFrameToken(info, param->PeerDeviceAddress, P2P_DEV_TYPE_DEV, P2P_FID_GO_NEG_RSP)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("failed to get go neg rsp token\n"));
		return NULL;
	}

	// prepare data and set to p2p info
	if(NDIS_STATUS_SUCCESS != (status = n63c_SendAction_GoNegConfirm_PrepareData(pAdapter, param)))
	{
		return NULL;
	}

	// prepare off chnl tx req
	if(NULL == (req = n63c_SendAction_PrepareReq(pAdapter, 
						txMac, 
						txChnl, 
						param->uSendTimeout)))
	{
		return NULL;
	}
	OffChnlTx_SetPostAckDwellTime(req, 100);
	OffChnlTx_SetProbeEnabled(req, FALSE);
	OffChnlTx_SetTxAttemptCount(req, 1);
	OffChnlTx_SetShtPostAckDwellOnPositivelyAcked(req, TRUE);

	// setup cb
	ctx = (N63C_SEND_ACTION_CTX *)OffChnlTx_GetCliRsvdBuf(req);
	OffChnlTx_SetupCbCtx(req, stateCb, ctx);
	n63c_SendAction_InitCtx(ctx, 
		N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM, 
		N63C_P2P_ACTION_FRAME_MAX_VALUE, 
		pAdapter, 
		param->PeerDeviceAddress, 
		FALSE,
		param->DialogToken, 
		oldToken,
		param->uSendTimeout, 
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
	n63c_SendAction_IssueReq(pAdapter, req, "n63c-send-p2p-action-frame-go-confirm");
	
	return req;
}

