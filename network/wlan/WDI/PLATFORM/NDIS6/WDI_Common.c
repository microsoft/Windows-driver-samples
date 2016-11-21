#include "MP_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "WDI_Common.tmh"
#endif

#if (WDI_SUPPORT == 1)

VOID
wdi_UnsolicitedIndication(
	IN	PADAPTER		pAdapter,
	IN	BOOLEAN			bAdapterObj,
	IN	PVOID			pRequestId,
	IN	ULONG			Indication,
	IN	NDIS_STATUS		status,
	IN	PUCHAR			pInput,
	IN	ULONG			Length
	)
{
	PRT_GEN_TEMP_BUFFER	pGenBuffer = NULL;
	pu1Byte					pIndicateBuffer = NULL;
	NDIS_STATUS_INDICATION	*indic = NULL;
	WDI_MESSAGE_HEADER	*wdiHdr = NULL;
	NDIS_OID_REQUEST		*req = NULL;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("==>wdi_UnsolicitedIndication(): status indication code = %x, length=%d\n", Indication, Length));
	
	indic = (NDIS_STATUS_INDICATION *)pInput;
	wdiHdr = (WDI_MESSAGE_HEADER *)(pInput + sizeof(*indic));

	// indication data
	PlatformZeroMemory(indic, sizeof(*indic));
	indic->Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
	indic->Header.Size = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
	indic->Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
	indic->RequestId = pRequestId;
	indic->SourceHandle = pAdapter;
	indic->PortNumber = 0;
	indic->StatusCode = Indication;
	indic->StatusBuffer = wdiHdr;
	indic->StatusBufferSize = Length - sizeof(*indic);

	// wdi message header
	PlatformZeroMemory(wdiHdr, sizeof(*wdiHdr));
	if( bAdapterObj == TRUE )
	{
		wdiHdr->PortId = WDI_PORT_ANY;
	}
	else
	{
		wdiHdr->PortId = (WDI_PORT_ID)GET_PORT_NUMBER(pAdapter);
	}
	wdiHdr->Reserved = 0;
	wdiHdr->Status = status;
	wdiHdr->TransactionId = 0;
	wdiHdr->IhvSpecificId = 0;

	NdisMIndicateStatusEx(
		pAdapter->pNdisCommon->hNdisAdapter,
		indic
		);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== wdi_UnsolicitedIndication()\n"));
}

//
// The following helper functions are used to determined if a 
// BSS has the capability of specific wireless mode.
// 2005.01.13, by rcnjko.
//
BOOLEAN
WDIWithWirelessB(
	PRT_WLAN_BSS	pRtBss)
{
	u2Byte i;
	u1Byte rate;

	// Check channels.
	if(pRtBss->ChannelNumber > 14)
	{
		return FALSE;
	}

	// Check supported rates.
	for(i = 0; i < pRtBss->bdSupportRateEXLen; i++)
	{
		rate = pRtBss->bdSupportRateEXBuf[i] & 0x7f;
		if( MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_B) )
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN
WDIWithWirelessG(
	PRT_WLAN_BSS	pRtBss)
{
	u2Byte i;
	u1Byte rate;

	// Check channels.
	if(pRtBss->ChannelNumber > 14)
	{
		return FALSE;
	}
	
	// Check supported rates.
	for(i = 0; i < pRtBss->bdSupportRateEXLen; i++)
	{
		rate = pRtBss->bdSupportRateEXBuf[i] & 0x7f;
		if( MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_G) &&
			!MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_B) )
		{
			return TRUE;
		}
	}

	// Check if it contains ERP information element.
	// <NOTE> Some AP, WAG-302, will not always claim OFDM rates in their ProbeRsp or Beacon 
	// but it is in G mode. 2005.08.23, by rcnjko.
	if(pRtBss->bERPInfoValid)
	{
		return TRUE;
	}	

	return FALSE;
}

BOOLEAN
WDIWithWirelessA(
	PRT_WLAN_BSS	pRtBss)
{
	u2Byte i;
	u1Byte rate;

	// Check channels.
	if(pRtBss->ChannelNumber <= 14 ||
		pRtBss->ChannelNumber > 184 )
	{
		return FALSE;
	}

	// Check supported rates.
	for(i = 0; i < pRtBss->bdSupportRateEXLen; i++)
	{
		rate = pRtBss->bdSupportRateEXBuf[i] & 0x7f;
		if( MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_A) )
		{
			return TRUE;
		}
	}

	return FALSE;
}

////
// This routine determine if we should report the BSS scanned
// to upper layer. 
// 2005.01.13, by rcnjko.
//
BOOLEAN
WDIFilterRtBss(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pRtBss
	)
{
	PRT_NDIS6_COMMON pNdisCommon = Adapter->pNdisCommon;
	BOOLEAN bAcceptable = TRUE;
	BOOLEAN bHasB, bHasG, bHasA; 

	// Determine the capability of wireless mode of the BSS.
	bHasB = WDIWithWirelessB(pRtBss);
	bHasG = WDIWithWirelessG(pRtBss);
	bHasA = WDIWithWirelessA(pRtBss);
	
	// <RJ_TODO> 
	// 1. Maybe we may need to use a table in the future. 
	// 2. Currently, we only consider B/G case.
	if( bHasB && !bHasG && !bHasA )
	{ // The BSS is pure B.
		if( pNdisCommon->RegWirelessMode4ScanList == WIRELESS_MODE_G || 
			pNdisCommon->RegWirelessMode4ScanList == WIRELESS_MODE_A )
		{ // We are pure A or pure G.
			bAcceptable = FALSE;
		}
	}

	if(pRtBss->IE.Length <= sizeof(RT_FIXED_IE_FIELD))
		bAcceptable = FALSE;

	return bAcceptable;
}

static
VOID
wdiext_IndicateP2PDeviceFound(
	IN	ADAPTER		*pAdapter,
	IN	VOID 		*pvDev
	)
{
	WDI_INDICATION_BSS_ENTRY_LIST_PARAMETERS param;
	WDI_BSS_ENTRY_CONTAINER	entry;
	LARGE_INTEGER	CurrentTime;

	P2P_DEV_LIST_ENTRY 			*dev = (P2P_DEV_LIST_ENTRY *)pvDev;
	P2P_FRAME_INFO				*finfo = NULL;

	u1Byte						*buf = NULL;
	ULONG						buflen = 0;

	NDIS_OID_REQUEST			*req = NULL;
	WDI_MESSAGE_HEADER 			*reqWdiHdr = NULL;

	u4Byte						devSpecCtx = 0x07060504;

	// get info of last rx frame
	if(RTIsListEmpty(&dev->rxFrameQ))
		return;
	
	if(!pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest)
		return;

	req = pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest;
	reqWdiHdr = (WDI_MESSAGE_HEADER *)req->DATA.METHOD_INFORMATION.InformationBuffer;

	finfo = (P2P_FRAME_INFO *)RTGetTailList(&dev->rxFrameQ);

	// zero
	PlatformZeroMemory(&param, sizeof(param));
	PlatformZeroMemory(&entry, sizeof(entry));

	// optional
	param.Optional.DeviceDescriptor_IsPresent = TRUE;

	// ArrayOfElementsOfWIFI_BSS_ENTRY_CONTAINER DeviceDescriptor
	param.DeviceDescriptor.ElementCount = 1;
	param.DeviceDescriptor.pElements = &entry;

	// WIFI_MAC_ADDRESS_CONTAINER BSSID
	cpMacAddr(entry.BSSID.Address, finfo->msg->bssid);

	// Get current timestamp
	NdisGetCurrentSystemTime(&CurrentTime);

	// WIFI_BYTE_BLOB ProbeResponseFrame, BeaconFrame
	if(P2P_FID_BEACON == finfo->type)
	{
		entry.Optional.BeaconFrame_IsPresent = TRUE;
		entry.BeaconFrame.ElementCount = finfo->frameLen - sMacHdrLng;
		entry.BeaconFrame.pElements = (UINT8 *)finfo->frame + sMacHdrLng;
	}
	else if(P2P_FID_PROBE_RSP == finfo->type)
	{
		entry.Optional.ProbeResponseFrame_IsPresent = TRUE;
		entry.ProbeResponseFrame.ElementCount = finfo->frameLen - sMacHdrLng;
		entry.ProbeResponseFrame.pElements = (UINT8 *)finfo->frame + sMacHdrLng;
	}
	else
		return;

	// WDI_SIGNAL_INFO_CONTAINER SignalInfo
	entry.SignalInfo.RSSI = (INT32)(((finfo->sigStrength + 1) >> 1) - 95);
	entry.SignalInfo.LinkQuality = finfo->sigStrength;

	// WDI_CHANNEL_INFO_CONTAINER ChannelInfo
	entry.ChannelInfo.ChannelNumber = finfo->channel;
	entry.ChannelInfo.BandId = (finfo->channel < 36) ? WDI_BAND_ID_2400 : WDI_BAND_ID_5000;

	// WDI_BYTE_BLOB DeviceSpecificContext, this is OPTIONAL
	entry.Optional.DeviceSpecificContext_IsPresent = TRUE;
	entry.DeviceSpecificContext.ElementCount = sizeof(devSpecCtx);
	entry.DeviceSpecificContext.pElements = (UINT8 *)&devSpecCtx;

	// WDI_AGE_INFO_CONTAINER EntryAgeInfo
	entry.EntryAgeInfo.HostTimeStamp = CurrentTime.QuadPart;// [SDIO-482] in JIRA
	entry.EntryAgeInfo.CachedInformation = 0;

	// Gen tlv
	if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationBssEntryList(
		&param, 
		sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&buflen, &((UINT8 *)buf)))
		return;

	wdi_UnsolicitedIndication(pAdapter, 
							FALSE,
							(PVOID)req->RequestId,
							NDIS_STATUS_WDI_INDICATION_BSS_ENTRY_LIST,
							NDIS_STATUS_SUCCESS,
							buf,
							buflen);

	// cleanup
	FreeGenerated((UINT8 *)buf);

	return;
}

VOID
wdiext_IndicateP2PActionFrameReceived(
	IN	VOID 			*pvP2PInfo, 
	IN	u4Byte 			eid,
	IN	MEMORY_BUFFER 	*info
	)
{
	P2P_INFO					*pP2PInfo = (P2P_INFO *)pvP2PInfo;
	ADAPTER						*pAdapter = pP2PInfo->pAdapter;

	P2P_EVENT_DATA 				*evtData = (P2P_EVENT_DATA *)info->Buffer;
	u4Byte						ieOffset = FRAME_OFFSET_P2P_PUB_ACT_ELEMENTS;
	RT_STATUS					rtStatus = RT_STATUS_SUCCESS;
	NDIS_STATUS					status = NDIS_STATUS_SUCCESS;

	u1Byte						*buf = NULL;
	ULONG						buflen = 0;

	WDI_INDICATION_P2P_ACTION_FRAME_RECEIVED_PARAMETERS param = {0};

	FunctionIn(COMP_OID_SET);

	// prepare structure, ref N63CIndicateReceivedInvitationResponse
	rtStatus = (ieOffset < evtData->Packet.Length) ? RT_STATUS_SUCCESS : evtData->rtStatus;
	status = NdisStatusFromRtStatus(rtStatus);

	if(NDIS_STATUS_SUCCESS == status)
	{
		param.FrameInfo.Optional.DeviceContext_IsPresent = FALSE;

		if(P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_REQUEST == eid)
			param.FrameInfo.FrameParams.ActionFrameType = WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST;
		else if(P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE == eid)
			param.FrameInfo.FrameParams.ActionFrameType = WDI_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE;
		else if(P2P_EVENT_RECEIVED_GO_NEGOTIATION_REQUEST == eid)
			param.FrameInfo.FrameParams.ActionFrameType = WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST;
		else if(P2P_EVENT_RECEIVED_GO_NEGOTIATION_RESPONSE == eid)
			param.FrameInfo.FrameParams.ActionFrameType = WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE;
		else if(P2P_EVENT_RECEIVED_GO_NEGOTIATION_CONFIRM == eid)
			param.FrameInfo.FrameParams.ActionFrameType = WDI_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM;
		else if(P2P_EVENT_RECEIVED_INVITATION_REQUEST == eid)
			param.FrameInfo.FrameParams.ActionFrameType = WDI_P2P_ACTION_FRAME_INVITATION_REQUEST;
		else if(P2P_EVENT_RECEIVED_INVITATION_RESPONSE == eid)
			param.FrameInfo.FrameParams.ActionFrameType = WDI_P2P_ACTION_FRAME_INVITATION_RESPONSE;
		else
			return;
		
		cpMacAddr(param.FrameInfo.FrameParams.PeerDeviceAddress.Address, evtData->Packet.Buffer + FRAME_OFFSET_ADDRESS2);
		param.FrameInfo.FrameParams.DialogToken = *(evtData->Packet.Buffer + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);
		param.FrameInfo.FrameIEs.ElementCount = evtData->Packet.Length - ieOffset;
		param.FrameInfo.FrameIEs.pElements = evtData->Packet.Buffer + ieOffset;
	}
	
	// gen TLV
	if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationP2pActionFrameReceived(
		&param, 
		sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&buflen, &((UINT8 *)buf)))
		return;

	wdi_UnsolicitedIndication(pAdapter, 
							FALSE,
							NULL,
							NDIS_STATUS_WDI_INDICATION_P2P_ACTION_FRAME_RECEIVED,
							NDIS_STATUS_SUCCESS,
							buf,
							buflen);

	// cleanup
	FreeGenerated((UINT8 *)buf);
	
	FunctionOut(COMP_OID_SET);

	return;
}

VOID
wdiext_IndicateBSSEntry(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO									pMgntInfo = &(pAdapter->MgntInfo);
	WDI_INDICATION_BSS_ENTRY_LIST_PARAMETERS	param;
	WDI_BSS_ENTRY_CONTAINER					entry;
	RT_802_11_BSSID_LIST 						RtBssList = {0};
	PRT_WLAN_BSS								pRtBss = NULL;
	u4Byte										i = 0;
	ULONG										buflen = 0;
	u1Byte										*buf = NULL;
	NDIS_OID_REQUEST							*req = NULL;
	WDI_MESSAGE_HEADER 						*reqWdiHdr = NULL;
	u4Byte										devSpecCtx = 0x07060504;
	LARGE_INTEGER	CurrentTime;
	
	if(!pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest)
		return;

	FunctionIn(COMP_MLME);

	//Get scan list
	RtBssList.NumberOfItems = 0;
	RtBssList.pbssidentry = pAdapter->bssDescList;
	if( (pMgntInfo->bPrepareRoaming == TRUE) && (pMgntInfo->PrepareRoamState ==RT_PREPARE_ROAM_NORMAL_ROAM_BETTER_AP) )
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Skip scan triggered by Roam\n"));
		FunctionOut(COMP_MLME);
		return;
	}
	else
	{
#if BSS_LIST_CACHE
		MgntActQuery_802_11_BSSID_LIST(pAdapter, &RtBssList, FALSE);
#else
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Indicate actual scan result of BSS list\n"));
		MgntActQuery_802_11_BSSID_LIST(pAdapter, &RtBssList, TRUE);
#endif
	}				

	req = pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest;
	reqWdiHdr = (WDI_MESSAGE_HEADER *)req->DATA.METHOD_INFORMATION.InformationBuffer;

	//Indicate BSS entry one by one.
	for (i=0; i<RtBssList.NumberOfItems; i++)
	{
		if(RtBssList.pbssidentry[i].IE.Length <= sizeof(RT_FIXED_IE_FIELD))
		{
			continue;
		}
		pRtBss = &(RtBssList.pbssidentry[i]);

		// Determine if we shall report the BSS found by RegWirelessMode4ScanList.
		if(!WDIFilterRtBss(pAdapter, pRtBss))
		{
			// Skip this BSS.
			continue;
		}

		// zero
		PlatformZeroMemory(&param, sizeof(param));
		PlatformZeroMemory(&entry, sizeof(entry));

		// optional
		param.Optional.DeviceDescriptor_IsPresent = TRUE;

		// ArrayOfElementsOfWIFI_BSS_ENTRY_CONTAINER DeviceDescriptor
		param.DeviceDescriptor.ElementCount = 1;
		param.DeviceDescriptor.pElements = &entry;

		// WIFI_MAC_ADDRESS_CONTAINER BSSID
		cpMacAddr(entry.BSSID.Address, pRtBss->bdBssIdBuf);

		// WIFI_BYTE_BLOB ProbeResponseFrame, BeaconFrame
		if(pRtBss->BssPacketType & BSS_PKT_PROBE_RSP)
		{
			entry.Optional.ProbeResponseFrame_IsPresent = TRUE;
			entry.ProbeResponseFrame.ElementCount = pRtBss->IE.Length;
			entry.ProbeResponseFrame.pElements = (UINT8 *)pRtBss->IE.Octet;
		}
		else if(pRtBss->BssPacketType & BSS_PKT_BEACON)
		{
			entry.Optional.BeaconFrame_IsPresent = TRUE;
			entry.BeaconFrame.ElementCount = pRtBss->IE.Length;
			entry.BeaconFrame.pElements = (UINT8 *)pRtBss->IE.Octet;
		}
		else
		{
			FunctionOut(COMP_MLME);
			return;		
		}

		// WDI_SIGNAL_INFO_CONTAINER SignalInfo
		pRtBss->RecvSignalPower = 0xffffffce;
		entry.SignalInfo.RSSI = (INT32)pRtBss->CumRecvSignalPower;
		entry.SignalInfo.LinkQuality = (UINT32)pRtBss->RSSI;

		// WDI_CHANNEL_INFO_CONTAINER ChannelInfo
		entry.ChannelInfo.ChannelNumber = (UINT32)pRtBss->ChannelNumber;
		entry.ChannelInfo.BandId = (pRtBss->ChannelNumber < 36) ? WDI_BAND_ID_2400 : WDI_BAND_ID_5000;
		RT_TRACE(COMP_MLME, DBG_LOUD, ("pRtBss->ChannelNumber=%d\n", pRtBss->ChannelNumber));

		// WDI_BYTE_BLOB DeviceSpecificContext, this is OPTIONAL
		entry.Optional.DeviceSpecificContext_IsPresent = TRUE;
		entry.DeviceSpecificContext.ElementCount = sizeof(devSpecCtx);
		entry.DeviceSpecificContext.pElements = (UINT8 *)&devSpecCtx;

		NdisGetCurrentSystemTime(&CurrentTime);
		
		// WDI_AGE_INFO_CONTAINER EntryAgeInfo
		entry.EntryAgeInfo.HostTimeStamp = CurrentTime.QuadPart;// [SDIO-482] in JIRA
		entry.EntryAgeInfo.CachedInformation = 0;

		// Gen tlv
		if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationBssEntryList(
			&param, 
			sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
			&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
			&buflen, &((UINT8 *)buf)))
		{
			FunctionOut(COMP_MLME);
			return;
		}
		
		wdi_UnsolicitedIndication(pAdapter, 
								FALSE,
								(PVOID)req->RequestId,
								NDIS_STATUS_WDI_INDICATION_BSS_ENTRY_LIST,
								NDIS_STATUS_SUCCESS,
								buf,
								buflen);

		// cleanup
		FreeGenerated((UINT8 *)buf);
	}

	FunctionOut(COMP_MLME);
	
	return;
}

VOID
wdiext_IndicateBSSEntryBySSID(
	IN	PADAPTER	pAdapter,
	IN	WDI_SSID	*pSSID
	)
{
	PMGNT_INFO									pMgntInfo = &(pAdapter->MgntInfo);
	WDI_INDICATION_BSS_ENTRY_LIST_PARAMETERS	param;
	WDI_BSS_ENTRY_CONTAINER						entry;
	RT_802_11_BSSID_LIST 						RtBssList = {0};
	PRT_WLAN_BSS								pRtBss = NULL;
	u4Byte										i = 0;
	ULONG										buflen = 0;
	u1Byte										*buf = NULL;
	NDIS_OID_REQUEST							*req = NULL;
	WDI_MESSAGE_HEADER 							*reqWdiHdr = NULL;
	u4Byte										devSpecCtx = 0x07060504;
	BOOL										bWildCardSSID = TRUE;
	LARGE_INTEGER	CurrentTime;
	
	if(!pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest)
		return;

	FunctionIn(COMP_MLME);

	if(pSSID->ElementCount != 0)
		bWildCardSSID = FALSE;

	//Get scan list
	RtBssList.NumberOfItems = 0;
	RtBssList.pbssidentry = pAdapter->bssDescList;
	if( (pMgntInfo->bPrepareRoaming == TRUE) && (pMgntInfo->PrepareRoamState ==RT_PREPARE_ROAM_NORMAL_ROAM_BETTER_AP) )
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Skip scan triggered by Roam\n"));
		FunctionOut(COMP_MLME);
		return;
	}
	else
	{
#if BSS_LIST_CACHE
		MgntActQuery_802_11_BSSID_LIST(pAdapter, &RtBssList, FALSE);
#else
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Indicate actual scan result of BSS list\n"));
		MgntActQuery_802_11_BSSID_LIST(pAdapter, &RtBssList, TRUE);
#endif
	}				

	req = pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest;
	reqWdiHdr = (WDI_MESSAGE_HEADER *)req->DATA.METHOD_INFORMATION.InformationBuffer;

	//Indicate BSS entry one by one.
	for (i=0; i<RtBssList.NumberOfItems; i++)
	{
		if(RtBssList.pbssidentry[i].IE.Length <= sizeof(RT_FIXED_IE_FIELD))
		{
			continue;
		}
		pRtBss = &(RtBssList.pbssidentry[i]);

		// Determine if we shall report the BSS found by RegWirelessMode4ScanList.
		if(!WDIFilterRtBss(pAdapter, pRtBss))
		{
			// Skip this BSS.
			continue;
		}

		if(!bWildCardSSID)
		{
			if(RtBssList.pbssidentry[i].bdSsIdLen != pSSID->ElementCount ||
				NdisEqualMemory(RtBssList.pbssidentry[i].bdSsIdBuf, pSSID->pElements, pSSID->ElementCount) == 0)
			{
				//UE specify SSID to update, skip this one
				continue;
			}
		}

		// zero
		PlatformZeroMemory(&param, sizeof(param));
		PlatformZeroMemory(&entry, sizeof(entry));

		// optional
		param.Optional.DeviceDescriptor_IsPresent = TRUE;

		// ArrayOfElementsOfWIFI_BSS_ENTRY_CONTAINER DeviceDescriptor
		param.DeviceDescriptor.ElementCount = 1;
		param.DeviceDescriptor.pElements = &entry;

		// WIFI_MAC_ADDRESS_CONTAINER BSSID
		cpMacAddr(entry.BSSID.Address, pRtBss->bdBssIdBuf);

		// WIFI_BYTE_BLOB ProbeResponseFrame, BeaconFrame
		if(pRtBss->BssPacketType & BSS_PKT_PROBE_RSP)
		{
			entry.Optional.ProbeResponseFrame_IsPresent = TRUE;
			entry.ProbeResponseFrame.ElementCount = pRtBss->IE.Length;
			entry.ProbeResponseFrame.pElements = (UINT8 *)pRtBss->IE.Octet;
		}
		else if(pRtBss->BssPacketType & BSS_PKT_BEACON)
		{
			entry.Optional.BeaconFrame_IsPresent = TRUE;
			entry.BeaconFrame.ElementCount = pRtBss->IE.Length;
			entry.BeaconFrame.pElements = (UINT8 *)pRtBss->IE.Octet;
		}
		else
		{
			FunctionOut(COMP_MLME);
			return;		
		}

		// WDI_SIGNAL_INFO_CONTAINER SignalInfo
		pRtBss->RecvSignalPower = 0xffffffce;
		entry.SignalInfo.RSSI = (INT32)pRtBss->CumRecvSignalPower;
		entry.SignalInfo.LinkQuality = (UINT32)pRtBss->RSSI;

		// WDI_CHANNEL_INFO_CONTAINER ChannelInfo
		entry.ChannelInfo.ChannelNumber = (UINT32)pRtBss->ChannelNumber;
		entry.ChannelInfo.BandId = (pRtBss->ChannelNumber < 36) ? WDI_BAND_ID_2400 : WDI_BAND_ID_5000;
		RT_TRACE(COMP_MLME, DBG_LOUD, ("pRtBss->ChannelNumber=%d\n", pRtBss->ChannelNumber));

		// WDI_BYTE_BLOB DeviceSpecificContext, this is OPTIONAL
		entry.Optional.DeviceSpecificContext_IsPresent = TRUE;
		entry.DeviceSpecificContext.ElementCount = sizeof(devSpecCtx);
		entry.DeviceSpecificContext.pElements = (UINT8 *)&devSpecCtx;

		NdisGetCurrentSystemTime(&CurrentTime);
		
		// WDI_AGE_INFO_CONTAINER EntryAgeInfo
		entry.EntryAgeInfo.HostTimeStamp = CurrentTime.QuadPart;// [SDIO-482] in JIRA
		entry.EntryAgeInfo.CachedInformation = 0;

		// Gen tlv
		if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationBssEntryList(
			&param, 
			sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
			&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
			&buflen, &((UINT8 *)buf)))
		{
			FunctionOut(COMP_MLME);
			return;
		}
		
		wdi_UnsolicitedIndication(pAdapter, 
								FALSE,
								(PVOID)req->RequestId,
								NDIS_STATUS_WDI_INDICATION_BSS_ENTRY_LIST,
								NDIS_STATUS_SUCCESS,
								buf,
								buflen);

		// cleanup
		FreeGenerated((UINT8 *)buf);
	}

	FunctionOut(COMP_MLME);
	
	return;
}


VOID
wdiext_IndicateAssociationResult(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO									pMgntInfo = &(pAdapter->MgntInfo);
	WDI_INDICATION_ASSOCIATION_RESULT_LIST	AsocResultList;
	WDI_ASSOCIATION_RESULT_CONTAINER			AsocResult;
	RT_AUTH_MODE								AuthMode = RT_802_11AuthModeOpen;
	u4Byte										AuthAlgo = 0, UnicastCipher = 0, McastDataCipher = 0, McastMgntCipher = 0;
	u1Byte										FourAddrSupport = FALSE, PortAuthorized = FALSE, WMMQos = FALSE;
	u4Byte										DSInfo = 0, AssocComeBackTime = 0, BandID = 0;
	WDI_PHY_TYPE								PhyType = WDI_PHY_TYPE_UNKNOWN;
	PUCHAR										pOutput = NULL;
	ULONG										length = 0;
	NDIS_STATUS									ndisStatus = NDIS_STATUS_SUCCESS;

	MgntActQuery_802_11_AUTHENTICATION_MODE( pAdapter, &AuthMode );
	AuthAlgo = N6CAuthModeToDot11( &AuthMode );
	UnicastCipher = N6CEncAlgorithmToDot11( &(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm) );
	McastDataCipher = N6CEncAlgorithmToDot11( &(pMgntInfo->SecurityInfo.GroupEncAlgorithm) );
	if( pMgntInfo->bInBIPMFPMode && TEST_FLAG(pMgntInfo->targetAKMSuite, AKM_RSNA_1X_SHA256 | AKM_RSNA_PSK_SHA256))
		McastMgntCipher = WDI_CIPHER_ALGO_BIP;
	else
		McastMgntCipher = WDI_CIPHER_ALGO_NONE;		// PMF ?
	WMMQos = (pMgntInfo->pStaQos->QosCapability > QOS_DISABLE)?TRUE:FALSE;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("wdiext_IndicateAssociationResult WMMQos = %d\n", WMMQos));
	DSInfo = WDI_DS_UNKNOWN;
	if( pMgntInfo->RspStatusCode == 30 && pMgntInfo->TIE.Length != 0)
	{
		if(pMgntInfo->TIE.Octet[0] == 3 )
		{
			AssocComeBackTime = (u4Byte)(pMgntInfo->TIE.Octet[1] & 0xff);
			AssocComeBackTime += (u4Byte)(pMgntInfo->TIE.Octet[2] << 8) & 0xff;
			AssocComeBackTime += (u4Byte)(pMgntInfo->TIE.Octet[3] << 16) & 0xff;
			AssocComeBackTime += (u4Byte)(pMgntInfo->TIE.Octet[4] << 24) & 0xff;
		}
		else
			AssocComeBackTime = 0;
	}
	else 
		AssocComeBackTime = 0;
	if(pMgntInfo->dot11CurrentChannelNumber < 36)
		BandID = WDI_BAND_ID_2400;
	else
		BandID = WDI_BAND_ID_5000;

	switch (pMgntInfo->dot11CurrentWirelessMode)
	{
		default:
			PhyType = WDI_PHY_TYPE_OFDM;
			break;

		case WIRELESS_MODE_A:
			PhyType = WDI_PHY_TYPE_OFDM;
			break;
			
		case WIRELESS_MODE_G:
			PhyType = WDI_PHY_TYPE_ERP;
			break;

		case WIRELESS_MODE_B:
			PhyType = WDI_PHY_TYPE_HRDSSS;
			break;

		case WIRELESS_MODE_N_24G:
		case WIRELESS_MODE_N_5G:
		case WIRELESS_MODE_AUTO:
			PhyType = WDI_PHY_TYPE_HT;
			break;

		case WIRELESS_MODE_AC_5G:
		case WIRELESS_MODE_AC_ONLY:
		case WIRELESS_MODE_AC_24G:
			PhyType = WDI_PHY_TYPE_VHT;
			break;

			
	}

	PlatformZeroMemory(&AsocResult, sizeof(WDI_ASSOCIATION_RESULT_CONTAINER));

	PlatformMoveMemory(AsocResult.BSSID.Address, pMgntInfo->Bssid, 6);
	AsocResultList.AssociationResults.ElementCount = 1;
	AsocResultList.AssociationResults.pElements = &AsocResult;
	AsocResult.AssociationResultParameters.AssociationStatus = (pMgntInfo->mAssoc)?WDI_ASSOC_STATUS_SUCCESS:WDI_ASSOC_STATUS_FAILURE;
	AsocResult.AssociationResultParameters.StatusCode = pMgntInfo->RspStatusCode;
	AsocResult.AssociationResultParameters.ReAssociation = pMgntInfo->AsocInfo.FlagReAsocReq;
	AsocResult.AssociationResultParameters.AuthAlgorithm = AuthAlgo;
	AsocResult.AssociationResultParameters.UnicastCipherAlgorithm = UnicastCipher;
	AsocResult.AssociationResultParameters.MulticastDataCipherAlgorithm = McastDataCipher;
	AsocResult.AssociationResultParameters.MulticastMgmtCipherAlgorithm = McastMgntCipher;
	AsocResult.AssociationResultParameters.FourAddressSupported = FourAddrSupport;
	AsocResult.AssociationResultParameters.PortAuthorized = PortAuthorized;
	AsocResult.AssociationResultParameters.WMMQoSEnabled = WMMQos;
	AsocResult.AssociationResultParameters.DSInfo = DSInfo;
	AsocResult.AssociationResultParameters.AssociationComebackTime = AssocComeBackTime;
	AsocResult.AssociationResultParameters.BandID = BandID;
	AsocResult.Optional.AssociationRequestFrame_IsPresent = TRUE;
	AsocResult.AssociationRequestFrame.ElementCount = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.AsocReqLength);
	AsocResult.AssociationRequestFrame.pElements = MMPDU_BODY(pMgntInfo->AsocInfo.AsocReq);
	AsocResult.Optional.AssociationResponseFrame_IsPresent = TRUE;
	AsocResult.AssociationResponseFrame.ElementCount = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.AsocRespLength);
	AsocResult.AssociationResponseFrame.pElements = MMPDU_BODY(pMgntInfo->AsocInfo.AsocResp);
	AsocResult.Optional.BeaconProbeResponse_IsPresent = TRUE;
	AsocResult.BeaconProbeResponse.ElementCount = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.BeaconLength);
	AsocResult.BeaconProbeResponse.pElements = MMPDU_BODY(pMgntInfo->AsocInfo.Beacon);
	AsocResult.Optional.ActivePhyTypeList_IsPresent = TRUE;
	AsocResult.ActivePhyTypeList.ElementCount = 1;
	AsocResult.ActivePhyTypeList.pElements = &PhyType;

	ndisStatus = GenerateWdiIndicationAssociationResult(
					&AsocResultList, 
					sizeof(NDIS_STATUS_INDICATION)+sizeof(WDI_MESSAGE_HEADER),
					&pAdapter->pPortCommonInfo->WdiData.TlvContext,
					&length,
					&pOutput);
	if(NDIS_STATUS_SUCCESS == ndisStatus)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("GenerateWdiIndicationAssociationResult(): return length %d\n", length));
		RT_PRINT_DATA(COMP_MLME, DBG_TRACE, ("buffer\n"), pOutput, length);
		RT_TRACE(COMP_MLME, DBG_LOUD, ("SendUnsolictedIndication(): EthertypeEncapTable_IsPresent %d\n", AsocResult.Optional.EthertypeEncapTable_IsPresent));
		
		ndisStatus = WDI_AddDatapathPeer(pAdapter, AsocResult.BSSID.Address);
		
		wdi_UnsolicitedIndication(pAdapter, 
								FALSE,
								(PVOID)pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest->RequestId,
								NDIS_STATUS_WDI_INDICATION_ASSOCIATION_RESULT,
								NDIS_STATUS_SUCCESS,
								pOutput,
								length);

		FreeGenerated(pOutput);
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("GenerateWdiIndicationAssociationResult(): generate TLV failure\n"));
	}

	if( NDIS_STATUS_SUCCESS != ndisStatus )
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("wdiext_IndicateAssociationResult: fail to create peer, so disconnect with this AP[%2x:%2x:%2x:%2x:%2x:%2x]\n", AsocResult.BSSID.Address[0],AsocResult.BSSID.Address[1],AsocResult.BSSID.Address[2], AsocResult.BSSID.Address[3], AsocResult.BSSID.Address[4], AsocResult.BSSID.Address[5]));
		MgntDisconnectAP(pAdapter, inactivity);
	}
}

//
// Description:
//	Indicate unsolicited event WDI_INDICATION_FT_ASSOC_PARAMS_NEEDED to WDI.
//
RT_STATUS
wdiext_IndicateFtAssocNeeded(
	IN	PADAPTER	pAdapter	
	)
{
	RT_STATUS											rtStatus = RT_STATUS_SUCCESS;
	NDIS_STATUS											ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO											pMgntInfo = &(pAdapter->MgntInfo);
	WDI_INDICATION_FT_ASSOC_PARAMS_NEEDED_PARAMETERS	param;
	pu1Byte												pIndicBuf = NULL;
	ULONG												indicBufLen = 0;

	PlatformZeroMemory(&param, sizeof(WDI_INDICATION_FT_ASSOC_PARAMS_NEEDED_PARAMETERS));

	do
	{
		if(pMgntInfo->AsocInfo.AuthSeq1Length < sMacHdrLng)
		{
			RT_TRACE_F(COMP_INDIC, DBG_WARNING, ("Too short of AuthSeq2Length = %d\n", pMgntInfo->AsocInfo.AuthSeq1Length));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;			
		}

		if(pMgntInfo->AsocInfo.AuthSeq2Length < sMacHdrLng)
		{
			RT_TRACE_F(COMP_INDIC, DBG_WARNING, ("Too short of AuthSeq2Length = %d\n", pMgntInfo->AsocInfo.AuthSeq2Length));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;			
		}

		cpMacAddr(param.BssId.Address, pMgntInfo->Bssid);
	
		param.AuthRequest.ElementCount = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.AuthSeq1Length);
		param.AuthRequest.pElements = MMPDU_BODY(pMgntInfo->AsocInfo.AuthSeq1);

		param.AuthResponse.ElementCount = MMPDU_BODY_LEN(pMgntInfo->AsocInfo.AuthSeq2Length);
		param.AuthResponse.pElements = MMPDU_BODY(pMgntInfo->AsocInfo.AuthSeq2);

		// Gen tlv
		if(NDIS_STATUS_SUCCESS != (ndisStatus = 
			GenerateWdiIndicationFtAssocParamsNeeded(
				&param, 
				sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
				&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
				&indicBufLen,
				&((UINT8 *)pIndicBuf))))
		{
			RT_TRACE_F(COMP_INDIC, DBG_SERIOUS, ("Fail (ndisStatus = 0x%08X) from GenerateWdiIndicationFtAssocParamsNeeded()\n", ndisStatus));
			rtStatus = RT_STATUS_OS_API_FAILED;
			break;			
		}

		RT_PRINT_DATA(COMP_INDIC, DBG_LOUD, "NDIS_STATUS_WDI_INDICATION_FT_ASSOC_PARAMS_NEEDED:\n", pIndicBuf, indicBufLen);
		wdi_UnsolicitedIndication(pAdapter, 
							FALSE,
							NULL,
							NDIS_STATUS_WDI_INDICATION_FT_ASSOC_PARAMS_NEEDED,
							NDIS_STATUS_SUCCESS,
							pIndicBuf,
							indicBufLen);
	}while(FALSE);

	if(pIndicBuf)
	{
		// cleanup
		FreeGenerated((UINT8 *)pIndicBuf);
	}

	return rtStatus;
}

VOID
wdiext_IndicateDisassociation(
	IN	ADAPTER		*pAdapter,
	IN	u1Byte		*addr,
	IN	u4Byte		frameLen,
	IN	u1Byte		*frame,
	IN	BOOLEAN		bDisassoc,
	IN	u2Byte		reason
	)
{
	WDI_INDICATION_DISASSOCIATION_PARAMETERS param;

	u1Byte						*buf = NULL;
	ULONG						buflen = 0;

	if(0 < frameLen && frameLen < sMacHdrLng)
	{
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("invalid deauth/disassociation len: %u\n", frameLen));
		return;
	}

	PlatformZeroMemory(&param, sizeof(param));

	param.Optional.DeauthFrame_IsPresent = (FALSE == bDisassoc) && frameLen;
	param.Optional.DisassociationFrame_IsPresent = (TRUE == bDisassoc) && frameLen;
	param.Optional.NeedPeerStateCleanup_IsPresent = FALSE;

	cpMacAddr(param.DisconnectIndicationParameters.MacAddress.Address, addr);
	param.DisconnectIndicationParameters.DisassociationWABIReason = bDisassoc 
		? WDI_ASSOC_STATUS_PEER_DISASSOCIATED 
		: WDI_ASSOC_STATUS_PEER_DEAUTHENTICATED;

	if(frameLen)
	{
		if(bDisassoc)
		{
			param.DisassociationFrame.ElementCount = frameLen - sMacHdrLng;
			param.DisassociationFrame.pElements = frame + sMacHdrLng;
		}
		else
		{
			param.DeauthFrame.ElementCount = frameLen - sMacHdrLng;
			param.DeauthFrame.pElements = frame + sMacHdrLng;
		}
	}
	
	// Gen tlv
	if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationDisassociation(
		&param, 
		sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&buflen, &((UINT8 *)buf)))
		return;

	wdi_UnsolicitedIndication(pAdapter, 
							FALSE,
							NULL,
							NDIS_STATUS_WDI_INDICATION_DISASSOCIATION,
							NDIS_STATUS_SUCCESS,
							buf,
							buflen);
	
	// cleanup
	FreeGenerated((UINT8 *)buf);

	return;
}

VOID
wdiext_IndicateRadioStatus(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO									pMgntInfo = &(pAdapter->MgntInfo);
	WDI_INDICATION_RADIO_STATUS_PARAMETERS	Params = {0};
	PUCHAR										pOutput = NULL;
	ULONG										length = 0;
	NDIS_STATUS									ndisStatus = NDIS_STATUS_SUCCESS;
	PWDI_DATA_STRUCT							pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PRT_OID_HANDLER							pOidHandle = &(pWdi->TaskHandle);

	Params.RadioState.SoftwareState = (pMgntInfo->RfOffReason&RF_CHANGE_BY_SW)?0:1;
	Params.RadioState.HardwareState = (pMgntInfo->RfOffReason&RF_CHANGE_BY_HW)?0:1;

	RT_TRACE(COMP_RF, DBG_LOUD, ("wdiext_IndicateRadioStatus(): Params.RadioState.SoftwareState is %d, Params.RadioState.HardwareState is %d\n", Params.RadioState.SoftwareState, Params.RadioState.HardwareState));
	
	ndisStatus = GenerateWdiIndicationRadioStatus(
				&Params,
				sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
				&pAdapter->pPortCommonInfo->WdiData.TlvContext,
				&length,
				&pOutput);

	if(NDIS_STATUS_SUCCESS == ndisStatus)
	{
		wdi_UnsolicitedIndication(pAdapter, 
							TRUE,
							NULL,
							NDIS_STATUS_WDI_INDICATION_RADIO_STATUS,
							NDIS_STATUS_SUCCESS,
							pOutput,
							length);
	
		FreeGenerated(pOutput);
		pOutput = NULL;
	}
	ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
}

VOID
wdiext_IndicateLinkStateChanged(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		bForceLinkQuality,
	IN	u1Byte		ucLinkQuality
	)
{
	PMGNT_INFO										pMgntInfo = &(pAdapter->MgntInfo);
	WDI_INDICATION_LINK_STATE_CHANGE_PARAMETERS	Params = {0};
	PUCHAR											pOutput = NULL;
	ULONG											length = 0;
	NDIS_STATUS										ndisStatus = NDIS_STATUS_SUCCESS;
	
	if (pMgntInfo->mAssoc || pMgntInfo->mIbss)
	{
		Params.LinkStateChangeParameters.LinkQuality = GetSignalQuality(pAdapter);
	}
	else
	{
		Params.LinkStateChangeParameters.LinkQuality = 0;
	}

	if(bForceLinkQuality)
	{
		Params.LinkStateChangeParameters.LinkQuality = ucLinkQuality;
	}

	Params.LinkStateChangeParameters.TxLinkSpeed = MgntActQuery_RT_11N_USER_SHOW_RATES(pAdapter , pMgntInfo->bForcedShowRxRate, FALSE)*1000/2;
	Params.LinkStateChangeParameters.RxLinkSpeed = pMgntInfo->HighestOperaRate*1000/2;
	cpMacAddr(Params.LinkStateChangeParameters.PeerMACAddress.Address, pMgntInfo->Bssid);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("wdiext_IndicateLinkStateChanged(): Params.LinkStateChangeParameters.TxLinkSpeed is %d\n", Params.LinkStateChangeParameters.TxLinkSpeed));
	RT_TRACE(COMP_MLME, DBG_LOUD, ("wdiext_IndicateLinkStateChanged(): Params.LinkStateChangeParameters.RxLinkSpeed is %d\n", Params.LinkStateChangeParameters.RxLinkSpeed));
	
	ndisStatus = GenerateWdiIndicationLinkStateChange(
				&Params,
				sizeof(NDIS_STATUS_INDICATION)+sizeof(WDI_MESSAGE_HEADER),
				&pAdapter->pPortCommonInfo->WdiData.TlvContext,
				&length,
				&pOutput);

	if(NDIS_STATUS_SUCCESS == ndisStatus)
	{
		wdi_UnsolicitedIndication(pAdapter,
								FALSE,
								NULL,
								NDIS_STATUS_WDI_INDICATION_LINK_STATE_CHANGE,
								ndisStatus,
								pOutput,
								length);

		FreeGenerated(pOutput);
		pOutput = NULL;
	}
}

VOID
wdiext_IndicateRoamingNeeded(
	IN	PADAPTER	pAdapter,
	IN	u2Byte		IndicationReason
	)
{
	PMGNT_INFO										pMgntInfo = &(pAdapter->MgntInfo);
	WDI_INDICATION_ROAMING_NEEDED_PARAMETERS		Params = {0};
	PUCHAR											pOutput = NULL;
	ULONG											length = 0;
	NDIS_STATUS										ndisStatus = NDIS_STATUS_SUCCESS;

	pMgntInfo->bPrepareRoaming = TRUE;
	
	Params.RoamingReason = IndicationReason;
	RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Params.RoamingReason %d\n", Params.RoamingReason));

	ndisStatus = GenerateWdiIndicationRoamingNeeded(
				&Params,
				sizeof(NDIS_STATUS_INDICATION)+sizeof(WDI_MESSAGE_HEADER),
				&pAdapter->pPortCommonInfo->WdiData.TlvContext,
				&length,
				&pOutput);

	if(NDIS_STATUS_SUCCESS == ndisStatus)
	{
		wdi_UnsolicitedIndication(pAdapter,
								FALSE,
								NULL,
								NDIS_STATUS_WDI_INDICATION_ROAMING_NEEDED,
								ndisStatus,
								pOutput,
								length);

		FreeGenerated(pOutput);
		pOutput = NULL;
	}
}

static
VOID
wdiext_IndicateP2POpChnl(
	IN  ADAPTER					*pAdapter,
	IN	P2P_INFO				*pP2PInfo
	)
{
	WDI_INDICATION_P2P_GROUP_OPERATING_CHANNEL_PARAMETERS param;

	u1Byte						*buf = NULL;
	ULONG						buflen = 0;

	NDIS_OID_REQUEST			*req = NULL;
	WDI_MESSAGE_HEADER 			*reqWdiHdr = NULL;

	if(!pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest)
		return;

	req = pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest;
	reqWdiHdr = (WDI_MESSAGE_HEADER *)req->DATA.METHOD_INFORMATION.InformationBuffer;

	// zero
	PlatformZeroMemory(&param, sizeof(param));

	// fill
	PlatformMoveMemory(param.Channel.CountryOrRegionString, pP2PInfo->CountryString, sizeof(pP2PInfo->CountryString));
	param.Channel.OperatingClass = pP2PInfo->RegulatoryClass;
	param.Channel.ChannelNumber = pP2PInfo->OperatingChannel;

	param.IndicateReason = WDI_P2P_CHANNEL_INDICATE_REASON_NEW_CONNECTION;
	if(pP2PInfo->bChannelSwitch)
	{
		param.IndicateReason = pP2PInfo->P2PChannelIndicationReason;
	}

	// Gen tlv
	if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationP2pGroupOperatingChannel(
		&param, 
		sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&buflen, &((UINT8 *)buf)))
		return;

	wdi_UnsolicitedIndication(pAdapter, 
							FALSE,
							(PVOID)req->RequestId,
							NDIS_STATUS_WDI_INDICATION_P2P_GROUP_OPERATING_CHANNEL,
							NDIS_STATUS_SUCCESS,
							buf,
							buflen);
	
	// cleanup
	FreeGenerated((UINT8 *)buf);

	return;
}


PVOID
wdi_FindGroupPeerByPort(
	IN	PADAPTER	pAdapter,
	IN	u2Byte		PortNumber
)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PRT_PEER_ENTRY		pPeerEntry = NULL;
	u1Byte				i;
	
	for(i = 0; i < MP_DEFAULT_NUMBER_OF_PORT; i++)
	{
		pPeerEntry = &pDefaultAdapter->RxPeerTable[i];
		if( pPeerEntry->bUsed == TRUE )
		{
			if( pPeerEntry->uPortId == PortNumber )
			{
				return (PVOID)pPeerEntry;
			}
		}
	}
	
	return NULL;
}

PVOID
wdi_FindPeerByAddr(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			pAddr
)
{
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PRT_PEER_ENTRY	pPeerEntry = NULL;
	u1Byte			i = 0, count = 0;
	
	count = pDefaultAdapter->RxPeerUsedCount;
	for(i = MP_DEFAULT_NUMBER_OF_PORT; i < MAX_PEER_NUM; i++)
	{
		if( count == 0 )
			break;
		
		pPeerEntry = &pDefaultAdapter->RxPeerTable[i];
		if( pPeerEntry->bUsed == TRUE )
		{
			count--;	
			if( PlatformCompareMemory(pPeerEntry->PeerAddr, pAddr, 6) == 0 )
			{
				return (PVOID)pPeerEntry;
			}
		}
	}
	
	return NULL;
}

PVOID
wdi_GetAvailablePeer(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			pAddr
)
{
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	WDI_PORT_ID	PortId = (WDI_PORT_ID)pAdapter->pNdis62Common->PortNumber;
	PRT_PEER_ENTRY	pCurPeerEntry = NULL, pPeerEntry = NULL;
	u1Byte			i = 0, UnusedIndex = 0, count = 0;
	BOOLEAN			bFound = FALSE;
	
	FunctionIn(COMP_MLME);
	
	count = pDefaultAdapter->RxPeerUsedCount;
	
	for(i = MP_DEFAULT_NUMBER_OF_PORT; i < MAX_PEER_NUM; i++)
	{
		if( (count == 0) && (bFound == TRUE) )
			break;
		
		pPeerEntry = &pDefaultAdapter->RxPeerTable[i];
		
		if( (pPeerEntry->bUsed == FALSE) && (bFound == FALSE) )
		{
			bFound = TRUE;
			UnusedIndex = i;
		}
		else if( pPeerEntry->bUsed == TRUE )
		{
			count--;
			if( PlatformCompareMemory(pPeerEntry->PeerAddr, pAddr, 6) == 0 )
			{
				RT_TRACE(COMP_MLME, DBG_WARNING, ("wdi_GetAvailablePeer: The addres [%2x:%2x:%2x:%2x:%2x:%2x] had associated with specified Peer %d\n", pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5], pPeerEntry->uPeerId));	
				FunctionOut(COMP_MLME);
				return (PVOID)pPeerEntry;
			}
		}
	}
	
	if( (i == MAX_PEER_NUM) && (bFound == FALSE) )
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("wdi_GetAvailablePeer: Peer is already full, so cannot find an unused peer to associate [%2x:%2x:%2x:%2x:%2x:%2x]\n", pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5]));	
		FunctionOut(COMP_MLME);
		return NULL;
	}
	
	pDefaultAdapter->RxPeerUsedCount++;
	
	pCurPeerEntry = &pDefaultAdapter->RxPeerTable[UnusedIndex];
	pCurPeerEntry->bUsed = TRUE;
	pCurPeerEntry->uPeerId = UnusedIndex;
	pCurPeerEntry->uPortId = (u2Byte)PortId;
	PlatformMoveMemory(pCurPeerEntry->PeerAddr, pAddr, 6);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("wdi_GetAvailablePeer: Port %d associate the addres [%2x:%2x:%2x:%2x:%2x:%2x] at Peer %d\n", (u2Byte)PortId, pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5], UnusedIndex));	
	
	FunctionOut(COMP_MLME);
	
	return (PVOID)pCurPeerEntry;
}

VOID
wdi_NotifyPeerData(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					ExTid,
	IN	u2Byte					Pid,
	IN	RT_RX_INDICATION_LEVEL	level,
	IN	PNDIS_STATUS			pNdisStatus
)
{
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PRT_NDIS6_COMMON	pDefaultNdisCommon = pDefaultAdapter->pNdisCommon;
	PADAPTER			pTempAdapter = NULL;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	WDI_PEER_ID			PeerId = (WDI_PEER_ID)Pid;
	WDI_EXTENDED_TID	ExtendTid = (WDI_EXTENDED_TID)ExTid;
	KIRQL				Irql = 0;
	BOOLEAN				bLackResource = FALSE;
	
	PlatformAcquireSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
	if( (pNdisCommon->bRxControlState != RT_RX_STOP) && (pDefaultNdisCommon->bRxDataPathState != RT_RX_PAUSE) )
	{		
		pNdisCommon->bRxControlState = RT_RX_NOTIFYING;
		PlatformReleaseSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
		
		PlatformAcquireSpinLock(pAdapter, RT_RX_SPINLOCK);
		bLackResource = RxCheckResource(pAdapter)?FALSE:TRUE;
		PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);
		
		if( bLackResource )
			RT_TRACE(COMP_RECV, DBG_WARNING, ("wdi_NotifyPeerData(): RFD is less than lower bound (%d)\n", RFD_LOWER_BOUND));

		Irql =	KeGetCurrentIrql();
		// Prefast warning C6387: '_Param_(5)' could be '0':  this does not adhere to the specification for the function 'RxInorderDataIndication'.
		// False positive, caller gurantee pNdisStatus valid.
#pragma warning( disable:6387 )
		if(Irql > PASSIVE_LEVEL )
		{
			RT_TRACE(COMP_RECV, DBG_TRACE, ("wdi_NotifyPeerData(): it is DISPATCH level\n"));
			if( bLackResource )
				pWdi->WdiDataApi.RxInorderDataIndication(pWdi->DataPathHandle, WDI_RX_INDICATION_DISPATCH_GENERAL_WITH_LOW_RESOURCES, PeerId, ExtendTid, NULL, pNdisStatus);
			else
				pWdi->WdiDataApi.RxInorderDataIndication(pWdi->DataPathHandle, WDI_RX_INDICATION_DISPATCH_GENERAL, PeerId, ExtendTid, NULL, pNdisStatus);
		}
		else
		{
			RT_TRACE(COMP_RECV, DBG_TRACE, ("wdi_NotifyPeerData(): it is PASSIVE level\n"));
			if( bLackResource )
				pWdi->WdiDataApi.RxInorderDataIndication(pWdi->DataPathHandle, WDI_RX_INDICATION_PASSIVE_WITH_LOW_RESOURCES, PeerId, ExtendTid, NULL, pNdisStatus);
			else
				pWdi->WdiDataApi.RxInorderDataIndication(pWdi->DataPathHandle, WDI_RX_INDICATION_PASSIVE, PeerId, ExtendTid, NULL, pNdisStatus);
		}

		PlatformAcquireSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
		if( pNdisCommon->bRxControlState == RT_RX_PREPARE_STOP )
		{
			RT_TRACE(COMP_INIT, DBG_WARNING, ("Driver change Rx control state to stop mode and stop to notify Rx manager data in queue\n"));
			pTempAdapter = GetDefaultAdapter(pAdapter);
			do
			{
				pNdisCommon = pTempAdapter->pNdisCommon;
				if( pNdisCommon->bRxControlState == RT_RX_PREPARE_STOP )
				{
					pNdisCommon->bRxControlState = RT_RX_STOP;
					GLWdiTxRxStatistics.numWdiRxStop++;
				}
				pTempAdapter = GetNextExtAdapter(pTempAdapter);
			}while(pTempAdapter != NULL );
			
			PlatformReleaseSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
			RT_TRACE(COMP_INIT, DBG_WARNING, ("Driver call RxStopConfirm for Rx manager\n"));
			pWdi->WdiDataApi.RxStopConfirm(pWdi->DataPathHandle);
		}
		else if( pNdisCommon->bRxControlState == RT_RX_NOTIFYING )
		{
			pNdisCommon->bRxControlState = RT_RX_NORMAL;
			PlatformReleaseSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
		}
		else
		{
			RT_TRACE(COMP_INIT, DBG_WARNING, ("wdi_NotifyPeerData: Unknown case and RxControlState is %d\n", pNdisCommon->bRxControlState));
			PlatformReleaseSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
		}
	}
	else
	{
		PlatformReleaseSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
		*pNdisStatus = NDIS_STATUS_MEDIA_BUSY;
		RT_TRACE(COMP_RECV, DBG_WARNING, ("Stop to notify Rx manager data in queue\n"));
	}
}

NDIS_STATUS
WDI_Initialize(
	IN	PDRIVER_OBJECT							pDriverObject,
	IN	PUNICODE_STRING							RegistryPath,
	IN	PNDIS_MINIPORT_DRIVER_CHARACTERISTICS	pMChars
	)
{
	NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	NDIS_MINIPORT_DRIVER_WDI_CHARACTERISTICS  WdiChar = {0};

    pMChars->ShutdownHandlerEx				= N6SdioShutdown;//ok
	pMChars->DevicePnPEventNotifyHandler	= N6SdioPnPEventNotify;//ok
	pMChars->UnloadHandler					= N6SdioUnload;//ok
	pMChars->OidRequestHandler				= N6SdioOidRequest;
	pMChars->CancelOidRequestHandler		= N6SdioCancelOidRequest;
	pMChars->SetOptionsHandler				= N6SdioSetOptions;// MPSetOptions;
	pMChars->CheckForHangHandlerEx			= NULL;	// Need check
	pMChars->DirectOidRequestHandler		= N6CDirectOidRequest;
	pMChars->CancelDirectOidRequestHandler	= N6CCancelDirectOidRequest;

	// WorkAround for 0x9F BSOD
	pMChars->ResetHandlerEx 				= N6SdioReset;
	
	RtlZeroMemory(&WdiChar, sizeof(WdiChar));

	WdiChar.WdiVersion				= WDI_VERSION_LATEST;

	WdiChar.Header.Type				= NDIS_OBJECT_TYPE_MINIPORT_WDI_CHARACTERISTICS,
	WdiChar.Header.Size				= NDIS_SIZEOF_MINIPORT_WDI_CHARACTERISTICS_REVISION_1;
	WdiChar.Header.Revision			= NDIS_MINIPORT_DRIVER_WDI_CHARACTERISTICS_REVISION_1;    

	WdiChar.AllocateAdapterHandler  = N6SdioWdi_AllocateAdapter;
	WdiChar.FreeAdapterHandler      = N6SdioWdi_FreeAdapter;
	WdiChar.OpenAdapterHandler      = N6SdioWdi_OpenAdapter;    
	WdiChar.CloseAdapterHandler     = N6SdioWdi_CloseAdapter;    
	WdiChar.TalTxRxInitializeHandler    = N6SdioWdi_TalTxRxInitialize;
	WdiChar.TalTxRxDeinitializeHandler  = N6SdioWdi_TalTxRxDeinitialize;

	WdiChar.HangDiagnoseHandler		= N6SdioWdi_HangDiagnose; 

	// Optional handler
	WdiChar.StartOperationHandler       = N6SdioWdi_StartOperation;
	WdiChar.StopOperationHandler        = N6SdioWdi_StopOperation;

	Status = NdisMRegisterWdiMiniportDriver(
				pDriverObject,
				RegistryPath,
				&GlobalRtDriverContext,
				pMChars,
				&WdiChar,
				&(GlobalRtDriverContext.NdisContext.Ndis6MiniportDriverHandle)
				);
    RT_TRACE(COMP_INIT, DBG_LOUD, ("WDI register WDI miniport! Status 0x%x\n", Status));

	return Status;
}


VOID
WDI_DeInitialize(	
	IN  NDIS_HANDLE			DriverHandle
	)
{
	NdisMDeregisterWdiMiniportDriver(DriverHandle);
}

VOID
WDI_InitRxQueue(	
	IN  PADAPTER		pAdapter
	)
{
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	u2Byte			index = 0;
	PRT_PEER_ENTRY	pPeerEntry = NULL;

	PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	for(index=0; index < MAX_PEER_NUM; index++)
	{
		RTInitializeSListHead( &(pDefaultAdapter->RxPeerQueue[index]) );
	}
	for(index=0; index < MAX_PEER_NUM; index++)
	{
		pPeerEntry = &pDefaultAdapter->RxPeerTable[index];
		PlatformZeroMemory(pPeerEntry, sizeof(RT_PEER_ENTRY));
	}
	pDefaultAdapter->RxPeerUsedCount = 0;
	PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
}

VOID
WDI_DeInitRxQueue(	
	IN  PADAPTER		pAdapter
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	u2Byte				index = 0;
	PNET_BUFFER_LIST	pNetBufferList=NULL;
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], WDI_DeInitRxQueue() ==> \n"));
	PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	for(index=0; index<MAX_PEER_NUM; index++)
	{
		if( !N6CIsNblWaitQueueEmpty(pDefaultAdapter->RxPeerQueue[index]) )
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], Free MDLs/NBLs for Peer ID %d \n", index));
			pNetBufferList = N6CGetHeadNblWaitQueue(pDefaultAdapter->RxPeerQueue[index]);
			RTInitializeSListHead( &(pDefaultAdapter->RxPeerQueue[index]) );
			PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
			WDI_FreeRxFrame(pAdapter, pNetBufferList);
			PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		}
	}
	PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], WDI_DeInitRxQueue() <== \n"));
}


VOID
WDI_PnPNotiry(	
	IN  PADAPTER				pAdapter
	)
{
	PWDI_DATA_STRUCT		pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	
	if( pWdi->TaskHandle.Status & RT_OID_HANDLER_STATUS_SET )
	{
		OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
	}

	if( pWdi->PropertyHandle.Status & RT_OID_HANDLER_STATUS_SET )
	{
		OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_PROPERTY);
	}
}


NDIS_STATUS
WDI_HandleOidRequest(
	IN  NDIS_HANDLE			MiniportAdapterContext,
	IN  PNDIS_OID_REQUEST	pNdisRequest
	)
{
	return N6WdiHandleOidRequest(MiniportAdapterContext, pNdisRequest);
}


VOID
WdiExt_IndicateApAssocReqReceived(
	IN  ADAPTER					*pAdapter,
	IN  u4Byte					assocReqLen,
	IN  u1Byte					*assocReq
	)
{
	WDI_INDICATION_AP_ASSOCIATION_REQUEST_PARAMETERS param;
	OCTET_STRING				osAssocReq = {0};
	u2Byte						ieOffset = 0;

	u1Byte						*buf = NULL;
	ULONG						buflen = 0;

	if(assocReqLen < sMacHdrLng)
	{
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("invalid assoc req len: %u\n", assocReqLen));
		return;
	}

	PlatformZeroMemory(&param, sizeof(param));

	cpMacAddr(param.IncomingRequestInfo.AssocRequestParams.PeerMacAddress.Address, 
		assocReq + FRAME_OFFSET_ADDRESS2
		);

	param.IncomingRequestInfo.AssocRequestParams.IsReassociationRequest = 
		(Type_Reasoc_Req == GET_80211_HDR_TYPE(assocReq)) ? TRUE : FALSE;

	FillOctetString(osAssocReq, assocReq, (u2Byte)assocReqLen);

	if(!PacketGetIeOffset(&osAssocReq, &ieOffset))
	{
		RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("PacketGetIeOffset returns FALSE\n"));
		return;
	}

	param.IncomingRequestInfo.AssocRequestFrame.ElementCount = osAssocReq.Length - sMacHdrLng;
	param.IncomingRequestInfo.AssocRequestFrame.pElements = osAssocReq.Octet + sMacHdrLng;

	// Gen tlv
	if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationApAssociationRequestReceived(
		&param, 
		sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&buflen, &((UINT8 *)buf)))
		return;

	wdi_UnsolicitedIndication(pAdapter, 
							FALSE,
							NULL,
							NDIS_STATUS_WDI_INDICATION_AP_ASSOCIATION_REQUEST_RECEIVED,
							NDIS_STATUS_SUCCESS,
							buf,
							buflen);
	
	// cleanup
	FreeGenerated((UINT8 *)buf);

	return;
}

VOID
WdiExt_IndicateApAssocRspSent(
	IN  ADAPTER					*pAdapter,
	IN  u2Byte					status
	)
{
	RT_OID_HANDLER				*task = &pAdapter->pPortCommonInfo->WdiData.TaskHandle;

	RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("indicate association rsp sent\n"));

	if(FALSE == OidHandle_VerifyTask(pAdapter, OID_WDI_TASK_SEND_AP_ASSOCIATION_RESPONSE))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, 
			("WdiExt_IndicateApAssocRspSent(): skip indication because OidHandle_VerifyTask() returns FALSE\n"));
		return;
	}
	
	if(task->pvCtx)
	{
		if(WDI_CMD_SendApAssociationResponseComplete(
				pAdapter, 
				(NDIS_OID_REQUEST *)task->pvCtx, 
				StatusCode_success == status ? RT_STATUS_SUCCESS : RT_STATUS_FAILURE))
		{// set pvCtx to NULL only when it is freed in WDI_CMD_SendApAssociationResponseComplete
			task->pvCtx = NULL;
		}
	}

	return;
}

RT_STATUS
WDIAllocateMetaData(
	IN	PADAPTER	pAdapter,
	IN	PNET_BUFFER_LIST	pNBL,
	IN	PRT_RFD				pRfd
)
{
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PWDI_FRAME_METADATA	pWdiFrameMeta = NULL;
	
	pWdiFrameMeta = pWdi->WdiDataApi.AllocateWiFiFrameMetaData(pWdi->DataPathHandle);
	if( pWdiFrameMeta == NULL )
		return RT_STATUS_FAILURE;

	pWdiFrameMeta->u.rxMetaData.PayloadType = WDI_FRAME_MSDU;
	pWdiFrameMeta->pNBL = pNBL;
	
	RT_TRACE(COMP_RECV, DBG_TRACE, ("PayloadType %d\n", pWdiFrameMeta->u.rxMetaData.PayloadType));

	MP_SET_PACKET_RFD_RESERVED_0(pNBL, pWdiFrameMeta);
	
	return RT_STATUS_SUCCESS;
}


VOID
WDIFreeMetaData(
	IN	PADAPTER	pAdapter,
	IN	PNET_BUFFER_LIST	pNBL
)
{
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PWDI_FRAME_METADATA	pWdiFrameMeta = NULL;

	pWdiFrameMeta = MP_GET_PACKET_RFD_RESERVED_0(pNBL);

	if( pWdiFrameMeta != NULL )
		pWdi->WdiDataApi.FreeWiFiFrameMetaData(pWdi->DataPathHandle, pWdiFrameMeta);
	else
		return;

	return;
}

u4Byte
WDI_InsertDataInQueue(
	IN	PADAPTER			pAdapter,
	IN	PRT_RFD				pRfd,
	IN	PNET_BUFFER_LIST	pNBL
)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	u2Byte				PortNumber = (u2Byte)pAdapter->pNdis62Common->PortNumber;
	OCTET_STRING		osMpdu = {NULL, 0};
	u4Byte				PeerId = 0;
	PRT_PEER_ENTRY		pPeerEntry = NULL;
	PNET_BUFFER_LIST	pCurrNetBufferList = NULL, pNextNetBufferList = NULL;

	FillOctetString(osMpdu, pRfd->Buffer.VirtualAddress + pRfd->FragOffset, pRfd->FragLength);

	PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	
/****************************************************************************************************
* 20150707 Sinda
* We insert frames in queue as below rules
* 1. Can find the peer with MAC address
* 2. Cannot find the peer with MAC address and insert frames in queue of group peer
****************************************************************************************************/
	pPeerEntry = (PRT_PEER_ENTRY)wdi_FindPeerByAddr(pAdapter, Frame_pTaddr(osMpdu));
	if( pPeerEntry != NULL )
		PeerId = pPeerEntry->uPeerId;
	else
	{
		pPeerEntry = (PRT_PEER_ENTRY)wdi_FindGroupPeerByPort(pAdapter, PortNumber);
		if( pPeerEntry == NULL )
		{
			PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
			return 0;
		}
		else
			PeerId = pPeerEntry->uPeerId;
	}
	
	if(pPeerEntry->bUsed == TRUE)
	{
		RT_TRACE(COMP_RECV, DBG_TRACE, ("WDI_InsertDataInQueue: Port %d, PeerId %d\n", PortNumber, PeerId));
		for (pCurrNetBufferList = pNBL; pCurrNetBufferList != NULL; pCurrNetBufferList = pNextNetBufferList)
		{					
			pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);
			NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList) = NULL;
			N6CAddNblWaitQueue(&pDefaultAdapter->RxPeerQueue[PeerId], pCurrNetBufferList, FALSE);
		}
		PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		return PeerId + 1;
	}
	else
	{
		PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		return 0;
	}
}

BOOLEAN
WDI_NotifyDataInQueue(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					ExTid,
	IN	u2Byte					PeerId,
	IN	RT_RX_INDICATION_LEVEL	level
)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PRT_NDIS6_COMMON	pDefaultNdisCommon = pDefaultAdapter->pNdisCommon;
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;

	RT_TRACE(COMP_RECV, DBG_TRACE, ("WDI_NotifyDataInQueue: PeerId %d, ExTid %d\n", PeerId, ExTid));

	wdi_NotifyPeerData(pAdapter, ExTid, PeerId, level, &status);
	if( status == NDIS_STATUS_PAUSED )
	{
		RT_TRACE_F(COMP_INIT, DBG_WARNING, ("Rx manager pause frame and wait for it to be ready. return value (%d)\n", status));
		PlatformAcquireSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
		pDefaultNdisCommon->bRxDataPathState = RT_RX_PAUSE;
		GLWdiTxRxStatistics.numWdiRxPause++;
		PlatformReleaseSpinLock(pAdapter, RT_RX_CONTROL_SPINLOCK);
		return FALSE;
	}
	else
		return TRUE;
}


VOID
WDI_CompletePacket(
	IN	PADAPTER			pAdapter,
	IN	PNET_BUFFER_LIST	pNBL
)
{
	PWDI_DATA_STRUCT		pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	u2Byte					count = 0;
	WDI_FRAME_ID			FrameID[100] = {0};
	PWDI_FRAME_METADATA	pFrameMeta = NULL;
	BOOLEAN					bIndicateSendComplete = FALSE;

	GLWdiTxRxStatistics.numWdiCompletePkt++;

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], WDI_CompletePacket() ==>\n"));
	// complete only one packet
	pFrameMeta = (PWDI_FRAME_METADATA)pNBL->MiniportReserved[0];
	NET_BUFFER_LIST_NEXT_NBL(pNBL) = NULL;
	NET_BUFFER_LIST_STATUS(pNBL) = NDIS_STATUS_SUCCESS;
	FrameID[count] = pFrameMeta->FrameID;

	if(pFrameMeta->u.txMetaData.bTxCompleteRequired == TRUE)
		bIndicateSendComplete = TRUE;

	pWdi->WdiDataApi.TxTransferCompleteIndication(pWdi->DataPathHandle,WDI_TxFrameStatus_Ok,pNBL);
	if(bIndicateSendComplete)
	{
		pWdi->WdiDataApi.TxSendCompleteIndication(pWdi->DataPathHandle, WDI_TxFrameStatus_Ok, count, FrameID, NULL);
	}
}


VOID
WDI_IndicateScanComplete(
	PADAPTER		pAdapter,
	RT_STATUS		status
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);

	if(FALSE == OidHandle_VerifyTask(pAdapter, OID_WDI_TASK_SCAN))
		return;

	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
}

VOID
WDI_IndicateBssList(
	PADAPTER		pAdapter,
	RT_STATUS		status
	)
{
	wdiext_IndicateBSSEntry(pAdapter);
}

VOID
WDI_IndicateBssListBySSID(
	PADAPTER		pAdapter,
	WDI_SSID		*pSSID
	)
{
	wdiext_IndicateBSSEntryBySSID(pAdapter, pSSID);
}

VOID
WDI_IndicateConnectionComplete(	
	PADAPTER		pAdapter,
	RT_STATUS		status
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);

	if(FALSE == OidHandle_VerifyTask(pAdapter, OID_WDI_TASK_CONNECT))
		return;
	
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
}


VOID
WDI_IndicateAssociationComplete(
	PADAPTER		pAdapter,
	RT_STATUS		status
	)
{	
	if( status == RT_STATUS_SUCCESS )
	{
		wdiext_IndicateAssociationResult(pAdapter);
		wdiext_IndicateLinkStateChanged(pAdapter, FALSE, 0);
	}
}


VOID
WDI_IndicateDisassociation(	
	PADAPTER	pAdapter,
	u2Byte		reason,
	pu1Byte		pAddr
)
{
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);
	
	if(ACTING_AS_AP(pAdapter))
	{// AP mode
		RT_WLAN_STA *sta = pMgntInfo->pCurrentSta;		

		RT_ASSERT(sta, ("%s(): sta is NULL!!!\n", __FUNCTION__));

		if(NULL == sta)
		{
			RT_TRACE_F(COMP_AP, DBG_SERIOUS, ("pMgntInfo->pCurrentSta is NULL!\n"));
			return;
		}
		
		wdiext_IndicateDisassociation(pAdapter, sta->MacAddr, 0, NULL, TRUE, reason);
	}
	else
	{// Client mode
		wdiext_IndicateDisassociation(pAdapter, pMgntInfo->Bssid, 0, NULL, TRUE, reason);
	}

	if( pAddr != NULL )
	{
		WDI_DeleteDatapathPeer(pAdapter, pAddr);
	}
}

VOID
WDI_IndicateNLODiscovery(
	PADAPTER		pAdapter
	)
{
	WDI_INDICATION_NLO_DISCOVERY_PARAMETERS		param;
	PWDI_BSS_ENTRY_CONTAINER					pEntry;	

	PRT_NLO_INFO			pNLOInfo	= &(pAdapter->MgntInfo.NLOInfo);
	PRT_WLAN_BSS			pRtBss 		= NULL;
	RT_802_11_BSSID_LIST 	RtBssList	= {0};	
	u4Byte					i 			= 0;
	u4Byte					j 			= 0;
	u4Byte					nBss		= 0;	
	ULONG					buflen 		= 0;
	pu1Byte					buf 		= NULL;	
	pu1Byte					pBssList	= NULL;	
	u4Byte					devSpecCtx 	= 0x07060504;
	BOOLEAN					Hit			= FALSE;
	LARGE_INTEGER			CurrentTime;

	if(!pAdapter->pPortCommonInfo->WdiData.TaskHandle.pNdisRequest)
		return;

	FunctionIn(COMP_POWER);

	// zero
	PlatformZeroMemory(&param, sizeof(param));
	
	//Get scan list
	RtBssList.NumberOfItems = 0;
	RtBssList.pbssidentry = pAdapter->bssDescList;
	
	MgntActQuery_802_11_BSSID_LIST(pAdapter, &RtBssList, FALSE);

	PlatformAllocateMemoryWithZero(pAdapter, (PVOID *)&pEntry, sizeof(WDI_BSS_ENTRY_CONTAINER)*RtBssList.NumberOfItems);	
	
	param.DeviceDescriptor.pElements = pEntry;
	nBss = 0;


	// Indicate those found BSS which we desired
	for ( i=0 ; i<RtBssList.NumberOfItems ; i++)
	{
		Hit = FALSE;

		if(RtBssList.pbssidentry[i].IE.Length <= sizeof(RT_FIXED_IE_FIELD))
		{
			continue;
		}

		pRtBss = &(RtBssList.pbssidentry[i]);
		
		// Determine if we shall report the BSS found by RegWirelessMode4ScanList.
		if(!WDIFilterRtBss(pAdapter, pRtBss))
		{
			// Skip this BSS.
			continue;
		}
		
		for( j=0 ; j<pNLOInfo->NumDot11OffloadNetwork ; ++j )
		{
			// Compare to check whether it is what we desired or not
			if(!PlatformCompareMemory(pNLOInfo->dDot11OffloadNetworkList[j].ssidbuf, pRtBss->bdSsIdBuf, 32))
			{
				++nBss;
				Hit = TRUE;
			}
		}

		if( !Hit )	// This BSS is not we desired.
			continue;
				
		// WIFI_MAC_ADDRESS_CONTAINER BSSID
		cpMacAddr(pEntry->BSSID.Address, pRtBss->bdBssIdBuf);

		// WIFI_BYTE_BLOB ProbeResponseFrame, BeaconFrame
		if(pRtBss->BssPacketType & BSS_PKT_BEACON)
		{
			pEntry->Optional.BeaconFrame_IsPresent = TRUE;
			pEntry->BeaconFrame.ElementCount = pRtBss->IE.Length;
			pEntry->BeaconFrame.pElements = (UINT8 *)pRtBss->IE.Octet;
		}
		else if(pRtBss->BssPacketType & BSS_PKT_PROBE_RSP)
		{
			pEntry->Optional.ProbeResponseFrame_IsPresent = TRUE;
			pEntry->ProbeResponseFrame.ElementCount = pRtBss->IE.Length;
			pEntry->ProbeResponseFrame.pElements = (UINT8 *)pRtBss->IE.Octet;
		}
		else
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Unknown BssPacketType: %u\n", pRtBss->BssPacketType));
			FunctionOut(COMP_POWER);
			return;		
		}

		// WDI_SIGNAL_INFO_CONTAINER SignalInfo
		pRtBss->RecvSignalPower = 0xffffffce;
		pEntry->SignalInfo.RSSI = (INT32)pRtBss->CumRecvSignalPower;
		pEntry->SignalInfo.LinkQuality = (UINT32)pRtBss->RSSI;

		// WDI_CHANNEL_INFO_CONTAINER ChannelInfo
		pEntry->ChannelInfo.ChannelNumber = (UINT32)pRtBss->ChannelNumber;
		pEntry->ChannelInfo.BandId = (pRtBss->ChannelNumber < 36) ? WDI_BAND_ID_2400 : WDI_BAND_ID_5000;
		RT_TRACE(COMP_POWER, DBG_LOUD, ("pRtBss->ChannelNumber=%d, ", pRtBss->ChannelNumber));
		RT_TRACE(COMP_POWER, DBG_LOUD, ("%x:%x:%x:%x:%x:%x, ", 
			pEntry->BSSID.Address[0], pEntry->BSSID.Address[1], 
			pEntry->BSSID.Address[2], pEntry->BSSID.Address[3], 
			pEntry->BSSID.Address[4], pEntry->BSSID.Address[5]));
		RT_TRACE(COMP_POWER, DBG_LOUD, ("SSID: %s\n", pRtBss->bdSsIdBuf));

		// WDI_BYTE_BLOB DeviceSpecificContext, this is OPTIONAL
		pEntry->Optional.DeviceSpecificContext_IsPresent = TRUE;
		pEntry->DeviceSpecificContext.ElementCount = sizeof(devSpecCtx);
		pEntry->DeviceSpecificContext.pElements = (UINT8 *)&devSpecCtx;

		// Get current timestamp
		NdisGetCurrentSystemTime(&CurrentTime);
		
		// WDI_AGE_INFO_CONTAINER EntryAgeInfo
		pEntry->EntryAgeInfo.HostTimeStamp = CurrentTime.QuadPart;  // [SDIO-482] in JIRA
		pEntry->EntryAgeInfo.CachedInformation = 0;
	}

	if(nBss == 0)
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("WDI_IndicateNLODiscovery(): Error, # of Hit SSID is ZERO.\n"));
		FunctionOut(COMP_POWER);
		return;
	}

	param.DeviceDescriptor.ElementCount = nBss;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("# of BSS Found: %u\n", param.DeviceDescriptor.ElementCount));

	// Gen tlv
	if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationNloDiscovery(
		&param, 
		sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
		&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
		&buflen, &((UINT8 *)buf)))
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("WDI_IndicateNLODiscovery(): GenerateWdiIndicationNloDiscovery Failed\n"));
		FunctionOut(COMP_POWER);
		return;
	}
		
	wdi_UnsolicitedIndication(	pAdapter, 
								FALSE,
								NULL,
								NDIS_STATUS_WDI_INDICATION_NLO_DISCOVERY,
								NDIS_STATUS_SUCCESS,
								buf,
								buflen);

	// cleanup
	FreeGenerated((UINT8 *)buf);	
	
	FunctionOut(COMP_POWER);
}


VOID
WDI_IndicateCurrentPhyStatus(
	PADAPTER		pAdapter
	)
{
	wdiext_IndicateRadioStatus(pAdapter);
}


VOID
WDI_IndicateP2PEvent(	
	VOID 			*pvP2PInfo,
	u4Byte 			EventID,
	MEMORY_BUFFER	*pInformation

	)
{
	P2P_INFO				*pP2PInfo = (P2P_INFO *)pvP2PInfo;
	RT_OID_HANDLER			*task = &pP2PInfo->pAdapter->pPortCommonInfo->WdiData.TaskHandle;
	
	if(P2P_EVENT_DEV_FOUND == EventID)
	{
		wdiext_IndicateP2PDeviceFound(pP2PInfo->pAdapter, pInformation->Buffer);
	}
	else if(P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_REQUEST == EventID
		|| P2P_EVENT_RECEIVED_GO_NEGOTIATION_REQUEST == EventID
		|| P2P_EVENT_RECEIVED_INVITATION_REQUEST == EventID
		)
	{// rx req action 
		WDI_CMD_PreIndicateRxP2pActionFrameCb(pvP2PInfo, EventID, pInformation);	
		wdiext_IndicateP2PActionFrameReceived(pvP2PInfo, EventID, pInformation);
		WDI_CMD_PostIndicateRxP2pActionFrameCb(pvP2PInfo, EventID, pInformation);
	}
	else if(P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE == EventID
		|| P2P_EVENT_RECEIVED_INVITATION_RESPONSE == EventID
		|| P2P_EVENT_RECEIVED_GO_NEGOTIATION_RESPONSE == EventID
		|| P2P_EVENT_RECEIVED_GO_NEGOTIATION_CONFIRM == EventID
		)
	{// rx rsp action
		WDI_CMD_PreIndicateRxP2pActionFrameCb(pvP2PInfo, EventID, pInformation);
		
		// indicate in the corresp. OffChnlTx complete cb
		//wdiext_IndicateP2PActionFrameReceived(pvP2PInfo, EventID, pInformation);
		
		WDI_CMD_PostIndicateRxP2pActionFrameCb(pvP2PInfo, EventID, pInformation);
	}
	else if(P2P_EVENT_GO_OPERATING_CHANNEL == EventID)
	{
		wdiext_IndicateP2POpChnl(pP2PInfo->pAdapter, pP2PInfo);
	}

	return;		
}


BOOLEAN
WDI_CompleteCreateDeleteMac(
	IN  PADAPTER				pAdapter,
	IN  NDIS_STATUS				status
	)
{
	PPORT_HELPER			pPortHelper = pAdapter->pPortCommonInfo->pPortHelper;
	BOOLEAN					bCompleteStatus = FALSE;

	if(pPortHelper->bCreateMac)
		bCompleteStatus = WDI_CMD_CreatePortComplete(pAdapter, pPortHelper->pCreateDeleteOID, status);
	else if(pPortHelper->bDeleteMac)
		bCompleteStatus = WDI_CMD_DeletePortComplete(pAdapter, pPortHelper->pCreateDeleteOID, status);

	return bCompleteStatus;
}

VOID
WDI_IndicateLinkStateChanged(	
	PADAPTER		pAdapter,
	BOOLEAN			bForceLinkQuality,
	u1Byte			ucLinkQuality
	)
{
	wdiext_IndicateLinkStateChanged(pAdapter, bForceLinkQuality, ucLinkQuality);
}

RT_STATUS
WDI_IndicateActionFrame(
	IN	PADAPTER		pAdapter,
	IN	POCTET_STRING	posMpdu
)
{
	NDIS_STATUS_INDICATION		*indic = NULL;
	WDI_MESSAGE_HEADER			*wdiHdr = NULL;
	
	u1Byte						*buf = NULL;
	ULONG						buflen = 0;

	WDI_INDICATION_ACTION_FRAME_RECEIVED_PARAMETERS param = {0};

	RT_TRACE(COMP_OID_SET, DBG_WARNING, ("WDI_IndicateActionFrame +\n"));
	

	// TODO: fill the valid info into param
	PlatformMoveMemory(param.SourceAddress.Address,Frame_pSaddr(*posMpdu),6);
	param.ActionFrameBody.pElements = posMpdu->Octet+24;//get rid of header
	param.ActionFrameBody.ElementCount = posMpdu->Length;

	// TODO: double check here to make sure channel correct
	param.ChannelInfo.ChannelNumber = RT_GetChannelNumber(pAdapter);	
	param.ChannelInfo.BandId = WDI_BAND_ID_2400;

	if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationActionFrameReceived(
			&param, 
			sizeof(*indic) + sizeof(*wdiHdr), 
			&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
			&buflen, &((UINT8 *)buf)))
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("ActionFrame generate tlv failed\n"));
		return NDIS_STATUS_FAILURE;
	}

	indic = (NDIS_STATUS_INDICATION *)buf;
	wdiHdr = (WDI_MESSAGE_HEADER *)(buf + sizeof(*indic));

	PlatformZeroMemory(indic, sizeof(*indic));
	indic->Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
	indic->Header.Size = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
	indic->Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
	indic->SourceHandle = pAdapter;
	indic->PortNumber = 0;
	indic->StatusCode = NDIS_STATUS_WDI_INDICATION_ACTION_FRAME_RECEIVED;
	indic->RequestId = 0; // ?
	indic->StatusBuffer = wdiHdr;
	indic->StatusBufferSize = buflen - sizeof(*indic);

	wdiHdr->PortId = (WDI_PORT_ID)(GET_PORT_NUMBER(pAdapter));
	wdiHdr->Reserved = 0;
	wdiHdr->Status = NDIS_STATUS_SUCCESS;
	wdiHdr->TransactionId = 0;
	wdiHdr->IhvSpecificId = 0;

	NdisMIndicateStatusEx(pAdapter->pNdisCommon->hNdisAdapter, indic);

	FreeGenerated((UINT8 *)buf);
	
	return RT_STATUS_SUCCESS;
}

VOID
WDI_IndicateWakeReason(
	IN PADAPTER		pAdapter,
	IN BOOLEAN		bWakePacket,
	IN pu1Byte		pBuffer,
	IN u2Byte		BufferLen
	)
{	
	WDI_INDICATION_WAKE_REASON_PARAMETERS		param;

	PMGNT_INFO					pMgntInfo 	= &(pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL		pPSC		= GET_POWER_SAVE_CONTROL(pMgntInfo);
	pu1Byte						pWakePktbuf	= NULL;	
	BOOLEAN						bIndicate 	= TRUE;
	u1Byte						*buf 		= NULL;
	ULONG						buflen 		= 0;

	PlatformZeroMemory(&param, sizeof(WDI_INDICATION_WAKE_REASON_PARAMETERS));

	if(bWakePacket)
	{
		if(pPSC->WakeUpReason == WOL_REASON_MAGIC_PKT)
			param.Optional.WakePacket_IsPresent = 1;
		else if(pPSC->WakeUpReason == WOL_REASON_PATTERN_PKT)
			param.Optional.WakePacketPatternId_IsPresent = 1;
		else
		{
			RT_TRACE(COMP_POWER, DBG_SERIOUS, ("WDI_IndicateWakeReason(): Unknown pPSC->WakeUpReason(%#x)\n", pPSC->WakeUpReason));
		}

		param.WakeEventCode = WDI_WAKE_REASON_CODE_PACKET;		
		param.WakePacketPatternId = pPSC->MagicPacketPatternId;

		PlatformAllocateMemoryWithZero(pAdapter, (PVOID *)&pWakePktbuf, BufferLen);

		PlatformMoveMemory(pWakePktbuf, pBuffer, BufferLen);

		param.WakePacket.ElementCount = 1;
		param.WakePacket.pElements = pWakePktbuf;

		RT_PRINT_DATA(COMP_POWER, DBG_LOUD, "WDI_IndicateWakeReason(): packet\n", param.WakePacket.pElements, BufferLen);
	}
	else
	{
		if(pPSC->WakeUpReason == WOL_REASON_AP_LOST ||
			pPSC->WakeUpReason == WOL_REASON_DEAUTH ||
			pPSC->WakeUpReason == WOL_REASON_DISASSOC)
		{
			param.WakeEventCode = WDI_WAKE_REASON_CODE_AP_ASSOCIATION_LOST;
		}
		else if(pPSC->WakeUpReason == WOL_REASON_NLO_SSID_MATCH)
		{
			param.WakeEventCode = WDI_WAKE_REASON_CODE_NLO_DISCOVERY;
		}
		else if(pPSC->WakeUpReason == WOL_REASON_PTK_UPDATE)
		{
			param.WakeEventCode = WDI_WAKE_REASON_CODE_4WAY_HANDSHAKE_REQUEST;
		}
		else
		{
			param.WakeEventCode = NdisWakeReasonUnspecified;
			bIndicate = FALSE;
		}
	}
	
	if(bIndicate)
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("WDI_IndicateWakeReason(): WakeEventCode(%#X)\n", param.WakeEventCode));

		// Gen tlv
		if(NDIS_STATUS_SUCCESS != GenerateWdiIndicationWakeReasonFromIhv(
			&param, 
			sizeof(NDIS_STATUS_INDICATION) + sizeof(WDI_MESSAGE_HEADER), 
			&pAdapter->pPortCommonInfo->WdiData.TlvContext, 
			&buflen, &((UINT8 *)buf)))
		{
				RT_TRACE(COMP_POWER, DBG_LOUD, ("WDI_IndicateWakeReason(): GenerateWdiIndicationWakeReasonFromIhv Failed\n"));
				return;
		}
		
		wdi_UnsolicitedIndication(pAdapter, 
								FALSE,
								NULL,
								NDIS_STATUS_WDI_INDICATION_WAKE_REASON,
								NDIS_STATUS_SUCCESS,
								buf,
								buflen);
		
		// cleanup
		FreeGenerated((UINT8 *)buf);
	}	
	
}


VOID
WDI_IndicateRoamingNeeded(	
	PADAPTER				pAdapter,
	RT_PREPARE_ROAM_TYPE	IndicationReason
	)
{
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);
	
	pMgntInfo->PrepareRoamState = IndicationReason;
		
	switch(IndicationReason)
	{
		case RT_PREPARE_ROAM_DEAUTH_DISASSOC:
			wdiext_IndicateRoamingNeeded(pAdapter, WDI_ASSOC_STATUS_PEER_DEAUTHENTICATED);
			break;
		case RT_PREPARE_ROAM_NORMAL_ROAM_POOR_LINK:
			wdiext_IndicateRoamingNeeded(pAdapter, WDI_ASSOC_STATUS_DISASSOCIATE_NOT_VISIBLE);
			break;
		case RT_PREPARE_ROAM_NORMAL_ROAM_BETTER_AP:
			wdiext_IndicateRoamingNeeded(pAdapter, WDI_ASSOC_STATUS_ROAMING_BETTER_AP_FOUND);
			break;
		default:
			break;
	}
}

VOID
WDI_IndicateRoamingComplete(	
	PADAPTER		pAdapter
	)
{
	PMGNT_INFO			pMgntInfo = GetDefaultMgntInfo(pAdapter);

	if(FALSE == OidHandle_VerifyTask(pAdapter, OID_WDI_TASK_ROAM))
		return;
	
	OidHandle_Complete(pAdapter, OIDHANDLE_TYPE_TASK);
}

VOID 
WDI_AddGroupPeer(
	IN  PADAPTER 	pAdapter,
	IN  u2Byte		PortNumber
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	WDI_MAC_ADDRESS	PeerAddr = {0};	
	PRT_PEER_ENTRY		pPeerEntry = NULL;
	u1Byte				i = 0;
	
	FunctionIn(COMP_INIT);
	
	PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	for(i = 0; i < MP_DEFAULT_NUMBER_OF_PORT; i++)
	{
		pPeerEntry = &pDefaultAdapter->RxPeerTable[i];
		if( pPeerEntry->bUsed == FALSE )
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("WDI_AddGroupPeer: create group peer %d for port %d\n", i, PortNumber));	
			break;
		}
	}
	
	if( i == MP_DEFAULT_NUMBER_OF_PORT )
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("WDI_AddGroupPeer: there is no available group peer for port %d\n", PortNumber));	
		FunctionOut(COMP_INIT);
		PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		return ;
	}
	
	pPeerEntry->bUsed = TRUE;
	pPeerEntry->uPeerId = i;
	pPeerEntry->uPortId = PortNumber;
	PlatformMoveMemory(pPeerEntry->PeerAddr, MAC_BROADCAST_ADDR, 6);
	PlatformMoveMemory(PeerAddr.Address, pPeerEntry->PeerAddr, 6);
	pWdi->WdiDataApi.PeerCreateIndication(pWdi->DataPathHandle, (WDI_PORT_ID)pPeerEntry->uPortId, (WDI_PEER_ID)pPeerEntry->uPeerId, PeerAddr);
	
	PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	
	FunctionOut(COMP_INIT);
}

VOID 
WDI_DeleteGroupPeer(
	IN  PADAPTER 	pAdapter,
	IN  u2Byte		PortNumber
	)
{
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	NDIS_STATUS 		ndisStatus = NDIS_STATUS_SUCCESS;
	WDI_PEER_ID			PeerId = 0;
	PRT_PEER_ENTRY		pPeerEntry = NULL;
	u1Byte				i;
	
	FunctionIn(COMP_INIT);

	PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	
	pPeerEntry = (PRT_PEER_ENTRY)wdi_FindGroupPeerByPort(pAdapter, PortNumber);
	if( pPeerEntry == NULL )
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("wdi_FindPeerByAddr: Cannot find group peer of port %d\n", PortNumber));	
		FunctionOut(COMP_INIT);
		PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		return ;
	}
	else
	{
		PeerId = (WDI_PEER_ID)pPeerEntry->uPeerId;
		pPeerEntry->bUsed = FALSE;
		
		PlatformZeroMemory(pPeerEntry->PeerAddr, 6);	
		
		pWdi->WdiDataApi.PeerDeleteIndication(pWdi->DataPathHandle, (WDI_PORT_ID)PortNumber, PeerId, &ndisStatus);
		
		WDI_FreeRxFrame(pAdapter, (PNET_BUFFER_LIST)N6CGetHeadNblWaitQueue(pDefaultAdapter->RxPeerQueue[PeerId]));
		RTInitializeSListHead(&pDefaultAdapter->RxPeerQueue[PeerId]);
		
		RT_TRACE(COMP_INIT, DBG_LOUD, ("WDI_DeleteGroupPeer: delete group peer %d of port %d, Status: %d\n", PeerId, PortNumber, ndisStatus));
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	
	FunctionOut(COMP_INIT);
}

NDIS_STATUS 
WDI_AddDatapathPeer(
	IN	PADAPTER 	pAdapter,
	IN	pu1Byte		pAddr)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	WDI_PEER_ID			HashPeer = 0, RelocatePeer = 0;
	WDI_PORT_ID		PortId = (WDI_PORT_ID)pAdapter->pNdis62Common->PortNumber;
	WDI_MAC_ADDRESS	PeerAddr = {0};
	PRT_PEER_ENTRY		pPeerEntry = NULL;
	
	FunctionIn(COMP_MLME);
	
	PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	
	pPeerEntry = (PRT_PEER_ENTRY)wdi_GetAvailablePeer(pAdapter, pAddr);
	if( pPeerEntry != NULL )
	{			
		PlatformMoveMemory(PeerAddr.Address, pPeerEntry->PeerAddr, 6);
		pWdi->WdiDataApi.PeerCreateIndication(pWdi->DataPathHandle, (WDI_PORT_ID)pPeerEntry->uPortId, (WDI_PEER_ID)pPeerEntry->uPeerId, PeerAddr);
		
		PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		
		FunctionOut(COMP_MLME);
		
		return NDIS_STATUS_SUCCESS;
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	
	FunctionOut(COMP_MLME);
	
	return NDIS_STATUS_FAILURE;
}

void WDI_DeleteDatapathPeer(
	IN	PADAPTER	pAdapter,
	IN	pu1Byte		pAddr
)
{
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	WDI_PORT_ID		PortId = 0;
	NDIS_STATUS 		ndisStatus = NDIS_STATUS_SUCCESS;
	WDI_PEER_ID			PeerId = 0;
	PRT_PEER_ENTRY		pPeerEntry = NULL;
	
	FunctionIn(COMP_MLME);
	
	PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	
	pPeerEntry = (PRT_PEER_ENTRY)wdi_FindPeerByAddr(pAdapter, pAddr);
	if( pPeerEntry == NULL )
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("wdi_FindPeerByAddr: Cannot find the addres [%2x:%2x:%2x:%2x:%2x:%2x] associated at any peer\n", pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5]));
		FunctionOut(COMP_MLME);
		PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		return ;
	}
	RT_TRACE(COMP_INIT, DBG_LOUD, ("WDI_DeleteDatapathPeer: Find the addres [%2x:%2x:%2x:%2x:%2x:%2x] associated at Peer %d\n", pAddr[0], pAddr[1], pAddr[2], pAddr[3], pAddr[4], pAddr[5], pPeerEntry->uPeerId));	
	
	pDefaultAdapter->RxPeerUsedCount--;
	PeerId = (WDI_PEER_ID)pPeerEntry->uPeerId;
	PortId = (WDI_PORT_ID)pPeerEntry->uPortId;
	pPeerEntry->bUsed = FALSE;
	PlatformZeroMemory(pPeerEntry->PeerAddr, 6);	
	
	pWdi->WdiDataApi.PeerDeleteIndication(pWdi->DataPathHandle, PortId, PeerId, &ndisStatus);
	
	WDI_FreeRxFrame(pAdapter, (PNET_BUFFER_LIST)N6CGetHeadNblWaitQueue(pDefaultAdapter->RxPeerQueue[PeerId]));
	RTInitializeSListHead(&pDefaultAdapter->RxPeerQueue[PeerId]);
	
	PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("WDI_DeleteDatapathPeer: delete peer %d, Status: %d\n", PeerId, ndisStatus));
	
	FunctionOut(COMP_MLME);
}

VOID
WDI_FetchNBLByPort(
	IN		PADAPTER			pAdapter,
	IN		u2Byte				Port_ID,
	OUT		PNET_BUFFER_LIST	*ppNetBufferList
)
{
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(pAdapter);
	u2Byte					index = 0;
	PRT_SINGLE_LIST_ENTRY	pFirst = NULL, pTemp = NULL;
	
	*ppNetBufferList = NULL;

	RT_TRACE(COMP_RECV, DBG_LOUD, ("WDI_FetchNBLByPort: fetch frames of all peers at port %d\n", Port_ID));

	if( Port_ID != WDI_PORT_ANY )
	{
		PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		for(index = 0; index < MAX_PEER_NUM; index++)
		{
			if( (pDefaultAdapter->RxPeerTable[index].bUsed == TRUE) && (pDefaultAdapter->RxPeerTable[index].uPortId == Port_ID) )
			{
				if(N6CIsNblWaitQueueEmpty(pDefaultAdapter->RxPeerQueue[index]))
					continue;
				
				pTemp = RTGetHeadSList(&pDefaultAdapter->RxPeerQueue[index]);
				if( pFirst == NULL )
				{
					pFirst = pTemp;
				}
				else
				{
					N6CConcatenateTwoList(pFirst, pTemp);
				}
				RTInitializeSListHead(&pDefaultAdapter->RxPeerQueue[index]);
			}
		}
		
		if( pFirst != NULL )
		{
			*ppNetBufferList = RT_GET_NBL_FROM_QUEUE_LINK(pFirst);
		}
		PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	}
	else
	{
		PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		for(index = 0; index < MAX_PEER_NUM; index++)
		{
			if( (pDefaultAdapter->RxPeerTable[index].bUsed == TRUE) )
			{
				if(N6CIsNblWaitQueueEmpty(pDefaultAdapter->RxPeerQueue[index]))
					continue;
				
				pTemp = RTGetHeadSList(&pDefaultAdapter->RxPeerQueue[index]);
				if( pFirst == NULL )
				{
					pFirst = pTemp;
				}
				else
				{
					N6CConcatenateTwoList(pFirst, pTemp);
				}
				RTInitializeSListHead(&pDefaultAdapter->RxPeerQueue[index]);
			}
		}
		
		if( pFirst != NULL )
		{
			*ppNetBufferList = RT_GET_NBL_FROM_QUEUE_LINK(pFirst);
		}
		PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	}
}

VOID
WDI_FreeRxFrame(
	IN	PADAPTER	pAdapter,
	IN	PNET_BUFFER_LIST pNetBufferList
)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PRT_RFD				pRfd = NULL;
	PMDL				pCurrMdl = NULL, pNextMdl = NULL;
	PNET_BUFFER_LIST		pNextNBL = NULL;
	u4Byte				Count;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;

	FunctionIn(COMP_RECV);

	if( pNetBufferList == NULL )
	{
		RT_TRACE(COMP_RECV, DBG_TRACE, ("WDI_FreeRxFrame: pNetBufferList is NULL\n"));
		FunctionOut(COMP_RECV);
		return;
	}
	
	do
	{		
		//Get next NBL
		pNextNBL = NET_BUFFER_LIST_NEXT_NBL(pNetBufferList);	

		//Get RFD from NBL
		pRfd = MP_GET_PACKET_RFD(pNetBufferList);

		//Free all MDL
		for (pCurrMdl = NET_BUFFER_FIRST_MDL( NET_BUFFER_LIST_FIRST_NB(pNetBufferList) );
			pCurrMdl != NULL;
			pCurrMdl = pNextMdl)
		{
			pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
			NdisFreeMdl(pCurrMdl);
		}

		//Free Rx metadata
		WDIFreeMetaData(pAdapter, pNetBufferList);

		//Free NBL
		NdisFreeNetBufferList(pNetBufferList);

		//Re-calculate reference count
		PlatformAcquireSpinLock(pAdapter, RT_RX_REF_CNT_SPINLOCK);
		RT_DEC_RCV_REF(pDefaultAdapter);
		Count = RT_GET_RCV_REF(pDefaultAdapter);
		GLWdiTxRxStatistics.numRxReturnNBLFromUE++;
		PlatformReleaseSpinLock(pAdapter, RT_RX_REF_CNT_SPINLOCK);

		//Return RFD to RFD list
		if( pRfd != NULL )
		{
			ReturnRFDList(pAdapter, pRfd);
		}

		//Assign NextNBL pointer
		pNetBufferList = pNextNBL;
	}while(pNetBufferList!=NULL);
	
	if (Count == 0)
	{
		NdisSetEvent(&pNdisCommon->AllPacketReturnedEvent);

		if(pAdapter->MgntInfo.NdisVersion >= RT_NDIS_VERSION_6_20)		
		{		
			PADAPTER pTempAdapter = GetFirstExtAdapter(pAdapter);
			while(pTempAdapter != NULL)
			{
				NdisSetEvent(&pTempAdapter->pNdisCommon->AllPacketReturnedEvent);
				pTempAdapter = GetNextExtAdapter(pTempAdapter);				
			}		
		}
	}
	
	FunctionOut(COMP_RECV);
}

VOID
WDI_IndicateTKIPMICFailure(
	PADAPTER		pAdapter
	)
{
	PMGNT_INFO				pMgntInfo;		
	pMgntInfo = &(pAdapter->MgntInfo);
	PUCHAR											pOutput = NULL;
	ULONG											length = 0;
	NDIS_STATUS 									ndisStatus = NDIS_STATUS_SUCCESS;
	WDI_INDICATION_TKIP_MIC_FAILURE_PARAMETERS 		Params={0};

	FunctionIn(COMP_SEC);

	
	Params.FailureInfo.DefaultKeyFailure = FALSE;
	Params.FailureInfo.KeyIndex=0;
	//WDI_TKIPFailureParam.PeerMacAddress=pMgntInfo->Bssid;
	cpMacAddr(Params.FailureInfo.PeerMacAddress.Address, pMgntInfo->Bssid);
	
	

	ndisStatus = GenerateWdiIndicationTkipMicFailure(
				&Params,
				sizeof(NDIS_STATUS_INDICATION)+sizeof(WDI_MESSAGE_HEADER),
				&pAdapter->pPortCommonInfo->WdiData.TlvContext,
				&length,
				&pOutput);

	if(NDIS_STATUS_SUCCESS == ndisStatus)
	{
		wdi_UnsolicitedIndication(pAdapter,
								FALSE,
								NULL,
								NDIS_STATUS_WDI_INDICATION_TKIP_MIC_FAILURE,
								ndisStatus,
								pOutput,
								length);

		FreeGenerated(pOutput);
		pOutput = NULL;
	}
	FunctionOut(COMP_SEC);


}

VOID
WDI_TxCreditCheck(
	IN  PADAPTER		pAdapter
	)
{
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	u4Byte				preTxPauseReason;
	u2Byte				txCredit=0;

	GLWdiTxRxStatistics.numCheckTxPause++;
	GLWdiTxRxStatistics.monitorTxPauseReason = pWdi->txPauseReason;

	// First check if we have pause indication before.
	// not to acquire tx lock frequently, so we do not acquire tx lock when read, but remember 
	// we should acquire tx lock when need to change the pause reason.
	if(pWdi->txPauseReason)
	{
		PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

		txCredit = *Get_NUM_IDLE_TCB(pAdapter);
		preTxPauseReason = pWdi->txPauseReason;
		
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], WDI_TxCreditCheck(): txCredit=%d\n", txCredit));
		
		if(txCredit > (pAdapter->RT_TCB_NUM/4))
		{
			GLWdiTxRxStatistics.numTxResourceAvailable++;
			
			if( (preTxPauseReason&WDI_TX_PAUSE_REASON_CREDIT) == WDI_TX_PAUSE_REASON_CREDIT)
			{
				RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], WDI_TxCreditCheck(): TxPauseReason = 0x%x!!!\n", preTxPauseReason));				
				GLWdiTxRxStatistics.numWdiTxRestartIndicate++;
				RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], WDI_TxCreditCheck(): TxSendRestart: %d times!!!\n",
					GLWdiTxRxStatistics.numWdiTxRestartIndicate));
		
				pWdi->txPauseReason &= ~WDI_TX_PAUSE_REASON_CREDIT;
				PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

				// Before indicate to WDI, release the tx lock.
				pWdi->WdiDataApi.TxSendRestartIndication(pWdi->DataPathHandle, 
														WDI_PORT_ANY, 
														WDI_PEER_ANY,
	    													WDI_EXT_TID_UNKNOWN,
	    													WDI_TX_PAUSE_REASON_CREDIT);
				RT_TRACE(COMP_INIT, DBG_LOUD, ("[WDI], WDI_TxCreditCheck(): pWdi->txPauseReason = 0x%x\n", pWdi->txPauseReason));
			}
			else
			{
				PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
			}
		}
		else
		{
			PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
		}
	}
}

VOID
WDI_IndicateFWStalled(
	IN  PADAPTER				pAdapter
	)
{
	PWDI_DATA_STRUCT			pWdi = &(pAdapter->pPortCommonInfo->WdiData);
	NDIS_STATUS_INDICATION		statusIndication = {0};
	WDI_MESSAGE_HEADER			WdiHeader = {0};
	
	if(!pWdi->bWdiLEPLDR)
		return;
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("==> WDI_IndicateFWStalled\n"));

	N6_ASSIGN_OBJECT_HEADER(
		statusIndication.Header,
		NDIS_OBJECT_TYPE_STATUS_INDICATION,
		NDIS_SIZEOF_STATUS_INDICATION_REVISION_1,
		NDIS_STATUS_INDICATION_REVISION_1);

	WdiHeader.PortId = 0xFFFF;
	WdiHeader.Status = NDIS_STATUS_SUCCESS;
	WdiHeader.TransactionId = 0;

	statusIndication.RequestId = NULL;
	statusIndication.SourceHandle = pAdapter;
	statusIndication.StatusBuffer = &WdiHeader;
	statusIndication.StatusBufferSize = sizeof(WDI_MESSAGE_HEADER);

	statusIndication.StatusCode = NDIS_STATUS_WDI_INDICATION_FIRMWARE_STALLED;

	NdisMIndicateStatusEx(
		pAdapter->pNdisCommon->hNdisAdapter,
		&statusIndication
		);
}

VOID
WDI_UpdateDefaultSetting(
	IN  PADAPTER				pAdapter
	)
{
	PRT_NDIS_COMMON			pNdisCommon = pAdapter->pNdisCommon;
	PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);

	// Enable for LE Hang detection and recovery function
	pWdi->bWdiLEPLDR = pNdisCommon->RegHangDetection;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("WDI_UpdateDefaultSetting(): bWdiLEPLDR(%d)\n", pWdi->bWdiLEPLDR));
}

//
// Description:
//	Indicate the specific (non-common for all platform) event/status to WDI.
// Arguments:
//	[in] pAdapter -
//		The adapter context.
//	[in] event -
//		The event of the indication.
//	[in] pInfoBuffer -
//		The pointer to the input information. It can be NULL.
//	[in] InfoBruuferLen -
//		The length in byte for pInfoBuffer.
// Return:
//	It may return RT_STATUS_SUCCESS if this function completes successfully, or RT_STATUS_PENDING
//	for the asynchrous process.
// By Bruce, 2015-12-16.
//
RT_STATUS
WDI_IndicateGeneralEvent(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			Event,
	IN	PVOID			pInfoBuffer,
	IN	u4Byte			InfoBruuferLen
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	
	switch(Event)
	{
	default:
		{
			RT_TRACE_F(COMP_INDIC, DBG_WARNING, ("Unrecognized event = 0x%08X\n", Event));
			rtStatus = RT_STATUS_NOT_RECOGNIZED;
		}
		break;




	case RT_CUSTOM_EVENT_WDI_FT_ASSOC_NEEDED:
		{
			rtStatus = wdiext_IndicateFtAssocNeeded(pAdapter);
		}
		break;
	}
	return rtStatus;
}

#endif
