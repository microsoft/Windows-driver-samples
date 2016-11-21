#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Build_PublicAction.tmh"
#endif

#include "P2P_Internal.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

static
VOID
p2p_build_GoNegReqIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;
	u1Byte						grpCap = 0;
	P2P_WPS_ATTRIBUTES 			*pWps = &pP2PInfo->WpsAttributes;
	
	u1Byte						intent = 0;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	if(p2p_ActingAs_Go(pP2PInfo))
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		grpCap = (u1Byte)pP2PInfo->NegotiationRequestGroupCapability;
	}
	else
		grpCap = pP2PInfo->GroupCapability;
	
	P2PAttr_Make_Capability(pBuf, 
		pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
		grpCap);

	if(pP2PInfo->ConnectionContext.bProbePeerChannelList)
		intent = 0; // make sure that peer will become the GO, note that the tie breaker bit is 0
	else
		intent = pP2PInfo->GOIntent;
	P2PAttr_Make_GoIntent(pBuf, intent);

	P2PAttr_Make_ConfigTimeout(pBuf, pP2PInfo->GOConfigurationTimeout, pP2PInfo->ClientConfigurationTimeout);
	P2PAttr_Make_ListenChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->ListenChannel);
	P2PAttr_Make_ExtListenTiming(pBuf, pP2PInfo->ExtListenTimingDuration, pP2PInfo->ExtListenTimingPeriod);
	P2PAttr_Make_IntendedIntfAddr(pBuf, pP2PInfo->InterfaceAddress);

	P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pP2PInfo->ChannelEntryList);

	P2PAttr_Make_DevInfo(pBuf, 
		pP2PInfo->DeviceAddress, 
		pWps->ConfigMethod, 
		&pWps->PrimaryDeviceType, 
		pWps->SecondaryDeviceTypeLength, pWps->SecondaryDeviceTypeList,
		pWps->DeviceNameLength, pWps->DeviceName);

	P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->OperatingChannel);
	//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Op Chnl: %d\n", pP2PInfo->OperatingChannel));

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_GoNegRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte 			*da
	)
{
	pu1Byte						pLen = NULL;
	u1Byte						grpCap = 0;
	PP2P_WPS_ATTRIBUTES 		pWps = &pP2PInfo->WpsAttributes;
	
	u1Byte						status = 0;
	u1Byte						intent = 0;
	pu1Byte						pGrpDevAddr = NULL;
	pu1Byte						pGrpSsidBuf = NULL;
	u1Byte						grpSsidLen = 0;

	const P2P_DEV_LIST_ENTRY	*pDev = NULL;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	pDev = p2p_DevList_Find(&pP2PInfo->devList, da, P2P_DEV_TYPE_DEV);
	
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		status = pP2PInfo->NegotiationResponseStatus;
	else
		status = pP2PInfo->Status;
	P2PAttr_Make_Status(pBuf, status);

	if(p2p_ActingAs_Go(pP2PInfo))
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		grpCap = (u1Byte)pP2PInfo->NegotiationResponseGroupCapability;
	}
	else
		grpCap = pP2PInfo->GroupCapability;
	
	P2PAttr_Make_Capability(pBuf, 
		pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
		grpCap);

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		intent = pP2PInfo->GOIntent;
	}
	else 
	{
		// TODO: this is bad doing jobs other than making IE here!!!
		// The tie breaker bit in a GONRsp shall be toggled from the corresponding GONReq
		//
		pP2PInfo->GOIntent = (pP2PInfo->GOIntent | !(pP2PInfo->ConnectionContext.ConnectingDevice.GOIntent & 0x01));
		
		intent = pP2PInfo->GOIntent;
	}

	P2PAttr_Make_GoIntent(pBuf, intent);

	P2PAttr_Make_ConfigTimeout(pBuf, pP2PInfo->GOConfigurationTimeout, pP2PInfo->ClientConfigurationTimeout);
	P2PAttr_Make_IntendedIntfAddr(pBuf, pP2PInfo->InterfaceAddress);

	if(pP2PInfo->ConnectionContext.bGoingToBeGO)
		P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pDev->p2p->commonChannels);
	else
		P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pP2PInfo->ChannelEntryList);

	P2PAttr_Make_DevInfo(pBuf, 
		pP2PInfo->DeviceAddress, 
		pWps->ConfigMethod, 
		&pWps->PrimaryDeviceType, 
		pWps->SecondaryDeviceTypeLength, pWps->SecondaryDeviceTypeList,
		pWps->DeviceNameLength, pWps->DeviceName);

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		if(pP2PInfo->bNegotiationResponseUseGroupID)
		{
			pGrpDevAddr = pP2PInfo->NegotiationResponseGroupIDDeviceAddress;
			pGrpSsidBuf = pP2PInfo->NegotiationResponseGroupIDSSID;
			grpSsidLen = pP2PInfo->uNegotiationResponseGroupIDSSIDLength;
		}
	}
	else
	{
		if(pP2PInfo->ConnectionContext.bGoingToBeGO &&
			P2P_STATUS_SUCCESS == pP2PInfo->ConnectionContext.Status)
		{
			pGrpDevAddr = pP2PInfo->DeviceAddress;
			pGrpSsidBuf = pP2PInfo->SSIDBuf;
			grpSsidLen = pP2PInfo->SSIDLen;
		}
	}

	if(pGrpDevAddr)
		P2PAttr_Make_GroupId(pBuf, pGrpDevAddr, pGrpSsidBuf, grpSsidLen);
	
	//
	// Going to be GO:
	// The Operating Channel attribute shall indicate the intended Operating Channel 
	// of the P2P Group. The channel indicated in the Operating Channel attribute shall 
	// be one of the channels in the Channel List attribute in the GO Negotiation Response 
	// frame.
	//
	// Going to be Client:
	// The Operating Channel attribute may indicate a preferred Operating Channel of the 
	// P2P Group, or may be omitted. Any channel indicated in the Operating Channel attribute 
	// shall be one of the channels in the Channel List attribute in the GO Negotiation Response frame.
	//
	P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->OperatingChannel);

	p2p_update_IeHdrLen(pBuf, pLen);
	
	return;
}

static
VOID
p2p_build_GoNegConfIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte 			*da
	)
{
	pu1Byte						pLen = NULL;
	u1Byte						grpCap = 0;

	u1Byte 						status = 0;
	pu1Byte						pGrpDevAddr = NULL;
	pu1Byte						pGrpSsidBuf = NULL;
	u1Byte						grpSsidLen = 0;
	u1Byte						opChannel = 0;

	const P2P_DEV_LIST_ENTRY	*pDev = NULL;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	pDev = p2p_DevList_Find(&pP2PInfo->devList, da, P2P_DEV_TYPE_DEV);

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		status = pP2PInfo->NegotiationConfirmStatus;
	else
		status = pP2PInfo->Status;
		P2PAttr_Make_Status(pBuf, pP2PInfo->Status);

	if(p2p_ActingAs_Go(pP2PInfo))
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		grpCap = (u1Byte)pP2PInfo->NegotiationConfirmGroupCapability;
	}
	else
		grpCap = pP2PInfo->GroupCapability;
	
	P2PAttr_Make_Capability(pBuf, 
		pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
		grpCap);

	P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pDev->p2p->commonChannels);

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		if(pP2PInfo->bNegotiationConfirmUseGroupID)
		{
			pGrpDevAddr = pP2PInfo->NegotiationConfirmGroupIDDeviceAddress;
			pGrpSsidBuf = pP2PInfo->NegotiationConfirmGroupIDSSID;
			grpSsidLen = pP2PInfo->uNegotiationConfirmGroupIDSSIDLength;
		}
	}
	else
	{
		if(pP2PInfo->ConnectionContext.bGoingToBeGO &&
			P2P_STATUS_SUCCESS == pP2PInfo->ConnectionContext.Status)
		{
			pGrpDevAddr = pP2PInfo->DeviceAddress;
			pGrpSsidBuf = pP2PInfo->SSIDBuf;
			grpSsidLen = pP2PInfo->SSIDLen;
		}
	}
	
	if(pGrpDevAddr)
		P2PAttr_Make_GroupId(pBuf, pGrpDevAddr, pGrpSsidBuf, grpSsidLen);

	//
	// Going to be GO: 
	// The Operating Channel attribute shall indicate the intended Operating Channel of the 
	// P2P Group. The channel indicated in the Operating Channel attribute shall be one of the 
	// channels in the Channel List attribute in the GO Negotiation Confirmation frame.
	//
	// Going to be Client:
	// The Operating Channel attribute in the GO Negotiation Confirmation frame shall be the 
	// Operating Channel attribute from the GO Negotiation Response frame.
	//
	if(pP2PInfo->ConnectionContext.bGoingToBeGO)
	{
		opChannel = pP2PInfo->OperatingChannel;
	}
	else
	{//Chnl from the GONRsp
		opChannel = pP2PInfo->ConnectionContext.ConnectingDevice.OperatingChannel;
	}

	P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, opChannel);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_InvitationReqIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte 			*da
	)
{
	pu1Byte						pLen = NULL;
	PP2P_WPS_ATTRIBUTES 		pWps = &pP2PInfo->WpsAttributes;

	u1Byte 						goTimeout = 0;
	u1Byte 						cliTimeout = 0;
	u1Byte						opChannel = 0;
	u1Byte						invitFlag = 0;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	if(pP2PInfo->InvitationContext.bPersistentInvitation)
	{// in persistent case, we have to tell the peer our conf time
		// No matter GO or client, we fill them with the max value
		goTimeout = pP2PInfo->GOConfigurationTimeout;
		cliTimeout = pP2PInfo->ClientConfigurationTimeout;
	}
	else 
	{// otherwise, we're either a GO or client in operating phase, wo we don't have to fill the conf time
		// Ref Clause 3.1.5.1, a normal invitation always have them to be 0
		goTimeout = 0;
		cliTimeout = 0;
	}
	P2PAttr_Make_ConfigTimeout(pBuf, goTimeout, cliTimeout);

	P2PAttr_Make_GroupBssid(pBuf, pP2PInfo->InvitationContext.GroupBssid);

	P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pP2PInfo->ChannelEntryList);

	P2PAttr_Make_DevInfo(pBuf, 
		pP2PInfo->DeviceAddress, 
		pWps->ConfigMethod, 
		&pWps->PrimaryDeviceType, 
		pWps->SecondaryDeviceTypeLength, 
		pWps->SecondaryDeviceTypeList,
		pWps->DeviceNameLength,
		pWps->DeviceName);		

	P2PAttr_Make_GroupId(pBuf, 
		pP2PInfo->InvitationContext.GODeviceAddress, 
		pP2PInfo->InvitationContext.SsidBuf, 
		pP2PInfo->InvitationContext.SsidLen);
	
	//
	// GO: 
	// the Operating Channel attribute indicates the Operating Channel of the P2P Group
	//
	// Peristent GO:
	//  the Operating Channel attribute indicates the intended Operating Channel of the P2P Group
	//
	// Client: 
	// an Operating Channel attribute shall also be present, indicating the Operating Channel of the P2P Group
	//
	// Persistent Client:
	// an Operating Channel attribute may be present to indicate a preferred Operating Channel
	//
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter)
		&& 0 != pP2PInfo->uInvitationRequestOperatingChannelNumber
		)
	{
		opChannel = (u1Byte)pP2PInfo->uInvitationRequestOperatingChannelNumber;
	}
	else
	{
		if(pP2PInfo->InvitationContext.bPersistentInvitation)
		{
			if(0 == pP2PInfo->InvitationContext.OpChannel)
			{// follow op ch
				opChannel = pP2PInfo->OperatingChannel;
			}
			else
			{// forced op ch
				opChannel = pP2PInfo->InvitationContext.OpChannel;
			}
		}
		else
		{
			opChannel = pP2PInfo->InvitationContext.OpChannel;
		}
	}
	
	P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, opChannel);

	if(pP2PInfo->InvitationContext.bPersistentInvitation)
		SET_FLAG(invitFlag, P2P_INVITATION_FLAGS_TYPE);
	
	P2PAttr_Make_InvitationFlags(pBuf, invitFlag);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_InvitationRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte 			*da
	)
{
	pu1Byte						pLen = NULL;

	u1Byte						status = 0;
	u1Byte 						goTimeout = 0;
	u1Byte 						cliTimeout = 0;
	u1Byte						opChannel = 0;

	const P2P_DEV_LIST_ENTRY	*pDev = NULL;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	pDev = p2p_DevList_Find(&pP2PInfo->devList, da, P2P_DEV_TYPE_DEV);

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		status = pP2PInfo->InvitationResponseStatus;
	else
		status = pP2PInfo->Status;

	P2PAttr_Make_Status(pBuf, status);

	if(pP2PInfo->InvitationContext.bPersistentInvitation)
	{// in persistent case, we have to tell the peer our conf time
		// No matter GO or client, we fill them with the max value
		goTimeout = pP2PInfo->GOConfigurationTimeout;
		cliTimeout = pP2PInfo->ClientConfigurationTimeout;
	}
	else 
	{// non persistent case
		goTimeout = 0;
		cliTimeout = 0;
	}
	P2PAttr_Make_ConfigTimeout(pBuf, goTimeout, cliTimeout);

	P2PAttr_Make_GroupBssid(pBuf, pP2PInfo->InvitationContext.GroupBssid);

	P2PAttr_Make_ChannelList(pBuf, pP2PInfo, &pDev->p2p->commonChannels);

	//
	// GO or Persisstent GO: 
	// intended Operating Channel
	//
	// Client or Persistent Client (optional): 
	// not specified in the spec
	//
	if(pP2PInfo->InvitationContext.bPersistentInvitation)
	{
		//
		// if GO => intended op chnl, shall be one of the channels in the channel list in the InvitationReq
		// if Client => intended op chnl.
		//
		if(P2P_CLIENT == pP2PInfo->InvitationContext.InvitorRole	// peer is Cli
			&& pP2PInfo->InvitationContext.bPersistentInvitation	// persistent
			)
		{// I'm persistent GO
			opChannel = pP2PInfo->InvitationContext.OpChannel;
		}
		else
		{
			opChannel = pP2PInfo->InvitationContext.InvitedDevice.OperatingChannel;
		}
	}
	else
	{// we are invited
		//
		// Not defined in the spec.
		// Follow the op channel of the peer.
		//
		opChannel = pP2PInfo->InvitationContext.InvitedDevice.OperatingChannel;
	}
	
	P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, opChannel);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_DevDiscReqIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;

	PP2P_DEVICE_DISCOVERABILITY_CONTEXT pDevDiscContext = &pP2PInfo->DeviceDiscoverabilityContext;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	P2PAttr_Make_DevId(pBuf, pP2PInfo->DeviceDiscoverabilityContext.ClientDeviceAddress);
	P2PAttr_Make_GroupId(pBuf, pDevDiscContext->GODeviceAddr, pP2PInfo->SSIDBuf, pP2PInfo->SSIDLen);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_DevDiscRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte					status
	)
{
	pu1Byte						pLen = NULL;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;
	
	P2PAttr_Make_Status(pBuf, status);
	p2p_update_IeHdrLen(pBuf, pLen);
	
	return;
}

static
VOID
p2p_build_PdReqIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;
	PP2P_WPS_ATTRIBUTES 		pWps = &pP2PInfo->WpsAttributes;
	u1Byte						grpCap = 0;

	pu1Byte						pGrpDevAddr = NULL;
	pu1Byte						pGrpSsidBuf = NULL;
	u1Byte						grpSsidLen = 0;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	if(p2p_ActingAs_Go(pP2PInfo))
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		grpCap = (u1Byte)pP2PInfo->ProvisionRequestGroupCapability;
	}
	else
		grpCap = pP2PInfo->GroupCapability;
	
	P2PAttr_Make_Capability(pBuf, 
		pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
		grpCap);

	P2PAttr_Make_DevInfo(pBuf, 
		pP2PInfo->DeviceAddress, 
		pWps->ConfigMethod, 
		&pWps->PrimaryDeviceType, 
		pWps->SecondaryDeviceTypeLength, 
		pWps->SecondaryDeviceTypeList,
		pWps->DeviceNameLength,
		pWps->DeviceName);

	if(pP2PInfo->ProvisionDiscoveryContext.go)
	{// connecting to GO
		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			if(pP2PInfo->bProvisionRequestUseGroupID)
			{
				pGrpDevAddr = pP2PInfo->ProvisionRequestGroupIDDeviceAddress;
				pGrpSsidBuf = pP2PInfo->ProvisionRequestGroupIDSSID;
				grpSsidLen = pP2PInfo->uProvisionRequestGroupIDSSIDLength;
			}
		}
		else
		{
			pGrpDevAddr = pP2PInfo->ProvisionDiscoveryContext.devAddr;
			pGrpSsidBuf = pP2PInfo->ProvisionDiscoveryContext.SsidBuf;
			grpSsidLen = pP2PInfo->ProvisionDiscoveryContext.SsidLen;
		}

		if(pGrpDevAddr)
			P2PAttr_Make_GroupId(pBuf, pGrpDevAddr, pGrpSsidBuf, grpSsidLen);
	}

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_PdRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	/* Nothing to Append */
	return;
}

static
VOID
p2p_add_WpsIeConfigMethods(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					configMethods
	)
{
	pu1Byte						pLen = NULL;
	
	FrameBuf_Add_u1(pBuf, (u1Byte)EID_Vendor);

	pLen = FrameBuf_Add(pBuf, 1);
	
	FrameBuf_Add_be_u4(pBuf, 0x0050F204);

	// Version
	FrameBuf_Add_be_u2(pBuf, P2P_WPS_ATTR_TAG_VERSION);
	FrameBuf_Add_be_u2(pBuf, 1);
	FrameBuf_Add_u1(pBuf, 0x10);

	// Config Method
	FrameBuf_Add_be_u2(pBuf, P2P_WPS_ATTR_TAG_CONFIG_METHODS);
	FrameBuf_Add_be_u2(pBuf, 2);
	FrameBuf_Add_be_u2(pBuf, configMethods);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

VOID
p2p_add_P2PPublicActionHdr(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					subtype,
	IN  u1Byte					dialogToken
	)
{
	RT_TRACE_F(COMP_P2P, pBuf->dbgLevel, ("token: %u\n", dialogToken));
	
	FrameBuf_Add_u1(pBuf, WLAN_ACTION_PUBLIC);
	FrameBuf_Add_u1(pBuf, WLAN_PA_VENDOR_SPECIFIC);
	FrameBuf_Add_be_u4(pBuf, P2P_IE_VENDOR_TYPE);
	FrameBuf_Add_u1(pBuf, subtype);
	FrameBuf_Add_u1(pBuf, dialogToken);

	return;
}

VOID
p2p_Construct_GoNegReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte					dialogToken
	)
{	
	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, da);

	// Action Header
	p2p_add_P2PPublicActionHdr(pBuf, P2P_GO_NEG_REQ, dialogToken);

	// P2P IE
	p2p_build_GoNegReqIe(pBuf, pP2PInfo);

	// Additional IE
	P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_GO_NEGOTIATION_REQUEST, pBuf);

	// WPS IE
	WPS_AppendElement(pP2PInfo->pAdapter, &pBuf->os, TRUE, WPS_INFO_PROBEREQ_IE);

	// WFD IE
	WFD_AppendP2pGoNegReqIEs(pP2PInfo->pAdapter, FrameBuf_Cap(pBuf), &pBuf->os);

	RT_TRACE(COMP_P2P, DBG_LOUD, 
		("%s(): intent = %u, tie breaker: %u, dialog token: %u\n",
		__FUNCTION__,
		pP2PInfo->GOIntent >> 1,
		pP2PInfo->GOIntent & 0x01,
		dialogToken));
	
	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);
	
	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_GoNegRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	)
{
	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, pP2PInfo->DeviceAddress);

	// Action Header
	p2p_add_P2PPublicActionHdr(pBuf, P2P_GO_NEG_RSP, dialogToken);

	// P2P IE
	p2p_build_GoNegRspIe(pBuf, pP2PInfo, da);

	// Additional IE
	P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_GO_NEGOTIATION_RESPONSE, pBuf);

	// WPS IE
	WPS_AppendElement(pP2PInfo->pAdapter, &pBuf->os, TRUE, WPS_INFO_PROBERSP_IE);

	// WFD IE
	WFD_AppendP2pGoNegRspIEs(pP2PInfo->pAdapter, FrameBuf_Cap(pBuf), &pBuf->os);

	RT_TRACE(COMP_P2P, DBG_LOUD, 
		("%s(): intent = %u, tie breaker: %u, dialog token: %u\n",
		__FUNCTION__,
		pP2PInfo->GOIntent >> 1,
		pP2PInfo->GOIntent & 0x01,
		dialogToken));

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);

	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_GoNegConf(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	)
{	
	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, da);

	// Action Header
	p2p_add_P2PPublicActionHdr(pBuf, P2P_GO_NEG_CONF, dialogToken);

	// P2P IE
	p2p_build_GoNegConfIe(pBuf, pP2PInfo, da);

	// Additional IE
	P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_GO_NEGOTIATION_CONFIRM, pBuf);

	// WFD IE
	WFD_AppendP2pGoNegConfirmIEs(pP2PInfo->pAdapter, FrameBuf_Cap(pBuf), &pBuf->os);

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);

	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_InvitationReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken
	)
{
	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, da);

	// Action Header
	p2p_add_P2PPublicActionHdr(pBuf, P2P_INVITATION_REQ, dialogToken);

	// P2P IE
	p2p_build_InvitationReqIe(pBuf, pP2PInfo, da);

	// Additional IE
	P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_INVITATION_REQUEST, pBuf);
	
	/* Below use octet string ONLY */
	{
		WFD_AppendP2pInvitationReqIEs(pP2PInfo->pAdapter, FrameBuf_Cap(pBuf), &pBuf->os);
	}

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);

	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_InvitationRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	)
{
	//
	// Assume that pP2PInfo->Status has been set.
	//
	
	FunctionIn(COMP_P2P);
	
	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, pP2PInfo->DeviceAddress);

	// Action Header
	p2p_add_P2PPublicActionHdr(pBuf, P2P_INVITATION_RSP, dialogToken);

	// P2P IE
	p2p_build_InvitationRspIe(pBuf, pP2PInfo, da);

	// Additional IE
	P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_INVITATION_RESPONSE, pBuf);
	
	// WFD IE
	WFD_AppendP2pInvitationRspIEs(pP2PInfo->pAdapter, FrameBuf_Cap(pBuf), &pBuf->os);

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);

	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_DevDiscReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  const u1Byte			*bssid
	)
{	
	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, bssid, pP2PInfo->DeviceAddress, bssid);

	// Action Header
	p2p_add_P2PPublicActionHdr(pBuf, P2P_DEV_DISC_REQ, dialogToken);

	// P2P IE
	p2p_build_DevDiscReqIe(pBuf, pP2PInfo);

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);
	
	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_DevDiscRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte					status
	)
{	
	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, pP2PInfo->DeviceAddress);

	// Action Header
	p2p_add_P2PPublicActionHdr(pBuf, P2P_DEV_DISC_RSP, dialogToken);

	// P2P IE
	p2p_build_DevDiscRspIe(pBuf, pP2PInfo, status);
	
	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);
	
	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_PDReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte					dialogToken,
	IN  u2Byte					configMethod
	)
{
	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, da);

	// Action Header
	p2p_add_P2PPublicActionHdr(pBuf, P2P_PROV_DISC_REQ, dialogToken);

	// P2P IE
	p2p_build_PdReqIe(pBuf, pP2PInfo);

	// Additional IE
	if(0 == P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_PROVISION_DISCOVERY_REQUEST, pBuf))
	{
		p2p_add_WpsIeConfigMethods(pBuf, configMethod);
	}

	// WFDS IE
	P2PSvc_MakePDReqIE(pP2PInfo->pP2PSvcInfo, pBuf);

	// WFD IE
	WFD_AppendP2pProvDiscoveryReqIEs(pP2PInfo->pAdapter, FrameBuf_Cap(pBuf), &pBuf->os);

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);

	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_Construct_PDRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  OCTET_STRING			*posP2PAttrs,
	IN  u2Byte					configMethod,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	)
{
	FunctionIn(COMP_P2P);

	// MAC Header
	p2p_add_ActionFrameMacHdr(pBuf, da, pP2PInfo->DeviceAddress, pP2PInfo->DeviceAddress);

	// Action Header
	p2p_add_P2PPublicActionHdr(pBuf, P2P_PROV_DISC_RSP, dialogToken);

	// P2P IE
	p2p_build_PdRspIe(pBuf, pP2PInfo);

	// Additional IE
	if(0 == P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_PROVISION_DISCOVERY_RESPONSE, pBuf))
	{
		// TODO: we temporarilly accept all confing method in ProvisionDiscoveryReq
		p2p_add_WpsIeConfigMethods(pBuf, configMethod);
	}

	// WFDS IE
	P2PSvc_MakePDRspIE(pP2PInfo->pP2PSvcInfo, posP2PAttrs, pBuf);

	// WFD IE
	WFD_AppendP2pProvDiscoveryRspIEs(pP2PInfo->pAdapter, FrameBuf_Cap(pBuf), &pBuf->os);

	FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);

	FunctionOut(COMP_P2P);

	return;
}

VOID
p2p_build_FakeInvitationRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte 			*da
	)
{
	p2p_build_InvitationRspIe(pBuf, pP2PInfo, da);

	return;
}

//
// Description:
//	Construct fake provision discovery response frame by the device which is considered as the
//	respondor and fill the content by the previous received information.
// Arguments:
//	[in] pP2PInfo -
//		P2P information context.
//	[in] pRspDev -
//		The device which sends the provision discovery response frame.
//	[in] da -
//		The destination which the response frame is sent to.
//	[out] -pBuf
//		The context of FRAME_BUF to put the frame.
// Return:
//	Return RT_STATUS_SUCCESS if the construction of this response frame succeeds.
// By Bruce, 2015-02-17.
//
RT_STATUS
p2p_Construct_FakePDRsp(
	IN	P2P_INFO				*pP2PInfo,
	IN  P2P_DEV_LIST_ENTRY		*pRspDev,
	IN  const u1Byte 			*da,
	OUT  FRAME_BUF 				*pBuf
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	pu1Byte			pPdReqFrame = NULL;
	OCTET_STRING	osPdReq, osTmpIe;
	pu1Byte			pPdRspTa = NULL;;
	
	FunctionIn(COMP_P2P);

	do
	{
		if(!pRspDev)
		{
			RT_ASSERT(FALSE, ("pRspDev = NULL\n"));
			rtStatus = RT_STATUS_INVALID_PARAMETER;
			break;
		}
		
		if(!pRspDev->txFrames[P2P_FID_PD_REQ])
		{
			RT_ASSERT(FALSE, ("pRspDev->txFrames[P2P_FID_PD_REQ] = NULL\n"));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(0 == pRspDev->txFrames[P2P_FID_PD_REQ]->frameLen)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("pRspDev->txFrames[P2P_FID_PD_REQ].frameLen = 0\n"));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		FillOctetString(osPdReq, pRspDev->txFrames[P2P_FID_PD_REQ]->frame, pRspDev->txFrames[P2P_FID_PD_REQ]->frameLen);

		pPdReqFrame = pRspDev->txFrames[P2P_FID_PD_REQ]->frame;
		pPdRspTa = Frame_pDaddr(osPdReq);

		// MAC Header
		p2p_add_ActionFrameMacHdr(pBuf, da, pPdRspTa, pPdRspTa);

		// Action Header
		p2p_add_P2PPublicActionHdr(pBuf, P2P_PROV_DISC_RSP, pRspDev->txFrames[P2P_FID_PD_REQ]->token);

		// WPS IE, getting from the original PD request
		osTmpIe = PacketGetElement(osPdReq, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUB_DONT_CARE);
		if(osTmpIe.Length > 0)
		{
			PacketMakeElement(&(pBuf->os), EID_Vendor, osTmpIe);
		}
		
		// WFD IE
		if(pRspDev->rxFrames[P2P_FID_PROBE_RSP]->frameLen > 0)
		{
			osTmpIe = PacketGetElement(osPdReq, EID_Vendor, OUI_SUB_WIFI_DISPLAY, OUI_SUB_DONT_CARE);
			if(osTmpIe.Length > 0)
			{
				PacketMakeElement(&(pBuf->os), EID_Vendor, osTmpIe);
			}			
		}
		FrameBuf_Dump(pBuf, 0, DBG_LOUD, __FUNCTION__);
	}while(FALSE);	

	FunctionOut(COMP_P2P);

	return rtStatus;
}

#endif

