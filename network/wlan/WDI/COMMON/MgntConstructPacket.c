#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "MgntConstructPacket.tmh"
#endif

static u1Byte	BroadcastAddress[6]={0xff,0xff,0xff,0xff,0xff,0xff};

VOID
AppendQoSElement(
	OUT		POCTET_STRING	packet,
	IN		POCTET_STRING	element,
	IN		u1Byte			OUI_Subtype
	)
{
	u1Byte			szQoSOUI[] ={221, 0, 0x00, 0x50, 0xf2, OUI_SUB_WMM, 0, 1};
	OCTET_STRING	tmp;
	
	if (OUI_Subtype == OUI_SUBTYPE_QOS_CAPABI)
	{
		szQoSOUI[0] = 46;
		szQoSOUI[1] = (u1Byte)element->Length;
		FillOctetString(tmp, szQoSOUI, 2);
		PacketAppendData(packet, tmp);	
	}	
	else
	{
	szQoSOUI[1] = element->Length + 6;
	szQoSOUI[6] = OUI_Subtype;
	FillOctetString(tmp, szQoSOUI, 8);
	PacketAppendData(packet, tmp);
	}	

	// QoS Capability IE
	PacketAppendData(packet, *element);
}


VOID
SelectSupportedRatesElement(
	IN	WIRELESS_MODE	wirelessmode,
	IN	OCTET_STRING	SuppRates,
	IN	BOOLEAN			bFilterCck,
	OUT	POCTET_STRING	pSuppRates,
	OUT	POCTET_STRING	pExtSuppRates
)	
{
	OCTET_STRING 	FilteredSuppRates;
	u1Byte 			FilteredSuppRatesBuf[MAX_NUM_RATES];

	FilteredSuppRates.Length = 0;
	FilteredSuppRates.Octet = FilteredSuppRatesBuf;

	if(bFilterCck)
	{
		u2Byte i = 0;

		for(i = 0; i < SuppRates.Length; i++)
		{
			if(IS_CCK_RATE((SuppRates.Octet[i] & 0x7F)))
			{
				continue;
			}
			else
			{
				FilteredSuppRates.Octet[FilteredSuppRates.Length] = SuppRates.Octet[i];
				FilteredSuppRates.Length++;
			}
		}
	}
	else
	{
		PlatformMoveMemory(FilteredSuppRates.Octet, SuppRates.Octet, SuppRates.Length);
		FilteredSuppRates.Length = SuppRates.Length;
	}
		
	pSuppRates->Length = 0;
	pExtSuppRates->Length = 0;

#if 1
	if(wirelessmode & (WIRELESS_MODE_A|WIRELESS_MODE_B|WIRELESS_MODE_N_5G|WIRELESS_MODE_AC_5G))
	{
		u2Byte cbCopied = ((FilteredSuppRates.Length <= 8) ? FilteredSuppRates.Length : 8);

		// NOTE! Length of Support Rates <= 8.
		CopyMem( pSuppRates->Octet, FilteredSuppRates.Octet, cbCopied );
		pSuppRates->Length = cbCopied;
	}
	else if(wirelessmode & (WIRELESS_MODE_G|WIRELESS_MODE_N_24G|WIRELESS_MODE_AC_24G))
	{
		u4Byte i; 

		for(i = 0; i < FilteredSuppRates.Length; i++)
		{
			// There are only 4 rates in WIRELESS_MODE_B, just put them into SuppRateIE first.
			// Always put B mode rate in SuppRateIE.
			if(MgntIsRateValidForWirelessMode((FilteredSuppRates.Octet[i] & 0x7F), WIRELESS_MODE_B))
			{
				pSuppRates->Octet[pSuppRates->Length] = FilteredSuppRates.Octet[i];
				pSuppRates->Length++;
			}
			// Put other rates to ExtSuppRatesIE first. (Also check if ExtSuppRatesIE length longer than 255.)
			else if(pExtSuppRates->Length < 255)
			{
				pExtSuppRates->Octet[pExtSuppRates->Length] = FilteredSuppRates.Octet[i];
				pExtSuppRates->Length++;
			}
			else
			{
				RT_TRACE(COMP_MLME, DBG_WARNING, ("SelectSupportedRatesElement(): ExtSuppRatesIE length larger than 255. \n"));
			}
		}

		// pSuppRates->Length must less than 4 at this point.
		if((pSuppRates->Length + pExtSuppRates->Length) <= MAX_SUP_RATE_LEN)
		{
			CopyMem( pSuppRates->Octet+pSuppRates->Length,  pExtSuppRates->Octet, pExtSuppRates->Length);
			pSuppRates->Length = pSuppRates->Length + pExtSuppRates->Length;
			pExtSuppRates->Length = 0;
		}
		else
		{
			u1Byte	SupRateAddLen = 0, NewLen_ExtSupRate = 0;
			u1Byte	TmpRateBuffer[256] = {0};

			SupRateAddLen = MAX_SUP_RATE_LEN - pSuppRates->Length;
			NewLen_ExtSupRate = pExtSuppRates->Length - SupRateAddLen;
			
			CopyMem( pSuppRates->Octet+pSuppRates->Length,  pExtSuppRates->Octet, SupRateAddLen);
			pSuppRates->Length = MAX_SUP_RATE_LEN;
		
			CopyMem( TmpRateBuffer, pExtSuppRates->Octet+SupRateAddLen, NewLen_ExtSupRate);
			CopyMem(pExtSuppRates->Octet, TmpRateBuffer, NewLen_ExtSupRate);
			pExtSuppRates->Length = NewLen_ExtSupRate;
		}

		RT_PRINT_DATA(COMP_SEC, DBG_TRACE, "pSuppRates", pSuppRates->Octet, pSuppRates->Length);
		RT_PRINT_DATA(COMP_SEC, DBG_TRACE, "pExtSuppRates", pExtSuppRates->Octet, pExtSuppRates->Length);
	}
	else
	{
		RT_ASSERT(FALSE, ("SelectSupportedRatesElement(): Unknown wirelessmode: 0x%x\n", wirelessmode));	
	}
#else	
	switch( wirelessmode )
	{
	case WIRELESS_MODE_A:
	case WIRELESS_MODE_B:
	case WIRELESS_MODE_N_5G:		//Added by Emily 2006.11.10
	case WIRELESS_MODE_AC_5G:
		{
			u2Byte cbCopied = ((FilteredSuppRates.Length <= 8) ? FilteredSuppRates.Length : 8);

			// NOTE! Length of Support Rates <= 8.
			CopyMem( pSuppRates->Octet, FilteredSuppRates.Octet, cbCopied );
			pSuppRates->Length = cbCopied;
		}
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:	//Added by Emily 2006.11.10
	case WIRELESS_MODE_AC_24G:
		{
			u4Byte i; 

			for(i = 0; i < FilteredSuppRates.Length; i++)
			{
				// There are only 4 rates in WIRELESS_MODE_B, just put them into SuppRateIE first.
				// Always put B mode rate in SuppRateIE.
				if(MgntIsRateValidForWirelessMode((FilteredSuppRates.Octet[i] & 0x7F), WIRELESS_MODE_B))
				{
					pSuppRates->Octet[pSuppRates->Length] = FilteredSuppRates.Octet[i];
					pSuppRates->Length++;
				}
				// Put other rates to ExtSuppRatesIE first. (Also check if ExtSuppRatesIE length longer than 255.)
				else if(pExtSuppRates->Length < 255)
				{
					pExtSuppRates->Octet[pExtSuppRates->Length] = FilteredSuppRates.Octet[i];
					pExtSuppRates->Length++;
				}
				else
				{
					RT_TRACE(COMP_MLME, DBG_WARNING, ("SelectSupportedRatesElement(): ExtSuppRatesIE length larger than 255. \n"));
				}
			}

			// pSuppRates->Length must less than 4 at this point.
			if((pSuppRates->Length + pExtSuppRates->Length) <= MAX_SUP_RATE_LEN)
			{
				CopyMem( pSuppRates->Octet+pSuppRates->Length,  pExtSuppRates->Octet, pExtSuppRates->Length);
				pSuppRates->Length = pSuppRates->Length + pExtSuppRates->Length;
				pExtSuppRates->Length = 0;
			}
			else
			{
				u1Byte	SupRateAddLen = 0, NewLen_ExtSupRate = 0;
				u1Byte	TmpRateBuffer[256] = {0};

				SupRateAddLen = MAX_SUP_RATE_LEN - pSuppRates->Length;
				NewLen_ExtSupRate = pExtSuppRates->Length - SupRateAddLen;
				
				CopyMem( pSuppRates->Octet+pSuppRates->Length,  pExtSuppRates->Octet, SupRateAddLen);
				pSuppRates->Length = MAX_SUP_RATE_LEN;
			
				CopyMem( TmpRateBuffer, pExtSuppRates->Octet+SupRateAddLen, NewLen_ExtSupRate);
				CopyMem(pExtSuppRates->Octet, TmpRateBuffer, NewLen_ExtSupRate);
				pExtSuppRates->Length = NewLen_ExtSupRate;
			}

			RT_PRINT_DATA(COMP_SEC, DBG_TRACE, "pSuppRates", pSuppRates->Octet, pSuppRates->Length);
			RT_PRINT_DATA(COMP_SEC, DBG_TRACE, "pExtSuppRates", pExtSuppRates->Octet, pExtSuppRates->Length);
		}
		break;
	
	// Note: HT rate is filled in HTCapability Information Element, so the supported MCS rate is not initialized here
	default:
		RT_ASSERT(FALSE, ("SelectSupportedRatesElement(): Unknown wirelessmode: 0x%x\n", wirelessmode));
		break;
	}
#endif	
}



/*
  *  According to experiment, Realtek AP to STA (based on rtl8190) may achieve best performance 
  *  if both STA and AP set limitation of aggregation size to 32K, that is, set AMPDU density to 2 
  *  (Ref: IEEE 11n specification). However, if Realtek STA associates to other AP, STA should set 
  *  limitation of aggregation size to 8K, otherwise, performance of traffic stream from STA to AP 
  *  will be much less than the traffic stream from AP to STA if both of the stream runs concurrently 
  *  at the same time.
  *  
  *  Frame Format
  *  Element ID		Length		OUI			Type1		Reserved
  *  1 byte			1 byte		3 bytes		1 byte		1 byte
  * 
  *  OUI 		= 0x00, 0xe0, 0x4c,
  *  Type 	= 0x02
  *  Reserved 	= 0x00
  *
  *  2007.8.21 by Emily
*/
VOID
ConstructRTKAggElement(
	PADAPTER		Adapter,
	POCTET_STRING	posRTKAggIE
)
{
	BOOLEAN		bSupportRemoteWakeUp;
	PlatformZeroMemory(posRTKAggIE->Octet, 7);
	
	posRTKAggIE->Octet[0] = 0x00;
	posRTKAggIE->Octet[1] = 0xe0;
	posRTKAggIE->Octet[2] = 0x4c;
	posRTKAggIE->Octet[3] = 0x02;
	posRTKAggIE->Octet[4] = 0x02;

	if(ACTING_AS_AP(Adapter))
		posRTKAggIE->Octet[5] |= RT_HT_CAP_USE_SOFTAP;

	if(IS_VENDOR_8812A_C_CUT(Adapter))
		posRTKAggIE->Octet[6] |= RT_HT_CAP_USE_JAGUAR_CCUT;
	
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_WOWLAN , &bSupportRemoteWakeUp);

	if(bSupportRemoteWakeUp)
		posRTKAggIE->Octet[5] |=RT_HT_CAP_USE_WOW;
		
	posRTKAggIE->Length = 7;
}



VOID
ConstructBeaconFrame(
	PADAPTER		Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;	
	OCTET_STRING		DSParms;
	OCTET_STRING		IBSSParms;
	u8Byte			TimeStamp;
	pu1Byte			pbcn;
	OCTET_STRING		SuppRates;
	u1Byte			SuppRatesContent[8]; // NOTE! Length of Support Rates <= 8.
	OCTET_STRING		ExtSuppRates;
	u1Byte			ExtSuppRatesContent[255]; // NOTE! Length of Extended Support Rates <= 255.
	OCTET_STRING		ERPInfo;
	u1Byte			ERPInfoContent[1];
	OCTET_STRING	osEDCAInfoElem;
	u1Byte			EDCAInfo[1];
	OCTET_STRING		IbssAdditionalIEs;
	BOOLEAN		bFilterCck = FALSE;
	
	pbcn = pMgntInfo->beaconframe.Octet;

	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	// Frame Control.
	SET_80211_HDR_FRAME_CONTROL(pbcn, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pbcn, Type_Beacon);

	// Duration.
	SET_80211_HDR_DURATION(pbcn, 0);

	// Addresses (DA/SA/BSSID).
	SET_80211_HDR_ADDRESS1(pbcn, BroadcastAddress);
	SET_80211_HDR_ADDRESS2(pbcn, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pbcn, Adapter->MgntInfo.Bssid);

	//
	// Sequence Control.
	// 
	// TODO: 
	// 1. If the beacon is send by TransmitTcb, Seq shall be 0, otherwise, we shall fill it with 
	// proper value.
	// 2. We shall fix SendBeaconFrame to allow PCI use TransmitTCB to send beacon.
	//

	SET_80211_HDR_FRAGMENT_SEQUENCE(pbcn, 0);

	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	// Timestamp.

// [87B WiFi Ad-Hoc issue] Added by Annie, 2006-06-07.
// 1. TSF field will be updated by HW beacon.
// 2. On purpose set 0 for SW beacon is used. It can prevent other STA to update this value. It's a workaround for WiFi test.
	TimeStamp = 0;
	SET_BEACON_PROBE_RSP_TIME_STAMP_LOW(pbcn, (u4Byte)(TimeStamp&0xffffffff));
	SET_BEACON_PROBE_RSP_TIME_STAMP_HIGH(pbcn, (u4Byte)(TimeStamp >> 32));
//---

	// Beacon interval.
	SET_BEACON_PROBE_RSP_BEACON_INTERVAL(pbcn, pMgntInfo->dot11BeaconPeriod);

	// Capability information.
	if( ACTING_AS_AP(Adapter) )
	{
		SET_BEACON_PROBE_RSP_CAPABILITY_INFO(pbcn, (pMgntInfo->mCap & ~cIBSS) );
	}
	else
	{
		SET_BEACON_PROBE_RSP_CAPABILITY_INFO(pbcn, (pMgntInfo->mCap & ~cESS) );
	}

	if( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm!= RT_ENC_ALG_NO_CIPHER )
	{
		UNION_BEACON_PROBE_RSP_CAPABILITY_INFO(pbcn, cPrivacy);
	}
	else
	{
		MASK_BEACON_PROBE_RSP_CAPABILITY_INFO(pbcn, cPrivacy);
	}

	// Size of this Frame until last fixed size field.
	pMgntInfo->beaconframe.Length = 36;

	// SSID.
	if( !ACTING_AS_AP(Adapter) || !pMgntInfo->bHiddenSSID )
	{
		PacketMakeElement(&pMgntInfo->beaconframe, EID_SsId, Adapter->MgntInfo.Ssid);
	}
	else
	{ // AP mode Hidden SSID. Added by Annie, 2006-02-15.
		u1Byte			NullSSID[1];
		OCTET_STRING	HiddenSSID;
		FillOctetString(HiddenSSID, NullSSID, 0);
		PacketMakeElement(&pMgntInfo->beaconframe, EID_SsId, HiddenSSID);
	}

	// Supported rate.
	FillOctetString(SuppRates, SuppRatesContent, 0);
	FillOctetString(ExtSuppRates, ExtSuppRatesContent, 0);
	// Set supported and extended supported rate according to Wirelessmode and dot11OperationalRateSet. 

	P2P_FilerCck(Adapter, &bFilterCck);

	SelectSupportedRatesElement( pMgntInfo->dot11CurrentWirelessMode, pMgntInfo->dot11OperationalRateSet, bFilterCck,&SuppRates, &ExtSuppRates );

	PacketMakeElement(&pMgntInfo->beaconframe, EID_SupRates, SuppRates);
	
	// DS parameters element.
	// Note:
	//	We should have changed the dot11CurrentChannelNumber before constructing beacon frame no matter when the media is connected.
	DSParms.Octet = &pMgntInfo->dot11CurrentChannelNumber;
	DSParms.Length = 1;
	PacketMakeElement(&pMgntInfo->beaconframe, EID_DSParms, DSParms);
	RT_TRACE(COMP_AP, DBG_TRACE, ("Constructure Beacon Mediaconnect %d dot11CurrentChannel %d dot11CurrentWirelessMode=%x\n",
		pMgntInfo->bMediaConnect,pMgntInfo->dot11CurrentChannelNumber, pMgntInfo->dot11CurrentWirelessMode));

	// For debugging channel
	// RT_TRACE_F(COMP_P2P | COMP_MLME, DBG_LOUD, ("Channel Register: %x\n" , PHY_QueryRFReg(GetDefaultAdapter(Adapter), 0, RF_CHNLBW, bMaskDWord)));

	// Ibss or TIM element
	if( !ACTING_AS_AP(Adapter))
	{
		IBSSParms.Octet = (pu1Byte)(&pMgntInfo->mIbssParms.atimWin);
		IBSSParms.Length = 2;
		PacketMakeElement(&pMgntInfo->beaconframe, EID_IbssParms, IBSSParms);
	}
	else
	{
		// Update DTIM Count.
		if(pMgntInfo->mDtimCount == 0)
			pMgntInfo->mDtimCount = pMgntInfo->dot11DtimPeriod - 1;
		else 
			pMgntInfo->mDtimCount--;

		AP_PS_FillTim(pMgntInfo);

		PacketMakeElement(&pMgntInfo->beaconframe, EID_Tim, pMgntInfo->Tim);
	}

	DFS_ApConstructBeaconIEcsa(Adapter);
	
	// Wireless mode dependent elements.
	if( IS_WIRELESS_OFDM_24G(Adapter))
	{
		// Capability.
		if(ACTING_AS_IBSS(Adapter) == FALSE)
			UNION_BEACON_PROBE_RSP_CAPABILITY_INFO(pbcn, cShortSlotTime);

		if(	IS_WIRELESS_MODE_G(Adapter) ||
			(IS_WIRELESS_MODE_HT_24G(Adapter) && pMgntInfo->pHTInfo->bCurSuppCCK))
		{
			if(pMgntInfo->bUseProtection)
				ERPInfoContent[0] = ERP_UseProtection;

			FillOctetString(ERPInfo, ERPInfoContent, 1);
			PacketMakeElement(&pMgntInfo->beaconframe, EID_ERPInfo, ERPInfo);
		}

		// Extended Supported Rates.
		if(ExtSuppRates.Length != 0)
		{
			PacketMakeElement(&pMgntInfo->beaconframe, EID_ExtSupRates, ExtSuppRates);
		}
	}
	// <RJ_TODO> IS_WIRELESS_MODE_A()

	//
	// Append Additional IEs from Ndis6, for DTM Vista 1.0c premium logo test. By Bruce, 2007-08-13.
	//
	if(!ACTING_AS_AP(Adapter))
	{
		GetIbssAdditionalData(Adapter, &IbssAdditionalIEs);
		if(IbssAdditionalIEs.Octet != NULL && IbssAdditionalIEs.Length != 0)
		{
			AppendAdditionalIEs(&pMgntInfo->beaconframe, IbssAdditionalIEs);
		}
	}

	RT_DISP(FBEACON, BCN_SHOW, 
	("Support Mode=%x pMgntInfo->mIbss=%d pMgntInfo->dot11CurrentWirelessMode=%x\n", 
	Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter), pMgntInfo->mIbss, pMgntInfo->dot11CurrentWirelessMode));

	//
	// Include High Throuput capability and High Throuput info
	//	- Do not include HT if CurrentWirelessMode is less than N mode even if hardware support it
	//
	if(IS_WIRELESS_MODE_N(Adapter))
	{
		// Construct HTCapability and HTInfo IE
		OCTET_STRING		osHTCap, osHTInfo;
		
		FillOctetString(osHTCap, &(pMgntInfo->pHTInfo->SelfHTCap), sizeof(pMgntInfo->pHTInfo->SelfHTCap));
		FillOctetString(osHTInfo, &(pMgntInfo->pHTInfo->SelfHTInfo), sizeof(pMgntInfo->pHTInfo->SelfHTInfo));
		HTConstructCapabilityElement(Adapter, &osHTCap, FALSE);
		HTConstructInfoElement(Adapter, &osHTInfo);

		// TODO: Construct CAP info element here
		PacketMakeElement(&pMgntInfo->beaconframe, EID_HTCapability, osHTCap);
		PacketMakeElement(&pMgntInfo->beaconframe, EID_HTInfo, osHTInfo);

		// Construct Realtek Proprietary Aggregation mode (Set AMPDU Factor to 2, 32k)
		{
			OCTET_STRING		osRealtekIEType2;
			FillOctetString(osRealtekIEType2, pMgntInfo->RTIEType2Buffer, sizeof(pMgntInfo->RTIEType2Buffer));
			ConstructRTKAggElement(Adapter, &osRealtekIEType2);
			PacketMakeElement(&pMgntInfo->beaconframe, EID_Vendor, osRealtekIEType2);
		}
	}

	if(IS_WIRELESS_MODE_AC(Adapter))
	{
		OCTET_STRING		osVHTCap, osVHTOperation;

		FillOctetString(osVHTCap, &(pMgntInfo->pVHTInfo->SelfVHTCap), sizeof(pMgntInfo->pVHTInfo->SelfVHTCap));
		FillOctetString(osVHTOperation, &(pMgntInfo->pVHTInfo->SelfVHTInfo), sizeof(pMgntInfo->pVHTInfo->SelfVHTInfo));
		VHTConstructCapabilityElement(Adapter, &osVHTCap, FALSE);
		VHTConstructOperationElement(Adapter, &osVHTOperation);

		// TODO: Construct CAP & Operation element here
		PacketMakeElement(&pMgntInfo->beaconframe, EID_VHTCapability, osVHTCap);
		PacketMakeElement(&pMgntInfo->beaconframe, EID_VHTOperation, osVHTOperation);
	}

	
	//2004/07/22, kcwu, construct Security IE
	//2004/09/15, kcwu, decide to construct which kind of element,WPA or WPA2
	if(pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch)
	{
		PacketMakeElement(&pMgntInfo->beaconframe, (pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA)?EID_Vendor:EID_WPA2, pMgntInfo->SecurityInfo.RSNIE);
	}

	//
	// Append Additional IEs from Ndis6, for DTM Vista 1.0c premium logo test. By Bruce, 2007-08-13.
	//
	if(ACTING_AS_IBSS(Adapter))
	{
		GetIbssAdditionalData(Adapter, &IbssAdditionalIEs);
		if(IbssAdditionalIEs.Octet != NULL && IbssAdditionalIEs.Length != 0)
		{
			AppendAdditionalIEs(&pMgntInfo->beaconframe, IbssAdditionalIEs);
		}

		if(pMgntInfo->pStaQos->QosCapability != QOS_DISABLE)	
		{
			FillOctetString(osEDCAInfoElem, EDCAInfo, 1);
			PlatformZeroMemory(osEDCAInfoElem.Octet, 1); 
			AppendQoSElement(&pMgntInfo->beaconframe, &osEDCAInfoElem, OUI_SUBTYPE_WMM_INFO);
		}
	}
	
	//
	// Append additional IEs. By haich, 2008.08.01.
	//
	if(ACTING_AS_AP(Adapter))
	{
		if(pMgntInfo->AdditionalBeaconIESize > 0&&
			pMgntInfo->AdditionalBeaconIEData != NULL &&
			pMgntInfo->beaconframe.Length + pMgntInfo->AdditionalBeaconIESize <= sMaxMpduLng)
		{
			OCTET_STRING osAdditionalBeaconIEs;
			osAdditionalBeaconIEs.Length = (u2Byte)pMgntInfo->AdditionalBeaconIESize;
			osAdditionalBeaconIEs.Octet = (pu1Byte)pMgntInfo->AdditionalBeaconIEData;
			AppendAdditionalIEs(&pMgntInfo->beaconframe, osAdditionalBeaconIEs);
		}
		Ap_AppendWmmIe(Adapter, &pMgntInfo->beaconframe);
		Ap_AppendCountryIE(Adapter, &pMgntInfo->beaconframe);
		Ap_AppendPowerConstraintIE(Adapter, &pMgntInfo->beaconframe);
	}

	WAPI_SecFuncHandler(WAPI_CONSTRUCTBEACON, Adapter, WAPI_END);

	WPS_ConstructBeaconFrame(Adapter);

    
#if (P2P_SUPPORT == 1)
	P2P_Append_BeaconIe(Adapter);
#endif

	WFD_AppendBeaconIEs(Adapter, pMgntInfo->BcnSharedMemory.Length, &(pMgntInfo->beaconframe));
	
}


//
// Description: Construct a beacon frame according to current status and 
// 				update the beacon to HW beacon queue.
// 2005.06.15, by rcnjko.
//
VOID
UpdateBeaconFrame(
	PADAPTER		Adapter
	)
{
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);

	pDefaultAdapter->TxStats.NumTxBeaconUpdate++;
	ConstructBeaconFrame(Adapter);
	SendBeaconFrame(Adapter, BEACON_QUEUE);
}

BOOLEAN
Isdot11acAvailable(
	PADAPTER		Adapter
	)
{
	u4Byte	Availabledot11acCountry[] = {0x818, 0x368, 0x398, 0x417, 0x516, 0x643, 0x804, 0x360};
	u1Byte  index, result;
	
	result = TRUE;
	// RT_TRACE(COMP_INIT, DBG_LOUD, ("$$$ Isdot11acAvailable(): Adapter->bDisable11ac = %d\n", Adapter->bDisable11ac));
	if(Adapter->bDisable11ac)
	{
		result = FALSE;
		return result;
	}
	for(index=0; index<(sizeof(Availabledot11acCountry)/4); index++)
	{
		if(SMBIOS_GET_COUNTRY_CODE(Adapter) ==  Availabledot11acCountry[index])
		{
			result = FALSE;
			break;
		}
	}
	return result;
}


VOID
ConstructProbeRequest(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	BOOLEAN			bBroadcastBssid,
	BOOLEAN			bAnySsid,
	BOOLEAN			bForcePowerSave
	)
{
	PMGNT_INFO      	pMgntInfo = &Adapter->MgntInfo;	
	OCTET_STRING		os;
	OCTET_STRING		ProbeReq;
	pu1Byte			pProbeRequestPartial;
	OCTET_STRING		SuppRates;
	u1Byte			SuppRatesContent[8]; // NOTE! Length of Support Rates <= 8.
	OCTET_STRING		ExtSuppRates;
	u1Byte			ExtSuppRatesContent[255]; // NOTE! Length of Support Rates <= 255.
	u1Byte			RegSuppRatesIdx=0;
	WIRELESS_MODE	wirelessmode = WIRELESS_MODE_UNKNOWN;
	
	pProbeRequestPartial = Buffer;

	SET_80211_HDR_FRAME_CONTROL(pProbeRequestPartial, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pProbeRequestPartial, Type_Probe_Req);
	SET_80211_HDR_PWR_MGNT(pProbeRequestPartial, (bForcePowerSave) ? 1 : 0);
	SET_80211_HDR_DURATION(pProbeRequestPartial, 0);
	SET_80211_HDR_ADDRESS1(pProbeRequestPartial, BroadcastAddress);
	SET_80211_HDR_ADDRESS2(pProbeRequestPartial, Adapter->CurrentAddress);	
	if(bBroadcastBssid)
	{
		SET_80211_HDR_ADDRESS3(pProbeRequestPartial, BroadcastAddress);
	}
	else
	{
		SET_80211_HDR_ADDRESS3(pProbeRequestPartial, Adapter->MgntInfo.Bssid);
	}
		
	SET_80211_HDR_FRAGMENT_SEQUENCE(pProbeRequestPartial, 0);

	//3 Size of Probe Request, can not use sizeof(GeneralPacketPartial)
	FillOctetString(ProbeReq, Buffer, 0x18);

	// TODO: Change length
	if(bAnySsid)
	{
		FillOctetString(os, NULL, 0);
		PacketMakeElement(&ProbeReq, EID_SsId, os);
	}
	else
	{
		PacketMakeElement(&ProbeReq, EID_SsId, pMgntInfo->Ssid);
	}

	// Supported rates
	FillOctetString(SuppRates, SuppRatesContent, 0);
	FillOctetString(ExtSuppRates, ExtSuppRatesContent, 0);
	
	//
	// Basic rate setting should follow default setting in registry
	// instead of CurrentWirelessMode. Otherwise, previous connection may effect on
	// later connection. Neo, 2011/11/9	

	// 2011/03/17 MH For 92D, we need to change RegWirelessMode when dual mac contrl algorithm
	// want to switch between 2.4G & 5G.
	if (IS_WIRELESS_MODE_24G(Adapter) )
	{
		RegSuppRatesIdx = 2;
		wirelessmode = WIRELESS_MODE_N_24G;

		if (Adapter->RegWirelessMode == WIRELESS_MODE_B)
		{
			RegSuppRatesIdx = 1;
			wirelessmode = WIRELESS_MODE_B;
		}
		else if (Adapter->RegWirelessMode == WIRELESS_MODE_G)
			wirelessmode = WIRELESS_MODE_G;
		else if(Adapter->RegWirelessMode == WIRELESS_MODE_AC_24G)
			wirelessmode = WIRELESS_MODE_AC_24G;
	}
	else if (IS_WIRELESS_MODE_5G(Adapter))
	{
		RegSuppRatesIdx = 0;

		if(Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter) & WIRELESS_MODE_AC_5G)
			wirelessmode = WIRELESS_MODE_AC_5G;
		else
			wirelessmode = WIRELESS_MODE_N_5G;
		
		if (Adapter->RegWirelessMode == WIRELESS_MODE_AC_5G)
			wirelessmode = WIRELESS_MODE_AC_5G;
		else if (Adapter->RegWirelessMode == WIRELESS_MODE_N_5G)
			wirelessmode = WIRELESS_MODE_N_5G;
		else if (Adapter->RegWirelessMode == WIRELESS_MODE_A)
			wirelessmode = WIRELESS_MODE_A;
	}
#if 0 
	if(!Isdot11acAvailable(Adapter))
	{
		if(wirelessmode == WIRELESS_MODE_AC_5G)	
		{
			wirelessmode = WIRELESS_MODE_N_5G;
		}
		if(wirelessmode == WIRELESS_MODE_AC_24G)
		{
			wirelessmode = WIRELESS_MODE_N_24G;
		}
		RT_TRACE(COMP_MLME, DBG_TRACE, ("ConstructProbeRequest(): wirelessmode = %d\n", wirelessmode));
	}
#endif

	SelectSupportedRatesElement( wirelessmode, pMgntInfo->RegSuppRateSets[RegSuppRatesIdx], FALSE,&SuppRates, &ExtSuppRates );

	PacketMakeElement(&ProbeReq, EID_SupRates, SuppRates);
	// Extended supported rates
	if(ExtSuppRates.Length != 0)
	{
		PacketMakeElement(&ProbeReq, EID_ExtSupRates, ExtSuppRates);
	}


	//
	// Simple config IE. by CCW - copy from 818x
	//
	if(GET_SIMPLE_CONFIG_ENABLED(pMgntInfo))
	{		
		WPS_AppendElement(Adapter, &ProbeReq, TRUE, WPS_INFO_PROBEREQ_IE);
	}

	//
	// For CCXv4 S59.2.3
	//
	CCX_RM_AppendRmCapIE(Adapter, &ProbeReq);
	

	//merge from 1020 to pass WHCK Scan_AdditionalIE
	if(	pMgntInfo->AdditionalProbeReqIESize > 0&&
		pMgntInfo->AdditionalProbeReqIEData != NULL &&
		ProbeReq.Length + pMgntInfo->AdditionalProbeReqIESize <= sMaxMpduLng)
	{
		OCTET_STRING osAdditionalIEs;
		osAdditionalIEs.Length = (u2Byte)pMgntInfo->AdditionalProbeReqIESize;
		osAdditionalIEs.Octet = (pu1Byte)pMgntInfo->AdditionalProbeReqIEData;
		

		AppendAdditionalIEs(&ProbeReq, osAdditionalIEs);
	}


	//
	// HT Capability IE. by Joseph - according to 802.11n latest spec.
	//
	if(	(Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter)>WIRELESS_MODE_AUTO) &&
		(Adapter->RegWirelessMode >= WIRELESS_MODE_AUTO))
	{
		FillOctetString(os, &(pMgntInfo->pHTInfo->SelfHTCap), sizeof(pMgntInfo->pHTInfo->SelfHTCap));
		HTConstructCapabilityElement(Adapter, &os, FALSE);
		PacketMakeElement(&ProbeReq, EID_HTCapability, os);
	}

	if(pMgntInfo->AdditionalProbeReqIESize > 0&&
		pMgntInfo->AdditionalProbeReqIEData != NULL &&
		ProbeReq.Length + pMgntInfo->AdditionalProbeReqIESize <= sMaxMpduLng)
	{
		OCTET_STRING osAdditionalIEs;
		osAdditionalIEs.Length = (u2Byte)pMgntInfo->AdditionalProbeReqIESize;
		osAdditionalIEs.Octet = (pu1Byte)pMgntInfo->AdditionalProbeReqIEData;
		

		AppendAdditionalIEs(&ProbeReq, osAdditionalIEs);
	}

	if(wirelessmode == WIRELESS_MODE_AC_5G || wirelessmode == WIRELESS_MODE_AC_24G)
	{
		FillOctetString(os, &(pMgntInfo->pVHTInfo->SelfVHTCap), sizeof(pMgntInfo->pVHTInfo->SelfVHTCap));
		VHTConstructCapabilityElement(Adapter, &os, FALSE);
		PacketMakeElement(&ProbeReq, EID_VHTCapability, os);
	}

#if P2P_SUPPORT == 1
	// Solution for service discovery.
	P2P_Append_ProbeReqIe(&ProbeReq, GET_P2P_INFO(Adapter));
#endif

	// Append WFD Probe request IEs
	WFD_AppendProbeReqIEs(Adapter, Adapter->MAX_TRANSMIT_BUFFER_SIZE, &ProbeReq);
	
	*pLength=ProbeReq.Length;
}

//
//	Description:
//		Advanced version of ConstructProbeRequest, please refer to SendProbeReq for
//		more information.
//	2006.12.20, by shien chang.
//
VOID
ConstructProbeRequestEx(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	OCTET_STRING	Ssid,
	BOOLEAN			bForcePowerSave
	)
{
	PMGNT_INFO      		pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING			ProbeReq;
	pu1Byte				pProbeRequestPartial;
	OCTET_STRING			SuppRates;
	u1Byte				SuppRatesContent[8]; // NOTE! Length of Support Rates <= 8.
	OCTET_STRING			ExtSuppRates;
	u1Byte				ExtSuppRatesContent[255]; // NOTE! Length of Support Rates <= 255.
	OCTET_STRING		os;
	u1Byte				RegSuppRatesIdx=0;
	WIRELESS_MODE		wirelessmode = WIRELESS_MODE_UNKNOWN;
	
	pProbeRequestPartial = Buffer;

	SET_80211_HDR_FRAME_CONTROL(pProbeRequestPartial, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pProbeRequestPartial, Type_Probe_Req);
	SET_80211_HDR_PWR_MGNT(pProbeRequestPartial, (bForcePowerSave) ? 1 : 0);
	SET_80211_HDR_DURATION(pProbeRequestPartial, 0);
	SET_80211_HDR_ADDRESS1(pProbeRequestPartial, BroadcastAddress);
	SET_80211_HDR_ADDRESS2(pProbeRequestPartial, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pProbeRequestPartial, BroadcastAddress);
		
	SET_80211_HDR_FRAGMENT_SEQUENCE(pProbeRequestPartial, 0);

	//3 Size of Probe Request, can not use sizeof(GeneralPacketPartial)
	FillOctetString(ProbeReq, Buffer, 0x18);

	// Probe request with SSID.
	PacketMakeElement(&ProbeReq, EID_SsId, Ssid);

	// Supported rates
//	PacketMakeElement(&ProbeReq, EID_SupRates, Adapter->MgntInfo.SupportedRates);
	FillOctetString(SuppRates, SuppRatesContent, 0);
	FillOctetString(ExtSuppRates, ExtSuppRatesContent, 0);

	//
	// Basic rate setting should follow default setting in registry
	// instead of CurrentWirelessMode. Otherwise, previous connection may effect on
	// later connection. Neo, 2011/11/9	

	// 2011/03/17 MH For 92D, we need to change RegWirelessMode when dual mac contrl algorithm
	// want to switch between 2.4G & 5G.
	if (IS_WIRELESS_MODE_24G(Adapter) )
	{
		RegSuppRatesIdx = 2;
		wirelessmode = WIRELESS_MODE_N_24G;

		if (Adapter->RegWirelessMode == WIRELESS_MODE_B)
		{
			RegSuppRatesIdx = 1;
			wirelessmode = WIRELESS_MODE_B;
		}
		else if (Adapter->RegWirelessMode == WIRELESS_MODE_G)
			wirelessmode = WIRELESS_MODE_G;
		else if(Adapter->RegWirelessMode == WIRELESS_MODE_AC_24G)
			wirelessmode = WIRELESS_MODE_AC_24G;
	}
	else if (IS_WIRELESS_MODE_5G(Adapter))
	{
		RegSuppRatesIdx = 0;

		if(Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter) & WIRELESS_MODE_AC_5G)
			wirelessmode = WIRELESS_MODE_AC_5G;
		else
			wirelessmode = WIRELESS_MODE_N_5G;
		
		if (Adapter->RegWirelessMode == WIRELESS_MODE_AC_5G)
			wirelessmode = WIRELESS_MODE_AC_5G;
		else if (Adapter->RegWirelessMode == WIRELESS_MODE_N_5G)
			wirelessmode = WIRELESS_MODE_N_5G;
		else if (Adapter->RegWirelessMode == WIRELESS_MODE_A)
			wirelessmode = WIRELESS_MODE_A;
	}

	SelectSupportedRatesElement( wirelessmode, pMgntInfo->RegSuppRateSets[RegSuppRatesIdx], FALSE,&SuppRates, &ExtSuppRates );
	PacketMakeElement(&ProbeReq, EID_SupRates, SuppRates);
	// Extended supported rates
	if(ExtSuppRates.Length != 0)
	{
		PacketMakeElement(&ProbeReq, EID_ExtSupRates, ExtSuppRates);
	}

	//
	// Simple config IE. by CCW - copy from 818x
	//
	if(GET_SIMPLE_CONFIG_ENABLED(pMgntInfo))
	{
		WPS_AppendElement(Adapter, &ProbeReq, TRUE, WPS_INFO_PROBEREQ_IE);
	}

	//
	// HT Capability IE. by Joseph - according to 802.11n latest spec.
	//
	if((Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter)>WIRELESS_MODE_AUTO) &&
		(Adapter->RegWirelessMode >= WIRELESS_MODE_AUTO))
	{
		FillOctetString(os, &(pMgntInfo->pHTInfo->SelfHTCap), sizeof(pMgntInfo->pHTInfo->SelfHTCap));
		HTConstructCapabilityElement(Adapter, &os, FALSE);
		PacketMakeElement(&ProbeReq, EID_HTCapability, os);
	}

#if P2P_SUPPORT == 1
	// Solution for service discovery.
	P2P_Append_ProbeReqIe(&ProbeReq, GET_P2P_INFO(Adapter));
#endif	

	// Append WFD Probe request IEs
	WFD_AppendProbeReqIEs(Adapter, Adapter->MAX_TRANSMIT_BUFFER_SIZE, &ProbeReq);
	
	if((Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter) & WIRELESS_MODE_AC_5G) &&
		(Adapter->RegWirelessMode ==WIRELESS_MODE_AUTO || Adapter->RegWirelessMode == WIRELESS_MODE_AC_5G))
	{
		FillOctetString(os, &(pMgntInfo->pVHTInfo->SelfVHTCap), sizeof(pMgntInfo->pVHTInfo->SelfVHTCap));
		VHTConstructCapabilityElement(Adapter, &os, FALSE);
		PacketMakeElement(&ProbeReq, EID_VHTCapability, os);
	}
	
	*pLength=ProbeReq.Length;
}

VOID
ConstructAuthenticatePacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			auStaAddr,
	u1Byte			AuthAlg,
	u1Byte			AuthSeq,
	u1Byte			AuthStatusCode,
	OCTET_STRING		AuthChallengetext
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	pu1Byte			pAuthPartial = Buffer;
	OCTET_STRING	osAuthPkt = {Buffer, 0};
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> ConstructAuthenticatePacket()\n"));

	SET_80211_HDR_FRAME_CONTROL(osAuthPkt.Octet, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(osAuthPkt.Octet, Type_Auth);
	SET_80211_HDR_DURATION(osAuthPkt.Octet, 0);
	
	SET_80211_HDR_ADDRESS1(osAuthPkt.Octet, auStaAddr);
	SET_80211_HDR_ADDRESS2(osAuthPkt.Octet, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(osAuthPkt.Octet, Adapter->MgntInfo.Bssid);
	SET_80211_HDR_FRAGMENT_SEQUENCE(osAuthPkt.Octet, 0);

	if(AuthSeq != 3)
	{
		//add for CCX NETWORK EAP (LEAP) 2006.07.31 ,by CCW 
		if( !Adapter->MgntInfo.bNETWORKEAP)
			SET_AUTH_FRAME_AUTH_ALG_NUM(osAuthPkt.Octet, AuthAlg); // Auth Algorithm:	Open system
		else
			SET_AUTH_FRAME_AUTH_ALG_NUM(osAuthPkt.Octet, NETWORK_EAP); //Auth Algorithm:   NETWORK EAP

		SET_AUTH_FRAME_AUTH_SEQ_NUM(osAuthPkt.Octet, AuthSeq); // Auth Seq Num
		SET_AUTH_FRAME_STATUS_CODE(osAuthPkt.Octet, AuthStatusCode); // Status Code:		Reserved

		osAuthPkt.Length = 30;

		if( AuthChallengetext.Length != 0 ){
			pu1Byte	buf = GET_AUTH_FRAME_CHALLENG_TEXT(osAuthPkt.Octet);
			buf[0] = (u1Byte)EID_Ctext;
			buf[1] = (u1Byte)AuthChallengetext.Length;
			PlatformMoveMemory( buf + 2, AuthChallengetext.Octet, AuthChallengetext.Length);
			osAuthPkt.Length += (AuthChallengetext.Length + 2);
		}
		else if(1 == AuthSeq)
		{
				//4 // RSN IE
				if(RT_STATUS_SUCCESS != (rtStatus = 
					Sec_AppendRSNIE(
						Adapter, // Adapter context
						(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
						&(pMgntInfo->targetBSS), // target info
						sizeof(RT_WLAN_BSS), // target info length
						Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
						&osAuthPkt	// Packet Content octet string
						)))
				{
					RT_TRACE_F(COMP_SEC, DBG_TRACE, ("Fail (0x%08X) to append RSN IE\n", rtStatus));
				}

				//4 // Fast Transition MD IE
				if(RT_STATUS_SUCCESS != (rtStatus = 
					FT_AppendMdIE(
							Adapter, // Adapter context
							(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
							&(pMgntInfo->targetBSS), // target info
							sizeof(RT_WLAN_BSS), // target info length
							Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
							&osAuthPkt	// Packet Content octet string
							)))
				{
					RT_TRACE_F(COMP_MLME, DBG_TRACE, ("Fail (0x%08X) to append MD IE\n", rtStatus));
				}

				//4 // Fast Transition FT IE
				if(RT_STATUS_SUCCESS != (rtStatus = 
					FT_AppendFtIE(
							Adapter, // Adapter context
							(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
							&(pMgntInfo->targetBSS), // target info
							sizeof(RT_WLAN_BSS), // target info length
							Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
							&osAuthPkt	// Packet Content octet string
							)))
				{
					RT_TRACE_F(COMP_MLME, DBG_TRACE, ("Fail (0x%08X) to append FT IE\n", rtStatus));
				}
		}
	}
	else
	{ // 3rd auth frame. 2005.03.08, by rcnjko.
		// Set WEP bit, see also MgntGetEncryptionInfo(), 2005.08.18, by rcnjko.
		SET_80211_HDR_WEP(osAuthPkt.Octet, 1);

		// Auth Alg Num. 
		*((UNALIGNED pu2Byte)(osAuthPkt.Octet+28)) = EF2Byte(AuthAlg); // 24+4+0, 802.11 Header + IV + field offset
		// Auth Seq Num.
		*((UNALIGNED pu2Byte)(osAuthPkt.Octet+30)) = EF2Byte(AuthSeq); // 24+4+2
		// Status Code. 
		*((UNALIGNED pu2Byte)(osAuthPkt.Octet+32)) = EF2Byte(AuthStatusCode); // 24+4+4
		osAuthPkt.Length = 34; // 24+4+6

		// Challenge text.
		if( AuthChallengetext.Length != 0 )
		{
			pu1Byte buf = osAuthPkt.Octet + 34; // 24+4+6

			buf[0] = (u1Byte)EID_Ctext;
			buf[1] = (u1Byte)AuthChallengetext.Length;
			PlatformMoveMemory( buf + 2, AuthChallengetext.Octet, AuthChallengetext.Length);
			osAuthPkt.Length += (AuthChallengetext.Length + 2);
		}
	}
	*pLength = osAuthPkt.Length;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== ConstructAuthenticatePacket()\n"));
}

VOID
ConstructAssociateReqEpig(
	IN		PADAPTER		Adapter,
	IN		POCTET_STRING	pAsocReq,
	IN		POCTET_STRING	pVHTCap
	)
{
	OCTET_STRING			EpigIE;
	u1Byte					EpigIEContent[100];

	if(Adapter->MgntInfo.bRegIOTBcm256QAM == FALSE)
		return;

	FillOctetString(EpigIE, EpigIEContent, 0);

	{
		u1Byte	Epigram11ACCap[7] = {0x00, 0x90, 0x4C, 0x04, 0x08, 0xBF, 0x0C} ;
		PlatformMoveMemory(EpigIEContent, Epigram11ACCap, 7);
		PlatformMoveMemory(EpigIEContent+7, pVHTCap->Octet, pVHTCap->Length);
		EpigIE.Length = 19;
		PacketMakeElement(pAsocReq, EID_Vendor, EpigIE);
	}

}


VOID
ConstructAssociateReq(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			asocStaAddr,
	u2Byte			asocCap,
	u2Byte			asocListenInterval,
	OCTET_STRING	asocSsid,
	OCTET_STRING	asocSuppRates
	)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING			AsocReq;
	pu1Byte					pAsocPartial;
	OCTET_STRING			SuppRates;
	u1Byte					SuppRatesContent[8]; // NOTE! Length of Support Rates <= 8.
	OCTET_STRING			ExtSuppRates;
	u1Byte					ExtSuppRatesContent[255]; // NOTE! Length of Extended Support Rates <= 255.
	BOOLEAN					bFilterCck = FALSE;
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> ConstructAssociateReq()\n"));

	//
	// Construct CCXv5 Special Association Req Packet for S64 Diagnostic Channel.
	// By Bruce, 2009-09-21.
	//
	if (CCX_Construct_DiagChnl_AssocReq(Adapter, Buffer, pLength))
		return;
	
	pAsocPartial = Buffer;

	SET_80211_HDR_FRAME_CONTROL(pAsocPartial, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pAsocPartial, Type_Asoc_Req);
	SET_80211_HDR_DURATION(pAsocPartial, 0);

	SET_80211_HDR_ADDRESS1(pAsocPartial, asocStaAddr);
	SET_80211_HDR_ADDRESS2(pAsocPartial, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pAsocPartial, Adapter->MgntInfo.Bssid);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pAsocPartial, 0);

	// TODO: Change this
	SET_ASOC_REQ_CAPABILITY_INFO(pAsocPartial, asocCap);
	SET_ASOC_REQ_LISTEN_INTERVAL(pAsocPartial, asocListenInterval);

	//3 Size of Associate Request, can not use sizeof(AssociateReqPartial)
	FillOctetString(AsocReq, Buffer, 0x1c);

	// SSID element
	PacketMakeElement(&AsocReq, EID_SsId, asocSsid);

	P2P_ConstructAssociateReqFilterCck(Adapter, AsocReq, &bFilterCck);

	// Supported rates element
	FillOctetString(SuppRates, SuppRatesContent, 0);
	FillOctetString(ExtSuppRates, ExtSuppRatesContent, 0);
	SelectSupportedRatesElement( pMgntInfo->dot11CurrentWirelessMode, asocSuppRates, bFilterCck, &SuppRates, &ExtSuppRates );
	PacketMakeElement(&AsocReq, EID_SupRates, SuppRates);

	// Extended supported rates
	if(ExtSuppRates.Length != 0)
	{
		PacketMakeElement(&AsocReq, EID_ExtSupRates, ExtSuppRates);
	}

	if(!GET_SIMPLE_CONFIG_ENABLED(pMgntInfo))
	{
		// RSNIE. 2004/07/22, kcwu.
		if(pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch)
		{
			//Fix WPA/WPA2 AP connect fail when wapi is support
			//Modified by ylb, 20110906
			if (pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWAPI || pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeCertificateWAPI )
			{
				WAPI_SecFuncHandler(WAPI_CONSTRUCTASSOCIATEREQ, Adapter, (PVOID)&AsocReq, WAPI_END);
			}
			else
			{
				//4 // WPA IE
				if(RT_STATUS_SUCCESS != (rtStatus = 
					Sec_AppendWPAIE(
					Adapter, // Adapter context
					(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
					&(pMgntInfo->targetBSS), // target info
					sizeof(RT_WLAN_BSS), // target info length
					Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
					&AsocReq	// Packet Content octet string
					)))
				{
					RT_TRACE_F(COMP_SEC, DBG_TRACE, ("Fail (0x%08X) to append WPA IE\n", rtStatus));
				}

				//4 // RSN IE
				if(RT_STATUS_SUCCESS != (rtStatus = 
					Sec_AppendRSNIE(
						Adapter, // Adapter context
						(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
						&(pMgntInfo->targetBSS), // target info
						sizeof(RT_WLAN_BSS), // target info length
						Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
						&AsocReq	// Packet Content octet string
						)))
				{
					RT_TRACE_F(COMP_SEC, DBG_TRACE, ("Fail (0x%08X) to append RSN IE\n", rtStatus));
				}

				//4 // Fast Transition MD IE
				if(RT_STATUS_SUCCESS != (rtStatus = 
					FT_AppendMdIE(
							Adapter, // Adapter context
							(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
							&(pMgntInfo->targetBSS), // target info
							sizeof(RT_WLAN_BSS), // target info length
							Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
							&AsocReq	// Packet Content octet string
							)))
				{
					RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Fail (0x%08X) to append MD IE\n", rtStatus));
				}
			}
		}
	}
	
	DFS_StaConstructAssociateReq(Adapter, asocCap, AsocReq);
	
	if( pMgntInfo->pStaQos->CurrentQosMode > QOS_DISABLE )
	{
		PacketMakeElement( &AsocReq, EID_Vendor, pMgntInfo->pStaQos->WMMIE );
	}

	// Append CCX Aironet IE for CCXv1 S19, S13, CCXv4 S61.
	CCX_AppendAironetIE(Adapter, &AsocReq);

	// CCX 2 S31 AP Control of Client Transmit Power (TPC).
	CCX_AppendCcxCellPowerIE(Adapter, &AsocReq);

	// For CCX 2 S36, Radio Management Capability element, 2006.05.15, by rcnjko.
	CCX_RM_AppendRmCapIE(Adapter, &AsocReq);
	
	// For CCX 2 S38, WLAN Device Version Number element.
	CCX_AppendCcxVerIE(Adapter, &AsocReq);

	// For CCX S67 MFP
	CCX_AppendCcxSFAIE(Adapter, &AsocReq);

	// Include High Throuput capability
	if( pMgntInfo->pHTInfo->bCurrentHTSupport )
	{		
		// Construct HTCapability and HTInfo IE
		OCTET_STRING		osHTCap;	
		RT_TRACE(COMP_HT, DBG_LOUD, ("ConstructAssociateReq(): Trying to associate to an 802.11n HT AP\n"));
		FillOctetString(osHTCap, &(pMgntInfo->pHTInfo->SelfHTCap), sizeof(pMgntInfo->pHTInfo->SelfHTCap));
		HTConstructCapabilityElement(Adapter, &osHTCap, TRUE);
		if(pMgntInfo->pHTInfo->ePeerHTSpecVer == HT_SPEC_VER_EWC)
			PacketMakeElement(&AsocReq, EID_Vendor, osHTCap);
		else
			PacketMakeElement(&AsocReq, EID_HTCapability, osHTCap);

		BSS_AppendExentedCapElement(Adapter, &AsocReq);	
	}

	if(pMgntInfo->pVHTInfo->bCurrentVHTSupport )
	{		
		OCTET_STRING		osVHTCap;	
		u1Byte				opmode = 0;
		OCTET_STRING		osVHTNotif;
		CHANNEL_WIDTH		opNotifBW = CHNL_GetRegBWSupport(Adapter);
		u1Byte				opNotifNss = pMgntInfo->pVHTInfo->nRxSPStream;
		
		RT_TRACE(COMP_HT, DBG_LOUD, ("ConstructAssociateReq(): Trying to associate to an 802.11ac VHT AP\n"));
		FillOctetString(osVHTCap, &(pMgntInfo->pVHTInfo->SelfVHTCap), sizeof(pMgntInfo->pVHTInfo->SelfVHTCap));
		VHTConstructCapabilityElement(Adapter, &osVHTCap, TRUE);
		PacketMakeElement(&AsocReq, EID_VHTCapability, osVHTCap);

		if(pMgntInfo->pVHTInfo->bOpModeNotif)
		{
			opNotifBW = pMgntInfo->pVHTInfo->BWToSwitch;
			DbgPrint("pMgntInfo->pVHTInfo->bOpModeNotif = TRYE BW = %d\n", opNotifBW);
			opNotifNss = pMgntInfo->pVHTInfo->RxSSToSwitch;
		}
		
		RT_TRACE(COMP_HT, DBG_LOUD, ("ConstructAssociateReq(): carry operating mode notification with BW(%d) RxSS(%d)\n", opNotifBW, opNotifNss));
		FillOctetString(osVHTNotif, &opmode, sizeof(opmode));
		SET_VHT_OPERATING_MODE_FIELD_CHNL_WIDTH(&opmode, opNotifBW);
		SET_VHT_OPERATING_MODE_FIELD_RX_NSS(&opmode, (opNotifNss-1));
		SET_VHT_OPERATING_MODE_FIELD_RX_NSS_TYPE(&opmode, pMgntInfo->pVHTInfo->RxSSTypeBfmeeToSwitch);

		PacketMakeElement(&AsocReq, EID_OpModeNotification, osVHTNotif);
	}
	
	// For WiFi VHT Testbed, it requires special feature: VHT under TKIP & WEP mode
	if(pMgntInfo->VhtWeakSecurity > 0)
	{
		OCTET_STRING		osHTCap;
		OCTET_STRING		osVHTCap;	
		WIRELESS_MODE		wirelessModeBackup = pMgntInfo->dot11CurrentWirelessMode;

		RT_TRACE(COMP_HT, DBG_LOUD, ("ConstructAssociateReq(): Trying to associate to an 802.11n HT AP\n"));
		RT_TRACE(COMP_HT, DBG_LOUD, ("ConstructAssociateReq(): carry VHT IE  due to WiFi tested special feature\n"));
		FillOctetString(osHTCap, &(pMgntInfo->pHTInfo->SelfHTCap), sizeof(pMgntInfo->pHTInfo->SelfHTCap));
		FillOctetString(osVHTCap, &(pMgntInfo->pVHTInfo->SelfVHTCap), sizeof(pMgntInfo->pVHTInfo->SelfVHTCap));

		pMgntInfo->dot11CurrentWirelessMode = WIRELESS_MODE_AC_5G;
		MgntRefreshSuppRateSet(Adapter);
		pMgntInfo->dot11CurrentWirelessMode = wirelessModeBackup;
		
		// HT IE
		HTConstructCapabilityElement(Adapter, &osHTCap, TRUE);
		PacketMakeElement(&AsocReq, EID_HTCapability, osHTCap);
		// VHT IE
		VHTConstructCapabilityElement(Adapter, &osVHTCap, TRUE);
		PacketMakeElement(&AsocReq, EID_VHTCapability, osVHTCap);
	}
	
	// Construct Realtek Proprietary Aggregation mode (Set AMPDU Factor to 2, 32k)	
	//2008.08.13 If connect to realtek ap, append realtek OUI for WOW
	if(pMgntInfo->bRealtekAggCapExist)
	{
		OCTET_STRING		osRealtekIEType2;	
		FillOctetString(osRealtekIEType2, pMgntInfo->RTIEType2Buffer, sizeof(pMgntInfo->RTIEType2Buffer));
		ConstructRTKAggElement(Adapter, &osRealtekIEType2);
		PacketMakeElement(&AsocReq, EID_Vendor, osRealtekIEType2);
	}

	//
	// Simple config IE. by CCW - copy from 818x
	//
	if(GET_SIMPLE_CONFIG_ENABLED(pMgntInfo))
	{
		WPS_AppendElement(Adapter, &AsocReq, FALSE, WPS_INFO_ASOCREQ_IE);
	}
	
	//
	// CCX4 CAC -- TSPEC IE, 2006.06.26, by shien chang.
	//
	if (CCX_CAC_IsVoiceTsExist(Adapter))
	{
		WMM_TSPEC		tsVoice, tsSignal;
		OCTET_STRING	VoiceEle, SignalEle;
		PQOS_TSTREAM	pTs = NULL;
		PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
		u1Byte			tidVoice = 0;

		RT_TRACE(COMP_MLME, DBG_LOUD, ("ConstructAssociateReq(): CCX_CAC_ConstructVoiceTspec...\n"));
		CCX_CAC_ConstructVoiceTspec(Adapter, tsVoice);
		FillOctetString(VoiceEle, &tsVoice[2], TSPEC_SIZE - 2);
		PacketMakeElement( &AsocReq, EID_Vendor, VoiceEle );

		RT_TRACE(COMP_MLME, DBG_LOUD, ("ConstructAssociateReq(): CCX_CAC_ConstructSignalTspec...\n"));
		CCX_CAC_ConstructSignalTspec(Adapter, tsSignal);
		FillOctetString(SignalEle, &tsSignal[2], TSPEC_SIZE - 2);
		PacketMakeElement( &AsocReq, EID_Vendor, SignalEle );

		// TSRS IE
		pTs = &(pStaQos->StaTsArray[tidVoice]);
		if (QOS_RATE_TO_BPS( QosGetNPR(Adapter, pTs) ) >=
			GET_TSPEC_MIN_PHY_RATE(pTs->OutStandingTSpec) )
		{
			QosConstructTSRS(Adapter, pTs, &AsocReq);
		}
	}

#if (WPS_SUPPORT == 1)
	// Append customized IE, by hpfan
	if(pMgntInfo->bCustomizedAsocIE)
	{
		OCTET_STRING	osCustomizedIE;

		RT_PRINT_DATA(COMP_MLME, DBG_LOUD, ("ConstructAssociateReq(): append customzied association req IE\n"), pMgntInfo->CustomizedAsocIEBuf, pMgntInfo->CustomizedAsocIELength);
		osCustomizedIE.Length = pMgntInfo->CustomizedAsocIELength;
		osCustomizedIE.Octet = &pMgntInfo->CustomizedAsocIEBuf[0];
		AppendAdditionalIEs(&AsocReq, osCustomizedIE);
	}
#endif	
	
	//
	// Append additional IEs. By haich, 2008.11.27.
	//
	RT_PRINT_DATA(COMP_OID_SET | COMP_MLME, DBG_LOUD, "ConstructAssociateReq(): append AssocReqIE", 
			pMgntInfo->AdditionalAssocReqIEData, pMgntInfo->AdditionalAssocReqIESize);
	
	if(pMgntInfo->AdditionalAssocReqIESize > 0&&
		pMgntInfo->AdditionalAssocReqIEData != NULL &&
		AsocReq.Length + pMgntInfo->AdditionalAssocReqIESize <= sMaxMpduLng)
	{
		OCTET_STRING osAdditionalIEs;
		osAdditionalIEs.Length = (u2Byte)pMgntInfo->AdditionalAssocReqIESize;
		osAdditionalIEs.Octet = (pu1Byte)pMgntInfo->AdditionalAssocReqIEData;
		

		AppendAdditionalIEs(&AsocReq, osAdditionalIEs);
	}

#if (P2P_SUPPORT == 1)
	P2P_Append_AssociationReqIe(&AsocReq, Adapter);
#endif
    
	WFD_AppendAssocReqIEs(Adapter, Adapter->MAX_TRANSMIT_BUFFER_SIZE, &AsocReq);
	
	*pLength=AsocReq.Length;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== ConstructAssociateReq()\n"));
}

VOID
ConstructReAssociateReq(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			ReasocStaAddr,
	u2Byte			ReasocCap,
	u2Byte			ReasocListenInterval,
	pu1Byte			CurrentasocStaAddr,
	OCTET_STRING	ReasocSsid,
	OCTET_STRING	ReasocSuppRates
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;	
	OCTET_STRING		ReAsocReq;
	pu1Byte				pReAsocPartial;
	OCTET_STRING		SuppRates;
	u1Byte				SuppRatesContent[8]; // NOTE! Length of Support Rates <= 8.
	OCTET_STRING		ExtSuppRates;
	u1Byte				ExtSuppRatesContent[255]; // NOTE! Length of Extended Support Rates <= 255.
	OCTET_STRING		CCKMIE;
	u1Byte				CCKMIEBuff[30] ={0};
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> ConstructReAssociateReq()\n"));
	
	pReAsocPartial = Buffer;
	//Init CCKM IE
	CCKMIE.Octet = CCKMIEBuff;

	SET_80211_HDR_FRAME_CONTROL(pReAsocPartial, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pReAsocPartial, Type_Reasoc_Req);
	SET_80211_HDR_DURATION(pReAsocPartial, 0);

	SET_80211_HDR_ADDRESS1(pReAsocPartial, ReasocStaAddr);
	SET_80211_HDR_ADDRESS2(pReAsocPartial, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pReAsocPartial, ReasocStaAddr);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pReAsocPartial, 0);

	// TODO: Change this
	SET_REASOC_REQ_CAPABILITY_INFO(pReAsocPartial, ReasocCap);
	SET_REASOC_REQ_LISTEN_INTERVAL(pReAsocPartial, ReasocListenInterval);

	PlatformMoveMemory( GET_REASOC_REQ_CURR_AP_ADDR(pReAsocPartial), CurrentasocStaAddr, 6);

	//3 Size of Associate Request, can not use sizeof(AssociateReqPartial)
	FillOctetString(ReAsocReq, Buffer, 0x22);

	// SSID element
	PacketMakeElement(&ReAsocReq, EID_SsId, ReasocSsid);

	// Supported rates element
	//		PacketMakeElement(&AsocReq, EID_SupRates, Adapter->MgntInfo.SupportedRates);
	FillOctetString(SuppRates, SuppRatesContent, 0);
	FillOctetString(ExtSuppRates, ExtSuppRatesContent, 0);
	SelectSupportedRatesElement( pMgntInfo->dot11CurrentWirelessMode, ReasocSuppRates, FALSE, &SuppRates, &ExtSuppRates );
	PacketMakeElement(&ReAsocReq, EID_SupRates, SuppRates);

	// Extended supported rates
	if(ExtSuppRates.Length != 0)
	{
		PacketMakeElement(&ReAsocReq, EID_ExtSupRates, ExtSuppRates);
	}

	if(!GET_SIMPLE_CONFIG_ENABLED(pMgntInfo))
	{	
		// RSNIE. 2004/07/22, kcwu.
		if(pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch)
		{
				//Fix WPA/WPA2 AP connect fail when wapi is support
				//Modified by ylb, 20110906
				if (pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWAPI || pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeCertificateWAPI )
				{
					WAPI_SecFuncHandler(WAPI_CONSTRUCTASSOCIATEREQ, Adapter, (PVOID)&ReAsocReq, WAPI_END);
				}
				else if(pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch)
				{
					//4 // WPA IE
					if(RT_STATUS_SUCCESS != (rtStatus = 
						Sec_AppendWPAIE(
						Adapter, // Adapter context
						(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
						&(pMgntInfo->targetBSS), // target info
						sizeof(RT_WLAN_BSS), // target info length
						Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
						&ReAsocReq	// Packet Content octet string
						)))
					{
						RT_TRACE_F(COMP_SEC, DBG_TRACE, ("Fail (0x%08X) to append WPA IE\n", rtStatus));
					}

					//4 // RSN IE
					if(RT_STATUS_SUCCESS != (rtStatus = 
						Sec_AppendRSNIE(
							Adapter, // Adapter context
							(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
							&(pMgntInfo->targetBSS), // target info
							sizeof(RT_WLAN_BSS), // target info length
							Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
							&ReAsocReq	// Packet Content octet string
							)))
					{
						RT_TRACE_F(COMP_SEC, DBG_TRACE, ("Fail (0x%08X) to append RSN IE\n", rtStatus));
					}

					//4 // Fast Transition MD IE
					if(RT_STATUS_SUCCESS != (rtStatus = 
						FT_AppendMdIE(
								Adapter, // Adapter context
								(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
								&(pMgntInfo->targetBSS), // target info
								sizeof(RT_WLAN_BSS), // target info length
								Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
								&ReAsocReq	// Packet Content octet string
								)))
					{
						RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Fail (0x%08X) to append MD IE\n", rtStatus));
					}

					//4 // Fast Transition FT IE
					if(RT_STATUS_SUCCESS != (rtStatus = 
						FT_AppendFtIE(
								Adapter, // Adapter context
								(CONTENT_PKT_TYPE_802_11 | CONTENT_PKT_TYPE_CLIENT), // content type
								&(pMgntInfo->targetBSS), // target info
								sizeof(RT_WLAN_BSS), // target info length
								Adapter->MAX_TRANSMIT_BUFFER_SIZE, // Content max length capacity
								&ReAsocReq	// Packet Content octet string
								)))
					{
						RT_TRACE_F(COMP_MLME, DBG_TRACE, ("Fail (0x%08X) to append FT IE\n", rtStatus));
					}
			}
		}
	}

	// WMM-IE. Added by Annie, 2005-11-08.
	if( pMgntInfo->pStaQos->CurrentQosMode>QOS_DISABLE )
	{
		PacketMakeElement(&ReAsocReq, EID_Vendor, pMgntInfo->pStaQos->WMMIE);
	}

	// Append CCX Aironet IE for CCXv1 S19, S13, CCXv4 S61.
	CCX_AppendAironetIE(Adapter, &ReAsocReq);

	// CCX 2 S31 AP Control of Client Transmit Power (TPC).
	CCX_AppendCcxCellPowerIE(Adapter, &ReAsocReq);

	// For CCX 2 S36, Radio Management Capability element, 2006.05.15, by rcnjko.
	CCX_RM_AppendRmCapIE(Adapter, &ReAsocReq);
	
	// For CCX 2 S38, WLAN Device Version Number element.
	CCX_AppendCcxVerIE(Adapter, &ReAsocReq);

	// For CCX S67 MFP
	CCX_AppendCcxSFAIE(Adapter, &ReAsocReq);

	// CCKM Fast Roam , by CCW
	CCX_ConstructReAssociateReq(Adapter, &ReAsocReq);
	

	// Include High Throuput capability && Realtek proprietary
	if( pMgntInfo->pHTInfo->bCurrentHTSupport )
	{		
		//
		// Construct HTCapability and HTInfo IE
		// 
		OCTET_STRING		osHTCap;	
		RT_TRACE(COMP_HT, DBG_LOUD, ("ConstructAssociateReq(): Trying to associate to an 802.11n HT AP\n"));
		FillOctetString(osHTCap, &(pMgntInfo->pHTInfo->SelfHTCap), sizeof(pMgntInfo->pHTInfo->SelfHTCap));
		HTConstructCapabilityElement(Adapter, &osHTCap, TRUE);
		if(pMgntInfo->pHTInfo->ePeerHTSpecVer == HT_SPEC_VER_EWC)
			PacketMakeElement(&ReAsocReq, EID_Vendor, osHTCap);
		else
			PacketMakeElement(&ReAsocReq, EID_HTCapability, osHTCap);
	}

	if( pMgntInfo->pVHTInfo->bCurrentVHTSupport )
	{		
		OCTET_STRING		osVHTCap;	
		u1Byte				opmode = 0;
		OCTET_STRING		osVHTNotif;
		CHANNEL_WIDTH		opNotifBW = CHNL_GetRegBWSupport(Adapter);
		u1Byte				opNotifNss = pMgntInfo->pVHTInfo->nRxSPStream;
		
		RT_TRACE(COMP_HT, DBG_LOUD, ("ConstructAssociateReq(): Trying to associate to an 802.11ac VHT AP\n"));
		FillOctetString(osVHTCap, &(pMgntInfo->pVHTInfo->SelfVHTCap), sizeof(pMgntInfo->pVHTInfo->SelfVHTCap));
		VHTConstructCapabilityElement(Adapter, &osVHTCap, TRUE);
		PacketMakeElement(&ReAsocReq, EID_VHTCapability, osVHTCap);

		if(pMgntInfo->pVHTInfo->bOpModeNotif)
		{
			opNotifBW = pMgntInfo->pVHTInfo->BWToSwitch;
			DbgPrint("pMgntInfo->pVHTInfo->bOpModeNotif = TRYE BW = %d 222\n", opNotifBW);
			opNotifNss = pMgntInfo->pVHTInfo->RxSSToSwitch;
		}
		
		RT_TRACE(COMP_HT, DBG_LOUD, ("ConstructAssociateReq(): carry operating mode notification with BW(%d) RxSS(%d)\n", opNotifBW, opNotifNss));
		FillOctetString(osVHTNotif, &opmode, sizeof(opmode));
		SET_VHT_OPERATING_MODE_FIELD_CHNL_WIDTH(&opmode, opNotifBW);
		SET_VHT_OPERATING_MODE_FIELD_RX_NSS(&opmode, (opNotifNss-1));
		SET_VHT_OPERATING_MODE_FIELD_RX_NSS_TYPE(&opmode, pMgntInfo->pVHTInfo->RxSSTypeBfmeeToSwitch);

		PacketMakeElement(&ReAsocReq, EID_OpModeNotification, osVHTNotif);
	}

	//
	// Construct Realtek Proprietary Aggregation mode (Set AMPDU Factor to 2, 32k)
	//
	//2008.08.13 If connect to realtek ap, append realtek OUI for WOW
	if(pMgntInfo->bRealtekAggCapExist)
	{
		OCTET_STRING		osRealtekIEType2;	
		FillOctetString(osRealtekIEType2, pMgntInfo->RTIEType2Buffer, sizeof(pMgntInfo->RTIEType2Buffer));
		ConstructRTKAggElement(Adapter, &osRealtekIEType2);
		PacketMakeElement(&ReAsocReq, EID_Vendor, osRealtekIEType2);
	}

	
	//
	// Simple config IE. by CCW - copy from 818x
	//
	if(GET_SIMPLE_CONFIG_ENABLED(pMgntInfo))
	{
		WPS_AppendElement(Adapter, &ReAsocReq, FALSE, WPS_INFO_ASOCREQ_IE);
	}

	//
	// CCX4 CAC -- TSPEC IE, 2006.06.26, by shien chang.
	//
	if (CCX_CAC_IsVoiceTsExist(Adapter))
	{
		WMM_TSPEC		tsVoice, tsSignal;
		OCTET_STRING	VoiceEle, SignalEle;
		PQOS_TSTREAM	pTs = NULL;
		PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
		u1Byte			tidVoice = 0;
		RT_DISP( FCCX, CCX_CAC, ( "VoTs exists, append Voice TSPEC Signal TSPEC in Reassoc Req\n" ) ); 
		
		CCX_CAC_ConstructVoiceTspec(Adapter, tsVoice);
		FillOctetString(VoiceEle, &tsVoice[2], TSPEC_SIZE - 2);
		PacketMakeElement( &ReAsocReq, EID_Vendor, VoiceEle );
		
		CCX_CAC_ConstructSignalTspec(Adapter, tsSignal);
		FillOctetString(SignalEle, &tsSignal[2], TSPEC_SIZE - 2);
		PacketMakeElement( &ReAsocReq, EID_Vendor, SignalEle );

		// TSRS IE
		pTs = &(pStaQos->StaTsArray[tidVoice]);
		if (QOS_RATE_TO_BPS( QosGetNPR(Adapter, pTs) ) >=
			GET_TSPEC_MIN_PHY_RATE(pTs->OutStandingTSpec) )
		{
			RT_DISP( FCCX, CCX_CAC, ( "Append TSRS in Reassoc Req\n" ) ); 
			QosConstructTSRS(Adapter, pTs, &ReAsocReq);
		}
	}

#if (P2P_SUPPORT == 1)
	P2P_Append_AssociationReqIe(&ReAsocReq, Adapter);
#endif
    
	WFD_AppendAssocReqIEs(Adapter, Adapter->MAX_TRANSMIT_BUFFER_SIZE, &ReAsocReq);

	WriteEF4Byte(pLength, ReAsocReq.Length);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== ConstructReAssociateReq()\n"));
}

//
//	Description: Construct a Association Response packet.
//	2005.06.02, by rcnjko.
//
VOID
ConstructAssociateRsp(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			asocStaAddr,
	u2Byte			asocCap,
	u2Byte			asocStatusCode,
	u2Byte			asocID,
	BOOLEAN			bReAssocRsp,
	BOOLEAN			bQosSTA
	)
{
	PMGNT_INFO      	pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING	AsocRsp;
	BOOLEAN			bFilterCck = FALSE;
	pu1Byte			pAsocRspPartial;
	OCTET_STRING	SuppRates;
	u1Byte			SuppRatesContent[8]; // NOTE! Length of Support Rates <= 8.
	OCTET_STRING	ExtSuppRates;
	u1Byte			ExtSuppRatesContent[255]; // NOTE! Length of Extended Support Rates <= 255.

	pAsocRspPartial = Buffer;

	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	// Frame Control.
	SET_80211_HDR_FRAME_CONTROL(pAsocRspPartial, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pAsocRspPartial, (bReAssocRsp ? Type_Reasoc_Rsp : Type_Asoc_Rsp));
	SET_80211_HDR_DURATION(pAsocRspPartial, 0);

	// Addresses (DA/SA/BSSID).
	SET_80211_HDR_ADDRESS1(pAsocRspPartial, asocStaAddr);
	SET_80211_HDR_ADDRESS2(pAsocRspPartial, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pAsocRspPartial, pMgntInfo->Bssid);

	// Sequence Control.
	SET_80211_HDR_FRAGMENT_SEQUENCE(pAsocRspPartial, 0);

	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	// Capability information.
	SET_ASOC_RSP_CAPABILITY_INFO(pAsocRspPartial, asocCap);

	// Status Code.
	SET_ASOC_RSP_STATUS_CODE(pAsocRspPartial, asocStatusCode);

	// AID.
	SET_ASOC_RSP_AID(pAsocRspPartial, (asocID | 0xC000) );

	// Size of this Frame until last fixed size field.
	AsocRsp.Length = 30;
	AsocRsp.Octet = Buffer;

	// Supported rates element.
	FillOctetString(SuppRates, SuppRatesContent, 0);
	FillOctetString(ExtSuppRates, ExtSuppRatesContent, 0);

	P2P_FilerCck(Adapter, &bFilterCck);

	SelectSupportedRatesElement( pMgntInfo->dot11CurrentWirelessMode, pMgntInfo->dot11OperationalRateSet, bFilterCck, &SuppRates, &ExtSuppRates );
	PacketMakeElement(&AsocRsp, EID_SupRates, SuppRates);

	// Wireless mode dependent elements.
	if( IS_WIRELESS_OFDM_24G(Adapter))
	{
		// Extended Supported Rates.
		if(ExtSuppRates.Length != 0)
		{
			PacketMakeElement(&AsocRsp, EID_ExtSupRates, ExtSuppRates);
		}
	}

	if(bQosSTA)
	{
		Ap_AppendWmmIe(Adapter, &AsocRsp);
	}

	// Include High Throuput capability and High Throuput info, Guangan add this for 40MHz ap mode.
	if(IS_WIRELESS_MODE_N(Adapter))
	{
		OCTET_STRING		osHTCap, osHTInfo;
		FillOctetString(osHTCap, &(pMgntInfo->pHTInfo->SelfHTCap), sizeof(pMgntInfo->pHTInfo->SelfHTCap));
		FillOctetString(osHTInfo, &(pMgntInfo->pHTInfo->SelfHTInfo), sizeof(pMgntInfo->pHTInfo->SelfHTInfo));
		HTConstructCapabilityElement(Adapter, &osHTCap, TRUE);
		HTConstructInfoElement(Adapter, &osHTInfo);

		// TODO: Construct CAP info element here
		PacketMakeElement(&AsocRsp, EID_HTCapability, osHTCap);
		PacketMakeElement(&AsocRsp, EID_HTInfo, osHTInfo);
	}

	if(IS_WIRELESS_MODE_AC(Adapter))
	{
		OCTET_STRING		osVHTCap, osVHTOperation;
		FillOctetString(osVHTCap, &(pMgntInfo->pVHTInfo->SelfVHTCap), sizeof(pMgntInfo->pVHTInfo->SelfVHTCap));
		FillOctetString(osVHTOperation, &(pMgntInfo->pVHTInfo->SelfVHTInfo), sizeof(pMgntInfo->pVHTInfo->SelfVHTInfo));
		VHTConstructCapabilityElement(Adapter, &osVHTCap, TRUE);
		VHTConstructOperationElement(Adapter, &osVHTOperation);

		// TODO: Construct CAP info element here
		PacketMakeElement(&AsocRsp, EID_VHTCapability, osVHTCap);
		PacketMakeElement(&AsocRsp, EID_VHTOperation, osVHTOperation);
	}

	if(ACTING_AS_AP(Adapter))
	{	
		//
		// Append IE for AsocRsp from OS.
		//
		PRT_WLAN_STA pEntry = AsocEntry_GetEntry(pMgntInfo, asocStaAddr);		

		if(pEntry)
		{
			OCTET_STRING		asocReqIe = {NULL, 0};
			OCTET_STRING		wpsIe = {NULL, 0};
			
			if(pEntry->AP_AsocRspIEFromOSLength > 0&&
				pEntry->AP_AsocRspIEFromOS != NULL &&
				AsocRsp.Length + pEntry->AP_AsocRspIEFromOSLength <= sMaxMpduLng)
			{
				OCTET_STRING osAsocRspIEFromOS;
				osAsocRspIEFromOS.Length = (u2Byte)pEntry->AP_AsocRspIEFromOSLength;
				osAsocRspIEFromOS.Octet = pEntry->AP_AsocRspIEFromOS;
				AppendAdditionalIEs(&AsocRsp, osAsocRspIEFromOS);
			}

			if(pEntry->AP_RecvAsocReqLength) // for WDI this is 0
			{
				FillOctetString(asocReqIe, pEntry->AP_RecvAsocReq, (u2Byte)pEntry->AP_RecvAsocReqLength);
				wpsIe = PacketGetElement(asocReqIe, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUBTYPE_DONT_CARE);
				if(wpsIe.Length > 0)
				{
					WPS_AppendElement(Adapter, &AsocRsp, TRUE, WPS_INFO_ASOCRSP_IE);
				}
			}
		}
	}

#if (P2P_SUPPORT == 1)
	P2P_Append_AssociationRspIe(&AsocRsp, Adapter);
#endif

	WFD_AppendAssocRspIEs(Adapter, Adapter->MAX_TRANSMIT_BUFFER_SIZE, &AsocRsp);

	WriteEF4Byte(pLength, AsocRsp.Length);
}


VOID
ConstructDeauthenticatePacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			auSta,
	u1Byte			asRsn
)
{
	pu1Byte			pdeauth;
	OCTET_STRING	Deauth;
	PMGNT_INFO      	pMgntInfo = &Adapter->MgntInfo;
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);

	pdeauth = Buffer;

	SET_80211_HDR_FRAME_CONTROL(pdeauth, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pdeauth, Type_Deauth);
	SET_80211_HDR_DURATION(pdeauth, 0);

	SET_80211_HDR_ADDRESS1(pdeauth, auSta);
	SET_80211_HDR_ADDRESS2(pdeauth, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pdeauth, Adapter->MgntInfo.Bssid);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pdeauth, 0);
	
	// Size of this Frame until last fixed size field.
	Deauth.Octet	 = Buffer;
	Deauth.Length = sMacHdrLng;

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	CCX_ConstructDeauthenticatePacket(Adapter, pdeauth, &Deauth);
	
	if( pMgntInfo->bInBIPMFPMode )
	{
		if( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP && // Now we just support AES MFP !!
			!(MacAddr_isBcst(auSta) ))
		{
			// Set WEP bit !!
			SET_80211_HDR_WEP( pdeauth  , 1 );  

			if(pSecInfo->EncryptionHeadOverhead > 0)
			{
				PlatformZeroMemory( ( Deauth.Octet+ Deauth.Length ) , pSecInfo->EncryptionHeadOverhead);
				Deauth.Length += pSecInfo->EncryptionHeadOverhead;
			}	
		}
	}
	
	// Add reason code
	WriteEF2Byte( ( Deauth.Octet+ Deauth.Length ) , asRsn );
	Deauth.Length += 2;

	//
	// if need, Append MHDRIE 
	//
	CCX_AppendMHDRIE( Adapter, &Deauth );

	if( pMgntInfo->bInBIPMFPMode && (MacAddr_isBcst(auSta)))
	{
		SecAppenMMIE( Adapter, &Deauth );
	}

	*pLength = Deauth.Length;
}




VOID
ConstructDisassociatePacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			asSta,
	u1Byte			asRsn
)
{
	pu1Byte			pdisassoc;
	OCTET_STRING	Disas;
	PMGNT_INFO      	pMgntInfo = &Adapter->MgntInfo;
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);

	pdisassoc = Buffer;

	SET_80211_HDR_FRAME_CONTROL(pdisassoc, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pdisassoc, Type_Disasoc);
	SET_80211_HDR_DURATION(pdisassoc, 0);

	SET_80211_HDR_ADDRESS1(pdisassoc, asSta);
	SET_80211_HDR_ADDRESS2(pdisassoc, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pdisassoc, Adapter->MgntInfo.Bssid);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pdisassoc, 0);
	
	// Size of this Frame until last fixed size field.
	Disas.Octet	 = Buffer;
	Disas.Length	 = sMacHdrLng;

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	CCX_ConstructDisassociatePacket(Adapter, pdisassoc, &Disas);	

	if( pMgntInfo->bInBIPMFPMode )
	{
		if( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP && // Now we just support AES MFP !!
			!(MacAddr_isBcst(asSta) ))
		{
			// Set WEP bit !!
			SET_80211_HDR_WEP( pdisassoc  , 1 );  

			if(pSecInfo->EncryptionHeadOverhead > 0)
			{
				PlatformZeroMemory( ( Disas.Octet+ Disas.Length ) , pSecInfo->EncryptionHeadOverhead);
				Disas.Length += pSecInfo->EncryptionHeadOverhead;
			}	
		}
		
		
	}

	// Add reason code
	WriteEF2Byte( ( Disas.Octet+ Disas.Length ) , asRsn );
	Disas.Length += 2;

	//
	// if need, Append MHDRIE 
	//
	CCX_AppendMHDRIE( Adapter, &Disas );

	if( pMgntInfo->bInBIPMFPMode && (MacAddr_isBcst(asSta)))
	{
		SecAppenMMIE( Adapter, &Disas );
	}

	*pLength = Disas.Length;
}

//
// Description: 
//	Construct a Probe Response packet.
// Arguments:
//	[in] Adapter -
//		The adapter context.
//	[in] Buffer -
//		The buffer for the constructed probe response.
//	[out] pLength -
//		Return the length in byte for the constructed probe response.
//	[in] StaAddr -
//		The target mac address.
//	[in] bHideSSID -
//		Hide our SSID in the SSID element.
//	[in[ pRfdProbeReq -
//		The RFD contains the probe request. This field may be NULL.
//	[in] posProbeReq -
//		The location of OCTET_STRING for the probe request. This filed may be NULL if no probe request
//		is input.
// Return:
//	None
// Remark:
//	This construction function may check the probe request content and deside if some information should be
//	included in the probe response.
//	The input RFD pRfdProbeReq and posProbeReq may be NULL. In such case, this function only sends the
//	default content of probe response.
// 2004.10.07, by rcnjko.
// Revised by Bruce, 2012-06-08.
//
VOID
ConstructProbeRsp(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			StaAddr,
	IN	BOOLEAN			bHideSSID,
	IN	PRT_RFD			pRfdProbeReq,
	IN	POCTET_STRING	posProbeReq
	)
{
	PMGNT_INFO		pMgntInfo;
	pu1Byte			pProbeRsp;
	OCTET_STRING		ProbeRspFrame;
	u8Byte			TimeStamp;
	OCTET_STRING		DSParms;
	OCTET_STRING		IBSSParms;
	OCTET_STRING		SuppRates;
	u1Byte			SuppRatesContent[8]; // NOTE! Length of Support Rates <= 8.
	OCTET_STRING		ExtSuppRates;
	u1Byte			ExtSuppRatesContent[255]; // NOTE! Length of Extended Support Rates <= 255.
	OCTET_STRING		ERPInfo;
	u1Byte			ERPInfoContent[1];
	
	OCTET_STRING		IbssAdditionalIEs;
	OCTET_STRING		osEDCAInfoElem;
	u1Byte			EDCAInfo[1];
	BOOLEAN			bFilterCck = FALSE;	
	PMGNT_INFO 		pDefaultMgntInfo;
	
	pMgntInfo = &Adapter->MgntInfo;
	pProbeRsp = Buffer;
	
	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	// Frame Control.
	SET_80211_HDR_FRAME_CONTROL(pProbeRsp, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pProbeRsp, Type_Probe_Rsp);
	SET_80211_HDR_DURATION(pProbeRsp, 0);

	// Addresses (DA/SA/BSSID).
	SET_80211_HDR_ADDRESS1(pProbeRsp, StaAddr);
	SET_80211_HDR_ADDRESS2(pProbeRsp, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pProbeRsp, Adapter->MgntInfo.Bssid);

	// Sequence Control.
	SET_80211_HDR_FRAGMENT_SEQUENCE(pProbeRsp, 0);
	

	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	// Timestamp.
	// <RJ_TODO> I think this is not necessary, becuase our hw should fill timestamp.
	TimeStamp = PlatformGetCurrentTime();
	SET_BEACON_PROBE_RSP_TIME_STAMP_LOW(pProbeRsp, (u4Byte)(TimeStamp&0xffffffff));
	SET_BEACON_PROBE_RSP_TIME_STAMP_HIGH(pProbeRsp, (u4Byte)(TimeStamp >> 32));

	// Beacon interval.
	SET_BEACON_PROBE_RSP_BEACON_INTERVAL(pProbeRsp, pMgntInfo->dot11BeaconPeriod );

	// Capability information.
	if( ACTING_AS_AP(Adapter) )
	{
		SET_BEACON_PROBE_RSP_CAPABILITY_INFO(pProbeRsp, (pMgntInfo->mCap & ~cIBSS) ); 
	}
	else
	{
		SET_BEACON_PROBE_RSP_CAPABILITY_INFO(pProbeRsp, (pMgntInfo->mCap & ~cESS) ); 
	}

	if( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm!= RT_ENC_ALG_NO_CIPHER )
	{
		UNION_BEACON_PROBE_RSP_CAPABILITY_INFO(pProbeRsp, cPrivacy);
	}
	else
	{
		MASK_BEACON_PROBE_RSP_CAPABILITY_INFO(pProbeRsp, cPrivacy);
	}

	// Size of this Frame until last fixed size field.
	ProbeRspFrame.Length = 36;
	ProbeRspFrame.Octet = Buffer;

	// SSID.
	if( bHideSSID && pMgntInfo->bHiddenSSID )
	{
		u1Byte			NullSSID[1];
		OCTET_STRING	HiddenSSID;
		FillOctetString(HiddenSSID, NullSSID, 0);
		PacketMakeElement(&ProbeRspFrame, EID_SsId, HiddenSSID);
	}
	else
	{
		PacketMakeElement(&ProbeRspFrame, EID_SsId, Adapter->MgntInfo.Ssid);
	}

	// Supported rate.
	FillOctetString(SuppRates, SuppRatesContent, 0);
	FillOctetString(ExtSuppRates, ExtSuppRatesContent, 0);

	P2P_FilerCck(Adapter, &bFilterCck);

	// Set supported and extended supported rate according to Wirelessmode and dot11OperationalRateSet. 
	SelectSupportedRatesElement( pMgntInfo->dot11CurrentWirelessMode, pMgntInfo->dot11OperationalRateSet, bFilterCck, &SuppRates, &ExtSuppRates );
	PacketMakeElement(&ProbeRspFrame, EID_SupRates, SuppRates);
	
	// DS parameters element.
	DSParms.Octet = &pMgntInfo->dot11CurrentChannelNumber;
	DSParms.Length = 1;
	PacketMakeElement(&ProbeRspFrame, EID_DSParms, DSParms);

	RT_TRACE(COMP_AP, DBG_TRACE, ("Constructure Probe RSP Mediaconnect %d  dot11CurrentChannel %d, dot11CurrentWirelessMode=%x\n",
		pMgntInfo->bMediaConnect,pMgntInfo->dot11CurrentChannelNumber,pMgntInfo->dot11CurrentWirelessMode));

	// Ibss or TIM element.
	if( !ACTING_AS_AP(Adapter)){
		IBSSParms.Octet = (pu1Byte)(&pMgntInfo->mIbssParms.atimWin);
		IBSSParms.Length = 2;
		PacketMakeElement(&ProbeRspFrame, EID_IbssParms, IBSSParms);
	}

	//2004/07/22, kcwu, construct Security IE
	//2004/09/15, kcwu, decide to construct which kind of element,WPA or WPA2
	if(pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch)
	{
		PacketMakeElement(&ProbeRspFrame, (pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA)?EID_Vendor:EID_WPA2, pMgntInfo->SecurityInfo.RSNIE);
	}

	// Wireless mode dependent elements.
	if( IS_WIRELESS_OFDM_24G(Adapter))
	{
		// Capability.
		if(ACTING_AS_IBSS(Adapter) == FALSE)
			UNION_BEACON_PROBE_RSP_CAPABILITY_INFO(pProbeRsp, cShortSlotTime);

		if( IS_WIRELESS_MODE_G(Adapter) ||
			(IS_WIRELESS_MODE_HT_24G(Adapter) && pMgntInfo->pHTInfo->bCurSuppCCK))
		{
			ERPInfoContent[0] = 0;
			if(pMgntInfo->bUseProtection)
				ERPInfoContent[0] |= ERP_UseProtection;

			FillOctetString(ERPInfo, ERPInfoContent, 1);
			PacketMakeElement(&ProbeRspFrame, EID_ERPInfo, ERPInfo);
		}

		// Extended Supported Rates.
		if(ExtSuppRates.Length != 0)
		{
			PacketMakeElement(&ProbeRspFrame, EID_ExtSupRates, ExtSuppRates);
		}
	}

	//
	// Append Additional IEs from Ndis6, for DTM Vista 1.0c premium logo test. By Bruce, 2007-08-13.
	//
	if(ACTING_AS_IBSS(Adapter))
	{
		GetIbssAdditionalData(Adapter, &IbssAdditionalIEs);
		if(IbssAdditionalIEs.Octet != NULL && IbssAdditionalIEs.Length != 0)
		{
			AppendAdditionalIEs(&ProbeRspFrame, IbssAdditionalIEs);
		}

		if(pMgntInfo->pStaQos->QosCapability != QOS_DISABLE)	
		{
			FillOctetString(osEDCAInfoElem, EDCAInfo, 1);
			PlatformZeroMemory(osEDCAInfoElem.Octet, 1); 
			AppendQoSElement(&ProbeRspFrame, &osEDCAInfoElem, OUI_SUBTYPE_WMM_INFO);
		}
	}

	//
	// Append additional IEs. By haich, 2008.08.01.
	//
	if(ACTING_AS_AP(Adapter))
	{
		if(pMgntInfo->AdditionalResponseIESize> 0&&
			pMgntInfo->AdditionalResponseIEData!= NULL &&
			ProbeRspFrame.Length + pMgntInfo->AdditionalResponseIESize <= sMaxMpduLng)
		{
			OCTET_STRING osAdditionalProbeRspIEs;
			osAdditionalProbeRspIEs.Length = (u2Byte)pMgntInfo->AdditionalResponseIESize;
			osAdditionalProbeRspIEs.Octet = (pu1Byte)pMgntInfo->AdditionalResponseIEData;
			AppendAdditionalIEs(&ProbeRspFrame, osAdditionalProbeRspIEs);
		}
		Ap_AppendWmmIe(Adapter, &ProbeRspFrame);
		Ap_AppendCountryIE(Adapter, &ProbeRspFrame);
		Ap_AppendPowerConstraintIE(Adapter, &ProbeRspFrame);
	}

	//
	// Include High Throuput capability and High Throuput info
	//	- Do not include HT if CurrentWirelessMode is less than N mode even if hardware support it
	//
	if(IS_WIRELESS_MODE_N(Adapter))
	{
		OCTET_STRING		osHTCap, osHTInfo;
		
		FillOctetString(osHTCap, &(pMgntInfo->pHTInfo->SelfHTCap), sizeof(pMgntInfo->pHTInfo->SelfHTCap));
		FillOctetString(osHTInfo, &(pMgntInfo->pHTInfo->SelfHTInfo), sizeof(pMgntInfo->pHTInfo->SelfHTInfo));
		HTConstructCapabilityElement(Adapter, &osHTCap, FALSE);
		HTConstructInfoElement(Adapter, &osHTInfo);

		// TODO: Construct CAP info element here
		PacketMakeElement(&ProbeRspFrame, EID_HTCapability, osHTCap);
		PacketMakeElement(&ProbeRspFrame, EID_HTInfo, osHTInfo);
	}

	//
	// Construct Realtek Proprietary Aggregation mode (Set AMPDU Factor to 2, 32k)
	//
	{
		OCTET_STRING		osRealtekIEType2;
		FillOctetString(osRealtekIEType2, pMgntInfo->RTIEType2Buffer, sizeof(pMgntInfo->RTIEType2Buffer));
		ConstructRTKAggElement(Adapter, &osRealtekIEType2);
		PacketMakeElement(&ProbeRspFrame, EID_Vendor, osRealtekIEType2);
	}

	if(IS_WIRELESS_MODE_AC(Adapter))
	{
		OCTET_STRING		osVHTCap, osVHTOperation;

		FillOctetString(osVHTCap, &(pMgntInfo->pVHTInfo->SelfVHTCap), sizeof(pMgntInfo->pVHTInfo->SelfVHTCap));
		FillOctetString(osVHTOperation, &(pMgntInfo->pVHTInfo->SelfVHTInfo), sizeof(pMgntInfo->pVHTInfo->SelfVHTInfo));
		VHTConstructCapabilityElement(Adapter, &osVHTCap, FALSE);
		VHTConstructOperationElement(Adapter, &osVHTOperation);

		// TODO: Construct CAP & Operation element here
		PacketMakeElement(&ProbeRspFrame, EID_VHTCapability, osVHTCap);
		PacketMakeElement(&ProbeRspFrame, EID_VHTOperation, osVHTOperation);
	}
	
	WAPI_SecFuncHandler(WAPI_CONSTRUCTPROBERSP, Adapter, (PVOID)&ProbeRspFrame, WAPI_END);
	
	//
	// Simple config IE. by CCW - copy from 818x
	//
	pDefaultMgntInfo = &(GetDefaultAdapter(Adapter)->MgntInfo);
	if(GET_SIMPLE_CONFIG_IE_LEN(pDefaultMgntInfo) > 0)
	{
		RT_TRACE(COMP_WPS, DBG_TRACE, ("AP Construct Probe Response \n"));
		WPS_AppendElement(GetDefaultAdapter(Adapter), &ProbeRspFrame, FALSE, WPS_INFO_PROBERSP_IE);
	}

#if (P2P_SUPPORT == 1)
	P2P_Append_GoProbeRspIe(&ProbeRspFrame, Adapter, Type_Probe_Rsp, pRfdProbeReq, posProbeReq);
#endif
    
	WFD_AppendProbeRspIEs(Adapter, Adapter->MAX_TRANSMIT_BUFFER_SIZE, &ProbeRspFrame, pRfdProbeReq, posProbeReq);

	*pLength = ProbeRspFrame.Length;
}

//
//	Description: Construct a customized Data frame.
//	2004.12.05, by rcnjko.
//
VOID
ConstructCustomizedData(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	u2Byte			pktLength
)
{
	PMGNT_INFO     		pMgntInfo;
	pu1Byte			pHeader;
	
	pMgntInfo = &Adapter->MgntInfo;
	pHeader = Buffer;
	
	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
	SET_80211_HDR_PWR_MGNT(pHeader, 0);
	
	SET_80211_HDR_TO_DS(pHeader, 1);
	SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid);
	SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid);
	
	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);

	*pLength = 24 + (u4Byte)pktLength;
}


//
//	Description: Construct a customized Data frame.
//	2005.03.23, by rcnjko.
//
VOID
ConstructNullFunctionData(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			StaAddr,
	IN	BOOLEAN			bQoS,
	IN	u1Byte			AC,
	IN	BOOLEAN			bEosp,
	IN	BOOLEAN			bForcePowerSave
	)
{
	PMGNT_INFO     		pMgntInfo;
	pu1Byte			pHeader;
	
	pMgntInfo = &Adapter->MgntInfo;
	pHeader = Buffer;
	
	// [APSD] QoS Null Function Data , 2006-06-22, by Isaiah 
	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, ( (bQoS) ? Type_QosNull: Type_Null_Frame) );
	SET_80211_HDR_PWR_MGNT(pHeader, (bForcePowerSave) ? 1 : 0 );
	
	switch(pMgntInfo->OpMode)
	{
	case RT_OP_MODE_INFRASTRUCTURE:	// 1,0
		SET_80211_HDR_TO_DS(pHeader, 1);
		SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid);
		SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
		SET_80211_HDR_ADDRESS3(pHeader, StaAddr);
		break;
	case RT_OP_MODE_AP:				// 0,1
		SET_80211_HDR_FROM_DS(pHeader, 1);
		SET_80211_HDR_ADDRESS1(pHeader, StaAddr);
		SET_80211_HDR_ADDRESS2(pHeader, pMgntInfo->Bssid);
		SET_80211_HDR_ADDRESS3(pHeader, Adapter->CurrentAddress);
		break;
	case RT_OP_MODE_IBSS:			// 0,0
	default:
		// Handle it as IBSS, 2004.08.21, by rcnjko.
		SET_80211_HDR_ADDRESS1(pHeader, StaAddr );
		SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
		SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid );
		break;
	}
	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);

	if(bQoS) //[APSD] Isaiah for QoSNullFunc  2006-07-31
	{
		SET_QOS_CTRL(pHeader, 0);
		SET_QOS_CTRL_WMM_UP(pHeader, AC);
		SET_QOS_CTRL_WMM_EOSP(pHeader, bEosp ? 1 : 0);
		*pLength = 26;
	}
	else
		*pLength = 24;
}

VOID
ConstructBtNullFunctionData(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			StaAddr,
	IN	BOOLEAN			bQoS,
	IN	u1Byte			AC,
	IN	BOOLEAN			bEosp,
	IN	BOOLEAN			bForcePowerSave
	)
{
	PMGNT_INFO     		pMgntInfo;
	pu1Byte			pHeader;
	
	pMgntInfo = &Adapter->MgntInfo;
	pHeader = Buffer;
	
	// [APSD] QoS Null Function Data , 2006-06-22, by Isaiah 
	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, ( (bQoS) ? Type_QosNull: Type_Null_Frame) );
	SET_80211_HDR_PWR_MGNT(pHeader, (bForcePowerSave) ? 1 : 0 );

	SET_80211_HDR_FROM_DS(pHeader, 1);
	SET_80211_HDR_ADDRESS1(pHeader, StaAddr);
	SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pHeader, Adapter->CurrentAddress);
#if 0
	switch(pMgntInfo->OpMode)
	{
	case RT_OP_MODE_INFRASTRUCTURE:	// 1,0
		SET_80211_HDR_TO_DS(pHeader, 1);
		SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid);
		SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
		SET_80211_HDR_ADDRESS3(pHeader, StaAddr);
		break;
	case RT_OP_MODE_AP:				// 0,1
		SET_80211_HDR_FROM_DS(pHeader, 1);
		SET_80211_HDR_ADDRESS1(pHeader, StaAddr);
		SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
		SET_80211_HDR_ADDRESS3(pHeader, Adapter->CurrentAddress);
		break;
	case RT_OP_MODE_IBSS:			// 0,0
	default:
		// Handle it as IBSS, 2004.08.21, by rcnjko.
		SET_80211_HDR_ADDRESS1(pHeader, StaAddr );
		SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
		SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid );
		break;
	}
#endif
	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);

	if(bQoS) //[APSD] Isaiah for QoSNullFunc  2006-07-31
	{
		SET_QOS_CTRL(pHeader, 0);
		SET_QOS_CTRL_WMM_UP(pHeader, AC);
		SET_QOS_CTRL_WMM_EOSP(pHeader, bEosp ? 1 : 0);
		*pLength = 26;
	}
	else
		*pLength = 24;
}


//
//	Description: Construct a PS-Poll frame.
//	2005.02.15, by rcnjko.
//
VOID
ConstructPSPoll(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength
)
{
	pu1Byte			pPSPoll;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);

	pPSPoll = Buffer;

	// Frame control.
	SET_80211_HDR_FRAME_CONTROL(pPSPoll, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pPSPoll, Type_PS_poll);
	SET_80211_HDR_PWR_MGNT(pPSPoll, 1);

	// AID.
	SET_80211_PS_POLL_AID(pPSPoll, (pMgntInfo->mAId | 0xc000) );


	// BSSID.
	SET_80211_PS_POLL_BSSID(pPSPoll, pMgntInfo->Bssid );

	// TA.
	SET_80211_PS_POLL_TA(pPSPoll, Adapter->CurrentAddress);

	*pLength = 16;
}

//
//	Description: Construct a magic packet.
//	2005.10.20, by rcnjko.
//
VOID
ConstructMagicPacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			StaAddr
)
{
	PMGNT_INFO     		pMgntInfo;
	pu1Byte			pHeader;
	int							i;
	
	pMgntInfo = &Adapter->MgntInfo;
	pHeader = Buffer;
	
	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
	
	switch(pMgntInfo->OpMode)
	{
	case RT_OP_MODE_INFRASTRUCTURE:	// 1,0
		SET_80211_HDR_TO_DS(pHeader, 1);
		SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid);

		// Old:
		//PlatformMoveMemory( pHeader->Addr2, Adapter->CurrentAddress, 6 ); // SA
		// New: Suggested by WCChu for 8187 wake up by unicast probereq: 2005.10.20, by rcnjko:
		SET_80211_HDR_ADDRESS2(pHeader, StaAddr); // SA

		SET_80211_HDR_ADDRESS3(pHeader, StaAddr);// DA
		break;

	case RT_OP_MODE_AP:				// 0,1
		SET_80211_HDR_FROM_DS(pHeader, 1);
		SET_80211_HDR_ADDRESS1(pHeader, StaAddr);	

		// Old:
		//PlatformMoveMemory( pHeader->Addr2, pMgntInfo->Bssid, 6 ); // BSSID
		// New: Suggested by WCChu for 8187 wake up by unicast probereq: 2005.10.20, by rcnjko:
		SET_80211_HDR_ADDRESS2(pHeader, StaAddr);// BSSID

		SET_80211_HDR_ADDRESS3(pHeader, Adapter->CurrentAddress); // SA
		break;

	case RT_OP_MODE_IBSS:			// 0,0
	default:
		// Handle it as IBSS, 2004.08.21, by rcnjko.
		SET_80211_HDR_ADDRESS1(pHeader, StaAddr); // DA

		// Old:
		//PlatformMoveMemory( pHeader->Addr2, Adapter->CurrentAddress, 6 ); // SA
		// New: Suggested by WCChu for 8187 wake up by unicast probereq: 2005.10.20, by rcnjko:
		SET_80211_HDR_ADDRESS2(pHeader, StaAddr); // SA

		SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid);  // BSSID
		break;
	}
	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);

	// Set STA addr as Addr1.
	SET_80211_HDR_ADDRESS1(pHeader, StaAddr);
	// FF FF FF FF FF FF.
	for(i = 0; i < 6; i++)
	{
		Buffer[sMacHdrLng+i] = 0xFF;
	}
	// Repeat STA addr 16 times.
	for(i = 0; i < 16; i++)
	{
		PlatformMoveMemory( Buffer+sMacHdrLng+6+i*6, StaAddr, 6 ); // BSSID.
	}
	*pLength = sMacHdrLng + 6 + 6*16;
}

//
//	Description: Construct an EAPOL-Key packet.
//	2005.07.01, by rcnjko.
//
VOID
ConstructEapolKeyPacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			StaAddr,

	pu1Byte			pKCK, // Pointer to KCK (EAPOL-Key Confirmation Key).
	pu1Byte			pKEK, // Pointer to KEK (EAPOL-Key Encryption Key).

	KeyType			eKeyType, // EAPOL-Key Information field: Key Type bit: type_Group or type_Pairwise.
	BOOLEAN			bInstall, // EAPOL-Key Information field: Install Flag.
	BOOLEAN			bKeyAck, // EAPOL-Key Information field: Key Ack bit.
	BOOLEAN			bKeyMIC, // EAPOL-Key Information field: Key MIC bit. If true, we will calculate EAPOL MIC and fill it into Key MIC field. 
	BOOLEAN			bSecure, // EAPOL-Key Information field: Secure bit.
	BOOLEAN			bError, // EAPOL-Key Information field: Error bit. True for MIC failure report.
	BOOLEAN			bRequest, // EAPOL-Key Information field: Requst bit.
	
	u8Byte			u8bKeyReplayCounter, // EAPOL-KEY Replay Counter field.
	pu1Byte			pKeyNonce, // EAPOL-Key Key Nonce field (32-byte).
	u8Byte			u8bKeyRSC, // EAPOL-Key Key RSC field (8-byte).

	POCTET_STRING		posRSNIE, // Key Data field: Pointer to RSN IE.
	pu1Byte			pGTK, // Key Data field: Pointer to GTK.
	BOOLEAN			bEncrypt		// encrypt the packet or not. added by Annie, 2005-07-11.
)
{
	PMGNT_INFO     		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	pu1Byte			pHeader = Buffer;
	static u1Byte 		Snap802_1H[6] = { 0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00 };

	PEAPOL_STRUCT		pEapol = NULL;
	PEAPOL_KEY_STRUCT	pEapolKey = NULL;
	OCTET_STRING		osEapolKey;
	KeyDescVer		eKeyDescVer;
	u2Byte			u2bEapolKeyMsgSz = 0;
	u2Byte			u2bEapolKeyDataLen = 0;
	u1Byte			tmpBuf[256];
	OCTET_STRING		osKeyNonce;	
	OCTET_STRING		osEapolKeyIV;
	OCTET_STRING		osKeyRSC;
	OCTET_STRING		osKeyID;

	// Figure Key Data Length and size of EAPOL-Key Msg.
	if(posRSNIE != NULL)
	{
		u2bEapolKeyDataLen += (2+posRSNIE->Length);
	}
	if(pGTK != NULL)
	{
		//AP-WPA AES test,CCW
		if( pSecInfo->SecLvl==RT_SEC_LVL_WPA )
		{
			if(pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP)
				u2bEapolKeyDataLen += GTK_LEN_CCMP+8;
			else
				u2bEapolKeyDataLen += GTK_LEN;
	}
		else if( pSecInfo->SecLvl==RT_SEC_LVL_WPA2 )
		{
			if(pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP)
				u2bEapolKeyDataLen += 24+8+2; // 24 is KED lengh ,8 is AES-WRAP add ,2 is for RSNIE to 8's muitiple ;
			else
				u2bEapolKeyDataLen += 40+8+2;	// 40 is KED lengh,8 is AES-WRAP add ,2 is for RSNIE to 8's muitiple
		}
	}
	u2bEapolKeyMsgSz = EAPOLMSG_HDRLEN + u2bEapolKeyDataLen;

	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
	
	switch(pMgntInfo->OpMode)
	{
	case RT_OP_MODE_INFRASTRUCTURE:	// 1,0
		SET_80211_HDR_TO_DS(pHeader, 1);
		SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid);  // BSSID
		SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress); // SA
		SET_80211_HDR_ADDRESS3(pHeader, StaAddr); // DA
		break;
	case RT_OP_MODE_AP:				// 0,1
		SET_80211_HDR_FROM_DS(pHeader, 1);
		SET_80211_HDR_ADDRESS1(pHeader, StaAddr); // DA
		SET_80211_HDR_ADDRESS2(pHeader, pMgntInfo->Bssid); // BSSID
		SET_80211_HDR_ADDRESS3(pHeader, Adapter->CurrentAddress);  // SA
		break;
	case RT_OP_MODE_IBSS:			// 0,0
	default:
		// Handle it as IBSS, 2004.08.21, by rcnjko.
		SET_80211_HDR_ADDRESS1(pHeader, StaAddr); // DA
		SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);  // SA
		SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid); // BSSID
		break;
	}

	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);
	*pLength = sMacHdrLng;

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
//	if(eKeyType == type_Group)
	if( bEncrypt )
	{ // EAPOL-Key of Group key handshake will be encrypted via PTK, 2005.07.01, by rcnjko.
		PlatformFillMemory( &(Buffer[*pLength]), pSecInfo->EncryptionHeadOverhead, 0x00);
		*pLength += pSecInfo->EncryptionHeadOverhead;
	}	

	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	//
	// 1. LLC+TypeLen
	//
	PlatformMoveMemory( &(Buffer[*pLength]), Snap802_1H, LLC_HEADER_SIZE);
	*pLength += LLC_HEADER_SIZE;
	Buffer[*pLength] = 0x88;
	Buffer[*pLength+1] = 0x8e;
	*pLength += 2;

	//
	// 2. EAPOL Header
	//
	pEapol = (PEAPOL_STRUCT)( &(Buffer[*pLength]) );
	pEapol->protocol_version = LIB1X_EAPOL_VER;
	pEapol->packet_type = LIB1X_EAPOL_KEY;
	short2net(u2bEapolKeyMsgSz, ((pu1Byte)&(pEapol->packet_body_length)) );
	*pLength += LIB1X_EAPOL_HDRLEN;
	
	//
	// 3. EAPOL-Key descriptor.
	//
	pEapolKey = (PEAPOL_KEY_STRUCT)( &(Buffer[*pLength]) );
	FillOctetString(osEapolKey, pEapolKey, EAPOLMSG_HDRLEN);

	// 3.1 Descryptor Type.
	Message_setDescType(osEapolKey, ((pSecInfo->SecLvl==RT_SEC_LVL_WPA) ? desc_type_RSN : desc_type_WPA2));

	// 3.2 Key Information.
	PlatformFillMemory(pEapolKey->key_info, 2, 0x00);
	//AP-WPA AES test,CCW
	eKeyDescVer = ((pSecInfo->PairwiseEncAlgorithm==RT_ENC_ALG_TKIP ) ? key_desc_ver1 : key_desc_ver2);
	Message_setKeyDescVer(osEapolKey, eKeyDescVer);
	Message_setKeyType(osEapolKey, ((eKeyType == type_Group) ? 0 : 1) );
	Message_setKeyIndex(osEapolKey, ((eKeyType == type_Group) ? 1 : 0) );
	Message_setInstall(osEapolKey, bInstall);
	Message_setKeyAck(osEapolKey, bKeyAck);
	Message_setKeyMIC(osEapolKey, bKeyMIC);
	Message_setSecure(osEapolKey, bSecure);
	Message_setError(osEapolKey, bError);
	Message_setRequest(osEapolKey, bRequest);
	Message_setEncryptedKeyData(osEapolKey, ((pSecInfo->SecLvl == RT_SEC_LVL_WPA2 && pGTK != NULL) ? TRUE : FALSE));		// It falls to a reserved bit. Marked by Annie, 2005-07-10.

	// 3.3 Key Length.
	//AP-WPA AES test,CCW
	Message_setKeyLength(osEapolKey, ((pSecInfo->PairwiseEncAlgorithm==RT_ENC_ALG_TKIP) ? 32 : 16));

	// 3.4 Key Replay Counter.
	PlatformMoveMemory( osEapolKey.Octet + ReplayCounterPos, &u8bKeyReplayCounter, 8);

	// 3.5 Key Nonce.
	FillOctetString(osKeyNonce, pKeyNonce, KEY_NONCE_LEN);
	Message_setKeyNonce(osEapolKey, osKeyNonce);

	// 3.6 EAPOL-Key IV.
	PlatformFillMemory(tmpBuf, KEY_IV_LEN, 0);
	FillOctetString(osEapolKeyIV, tmpBuf, KEY_IV_LEN);
	Message_setKeyIV(osEapolKey, osEapolKeyIV);
	
	// 3.7 Key RSC.
	FillOctetString(osKeyRSC, &(u8bKeyRSC), sizeof(u8bKeyRSC));
	Message_setKeyRSC(osEapolKey, osKeyRSC);

	// 3.8 Key ID?
	PlatformFillMemory(tmpBuf, KEY_ID_LEN, 0);
	FillOctetString(osKeyID, tmpBuf, KEY_ID_LEN);
	Message_setKeyID(osEapolKey, osKeyID);

	// 3.9 Key MIC. We will fill it up later if required.
	Message_clearMIC(osEapolKey);
	
	// 3.10 Key Data Length.
	Message_setKeyDataLength(osEapolKey, u2bEapolKeyDataLen);
	
	// 3.11 Key Data.
	// IE.
	if(posRSNIE != NULL && posRSNIE->Length > 0)
	{
		PacketMakeElement( &osEapolKey, 
			(pSecInfo->SecLvl == RT_SEC_LVL_WPA) ? EID_Vendor : EID_WPA2, (*posRSNIE) );
	}
	// KDE.
	if(pGTK != NULL && pSecInfo->SecLvl == RT_SEC_LVL_WPA)
	{
		RT_ASSERT((pKEK!=NULL), ("ConstructEapolKeyPacket(): pGTK!=NULL pKCK=%p!!!", pKEK));
		//AP-WPA AES test,CCW
		//PlatformMoveMemory( &(osEapolKey.Octet[osEapolKey.Length]), pGTK, GTK_LEN);
		//osEapolKey.Length += GTK_LEN;
		if( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP )
		{
			PlatformMoveMemory( &(osEapolKey.Octet[osEapolKey.Length]), pGTK, GTK_LEN_CCMP);
			osEapolKey.Length += GTK_LEN_CCMP + 8; //When after AES-WRAP keydata length add 8;
		}
		else
		{
			PlatformMoveMemory( &(osEapolKey.Octet[osEapolKey.Length]), pGTK, GTK_LEN);
			osEapolKey.Length += GTK_LEN;
		}
		if(pKEK)
		{
			// Encode Key Data field if necessary.
			//RT_TRACE(COMP_WPAAES, DBG_LOUD, ("=========> Encrypt EAPOL ^^"));
			EncEapolKeyData(Adapter, osEapolKey, pKEK, PTK_LEN_EAPOLENC);
		}
	}
	else if( pGTK != NULL && pSecInfo->SecLvl == RT_SEC_LVL_WPA2 )
	{
		PKDE_STRUCT pKDE = NULL;
		u1Byte		 KEDOUI[] = {0x00,0x0F,0xAC};   //00-0F-AC
		pKDE = (PKDE_STRUCT)( &(osEapolKey.Octet[osEapolKey.Length]));
		//   KeyID  = bit0~bit2 
		//   TX       = bit3
		//   resved = bit4~bit7 and byte2
		//   GTK data length = ( Length -6 )
		
		pKDE->IEType= 0xdd;  //IE ID = 0xDD
		
		if( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP )
		{
			pKDE->IELen = 0x16;      // 18+4 = 22
			osEapolKey.Length += 24+8+2;
		}
		else if( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP ) 
		{
			pKDE->IELen = 0x26;     // 34+4 = 38
			osEapolKey.Length += 40+8+2;
		}

		PlatformMoveMemory(pKDE->OUI, KEDOUI, 3);

		pKDE->Datatype = 0x01;
		
		pKDE->KID = 0x01; //Set Group Key ID = 1;

		pKDE->Reserved = 0x00; // Set Reserved to all 0;

		PlatformZeroMemory(pKDE->GTK , 64 );

		if( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP )
			PlatformMoveMemory(pKDE->GTK , pGTK , 16);
		else if( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP )
			PlatformMoveMemory(pKDE->GTK , pGTK , 32);
		
		if(pKEK)
		{
			// Encode Key Data field if necessary.
			EncEapolKeyData(Adapter, osEapolKey, pKEK, PTK_LEN_EAPOLENC);
		}
	}	
	*pLength += u2bEapolKeyMsgSz;

	// Figure out EAPOL-Key MIC if required.
	if(bKeyMIC)
	{
		OCTET_STRING	osEapolMsgToSend;

		RT_ASSERT((pKCK!=NULL), ("ConstructEapolKeyPacket(): bKeyMIC pKCK=%p!!!", pKCK));
		if(pKCK != NULL)
		{
			FillOctetString(osEapolMsgToSend, pEapol, (LIB1X_EAPOL_HDRLEN+u2bEapolKeyMsgSz) );
			CalcEapolMIC(osEapolMsgToSend, eKeyDescVer, pKCK, KEY_MIC_LEN);
		}
	}
}


//
//   Description: Construct EAPOL-Star Packet Let AP try to re-key !!
//   
VOID
ConstructEAPOLStartPacket(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN   BOOLEAN				bEncrypt
	)
{
	PMGNT_INFO     		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	pu1Byte				pHeader = Buffer;
	static u1Byte 			Snap802_1H[6] = { 0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00 };
	PEAPOL_STRUCT		pEapol = NULL;
	u2Byte				u2bEapol = 0;


	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);

	switch(pMgntInfo->OpMode)
	{
		case RT_OP_MODE_INFRASTRUCTURE:	// 1,0
			SET_80211_HDR_TO_DS(pHeader, 1);
			SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid);  // BSSID
			SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress); // SA
			SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid); // DA
			break;
		case RT_OP_MODE_AP:				// 0,1
			SET_80211_HDR_FROM_DS(pHeader, 1);
			SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid); // DA
			SET_80211_HDR_ADDRESS2(pHeader, pMgntInfo->Bssid); // BSSID
			SET_80211_HDR_ADDRESS3(pHeader, Adapter->CurrentAddress);  // SA
			break;
		case RT_OP_MODE_IBSS:			// 0,0
		default:
			// Handle it as IBSS, 2004.08.21, by rcnjko.
			SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid); // DA
			SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);  // SA
			SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid); // BSSID
			break;
	}

	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);
	*pLength = sMacHdrLng;

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if( bEncrypt )
	{ // EAPOL-Key of Group key handshake will be encrypted via PTK, 2005.07.01, by rcnjko.
		PlatformFillMemory( &(Buffer[*pLength]), pSecInfo->EncryptionHeadOverhead, 0x00);
		*pLength += pSecInfo->EncryptionHeadOverhead;
	}

	
	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	//
	// 1. LLC+TypeLen
	//
	PlatformMoveMemory( &(Buffer[*pLength]), Snap802_1H, LLC_HEADER_SIZE);
	*pLength += LLC_HEADER_SIZE;
	Buffer[*pLength] = 0x88;
	Buffer[*pLength+1] = 0x8e;
	*pLength += 2;

	//
	// 2. EAPOL Header
	//
	pEapol = (PEAPOL_STRUCT)( &(Buffer[*pLength]) );
	pEapol->protocol_version = LIB1X_EAPOL_VER;
	pEapol->packet_type = LIB1X_EAPOL_START;
	short2net(u2bEapol, ((pu1Byte)&(pEapol->packet_body_length)) );
	*pLength += LIB1X_EAPOL_HDRLEN;

}


//
//	Description: Construct CCX Radio Measure Report packet.
//	2006.05.07, by rcnjko.
//
VOID
ConstructCcxRmReport(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN	PRT_RM_REPORTS		pRmReports
	)
{
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS					pStaQos = pMgntInfo->pStaQos;
	PRT_SECURITY_T				pSecInfo = &(pMgntInfo->SecurityInfo);
	pu1Byte 					pHeader = Buffer;
	PRM_REPORT_PACKET			pRmRptPkt = NULL;
	POCTET_STRING				posRpts = &(pRmReports->osMeasRptEles);
	static u1Byte				CiscoAironetSnap[CISCO_AIRONET_SNAP_LENGTH] = { 0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00};
	u2Byte						IappIdLen;

	RT_TRACE(COMP_SEND, DBG_LOUD, ("+ ConstructCcxRmReport()\n"));

	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	RT_ASSERT(pMgntInfo->OpMode==RT_OP_MODE_INFRASTRUCTURE, 
		("ConstructCcxRmReport(): OpMode(%d) != RT_OP_MODE_INFRASTRUCTURE(%d)\n", pMgntInfo->OpMode, RT_OP_MODE_INFRASTRUCTURE));

	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
	SET_80211_HDR_TO_DS(pHeader, 1);
	SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid);
	SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid);

	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);
	*pLength = sMacHdrLng;

	//-------------------------------------------------------------------------
	// Qos Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pStaQos->CurrentQosMode > QOS_DISABLE)
	{
		SET_80211_HDR_QOS_EN(pHeader, 1);
		PlatformZeroMemory(&(Buffer[*pLength]), sQoSCtlLng);
		*pLength += sQoSCtlLng;
	}

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pSecInfo->EncryptionHeadOverhead > 0)
	{
		PlatformZeroMemory(&(Buffer[*pLength]), pSecInfo->EncryptionHeadOverhead);
		*pLength += pSecInfo->EncryptionHeadOverhead;
	}	

	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	pRmRptPkt =  (PRM_REPORT_PACKET)(Buffer + *pLength); 

	//
	// 1. Cisco Aironet SNAP Header.
	//
	PlatformMoveMemory( pRmRptPkt->SnapHeader, CiscoAironetSnap, CISCO_AIRONET_SNAP_LENGTH);
	*pLength += CISCO_AIRONET_SNAP_LENGTH;

	//
	// 2. IAPP ID & Length 
	//
	IappIdLen = (CISCO_RM_RPT_IAPP_LENGTH - CISCO_AIRONET_SNAP_LENGTH - 1) + posRpts->Length;	

	RT_TRACE(COMP_SEND, DBG_LOUD, ("IAPP ID & Length: 0x%04X\n", IappIdLen));
	RT_ASSERT((IappIdLen < 0x1000), ("ConstructCcxRmReport(): IappIdLen(%#X) should < 0x1000 !!!\n", IappIdLen));
	SET_RMRPT_IAPPIDLEN(pRmRptPkt, IappIdLen);

	//
	// 3. IAPP Type 
	//
	SET_RMRPT_IAPPTYPE(pRmRptPkt, CCX_RM_IAPP_TYPE);

	//
	// 4. IAPP SubType 
	//
	SET_RMRPT_IAPPSUBTYPE(pRmRptPkt, CCX_RM_REPORT_IAPP_SUBTYPE);

	//
	// 5. Destination MAC Address.
	//
	PlatformZeroMemory(pRmRptPkt->DstAddr, 6);

	//
	// 6. Source MAC Address.
	//
	PlatformMoveMemory(pRmRptPkt->SrcAddr, Adapter->CurrentAddress, 6);

	//
	// 7. Dialog Token.
	//
	if(pRmReports->bAutonomous == FALSE)
		SET_RMRPT_DIALOGTOKEN(pRmRptPkt, pRmReports->DialogToken);
	else
		SET_RMRPT_DIALOGTOKEN(pRmRptPkt, 0);

	//
	// 8. Measurement Report Elements.
	//
	PlatformMoveMemory(pRmRptPkt->Elements, posRpts->Octet, posRpts->Length);
	*pLength += IappIdLen;

	RT_PRINT_DATA((COMP_RM|COMP_SEND), DBG_LOUD, ("CCX RM Report:\n"), Buffer, *pLength);

	RT_TRACE(COMP_SEND, DBG_TRACE, ("- ConstructCcxRmReport()\n"));
}

//
// Description:
//	Construct the packet of dot11k radio measurment report.
// By Bruce, 2009-07-23.
//
VOID
ConstructDot11kRmReport(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN	PRT_RM_REQUESTS		pRmRequests,
	IN	PRT_RM_REPORTS		pRmReports
	)
{
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	pu1Byte 					pHeader = Buffer;
	POCTET_STRING				posRpts = &(pRmReports->osMeasRptEles);

	RT_TRACE(COMP_SEND, DBG_LOUD, ("+ ConstructDot11kRmReport()\n"));

	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Action);

	SET_80211_HDR_ADDRESS1(pHeader, pRmRequests->SrcAddr);
	SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid);

	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);
	*pLength = sMacHdrLng;

	SET_ACTFRAME_CATEGORY_CODE(pHeader, ACT_CAT_RM);
	SET_ACTFRAME_ACTION_CODE(pHeader, ACT_RM_REPORT);
	SET_ACTFRAME_DIALOG_TOKEN(pHeader, pRmRequests->DialogToken);
	*pLength += 3;

	//
	// Measurement Report Elements.
	//
	PlatformMoveMemory((pHeader + *pLength), posRpts->Octet, posRpts->Length);

	*pLength += posRpts->Length;

	RT_PRINT_DATA((COMP_RM|COMP_SEND), DBG_LOUD, ("ConstructDot11kRmReport:\n"), Buffer, *pLength);

	RT_TRACE(COMP_SEND, DBG_TRACE, ("- ConstructDot11kRmReport()\n"));
}

//Add for ROGUE AP 2007.07.28 , by CCW
//Description: Construct CCX ROGUE AP Report packet.
VOID
ConstructCcxROGUEAPReport(
	PADAPTER			Adapter,
	pu1Byte				Buffer,
	pu4Byte				pLength,
	int                             index
)
{
	PMGNT_INFO     				pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS				pStaQos = pMgntInfo->pStaQos;
	PRT_SECURITY_T				pSecInfo = &(pMgntInfo->SecurityInfo);
	pu1Byte					pHeader = Buffer;
	static u1Byte 				CiscoAironetSnap[CISCO_AIRONET_SNAP_LENGTH] = { 0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00};
	PROGUEAP_REPORT_PACKET			pROGUEAPRptPkt = NULL;
	static u1Byte                           ROGUEAPRptLen[2] = {0x00, 0x28};
	static u1Byte                           AuthTimeoutFailure[2] = {0x00,0x02};
	static u1Byte                           ChallengeFromApFailure[2] = {0x00,0x03};
	static u1Byte                           ChallengeToApFailure[2] = {0x00,0x04};
	static u1Byte                           Invalidauthenticationtype[2] = {0x00,0x01};
	ULONG	ln;
	//RT_TRACE(COMP_SEND), DBG_TRACE, ("+ ConstructCcxRmReport()\n"));
	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------

	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
	SET_80211_HDR_TO_DS(pHeader, 1);
	SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid); // BSSID
	SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress); //SA
	SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid);  //DA

	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);
	*pLength = sMacHdrLng;

	//-------------------------------------------------------------------------
	// Qos Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pStaQos->CurrentQosMode > QOS_DISABLE)
	{
		SET_80211_HDR_QOS_EN(pHeader, 1);
		PlatformZeroMemory(&(Buffer[*pLength]), sQoSCtlLng);
		*pLength += sQoSCtlLng;
	}

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pSecInfo->SecLvl > RT_SEC_LVL_NONE)
	{
		PlatformZeroMemory(&(Buffer[*pLength]), pSecInfo->EncryptionHeadOverhead);
		*pLength += pSecInfo->EncryptionHeadOverhead;
	}	

	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	pROGUEAPRptPkt =  (PROGUEAP_REPORT_PACKET)(Buffer + *pLength); 

	//
	// 1. Cisco Aironet SNAP Header.
	//
	PlatformMoveMemory( pROGUEAPRptPkt->SnapHeader, CiscoAironetSnap, CISCO_AIRONET_SNAP_LENGTH);
	
	*pLength += CISCO_AIRONET_SNAP_LENGTH; 


	//
	// 2.Set length :length in bytes of all the following fields including this field (40 bytes)
	//

	PlatformMoveMemory(&(pROGUEAPRptPkt->ROGUEAPLen) , ROGUEAPRptLen , 2 );
	
	//
	// 3.message type : 0x40
	//
	
	pROGUEAPRptPkt->message_type = H2N1BYTE(ROGUEAP_message_type);
	
	//
	// 4.function code : 0x8e
	//
	
	pROGUEAPRptPkt->function_code = H2N1BYTE(ROGUEAP_function_code);
	
	//
	// 5.Destination MAC Address. 
	//
	PlatformMoveMemory(pROGUEAPRptPkt->DstAddr, pMgntInfo->Bssid, 6);
	
	//
	// 6.Source MAC Address.
	//
	PlatformMoveMemory(pROGUEAPRptPkt->SrcAddr, Adapter->CurrentAddress, 6);
	
	//
	// 7.Failure Reason
	//
	switch(pMgntInfo->ROGUE_AP_ENTEY[index].code)
	{
		case MH_CCX_ROGUE_AP_STATUS_AuthTimeoutFailure:
			//pROGUEAPRptPkt->Failure_Reason = H2N2BYTE(AuthTimeoutFailure);
			PlatformMoveMemory(&(pROGUEAPRptPkt->Failure_Reason) , AuthTimeoutFailure , 2 );
			break;
		case MH_CCX_ROGUE_AP_STATUS_ChallengeFromApFailure:
			//pROGUEAPRptPkt->Failure_Reason = H2N2BYTE(ChallengeFromApFailure);
			PlatformMoveMemory(&(pROGUEAPRptPkt->Failure_Reason) , ChallengeFromApFailure , 2 );
			break;
		case MH_CCX_ROGUE_AP_STATUS_ChallengeToApFailure:
			//pROGUEAPRptPkt->Failure_Reason = H2N2BYTE(ChallengeToApFailure);
			PlatformMoveMemory(&(pROGUEAPRptPkt->Failure_Reason) , ChallengeToApFailure , 2 );
			break;
		case MH_CCX_ROGUE_AP_STATUS_Invalidauthenticationtype :
			//pROGUEAPRptPkt->Failure_Reason = H2N2BYTE(Invalidauthenticationtype);
			PlatformMoveMemory(&(pROGUEAPRptPkt->Failure_Reason) , Invalidauthenticationtype , 2 );
			break;
	}

	
	//
	// 8. Rogue AP Mac address
	//
	PlatformMoveMemory(pROGUEAPRptPkt->RogueAPAddr, pMgntInfo->ROGUE_AP_ENTEY[index].bssid , 6);
	
	//
	// 9.Rogue AP name
	//
	PlatformZeroMemory(pROGUEAPRptPkt->RogueAPname, 16);

	*pLength += 40;

	ln = *pLength;
	

	//RT_PRINT_DATA((COMP_RM|COMP_SEND), DBG_LOUD, ("ROGUE AP Report: "), Buffer, *pLength);
		
}


VOID
ConstructMaFrameHdr(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pAddr,
	IN	u1Byte			Category,
	IN	u1Byte			Action,
	OUT	POCTET_STRING		posMaFrame
	)
{
	pu1Byte				pMaPartial = posMaFrame->Octet;
	u1Byte				MaHdr[5];
	OCTET_STRING			osTemp;

	SET_80211_HDR_FRAME_CONTROL(pMaPartial,0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pMaPartial,Type_Action);
	SET_80211_HDR_DURATION(pMaPartial,0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pMaPartial,0);
	SET_80211_HDR_ADDRESS1(pMaPartial, pAddr);
	SET_80211_HDR_ADDRESS2(pMaPartial, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pMaPartial, Adapter->MgntInfo.Bssid);
	posMaFrame->Length = sMacHdrLng;

	MaHdr[0] = Category;
	MaHdr[1] = Action;

	FillOctetString(osTemp, MaHdr, 2);
	PacketAppendData(posMaFrame, osTemp);	
	
}

// 
// Description:
// 	Append the additional IEs to the Beacon frame, by Bruce, 2007-08-13.
// Note:
//	No need to specify the Element IDs, because the input buffer has included the ID and length.
//
VOID
AppendAdditionalIEs(
	OUT		POCTET_STRING	packet,
	IN		OCTET_STRING	ies
	)
{
	RT_TRACE(COMP_MLME, DBG_TRACE, ("AppendAdditionalIEs(): Length = %d\n", ies.Length));
	PacketAppendData(packet, ies);
}

//
// Description:
//	Construct Qos ADDTS packet by the input ts.
//
VOID
ConstructQosADDTSPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	u1Byte			DialogToken,
	IN	PQOS_TSTREAM	pTs,
	IN	u4Byte			numTspec
	)
{
	PMGNT_INFO     			pMgntInfo = &(Adapter->MgntInfo);
//	PRT_SECURITY_T			pSecInfo = &(pMgntInfo->SecurityInfo);
	PSTA_QOS				pStaQos = pMgntInfo->pStaQos;
	OCTET_STRING			osAddTs;
	OCTET_STRING			osTempField;
	u4Byte					i;
	PQOS_TSTREAM			pStream = pTs;
	u1Byte					WMM_Filed[4] = {0};

	RT_ASSERT(pTs!=NULL, ("ConstructQosADDTSPacket(): pTs should not be NULL\n"));

	FillOctetString(osAddTs, Buffer, 0);
	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------

	SET_80211_HDR_FRAME_CONTROL(osAddTs.Octet, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(osAddTs.Octet, Type_Action);
	SET_80211_HDR_ADDRESS1(osAddTs.Octet, pMgntInfo->Bssid);
	SET_80211_HDR_ADDRESS2(osAddTs.Octet, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(osAddTs.Octet, pMgntInfo->Bssid);

	SET_80211_HDR_DURATION(osAddTs.Octet, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(osAddTs.Octet, 0);
	osAddTs.Length += sMacHdrLng;

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	CCX_ConstructQosADDTSPacket(Adapter, &osAddTs);

	//-------------------------------------------------------------------------
	//WMM Action Frame Fixed Field
	//-------------------------------------------------------------------------
	WMM_Filed[0] = WMM_ACTION_CATEGORY_CODE;	// Category Code
	WMM_Filed[1] = (u1Byte)ADDTS_REQ;			// Action Code
	WMM_Filed[2] = DialogToken;					// Dialog Token
	WMM_Filed[3] = 0;							// Status Code

	FillOctetString(osTempField, WMM_Filed, sizeof(WMM_Filed));
	PacketAppendData(&osAddTs, osTempField);
	
	//-------------------------------------------------------------------------
	// TSPEC IE & TSRS
	//-------------------------------------------------------------------------
	for (i = 0; i < numTspec; i ++)
	{
		// TSPEC
		FillOctetString(osTempField, pStream->OutStandingTSpec, sizeof(WMM_TSPEC));
		PacketAppendData(&osAddTs, osTempField);

		// TSRS
		// CCXv4 S54, A STA shall use the TSRS IE in the ADDTS request or (Re-)association Request. This value of Nominal PHY rate must be 
		// greater than or equal to the Minimum PHY rate expressed in the TSPEC IE. By Bruce, 2009-06-23.
		if (QOS_RATE_TO_BPS( QosGetNPR(Adapter, pStream) ) >=
			GET_TSPEC_MIN_PHY_RATE(pStream->OutStandingTSpec) )
		{
			QosConstructTSRS(Adapter, pStream, &osAddTs);
		}
		
		pStream->DialogToken = pStaQos->DialogToken;
		pStream ++;
	}

	//-------------------------------------------------------------------------
	// MHDR IE for MFP  
	//-------------------------------------------------------------------------
	CCX_AppendMHDRIE(Adapter, &osAddTs);

	*pLength = osAddTs.Length;
}

//
// Description:
//	Construct Qos DELTS packet by the input TS.
//
VOID
ConstructQosDELTSPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	PQOS_TSTREAM	pTs,
	IN	u4Byte			numTspec
	)
{
	PMGNT_INFO     			pMgntInfo = &(Adapter->MgntInfo);
//	PRT_SECURITY_T			pSecInfo = &(pMgntInfo->SecurityInfo);
	OCTET_STRING			osDelTs;
	OCTET_STRING			osTempField;
	u4Byte					i;
	PQOS_TSTREAM			pStream = pTs;
	u1Byte					WMM_Filed[4] = {0};

	RT_ASSERT(pTs!=NULL, ("ConstructQosDELTSPacket(): pTs should not be NULL\n"));

	FillOctetString(osDelTs, Buffer, 0);
	
	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	SET_80211_HDR_FRAME_CONTROL(osDelTs.Octet, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(osDelTs.Octet, Type_Action);
	SET_80211_HDR_ADDRESS1(osDelTs.Octet, pMgntInfo->Bssid);
	SET_80211_HDR_ADDRESS2(osDelTs.Octet, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(osDelTs.Octet, pMgntInfo->Bssid);

	SET_80211_HDR_DURATION(osDelTs.Octet, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(osDelTs.Octet, 0);
	osDelTs.Length += sMacHdrLng;

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	CCX_ConstructQosDELTSPacket(Adapter, &osDelTs);
	
	//-------------------------------------------------------------------------
	// WMM Action Frame Fixed Field
	//-------------------------------------------------------------------------
	WMM_Filed[0] = WMM_ACTION_CATEGORY_CODE;	// Category Code
	WMM_Filed[1] = (u1Byte)DELTS;				// Action Code
	WMM_Filed[2] = 0;							// Dialog Token
	WMM_Filed[3] = 0;							// Status Code

	FillOctetString(osTempField, WMM_Filed, sizeof(WMM_Filed));
	PacketAppendData(&osDelTs, osTempField);
	
	//-------------------------------------------------------------------------
	// TSPEC IE
	//-------------------------------------------------------------------------
	for (i = 0; i < numTspec; i ++)
	{
		FillOctetString(osTempField, pStream->TSpec, sizeof(WMM_TSPEC));
		PacketAppendData(&osDelTs, osTempField);
		pStream ++;
	}
	
	//-------------------------------------------------------------------------
	// MHDR IE for MFP  
	//-------------------------------------------------------------------------
	CCX_AppendMHDRIE(Adapter, &osDelTs );

	*pLength = osDelTs.Length;	
}

//
// Description:
//	Construct the CCX packet of Neighbor List Polling to the Cisco AP.
// By Bruce, 2008-03-22.
//
VOID
ConstructCcxNeighborPoll(
	IN	PADAPTER			Adapter,
	OUT	pu1Byte				Buffer,
	OUT	pu4Byte				pLength
	)
{
	PMGNT_INFO 				pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS				pStaQos = pMgntInfo->pStaQos;
	PRT_SECURITY_T			pSecInfo = &(pMgntInfo->SecurityInfo);
	pu1Byte					pHeader = Buffer;
	static u1Byte			CiscoAironetSnap[CISCO_AIRONET_SNAP_LENGTH] = { 0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x00};
	u2Byte 					IappIdLen, tmp2Byte;
	pu1Byte					pIappPkt;

	//
	// Figure out IAPP pakcet length.
	//
	IappIdLen = 0x10; 
	
	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
	SET_80211_HDR_TO_DS(pHeader, 1);
	SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->Bssid);
	SET_80211_HDR_ADDRESS2(pHeader, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pHeader, pMgntInfo->Bssid);

	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);
	*pLength = sMacHdrLng;

	//-------------------------------------------------------------------------
	// Qos Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pStaQos->CurrentQosMode > QOS_DISABLE)
	{
		SET_80211_HDR_QOS_EN(pHeader, 1);
		PlatformZeroMemory(&(Buffer[*pLength]), sQoSCtlLng);
		*pLength += sQoSCtlLng;
	}

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pSecInfo->EncryptionHeadOverhead > 0)
	{
		PlatformZeroMemory(&(Buffer[*pLength]), pSecInfo->EncryptionHeadOverhead);
		*pLength += pSecInfo->EncryptionHeadOverhead;
	}	

	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	pIappPkt = (Buffer + *pLength); 

	//
	// 1. Cisco Aironet SNAP Header.
	//
	SET_AARPT_IAPP_SNAP_HEADER(pIappPkt, CiscoAironetSnap, CISCO_AIRONET_SNAP_LENGTH);
	*pLength += CISCO_AIRONET_SNAP_LENGTH; 


	//
	// 2. IAPP length field: length of packet including this field.
	//
	tmp2Byte = H2N2BYTE(IappIdLen);
	SET_AARPT_IAPPIDLEN(pIappPkt, &tmp2Byte);
	*pLength +=  2;
	
	//
	// 3. IAPP type: 0x33
	//
	SET_AARPT_IAPPTYPE(pIappPkt, CCX_L2ROAM_IAPP_TYPE);
	*pLength +=  1;
	
	//
	// 4. Function code: 0x01
	//
	SET_AARPT_IAPPFUNCTION(pIappPkt, 0x01);
	*pLength +=  1;
	
	// 
	// 5. Destination MAC Address. 
	//
	SET_AARPT_IAPPDEST(pIappPkt, pMgntInfo->Bssid, 6);
	*pLength +=  6;

	//
	// 6. Source MAC Address.
	//
	SET_AARPT_IAPPSRC(pIappPkt, Adapter->CurrentAddress, 6);
	*pLength +=  6;

	RT_PRINT_DATA(COMP_RM, DBG_LOUD, " ConstructCcxNeighborPoll:\n", Buffer, *pLength);
}


//
// Description:
//	Construct the ARP response packet to support ARP offload.
//
VOID
ConstructARPResponse(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			pIPAddress
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	//PSTA_QOS			pStaQos = pMgntInfo->pStaQos;
	PRT_SECURITY_T			pSecInfo = &(pMgntInfo->SecurityInfo);
	pu1Byte				pARPRspPkt = Buffer;
	static u1Byte			ARPLLCHeader[8] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08, 0x06};
	
	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	SET_80211_HDR_FRAME_CONTROL(pARPRspPkt, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pARPRspPkt, Type_Data);
	SET_80211_HDR_TO_DS(pARPRspPkt, 1);
	SET_80211_HDR_ADDRESS1(pARPRspPkt, pMgntInfo->Bssid);
	SET_80211_HDR_ADDRESS2(pARPRspPkt, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pARPRspPkt, pMgntInfo->Bssid);

	SET_80211_HDR_DURATION(pARPRspPkt, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pARPRspPkt, 0);
	*pLength = sMacHdrLng;

//YJ,del,120503
#if 0
	//-------------------------------------------------------------------------
	// Qos Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pStaQos->CurrentQosMode > QOS_DISABLE)
	{
		SET_80211_HDR_QOS_EN(pARPRspPkt, 1);
		PlatformZeroMemory(&(Buffer[*pLength]), sQoSCtlLng);
		*pLength += sQoSCtlLng;
	}
#endif
	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pSecInfo->EncryptionHeadOverhead > 0)
	{
		PlatformZeroMemory(&(Buffer[*pLength]), pSecInfo->EncryptionHeadOverhead);
		*pLength += pSecInfo->EncryptionHeadOverhead;
		SET_80211_HDR_WEP(pARPRspPkt, 1);  //Suggested by CCW.
	}	

	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	pARPRspPkt =  (pu1Byte)(Buffer + *pLength); 
	// LLC header
	PlatformMoveMemory(pARPRspPkt, ARPLLCHeader, 8);	
	*pLength += 8;

	// ARP element
	pARPRspPkt += 8;
	SET_ARP_PKT_HW(pARPRspPkt, 0x0100);
	SET_ARP_PKT_PROTOCOL(pARPRspPkt, 0x0008);	// IP protocol
	SET_ARP_PKT_HW_ADDR_LEN(pARPRspPkt, 6);
	SET_ARP_PKT_PROTOCOL_ADDR_LEN(pARPRspPkt, 4);
	SET_ARP_PKT_OPERATION(pARPRspPkt, 0x0200); // ARP response
	SET_ARP_PKT_SENDER_MAC_ADDR(pARPRspPkt, Adapter->CurrentAddress);
	SET_ARP_PKT_SENDER_IP_ADDR(pARPRspPkt, pIPAddress);
	SET_ARP_PKT_TARGET_MAC_ADDR(pARPRspPkt, Adapter->CurrentAddress);
	SET_ARP_PKT_TARGET_IP_ADDR(pARPRspPkt, pIPAddress);
	*pLength += 28;

	RT_PRINT_DATA(COMP_INIT, DBG_LOUD, "ConstructARPResponse():\n", Buffer, *pLength);
}

VOID
ConstructCtsPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			pStaAddr
	)
{
	pu1Byte			pCts;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> ConstructCtsPacket()\n"));
	
	pCts = Buffer;

	SET_80211_HDR_FRAME_CONTROL(pCts, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pCts, Type_CTS);
	SET_80211_HDR_DURATION(pCts, 0);
	
	SET_80211_HDR_ADDRESS1(pCts, pStaAddr);

	*pLength = 10;
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== ConstructCtsPacket()\n"));
}


//
// Description:
//	Construct the IP66 NS packet to support NS offload.
//
// buffer : 
//		Packet buffer
// pLength : 
//		Packet Length after construct 
// PTAAdress :
//		IPv6 , Target Link address ( MAC address )
VOID
ConstructNSPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			pTAAddress   
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	//PSTA_QOS			pStaQos = pMgntInfo->pStaQos;
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	pu1Byte				pNSRspPkt = Buffer;
	static u1Byte			NSLLCHeader[8] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x86, 0xDD};
	static u1Byte			IPv6HeadInfo[4] = {0x60, 0x00, 0x00, 0x00};
	static u1Byte			IPv6HeadContx[4] = {0x00, 0x20, 0x3a, 0xff};
	static u1Byte			ICMPv6Head[8] = {0x88, 0x00, 0x00, 0x00 , 0x60 , 0x00 , 0x00 , 0x00};
	
	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	SET_80211_HDR_FRAME_CONTROL(pNSRspPkt, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(pNSRspPkt, Type_Data);
	SET_80211_HDR_TO_DS(pNSRspPkt, 1);
	SET_80211_HDR_ADDRESS1(pNSRspPkt, pMgntInfo->Bssid);
	SET_80211_HDR_ADDRESS2(pNSRspPkt, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(pNSRspPkt, pMgntInfo->Bssid);

	SET_80211_HDR_DURATION(pNSRspPkt, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pNSRspPkt, 0);
	*pLength = sMacHdrLng;


#if 0
	//-------------------------------------------------------------------------
	// Qos Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pStaQos->CurrentQosMode > QOS_DISABLE)
	{
		SET_80211_HDR_QOS_EN(pARPRspPkt, 1);
		PlatformZeroMemory(&(Buffer[*pLength]), sQoSCtlLng);
		*pLength += sQoSCtlLng;
	}
#endif
	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------
	if(pSecInfo->EncryptionHeadOverhead > 0)
	{
		PlatformZeroMemory(&(Buffer[*pLength]), pSecInfo->EncryptionHeadOverhead);
		*pLength += pSecInfo->EncryptionHeadOverhead;
		SET_80211_HDR_WEP(pNSRspPkt, 1);  //Suggested by CCW.
	}	

	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	pNSRspPkt =  (pu1Byte)(Buffer + *pLength); 
	// LLC header
	PlatformMoveMemory(pNSRspPkt, NSLLCHeader, 8);	
	pNSRspPkt += 8;
	*pLength += 8;

	//-------------------------------------------------------------------------
	// IPv6 Heard 
	//-------------------------------------------------------------------------
	// 1 . Information (4 bytes): 0x60 0x00 0x00 0x00
	PlatformMoveMemory(pNSRspPkt, IPv6HeadInfo, 4);	
	pNSRspPkt += 4;
	*pLength += 4;
	// 2 . playload : 0x00 0x20 , NextProt : 0x3a (ICMPv6) HopLim : 0xff 
	PlatformMoveMemory(pNSRspPkt, IPv6HeadContx, 4);	
	pNSRspPkt += 4;
	*pLength += 4;
	// 3 . SA : 16 bytes , DA : 16 bytes ( Fw will filled )
	PlatformZeroMemory(pNSRspPkt , 32);
	pNSRspPkt += 32;
	*pLength += 32;

	//-------------------------------------------------------------------------
	// ICMPv6  
	//-------------------------------------------------------------------------
	// 1. Type : 0x88 (NA) , Code : 0x00 , ChechSum : 0x00 0x00 (RSvd) NAFlag: 0x60 0x00 0x00 0x00 ( Solicited , Override)
	PlatformMoveMemory(pNSRspPkt, ICMPv6Head, 8);	
	pNSRspPkt += 8;
	*pLength += 8;
	// 2. TA : 16 bytes
	PlatformZeroMemory(pNSRspPkt , 16);
	pNSRspPkt += 16;
	*pLength += 16;
	//-------------------------------------------------------------------------
	// ICMPv6  Target Link Layer address 
	//-------------------------------------------------------------------------
	pNSRspPkt[0] = 0x02 ; // Type
	pNSRspPkt[1] = 0x01 ; // Len 1 unit of 8 octes 
	pNSRspPkt += 2;
	*pLength += 2;
	//PlatformMoveMemory(pNSRspPkt, pTAAddress, 6);
	PlatformZeroMemory(pNSRspPkt , 6);
	pNSRspPkt += 6;
	*pLength += 6;

	RT_PRINT_DATA(COMP_INIT, DBG_LOUD, "ConstructNSPacket():\n", Buffer, *pLength);
}

//
// Description:
//	Construct SA Query request packet.
//	[in] Adapter -
//		The adapter context.
//	[in] Buffer -
//		The buffer for the constructed packet.
//	[out] pLength -
//		Return the length in byte for the constructed packet.
//	[in] pTargetSta -
//		Target STA which receives this frame.
//	[in] Identifier -
//		The identifier to fill in the SA query request.
// By Bruce, 2014-12-31.
//
VOID
ConstructSAQeuryReq(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN	pu1Byte				pTargetSta,
	IN	u2Byte				Identifier
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	OCTET_STRING		osFrame;

	RT_TRACE_F(COMP_SEC, DBG_LOUD, ("SA Identifier = 0x%04X\n", Identifier));

	FillOctetString(osFrame, Buffer, 0);

	SET_80211_HDR_FRAME_CONTROL(osFrame.Octet, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(osFrame.Octet, Type_Action);

	SET_80211_HDR_ADDRESS1(osFrame.Octet, pTargetSta);
	SET_80211_HDR_ADDRESS2(osFrame.Octet, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(osFrame.Octet, pMgntInfo->Bssid);

	SET_80211_HDR_DURATION(osFrame.Octet, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(osFrame.Octet, 0);
	osFrame.Length = sMacHdrLng;

	SET_ACTFRAME_CATEGORY_CODE(osFrame.Octet, ACT_CAT_SAQ);
	SET_ACTFRAME_ACTION_CODE(osFrame.Octet, ACT_SA_REQ);
	SET_ACTFRAME_SA_QUERY_ID(osFrame.Octet, Identifier);
	osFrame.Length += 4;

	*pLength = osFrame.Length;

	RT_PRINT_DATA((COMP_SEC), DBG_LOUD, ("ConstructSAQeuryReq:\n"), Buffer, *pLength);
}



//
// Description:
//	Construct SA Query response packet.
//	[in] Adapter -
//		The adapter context.
//	[in] Buffer -
//		The buffer for the constructed packet.
//	[out] pLength -
//		Return the length in byte for the constructed packet.
//	[in] pTargetSta -
//		Target STA which receives this frame.
//	[in] Identifier -
//		The identifier from the SA query request frame to fill in the SA query response.
// By Bruce, 2014-12-24.
//
VOID
ConstructSAQeuryRsp(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN	pu1Byte				pTargetSta,
	IN	u2Byte				Identifier
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	OCTET_STRING		osFrame;

	RT_TRACE_F(COMP_SEC, DBG_LOUD, ("SA Identifier = 0x%04X\n", Identifier));

	FillOctetString(osFrame, Buffer, 0);

	SET_80211_HDR_FRAME_CONTROL(osFrame.Octet, 0);
	SET_80211_HDR_TYPE_AND_SUBTYPE(osFrame.Octet, Type_Action);

	SET_80211_HDR_ADDRESS1(osFrame.Octet, pTargetSta);
	SET_80211_HDR_ADDRESS2(osFrame.Octet, Adapter->CurrentAddress);
	SET_80211_HDR_ADDRESS3(osFrame.Octet, pMgntInfo->Bssid);

	SET_80211_HDR_DURATION(osFrame.Octet, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(osFrame.Octet, 0);
	osFrame.Length = sMacHdrLng;

	SET_ACTFRAME_CATEGORY_CODE(osFrame.Octet, ACT_CAT_SAQ);
	SET_ACTFRAME_ACTION_CODE(osFrame.Octet, ACT_SA_RSP);
	SET_ACTFRAME_SA_QUERY_ID(osFrame.Octet, Identifier);
	osFrame.Length += 4;

	*pLength = osFrame.Length;

	RT_PRINT_DATA((COMP_SEC), DBG_LOUD, ("ConstructSAQeuryRsp:\n"), Buffer, *pLength);
}

//
// Description:
//	Append Fast Transition MD (Mobility Domain) IE to this packet.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] contentType -
//		The type of content to append packet. This field also indicates which type of posOutContent.
//		CONTENT_PKT_TYPE_xxx -
//	[in] pInfoBuf -
//		This field is determined by contentType and shall be casted to the corresponding type.
//		pInfoBuf is PRT_WLAN_BSS if contentType is CONTENT_PKT_TYPE_CLIENT.
//	[in] InfoBufLen -
//		Length in byte of pInfoBuf.
//	[in] maxBufLen -
//		The max length of posOutContent buffer in byte.
//	[out] posOutContent -
//		The OCTECT_STRING structure for the content buffer and length.
//		posOutContent is a 802.11 packet if contentType is CONTENT_PKT_TYPE_802_11.
// Return:
//	RT_STATUS_SUCCESS, if the content is appended withour error.
//	RT_STATUS_BUFFER_TOO_SHORT, if the max length of buffer is not long enough.
//	RT_STATUS_INVALID_STATE, if the security parameter is mismatched.
// Remark:
//	To determine what to append to the output content, the pTargetBss, contentType, and posOutContent
//	shall be filled when calling this function. If MD IE will be appended to an association request frame in posOutContent,
//	the 802.11 packet type and MAC header shall be filled in posOutContent before calling this function.
// By Bruce, 2015-12-16.
//
RT_STATUS
FT_AppendMdIE(
	IN	PADAPTER				pAdapter,
	IN	CONTENT_PKT_TYPE		contentType,
	IN	PVOID					pInfoBuf,
	IN	u4Byte					InfoBufLen,
	IN	u4Byte					maxBufLen,
	OUT POCTET_STRING			posOutContent
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PRT_SECURITY_T			pSecInfo = &pAdapter->MgntInfo.SecurityInfo;
	PFT_INFO_ENTRY			pEntry = NULL;
	OCTET_STRING			osMdIe = {NULL, 0};
	PRT_WLAN_BSS			pTargetBss = NULL;
	u1Byte					pktType = 0xFF;

	CHECK_NULL_RETURN_STATUS(posOutContent);	

	do
	{
		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT | CONTENT_PKT_TYPE_AP | CONTENT_PKT_TYPE_IBSS))
		{
			if(RT_SEC_LVL_WPA2 != pSecInfo->SecLvl)
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("SecLvl (%d) != RT_SEC_LVL_WPA2\n", pSecInfo->SecLvl));
				rtStatus = RT_STATUS_INVALID_STATE;
				break;
			}
		}

		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_802_11))
		{
			pktType = PacketGetType(*posOutContent);
		}
		
		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT))
		{
			if(!pInfoBuf || InfoBufLen < sizeof(RT_WLAN_BSS))			
			{
				RT_TRACE_F(COMP_SEC, DBG_SERIOUS, ("Invalid input buffer or length, pInfoBuf = %p, InfoBufLen = %d\n", pInfoBuf, InfoBufLen));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
			pTargetBss = (PRT_WLAN_BSS)pInfoBuf;

			if(NULL == (pEntry = FtGetEntry(pAdapter, pTargetBss->bdBssIdBuf)))
			{
				RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "FT_AppendMdIE() No Entry for BSSID = ", pTargetBss->bdBssIdBuf);
				rtStatus = RT_STATUS_INVALID_STATE;
				break;
			}

			// Do not append RSN IE in default.
			rtStatus = RT_STATUS_INVALID_DATA;
			switch(pktType)
			{
			default:
				rtStatus = RT_STATUS_INVALID_DATA;
				break;

			case Type_Auth:
				{
					if(pEntry)
					{
						if(pEntry->PMKR0NameLen > 0 && pEntry->FTELen > 0 && pEntry->MDELen)
						{
							rtStatus = RT_STATUS_SUCCESS;
						}
					}
				}
				break;

			case Type_Asoc_Req:
			case Type_Reasoc_Req:
				rtStatus = RT_STATUS_SUCCESS;
				break;
			}

			if(RT_STATUS_SUCCESS != rtStatus)
			{
				// No need to append This IE
				RT_TRACE_F(COMP_MLME, DBG_TRACE, ("pktType = 0x%02X, no need to append MD IE\n", pktType));
				break;
			}
			
			// MD Element
			FillOctetString(osMdIe, pEntry->MDE, (u2Byte)(pEntry->MDELen));	
		}
		else
		{
			//3 // ToDo: Support AP mode later.
			rtStatus = RT_STATUS_NOT_SUPPORT;
			break;			
		}

		if(maxBufLen < ((u4Byte)posOutContent->Length + osMdIe.Length))
		{
			RT_TRACE_F(COMP_MLME, DBG_WARNING, ("maxBufLen (%d) <= packet + MDIE len (%d)\n", maxBufLen, (posOutContent->Length + osMdIe.Length)));
			rtStatus = RT_STATUS_BUFFER_TOO_SHORT;
			break;
		}
		RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FT_AppendMdIE() Append MDIE:\n", osMdIe.Octet, osMdIe.Length);
		AppendAdditionalIEs(posOutContent, osMdIe);
	}while(FALSE);

	FunctionOut(COMP_MLME);

	return rtStatus;
}

//
// Description:
//	Append Fast Transition FT (Fast BSS transition) IE to this packet.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] contentType -
//		The type of content to append packet. This field also indicates which type of posOutContent.
//		CONTENT_PKT_TYPE_xxx -
//	[in] pInfoBuf -
//		This field is determined by contentType and shall be casted to the corresponding type.
//		pInfoBuf is PRT_WLAN_BSS if contentType is CONTENT_PKT_TYPE_CLIENT.
//	[in] InfoBufLen -
//		Length in byte of pInfoBuf.
//	[in] maxBufLen -
//		The max length of posOutContent buffer in byte.
//	[out] posOutContent -
//		The OCTECT_STRING structure for the content buffer and length.
//		posOutContent is a 802.11 packet if contentType is CONTENT_PKT_TYPE_802_11.
// Return:
//	RT_STATUS_SUCCESS, if the content is appended withour error.
//	RT_STATUS_BUFFER_TOO_SHORT, if the max length of buffer is not long enough.
//	RT_STATUS_INVALID_STATE, if the security parameter is mismatched.
// Remark:
//	To determine what to append to the output content, the pTargetBss, contentType, and posOutContent
//	shall be filled when calling this function. If FT IE will be appended to an association request frame in posOutContent,
//	the 802.11 packet type and MAC header shall be filled in posOutContent before calling this function.
// By Bruce, 2015-12-16.
//
RT_STATUS
FT_AppendFtIE(
	IN	PADAPTER				pAdapter,
	IN	CONTENT_PKT_TYPE		contentType,
	IN	PVOID					pInfoBuf,
	IN	u4Byte					InfoBufLen,
	IN	u4Byte					maxBufLen,
	OUT POCTET_STRING			posOutContent
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PRT_SECURITY_T			pSecInfo = &pAdapter->MgntInfo.SecurityInfo;
	PFT_INFO_ENTRY			pEntry = NULL;
	OCTET_STRING			osFtIe = {NULL, 0};
	PRT_WLAN_BSS			pTargetBss = NULL;
	u1Byte					pktType = 0xFF;

	CHECK_NULL_RETURN_STATUS(posOutContent);	

	do
	{
		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT | CONTENT_PKT_TYPE_AP | CONTENT_PKT_TYPE_IBSS))
		{
			if(RT_SEC_LVL_WPA2 != pSecInfo->SecLvl)
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("SecLvl (%d) != RT_SEC_LVL_WPA2\n", pSecInfo->SecLvl));
				rtStatus = RT_STATUS_INVALID_STATE;
				break;
			}
		}

		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_802_11))
		{
			pktType = PacketGetType(*posOutContent);
		}
		
		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT))
		{
			if(!pInfoBuf || InfoBufLen < sizeof(RT_WLAN_BSS))			
			{
				RT_TRACE_F(COMP_SEC, DBG_SERIOUS, ("Invalid input buffer or length, pInfoBuf = %p, InfoBufLen = %d\n", pInfoBuf, InfoBufLen));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
			pTargetBss = (PRT_WLAN_BSS)pInfoBuf;

			if(NULL == (pEntry = FtGetEntry(pAdapter, pTargetBss->bdBssIdBuf)))
			{
				RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "FT_AppendMdIE() No Entry for BSSID = ", pTargetBss->bdBssIdBuf);
				rtStatus = RT_STATUS_INVALID_STATE;
				break;
			}

			// Do not append RSN IE in default.
			rtStatus = RT_STATUS_INVALID_DATA;
			switch(pktType)
			{
			default:
				rtStatus = RT_STATUS_INVALID_DATA;
				break;

			case Type_Auth:
				{
					if(pEntry)
					{
						if(pEntry->PMKR0NameLen > 0 && pEntry->FTELen > 0 && pEntry->MDELen)
						{
							rtStatus = RT_STATUS_SUCCESS;
						}
					}
				}
				break;

			case Type_Asoc_Req:
			case Type_Reasoc_Req:
				rtStatus = RT_STATUS_SUCCESS;
				break;
			}

			if(RT_STATUS_SUCCESS != rtStatus)
			{
				// No need to append This IE
				RT_TRACE_F(COMP_MLME, DBG_TRACE, ("pktType = 0x%02X, no need to append Ft IE\n", pktType));
				break;
			}
			
			// MD Element
			FillOctetString(osFtIe, pEntry->FTE, (u2Byte)(pEntry->FTELen));				
		}
		else
		{
			//3 // ToDo: Support AP mode later.
			rtStatus = RT_STATUS_NOT_SUPPORT;
			break;			
		}

		if(maxBufLen < ((u4Byte)posOutContent->Length + osFtIe.Length))
		{
			RT_TRACE_F(COMP_MLME, DBG_WARNING, ("maxBufLen (%d) <= packet + FTIE len (%d)\n", maxBufLen, (posOutContent->Length + osFtIe.Length)));
			rtStatus = RT_STATUS_BUFFER_TOO_SHORT;
			break;
		}
		RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "FT_AppendFtIE() Append FTIE:\n", osFtIe.Octet, osFtIe.Length);
		AppendAdditionalIEs(posOutContent, osFtIe);
	}while(FALSE);

	FunctionOut(COMP_MLME);

	return rtStatus;
}

