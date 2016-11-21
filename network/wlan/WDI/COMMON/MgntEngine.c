#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "MgntEngine.tmh"
#endif

//=============================================================================
//	Private Definitions
//=============================================================================
//
// Description:
//	Function prototype of action frame handler.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
//	Return RT_STATUS_PKT_DROP if this packet shall be dropped after this function finishes.
//
typedef RT_STATUS
(*RT_ACTION_CMD_HANDLER)(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
    );

typedef struct _RTL_DO11_ACTION_FRAME_CMD
{
	const ACT_PKT_TYPE				PktType;		// Action packet type
    const RT_ACTION_CMD_HANDLER		CmdHandler; 	// Command Action
    const char*						Name;			// Command Name
}RTL_DO11_ACTION_FRAME_CMD, *PRTL_DO11_ACTION_FRAME_CMD;

RTL_DO11_ACTION_FRAME_CMD	ActionFrameCmd[] =
{
	//==BA==//
	{ACT_PKT_BA_ADDBAREQ,		OnADDBAReq,				"OnADDBAReq"},
	{ACT_PKT_BA_ADDBARSP,		OnADDBARsp,				"OnADDBARsp"},
	{ACT_PKT_BA_DELBA,			OnDELBA,				"OnDELBA"},
	//==Public==//
	{ACT_PKT_BSS_COEXIST,		OnBssCoexistence,		"OnBssCoexistence"},
	//==SA Query==//
	{ACT_PKT_SA_QUERY_REQ,		OnSAQueryReq,				"OnSAQueryReq"},
	{ACT_PKT_SA_QUERY_RSP,		OnSAQueryRsp,				"OnSAQueryRsp"},
	{ACT_PKT_TDLS_DISC_RSP,		TDLS_OnDiscoveryRsp,	"TDLS_OnDiscoveryRsp"},
	//==RM==//
	//		DO NOT define function handler to NULL
	//{ACT_PKT_RM_RM_REQ,			OnDot11kRmRequest,		"OnDot11kRmRequest"},
	//==HT==//
	{ACT_PKT_HT_SM_PS,					OnMimoPs,						"OnMimoPs"},
	//==WMM==//
	{ACT_PKT_WMM_ADDTSREQ,		OnAddTsReq,				"OnAddTsReq"},
	{ACT_PKT_WMM_ADDTSRSP,		OnAddTsRsp,				"OnAddTsRsp"},
	{ACT_PKT_WMM_DELTS,			OnDelTs,				"OnDelTs"},
	//==VHT==//
	{ACT_PKT_VHT_OPMODE_NOTIFICATION,		VHT_OnOpModeNotify, 		"VHT_OpModeNotification"},

	// ===== P2P =====
#if (P2P_SUPPORT == 1)
	{ACT_PKT_P2P_GO_NEG_REQ,	P2P_OnGONReq,			"P2P_OnGONReq"},
	{ACT_PKT_P2P_GO_NEG_RSP,	P2P_OnGONRsp,			"P2P_OnGONRsp"},
	{ACT_PKT_P2P_GO_NEG_CONF,	P2P_OnGONConfirm,		"P2P_OnGONConfirm"},
	{ACT_PKT_P2P_INVIT_REQ,		P2P_OnInvitationReq,	"P2P_OnInvitationReq"},
	{ACT_PKT_P2P_INVIT_RSP,		P2P_OnInvitationRsp,	"P2P_OnInvitationRsp"},
	{ACT_PKT_P2P_DEV_DISCOVERABILITY_REQ,		P2P_OnDeviceDiscoverabilityReq,	"P2P_OnDeviceDiscoverabilityReq"},
	{ACT_PKT_P2P_PROV_DISC_REQ,	P2P_OnProvisionDiscoveryReq,	"P2P_OnProvisionDiscoveryReq"},
	{ACT_PKT_P2P_PROV_DISC_RSP,	P2P_OnProvisionDiscoveryRsp,	"P2P_OnProvisionDiscoveryRsp"},
	{ACT_PKT_P2P_PRESENCE_REQ,	P2P_OnPresenceReq,		"P2P_OnPresenceReq"},
	{ACT_PKT_P2P_GO_DISCOVERABILITY_REQ,		P2P_OnGODiscoverabilityReq,		"P2P_OnGODiscoverabilityReq"},
#endif // end #if (P2P_SUPPORT == 1)
	{ACT_PKT_GAS_INT_REQ,		GAS_OnInitReq,			"GAS_OnInitReq"},
	{ACT_PKT_GAS_INT_RSP,		GAS_OnInitRsp,			"GAS_OnInitRsp"},
	{ACT_PKT_GAS_COMEBACK_REQ,	GAS_OnComebackReq,		"GAS_OnComebackReq"},
	{ACT_PKT_GAS_COMEBACK_RSP,	GAS_OnComebackRsp,		"GAS_OnComebackRsp"},

#if (NAN_SUPPORT == 1)
	{ACT_PKT_NAN_SDF,	NAN_OnSDFReceived,	"NAN_OnSDF"},
#endif
	// ===== Add new item above this line =====
	{ACT_PKT_TYPE_UNKNOWN,		NULL,					"NULL"}
};

//
// Description:
//	Function prototype of encapsulated data frame handler.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
//	[in] contentOffset -
//		The offset from 802.11 header (MAC header + Qos + Security + HC control ..) to the content.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
//	Return RT_STATUS_PKT_DROP if this packet shall be dropped after this function finishes.
//
typedef RT_STATUS
(*RT_ENCAP_DATA_CMD_HANDLER)(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu,
	IN	u4Byte			contentOffset
    );

typedef struct _RTL_ENCAP_DATA_FRAME_CMD
{
	const ENCAP_DATA_PKT_TYPE		PktType;		// Encapsulated data packet type
    const RT_ENCAP_DATA_CMD_HANDLER	CmdHandler; 	// Command Action
    const char*						Name;			// Command Name
}RTL_ENCAP_DATA_FRAME_CMD, *PRTL_ENCAP_DATA_FRAME_CMD;

RTL_ENCAP_DATA_FRAME_CMD	EncapDataFrameCmd[] =
{
	//==TDLS==//
	{ENCAP_DATA_PKT_TDLS_SETUP_REQ,	TDLS_OnSetupReq,		"TDLS_OnSetupReq"},
	{ENCAP_DATA_PKT_TDLS_SETUP_RSP,	TDLS_OnSetupRsp,		"TDLS_OnSetupRsp"},
	{ENCAP_DATA_PKT_TDLS_SETUP_CONFIRM,	TDLS_OnSetupConfirm,	"TDLS_OnSetupConfirm"},
	{ENCAP_DATA_PKT_TDLS_TEARDOWN,	TDLS_OnTearDown,	"TDLS_OnTearDown"},
	{ENCAP_DATA_PKT_TDLS_DISC_REQ,	TDLS_OnDiscoveryReq,	"TDLS_OnDiscoveryReq"},
	{ENCAP_DATA_PKT_TDLS_TRAFFIC_INDI,	TDLS_OnTrafficInd,	"TDLS_OnTrafficInd"},
	{ENCAP_DATA_PKT_TDLS_TRAFFIC_RSP,	TDLS_OnTrafficRsp,	"TDLS_OnTrafficRsp"},
	{ENCAP_DATA_PKT_TDLS_CHNL_SW_REQ,	TDLS_OnChnlSwitchReq,	"TDLS_OnChnlSwitchReq"},
	{ENCAP_DATA_PKT_TDLS_CHNL_SW_RSP,	TDLS_OnChnlSwitchRsp,	"TDLS_OnChnlSwitchRsp"},
	{ENCAP_DATA_PKT_TDLS_PROBE_REQ,	TDLS_OnTunneledProbeReq,	"TDLS_OnTunneledProbeReq"},
	{ENCAP_DATA_PKT_TDLS_PROBE_RSP, TDLS_OnTunneledProbeRsp,	"TDLS_OnTunneledProbeRsp"},
	{ENCAP_DATA_PKT_CCX_IAPP, CCX_OnIAPPPacket, "CCX_OnIAPPPacket"},
	// ===== Add new item above this line =====
	{ENCAP_DATA_PKT_UNKNOWN,		NULL,					"NULL"}
};

//=============================================================================
//	Prototype of private functions.
//=============================================================================


//=============================================================================
//	End of Prototype of private functions.
//=============================================================================

//=============================================================================
//	Private functions.
//=============================================================================


//=============================================================================
//	End of Private functions.
//=============================================================================


RT_WLAN_BSS *BssDescDupSource(
	PADAPTER		Adapter,
//	OCTET_STRING	mmpdu
	PRT_RFD			pRfd
);

VOID
DFS_OnBeacon_Bss(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pBssDesc
);


//
//	Description:
//		Add new STA of the IBSS and indicate association start, 
//		association complete events for it.
//
BOOLEAN
AsocEntry_AddIbssPeerSta(
	IN PADAPTER			pAdapter,
	IN PRT_WLAN_BSS		pBssDesc
	)
{
	PMGNT_INFO					pMgntInfo = &(pAdapter->MgntInfo);
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	pu1Byte						pTaddr = pBssDesc->bdMacAddressBuf;
	PRT_WLAN_STA				pEntry;
	BOOLEAN						bUseRAMask = FALSE;
	BOOLEAN bAMSDUTestChk;
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T					pDM_Odm = &pHalData->DM_OutSrc;
	
	//
	// Add the STA into table and indicate association related event 
	// if this is the 1st time we hear Beacon from this STA.
	//
	if(AsocEntry_GetEntry(pMgntInfo, pTaddr) !=  NULL)
	{
		return FALSE;
	}
	
	AsocEntry_AddStation(pAdapter, pTaddr, OPEN_SYSTEM);
	pEntry = AsocEntry_GetEntry(pMgntInfo, pTaddr);
	if(pEntry != NULL)
	{
		u1Byte			i, rate;

		pEntry->AID = AsocEntry_AssignAvailableAID(pMgntInfo, pTaddr);
		pEntry->IOTPeer = pBssDesc->Vender;
		pEntry->AssociatedMacId = (u1Byte) MacIdRegisterMacIdForAssociatedID(pAdapter, MAC_ID_OWNER_AD_HOC, pEntry->AID);
		
		//YJ, add for TX RPT, 111213
		if(pMgntInfo->MaxMACID < pEntry->AssociatedMacId)
		{
			pMgntInfo->MaxMACID = (u2Byte) pEntry->AssociatedMacId;
			pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_TX_RPT_MAX_MACID, (pu1Byte)(&(pMgntInfo->MaxMACID)));
		}
		pEntry->bAssociated = TRUE;
		AsocEntry_UpdateTimeStamp(pEntry);

		//
		// Get the supported rate of the STA.
		//
		pEntry->bdSupportRateEXLen = pBssDesc->bdSupportRateEXLen;
		CopyMem(pEntry->bdSupportRateEXBuf, pBssDesc->bdSupportRateEXBuf, pEntry->bdSupportRateEXLen);
	
		//
		// Get highest supported rate of the STA.
		//
		pEntry->HighestOperaRate = pMgntInfo->LowestBasicRate & 0x7f;
		for(i = 0; i < pEntry->bdSupportRateEXLen; i++)
		{
			if( (pEntry->bdSupportRateEXBuf[i] & 0x7f) > pEntry->HighestOperaRate)
				pEntry->HighestOperaRate = pEntry->bdSupportRateEXBuf[i] & 0x7f;
		}

		// Update Qos mode!!
		pEntry->QosMode = pBssDesc->BssQos.bdQoSMode;

		// Update WirelessMode.
		if(IS_WIRELESS_MODE_24G(pAdapter))
		{
			pEntry->WirelessMode = WIRELESS_MODE_B;
			for(i = 0; i < pEntry->bdSupportRateEXLen; i++)
			{
				rate = (pEntry->bdSupportRateEXBuf[i] & 0x7f);
				if( !MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_B) &&
					MgntIsRateValidForWirelessMode(rate, WIRELESS_MODE_G) )
				{
					pEntry->WirelessMode = WIRELESS_MODE_G;
					break;
				}
			}
		}
		else
		{
			pEntry->WirelessMode = WIRELESS_MODE_A;
		}
		
		// Update HT related information
		if(pHTInfo->bCurrentHTSupport && pBssDesc->BssHT.bdSupportHT)
		{
			pu1Byte pHTCapEle = (pu1Byte)pBssDesc->BssHT.bdHTCapBuf;

			// Use tx adsud for 11n adhoc mode
			if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_NO_CIPHER)
			{
				
				bAMSDUTestChk = FALSE;
				pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_AMSDU_TEST_SETTING, (pu1Byte)(&bAMSDUTestChk));
				if(bAMSDUTestChk)
				{
					if(pHTInfo->ForcedAMSDUMode == HT_AMSDU_ENABLE)
						pEntry->IOTAction = HT_IOT_ACT_AMSDU_ENABLE;
					else if(pHTInfo->ForcedAMSDUMode == HT_AMSDU_WITHIN_AMPDU)
						pEntry->IOTAction = HT_IOT_ACT_AMSDU_AMPDU;
					else 
						pEntry->IOTAction = 0;
				}
			}

			pEntry->HTInfo.bEnableHT = TRUE;
			if(pEntry->WirelessMode == WIRELESS_MODE_A)
				pEntry->WirelessMode = WIRELESS_MODE_N_5G;
			else if(pEntry->WirelessMode == WIRELESS_MODE_G)
				pEntry->WirelessMode = WIRELESS_MODE_N_24G;

			HTCheckHTCap(pAdapter, pEntry, pHTCapEle);

			if(pVHTInfo->bCurrentVHTSupport && pBssDesc->BssVHT.bdSupportVHT)
			{
				pu1Byte pVHTCapEle = (pu1Byte)pBssDesc->BssVHT.bdVHTCapBuf;
		
				pEntry->VHTInfo.bEnableVHT = TRUE;
				if(pEntry->WirelessMode == WIRELESS_MODE_N_5G)
					pEntry->WirelessMode = WIRELESS_MODE_AC_5G;
				else
					pEntry->WirelessMode = WIRELESS_MODE_AC_24G;

				VHTCheckVHTCap(pAdapter, pEntry, pVHTCapEle);
			}
		}

		// Porting from AP_OnAsocReq() to initialize the protection mechnism in IBSS mode.
		// These part is lost before. 2010.11.02. Added by tynli.
		// Enable protection mode and Disable short slot time if an [B-mode STA joined] or[ G-mode Connect to Soft N AP]		
		if(pEntry->WirelessMode == WIRELESS_MODE_B)
		{
			ActUpdate_ProtectionMode(pAdapter, TRUE);
			
			if(pEntry->WirelessMode == WIRELESS_MODE_B)
				pMgntInfo->mCap &= (~cShortSlotTime);
			else
				pMgntInfo->mCap |= (cShortSlotTime);

			ActUpdate_mCapInfo(pAdapter, pMgntInfo->mCap);
		}

		if(	IS_WIRELESS_MODE_AC(pAdapter) &&
			pEntry->IOTPeer == HT_IOT_PEER_REALTEK_JAGUAR_CCUTAP)
		{
			u2Byte JaguarPatch = (HT_IOT_PEER_REALTEK_JAGUAR_CCUTAP << 8) | (RT_MEDIA_CONNECT);
			pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_JAGUAR_PATCH, (pu1Byte)(&JaguarPatch)); 
		}

		// Update the Operation mode field in HT Info element.
		if(IS_WIRELESS_MODE_N(pAdapter) )
		{
			if(pEntry->WirelessMode == WIRELESS_MODE_B || pEntry->WirelessMode == WIRELESS_MODE_G)
				pHTInfo->CurrentOpMode = HT_OPMODE_MIXED;
			else
			{
				if(CHNL_RUN_ABOVE_40MHZ(pMgntInfo) && (pEntry->WirelessMode == WIRELESS_MODE_N_24G))
					pHTInfo->CurrentOpMode = HT_OPMODE_40MHZ_PROTECT;
				else
					pHTInfo->CurrentOpMode = HT_OPMODE_NO_PROTECT;
					
			}
		}

		pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_USE_RA_MASK, &bUseRAMask);
		if(bUseRAMask)
		{
			AP_InitRateAdaptiveState(pAdapter, pEntry);
			pMgntInfo->Ratr_State = 0;
			pAdapter->HalFunc.UpdateHalRAMaskHandler(pAdapter, pEntry->AssociatedMacId, pEntry, pMgntInfo->Ratr_State);
		}
		else
		{
			pAdapter->HalFunc.UpdateHalRATRTableHandler(
									pAdapter, 
									&pMgntInfo->dot11OperationalRateSet,
									pMgntInfo->dot11HTOperationalRateSet,pEntry);
		}


		// Update RTS init rate after we set RA MASK. by tynli. 2010.11.25.
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_INIT_RTS_RATE, (pu1Byte)&pAdapter);
		return TRUE;
	}
	else
	{
		RT_PRINT_ADDR(COMP_MLME, DBG_WARNING, ("AsocEntry_AddIbssPeerSta(): this station is already here: \n"), pTaddr);
		return FALSE;
	}
}



VOID
OnBeacon_Scan(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;

	pMgntInfo->LinkDetectInfo.OnBeaconScanCnt_OnBeacon_Scan++;	//cosa add for debug.
	GetScanInfo( Adapter, pRfd);
}


void 
ChangeRsnIEOnSameBssid(
	PADAPTER		Adapter,
	PRT_RFD			pRfd,
	u1Byte			Authtype
)
{
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;
	OCTET_STRING  	RSNIE;
	OCTET_STRING	mmpdu;
	u1Byte	*pCurr;
	u1Byte	count;
	u1Byte	PairwiseCipherCount = 0;	
				
	mmpdu.Octet = pRfd->Buffer.VirtualAddress;
	mmpdu.Length = pRfd->PacketLength;

	RSNIE.Octet = NULL;
	RSNIE.Length = 0;

	if( pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA2 )
	{
		RSNIE = PacketGetElement(mmpdu, EID_WPA2, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	}
	else if( pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA )
	{
		RSNIE = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_WPA, OUI_SUBTYPE_DONT_CARE );
	}

	if( RSNIE.Length == 0 )
		return;

	//RT_PRINT_DATA(COMP_INDIC , DBG_LOUD,  " Change :\n" , mmpdu.Octet , mmpdu.Length);
	RT_TRACE(COMP_INDIC , DBG_LOUD , (" Authtype : %d\n",Authtype) );

	RT_PRINT_DATA(COMP_INDIC, DBG_LOUD,  " Change :\n" , RSNIE.Octet , RSNIE.Length);
	pCurr = RSNIE.Octet;

	// SKIP Head 
	if( pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA)
	{
		pCurr+=4;
		pCurr+=2;
	}
	else if(pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA2)
	{
		pCurr+=2;
	}

	// SKIP Group Chiper
	pCurr += 3;
	pCurr += 1;


	// SKIP Uncase Chiper
	PairwiseCipherCount = *pCurr;
	pCurr+=2; //  Munber two byte

	if(PairwiseCipherCount >0 )
	{
		for(count = 0; count < PairwiseCipherCount; count++)
		{
			pCurr+=4;
		}
	}

	pCurr+=2;
	// Get AKM suite
	RT_PRINT_DATA(COMP_INDIC , DBG_LOUD , " AKM data: \n " , pCurr  , 4 );
	pCurr[3] = Authtype;
	RT_PRINT_DATA(COMP_INDIC , DBG_LOUD , "After AKM data: \n " , pCurr  , 4 );
}


VOID
OnBeacon_Join(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;	
	PRT_WLAN_BSS	pBssDesc = NULL; // Reduce stack size

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	pMgntInfo->LinkDetectInfo.OnBeaconJoinCnt_OnBeacon_Join++;	//cosa add for debug.

	pBssDesc = &pMgntInfo->bssDesc_OnBeacon;
	
	GetValueFromBeaconOrProbeRsp( Adapter, pRfd, pBssDesc, FALSE);

	RT_PRINT_ADDR(COMP_MLME|COMP_BEACON, DBG_TRACE, "===> OnBeacon_Join():", pBssDesc->bdBssIdBuf);
	RT_TRACE(COMP_MLME, DBG_TRACE, ("OnBeacon_Join(): channel  = %d\n", pBssDesc->ChannelNumber));

	//
	// 061026, Rcnjko: 
	// 1. 8186 doesn't include Realtek CAP IE in ProbeRsp and therefore we
	// determine bRealtekCapType1Exist   here instead of SetupJoinInfraInfo. 
	// 2. awlays keep Realtek CAP IE. We might use it 
	// for 8186 Auto Turbo mode or other compatibility issue.
	// 3. 8186 include this IE in Beacon when its Turbo Mode is configured 
	// as "Auto" or "Always".
	// Added by Roger, 2006.12.08.
	//
	pMgntInfo->bRealtekCapType1Exist = pBssDesc->bRealtekCapType1Exist  ;
	pMgntInfo->bRealtekAggCapExist = pBssDesc->bRealtekAggCapExist;

	if(pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE)
	{ // Infra-structure mode.
		// For Hidden AP and EzConfig AP, their beacons won't contain the SSID 
		// we want, and therefore, either BSSID or SSID matched is acceptable.
		// 2004.11.18, by rcnjko.
		if( PlatformCompareMemory( pMgntInfo->Bssid, pBssDesc->bdBssIdBuf, 6) != 0 ) // BSSID mismatched
		{
			return;
		}
	}
	else
	{ // IBSS or others.
		if( PlatformCompareMemory( pMgntInfo->Bssid, pBssDesc->bdBssIdBuf, 6) != 0 || // BSSID mismatched.
			!CompareSSID(pBssDesc->bdSsIdBuf, pBssDesc->bdSsIdLen, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length)) // SSID mismatchec.
		{
			return;
		}
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}

	CopyWlanBss(&(pMgntInfo->targetBSS), pBssDesc);
	
	RT_PRINT_ADDR(COMP_MLME|COMP_BEACON, DBG_LOUD, "===> OnBeacon_Join():", pBssDesc->bdBssIdBuf);	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("OnBeacon_Join(): channel  = %d\n", pBssDesc->ChannelNumber));

	//Update Dtim period.
	if(pMgntInfo->dot11DtimPeriod != pBssDesc->bdDtimPer)
	{
		pMgntInfo->dot11DtimPeriod = pBssDesc->bdDtimPer;
	}
	if(pMgntInfo->dot11PowerSaveMode !=eFastPs)
	{
		pMgntInfo->ListenInterval = pMgntInfo->dot11DtimPeriod;
	}
	
	// Update beacon and peer information, 2006.11.14, by shien chang.
	MgntUpdateAsocInfo(Adapter, UpdateAsocBeacon, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, pBssDesc->bdMacAddressBuf, 6);

	CCX_SSIDLUpdateJoinBss(Adapter, pRfd, pBssDesc);
	
	pMgntInfo->TSFValue = pBssDesc->bdTstamp; // by tynli.

	//Set state_Synchronization_Sta
	if( pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE )
	{
		// For receiving beacon and preventing wrong restoration in ScanComplete() -------------------------------------
		pMgntInfo->state_Synchronization_Sta = pMgntInfo->state_Synchronization_Sta_BeforeScan = STATE_Bss;
		RT_TRACE_F(COMP_SCAN, DBG_LOUD, ("Set Port (%d) pMgntInfo->state_Synchronization_Sta = %d\n", 
				GET_PORT_NUMBER(Adapter), Adapter->MgntInfo.state_Synchronization_Sta)
			);
		// ---------------------------------------------------------------------------------------------------
	}
	else if( pMgntInfo->OpMode == RT_OP_MODE_IBSS )
	{
		if(pMgntInfo->pHTInfo->bCurrentHTSupport == TRUE)
		{
			OCTET_STRING	pduOS; 
			FillOctetString(pduOS, pMgntInfo->AsocInfo.Beacon, (u2Byte)pMgntInfo->AsocInfo.BeaconLength);
			HTOnAssocRsp(Adapter, pduOS);	

			if(pMgntInfo->pVHTInfo->bCurrentVHTSupport)
				VHTOnAssocRsp(Adapter, pduOS);

			CHNL_SetBwChnlFromPeer(Adapter);
		}
		else
		{
			PlatformZeroMemory(pMgntInfo->dot11HTOperationalRateSet, 16);
			CHNL_SetBwChnl(Adapter, pMgntInfo->dot11CurrentChannelNumber, CHANNEL_WIDTH_20, EXTCHNL_OFFSET_NO_EXT);	
		}

		if(MgntResetOrPnPInProgress(Adapter))
		{
			RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case IBSS\n"));
			return;
		}
		
		//
		// 061205, rcnjko: Add new STA of the IBSS and indicate association start, 
		// association complete events for it.
		//
		if(AsocEntry_AddIbssPeerSta(Adapter, pBssDesc))
		{
			// Preprocessing for association operation. 2006.11.14, by shien chang.
			DrvIFIndicateAssociationStart(Adapter);
		
			// Postprocessing for association operation. 2006.11.14, by shien chang.
			DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_SUCCESS);
		}

		// Postprocessing for completion of a connection request, 2006.10.24, by shien chang.

		if(MgntResetOrPnPInProgress(Adapter))
		{
			RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case IBSS 2\n"));
			return;
		}
		
		// <Roger_Notes> Sync from 91su branch, 2009.04.27.
		if( !pMgntInfo->bInToSleep && 
			!MgntRoamingInProgress(pMgntInfo) && pMgntInfo->RequestFromUplayer)
			DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_SUCCESS);
		else if(MgntRoamingInProgress(pMgntInfo))
		{
			MgntRoamComplete(Adapter, MlmeStatus_success);
		}

		pMgntInfo->bInToSleep = FALSE;	
		pMgntInfo->state_Synchronization_Sta = STATE_Ibss_Idle;

		Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_LINK); 	
	}
	else
	{
		RT_TRACE(COMP_MLME, DBG_LOUD,("OnBeacon_Join(): Unknown OP mode\n"));
	}

	// This function move to here for RATR table setting when IBSS.
	MgntLinkStatusUpdateRxBeacon( Adapter );
	PlatformCancelTimer( Adapter, &pMgntInfo->JoinTimer);
	PlatformCancelTimer( Adapter, &pMgntInfo->JoinProbeReqTimer);
	if(pMgntInfo->pChannelInfo->bSwBwInProgress)
		PlatformSetTimer(Adapter,&pMgntInfo->JoinConfirmTimer, 200);
	else
		PlatformSetTimer(Adapter,&pMgntInfo->JoinConfirmTimer, 0);

	TDLS_CheckCapability(Adapter);

	//PlatformFreeMemory(pBssDesc, sizeof(RT_WLAN_BSS));
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== OnBeacon_Join()\n"));
}


VOID
OnBeacon_Bss(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	PADAPTER 		pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;
	PRT_WLAN_BSS	pBssDesc = NULL;
	pu1Byte			pbssid = pRfd->Buffer.VirtualAddress + 16;
		
	pMgntInfo->LinkDetectInfo.OnBeaconBssCnt_OnBeacon_Bss++;	//cosa add for debug
	
	if( pMgntInfo->mAssoc == FALSE )
		return;

	// Check BSSID.
	if( PlatformCompareMemory( pMgntInfo->Bssid, pbssid, 6) != 0 )
	{
		// BSSID mismatched.
		return;
	}

	pBssDesc = &pMgntInfo->bssDesc_OnBeacon;

	

	if(pDefaultAdapter->MgntInfo.RegMultiChannelFcsMode == MULTICHANNEL_FCS_SUPPORT_GO)
	{
		u8Byte		u8APTsf = 0;
		u8Byte		uCurrentTsf = 0;
				
		u8APTsf = GET_BEACON_PROBE_RSP_TIME_STAMP_HIGH(pRfd->Buffer.VirtualAddress);
		u8APTsf <<= 32;
		u8APTsf |= GET_BEACON_PROBE_RSP_TIME_STAMP_LOW(pRfd->Buffer.VirtualAddress);

		uCurrentTsf = (((u8Byte)pRfd->Status.TimeStampHigh) << 32) + ((u8Byte)pRfd->Status.TimeStampLow);

		MultiChannel_OnBss(pDefaultAdapter, u8APTsf, uCurrentTsf);

	}

	GetValueFromBeaconOrProbeRsp( Adapter, pRfd, pBssDesc, FALSE);

	DFS_StaCheckCsaInfo(Adapter, pBssDesc);

// We should not compare SSID for Hidden AP case. 2005.04.14, by rcnjko.
//	// Check SSID.
//	if( !CompareSSID(bssDesc.bdSsIdBuf, bssDesc.bdSsIdLen, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length) ) 
//	{
//		// SSID mismatched.
//		return;
//	}	
//

	// Filter beacon from IBSS, 2006.11.22, by shien chang.
	if ( (pBssDesc->bdCap & cESS) == 0)
	{
		//PlatformFreeMemory(pBssDesc, sizeof(RT_WLAN_BSS));
		return;
	}

	//Update Dtim period.
	if(pMgntInfo->dot11DtimPeriod != pBssDesc->bdDtimPer)
	{
		pMgntInfo->dot11DtimPeriod = pBssDesc->bdDtimPer;
	}
	if(pMgntInfo->dot11PowerSaveMode !=eFastPs)
	{
		pMgntInfo->ListenInterval = pMgntInfo->dot11DtimPeriod;
	}
	
	// Update beacon informaton, 2006.11.14, by shien chang.
	// Just Update on Join step !! 
	//MgntUpdateAsocInfo(Adapter, UpdateAsocBeacon, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

	// For WMM QoS Info Field update (WMM Power Save Mode). Annie, 2005-11-14.
        // Isaiah move it
	if( pMgntInfo->pStaQos->CurrentQosMode > QOS_DISABLE )
	{
		QosOnBeaconUpdateParameter( Adapter, pBssDesc );
	}

	// For HT Capability and HT information Element update.
	if( pMgntInfo->pHTInfo->bCurrentHTSupport )
	{
		HTUpdateSelfAndPeerSetting( Adapter, pBssDesc );
		if(pMgntInfo->pVHTInfo->bCurrentVHTSupport)
			VHTUpdateSelfAndPeerSetting(Adapter, pBssDesc);

		if( PlatformIsWorkItemScheduled(&(pMgntInfo->ChangeBwChnlFromPeerWorkitem)) == FALSE)
		{
			//RT_TRACE(COMP_MLME, DBG_LOUD, ("ScheduleWorkItem -> ChangeBwChnlFromPeerWorkitem.\n"));
			PlatformScheduleWorkItem(&(pMgntInfo->ChangeBwChnlFromPeerWorkitem));
		}
	}

	ActUpdate_mCapInfo( Adapter, pBssDesc->bdCap );	// Uncommented for slot time setting in Wireless mode auto. By Annie, 2005-10-31.
	if(pBssDesc->bERPInfoValid)
	{
		ActUpdate_ERPInfo( Adapter, pBssDesc->bdERPInfo);
	}

	if(!GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS)
	{
	        //
		// Parse TIM in Beacon for ps mode, 070109, by rcnjko.
		//
		if( pMgntInfo->mAssoc &&
			pMgntInfo->dot11PowerSaveMode != eActive ) // We are configured to power save mode.
		{
			OCTET_STRING	mmpdu;

			mmpdu.Octet = pRfd->Buffer.VirtualAddress;
			mmpdu.Length = pRfd->PacketLength;
			if(IN_LEGACY_POWER_SAVE(pMgntInfo->pStaQos))
			{
				LPS_OnBeacon_BSS(Adapter, mmpdu);
			}
			else
			{
				WMMPS_OnBeacon_BSS(Adapter, mmpdu);
			}
		}
	}


	//-------------------------------------------------------------------------
	// For 8186 Auto-Turbo mode. Added by Roger, 2006.12.07.
	if( pMgntInfo->bRealtekCapType1Exist   && pBssDesc->bRealtekCapType1Exist   )
	{
		// I don't want to enter original 8187 turbo mechanism in this case. Added by Roger, 2006.12.07.
		// Still need to check the MgntActSet_EnterTurboModeByStyle function(should be moved!!).
		
		RT_ASSERT( (!pMgntInfo->bSupportTurboMode), ("OnBeacon_Bss(): bSupportTurboMode is TRUE in 8186 Auto Turbo Mdoe!!!\n") );

		if(  GET_RTIE_CAPABILITY_TURBO_MODE(&(pBssDesc->RealtekCap) ) )
		{ // Auto Turbo Mode by 8186.
			BOOLEAN		bEnterTurbo = !GET_RTIE_CAPABILITY_DISABLE_TURBO(&(pBssDesc->RealtekCap));
			PRT_TURBO_CA	pTurboCa = &(pMgntInfo->TurboChannelAccess);

			// For debug, added by Roger, 2006.12.11.
			RT_TRACE(COMP_MLME, DBG_TRACE, 
					("OnBeacon_Bss(): Auto Turbo Mode by 8186.\n"));
			RT_TRACE(COMP_MLME, DBG_TRACE, 
					("OnBeacon_Bss():bEnterTurbo = %d, pTurboCa->bEnabled = %d\n",bEnterTurbo, pTurboCa->bEnabled));

			if( bEnterTurbo != pTurboCa->bEnabled )
			{					
				// For debug, added by Roger, 2006.12.11.
				RT_TRACE(COMP_MLME, DBG_TRACE, 
					("OnBeacon_Bss(): Enter Turbo Mode by 8186.\n"));
				//Modefied by Roger, 2006.12.07.
				MgntActSet_EnterTurboMode( Adapter, bEnterTurbo );
			}
		}
	}
	
	//-------------------------------------------------------------------------

	MgntLinkStatusUpdateRxBeacon( Adapter );
	//PlatformFreeMemory(pBssDesc, sizeof(RT_WLAN_BSS));
}




VOID
OnBeacon_Ibss(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	OCTET_STRING	pduOS; 
	PRT_WLAN_BSS	pBssDesc = NULL;
	pu1Byte			pbssid, pTaddr;
	PRT_WLAN_STA	pEntry;
	PRT_SECURITY_T		pSecurityInfo = &(Adapter->MgntInfo.SecurityInfo);
	RT_ENC_ALG			PairwiseEncAlgo = pSecurityInfo->PairwiseEncAlgorithm;
	PRT_CHNL_LIST_ENTRY	pChnlListEntry = NULL;
	OCTET_STRING		mmpdu;
	RT_WLAN_BSS 	*bssDescTmp;

	mmpdu.Octet = pRfd->Buffer.VirtualAddress;
	mmpdu.Length = pRfd->PacketLength;

	RT_TRACE(COMP_MLME, DBG_TRACE, ("=======> OnBeacon_Ibss!\n"));

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return;		
	}

	pMgntInfo->LinkDetectInfo.OnBeaconIbssCnt_OnBeacon_Ibss++;	//cosa add for debug

	FillOctetString(pduOS, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	pbssid = Frame_Addr3(pduOS);
	pTaddr = Frame_Addr2(pduOS);
		
	// Skip it if we are not in IBSS mode.
	if( pMgntInfo->mIbss == FALSE )
	{
		return;
	}
	
	bssDescTmp = BssDescDupSource( Adapter, pRfd );

	pBssDesc = &pMgntInfo->bssDesc_OnBeacon;

	// Retrive information from beacon.
	GetValueFromBeaconOrProbeRsp( Adapter, pRfd, pBssDesc, FALSE);

	// Check SSID.
	if( !CompareSSID(pBssDesc->bdSsIdBuf, pBssDesc->bdSsIdLen, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length) )
	{
		return;
	}

	// Check bss type, 2006.11.22, by shien chang.
	if ( (pBssDesc->bdCap & cIBSS) == 0)
	{
		return;
	}

	//
	// Get channel info and determine if this channel is valid to join.
	//
	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL, &(pBssDesc->ChannelNumber), &pChnlListEntry);

	// Invalid channel number.
	if(!pChnlListEntry)
	{
		return;
	}
	
	if(pChnlListEntry->ExInfo & CHANNEL_EXINFO_NO_IBSS_JOIN)
	{
		return;
	}

	if( (pBssDesc->bdCap & cPrivacy) )
	{
		if(PairwiseEncAlgo == RT_ENC_ALG_NO_CIPHER )
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("OnBeacon_Ibss(): BSS Cap shows privacy, but PairwiseEncAlgo is RT_ENC_ALG_NO_CIPHER\n"));
			return;
		}
	}
	else
	{
		if( PairwiseEncAlgo != RT_ENC_ALG_NO_CIPHER )
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("OnBeacon_Ibss(): BSS Cap shows no privacy, but PairwiseEncAlgo is %d\n", PairwiseEncAlgo));
			return;
		}
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return;
	}	

	// Check BSSID.
	if( PlatformCompareMemory( pMgntInfo->Bssid, pbssid, 6) == 0 )
	{ // Normal Case, BSSID and SSID matched.

		// Update beacon information, 2006.11.14, by shien chang.
		MgntUpdateAsocInfo(Adapter, UpdateAsocBeacon, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

		MgntLinkStatusUpdateRxBeacon( Adapter );

		pEntry = AsocEntry_GetEntry(pMgntInfo, pTaddr);
		if(pEntry != NULL)
		{ 
			// Update information about this STA. 
			AsocEntry_UpdateTimeStamp(pEntry);
		}
		else
		{ 

			RT_TRACE_F(COMP_MLME, DBG_LOUD, ("MgntUpdateAsocInfo"));
		
			MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, pTaddr, 6); // 061204, rcnjko: this is a MUST before indicating association related event.

			if(MgntResetOrPnPInProgress(Adapter))
			{
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));
				return;
			}
			
			//
			// 061205, rcnjko: Add new STA of the IBSS and indicate association start, 
			// association complete events for it.
			//
			if(AsocEntry_AddIbssPeerSta(Adapter, pBssDesc))
			{
				// Preprocessing for association operation. 2006.11.14, by shien chang.
				DrvIFIndicateAssociationStart(Adapter);

				// Postprocessing for association operation. 2006.11.14, by shien chang.
				DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_SUCCESS);
				Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_LINK); 
			}
		}
		// Fixed by CCW, Bruce It will cause beacon channel number error when media connect 
		if( !pMgntInfo->bMediaConnect )
		{
			MgntIndicateMediaStatus( Adapter, RT_MEDIA_CONNECT, GENERAL_INDICATE );		
		}
		else
		{
			WAPI_OptionHandler(Adapter, pTaddr,FALSE, FALSE, FALSE, FALSE, FALSE,FALSE);
		}


		if( bssDescTmp == NULL )
		{
			do
			{
				// Add a new bssdesc entry
				if( pMgntInfo->NumBssDesc < MAX_BSS_DESC)
				{
					bssDescTmp = &pMgntInfo->bssDesc[pMgntInfo->NumBssDesc];
				}
				else
				{
					RT_TRACE(COMP_SCAN, DBG_WARNING, ("OnBeacon_Ibss(): ERROR! No bssdesc entry.\n"));
					//return;
					break;
				}

				// ShuChen TODO: Check Broadcom AP's 1M-Short Preamble issue.
				GetValueFromBeaconOrProbeRsp( Adapter, pRfd, bssDescTmp, FALSE);

				// Determine the receiving packet type for BSS weighting.
				bssDescTmp->BssPacketType |= 
					(PacketGetType(mmpdu) == Type_Probe_Rsp) ? BSS_PKT_PROBE_RSP : BSS_PKT_BEACON;

				CopyWlanBss(pMgntInfo->bssDesc4Query+pMgntInfo->NumBssDesc, pMgntInfo->bssDesc+pMgntInfo->NumBssDesc);
				pMgntInfo->NumBssDesc += 1;			
				pMgntInfo->NumBssDesc4Query = pMgntInfo->NumBssDesc;
		
			}while(0);
		}
	}
	else
	{ // SSID matched but BSSID mismatched.

		u8Byte	u8CurrTsf;
		RT_TRACE(COMP_MLME, DBG_LOUD, ("OnBeacon_Ibss() SSID matched but BSSID mismatched.\n"));
		u8CurrTsf = pRfd->Status.TimeStampHigh;
		u8CurrTsf <<= 32;
		u8CurrTsf |= pRfd->Status.TimeStampLow;

		// Check TSF, 2005.02.02, by rcnjko.
		if(u8CurrTsf <pBssDesc->bdTstamp)
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, (" OnBeacon_Ibss() Merge two IBSSs. \n"));
			// Merge two IBSSs. 2004.10.12, by rcnjko.
			pMgntInfo->JoinAction = RT_JOIN_IBSS;

			pMgntInfo->bInToSleep = TRUE;
			
			JoinRequest( Adapter, pMgntInfo->JoinAction, pBssDesc );
		}
	}
}



BOOLEAN
OnBeacon(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING	osMpdu =  {pRfd->Buffer.VirtualAddress, pRfd->PacketLength};
	
	RT_TRACE(COMP_RECV, DBG_TRACE,("OnBeacon!!!\n"));

	pMgntInfo->LinkDetectInfo.OnBeaconCnt_OnBeacon++;	//cosa add for debug.

#if (MULTICHANNEL_SUPPORT == 1)
{
	OCTET_STRING osPacket = {NULL, 0};
	osPacket.Length = pRfd->PacketLength;
	osPacket.Octet = pRfd->Buffer.VirtualAddress;

	MultiChannelHandleReceivedBeacon(Adapter, osPacket);

	if(eqMacAddr(Frame_pBssid(osPacket), pMgntInfo->Bssid))
	{
		RT_PRINT_ADDR(COMP_P2P, DBG_TRACE, "OnBeacon(): Client Associated BSSID: ", Adapter->MgntInfo.Bssid);
		RT_TRACE_F(COMP_P2P, DBG_TRACE, ("pMgntInfo->state_Synchronization_Sta: %d\n", pMgntInfo->state_Synchronization_Sta));
	}
}
#endif

	switch( pMgntInfo->state_Synchronization_Sta )
	{
		case STATE_Act_Receive:
		case STATE_Pas_Listen:
		case STATE_Act_Listen:
				OnBeacon_Scan( Adapter, pRfd );
				break;
		case STATE_Join_Wait_Beacon:
				OnBeacon_Join( Adapter, pRfd );
				break;
		case STATE_Bss:
				OnBeacon_Bss( Adapter, pRfd );
				break;

		case STATE_Ibss_Active:
		case STATE_Ibss_Idle:
				OnBeacon_Ibss( Adapter, pRfd );
				break;

		default:
				break;
	}

	P2P_OnBeacon(Adapter, pRfd, &osMpdu);
	WFD_OnBeacon(Adapter, pRfd, &osMpdu);
	NAN_OnBeaconReceived(Adapter, pRfd, &osMpdu);

	return TRUE;
}

VOID 
OnProbeRsp(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	OCTET_STRING	osMpdu = {NULL, 0};
		
	if(Adapter->MgntInfo.state_Synchronization_Sta == STATE_Join_Wait_Beacon)
	{
		RT_TRACE( COMP_MLME , DBG_LOUD , ( " Used ProbeRsp to Join!! \n" ) );
		OnBeacon_Join(  Adapter,  pRfd);
	}
	else
	{
		GetScanInfo( Adapter, pRfd );
	}

	FillOctetString(osMpdu, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	CustomScan_OnProbeRspCb(GET_CUSTOM_SCAN_INFO(Adapter), pRfd);
	P2P_OnProbeRsp(Adapter, pRfd, &osMpdu);
	WFD_OnProbeRsp(Adapter, pRfd, &osMpdu);
}






BOOLEAN
OnAuth_even(
	PADAPTER		Adapter,
	OCTET_STRING	authpdu
)
{
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;
	AUTH_ALGORITHM	auth_alg = OPEN_SYSTEM;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE; 	
	}
	
//Check Bssid
	if( PlatformCompareMemory( pMgntInfo->Bssid, Frame_Addr3( authpdu ), 6 ) != 0 ){
		return FALSE;
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===>OnAuth_even(): State_AuthReqService %d \n", pMgntInfo->State_AuthReqService));


	switch( pMgntInfo->State_AuthReqService )
	{
		case STATE_Wait_Auth_Seq_2:
			{
				if( Frame_AuthTransactionSeqNum(authpdu) != 2){
					return FALSE;
				}

				//TODO: Need to check source address??

				if( Frame_AuthStatusCode(authpdu) != StatusCode_success ){
					
					//
					// <Roger_Notes> If Responding station does NOT support the specified WEP Auth,
					// we shall NOT retry the same WEP Auth algorithm any more.
					// By Netgear Lancelot's request, added by Roger, 2007.11.23.
					//
					if((Frame_AuthStatusCode(authpdu) == StatusCode_notsupport_authalg) &&
						(pMgntInfo->SecurityInfo.AuthMode != RT_802_11AuthModeAutoSwitch) &&
						(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 || 
					 	pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104))
					{
						pMgntInfo->AuthRetryCount = RT_AUTH_RETRY_LIMIT;				
					}
					pMgntInfo->State_AuthReqService = STATE_Auth_Req_Idle;
					MlmeAuthenticateRequest_Confirm(Adapter, MlmeStatus_refused);
					return FALSE;
				}

				MgntUpdateAsocInfo(Adapter, UpdateAuthSeq2, authpdu.Octet, authpdu.Length);

				//Check Auth algorithm
				auth_alg = (AUTH_ALGORITHM)Frame_AuthAlgorithmNum(authpdu);
				if( ((pMgntInfo->AuthReq_auAlg == OPEN_SYSTEM)
					&& (auth_alg == OPEN_SYSTEM ) )|| 
					( (auth_alg == NETWORK_EAP) &&  Adapter->MgntInfo.bNETWORKEAP)) //For NETWORK EAP
				{
					pMgntInfo->State_AuthReqService = STATE_Auth_Req_Idle;
					MlmeAuthenticateRequest_Confirm(Adapter, MlmeStatus_success);
				}			
				else if( (pMgntInfo->AuthReq_auAlg == SHARED_KEY)
					&& (auth_alg == SHARED_KEY) )
				{
					OCTET_STRING	AuthChallengetext;
							
					// Cancel AuthTimer
					PlatformCancelTimer( Adapter, &pMgntInfo->AuthTimer );

					AuthChallengetext = PacketGetElement( authpdu, EID_Ctext, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);

					//Send 3rd auth frame
					SendAuthenticatePacket( Adapter, pMgntInfo->Bssid, SHARED_KEY, 3, StatusCode_success, AuthChallengetext );

					PlatformSetTimer( Adapter, &pMgntInfo->AuthTimer, AUTH_REQ_TIMEOUT );

					pMgntInfo->State_AuthReqService = STATE_Wait_Auth_Seq_4;
				}
				else if(AUTH_ALG_FT == auth_alg)
				{
					RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
					
					pMgntInfo->State_AuthReqService = STATE_Auth_Req_Idle;
					FtUpdateEntryInfo(Adapter, FT_WAIT_OS_DECISION, pMgntInfo->Bssid, NULL, 0);
					// Indicate to WDI to request the association request information for FT
					PlatformIndicateCustomStatus(
						Adapter,
						RT_CUSTOM_EVENT_WDI_FT_ASSOC_NEEDED,
						RT_CUSTOM_INDI_TARGET_WDI,
						&rtStatus,
						(u4Byte)sizeof(RT_STATUS));
					//MlmeAuthenticateRequest_Confirm(Adapter, MlmeStatus_success);
				}
				else{
					pMgntInfo->State_AuthReqService = STATE_Auth_Req_Idle;
					RT_TRACE(COMP_MLME, DBG_LOUD, ("Mismatch auth_alg: %#X\n", auth_alg));
					MlmeAuthenticateRequest_Confirm(Adapter, MlmeStatus_invalid);					
				}
				
			}
			break;

		case STATE_Wait_Auth_Seq_4:
			{
				if( Frame_AuthTransactionSeqNum(authpdu) != 4 ){
					return FALSE;
				}

				//ShuChen TODO: receive 4th auth frame
				auth_alg = (AUTH_ALGORITHM)Frame_AuthAlgorithmNum(authpdu);
				if( !((pMgntInfo->AuthReq_auAlg == SHARED_KEY)
					&& (auth_alg == SHARED_KEY))  )
				{
					pMgntInfo->State_AuthReqService = STATE_Auth_Req_Idle;
					RT_TRACE(COMP_INIT, DBG_LOUD,("Mismatch auth_alg\n"));
					MlmeAuthenticateRequest_Confirm(Adapter, MlmeStatus_invalid);				
					return FALSE;
				}

				if( Frame_AuthStatusCode(authpdu) != StatusCode_success ){
	//				StaState(Adapter, auSta, StationState_not_auth);
					pMgntInfo->State_AuthReqService = STATE_Auth_Req_Idle;
					MlmeAuthenticateRequest_Confirm(Adapter, MlmeStatus_refused);
					return FALSE;
				}

	//			StaState(Adapter, auSta , StationState_auth_sharedkey);
				pMgntInfo->State_AuthReqService = STATE_Auth_Req_Idle;
				MlmeAuthenticateRequest_Confirm(Adapter, MlmeStatus_success);
			}
			break;

		default:
			break;
	}

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<====OnAuth_even(): State_AuthReqService %d \n", pMgntInfo->State_AuthReqService));
	
//Check Status Code
	return TRUE;
}








BOOLEAN
OnAuth_odd(
	PADAPTER		Adapter,
	OCTET_STRING	authpdu
)
{
	return TRUE;
}









BOOLEAN
OnAssocRsp(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;
	OCTET_STRING	asocpdu;
	BOOLEAN		FlagReAsoc = FALSE;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}
 
	asocpdu.Octet = pRfd->Buffer.VirtualAddress;
	asocpdu.Length = pRfd->PacketLength;

	RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "Addr = ", Frame_Addr2(asocpdu));

	//2004/08/23, kcwu, for ndtest
	pMgntInfo->RspCapability = Frame_AssocCap(asocpdu);
	pMgntInfo->RspStatusCode = Frame_AssocStatusCode(asocpdu);
	pMgntInfo->RspAssociationID = Frame_AssocAID(asocpdu);	

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}	

	switch( pMgntInfo->State_AsocService )
	{
		case STATE_Wait_Asoc_Response:
				{
					//Check source address
					if( PlatformCompareMemory( Frame_Addr2(asocpdu),  pMgntInfo->Bssid, 6) != 0 )	{
						return FALSE;
					}

					// Update association information, 2006.11.14, by shien chang.
					MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, pMgntInfo->Bssid, 6);
					MgntUpdateAsocInfo(Adapter, UpdateAsocResp, asocpdu.Octet, asocpdu.Length);
					MgntUpdateAsocInfo(Adapter, UpdateFlagReAsocResp, (UCHAR*)&FlagReAsoc, sizeof(BOOLEAN));
					
					RT_TRACE(COMP_MLME, DBG_LOUD, ("OnAssocRsp(): STATE_Wait_Asoc_Response => RspStatusCode: %d, RspAssociationID: %d\n", pMgntInfo->RspStatusCode, pMgntInfo->RspAssociationID));
					MlmeAssociateConfirm( Adapter, pRfd, &asocpdu, FALSE, MlmeStatus_success );
				}
				break;

		case STATE_Wait_Reasoc_Response:
			RT_TRACE(COMP_MLME, DBG_LOUD, ("OnAssocRsp(): STATE_Wait_Reasoc_Response => RspStatusCode: %d, RspAssociationID: %d\n", pMgntInfo->RspStatusCode, pMgntInfo->RspAssociationID));
			break;

		case STATE_Asoc_Idle:
			RT_TRACE(COMP_MLME, DBG_LOUD, ("OnAssocRsp(): STATE_Asoc_Idle => RspStatusCode: %d, RspAssociationID: %d\n", pMgntInfo->RspStatusCode, pMgntInfo->RspAssociationID));
			break;

		default:
			break;
	}

	return TRUE;
}



BOOLEAN
OnReAssocRsp(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;
	OCTET_STRING	Reasocpdu;
	BOOLEAN		FlagReAsoc = TRUE;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}
 
	Reasocpdu.Octet = pRfd->Buffer.VirtualAddress;
	Reasocpdu.Length = pRfd->PacketLength;

	RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "Addr = ", Frame_Addr2(Reasocpdu));

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}

	switch( pMgntInfo->State_AsocService )
	{
		case STATE_Wait_Asoc_Response:
				//20070202 David . 3 com AP always send Reassocation
				//RT_TRACE(COMP_MLME, DBG_LOUD, ("OnReAssocRsp(): STATE_Wait_Asoc_Response => RspStatusCode: %d, RspAssociationID: %d\n", Frame_AssocStatusCode(Reasocpdu), Frame_AssocAID(Reasocpdu)));
				//break;

		case STATE_Wait_Reasoc_Response:
				{
					//Check source address
					if( PlatformCompareMemory( Frame_Addr2(Reasocpdu),  pMgntInfo->Bssid, 6) != 0 )	{
						return FALSE;
					}

					// Update reassociation information, 2006.11.14, by shien chang.
					MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, pMgntInfo->Bssid, 6);
					MgntUpdateAsocInfo(Adapter, UpdateAsocResp, Reasocpdu.Octet, Reasocpdu.Length);
					MgntUpdateAsocInfo(Adapter, UpdateFlagReAsocResp, (UCHAR*)&FlagReAsoc, sizeof(BOOLEAN));
					
					RT_TRACE(COMP_MLME, DBG_LOUD, ("OnReAssocRsp(): STATE_Wait_Reasoc_Response => RspStatusCode: %d, RspAssociationID: %d\n", Frame_AssocStatusCode(Reasocpdu), Frame_AssocAID(Reasocpdu)));
					MlmeAssociateConfirm( Adapter, pRfd, &Reasocpdu, TRUE, MlmeStatus_success );
				}
			break;

		case STATE_Asoc_Idle:
			RT_TRACE(COMP_MLME, DBG_LOUD, ("OnReAssocRsp(): STATE_Asoc_Idle\n"));
			break;

		default:
			break;
	}

	return TRUE;
}



BOOLEAN
OnDisassoc(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
)
{
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;
	OCTET_STRING	disassocpdu;
 	pu1Byte			disassocBssid;
	pu1Byte			disassocSta;
	u2Byte 			disassocReason;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	pMgntInfo->LinkDetectInfo.OnDisassocCnt++;	//cosa add for debug
	
	disassocpdu.Octet = pRfd->Buffer.VirtualAddress;
	disassocpdu.Length = pRfd->PacketLength;

	disassocBssid = Frame_pBssid(disassocpdu);
	disassocSta = Frame_Addr2(disassocpdu);

	if(pMgntInfo->bSetPnpPwrInProgress)
		return TRUE;

	//
	// Verify if this packet length is valid, by Bruce, 2008-07-01.
	//
	if(pRfd->PacketLength < (sMacHdrLng + DISASSOC_REASON_SIZE))
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("OnDisassoc(): packet too short(%d) < (needed = %d)!\n", pRfd->PacketLength, (sMacHdrLng + DISASSOC_REASON_SIZE)));
		return FALSE;
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}
	
	disassocReason = Frame_DeassocReasonCode(disassocpdu);

	if(!ACTING_AS_AP(Adapter))
	{ // STA mode.
		if( PlatformCompareMemory( pMgntInfo->Bssid, disassocBssid, 6 ) == 0 )
		{
			u1Byte			tmp_ssid[256];
			u2Byte			tmp_len = pMgntInfo->Ssid.Length;
			
			if(!pMgntInfo->bMediaConnect || pMgntInfo->bDisconnectRequest)
			{
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("bMediaConnect %d bDisconnectRequest %d \n", pMgntInfo->bMediaConnect, pMgntInfo->bDisconnectRequest));
				return FALSE;
			}
			
			RT_TRACE(COMP_MLME, DBG_LOUD, ("OnDisassoc in STA mode (mAssoc%d, mIbss:%d)\n", pMgntInfo->mAssoc, pMgntInfo->mIbss));
			RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "OnDisassoc from\n", disassocSta);

			if((disassocReason<unspec_reason) ||(disassocReason>dest_not_QSTA) )
			{
				RT_TRACE(COMP_MLME, DBG_LOUD, ("OnDisassoc in STA mode with WRONG disassoc reason %x\n", disassocReason));
				return TRUE;
			}

			// We should leave LPS mode first. 2011.03.23. by tynli.
			if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS)
			{
				LeisurePSLeave(Adapter, LPS_DISABLE_ON_DISASSOC);
			}

			// Remember the rejected AP, if another AP in the same ESS is better,
			// pick the better one in further connection. 2006.11.21, by shien chang.
			MgntAddRejectedAsocAP(Adapter, pMgntInfo->Bssid);

			MultiChannelDisconnectClient(Adapter, FALSE);

			P2P_ClientOnDisassoc(Adapter, pRfd, &disassocpdu);

			//
			// <Roger_Notes> We should complete all SendNBLs before NDIS_STATUS_DOT11_DISASSOCIATION status is indicated
			// to prevent MiniportReturnNetBufferLists routine will be called in the same thread context and Rx deadlock in N6PciReturnNetBufferLists
			// routine might cause bug check code DPC_WATCHDOG_VIOLATION (133).
			//
			// Workaround for Win8 and later OS.	Added by tynli. 2013.09.03.
			ReleaseDataFrameQueued(Adapter);

			if(MgntResetOrPnPInProgress(Adapter))
			{
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));
				return FALSE;
			}

			// 2006.11.15, by shien chang.
			DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_FAILURE);
			DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);
			RT_TRACE(COMP_MLME, DBG_LOUD, ("OnDisassoc(): ReasonCode = %X\n", Frame_DeassocReasonCode(disassocpdu)));
			DrvIFIndicateDisassociation(Adapter, Frame_DeassocReasonCode(disassocpdu), pMgntInfo->Bssid);
			
			//2004/08/23, kcwu, the ssid should be retained
			PlatformMoveMemory(tmp_ssid, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);
			tmp_len = pMgntInfo->Ssid.Length;
			
			ResetMgntVariables( Adapter );

			//Fix WPA WPA2 issue , New key will set after re-handshaking, by CCW 20070521
			{
				BOOLEAN		bAPSuportCCKM = FALSE;
				BOOLEAN		bCCX8021xenable = FALSE;

				CCX_QueryCCKMSupport(Adapter, &bCCX8021xenable, &bAPSuportCCKM);
				
				if(   pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch ||
					(bAPSuportCCKM && bCCX8021xenable) )  // In CCKM mode will Clear key
				{
					SecClearAllKeys(Adapter);
					RT_TRACE(COMP_SEC, DBG_LOUD,("OnDisassoc():======>CCKM clear key..."));
				}
			}

			PlatformMoveMemory(pMgntInfo->Ssid.Octet, tmp_ssid, tmp_len);
			pMgntInfo->Ssid.Length = tmp_len;
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pMgntInfo->Bssid );
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode) );
	
			// Start Roaming and do not indicate status to OS (XP), so we still keep connected until retry failed. By Bruce, 2008-08-16.
			// CCW said: we must indicate DISCONNECT when received DEAUTH frme include 4-way timeout. 2008-11-24
			if( pMgntInfo->mDeauthCount > 2 || pMgntInfo->IndicateByDeauth)
			{
				MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );
				pMgntInfo->mDeauthCount = 0;
			}
			else
			{
				MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, FORCE_NO_INDICATE);
			}

			if(MgntResetOrPnPInProgress(Adapter))
			{
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));
				return FALSE;
			}
			
			if( OS_SUPPORT_WDI(Adapter) == FALSE)
			{
				// Roaming, 2006.12.07, by shien chang.
				if(!MgntRoamingInProgress(pMgntInfo))
				{
					// Run as roaming to connect to the original AP. By Bruce, 2008-06-09.
					MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_DEAUTH, ROAMINGSTATE_SCANNING);
					DrvIFIndicateRoamingStart(Adapter);

					CCX_SetAssocReason(Adapter, CCX_AR_DEAUTH_DISASSOC);

					// Roaming retry
					if(!MgntRoamRetry(Adapter, FALSE)) // Raom again.
					{
						if(MgntResetOrPnPInProgress(Adapter))
						{
							RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 3\n"));				
							return FALSE;
						}
						MgntRoamComplete(Adapter, MlmeStatus_refused); // Roam failed.
					}
				}		
			}
		}
	}
	else
	{ // AP mode.
		if( PlatformCompareMemory( pMgntInfo->Bssid, disassocBssid, 6 ) == 0 )
		{
			pu1Byte			disassocSta = Frame_pSaddr(disassocpdu);
			PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, disassocSta);

			if(pEntry != NULL)
			{
				// Added by Annie, 2005-07-12.
				if( pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPAPSK 
					||pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPA2PSK
					||pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPA)
					Authenticator_StateINITIALIZE( Adapter, pEntry );
					
				//
				// <Roger_Notes> We should complete all SendNBLs before NDIS_STATUS_DOT11_DISASSOCIATION status is indicated
				// to prevent MiniportReturnNetBufferLists routine will be called in the same thread context and Rx deadlock in N6PciReturnNetBufferLists
				// routine might cause bug check code DPC_WATCHDOG_VIOLATION (133).
				// 2013.06.26.
				//
				AsocEntry_BecomeDisassoc(Adapter, pEntry);
					
				if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED
				|| MgntActQuery_ApType(Adapter) == RT_AP_TYPE_LINUX)
				{
					MgntUpdateAsocInfo(Adapter, UpdateDeauthAddr, disassocSta, 6); 
					DrvIFIndicateDisassociation(Adapter, Frame_DeassocReasonCode(disassocpdu), disassocSta);
				}
				else if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP)
				{
					pMgntInfo->pCurrentSta = pEntry;
					RT_TRACE(COMP_MLME, DBG_LOUD, ("OnDisassoc(): on VWIFI_AP ReasonCode = %X\n", Frame_DeassocReasonCode(disassocpdu)));
					
					DrvIFIndicateIncommingAssociationComplete(Adapter, Frame_DeassocReasonCode(disassocpdu));
					DrvIFIndicateDisassociation(Adapter, Frame_DeassocReasonCode(disassocpdu), disassocSta);
				}
				//AsocEntry_BecomeDisassoc(Adapter, pEntry);
			}
		}
	}

	RemovePeerTS(Adapter, Frame_pSaddr(disassocpdu));
	
	return TRUE;
}



BOOLEAN
OnDeauth(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd
	)
{
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;	
	OCTET_STRING	deauthpdu;
	pu1Byte			deauBssid;
	u1Byte			QueueID;
	PRT_TCB			pTcb = NULL;
	
	pu1Byte			deauSta;
 	u1Byte			tmp_ssid[256];
	u2Byte			tmp_len = pMgntInfo->Ssid.Length;
	PRT_WLAN_STA	pEntry = NULL;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

 	pMgntInfo->LinkDetectInfo.OnDeauthCnt++;	//cosa add for debug
 
	deauthpdu.Octet = pRfd->Buffer.VirtualAddress;
	deauthpdu.Length = pRfd->PacketLength;

	deauBssid = Frame_pBssid(deauthpdu);
	deauSta = Frame_Addr2(deauthpdu);	

	if(!IsDefaultAdapter(Adapter)&& 
		!ACTING_AS_AP(Adapter) )
	{
		// Is it possible failing into this condition???? Neo Test 123
		// need to set assert here ?
		return TRUE;
	}

	if(pMgntInfo->bSetPnpPwrInProgress)
		return TRUE;

	//
	// Verify if this packet length is valid, by Bruce, 2008-07-01.
	//
	if(pRfd->PacketLength < (sMacHdrLng + DEAUTH_REASON_SIZE))
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, ("OnDeauth(): packet too short(%d) < (needed = %d)!\n", pRfd->PacketLength, (sMacHdrLng + DEAUTH_REASON_SIZE)));
		return FALSE;
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return FALSE;
	}	

	//RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "deauth:\n", deauthpdu.Octet, deauthpdu.Length);

	if( !ACTING_AS_AP(Adapter))
	{ // STA mode.

		if( !MacAddr_isBcst(deauBssid)&&((PlatformCompareMemory( pMgntInfo->Bssid, deauBssid, 6 ) == 0) 
			|| MgntIsMacAddrGroup( pMgntInfo->Bssid )))
		{ 
			switch(pMgntInfo->OpMode)
			{
				case RT_OP_MODE_IBSS:
					if(pMgntInfo->bDisconnectRequest)
					{
						RT_TRACE_F(COMP_MLME, DBG_LOUD, ("bMediaConnect %d bDisconnectRequest %d \n", pMgntInfo->bMediaConnect, pMgntInfo->bDisconnectRequest));
						break;
					}
					
					RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "OnDeauth in IBSS by deauSta: \n", deauSta);

					pEntry = AsocEntry_GetEntry(pMgntInfo, deauSta);
					if(pEntry == NULL)
					{
						RT_TRACE(COMP_MLME, DBG_LOUD, ("OnDeauth in IBSS receive duplicate deauth \n"));
						break;
					}
					
					//
					// We have to indicate the address of leaving peer STA to upper layer. 
					// for example, authenticator/supplicant.
					//
					MgntUpdateAsocInfo(Adapter, UpdateDeauthAddr, deauSta, 6); 
					DrvIFIndicateDisassociation(Adapter, Frame_DeauthReasonCode(deauthpdu), deauSta);

					//
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
						MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, DummySta, 6);
						DrvIFIndicateAssociationStart(Adapter);
						DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_SUCCESS);
					}
#endif
					RT_TRACE_F(COMP_AP, DBG_LOUD, ("AsocEntry_RemoveStation\n"));

					AsocEntry_RemoveStation(Adapter, deauSta);

					break;

				case RT_OP_MODE_INFRASTRUCTURE:
				default:
					if(!pMgntInfo->bMediaConnect || pMgntInfo->bDisconnectRequest)
					{
						RT_TRACE_F(COMP_MLME, DBG_LOUD, ("bMediaConnect %d bDisconnectRequest %d \n", pMgntInfo->bMediaConnect, pMgntInfo->bDisconnectRequest));				
						break;
					}
					
					RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "OnDeauth in infra-BSS by deauSta: \n", deauSta);
					RT_TRACE(COMP_MLME, DBG_LOUD, ("OnDeauth() reason %d\n",Frame_DeauthReasonCode(deauthpdu)));

					// We should leave LPS mode first. 2011.03.23. by tynli.
					if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS)
					{
						LeisurePSLeave(Adapter, LPS_DISABLE_ON_DEAUTH);
					}

					// Remember the rejected AP, if another AP in the same ESS is better,
					// pick the better one in further connection. 2006.11.21, by shien chang.
					if( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm > RT_ENC_ALG_NO_CIPHER )
					{
						if( SecIsTxKeyInstalled( Adapter,pMgntInfo->Bssid)  )
						{
							MgntAddRejectedAsocAP(Adapter, pMgntInfo->Bssid);
						}
					}
					else
					{
						MgntAddRejectedAsocAP(Adapter, pMgntInfo->Bssid);
					}

					P2P_ClientOnDeauth(Adapter, pRfd, &deauthpdu);

					//
					// <Roger_Notes> We should complete all SendNBLs before NDIS_STATUS_DOT11_DISASSOCIATION status is indicated
					// to prevent MiniportReturnNetBufferLists routine will be called in the same thread context and Rx deadlock in N6PciReturnNetBufferLists
					// routine might cause bug check code DPC_WATCHDOG_VIOLATION (133).
					//
					// Workaround for Win8 and later OS.	Added by tynli. 2013.09.03.
					ReleaseDataFrameQueued(Adapter);

					if(MgntResetOrPnPInProgress(Adapter))
					{
						RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 2\n"));				
						return FALSE;
					}
					
					// 2006.11.15, by shien chang.
					DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_FAILURE);
					DrvIFIndicateConnectionComplete(Adapter, RT_STATUS_FAILURE);
					DrvIFIndicateDisassociation(Adapter, Frame_DeauthReasonCode(deauthpdu), pMgntInfo->Bssid);
					
					//2004/08/23, kcwu, the ssid should be retained
					PlatformMoveMemory(tmp_ssid, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);
					tmp_len = pMgntInfo->Ssid.Length;
		
					if(GET_TDLS_ENABLED(pMgntInfo))
						TDLS_Stop(Adapter);

					// Start Roaming and do not indicate status to OS (XP), so we still keep connected until retry failed. By Bruce, 2008-08-16.
					// CCW said: we must indicate DISCONNECT when received DEAUTH frme include 4-way timeout. 2008-11-24
					if( pMgntInfo->mDeauthCount > 2 || pMgntInfo->IndicateByDeauth)
					{
						MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );
						pMgntInfo->mDeauthCount = 0;
					}
					else
					{
						MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, FORCE_NO_INDICATE);
					}
					
					ResetMgntVariables( Adapter );

					//Fix WPA WPA2 issue , New key will set after re-handshaking, by CCW 20070521
					{
						BOOLEAN		bAPSuportCCKM = FALSE;
						BOOLEAN		bCCX8021xenable = FALSE;

						CCX_QueryCCKMSupport(Adapter, &bCCX8021xenable, &bAPSuportCCKM);
						
						if(   pMgntInfo->SecurityInfo.AuthMode > RT_802_11AuthModeAutoSwitch ||
							(bAPSuportCCKM && bCCX8021xenable) )  // In CCKM mode will Clear key
						{
							SecClearAllKeys(Adapter);
							RT_TRACE(COMP_SEC, DBG_LOUD,("OnDeauth():======>CCKM clear key..."));
						}
					}

					PlatformMoveMemory(pMgntInfo->Ssid.Octet, tmp_ssid, tmp_len);
					pMgntInfo->Ssid.Length = tmp_len;
					Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pMgntInfo->Bssid );
					Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode) );

					if(MgntResetOrPnPInProgress(Adapter))
					{
						RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 3\n"));				
						return FALSE;
					}
				
					if( OS_SUPPORT_WDI(Adapter) == FALSE )
					{
						// Roaming, 2006.12.07, by shien chang.
						if(!MgntRoamingInProgress(pMgntInfo))
						{
							// Run as roaming to connect to the original AP. By Bruce, 2008-06-09.
							MgntLinkStatusSetRoamingState(Adapter, 0, RT_ROAMING_BY_DEAUTH, ROAMINGSTATE_SCANNING);
							DrvIFIndicateRoamingStart(Adapter);

							//  Note :
							//  SSID List Maybe not clear in Before !!
							//  FilterHiddenAP can't judge Add or not !!
							//  Side-effic : We can connect Hidde AP after OnDeauth !!
							if( pMgntInfo->FilterHiddenAP  == FALSE )
							{
								RT_TRACE( COMP_MLME , DBG_LOUD , ("===> Add SSID to SCAN List ") );
								MgntAddSsidsToScan(  Adapter, pMgntInfo->Ssid );
							}

							CCX_SetAssocReason(Adapter, CCX_AR_DEAUTH_DISASSOC);

							// Roaming retry
							if(!MgntRoamRetry(Adapter, FALSE)) // Raom again.
							{
								if(MgntResetOrPnPInProgress(Adapter))
								{
									RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress MgntRoamRetry FAIL\n"));				
									return FALSE;
								}	
								MgntRoamComplete(Adapter, MlmeStatus_refused); // Roam failed.
							}						
						}
					}	
				
					//do not send buffered packets while STA is deauthed by AP.
					PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);		
					for(QueueID = 0; QueueID < MAX_TX_QUEUE; QueueID++)
					{
						// 2004.08.11, revised by rcnjko.
						while(!RTIsListEmpty(&Adapter->TcbWaitQueue[QueueID]))
						{
							pTcb=(PRT_TCB)RTRemoveHeadList(&Adapter->TcbWaitQueue[QueueID]);
							ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
						}
					}
					for(QueueID = 0; QueueID < MAX_TX_QUEUE; QueueID++)
					{
						while(!RTIsListEmpty(&Adapter->TcbAggrQueue[QueueID]))
						{
							pTcb = (PRT_TCB)RTRemoveHeadList(&Adapter->TcbAggrQueue[QueueID]);
							Adapter->TcbCountInAggrQueue[QueueID]--;
							ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
						}
					}
					PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
						
					break;
			}	
		}
	}
	else
	{ // AP mode.
		if( PlatformCompareMemory( pMgntInfo->Bssid, deauBssid, 6 ) == 0) 
		{
			if(	MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED
				|| MgntActQuery_ApType(Adapter) == RT_AP_TYPE_LINUX)
			{
				MgntUpdateAsocInfo(Adapter, UpdateDeauthAddr, deauSta, 6); 
				pEntry = AsocEntry_GetEntry(pMgntInfo, deauSta);
				if( pEntry != NULL )
					DrvIFIndicateDisassociation(Adapter, Frame_DeauthReasonCode(deauthpdu), deauSta);
			}
			else if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP)
			{
				pMgntInfo->pCurrentSta = AsocEntry_GetEntry(pMgntInfo, deauSta);
				if(pMgntInfo->pCurrentSta != NULL)
				{
					if(pMgntInfo->pCurrentSta->bDisassociated)
					{
						RT_TRACE_F(COMP_AP, DBG_LOUD, ("AP_DisassociateStation in progress\n"));
						return TRUE;
					}
				
					DrvIFIndicateIncommingAssociationComplete(Adapter, Frame_DeauthReasonCode(deauthpdu));
					if(pMgntInfo->pCurrentSta->bAssociated)
					{
						//
						// <Roger_Notes> We should complete all SendNBLs before NDIS_STATUS_DOT11_DISASSOCIATION status is indicated
						// to prevent MiniportReturnNetBufferLists routine will be called in the same thread context and Rx deadlock in N6PciReturnNetBufferLists
						// routine might cause bug check code DPC_WATCHDOG_VIOLATION (133).
						// 2013.06.26.
						//
						AsocEntry_BecomeDisassoc(Adapter, pMgntInfo->pCurrentSta);

						// HalMacId Remove the specific MacId associated with this peer ---------
						MacIdDeregisterSpecificMacId(Adapter, pMgntInfo->pCurrentSta->AssociatedMacId);

						RT_TRACE(COMP_MLME, DBG_LOUD, ("OnDeauth(): ReasonCode = %X\n", Frame_DeauthReasonCode(deauthpdu)));
						
						DrvIFIndicateDisassociation(Adapter, Frame_DeauthReasonCode(deauthpdu), pMgntInfo->pCurrentSta->MacAddr);
					}					
				}
			}
			RT_TRACE_F(COMP_AP, DBG_LOUD, ("AsocEntry_RemoveStation case 2\n"));
			
			AsocEntry_RemoveStation(Adapter, deauSta);
		}
	}

	return TRUE;
}


//
//	Description:
//		Handle the Probe Request received.
//	2004.10.07, by rcnjko.
//
BOOLEAN
OnProbeReq(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
	)
{
	PMGNT_INFO		pMgntInfo;
	OCTET_STRING	mmpdu;
	OCTET_STRING	ssidToScan;
	u1Byte	SrcAddr[6];
	BOOLEAN			bSentProbRsp = FALSE;

	RT_TRACE(COMP_RECV, DBG_TRACE, ("=======> OnProbeReq!\n"));

	pMgntInfo = &(Adapter->MgntInfo);
	mmpdu.Octet = pRfd->Buffer.VirtualAddress;
	mmpdu.Length = pRfd->PacketLength;
	PlatformMoveMemory(SrcAddr, Frame_Addr2(mmpdu), 6);

	do
	{
		//3 //Note, the sequence of conditions are very important.
		// Skip ProbeReq if SSID is neither any nor our SSID. 
		ssidToScan = PacketGetElement(mmpdu, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
		
		// Check if this packet should be handled by P2P
		if(RT_STATUS_SUCCESS != P2P_OnProbeReq(Adapter, pRfd, &mmpdu))
		{
			// RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "OnProbeReq(): Skip by P2P_OnProbeReq(), Addr = ", SrcAddr);
			break;
		}

		// Skip ProbeReq if we are not in AP mode or we are not a coordinator of IBSS mode.
		if( !(ACTING_AS_AP(Adapter)) && 
			!(pMgntInfo->mIbss && !pMgntInfo->mDisable && pMgntInfo->bIbssCoordinator) )
		{
			break;
		}

		// Skip ProbeReq if we are scanning. 
		if( pMgntInfo->bScanInProgress && (Adapter->MgntInfo.dot11CurrentChannelNumber != RT_GetChannelNumber(Adapter)))
		{
			// RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "OnProbeReq(): Skip by ScanInProgress, Addr = ", SrcAddr);
			// RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Adapter->MgntInfo.dot11CurrentChannelNumber (%d) != RT_GetChannelNumber(Adapter) (%d)\n",
			//	Adapter->MgntInfo.dot11CurrentChannelNumber, RT_GetChannelNumber(Adapter)));
			break;
		}

		// Skip ProbeReq if we are stop beacon to aviod client fast roaming and get error probe response.
		// 1. PNP sleep 2. During SendDisassocWithOldChnl in AP mode.
		if(pMgntInfo->BeaconState == BEACON_STOP)
		{
			// RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "OnProbeReq(): Skip by pMgntInfo->BeaconState == BEACON_STOP, Addr = ", SrcAddr);
			break;
		}

		if(P2P_IsGoAcceptProbeReq(Adapter, &mmpdu, &ssidToScan))
		{
			bSentProbRsp = TRUE;
			break;
		}
		else
		{
			// RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "OnProbeReq(): Skip by P2P_IsGoAcceptProbeReq == BEACON_STOP, Addr = ", SrcAddr);
		}

		// Skip ProbeReq if SSID is neither any nor our SSID. 
		if( ssidToScan.Length > 0 && EqualOS(ssidToScan, pMgntInfo->Ssid) == FALSE)
			break;		
	}while(FALSE);

	if(bSentProbRsp)
	{
		// Send ProbeRsp to respond the ProbeReq received.
		SendProbeRsp(Adapter, SrcAddr, IsSSIDAny(ssidToScan), pRfd, &mmpdu);
	}
	
	return TRUE;
}


VOID 
MlmeDeauthenticateRequest(
	PADAPTER		Adapter,
	pu1Byte			auSta,
	u1Byte			asRsn
	)
{
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;

	FunctionIn(COMP_MLME);

	// 2010.11.09 Sync with Association operation. Emily
	RemovePeerTS(Adapter, auSta);

	SendDeauthentication( Adapter, auSta, asRsn );

	if((PlatformCompareMemory( pMgntInfo->Bssid, auSta, 6 ) == 0) ||MgntIsMacAddrGroup( pMgntInfo->Bssid ))
	{
		u1Byte		i;

		DrvIFIndicateDisassociation(Adapter, unspec_reason, pMgntInfo->Bssid);
		
		pMgntInfo->mAssoc = FALSE;
		pMgntInfo->AsocTimestamp = 0;
		for(i=0;i<6;i++)  pMgntInfo->Bssid[i] = 0x22;	//0x11;	// Modified to 0x22 by Annie, I don't want it set addr[0] BIT0. 2006-05-04.
		pMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
		Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode) );
		MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );
		Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pMgntInfo->Bssid );
	}

}



VOID
MlmeDisassociateRequest(
	PADAPTER		Adapter,
	pu1Byte			asSta,
	u1Byte			asRsn
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	FunctionIn(COMP_MLME);

	RemovePeerTS(Adapter, asSta);

	if(!ACTING_AS_AP(Adapter))	
	{
		if(pMgntInfo->RoamingState == ROAMINGSTATE_IDLE)
			SendDisassociation( Adapter, asSta, asRsn );
	}
	else
	{
		AP_DisassociateAllStation(Adapter, unspec_reason);
	}

	if(PlatformCompareMemory( pMgntInfo->Bssid, asSta, 6 ) == 0)
	{
		u1Byte i;

		if(pMgntInfo->bMediaConnect)
			DrvIFIndicateDisassociation(Adapter, asRsn, pMgntInfo->Bssid);

		if(!MgntRoamingInProgress(pMgntInfo))
			MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );
		
		pMgntInfo->mAssoc = FALSE;
		pMgntInfo->AsocTimestamp = 0;
		for(i=0;i<6;i++)  pMgntInfo->Bssid[i] = 0x22;
		pMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
		Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode) );
		
		if(pMgntInfo->RoamingState == ROAMINGSTATE_IDLE)
			MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE );

		Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pMgntInfo->Bssid );
	}
}



VOID
OnCls2err(
	PADAPTER		Adapter,
	pu1Byte			auSta)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	RT_TRACE( COMP_RECV, DBG_LOUD, (" ==> OnCls2err()\n") );

	if( ACTING_AS_AP(Adapter) )
	{ // AP mode.
 		PRT_WLAN_STA	pEntry;
		BOOLEAN			BAssoiciated;

		// 1. Send deauthentiation frame to the STA.
		SendDeauthentication( Adapter, auSta, class2_err );

		// 2. Get STA entry.
		pEntry = AsocEntry_GetEntry(pMgntInfo, auSta);
		if( pEntry == NULL )
		{
			RT_TRACE( COMP_RECV, DBG_LOUD, ("OnCls2err(): Cannot find the STA in the table!!! drop it.\n") );
			return;
		}

		BAssoiciated = pEntry->bAssociated;

		// 3. Mark the STA as disassoicated and related.
		AsocEntry_BecomeDisassoc(Adapter, pEntry);
		if(BAssoiciated)
			MacIdDeregisterSpecificMacId(Adapter, pEntry->AssociatedMacId);

		// 4. To initialize WPA-PSK key mgnt state machine. Added by Annie, 2005-07-15.
		if( Adapter->MgntInfo.SecurityInfo.AuthMode == RT_802_11AuthModeWPA )
		{
			Authenticator_StateDISCONNECTED(Adapter, pEntry);
		}

		if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP)
		{
			pMgntInfo->pCurrentSta = pEntry;
			DrvIFIndicateIncommingAssociationComplete(Adapter, unspec_reason);
		}

		// 5. Remove whole entry.
		RT_TRACE_F(COMP_AP, DBG_LOUD, ("AsocEntry_RemoveStation case 3\n"));
		
		AsocEntry_RemoveStation( Adapter, auSta );
	}
	else
	{ // TODO: STA mode.
		//MlmeDeauthenticateRequest(Adapter, auSta, class2_err);
	}

	RT_TRACE( COMP_RECV, DBG_LOUD, (" <== OnCls2err()\n") );	
}



VOID
OnCls3err(
	PADAPTER		Adapter,
	pu1Byte			asSta)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	RT_TRACE( COMP_RECV, DBG_LOUD, (" ==> OnCls3err()\n") );

	if( ACTING_AS_AP(Adapter))
	{ // AP mode.
 		PRT_WLAN_STA	pEntry;

		// 1. Send disassoc frame to the STA.
		SendDisassociation(Adapter, asSta, class3_err );

		// 2. Get STA entry.
		pEntry = AsocEntry_GetEntry(pMgntInfo, asSta);
		if( pEntry == NULL )
		{
			RT_TRACE( COMP_RECV, DBG_LOUD, ("OnCls3err(): Cannot find the STA in the table!!! drop it.\n") );
			return;
		}

		// 3. Update its timestamp.
		AsocEntry_UpdateTimeStamp(pEntry);

		// 4. Mark the STA as disassoicated and related.
		AsocEntry_BecomeDisassoc(Adapter, pEntry);

		// 5. To initialize WPA-PSK key mgnt state machine. Added by Annie, 2005-07-15.
		if( Adapter->MgntInfo.SecurityInfo.AuthMode == RT_802_11AuthModeWPA )
		{
			Authenticator_StateDISCONNECTED(Adapter, pEntry);
		}
	}
	else
	{ // TODO: STA mode.
		//MlmeDisassociateRequest(Adapter, asSta, class3_err);
	}

	RT_TRACE( COMP_RECV, DBG_LOUD, (" <== OnCls3err()\n") );
}

//
// Description:
//	Handle action packet.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	This parses the patterns of action frames and determine which type it is.
//	By the packet type, it calls the corresponding function.
// By Bruce, 2012-03-12.
//
RT_STATUS
OnAction(
	IN	PADAPTER 		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	ACT_PKT_TYPE	pktType = ACT_PKT_TYPE_UNKNOWN;
	u1Byte			idx = 0;
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T					pDM_Odm = &pHalData->DM_OutSrc;

	//
	// Note:
	//	Most types of action frame are issued for the specified BSS (depend on BSSID), but some like 
	//	measurement pilot of Public Action frame similar to a subset of a beacon is allowed to receive
	//	for any station when performing a scan process.
	//	If anyone implements such action frames, we shall not check the BSSID here. By Bruce, 2009-07-17.
	//

	// [Accept Action Frame as AP mode] If the Address is correct and having Entry associated, we will process Action
	// Frame received, 2009.08.20, by Bohn.

	// Note:
	//	We shall determine each packet in each packet process function.

	pktType = PacketGetActionFrameType(posMpdu);
	if(ACT_PKT_TYPE_UNKNOWN == pktType)
	{
		// Move here by ylb 20130516 to print HT NDPA

		return RT_STATUS_NOT_RECOGNIZED;
	}

	// Retrieve each packet type and find the matching handler
	for(idx = 0; ACT_PKT_TYPE_UNKNOWN != ActionFrameCmd[idx].PktType; idx ++)
	{
		if(ActionFrameCmd[idx].PktType == pktType)
		{
			rtStatus = ActionFrameCmd[idx].CmdHandler(pAdapter, pRfd, posMpdu);
			if(RT_STATUS_SUCCESS != rtStatus)
			{
				RT_TRACE_F(COMP_MLME, DBG_WARNING, 
						("[WARNING] Execute %s with status = %d\n",  ActionFrameCmd[idx].Name, rtStatus));
			}
			break;
		}
	}	
	
	return rtStatus;
}

VOID
Mgnt_Indicate(
	PADAPTER			Adapter,
	PRT_RFD				pRfd,
	u1Byte				error_code
	)
{
	OCTET_STRING	mmpdu;

	mmpdu.Octet = pRfd->Buffer.VirtualAddress;
	mmpdu.Length = pRfd->PacketLength;

	if( error_code == class2 )
	{
		pu1Byte	auSta = mmpdu.Octet+10;
		OnCls2err( Adapter, auSta );
		return;
	}
	else if( error_code == class3 )
	{
		pu1Byte	asSta = mmpdu.Octet+10;
		OnCls3err( Adapter, asSta );
		return;
	}

	switch( PacketGetType(mmpdu) ){

	case Type_Asoc_Rsp:
			if(PacketCheckIEValidity(mmpdu,pRfd))
			{
				OnAssocRsp( Adapter, pRfd );
			}
			break;
	case Type_Reasoc_Rsp:
			if(PacketCheckIEValidity(mmpdu,pRfd))
			{
				OnReAssocRsp( Adapter, pRfd );
			}
			break;

	case Type_Probe_Rsp:
			if(PacketCheckIEValidity(mmpdu,pRfd))
			{
				OnProbeRsp( Adapter, pRfd );
			}
			break;
	case Type_Beacon:
			if(PacketCheckIEValidity(mmpdu,pRfd))
			{
				OnBeacon( Adapter, pRfd );
			}
			break;

	case Type_Auth:
			if( (Frame_AuthTransactionSeqNum( mmpdu )%2) == 0 )
			{
				RT_TRACE(COMP_MLME, DBG_TRACE,("auth_even\n"));
				OnAuth_even( Adapter, mmpdu );
			}
			else
			{
				RT_TRACE(COMP_MLME, DBG_TRACE,("auth_odd\n"));
				if(ACTING_AS_AP(Adapter))
				{
					AP_OnAuthOdd(Adapter, mmpdu);
				}
				else
				{
					OnAuth_odd( Adapter, mmpdu );
				}
			}
			RT_TRACE(COMP_MLME, DBG_TRACE,("auth\n"));
			break;
	case Type_Disasoc:
			OnDisassoc( Adapter, pRfd );
			RT_TRACE(COMP_MLME, DBG_TRACE,("disassoc\n"));
			break;
	case Type_Deauth:
			OnDeauth( Adapter, pRfd );
			RT_TRACE(COMP_MLME, DBG_TRACE,("deauth\n"));
			break;

	case Type_Probe_Req:
			OnProbeReq( Adapter, pRfd );
			break;

	case Type_Asoc_Req:
	case Type_Reasoc_Req:
			if(ACTING_AS_AP(Adapter))
			{
				AP_OnAsocReq(Adapter, pRfd, mmpdu);
			}
			break;

	case Type_Action:
	case Type_Action_No_Ack:
			OnAction(Adapter, pRfd, &mmpdu);
			break;

	default:
			break;
	}
}

RT_STATUS
EncapDataFrame_ParsePkt(
	IN	PADAPTER	pAdapter,
	IN	PRT_RFD		pRfd
	)
{
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;
	OCTET_STRING		osMpdu, osContent;
	ENCAP_DATA_PKT_TYPE	encDataType = ENCAP_DATA_PKT_UNKNOWN;
	u1Byte				contentOffset = sMacHdrLng;
	u4Byte				idx = 0;

	osMpdu.Octet = pRfd->Buffer.VirtualAddress;
	osMpdu.Length = pRfd->PacketLength;

	if(!IsDataFrame(osMpdu.Octet))
	{ // It's not a data frame
		return RT_STATUS_SUCCESS;
	}

	if(IsQoSDataFrame(osMpdu.Octet))
	{
		contentOffset += sQoSCtlLng;	// +2
	}

	if(Frame_WEP(osMpdu))
	{
		if(Frame_FromDS(osMpdu))
		{
			contentOffset += pAdapter->MgntInfo.SecurityInfo.EncryptionHeadOverhead;	// 4 or 8
		}
		else
		{
			contentOffset += 8;
		}
	}

	if(pRfd->PacketLength <= contentOffset)
	{ // No content
		return RT_STATUS_SUCCESS;
	}

	FillOctetString(osContent, osMpdu.Octet + contentOffset, (osMpdu.Length - contentOffset));

	if(ENCAP_DATA_PKT_UNKNOWN == (encDataType = PacketGetEncapDataFrameType(&osContent)))
	{ // Cnnot recognize this frame
		return RT_STATUS_NOT_RECOGNIZED;
	}

	rtStatus = RT_STATUS_NOT_RECOGNIZED;
	// Retrieve each packet type and find the matching handler
	for(idx = 0; ENCAP_DATA_PKT_UNKNOWN != EncapDataFrameCmd[idx].PktType; idx ++)
	{
		if(EncapDataFrameCmd[idx].PktType == encDataType)
		{
			rtStatus = EncapDataFrameCmd[idx].CmdHandler(pAdapter, pRfd, &osMpdu, contentOffset);
			if(RT_STATUS_SUCCESS != rtStatus)
			{
				RT_TRACE_F(COMP_MLME, DBG_WARNING, 
						("[WARNING] Execute %s with status = %d\n",  EncapDataFrameCmd[idx].Name, rtStatus));
			}
			break;
		}
	}
	return rtStatus;
}



VOID
CopyWlanBss(
	IN	PRT_WLAN_BSS	dest,
	IN	PRT_WLAN_BSS	src
	)
{
	PlatformMoveMemory(dest, src, sizeof(RT_WLAN_BSS));
	FillOctetString((dest)->IE, (dest)->IEBuf, (src)->IE.Length);

#if (WPS_SUPPORT == 1)
	FillOctetString((dest)->bdSimpleConfIE, (dest)->bdSimpleConfIEBuf, (src)->bdSimpleConfIE.Length);		
	FillOctetString((dest)->osBeaconWcnIe, (dest)->BeaconWcnIeBuf, (src)->osBeaconWcnIe.Length);	
	FillOctetString((dest)->osProbeRspWcnIe, (dest)->ProbeRspWcnIeBuf, (src)->osProbeRspWcnIe.Length);	
#endif
	FillOctetString((dest)->bdCountryIE, (dest)->bdCountryIEBuf, (src)->bdCountryIE.Length);	
	FillOctetString((dest)->BssQos.bdWMMIE, (dest)->BssQos.bdWMMIEBuf, (src)->BssQos.bdWMMIE.Length);	
	FillOctetString((dest)->WpaIe, (dest)->WpaIeBuf, (src)->WpaIe.Length);	
	FillOctetString((dest)->RsnIe, (dest)->RsnIeBuf, (src)->RsnIe.Length);		
	FillOctetString((dest)->osWmmAcParaIE, (dest)->WmmAcParaBuf, (src)->osWmmAcParaIE.Length);	
	FillOctetString((dest)->osPowerConstraintIe, (dest)->PowerConstraintBuf, (src)->osPowerConstraintIe.Length);
}


