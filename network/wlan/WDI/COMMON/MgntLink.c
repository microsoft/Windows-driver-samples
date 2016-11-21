#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "MgntLink.tmh"
#endif

BOOLEAN		bDebugFixBssid=FALSE;
u1Byte		debug_fixed_bssid[6]={0};
/*------------------------ Local parameter------------------------------------*/
u4Byte DSSS_Freq_Channel[]={
			0,
			2412,
			2417,
			2422,
			2427,
			2432,
			2437,
			2442,
			2447,
			2452,
			2457,
			2462,
			2467,
			2472,
			2484,
};

//
// MAC address of AP for IOT
//
static u1Byte DLINK_ATHEROS_DIR635_1[3] = {0x00, 0x15, 0xe9};
static u1Byte DLINK_ATHEROS_DIR635_2[3] = {0x00, 0x17, 0x9A};
#if 1//(HARDWARE_TYPE_IS_RTL8192D == 1)
//vivi add for dlink 655, 20101125
static u1Byte DLINK_ATHEROS_DIR655_1[3] = {0x00, 0x18, 0xe7};
static u1Byte DLINK_ATHEROS_DIR655_2[3] = {0x00, 0x19, 0x5B};
static u1Byte DLINK_ATHEROS_DIR655_3[3] = {0x00, 0x1B, 0x11};
static u1Byte DLINK_ATHEROS_DIR655_4[3] = {0x00, 0x1C, 0xF0};
static u1Byte DLINK_ATHEROS_DIR655_5[3] = {0x00, 0x1E, 0x58};
static u1Byte DLINK_ATHEROS_DIR655_6[3] = {0x00, 0x21, 0x91};
static u1Byte DLINK_ATHEROS_DIR655_7[3] = {0x00, 0x22, 0xB0};
//static u1Byte DLINK_ATHEROS_DIR655_8[3] = {0x00, 0x24, 0x01};//dir855
#endif

static u1Byte DLINK_RALINK_DIR300[3] = {0xFC,0x75,0x16};

/*
static u1Byte LG_4G_LTE_HOT_SPOT_1[3] = {0x00, 0xAA, 0x70};
static u1Byte LG_4G_LTE_HOT_SPOT_2[3] = {0x00, 0x1E, 0x75};
static u1Byte LG_4G_LTE_HOT_SPOT_3[3] = {0x00, 0x1F, 0x6B};
static u1Byte LG_4G_LTE_HOT_SPOT_4[3] = {0x00, 0x1F, 0xE3};
static u1Byte LG_4G_LTE_HOT_SPOT_5[3] = {0x00, 0x21, 0xFB};
static u1Byte LG_4G_LTE_HOT_SPOT_6[3] = {0x00, 0x22, 0xA9};
static u1Byte LG_4G_LTE_HOT_SPOT_7[3] = {0x00, 0x24, 0x83};
static u1Byte LG_4G_LTE_HOT_SPOT_8[3] = {0x00, 0x25, 0xE5};
static u1Byte LG_4G_LTE_HOT_SPOT_9[3] = {0x00, 0x26, 0xE2};
static u1Byte LG_4G_LTE_HOT_SPOT_10[3] = {0x00, 0xE0, 0x91};
static u1Byte LG_4G_LTE_HOT_SPOT_11[3] = {0x10, 0xF9, 0x6F};
static u1Byte LG_4G_LTE_HOT_SPOT_12[3] = {0x20, 0x21, 0xA5};
static u1Byte LG_4G_LTE_HOT_SPOT_13[3] = {0x3C, 0xBD, 0xD8};
static u1Byte LG_4G_LTE_HOT_SPOT_14[3] = {0x6C, 0xD6, 0x8A};
static u1Byte LG_4G_LTE_HOT_SPOT_15[3] = {0x70, 0x05, 0x14};
static u1Byte LG_4G_LTE_HOT_SPOT_16[3] = {0x74, 0xA7, 0x22};
static u1Byte LG_4G_LTE_HOT_SPOT_17[3] = {0xA8, 0x16, 0xB2};
static u1Byte LG_4G_LTE_HOT_SPOT_18[3] = {0xA8, 0x92, 0x2C};
static u1Byte LG_4G_LTE_HOT_SPOT_19[3] = {0xC0, 0x41, 0xF6};
static u1Byte LG_4G_LTE_HOT_SPOT_20[3] = {0xE8, 0x92, 0xA4};
static u1Byte LG_4G_LTE_HOT_SPOT_21[3] = {0xF0, 0x1C, 0x13};
static u1Byte LG_4G_LTE_HOT_SPOT_22[3] = {0xF8, 0x0C, 0xF3};
*/

// 2011/11/03 MH Add for Netgear Germany case. We can not combine extention and primary channel.
static u1Byte NETGEAR_WNDAP3200_1[3] = {0xE0, 0x91, 0xF5};
// 2011/11/03 MH Add for Netgear Germany case. We can not combine extention and primary channel.
static u1Byte NETGEAR_WNDAP4500_1[3] = {0xC0, 0x00, 0x12};
// 2012/02/09 by wl.For Netgear3500v1 92d high power issue.
static u1Byte NETGEAR_WNDAP3500_1[3] = {0x00,0x24,0xb2};
// 2012/06/25 Zhiyuan Add for Linksys E4200 V1
//static u1Byte Linksys_E4200_1[3] = {0x58,0x6D,0x8F};

static u1Byte TPLINK_AC1750[3] = {0xE8,0x94,0xF6};
static u1Byte CMW500[6] = {0x00,0x01,0x02, 0x03, 0x04, 05};

u4Byte	RangePerLevel = 0;

/*------------------------ Funtion Declaration-------------------------------------*/
// The following function are called only in MgntLink.c
VOID
RecognizePeer(
	PADAPTER Adapter,
	POCTET_STRING pInmmpdu, 
	PRT_WLAN_BSS bssDesc);


/*------------------------ End of Funtion Declaration------------------------------*/
RT_WLAN_BSS *BssDescDupSource(
	PADAPTER		Adapter,
//	OCTET_STRING	mmpdu
	PRT_RFD			pRfd
)
{
	PMGNT_INFO      	pMgntInfo = &Adapter->MgntInfo;
	u2Byte			i;
	OCTET_STRING	bssIdBeacon,SsidBeacon;
	OCTET_STRING	mmpdu;
	

	mmpdu.Octet = pRfd->Buffer.VirtualAddress;
	mmpdu.Length = pRfd->PacketLength;

	bssIdBeacon.Octet = Frame_Addr3(mmpdu);
	bssIdBeacon.Length = ETHERNET_ADDRESS_LENGTH;

	SsidBeacon = PacketGetElement(mmpdu, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	for(i=0; i<pMgntInfo->NumBssDesc; i++)
	{
		if( PlatformCompareMemory(bssIdBeacon.Octet, pMgntInfo->bssDesc[i].bdBssIdBuf, 6) == 0 )
		{
//			SsidBeacon = PacketGetElement(mmpdu, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);

			// If one of them are hidden, then return dup !!
			if(!IsHiddenSsid(SsidBeacon) && !BeHiddenSsid(pMgntInfo->bssDesc[i].bdSsIdBuf, pMgntInfo->bssDesc[i].bdSsIdLen))
			{	// None of them is hidden AP, check if they have the same SSID
				if( ! CompareSSID(pMgntInfo->bssDesc[i].bdSsIdBuf, pMgntInfo->bssDesc[i].bdSsIdLen, SsidBeacon.Octet, SsidBeacon.Length) )
					continue;
			}
			return &pMgntInfo->bssDesc[i];
		}
	}

	return NULL;
}

//
//Description:
//	Check and find if the input BSS desc is matched with one of the BSS list in MgntInfo by
//	comparison of the SSID and BSSID.
// Arguments:
//	Adapter -
//		NIC adapter context pointer.
//	pRtBSS -
//		The input BSS desc is examined to find which is the matched one in the scan list.
// Return:
//	If one of the BSS desc in the scan list is matched with pRtBSS, then return it.
//	Or return NULL.
// By Bruce, 2008-05-26.
//
PRT_WLAN_BSS 
BssDescDupByDesc(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pRtBSS
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	u2Byte			i;
	
	for(i = 0; i < pMgntInfo->NumBssDesc; i ++)
	{
		if( PlatformCompareMemory(pRtBSS->bdBssIdBuf, pMgntInfo->bssDesc[i].bdBssIdBuf, ETHERNET_ADDRESS_LENGTH) == 0 )
		{

			// 1. Check security !!
			// All Chiper need the same !!
			if( pRtBSS->PairwiseChiper != pMgntInfo->bssDesc[i].PairwiseChiper)
			{
				// Suport differ security !!
				if( CompareSSID( pRtBSS->bdSsIdBuf , pRtBSS->bdSsIdLen , pMgntInfo->bssDesc[i].bdSsIdBuf , pMgntInfo->bssDesc[i].bdSsIdLen  ) )
				{
					RT_TRACE(COMP_SCAN, DBG_LOUD, ("BSSID&SSID the same replace security\n"));
				}
				else
					continue;
			}

			// 2. Check SSID  !!
			// Note :
			//		1. one of BssDes is hidden 
			//		2. security is the same 
			//		3. BSSID is the same
			//		Then they are the same AP !!
			if( !BeHiddenSsid( pRtBSS->bdSsIdBuf , pRtBSS->bdSsIdLen ) && !BeHiddenSsid( pMgntInfo->bssDesc[i].bdSsIdBuf , pMgntInfo->bssDesc[i].bdSsIdLen ))
			{
				if( !CompareSSID( pRtBSS->bdSsIdBuf , pRtBSS->bdSsIdLen , pMgntInfo->bssDesc[i].bdSsIdBuf , pMgntInfo->bssDesc[i].bdSsIdLen  ) )
				{
					// SSID is differ !!
					continue;
				}
			}
			
			return &pMgntInfo->bssDesc[i];
		}
	}
	return NULL;
}

//
//Description:
//	Find the BSS descriptor by the BSSID.
// Arguments:
//	[in] Adapter -
//		NIC adapter context pointer.
//	[in] pBssid -
//		The input BSSID for BSS descriptor searching.
// Return:
//	If one of the BSS desc in the scan list is matched with pBssid, then return it.
//	Or return NULL.
// By Bruce, 2008-05-26.
//
PRT_WLAN_BSS 
BssDescDupByBssid(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pBssid
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	u2Byte			i;
	
	for(i = 0; i < pMgntInfo->NumBssDesc; i ++)
	{
		if( PlatformCompareMemory(pBssid, pMgntInfo->bssDesc[i].bdBssIdBuf, ETHERNET_ADDRESS_LENGTH) == 0 )
		{
			return &pMgntInfo->bssDesc[i];
		}
	}
	return NULL;
}

PRT_WLAN_RSSI
BssDescDupByBssid4Rssi(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pBssid
	)
{
	u2Byte			i;
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	
	for(i = 0; i < pMgntInfo->NumBssDesc4Rssi; i ++)
	{
		if( PlatformCompareMemory(pBssid, pMgntInfo->bssDesc4Rssi[i].bdBssIdBuf, ETHERNET_ADDRESS_LENGTH) == 0 )
		{
			return &pMgntInfo->bssDesc4Rssi[i];
		}
	}
	
	return NULL;
}

//
// Description:
//	Check QBss Load of the BSS setting if it is sufficient to join especially for 
//	CCX CAC roaming.
// Argument:
//	Adapter -
//		NIC adapter context pointer.
//	pRtBss -
//		The BSS to be checked if it meets the specified settings.
// Return:
//	TRUE if the BSS has sufficient utilization, or return FALSE.
// By Bruce, 2009-02-12.
//
BOOLEAN
CheckQBssLoad(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pRtBss
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	PQOS_TSTREAM	pTs = NULL;
	
	//
	// Determine the insufficient AAC in QBSS Load.
	//
	if (CCX_CAC_IsVoiceTsExist(Adapter))
	{
		if(pRtBss->BssQos.bQBssLoadValid)
		{
			pTs = QosGetTs(Adapter, 0, pMgntInfo->Bssid, Adapter->CurrentAddress);
			if (pTs && pTs->bUsed)
			{
				u2Byte	aac = GET_QBSS_LOAD_AVAILABLE_CAPACITY(pRtBss->BssQos.QBssLoad);
				u2Byte	mtime = GET_TSPEC_MEDIUM_TIME(pTs->TSpec);
				RT_PRINT_ADDR(COMP_QOS, DBG_LOUD,"CheckQBssLoad(): ", pRtBss->bdBssIdBuf);
				RT_TRACE(COMP_QOS, DBG_LOUD, ("CheckQBssLoad(): AAC = %d, MediumTime = %d\n", aac, mtime));
				if ( aac < mtime )
				{
					RT_TRACE(COMP_QOS, DBG_LOUD, ("CheckQBssLoad(): insufficient bandwith, skip this bss\n "));
					return FALSE;	
				}
			}
		}
	}
	return TRUE;
}

//
// Description:
//	Check BSS setting such as security ...
//	2006.11.15, by shien chang.
//	Move from "N6C_OidSet.c to here for the reference of SelectNetworkBySSID().
// Argument:
//	Adapter -
//		NIC adapter context pointer.
//	pRtBss -
//		The BSS to be checked if it meets the specified settings.
// Return:
//	TRUE if the BSS matches driver's setting, or return FALSE.
// Moved by Bruce, 2009-02-12.
//
BOOLEAN
CheckBSSSetting(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pRtBss
	)
{
	PRT_SECURITY_T		pSecurityInfo = &(Adapter->MgntInfo.SecurityInfo);
	RT_ENC_ALG			PairwiseEncAlgo = pSecurityInfo->PairwiseEncAlgorithm;
	BOOLEAN 			bPassCheck = TRUE;
#if (WPS_SUPPORT == 1)	
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);

	pMgntInfo->bWPSProcess = FALSE;

	if(pSimpleConfig->bEnabled)
	{
		RT_TRACE(COMP_MLME, DBG_TRACE, ("CheckBSSSetting(): skip security check when wps in progress.\n"));
		return bPassCheck;
	}
#endif	
	do
	{		
		if( (pRtBss->bdCap & cPrivacy) )
		{
			if(PairwiseEncAlgo == RT_ENC_ALG_NO_CIPHER )
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("CheckBSSSetting(): BSS Cap shows privacy, but PairwiseEncAlgo is RT_ENC_ALG_NO_CIPHER\n"));
				bPassCheck = FALSE;
				break;
			}
#if (WPS_SUPPORT == 1)	
			else if ( (pSecurityInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40) &&
				(pRtBss->PairwiseChiper > NONE_WPA) &&
				(pSimpleConfig->bEnabled == FALSE))
			{
				pMgntInfo->bWPSProcess = TRUE;
			}
#endif
		}
		else
		{
#if (WPS_SUPPORT == 1)	
			if ( (pSecurityInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40) &&
				(pRtBss->PairwiseChiper == NONE_WPA) )
			{
				pMgntInfo->bWPSProcess = TRUE;
			}
#endif
			if( PairwiseEncAlgo != RT_ENC_ALG_NO_CIPHER )
			{
				//
				// Note that this is for Win7 NDISTest ExcludeUnencrypted_ext preview 3, 
				// 2008.11.24, haich.
				//
				if(Adapter->MgntInfo.bExcludeUnencrypted)
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("CheckBSSSetting(): BSS Cap shows no privacy, but PairwiseEncAlgo is %d\n", PairwiseEncAlgo));
					bPassCheck = FALSE;
					break;	
				}
			}
		}

		//
		// Check encryption setting.
		//
		if(pSecurityInfo->SecLvl > RT_SEC_LVL_NONE)
		{
			//
			// The CCKM may be included in WPA2 or WPA IE, we should modify the SEC Level according
			// to the auth mode from the BSS. By Bruce, 2011-02-09.
			// <ToDo> It is not a good solution to set the security level here.
			//
			if(pSecurityInfo->AuthMode == RT_802_11AuthModeCCKM)
			{
				if(pSecurityInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP)
				{
					// Check BSS support WPA2 TKIP
					if(pRtBss->PairwiseChiper& WPA2_AES)
					{
						bPassCheck = TRUE;
						break;
					}
					else if(pRtBss->PairwiseChiper& WPA_AES)
					{
						bPassCheck = TRUE;
						break;
					}
					else
					{
						RT_PRINT_STR(COMP_MLME | COMP_SEC, DBG_LOUD, " RT_802_11AuthModeCCKM and PairwiseEnc = RT_ENC_ALG_AESCCMP but this BSS doesn't has AES for SSID = ", pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen);
						bPassCheck = FALSE;
						break;
					}
				}
				else if(pSecurityInfo->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP)
				{
					// Check BSS support WPA2 TKIP
					if(pRtBss->PairwiseChiper& WPA2_TKIP)
					{
						bPassCheck = TRUE;
						break;
					}
					else if(pRtBss->PairwiseChiper& WPA_TKIP)
					{
						bPassCheck = TRUE;
						break;
					}
					else
					{
						RT_PRINT_STR(COMP_MLME | COMP_SEC, DBG_LOUD, " RT_802_11AuthModeCCKM and PairwiseEnc = RT_ENC_ALG_TKIP but this BSS doesn't has TKIP for SSID = ", pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen);
						bPassCheck = FALSE;
						break;
					}
				}
				else
				{
					RT_TRACE_F(COMP_SEC, DBG_WARNING, ("We don't support this encryption (0x%08X) for RT_802_11AuthModeCCKM!!!\n", pSecurityInfo->PairwiseEncAlgorithm));
					bPassCheck = FALSE;
					break;
				}
			}
			else
			{
				switch( pSecurityInfo->SecLvl )
				{
					case RT_SEC_LVL_WPA :
						if( pSecurityInfo->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP )
						{
							// Check BSS support WPA TKIP
							if( !(pRtBss->PairwiseChiper & WPA_TKIP) )
							{
								RT_PRINT_STR(COMP_MLME | COMP_SEC, DBG_LOUD, " PairwiseEnc = WPA_TKIP but this BSS doesn't has TKIP for SSID = ", pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen);
								bPassCheck = FALSE;
								break;
							}
						}
						else if( pSecurityInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP )
						{
							// Check BSS support WPA TKIP
							if( !(pRtBss->PairwiseChiper & WPA_AES) )
							{
								RT_PRINT_STR(COMP_MLME | COMP_SEC, DBG_LOUD, " PairwiseEnc = WPA_AES but this BSS doesn't has AES for SSID = ", pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen);
								bPassCheck = FALSE;
								break;
							}	
						}
						break;
					case RT_SEC_LVL_WPA2 :
						if( pSecurityInfo->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP )
						{
							// Check BSS support WPA2 TKIP
							if( !(pRtBss->PairwiseChiper & WPA2_TKIP) )
							{
								RT_PRINT_STR(COMP_MLME | COMP_SEC, DBG_LOUD, " PairwiseEnc = WPA2_TKIP but this BSS doesn't has TKIP for SSID = ", pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen);
								bPassCheck = FALSE;
								break;
							}
						}
						else if( pSecurityInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP )
						{
							// Check BSS support WPA2 TKIP
							if( !(pRtBss->PairwiseChiper& WPA2_AES) )
							{
								RT_PRINT_STR(COMP_MLME | COMP_SEC, DBG_LOUD, " PairwiseEnc = WPA2_AES but this BSS doesn't has AES for SSID = ", pRtBss->bdSsIdBuf, pRtBss->bdSsIdLen);
								bPassCheck = FALSE;
								break;
							}	
						}
						break;
					default :
						RT_TRACE(COMP_MLME, DBG_LOUD , ("SelectNetworkBySSID() : secLvl = %x , Pairwise chiper = %x\n",pSecurityInfo->SecLvl, pSecurityInfo->PairwiseEncAlgorithm) );					
				}
			}
		}

	} while (FALSE);

	return bPassCheck;
}

VOID
CheckRateSet(
	POCTET_STRING	pSuppRateSet
	)
{
	u1Byte	index0 = 0, index1=0, index2=0;
	u1Byte	AvailRateTable[] = {0x2, 0x4, 0xb, 0x16, 0xc, 0x12, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c};

	// Trace each rate in the SuppRateSet.
	for(index0=0; index0<pSuppRateSet->Length; index0++)
	{
		// Check if the rate is available.
		for(index1=0; index1<sizeof(AvailRateTable); index1++)
		{
			if((pSuppRateSet->Octet[index0]&0x7f) == AvailRateTable[index1])
				break;
		}

		// If the rate is available, it will be keep in the SuppRateSet.
		// If it is not, it will be clear.
		if(index1 < sizeof(AvailRateTable))
		{
			pSuppRateSet->Octet[index2] = pSuppRateSet->Octet[index0];
			index2++;
		}
	}

	pSuppRateSet->Length = index2;
}

//
// Description:
//	The callback function in client mode for the tx packet feedback. This callback checks  the null packet was sent successfully and
//	start scan process immediately.
// Arguments:
//	[in] pAdapter -
//		The adapter address of the caller.
//	[in] pContext -
//		The information address of RT_TX_FEEDBACK_INFO.
// Return:
//	NULL
// Assumption:
//	The RT_TX_SPINLOCK is acquired.
// Note:
//	If the null packet wasn't sent to AP, we should send this null packet again or skip this scan process.
//	Currently, we don't skip this scan process or resend this packet because we don't make sure the tx report
//	is robust which means the report may be lost.
// By Bruce, 2011-10-25.
//
VOID
mgntScanTxFeedbackCallback(
	IN	PADAPTER						pAdapter,
	const RT_TX_FEEDBACK_INFO * const 	pTxFeedbackInfo
	)
{
	PADAPTER				pTargetAdapter = pTxFeedbackInfo->pAdapter;
	PMGNT_INFO				pTargetMgntInfo=&pTargetAdapter->MgntInfo;
	
	PMGNT_INFO				pMgntInfo=&pAdapter->MgntInfo;

	if(NULL == pTxFeedbackInfo)
		return;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("mgntScanTxFeedbackCallback()=====>\n"));

	// This tx packet feedback reason doen't match.
	if(!(pTxFeedbackInfo->Reason & RT_TX_FEEDBACK_ID_SCAN_TX_NULL))
		return;

	// The scan state is invalid
	if(!pMgntInfo->bScanInProgress)
		return;

	//3 // <ToDo> Check the packet is transmitted ok

	RT_TRACE(COMP_MLME, DBG_WARNING, ("Scan callback immediately!\n"));

	//Using target adapter to make sure set the same scan process.
	PlatformSetTimer(pTargetAdapter, &pTargetMgntInfo->ScanTimer, 0);	
}


u2Byte
scanGetScanTimer(
	IN	PADAPTER	pAdapter
	)
{
	PADAPTER				ptmpAdapter = GetDefaultAdapter(pAdapter);
	PMGNT_INFO				ptmpMgntInfo = NULL;
	PMGNT_INFO				pMgntInfo=&pAdapter->MgntInfo;
	RT_LIST_ENTRY			nullPktList;
	PRT_LIST_ENTRY			pEntry = NULL;
	u2Byte					scanTimer = 0;
	PRT_TX_LOCAL_BUFFER 	pBuf = NULL;
	PRT_TCB					pTcb = NULL;
	u8Byte					curTime = PlatformGetCurrentTime();
	u8Byte					lastActionTime = MultiportGetLastConnectionActionTime(pAdapter);

	if(curTime < MultiportGetLastConnectionActionTime(pAdapter) + 5 * 1000000)
	{
		scanTimer = 1000;
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Delay scan for curTime (%d), last (%d)\n",
				(u4Byte)(curTime & 0xFFFFFFFF), (u4Byte)(lastActionTime & 0xFFFFFFFF)));
	}

	RTInitializeListHead(&nullPktList);

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
	
	// check each adapter if it is associated to the AP.
	do
	{
		ptmpMgntInfo = &ptmpAdapter->MgntInfo;

		// No need to care AP mode.
		if(ACTING_AS_AP(ptmpAdapter))
			continue;

		// If the client is not assciated or connected.
		if(!ptmpMgntInfo->mAssoc || !ptmpMgntInfo->bMediaConnect || MgntRoamingInProgress(ptmpMgntInfo))
			continue;

		if(!MgntGetBuffer(pAdapter, &pTcb, &pBuf))
			break;

		ConstructNullFunctionData(
						ptmpAdapter, 
						pBuf->Buffer.VirtualAddress,
						&pTcb->PacketLength,
						ptmpMgntInfo->Bssid,
						FALSE,
						0,
						FALSE,
						TRUE);
		pTcb->pAdapter = ptmpAdapter;

		scanTimer += SCAN_DELAY_TIME_MS_SHORT;

		RTInsertTailList(&nullPktList, &pTcb->List);
	}while(NULL != (ptmpAdapter = GetNextExtAdapter(ptmpAdapter)));

	while(RTIsListNotEmpty(&nullPktList))
	{
		pEntry = RTRemoveHeadList(&nullPktList);
		pTcb = (PRT_TCB)CONTAINING_RECORD(pEntry, RT_TCB, List);

		if(RTIsListEmpty(&nullPktList))
		{
			if(TxFeedbackInstallTxFeedbackInfoForTcb(pAdapter, pTcb))
			{
				TxFeedbackFillTxFeedbackInfoUserConfiguration(
							pTcb, 
							RT_TX_FEEDBACK_ID_SCAN_TX_NULL, 
							pAdapter, 
							mgntScanTxFeedbackCallback, 
							(PVOID)(pTcb->pAdapter)
						);
				scanTimer = SCAN_DELAY_TIME_MS_LONG;
			}
			else
			{
				scanTimer = 100;
			}
		}
		// Set pTcb->bTxUseDriverAssingedRate as "TRUE" for fixed rate packet, by hana 2015/04/13
		pTcb->bTxUseDriverAssingedRate = TRUE;
		MgntSendPacket(pTcb->pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, pMgntInfo->LowestBasicRate);
	}

	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

	return scanTimer;
}
	


//
// Description:
//	To simplify the procedure in the MlmeAssociateConfirm(), segment the Association OK to here.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu - 
//		The poniter of the current packet to the 802.11 header.
//	[in] bReassociate -
//		TRUE if this packet is Reassociation response packet. FALSE if this packet is association response packet.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	By determining the result status code, handle the assoication process for client mode.
// Revised by Bruce, 2012-03-09.
//
RT_STATUS
MlmeOnAssocOK(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu,
	IN	BOOLEAN			bReassociate
	)
{
	PMGNT_INFO      		pMgntInfo = &Adapter->MgntInfo;
	PSTA_QOS				pStaQos = pMgntInfo->pStaQos;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	OCTET_STRING			SuppRateSet;
	u1Byte					SuppRateSetBuf[MAX_NUM_RATES];
	OCTET_STRING			ExtSuppRateSet;
	BOOLEAN					bFilterOutNonAssociatedBSSID = FALSE;
	u1Byte					RetryLimit = HAL_RETRY_LIMIT_INFRA;
	BOOLEAN					bTypeIbss = FALSE; 
	PADAPTER				pExtAdapter = GetFirstAPAdapter(Adapter) ? GetFirstAPAdapter(Adapter) : GetDefaultAdapter(Adapter);
	BOOLEAN					bUseRAMask = FALSE;
	BOOLEAN					bConnected =TRUE;
	u1Byte					AutoChnlSelOp = 0;
	HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T				pDM_Odm = &pHalData->DM_OutSrc;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return RT_STATUS_FAILURE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return RT_STATUS_FAILURE;
	}

	// Join a BSS !!!!
	RT_TRACE(COMP_MLME, DBG_LOUD, ("Join a BSS succeeded!\n")); 
	pMgntInfo->mAssoc = TRUE;
	pMgntInfo->mIbss = FALSE;
	pMgntInfo->mDisable = FALSE;
	pMgntInfo->mAId = Frame_AssocAID(*posMpdu);
	pMgntInfo->AsocTimestamp = PlatformGetCurrentTime();
	pMgntInfo->bStartDelayActionFrame = TRUE;
	pMgntInfo->CntAfterLink = 0;
	Adapter->LedTxCnt=0;
	Adapter->LedRxCnt=0;

	//Move MacId Register to SetupJoinInfraInfo(), by hana 2015/09/02
	// -- HalMacId: Set a specific MacId for this client ---
	//pMgntInfo->mMacId = (u1Byte)MacIdRegisterMacIdForInfraClient(Adapter);
	//pMgntInfo->mcastMacId = (u1Byte)MacIdRegisterMacIdForMCastClient(Adapter);
	// -------------------------------------------
	
	//
	//	Add Assoc OK counter !!
	//
	if(pMgntInfo->SecurityInfo.SecLvl > RT_SEC_LVL_NONE )
		pMgntInfo->mDeauthCount++;

	// Set basic/operational rate set.
	// (1) Copy Supported Rate Set to a local bufer.
	// 20110511 Joseph: Add length checking to prevent from RateSetBuf overflow.
	SuppRateSet = PacketGetElement(*posMpdu, EID_SupRates, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	CopyMem( SuppRateSetBuf, SuppRateSet.Octet, (SuppRateSet.Length>MAX_NUM_RATES)?MAX_NUM_RATES:SuppRateSet.Length );
	FillOctetString( SuppRateSet, SuppRateSetBuf, SuppRateSet.Length );
	// (2) Get Extened Supported Rate Set.
	ExtSuppRateSet = PacketGetElement(*posMpdu, EID_ExtSupRates, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	// (3) If Extened Supported Rate Set exists, append it to SuppRateSet (local buffer).
	// 20110511 Joseph: Add length checking to prevent from RateSetBuf overflow.
	if( (ExtSuppRateSet.Length != 0) && (SuppRateSet.Length<MAX_NUM_RATES))
	{
		if(SuppRateSet.Length + ExtSuppRateSet.Length > MAX_NUM_RATES)
		{
			CopyMem( SuppRateSet.Octet+SuppRateSet.Length, ExtSuppRateSet.Octet, MAX_NUM_RATES-SuppRateSet.Length);
			SuppRateSet.Length = MAX_NUM_RATES;
		}
		else
		{
			CopyMem( SuppRateSet.Octet+SuppRateSet.Length, ExtSuppRateSet.Octet, ExtSuppRateSet.Length);
			SuppRateSet.Length += ExtSuppRateSet.Length;
		}
	}

	// 20110511 Joseph: Check if the rate set is correct.
	CheckRateSet(&SuppRateSet);
	if(SuppRateSet.Length == 0)
	{
		SuppRateSet.Length = 1;
		SuppRateSet.Octet[0] = (IS_WIRELESS_MODE_5G(Adapter))?0x8c:0x82;
	}

	//trigger wapi thread
	WAPI_SecFuncHandler(WAPI_MLMEONASSOCOK, Adapter, (PVOID)posMpdu, WAPI_END);

	// 2007.07.17 lanhsin Reset issue for security.
	//In CCKM Fast Roaming , we can't reset key in Assoc Rsp it will cause Fail
	if (Adapter->ResetProgress == RESET_TYPE_NORESET  &&  pMgntInfo->SecurityInfo.AuthMode != RT_802_11AuthModeCCKM)
	{
		// Clear KeyLen[] for TKIP/AES.
		// 2004.10.09, by rcnjko.
		if( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_TKIP ||
			pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP )
		{
			PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo); 
			u1Byte	i = 0;
	
			for(i = 0; i < KEY_BUF_SIZE; i++)
			{
				PlatformZeroMemory( pSecInfo->KeyBuf[i], MAX_KEY_LEN );
				pSecInfo->KeyLen[i] = 0;
			}	
		}
	}

	//
	//Set HW Register
	//
	pMgntInfo->dot11CurrentPreambleMode = PREAMBLE_AUTO;	
	// 2. BSSID
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pMgntInfo->Bssid );
	// 3. MSR
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode) );
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_RETRY_LIMIT, (pu1Byte)(&RetryLimit));
	// 4. BcnIntv 
	//
	//We only need to update beacon interval when IBSS, or it will fail when in WLK 1.4 test (VwifiInfraSoft_ext).
	//It will cause update tsf too long so it will loss packet. By Maddest 06052009
	//
	//if(pMgntInfo->mIbss)
	// We should set beacon interval register for client mode otherwise it will cause TBTT does not sync with AP.
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BEACON_INTERVAL, (pu1Byte)(&pMgntInfo->dot11BeaconPeriod) );
	// 5. Slot time
	// To solve Win98 IRQL==PASIVE_LEVEL in timer callback function, 
	// we call ActUpdate_mCapInfo() here to make sure related IOs are perfomed in correct IRQL, 
	// that is, DISPATCH_LEVEL=>Asyn IO for this case. 2005.03.15, by rcnjko.
	ActUpdate_mCapInfo(Adapter, Frame_AssocCap(*posMpdu));

	// 6. Update QoS parameters. Added by Annie, 2005-11-12.
	if( pMgntInfo->pStaQos->CurrentQosMode > QOS_DISABLE )
	{
		QosOnAssocRsp( Adapter, *posMpdu );
	}
	else
	{ // Update AC_PARAM: follow Legacy setting. Annie, 2006-03-31.
		QosSetLegacyACParam(Adapter);
	}

	//
	// 7. Update HT parameters
	//
	if (IS_NEED_EXT_IQK(Adapter))
	{
		GET_HAL_DATA(Adapter)->bNeedIQK = TRUE;
		GET_HAL_DATA(Adapter)->DM_OutSrc.RFCalibrateInfo.bNeedIQK = TRUE;
	}

	if(pMgntInfo->pHTInfo->bCurrentHTSupport)
	{
		HTOnAssocRsp(Adapter, *posMpdu);

		if(pMgntInfo->pVHTInfo->bCurrentVHTSupport)
			VHTOnAssocRsp(Adapter, *posMpdu);		

		CHNL_SetBwChnlFromPeer(Adapter);
	}
	else
	{	
		PlatformZeroMemory(pMgntInfo->dot11HTOperationalRateSet, 16);
		CHNL_SetBwChnl(	Adapter, 
							pMgntInfo->dot11CurrentChannelNumber,
					 		CHANNEL_WIDTH_20, 
					 		EXTCHNL_OFFSET_NO_EXT
					 		);
	}

	// (4) Select Rate Set.
	SelectRateSet( Adapter, SuppRateSet );

	Adapter->HalFunc.AllowErrorPacketHandler(Adapter, TRUE, TRUE);
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_ENABLE_TXOK_INTERRUPT, (pu1Byte)&bConnected);


	//
	// 8. Update RATR table
	//	   This function shall be called after HTOnAssocRsp() because we will select Tx MCS rate
	//	   in HTOnAssocRsp() function.
	//
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USE_RA_MASK, &bUseRAMask);
	if(bUseRAMask)
	{
		// we only need to set rate mask
		pMgntInfo->Ratr_State = 0;
		Adapter->HalFunc.UpdateHalRAMaskHandler(Adapter, pMgntInfo->mMacId, NULL, pMgntInfo->Ratr_State);
		Adapter->HalFunc.UpdateHalRAMaskHandler(Adapter, pMgntInfo->mcastMacId, NULL, pMgntInfo->Ratr_State);
	}
	else
	{
		Adapter->HalFunc.UpdateHalRATRTableHandler(
									Adapter, 
									&pMgntInfo->dot11OperationalRateSet,
									pMgntInfo->dot11HTOperationalRateSet,NULL);
	}

	//
	// 2015/04/16 MH According to ODM input parameters, we need to send default port HAL datastructure's ODM 
	// Otherwise, ext port will send null HAL pointer and cause BSOD.
	//
	GetDefaultAdapter(Adapter)->HalFunc.RAPostActionHandler(&(((PHAL_DATA_TYPE)GetDefaultAdapter(Adapter)->HalData)->DM_OutSrc));

	// Update RTS init rate after we set RA MASK. by tynli. 2010.11.25.
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_INIT_RTS_RATE, (pu1Byte)&Adapter);

	// 9. Set UsbRxAggr parameters
	// 	 This is just a temporary fix
#if (RX_AGGREGATION/* && HARDWARE_TYPE_IS_RTL8192D != 1*/)
	if(!Adapter->bInHctTest)
	{
		if(!pMgntInfo->bWiFiConfg || IS_SET_RX_AGGR(Adapter))
		{
			PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
			PlatformScheduleWorkItem(&pHalData->RxAggrSettingWorkItem);
		}
	}
#endif

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));
		return RT_STATUS_FAILURE;
	}	
	
	if (Adapter->ResetProgress == RESET_TYPE_NORESET)
	{	
		pMgntInfo->uConnectedTime = PlatformGetCurrentTime();
		MgntIndicateMediaStatus( Adapter, RT_MEDIA_CONNECT,  FORCE_INDICATE );
	}
	else
		MgntIndicateMediaStatus( Adapter, RT_MEDIA_CONNECT,  GENERAL_INDICATE);

	// Half the time required to determine if we are disconneted. 2005.03.10, by rcnjko.
	//Adapter->MgntInfo.LinkDetectInfo.SlotNum = 4 * (1 + pMgntInfo->dot11BeaconPeriod/500);
	if(pMgntInfo->NdisVersion >= RT_NDIS_VERSION_6_20 && Adapter->bInHctTest)	// Neo test 123
		Adapter->MgntInfo.LinkDetectInfo.SlotNum = 1;
	else
		Adapter->MgntInfo.LinkDetectInfo.SlotNum = 2 * (1 + pMgntInfo->dot11BeaconPeriod/500);

	RT_ASSERT(pMgntInfo->LinkDetectInfo.SlotNum < RT_MAX_LD_SLOT_NUM, ("SlotNum exceeds!\n") );

	// To prevent the immediately calling MgntLinkStatusWatchdog after association. Annie, 2005-04-22.
	if( pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod==0 ||
		pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod==0 )
	{
		pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod = 1;
		pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod= 1;
	}

	CCX_OnAssocOk(Adapter, posMpdu, bReassociate);

	TDLS_OnAsocOK(Adapter, *posMpdu);

	//[LPS] Reset LPS Setting
	pPSC->LpsIdleCount=0;

	// Remove A2 entry since NO ICs support this
	// Set A2 entry when starting win7 soft AP, otherwise, set CHECK_BSSID
	/*
	if(bUseRAMask &&
		((MgntActQuery_ApType(pExtAdapter) == RT_AP_TYPE_VWIFI_AP) ||
			(MgntActQuery_ApType(pExtAdapter) == RT_AP_TYPE_EXT_AP)))
	{
	
		// 2009.05.22 hpfan modify for fill A2 entry by AP's bssid
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_FILL_A2ENTRY, pExtAdapter->MgntInfo.Bssid);
	}
	else
	*/
	{
		// 2007/10/16 MH MAC Will update TSF according to all received beacon, so we have
		// To set CBSSID bit when link with any AP. 
		if(!IsAPModeExist(Adapter))
		{
			bFilterOutNonAssociatedBSSID = TRUE;
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
		}
				
		//Correct TSF value. Suggest by Scott. Added by tynli. 2009.12.01.
		bTypeIbss = FALSE;

		if(MultiChannelSyncHwTsfNeeded(Adapter))
		{
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CORRECT_TSF, (pu1Byte)(&bTypeIbss));
		}
	}

	// ACM parameter update timer, by Bruce, 2008-03-21.
	if (pStaQos->CurrentQosMode > QOS_DISABLE) 
	{
		PlatformSetTimer(Adapter, &pStaQos->ACMTimer, ACM_TIMEOUT);
	}

	// Clear all rejected AP record, 2006.11.21, by shien chang.
	MgntClearRejectedAsocAP(Adapter);
	
	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 3\n"));
		return RT_STATUS_FAILURE;
	}	
	
	// 2006.11.15, by shien chang.
	DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_SUCCESS);

	(MgntRoamingInProgress(pMgntInfo)) ?
		MgntRoamComplete(Adapter, MlmeStatus_success) :
		DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_SUCCESS);

	MgntResetJoinCounter(pMgntInfo);
	MgntResetRoamingState(pMgntInfo);

	// From MSDN:
	//	After the 802.11 station successfully associates with an AP or peer station, the miniport driver must make
	//	an NDIS_STATUS_DOT11_LINK_QUALITY indication shortly after it makes the
	//	NDIS_STATUS_DOT11_ASSOCIATION_COMPLETION indication.
	//	The WlanTimedLinkQuality rule specifies the NDIS_STATUS_DOT11_LINK_QUALITY indication is made in 15
	//	seconds after a successful NDIS_STATUS_DOT11_ASSOCIATION_COMPLETION.
	// By Bruce, 2015-04-29.
	LinkQualityReportCallback(Adapter);

	P2P_OnAssocOK(Adapter, pRfd, posMpdu);
	WFD_OnAssocOK(Adapter, pRfd, posMpdu);

#if (AUTO_CHNL_SEL_NHM == 1)
	if(IS_AUTO_CHNL_SUPPORT(Adapter)){
		RT_TRACE(COMP_P2P, DBG_LOUD, ("[ACS] MlmeOnAssocOK(): Reset ACS statistics\n"));
                        Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AUTO_CHNL_SEL, (pu1Byte)&AutoChnlSelOp);
	}
#endif

	//2013.11.18 Sinda BK set the same settings with BK. 
	//Otherwise BK have a lower priority and have more packet loss.
	if( Adapter->bInHctTest == TRUE )
	{
		PlatformEFIOWrite1Byte(Adapter, REG_EDCA_BK_PARAM, 43);
	}

	FunctionOut(COMP_MLME);
	return RT_STATUS_SUCCESS;
}

//
// Description:
//	Select another BSS to join, and the selected BSS is not in the reected list.
// Arguments:
//	Adapter -
//		NIC adapter context pointer.
// Return:
//	TRUE if an BSS had been selected to perform join or roam, or FALSE.
// Note:
// 	(1) Because "JoinTimeout(), MlmeAuthenticateRequest_Confirm(), 
//	and MlmeAssociateConfirm()" perform the same process, I merged 
//	them into this function.
//	(2) Before calling this function, be sure that we have not indicated 
//	"AssocitionStart" event, or have indicated "AssocitionComplete"
//	event to OS. Calling it will restart a complete association procedure.
//	(3) This function won't indicate one of the events of "RoamingStart",
//	"RoamingComplete", and "ConnectionComplete" to OS.
// By Bruce, 2009-02-12.
//
BOOLEAN
MgntConnectOtherBss(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	BOOLEAN			bConnect = FALSE;
	PRT_WLAN_BSS	pRtBss;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 3\n"));				
		return FALSE;
	}

	if(PlatformAllocateMemory(Adapter, (PVOID*)&pRtBss, sizeof(RT_WLAN_BSS))  != RT_STATUS_SUCCESS)		
		return FALSE;
	
	if(MgntIsTimeOutForIndication(pMgntInfo))
	{
		PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
		return FALSE;
	}
	
	if(MgntRoamingInProgress(pMgntInfo))
	{ // Roaming.
		// Roaming retry
		if(MgntRoamRetry(Adapter, FALSE)) // Raom again.
			bConnect = TRUE;
		else
		{
			if(MgntResetOrPnPInProgress(Adapter))
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 3\n"));
				PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
				
				return FALSE;
			}
				
			if(SelectNetworkBySSID(Adapter, &(pMgntInfo->Ssid), FALSE, pRtBss) == RT_JOIN_INFRA)
			{
				if(!MgntIsInRejectedAPList(Adapter, pRtBss->bdBssIdBuf))
				{ // Do not join the rejected AP.
					MgntResetJoinCounter(pMgntInfo);
					JoinRequest(Adapter, pMgntInfo->JoinAction, pRtBss);
					bConnect = TRUE;
				}
			}		
		}
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntConnectOtherBss(): Roam Retry = %d!\n", bConnect));
	}
	else
	{ // Fisrt connection.
		// Has the other AP with the same SSID
		if(SelectNetworkBySSID(Adapter, &(pMgntInfo->Ssid), FALSE, pRtBss) == RT_JOIN_INFRA)
		{
			if(!MgntIsInRejectedAPList(Adapter, pRtBss->bdBssIdBuf))
			{ // Do not join the rejected AP.
				MgntResetJoinCounter(pMgntInfo);
				JoinRequest(Adapter, pMgntInfo->JoinAction, pRtBss);
				bConnect = TRUE;
			}
		}
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntConnectOtherBss(): Connect other = %d!\n", bConnect));
	}
	PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));

	return bConnect;
}


//
//	Description:
//		Translate freq_channel (Hz or channel number) to an valid channel number.
//
u1Byte
ToLegalChannel(
	PADAPTER		Adapter,
	u8Byte			freq_channel
)
{
	PRT_CHANNEL_LIST	pChanneList = NULL;
	u2Byte				i;
	u2Byte				firstLegalIndex=0xff;

	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST, NULL, &pChanneList);

	for(i = 0; i < pChanneList->ChannelLen; i ++)
	{
		if(ACTING_AS_AP(Adapter))
		{
			if(!DFS_ApChnlLocked(Adapter, pChanneList->ChnlListEntry[i].ChannelNum))
			{
				if(firstLegalIndex == 0xff)
					firstLegalIndex = i;
				if(freq_channel == pChanneList->ChnlListEntry[i].ChannelNum)
				{
					return pChanneList->ChnlListEntry[i].ChannelNum;
				}
			}
		}
		else
		{
			if(firstLegalIndex == 0xff)
				firstLegalIndex = 0;
			if(freq_channel == pChanneList->ChnlListEntry[i].ChannelNum)
			{
				return pChanneList->ChnlListEntry[i].ChannelNum;
			}
		}

		if( (pChanneList->ChnlListEntry[i].ChannelNum <= 14) && (freq_channel == DSSS_Freq_Channel[pChanneList->ChnlListEntry[i].ChannelNum]*1000) )
		{
			return pChanneList->ChnlListEntry[i].ChannelNum;
		}
	}

	// Prefast warning C6385: Reading invalid data from 'pChanneList->ChnlListEntry':  the readable size is '760' bytes, but '5120' bytes may be read.
	// false positive, disable it here.
#pragma warning( disable: 6385 )
	return pChanneList->ChnlListEntry[firstLegalIndex].ChannelNum;
}

BOOLEAN IsPeer11G(
	RT_WLAN_BSS	*bssDesc
)
{
	u2Byte i;
	u1Byte rate;

	// Check if it contains OFDM rates.
	for(i = 0; i < bssDesc->bdSupportRateEXLen; i++)
	{
		rate = bssDesc->bdSupportRateEXBuf[i] & 0x7f;
		if( !MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_B) &&
			MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_G) )
		{ 
			return TRUE;
		}
	}

	return FALSE;
}


BOOLEAN 
IsPeer11B(
	RT_WLAN_BSS	*bssDesc
)
{
	u2Byte i;
	u1Byte rate;
	
	// Check if it contains OFDM rates.
	for(i = 0; i < bssDesc->bdSupportRateEXLen; i++)
	{
		rate = bssDesc->bdSupportRateEXBuf[i] & 0x7f;
		if(MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_B))
		{ 
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN 
Is11bBasicRate(
	RT_WLAN_BSS	*bssDesc
)
{
	u2Byte i;
	u1Byte rate;
	
	// Check if it contains OFDM rates.
	for(i = 0; i < bssDesc->bdSupportRateEXLen; i++)
	{
		// Check 802.11B Basic Rate
		if(bssDesc->bdSupportRateEXBuf[i] & 0x80)
		{
			rate = bssDesc->bdSupportRateEXBuf[i] & 0x7f;
			if(!MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_B))
			{ 
				return FALSE;
			}
		}
	}

	return TRUE;
}


//
// Parse Realtek information element (sent from 8186). Added by Roger, 2006.12.07.
//
VOID
GetRealtekIEContentForTurboMode(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pBssDesc,
	IN	POCTET_STRING	pRealtekIE
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u1Byte			TypeOffset;
	PRT_TURBO_MODE_IE_VALUE	pRTIEValue;

	// Turbo mode switch check.
	if( !pMgntInfo->bAutoTurboBy8186 && !pMgntInfo->bSupportTurboMode )
	{
		return;
	}

	// Input data Check.
	if( !pBssDesc || !pRealtekIE->Octet )
	{		
		return;
	}


	// Length check.
	// OUI(3), first type and length: total 5 bytes.
	//
	// 00-E0-4C-(Type)-(Length)
	// 01 02 03   04     05
	if( pRealtekIE->Length <= (SIZE_OUI + SIZE_TYPE_AND_LEN) )	// 5 bytes
	{
		return;
	}

	TypeOffset = SIZE_OUI;	// 3 bytes OUI: Jump out "00-E0-4C".
	do
	{
		pRTIEValue = (PRT_TURBO_MODE_IE_VALUE)( pRealtekIE->Octet + TypeOffset );

		// Length Check.
		if( (TypeOffset+SIZE_TYPE_AND_LEN+pRTIEValue->Length) > pRealtekIE->Length )
		{
			break;
		}

		switch( pRTIEValue->Type )
		{
		case eRT_IE_TYPE1_TURBO_MODE:
			{ // Type 1: Capability.
				RT_ASSERT( ( pRTIEValue->Length == sizeof(u2Byte) ), ("GetRealtekIEContentForTurboMode(): Length of type 1(Capability) is not 2 bytes!!!!! Go to tell SD4!\n") );
				pBssDesc->bRealtekCapType1Exist  = TRUE;
				PlatformMoveMemory(&(pBssDesc->RealtekCap),  &(pRTIEValue->Value), 2);
			}
			break;

		default:
			break;
		}

		TypeOffset += (SIZE_TYPE_AND_LEN+pRTIEValue->Length);
	}while(TRUE);
}

u2Byte 
RemoveWcnIeFromMmpdu(
	IN OUT	POCTET_STRING posMmpdu
)
/*
	If Reatelk UI WPS is enabled, we shall not remove WCN IE except Win7 
	otherwise we would fail to get the IE for WPS.
*/
{
	OCTET_STRING 	osWcnIe = {0,0};// the WCN IE in the mmpdu
	u2Byte			RmvIeLen = 0;
	u4Byte i, len;

	do
	{
		//
		// Note that osWcnIe does not include EID and OUI.
		//
		osWcnIe = PacketGetElement(*posMmpdu, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUBTYPE_DONT_CARE);

		if(osWcnIe.Octet == 0)
		{
			//RT_TRACE(COMP_SCAN, DBG_LOUD, ("RemoveWcnIeFromMmpdu(): ALL WCN IE removed.\n"));
			break;
		}
		else
		{
			RmvIeLen += (osWcnIe.Length+2);
		}

		//RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, 
		//	"RemoveWcnIeFromMmpdu(): before", posMmpdu->Octet, posMmpdu->Length);

		//
		// To make osWcnIe include EID and OUI.
		//
		osWcnIe.Octet -= 2;
		osWcnIe.Length += 2;

		//
		// Move the IEs after Wcn IE foreward.
		// --------------------------------
		// | Hdr | prev IEs | Wcn IE | post IEs |
		// --------------------------------
		//
		len = (u4Byte)(posMmpdu->Length - (u2Byte)(osWcnIe.Octet - posMmpdu->Octet) - osWcnIe.Length);
		for(i = 0; i < len; i++)
		{
			PlatformMoveMemory(osWcnIe.Octet + i, 
				osWcnIe.Octet + osWcnIe.Length + i, 
				1);
		}
		posMmpdu->Length -= osWcnIe.Length;
	} while(TRUE);

	//RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, 
	//	"RemoveWcnIeFromMmpdu(): after", posMmpdu->Octet, posMmpdu->Length);
	
	return RmvIeLen;
	
}

VOID
UpdateBssWcnIe(
	IN OUT	PRT_WLAN_BSS		pBssDesc,
	IN 		POCTET_STRING		posMmpdu
)
/*
	If UI WPS enabled, we shall not remove WCN IE from mmpdu and update WCN IE except Win7.
	Otherwise we will have duplicate WCN IE indicated up in N6CTranslateRtBssToDot11Bss().
*/
{
#if (WPS_SUPPORT == 1)
	POCTET_STRING posWcnIe;
	OCTET_STRING osNewWcnIe;

	RT_ASSERT((pBssDesc->BeaconWcnIeBuf), 
		("UpdateBssWcnIe(): pBssDesc->BeaconWcnIeBuf shuld not be null\n"));
	RT_ASSERT((pBssDesc->ProbeRspWcnIeBuf), 
		("UpdateBssWcnIe(): pBssDesc->ProbeRspWcnIeBuf shuld not be null\n"));
		
	//RT_TRACE(COMP_SCAN, DBG_LOUD, 
	//	("UpdateBssWcnIe(): packet type: %X\n", PacketGetType(*posMmpdu)));

	pBssDesc->osBeaconWcnIe.Octet = pBssDesc->BeaconWcnIeBuf;
	pBssDesc->osProbeRspWcnIe.Octet = pBssDesc->ProbeRspWcnIeBuf;

	posWcnIe = (PacketGetType(*posMmpdu) == Type_Beacon ? 
		(&pBssDesc->osBeaconWcnIe) : (&pBssDesc->osProbeRspWcnIe));

	osNewWcnIe = PacketGetElement(*posMmpdu, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUBTYPE_DONT_CARE);
	if(osNewWcnIe.Length + 2 < WCN_IE_BUF_LEN) // 2 = 1 (element id )+ 1 (oui).
	{	
		//
		// Store the Wcn IE from the frame.
		//
		if(osNewWcnIe.Length > 0)
		{
			posWcnIe->Length = 0;
			PacketMakeElement(posWcnIe, EID_Vendor, osNewWcnIe);
		}
		else
		{
				//RT_TRACE(COMP_SCAN, DBG_WARNING, 
				//("No WCN IE.\n"));
		}
	}
	else
	{
		RT_TRACE(COMP_SCAN, DBG_WARNING, 
			("UpdateBssWcnIe(): length of the recvd Wcn IE (%u) greater than the maximum length (%u) we can deal with.\n",
			osNewWcnIe.Length + 2, WCN_IE_BUF_LEN));
	}
/*
	{
		static u1Byte temp[]={0x00,0x12,0x0e,0x6c,0x13,0xc9};
		if(eqMacAddr(temp, Frame_pBssid((*posMmpdu))))
		{
			RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, 
				"UpdateBssWcnIe(): mmpdu:", posMmpdu->Octet, posMmpdu->Length);
			RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, 
				"UpdateBssWcnIe(): Wcn IE:", posWcnIe->Octet, posWcnIe->Length);
		}
	}
*/
#endif
}

BOOLEAN
AssembleFragmentWcnIeFromMmpdu(
	IN	PADAPTER		Adapter,
	IN    PSIMPLE_CONFIG_T	pSimpleConfig,
	IN OUT	POCTET_STRING posMmpdu
)
{
	OCTET_STRING 	osWcnIe;		// the WCN IE in the mmpdu
	OCTET_STRING 	ret;		// the WCN IE in the mmpdu
	u1Byte			IENum;
	int i =0;
	BOOLEAN			bRet = FALSE;

	if(!pSimpleConfig->bEnabled)
		return	bRet;

	// Check the IE Fragment
	IENum = PacketGetElementNum(*posMmpdu, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUBTYPE_DONT_CARE);
	
	// Get the fragment ie
	if(IENum > 1)
	{
		u4Byte j=0,len =0;
		RT_TRACE(COMP_WPS,DBG_LOUD,("The IE is Fragment, here will assemble it \n"));

		PlatformZeroMemory(pSimpleConfig->AssembleIEBuf, MAX_SIMPLE_CONFIG_IE_ASSEMBLE_LEN);

		for(i=0;i<IENum;i++)
		{
			//
			// Note that osWcnIe does not include EID and OUI.
			//
			osWcnIe = PacketGetElement(*posMmpdu, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUBTYPE_DONT_CARE);
			ret.Octet = osWcnIe.Octet;
			ret.Length = osWcnIe.Length;

			RT_PRINT_DATA(COMP_WPS, DBG_TRACE, "The Fragment IE : \n", ret.Octet , ret.Length);

			if(i==0)
			{
				// The first IE, We copy it from DD

				// ||DD||Length ||OUI||TAG||LEN||VALUE||...
				//     1B       1B         4B      2B     2B    LEN
				PlatformMoveMemory(pSimpleConfig->AssembleIEBuf, (osWcnIe.Octet-2), (osWcnIe.Length+2));
				pSimpleConfig->AssembleIELen =  osWcnIe.Length +2;
			}
			else
			{
				// The later IE , we copy it from TLV skip OUI
				PlatformMoveMemory((pSimpleConfig->AssembleIEBuf+pSimpleConfig->AssembleIELen), (osWcnIe.Octet+4), (osWcnIe.Length-4));
				pSimpleConfig->AssembleIELen +=  (osWcnIe.Length - 4);
				
			}			

			//
			// To make osWcnIe include EID and OUI.
			//
			osWcnIe.Octet -= 2;
			osWcnIe.Length += 2;

			//
			// Move the IEs after Wcn IE foreward.
			// --------------------------------
			// | Hdr | prev IEs | Wcn IE | post IEs |
			// --------------------------------
			//
			len = (u4Byte)(posMmpdu->Length - (u2Byte)(osWcnIe.Octet - posMmpdu->Octet) - osWcnIe.Length);

			for(j = 0; j < len; j++)
			{
				PlatformMoveMemory(osWcnIe.Octet + j, 
					osWcnIe.Octet + osWcnIe.Length + j, 
					1);
			}
			posMmpdu->Length -= osWcnIe.Length;			
			
		}

		pSimpleConfig->AssembleIEBuf[1] = (pSimpleConfig->AssembleIELen -2);
		RT_PRINT_DATA(COMP_WPS, DBG_LOUD, "The Assemble IE : \n", pSimpleConfig->AssembleIEBuf, pSimpleConfig->AssembleIELen);

		// Remove WPS IE
		RemoveWcnIeFromMmpdu(posMmpdu);

		//return osWcnIe;
		PlatformMoveMemory((posMmpdu->Octet + posMmpdu->Length), 
			pSimpleConfig->AssembleIEBuf, 
			pSimpleConfig->AssembleIELen);
		posMmpdu->Length +=pSimpleConfig->AssembleIELen;

		bRet = TRUE;
	}

	return bRet;
}


u2Byte
RemoveWCNIE(
	IN PADAPTER	Adapter,
	IN PRT_WLAN_BSS	bssDesc,
	IN PSIMPLE_CONFIG_T pSimpleConfig,
	IN POCTET_STRING	mmpdu
)	
{
	u2Byte ret = 0;

	if(!pSimpleConfig->bEnabled ||
		Adapter->bInHctTest)
	{// If we remove WCN IE from mmpdu, simple config will not get the IE. Basically, simple config and WCN shall be exclusive.
		UpdateBssWcnIe(bssDesc, mmpdu);	
		ret = RemoveWcnIeFromMmpdu(mmpdu);
	}
	
	return ret;
}

//
// Description:
//	Get the current packet content of RFD and put into the BSS desc by parsing the IE and capabilities.
// Arguments:
//	Adapter - The NIC adapter context
//	pRfd - The current packet to be parsed
//	bssDesc - The container of BSS information
//	bUpdate - 
//		TRUE: The input bssDesc has been filled with the previous packet information. The bssDesc should not
//			be cleaned but just update the IE/capabilities information.
//		FALSE: The bssDesc will be cleaned before put into any information into the bssDesc.
// Note:
//	The information contianied between Beacon and ProbeRsp may be different. Normally, the beacon shall be
//	the final basis, and we shall not overwrite from the non-exist information to the exist information in the bssDesc
//	if bUpdate is set true. The 2nd packet to be update is just a supplementary information.
// Revised by Bruce, 2009-07-07.
//
BOOLEAN
GetValueFromBeaconOrProbeRsp(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	OUT	RT_WLAN_BSS		*bssDesc,
	IN	BOOLEAN			bUpdate
	)
{
	OCTET_STRING	ssidBeacon,	IbssBeacon,	BratesBeacon, TimBeacon, DsPmBeacon, HTInfoIE;
	OCTET_STRING	ExtratesBeacon, ERPInfo;
	OCTET_STRING	mmpdu;
	OCTET_STRING	RSNIE;
	OCTET_STRING	QBssLoad;
	OCTET_STRING	WMMElement;
	OCTET_STRING	RegulatoryElement;
	OCTET_STRING	PowerConstraint;
	pu1Byte			pframe;
	pu1Byte			pPeerHTInfo;
	u1Byte			LastRSSI;
	OCTET_STRING	RealtekElement;		// Added by Roger, 2006.12.07.
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);	
	BOOLEAN			bEDCAParms = FALSE;
	OCTET_STRING	Country;
	static u1Byte		sMFPBIPOui[] = {0x00, 0x0f, 0xac, 0x06};
	static u1Byte		WPATag[] = {0x00, 0x50, 0xf2, 0x01};
	static u1Byte		sRSNOui[] = {0x00, 0x0f, 0xac};
	
	s4Byte			preCumRecvSignalPower = bssDesc->CumRecvSignalPower;

	mmpdu.Octet = pRfd->Buffer.VirtualAddress;
	mmpdu.Length = pRfd->PacketLength;
	pframe = mmpdu.Octet;

	if(!bUpdate) // Clean up the bssDesc.
	{
		PlatformZeroMemory(bssDesc, sizeof(RT_WLAN_BSS));
	}

	// Signal strength.

	LastRSSI = bssDesc->RSSI;
	bssDesc->RSSI = pRfd->Status.SignalStrength; // 0-100 index.
	bssDesc->RecvSignalPower = pRfd->Status.RecvSignalPower; // dBm.

	// Signal quality.
	bssDesc->SignalQuality = pRfd->Status.SignalQuality; // 0-100 index.

	if(GET_HAL_DATA(Adapter)->CurrentBandType == BAND_ON_5G)
	{
		//OFDM Noise, only available in 5G
		bssDesc->Noise = (s1Byte)pRfd->Status.RxPwr[0] - (s1Byte)pRfd->Status.RxSNR[0];
	}
	else
	{
		//CCK noise, not accurate, just for estimation
		bssDesc->Noise = -1*(pRfd->Status.RxSNR[0] /3) - 65;
	}

	// Our TSF when receving thisp packet.
	bssDesc->RecvTsfLow = pRfd->Status.TimeStampLow;
	bssDesc->RecvTsfHigh = pRfd->Status.TimeStampHigh;

	// Check WPS IE format and defragment	
#if (WPS_SUPPORT == 1)
	{
		BOOLEAN				bIeFrag = FALSE;
		u2Byte				lengthBeforeAssemble = pRfd->PacketLength;
		PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);
	
		bIeFrag = AssembleFragmentWcnIeFromMmpdu(Adapter,pSimpleConfig,&mmpdu);

		if(TRUE == bIeFrag)
		{
			pRfd->PacketLength = mmpdu.Length;
			pRfd->ValidPacketLength -= (lengthBeforeAssemble - mmpdu.Length);
		}
	}
#endif
	//
	// 061124, rcnjko: keep frame body to  get IE in later use and 
	// NDIS5 BSS list report.
	//
	// 2010/04/16 MH For DTM WNCIE_Ex.cmn test. We can not indicate too much.
	// Incorrect indicate number will + 1;
	//
	// Proting by tynli from 92E svn revision 7281. 2010.09.07.
	//
	// 2011/05/26 probe rsp can overwrite beacon IE content but not in reverse, by hpfan
	if(pRfd->ValidPacketLength == mmpdu.Length)
	{
		if(!bUpdate || (bssDesc->BssPacketType == BSS_PKT_BEACON) || (PacketGetType(mmpdu) == Type_Probe_Rsp) )
		{
			CopyRtWlanBssIE(bssDesc, 
				mmpdu.Octet + sMacHdrLng, 
				mmpdu.Length - sMacHdrLng);
		}
		else
		{
			// Because we don't copy the content to the BSS, we don't parse the content to prevent
			// mismatch between parsing result and IE.
			// By Bruce, 2011-06-27.
			return TRUE;
		}
	}
	else if(pRfd->ValidPacketLength != 0)
	{
		CopyRtWlanBssIE(bssDesc, 
			mmpdu.Octet + sMacHdrLng, 
			pRfd->ValidPacketLength - sMacHdrLng);
	}

	// Determine the receiving packet type for BSS weighting.
	bssDesc->BssPacketType |= 
		(PacketGetType(mmpdu) == Type_Probe_Rsp) ? BSS_PKT_PROBE_RSP : BSS_PKT_BEACON;

	//bssid
	CopyMem(bssDesc->bdBssIdBuf, Frame_Addr3(mmpdu), 6);
	
	//Src address
	CopyMem(bssDesc->bdMacAddressBuf, Frame_Addr2(mmpdu), 6);

	//CumRecvSignalPower is shown on Antenna test (%) and RSSI (dBm)
	if(!bUpdate)
	{
		PRT_WLAN_RSSI	pBssRssi;

		pBssRssi = BssDescDupByBssid4Rssi(Adapter, bssDesc->bdBssIdBuf);

		if(pBssRssi)
			bssDesc->CumRecvSignalPower = pBssRssi->CumRecvSignalPower;
		else
			bssDesc->CumRecvSignalPower = pRfd->Status.RecvSignalPower; // dBm.
	}
	else if(preCumRecvSignalPower >= 0)
	{
		bssDesc->CumRecvSignalPower = pRfd->Status.RecvSignalPower; // dBm.
	}
	else
	{
		bssDesc->CumRecvSignalPower = (preCumRecvSignalPower * 15 + pRfd->Status.RecvSignalPower) >> 4;
	}

// 1.Timestamp, 2005.06.27, by rcnjko.
	bssDesc->bdTstamp = GET_BEACON_PROBE_RSP_TIME_STAMP_HIGH(pframe);
	bssDesc->bdTstamp <<= 32;
	bssDesc->bdTstamp |= GET_BEACON_PROBE_RSP_TIME_STAMP_LOW(pframe);

// 2.Beacon interval
	bssDesc->bdBcnPer = GET_BEACON_PROBE_RSP_BEACON_INTERVAL(pframe);

// 3.Capability information
	bssDesc->bdCap = GET_BEACON_PROBE_RSP_CAPABILITY_INFO(pframe);

// 4.SSID
	ssidBeacon = PacketGetElement(mmpdu, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);

	// 
	// Update SSID as the 2 conditions:
	// (1) !bUpdate - This bssDesc is a new one, copy the ssid by all means.
	// (2) !IsHiddenSsid(ssidBeacon) - This packet includes a non-Hidden SSID. The current bssDesc may conatin a hidden SSID by
	//							previous parsed packet, so update the SSID here.
	// By Bruce, 2009-07-07.
	//
	if(!bUpdate || !IsHiddenSsid(ssidBeacon))
	{
		CopySsid(bssDesc->bdSsIdBuf, bssDesc->bdSsIdLen, ssidBeacon.Octet, ssidBeacon.Length);
	}

// 5.Supported rate
	BratesBeacon = PacketGetElement(mmpdu, EID_SupRates, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if( BratesBeacon.Length != 0)
	{
		CopyMem( bssDesc->bdSupportRateEXBuf, BratesBeacon.Octet, BratesBeacon.Length);
		bssDesc->bdSupportRateEXLen= BratesBeacon.Length;
	}

// 7.DS parameter Set
	DsPmBeacon = PacketGetElement(mmpdu, EID_DSParms, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if( DsPmBeacon.Length != 0 ) //2.4G band
		bssDesc->ChannelNumber = EF1Byte( *(u1Byte *)(DsPmBeacon.Octet) );
	//Get 5G Chnl info from IE, except 5G A mode
	else if(GET_HAL_DATA(Adapter)->CurrentBandType == BAND_ON_5G)
	{
		HTInfoIE = PacketGetElement(mmpdu, EID_HTInfo, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
		if( HTInfoIE.Length !=0)
		{
			HTParsingHTInfoElement(Adapter, HTInfoIE, bssDesc);
			pPeerHTInfo = (pu1Byte)bssDesc->BssHT.bdHTInfoBuf;
			bssDesc->ChannelNumber  = GET_HT_INFO_ELE_CHANNEL(pPeerHTInfo);
		}
		else
		{
			u1Byte halCurChnl=GET_HAL_DATA(Adapter)->CurrentChannel;
			
			if(bssDesc->LastChnlUpdatecount != pMgntInfo->Scancount)
			{
				if(halCurChnl >= 36)
					bssDesc->ChannelNumber = halCurChnl;
			}
			else if(LastRSSI < bssDesc->RSSI)
			{
				//deal with AP singnal duplicate on 64-shifted channel problem: If same BSSID occur on two channels in one scan, select according to RSSI
				if(halCurChnl >= 36)
					bssDesc->ChannelNumber = halCurChnl;
			}

			if(bssDesc->ChannelNumber < 36)
				bssDesc->ChannelNumber = 36;
		}
	}
	else
	{
		//should not get here
		RT_TRACE(COMP_SCAN , DBG_LOUD , ("Can't get channel information\n"));
		bssDesc->ChannelNumber = GET_HAL_DATA(Adapter)->CurrentChannel;
	}
	bssDesc->LastChnlUpdatecount = pMgntInfo->Scancount;
	
	if(IsHiddenSsid(ssidBeacon) )
	{
		pMgntInfo->hiddenChannel = bssDesc->ChannelNumber;
	}
#if (DFS_SUPPORT == 1)	
	if(pMgntInfo->DFSMgnt.staMode.bMonitorAfterSwitchIsDone)
	{
		if(pMgntInfo->DFSMgnt.staMode.dfsOldConnectedChannel == bssDesc->ChannelNumber)
			pMgntInfo->DFSMgnt.staMode.dfsOldConnectedChannel =0;
	}
#endif
// 9.IBSS parameter set
	IbssBeacon = PacketGetElement(mmpdu, EID_IbssParms, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if( IbssBeacon.Length != 0){
		bssDesc->bdIbssParms.atimWin = EF2Byte( *(UNALIGNED pu2Byte)(IbssBeacon.Octet) );
	}

// 10.TIM
	//bdDtimPer
	TimBeacon = PacketGetElement(mmpdu, EID_Tim, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if( TimBeacon.Length != 0)
	{
		bssDesc->bdDtimPer =EF1Byte(*(TimBeacon.Octet+1));
	}

// 11. Country Information.
	FillOctetString(bssDesc->bdCountryIE, bssDesc->bdCountryIEBuf, 0);
	{
		Country = PacketGetElement(mmpdu, EID_Country, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
		if(Country.Length != 0)
			CopyMemOS(&bssDesc->bdCountryIE, Country, Country.Length );
	}

// 13. QBSS Load.
	QBssLoad = PacketGetElement(mmpdu, EID_QBSSLoad, OUI_SUB_DONT_CARE ,OUI_SUBTYPE_DONT_CARE);
	if(QBssLoad.Length == QBSS_LOAD_SIZE)
	{
		bssDesc->BssQos.bQBssLoadValid = TRUE;
		PlatformMoveMemory(bssDesc->BssQos.QBssLoad, QBssLoad.Octet, QBSS_LOAD_SIZE);
	}
	else if(!bUpdate) // The previous QBSS Load may be valid.
	{
		bssDesc->BssQos.bQBssLoadValid = FALSE;
	}


// 14.RSN
	// ShuChen TODO: Get RSN IE information
	//2004/06/29, kcwu, parse RSN IE info
	{
		u1Byte count;

		//kcwu, initialize
		bssDesc->SecLvl = RT_SEC_LVL_NONE;
		bssDesc->GroupCipherSuite = RT_ENC_ALG_NO_CIPHER;
		bssDesc->PairwiseCipherCount = 0;
		bssDesc->AuthSuiteCount = 0;
		for(count = 0; count < MAX_CIPHER_SUITE_NUM; count++){
			bssDesc->PairwiseCipherSuite[count] = RT_ENC_ALG_NO_CIPHER;
		}
		for(count = 0; count < MAX_AUTH_SUITE_NUM; count++){
			bssDesc->AuthSuite[count] = AKM_SUITE_NONE;
		}
		bssDesc->bPreAuth = FALSE;
		bssDesc->NumOfPTKReplayCounter = 0;
		bssDesc->NumOfGTKReplayCounter = 0;
	}

	bssDesc->WpaIe.Octet = bssDesc->WpaIeBuf;

	bssDesc->RsnIe.Octet = bssDesc->RsnIeBuf;
	bssDesc->WpaIe.Length = 0;
	bssDesc->RsnIe.Length = 0;
	bssDesc->bMFPC = 0;
	bssDesc->bMFPR = 0;
	bssDesc->bMFPBIP = 0;


	RSNIE = PacketGetElement(mmpdu, EID_WPA2, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if(RSNIE.Length != 0)
	{
		bssDesc->SecLvl = RT_SEC_LVL_WPA2;
		bssDesc->RsnIe.Length = RSNIE.Length+2;
		PlatformMoveMemory (bssDesc->RsnIeBuf, RSNIE.Octet-2, RSNIE.Length+2);
				
	}
	else
	{
		RSNIE = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_WPA, OUI_SUBTYPE_DONT_CARE );
		if(RSNIE.Length != 0)
		{
			bssDesc->SecLvl = RT_SEC_LVL_WPA;
			bssDesc->WpaIe.Length = RSNIE.Length+2;
			PlatformMoveMemory (bssDesc->WpaIeBuf, RSNIE.Octet-2, RSNIE.Length+2);
		}
	}

	if (bssDesc->WpaIe.Length == 0)
	{
		bssDesc->WpaIe = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_WPA, OUI_SUBTYPE_DONT_CARE );
		if(bssDesc->WpaIe.Length != 0)
		{
			bssDesc->WpaIe.Length += 2;
			PlatformMoveMemory (bssDesc->WpaIeBuf, bssDesc->WpaIe.Octet-2, bssDesc->WpaIe.Length);
			bssDesc->WpaIe.Octet = bssDesc->WpaIeBuf;
		}
	}
	
	if(RSNIE.Length != 0)
	{
		u1Byte	*pCurr;
		u1Byte	count;
		u4Byte	Readlength =0;
		
		pCurr = RSNIE.Octet;
		
		//2004/09/15, kcwu, parse WPA2 packet
		if(bssDesc->SecLvl == RT_SEC_LVL_WPA)
		{
			pCurr+=4;
			CopyMem(&bssDesc->SecVersion, pCurr, 2);
			pCurr+=2;
		}
		else if(bssDesc->SecLvl == RT_SEC_LVL_WPA2)
		{
			//pCurr+=2;
			CopyMem(&bssDesc->SecVersion, pCurr, 2);
			pCurr+=2;
			Readlength =  2;
		}
		else
		{
			//NO WAY!!!!
		}
		Sec_MapOUITypeToCipherSuite(&(pCurr[0]), &(pCurr[3]), bssDesc->SecLvl, &(bssDesc->GroupCipherSuite));

		pCurr += 3;
		Readlength +=  3;
		pCurr += 1;
		Readlength +=  1;

		bssDesc->PairwiseCipherCount = *pCurr;
		pCurr+=2;
		Readlength +=  2;

		if(bssDesc->PairwiseCipherCount >0 && bssDesc->PairwiseCipherCount < MAX_CIPHER_SUITE_NUM)
		{
			for(count = 0; count < bssDesc->PairwiseCipherCount; count++)
			{
				Sec_MapOUITypeToCipherSuite(&(pCurr[0]), &(pCurr[3]), bssDesc->SecLvl, &(bssDesc->PairwiseCipherSuite[count]));
				pCurr+=4;
				Readlength +=  4;
			}
		}
		bssDesc->AuthSuiteCount = *pCurr;
		pCurr+=2;
		Readlength +=  2;


		if(bssDesc->AuthSuiteCount > 0 && bssDesc->AuthSuiteCount < MAX_AUTH_SUITE_NUM)
		{
			for(count = 0; count < bssDesc->AuthSuiteCount; count++)
			{
				Sec_MapOUITypeToAKMSuite(&pCurr[0], &pCurr[3], bssDesc->SecLvl, &(bssDesc->AuthSuite[count]));
				pCurr+=4;
				Readlength +=  4;
			}
		}
		bssDesc->bPreAuth = (BOOLEAN)GET_RSN_CAP_PREAUTH(pCurr);
		bssDesc->NumOfPTKReplayCounter = (u1Byte)GET_RSN_CAP_PTKSA_REPLAY_COUNTER(pCurr);
		bssDesc->NumOfGTKReplayCounter = (u1Byte)GET_RSN_CAP_GTKSA_REPLAY_COUNTER(pCurr);


		//
		// Get MFP Capabile and Required , Just support "WPA2"
		//
		if( bssDesc->SecLvl == RT_SEC_LVL_WPA2 && Readlength < RSNIE.Length ) // NOTE may need to check Len !!
		{
			bssDesc->bMFPC = (u1Byte)GET_RSN_CAP_MFP_CAPABLE(pCurr);
			bssDesc->bMFPR= (u1Byte)GET_RSN_CAP_MFP_REQUIRED(pCurr);
			pCurr += 2;
			Readlength += 2;
			
			// Check BIP support !!
			// SKIP PMKIDs
			pCurr += 2;
			Readlength += 2;
			if( Readlength <  RSNIE.Length )
			{
				if( !PlatformCompareMemory( pCurr, sRSNOui , 3) && 
					(DOT11_AuthKeyType_1X_SHA256 == pCurr[3] || DOT11_AuthKeyType_PSK_SHA256 == pCurr[3]))
					bssDesc->bMFPBIP = TRUE;
				else
					bssDesc->bMFPBIP = FALSE;
			}
		}
	}

	bssDesc->PairwiseChiper = NONE_WPA;
	bssDesc->GroupChiper = 0;
	CLEAR_FLAGS(bssDesc->AKMsuit);
 
	// 1. Get WPA2 chiper 
 	RSNIE = PacketGetElement(mmpdu, EID_WPA2, OUI_SUB_DONT_CARE , OUI_SUBTYPE_DONT_CARE);
 	if( RSNIE.Length != 0 )
	 {
		  u1Byte  *pCurr;
		  u4Byte  count,i;
		  u4Byte  Readlength =0;
		 
		  //RT_PRINT_DATA( COMP_TEST, DBG_LOUD, " WPA2 IE :  ", RSNIE.Octet , RSNIE.Length );
		  pCurr = RSNIE.Octet;
		 
		  // 1. Skip Version 
		  pCurr += 2;
		  Readlength +=2;
		 
		  // 2. Get Group Chiper
		  bssDesc->GroupChiper = pCurr[3] ;
		  pCurr += 4;
		  Readlength +=4;
		 
		  // 3. Get Pairwise chiper count 
		  count = pCurr[0]; //+ (pCurr[1]>>8);
		  pCurr += 2;
		  Readlength +=2;
		 
		  // 4. Get Pairwise chiper type
		  for( i = 0 ; i < count ; i++ )
		  {
			   if( pCurr[3] == 0x02 )
			    	bssDesc->PairwiseChiper += WPA2_TKIP;
			   else if( pCurr[3] == 0x04 )
			    	bssDesc->PairwiseChiper += WPA2_AES;
			 
			   pCurr += 4;
			   Readlength +=4;
		  }
		  
		  // 5. Get AKM Suit count
		  count = pCurr[0];// + (pCurr[1]>>8);
		  pCurr += 2;
		  Readlength +=2;
		 
		  // 6. Get AKM suit 
		  for( i = 0 ; i < count ; i++ )
		  {
		   
			   if( !PlatformCompareMemory(sRSNOui , pCurr , 3))
			   {
				    if( pCurr[3] == 0x01 )
						SET_FLAG(bssDesc->AKMsuit, AKM_WPA2_1X);
				    else if( pCurr[3] == 0x02 )
						SET_FLAG(bssDesc->AKMsuit, AKM_WPA2_PSK);
					else if( pCurr[3] == 0x03 )
						SET_FLAG(bssDesc->AKMsuit, AKM_FT_1X);
					else if( pCurr[3] == 0x04 )
						SET_FLAG(bssDesc->AKMsuit, AKM_FT_PSK);
					else if( pCurr[3] == 0x05 )
						SET_FLAG(bssDesc->AKMsuit, AKM_RSNA_1X_SHA256);
				    else if( pCurr[3] == 0x06 )
						SET_FLAG(bssDesc->AKMsuit, AKM_RSNA_PSK_SHA256);					
			   }
		   else
		   {
		    	RT_TRACE(COMP_TEST , DBG_LOUD , ("===> unknow AKM !!\n"));
		   }
		   pCurr += 4;
		   Readlength +=4;
		  }
		 
		  // 7. Get RSN Cap 
		  bssDesc->bMFPC = GET_RSN_CAP_MFP_CAPABLE(pCurr);
		  bssDesc->bMFPR = GET_RSN_CAP_MFP_REQUIRED(pCurr);
		  pCurr += 2;
		  Readlength +=2;
	 
		  // 8. Get BIP 
		  if( (Readlength + 5 ) < RSNIE.Length )
		  {
		   	// Skip PMKID
		   	pCurr += 2;
		   	Readlength +=2;
		   	if(!PlatformCompareMemory( pCurr, sMFPBIPOui , 4))
		   	{
		    		bssDesc->bMFPBIP = TRUE;
		   	}
	  	}
	 }
	// 2. Get WPA Chiper
	 RSNIE = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_WPA , OUI_SUBTYPE_DONT_CARE);
	 if( RSNIE.Length != 0 )
	{
		  u1Byte  *pCurr;
		  u4Byte  count,i;
		  u4Byte  Readlength =0;
		 
		  //RT_PRINT_DATA( COMP_TEST, DBG_LOUD, " WPA IE :  ", RSNIE.Octet , RSNIE.Length );
		  pCurr = RSNIE.Octet;
		 
		  // 1. SKIP WPA OUI , Version
		  pCurr += 6;
		  Readlength += 6;
		 
		  // 2. Get Group chiper
		  bssDesc->GroupChiper = pCurr[3] ;
		  pCurr += 4;
		  Readlength +=4;
		 
		  // 3. Get Pairwise chiper count  
		  count = pCurr[0]; //+ (pCurr[1]>>8);
		  pCurr += 2;
		  Readlength +=2;
		 
		  // 4. Get Pairwise chiper type
		  for( i = 0 ; i < count ; i++ )
		  {
			   if( pCurr[3] == 0x02 )
			    	bssDesc->PairwiseChiper += WPA_TKIP;
			   else if( pCurr[3] == 0x04 )
			    	bssDesc->PairwiseChiper += WPA_AES;
			   
			   pCurr += 4;
			   Readlength +=4;
		  }
		 
		  // 5. Get AKM Suit count
		  count = pCurr[0] ;//+ (pCurr[1]>>8);
		  pCurr += 2;
		  Readlength +=2;
		 
		  // 6. Get AKM suit 
		  for( i = 0 ; i < count ; i++ )
		  {
			   if( !PlatformCompareMemory(WPATag , pCurr , 3))
			   {
					if( pCurr[3] == 0x01 )
						SET_FLAG(bssDesc->AKMsuit, AKM_WPA_1X);
				   	 else if( pCurr[3] == 0x02 )
					 	SET_FLAG(bssDesc->AKMsuit, AKM_WPA_PSK);
			   }
			   else
			   {
			    		RT_TRACE(COMP_TEST , DBG_LOUD , ("===> unknow AKM !!\n"));
			   }
			   pCurr += 4;
			   Readlength +=4;
		  }
	}


// 14. QBSS Load element. (see 802.11e/D13.0, 7.2.3.1, p37.) (AnnieTODO)

// 15. WMM Parameter Element. Added by Annie, 2005-11-08.
	// Initialize BSS_QOS
	QosInitializeBssDesc( &bssDesc->BssQos );
	
	// Get WMM PARAM element: DD-18-00-50-F2-02.
	WMMElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_WMM, OUI_SUBTYPE_WMM_PARAM);

	// Get WMM INFO element: DD-07-00-50-F2-02.
	FillOctetString(bssDesc->osWmmAcParaIE, bssDesc->WmmAcParaBuf, 0);
	if(WMMElement.Length == 0)
		WMMElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_WMM, OUI_SUBTYPE_WMM_INFO);
	else if(WMMElement.Length == WMM_PARAM_ELEMENT_SIZE)
		CopyMemOS(&bssDesc->osWmmAcParaIE, WMMElement, WMMElement.Length );
		
	if(WMMElement.Length == 0)
	{
		bEDCAParms = TRUE;
		WMMElement = PacketGetElement( mmpdu, EID_EDCAParms, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	}

	if( WMMElement.Length != 0 )
	{ // Parsing WMM  Element
		QosParsingQoSElement( Adapter, bEDCAParms, WMMElement, bssDesc );
	}else
	{
		bssDesc->BssQos.bdQoSMode &= ~QOS_WMM;
	}

	FillOctetString(bssDesc->osPowerConstraintIe, bssDesc->PowerConstraintBuf, 0);
	PowerConstraint = PacketGetElement(mmpdu, EID_POWER_CONSTRAINT, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(PowerConstraint.Length == MAX_DOT11_POWER_CONSTRAINT_IE_LEN)
		CopyMemOS(&bssDesc->osPowerConstraintIe, PowerConstraint, MAX_DOT11_POWER_CONSTRAINT_IE_LEN);

#if DBG_CMD
// 16. HT related element.
	Peer_bssDesc.BssQos.bdQoSMode = bssDesc->BssQos.bdQoSMode;
	// (1) Parse HTCap, and HTInfo, and record whether it is 11n AP
	// (2) If peer is HT, but not WMM, call QosSetLegacyWMMParamWithHT()
	// (3) Check whether peer is Realtek AP(for Realtek proprietary aggregation mode)
#endif

	HTGetValueFromBeaconOrProbeRsp(Adapter, &mmpdu, bssDesc);
	VHTGetValueFromBeaconOrProbeRsp(Adapter, &mmpdu,bssDesc);

// 19.Extended supported rates
	ExtratesBeacon = PacketGetElement(mmpdu, EID_ExtSupRates, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if( ExtratesBeacon.Length != 0)
	{
		CopyMem( bssDesc->bdSupportRateEXBuf+bssDesc->bdSupportRateEXLen, ExtratesBeacon.Octet, ExtratesBeacon.Length);
		bssDesc->bdSupportRateEXLen += ExtratesBeacon.Length;
	}

	// Keep AP support rate from beacon or prob resp. 2010.10.29. by tynli.
	if(pMgntInfo->mAssoc &&
		!MgntScanInProgress(pMgntInfo)	&&
		!MgntIsLinkInProgress(pMgntInfo) &&
		!pMgntInfo->SnifferTurnOnFlag)
	{
		CopyMem(pMgntInfo->SupportRatesfromBCN.Octet, bssDesc->bdSupportRateEXBuf, bssDesc->bdSupportRateEXLen);
		pMgntInfo->SupportRatesfromBCN.Length = bssDesc->bdSupportRateEXLen;
	}
	
// 20.ERP Information
	ERPInfo = PacketGetElement(mmpdu, EID_ERPInfo, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if( ERPInfo.Length != 0 )
	{
		bssDesc->bdERPInfo = ERPInfo.Octet[0];
		bssDesc->bERPInfoValid = TRUE;
	}
	else
	{
		bssDesc->bdERPInfo = 0;
		bssDesc->bERPInfoValid = FALSE;
	}

//------------------------------------------------------------------------
//
// 21: Realtek Inforfation Element of 8186: 00-E0-4C.
//
// 051230, Annie: Asked by SD4 David, we have to recognize 8186 IE in AutoTurbo and EnableTurbo Mode.
// 061026, Rcnjko: 8187 has compatiblility issue with 8186, so we have to recognize 8186 and adjust HW parameter.
// Added by Roger, 2006.12.07.
//
	bssDesc->bRealtekCapType1Exist  = FALSE;	// Initialize.
	RealtekElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_REALTEK_TURBO, OUI_SUBTYPE_DONT_CARE);
	GetRealtekIEContentForTurboMode( Adapter, bssDesc, &RealtekElement );
	
//------------------------------------------------------------------------	

// 22.Supported Regulatory Classes Information
	RegulatoryElement = PacketGetElement(mmpdu, EID_SupRegulatory, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if( RegulatoryElement.Length != 0)
		bssDesc->RegulatoryClass = EF1Byte(*(u1Byte *)RegulatoryElement.Octet);
	else
		bssDesc->RegulatoryClass = 0;

	DFS_StaGetValueFromBeacon(Adapter, mmpdu, bssDesc);	


#if (WPS_SUPPORT == 1)
{
	OCTET_STRING	SimpleConfigInfo;
	// 22: Simple Config Element  added by CCW copy from 818x
	//if(pSimpleConfig->bEnabled)
	{
		SimpleConfigInfo = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUBTYPE_DONT_CARE);

		bssDesc->bdSimpleConfIE.Octet = bssDesc->bdSimpleConfIEBuf;
		bssDesc->bdSimpleConfIE.Length = (SimpleConfigInfo.Length > MAX_SIMPLE_CONFIG_IE_LEN_V2) ? MAX_SIMPLE_CONFIG_IE_LEN_V2 : SimpleConfigInfo.Length; 
		CopyMem( bssDesc->bdSimpleConfIE.Octet, SimpleConfigInfo.Octet, bssDesc->bdSimpleConfIE.Length);
	}
}
#endif	// #if (WPS_SUPPORT == 1)

	if(!GET_SIMPLE_CONFIG_ENABLED(pMgntInfo) || Adapter->bInHctTest)
		UpdateBssWcnIe(bssDesc, &mmpdu);

	if(bssDesc->ChannelNumber <= 14)
		bssDesc->wirelessmode = IsPeer11G(bssDesc) ? WIRELESS_MODE_G : WIRELESS_MODE_B;
	else
		bssDesc->wirelessmode = WIRELESS_MODE_A;

#if DBG_CMD
	Peer_bssDesc.BssHT.bdSupportHT = bssDesc->BssHT.bdSupportHT;
#endif

	if(bssDesc->BssHT.bdSupportHT)
	{
		RT_DISP(FBEACON, BCN_PEER, ("GetValueFromBeaconOrProbeRsp HT N mode\r\n"));
		if(bssDesc->wirelessmode == WIRELESS_MODE_A)
			bssDesc->wirelessmode = WIRELESS_MODE_N_5G;
		else if(bssDesc->wirelessmode & (WIRELESS_MODE_G|WIRELESS_MODE_B))
			bssDesc->wirelessmode = WIRELESS_MODE_N_24G;

		if(bssDesc->BssVHT.bdSupportVHT)
		{
			if(bssDesc->wirelessmode == WIRELESS_MODE_N_5G)
				bssDesc->wirelessmode = WIRELESS_MODE_AC_5G;
			else
				bssDesc->wirelessmode = WIRELESS_MODE_AC_24G;
		}
	}
	else
		RT_DISP(FBEACON, BCN_PEER, ("GetValueFromBeaconOrProbeRsp Not HT N mode\r\n"));

	TDLS_GetValueFromBeaconOrProbeRsp(Adapter, &mmpdu, bssDesc);
	
	//
	// IOT Action
	// 
	bssDesc->Vender = HT_IOT_PEER_UNKNOWN;
	bssDesc->bRealtekAggCapExist = FALSE;
	RecognizePeer( Adapter,&mmpdu, bssDesc);
	
	return TRUE;
}


VOID
RecognizePeer(
	PADAPTER 			Adapter, 
	POCTET_STRING 		pInmmpdu,
	PRT_WLAN_BSS 		bssDesc
	)
{
	OCTET_STRING IElement;
	OCTET_STRING mmpdu;
	u4Byte	Length = 0;
		
	FillOctetString(mmpdu, pInmmpdu->Octet, pInmmpdu->Length)

	// 2011/11/03 MH Add peer recognize for WNDAP3200 bad TP in HT40 auto channel mode. It will send
	// illegal response packet in extension channel. All RTK IC will meet the same problem.
	if (PlatformCompareMemory(bssDesc->bdBssIdBuf, NETGEAR_WNDAP3200_1, 3)==0)
	{
		PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
		bssDesc->Vender = HT_IOT_PEER_ATHEROS;
		bssDesc->SubTypeOfVender = HT_IOT_PEER_ATHEROS_NETGEAR_WNDAP3200;
		// ugly revise due to BW set is before HT IOT recognize in seup join info. We are afraid
		// There are other AP which we have to disable channel combine ability.
		pMgntInfo->IOTAction |=HT_IOT_ACT_DISABLE_EXT_CHNL_COMBINE;
		return;
	}

	if (PlatformCompareMemory(bssDesc->bdBssIdBuf, NETGEAR_WNDAP3500_1, 3)==0)
	{
		bssDesc->Vender = HT_IOT_PEER_MARVELL;
		bssDesc->SubTypeOfVender = HT_IOT_PEER_ATHEROS_NETGEAR_WNDAP3500;
		return;
	}

	// 2011/11/28 MH Need to disable TXOP to prevent serious collision with the AP.
	if (PlatformCompareMemory(bssDesc->bdBssIdBuf, NETGEAR_WNDAP4500_1, 3)==0)
	{
		PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
		bssDesc->Vender = HT_IOT_PEER_BROADCOM;
		bssDesc->SubTypeOfVender = HT_IOT_PEER_BROADCOM_NETGEAR_WNDAP4500;	
		pMgntInfo->IOTAction |= HT_IOT_ACT_DISABLE_AC_TXOP;
		//return;
	}
	
	if (PlatformCompareMemory(bssDesc->bdBssIdBuf, CMW500, 6)==0)
	{
		bssDesc->Vender = HT_IOT_PEER_CMW500;
		return;
	}
	
	//3 Atheros OUI
	IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_ATHEROS_IE_1, OUI_SUBTYPE_DONT_CARE);
	if(IElement.Length == 0)
		IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_ATHEROS_IE_2, OUI_SUBTYPE_DONT_CARE);
	if(IElement.Length != 0) 
	{
		bssDesc->Vender = HT_IOT_PEER_ATHEROS;
		if(PlatformCompareMemory(bssDesc->bdBssIdBuf, DLINK_ATHEROS_DIR655_4, 3)==0)
			bssDesc->SubTypeOfVender = HT_IOT_PEER_ATHEROS_DIR655;	
		else if(PlatformCompareMemory(bssDesc->bdBssIdBuf, TPLINK_AC1750, 3)==0)
			bssDesc->SubTypeOfVender = HT_IOT_PEER_TPLINK_AC1750;
		
		return;
	}

	// mark by hpfan 2009.08.04 For some version of DIR655 can not be recognized from mac address
	// Some of the Dlink AP such as DIR655 or DIR855 does not contain OUI
	if((PlatformCompareMemory(bssDesc->bdBssIdBuf, DLINK_ATHEROS_DIR635_1, 3)==0) ||
		(PlatformCompareMemory(bssDesc->bdBssIdBuf, DLINK_ATHEROS_DIR635_2, 3)==0))
	{
		bssDesc->Vender = HT_IOT_PEER_ATHEROS;
		bssDesc->SubTypeOfVender = HT_IOT_PEER_ATHEROS_DIR635;
		return;
	}


	//3 Ralink OUI
	IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_RALINK_IE, OUI_SUBTYPE_DONT_CARE);
	if(IElement.Length != 0)
	{
		bssDesc->Vender = HT_IOT_PEER_RALINK;
		if(!PlatformCompareMemory(bssDesc->bdBssIdBuf, DLINK_RALINK_DIR300, 3))
		{
			bssDesc->SubTypeOfVender = HT_IOT_PEER_RALINK_DIR300;
		}
		return;
	}

	//3 Cisco OUI
	IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_CISCO_IE, OUI_SUBTYPE_DONT_CARE);
	if(IElement.Length != 0)
	{
		bssDesc->Vender = HT_IOT_PEER_CISCO;
		return;
	}

	//3 Meru OUI
	IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_MERU_IE, OUI_SUBTYPE_DONT_CARE);
	if(IElement.Length != 0)
	{
		bssDesc->Vender = HT_IOT_PEER_MERU;	
		return;
	}

	//2 BroadCom OUI
	// 2008/01/25 MH Get Broadcom AP IE for manamgent frame CCK rate problem.
	// AP can not receive CCK managemtn from from 92E.
	IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_BROADCOM_IE_1, OUI_SUBTYPE_DONT_CARE);
	Length += IElement.Length;
	IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_BROADCOM_IE_2, OUI_SUBTYPE_DONT_CARE);
	Length += IElement.Length;
	IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_BROADCOM_IE_3, OUI_SUBTYPE_DONT_CARE);
	Length += IElement.Length;
	if(Length > 0)
	{
		bssDesc->Vender = HT_IOT_PEER_BROADCOM;
		Length = 0;	
		IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_BROADCOM_LINKSYSE4200_IE_1, OUI_SUBTYPE_DONT_CARE);
		Length += IElement.Length;
		IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_BROADCOM_LINKSYSE4200_IE_1, OUI_SUBTYPE_DONT_CARE);
		Length += IElement.Length;
		IElement = PacketGetElement( mmpdu, EID_Vendor, OUI_SUB_BROADCOM_LINKSYSE4200_IE_1, OUI_SUBTYPE_DONT_CARE);
		Length += IElement.Length;
		if(Length > 0)
			bssDesc->SubTypeOfVender = HT_IOT_PEER_LINKSYS_E4200_V1;
		
		return;
	}

	//3 Marvell OUI
	IElement = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_MARVELL, OUI_SUBTYPE_DONT_CARE);
	if(IElement.Length != 0)
	{
		bssDesc->Vender = HT_IOT_PEER_MARVELL;
		return;
	}

	//3 Airgo OUI
	IElement = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_AIRGO, OUI_SUBTYPE_DONT_CARE);
	if(IElement.Length != 0)
	{
		bssDesc->Vender = HT_IOT_PEER_AIRGO;
		return;
	}

	//3 Realtek OUI
	IElement = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_REALTEK_AGG, OUI_SUBTYPE_DONT_CARE);
	if(IElement.Length != 0)
	{
		bssDesc->Vender = HT_IOT_PEER_REALTEK;
		bssDesc->bRealtekAggCapExist = TRUE;

		if(IElement.Length >= 5)
		{
			if(bssDesc->BssHT.bdSupportHT)
			{
				bssDesc->BssHT.RT2RT_HT_Mode |= RT_HT_CAP_USE_TURBO_AGGR;
		
				if(IElement.Octet[4]==1)
				{
					if(IElement.Octet[5] & RT_HT_CAP_USE_LONG_PREAMBLE)
						bssDesc->BssHT.RT2RT_HT_Mode |= RT_HT_CAP_USE_LONG_PREAMBLE;

					if(IElement.Octet[5] & RT_HT_CAP_USE_92SE)
					{
						bssDesc->BssHT.RT2RT_HT_Mode |= RT_HT_CAP_USE_92SE;
						bssDesc->Vender = HT_IOT_PEER_REALTEK_92SE;
					}
				}
			}
			
			if(IElement.Octet[5] & RT_HT_CAP_USE_SOFTAP)
				bssDesc->Vender = HT_IOT_PEER_REALTEK_SOFTAP;

			if(IElement.Octet[4] == 2)
			{
				if(IElement.Octet[6] & RT_HT_CAP_USE_JAGUAR_BCUT)
					bssDesc->Vender = HT_IOT_PEER_REALTEK_JAGUAR_BCUTAP;
				else if(IElement.Octet[6] & RT_HT_CAP_USE_JAGUAR_CCUT)
					bssDesc->Vender = HT_IOT_PEER_REALTEK_JAGUAR_CCUTAP;
			}
			
		}
		return;
	}

}





BOOLEAN
bActiveScan(	
	PMGNT_INFO		pMgntInfo,
	PRT_CHNL_LIST_ENTRY pChnlListEntry
)
{
	BOOLEAN ret = FALSE;	
	PRT_SSIDS_TO_SCAN	pSsidsToScan = &(pMgntInfo->SsidsToScan);
	u2Byte			i;
	
	if(pChnlListEntry->ScanType == SCAN_MIX)
	{
		if(IS_DOT11D_ENABLE(pMgntInfo) && (!IS_COUNTRY_IE_VALID(pMgntInfo)))
			ret = FALSE;
		else if(pMgntInfo->bScanOnly == FALSE || pSsidsToScan->NumSsid > 0)	
		{
			u1Byte	j;
			PRT_CHANNEL_LIST	pChannelList = GET_RT_CHANNEL_LIST(pMgntInfo);
			for(j = 0; j < pChannelList->ChannelLen; j++)
			{
				if(pChannelList->ChnlListEntry[j].ChannelNum == pChnlListEntry->ChannelNum)
				{
					break;
				}
			}	
			if(j == pChannelList->ChannelLen)
				ret = FALSE;
			else if( pChannelList->EachChannelSTAs[j] > 0)
				ret = TRUE;
			else 
				ret = FALSE;
		}
		else 
			ret = FALSE;
	}
	else if(pChnlListEntry->ScanType == SCAN_PASSIVE)
	{
		if(DFS_5G_RADAR_CHANNEL(pChnlListEntry->ChannelNum))
		{
			for(i=0;i<pMgntInfo->NumBssDesc4Query;i++)
			{
#if (DFS_SUPPORT == 1)
				if(pMgntInfo->DFSMgnt.staMode.dfsOldConnectedChannel == pChnlListEntry->ChannelNum)
					ret = FALSE;
#endif				
				if(	(pMgntInfo->bssDesc4Query[i].HistoryCount == History_Count_Limit) &&
					(pMgntInfo->bssDesc4Query[i].ChannelNumber == pChnlListEntry->ChannelNum))
					ret = TRUE;
			}

		}
		else
		{
			ret = FALSE;
		}
	}
	else if(pChnlListEntry->ScanType == SCAN_ACTIVE)
	{
		ret = TRUE;
	}	

	//2 Carrier Sense Test
	// <20140414, Kordan> Under a carrier sense test (Adaptvity=1 && CarrierSense=1), we do not 
	// send the probe request on all sub-channels of current center frequency. (Asked by Yu Chen)
	if (pMgntInfo->RegEnableAdaptivity && pMgntInfo->RegEnableCarrierSense && pMgntInfo->bMediaConnect == FALSE) 
	{
		int channel = pChnlListEntry->ChannelNum;
		DbgPrint("[kordan] (pChnlListEntry->ChannelNum, LastConnectedBandwidth, LastConnectedCenterFrequency) = (%d, %d, %d)\n",
			channel, pMgntInfo->LinkDetectInfo.LastConnectedBandwidth, pMgntInfo->LinkDetectInfo.LastConnectedCenterFrequency);
		switch (pMgntInfo->LinkDetectInfo.LastConnectedBandwidth) 
		{
			case CHANNEL_WIDTH_40:
					if (channel-4 <= pMgntInfo->LinkDetectInfo.LastConnectedCenterFrequency && 
						pMgntInfo->LinkDetectInfo.LastConnectedCenterFrequency <= channel+4)
						ret = FALSE;
				break;
			case CHANNEL_WIDTH_80:
					if (channel-8 <= pMgntInfo->LinkDetectInfo.LastConnectedCenterFrequency && 
						pMgntInfo->LinkDetectInfo.LastConnectedCenterFrequency <= channel+8)
						ret = FALSE;
					break;
			default:
				break;
		}

	}

	return ret;
}		


BOOLEAN
SelectChnlListEntry(
	IN	PADAPTER				Adapter,
	IN	u1Byte					ChannelNumber,
	IN	u1Byte					DualBandArrayLen,
	IN	PRT_CHNL_LIST_ENTRY		ChnlListDualBandArray,
	OUT	PRT_CHNL_LIST_ENTRY		*pChnlListEntry
	)
{
	u1Byte			i;
	BOOLEAN			ret = FALSE;

	if(IS_DUAL_BAND_SUPPORT(Adapter))
	{	
		for(i = 0; i < DualBandArrayLen; i++)
		{
			if(ChnlListDualBandArray[i].ChannelNum != ChannelNumber)
				continue;
			else 
				break;
		}

		if(i == DualBandArrayLen)
			ret = FALSE;
		else
		{
			ret = TRUE;
			*pChnlListEntry = ChnlListDualBandArray+i;
		}	
	}
	else
	{
		RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL, &ChannelNumber, pChnlListEntry);			
		if(*pChnlListEntry  == NULL)
			ret = FALSE;
		else
			ret = TRUE;
	}

	return ret;
}

BOOLEAN
CheckSupportRate(
	IN		PADAPTER		pAdapter,
	IN		RT_WLAN_BSS		*bssdesc
	)
{
	
	//3 // 802.11g only and 802.11a/g can not connect to 802.11b only AP
	if(IS_WIRELESS_MODE_WITHOUT_B(pAdapter->RegOrigWirelessMode) && (bssdesc->wirelessmode & WIRELESS_MODE_B))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("CheckSupportRate(): 802.11g only and 802.11a/g cannot connect to 802.11b only AP\n"));
		return TRUE;
	}

	//3 // 802.11b only can not connect to 802.11g only and 802.11a/g AP
	if(IS_WIRELESS_MODE_B_ONLY(pAdapter->RegWirelessMode) && !Is11bBasicRate(bssdesc))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("CheckSupportRate(): 802.11b only cannot connect to 802.11g only and 802.11a/g AP\n"));
		return TRUE;
	}

	return FALSE;
}

VOID
GetScanInfo(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	PMGNT_INFO      		pMgntInfo = &Adapter->MgntInfo;
	RT_WLAN_BSS		*bssdesc;
	OCTET_STRING		mmpdu;
	PRT_CHNL_LIST_ENTRY	pChnlListEntry = NULL;

	mmpdu.Octet = pRfd->Buffer.VirtualAddress;
	mmpdu.Length = pRfd->PacketLength;

	RT_TRACE(COMP_SCAN,DBG_TRACE, ("====>GetScanInfo(): portnumber %d NumBssDesc %d\n", Adapter->pNdis62Common->PortNumber, pMgntInfo->NumBssDesc));

	if( (pMgntInfo->state_Synchronization_Sta < STATE_Act_Receive) ||
		(pMgntInfo->state_Synchronization_Sta > STATE_Act_Listen)
	)
	{
		RT_TRACE(COMP_SCAN, DBG_TRACE,("GetScanInfo(): ERROR pMgntInfo->state_Synchronization_Sta = %d\n", pMgntInfo->state_Synchronization_Sta));
		return;
	}

	//if(Adapter->bInHctTest)
	//
	//  FilterHiddenAP is set by MgntLinkRequest , 
	//  Now MgntLinkRetry , FilterHiddenAP will same as bHiddenSSIDEnable
	//  normal case :
	//	In XP we always "NO" filter hidden  
	//	In Vista/Win7 , we will filter hidden when OS set bHiddenSSIDEnable.
	if(pMgntInfo->FilterHiddenAP && pMgntInfo->bHiddenSSIDEnable)
	{
		if( NullSSID(mmpdu) ){
				RT_TRACE(COMP_SCAN, DBG_LOUD,("HCT:GetScanInfo(): discard for null ssid\n"));
				return;
		}
	}	

	// Customer special requirement, it should supportby demand.	
	if (Adapter->RegWirelessMode == WIRELESS_MODE_AC_ONLY)
	{		
		if (VHTModeSupport(Adapter, &mmpdu) == FALSE)
		{
			return;
		}
	}
	
	bssdesc = BssDescDupSource( Adapter, pRfd );
	if( bssdesc == NULL )
	{
		// Add a new bssdesc entry
		if( pMgntInfo->NumBssDesc < MAX_BSS_DESC)
		{
			bssdesc = &pMgntInfo->bssDesc[pMgntInfo->NumBssDesc];
		}
		else
		{
			RT_TRACE(COMP_SCAN, DBG_WARNING, ("GetScanInfo(): ERROR! No bssdesc entry. NumBssDesc %d MAX_BSS_DESC %d\n", pMgntInfo->NumBssDesc, MAX_BSS_DESC));
			return;
		}

		// ShuChen TODO: Check Broadcom AP's 1M-Short Preamble issue.
		GetValueFromBeaconOrProbeRsp( Adapter, pRfd, bssdesc, FALSE);

		RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL, &(bssdesc->ChannelNumber), &pChnlListEntry);

		if(!pChnlListEntry)
			return;

		if(CheckSupportRate(Adapter, bssdesc))
			return;

		//
		// In theory, we cannot receive the probe responses at this channel with passive scan, so filter this probe response to
		// prevent from joining the Hidden AP at passive scaning channel. By Bruce, 2008-09-08.
		//
		if(	(PacketGetType(mmpdu) == Type_Probe_Rsp) &&  (bActiveScan(pMgntInfo,pChnlListEntry) == FALSE))
		{
			RT_TRACE(COMP_SCAN, DBG_TRACE, ("GetScanInfo(): Filter probe response at passive scaning channel(%d).\n", 
				bssdesc->ChannelNumber));
			return;
		}

		

		RT_TRACE(COMP_SCAN, DBG_TRACE, ("GetScanInfo(): create new BssDesc at ch %d, bss index:%d\n", bssdesc->ChannelNumber, pMgntInfo->NumBssDesc));
		RT_TRACE(COMP_MLME, DBG_LOUD, ("GetScanInfo(): New SSID: %s \n",bssdesc->bdSsIdBuf));
		RT_PRINT_ADDR(COMP_SCAN, DBG_TRACE, "GetScanInfo(): new Bss BSSID:", bssdesc->bdBssIdBuf);
		RT_PRINT_STR(COMP_SCAN, DBG_LOUD, "GetScanInfo(): new Bss SSID:", bssdesc->bdSsIdBuf, bssdesc->bdSsIdLen);
		RT_TRACE(COMP_SCAN, DBG_TRACE, ("GetScanInfo():  new BssDesc is %d HT mode\n", bssdesc->BssHT.bdSupportHT));
		pMgntInfo->NumBssDesc += 1;

		if( 	pMgntInfo->bNeedSkipScan 
			&& CompareSSID(pMgntInfo->Ssid.Octet,pMgntInfo->Ssid.Length,bssdesc->bdSsIdBuf,bssdesc->bdSsIdLen) )
		{
			//pMgntInfo->bNeedSkipScan = FALSE;
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("GetScanInfo():bCompleteScan %d \n", pMgntInfo->bCompleteScan));
			pMgntInfo->bCompleteScan = TRUE;
		}

	}
	else
	{
		u1Byte			RSSI = bssdesc->RSSI; // 0-100 index..
		s4Byte			RecvSignalPower =  bssdesc->RecvSignalPower ; // dBm.
		u1Byte			SignalQuality =  bssdesc->SignalQuality; // 0-100 index.

		GetValueFromBeaconOrProbeRsp(Adapter, pRfd, bssdesc, TRUE);

		// Update the exist BSS.
		if(pRfd->Status.SignalStrength < RSSI) 
		{	//3 Update signal information to a better packet
			HAL_DATA_TYPE			*pHalData	= GET_HAL_DATA(Adapter);
			if(pHalData->CurrentChannel != bssdesc->ChannelNumber)
			{
				bssdesc->RSSI = RSSI;
				bssdesc->RecvSignalPower = RecvSignalPower;			
				bssdesc->SignalQuality = SignalQuality; 
				}
			}	

		if(!GET_SIMPLE_CONFIG_ENABLED(pMgntInfo) ||
				Adapter->bInHctTest)
		{
			UpdateBssWcnIe(bssdesc, &mmpdu);
		}
		RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL, &(bssdesc->ChannelNumber), &pChnlListEntry);
			
		if(!pChnlListEntry)
			return;
			
		//
		// In theory, we cannot receive the probe responses at this channel with passive scan, so filter this probe response to
		// prevent from joining the Hidden AP at passive scaning channel. By Bruce, 2008-09-08.
		//
		if((PacketGetType(mmpdu) == Type_Probe_Rsp) && (pChnlListEntry->ScanType == SCAN_PASSIVE))
		{
			RT_TRACE(COMP_SCAN, DBG_TRACE, ("GetScanInfo(): Filter probe response at passive scaning channel(%d).\n", 
				bssdesc->ChannelNumber));
			return;
		}
	}

	if(bssdesc != NULL)
	{
		DFS_StaCheckRadarChnl(Adapter, bssdesc);

		// Check SSIDL for CCX S53. Slpit the beacon and insert into the list.
		// Move it to scan complete
		// CCXGetSSIDLToBssList(Adapter, bssdesc);		

		//
		// Moved from GetValueBeaconOrProbeRsp to here. We should check the channel is valid first and then update the country IE.
		// By Bruce, 2008-11-12.
		// 
		if(IS_DOT11D_ENABLE(pMgntInfo))
		{
			if(bssdesc->bdCountryIE.Length != 0)
			{
				RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, "GetScanInfo(): Country IE:", 
					bssdesc->bdCountryIE.Octet, bssdesc->bdCountryIE.Length);
				if(!IS_COUNTRY_IE_VALID(pMgntInfo))
				{
					Dot11d_UpdateCountryIe(Adapter, bssdesc->bdMacAddressBuf, &bssdesc->bdCountryIE);
				}
				else if( IS_EQUAL_CIE_SRC(pMgntInfo, bssdesc->bdMacAddressBuf) )
				{
					RT_PRINT_DATA(COMP_SCAN, DBG_LOUD, "GetScanInfo(): Dot11dCountry IE:", 
						GET_DOT11D_INFO(pMgntInfo)->CountryIeBuf, GET_DOT11D_INFO(pMgntInfo)->CountryIeLen);
					// Check if this country IE is changed.
					if(!IS_COUNTRY_IE_CHANGED(pMgntInfo, bssdesc->bdCountryIE))
					{
						UPDATE_CIE_WATCHDOG(pMgntInfo);
					}
				}
			}
		}

		
	}
}

BOOLEAN
InPrefferedBssidList(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		MacAddr
	)
{
	// check if debug mode, used for that we want to connect the AP with desired bssid
	if(bDebugFixBssid)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("InPrefferedBssidList(): Fixed bssid!!!\n"));
		if(eqMacAddr(debug_fixed_bssid, MacAddr))
		{
			return TRUE;
		}
		else
			return FALSE;
	}
	
	return TRUE;
}

//
// Description:
//	Select an BSS according to the input SSID to be joined.
//	We use the list "bssDesc4Query" in pMgntInfo instead of "bssDesc", because the scan list
//	is consistent with the returned list for query to the OS.
// Argument:
//	Adapter -
//		NIC adapter context pointer.
//	ssid2match -
//		The SSID to be met.
//	bRoaming -
//		Roaming operation indicates to select a different BSSID with the current one. 
//	pRtBss -
//		BSS descriptor object returned if it meets the selection conditions. I rewrote this variable
//		definition by "copying all info into this desc" instead of pointer, because the Bss we selected
//		in the scan list is probably chnaged from everywhere, and we need this desc's info for a long 
//		time. Therefore, it's safer copy this Bss to the local buffer.
// Return:
//	RT_JOIN_ACTION.
// Note:
//	In actual, there may be several BSSs that meet the required SSID,
//	however, we choose the best one based on the various conditions.
//
// Revised by Bruce, 2009-02-09.
//
RT_JOIN_ACTION
SelectNetworkBySSID(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	*ssid2match,
	IN	BOOLEAN			bRoaming,
	OUT	PRT_WLAN_BSS	pRtBss
	)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	RT_JOIN_ACTION			join_action = RT_NO_ACTION;
	RT_JOIN_NETWORKTYPE		CurrBSSType;	
	u4Byte					i, MaxWeight = 0, CurrWeight = 0;
	BOOLEAN					bSameSSIDIbss = FALSE;
	PRT_CHNL_LIST_ENTRY		pChnlListEntry = NULL;
	PRT_WLAN_BSS			pRtMatchedBss = NULL, pRtTmpBss = NULL;

	u1Byte					DualBandArrayLen = 0;
	RT_CHNL_LIST_ENTRY		pChnlListDualBandArray[MAX_CHANNEL_NUM] = {0};

	
	if(pRtBss == NULL)
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("SelectNetworkBySSID(): pRtBss = NULL, return RT_NO_ACTION!!\n"));
		return RT_NO_ACTION;
	}

	CurrBSSType = MgntActQuery_802_11_INFRASTRUCTURE_MODE(Adapter);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> SelectNetworkBySSID(): %s \n", ssid2match->Octet));
	RT_PRINT_STR(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID \n"), ssid2match->Octet, ssid2match->Length);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): BssDescNum = %d CurrBSSType %d\n", pMgntInfo->NumBssDesc4Query, CurrBSSType));


	if(IS_DUAL_BAND_SUPPORT(Adapter))
		DualBandArrayLen = RtGetDualBandChannel(Adapter, pChnlListDualBandArray);

	for(i = 0; i < pMgntInfo->NumBssDesc4Query; i ++)
	{
		// Get channel info and determine if this channel is valid to join.
		pChnlListEntry = NULL;

		RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): SSID:%s, at channel %d\n",pMgntInfo->bssDesc4Query[i].bdSsIdBuf,pMgntInfo->bssDesc4Query[i].ChannelNumber));
		RT_PRINT_STR(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID SSID \n"), pMgntInfo->bssDesc4Query[i].bdSsIdBuf, pMgntInfo->bssDesc4Query[i].bdSsIdLen);
		RT_PRINT_ADDR(COMP_SCAN, DBG_LOUD, "SelectNetworkBySSID(): Bss BSSID:", pMgntInfo->bssDesc4Query[i].bdBssIdBuf);

		if(FALSE == SelectChnlListEntry(Adapter, pMgntInfo->bssDesc4Query[i].ChannelNumber, DualBandArrayLen, pChnlListDualBandArray, &pChnlListEntry))
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): SelectChnlListEntry return FALSE\n"));
			continue;
		}


		pRtTmpBss = &(pMgntInfo->bssDesc4Query[i]);

		CurrWeight = 0;

		//3  // Check SSID
		if( !IsSSIDAny(*ssid2match) )
		{
			if(CHECK_HIDDEN_SSID(Adapter))
			{
				// 2013/10/15 MH When create adhoc, it will be aborted in open sapce if there is a hidden ap. Why the AC series
				// execute different behavior with other IC?? in check in V4444. After discussing with Maddest, use XP compile
				// flag to disable temporarily. Need to revise with better way. Only we will not consider XP case in the future~!?
				if(!CompareSSID(pRtTmpBss->bdSsIdBuf, pRtTmpBss->bdSsIdLen, ssid2match->Octet, ssid2match->Length))
				 {
				 	continue;
				 }
				 
			}
			else if(!CompareSSID(pRtTmpBss->bdSsIdBuf, pRtTmpBss->bdSsIdLen, ssid2match->Octet, ssid2match->Length))
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): SSID mismatch\n"));
				continue;
			}

			CurrWeight += WEIGHTING_SAME_SSID;
		}
		else
		{
			// 2007-11-30 Isaiah : Because SecurityInfo.PairwiseEncAlgorithm & SecLvl initial value is WPA2-PSK AES, 
			// We have a chance to connect OPEN-WEP AP and MacOS UI shows "connect" and "security field" = WAP2-PSK AES. 
			// Connection State would last few minutes, it looks strange said by QC.CCW solved it.
			
			if(pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeShared ||
			   pMgntInfo->SecurityInfo.PairwiseEncAlgorithm > RT_ENC_ALG_NO_CIPHER)
			{
				pRtMatchedBss = NULL;
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): Check security\n"));
				break;
			}
		}

		//3 // User specify network infrastructure to match
		if(((pRtTmpBss->bdCap & cESS) && (CurrBSSType == RT_JOIN_NETWORKTYPE_ADHOC)) ||
			((pRtTmpBss->bdCap & cIBSS) && (CurrBSSType == RT_JOIN_NETWORKTYPE_INFRA)))
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): Matched bdCap and CurrBSSType\n"));
			continue;
		}

		//3 // Do not join the IBSS in this channel.
		// 2011/04/28 MH For 92D BSOD case prevention, pChnlListEntry might be NULL for VS. Need better revise later.
		if(pChnlListEntry) 
		{
			if((pRtTmpBss->bdCap & cIBSS) && (pChnlListEntry->ExInfo & CHANNEL_EXINFO_NO_IBSS_JOIN))
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): CHANNEL_EXINFO_NO_IBSS_JOIN\n"));
				continue;
			}
		}

		// For the Connect BSS Selection Override, the best way is fill the target BSS using parameters from UE.
#if CONNECT_BSS_SELECTION_OVERRIDE
		if(!InPrefferedBssidList(Adapter, pRtTmpBss->bdBssIdBuf) && !bRoaming)
#else
		if(!InPrefferedBssidList(Adapter, pRtTmpBss->bdBssIdBuf))
#endif
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): BSSID is not in the preffered bssid list\n"));
			continue;
		}
		
		//3 // Check if this BSSID is in the desired list
		// <Note> It is always true except under Ndis6. By Bruce, 2008-05-27.
		//		In roaming case, we don't need to care about the desired BSSID list.
#if CONNECT_BSS_SELECTION_OVERRIDE
		if (!IsInDesiredBSSIDList(Adapter, pRtTmpBss->bdBssIdBuf) && !bRoaming)
#else
		if (!IsInDesiredBSSIDList(Adapter, pRtTmpBss->bdBssIdBuf))
#endif
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): BSSID is not in the desired list\n"));
			continue;
		}

		//3 // Check the roaming condition.
#if CONNECT_BSS_SELECTION_OVERRIDE
		if(bRoaming)
		{
			// We need select a different BSSID.
			if(eqMacAddr(pMgntInfo->Bssid, pRtTmpBss->bdBssIdBuf))
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): Different BSSID\n"));
				continue;
			}
			// Check QBSS Load
			if(!CheckQBssLoad(Adapter, pRtTmpBss))
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): Check QBSS Load\n"));
				continue;
			}
		}
#endif


#if (WPS_SUPPORT == 1)
		{
			PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);

			if(pSimpleConfig->bEnabled && (!BeHiddenSsid(pRtTmpBss->bdSsIdBuf,pRtTmpBss->bdSsIdLen)) && (CurrWeight >= WEIGHTING_SAME_SSID))
			{
				if(eqMacAddr(pMgntInfo->Bssid, pRtTmpBss->bdBssIdBuf))
				{
					RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): WPS BSSID Weight More\n"), pMgntInfo->Bssid);
					CurrWeight += WEIGHTING_SAME_BSSID;
				}
			}
		}
#endif

		//3 // Config for WPS
		//For WPS connect to a AP, we lie to ndis and send out Auth request
		//if WPS be enable modify by Mars on 12th April 2007
		//we modify here for wep in XP and modify it to fix XP and Vista
#if 0//(WPS_SUPPORT == 1)
{
		PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);
		
		RT_TRACE(COMP_WPS, DBG_LOUD,( "Simple Config WPS IS bEnabled %d \n",pSimpleConfig->bEnabled));
		if(pSimpleConfig->bEnabled &&
			CompareSSID(pRtTmpBss->bdSsIdBuf, pRtTmpBss->bdSsIdLen, ssid2match->Octet, ssid2match->Length))
		{
			RT_TRACE(COMP_WPS, DBG_LOUD, ("SelectNetworkBySSID(): In WPS Lie to Autoconfig\n"));
			if(pRtTmpBss->bdCap & cPrivacy)
			{
				RT_TRACE(COMP_WPS, DBG_LOUD, ("==>WPS changr security mode to open for connect\n"));
#ifdef NDIS60_MINIPORT	
				pRtTmpBss->bdCap = pRtTmpBss->bdCap & 0xffef;
#else
				if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_NO_CIPHER
				   || pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 ||
				   pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104)
				{
					pRtTmpBss->bdCap = pRtTmpBss->bdCap & 0xffef;
				}
#endif
			}
		}
		
		
}
#endif

		//3 // Check BSS security
		if(!CheckBSSSetting(Adapter, pRtTmpBss))
		{
			RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "SelectNetworkBySSID(): BSS has incompatiable parameters, bssid -> ", pRtTmpBss->bdBssIdBuf);
			if(Adapter->bInHctTest)
				pMgntInfo->state_Synchronization_Sta = STATE_Pas_Listen;		

			if(CurrBSSType == RT_JOIN_NETWORKTYPE_ADHOC)
				bSameSSIDIbss = TRUE;
			
			continue;
		}

		//3  //Check if This BSS is a cached bss.
		if((0 < pRtTmpBss->HistoryCount)  && (pRtTmpBss->HistoryCount < History_Count_Limit))
		{
			if(pMgntInfo->AntennaTest != 1)
			{
				RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "SelectNetworkBySSID(): cached BSDD, bssid Skip...-> ", pRtTmpBss->bdBssIdBuf);
				continue;
			}
			else
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): Antenna testing...\n"));
			}
		}
	
		//3 // Security Deny List
		// Deny to link to the AP if TKIP MIC error occured twice in 60 sec.
		// 2004.10.06, by rcnjko.
		if(SecIsInDenyBssidList( &(pMgntInfo->SecurityInfo), pRtTmpBss->bdBssIdBuf))
		{
			RT_PRINT_ADDR(COMP_MLME | COMP_SEC, DBG_LOUD, "SelectNetworkBySSID(): Security Deny bssid Skip...-> ", pRtTmpBss->bdBssIdBuf);
			continue;
		}

		// Excluded List
		if(MgntIsInExcludedMACList(Adapter, pRtTmpBss->bdBssIdBuf))
		{
			RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "SelectNetworkBySSID(): Skip this BSSID because of excluded list! ", pRtTmpBss->bdBssIdBuf);
			continue;
		}

		// When antenna test, connect to AP which BSS power is the best.
		if((pMgntInfo->AntennaTest == 1) && (pRtMatchedBss != NULL))
		{
			if(pRtTmpBss->RecvSignalPower > pRtMatchedBss->RecvSignalPower)
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): BssPower %d > %d\n", pRtTmpBss->RecvSignalPower, pRtMatchedBss->RecvSignalPower));
				CurrWeight += WEIGHTING_BEST_RSSI;
			}
		}

		//3 // Weight user selected prefer band.
		if(((pMgntInfo->RegPreferBand == 1) && IS_24G_WIRELESS_MODE(pRtTmpBss->wirelessmode)) ||
			((pMgntInfo->RegPreferBand == 2) && IS_5G_WIRELESS_MODE(pRtTmpBss->wirelessmode)))
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): Weight the prefer band\n"));
			CurrWeight += WEIGHTING_PREFER_BAND;
		}
		
		//3 // Caculate the weight of BSS to determine which BSS is the Best.
		CurrWeight += pRtTmpBss->RSSI;

		//3 // Weight the probe response packet.
		CurrWeight += (pRtTmpBss->BssPacketType & BSS_PKT_PROBE_RSP) ? WEIGHTING_PROBE_RSP : 0;

		//3 // Weight channel load. 
		// 1. For CCX test plan v4.59 6.2.1.1 and 6.2.1.2, 070625, by rcnjko.
		// 2. Assume the channel utilization of BSS without channel load is 50%.
		if(pRtTmpBss->BssQos.bQBssLoadValid)
		{
			u1Byte	ChnlWeight = 0xff - GET_QBSS_LOAD_CHNL_UTILIZATION(pRtTmpBss->BssQos.QBssLoad);
			ChnlWeight /= 8; // translate [0,255] to [0,31].
			CurrWeight += ChnlWeight;
		}
		else
		{
			RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "SelectNetworkBySSID(): No QBSS Load", pRtTmpBss->bdBssIdBuf);
			CurrWeight += WEIGHTING_DEFAULT_QBSS_LOAD;
		}

		//3 // Connect back to the origianl associated AP after S3/S4. 
		// Add bRoamingbySleep for DTM 1.0c test ,070823 
		// In this case, we should connect back to the original AP by weighting the case of the same BSSID.
		// By Bruce, 2008-05-23.
		if(	pMgntInfo->RoamingType == RT_ROAMING_BY_SLEEP ||
			pMgntInfo->RoamingType == RT_ROAMING_BY_DEAUTH)
		{
			if(eqMacAddr(pMgntInfo->Bssid, pRtTmpBss->bdBssIdBuf))
			{
				RT_TRACE(COMP_MLME, DBG_TRACE, ("SelectNetworkBySSID(): RT_ROAMING_BY_SLEEP/RT_ROAMING_BY_DEAUTH Weight More\n"));
				CurrWeight += WEIGHTING_SAME_BSSID;
			}
		}

		//3 // Reject AP List
		if(MgntIsInRejectedAPList(Adapter, pRtTmpBss->bdBssIdBuf))
		{
			if (CurrWeight > WEIGHTING_SAME_SSID + WEIGHTING_PROBE_RSP)
				CurrWeight -= (WEIGHTING_SAME_SSID + WEIGHTING_PROBE_RSP);
			else
				CurrWeight = 0;

			RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "SelectNetworkBySSID(): In Reject List --> \n", pRtTmpBss->bdBssIdBuf);
		}

		//2 // ***Get The Best BSS by Comparing All Candidates***
		if( pRtMatchedBss == NULL )
		{
			MaxWeight = CurrWeight;
			pRtMatchedBss = pRtTmpBss;
		}
		else if(CurrWeight > MaxWeight)
		{
			MaxWeight = CurrWeight;
			pRtMatchedBss = pRtTmpBss;
			RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "SelectNetworkBySSID(): The current Max Weight of BSS: \n", pRtTmpBss->bdBssIdBuf);
			RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): BssPower = %d\n", pRtTmpBss->RecvSignalPower));
		}
	}//end for( i=0; i<Adapter->NumBssDesc; i++)

	
	if(pRtMatchedBss)
	{ // An BSS has been selected.
		if(pRtMatchedBss->bdCap & cESS)
		{
			if(CurrBSSType != RT_JOIN_NETWORKTYPE_ADHOC)
				join_action = RT_JOIN_INFRA;
		}
		else if(pRtMatchedBss->bdCap & cIBSS )
		{
			if(CurrBSSType != RT_JOIN_NETWORKTYPE_INFRA )
				join_action = RT_JOIN_IBSS;
		}

		// Copy to the input buffer.
		PlatformZeroMemory(pRtBss, sizeof(RT_WLAN_BSS));
		CopyWlanBss(pRtBss, pRtMatchedBss);
		RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "SelectNetworkBySSID(): macth bssid \n", pRtBss->bdBssIdBuf);
		pMgntInfo->bIbssStarter = FALSE;
	}
	else
	{	//match not found
		RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): no BSS matched, Regdot11networktype = %d\n", pMgntInfo->Regdot11networktype));
		
		join_action = RT_NO_ACTION;

		do
		{
			if( (CurrBSSType == RT_JOIN_NETWORKTYPE_INFRA))
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): Do not start IBSS if RT_JOIN_NETWORKTYPE_INFRA!<=== \n"));
				break;
			}
			// Do not start IBSS if it is joined only.
			else if(GetIbssbJoinOnly(Adapter))
			{
				join_action = RT_NO_ACTION;
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): Do not start IBSS for Join Only!<=== \n"));
				break;
			}
			else if(bSameSSIDIbss)
			{
				join_action = RT_NO_ACTION;
				RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): Do not start IBSS for security mismatch<=== \n"));
				break;
			}

			//Will start a adhoc at channel:pMgntInfo->Regdot11ChannelNumber
#if 0			
			if(IS_DUAL_BAND_SUPPORT(Adapter))
			{
				WIRELESS_MODE wirelessmode;
				
				if(IS_WIRELESS_MODE_5G(Adapter))
				{
					if(pMgntInfo->Regdot11ChannelNumber <= 14)
					{
						wirelessmode = WIRELESS_MODE_N_24G;
						Adapter->HalFunc.SetWirelessModeHandler(Adapter, (u1Byte)(wirelessmode));
					}
				}
				else if(IS_WIRELESS_MODE_24G(Adapter))
				{
					if(pMgntInfo->Regdot11ChannelNumber > 14)
					{
						wirelessmode = WIRELESS_MODE_N_5G;
						Adapter->HalFunc.SetWirelessModeHandler(Adapter, (u1Byte)(wirelessmode));
					}
				}
			}
#else
			RT_TRACE(COMP_MLME, DBG_LOUD, ("ChangeWirelessModeHandler\n"));
			HalChangeWirelessMode(Adapter, pMgntInfo->Regdot11ChannelNumber);
#endif
			
			RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL, &(pMgntInfo->Regdot11ChannelNumber), &pChnlListEntry);

			if(!pChnlListEntry) // Invalid channel
				return RT_NO_ACTION;

			if(pChnlListEntry->ExInfo & CHANNEL_EXINFO_NO_IBSS_START) // Cannot start IBSS at this channel.
				return RT_NO_ACTION;

			//
			// Switch to channel specified before.
			// NOTE: For an IBSS Creator, Regdot11ChannelNumber should be assigned before.
			// Otherwise(e.g. wzcsvc), we should set it to a valid channel on current band.
			// 2004.10.07, by rcnjko.
			//
			MgntActSet_802_11_REG_20MHZ_CHANNEL_AND_SWITCH(Adapter, pMgntInfo->Regdot11ChannelNumber);
			// Start an IBSS of given SSID.
			join_action = RT_START_IBSS;
		}while(FALSE);
	}

	RT_TRACE(COMP_MLME, DBG_LOUD, ("SelectNetworkBySSID(): join_action = %d<=== \n", join_action));
	return join_action;
}



//
//	Description:
//		Assign default preferance to each channel. 
//	2005.12.27, by rcnjko
//
VOID
InitChannelWeight(
	IN	PCHANNEL_INFO	pChnl,
	IN	u1Byte			ChannelNumber
)
{
	int	nWeight;

	RT_ASSERT(pChnl != NULL, ("InitChannelWeight(): pChnl should not be NULL !!!\n"));

	pChnl->ChannelNumber = ChannelNumber;

	switch(ChannelNumber)
	{
	case 1:
	case 6:
	case 11:
	case 14:
		nWeight = 5;
		break;

	case 2:
	case 5:
	case 7:
	case 10:
		nWeight = -5;
		break;
	
	default:
		nWeight = 0;
		break;
	}

	pChnl->Weight = nWeight;
}



//
//	Description:
//		Find a best channel from specific ChnlWeightList.
//	Annie, 2006-07-26.
//
u1Byte
FindBestChannel(
	PCHANNEL_INFO			ChnlWeightList,
	int						nNumChnlFound,
	CHANNEL_START_MODE	InitMode,
	CHANNEL_SELECT_MODE	Behavior
	)
{
	int				nChnlIdx, nCurIdx, InitIndex;
	PCHANNEL_INFO	pChnl = NULL;
	int				nBestChnlWeight;
	u1Byte			BestChnl;

	RT_ASSERT( ChnlWeightList!=NULL, ("FindBestChannel(): INPUT error - ChnlWeightList is NULL!\n") );
	RT_TRACE( COMP_SCAN, DBG_LOUD, ("FindBestChannel(): nNumChnlFound=%d, InitMode=%d, Behavior=%d\n", nNumChnlFound, InitMode, Behavior) );

	if( Behavior == CH_SEL_RANDOM )
	{
		Behavior = (CHANNEL_SELECT_MODE)GetRandomNumber(0,2);
		RT_TRACE( COMP_SCAN, DBG_LOUD, ("Behavior=2: CH_SEL_RANDOM => %d\n", Behavior ) );
	}

	switch( InitMode )
	{
		case CH_START_MIN:
			InitIndex = 0;
			break;

		case CH_START_MAX:
			InitIndex = nNumChnlFound-1;
			break;
			
		case CH_START_MIDDLE:
			InitIndex = nNumChnlFound/2;
			break;

		default:
			InitIndex = 0;
			break;
	}
	nBestChnlWeight = ChnlWeightList[InitIndex].Weight;
	BestChnl = ChnlWeightList[InitIndex].ChannelNumber;

	for( nChnlIdx=0; nChnlIdx<nNumChnlFound; nChnlIdx++ )
	{
		if( Behavior == CH_SEL_DECREASE )
		{ // Decrease
			nCurIdx = (InitIndex-nChnlIdx)%nNumChnlFound;
			if( nCurIdx<0 )  nCurIdx += nNumChnlFound;
		}
		else
		{ // Increase or other mode
			nCurIdx = (InitIndex+nChnlIdx)%nNumChnlFound;
		}

		RT_TRACE( COMP_SCAN, DBG_LOUD, ("FindBestChannel(): ChnlWeightList[%d]: Ch=%d, Weight=%d\n", nCurIdx, ChnlWeightList[nCurIdx].ChannelNumber, ChnlWeightList[nCurIdx].Weight) );
		pChnl = &(ChnlWeightList[nCurIdx]); 
		if(pChnl->Weight > nBestChnlWeight)
		{
			nBestChnlWeight = pChnl->Weight;
			BestChnl = pChnl->ChannelNumber;
		}
	}

	RT_TRACE( COMP_SCAN, DBG_LOUD, ("FindBestChannel(): nBestChnlWeight=%d, BestChnl=%d\n", nBestChnlWeight, BestChnl) );
	return	BestChnl;
}



//
//	Description:
//		Select a channel from latest scan result.
//	2005.12.27, by rcnjko
//
u1Byte
AutoSelectChannel(
	PADAPTER		Adapter, 
	PRT_WLAN_BSS	pBssList,
	int				nNumBss
)
{
	#define			MAX_NUM_CHANNEL_TO_SELECT		256

	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	u2Byte			listLen = 0;
	PCHANNEL_INFO	ChnlWeightList;
	int				nNumChnlFound = 0;
	int				nBssIdx;
	int				nChnlIdx, nChWindex;
	PCHANNEL_INFO	pChnl = NULL;
	PRT_WLAN_BSS	pBss = NULL;
	u1Byte			BestChnl;
	PRT_CHANNEL_LIST	pChannelList = NULL;
	PRT_GEN_TEMP_BUFFER pGenBufChnlWeightList;

	RT_ASSERT(pBssList != NULL, ("AutoSelectChannel(): pBssList should not be NULL !!!\n"));

	// Initialize ChnlWeightList and nNumChnlFound.

	{
		pChannelList = MgntActQuery_ChannelList(Adapter);

		if(!pChannelList) // Get channel list failed
			return 1;

		pGenBufChnlWeightList = GetGenTempBuffer (Adapter, sizeof(CHANNEL_INFO)*MAX_NUM_CHANNEL_TO_SELECT);
		ChnlWeightList = (CHANNEL_INFO *)pGenBufChnlWeightList->Buffer.Ptr;
		
		listLen = pChannelList->ChannelLen;

		RT_TRACE(COMP_MLME,DBG_LOUD,("AutoSelectChannel: listLen %d \n",listLen));

		if(listLen > MAX_NUM_CHANNEL_TO_SELECT) 
			listLen = MAX_NUM_CHANNEL_TO_SELECT;	
		for(nChnlIdx = 0; nChnlIdx < listLen; nChnlIdx++)
		{
			InitChannelWeight( &(ChnlWeightList[nChnlIdx]),  pChannelList->ChnlListEntry[nChnlIdx].ChannelNum);
		}

	}

	nNumChnlFound = listLen;

	// Enumerate each BSS scanned and figure out weight of each channel.
	for(nBssIdx = 0; nBssIdx < nNumBss; nBssIdx++)
	{
		pChnl = NULL;
		pBss = &(pBssList[nBssIdx]);

		// Find channel of the BSS.
		for(nChnlIdx = 0; nChnlIdx < nNumChnlFound; nChnlIdx++)
		{
			pChnl = &(ChnlWeightList[nChnlIdx]); 
			if(pChnl->ChannelNumber == pBss->ChannelNumber)
			{
				break;
			}
		}


		// Add a new channel found into ChnlWeightList if we cannot find in ChnlWeightList.
		if(nChnlIdx == nNumChnlFound)
		{
			if(nNumChnlFound < MAX_NUM_CHANNEL_TO_SELECT)
			{
				pChnl = &(ChnlWeightList[nNumChnlFound]);
				pChnl->ChannelNumber = (u1Byte)pBss->ChannelNumber;
				nNumChnlFound++;
			}
			else
			{
				// ChnlWeightList is full, so, we just skip BSS.
				continue;
			}
		}

		// Update its weight.
		RT_ASSERT(pChnl != NULL, ("AutoSelectChannel(): pChnl should not be NULL !!!\n"));
		RT_ASSERT(pChnl->ChannelNumber != 0, ("AutoSelectChannel(): pChnl->ChannelNumber should not be 0 !!!\n"));
		if(pChnl != NULL)
			pChnl->Weight--;
	}

	for( nChWindex = 0 ;  nChWindex < nNumChnlFound ; nChWindex++ )
	{
		//
		// We should accord to the scan type of the channel which is determined by the channel plan
		// to assign the weight to prevent an invalid channel being selected. e.g. Select ch12 or 13 in SoftAP mode.
		// 2011.11.28. by tynli.
		//
		if(pChannelList->ChnlListEntry[nChWindex].ScanType != SCAN_ACTIVE)
		{
			ChnlWeightList[nChWindex].Weight = (-1)*MAX_BSS_DESC;
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Channel %d cannot process active scan.\n", nChWindex));
		}
	}

	// Find best channel; 
	if(nNumChnlFound > 0)
	{
		RT_TRACE( COMP_SCAN, DBG_LOUD, ("AutoSelectChannel(): ChnlWeightMode=%d\n", pMgntInfo->ChnlWeightMode ) );
		switch( pMgntInfo->ChnlWeightMode )
		{
			case 0:
				// 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1
				BestChnl = FindBestChannel( ChnlWeightList, nNumChnlFound, CH_START_MAX, CH_SEL_DECREASE );
				break;
			case 1:
				// 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
				BestChnl = FindBestChannel( ChnlWeightList, nNumChnlFound, CH_START_MIN, CH_SEL_INCREASE );
				break;
			case 2:
				// 6, 7, 8, 9, 10, 11, 1, 2, 3, 4, 5
				BestChnl = FindBestChannel( ChnlWeightList, nNumChnlFound, CH_START_MIDDLE, CH_SEL_INCREASE );
				break;
			case 3:
				// 6, 5, 4, 3, 2, 1, 11, 10, 9, 8, 7
				BestChnl = FindBestChannel( ChnlWeightList, nNumChnlFound, CH_START_MIDDLE, CH_SEL_DECREASE );
				break;
			case 4:
				// Random Select: Increase or decrease. Start from Ch11.
				BestChnl = FindBestChannel( ChnlWeightList, nNumChnlFound, CH_START_MAX, CH_SEL_RANDOM );
				break;
			default:
				// Random Select: Increase or decrease. Start from Ch1.
				BestChnl = FindBestChannel( ChnlWeightList, nNumChnlFound, CH_START_MIN, CH_SEL_RANDOM );
				break;
		}
	}
	else
	{
		BestChnl = pMgntInfo->Regdot11ChannelNumber;
	}

	ReturnGenTempBuffer (Adapter, pGenBufChnlWeightList);

	return BestChnl;
}



VOID
FilterSupportRate(
	OCTET_STRING	RegRate,
	POCTET_STRING	pSupRate,
	BOOLEAN			bFilterOutCCKRate
	)
{
	u2Byte i,j,k;
	u1Byte rate;

	for(i=0,j=0;i<pSupRate->Length;i++)
	{
		rate=pSupRate->Octet[i]&0x7f;

		// HT capability info indicate that whether the peer support CCK rate in 20/40 Mode. 2007.01.15 by Emily
		if(bFilterOutCCKRate && (rate == MGN_1M || rate == MGN_2M || rate == MGN_5_5M || rate==MGN_11M))
			continue;

		for(k=0;k<RegRate.Length;k++)
		{
			if(rate==(RegRate.Octet[k]&0x7f))
			{
				pSupRate->Octet[j++]=pSupRate->Octet[i];
				break;
			}
		}
	}
	pSupRate->Length=(u2Byte)j;
}

/**
* Function:	SelectRateSet
* 
* Overview:	Decide <1>dot11OperationalRateSet, <2>mBrates, <3>HighestBasicRate, 
*				   <4>LowestBasicRate, <5>HighestOperaRate.<6>CurrentBasicRate, 
*				   <7>CurrentOperaRate <8> SupportedRates
* 
* Input:		
* 	PADAPTER		Adapter
*	OCTET_STRING	SuppRateSet	// Rate sets copied from descriptor(Infrustructure 
*				      STA more, IBSS join) or refereced from self setting(Infrastructure AP 
*					mode or IBSS Creator)
* 			
* Output:		
*	PRT_TCB		pTcb
*				(Frame in the Buffer of pTcb will be modified)
* 		
* Return:     	
*		None
*/


VOID
SelectRateSet(
	PADAPTER		Adapter,
	OCTET_STRING	SuppRateSet
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	u2Byte		i, idx;
	u2Byte		bi = 0;
	u1Byte		hBsRate, lBsRate, hOpRate, curRate;
	BOOLEAN		bFilterOutCCKRate = FALSE;

	RT_TRACE(COMP_MLME, DBG_TRACE, ("====>  SelectRateSet()\n"));
	
	// <1> dot11OperationalRateSet
	CopyMemOS( &pMgntInfo->dot11OperationalRateSet, SuppRateSet, SuppRateSet.Length );

	// HT capability info indicate that whether the peer support CCK rate in 20/40 Mode. 2007.01.15 by Emily
	bFilterOutCCKRate = (IS_WIRELESS_MODE_HT_24G(Adapter) && pMgntInfo->pHTInfo->bCurSuppCCK == FALSE); 

	FilterSupportRate(	pMgntInfo->Regdot11OperationalRateSet, 
						&pMgntInfo->dot11OperationalRateSet, bFilterOutCCKRate);

	idx = (pMgntInfo->dot11OperationalRateSet.Length<64) ? pMgntInfo->dot11OperationalRateSet.Length : 64;
	
	// <2> mBrates, <3> HighestBasicRate, <4> LowestBasicRate, <5> HighestOperaRate
	hBsRate = hOpRate = 0x00;		// Assign an impossible value first. Annie, 2005-03-31.
	lBsRate = 0x7f;	

	for(i=0; i<idx; i++)
	{
		// Compare current rate and HighestOperaRate. Annie, 2005-03-31.
		curRate = pMgntInfo->dot11OperationalRateSet.Octet[i] & 0x7f;
		if( hOpRate < curRate )
			hOpRate = curRate;
		
		if(pMgntInfo->dot11OperationalRateSet.Octet[i] & 0x80)
		{
			pMgntInfo->mBrates.Octet[bi] = pMgntInfo->dot11OperationalRateSet.Octet[i];
			bi++;

			// To decide the highest and lowest basic rates. Annie, 2005-03-31
			if( hBsRate < curRate )		hBsRate = curRate;
			if( lBsRate > curRate )		lBsRate = curRate;
		}
		else
		{
			// 
			// We shall unite {dot11OperationalRateSet} with {6M,12M,24M} to extend basic rate. 
			// See 802.11g/D8.2 section 9.6 (p17) and section 19.1.1 (p20). 2005.12.22, by rcnjko.
			//
			if(	(pMgntInfo->dot11OperationalRateSet.Octet[i] == MGN_6M) || // 6M 
				(pMgntInfo->dot11OperationalRateSet.Octet[i] == MGN_12M) || // 12M
				(pMgntInfo->dot11OperationalRateSet.Octet[i] == MGN_24M) ) // 24M
			{
				pMgntInfo->mBrates.Octet[bi] = pMgntInfo->dot11OperationalRateSet.Octet[i];
				bi++;

				// To decide the highest and lowest basic rates. Annie, 2005-03-31
				if( hBsRate < curRate )		hBsRate = curRate;
				if( lBsRate > curRate )		lBsRate = curRate;
			}
		}
	}

	pMgntInfo->mBrates.Length = bi;
	if( pMgntInfo->mBrates.Length == 0 )
	{
		// If support rates in association response does not include basic rate, apply 0x02 as basic rate.
		pMgntInfo->mBrates.Length = 1;
		
		// Apply the basic data rate by wireless mode. Annie, 2005-04-01
		if(	IS_WIRELESS_MODE_5G(Adapter) ||
			(IS_WIRELESS_MODE_HT_24G(Adapter) && !pMgntInfo->pHTInfo->bCurSuppCCK))
		{
			pMgntInfo->mBrates.Octet[0] = 0x8c;		// 6M
			hBsRate = MGN_6M;
			lBsRate = MGN_6M;			
		}
		else
		{
			pMgntInfo->mBrates.Octet[0] = 0x82;		// 1M
			hBsRate = MGN_1M;
			lBsRate = MGN_1M;			
		}
		
	}

	if( hOpRate == 0x00 )	// i.e. not been updated. Annie, 2005-04-01.
	{
		RT_TRACE( COMP_SYSTEM, DBG_WARNING, ("Warning: Operational rate set is empty !\n") );
	}

	pMgntInfo->HighestBasicRate = hBsRate;
	pMgntInfo->LowestBasicRate = lBsRate;
	pMgntInfo->HighestOperaRate = hOpRate;

	// <6> CurrentBasicRate
	pMgntInfo->CurrentBasicRate = pMgntInfo->LowestBasicRate;

	// <7> CurrentOperaRate
	pMgntInfo->CurrentOperaRate = pMgntInfo->HighestOperaRate;

	// <8> SupportedRates, 2005.11.25, by rcnjko.	
	CopyMemOS( &(pMgntInfo->SupportedRates), pMgntInfo->dot11OperationalRateSet, pMgntInfo->dot11OperationalRateSet.Length);

	IOTActForcedDataRate(Adapter);

	// <9> 2009/04/13 MH Before any link sequence, we must change to correct rate setting. 
	// Otherwise, auth may fail if peer AP or STA do not accept our ACK rate.
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BASIC_RATE, (pu1Byte)(&pMgntInfo->mBrates) );

	// Keep the default basic rate. 2010.11.03. by tynli.

	RT_TRACE(COMP_MLME, DBG_TRACE, ("SelectRateSet(): return TRUE. <====\n"));
}


//
// Return BOOLEAN: TxRate is included in SupportedRates or not.
// Added by Annie, 2006-05-16, in SGS, for Cisco B mode AP. (with ERPInfoIE in its beacon.)
//
BOOLEAN
IncludedInSupportedRates(
	IN	PADAPTER	Adapter,
	IN	u1Byte		TxRate
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	POCTET_STRING	pSupRateSet = &(pMgntInfo->SupportedRates);
	u1Byte			RateMask = 0x7F;
	u1Byte			idx;
	BOOLEAN			Found = FALSE;
	u1Byte			NaiveTxRate = TxRate&RateMask;


	for( idx=0; idx<pSupRateSet->Length; idx++ )
	{
		if( (pSupRateSet->Octet[idx] & RateMask) == NaiveTxRate )
		{
			Found = TRUE;
			break;
		}
	}

	return Found;
}

//
// Description: Update bUseProtection accoding to the rule = (default | Ext. port).
//			The protection status of default port need to follow extension port and the 
//			extension port also need to follow default port when they are co-exist.
// 2010.11.02. Added by tynli.
//
VOID
ActUpdate_ProtectionMode(
	PADAPTER		Adapter,
	BOOLEAN		bProtection
)
{
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO	pDefaultMgntInfo = &(pDefaultAdapter->MgntInfo);
	PADAPTER		pExtAdapter = NULL;
	PMGNT_INFO	pExtMgntInfo = NULL;
	u1Byte 		TwoPortStatus = (u1Byte)TWO_PORT_STATUS__WITHOUT_ANY_ASSOCIATE;

	pExtAdapter = GetFirstExtAdapter(Adapter);
	if(pExtAdapter == NULL) pExtAdapter = pDefaultAdapter;
	
	pExtMgntInfo = &(pExtAdapter->MgntInfo);
	
	GetTwoPortSharedResource(Adapter,TWO_PORT_SHARED_OBJECT__STATUS,NULL,&TwoPortStatus);

	if(IsDefaultAdapter(Adapter)) //Default adapter
	{
		pDefaultMgntInfo->bProtection = bProtection;
		if(	(TWO_PORT_STATUS)TwoPortStatus == TWO_PORT_STATUS__EXTENSION_FOLLOW_DEFAULT
			|| (TWO_PORT_STATUS)TwoPortStatus == TWO_PORT_STATUS__DEFAULT_G_EXTENSION_N20)
		{
			pDefaultMgntInfo->bUseProtection = (bProtection|pExtMgntInfo->bProtection);
		}
		else
		{
			pDefaultMgntInfo->bUseProtection = bProtection;
		}
	}
	else 
	{
		pExtMgntInfo->bProtection = bProtection;
		if(	(TWO_PORT_STATUS)TwoPortStatus == TWO_PORT_STATUS__EXTENSION_FOLLOW_DEFAULT
			||(TWO_PORT_STATUS)TwoPortStatus == TWO_PORT_STATUS__DEFAULT_G_EXTENSION_N20)
		{
			pExtMgntInfo->bUseProtection = (bProtection|pDefaultMgntInfo->bProtection);
		}
		else
		{
			pExtMgntInfo->bUseProtection = bProtection;
		}
	}
}


VOID
ActUpdate_mCapInfo(
	PADAPTER		Adapter,
	u2Byte			updateCap
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	pMgntInfo->mCap = updateCap;
	
	// Check preamble mode, 2005.01.06, by rcnjko.
	// Mark to update preamble value forever, 2008.03.18 by lanhsin
//	if( pMgntInfo->RegPreambleMode == PREAMBLE_AUTO )
	{
		BOOLEAN		ShortPreamble;
			
		if(pMgntInfo->mCap & cShortPreamble)
		{ // Short Preamble
			if(pMgntInfo->dot11CurrentPreambleMode != PREAMBLE_SHORT) // PREAMBLE_LONG or PREAMBLE_AUTO
			{
				ShortPreamble = TRUE;
				pMgntInfo->dot11CurrentPreambleMode = PREAMBLE_SHORT;
				Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_ACK_PREAMBLE, (UCHAR *)&ShortPreamble );
			}
		}
		else
		{ // Long Preamble
			if(pMgntInfo->dot11CurrentPreambleMode != PREAMBLE_LONG)  // PREAMBLE_SHORT or PREAMBLE_AUTO
			{
				ShortPreamble = FALSE;
				pMgntInfo->dot11CurrentPreambleMode = PREAMBLE_LONG;
				Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_ACK_PREAMBLE, (UCHAR *)&ShortPreamble );
			}
		}
	}
	
	// STA's shall set MAC variable aSlotTime to short slot value upon transmission or
	// reception of Beacon, ProbeRsp, AssocRsp, and ReAssocRsp from the BSS that the 
	// STA has joined or started... (ref 802.11g/D8.2, sec. 7.3.1, p12.).
	if( IS_WIRELESS_MODE_G(Adapter))
	{
		u1Byte	slot_time_val;
		u1Byte	CurSlotTime;
		Adapter->HalFunc.GetHwRegHandler( Adapter, HW_VAR_SLOT_TIME, &(CurSlotTime) );		
		
		if( (updateCap & cShortSlotTime) && (!(pMgntInfo->pHTInfo->RT2RT_HT_Mode & RT_HT_CAP_USE_LONG_PREAMBLE)))
		{ // Short Slot Time
			if(CurSlotTime != SHORT_SLOT_TIME)
			{
				slot_time_val = SHORT_SLOT_TIME;
				Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_SLOT_TIME, &slot_time_val );
			}
		}
		else
		{ // Long Slot Time
			if(CurSlotTime != NON_SHORT_SLOT_TIME)
			{
				slot_time_val = NON_SHORT_SLOT_TIME;
				Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_SLOT_TIME, &slot_time_val );
			}
		}
	}
}



VOID
ActUpdate_ERPInfo(
	PADAPTER		Adapter,
	u1Byte			ERPInfo
)
{
	// [Win7] pChnlAccessSetting use the common part for both adapters. 2009.07.02, by Bohn
	Adapter = GetDefaultAdapter(Adapter);
	
	if( ERPInfo & ERP_UseProtection )
	{
		ActUpdate_ProtectionMode(Adapter, TRUE);
	}
	else
	{
		ActUpdate_ProtectionMode(Adapter, FALSE);
	}

	

	// 2005.01.06, by rcnjko. 
	if( Adapter->MgntInfo.RegPreambleMode == PREAMBLE_AUTO )
	{
		if( ERPInfo & ERP_BarkerPreambleMode )
		{
			Adapter->MgntInfo.dot11CurrentPreambleMode = PREAMBLE_LONG;
		}
	}
}


VOID 
EnableSingleAmpduorNot(
	PADAPTER		Adapter,
  	PRT_WLAN_BSS	pBssDesc
 )
{
	u1Byte	value = 0;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);
	
	if (!ENABLE_SINGLE_AMPDU(Adapter))	// Confirm in the future.
		return;

	if(Adapter->bInHctTest)
		return;

	if((pBssDesc->Vender==HT_IOT_PEER_CISCO) && (!ENABLE_SINGLE_AMPDU(Adapter)))
	{
		RT_TRACE(COMP_MLME,DBG_LOUD,("For Cisco AP,we disable Earlymode!!!\n"));
		pHalData->AMPDUBurstMode = RT_AMPDU_BRUST_NONE;
		PlatformEFIOWrite1Byte(Adapter,0x4d3,0);
		PlatformEFIOWrite1Byte(Adapter,0x4d0,0);
		return;
	}
	else  if(pHalData->AMPDUBurstModebackup != RT_AMPDU_BRUST_NONE &&  pHalData->AMPDUBurstMode == RT_AMPDU_BRUST_NONE)
	{
		//When connected with Cisco AP ,we disable earlymode. But if connected with other AP, we should restore it.by wl2012-01-12
		pHalData->AMPDUBurstMode = pHalData->AMPDUBurstModebackup;
		if(pHalData->AMPDUBurstMode)
		{	
			if(ENABLE_SINGLE_AMPDU(Adapter))
				PlatformEFIOWrite4Byte(Adapter,0x4d0,0x8104001f);
		}
	}
	
	if(!pHalData->AMPDUBurstMode)
		return;
	if((pBssDesc->Vender==HT_IOT_PEER_UNKNOWN)||
		(pBssDesc->Vender==HT_IOT_PEER_ATHEROS&&pBssDesc->SubTypeOfVender == HT_IOT_PEER_ATHEROS_DIR655))
	{
		//Single AMPDU will cause DIR 655 RX bad.
		value = 0;
		//Left at least two packets in TX FIFO(Totoal 12 packets, first AMPDU 10 packets) to trigger early mode but avoid Single AMPDU.  //By YJ,120210
		if(ENABLE_SINGLE_AMPDU(Adapter))
			PlatformEFIOWrite1Byte(Adapter, 0x04CA, 0x0A); 
	}
	else
	{
		value = 0x80;
		if(ENABLE_SINGLE_AMPDU(Adapter))
			PlatformEFIOWrite1Byte(Adapter, 0x04CA, 0x0B); 
	}

	if(ENABLE_SINGLE_AMPDU(Adapter))
		value |= BIT0;
	
	PlatformEFIOWrite1Byte(Adapter,0x4d3,value);		
}


WIRELESS_MODE
SetupJoinWirelessMode(
	PADAPTER		Adapter,
	BOOLEAN			bSupportNmode,
	WIRELESS_MODE	bssWirelessMode
)
{
	WIRELESS_MODE	JoinWirelessMode;
	u2Byte		 	SupportedWirelessMode = Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter);

	RT_TRACE(COMP_MLME,DBG_LOUD,("SetupJoinWirelessMode(), RegWirelessMode=%x bssWirelessMode=0x%x, SupportedWirelessMode=0x%x\n", Adapter->RegWirelessMode, bssWirelessMode, SupportedWirelessMode));
	 if(	(IS_24G_WIRELESS_MODE(Adapter->RegWirelessMode) && IS_24G_WIRELESS_MODE(bssWirelessMode)) ||
	 	(IS_5G_WIRELESS_MODE(Adapter->RegWirelessMode) && IS_5G_WIRELESS_MODE(bssWirelessMode))	)
	{
		if(Adapter->RegWirelessMode < bssWirelessMode)	// Why use the judgement?
		{
			JoinWirelessMode = Adapter->RegWirelessMode;
			
			 if (bssWirelessMode == WIRELESS_MODE_AC_24G)
			{
				JoinWirelessMode = WIRELESS_MODE_AC_24G;
			}
		}
		else
			JoinWirelessMode = bssWirelessMode;
	}
	else if(Adapter->RegWirelessMode == WIRELESS_MODE_AUTO)
	{
	 	if(bssWirelessMode <= WIRELESS_MODE_N_24G)
		{
			// 2004.10.18, by rcnjko.
			if(SupportedWirelessMode & (WIRELESS_MODE_G|WIRELESS_MODE_N_24G) && bssWirelessMode >=WIRELESS_MODE_G)
			{
				if(	(SupportedWirelessMode & WIRELESS_MODE_N_24G) && bSupportNmode
					&& bssWirelessMode >=WIRELESS_MODE_N_24G && IS_HT_SUPPORTED(Adapter))
				{
					JoinWirelessMode = WIRELESS_MODE_N_24G;
				}
				else
					JoinWirelessMode = WIRELESS_MODE_G;
			}
			else
			{
				if( (SupportedWirelessMode&WIRELESS_MODE_A)&&(bssWirelessMode==WIRELESS_MODE_A))
					JoinWirelessMode = WIRELESS_MODE_A;
				else if( (SupportedWirelessMode&WIRELESS_MODE_B)&&(bssWirelessMode==WIRELESS_MODE_B))
					JoinWirelessMode = WIRELESS_MODE_B;
				else
					JoinWirelessMode = WIRELESS_MODE_B;
			}
		}
		else if (bssWirelessMode == WIRELESS_MODE_AC_24G)
		{
			JoinWirelessMode = WIRELESS_MODE_AC_24G;
		}
		else
		{
			if((SupportedWirelessMode & WIRELESS_MODE_AC_5G) && bSupportNmode
				&& bssWirelessMode >=WIRELESS_MODE_AC_5G && IS_VHT_SUPPORTED(Adapter))
			{
				JoinWirelessMode = WIRELESS_MODE_AC_5G;
			}
			else if((SupportedWirelessMode & WIRELESS_MODE_N_5G) && bSupportNmode
				&& bssWirelessMode >=WIRELESS_MODE_N_5G && IS_HT_SUPPORTED(Adapter))
			{
				JoinWirelessMode = WIRELESS_MODE_N_5G;
			}
			else
				JoinWirelessMode = WIRELESS_MODE_A;
		}
	 }
	else
	{
		JoinWirelessMode = Adapter->RegWirelessMode;
	}

	if((!bSupportNmode) && (JoinWirelessMode >= WIRELESS_MODE_N_24G))
	{
		if(IS_5G_WIRELESS_MODE(JoinWirelessMode))
			JoinWirelessMode = WIRELESS_MODE_A;
		else
			JoinWirelessMode = WIRELESS_MODE_G;
	}

	RT_TRACE(COMP_MLME,DBG_LOUD,("JoinWirelessMode = %x\n", JoinWirelessMode));

	return JoinWirelessMode;
}

WIRELESS_MODE
SetupStarWirelessMode(
	PADAPTER		Adapter,
	BOOLEAN			bSupportNmode
)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	WIRELESS_MODE	WirelessMode = 0;
	u2Byte		 	SupportedWirelessMode = Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter);

	if(Adapter->RegWirelessMode == WIRELESS_MODE_AUTO)
	{
		if(pMgntInfo->dot11CurrentChannelNumber <= 14)
		{
			// 2013/10/04 MH Add registry for 2.4G wireless mode VHT enabled.
			if(pMgntInfo->bRegVht24g && (SupportedWirelessMode & (WIRELESS_MODE_AC_24G)))
			{				
				WirelessMode = WIRELESS_MODE_AC_24G;
			}			
			else if(SupportedWirelessMode & (WIRELESS_MODE_G|WIRELESS_MODE_N_24G))
			{
				if(	(SupportedWirelessMode & WIRELESS_MODE_N_24G) && bSupportNmode)
					WirelessMode = WIRELESS_MODE_N_24G;
				else
					WirelessMode = WIRELESS_MODE_G;
			}
			else
				WirelessMode = WIRELESS_MODE_B;
		}
		else
		{
			if((SupportedWirelessMode & WIRELESS_MODE_AC_5G) && bSupportNmode)
				WirelessMode = WIRELESS_MODE_AC_5G;
			else if((SupportedWirelessMode & WIRELESS_MODE_N_5G) && bSupportNmode)
				WirelessMode = WIRELESS_MODE_N_5G;
			else
				WirelessMode = WIRELESS_MODE_A;
		}
	}
	else 
	{
		if( IS_5G_WIRELESS_MODE(Adapter->RegWirelessMode))
		{
			if(pMgntInfo->dot11CurrentChannelNumber <= 14)
				pMgntInfo->dot11CurrentChannelNumber = 36;

			if (bSupportNmode == FALSE)
				WirelessMode = WIRELESS_MODE_A;
			else
				WirelessMode = Adapter->RegWirelessMode;
		}
		else if(IS_24G_WIRELESS_MODE(Adapter->RegWirelessMode))
		{
			if(pMgntInfo->dot11CurrentChannelNumber > 14)
				pMgntInfo->dot11CurrentChannelNumber = 1;

			// Set wireless mode to refresh all channel access related setting. 2006.02.16, by rcnjko.
		 	if (Adapter->RegWirelessMode == WIRELESS_MODE_N_24G || Adapter->RegWirelessMode == WIRELESS_MODE_AC_24G)
			{
				if (bSupportNmode)
					WirelessMode = Adapter->RegWirelessMode;
				else
					WirelessMode = WIRELESS_MODE_G;
			}	
			else
				WirelessMode =  Adapter->RegWirelessMode;
		}
	}

	return WirelessMode;
}

VOID
SetupIOTAction(
	PADAPTER		Adapter,
	RT_WLAN_BSS		*bssDesc
)
{	
	BOOLEAN 	bIOTAction = 0;	
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	bIOTAction = IOTActWAIOTBroadcom(Adapter, bssDesc); // only broadcom
	if(bIOTAction)
	{
		RT_DISP(FDM, WA_IOT, ("SetupJoinInfraInfo() set bIOTAction for HT_IOT_ACT_WA_IOT_Broadcom\n"));
		pMgntInfo->IOTAction |= HT_IOT_ACT_WA_IOT_Broadcom;
	}

	bIOTAction = IOTActBcmRxFailRelink(Adapter, bssDesc);
	if(bIOTAction)
	{
		RT_DISP(FDM, WA_IOT, ("HTResetSelfAndSavePeerSetting() set bIOTAction for HT_IOT_ACT_WA_IOT_Broadcom\n"));
		pMgntInfo->IOTAction |= HT_IOT_ACT_BCM_AP_RX_FAIL_RELINK;
	}

	bIOTAction = IOTActIsDisableEDCATurbo(Adapter, bssDesc);
	if(bIOTAction)
		pMgntInfo->IOTAction |= HT_IOT_ACT_DISABLE_EDCA_TURBO;

	bIOTAction = IOTActIsEnableBETxOPLimit(Adapter, bssDesc);
	if(bIOTAction)
		pMgntInfo->IOTAction |= HT_IOT_ACT_FORCED_ENABLE_BE_TXOP;
	
	bIOTAction = IOTActIsForcedRTSCTS(Adapter, bssDesc);
	if(bIOTAction)
		pMgntInfo->IOTAction |= HT_IOT_ACT_FORCED_RTS;

	bIOTAction = IOTActIsForcedCTS2Self(Adapter, bssDesc);
	if(bIOTAction)
		pMgntInfo->IOTAction |= HT_IOT_ACT_FORCED_CTS2SELF;

	bIOTAction = IOTActIsNullDataPowerSaving(Adapter, bssDesc);
	if(bIOTAction)
		pMgntInfo->IOTAction |= HT_IOT_ACT_NULL_DATA_POWER_SAVING;

	bIOTAction = IOTActIsDisableTxPowerTraining(Adapter, bssDesc);
	if(bIOTAction)
	{
		pMgntInfo->bDisableTXPowerTraining = TRUE;
	}

	bIOTAction = IOTActIsDisableProtectionMode(Adapter, bssDesc);
	if(bIOTAction)
	{
		pMgntInfo->ForcedProtectionMode = PROTECTION_MODE_FORCE_DISABLE;
	}

	bIOTAction = IOTActIsForcedDataRate(Adapter, bssDesc);
	if(bIOTAction)
	{
		pMgntInfo->IOTAction |= HT_IOT_ACT_FORCED_DATA_RATE;
	}

	bIOTAction = IOTActIsDisableTxPowerTraining(Adapter, bssDesc);
	if(bIOTAction)
	{
		pMgntInfo->bDecreaseTXPowerTraining = TRUE;
	}

}


VOID
SetupJoinInfraInfo(
	PADAPTER		Adapter,
	RT_WLAN_BSS		*bssDesc
)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PRT_SECURITY_T	pSecInfo = &pMgntInfo->SecurityInfo;
	OCTET_STRING	supprateEX;
	BOOLEAN	 		bSupportNmode = TRUE, bHalfSupportNmode = FALSE; // default aes or none encryption
	u1Byte			CurrCcxVerNumber = 0;
	u2Byte			nDataRate = 0;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  SetupJoinInfraInfo()\n"));

	pMgntInfo->bJoinInProgress = TRUE;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

	CopyWlanBss(&pMgntInfo->targetBSS, bssDesc);

	// Return to previous IPS mode. 2007.08.
	if (!pMgntInfo->bSetPnpPwrInProgress)
		IPSReturn(Adapter, IPS_DISABLE_DEF_OPMODE);
	
	//Emily. 20051005. In WPA/WPA2 mixed mode, group key cipher will not be set from WZC/UI,
	//		we have to get it from parsing of beacon.	
	if(pSecInfo->AuthMode > RT_802_11AuthModeAutoSwitch)
	{
		pSecInfo->bGroupKeyFixed = TRUE;
		
		if (pSecInfo->AuthMode != RT_802_11AuthModeWAPI &&  pSecInfo->AuthMode != RT_802_11AuthModeCertificateWAPI)
			SecGetGroupCipherFromBeacon(Adapter, bssDesc);
	}

	if(P2P_ADAPTER_OS_SUPPORT_P2P(Adapter) && P2P_ACTING_IS_CLIENT(Adapter))
	{
		u1Byte	BoostInitGainValue = 0;

		BoostInitGainValue = (u1Byte)(bssDesc->RecvSignalPower+110);

		McDynamicMachanismSet(Adapter, MC_DM_INIT_GAIN_BOOST_START, &BoostInitGainValue, sizeof(u1Byte));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] JoinRequest, need to pause DIG and increase initial GAIN; SignalStrength(%#x)\n", bssDesc->RecvSignalPower));		
	}

	//WIRELESS MODE
	// Set wireless mode to N mode if using SW wep/tkip encyption
	// Set wireless mode to half N mode if using HW wep/tkip encryption
	// Set wireless mode to B/G mode if using WEP/TKIP encryption
	// 2008.06.12
	// 2010/06/25 hpfan: determine IOT Peer before checking GetNmodeSupportBySecCfgHandler
	ResetIOTSetting(pMgntInfo);	
	IOTPeerDetermine(Adapter, bssDesc);
	bSupportNmode = Adapter->HalFunc.GetNmodeSupportBySecCfgHandler(Adapter);
	
 	if(!bSupportNmode)
	{
		bSupportNmode = FALSE;
		bHalfSupportNmode = FALSE;
	}

	// Added by Bruce, 2010-12-15.
	// The WiFi Test Plan restricts the STA uses legacy mode in TKIP or WEP setting in Test 5.2.51/52.
	if(bSupportNmode && pMgntInfo->bWiFiConfg)
	{
		if(pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP || (pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP104) || (pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40))
			bSupportNmode = FALSE;
	}
	
	HTInitializeHTInfo(Adapter);
	VHTInitializeVHTInfo(Adapter);

	pMgntInfo->CurrentBssWirelessMode = bssDesc->wirelessmode; // Keep wireless mode of Infra-BSS to join. 2005.02.17, by rcnjko.
	pMgntInfo->SettingBeforeScan.WirelessMode = pMgntInfo->dot11CurrentWirelessMode;//For N solution won't be change the wireless mode in scan
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_ANT_SWITCH, (pu1Byte)(&pMgntInfo->dot11CurrentWirelessMode));
    // BSSID
	CopyMem( pMgntInfo->Bssid, bssDesc->bdBssIdBuf, 6 );
	RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "SetupJoinInfraInfo() BSSID:", pMgntInfo->Bssid);

    // SSID
	if((BeHiddenSsid(bssDesc->bdSsIdBuf,bssDesc->bdSsIdLen)) && (CHECK_HIDDEN_SSID(Adapter)))
	{
		CopySsid(bssDesc->bdSsIdBuf, bssDesc->bdSsIdLen, pMgntInfo->SsidBuf, pMgntInfo->Ssid.Length);
	}
	else
	{
		pMgntInfo->Ssid.Octet = pMgntInfo->SsidBuf;
		CopySsid(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, bssDesc->bdSsIdBuf, bssDesc->bdSsIdLen);
	}

     // Supported rates
	supprateEX.Octet = bssDesc->bdSupportRateEXBuf;
	supprateEX.Length = bssDesc->bdSupportRateEXLen;

	// Beacon period
	pMgntInfo->dot11BeaconPeriod = bssDesc->bdBcnPer;

	// DTIM period
	pMgntInfo->dot11DtimPeriod = bssDesc->bdDtimPer;

	pMgntInfo->ChannelOffset = (EXTCHNL_OFFSET)GET_HT_INFO_ELE_EXT_CHL_OFFSET(bssDesc->BssHT.bdHTInfoBuf);		
	// Channel number
	pMgntInfo->dot11CurrentChannelNumber = bssDesc->ChannelNumber;

	// Notify AP mode STA changes channel, bw and wireless mode
	if((GetDefaultMgntInfo(Adapter))->RegMultiChannelFcsMode != MULTICHANNEL_FCS_SUPPORT_GO)
	{
		EXT_AP_NOTIFY_STA_LINK_CHANGE(pMgntInfo);
	}
	
	// Capability, 2005.04.14, by rcnjko.
	pMgntInfo->mCap = bssDesc->bdCap;
	if( pMgntInfo->RegPreambleMode == PREAMBLE_AUTO )
	{
		if( (pMgntInfo->mCap & cShortPreamble) )
			pMgntInfo->dot11CurrentPreambleMode = PREAMBLE_SHORT;
		else
			pMgntInfo->dot11CurrentPreambleMode = PREAMBLE_LONG;
	}

	if(IS_WIRELESS_OFDM_24G(Adapter) && bssDesc->bERPInfoValid)
	{
		ActUpdate_ERPInfo(Adapter, bssDesc->bdERPInfo);
	}

	pMgntInfo->mAssoc = FALSE; //Set TRUE while receiving association response
	pMgntInfo->mIbss = FALSE; //
	pMgntInfo->mDisable = TRUE;
	
	pMgntInfo->AsocTimestamp = 0;
	pMgntInfo->AuthRetryCount = 0;
	PlatformCancelTimer(Adapter, &pMgntInfo->AuthTimer);
	PlatformCancelTimer(Adapter, &pMgntInfo->AsocTimer);

	// -- HalMacId: Set a specific MacId for this client ---
	pMgntInfo->mMacId = (u1Byte)MacIdRegisterMacIdForInfraClient(Adapter);
	pMgntInfo->mcastMacId = (u1Byte)MacIdRegisterMacIdForMCastClient(Adapter);
	// -------------------------------------------
	
	//stop BE Queue and VI Queue when another port is connectiong to network with another channel
	//by sherry 20130117
	{
		PADAPTER	pDefAdapter = GetDefaultAdapter(Adapter);
		PMGNT_INFO	pDefMgntInfo = &pDefAdapter->MgntInfo;
		u1Byte		TxPauseCommand = 0x6;


		if(Adapter == pDefAdapter)
		{
			RT_TRACE(COMP_MLME,DBG_LOUD,("SetupjoinInfraInfo: defalut port to connect \n"));
			if(GetFirstClientPort(Adapter) != NULL)
			{
				if((MgntLinkStatusQuery(GetFirstClientPort(Adapter)) == RT_MEDIA_CONNECT)  && (pMgntInfo->dot11CurrentChannelNumber != GET_HAL_DATA(Adapter)->CurrentChannel))
				{
					RT_TRACE(COMP_MLME,DBG_LOUD,("p2p client is linked \n "));
					if(pDefMgntInfo->RegMultiChannelFcsMode == 0)
					{
						TxPauseCommand = 0x2;
						Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_TX_PAUSE, (pu1Byte)(&TxPauseCommand));
						RT_TRACE(COMP_MLME, DBG_LOUD, ("Tx pause queue (%x)\n", TxPauseCommand));					
					}
				}
			}
		}
		else
		{
			RT_TRACE(COMP_MLME,DBG_LOUD,("SetupjoinInfraInfo: client port to connect \n"));
			if((MgntLinkStatusQuery(pDefAdapter) == RT_MEDIA_CONNECT) && (pMgntInfo->dot11CurrentChannelNumber != GET_HAL_DATA(Adapter)->CurrentChannel))
			{
				RT_TRACE(COMP_MLME,DBG_LOUD,("default port is linked \n "));
				if(pDefMgntInfo->RegMultiChannelFcsMode == 0)
				{
					TxPauseCommand = 0x4;
					SendNullFunctionData(pDefAdapter,pDefMgntInfo->Bssid,TRUE);
					PlatformStallExecution(10000);
					Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_TX_PAUSE, (pu1Byte)(&TxPauseCommand));
					RT_TRACE(COMP_MLME, DBG_LOUD, ("Tx pause queue (%x)\n", TxPauseCommand));				
				}
			}
		}
	}

	// 2008.06.12, by Lanhsin. Support half n wireless for ralink ap
	if((pMgntInfo->dot11CurrentWirelessMode == WIRELESS_MODE_N_24G) && (bHalfSupportNmode == TRUE))
		pMgntInfo->bHalfWiirelessN24GMode = TRUE;
	else 
		pMgntInfo->bHalfWiirelessN24GMode = FALSE;

	// Set QoS mode. Added by Annie, 2005-11-09. Isaiah for WMM_APSD
	if( pMgntInfo->pStaQos->QosCapability & QOS_WMM )
	{
		pMgntInfo->pStaQos->CurrentQosMode= (bssDesc->BssQos.bdQoSMode) & 
											(pMgntInfo->pStaQos->QosCapability);
		if(pMgntInfo->pStaQos->CurrentQosMode & QOS_WMM_UAPSD)
		{
			u1Byte	qosinfo;
			
			pMgntInfo->pStaQos->Curr4acUapsd = pMgntInfo->pStaQos->b4ac_Uapsd & 0x0F;

			SET_WMM_QOS_INFO_FIELD(&qosinfo, (pMgntInfo->pStaQos->Curr4acUapsd) & 0x0F);
			SET_WMM_QOS_INFO_FIELD_STA_MAX_SP_LEN(&qosinfo, (pMgntInfo->pStaQos->MaxSPLength) & 0x03);
			SET_WMM_INFO_ELE_QOS_INFO_FIELD(pMgntInfo->pStaQos->pWMMInfoEle, qosinfo);
		
		}else //legacy AP or WMM AP
		{
			SET_WMM_INFO_ELE_QOS_INFO_FIELD(pMgntInfo->pStaQos->pWMMInfoEle, 0);
			pMgntInfo->pStaQos->Curr4acUapsd = 0;
		}

		pMgntInfo->pStaQos->QBssWirelessMode = bssDesc->wirelessmode;
	}
	else //legacy
	{
		pMgntInfo->pStaQos->CurrentQosMode = QOS_DISABLE;
		pMgntInfo->pStaQos->QBssWirelessMode = WIRELESS_MODE_UNKNOWN;
	}

	Adapter->HalFunc.SetWirelessModeHandler( 	Adapter, 
											SetupJoinWirelessMode(Adapter, bSupportNmode, bssDesc->wirelessmode));

	//
	// Set High Throughput Configuration
	//
	if(	(pMgntInfo->pStaQos->CurrentQosMode & QOS_WMM) &&
		(pMgntInfo->pHTInfo->bEnableHT && bssDesc->BssHT.bdSupportHT))
	{
		HTResetSelfAndSavePeerSetting(Adapter, bssDesc);
		
		if(pMgntInfo->pVHTInfo->bEnableVHT && bssDesc->BssVHT.bdSupportVHT)
			VHTResetSelfAndSavePeerSetting(Adapter, bssDesc);
		else
			pMgntInfo->pVHTInfo->bCurrentVHTSupport = FALSE;
	}
	else
	{
		pMgntInfo->pHTInfo->bCurrentHTSupport = FALSE;
		pMgntInfo->pVHTInfo->bCurrentVHTSupport = FALSE;
	}

	if((GetDefaultMgntInfo(Adapter))->RegMultiChannelFcsMode == MULTICHANNEL_FCS_SUPPORT_GO)
	{
		u2Byte	OpCode = FCS_CHNL_OPMODE_JOINBSS;
		
		FCS_Notify(Adapter, FCS_NOTIFY_TYPE_CHANNEL, (pu1Byte)&OpCode, sizeof(OpCode));
	}	

	RT_TRACE(COMP_MLME, DBG_TRACE, ("Wireless mode %#x \n", pMgntInfo->dot11CurrentWirelessMode));
	
	// Change bandwidth before join process because the AP may send the Auth and Assoc Rsp frame with
	// fixed rate in 40MHz/80Mhz.
	CHNL_SetBwChnlFromPeer(Adapter);

	HTSetCCKSupport(Adapter, FALSE, NULL);

	SelectRateSet( Adapter, supprateEX );
	
	SetupIOTAction(Adapter, bssDesc);

	// Reset TStream related, 070614, by rcnjko.
	// If we are in calling session under CCX4, not to remove TSs.
	// 2007.07.29, by shien chang.
	CCX_QueryVersionNum(Adapter, &CurrCcxVerNumber);
					
	if (   ((pMgntInfo->pStaQos->CurrentQosMode & QOS_WMM) ||
		(pMgntInfo->pStaQos->CurrentQosMode & QOS_WMMSA)) &&
		( CurrCcxVerNumber >= 4 )  )
	{
		u1Byte			idx;
		PQOS_TSTREAM	pTs;
		for (idx=0; idx<2; idx++)
		{
			pTs = NULL;
			pTs = QosGetTs(Adapter, idx, pMgntInfo->Bssid, Adapter->CurrentAddress);
			if (pTs && pTs->bUsed)
				pTs->bEstablishing = TRUE;
		}
	}
	else
	{
		QosResetAllTs(Adapter); 
	}
	
	// Reset Hw sequence number. 2012.11.30. by tynli.
	pMgntInfo->SequenceNumber = 0;
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_NQOS_SEQ_NUM, (pu1Byte)&pMgntInfo->SequenceNumber); 
	
	// For debugging ONLY. Annie, 2005-11-12.
	QosParsingDebug_BssDesc( bssDesc );

	// Initialize CCX setting to join the BSS, 070627, by rcnjko.
	CCX_OnSetupJoinInfraInfo(Adapter, bssDesc);

	// For Turbo Mode resetting. Added by Roger, 2006.12.15.
	pMgntInfo->TurboChannelAccess.bEnabled=FALSE;
	MgntActSet_EnterTurboMode( Adapter, FALSE); 

	pMgntInfo->targetAKMSuite = bssDesc->AKMsuit;
	//
	// Initialize 802.11 MFP 
	//
	PlatformZeroMemory( pSecInfo->BIPKeyBuffer, MAX_KEY_LEN );

	if( bssDesc->bMFPC && IS_AKM_SUPPORT_SHA_256(pMgntInfo->RegCapAKMSuite)) 
	{
		pMgntInfo->bInBIPMFPMode = TRUE;				
	}
	else
	{
		pMgntInfo->bInBIPMFPMode = FALSE;

	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));
		return;
	}

	// Preprocessing for starting a connection. 2006.10.17, by shien chang.
	if (!MgntRoamingInProgress(pMgntInfo) && pMgntInfo->RequestFromUplayer)
	{
		DrvIFIndicateConnectionStart(Adapter);
	}

	//vivi, dir655 can not receive single ampdu; so we have to turn off single ampdu when peer ap is dlink 655, 20101125
	//as we can not recongize dir655, atheros or ap which doesnot carry vender information will be treated as no single ampdu received
	EnableSingleAmpduorNot(Adapter, bssDesc);
	
	nDataRate = ( IS_WIRELESS_MODE_5G(Adapter) ||
				(IS_WIRELESS_MODE_HT_24G(Adapter) && !pMgntInfo->pHTInfo->bCurSuppCCK)) ? MGN_6M: MGN_1M;

	//For Win8 InstantConnect_ext test ,complete connect in 0.075s
	if((Adapter->bInHctTest || pMgntInfo->IsAMDIOIC))
	{
		SendProbeReq(Adapter, FALSE, nDataRate, TRUE);
		SendProbeReq(Adapter, FALSE, nDataRate, TRUE);
	}

	pSecInfo->TxIV =  0;	
	
	RT_TRACE(COMP_MLME, DBG_TRACE, ("SetupJoinInfraInfo(): bJoinInProgress = %d\n", pMgntInfo->bJoinInProgress));
	RT_TRACE(COMP_MLME, DBG_LOUD, ("SetupJoinInfraInfo() <====\n"));
}


VOID
SetupJoinIBSSInfo(
	PADAPTER		Adapter,
	RT_WLAN_BSS		*bssDesc
)
{
	PMGNT_INFO     	pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING	supprateEX;
	BOOLEAN	 		bSupportNmode = FALSE;
	HAL_AP_IBSS_INT_MODE	intMode = HAL_AP_IBSS_INT_IBSS;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  SetupJoinIBSSInfo()\n"));

	pMgntInfo->bJoinInProgress = TRUE;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

	// Disable IPS, 2007.08.17, by shien chang.
	IPSDisable(Adapter,FALSE, IPS_DISABLE_DEF_OPMODE);
	//[Win7 Two Port Status] This init must before Set Wireless mode and every thing, 2009.07.31, by Bohn
	pMgntInfo->mAssoc = FALSE;
	pMgntInfo->mIbss = TRUE;
	pMgntInfo->mDisable = TRUE;
	pMgntInfo->bStartApDueToWakeup=FALSE;
	
	// WIRELESS MODE
	// Set wireless mode to B/G mode if using WEP/TKIP encryption
	//bSupportNmode = Adapter->HalFunc.GetNmodeSupportBySecCfgHandler(Adapter);
	if(pMgntInfo->bReg11nAdhoc || pMgntInfo->bRegVht24g)
		bSupportNmode = TRUE;

	HTInitializeHTInfo(Adapter);
	VHTInitializeVHTInfo(Adapter);
			
	pMgntInfo->CurrentBssWirelessMode = bssDesc->wirelessmode; // Keep wireless mode of IBSS to join. 2005.02.17, by rcnjko.
	pMgntInfo->SettingBeforeScan.WirelessMode = pMgntInfo->dot11CurrentWirelessMode;//For N solution won't be change the wireless mode in scan
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_ANT_SWITCH, (pu1Byte)(&pMgntInfo->dot11CurrentWirelessMode));	
	// BSSID
	CopyMem( pMgntInfo->Bssid, bssDesc->bdBssIdBuf, 6 );

    // SSID
	CopyMem( pMgntInfo->SsidBuf, bssDesc->bdSsIdBuf, bssDesc->bdSsIdLen );
	CopySsid(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, bssDesc->bdSsIdBuf, bssDesc->bdSsIdLen);

	//Rate set
	supprateEX.Octet = bssDesc->bdSupportRateEXBuf;
	supprateEX.Length = bssDesc->bdSupportRateEXLen;

	pMgntInfo->dot11CurrentChannelNumber = bssDesc->ChannelNumber;
	pMgntInfo->dot11BeaconPeriod = bssDesc->bdBcnPer;

	// Capability, 2005.03.15, by rcnjko.
	pMgntInfo->mCap = bssDesc->bdCap;
	if( pMgntInfo->RegPreambleMode == PREAMBLE_AUTO )
	{
		if( (pMgntInfo->mCap & cShortPreamble) )
		{
			pMgntInfo->dot11CurrentPreambleMode = PREAMBLE_SHORT;
		}
		else
		{
			pMgntInfo->dot11CurrentPreambleMode = PREAMBLE_LONG;
		}
	}

	//
	// Disable Short slot time. Added by Annie, 2005-11-17.
	// 
	// Ref: 802.11g/D8.2, sec. 7.3.1.4, p13. First line...
	// 	"For IBSS, the Short Slot Time subfield shall be set to 0."
	//
	if( pMgntInfo->mCap & cShortSlotTime )
	{
		pMgntInfo->mCap &= (~cShortSlotTime);
	}

	if(IS_WIRELESS_OFDM_24G(Adapter) && bssDesc->bERPInfoValid)
	{
		ActUpdate_ERPInfo(Adapter, bssDesc->bdERPInfo);
	}

	// Reset roaming status, 2004.10.12, by rcnjko.
	pMgntInfo->AsocRetryCount = 0;

	// Add by hpfan to prevent infrastructure and adhoc mode connect at the same
	PlatformCancelTimer( Adapter, &pMgntInfo->AuthTimer );

	// 2008.06.12, by Lanhsin.
	pMgntInfo->bHalfWiirelessN24GMode = FALSE;

	RT_TRACE(COMP_SEC , DBG_LOUD , ("====>SWRxDecryptFlag = %d \n",pMgntInfo->SecurityInfo.SWRxDecryptFlag) );

	if(!pMgntInfo->SecurityInfo.SWRxDecryptFlag)
		Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);
	else
	       Adapter->HalFunc.DisableHWSecCfgHandler(Adapter);

	if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm!= RT_ENC_ALG_NO_CIPHER)
		pMgntInfo->mCap |= cPrivacy;
	else
		pMgntInfo->mCap &= ~cPrivacy;

	// Set QoS mode. Added by Annie, 2005-11-09.
	if( (pMgntInfo->pStaQos->QosCapability & QOS_WMM) &&  (bssDesc->BssQos.bdQoSMode & QOS_WMM))
	{
		RT_TRACE(COMP_QOS, DBG_LOUD, 
		("SetupJoinIBSSInfo Set a CurrentQosMode = QOS_WMM\r\n"));
		pMgntInfo->pStaQos->CurrentQosMode = QOS_WMM;
		pMgntInfo->pStaQos->QBssWirelessMode = pMgntInfo->CurrentBssWirelessMode;
		pMgntInfo->mCap |= cQos;
			
		// Use default EDCA parameters
		SET_WMM_PARAM_ELE_AC_PARAMS(pMgntInfo->pStaQos->WMMParamEle, Adapter->STA_EDCA_PARAM);
	}
	else
	{
		RT_TRACE(COMP_QOS, DBG_LOUD, 
		("SetupJoinIBSSInfo Set a CurrentQosMode = QOS_DISABLE\r\n"));
		pMgntInfo->pStaQos->CurrentQosMode = QOS_DISABLE;
		pMgntInfo->pStaQos->QBssWirelessMode = WIRELESS_MODE_UNKNOWN;
		pMgntInfo->mCap &= ~cQos;
	}

	//
	// Set High Throughput Configuration
	//
	ResetIOTSetting(pMgntInfo);
	if(	(pMgntInfo->pStaQos->CurrentQosMode & QOS_WMM) &&
		 (pMgntInfo->pHTInfo->bEnableHT && bssDesc->BssHT.bdSupportHT))
	{
		HTResetSelfAndSavePeerSetting(Adapter, bssDesc);

		//Added by sherry for vht  2011227
		if(pMgntInfo->pVHTInfo->bEnableVHT && bssDesc->BssVHT.bdSupportVHT)
			VHTResetSelfAndSavePeerSetting(Adapter, bssDesc);
		else
			pMgntInfo->pVHTInfo->bCurrentVHTSupport = FALSE;
	}
	else
	{
		pMgntInfo->pHTInfo->bCurrentHTSupport = FALSE;
		pMgntInfo->pVHTInfo->bCurrentVHTSupport = FALSE;
	}
	
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_AP_IBSS_INTERRUPT, (pu1Byte)&intMode);

	Adapter->HalFunc.SetWirelessModeHandler( 	Adapter, 
											SetupJoinWirelessMode(Adapter, bSupportNmode, bssDesc->wirelessmode));

	// Change bandwidth before join process because the AP may send the Auth and Assoc Rsp frame with
	// fixed rate in 40MHz/80Mhz.
	CHNL_SetBwChnlFromPeer(Adapter);

	HTSetCCKSupport(Adapter, FALSE, NULL);
	
	SelectRateSet( Adapter, supprateEX );

	IOTPeerDetermine(Adapter,bssDesc);

	// Reset TStream related, 070614, by rcnjko.
	QosResetAllTs(Adapter); 
	
	// Reset CCX related setting.
	CCX_OnBssReset(Adapter);

	// For Turbo Mode resetting. Added by Roger, 2006.12.15.
	pMgntInfo->TurboChannelAccess.bEnabled=FALSE;
	MgntActSet_EnterTurboMode( Adapter, FALSE); 

	// Reset all association entry.
	AsocEntry_ResetAll(Adapter);

	// Preprocessing for starting a connection. 2006.10.17, by shien chang.
	MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, bssDesc->bdMacAddressBuf, 6);
	
	RT_TRACE(COMP_INIT, DBG_TRACE,("SetupJoinIBSSInfo(): \n"));

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));
		return;
	}
	
	//
	//Porting from 8xb to avoid IBSS could't reconnected issue after s3/s4, by Maddest.
	//
	if(!pMgntInfo->bInToSleep &&
		!MgntRoamingInProgress(pMgntInfo) && pMgntInfo->RequestFromUplayer)
		DrvIFIndicateConnectionStart(Adapter);
	else
	{
		MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_NORMAL, ROAMINGSTATE_SCANNING);
		DrvIFIndicateRoamingStart( Adapter );
	}

//	pMgntInfo->bJoinInProgress = TRUE;
	RT_TRACE(COMP_MLME, DBG_TRACE, ("SetupJoinIBSSInfo(): bJoinInProgress = %d\n", pMgntInfo->bJoinInProgress));
	RT_TRACE(COMP_MLME, DBG_LOUD, ("SetupJoinIBSSInfo() <====\n"));
}


void
SetupStartIBSSInfo(
	PADAPTER		Adapter
)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	BOOLEAN	 		bSupportNmode = TRUE;
	OCTET_STRING	SuppRateSets = pMgntInfo->Regdot11OperationalRateSet;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  SetupStartIBSSInfo() bIbssStarter: %x.\n", pMgntInfo->bIbssStarter));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

	// Disable IPS, 2007.08.17, by shien chang.
	IPSDisable(Adapter,FALSE, IPS_DISABLE_DEF_OPMODE);
	
	if(!pMgntInfo->bIbssStarter)
	{
		// <1>BSSID	
		u1Byte	rbssid[6];
		u4Byte	 sed= (u4Byte)(PlatformGetCurrentTime() & 0xffffffff);
		
		rbssid[0] = 0x0;
		rbssid[1] = 0xe0;
		rbssid[2] = (u1Byte)((sed & 0xff000000)>>24);
		rbssid[3] = (u1Byte)((sed & 0x00ff0000)>>16);
		rbssid[4] = (u1Byte)((sed & 0x0000ff00)>>8);
		rbssid[5] = (u1Byte)((sed & 0x000000ff));
		PlatformMoveMemory(pMgntInfo->Bssid, rbssid, 6);
		//Make sure this is a unicast address
		pMgntInfo->Bssid[0] = (pMgntInfo->Bssid[0] & 0x0FC) | 0x02;

		pMgntInfo->bIbssStarter=TRUE;
	}

	pMgntInfo->mAssoc = FALSE;
	pMgntInfo->mIbss = TRUE;
	pMgntInfo->mDisable = TRUE;
	pMgntInfo->bStartApDueToWakeup=FALSE;
	pMgntInfo->dot11BeaconPeriod = pMgntInfo->Regdot11BeaconPeriod;

	pMgntInfo->mCap |= cIBSS;
	pMgntInfo->mCap &= (~cESS);

	// Preamble mode.
	if(pMgntInfo->RegPreambleMode != PREAMBLE_LONG)
	{
		pMgntInfo->mCap |= cShortPreamble;
	}
	else
	{
		pMgntInfo->mCap &= (~cShortPreamble);
	}

	//
	// Disable Short slot time. Added by Annie, 2005-11-17.
	// 
	// Ref: 802.11g/D8.2, sec. 7.3.1.4, p13. First line...
	// 	"For IBSS, the Short Slot Time subfield shall be set to 0."
	//
	if( pMgntInfo->mCap & cShortSlotTime )
	{
		pMgntInfo->mCap &= (~cShortSlotTime);
	}

	// Set wireless mode to B/G mode if using WEP/TKIP encryption
	//bSupportNmode = Adapter->HalFunc.GetNmodeSupportBySecCfgHandler(Adapter);
	HTInitializeHTInfo(Adapter);
	VHTInitializeVHTInfo(Adapter);

	// Start N mode AP
	// Start N mode IBSS, if register allow it.
	if(ACTING_AS_AP(Adapter) == FALSE && pMgntInfo->bReg11nAdhoc == FALSE)
		bSupportNmode = FALSE;

	if(pMgntInfo->bReg11nAdhoc || pMgntInfo->bRegVht24g)
		bSupportNmode = TRUE;
	
	//Set dot11currentchannelnumber--------------
	pMgntInfo->dot11CurrentChannelNumber = pMgntInfo->Regdot11ChannelNumber;
	Adapter->HalFunc.SetWirelessModeHandler(Adapter, SetupStarWirelessMode(Adapter, bSupportNmode));

	//
	// 060207, rcnjko:
	// For 11d case, we are here only if we had acquired valid country IE
	// (see also JoinRequest() for details), 
	//
	pMgntInfo->dot11CurrentChannelNumber = ToLegalChannel(Adapter, pMgntInfo->dot11CurrentChannelNumber);

	//------------------------------------------

	// Add by hpfan to prevent infrastructure and adhoc mode from connection at the same time
	PlatformCancelTimer( Adapter, &pMgntInfo->AuthTimer );

	pMgntInfo->CurrentBssWirelessMode = pMgntInfo->dot11CurrentWirelessMode; // Keep wireless mode of IBSS to start. 2005.02.17, by rcnjko.

	// 2008.06.12, by Lanhsin. 
	pMgntInfo->bHalfWiirelessN24GMode = FALSE;

	RT_TRACE(COMP_SEC , DBG_LOUD , ("====>SWRxDecryptFlag = %d \n",pMgntInfo->SecurityInfo.SWRxDecryptFlag) );

	if(!pMgntInfo->SecurityInfo.SWRxDecryptFlag)
		Adapter->HalFunc.EnableHWSecCfgHandler(Adapter);
	else
	       Adapter->HalFunc.DisableHWSecCfgHandler(Adapter);

	if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm!= RT_ENC_ALG_NO_CIPHER)
		pMgntInfo->mCap |= cPrivacy;
	else
		pMgntInfo->mCap &= ~cPrivacy;

	// Set QoS mode. Added by Annie, 2005-11-09.
	if(pMgntInfo->pStaQos->QosCapability != QOS_DISABLE)
	{
		pMgntInfo->pStaQos->CurrentQosMode = QOS_WMM;
		pMgntInfo->pStaQos->QBssWirelessMode = pMgntInfo->dot11CurrentWirelessMode;
		pMgntInfo->mCap |= cQos;
		
		// Use default EDCA parameters
		SET_WMM_PARAM_ELE_AC_PARAMS(pMgntInfo->pStaQos->WMMParamEle, Adapter->STA_EDCA_PARAM);
	}
	else
	{
		pMgntInfo->pStaQos->CurrentQosMode = QOS_DISABLE;
		pMgntInfo->pStaQos->QBssWirelessMode = WIRELESS_MODE_UNKNOWN;
		pMgntInfo->mCap &= ~cQos;
	}

	// Set HT related capabilities
	if((pMgntInfo->pStaQos->CurrentQosMode & QOS_WMM))
	{
		HTUseDefaultSetting(Adapter);
		VHTUseDefaultSetting(Adapter);
	}
	else
	{
		pMgntInfo->pHTInfo->bCurrentHTSupport = FALSE;
		pMgntInfo->pVHTInfo->bCurrentVHTSupport = FALSE;
		pMgntInfo->dot11CurrentChannelBandWidth = CHANNEL_WIDTH_20;
		PlatformZeroMemory(pMgntInfo->dot11HTOperationalRateSet, 16);
	}

	HTSetCCKSupport(Adapter, TRUE, NULL);

	SelectRateSet( Adapter, SuppRateSets );

	if(	IS_WIRELESS_MODE_G(Adapter) || 
		(IS_WIRELESS_MODE_HT_24G(Adapter) && pMgntInfo->pHTInfo->bCurSuppCCK) )
	{
		ActUpdate_ERPInfo(Adapter, 0);
	}

	//Sherry Modified for dmsp mode 20110517 sync with 92C_92D 20110701
	//Vista/softAP not to set BW Mode here Sherry sync with  92C_92D_20110701
	{
		AP_SetBandWidth(Adapter);
	}

	// Reset TStream related, 070614, by rcnjko.
	QosResetAllTs(Adapter);
	
	// Reset CCX related setting.
	CCX_OnBssReset(Adapter);

	// For Turbo Mode resetting. Added by Roger, 2006.12.15.
	pMgntInfo->TurboChannelAccess.bEnabled=FALSE;
	MgntActSet_EnterTurboMode( Adapter, FALSE); 

	// Reset all association entry.
	AsocEntry_ResetAll(Adapter);

	pMgntInfo->bJoinInProgress = FALSE;
	RT_TRACE(COMP_MLME, DBG_TRACE, ("SetupStartIBSSInfo(): bJoinInProgress = %d\n", pMgntInfo->bJoinInProgress));
	RT_TRACE(COMP_MLME, DBG_LOUD, ("SetupStartIBSSInfo() <====\n"));
}	



//
// For 8187, we have to do the IOs with read operation in sync IO manner. 
// 2005.01.06, by rcnjko.
//
void
IbssStartRequest( 
	PADAPTER		Adapter
)
{
#if USE_WORKITEM
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);

	// Make sure at most one IbssStartRequestWorkItem is executed. 2005.03.04, by rcnjko.
	if( PlatformIsWorkItemScheduled(&(pMgntInfo->IbssStartRequestWorkItem)) == FALSE);
	{
		PlatformScheduleWorkItem( &(pMgntInfo->IbssStartRequestWorkItem) );
	}
#else
	IbssStartRequestCallback((PVOID)Adapter);
#endif
}



//
// Configure HW to start or join an IBSS.
// 2005.01.06, by rcnjko.
//
VOID
IbssStartRequestCallback(
	IN	PVOID	Context
	)
{
	PADAPTER		Adapter = (PADAPTER)Context;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	RT_OP_MODE		btLinkStatus = RT_OP_MODE_NO_LINK;
	BOOLEAN			bFilterOutNonAssociatedBSSID = FALSE;
	u1Byte			RetryLimit = HAL_RETRY_LIMIT_AP_ADHOC;
	BOOLEAN			bTypeIbss = FALSE;
	u1Byte			SwBeaconType = BEACON_SEND_AUTO_HW;
	HAL_AP_IBSS_INT_MODE	intMode = HAL_AP_IBSS_INT_IBSS;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  IbssStartRequestCallback()\n"));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_HW_BEACON_SUPPORT, (pu1Byte)&SwBeaconType);

	pMgntInfo->BeaconState = BEACON_STARTING;

	//--------------------------------------------------------------------------------
	// IBSS HW Configuration.
	//--------------------------------------------------------------------------------

	//
	// Clear MSR first, asked by Owen, 2005.01.27, by rcnjko.
	//
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)&btLinkStatus );
	pMgntInfo->dot11CurrentPreambleMode = PREAMBLE_AUTO;	// Added for short preamble peer. Added by Annie, 2006-06-08.


	//
	// Update Capability field related setting.
	//
	// <NOTE> To solve Win98 IRQL==PASIVE_LEVEL in timer callback function, 
	// we call ActUpdate_mCapInfo() here to make sure related IOs are perfomed in correct IRQL, 
	// that is, PASSIVE_LEVEL=>Syn IO for this case. 2005.03.15, by rcnjko.
	//
	ActUpdate_mCapInfo( Adapter, pMgntInfo->mCap );
	
	//
	// Set BSSID.
	//
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pMgntInfo->Bssid );


	//
	// Set Beacon related registers
	// <RJ_TODO> We shall update AtimWindow specified in Beacon of the IBSS if "every thing is ready." 
	//
	pMgntInfo->dot11AtimWindow = 2;	// ATIM Window (in unit of TU).
	Adapter->HalFunc.SetBeaconRelatedRegistersHandler(Adapter, (PVOID)(&pMgntInfo->OpMode), \
								pMgntInfo->dot11BeaconPeriod, pMgntInfo->dot11AtimWindow);

	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_AP_IBSS_INTERRUPT, (pu1Byte)&intMode);

	//
	// Set operation mode to IBSS.
	//
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode) );
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_RETRY_LIMIT, (pu1Byte)(&RetryLimit));
	
	if(!pMgntInfo->bIbssStarter)			
	{
		//Correct TSF value. Suggest by Scott. Added by tynli. 2009.12.01.
		bTypeIbss = TRUE;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CORRECT_TSF, (pu1Byte)(&bTypeIbss));
	}

	//
	// Start to send beacon.
	//
	if(!ACTING_AS_AP(Adapter))   //For RT_AP_TYPE_IBSS_EMULATED CCK Hang,sherry sync with 92C_92D, 20110701
	{
		if(SwBeaconType >= BEACON_SEND_AUTO_SW) // Using SW Beacon Timer		
		{ // S/W Beacon.
			PlatformSetTimer( Adapter, &pMgntInfo->SwBeaconTimer, 0 );
		}
		else
		{ // H/W Beacon.	
			ConstructBeaconFrame( Adapter );
			SendBeaconFrame(Adapter, BEACON_QUEUE);
		}
	}

	// If connected, set RCR CBSSID bit to prevent wrong TSF for 819x series..
	bFilterOutNonAssociatedBSSID = pMgntInfo->bMediaConnect; // Set to FALSE for our NIC is ibss starter by tynli.
	if(!pMgntInfo->bIbssStarter)
		bFilterOutNonAssociatedBSSID = TRUE;		
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));

	//
	// Set EDCA Parameter Set for QIBSS
	//
	if(pMgntInfo->pStaQos->CurrentQosMode != QOS_DISABLE)
	{
		// Update AC parameters to HW.
		u1Byte	eACI = 0;	
		for(eACI = 0; eACI < AC_MAX; eACI++)
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, GET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pMgntInfo->pStaQos->WMMParamEle, eACI) );
	}
	else
	{
		// Using 802.11 defualt settings.
		QosSetLegacyACParam(Adapter);
	}

	if(!pMgntInfo->bWiFiConfg && !Adapter->bInHctTest)
		Adapter->HalFunc.HalRxAggrHandler(Adapter,TRUE);

	//--------------------------------------------------------------------------------
	// IBSS SW Configuration.
	//--------------------------------------------------------------------------------
	

	MgntResetLinkStatusWatchdogState(Adapter);

	if( pMgntInfo->JoinAction == RT_JOIN_IBSS )
	{
		MgntIndicateMediaStatus( Adapter, RT_MEDIA_CONNECT, GENERAL_INDICATE );
		pMgntInfo->bJoinInProgress = FALSE;
		RT_TRACE(COMP_MLME, DBG_TRACE, ("IbssStartRequestCallback(): bJoinInProgress = %d\n", pMgntInfo->bJoinInProgress));
	}

	//
	// Reset bIbssCorordinator.
	// <NOTE>
	// 1. It will be update TRUE if we are winner of latest Beacon, FALSE otherwise. 
	// See also OnProbeReq() and NicIFHandleInterrupt() for details. 
	// 2. On 8187, we don't have interrupt mechanism to check if the we are winner of Beacon, 
	// so, bIbssCoordinator must be initialize to TRUE and no other place will change it.
	// 2005.12.14, by rcnjko.
	//
	pMgntInfo->bIbssCoordinator = TRUE;

	//
	// Set mDisable to FALSE after we had finished all configuration for the IBSS.
	// 2005.12.13, by rcnjko.
	//
	pMgntInfo->mDisable = FALSE;
	Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_START_TO_LINK); 
	RT_TRACE(COMP_MLME, DBG_TRACE, ("IbssStartRequestCallback() <====\n"));
}


//
//	Description:
//		Remove idle STA of the IBSS.
//
VOID
IbssAgeFunction(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_WLAN_STA	AsocEntry = pMgntInfo->AsocEntry;
	u8Byte			TimeDifference;
	u2Byte			idx, nBModeStaCnt = 0;
	u1Byte			nLegacyStaCnt = 0, n20MHzStaCnt = 0;
	u8Byte			CurrentTime = PlatformGetCurrentTime();

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

	for(idx = 0; idx < ASSOCIATE_ENTRY_NUM; idx++)
	{
		if(AsocEntry[idx].bUsed)
		{
			// TimeDifference is in ms.
			TimeDifference = PlatformDivision64((CurrentTime - AsocEntry[idx].LastActiveTime), 1000);

			// 20 second.
			if(TimeDifference > 20000)
			{
				RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "IbssAgeFunction(): timeout\n", AsocEntry[idx].MacAddr);
				MgntUpdateAsocInfo(Adapter, UpdateDeauthAddr, AsocEntry[idx].MacAddr, 6); 
				DrvIFIndicateDisassociation(Adapter, unspec_reason, AsocEntry[idx].MacAddr);

				// In Win7 NDISTest AdhocInterop, it verifies that we indicate dissassoc for
				// each IBSS member that sends deauth to us.
				// We indicate a dummy assoc event so that we could indicate a dissassoc
				// again later.
				// 2008.08.05, haich.
				//
				//if(Adapter->bInHctTest == TRUE)
#if 0				
				{
					static u1Byte DummySta[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

					//
					// Indicate dummy STA assocation event.
					//
					RT_TRACE(COMP_MLME, DBG_LOUD, ("IbssAgeFunction indicate dummy assoc\n"));
					MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, DummySta, 6);
					DrvIFIndicateAssociationStart(Adapter);
					DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_SUCCESS);
				}
#endif
				RT_TRACE(COMP_AP, DBG_LOUD, ("AsocEntry_RemoveStation\n"));

				AsocEntry_RemoveStation(Adapter, AsocEntry[idx].MacAddr);

				WAPI_SecFuncHandler(WAPI_RETURNONESTAINFO, Adapter, (PVOID)(AsocEntry[idx].MacAddr), WAPI_END);

			}

			// Set nBModeStaCnt, nLegacyStaCnt, n20MHzStaCnt.
			// We do not set these values before. 2010.11.02. Added by tynli.
			if(AsocEntry[idx].bAssociated)
			{
				if(AsocEntry[idx].WirelessMode == WIRELESS_MODE_B)
					nBModeStaCnt++;
				if(AsocEntry[idx].WirelessMode < WIRELESS_MODE_N_24G) 
					nLegacyStaCnt++;
				else if(AsocEntry[idx].BandWidth == CHANNEL_WIDTH_20)
					n20MHzStaCnt++;
			}
		}
	}

	if(AsocEntry_AnyStationAssociated(pMgntInfo)==FALSE)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("IbssAgedFuction: AsocEntry_AnyStationAssociated() FALSE\n"));
	//	DrvIFIndicateDisassociation(Adapter, unspec_reason);
	}

	// Disable protection mode and Enable short slot time if an B-mode STA joined.
	if(	IS_WIRELESS_MODE_G(Adapter)||
		(IS_WIRELESS_MODE_HT_24G(Adapter) && pMgntInfo->pHTInfo->bCurSuppCCK))
	{
		if(nBModeStaCnt == 0)
			ActUpdate_ProtectionMode(Adapter, FALSE);
		else
			ActUpdate_ProtectionMode(Adapter, TRUE);
	}

	// Update the Operation mode field in HT Info element.
	if(IS_WIRELESS_MODE_N(Adapter))
	{
		if(nLegacyStaCnt > 0)
		{
			pMgntInfo->pHTInfo->CurrentOpMode = HT_OPMODE_MIXED;
		}
		else
		{
			if(CHNL_RUN_ABOVE_40MHZ(pMgntInfo) && (n20MHzStaCnt > 0))
				pMgntInfo->pHTInfo->CurrentOpMode = HT_OPMODE_40MHZ_PROTECT;
			else
				pMgntInfo->pHTInfo->CurrentOpMode = HT_OPMODE_NO_PROTECT;
		}
	}

	//
	// This setup the RTS rate set for rate adaptive in firmware.
	// For the condition of the USE_PROTECTION bit is set in ERP IE,
	// we shall set the RTS rate to 11b rate, else set all basic rate by default.
	// Joseph, 20070131
	//
	if(	IS_WIRELESS_MODE_G(Adapter) ||
		(IS_WIRELESS_MODE_HT_24G(Adapter) && pMgntInfo->pHTInfo->bCurSuppCCK))
	{
		if(pMgntInfo->bUseProtection)
		{
			u1Byte CckRate[4] = { MGN_1M, MGN_2M, MGN_5_5M, MGN_11M };
			OCTET_STRING osCckRate;
			FillOctetString(osCckRate, CckRate, 4);
			FilterSupportRate(pMgntInfo->mBrates, &osCckRate, FALSE);
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_BASIC_RATE, (pu1Byte)&osCckRate);
		}
		else
		{
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BASIC_RATE, (pu1Byte)(&pMgntInfo->mBrates) );
		}
	}
	else
	{
		// Periodically update RTS rate to prevent the RTS rate > init data rate. 2010.11.24. by tynli.
		Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_INIT_RTS_RATE, (pu1Byte)(&Adapter));
	}

}


void
MgntDisconnectAP(
	PADAPTER		Adapter,
	u1Byte			asRsn
) 
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;	
	BOOLEAN			bFilterOutNonAssociatedBSSID = FALSE;
	BOOLEAN		bConnected =FALSE;

	{
		bFilterOutNonAssociatedBSSID = FALSE;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
	}
	Adapter->HalFunc.AllowErrorPacketHandler(Adapter, FALSE, TRUE);
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_ENABLE_TXOK_INTERRUPT, (pu1Byte)&bConnected);
		
	MultiChannelDisconnectClient(Adapter, TRUE);

	// 2004.10.11, by rcnjko.
	MlmeDisassociateRequest( Adapter, pMgntInfo->Bssid, asRsn );

	// In WPA WPA2 need to Clear all key ... because new key will set after new handshaking.
	// We shall only clear the keys after sedning Mgnt frames because the frames may still be protected by these keys.
	// By Bruce, 2015-01-10.
	{
		BOOLEAN		bAPSuportCCKM = FALSE;
		BOOLEAN		bCCX8021xenable = FALSE;

		CCX_QueryCCKMSupport(Adapter, &bCCX8021xenable, &bAPSuportCCKM);
		
		if(   pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch ||
			(bAPSuportCCKM && bCCX8021xenable) )  // In CCKM mode will Clear key
		{
			SecClearAllKeys(Adapter);
			RT_TRACE(COMP_SEC, DBG_LOUD,("======>CCKM clear key..."));
		}
	}

	pMgntInfo->mAssoc = FALSE;
	pMgntInfo->AsocTimestamp = 0;
}




BOOLEAN
MgntDisconnect(
	PADAPTER		Adapter,
	u1Byte			asRsn
) 
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnect()====>\n"));
	pMgntInfo->bDisconnectInProgress = TRUE;
	pMgntInfo->RequestFromUplayer = FALSE;

	//
	// Schedule an workitem to wake up for ps mode, 070109, by rcnjko.
	//
	if(pMgntInfo->mPss != eAwake)
	{
		// 
		// Using AwkaeTimer to prevent mismatch ps state.
		// In the timer the state will be changed according to the RF is being awoke or not. By Bruce, 2007-10-31. 
		//
		// PlatformScheduleWorkItem( &(pMgntInfo->AwakeWorkItem) );
		PlatformSetTimer( Adapter, &(pMgntInfo->AwakeTimer), 0 );
	}

	// Follow 8180 AP mode, 2005.05.30, by rcnjko.
	if(ACTING_AS_AP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnect() ===> AP_DisassociateAllStation\n"));
		AP_DisassociateAllStation(Adapter, unspec_reason);
		return TRUE;
	}	


	//stop beacon when ibssstarter and no station joins
	if( pMgntInfo->mIbss )
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnect() ===> MgntDisconnectIBSS\n"));

		if (WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0))
		{
			WAPI_SecFuncHandler(WAPI_RETURNALLCAMENTRY,Adapter, WAPI_END);
			SecClearAllKeys(Adapter);
		}
		MgntDisconnectIBSS( Adapter );
	}

	// In adhoc mode, update beacon frame.
	if( pMgntInfo->bMediaConnect )
	{
		BOOLEAN		bConnected =FALSE;
		
		if( pMgntInfo->mAssoc )
		{
			// We should leave LPS mode first. 2011.03.23. by tynli.
			if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS)
			{
				LeisurePSLeave(Adapter, LPS_DISABLE_MGNT_DISCONNECT);
			}
		
			// We clear key here instead of MgntDisconnectAP() because that  
			// MgntActSet_802_11_DISASSOCIATE() is an interface called by OS, 
			// e.g. OID_802_11_DISASSOCIATE in Windows while as MgntDisconnectAP() is 
			// used to handle disassociation related things to AP, e.g. send Disassoc
			// frame to AP.  2005.01.27, by rcnjko.

			WAPI_SecFuncHandler(WAPI_RETURNALLCAMENTRY,Adapter, WAPI_END);

			SecClearAllKeys(Adapter); 

			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntDisconnect() ===> MgntDisconnectAP\n"));
			MgntDisconnectAP(Adapter, asRsn);
		}
		
		Adapter->HalFunc.AllowErrorPacketHandler(Adapter, FALSE, TRUE);
		Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_ENABLE_TXOK_INTERRUPT, (pu1Byte)&bConnected);
		

		// Inidicate Disconnect, 2005.02.23, by rcnjko.
		MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE);	
	}
	pMgntInfo->bDisconnectInProgress = FALSE;

	FunctionOut(COMP_MLME);
	return TRUE;
}



void
MgntDisconnectIBSS(
	PADAPTER		Adapter
) 
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u1Byte			i;
	BOOLEAN			bFilterOutNonAssociatedBSSID = FALSE;
	PRT_WLAN_STA	AsocEntry = pMgntInfo->AsocEntry;
	HAL_AP_IBSS_INT_MODE	intMode = HAL_AP_IBSS_INT_DISABLE;
	FunctionIn(COMP_MLME);

	pMgntInfo->mIbss = FALSE;
	pMgntInfo->bIbssStarter = FALSE;
	pMgntInfo->OpMode = RT_OP_MODE_NO_LINK;

	// Disable Beacon early interrupt mask.
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_AP_IBSS_INTERRUPT, (pu1Byte)&intMode);

	//Stop Beacon.
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pMgntInfo->Bssid );
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode) );

	// Stop SW Beacon.
	MgntStopBeacon(Adapter);

	//
	// Send every station in the ad hoc network a de-auth message and clear their
	// association states.
	//
	for(i = 0;i < ASSOCIATE_ENTRY_NUM; i++)
	{
		if(AsocEntry[i].bUsed && AsocEntry[i].bAssociated)
		{
			SendDeauthentication(Adapter, AsocEntry[i].MacAddr, deauth_lv_ss);
			AsocEntry[i].bAssociated = FALSE;

			MgntUpdateAsocInfo(Adapter, UpdateDeauthAddr, AsocEntry[i].MacAddr, 6); 
			DrvIFIndicateDisassociation(Adapter, unspec_reason, AsocEntry[i].MacAddr);

			RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "Sending Deauth to:", AsocEntry[i].MacAddr);
		}
	}

	// If disconnect, clear RCR CBSSID bit
	bFilterOutNonAssociatedBSSID = FALSE;
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));

//	DrvIFIndicateDisassociation(Adapter, unspec_reason);
	MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );

	AsocEntry_ResetAll(Adapter);

	// We should return IPS state when IBSS disconnect. Added by tynli. 2010.04.09.
	// 20100414 Joseph: Add IPS return reason.
	IPSReturn(Adapter, IPS_DISABLE_DEF_OPMODE);

}

VOID
MgntIndicateMediaDisconnectStatus(
			PADAPTER			Adapter
)
{
	BOOLEAN		bSW20BW = FALSE;
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);	

	pMgntInfo->LinkDetectInfo.IndicateDisconnectCnt++;//cosa add for debug
	pMgntInfo->bMediaConnect = FALSE;
	pMgntInfo->DisconnectedSlotCount=0;


	if(!ACTING_AS_AP(Adapter))
	{
		if(!pMgntInfo->bIbssStarter)
		{
			if(IsDefaultAdapter(Adapter))
			{
				RT_TRACE(COMP_MLME,DBG_LOUD,("Now is Default Adapter.\n"));				

				//Fix can not ping wapi ap with defaultport when stop hostednetwork, move it to here from below by ylb 20110915
				WAPI_SecFuncHandler(WAPI_RETURNALLSTAINFO,pDefaultAdapter, WAPI_END);
				WAPI_SecFuncHandler(WAPI_RETURNALLCAMENTRY,pDefaultAdapter, WAPI_END);
			
			}
			else
			{
				RT_TRACE(COMP_MLME,DBG_LOUD,("Now is Extension Adapter.\n"));
					
				if(pDefaultAdapter->MgntInfo.bMediaConnect== FALSE)
				{
					RT_TRACE(COMP_MLME,DBG_LOUD,("Has no Def   started.->20MHz\n"));
					bSW20BW = TRUE;
				}
			}

			if(bSW20BW && !MgntScanInProgress(pMgntInfo))
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("ChangeWirelessModeHandler\n"));
			
				HalChangeWirelessMode(Adapter,pMgntInfo->dot11CurrentChannelNumber);
				CHNL_SetBwChnl(	Adapter, 
									pMgntInfo->dot11CurrentChannelNumber,
									CHANNEL_WIDTH_20,
									EXTCHNL_OFFSET_NO_EXT
									);
			}
			pMgntInfo->pHTInfo->bCurrentHTSupport = FALSE;
			pMgntInfo->pVHTInfo->bCurrentVHTSupport = FALSE;
			pMgntInfo->pStaQos->CurrentQosMode = QOS_DISABLE;
			pMgntInfo->pStaQos->QBssWirelessMode = WIRELESS_MODE_UNKNOWN;
		}
		// Do this to reset TS data structure.
		RemoveAllTS(Adapter);
	}
	
	//
	// <Roger_Notes> We disable FW DIG while medium is disconnected. 
	// We currently do NOT update OpMode to allow roaming case in watchdog.
	// 2008.11.27.
	// Currently we do NOT need to enable DIG(i.e., it will reference MSR automatically).
	//
	if(!pMgntInfo->bIbssStarter)
	{
		RT_OP_MODE	OpStatus = RT_OP_MODE_NO_LINK;	// update HW operation mode here.		
		Adapter->HalFunc.SetHwRegHandler( pDefaultAdapter, HW_VAR_MEDIA_STATUS, (pu1Byte)&OpStatus);
	}
	Dot11d_Reset(Adapter);

}


void 
MgntIndicateMediaStatus(
	PADAPTER			Adapter,
	RT_MEDIA_STATUS		mstatus,
	u1Byte				Indicatemode
)
{
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);
	BOOLEAN		bIndicate = FALSE;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);	
	BOOLEAN		bFilterOutNonAssociatedBSSID = FALSE;
	HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T				pDM_Odm = &pHalData->DM_OutSrc;
	
	// For the first time indicating MEDIA_CONNECT.
	if(mstatus==RT_MEDIA_CONNECT && pDefaultAdapter->ResetProgress==RESET_TYPE_NORESET)//??
	{
		MgntResetLinkStatusWatchdogState(Adapter);
	}

	// <RJ_EXPR> We don't know when this case will be triggered and it SHOULD NOT happend. 
	// So, we just skip AP mode's disconnected event indication here as a workaround and 
	// add assertion for debug purpose. 2005.08.16, by rcnjko.
	if( ACTING_AS_AP(pDefaultAdapter) && mstatus==RT_MEDIA_DISCONNECT &&
		!(pMgntInfo->RfOffReason & (RF_CHANGE_BY_SW|RF_CHANGE_BY_HW)) )
	{
		RT_ASSERT(TRUE, ("MgntIndicateMediaStatus(): AP mode should not indicate disconnected event!!!\n"));
		return;
	}	

	if( mstatus == RT_MEDIA_CONNECT )
	{
		if( pMgntInfo->bMediaConnect == FALSE )
		{
			pMgntInfo->bMediaConnect = TRUE;
			bIndicate = TRUE;

			// Only Ibss starter need to set check bssid=TRUE again.
			// (fixed for DTM for beacon can't tx issue???)
			// All IC need to do this?? or only 92C need to do??
			if(pMgntInfo->bIbssStarter)
			{
				bFilterOutNonAssociatedBSSID = TRUE;		
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
			}

			// H2C cmd for BT information. 2011.02.10. by tynli.
			if(pMgntInfo->mIbss)
			{
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_WLAN_INFO, (pu1Byte)(&mstatus)); 
			}
			
			RT_TRACE(COMP_MLME, DBG_WARNING, ("MEDIA_STATUS_CONNECT !!\n"));
		}
		else
		{
			RT_TRACE(COMP_MLME, DBG_WARNING, ("MEDIA_STATUS_KEEP_CONNECT !!\n"));
		}
	}
	else if( mstatus == RT_MEDIA_DISCONNECT )
	{

		if( pMgntInfo->bMediaConnect == TRUE )
		{
			bIndicate = TRUE;
			MgntIndicateMediaDisconnectStatus(Adapter);
			RT_TRACE(COMP_MLME, DBG_WARNING, ("MEDIA_STATUS_DISCONNECT !!\n"));
			
			if(pMgntInfo->mAssoc && pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
			{
				u2Byte	JaguarPatch = (pMgntInfo->IOTPeer << 8) | mstatus;
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_JAGUAR_PATCH, (pu1Byte)(&JaguarPatch)); 

				if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS)
				{
					LeisurePSLeave(Adapter, LPS_DISABLE_MGNT_INDI_DISCONNECT);
					Hal_ClearRsvdCtrl(Adapter, RSVDPAGE_TYPE_LPS);
					Adapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_H2C_FW_JOINBSSRPT, (pu1Byte)(&mstatus)); 
				}

				
				// Deregister all MacIDs created by this adpater ------------
				MacIdDeregisterMacIdForInfraClient(Adapter, pMgntInfo->mMacId);
				MacIdDeregisterSpecificMacId(Adapter, pMgntInfo->mcastMacId);
				// --------------------------------------------------
			}
			Adapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_H2C_WLAN_INFO, (pu1Byte)(&mstatus)); 
		}
		else
		{
			RT_TRACE(COMP_MLME, DBG_WARNING, ("MEDIA_STATUS_KEEP_DISCONNECT !!\n"));
		}

		//Reset BCN related function. Suggested by TimChen. Added by tynli. 2009.12.03.
		//This check only apply to two port, consider port 2 ap mode. 
		//CHECK_ADAPTER_SENDS_BEACON return zero in one port design and just reset tsf.
		// 
		if(IsAPModeExist(Adapter))	// IS_ADAPTER_SENDS_BEACON(Adapter)
		{
			Adapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_DUAL_TSF_RST, 0);
		}
		// If we disconnet with AP or NIC, clear RCR CBSSID bit.

		{
			bFilterOutNonAssociatedBSSID = FALSE;
			Adapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
		}
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("Warning: Unknown link status !!\n"));
	}
	
	if((bIndicate && Indicatemode != FORCE_NO_INDICATE) || (Indicatemode==FORCE_INDICATE))
	{
		if(mstatus==RT_MEDIA_CONNECT)
			Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_LINK);
		else
			Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_NO_LINK);
		PlatformIndicateMediaStatus( Adapter,  mstatus);
	}

#if (MULTICHANNEL_SUPPORT == 1)
{
	MultiChannelStatusWatchdog(Adapter);
}
#endif

}



void
AsocTimeout(
	PRT_TIMER		pTimer
)
{
	PADAPTER		Adapter=(PADAPTER)pTimer->Adapter;
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING	nullpdu;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  AsocTimeout()\n"));
	//Adapter->MgntInfo.bJoinInProgress = FALSE;

	nullpdu.Length = 0;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}
	
	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

#if (P2P_SUPPORT == 1)
	if(P2P_ADAPTER_OS_SUPPORT_P2P(Adapter) && P2P_ACTING_IS_CLIENT(Adapter))
	{
		McDynamicMachanismSet(Adapter, MC_DM_INIT_GAIN_BOOST_END, NULL, 0);
		RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] AsocTimeout...resume DIG\n"));
	}
#endif
    
	//ShuChen TODO: Retry association request 
	
	switch( pMgntInfo->State_AsocService )
	{
		case STATE_Wait_Asoc_Response:
			RT_TRACE(COMP_MLME, DBG_LOUD, ("AsocTimeout(): STATE_Wait_Asoc_Response.\n"));
			MlmeAssociateConfirm(Adapter, NULL, &nullpdu, FALSE, MlmeStatus_timeout);
			break;
		case STATE_Wait_Reasoc_Response:
			RT_TRACE(COMP_MLME, DBG_LOUD, ("AsocTimeout(): STATE_Wait_Reasoc_Response.\n"));
			MlmeAssociateConfirm(Adapter, NULL, &nullpdu, TRUE, MlmeStatus_timeout);
			break;
		default:
			break;
	}

	RT_TRACE(COMP_MLME, DBG_LOUD, ("AsocTimeout() <====\n"));
}



BOOLEAN
MlmeAssociateRequest(
	PADAPTER		Adapter,
	pu1Byte			asocStaAddr,
	u4Byte			asocTmot,
	u2Byte			asCap,
	u2Byte			asListenInterval,
	BOOLEAN			Reassociate	
)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;	
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  MlmeAssociateRequest()\n"));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}	

	DrvIFIndicateAssociationStart(Adapter);

	if (CCX_MlmeAssociateRequest(Adapter))
		return	TRUE;

	P2PMlmeAssociateRequest(Adapter, asocStaAddr, asocTmot, asCap, asListenInterval, Reassociate);

	if( Reassociate ){
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAssociateRequest(): Send Reassociate Req\n"));

		SendReassociateReq( Adapter, asocStaAddr, asCap, asListenInterval, pMgntInfo->APaddrbeforeRoaming );
		pMgntInfo->State_AsocService = STATE_Wait_Reasoc_Response;
	}
	else{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAssociateRequest(): Send Associate Req\n"));

		SendAssociateReq( Adapter, asocStaAddr, asCap, asListenInterval);
		pMgntInfo->State_AsocService = STATE_Wait_Asoc_Response;
	}

	PlatformSetTimer( Adapter, &pMgntInfo->AsocTimer, asocTmot );

	RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAssociateRequest(): return TRUE. <====\n"));
	return TRUE;
}


//
// Description:
//	Confirm the assoication status.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//		If this function is not triggered by rx packet, this field is assigned as NULL.
//	[in] posMpdu - 
//		The poniter of the current packet to the 802.11 header.
//		If this function is not triggered by rx packet, this field is assigned as NULL.
//	[in] bReassociate -
//		TRUE if this packet is Reassociation response packet. FALSE if this packet is association response packet.
//	[in] result -
//		The status of this confirm process.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	By determining the result status code, handle the assoication process for client mode.
//
RT_STATUS
MlmeAssociateConfirm(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu,
	IN	BOOLEAN			bReassociate,
	IN	u2Byte			result
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u1Byte			asocStaAddr[6];
	u2Byte			myCap;
	u2Byte			asListenInterval = pMgntInfo->ListenInterval;
	u1Byte			ErrorPoint = AssocSuccess;
	u1Byte			mstatus = RT_MEDIA_CONNECT;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("result: %d. Current Channel %d\n", result,pMgntInfo->dot11CurrentChannelNumber));

	// Cancel AsocTimer
	PlatformCancelTimer( Adapter, &pMgntInfo->AsocTimer );

	pMgntInfo->State_AsocService = STATE_Asoc_Idle;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return RT_STATUS_FAILURE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return RT_STATUS_FAILURE;
	}	

	//
	// <Burce_Note> To simplify handling association fail, I use ErrorPoint to indicate how to 
	//	handle the assocaition failed process later. By Bruce, 2008-05-28.
	//
	ErrorPoint = AssocUnspecFail;
	
	if(result == MlmeStatus_success)
	{
		if(posMpdu->Length > 0)
		{
			if(Frame_AssocStatusCode(*posMpdu) == StatusCode_success)
			{
				u2Byte			JaguarPatch;
				RT_STATUS		bMlmeOkState;

				ErrorPoint = AssocSuccess;
				bMlmeOkState = MlmeOnAssocOK(Adapter, pRfd, posMpdu, bReassociate);

#if (P2P_SUPPORT == 1)
				if(P2P_ADAPTER_OS_SUPPORT_P2P(Adapter) && P2P_ACTING_IS_CLIENT(Adapter))
				{
					u2Byte	boostDelaySec = ASSOC_HANDSHAKE_DHCP_DELAY_SEC;
					
					McDynamicMachanismSet(Adapter, MC_DM_INIT_GAIN_BOOST_END_DELAY_SEC, &boostDelaySec, sizeof(u2Byte));
					RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] Asoc SUCCESS! resume DIG\n"));
				}
#endif
			
				if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS)
				{	//For FW LPS. by tynli. To tell firmware we have connected to an AP. For 92SE/CE power save v2.
					Hal_SetRsvdCtrl(Adapter, RSVDPAGE_TYPE_LPS);
				}
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_WLAN_INFO, (pu1Byte)(&mstatus));

				JaguarPatch = (pMgntInfo->IOTPeer << 8) | mstatus;
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_JAGUAR_PATCH, (pu1Byte)(&JaguarPatch)); 

				// New Connection is comming, we need to confirm new channel connect and start
				// to judge if we need to start multi channel switch.
				MultiChannelAssociateConfirm(Adapter, TRUE);
				
				// If return Fail , try to complete State On Reset Request by bJoinInProgress flag !!
				if( !(bMlmeOkState == RT_STATUS_FAILURE && MgntResetOrPnPInProgress(Adapter)) )
				{
					pMgntInfo->bJoinInProgress = FALSE;
				}

				// Download rsvd page after all required bitmap is set
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_FW_JOINBSSRPT, (pu1Byte)(&mstatus));
				
				RT_TRACE(COMP_MLME, DBG_TRACE, ("MlmeAssociateConfrim(): - success, bJoinInProgress = %d\n", pMgntInfo->bJoinInProgress));
				RT_TRACE(COMP_MLME, DBG_LOUD, ("return TRUE. <====\n"));

				return RT_STATUS_SUCCESS;
			}
			else
			{
				RT_TRACE(COMP_MLME, DBG_WARNING, ("[WARNING] AssocFail, StatusCode = %d\n", Frame_AssocStatusCode(*posMpdu)));
			}
		}
	}
	else if(pMgntInfo->AsocRetryCount < RT_ASOC_RETRY_LIMIT &&
		!MgntIsTimeOutForIndication(pMgntInfo))
	{ // Timeout...
		ErrorPoint = AssocRetry ;
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));
		return RT_STATUS_FAILURE;
	}	

	switch(ErrorPoint)
	{
		case AssocRetry:
			{ // Try again
				pMgntInfo->AsocRetryCount ++;
				RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAssociateConfirm(): failed(%d) => Asoc retry for AsocRetryCount(%d) < RT_ASOC_RETRY_LIMIT(%d)\n", result, pMgntInfo->AsocRetryCount, RT_ASOC_RETRY_LIMIT));

				CopyMem( asocStaAddr, pMgntInfo->Bssid, 6 );
				myCap = pMgntInfo->mCap;
				MlmeAssociateRequest( Adapter, asocStaAddr, ASSOC_REQ_TIMEOUT,	myCap,	asListenInterval, bReassociate );
			}
			break;

		case AssocUnspecFail:
			{ // Try to connect to the other.
				RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAssociateConfirm(): failed(%d) => No Retry! Try to connect to the other one BSS!!\n", result));

				// Remember the rejected AP, if another AP in the same ESS is better,
				// pick the better one in further connection. 2006.11.21, by shien chang.
				MgntAddRejectedAsocAP(Adapter, pMgntInfo->Bssid);

				// Postprocessing for completion of a association request, 2006,10.17, by shien chang.
				DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_FAILURE);

				Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_NO_LINK); 

				if(!MgntConnectOtherBss(Adapter))
				{ // Connect to the Other BSS failed.

					if(MgntResetOrPnPInProgress(Adapter))
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 4\n"));
						return RT_STATUS_FAILURE;
					}				
					
					DrvIFIndicateAssociationStart(Adapter);
					DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_FAILURE);
					
					if(MgntRoamingInProgress(pMgntInfo)) // Roaming.
						MgntRoamComplete(Adapter, MlmeStatus_invalid);
					else // First connection.
						DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);

					pMgntInfo->bJoinInProgress = FALSE;
					MultiChannelAssociateConfirm(Adapter, FALSE);
				}
				
	//			pMgntInfo->bJoinInProgress = FALSE;

				RT_TRACE(COMP_MLME, DBG_TRACE, ("MlmeAssociateConfirm(): -fail 0, bJoinInProgress = %d\n", pMgntInfo->bJoinInProgress));
			}
			break;
			
		default:
			break;
	}
	RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAssociateConfirm(): return FALSE. <====\n"));
	return RT_STATUS_SUCCESS;
}




void
AuthTimeout(
	PRT_TIMER		pTimer
)
{
	PADAPTER		Adapter=(PADAPTER)pTimer->Adapter;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  AuthTimeout()State_AuthReqService %d\n", Adapter->MgntInfo.State_AuthReqService));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}	
	
	switch( Adapter->MgntInfo.State_AuthReqService )
	{
		case STATE_Wait_Auth_Seq_2:
		case STATE_Wait_Auth_Seq_4:
			
	//		StaState(Adapter, auSta, StationState_not_auth);
			Adapter->MgntInfo.State_AuthReqService = STATE_Auth_Req_Idle;
			MlmeAuthenticateRequest_Confirm(Adapter, MlmeStatus_timeout);
			break;
		default:
			break;
	}

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<====AuthTimeout() State_AuthReqService %d\n", Adapter->MgntInfo.State_AuthReqService));
}




BOOLEAN
MlmeAuthenticateRequest(
	PADAPTER		Adapter,
	pu1Byte			auStaAddr,
	u1Byte			auAlg,
	u4Byte			auTmot
)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	u1Byte			AuthSeq;
	u1Byte			AuthStatusCode;
	OCTET_STRING	AuthChallengetext;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  MlmeAuthenticateRequest()\n"));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}	

	AuthSeq = 1;
	AuthStatusCode = StatusCode_success;

	//Since this is always the 1st authentication frame
	AuthChallengetext.Length = 0;

	//Send authentication frame
	SendAuthenticatePacket( Adapter, auStaAddr, auAlg, AuthSeq, AuthStatusCode, AuthChallengetext );

	PlatformSetTimer( Adapter, &pMgntInfo->AuthTimer, auTmot );

	//Set authentication state
	pMgntInfo->State_AuthReqService = STATE_Wait_Auth_Seq_2;
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<====MlmeAuthenticateRequest(): return TRUE. State_AuthReqService %d\n", pMgntInfo->State_AuthReqService));
	return TRUE;
}


BOOLEAN
MlmeAuthenticateRequest_Confirm(
	PADAPTER		Adapter,
	u2Byte			result
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	u1Byte			auStaAddr[6];
	u1Byte			asocStaAddr[6];
	u2Byte			myCap;
	u2Byte			asListenInterval = pMgntInfo->ListenInterval;
	u1Byte			auAlg;
	BOOLEAN			bReassociation = FALSE;
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  MlmeAuthenticateRequest_Confirm(): result: %d., opMode: %u\n", result, pMgntInfo->OpMode));

	// Cancel AuthTimer
	PlatformCancelTimer( Adapter, &pMgntInfo->AuthTimer );

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}

	if( result != MlmeStatus_success)
	{
		//Retry MlmeAuthenticateRequest
		if( (pMgntInfo->AuthRetryCount++ < RT_AUTH_RETRY_LIMIT) &&
			result != MlmeStatus_invalid &&
			!MgntIsTimeOutForIndication(pMgntInfo))
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAuthenticateRequest_Confirm(): Auth retry for AuthRetryCount(%d) < RT_AUTH_RETRY_LIMIT(%d)\n", pMgntInfo->AuthRetryCount, RT_AUTH_RETRY_LIMIT));

			// Auto switch authentication mode if we are in WEP encryption. 2005.03.08, by rcnjko.
			if( pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeAutoSwitch &&
				(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 || 
				 pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104) ) 
			{
				// Change auth alg.
				RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAuthenticateRequest_Confirm(): Change auth alg\n"));
				if(pMgntInfo->AuthReq_auAlg == OPEN_SYSTEM)
				{
					pMgntInfo->AuthReq_auAlg = SHARED_KEY;
				}
				else
				{
					pMgntInfo->AuthReq_auAlg = OPEN_SYSTEM;
				}
			}

			auAlg = pMgntInfo->AuthReq_auAlg;
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAuthenticateRequest_Confirm(): auAlg(%d)\n", auAlg));

			CopyMem( auStaAddr, pMgntInfo->Bssid, 6 );
			MlmeAuthenticateRequest( Adapter, auStaAddr, auAlg, AUTH_REQ_TIMEOUT );
			pMgntInfo->AuthStatus = AUTH_STATUS_IN_PROGRESS;
		}
		else
		{	
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAuthenticateRequest_Confirm(): No more Auth retry for AuthRetryCount(%d) >= RT_AUTH_RETRY_LIMIT(%d)\n", pMgntInfo->AuthRetryCount, RT_AUTH_RETRY_LIMIT));
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAuthenticateRequest_Confirm(): Reset roaming and AuthStatus!\n"));

#if (P2P_SUPPORT == 1)
			if(P2P_ADAPTER_OS_SUPPORT_P2P(Adapter) && P2P_ACTING_IS_CLIENT(Adapter))
			{				
				McDynamicMachanismSet(Adapter, MC_DM_INIT_GAIN_BOOST_END, NULL, 0);
				RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] Auth Timeout...resume DIG\n"));
			}
#endif
			
			// Remember the rejected AP, if another AP in the same ESS is better,
			// pick the better one in further connection. 2006.11.21, by shien chang.
			MgntAddRejectedAsocAP(Adapter, pMgntInfo->Bssid);

			pMgntInfo->AuthStatus = AUTH_STATUS_FAILED;

			RT_TRACE(COMP_MLME, DBG_TRACE, ("MlemAuthenticateRequest_Confirm(): bJoinInProgress = %d\n", pMgntInfo->bJoinInProgress));

			Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_NO_LINK);

			// Rewritten by Bruce, 2008-05-29.
			if(!MgntConnectOtherBss(Adapter))
			{ // Connect the Other BSS failed.

				if(MgntResetOrPnPInProgress(Adapter))
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 4\n"));
					return FALSE;
				}			
				
				DrvIFIndicateAssociationStart(Adapter);
				DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_FAILURE);
					
				if(MgntRoamingInProgress(pMgntInfo)) // Roaming.
					MgntRoamComplete(Adapter, MlmeStatus_invalid);
				else // First connection.
					DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);

				pMgntInfo->bJoinInProgress = FALSE;
				MultiChannelAssociateConfirm(Adapter, FALSE);
			}	
		}
		return FALSE;
	}


	if( pMgntInfo->OpMode == RT_OP_MODE_IBSS )
	{
		IbssStartRequest( Adapter );
		MgntIndicateMediaStatus( Adapter, RT_MEDIA_CONNECT, GENERAL_INDICATE );
	}
	else if( pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
	{
		CopyMem( asocStaAddr, pMgntInfo->Bssid, 6 );

		// cESS
		myCap = cESS; // Revised by rcnjko, 2005.01.06.

		// cPrivacy
		if( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm != RT_ENC_ALG_NO_CIPHER ){
			myCap |= cPrivacy;
		}

		// cShortPreamble, 2005.01.06, by rcnjko.
		if( (pMgntInfo->mCap & cShortPreamble) )
		{ // AP supports short and long preamble mode.
			myCap |= cShortPreamble; 
		}
		
		// cQoS
		if (pMgntInfo->pStaQos->CurrentQosMode)
		{
			myCap |= cQos;
		}
		
		// cShortSlotTime
		if(pMgntInfo->dot11CurrentWirelessMode != WIRELESS_MODE_B)
		{
			myCap |= cShortSlotTime;
		}

		if( pMgntInfo->RoamingState == ROAMINGSTATE_AUTHENTICATION &&
			!Adapter->bInHctTest) // not to send reassociation if this is the association after wake up since we have sent disassociation to the AP before sleep , by haich.
		{
			pMgntInfo->RoamingState = ROAMINGSTATE_REASSOCIATION;
			bReassociation = TRUE;
		}
		else
		{
			bReassociation = FALSE;
		}

		//
		// cDelayedBA & cImmediateBA
		// This part shall be add later.
		//
		
		//For 11h,Spectrum Management.
		if( (pMgntInfo->mCap & cSpectrumMgnt) )
		{ // AP supports short and long preamble mode.
			myCap |= cSpectrumMgnt; 
		}

		MlmeAssociateRequest( Adapter, asocStaAddr, ASSOC_REQ_TIMEOUT, myCap,  asListenInterval, bReassociation);
	}
	else{
		RT_TRACE(COMP_MLME, DBG_WARNING,("MlmeAuthenticateRequest_Confirm(): ERROR!!\n"));
	}

	pMgntInfo->AuthStatus = AUTH_STATUS_SUCCESSFUL;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("MlmeAuthenticateRequest_Confirm(): return TRUE. <====\n"));
	return TRUE;
}













//(1)Adhoc mode: Start an Ibss
//(2)Infrastructure mode: change 	pMgntInfo->state_Synchronization_Sta = STATE_no_BSS;

VOID
JoinTimeout(
	PRT_TIMER		pTimer
	)
{
	PADAPTER		Adapter=(PADAPTER)pTimer->Adapter;
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	RT_TRACE(COMP_MLME, DBG_LOUD,("====>  JoinTimeout(): Join timeout!\n"));

	// Synchronize the timer status, 2006.11.14, by shien chang.
	pTimer->Status = RT_TIMER_STATUS_INITIALIZED;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

	//pMgntInfo->bJoinInProgress = FALSE;
	pMgntInfo->state_Synchronization_Sta = STATE_No_Bss;

	RT_TRACE(COMP_MLME, DBG_TRACE, ("JoinTimeout(): bJoinInProgress = %d\n", pMgntInfo->bJoinInProgress));

	PlatformCancelTimer(Adapter, &pMgntInfo->JoinProbeReqTimer );	
			
	// Remember the rejected AP, if another AP in the same ESS is better,
	// pick the better one in further connection. 2006.11.21, by shien chang.
	MgntAddRejectedAsocAP(Adapter, pMgntInfo->Bssid);
			
	if(MgntLinkRetry(Adapter, FALSE))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD,("JoinTimeout(): try to link again\n"));
		return;
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_LOUD,("JoinTimeout(): can't perform link retry any more => reset roaming and Synchronization_Sta_State!\n"));
		
		if(MgntRoamingInProgress(pMgntInfo))
		{
			// Roaming retry
			if(MgntRoamRetry(Adapter, FALSE)) // Raom again.
				return;
			else // Roam failed.
			{
				if(MgntResetOrPnPInProgress(Adapter))
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));				
					return;
				}	
			
				MgntRoamComplete(Adapter, MlmeStatus_timeout);
			}
		}
		else
		{	
			// Remember the rejected AP, if another AP in the same ESS is better,
			// pick the better one in further connection. 2006.11.21, by shien chang.
			MgntAddRejectedAsocAP(Adapter, pMgntInfo->Bssid);

			if(!MgntConnectOtherBss(Adapter) &&
				pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_INFRA)
			{ // Connect the Other BSS failed.

				if(MgntResetOrPnPInProgress(Adapter))
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 3\n"));				
					return;
				}		
				
				DrvIFIndicateAssociationStart(Adapter);
				DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_FAILURE);
				
				if(MgntRoamingInProgress(pMgntInfo)) // Roaming.
					MgntRoamComplete(Adapter, MlmeStatus_timeout);
				else // First connection.
					DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);

				pMgntInfo->bJoinInProgress = FALSE;
			}
		}
		
//		pMgntInfo->state_Synchronization_Sta = STATE_No_Bss;

		// Disable current QoS and HT setting since Join BSS process is failed.
		pMgntInfo->pStaQos->CurrentQosMode = QOS_DISABLE;
		pMgntInfo->pHTInfo->bCurrentHTSupport = FALSE;
		pMgntInfo->pVHTInfo->bCurrentVHTSupport = FALSE;
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 4\n"));
		return;
	}

	if( pMgntInfo->Regdot11networktype != RT_JOIN_NETWORKTYPE_INFRA)
	{
		if(GetIbssbJoinOnly(Adapter))
		{
			DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);
			return;
		}
		else
		{
			// Proting by tynli from 92E svn revision 7396. 2010.09.07.
			DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_SUCCESS);
		}
		
		// Check the IBSS starting channel is valid.
		if(!CHNL_IsLegalChannel(Adapter, pMgntInfo->Regdot11ChannelNumber))
			return;
		
		// Start an IBSS network if Join infra or adhoc network fails!
		RT_TRACE(COMP_MLME, DBG_LOUD,("JoinTimeout(): Start an IBSS\n"));

		pMgntInfo->OpMode = RT_OP_MODE_IBSS;

		if (pMgntInfo->JoinAction == RT_JOIN_IBSS) 
		{
			pMgntInfo->JoinAction = RT_START_IBSS; 
		}

		pMgntInfo->bIbssStarter = TRUE;

		SetupStartIBSSInfo( Adapter );

		IbssStartRequest( Adapter );
		pMgntInfo->state_Synchronization_Sta = STATE_Ibss_Active;
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_LOUD,("JoinTimeout(): indicate disconnect\n"));
		Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_NO_LINK); 
		MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );
	}

	RT_TRACE(COMP_MLME, DBG_LOUD,("JoinTimeout(): Join timeout! <====\n"));
}











/*
  * JoinRequestReschedule: 
  *			Check wheter reschedule of JoinRequest is required if RF is in OFF state
  *			<Step 1> Check whether reschedule is required
  *			<Step 2> If reschedule is required, Turn on RF, Save required parameter and 
  *					 reschedule for JoinRequest
  *	2008/01/03 Emily
  */
VOID
JoinRequestReschedule(
	PADAPTER				Adapter,
	RT_JOIN_ACTION		JoinAction,
	RT_WLAN_BSS			*bssDesc
)
{
	PMGNT_INFO      					pMgntInfo = &Adapter->MgntInfo;
	PMGNT_INFO      					pDefaultMgntInfo = &GetDefaultAdapter(Adapter)->MgntInfo;
	PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(pDefaultMgntInfo);
	RT_RF_POWER_STATE 			rtState;
	
	//2//<1> Check whether reschedule is required

	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));	
	if (rtState == eRfOff)
	{
		//
		// Check if in inactive power save mode. If yes, we should turn on rf and continue scan.
		// 2007.07.16, by shien chang.
		//
		if (pMgntInfo->RfOffReason > RF_CHANGE_BY_IPS)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("RescheduleJoinRequest(): RF is OFF. Reject JoinRequest\n"));
			return;
			
		}
		else if (!pMgntInfo->bSetPnpPwrInProgress)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("RescheduleJoinRequest(): Under inactive ps mode, turn on RF and continue.\n"));
						
			pPSC->tmpJoinAction = JoinAction;
			if (bssDesc == NULL)
			{
				pPSC->bTmpBssDesc = FALSE;
			}
			else
			{
				pPSC->bTmpBssDesc = TRUE;
				PlatformMoveMemory(&pPSC->tmpBssDesc, bssDesc, sizeof(RT_WLAN_BSS));
			}

			pPSC->ReturnPoint = IPS_CALLBACK_JOIN_REQUEST;
			pPSC->ptmpAdapter = Adapter;
			IPSLeave(Adapter,FALSE);
			PlatformSetTimer(Adapter, &pPSC->InactivePsTimer, SWRF_TIMEOUT);
			
			return;
		}
	
	}
}


VOID
JoinRequest(
	PADAPTER		Adapter,
	RT_JOIN_ACTION	JoinAction,
	RT_WLAN_BSS		*bssDesc
)
{
	PMGNT_INFO      	pMgntInfo = &Adapter->MgntInfo;
	RT_RF_POWER_STATE 	rtState;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  JoinRequest()\n"));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	// Scan is currently performed, and mark bScanOnly to be FASLE to start join after scanning.
	if(pMgntInfo->bScanInProgress && JoinAction != RT_NO_ACTION)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("JoinRequest Scan in progress! set bScanOnly to FALSE\n"));
		pMgntInfo->bScanOnly = FALSE;
		return;
	}

        //
	// When RF is off, we should not accept join request. 
	// 2007.02.07, by Roger.
	//
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));	
	if (rtState == eRfOff)
	{	
		JoinRequestReschedule(Adapter, JoinAction, bssDesc);
		RT_TRACE(COMP_MLME, DBG_LOUD, ("JoinRequest() <====\n"));
		return;
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

	//sherry added for adhoc connection
	//2010.02.03 
	WAPI_SecFuncHandler(WAPI_RETURNALLSTAINFO,Adapter, WAPI_END);

	if(IS_DUAL_BAND_SUPPORT(Adapter) && (IsExtAPModeExist(Adapter)))
	{
		if((GET_HAL_DATA(Adapter)->CurrentBandType == BAND_ON_2_4G) && (bssDesc->ChannelNumber>14))
		{
			RT_TRACE(COMP_MLME,DBG_LOUD,("JoinRequest: avoid CCK Hang \n"));
			pMgntInfo->bDisableCCKForDualBand = TRUE;

			{
				PADAPTER pApAdapter = GetFirstAPAdapter(Adapter) ? GetFirstAPAdapter(Adapter) : GetDefaultAdapter(Adapter);
				pApAdapter->MgntInfo.bDisableCCKRateForVWIFIChangeChannel = TRUE;
			}
		}
	}
	

	//
	// <Roger_Notes> After discussion with CCW, we should remove all TS here to avoid incorrect TS maintenance from 
	// any roaming process or re-connection process after resuming from system hibernation or sleep, media status changed 
	// or AP re-connection. Re-correction would reset all TS in AP site but we do NOT reset them identically.
	//
	// 2012.08.13.
	//
	if(JoinAction != RT_NO_ACTION)
	{
		RemoveAllTS(Adapter);
		pMgntInfo->last_auth_seq = 0xffff;
		pMgntInfo->last_asoc_req_seq = 0xffff;
		pMgntInfo->last_asoc_rsp_seq = 0xffff;
		pMgntInfo->last_Disassoc_seq = 0xffff;
		pMgntInfo->last_Deauth_seq = 0xffff;
	}
	
	switch(JoinAction)
	{
		case RT_JOIN_INFRA:
			RT_TRACE(COMP_MLME, DBG_LOUD, ("JoinRequest(): RT_JOIN_INFRA.......\n"));
			Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_START_TO_LINK); 
			pMgntInfo->OpMode = RT_OP_MODE_INFRASTRUCTURE;
			
			if((GET_POWER_SAVE_CONTROL(pMgntInfo)->bLeisurePs) && 
				(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS))
			{
				LeisurePSLeave(Adapter, LPS_DISABLE_JOIN_REQ);
			}
			
			// Setup infrastructure variables
			SetupJoinInfraInfo( Adapter, bssDesc );
			
			// Start a join timer
			PlatformSetTimer( Adapter, &pMgntInfo->JoinTimer, JOIN_TIMEOUT );

			PlatformSetTimer(Adapter,&pMgntInfo->JoinProbeReqTimer, 200);

			// State: Wait beacon
			pMgntInfo->state_Synchronization_Sta = STATE_Join_Wait_Beacon;

			break;

		case RT_JOIN_IBSS:
			RT_TRACE(COMP_MLME, DBG_LOUD, ("JoinRequest(): RT_JOIN_IBSS.......\n"));


			pMgntInfo->OpMode = RT_OP_MODE_IBSS;

			GET_HAL_DATA(Adapter)->bNeedIQK = TRUE;
			GET_HAL_DATA(Adapter)->DM_OutSrc.RFCalibrateInfo.bNeedIQK = TRUE;

			// Setup IBSS variables
			SetupJoinIBSSInfo( Adapter, bssDesc );

			// Start a join timer
				PlatformSetTimer( Adapter, &pMgntInfo->JoinTimer, JOIN_TIMEOUT );

			// State: Wait beacon
			pMgntInfo->state_Synchronization_Sta = STATE_Join_Wait_Beacon;
			
			break;

		case RT_START_IBSS:
			{
				PRT_CHNL_LIST_ENTRY	pChnlListEntry = NULL;
				u1Byte				Channel = pMgntInfo->Regdot11ChannelNumber;
				BOOLEAN				bStart = FALSE;
				
				RT_TRACE(COMP_MLME, DBG_LOUD, ("JoinRequest(): RT_START_IBSS.......\n"));

				do
				{	
					RT_TRACE(COMP_MLME, DBG_LOUD, ("ChangeWirelessModeHandler\n"));
					HalChangeWirelessMode(Adapter, Channel);
					
					//
					// Get channel info.
					//
					RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL, &Channel, &pChnlListEntry);

					if(!pChnlListEntry) // This channel is invlid.
						break;
			
					if(pChnlListEntry->ExInfo & CHANNEL_EXINFO_NO_IBSS_START) // Do not start IBSS at this channel.
					{
						//
						//When in NDISTest, SUT may start IBSS with channel 1. So we need should not break there.
						//By Maddest 05132009.
						//
						if(!Adapter->bInHctTest)
							break;
					}

					pMgntInfo->OpMode = RT_OP_MODE_IBSS;
					GET_HAL_DATA(Adapter)->bNeedIQK = TRUE;
					GET_HAL_DATA(Adapter)->DM_OutSrc.RFCalibrateInfo.bNeedIQK = TRUE;

					//
					//In some situation, we will send beacon in the wrong channel, such as send beacon on channel 10, 
					//but the beacon IE channel is 1. So that no one can connect to it. By Maddest 05132009
					//

					{
						if(Channel != pMgntInfo->dot11CurrentChannelNumber)
							Mgnt_SwChnl( Adapter, Channel, 1);
					}
					//
					//Porting from 8xb to avoid IBSS can not relink after s3/s4 on Vista, by Maddest
					//
					if(pMgntInfo->bInToSleep)
						pMgntInfo->bIbssStarter = TRUE;

					// Setup IBSS variables
					SetupStartIBSSInfo( Adapter );

					RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, ("JoinRequest(): RT_START_IBSS \n"), pMgntInfo->Bssid );

					if(MgntResetOrPnPInProgress(Adapter))
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress RT_START_IBSS\n"));
						return;
					}
					
					//
					//Porting from 8xb to avoid IBSS can not relink after s3/s4 on Vista, by Maddest
					//
					// Preprocessing for starting a connection. 2006.10.17, by shien chang.
					if((!pMgntInfo->bInToSleep   || ACTING_AS_AP(Adapter)) &&
						!MgntRoamingInProgress(pMgntInfo) && pMgntInfo->RequestFromUplayer)
						DrvIFIndicateConnectionStart(Adapter);
					else if(MgntRoamingInProgress(pMgntInfo))
						DrvIFIndicateRoamingStart( Adapter);
					
					IbssStartRequest( Adapter );
					RT_TRACE(COMP_MLME, DBG_TRACE,("JoinRequest(): \n"));

					if(MgntResetOrPnPInProgress(Adapter))
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 4\n"));
						return;
					}					
					//
					//Porting from 8xb to avoid IBSS can not relink after s3/s4 on Vista, by Maddest
					//
					// Postprocessing for starting a connection, 2006.10.27, by shien chang.
					if((!pMgntInfo->bInToSleep   || ACTING_AS_AP(Adapter)) &&
						!MgntRoamingInProgress(pMgntInfo) && pMgntInfo->RequestFromUplayer)
						DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_SUCCESS);
					else if (MgntRoamingInProgress(pMgntInfo))
					{
						MgntRoamComplete(Adapter, MlmeStatus_success);
					}

					// <Roger_Notes> Sync from 91su branch, 2009.04.27.
					pMgntInfo->bInToSleep = FALSE;
					// State: Active Ibss
					pMgntInfo->state_Synchronization_Sta = STATE_Ibss_Active;
					
					bStart = TRUE;
					
				}while(FALSE);

				if(!bStart)
				{
					pMgntInfo->JoinAction = RT_NO_ACTION;
					RT_TRACE(COMP_SCAN, DBG_TRACE, ("JoinRequest():RT_START_IBSS - Do not start IBSS at channel(%d).\n", Channel));
					pMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
					pMgntInfo->state_Synchronization_Sta = STATE_No_Bss;
				}
			}
			break;

		case RT_NO_ACTION:
			RT_TRACE(COMP_MLME, DBG_LOUD, ("JoinRequest(): RT_NO_ACTION !!!!!!!!!!!!\n"));
			break;

		default:
			RT_TRACE(COMP_MLME, DBG_WARNING, ("JoinRequest(): unknown JoinAction (%d)!!!\n", JoinAction));
			break;
	}
	RT_TRACE(COMP_MLME, DBG_LOUD, ("JoinRequest() <====\n"));
}



VOID
JoinProbeReq(
	PRT_TIMER	pTimer
)
{
	PADAPTER		Adapter=(PADAPTER)pTimer->Adapter;
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	u2Byte	nDataRate = (IS_WIRELESS_MODE_5G(Adapter) ||
						(IS_WIRELESS_MODE_HT_24G(Adapter)&& !pMgntInfo->pHTInfo->bCurSuppCCK)) ? MGN_6M: MGN_1M;

	if(RT_DRIVER_STOP(Adapter)||Adapter->bDriverIsGoingToPnpSetPowerSleep)
		return;

	if(!pMgntInfo->bJoinInProgress)
		return;

	while(pHalData->SwChnlInProgress||pMgntInfo->pChannelInfo->bSwBwInProgress )
	{
		PlatformSetTimer(Adapter,&pMgntInfo->JoinProbeReqTimer,50);
		return;
	}

	SendProbeReq(Adapter, FALSE, nDataRate, FALSE);
	SendProbeReqEx(Adapter, nDataRate, FALSE);
	
	PlatformSetTimer(Adapter,&pMgntInfo->JoinProbeReqTimer,200);

}




//(1)Adhoc mode: Send auth to Ibss
//(2)Infrastructure mode: Send auth to AP
VOID
JoinConfirm(
	PRT_TIMER	pTimer
)
{
	PADAPTER	Adapter=(PADAPTER)pTimer->Adapter;

	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;
	RT_OP_MODE	OpMode = pMgntInfo->OpMode;
	u1Byte		needauthreq = 0;
	u1Byte		auStaAddr[6];
	u1Byte		auAlg = OPEN_SYSTEM;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("====>  JoinConfirm()\n"));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

	pMgntInfo->mDisable = FALSE;

	while(pHalData->SwChnlInProgress||pMgntInfo->pChannelInfo->bSwBwInProgress)
	{
		PlatformSetTimer(Adapter,&pMgntInfo->JoinConfirmTimer,50);
		return;
	}
	
	if( OpMode == RT_OP_MODE_INFRASTRUCTURE ){
	//(1)Infrastructure mode: Send auth to AP
		//Send authentication request if received beacon during join period
		needauthreq = 1;
	}
	else if( OpMode == RT_OP_MODE_IBSS ){
	//(2)Adhoc mode: Send auth to Ibss, or Start an IBSS
		if(pMgntInfo->bIbssNeedAuth){
			needauthreq = 1;
		}
		else{
			IbssStartRequest( Adapter );
		}
	}
	else{
		RT_TRACE(COMP_INIT, DBG_TRACE,("JoinConfirm(): Unknown OP mode\n"));
	}

	if( needauthreq ){	
		CopyMem( auStaAddr, pMgntInfo->Bssid, 6 );

		// If we are in auto switch mode, start with shared key authentication first. 2005.03.08, by rcnjko.  
		if(pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeAutoSwitch)
		{
			pMgntInfo->AuthReq_auAlg = SHARED_KEY;
		}
		
		if(FtIsFtAuthReady(Adapter, pMgntInfo->Bssid))			
		{
			RT_PRINT_ADDR(COMP_SEC, DBG_LOUD, "Change auth alg to FT for BSSID = ", pMgntInfo->Bssid);
			pMgntInfo->AuthReq_auAlg = AUTH_ALG_FT;
		}

		auAlg = pMgntInfo->AuthReq_auAlg;

		if( pMgntInfo->RoamingState == ROAMINGSTATE_SCANNING){
			pMgntInfo->RoamingState = ROAMINGSTATE_AUTHENTICATION;
		}
		PlatformCancelTimer(Adapter, &pMgntInfo->AuthTimer);
		pMgntInfo->AuthRetryCount = 0;		

		MlmeAuthenticateRequest( Adapter, auStaAddr, auAlg, AUTH_REQ_TIMEOUT );
	}

	if(pHalData->bAutoAMPDUBurstMode)
	{
		// For Early mode timeout interrupt.
		HAL_HW_TIMER_TYPE TimerType = HAL_TIMER_EARLYMODE;
		u4Byte	val = TimerType<<16;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_HW_REG_TIMER_INIT,  (pu1Byte)(&val));
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_HW_REG_TIMER_RESTART,  (pu1Byte)(&TimerType));	
	}
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("JoinConfirm() <====\n"));
}

//
// Description:
//	Handle the actions (ex. event indications)  after roaming is complete.
// Arguments:
//	pAdapter -
//		NIC Adapter context pointer.
//	Result -
//		The input roaming status indicating that roaming is success or fail.
// Note:
//	We use "MlmeStatus" to indicate the roaming status for input, because the 
//	status is now enough and satisfied for the current roaming procedure. 
// By Bruce, 2008-05-16.
//
VOID
MgntRoamComplete(
	IN	PADAPTER		pAdapter,
	IN	u2Byte			Result
	)
{
	PMGNT_INFO			pMgntInfo = &pAdapter->MgntInfo;
	
	FunctionIn(COMP_MLME);
	
	if( (Result != MlmeStatus_success) )
	{ // Roaming Failed.
		if(!OS_SUPPORT_WDI(pAdapter))
		{
			DrvIFIndicateAssociationStart(pAdapter);
			DrvIFIndicateAssociationComplete(pAdapter, RT_STATUS_FAILURE);
			DrvIFIndicateRoamingComplete(pAdapter, RT_STATUS_FAILURE);
		}

		CCX_OnRoamFailed(pAdapter);

		if(pMgntInfo->bMediaConnect)
			DrvIFIndicateDisassociation(pAdapter, dest_unreachable, pMgntInfo->Bssid);//unspec_reason); // for win8 dtm

		MgntIndicateMediaStatus(pAdapter, RT_MEDIA_DISCONNECT, FORCE_INDICATE);

		if(OS_SUPPORT_WDI(pAdapter))
		{
			//20150415 Sinda
		//We should invoke roaming complete if WDI had sent ROAM task.
		if( pMgntInfo->bRoamRequest == TRUE )
		{
			DrvIFIndicateConnectionComplete(pAdapter, RT_STATUS_FAILURE);
			pMgntInfo->bRoamRequest = FALSE;
		}
		else
			DrvIFIndicateRoamingComplete(pAdapter, RT_STATUS_FAILURE);
		}
		
		// Reset all mgnt variables.
		ResetMgntVariables(pAdapter);
	}
	else
	{
		if(OS_SUPPORT_WDI(pAdapter))
		{
			//20141027 Sinda
		//We should not indicate WDI_INDICATION_ASSOCIATION_RESULT and WDI_INDICATION_LINK_STATE_CHANGE
		//Because they had indicated when association complete on roaming flow.
		if( pMgntInfo->bRoamRequest == TRUE )
		{
			DrvIFIndicateConnectionComplete(pAdapter, RT_STATUS_SUCCESS);
			pMgntInfo->bRoamRequest = FALSE;
		}
		else
			DrvIFIndicateRoamingComplete(pAdapter, RT_STATUS_SUCCESS);
		}
		else
			DrvIFIndicateRoamingComplete(pAdapter, RT_STATUS_SUCCESS);
		// Reset all variables about roaming.
		MgntResetRoamingState(pMgntInfo);
		MgntResetJoinCounter(pMgntInfo);	

		//to avoid no linkquality indication
		pAdapter->RxStats.LastLinkQuality = 0xFF;
	}

	pMgntInfo->RegFakeRoamSignal[0] = 0;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntRoamComplete(): Roaming is %s!!!\n",
		(Result == MlmeStatus_success) ? "successful" : "failed"));

}



VOID
MgntSetScanTimer(
	IN	PADAPTER	Adapter,
	IN	u4Byte		TimerIntv)
{
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;

	PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, TimerIntv);// For WinXP, Vista, etc.
}


BOOLEAN
ScanSendProbeReq(
	PADAPTER			Adapter,
	u1Byte				Idx,
	PRT_CHNL_LIST_ENTRY	pChnlListEntry
	)
{
	BOOLEAN		bAnySsid;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	u2Byte		nDataRate = IS_WIRELESS_MODE_5G(Adapter) ? MGN_6M: MGN_1M;

	if(!CustomScan_SendProbeCb(GET_CUSTOM_SCAN_INFO(Adapter), pChnlListEntry))
		return TRUE;

	//	Send out 2 probe request for each channel
	//		0. Dedicate probe	(Low rate)
	//		1. Any probe		(Low rate)

	if(	(IsSSIDAny( pMgntInfo->Ssid ))	||
		(IsSSIDDummy( pMgntInfo->Ssid )) || // This SSID is not regular. By Bruce, 2008-12-30.
		(Idx%2!=0)) // Asked by owen, 2005.03.24. 
	{
		//3 Any probe
		if(!MgntRoamingInProgress(pMgntInfo))
			bAnySsid=TRUE;
		else 
			return FALSE;
	}
	else
	{	//3 Dedicate probe
		bAnySsid=FALSE;
	}

	//20061212 david, in order to solve packet filter bug
	if(pMgntInfo->bHiddenSSIDEnable == FALSE)
		bAnySsid = TRUE;

	// TODO: Send Probe request use different data rate
	// <RJ_TODO> We should send ProbeReq in basic rate, o.w. pure-A or pure-G BSS
	// won't respond to us. 2004.12.06, by rcnjko.
	// We set PwrMgt bit of ProbeReq of original channel in prevent of ping packet loss. 2004.12.10, by rcnjko.
	//
	// We cannot send ProbeReq with PwrMgt bit, o.w. some hidden AP, e.g. WGR615v, will not 
	// response our ProbeRsp. 2005.03.23, by rcnjko.
	//
	if(	pMgntInfo->mAssoc && 
		pChnlListEntry->ChannelNum == pMgntInfo->SettingBeforeScan.ChannelNumber&&
		bAnySsid == TRUE &&
		pMgntInfo->RoamingType != RT_ROAMING_NORMAL && pMgntInfo->bMediaConnect)
	{
		if(!pMgntInfo->bScanWithMagicPacket)
		{
			SendProbeReq(Adapter, bAnySsid, nDataRate, TRUE);
		
			if(!pMgntInfo->bHiddenSSIDEnable)
				SendProbeReqEx(Adapter, nDataRate, TRUE);

			if(Adapter->bInHctTest || CustomScan_InProgress(GET_CUSTOM_SCAN_INFO(Adapter)))
			{
				SendProbeReq(Adapter, bAnySsid, nDataRate, TRUE);
				SendProbeReq(Adapter, bAnySsid, nDataRate, TRUE);
			}
		}
		else
		{ // 2005.06.27, by rcnjko.
			SendMagicPacket(Adapter, pMgntInfo->MagicPacketDstAddr);
		}
	}
	else
	{
		if(!pMgntInfo->bScanWithMagicPacket)
		{
			if(!pMgntInfo->bScanOnly && !Adapter->bInHctTest)					
				bAnySsid = FALSE;
	
			SendProbeReq(Adapter, bAnySsid, nDataRate, FALSE);
			SendProbeReqEx(Adapter, nDataRate, FALSE);
			if(Adapter->bInHctTest || CustomScan_InProgress(GET_CUSTOM_SCAN_INFO(Adapter)))
			{
				SendProbeReq(Adapter, bAnySsid, nDataRate, FALSE);
				SendProbeReq(Adapter, bAnySsid, nDataRate, FALSE);
			}
		}
		else
		{ // 2005.06.27, by rcnjko.
			SendMagicPacket(Adapter, pMgntInfo->MagicPacketDstAddr);
		}
	}

	return TRUE;
}

VOID
ScanSwitchChannel(
	PADAPTER			Adapter,
	PMGNT_INFO			pMgntInfo,
	PRT_CHNL_LIST_ENTRY	pChnlListEntry
)
{
	WIRELESS_MODE WirelessModeInScan = pMgntInfo->SettingBeforeScan.WirelessMode;

	if(pMgntInfo->bResetInProgress)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("pMgntInfo->bResetInProgress\n"));
		return;
	}

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("ScanSwitchChannel():Switch to channel(%d) period (%d)\n", pChnlListEntry->ChannelNum, pChnlListEntry->ScanPeriod));	

	if(IS_DUAL_BAND_SUPPORT(Adapter))
		WirelessModeInScan = pMgntInfo->SettingBeforeScan.WirelessModeScanInProgress;

	if(WirelessModeInScan != pMgntInfo->dot11CurrentWirelessMode)
	{
		RT_TRACE(COMP_SCAN, DBG_WARNING, ("ScanSwitchChannel():Befor scan wirelessmode(%d) dot11wirelessmode (%d)\n", 
											WirelessModeInScan, pMgntInfo->dot11CurrentWirelessMode));
		Adapter->HalFunc.SetWirelessModeHandler(Adapter, (u1Byte)(WirelessModeInScan));
	}
		
	Mgnt_SwChnl(Adapter, pChnlListEntry->ChannelNum, 1);

	// Go to next step.
	pMgntInfo->ScanStep++;
	
	//Sinda 20150915, we use workitem to switch channel and os execute workitem after 15 ms.
	//So we should set timer with 15 ms to avoid timer break off workitem.
	MgntSetScanTimer(Adapter, 15);
}

VOID
ScanSwitchComplete(
	PADAPTER	Adapter,
	PMGNT_INFO	pMgntInfo
)
{
	BOOLEAN bDoWMMFix;
	
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("===> ScanSwitchComplete(),scan complete restore.\n"));

	if(IS_DUAL_BAND_SUPPORT(Adapter))
	{
		if(pMgntInfo->bDualModeScanStep == 1 && !pMgntInfo->bCompleteScan)
			goto GOTO_NEXT_BAND_SCAN;	

		if(pMgntInfo->bCompleteScan)
		{
			pMgntInfo->bDualModeScanStep = 2;
			RT_TRACE(COMP_MLME, DBG_LOUD, ("===> ScanSwitchComplete(),skip scan.\n"));
		}
	}

	CustomScan_PreScanCompleteCb(GET_CUSTOM_SCAN_INFO(Adapter));
	
	// Scan completed. Restore original setting.
	if(	pMgntInfo->SettingBeforeScan.WirelessMode != pMgntInfo->dot11CurrentWirelessMode)
	{
		bDoWMMFix = FALSE;
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_WMM_NEED_FIX_PARAM, (pu1Byte)(&bDoWMMFix));
		if(bDoWMMFix)
		{
			if(PlatformCompareMemory(Adapter->MgntInfo.pStaQos->WMMParamEle, pMgntInfo->SettingBeforeScan.WMMParamEle,WMM_PARAM_ELEMENT_SIZE))
			{
				PlatformMoveMemory(Adapter->MgntInfo.pStaQos->WMMParamEle, pMgntInfo->SettingBeforeScan.WMMParamEle,WMM_PARAM_ELEMENT_SIZE);
			}
		}
		
		Adapter->HalFunc.SetWirelessModeHandler(Adapter, (u1Byte)(pMgntInfo->SettingBeforeScan.WirelessMode));
	}

	HAL_ChannelAndBandWidthRecoveryAfterScan(Adapter);

	GOTO_NEXT_BAND_SCAN:			
	// Wait for original status
	pMgntInfo->ScanStep = 2;
//	PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, 1);
	ScanComplete(Adapter);
	FunctionOut(COMP_SCAN);

}


VOID
ScanMergeResult(
	PADAPTER	Adapter,
	PMGNT_INFO	pMgntInfo
	)
{
	u2Byte CountBss,CountQueryBss, CountToMoveToBack, StartToMoveBackAddr;
	u2Byte NumBssDesc4Query = pMgntInfo->NumBssDesc4Query;
	u8Byte DiffTime=0, CurrentTime = PlatformGetCurrentTime();
	u8Byte DiffTimeThreshold = 0;
	BOOLEAN bFoundBss[MAX_BSS_DESC] = {0};
	BOOLEAN bMoveToBack, bNeedToMove;

	//Fix Adhoc UI Issue for 8812AU, Merge from branch1022
	if(pMgntInfo->bCompleteScan && !pMgntInfo->bScanTimeExceed && !pMgntInfo->bNeedSkipScan)
		return;

	if(pMgntInfo->NdisVersion >= RT_NDIS_VERSION_6_20 && Adapter->bInHctTest)
	{
		DiffTimeThreshold = 10;
	}
	else
	{
		DiffTimeThreshold = 120;
	}

	// Check all Desc4Query is in bssDesc
	for(CountQueryBss = 0; CountQueryBss < NumBssDesc4Query; CountQueryBss++)
	{
		if(BssDescDupByDesc( Adapter, &pMgntInfo->bssDesc4Query[CountQueryBss]))
		{
			bFoundBss[CountQueryBss] = TRUE;
		}
	}
	

	if(pMgntInfo->NumBssDesc4Query > pMgntInfo->NumBssDesc)
	{
		bNeedToMove = TRUE;
		StartToMoveBackAddr = CountToMoveToBack = pMgntInfo->NumBssDesc4Query;
	}
	else
	{
		bNeedToMove = FALSE;
		StartToMoveBackAddr = CountToMoveToBack = pMgntInfo->NumBssDesc;
	}	

	// If the bss is not in the lastest scan list, then decrease its history count, check its live time.
	// Remove the bss, if history count is reach to zero.
	// Remove the bss, if its live time out of 2 seconds. 
	for(CountQueryBss=0; CountQueryBss < NumBssDesc4Query && CountToMoveToBack < MAX_BSS_DESC; CountQueryBss++)
	{
		if(bFoundBss[CountQueryBss] == FALSE)
		{
			// If scan is reset because of reset, don't decrease the count otherwise the folowing 
			// SelectNetworkBySSID of connect request would return no action and force an 
			// unnecessary scan. 
			if(pMgntInfo->bssDesc4Query[CountQueryBss].HistoryCount > 0
				&& !pMgntInfo->bResetInProgress
				)
				pMgntInfo->bssDesc4Query[CountQueryBss].HistoryCount --;
			
			if( (pMgntInfo->bssDesc4Query[CountQueryBss].HistoryCount != 0) )
			{
				if(CurrentTime > pMgntInfo->bssDesc4Query[CountQueryBss].HistoryTime)
					DiffTime  = PlatformDivision64(CurrentTime - pMgntInfo->bssDesc4Query[CountQueryBss].HistoryTime, 1000000);
				else
					DiffTime  = PlatformDivision64(pMgntInfo->bssDesc4Query[CountQueryBss].HistoryTime - CurrentTime, 1000000);

				if(DiffTime < DiffTimeThreshold)				
					bMoveToBack = TRUE; // Keep it if keepalive
				else
					bMoveToBack = FALSE; // Remove it when timeout
			}
			else
				bMoveToBack = FALSE;	// Remove it when count zero
		}
		else
			bMoveToBack = FALSE; //Keep it 
		
		if(bMoveToBack)
		{		
			CopyWlanBss(pMgntInfo->bssDesc4Query+CountToMoveToBack, pMgntInfo->bssDesc4Query+CountQueryBss);
			CountToMoveToBack++;
		}
		if(bFoundBss[CountQueryBss] == FALSE)
		{
			//RT_TRACE(COMP_SCAN, DBG_TRACE, ("History %d DiffTime %"i64fmt"d channel %d\n",pMgntInfo->bssDesc4Query[CountQueryBss].HistoryCount, DiffTime, pMgntInfo->bssDesc4Query[CountQueryBss].ChannelNumber));
			RT_PRINT_STR(COMP_SCAN, DBG_TRACE,"Ssid", pMgntInfo->bssDesc4Query[CountQueryBss].bdSsIdBuf, pMgntInfo->bssDesc4Query[CountQueryBss].bdSsIdLen);
			RT_PRINT_STR(COMP_SCAN, DBG_TRACE,"Bssid", pMgntInfo->bssDesc4Query[CountQueryBss].bdBssIdBuf, 6);		
		}
	}
	
	pMgntInfo->NumBssDesc4Query = pMgntInfo->NumBssDesc;
	// Move the lost Bss to the tail of Bss List
	for(	CountBss = pMgntInfo->NumBssDesc, CountQueryBss = StartToMoveBackAddr; 
		CountQueryBss < CountToMoveToBack; CountQueryBss++, CountBss++)
	{
		if(bNeedToMove)
			CopyWlanBss(pMgntInfo->bssDesc4Query+CountBss, pMgntInfo->bssDesc4Query+CountQueryBss);

		pMgntInfo->NumBssDesc4Query++;
	}
		
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: ScanMergeResult(), NumBssDesc4Query=%d\n", pMgntInfo->NumBssDesc4Query));
	for(CountBss = 0; CountBss < pMgntInfo->NumBssDesc; CountBss++)
	{
		CopyWlanBss(pMgntInfo->bssDesc4Query+CountBss, pMgntInfo->bssDesc+CountBss);
		pMgntInfo->bssDesc4Query[CountBss].HistoryCount = History_Count_Limit;
		pMgntInfo->bssDesc4Query[CountBss].HistoryTime = CurrentTime;
	}
	
	for(CountQueryBss = 0;CountQueryBss < pMgntInfo->NumBssDesc4Query;CountQueryBss++)
	{
		//RT_TRACE(COMP_SCAN, DBG_TRACE, ("# %d HistoryCount %d HistoryTime %"i64fmt"d\n",CountQueryBss, pMgntInfo->bssDesc4Query[CountQueryBss].HistoryCount, pMgntInfo->bssDesc4Query[CountQueryBss].HistoryTime));
		RT_PRINT_STR(COMP_SCAN, DBG_TRACE,"Ssid", pMgntInfo->bssDesc4Query[CountQueryBss].bdSsIdBuf, pMgntInfo->bssDesc4Query[CountQueryBss].bdSsIdLen);
	}	
	
	RxDesc_BackupScanList(Adapter);
}



VOID
ScanByTimer(
	PADAPTER	Adapter,
	BOOLEAN		bActiveScan,
	BOOLEAN		bScanOnly
	)
{
	BOOLEAN		bFilterOutNonAssociatedBSSID;
	PADAPTER 	pDfAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO 	pDfMgntInfo = &pDfAdapter->MgntInfo;
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;
	u2Byte		scanTimer = 0;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("MAC %d: ===> ScanByTimer() port number %d CustomizedScanRequest %d ScanOnly %d\n",Adapter->interfaceIndex, Adapter->pNdis62Common->PortNumber, CustomScan_InProgress(GET_CUSTOM_SCAN_INFO(Adapter)), bScanOnly));

	if(RT_DRIVER_STOP(pDfAdapter))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("<=== ScanByTimer(): bDriverStopped.\n"));
		{
			PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);
		
			while(pLoopAdapter)
			{
				pLoopAdapter->MgntInfo.bScanInProgress = FALSE;
				pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
			}
		}
		return;	
	}
	
	if(MgntScanInProgress(pMgntInfo) || (pMgntInfo->bDualModeScanStep==2) )
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("<=== ScanByTimer(): scan in progress. MgntScanInProgress(pMgntInfo) %d bDualModeScanStep %d\n", MgntScanInProgress(pMgntInfo), pMgntInfo->bDualModeScanStep));

		// An dirty workaround for scan process. If the scan is in progress, and it is called
		// before connect. Then we set the scan flag again to let the last scan complete event
		// be indicated. 2007.01.04, by shien chang.
		if (pMgntInfo->bIndicateConnectEvent) 
		{
			DrvIFIndicateScanStart(Adapter);
		}
		return;
	}
	else if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress\n"));
		return;
	}	

	// For 40Mhz mode
	pMgntInfo->bScan_20MHz = FALSE;

	// Save parameters ---------------------------------------------
	{
		PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);

		while(pLoopAdapter)
		{
			pLoopAdapter->MgntInfo.bScanInProgress = TRUE;
			if(pLoopAdapter->MgntInfo.Scancount==0xFF)
				pLoopAdapter->MgntInfo.Scancount = 0;
			else 
				pLoopAdapter->MgntInfo.Scancount++;
			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}
	// -----------------------------------------------------------
	
	CustomScan_ScanByTimerCb(GET_CUSTOM_SCAN_INFO(Adapter));

	//Sherry Sync with 92C_92D 20110701.  To Avoid CCK Hang with 2.4G Send Beacon on CCK Rate
	pDfMgntInfo->bDisableCCKForDualBand = TRUE;

	pMgntInfo->bActiveScan=bActiveScan;
	pMgntInfo->bScanOnly=bScanOnly;

	pDfMgntInfo->bLoadIMRandIQKSettingFor2G = FALSE;

	// Reset scan state
	pMgntInfo->ScanStep=0;
	pMgntInfo->bResetScan = FALSE;

	// Save state_Synchronization_Sta parameters --------------------------------------------------------------------------
	{
		PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);

		RT_TRACE(COMP_SCAN, DBG_LOUD,("Set All Port's pMgntInfo->state_Synchronization_Sta = STATE_Act_Listen\n"));

		while(pLoopAdapter)
		{
			pLoopAdapter->MgntInfo.state_Synchronization_Sta_BeforeScan = pLoopAdapter->MgntInfo.state_Synchronization_Sta;

			pLoopAdapter->MgntInfo.state_Synchronization_Sta = STATE_Act_Listen;

			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}
	// ----------------------------------------------------------------------------------------------------------------


	//3 Construct channel list
	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_CONSTRUCT_SCAN_LIST, NULL, NULL);

	DFS_StaUpdateRadarChnlScanType(Adapter);

	// Keep log for DOT11D verification.
	if(IS_DOT11D_ENABLE(pMgntInfo) )
	{
		if(bActiveScan)
		{
			PRT_DOT11D_INFO pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);

			AddDrvLog(Adapter, LID_DOT11D_ACTIVE_SCAN_CHNL, pDot11dInfo->ChnlList, pDot11dInfo->ChnlListLen);
			AddDrvLog(Adapter, LID_DOT11D_ACTIVE_SCAN_TXPWR, (UCHAR *)pDot11dInfo->MaxTxPwrDbmList, pDot11dInfo->ChnlListLen);
		}
		else
		{
			AddDrvLog(Adapter, LID_DOT11D_PASSIVE_SCAN_CHNL, (pu1Byte)GET_RT_CHANNEL_LIST(pMgntInfo), sizeof(RT_CHANNEL_LIST));
		}
	}


	//cosa masked 03302009 for lenovo.
	// Because of the following ASPM flow, the HW_VAR_CHECK_BSSID will be at the wrong
	// state when disconnet, so now we masked here and make sure the check bssid state
	// will be set ok when scan. (even disconnect)
	// 1. Enable ASPM
	// 2. Disassociate
	// 3. Scan
	// 4. Disable ASPM
	// Note: The io between 1. and 4. will be ignored.
	//if(pMgntInfo->mIbss || pMgntInfo->mAssoc)
	{
		//xiong:for 8192 usb, need do this? consider it later
		// Clear RCR_CBSSID to receive the frames from other BSSID.
		// This is needed for IBSS mode or it cannot scan other BSS.
		bFilterOutNonAssociatedBSSID = FALSE;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
	}

	Adapter->HalFunc.ScanOperationBackupHandler(Adapter, SCAN_OPT_BACKUP_BAND0);

	MultiChannelScanHandler(Adapter, TRUE, FALSE);

	// Joseph 20090703: Prepare to sniff the packets in the channel. This can be used to determine
	// whether traffic is busy or not.
	Adapter->RxStats.CurRxSniffMediaPktCnt = 0;

	// Tell AP we are in Power-Save mode.
	Hal_PauseTx(Adapter, HW_DISABLE_TX_DATA);
	
	// In prevent of lossing ping packet during scanning.  2004.12.06, by rcnjko.
	scanTimer = scanGetScanTimer(Adapter);

	PlatformSetTimer(Adapter, &Adapter->MgntInfo.ScanTimer, scanTimer);

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("MAC %d timer = %d ms <=== ScanByTimer()\n",Adapter->interfaceIndex, scanTimer));
}


VOID
MgntConstructAnotherBandScanList(
	IN	PADAPTER	Adapter
)
{
	WIRELESS_MODE	wirelessmode;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	if(IS_WIRELESS_MODE_5G(Adapter))
		wirelessmode = WIRELESS_MODE_G;
	else
		wirelessmode = WIRELESS_MODE_A;
	
	Adapter->HalFunc.SetWirelessModeHandler(Adapter, (u1Byte)(wirelessmode));
	pMgntInfo->SettingBeforeScan.WirelessModeScanInProgress = wirelessmode;
	RT_TRACE(COMP_SCAN,DBG_LOUD,("Reconstruct scan list wireless mode 0x%x \n", wirelessmode));
	
	// Reset scan state
	pMgntInfo->ScanStep=0;
	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_CONSTRUCT_SCAN_LIST, NULL, NULL);

	Adapter->HalFunc.ScanOperationBackupHandler(Adapter, SCAN_OPT_BACKUP_BAND1);
}


BOOLEAN
ScanSelectAntenna(
	PADAPTER	Adapter
)
{
	BOOLEAN			ret = FALSE;
	PHAL_DATA_TYPE	pHalData	= GET_HAL_DATA(Adapter);

	//	Note for CustomizedScanRequest
	//	If the scaning process is initiated by the CustomizedScanRequest, simply do the scan 
	//	specified in the CustomizedScanRequest context.
	
	// The following codes conduct the normal scanning process again due to the antenna diversity.
	
	// 20100514 Joseph: Interrupt scan operation here.
	// For SW antenna diversity before link, it needs to switch to another antenna and scan again.
	// It compares the scan result and select beter one to do connection.
	
	if(CustomScan_InProgress(GET_CUSTOM_SCAN_INFO(Adapter)))
		ret = FALSE;
	else if ((&pHalData->DM_OutSrc)->Adapter != NULL)
	{
		if(Adapter->HalFunc.SwAntDivCheckBeforeLinkHandler(&pHalData->DM_OutSrc))
		ret = TRUE;
	else if(Adapter->HalFunc.PathDivCheckBeforeLinkHandler(&pHalData->DM_OutSrc))
		ret = TRUE;
	}
	return ret;
}

VOID
ScanComplete(
	PADAPTER	Adapter
	)
{
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;
	BOOLEAN		bFilterOutNonAssociatedBSSID = FALSE;
	u2Byte		i;
	u4Byte		OrgBssMunber = 0;
	BOOLEAN		bStopScan = FALSE;
	
	PLATFORM_WRITE_EVENT_LOG(Adapter, RT_SCAN_COMPLETE, 0);

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("MAC %d ===> ScanComplete() port number %d CustomizedScanRequest.bEnabled %d bCompleteScan %d bScanOnly %d\n",Adapter->interfaceIndex, Adapter->pNdis62Common->PortNumber, CustomScan_InProgress(GET_CUSTOM_SCAN_INFO(Adapter)), pMgntInfo->bCompleteScan, pMgntInfo->bScanOnly));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}
	
	PlatformAcquireSpinLock(Adapter, RT_SCAN_SPINLOCK);

	bStopScan = pMgntInfo->bCompleteScan;

	//David 2006.05.16, switch band if we need to scan another band
	// TODO: need to consider N only situation if we have n only in the future.
	if(pMgntInfo->bDualModeScanStep == 1 && !pMgntInfo->bCompleteScan)
	{
		pMgntInfo->bDualModeScanStep++;

		MgntConstructAnotherBandScanList(Adapter);

		if(!CustomScan_DualBandScanCb(GET_CUSTOM_SCAN_INFO(Adapter)))
		{
			PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, 0);
			/* 2007/05/04 MH Release spin lock. */
			PlatformReleaseSpinLock(Adapter, RT_SCAN_SPINLOCK);
			return;
		}
	}

	if( pMgntInfo->bCheckScanTime == TRUE )
		pMgntInfo->bCheckScanTime = FALSE;

	// Clear the CustomizedScanRequest 
	// Prefast warning C6011: Dereferencing NULL pointer '((Adapter))->pPortCommonInfo->pDefaultAdapter'.
	if(GetDefaultMgntInfo(Adapter) != NULL &&
		CustomScan_InProgress(GET_CUSTOM_SCAN_INFO(Adapter)))
	{
#if (P2P_SUPPORT==1)
		// Clean the Stop Sending Probe Response Flag ---------------------------------------------
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);
			if(P2P_ENABLED(pP2PInfo))
			{
				pP2PInfo->TimeStartToStopSendingProbeResponse = 0;
			}
		}
		// -----------------------------------------------------------------------------------

		// Start P2P Mgnt Timer -----------------------------------------------------------------	
		{
			PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);
			if(P2P_ENABLED(pP2PInfo))
			{
				PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
				PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);
			}
		}
		// -----------------------------------------------------------------------------------


		// Restart P2PWaitForWpsReadyTimer For Connecting to GO -----------------------------------------------------
		// moved to p2pWaitForWpsReady_CustomScanCb
		// -----------------------------------------------------------------------------------------------------
#endif
	}

	CustomScan_ScanCompleteCb(GET_CUSTOM_SCAN_INFO(Adapter));

	{
		PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);

		while(pLoopAdapter)
		{
			pLoopAdapter->MgntInfo.bScanInProgress = FALSE;
			pLoopAdapter->MgntInfo.bDualModeScanStep = 0;
			
			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}
	
	//sherry added for wlk1.6 VWifiInfraSoftAP_ext
	//20110927
	if(!Adapter->bInHctTest)
		pMgntInfo->bLoadIMRandIQKSettingFor2G = FALSE;
	//Restore previous corresponging DM settings.
	Adapter->HalFunc.ScanOperationBackupHandler(Adapter, SCAN_OPT_RESTORE);

	// In order to prevent to call MgntLinkRetry immediately. Annie, 2005-04-25.
	// Do not change these counters if we are in romaing state. 2005.05.27, by rcnjko.
	if(!MgntRoamingInProgress(pMgntInfo) && 
		(pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod==0 || pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod==0 ) )
	{
		pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod = 1;
		pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod= 1;
	}

	if(!MgntRoamingInProgress(pMgntInfo))
	{
		if((pMgntInfo->mIbss && pMgntInfo->bMediaConnect) || pMgntInfo->mAssoc)
		{
			//xiong:for 8192 usb, need do this? consider it later
			// Set RCR_CBSSID for connected IBSS.
			if(!IsAPModeExist(Adapter))
			{
				{
					bFilterOutNonAssociatedBSSID = TRUE;
					Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
				}
			}		
		}
	}

	// Joseph 20090703: Finish sniffing the packets in the channel.
	if(Adapter->RxStats.CurRxSniffMediaPktCnt != 0)
		Adapter->RxStats.LastRxSniffMediaPktCnt = Adapter->RxStats.CurRxSniffMediaPktCnt;
	
	// In prevent of lossing ping packet during scanning.
	// 2004.12.06, by rcnjko.
	//
	{
		PADAPTER	ptmpAdapter = GetDefaultAdapter(Adapter);
		PMGNT_INFO	ptmpMgntInfo = NULL;
		while(ptmpAdapter != NULL)
		{
			ptmpMgntInfo = &ptmpAdapter->MgntInfo;
			if(	ptmpMgntInfo->mAssoc &&
				!MgntRoamingInProgress(ptmpMgntInfo) && ptmpMgntInfo->bMediaConnect &&
				!ACTING_AS_AP(ptmpAdapter))
			{
				SendNullFunctionData(ptmpAdapter, ptmpMgntInfo->Bssid, IS_WIFI_POWER_SAVE(ptmpMgntInfo) ? TRUE : FALSE); // 1st		
			}
			ptmpAdapter = GetNextExtAdapter(ptmpAdapter);
		}
	}
		
	Hal_PauseTx(Adapter, HW_ENABLE_TX_ALL);

	//
	//   CCX SSIDL to construct NEW BssDesc
	//
	OrgBssMunber = pMgntInfo->NumBssDesc;
	
	if(!Adapter->bInHctTest)
	{
		ScanMergeResult(Adapter, pMgntInfo);
		//3 Construct Scan Period
		RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_CONSTRUCT_SCAN_PERIOD, NULL, NULL);		
	}
	else
	{
		// Update Bssid list for query
		pMgntInfo->NumBssDesc4Query = pMgntInfo->NumBssDesc;
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: ScanComplete(), NumBssDesc4Query=%d\n", pMgntInfo->NumBssDesc4Query));
		for(i = 0; i < pMgntInfo->NumBssDesc4Query; i++)
		{
			CopyWlanBss(pMgntInfo->bssDesc4Query+i, pMgntInfo->bssDesc+i);
		}
	}
	
	// Restore state_Synchronization_Sta parameters --------------------------------------------------------------------------
	{
		PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);

		RT_TRACE(COMP_SCAN, DBG_LOUD,("Restore All Port's pMgntInfo->state_Synchronization_Sta\n"));

		while(pLoopAdapter)
		{
			pLoopAdapter->MgntInfo.state_Synchronization_Sta = pLoopAdapter->MgntInfo.state_Synchronization_Sta_BeforeScan;
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Restore Port (%d) pMgntInfo->state_Synchronization_Sta = %d\n", GET_PORT_NUMBER(pLoopAdapter), pLoopAdapter->MgntInfo.state_Synchronization_Sta));

			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}
	// ----------------------------------------------------------------------------------------------------------------

	pMgntInfo->bScanWithMagicPacket = FALSE; // 2005.06.27, by rcnjko.


	if(IS_DOT11D_ENABLE(pMgntInfo) )
		DOT11D_ScanComplete(Adapter);

	// Save the current time as the last scan complete time.
	MultiportRecordLastScanTime(Adapter);

	//restore for hidden AP scan with 92d dual band. 
//	if(pMgntInfo->bCompleteScan && IS_DUAL_BAND_SUPPORT(Adapter))
	{
		pMgntInfo->bNeedSkipScan = FALSE;	
		pMgntInfo->bCompleteScan = FALSE;		
		pMgntInfo->bScanTimeExceed = FALSE;
	}
	
	// Handle the packet queued during scanning.
#if (MULTICHANNEL_SUPPORT == 1)
//multichannel mechnism sending packets after scan
//by sherry 20130117	

	// Prefast warning C6011 : Dereferencing NULL pointer '((Adapter))->pPortCommonInfo->pDefaultAdapter'
	if(GetDefaultAdapter(Adapter) != NULL && GetDefaultAdapter(Adapter)->bInHctTest)
		MultichannelHandlePacketDuringScan(Adapter,FALSE);
	else
		SendDataFrameQueued(Adapter);
#else
		SendDataFrameQueued(Adapter);
#endif

	MultiChannelScanHandler(Adapter, FALSE, TRUE);

	PlatformReleaseSpinLock(Adapter, RT_SCAN_SPINLOCK);

	{
		// Post process for scan operation. 2006.10.16, by shien chang.
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);		
	}
	
	if(pMgntInfo->bScanOnly == FALSE && bStopScan == FALSE)
	{
		//
		// Select a BSS to join or start if required.
		//
		RT_JOIN_ACTION		join_action = RT_NO_ACTION;
		PRT_WLAN_BSS		pRtBss;
		BOOLEAN 			bRoaming = (pMgntInfo->RoamingType == RT_ROAMING_NORMAL) ? TRUE : FALSE;

		if(MgntResetOrPnPInProgress(Adapter))
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress bScanOnly FALSE\n"));
			return;
		}
		
		PlatformAllocateMemory(Adapter, (PVOID*)&pRtBss, sizeof(RT_WLAN_BSS));
		if(pRtBss != NULL)
		{
			join_action = SelectNetworkBySSID( Adapter, &pMgntInfo->Ssid, bRoaming, pRtBss);
			RT_TRACE(COMP_MLME, DBG_LOUD, ("ScanComplete(): join_action(%d) bIbssStarter(%d)\n", join_action, pMgntInfo->bIbssStarter));
		}
		
		RT_TRACE(COMP_MLME, DBG_LOUD, ("RoamgingType %d, and JoinAction %d\n", pMgntInfo->RoamingType, join_action));	
		// By Bruce, 2008-06-02.
		if (MgntRoamingInProgress(pMgntInfo) && join_action == RT_NO_ACTION)
		{
			// Roaming retry
			if(!MgntRoamRetry(Adapter, FALSE)) // Raom again.
			{
				if(MgntResetOrPnPInProgress(Adapter))
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));				
					if(pRtBss != NULL)
						PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
					
					return ;
				}				
				MgntRoamComplete(Adapter, MlmeStatus_timeout); // Roam failed.
			}
		}
		else if(join_action == RT_NO_ACTION)
		{
			if(!MgntLinkRetry(Adapter, FALSE))
			{
				if(MgntResetOrPnPInProgress(Adapter))
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 3\n"));				
					if(pRtBss != NULL)
						PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
					
					return ;
				}
			
				//
				// Indicate connection complete event if the first connection.
				//
				if(MgntRoamingInProgress(pMgntInfo))
				{
					// Roam failed.
					MgntRoamComplete(Adapter, MlmeStatus_timeout);
				}				
				else if(pMgntInfo->bIndicateConnectEvent)
				{
					RT_OP_MODE	rtOpMode = pMgntInfo->OpMode;
					pMgntInfo->OpMode = RT_OP_MODE_INFRASTRUCTURE;
					DrvIFIndicateConnectionStart(Adapter);
					DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);
					pMgntInfo->OpMode = rtOpMode;
					pMgntInfo->bIndicateConnectEvent = FALSE;
				}
				else
				{
					DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);
				}
				pMgntInfo->state_Synchronization_Sta = STATE_No_Bss;
				Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_NO_LINK);
			}
		}
		else if( !pMgntInfo->bIbssStarter )
		{
			pMgntInfo->JoinAction = join_action;
	//		pMgntInfo->RequestFromUplayer = FALSE;
			JoinRequest(Adapter, join_action, pRtBss);
		}
		if(pRtBss != NULL)
			PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
	}
	else
	{ // scan only.
		{
			CCX_OnScanComplete(Adapter);
		
			BSS_OnScanComplete(Adapter);

			// Check PreAuth state
			if(  pMgntInfo->bMediaConnect && Adapter->bInHctTest ) //(!SecIsTxKeyInstalled( Adapter, pMgntInfo->Bssid ))
			{
				PlatformRequestPreAuthentication(Adapter , PRE_AUTH_INDICATION_REASON_ASSOCIATION);
			}

			// For Auto Select Channel, 2005.12.27, by rcnjko.
			if(ACTING_AS_AP(Adapter) && pMgntInfo->bAutoSelChnl)
			{
				pMgntInfo->Regdot11ChannelNumber = AutoSelectChannel(Adapter, pMgntInfo->bssDesc, pMgntInfo->NumBssDesc);
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("ScanComplete(): AutoSelectChnl: %d\n", pMgntInfo->Regdot11ChannelNumber));
				AP_Reset(Adapter);
			}

		}
	}
	
	CHNL_ReleaseOpLock(Adapter);

	PlatformHandleNLOnScanComplete(Adapter);

	// Prefast warning C6011: Dereferencing NULL pointer '((Adapter))->pPortCommonInfo->pDefaultAdapter'.
	if(GetDefaultMgntInfo(Adapter) != NULL)
		CustomScan_ScanCompleteReturnCb(GET_CUSTOM_SCAN_INFO(Adapter));

	PLATFORM_WRITE_EVENT_LOG(Adapter, RT_SCAN_NUM, (ULONG)pMgntInfo->NumBssDesc4Query);

	RT_TRACE(COMP_SCAN, DBG_TRACE, ("ScanComplete() <==== currentwireless mode 0x%x \n", pMgntInfo->dot11CurrentWirelessMode));
}


VOID
ScanCallback(
	PRT_TIMER		pTimer
	)
{
	PADAPTER					Adapter=(PADAPTER)pTimer->Adapter, pFirstDevicePort = NULL;
	PMGNT_INFO					pMgntInfo=&Adapter->MgntInfo;
	PADAPTER					pDefAdapter = GetDefaultAdapter(Adapter);
	PRT_CHNL_LIST_ENTRY			pChnlListEntry;
	u8Byte	now_time;
	u4Byte	deltatime;

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("==========> scancallback portnumber %d pMgntInfo->ScanStep %d \n", Adapter->pNdis62Common->PortNumber, pMgntInfo->ScanStep));

	// Stop if driver is stopped
	if(RT_DRIVER_STOP(pDefAdapter))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("ScanCallback: bDriverStopped return \n"));
		{
			PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);
		
			while(pLoopAdapter)
			{
				pLoopAdapter->MgntInfo.bScanInProgress = FALSE;
				pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
			}
		}
		return;
	}
	else if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("reset in progress\n"));
		return;
	}
	else if(pMgntInfo->bResetScan)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("bResetScan\n"));
		pMgntInfo->bResetScan = FALSE;
		return;
	}

#if(AUTO_CHNL_SEL_NHM == 1)
	if(IS_AUTO_CHNL_SUPPORT(Adapter)){
		PlatformAcquireSpinLock(Adapter, RT_ACS_SPINLOCK);
		if(IS_AUTO_CHNL_IN_PROGRESS(Adapter))
		{
			PlatformReleaseSpinLock(Adapter, RT_ACS_SPINLOCK);
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("[ACS] ScanCallback: AutoChnlSelCalInProgress...return and wait for a while\n"));		
			PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, 2);
			return;
		}
		else{
			PlatformReleaseSpinLock(Adapter, RT_ACS_SPINLOCK);
		}
	}
#endif

	if(MultiChannel_IsFCSInProgress(Adapter))
	{
		RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("FW is switching channel, waiting for FCS stop!\n"));
		MultiChannelScanHandler(Adapter, TRUE, FALSE);
		PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, 5);
		return;
	}

	if( pMgntInfo->bCheckScanTime == TRUE && Adapter->bScanTimeCheck)	{
		RT_TRACE(COMP_SCAN, DBG_TRACE, ("ScanCallback: bCheckScanTime is TRUE\n"));
		if( pMgntInfo->bScanOnly == TRUE )
		{
			now_time = PlatformGetCurrentTime();
			deltatime = (ULONG)( (now_time - pMgntInfo->ScanOnlyStartTime) / 1000 ); // mini-second

			RT_TRACE(COMP_SCAN, DBG_TRACE, ("ScanCallback: bScanOnly is TRUE %d \n", deltatime));
			if( deltatime >= 2800 )
			{
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("ScanCallback: scan time is over 2800 mini-seconds and indicate scan complete to avoid scan time is too long\n"));
		
				pMgntInfo->bCompleteScan = TRUE;
				pMgntInfo->bScanTimeExceed = TRUE;
				ScanComplete(Adapter);

				// Scan completed. Restore original setting.
				if(	pMgntInfo->SettingBeforeScan.WirelessMode != pMgntInfo->dot11CurrentWirelessMode){
					Adapter->HalFunc.SetWirelessModeHandler(Adapter, (u1Byte)(pMgntInfo->SettingBeforeScan.WirelessMode));
				}

				HAL_ChannelAndBandWidthRecoveryAfterScan(Adapter);
				
				return;
			}
		}
	}	

	// Set this to make sure that scan process and bandwidth switching would not  run at the same time.
	if(pMgntInfo->pChannelInfo->ChnlOp != CHNLOP_SCAN)
	{	
		if(!CHNL_AcquireOpLock(Adapter, CHNLOP_SCAN))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("ScanCallback: CHNLOP_SCAN acquire fail \n"));
		
			PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, 1);
			return;
		}
	}	

	// Wait for previous channel switching complete before starting scan.
	if(RT_IsSwChnlAndBwInProgress(Adapter) || CHNL_SwChnlAndSetBwInProgress(Adapter))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("ScanCallback():Wait for previous channel switching\n"));
		PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, 1);
		return;	
	}

	// Make sure that bandwidth is switching to 20MHz mode
	if(pMgntInfo->bScan_20MHz == FALSE)
	{
		if(HAL_SwitchTo20MHzBeforeScan(Adapter))
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("ScanCallback: HAL_SwitchTo20MHzBeforeScan   \n"));
		
			PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, 1);
			return;
		}
	}

	switch(pMgntInfo->ScanStep)
	{
		case 0:
			// 2012.11.15 Lansin Need consider more. 
			// 1. Shall Send null data after send data to AP
			// 2. Shall not send null data on other channel. 
#if (MULTICHANNEL_SUPPORT ==1 && P2P_SUPPORT == 1) 
			// Disable Tx Data ---------------------------------------------------------------------
			if(Adapter->bInHctTest == TRUE )
			{
				PADAPTER pClientPort = GetFirstClientPort(Adapter);
				PADAPTER pDefaultAdapter = GetDefaultAdapter(Adapter);

				//multichannel disable BE Queue and VI Queue by sherry 20130117
				MultichannelHandlePacketDuringScan(Adapter,TRUE);
			}
			// ----------------------------------------------------------------------------------	
#endif
			{
				BOOLEAN bComplete = !RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_SCAN_CHANNEL, NULL, &pChnlListEntry);

				if(bComplete)
				{
					RT_TRACE(COMP_SCAN, DBG_LOUD, ("channel list empty!! \n"));
					if(ScanSelectAntenna(Adapter))
						return;
				}
				
				if(IS_DUAL_BAND_SUPPORT(Adapter))
					bComplete |= (pMgntInfo->bCompleteScan || Adapter->bDriverIsGoingToUnload || Adapter->bSurpriseRemoved);

				if(bComplete)
				{
					RT_TRACE(COMP_SCAN, DBG_LOUD, ("bCompleteScan %d bDriverIsGoingToUnload %d bSurpriseRemoved %d\n", pMgntInfo->bCompleteScan, Adapter->bDriverIsGoingToUnload, Adapter->bSurpriseRemoved));			
					ScanSwitchComplete(Adapter, pMgntInfo); // Scan completed.	
				}
				else // Switch channel.
				{
					u2Byte extendedDwellTime = 0;

					if(0 != (extendedDwellTime = CustomScan_PreSwChnlCb(GET_CUSTOM_SCAN_INFO(Adapter), pChnlListEntry)))
					{
						PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, extendedDwellTime);
						return;
					}

#if(AUTO_CHNL_SEL_NHM == 1)
					if(IS_AUTO_CHNL_SUPPORT(Adapter))
					{
						RT_TRACE(COMP_SCAN, DBG_LOUD, ("[ACS] ScanCallback(): (1) RT_GetChannelNumber(%d), AUTO_CHNL_STATE(%d)\n", RT_GetChannelNumber(pDefAdapter),GET_AUTO_CHNL_STATE(Adapter)));

						PlatformAcquireSpinLock(Adapter, RT_ACS_SPINLOCK);
						if((MgntLinkStatusQuery(pDefAdapter) == RT_MEDIA_DISCONNECT) && 
							P2PIsSocialChannel(RT_GetChannelNumber(pDefAdapter)) && 
							(GET_AUTO_CHNL_STATE(pDefAdapter) == ACS_BEFORE_NHM))
						{
							PlatformReleaseSpinLock(Adapter, RT_ACS_SPINLOCK);

							RT_TRACE(COMP_SCAN, DBG_LOUD, ("[ACS] ScanCallback(): (2) RT_GetChannelNumber(%d)\n", RT_GetChannelNumber(pDefAdapter)));
							if( PlatformScheduleWorkItem( &(pMgntInfo->AutoChnlSel.AutoChnlSelWorkitem)))
							{
								SET_AUTO_CHNL_PROGRESS(Adapter, TRUE);
								SET_AUTO_CHNL_STATE(Adapter, ACS_IN_NHM);// ACS is Calculating now by NHM measurement	
	
								RT_TRACE(COMP_SCAN, DBG_LOUD, ("[ACS] ScanCallback: Defer for NHM measurement in another WI Callback...\n"));
								PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, 5);
								return;
							}						
						}
						else{
							PlatformReleaseSpinLock(Adapter, RT_ACS_SPINLOCK);
						}	
					}
#endif									
					ScanSwitchChannel(Adapter, pMgntInfo, pChnlListEntry);		
				}
			}
			break;

		case 1:
			RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_POP_SCAN_CHANNEL, NULL, &pChnlListEntry);

			if(!pChnlListEntry)
			{ // Scan completed.
				ScanComplete(Adapter);
				return;
			}

			// Add 30 ms for Stopping Sending Probe Response -----------------------------------------------------------------------------------
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Cusstomized scan enabled: %d\n", CustomScan_InProgress(GET_CUSTOM_SCAN_INFO(Adapter))));

			pFirstDevicePort = GetFirstDevicePort(Adapter);
			if(CustomScan_InProgress(GET_CUSTOM_SCAN_INFO(Adapter)) && (NULL != pFirstDevicePort))
			{
				PRT_CHNL_LIST_ENTRY	pNextChnlListEntry = NULL;
#if P2P_SUPPORT == 1
				PP2P_INFO pDevicePortP2PInfo = GET_P2P_INFO(pFirstDevicePort);
#endif

				RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_SCAN_CHANNEL, NULL, &pNextChnlListEntry);

				//RT_TRACE(COMP_P2P, DBG_LOUD, ("pNextChnlListEntry: %d\n", pNextChnlListEntry));
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("pChnlListEntry->ChannelNum: %d\n", pChnlListEntry->ChannelNum));

				if(pNextChnlListEntry)
				{
					RT_TRACE(COMP_SCAN, DBG_LOUD, ("pNextChnlListEntry->ChannelNum: %d\n",  pNextChnlListEntry->ChannelNum));
				}
				
#if P2P_SUPPORT == 1			
				if(pNextChnlListEntry == NULL || pChnlListEntry->ChannelNum != pNextChnlListEntry->ChannelNum)
					pDevicePortP2PInfo->TimeStartToStopSendingProbeResponse = PlatformGetCurrentTime() + pChnlListEntry->ScanPeriod * 1000 - P2P_RESERVED_TIME_FOR_ACTION_FRAME_MS * 1000;
				else
					pDevicePortP2PInfo->TimeStartToStopSendingProbeResponse = 0;

				//RT_TRACE(COMP_SCAN, DBG_LOUD, ("pDevicePortP2PInfo->TimeStartToStopSendingProbeResponse: %lld\n", pDevicePortP2PInfo->TimeStartToStopSendingProbeResponse));
#endif
			}
			// ------------------------------------------------------------------------------------------------------------------------------------------------------------

			CustomScan_OnChnlCb(GET_CUSTOM_SCAN_INFO(Adapter), pChnlListEntry);

			// Send probe request if active scan is used.
			if((pMgntInfo->bActiveScan) &&  bActiveScan(pMgntInfo, pChnlListEntry))
			{
				u1Byte		i;

				for(i = 0; i < 2; i++)
				{
					if(ScanSendProbeReq(Adapter, i, pChnlListEntry) == FALSE)
						break;
					if (P2P_ScanCallback(Adapter) == FALSE)
						break;
				}
			}

			// In prevent of lossing ping packet during scanning.
			// Since we've send notification to AP, it is not necessary to
			// switch back to original channel. 2004.12.06, by rcnjko.
			pMgntInfo->ScanStep = 0;		
			//---------------------------------------------------------------------------
	
			{
#if( MULTICHANNEL_SUPPORT ==1 && P2P_SUPPORT == 1) //add by ylb 201201029 ,WIN8 WHCK WFD Invitaion After Before Test: DUT will connect to AP after WFD connected, it will Send Disassotiate to SUT, SUT will roaming and do scan, but it still connect to AP, that scan process result in Packet loss or long Lantency between SUT and AP, So we send Packet in each scan step
				if(Adapter->bInHctTest == TRUE )
				{
					// Send packet when switch to connected chanel
					//by sherry 20130117
					if(pChnlListEntry->ExInfo & CHANNEL_EXINFO_CONNECTED_CHANNEL)
						MultichannelHandlePacketDuringScan(Adapter,FALSE);
					// -------------------------------------------------------------------------------------------------------------------
				}
#endif

#if(AUTO_CHNL_SEL_NHM == 1)
				if(IS_AUTO_CHNL_SUPPORT(Adapter))
				{	
					SET_AUTO_CHNL_SCAN_PERIOD(Adapter, pChnlListEntry->ScanPeriod);
					
					RT_TRACE(COMP_SCAN, DBG_WARNING, ("[ACS] ScanCallback(): Current channel(%d) ScanPeriod(%d)!!!\n", 
							pChnlListEntry->ChannelNum, Adapter->MgntInfo.AutoChnlSel.AutoChnlSelPeriod));
				}
#endif

				if(CustomScan_PreSetDwellTimerCb(GET_CUSTOM_SCAN_INFO(Adapter), pChnlListEntry))
				{
					PlatformSetTimer(Adapter, &pMgntInfo->ScanTimer, pChnlListEntry->ScanPeriod);
				}
			}
			break;

		case 2:
			ScanComplete(Adapter);
			break;
			
		default:
			RT_TRACE(COMP_SCAN, DBG_WARNING, ("ScanCallback(): Unexpected ScanStep(%d)!!!\n", pMgntInfo->ScanStep));
			break;
	}

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("<=========== scancallback portnumber %d\n", Adapter->pNdis62Common->PortNumber));
}


//
//	Description:
//		Send out data frames queued.
//	2006.04.28, by rcnjko.
//
VOID
SendDataFrameQueued(
	PADAPTER	Adapter
	)
{
	u1Byte		QueueID;
	s1Byte		Counter = 14;
	RT_STATUS	rtStatus;
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	for(QueueID = 0; QueueID < MAX_TX_QUEUE; QueueID++)
	{
		if(QueueID == BEACON_QUEUE)
			continue;

		while( !RTIsListEmpty(Get_WAIT_QUEUE(Adapter,QueueID)) && (Counter-- >0))
		{
			PRT_TCB			pTcb;

			pTcb=(PRT_TCB)RTRemoveHeadListWithCnt(Get_WAIT_QUEUE(Adapter,QueueID),GET_WAIT_QUEUE_CNT(Adapter,QueueID));

			rtStatus = TransmitTCB(Adapter, pTcb);
			if(RT_STATUS_SUCCESS != rtStatus && RT_STATUS_PKT_DROP != rtStatus)
			{
				RTInsertHeadListWithCnt(Get_WAIT_QUEUE(Adapter,QueueID), &pTcb->List,GET_WAIT_QUEUE_CNT(Adapter,QueueID));
				break;
			}
		}	
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}


VOID
MgntResetScanProcess(
	PADAPTER		pAdapter
)
{
	PADAPTER	Adapter = GetDefaultAdapter(pAdapter);
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	CHANNEL_WIDTH 	bw;
	EXTCHNL_OFFSET 	extchnl20MHz,extchnl40MHz;
	u1Byte			centerfrequencyindex1;
	PADAPTER 	pLoopAdapter = GetDefaultAdapter(Adapter);
	PRT_CHANNEL_INFO		pChnlInfo;
	BOOLEAN		bAdapterIsNull = FALSE;

	if(pAdapter->pNdis62Common)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("MgntResetScanProcess(): padapter port number %d \n", pAdapter->pNdis62Common->PortNumber));
	}
	
#if (MULTICHANNEL_SUPPORT == 1)
	MultiChannelUpdateSettingBeforeScanOnResetRequest(pAdapter);

	RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("pMgntInfo->SettingBeforeScan.ChannelNumber: %d\n", pMgntInfo->SettingBeforeScan.ChannelNumber));
	RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("pMgntInfo->SettingBeforeScan.ChannelBandwidth: %d\n", pMgntInfo->SettingBeforeScan.ChannelBandwidth));
	RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz: %d\n", pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz));
#else
	if(MgntLinkStatusQuery(Adapter) == RT_MEDIA_CONNECT)
	{
		bw = RT_GetBandWidth(Adapter);
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_BW40MHZ_EXTCHNL, (pu1Byte)(&extchnl20MHz));
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_BW80MHZ_EXTCHNL, (pu1Byte)(&extchnl40MHz));
		centerfrequencyindex1 = RT_GetCenter_Frequency_Index1(Adapter);

		pMgntInfo->SettingBeforeScan.ChannelNumber = pMgntInfo->dot11CurrentChannelNumber;
		pMgntInfo->SettingBeforeScan.ChannelBandwidth = bw;
		pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz = extchnl20MHz;
		pMgntInfo->SettingBeforeScan.Ext40MHzChnlOffsetOf80MHz = extchnl40MHz;
		pMgntInfo->SettingBeforeScan.CenterFrequencyIndex1 = CHNL_GetCenterFrequency(pMgntInfo->SettingBeforeScan.ChannelNumber, bw, extchnl20MHz);
	}
#endif

	if( pMgntInfo->bCheckScanTime == TRUE )
		pMgntInfo->bCheckScanTime = FALSE;
	
	if(!pMgntInfo->bScanInProgress && pMgntInfo->bDualModeScanStep ==0) 
	{
		if(	(IsAPModeExist(Adapter) == FALSE) &&
			(MgntLinkStatusQuery(Adapter) == RT_MEDIA_CONNECT || MgntLinkStatusQuery(GetFirstClientPort(pAdapter)) == RT_MEDIA_CONNECT)	)
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("ChangeWirelessModeHandler\n"));
		
			HalChangeWirelessMode(Adapter, pMgntInfo->SettingBeforeScan.ChannelNumber);
		
			MgntActSet_802_11_CHANNEL_AND_BANDWIDTH(
				Adapter, 
				pMgntInfo->SettingBeforeScan.ChannelNumber, 
				pMgntInfo->SettingBeforeScan.ChannelBandwidth, 
				pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz, 
				pMgntInfo->SettingBeforeScan.Ext40MHzChnlOffsetOf80MHz, 
				0
				);
		}	
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
	}
	else
	{
		Adapter->HalFunc.CancelAllTimerHandler(Adapter);
		RT_ResetSwChnlProgress(Adapter);

		if(!Adapter->bInHctTest)
			pMgntInfo->bLoadIMRandIQKSettingFor2G = FALSE;

		while(pLoopAdapter)
		{
			pMgntInfo = &pLoopAdapter->MgntInfo;
			pChnlInfo = pMgntInfo->pChannelInfo;
			if(pChnlInfo->ChnlOp == CHNLOP_SCAN)
				break;
			else			
				pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}

		if(pLoopAdapter == NULL || ((pLoopAdapter == Adapter) && (pLoopAdapter->MgntInfo.pChannelInfo->ChnlOp == CHNLOP_NONE)))
		{	
	//		RT_ASSERT(FALSE, ("pLoopAdapter is NULL\n"));
			bAdapterIsNull = TRUE;
			pLoopAdapter = Adapter;
			pMgntInfo = &pLoopAdapter->MgntInfo;
		}

		{
			if(!bAdapterIsNull)
			{
				if(pLoopAdapter->pNdis62Common)
					RT_TRACE(COMP_SCAN, DBG_LOUD, ("MgntResetScanProcess reset scan process port number %d!!!\n", pLoopAdapter->pNdis62Common->PortNumber));

				pLoopAdapter->HalFunc.ScanOperationBackupHandler(pLoopAdapter, SCAN_OPT_RESTORE);
				if(!PlatformCancelTimer(pLoopAdapter, &(pMgntInfo->ScanTimer)))
				{
					pLoopAdapter->MgntInfo.bResetScan = TRUE;
					RT_TRACE(COMP_SCAN, DBG_LOUD, ("MgntResetScanProcess cancel scantimer FAIL!!!\n"));					
				}
				else
				{
					pLoopAdapter->MgntInfo.bResetScan = FALSE;				
				}
				
				PlatformHandleNLOnScanCancel(pLoopAdapter);
				Hal_PauseTx(pLoopAdapter, HW_ENABLE_TX_ALL);			
				CHNL_ReleaseOpLock(pLoopAdapter);
			}

			//
			// <Roger_Notes> Update latest BSS list for query immediately if scan process is broken by OID_DOT11_RESET_REQUEST. 
			// The reason why we need to do this is because WLAN AutoCfg would try to prepare connection request after radio status 
			// was changed from off to on. At the same time, OID_DOT11_RESET_REQUEST would be scheduled to break this scan process.
			// 2014.03.06.
			//
			if(!Adapter->bInHctTest)
				ScanMergeResult(Adapter, pMgntInfo);// ScanComplete shall be called in this function instead, so fix me!!	
				
			//
			// <Roger_Notes> We should indicate failure status for current scan requirement when driver is going to suspend, 
			// otherwise wireless icon will be red X in resumption flow.
			// 2013.12.26.
			//
			if(pMgntInfo->bDriverIsGoingToSleep)
				DrvIFIndicateScanComplete(pLoopAdapter, RT_STATUS_FAILURE);
			else
				DrvIFIndicateScanComplete(pLoopAdapter, RT_STATUS_SUCCESS);
			

			{ // Reset ScanProcess --------------------------------------------------
				pLoopAdapter = GetDefaultAdapter(Adapter);

				while(pLoopAdapter)
				{
					pLoopAdapter->MgntInfo.state_Synchronization_Sta = pLoopAdapter->MgntInfo.state_Synchronization_Sta_BeforeScan;

					pLoopAdapter->MgntInfo.bScanInProgress = FALSE;
					pLoopAdapter->MgntInfo.ScanStep = 0;
//				pLoopAdapter->MgntInfo.RequestFromUplayer = FALSE;
					pLoopAdapter->MgntInfo.bDualModeScanStep = 0;
					pLoopAdapter->MgntInfo.bScanOnly = TRUE;					
					//pLoopAdapter->MgntInfo.bCompleteScan = TRUE;
					pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
				}
			} // -----------------------------------------------------------------
		}
	}

	CustomScan_ScanResetCb(GET_CUSTOM_SCAN_INFO(pAdapter));

	FunctionOut(COMP_SCAN);
	
}


VOID
MgntResetLinkProcess(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	u1Byte			i;

	FunctionIn(COMP_MLME);

	MgntCancelAllTimer( Adapter	);

	pMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
	pMgntInfo->AuthStatus = AUTH_STATUS_IDLE;
	pMgntInfo->mDisable = TRUE;
	pMgntInfo->mAssoc = FALSE;
	pMgntInfo->mIbss = FALSE;
	pMgntInfo->bMlmeStartReqRsn = MLMESTARTREQ_NONE;
	pMgntInfo->bJoinInProgress = FALSE;
	pMgntInfo->bScanOnly = TRUE;
//	MgntResetRoamingState(pMgntInfo);	
	pMgntInfo->state_Synchronization_Sta = STATE_No_Bss;
	MgntResetJoinCounter(pMgntInfo);

	MgntLinkStatusResetRxBeacon(Adapter);

	for( i=0; i<RT_MAX_LD_SLOT_NUM; i++ ){
		pMgntInfo->LinkDetectInfo.RxBcnNum[i] = 0;
	}
	
	// Half the time required to determine if we are disconneted. 2005.03.10, by rcnjko.
	//Adapter->MgntInfo.LinkDetectInfo.SlotNum = 5;
	pMgntInfo->LinkDetectInfo.SlotNum = 2;

	pMgntInfo->LinkDetectInfo.SlotIndex = 0;

	FunctionOut(COMP_MLME);
}

VOID
MgntResetJoinProcess(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;

	FunctionIn(COMP_MLME);

	if (pMgntInfo->state_Synchronization_Sta == STATE_Join_Wait_Beacon)
	{
		PlatformCancelTimer(Adapter, &pMgntInfo->JoinTimer);		
		PlatformCancelTimer(Adapter, &pMgntInfo->JoinConfirmTimer );
		PlatformCancelTimer(Adapter, &pMgntInfo->JoinProbeReqTimer );
		pMgntInfo->state_Synchronization_Sta = STATE_No_Bss;
		pMgntInfo->JoinRetryCount = 0;
	}
	
	if (pMgntInfo->State_AuthReqService != STATE_Auth_Req_Idle)
	{
		PlatformCancelTimer( Adapter, &pMgntInfo->AuthTimer );
		pMgntInfo->State_AuthReqService = STATE_Auth_Req_Idle;
		pMgntInfo->AuthRetryCount = 0;
	}
	if (pMgntInfo->State_AsocService != STATE_Asoc_Idle)
	{
		PlatformCancelTimer( Adapter, &pMgntInfo->AsocTimer );
		pMgntInfo->State_AsocService = STATE_Asoc_Idle;
		pMgntInfo->AsocRetryCount = 0;
	}

	FunctionOut(COMP_MLME);
}

BOOLEAN
MgntLinkRetry(
	PADAPTER		Adapter,
	BOOLEAN			bForceRetry
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	PADAPTER        DefAdapter = GetDefaultAdapter(Adapter);
	PADAPTER		pDevicePort = NULL;
	PP2P_INFO		pP2PInfo = NULL;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	if(pMgntInfo->bSetPnpPwrInProgress)
		return FALSE;

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}

#if P2P_SUPPORT == 1
	// Only scan in the idle device port ----------------------------
	pDevicePort = GetFirstDevicePort(Adapter);
	
	if(pDevicePort != NULL)
	{
		pP2PInfo = GET_P2P_INFO(pDevicePort);
		
		if((pP2PInfo->State != P2P_STATE_INITIALIZED) || pP2PInfo->bDeviceDiscoveryInProgress)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: NDIS_STATUS_DOT11_MEDIA_IN_USE\n", __FUNCTION__));
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("%s: pP2PInfo->State: %d bDeviceDiscoveryInProgress %d\n", __FUNCTION__, pP2PInfo->State, pP2PInfo->bDeviceDiscoveryInProgress));
			return FALSE;
		}	
	}
#endif

#if(MULTICHANNEL_SUPPORT ==1)
	if(DefAdapter->MgntInfo.RegMultiChannelFcsMode == 0)
	{
		if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
	
		{
			PP2P_INFO	    pP2PInfoTmp = (PP2P_INFO)pMgntInfo->pP2PInfo;
			if(pP2PInfoTmp->Role == P2P_CLIENT &&MgntLinkStatusQuery(DefAdapter) == RT_MEDIA_CONNECT )
				MultiChannelHandleChannelSwitchToCurrentPort(DefAdapter, Adapter);
		}
	}
	else
		RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("Stop FW switch channel, temporary do nothing!\n"));
#endif
	//
	// If the media is disconnected, and the OS set the dummy (non-sense SSID), then the OS jsut wants
	// the driver to be disconnected. When we have been disconnected, do not retry so that we can save
	// scan operations. By Bruce, 2008-12-30.
	//
	if(IsSSIDDummy(pMgntInfo->Ssid))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkRetry(): Dummy SSID! Not to retry!\n") );
		return FALSE;
	}

	if(	bForceRetry	||
		((pMgntInfo->Regdot11networktype != RT_JOIN_NETWORKTYPE_ADHOC||Adapter->bInHctTest)
		&& pMgntInfo->JoinRetryCount < MAX_JOIN_RETRY_COUNT &&
		!MgntIsTimeOutForIndication(pMgntInfo))	
		)
	{
		if(bForceRetry)
			pMgntInfo->JoinRetryCount=MAX_JOIN_RETRY_COUNT;
		else
			pMgntInfo->JoinRetryCount++;

		if(pMgntInfo->RequestFromUplayer)
		{
//			pMgntInfo->RequestFromUplayer = FALSE;
			if(!Adapter->bInHctTest)
			pMgntInfo->JoinRetryCount=MAX_JOIN_RETRY_COUNT;
		}

		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkRetry NEW!!(): bForceRetry(%d) RequestFromUplayer (%d) JoinRetryCount(%d)\n", bForceRetry, pMgntInfo->RequestFromUplayer, pMgntInfo->JoinRetryCount));

		if (!MgntScanInProgress(pMgntInfo) &&
			!((pMgntInfo->bScanInProgress && !pMgntInfo->bScanOnly)	||	\
			(pMgntInfo->JoinTimer.Status&RT_TIMER_STATUS_SET)			||	\
			(pMgntInfo->JoinConfirmTimer.Status&RT_TIMER_STATUS_SET)			||	\
			(pMgntInfo->AuthTimer.Status&RT_TIMER_STATUS_SET)			||	\
			(pMgntInfo->AsocTimer.Status&RT_TIMER_STATUS_SET)))
		{
			MgntLinkRequest(
					Adapter,
					FALSE,	//bScanOnly
					TRUE,	//bActiveScan,
					pMgntInfo->bHiddenSSIDEnable, //TRUE,	//FilterHiddenAP
					FALSE,	// Update parameters
					NULL,	//ssid2scan
					0,		//NetworkType,
					0,		//ChannelNumber,
					0,		//BcnPeriod,
					0,		//DtimPeriod,
					0,		//mCap,
					NULL,	//SuppRateSet,
					NULL	//yIbpm,
					);
		}
		else
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkRetry(): scan is in progress.\n"));
			pMgntInfo->bScanOnly = FALSE;
		}
	
		
		return TRUE;
	}

	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkRetry(): return FALSE for JoinRetryCount(%d) >= MAX_JOIN_RETRY_COUNT(%d)\n", pMgntInfo->JoinRetryCount, MAX_JOIN_RETRY_COUNT));

	return FALSE;
}

//
// Description:
//	Retry to roam to the other AP. Or it may roam back to the original associated AP.
// Arguments:
//	Adapter -
//		NIC adapter context pointer.
//	bForceRetry -
//		Be forced to roam no matter the roaming fail count.
// Retrun Value:
//	TRUE -
//		Roaming action is invoked.
//	FALSE -
//		No retry roaming.
// Note:
//	This function named "MgntRoamRetry" does not only mean 802.11 roaming but also
//	includes link retry using the "roaming event under Ndis6 or later" to meet DTM or 
//	customized request.
//	So when calling this function, the driver whould not be sure to send reassociation packets.
// By Bruce, 2009-02-12.
//
BOOLEAN
MgntRoamRetry(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN 		bForceRetry
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	BOOLEAN			bRetry = FALSE;
	PRT_WLAN_BSS	pRtBss = NULL;
	RT_JOIN_ACTION	join_action = RT_NO_ACTION;

	// Reset Join Retry count to re-join for roaming.
	pMgntInfo->JoinRetryCount = 0;
	pMgntInfo->AsocRetryCount = 0;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntRoamRetry(): Roaming Type = %d, RoamingFailCount = %d (Limit = %d)\n", 
		pMgntInfo->RoamingType, pMgntInfo->RoamingFailCount, pMgntInfo->RegRoamingLimitCount));

#if 0
	if(MgntIsLinkInProgress(pMgntInfo)||  // Case 1 : we in link process so don't it !!
	(MgntScanInProgress(pMgntInfo)&& pMgntInfo->bScanOnly == FALSE)) // Case 2 : we in link after scan , so we do no thing !!
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("RETURN!! bScanInProgress %d bScanOnly %d JoinTimer.Status 0x%x JoinConfirmTimer.Status 0x%x JoinProbeReqTimer.Status 0x%x AuthTimer.Status 0x%x AsocTimer.Status 0x%x bJoinInProgress %d\n", pMgntInfo->bScanInProgress, pMgntInfo->bScanOnly, pMgntInfo->JoinTimer.Status, pMgntInfo->JoinConfirmTimer.Status, pMgntInfo->JoinProbeReqTimer.Status, pMgntInfo->AuthTimer.Status, pMgntInfo->AsocTimer.Status, pMgntInfo->bJoinInProgress  ));
	
		return TRUE;
	}
#endif
	
	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("bResetInProgress\n"));
		return FALSE;
	}

	if(MgntIsTimeOutForIndication(pMgntInfo))
		return FALSE;
	
	pMgntInfo->RequestFromUplayer = FALSE;	

	if(PlatformAllocateMemory(Adapter, (PVOID*)&pRtBss, sizeof(RT_WLAN_BSS))  != RT_STATUS_SUCCESS)		
		return FALSE;
	
	if((pMgntInfo->RoamingFailCount > pMgntInfo->RegRoamingLimitCount + 
		((pMgntInfo->RoamingType == RT_ROAMING_BY_SLEEP) ? 1 : 0)) // Wake up from sleep, more time to raom.
		&& !bForceRetry)
	{ // Exceeds Roaming Limit, do not try.
		if(pMgntInfo->RoamingType != RT_ROAMING_NONE)
		{
			while(GetDesiredSSIDList(Adapter))
			{			
				join_action = SelectNetworkBySSID(Adapter, &(pMgntInfo->Ssid), TRUE, pRtBss);
				//
				// Select a different AP to roam.
				//
				if(join_action == RT_JOIN_INFRA)
				{ // An BSS has been selected.
					RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "MgntRoamRetry(): BSS to roam", pRtBss->bdBssIdBuf);			
					pMgntInfo->RoamingState = ROAMINGSTATE_SCANNING;
					JoinRequest(Adapter, RT_JOIN_INFRA, pRtBss);
					bRetry = TRUE;
					break;
				}			
			}

			if(join_action == RT_NO_ACTION)
				bRetry = FALSE;
		}
		else
		{
			bRetry = FALSE;
		}
	}
	else if(pMgntInfo->RoamingType == RT_ROAMING_NONE)
	{
		bRetry = FALSE;
	}
	else if(pMgntInfo->RoamingType == RT_ROAMING_BY_DEAUTH ||
		pMgntInfo->RoamingType == RT_ROAMING_BY_SLEEP)
	{ // Try to link to the original AP.
		if(MgntLinkRetry(Adapter, FALSE))
			bRetry = TRUE;
		else
			bRetry = FALSE;
	}
	else
	{ // Roam to the other AP.
		if(pRtBss != NULL)
			join_action = SelectNetworkBySSID(Adapter, &(pMgntInfo->Ssid), TRUE, pRtBss);
		//
		// Select a different AP to roam.
		//
		if(join_action == RT_JOIN_INFRA)
		{ // An BSS has been selected.
			RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "MgntRoamRetry(): BSS to roam", pRtBss->bdBssIdBuf);			
			pMgntInfo->RoamingState = ROAMINGSTATE_SCANNING;
			JoinRequest(Adapter, RT_JOIN_INFRA, pRtBss);
			bRetry = TRUE;
		}
		else
		{ // No another AP can be selected, try to scan and to re-do join action.
			if(MgntLinkRetry(Adapter, FALSE))
				bRetry = TRUE;
			else
				bRetry = FALSE;
		}
	}
		
	if(pRtBss != NULL)
		PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));

	FunctionOut(COMP_MLME);

	return bRetry;
}


/*
  * MgntLinkRequestReschedule: 
  *			Check wheter reschedule of JoinRequest is required if RF is in OFF state
  *			<Step 1> Check whether reschedule is required
  *			<Step 2> If reschedule is required, Turn on RF, Save required parameter and 
  *					 reschedule for MgntLinkRequestReschedule
  *	2008/01/03 Emily
  */
BOOLEAN
MgntLinkRequestReschedule(
	PADAPTER			Adapter,
	BOOLEAN        		bScanOnly,
	BOOLEAN         	bActiveScan,
	BOOLEAN        		bFilterHiddenAP,
	BOOLEAN				bUpdateParms,
	POCTET_STRING		pSsid2scan,
	u1Byte          	NetworkType,
	u1Byte          	ChannelNumber,
	u2Byte          	BcnPeriod,
	u1Byte          	DtimPeriod,
	u2Byte          	mCap,
	POCTET_STRING		pSuppRateSet,
	PIbssParms       	pIbpm
)
{
	PMGNT_INFO					pMgntInfo=&Adapter->MgntInfo;
	PMGNT_INFO      			pDefaultMgntInfo = &GetDefaultAdapter(Adapter)->MgntInfo;
	PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(pDefaultMgntInfo);
	RT_RF_POWER_STATE 			rtState;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return TRUE;		
	}

	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));	
	if (rtState == eRfOff/* || rtState ==eRfSleep*/)
	{
		//
		// Check if in inactive power save mode. If yes, we should turn on rf and continue scan.
		// 2007.07.16, by shien chang.
		//
		if (pMgntInfo->RfOffReason > RF_CHANGE_BY_IPS)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("MgntLinkRequestReschedule(): Dont scan beacuse RF is OFF.\n"));
			if(bScanOnly)
			{
				if(MgntScanInProgress(pMgntInfo))			
					MgntResetScanProcess( Adapter );
			}
			else
			{		
				DrvIFIndicateScanStart( Adapter);
				DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
				
				if(MgntResetOrPnPInProgress(Adapter))
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));				
					return TRUE;
				}
				
				if(MgntRoamingInProgress(pMgntInfo))
				{
					MgntRoamComplete(Adapter, MlmeStatus_invalid) ;
				}	
				else if (pMgntInfo->RequestFromUplayer)
				{
					DrvIFIndicateConnectionStart( Adapter);
					DrvIFIndicateConnectionComplete( Adapter,RT_STATUS_FAILURE);
				}	
				MgntResetLinkProcess(Adapter);
			}
			return TRUE;
		}
		else  if (!pMgntInfo->bSetPnpPwrInProgress)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("MgntLinkRequestReschedule(): Under inactive ps mode, turn on RF and continue.\n"));
			pPSC->bTmpScanOnly = bScanOnly;
			pPSC->bTmpActiveScan = bActiveScan;
			pPSC->bTmpFilterHiddenAP = bFilterHiddenAP;
			pPSC->bTmpUpdateParms = bUpdateParms;
			pPSC->ptmpAdapter = Adapter;
			if (pSsid2scan == NULL)
			{
				pPSC->bTmpSsid2Scan = FALSE;
			}
			else
			{
				pPSC->bTmpSsid2Scan = TRUE;
				FillOctetString(pPSC->tmpSsid2Scan, 
								pPSC->tmpSsidBuf, 
								sizeof(pPSC->tmpSsidBuf) );
				CopyMemOS(&(pPSC->tmpSsid2Scan),
							*pSsid2scan, pSsid2scan->Length);
			}
			pPSC->tmpNetworkType = NetworkType;
			pPSC->tmpChannelNumber = ChannelNumber;
			pPSC->tmpBcnPeriod = BcnPeriod;
			pPSC->tmpDtimPeriod = DtimPeriod;
			pPSC->tmpmCap = mCap;
			if (pSuppRateSet == NULL)
			{
				pPSC->bTmpSuppRate = FALSE;
			}
			else
			{
				pPSC->bTmpSuppRate = TRUE;
				FillOctetString(pPSC->tmpSuppRateSet, 
								pPSC->tmpSuppRateBuf, 
								sizeof(pPSC->tmpSuppRateBuf) );
				CopyMemOS(&(pPSC->tmpSuppRateSet),
							*pSuppRateSet, pSuppRateSet->Length);
			}
			if (pIbpm == NULL)
			{
				pPSC->bTmpIbpm = FALSE;
			}
			else
			{
				pPSC->bTmpIbpm = TRUE;
				PlatformMoveMemory(&(pPSC->tmpIbpm), pIbpm, sizeof(IbssParms));
			}
			pPSC->ReturnPoint = IPS_CALLBACK_MGNT_LINK_REQUEST; // MgntLinkRequest
			if(rtState == eRfOff && pMgntInfo->RfOffReason == RF_CHANGE_BY_IPS)
			{
				IPSLeave(Adapter,FALSE);
				PlatformSetTimer(Adapter, &pPSC->InactivePsTimer, SWRF_TIMEOUT);
			}
			else if(pPSC->bSwRfProcessing)
			{
				IPSLeave(Adapter,FALSE);
				PlatformSetTimer(Adapter, &pPSC->InactivePsTimer, SWRF_TIMEOUT);
			}

			CustomScan_RescheduleCb(GET_CUSTOM_SCAN_INFO(Adapter));

			RT_TRACE(COMP_SCAN, DBG_LOUD, ("<=== MgntLinkRequestReschedule()\n"));
			return TRUE;

		}
	}
	return TRUE;
}



// <1>ScanByTimer
// <2>JoinRequest
BOOLEAN
MgntLinkRequest(
	PADAPTER		Adapter,
	BOOLEAN        	bScanOnly,
	BOOLEAN         bActiveScan,
	BOOLEAN        	bFilterHiddenAP,
	BOOLEAN			bUpdateParms,
	POCTET_STRING	pSsid2scan,
	u1Byte          NetworkType,
	u1Byte          ChannelNumber,
	u2Byte          BcnPeriod,
	u1Byte          DtimPeriod,
	u2Byte          mCap,
	POCTET_STRING	pSuppRateSet,
	PIbssParms      pIbpm
)
{
	HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(Adapter);
	PMGNT_INFO				pMgntInfo=&Adapter->MgntInfo;
	u2Byte					SupportedWirelessMode;
	RT_RF_POWER_STATE 		rtState;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	BOOLEAN 				bIQKInProgress = FALSE;

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("MAC %d ===> MgntLinkRequest() port number %d scanonly %d\n",Adapter->interfaceIndex, Adapter->pNdis62Common->PortNumber, bScanOnly));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}	

	//2008.12.31 Added by tynli
	if (pPSC->bSwRfProcessing) 
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("<=== MgntLinkRequest(): Rf is in switching state.\n"));
		//2009.01.01 Added by tynli to fix the race condision problem when IPS is truning RF to off.
		//1.Store the Link information
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));
		
		if ((rtState == eRfOff) && (pPSC->BufConnectinfoBefore == FALSE))
		{
			pPSC->bTmpScanOnly = bScanOnly;
			if(!bScanOnly)
				pPSC->BufConnectinfoBefore = TRUE;
			pPSC->bTmpActiveScan = bActiveScan;
			pPSC->bTmpFilterHiddenAP = bFilterHiddenAP;
			pPSC->bTmpUpdateParms = bUpdateParms;
			if (pSsid2scan == NULL)
			{
				pPSC->bTmpSsid2Scan = FALSE;
			}
			else
			{
				pPSC->bTmpSsid2Scan = TRUE;
				FillOctetString(pPSC->tmpSsid2Scan, 
								pPSC->tmpSsidBuf, 
								sizeof(pPSC->tmpSsidBuf) );
				CopyMemOS(&(pPSC->tmpSsid2Scan),
							*pSsid2scan, pSsid2scan->Length);
			}
			pPSC->tmpNetworkType = NetworkType;
			pPSC->tmpChannelNumber = ChannelNumber;
			pPSC->tmpBcnPeriod = BcnPeriod;
			pPSC->tmpDtimPeriod = DtimPeriod;
			pPSC->tmpmCap = mCap;
			if (pSuppRateSet == NULL)
			{
				pPSC->bTmpSuppRate = FALSE;
			}
			else
			{
				pPSC->bTmpSuppRate = TRUE;
				FillOctetString(pPSC->tmpSuppRateSet, 
								pPSC->tmpSuppRateBuf, 
								sizeof(pPSC->tmpSuppRateBuf) );
				CopyMemOS(&(pPSC->tmpSuppRateSet),
							*pSuppRateSet, pSuppRateSet->Length);
			}
			if (pIbpm == NULL)
			{
				pPSC->bTmpIbpm = FALSE;
			}
			else
			{
				pPSC->bTmpIbpm = TRUE;
				PlatformMoveMemory(&(pPSC->tmpIbpm), pIbpm, sizeof(IbssParms));
			}
			//2. Set the LinkReqInIPSRFOffPgs bit to TRUE. It presents that we need to recall this fuction
			//    and do the link action when IPS RF off progress is finished. (e.g. Associstion)
			pPSC->LinkReqInIPSRFOffPgs = TRUE;
		}
		
		// 2010/05/10 MH For the timing test. We need to indicate a scan complete event
		// to prevent OS stop to send scan request OID for 1~2 minutes. If driver is trying to
		// enter or exit IPS when we execute scan request.
		

		//
		// <Roger_Notes> We should perform above scan operation under IPS mode to prevent NULL scan list before
		// BSS enumeration from upper layer, 2013.08.01.
		//
		if (pMgntInfo->RfOffReason > RF_CHANGE_BY_IPS)
		{
			DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
			return TRUE;
		}

	}
	
	// When scan only condition, we save the scan start time and prevent
	// the scan time from exceeding 4 minutes.
	if(!pMgntInfo->bPendScanOID)
	{
		if( pMgntInfo->bCheckScanTime == FALSE && bScanOnly == TRUE )
		{
			pMgntInfo->ScanOnlyStartTime = PlatformGetCurrentTime();
			pMgntInfo->bCheckScanTime = TRUE;
		}
	}
	else
		pMgntInfo->bPendScanOID = FALSE; 

	//
	// 1. Checking RF state for entering C3/C4 issue.
	// 2. When RF is off, we should not accept link request. 
	// 3. 2007.02.09, by Roger.
	//
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));	
	if (rtState == eRfOff)
	{	
		MgntLinkRequestReschedule(Adapter, bScanOnly, bActiveScan, bFilterHiddenAP, \
			bUpdateParms, pSsid2scan,  NetworkType, ChannelNumber, BcnPeriod, \
			DtimPeriod, mCap, pSuppRateSet, pIbpm);
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("<=== MgntLinkRequest(): RF is OFF.\n"));
		// We should not indicate scan complete here or it will cause acting the flow "IPS-> enter S3/S4 
		// -> resume from S3/S4 -> IPS -> manually scan once" then OS WLAN icon shows "Red X" because
		// we indicate a null scan list here. The scan complete should have been indicated after 
		// IPS re-scan flow. 2013.03.05, noted by tynli.
		//DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		return TRUE;
	}	

	if( 	(pMgntInfo->mAssoc) &&
		( pMgntInfo->bMediaConnect) &&
		!(pMgntInfo->bScanInProgress) && 
		!(pMgntInfo->bDualModeScanStep==2))
	{
		LeisurePSLeave(Adapter, LPS_DISABLE_LINK_REQ);
	}

	PlatformAcquireSpinLock(Adapter, RT_SCAN_SPINLOCK);

	if (Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_IQK_STATUS, (PBOOLEAN)&bIQKInProgress)) 
	{
		if(bIQKInProgress)
		{
			MgntLinkRequestReschedule(Adapter, bScanOnly, bActiveScan, bFilterHiddenAP, \
					bUpdateParms, pSsid2scan,  NetworkType, ChannelNumber, BcnPeriod, \
					DtimPeriod, mCap, pSuppRateSet, pIbpm);
			RT_TRACE(COMP_SCAN, DBG_LOUD, ("<=== MgntLinkRequest(): IQK inprogress.\n"));
			PlatformReleaseSpinLock(Adapter, RT_SCAN_SPINLOCK);
			return TRUE;
		}
	}
	
	if(pMgntInfo->bScanInProgress || (pMgntInfo->bDualModeScanStep==2) )
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("<=== MgntLinkRequest(): scan in progress.\n"));

		PlatformReleaseSpinLock(Adapter, RT_SCAN_SPINLOCK);

		RT_TRACE(COMP_MP, DBG_LOUD, ("== Scan Release Mutex of RF Protection ==\n"));
		
		// An dirty workaround for scan process. If the scan is in progress, and it is called
		// before connect. Then we set the scan flag again to let the last scan complete event
		// be indicated. 2007.01.04, by shien chang.
		if (pMgntInfo->bIndicateConnectEvent) 
		{
			DrvIFIndicateScanStart(Adapter);
			// to connect after scan, see also scancomplete 2008 06 20 Neo
		}

		// If the current request is a join action, mark this flag to join after scan. By Bruce, 2009-02-05.
		if(bScanOnly == FALSE)
			pMgntInfo->bScanOnly = FALSE;
		return TRUE;
	}
	
	pMgntInfo->bScanOnly = bScanOnly;
	pMgntInfo->FilterHiddenAP = bFilterHiddenAP;

	pMgntInfo->bActiveScan = bActiveScan;
	if(IS_DOT11D_ENABLE(pMgntInfo) )
	{			
		//
		// Check if we still get the same country IE in this scan, 
		// if not, we shall reset it.
		//
		RESET_CIE_WATCHDOG(pMgntInfo); 
	}
	RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkRequest(): %s....\n", (pMgntInfo->bActiveScan ? "Active Scan": "Passive Scan")));


	if(bUpdateParms)
	{
		if( !bScanOnly )
		{
			pMgntInfo->Regdot11networktype = NetworkType;
			if( pSsid2scan->Length )
			{
				CopySsidOS(pMgntInfo->Ssid, *pSsid2scan);
			} 
			pMgntInfo->dot11CurrentChannelNumber = ChannelNumber;
			pMgntInfo->dot11BeaconPeriod = BcnPeriod;
			pMgntInfo->dot11DtimPeriod = DtimPeriod;
			pMgntInfo->mCap = mCap;
			if( pSuppRateSet->Length > 0)
			{
				pMgntInfo->SupportedRates.Length = (pSuppRateSet->Length > MAX_NUM_RATES) ? MAX_NUM_RATES : pSuppRateSet->Length;
				CopyMem(pMgntInfo->SupportedRates.Octet, pSuppRateSet->Octet, pMgntInfo->SupportedRates.Length);
			}
			else
			{
				// Use Regdot11OperationalRateSet as default, 2004.11.16, by rcnjko.
				CopyMemOS( &pMgntInfo->SupportedRates, 
							pMgntInfo->Regdot11OperationalRateSet, 
							pMgntInfo->Regdot11OperationalRateSet.Length );
			}
			SelectRateSet( Adapter, pMgntInfo->SupportedRates );
			pMgntInfo->mIbssParms.atimWin = pIbpm->atimWin;
			pPSC->BufConnectinfoBefore = FALSE;
		}
		
	}

	HAL_HiddenSSIDHandleBeforeScan(Adapter);

	// TODO: In order to stable/optimize scan result, we might actually need to save the result for a certain time
	// TODO: We will leave this as it is at this time since we want to see the ture RF performance
	// Clear content of bssDesc[] before scanning.
	if(!CustomScan_InProgress(GET_CUSTOM_SCAN_INFO(Adapter)))
	{// customized scan shall not have effect on normal scan, here it should not clear the scan list, 2011.01.11, haich
		if(!GET_SIMPLE_CONFIG_ENABLED(pMgntInfo))   //suggested by hpfan, added by page, 20120921
			pMgntInfo->NumBssDesc = 0;
	}
	pMgntInfo->bFlushScanList = FALSE; // We have flushed scan list.	

	// Get supported wireless mode.
	SupportedWirelessMode = Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter);	

	//2 //Backup wireless setting before scan(restore in ScanComplete)
	//The restore setting should be the default adapter setting when multiports. by Wl,2011.07.18
	{
		CHANNEL_WIDTH 	bw = RT_GetBandWidth(Adapter);
		EXTCHNL_OFFSET 	extchnl20MHz,extchnl40MHz;
		u1Byte			centerfrequencyindex1;
		PADAPTER 		pDfAdapter = GetDefaultAdapter(Adapter);
		PMGNT_INFO 		pDfMgntInfo = &pDfAdapter->MgntInfo;

		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_BW40MHZ_EXTCHNL, (pu1Byte)(&extchnl20MHz));
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_BW80MHZ_EXTCHNL, (pu1Byte)(&extchnl40MHz));
		centerfrequencyindex1 = RT_GetCenter_Frequency_Index1(Adapter);

		pMgntInfo->SettingBeforeScan.WirelessModeScanInProgress = pMgntInfo->dot11CurrentWirelessMode;
		if(pDfMgntInfo->bMediaConnect) // If default Adapter not connected, we need not to restore to it's channel after scancomplete
			pMgntInfo->SettingBeforeScan.WirelessMode = pDfMgntInfo->dot11CurrentWirelessMode;
		else 
			pMgntInfo->SettingBeforeScan.WirelessMode = pMgntInfo->dot11CurrentWirelessMode;

		RT_TRACE(COMP_MLME, DBG_LOUD, ("pDfMgntInfo->bMediaConnect %d pMgntInfo->SettingBeforeScan.WirelessMode 0x%x\n", pDfMgntInfo->bMediaConnect, pMgntInfo->SettingBeforeScan.WirelessMode));

		if(IS_RSTO_WMM_AFTER_SCAN(Adapter))//add by ylb 20130627 for restore WMMParamEle after scan, Fix WMMParamEle changed during scan and set the wrong value of QOS Parameter( TxOP)
			PlatformMoveMemory(pMgntInfo->SettingBeforeScan.WMMParamEle, Adapter->MgntInfo.pStaQos->WMMParamEle,WMM_PARAM_ELEMENT_SIZE);

		HAL_SetCorrentChannelAndWirelessModeBeforeScan(Adapter);

		if(pDfMgntInfo->bMediaConnect)
		{
			pMgntInfo->SettingBeforeScan.ChannelNumber = pDfMgntInfo->dot11CurrentChannelNumber;
		}
		else
		{
			//use P2P client connected channel when default port is disconnected
			//by sherry 20130117
#if (MULTICHANNEL_SUPPORT == 1)
			PMULTICHANNEL_PORT_CONTEXT pClientPortContext; 
			if(GetFirstClientPort(Adapter) != NULL)
			{
				pClientPortContext =  (PMULTICHANNEL_PORT_CONTEXT)(GetFirstClientPort(Adapter))->MultiChannel;
				if(MgntLinkStatusQuery(GetFirstClientPort(Adapter)) == RT_MEDIA_CONNECT)
					pMgntInfo->SettingBeforeScan.ChannelNumber	= pClientPortContext->ConnectedPrimary20MhzChannel;
				else
					pMgntInfo->SettingBeforeScan.ChannelNumber = pMgntInfo->dot11CurrentChannelNumber;
			}
			// Comment the following code because pGoPortContext->ConnectedPrimary20MhzChannel may be 0 when the 
			// FCS mode is not enabled. By Bruce, 2014-12-15.
			else if(GetFirstGOPort(Adapter) != NULL)
			{
				PMULTICHANNEL_PORT_CONTEXT pGoPortContext; 
				
				pGoPortContext =  (PMULTICHANNEL_PORT_CONTEXT)(GetFirstGOPort(Adapter))->MultiChannel;
				
				if(MgntLinkStatusQuery(GetFirstGOPort(Adapter)) == RT_MEDIA_CONNECT)
				{
					pMgntInfo->SettingBeforeScan.ChannelNumber = pGoPortContext->ConnectedPrimary20MhzChannel;
				}
			}
			else
			{
				RT_TRACE(COMP_MLME,DBG_LOUD,("MgntLinkRequest: pMgntInfo->dot11CurrentChannelNumber:  %d \n",pMgntInfo->dot11CurrentChannelNumber));
			
				pMgntInfo->SettingBeforeScan.ChannelNumber = pMgntInfo->dot11CurrentChannelNumber;
		
			}
#else
			pMgntInfo->SettingBeforeScan.ChannelNumber = pMgntInfo->dot11CurrentChannelNumber;
#endif	
		
		}
		
		pMgntInfo->SettingBeforeScan.ChannelBandwidth = bw;
		pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz = extchnl20MHz;
		pMgntInfo->SettingBeforeScan.Ext40MHzChnlOffsetOf80MHz = extchnl40MHz;
		pMgntInfo->SettingBeforeScan.CenterFrequencyIndex1 = CHNL_GetCenterFrequency(pMgntInfo->SettingBeforeScan.ChannelNumber, bw, extchnl20MHz);

		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("MgntLinkRequest(): chnl: %u, bw: %u, Offset: %u\n",
			pMgntInfo->SettingBeforeScan.ChannelNumber, 
			pMgntInfo->SettingBeforeScan.ChannelBandwidth, 
			pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz));
	}

	RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("pMgntInfo->SettingBeforeScan.WirelessMode: %d\n", pMgntInfo->SettingBeforeScan.WirelessMode));
	RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("pMgntInfo->SettingBeforeScan.ChannelNumber: %d\n", pMgntInfo->SettingBeforeScan.ChannelNumber));
	RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("pMgntInfo->SettingBeforeScan.ChannelBandwidth: %d\n", pMgntInfo->SettingBeforeScan.ChannelBandwidth));
	RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz: %d\n", pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz));


	// TODO: possiblely  need to change here because of support of single band scan
	// TODO: need to add other condifion if we change the regwirelessmode rule and support N only scan

	if( (Adapter->RegWirelessMode & WIRELESS_MODE_AUTO) &&  (IS_DUAL_BAND_SUPPORT(Adapter)))
		pMgntInfo->bDualModeScanStep = 1;
	else
		pMgntInfo->bDualModeScanStep = 0;

	ScanByTimer( Adapter, pMgntInfo->bActiveScan, pMgntInfo->bScanOnly );

	PlatformReleaseSpinLock(Adapter, RT_SCAN_SPINLOCK);

	RT_TRACE(COMP_MP, DBG_LOUD, ("== Scan Release Mutex of RF Protection ==\n"));	
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("<=== MgntLinkRequest(): return TRUE.\n"));
	return TRUE;
}

VOID
MgntLinkStatusUpdateRxCounts(
	PADAPTER		Adapter,
	pu4Byte			TotalRxBcnNum,
	pu4Byte			TotalRxDataNum
)	
{
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;
	u2Byte 			SlotIndex;
	u1Byte			i;

	*TotalRxBcnNum = 0;
	*TotalRxDataNum = 0;

	//
	//Add by CCW and Maddest with WLK 1.3 Performance_ext,
	//To detect AP is disappear within 2 sec, 20090311.
	//
	if(Adapter->bInHctTest)
	{
		*TotalRxBcnNum = pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod;
		*TotalRxDataNum = pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod;			
	}
	else
	{	
		SlotIndex = (pMgntInfo->LinkDetectInfo.SlotIndex++)%pMgntInfo->LinkDetectInfo.SlotNum;
		pMgntInfo->LinkDetectInfo.RxBcnNum[SlotIndex] = pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod;
		pMgntInfo->LinkDetectInfo.RxDataNum[SlotIndex] = pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod;

		for( i=0; i<pMgntInfo->LinkDetectInfo.SlotNum; i++ )
		{
			*TotalRxBcnNum += pMgntInfo->LinkDetectInfo.RxBcnNum[i];
			*TotalRxDataNum += pMgntInfo->LinkDetectInfo.RxDataNum[i];
		}
	}	

	//cosa add for debug 09/26/2007. Report to DbgCmd utility
	if(pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod == 0)
		pMgntInfo->LinkDetectInfo.ConnectButNoBcnInPeriodCnt++;
	if(pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod == 0)
		pMgntInfo->LinkDetectInfo.ConnectButNoDataInPeriodCnt++;
	
	if( (*TotalRxBcnNum+*TotalRxDataNum) == 0 ) 
	{
		pMgntInfo->LinkDetectInfo.BecomeDisconnectedCnt++;	//cosa add for debug
		pMgntInfo->LinkDetectInfo.BecomeDisconnectedTime[pMgntInfo->LinkDetectInfo.BecomeDisconnectedIndex++] = PlatformGetCurrentTime();//cosa add for debug
		if(pMgntInfo->LinkDetectInfo.BecomeDisconnectedIndex >= 10)//cosa add for debug
			pMgntInfo->LinkDetectInfo.BecomeDisconnectedIndex = 0;
	}
}

/*
  * MgntLinkStatusProcWhenRFIsOff
  *		This function is called when
  *		(1) Driver detect that no beacon or data is received because RF is Off in "Connected" state
  *		(2) Driver is in "Disconnected State" because RF is Off
  */
VOID
MgntLinkStatusProcWhenRFIsOff(
	PADAPTER			Adapter,
	RT_MEDIA_STATUS		ConnectedState
)
{
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}
	
	switch(ConnectedState)
	{
		case RT_MEDIA_CONNECT:
			// When Received number of Beacon and Data packet is zero because
			// RF is in OFF state, do not start roaming, and we start indicating 
			// disconnected. By Bruce, 2007-10-03.
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusProcWhenRFIsOff(): RF OFF => inidcate disconnect and no more roaming\n"));

			if(MgntResetOrPnPInProgress(Adapter))
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));								
				return ;
			}

			if(MgntRoamingInProgress(pMgntInfo))
				MgntRoamComplete(Adapter, MlmeStatus_invalid);
			
			DrvIFIndicateDisassociation(Adapter, unspec_reason, pMgntInfo->Bssid);

			// We only do roaming in infra-structure mode, asked by Netgear, 2003.03.10, by rcnjko.
//			MgntResetRoamingState(pMgntInfo);
			
			MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, FORCE_INDICATE );

			// Reset all association entry.
			AsocEntry_ResetAll(Adapter);
			break;
			
		case RT_MEDIA_DISCONNECT:
			// If driver is in disconneted state because RF is Off, do not do MgntLinkRetry
			// By Emily, 2008-01-02
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusProcWhenRFIsOff(): ------- Keep DISCONNECTED ------- DisconnectedSlotCount(%d)\n", pMgntInfo->DisconnectedSlotCount));
			RT_TRACE(COMP_MLME, DBG_LOUD, 
				("MgntLinkStatusProcWhenRFIsOff(): RF is OFF, mAssoc(%d), mIbss(%d), OpMode(%d), Regdot11networktype(%d)\n", 
				pMgntInfo->mAssoc, pMgntInfo->mIbss, pMgntInfo->OpMode, pMgntInfo->Regdot11networktype));
			//2009.01.01 Added by tynli.
			if(pPSC->LinkReqInIPSRFOffPgs == TRUE)
			{
				pPSC->LinkReqInIPSRFOffPgs = FALSE;
				MgntLinkRequest(
						Adapter,
						pPSC->bTmpScanOnly,
						pPSC->bTmpActiveScan,
						pPSC->bTmpFilterHiddenAP,
						pPSC->bTmpUpdateParms,
						(pPSC->bTmpSsid2Scan ? &(pPSC->tmpSsid2Scan) : NULL),
						pPSC->tmpNetworkType,
						pPSC->tmpChannelNumber,
						pPSC->tmpBcnPeriod,
						pPSC->tmpDtimPeriod,
						pPSC->tmpmCap,
						(pPSC->bTmpSuppRate ? &(pPSC->tmpSuppRateSet) : NULL),
						(pPSC->bTmpIbpm ? &(pPSC->tmpIbpm) : NULL) );
				
				//clean the link request info
				pPSC->bTmpScanOnly = FALSE;
				pPSC->bTmpActiveScan = FALSE;
				pPSC->bTmpFilterHiddenAP = FALSE;
				pPSC->bTmpUpdateParms = FALSE;
				pPSC->bTmpSsid2Scan = FALSE;
				pPSC->tmpSsid2Scan.Octet = 0;
				pPSC->tmpSsid2Scan.Length = 0;
				pPSC->tmpNetworkType = 0;
				pPSC->tmpChannelNumber= 0;
				pPSC->tmpBcnPeriod = 0;
				pPSC->tmpDtimPeriod = 0;
				pPSC->tmpmCap = FALSE;
				pPSC->bTmpSuppRate = FALSE;
				pPSC->tmpSuppRateSet.Octet = 0;
				pPSC->tmpSuppRateSet.Length = 0;
				pPSC->bTmpIbpm = FALSE;
				pPSC->tmpIbpm.atimWin = 0; 

			}
			break;
			
	}
}

VOID
SwitchCheckBSSID(
	PADAPTER		Adapter
)
{
	static u4Byte count = 0;
	BOOLEAN	Value;
	
	// In 92C, we do not switch BSSID anymore because Hw has fixed the bug.
	// Suggested by Bruce. Added by tynli. 2009.11.26.
	return;
	

	if(count%2 == 0)
	{
		Value = FALSE;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)&Value);
		
	}else
	{
		Value = TRUE;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)&Value);
	}

	count++;
	
}


VOID
MgntLinkIotRelinkCheck(
	PADAPTER		Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	BOOLEAN		bNeedRelink=FALSE;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	if(pMgntInfo->IOTAction & HT_IOT_ACT_BCM_AP_RX_FAIL_RELINK)
	{		
		if(pMgntInfo->bMediaConnect && 
			(pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE) &&
			!MgntRoamingInProgress(pMgntInfo) &&
			(Adapter->RxStats.CurRxDataRate.RawValue != 0xffff ) &&
			(Adapter->RxStats.CurRxOfdmNum == 0) &&
			(GET_UNDECORATED_AVERAGE_RSSI(Adapter) > 15))
		{
			bNeedRelink = TRUE;
		}

		// execute relink procedure
		if(bNeedRelink)
		{

			if(MgntResetOrPnPInProgress(Adapter))
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));								
				return ;
			}		
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkIotRelinkCheck(): Infra. client => start roaming...\n"));
			MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_DEAUTH, ROAMINGSTATE_SCANNING);
			PlatformMoveMemory( pMgntInfo->APaddrbeforeRoaming, pMgntInfo->Bssid, 6);
			// Do this to reset TS data structure.
			RemoveAllTS(Adapter);

			DrvIFIndicateRoamingStart(Adapter);
			MgntLinkRetry(Adapter, TRUE);
		}
	}
}

VOID
MgntCheckScanListEmpty(
	PADAPTER Adapter
)
{
	PMGNT_INFO			pMgntInfo = GetDefaultMgntInfo(Adapter);
	RT_RF_POWER_STATE	rfState;

	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));

	if((pMgntInfo->NumBssDesc4Query == 0) && 
		(rfState==eRfOn || (rfState==eRfOff && RT_IN_PS_LEVEL(Adapter, RT_RF_OFF_LEVEL_FW_IPS_32K) && 
		!(pMgntInfo->RfOffReason & (RF_CHANGE_BY_SW|RF_CHANGE_BY_HW)))))
	{
		if((pMgntInfo->DumpDbgRegCnt < RT_SCAN_EMPTY_DUMP_REG_CNT) && !MgntScanInProgress(pMgntInfo))
		{
			// Dump all MAC and RF registers for debug
			HAL_DumpHwAllRegisters(Adapter);
			pMgntInfo->DumpDbgRegCnt++;
		}
	}
	else
	{
		// Reset counter
		pMgntInfo->DumpDbgRegCnt =0;
	}

}

VOID
MgntLinkHandleIPS(
	PADAPTER Adapter
)
{
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);
	PADAPTER	pAdapter =  pDefaultAdapter;
	PMGNT_INFO  pDefMgntInfo = GetDefaultMgntInfo(Adapter);
	PMGNT_INFO  pMgntInfo = pDefMgntInfo;	
	RT_RF_POWER_STATE    rfState;
	   
	RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS() ====>\n"));
	if(pDefaultAdapter->bInHctTest)
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS(): return for Hct Test!!\n"));
		return;
	}

	if(IsDevicePortDiscoverable(Adapter) || GetFirstGOPort(Adapter) || GetFirstClientPort(Adapter))
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS(): return for p2p!!\n"));
		return;
	}
	
	if(IsActiveAPModeExist(Adapter))
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS(): return for active Ap!!\n"));
		return;		
	}

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));	
	if(rfState != eRfOn)
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS(): return for RF off!!\n"));
		return;
	}
		
	while(pAdapter != NULL)
	{
		if(MgntScanInProgress(pMgntInfo)	||MgntIsLinkInProgress(pMgntInfo) ||pMgntInfo->SnifferTurnOnFlag  )
		{
			RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS(): return for scan or link progress!!\n"));
			return;
		}
		
              if ( (MgntLinkStatusQuery(pAdapter)==RT_MEDIA_CONNECT) || ACTING_AS_IBSS(pAdapter) || MgntRoamingInProgress(pMgntInfo))              
			return;
			
		pAdapter = GetNextExtAdapter(pAdapter);

		if(pAdapter == NULL) break;
		
		pMgntInfo = &pAdapter->MgntInfo;
	}

	//
	// <Roger_Notes> We should not enter IPS mode when the OID_DOT11_CURRENT_PHY_ID request for the value of the Extensible Station (ExtSTA) 
	// msDot11CurrentPhyID management are still under configuration on Win8 or later OS version.
	// 2013.08.27.
	//
	if(!PLATFORM_GET_PHY_ID_READY(pDefaultAdapter))
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS(): Value for the current PHY IDs are not ready!!\n"));
		return;
	}
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog(): RF is ON ,ReturnPoint(%d), bSetPnpPwrInProgress(%d), bDriverIsGoingToPnpSetPowerSleep(%d)\n", 
		pDefMgntInfo->PowerSaveControl.ReturnPoint, pDefMgntInfo->bSetPnpPwrInProgress, pDefaultAdapter->bDriverIsGoingToPnpSetPowerSleep));
	
	if((pDefMgntInfo->PowerSaveControl.ReturnPoint == IPS_CALLBACK_NONE) 
		&& (pDefMgntInfo->bSetPnpPwrInProgress == FALSE ) && (!pDefaultAdapter->bDriverIsGoingToPnpSetPowerSleep) //tynli_Test add Adapter->bDriverIsGoingToUnload)) 
#if (P2P_SUPPORT == 1)	
		&& !P2P_DOING_DEVICE_DISCOVERY(GET_P2P_INFO(pDefaultAdapter)))  // #if (P2P_SUPPORT == 1)	
#else
		)  
#endif

	{
		// Enter IPS only when no InactivePsTimerCallback is scheduled, by Bruce, 2007-10-03.
		// For inactive power save mode. 2007.07.16, by shien chang.
		RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS() call IPSEnter()\n"));
		IPSEnter(pDefaultAdapter);		
	}		
	else
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS(), ReturnPoint=%d/ bSetPnpPwrInProgress=%d\n", 
			pDefMgntInfo->PowerSaveControl.ReturnPoint, pDefMgntInfo->bSetPnpPwrInProgress));
	}
	RT_TRACE(COMP_RF, DBG_LOUD, ("MgntLinkHandleIPS() <====\n"));
}
 
VOID
MgntLinkHandleLPS(
	PADAPTER Adapter
)
{
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);
	PADAPTER	pAdapter =  pDefaultAdapter;
	PMGNT_INFO  pDefMgntInfo = GetDefaultMgntInfo(Adapter);
	PMGNT_INFO  pMgntInfo = pDefMgntInfo;
	BOOLEAN        bEnterLPS = FALSE;  //TRUE //Fix cannot enter LPS after start/stop Hostednetwork. By YJ,120405
	BOOLEAN		bFwCurrentInPSMode=FALSE;
	BOOLEAN		bFwPSAwake=TRUE;
#if (USB_TX_THREAD_ENABLE)	
	BOOLEAN 	bSupportUsbTxThread;
#endif
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	if(IsDevicePortDiscoverable(Adapter) || GetFirstGOPort(Adapter) || GetFirstClientPort(Adapter))
		return;
	
	if(IsActiveAPModeExist(Adapter))
		return;	
	
	while(pAdapter != NULL)
	{	
		if(MgntScanInProgress(pMgntInfo)	||MgntIsLinkInProgress(pMgntInfo)||pMgntInfo->SnifferTurnOnFlag  )
		     return;
	
		if(pMgntInfo->bMediaConnect &&  !MgntInitAdapterInProgress(pMgntInfo))
		{
			// Check busy traffic condition
			// Always enter LPS mode during WHQL DPC ISR test for SDIO interface.
			if( (!pMgntInfo->bSdioDpcIsr) && (((pMgntInfo->LinkDetectInfo.NumRxUnicastOkInPeriod + pMgntInfo->LinkDetectInfo.NumTxOkInPeriod) > 
				pMgntInfo->LinkDetectInfo.NumRxUnicastTxOkThresh) || 
				(pMgntInfo->LinkDetectInfo.NumRxUnicastOkInPeriod > pMgntInfo->LinkDetectInfo.NumRxUnicastThresh) ||
				(pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > pMgntInfo->LinkDetectInfo.NumRxOkThresh))    )  
			{
				if(pPSC->RegAdvancedLPs)
				{
					bEnterLPS = TRUE;
				}
				else
				{
					bEnterLPS = FALSE;
					break;
				}
			}
			else
			{
				bEnterLPS = TRUE;
			}

			if(pMgntInfo->mIbss)
			{
				bEnterLPS = FALSE;
				break;
			}

			if( pMgntInfo->SecurityInfo.SecLvl > RT_SEC_LVL_NONE  && 
				pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
			{
				//WPA and Infra mode , need check key is install 
				if(!SecIsTxKeyInstalled( Adapter, pMgntInfo->Bssid ))
				{
					bEnterLPS = FALSE;
					RT_TRACE( COMP_MLME , DBG_LOUD , (" Now key is not ready !! \n") );
					break;
				}
			}		
		}
		
		pAdapter = GetNextExtAdapter(pAdapter);

		if(pAdapter == NULL) break;
		
		pMgntInfo = &pAdapter->MgntInfo;

	}


#if (USB_TX_THREAD_ENABLE)
	pDefaultAdapter->HalFunc.GetHalDefVarHandler(pDefaultAdapter, HAL_DEF_USB_TX_THREAD, (PBOOLEAN)&bSupportUsbTxThread);
	
	if(bSupportUsbTxThread && pPSC->bLowPowerEnable && pPSC->bLeisurePs)
	{
		if(bEnterLPS)
		{
			// Before enter LPS mode, we should enable USB Tx thread first! 2012.08.23. by tynli.
			pDefaultAdapter->bUseUsbTxThread = TRUE;
			LeisurePSEnter(pDefaultAdapter);
		}
		else
		{
			LeisurePSLeave(pDefaultAdapter, LPS_DISABLE_WATCH_DOG_BUSY_TRAFFIC);

			pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PSMODE_STATUS, (pu1Byte)(&bFwCurrentInPSMode)); 
			pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FWLPS_RF_ON, (pu1Byte)(&bFwPSAwake));
			// Set bUseUsbTxThread to FALSE after we really leave LPS mode by checking Hw regsiters. 2012.08.23. by tynli.
			if((!bFwCurrentInPSMode) && bFwPSAwake)
			{
				pDefaultAdapter->bUseUsbTxThread = FALSE;
			}
		}
	}
	else
#endif		
	{
		if(bEnterLPS)
		{	
			LeisurePSEnter(pDefaultAdapter);
		}
		else
		{
			LeisurePSLeave(pDefaultAdapter, LPS_DISABLE_WATCH_DOG_BUSY_TRAFFIC);
		} 	
	}
}

VOID
MgntLinkStatusWatchdogForSystemWide(
	PADAPTER 		Adapter
)
{
	PADAPTER  pDefaultAdapter = GetDefaultAdapter(Adapter);
	PADAPTER  pAdapter = pDefaultAdapter; //add by ylb 20111130
	PMGNT_INFO	pDefMgntInfo = &pDefaultAdapter->MgntInfo;


{
	if(!FwLPS_IsInPSAndSupportLowPower(Adapter))
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Channel Register: %x\n" , PHY_QueryRFReg(GetDefaultAdapter(Adapter), 0, RF_CHNLBW, bMaskDWord)));

	// This checks the number of ports for each role in wifi-direct
	N63CValidateRoleOfWifiDirectPorts(pDefaultAdapter);
}

#if (MULTICHANNEL_SUPPORT == 1)
{
	MultiChannelStatusWatchdog(pDefaultAdapter);
}
#endif

	MgntCheckScanListEmpty(pDefaultAdapter);

	MgntLinkHandleIPS(pDefaultAdapter);
	MgntLinkHandleLPS(pDefaultAdapter);

	//Move From MgntLinkHandleLPS() by ylb 20111130 for DTM error : Skip scan because of BustTraffic = 1
	while(pAdapter != NULL)
	{
		pAdapter->MgntInfo.LinkDetectInfo.NumRxOkInPeriod = 0;
		pAdapter->MgntInfo.LinkDetectInfo.NumTxOkInPeriod = 0;
		pAdapter->MgntInfo.LinkDetectInfo.NumRxUnicastOkInPeriod = 0;

		pAdapter = GetNextExtAdapter(pAdapter);
	}	

	CustomScan_WatchDogCb(GET_CUSTOM_SCAN_INFO(pDefaultAdapter));
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("OnBeacon(BSS) for def port = %d\n", pDefMgntInfo->LinkDetectInfo.OnBeaconBssCnt_OnBeacon_Bss));
	RT_TRACE(COMP_RECV, DBG_LOUD, ("NumIdleRfd (%x) , NumBusyRfd(%x) \n",pDefaultAdapter->NumIdleRfd,pDefaultAdapter->NumBusyRfd[0]));
	RT_TRACE(COMP_RECV, DBG_LOUD, ("NextRxDescToFill (%x) , NextRxDescToCheck(%x) \n",pDefaultAdapter->NextRxDescToFill[0],pDefaultAdapter->NextRxDescToCheck[0]));
}

VOID
MgntLinkMultiPortStatusWatchdog(
	PADAPTER 		Adapter
)
{
	PADAPTER  pDefaultAdapter = GetDefaultAdapter(Adapter);
	PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);

	while(pTargetAdapter != NULL)
	{
		MgntLinkStatusWatchdogForEachPortSpecific(	pTargetAdapter);

		pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
	}
	//Move IPS/LPS here for multiport
	MgntLinkStatusWatchdogForSystemWide(pDefaultAdapter);
	
	// wpp trace for debug monitor
	EXdbgmon_AutoCollectStatisticsInfo(pDefaultAdapter);
}

BOOLEAN
MgntLinkStatusIsWifiBusy(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(GetDefaultAdapter(Adapter)->MgntInfo);
	PADAPTER		pLoopAdapter=NULL;

	if( (pMgntInfo->bMediaConnect) )		// HS mode
	{
		if(pMgntInfo->LinkDetectInfo.bBusyTraffic)
			return TRUE;
	}

	if(IsExtAPModeExist(Adapter))					// AP mode wifi traffic
	{
		PADAPTER		pExtAdapter = GetFirstExtAdapter(Adapter);
		PMGNT_INFO		pExtMgntInfo = NULL;

		pExtMgntInfo = &(pExtAdapter->MgntInfo);

		if( (pExtMgntInfo->LinkDetectInfo.bBusyTraffic))
			return TRUE;
	}

	// p2p busy state
	pLoopAdapter = GetDefaultAdapter(Adapter);
	while(pLoopAdapter != NULL)
	{
		if(pLoopAdapter->MgntInfo.LinkDetectInfo.bBusyTraffic)
			return TRUE;
		pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
	}

	return FALSE;
}


VOID
MgntLinkStatusWatchdogExtAp(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	BOOLEAN				bBusyTraffic = FALSE,bTxBusyTraffic=FALSE,bRxBusyTraffic=FALSE;
	BOOLEAN				bHigherBusyTraffic = FALSE;
	BOOLEAN				bHigherBusyRxTraffic = FALSE;

	if( pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > BUSY_TRAFFIC_THREADHOLD ||
		pMgntInfo->LinkDetectInfo.NumTxOkInPeriod > BUSY_TRAFFIC_THREADHOLD )
	{
		bBusyTraffic = TRUE;

		if(pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > pMgntInfo->LinkDetectInfo.NumTxOkInPeriod)
			bRxBusyTraffic=TRUE;
		else if(pMgntInfo->LinkDetectInfo.NumTxOkInPeriod > pMgntInfo->LinkDetectInfo.NumRxOkInPeriod)
			bTxBusyTraffic=TRUE;
	}

	// Higher Tx/Rx data.
	if( pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > BUSY_TRAFFIC_HIGH_THREADHOLD ||
		pMgntInfo->LinkDetectInfo.NumTxOkInPeriod > BUSY_TRAFFIC_HIGH_THREADHOLD )
	{
		bHigherBusyTraffic = TRUE;

		if(pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > pMgntInfo->LinkDetectInfo.NumTxOkInPeriod)
			bHigherBusyRxTraffic = TRUE;
		else
			bHigherBusyRxTraffic = FALSE;
	}

	pMgntInfo->LinkDetectInfo.NumRxOkInPeriod = 0;
	pMgntInfo->LinkDetectInfo.NumTxOkInPeriod = 0;
	pMgntInfo->LinkDetectInfo.NumRxUnicastOkInPeriod = 0;
	pMgntInfo->LinkDetectInfo.bBusyTraffic = bBusyTraffic;
	if(!pMgntInfo->LinkDetectInfo.bBusyTraffic)
		pMgntInfo->LinkDetectInfo.continuousIdleCnt++;
	else
		pMgntInfo->LinkDetectInfo.continuousIdleCnt = 0;
	pMgntInfo->LinkDetectInfo.bTxBusyTraffic = bTxBusyTraffic;
	pMgntInfo->LinkDetectInfo.bRxBusyTraffic = bRxBusyTraffic;
	pMgntInfo->LinkDetectInfo.bHigherBusyTraffic = bHigherBusyTraffic;
	pMgntInfo->LinkDetectInfo.bHigherBusyRxTraffic = bHigherBusyRxTraffic;
}

VOID
CheckInHighTPState(
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	PMGNT_INFO 		pMgntInfo = &(pAdapter->MgntInfo);
	u4Byte			TxThroughput=0; // Tx throughput in KBps.
	u4Byte			RxThroughput=0; // Rx throughput in KBps. 
	u4Byte			TotalThroughput=0; // Total throughput in last interval in KBps.

	//Reset the static variable, avoid  set the bBusyTrafficAccordingTP =true after wake up from S3 in the wrong state. It will causing ping loss.
	if((pAdapter->TxStats.NumTxOkBytesTotal<pAdapter->TxStats.LastNumTxOkBytesTotal)||(pAdapter->RxStats.NumRxOkBytesTotal<pAdapter->RxStats.LastNumRxOkBytesTotal))
	{
		//pAdapter->TxStats.LastNumTxOkBytesTotal = 0;
		//pAdapter->RxStats.LastNumRxOkBytesTotal = 0;
	}

	// Figure out throughput in last interval.
	if( MgntLinkStatusQuery(pAdapter)==RT_MEDIA_CONNECT) 
	{
		TxThroughput = ( (u4Byte)(pAdapter->TxStats.NumTxOkBytesTotal - pAdapter->TxStats.LastNumTxOkBytesTotal) )*8/2/1024 ;
		RxThroughput = ( (u4Byte)(pAdapter->RxStats.NumRxOkBytesTotal - pAdapter->RxStats.LastNumRxOkBytesTotal) )*8/2/1024 ;
		TotalThroughput = TxThroughput + RxThroughput;

		pAdapter->TxStats.LastNumTxOkBytesTotal =  (u4Byte)pAdapter->TxStats.NumTxOkBytesTotal;
		pAdapter->RxStats.LastNumRxOkBytesTotal =  (u4Byte)pAdapter->RxStats.NumRxOkBytesTotal ;

		pAdapter->TxStats.CurrentTxTP_Kbps = (u4Byte)(TxThroughput);
		pAdapter->RxStats.CurrentRxTP_Kbps = (u4Byte)(RxThroughput);

		pAdapter->TxStats.CurrentTxTP = (u2Byte)(TxThroughput/1024);
		pAdapter->RxStats.CurrentRxTP = (u2Byte)(RxThroughput/1024);

		if(((TxThroughput/1024)>pHalData->TxHignTPThreshold) && ((RxThroughput/1024)>=pHalData->RxHignTPThreshold))
		{
			pMgntInfo->LinkDetectInfo.bBusyTrafficAccordingTP = TRUE;
		}
		else
		{
			pMgntInfo->LinkDetectInfo.bBusyTrafficAccordingTP = FALSE;
		}

		if(pHalData->RxHignTPThreshold==9)
			pMgntInfo->LinkDetectInfo.bBusyTrafficAccordingTP = TRUE;
		else
			pMgntInfo->LinkDetectInfo.bBusyTrafficAccordingTP = FALSE;
		
		RT_TRACE(COMP_MLME,DBG_LOUD,("Tx: %d Mbps (%d Kbps)	:Rx: %d Mbps (%d Kbps), ------> In HighTP %d\n",
			pAdapter->TxStats.CurrentTxTP,
			TxThroughput,
			pAdapter->RxStats.CurrentRxTP,
			RxThroughput,
			pMgntInfo->LinkDetectInfo.bBusyTrafficAccordingTP));	
	}
	

	
}


VOID
MgntLinkStatusWatchdogForEachPortSpecific(
	PADAPTER		Adapter
	)
{
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	BOOLEAN				bBusyTraffic = FALSE,bTxBusyTraffic=FALSE,bRxBusyTraffic=FALSE;
	BOOLEAN				bHigherBusyTraffic = FALSE;
	BOOLEAN				bHigherBusyRxTraffic = FALSE;
	BOOLEAN				bVeryHigherBusyTraffic = FALSE;
	BOOLEAN				bVeryHigherBusyRxTraffic = FALSE;
	BOOLEAN				bVeryHigherBusyTxTraffic = FALSE;
	RT_RF_POWER_STATE 	rfState;
	u8Byte				u8TSF, u8TSFtmp;
	u1Byte				beacondelay = 30;	//count as 0.1s
	u1Byte				u1tmp;
	BOOLEAN				bWiFiConfig	= pMgntInfo->bWiFiConfg;

#if MULTIPORT_SUPPORT == 1
{	
	FunctionIn(COMP_MLME);
	MultiPortDumpPortStatus(Adapter);
}
#endif

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

    if(Adapter->bSurpriseRemoved)
	   	return;

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress bScanOnly FALSE\n"));
		return;
	}
	
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));
	
   	// Caculate the roaming fail count to decide if the roaming procedure should be canceled. By Bruce, 2008-05-22.
	if(MgntRoamingInProgress(pMgntInfo))
		pMgntInfo->RoamingFailCount ++;

	// Calculate the prepare roaming count to decide if the prepare roaming procedure should be canceled. By Sinda, 20150507.
	if(pMgntInfo->bPrepareRoaming == TRUE)
		pMgntInfo->PrepareRoamingCount++;

	DFS_StaMonitor(Adapter);

	LinkQualityReportCallback(Adapter);

	if(	ACTING_AS_AP(Adapter) == FALSE	&&
		!MgntScanInProgress(pMgntInfo)	&&
		!MgntIsLinkInProgress(pMgntInfo)	&&
		!pMgntInfo->SnifferTurnOnFlag  )
	{// Link Status Observation.
#if ALLOW_INDICATE_FW_STALL
		DrvIFIndicateFWStalled(Adapter);
#endif

		if( (MgntLinkStatusQuery(Adapter)==RT_MEDIA_CONNECT) 
			|| MgntRoamingInProgress(pMgntInfo))
		{
		 	// TODO: Add data packets observation.
			u4Byte	TotalRxBcnNum = 0;
			u4Byte	TotalRxDataNum = 0;	
			PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);

			CheckInHighTPState(Adapter);
			MgntLinkStatusUpdateRxCounts(Adapter, &TotalRxBcnNum, &TotalRxDataNum);
			
			// Switch RxReorder by Tx/Rx TP when RegRxReorder is 2. Add by Emerson 2015/01/29.
			if( pMgntInfo->RegRxReorder == 2 )
				pHTInfo->bCurRxReorderEnable = ( Adapter->TxStats.CurrentTxTP > 20 || Adapter->RxStats.CurrentRxTP > 20 ) ? TRUE : FALSE ;
			
			//
			// Age-out idle STA for Ad hoc mode.
			//
			if(pMgntInfo->mIbss)
			{
				IbssAgeFunction(Adapter);

				//
				// According to DD Tim's explanation, the 8192C chip should modify the TSF in IBSS mode, because there
				// is one issue in the TSF updating part which the TSF machine may update any beacon in air that cause
				// the TSF cannot synchronize each other.
				// <ToDo> This is very dirty fix for IBSS TSF here, we should move it to HAL or revise the whole fix procedure.
				// The current process was added by chiyokolin and discussed with DD Tim.
				// Comment added by Bruce, 2011-01-27.
				//
				if(!pMgntInfo->bIbssStarter)	//for joiner only
				{
					Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_TSF_TIMER, (pu1Byte)&u8TSFtmp);
					
					u8TSF = u8TSFtmp - PlatformModular64(u8TSFtmp, pMgntInfo->dot11BeaconPeriod*sTU) -sTU; //us, nextbeacon				

					//RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog before read back TSF 0x%"i64fmt"x \n", u8TSFtmp));										
					RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog before read back 0x418 0x%x \n", PlatformEFIORead4Byte(Adapter, 0x418)));					
					//RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog before pMgntInfo->NextBCN 0x%"i64fmt"x\n", pMgntInfo->NextBeacon));					

					if(pMgntInfo->NextBeacon > u8TSFtmp || ((u8TSFtmp - pMgntInfo->NowTSF) > (beacondelay * pMgntInfo->dot11BeaconPeriod * sTU)))
					{
						Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_TSF_TIMER, (pu1Byte)&u8TSF);
						//RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog UPDATE TSF to 0x%"i64fmt"x !!!!!!\n", u8TSF));	
					}

					pMgntInfo->NextBeacon = u8TSF;
					pMgntInfo->NowTSF = u8TSFtmp;				
			
				}
				
			}

#if RX_AGGREGATION
			// 2011/02/22 MH According to AP team's NAS test, we may meet a problem in WIN XP,
			// when Fizela copy file throughput network disk, it may occupy ndis packet for a period 
			// of time. At the same time, driver RFD list will be empty. But if USB aggregation is enabled
			// The rx bcn & data num will be zero and report incorrect disconnect information.
			PlatformAcquireSpinLock(Adapter, RT_RFD_SPINLOCK);
			if( (TotalRxBcnNum+TotalRxDataNum) == 0 && !RTIsListEmpty(&Adapter->RfdTmpList)) // Disconnected
#else
			if( (TotalRxBcnNum+TotalRxDataNum) == 0 ) // Disconnected.
#endif
			{
#if RX_AGGREGATION
				PlatformReleaseSpinLock(Adapter, RT_RFD_SPINLOCK);
#endif
				pMgntInfo->LinkDetectInfo.LastConnectedCenterFrequency = pMgntInfo->pChannelInfo->CurrentChannelCenterFrequency;
				pMgntInfo->LinkDetectInfo.LastConnectedBandwidth = pMgntInfo->dot11CurrentChannelBandWidth; 
				//1 Total Number of Received Beacon and Data packets is Zero

				RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog(): --- Become DISCONNECTED --- TotalRxBcnNum(%d) TotalRxDataNum(%d)\n", TotalRxBcnNum, TotalRxDataNum));
				
				if(rfState == eRfOff)
				{
					// When RF OFF, do not start roaming, and we start indicating disconnected. By Bruce, 2007-10-03.
					//For Win7 when HW & SW Radio Off, Donot Indicate Disassociation State by Sherry, 2009-12-25
					//Because driver indicate disconnected once RF is OFF and indicate roaming start, driver should not report disconnection here. Emily
					if(pMgntInfo->RfOffReason < RF_CHANGE_BY_HW)
						MgntLinkStatusProcWhenRFIsOff(Adapter, RT_MEDIA_CONNECT);
				}
				//
				//To avoid when pMgntInfo->mAssoc is set to false we will jumping to deal with Adhoc mode,
				//but we actually are in STA mode
				//
				else if(pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE )
				{
					LeisurePSLeave(GetDefaultAdapter(Adapter), LPS_DISABLE_FOR_ROAMING); // by tynli.

 					if( OS_SUPPORT_WDI(Adapter) )
 					{
					if(!MgntRoamingInProgress(pMgntInfo) && !pMgntInfo->bDisconnectRequest && pMgntInfo->bPrepareRoaming == FALSE)
					{
						if(MgntResetOrPnPInProgress(Adapter))
						{
							RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case POOR_LINK\n"));											
							return ;
						}					
						pMgntInfo->LinkDetectInfo.InfraDisconnectRoamingStartCnt++;	//cosa add for debug.
						RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog(): Infra. client => start roaming...\n"));
						PlatformMoveMemory( pMgntInfo->APaddrbeforeRoaming, pMgntInfo->Bssid, 6);
						Dot11d_Reset(Adapter);
						// Do this to reset TS data structure.
						RemoveAllTS(Adapter);
						
						// Stop multichannel switch
						MultiChannelDisconnectClient(Adapter, FALSE);					

						if(GET_TDLS_ENABLED(pMgntInfo))
							TDLS_Stop(Adapter);

						CCX_SetAssocReason(Adapter, CCX_AR_NORMAL_ROAM_POOR_LINK);

						DrvIFIndicateLinkStateChanged(Adapter, TRUE, 5);
						DrvIFIndicateRoamingNeeded(Adapter, RT_PREPARE_ROAM_NORMAL_ROAM_POOR_LINK);
					}					
				}
				else	
				{
						if(!MgntRoamingInProgress(pMgntInfo) && !pMgntInfo->bDisconnectRequest)
						{
							if(MgntResetOrPnPInProgress(Adapter))
							{
								RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case POOR_LINK\n"));											
								return ;
							}					
							pMgntInfo->LinkDetectInfo.InfraDisconnectRoamingStartCnt++;	//cosa add for debug.
							RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog(): Infra. client => start roaming...\n"));
							MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_DISCONNECT_POOR_LINK, ROAMINGSTATE_SCANNING);
							PlatformMoveMemory( pMgntInfo->APaddrbeforeRoaming, pMgntInfo->Bssid, 6);
							Dot11d_Reset(Adapter);
							// Do this to reset TS data structure.
							RemoveAllTS(Adapter);
							
							// Stop multichannel switch
							MultiChannelDisconnectClient(Adapter, FALSE);					

							if(GET_TDLS_ENABLED(pMgntInfo))
								TDLS_Stop(Adapter);

							CCX_SetAssocReason(Adapter, CCX_AR_NORMAL_ROAM_POOR_LINK);

							DrvIFIndicateRoamingStart(Adapter);
							MgntLinkRetry(Adapter, TRUE);
						}
						else
						{
							pMgntInfo->LinkDetectInfo.InfraDisconnectRoamingFailCnt++;	//cosa add for debug.

							RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog(): Infra. client RoamingFailCount(%d)\n", Adapter->MgntInfo.RoamingFailCount));

							if(MgntRoamingInProgress(pMgntInfo))
							{
								// Roaming retry
								if(MgntRoamRetry(Adapter, FALSE)) // Raom again.
									return;
								else // Roam failed.
								{
									if(MgntResetOrPnPInProgress(Adapter))
									{
										RT_TRACE(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));				
										return;
									}	
								
									MgntRoamComplete(Adapter, MlmeStatus_timeout);
								}
							}	
						}
					}
				}
				else	
				{
					pMgntInfo->LinkDetectInfo.AdhocDisconnectCnt++;	//cosa add for debug.
					RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog(): AdHoc mode => inidcate disconnect and no more roaming\n"));

					if(MgntResetOrPnPInProgress(Adapter))
					{
						RT_TRACE(COMP_INIT, DBG_LOUD, ("reset in progress becomde disconnected IBSS\n"));
						return;
					}

					if(MgntRoamingInProgress(pMgntInfo))
						DrvIFIndicateRoamingComplete(Adapter, RT_STATUS_FAILURE);
					else if(pMgntInfo->bMediaConnect)
						DrvIFIndicateDisassociation(Adapter, unspec_reason, pMgntInfo->Bssid);

					MgntResetJoinProcess(Adapter);
					MgntResetRoamingState(pMgntInfo);
					
					if(	(pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC) || 
						(pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_AUTO) )
					{
						pMgntInfo->bIbssStarter = TRUE; // For we are the last one in this IBSS, 2005.07.11, by rcnjko.
					}

					MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, FORCE_INDICATE );

					// Reset all association entry.
					AsocEntry_ResetAll(Adapter);
					// NOTE! For HCT WPA_AdHoc test, we should not link retry here. 2005.07.11, by rcnjko.
				}
			}
			else
			{
#if RX_AGGREGATION
				PlatformReleaseSpinLock(Adapter, RT_RFD_SPINLOCK);
#endif
				//1 Total Number of Received Beacon and Data packets is not Zero

				//Suggested by SD1 Isaac, add received beacon number message for FW debug, by hana 2015/08/05 
				RT_TRACE(COMP_AP,DBG_LOUD,("[FW debug] Number of recv beacon in 2 sec = %d \n",pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod));


				RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog(): +++++++ Keep CONNECTED +++++++ TotalRxBcnNum(%d) TotalRxDataNum(%d)\n", TotalRxBcnNum, TotalRxDataNum));

				BSS_IdleScanWatchDog(Adapter);

				// For infrastructure mode. Periodically update RTS rate to prevent the RTS rate > init data
				// rate. 2010.11.24. by tynli.
				if(!(FwLPS_IsInPSAndSupportLowPower(Adapter) && RT_IN_PS_LEVEL(Adapter, RT_RF_OFF_LEVEL_FW_IPS_32K)))
				{
					if(pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
						Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_INIT_RTS_RATE, (pu1Byte)(&Adapter));
				}

				if(GET_TDLS_ENABLED(pMgntInfo))
				{
					TDLS_LinkStatusWatchDog(Adapter);
					TDLS_UpdatePeerStatus(Adapter);
					AsocEntry_AgeFunction(Adapter);
				}
			}

			pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod = 0;
			pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod = 0;
			
			if( OS_SUPPORT_WDI(Adapter) )
			{
			if(pMgntInfo->bPrepareRoaming == TRUE)
			{
				pMgntInfo->LinkDetectInfo.InfraDisconnectRoamingFailCnt++;	//cosa add for debug.

				RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog(): Prepare roaming. PrepareRoamingCount(%d), RoamingLimitCount(%d)\n", Adapter->MgntInfo.PrepareRoamingCount, Adapter->MgntInfo.RegRoamingLimitCount));

				if( (pMgntInfo->PrepareRoamingCount > pMgntInfo->RegRoamingLimitCount) && (pMgntInfo->bPrepareRoaming == TRUE) )
				{
					pMgntInfo->bPrepareRoaming = FALSE;
					pMgntInfo->PrepareRoamState = RT_PREPARE_ROAM_NONE;
					pMgntInfo->PrepareRoamingCount = 0;

					if( pMgntInfo->bMediaConnect || pMgntInfo->bIbssStarter)
					{
						if( pMgntInfo->mIbss )
						{
							RT_TRACE( COMP_OID_SET, DBG_LOUD, ("MgntLinkStatusWatchdog() => MgntDisconnectIBSS\n") );
							MgntDisconnectIBSS(Adapter);
						}

						if( pMgntInfo->mAssoc)
						{
							RT_TRACE( COMP_OID_SET, DBG_LOUD, ("MgntLinkStatusWatchdog() => MgntDisconnectAP\n") );
							MgntDisconnectAP(Adapter, disas_lv_ss);
						}
					}
				}
			}
		}
		}
		else
		{
			//1 Driver is in Disconnected State

			switch(rfState)
			{
				case eRfOn:
					pMgntInfo->DisconnectedSlotCount++;
					RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntLinkStatusWatchdog(): ------- Keep DISCONNECTED ------- DisconnectedSlotCount(%d)\n", pMgntInfo->DisconnectedSlotCount));
					RT_TRACE(COMP_MLME, DBG_LOUD, 
						("MgntLinkStatusWatchdog(): RF is ON ,mAssoc(%d), mIbss(%d), bMediaConnect(%d), OpMode(%d), Regdot11networktype(%d)\n", 
						pMgntInfo->mAssoc, pMgntInfo->mIbss, pMgntInfo->bMediaConnect, pMgntInfo->OpMode, pMgntInfo->Regdot11networktype));

					if( pMgntInfo->mIbss )
					{
						// NOTE! For HCT WPA_AdHoc test, we should not link retry here. 2005.07.11, by rcnjko.
						if(!Adapter->bInHctTest)
							SwitchCheckBSSID(Adapter);
					} 
					break;
					
				case eRfOff:
				default:
					MgntLinkStatusProcWhenRFIsOff(Adapter, RT_MEDIA_DISCONNECT);
					break;
					
			}
		}

		// Moved by tynli. 2009.02.17
		//
		// Determine if our traffic is busy now, 060515, by rcnjko.
		//
		if(pMgntInfo->bMediaConnect && !MgntInitAdapterInProgress(pMgntInfo))
		{

			if(Adapter->bInHctTest)
			{
				if( pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > 666 ||
					pMgntInfo->LinkDetectInfo.NumTxOkInPeriod > 666 )
				{
					bBusyTraffic = TRUE;
				}
			}
			else
			{
				u4Byte	busyThreshold=BUSY_TRAFFIC_THREADHOLD;
				
				
			
				if( pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > busyThreshold ||
					pMgntInfo->LinkDetectInfo.NumTxOkInPeriod > busyThreshold )
				{
					bBusyTraffic = TRUE;

					if(pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > pMgntInfo->LinkDetectInfo.NumTxOkInPeriod)
						bRxBusyTraffic=TRUE;
					else if(pMgntInfo->LinkDetectInfo.NumTxOkInPeriod > pMgntInfo->LinkDetectInfo.NumRxOkInPeriod)
						bTxBusyTraffic=TRUE;
				}

				// Higher Tx/Rx data.
				if( pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > BUSY_TRAFFIC_HIGH_THREADHOLD ||
					pMgntInfo->LinkDetectInfo.NumTxOkInPeriod > BUSY_TRAFFIC_HIGH_THREADHOLD )
				{
					bHigherBusyTraffic = TRUE;

					// Extremely high Rx data.
					if(pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > pMgntInfo->LinkDetectInfo.NumTxOkInPeriod)
						bHigherBusyRxTraffic = TRUE;
					else
						bHigherBusyRxTraffic = FALSE;
				}

				//if( pMgntInfo->LinkDetectInfo.NumTxOknPeriod > BUSY_TRAFFIC_VERY_HIGH_THREADHOLD)
				if (Adapter->TxStats.CurrentTxTP > BUSY_TRAFFIC_VERY_HIGH_TP)
				{
					bVeryHigherBusyTraffic = TRUE;
					bVeryHigherBusyTxTraffic = TRUE;
				}
				else
				{
					bVeryHigherBusyTraffic = FALSE;
					bVeryHigherBusyTxTraffic = FALSE;
				}

				//if( pMgntInfo->LinkDetectInfo.NumRxOkInPeriod > BUSY_TRAFFIC_VERY_HIGH_THREADHOLD )
				if (Adapter->RxStats.CurrentRxTP > BUSY_TRAFFIC_VERY_HIGH_TP)
				{
					bVeryHigherBusyTraffic = TRUE;
					bVeryHigherBusyRxTraffic = TRUE;										
				}
				else
					bVeryHigherBusyRxTraffic = FALSE;
			}
			
			RT_TRACE(COMP_SEND, DBG_LOUD, ("MgntLinkStatusWatchdog():  RxUnicastOk(%d) TxOk(%d) RxOk(%d)\n", 
						pMgntInfo->LinkDetectInfo.NumRxUnicastOkInPeriod, 
						pMgntInfo->LinkDetectInfo.NumTxOkInPeriod,
						pMgntInfo->LinkDetectInfo.NumRxOkInPeriod));
		}
	}
	else if(ACTING_AS_AP(Adapter))
	{
		MgntLinkStatusWatchdogExtAp(Adapter);
		AsocEntry_AgeFunction(Adapter);
	}

	//This check is only for Win7/XP two port.
	//Vista/XP one port don't need it.
	//if(IsAPModeExist(Adapter))	// IS_ADAPTER_SENDS_BEACON(Adapter)
	if(IsExtAPModeExist(Adapter))
	{
		PADAPTER 		pExtAdapter = GetFirstExtAdapter(Adapter);
		PMGNT_INFO		pExtMgntInfo = NULL;
		
		pExtMgntInfo = &(pExtAdapter->MgntInfo);

		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_TSF_TIMER, (pu1Byte)&u8TSFtmp);
		
		u8TSF = u8TSFtmp - PlatformModular64(u8TSFtmp , (pMgntInfo->dot11BeaconPeriod*sTU)) -sTU; //us, nextbeacon				

#if (WPP_SOFTWARE_TRACE == 0)
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before read back TSF %"i64fmt"d nextbeacon  %"i64fmt"d\n", u8TSFtmp, u8TSF));					
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before pMgntInfo->NextBCN %"i64fmt"d ;   NowTSF %"i64fmt"d\n", pExtMgntInfo->NextBeacon, pExtMgntInfo->NowTSF));				
#endif
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before read back 0x120 0x%x \n", PlatformEFIORead4Byte(Adapter, REG_HIMR)));									
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before read back 0x418 0x%x \n", PlatformEFIORead4Byte(Adapter, 0x418)));					
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before read back 0x510 0x%x \n", PlatformEFIORead4Byte(Adapter, 0x510)));					
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before read back 0x520 0x%x\n", PlatformEFIORead4Byte(Adapter, REG_TX_PTCL_CTRL)));												
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before read back 0x540 0x%x \n", PlatformEFIORead4Byte(Adapter, 0x540)));									
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before read back 0x550 0x%x \n", PlatformEFIORead4Byte(Adapter, 0x550)));									
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before read back 0x554 0x%x \n", PlatformEFIORead4Byte(Adapter, 0x554)));
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog before read back 0x55d 0x%x \n", PlatformEFIORead4Byte(Adapter, 0x55d)));									

		if((pExtMgntInfo->NowTSF == 0 && !MultiChannelSwitchNeeded(Adapter)) || pExtMgntInfo->NextBeacon > u8TSFtmp || ((u8TSFtmp - pExtMgntInfo->NowTSF) > (beacondelay * pMgntInfo->dot11BeaconPeriod * sTU)))
		{
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_TSF_TIMER, (pu1Byte)&u8TSF);
#if (WPP_SOFTWARE_TRACE == 0)
			RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog UPDATE TSF to %"i64fmt"d !!!!!!!\n", u8TSF)); 					
#endif
		}

		pExtMgntInfo->NextBeacon = u8TSF;
		pExtMgntInfo->NowTSF = u8TSFtmp;				

		if(Adapter->bInHctTest)
		{
			u1tmp = PlatformEFIORead1Byte(Adapter, rOFDM0_XAAGCCore1);
			RT_TRACE(COMP_BEACON, DBG_LOUD, ("MgntLinkStatusWatchdog 0xc50 0x%x\n", u1tmp));		
			if(u1tmp != 0x30)
				PlatformEFIOWrite1Byte(Adapter, rOFDM0_XAAGCCore1, 0x30);
		}
		
		//if(ACTING_AS_AP(Adapter))
		//{
		//	AsocEntry_AgeFunction(Adapter);
		//}
	}	

	PlatformHandleNLOnScanRequest( (PVOID) Adapter);


	// In AP mode, GO shall skip scanning when client is sending Null Data for FCS.
	// Check if client is switching channel.  by hana, 2014.05.15

	if(Adapter != GetDefaultAdapter(Adapter) && pMgntInfo->bWaitBeforeGoSkipScan != 0)
	{
		u1Byte RxNullDataNumThreshold = 3 ;
		u1Byte WaitTimeThreshold = pMgntInfo->bWaitBeforeGoSkipScan; // Wait 2 * WaitTimeThreshold  seconds before GO skip scanning 

		if(pMgntInfo->LinkDetectInfo.RxNullDataNum > RxNullDataNumThreshold)
		{
			pMgntInfo->LinkDetectInfo.ClientAskForFCSNum++;
			if(pMgntInfo->LinkDetectInfo.ClientAskForFCSNum >= WaitTimeThreshold)
			{
				pMgntInfo->LinkDetectInfo.bClientAskForFCS = TRUE ;
				RT_TRACE(COMP_MLME, DBG_LOUD,("GO skip scan when client is switching channel : %d [%d]\n",
					pMgntInfo->LinkDetectInfo.RxNullDataNum,
					pMgntInfo->LinkDetectInfo.ClientAskForFCSNum));

				pMgntInfo->LinkDetectInfo.ClientAskForFCSNum = 0;
			}
		}
		else 
		{
			pMgntInfo->LinkDetectInfo.bClientAskForFCS = FALSE ;
			RT_TRACE(COMP_MLME, DBG_TRACE,("Client is not switching channel : %d [%d]\n",
				pMgntInfo->LinkDetectInfo.RxNullDataNum,
				pMgntInfo->LinkDetectInfo.ClientAskForFCSNum));
		}
		
		pMgntInfo->LinkDetectInfo.RxNullDataNum=0;

	}

	// CCX status watchdog.
	CCX_OnLinkStatusWatchdog(Adapter);

	P2POnLinkStatusWatchdog(Adapter);

	RxStatisticsWatchdog(Adapter);

	if(ACTING_AS_AP(Adapter)) AP_StatusWatchdog(Adapter);

	//
	// Determine if action frame is allowed
	//
	// <Note> If we need to delay action frame, then pMgntInfo->bStartDelayActionFrame should be set 
	// to TRUE and reset pMgntInfo->CntAfterLink to zero.
	if(pMgntInfo->bStartDelayActionFrame && (pMgntInfo->CntAfterLink < RT_ACTION_FRAME_DELAY_CNT))
	{
		pMgntInfo->CntAfterLink++;
	}
	else
	{
		pMgntInfo->CntAfterLink = RT_ACTION_FRAME_DELAY_CNT+ 1;
		pMgntInfo->bStartDelayActionFrame = FALSE;
	}
	
	if(!ACTING_AS_AP(Adapter))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, 
			("MgntLinkStatusWatchdog(): RxUnicastOk: %d TxOk: %d => bBusyTraffic: %d  bStableHigherBusyTraffic:%d\n", 
			pMgntInfo->LinkDetectInfo.NumRxUnicastOkInPeriod, pMgntInfo->LinkDetectInfo.NumTxOkInPeriod, bBusyTraffic,pMgntInfo->LinkDetectInfo.bStableHigherBusyTraffic));

		pMgntInfo->LinkDetectInfo.bBusyTraffic = bBusyTraffic;
		if(!pMgntInfo->LinkDetectInfo.bBusyTraffic)
			pMgntInfo->LinkDetectInfo.continuousIdleCnt++;
		else
			pMgntInfo->LinkDetectInfo.continuousIdleCnt = 0;
		pMgntInfo->LinkDetectInfo.bTxBusyTraffic = bTxBusyTraffic;
		pMgntInfo->LinkDetectInfo.bRxBusyTraffic = bRxBusyTraffic;
		pMgntInfo->LinkDetectInfo.bHigherBusyTraffic = bHigherBusyTraffic;
		pMgntInfo->LinkDetectInfo.bHigherBusyRxTraffic = bHigherBusyRxTraffic;
		pMgntInfo->LinkDetectInfo.bVeryHigherBusyTraffic = bVeryHigherBusyTraffic;
		pMgntInfo->LinkDetectInfo.bVeryHigherBusyRxTraffic = bVeryHigherBusyRxTraffic;
		pMgntInfo->LinkDetectInfo.bVeryHigherBusyTxTraffic = bVeryHigherBusyTxTraffic;
	 }


	MgntLinkIotRelinkCheck(Adapter);
}

VOID
MgntResetLinkStatusWatchdogState(
	PADAPTER		Adapter
	)
{
	//1 Reset disconnect state machine
	
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;

	pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod = 1;
}

RT_MEDIA_STATUS
MgntLinkStatusQuery(
	PADAPTER		Adapter
)
{
	PMGNT_INFO			pMgntInfo = NULL;
	RT_MEDIA_STATUS	mStatus = RT_MEDIA_DISCONNECT;
		
	if(Adapter == NULL)
		return RT_MEDIA_DISCONNECT;

	pMgntInfo = &Adapter->MgntInfo;

	if( (pMgntInfo->mIbss==TRUE) || (pMgntInfo->mAssoc==TRUE) )
	{
		if( pMgntInfo->bMediaConnect == TRUE )
		{
			mStatus = RT_MEDIA_CONNECT;
		}
	}

	return mStatus;
}

VOID
SwBeaconCallback(
	PRT_TIMER		pTimer
)
{
	PADAPTER	Adapter = (PADAPTER)pTimer->Adapter;
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);
	
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	
	if(RT_CANNOT_IO(Adapter))
		return;

	//We need use default bDriverIsGoingToUnload to check driver is unload or not, or it will BSOD when in 
	//WLK 1.4 test (SoftAP_notification), by Maddest 06052009.
	//
	if(pDefaultAdapter->bDriverIsGoingToUnload)
		return;

	if(pMgntInfo->BeaconState == BEACON_STOP)
	{
		RT_TRACE(COMP_AP, DBG_LOUD, ("SwBeaconCallback(): BeaconState = BEACON_STOP, stop timer!!!\n"));
		return;
	}

	if(pDefaultAdapter->MgntInfo.bStartApDueToWakeup)
	{
		RT_TRACE(COMP_AP, DBG_LOUD, ("SwBeaconCallback(): bStartApDueToWakeup ==TRUE, stop timer!!!\n"));
		return;
	}

	if(RT_DRIVER_STOP(Adapter))
	{
		return;
	}

	RT_TRACE(COMP_BEACON, DBG_TRACE, ("SwBeaconCallback(): S/W Beacon\n"));
	if(((PRT_WORK_ITEM)(&pMgntInfo->TbttPollingWorkItem))->RefCount!=1)
	{
		PlatformSetTimer(Adapter, &(pMgntInfo->SwBeaconTimer), 1); // Wait the previous workitem complete.
		RT_TRACE(COMP_BEACON, DBG_LOUD, ("SwBeaconCallback(): TbttPollingWorkItem refcount == %d\n",
			((PRT_WORK_ITEM)(&pMgntInfo->TbttPollingWorkItem))->RefCount));
	}
	else
	{
		PlatformScheduleWorkItem( &(pMgntInfo->TbttPollingWorkItem) );
	}
}


//
//	Generate a random integer number in the interval [MinIndex, MaxIndex).
//	Annie, 2005-11-24.
//
int
GetRandomNumber(
	IN	int	MinIndex,
	IN	int	MaxIndex
)
{
	u1Byte	Hashed[20];
	int		iRdm, interval;

	// Input check
	interval = MaxIndex - MinIndex;
	if( interval <= 0 )
	{
		RT_ASSERT( FALSE, ("GetRandomNumber(): Invalid Index!\n") );
		return MinIndex;
	}

	GetRandomBuffer(Hashed);
	
	iRdm = *( (int *)Hashed );			// usd first 4 bytes only (int: 32 bits.)
	if( iRdm < 0 )	iRdm = 0-iRdm;		// make positive
	iRdm = iRdm % interval;			// Now iRdm is in [0, interval)
	iRdm += MinIndex;				// Now iRdm is in [MinIndex, MaxIndex)

	//RT_TRACE( COMP_DBG, DBG_LOUD, ("GetRandomNumber(): iRdm =%d\n", iRdm) );
	return iRdm;
}


//
//	Generate a random u2Byte number in the interval [MinIndex, MaxIndex).
//	Created by Roger, 2008-05-07.
//
u2Byte
GetRandomU2ByteNumber(
	IN	u2Byte	MinIndex,
	IN	u2Byte	MaxIndex
)
{
	u1Byte	Hashed[20];
	u2Byte	iRdm, interval;

	// Input check
	interval = MaxIndex - MinIndex;
	if( interval <= 0 )
	{
		RT_ASSERT( FALSE, ("GetRandomU2ByteNumber(): Invalid Index!\n") );
		return MinIndex;
	}

	GetRandomBuffer(Hashed);
	
	iRdm = *( (u2Byte *)Hashed );		// usd first 2 bytes only (short: 16 bits.)
	//if( iRdm < 0 )	iRdm = 0-iRdm;		// make positive
	iRdm = iRdm % interval;			// Now iRdm is in [0, interval)
	iRdm += MinIndex;				// Now iRdm is in [MinIndex, MaxIndex)

	//RT_TRACE( COMP_DBG, DBG_LOUD, ("GetRandomNumber(): iRdm =%d\n", iRdm) );
	return iRdm;
}

//
//	Use system time to generate a random 20-bytes-buffer.
//	(SHA1HashSize: 20)
//
//	Annie, 2005-11-24.
//
VOID
GetRandomBuffer(
	OUT	pu1Byte	pHashed
)
{
	u1Byte	i;
	u1Byte	Buffer[32];
	pu1Byte	pSeed1, pSeed2, pTime;
	u1Byte	LenSeed1 = 32;
	u1Byte	LenSeed2 = 16;
	u8Byte	CurTime = PlatformGetCurrentTime();

	// 1. Use current time to generate a 32-byte buffer.
	pTime = (pu1Byte)&CurTime;
	{
		// byte 0~7
		CopyMem( Buffer, pTime, 8 );

		for( i=0; i<8; i++ )
		{
			// byte 8~15
			Buffer[8+i] = ~pTime[i];
			
			// byte 16~23
			Buffer[16+i] = pTime[7-i];
			
			// byte 24~31
			Buffer[24+i] = ~pTime[7-i];
		}
	}

	// 2. Assign the seeds.
	pSeed1 = Buffer;		// Buffer[0]~Buffer[31], 32 bytes.
	pSeed2 = Buffer+8;	// Buffer[8]~Buffer[23], 16 bytes.

	// 3. Hash Function: hmac_sha1.		// [AnnieNote] PasswordHash() takes about 350 ms!It's too long time!
	hmac_sha1( pSeed1, LenSeed1, pSeed2, LenSeed2, pHashed );

	//RT_PRINT_DATA( COMP_AP, DBG_LOUD, "GetRandomBuffer(): Hashed Buffer", pHashed, 20 );
}

VOID
MgntAddRejectedAsocAP(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		AddrOfAP
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	u1Byte i;

	RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "MgntAddRejectedAsocAP()", AddrOfAP);
	
	if ( MgntIsInRejectedAPList(Adapter, AddrOfAP) )
	{
		return;
	}
	
	if (pMgntInfo->NumRejectedAsocAP == MAX_REJECTED_ACCESS_AP)
	{
		for (i=1; i<MAX_REJECTED_ACCESS_AP; i++)
		{
			PlatformMoveMemory(
				pMgntInfo->RejectedAsocAP[i-1],
				pMgntInfo->RejectedAsocAP[i],
				6);
		}
		PlatformMoveMemory(
			pMgntInfo->RejectedAsocAP[MAX_REJECTED_ACCESS_AP-1],
			AddrOfAP,
			6);
	}
	else
	{
		PlatformMoveMemory(
			pMgntInfo->RejectedAsocAP[pMgntInfo->NumRejectedAsocAP],
			AddrOfAP,
			6);
		pMgntInfo->NumRejectedAsocAP++;
	}
}

VOID
MgntClearRejectedAsocAP(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntClearRejectedAsocAP()...........\n"));
	PlatformZeroMemory(pMgntInfo->RejectedAsocAP, 6*MAX_REJECTED_ACCESS_AP);
	pMgntInfo->NumRejectedAsocAP = 0;
}

u4Byte
MgntGetRejectedAPIndex(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		AddrOfAP
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u4Byte	i;

	for (i=0; i<pMgntInfo->NumRejectedAsocAP; i++)
	{
		if ( eqNByte(pMgntInfo->RejectedAsocAP[i], AddrOfAP, 6) )
		{
			return i;
		}
	}

	return MAX_REJECTED_ACCESS_AP;
}

PRT_WLAN_BSS
MgntHasOtherAPwithSameSSID(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u4Byte i;

	for (i=0; i<pMgntInfo->NumBssDesc4Query; i++)
	{
		if (CompareSSID( pMgntInfo->bssDesc4Query[i].bdSsIdBuf , pMgntInfo->bssDesc4Query[i].bdSsIdLen,
					   pMgntInfo->Ssid.Octet,pMgntInfo->Ssid.Length) == TRUE )
		{
			if ( !MgntIsInRejectedAPList(Adapter, pMgntInfo->bssDesc4Query[i].bdBssIdBuf) )
			{
				return &(pMgntInfo->bssDesc4Query[i]);
			}			
		}
	}

	return NULL;
}

BOOLEAN
MgntIsInRejectedAPList(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		AddrOfAP
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u1Byte	i;

	for (i=0; i<pMgntInfo->NumRejectedAsocAP; i++)
	{
		if ( eqNByte(pMgntInfo->RejectedAsocAP[i], AddrOfAP, 6) )
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN
MgntTryToRoam(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u4Byte i;
	BOOLEAN bNetworkToRoam = FALSE;
	PRT_WLAN_BSS	pRtBss = NULL;
	
	for (i=0; i<pMgntInfo->NumBssDesc4Query; i++)
	{
		pRtBss = &(pMgntInfo->bssDesc4Query[i]);
		if ( (pRtBss->bdSsIdLen == pMgntInfo->Ssid.Length) &&
			(PlatformCompareMemory(
				pRtBss->bdSsIdBuf,
				pMgntInfo->Ssid.Octet,
				pMgntInfo->Ssid.Length) == 0) )
		{
			if ( ((pRtBss->bdCap & cESS) && (pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)) )
			{
				if ( !MgntIsInRejectedAPList(Adapter, pMgntInfo->Bssid) )
				{
					bNetworkToRoam = TRUE;
					break;
				}
			}
		}
	}

	if (bNetworkToRoam && pRtBss != NULL)
	{
		MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_NORMAL, ROAMINGSTATE_SCANNING);
		PlatformMoveMemory( pMgntInfo->APaddrbeforeRoaming, pMgntInfo->Bssid, 6);

		JoinRequest(Adapter, (RT_JOIN_ACTION)pMgntInfo->OpMode, pRtBss);
	}

	return bNetworkToRoam;
}

//
//	Description:
//		Set up default configuration and allocate resource for Turbo Channel Access. 
//
VOID
InitTurboChannelAccess(
	PADAPTER	Adapter
)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_TURBO_CA	pTurboCa = &(pMgntInfo->TurboChannelAccess);

	pTurboCa->bEnabled = FALSE;
	pTurboCa->CheckCnt = 1;
	pTurboCa->CheckInterval = TCA_DEF_CHECK_INTERVAL;

	PlatformInitializeTimer( Adapter, &pMgntInfo->TurboChannelAccess.TcaCheckTimer, (RT_TIMER_CALL_BACK)TcaCheckTimerCallback, NULL, "TcaCheckTimer");

#if 0 // Disable TCA funcion in default, added by Roger, 2013.08.15
	if(!(RT_DRIVER_HALT(Adapter)) && !Adapter->bInHctTest)
	{
		PlatformSetTimer( Adapter, &(pTurboCa->TcaCheckTimer), TCA_CHECK_PERIOD );
	}
#endif

}


//
//	Description:
//		DeInitialization for TCA.
//
VOID
DeInitTurboChannelAccess(
	PADAPTER	Adapter
)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	
	PlatformCancelTimer(Adapter, &pMgntInfo->TurboChannelAccess.TcaCheckTimer );

	PlatformReleaseTimer(Adapter, &pMgntInfo->TurboChannelAccess.TcaCheckTimer );
}



//
//	Description:
//		Periodical timer for Turbo Channel Access checking.
//	2006.08.15, by rcnjko. 
//
VOID
TcaCheckTimerCallback(
	PRT_TIMER		pTimer
)
{
	PADAPTER		Adapter=(PADAPTER)pTimer->Adapter;
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PRT_TURBO_CA	pTurboCa = &(pMgntInfo->TurboChannelAccess);

#if RTL8192SU_ASIC_VERIFICATION	
	return;
#endif

	//if(Adapter->bDriverStopped)
		return;
	
	do{
		u4Byte			TxThroughput; // Tx throughput in KBps.
		u4Byte			RxThroughput; // Rx throughput in KBps. 
		u4Byte			TotalThroughput; // Total throughput in last interval in KBps.
		u4Byte			HighestOperaRateInMbps = (pMgntInfo->HighestOperaRate >= 2 ? (pMgntInfo->HighestOperaRate/2) : 1); // In Mbps

		// Figure out throughput in last interval.
		TxThroughput = ( (u4Byte)(Adapter->TxStats.NumTxOkBytesTotal - pTurboCa->LastTxStats.NumTxOkBytesTotal) ) / TCA_CHECK_PERIOD;
		RxThroughput = ( (u4Byte)(Adapter->RxStats.NumRxOkBytesTotal - pTurboCa->LastRxStats.NumRxOkBytesTotal) ) / TCA_CHECK_PERIOD;
		TotalThroughput = TxThroughput + RxThroughput;
		pTurboCa->TotalThroughput = TotalThroughput/1024;

		// Keep information about throughput of last interval. 
		pTurboCa->TxThroughput = TxThroughput;
		pTurboCa->RxThroughput = RxThroughput;
		PlatformMoveMemory(&(pTurboCa->LastTxStats), &(Adapter->TxStats), sizeof(RT_TX_STATISTICS));
		PlatformMoveMemory(&(pTurboCa->LastRxStats), &(Adapter->RxStats), sizeof(RT_RX_STATISTICS));

		//2 Enter TCA if we are infrastructure mode STA and !bAutoTurboBy8186, FALSE otherwise.
		if(!pMgntInfo->bSupportTurboMode)
		{
			break;
		}

		if(pMgntInfo->bAutoTurboBy8186)
		{
			break;
		}

		if(pMgntInfo->pStaQos->CurrentQosMode > QOS_DISABLE)
		{
			break;
		}

		if(!pMgntInfo->mAssoc || ACTING_AS_AP(Adapter))
		{
			break;
		}

		// If total throughput is smaller than 1/8 of highest operation rate, turn off TCA.
		// For example, in G mode, if total throughput < 6.75 Mbps, we turn off TCA.
		
		if( ((TotalThroughput*8)/1000) < (HighestOperaRateInMbps/8) )
		{
			if(pTurboCa->bEnabled)
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, 
					("----- Turn off TCA for TotalThroughput: %d KBps is not large enough, HighestOperaRate(%d).\n", 
					TotalThroughput, pMgntInfo->HighestOperaRate));
				MgntActSet_EnterTurboMode( Adapter, FALSE); 
			}

			pTurboCa->CheckCnt = 1;
			pTurboCa->CheckInterval = TCA_DEF_CHECK_INTERVAL;
			PlatformMoveMemory(&(pTurboCa->LastNormalTxStats), &(Adapter->TxStats), sizeof(RT_TX_STATISTICS));
			PlatformMoveMemory(&(pTurboCa->LastNormalRxStats), &(Adapter->RxStats), sizeof(RT_RX_STATISTICS));

			break;
		}

		// Change state of TCA in the boundary of (CheckInterval * TCA_CHECK_PERIOD) ms, or
		// wait the comming of the boundary.
		if(pTurboCa->CheckCnt % pTurboCa->CheckInterval == 0)
		{
			if(!pTurboCa->bEnabled)
			{ // TCA is OFF.
				u4Byte			NormalTxThroughput; // Tx throughput in KBps.
				u4Byte			NormalRxThroughput; // Rx throughput in KBps. 

				// Figure out Total throughput in Non-TCA mode.
				RT_ASSERT(pTurboCa->CheckInterval > 0, ("TcaCheckTimerCallback(): CheckInterval(%d) should > 0\n", pTurboCa->CheckInterval));
				NormalTxThroughput = ((u4Byte)(Adapter->TxStats.NumTxOkBytesTotal - pTurboCa->LastNormalTxStats.NumTxOkBytesTotal) ) / (TCA_CHECK_PERIOD * pTurboCa->CheckInterval);
				NormalRxThroughput = ((u4Byte)(Adapter->RxStats.NumRxOkBytesTotal - pTurboCa->LastNormalRxStats.NumRxOkBytesTotal) ) / (TCA_CHECK_PERIOD * pTurboCa->CheckInterval);
				pTurboCa->NormalTotalThroughput = NormalTxThroughput + NormalRxThroughput;

				// Turn on TCA.
				RT_TRACE(COMP_MLME, DBG_LOUD, 
					("+++++ Turn on TCA, CheckCnt(%d), CheckInterval(%d), NormalTotalThroughput: %d KBps.\n", 
					pTurboCa->CheckCnt, pTurboCa->CheckInterval, pTurboCa->NormalTotalThroughput));

				// Modified by Roger, 2006.12.07.
				MgntActSet_EnterTurboMode( Adapter, TRUE);

				// Reset CheckInterval to default setting for TCA on. 
				pTurboCa->CheckInterval = TCA_DEF_CHECK_INTERVAL;
			}
			else
			{ // TCA is ON.
				s4Byte			DeltaTotalThroughput; // Delta of Total throughput in KBps. 
				
				//2 Figure out Delta of Total throughput.
				DeltaTotalThroughput = (s4Byte)TotalThroughput - (s4Byte)(pTurboCa->NormalTotalThroughput);
		
				if(DeltaTotalThroughput < 0) //&&
					//(u4Byte)(-1 * (DeltaTotalThroughput*200 / 125)) > HighestOperaRateInMbps )
				{
					// Figure out CheckInterval such that loss of throughput <= 0.005* Highest Rate.
					// => CheckInterVal = ceiling(Loss in Mbps / (0.005 * Highest Op Rate in Mbps)) - 1
					u4Byte LossInMbps = (u4Byte)(-1 * (DeltaTotalThroughput / 125)); // in Mbps: (-1 * DeltaTotalThroughput * 8) / 1000
					u4Byte PenalityFactor = 200; // 1 / 0.005.

					if(	(LossInMbps * PenalityFactor) > HighestOperaRateInMbps )  
					{
						pTurboCa->CheckInterval = (((LossInMbps * PenalityFactor) + HighestOperaRateInMbps) / HighestOperaRateInMbps) - 1;
						RT_TRACE(COMP_MLME, DBG_LOUD, ("DeltaTotalThroughput(%d) < 0 => 1. Update CheckInterval(%d)\n", DeltaTotalThroughput, pTurboCa->CheckInterval));
					}
					else
					{
						pTurboCa->CheckInterval = 1;
						RT_TRACE(COMP_MLME, DBG_LOUD, ("DeltaTotalThroughput(%d) < 0 => 2. Update CheckInterval(%d)\n", DeltaTotalThroughput, pTurboCa->CheckInterval));
					}
					RT_ASSERT(pTurboCa->CheckInterval >= 1, ("TcaCheckTimerCallback(): pTurboCa->CheckInterval(%d) < !\n", pTurboCa->CheckInterval));

					pTurboCa->CheckInterval += TCA_NG_EXTRA_CHECK_INTERVAL;
					// Turn off TCA.
					RT_TRACE(COMP_MLME, DBG_LOUD, 
						("----- Turn off TCA for DeltaTotalThroughput(%d) < 0, NormalTotalThroughput(%d)\n", 
						DeltaTotalThroughput, pTurboCa->NormalTotalThroughput));
					
					// Modified by Roger, 2006.12.07.
					MgntActSet_EnterTurboMode( Adapter, FALSE);
					
					PlatformMoveMemory(&(pTurboCa->LastNormalTxStats), &(Adapter->TxStats), sizeof(RT_TX_STATISTICS));
					PlatformMoveMemory(&(pTurboCa->LastNormalRxStats), &(Adapter->RxStats), sizeof(RT_RX_STATISTICS));
				}
				else
				{
					RT_TRACE(COMP_MLME, DBG_TRACE, ("DeltaTotalThroughput: %d > 0 => Keep turn on TCA\n", DeltaTotalThroughput));
					pTurboCa->CheckInterval = TCA_OK_CHECK_INTERVAL;
				}
			}

			// Reset CheckCnt to 1. 
			pTurboCa->CheckCnt = 1;
		}
		else
		{
			// Increase CheckCnt.
			pTurboCa->CheckCnt++;
		}
	} while(FALSE);

	if(!Adapter->bInHctTest)
		PlatformSetTimer( Adapter, &pMgntInfo->TurboChannelAccess.TcaCheckTimer, TCA_CHECK_PERIOD );

	RT_TRACE( COMP_MLME, DBG_TRACE, ("<=== TcaCheckTimerCallback()\n") );
}


VOID
PnPWakeUpJoinTimerCallback(
	IN PRT_TIMER		pTimer
	)
{
	PADAPTER	pAdapter = (PADAPTER)pTimer->Adapter;

	//PnpSetPower(pAdapter);
	RT_TRACE(COMP_POWER, DBG_LOUD, ("===>PnPWakeUpJoinTimerCallback\n"));

	/* This can not be successfully compiled on xp.
	if(pAdapter->bInHctTest)
	{
		// Wait for Pending Request
		for(i=0;i<10;i++)
		{
			if(pAdapter->pNdisCommon->PendedRequest  == NULL)
				break;
			PlatformStallExecution(20);
		}
	}
*/	
	
	MgntActSet_802_11_SSID(pAdapter, pAdapter->MgntInfo.Ssid.Octet, pAdapter->MgntInfo.Ssid.Length, TRUE );
//	pMgntInfo->bRoamingbySleep = FALSE;	
	
	RT_TRACE(COMP_POWER, DBG_LOUD, ("<===PnPWakeUpJoinTimerCallback\n"));
}

//
// Description:
//	Decide whether the BSSID of AP or peer address is agreed by desired list.
//	2006.11.14, by shien chang.
//	Move it from N6C_OidSet.c to here.
// Argument:
//	Adapter -
//		NIC adapter context pointer.
//	MacAddr -
//		The mac address to be verified if it is in the desired list.
// Rewritten by Bruce, 2008-05-27.
//
BOOLEAN
IsInDesiredBSSIDList(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		MacAddr
	)
{
	return PlatformIsInDesiredBSSIDList(Adapter, MacAddr);
}

BOOLEAN
GetDesiredSSIDList(
	IN	PADAPTER	Adapter
	)
{
	return PlatformGetDesiredSSIDList(Adapter);
}


VOID
WaitingKeyTimerCallback(
	PRT_TIMER		pTimer
)
{
	PADAPTER	Adapter=(PADAPTER)pTimer->Adapter;

	MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, FORCE_INDICATE);
	//delay_ms(1000);
	//MgntIndicateMediaStatus( Adapter, RT_MEDIA_CONNECT, FORCE_INDICATE );
	return;
	
}

//
// Description:
//	Stop sending Beacon in AP mode or IBSS and stop the SW Beacon timer. Replace "AP_StopSendBeacon()" by this function.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
// Return:
//	None
// By Bruce, 2011-01-18.
//
VOID
MgntStopBeacon(
	IN		PADAPTER	pAdapter
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u1Byte			SwBeaconType = BEACON_SEND_AUTO_HW;

	FunctionIn(COMP_MLME);

	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_HW_BEACON_SUPPORT, (pu1Byte)&SwBeaconType);

	// Mark the Beacon state first to prevent the timer still going.
	pMgntInfo->BeaconState = BEACON_STOP;

	PlatformCancelTimer(pAdapter, &pMgntInfo->SwBeaconTimer);

	// We need to stop HW beacon related configuration.
	if(SwBeaconType <= BEACON_SEND_AUTO_SW)
	{
		BOOLEAN 	bStopSendBeacon = TRUE;
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_STOP_SEND_BEACON, (pu1Byte)&bStopSendBeacon);
	}
}

VOID
MgntResumeBeacon(
	IN		PADAPTER	pAdapter
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u1Byte			SwBeaconType = BEACON_SEND_AUTO_HW;

	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_HW_BEACON_SUPPORT, (pu1Byte)&SwBeaconType);

	// Mark the Beacon state first to prevent the timer still going.
	pMgntInfo->BeaconState = BEACON_STARTED;

	if(SwBeaconType >= BEACON_SEND_AUTO_SW) // Using SW Beacon Timer		
		PlatformSetTimer( pAdapter, &pMgntInfo->SwBeaconTimer, 0);

	// We need to stop HW beacon related configuration.
	if(SwBeaconType <= BEACON_SEND_AUTO_SW)
	{
		BOOLEAN 	bStopSendBeacon = FALSE;
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_STOP_SEND_BEACON, (pu1Byte)&bStopSendBeacon);
	}
}

BOOLEAN	
MgntScanInProgress(
	PMGNT_INFO		pMgntInfo
)
{

	PADAPTER	pAdapter = (PADAPTER)CONTAINING_RECORD(pMgntInfo, ADAPTER, MgntInfo);
	pAdapter = GetDefaultAdapter(pAdapter);

	while(pAdapter !=NULL)
	{
		if(pAdapter->MgntInfo.bScanInProgress)
			return TRUE;
		pAdapter = GetNextExtAdapter(pAdapter);
	}
	return FALSE;

}

BOOLEAN 
MgntIsLinkInProgress(
	PMGNT_INFO		pMgntInfo
)
{	
	PADAPTER	pAdapter = (PADAPTER)CONTAINING_RECORD(pMgntInfo, ADAPTER, MgntInfo);
	BOOLEAN		bResult = FALSE;

	PPORT_COMMON_INFO 	pPortCommonInfo = pAdapter->pPortCommonInfo;

	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	bResult = (pMgntInfo->bScanInProgress && !pMgntInfo->bScanOnly)	||	
	((pMgntInfo->JoinTimer.Status&RT_TIMER_STATUS_SET) && !(pMgntInfo->JoinTimer.Status&RT_TIMER_STATUS_CANCEL_NG))			||	
	((pMgntInfo->JoinConfirmTimer.Status&RT_TIMER_STATUS_SET) && !(pMgntInfo->JoinConfirmTimer.Status&RT_TIMER_STATUS_CANCEL_NG))			||	
	((pMgntInfo->JoinProbeReqTimer.Status&RT_TIMER_STATUS_SET) && !(pMgntInfo->JoinProbeReqTimer.Status&RT_TIMER_STATUS_CANCEL_NG))			||	
	((pMgntInfo->AuthTimer.Status&RT_TIMER_STATUS_SET)	&& !(pMgntInfo->AuthTimer.Status&RT_TIMER_STATUS_CANCEL_NG))		||	
	((pMgntInfo->AsocTimer.Status&RT_TIMER_STATUS_SET)	&& !(pMgntInfo->AsocTimer.Status&RT_TIMER_STATUS_CANCEL_NG))		||	
	(pMgntInfo->bJoinInProgress);
	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock)); 	
	

	RT_TRACE(COMP_MLME, DBG_TRACE, ("MgntIsLinkInProgress %d\n", bResult));

	return bResult;								
}

BOOLEAN 
MgntIsTimeOutForIndication(
	PMGNT_INFO		pMgntInfo
)
{	
	BOOLEAN		bResult = FALSE;

	PADAPTER	pAdapter = (PADAPTER)CONTAINING_RECORD(pMgntInfo, ADAPTER, MgntInfo);
	u8Byte		nowTime = 0, deltatime = 0;

	nowTime = PlatformGetCurrentTime();

	RT_TRACE(COMP_MLME, DBG_LOUD, ("RoamingType 0x%x\n", pMgntInfo->RoamingType));
	
	if(MgntRoamingInProgress(pMgntInfo))
	 	deltatime = (ULONG)( (nowTime - pAdapter->LastRoamingStartIndicationTime) / 100000 ); // in 0.1 second.
	else
		deltatime = (ULONG)( (nowTime - pAdapter->LastConnectStartIndicationTime) / 100000 ); // in 0.1 second.			

	if(deltatime > RT_CONNECT_ROAM_INDICATE_TIME_LIMIT)	
	{	
		if(MgntRoamingInProgress(pMgntInfo))
		{
			RT_TRACE_F(COMP_MLME, DBG_LOUD, ("LastRoamingStartIndicationTime %d\n", (u4Byte)pAdapter->LastRoamingStartIndicationTime));
		}
		else
		{
			RT_TRACE_F(COMP_MLME, DBG_LOUD, ("LastConnectStartIndicationTime %d\n", (u4Byte)pAdapter->LastConnectStartIndicationTime));
		}

		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Time Out deltatime %d\n", (u4Byte)deltatime));
		bResult = TRUE;
	}

	return bResult;								
}
	

BOOLEAN
IsSendingBeacon(
	PADAPTER	Adapter
	)
{
	if(IsAPModeExist(Adapter) || GetDefaultAdapter(Adapter)->MgntInfo.mIbss)
		return TRUE;
	return FALSE;
}

/***********************************************************************
Function: Mgnt_SwChnl()
		Merge all switch channel operation in 1 function
Parameter: 
		Channel: the channel number to switch
		SwitchChannelMethod: 1: calling SwChnlByTimerHandler
						     2: calling  SwChnlByDelayHandler
************************************************************************/

VOID
Mgnt_SwChnl(
	IN	PADAPTER	Adapter,
	IN	u1Byte		Channel,
	IN	u1Byte		SwitchChannelMethod
)
{

	RT_TRACE(COMP_SCAN, DBG_LOUD,("============>Mgnt_SwChnl channel %d\n ",Channel));

	if(!IS_HARDWARE_TYPE_OLDER_THAN_8812A(Adapter))
	{
		if(SwitchChannelMethod == 1)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD,("Mgnt_Swchnl: calling SwChnlAndSetBWHandler case \n"));
			Adapter->HalFunc.SwChnlAndSetBWHandler(
				Adapter,
				TRUE,
				FALSE,
				Channel,
				CHANNEL_WIDTH_20,
				EXTCHNL_OFFSET_NO_DEF,
				EXTCHNL_OFFSET_NO_DEF,
				Channel
			);
		}
		else if(SwitchChannelMethod == 2)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD,("Mgnt_Swchnl: calling SwChnlByDelayHandler case \n"));
			Adapter->HalFunc.SwChnlByDelayHandler(Adapter,Channel);
		}
		else
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD,("Mgnt_Swchnl: error case \n"));
		}
	}
	else
	{
		if(SwitchChannelMethod == 1)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD,("Mgnt_Swchnl: calling SwChnlByTimerHandler case \n"));
			Adapter->HalFunc.SwChnlByTimerHandler(Adapter,Channel);
		}
		else if(SwitchChannelMethod == 2)
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD,("Mgnt_Swchnl: calling SwChnlByDelayHandler case \n"));
			Adapter->HalFunc.SwChnlByDelayHandler(Adapter,Channel);
		}
		else
		{
			RT_TRACE(COMP_SCAN, DBG_LOUD,("Mgnt_Swchnl: error case \n"));
		}
	}
}

VOID
Mgnt_BackupVarBeforeScan(
	IN	PADAPTER			Adapter,
	IN	u1Byte				wirelessMode,
	IN	u1Byte				chnl,
	IN	CHANNEL_WIDTH		bw,
	IN	EXTCHNL_OFFSET		chExt
	)
{
	PMGNT_INFO         pMgntInfo = &(Adapter->MgntInfo);
	
	pMgntInfo->SettingBeforeScan.WirelessMode = (WIRELESS_MODE)wirelessMode;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("pMgntInfo->SettingBeforeScan.WirelessMode 0x%x\n", pMgntInfo->SettingBeforeScan.WirelessMode));
		
	pMgntInfo->SettingBeforeScan.ChannelNumber = chnl;
	pMgntInfo->SettingBeforeScan.ChannelBandwidth = bw;
	pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz = chExt;
}

//
// Descriptor: Recover FW offload machanisms for AOAC mode. Because we will alternate
//			WoWLAN and normal FW when entering and leaving WoWLAN mode
//			such that some machanisms be stoped by redownloading FW.
//			During connection state, we should restart:
//			(1) Rate adaptive
//			(2) Download reserved page
//
// 2015.06.25. Added by tynli.
//
VOID
MgntRecoverFWOffloadMechanism(
	IN	PADAPTER			Adapter
)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	BOOLEAN			bUseRAMask = FALSE;
	u1Byte			mstatus = RT_MEDIA_CONNECT;


	// HalMacId: Set a specific MacId for this client -----
	//pMgntInfo->mMacId = (u1Byte)MacIdRegisterMacIdForInfraClient(Adapter);
	//pMgntInfo->mcastMacId = (u1Byte)MacIdRegisterMacIdForMCastClient(Adapter);
	// -------------------------------------------

	//
	// Rate adaptive
	//
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USE_RA_MASK, &bUseRAMask);

	RT_TRACE_F(COMP_INIT, DBG_LOUD, ("[DBG] bUseRAMask(%u)\n", bUseRAMask));
	
	if(bUseRAMask)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[DBG] 1\n"));
		// we only need to set rate mask
		Adapter->HalFunc.UpdateHalRAMaskHandler(Adapter, pMgntInfo->mMacId, NULL, 0);
		Adapter->HalFunc.UpdateHalRAMaskHandler(Adapter, pMgntInfo->mcastMacId, NULL, 0);

	}
	//
	// 2015/04/16 MH According to ODM input parameters, we need to send default port HAL datastructure's ODM 
	// Otherwise, ext port will send null HAL pointer and cause BSOD.
	//
	GetDefaultAdapter(Adapter)->HalFunc.RAPostActionHandler(&(((PHAL_DATA_TYPE)GetDefaultAdapter(Adapter)->HalData)->DM_OutSrc));

	// Update RTS init rate after we set RA MASK. by tynli. 2010.11.25.
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_INIT_RTS_RATE, (pu1Byte)&Adapter);

	//
	// Reserved page related settings
	//

	if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS)
	{	//For FW LPS. by tynli. To tell firmware we have connected to an AP. For 92SE/CE power save v2.
		Hal_SetRsvdCtrl(Adapter, RSVDPAGE_TYPE_LPS);
	}

	// New Connection is comming, we need to confirm new channel connect and start
	// to judge if we need to start multi channel switch.
	//MultiChannelAssociateConfirm(Adapter, TRUE);

	// Download rsvd page after all required bitmap is set
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_FW_JOINBSSRPT, (pu1Byte)(&mstatus));
}

//
// Description:
//	Clear all entries for Fast Transmition list.
//
VOID
FtResetEntryList(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u2Byte			i = 0;

	for(i = 0; i < MAX_FT_ENTRY_NUM; i ++)
	{
		PlatformZeroMemory(&pMgntInfo->FtEntryList[i], sizeof(FT_INFO_ENTRY));
	}

	FunctionOut(COMP_MLME);
}

//
// Description:
//	Return a new FT entry. 
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
//	[in] pTargetAddr -
//		The target address to be the key of the returned entry.
// Return:
//	If there is no empty FT entry, return NULL.
// Remark:
//	When the returned entry is not NULL, this function has marked bValid flag as TRUE when this entry is returned.
//
PFT_INFO_ENTRY
FtGetNewEntry(
	IN	PADAPTER			pAdapter,
	IN	pu1Byte				pTargetAddr
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u2Byte			i = 0;

	for(i = 0; i < MAX_FT_ENTRY_NUM; i ++)
	{
		if(!TEST_FLAG(pMgntInfo->FtEntryList[i].Flag, FT_INFO_VALID))
		{
			PlatformZeroMemory(&pMgntInfo->FtEntryList[i], sizeof(FT_INFO_ENTRY));
			PlatformMoveMemory(pMgntInfo->FtEntryList[i].targetAddr, pTargetAddr, 6);
			SET_FLAG(pMgntInfo->FtEntryList[i].Flag, FT_INFO_VALID);
			return &pMgntInfo->FtEntryList[i];
		}
	}

	return NULL;
}

//
// Description:
//	Return the corresponding FT entry according to the target address.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
//	[in] pTargetAddr -
//		The target address to be the key of the returned entry.
// Return:
//	If there is no matching entry, return NULL.
//
PFT_INFO_ENTRY
FtGetEntry(
	IN	PADAPTER			pAdapter,
	IN	pu1Byte				pTargetAddr
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u2Byte			i = 0;

	for(i = 0; i < MAX_FT_ENTRY_NUM; i ++)
	{
		if(TEST_FLAG(pMgntInfo->FtEntryList[i].Flag, FT_INFO_VALID) &&
			eqMacAddr(pMgntInfo->FtEntryList[i].targetAddr, pTargetAddr))
			return &pMgntInfo->FtEntryList[i];
	}

	return NULL;
}

//
// Description:
//	Clear all entries for Fast Transmition list.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
//	[in] action -
//		The action in FT_ENTRY_ACTION.
//	[in] pTargetAddr -
//		The target address of the key of the entry to update the info.
//	[in] pInfoBuffer -
//		The updated info buffer location. This field can be NULL only if BufLen is 0.
//	[in] BufLen -
//		The length in byte of pInfoBuffer.
// Return:
//	RT_STATUS_SUCCESS if updating is OK.
//	RT_STATUS_RESOURCE if there is no enough new entry to update.
//	RT_STATUS_INVALID_LENGTH if the input BufLen is not correct.
//
RT_STATUS
FtUpdateEntryInfo(
	IN	PADAPTER			pAdapter,
	IN	FT_ENTRY_ACTION		action,
	IN	pu1Byte				pTargetAddr,
	IN	PVOID				pInfoBuffer,
	IN	u4Byte				BufLen	
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	PFT_INFO_ENTRY		pEntry = NULL;
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;

	if(BufLen > 0 && !pInfoBuffer)
	{
		RT_TRACE_F(COMP_MLME, DBG_SERIOUS, ("Invalid BufLen (%d) and pInfoBuffer (%p)\n", BufLen, pInfoBuffer));
		return RT_STATUS_INVALID_DATA;
	}

	if(NULL == (pEntry = FtGetEntry(pAdapter, pTargetAddr)))
	{
		pEntry = FtGetNewEntry(pAdapter, pTargetAddr);
	}

	if(!pEntry)
	{
		RT_TRACE_F(COMP_MLME, DBG_WARNING, ("No More entry is available!\n"));
		return RT_STATUS_RESOURCE;
	}

	RT_TRACE_F(COMP_MLME, DBG_LOUD, ("FT action = %d\n", action));
	RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "FtUpdateEntryInfo(): Addr = ", pTargetAddr);
	if(pInfoBuffer && BufLen)
	{
		RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "Data = \n", pInfoBuffer, BufLen);
	}

	switch(action)
	{
	default:
		{
			RT_TRACE_F(COMP_MLME, DBG_WARNING, ("Unknown action = %d\n", action));
		}
		break;

	case FT_ENTRY_ACTION_DEL_TARGET:
		{
			pEntry->Flag = 0;
		}
		break;

	case FT_ENTRY_ACTION_UPDATE_MDE:
		{
			if(BufLen > (MAX_MD_IE_LEN + SIZE_EID_AND_LEN))
			{
				RT_TRACE_F(COMP_MLME, DBG_WARNING, ("FT_ENTRY_ACTION_UPDATE_MDE: Input BufLen(%d) is too long\n", BufLen));
				rtStatus = RT_STATUS_INVALID_LENGTH;
				break;
			}
			if(pInfoBuffer && BufLen > 0)
			{
				PlatformMoveMemory(pEntry->MDE, pInfoBuffer, BufLen);
			}
			pEntry->MDELen = (u2Byte)BufLen;
		}
		break;

	case FT_ENTRY_ACTION_UPDATE_FTE:
		{
			if(BufLen > 0 && 
				(BufLen > (MAX_FT_IE_LEN + SIZE_EID_AND_LEN) || BufLen < (MIN_FT_IE_LEN + SIZE_EID_AND_LEN)))
			{
				RT_TRACE_F(COMP_MLME, DBG_WARNING, ("FT_ENTRY_ACTION_UPDATE_FTE: Input BufLen(%d) is invalid\n", BufLen));
				rtStatus = RT_STATUS_INVALID_LENGTH;
				break;
			}			
				
			if(pInfoBuffer && BufLen > 0)
			{
				PlatformMoveMemory(pEntry->FTE, pInfoBuffer, BufLen);
			}
			pEntry->FTELen = (u2Byte)BufLen;
		}
		break;

	case FT_ENTRY_ACTION_UPDATE_PMKR0_NAME:
		{
			if(BufLen > 0 &&  PMKID_LEN != BufLen)
			{
				RT_TRACE_F(COMP_MLME, DBG_WARNING, ("FT_ENTRY_ACTION_UPDATE_PMKR0_NAME: Input BufLen(%d) is invalid\n", BufLen));
				rtStatus = RT_STATUS_INVALID_LENGTH;
				break;
			}			
				
			if(pInfoBuffer && BufLen > 0)
			{
				PlatformMoveMemory(pEntry->PMKR0Name, pInfoBuffer, BufLen);
			}
			pEntry->PMKR0NameLen = (u2Byte)BufLen;
		}
		break;

	case FT_ENTRY_ACTION_UPDATE_RSNE:
		{				
			if(pInfoBuffer && BufLen > 0)
			{
				PlatformMoveMemory(pEntry->RSNE, pInfoBuffer, BufLen);
			}
			pEntry->RSNELen = (u2Byte)BufLen;
		}
		break;

	case FT_ENTRY_ACTION_UPDATE_ASSOC_INFO_STATUS:
		{
			if(BufLen < sizeof(u4Byte))
			{
				RT_TRACE_F(COMP_MLME, DBG_WARNING, ("FT_ENTRY_ACTION_UPDATE_ASSOC_INFO_STATUS: Invalid BufLen(%d)\n", BufLen));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
			pEntry->osAssocInfoStatus = *((pu4Byte)pInfoBuffer);
		}
		break;

	case FT_WAIT_OS_DECISION:
		{
			CLEAR_FLAG(pEntry->Flag, FT_INFO_OS_DECISION_MADE);
			SET_FLAG(pEntry->Flag, FT_INFO_WAIT_OS_DECISION);
		}
		break;

	case FT_OS_DECISION_MADE:
		{
			CLEAR_FLAG(pEntry->Flag, FT_INFO_WAIT_OS_DECISION);
			SET_FLAG(pEntry->Flag, FT_INFO_OS_DECISION_MADE);
		}
		break;
	}
	
	FunctionOut(COMP_MLME);

	return rtStatus;
}

//
// Description:
//	Return if we can choose FT authentication algorithm for this target.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
//	[in] pTargetAddr -
//		The target address to be the key.
// Return:
//	Return TRUE if the current information is enough to use FT authentication.
//
BOOLEAN
FtIsFtAuthReady(
	IN	PADAPTER			pAdapter,
	IN	pu1Byte				pTargetAddr
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u2Byte			i = 0;

	for(i = 0; i < MAX_FT_ENTRY_NUM; i ++)
	{
		if(TEST_FLAG(pMgntInfo->FtEntryList[i].Flag, FT_INFO_VALID) &&
			eqMacAddr(pMgntInfo->FtEntryList[i].targetAddr, pTargetAddr))
		{
			if(pMgntInfo->FtEntryList[i].MDELen > 0 &&
				pMgntInfo->FtEntryList[i].FTELen > 0 &&
				pMgntInfo->FtEntryList[i].PMKR0NameLen > 0)
			{
				return TRUE;
			}
		}			
	}

	return FALSE;
}

//
// Description:
//	Return if we can choose FT association req info set from the supplicant.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
//	[in] pTargetAddr -
//		The target address to be the key.
// Return:
//	Return TRUE if the current information is enough to use FT association req info.
//
BOOLEAN
FtIsFtAssocReqReady(
	IN	PADAPTER			pAdapter,
	IN	pu1Byte				pTargetAddr
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u2Byte			i = 0;

	for(i = 0; i < MAX_FT_ENTRY_NUM; i ++)
	{
		if(TEST_FLAGS(pMgntInfo->FtEntryList[i].Flag, FT_INFO_VALID | FT_INFO_OS_DECISION_MADE) &&
			eqMacAddr(pMgntInfo->FtEntryList[i].targetAddr, pTargetAddr))
		{
			if(pMgntInfo->FtEntryList[i].MDELen > 0 &&
				pMgntInfo->FtEntryList[i].FTELen > 0 &&
				pMgntInfo->FtEntryList[i].PMKR0NameLen > 0 &&
				pMgntInfo->FtEntryList[i].RSNELen)
			{
				return TRUE;
			}
		}			
	}

	return FALSE;
}

//
// Description:
//	Get the address of the entry which is waiting for the OS decision.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
//	[in] pTargetAddr -
//		The target address which is waiting for the OS decision.
//		This field is NOT filled if the returned status is not RT_STATUS_SUCCESS.
// Return:
//	RT_STATUS_SUCCESS if there is an entry waiting for decision.
//	RT_STATUS_INVALID_STATE if there is no entry in waiting decision state.
//
RT_STATUS
FtGetWaitOSDecisionAddr(
	IN	PADAPTER	pAdapter,
	OUT	pu1Byte		pAddr
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	u2Byte			i = 0;

	CHECK_NULL_RETURN_STATUS(pAddr);

	for(i = 0; i < MAX_FT_ENTRY_NUM; i ++)
	{
		if(TEST_FLAGS(pMgntInfo->FtEntryList[i].Flag, FT_INFO_VALID | FT_INFO_WAIT_OS_DECISION))
		{
			cpMacAddr(pAddr, pMgntInfo->FtEntryList[i].targetAddr);
			return RT_STATUS_SUCCESS;
		}
	}

	return RT_STATUS_INVALID_STATE;
}
