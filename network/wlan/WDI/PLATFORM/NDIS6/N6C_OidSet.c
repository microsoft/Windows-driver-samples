#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "N6C_OidSet.tmh"
#endif


//
// The following helper functions are used to determined if a 
// BSS has the capability of specific wireless mode.
// 2005.01.13, by rcnjko.
//
BOOLEAN
WithWirelessB(
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
WithWirelessG(
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
WithWirelessA(
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
FilterRtBss(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pRtBss
	)
{
	PRT_NDIS6_COMMON pNdisCommon = Adapter->pNdisCommon;
	BOOLEAN bAcceptable = TRUE;
	BOOLEAN bHasB, bHasG, bHasA; 

	// Determine the capability of wireless mode of the BSS.
	bHasB = WithWirelessB(pRtBss);
	bHasG = WithWirelessG(pRtBss);
	bHasA = WithWirelessA(pRtBss);
	
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


/**
**
**20061130 david add in order to copy IE into entry and fill in hidden ssid IE. 
**
**We don't need to worry about Length in this function because it has been handled already
**
/@ first parameter:  Adapter bss list
** /@ second parameter:  management bss list we are going to fill
** 
**/ 
NDIS_STATUS
FillHiddenSSIDinIE(
	IN PADAPTER					Adapter,
	OUT	PDOT11_BSS_ENTRY		pDot11Bss,
	IN PRT_WLAN_BSS				pRtBss
	)
{
	PUCHAR  pSrcBlob = pDot11Bss->ucBuffer;
	ULONG   SizeOfSrcBlob = pDot11Bss->uBufferLength;
	PDOT11_INFO_ELEMENT pInfoElemHdr = NULL;
	u1Byte		tmpIEBuf[BSS_IE_BUF_LEN];
	u4Byte 		tmpIELen=0;
	u4Byte		tmpHiddenLen=0;


	pInfoElemHdr = (PDOT11_INFO_ELEMENT)pSrcBlob;
	if (SizeOfSrcBlob < sizeof(DOT11_INFO_ELEMENT)) 
	{
		// Shouldnt happen. The IE's must already be verified
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	// We shouldn't use the HiddenAP flag to judge the BSS STA is a hidden AP because the
	// flag is undetermineable, just copy SSID into IE in each BSS STA. 
	// 2006.12.20, by shien chang.
	//if (pRtBss->HiddenAP)
	{
		//
		// For SSID, we copy cached Probe SSID if we do not have a SSID
		// in the Beacon. This is to handle hidden SSID. For hidden SSID, 
		// the OS would first do a scan and expects us to indicate the 
		// found SSID so that it can do a connect. Since we may overwrite the 
		// probe with beacon, we copy the cached SSID over
		//
		tmpHiddenLen = pInfoElemHdr->Length;
		tmpIELen=SizeOfSrcBlob-sizeof(DOT11_INFO_ELEMENT)-tmpHiddenLen;
		CopyMem(tmpIEBuf, pSrcBlob+sizeof(DOT11_INFO_ELEMENT)+tmpHiddenLen, tmpIELen);
		    CopyMem(pSrcBlob+sizeof(DOT11_INFO_ELEMENT), pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen);
		pInfoElemHdr->Length=pRtBss->bdSsIdLen;
		CopyMem(pSrcBlob+sizeof(DOT11_INFO_ELEMENT)+pInfoElemHdr->Length, tmpIEBuf, tmpIELen);
		// 
		// Commented out by haich.
		// We shall not expect this is how the length is calculated since we may append 
		// other IEs outside this function, eg: WCN IE. Therefore we shall update it, not
		// re-calculate it. 2008.10.20, haich.
		//
		//pDot11Bss->uBufferLength = (u2Byte)(((pRtBss->IE.Length -sizeof(RT_FIXED_IE_FIELD)) - tmpHiddenLen)+pInfoElemHdr->Length);	
		pDot11Bss->uBufferLength = (u2Byte)((pDot11Bss->uBufferLength - tmpHiddenLen)+pInfoElemHdr->Length);
	}

	return NDIS_STATUS_SUCCESS;
}


BOOLEAN
NeedRemoveWCNIE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pRtBss
	)
{
	BOOLEAN		ret = FALSE;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);	
	OCTET_STRING	osIE;
	OCTET_STRING	osWCN;
#if (WPS_SUPPORT == 1)
{
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);
	if(!pSimpleConfig->bEnabled || Adapter->bInHctTest)
	{
		if(pRtBss->bdSimpleConfIE.Length > 0)
			ret = TRUE;
	}
}
#endif

	return ret;
}


VOID
N6CTranslateRtBssToDot11Bss(
	IN	PADAPTER		Adapter,
	OUT	PVOID			pDot11Bss,
	IN	PRT_WLAN_BSS	pRtBss
	)
{
	NDIS_STATUS         	ndisStatus = NDIS_STATUS_SUCCESS;
	PDOT11_BSS_ENTRY	pDot11BssEntry = (PDOT11_BSS_ENTRY)pDot11Bss;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	LARGE_INTEGER	CurrentTime;

	pDot11BssEntry->uPhyId = N6CQuery_DOT11_RTBSS_OPERATING_PHYID(Adapter, pRtBss);
	
	pDot11BssEntry->PhySpecificInfo.uChCenterFrequency = MgntGetChannelFrequency(pRtBss->ChannelNumber);
	
	
	CopyMem(pDot11BssEntry->dot11BSSID, 
			pRtBss->bdBssIdBuf,
			sizeof(DOT11_MAC_ADDRESS));
	if (pRtBss->bdCap & cIBSS)
	{
		pDot11BssEntry->dot11BSSType = dot11_BSS_type_independent;
	}
	else
	{
		pDot11BssEntry->dot11BSSType = dot11_BSS_type_infrastructure;
	}

	// Get current timestamp
	NdisGetCurrentSystemTime(&CurrentTime);

	// This value will be showed as the value of "Signal" on those monitor program  	
	pDot11BssEntry->lRSSI = pRtBss->CumRecvSignalPower;
			       
	// Because our HW has signal quality issues, we use signal strength
	// instead of signal quality. 2006.11.27, by shien chang, suggest by David.
	pDot11BssEntry->uLinkQuality = pRtBss->RSSI; // pRtBss->SignalQuality;
	pDot11BssEntry->bInRegDomain = FALSE;
	pDot11BssEntry->usBeaconPeriod = pRtBss->bdBcnPer;
	pDot11BssEntry->ullTimestamp = pRtBss->bdTstamp;
	pDot11BssEntry->ullHostTimestamp = CurrentTime.QuadPart; // Timestamp which records when Beacon/ProbeRsp received, by Roger. 2015.09.04
	pDot11BssEntry->usCapabilityInformation = pRtBss->bdCap;
	pDot11BssEntry->uBufferLength = 0;

	// 2008/11/22 MH SD1 Richard suggest to do error check.
	if (pRtBss->IE.Length > sizeof(RT_FIXED_IE_FIELD))
	{
		// 20090508 Joseph: Revise size checking. Handling hidden SSID IE handling.
		// This makes sure that memory copy operation is not exceed the buffer provided by OS.
		PDOT11_INFO_ELEMENT pDot11SsidElement = 
			(PDOT11_INFO_ELEMENT)(pRtBss->IE.Octet +sizeof(RT_FIXED_IE_FIELD));
		pDot11BssEntry->uBufferLength = pRtBss->IE.Length - sizeof(RT_FIXED_IE_FIELD);
		
		if ( ! BeHiddenSsid(pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen) &&
			BeHiddenSsid((pu1Byte)pDot11SsidElement+sizeof(DOT11_INFO_ELEMENT), pDot11SsidElement->Length) )
		{
			PDOT11_INFO_ELEMENT pFirstIE = (PDOT11_INFO_ELEMENT)pDot11BssEntry->ucBuffer;
			pFirstIE->ElementID = EID_SsId;
			pFirstIE->Length = pRtBss->bdSsIdLen;
			CopyMem(pDot11BssEntry->ucBuffer+sizeof(DOT11_INFO_ELEMENT), 
					pRtBss->bdSsIdBuf,
					pRtBss->bdSsIdLen);
			CopyMem(pDot11BssEntry->ucBuffer+sizeof(DOT11_INFO_ELEMENT)+pRtBss->bdSsIdLen, 
					pRtBss->IE.Octet+sizeof(RT_FIXED_IE_FIELD)+sizeof(DOT11_INFO_ELEMENT)+pDot11SsidElement->Length,
					pRtBss->IE.Length-sizeof(RT_FIXED_IE_FIELD)-sizeof(DOT11_INFO_ELEMENT)-pDot11SsidElement->Length);
					
			pDot11BssEntry->uBufferLength = pDot11BssEntry->uBufferLength - pDot11SsidElement->Length + pRtBss->bdSsIdLen;
		}
		else
		{
			if(!NeedRemoveWCNIE(Adapter, pRtBss))
			{
				CopyMem(pDot11BssEntry->ucBuffer, 
						pRtBss->IE.Octet+sizeof(RT_FIXED_IE_FIELD),
						(pRtBss->IE.Length>sizeof(RT_FIXED_IE_FIELD))?
						(pRtBss->IE.Length-sizeof(RT_FIXED_IE_FIELD)):0);
			}
			else
			{
				static u1Byte		Simpleconf[]={0x00, 0x50, 0xF2, 0x04};
				RT_DOT11_IE		Dot11Ie;
				u4Byte	nextOffset = sizeof(RT_FIXED_IE_FIELD);
				u4Byte	curOffset;
				u4Byte	ieCount = 0, skiplength = 0;

				while(HasNextIE(&pRtBss->IE, nextOffset))
				{
					curOffset = nextOffset;
					Dot11Ie = AdvanceToNextIE(&pRtBss->IE, &nextOffset);
					if (Dot11Ie.Id == 0xDD)
					{
						if(PlatformCompareMemory(Dot11Ie.Content.Octet, Simpleconf, sizeof(Simpleconf)) == 0)
						{
							skiplength = Dot11Ie.Content.Length;
#if (WPS_SUPPORT == 1)
							if(skiplength != 0)
							{
								pDot11BssEntry->uBufferLength = pDot11BssEntry->uBufferLength - skiplength - 2;
							}
#endif
							continue;
						}
					}
					CopyMem(pDot11BssEntry->ucBuffer+ieCount, 
								pRtBss->IE.Octet+curOffset,
								sizeof(DOT11_INFO_ELEMENT)+Dot11Ie.Content.Length);
					ieCount += (Dot11Ie.Content.Length + 2);
				}
				//pDot11BssEntry->uBufferLength = ieCount;
			}
		}
	}

#if (WPS_SUPPORT == 1)
	//
	// Here we append the Wcn IE extracted from the beacon and probe response received
	// from the BSS to the IE buffer we report to the upper layer. 
	// The Wcn IE is filled in by UpdateBssWcnIe().
	// haich, 2008.08.26.
	//
	{
	// Copy Wcn IE from beacon.
	CopyMem(pDot11BssEntry->ucBuffer + pDot11BssEntry->uBufferLength, 
			pRtBss->osBeaconWcnIe.Octet,
			pRtBss->osBeaconWcnIe.Length);
	pDot11BssEntry->uBufferLength += pRtBss->osBeaconWcnIe.Length;

		//RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, 
		//	"beacon wcn ie", pRtBss->osBeaconWcnIe.Octet, pRtBss->osBeaconWcnIe.Length);

	// Copy Wcn IE from ProbeRsp.
	CopyMem(pDot11BssEntry->ucBuffer + pDot11BssEntry->uBufferLength, 
				pRtBss->osProbeRspWcnIe.Octet,
				pRtBss->osProbeRspWcnIe.Length);
	pDot11BssEntry->uBufferLength += pRtBss->osProbeRspWcnIe.Length;
		//RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, 
		//	"probe rsp wcn ie", pRtBss->osProbeRspWcnIe.Octet, pRtBss->osProbeRspWcnIe.Length);

	}	
#endif
}


//
// Translate DOT11_AUTH_ALGORITHM to RT_AUTH_MODE.
// Added by Annie, 2006-10-19.
//
RT_AUTH_MODE
N6CDot11ToAuthMode(
	IN	DOT11_AUTH_ALGORITHM			dot11AuthAlg
	)
{
	RT_AUTH_MODE	AuthMode;

	switch(dot11AuthAlg)
	{
		case DOT11_AUTH_ALGO_80211_OPEN:
			AuthMode = RT_802_11AuthModeOpen;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToAuthMode(): DOT11_AUTH_ALGO_80211_OPEN\n") );
			break;

		case DOT11_AUTH_ALGO_80211_SHARED_KEY:
			AuthMode = RT_802_11AuthModeShared;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToAuthMode(): DOT11_AUTH_ALGO_80211_SHARED_KEY\n") );
			break;
			
		case DOT11_AUTH_ALGO_WPA:
			AuthMode = RT_802_11AuthModeWPA;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToAuthMode(): DOT11_AUTH_ALGO_WPA\n") );
			break;
			
		case DOT11_AUTH_ALGO_WPA_PSK:
			AuthMode = RT_802_11AuthModeWPAPSK;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToAuthMode(): DOT11_AUTH_ALGO_WPA_PSK\n") );
			break;
			
		case DOT11_AUTH_ALGO_RSNA:
			AuthMode = RT_802_11AuthModeWPA2;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToAuthMode(): DOT11_AUTH_ALGO_RSNA\n") );
			break;
			
		case DOT11_AUTH_ALGO_RSNA_PSK:
			AuthMode = RT_802_11AuthModeWPA2PSK;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToAuthMode(): DOT11_AUTH_ALGO_RSNA_PSK\n") );
			break;
			
		default:
			AuthMode = RT_802_11AuthModeOpen;
			RT_TRACE( COMP_SEC, DBG_WARNING, ("N6CDot11ToAuthMode(): unknow case, 0x%X => RT_802_11AuthModeOpen (WARNING!!!)\n", dot11AuthAlg) );
			break;
	}
	
	return AuthMode;
}


//
// Translate DOT11_CIPHER_ALGORITHM to RT_ENC_ALG.
// Added by Annie, 2006-10-19.
//
RT_ENC_ALG
N6CDot11ToEncAlgorithm(
	IN	DOT11_CIPHER_ALGORITHM			dot11CipherAlg,
	IN	u4Byte							dot11CipherKeyLength
	)
{
	RT_ENC_ALG	EncAlg;

	switch( dot11CipherAlg )
	{
		case DOT11_CIPHER_ALGO_NONE:
			EncAlg = RT_ENC_ALG_NO_CIPHER;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): DOT11_CIPHER_ALGO_NONE\n") );
			break;

		case DOT11_CIPHER_ALGO_WEP:
			if (dot11CipherKeyLength == NATIVE_802_11_WEP40_KEY_LENGTH)
			{
				EncAlg = RT_ENC_ALG_WEP40;
				RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): DOT11_CIPHER_ALGO_WEP40\n") );
			}
			else if (dot11CipherKeyLength == NATIVE_802_11_WEP104_KEY_LENGTH)
			{
				EncAlg = RT_ENC_ALG_WEP104;
				RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): DOT11_CIPHER_ALGO_WEP104\n") );
			}
			else
			{
				RT_TRACE(COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): Warning, key length is invalid.\n"));
				EncAlg = RT_ENC_ALG_NO_CIPHER;
			}
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): DOT11_CIPHER_ALGO_WEP\n") );
			break;
			
		case DOT11_CIPHER_ALGO_WEP40:
			EncAlg = RT_ENC_ALG_WEP40;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): DOT11_CIPHER_ALGO_WEP40\n") );
			break;

		case DOT11_CIPHER_ALGO_TKIP:
			EncAlg = RT_ENC_ALG_TKIP;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): DOT11_CIPHER_ALGO_TKIP\n") );
			break;

		case DOT11_CIPHER_ALGO_CCMP:
			EncAlg = RT_ENC_ALG_AESCCMP;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): DOT11_CIPHER_ALGO_CCMP\n") );
			break;

		case DOT11_CIPHER_ALGO_WAPI_SMS4: //For WAPI IHV service support add  by ylb 20111114
			EncAlg = RT_ENC_ALG_SMS4;		
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): DOT11_CIPHER_ALGO_WAPI_SMS4\n") );			
			break;
		case DOT11_CIPHER_ALGO_WEP104:
			EncAlg = RT_ENC_ALG_WEP104;
			RT_TRACE( COMP_SEC, DBG_LOUD, ("N6CDot11ToEncAlgorithm(): DOT11_CIPHER_ALGO_WEP104\n") );
			break;

		default:
			EncAlg = RT_ENC_ALG_NO_CIPHER;
			RT_TRACE( COMP_SEC, DBG_WARNING, ("N6CDot11ToEncAlgorithm(): Unknown AlgorithmId 0x%X (WARNING!!!)\n", dot11CipherAlg) );
			break;
	}

	return	EncAlg;
}


//  N6CSet _DOT11_AUTHENTICATION_ALOGORITHM
//  
//
VOID
N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(
	IN	PADAPTER						Adapter,
	IN 	DOT11_AUTH_ALGORITHM       		AlgorithmId
	)
{

	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);	
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;

	if(FALSE == WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPIIHVSUPPORT, 0))
	{
		if(WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPIENABLE, 0))
			return;		
	}

	pMgntInfo->bRSNAPSKMode = FALSE; // maybe need to move to Ondissconnent.
	pNdisCommon->RegAuthalg = AlgorithmId;

	CCX_8021xModeChange(Adapter, FALSE);
	
	switch( AlgorithmId )
	{
		case DOT11_AUTH_ALGO_80211_OPEN:
			pSecInfo->AuthMode = RT_802_11AuthModeOpen;
			pSecInfo->SecLvl = RT_SEC_LVL_NONE;
//			pSecInfo->PairwiseEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
//			SecClearAllKeys(Adapter);
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(): DOT11_AUTH_ALGO_80211_OPEN\n") );
			break;

		case DOT11_AUTH_ALGO_80211_SHARED_KEY:
			pSecInfo->AuthMode = RT_802_11AuthModeShared;
			pSecInfo->SecLvl = RT_SEC_LVL_NONE;
//			pSecInfo->PairwiseEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
//			SecClearAllKeys(Adapter);
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(): DOT11_AUTH_ALGO_80211_SHARED_KEY\n") );
			break;
			
		case DOT11_AUTH_ALGO_WPA:
			pSecInfo->AuthMode = RT_802_11AuthModeWPA;
			pSecInfo->SecLvl = RT_SEC_LVL_WPA;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(): DOT11_AUTH_ALGO_WPA\n") );
			break;
			
		case DOT11_AUTH_ALGO_WPA_PSK:
			pSecInfo->AuthMode = RT_802_11AuthModeWPAPSK;
			pSecInfo->SecLvl = RT_SEC_LVL_WPA;
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(): DOT11_AUTH_ALGO_WPA_PSK\n") );
			break;
			
		case DOT11_AUTH_ALGO_RSNA:
			pSecInfo->AuthMode = RT_802_11AuthModeWPA2;
			pSecInfo->SecLvl = RT_SEC_LVL_WPA2;
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(): DOT11_AUTH_ALGO_RSNA\n") );
			break;
			
		case DOT11_AUTH_ALGO_RSNA_PSK:
			pSecInfo->AuthMode = RT_802_11AuthModeWPA2PSK;
			pSecInfo->SecLvl = RT_SEC_LVL_WPA2;
			//Add for RSNA IBSS , by CCW
			if(pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC)
			{
				if(pMgntInfo->bRegAdhocUseHWSec == 0)
				{
					pMgntInfo->bRSNAPSKMode = TRUE;
				}
				pSecInfo->PairwiseEncAlgorithm = RT_ENC_ALG_AESCCMP;
				pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_AESCCMP;
			}
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(): DOT11_AUTH_ALGO_RSNA_PSK\n") );
			break;
			
		case DOT11_AUTH_ALGO_CCKM:
			pSecInfo->AuthMode = RT_802_11AuthModeCCKM;
			pSecInfo->SecLvl = RT_SEC_LVL_WPA2;
			// TODO: AES Must take for consider

			CCX_8021xModeChange(Adapter, TRUE);
			
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("%s DOT11_AUTH_ALGO_CCKM\n", __FUNCTION__) );
			break;
			
		case DOT11_AUTH_ALGO_WAPI_PSK: //For WAPI IHV service support add  by ylb 20111114
			pSecInfo->AuthMode = RT_802_11AuthModeWAPI;
			pSecInfo->SecLvl = RT_SEC_LVL_WAPI;	
	
			//pSecInfo->wapiInfo.bWapiEnable = TRUE;
			WAPI_QuerySetVariable(Adapter, WAPI_SET, WAPI_VAR_WAPIPSK, TRUE);
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("%s:DOT11_AUTH_ALGO_WAPI_PSK\n", __FUNCTION__) );
			break;	
		case DOT11_AUTH_ALGO_WAPI_CERTIFICATE: //For WAPI IHV service support add  by ylb 20111114
			pSecInfo->AuthMode = RT_802_11AuthModeCertificateWAPI;
			pSecInfo->SecLvl = RT_SEC_LVL_WAPI;	
	
			//pSecInfo->wapiInfo.bWapiEnable = TRUE;
			WAPI_QuerySetVariable(Adapter, WAPI_SET, WAPI_VAR_WAPIPSK, FALSE);
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("%s:DOT11_AUTH_ALGO_WAPI_CERTIFICATE\n", __FUNCTION__) );
			break;
			
		default:
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(): unknow case, 0x%X\n", AlgorithmId) );
			break;
	}


	MgntActSet_802_11_AUTHENTICATION_MODE( Adapter, pSecInfo->AuthMode );
	SecConstructRSNIE(Adapter);
}



// Implementation of OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM setting for NDIS6.
// Added by Annie, 2006-10-10.
// (reference Sta11SetEnabledAuthenticationAlgorithm() in MS's code)
//
NDIS_STATUS
N6CSet_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	PDOT11_AUTH_ALGORITHM_LIST		pAuthAlgoList = (PDOT11_AUTH_ALGORITHM_LIST)InformationBuffer;

	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("==> N6CSet_DOT11_ENABLED_AUTHENTICATION_ALGORITHM()\n") );

	*BytesNeeded = sizeof(DOT11_AUTH_ALGORITHM_LIST);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}

	if (!N6_VERIFY_OBJECT_HEADER_DEFAULT(
					pAuthAlgoList->Header, 
					NDIS_OBJECT_TYPE_DEFAULT,
					DOT11_AUTH_ALGORITHM_LIST_REVISION_1,
					sizeof(DOT11_AUTH_ALGORITHM_LIST)) )
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(), NDIS_STATUS_INVALID_DATA\n") );
		return NDIS_STATUS_INVALID_DATA;
	}

	// Must have atleast one entry in the list
	if (pAuthAlgoList->uNumOfEntries < 1)
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(), NDIS_STATUS_INVALID_DATA\n") );
		return NDIS_STATUS_INVALID_DATA;
	}

	if(pAuthAlgoList->uNumOfEntries > 1)
	{
		u1Byte	i = 0;
		RT_TRACE( COMP_OID_SET, DBG_WARNING, ("%s pAuthAlgoList->uNumOfEntries > 1, but we don't support now!\n", __FUNCTION__) );
		for(i = 0; i < pAuthAlgoList->uNumOfEntries; i ++)
		{
			RT_TRACE( COMP_OID_SET, DBG_WARNING, ("%s [%d]AlgorithmId = %d!\n", __FUNCTION__, i, pAuthAlgoList->AlgorithmIds[i]));
		}
	}

	N6CSet_DOT11_AUTHENTICATION_ALOGORITHM(Adapter, pAuthAlgoList->AlgorithmIds[0] );

	*BytesRead = *BytesNeeded;

	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("<== N6CSet_DOT11_ENABLED_AUTHENTICATION_ALGORITHM(): pSecInfo->AuthMode=0x%X\n", pSecInfo->AuthMode) );
	return NDIS_STATUS_SUCCESS;
}


//  N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM
//
//

VOID
N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	IN 	DOT11_CIPHER_ALGORITHM  		AlgorithmId
)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;

	if(FALSE == WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPIIHVSUPPORT, 0))
	{
		if(WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPIENABLE, 0))
			return;		
	}

	pNdisCommon->RegPairwiseAlg= AlgorithmId;
	switch( AlgorithmId )
	{
		case DOT11_CIPHER_ALGO_NONE:
			pSecInfo->PairwiseEncAlgorithm = RT_ENC_ALG_NO_CIPHER;
			pSecInfo->UseDefaultKey = FALSE;
			pSecInfo->EncryptionHeadOverhead = 0;
			pSecInfo->EncryptionTailOverhead = 0;
			pSecInfo->EncryptionStatus = RT802_11WEPDisabled;
			pMgntInfo->LEDAssocState = LED_ASSOC_SECURITY_NONE;
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(): DOT11_CIPHER_ALGO_NONE\n") );
			break;

		case DOT11_CIPHER_ALGO_WEP40:
			pSecInfo->PairwiseEncAlgorithm = RT_ENC_ALG_WEP40;
			pSecInfo->UseDefaultKey = TRUE;
			pSecInfo->EncryptionHeadOverhead = WEP_IV_LEN;
			pSecInfo->EncryptionTailOverhead = WEP_ICV_LEN;
			pSecInfo->EncryptionStatus = RT802_11Encryption1Enabled;
			pMgntInfo->LEDAssocState = LED_ASSOC_SECURITY_BEGIN;
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(): DOT11_CIPHER_ALGO_WEP40\n") );
			pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_WEP64;
			break;

		case DOT11_CIPHER_ALGO_WEP104:
			pSecInfo->PairwiseEncAlgorithm = RT_ENC_ALG_WEP104;
			pSecInfo->UseDefaultKey = TRUE;
			pSecInfo->EncryptionHeadOverhead = WEP_IV_LEN;
			pSecInfo->EncryptionTailOverhead = WEP_ICV_LEN;
			pSecInfo->EncryptionStatus = RT802_11Encryption1Enabled;
			pMgntInfo->LEDAssocState = LED_ASSOC_SECURITY_BEGIN;
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(): DOT11_CIPHER_ALGO_WEP104\n") );
			pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_WEP128;
			break;
			
		case DOT11_CIPHER_ALGO_WEP:
			//
			// At this time, we don't know the key size, defer the setting of algorithm to
			// OID_DOT11_CIPHER_DEFAULT_KEY, 2006.10.26, by shien chang.
			//
			if(OS_SUPPORT_WDI(Adapter))
			{
				pSecInfo->PairwiseEncAlgorithm = (pSecInfo->GroupEncAlgorithm==RT_ENC_ALG_WEP104)?RT_ENC_ALG_WEP104:RT_ENC_ALG_WEP40;
				RT_TRACE(COMP_SEC , DBG_LOUD , ("==>[WDI:WEP]set pSecInfo->PairwiseEncAlgorithm =0x%08X \n",pSecInfo->PairwiseEncAlgorithm));
			}
			else
			{
				pSecInfo->PairwiseEncAlgorithm = (pSecInfo->PairwiseEncAlgorithm==RT_ENC_ALG_WEP104)?RT_ENC_ALG_WEP104:RT_ENC_ALG_WEP40;
				RT_TRACE(COMP_SEC , DBG_LOUD , ("==>[non-WDI:WEP]set pSecInfo->PairwiseEncAlgorithm=0x%08X\n",pSecInfo->PairwiseEncAlgorithm));
			}
				
			pSecInfo->UseDefaultKey = TRUE;
			pSecInfo->EncryptionHeadOverhead = WEP_IV_LEN;
			pSecInfo->EncryptionTailOverhead = WEP_ICV_LEN;
			pSecInfo->EncryptionStatus = RT802_11Encryption1Enabled;
			pMgntInfo->LEDAssocState = LED_ASSOC_SECURITY_BEGIN;
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(): DOT11_CIPHER_ALGO_WEP %s\n", (pSecInfo->PairwiseEncAlgorithm==RT_ENC_ALG_WEP104)?"WEP104":"WEP40") );
			break;

		case DOT11_CIPHER_ALGO_TKIP:
			pSecInfo->PairwiseEncAlgorithm = RT_ENC_ALG_TKIP;
			pSecInfo->UseDefaultKey = FALSE;
			pSecInfo->EncryptionHeadOverhead = EXT_IV_LEN;
			pSecInfo->EncryptionTailOverhead = WEP_ICV_LEN;
			pSecInfo->EncryptionStatus = RT802_11Encryption2Enabled;
			pMgntInfo->LEDAssocState = LED_ASSOC_SECURITY_BEGIN;
			// If Auth is CCKM, we check Enc Algroithm is TKIP or Aes
			// Marked by Bruce, 2011-02-09.
			// We should not restrct the security level to WPA2 or WPA.
			//if(pSecInfo->AuthMode == RT_802_11AuthModeCCKM)
			//{
			//	pMgntInfo->SecurityInfo.SecLvl = RT_SEC_LVL_WPA;
			//	RT_TRACE( COMP_OID_SET, DBG_LOUD, (" Change RT_802_11AuthModeCCKM to RT_SEC_LVL_WPA\n", __FUNCTION__) );
			//}
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(): DOT11_CIPHER_ALGO_TKIP\n") );
			break;

		case DOT11_CIPHER_ALGO_CCMP:
			pSecInfo->PairwiseEncAlgorithm = RT_ENC_ALG_AESCCMP;
			pSecInfo->UseDefaultKey = FALSE;
			pSecInfo->AESCCMPMicLen = 8;
			pSecInfo->EncryptionHeadOverhead = EXT_IV_LEN;
			pSecInfo->EncryptionTailOverhead = AES_MIC_LEN;
			pSecInfo->EncryptionStatus = RT802_11Encryption3Enabled;
			pMgntInfo->LEDAssocState = LED_ASSOC_SECURITY_BEGIN;
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(): DOT11_CIPHER_ALGO_CCMP\n") );			
			break;

		case DOT11_CIPHER_ALGO_WAPI_SMS4: //For WAPI IHV service support add  by ylb 20111114
			pSecInfo->PairwiseEncAlgorithm = RT_ENC_ALG_SMS4;
			pSecInfo->GroupEncAlgorithm = RT_ENC_ALG_SMS4;
			pSecInfo->SWTxEncryptFlag  = TRUE;
        		pSecInfo->SWRxDecryptFlag = TRUE;
			pSecInfo->UseDefaultKey = TRUE; //for hw en/decryption
			pSecInfo->EncryptionHeadOverhead = WAPI_EXT_LEN;
			pSecInfo->EncryptionTailOverhead = SMS4_MIC_LEN;

			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(): DOT11_CIPHER_ALGO_WAPI_SMS4\n") );			
			break;
			
		default:
			pSecInfo->EncryptionStatus = RT802_11WEPDisabled;
			pMgntInfo->LEDAssocState = LED_ASSOC_SECURITY_NONE;
			RT_TRACE( COMP_OID_SET, DBG_WARNING, ("N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(): Unknown AlgorithmId 0x%X\n", AlgorithmId ) );
			break;
	}

	//vivi modify for new cam search flow, 20091028. 
	//new cam search flow can use hw en/decryption, while original can not.
	if (IS_EN_DE_CRYPTION_NEW_CAM_SUPPORT())
	{
		//we only use SWTxEncryptFlag&SWRxDecryptFlag to decide hw or sw en/decryption

		//temply add this function here, reconsider it later, vivi
		SecSetSwEncryptionDecryption( Adapter , FALSE , FALSE );

		// Please consider two-client scenario: One is no security, and the other needs HW security. (Do not disable HW security)
		if(!pSecInfo->SWTxEncryptFlag || !pSecInfo->SWRxDecryptFlag)
		{
			Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);
		}
		else
		{
			if (IsMultiPortAllowDisableHWSecurity(Adapter))
			Adapter->HalFunc.DisableHWSecCfgHandler(Adapter);
		}
	}
	else
	{
		// Switch HW encryption/decryption mode.
		if(Adapter->bInHctTest == 1 || Adapter->MgntInfo.SafeModeEnabled)
		{
			Adapter->HalFunc.DisableHWSecCfgHandler(Adapter);
			SecSetSwEncryptionDecryption( Adapter , TRUE , TRUE );
		}
		else
		{
			//pSecInfo->RegSWRxDecryptFlag = EncryptionDecryptionMechanism_Auto;
			//pSecInfo->RegSWTxEncryptFlag = EncryptionDecryptionMechanism_Auto;
			if( pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC &&
			     MgntActQuery_ApType(Adapter) == RT_AP_TYPE_NONE )
			{
				if(pMgntInfo->bRegAdhocUseHWSec)
				{
					RT_TRACE(COMP_SEC , DBG_LOUD , ("==>N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM() Win7 we used HW in AD-HOT in FPGA Verify\n"));
					SecSetSwEncryptionDecryption( Adapter , FALSE , FALSE );
				}
				else
				{
					RT_TRACE(COMP_SEC , DBG_LOUD , ("==>N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM() Win7 we used SW in AD-HOT \n"));
					SecSetSwEncryptionDecryption( Adapter , TRUE , TRUE );
				}
			}
			else if( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP104 ||
				     pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40)
			{
				RT_TRACE(COMP_SEC , DBG_LOUD , ("==>Win7 we used SW in WEP \n"));
				SecSetSwEncryptionDecryption( Adapter , TRUE , TRUE );
			}
			else
			{
				RT_TRACE(COMP_SEC , DBG_LOUD , ("==>Win7 we used HW in default !! \n"));
				SecSetSwEncryptionDecryption( Adapter , FALSE , FALSE );
			}
				
			// Please consider two-client scenario: One is no security, and the other needs HW security. (Do not disable HW security)
			if(!pSecInfo->SWTxEncryptFlag || !pSecInfo->SWRxDecryptFlag)
			{
	 			Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);
			}
			else
			{
			//should consider multi-port scenario:default adapter is SW encryption/decryption ,vwifi use HW.
			// start vwifi first,then default adapter connect to a ap,the SecCfg will disable HW encryption/decryption,
			// and will cause vwifi not use encryption.
			// so need to check if need to disable HW encryption/decryption.    
				if (IsMultiPortAllowDisableHWSecurity(Adapter) )
				Adapter->HalFunc.DisableHWSecCfgHandler(Adapter);
			}
		}
	}

	// Update mCap.
	if( pSecInfo->PairwiseEncAlgorithm != RT_ENC_ALG_NO_CIPHER )
	{
		pMgntInfo->mCap |= cPrivacy;
	}
	else
	{
		pMgntInfo->mCap &= ~cPrivacy;
	}

	// Update STA's RSNIE.
	SecConstructRSNIE(Adapter);
}

//
// Implementation of OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM setting for NDIS6.
// Added by Annie, 2006-10-10.
// (reference Sta11SetEnabledUnicastCipherAlgorithm() in MS's code)
//
NDIS_STATUS
N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	PDOT11_CIPHER_ALGORITHM_LIST	pCipherAlgoList = (PDOT11_CIPHER_ALGORITHM_LIST)InformationBuffer;

	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("==> N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM()\n") );

	*BytesNeeded = sizeof(DOT11_CIPHER_ALGORITHM_LIST);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}
	
	if (!N6_VERIFY_OBJECT_HEADER_DEFAULT(
					pCipherAlgoList->Header, 
					NDIS_OBJECT_TYPE_DEFAULT,
					DOT11_CIPHER_ALGORITHM_LIST_REVISION_1,
					sizeof(DOT11_CIPHER_ALGORITHM_LIST)) )
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(), NDIS_STATUS_INVALID_DATA\n") );
		return NDIS_STATUS_INVALID_DATA;
	}

	// Must have atleast one entry in the list
	if (pCipherAlgoList->uNumOfEntries < 1)
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(), NDIS_STATUS_INVALID_DATA\n") );
		return NDIS_STATUS_INVALID_DATA;
	}

	// Only support one cipher algorithm
	if (pCipherAlgoList->uNumOfEntries != 1)
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(), NDIS_STATUS_INVALID_LENGTH\n") );
		return NDIS_STATUS_INVALID_LENGTH;
	}

	N6CSet_DOT11_UNICAST_CIPHER_ALGORITHM( Adapter ,pCipherAlgoList->AlgorithmIds[0] );

	*BytesRead = *BytesNeeded;
	
	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("<== N6CSet_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM(): PairwiseEncAlgorithm=0x%X\n", pSecInfo->PairwiseEncAlgorithm) );
	return NDIS_STATUS_SUCCESS;
}


//N6CSet_DOT11_MULTICAST_CIPHER_ALGORITHM
//
//
VOID
N6CSet_DOT11_MULTICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	IN 	DOT11_CIPHER_ALGORITHM  		AlgorithmId
)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;

	if(FALSE == WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPIIHVSUPPORT, 0))
	{
		if(WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPIENABLE, 0))
			return;		
	}

	FunctionIn(COMP_SEC);

	pSecInfo->GroupEncAlgorithm = N6CDot11ToEncAlgorithm( 
									AlgorithmId, 
									(pSecInfo->PairwiseEncAlgorithm==RT_ENC_ALG_WEP104)?NATIVE_802_11_WEP104_KEY_LENGTH:NATIVE_802_11_WEP40_KEY_LENGTH
									);
	// Save Multicast Alg
	pNdisCommon->RegGroupALg = AlgorithmId;

	SecConstructRSNIE(Adapter);
}

//
// Implementation of OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM setting for NDIS6.
// Added by Annie, 2006-10-10.
// (reference Sta11SetEnabledMulticastCipherAlgorithm() in MS's code)
//
NDIS_STATUS
N6CSet_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	PDOT11_CIPHER_ALGORITHM_LIST	pCipherAlgoList = (PDOT11_CIPHER_ALGORITHM_LIST)InformationBuffer;

	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("==> N6CSet_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM()\n") );

	if (!N6_VERIFY_OBJECT_HEADER_DEFAULT(
					pCipherAlgoList->Header, 
					NDIS_OBJECT_TYPE_DEFAULT,
					DOT11_CIPHER_ALGORITHM_LIST_REVISION_1,
					sizeof(DOT11_CIPHER_ALGORITHM_LIST)) )
	{
		return NDIS_STATUS_INVALID_DATA;
	}

	// Must have atleast one entry in the list
	if (pCipherAlgoList->uNumOfEntries < 1)
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM(), NDIS_STATUS_INVALID_DATA\n") );
		return NDIS_STATUS_INVALID_DATA;
	}

	// Only support one cipher algorithm
	if (pCipherAlgoList->uNumOfEntries != 1)
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM(), NDIS_STATUS_INVALID_LENGTH\n") );
		return NDIS_STATUS_INVALID_LENGTH;
	}

	N6CSet_DOT11_MULTICAST_CIPHER_ALGORITHM( Adapter,pCipherAlgoList->AlgorithmIds[0]);

	*BytesRead = *BytesNeeded;
	
	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("<== N6CSet_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM(): GroupEncAlgorithm=0x%X\n", pSecInfo->GroupEncAlgorithm) );
	return NDIS_STATUS_SUCCESS;
}

//
//	Description:
//		ADD, MODIFY, or DELETE an entry of 
//		default key (i.e. WEP Key, group key) or 
//		per-station default key (i.e. IBSS RSNA per-station group key).
//
NDIS_STATUS
N6CSet_DOT11_CIPHER_DEFAULT_KEY(
	IN	PADAPTER							Adapter,
	OUT	PVOID								InformationBuffer,
	IN	ULONG								InformationBufferLength,
	OUT	PULONG								BytesRead,
	OUT	PULONG								BytesNeeded
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
	PDOT11_CIPHER_DEFAULT_KEY_VALUE	pDefaultKey = (PDOT11_CIPHER_DEFAULT_KEY_VALUE)InformationBuffer;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u4Byte	KeyIndex = 0;
	u1Byte	MacAddress[ETHERNET_ADDRESS_LENGTH] = {0x00,0x00,0x00,0x00,0x00,0x00};
	RT_ENC_ALG rtEncAlgo = RT_ENC_ALG_NO_CIPHER;
	pu1Byte	pKeyMaterial = NULL;
	u4Byte	KeyLen = 0;
	u8Byte	KeyRSC = 0;
	BOOLEAN	bValidKeyRSC = FALSE;

	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("==> N6CSet_DOT11_CIPHER_DEFAULT_KEY()\n") );

	if(WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPIENABLE, 0))
		return NDIS_STATUS_SUCCESS;

	//
	// Check if InformationBuffer contains a valid DOT11_CIPHER_DEFAULT_KEY_VALUE object.
	//
	*BytesNeeded = FIELD_OFFSET(DOT11_CIPHER_DEFAULT_KEY_VALUE, ucKey);
	if ( InformationBufferLength < *BytesNeeded )
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_DEFAULT_KEY(), NDIS_STATUS_INVALID_LENGTH: %d\n", InformationBufferLength) );
		return NDIS_STATUS_BUFFER_OVERFLOW;	
	}
	
	if (!N6_VERIFY_OBJECT_HEADER_DEFAULT(
					pDefaultKey->Header, 
					NDIS_OBJECT_TYPE_DEFAULT,
					DOT11_CIPHER_DEFAULT_KEY_VALUE_REVISION_1,
					sizeof(DOT11_CIPHER_DEFAULT_KEY_VALUE)) )
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_DEFAULT_KEY(), NDIS_STATUS_INVALID_DATA\n") );
		return NDIS_STATUS_INVALID_DATA;
	}

	*BytesNeeded = FIELD_OFFSET(DOT11_CIPHER_DEFAULT_KEY_VALUE, ucKey) + 
					pDefaultKey->usKeyLength;
	if (InformationBufferLength < *BytesNeeded)
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_DEFAULT_KEY(), NDIS_STATUS_BUFFER_OVERFLOW: %d, BytesNeeded: %d\n", InformationBufferLength, *BytesNeeded) );
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	//
	// For debug purpose. Annie, 2006-10-19.
	//
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("====================================\n") );
	RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, "DOT11_CIPHER_DEFAULT_KEY_VALUE\n", pDefaultKey, InformationBufferLength );
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("-------------------------------------\n") );
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("uKeyIndex=0x%X\n", pDefaultKey->uKeyIndex) );
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("AlgorithmId=0x%X\n", pDefaultKey->AlgorithmId) );
	RT_PRINT_ADDR( COMP_OID_SET, DBG_LOUD, "MacAddr", pDefaultKey->MacAddr );
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("bDelete=0x%X\n", pDefaultKey->bDelete ) );
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("bStatic=0x%X\n", pDefaultKey->bStatic ) );
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("usKeyLength=0x%X\n", pDefaultKey->usKeyLength ) );
	RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, "ucKey\n", pDefaultKey->ucKey, pDefaultKey->usKeyLength);
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("====================================\n") );

	//
	// Collect information in pDefaultKey and those embedded in pDefaultKey->ucKey.
	//
	
	KeyIndex = pDefaultKey->uKeyIndex;

	if(	pMgntInfo->OpMode == RT_OP_MODE_IBSS && pSecInfo->SecLvl == RT_SEC_LVL_WPA2 &&
		!eqMacAddr(pDefaultKey->MacAddr, MacAddress) && !MacAddr_isMulticast(pDefaultKey->MacAddr) )
	{
		//
		// 061028, rcnjko: 
		// WDK said: 
		// - We shall don't care MacAddr in infrastructure mode.
		// - In an IBSS with RSNA AES-CCMP, we use TA and index to search the key 
		// to decrypt mcst/bcst frame received. 
		//
		// So, a unicast MacAddr is used to search per-station default keys in 
		// RSNA IBSS case and can be ignore in other cases.
		//
		cpMacAddr(MacAddress, pDefaultKey->MacAddr);
	}

	if(pDefaultKey->bDelete)
	{ // Remove a key
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_DEFAULT_KEY(): delete Key\n"));

		//
		// 061028, rcnjko: WDK said we shall ignore AlgorithmId if bDelete is set, 
		// so we use GroupEncAlgorithm instead. 
		//
		rtEncAlgo = pSecInfo->GroupEncAlgorithm;

		rtEncAlgo = pSecInfo->GroupEncAlgorithm;
		if( KeyIndex > 3 && KeyIndex < 6 )
		{  // 802.11w BIP key Key index 5 or 6
		
		    PlatformZeroMemory(pSecInfo->BIPKeyBuffer, MAX_KEY_LEN);
		}
		else if( !pMgntInfo->bRSNAPSKMode)
		{
			MgntActSet_802_11_REMOVE_KEY(
				Adapter, 
				rtEncAlgo,
				KeyIndex,
				MacAddress,
				TRUE); // IsGroup.
		}
		else
		{
			if(!MgntActSet_RSNA_REMOVE_DEAULT_KEY(
					Adapter,
					KeyIndex,
					MacAddress))
			{
				RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_DEFAULT_KEY(), MgntActSet_RSNA_REMOVE_DEAULT_KEY return FALSE !!!\n"));
				return NDIS_STATUS_INVALID_DATA;
			}
		}
		
	}
	else
	{ // Add or Modify a key.

		switch(pDefaultKey->AlgorithmId) 
		{
		case DOT11_CIPHER_ALGO_WEP:
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_DEFAULT_KEY(): Set a WEP Key\n"));
				rtEncAlgo = N6CDot11ToEncAlgorithm(pDefaultKey->AlgorithmId, pDefaultKey->usKeyLength);
				pKeyMaterial  = pDefaultKey->ucKey;
				KeyLen = pDefaultKey->usKeyLength;
				switch(rtEncAlgo)
				{
					case RT_ENC_ALG_WEP40:
						pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_WEP64;
						pNdisCommon->RegEncAlgorithm = REG_WEP_STATUS_WEP64;
						pNdisCommon->RegPairwiseAlg = DOT11_CIPHER_ALGO_WEP40;
						pNdisCommon->RegRSNAKeyID = KeyIndex;				
						// Save for Reconnent for Ad-hot ,by CCW
						PlatformMoveMemory(pNdisCommon->RegDefaultKeyBuf[KeyIndex], pKeyMaterial, KeyLen);
						pNdisCommon->RegDefaultKey[KeyIndex].Length  = (u2Byte)KeyLen;
						break;
					case RT_ENC_ALG_WEP104:
						pNdisCommon->RegWepEncStatus = REG_WEP_STATUS_WEP128;
						pNdisCommon->RegEncAlgorithm = REG_WEP_STATUS_WEP128;
						pNdisCommon->RegPairwiseAlg = DOT11_CIPHER_ALGO_WEP104;
						pNdisCommon->RegRSNAKeyID = KeyIndex;	
						// Save for Reconnent for Ad-hot ,by CCW
						PlatformMoveMemory( pNdisCommon->RegDefaultKeyWBuf[KeyIndex], pKeyMaterial, KeyLen);
						pNdisCommon->RegDefaultKeyW[KeyIndex].Length  = (u2Byte)KeyLen;
						break;
					default:
						break;
				}
			}
			break;

		case DOT11_CIPHER_ALGO_WEP40:
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_DEFAULT_KEY(): Set a WEP40 Key\n"));
				rtEncAlgo = RT_ENC_ALG_WEP40;
				pKeyMaterial = pDefaultKey->ucKey;
				KeyLen = pDefaultKey->usKeyLength;
				// Save for Reconnent for Ad-hot ,by CCW
				PlatformMoveMemory(pNdisCommon->RegDefaultKeyBuf[KeyIndex], pKeyMaterial, KeyLen);
				pNdisCommon->RegDefaultKey[KeyIndex].Length  = (u2Byte)KeyLen;
			}
			break;

		case DOT11_CIPHER_ALGO_WEP104:
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_DEFAULT_KEY(): Set a WEP104 Key\n"));
				rtEncAlgo = RT_ENC_ALG_WEP104;
				pKeyMaterial = pDefaultKey->ucKey;
				KeyLen = pDefaultKey->usKeyLength;
				// Save for Reconnent for Ad-hot ,by CCW
				PlatformMoveMemory( pNdisCommon->RegDefaultKeyWBuf[KeyIndex], pKeyMaterial, KeyLen);
				pNdisCommon->RegDefaultKeyW[KeyIndex].Length  = (u2Byte)KeyLen;
			}
			break;
			
		case DOT11_CIPHER_ALGO_TKIP:
			{
				PDOT11_KEY_ALGO_TKIP_MIC pTkipMicKey = (PDOT11_KEY_ALGO_TKIP_MIC)pDefaultKey->ucKey;
				//pNdisCommon->RegPairwiseAlg = DOT11_CIPHER_ALGO_TKIP;
				pNdisCommon->RegRSNAKeyID = KeyIndex;	
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_DEFAULT_KEY(): Set a TKIP Key\n"));
				rtEncAlgo = RT_ENC_ALG_TKIP;
				pKeyMaterial = pTkipMicKey->ucTKIPMICKeys; // Tkip Enc Key(16) + STA Rx MIC Key(8) + STA Tx MIC Key(8).
				KeyLen = pTkipMicKey->ulTKIPKeyLength + pTkipMicKey->ulMICKeyLength; 
				PlatformMoveMemory(&KeyRSC, pTkipMicKey->ucIV48Counter, 6);
				bValidKeyRSC = TRUE;
			}
			break;

		case DOT11_CIPHER_ALGO_CCMP:
			{
				PDOT11_KEY_ALGO_CCMP pCcmpKey = (PDOT11_KEY_ALGO_CCMP)pDefaultKey->ucKey;

				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_DEFAULT_KEY(): Set an AES-CCMP Key\n"));
				rtEncAlgo = RT_ENC_ALG_AESCCMP;
				pKeyMaterial = pCcmpKey->ucCCMPKey;
				KeyLen = pCcmpKey->ulCCMPKeyLength;
				PlatformMoveMemory(&KeyRSC, pCcmpKey->ucIV48Counter, 6);
				bValidKeyRSC = TRUE;
				if( MacAddress[0] == 0 &&  MacAddress[1] == 0 &&
	   		     	    MacAddress[2] == 0 &&  MacAddress[3] == 0 &&
	    		    	    MacAddress[4] == 0 &&  MacAddress[5] == 0  &&
	    		    	    pMgntInfo->bRSNAPSKMode
	    		    	)
				{
					pNdisCommon->RegRSNADefaultkey.Octet = pNdisCommon->RegRSNADefaultkeybuf;
					PlatformMoveMemory(pNdisCommon->RegRSNADefaultkeybuf, pKeyMaterial, KeyLen);
					pNdisCommon->RegRSNADefaultkey.Length = (u2Byte)KeyLen;
					pNdisCommon->RegRSNAKeyID = KeyIndex;
				}
			
			}
			break;

		case DOT11_CIPHER_ALGO_BIP :
			{
				PDOT11_KEY_ALGO_BIP		pBIPkey = (PDOT11_KEY_ALGO_BIP)pDefaultKey->ucKey;

				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_DEFAULT_KEY(): Set an BIP Key\n"));

				// NOTE: Only for Build Pass
				
				PlatformMoveMemory( pSecInfo->IPN ,pBIPkey->ucIPN, 6 );

				if( pBIPkey->ulBIPKeyLength == 16  )
				{
					PlatformMoveMemory( pSecInfo->BIPKeyBuffer ,pBIPkey->ucBIPKey, 16 );
					return NDIS_STATUS_SUCCESS;
				}
				else
				{
					return NDIS_STATUS_INVALID_DATA;
				}
				
				break;
			}

		default:
			RT_TRACE( COMP_OID_SET, DBG_WARNING, ("N6CSet_DOT11_CIPHER_DEFAULT_KEY: Warning, Set key of unknown algorithm: %#X\n", pDefaultKey->AlgorithmId));
			return NDIS_STATUS_INVALID_DATA;
			break;
		}
		if( ACTING_AS_AP(Adapter))
		{
			PAUTH_GLOBAL_KEY_TAG	pGlInfo = &(pMgntInfo->globalKeyInfo);
			u1Byte					CAM_CONST_BROAD[6]  = {0xff ,0xff ,0xff ,0xff ,0xff ,0xff };
			
			if(  MacAddress[0] == 0 &&  MacAddress[1] == 0 &&
	   		     MacAddress[2] == 0 &&  MacAddress[3] == 0 &&
	    		     MacAddress[4] == 0 &&  MacAddress[5] == 0   )
			{
				PlatformZeroMemory(pMgntInfo->globalKeyInfo.GTK , GTK_LEN );
				PlatformMoveMemory(pMgntInfo->globalKeyInfo.GTK , pKeyMaterial , KeyLen );

				if( pDefaultKey->AlgorithmId ==  DOT11_CIPHER_ALGO_TKIP )
				{
					pGlInfo->TxMICKey = pGlInfo->GTK + GTK_MIC_TX_POS;
					pGlInfo->RxMICKey = pGlInfo->GTK + GTK_MIC_RX_POS;
					pMgntInfo->SecurityInfo.GroupTransmitKeyIdx = (u1Byte)KeyIndex;
					// Set For Hw encrypt !!
					/*
					AP_Setkey(  Adapter , 
			     					CAM_CONST_BROAD,
			     					1,  // Index entry
			     					CAM_TKIP,
			     					1,  // Set Group Key
			     					pGlInfo->GTK);
			     					*/
				}
				else if( pDefaultKey->AlgorithmId ==  DOT11_CIPHER_ALGO_CCMP )
				{
					AESCCMP_BLOCK		blockKey;
					
					PlatformMoveMemory( blockKey.x , pGlInfo->GTK , 16);
					pMgntInfo->SecurityInfo.GroupTransmitKeyIdx = (u1Byte)KeyIndex;
					AES_SetKey(blockKey.x, AESCCMP_BLK_SIZE*8, (pu4Byte)pGlInfo->AESGTK);
					// Set For Hw encrypt !!
					/*
					AP_Setkey(  Adapter , 
			     					CAM_CONST_BROAD,
			     					1,  // Index entry
			     					CAM_AES,
			     					1,  // Set Group Key
			     					pGlInfo->GTK);
			     		*/
				}
				
			}
		}
		else if( !pMgntInfo->bRSNAPSKMode )
		{

			pSecInfo->GroupTransmitKeyIdx = (u1Byte)KeyIndex; //by tynli. Suggested by CCW.

			RT_TRACE( COMP_SEC ,DBG_LOUD,( "====> pSecInfo->GroupTransmitKeyIdx  = %d\n ",KeyIndex));
			MgntActSet_802_11_ADD_KEY(
				Adapter,
				rtEncAlgo,
				( KeyIndex | ((bValidKeyRSC) ? ADD_KEY_IDX_RSC: 0) ),
				KeyLen,
				pKeyMaterial,
				MacAddress, // Only be a valid unicast address in RSNA IBSS case.
				FALSE, // IsGroupTransmitKey, since DefaultTransmitKeyIdx and GroupTransmitKeyIdx is determined by OID_DOT11_CIPHER_DEFAULT_KEY_ID, we just set it to FALSE.
				TRUE, // IsGroup.
				KeyRSC);
				
			PlatformRequestPreAuthentication( Adapter, PRE_AUTH_INDICATION_REASON_ASSOCIATION );
		}
		else{
			//vivi added for new cam search flow, 20091028
			//vivi redefine this function to add the parameter rtEncAlgo and Isgroup
			if( !MgntActSet_RSNA_ADD_DEAULT_KEY(
					Adapter,
					rtEncAlgo,
					KeyIndex,
					KeyLen,
					pKeyMaterial,
					MacAddress) )
			{ // Return NDIS_STATUS_RESOURCES because out of free entry.

				RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_DEFAULT_KEY(), MgntActSet_RSNA_ADD_DEAULT_KEY return FALSE !!!\n"));
				return NDIS_STATUS_RESOURCES;
			}
		}
	
	}

	*BytesRead = *BytesNeeded;

	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("<== N6CSet_DOT11_CIPHER_DEFAULT_KEY()\n") );
	return NDIS_STATUS_SUCCESS;
}


//
//	Description:
//		ADD, MODIFY, or DELETE ONE or MORE entries in key-mapping key table. 
//
NDIS_STATUS
N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
	PDOT11_BYTE_ARRAY	pByteArray = (PDOT11_BYTE_ARRAY)InformationBuffer;
	PDOT11_CIPHER_KEY_MAPPING_KEY_VALUE	pKeyMappingKey = NULL;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;

	u1Byte	MacAddress[ETHERNET_ADDRESS_LENGTH] = {0x00,0x00,0x00,0x00,0x00,0x00};
	RT_ENC_ALG	rtEncAlgo = RT_ENC_ALG_NO_CIPHER;
	pu1Byte	pKeyMaterial = NULL;
	u4Byte	KeyLen = 0;
	u8Byte	KeyRSC = 0;
	BOOLEAN	bValidKeyRSC = FALSE;

	u4Byte	i = 0;
	u4Byte	nBytesParsed = 0;
	BOOLEAN	bInvalidKey = FALSE;

	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("==> N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY()\n") );

	//
	// Check if InformationBuffer contains a valid DOT11_BYTE_ARRAY object.
	//
	*BytesNeeded = sizeof(DOT11_BYTE_ARRAY);
	if ( InformationBufferLength < *BytesNeeded )
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(), NDIS_STATUS_INVALID_LENGTH: %d\n", InformationBufferLength) );
		return NDIS_STATUS_BUFFER_OVERFLOW;	
	}
	
	if (!N6_VERIFY_OBJECT_HEADER_DEFAULT(
					pByteArray->Header, 
					NDIS_OBJECT_TYPE_DEFAULT,
					DOT11_CIPHER_KEY_MAPPING_KEY_VALUE_BYTE_ARRAY_REVISION_1,
					sizeof(DOT11_BYTE_ARRAY)) )
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(), NDIS_STATUS_INVALID_DATA\n") );
		return NDIS_STATUS_INVALID_DATA;
	}

	*BytesNeeded = FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer) + 
					pByteArray->uNumOfBytes;
	if (InformationBufferLength < *BytesNeeded)
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(), NDIS_STATUS_BUFFER_OVERFLOW: %d, BytesNeeded: %d\n", InformationBufferLength, *BytesNeeded) );
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	//
	// Enumerate each entry in ucBuffer array.
	//
	for(i = 0; nBytesParsed < pByteArray->uNumOfBytes; i++)
	{
		pKeyMappingKey = (PDOT11_CIPHER_KEY_MAPPING_KEY_VALUE)(pByteArray->ucBuffer + nBytesParsed);
		nBytesParsed += (FIELD_OFFSET(DOT11_CIPHER_KEY_MAPPING_KEY_VALUE, ucKey) + pKeyMappingKey->usKeyLength);
		bInvalidKey = FALSE;

		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("====================================\n") );
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("DOT11_CIPHER_KEY_MAPPING_KEY: %d\n", i) );
		RT_PRINT_ADDR( COMP_OID_SET, DBG_LOUD, "PeerMacAddr", pKeyMappingKey->PeerMacAddr );
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("AlgorithmId=0x%X\n", pKeyMappingKey->AlgorithmId) );
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Direction=0x%X\n", pKeyMappingKey->Direction) );
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("bDelete=0x%X\n", pKeyMappingKey->bDelete ) );
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("bStatic=0x%X\n", pKeyMappingKey->bStatic ) );
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("usKeyLength=0x%X\n", pKeyMappingKey->usKeyLength ) );
		RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, "ucKey", pKeyMappingKey->ucKey, pKeyMappingKey->usKeyLength);
		RT_TRACE( COMP_OID_SET, DBG_LOUD, ("====================================\n") );


		RT_TRACE(COMP_CCX, DBG_LOUD, ("====================================\n") );
		RT_TRACE( COMP_CCX, DBG_LOUD, ("DOT11_CIPHER_KEY_MAPPING_KEY: %d\n", i) );
		RT_PRINT_ADDR(COMP_CCX, DBG_LOUD, "PeerMacAddr", pKeyMappingKey->PeerMacAddr );
		RT_TRACE( COMP_CCX, DBG_LOUD, ("AlgorithmId=0x%X\n", pKeyMappingKey->AlgorithmId) );
		RT_TRACE( COMP_CCX, DBG_LOUD, ("Direction=0x%X\n", pKeyMappingKey->Direction) );
		RT_TRACE( COMP_CCX, DBG_LOUD, ("bDelete=0x%X\n", pKeyMappingKey->bDelete ) );
		RT_TRACE( COMP_CCX, DBG_LOUD, ("bStatic=0x%X\n", pKeyMappingKey->bStatic ) );
		RT_TRACE( COMP_CCX, DBG_LOUD, ("usKeyLength=0x%X\n", pKeyMappingKey->usKeyLength ) );
		RT_PRINT_DATA( COMP_CCX, DBG_LOUD, "ucKey", pKeyMappingKey->ucKey, pKeyMappingKey->usKeyLength);
		RT_TRACE( COMP_CCX, DBG_LOUD, ("====================================\n") );

		cpMacAddr(MacAddress, pKeyMappingKey->PeerMacAddr);

		switch(pKeyMappingKey->AlgorithmId) 
		{
		case DOT11_CIPHER_ALGO_WEP:
			{
				rtEncAlgo = N6CDot11ToEncAlgorithm(pKeyMappingKey->AlgorithmId, pKeyMappingKey->usKeyLength);
				pKeyMaterial = pKeyMappingKey->ucKey;
				KeyLen = pKeyMappingKey->usKeyLength;
			}
			break;

		case DOT11_CIPHER_ALGO_WEP40:
			{
				rtEncAlgo = RT_ENC_ALG_WEP40;
				pKeyMaterial = pKeyMappingKey->ucKey;
				KeyLen = pKeyMappingKey->usKeyLength;
			}
			break;

		case DOT11_CIPHER_ALGO_WEP104:
			{
				rtEncAlgo = RT_ENC_ALG_WEP104;
				pKeyMaterial = pKeyMappingKey->ucKey;	
				KeyLen = pKeyMappingKey->usKeyLength;
			}
			break;			
			
		case DOT11_CIPHER_ALGO_TKIP:
			{
				PDOT11_KEY_ALGO_TKIP_MIC pTkipMicKey = (PDOT11_KEY_ALGO_TKIP_MIC)pKeyMappingKey->ucKey;
			
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(): %s TKIP Key\n", ((pKeyMappingKey->bDelete)? "Delete": "Set" )));
				rtEncAlgo = RT_ENC_ALG_TKIP;
				pKeyMaterial = pTkipMicKey->ucTKIPMICKeys; // Tkip Enc Key(16) + STA Rx MIC Key(8) + STA Tx MIC Key(8).
				KeyLen = pTkipMicKey->ulTKIPKeyLength + pTkipMicKey->ulMICKeyLength; 
				PlatformMoveMemory(&KeyRSC, pTkipMicKey->ucIV48Counter, 6);
				bValidKeyRSC = TRUE;
			}
			break;

		case DOT11_CIPHER_ALGO_CCMP:
			{
				PDOT11_KEY_ALGO_CCMP pCcmpKey = (PDOT11_KEY_ALGO_CCMP)pKeyMappingKey->ucKey;

				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(): %s AES-CCMP Key\n", ((pKeyMappingKey->bDelete)? "Set" : "Delete")));
				rtEncAlgo = RT_ENC_ALG_AESCCMP;
				pKeyMaterial = pCcmpKey->ucCCMPKey;
				KeyLen = pCcmpKey->ulCCMPKeyLength;
				PlatformMoveMemory(&KeyRSC, pCcmpKey->ucIV48Counter, 6);
				bValidKeyRSC = TRUE;
			}
			break;

		case DOT11_CIPHER_ALGO_MFPTKIP :
			{
				PDOT11_KEY_ALGO_TKIP_MIC pTkipMicKey = (PDOT11_KEY_ALGO_TKIP_MIC)pKeyMappingKey->ucKey;
			
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(): %s TKIP MFP Key\n", ((pKeyMappingKey->bDelete)? "Delete": "Set" )));
				rtEncAlgo = RT_ENC_ALG_TKIP;
				pKeyMaterial = pTkipMicKey->ucTKIPMICKeys; // Tkip Enc Key(16) + STA Rx MIC Key(8) + STA Tx MIC Key(8).
				KeyLen = pTkipMicKey->ulTKIPKeyLength + pTkipMicKey->ulMICKeyLength; 
				PlatformMoveMemory(&KeyRSC, pTkipMicKey->ucIV48Counter, 6);
				bValidKeyRSC = TRUE;

				CCX_MFPModeChange(Adapter, TRUE);
				
				// Keep Key to Local buffer ...
				if(KeyLen <= MAX_KEY_LEN)
				{
				}
			}
			break;

		case DOT11_CIPHER_ALGO_MFPCCMP :
			{
				PDOT11_KEY_ALGO_CCMP pCcmpKey = (PDOT11_KEY_ALGO_CCMP)pKeyMappingKey->ucKey;

				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(): %s AES-CCMP MFP Key\n", ((!pKeyMappingKey->bDelete)? "Set" : "Delete")));
				rtEncAlgo = RT_ENC_ALG_AESCCMP;
				pKeyMaterial = pCcmpKey->ucCCMPKey;
				KeyLen = pCcmpKey->ulCCMPKeyLength;
				PlatformMoveMemory(&KeyRSC, pCcmpKey->ucIV48Counter, 6);
				bValidKeyRSC = TRUE;

				CCX_MFPModeChange(Adapter, TRUE);

				// Keep Key to Local buffer ...
				{
				}
			}
			break;

		default:
			RT_TRACE( COMP_OID_SET, DBG_WARNING, ("N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY: Warning, %s key of invalid algorithm: %#X\n", ((pKeyMappingKey->bDelete)? "Set" : "Delete"), pKeyMappingKey->AlgorithmId));
			bInvalidKey = TRUE;
			break;
		}

		if(bInvalidKey)
		{
			continue;
		}

		if(pKeyMappingKey->bDelete)
		{ // Remove a key
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(): delete Key\n"));

			if( MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP )
			{
				RT_TRACE( COMP_MLME , DBG_LOUD , ("====>  We had remove station on dissac !! \n") );
			}
			else if( !pMgntInfo->bRSNAPSKMode )
			{
				MgntActSet_802_11_REMOVE_KEY(
					Adapter, 
					rtEncAlgo,
					0, // KeyIndex,
					MacAddress,
					FALSE); // IsGroup
			}
			else{
				if(!MgntActSet_RSNA_REMOVE_MAPPING_KEY(
						Adapter,
		 				MacAddress))
				{
					RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY(), MgntActSet_RSNA_REMOVE_MAPPING_KEY return FALSE!!!\n") );
	 				return NDIS_STATUS_INVALID_DATA;
				}
			}
		}
		else
		{ // Add or Modify a key.

			if( ACTING_AS_AP(Adapter))
			{
				PRT_WLAN_STA pEntry = AsocEntry_GetEntry(pMgntInfo , MacAddress );

				if( pEntry == NULL )
				{
					return  NDIS_STATUS_INVALID_DATA; 
				}
				else
				{
					u4Byte  ucIndex = 0;
					CopyMem(pEntry->perSTAKeyInfo.PTK, pKeyMaterial , KeyLen );	// Added by Annie, 2005-07-12.
					if( pKeyMappingKey->AlgorithmId == DOT11_CIPHER_ALGO_TKIP )
					{
						pEntry->perSTAKeyInfo.TempEncKey = pEntry->perSTAKeyInfo.PTK+TKIP_ENC_KEY_POS;
						pEntry->perSTAKeyInfo.TxMICKey = pEntry->perSTAKeyInfo.PTK+(TKIP_MIC_KEY_POS);	
						pEntry->perSTAKeyInfo.RxMICKey = pEntry->perSTAKeyInfo.PTK+(TKIP_MIC_KEY_POS+TKIP_MIC_KEY_LEN);

						//Add for AP mode HW enc,by CCW		
						ucIndex = AP_FindFreeEntry(Adapter , pEntry->MacAddr);
						if(ucIndex == Adapter->TotalCamEntry)
						{
							RT_TRACE( COMP_OID_SET, DBG_WARNING, ("[Warning]:N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY: Cam Entry is FULL!!!\n"));
							return NDIS_STATUS_INVALID_DATA;
						}

						//set key
						AP_Setkey(  Adapter , 
								      pEntry->perSTAKeyInfo.pWLanSTA->MacAddr,
								      ucIndex,  // Entey  index 
								      CAM_TKIP,
								      0,  // Parise key 
								      pEntry->perSTAKeyInfo.TempEncKey);	

						pEntry->keyindex  = ucIndex;
					}
					else if ( pKeyMappingKey->AlgorithmId == DOT11_CIPHER_ALGO_CCMP )
					{  // AES mode AP-WPA AES,CCW
					
						AESCCMP_BLOCK		blockKey;
						PAUTH_PKEY_MGNT_TAG	pKeyMgnt = &pEntry->perSTAKeyInfo;
						RT_TRACE( COMP_SEC , DBG_LOUD, ("====>CCMP Set Station Key.\n"));
						RT_PRINT_ADDR(COMP_SEC , DBG_LOUD, " ====> Peer Address :  ", MacAddress );
						//Add for AP mode HW enc,by CCW		
						ucIndex = AP_FindFreeEntry(Adapter , pEntry->MacAddr);
						if(ucIndex == Adapter->TotalCamEntry)
						{
							RT_TRACE( COMP_OID_SET, DBG_WARNING, ("[Warning]:N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY: Cam Entry is FULL!!!\n"));
							return NDIS_STATUS_INVALID_DATA;
						}
						
						//Set Key 
						PlatformMoveMemory( blockKey.x , pEntry->perSTAKeyInfo.PTK  , 16);
						AES_SetKey(blockKey.x, AESCCMP_BLK_SIZE*8, (pu4Byte)pEntry->perSTAKeyInfo.AESKeyBuf);
						//set hw key
						AP_Setkey(  Adapter , 
								      pEntry->perSTAKeyInfo.pWLanSTA->MacAddr,
								      ucIndex,  // Entey  index 
								      CAM_AES,
								      0,  // Parise key 
								     pEntry->perSTAKeyInfo.PTK);	
						pEntry->keyindex  = ucIndex;
					}
		
				}
			}
			else if( !pMgntInfo->bRSNAPSKMode ){
				if( !(rtEncAlgo == RT_ENC_ALG_WEP40 || rtEncAlgo == RT_ENC_ALG_WEP104 )  ){
					MgntActSet_802_11_ADD_KEY(
						Adapter,
						rtEncAlgo,
						(PAIRWISE_KEYIDX | ((bValidKeyRSC) ? ADD_KEY_IDX_RSC: 0) ),
						KeyLen,
						pKeyMaterial,
						MacAddress,
						FALSE, // IsGroupTransmitKey.
						FALSE, // IsGroup.
						KeyRSC);
				}
				else{
					for(i=0;i<4;i++){
						// Save for Reconnent for Ad-hot ,by CCW
						PlatformMoveMemory( pNdisCommon->RegDefaultKeyBuf[i], pKeyMaterial, KeyLen);
						pNdisCommon->RegDefaultKey[i].Length  = (u2Byte)KeyLen;
						MgntActSet_802_11_ADD_KEY(
							Adapter,
							rtEncAlgo,
							i,
							KeyLen,
							pKeyMaterial,
							MacAddress,
							FALSE, // IsGroupTransmitKey.
							FALSE, // IsGroup.
							KeyRSC);
					}
				}
				PlatformRequestPreAuthentication( Adapter, PRE_AUTH_INDICATION_REASON_ASSOCIATION );
				
			}
			else{
//vivi added for new cam search flow, 20091028......to find entry 4~31, for uni&broad&multicast
				if(!MgntActSet_RSNA_ADD_MAPPING_KEY(
						Adapter,
						rtEncAlgo,
						0, //pairwised keyindex always be 0
						KeyLen,
						pKeyMaterial,
		 				MacAddress))
				{ 
					RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY, MgntActSet_RSNA_ADD_MAPPING_KEY() return FALSE!!!\n") );
					return NDIS_STATUS_RESOURCES;
				}
			}
		}
	}

	*BytesRead = *BytesNeeded;
	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("<== N6CSet_DOT11_CIPHER_KEY_MAPPING_KEY()\n") );
	return NDIS_STATUS_SUCCESS;
}


//
// Implementation of OID_DOT11_PMKID_LIST setting for NDIS6.
// Added by Annie, 2006-10-13.
// (reference MpSetPMKIDList() and Sta11SetPMKIDList() in MS's code)
//
NDIS_STATUS
N6CSet_DOT11_PMKID_LIST(
	IN	PADAPTER						Adapter,
	IN	PDOT11_PMKID_LIST				pPMKIDList,
	IN	ULONG							InfoBufLength
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	ULONG				BytesNeeded;
	//ULONG				index1, index2;
	u4Byte				ulIndex, i, j, count;
	BOOLEAN				blInserted;
	PDOT11_PMKID_ENTRY	pBssidInfo;

	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==> N6CSet_DOT11_PMKID_LIST()\n") );

	if (!N6_VERIFY_OBJECT_HEADER_DEFAULT(
					pPMKIDList->Header, 
					NDIS_OBJECT_TYPE_DEFAULT,
					DOT11_PMKID_LIST_REVISION_1,
					sizeof(DOT11_PMKID_LIST)) )
	{
		return NDIS_STATUS_INVALID_DATA;
	}

	// Must have atleast one entry in the list
	if (pPMKIDList->uNumOfEntries < 1)
	{
		return NDIS_STATUS_INVALID_DATA;
	}

	// Verify length/number of entries match up
	BytesNeeded = pPMKIDList->uNumOfEntries * sizeof(DOT11_PMKID_ENTRY) +
					FIELD_OFFSET(DOT11_PMKID_LIST, PMKIDs);

	if (InfoBufLength < BytesNeeded)
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	//
	// If the list is too long or too short, simply return error.
	//
	if (pPMKIDList->uNumOfEntries > NUM_PMKID_CACHE || pPMKIDList->uNumOfEntries < 1)
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}


	//
	// Copy the PMKID list. 	// TODO: Re-use SecSetPMKID().
	//
	

	// 1. Clear the entry with different SSID from the AP we are associating with.

	for( ulIndex=0; ulIndex<NUM_PMKID_CACHE; ulIndex++ )
	{
		if(	!eqNByte(pMgntInfo->Ssid.Octet, pSecInfo->PMKIDList[ulIndex].SsidBuf, pMgntInfo->Ssid.Length) ||
			(pMgntInfo->Ssid.Length !=  pSecInfo->PMKIDList[ulIndex].Ssid.Length) )
		{ // SSID is not matched => Clear the entry!
			pSecInfo->PMKIDList[ulIndex].bUsed = FALSE;
		}
	}

	
	// 2. Insert or cover with new PMKID.
	
	pBssidInfo = pPMKIDList->PMKIDs;	// pointer to the first DOT11_PMKID_ENTRY.
	for( i=0; i<pPMKIDList->uNumOfEntries; i++ )
	{
		if( i >= NUM_PMKID_CACHE )
		{
			RT_TRACE( COMP_SEC, DBG_WARNING, ("SecSetPMKID(): uNumOfEntries is more than NUM_PMKID_CACHE!%d\n", pPMKIDList->uNumOfEntries) );
			break;
		}

		blInserted = FALSE;
		for(j=0 ; j<NUM_PMKID_CACHE; j++)
		{
			if( pSecInfo->PMKIDList[j].bUsed && eqMacAddr(pSecInfo->PMKIDList[j].Bssid, pBssidInfo->BSSID) )
			{ // BSSID is matched, the same AP => rewrite with new PMKID.
				CopyMem(pSecInfo->PMKIDList[j].PMKID, pBssidInfo->PMKID, sizeof(pBssidInfo->PMKID));
				blInserted = TRUE;
				break;
			}
		}

		if(!blInserted)
		{
			// Find a new entry
			for( j=0 ; j<NUM_PMKID_CACHE; j++ )
			{
				if(pSecInfo->PMKIDList[j].bUsed == FALSE)
				{
					pSecInfo->PMKIDList[j].bUsed = TRUE;
					CopyMem(pSecInfo->PMKIDList[j].Bssid, pBssidInfo->BSSID, 6);
					CopyMem(pSecInfo->PMKIDList[j].PMKID, pBssidInfo->PMKID, PMKID_LEN);
					CopyMem(pSecInfo->PMKIDList[j].SsidBuf, pMgntInfo->SsidBuf, pMgntInfo->Ssid.Length);
					pSecInfo->PMKIDList[j].Ssid.Length= pMgntInfo->Ssid.Length;
					break;
				}
			}
		}

		pBssidInfo ++;	// pointer to next DOT11_PMKID_ENTRY.
	}

	count = 0;
	for( ulIndex=0; ulIndex<NUM_PMKID_CACHE ; ulIndex++ )
	{
		if(pSecInfo->PMKIDList[ulIndex].bUsed)
		{
			count ++;

			// For debug purpose: Check current PMKID list.
			RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
			RT_TRACE( COMP_SEC, DBG_LOUD, ("[PMKID %d]\n", ulIndex ) );
			RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
			RT_TRACE( COMP_SEC, DBG_LOUD, ("SecSetPMKID(): PMKIDList[%d].bUsed is TRUE\n", ulIndex) );
			RT_PRINT_STR( COMP_SEC, DBG_LOUD, "SSID", pSecInfo->PMKIDList[ulIndex].Ssid.Octet, pSecInfo->PMKIDList[ulIndex].Ssid.Length);
			RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "BSSID", pSecInfo->PMKIDList[ulIndex].Bssid, 6);
			RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "PMKID", pSecInfo->PMKIDList[ulIndex].PMKID, sizeof(pBssidInfo->PMKID));
		}
	}
	pSecInfo->PMKIDCount  = count;

	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("<== N6CSet_DOT11_PMKID_LIST()\n") );
	return	NDIS_STATUS_SUCCESS;
}

//
// Implementation of OID_DOT11_CONNECT_REQUEST setting for NDIS6.
// Added by Annie, 2006-10-16.
// (reference MpConnectRequest(), Sta11ConnectInfra() and Sta11ConnectAdHoc() in MS's code)
//
NDIS_STATUS
N6CSet_DOT11_CONNECT_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_SSID_LIST	pSsidList = &(pNdisCommon->dot11DesiredSSIDList);
	PRT_WLAN_BSS		pRtBss = NULL;
	RT_JOIN_ACTION		join_action;
	RT_JOIN_NETWORKTYPE	CurrBSSType;
	OCTET_STRING		ssid2match;
	PRT_CHNL_LIST_ENTRY	pChnlListEntry = NULL;
	HAL_DATA_TYPE 		*pHalData = GET_HAL_DATA(Adapter);
	u8Byte				time_offset = 0;
	
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==> N6CSet_DOT11_CONNECT_REQUEST()\n") );

	if(!N6_INIT_READY(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, 
				("N6CSet_DOT11_CONNECT_REQUEST() <===, return by N6_INIT_READY\n"));
		return NDIS_STATUS_DOT11_MEDIA_IN_USE;
	}

    if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return NDIS_STATUS_FAILURE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 1\n"));											
		return NDIS_STATUS_FAILURE;
	}	

	if(PlatformAllocateMemory(Adapter, (PVOID*)&pRtBss, sizeof(RT_WLAN_BSS))  != RT_STATUS_SUCCESS)
       	 return NDIS_STATUS_FAILURE;
			
	ssid2match.Octet = pSsidList->SSIDs[0].ucSSID;
	ssid2match.Length = (u2Byte)pSsidList->SSIDs[0].uSSIDLength;

	//
	// 061207, rcnjko: We must keep current BSS type before leaving the BSS by 
	// MgntDisconnectAP or MgntDisconnectIBSS.
	//
	CurrBSSType = MgntActQuery_802_11_INFRASTRUCTURE_MODE(Adapter);


	if( pMgntInfo->bMediaConnect || pMgntInfo->bIbssStarter)
	{
		if( pMgntInfo->mIbss ){
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CONNECT_REQUEST() => MgntDisconnectIBSS\n") );
			MgntDisconnectIBSS( Adapter );
		}

		if( pMgntInfo->mAssoc)
		{
			RT_TRACE( COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CONNECT_REQUEST() => MgntDisconnectAP\n") );
			MgntDisconnectAP(Adapter, disas_lv_ss);
		}
	}
	
	if(MgntRoamingInProgress(pMgntInfo)) // Roaming.
	{
		if(MgntResetOrPnPInProgress(Adapter))
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));											
			PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
			
			return NDIS_STATUS_FAILURE;
		}	
		MgntRoamComplete(Adapter, MlmeStatus_invalid);
	}
	
	if(MgntIsLinkInProgress(pMgntInfo) && !OS_SUPPORT_WDI(Adapter)) // connecting.
	{		
		DrvIFIndicateConnectionStart(Adapter);
		DrvIFIndicateAssociationStart(Adapter);
		DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_FAILURE);
		DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);
		MgntResetLinkProcess(Adapter);
	}
	
	if (CurrBSSType == RT_JOIN_NETWORKTYPE_ADHOC)
	{
		// Clear all rejected AP record, 2006.11.21, by shien chang.
		MgntClearRejectedAsocAP(Adapter);
	}
	
	//
	// Find the Bss desc
	//
	RT_PRINT_STR( COMP_OID_SET|COMP_SEC, DBG_LOUD, "N6CSet_DOT11_CONNECT_REQUEST(): SSID", pSsidList->SSIDs[0].ucSSID, pSsidList->SSIDs[0].uSSIDLength);


	MgntAddSsidsToScan(Adapter, ssid2match);

	PlatformZeroMemory(pRtBss, sizeof(RT_WLAN_BSS));  
	join_action = SelectNetworkBySSID(Adapter, &ssid2match, FALSE, pRtBss);
	pMgntInfo->RequestFromUplayer = TRUE;	
	pMgntInfo->bDisconnectRequest = FALSE;
	Adapter->LastConnectStartIndicationTime = PlatformGetCurrentTime();

	if(join_action == RT_NO_ACTION)
	{
		RT_TRACE( COMP_OID_SET, DBG_LOUD, 
			("N6CSet_DOT11_CONNECT_REQUEST(): No BSS found, CurrBSSType: %d, #of_BSS = %d\n",
			CurrBSSType, pMgntInfo->NumBssDesc4Query));

		CopyMem(pMgntInfo->Ssid.Octet, pSsidList->SSIDs[0].ucSSID, pSsidList->SSIDs[0].uSSIDLength);
		pMgntInfo->Ssid.Length =(u1Byte) pSsidList->SSIDs[0].uSSIDLength;		

		pMgntInfo->bIbssStarter = FALSE;
		pMgntInfo->bIndicateConnectEvent = TRUE;

//		pMgntInfo->RequestFromUplayer = TRUE;

		pMgntInfo->RoamingCount=0;
		pMgntInfo->DisconnectCount=0;

		N6InitializeIndicateStateMachine(Adapter);

		MgntLinkRequest(
				Adapter,
				FALSE,		//bScanOnly
				TRUE,		//bActiveScan,
				FALSE,		//FilterHiddenAP // Asked by Netgear's Lancelot for 8187 should look like their damn wg111v1, 2005.02.01, by rcnjko.
				FALSE,		// Update parameters
				NULL,		//ssid2scan
				0,			//NetworkType,
				0,			//ChannelNumber,
				0,			//BcnPeriod,
				0,			//DtimPeriod,
				0,			//mCap,
				NULL,		//SuppRateSet,
				NULL		//yIbpm,
				);
	}
	else
	{		
		if(join_action == RT_START_IBSS)
		{
			if ( (pNdisCommon->NumDot11DesiredBSSIDList == 0) ||
				N6Dot11AddrIsBcast( pNdisCommon->dot11DesiredBSSIDList[0] ))
			{
				pMgntInfo->bIbssStarter = FALSE;
			}
			else
			{
				pMgntInfo->bIbssStarter = TRUE;
				CopyMem(pMgntInfo->Bssid, &(pNdisCommon->dot11DesiredBSSIDList[0]), sizeof(DOT11_MAC_ADDRESS));
			}
		}
		else if (join_action == RT_JOIN_IBSS)
		{			
			// 2009/02/25 MH For Samsung workaround to record adhoc link status.
			pHalData->SystemCurrTime = PlatformGetCurrentTime();
			time_offset = pHalData->SystemCurrTime - pHalData->SystemStartTime;
			//DbgPrint("time_offset=%d\n", time_offset);		
			//if (time_offset >= 2000000)
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_CONNECT_REQUEST\n") );
				pHalData->AdhocLinkState = TRUE;
				PlatformScheduleWorkItem(&(pNdisCommon->SetAdhocLinkStateWorkItem));
			}
		}
		
		CopyMem(pMgntInfo->Ssid.Octet, pSsidList->SSIDs[0].ucSSID, pSsidList->SSIDs[0].uSSIDLength);
		pMgntInfo->Ssid.Length =(u1Byte) pSsidList->SSIDs[0].uSSIDLength;
//		pMgntInfo->RequestFromUplayer = FALSE;

		//2004/08/23, kcwu, set the Privacy bit on in Capability field
		if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm != RT_ENC_ALG_NO_CIPHER)
			pMgntInfo->mCap |= cPrivacy;
		else
			pMgntInfo->mCap &= ~cPrivacy;

		
		// Initialize state machine for indication, 2006.11.30, by shien chang.
		N6InitializeIndicateStateMachine(Adapter);
		
		pMgntInfo->JoinAction = join_action;

		CCX_SetAssocReason(Adapter, CCX_AR_FIRST_ASSOCIATION);

		JoinRequest( Adapter, join_action, pRtBss);
	}

	PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
	
	RT_TRACE( COMP_OID_SET, DBG_LOUD, ("<== N6CSet_DOT11_CONNECT_REQUEST()\n") );
	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
N6CSet_DOT11_DISCONNECT_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	NDIS_STATUS		ndisStatus;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	HAL_DATA_TYPE 	*pHalData = GET_HAL_DATA(Adapter);
	u8Byte			time_offset = 0;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("N6CSet_DOT11_DISCONNECT_REQUEST()=====>\n"));

	// 2009/02/25 MH For Samsung workaround to record adhoc link status.
	pHalData->SystemCurrTime = PlatformGetCurrentTime();
	pMgntInfo->RequestFromUplayer = FALSE;
	time_offset = pHalData->SystemCurrTime - pHalData->SystemStartTime;
	//DbgPrint("time_offset=%d\n", time_offset);
	if (pMgntInfo->mIbss == TRUE/* && time_offset >= 2000000*/)
	{	
		RT_TRACE(COMP_INIT,DBG_LOUD,("N6CSet_DOT11_DISCONNECT_REQUEST\n"));
		pHalData->AdhocLinkState = FALSE;
		PlatformScheduleWorkItem(&(pNdisCommon->SetAdhocLinkStateWorkItem));
	}

	if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED)
	{
		return NDIS_STATUS_SUCCESS;	
	}
	
	if(pMgntInfo->bReceiveSystemPSOID && GET_POWER_SAVE_CONTROL(pMgntInfo)->WoWLANS5Support)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Set OID_RT_DISCONNECT_REQUEST: pMgntInfo->bReceiveSystemPSOID = TRUE and return!!\n"));
		return NDIS_STATUS_SUCCESS;
	}
	
	if(MultiChannel_IsFCSInProgress(Adapter))
	{
		MultiChannelDisconnectClient(Adapter, TRUE);
		RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("<=== Return N6CSet_DOT11_DISCONNECT_REQUEST since MCC\n"));
		return NDIS_STATUS_SUCCESS;
	}

	pMgntInfo->bDisconnectRequest = TRUE;
	
	ndisStatus = MgntActSet_802_11_DISASSOCIATE( Adapter, disas_lv_ss );

//	if(ndisStatus == NDIS_STATUS_SUCCESS)		
//		pMgntInfo->bDisconnectRequest = TRUE;

	return ndisStatus;
}

//
// Implementation of OID_DOT11_POWER_MGMT_REQUEST setting for NDIS6.
// Added by Annie, 2006-10-16.
// (reference MpSetPowerMgmtRequest() and Sta11SetPowerMgmtRequest() in MS's code)
//
NDIS_STATUS
N6CSet_DOT11_POWER_MGMT_REQUEST(
	IN	PADAPTER						Adapter,
	IN	pu4Byte							pPowerSaveLevel
	)
{
	PMGNT_INFO					pMgntInfo=&(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	
	pPSC->PowerSaveLevel = *pPowerSaveLevel;

	switch(pPSC->PowerSaveLevel)
	{
		case DOT11_POWER_SAVING_NO_POWER_SAVING:
			pPSC->PowerSaveLevel = POWER_SAVING_NO_POWER_SAVING;
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CSet_DOT11_POWER_MGMT_REQUEST(): DOT11_POWER_SAVING_NO_POWER_SAVING\n") );	
			break;
		case DOT11_POWER_SAVING_FAST_PSP:
			pPSC->PowerSaveLevel = POWER_SAVING_FAST_PSP;
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CSet_DOT11_POWER_MGMT_REQUEST(): DOT11_POWER_SAVING_FAST_PSP\n") );	
			break;
		case DOT11_POWER_SAVING_MAX_PSP:
			pPSC->PowerSaveLevel = POWER_SAVING_MAX_PSP;
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CSet_DOT11_POWER_MGMT_REQUEST():DOT11_POWER_SAVING_MAX_PSP\n") );	
			break;
		case DOT11_POWER_SAVING_MAXIMUM_LEVEL:
			pPSC->PowerSaveLevel = POWER_SAVING_MAXIMUM_LEVEL;
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CSet_DOT11_POWER_MGMT_REQUEST(): DOT11_POWER_SAVING_MAXIMUM_LEVEL\n") );	
			break;
		default:
			RT_TRACE( COMP_POWER, DBG_LOUD, ("N6CSet_DOT11_POWER_MGMT_REQUEST(): unkown\n")); 
			break;
	}

	RT_TRACE( COMP_OID_SET, DBG_TRACE, ("N6CSet_DOT11_POWER_MGMT_REQUEST(): %d\n", pPSC->PowerSaveLevel) );	
	return NDIS_STATUS_SUCCESS;
}


//
// Implementation of OID_DOT11_EXCLUDED_MAC_ADDRESS_LIST setting for NDIS6.
// Added by Shien Chang, 2006-11-01.
// (reference MpSetExcludedMACAddressList() and Sta11SetExcludedMACAddressList() in MS's code)
//
NDIS_STATUS
N6CSet_DOT11_EXCLUDED_MAC_ADDRESS_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PDOT11_MAC_ADDRESS_LIST	pMacAddrList = (PDOT11_MAC_ADDRESS_LIST)InformationBuffer;
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	
	*BytesNeeded = sizeof(DOT11_MAC_ADDRESS_LIST);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}

	if(!N6_VERIFY_OBJECT_HEADER_DEFAULT(
		pMacAddrList->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_MAC_ADDRESS_LIST_REVISION_1,
		sizeof(DOT11_MAC_ADDRESS_LIST)))
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, 
			("N6_VERIFY_OBJECT_HEADER_DEFAULT(): obj type: %u vs %u.\n", 
			pMacAddrList->Header.Type, NDIS_OBJECT_TYPE_DEFAULT));
		
		RT_TRACE(COMP_OID_SET, DBG_LOUD, 
			("N6_VERIFY_OBJECT_HEADER_DEFAULT(): mac addr list rev: %u vs %u.\n", 
			pMacAddrList->Header.Revision, DOT11_MAC_ADDRESS_LIST_REVISION_1));
		
		// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
		RT_TRACE(COMP_OID_SET, DBG_LOUD, 
			("N6_VERIFY_OBJECT_HEADER_DEFAULT(): size: %u vs %u.\n", 
			pMacAddrList->Header.Size, sizeof(DOT11_MAC_ADDRESS_LIST)));
		
		return NDIS_STATUS_INVALID_DATA;
	}

	if (pMacAddrList->uNumOfEntries == 0)
	{
			return NDIS_STATUS_SUCCESS;
	}
	
	if (pMacAddrList->uNumOfEntries > NATIVE_802_11_MAX_EXCLUDED_MACADDR)
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}

	*BytesNeeded = FIELD_OFFSET(DOT11_MAC_ADDRESS_LIST, MacAddrs)+pMacAddrList->uNumOfEntries*6;
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}
	
	if (! MgntActSet_ExcludedMacAddressList(
			Adapter, 
			(pu1Byte)pMacAddrList->MacAddrs, 
			pMacAddrList->uNumOfEntries))
	{
		return NDIS_STATUS_INVALID_DATA;
	}

	// Get the Mac address in the excluded list and if the address of AP which 
	// we are currently connecting is in it, disconnect from the AP.
	// <SC_TODO: to handle adhoc mode.>
	if ( pMgntInfo->mAssoc || pMgntInfo->mIbss )
	{
		if ( MgntIsInExcludedMACList(Adapter, pMgntInfo->Bssid) )
		{
			MgntActSet_802_11_DISASSOCIATE( Adapter, disas_lv_ss );
		}
	}
	
	*BytesRead = *BytesNeeded;
	
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CSet_DOT11_DESIRED_BSSID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_BSSID_LIST	pBssidList = (PDOT11_BSSID_LIST)InformationBuffer;
	u4Byte i = 0;

	if(!N6_VERIFY_OBJECT_HEADER_DEFAULT(
		pBssidList->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_BSSID_LIST_REVISION_1,
		sizeof(DOT11_BSSID_LIST)))
	{
		return NDIS_STATUS_INVALID_DATA;
	}

	*BytesNeeded = FIELD_OFFSET(DOT11_BSSID_LIST, BSSIDs) +
		pBssidList->uNumOfEntries*sizeof(DOT11_MAC_ADDRESS);

	//
	// For Win7 DesiredBSSIDList_ext.
	//
	if (pBssidList->uNumOfEntries > 0)
	{
		//
		// Ensure enough space for all the entries
		//
		if ( InformationBufferLength < *BytesNeeded )
		{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, 
					("N6CSet_DOT11_DESIRED_BSSID_LIST(): InformationBufferLength (%u) < BytesNeeded (%u).\n", 
					InformationBufferLength, *BytesNeeded));
			return NDIS_STATUS_BUFFER_OVERFLOW;
		}
	}

	if (pBssidList->uNumOfEntries > NATIVE_802_11_MAX_DESIRED_BSSID)
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}

	if ( InformationBufferLength < *BytesNeeded)
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}
	
	if (pBssidList->uNumOfEntries != 0)
	{
		//
		//Add for DTM 1.0c test, sometimes DTM will set this wrong mac address 
		//
		if(N6Dot11AddrIsfe(pBssidList->BSSIDs[0]))
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_DESIRED_BSSID_LIST(): Mac add is FE:FF:FF:FF:FF:FF"));
		}
		else
		{	
		PlatformMoveMemory(
			pNdisCommon->dot11DesiredBSSIDList,
			pBssidList->BSSIDs,
			pBssidList->uNumOfEntries*sizeof(DOT11_MAC_ADDRESS));

			for(; i < pBssidList->uNumOfEntries; i++)
			{
				RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "N6CSet_DOT11_DESIRED_BSSID_LIST(): desired Bss BSSID:", pNdisCommon->dot11DesiredBSSIDList[i]);
			}
	}
	}
	else
	{
		PlatformZeroMemory(
			pNdisCommon->dot11DesiredBSSIDList,
			NATIVE_802_11_MAX_DESIRED_BSSID*sizeof(DOT11_MAC_ADDRESS));
	}
	pNdisCommon->NumDot11DesiredBSSIDList = pBssidList->uNumOfEntries;

	*BytesRead = *BytesNeeded;
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CSet_DOT11_CURRENT_OPERATION_MODE(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u4Byte				length = sizeof(DOT11_CURRENT_OPERATION_MODE);

	if ( InformationBufferLength < length )
	{
		*BytesNeeded = length;
		return NDIS_STATUS_INVALID_LENGTH;
	}

	PlatformMoveMemory(
		&(pNdisCommon->dot11CurrentOperationMode),
		InformationBuffer,
		length);
	*BytesRead = length;
	
	if ( (pMgntInfo->bNetMonitorMode == FALSE) && 
		(pNdisCommon->dot11CurrentOperationMode.uCurrentOpMode == DOT11_OPERATION_MODE_NETWORK_MONITOR) )
	{
		MgntEnableNetMonitorMode(Adapter, FALSE);
	}
	else if ( (pMgntInfo->bNetMonitorMode == TRUE) &&
		(pNdisCommon->dot11CurrentOperationMode.uCurrentOpMode != DOT11_OPERATION_MODE_NETWORK_MONITOR) )
	{
		MgntDisableNetMonitorMode(Adapter, FALSE);
	}
	
	
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CSet_DOT11_DESIRED_BSS_TYPE(
	IN	PADAPTER						Adapter,
	IN	PDOT11_BSS_TYPE				pBssType
	)
{
	RT_JOIN_NETWORKTYPE	networktype = RT_JOIN_NETWORKTYPE_AUTO;
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON          pNdisCommon = Adapter->pNdisCommon;

	switch (*pBssType)
	{
	case dot11_BSS_type_infrastructure:
		networktype = RT_JOIN_NETWORKTYPE_INFRA;
		break;
	case dot11_BSS_type_independent:
		networktype = RT_JOIN_NETWORKTYPE_ADHOC;
		break;
	case dot11_BSS_type_any:
		break;
	}
	pNdisCommon->RegNetworkType = networktype;

	MgntActSet_802_11_INFRASTRUCTURE_MODE( Adapter, networktype );

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CSet_DOT11_PRIVACY_EXEMPTION_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	PRT_NDIS6_COMMON pNdisCommon = Adapter->pNdisCommon;
	PDOT11_PRIVACY_EXEMPTION_LIST	pExemptionList = 
		(PDOT11_PRIVACY_EXEMPTION_LIST)InformationBuffer;
	
	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, 
			("Set  OID_DOT11_PRIVACY_EXEMPTION_LIST.\n"));
	do
	{
	if (! N6_VERIFY_OBJECT_HEADER_DEFAULT(
				pExemptionList->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_PRIVACY_EXEMPTION_LIST_REVISION_1,
			sizeof(DOT11_PRIVACY_EXEMPTION_LIST)) )
	{
			Status = NDIS_STATUS_INVALID_DATA;
			break;
	}

	*BytesNeeded = FIELD_OFFSET(DOT11_PRIVACY_EXEMPTION_LIST, PrivacyExemptionEntries);
		
	if ( InformationBufferLength < *BytesNeeded )
	{
			Status = NDIS_STATUS_BUFFER_OVERFLOW;
			break;
	}
	
		if(pExemptionList->uNumOfEntries > NATIVE_802_11_MAX_PRIVACY_EXEMPTION)
	{
			Status = NDIS_STATUS_INVALID_LENGTH;
			break;
	}

		*BytesNeeded += pExemptionList->uNumOfEntries * sizeof(DOT11_PRIVACY_EXEMPTION);
	if ( InformationBufferLength < *BytesNeeded )
	{
			Status = NDIS_STATUS_BUFFER_OVERFLOW;
			break;
	}
	
		pNdisCommon->PrivacyExemptionEntrieNum = pExemptionList->uNumOfEntries;
		PlatformMoveMemory(pNdisCommon->PrivacyExemptionEntries, 
			pExemptionList->PrivacyExemptionEntries, 
			pExemptionList->uNumOfEntries * sizeof(DOT11_PRIVACY_EXEMPTION));

		RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, 
			"Privacy exempt list:", 
			pNdisCommon->PrivacyExemptionEntries, 
			(int)(pExemptionList->uNumOfEntries * sizeof(DOT11_PRIVACY_EXEMPTION)));
	}while(FALSE);

	return Status;

}

NDIS_STATUS
N6CSet_DOT11_IBSS_PARAMS(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_IBSS_PARAMS	pParams = (PDOT11_IBSS_PARAMS)InformationBuffer;
	PRTL_DOT11_IBSS_PARAMS	pdot11IbssParams = &pNdisCommon->dot11IbssParams;
	
	*BytesNeeded = sizeof(DOT11_IBSS_PARAMS);
	if ( InformationBufferLength < *BytesNeeded)
	{
		return NDIS_STATUS_INVALID_LENGTH;
	}

	if (!N6_VERIFY_OBJECT_HEADER_DEFAULT(
					pParams->Header, 
					NDIS_OBJECT_TYPE_DEFAULT,
					DOT11_AUTH_ALGORITHM_LIST_REVISION_1,
					sizeof(DOT11_IBSS_PARAMS)) )
	{
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_IBSS_PARAMS(), NDIS_STATUS_INVALID_DATA\n") );
		return NDIS_STATUS_INVALID_DATA;
	}

	//
    	// Verify IE blob length
   	//
   	 if ((pParams->uIEsOffset + pParams->uIEsLength) > InformationBufferLength)
   	 {
      	  	*BytesNeeded = pParams->uIEsOffset + pParams->uIEsLength;
		RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_IBSS_PARAMS(), NDIS_STATUS_BUFFER_OVERFLOW\n") );
        	return NDIS_STATUS_BUFFER_OVERFLOW;
   	 }

	// 
	// Free the past IE buffer.
	// 
	if(pdot11IbssParams->AdditionalIEData)
	{
		PlatformFreeMemory(pdot11IbssParams->AdditionalIEData, pdot11IbssParams->AdditionalIESize);
		pdot11IbssParams->AdditionalIEData = NULL;
		pdot11IbssParams->AdditionalIESize = 0;
	}

	// 
	// Allocate IE buffer
	//
	if(pParams->uIEsLength > 0)
	{
		if(PlatformAllocateMemory(Adapter, &(pdot11IbssParams->AdditionalIEData),pParams->uIEsLength)
			!= RT_STATUS_SUCCESS)
		{
			RT_TRACE( COMP_OID_SET, DBG_WARNING, (" <- [Error]N6CSet_DOT11_IBSS_PARAMS(), PlatformAllocateMemory Fail!\n") );
			return NDIS_STATUS_FAILURE;
		}

		PlatformMoveMemory(pdot11IbssParams->AdditionalIEData, (PVOID)((pu1Byte)pParams + pParams->uIEsOffset), pParams->uIEsLength);
		pdot11IbssParams->AdditionalIESize = pParams->uIEsLength;
	}
	
	pdot11IbssParams->bDot11IbssJoinOnly = pParams->bJoinOnly;

	// <SC_TODO: to store the append IE>
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_IBSS_PARAMS(): bJoinOnly = %d\n", pParams->bJoinOnly));
	
	*BytesRead = pParams->uIEsLength + pParams->uIEsOffset;
	
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
ValidateScanRequest(
	IN	PADAPTER						Adapter,
	IN	PDOT11_SCAN_REQUEST_V2		pScanRequest
	)
{
	ULONG i, BytesParsed = 0;
	PDOT11_SSID		pDot11SSID;
	
	if(!OS_SUPPORT_WDI(Adapter))
	{
		// <SC_TODO: we should validate buffer length>
		
		if (pScanRequest->uNumOfdot11SSIDs == 0)
		{
			return NDIS_STATUS_INVALID_DATA;
		}

		if (pScanRequest->uNumOfdot11SSIDs > NATIVE_802_11_MAX_SCAN_SSID)
		{
			return NDIS_STATUS_INVALID_DATA;
		}

		for (i=0; i<pScanRequest->uNumOfdot11SSIDs; i++)
		{
			pDot11SSID = (PDOT11_SSID) (pScanRequest->ucBuffer + pScanRequest->udot11SSIDsOffset + BytesParsed);
			if (pDot11SSID->uSSIDLength > DOT11_SSID_MAX_LENGTH)
			{
				return NDIS_STATUS_INVALID_DATA;
			}
			BytesParsed += sizeof(DOT11_SSID);
		}
	}
	
    if (pScanRequest->dot11BSSType != dot11_BSS_type_infrastructure &&
        pScanRequest->dot11BSSType != dot11_BSS_type_independent &&
        pScanRequest->dot11BSSType != dot11_BSS_type_any)
    {
		return NDIS_STATUS_INVALID_DATA;
    }

    switch (pScanRequest->dot11ScanType)
    {
        case dot11_scan_type_active:
        case dot11_scan_type_active | dot11_scan_type_forced:
        case dot11_scan_type_passive:
        case dot11_scan_type_passive | dot11_scan_type_forced:
        case dot11_scan_type_auto:
        case dot11_scan_type_auto | dot11_scan_type_forced:
            break;

        default:
            return NDIS_STATUS_INVALID_DATA;
    }
		
	return NDIS_STATUS_SUCCESS;
}



NDIS_STATUS
N6CSet_DOT11_SCAN_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PDOT11_SCAN_REQUEST_V2 pScanRequest = (PDOT11_SCAN_REQUEST_V2)InformationBuffer;
	NDIS_STATUS		ndisStatus;
	PADAPTER		pGoAdapter = GetFirstGOPort(Adapter);
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	BOOLEAN			bActiveScan = TRUE;
	PDOT11_SSID		pDot11SSID;
	u4Byte			i;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("=====>N6CSet_DOT11_SCAN_REQUEST()\n"));
	*BytesNeeded = FIELD_OFFSET(DOT11_SCAN_REQUEST_V2, ucBuffer);
	if ( InformationBufferLength < *BytesNeeded )
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, 
			("N6CSet_DOT11_SCAN_REQUEST(): skip scan by InformationBufferLength(%d) < BytesNeeded(%d)\n", 
			InformationBufferLength, *BytesNeeded));
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	//
	// If during auto link stage after wakeup, ignore the scan request.
	// 2007.01.22, by shien chang.
	//
	if (pNdisCommon->bWakeupAutoLinkInProgressing)
	{
		DrvIFIndicateScanStart(Adapter);
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): skip scan by bWakeupAutoLinkInProgressing.\n"));
		return NDIS_STATUS_SUCCESS;
	}

	if(ACTING_AS_AP(Adapter))
	{
		DrvIFIndicateScanStart(Adapter);
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): skip scan by ACTING_AS_AP.\n"));		
		return NDIS_STATUS_SUCCESS;	
	}

	if( // No need to scan for passing WHCK Test 
		MgntLinkStatusQuery(GetDefaultAdapter(Adapter)) == RT_MEDIA_CONNECT && 
		GetFirstGOPort(Adapter) != NULL
	)
	{
		DrvIFIndicateScanStart(Adapter);
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): skip scan by WHCK setting.\n"));
		return NDIS_STATUS_SUCCESS;
	}

	if(pMgntInfo->bDisableScanByOID)
	{
		DrvIFIndicateScanStart(Adapter);
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): skip scan by OID.\n"));
		return NDIS_STATUS_SUCCESS;
	}

	// No need to scan for passing WHCK Test 
	//by sherry 20130117
	if( (GetFirstGOPort(Adapter) != NULL) && 
		 (MgntLinkStatusQuery(GetFirstGOPort(Adapter))) &&
		 (GetDefaultAdapter(Adapter)->MgntInfo.bGoSkipScanForWFD == 1))
	{
		DrvIFIndicateScanStart(Adapter);
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		RT_TRACE(COMP_SCAN,DBG_LOUD,("N6CSet_DOT11_SCAN_REQUEST(): skip scan by WHCK setting\n"));
		return NDIS_STATUS_SUCCESS;
	}

	
	if( (GetFirstClientPort(Adapter) != NULL) && 
		 (MgntLinkStatusQuery(GetFirstClientPort(Adapter))) &&
		 (GetDefaultAdapter(Adapter)->MgntInfo.bClientSkipScanForWFD == 1))
	{
		DrvIFIndicateScanStart(Adapter);
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		RT_TRACE(COMP_SCAN,DBG_LOUD,("N6CSet_DOT11_SCAN_REQUEST(): skip scan by client!!!!!!!!!!!!!! \n"));
		return NDIS_STATUS_SUCCESS;
	}
#if 0 //(WPS_SUPPORT == 1)
	{
		PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);
		if (IsExtAPModeExist(Adapter) && pSimpleConfig->bEnabled)
		{
			RT_TRACE((COMP_OID_SET | COMP_AP), DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): AP in WPS mode! Do not scan!\n"));
			DrvIFIndicateScanStart(Adapter);
			DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
			return NDIS_STATUS_SUCCESS;
		}
	}
#endif

	if( (MgntLinkStatusQuery(GetDefaultAdapter(Adapter)) == RT_MEDIA_CONNECT) &&
		 (MgntLinkStatusQuery(GetFirstClientPort(Adapter)) == RT_MEDIA_CONNECT ||
		 MgntLinkStatusQuery(GetFirstGOPort(Adapter)) == RT_MEDIA_CONNECT) &&
		 GetDefaultAdapter(Adapter)->bInWFDTest)
	{
		DrvIFIndicateScanStart(Adapter);
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): skip scan when both port get connected\n"));
		return NDIS_STATUS_SUCCESS;
	}


	if(pGoAdapter != NULL)
	{

		PMGNT_INFO pGoMgntInfo = &(pGoAdapter->MgntInfo);
		
		if(	(pGoMgntInfo->LinkDetectInfo.bClientAskForFCS == 1) &&	
			(MgntLinkStatusQuery(GetDefaultAdapter(Adapter)) == RT_MEDIA_DISCONNECT) &&
			(MgntLinkStatusQuery(GetFirstGOPort(Adapter)) == RT_MEDIA_CONNECT) &&
		 	pGoMgntInfo->bWaitBeforeGoSkipScan != 0)
		{
			DrvIFIndicateScanStart(Adapter);
			DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): skip scan when Client is asking for FCS\n"));
			return NDIS_STATUS_SUCCESS;
		}
	
	}
	
#if (P2P_SUPPORT == 1)
	{
		PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);

		while(pLoopAdapter)
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(pLoopAdapter);
			PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(&pLoopAdapter->MgntInfo);
			
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): role: %u, state: %u, wps: %u\n", pP2PInfo->Role, pP2PInfo->State, pSimpleConfig->bEnabled));
			if(P2P_ACTING_AS_GO(pP2PInfo)
				&& (P2P_STATE_OPERATING == pP2PInfo->State || P2P_STATE_PROVISIONING == pP2PInfo->State)
				&& pSimpleConfig->bEnabled
				|| P2P_STATE_PRE_PROVISIONING == pP2PInfo->State
				)
			{
				DrvIFIndicateScanStart(Adapter);
				DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): skip scan for doing GO WPS\n"));
				return NDIS_STATUS_SUCCESS;
			}

			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}
#endif

	ndisStatus = ValidateScanRequest(Adapter, pScanRequest);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, 
			("N6CSet_DOT11_SCAN_REQUEST(): return for ValidateScanRequest() return %#X\n", 
			ndisStatus));
		return ndisStatus;
	}	

	if(	MgntScanInProgress(pMgntInfo) || MgntIsLinkInProgress(pMgntInfo))
	{
		DrvIFIndicateScanStart(Adapter);
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		RT_TRACE(COMP_SCAN, DBG_LOUD, 
				("N6CSet_DOT11_SCAN_REQUEST(): skip scan by join %d or scan %d in prorgress !!!\n", MgntIsLinkInProgress(pMgntInfo), MgntScanInProgress(pMgntInfo)));
		return NDIS_STATUS_SUCCESS;	
	}


	//
	//Add by maddest for stop sitesurvey when test chariot in ad-hoc mode
	//We will block site survey when test chariot in ad-hoc mode,
	//but we will not block site survey when site survey less than 40 second.
	//

	if(pMgntInfo->bStopScan)
	{
		pMgntInfo->bFlushScanList = TRUE;
		DrvIFIndicateScanStart(Adapter);
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		pMgntInfo->NumBssDesc = 0;
		pMgntInfo->NumBssDesc4Query = 0;

		RT_TRACE(COMP_SCAN, DBG_LOUD, 
				("[REDX]: N6CSet_DOT11_SCAN_REQUEST(): clear NumBssDesc4Query!!!\n"));
		RT_TRACE(COMP_SCAN, DBG_LOUD, 
			("N6CSet_DOT11_SCAN_REQUEST(): skip scan by pMgntInfo->bStopScan!!!\n"));
		return NDIS_STATUS_SUCCESS;
	}

	if( ((MgntLinkStatusIsWifiBusy(Adapter) || (NULL != GetFirstAPAdapter(Adapter)) || IsRTKP2PDeviceExisting(Adapter)) && 
		(Adapter->bInHctTest==FALSE) ) )
	{
		u8Byte	now_time;
		u4Byte	deltatime;
	
		now_time = PlatformGetCurrentTime();
		deltatime = (ULONG)( (now_time - Adapter->lastscantime) / 100000 ); // in 0.1 second.
		Adapter->lastscantime = now_time; // in micro-second.
		// To avoid "red X" issue when AP exist, we should consider "pMgntInfo->bFlushScanList".
		if((deltatime > 400) && pMgntInfo->bFlushScanList != TRUE)
		{
			//
			// Note: 
			//	Re-mark "bFlushScanList" as false because we cheat the OS that we have scanned completely and re-copy the old
			// 	scan list into the list for query. In other words, because we never scaned, to avoid the result of empty scan list 
			//	in OID querying, the pMgntInfo->bssDesc4Query list should be cleared but we keep pMgntInfo->bssDesc to copy the 
			//	scan list into pMgntInfo->bssDesc4Query if the following scan operation is not performed actually.
			// By Bruce, 2009-02-25.
			//
			pMgntInfo->bFlushScanList = FALSE;
			DrvIFIndicateScanStart(Adapter);
			DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
			RT_TRACE(COMP_SCAN, DBG_LOUD, 
				("N6CSet_DOT11_SCAN_REQUEST(): skip scan by busy traffic!!!\n"));
			return NDIS_STATUS_SUCCESS;
		}
	}

	DrvIFIndicateScanStart(Adapter);

	Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_SITE_SURVEY);

	switch (pScanRequest->dot11ScanType)
	{
		case dot11_scan_type_active:
		case dot11_scan_type_auto:
			bActiveScan = TRUE;
			break;
		case dot11_scan_type_passive:
			bActiveScan = FALSE;
			break;
	}

	//
	// Save the SSID to scan. 2006.12.20, by shien chang.
	//
	if(!OS_SUPPORT_WDI(Adapter))
	{
		MgntClearSsidsToScan(Adapter);
		
		pDot11SSID = (PDOT11_SSID)((pu1Byte)pScanRequest + 
					FIELD_OFFSET(DOT11_SCAN_REQUEST_V2, ucBuffer) +
					pScanRequest->udot11SSIDsOffset);
	
		for (i=0; i<pScanRequest->uNumOfdot11SSIDs; i++)
		{
			if (pDot11SSID->uSSIDLength != 0)
			{
				OCTET_STRING  tmpSsid;
	
				tmpSsid.Octet = pDot11SSID->ucSSID;
				tmpSsid.Length = (u2Byte)pDot11SSID->uSSIDLength;
	
				MgntAddSsidsToScan(Adapter, tmpSsid);	
			}
			
			pDot11SSID ++;
		}
	
	//merge from branch 1021 to pass WHCK Scan_AdditionalIE
	MgntActSet_AdditionalProbeReqIE(Adapter, 
				(pu1Byte)((pu1Byte)pScanRequest->ucBuffer + pScanRequest->uIEsOffset), 
				pScanRequest->uIEsLength);

	}

	//merge from branch 1021 to pass WHCK Scan_AdditionalIE
	MgntActSet_AdditionalProbeReqIE(Adapter, 
				(pu1Byte)((pu1Byte)pScanRequest->ucBuffer + pScanRequest->uIEsOffset), 
				pScanRequest->uIEsLength);

	{
		if(!OS_SUPPORT_WDI(Adapter))
		{
			if(RT_STATUS_SUCCESS != CustomScan_IssueSysScan(GET_CUSTOM_SCAN_INFO(Adapter), bActiveScan))
			{
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("N6CSet_DOT11_SCAN_REQUEST(): CustomScan_IssueSysScan FAIL!!!!!\n"));
				DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
			}
		}
	}
	
FunctionOut(COMP_SCAN);

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CSet_DOT11_CUSTOMIZED_SCAN_REQUEST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PCUSTOMIZED_SCAN_REQUEST pScanRequest = (PCUSTOMIZED_SCAN_REQUEST)InformationBuffer;

	VOID						*customScanInfo = GET_CUSTOM_SCAN_INFO(Adapter);
	VOID 						*req = NULL;
	FRAME_BUF					*probeReqBuf = NULL;

	u4Byte						itChnl = 0;

	if(InformationBufferLength == 1)
	{// to cease current scan process
		// do nothing
	}
	else if(InformationBufferLength == 0)
	{// in this case, a default scan is performed
		pScanRequest = NULL;
	}
	else
	{
	*BytesNeeded = FIELD_OFFSET(CUSTOMIZED_SCAN_REQUEST, ProbeReqBuf);
		if ( InformationBufferLength < *BytesNeeded )
		{// we can't read to the ProbeReqLen field
			RT_TRACE(COMP_MLME, DBG_LOUD, 
				("N6CSet_DOT11_CUSTOMIZED_SCAN_REQUEST(): a: return for InformationBufferLength(%u) < BytesNeeded(%u)\n", 
				InformationBufferLength, *BytesNeeded));
			return NDIS_STATUS_BUFFER_OVERFLOW;
		}

		*BytesNeeded += pScanRequest->ProbeReqLen;
	if(InformationBufferLength < *BytesNeeded)
		{// we can't read all ProbeReq buffer
		RT_TRACE(COMP_MLME, DBG_LOUD, 
				("N6CSet_DOT11_CUSTOMIZED_SCAN_REQUEST(): b: return for InformationBufferLength(%u) < BytesNeeded(%u)\n", 
				InformationBufferLength, *BytesNeeded));
			return NDIS_STATUS_BUFFER_OVERFLOW;
		}
	}	

	if(!pScanRequest)
		{
		MgntActSet_802_11_BSSID_LIST_SCAN(Adapter);
		return NDIS_STATUS_SUCCESS;
		}

	if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
		return NDIS_STATUS_RESOURCES;

	probeReqBuf = CustomScan_GetProbeReqBuf(req);
	PlatformMoveMemory(FrameBuf_MHead(probeReqBuf), pScanRequest->ProbeReqBuf, pScanRequest->ProbeReqLen);
	FrameBuf_Add(probeReqBuf, pScanRequest->ProbeReqLen);

	for(itChnl = 0; itChnl < pScanRequest->nChannels; itChnl++)
	{
		CustomScan_AddScanChnl(req, pScanRequest->Channels[itChnl], 1, 
			pScanRequest->ScanType, pScanRequest->Duration, 
			pScanRequest->DataRate, probeReqBuf);
	}

	CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_UNSPECIFIED, "custom scan oid");
	
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CSet_DOT11_DESIRED_SSID_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesRead,
	OUT	PULONG							BytesNeeded
	)
{
	PDOT11_SSID_LIST	pSsidList = (PDOT11_SSID_LIST)InformationBuffer;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	u4Byte	i = 0;

	FunctionIn(COMP_OID_SET);

	do

	{
		if(InformationBufferLength < (ULONG)FIELD_OFFSET(DOT11_SSID_LIST, uTotalNumOfEntries))
		{
			*BytesNeeded = sizeof(DOT11_SSID_LIST);
			*BytesRead = 0;
			ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
			break;
		}
		
		if((pSsidList->uNumOfEntries != 1) ||
			(pSsidList->uTotalNumOfEntries !=1))
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, 
				("Set AP mode OID_DOT11_DESIRED_SSID_LIST: SsidList->uNumOfEntries = %u.\n", pSsidList->uNumOfEntries));
			ndisStatus = NDIS_STATUS_INVALID_LENGTH;
			break;
		}

		if((pSsidList->SSIDs[0].uSSIDLength > 32) ||
			(pSsidList->SSIDs[0].uSSIDLength < 1))
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, 
				("N6CSet_DOT11_DESIRED_SSID_LIST(): SsidList->SSIDs[0].uSSIDLength = %u.\n", pSsidList->SSIDs[0].uSSIDLength));
			ndisStatus = NDIS_STATUS_INVALID_DATA;
			break;
		}

		PlatformMoveMemory(
			&(pNdisCommon->dot11DesiredSSIDList),
			pSsidList,
			sizeof(DOT11_SSID_LIST));
		
		PlatformMoveMemory(
			&(pNdisCommon->dot11DesiredSSIDListCopy),
			pSsidList,
			sizeof(DOT11_SSID_LIST));

		for(; i < pSsidList->uNumOfEntries; i++)
		{
			RT_PRINT_STR(COMP_SCAN, DBG_LOUD, "N6CSet_DOT11_DESIRED_SSID_LIST(): SSID:", pSsidList->SSIDs[0].ucSSID, pSsidList->SSIDs[i].uSSIDLength);
		}

		pNdisCommon->dot11DesiredSSIDListIndex = 0;
		
		*BytesRead = sizeof(DOT11_SSID_LIST);
		
	} while(FALSE);
	
	return ndisStatus;
}

NDIS_STATUS
N6CSet_DOT11_ATIM_WINDOW(
    IN PADAPTER					pAdapter,
    IN PULONG					pValue
	)
{
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);

	// <RJ_NW_TODO> check if current state is allowed to change ATIM window.
	
	pMgntInfo->dot11AtimWindow = (u2Byte)(*pValue);
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_ATIM_WINDOW(): %d\n", *pValue));
	return NDIS_STATUS_SUCCESS; 
}

NDIS_STATUS
N6CSet_DOT11_NIC_POWER_STATE(
	IN PADAPTER					Adapter,
	IN PNDIS_OID_REQUEST		pNdisOidRequest,
	IN BOOLEAN					bValue
	)
{
	PADAPTER			pAdapter = GetDefaultAdapter(Adapter);	
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);

	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===> N6CSet_DOT11_NIC_POWER_STATE(): bValue %d\n", bValue));
	// We must update variables in SetRFPowerStateWorkItemCallback(), 
	// and RF change action would be rejected in MgntActSet_RF_State(). 
	// Isaiah 2007-12-21. 
	
	//
	// Synchronized access, 2006.11.07, by shien chang.
	// Change RF power state.
	//
	// 061214, rcnjko: schedule a workitem to change RF power state.
	//
	if( bValue == TRUE )
	{
		pNdisCommon->eRfPowerStateToSet = eRfOn;
		pMgntInfo->RfRequestFromUplayer = FALSE;
	}
	else
	{
		pNdisCommon->eRfPowerStateToSet = eRfOff;
		pMgntInfo->RfRequestFromUplayer = TRUE;
	}

	// This OID might run in Passive with priority 0x9~0xA,
	// and SetRFPowerStateWorkItem run in Pssive with priority 0xE.
	// SetRFPowerStateWorkItem would end before return NDIS_STATUS_PENDING.
	if(NDIS_CURRENT_IRQL() > PASSIVE_LEVEL)
	{
		PlatformScheduleWorkItem(&(pNdisCommon->SetRFPowerStateWorkItem));
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== N6CSet_DOT11_NIC_POWER_STATE(): %d\n", bValue));
		return NDIS_STATUS_PENDING; 
	}
	else
	{
		//
		// <Roger_Notes> Revise the following synchronization method to prevent race condition, i.e., use RT_PENDED_OID_SPINLOCK instead.
		// Added by Roger, 2013.07.07.
		//
		PlatformAcquireSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		if( pAdapter->pNdisCommon->PendedRequest != NULL )
		{
			PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
			RT_ASSERT(pAdapter->pNdisCommon->PendedRequest == NULL, ("N6CSet_DOT11_NIC_POWER_STATE(): (%p)\n", pAdapter->pNdisCommon->PendedRequest));
			N6CompletePendedOID(pAdapter, RT_PENDED_OID_RF_STATE, NDIS_STATUS_FAILURE);
			PlatformAcquireSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		}
		pNdisCommon->PendedRequest = NULL;
		PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		
		SetRFPowerStateWorkItemCallback((PVOID)pAdapter);
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== N6CSet_DOT11_NIC_POWER_STATE(): bValue %d\n", bValue));
		return NDIS_STATUS_SUCCESS; 
	}
}


NDIS_STATUS
N6CSet_DOT11_OPERATIONAL_RATE_SET(
	IN	PADAPTER					pAdapter,
	IN	PVOID						InformationBuffer,
	IN	ULONG						InformationBufferLength,
	OUT	PULONG						BytesRead,
	OUT	PULONG						BytesNeeded
	)
{
	PMGNT_INFO pMgntInfo = &(pAdapter->MgntInfo);

	NDIS_STATUS ndisStatus = NDIS_STATUS_SUCCESS;
	PDOT11_RATE_SET pDot11RateSet = NULL;
	DWORD dwRequiredSize = 0;
	OCTET_STRING osRateSet;
	int DataRateIndex;

	do
	{
		*BytesRead = 0;
		*BytesNeeded = 0;

		dwRequiredSize = FIELD_OFFSET(DOT11_RATE_SET, ucRateSet);
		if (InformationBufferLength < dwRequiredSize)
		{
			*BytesNeeded = dwRequiredSize;
			ndisStatus = NDIS_STATUS_INVALID_LENGTH;
			break;
		}

		pDot11RateSet = InformationBuffer;
		if (pDot11RateSet->uRateSetLength > DOT11_RATE_SET_MAX_LENGTH ||
			pDot11RateSet->uRateSetLength == 0)
		{
			*BytesNeeded = dwRequiredSize;
			ndisStatus = NDIS_STATUS_INVALID_DATA;
			break;
		}

		dwRequiredSize += pDot11RateSet->uRateSetLength;
		if (InformationBufferLength < dwRequiredSize)
		{
			*BytesNeeded = dwRequiredSize;
			ndisStatus = NDIS_STATUS_INVALID_LENGTH;
			break;
		}
		*BytesRead = dwRequiredSize;

		FillOctetString(osRateSet, pDot11RateSet->ucRateSet, (u2Byte)(pDot11RateSet->uRateSetLength));
		
		//
		// Prevent support rate set from incorrect setting(zero data rate).
		// Added by Roger, 2007.03.21.
		//
		for( DataRateIndex=0; DataRateIndex<osRateSet.Length; DataRateIndex++ )
		{
			if( (osRateSet.Octet[DataRateIndex] & 0x7f) == 0x00 )
			{
				RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Zero Data rate exists!!\n"));
				switch(pMgntInfo->dot11CurrentWirelessMode)
				{
				case WIRELESS_MODE_A:
					CopyMemOS(&osRateSet, pMgntInfo->RegSuppRateSets[0],pMgntInfo->RegSuppRateSets[0].Length);
					break;
				case WIRELESS_MODE_B:
					CopyMemOS(&osRateSet, pMgntInfo->RegSuppRateSets[1],pMgntInfo->RegSuppRateSets[1].Length);
					break;
				case WIRELESS_MODE_G:
					CopyMemOS(&osRateSet, pMgntInfo->RegSuppRateSets[2],pMgntInfo->RegSuppRateSets[2].Length);
					break;
				default:
					break;
				}				
				break;	
			}
		}		
		

	} while(FALSE);

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CSet_DOT11_OPERATIONAL_RATE_SET()\n"));
    return ndisStatus;
}

NDIS_STATUS
N6CSet_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER					Adapter,
	OUT	PVOID						InformationBuffer,
	IN	ULONG						InformationBufferLength,
	OUT	PULONG						BytesRead,
	OUT	PULONG						BytesNeeded
	)
{
	PDOT11_PHY_ID_LIST	pPhyIdList = (PDOT11_PHY_ID_LIST)InformationBuffer;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	BOOLEAN			AnyPhyId = FALSE;
	u1Byte			index;
	
	*BytesRead = 0;
	*BytesNeeded =0;

	if ( InformationBufferLength < sizeof(DOT11_PHY_ID_LIST) )
	{
	*BytesNeeded = sizeof(DOT11_PHY_ID_LIST);
		return NDIS_STATUS_INVALID_LENGTH;
	}

	if (!N6_VERIFY_OBJECT_HEADER_DEFAULT(pPhyIdList->Header, 
                                          NDIS_OBJECT_TYPE_DEFAULT,
                                          DOT11_PHY_ID_LIST_REVISION_1,
                                          sizeof(DOT11_PHY_ID_LIST)))
        {
        	return NDIS_STATUS_INVALID_DATA;
	}

        if (pPhyIdList->uNumOfEntries < 1)
        {
             return NDIS_STATUS_INVALID_DATA;
        }

	 // Verify length/number of entries match up
        *BytesNeeded = pPhyIdList->uNumOfEntries * sizeof(ULONG) +
                       FIELD_OFFSET(DOT11_PHY_ID_LIST, dot11PhyId);

	if ( InformationBufferLength < *BytesNeeded )
	{
            return  NDIS_STATUS_BUFFER_OVERFLOW;
        }

	//
	// If the list is too long or too short, simply return error.
	//
	if (pPhyIdList->uNumOfEntries > NATIVE_802_11_MAX_NUM_PHY_TYPES)
	{
		*BytesRead = FIELD_OFFSET(DOT11_PHY_ID_LIST, dot11PhyId);
		return NDIS_STATUS_INVALID_LENGTH;
	}


	*BytesRead = FIELD_OFFSET(DOT11_PHY_ID_LIST, dot11PhyId) + pPhyIdList->uNumOfEntries * sizeof(ULONG);
	
	//
	// Make sure we support all the PHY IDs in the list.
	//
	for (index = 0; index < pPhyIdList->uNumOfEntries; index++)
	{
		if (pPhyIdList->dot11PhyId[index] == DOT11_PHY_ID_ANY)
		{
			if(pPhyIdList->uNumOfEntries > 1)
			{
				return NDIS_STATUS_INVALID_DATA;
			}

			AnyPhyId = TRUE;
	        }
		else if (pPhyIdList->dot11PhyId[index] >= pNdisCommon->pDot11SupportedPhyTypes->uTotalNumOfEntries) 
		{
			return NDIS_STATUS_INVALID_DATA;
		}
    	}


	//
	// Copy the desired PHY list.
	//
	if (AnyPhyId)
	{
		
		pNdisCommon->staDesiredPhyCount = 1;
		pNdisCommon->staDesiredPhyList[0] = DOT11_PHY_ID_ANY;
	}
	else
	{
		pNdisCommon->staDesiredPhyCount = pPhyIdList->uNumOfEntries;
		NdisMoveMemory( pNdisCommon->staDesiredPhyList,
			pPhyIdList->dot11PhyId,
			pNdisCommon->staDesiredPhyCount * sizeof(ULONG));
	}

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N6CQuerySet_DOT11_RESET_REQUEST(
	IN	PADAPTER	Adapter,
	OUT	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	IN	ULONG		OutputBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
	)
{

	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_RESET_REQUEST pDot11ResetRequest = InformationBuffer;
	PDOT11_STATUS_INDICATION pDot11StatusIndication = InformationBuffer;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER                 pLoopAdapter = GetDefaultAdapter(Adapter);
    PRT_CHANNEL_INFO                pChnlInfo;

			
	//
	// First make sure the input buffer is large enough to
	// hold a RESET_CONFIRM
	//
	if ( OutputBufferLength < sizeof(DOT11_STATUS_INDICATION) )
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OutputBufferLength too short\n"));
		*BytesNeeded = sizeof(DOT11_STATUS_INDICATION);
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}
    
	//
	// Validate the buffer length
	//		
	if ( InputBufferLength < sizeof(DOT11_RESET_REQUEST) )
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("InputBufferLength too short\n"));
		*BytesNeeded = sizeof(DOT11_RESET_REQUEST);
		return NDIS_STATUS_INVALID_LENGTH;
	}

	switch (pDot11ResetRequest->dot11ResetType)
	{
		case dot11_reset_type_phy:
		case dot11_reset_type_mac:
		case dot11_reset_type_phy_and_mac:
			break;
		default:
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Unknown reset request.\n"));
			return NDIS_STATUS_INVALID_DATA;
	}


	//
	// <Roger_Notes> When the reset operation is complete, the miniport driver must return a DOT11_STATUS_INDICATION 
	//structure to confirm the reset operation, so do NOT return ndisStatus immediately without status indication.
	// 2014.05.02.
	//	
	do{
		
		//
		// According to WDK, reset the multicast list when the reset layer is mac layer, by Bruce, 2008-06-25.
		//
		if((pDot11ResetRequest->dot11ResetType) >= dot11_reset_type_mac &&
			(pDot11ResetRequest->dot11ResetType) <= dot11_reset_type_phy_and_mac)
		{
			// Clear all multicast list.
			Adapter->MCAddrCount = 0;
			PlatformZeroMemory(Adapter->MCList, (MAX_MCAST_LIST_NUM * ETHERNET_ADDRESS_LENGTH));
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CQuerySet_DOT11_RESET_REQUEST(): Reset Multicast List!\n"));
		}

		if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED)
		{ 
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CQuerySet_DOT11_RESET_REQUEST(): Fake AP mode, Skip Reset process!!\n"));
			break;
		}
		
		if(pMgntInfo->bReceiveSystemPSOID && GET_POWER_SAVE_CONTROL(pMgntInfo)->WoWLANS5Support)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N6CQuerySet_DOT11_RESET_REQUEST(): bReceiveSystemPSOID and WoWLANS5Support, Skip Reset process!!\n"));
			break;
		}
	
		pMgntInfo->bResetInProgress = TRUE;

		RT_TRACE(COMP_MLME, DBG_LOUD, ("before reset State_AuthReqService %d port number %d\n", pMgntInfo->State_AuthReqService, Adapter->pNdis62Common->PortNumber));
		MgntResetJoinProcess(Adapter);

		if(MgntIsLinkInProgress(pMgntInfo) || MgntRoamingInProgress(pMgntInfo))
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntIsLinkInProgress\n"));
		
			pMgntInfo->bJoinInProgress = FALSE;
			if(MgntRoamingInProgress(pMgntInfo))
				DrvIFIndicateRoamingStart(Adapter);
			else
				DrvIFIndicateConnectionStart(Adapter);
			
			DrvIFIndicateAssociationStart(Adapter);
			DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_FAILURE);
			
			if(MgntRoamingInProgress(pMgntInfo))
				DrvIFIndicateRoamingComplete(Adapter, RT_STATUS_FAILURE);
			else
				DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);
		}

		MgntResetRoamingState(pMgntInfo);

		// The SCAN_REQUEST work item may be canceled by RESET_REQUEST, that will cause 0xC4 BSOD
		// We now first indicate the ScanComplete to prevent this issue.
		if(pNdisCommon->bToIndicateScanComplete)
			DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		
		// <SC_TODO: Actual reset action>
		if(FALSE == WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPIENABLE, 0))
		{
		    if( pMgntInfo->mIbss ){			
				RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnectIBSS\n"));
				
		        MgntDisconnectIBSS( Adapter );
		    }
		}

		if( pMgntInfo->mAssoc){
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnectAP\n"));
			
			MgntDisconnectAP(Adapter, disas_lv_ss);
		}

		//check if there's any other port doing scan
		while(pLoopAdapter)
		{
			PMGNT_INFO	pLoopMgntInfo = &pLoopAdapter->MgntInfo;
			pChnlInfo = pLoopMgntInfo->pChannelInfo;
			if(pChnlInfo->ChnlOp == CHNLOP_SCAN)
				break;
			else                    
				pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
         
		if(pLoopAdapter == NULL || Adapter == pLoopAdapter)
		{
			if(MgntScanInProgress(&(Adapter->MgntInfo)))
				MgntResetScanProcess(Adapter);
			else
			{
				// Prefast warning C28182: Dereferencing NULL pointer.
				if (GetDefaultAdapter(Adapter) != NULL &&
					GetDefaultMgntInfo(Adapter) != NULL)
					CustomScan_ResetReqNoScanCb(GET_CUSTOM_SCAN_INFO(Adapter));
			}
		}
		else
		{
			RT_TRACE(COMP_MLME,DBG_LOUD,("Other port number %d is scaning, abandon ResetScan\n", pLoopAdapter->pNdis62Common->PortNumber));
		}        

		PlatformCancelTimer(Adapter, &(pMgntInfo->pChannelInfo->SwBwTimer));

		if(IS_HARDWARE_TYPE_8821U(Adapter))
		{
			pMgntInfo->LEDAssocState = LED_ASSOC_SECURITY_NONE;
		}

		//
		// For Win7 NDISTest.
		// During the NIC reset issued from NDIS, the card must preserve the current setting of operation mode.
		// Refer to OID_DOT11_CURRENT_OPERATION_MODE in DDK Document.
		// 2008.08.08, haich.
		//
		//if(pDot11ResetRequest->bSetDefaultMIB) // This causes some problem (can's send packet, invalid state) 
											  // and it's even not refered to in sample code.
		{
			DOT11_CURRENT_OPERATION_MODE dot11CurrentOperationMode;

			// backup
			PlatformMoveMemory(&dot11CurrentOperationMode, 
				&pNdisCommon->dot11CurrentOperationMode, 
				sizeof(DOT11_CURRENT_OPERATION_MODE));

			N6InitializeNative80211MIBs(Adapter);

			// restore
			PlatformMoveMemory(&pNdisCommon->dot11CurrentOperationMode,
				&dot11CurrentOperationMode, 
				sizeof(DOT11_CURRENT_OPERATION_MODE));
		}

		//
		// DDK: The NIC shall clear privacy exemption list on every reset 
		// regardless of the rest requests parameters.
		// 2008.10.17, haich
		//
		Adapter->pNdisCommon->PrivacyExemptionEntrieNum = 0;


		//
		// 061204, ccw: clear key content and reset key id.
		//	
		if(FALSE == WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPIENABLE, 0))
		{
			SecClearAllKeys(Adapter);
			SecInit(Adapter);
		}

		//
		// A very dirty fix for driver initialization issue only visible on HP compaq nx6325 which
		// borrowed from Lancelot.
		// When the issue occured, our wireless adapter is initialized successfully but AutoConfig
		// doesn't recognize our adapter as wireless adapter. At the moment, we can't site
		// survey or do any thing else, but the HW do normally works.
		// To fix the issue, we put the following IO operation or delay 1sec. It may be a timing issue.
		// 2007.02.02, by shien chang.
		//
		//Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);

		// Reset network type. 
		pMgntInfo->Regdot11networktype = RT_JOIN_NETWORKTYPE_INFRA;

		// Reset Bsssid. 
		PlatformZeroMemory(pMgntInfo->Bssid, sizeof(pMgntInfo->Bssid));

		// 061206, rcnjko: reset managment related setting.
		ResetMgntVariables(Adapter);

		if(!Adapter->bInHctTest)	//workaround for Performance_ext
		{
			// cosa add for redX issue, if ssid not cleared, then in pnp wakeup we will indicate roam start
			// and try to connect the ap with ssid matched, but OS don't think we should in roam state, 
			// so it will request wifi to do scan, the scan will be returned then OS later query scan list will be empty. => redX.
			pMgntInfo->bScanOnly = TRUE;
			//Set SSID as dummy to stop the current join process.
			SET_SSID_DUMMY(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);
		}
		ResetRxStatistics(Adapter);
		ResetTxStatistics(Adapter);
		Adapter->RxStats.QueryAfterFirstReset =1;	
		pMgntInfo->bResetInProgress = FALSE;
	
	}while(FALSE);
		
	//
	// <Roger_Notes> After reset completed, return the status indication. The miniport driver must not set the value 
	// of the BytesWritten member of the OidRequest parameter, such set operation has been removed by me, 2014.05.02.
	//	
	PlatformZeroMemory(pDot11StatusIndication, sizeof(DOT11_STATUS_INDICATION));
	pDot11StatusIndication->uStatusType = DOT11_STATUS_RESET_CONFIRM;
	pDot11StatusIndication->ndisStatus = ndisStatus;		
	*BytesRead = sizeof(DOT11_RESET_REQUEST);
	*BytesWritten = sizeof(DOT11_STATUS_INDICATION);

	FunctionOut(COMP_MLME);

	return ndisStatus;
}

NDIS_STATUS
N6CQuerySet_DOT11_ENUM_BSS_LIST(
	IN	PADAPTER	Adapter,
	OUT	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	IN	ULONG		OutputBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesNeeded
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PDOT11_BYTE_ARRAY	pByteArray = (PDOT11_BYTE_ARRAY)InformationBuffer;
	RT_802_11_BSSID_LIST RtBssList;
	PRT_WLAN_BSS		pRtBss;
	PDOT11_BSS_ENTRY	pDot11BssEntry;
	u4Byte				i;
	u4Byte				BssEntryLength;
	u4Byte				RemainingBytes;
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PDOT11_INFO_ELEMENT	pDot11SsidElement;

	*BytesWritten = 0;
	*BytesNeeded = 0;

	RT_TRACE(COMP_MLME, DBG_LOUD, 
					("[REDX]: N6CQuerySet_DOT11_ENUM_BSS_LIST() ===> \n"));
	
	if ( OutputBufferLength < (ULONG)FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer) )
	{
		*BytesNeeded = sizeof(DOT11_BYTE_ARRAY);
		RT_TRACE(COMP_MLME, DBG_LOUD, 
					("[REDX]: N6CQuerySet_DOT11_ENUM_BSS_LIST() <===, return by NDIS_STATUS_BUFFER_OVERFLOW \n"));
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	if(Adapter->bDriverIsGoingToUnload || Adapter->bSurpriseRemoved)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, 
					("[REDX]: N6CQuerySet_DOT11_ENUM_BSS_LIST() <===, return by bDriverIsGoingToUnload=%d, bSurpriseRemoved=%d\n",
					Adapter->bDriverIsGoingToUnload, Adapter->bSurpriseRemoved));
		return NDIS_STATUS_FAILURE;	
	}

	// Check if media is in use. 2006.11.21, by shien chang.
	// Comment out by Bruce, 2009-03-16.
	// We have protected the query list by RT_SCAN_SPIN_LOCK. Thus, the list shall be 
	// returned, otherwise the scan list prompted on vista UI would be empty.
	// if ( pNdisCommon->bToIndicateScanComplete )
	// {
	//	return NDIS_STATUS_DOT11_MEDIA_IN_USE;
	// }
	
	N6_ASSIGN_OBJECT_HEADER(
		pByteArray->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_BSS_ENTRY_BYTE_ARRAY_REVISION_1,
		sizeof(DOT11_BYTE_ARRAY));
	
	if (pNdisCommon->KeepDisconnectFlag)
	{
		pByteArray->uNumOfBytes = 0;
		pByteArray->uTotalNumOfBytes = 0;
		*BytesWritten = FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer);
		RT_TRACE(COMP_MLME, DBG_LOUD, 
					("[REDX]: N6CQuerySet_DOT11_ENUM_BSS_LIST() <===, return by KeepDisconnectFlag\n"));
		return NDIS_STATUS_SUCCESS;
	}

	if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED)
	{
		pByteArray->uNumOfBytes = 0;
		pByteArray->uTotalNumOfBytes = 0;
		*BytesWritten = FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer);
		RT_TRACE(COMP_MLME, DBG_LOUD, 
					("[REDX]: N6CQuerySet_DOT11_ENUM_BSS_LIST() <===, return by RT_AP_TYPE_IBSS_EMULATED\n"));
		return NDIS_STATUS_SUCCESS;
	}

	// Get the list of BSS scanned.
	RtBssList.NumberOfItems = 0;
	RtBssList.pbssidentry = Adapter->bssDescList;

	
	
	MgntActQuery_802_11_BSSID_LIST(Adapter, &RtBssList, FALSE);

	pDot11BssEntry = (PDOT11_BSS_ENTRY)(pByteArray->ucBuffer);
	pByteArray->uNumOfBytes = 0;
	pByteArray->uTotalNumOfBytes = 0;
	RemainingBytes = OutputBufferLength - FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer);

	for (i=0; i<RtBssList.NumberOfItems; i++)
	{
		//
		//To avoid pass the IE length is 0, Maddest 20081217
		//
		if(RtBssList.pbssidentry[i].IE.Length <= sizeof(RT_FIXED_IE_FIELD))			
		{
			continue;
		}
		pRtBss = &(RtBssList.pbssidentry[i]);

		// Determine if we shall report the BSS found by RegWirelessMode4ScanList.
		if(!FilterRtBss(Adapter, pRtBss))
		{
			// Skip this BSS.
			continue;
		}

		// Because NDIS6 define that IE starts from variable field, so we need minus
		// fixed field.
		BssEntryLength = pRtBss->IE.Length - sizeof(RT_FIXED_IE_FIELD) + 
							FIELD_OFFSET(DOT11_BSS_ENTRY, ucBuffer);

#if (WPS_SUPPORT == 1)
		BssEntryLength += (pRtBss->osBeaconWcnIe.Length + pRtBss->osProbeRspWcnIe.Length);
#endif				

		pDot11SsidElement = (PDOT11_INFO_ELEMENT)(pRtBss->IE.Octet +sizeof(RT_FIXED_IE_FIELD));

		// 20090508 Joseph: Revise size checking. Prevent from error IE length.
		if((pDot11SsidElement->ElementID != EID_SsId) ||
			((pDot11SsidElement->Length+sizeof(DOT11_INFO_ELEMENT))>(pRtBss->IE.Length-sizeof(RT_FIXED_IE_FIELD))))
			continue;

		if ( ! BeHiddenSsid(pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen) &&
			BeHiddenSsid((pu1Byte)pDot11SsidElement+sizeof(DOT11_INFO_ELEMENT), pDot11SsidElement->Length) )
		{
			BssEntryLength = (BssEntryLength - pDot11SsidElement->Length) + pRtBss->bdSsIdLen;
		}
		else
		{
			if(NeedRemoveWCNIE(Adapter, pRtBss))
			{
#if (WPS_SUPPORT == 1)
				static u1Byte	Simpleconf[]={0x00, 0x50, 0xF2, 0x04};
				RT_DOT11_IE		Dot11Ie;
				u4Byte	nextOffset = sizeof(RT_FIXED_IE_FIELD);
				u4Byte	curOffset;
				u4Byte	skiplength = 0;

				while(HasNextIE(&pRtBss->IE, nextOffset))
				{
					curOffset = nextOffset;
					Dot11Ie = AdvanceToNextIE(&pRtBss->IE, &nextOffset);
					if (Dot11Ie.Id == 0xDD)
					{
						if(PlatformCompareMemory(Dot11Ie.Content.Octet, Simpleconf, sizeof(Simpleconf)) == 0)
						{
							skiplength = Dot11Ie.Content.Length;
							if(skiplength != 0)
							{
								RT_ASSERT(BssEntryLength >= (ULONG)(skiplength + 2), ("invalid length BssEntryLength %d skiplength %d", BssEntryLength, skiplength));		
								BssEntryLength = BssEntryLength - skiplength - 2;
							}
							continue;
						}
					}

				}
#endif
			}
		}
	
		//
		// 20090508 Joseph: Revise size checking.
		// Since the type of variable "BssEntryLength" is "u4Byte", it is always ">=0". Also, according to the
		// caculation of "BssEntryLength", it is always ">0". We do not need to check "if(BssEntryLength<=0)" case.
		//if(BssEntryLength <= 0)
		//	continue;
	
		RT_PRINT_ADDR(COMP_SCAN, DBG_LOUD, "N6CQuerySet_DOT11_ENUM_BSS_LIST(): new Bss BSSID:", pRtBss->bdBssIdBuf);
		RT_PRINT_STR(COMP_SCAN, DBG_LOUD, "N6CQuerySet_DOT11_ENUM_BSS_LIST(): new Bss SSID:", pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen);		
	
		pByteArray->uTotalNumOfBytes += BssEntryLength;	
		if ((RemainingBytes >= BssEntryLength) && (RemainingBytes > 0))
		{
			pByteArray->uNumOfBytes += BssEntryLength;
			RemainingBytes -= BssEntryLength;

			// Copy the BSS Entry.
			PlatformZeroMemory(pDot11BssEntry, BssEntryLength);
			N6CTranslateRtBssToDot11Bss(Adapter, pDot11BssEntry, pRtBss);
			
			//
			// Copy the hidden ssid
			//
			// 20090508 Joseph: Revise size checking. Move Hidden SSID IE handling to N6CTranslateRtBssToDot11Bss().
			// This makes sure that memory copy operation is not exceed the buffer provided by OS.
			/*
			if ( ! BeHiddenSsid(pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen) &&
				BeHiddenSsid((pu1Byte)pDot11SsidElement+sizeof(DOT11_INFO_ELEMENT), pDot11SsidElement->Length) )
			{
				ndisStatus = FillHiddenSSIDinIE(Adapter, pDot11BssEntry, pRtBss);
			}
			*/
		
			pDot11BssEntry = (PDOT11_BSS_ENTRY)(((pu1Byte)pDot11BssEntry) + BssEntryLength);
		}
		else
		{
			RemainingBytes = 0;
			ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
		}

				
	}

	*BytesWritten = pByteArray->uNumOfBytes + FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer);
	*BytesNeeded = pByteArray->uTotalNumOfBytes + FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer);

	RT_TRACE(COMP_MLME, DBG_LOUD, 
					("[REDX]: N6CQuerySet_DOT11_ENUM_BSS_LIST() <===, ndisStatus=0x%x\n", ndisStatus));
	return ndisStatus;
}

//
// Description:
//	Set the access multicast address list of MAC.
// Arguments:
//	Adapter - 
//		NIC adapter context.
//	MCListbuf -
//		Multicast address list.
//	MCListlen
//		The length of MCListbuf.
//	bAcceptAllMulticast -
//		Determine if the NIC accept all multicast packets.
// Note:
//	This part does the same thing as MgntActSet_802_3_MULTICAST_LIST().
// By Bruce, 2008-06-20.
//
NDIS_STATUS
N6CSet_DOT11_MULTICAST_LIST(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	OUT	PULONG		BytesRead,
    OUT PULONG		BytesNeeded
	)
{
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	

	*BytesRead = 0;
	*BytesNeeded = sizeof(DOT11_MAC_ADDRESS);

	if(InputBufferLength % sizeof(DOT11_MAC_ADDRESS)) 
	{
		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("N6CSet_DOT11_MULTICAST_LIST(): not a multiple of sizeof(DOT11_MAC_ADDRESS)!\n"));    
		ndisStatus = NDIS_STATUS_INVALID_LENGTH;
        return ndisStatus;
    }

	//
	// Verify that we can hold the multicast list
	//
    if(InputBufferLength > (MAX_MCAST_LIST_NUM * sizeof(DOT11_MAC_ADDRESS)))
    {
    	RT_TRACE(COMP_OID_SET, DBG_WARNING, ("N6CSet_DOT11_MULTICAST_LIST(): contains more entries than supported by this miniport!\n"));
		ndisStatus = NDIS_STATUS_MULTICAST_FULL;
		*BytesNeeded = MAX_MCAST_LIST_NUM * sizeof(DOT11_MAC_ADDRESS);
        return ndisStatus;
    }

	*BytesRead = InputBufferLength;

	MgntActSet_802_3_MULTICAST_LIST(
		Adapter,
		InformationBuffer,
		InputBufferLength,
		(BOOLEAN)(pNdisCommon->NdisPacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST));
	
	return ndisStatus;
}

NDIS_STATUS
N6C_OID_DOT11_NIC_POWER_STATE(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest)
{
	//
	// Power state OID is phy specific, not port specific. Miniport driver should ignore the port number 
	// on this OID and only use the current phy id.
	// If miniport supports the functionality of multiple MAC entities through virtualization 
	// the property msDot11NICPowerState should apply to the same PHY on ALL such MAC entities. 
	// If the OS enables or disables a PHY through a set OID call on one of the MAC entities, 
	// the miniport should apply that on the same PHY on all virtualized MAC entities and send appropriate 
	// NDIS_STATUS_DOT11_PHY_STATE_CHANGED indications for all the MAC state entities (physical and virtual).
	//
	NDIS_STATUS	ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);

	if(NdisRequest->RequestType == NdisRequestQueryInformation)	// query default power state , this is the same as vista sta oid.
	{// use default port to query
		ULONG ulInfo = (ULONG)N6CQuery_DOT11_NIC_POWER_STATE(pDefaultAdapter);
		PVOID pInfo = (PVOID) &ulInfo;
		if(sizeof(BOOLEAN) <= NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength)
		{
			NdisMoveMemory(NdisRequest->DATA.METHOD_INFORMATION.InformationBuffer, pInfo, sizeof(BOOLEAN));

			NdisRequest->DATA.QUERY_INFORMATION.BytesWritten = sizeof(BOOLEAN);
			ndisStatus = NDIS_STATUS_SUCCESS;
		}
		else
		{
			NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded = sizeof(BOOLEAN);
			ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
		}
	}
	else if(NdisRequest->RequestType == NdisRequestSetInformation)	// set default port power state
	{
		BOOLEAN bRfOn = *((PBOOLEAN)NdisRequest->DATA.SET_INFORMATION.InformationBuffer);
		
		NdisRequest->DATA.SET_INFORMATION.BytesNeeded = sizeof(BOOLEAN);
		
		if( NdisRequest->DATA.SET_INFORMATION.InformationBufferLength < NdisRequest->DATA.SET_INFORMATION.BytesNeeded)
		{
			ndisStatus = NDIS_STATUS_INVALID_LENGTH;
		}
		else
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OID_DOT11_NIC_POWER_STATE: bRFOn(%d)\n", bRfOn));

			ndisStatus = N6CSet_DOT11_NIC_POWER_STATE(
						pDefaultAdapter, 
						pDefaultAdapter->pNdisCommon->PendedRequest, 
						bRfOn
				);
		}
	}

	
	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	ndisStatus = N6CSet_DOT11_DESIRED_PHY_LIST(
			pTargetAdapter,
			InformationBuffer,
			InformationBufferLength,
			BytesRead,
			BytesNeeded
		);
		
	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_DOT11_AUTO_CONFIG_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	u4Byte	tempAutoConfigEnable = 0;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	*BytesNeeded = sizeof(ULONG);

	if ( InformationBufferLength < *BytesNeeded )
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set AP mode OID_DOT11_AUTO_CONFIG_ENABLED: Invalid length\n"));
		return NDIS_STATUS_INVALID_LENGTH;
	}

	tempAutoConfigEnable=ALLOWED_AUTO_CONFIG_FLAGS & *((PULONG)InformationBuffer);
	pTargetAdapter->pNdisCommon->dot11AutoConfigEnabled = tempAutoConfigEnable;
		
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<===Set AP mode OID_DOT11_AUTO_CONFIG_ENABLED:\n"));
		
	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_DOT11_BEACON_PERIOD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	u2Byte u2BcnIntv = 0;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	// Verify input paramter.
	if(InformationBufferLength < sizeof(u2Byte))
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set AP mode OID_DOT11_BEACON_PERIOD: Invalid length\n"));
		ndisStatus = NDIS_STATUS_INVALID_STATE;
		*BytesNeeded = sizeof(u2Byte);
		return ndisStatus;
	}

	u2BcnIntv = *((pu2Byte)InformationBuffer);
	pTargetAdapter->pNdisCommon->RegBeaconPeriod = u2BcnIntv;
	MgntActSet_802_11_BeaconInterval(pTargetAdapter, u2BcnIntv);	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<===Set AP mode OID_DOT11_BEACON_PERIOD:\n"));
	
	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_DOT11_DTIM_PERIOD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	u1Byte u1DtimPeriod = 0;
		
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	// Verify input paramter.
	if(InformationBufferLength < sizeof(u1Byte))
	{
		ndisStatus = NDIS_STATUS_INVALID_LENGTH;
		*BytesNeeded = sizeof(u1Byte);
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set AP mode OID_DOT11_DTIM_PERIOD: Invalid length\n"));
		return ndisStatus;
	}

	u1DtimPeriod = *((pu1Byte)InformationBuffer);
	pTargetAdapter->pNdisCommon->RegDtimPeriod = u1DtimPeriod;
	MgntActSet_802_11_DtimPeriod(pTargetAdapter, u1DtimPeriod);
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<===Set AP mode OID_DOT11_DTIM_PERIOD:\n"));
		
	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_DOT11_DESIRED_SSID_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
		
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	if (GetAPState(pTargetAdapter) != AP_STATE_STOPPED)
	{
		ndisStatus = NDIS_STATUS_INVALID_STATE;
		RT_TRACE(COMP_INDIC, DBG_LOUD, ("AP is not in INIT STATE\n"));
		return ndisStatus;
	}

	ndisStatus = N6CSet_DOT11_DESIRED_SSID_LIST(
					pTargetAdapter,
					InformationBuffer,
					InformationBufferLength,
					BytesRead,
					BytesNeeded);

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set AP mode OID_DOT11_DESIRED_SSID_LIST\n"));

	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_GEN_CURRENT_PACKET_FILTER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG			PacketFilter = 0;
	PULONG			localInformationBuffer = (PULONG)InformationBuffer;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);


	if(InformationBufferLength < sizeof(ULONG))
	{
		ndisStatus = NDIS_STATUS_INVALID_LENGTH;
		return ndisStatus;
	}

	PacketFilter = NIC_SUPPORTED_FILTERS;

	*BytesRead = InformationBufferLength;
	
	if( InformationBufferLength == sizeof(ULONG) )
	{
		PacketFilter = *(PULONG)InformationBuffer;
	}
	else
	{
		PlatformMoveMemory(&PacketFilter, (PVOID)(localInformationBuffer+4), sizeof(ULONG));
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("PacketFilter: %x\n", PacketFilter));

	//
	// Note:
	//	A miniport driver operating in other Native 802.11 modes besides NetMon must
	//	not enable these packet filter settings (Defined in WDK).
	// By Bruce, 2008-06-27.
	//
	if(!pTargetAdapter->MgntInfo.bNetMonitorMode)
	{
		PacketFilter &= ~(NDIS_PACKET_TYPE_PROMISCUOUS | 
							NDIS_PACKET_TYPE_802_11_RAW_DATA |
							NDIS_PACKET_TYPE_802_11_PROMISCUOUS_MGMT |
							NDIS_PACKET_TYPE_802_11_RAW_MGMT);
	}

	if( PacketFilter & NDIS_PACKET_TYPE_DIRECTED) 					{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_DIRECTED\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_MULTICAST) 					{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_MULTICAST\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) 				{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_ALL_MULTICAST\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_BROADCAST) 					{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_BROADCAST\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS) 				{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_PROMISCUOUS\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_ALL_LOCAL) 					{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_ALL_LOCAL\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_SOURCE_ROUTING) 			{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_SOURCE_ROUTING\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_SMT) 						{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_SMT\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_GROUP) 						{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_GROUP\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_ALL_FUNCTIONAL) 			{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_ALL_FUNCTIONAL\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_FUNCTIONAL) 				{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_FUNCTIONAL\n"));}
	if( PacketFilter & NDIS_PACKET_TYPE_MAC_FRAME) 					{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_MAC_FRAME\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_RAW_DATA)			{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_RAW_DATA\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_DIRECTED_MGMT)		{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_DIRECTED_MGMT\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_MULTICAST_MGMT )		{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_MULTICAST_MGMT\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_ALL_MULTICAST_MGMT)	{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_ALL_MULTICAST_MGMT\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_BROADCAST_MGMT)		{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_BROADCAST_MGMT\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_PROMISCUOUS_MGMT)	{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_PROMISCUOUS_MGMT\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_RAW_MGMT)			{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_RAW_MGMT\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_DIRECTED_CTRL)		{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_DIRECTED_CTRL\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_BROADCAST_CTRL)		{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_BROADCAST_CTRL\n"));}
	if(PacketFilter & NDIS_PACKET_TYPE_802_11_PROMISCUOUS_CTRL)	{RT_TRACE(COMP_OID_SET, DBG_LOUD, ("NDIS_PACKET_TYPE_802_11_PROMISCUOUS_CTRL\n"));}

	if( (PacketFilter & NIC_SUPPORTED_FILTERS) == 0 )
	{
		ndisStatus=NDIS_STATUS_INVALID_DATA; 
		return ndisStatus;
	}
			
	if( pTargetAdapter->pNdisCommon->NdisPacketFilter == PacketFilter )
	{
		ndisStatus = NDIS_STATUS_SUCCESS;
		return ndisStatus;
	}

	if( PacketFilter & 
		(NDIS_PACKET_TYPE_ALL_MULTICAST |NDIS_PACKET_TYPE_PROMISCUOUS )
	)
	{
		u4Byte	MCR[2];
		MCR[0] = 0xffffffff;
		MCR[1] = 0xffffffff;
		pTargetAdapter->HalFunc.SetHwRegHandler(pTargetAdapter, HW_VAR_MULTICAST_REG, (pu1Byte)(MCR) );
	}

	{
		PMGNT_INFO				pMgntInfo = &(pTargetAdapter->MgntInfo);	
		pTargetAdapter->pNdisCommon->NdisPacketFilter = PacketFilter| 
														pMgntInfo->RegPktIndicate;
	}
	
	return ndisStatus;
}


NDIS_STATUS	
N6C_SET_OID_DOT11_CURRENT_CHANNEL(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	u1Byte btChannel = 0;
	PMGNT_INFO pDefaultMgntInfo = GetDefaultMgntInfo(pTargetAdapter);
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);


	*BytesNeeded = sizeof(ULONG);
	if ( InformationBufferLength < *BytesNeeded )
	{
		ndisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
		return ndisStatus;
	}

	{
		//
		// Verify the channel given by the upper layer first.
		//
		u2Byte	i, listLen;
		PRT_CHANNEL_LIST	pChannelList = NULL;
		u1Byte	chnlNum;
		BOOLEAN bFound = FALSE;
		btChannel = (u1Byte)(*(PULONG)InformationBuffer);

		pChannelList = MgntActQuery_ChannelList(pTargetAdapter);

		if(!pChannelList) // Get channel list failed
			return NDIS_STATUS_FAILURE;

		listLen = pChannelList->ChannelLen;

		for(i=0; i<listLen; i++)
		{
			chnlNum = pChannelList->ChnlListEntry[i].ChannelNum;
			if( (btChannel == chnlNum) || (btChannel == DSSS_Freq_Channel[chnlNum]*1000) )
			{
				RT_ASSERT(chnlNum > 0, ("ToLegalChannel(): chnlNum(%d) should > 0 !!!\n", chnlNum));
				bFound = TRUE;
			}
		}

		if(!bFound)
		{
			ndisStatus = NDIS_STATUS_INVALID_DATA;
			return ndisStatus;
		}
					
		if (MgntActSet_802_11_REG_20MHZ_CHANNEL_AND_SWITCH(pTargetAdapter, (u1Byte)(*(PULONG)InformationBuffer)) != TRUE)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("!!! Failed to switch to the channel, %d\n", btChannel));
		}
	}

	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_DOT11_DISCONNECT_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	u4Byte 			id = 0;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);


	//
	// <Roger_Notes> We should reset PHY ready status if WiFi status was changed for this PHY ID, 
	// Using uNumOfEntries for Dot11SupportedPhyTypes instead, added by Roger, 2013.08.29.
	// 
	for (id = 0; id < pTargetAdapter->pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries; ++id) 
		pTargetAdapter->pNdis62Common->bIsDot11PhyIdSet[id] = FALSE;
	pTargetAdapter->pNdis62Common->bDot11SetPhyIdReady = TRUE;

	pTargetAdapter->pNdis62Common->CurrentOpState = INIT_STATE;


	ndisStatus = N6CSet_DOT11_DISCONNECT_REQUEST(
			pTargetAdapter,
			InformationBuffer,
			InformationBufferLength,
			BytesRead,
			BytesNeeded
		);

	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_DOT11_CONNECT_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_MLME);

	pTargetAdapter->pNdis62Common->CurrentOpState = OP_STATE;

	ndisStatus = N6CSet_DOT11_CONNECT_REQUEST(
			pTargetAdapter,
			InformationBuffer,
			InformationBufferLength,
			BytesRead,
			BytesNeeded
		);

	return ndisStatus;
}


NDIS_STATUS	
N6C_SET_OID_GEN_LINK_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_MLME);
		
	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_DOT11_SAFE_MODE_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 		ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_NDIS6_COMMON	pNdis6Common = pTargetAdapter->pNdisCommon;
	PMGNT_INFO      pMgntInfo = &pTargetAdapter->MgntInfo;

	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_MLME);

#if 1
	pMgntInfo->SafeModeEnabled = *(PBOOLEAN)InformationBuffer;

	if(pMgntInfo->SafeModeEnabled)
		pMgntInfo->pStaQos->QosCapability = QOS_DISABLE;
	else
		pMgntInfo->pStaQos->QosCapability = pMgntInfo->pStaQos->QosCapabilityBackup;
#else
	pMgntInfo->SafeModeEnabled = FALSE;
#endif
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SafeModeEnabled %d\n", pMgntInfo->SafeModeEnabled));
	return ndisStatus;
}


NDIS_STATUS	
N6C_SET_OID_DOT11_CURRENT_PHY_ID(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG PhyId = *(PULONG)InformationBuffer;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_MLME);

	*BytesNeeded = sizeof(ULONG);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	if ( PhyId >= pTargetAdapter->pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries )
	{
		*BytesRead = 0;
		return NDIS_STATUS_INVALID_DATA;
	}
	else
	{
		pTargetAdapter->pNdisCommon->dot11SelectedPhyId = PhyId;
		pTargetAdapter->pNdisCommon->pDot11SelectedPhyMIB = pTargetAdapter->pNdisCommon->pDot11PhyMIBs + PhyId;
		*BytesRead = *BytesNeeded;
	}

	// If PHY IDs are not already set  for all number of entries we indicate before
	
	pTargetAdapter->pNdis62Common->bIsDot11PhyIdSet[PhyId] = TRUE;

	if (PhyId == 0) {
		u4Byte id = 0;
		for (id = 1; id < pTargetAdapter->pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries; id++)
			pTargetAdapter->pNdis62Common->bIsDot11PhyIdSet[id] = FALSE;
		pTargetAdapter->pNdis62Common->bDot11SetPhyIdReady = FALSE;
	} else if (PhyId == pTargetAdapter->pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries - 1) {
		pTargetAdapter->pNdis62Common->bDot11SetPhyIdReady = TRUE;
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_DOT11_CURRENT_PHY_ID: phyid%d\n", pTargetAdapter->pNdisCommon->dot11SelectedPhyId));

	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_PNP_ENABLE_WAKE_UP(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG PhyId = *(PULONG)InformationBuffer;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	// NDIS notify us we shall set up HW to support WOL feature.
	// Since we will schedule workitem in OID_PNP_SET_POWER, all HW setting 
	// will be done there. 2006.09.19, by rcnjko.
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set OID_PNP_ENABLE_WAKE_UP(%#X)\n", *((PULONG)InformationBuffer)));
	
	return ndisStatus;
}


NDIS_STATUS	
N6C_SET_OID_PNP_ADD_WAKE_UP_PATTERN(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	NDIS_PM_PACKET_PATTERN PMPacketPattern = *((PNDIS_PM_PACKET_PATTERN)InformationBuffer);	
				
	u1Byte	WoLBitMapPatternMask[MAX_WOL_BIT_MASK_SIZE];
	u1Byte	WoLBitMapPatternContent[MAX_WOL_PATTERN_SIZE];
	u1Byte	Index;
	PMGNT_INFO				pMgntInfo = &(pTargetAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

				
	PlatformZeroMemory(WoLBitMapPatternMask, sizeof(WoLBitMapPatternMask));
	PlatformZeroMemory(WoLBitMapPatternContent, sizeof(WoLBitMapPatternContent));


	FunctionIn(COMP_OID_SET);
	if(PMPacketPattern.MaskSize <= MAX_WOL_BIT_MASK_SIZE)
	{
		PlatformMoveMemory(WoLBitMapPatternMask, (PNDIS_PM_PACKET_PATTERN)InformationBuffer+1, PMPacketPattern.MaskSize);						
		RT_PRINT_DATA( (COMP_OID_SET|COMP_POWER), DBG_LOUD, ("SET OID_PNP_ADD_WAKE_UP_PATTERN Mask: "), 
		WoLBitMapPatternMask, PMPacketPattern.MaskSize);
	}	

	if(PMPacketPattern.PatternSize <= MAX_WOL_PATTERN_SIZE)
	{
		PlatformMoveMemory(WoLBitMapPatternContent, (pu1Byte)InformationBuffer+PMPacketPattern.PatternOffset, PMPacketPattern.PatternSize);
		RT_PRINT_DATA( (COMP_OID_SET|COMP_POWER), DBG_LOUD, ("SET OID_PNP_ADD_WAKE_UP_PATTERN Pattern: "), 
		WoLBitMapPatternContent, PMPacketPattern.PatternSize);
	}
			
	//Find the index of the first empty entry.
	for(Index=0; Index<MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter); Index++)
	{
		if(pPmWoLPatternInfo[Index].PatternId == PMPacketPattern.PatternSize)
		{
			Index = MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter);
			break;
		}

		if(pPmWoLPatternInfo[Index].PatternId == 0)
			break;
	}
				
	if(Index >= MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter))
	{
		ndisStatus = NDIS_STATUS_RESOURCES;
		RT_TRACE(COMP_OID_SET, DBG_LOUD,
			("SET OID_PNP_ADD_WAKE_UP_PATTERN: Return status(%#X). The number of wake up pattern is more than %d or the pattern Id is exist.\n", 
			ndisStatus, MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter)));
		return ndisStatus;

	}
	else
	{
		u1Byte	BROADCAST_ADDR[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		u1Byte	DONTCARE_ADDR[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
		u1Byte	MULTICAST_ADDR[2]={0x33, 0x33};

		pPmWoLPatternInfo[Index].PatternType = eUnknownType;
		//packet type.
		if(PlatformCompareMemory(WoLBitMapPatternContent, pTargetAdapter->PermanentAddress, sizeof(pTargetAdapter->PermanentAddress)) == 0 )
		{	//unicast
			pPmWoLPatternInfo[Index].PatternType = eUnicastPattern;
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
		//DbgPrint("PatternType = %d\n", pPmWoLPatternInfo[Index].PatternType);
					
		// We use the PatternSize to replace PatternId because N5/N6 does not content PatternId.
		pPmWoLPatternInfo[Index].PatternId = PMPacketPattern.PatternSize;
		pPmWoLPatternInfo[Index].IsUserDefined = 0;

		if(pPmWoLPatternInfo[Index].PatternType == eUnicastPattern) 		
		{ // To wake up by any packet which DA is our MAC addr.
			BOOLEAN bUWF = TRUE;
						
			//pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_WF_IS_MAC_ADDR, (pu1Byte)(&bUWF)); 
			RT_TRACE(COMP_OID_SET, DBG_LOUD,
				("OID_PNP_ADD_WAKE_UP_PATTERN: Set HW to wake up for any packet which DA is our MAC addr.\n"));
		}
		else
		{
			GetWOLWakeUpPattern(pTargetAdapter, 
					(pu1Byte)&WoLBitMapPatternMask, 
					PMPacketPattern.MaskSize, 
					(pu1Byte)&WoLBitMapPatternContent, 
					PMPacketPattern.PatternSize,
					Index,
					FALSE);
						
				pPSC->WoLPatternNum++;	
		}
	}

	RT_PRINT_DATA( (COMP_OID_SET|COMP_POWER), DBG_TRACE, ("SET OID_PNP_ADD_WAKE_UP_PATTERN: "), InformationBuffer , InformationBufferLength);
		
	return ndisStatus;
}

NDIS_STATUS	
N6C_SET_OID_PNP_REMOVE_WAKE_UP_PATTERN(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	NDIS_PM_PACKET_PATTERN PMPacketPattern = *((PNDIS_PM_PACKET_PATTERN)InformationBuffer);
	u1Byte	Index;
	PMGNT_INFO				pMgntInfo = &(pTargetAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	for(Index=0; Index<MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter); Index++)
	{
		if(pPmWoLPatternInfo[Index].PatternId == PMPacketPattern.PatternSize)
			break;
	}

	if(Index >= MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter))
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD,("SET OID_PM_REMOVE_WOL_PATTERN: Cannot find the wake up pattern size(%08X).\n", PMPacketPattern.PatternSize));
	}
	else
	{
		//Reset the structure and set WFCRC register to non-zero value.
		pPmWoLPatternInfo[Index].PatternId = 0;
		PlatformZeroMemory(pPmWoLPatternInfo[Index].Mask, sizeof(pPmWoLPatternInfo[Index].Mask));
		pPmWoLPatternInfo[Index].CrcRemainder = 0xffff;
		pPmWoLPatternInfo[Index].IsPatternMatch = 0;
		pPmWoLPatternInfo[Index].IsSupportedByFW = 0;
		
		pTargetAdapter->HalFunc.SetHwRegHandler(pTargetAdapter, HW_VAR_WF_MASK, (pu1Byte)(&Index)); 
		pTargetAdapter->HalFunc.SetHwRegHandler(pTargetAdapter, HW_VAR_WF_CRC, (pu1Byte)(&Index)); 

		pPmWoLPatternInfo[Index].HwWFMIndex = 0xff; // reset the value after clear HW/CAM entry.
		
		if(PMPacketPattern.PatternSize == 0x1)
		{
			BOOLEAN 	bUWF=FALSE;
			pTargetAdapter->HalFunc.SetHwRegHandler(pTargetAdapter, HW_VAR_WF_IS_MAC_ADDR, (pu1Byte)(&bUWF)); 
		}
		else
		{
			pPSC->WoLPatternNum--;
		}
	}

	RT_PRINT_DATA( (COMP_OID_QUERY|COMP_POWER), DBG_LOUD, ("SET OID_PNP_REMOVE_WAKE_UP_PATTERN: "), InformationBuffer , InformationBufferLength);
		
	return ndisStatus;
}


NDIS_STATUS	
N6C_SET_OID_PNP_SET_POWER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

		ndisStatus = N6Sdio_Mgnt_SetPower(
	            		pTargetAdapter,
	            		InformationBuffer,
	            		InformationBufferLength,
	            		BytesNeeded,
	            		BytesRead
	            );
		
	return ndisStatus;
}
