#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Build_Mgnt.tmh"
#endif

#if (P2P_SUPPORT == 1)
#include "P2P_Internal.h"

static u1Byte	BroadcastAddress[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

static
VOID
p2p_Make_SupportedRate(
	IN  FRAME_BUF				*pBuf,
	IN  const MGNT_INFO			*pMgntInfo
	)
{
	OCTET_STRING 				SuppRates, ExtSuppRates;
	u1Byte						SuppRatesContent[8]; // NOTE! Length of Support Rates <= 8.
	u1Byte						ExtSuppRatesContent[255]; // NOTE! Length of Support Rates <= 255.

	FillOctetString(SuppRates, SuppRatesContent, 0);
	FillOctetString(ExtSuppRates, ExtSuppRatesContent, 0);

	SelectSupportedRatesElement(pMgntInfo->dot11CurrentWirelessMode, 
		pMgntInfo->SupportedRates,
		TRUE,
		&SuppRates, 
		&ExtSuppRates);

	// Supported Rates
	FrameBuf_Add_u1(pBuf, EID_SupRates);
	FrameBuf_Add_u1(pBuf, (u1Byte)SuppRates.Length);
	FrameBuf_Add_Data(pBuf, SuppRates.Octet, SuppRates.Length);
	
	// Extended Supported Rates
	if(ExtSuppRates.Length != 0)
	{
		FrameBuf_Add_u1(pBuf, EID_ExtSupRates);
		FrameBuf_Add_u1(pBuf, (u1Byte)ExtSuppRates.Length);
		FrameBuf_Add_Data(pBuf, ExtSuppRates.Octet, ExtSuppRates.Length);
	}
}

static
VOID
p2p_build_TimeStamp(
	IN  FRAME_BUF				*pBuf
	)
{
	u8Byte						timeStamp = 0;
	
	timeStamp = PlatformGetCurrentTime();

	FrameBuf_Add_le_u4(pBuf, (u4Byte)(timeStamp & 0xffffffff));
	FrameBuf_Add_le_u4(pBuf, (u4Byte)(timeStamp >> 32));

	return;
}

static
VOID
p2p_build_ProbeReqIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;
		
	if(p2p_ActingAs_Go(pP2PInfo))
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));

	P2PAttr_Make_Capability(pBuf, 
			pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
			0);

	// The P2P Device ID attribute may be present in the 
	// Probe Request frame when using the discovery 
	// protocol to find a P2P Device with a specific Device
	// Address.
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		if(pP2PInfo->ScanDeviceIDs.uNumOfDeviceIDs > 0)
		{
			P2PAttr_Make_DevId(pBuf, pP2PInfo->ScanDeviceIDs.DeviceIDs[0]);
		}
	}

	P2PAttr_Make_ListenChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->ListenChannel);
	P2PAttr_Make_ExtListenTiming(pBuf, pP2PInfo->ExtListenTimingDuration, pP2PInfo->ExtListenTimingPeriod);
	if(P2P_GO == pP2PInfo->Role)
		P2PAttr_Make_OperatingChannel(pBuf, pP2PInfo->CountryString, pP2PInfo->RegulatoryClass, pP2PInfo->OperatingChannel);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_ProbeRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;
	P2P_WPS_ATTRIBUTES 			*pWps = &pP2PInfo->WpsAttributes;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	if(p2p_ActingAs_Go(pP2PInfo))
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));
	P2PAttr_Make_Capability(pBuf, 
			pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
			0);

	P2PAttr_Make_ExtListenTiming(pBuf, pP2PInfo->ExtListenTimingDuration, pP2PInfo->ExtListenTimingPeriod);

	P2PAttr_Make_DevInfo(pBuf, 
		pP2PInfo->DeviceAddress, 
		pWps->ConfigMethod, 
		&pWps->PrimaryDeviceType, 
		pWps->SecondaryDeviceTypeLength, pWps->SecondaryDeviceTypeList,
		pWps->DeviceNameLength, pWps->DeviceName);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_GoProbeRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;
	u1Byte						grpCap = 0;
	const P2P_WPS_ATTRIBUTES 	*pWps = &pP2PInfo->WpsAttributes;
	const ADAPTER 				*pExtAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? pP2PInfo->pAdapter : GetFirstGOPort(pP2PInfo->pAdapter);
	const MGNT_INFO 			*pExtMgntInfo = &pExtAdapter->MgntInfo;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	if(p2p_ActingAs_Go(pP2PInfo))
	{
		grpCap = pP2PInfo->GroupCapability;
		if(p2p_Doing_Provisioning(pP2PInfo))
			SET_FLAG(grpCap, P2P_GROUP_CAP_GROUP_FORMATION);
		if(p2p_Check_GroupLimitReached(pP2PInfo))
			SET_FLAG(grpCap, P2P_GROUP_CAP_GROUP_LIMIT);
	}
	else
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid role\n"));
	}
	
	P2PAttr_Make_Capability(pBuf, 
		pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
		grpCap);

	P2PAttr_Make_ExtListenTiming(pBuf, pP2PInfo->ExtListenTimingDuration, pP2PInfo->ExtListenTimingPeriod);
	if(P2P_GO == pP2PInfo->Role)
		P2PAttr_Make_Noa(pBuf, pP2PInfo->NoAIEIndex, pP2PInfo->bOppPS, pP2PInfo->CTWindow, P2P_MAX_NUM_NOA_DESC, pP2PInfo->NoADescriptors);

	P2PAttr_Make_DevInfo(pBuf, 
		pP2PInfo->DeviceAddress, 
		pWps->ConfigMethod, 
		&pWps->PrimaryDeviceType, 
		pWps->SecondaryDeviceTypeLength, 
		pWps->SecondaryDeviceTypeList,
		pWps->DeviceNameLength,
		pWps->DeviceName);

	if(P2P_GO == pP2PInfo->Role)
		P2PAttr_Make_GroupInfo(pBuf, pExtMgntInfo->AsocEntry);

	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_BeaconIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;
	u1Byte						grpCap = 0;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	if(p2p_ActingAs_Go(pP2PInfo))
	{
		grpCap = pP2PInfo->GroupCapability;
		if(p2p_Doing_Provisioning(pP2PInfo))
			SET_FLAG(grpCap, P2P_GROUP_CAP_GROUP_FORMATION);
		if(p2p_Check_GroupLimitReached(pP2PInfo))
			SET_FLAG(grpCap, P2P_GROUP_CAP_GROUP_LIMIT);
	}
	else
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid role\n"));
	}
	
	P2PAttr_Make_Capability(pBuf, 
		pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
		grpCap);

	P2PAttr_Make_DevId(pBuf, pP2PInfo->DeviceAddress);

	P2PAttr_Make_Noa(pBuf, 
		pP2PInfo->NoAIEIndex, 
		pP2PInfo->bOppPS, 
		pP2PInfo->CTWindow, 
		P2P_MAX_NUM_NOA_DESC, 
		pP2PInfo->NoADescriptors);
	
	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_ManagedIe(
	IN  FRAME_BUF				*pBuf
	)
{
#if P2P_SIMULATE_MANAGED_AP == 1
		pu1Byte					pLen = NULL;

		if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

		P2PAttr_Make_Manageability(pBuf, maP2PDeviceManagement);

		p2p_update_IeHdrLen(pBuf, pLen);
#endif

	return;
}

static
VOID
p2p_build_AssociationReqIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;
	const P2P_WPS_ATTRIBUTES 	*pWps = &pP2PInfo->WpsAttributes;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	if(p2p_ActingAs_Go(pP2PInfo))
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));
	P2PAttr_Make_Capability(pBuf, 
		pP2PInfo->DeviceCapability,
		0);

	P2PAttr_Make_ExtListenTiming(pBuf, pP2PInfo->ExtListenTimingDuration, pP2PInfo->ExtListenTimingPeriod);

	P2PAttr_Make_DevInfo(pBuf, 
		pP2PInfo->DeviceAddress, 
		pWps->ConfigMethod, 
		&pWps->PrimaryDeviceType, 
		pWps->SecondaryDeviceTypeLength, 
		pWps->SecondaryDeviceTypeList,
		pWps->DeviceNameLength,
		pWps->DeviceName);
	
	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_AssociationReqToWlanApIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	if(p2p_ActingAs_Go(pP2PInfo))
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));
	P2PAttr_Make_Capability(pBuf, pP2PInfo->DeviceCapability, pP2PInfo->GroupCapability);

	P2PAttr_Make_Interface(pBuf, pP2PInfo->DeviceAddress, 1, pP2PInfo->InterfaceAddress);
	
	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

static
VOID
p2p_build_AssociationRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	pu1Byte						pLen = NULL;
	u1Byte						grpCap = 0;
	const P2P_WPS_ATTRIBUTES 	*pWps = &pP2PInfo->WpsAttributes;

	if(NULL == (pLen = p2p_add_IEHdr(pBuf))) return;

	P2PAttr_Make_Status(pBuf, pP2PInfo->Status);

	if(!p2p_ActingAs_Go(pP2PInfo))
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Invalid role\n"));

	grpCap = pP2PInfo->GroupCapability;
	if(p2p_Doing_Provisioning(pP2PInfo))
		SET_FLAG(grpCap, P2P_GROUP_CAP_GROUP_FORMATION);
	if(p2p_Check_GroupLimitReached(pP2PInfo))
		SET_FLAG(grpCap, P2P_GROUP_CAP_GROUP_LIMIT);
	
	P2PAttr_Make_Capability(pBuf, 
		pP2PInfo->DeviceCapability & ~P2P_DEV_CAP_CLIENT_DISCOVERABILITY, // this cap valid only in P2P Group Info and AssocReq
		grpCap);

	P2PAttr_Make_ExtListenTiming(pBuf, pP2PInfo->ExtListenTimingDuration, pP2PInfo->ExtListenTimingPeriod);
	
	p2p_update_IeHdrLen(pBuf, pLen);

	return;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

VOID
p2p_Construct_ProbeReqEx(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					ssidLen,
	IN  const u1Byte			*ssidBuf
	)
{
	// MAC Header
	p2p_add_MgntFrameMacHdr(pBuf, Type_Probe_Req, da, pP2PInfo->DeviceAddress, da);

	// SSID
	FrameBuf_Add_u1(pBuf, EID_SsId);
	FrameBuf_Add_u1(pBuf, ssidLen);
	FrameBuf_Add_Data(pBuf, ssidBuf, ssidLen);

	// Supported Rates
	p2p_Make_SupportedRate(pBuf, &(pP2PInfo->pAdapter->MgntInfo));

	// P2P IE
	p2p_build_ProbeReqIe(pBuf, pP2PInfo);

	// Additional IE
	P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_DEFAULT_REQUEST, pBuf);

	// WFDS IE
	P2PSvc_MakeProbeReqIE(pP2PInfo->pP2PSvcInfo, pBuf);

	// WPS IE
	WPS_AppendElement(pP2PInfo->pAdapter, &pBuf->os, TRUE, WPS_INFO_PROBEREQ_IE);

	// WFD IE
	WFD_AppendProbeReqIEs(pP2PInfo->pAdapter, FrameBuf_Cap(pBuf), &pBuf->os);
	
	FrameBuf_Dump(pBuf, 0, FrameBuf_DbgLevel(pBuf), __FUNCTION__);

	return;
}

VOID
p2p_Construct_ProbeReq(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	u1Byte 						ssidLen = P2P_WILDCARD_SSID_LEN;
	u1Byte 						*ssidBuf = P2P_WILDCARD_SSID;
	
	if(P2P_STATE_SCAN == pP2PInfo->State)
	{
		ssidLen = 0;
		ssidBuf = NULL;
	}

	p2p_Construct_ProbeReqEx(pBuf, pP2PInfo, BroadcastAddress, ssidLen, ssidBuf);

	return;
}

BOOLEAN
p2p_Construct_ProbeRsp(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  RT_RFD					*rfdProbeReq,
	IN  OCTET_STRING			*posProbeReq,
	IN  OCTET_STRING			*posAttrs
	)
{
	ADAPTER						*pAdapter = pP2PInfo->pAdapter;
	u1Byte 						ssidLen = P2P_WILDCARD_SSID_LEN;
	u1Byte 						*ssidBuf = P2P_WILDCARD_SSID;
	u1Byte						channel = 0;

	BOOLEAN						bToSendProbeRsp = TRUE;
	
	// MAC Header
	p2p_add_MgntFrameMacHdr(pBuf, Type_Probe_Rsp, da, pP2PInfo->DeviceAddress, pP2PInfo->DeviceAddress);

	// Time Stamp
	// <RJ_TODO> I think this is not necessary, becuase our hw should fill timestamp.
	p2p_build_TimeStamp(pBuf);

	// Beacon Interval
	FrameBuf_Add_le_u2(pBuf, pP2PInfo->pAdapter->MgntInfo.dot11BeaconPeriod);

	// Capability Info
	FrameBuf_Add_le_u2(pBuf, 0);

	// SSID
	if(P2P_GO == pP2PInfo->Role)
	{// TODO: we shall not get here since ProbeRsp of a GO is constructed else where
		ssidLen = (u1Byte)pAdapter->MgntInfo.Ssid.Length;
		ssidBuf = pAdapter->MgntInfo.Ssid.Octet;
	}
	FrameBuf_Add_u1(pBuf, EID_SsId);
	FrameBuf_Add_u1(pBuf, ssidLen);
	FrameBuf_Add_Data(pBuf, ssidBuf, ssidLen);

	// Supported Rates
	p2p_Make_SupportedRate(pBuf, &(pP2PInfo->pAdapter->MgntInfo));

	// DS Parameter
	if(pP2PInfo->State == P2P_STATE_LISTEN)
		channel = pP2PInfo->ListenChannel;
	else if(pP2PInfo->State == P2P_STATE_OPERATING)
		channel = pP2PInfo->OperatingChannel;
	else 
		channel = pP2PInfo->ListenChannel;
	FrameBuf_Add_u1(pBuf, EID_DSParms);
	FrameBuf_Add_u1(pBuf, 1);
	FrameBuf_Add_u1(pBuf, channel);

	// P2P IE
	p2p_build_ProbeRspIe(pBuf, pP2PInfo);

	// Additional IE
	P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_PROBE_RESPONSE, pBuf);

	// WFDS IE
	P2PSvc_MakeProbeRspIE(pP2PInfo->pP2PSvcInfo, posAttrs, pBuf, &bToSendProbeRsp);

	// WPS IE
	WPS_AppendElement(pP2PInfo->pAdapter, &pBuf->os, TRUE, WPS_INFO_PROBERSP_IE);

	// WFD IE
	WFD_AppendProbeRspIEs(pAdapter, FrameBuf_Cap(pBuf), &pBuf->os, rfdProbeReq, posProbeReq);

	// Debug IE
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		FrameBuf_Add_u1(pBuf, EID_Vendor);
		FrameBuf_Add_u1(pBuf, 5);
		FrameBuf_Add_u1(pBuf, 0x00);
		FrameBuf_Add_u1(pBuf, 0xe0);
		FrameBuf_Add_u1(pBuf, 0x4c);
		FrameBuf_Add_le_u2(pBuf, pP2PInfo->ProbeRequestSequenceNum); // TODO: the original code use H2N2BYTE, but it should be LE
	}

	FrameBuf_Dump(pBuf, 0, FrameBuf_DbgLevel(pBuf), __FUNCTION__);

	return bToSendProbeRsp;
}

//-----------------------------------------------------------------------------
// Exported: public
//-----------------------------------------------------------------------------

VOID
P2P_Append_GoProbeRspIe(
	IN  OCTET_STRING			*posFrame,
	IN  ADAPTER					*pAdapter,
	IN  u1Byte					reason,
	IN  RT_RFD					*rfdProbeReq,
	IN  OCTET_STRING			*posProbeReq
	)
{
	FRAME_BUF					fbuf;
	P2P_INFO 					*pP2PInfo = GET_P2P_INFO(pAdapter);

	do
	{
		FrameBuf_Init(FRAME_BUF_CAP_UNKNOWN, posFrame->Length, posFrame->Octet, &fbuf);

		if(P2P_ENABLED(pP2PInfo))
		{
			FRAME_BUF				p2pAttrs;
			BOOLEAN 				bToSendProbeRsp = FALSE;
			
			if(NULL == posProbeReq)
			{// No probe request.
				break;
			}

			// Additional IE
			P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_PROBE_RESPONSE, &fbuf);

			// Clause 3.2.2: GO shall not include P2P IE in ProbeRsp if the ProbeReq does not include a P2P IE
			if(0 == PacketGetElementNum(*posProbeReq, EID_Vendor, OUI_SUB_WIFI_DIRECT, OUI_SUB_DONT_CARE))
			{// No P2P IE, no need to append P2P element in the probe response.
				break;
			}

			// P2P IE
			p2p_build_GoProbeRspIe(&fbuf, pP2PInfo);

			// Get assembled P2P attributes
			p2p_parse_AssembleIe(rfdProbeReq->Buffer.VirtualAddress, rfdProbeReq->PacketLength, OUI_SUB_WIFI_DIRECT, &p2pAttrs);
			
			// WFDS IE
			P2PSvc_MakeProbeRspIE(pP2PInfo->pP2PSvcInfo, &p2pAttrs.os, &fbuf, &bToSendProbeRsp);

			// Free assembled P2P attributes
			p2p_parse_FreeAssembledIe(&p2pAttrs);
		}
		else
		{// P2P Managed Function simulator for test only.
			p2p_build_ManagedIe(&fbuf);
		}
	}while(FALSE);

	// Synch octet string with frame buffer
	FillOctetString(*posFrame, FrameBuf_MHead(&fbuf), FrameBuf_Length(&fbuf));

	FrameBuf_Dump(&fbuf, 0, FrameBuf_DbgLevel(&fbuf), __FUNCTION__);
	
	return;
}

VOID
P2P_Append_BeaconIe(
	IN  ADAPTER					*pAdapter
	)
{
	FRAME_BUF					fbuf;
	P2P_INFO 					*pP2PInfo = GET_P2P_INFO(pAdapter);
	MGNT_INFO					*pMgntInfo = &pAdapter->MgntInfo;

	FrameBuf_Init(FRAME_BUF_CAP_UNKNOWN, pMgntInfo->beaconframe.Length, pMgntInfo->beaconframe.Octet, &fbuf);
		
	if(P2P_ENABLED(pP2PInfo))
	{
		// Additional IE
		P2P_AddIe_Append(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_BEACON, &fbuf);
		
		// P2P IE
		p2p_build_BeaconIe(&fbuf, pP2PInfo);
	}	
	else
	{// P2P Managed Function simulator for test only.
		p2p_build_ManagedIe(&fbuf);
	}

	// Synch octet string with frame buffer
	FillOctetString(pMgntInfo->beaconframe, FrameBuf_MHead(&fbuf), FrameBuf_Length(&fbuf));

	FrameBuf_Dump(&fbuf, 0, FrameBuf_DbgLevel(&fbuf), __FUNCTION__);

	return;
}

VOID
P2P_Append_AssociationReqIe(
	IN  OCTET_STRING			*posFrame,
	IN  ADAPTER					*pAdapter
	)
{
	FRAME_BUF					fbuf;
	P2P_INFO 					*pP2PInfo = GET_P2P_INFO(pAdapter);

	FrameBuf_Init(FRAME_BUF_CAP_UNKNOWN, posFrame->Length, posFrame->Octet, &fbuf);
	FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);
	
	// Append P2P IE
	if(P2P_ENABLED(GET_P2P_INFO(pAdapter)))
	{			
		if(P2PScanListIsGo(pP2PInfo, Frame_pDaddr(*posFrame)))
		{// to P2P GO
			p2p_build_AssociationReqIe(&fbuf, pP2PInfo);
		}
		else
		{// to wlan AP
			p2p_build_AssociationReqToWlanApIe(&fbuf, pP2PInfo);
		}
	}

	// Synch octet string with frame buffer
	FillOctetString(*posFrame, FrameBuf_MHead(&fbuf), FrameBuf_Length(&fbuf));

	FrameBuf_Dump(&fbuf, 0, FrameBuf_DbgLevel(&fbuf), __FUNCTION__);
	
	return;
}


VOID
P2P_Append_AssociationRspIe(
	IN  OCTET_STRING			*posFrame,
	IN  ADAPTER					*pAdapter
	)
{
	FRAME_BUF					fbuf;
	P2P_INFO 					*pP2PInfo = GET_P2P_INFO(pAdapter);
	MGNT_INFO					*pMgntInfo = &pAdapter->MgntInfo;

	FrameBuf_Init(FRAME_BUF_CAP_UNKNOWN, posFrame->Length, posFrame->Octet, &fbuf);
	FrameBuf_SetDbgLevel(&fbuf, DBG_LOUD);
	
	// Append P2P IE
	if(P2P_ENABLED(pP2PInfo))
	{
		p2p_build_AssociationRspIe(&fbuf, pP2PInfo);
	}
	else
	{// P2P Managed Function simulator for test only.
		p2p_build_ManagedIe(&fbuf);
	}

	// Synch octet string with frame buffer
	FillOctetString(*posFrame, FrameBuf_MHead(&fbuf), FrameBuf_Length(&fbuf));

	FrameBuf_Dump(&fbuf, 0, FrameBuf_DbgLevel(&fbuf), __FUNCTION__);

	return;
}

VOID
P2P_Append_ProbeReqIe(
	IN  OCTET_STRING			*posFrame,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	FRAME_BUF					fbuf;
	
	FrameBuf_Init(FRAME_BUF_CAP_UNKNOWN, posFrame->Length, posFrame->Octet, &fbuf);

	if(P2P_ENABLED(pP2PInfo))
	{
		p2p_build_ProbeReqIe(&fbuf, pP2PInfo);

		// Synch octet string with frame buffer
		FillOctetString(*posFrame, FrameBuf_MHead(&fbuf), FrameBuf_Length(&fbuf));
	}
	return;
}

#endif
