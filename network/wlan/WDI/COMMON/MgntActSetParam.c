#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "MgntActSetParam.tmh"
#endif

//
// Set turbo mode type switches; it's implemented for AutoTurbo by 8186. (asked by SD4)
// Value: (given by Mingyi, 2005-12-22, 18:04.)
//		00 - disable turbo
//		01 - turbo on
//		02 - Auto Turbo
//
// So I regard bit0 as bSupportTurboMode switch,
// and regard bit1  as bAutoTurboBy8186 switch.
//
// Added by Roger, 2006.12.07.
//
BOOLEAN
MgntActSet_RT_TurboModeType(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pTurboModeType
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PTURBOMODE_TYPE	pForceType = (PTURBOMODE_TYPE)pTurboModeType;


	//
	// Input value Check. Currently I want to allow 0, 1, and 2 only.
	//
	if( *pTurboModeType >= 3 )
	{
		return FALSE;
	}

	//
	// Set turbo mode switches: (1)bSupportTurboMode (2)bSupportTurboMode.
	// I suppose this two switches are not both TRUE at the same time.
	//
	pMgntInfo->bSupportTurboMode = pForceType->field.SupportTurboMode;
	pMgntInfo->bAutoTurboBy8186  = pForceType->field.AutoTurboBy8186;

	RT_ASSERT( !(pMgntInfo->bSupportTurboMode && pMgntInfo->bAutoTurboBy8186),
				("bSupportTurboMode and bAutoTurboBy8186 both are TRUE!\n") );

	// Asked by PJ and SD4 David: we should reconnect after setting OID_RT_TURBOMODE. Annie, 2005-12-30.
	// We decide to do it in UI, so I don't disconnect it again. Annie, 2006-01-02.
	//MgntActSet_802_11_DISASSOCIATE( Adapter, unspec_reason );

	return TRUE;
}


// For turbo mode swtiching, added by Roger, 2006.12.07.
VOID
MgntActSet_EnterTurboMode(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bEnterTurboMode
	)
{
	
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);	
	PRT_TURBO_CA	pTurboCa = &(pMgntInfo->TurboChannelAccess);	
	BOOLEAN			bEnterTCA;	


	pTurboCa->bEnabled = bEnterTCA = bEnterTurboMode; //Modefied by Roger, 2006.12.15.	

	if( bEnterTurboMode )
	{ // Enter turbo mode
		Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_TURBO_MODE, (UCHAR*)&bEnterTCA );		
	}
	else	
	{ // Resume
		Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_TURBO_MODE, (UCHAR*)&bEnterTCA );			
	}


}

//
// Locked STA MAC Address in AP mode.
//	- LockedListLen is 6*LockedSTACount. (Reference OID_802_3_MULTICAST_LIST)
// 	- Not to lock if LockedListLen is 0.
//
// Added by Annie, 2006-02-15.
//
BOOLEAN
MgntActSet_Locked_STA_Address(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			LockedListBuf,
	IN	u4Byte			LockedListCnt
)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u4Byte			idx;

	RT_TRACE( COMP_AP, DBG_LOUD, ("====> MgntActSet_Locked_STA_Address()\n") );

	//
	// 061122, rcnjko: Prevent malicious buffer overflow attack.
	//
	if(LockedListCnt > MAX_LOCKED_STA_LIST_NUM)
	{
		LockedListCnt = MAX_LOCKED_STA_LIST_NUM;
	}

	//
	// 1. Update LockedSTACount and LockedSTAList.
	//
	pMgntInfo->LockType = MAC_FILTER_LOCK;
	pMgntInfo->bDefaultPermited = FALSE;
	pMgntInfo->LockedSTACount = LockedListCnt;

	if( LockedListCnt != 0 )
	{
		CopyMem( pMgntInfo->LockedSTAList, LockedListBuf, LockedListCnt*ETHERNET_ADDRESS_LENGTH );
	}

	RT_PRINT_ADDRS( COMP_AP, DBG_LOUD, ("MgntActSet_Locked_STA_Address(): Locked STA List"), pMgntInfo->LockedSTAList, pMgntInfo->LockedSTACount );


	//
	// 2. Disassociate unlocked STAs.
	//
	if( LockedListCnt != 0 )
	{
		// Initialize bLocked
		for( idx=0; idx<ASSOCIATE_ENTRY_NUM; idx++ )
		{
			pMgntInfo->AsocEntry[idx].bLocked = FALSE;
		}

		// Update bLocked
		for( idx=0; idx<LockedListCnt; idx++ )
		{
			PRT_WLAN_STA	pEntry;
			pEntry = AsocEntry_GetEntry( pMgntInfo, pMgntInfo->LockedSTAList[idx] );

			if( pEntry != NULL )
			{
				pEntry->bLocked = TRUE;
			}
		}

		// Disassociate unlocked STAs.
		for( idx=0; idx<ASSOCIATE_ENTRY_NUM; idx++ )
		{
			PRT_WLAN_STA	pSta = &(pMgntInfo->AsocEntry[idx]);
			
			if( pSta->bUsed && pSta->bAssociated && !pSta->bLocked )
			{
				AP_DisassociateStation( Adapter, pSta, inactivity );
			}
		}
	}
	
	RT_TRACE( COMP_AP, DBG_LOUD, ("MgntActSet_Locked_STA_Address() <====\n") );
	return TRUE;
}



BOOLEAN
MgntActSet_Accepted_STA_Address(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			LockedListBuf,
	IN	u4Byte			LockedListCnt
)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u4Byte			idx;

	RT_TRACE( COMP_AP, DBG_LOUD, ("====> MgntActSet_Accepted_STA_Address()\n") );

	//
	// 061122, rcnjko: Prevent malicious buffer overflow attack.
	//
	if(LockedListCnt > MAX_LOCKED_STA_LIST_NUM)
	{
		LockedListCnt = MAX_LOCKED_STA_LIST_NUM;
	}

	//
	// 1. Update LockedSTACount and LockedSTAList.
	//
	pMgntInfo->LockType = MAC_FILTER_ACCEPT;
	pMgntInfo->bDefaultPermited = FALSE;
	pMgntInfo->LockedSTACount = LockedListCnt;

	if( LockedListCnt != 0 )
	{
		CopyMem( pMgntInfo->LockedSTAList, LockedListBuf, LockedListCnt*ETHERNET_ADDRESS_LENGTH );
	}

	RT_PRINT_ADDRS( COMP_AP, DBG_LOUD, ("MgntActSet_Accepted_STA_Address(): Locked STA List"), pMgntInfo->LockedSTAList, pMgntInfo->LockedSTACount );


	//
	// 2. Disassociate unaccepted STAs.
	//
	if( LockedListCnt != 0 )
	{
		// Initialize bLocked
		for( idx=0; idx<ASSOCIATE_ENTRY_NUM; idx++ )
		{
			pMgntInfo->AsocEntry[idx].bLocked = FALSE;
		}

		// Update bLocked
		for( idx=0; idx<LockedListCnt; idx++ )
		{
			PRT_WLAN_STA	pEntry;
			pEntry = AsocEntry_GetEntry( pMgntInfo, pMgntInfo->LockedSTAList[idx] );

			if( pEntry != NULL )
			{
				pEntry->bLocked = TRUE;
			}
		}

		// Disassociate unaccepted STAs.
		for( idx=0; idx<ASSOCIATE_ENTRY_NUM; idx++ )
		{
			PRT_WLAN_STA	pSta = &(pMgntInfo->AsocEntry[idx]);
			
			if( pSta->bUsed && pSta->bAssociated && !pSta->bLocked )
			{
				AP_DisassociateStation( Adapter, pSta, inactivity );
			}
		}
	}
	else	// Reject all
	{
		for( idx=0; idx<ASSOCIATE_ENTRY_NUM; idx++ )
		{
			PRT_WLAN_STA	pSta = &(pMgntInfo->AsocEntry[idx]);
			
			if( pSta->bUsed && pSta->bAssociated)
			{
				AP_DisassociateStation( Adapter, pSta, inactivity );
			}
		}
	}
	
	RT_TRACE( COMP_AP, DBG_LOUD, ("MgntActSet_Accepted_STA_Address() <====\n") );
	return TRUE;
}

BOOLEAN
MgntActSet_Rejected_STA_Address(
	IN	PADAPTER		Adapter,
	IN	pu1Byte		LockedListBuf,
	IN	u4Byte		LockedListCnt
)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u4Byte			idx;

	RT_TRACE( COMP_AP, DBG_LOUD, ("====> MgntActSet_Rejected_STA_Address()\n") );

	//
	// 061122, rcnjko: Prevent malicious buffer overflow attack.
	//
	if(LockedListCnt > MAX_LOCKED_STA_LIST_NUM)
	{
		LockedListCnt = MAX_LOCKED_STA_LIST_NUM;
	}

	//
	// 1. Update LockedSTACount and LockedSTAList.
	//
	pMgntInfo->LockType = MAC_FILTER_REJECT;
	pMgntInfo->bDefaultPermited = TRUE;
	pMgntInfo->LockedSTACount_Reject= LockedListCnt;

	if( LockedListCnt != 0 )
	{
		CopyMem( pMgntInfo->LockedSTAList_Reject, LockedListBuf, LockedListCnt*ETHERNET_ADDRESS_LENGTH );
	}

	RT_PRINT_ADDRS( COMP_AP, DBG_LOUD, ("MgntActSet_Rejected_STA_Address(): Locked STA List"), pMgntInfo->LockedSTAList_Reject, pMgntInfo->LockedSTACount_Reject);


	//
	// 2. Disassociate rejected STAs.
	//
	if( LockedListCnt != 0 )
	{
		// Initialize bLocked
		for( idx=0; idx<ASSOCIATE_ENTRY_NUM; idx++ )
		{
			pMgntInfo->AsocEntry[idx].bLocked = FALSE;
		}

		// Update bLocked
		for( idx=0; idx<LockedListCnt; idx++ )
		{
			PRT_WLAN_STA	pEntry;
			pEntry = AsocEntry_GetEntry( pMgntInfo, pMgntInfo->LockedSTAList_Reject[idx] );

			if( pEntry != NULL )
			{
				pEntry->bLocked = TRUE;
			}
		}

		// Disassociate rejected STAs.
		for( idx=0; idx<ASSOCIATE_ENTRY_NUM; idx++ )
		{
			PRT_WLAN_STA	pSta = &(pMgntInfo->AsocEntry[idx]);
			
			if( pSta->bUsed && pSta->bAssociated && pSta->bLocked )
			{
				AP_DisassociateStation( Adapter, pSta, inactivity );
			}
		}
	}
	
	RT_TRACE( COMP_AP, DBG_LOUD, ("MgntActSet_Rejected_STA_Address() <====\n") );
	return TRUE;
}




//Sets the MAC address of the desired AP.
BOOLEAN
MgntActSet_802_3_MAC_ADDRESS(
	PADAPTER		Adapter,
	pu1Byte          	addrbuf
)
{
	RT_PRINT_ADDR(COMP_INIT,  DBG_LOUD, ("MgntActSet_802_3_MAC_ADDRESS \n"), addrbuf);

	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_ETHER_ADDR, addrbuf);
	PlatformMoveMemory(Adapter->CurrentAddress, addrbuf, 6);

	return TRUE;
}




//Sets the MAC address of the desired AP.
BOOLEAN
MgntActSet_802_11_BSSID(
	PADAPTER		Adapter,
	pu1Byte          	bssidbuf
)
{
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;

	// Set management bssid
	if(ACTING_AS_AP(Adapter)== FALSE) // Follow 8180 AP mode. 2005.05.30, by rcnjko.
	{
		CopyMem( pMgntInfo->Bssid, bssidbuf, 6 );
	}
	
	// Prefast warning C6011: Dereferencing NULL pointer 'bssidbuf'.
	if (bssidbuf != NULL)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("MgntActSet_802_11_BSSID: %02X%02X%02X%02X%02X%02X\n", bssidbuf[0], bssidbuf[1], bssidbuf[2], bssidbuf[3], bssidbuf[4], bssidbuf[5]));
	}

	return TRUE;
}




// Sets the SSID to a specified value
// Select a Basic Service Set to join?
BOOLEAN
MgntActSet_802_11_SSID(
	PADAPTER		Adapter,
	pu1Byte			ssidbuf,
	u2Byte			ssidlen,
	BOOLEAN			JoinAfterSetSSID
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;	
	PRT_WLAN_BSS		pRtBss;
	BOOLEAN 			bTheSameSSID = FALSE;
	RT_JOIN_ACTION		join_action;
	u8Byte 				CurrTime ,DiffTime;
	BOOLEAN				bNeedChkJoinAction;

	RT_PRINT_STR(COMP_MLME, DBG_LOUD, "MgntActSet_802_11_SSID()====> SSID ", ssidbuf, ssidlen);
	//
	// For XP DTM 1.0C association_cmn T1017, we have to clear reject list when 
	// upper layer try to link with new profile. 070912, by rcnjko.
	//
	MgntClearRejectedAsocAP(Adapter);

	//
	//  Reset mDeauthCount
	//
	pMgntInfo->mDeauthCount = 0;

	if(ACTING_AS_AP(Adapter))
	{
		OCTET_STRING	SsidToCheck;
		
		FillOctetString( SsidToCheck, ssidbuf, ssidlen );

		//
		// 060227, Annie:
		// For MeetingHouse and Zero Config coexistence issue: not to set dummy SSID in AP mode.
		//
		if( IsSSIDDummy(SsidToCheck) )
		{
			RT_TRACE( COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_SSID(): mActingAsAp, Dummy SSID! Not to set it!\n") );
			return FALSE;
		}
	
		CopySsid(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, ssidbuf, ssidlen);
		
		if(pMgntInfo->bAutoSelChnl)
		{
			{
				if(	!MgntScanInProgress(pMgntInfo)	&&
					!MgntIsLinkInProgress(pMgntInfo)	)
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_SSID(): mActingAsAp &&  bAutoSelChnl ==> MgntLinkRequest\n"));
					MgntLinkRequest(
							Adapter,
							TRUE,		//bScanOnly
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
			}
		}
		else
		{
			AP_Reset(Adapter);	
		}
		return TRUE;
	}

//	pMgntInfo->bIbssStarter=FALSE;

	CCX_SetAssocReason(Adapter, CCX_AR_FIRST_ASSOCIATION);

	if(MgntIsLinkInProgress(pMgntInfo))
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("Ssid during association !!\n"));
	}

	if(	ssidlen==pMgntInfo->Ssid.Length	&&
		!PlatformCompareMemory( ssidbuf, pMgntInfo->Ssid.Octet, ssidlen ) )
	{
		bTheSameSSID = TRUE;
	}

	// In adhoc mode, update beacon frame.
	if( pMgntInfo->bMediaConnect || pMgntInfo->bIbssStarter)
	{
		if( pMgntInfo->mIbss )
		{
			if( bTheSameSSID )
			{
				if(pMgntInfo->bIbssStarter && !pMgntInfo->bMediaConnect)
				{
					RT_TRACE_F(COMP_MLME, DBG_LOUD, ("theSameSSID bIbssStarter TRUE bMediaConnect FALSE!\n"));
					return TRUE;
				}
				
				// If SSID is the same, update security status.
				if( pMgntInfo->bMlmeStartReqRsn == MLMESTARTREQ_NONE )
				{
					// do nothing if SSID is the same.
					// 2004/03/11 For NDTest -> Indicating connect while connect to the same IBSS network.
					MgntIndicateMediaStatus( Adapter, RT_MEDIA_CONNECT, FORCE_INDICATE );
					return TRUE;
				}
				else
				{
					MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE);
				}
				//TODO: update beacon frame if necessary.
			}
			else
			{
				MgntDisconnectIBSS( Adapter );
			}
		}

		if( pMgntInfo->mAssoc)
		{
			if( bTheSameSSID )
			{
				// 2004/03/09 For NDTest -> Indicating connect while connect to the same AP.
				MgntIndicateMediaStatus( Adapter, RT_MEDIA_CONNECT, FORCE_INDICATE );
		
				// If SSID is the same, update security status.
				if( pMgntInfo->bMlmeStartReqRsn == MLMESTARTREQ_NONE )
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_SSID(): Return because MLMESTARTREQ_NONE!\n"));
					return TRUE;
				}
			}
			else
			{
				// If SSID is different, disassociate AP.
				MgntDisconnectAP(Adapter, disas_lv_ss);
			}
		}
	}

	// Set RegSSID, ssid2match.
	CopySsid(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, ssidbuf, ssidlen);
	
	//
	// Do not scan but reset all mgnt variables when the OS wants to reset. By Bruce, 2009-04-10.
	//
	if(IsSSIDDummy(pMgntInfo->Ssid))
	{
		ResetMgntVariables(Adapter);
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_SSID(): Dummy SSID! Return!!!!\n"));
		return FALSE;
	}
	
	if( pMgntInfo->bScanInProgress || (pMgntInfo->bDualModeScanStep!=0) )
	{	
		pMgntInfo->bScanOnly = FALSE;

		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_SSID():pMgntInfo->bScanInProgress=true Return!!!!\n"));
		return FALSE;
	}

	// If in WPS process, do not connect to the same AP twice
	// Add by hpfan 2008.02.06
	if (WPS_MgntActSet_802_11_SSID(Adapter) == TRUE)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_SSID(): do not connect to the same AP twice  Return!!!!\n"));
		return	TRUE;
	}

	MgntResetJoinCounter(pMgntInfo);
	
	//2004/08/23, kcwu, set the Privacy bit on in Capability field
	if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm != RT_ENC_ALG_NO_CIPHER)
		pMgntInfo->mCap |= cPrivacy;
	else
		pMgntInfo->mCap &= ~cPrivacy;

	if(PlatformAllocateMemory(Adapter, (PVOID*)&pRtBss, sizeof(RT_WLAN_BSS)) != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_SSID(): Allocate memory for pRtBss failed!Return!!!\n"));
		return FALSE;
	}

	CurrTime = PlatformGetCurrentTime(); // In micro-second.
	DiffTime = CurrTime - Adapter->LastScanCompleteTime; // In micro-second.
	
	if (DiffTime >= 10000000) //it's over 10 sec after last scan complete 
	{
		if((Adapter->bInHctTest || pMgntInfo->IsAMDIOIC) && pMgntInfo->RoamingType == RT_ROAMING_BY_SLEEP)
		{
			//
			//
			// This is a very dirty work around for Win7 NDISTest performance_ext.
			// It requires that the period from system transit to D0 to the time we
			// associate with the AP to be less than 1.1 sec. Since LastScanCompleteTime 
			// is greater than 10 sec., we will have to do a scan before associate with the AP
			// in the original design. The scan takes too much time (around 1.7 sec)
			// to associate with the AP (threshold = 1.1 sec). We ignore checking of 
			// LastScanCompleteTime to avoid scan and time is saved.
			// If the original AP disappears then join timeout occurs.
			// 2008.08.28, haich.
			//
			//PlatformStallExecution(50);
			join_action = SelectNetworkBySSID( Adapter, &pMgntInfo->Ssid, FALSE, pRtBss );
		}
		else
		{		
			bNeedChkJoinAction = FALSE;
			Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_CHK_JOINACTION, (pu1Byte)(&bNeedChkJoinAction));
			if(bNeedChkJoinAction)
			{
				join_action = SelectNetworkBySSID( Adapter, &pMgntInfo->Ssid, FALSE, pRtBss );
			}
			else
			{
				join_action = RT_NO_ACTION;
			}
		}
	}
	else
	{
		join_action = SelectNetworkBySSID( Adapter, &pMgntInfo->Ssid, FALSE, pRtBss);
	}

	if(join_action==RT_JOIN_INFRA || join_action==RT_JOIN_IBSS)
	{
		pMgntInfo->JoinAction = join_action;
		JoinRequest( Adapter, join_action, pRtBss );
	}
	else
	{	

		u1Byte			ssid2scanbuf[33];
		OCTET_STRING	ssid2scan;
		u1Byte			SuppRatebuf[MAX_NUM_RATES];
		OCTET_STRING	SuppRate;
		IbssParms		yIbssP;

		FillOctetString(ssid2scan, ssid2scanbuf, 0);
		CopySsid(ssid2scan.Octet, ssid2scan.Length, ssidbuf, ssidlen);
		
		FillOctetString(SuppRate, SuppRatebuf, 0);
		CopyMemOS( &SuppRate, pMgntInfo->Regdot11OperationalRateSet, pMgntInfo->Regdot11OperationalRateSet.Length);

		//yIbssP.atimWin = 80;
		yIbssP.atimWin = 0; // Asked by owen: since we have not yet implement power save mode on IBSS now. 2005.03.04, by rcnjko.
						// O.w., it will degrade the throughput when Netgear WG511T join the IBSS created by us.

		if(	!MgntScanInProgress(pMgntInfo)	&&
			!MgntIsLinkInProgress(pMgntInfo)	)
		{
			MgntLinkRequest(
				Adapter,
				FALSE,			 //bScanOnly
				TRUE,			 //bActiveScan,
				FALSE,			  //FilterHiddenAP
				TRUE,		// Update parameters
				&ssid2scan, 		//ssid2scan
				pMgntInfo->Regdot11networktype,		//NetworkType,
				pMgntInfo->Regdot11ChannelNumber,		//ChannelNumber,
				pMgntInfo->Regdot11BeaconPeriod,		//BcnPeriod,
				pMgntInfo->Regdot11DtimPeriod,			//DtimPeriod,
				pMgntInfo->RegmCap, 					//mCap,
				&SuppRate,							//SuppRateSet,
				&yIbssP 								//yIbpm,
			);
		}
	}
	PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));

	pMgntInfo->bMlmeStartReqRsn = MLMESTARTREQ_NONE;	
	return TRUE;
}






// Sets the current NDIS_802_11_TX_POWER_LEVEL value in mW.
BOOLEAN
MgntActSet_802_11_TX_POWER_LEVEL(
	PADAPTER		Adapter,
	u4Byte	          	powerlevel
)
{
	// TODO:
	return TRUE;
}





// Sets the RSSI trigger value for an event.
BOOLEAN
MgntActSet_802_11_RSSI_TRIGGER(
	PADAPTER		Adapter,
	u4Byte	          	param1
)
{
	return TRUE;
}





// Sets mode to Infrastructure or IBSS, or automatic switch between the two.
// This also resets the network association algorithm.
// When this OID is called to set the mode, 
//     all keys set through OID_802_11_ADD_WEP and OID_802_11_ADD_KEY should be deleted.
// <Bruce_Note>
//	Refering to the DDK 3790, " If the device is associated and a set of this OID changes the driver's network mode, 
//	the driver must disassociate and make a media disconnect indication."
//	However, whenever the WZC changes the security setting of the same profile and set infrastructure mode as the previous, 
//	the NIC never becomes disconnected. Actually we just reassociate and wait for the security handshake from
//	WZC supplicant, but the supplicant cannot determine which the settings between the profile and driver shall be used,
//	because we never indicate disconnected status.
//	We always disconnect from the current BSS, and indicate the status to OS no matter the infrastructure mode is changged.
//	Pass WLK 1.4 XP DTM.
//	By Bruce, 2009-05-27.
//
BOOLEAN
MgntActSet_802_11_INFRASTRUCTURE_MODE(
	PADAPTER			Adapter,
	RT_JOIN_NETWORKTYPE networktype
)
{
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;	
	

	if( Adapter->bHWInitReady == FALSE )
		return FALSE;

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 4\n"));
		return FALSE;
	}	

	switch( networktype )
	{
		case RT_JOIN_NETWORKTYPE_ADHOC:
		case RT_JOIN_NETWORKTYPE_INFRA:
		case RT_JOIN_NETWORKTYPE_AUTO:
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_INFRASTRUCTURE_MODE(): %d\n", networktype));

			MgntDisconnect(Adapter, disas_lv_ss);

			if(MgntRoamingInProgress(pMgntInfo))
				MgntRoamComplete(Adapter, MlmeStatus_invalid);
			
			if (!IS_DUAL_BAND_SUPPORT(Adapter))
				MgntResetLinkProcess(Adapter);
			else
			{	
				//When we do scan for connect on dual band. The scan time is  so long(>2s) that it can't be completed before connect.
				//We do connect in ScanComplete(). by Lawrence.
				// 2013/08/21 MH Revise for XP, when reset link process is blocked, the 4 query scan list will not exist
				// previous linked AP and the scan process will be delay for a while and this will cause XP 2sec reset event.
				// The case: Link with one AP, then AP disappear. Then the scan and connect process will be delayed 
				// due to the the reset process is not executed. Need to find why 
	
				if(pMgntInfo->bMediaConnect || MgntIsLinkInProgress(pMgntInfo))
					MgntResetLinkProcess(Adapter);
				else
				{
					pMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
					pMgntInfo->AuthStatus = AUTH_STATUS_IDLE;
					pMgntInfo->mDisable = TRUE;
					pMgntInfo->mAssoc = FALSE;
					pMgntInfo->mIbss = FALSE;
					pMgntInfo->bMlmeStartReqRsn = MLMESTARTREQ_NONE;
					pMgntInfo->bJoinInProgress = FALSE;
				}
			}
			
			if(IS_HARDWARE_TYPE_8821U(Adapter))
			{
				pMgntInfo->LEDAssocState = LED_ASSOC_SECURITY_NONE;
			}
			
			break;
			
		default:
			break;// auto mode
	}

	pMgntInfo->Regdot11networktype = networktype;

	//
	// <Roger_Notes> The follwing decision will update current PSP control settings dynamically,
	// which according as network type, i.e., infracture mode will disable PSP control as default.
	// 2009.12.03.
	//
	if( pMgntInfo->bDefaultPSPXlinkMode &&  
		networktype == RT_JOIN_NETWORKTYPE_ADHOC )
	{
		// Update ReceiveConfig variable. 
		Adapter->HalFunc.AllowAllDestAddrHandler(Adapter, TRUE, TRUE);
		pMgntInfo->bPSPXlinkMode = TRUE;
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_INFRASTRUCTURE_MODE(): Enable PSP Xlink!!\n"));
	}
	else
	{// Disable PSP Xlink control even bDefaultPSPXlinkMode is TRUE.
	
		// Update ReceiveConfig variable. 
		if(!pMgntInfo->bNetMonitorMode)
			Adapter->HalFunc.AllowAllDestAddrHandler(Adapter, FALSE, TRUE);
		pMgntInfo->bPSPXlinkMode = FALSE;
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_INFRASTRUCTURE_MODE(): Disable PSP Xlink!!\n"));
	}

	//2004/07/27, kcwu,
	/*
		When this OID is called to set the mode, all keys set through OID_802_11_ADD_WEP
		and OID_802_11_ADD_KEY should be deleted
	*/
	SecClearAllKeys(Adapter);
	return TRUE;
}





// Sets the fragmentation threshold in bytes.
BOOLEAN
MgntActSet_802_11_FRAGMENTATION_THRESHOLD(
	PADAPTER		Adapter,
	u2Byte	          	fragthres
)
{
	Adapter->MgntInfo.FragThreshold = fragthres;
	return TRUE;
}





//Sets the RTS threshold.
BOOLEAN
MgntActSet_802_11_RTS_THRESHOLD(
	PADAPTER		Adapter,
	u2Byte	          	RtsThres
)
{
	RT_TRACE(COMP_INIT, DBG_SERIOUS,("TODO: Set Rts Threshold = %02x .\n", RtsThres) );

	Adapter->MgntInfo.dot11RtsThreshold = RtsThres;
	return TRUE;
}





//Sets the antenna used for receiving.
BOOLEAN
MgntActSet_802_11_RX_ANTENNA_SELECTED(
	PADAPTER		Adapter,
	u4Byte	          	param1
)
{
	return TRUE;
}





//Sets the antenna used for transmitting.
BOOLEAN
MgntActSet_802_11_TX_ANTENNA_SELECTED(
	PADAPTER		Adapter,
	u4Byte	          	param1
)
{
	return TRUE;
}





//Sets the set of data rates.
BOOLEAN
MgntActSet_802_11_DESIRED_RATES(
	PADAPTER		Adapter,
	pu1Byte	        RateSetbuf,
	u2Byte			RateSetlen
)
{
	return TRUE;
}




BOOLEAN
MgntActSet_802_11_RATE(
	PADAPTER		Adapter,
	u1Byte	          	rate			// 0x02 := 1Mbps, ....
	)
{
	//TODO
	// <1>Set operational rate or basic rate or both???
	// <2>Set fixed rate or only enable the rate set??
	RT_TRACE(COMP_OID_SET, DBG_LOUD,("TODO: Set rate =%02x.\n",rate) );

	return TRUE;
}





//Sets the radio configuration.
// 2004.06.14, by rcnjko.
BOOLEAN
MgntActSet_802_11_CONFIGURATION(
	PADAPTER		Adapter,
	u2Byte			BeaconPeriod,
	u1Byte			ChannelNumber
)
{
	PMGNT_INFO	pMgntInfo= &Adapter->MgntInfo;

	// Change default value of beacon period. 
	pMgntInfo->Regdot11BeaconPeriod = BeaconPeriod;

	// Switch to channel specified.
	MgntActSet_802_11_REG_20MHZ_CHANNEL_AND_SWITCH(Adapter, ChannelNumber);

	return TRUE;
}





//Sets the desired WEP key.
// <1>Set 4 Default key refered to pMgntInfo->SecurityInfo.DefaultKeyBuf.
// <2>Set SCR refered to pMgntInfo->SecurityInfo.EncAlgorithm.
BOOLEAN
MgntActSet_802_11_ADD_WEP(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte			KeyLength,
	pu1Byte			KeyMaterial,
	BOOLEAN			IsDefaultKeyId
)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	AESCCMP_BLOCK		blockKey;

	RT_TRACE(COMP_SEC, DBG_LOUD, ("====> MgntActSet_802_11_ADD_WEP(): EncAlgorithm=%d, KeyIndex=0x%X, KeyLength=%d, IsDefaultKeyId=%d\n",
																		EncAlgorithm, KeyIndex, KeyLength, IsDefaultKeyId ));
	pSecInfo->PairwiseEncAlgorithm = EncAlgorithm;

	KeyIndex = (KeyIndex < 4) ? KeyIndex : 0;
	KeyLength = ( KeyLength <= 13  ) ? KeyLength : 13;
	CopyMem( pSecInfo->KeyBuf[KeyIndex], KeyMaterial, KeyLength );
	pSecInfo->KeyLen[KeyIndex] = (u1Byte)KeyLength;
	if( IsDefaultKeyId ){
 		pSecInfo->DefaultTransmitKeyIdx = (u1Byte)KeyIndex;
	}

	pMgntInfo->bMlmeStartReqRsn = MLMESTARTREQ_KEY_CHANGE;

	Adapter->HalFunc.SetKeyHandler(Adapter, KeyIndex, 0, FALSE, EncAlgorithm, TRUE, FALSE);


	// add for set CKIP key exp. and AES_set Key( for MIC )  , by CCW
	if( KeyLength==5 && KeyIndex<4 )
	{				
		CopyMem( pSecInfo->pCkipPara->CKIPKeyBuf[KeyIndex]+0*KeyLength , KeyMaterial, KeyLength );
		CopyMem( pSecInfo->pCkipPara->CKIPKeyBuf[KeyIndex]+1*KeyLength , KeyMaterial, KeyLength );
		CopyMem( pSecInfo->pCkipPara->CKIPKeyBuf[KeyIndex]+2*KeyLength , KeyMaterial, KeyLength );
		CopyMem( pSecInfo->pCkipPara->CKIPKeyBuf[KeyIndex]+3*KeyLength , KeyMaterial, 1 );
	}
	else if( KeyLength==13 && KeyIndex<4 )
	{	
		CopyMem( pSecInfo->pCkipPara->CKIPKeyBuf[KeyIndex]+0*KeyLength , KeyMaterial, KeyLength );
		CopyMem( pSecInfo->pCkipPara->CKIPKeyBuf[KeyIndex]+1*KeyLength , KeyMaterial, CKIP_KEY_LEN-KeyLength );
	}
	RT_PRINT_DATA( COMP_CKIP, DBG_LOUD, "CKIPKeyBuf", pSecInfo->pCkipPara->CKIPKeyBuf[KeyIndex], CKIP_KEY_LEN );

	PlatformMoveMemory( blockKey.x, pSecInfo->pCkipPara->CKIPKeyBuf[KeyIndex], CKIP_KEY_LEN );

	// When reset the WEP key, just reset the IV.
	pSecInfo->TxIV = DEFAULT_INIT_TX_IV;

	AES_SetKey( blockKey.x, AESCCMP_BLK_SIZE*8, (pu4Byte)pSecInfo->AESKeyBuf[KeyIndex] );
	
	RT_TRACE(COMP_SEC, DBG_LOUD, ("MgntActSet_802_11_ADD_WEP() <====\n"));

	return TRUE;
}





//Removes the desired WEP key.
BOOLEAN
MgntActSet_802_11_REMOVE_WEP(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte			KeyLength
)
{

	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	KeyLength = ( KeyLength <= 13  ) ? KeyLength : 13;
	if( KeyIndex < 4 ){
		PlatformZeroMemory(  pMgntInfo->SecurityInfo.KeyBuf[KeyIndex], KeyLength);
		pMgntInfo->SecurityInfo.KeyLen[KeyIndex] = 0;
	}else{
		return FALSE;
	}

	Adapter->HalFunc.SetKeyHandler(Adapter, KeyIndex, 0, FALSE, EncAlgorithm, TRUE, FALSE);


	return TRUE;
}





//Disassociates with the current SSID and turns off the radio.
NDIS_STATUS
MgntActSet_802_11_DISASSOCIATE(
	PADAPTER		Adapter,
	u1Byte			asRsn
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	NDIS_STATUS	ndisStatus = NDIS_STATUS_SUCCESS;
	RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_DISASSOCIATE()====>\n"));
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
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_DISASSOCIATE() ===> AP_DisassociateAllStation\n"));
		AP_DisassociateAllStation(Adapter, unspec_reason);
		return ndisStatus;
	}	

	pMgntInfo->RequestFromUplayer = FALSE;

	// In adhoc mode, update beacon frame.
	if( pMgntInfo->bMediaConnect || pMgntInfo->bIbssStarter)
	{
		if( pMgntInfo->mIbss)
		{
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_DISASSOCIATE() ===> MgntDisconnectIBSS\n"));

			//should clear key here,if not , AES/TKIP Can not ping after use WAPI zhiyuan 2009/12/25	
			RT_TRACE(COMP_SEC,DBG_LOUD,("MgntActSet_802_11_Disassociate  sec mode = 0x%08X\n",pMgntInfo->SecurityInfo.PairwiseEncAlgorithm));
			if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm==RT_ENC_ALG_SMS4 && WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0))
				SecClearAllKeys(Adapter); 

			MgntDisconnectIBSS( Adapter );
		}
		
		if( pMgntInfo->mAssoc )
		{
			// We should leave LPS mode first. 2011.03.23. by tynli.
			if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS)
			{
				LeisurePSLeave(Adapter, LPS_DISABLE_SET_DISASSOCIATE);
			}
			
			RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_DISASSOCIATE() ===> MgntDisconnectAP\n"));
			MgntDisconnectAP(Adapter, asRsn);
			
			// We clear key here instead of MgntDisconnectAP() because that  
			// MgntActSet_802_11_DISASSOCIATE() is an interface called by OS, 
			// e.g. OID_802_11_DISASSOCIATE in Windows while as MgntDisconnectAP() is 
			// used to handle disassociation related things to AP, e.g. send Disassoc
			// frame to AP.  2005.01.27, by rcnjko.
			// The key shall be cleared after sending disassociate/deauth to AP because these disconnecting frames
			// may be protected by the keys, we shall keep the key before sending the mgnt frames.
			SecClearAllKeys(Adapter); 
		}

		// Inidicate Disconnect, 2005.02.23, by rcnjko.
		MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE);	
	}
	else 
    {   
    	if(pMgntInfo->bScanInProgress == TRUE || pMgntInfo->bDualModeScanStep != 0)
      	{   //2013.05.14, Revised by Ping-Yan, to avoid skip scans when Port1 exists for single band chip. 
      		MgntResetScanProcess(Adapter);
      	}

		ndisStatus = NDIS_STATUS_INVALID_STATE;

	}
	
	//Set SSID as dummy to stop the current join process.
	SET_SSID_DUMMY(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);

	return ndisStatus;
}



//
// Deauthentication with the current SSID.
// Reference MgntActSet_802_11_DISASSOCIATE().
// Added by Annie, 2006-05-04.
//
BOOLEAN
MgntActSet_802_11_DEAUTHENTICATION(
	PADAPTER		Adapter,
	u1Byte			asRsn
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	if(ACTING_AS_AP(Adapter))
	{ // AP Mode.
		// TODO: AP MODE.
		return TRUE;
	}	

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return TRUE;		
	}
	
	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress \n"));
		return TRUE;
	}
	
	// In adhoc mode, update beacon frame.
	if( pMgntInfo->bMediaConnect || pMgntInfo->bIbssStarter)
	{
		if( pMgntInfo->mIbss )
		{ // Ad-Hoc Mode: Following MgntActSet_802_11_DISASSOCIATE().
			MgntDisconnectIBSS( Adapter );
		}
		if( pMgntInfo->mAssoc )
		{
			// We should leave LPS mode first. 2011.03.23. by tynli.
			if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS)
			{
				LeisurePSLeave(Adapter, LPS_DISABLE_SET_DEAUTH);
			}
			
			// We clear key here instead of MgntDisconnectAP() because that  
			// MgntActSet_802_11_DISASSOCIATE() is an interface called by OS, 
			// e.g. OID_802_11_DISASSOCIATE in Windows while as MgntDisconnectAP() is 
			// used to handle disassociation related things to AP, e.g. send Disassoc
			// frame to AP.  2005.01.27, by rcnjko.
			SecClearAllKeys(Adapter); 

			MlmeDeauthenticateRequest( Adapter, pMgntInfo->Bssid, asRsn );
		}

		// Inidicate Disconnect, 2005.02.23, by rcnjko.
		MgntIndicateMediaStatus( Adapter, RT_MEDIA_DISCONNECT, GENERAL_INDICATE);	
	}

	//Set SSID as dummy to stop the current join process.
	SET_SSID_DUMMY(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);

	return TRUE;
}




//Sets the authentication mode to open, shared, auto-switch, WPA, WPA-PSK or WPA-None.
BOOLEAN
MgntActSet_802_11_AUTHENTICATION_MODE(
	PADAPTER		Adapter,
	RT_AUTH_MODE	authmode
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	
	if( pMgntInfo->SecurityInfo.AuthMode != authmode )
	{
		pMgntInfo->SecurityInfo.AuthMode = authmode;
		pMgntInfo->bMlmeStartReqRsn = MLMESTARTREQ_AUTH_CHANGE;
	}

	switch( authmode ){
	case RT_802_11AuthModeOpen:
			pMgntInfo->AuthReq_auAlg = OPEN_SYSTEM;
			break;
	case RT_802_11AuthModeShared:
			pMgntInfo->AuthReq_auAlg = SHARED_KEY;
			break;
	case RT_802_11AuthModeAutoSwitch:
			pMgntInfo->AuthReq_auAlg = SHARED_KEY; // 2005.03.08, by rcnjko.
			break;
	case RT_802_11AuthModeWPA:
	case RT_802_11AuthModeWPAPSK:
	case RT_802_11AuthModeWPANone:
	case RT_802_11AuthModeWPA2:			// Added by Annie, 2005-09-14.
	case RT_802_11AuthModeWPA2PSK:		// Added by Annie, 2005-09-14.
	case RT_802_11AuthModeMax:
	case RT_802_11AuthModeCCKM:
	case RT_802_11AuthModeWAPI:
	case RT_802_11AuthModeCertificateWAPI:
			//2004/07/24, kcwu, it should be "OPEN_SYSTEM"
			//pMgntInfo->AuthReq_auAlg = SHARED_KEY;
			pMgntInfo->AuthReq_auAlg = OPEN_SYSTEM;
	default:
			break;
	}

	//add for RSNA IBSS ,by CCW
	if( pMgntInfo->bRSNAPSKMode )
	{
		if( !pMgntInfo->SecurityInfo.SWTxEncryptFlag || !pMgntInfo->SecurityInfo.SWRxDecryptFlag )
		{
				// RSNA IBSS : use software encryption/decryption.
				SecSetSwEncryptionDecryption(Adapter, TRUE, TRUE);
				if(pMgntInfo->NdisVersion < RT_NDIS_VERSION_6_20)
				Adapter->HalFunc.DisableHWSecCfgHandler(Adapter);
		}
	}

	return TRUE;
}







//Sets to open or 802.1X filtering, zero is open, 1 is 802.1X filtering.
BOOLEAN
MgntActSet_802_11_PRIVACY_FILTER(
	PADAPTER		Adapter,
	u4Byte	          	param1
)
{
	return TRUE;
}


//Performs a survey of potential BSSs.
BOOLEAN
MgntActSet_802_11_BSSID_LIST_SCAN(
	PADAPTER		Adapter
)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	u8Byte		now_time, Diff_Time;
	
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("====> MgntActSet_802_11_BSSID_LIST_SCAN()\n"));

	
	// Follow 8180 AP mode, 2005.05.30, by rcnjko.
	// <RJ_TODO_AP> The client may still want to scan in AP mode.
	if(ACTING_AS_AP(Adapter))
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("MgntActSet_802_11_BSSID_LIST_SCAN(): in AP mode, return TRUE. <====\n"));
		DrvIFIndicateScanComplete(Adapter, RT_STATUS_FAILURE);
		return TRUE;
	}
	
	now_time = PlatformGetCurrentTime();
	Diff_Time = now_time - pMgntInfo->uConnectedTime;
	if(Diff_Time/100000  <=100)
	{
		return TRUE;
	}
	
	Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_SITE_SURVEY);
	
	if(	!MgntScanInProgress(pMgntInfo)	&&
		!MgntIsLinkInProgress(pMgntInfo) 		)
	{
		{
			MgntLinkRequest(
					Adapter,
					TRUE,		//bScanOnly
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
	}
	else
	{
		// In this case, the scan process is progressing, just return SUCCESS.
		// 2006.11.15, by shien chang.
		if ( MgntIsLinkInProgress(pMgntInfo) )
		{
			DrvIFIndicateScanComplete(Adapter, RT_STATUS_SUCCESS);
		}
	}
	
	RT_TRACE(COMP_SCAN, DBG_TRACE, ("MgntActSet_802_11_BSSID_LIST_SCAN(): return TRUE. <====\n"));
	return TRUE;
}







//Permitted to enable or disable encryption and the encryption modes.
BOOLEAN
MgntActSet_802_11_ENCRYPTION_STATUS(
	PADAPTER		Adapter,
	u4Byte	          	param1
)
{
	return TRUE;
}





// Sets results in the reloading of the available defaults for the specified type field.
BOOLEAN
MgntActSet_802_11_RELOAD_DEFAULTS(
	PADAPTER		Adapter,
	u4Byte	          	param1
)
{
	return TRUE;
}







//-------------------------------------------------
// WPA
//-------------------------------------------------
//Sets the desired key.
BOOLEAN
MgntActSet_802_11_ADD_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte			KeyLength,
	pu1Byte			KeyMaterial,
	pu1Byte			MacAddress,
	BOOLEAN			IsGroupTransmitKey,
	BOOLEAN			IsGroup,
	u8Byte			KeyRSC 				// Instialized value used for received multicast packet. Only used when the input parameter "IsGroup" is set TRUE and "IsGroupTransmitKey" set FALSE and KeyIndex | ADD_KEY_IDX_RSC.
)
{
	
	PRT_SECURITY_T	pSecInfo = &Adapter->MgntInfo.SecurityInfo;
	pu1Byte			pKeyRSC = (pu1Byte)&KeyRSC;

	RT_TRACE( COMP_SEC, DBG_LOUD, ("====> MgntActSet_802_11_ADD_KEY()\n") );

//	RT_TRACE( COMP_SEC, DBG_LOUD, ("EncAlgorithm=%d, KeyIndex=0x%X, IsGroupTransmitKey=%d, IsGroup=%d, KeyRSC=0x%"i64fmt"X\n",
	//									EncAlgorithm, KeyIndex, IsGroupTransmitKey, IsGroup, KeyRSC ) );

	RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "KeyMaterial:\n", KeyMaterial, KeyLength );
	RT_PRINT_ADDR( COMP_SEC, DBG_LOUD, "MacAddress: ", MacAddress );

	//
	// 061122, rcnjko: Prevent malicious buffer overflow attack.
	//
	if(KeyLength > MAX_KEY_LEN)
	{
		RT_ASSERT(FALSE, ("MgntActSet_802_11_ADD_KEY(): KeyLength(%d) > MAX_KEY_LEN(%d) !!!\n", KeyLength, MAX_KEY_LEN));
		return FALSE;
	}

	//2004/09/01, kcwu
	if((KeyIndex & 0x00000003) > 3)
		return FALSE;

	//
	// When the mode is mix of TKIP and WEP for GroupTransmitKey, it should not enter this case!.
	// Modified for testing over DTM XP 1.0c, by Bruce, 2007-07-26.
	//2004/09/15, kcwu, for AES
	//
	if( !IsGroup && !IsGroupTransmitKey &&((KeyIndex & 0x00000003) == 0) && ((KeyLength != 32 && pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP)|| (KeyLength != 16 && pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP)))
	{
		RT_TRACE( COMP_SEC, DBG_LOUD, ("MgntActSet_802_11_ADD_KEY(): key length not match <====\n") );
		return FALSE;
	}

	//2004/09/01, kcwu
	if( (KeyLength == 5) && ((KeyIndex & 0x00000003) < 4) && (pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_NO_CIPHER)){
		EncAlgorithm = RT_ENC_ALG_WEP40;
	}
	else if( (KeyLength == 13) && ((KeyIndex & 0x00000003) < 4) && (pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_NO_CIPHER)){
		EncAlgorithm = RT_ENC_ALG_WEP104;
	}
	else if( (KeyLength == 16) && ((KeyIndex & 0x00000003) < 4) && (pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_NO_CIPHER)) {
		// 2009.03.30 Reject WEP 152 connection, add  by hpfan
		RT_TRACE(COMP_SEC, DBG_LOUD, ("MgntActSet_802_11_ADD_KEY(): Wep with wrong key length <====\n") );
		return FALSE;
	}

	if(IsGroup)
	{
		// Check the previous WOL event if the GTK triggered waking.
		WolByGtkUpdate(Adapter);
	}
	

	// 
	// No need to check SecLvl in WEP mode, and it only depends on EncAlgorithm.
	// Comment out for testing over DTM XP 1.0c, by Bruce, 2007-07-26.
	//
	if( pSecInfo->SecLvl == RT_SEC_LVL_NONE || Adapter->bInHctTest)
	{
		if(EncAlgorithm == RT_ENC_ALG_WEP40||EncAlgorithm == RT_ENC_ALG_WEP104)
		{
			MgntActSet_802_11_ADD_WEP(
							Adapter,
							EncAlgorithm,
							(KeyIndex & 0x000000ff),
							KeyLength,
							KeyMaterial,
							( KeyIndex & 0x80000000 ) ? TRUE : FALSE
			);

			return TRUE;
		}
	}

	if(!IsGroup) // This key is pairwise key
	{
		// The pairwise key is updated, so are the Tx/Rx IV. By Bruce, 2010-11-04.
		pSecInfo->TxIV = DEFAULT_INIT_TX_IV;
		pSecInfo->RXUntiIV = DEFAULT_INIT_RX_IV;
	}

	if(IsGroup)
	{
		// When the group key is updated, we need to update the rx IV.
		// Note:
		//	Maybe we shall check the flag IsGroupTransmitKey as FALSE for RX group key case.
		// By Bruce, 2010-11-04.
		pSecInfo->RXMutiIV = DEFAULT_INIT_RX_IV; 
	}

	//
	// Else: WPA or WPA2.
	//

	if(KeyIndex & ADD_KEY_IDX_RSC)
	{//kcwu: SetRSC
		if(IsGroup)
		{
			// Record the RSC for multicast IV to avoid IV replay.
			pSecInfo->RXMutiIV = 
				//LowPart
				( (u8Byte)(
				(u4Byte)((u1Byte)*(pKeyRSC+0) <<  8) | (u4Byte)((u1Byte)*(pKeyRSC+1) <<  0)
				| (u4Byte)((u1Byte)*(pKeyRSC+4) << 24) | (u4Byte)((u1Byte)*(pKeyRSC+5) << 16)
				)<<32)
				//HighPart
				|((u4Byte)((u1Byte)*(pKeyRSC+2) <<  8) | (u4Byte)((u1Byte)*(pKeyRSC+3) <<  0));
		}
	}		
	/* Remove It , Group Key Updata should not init Unicase IV
	else
	{
		if(IsGroup)
		{
			pSecInfo->TxIV = 0;
		}
	}
	*/

	KeyIndex &= 0x00000003;
	
	//kcwu: Indicate this key idx is used for TX
	if(IsGroup){//Group transmit key
		if(IsGroupTransmitKey){
			pSecInfo->GroupTransmitKeyIdx= (u1Byte)KeyIndex;
		}
		SecClearGroupKeyByIdx(Adapter, (u1Byte)KeyIndex);
		CopyMem(pSecInfo->KeyBuf[KeyIndex], KeyMaterial, KeyLength );
		pSecInfo->KeyLen[KeyIndex]= (u1Byte)KeyLength;
	}else{
		
		//kcwu: clear keybuffer
		SecClearPairwiseKeyByMacAddr(Adapter, 0);
		CopyMem( pSecInfo->PairwiseKey, KeyMaterial, KeyLength );

		//This is for pairwise key
		pSecInfo->KeyLen[PAIRWISE_KEYIDX]= (u1Byte)KeyLength;

		//
		// We had done 4-way 
		//
		Adapter->MgntInfo.mDeauthCount = 0;
		RT_TRACE( COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_ADD_KEY: Set Retry count to be 0!\n") );
	}
	
	
	if(EncAlgorithm == RT_ENC_ALG_TKIP)
	{
		if((KeyIndex & ADD_KEY_IDX_AS)){//kcwu: Key is set by an Authenticator, exchange Tx/Rx MIC
			u1Byte tmpbuf[TKIP_MIC_KEY_LEN];
			PlatformMoveMemory(tmpbuf, KeyMaterial+TKIP_ENC_KEY_LEN, TKIP_MIC_KEY_LEN);
			PlatformMoveMemory(KeyMaterial+TKIP_ENC_KEY_LEN, 
				KeyMaterial+TKIP_ENC_KEY_LEN+TKIP_MIC_KEY_LEN, TKIP_MIC_KEY_LEN);
			PlatformMoveMemory(KeyMaterial+TKIP_ENC_KEY_LEN+TKIP_MIC_KEY_LEN, tmpbuf, TKIP_MIC_KEY_LEN);
		}
		PlatformMoveMemory(pSecInfo->RxMICKey, KeyMaterial+TKIP_MICKEYRX_POS, TKIP_MIC_KEY_LEN);
		PlatformMoveMemory(pSecInfo->TxMICKey, KeyMaterial+TKIP_MICKEYTX_POS, TKIP_MIC_KEY_LEN);
	}
	else if(EncAlgorithm == RT_ENC_ALG_AESCCMP)
	{
		//2004/09/07, kcwu, AES Key Initialize
		AESCCMP_BLOCK   blockKey;
		PlatformMoveMemory(blockKey.x, pSecInfo->KeyBuf[IsGroup?KeyIndex:PAIRWISE_KEYIDX],16);
		AES_SetKey(blockKey.x,
					AESCCMP_BLK_SIZE*8,
					(u4Byte *)pSecInfo->AESKeyBuf[IsGroup?KeyIndex:PAIRWISE_KEYIDX]);     // run the key schedule
	}
	else if(EncAlgorithm==RT_ENC_ALG_WEP40 || EncAlgorithm==RT_ENC_ALG_WEP104)
	{
		RT_TRACE( COMP_SEC, DBG_LOUD, ("MgntActSet_802_11_ADD_KEY(): SecLvl>0, EncAlgorithm=%d, IsGroup=%d\n", EncAlgorithm, IsGroup) );
	}


	//kcwu: Maybe it can be removed. After add WPA feature, run 2c_wlan_wep
	//          to test the following flag
	Adapter->MgntInfo.bMlmeStartReqRsn = MLMESTARTREQ_KEY_CHANGE;

	//No matter software en/de-cryption is turned on or not, always set key into hardware
	Adapter->HalFunc.SetKeyHandler(Adapter, 
		KeyIndex,
		MacAddress,
		IsGroup,
		EncAlgorithm,
		FALSE,
		FALSE);
	
	RT_TRACE( COMP_SEC, DBG_LOUD, ("MgntActSet_802_11_ADD_KEY(): return TRUE. <====\n") );
	return TRUE;
}



//Add for RSNA IBSS , by CCW
BOOLEAN
MgntActSet_RSNA_REMOVE_DEAULT_KEY(
	PADAPTER		Adapter,
	u4Byte			KeyIndex,
	pu1Byte			MacAddress
)
{
	PRT_SECURITY_T   pSecInfo = &Adapter->MgntInfo.SecurityInfo;
	ULONG                   index;
	PPER_STA_DEFAULT_KEY_ENTRY   pDefKey;

	RT_TRACE(COMP_RSNA, DBG_LOUD, ("====> Remove Default Key\n"));
	RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, " Station MAC : ", MacAddress);
	pDefKey = pSecInfo->PerStaDefKeyTable;

	for( index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++, pDefKey++)
	{
		if( pDefKey->Valid && (PlatformCompareMemory(MacAddress,pDefKey->MACAdrss, 6)==0))
			break;
	}

	if( index == MAX_NUM_PER_STA_KEY )
		return FALSE; // Can't find it. return NDIS_STATUS_INVALID_DATA
	else
	  	PlatformZeroMemory(pDefKey->DefKeyBuf+KeyIndex, AESCCMP_BLK_SIZE_TOTAL);

	pDefKey->DefkeyValid[KeyIndex] = FALSE;
	
	for( index = 0 ; index < 4 ; index++ )
	{
		if(pDefKey->DefkeyValid[index] == TRUE)
			break;
	}

	if( index == 4 )
	{
		PlatformZeroMemory(pDefKey ,sizeof(PER_STA_DEFAULT_KEY_ENTRY));
		pDefKey->Valid = FALSE; 
	}
	RT_TRACE(COMP_RSNA, DBG_LOUD, ("====>Remove Default Key\n"));
	return TRUE;
}

//vivi added for new cam search flow, 2009102
/*
	vivi only modify add default key function but not remove default key function,
	cause actually MgntActSet_RSNA_REMOVE_DEAULT_KEY will not be called, 
	and we reset all the key in N6CQuerySet_DOT11_RESET_REQUEST: SecClearAllKeys, 
	in that function all sw&hw key will be reset including RSNA IBSS key
*/
BOOLEAN
MgntActSet_RSNA_ADD_DEAULT_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte			KeyLen,
	pu1Byte               pKeyMaterial,
	pu1Byte		      MacAddress
)
{
	PRT_SECURITY_T   pSecInfo = &Adapter->MgntInfo.SecurityInfo;
	ULONG                   index;
	ULONG                   emptyindex;
	PPER_STA_DEFAULT_KEY_ENTRY   pDefKey;
	AESCCMP_BLOCK		blockKey;
	u4Byte	EntryId = 0;
	u2Byte	usConfig = 0;
	//u1Byte	CAM_CONST_BROAD[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	u1Byte	NullMacadress[6] = {0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00};
	u1Byte	MacAddress_Original[6] = {0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00};
	
	RT_TRACE(COMP_RSNA, DBG_LOUD, ("====> Add Default Key\n"));
	RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, " Station MAC : ", MacAddress);
	RT_PRINT_DATA(COMP_RSNA, DBG_LOUD, " KeyMaterial : ", pKeyMaterial, KeyLen);
	RT_TRACE(COMP_RSNA, DBG_LOUD, (" EncAlgorithm: %02x, Key index : %02x \n",
		EncAlgorithm, KeyIndex));
	emptyindex = MAX_NUM_PER_STA_KEY;
	pDefKey = pSecInfo->PerStaDefKeyTable;

	if (IS_EN_DE_CRYPTION_NEW_CAM_SUPPORT())
	{
		//for sw, mac address 0 will be covered, so save the original mac address
		PlatformMoveMemory(MacAddress_Original, MacAddress, 6);
	}

	if(KeyLen != AESCCMP_BLK_SIZE)
	{
		return FALSE;
	}

	if( MacAddress[0] == 0 &&  MacAddress[1] == 0 &&
	    MacAddress[2] == 0 &&  MacAddress[3] == 0 &&
	    MacAddress[4] == 0 &&  MacAddress[5] == 0 )
	{
		//
		// This is our group key.
		//
		PlatformMoveMemory(MacAddress,Adapter->CurrentAddress,6);
	}
	
	for( index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++, pDefKey++)
	{
		if( pDefKey->Valid && (PlatformCompareMemory(MacAddress,pDefKey->MACAdrss, 6)==0))
			break; // find out the same entry.

		if( !pDefKey->Valid && emptyindex == MAX_NUM_PER_STA_KEY )
			emptyindex = index;  // find out one empty entry. 
	}

	if( index == MAX_NUM_PER_STA_KEY ) //Don't find the same entry
	{
		if( emptyindex == MAX_NUM_PER_STA_KEY )
			return FALSE;  //return NIDS_STATUS_RESOURES
		pDefKey = pSecInfo->PerStaDefKeyTable + emptyindex;

		//Set MAC
		PlatformMoveMemory( pDefKey->MACAdrss , MacAddress , 6 );
	}
	else // find the same entry
	{
		pDefKey = pSecInfo->PerStaDefKeyTable + index;
	}
	
	//Set Key 
	PlatformMoveMemory( blockKey.x , pKeyMaterial , KeyLen);
	AES_SetKey(blockKey.x, AESCCMP_BLK_SIZE*8, (pu4Byte)pDefKey->DefKeyBuf[KeyIndex] );
	
	
	pDefKey->DefkeyValid[KeyIndex] = TRUE;
	pDefKey->Valid = TRUE;	
	//all above reserved, vivi. actually for sw en/decryption

	if (IS_EN_DE_CRYPTION_NEW_CAM_SUPPORT())
	{
		//1 vivi add new for hw encryption, 20091028
		//adhoc or infra sta in aes mode, pMacAddr equals 0 means tx broadcast key(adhoc) or rx broadcast key(infra)
		if(PlatformCompareMemory(MacAddress_Original, NullMacadress, 6) == 0)
		{
			//set in entry 0-3, hw find key according to KeyIndex, not address
			//MacAddress = CAM_CONST_BROAD;
			int	i;
			EntryId = (u1Byte)KeyIndex;
			//find the cam 0~3 for group key
			for(i = 0 ; i < 4 ; i++)
			{
				if(Adapter->MgntInfo.SWCamTable[i].bUsed)
					PlatformZeroMemory(  &	Adapter->MgntInfo.SWCamTable[i] , sizeof(SW_CAM_TABLE) );
			}

			//set SW CAM
			Adapter->MgntInfo.SWCamTable[EntryId].bUsed = 1;
			Adapter->MgntInfo.SWCamTable[EntryId].ulUseDK= KeyIndex;
			PlatformMoveMemory(Adapter->MgntInfo.SWCamTable[EntryId].macAddress, MacAddress_Original, 6);
			CopyMem( Adapter->MgntInfo.SWCamTable[EntryId].pucKey , pKeyMaterial , sizeof(Adapter->MgntInfo.SWCamTable[EntryId].pucKey) );	
			Adapter->MgntInfo.SWCamTable[EntryId].ulEncAlg = EncAlgorithm;
			Adapter->MgntInfo.SWCamTable[EntryId].ulKeyId = EntryId;	//cam entryid in cam			
			Adapter->MgntInfo.SWCamTable[EntryId].portNumber = 0;	// to ID different port		
		}
		else //otherwise, set EntryId according to macaddr 
		{
			//find EntryId, and also set sw cam, this sw cam table is consistent with hw table...later virtual also use sw cam table
			EntryId = Sta_FindFreeEntry(Adapter,
							MacAddress_Original,
							KeyIndex,
							EncAlgorithm,
							pKeyMaterial);
			if(EntryId == (Adapter->TotalCamEntry + 1))
				return FALSE;	//return NIDS_STATUS_RESOURES
		}

		//Set HW CAM
		usConfig=usConfig|CFG_VALID|((u2Byte)(Sec_MapNewCipherToDepCipherAlg(pSecInfo->GroupEncAlgorithm))<<2)|(u1Byte)KeyIndex;
		CAM_program_entry(Adapter, EntryId, MacAddress_Original, pKeyMaterial, usConfig);

		//vivi add for debug
		RT_TRACE(COMP_RSNA, DBG_LOUD, (" SWCam Information: EntryId: %02x\n", EntryId));
		RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, " Station MAC : ", Adapter->MgntInfo.SWCamTable[EntryId].macAddress);
		RT_PRINT_DATA(COMP_RSNA, DBG_LOUD, " KeyMaterial : ", Adapter->MgntInfo.SWCamTable[EntryId].pucKey, KeyLen);
		RT_TRACE(COMP_RSNA, DBG_LOUD, (" EncAlgorithm: %02x\n",
			Adapter->MgntInfo.SWCamTable[EntryId].ulEncAlg));
		RT_TRACE(COMP_RSNA, DBG_LOUD, (" KeyINdex: %02x\n",
			Adapter->MgntInfo.SWCamTable[EntryId].ulUseDK));
	}
		
	RT_TRACE(COMP_RSNA, DBG_LOUD, ("====>Add Default Key\n"));
	return TRUE;
	
}


BOOLEAN
MgntActSet_RSNA_REMOVE_MAPPING_KEY(
	PADAPTER		Adapter,
	pu1Byte			MacAddress
)
{
	PRT_SECURITY_T   pSecInfo = &Adapter->MgntInfo.SecurityInfo;
	ULONG                   index;
	PPER_STA_MPAKEY_ENTRY   pMAPKey;
	RT_TRACE(COMP_RSNA, DBG_LOUD, ("====> Remove Mapping Key\n"));
	RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, " Station MAC : ", MacAddress);

	pMAPKey = pSecInfo->MAPKEYTable;

	for( index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++, pMAPKey++)
	{
		if( pMAPKey->Valid && (PlatformCompareMemory(MacAddress,pMAPKey->MACAdrss, 6)==0))
			break;
	}

	if( index == MAX_NUM_PER_STA_KEY )
		return FALSE; // Can't find it. return NDIS_STATUS_INVALID_DATA
	else
	  	PlatformZeroMemory(pMAPKey, sizeof(PER_STA_MPAKEY_ENTRY));

	pMAPKey->Valid = FALSE;
	RT_TRACE(COMP_RSNA, DBG_LOUD, ("====>Remove Mapping Key\n"));
	return TRUE;
}


BOOLEAN
MgntActSet_RSNA_ADD_MAPPING_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	u4Byte		       KeyLen,
	pu1Byte               pKeyMaterial,
	pu1Byte			MacAddress
)
{
	PRT_SECURITY_T   pSecInfo = &Adapter->MgntInfo.SecurityInfo;
	ULONG                   index;
	ULONG                   emptyindex;
	PPER_STA_MPAKEY_ENTRY   pMAPKey;
	AESCCMP_BLOCK		blockKey;
	u4Byte	EntryId = 0;
	u2Byte	usConfig = 0;

	RT_TRACE(COMP_RSNA, DBG_LOUD, ("====> Add Mapping Key\n"));
	RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, " Station MAC : ", MacAddress);
	//RT_PRINT_DATA(COMP_RSNA, DBG_LOUD, " KeyMaterial : ", pKeyMaterial, KeyLen);
	RT_TRACE(COMP_RSNA, DBG_LOUD, (" EncAlgorithm: %02x, Key index : %02x \n",
		EncAlgorithm, KeyIndex));

	pMAPKey = pSecInfo->MAPKEYTable;
	emptyindex = MAX_NUM_PER_STA_KEY;

	if(KeyLen != AESCCMP_BLK_SIZE)
	{
		return FALSE;
	}

	for( index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++, pMAPKey++)
	{
		if( pMAPKey->Valid && (PlatformCompareMemory(MacAddress,pMAPKey->MACAdrss, 6)==0))
			break; // find out the same entry.

		if( !pMAPKey->Valid && emptyindex == MAX_NUM_PER_STA_KEY )
			emptyindex = index;  // find out one empty entry. 
	}

	if( index == MAX_NUM_PER_STA_KEY ) //Don't find the same entry
	{
		if( emptyindex == MAX_NUM_PER_STA_KEY )
			return FALSE;  //return NIDS_STATUS_RESOURES
		pMAPKey = pSecInfo->MAPKEYTable + emptyindex;
		//Set MAC
		PlatformMoveMemory( pMAPKey->MACAdrss , MacAddress , 6 );
	}
	else // find the same entry
	{
		pMAPKey = pSecInfo->MAPKEYTable + index;
	}

	//Set Key 
	PlatformMoveMemory( blockKey.x , pKeyMaterial , KeyLen);
	AES_SetKey(blockKey.x, AESCCMP_BLK_SIZE*8, (pu4Byte)pMAPKey->MapKeyBuf );

	pMAPKey->Valid = TRUE;

	if (IS_EN_DE_CRYPTION_NEW_CAM_SUPPORT())
	{
		//all above reserved, vivi. actually for sw en/decryption

		//1 vivi add new for hw encryption, 20091028
		//get EntryId according to macaddr 
		//find EntryId, and also set sw cam, this sw cam table is consistent with hw table...later virtual also use sw cam table
		EntryId = Sta_FindFreeEntry(Adapter,
							MacAddress,
							KeyIndex,
							EncAlgorithm,
							pKeyMaterial);

		//Set HW CAM
		usConfig=usConfig|CFG_VALID|((u2Byte)(pSecInfo->GroupEncAlgorithm)<<2)|(u1Byte)KeyIndex;
		CAM_program_entry(Adapter, EntryId, MacAddress, pKeyMaterial, usConfig);

		//vivi add for debug
		RT_TRACE(COMP_RSNA, DBG_LOUD, (" EntryId: %02x\n", EntryId));
		RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, " Station MAC : ", Adapter->MgntInfo.SWCamTable[EntryId].macAddress);
		RT_PRINT_DATA(COMP_RSNA, DBG_LOUD, " KeyMaterial : ", Adapter->MgntInfo.SWCamTable[EntryId].pucKey, KeyLen);
		RT_TRACE(COMP_RSNA, DBG_LOUD, (" EncAlgorithm: %02x\n",
			Adapter->MgntInfo.SWCamTable[EntryId].ulEncAlg));
		RT_TRACE(COMP_RSNA, DBG_LOUD, (" KeyIndex: %02x\n",
			Adapter->MgntInfo.SWCamTable[EntryId].ulUseDK));
	}

	RT_TRACE(COMP_RSNA, DBG_LOUD, ("====>Add Mapping Key\n"));
	return TRUE;
}

BOOLEAN
MgntActSet_802_11_REMOVE_KEY(
	PADAPTER		Adapter,
	RT_ENC_ALG		EncAlgorithm,
	u4Byte			KeyIndex,
	pu1Byte			BSSID,
	BOOLEAN			IsGroup
)
{
//2004/08/23, kcwu, remove key
//	PRT_SECURITY_T pSecInfo = &Adapter->MgntInfo.SecurityInfo;
	KeyIndex&=0x00000003;
	if(IsGroup){
		SecClearGroupKeyByIdx(Adapter, (u1Byte)KeyIndex);
	}else{
		SecClearPairwiseKeyByMacAddr(Adapter, BSSID);
	}

	return TRUE;
}





BOOLEAN
MgntActSet_802_11_WIRELESS_MODE(
	PADAPTER		Adapter,
	WIRELESS_MODE  	WirelessMode
)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u2Byte			SupportedWirelessMode = Adapter->HalFunc.GetSupportedWirelessModeHandler(Adapter);

	if(	(WirelessMode!=WIRELESS_MODE_AUTO) &&
		((WirelessMode & SupportedWirelessMode) == 0) )
	{ // Don't switch to unsupported wireless mode, 2006.02.15, by rcnjko.
		RT_TRACE(COMP_DBG, DBG_SERIOUS, 
			("MgntActSet_802_11_WIRELESS_MODE(): WirelessMode(0x%X) is not supported (0x%X)!\n", 
			WirelessMode, SupportedWirelessMode));
		return FALSE;
	}

	if(RT_DRIVER_STOP(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Driver is going to stop \n"));
		return FALSE;		
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 4\n"));
		return FALSE;
	}

	FunctionIn(COMP_MLME);

	if(MgntScanInProgress(pMgntInfo))
		MgntResetScanProcess( Adapter );
	MgntActSet_802_11_DISASSOCIATE( Adapter, disas_lv_ss );
	if(MgntRoamingInProgress(pMgntInfo))
		MgntRoamComplete(Adapter, MlmeStatus_invalid);
	MgntResetLinkProcess( Adapter );

	Adapter->RegWirelessMode = WirelessMode;

	// In prevent of switching back to wrong band in scancallback(). 2004.12.10, by rcnjko. 
	pMgntInfo->SettingBeforeScan.WirelessMode = WirelessMode;

	switch(WirelessMode)
	{
		case WIRELESS_MODE_A:
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set WIRELESS_MODE_A\n"));
			break;		
		case WIRELESS_MODE_B:
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set WIRELESS_MODE_B\n"));
			break;		
		case WIRELESS_MODE_G:
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set WIRELESS_MODE_G\n"));
			break;		
		case WIRELESS_MODE_AUTO:
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set WIRELESS_MODE_AUTO\n"));
			break;		
		case WIRELESS_MODE_N_24G: 
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set WIRELESS_MODE_N_24G\n"));
			break;
		case WIRELESS_MODE_AC_24G: 
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set WIRELESS_MODE_AC_24G\n"));
			break;	
		case WIRELESS_MODE_N_5G: 
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set WIRELESS_MODE_N_5G\n"));
			break;	
		case WIRELESS_MODE_AC_5G: 
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set WIRELESS_MODE_AC_5G\n"));
			break;		
		default: //For MacOs Warning: "WIRELESS_MODE_UNKNOWN" not handled in switch.
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set WIRELESS_MODE_UNKNOWN\n"));
			break;	
	}

	Adapter->HalFunc.SetWirelessModeHandler(Adapter, (u1Byte)Adapter->RegWirelessMode);

	// Adjust dot11CurrentChannelNumber here, we dot11CurrentChannelNumber is not
	// a legal channel for current band, it will be reset to first legal channel
	// of current band. 2004.06.09, by rcnjko.
	MgntActSet_802_11_REG_20MHZ_CHANNEL_AND_SWITCH(Adapter, pMgntInfo->dot11CurrentChannelNumber);

	FunctionOut(COMP_MLME);

	return TRUE;
}



BOOLEAN
MgntActSet_802_11_RETRY_LIMIT(
	PADAPTER Adapter, 
	u2Byte ShortRetryLimit, 
	u2Byte LongRetryLimit
)
{
	if( ShortRetryLimit != 0 ){
		//RTL8185_TODO: Set short retry limit
	}

	if( LongRetryLimit != 0 ){
		//RTL8185_TODO: Set long retry limit
	}

	RT_TRACE(COMP_INIT, DBG_SERIOUS,("TODO: Set short/long retry limit .\n") );

	return TRUE;
}

static BOOLEAN
MgntActSet_802_11_20MHZ_CHANNEL(
	PADAPTER	Adapter, 
	u1Byte 		channel
)
{

// Keep this function private: -----------------------------------------------------------------------------------------------
// 	+ If you want to switch channel in MAC common layer perspective, please use  MgntActSet_802_11_CHANNEL_AND_BANDWIDTH().
//		+ This can make sure which 20, 40, 80, 80+80, 160 channel you want to switch to
// ----------------------------------------------------------------------------------------------------------------------

	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	RT_TRACE(COMP_MLME,DBG_LOUD,("====>MgntActSet_802_11_20MHZ_CHANNEL: %d\n", channel));

	if(ACTING_AS_AP(Adapter) && pMgntInfo->bAutoSelChnl)
	{ // To prevent that UI set channel when bAutoSelChnl==TRUE. 2005.12.27, by rcnjko.
		RT_TRACE(COMP_AP, DBG_LOUD, ("MgntActSet_802_11_20MHZ_CHANNEL(): reject to switch channel to %d under AP mode auto-channel selection process...\n",channel));
	}

	//Sherry added for dual band 20110517
	if(IS_DUAL_BAND_SUPPORT(Adapter))
	{
		RT_CHNL_LIST_ENTRY	ChnlListEntryArray[MAX_CHANNEL_NUM] = {0};
		u1Byte				ChannelLen = 0, i = 0, firstLegalIndex = 0xFF;
		
		ChannelLen = RtGetDualBandChannel(Adapter, ChnlListEntryArray);
			
		if(ChannelLen == 0)
			return FALSE;

		for(i = 0; i < ChannelLen; i ++)
		{
			if(ACTING_AS_AP(Adapter))
			{
				if(!DFS_ApChnlLocked(Adapter, ChnlListEntryArray[i].ChannelNum))
				{
					if(firstLegalIndex == 0xff){
						firstLegalIndex = i;
					}
					if(channel == ChnlListEntryArray[i].ChannelNum)
					{
						break;
					}
				}
			}
			else
			{
				if(firstLegalIndex == 0xff)
					firstLegalIndex = 0;
				if(channel == ChnlListEntryArray[i].ChannelNum)
				{
					break;
				}
			}
		}
		if(i == ChannelLen){
			// Prefast warning C6385: Reading invalid data from 'ChnlListEntryArray':  the readable size is '760' bytes, but '5120' bytes may be read.
			// false positive, disable it here
#pragma warning( disable: 6385 )
			channel = ChnlListEntryArray[firstLegalIndex].ChannelNum;
		}
		
#if 0		
		if(channel <= 14)
		{
			if(IS_WIRELESS_MODE_5G(Adapter))	
				pMgntInfo->dot11CurrentWirelessMode = WIRELESS_MODE_N_24G;
		}
		else
		{
			if(IS_WIRELESS_MODE_24G(Adapter))
				pMgntInfo->dot11CurrentWirelessMode = WIRELESS_MODE_N_5G;
		}

		// Let Hardware WirelessMode Consistent ---------------------------------------------------
		if(pHalData->CurrentWirelessMode != pMgntInfo->dot11CurrentWirelessMode)
		{
			Adapter->HalFunc.SetWirelessModeHandler(Adapter,pMgntInfo->dot11CurrentWirelessMode);
		}
		// ------------------------------------------------------------------------------------
	
#else
		RT_TRACE_F(COMP_MLME,DBG_LOUD, ("ChangeWirelessModeHandler\n"));
		HalChangeWirelessMode(Adapter,channel);

//		SetupWirelessMode(Adapter,channel);		
#endif		

	
	}
	else
	{
		channel = ToLegalChannel(Adapter, channel);
	}


	{ // Update MAC Channel and Wireless Mode ---------------------------------------------------
		PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);
		
		while(pLoopAdapter != NULL)
		{
			pLoopAdapter->MgntInfo.dot11CurrentChannelNumber = channel;

			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("pLoopAdapter->MgntInfo.dot11CurrentChannelNumber: %d\n", pLoopAdapter->MgntInfo.dot11CurrentChannelNumber));
			pLoopAdapter->MgntInfo.dot11CurrentWirelessMode = Adapter->MgntInfo.dot11CurrentWirelessMode;

			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}// ----------------------------------------------------------------------------------

	DFS_ApIfNeedMonitorChnl(Adapter);

	Mgnt_SwChnl(Adapter, pMgntInfo->dot11CurrentChannelNumber,1);

	return TRUE;
}

// Use this interface to swith channel and bandwidth in MAC layer --------------
VOID
MgntActSet_802_11_CHANNEL_AND_BANDWIDTH(
	PADAPTER			pAdapter, 
	u1Byte 				Primary20MhzChannel,
	CHANNEL_WIDTH		BandWidthMode,
	EXTCHNL_OFFSET		ExtChnlOffsetOf40MHz,
	EXTCHNL_OFFSET		ExtChnlOffsetOf80MHz,
	u1Byte				Secondary80MhzChannelCenterFrequency
)
{
	int i;

	FunctionIn(COMP_MLME);
	
	switch(BandWidthMode)
	{
		case CHANNEL_WIDTH_20:
			RT_ASSERT(Primary20MhzChannel != 0, ("Wrong Primary20MhzChannel: %d\n", Primary20MhzChannel));
			break;
			
		case CHANNEL_WIDTH_40:
			RT_ASSERT(Primary20MhzChannel != 0, ("Wrong Primary20MhzChannel: %d\n", Primary20MhzChannel));
			RT_ASSERT(ExtChnlOffsetOf40MHz == EXTCHNL_OFFSET_UPPER || ExtChnlOffsetOf40MHz == EXTCHNL_OFFSET_LOWER, ("Wrong ExtChnlOffsetOf40MHz: %d\n", ExtChnlOffsetOf40MHz));
			break;

		case CHANNEL_WIDTH_80:
			RT_ASSERT(Primary20MhzChannel != 0, ("Wrong Primary20MhzChannel: %d\n", Primary20MhzChannel));
			RT_ASSERT(ExtChnlOffsetOf40MHz == EXTCHNL_OFFSET_UPPER || ExtChnlOffsetOf40MHz == EXTCHNL_OFFSET_LOWER, ("Wrong ExtChnlOffsetOf40MHz: %d\n", ExtChnlOffsetOf40MHz));
			RT_ASSERT(ExtChnlOffsetOf80MHz == EXTCHNL_OFFSET_UPPER || ExtChnlOffsetOf80MHz == EXTCHNL_OFFSET_LOWER, ("Wrong ExtChnlOffsetOf80MHz: %d\n", ExtChnlOffsetOf80MHz));
			break;
			
		case CHANNEL_WIDTH_160:
		case CHANNEL_WIDTH_80_80:
			RT_ASSERT(0, ("Currently No Use for BandwidthMode: %d\n", BandWidthMode));
			break;

		default:
			RT_ASSERT(0, ("Wrong BandwidthMode: %d\n", BandWidthMode));
			break;
	}

//skip switch channel when channel is the same
//by sherry 20130117
	if(GetDefaultAdapter(pAdapter)->bInHctTest)
	{
		if(GET_HAL_DATA(pAdapter)->CurrentChannel == Primary20MhzChannel )
		{	
			RT_TRACE(COMP_MLME,DBG_LOUD,("MgntActSet_802_11_CHANNEL_AND_BANDWIDTH: the channel is same return \n"));
			return;
		}
	}

	// Switch to Primary 20Mhz Channel
	//MgntActSet_802_11_20MHZ_CHANNEL(pAdapter, Primary20MhzChannel);

	// Currently do not consider VHT support: This should be extended for 802.11ac
	CHNL_SetBwChnl(pAdapter, Primary20MhzChannel, BandWidthMode, ExtChnlOffsetOf40MHz);

	//wait for channel switch successful by sherry 20130117
	for(i = 0; i < 100; i++)  
	{			
		if(!RT_IsSwChnlAndBwInProgress(pAdapter) && !CHNL_SwChnlAndSetBwInProgress(pAdapter))
				break;
		
		RT_TRACE_F(COMP_MULTICHANNEL, DBG_LOUD, ("MgntActSet_802_11_CHANNEL_AND_BANDWIDTH:Delay 100 us Waiting for Switch Channel Complete!\n"));
		delay_us(100);
	}

	FunctionOut(COMP_MLME);
}
// -------------------------------------------------------------------


BOOLEAN
MgntActSet_802_11_REG_20MHZ_CHANNEL_AND_SWITCH(
	PADAPTER	Adapter, 
	u1Byte 		channel
)
{
	PADAPTER pLoopAdapter = NULL;
	BOOLEAN	bSwitchOk = FALSE;

	bSwitchOk = MgntActSet_802_11_20MHZ_CHANNEL(Adapter, channel);

	if(bSwitchOk)
	{ // Update Reg MAC Channel --------------------------------------------------------------
		pLoopAdapter = GetDefaultAdapter(Adapter);
		
		while(pLoopAdapter != NULL)
		{
			pLoopAdapter->MgntInfo.Regdot11ChannelNumber = channel;
			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}// ----------------------------------------------------------------------------------

	return bSwitchOk;
}

// Annie, 2004-12-27
BOOLEAN
MgntActSet_802_11_CHANNELPLAN(
	PADAPTER	Adapter, 
	u2Byte 		ChannelPlan
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	if(pMgntInfo->bChnlPlanFromHW)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_CHANNELPLAN(): reject to set channel plan. (HW reject)\n"));
		return FALSE;
	}

	if(ChannelPlan >= RT_CHANNEL_DOMAIN_MAX)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_CHANNELPLAN(): not support this channel plan (%d > MAX: %d). (HW reject)\n", 
			ChannelPlan, RT_CHANNEL_DOMAIN_MAX));
		return FALSE;
	}
	pMgntInfo->ChannelPlan = (RT_CHANNEL_DOMAIN)ChannelPlan;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_CHANNELPLAN(): set channel plan (%d)\n", ChannelPlan));

	return TRUE;
}


BOOLEAN
MgntActSet_802_11_PMKID(
	IN	PADAPTER     		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	ULONG			InformationBufferLength
)
{
	RT_TRACE( COMP_SEC, DBG_LOUD, ("MgntActSet_802_11_PMKID(): Length=%d\n", InformationBufferLength) );
	RT_PRINT_DATA( COMP_SEC, DBG_TRACE, "OID Buffer", InformationBuffer, InformationBufferLength );
	
	SecSetPMKID( Adapter, (PN5_802_11_PMKID)InformationBuffer );
	return TRUE;
}



u4Byte
ComputeCrc(
    IN pu1Byte	Buffer,
    IN u4Byte	Length
    )
{
    u4Byte	Crc, Carry;
    u4Byte	i, j;
    u1Byte	CurByte;

    Crc = 0xffffffff;

    for (i = 0; i < Length; i++)
    {
        CurByte = Buffer[i];
        for (j = 0; j < 8; j++)
        {
            Carry = ((Crc & 0x80000000) ? 1 : 0) ^ (CurByte & 0x01);
            Crc <<= 1;
            CurByte >>= 1;
            if (Carry)
            {
                Crc = (Crc ^ 0x04c11db6) | Carry;
            }
        }
    }
    Crc = Crc & 0x0ff000000;                    // 95.05.04     Sid
    return Crc;
}


VOID
GetMulticastBit(
    IN u1Byte Address[6],
    OUT u1Byte * Byte,
    OUT u1Byte * Value
    )
{
    u4Byte 	Crc;
    u4Byte 	BitNumber;

    Crc = ComputeCrc(Address, 6);

    // The bit number is now in the 6 most significant bits of CRC.
    BitNumber = (u4Byte)((Crc >> 26) & 0x3f);
    *Byte = (u1Byte)(BitNumber / 8);
    *Value = (u1Byte)((u1Byte)1 << (BitNumber % 8));
}


VOID
MgntActSet_802_3_MULTICAST_LIST(
	PADAPTER		Adapter,
	pu1Byte			MCListbuf,
	u4Byte			MCListlen,
	BOOLEAN			bAcceptAllMulticast
)
{
	u1Byte			MCReg[8];
	u4Byte			i;
	u1Byte 			Byte;
	u1Byte			Bit;
	u4Byte			MCR[2];

	RT_TRACE(COMP_MLME, DBG_TRACE, ("MgntActSet_802_3_MULTICAST_LIST(): Length = %d, bAcceptAllMulticast = %d\n", MCListlen, bAcceptAllMulticast));

	Adapter->MCAddrCount = MCListlen/6;

	if(Adapter->MCAddrCount > 0)
	{
		CopyMem( Adapter->MCList, MCListbuf, MCListlen);
	}
	else
	{
		PlatformZeroMemory(Adapter->MCList, (MAX_MCAST_LIST_NUM * ETHERNET_ADDRESS_LENGTH));
	}

	for( i=0; i<8; i++ ){ MCReg[i] = 0; }

	for( i=0; i < Adapter->MCAddrCount; i++ )
	{
		RT_PRINT_ADDR(COMP_MLME | COMP_OID_SET, DBG_TRACE, "MgntActSet_802_3_MULTICAST_LIST: ", Adapter->MCList[i]); 
		if( i < MAX_MCAST_LIST_NUM )
		{
			GetMulticastBit( Adapter->MCList[i], &Byte, &Bit );
			MCReg[Byte] |= Bit;
		}
	}

	if( ACTING_AS_AP(Adapter) || bAcceptAllMulticast ){
		MCR[0] = 0xffffffff;
		MCR[1] = 0xffffffff;
	}
	else
	{
		MCR[0] = 0;MCR[1] = 0;

		for (i=0; i<4; i++)
		{
			MCR[0] = MCR[0] + ((MCReg[i])<<(8*i));
		}
		for (i=4; i<8; i++)
		{
			MCR[1] = MCR[1] + ((MCReg[i])<<(8*(i-4)));
		}
	}

	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MULTICAST_REG, (pu1Byte)(&MCR[0]) );

}



// 
// Change current and default preamble mode.
// 2005.01.06, by rcnjko.
//
VOID
MgntActSet_802_11_PREAMBLE_MODE(
	PADAPTER		Adapter,
	u1Byte	PreambleMode
)
{
	PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;

	pMgntInfo->RegPreambleMode = PreambleMode;
	pMgntInfo->dot11CurrentPreambleMode = PreambleMode;
}


// 
// Change current and default preamble mode.
// 2005.01.06, by rcnjko.
//
BOOLEAN
MgntActSet_802_11_PowerSaveMode(
	PADAPTER		Adapter,
	RT_PS_MODE		rtPsMode
)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	u1Byte	FwPwrMode;

	// Currently, we do not change power save mode on IBSS mode. 
	if(pMgntInfo->mIbss)	
		return FALSE;

	if(pMgntInfo->dot11PowerSaveMode == rtPsMode)
		return TRUE;
	
	// Update power save mode configured.
	pMgntInfo->dot11PowerSaveMode = rtPsMode;

	// Determine ListenInterval. 
	if(pMgntInfo->dot11PowerSaveMode == eMaxPs)
	{
		pMgntInfo->ListenInterval = pMgntInfo->dot11DtimPeriod;
	}
	else
	{
		pMgntInfo->ListenInterval = 2;
	}

	if(pMgntInfo->mPss != eAwake && rtPsMode == eActive)		
		PlatformSetTimer(Adapter, &(pMgntInfo->AwakeTimer), 0);

	if(!pPSC->bFwCtrlLPS)
	{
		// Awake immediately and notify the AP.
		if(!pMgntInfo->bAwakePktSent && rtPsMode == eActive)
		{
			// Notify the AP we awke.
			SendNullFunctionData(Adapter, pMgntInfo->Bssid, FALSE);
		}
		else if(rtPsMode != eActive)
		{
			// Notify the AP we are going to dozed.
			SendNullFunctionData(Adapter, pMgntInfo->Bssid, TRUE);
		}
	}

	// <FW control LPS>
	// 1. Enter PS mode 
	//    Set RPWM to Fw to turn RF off and send H2C FwPwrMode cmd to set Fw into PS mode.
	// 2. Leave PS mode
	//    Send H2C FwPwrMode cmd to Fw to set Fw into Active mode and set RPWM to turn RF on.
	// By tynli. 2009.01.19.
	if((pPSC->bFwCtrlLPS))
	{	
		if(pMgntInfo->dot11PowerSaveMode == eActive)
		{
			FwPwrMode = FW_PS_ACTIVE_MODE;
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_FW_PWRMODE, (pu1Byte)(&FwPwrMode));
		}
		else
		{
			if(GetFwLPS_Doze(Adapter))
			{
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_H2C_FW_PWRMODE, (pu1Byte)(&pPSC->FWCtrlPSMode));
			}
			else
			{
				// Reset the power save related parameters.
				pMgntInfo->dot11PowerSaveMode = eActive;
			}
		}
	}

	return TRUE;
}


// 
// Change to either AP or STA mode. 
// 2005.05.27, by rcnjko.
//
BOOLEAN
MgntActSet_ApMode(
	PADAPTER		Adapter,
	BOOLEAN			bApMode
)
{

	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);

	if(bApMode)
	{ // AP mode.

		if(!ACTING_AS_AP(Adapter))
		{ // Turn off IPS in AP mode for we shall always be active in this case, 070816, by rcnjko.
			IPSDisable(Adapter,FALSE, IPS_DISABLE_DEF_OPMODE);
              }

		pMgntInfo->bSwitchingAsApInProgress = TRUE;
		MgntActSet_ApType(Adapter, TRUE);

		// <RJ_TODO> We use SW encryption/decryption in this stage, 
		// and we should use HW encryption/decryption later. 2005.07.01, by rcnjko.
		SecSetSwEncryptionDecryption(Adapter, FALSE, FALSE);

		AP_Reset(Adapter);
	}
	else
	{ // STA mode.
		if(!ACTING_AS_AP(Adapter)){
			RT_TRACE(COMP_AP, DBG_LOUD, ("MgntActSet_ApMode(): Switch to STA. Do nothing since already in STA mode.\n"));
			return TRUE;
		}
	
		AP_DisassociateAllStation(Adapter, unspec_reason);
		PlatformSleepUs(5000);
		if(ACTING_AS_AP(Adapter))
		{ // Restore original IPS setting, 070816, by rcnjko.
			IPSReturn(Adapter, IPS_DISABLE_DEF_OPMODE);
		}

//sherry syn with 92C_92D 20110701
//To indicate dissassociation in Vista/Win7 SoftAP Mode
//To let WZC show disconnect when change from AP to STA
		if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED)  //YJ,ADD,110504
		{
			DrvIFIndicateDisassociation(Adapter, unspec_reason, pMgntInfo->Bssid); 
		}
	
		pMgntInfo->bSwitchingAsApInProgress = TRUE;

		MgntActSet_ApType(Adapter, FALSE);

		//
		// 2012/01/17 CCW Add for fake soft ap & vwifi soft ap coexist mode.
		//
		PlatformZeroMemory(pMgntInfo->Bssid, 6);		
		
		AP_Reset(Adapter);

		// <RJ_TODO> I assume STA only using SW encryption/decryption.
		SecSetSwEncryptionDecryption(Adapter, FALSE, FALSE);
	}

	return TRUE;
}

VOID
MgntActSet_802_11_DtimPeriod(
	PADAPTER		Adapter,
	u1Byte			u1DtimPeriod
)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);

	pMgntInfo->Regdot11DtimPeriod = u1DtimPeriod;
	pMgntInfo->dot11DtimPeriod = pMgntInfo->Regdot11DtimPeriod;
}

//
// Update beacon interval. 2005.06.17, by rcnjko.
//
VOID
MgntActSet_802_11_BeaconInterval(
	PADAPTER		Adapter,
	u2Byte			u2BeaconPeriod
)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);

	if(u2BeaconPeriod != pMgntInfo->dot11BeaconPeriod)
	{
		pMgntInfo->Regdot11BeaconPeriod = u2BeaconPeriod;
		pMgntInfo->dot11BeaconPeriod = pMgntInfo->Regdot11BeaconPeriod;
		
		Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BEACON_INTERVAL, (pu1Byte)(&pMgntInfo->dot11BeaconPeriod) );
	}
}


//
// Send magic packet to each channel for WOL feature.
// Currently, we use the replace the probe request with magic packet to 
// achive the feature, see ScanCallback(), ScanComplete() and PreTransmitTcb() for details.
// 2005.06.27, by rcnjko.
//
VOID 
MgntActSet_802_11_ScanWithMagicPacket(
	PADAPTER		Adapter,
	pu1Byte			pDstAddr
)
{
	PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;

	pMgntInfo->bScanWithMagicPacket = TRUE;
	PlatformMoveMemory(pMgntInfo->MagicPacketDstAddr, pDstAddr, 6);

	MgntActSet_802_11_BSSID_LIST_SCAN(Adapter);

	if(!pMgntInfo->bScanInProgress)
	{ 
		// Prevent the scan request failed, 2005.06.29, by rcnjko.
		pMgntInfo->bScanWithMagicPacket = FALSE;
	}
}

VOID
MgntActSet_Passphrase(
	PADAPTER		Adapter,
	POCTET_STRING	posPassphrase
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PAUTH_GLOBAL_KEY_TAG	pGlInfo = &(pMgntInfo->globalKeyInfo);

	RT_ASSERT( (posPassphrase->Length >= 8 && posPassphrase->Length <= 64), 		// Modify length from 1-61 to 8-63. Annie, 2005-11-21.
		("MgntActSet_Passphrase(): passphrase len: %d !!!", posPassphrase->Length));    // Modify length 63 to 64 for PMK 64-Hex mode 

	if(ACTING_AS_AP(Adapter))
	{ // AP mode.

	PlatformZeroMemory( pGlInfo->Passphrase, PASSPHRASE_LEN );
	if(posPassphrase->Length >= 8 && posPassphrase->Length <= 63)				// Modify length from 1-61 to 8-63. Annie, 2005-11-21.
	{
		PlatformMoveMemory( pGlInfo->Passphrase, posPassphrase->Octet, posPassphrase->Length );
		pGlInfo->PassphraseLen = (u1Byte)posPassphrase->Length;
	}
	else
	{
		pGlInfo->PassphraseLen = 0;
	}

		//
		// Add for AP mode support PMK 64-Hex mode !!
		//
		if( posPassphrase->Length == 64 )
		{
			// 1. Save PMK data for 32 byte
			PlatformMoveMemory( pGlInfo->Passphrase, posPassphrase->Octet , 32 );
			// 2. Set PMK data
			PlatformMoveMemory( pGlInfo->PMK , pGlInfo->Passphrase , 32 );
			// 3. Set PassphraseLen to 64 ... Point to 64-Hex mode !!
			pGlInfo->PassphraseLen = 64;
		}
		
}
}




BOOLEAN
MgntActSet_ExcludedMacAddressList(
	PADAPTER		Adapter,
	pu1Byte			pMacAddrList,
	u4Byte			NumMacAddrList
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

	if (NumMacAddrList > MAX_EXCLUDED_MAC_ADDRESS_LIST)
	{
		return FALSE;
	}

	PlatformMoveMemory(
		pMgntInfo->ExcludedMacAddr,
		pMacAddrList,
		NumMacAddrList*6);
	pMgntInfo->ExcludedMacAddrListLength = NumMacAddrList;

	return TRUE;
}

VOID
Mgnt_SetLedForRfState(
	IN	PADAPTER				Adapter,
	IN	RT_RF_POWER_STATE		StateToSet,
	IN	RT_RF_CHANGE_SOURCE		ChangeSource
	)
{
}

VOID
MgntReturnTxResource(
	IN	PADAPTER			Adapter
	)
{

}

//
//	Description: 
//		Chang RF Power State.
//		Note that, only MgntActSet_RF_State() is allowed to set HW_VAR_RF_STATE.
//
//	Assumption:
//		PASSIVE LEVEL.
//
BOOLEAN
MgntActSet_RF_State(
	IN	PADAPTER			Adapter,
	IN	RT_RF_POWER_STATE	StateToSet,
	IN	RT_RF_CHANGE_SOURCE	ChangeSource,
	IN	BOOLEAN				ProtectOrNot
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	BOOLEAN				bActionAllowed = FALSE; 
	BOOLEAN				bConnectBySSID = FALSE;
	RT_RF_POWER_STATE 	rtState;
	u2Byte				RFWaitCounter = 0;
	PADAPTER			tempAdapter = NULL;
	BOOLEAN				bHwOrSwRfChange=FALSE;

	RT_TRACE(COMP_RF, DBG_LOUD, ("====>MgntActSet_RF_State(): StateToSet(%d)\n",StateToSet));

	if(ChangeSource >= RF_CHANGE_BY_HW)
	{
		bHwOrSwRfChange = TRUE;
	}

	//1//
	//1//<1>Prevent the race condition of RF state change. 
	//1//
	// Only one thread can change the RF state at one time, and others should wait to be executed. By Bruce, 2007-11-28.

	if(!ProtectOrNot)
	{

		while(TRUE)
		{
			PlatformAcquireSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
			if(pMgntInfo->RFChangeInProgress)
			{
				RT_TRACE(COMP_RF, DBG_LOUD, ("MgntActSet_RF_State(): RF Change in progress! Wait to set..StateToSet(%d).\n", StateToSet));
				// Set RF after the previous action is done. 
				while(pMgntInfo->RFChangeInProgress)
				{
					PlatformReleaseSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
					RFWaitCounter ++;
					RT_TRACE(COMP_RF, DBG_LOUD, ("MgntActSet_RF_State(): Wait 1 ms (%d times)...\n", RFWaitCounter));

					PlatformStallExecution(1000); // 1 ms
					// Wait too long, return FALSE to avoid to be stuck here.
					if(RFWaitCounter > 100)
					{
						RT_ASSERT(FALSE, ("MgntActSet_RF_State(): Wait too long to set RF\n"));
						// TODO: Reset RF state?
						return FALSE;
					}
					PlatformAcquireSpinLock(Adapter, RT_RF_STATE_SPINLOCK);				
				}
				PlatformReleaseSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
			}
			else
			{
				pMgntInfo->RFChangeInProgress = TRUE;
				PlatformReleaseSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
				break;
			}
		}
	}

	if(Adapter->bDriverStopped || Adapter->bDriverIsGoingToPnpSetPowerSleep)
	{

		if(!ProtectOrNot)
		{
			PlatformAcquireSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
			pMgntInfo->RFChangeInProgress = FALSE;
			PlatformReleaseSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
		}
		return FALSE;
	}
	
	Mgnt_SetLedForRfState(Adapter, StateToSet, ChangeSource);
	
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));	


	switch(StateToSet) 
	{
		case eRfOn:
			//
			// Turn On RF no matter the IPS setting because we need to update the RF state to Ndis under Vista, or
			// the Windows does not allow the driver to perform site survey any more. By Bruce, 2007-10-02. 
			//

			pMgntInfo->RfOffReason &= (~ChangeSource);

			RT_TRACE(COMP_RF, DBG_LOUD, ("MgntActSet_RF_State - eRfon RfOffReason= 0x%x, ChangeSource=0x%X\n", pMgntInfo->RfOffReason, ChangeSource));
			if(! pMgntInfo->RfOffReason)
			{
				pMgntInfo->RfOffReason = 0;
				
				if(rtState != eRfOn)
				{
					bActionAllowed = TRUE;

					Adapter->pPortCommonInfo->WdiData.bRFOffSwitchProgress = TRUE;
				}

				if(rtState == eRfOff && ChangeSource >=RF_CHANGE_BY_HW && !Adapter->bInHctTest)
				{
					bConnectBySSID = TRUE;
				}
			}
			else
				RT_TRACE(COMP_RF, DBG_LOUD, ("MgntActSet_RF_State - eRfon reject pMgntInfo->RfOffReason= 0x%x, ChangeSource=0x%X\n", pMgntInfo->RfOffReason, ChangeSource));

			break;

		case eRfOff:
			//Move the following assignment to later in order to carry out
			//Let HW Radio off has little time
			//by sherry 20091225

			Adapter->pPortCommonInfo->WdiData.bRFOffSwitchProgress = TRUE;

			//pMgntInfo->RfOffReason |= ChangeSource;
			//bActionAllowed = TRUE;
			tempAdapter = GetDefaultAdapter(Adapter);

			while(tempAdapter !=NULL)
			{
				if(!ACTING_AS_AP(tempAdapter)) // 070125, rcnjko: we always keep connected in AP mode.
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_RF_State - is not AP mode.\n"));
					//Porting from 8xb, by Maddest 20090921
					if ( (ChangeSource> RF_CHANGE_BY_PS) && 
						!(pMgntInfo->RfOffReason & (RF_CHANGE_BY_SW|RF_CHANGE_BY_HW)) )
					{
						if((pMgntInfo->bMediaConnect || pMgntInfo->bIbssStarter)
							&& !(GET_TDLS_WIFITESTBED_RADIO_OFF(pMgntInfo))
							)
						{

							MgntDisconnect(tempAdapter, disas_lv_ss);
							//for Win 7 it must indicate Roaming_Start and set Roaming state 
							//By doing this, OS does not set RESET_REQUEST within 10 seconds which may 
							//interrupt auto-connect mechanism activated by driver
							//by sherry, 20091225
							//MgntLinkStatusSetRoamingState(tempAdapter, 0, RT_ROAMING_BY_DEAUTH, ROAMINGSTATE_SCANNING);
							//DrvIFIndicateRoamingStart(tempAdapter);
						}
			
						// Clear content of bssDesc[] and bssDesc4Query[] to avoid reporting old bss to UI. 
						// 2007.05.28, by shien chang.
						//PlatformZeroMemory( pMgntInfo->bssDesc, sizeof(RT_WLAN_BSS)*MAX_BSS_DESC );
						pMgntInfo->NumBssDesc = 0;
						pMgntInfo->NumBssDesc4Query = 0;
					RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: MgntActSet_RF_State(), clear NumBssDesc4Query\n"));

					}
			
					pMgntInfo->RfOffReason |= ChangeSource;
				}
				else
				{
					RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_RF_State - Disassociate all peers.\n"));
					pMgntInfo->RfOffReason |= ChangeSource;
					AP_DisassociateAllStation(tempAdapter, unspec_reason);
					MgntIndicateMediaStatus( tempAdapter, RT_MEDIA_DISCONNECT, FORCE_INDICATE );
				}
				
				tempAdapter = GetNextExtAdapter(tempAdapter);
			}
			bActionAllowed = TRUE;
			break;

		case eRfSleep:
			pMgntInfo->RfOffReason |= ChangeSource;
			bActionAllowed = TRUE;

			Adapter->pPortCommonInfo->WdiData.bRFOffSwitchProgress = TRUE;
			break;

		default:
			break;
	}


	//
	// <Roger_TODO> We should update RfOffReason for each port to keep consistency on multi-port architecture.
	// 2013.09.04.
	//
	tempAdapter = GetDefaultAdapter(Adapter);		
	while(tempAdapter !=NULL){
		tempAdapter->MgntInfo.RfOffReason = pMgntInfo->RfOffReason;
		tempAdapter = GetNextExtAdapter(tempAdapter);
	}

	if(bActionAllowed)
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("MgntActSet_RF_State(): Action is allowed.... StateToSet(%d), RfOffReason(%#X)\n", StateToSet, pMgntInfo->RfOffReason));

		// We should leave device power state D2 prior to the following pipe configurations, added by Roger, 2010.03.30.
		if((StateToSet == eRfOn) && (rtState ==eRfOff))			
		{
			Adapter->HalFunc.HalEnableTxHandler(Adapter);
			Adapter->HalFunc.HalEnableRxHandler(Adapter);
			Adapter->HalFunc.HalLeaveSSHandler(Adapter);	
		}		
				
                // Config HW to the specified mode.
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&StateToSet));
			
		//
		// Background :
		// When RF is turned to OFF by HW/SW, sometimes due to too much low rate retry packets in tx FIFO or 
		// a lot of RTS packets(no CTS rsp), it will spend a lot of time to tx all packets in Tx FIFO. 
		// Currently, after RF is turned off, there are possibility that HW FIFO not tx all the packets and then it's power is 
		// turned to OFF, these packets are just gone.
		// We must handle this here to return all the tx packets to upper layer windows to avoid BSOD 0x9f or some strange condition.
		// Solution: The following, when RF is turned off by HW/SW, we will return all the packets back to OS.
		// 
		if(StateToSet == eRfOff && rtState == eRfOn &&	bHwOrSwRfChange)
		{
			// If RF is from Hw/Sw ON => OFF
			MgntReturnTxResource(Adapter);
		}

		// Turn on RF.
		if(StateToSet == eRfOn) 
		{				
			if(rtState ==eRfOff)
			{
				Adapter->HalFunc.HalEnableTxHandler(Adapter);	
				Adapter->HalFunc.HalEnableRxHandler(Adapter);
				
				DrvIFIndicateCurrentPhyStatus(Adapter);
			}
		}
		// Turn off RF.
		else if(StateToSet == eRfOff)
		{		
			Adapter->HalFunc.HalDisableRxHandler(Adapter);
			Adapter->HalFunc.HalDisableTxHandler(Adapter);		
			
			//
			// <Roger_Notes> If RF OFF control is set by this routine, which means the power state 
			// our device currently in D0. So we shedule Idle IRP request immediately. 2009.07.03.
			// 
			RT_TRACE(COMP_RF, DBG_LOUD, ("MgntActSet_RF_State(): Turn off RF!!\n"));
			Adapter->HalFunc.HalEnterSSHandler(Adapter);

			// Prefast warning C28182 : Dereferencing NULL pointer. '((Adapter))->pPortCommonInfo->pDefaultAdapter' contains the same NULL value as 'tempAdapter' did.
			if(((Adapter))->pPortCommonInfo->pDefaultAdapter != NULL)
				CustomScan_TermReq(GET_CUSTOM_SCAN_INFO(Adapter), FALSE);

			RT_TRACE(COMP_SCAN, DBG_LOUD, ("Terminate scan because of radio off\n"));

			if(pMgntInfo->RfOffReason > RF_CHANGE_BY_PS) 
				DrvIFIndicateCurrentPhyStatus(Adapter);
		}		

		Adapter->pPortCommonInfo->WdiData.bRFOffSwitchProgress = FALSE;
	}
	else
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("MgntActSet_RF_State(): Action is rejected.... StateToSet(%d), ChangeSource(%#X), RfOffReason(%#X)\n", StateToSet, ChangeSource, pMgntInfo->RfOffReason));
	}

	// Release RF spinlock
	if(!ProtectOrNot)
	{
		PlatformAcquireSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
		pMgntInfo->RFChangeInProgress = FALSE;
		PlatformReleaseSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
	}
	
	RT_TRACE(COMP_POWER, DBG_LOUD, ("MgntActSet_RF_State() <====\n"));
	return bActionAllowed;
}

//
//	Description: 
//		Update current transmit power level.
//    TxPowerLevel Setting
//		0 => Default 1 => 100% 2 => 75% 3 => 50% 4 => 25%
//	By Marsyang, 2008-02-04.
//
VOID
MgntActSet_TX_POWER_LEVEL(
	PADAPTER		Adapter,
	int				TxPowerLevel
	)
{
	s4Byte	minPowerDBM  = 0;
	s4Byte	maxPowerDBM = 0;
	s4Byte	PowerAmount =0;	
	
	if(TxPowerLevel == 0)
	{
		return;
	}
		
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_MIN_TX_POWER_DBM, (pu1Byte)&minPowerDBM);
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_MAX_TX_POWER_DBM, (pu1Byte)&maxPowerDBM);

	RT_TRACE(COMP_RM, DBG_LOUD, ("MgntActSet_TX_POWER_LEVEL(): Power Range from Max TX Power Dbm(%d) To Min Dbm(%d)\n",maxPowerDBM,minPowerDBM ));
	PowerAmount = ((maxPowerDBM - minPowerDBM)/6)*(TxPowerLevel+1);

	MgntActSet_TX_POWER_DBM(Adapter,(maxPowerDBM-PowerAmount));
	RT_TRACE(COMP_RM, DBG_LOUD, ("MgntActSet_TX_POWER_LEVEL(): Set TX Power(TX Power Level: %d) to Dbm(%d)\n",TxPowerLevel,(maxPowerDBM-PowerAmount)));	
	
}

//
//	Description: 
//		Update current transmit power level in dBm.
//	By Bruce, 2008-02-04.
//
BOOLEAN
MgntActSet_TX_POWER_DBM(
	PADAPTER		Adapter,
	s4Byte			powerInDbm
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	
	PlatformAcquireSpinLock(Adapter, RT_RM_SPINLOCK);
	if( !pMgntInfo->bUpdateTxPowerInProgress )
	{
		RT_TRACE(COMP_RM, DBG_LOUD, ("MgntActSet_TX_POWER_DBM(): powerInDbm(%d)\n", powerInDbm));

		pMgntInfo->bUpdateTxPowerInProgress = TRUE;
		pMgntInfo->PowerToUpdateInDbm = powerInDbm;
		PlatformReleaseSpinLock(Adapter, RT_RM_SPINLOCK);

		PlatformScheduleWorkItem(&(pMgntInfo->UpdateTxPowerWorkItem));

		return TRUE;
	}
	else
	{
		RT_TRACE(COMP_RM, DBG_LOUD, ("MgntActSet_TX_POWER_DBM(): powerInDbm(%d), UpdateTxPowerWorkItem is in progress!!!\n", powerInDbm));

		PlatformReleaseSpinLock(Adapter, RT_RM_SPINLOCK);

		return FALSE;
	}
}

//
//	Description: 
//		callback function of UpdateTxPowerWorkItem. 
//	By Bruce, 2008-02-04.
//
VOID
UpdateTxPowerDbmWorkItemCallback(
	IN PVOID		pContext
	)
{
	PADAPTER		pAdapter = (PADAPTER)pContext;
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	if(pAdapter->bDriverStopped)
		return;

	pAdapter->HalFunc.UpdateTxPowerDbmHandler(pAdapter, pMgntInfo->PowerToUpdateInDbm);

	PlatformAcquireSpinLock(pAdapter, RT_RM_SPINLOCK);
	pMgntInfo->bUpdateTxPowerInProgress = FALSE;
	PlatformReleaseSpinLock(pAdapter, RT_RM_SPINLOCK);
}

//
//	Description:
//		Change additioinal beacon IE.
//
RT_STATUS
MgntActSet_AdditionalBeaconIE(
	IN 	PADAPTER	pAdapter,
	IN 	pu1Byte 		pAdditionalIEBuf,
	IN  	u4Byte		AdditionalIEBufLen
	)
{
	PMGNT_INFO pMgntInfo = &pAdapter->MgntInfo;
	RT_STATUS Status = RT_STATUS_SUCCESS;
	PVOID newBeaconIeData = NULL;
	u4Byte newBeaconIeSize = 0;

	do
	{
		// 
		// allocate memory for the new IE data and copy the IEs
		//
		if (AdditionalIEBufLen > 0)
		{
			Status = PlatformAllocateMemory(pAdapter,
						&newBeaconIeData,
						AdditionalIEBufLen);
			if (Status != RT_STATUS_SUCCESS)
			{
				break;
			}		

			PlatformMoveMemory(newBeaconIeData, 
								pAdditionalIEBuf, 
								AdditionalIEBufLen);
			newBeaconIeSize = AdditionalIEBufLen;
		}
		else
		{// the IE is cleared.
		}


		// 
		// free the old IEs
		//
		if (pMgntInfo->AdditionalBeaconIESize > 0)
		{
			PlatformFreeMemory(pMgntInfo->AdditionalBeaconIEData, pMgntInfo->AdditionalBeaconIESize);
			pMgntInfo->AdditionalBeaconIEData = NULL;
			pMgntInfo->AdditionalBeaconIESize = 0;
		}

		// 
		// cache new IEs. 
		// Note that if AdditionalIEBufLen == 0, pMgntInfo->AdditionalBeaconIEData is cleared.
		//
		pMgntInfo->AdditionalBeaconIEData = newBeaconIeData;
		pMgntInfo->AdditionalBeaconIESize = (u2Byte)newBeaconIeSize;
		newBeaconIeData = NULL;
	} while(FALSE);

	return Status;
}


//merge from branch 1021 for WHCK Scan_AddtionalIE
RT_STATUS
MgntActSet_AdditionalProbeReqIE(
	IN 	PADAPTER	pAdapter,
	IN 	pu1Byte 		pAdditionalIEBuf,
	IN  	u4Byte		AdditionalIEBufLen
	)
{
	PMGNT_INFO pMgntInfo = &pAdapter->MgntInfo;
	RT_STATUS Status = RT_STATUS_SUCCESS;
	PVOID newIeData = NULL;
	u4Byte newIeSize = 0;
	
	do
	{
		// 
		// allocate memory for the new IE data and copy the IEs
		//
		if (AdditionalIEBufLen > 0)
		{
			Status = PlatformAllocateMemory(pAdapter,
						&newIeData,
						AdditionalIEBufLen);
			if (Status != RT_STATUS_SUCCESS)
			{
				break;
			}		

			PlatformMoveMemory(newIeData, 
								pAdditionalIEBuf, 
								AdditionalIEBufLen);
			newIeSize = AdditionalIEBufLen;
		}
		else
		{// the IE is cleared.
		}


		// 
		// free the old IEs
		//
		if (pMgntInfo->AdditionalProbeReqIESize > 0)
		{
			PlatformFreeMemory(pMgntInfo->AdditionalProbeReqIEData, pMgntInfo->AdditionalProbeReqIESize);
			pMgntInfo->AdditionalProbeReqIEData = NULL;
			pMgntInfo->AdditionalProbeReqIESize = 0;
		}

		// 
		// cache new IEs. 
		// Note that if AdditionalIEBufLen == 0, pMgntInfo->AdditionalBeaconIEData is cleared.
		//
		pMgntInfo->AdditionalProbeReqIEData = newIeData;
		pMgntInfo->AdditionalProbeReqIESize = (u2Byte)newIeSize;
		newIeData = NULL;
	} while(FALSE);

	return Status;
}



//
//	Description:
//		Change additioinal probe rsp IE.
//
RT_STATUS
MgntActSet_AdditionalProbeRspIE(
	IN 	PADAPTER	pAdapter,
	IN 	pu1Byte 		pAdditionalIEBuf,
	IN  	u4Byte		AdditionalIEBufLen
	)
{
	PMGNT_INFO pMgntInfo = &pAdapter->MgntInfo;
	RT_STATUS Status = RT_STATUS_SUCCESS;
	PVOID newProbeRspIeData = NULL;
	u4Byte newProbeRspIeSize = 0;

	do
	{
		// 
		// allocate memory for the new IE data and copy the IEs
		//
		if (AdditionalIEBufLen > 0)
		{	
			Status = PlatformAllocateMemory(pAdapter,
						&newProbeRspIeData,
						AdditionalIEBufLen);
			if (Status != RT_STATUS_SUCCESS)
			{
				break;
			}		

			PlatformMoveMemory(newProbeRspIeData, 
								pAdditionalIEBuf, 
								AdditionalIEBufLen);
			newProbeRspIeSize = AdditionalIEBufLen;
		}
		else
		{// the IE is cleared.
		}


		// 
		// free the old IEs
		//
		if (pMgntInfo->AdditionalResponseIESize > 0)
		{
			PlatformFreeMemory(pMgntInfo->AdditionalResponseIEData, pMgntInfo->AdditionalResponseIESize);
			pMgntInfo->AdditionalResponseIEData = NULL;
			pMgntInfo->AdditionalResponseIESize = 0;
		}

		// 
		// cache new IEs
		//
		pMgntInfo->AdditionalResponseIEData = newProbeRspIeData;
		pMgntInfo->AdditionalResponseIESize = (u2Byte)newProbeRspIeSize;
		newProbeRspIeData = NULL;
	} while(FALSE);

	return Status;
}

RT_STATUS
MgntActSet_AdditionalAssocReqIE(
	IN 	PADAPTER	pAdapter,
	IN 	pu1Byte 		pAdditionalIEBuf,
	IN  	u4Byte		AdditionalIEBufLen
	)
{
	PMGNT_INFO pMgntInfo = &pAdapter->MgntInfo;
	RT_STATUS Status = RT_STATUS_SUCCESS;
	PVOID newIeData = NULL;
	u4Byte newIeSize = 0;
	
	do
	{
		// 
		// allocate memory for the new IE data and copy the IEs
		//
		if (AdditionalIEBufLen > 0)
		{	
			Status = PlatformAllocateMemory(pAdapter,
						&newIeData,
						AdditionalIEBufLen);
			if (Status != RT_STATUS_SUCCESS)
			{
				break;
			}		

			PlatformMoveMemory(newIeData, 
								pAdditionalIEBuf, 
								AdditionalIEBufLen);
			newIeSize = AdditionalIEBufLen;
		}
		else
		{// the IE is cleared.
		}


		// 
		// free the old IEs
		//
		if (pMgntInfo->AdditionalAssocReqIESize > 0)
		{
			PlatformFreeMemory(pMgntInfo->AdditionalAssocReqIEData, pMgntInfo->AdditionalAssocReqIESize);
			pMgntInfo->AdditionalAssocReqIEData = NULL;
			pMgntInfo->AdditionalAssocReqIESize = 0;
		}

		// 
		// cache new IEs. 
		// Note that if AdditionalIEBufLen == 0, pMgntInfo->AdditionalBeaconIEData is cleared.
		//
		pMgntInfo->AdditionalAssocReqIEData = newIeData;
		pMgntInfo->AdditionalAssocReqIESize = (u2Byte)newIeSize;
		newIeData = NULL;
	} while(FALSE);

	return Status;
}

RT_NDIS_VERSION
MgntTranslateNdisVersionToRtNdisVersion(
	IN UINT NdisVersion
	)
{
	RT_NDIS_VERSION ret = RT_NDIS_VERSION_NONE_NDIS;
	
	if(NdisVersion == 0)
	{
		ret = RT_NDIS_VERSION_NONE_NDIS;
	}
	else if(NdisVersion <= NDIS_VERSION_BASE_5_0)
	{
		ret = RT_NDIS_VERSION_5_0;
	}
	else if(NdisVersion <= NDIS_VERSION_BASE_5_2)
	{
		ret = RT_NDIS_VERSION_5_1;
	}
	else if(NdisVersion <= NDIS_VERSION_BASE_6_0)
	{
		ret = RT_NDIS_VERSION_6_0;
	}
	else if(NdisVersion <= NDIS_VERSION_BASE_6_1)
	{
		ret = RT_NDIS_VERSION_6_1;
	}
	else if(NdisVersion <= NDIS_VERSION_BASE_6_20)
	{
		ret = RT_NDIS_VERSION_6_20;
	}
	else if(NdisVersion <= NDIS_VERSION_BASE_6_30)
	{
		ret = RT_NDIS_VERSION_6_30;
	}
	else if(NdisVersion <= NDIS_VERSION_BASE_6_40)
	{		
		ret = RT_NDIS_VERSION_6_40;
	}
	else if(NdisVersion <= NDIS_VERSION_BASE_6_50)
	{		
		ret = RT_NDIS_VERSION_6_50;
	}
	else
	{// should not get here

		RT_ASSERT(0, ("Wrong NdisVersion: 0x%X\n", NdisVersion));
		
		ret = RT_NDIS_VERSION_NONE_NDIS;
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, 
		("MgntTranslateNdisVersionToRtNdisVersion(): %X => %X\n", NdisVersion, ret));
	
	return ret;
}



VOID
MgntActSet_ApType(
	IN PADAPTER pAdapter,
	IN BOOLEAN mActingAsAp
	)
{
	if(mActingAsAp)
	{
		pAdapter->MgntInfo.ApType = AP_DetermineApType(pAdapter);
	}
	else
	{
		pAdapter->MgntInfo.ApType = RT_AP_TYPE_NONE;
	}
	RT_TRACE(COMP_AP, DBG_LOUD, ("MgntActSet_ApType(): %u\n", pAdapter->MgntInfo.ApType));
}

//
// Description:
//	Set WMM enable or disable.
// Argument:
//	Adapter -
//		NIC adapter context pointer.
//	bWMMEnable -
//		Enable(True) or Disable(FALSE) WMM.
// By Bruce, 2008-05-28.
//
VOID
MgntActSet_802_11_WMM_MODE(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			bWMMEnable
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING	tmpSsid;
	u1Byte			tmpSsidBuf[32];
	RT_JOIN_ACTION	join_action;
	BOOLEAN			bReconnect = FALSE;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("MgntActSet_802_11_WMM_MODE(): WMM is %s\n", (bWMMEnable ? "Enabled" : "Disabled")));

	// Backup SSID.
	tmpSsid.Octet = tmpSsidBuf;
	tmpSsid.Length = sizeof(tmpSsidBuf);
	CopySsid(tmpSsid.Octet, tmpSsid.Length, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);

	if(pMgntInfo->bMediaConnect && pMgntInfo->mAssoc && !ACTING_AS_AP(Adapter))
		bReconnect = TRUE;

	// Disconnect before change WMM capability. 2007.01.15, by shien chang.
	MgntActSet_802_11_DISASSOCIATE(Adapter, disas_lv_ss);

	// Restore SSID.
	CopySsid(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, tmpSsid.Octet, tmpSsid.Length);
	
	if (bWMMEnable)
	{
		pMgntInfo->pStaQos->QosCapability |= QOS_WMM;
	}
	else
	{
		pMgntInfo->pStaQos->QosCapability &= ~QOS_WMM;
	}

	if (ACTING_AS_AP(Adapter))
	{
		AP_Reset(Adapter);
	}
	else if(bReconnect)
	{
		PRT_WLAN_BSS	pRtBss;

		if(PlatformAllocateMemory(Adapter, (PVOID*)&pRtBss, sizeof(RT_WLAN_BSS)) != RT_STATUS_SUCCESS)
			return;
		
		join_action = SelectNetworkBySSID(Adapter, &pMgntInfo->Ssid, FALSE, pRtBss);
		JoinRequest(Adapter, join_action, pRtBss);
		
		PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
	}
}

//
// Description:
//	Set WMM UPSD.
// Argument:
//	Adapter -
//		NIC adapter context pointer.
//	WMM_UAPSD -
//		Enable / Disable UPSD for each queue.
// By Bruce, 2008-05-28.
//
VOID
MgntActSet_802_11_WMM_UAPSD(
	IN	PADAPTER		Adapter,
	IN	AC_UAPSD		WMM_UAPSD
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING	tmpSsid;
	u1Byte			tmpSsidBuf[32];
	RT_JOIN_ACTION	join_action;
	BOOLEAN			bReconnect = FALSE;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("MgntActSet_802_11_WMM_UAPSD(): UPSD is %X\n", WMM_UAPSD));
	
	// Backup SSID.
	tmpSsid.Octet = tmpSsidBuf;
	tmpSsid.Length = sizeof(tmpSsidBuf);
	CopySsid(tmpSsid.Octet, tmpSsid.Length, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);

	if(pMgntInfo->bMediaConnect && pMgntInfo->mAssoc && !ACTING_AS_AP(Adapter))
		bReconnect = TRUE;

	// Disconnect before change WMM capability. 2007.01.15, by shien chang.
	MgntDisconnect(Adapter, disas_lv_ss);

	// Restore SSID.
	CopySsid(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, tmpSsid.Octet, tmpSsid.Length);
	
	pMgntInfo->pStaQos->b4ac_Uapsd = WMM_UAPSD & 0x0F;
	if(pMgntInfo->pStaQos->b4ac_Uapsd & 0x0F)
	{
		pMgntInfo->pStaQos->QosCapability |= QOS_WMM_UAPSD;
	}
	else
	{
		pMgntInfo->pStaQos->QosCapability &= ~QOS_WMM_UAPSD;
	}
	
	// Use MSB 4 BIT for MaxSPLength
	pMgntInfo->pStaQos->MaxSPLength = WMM_UAPSD>>4;

	if(bReconnect)
	{
		PRT_WLAN_BSS		pRtBss;

		if(PlatformAllocateMemory(Adapter, (PVOID*)&pRtBss, sizeof(RT_WLAN_BSS))  != RT_STATUS_SUCCESS)
			return;
		
		join_action = SelectNetworkBySSID(Adapter, &pMgntInfo->Ssid, FALSE, pRtBss);
		JoinRequest(Adapter, join_action, pRtBss);

		PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
	}
}

BOOLEAN
MgntActSet_802_11_CustomizedAsocIE(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u1Byte			InformationBufferLength
	)
{
#if (WPS_SUPPORT == 1)
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	u2Byte		AsocIeLength = *((pu2Byte)(InformationBuffer));

	//
	// Minimum IE length should be larger than zero,
	// Maximum IE length should be less/equal than 0xFF + IE ID + length (0xFF + 2)
	//	
	
	pMgntInfo->bCustomizedAsocIE = FALSE;

	if(AsocIeLength > 0 && AsocIeLength <= (MAX_IE_LEN+2))
	{
		
		pMgntInfo->bCustomizedAsocIE = TRUE;
		pMgntInfo->CustomizedAsocIELength = AsocIeLength;
		PlatformZeroMemory(pMgntInfo->CustomizedAsocIEBuf, sizeof(pMgntInfo->CustomizedAsocIEBuf));
		PlatformMoveMemory(pMgntInfo->CustomizedAsocIEBuf, (pu1Byte)InformationBuffer+2, pMgntInfo->CustomizedAsocIELength);
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntActSet_802_11_CustomizedAsocIE(): length = %d\n", pMgntInfo->CustomizedAsocIELength));
		RT_PRINT_DATA(COMP_MLME, DBG_TRACE, ("MgntActSet_802_11_CustomizedAsocIE():"), pMgntInfo->CustomizedAsocIEBuf, pMgntInfo->CustomizedAsocIELength);

		return TRUE;
	}	
#endif
	return FALSE;
}


BOOLEAN
MgntActSet_802_11_Sigma_Capability(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u1Byte			InformationBufferLength
	)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	HT_CAP_OP_CODE			capability = (HT_CAP_OP_CODE)(*((pu1Byte)InformationBuffer));
	u4Byte					neededLen = 1;
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;

	if(InformationBufferLength < neededLen)
		goto end;
		

	switch(capability)
	{
		case HT_CAP_OP_40_INTOLERANT:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			
			pHTInfo->b40Intolerant = (BOOLEAN)(*((pu1Byte)InformationBuffer+1));
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set 40 intolerant = %d\n", pHTInfo->b40Intolerant));
		}
		break;

		case HT_CAP_OP_AMPDU:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			
			pHTInfo->bAMPDUManual = (BOOLEAN)(*((pu1Byte)InformationBuffer+1));
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set AMPDU (manual) = %d\n", pHTInfo->bAMPDUManual));
		}
		break;

		case HT_CAP_OP_STBC:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set HT_CAP_OP_STBC\n"));
			if(RT_STATUS_SUCCESS != 
				(rtStatus = MgntActSet_802_11_STBC_MODE(Adapter, ((pu1Byte)InformationBuffer+1), InformationBufferLength - 1)))
			{
				RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] MgntActSet_802_11_STBC_MODE failed = %d\n", rtStatus));
				return FALSE;
			}
		}
		break;

		case HT_CAP_OP_WIDTH:
		{
			PRT_CHANNEL_INFO	pChnlInfo = pMgntInfo->pChannelInfo;

			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			
			if((CHANNEL_WIDTH)(*((pu1Byte)InformationBuffer+1)) == CHANNEL_WIDTH_20)
				pChnlInfo->RegBWSetting = CHANNEL_WIDTH_20;
			else if((CHANNEL_WIDTH)(*((pu1Byte)InformationBuffer+1)) == CHANNEL_WIDTH_40)
				pChnlInfo->RegBWSetting = CHANNEL_WIDTH_40;
			else if((CHANNEL_WIDTH)(*((pu1Byte)InformationBuffer+1)) == CHANNEL_WIDTH_80)
				pChnlInfo->RegBWSetting = CHANNEL_WIDTH_80;

			RT_TRACE(COMP_HT, DBG_LOUD, ("Set Width = %s\n", (pChnlInfo->RegBWSetting==2)?"80MHz":(pChnlInfo->RegBWSetting==1)?"40MHz":"20MHz"));
		}
		break;

		case HT_CAP_OP_SEND_ADDBA:
		{
			PTX_TS_RECORD	pTxTs = NULL;
			u1Byte			tid = (*((pu1Byte)InformationBuffer+1));
			
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			
			if(GetTs(Adapter, (PTS_COMMON_INFO*)(&pTxTs), pMgntInfo->Bssid, tid, TX_DIR, TRUE))
			{
				TsStartAddBaProcess(Adapter, pTxTs);
				RT_TRACE(COMP_HT, DBG_LOUD, ("Send addba request to associated AP with TID(%d)\n", tid));
			}
			else
				RT_TRACE(COMP_HT, DBG_LOUD, ("Fail to setup ts with TID(%d)\n", tid));
		}
		break;

		case HT_CAP_OP_NOACK:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			
			pMgntInfo->pStaQos->AcNoAck = (*(PAC_NOACK)((pu1Byte)InformationBuffer+1));
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set NoAckPolicy = %x\n", pMgntInfo->pStaQos->AcNoAck));
		}
		break;

		case HT_CAP_OP_VHT_TXSP_STREAM:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;

			pVHTInfo->nTxSPStream = (*((pu1Byte)InformationBuffer+1));
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set VHT TxSPStream = %d\n", pVHTInfo->nTxSPStream));
		}
		break;

		case HT_CAP_OP_VHT_RXSP_STREAM:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			
			pVHTInfo->nRxSPStream = (*((pu1Byte)InformationBuffer+1));
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set VHT RxSPStream = %d\n", pVHTInfo->nRxSPStream));
		}
		break;

		case HT_CAP_OP_HT_TXSP_STREAM:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			
			pHTInfo->nTxSPStream = (*((pu1Byte)InformationBuffer+1));
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set TxSPStream = %d\n", pHTInfo->nTxSPStream));
		}
		break;

		case HT_CAP_OP_HT_RXSP_STREAM:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			
			pHTInfo->nRxSPStream = (*((pu1Byte)InformationBuffer+1));
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set RxSPStream = %d\n", pHTInfo->nRxSPStream));
		}
		break;

		case HT_CAP_OP_OPMODE_NOTIF:
		{
			neededLen += 2;
			if(InformationBufferLength < neededLen)
				goto end;
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set HT_CAP_OP_OPMODE_NOTIF\n"));
			pVHTInfo->BWToSwitch = (CHANNEL_WIDTH)(*((pu1Byte)InformationBuffer+1));

			DbgPrint("set value = %d\n", pVHTInfo->BWToSwitch);
			
			pVHTInfo->RxSSToSwitch = (*((pu1Byte)InformationBuffer+2));
			VHTSendOperatingModeNotification(Adapter, pMgntInfo->Bssid);
			CHNL_SetBwChnl(	Adapter, 
								pMgntInfo->dot11CurrentChannelNumber,
								pVHTInfo->BWToSwitch, 
								pMgntInfo->pChannelInfo->Ext20MHzChnlOffsetOf40MHz);
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set Bw to %d\n", pVHTInfo->BWToSwitch));
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set RxSS to %d\n", pVHTInfo->RxSSToSwitch));
		}
		break;

		case HT_CAP_OP_OPT_MD_NOTIF_IE:
		{
			neededLen += 2;
			if(InformationBufferLength < neededLen)
				goto end;
			
			pVHTInfo->RxSSToSwitch = (*((pu1Byte)InformationBuffer+1));
			if(pVHTInfo->RxSSToSwitch == 0)
			{
				pVHTInfo->bOpModeNotif = FALSE;
			}
			else
			{
				pVHTInfo->bOpModeNotif = TRUE;
				pVHTInfo->BWToSwitch = (CHANNEL_WIDTH)(*((pu1Byte)InformationBuffer+2));

				DbgPrint("set value = %d 222\n", pVHTInfo->BWToSwitch);
				RT_TRACE(COMP_HT, DBG_LOUD, ("Set RxSS to %d at asoc\n", pVHTInfo->RxSSToSwitch));
				RT_TRACE(COMP_HT, DBG_LOUD, ("Set Bw to %d at asoc\n", pVHTInfo->BWToSwitch));
			}
		}
		break;

		case HT_CAP_OP_TXBF:
		case HT_CAP_OP_BEAMFORMING_CAP:
			{
				neededLen += 1;
				if(InformationBufferLength < neededLen)
					goto end;
				RT_TRACE(COMP_HT, DBG_LOUD, ("Set HT_CAP_OP_BEAMFORMING_CAP\n"));
				if(RT_STATUS_SUCCESS != 
					(rtStatus = MgntActSet_802_11_Beamforming_MODE(Adapter, ((pu1Byte)InformationBuffer+1), InformationBufferLength - 1)))
				{
					RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] MgntActSet_802_11_Beamforming_MODE failed = %d\n", rtStatus));
					return FALSE;
				}
			}
			break;

		case HT_CAP_OP_SGI80:
			{
				neededLen += 1;
				if(InformationBufferLength < neededLen)
					goto end;
				pVHTInfo->bRegShortGI80MHz = (BOOLEAN)(*((pu1Byte)InformationBuffer + 1));
				RT_TRACE(COMP_HT, DBG_LOUD, ("Set HT_CAP_OP_SGI80 = %d\n", pVHTInfo->bRegShortGI80MHz));
			}
			break;

		break;

		case HT_CAP_OP_BW_SIGNALING_CTRL:
		{
			u1Byte	BWSignalingEnabled = (u1Byte)(*((pu1Byte)InformationBuffer+1));

			if(BWSGNL_Enable == BWSignalingEnabled)
			{
				pVHTInfo->BWSignalingCtrl = (*((pu1Byte)InformationBuffer+2));
				pMgntInfo->ForcedProtectionMode = PROTECTION_MODE_FORCE_ENABLE;
				pMgntInfo->ForcedProtectRate = MGN_24M;
				pMgntInfo->ForcedProtectCCA = pVHTInfo->BWSignalingCtrl;
				pMgntInfo->bForcedProtectRTSFrame = TRUE;
				pMgntInfo->bForcedProtectCTSFrame = FALSE;
				RT_TRACE(COMP_HT, DBG_LOUD, ("Set RTS BW signaling = %d\n", pVHTInfo->BWSignalingCtrl));
			}
			else if(BWSGNL_Disable == BWSignalingEnabled)
			{
				pMgntInfo->ForcedProtectionMode = PROTECTION_MODE_FORCE_DISABLE;
				RT_TRACE(COMP_HT, DBG_LOUD, ("Set RTS protection disabled\n"));
			}
			else	//BWSGNL_Auto
			{
				pMgntInfo->ForcedProtectionMode = PROTECTION_MODE_AUTO;
				RT_TRACE(COMP_HT, DBG_LOUD, ("Set RTS protection to auto\n"));
			}
		}
		break;

		case HT_CAP_OP_NSS_MCS_CAP:
		{			
			neededLen += 2;
			if(InformationBufferLength < neededLen)
				goto end;

			pVHTInfo->bUpdateMcsCap = (BOOLEAN)(*((pu1Byte)InformationBuffer + 1));
			if(pVHTInfo->bUpdateMcsCap == 0)
				pVHTInfo->McsCapability = 3;
			else
				pVHTInfo->McsCapability = (u1Byte)(*((pu1Byte)InformationBuffer+2));
		}
		break;

		case HT_CAP_OP_TX_LGI_RATE:
		{
			neededLen += 3;
			if(InformationBufferLength < neededLen)
				goto end;

			pVHTInfo->bAssignTxLGIRate = (BOOLEAN)(*((pu1Byte)InformationBuffer + 1));
			if(pVHTInfo->bAssignTxLGIRate)
				pVHTInfo->highestTxLGIRate = (u2Byte)(*((pu2Byte)((pu1Byte)InformationBuffer+2)));
		}
		break;

		case HT_CAP_OP_SGI20:
		{
			u1Byte	shortGI = 0;
			
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;
			shortGI = (u1Byte)(*((pu1Byte)InformationBuffer + 1));
			pHTInfo->bRegShortGI20MHz = (shortGI & BIT0) ? TRUE : FALSE;
			pHTInfo->bRegShortGI40MHz = (shortGI & BIT1) ? TRUE : FALSE;
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set HT_CAP_OP_SGI20 = %d\n", shortGI));
		}
		break;

		case HT_CAP_OP_VHT_TKIP:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;

			if(*((pu1Byte)InformationBuffer + 1) == 1)
			{
				pMgntInfo->VhtWeakSecurity |= BIT0;
			}
			else
			{
				pMgntInfo->VhtWeakSecurity &= ~BIT0;
			}
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set VHT weak security [TKIP] = %d\n", pMgntInfo->VhtWeakSecurity));
		}
		break;

		case HT_CAP_OP_VHT_WEP:
		{
			neededLen += 1;
			if(InformationBufferLength < neededLen)
				goto end;

			if(*((pu1Byte)InformationBuffer + 1) == 1)
			{
				pMgntInfo->VhtWeakSecurity |= BIT1;
			}
			else
			{
				pMgntInfo->VhtWeakSecurity &= ~BIT1;
			}
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set VHT weak security [WEP] = %d\n", pMgntInfo->VhtWeakSecurity));
		}
		break;

		case HT_CAP_OP_CTS_ENABLE:
		{			
			PRT_CHANNEL_INFO	pChnlInfo = pMgntInfo->pChannelInfo;
			u1Byte				BWFallBackLvl = 0;
			
			pVHTInfo->DynamicCTSBW = (CHANNEL_WIDTH)((u1Byte)(*((pu1Byte)InformationBuffer+1)));

			if(pChnlInfo->RegBWSetting == CHANNEL_WIDTH_80) // If current BW is 80MHz
			{
				if(pVHTInfo->DynamicCTSBW == CHANNEL_WIDTH_40)
					BWFallBackLvl = 1;
				else if(pVHTInfo->DynamicCTSBW == CHANNEL_WIDTH_20)
					BWFallBackLvl = 2;
			}
			else if(pChnlInfo->RegBWSetting == CHANNEL_WIDTH_40)
			{
				if(pVHTInfo->DynamicCTSBW == CHANNEL_WIDTH_20)
					BWFallBackLvl = 1;
			}
			else if(pChnlInfo->RegBWSetting == CHANNEL_WIDTH_20)
			{
				BWFallBackLvl = 0;
			}
			else
			{
				RT_TRACE(COMP_HT, DBG_LOUD, ("Set HT_CAP_OP_CTS_ENABLE: Not support RegBWSetting=%d,\n", pVHTInfo->DynamicCTSBW));
			}
			
			Adapter->HalFunc.HalSetCTSDynamicBWSelectHandler(Adapter, BWFallBackLvl);
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set dynamic CTS BW = %d, RegBWSetting=%d, BWFallBackLvl(%d)\n", pVHTInfo->DynamicCTSBW, pChnlInfo->RegBWSetting, BWFallBackLvl));
		}
		break;

		#if 0
		case HT_SIGMA_11N_CAP_RTS_DYNAMIC_BW:
		{
			pVHTInfo->bFixedRTSBW = (BOOLEAN)(*((pu1Byte)InformationBuffer+1));
			if(pVHTInfo->bFixedRTSBW)
				pVHTInfo->DynamicRTSBW = (*((pu1Byte)InformationBuffer+2));
			// hpfan_todo: need fix rate for bw setting
			RT_TRACE(COMP_HT, DBG_LOUD, ("Set dynamic RTS bw = %d\n", pVHTInfo->DynamicRTSBW));
		}
		break;
		#endif
		
		default:
			break;
	}

end:
	if(InformationBufferLength < neededLen)
	{
		RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] Not enough buffer length (%d) for needed (%d)\n", InformationBufferLength, neededLen));
		return FALSE;
}

	return TRUE;
}

//
// Description:
//	Set 802.11 Beanforming capability mode for VHT.
// Arguments:
//	[in] Adapter -
//		NIC adapter context pointer.
//	[in] InformationBuffer -
//		The content location
//	[in] InformationBufferLength -
//		The length in byte of InformationBuffer.
// Remark:
//	Information Buffer contains one byte as the following setting.
// 	BIT0 - BEAMFORMING_VHT_BEAMFORMER_ENABLE, 
//	BIT1 - BEAMFORMING_VHT_BEAMFORMEE_ENABLE
//	BIT2 - BEAMFORMING_VHT_BEAMFORMER_TEST		(Transmiting Beamforming no matter the target supports it or not)
//	BIT3 - BEAMFORMING_VHT_MU_BEAMFORMER_ENABLE
// 	BIT4 - BEAMFORMING_HT_BEAMFORMER_ENABLE, 
//	BIT5 - BEAMFORMING_HT_BEAMFORMEE_ENABLE
//	BIT6 - BEAMFORMING_HT_BEAMFORMER_TEST		(Transmiting Beamforming no matter the target supports it or not)
//	BIT7 - BEAMFORMING_VHT_MU_BEAMFORMEE_ENABLE
// By Bruce, 2012-08-28.
//
RT_STATUS
MgntActSet_802_11_Beamforming_MODE(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength
	)
{
	PADAPTER					tmpAdapter = GetDefaultAdapter(pAdapter);
	PRT_HIGH_THROUGHPUT			pTmpHTInfo = NULL;
	PRT_VERY_HIGH_THROUGHPUT	pTmpVHTInfo = NULL;
	u1Byte						userBeamformSet = 0;
	BOOLEAN						bHwSupportBeamformer = FALSE, bHwSupportBeamformee = FALSE;
	BOOLEAN						bHwSupportMUBeamformer = FALSE, bHwSupportMUBeamformee = FALSE;

	if(InformationBufferLength < sizeof(u1Byte))
	{
		RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] Invalid buffer length (%d)\n", InformationBufferLength));
		return RT_STATUS_INVALID_PARAMETER;
	}

	userBeamformSet = *(InformationBuffer);

	RT_TRACE_F(COMP_HT, DBG_LOUD, ("Beamforming config = 0x%02X\n", userBeamformSet));
	
	// Set for all adapters
	do
	{
		pTmpVHTInfo = GET_VHT_INFO(&(tmpAdapter->MgntInfo));

		CLEAR_FLAGS(pTmpVHTInfo->VhtBeamformCap);
		if(userBeamformSet & BIT0)
		{
			if(bHwSupportBeamformer)
			{
				SET_FLAG(pTmpVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_BEAMFORMER_ENABLE);
			}
			else
			{
				RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] HW doesn't support BEAMFORMER! Skip setting...\n"));
			}
		}
		if(userBeamformSet & BIT1)
		{
			if(bHwSupportBeamformee)
			{
				SET_FLAG(pTmpVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_BEAMFORMEE_ENABLE);
			}
			else
			{
				RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] HW doesn't support BEAMFORMEE! Skip setting...\n"));
			}
		}
		if(userBeamformSet & BIT2)
		{
			if(bHwSupportBeamformer)
			{
				SET_FLAG(pTmpVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_BEAMFORMER_TEST);
			}
			else
			{
				RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] HW doesn't support BEAMFORMER! Skip setting...\n"));
			}
		}


		if(userBeamformSet & BIT3)
		{
			if(bHwSupportMUBeamformer)
			{
				SET_FLAG(pTmpVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_MU_MIMO_AP_ENABLE);
			}
			else
			{
				RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] HW doesn't support MU BEAMFORMER! Skip setting...\n"));
			}
		}

		if((userBeamformSet & BIT4) && bHwSupportBeamformer)
		{
			SET_FLAG(pTmpHTInfo->HtBeamformCap, BEAMFORMING_HT_BEAMFORMER_ENABLE);
		}
		if((userBeamformSet & BIT5) && bHwSupportBeamformee)
		{
			SET_FLAG(pTmpHTInfo->HtBeamformCap, BEAMFORMING_HT_BEAMFORMEE_ENABLE);
		}
		if((userBeamformSet & BIT6) && bHwSupportBeamformer)
		{
			SET_FLAG(pTmpHTInfo->HtBeamformCap, BEAMFORMING_HT_BEAMFORMER_TEST);
		}

		if(userBeamformSet & BIT7)
		{
			if(bHwSupportMUBeamformee)
			{
				SET_FLAG(pTmpVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_MU_MIMO_STA_ENABLE);
			}
			else
			{
				RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] HW doesn't support MU BEAMFORMEE! Skip setting...\n"));
			}
		}

		tmpAdapter = GetNextExtAdapter(tmpAdapter);
	}while(tmpAdapter != NULL);

	return RT_STATUS_SUCCESS;
}

//
// Description:
//	Set 802.11 LDPC mode for HT and VHT capability
// Arguments:
//	[in] Adapter -
//		NIC adapter context pointer.
//	[in] InformationBuffer -
//		The content location
//	[in] InformationBufferLength -
//		The length in byte of InformationBuffer.
// Remark:
//	Information Buffer contains one byte as the following setting.
// 	BIT0 - LDPC_VHT_ENABLE_RX, 
//	BIT1 - LDPC_VHT_ENABLE_TX
//	BIT2 - LDPC_VHT_TEST_TX_ENABLE(Don't care protocol, just sending VHT LDPC)
//
//	BIT4 - LDPC_HT_ENABLE_RX
//	BIT5 - LDPC_HT_ENABLE_TX
// 	BIT6 - LDPC_HT_TEST_TX_ENABLE (Don't care protocol, just sending HT LDPC)
// By Bruce, 2012-08-27.
//
RT_STATUS
MgntActSet_802_11_LDPC_MODE(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength
	)
{
	PADAPTER					tmpAdapter = GetDefaultAdapter(pAdapter);
	PRT_HIGH_THROUGHPUT			pTmpHTInfo = NULL;
	PRT_VERY_HIGH_THROUGHPUT	pTmpVHTInfo = NULL;
	u1Byte						userLdpcSet = 0;
	BOOLEAN						bHwSupportLdpcTx = FALSE, bHwSupportLdpcRx = FALSE;

	if(InformationBufferLength < sizeof(u1Byte))
	{
		RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] Invalid buffer length (%d)\n", InformationBufferLength));
		return RT_STATUS_INVALID_PARAMETER;
	}

	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_TX_LDPC, (pu1Byte)&bHwSupportLdpcTx);
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_RX_LDPC, (pu1Byte)&bHwSupportLdpcRx);
	if(!bHwSupportLdpcTx && !bHwSupportLdpcRx)
	{
		RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] HW doesn't not support LDPC, skip setting...\n"));
		return RT_STATUS_FAILURE;
	}

	userLdpcSet = *(InformationBuffer);

	if(!bHwSupportLdpcTx && 
		((userLdpcSet & LDPC_VHT_ENABLE_TX) || 
		  (userLdpcSet & LDPC_VHT_TEST_TX_ENABLE) ||
		  (userLdpcSet & LDPC_HT_ENABLE_TX) ||
		  (userLdpcSet & LDPC_HT_TEST_TX_ENABLE)))
	{
		RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] HW doesn't support set LDPC Tx\n"));
		return RT_STATUS_FAILURE;
	}
	if(!bHwSupportLdpcRx &&
		((userLdpcSet & LDPC_VHT_ENABLE_RX) ||
		  (userLdpcSet & LDPC_HT_ENABLE_RX)))
	{
		RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] HW doesn't support set LDPC Rx\n"));
		return RT_STATUS_FAILURE;
	}
	
	SetRegLdpc(pAdapter, *(InformationBuffer) );

	RT_TRACE_F(COMP_HT, DBG_LOUD, ("LDPC config = 0x%02X\n", userLdpcSet));
	
	// Set for all adapters
	do
	{
		pTmpHTInfo = GET_HT_INFO(&(tmpAdapter->MgntInfo));
		pTmpVHTInfo = GET_VHT_INFO(&(tmpAdapter->MgntInfo));

		CLEAR_FLAGS(pTmpVHTInfo->VhtLdpcCap);
		CLEAR_FLAGS(pTmpHTInfo->HtLdpcCap);
		if(userLdpcSet & BIT0)
		{
			SET_FLAG(pTmpVHTInfo->VhtLdpcCap, LDPC_VHT_ENABLE_RX);
		}
		if(userLdpcSet & BIT1)
		{
			SET_FLAG(pTmpVHTInfo->VhtLdpcCap, LDPC_VHT_ENABLE_TX);
		}
		if(userLdpcSet & BIT2)
		{
			SET_FLAG(pTmpVHTInfo->VhtLdpcCap, LDPC_VHT_TEST_TX_ENABLE);
		}
		if(userLdpcSet & BIT4)
		{
			SET_FLAG(pTmpHTInfo->HtLdpcCap, LDPC_HT_ENABLE_RX);
		}
		if(userLdpcSet & BIT5)
		{
			SET_FLAG(pTmpHTInfo->HtLdpcCap, LDPC_HT_ENABLE_TX);
		}
		if(userLdpcSet & BIT6)
		{
			SET_FLAG(pTmpHTInfo->HtLdpcCap, LDPC_HT_TEST_TX_ENABLE);
		}
		tmpAdapter = GetNextExtAdapter(tmpAdapter);
	}while(tmpAdapter != NULL);

	return RT_STATUS_SUCCESS;
}

//
// Description:
//	Set 802.11 STBC mode for HT and VHT capability
// Arguments:
//	[in] Adapter -
//		NIC adapter context pointer.
//	[in] InformationBuffer -
//		The content location
//	[in] InformationBufferLength -
//		The length in byte of InformationBuffer.
// Remark:
//	Information Buffer contains one byte as the following setting.
// 	BIT0 - STBC_VHT_ENABLE_RX, 
//	BIT1 - STBC_VHT_ENABLE_TX
//	BIT2 - STBC_VHT_TEST_TX_ENABLE(Don't care protocol, just sending VHT STBC), test mode.
//
//	BIT4 - STBC_HT_ENABLE_RX
//	BIT5 - STBC_HT_ENABLE_TX
// 	BIT6 - STBC_HT_TEST_TX_ENABLE (Don't care protocol, just sending HT STBC), test mode.
// By Bruce, 2012-11-07.
//
RT_STATUS
MgntActSet_802_11_STBC_MODE(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength
	)
{
	PADAPTER					tmpAdapter = GetDefaultAdapter(pAdapter);
	PRT_HIGH_THROUGHPUT			pTmpHTInfo = NULL;
	PRT_VERY_HIGH_THROUGHPUT	pTmpVHTInfo = NULL;
	u1Byte						userStbcSet = 0;
	BOOLEAN						bHwSupportStbc = FALSE;

	if(InformationBufferLength < sizeof(u1Byte))
	{
		RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] Invalid buffer length (%d)\n", InformationBufferLength));
		return RT_STATUS_INVALID_PARAMETER;
	}

	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_TX_STBC, (pu1Byte)&bHwSupportStbc);
	
	if(!bHwSupportStbc)
	{
		RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] HW doesn't not support STBC, skip setting...\n"));
		return RT_STATUS_FAILURE;
	}
	userStbcSet = *(InformationBuffer);
	SetRegStbc(pAdapter, *(InformationBuffer));

	RT_TRACE_F(COMP_HT, DBG_LOUD, ("STBC config = 0x%02X\n", userStbcSet));
	
	// Set for all adapters
	do
	{
		pTmpHTInfo = GET_HT_INFO(&(tmpAdapter->MgntInfo));
		pTmpVHTInfo = GET_VHT_INFO(&(tmpAdapter->MgntInfo));

		CLEAR_FLAGS(pTmpVHTInfo->VhtStbcCap);
		CLEAR_FLAGS(pTmpHTInfo->HtStbcCap);
		if(userStbcSet & BIT0)
		{
			SET_FLAG(pTmpVHTInfo->VhtStbcCap, STBC_VHT_ENABLE_RX);
		}
		if(userStbcSet & BIT1)
		{
			SET_FLAG(pTmpVHTInfo->VhtStbcCap, STBC_VHT_ENABLE_TX);
		}
		if(userStbcSet & BIT2)
		{
			SET_FLAG(pTmpVHTInfo->VhtStbcCap, STBC_VHT_TEST_TX_ENABLE);
		}
		if(userStbcSet & BIT4)
		{
			SET_FLAG(pTmpHTInfo->HtStbcCap, STBC_HT_ENABLE_RX);
		}
		if(userStbcSet & BIT5)
		{
			SET_FLAG(pTmpHTInfo->HtStbcCap, STBC_HT_ENABLE_TX);
		}
		if(userStbcSet & BIT6)
		{
			SET_FLAG(pTmpHTInfo->HtStbcCap, STBC_HT_TEST_TX_ENABLE);
		}
		tmpAdapter = GetNextExtAdapter(tmpAdapter);
	}while(tmpAdapter != NULL);

	return RT_STATUS_SUCCESS;
}


VOID
MgntActSet_S5_WAKEUP_INFO(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength
	)
{
	PMGNT_INFO			pMgntInfo 	= &(pAdapter->MgntInfo);
	PRT_S5WakeUPInfo	pS5WakeInfo = NULL;
	
	pS5WakeInfo	= (PRT_S5WakeUPInfo)InformationBuffer;

	pMgntInfo->mPasspharseLen = pS5WakeInfo->PSKLen;

	PlatformMoveMemory(pMgntInfo->mbPassphrase, pS5WakeInfo->PSK, pMgntInfo->mPasspharseLen);

	RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "S5_WAKEUP_INFO PSK (Passphrase): \n", pMgntInfo->mbPassphrase, pMgntInfo->mPasspharseLen)

	SecStaGenPMKForS5(pAdapter);
}



#if (P2P_SUPPORT == 1)	
VOID
MgntActSet_P2PMode(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN		bP2PMode,
	IN 	BOOLEAN		bGO,
	IN	u1Byte			ListenChannel,
	IN	u1Byte			IntendedOpChannel,
	IN	u1Byte			GOIntent
	)
{
	PP2P_INFO	pP2PInfo = (PP2P_INFO)(Adapter->MgntInfo.pP2PInfo);
	BOOLEAN		bCurrentP2PEnabled = P2P_ENABLED(pP2PInfo);
	
	RT_TRACE_F(COMP_P2P, DBG_LOUD, 
		("bP2PMode(%u), bGO(%u), LsnCh(%u), OpCh(%u), GOIntent(%u)\n",
		bP2PMode, bGO, ListenChannel, IntendedOpChannel, GOIntent));

	if(!bCurrentP2PEnabled && !bP2PMode)
	{// disabled and want to disable again => ignore
		return;
	}

	if(bCurrentP2PEnabled)
	{
		BOOLEAN bScanInProgress;
	
		//
		// Stop any existing scan
		//
		bScanInProgress = MgntScanInProgress(&pP2PInfo->pAdapter->MgntInfo);
		
		if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
		{// Doing P2P Device Discovery
			//DbgPrint("P2P_DOING_DEVICE_DISCOVERY\n");
			P2PScanListCeaseScan(pP2PInfo);
			P2PDeviceDiscoveryComplete(pP2PInfo, TRUE); // P2P State is restored in this function.
		}
		else if(pP2PInfo->bExtendedListening)
		{// Doing extended listening
			//DbgPrint("bExtendedListening\n");
			P2PScanListCeaseScan(pP2PInfo);
			P2PExtendedListenComplete(pP2PInfo);
		}
		else if(bScanInProgress) 
		{// Doing normal scan
			//DbgPrint("bScanInProgress\n");
			P2PScanListCeaseScan(pP2PInfo);
//			P2PNormalScanComplete(pP2PInfo);
		}
	}

	if(bP2PMode) 
	{
		if(!P2P_ENABLED(pP2PInfo))
		{
			P2PInitialize(pP2PInfo, Adapter, ListenChannel, IntendedOpChannel, GOIntent);
		}
		
		if(bGO)
		{
			P2PGOStartAutomously(pP2PInfo);
		}
		else
		{
		}
	}
	else 
	{
//Disconnect when set P2P disable
//by sherry 20130117
		if(((MgntLinkStatusQuery(Adapter) == RT_MEDIA_CONNECT) || Adapter->MgntInfo.bIbssStarter)&&(GetDefaultAdapter(Adapter)->bInHctTest))
		{
			RT_TRACE(COMP_MLME,DBG_LOUD,("MgntActSet_P2PMode: media connect \n"));
			MgntDisconnect(Adapter,disas_lv_ss);
		}
		P2PDisable(Adapter);
	}
}

VOID
MgntActSet_P2PProvisionIE(
	IN	PADAPTER		Adapter,
	IN	pu1Byte 			Information, // along with OUI, no id and length
	IN	u2Byte		 	InformationLen
	)
{
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "MgntActSet_P2PProvisionIE:\n", 
		Information, InformationLen);
	
	P2P_SetWpsIe(Adapter, InformationLen, Information);
}

VOID
MgntActSet_P2PFlushScanList(
	IN	PADAPTER		Adapter
	)
{
	PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);
	
	if(P2P_ENABLED(pP2PInfo))
	{
		P2PScanListClear(pP2PInfo);
		WFD_ScanListClear(Adapter);
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Flush P2P scan list!\n"));
	}
}

VOID
MgntActSet_P2PProvisioningResult(
	IN	PADAPTER		Adapter,
	IN	P2P_PROVISIONING_RESULT ProvisioningResult
	)
{
	PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);
	
	if(P2P_ENABLED(pP2PInfo))
	{
		pP2PInfo->ConnectionContext.ProvisioningResult = ProvisioningResult;
	}
}

//
// Description:
//	Set P2P Channel List
// Arguments:
//	[IN] Adapter -
//		NIC adapter context pointer.
//	[IN] ChannelListLen -
//		number of channels in the pChannelList parameter
//	[IN] pChannelList -
//		a pointer to the buffer that contains the channel list to set
// Return:
//	VOID
// Remark:
//	Add the specified channel list to a list of supported regulatory class. A single channel
//	may appear in multiple regulatory class.
//
VOID
MgntActSet_P2PChannelList(
	IN	PADAPTER Adapter,
	IN	u1Byte ChannelListLen,
	IN	pu1Byte pChannelList
	)
{
	PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);
	u4Byte nRegClasses = 0;
	u1Byte i = 0;
	
	if(!P2P_ENABLED(pP2PInfo))
	{
		//return;
	}

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ChannelList to set: ", pChannelList, ChannelListLen);

	nRegClasses = sizeof(P2PRegClass) / sizeof(P2PRegClass[0]) - 1;
	
	RT_ASSERT(nRegClasses <= P2P_MAX_REG_CLASSES, 
			("MgntActSet_P2PChannelList(): not enough space for all regulatory class: %u > %u",
			nRegClasses, P2P_MAX_REG_CLASSES));

	// Reset channel entry list
	pP2PInfo->ChannelEntryList.regClasses = 0;

	for(i = 0; P2PRegClass[i].RegClass; i++) // foreach regulatory class
	{
		P2P_REG_CLASS *pChEntry = &(pP2PInfo->ChannelEntryList.regClass[pP2PInfo->ChannelEntryList.regClasses]);
		u1Byte Channel = 0;
		PP2P_REG_CLASS_MAP pRecClass = &(P2PRegClass[i]);

		if(CHANNEL_WIDTH_40 == pRecClass->chWidth &&
			!(GET_HT_INFO(&Adapter->MgntInfo))->bEnableHT)
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("MgntActSet_P2PChannelList(): skip reg class: %u because HT not enabled\n", 
				pRecClass->RegClass));
			continue;
		}

		// Add regulatory class
		pChEntry->regClass= pRecClass->RegClass;
		pChEntry->channels= 0;
		pP2PInfo->ChannelEntryList.regClasses++;
		
		for (Channel = pRecClass->MinChannel; 
				Channel <= pRecClass->MaxChannel; 
				Channel += pRecClass->ChannelGap) // foreach channels in the regulatory class
		{
			u1Byte j = 0;

			// The channel deriviated from the P2PRegClass is not in the input channel list
			for(j = 0; j < ChannelListLen && pChannelList[j] != Channel; j++){}
			if(j == ChannelListLen) continue;

			//
			// Note that same channel can belong to different reg class
			//

			pChEntry->channel[pChEntry->channels] = Channel;
			pChEntry->channels++;

			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("MgntActSet_P2PChannelList: add channel: %4s: %4u, %4s: %4u\n", 
				"reg", pChEntry->regClass,
				"ch", Channel));
		}
	}
}

VOID
MgntActSet_P2PListenChannel(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		ListenChannel
	)
{
	PP2P_INFO	pP2PInfo = GET_P2P_INFO(Adapter);
	PADAPTER	pDefAdapter = GetDefaultAdapter(Adapter);
	PADAPTER	pLoopAdapter = pDefAdapter;	
	PHAL_DATA_TYPE		pHalData = GET_HAL_DATA(pDefAdapter);
	BOOLEAN		bSetChannel = TRUE;
	
	if(!P2P_ENABLED(pP2PInfo))
	{
		return;
	}

	// check each adapter if it is associated to the AP.
	do
	{
		// AP mode.
		if(ACTING_AS_AP(pLoopAdapter))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Adapter (%p) is acting as AP, skip setting channel!\n", pLoopAdapter));
			bSetChannel = FALSE;
			break;
		}

		// Client is assciated or connected.
		if(RT_MEDIA_CONNECT == MgntLinkStatusQuery(pLoopAdapter) || MgntRoamingInProgress(&(pLoopAdapter->MgntInfo)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Adapter (%p) is in associating state, skip setting channel!\n", pLoopAdapter));
			bSetChannel = FALSE;
			continue;
		}

		if(MgntIsLinkInProgress(&pLoopAdapter->MgntInfo) || GET_HAL_DATA(pLoopAdapter)->SwChnlInProgress)
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("MgntActSet_P2PListenChannel():Not set bw and channel because of linking in progress.\n"));
		}
	}while(NULL != (pLoopAdapter = GetNextExtAdapter(pLoopAdapter)));

	if(bSetChannel)
	{
		if(IS_NEED_EXT_IQK(pDefAdapter)) 
		{
			pHalData->bNeedIQK = TRUE;			
			pHalData->DM_OutSrc.RFCalibrateInfo.bNeedIQK = TRUE;
		}

			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("MgntActSet_P2PListenChannel(): from %u to %u \n", 
				pP2PInfo->ListenChannel, ListenChannel));

		pP2PInfo->pAdapter->MgntInfo.dot11CurrentChannelNumber = ListenChannel;
		pP2PInfo->ListenChannel = ListenChannel;		
		
		if(!pDefAdapter->MgntInfo.RegRfOff)
		{
			Adapter->HalFunc.SetWirelessModeHandler(Adapter, (u1Byte)WIRELESS_MODE_G);

				CHNL_SetBwChnl(	Adapter, 
								Adapter->MgntInfo.dot11CurrentChannelNumber,
						 		CHANNEL_WIDTH_20, 
						 		EXTCHNL_OFFSET_NO_EXT
						 		);
		}
	}
}
#endif	// #if (P2P_SUPPORT == 1)	

#if (WPS_SUPPORT == 1)
RT_STATUS
MgntActSet_WPS_Information(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u2Byte			InformationBufferLength
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(pMgntInfo);
	u2Byte				NeedLength = 3;	// opcode + para length
	u1Byte				wpsinfoOpcode = WPS_INFO_NO_OPERATION;
	u2Byte				paraLength = 0;
	RT_STATUS			rtStatus = RT_STATUS_NOT_SUPPORT;

	if(pSimpleConfig == NULL)
		goto End;
	
	if(pSimpleConfig->WpsIeVersion < SUPPORT_WPS_INFO_VERSION)
	{
		RT_TRACE(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): Not support this version!\n"));
		goto End;
	}

	if(InformationBufferLength < NeedLength)
	{
		RT_TRACE(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): No op-code or length specified!\n"));
		rtStatus = RT_STATUS_RESOURCE;
		goto End;
	}

	wpsinfoOpcode = *((pu1Byte)InformationBuffer);
	paraLength = *((pu2Byte)((pu1Byte)InformationBuffer+1));
	
	NeedLength += paraLength;
	if(InformationBufferLength < NeedLength)
	{
		RT_TRACE(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): content length mismatch!\n"));
		rtStatus = RT_STATUS_RESOURCE;
		goto End;
	}
	
	switch(wpsinfoOpcode)
	{
	case WPS_INFO_BEACON_IE:
	{
		pSimpleConfig->ieBeaconLen = paraLength;
		PlatformFillMemory(pSimpleConfig->ieBeaconBuf, MAX_SIMPLE_CONFIG_IE_LEN_V2, 0);
		CopyMem(pSimpleConfig->ieBeaconBuf, ((u1Byte *)InformationBuffer + 3), pSimpleConfig->ieBeaconLen);
		RT_PRINT_DATA(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): Beacon IE\n"), pSimpleConfig->ieBeaconBuf, pSimpleConfig->ieBeaconLen);
	}
	break;

	case WPS_INFO_ASOCREQ_IE:
	{
		pSimpleConfig->ieAsocReqLen = paraLength;
		PlatformFillMemory(pSimpleConfig->ieAsocReqBuf, MAX_SIMPLE_CONFIG_IE_LEN_V2, 0);
		CopyMem(pSimpleConfig->ieAsocReqBuf, ((u1Byte *)InformationBuffer + 3), pSimpleConfig->ieAsocReqLen);
		RT_PRINT_DATA(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): Asoc Req IE\n"), pSimpleConfig->ieAsocReqBuf, pSimpleConfig->ieAsocReqLen);
	}
	break;

	case WPS_INFO_ASOCRSP_IE:
	{
		pSimpleConfig->ieAsocRspLen = paraLength;
		PlatformFillMemory(pSimpleConfig->ieAsocRspBuf, MAX_SIMPLE_CONFIG_IE_LEN_V2, 0);
		CopyMem(pSimpleConfig->ieAsocRspBuf, ((u1Byte *)InformationBuffer + 3), pSimpleConfig->ieAsocRspLen);
		RT_PRINT_DATA(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): Asoc Rsp IE\n"), pSimpleConfig->ieAsocRspBuf, pSimpleConfig->ieAsocRspLen);
	}
	break;

	case WPS_INFO_PROBEREQ_IE:
	{
		pSimpleConfig->ieProbeReqLen = paraLength;
		PlatformFillMemory(pSimpleConfig->ieProbeReqBuf, MAX_SIMPLE_CONFIG_IE_LEN_V2, 0);
		CopyMem(pSimpleConfig->ieProbeReqBuf, ((u1Byte *)InformationBuffer + 3), pSimpleConfig->ieProbeReqLen);
		RT_PRINT_DATA(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): Probe Req IE\n"), pSimpleConfig->ieProbeReqBuf, pSimpleConfig->ieProbeReqLen);
	}
	break;

	case WPS_INFO_PROBERSP_IE:
	{
		pSimpleConfig->ieProbeRspLen = paraLength;
		PlatformFillMemory(pSimpleConfig->ieProbeRspBuf, MAX_SIMPLE_CONFIG_IE_LEN_V2, 0);
		CopyMem(pSimpleConfig->ieProbeRspBuf, ((u1Byte *)InformationBuffer + 3), pSimpleConfig->ieProbeRspLen);
		RT_PRINT_DATA(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): Probe Rsp IE\n"), pSimpleConfig->ieProbeRspBuf, pSimpleConfig->ieProbeRspLen);
		if(pSimpleConfig->ieProbeRspLen > 2)
		{
#if (P2P_SUPPORT == 1)
			MgntActSet_P2PProvisionIE(Adapter, pSimpleConfig->ieProbeRspBuf + 2, pSimpleConfig->ieProbeRspLen - 2);
#endif
		}
	}
	break;

	case WPS_INFO_QUERY_INFO:
	{
		pSimpleConfig->InfoCtrl = *((pu1Byte)(InformationBuffer+3));
		RT_TRACE(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): query information control (%d)\n", pSimpleConfig->InfoCtrl));
	}
	break;

	default:
		RT_TRACE(COMP_WPS, DBG_LOUD, ("MgntActSet_WPS_Information(): No matching op-code (%d)\n", wpsinfoOpcode));
		break;
	}
	rtStatus = RT_STATUS_SUCCESS;

	End:
		return rtStatus;
}
#endif

