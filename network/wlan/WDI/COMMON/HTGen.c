
#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "HTGen.tmh"
#endif

u1Byte MCS_FILTER_ALL[16] = {	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
									0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	};

u1Byte MCS_FILTER_1SS[16] = {	0xff, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
									0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	};

const u2Byte MCS_DATA_RATE[2][2][33] = 
	{	{	{	13, 26, 39, 52, 78, 104, 117, 130, 
				26, 52, 78 ,104, 156, 208, 234, 260,
				39, 78, 117, 156, 234, 312, 351, 390, 
				52, 104, 156, 208, 312, 416, 468, 520, 0},			// Long GI, 20MHz
			{	14, 29, 43, 58, 87, 116, 130, 144, 
				29, 58, 87, 116, 173, 231, 260, 289, 
				43, 87, 130, 173, 260, 347, 390, 433, 
				58, 116, 173, 231, 347, 462, 520, 578, 0}	},		// Short GI, 20MHz
		{	{	27, 54, 81, 108, 162, 216, 243, 270, 
				54, 108, 162, 216, 324, 432, 486, 540, 
				81, 162, 243, 324, 486, 648, 729, 810, 
				108, 216, 324, 432, 648, 864, 972, 1080, 12}, 	// Long GI, 40MHz
			{	30, 60, 90, 120, 180, 240, 270, 300, 
				60, 120, 180, 240, 360, 480, 540, 600, 
				90, 180, 270, 360, 540, 720, 810, 900, 
				120, 240, 360, 480, 720, 960, 1080, 1200, 13}	}	// Short GI, 40MHz
	};



//static u1Byte UNKNOWN_BORADCOM[3] = {0x00, 0x14, 0xbf};
//static u1Byte LINKSYSWRT330_LINKSYSWRT300_BROADCOM[3] = {0x00, 0x1a, 0x70};
//static u1Byte LINKSYSWRT350_LINKSYSWRT150_BROADCOM[3] = {0x00, 0x1d, 0x7e};
//static u1Byte NETGEAR834Bv2_BROADCOM[3] = {0x00, 0x1b, 0x2f};
//static u1Byte BELKINF5D8233V1_RALINK[3] = {0x00, 0x17, 0x3f};	//cosa 03202008
//static u1Byte BELKINF5D82334V3_RALINK[3] = {0x00, 0x1c, 0xdf};
//static u1Byte PCI_RALINK[3] = {0x00, 0x90, 0xcc};
//static u1Byte EDIMAX_RALINK[3] = {0x00, 0x0e, 0x2e};
//static u1Byte AIRLINK_RALINK[3] = {0x00, 0x18, 0x02};
//static u1Byte DLINK_ATHEROS_1[3] = {0x00, 0x1c, 0xf0};
//static u1Byte DLINK_ATHEROS_2[3] = {0x00, 0x21, 0x91};
//static u1Byte CISCO_BROADCOM[3] = {0x00, 0x17, 0x94};
static u1Byte NETGEAR_BROADCOM[3] = {0x00, 0x1f, 0x33};
//static u1Byte NETGEAR_CONEXANT[3] = {0x00, 0x1f, 0x33};

// 2008/04/01 MH For Cisco G mode RX TP We need to change FW duration. Shoud we put the
// code in other place??
//static u1Byte WIFI_CISCO_G_AP[3] = {0x00, 0x40, 0x96};

/**
* Function:	HTDebugHTCapability
* 
* Overview:	Print out each field on HT capability IE in(Beacon/ProbeRsp/AssocReq/
*			AssocRsp)
* 
* Input:	
*			u4Byte			DebugLevel
*			PADAPTER		Adapter,
*			POCTET_STRING	pocCap,
*			pu1Byte			titleString
* 			
* Output:	None
* Return:     	None
* Note: 		Driver should not print out this message by default
*/
VOID
HTDebugHTCapability(
	u4Byte			DebugLevel,
	PADAPTER		Adapter,
	POCTET_STRING	pocCap,
	pu1Byte			TitleString
	
)
{
	
	static u1Byte	EWC11NHTCap[] = {0x00, 0x90, 0x4c, 0x33};	// For 11n EWC definition, 2007.07.17, by Emily
	pu1Byte 		pCapELE;
	pu1Byte			pMCS;
	
	if(!PlatformCompareMemory(pocCap->Octet, EWC11NHTCap, sizeof(EWC11NHTCap)))
	{
		// Not EWC IE
		pCapELE = (pu1Byte)&pocCap->Octet[4];		
	}else
		pCapELE = (pu1Byte)&pocCap->Octet[0];		
	
	pMCS= GET_HT_CAPABILITY_ELE_MCS(pCapELE);
	
	RT_TRACE(COMP_HT, DebugLevel, ("<Log HT Capability>. Called by %s\n", TitleString ));

	RT_TRACE(COMP_HT, DebugLevel, ("\tSupported Channel Width = %s\n", (GET_HT_CAPABILITY_ELE_CHL_WIDTH(pCapELE) )?"20MHz": "20/40MHz"));
	RT_TRACE(COMP_HT, DebugLevel, ("\tSupport Short GI for 20M = %s\n", (GET_HT_CAPABILITY_ELE_SHORT_GI20M(pCapELE))?"YES": "NO"));
	RT_TRACE(COMP_HT, DebugLevel, ("\tSupport Short GI for 40M = %s\n", (GET_HT_CAPABILITY_ELE_SHORT_GI40M(pCapELE))?"YES": "NO"));
	RT_TRACE(COMP_HT, DebugLevel, ("\tSupport TX STBC = %s\n", (GET_HT_CAPABILITY_ELE_TX_STBC(pCapELE))?"YES": "NO"));
	RT_TRACE(COMP_HT, DebugLevel, ("\tMax AMSDU Size = %s\n", (GET_HT_CAPABILITY_ELE_MAX_AMSDU_SIZE(pCapELE))?"3839": "7935"));
	RT_TRACE(COMP_HT, DebugLevel, ("\tSupport CCK in 20/40 mode = %s\n", (GET_HT_CAPABILITY_ELE_DSS_CCK(pCapELE))?"YES": "NO"));
	RT_TRACE(COMP_HT, DebugLevel, ("\tMax AMPDU Factor = %d\n", GET_HT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(pCapELE)));
	RT_TRACE(COMP_HT, DebugLevel, ("\tMPDU Density = %d\n", GET_HT_CAPABILITY_ELE_MPDU_DENSITY(pCapELE)));
	//RT_TRACE(COMP_HT, DebugLevel, ("\tMCS Rate Set = [%x][%x][%x][%x][%x]\n", pMCS[0],\
		//		pMCS[1], pMCS[2], pMCS[3], pMCS[4]));
	RT_TRACE(COMP_HT, DebugLevel, ("\n"));		
		
}

/**
* Function:	HTDebugHTInfo
* 
* Overview:	Print out each field on HT Information IE in(Beacon/PorbeRsp)
* 
* Input:		
*			u4Byte			DebugLevel,
*			PADAPTER		Adapter,
*			POCTET_STRING	pocCap,
*			pu1Byte			TitleString
* 			
* Output:	None
* Return:     	None
* Note: 		Driver should not print out this message by default
*/
VOID
HTDebugHTInfo(
	u4Byte			DebugLevel,
	PADAPTER		Adapter,
	POCTET_STRING	pocInfo,
	pu1Byte			TitleString
)
{
	
	static u1Byte	EWC11NHTInfo[] = {0x00, 0x90, 0x4c, 0x34};	// For 11n EWC definition, 2007.07.17, by Emily
	pu1Byte		pHTInfoEle, pBasicMCS;
	
	if(!PlatformCompareMemory(pocInfo->Octet, EWC11NHTInfo, sizeof(EWC11NHTInfo)))
		pHTInfoEle = (pu1Byte)(&pocInfo->Octet[4]);					// Not EWC IE
	else
		pHTInfoEle = (pu1Byte)(&pocInfo->Octet[0]);
	
		
	pBasicMCS= GET_HT_INFO_ELE_BASIC_MCS(pHTInfoEle);
			
	RT_TRACE(COMP_HT, DebugLevel, ("<Log HT Information Element>. Called by %s\n", TitleString ));

	RT_TRACE(COMP_HT, DebugLevel, ("\tPrimary channel = %d\n", GET_HT_INFO_ELE_CONTROL_CHL(pHTInfoEle)));
	RT_TRACE(COMP_HT, DebugLevel, ("\tSenondary channel ="));
	switch(GET_HT_INFO_ELE_EXT_CHL_OFFSET(pHTInfoEle))
	{
		case 0:
			RT_TRACE(COMP_HT, DebugLevel, ("Not Present\n"));		
			break;
		case 1:
			RT_TRACE(COMP_HT, DebugLevel, ("Upper channel\n"));
			break;
		case 2:
			RT_TRACE(COMP_HT, DebugLevel, ("Reserved. Eooro!!!\n"));		
			break;
		case 3:
			RT_TRACE(COMP_HT, DebugLevel, ("Lower Channel\n"));		
			break;
	}
	RT_TRACE(COMP_HT, DebugLevel, ("\tRecommended channel width = %s\n", (GET_HT_INFO_ELE_STA_CHL_WIDTH(pHTInfoEle))?"20Mhz": "40Mhz"));

	RT_TRACE(COMP_HT, DebugLevel, ("\tOperation mode for protection = "));
	switch(GET_HT_INFO_ELE_OPT_MODE(pHTInfoEle))
	{
		case 0:
			RT_TRACE(COMP_HT, DebugLevel, ("No Protection\n"));		
			break;
		case 1:
			RT_TRACE(COMP_HT, DebugLevel, ("HT non-member protection mode\n"));
			break;
		case 2:
			RT_TRACE(COMP_HT, DebugLevel, ("Suggest to open protection\n"));		
			break;
		case 3:
			RT_TRACE(COMP_HT, DebugLevel, ("HT mixed mode\n"));		
			break;
	}

	//RT_TRACE(COMP_HT, DebugLevel, ("\tBasic MCS Rate Set = [%x][%x][%x][%x][%x]\n", pBasicMCS[0],\
		//		pBasicMCS[1], pBasicMCS[2], pBasicMCS[3], pBasicMCS[4]));

	RT_TRACE(COMP_HT, DebugLevel, ("\n"));		
}


u2Byte
HTMcsToDataRate(
	PADAPTER		Adapter,
	u2Byte			nMcsRate
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	u1Byte 					BandWidth;
	
	u1Byte	isShortGI = CHNL_RUN_ABOVE_40MHZ(pMgntInfo)?
						((pHTInfo->bCurShortGI40MHz)?1:0):
						((pHTInfo->bCurShortGI20MHz)?1:0);

	nMcsRate -= MGN_MCS0;

	BandWidth = CHNL_RUN_ABOVE_40MHZ(pMgntInfo)?1:0;

	// Prefast warning ignore
#pragma warning( disable:6385 )
	RT_TRACE_F(COMP_HT, DBG_LOUD, ("MCS_DATA_RATE[%d][%d][(%d&0x3f)] = %d\n", 
		BandWidth, isShortGI, nMcsRate, MCS_DATA_RATE[BandWidth][isShortGI][(nMcsRate&0x3f)]));
						
	return MCS_DATA_RATE[BandWidth][isShortGI][nMcsRate];
}

u2Byte
HTPeerMcsToDataRate(
	PADAPTER		Adapter,
	PRT_WLAN_STA	pEntry,
	u1Byte			nMcsRate
	)
{
	u1Byte	is40MHz;
	u1Byte	isShortGI;

	if(pEntry!=NULL)
	{
		nMcsRate -= MGN_MCS0;
		is40MHz = (pEntry->BandWidth >= CHANNEL_WIDTH_40)? TRUE: FALSE;
		isShortGI = (is40MHz)?pEntry->HTInfo.bShortGI40M:pEntry->HTInfo.bShortGI20M;
		return MCS_DATA_RATE[is40MHz][isShortGI][nMcsRate];
	}
	else
		return HTMcsToDataRate(Adapter, nMcsRate);
}


VOID
HTSetCCKSupport(
	PADAPTER		Adapter,
	BOOLEAN			bStarter,	
	pu1Byte			pPeerHTCap
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);

	if(pMgntInfo->dot11CurrentChannelBandWidth == CHANNEL_WIDTH_80)
		pHTInfo->bCurSuppCCK = FALSE;
	else if(bStarter)	// AP mode or IBSS starter
	{
		// Set in HTUseDefaultSettingFromReg/HTUseDefaultSettingFromDefault
	}
	else				// Infrastuction client or IBSS joiner
	{
		if(pPeerHTCap != NULL)
			pHTInfo->bCurSuppCCK = pHTInfo->bRegSuppCCK?GET_HT_CAPABILITY_ELE_DSS_CCK(pPeerHTCap):FALSE;
		else
			pHTInfo->bCurSuppCCK = pHTInfo->bRegSuppCCK;
	}	
}

VOID
IOTPeerDetermine(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	PADAPTER	DefAdapter = GetDefaultAdapter(Adapter);

	// [For Win7 Two Port] 2009.07.30, by Bohn
	if(ACTING_AS_AP(Adapter))
	{
		if(Adapter->MgntInfo.NdisVersion < RT_NDIS_VERSION_6_20)
			pMgntInfo->IOTPeer = HT_IOT_PEER_SELF_SOFTAP;
		else if(Adapter->MgntInfo.NdisVersion >= RT_NDIS_VERSION_6_20)
		{
			if(DefAdapter->MgntInfo.mAssoc==TRUE)
				pMgntInfo->IOTPeer = DefAdapter->MgntInfo.IOTPeer;
			else
				pMgntInfo->IOTPeer = HT_IOT_PEER_SELF_SOFTAP;
		}
	}
	else
	{
		pMgntInfo->IOTPeer = pBssDesc->Vender;
		pMgntInfo->IOTPeerSubtype  = pBssDesc->SubTypeOfVender;


		// 2011/11/28 MH We need to disable TXOP for the AP to preven serious collision.
		if (pBssDesc->SubTypeOfVender == HT_IOT_PEER_BROADCOM_NETGEAR_WNDAP4500)
		{
			pMgntInfo->IOTAction |=HT_IOT_ACT_DISABLE_AC_TXOP;			
		}
		else
		{
			pMgntInfo->IOTAction &= ~HT_IOT_ACT_DISABLE_AC_TXOP;
		}

		//
		// Check MPDU maximum size 4/8/11K for VHT mode.~ VHT does not have the function now,
		//
		if (pBssDesc->BssVHT.bdVHTCapBuf)
		{
			pMgntInfo->IOTMaxMpduLen = GET_VHT_CAPABILITY_ELE_MAX_MPDU_LENGTH(pBssDesc->BssVHT.bdVHTCapBuf);
			pMgntInfo->IOTVhtAmsduSupport = TRUE;
			//pMgntInfo->IOTMaxMpduLen = GET_VHT_CAPABILITY_ELE_MAX_MPDU_LENGTH(pBssDesc->BssVHT.bdVHTOperBuf);
			//DbgPrint("pMgntInfo->IOTMaxMpduLen = %d\n", pMgntInfo->IOTMaxMpduLen);
		}
		else
		{
			pMgntInfo->IOTMaxMpduLen = 0;
			pMgntInfo->IOTVhtAmsduSupport = FALSE;
		}
		
	}
}




/**
* Function:	HTIOTActIsDisableMCSTwoSpatialStream
* 
* Overview:	Check whether driver should declare capability of receving All 2 ss packets
*			
* Input:	
*			PADAPTER		Adapter,
* 			
* Output:		None
* Return:     	TRUE if driver should disable all two spatial stream packet
* 2008.04.21	Emily
*/
BOOLEAN
HTIOTActIsDisableMCSTwoSpatialStream(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
)
{
	BOOLEAN		retValue = FALSE;

	// Apply for 819u only

	if(	(Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP104) || 
		(Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP40)  ||
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104) ||
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40) || 
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_TKIP)	)
	{
		if( (pBssDesc->Vender != HT_IOT_PEER_ATHEROS) &&
		    (pBssDesc->Vender != HT_IOT_PEER_UNKNOWN) &&
		    (pBssDesc->Vender != HT_IOT_PEER_MARVELL) && 
		    (pBssDesc->Vender != HT_IOT_PEER_REALTEK_92SE) &&
		    (pBssDesc->Vender != HT_IOT_PEER_RALINK)	)
			retValue = TRUE;
	}
	
	return retValue;
}





BOOLEAN
HTIOTActIsForcedCTS2Self(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
)
{
	BOOLEAN	retValue = FALSE;

	// Wifi Test 5.2.35 Basic Association in 802.11n Enviroment
	if(Adapter->MgntInfo.bWiFiConfg)
		retValue = FALSE;
	else if(pBssDesc->Vender == HT_IOT_PEER_MARVELL || pBssDesc->Vender == HT_IOT_PEER_ATHEROS)
		retValue = TRUE;
	
	return retValue;
}


/*
  *  EDCA parameters bias on downlink
  */
BOOLEAN
HTIOTActIsEDCABiasRx(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
	)
{
	BOOLEAN	retValue = FALSE;

	if(HAL_IsEdcaBiasRx(Adapter, pBssDesc))
		retValue = TRUE;
	
	return retValue;
}


BOOLEAN
HTIOTActDisableHighPower(
	PADAPTER		Adapter,
  	PRT_WLAN_BSS	pBssDesc
)
{
	BOOLEAN	retValue = FALSE;

	if(pBssDesc->Vender==HT_IOT_PEER_RALINK ||
		pBssDesc->Vender==HT_IOT_PEER_REALTEK ||
		pBssDesc->Vender==HT_IOT_PEER_REALTEK_92SE)
	{
		retValue = TRUE;
	}

	return retValue;
}
	

BOOLEAN
HTIOTActIsDisableTx40MHz(
	PADAPTER		Adapter,
  	PRT_WLAN_BSS	pBssDesc
)
{
	BOOLEAN	retValue = FALSE;

	if(	(Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP104) || 
		(Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP40)  ||
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104) ||
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40) || 
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_TKIP)	)
	{
		if((pBssDesc->Vender==HT_IOT_PEER_REALTEK) && (pBssDesc->BssHT.bdSupportHT))
			retValue = TRUE;
	}

	return retValue;
}

BOOLEAN
HTIOTActIsTxNoAggregation(
	PADAPTER		Adapter,
  	PRT_WLAN_BSS	pBssDesc
)
{
	BOOLEAN	retValue = FALSE;

	if(	(Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP104) || 
		(Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP40)  ||
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104) ||
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40) || 
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_TKIP)	)
	{
		if(pBssDesc->Vender==HT_IOT_PEER_REALTEK)
			retValue = TRUE;
	}

	return retValue;
}

BOOLEAN
HTIOTActIsDisableTx2SS(
	PADAPTER		Adapter,
  	PRT_WLAN_BSS	pBssDesc
)
{
	BOOLEAN	retValue = FALSE;

	if(	(Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP104) || 
		(Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP40)  ||
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104) ||
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40) || 
		(Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_TKIP)	)
	{
		if((pBssDesc->Vender==HT_IOT_PEER_REALTEK) && (pBssDesc->BssHT.bdSupportHT))
			retValue = TRUE;
	}

	return retValue;
}



BOOLEAN
HTIOCActIsDisableCckRate(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
)
{
	BOOLEAN	retValue = FALSE;
	PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;

	if(pMgntInfo->bDisableCck)
	{
		retValue = TRUE;
	}
	
	return retValue;

}

BOOLEAN
HTIOTActDisableRx40MHzShortGI(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
)
{
	return FALSE;
}

BOOLEAN
HTIOTActDisableRx20MHzShortGI(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pBssDesc
)
{
	return FALSE;
}


VOID
ResetIOTSetting(
	PMGNT_INFO		pMgntInfo
)
{
	pMgntInfo->IOTAction = 0;
	pMgntInfo->IOTPeer = HT_IOT_PEER_UNKNOWN;
}


/*
	The setting used in this functoin shall be our HT Rx capability that we want to inform the peer STA.
	Both AP and STA Attach the Capailbity Element
	AP/STA should include this element only if HTEnable is turned on.
*/
VOID
HTConstructCapabilityElement(
	PADAPTER		Adapter,
	POCTET_STRING	posHTCap,
	BOOLEAN			bAssoc
)
{	
	PMGNT_INFO      			pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	pu1Byte					pCapELE = NULL, pCapMCS;
	CHANNEL_WIDTH			ChnlWidth;	
	u1Byte					HtAggrSize = HT_AGG_SIZE_64K;
	u1Byte					HtDensity = HT_DENSITY_NO_RESTRICTION;
	u1Byte					HtSTBC = 0;

	PlatformZeroMemory(posHTCap->Octet, posHTCap->Length);
	
	if(bAssoc && pHTInfo->ePeerHTSpecVer == HT_SPEC_VER_EWC)
	{
		u1Byte	EWC11NHTCap[] = {0x00, 0x90, 0x4c, 0x33};	// For 11n EWC definition, 2007.07.17, by Emily
		PlatformMoveMemory(posHTCap->Octet, EWC11NHTCap, sizeof(EWC11NHTCap));
		pCapELE = (pu1Byte)&posHTCap->Octet[4];
		posHTCap->Length = 30;
	}
	else 
	{
		pCapELE = (pu1Byte)posHTCap->Octet;
		posHTCap->Length = 26;
	}

	// B0 
	SET_HT_CAPABILITY_ELE_LDPC_CAP(pCapELE, (pHTInfo->HtLdpcCap & LDPC_HT_ENABLE_RX) ? 1 : 0);
	
	ChnlWidth = (CHANNEL_WIDTH)AP_CheckBwWidth(Adapter);
	
	if(ACTING_AS_AP(Adapter) == FALSE && ACTING_AS_IBSS(Adapter) == FALSE)
	{
		// Determine peer capability for 40MHz bandwidth support when using STA mode.
		// This prevents from some retarded AP from bandwidth configuration error. -- 20110530 Joseph
		if(bAssoc && pHTInfo->bCurrentHTSupport)
		{
			if(GET_HT_CAPABILITY_ELE_CHL_WIDTH(pHTInfo->PeerHTCapBuf) == 0)
				ChnlWidth = CHANNEL_WIDTH_20;
		}
		
		ChnlWidth = CHNL_CheckChnlPlanWithBW(Adapter,pMgntInfo->dot11CurrentChannelNumber,ChnlWidth,pMgntInfo->ChannelOffset);
	}


	if(ChnlWidth == CHANNEL_WIDTH_40 || ChnlWidth == CHANNEL_WIDTH_80)
	{
		SET_HT_CAPABILITY_ELE_CHL_WIDTH(pCapELE, 1);
	}	
	else
	{
		SET_HT_CAPABILITY_ELE_CHL_WIDTH(pCapELE, 0);
	}

	// B2 B3
	SET_HT_CAPABILITY_ELE_MIMO_PWRSAVE(pCapELE, pHTInfo->SelfMimoPs);
	// B4
	SET_HT_CAPABILITY_ELE_GREEN_FIELD(pCapELE, 0); // This feature is not supported now!!
	// B5
	SET_HT_CAPABILITY_ELE_SHORT_GI20M(pCapELE, pHTInfo->bRegShortGI20MHz ? 1 : 0); // We can receive Short GI!!
	// B6
	SET_HT_CAPABILITY_ELE_SHORT_GI40M(pCapELE, pHTInfo->bRegShortGI40MHz ? 1 : 0); // We can receive Short GI!!

	// BIT7 STBC
	if(TEST_FLAG(pHTInfo->HtStbcCap, STBC_HT_ENABLE_TX))
	{
		SET_HT_CAPABILITY_ELE_TX_STBC(pCapELE, 1);
	}
	else
	{
		SET_HT_CAPABILITY_ELE_TX_STBC(pCapELE, 0);
	}
	// B8 B9
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_RX_STBC, (pu1Byte)&HtSTBC);
	if(TEST_FLAG(pHTInfo->HtStbcCap, STBC_HT_ENABLE_RX))
	{
		SET_HT_CAPABILITY_ELE_RX_STBC(pCapELE, HtSTBC);
	}
	else
	{
		SET_HT_CAPABILITY_ELE_RX_STBC(pCapELE, 0);
	}
	
	// B10
	SET_HT_CAPABILITY_ELE_DELAY_BA(pCapELE, 0);	// Do not support now!!
	// B11
	{
		SET_HT_CAPABILITY_ELE_MAX_AMSDU_SIZE(pCapELE, ( (Adapter->MAX_RECEIVE_BUFFER_SIZE >= HT_AMSDU_SIZE_8K) && \
							(!PLATFORM_LIMITED_RX_BUF_SIZE(Adapter)))?1:0);
	}
							
	// B12
	if(IS_WIRELESS_MODE_5G(Adapter))
	{
		SET_HT_CAPABILITY_ELE_DSS_CCK(pCapELE, 0);
	}
	else
	{
		SET_HT_CAPABILITY_ELE_DSS_CCK(pCapELE,(ChnlWidth?(pHTInfo->bRegSuppCCK?1:0):0) );
	}


	SET_HT_CAPABILITY_ELE_PSMP(pCapELE, 0); // Do not support now!!
	// B14
	if(IS_WIRELESS_MODE_5G(Adapter))
	{
		SET_HT_CAPABILITY_ELE_FORTY_INTOLERANT(pCapELE, 0);
	}
	else
	{
		SET_HT_CAPABILITY_ELE_FORTY_INTOLERANT(pCapELE, pHTInfo->b40Intolerant);
	}
	// B15
	SET_HT_CAPABILITY_ELE_LSIG_TXOP_PROTECT(pCapELE, 0); // Do not support now!!

	//MAC HT parameters info
	// Retrieve HAL AMPDU Factor and AMPDU density parameter for each ICs. Added by Roger, 2011.12.08.
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_RX_AMPDU_FACTOR, (pu1Byte)&HtAggrSize);
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_MPDU_DENSITY, (pu1Byte)&HtDensity);

	// RX AMPDU factor and MPDU density
	SET_HT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(pCapELE, HtAggrSize);
	SET_HT_CAPABILITY_ELE_MPDU_DENSITY(pCapELE, HtDensity);	

	// Supported MCS set
	pCapMCS = GET_HT_CAPABILITY_ELE_MCS(pCapELE);
	PlatformMoveMemory(pCapMCS, pMgntInfo->Regdot11HTOperationalRateSet, 16);

	//Extended HT Capability Info
	SET_HT_CAPABILITY_ELE_EXT_HTCAPINFO(pCapELE, 0);

	//TXBF Capabilities
	SET_HT_CAPABILITY_ELE_TXBF_CAP(pCapELE, 0);

	// HT Beamformer
	if(TEST_FLAG(pHTInfo->HtBeamformCap, BEAMFORMING_HT_BEAMFORMER_ENABLE))
	{
		SET_HT_CAP_TXBF_TRANSMIT_NDP_CAP(pCapELE, 1);
		SET_HT_CAP_TXBF_EXPLICIT_COMP_STEERING_CAP(pCapELE, 1);
		SET_HT_CAP_TXBF_COMP_STEERING_NUM_ANTENNAS(pCapELE, 1);
		SET_HT_CAP_TXBF_CHNL_ESTIMATION_NUM_ANTENNAS(pCapELE, HAL_QueryBeamformerCap(Adapter));
	}
	
	// HT Beamformee
	if(TEST_FLAG(pHTInfo->HtBeamformCap, BEAMFORMING_HT_BEAMFORMEE_ENABLE))
	{
		SET_HT_CAP_TXBF_RECEIVE_NDP_CAP(pCapELE, 1);
		SET_HT_CAP_TXBF_EXPLICIT_COMP_FEEDBACK_CAP(pCapELE, 2);
		SET_HT_CAP_TXBF_COMP_STEERING_NUM_ANTENNAS(pCapELE, HAL_QueryBeamformeeCap(Adapter));
	}

	//Antenna Selection Capabilities
	SET_HT_CAPABILITY_ELE_AS_CAP(pCapELE, 0);

	if(bAssoc)
	{
		if(pMgntInfo->IOTAction & HT_IOT_ACT_DISABLE_ALL_2SS)
			pCapMCS[1] &= 0x00;

		// disable Rx 40MHz Short GI
		if(pMgntInfo->IOTAction & HT_IOT_ACT_DISABLE_RX_40MHZ_SHORT_GI)
			SET_HT_CAPABILITY_ELE_SHORT_GI40M(pCapELE, 0);

		if(pMgntInfo->IOTAction & HT_IOT_ACT_DISABLE_RX_20MHZ_SHORT_GI)
			SET_HT_CAPABILITY_ELE_SHORT_GI20M(pCapELE, 0);
	}

		
	RT_PRINT_DATA( COMP_HT, DBG_TRACE, "Construct HTCap in Beacon/Assoc/ReAssoc", posHTCap->Octet, posHTCap->Length);
	
	//Print each field in detail. Driver should not print out this message by default
	HTDebugHTCapability(DBG_TRACE, Adapter, posHTCap, (pu1Byte)"HTConstructCapability()");	
}

/*
	Only AP will attach the HT Information Element
*/

VOID
HTConstructInfoElement(
	PADAPTER		Adapter,
	POCTET_STRING	posHTInfo
)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	pu1Byte					pHTInfoEle = (pu1Byte)(posHTInfo->Octet);
	PRT_CHANNEL_INFO		pChnlInfo = GET_CHNL_INFO(pMgntInfo);
	
	PlatformZeroMemory(posHTInfo->Octet, posHTInfo->Length);

	if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
	{
	
		if((GetDefaultAdapter(Adapter)->MgntInfo.bForceGoTxData20MBw == TRUE) && IsFirstGoAdapter(Adapter)){
			SET_HT_INFO_ELE_STA_CHL_WIDTH(pHTInfoEle, FALSE);
			SET_HT_INFO_ELE_EXT_CHL_OFFSET(pHTInfoEle, EXTCHNL_OFFSET_NO_EXT);
		}
		else{
			SET_HT_INFO_ELE_STA_CHL_WIDTH(pHTInfoEle, CHNL_RUN_ABOVE_40MHZ(pMgntInfo));
			SET_HT_INFO_ELE_EXT_CHL_OFFSET(pHTInfoEle, pChnlInfo->Ext20MHzChnlOffsetOf40MHz);	
		}

		SET_HT_INFO_ELE_CONTROL_CHL(pHTInfoEle, pMgntInfo->dot11CurrentChannelNumber);		
		SET_HT_INFO_ELE_RIFS(pHTInfoEle, 0);
		SET_HT_INFO_ELE_PSMP_ACCESS_ONLY(pHTInfoEle, 0);
		SET_HT_INFO_ELE_SRV_INT_GRAN(pHTInfoEle, 0);
		SET_HT_INFO_ELE_OPT_MODE(pHTInfoEle, pHTInfo->CurrentOpMode);
		SET_HT_INFO_ELE_NON_GF_DEV(pHTInfoEle, 0);
		SET_HT_INFO_ELE_DUAL_BEACON(pHTInfoEle, 0);
		SET_HT_INFO_ELE_SECONDARY_BEACON(pHTInfoEle, 0);
		SET_HT_INFO_ELE_LSIG_TXOP_PROTECT_FULL(pHTInfoEle, 0);
		SET_HT_INFO_ELE_PCO_ACTIVE(pHTInfoEle, 0);
		SET_HT_INFO_ELE_PCO_PHASE(pHTInfoEle, 0);

		PlatformZeroMemory(GET_HT_INFO_ELE_BASIC_MCS(pHTInfoEle), 16);

		posHTInfo->Length = 22;
	}
	else
	{
		//STA should not generate High Throughput Information Element
		posHTInfo->Length = 0;
	}	

}	



VOID
HT_PickMCSRate(
	IN	PADAPTER		Adapter,
	OUT	pu1Byte			pOperateMCS
)
{
	u1Byte	i;
	
	if(IS_WIRELESS_MODE_N(Adapter))
	{//HT part
		if(HAL_SkipMcsRates(Adapter))
			pOperateMCS[1] &=0xF8; //Skip MCS8~11 because mcs7 > mcs6, 9, 10, 11. 2007.01.16 by Emily
		
		pOperateMCS[4] &= RATE_ADPT_MCS32_MASK;
	}
	else
	{//no MCS rate
		for(i = 0 ; i <= 15 ; i++)
			pOperateMCS[i] = 0;
	}
}



/*
*	Description:
*		This function will get the highest speed rate in input MCS set.
*
*	/param 	Adapter			Pionter to Adapter entity
*			pMCSRateSet		Pointer to MCS rate bitmap
*			pMCSFilter		Pointer to MCS rate filter
*	
*	/return	Highest MCS rate included in pMCSRateSet and filtered by pMCSFilter.
*
*/
u1Byte
HTGetHighestMCSRate(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pMCSRateSet,
	IN	pu1Byte			pMCSFilter
	)
{
	u1Byte		i, j, k;
	u1Byte		bitMap;
	u1Byte		mcsRate = 0;
	u1Byte		availableMcsRate[16];

	for(i=0; i<16; i++)
		availableMcsRate[i] = pMCSRateSet[i] & pMCSFilter[i];

	for(i = 0; i < 16; i++)
	{
		if(availableMcsRate[i] != 0)
			break;
	}
	if(i == 16)
		return 0;

	for(i = 0; i < 16; i++)
	{
		if(availableMcsRate[i] != 0)
		{
			bitMap = availableMcsRate[i];
			for(j = 0; j < 8; j++)
			{
				if((bitMap%2) != 0)
				{
					k = 8*i+j;					
					if(MCS_DATA_RATE[0][0][k] > MCS_DATA_RATE[0][0][mcsRate])
						mcsRate = k;
				}
				bitMap = bitMap>>1;
			}
		}
	}
	return (mcsRate + MGN_MCS0);
}
	


/*
**
**1.Filter our operation rate set with AP's rate set 
**2.shall reference channel bandwidth, STBC, Antenna number
**3.generate rate adative table for firmware
**David 20060906
**
** \pHTSupportedCap: the connected STA's supported rate Capability element
*/
BOOLEAN
HTFilterMCSRate(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pSupportMCS,
	OUT	pu1Byte			pOperateMCS
)
{
	PMGNT_INFO      			pMgntInfo = &Adapter->MgntInfo;
	UINT i=0;
	
	// filter out operational rate set not supported by AP, the lenth of it is 16
	for(i = 0;i <= 15; i++)
		pOperateMCS[i] = pMgntInfo->Regdot11HTOperationalRateSet[i] & pSupportMCS[i];

	// we also shall suggested the first start rate set according to our singal strength
	HT_PickMCSRate(Adapter, pOperateMCS);

	return TRUE;
}

VOID
HTOnAssocRsp(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	asocpdu
)
{

	PMGNT_INFO      			pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	pu1Byte					pPeerHTCap, pPeerHTInfo;
	OCTET_STRING			osTmp;
	pu1Byte					pCapMCS, pMcsFilter;
	static u1Byte				EWC11NHTCap[] = {0x00, 0x90, 0x4c, 0x33};		// For 11n EWC definition, 2007.07.17, by Emily
	static u1Byte				EWC11NHTInfo[] = {0x00, 0x90, 0x4c, 0x34};	// For 11n EWC definition, 2007.07.17, by Emily
	u2Byte					nMaxAMSDUSize;

	if( pHTInfo->bCurrentHTSupport == FALSE )
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("<=== HTOnAssocRsp(): HT_DISABLE\n") );
		return;
	}
	
	PlatformZeroMemory(pHTInfo->PeerHTCapBuf, sizeof(pHTInfo->PeerHTCapBuf));
	PlatformZeroMemory(pHTInfo->PeerHTInfoBuf, sizeof(pHTInfo->PeerHTInfoBuf));
		
	// Get HT CAP and copy into buffer
	osTmp= PacketGetElement( asocpdu, EID_HTCapability, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(osTmp.Length == 0)
		osTmp = PacketGetElement( asocpdu, EID_Vendor, OUI_SUB_11N_EWC_HT_CAP, OUI_SUBTYPE_DONT_CARE);
	
	osTmp.Length = osTmp.Length > sizeof(pHTInfo->PeerHTCapBuf)?\
		sizeof(pHTInfo->PeerHTCapBuf):osTmp.Length;	//prevent from overflow
		
	if(osTmp.Length > 0)
		CopyMem(pHTInfo->PeerHTCapBuf, osTmp.Octet, osTmp.Length);
	
	// Get HT Info and copy into buffer
	osTmp= PacketGetElement( asocpdu, EID_HTInfo, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(osTmp.Length == 0)
		osTmp = PacketGetElement( asocpdu, EID_Vendor, OUI_SUB_11N_EWC_HT_INFO, OUI_SUBTYPE_DONT_CARE);
	
	osTmp.Length = osTmp.Length>sizeof(pHTInfo->PeerHTCapBuf)?\
		sizeof(pHTInfo->PeerHTCapBuf):osTmp.Length;	//prevent from overflow
		
	if(osTmp.Length > 0)
		CopyMem(pHTInfo->PeerHTInfoBuf, osTmp.Octet, osTmp.Length);

	if(!PlatformCompareMemory(pHTInfo->PeerHTCapBuf,EWC11NHTCap, sizeof(EWC11NHTCap)))
		pPeerHTCap = (pu1Byte)(&pHTInfo->PeerHTCapBuf[4]);
	else
		pPeerHTCap = (pu1Byte)(pHTInfo->PeerHTCapBuf);

	if(!PlatformCompareMemory(pHTInfo->PeerHTInfoBuf, EWC11NHTInfo, sizeof(EWC11NHTInfo)))
		pPeerHTInfo = (pu1Byte)(&pHTInfo->PeerHTInfoBuf[4]);
	else		
		pPeerHTInfo = (pu1Byte)(pHTInfo->PeerHTInfoBuf);
	
	// Get 20/40 BSS Coexistence Management Support from Extended Capabilities element
	osTmp = PacketGetElement(asocpdu, EID_EXTCapability, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(osTmp.Length != 0)
		pHTInfo->bPeerBssCoexistence = GET_EXT_CAPABILITY_ELE_BSS_COEXIST(osTmp.Octet);

	// Get OBSS scan interval from Overlapping BSS Scan Parameters element
	osTmp = PacketGetElement(asocpdu, EID_OBSS, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(osTmp.Length != 0)
		pHTInfo->CurOBSSScanInterval = GET_OBSS_PARAM_ELE_SCAN_INTERVAL(osTmp.Octet);

	// Get OBSS scan exemption grant from 20/40 BSS Coexistence element
	osTmp = PacketGetElement(asocpdu, EID_BSSCoexistence, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(osTmp.Length != 0)
		pHTInfo->bCurOBSSScanExemptionGrt = GET_BSS_COEXISTENCE_ELE_OBSS_EXEMPTION_GRT(osTmp.Octet);

	pHTInfo->bPeer40MHzCap = GET_HT_CAPABILITY_ELE_CHL_WIDTH(pPeerHTCap);
	pHTInfo->PeerExtChnlOffset = (EXTCHNL_OFFSET)GET_HT_INFO_ELE_EXT_CHL_OFFSET(pPeerHTInfo);
	pHTInfo->bPeer40MHzIntolerant  = GET_HT_CAPABILITY_ELE_FORTY_INTOLERANT(pPeerHTCap);
	

	// B0 Config LDPC Coding Capability
	if(TEST_FLAG(pHTInfo->HtLdpcCap, LDPC_HT_ENABLE_TX))
	{
		SET_FLAG(pHTInfo->HtCurLdpc, GET_HT_CAPABILITY_ELE_LDPC_CAP(pPeerHTCap) ? (LDPC_HT_ENABLE_TX | LDPC_HT_CAP_TX) : 0);
		RT_TRACE_F(COMP_HT, DBG_LOUD, ("Target BSS support HT LDPC, enable Tx LDPC!\n"));
	}	

	// B5 B6 Config Short GI/ Long GI setting
	pHTInfo->bCurShortGI20MHz = GET_HT_CAPABILITY_ELE_SHORT_GI20M(pPeerHTCap);
	pHTInfo->bCurShortGI40MHz = GET_HT_CAPABILITY_ELE_SHORT_GI40M(pPeerHTCap);

	// B7 B8 B9 Config STBC setting
	if(	TEST_FLAG(pHTInfo->HtStbcCap, STBC_HT_ENABLE_TX) && GET_HT_CAPABILITY_ELE_RX_STBC(pPeerHTCap))
	{
		SET_FLAG(pHTInfo->HtCurStbc, (STBC_HT_ENABLE_TX | STBC_HT_CAP_TX) );
	}

	// B11  Maximum A-MSDU Length
	pHTInfo->bCurrent_AMSDU_Support = pHTInfo->bAMSDU_Support;
	nMaxAMSDUSize = (GET_HT_CAPABILITY_ELE_MAX_AMSDU_SIZE(pPeerHTCap)==0)?HT_AMSDU_SIZE_4K:HT_AMSDU_SIZE_8K;
	if(pHTInfo->nAMSDU_MaxSize > nMaxAMSDUSize )
		pHTInfo->nCurrent_AMSDU_MaxSize = nMaxAMSDUSize;
	else
		pHTInfo->nCurrent_AMSDU_MaxSize = pHTInfo->nAMSDU_MaxSize;

	RT_TRACE_F(COMP_AMSDU, DBG_LOUD, ("pHTInfo->bCurrent_AMSDU_Support = %d, CurrentSize = %d, ForcedSize = %d\n", pHTInfo->bCurrent_AMSDU_Support, pHTInfo->nCurrent_AMSDU_MaxSize, pHTInfo->ForcedAMSDUMaxSize));

	// B12 Config DSSS/CCK  mode in 40MHz mode
	HTSetCCKSupport(Adapter, FALSE, pPeerHTCap);

	// A-MPDU setting
	pHTInfo->bCurrentAMPDUEnable = pHTInfo->bAMPDUEnable;
	if(	(pMgntInfo->SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP104) || 
		(pMgntInfo->SecurityInfo.GroupEncAlgorithm == RT_ENC_ALG_WEP40)  ||
		(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104) ||
		(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40) || 
		(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_TKIP))
	{
		if( (pMgntInfo->IOTPeer == HT_IOT_PEER_ATHEROS) ||
		    (pMgntInfo->IOTPeer == HT_IOT_PEER_UNKNOWN) )
			pHTInfo->bCurrentAMPDUEnable = FALSE;
	}	
	
	// B16 B17 Maximum A-MPDU  Length Exponent
	if(GET_HT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(pPeerHTCap) < pHTInfo->CurrentAMPDUFactor)
		pHTInfo->CurrentAMPDUFactor = GET_HT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(pPeerHTCap);

	// BIT18 BIT19 BIT20 AMPDU Minimum MPDU Start Spacing
	if(pHTInfo->MPDU_Density > GET_HT_CAPABILITY_ELE_MPDU_DENSITY(pPeerHTCap))
		pHTInfo->CurrentMPDUDensity = pHTInfo->MPDU_Density;
	else
		pHTInfo->CurrentMPDUDensity = GET_HT_CAPABILITY_ELE_MPDU_DENSITY(pPeerHTCap);

	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_FACTOR, &pHTInfo->CurrentAMPDUFactor);
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_MIN_SPACE, &pHTInfo->CurrentMPDUDensity);


	 if(	pMgntInfo->IOTAction & HT_IOT_ACT_AMSDU_ENABLE ||
	 	pMgntInfo->IOTAction & HT_IOT_ACT_AMSDU_AMPDU)
	{
		pHTInfo->bCurrent_AMSDU_Support = TRUE;

		if(pMgntInfo->IOTAction  & HT_IOT_ACT_AMSDU_ENABLE)
			pHTInfo->bCurrentAMPDUEnable = FALSE;
	}

	// Rx Reorder Setting
	pHTInfo->bCurRxReorderEnable = pHTInfo->bRegRxReorderEnable;

	pCapMCS = GET_HT_CAPABILITY_ELE_MCS(pPeerHTCap);
		
	HTFilterMCSRate(Adapter, pCapMCS, pMgntInfo->dot11HTOperationalRateSet);
		
	// B2 B3 SM Power Save
	pHTInfo->PeerMimoPs = GET_HT_CAPABILITY_ELE_MIMO_PWRSAVE(pPeerHTCap);
	if(pHTInfo->PeerMimoPs <= MIMO_PS_DYNAMIC)
		pMcsFilter = MCS_FILTER_1SS;
	else
		pMcsFilter = MCS_FILTER_ALL;
	
	pMgntInfo->HTHighestOperaRate = HTGetHighestMCSRate(
													Adapter, 
													pMgntInfo->dot11HTOperationalRateSet,
													pMcsFilter	);

	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_MAX_TIME, &pMgntInfo->HTHighestOperaRate);

	// Config Tx beamforming setting
	CLEAR_FLAGS(pHTInfo->HtCurBeamform);
	if(	TEST_FLAG(pHTInfo->HtBeamformCap, BEAMFORMING_HT_BEAMFORMEE_ENABLE) &&
		GET_HT_CAP_TXBF_EXPLICIT_COMP_STEERING_CAP(pPeerHTCap))
	{
		SET_FLAG(pHTInfo->HtCurBeamform, BEAMFORMING_HT_BEAMFORMER_ENABLE);
		// Shit to BEAMFORMING_HT_BEAMFORMEE_CHNL_EST_CAP
		SET_FLAG(pHTInfo->HtCurBeamform, GET_HT_CAP_TXBF_CHNL_ESTIMATION_NUM_ANTENNAS(pPeerHTCap)<<6);
	}

	if(	TEST_FLAG(pHTInfo->HtBeamformCap, BEAMFORMING_HT_BEAMFORMER_ENABLE) &&
		GET_HT_CAP_TXBF_EXPLICIT_COMP_FEEDBACK_CAP(pPeerHTCap))
	{
		SET_FLAG(pHTInfo->HtCurBeamform, BEAMFORMING_HT_BEAMFORMEE_ENABLE);
		// Shit to BEAMFORMING_HT_BEAMFORMER_STEER_NUM
		SET_FLAG(pHTInfo->HtCurBeamform, GET_HT_CAP_TXBF_COMP_STEERING_NUM_ANTENNAS(pPeerHTCap)<<4);
	}
	RT_DISP(FBEAM, FBEAM_FUN, ("Client HT Beaforming Cap = 0x%02X\n", pHTInfo->HtCurBeamform));

	// Config current operation mode.
	pHTInfo->CurrentOpMode = GET_HT_INFO_ELE_OPT_MODE(pPeerHTInfo);
}	


/*
*  This function is called when
*  (1) MPInitialization Phase
*  (2) Receiving of Deauthentication from AP
*/

// TODO: Should this funciton be called when receiving of Disassociation?
VOID
HTInitializeHTInfo(PADAPTER	Adapter)
{
	PRT_HIGH_THROUGHPUT pHTInfo = Adapter->MgntInfo.pHTInfo;

	// These parameters will be reset when receiving deauthentication packet 
	pHTInfo->bCurrentHTSupport = FALSE;

	// LDPC support
	CLEAR_FLAGS(pHTInfo->HtCurLdpc);

	// STBC support
	CLEAR_FLAGS(pHTInfo->HtCurStbc);

	pHTInfo->bPeer40MHzCap = FALSE;
	pHTInfo->bPeer40MHzIntolerant = FALSE;

	// Short GI support
	pHTInfo->bCurShortGI20MHz = FALSE;
	pHTInfo->bCurShortGI40MHz = FALSE;

	// CCK rate support
	// This flag is set to TRUE to support CCK rate by default.
	// It will be affected by "pHTInfo->bRegSuppCCK" and AP capabilities only when associate to 11N BSS.
	pHTInfo->bCurSuppCCK = TRUE;

	// AMSDU related
	pHTInfo->bCurrent_AMSDU_Support = FALSE;
	pHTInfo->nCurrent_AMSDU_MaxSize = pHTInfo->nAMSDU_MaxSize;

	// AMPUD related
	pHTInfo->CurrentMPDUDensity = pHTInfo->MPDU_Density;
	pHTInfo->CurrentAMPDUFactor = pHTInfo->AMPDU_Factor;

	// 20/40 Bss Coexistence related
	pHTInfo->bPeerBssCoexistence = FALSE;
	pHTInfo->bCurOBSSScanExemptionGrt = FALSE;
	pHTInfo->CurOBSSScanInterval = 0;
	pHTInfo->IdleOBSSScanCnt = 0;
	pHTInfo->lastTimeSentObssRptUs = 0;

	// Initialize all of the parameters related to 11n	
	PlatformZeroMemory((PVOID)(&(pHTInfo->SelfHTCap)), sizeof(pHTInfo->SelfHTCap));
	PlatformZeroMemory((PVOID)(&(pHTInfo->SelfHTInfo)), sizeof(pHTInfo->SelfHTInfo));
	PlatformZeroMemory((PVOID)(&(pHTInfo->PeerHTCapBuf)), sizeof(pHTInfo->PeerHTCapBuf));
	PlatformZeroMemory((PVOID)(&(pHTInfo->PeerHTInfoBuf)), sizeof(pHTInfo->PeerHTInfoBuf));

	// Set default IEEE spec for Draft N
	pHTInfo->ePeerHTSpecVer = HT_SPEC_VER_IEEE;

	// Realtek proprietary aggregation mode
	pHTInfo->RT2RT_HT_Mode = 0;	

	// Beamforming support
	CLEAR_FLAGS(pHTInfo->HtCurBeamform);
}

VOID 
HTInitializeBssDesc(
	IN	PBSS_HT		pBssHT
	)
{

	pBssHT->bdSupportHT = FALSE;
	PlatformZeroMemory(pBssHT->bdHTCapBuf, sizeof(pBssHT->bdHTCapBuf));
	pBssHT->bdHTCapLen = 0;
	PlatformZeroMemory(pBssHT->bdHTInfoBuf, sizeof(pBssHT->bdHTInfoBuf));
	pBssHT->bdHTInfoLen = 0;

	pBssHT->bdHTSpecVer= HT_SPEC_VER_IEEE;
	
	pBssHT->RT2RT_HT_Mode = 0;

	pBssHT->bd40Intolerant = FALSE;
	pBssHT->bdOBSSExemption = FALSE;
	pBssHT->OBSSScanInterval = 0;
}



VOID
HTParsingHTCapElement(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	HTCapIE,
	OUT	PRT_WLAN_BSS	pBssDesc
)
{
	PMGNT_INFO      			pMgntInfo = &Adapter->MgntInfo;
	
	if( HTCapIE.Length > sizeof(pBssDesc->BssHT.bdHTCapBuf) )
	{
		RT_TRACE( COMP_HT, DBG_LOUD, ("HTParsingHTCapElement(): HT Capability Element length is too long!\n") );
		return;
	}

	// TODO: Check the correctness of HT Cap 
	//Print each field in detail. Driver should not print out this message by default
	if(!ACTING_AS_AP(Adapter) && !pMgntInfo->mAssoc)
		HTDebugHTCapability(DBG_TRACE, Adapter, &HTCapIE, (pu1Byte)"HTParsingHTCapElement()");

	HTCapIE.Length = HTCapIE.Length > sizeof(pBssDesc->BssHT.bdHTCapBuf)?\
		sizeof(pBssDesc->BssHT.bdHTCapBuf):HTCapIE.Length;	//prevent from overflow
		
	CopyMem(pBssDesc->BssHT.bdHTCapBuf, HTCapIE.Octet, HTCapIE.Length);
	pBssDesc->BssHT.bdHTCapLen = HTCapIE.Length;

}


VOID
HTParsingHTInfoElement(
	PADAPTER		Adapter,
	OCTET_STRING	HTInfoIE,
	PRT_WLAN_BSS	pBssDesc
)
{
	PMGNT_INFO      			pMgntInfo = &Adapter->MgntInfo;
	
	if( HTInfoIE.Length > sizeof(pBssDesc->BssHT.bdHTInfoBuf))
	{
		RT_TRACE( COMP_HT, DBG_LOUD, ("HTParsingHTInfoElement(): HT Information Element length is too long!\n") );
		return;
	}

	// TODO: Check the correctness of HT Info 
	//Print each field in detail. Driver should not print out this message by default
	if(!ACTING_AS_AP(Adapter) && !pMgntInfo->mAssoc)		
		HTDebugHTInfo(DBG_TRACE, Adapter, &HTInfoIE, (pu1Byte)"HTParsingHTInfoElement()");		
	
	HTInfoIE.Length = HTInfoIE.Length > sizeof(pBssDesc->BssHT.bdHTInfoBuf)?\
		sizeof(pBssDesc->BssHT.bdHTInfoBuf):HTInfoIE.Length;	//prevent from overflow
		
	CopyMem( pBssDesc->BssHT.bdHTInfoBuf, HTInfoIE.Octet, HTInfoIE.Length);
	pBssDesc->BssHT.bdHTInfoLen = HTInfoIE.Length;
}

/*
  * Get HT related information from beacon and save it in BssDesc
  *
  * (1) Parse HTCap, and HTInfo, and record whether it is 11n AP
  * (2) If peer is HT, but not WMM, call QosSetLegacyWMMParamWithHT()
  * (3) Check whether peer is Realtek AP (for Realtek proprietary aggregation mode).
  * Input:		
  * 		PADAPTER	Adapter
  * 			
  * Output:		
  *		PRT_TCB		BssDesc
  *
*/
VOID
HTGetValueFromBeaconOrProbeRsp(
	PADAPTER			Adapter,
	POCTET_STRING		pSRCmmpdu,
	PRT_WLAN_BSS		bssDesc
)
{
	OCTET_STRING	HTCapIE, HTInfoIE, OBSSIE, BssCoexistenceIE, mmpdu;

	mmpdu.Octet = pSRCmmpdu->Octet;
	mmpdu.Length = pSRCmmpdu->Length;
		
	//2Note:  
	//   Mark for IOT testing using  Linksys WRT350N, This AP does not contain WMM IE  when 
	//   it is configured at pure-N mode.
	//	if(bssDesc->BssQos.bdQoSMode & QOS_WMM)
	//	

	HTInitializeBssDesc (&bssDesc->BssHT);

	//2<1> Parse HTCap, and HTInfo
	// Get HT Capability IE: (1) Get IEEE Draft N IE or (2) Get EWC IE
	HTCapIE = PacketGetElement(mmpdu, EID_HTCapability, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	RT_DISP(FBEACON, BCN_SHOW, 
	("EID_HTCapability HTCapIE.Length=%d\r\n", HTCapIE.Length))
	if(HTCapIE.Length == 0)
	{
		HTCapIE = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_11N_EWC_HT_CAP, OUI_SUBTYPE_DONT_CARE);		
		if(HTCapIE.Length != 0)
			bssDesc->BssHT.bdHTSpecVer= HT_SPEC_VER_EWC;
		RT_DISP(FBEACON, BCN_SHOW, 
		("EID_Vendor HTCapIE.Length=%d\r\n", HTCapIE.Length))
	}
	if(HTCapIE.Length != 0)
		HTParsingHTCapElement(Adapter, HTCapIE, bssDesc);

	// Get HT Information IE: (1) Get IEEE Draft N IE or (2) Get EWC IE
	HTInfoIE = PacketGetElement(mmpdu, EID_HTInfo, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(HTInfoIE.Length == 0)
	{
		HTInfoIE = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_11N_EWC_HT_INFO, OUI_SUBTYPE_DONT_CARE);
		if(HTInfoIE.Length != 0)
				bssDesc->BssHT.bdHTSpecVer  = HT_SPEC_VER_EWC;
	}
	if(HTInfoIE.Length != 0)
		HTParsingHTInfoElement(Adapter, HTInfoIE, bssDesc);

	//2<2>If peer is HT, but not WMM, call QosSetLegacyWMMParamWithHT()
	if(HTCapIE.Length != 0)
	{
		bssDesc->BssHT.bdSupportHT = TRUE;
		if(bssDesc->BssQos.bdQoSMode == QOS_DISABLE)
			QosSetLegacyWMMParamWithHT(Adapter, bssDesc);	
	}
	else
	{
		bssDesc->BssHT.bdSupportHT = FALSE;
	}

	//2<3>Parse OBSS Scan IE
	OBSSIE = PacketGetElement(mmpdu, EID_OBSS, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(OBSSIE.Length != 0)
		BSS_ParsingOBSSInfoElement(Adapter, OBSSIE, bssDesc);
	
	//2<3>Parse 20 40 BSS Coexistence IE
	// 23. 20/40 BSS Coexistence element	
	BssCoexistenceIE = PacketGetElement(mmpdu, EID_BSSCoexistence, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if(BssCoexistenceIE.Length != 0)
		BSS_ParsingBSSCoexistElement(Adapter, BssCoexistenceIE, bssDesc);
		
	//2<4>Check peer configuration
	if(HTCapIE.Length!=0 && HTCapIE.Octet != NULL)
	{
		bssDesc->bdBandWidth = (CHANNEL_WIDTH)(GET_HT_CAPABILITY_ELE_CHL_WIDTH(HTCapIE.Octet));
		bssDesc->BssHT.bd40Intolerant = GET_HT_CAPABILITY_ELE_FORTY_INTOLERANT(HTCapIE.Octet);
	}
	else
		bssDesc->bdBandWidth = CHANNEL_WIDTH_20;
}

/*-----------------------------------------------------------------------------
 * Function:    HTResetSelfAndSavePeerSetting()
 *
 * Overview:   This function should ONLY be called before association. SetupJoinInfraInfo(). 
 *			 Driver reset it's state to  it's capability and peer's capability
 * 			
 * Input:       PADAPTER				Adapter
 *			PRT_WLAN_BSS			pBssDesc	
 *
 * Output:      NONE
 *
 * Return:      NONE
 
 *---------------------------------------------------------------------------*/
VOID
HTResetSelfAndSavePeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc
)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	BOOLEAN					bIOTAction;

	//
	//  Save Peer Setting before Association
	//
	if( pHTInfo->bEnableHT &&  pBssDesc->BssHT.bdSupportHT)
	{
		pHTInfo->bCurrentHTSupport = TRUE;
		pHTInfo->ePeerHTSpecVer = pBssDesc->BssHT.bdHTSpecVer;

		// Save HTCap and HTInfo information Element
		if(pBssDesc->BssHT.bdHTCapLen > 0 && 	pBssDesc->BssHT.bdHTCapLen <= sizeof(pHTInfo->PeerHTCapBuf))
			CopyMem(pHTInfo->PeerHTCapBuf, pBssDesc->BssHT.bdHTCapBuf, pBssDesc->BssHT.bdHTCapLen);

		if(pBssDesc->BssHT.bdHTInfoLen > 0 && pBssDesc->BssHT.bdHTInfoLen <= sizeof(pHTInfo->PeerHTInfoBuf))
			CopyMem(pHTInfo->PeerHTInfoBuf, pBssDesc->BssHT.bdHTInfoBuf, pBssDesc->BssHT.bdHTInfoLen);

		if(pBssDesc->BssHT.bdHTCapLen != 0)
			pHTInfo->bPeer40MHzCap = GET_HT_CAPABILITY_ELE_CHL_WIDTH(pBssDesc->BssHT.bdHTCapBuf);
		
		if(pBssDesc->BssHT.bdHTInfoLen != 0)
			pHTInfo->PeerExtChnlOffset = (EXTCHNL_OFFSET)GET_HT_INFO_ELE_EXT_CHL_OFFSET(pBssDesc->BssHT.bdHTInfoBuf);

		// Check whether RT to RT aggregation mode is enabled
		pHTInfo->RT2RT_HT_Mode = pBssDesc->BssHT.RT2RT_HT_Mode;
	
		// Decide IOT Action
		// Must be called after the parameter of pHTInfo->bCurrentRT2RTAggregation is decided
		pMgntInfo->IOTAction = 0;
		bIOTAction = HTIOTActIsDisableMCSTwoSpatialStream(Adapter, pBssDesc);
		if(bIOTAction)
			pMgntInfo->IOTAction |= HT_IOT_ACT_DISABLE_ALL_2SS;

		bIOTAction = HTIOTActIsForcedCTS2Self(Adapter, pBssDesc);
		if(bIOTAction)	
			pMgntInfo->IOTAction |= HT_IOT_ACT_FORCED_CTS2SELF;

		bIOTAction = HTIOTActIsEDCABiasRx(Adapter, pBssDesc);
		if(bIOTAction)
			pMgntInfo->IOTAction |= HT_IOT_ACT_EDCA_BIAS_ON_RX;

		bIOTAction = HTIOCActIsDisableCckRate(Adapter, pBssDesc);
		if(bIOTAction)
			pMgntInfo->IOTAction |= HT_IOT_ACT_DISABLE_CCK_RATE;

		bIOTAction = HTIOTActDisableHighPower(Adapter, pBssDesc);
		if(bIOTAction)
			pMgntInfo->IOTAction |= HT_IOT_ACT_DISABLE_HIGH_POWER;

		bIOTAction = HTIOTActIsTxNoAggregation(Adapter, pBssDesc);
		if(bIOTAction)
			pMgntInfo->IOTAction |= HT_IOT_ACT_TX_NO_AGGREGATION;

		bIOTAction = HTIOTActIsDisableTx40MHz(Adapter, pBssDesc);
		if(bIOTAction)
			pMgntInfo->IOTAction |= HT_IOT_ACT_DISABLE_TX_40_MHZ;

		bIOTAction = HTIOTActIsDisableTx2SS(Adapter, pBssDesc);
		if(bIOTAction)
			pMgntInfo->IOTAction |= HT_IOT_ACT_DISABLE_TX_2SS;

		bIOTAction = HTIOTActDisableRx40MHzShortGI(Adapter, pBssDesc);
		if(bIOTAction)
			pMgntInfo->IOTAction |= HT_IOT_ACT_DISABLE_RX_40MHZ_SHORT_GI;

		bIOTAction = HTIOTActDisableRx20MHzShortGI(Adapter, pBssDesc);
		if(bIOTAction)
			pMgntInfo->IOTAction |= HT_IOT_ACT_DISABLE_RX_20MHZ_SHORT_GI;

		if (pBssDesc->SubTypeOfVender == HT_IOT_PEER_ATHEROS_NETGEAR_WNDAP3200)
		{
			pMgntInfo->IOTAction |=HT_IOT_ACT_DISABLE_EXT_CHNL_COMBINE;
		}
	}
	else
	{
		pHTInfo->bCurrentHTSupport = FALSE;
		pHTInfo->bPeer40MHzCap = CHANNEL_WIDTH_20;
		pHTInfo->PeerExtChnlOffset = EXTCHNL_OFFSET_NO_EXT;
		pHTInfo->RT2RT_HT_Mode = 0;
		pMgntInfo->IOTAction = 0;
	}
	
}

VOID
HTUpdateSelfAndPeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc
	)
{
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	pu1Byte						pPeerHTInfo = (pu1Byte)pBssDesc->BssHT.bdHTInfoBuf;
	
	if(pHTInfo->bCurrentHTSupport)
	{
		// Config current operation mode.
		if(pBssDesc->BssHT.bdHTInfoLen != 0)
		{
			pHTInfo->CurrentOpMode = GET_HT_INFO_ELE_OPT_MODE(pPeerHTInfo);
			pHTInfo->PeerExtChnlOffset = (EXTCHNL_OFFSET)GET_HT_INFO_ELE_EXT_CHL_OFFSET(pPeerHTInfo);
		}

		// <TODO: Config according to OBSS non-HT STA present!!>
		pHTInfo->CurOBSSScanInterval = pBssDesc->BssHT.OBSSScanInterval;
		pHTInfo->bCurOBSSScanExemptionGrt = pBssDesc->BssHT.bdOBSSExemption;
	}
}

VOID
HTUseDefaultSettingFromReg(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	PADAPTER				DefAdapter = GetDefaultAdapter(Adapter);
	BOOLEAN					bRegBW40MHz = CHNL_GetRegBWSupport(Adapter)?TRUE:FALSE;

	pHTInfo->HtCurLdpc = pHTInfo->HtLdpcCap;
	pHTInfo->HtCurStbc = pHTInfo->HtStbcCap;
	pHTInfo->HtCurBeamform = pHTInfo->HtBeamformCap;
	
	pHTInfo->bCurSuppCCK = pHTInfo->bRegSuppCCK;
	pHTInfo->bCurShortGI20MHz= TRUE;
	pHTInfo->bCurShortGI40MHz= bRegBW40MHz?TRUE:FALSE;
	pHTInfo->bCurrent_AMSDU_Support = pHTInfo->bAMSDU_Support;
	pHTInfo->nCurrent_AMSDU_MaxSize = pHTInfo->nAMSDU_MaxSize;
	pHTInfo->bCurrentAMPDUEnable = pHTInfo->bAMPDUEnable;
	pHTInfo->CurrentAMPDUFactor = pHTInfo->AMPDU_Factor;
	pHTInfo->CurrentMPDUDensity = pHTInfo->MPDU_Density;

	// XP, Win7	DefAdapter AP mode mAssoc = FALSE. AP_SetupStartApInfo/SetupStartIBSSInfo 	(only default port)
	//			ExtAdapter AP mode mAssoc = FALSE. AP_SetupStartApInfo/SetupStartIBSSInfo 	(two port)
	//			DefAdapter AP mode mAssoc = TRUE. If STA connected		(two port)
	// Vista	DefAdapter AP mode mAssoc = FALSE. AP_SetupStartApInfo/SetupStartIBSSInfo 	(only default port)
	if(DefAdapter->MgntInfo.mAssoc == TRUE)
		pMgntInfo->IOTPeer = DefAdapter->MgntInfo.IOTPeer;
	else	
		pMgntInfo->IOTPeer = HT_IOT_PEER_SELF_SOFTAP;

	pHTInfo->bCurRxReorderEnable = pHTInfo->bRegRxReorderEnable;

	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_FACTOR, &pHTInfo->CurrentAMPDUFactor);
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_MIN_SPACE, &pHTInfo->CurrentMPDUDensity);

	HTFilterMCSRate(Adapter, pMgntInfo->Regdot11HTOperationalRateSet, pMgntInfo->dot11HTOperationalRateSet);

	pMgntInfo->HTHighestOperaRate = HTGetHighestMCSRate(
													Adapter,
													pMgntInfo->dot11HTOperationalRateSet,
													MCS_FILTER_ALL	);

	if(bRegBW40MHz)
		pMgntInfo->dot11CurrentChannelBandWidth = CHANNEL_WIDTH_40;
	else
		pMgntInfo->dot11CurrentChannelBandWidth = CHANNEL_WIDTH_20;
}

VOID
HTUseDefaultSettingFromDefault(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);

	PADAPTER				DefAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO				pDefMgntInfo = &DefAdapter->MgntInfo;
	PRT_HIGH_THROUGHPUT 	pDefHTInfo = DefAdapter->MgntInfo.pHTInfo;
	
	//=====   The ones that not follow   ======	
	pHTInfo->bCurShortGI20MHz	= TRUE;

	pHTInfo->HtCurLdpc = pHTInfo->HtLdpcCap;
	pHTInfo->HtCurStbc = pHTInfo->HtStbcCap;
	pHTInfo->HtCurBeamform = pHTInfo->HtBeamformCap;

	pHTInfo->bCurrentAMPDUEnable = pHTInfo->bAMPDUEnable;
	pHTInfo->bCurrent_AMSDU_Support = pHTInfo->bAMSDU_Support;
	pHTInfo->nCurrent_AMSDU_MaxSize = pHTInfo->nAMSDU_MaxSize;

	pHTInfo->bCurRxReorderEnable = pHTInfo->bRegRxReorderEnable;

	HTFilterMCSRate(Adapter, pMgntInfo->Regdot11HTOperationalRateSet, pMgntInfo->dot11HTOperationalRateSet);
	pMgntInfo->HTHighestOperaRate = HTGetHighestMCSRate(
													Adapter,
													pMgntInfo->dot11HTOperationalRateSet,
													MCS_FILTER_ALL	);
	
	//==============================	
	pHTInfo->bCurSuppCCK 		= pDefHTInfo->bCurSuppCCK;
	pHTInfo->bCurShortGI40MHz	= pDefMgntInfo->dot11CurrentChannelBandWidth?FALSE:TRUE;
	//AMPDU Factor and Density is register, can't be per pkt. AMPDU_Factor must use Default for register. by Bohn, 2009.12.18
	pHTInfo->CurrentAMPDUFactor 	= pDefHTInfo->CurrentAMPDUFactor;
	pHTInfo->CurrentMPDUDensity 	= pDefHTInfo->CurrentMPDUDensity;
	//[Win7] Adapter act as AP already, Defaualt Port Connected already, 2009.07.30, by Bohn
	pMgntInfo->IOTPeer 			= pDefMgntInfo->IOTPeer;
			
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_FACTOR, &pHTInfo->CurrentAMPDUFactor);
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_MIN_SPACE, &pHTInfo->CurrentMPDUDensity);	

	pMgntInfo->dot11CurrentChannelBandWidth = pDefMgntInfo->dot11CurrentChannelBandWidth;
}
			

VOID
HTUseDefaultSetting(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	u1Byte				TwoPortStatus =(u1Byte) TWO_PORT_STATUS__DEFAULT_ONLY;

	if(pHTInfo->bEnableHT)
	{
		pHTInfo->bCurrentHTSupport = TRUE;
		
		//[Win7] [Vista/XP] Cross Platform Capbility Enable!, 2009.07.31, by Bohn
		GetTwoPortSharedResource(Adapter,TWO_PORT_SHARED_OBJECT__STATUS,NULL,&TwoPortStatus);

		//[win7 Two Port BW40Mhz issue] Extention Port Use Reg's Default Value. 2009.05.20, by bohn
		if(	TwoPortStatus==TWO_PORT_STATUS__DEFAULT_ONLY ||
			TwoPortStatus==TWO_PORT_STATUS__EXTENSION_ONLY ||
			TwoPortStatus==TWO_PORT_STATUS__WITHOUT_ANY_ASSOCIATE)		
		{
			HTUseDefaultSettingFromReg(Adapter);		
		}
		else if(TwoPortStatus==TWO_PORT_STATUS__EXTENSION_FOLLOW_DEFAULT)
		{	
			//[win7 Two Port BW40Mhz issue] Extention Port obey the rule of Default Port. 2009.05.20, by bohn
			HTUseDefaultSettingFromDefault(Adapter);
		}
		else if(TwoPortStatus==TWO_PORT_STATUS__DEFAULT_G_EXTENSION_N20 ||
				TwoPortStatus==TWO_PORT_STATUS__DEFAULT_A_EXTENSION_N20 ||
				TwoPortStatus==TWO_PORT_STATUS__ADHOC)
		{
			//[Win7] Now is the Default-G Extension-N20 case, we need to reset all HT value and forced to N20
			// 2009.07.09, by Bohn
			HTUseDefaultSettingFromReg(Adapter);

			pMgntInfo->dot11CurrentChannelBandWidth = CHANNEL_WIDTH_20;
		}
	}
	else
	{
		pHTInfo->bCurrentHTSupport = FALSE;
		pMgntInfo->dot11CurrentChannelBandWidth = CHANNEL_WIDTH_20;
	}
}

BOOLEAN
HTCCheck(
	PADAPTER			Adapter,
	PRT_RFD				pRfd,
	POCTET_STRING		pFrame
	)
{
	if( IsQoSDataFrame(pFrame->Octet) && Frame_Order(*pFrame) == 1)
		return TRUE;
	else
		return FALSE;
}

VOID
HTCheckHTCap(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry,
	IN	pu1Byte			pHTCap
)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	pu1Byte					pMcsFilter;
	u2Byte					TmpValue;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	u1Byte					HtSTBC = 0;

	//1 Get HT properties of the STA

	// B0 Config LDPC Coding Capability
	if(TEST_FLAG(pHTInfo->HtLdpcCap, LDPC_HT_ENABLE_TX))
		SET_FLAG(pEntry->HTInfo.LDPC, GET_HT_CAPABILITY_ELE_LDPC_CAP(pHTCap) ? (LDPC_HT_ENABLE_TX | LDPC_HT_CAP_TX) : 0);

	// B2 B3 Config Mimo power save settings
	pEntry->HTInfo.MimoPs = GET_HT_CAPABILITY_ELE_MIMO_PWRSAVE(pHTCap);

	// B5 B6 Config Short GI/ Long GI setting
	if(pHTInfo->bCurShortGI20MHz)	
		pEntry->HTInfo.bShortGI20M = GET_HT_CAPABILITY_ELE_SHORT_GI20M(pHTCap)?TRUE:FALSE;
	else
		pEntry->HTInfo.bShortGI20M = FALSE;

	if(pHTInfo->bCurShortGI40MHz)	
		pEntry->HTInfo.bShortGI40M = GET_HT_CAPABILITY_ELE_SHORT_GI40M(pHTCap)?TRUE:FALSE;
	else
		pEntry->HTInfo.bShortGI40M = FALSE;

	// B7 B8 B9 Config STBC setting
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_TX_STBC, (pu1Byte)&HtSTBC);
	if(!TEST_FLAG(pHTInfo->HtStbcCap, STBC_HT_ENABLE_TX))
		HtSTBC = 0;

	if(HtSTBC < GET_HT_CAPABILITY_ELE_RX_STBC(pHTCap))
		pEntry->HTInfo.STBC = HtSTBC;
	else
		pEntry->HTInfo.STBC = GET_HT_CAPABILITY_ELE_RX_STBC(pHTCap);

	// B11 Config A-MSDU setting
	TmpValue = (GET_HT_CAPABILITY_ELE_MAX_AMSDU_SIZE(pHTCap)==0)?HT_AMSDU_SIZE_4K:HT_AMSDU_SIZE_8K;
	if(pHTInfo->nAMSDU_MaxSize >= TmpValue)	
		pEntry->HTInfo.AMSDU_MaxSize = TmpValue;
	else
		pEntry->HTInfo.AMSDU_MaxSize = pHTInfo->nAMSDU_MaxSize;

	// B12 Config DSSS/CCK  mode in 40MHz mode
	pEntry->HTInfo.bSupportCck = 
		((pHTInfo->bCurSuppCCK)?(GET_HT_CAPABILITY_ELE_DSS_CCK(pHTCap)?TRUE:FALSE):FALSE);

	// Get operation bandwidth
	if(CHNL_RUN_ABOVE_40MHZ(pMgntInfo))
		pEntry->BandWidth = GET_HT_CAPABILITY_ELE_CHL_WIDTH(pHTCap)?CHANNEL_WIDTH_40:CHANNEL_WIDTH_20;
	else
		pEntry->BandWidth = CHANNEL_WIDTH_20;

	// Get AMPDU Factor
	if(GET_HT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(pHTCap) < pHTInfo->CurrentAMPDUFactor)
		pEntry->HTInfo.AMPDU_Factor = GET_HT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(pHTCap);
	else 
		pEntry->HTInfo.AMPDU_Factor = pHTInfo->CurrentAMPDUFactor;

	// Get AMPDU Density
	if(pHTInfo->MPDU_Density >= GET_HT_CAPABILITY_ELE_MPDU_DENSITY(pHTCap))
		pEntry->HTInfo.MPDU_Density = pHTInfo->MPDU_Density;
	else
		pEntry->HTInfo.MPDU_Density = GET_HT_CAPABILITY_ELE_MPDU_DENSITY(pHTCap);

	// Filter MCS rate set that we support.
	HTFilterMCSRate(Adapter, GET_HT_CAPABILITY_ELE_MCS(pHTCap), pEntry->HTInfo.McsRateSet);

	// This is for static mimo power save
	if(pEntry->HTInfo.MimoPs <= MIMO_PS_DYNAMIC)
		pMcsFilter = MCS_FILTER_1SS;
	else
		pMcsFilter = MCS_FILTER_ALL;

	pEntry->HTInfo.HTHighestOperaRate = HTGetHighestMCSRate(Adapter, pEntry->HTInfo.McsRateSet, pMcsFilter);

	CLEAR_FLAGS(pEntry->HTInfo.HtCurBeamform);
	// B11 SU Beamformer Capable, we are Beamformee
	if(	TEST_FLAG(pHTInfo->HtBeamformCap, BEAMFORMING_HT_BEAMFORMEE_ENABLE) && 
		GET_HT_CAP_TXBF_EXPLICIT_COMP_STEERING_CAP(pHTCap))
	{
		SET_FLAG(pEntry->HTInfo.HtCurBeamform, BEAMFORMING_HT_BEAMFORMER_ENABLE);
		// Shit to BEAMFORMING_HT_BEAMFORMEE_CHNL_EST_CAP
		SET_FLAG(pHTInfo->HtCurBeamform, GET_HT_CAP_TXBF_CHNL_ESTIMATION_NUM_ANTENNAS(pHTCap)<<6);
	}

	// B12 SU Beamformee Capable, we are Beamformer
	if(	TEST_FLAG(pHTInfo->HtBeamformCap, BEAMFORMING_HT_BEAMFORMER_ENABLE) && 
		GET_HT_CAP_TXBF_EXPLICIT_COMP_FEEDBACK_CAP(pHTCap))
	{
		SET_FLAG(pEntry->HTInfo.HtCurBeamform, BEAMFORMING_HT_BEAMFORMEE_ENABLE);
		// Shit to BEAMFORMING_HT_BEAMFORMER_STEER_NUM
		SET_FLAG(pHTInfo->HtCurBeamform, GET_HT_CAP_TXBF_COMP_STEERING_NUM_ANTENNAS(pHTCap)<<4);
	}
	RT_DISP(FBEAM, FBEAM_FUN, ("Entry HT Beaforming Cap = 0x%02X\n", pEntry->HTInfo.HtCurBeamform));

	RT_PRINT_DATA( COMP_HT, DBG_TRACE, "Received HTCap in AsocReq", pHTCap, EID_HTCapability);
}



//
// Description: Switch BW to 40HMz for a specific reason. It only needs to be operate if WoWLAN mode
// 	is enabled and we will call the function in PNP_SET_POWER OID handler.
//	<Note> According to 802.11n spec chapter 11.14.12 "Switching between 40 MHz and 20 MHz",
//		2.4G AP will switch BW (40 <--> 20MHz) in some cases, and we will really switch BW
//		to 20MHz if our BSS AP enable the capability in beacon. It will cause an issue that if we
//		switch BW to 20MHz before the system enters WoWLAN mode, and the AP returns to 40MHz
//		such that we cannot receive 40MHz data and will be deauth by AP in some cases, so we 
//		recover the BW to 40MHz in order to Rx 40MHz data and we should make sure that we will
//		not Tx MCS rate during WoWLAN mode (now we set all the Tx packet rates to 1M, see
//		FillFakeTxDescriptor_XXX().).
//
// 2012.12.20, by tynli.
//
VOID
HTRecoverBWTo40MHz(
	IN		PADAPTER	Adapter
)
{
	CHANNEL_WIDTH	RegBW = (CHANNEL_WIDTH)(CHNL_GetRegBWSupport(Adapter)?TRUE:FALSE);

	// Only processing recover flow if the STA support 40MHz bandwidth.
	if(RegBW == CHANNEL_WIDTH_20)
		return;

	// Only if the wireless mode is 2.4G. According to 802.11n spec chapter 11.14.12 "Switching between
	// 40 MHz and 20 MHz", it mentions that AP is 2.4G mode.
	if(!IS_WIRELESS_MODE_N_24G(Adapter))
		return;

	CHNL_SetBwChnlFromPeer(Adapter);
}
