#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "ApEngine.tmh"
#endif

#if 1//(AP_MODE_SUPPORT == 1)
//------------------------------------------------------------------------------
// AssociateEntry related operations.
//------------------------------------------------------------------------------
u1Byte
AsocEntry_ComputeSum(
	IN	pu1Byte		MacAddr
	)
{
	u4Byte sum;

	sum =	MacAddr[0]+
			MacAddr[1]+
			MacAddr[2]+
			MacAddr[3]+
			MacAddr[4]+
			MacAddr[5];

	return (u1Byte)(sum % ASSOCIATE_ENTRY_NUM);
}

PRT_WLAN_STA
AsocEntry_GetEntry(
	IN	PMGNT_INFO	pMgntInfo,
	IN	pu1Byte		MacAddr
	)
{
	int		i;
	u1Byte	sum;

	//RT_PRINT_ADDR(COMP_MLME, DBG_TRACE, "AsocEntry_GetEntry: \n", MacAddr);

	sum = AsocEntry_ComputeSum(MacAddr);

	if(!pMgntInfo->AsocEntry[sum].bUsed)
	{
		return NULL;
	}
	
	for(i = sum;i < ASSOCIATE_ENTRY_NUM; i++)
	{
		if(	pMgntInfo->AsocEntry[i].bUsed && 
			pMgntInfo->AsocEntry[i].Sum == sum &&
			PlatformCompareMemory(MacAddr, pMgntInfo->AsocEntry[i].MacAddr, 6) == 0 )
		{
			return &(pMgntInfo->AsocEntry[i]);
		}
	}
	for(i = 0; i < sum; i++)
	{
		if(	pMgntInfo->AsocEntry[i].bUsed && 
			pMgntInfo->AsocEntry[i].Sum == sum &&
			PlatformCompareMemory(MacAddr, pMgntInfo->AsocEntry[i].MacAddr, 6) == 0 )
		{
			return &(pMgntInfo->AsocEntry[i]);
		}
	}

	return NULL;
}


PRT_WLAN_STA
AsocEntry_GetEntryByMacId(
	IN	PMGNT_INFO	pMgntInfo,
	IN	u1Byte		MacId
	)
{
	int		i;
	u1Byte	AID = (u1Byte) MacIdGetOwnerAssociatedClientAID(GetAdapterByMgntInfo(pMgntInfo), MacId);

	// Prefast warning ignore for false positive
#pragma warning( disable:6064 )
	RT_PRINT_ADDR(COMP_MLME, DBG_TRACE, "AsocEntry_GetEntryByMacId: %d\n", MacId);

	for(i = 0;i < ASSOCIATE_ENTRY_NUM; i++)
	{
		if(pMgntInfo->AsocEntry[i].bUsed && pMgntInfo->AsocEntry[i].AID == AID)
		{
			return &(pMgntInfo->AsocEntry[i]);
		}
	}


	return NULL;
}



PRT_WLAN_STA
AsocEntry_GetFreeEntry(
	IN	PMGNT_INFO	pMgntInfo,
	IN	pu1Byte		MacAddr
	)
{
	int		i;
	u1Byte	sum;

	sum = AsocEntry_ComputeSum(MacAddr);

	for(i = sum; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		if(!pMgntInfo->AsocEntry[i].bUsed)
		{
			return &(pMgntInfo->AsocEntry[i]);
		}
	}
	for(i = 0; i < sum; i++)
	{
		if(!pMgntInfo->AsocEntry[i].bUsed)
		{
			return &(pMgntInfo->AsocEntry[i]);
		}
	}

	return NULL;
}

BOOLEAN
AsocEntry_AddStation(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			MacAddr,
	IN	AUTH_ALGORITHM	AuthAlg
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_WLAN_STA	pEntry;
	u1Byte			StaHWLimit = 0;
	
	if(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_NO_CIPHER)
		StaHWLimit = ASSOCIATE_ENTRY_NUM;
	else if(Adapter == GetDefaultAdapter (Adapter))
		StaHWLimit = 32;
	else
		StaHWLimit = 15;
	
	if( (ASSOCIATE_ENTRY_NUM -pMgntInfo->AvailableAsocEntryNum) >= StaHWLimit)
	{
		RT_TRACE( COMP_AP, DBG_LOUD, ("AsocEntry_AddStation(): Fail %d\n", pMgntInfo->AvailableAsocEntryNum) );
		return FALSE;
	}


	pEntry = AsocEntry_GetFreeEntry(pMgntInfo, MacAddr);

	if(pEntry != NULL)
	{
		RT_TRACE_F(COMP_AP, DBG_TRACE, ("AsocEntry_ResetEntry\n"));
	
		AsocEntry_ResetEntry(Adapter, pEntry);
	
		pEntry->bUsed = TRUE;
		pEntry->Sum = AsocEntry_ComputeSum(MacAddr);
		PlatformMoveMemory(pEntry->MacAddr, MacAddr, 6);
		pEntry->AuthAlg = AuthAlg;

		pMgntInfo->AvailableAsocEntryNum--;
		RT_TRACE( COMP_AP, DBG_LOUD, ("AsocEntry_AddStation(): Success %d\n", pMgntInfo->AvailableAsocEntryNum) );
		RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "Entry Address", pEntry->MacAddr);

		//vivi mask this, use Adapter->DM_DigTable.CurMultiSTAConnectState = DIG_MultiSTA_CONNECT instead of it
		//DM_MultiSTA_InitGainChangeNotify_CONNECT(Adapter);			
		{	// Temp fix need to fine tune later.
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
			pHalData->DM_OutSrc.DM_DigTable.CurMultiSTAConnectState = DIG_MultiSTA_CONNECT;
		}
	}

	return TRUE;
}


VOID
AsocEntry_ResetEntry(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry
	)
{
	PRT_TCB			pTcb;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);

	pEntry->AuthPassSeq = 0;
	pEntry->bAssociated = FALSE;
	pEntry->LastActiveTime = PlatformGetCurrentTime();
	pEntry->bPowerSave = FALSE;
	pEntry->WirelessMode = WIRELESS_MODE_UNKNOWN;

	pEntry->DataRate = 0;
	pEntry->bUseShortGI = FALSE;
	
	pEntry->IOTAction = 0;
	pEntry->IOTPeer = HT_IOT_PEER_UNKNOWN;

	//yangjun add this,20111226
 	PlatformZeroMemory(&pEntry->rssi_stat, sizeof(RSSI_STA));
	//vivi add this, 20110719
	pEntry->AID = 0;

	// HalMacId to this AID ------------------
	pEntry->AssociatedMacId = 0;
	// ------------------------------------
	
	//
	// QoS Capability
	//
	pEntry->QosMode = QOS_DISABLE;
	
	pEntry->PSIdleCunt = 0;
	

	//
	// HT Capability.
	//
	AP_HTResetSTAEntry(Adapter, pEntry);
	AP_VHTResetSTAEntry(Adapter, pEntry);
	
	// Added by Annie, 2005-07-01.
	if( ACTING_AS_AP(Adapter) && 
	   (pSecInfo->AuthMode == RT_802_11AuthModeWPAPSK ||pSecInfo->SecLvl == RT_SEC_LVL_WPA2 ) )
	{
		Authenticator_StateINITIALIZE(Adapter, pEntry);
	}

	TDLS_ResetAsocEntry(Adapter, pEntry);

	WFD_RemoveAssocClient(Adapter, pEntry->MacAddr);

	//
	// 061221, rcnjko: 
	// 1. We must clear PTK for RSNA IBSS case if peer STA is gone, 
	// otherwise, when the STA join this IBSS again, we'll try send 
	// encrypted EAPOL-KEY to it, and therefore 4-way fails. 
	//
	// 2. As to our GTK of RSNA IBSS and WEP key, NDIS6 won't set it again 
	// even all peer STA leaved. So, keep default key. 
	//
	if( pMgntInfo->OpMode == RT_OP_MODE_IBSS && pMgntInfo->bRSNAPSKMode )
	{
		
		int index;
		PPER_STA_MPAKEY_ENTRY		pMapKey;
		PPER_STA_DEFAULT_KEY_ENTRY	pDefKey;

		//
		// Clear peer STA's PTK and GTK.
		//
		for(index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++)
		{
			pMapKey = &(pSecInfo->MAPKEYTable[index]);
			if( pMapKey->Valid && eqMacAddr(pMapKey->MACAdrss, pEntry->MacAddr) )
			{
				RT_PRINT_ADDR( COMP_SEC, DBG_LOUD, "RSNA IBSS: Clear PTK of ", pEntry->MacAddr);
				PlatformZeroMemory( pMapKey, sizeof(PER_STA_MPAKEY_ENTRY) ); 
			}

			pDefKey = &(pSecInfo->PerStaDefKeyTable[index]);
			if( pDefKey->Valid && eqMacAddr(pDefKey->MACAdrss, pEntry->MacAddr))
			{
				RT_PRINT_ADDR( COMP_SEC, DBG_LOUD, "RSNA IBSS: Clear GTK of ", pEntry->MacAddr);
				PlatformZeroMemory( pDefKey, sizeof(PER_STA_DEFAULT_KEY_ENTRY) ); 
			}
		}
		
		//vivi added for new cam search flow, 20091028
		//also remove hw cam 
		SEC_AsocEntry_ResetEntry(Adapter, pEntry);

	}
	//
	// Initialize the indication state machine.
	//
	pEntry->IndicationEngine.CurrentState = RT_STA_STATE_INITIAL;
	pEntry->IndicationEngine.NextState = RT_STA_STATE_AP_INCOMING_ASOC_STARTED;

	pEntry->bOsDecisionMade = FALSE;
	pEntry->bNotAcceptedByOs = FALSE;

	// Isaiah for 92EU SoftAP with 11 clients, after 1hr BSOD. it shows AP_RecvAsocReq/AP_SendAsocResp/AP_AsocRspIEFromOS = null.
	// 2014-07-14
	if(pEntry->AP_RecvAsocReq  != NULL)
	{
		PlatformFreeMemory(pEntry->AP_RecvAsocReq, 1024);
		pEntry->AP_RecvAsocReq = NULL;
	}		
	pEntry->AP_RecvAsocReqLength = 0;

	if(pEntry->AP_SendAsocResp != NULL)
	{
		PlatformFreeMemory(pEntry->AP_SendAsocResp, 1024);
		pEntry->AP_SendAsocResp = NULL;
	}
	pEntry->AP_SendAsocRespLength = 0;

	if(pEntry->AP_AsocRspIEFromOS != NULL)
	{
		PlatformFreeMemory(pEntry->AP_AsocRspIEFromOS, 1024);
		pEntry->AP_AsocRspIEFromOS = NULL;
	}
	pEntry->AP_AsocRspIEFromOSLength = 0;

	pEntry->WmmStaQosInfo = 0;
	pEntry->WmmEosp = RT_STA_EOSP_STATE_OPENED;

	// Return all packets queued for the STA.
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	while( !RTIsListEmpty(&(pEntry->PsQueue)) )
	{
		pTcb = (PRT_TCB)RTRemoveHeadList(&(pEntry->PsQueue));
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
	}
	while( !RTIsListEmpty(&(pEntry->WmmPsQueue)) )
	{
		pTcb = (PRT_TCB)RTRemoveHeadList(&(pEntry->WmmPsQueue));
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
	}
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

#if (P2P_SUPPORT == 1)
	//
	// Reset P2P related entries
	//
	pEntry->bP2PClient = FALSE;
	PlatformZeroMemory(&pEntry->P2PClientInfoDesc, sizeof(P2P_CLIENT_INFO_DISCRIPTOR));
#endif	// #if (P2P_SUPPORT == 1)

}

VOID
AsocEntry_RemoveStation(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		MacAddr
	)
{
	u2Byte			i, j;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, MacAddr);
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T					pDM_Odm = &pHalData->DM_OutSrc;

	if(pEntry == NULL)
		return;

#if (P2P_SUPPORT==1)
	AP_CHNAGE_CLIENT_PS_STATE(Adapter, pEntry, FALSE);
#endif

	RemovePeerTS(Adapter, MacAddr);
	
	if(GET_TDLS_ENABLED(pMgntInfo) && !ACTING_AS_AP(Adapter))
	{
		TDLS_RemovePeer(Adapter, MacAddr);
	}

	AsocEntry_ResetAvailableAID(pMgntInfo, pEntry->AID);

	// HalMacId Remove the specific MacId associated with this peer ---------
	if(pEntry->bAssociated)
	{// Note that MacId is assigned when it is associated
		MacIdDeregisterSpecificMacId(Adapter, pEntry->AssociatedMacId);
	}
	// -------------------------------------------------------------

	RT_TRACE_F(COMP_AP, DBG_TRACE, ("AsocEntry_ResetEntry\n"));
	AsocEntry_ResetEntry(Adapter, pEntry); // This will free packets queued for this station.
	pEntry->bUsed = FALSE;
	pMgntInfo->AvailableAsocEntryNum++;

	if(pMgntInfo->AvailableAsocEntryNum == ASSOCIATE_ENTRY_NUM)
	{
		//vivi mask this, use the next line instead of it
		//DM_MultiSTA_InitGainChangeNotify_DISCONNECT(Adapter);
		{	// Temp fix need to fine tune later.
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
			pHalData->DM_OutSrc.DM_DigTable.CurMultiSTAConnectState = DIG_MultiSTA_DISCONNECT;
		}
	}

	// Since we have an empty entry now, we move entries to reduce the time for search entry.
	// <RJ_TODO> I don't think the clean up operation is worth, it is O(n^2).
	for(i = 0;i < ASSOCIATE_ENTRY_NUM; i++)
	{
		if(pMgntInfo->AsocEntry[i].bUsed)
			continue;
		
		// Move first of the entry with hash value=i to this empty entry.
		for(j = 0; j < ASSOCIATE_ENTRY_NUM; j++)
		{
			if(pMgntInfo->AsocEntry[j].bUsed && pMgntInfo->AsocEntry[j].Sum == i)
			{
				PlatformMoveMemory(
					&(pMgntInfo->AsocEntry[i]),
					&(pMgntInfo->AsocEntry[j]),
					sizeof(RT_WLAN_STA)
					);

				PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

				if( RTIsListEmpty(&(pMgntInfo->AsocEntry[j].PsQueue)) )
				{
					RTInitializeListHead( &(pMgntInfo->AsocEntry[i].PsQueue) );
				}
				else
				{
					
					pMgntInfo->AsocEntry[i].PsQueue.Flink->Blink = &(pMgntInfo->AsocEntry[i].PsQueue); // 1st node.
					pMgntInfo->AsocEntry[i].PsQueue.Blink->Flink = &(pMgntInfo->AsocEntry[i].PsQueue); // last node.
				}
				if( RTIsListEmpty(&(pMgntInfo->AsocEntry[j].WmmPsQueue)) )
				{
					RTInitializeListHead( &(pMgntInfo->AsocEntry[i].WmmPsQueue) );
				}
				else
				{
					pMgntInfo->AsocEntry[i].WmmPsQueue.Flink->Blink = &(pMgntInfo->AsocEntry[i].WmmPsQueue); // 1st node.
					pMgntInfo->AsocEntry[i].WmmPsQueue.Blink->Flink = &(pMgntInfo->AsocEntry[i].WmmPsQueue); // last node.
				}
				PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
				pMgntInfo->AsocEntry[i].perSTAKeyInfo.pWLanSTA = &(pMgntInfo->AsocEntry[i]);
				if( Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm != RT_ENC_ALG_AESCCMP )
				{
					pMgntInfo->AsocEntry[i].perSTAKeyInfo.TempEncKey = pMgntInfo->AsocEntry[i].perSTAKeyInfo.PTK + TKIP_ENC_KEY_POS;
					pMgntInfo->AsocEntry[i].perSTAKeyInfo.TxMICKey = pMgntInfo->AsocEntry[i].perSTAKeyInfo.PTK + TKIP_MIC_KEY_POS;
					pMgntInfo->AsocEntry[i].perSTAKeyInfo.RxMICKey = pMgntInfo->AsocEntry[i].perSTAKeyInfo.PTK + (TKIP_MIC_KEY_POS+TKIP_MIC_KEY_LEN);
				}

				//
				// Reset source entry.
				//
				pMgntInfo->AsocEntry[j].bUsed = FALSE;
				PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
				RTInitializeListHead( &(pMgntInfo->AsocEntry[j].PsQueue) );
				RTInitializeListHead( &(pMgntInfo->AsocEntry[j].WmmPsQueue) );
				PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
				pMgntInfo->AsocEntry[j].perSTAKeyInfo.pWLanSTA = &(pMgntInfo->AsocEntry[j]);
				
				if(Adapter->TDLSSupport)
				{
					TDLS_UpdatePeer(Adapter, &(pMgntInfo->AsocEntry[i]));
				}
				i = -1;	// Restart.
				break;
			}
		}
	}
}

VOID
AsocEntry_UpdateTimeStamp(
	IN	PRT_WLAN_STA	pEntry
	)
{
	if(pEntry!=NULL)
	{
		pEntry->LastActiveTime = PlatformGetCurrentTime();
		pEntry->PSIdleCunt = 0;
	}
}


BOOLEAN
AsocEntry_AnyStationAssociated(
	IN	PMGNT_INFO	pMgntInfo
	)
{
	int i;

	if(pMgntInfo->AvailableAsocEntryNum == ASSOCIATE_ENTRY_NUM)
	{
		return FALSE;
	}

	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		if(pMgntInfo->AsocEntry[i].bUsed && pMgntInfo->AsocEntry[i].bAssociated)
		{
			return TRUE;
		}
	}

	return FALSE;
}


BOOLEAN
AsocEntry_IsStationAssociated(
	IN	PMGNT_INFO	pMgntInfo,
	IN	pu1Byte		MacAddr
	)
{
	PRT_WLAN_STA	pEntry;

	pEntry = AsocEntry_GetEntry(pMgntInfo,MacAddr);

	if(pEntry==NULL)
	{
		return FALSE;
	}
	
	if(pEntry->bAssociated)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

VOID
AsocEntry_AgeFunction(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_WLAN_STA	AsocEntry = pMgntInfo->AsocEntry;
	u2Byte			i;
	u8Byte			CurrentTime;
	u8Byte			TimeDifference;
	BOOLEAN			bFree;
	u2Byte			nBModeStaCnt = 0;
	u1Byte			nLegacyStaCnt = 0;
	u1Byte			n20MHzStaCnt = 0;

	RT_TRACE(COMP_AP, DBG_TRACE, ("==> AsocEntry_AgeFunction()\n"));


	for(i = 0;i < ASSOCIATE_ENTRY_NUM; i++)
	{
		if(AsocEntry[i].bUsed)
		{
			// TimeDifference is in ms.

			//sherry modified to fix TimeDifference error 20110517
			CurrentTime = PlatformGetCurrentTime();
			TimeDifference = PlatformDivision64(CurrentTime - AsocEntry[i].LastActiveTime, 1000);

			bFree = FALSE;
			if(AsocEntry[i].bAssociated)
			{
				RT_TRACE(COMP_AP, DBG_TRACE, ("AsocEntry_AgeFunction() check, %02X-%02X-%02X-%02X-%02X-%02X MacId %d Entry index %d\n", AsocEntry[i].MacAddr[0], AsocEntry[i].MacAddr[1], AsocEntry[i].MacAddr[2], AsocEntry[i].MacAddr[3], AsocEntry[i].MacAddr[4], AsocEntry[i].MacAddr[5], AsocEntry[i].AssociatedMacId, i));	
				// 10 minutes.
				//if(TimeDifference > 600000)		// rewrited by Annie, 2006-02-16.
				if(TimeDifference >  ((u8Byte)pMgntInfo->LiveTime)*1000)
				{
					RT_TRACE(COMP_AP, DBG_LOUD, ("AsocEntry_AgeFunction() > 10 min, %02X-%02X-%02X-%02X-%02X-%02X\n", AsocEntry[i].MacAddr[0], AsocEntry[i].MacAddr[1], AsocEntry[i].MacAddr[2], AsocEntry[i].MacAddr[3], AsocEntry[i].MacAddr[4], AsocEntry[i].MacAddr[5]));

					if(ACTING_AS_AP(Adapter))
					{
						// Disassociate inactive STA.
						AP_DisassociateStation(Adapter, &(AsocEntry[i]), inactivity);
					}
					else if(Adapter->TDLSSupport && IS_TDL_EXIST(pMgntInfo))
					{
						RemovePeerTS(Adapter, AsocEntry[i].MacAddr);

						// Mark the STA as disassoicated and related.
						AsocEntry_BecomeDisassoc(Adapter, &AsocEntry[i]);
					}

					AsocEntry_UpdateTimeStamp(&AsocEntry[i]);
				}
				else
				{
					if(AsocEntry[i].WirelessMode == WIRELESS_MODE_B)
						nBModeStaCnt++;

					if(AsocEntry[i].WirelessMode < WIRELESS_MODE_N_24G)
						nLegacyStaCnt++;
					else if(!AsocEntry[i].BandWidth)
						n20MHzStaCnt++;		

					//
					// If the power save queues are not empty, check if we should clear the queue.
					// We don't want to queue the packets too long. By Bruce, 2010-12-28.
					//
					if(!RTIsListEmpty(&(AsocEntry[i].PsQueue)) || !RTIsListEmpty(&(AsocEntry[i].WmmPsQueue)))
					{
						PRT_TCB	pTcb;

						AsocEntry[i].PSIdleCunt = (AsocEntry[i].PSIdleCunt + 1 ) % ((pMgntInfo->bWiFiConfg ? AP_CLIENT_PS_QUEUE_TIMEOUT_LOGO_SEC : AP_CLIENT_PS_QUEUE_TIMEOUT_SEC) / RT_CHECK_FOR_HANG_PERIOD);
					
						if( AsocEntry[i].PSIdleCunt == 0 )
						{
							PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
							while( !RTIsListEmpty(&(AsocEntry[i].PsQueue)) )
							{
								RT_PRINT_ADDR( COMP_AP , DBG_WARNING , "[WARNING] AsocEntry_AgeFunction(): Return TCB in PsQueue for addr = ", AsocEntry[i].MacAddr);
								pTcb = (PRT_TCB)RTRemoveHeadList(&(AsocEntry[i].PsQueue));
								ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
							}
							while( !RTIsListEmpty(&(AsocEntry[i].WmmPsQueue)) )
							{
								RT_PRINT_ADDR( COMP_AP , DBG_WARNING , "[WARNING] AsocEntry_AgeFunction(): Return TCB in WmmPsQueue for addr = ", AsocEntry[i].MacAddr);
								pTcb = (PRT_TCB)RTRemoveHeadList(&(AsocEntry[i].WmmPsQueue));
								ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
							}

							//
							// We leave power saving mode when idle condition for PSQ and WMMPSQ was met to prevent bug check code 
							// DPC_WATCHDOG_VIOLATION (133), added by Roger, 2013.08.05.
							//
							AsocEntry[i].bPowerSave = FALSE;
							PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
						}
					}
					else
					{
						AsocEntry[i].PSIdleCunt = 0;
					}
				}
			}
			else
			{
				// 0.5 second.
				if(TimeDifference > 500)
				{
					RT_TRACE(COMP_AP, DBG_LOUD, ("AsocEntry_AgeFunction() > 0.5 sec, %02X-%02X-%02X-%02X-%02X-%02X\n", AsocEntry[i].MacAddr[0], AsocEntry[i].MacAddr[1], AsocEntry[i].MacAddr[2], AsocEntry[i].MacAddr[3], AsocEntry[i].MacAddr[4], AsocEntry[i].MacAddr[5]));

					bFree = TRUE;
					// Commented out by rcnjko, for it causes AP become disconnected when the only one STA is roaming.
					//bNeedCheckLinkState = TRUE;
				}
			}

			if(bFree)
			{
				if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP)
				{
					pMgntInfo->pCurrentSta = &(AsocEntry[i]);
					DrvIFIndicateIncommingAssociationComplete(Adapter, unspec_reason);
				}	
				RT_TRACE_F(COMP_AP, DBG_TRACE, ("AsocEntry_RemoveStation\n"));
				AsocEntry_RemoveStation(Adapter, AsocEntry[i].MacAddr);
			}
		}
	}

	// Disable protection mode and Enable short slot time if an B-mode STA joined.
	if(IS_WIRELESS_OFDM_24G(Adapter))
	{
		// [Win7 Two Port Ext:N Def:G Collision Fixed by Acquire Protection IE on Ext]
		// Plz Reference to AP_OnAsocReq, Demogen's law, 2009.07.22, by Bohn
		// Do not update bUseProtection bit when G mode STA add or leave. 2010.11.24. by tynli.

		BOOLEAN bUseProtection = (nBModeStaCnt == 0)? FALSE: TRUE;
		
		ActUpdate_ProtectionMode(Adapter, bUseProtection);
		if(nBModeStaCnt == 0)
			pMgntInfo->mCap |= cShortSlotTime;
		else
			pMgntInfo->mCap &= (~cShortSlotTime);

		ActUpdate_mCapInfo(Adapter, pMgntInfo->mCap);
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
			OCTET_STRING 	osRateSet;
			u1Byte 			RateSet[6] = { MGN_1M, MGN_2M, MGN_5_5M, MGN_11M,MGN_6M, MGN_9M };

			//[Win7 Two Port TP IOT BroadCom Issue] when G-STA link to  our Vwifi, we use protection, 
			// however, Default link to Broadcom can only accept OFDM ACK without CCK ones. 2009.07.30, by Bohn
			if(pMgntInfo->IOTPeer == HT_IOT_PEER_BROADCOM)
			{
				FillOctetString(osRateSet, RateSet, 6);
			}
			else
			{
				FillOctetString(osRateSet, RateSet, 4);
			}
			FilterSupportRate(pMgntInfo->mBrates, &osRateSet, FALSE);
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_BASIC_RATE, (pu1Byte)&osRateSet);
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

	RT_TRACE(COMP_AP, DBG_TRACE, ("<== AsocEntry_AgeFunction()\n"));
}


VOID
AsocEntry_BecomeDisassoc(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry
	)
{
	PRT_TCB	pTcb;

	// Return all packets queued for the STA.
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	while( !RTIsListEmpty(&(pEntry->PsQueue)) )
	{
		pTcb = (PRT_TCB)RTRemoveHeadList(&(pEntry->PsQueue));
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
	}
	while( !RTIsListEmpty(&(pEntry->WmmPsQueue)) )
	{
		pTcb = (PRT_TCB)RTRemoveHeadList(&(pEntry->WmmPsQueue));
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
	}
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
	
	// Reset association state of the STA.
	pEntry->bAssociated = FALSE;

}


PRT_WLAN_STA
AsocEntry_EnumStation(
	IN	PADAPTER		Adapter,
	IN	ULONG			nIndex
	)
{
	ULONG	i, nFound = 0;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PRT_WLAN_STA	AsocEntry = pMgntInfo->AsocEntry;
	
	//<SC_TODO: need a spinlock>
	for (i=0; i<ASSOCIATE_ENTRY_NUM; i++)
	{
		if (AsocEntry[i].bUsed)
		{
			if (nFound == nIndex)
			{
				return &(AsocEntry[i]);
			}
			else
			{
				nFound ++;
			}
		}
	}

	return NULL;
}

VOID
AsocEntry_ResetAll(
	IN	PADAPTER		Adapter
	)
{
	u1Byte			i;
	PRT_WLAN_STA	pEntry;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	BOOLEAN			bUseRAMask = FALSE;
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T					pDM_Odm = &pHalData->DM_OutSrc;

	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		pEntry = &(pMgntInfo->AsocEntry[i]);

		RT_TRACE_F(COMP_AP, DBG_TRACE, ("AsocEntry_ResetEntry\n"));
		
		AsocEntry_ResetEntry(Adapter, pEntry); // This will free packets queued for this station.
		pEntry->bUsed = FALSE;
		pMgntInfo->AvailableAsocEntryNum++;
		pMgntInfo->AvailableAIDTable[i] = 0xff;
	}

	pMgntInfo->MaxMACID = 0;
	pMgntInfo->AvailableAsocEntryNum = ASSOCIATE_ENTRY_NUM;


	// Deregister all MacIDs created by this adpater ------------
	MacIdDeregisterForOwnerAdapter(Adapter);
	// --------------------------------------------------


	//vivi mask this, use the next line instead of it
	//DM_MultiSTA_InitGainChangeNotify_DISCONNECT
	{	// Temp fix need to fine tune later.
		HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
		pHalData->DM_OutSrc.DM_DigTable.CurMultiSTAConnectState = DIG_MultiSTA_DISCONNECT;
	}

	pMgntInfo->PowerSaveStationNum = 0;

	// Reinitialize rate table
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USE_RA_MASK, &bUseRAMask);
	if(!bUseRAMask)
	{
		Adapter->HalFunc.InitHalRATRTableHandler(Adapter);
	}

}

VOID
AsocEntry_UpdateAsocInfo(
	IN	PADAPTER					Adapter,
	IN	pu1Byte						StaAddr,
	IN	pu1Byte						Content,
	IN	u4Byte						ContentLength,
	IN	ASOCENTRY_UPDATE_ASOC_INFO_ACTION	Action
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_WLAN_STA 	pEntry;

	pEntry = AsocEntry_GetEntry(pMgntInfo, StaAddr);

	//RT_ASSERT(pEntry != NULL, ("AsocEntry_UpdateAsocInfo(): corresponding STA entry not found.\n"));

	if(pEntry==NULL)
	{
		RT_TRACE(COMP_AP, DBG_WARNING, ("AsocEntry_UpdateAsocInfo(): Error occur!! AssocEntry may be reset.\n"));
		return;
	}
	//RT_TRACE_F(COMP_AP, DBG_LOUD, ("Action = %d CpntentLen = %d\n", Action, ContentLength));
	//RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "AsocEntry_UpdateAsocInfo(): Addr = ", StaAddr);
	
	switch(Action)
	{
		case ALLOCATE_ASOC_REQ:
		{
			if(pEntry->AP_RecvAsocReq == NULL)
			{
				PlatformAllocateMemory(
					Adapter, 
					(PVOID *)&(pEntry->AP_RecvAsocReq), 
					1024);
			}
			break;
		}
		case ALLOCATE_ASOC_RSP:
		{
			if(pEntry->AP_SendAsocResp == NULL)
			{
				PlatformAllocateMemory(
					Adapter, 
					(PVOID *)&(pEntry->AP_SendAsocResp), 
					1024);
			}
			break;
		}
		case ALLOCATE_ASOC_RSP_IE_FROM_OS:
		{
			if(pEntry->AP_AsocRspIEFromOS == NULL)
			{
				PlatformAllocateMemory(
					Adapter, 
					(PVOID *)&(pEntry->AP_AsocRspIEFromOS), 
					1024);
			}
			break;
		}
		case UPDATE_ASOC_REQ:
		{
			//RT_ASSERT(ContentLength <= 1024, 
			//	("AsocEntry_UpdateAsocInfo(): ContentLength greater than 1024.\n"));
			if(ContentLength > 1024)
			{
				pEntry->AP_RecvAsocReqLength = 0;
				break;
			}
			
			if(pEntry->AP_RecvAsocReq)
			{
				PlatformMoveMemory(
					pEntry->AP_RecvAsocReq,
					Content,
					ContentLength);
				pEntry->AP_RecvAsocReqLength= ContentLength;		
			}
			break;
		}
		case UPDATE_ASOC_RSP:
		{	
			//RT_ASSERT(ContentLength <= 1024, 
			//	("AsocEntry_UpdateAsocInfo(): ContentLength greater than 1024.\n"));
			//RT_TRACE_F(COMP_AP, DBG_LOUD, ("UPDATE_ASOC_RSP\n"));
			//RT_PRINT_DATA(COMP_AP, DBG_LOUD, "UPDATE_ASOC_RSP Content:\n", Content, ContentLength);
			if(ContentLength > 1024)
			{
				pEntry->AP_SendAsocRespLength = 0;
				break;
			}
			
			if(pEntry->AP_SendAsocResp)
			{
				PlatformMoveMemory(
					pEntry->AP_SendAsocResp,
					Content,
					ContentLength);
				pEntry->AP_SendAsocRespLength= ContentLength;	
			}
			break;
		}
		case UPDATE_ASOC_RSP_IE_FROM_OS:
		{
			//RT_ASSERT(ContentLength <= 1024, 
			//	("AsocEntry_UpdateAsocInfo(): ContentLength greater than 1024.\n"));
			if(ContentLength > 1024)
			{
				pEntry->AP_AsocRspIEFromOSLength= 0;
				break;
			}
			
			if(pEntry->AP_AsocRspIEFromOS)
			{
				PlatformMoveMemory(
					pEntry->AP_AsocRspIEFromOS,
					Content,
					ContentLength);
				pEntry->AP_AsocRspIEFromOSLength = ContentLength;	
			}
			break;
		}
		case FREE_ASOC_REQ:
		{
			if(pEntry->AP_RecvAsocReq  != NULL)
			{
				PlatformFreeMemory(pEntry->AP_RecvAsocReq, 1024);
				pEntry->AP_RecvAsocReq = NULL;
			}
			else
				RT_ASSERT(TRUE, ("AP_RecvAsocReq is NULL\n"));
			
			pEntry->AP_RecvAsocReqLength = 0;
			break;
		}
		case FREE_ASOC_RSP:
		{
			if(pEntry->AP_SendAsocResp != NULL)
			{
				PlatformFreeMemory(pEntry->AP_SendAsocResp, 1024);
				pEntry->AP_SendAsocResp = NULL;
			}
			else
				RT_ASSERT(TRUE, ("AP_SendAsocResp is NULL\n"));
			
			pEntry->AP_SendAsocRespLength = 0;
			break;
		}
		case FREE_ASOC_RSP_IE_FROM_OS:
		{
			if(pEntry->AP_AsocRspIEFromOS != NULL)
			{
				PlatformFreeMemory(pEntry->AP_AsocRspIEFromOS, 1024);
				pEntry->AP_AsocRspIEFromOS = NULL;
			}
			else
				RT_ASSERT(TRUE, ("AP_AsocRspIEFromOS is NULL\n"));

			pEntry->AP_AsocRspIEFromOSLength = 0;
			break;
		}
		default:
			break;
	}
	
}


u1Byte
AsocEntry_AssignAvailableAID(
	IN	PMGNT_INFO	pMgntInfo,
	IN	pu1Byte		MacAddr
	)
{
	u1Byte		i =0;

	// ending
	if(pMgntInfo->TestAID != 0)
	{
		if(pMgntInfo->TestAID <=  ASSOCIATE_ENTRY_NUM)
			pMgntInfo->AvailableAIDTable[pMgntInfo->TestAID-1] = AsocEntry_ComputeSum(MacAddr);

		return pMgntInfo->TestAID;
	}
	else
	{
		for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
		{
			if(pMgntInfo->AvailableAIDTable[i] == 0xff)
			{
				pMgntInfo->AvailableAIDTable[i] = AsocEntry_ComputeSum(MacAddr);
				//RT_TRACE(COMP_AP, DBG_LOUD, ("AsocEntry_AssignAvailableAID(): [%2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x]  MacID = <Dynamic>\n", \
					//MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]));
				return i+1;
			}
		}
	}
	return 1;
}


VOID
AsocEntry_ResetAvailableAID(
	IN	PMGNT_INFO	pMgntInfo,
	IN	u2Byte		AID
	)
{
	if (AID > 0 && AID-1 < ASSOCIATE_ENTRY_NUM)
		pMgntInfo->AvailableAIDTable[AID-1] = 0xff;
	else
		RT_TRACE(COMP_AP, DBG_LOUD, ("AsocEntry_ResetAvailableAID(): clear non-existing entry AID\n"));
}




//
// Description: Return all packet queued for power-save.
// Assumption: RT_TX_SPINLOCK is acquired.
// 2005.07.05, by rcnjko.
//
VOID
AP_PS_ReturnAllQueuedPackets(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bMulticastOnly
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	int	i;
	PRT_WLAN_STA	pEntry;
	PRT_TCB	pTcb;

	RT_ASSERT(IS_TX_LOCKED(Adapter) == TRUE, ("AP_PS_ReturnAllQueuedPackets(): bTxLocked(%x) should be TRUE\n!", Adapter->bTxLocked));

	// Return all mcst/bcst packets queued.
	while( !RTIsListEmpty(&(pMgntInfo->GroupPsQueue)) )
	{
		pTcb = (PRT_TCB)RTRemoveHeadList(&(pMgntInfo->GroupPsQueue));
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
	}
	
	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		pEntry = &(pMgntInfo->AsocEntry[i]);

		// Return all packets queued for the STA.
		if(pEntry->bUsed)
		{
			while( !RTIsListEmpty(&(pEntry->PsQueue)) )
			{
				pTcb = (PRT_TCB)RTRemoveHeadList(&(pEntry->PsQueue));
				ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
			}
			while( !RTIsListEmpty(&(pEntry->WmmPsQueue)) )
			{
				pTcb = (PRT_TCB)RTRemoveHeadList(&(pEntry->WmmPsQueue));
				ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
			}
		}
		else
		{
			RT_ASSERT( RTIsListEmpty(&(pEntry->PsQueue)), ("AP_PS_ReturnAllQueuedPackets() bUsed==FALSE, but PsQueue is not empty!!!\n") );
			RT_ASSERT( RTIsListEmpty(&(pEntry->WmmPsQueue)), ("AP_PS_ReturnAllQueuedPackets() bUsed==FALSE, but WmmPsQueue is not empty!!!\n") );
		}
	}
}


//
// Description: Update the power-save state of a STA and take proper response 
// 				to the STA changing their power-save mode.
// 2005.07.05, by rcnjko.
//
PRT_WLAN_STA
AP_PS_UpdateStationPSState(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	posFrame
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_WLAN_STA	pEntry;
	BOOLEAN			bPowerSave = (Frame_PwrMgt((*posFrame))==1) ? TRUE : FALSE;
	RT_STATUS		rtStatus;
	
	pEntry = AsocEntry_GetEntry(pMgntInfo, Frame_pTaddr((*posFrame)));
	if(pEntry == NULL)
	{
		return NULL;
	}

	if(!pEntry->bAssociated)
		return NULL;


	if(bPowerSave)
	{ // STA is power-save state.
		// Reset the PS idle count because the STA is still sending PS packets.
		pEntry->PSIdleCunt = 0;			
		if(!pEntry->bPowerSave)
		{
			// RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "AP_PS_UpdateStationPSState(): Client enter to ps for addr = ", pEntry->MacAddr);
			AP_CHNAGE_CLIENT_PS_STATE(Adapter, pEntry, TRUE);

//
// Removed by Bruce, 2013-08-22.
//	We should not disable HW queue directly when the client gets into PS mode,
//	and we should consider the PS-Poll and WMM cases.
//
#if 0 //(MULTICHANNEL_SUPPORT == 1)
			// For Win8 MultiChannel NdisTest (Trick): WFD_Concurrent_ext test: Disable GO/Client HW Queue -----
			if(Adapter == GetFirstGOPort(Adapter))
			{
				MultiChannelDisableEnableExactHwQueue(Adapter, TRUE);

				if(GetFirstClientPort(Adapter) != NULL)
				{
					MultiChannelDisableEnableExactHwQueue(GetFirstClientPort(Adapter), TRUE);
				}

				MultiChannelDumpHwQueueStatus(Adapter);
			}
			// -------------------------------------------------------------------------------------
#endif

			// Because it's a 1st packet to notify ps mode, Mark sp as ended.
			pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDED;
		}

		if(IsQoSDataFrame(posFrame->Octet))
		{
			BOOLEAN		WmmSp = FALSE;
			// Check the UP for this packet and the UAPSD info from the assoication.
			switch( GET_QOS_CTRL_WMM_UP(posFrame->Octet) )
			{
				case 0:
				case 3:
					WmmSp = GET_BE_UAPSD(pEntry->WmmStaQosInfo);
					break;
				case 1:
				case 2:
					WmmSp = GET_BK_UAPSD(pEntry->WmmStaQosInfo);
					break;
				case 4:
				case 5:
					WmmSp = GET_VI_UAPSD(pEntry->WmmStaQosInfo);
					break;
				case 6:
				case 7:
					WmmSp = GET_VO_UAPSD(pEntry->WmmStaQosInfo);
					break;
			}

			if(WmmSp && pEntry->WmmEosp == RT_STA_EOSP_STATE_ENDED)
			{
				PRT_TCB	pTcb;
				OCTET_STRING		osMpdu;
				BOOLEAN				bSupportTxFeedback = FALSE;
				
				// Acquire the Tx Spin Lock to protect the variables in pEntry,
				// because there are more than 1 functions reference these variables.
				PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

				// Check if this chip supports per tx packet feedback.
				bSupportTxFeedback = TxFeedbackCheckCurrentFunctionality(Adapter);

				if(!RTIsListEmpty(&pEntry->WmmPsQueue))
				{
					// Send out all packets queued for the STA.
					// RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "AP_PS_UpdateStationPSState(): Send out all ps packets queued for WMM addr = ", pEntry->MacAddr);
					while( !RTIsListEmpty(&pEntry->WmmPsQueue) )
					{
						pTcb = (PRT_TCB)RTRemoveHeadList(&pEntry->WmmPsQueue);
						FillOctetString(osMpdu, GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);

						// The service period is opened.
						pEntry->WmmEosp = RT_STA_EOSP_STATE_OPENED;

						if(RTIsListEmpty(&(pEntry->WmmPsQueue)))
						{
							// If this is the final packet, set the EOSP bit.
							// Note:
							//	There's a problem here that the final packet with EOSP may be sent twice in the air, because the Rx packet is processed slower than
							//	the these EOSP packets sent here. By Bruce, 2010-06-30.
							pEntry->WmmEosp = bSupportTxFeedback ? RT_STA_EOSP_STATE_ENDING : RT_STA_EOSP_STATE_ENDED; // Mark this flag so that our AP cannot send UAPSD packets.
						}


						bSupportTxFeedback = TxFeedbackInstallTxFeedbackInfoForTcb(Adapter, pTcb);
		
						if(bSupportTxFeedback)
						{
							TxFeedbackFillTxFeedbackInfoUserConfiguration(
									pTcb, 
									(pEntry->WmmEosp == RT_STA_EOSP_STATE_OPENED) ? RT_TX_FEEDBACK_ID_AP_WMM_PS_PKT : RT_TX_FEEDBACK_ID_AP_WMM_EOSP_ENDING, 
									Adapter, 
									Ap_PsTxFeedbackCallback, 
									(PVOID)pEntry
								);
						}
						else
						{
							bSupportTxFeedback = FALSE;
							if(pEntry->WmmEosp == RT_STA_EOSP_STATE_ENDING)
								pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDED;
						}

						RT_TRACE_F(COMP_AP, DBG_LOUD, ("Send buffered data with WmmEOSP = %d\n", pEntry->WmmEosp));

						// Set the more data if the EOSP ended so that notify the client we still queue packets.
						SET_80211_HDR_MORE_DATA(osMpdu.Octet, (pEntry->WmmEosp == RT_STA_EOSP_STATE_OPENED) ? 1 : 0);
						SET_QOS_CTRL_WMM_EOSP(osMpdu.Octet, (pEntry->WmmEosp == RT_STA_EOSP_STATE_OPENED) ? 0 : 1);

						rtStatus = TransmitTCB(Adapter, pTcb);
						if(RT_STATUS_SUCCESS != rtStatus && RT_STATUS_PKT_DROP != rtStatus)
							RTInsertTailListWithCnt(Get_WAIT_QUEUE(Adapter, pTcb->SpecifiedQueueID), &pTcb->List, GET_WAIT_QUEUE_CNT(Adapter,pTcb->SpecifiedQueueID));

						// If this chip supports TxFeedback, we only sends one packet here and the other packets should be
						// sent when this packet is transmitted completely.
						if(bSupportTxFeedback)
							break;

						// The SP is ended/ending.
						if(pEntry->WmmEosp != RT_STA_EOSP_STATE_OPENED)
							break;
					}
				}
				else
				{ // WMM queue is empty, send QosNullFrame with EOSP.
					PRT_TX_LOCAL_BUFFER 	pBuf;

					pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDED;
					
					if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
					{
						ConstructNullFunctionData(
							Adapter, 
							pBuf->Buffer.VirtualAddress,
							&pTcb->PacketLength,
							pEntry->MacAddr,
							TRUE,
							GET_QOS_CTRL_WMM_UP(posFrame->Octet),
							TRUE,
							FALSE);
						
						if(TxFeedbackInstallTxFeedbackInfoForTcb(Adapter, pTcb))
						{

							TxFeedbackFillTxFeedbackInfoUserConfiguration(
									pTcb, 
									RT_TX_FEEDBACK_ID_AP_WMM_EOSP_ENDING, 
									Adapter, 
									Ap_PsTxFeedbackCallback, 
									(PVOID)pEntry
								);

							pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDING;
						}

						pTcb->bTxUseDriverAssingedRate = TRUE;
						pTcb->priority = GET_QOS_CTRL_WMM_UP(posFrame->Octet);						
						MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, Adapter->MgntInfo.LowestBasicRate);

						RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "AP_PS_UpdateStationPSState(): SendQosNull of EOSP for client = ", pEntry->MacAddr);
					}
				}
				PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
			}
			else if(WmmSp && pEntry->WmmEosp != RT_STA_EOSP_STATE_ENDED)
			{
				// RT_TRACE_F(COMP_AP, DBG_LOUD, ("[WARNING] pEntry->WmmEosp = %d, do nothing\n", pEntry->WmmEosp));
			}
		}
	}
	else if(!bPowerSave && pEntry->bPowerSave)
	{ // STA is changing to active state.
		PRT_TCB	pTcb;
		RT_LIST_ENTRY	tmpList;

		// RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "AP_PS_UpdateStationPSState(): Client leave ps for addr = ", pEntry->MacAddr);

		RTInitializeListHead(&tmpList);

		// Update the TIM
//			AP_PS_FillTim(pMgntInfo);
		AP_CHNAGE_CLIENT_PS_STATE(Adapter, pEntry, FALSE);
		pEntry->WmmEosp = RT_STA_EOSP_STATE_OPENED;
//
// Removed by Bruce, 2013-08-22.
//	We should not disable HW queue directly when the client gets into PS mode,
//	and we should consider the PS-Poll and WMM cases.
//
#if 0 // (MULTICHANNEL_SUPPORT == 1)
		// For Win8 MultiChannel NdisTest (Trick): WFD_Concurrent_ext test: Enable GO/Client HW Queue -----
		if(Adapter == GetFirstGOPort(Adapter))
		{
			MultiChannelDisableEnableExactHwQueue(Adapter, FALSE);
			
			if(GetFirstClientPort(Adapter) != NULL)
			{
				MultiChannelDisableEnableExactHwQueue(GetFirstClientPort(Adapter), FALSE);
			}
			
			MultiChannelDumpHwQueueStatus(Adapter);
		}
		// -------------------------------------------------------------------------------------
#endif		

		// Send out all packet queued for the STA.
		PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
		//
		// We remove all TCBs from the PsQueue and insert into the tmp list to prevent that the TCBs may be  recurcively 
		// inserted back into the PsQueue when the client enters and leaves PS mode frequently.
		// By Bruce, 2010-06-02.
		//
		// Legacy
		while(!RTIsListEmpty(&pEntry->PsQueue))
		{
			pTcb = (PRT_TCB)RTRemoveHeadList(&pEntry->PsQueue);
			RTInsertTailList(&tmpList, &(pTcb->List));
		}
		// WMM
		while(!RTIsListEmpty(&pEntry->WmmPsQueue))
		{
			pTcb = (PRT_TCB)RTRemoveHeadList(&pEntry->WmmPsQueue);
			RTInsertTailList(&tmpList, &(pTcb->List));
		}
		// Group Packets
		if(pMgntInfo->PowerSaveStationNum == 0)
		{ // No client is in the Power Save mode, so we send all multicast packets immediately.
			while( !RTIsListEmpty(&(pMgntInfo->GroupPsQueue)) )
			{
				pTcb = (PRT_TCB)RTRemoveHeadList(&(pMgntInfo->GroupPsQueue));
				RTInsertTailList(&tmpList, &(pTcb->List));
			}
		}
		while( !RTIsListEmpty(&tmpList))
		{
			pTcb = (PRT_TCB)RTRemoveHeadList(&tmpList);
			
			if(pMgntInfo->bAPTimExtend)
			{
				// 
				// We set the TIM to force the client to wake up no matter if the queue has the packets for this client.
				// It's a workaround solution for the IPOD issue.
				//
				pEntry->FillTIMFlags = TRUE;
				pEntry->FillTIM = TRUE;
			}
			rtStatus =  TransmitTCB(Adapter, pTcb);
			if(RT_STATUS_SUCCESS != rtStatus && RT_STATUS_PKT_DROP != rtStatus)
			{
				RTInsertTailListWithCnt(Get_WAIT_QUEUE(Adapter, pTcb->SpecifiedQueueID), &pTcb->List,GET_WAIT_QUEUE_CNT(Adapter,pTcb->SpecifiedQueueID));
			}
		}		
		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
	}
	
	return pEntry;
}

//
// Description:	Queue the packet to send if necessary, o.w. send it donw.
// Assuption:	Tx spinlock is acquired.
// 2005.07.05, by rcnjko.
//
BOOLEAN
AP_PS_SendPacket(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	pu1Byte		pRaddr;

	if( pTcb->ProtocolType==RT_PROTOCOL_802_3 )
		pRaddr =  pTcb->BufferList[2].VirtualAddress;
	else
		pRaddr =  pTcb->BufferList[0].VirtualAddress+4;

	if( !MacAddr_isMulticast(pRaddr) )
	{ // Unicast.
		PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, pRaddr);	

		// Queue the unicast packet if dest STA is in power-save mode.
		if(pEntry != NULL && pEntry->bPowerSave)
		{
			RTInsertTailList(&(pEntry->PsQueue), &(pTcb->List));
			return FALSE;
		}
	}
	else
	{ // Bcst/Mcst.

		// Queue the Mcst/Bcst packet if one ore more STA is in power-save mode.
		if(pMgntInfo->PowerSaveStationNum > 0)
		{
			RTInsertTailList(&(pMgntInfo->GroupPsQueue), &(pTcb->List));
			return FALSE;
		}
	}
	pTcb->pAdapter = Adapter;
	return NicIFSendPacket(Adapter, pTcb);
}

//
// Description:	Fill TIM according to current state.
// 2005.07.06, by rcnjko.
//
VOID
AP_PS_FillTim(
	IN	PMGNT_INFO	pMgntInfo
	)
{
	PRT_WLAN_STA	pEntry = NULL;

	if(pMgntInfo->PowerSaveStationNum == 0)
	{
		pMgntInfo->Tim.Octet = (u1Byte *)pMgntInfo->TimBuf;

		// DTIM Count.
		pMgntInfo->Tim.Octet[0] = (u1Byte)pMgntInfo->mDtimCount;
		// DTIM Period.
		pMgntInfo->Tim.Octet[1] = (u1Byte)pMgntInfo->dot11DtimPeriod;
		// Bitmap Control.
		pMgntInfo->Tim.Octet[2] = 0;
		// Partial Virtual Bitmap.
		pMgntInfo->Tim.Octet[3] = 0;

		pMgntInfo->Tim.Length = 4;
	}
	else
	{
		int i;
		int MaxAID=-1,MinAID=-1;
		u1Byte ByteOffset,BitOffset,BeginByte,EndByte;

		PlatformFillMemory(pMgntInfo->TimBuf, 254, 0);
		
		// Set Virtual Bit Map for AID>0 (unicast).
		for(i = 0;i < ASSOCIATE_ENTRY_NUM; i++)
		{
			pEntry = &pMgntInfo->AsocEntry[i];

			//
			// These are the conditions the AP mode shall assemble the the Partial Virtual Bitmap.
			// (1) The entry is used and assoicated.
			// (2) The station is on power save mode
			// (3) (If any packet in any legacy qeue)
			// (4) All queues are UAPSD, in this case, the AP shall assemble if any pakcet in the WMM queues.
			//		The P2P test plan 6.1.12 is corrected so our AP can set pvb bit for the sta in WMM UAPSD.
			//		Revised by Bruce, 2012-06-14.
			// By Bruce, 2010-12-31.
			// 
			//
			if(pEntry->bUsed && pEntry->bAssociated && pEntry->bPowerSave &&
				(!RTIsListEmpty(&(pEntry->PsQueue)) ||
				(!RTIsListEmpty(&(pEntry->WmmPsQueue)) && ((pEntry->WmmStaQosInfo & 0xF) == 0xF)) || 
				pEntry->FillTIMFlags))
			{
				// RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "AP_PS_FillTim(): Fill Unicast TIM for addr = ", pEntry->MacAddr);
				ByteOffset = pEntry->AID >> 3;
				BitOffset = pEntry->AID % 8;

				pMgntInfo->TimBuf[3 + ByteOffset] |= (1<<BitOffset);

				if(MaxAID == -1 || pEntry->AID > MaxAID)
					MaxAID = pEntry->AID;

				if(MinAID == -1 || pEntry->AID < MinAID)
					MinAID = pEntry->AID;
				
				if(pEntry->FillTIM)
					pEntry->FillTIM= FALSE;
				else
					pEntry->FillTIMFlags = FALSE;
			}
		}

		// Decide Bitmap offset.
		if(MinAID == -1 || MaxAID == -1)
		{
			BeginByte = 0;
			EndByte = 0;
		}
		else
		{
			BeginByte = ((MinAID>>4)<<1);
			EndByte = (MaxAID>>3);
		}

		// Fill up TIM IE.
		pMgntInfo->Tim.Octet = (u1Byte *)(&pMgntInfo->TimBuf[BeginByte]);

		// DTIM Count.
		pMgntInfo->Tim.Octet[0] = (u1Byte)pMgntInfo->mDtimCount;
		// DTIM Period.
		pMgntInfo->Tim.Octet[1] = (u1Byte)pMgntInfo->dot11DtimPeriod;
		// Bitmap Control.
		pMgntInfo->Tim.Octet[2] = BeginByte | 
									(RTIsListEmpty(&pMgntInfo->GroupPsQueue) ? 0 : 1);

		pMgntInfo->Tim.Length = 4 + (EndByte-BeginByte);
	}
}

//
// Description:	Handle power-save poll.
// 2005.07.07, by rcnjko.
//
VOID 
AP_PS_OnPSPoll(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	osMpdu
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	pu1Byte			pSaddr = Frame_pSaddr(osMpdu);
	PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, pSaddr);
	RT_STATUS		rtStatus;
   
	if(pEntry == NULL)
		return;

	// We don't handle ps-poll with PwrMgt==TRUE, because this case is handle in 
	// AP_PS_UpdateStationPSState() before. 2005.07.07, by rcnjko.
	if(Frame_PwrMgt(osMpdu) == FALSE)
		return;

	if( !RTIsListEmpty(&(pEntry->PsQueue)) )
	{
		PRT_TCB			pTcb;
		OCTET_STRING	osPsPacket;

		RT_PRINT_ADDR(COMP_POWER, DBG_LOUD, "AP_PS_OnPSPoll(): Receive PS=Poll from client = ", pEntry->MacAddr);

		PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
		pTcb = (PRT_TCB)RTRemoveHeadList(&(pEntry->PsQueue));
		FillOctetString(osPsPacket, GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);

		// Set MoreData bit if there is one ore more packets queued for the STA.
		SET_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_PS_REPLY_PS_POLL);

		if(!RTIsListEmpty(&(pEntry->PsQueue)))
		{ // There are still packets queued in the queue, set the more data bit.
			SET_80211_HDR_MORE_DATA(osPsPacket.Octet, 1);
		}

		rtStatus =  TransmitTCB(Adapter, pTcb);
		
		if(RT_STATUS_SUCCESS != rtStatus && RT_STATUS_PKT_DROP != rtStatus)
		{
			// Insert it to tail of wait queue.
			RTInsertTailListWithCnt(Get_WAIT_QUEUE(Adapter, pTcb->SpecifiedQueueID), &pTcb->List,GET_WAIT_QUEUE_CNT(Adapter,pTcb->SpecifiedQueueID));
		}
		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
	}
	else
	{
		// 		
		// Too many packets will be sent in air, and no test plan check this.
		//		
		PRT_TCB					pTcb;
		PRT_TX_LOCAL_BUFFER 	pBuf;
		// Send Null Function Data to the Client, because we have no data buffered.
		PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

		if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
		{
			//Add bQoS parameter for QoS Null by Isaiah 2006-07-31
			ConstructNullFunctionData(
				Adapter, 
				pBuf->Buffer.VirtualAddress,
				&pTcb->PacketLength,
				pSaddr,
				FALSE,
				0,
				FALSE,
				FALSE);

			pTcb->bTxUseDriverAssingedRate = TRUE;
			pTcb->priority = 0;
			SET_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_PS_REPLY_PS_POLL);

			MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, pMgntInfo->LowestBasicRate);
		}

		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
	}	
}

//
// Description:	Forward the incoming packet to another STA assocated.
// Output:		FALSE if we failed to forward it.
// 2005.06.02, by rcnjko.
//
BOOLEAN
AP_ForwardPacketWithFromDS(
 IN PADAPTER		Adapter,
 IN PRT_RFD		pRfd,
 IN BOOLEAN		bNeedCopy
 )
{
	PMGNT_INFO    pMgntInfo = &(Adapter->MgntInfo);
	OCTET_STRING   mpdu;
	BOOLEAN     bResult = FALSE;
	PRT_TCB     pTcb = NULL;

	s4Byte maxMpduSizeToForward;
	maxMpduSizeToForward= (Adapter->MAX_TRANSMIT_BUFFER_SIZE - ENCRYPTION_MAX_OVERHEAD - Adapter->TXPacketShiftBytes);

	FillOctetString(mpdu, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

	// Note!, we should check buffer length before copy and forward it.
	if(mpdu.Length > maxMpduSizeToForward)
	{
		// We cannot forward this packet because the mpdu recived is large than 
		// our transmit buffer size. 2005.06.03, by rcnjko.
		return FALSE;
	}

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	// Set up TCB and its buffer.
	if(bNeedCopy == FALSE)
	{ // case 1: Reuse the buffer in RFD.

		if(!RTIsListEmpty(GET_TCB_IDLE_QUEUE(Adapter)))
		{
			pTcb = (PRT_TCB)RTRemoveHeadListWithCnt(GET_TCB_IDLE_QUEUE(Adapter), Get_NUM_IDLE_TCB(Adapter));

			// Set up TCB.
			pTcb->BufferList[0].VirtualAddress = pRfd->Buffer.VirtualAddress;
			pTcb->BufferList[0].PhysicalAddressHigh = pRfd->Buffer.PhysicalAddressHigh;
			pTcb->BufferList[0].PhysicalAddressLow = pRfd->Buffer.PhysicalAddressLow;

			// Get shifted bytes of Starting address of 802.11 header. 2006.09.28, by Emily	
			pTcb->BufferList[0].PhysicalAddressLow +=Adapter->HalFunc.GetRxPacketShiftBytesHandler(pRfd);
			pTcb->BufferList[0].Length = pRfd->PacketLength;
			pTcb->PacketLength = pRfd->PacketLength;
			pTcb->BufferCount = 1;
			pTcb->Tailer.Length = 0;
			pTcb->ProtocolType = RT_PROTOCOL_802_11;
			pTcb->DataRate = UNSPECIFIED_DATA_RATE;
			pTcb->SpecifiedQueueID = (IsQoSDataFrame(mpdu.Octet)?Frame_QoSTID(mpdu, sMacHdrLng):0);
			pTcb->priority = (IsQoSDataFrame(mpdu.Octet)?Frame_QoSTID(mpdu, sMacHdrLng):0);
			pTcb->FragCount=1;

			pTcb->BufferType = RT_TCB_BUFFER_TYPE_RFD;
			pTcb->Reserved = pRfd;
			pTcb->bInsert8BytesForEarlyMode=FALSE;
			bResult = TRUE;
		}
		else
		{ // No TCB available now.
			RT_TRACE(COMP_INIT, DBG_LOUD, ("AP_ForwardPacketWithFromDS(): Out of TCB !!!\n"));
		}
	}
	else
	{ // case 2: Get a local buffer to forwarding the packet.

		PRT_TX_LOCAL_BUFFER  pBuf;

		u2Byte     Packetleng;
		OCTET_STRING   mpduTcb;

		if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
		{
			// Copy the incoming frame to local buffer.
			if( IsQoSDataFrame(mpdu.Octet ) && MacAddr_isMulticast(Frame_pDaddr(mpdu)))
			{ // we need to remove Qos fild for muticast 
				pu1Byte pOrData = pBuf->Buffer.VirtualAddress;
				pu1Byte pDsData = mpdu.Octet;

				// Copy Head !!
				PlatformMoveMemory( pOrData , pDsData , sMacHdrLng );
				pOrData = pOrData + sMacHdrLng ;  
				pDsData = pDsData + sMacHdrLng + sQoSCtlLng ;

				// Copy payload !!
				PlatformMoveMemory(pOrData , pDsData , ( mpdu.Length - sMacHdrLng - sQoSCtlLng ) );

				// Clear Qos bite !!
				SET_80211_HDR_QOS_EN( pBuf->Buffer.VirtualAddress , 0  );

				// Set Packet Length 
				Packetleng = pRfd->PacketLength - sQoSCtlLng;
			}
			else
			{
			
				PlatformMoveMemory(pBuf->Buffer.VirtualAddress, mpdu.Octet, mpdu.Length);

				Packetleng = pRfd->PacketLength;
			}

			FillOctetString(mpduTcb , pBuf->Buffer.VirtualAddress , Packetleng);

			// Set up TCB.
			pTcb->BufferList[0] = pBuf->Buffer;
			pTcb->BufferList[0].Length = Packetleng; //pRfd->PacketLength;
			pTcb->PacketLength = Packetleng; //pRfd->PacketLength;
			pTcb->BufferCount = 1;
			pTcb->Tailer.Length = 0;
			pTcb->ProtocolType  =RT_PROTOCOL_802_11;
			pTcb->SpecifiedQueueID = (IsQoSDataFrame(mpduTcb.Octet)?Frame_QoSTID(mpduTcb, sMacHdrLng):0);
			pTcb->DataRate = UNSPECIFIED_DATA_RATE;
			pTcb->FragCount=1;

			pTcb->BufferType = RT_TCB_BUFFER_TYPE_LOCAL;
			pTcb->Reserved = pBuf;
			pTcb->bInsert8BytesForEarlyMode=FALSE;
                     
			bResult = TRUE;
		}
		else
		{
			RT_TRACE(COMP_AP, DBG_LOUD, ("AP_ForwardPacketWithFromDS(): MgntGetBuffer() return FALSE!!!\n"));
		}
	}

	if(bResult)
	{
		u1Byte SrcAddr[6];
		u1Byte DstAddr[6];
		pu1Byte pHeader = NULL;

		// Back up SA/DA addresses.
		PlatformMoveMemory(SrcAddr, Frame_pSaddr(mpdu), 6);
		PlatformMoveMemory(DstAddr, Frame_pDaddr(mpdu), 6);
		//RT_TRACE(COMP_INIT, DBG_LOUD, (">>>------------------------------------\n"));
		//RT_PRINT_ADDR(COMP_INIT, DBG_LOUD, ("DA"), DstAddr);
		//RT_PRINT_ADDR(COMP_INIT, DBG_LOUD, ("BSSID"), pMgntInfo->Bssid);
		//RT_PRINT_ADDR(COMP_INIT, DBG_LOUD, ("SA"), SrcAddr);
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("<<<------------------------------------\n"));
		pHeader = pTcb->BufferList[0].VirtualAddress;

		// Clear Flag
		SET_80211_HDR_MORE_FRAG(pHeader, 0);
		SET_80211_HDR_RETRY(pHeader, 0);
		SET_80211_HDR_PWR_MGNT(pHeader, 0);
		SET_80211_HDR_MORE_DATA(pHeader, 0);
		SET_80211_HDR_WEP(pHeader, 0);
		SET_80211_HDR_ORDER(pHeader, 0);

		// Change addresses (DA/BSSID/SA).
		SET_80211_HDR_ADDRESS1(pHeader, DstAddr); // DA
		SET_80211_HDR_ADDRESS2(pHeader, pMgntInfo->Bssid);// BSSID
		SET_80211_HDR_ADDRESS3(pHeader, SrcAddr);// SA

		// Changge FrDs/ToDs in Frame Control field.
		SET_80211_HDR_FROM_DS(pHeader, 1);
		SET_80211_HDR_TO_DS(pHeader, 0);

		// <RJ_NOTE> Duration and Sequence control will be set up in TransmitTcb().
		SET_80211_HDR_DURATION(pHeader, 0); 
		SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);

		//RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("Header \n"), pHeader, 32);
		// Forward it.
		// <NOTE> AP_PS_SendPacket() will queue the packet to send in proper queue 
		// if necessary, 2005.07.05, by rcnjko.
		pTcb->pAdapter = Adapter;
		NicIFSendPacket(Adapter, pTcb);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	return bResult;
}


//
//	Description: 
//		Translate and forward the packet received from WDS AP to STA associated.
//	2006.06.11, by rcnjko.
//
BOOLEAN
AP_FromWdsToBss(
	IN	PADAPTER		Adapter, 
	IN	PRT_RFD			pRfd,
	IN	BOOLEAN			bNeedCopy
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	OCTET_STRING			mpdu;
	BOOLEAN					bResult = FALSE;
	PRT_TCB					pTcb = NULL;
	u2Byte					WdsFrameHdrLng;
	u2Byte					BssFrameHdrLng;

	s4Byte			maxMpduSizeToForward;
	maxMpduSizeToForward = (Adapter->MAX_TRANSMIT_BUFFER_SIZE - ENCRYPTION_MAX_OVERHEAD - Adapter->TXPacketShiftBytes);

	FillOctetString(mpdu, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

	// Note!, we should check buffer length before copy and forward it.
	if(mpdu.Length > maxMpduSizeToForward)
	{
		// We cannot forward this packet because the mpdu recived is larger than 
		// our transmit buffer size.
		return bResult;
	}

	// <RJ_TODO_WDS> Consider WDS frame with Security/QoS case.
	WdsFrameHdrLng = Frame_FrameHdrLng(mpdu);

	// TODO: consider per-STA encryption algorithm and QoS.
	BssFrameHdrLng = sMacHdrLng + pMgntInfo->SecurityInfo.EncryptionHeadOverhead;
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(bNeedCopy == FALSE)
	{ // case 1: Reuse the buffer in RFD. 
		//
		// Note that, in this case:
		// pTcb->BufferList[0]: 802.11 Header without Addr4 (including QoS and Security), 
		// which references to pTcb->Header[0].
		// pTcb->BufferList[1]: 802.11 frame body (begine with LLC), 
		// which reuses pRfd->Buffer with proper offset. 
		// 2006.06.11, by rcnjko.
		//

		if(!RTIsListEmpty(GET_TCB_IDLE_QUEUE(Adapter)))
		{
			pTcb = (PRT_TCB)RTRemoveHeadList(GET_TCB_IDLE_QUEUE(Adapter));

			// Set up TCB.
			pTcb->Header[0].Length = BssFrameHdrLng;
			pTcb->BufferList[0] = pTcb->Header[0];

			pTcb->BufferList[1].VirtualAddress = (pRfd->Buffer.VirtualAddress + WdsFrameHdrLng);
			pTcb->BufferList[1].Length = (pRfd->PacketLength - WdsFrameHdrLng);

			pTcb->PacketLength = pTcb->BufferList[0].Length +  pTcb->BufferList[1].Length;
			pTcb->BufferCount = 2;
			pTcb->Tailer.Length = 0;
			pTcb->ProtocolType = RT_PROTOCOL_802_11;
			pTcb->DataRate = UNSPECIFIED_DATA_RATE;
			pTcb->SpecifiedQueueID = LOW_QUEUE;
			pTcb->FragCount = 1;

			pTcb->BufferType = RT_TCB_BUFFER_TYPE_RFD;
			pTcb->Reserved = pRfd;
			pTcb->bInsert8BytesForEarlyMode=FALSE;
			bResult = TRUE;
		}
		else
		{ // No TCB available now.
			RT_TRACE(COMP_AP, DBG_WARNING, ("AP_FromWdsToBss(): Out of TCB !!!\n"));
		}
	}
	else
	{ // case 2: Get a local buffer to forwarding the packet.

		PRT_TX_LOCAL_BUFFER 	pBuf;

		if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
		{
			// Copy the incoming frame to local buffer with proper offset.
			PlatformMoveMemory(
				pBuf->Buffer.VirtualAddress + BssFrameHdrLng, 
				mpdu.Octet + WdsFrameHdrLng, 
				mpdu.Length - WdsFrameHdrLng);

			// Set up TCB.
			pTcb->BufferList[0] = pBuf->Buffer;
			pTcb->BufferList[0].Length = (pRfd->PacketLength + BssFrameHdrLng - WdsFrameHdrLng);

			pTcb->PacketLength = pTcb->BufferList[0].Length;
			pTcb->BufferCount = 1;
			pTcb->Tailer.Length = 0;
			pTcb->ProtocolType  =RT_PROTOCOL_802_11;
			pTcb->SpecifiedQueueID = LOW_QUEUE;
			pTcb->DataRate = UNSPECIFIED_DATA_RATE;
			pTcb->FragCount=1;

			pTcb->BufferType = RT_TCB_BUFFER_TYPE_LOCAL;
			pTcb->Reserved = pBuf;

			bResult = TRUE;
		}
		else
		{
			RT_TRACE(COMP_AP, DBG_LOUD, ("AP_FromWdsToBss(): MgntGetBuffer() return FALSE!!!\n"));
		}
	}

	if(bResult)
	{
		u1Byte	SrcAddr[6];
		u1Byte	DstAddr[6];
		pu1Byte	pHeader = NULL;
	
		// Back up SA/DA addresses.
		PlatformMoveMemory(SrcAddr, Frame_pSaddr(mpdu), 6);
		PlatformMoveMemory(DstAddr, Frame_pDaddr(mpdu), 6);
	
		pHeader = pTcb->BufferList[0].VirtualAddress;
	
		// Clear Flag
		SET_80211_HDR_MORE_FRAG(pHeader, 0);
		SET_80211_HDR_RETRY(pHeader, 0);
		SET_80211_HDR_PWR_MGNT(pHeader, 0);
		SET_80211_HDR_MORE_DATA(pHeader, 0);
		SET_80211_HDR_WEP(pHeader, 0);
		SET_80211_HDR_ORDER(pHeader, 0);
	
		// Frame Control field.
		SET_80211_HDR_FRAME_CONTROL(pHeader, 0); // clear all flags before set
		SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
		SET_80211_HDR_FROM_DS(pHeader, 1);

		// Change addresses (DA/BSSID/SA).
		SET_80211_HDR_ADDRESS1(pHeader, DstAddr); // DA
		SET_80211_HDR_ADDRESS2(pHeader, pMgntInfo->Bssid); // BSSID
		SET_80211_HDR_ADDRESS3(pHeader, SrcAddr); // SA
	
		// <RJ_NOTE> Duration and Sequence control will be set up in TransmitTcb().
		SET_80211_HDR_DURATION(pHeader, 0);
		SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);

		RT_PRINT_DATA(COMP_AP, DBG_TRACE, ("AP_FromWdsToBss(): "), pTcb->BufferList[0].VirtualAddress, pTcb->BufferList[0].Length);
	
		// Forward it.
		// <NOTE> AP_PS_SendPacket() will queue the packet to send in proper queue 
		// if necessary, 2005.07.05, by rcnjko.
		pTcb->pAdapter = Adapter;
		NicIFSendPacket(Adapter, pTcb);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	return bResult;
}

//
//	Description: 
//		Translate and forward the packet received from STA associated to WDS AP.
//	2006.06.11, by rcnjko.
//
BOOLEAN
AP_FromBssToWds(
	IN	PADAPTER		Adapter, 
	IN	PRT_RFD			pRfd,
	IN	BOOLEAN			bNeedCopy
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	OCTET_STRING			mpdu;
	BOOLEAN					bResult = FALSE;
	PRT_TCB					pTcb = NULL;
	u2Byte					WdsFrameHdrLng;
	u2Byte					BssFrameHdrLng;

	s4Byte			maxMpduSizeToForward;
	maxMpduSizeToForward = (Adapter->MAX_TRANSMIT_BUFFER_SIZE - ENCRYPTION_MAX_OVERHEAD - Adapter->TXPacketShiftBytes);

	FillOctetString(mpdu, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

	// Note!, we should check buffer length before copy and forward it.
	if(mpdu.Length > maxMpduSizeToForward)
	{
		// We cannot forward this packet because the mpdu recived is larger than 
		// our transmit buffer size.
		return bResult;
	}

	// <RJ_TODO_WDS> Consider WDS frame with Security/QoS case.
	WdsFrameHdrLng = 30;

	// TODO: consider per-STA encryption algorithm.
	BssFrameHdrLng = Frame_FrameHdrLng(mpdu) + pMgntInfo->SecurityInfo.EncryptionHeadOverhead;
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(bNeedCopy == FALSE)
	{ // case 1: Reuse the buffer in RFD. 
		//
		// Note that, in this case:
		// pTcb->BufferList[0]: 802.11 Header with Addr4 (including QoS and Security), 
		// which references to pTcb->Header[0].
		// pTcb->BufferList[1]: 802.11 frame body (begine with LLC), 
		// which reuses pRfd->Buffer with proper offset. 
		// 2006.06.11, by rcnjko.
		//

		if(!RTIsListEmpty(GET_TCB_IDLE_QUEUE(Adapter)))
		{
			pTcb = (PRT_TCB)RTRemoveHeadList(GET_TCB_IDLE_QUEUE(Adapter));

			// Set up TCB.
			pTcb->Header[0].Length = WdsFrameHdrLng;
			pTcb->BufferList[0] = pTcb->Header[0];

			pTcb->BufferList[1].VirtualAddress = (pRfd->Buffer.VirtualAddress + BssFrameHdrLng);
			pTcb->BufferList[1].Length = (pRfd->PacketLength - BssFrameHdrLng);

			pTcb->PacketLength = pTcb->BufferList[0].Length +  pTcb->BufferList[1].Length;
			pTcb->BufferCount = 2;
			pTcb->Tailer.Length = 0;
			pTcb->ProtocolType = RT_PROTOCOL_802_11;
			pTcb->DataRate = UNSPECIFIED_DATA_RATE;
			pTcb->SpecifiedQueueID = LOW_QUEUE;
			pTcb->FragCount = 1;

			pTcb->BufferType = RT_TCB_BUFFER_TYPE_RFD;
			pTcb->Reserved = pRfd;
			pTcb->bInsert8BytesForEarlyMode=FALSE;
			bResult = TRUE;
		}
		else
		{ // No TCB available now.
			RT_TRACE(COMP_AP, DBG_WARNING, ("AP_FromBssToWds(): Out of TCB !!!\n"));
		}
	}
	else
	{ // case 2: Get a local buffer to forwarding the packet.

		PRT_TX_LOCAL_BUFFER 	pBuf;

		if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
		{
			// Copy the incoming frame to local buffer with proper offset.
			PlatformMoveMemory(
				pBuf->Buffer.VirtualAddress + WdsFrameHdrLng, 
				mpdu.Octet + BssFrameHdrLng, 
				mpdu.Length - BssFrameHdrLng);

			// Set up TCB.
			pTcb->BufferList[0] = pBuf->Buffer;
			pTcb->BufferList[0].Length = (pRfd->PacketLength + WdsFrameHdrLng - BssFrameHdrLng);

			pTcb->PacketLength = pTcb->BufferList[0].Length;
			pTcb->BufferCount = 1;
			pTcb->Tailer.Length = 0;
			pTcb->ProtocolType = RT_PROTOCOL_802_11;
			pTcb->SpecifiedQueueID = LOW_QUEUE;
			pTcb->DataRate = UNSPECIFIED_DATA_RATE;
			pTcb->FragCount = 1;

			pTcb->BufferType = RT_TCB_BUFFER_TYPE_LOCAL;
			pTcb->Reserved = pBuf;

			bResult = TRUE;
		}
		else
		{
			RT_TRACE(COMP_AP, DBG_LOUD, ("AP_FromBssToWds(): MgntGetBuffer() return FALSE!!!\n"));
		}
	}

	if(bResult)
	{
		u1Byte	SrcAddr[6];
		u1Byte	DstAddr[6];
		pu1Byte	pHeader;
	
		// Back up SA/DA addresses.
		PlatformMoveMemory(SrcAddr, Frame_pSaddr(mpdu), 6);
		PlatformMoveMemory(DstAddr, Frame_pDaddr(mpdu), 6);
	
		pHeader = pTcb->BufferList[0].VirtualAddress;
	
		// Frame Control field.
		// Clear Flag
		SET_80211_HDR_MORE_FRAG(pHeader, 0);
		SET_80211_HDR_RETRY(pHeader, 0);
		SET_80211_HDR_PWR_MGNT(pHeader, 0);
		SET_80211_HDR_MORE_DATA(pHeader, 0);
		SET_80211_HDR_WEP(pHeader, 0);
		SET_80211_HDR_ORDER(pHeader, 0);
		
		SET_80211_HDR_FRAME_CONTROL(pHeader, 0); // clear all flags before set
		SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
		SET_80211_HDR_FROM_DS(pHeader, 1);
		SET_80211_HDR_TO_DS(pHeader, 1);

		// Change addresses (RA/TA/DA/SA).
		SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->WdsApAddr); // RA
		SET_80211_HDR_ADDRESS2(pHeader, pMgntInfo->Bssid); // TA
		SET_80211_HDR_ADDRESS3(pHeader, DstAddr); // DA
		SET_80211_HDR_ADDRESS4(pHeader, SrcAddr); // SA
	
		// <RJ_NOTE> Duration and Sequence control will be set up in TransmitTcb().
		SET_80211_HDR_DURATION(pHeader, 0);
		SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);

		RT_PRINT_DATA(COMP_AP, DBG_TRACE, ("AP_FromBssToWds(): "), pTcb->BufferList[0].VirtualAddress, pTcb->BufferList[0].Length);
	
		// Forward it.
		// <NOTE> AP_PS_SendPacket() will queue the packet to send in proper queue 
		// if necessary, 2005.07.05, by rcnjko.
		pTcb->pAdapter = Adapter;
		NicIFSendPacket(Adapter, pTcb);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	return bResult;
}

//
//	Description:
//		Translate header or send packet to to WDS.
//
//	Assumption:
//		1. We are in AP mode and WDS is enabled.	
//		2. pTcb contains 802.11 Data frame from us.
//		3. TX spinlock is acquired.
//		4. Usage of pTcb->BufferList[]: 
//			pTcb->BufferList[0]: 802.11 header (including QoS and Security header).
//			pTcb->BufferList[1]: LLC.
//			pTcb->BufferList[2-n]: frame body (starting from TypeLength).
//
//	2006.06.11, by rcnjko. 
//
VOID
AP_LocalToWds(
	IN	PADAPTER		Adapter, 
	IN	PRT_TCB			pTcbSrc,
	IN	BOOLEAN			bCopyAndForward
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcbToWds = NULL;

	//
	// Set up pTcbToWds.
	//
	if(bCopyAndForward == FALSE)
	{
		pTcbToWds = pTcbSrc;

		// Change 802.11 header length and related.
		// <RJ_TODO_WDS> Consider WDS frame with Security/QoS case.
		pTcbToWds->PacketLength -= pTcbToWds->BufferList[0].Length; 
		pTcbToWds->BufferList[0].Length = 30;
		pTcbToWds->PacketLength += pTcbToWds->BufferList[0].Length; 
	}
	else
	{ // Copy frame to local buffer.
		PRT_TX_LOCAL_BUFFER pBuf;
		u2Byte idx;
		u4Byte BytesCopied;

		if(MgntGetBuffer(Adapter, &pTcbToWds, &pBuf))
		{
			// Copy the incoming frame to local buffer with proper offset.
			// <RJ_TODO_WDS> Consider WDS frame with Security/QoS case.
			BytesCopied = 30;
			for(idx = 1; idx < pTcbSrc->BufferCount; idx++)
			{
				PlatformMoveMemory(
					pBuf->Buffer.VirtualAddress + BytesCopied, 
					pTcbSrc->BufferList[idx].VirtualAddress, 
					pTcbSrc->BufferList[idx].Length);

				BytesCopied += pTcbSrc->BufferList[idx].Length;
			}
	
			// Set up TCB.
			pTcbToWds->BufferList[0] = pBuf->Buffer;
			pTcbToWds->BufferList[0].Length = BytesCopied; 

			pTcbToWds->PacketLength = pTcbToWds->BufferList[0].Length;
			pTcbToWds->BufferCount = 1;
			pTcbToWds->Tailer.Length = 0;
			pTcbToWds->ProtocolType = RT_PROTOCOL_802_11;
			pTcbToWds->SpecifiedQueueID = LOW_QUEUE;
			pTcbToWds->DataRate = UNSPECIFIED_DATA_RATE;
			pTcbToWds->FragCount = 1;

			pTcbToWds->BufferType = RT_TCB_BUFFER_TYPE_LOCAL;
			pTcbToWds->Reserved = pBuf;

			PlatformMoveMemory( pTcbToWds->DestinationAddress, pTcbSrc->DestinationAddress, 6 ); // DA
			PlatformMoveMemory( pTcbToWds->SourceAddress, pTcbSrc->SourceAddress, 6 ); // SA
		}
		else
		{
			RT_TRACE(COMP_AP, DBG_LOUD, ("AP_LocalToWds(): MgntGetBuffer() return FALSE!!!\n"));
		}
	}

	if(pTcbToWds != NULL)
	{
		pu1Byte		pHeader;

		//
		// Translate header.
		//
		pHeader = pTcbToWds->BufferList[0].VirtualAddress;

		// Clear Flag
		SET_80211_HDR_MORE_FRAG(pHeader, 0);
		SET_80211_HDR_RETRY(pHeader, 0);
		SET_80211_HDR_PWR_MGNT(pHeader, 0);
		SET_80211_HDR_MORE_DATA(pHeader, 0);
		SET_80211_HDR_WEP(pHeader, 0);
		SET_80211_HDR_ORDER(pHeader, 0);

		// Frame Control field.
		SET_80211_HDR_FRAME_CONTROL(pHeader, 0);// clear all flags before set
		SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
		SET_80211_HDR_FROM_DS(pHeader, 1);
		SET_80211_HDR_TO_DS(pHeader, 1);	

		// Change addresses (RA/TA/DA/SA).
		SET_80211_HDR_ADDRESS1(pHeader, pMgntInfo->WdsApAddr);
		SET_80211_HDR_ADDRESS2(pHeader, pMgntInfo->Bssid);
		SET_80211_HDR_ADDRESS3(pHeader, pTcbToWds->DestinationAddress);
		SET_80211_HDR_ADDRESS4(pHeader, pTcbToWds->SourceAddress);

		// <RJ_NOTE> Duration and Sequence control will be set up in TransmitTcb().
		SET_80211_HDR_DURATION(pHeader, 0);
		SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);

		RT_PRINT_DATA(COMP_AP, DBG_TRACE, ("AP_LocalToWds(): "), pTcbToWds->BufferList[0].VirtualAddress, pTcbToWds->BufferList[0].Length);

		//
		// Forward it if required.
		//
		if(bCopyAndForward)
		{
			// Forward it.
			// <NOTE> AP_PS_SendPacket() will queue the packet to send in proper queue 
			// if necessary, 2005.07.05, by rcnjko.
			pTcbToWds->pAdapter = Adapter;
			NicIFSendPacket(Adapter, pTcbToWds);
		}
	}

}

//
//	Description:
//		Dispatch the data frame to WDS if necessary.
//
//	Assumption:
//		1. We are in AP mode and WDS is enabled.	
//		2. pTcb contains 802.11 Data frame from us.
//		3. TX spinlock is acquired.
//
//	2006.06.11, by rcnjko. 
//
VOID
AP_WdsTx(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB			pTcb)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);	

	if(MacAddr_isMulticast(pTcb->DestinationAddress))
	{ // Mcst/Bcst destination => forward one copy to WDS AP. 
		AP_LocalToWds(Adapter, pTcb, TRUE);
	}
	else
	{ // unicast destination.

		if(!AsocEntry_IsStationAssociated(pMgntInfo, pTcb->DestinationAddress))
		{ // Translate 802.11 header for WDS for unicast destination not in the BSS.
			AP_LocalToWds(Adapter, pTcb, FALSE);
		}
	}
}


//
// Description:	Handle incoming Authentication frames with odd arSeq.
// Output:		Return TRUE if we will response the incoming auth frame, FALSE o.w.. 
//
// 2005.06.01, by rcnjko.
//
BOOLEAN
AP_OnAuthOdd(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	authpdu)
{
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO 		pDefMgntInfo = GetDefaultMgntInfo(Adapter);
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	u1Byte			AuthStatusCode;
	OCTET_STRING	AuthChallengetext;
	pu1Byte			arSta;
	AUTH_ALGORITHM	arAlg;
	u2Byte			arSeq;
	u4Byte			idx;
	BOOLEAN			allowed;
	PRT_WLAN_STA	pEntry = NULL;
	BOOLEAN		bDuplicateAuth = FALSE;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===>AP_OnAuthOdd()\n"));
	if(!pDefaultAdapter->bInHctTest && pDefMgntInfo->Regbcndelay)
	{
		if( pMgntInfo->bDelayApBeaconMode == TRUE )
		{
			RT_TRACE(COMP_AP, DBG_LOUD, ("Stop beacon and reject Auth\n"));

			return FALSE;
		}
	}
	// Check Bssid.
	if( PlatformCompareMemory( pMgntInfo->Bssid, Frame_Addr3( authpdu ), 6 ) != 0 )
	{
		return FALSE;
	}

	if(pMgntInfo->LockType == MAC_FILTER_ACCEPT)
	{
		if( pMgntInfo->LockedSTACount != 0 )
		{
			allowed = FALSE;
			for( idx=0; idx<pMgntInfo->LockedSTACount; idx++ )
			{
				if( eqMacAddr(pMgntInfo->LockedSTAList[idx], Frame_Addr2(authpdu)) )
				{
					RT_PRINT_ADDR( COMP_AP, DBG_LOUD, ("Mached Locked STA Address"), pMgntInfo->LockedSTAList[idx] );
					allowed = TRUE;
					break;
				}
			}

			if( !allowed )
			{
				RT_TRACE( COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): unknow STA MAC address!\n") );
				return FALSE;
			}
		}
		else if(pMgntInfo->bDefaultPermited)
		{
			allowed = TRUE;
		}
		else
			return FALSE;		
	}
	else if(pMgntInfo->LockType == MAC_FILTER_REJECT)
	{
		if( pMgntInfo->LockedSTACount_Reject!= 0 )
		{
			allowed = TRUE;
			for( idx=0; idx<pMgntInfo->LockedSTACount_Reject; idx++ )
			{
				if( eqMacAddr(pMgntInfo->LockedSTAList_Reject[idx], Frame_Addr2(authpdu)) )
				{
					RT_PRINT_ADDR( COMP_AP, DBG_LOUD, ("Mached Locked STA Address"), pMgntInfo->LockedSTAList_Reject[idx] );
					allowed = FALSE;
					break;
				}
			}

			if( !allowed )
			{
				RT_TRACE( COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): Reject this  STA MAC address!\n") );
				return FALSE;
			}
		}
		else if(!pMgntInfo->bDefaultPermited)
		{
			return FALSE;
		}
	}
	else if(pMgntInfo->LockType == MAC_FILTER_LOCK)
	{
		if( pMgntInfo->LockedSTACount != 0 )
		{
			allowed = FALSE;

			for( idx=0; idx<pMgntInfo->LockedSTACount; idx++ )
			{
				if( eqMacAddr(pMgntInfo->LockedSTAList[idx], Frame_Addr2(authpdu)) )
				{
					RT_PRINT_ADDR( COMP_AP, DBG_LOUD, ("Mached Locked STA Address"), pMgntInfo->LockedSTAList[idx] );
					allowed = TRUE;
					break;
				}
			}

			if( !allowed )
			{
				RT_TRACE( COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): unknow STA MAC address!\n") );
				return FALSE;
			}
		}	
	}

#if 1 //Added by Jay 0713 for processing integrity failure
	
	// Refuse all Authentication Requests within 60 sec after Integrity Failure event happened
	if(pMgntInfo->globalKeyInfo.TimeSlot_IntegrityFail2 != 0 &&
		pMgntInfo->globalKeyInfo.CurrentTimeSlot < pMgntInfo->globalKeyInfo.TimeSlot_IntegrityFail2 + 60)
	{
		return FALSE;
	}
#endif	

	// Initialize default value for Auth to resonse.
	AuthStatusCode = StatusCode_Unspecified_failure;
	FillOctetString(AuthChallengetext, NULL, 0);
	
	// Gather information from incoming Auth frame.
	arSta = Frame_Addr2(authpdu); 
	arAlg = (AUTH_ALGORITHM)Frame_AuthAlgorithmNum(authpdu);
	arSeq = Frame_AuthTransactionSeqNum(authpdu);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("AP_OnAuthOdd(): STA: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n", arSta[0], arSta[1], arSta[2], arSta[3], arSta[4], arSta[5]));
	
	// Classify with arSeq. 
	switch(arSeq)
	{
		case 1:
			// Classify with arAlg. 
			switch(arAlg)
			{
			case OPEN_SYSTEM:
				{
					pEntry = AsocEntry_GetEntry(pMgntInfo, arSta);
					if(pEntry != NULL)
					{
						RT_TRACE( COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): pEntry != NULL\n") );
						if(pEntry->bAssociated)
						{

							//
							// <Roger_Notes> We should complete all SendNBLs before NDIS_STATUS_DOT11_DISASSOCIATION status is indicated
							// to prevent MiniportReturnNetBufferLists routine will be called in the same thread context and Rx deadlock in N6PciReturnNetBufferLists
							// routine might cause bug check code DPC_WATCHDOG_VIOLATION (133).
							// 2013.06.26.
							//
							AsocEntry_BecomeDisassoc(Adapter, pEntry); 
							MacIdDeregisterSpecificMacId(Adapter, pEntry->AssociatedMacId);
							
							RT_TRACE( COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): pEntry->bAssociated\n") );
							if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED 
							 || MgntActQuery_ApType(Adapter) == RT_AP_TYPE_LINUX)
							{
								MgntUpdateAsocInfo(Adapter, UpdateDeauthAddr, pEntry->MacAddr, 6); 
								DrvIFIndicateDisassociation(Adapter, unspec_reason, pEntry->MacAddr);
							}
							else if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP)
							{
								AuthStatusCode = StatusCode_success;
								pMgntInfo->pCurrentSta = pEntry;
								DrvIFIndicateDisassociation(Adapter, unspec_reason, pEntry->MacAddr);
							}
						}
						else if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP)
						{
							bDuplicateAuth = TRUE;							
							AuthStatusCode = StatusCode_success;
							pMgntInfo->pCurrentSta = pEntry;
							DrvIFIndicateIncommingAssociationComplete(Adapter, unspec_reason);
						}
						
						if(!bDuplicateAuth)
						{
							RT_TRACE( COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): AsocEntry_RemoveStation\n") );
							AsocEntry_RemoveStation(Adapter, arSta);
						}
					}

					if(!bDuplicateAuth)
					{
						if(!AsocEntry_AddStation(Adapter, arSta, arAlg))
						{
							RT_TRACE( COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): StatusCode_assoc_deniedbyap\n") );
							AuthStatusCode = StatusCode_assoc_deniedbyap; 
							break;
						}
					}

					if(	pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeAutoSwitch ||
						pMgntInfo->AuthReq_auAlg == OPEN_SYSTEM)
					{
						if(!bDuplicateAuth)
						pEntry = AsocEntry_GetEntry(pMgntInfo, arSta);

						pEntry->AuthPassSeq = 2;
						AuthStatusCode = StatusCode_success; 
					}
					else
					{
						AuthStatusCode = StatusCode_notsupport_authalg;
					}
				}
				break;

			case SHARED_KEY:
				{
					pEntry = AsocEntry_GetEntry(pMgntInfo, arSta);
					if(pEntry != NULL)
					{
						if(pEntry->bAssociated)
						{
							if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED
							 || MgntActQuery_ApType(Adapter) == RT_AP_TYPE_LINUX)
							{
								MgntUpdateAsocInfo(Adapter, UpdateDeauthAddr, pEntry->MacAddr, 6); 
								DrvIFIndicateDisassociation(Adapter, unspec_reason, pEntry->MacAddr);
							}
							else if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP)
							{
								// TODO: WIN7
							}
						}
						RT_TRACE_F(COMP_AP, DBG_TRACE, ("AsocEntry_RemoveStation case 2\n"));

						AsocEntry_RemoveStation(Adapter, arSta);
					}

					if(!AsocEntry_AddStation(Adapter, arSta, arAlg))
					{
						AuthStatusCode = StatusCode_assoc_deniedbyap; 
						break;
					}

					if(	pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeAutoSwitch ||
						pMgntInfo->AuthReq_auAlg == SHARED_KEY)
					{
						pEntry = AsocEntry_GetEntry(pMgntInfo, arSta);
	
						pEntry->AuthPassSeq = 2;

						// <RJ_TODO_AP> We shall keep challenge text in each STA, and randomize them.
						FillOctetString(AuthChallengetext, pMgntInfo->arChalng, 128);
						AuthStatusCode = StatusCode_success;
					}
					else
					{
						AuthStatusCode = StatusCode_notsupport_authalg;
					}
				}
				break;

			default:
				RT_TRACE(COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): Illegal AuthAlg: %d for AuthSeq: %d !!!\n", arAlg, arSeq));
				AuthStatusCode = StatusCode_notsupport_authalg;
				break;
			}
			break;

		case 3:
			{
				if(	arAlg == SHARED_KEY && 
					(pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeAutoSwitch ||
					 pMgntInfo->AuthReq_auAlg == SHARED_KEY) )
				{
					pEntry = AsocEntry_GetEntry(pMgntInfo, arSta);

					if(pEntry != NULL)
					{
						if( pEntry->bAssociated == FALSE && pEntry->AuthAlg == SHARED_KEY && pEntry->AuthPassSeq == 2 )
						{
							OCTET_STRING	OurCText, PeerCText;
									
							FillOctetString(OurCText, pMgntInfo->arChalng, 128);
							PeerCText = PacketGetElement(authpdu, EID_Ctext, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE); 

							// Compare the challenge text.
							if(EqualOS( OurCText, PeerCText))
							{ // Matched.
								pEntry->AuthPassSeq = 4;
								AsocEntry_UpdateTimeStamp(pEntry);

								AuthStatusCode = StatusCode_success;
							}
							else
							{ // Challenge text is not matched.
								AuthStatusCode = StatusCode_challenge_failure;
							}
						}
						else
						{
							// The authentication/association state of the STA is not as expected. 
							AuthStatusCode = StatusCode_Unspecified_failure;
						}
					}
					else
					{
						// Cannot find the STA in the table => Sequence number illegal.
						AuthStatusCode = StatusCode_error_seqnum;
					}
				}
				else
				{
					// arAlg != SHARED_KEY or We don't support SA now => Sequence number illegal.
					AuthStatusCode = StatusCode_error_seqnum;
				}
			}
			break;

		default:
			RT_TRACE(COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): Illegal auth sequence number: %d !!!\n", arSeq));
			AuthStatusCode = StatusCode_error_seqnum;
			break;
	}

	// Remove association entry of the STA if AuthStatusCode is not successful. 
	if(AuthStatusCode != StatusCode_success)
	{
		RT_TRACE( COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): Remove association entry of the STA if AuthStatusCode is not successful. \n") );
		AsocEntry_RemoveStation(Adapter, arSta);
	}
	
	// Send auth frame.
	SendAuthenticatePacket( 
		Adapter, 
		arSta, // auStaAddr,
		arAlg, // AuthAlg,
		arSeq+1, // AuthSeq,
		AuthStatusCode, // AuthStatusCode
		AuthChallengetext // AuthChallengetext
		);

	return TRUE;
}


//
// Disassociate the STA specified.
// 2005.05.28, by rcnjko.
//
BOOLEAN
AP_DisassociateStation(
	IN	PADAPTER		Adapter, 
	IN	PRT_WLAN_STA	pSta, 
	IN	u1Byte			asRsn
	)
{
	// Check input parameter.
	if(pSta == NULL)
		return FALSE;
//	RT_ASSERT((pSta->bUsed && pSta->bAssociated), 
//		("AP_DisassociateStation(): bUsed: %x, bAssociated: %x !!!\n", pSta->bUsed, pSta->bAssociated));
	RT_TRACE(COMP_AP, DBG_LOUD, ("AP_DisassociateStation() pSta->AssociatedMacId %d\n", pSta->AssociatedMacId));

	pSta->bDisassociated = TRUE;

	if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP)
	{
		Adapter->MgntInfo.pCurrentSta = pSta;
		DrvIFIndicateIncommingAssociationComplete(Adapter, unspec_reason);
		DrvIFIndicateDisassociation(Adapter, asRsn, pSta->MacAddr);
	}

	RemovePeerTS(Adapter, pSta->MacAddr);

	// Send disassoc frame to the STA.
	SendDisassociation(Adapter, pSta->MacAddr, asRsn);	

//Added to del cam entrys for this station
//If pPeerMac is not NULL, then just del this  cam entry,by wl,2009-09-27
	if(WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0))
		Adapter->HalFunc.SetKeyHandler(Adapter,0, pSta->MacAddr,  0, 0, 0, TRUE);

	// Update its timestamp.
	AsocEntry_UpdateTimeStamp(pSta);

	// Mark the STA as disassoicated and related.
	AsocEntry_BecomeDisassoc(Adapter, pSta);

	// HalMacId Remove the specific MacId associated with this peer ---------
	MacIdDeregisterSpecificMacId(Adapter, pSta->AssociatedMacId);

	// To initialize WPA-PSK key mgnt state machine. Added by Annie, 2005-07-15.
	if( Adapter->MgntInfo.SecurityInfo.SecLvl == RT_SEC_LVL_WPA || Adapter->MgntInfo.SecurityInfo.SecLvl == RT_SEC_LVL_WPA2  )
		Authenticator_StateDISCONNECTED(Adapter, pSta);

	pSta->bDisassociated = FALSE;

	FunctionOut(COMP_AP);

	return TRUE;
}


//
// Disassociate all STAs associated.
// 2005.05.28, by rcnjko.
//
VOID
AP_DisassociateAllStation(
	IN	PADAPTER		Adapter,
	IN	u1Byte			asRsn
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	int				i;
	PRT_WLAN_STA	pSta;	

	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		pSta = &(pMgntInfo->AsocEntry[i]);

		if(pSta->bUsed)
		{
			if(pSta->bAssociated)
			{
				RT_TRACE(COMP_AP, DBG_LOUD, ("AP_DisassociateAllStation()  index %d MacId %d\n", i, pSta->AssociatedMacId));
				//AP_DisassociateStation(Adapter, pSta, unspec_reason);
				AP_DisassociateStation(Adapter, pSta, asRsn);
			}
		}
	}
}

//
// Reset AP mode related variables.
// 2005.05.27, by rcnjko.
//
VOID
AP_ResetVariables(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PADAPTER		DefAdapter = GetDefaultAdapter(Adapter);
	u1Byte			i;

	FunctionIn(COMP_AP);

	DefAdapter->MgntInfo.bSwitchingSTAStateInProgress = FALSE;
	
	// Added by Annie, 2006-01-26.
	MgntCancelAllTimer(Adapter);
	RemoveAllTS(Adapter);

	// <RJ_TODO> We shall randomize it? 
	for(i = 0; i < 128; i++)
	{
		pMgntInfo->arChalng[i] = (u1Byte)i;
	}

	// Reset Association Table.
	AsocEntry_ResetAll(Adapter);
		
	Authenticator_GlobalReset(Adapter);	// added by Annie
	
	ActUpdate_ProtectionMode(Adapter, FALSE); // 2010.11.26. by tynli.
	

	if(ACTING_AS_AP(Adapter))
	{ // AP mode.
		if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED)
		{
			static u1Byte DummySta[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

			ConstructBeaconFrame(Adapter);
			MgntUpdateAsocInfo(Adapter, UpdateAsocBeacon, pMgntInfo->beaconframe.Octet, pMgntInfo->beaconframe.Length);

			//
			// step 1. Indicate wildcard disassocation event to 
			// disassociate all STA associated before.
			//
			/*****************************************************************
			20150416 Sinda. 
			We need to delete peer in WDI architecture, but peer had been deleted before.
			pass NULL to skip this case to avoid delete incorrect peer.
			*****************************************************************/
			DrvIFIndicateDisassociation(Adapter, unspec_reason, NULL); 

			//
			// step 2. Indicate dummy STA assocation event.
			//
			// 061228, rcnjko: UI need fake AP mode to enter connected 
			// state immediate for ICS easy implementation.
			//
			MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, DummySta, 6);
			DrvIFIndicateAssociationStart(Adapter);
			DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_SUCCESS);
		}

		// Change Link status to connected.
		MgntIndicateMediaStatus(Adapter, RT_MEDIA_CONNECT, TRUE);	
	}
	else
	{ // STA mode.
		if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED)
		{
			//
			// step 1. Indicate wildcard disassocation event to 
			// disassociate all STA associated before.
			//
			/*****************************************************************
			20150416 Sinda. 
			We need to delete peer in WDI architecture, but peer had been deleted before.
			pass NULL to skip this case to avoid delete incorrect peer.
			*****************************************************************/
			DrvIFIndicateDisassociation(Adapter, unspec_reason, NULL); 
		}

		// Change Link status to disconnected.
		MgntIndicateMediaStatus(Adapter, RT_MEDIA_DISCONNECT, TRUE);

		// Reset TStream related, 070614, by rcnjko.
		QosResetAllTs(Adapter); 
	}
	FunctionOut(COMP_AP);
}


//
// Set up SW variables.
// 2005.05.27, by rcnjko.
//
VOID
AP_SetupStartApInfo(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO	pDefMgntInfo = GetDefaultMgntInfo(Adapter);
	int 		i;
	BOOLEAN 	bSupportNmode = Adapter->HalFunc.GetNmodeSupportBySecCfgHandler(Adapter);

	FunctionIn(COMP_AP);

	// Cancel key management timer. Added by Annie, 2005-06-29.
	PlatformCancelTimer(Adapter, &pMgntInfo->globalKeyInfo.KeyMgntTimer );

	DFS_TimerContrl(Adapter, DFS_TIMER_CANCEL);
	
	// Reset scan related state. 
	if(MgntScanInProgress(pMgntInfo))
		MgntResetScanProcess(Adapter);

	// Reset link detection related. <NOTE> Acturally, this is not necessary, because we won't perform link detection of STA mode in AP mode.
	pMgntInfo->LinkDetectInfo.NumRecvBcnInPeriod = 0;
	pMgntInfo->LinkDetectInfo.NumRecvDataInPeriod = 0;
	for( i=0; i < RT_MAX_LD_SLOT_NUM; i++ )
	{
		pMgntInfo->LinkDetectInfo.RxBcnNum[i] = 0;
		pMgntInfo->LinkDetectInfo.RxDataNum[i] = 0;
	}

	// Reset roaming status, 2004.10.12, by rcnjko.
	if(MgntRoamingInProgress(pMgntInfo))
	{
		if(MgntResetOrPnPInProgress(Adapter))
		{
			RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 4\n"));
			return;
		}
	
		MgntRoamComplete(Adapter, MlmeStatus_invalid);
	}
	
	// Initialize dirver operating states to AP mode.
	RT_ASSERT(ACTING_AS_AP(Adapter), ("AP_SetupStartApInfo(): ACTING_AS_AP(Adapter) == FALSE!!!\n"));
	pMgntInfo->OpMode = RT_OP_MODE_AP;
	pMgntInfo->state_Synchronization_Sta = STATE_No_Bss;
	pMgntInfo->mAssoc = FALSE; // <NOTE> We will set mAssoc=TRUE when starting sending beacon, 2005.07.19, by rcnjko.
	pMgntInfo->mIbss = FALSE;
	pMgntInfo->bIbssStarter = FALSE;
	pMgntInfo->mDisable = TRUE; // <NOTE> We will set mDisable=FALSE until starting sending beacon.
	pMgntInfo->bMlmeStartReqRsn = MLMESTARTREQ_NONE;
	pMgntInfo->bHalfWiirelessN24GMode = FALSE;
	pMgntInfo->NumBssDesc4Query = 0;
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("[REDX]: AP_SetupStartApInfo(), clear NumBssDesc4Query\n"));
	// BSSID
	PlatformMoveMemory(pMgntInfo->Bssid, Adapter->CurrentAddress, 6);
	
	RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "AP_SetupStartApInfo(): BSSID = ", pMgntInfo->Bssid);

	// Capability.
	pMgntInfo->mCap |= cESS;
	pMgntInfo->mCap &= (~cIBSS);

	// Preamble mode.
	if(pMgntInfo->RegPreambleMode != PREAMBLE_LONG)
		pMgntInfo->mCap |= cShortPreamble;
	else
		pMgntInfo->mCap &= (~cShortPreamble);

	// Beacon Period.
	pMgntInfo->dot11BeaconPeriod = pMgntInfo->Regdot11BeaconPeriod;

	// DTIM Period.
	pMgntInfo->dot11DtimPeriod = pMgntInfo->Regdot11DtimPeriod;
	
	//Set dot11currentchannelnumber--------------
	if(pDefMgntInfo->mAssoc && !IsDefaultAdapter(Adapter))
		pMgntInfo->dot11CurrentChannelNumber = pDefMgntInfo->dot11CurrentChannelNumber;
	else
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("GetFirstGOPort(Adapter): %p\n", GetFirstGOPort(Adapter)));
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Adapter: %p\n", Adapter));
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("pMgntInfo->dot11CurrentChannelNumber: %d\n", pMgntInfo->dot11CurrentChannelNumber));
		
#if (MULTICHANNEL_SUPPORT == 1 && P2P_SUPPORT == 1)			
		if(Adapter == GetFirstGOPort(Adapter))
		{
			// Win8 will set the GO channel:
			//	+ Do nothing
			// GO Start at operating channel when default port is disconnected
			//by sherry 20130117

			PP2P_INFO	pP2PInfo = GET_P2P_INFO(Adapter);
			PADAPTER	pDeviceAdapter = NULL;
			if(RT_MEDIA_CONNECT != MgntLinkStatusQuery(pDefaultAdapter) && 
				(NULL != (pDeviceAdapter = GetFirstDevicePort(Adapter))))
			{				
				pDefMgntInfo->dot11CurrentChannelNumber = pP2PInfo->OperatingChannel;
				pMgntInfo->dot11CurrentChannelNumber = pP2PInfo->OperatingChannel;
				pDeviceAdapter->MgntInfo.dot11CurrentChannelNumber = pP2PInfo->OperatingChannel;
			}
			else
			{
				pMgntInfo->dot11CurrentChannelNumber = pMgntInfo->Regdot11ChannelNumber;			
			}
		}
		else
#endif				
		{
			pMgntInfo->dot11CurrentChannelNumber = pMgntInfo->Regdot11ChannelNumber;
		}
	}

	

	//[Win7] Default port and Extension port can use different wireless mode, but they
	// should share the same bandwidth and QoS parameters like EDCA . 2009.07.09, by Bohn
	Adapter->HalFunc.SetWirelessModeHandler(Adapter, SetupStarWirelessMode(Adapter, bSupportNmode));

	if(!pDefMgntInfo->mAssoc && !IsDefaultAdapter(Adapter))
	{
		BOOLEAN	bSetDefPortWirelessMode = FALSE;
		if(	IS_WIRELESS_MODE_5G(Adapter) && IS_WIRELESS_MODE_24G(pDefaultAdapter))
			bSetDefPortWirelessMode = TRUE;
		else if(IS_WIRELESS_MODE_24G(Adapter) && IS_WIRELESS_MODE_5G(pDefaultAdapter))
			bSetDefPortWirelessMode = TRUE;

		//If they use different band , after ScanComplete of Default port, band will mismatch of Ext AP port.  
		if(bSetDefPortWirelessMode)
			Adapter->HalFunc.SetWirelessModeHandler(pDefaultAdapter, pMgntInfo->dot11CurrentWirelessMode);
	}

	//------------------------------------------
	pMgntInfo->CurrentBssWirelessMode = pMgntInfo->dot11CurrentWirelessMode; // Keep wireless mode of IBSS to start. 2005.02.17, by rcnjko.

	// Short slot time.
	if(pMgntInfo->dot11CurrentWirelessMode == WIRELESS_MODE_B)
	{
		pMgntInfo->mCap &= (~cShortSlotTime);
	}
	else
	{
		pMgntInfo->mCap |= cShortSlotTime;
	}

	// Protection mode.
	ActUpdate_ProtectionMode(Adapter, FALSE);
	
	// Reset roaming status, 2004.10.12, by rcnjko.
	pMgntInfo->AsocRetryCount = 0;

	// Set Key management timer. Annie, 2005-07-06.
	if( (pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPAPSK || 
		pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA2	)  &&
		MgntActQuery_ApType(Adapter) != RT_AP_TYPE_VWIFI_AP 
		&& MgntActQuery_ApType(Adapter) != RT_AP_TYPE_LINUX) // key mgnt is done by OS in Win7 AP mode
	{
		PlatformSetTimer( Adapter, &pMgntInfo->globalKeyInfo.KeyMgntTimer, KEY_MGNT_INTERVAL );
	}
	
	DFS_TimerContrl(Adapter, DFS_TIMER_SET);

	// Reset CCX related setting.
	CCX_OnBssReset(Adapter);
	
	// Set default EDCA parameter for AP mode.
	if(pMgntInfo->pStaQos->QosCapability != QOS_DISABLE)
	{
		// TODO: CurrentQosMode can be set to others. Now only support WMM.
		pMgntInfo->pStaQos->CurrentQosMode = QOS_WMM;
		pMgntInfo->pStaQos->QBssWirelessMode = pMgntInfo->dot11CurrentWirelessMode;
			
		//[Win7 For Two Port Shared Data this function will retrurn the Right Port Ones]2009.07.22, by Bohn 
		SET_WMM_PARAM_ELE_AC_PARAMS(
		(pu1Byte)GetTwoPortSharedResource(Adapter,TWO_PORT_SHARED_OBJECT__SET_OF_pStaQos_WMMParamEle,pMgntInfo->pStaQos->WMMParamEle,NULL)
		, Adapter->AP_EDCA_PARAM);
		//
		// cDelayedBA & cImmediateBA
		// This part shall be add to Capability field later.
		//
	}
	else
	{
		pMgntInfo->pStaQos->CurrentQosMode = QOS_DISABLE;
		pMgntInfo->pStaQos->QBssWirelessMode = WIRELESS_MODE_UNKNOWN;	
	}

	// Using HT Default setting when startup AP mode.
	// Using VHT Default setting when startup AP mode
	if(pMgntInfo->pStaQos->CurrentQosMode & QOS_WMM)
	{
		HTUseDefaultSetting(Adapter);
		VHTUseDefaultSetting(Adapter);
	}
	else
	{
		pMgntInfo->pHTInfo->bCurrentHTSupport = FALSE;
		pMgntInfo->pVHTInfo->bCurrentVHTSupport = FALSE;
		pMgntInfo->dot11CurrentChannelBandWidth = CHANNEL_WIDTH_20;

		if(pDefMgntInfo->mAssoc==TRUE)
			pMgntInfo->IOTPeer = pDefMgntInfo->IOTPeer;
		else
			pMgntInfo->IOTPeer = HT_IOT_PEER_SELF_SOFTAP;
	}	

	// Reset TStream related, 070614, by rcnjko.
	QosResetAllTs(Adapter); 

	FunctionOut(COMP_AP);
}

// 
// Configure HW to start AP mode.
//
VOID
AP_StartApRequest(
	IN	PVOID	Context
	)
{
	PADAPTER		Adapter = (PADAPTER)Context;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(Adapter);
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);

	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PMGNT_INFO 		pDefaultMgntInfo = GetDefaultMgntInfo(Adapter);

	RT_OP_MODE		btLinkStatus = RT_OP_MODE_NO_LINK;
	u1Byte			RetryLimit = HAL_RETRY_LIMIT_AP_ADHOC;
	u1Byte			SwBeaconType = BEACON_SEND_AUTO_HW;
	HAL_AP_IBSS_INT_MODE	intMode = HAL_AP_IBSS_INT_AP;
	BOOLEAN			bUseRAMask = FALSE;
	BOOLEAN			bP2PGo = FALSE;

	FunctionIn(COMP_AP);

	if (pDefaultAdapter->bDriverStopped)
	{
		pMgntInfo->bSwitchingAsApInProgress = FALSE; // this is depend on ADAPTER,Neo Test 123
		return;
	}

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_HW_BEACON_SUPPORT, (pu1Byte)&SwBeaconType);

	pMgntInfo->BeaconState = BEACON_STARTING;

	MultichannelEnableGoQueue(Adapter);
	
	//--------------------------------------------------------------------------------
	// Infrastructure AP mode HW Configuration.
	//--------------------------------------------------------------------------------
	
	pHalData->bNeedIQK = TRUE;
	pHalData->DM_OutSrc.RFCalibrateInfo.bNeedIQK = TRUE;

	// Clear MSR first, asked by Owen, 2005.01.27, by rcnjko.
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&btLinkStatus) );

	//
	// Update Capability field related setting.
	//
	// <NOTE> To solve Win98 IRQL==PASIVE_LEVEL in timer callback function, 
	// we call ActUpdate_mCapInfo() here to make sure related IOs are perfomed in correct IRQL, 
	// that is, PASSIVE_LEVEL=>Syn IO for this case. 2005.03.15, by rcnjko.
	//
	ActUpdate_mCapInfo( Adapter, pMgntInfo->mCap );

	// Set BSSID.
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pMgntInfo->Bssid );

	// Set Beacon related registers
	// <RJ_TODO>  We shall update AtimWindow specified in Beacon of the IBSS if "every thing is ready." 
	pMgntInfo->dot11AtimWindow = 2;	// ATIM Window (in unit of TU).
	Adapter->HalFunc.SetBeaconRelatedRegistersHandler(Adapter, (PVOID)(&pMgntInfo->OpMode), \
								pMgntInfo->dot11BeaconPeriod, pMgntInfo->dot11AtimWindow);

	// Set operation mode to IBSS.	
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_RETRY_LIMIT, (pu1Byte)(&RetryLimit));
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_AP_IBSS_INTERRUPT, (pu1Byte)&intMode);
	
	PlatformAtomicExchange((pu4Byte)(&GET_HAL_DATA(Adapter)->RegBcnCtrlVal), GET_HAL_DATA(Adapter)->RegBcnCtrlVal|BIT2);		
	PlatformEFIOWrite1Byte(Adapter, REG_BCN_CTRL, (u1Byte)(GET_HAL_DATA(Adapter)->RegBcnCtrlVal));

	// Set EDCA Parameter Set for QAP
	if(pMgntInfo->pStaQos->CurrentQosMode != QOS_DISABLE)
	{
		AC_CODING	eACI;
		for(eACI = 0; eACI < AC_MAX; eACI++)
			Adapter->HalFunc.SetHwRegHandler(
				Adapter, HW_VAR_AC_PARAM, GET_WMM_PARAM_ELE_SINGLE_AC_PARAM(
					(pu1Byte)GetTwoPortSharedResource /*,eACI*/
						(Adapter,TWO_PORT_SHARED_OBJECT__SET_OF_pStaQos_WMMParamEle,pMgntInfo->pStaQos->WMMParamEle,NULL),eACI));
	}
	else
	{
		// Using 802.11 default settings!!
		QosSetLegacyACParam(Adapter);
	}

	AP_SetBandWidth(Adapter);

	HTSetCCKSupport(Adapter, TRUE, NULL);

	// Adjust rate related setting according to current wireless mode and current bandwidth. 
	SelectRateSet( Adapter, pMgntInfo->Regdot11OperationalRateSet );

	// To ensure AP is started at default channel to prevent from that STA cannot scan AP. (e.g., single-band STA)
#if P2P_SUPPORT == 1
	if((!(P2P_ENABLED(GET_P2P_INFO(Adapter)) && (GET_P2P_INFO(Adapter)->Role == P2P_GO)))
		&& (pDefaultMgntInfo->mAssoc == FALSE) )
#else
	if(pDefaultMgntInfo->mAssoc == FALSE) 
#endif
		pMgntInfo->dot11CurrentChannelNumber = pDefaultMgntInfo->Regdot11ChannelNumber;;
	
	//Fix Vwifi channel show error	
	N62CAPIndicateFrequencyAdopted(Adapter);

	// Send beacon shall be delay until finish bw & channel offset setttings.
	RT_TRACE(COMP_AP, DBG_LOUD, ("AP_StartApRequest(): pMgntInfo->bStartApDueToWakeup %d\n",pDefaultMgntInfo->bStartApDueToWakeup));

	//
	// Start to send beacon.
	//
	if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP && pDefaultMgntInfo->bStartApDueToWakeup)
	{	// note that only win7 enters here since only win7 has vwifi ap
		//
		// In this case, we are called from PnpSetPower().
		// NDISTest preview 3 SoftAP_Power_ext requries us to stop beaconing 
		// after resuming from hibernation. So we do nothing here.
		// 2008.12.05, haich
		//
		
		//
		// We have to set MSR to no link so that HW will not send beacon automatically.
		//
		RT_TRACE(COMP_AP, DBG_LOUD, ("AP_StartApRequest(): Stopping Beacon... \n"));

		if(SwBeaconType <= BEACON_SEND_AUTO_SW) // Configure the HW to stop beaocn and related settings.
		{//stop hw beacon
			RT_TRACE(COMP_AP, DBG_LOUD, ("AP_StartApRequest(): Stopping H/W Beacon...\n"));
			pMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode));
			pMgntInfo->OpMode = RT_OP_MODE_AP;
		}
	}

	if(pDefaultMgntInfo->bStartApDueToWakeup==FALSE)
	{
		if(SwBeaconType >= BEACON_SEND_AUTO_SW)
		{ // S/W Beacon.
			pMgntInfo->NextBeacon = pMgntInfo->NowTSF = 0;

			RT_TRACE(COMP_AP, DBG_LOUD, ("AP_StartApRequest(): Set SwBeaconTimer...\n"));

			pMgntInfo->bDisSwDmaBcn = TRUE;
			PlatformSetTimer(Adapter, &pMgntInfo->SwBeaconTimer, 0);
		}
		else
		{ // H/W Beacon.	
			RT_TRACE(COMP_AP, DBG_LOUD, ("AP_StartApRequest(): Send H/W Beacon....\n"));
			
			ResumeTxBeacon_92C(Adapter);
			
			ConstructBeaconFrame(Adapter);
			SendBeaconFrame(Adapter, BEACON_QUEUE);
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_DIS_SW_BCN, (pu1Byte)(&Adapter));
		}
	}

	bP2PGo = P2P_ACTING_IS_GO(Adapter);
	MultiChannelResetApStatus(Adapter, bP2PGo);
	//--------------------------------------------------------------------------------
	// Infrastructure AP mode SW Configuration.
	//--------------------------------------------------------------------------------

	pMgntInfo->mAssoc = TRUE;
	pMgntInfo->mDisable = FALSE;

	pMgntInfo->bSwitchingAsApInProgress = FALSE;
	pMgntInfo->bDisableCCKRateForVWIFIChangeChannel = FALSE;

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USE_RA_MASK, &bUseRAMask);
	if(bUseRAMask)
	{
		u4Byte		MacId = MAC_ID_STATIC_FOR_AP_MULTICAST;

		MacId = MacIdRegisterMacIdForAPMcast(Adapter);
		Adapter->HalFunc.UpdateHalRAMaskHandler(
									Adapter,
									(u1Byte)MacId,
									NULL,
									0
									);
	}

	// Set media type/status in HW after sendning Beacon content to HW to make sure the content is correct.
	Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode) );


	P2P_StartApRequest(Adapter);

	// 2014/02/11 MH For ICS/DHCP delay, if ics and dhcp are not ready. And we start to 
	// send beacon. Then the client ca not acces ip directly, we need to delay until ICS is ready
	// wait OID to trigger.
	if(!pDefaultAdapter->bInHctTest && pMgntInfo->Regbcndelay)
	{
		RT_TRACE(COMP_AP, DBG_LOUD, ("Stop beacon\n"));
		Hal_PauseTx(Adapter, HW_DISABLE_TX_BEACON);
		pMgntInfo->bDelayApBeaconMode = TRUE;
		pMgntInfo->DelayApBeaconCnt = 0;
		RT_TRACE(COMP_AP, DBG_LOUD, ("<=== ++++++ AP_StartApRequest() ++++++\n"));
	}
	FunctionOut(COMP_AP);
}


// 
// Start AP mode.
// 2005.05.27, by rcnjko.
//
VOID
AP_Restart(
	IN	PADAPTER		Adapter
	)
{		
	FunctionIn(COMP_AP);
#if USE_WORKITEM
	{
		PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

		// Make sure at most one of such workitem is executed.
		if((PlatformIsWorkItemScheduled(&(pMgntInfo->ApStartRequestWorkItem)) == FALSE) )
		{
			// Set up SW setting about AP mode.
			// Since AP_SetupStartApInfo() will set mDisable=TRUE, we must make sure AP_StartApRequest() will be called after it. 2005.09.25, by rcnjko.
			AP_SetupStartApInfo(Adapter);

			// Configure HW to start AP mode.
			PlatformScheduleWorkItem( &(pMgntInfo->ApStartRequestWorkItem) );
		}
		else
		{
			RT_TRACE(COMP_AP, DBG_SERIOUS, ("AP_Restart(): failed to schedule a work item because another one exists.\n"));
		}
	}
#else
	// Set up SW setting about AP mode.
	AP_SetupStartApInfo(Adapter);

	// Configure HW to start AP mode.
	AP_StartApRequest((PVOID)Adapter);
#endif
	FunctionOut(COMP_AP);
}

// 
// Reset SW and start AP mode. 
// 2005.05.27, by rcnjko.
//
VOID
AP_Reset(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PMGNT_INFO	pDefMgntInfo = GetDefaultMgntInfo(Adapter);
	BOOLEAN 	bFilterOutNonAssociatedBSSID = TRUE;
	BOOLEAN		bUseRAMask = FALSE;
	HAL_AP_IBSS_INT_MODE	intMode = HAL_AP_IBSS_INT_DISABLE;

	FunctionIn(COMP_AP);

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USE_RA_MASK, &bUseRAMask);
	
	// Disable Beacon early interrupt mask.
	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_AP_IBSS_INTERRUPT, (pu1Byte)&intMode);
	
	// Return all packets queued (TxQ, PsQ, etc.).
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	AP_PS_ReturnAllQueuedPackets(Adapter,FALSE);

	// Release all queued MSDU packet from upper layer, added by Roger, 2013.07.11.
	PlatformReleaseDataFrameQueued(Adapter);
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	// Reset AP mode related variables.
	AP_ResetVariables(Adapter);

	// Start AP mode if requested.
	if(ACTING_AS_AP(Adapter))
	{	
		if(GET_TDLS_ENABLED(pDefMgntInfo))
		{
			if(pDefMgntInfo->mAssoc)
				TDLS_Stop(GetDefaultAdapter(Adapter));
		}

		//sherry moved set check bssid here 
		//for wlk1.6 VWifiInfraSoftAP_ext, 20110926
		if(bUseRAMask && 
			(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP ||
			MgntActQuery_ApType(Adapter) == RT_AP_TYPE_EXT_AP))
		{
			bFilterOutNonAssociatedBSSID = FALSE;
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
		}
		else
		{
			// Currently we set this variable to FALSE as default until CBSSID_DATE on normal chip is safely to set.
			bFilterOutNonAssociatedBSSID = FALSE;
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
		}

		AP_Restart(Adapter);		
	}
	else
	{ // STA mode setting. Added by Annie, 2005-11-23.

		u1Byte	RetryLimit = HAL_RETRY_LIMIT_INFRA;

		pMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
		
		// Under Win7, when AP mode is stopped,
		// default port may still connect to AP.
		// Hence, we have to set HwReg back to Infrastructure mode.

		if(pDefMgntInfo->bMediaConnect)
		{
			// Set BSSID
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pDefMgntInfo->Bssid );

			// Set Basic Rate
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BASIC_RATE, (pu1Byte)(&pDefMgntInfo->mBrates));

			// Set Operation Mode
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pDefMgntInfo->OpMode) );
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_RETRY_LIMIT, (pu1Byte)(&RetryLimit));

			if(bUseRAMask)
			{
				bFilterOutNonAssociatedBSSID = TRUE;
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
			}
			else
			{
				bFilterOutNonAssociatedBSSID = FALSE;
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
			}
		}
		else
		{
			ReleaseDataFrameQueued(Adapter);
			bFilterOutNonAssociatedBSSID = FALSE;

			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_BSSID, pDefMgntInfo->Bssid );
			// Set Operation Mode
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_MEDIA_STATUS, (pu1Byte)(&pMgntInfo->OpMode) );
			Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_RETRY_LIMIT, (pu1Byte)(&RetryLimit));
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CHECK_BSSID, (pu1Byte)(&bFilterOutNonAssociatedBSSID));
		}

		//
		// In AP_StartApRequest(), this flag is set to true, therefore if we are going to stop AP mode, 
		// we should recover this flag.
		// When Win7 starts SoftAP, it does reset for 2 times, at the second time, if this flag is still true, 
		// the driver would call MgntDisconnectAP and send disassociation to itself. 
		// 2010.11.15, haich.
		//
		pMgntInfo->mAssoc = FALSE;

		MultiChannelDisconnectGo(Adapter);

		pMgntInfo->bSwitchingAsApInProgress = FALSE;
	}

	FunctionOut(COMP_AP);
}


// Called by AP_DisconnectAfterSTANewConnected
// Adapter = Ext Adapter
VOID
Ap_SendDisassocWithOldChnlWorkitemCallback(
	IN PVOID			pContext
	)

{
	PADAPTER 			pAdapter = (PADAPTER)pContext;
	PADAPTER			pDefAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER			pExtAdapter = pAdapter;
	PMGNT_INFO			pDefMgntInfo = &pDefAdapter->MgntInfo;
	PRT_CHANNEL_INFO	pDefChnlInfo = GET_CHNL_INFO(pDefMgntInfo);
	PMGNT_INFO			pExtMgntInfo = &pAdapter->MgntInfo;
	PRT_CHANNEL_INFO	pExtChnlInfo = GET_CHNL_INFO(pExtMgntInfo);
	WIRELESS_MODE		wirelessModebackup = pDefMgntInfo->dot11CurrentWirelessMode;
	
	RT_TRACE(COMP_AP, DBG_LOUD, ("====> Ap_SendDisassocWithOldChnlWorkitemCallback\n"));

	pExtAdapter->bAPChannelInprogressForNewconnected = TRUE;

#if (P2P_SUPPORT == 1)    
	P2PIndicateChangingApChnl(GET_P2P_INFO(pExtAdapter), pExtMgntInfo->dot11CurrentChannelNumber, pDefMgntInfo->dot11CurrentChannelNumber);
#endif
	
// 1. ==========     AP Swith to Original Channel Send DisAssoc    ==========
	pExtAdapter->HalFunc.SetWirelessModeHandler(pExtAdapter,pExtMgntInfo->dot11CurrentWirelessMode);

	CHNL_SetBwChnl( pExtAdapter,
						pExtChnlInfo->PrimaryChannelNumber,
						pExtChnlInfo->CurrentChannelBandWidth,
						pExtChnlInfo->Ext20MHzChnlOffsetOf40MHz
						);	

	//wait for switch channel successful before sending disassociation by sherry 20130117
	while(RT_IsSwChnlAndBwInProgress(pAdapter))
		PlatformSleepUs(20);
	//<<-------- Change BW Total Command ----------

	RT_TRACE_F(COMP_AP, DBG_LOUD, (" Send Disassociate to All STA\n"));

	AP_DisassociateAllStation(pExtAdapter, unspec_reason);

// 2. ==========	 AP Swith to STA's Channel     ==========

	//[win7 Two Port BW40Mhz issue] Be carefull of 40Mhz Channel Setting
	// Because it is polute by "bad written function, MgntActSet_802_11_CHANNEL"
	// Which need to INPUT "central channel" and Polute Correct "dot11CurrentChannelNumber"
	//2009.05.20, by bohn

	//-------->> Change Channel Total Command ----------
	while(RT_IsSwChnlAndBwInProgress(pAdapter))
		PlatformSleepUs(20);

	//<<-------- Change Channel Total Command ----------

	//[win7 Two Port BW40Mhz issue] Default Port's "dot11CurrentChannelNumber" must be
	// updated to "pHalData->CurrentChannel". Also due to "bad written function:HTSetConnectBwMode"
	//-------->> Change BW Total Command ----------

	pDefAdapter->HalFunc.SetWirelessModeHandler(pDefAdapter,wirelessModebackup);
	CHNL_SetBwChnl( 	pDefAdapter,
						pDefChnlInfo->PrimaryChannelNumber,
						pDefChnlInfo->CurrentChannelBandWidth,
						pDefChnlInfo->Ext20MHzChnlOffsetOf40MHz
						);

	//<<-------- Change BW Total Command ----------
	//4 [win7 Two Port BW40Mhz issue] We May Recard the value for Extension Port, by Bohn, 2009.06.05
	//CHNL_SetChnlInfoFromDestPort(pExtAdapter, pDefAdapter);
	MgntStopBeacon(pExtAdapter);
// 3. =============	 Restart AP     ======================	
	if(1)//pDefAdapter->bInHctTest)	// In HctTest, do not delay beacon for 20seconds.
		PlatformSetTimer(pAdapter, &pExtMgntInfo->DelaySendBeaconTimer , 1000);
	else		
		PlatformSetTimer(pAdapter, &pExtMgntInfo->DelaySendBeaconTimer , pDefMgntInfo->ResumeBeaconTime);

	N62CAPIndicateFrequencyAdopted(pExtAdapter);	

	pExtAdapter->bAPChannelInprogressForNewconnected = FALSE;

	RT_TRACE(COMP_AP, DBG_LOUD, ("<==== Ap_SendDisassocWithOldChnlWorkitemCallback\n"));
}


// Called by Two port System
VOID 
AP_DisconnectAfterSTANewConnected(	
	PADAPTER Adapter
	)
{
	//
	//We should send disasoc packet to all the STA who are connect to the AP.
	//So we need to switch to original channel for send disasoc packet.
	//By Maddest, 080903
	//
	// Disconnect clients and delay send beacon if one of the following settings is different:
	// (a) Channel, (b) Bandwidth, (c) Chnloffset (for 40MHz)
	// Add by hpfan, 2009.12.09
	//
	PADAPTER			DefAdapter = GetDefaultAdapter(Adapter);
	PADAPTER			ExtAdapter = Adapter;
	PMGNT_INFO			pDefMgntInfo = &DefAdapter->MgntInfo;
	PMGNT_INFO			pExtMgntInfo = &ExtAdapter->MgntInfo;
	PRT_CHANNEL_INFO	pDefChnlInfo = GET_CHNL_INFO(pDefMgntInfo);
	PRT_CHANNEL_INFO	pExtChnlInfo = GET_CHNL_INFO(pExtMgntInfo);
	
	RT_TRACE(COMP_AP, DBG_LOUD, ("Def Channel  %d  Ext Channel  %d \n",
		pDefMgntInfo->dot11CurrentChannelNumber, pExtMgntInfo->dot11CurrentChannelNumber));
	RT_TRACE(COMP_AP, DBG_LOUD, ("Ext BW=%d, offset=%d\n", 
		pExtChnlInfo->CurrentChannelBandWidth, pExtChnlInfo->Ext20MHzChnlOffsetOf40MHz));
	RT_TRACE(COMP_AP, DBG_LOUD, ("Def BW=%d, offset=%d\n", 
		pDefChnlInfo->CurrentChannelBandWidth, pDefChnlInfo->Ext20MHzChnlOffsetOf40MHz));
	
	if(	AP_DetermineAlive(ExtAdapter) &&
		( (pExtMgntInfo->dot11CurrentChannelNumber !=pDefMgntInfo->dot11CurrentChannelNumber) ||
		(pExtChnlInfo->CurrentChannelCenterFrequency != pDefChnlInfo->CurrentChannelCenterFrequency))
		)
	{		
		PlatformScheduleWorkItem( &(pExtMgntInfo->ApSendDisassocWithOldChnlWorkitem));
	}
	else
	{
		RT_TRACE( COMP_AP, DBG_LOUD, ("Channel are the Same, Wouldn't Change\n"));
	}

	// Under win7, we may have an adapter running in AP/IBSS mode, 
	// this action would stop updating beacon (UpdateBeaconFrame()).
	// If one of the adapters needs to send beacon, we skip this action.
	// 2008.07.10, haich.
	// Update Interrup Mask for STA in infrastructure mode. 
	// Currently, we only remove the masks set in IBSS mode. 2005.12.14, by rcnjko.	
	if(!(pDefMgntInfo->OpMode == RT_OP_MODE_AP || pDefMgntInfo->OpMode  == RT_OP_MODE_IBSS ||
		pExtMgntInfo->OpMode  == RT_OP_MODE_AP ||pExtMgntInfo->OpMode  == RT_OP_MODE_IBSS))
	{
		HAL_AP_IBSS_INT_MODE	intMode = HAL_AP_IBSS_INT_DISABLE;
		
		Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_AP_IBSS_INTERRUPT, (pu1Byte)&intMode);
	}

}


VOID
AP_StatusWatchdog(
	IN	PADAPTER		Adapter
)
{
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO 	pDefMgntInfo = GetDefaultMgntInfo(Adapter);
	PMGNT_INFO 	pExtMgntInfo = &Adapter->MgntInfo;

	if(	MgntRoamingInProgress(pDefMgntInfo) ||
		MgntIsLinkInProgress(pDefMgntInfo) ||
		MgntScanInProgress(pDefMgntInfo))
		return;
	else if(pExtMgntInfo->bSwitchingAsApInProgress)
		;
	else if(pExtMgntInfo->bDelaySwitchToApMode)
	{
		pExtMgntInfo->bDelaySwitchToApMode = FALSE;
		pDefMgntInfo->bSwitchingSTAStateInProgress = FALSE;
		RT_TRACE(COMP_AP,DBG_LOUD, ("Handle bDelaySwitchToApMode in AP_StatusWatchdog\n"));
		EXT_AP_START_AP_MODE(Adapter);
	}		
	else if(pDefMgntInfo->bSwitchingSTAStateInProgress)
	{
		pDefMgntInfo->bSwitchingSTAStateInProgress = FALSE;
		RT_TRACE(COMP_AP,DBG_LOUD, ("bSwitchingSTAStateInProgress in AP_StatusWatchdog\n"));
		EXT_AP_DISASSOCATE_AFTER_STA_LINK_TO_DIFFERENT_CHNNEL_AP(Adapter);
	}

	//
	// 2014/02/11 MH Add for AP mode beacon delay mechanism for ICS/DHCP ready.
	//
	if(!pDefaultAdapter->bInHctTest && pDefMgntInfo->Regbcndelay)
	{
		if (pDefMgntInfo->bDelayApBeaconMode == TRUE)
		{
			if (pDefMgntInfo->DelayApBeaconCnt ++ >= pDefMgntInfo->Regbcndelay)
			{
				RT_TRACE(COMP_AP, DBG_LOUD, ("Start beacon\n"));
				pDefMgntInfo->bDelayApBeaconMode = FALSE;
				Hal_PauseTx(pDefaultAdapter, HW_ENABLE_TX_BEACON);
			}
		}
	}
}

BOOLEAN
AP_CheckRSNIE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry,
	IN	OCTET_STRING	asocpdu
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING	RSNIE;
	BOOLEAN			Match;
	BOOLEAN			bWPSEnable = FALSE;
	u1Byte			tmpOUI[SIZE_OUI] = {0};
	u1Byte			tmpOUIType = 0;
	PRT_NDIS62_COMMON	pNdis62Common = Adapter->pNdis62Common;
	bWPSEnable = pNdis62Common->bWPSEnable;

	//2 Step 1. Get RSNIE from STA's asocpud

	// Code reference: GetValueFromBeaconOrProbeRsp()
	{
		u1Byte count;

		// Initialize first
		pEntry->perSTAKeyInfo.SecLvl = RT_SEC_LVL_NONE;
		pEntry->perSTAKeyInfo.GroupCipherSuite = RT_ENC_ALG_NO_CIPHER;
		pEntry->perSTAKeyInfo.PairwiseCipherCount = 0;
		pEntry->perSTAKeyInfo.AuthSuiteCount = 0;
		for(count = 0; count < MAX_CIPHER_SUITE_NUM; count++){
			pEntry->perSTAKeyInfo.PairwiseCipherSuite[count] = RT_ENC_ALG_NO_CIPHER;
		}
		for(count = 0; count < MAX_AUTH_SUITE_NUM; count++){
			pEntry->perSTAKeyInfo.AuthSuite[count] = AKM_SUITE_NONE;
		}
		pEntry->perSTAKeyInfo.bPreAuth = FALSE;
		pEntry->perSTAKeyInfo.NumOfPTKReplayCounter = 0;
		pEntry->perSTAKeyInfo.NumOfGTKReplayCounter = 0;
	}
	
	RSNIE = PacketGetElement(asocpdu, EID_WPA2, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(RSNIE.Length != 0){
		pEntry->perSTAKeyInfo.SecLvl = RT_SEC_LVL_WPA2;
	}else{
		RSNIE = PacketGetElement(asocpdu, EID_Vendor, OUI_SUB_WPA, OUI_SUBTYPE_DONT_CARE);
		if(RSNIE.Length != 0){
			pEntry->perSTAKeyInfo.SecLvl = RT_SEC_LVL_WPA;
		}
		else if (pEntry->AuthAlg == OPEN_SYSTEM && bWPSEnable)
		{
			pEntry->Capability = GET_ASOC_REQ_CAPABILITY_INFO(asocpdu.Octet);
		
			if(pEntry->Capability & cPrivacy)
			{
				u1Byte count;
			
				pEntry->perSTAKeyInfo.GroupCipherSuite = RT_ENC_ALG_WEP;
				for(count = 0; count < MAX_CIPHER_SUITE_NUM; count++){
					pEntry->perSTAKeyInfo.PairwiseCipherSuite[count] = RT_ENC_ALG_WEP;
				}			
				pEntry->AuthMode = RT_802_11AuthModeOpen;
			}
			
			Match = TRUE;
			return Match;
		}
	}

	if(RSNIE.Length != 0){
		u1Byte	count;
		
		//2004/09/15, kcwu, parse WPA2 packet
		if(pEntry->perSTAKeyInfo.SecLvl == RT_SEC_LVL_WPA)
		{
			// Version
			pEntry->perSTAKeyInfo.SecVersion = GET_WPA_IE_VERSION(RSNIE.Octet);

			// Group Cipher
			GET_OUI_WITH_TYPE(GET_WPA_IE_GROUP_CIPHER_SUITE(RSNIE.Octet), tmpOUI, tmpOUIType);
			Sec_MapOUITypeToCipherSuite(tmpOUI, &tmpOUIType, RT_SEC_LVL_WPA, &(pEntry->perSTAKeyInfo.GroupCipherSuite));

			// Pairwise Cipher
			pEntry->perSTAKeyInfo.PairwiseCipherCount = GET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(RSNIE.Octet);
			for(count = 0; count < pEntry->perSTAKeyInfo.PairwiseCipherCount && count < MAX_CIPHER_SUITE_NUM; count ++)
			{
				GET_OUI_WITH_TYPE(GET_WPA_IE_PAIRWISE_CIPHER_SUITE_LIST(RSNIE.Octet, count), tmpOUI, tmpOUIType);
				Sec_MapOUITypeToCipherSuite(tmpOUI, &tmpOUIType, RT_SEC_LVL_WPA, &(pEntry->perSTAKeyInfo.PairwiseCipherSuite[count]));
			}

			// AKM Suite
			pEntry->perSTAKeyInfo.AuthSuiteCount = GET_WPA_IE_AKM_SUITE_CNT(RSNIE.Octet);
			for(count = 0; count < pEntry->perSTAKeyInfo.AuthSuiteCount && count < MAX_AUTH_SUITE_NUM; count ++)
			{
				GET_OUI_WITH_TYPE(GET_WPA_IE_AKM_SUITE_LIST(RSNIE.Octet, count), tmpOUI, tmpOUIType);
				Sec_MapOUITypeToAKMSuite(tmpOUI, &tmpOUIType, RT_SEC_LVL_WPA, &(pEntry->perSTAKeyInfo.AuthSuite[count]));
			}

			pEntry->perSTAKeyInfo.NumOfPTKReplayCounter = (u1Byte)GET_WPA_IE_CAP_PTKSA_REPLAY_COUNTER(RSNIE.Octet);
			pEntry->perSTAKeyInfo.NumOfGTKReplayCounter = (u1Byte)GET_WPA_IE_CAP_GTKSA_REPLAY_COUNTER(RSNIE.Octet);
		}
		else if(pEntry->perSTAKeyInfo.SecLvl == RT_SEC_LVL_WPA2)
		{
			// Version
			pEntry->perSTAKeyInfo.SecVersion = GET_RSN_IE_VERSION(RSNIE.Octet);

			// Group Cipher
			GET_OUI_WITH_TYPE(GET_RSN_IE_GROUP_CIPHER_SUITE(RSNIE.Octet), tmpOUI, tmpOUIType);
			Sec_MapOUITypeToCipherSuite(tmpOUI, &tmpOUIType, RT_SEC_LVL_WPA2, &(pEntry->perSTAKeyInfo.GroupCipherSuite));

			// Pairwise Cipher			
			pEntry->perSTAKeyInfo.PairwiseCipherCount = GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(RSNIE.Octet);

			// Bruce_Test
			DbgPrint("Client PairwiseCipherCount = %d\n", pEntry->perSTAKeyInfo.PairwiseCipherCount);
			for(count = 0; count < pEntry->perSTAKeyInfo.PairwiseCipherCount && count < MAX_CIPHER_SUITE_NUM; count ++)
			{
				GET_OUI_WITH_TYPE(GET_RSN_IE_PAIRWISE_CIPHER_SUITE_LIST(RSNIE.Octet, count), tmpOUI, tmpOUIType);				
				Sec_MapOUITypeToCipherSuite(tmpOUI, &tmpOUIType, RT_SEC_LVL_WPA2, &(pEntry->perSTAKeyInfo.PairwiseCipherSuite[count]));
				// Bruce_Test
				DbgPrint("Pairwise[%d] OUI = %02X-%02X-%02X-%02X, Pairwise = 0x%08X\n", count, tmpOUI[0], tmpOUI[1], tmpOUI[2], tmpOUIType, pEntry->perSTAKeyInfo.PairwiseCipherSuite[count]);
			}

			// AKM Suite
			pEntry->perSTAKeyInfo.AuthSuiteCount = GET_RSN_IE_AKM_SUITE_CNT(RSNIE.Octet);
			for(count = 0; count < pEntry->perSTAKeyInfo.AuthSuiteCount && count < MAX_AUTH_SUITE_NUM; count ++)
			{
				GET_OUI_WITH_TYPE(GET_RSN_IE_AKM_SUITE_LIST(RSNIE.Octet, count), tmpOUI, tmpOUIType);
				Sec_MapOUITypeToAKMSuite(tmpOUI, &tmpOUIType, RT_SEC_LVL_WPA2, &(pEntry->perSTAKeyInfo.AuthSuite[count]));
			}

			pEntry->perSTAKeyInfo.bPreAuth = (BOOLEAN)GET_RSN_IE_CAP_PREAUTH(RSNIE.Octet);

			pEntry->perSTAKeyInfo.NumOfPTKReplayCounter = (u1Byte)GET_RSN_IE_CAP_PTKSA_REPLAY_COUNTER(RSNIE.Octet);
			pEntry->perSTAKeyInfo.NumOfGTKReplayCounter = (u1Byte)GET_RSN_IE_CAP_GTKSA_REPLAY_COUNTER(RSNIE.Octet);
		}
		else
		{
			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("RSNIE parsing error!") );
		}		
	}


	//2 Step 2. Compare with Authenticator's RSNIE
	
	// Code reference: SelectNetworkBySSID()
	{
		int	i;
		Match = TRUE;
		{
			if( pMgntInfo->SecurityInfo.SecLvl != pEntry->perSTAKeyInfo.SecLvl )
				Match = FALSE;

			for( i=0; i < pEntry->perSTAKeyInfo.PairwiseCipherCount; i++ )
			{
				if(pEntry->perSTAKeyInfo.PairwiseCipherSuite[i] == pMgntInfo->SecurityInfo.PairwiseEncAlgorithm)
				{
					break;
				}
			}

			if( i == pEntry->perSTAKeyInfo.PairwiseCipherCount )
			{
				RT_TRACE_F(COMP_AP, DBG_LOUD, ("Cannot find matched pairwise cipher suite with ours (0x%08X)\n", pMgntInfo->SecurityInfo.PairwiseEncAlgorithm));
				Match = FALSE;
			}
			
		}
	}

	if(Match)
		pEntry->AuthMode = pMgntInfo->SecurityInfo.AuthMode;

	return Match;

}

void 
AP_OnEAPOL(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd
	)
{
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	PAUTH_GLOBAL_KEY_TAG	pGlInfo = &pMgntInfo->globalKeyInfo;
	OCTET_STRING		pduOS;
	pu1Byte				pSaddr;
	PRT_WLAN_STA		pEntry;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> AP_OnEAPOL()\n") );

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pSaddr = Frame_pSaddr(pduOS);

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("AP_OnEAPOL(): PacketLength=%d\n", pduOS.Length ) );
	RT_PRINT_ADDR( COMP_AUTHENTICATOR, DBG_LOUD, ("AP_OnEAPOL(): SourceAddress:"), pSaddr );

	pEntry = AsocEntry_GetEntry(pMgntInfo, pSaddr);
	if( pEntry == NULL )
	{
		// drop it.
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("AP_OnEAPOL(): Cannot find the STA in the table!!! drop it.\n") );
		return;
	}

	// Assign the EAPOL Frame header pointer. ( [AnnieNote] ex: from 01-03-00-5F-FE-00-89-...)

	//if(pEntry->QosMode == QOS_DISABLE)
	if(!Frame_ContainQc(pduOS))
	{
	        FillOctetString(pGlInfo->EAPOLMsgRecvd, pduOS.Octet+EAP_HDR_LEN, pduOS.Length-EAP_HDR_LEN);
	}
	else
	{
		FillOctetString(pGlInfo->EAPOLMsgRecvd, pduOS.Octet+EAP_HDR_LEN+2, pduOS.Length-EAP_HDR_LEN-2);
	}
	if( Frame_WEP(pduOS) )
	{
		pGlInfo->EAPOLMsgRecvd.Octet += pMgntInfo->SecurityInfo.EncryptionHeadOverhead;
		pGlInfo->EAPOLMsgRecvd.Length -= pMgntInfo->SecurityInfo.EncryptionHeadOverhead;
	}

	// Check type of EAPOL packet.
	if( (u1Byte)( *(pGlInfo->EAPOLMsgRecvd.Octet+1) ) == LIB1X_EAPOL_KEY )
	{
 		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("EAPOL-key\n") );
		Authenticator_OnEAPOLKeyRecvd( Adapter, &pEntry->perSTAKeyInfo, pduOS );
	}
 	else if( (u1Byte)( *(pGlInfo->EAPOLMsgRecvd.Octet+1) ) == LIB1X_EAPOL_START )
 	{
 		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("EAPOL-start\n") );
 		if( !KeyMgntStateIsWaitingEAPOLKey(&pEntry->perSTAKeyInfo) &&
			( pGlInfo->CurrentTimeSlot - pEntry->perSTAKeyInfo.TimeSlot_sendstart > 3 ) )	// don't response it in 3 sec. Added by Annie, 2005-07-12.
 		{
 			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Start 4-way handshake because of EAPOL-start.\n") );
			Authenticator_StateAUTHENTICATION2(Adapter, pEntry->perSTAKeyInfo.pWLanSTA);
 		}
		else
		{
 			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Don't responce!Maybe 4-way or 2-way handshaking\n") );
			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Drop the EAPOL-start packet.\n") );
		}
	}
	else
	{
		// Other EAPOL packets.
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("AP_OnEAPOL(): EAPOL packet with unknown type.\n") );
	}

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== AP_OnEAPOL()\n") );
}




//------------------------------------------------------------------------------
// HT(High Throughput) related operation
//------------------------------------------------------------------------------
VOID
AP_HTResetSTAEntry(
	PADAPTER		Adapter,
	PRT_WLAN_STA	pSTAEntry
)
{
	pSTAEntry->HTInfo.bEnableHT = FALSE;
	
	// LDPC support
	CLEAR_FLAGS(pSTAEntry->HTInfo.LDPC);
	
	// STBC support
	CLEAR_FLAGS(pSTAEntry->HTInfo.STBC);
	
	pSTAEntry->HTInfo.AMPDU_Factor = 0;
	pSTAEntry->HTInfo.AMSDU_MaxSize = 0;
	pSTAEntry->HTInfo.MPDU_Density = 0;	
	pSTAEntry->HTInfo.bSupportCck = FALSE;
	pSTAEntry->HTInfo.HTHighestOperaRate = 0;
}



BOOLEAN
AP_HTCheckHTCap(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry,
	IN	OCTET_STRING	asocpdu
)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	pu1Byte					pPeerHTCap;
	OCTET_STRING 			osHTCap = PacketGetElement(asocpdu, EID_HTCapability, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);

	//1 Reject STA to connect in N mode
	// If we do not support CCK rate in N mode, reject the B mode 
	if(IS_WIRELESS_MODE_HT_24G(Adapter) && pHTInfo->bCurSuppCCK==FALSE)
	{
		if(pEntry->WirelessMode == WIRELESS_MODE_B)
			return FALSE;
	}
	
	//1 Set Wireless Mode
	if(osHTCap.Length != 0)
	{
		pEntry->HTInfo.bEnableHT = TRUE;
		pPeerHTCap = (pu1Byte)osHTCap.Octet;
		pEntry->HTInfo.bCurRxReorderEnable = pHTInfo->bRegRxReorderEnable;
		if(pEntry->WirelessMode == WIRELESS_MODE_A)
			pEntry->WirelessMode = WIRELESS_MODE_N_5G;
		else
			pEntry->WirelessMode = WIRELESS_MODE_N_24G;
	}
	else
	{
		pEntry->HTInfo.bEnableHT = FALSE;
		return TRUE;
	}

	HTCheckHTCap(Adapter, pEntry, pPeerHTCap);
	
	return TRUE;
	
}



// Called by Ap_SendDisassocWithOldChnlWorkitemCallback
// Adapter = Ext Adapter
VOID
DelaySendBeaconTimerCallback(
	PRT_TIMER		pTimer
)
{
	PADAPTER	Adapter=(PADAPTER)pTimer->Adapter;
	PADAPTER	pExtAdapter = Adapter;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	u1Byte			SwBeaconType = BEACON_SEND_AUTO_HW;

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_HW_BEACON_SUPPORT, &SwBeaconType);

	pMgntInfo->BeaconState = BEACON_STARTING;

	// We need to configure the settings to let HW send Beacons.
	if(SwBeaconType <= BEACON_SEND_AUTO_SW)
	{
		BOOLEAN		bStopSendBeacon = FALSE;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_STOP_SEND_BEACON, (pu1Byte)&bStopSendBeacon);
	}

	if(AP_DetermineAlive(pExtAdapter) == FALSE)
	{
		RT_TRACE(COMP_AP, DBG_LOUD, ("DelaySendBeaconTimerCallback:() AP not started\n"));
		return;	
	}

	//[Win7] For Default-G _Extension-N20 case, we must reset SW and HW part. 2009.07.09, by Bohn
	AP_SetupStartApInfo(pExtAdapter);
	
	PlatformScheduleWorkItem( &(pMgntInfo->ApStartRequestWorkItem) );
}


/** Get AP state */
AP_STATE
GetAPState(
	IN	PADAPTER		pAdapter	
	)
{	
	return (AP_STATE)PlatformAtomicOr((pu1Byte)(&pAdapter->MgntInfo.APState),0);
}


/** Set AP state, return the old state */
VOID
SetAPState(
	IN	PADAPTER		pAdapter,
	IN	AP_STATE		NewSatate
	)
{
	PlatformAtomicExchange((pu4Byte)&pAdapter->MgntInfo.APState, (LONG)NewSatate);
}



RT_AP_TYPE
AP_DetermineApType(
	IN PADAPTER pAdapter
	)
{
	BOOLEAN bFakeApFlagSet = FALSE;
	PMGNT_INFO pMgntInfo = &pAdapter->MgntInfo;
	RT_AP_TYPE ApType = RT_AP_TYPE_NONE;
	
	if(IsDefaultAdapter(pAdapter))
		bFakeApFlagSet = TRUE;

	if(pMgntInfo->NdisVersion == RT_NDIS_VERSION_NONE_NDIS)
	{// not n6, n6, n62
		ApType = RT_AP_TYPE_NORMAL;
	}
	else if(pMgntInfo->NdisVersion == RT_NDIS_VERSION_5_0 ||
		pMgntInfo->NdisVersion == RT_NDIS_VERSION_5_1)
	{// n5
		if(bFakeApFlagSet)
			ApType = RT_AP_TYPE_NORMAL;
		else
			ApType = RT_AP_TYPE_EXT_AP;
	}
	else if(pMgntInfo->NdisVersion == RT_NDIS_VERSION_6_0 || (pMgntInfo->NdisVersion == RT_NDIS_VERSION_6_1))
	{// vista, only fake AP mode is valid
		ApType = RT_AP_TYPE_IBSS_EMULATED;
	}
	else if(pMgntInfo->NdisVersion >= RT_NDIS_VERSION_6_20)
	{// win7
		if(bFakeApFlagSet)
		{
			ApType = RT_AP_TYPE_IBSS_EMULATED;
		}
		else
		{
			ApType = RT_AP_TYPE_VWIFI_AP;
		}
	}
	else
	{
		ApType = RT_AP_TYPE_NORMAL;
	}
	RT_TRACE(COMP_AP, DBG_LOUD, 
		("AP_DetermineApType(): pMgntInfo->NdisVersion: %u, ApType is determined to be: %u\n", pMgntInfo->NdisVersion, ApType));
	return ApType;
	
}


BOOLEAN
AP_DetermineAlive(
	IN PADAPTER pAdapter
	)
{
	BOOLEAN 	bFakeApFlagSet = FALSE;
	PMGNT_INFO 	pMgntInfo = &pAdapter->MgntInfo;
	BOOLEAN		bAlive = FALSE;
	
	if(IsDefaultAdapter(pAdapter))
		bFakeApFlagSet = TRUE;

	if(!ACTING_AS_AP(pAdapter))
		bAlive = FALSE;
	else if(pMgntInfo->NdisVersion == RT_NDIS_VERSION_NONE_NDIS)
	{// not n6, n6, n62
		bAlive = FALSE;
	}
	else if(pMgntInfo->NdisVersion == RT_NDIS_VERSION_5_0 ||
		pMgntInfo->NdisVersion == RT_NDIS_VERSION_5_1)
	{// n5
		if(bFakeApFlagSet)
		{
			if(pMgntInfo->OpMode == RT_OP_MODE_AP)
				bAlive = TRUE;
			else
				bAlive = FALSE;
		}	
		else
		{
			if(GetAPState(pAdapter)==AP_STATE_STARTED)
				bAlive = TRUE;
			else
				bAlive = FALSE;
		}	
	}
	else if(pMgntInfo->NdisVersion == RT_NDIS_VERSION_6_1)
	{// vista, only fake AP mode is valid
		if(pMgntInfo->OpMode == RT_OP_MODE_AP)
			bAlive = TRUE;
		else
			bAlive = FALSE;
	}
	else if(pMgntInfo->NdisVersion >= RT_NDIS_VERSION_6_20)
	{// win7
		if(bFakeApFlagSet)
		{
			if(pMgntInfo->OpMode == RT_OP_MODE_AP)
				bAlive = TRUE;
			else
				bAlive = FALSE;
		}
		else
		{
			if(GetAPState(pAdapter)==AP_STATE_STARTED)
				bAlive = TRUE;
			else
				bAlive = FALSE;
		}
	}
	else
	{
		bAlive = FALSE;
	}
	RT_TRACE(COMP_AP, DBG_TRACE, 
		("AP_DetermineAlive(): pMgntInfo->NdisVersion: %u, Ap alive is determined to be: %u\n", pMgntInfo->NdisVersion, bAlive));
	return bAlive;
}



//
// Description:
//	Initialize the members of AP info.
// Arguments:
//	[in] pAdapter -
//		The adapter context.
// Return:
//	None.
// By Bruce, 2010-04-29.
//
VOID
AP_InitializeVariables(
	IN PADAPTER			pAdapter
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_AP_INFO		pApInfo = GET_AP_INFO(pMgntInfo);
	pu1Byte			pAcPara;
	u1Byte			WiFiOui[3] = {0x00, 0x50, 0xF2};

	pApInfo->bSupportWmm = TRUE;
	pApInfo->bSupportWmmUapsd = TRUE;
	pApInfo->WmmParaCnt = 0;

	PlatformZeroMemory(pApInfo->WmmAcParaBuf, WMM_PARAM_ELEMENT_SIZE);
	FillOctetString(pApInfo->osWmmAcParaIE, pApInfo->WmmAcParaBuf, WMM_PARAM_ELEMENT_SIZE);

	SET_WMM_PARAM_ELE_OUI(pApInfo->osWmmAcParaIE.Octet, WiFiOui);
	SET_WMM_PARAM_ELE_OUI_TYPE(pApInfo->osWmmAcParaIE.Octet, 2);
	SET_WMM_PARAM_ELE_OUI_SUBTYPE(pApInfo->osWmmAcParaIE.Octet, OUI_SUBTYPE_WMM_PARAM);
	SET_WMM_PARAM_ELE_VERSION(pApInfo->osWmmAcParaIE.Octet, 1);
	SET_WMM_PARAM_ELE_QOS_INFO_FIELD(pApInfo->osWmmAcParaIE.Octet, (((pApInfo->bSupportWmmUapsd) ? BIT7 : 0) | (pApInfo->WmmParaCnt & 0xF)));

	// These are WIFi default EDCA parameters.
	// Default BE
	pAcPara = &(pApInfo->osWmmAcParaIE.Octet[8]);
	SET_WMM_AC_PARAM_ACI(pAcPara, AC0_BE);
	SET_WMM_AC_PARAM_AIFSN(pAcPara, 3); // BE AIFS = 3
	SET_WMM_AC_PARAM_ECWMIN(pAcPara, 4); // BE CMinWin = 4.
	SET_WMM_AC_PARAM_ECWMAX(pAcPara, 10); // BE CMaxWin = 10.
	SET_WMM_AC_PARAM_TXOP_LIMIT(pAcPara, 0); // BE TxOP = 0.

	// Default BK
	pAcPara = &(pApInfo->osWmmAcParaIE.Octet[8 + 4]);
	SET_WMM_AC_PARAM_ACI(pAcPara, AC1_BK);
	SET_WMM_AC_PARAM_AIFSN(pAcPara, 7); // BK AIFS = 3
	SET_WMM_AC_PARAM_ECWMIN(pAcPara, 4); // BK CMinWin = 4.
	SET_WMM_AC_PARAM_ECWMAX(pAcPara, 10); // BK CMaxWin = 10.
	SET_WMM_AC_PARAM_TXOP_LIMIT(pAcPara, 0); // BK TxOP = 0.

	// Default VI
	pAcPara = &(pApInfo->osWmmAcParaIE.Octet[8 + 8]);
	SET_WMM_AC_PARAM_ACI(pAcPara, AC2_VI);
	SET_WMM_AC_PARAM_AIFSN(pAcPara, 2); // VI AIFS = 2
	SET_WMM_AC_PARAM_ECWMIN(pAcPara, 3); // VI CMinWin = 3.
	SET_WMM_AC_PARAM_ECWMAX(pAcPara, 4); // VI CMaxWin = 4.
	SET_WMM_AC_PARAM_TXOP_LIMIT(pAcPara, 94); // VI TxOP = 94.

	// Default VO
	pAcPara = &(pApInfo->osWmmAcParaIE.Octet[8 + 12]);
	SET_WMM_AC_PARAM_ACI(pAcPara, AC3_VO);
	SET_WMM_AC_PARAM_AIFSN(pAcPara, 2); // VO AIFS = 2
	SET_WMM_AC_PARAM_ECWMIN(pAcPara, 2); // VO CMinWin = 2.
	SET_WMM_AC_PARAM_ECWMAX(pAcPara, 3); // VO CMaxWin = 3.
	SET_WMM_AC_PARAM_TXOP_LIMIT(pAcPara, 47); // VO TxOP = 47.

	pApInfo->bSupportCountryIe = FALSE;
	PlatformZeroMemory(pApInfo->CountryIeBuf, MAX_IE_LEN);
	FillOctetString(pApInfo->osCountryIe, pApInfo->CountryIeBuf, 0);

	pApInfo->bSupportPowerConstraint = FALSE;
	PlatformZeroMemory(pApInfo->PowerConstraintBuf, MAX_DOT11_POWER_CONSTRAINT_IE_LEN);
	FillOctetString(pApInfo->osPowerConstraintIe, pApInfo->PowerConstraintBuf, 0);
}

//
// Description:
//	Append the WMM IE for the AP mode.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
//	[out] posFrame -
//		The packet to be appended with the WMM IE.
// Return:
//	None.
// By Bruce, 2010-04-29.
//
VOID
Ap_AppendWmmIe(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_AP_INFO		pApInfo = GET_AP_INFO(pMgntInfo);

	// This AP mode doesn't support WMM.
	if(!pApInfo->bSupportWmm || pApInfo->osWmmAcParaIE.Length == 0)
		return;

	PacketMakeElement(posFrame, EID_Vendor, pApInfo->osWmmAcParaIE);
}

//
// Description:
//	Append the Country IE for the AP mode.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
//	[out] posFrame -
//		The packet to be appended with the specified IE.
// Return:
//	None.
// By Bruce, 2010-05-14.
//
VOID
Ap_AppendCountryIE(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_AP_INFO		pApInfo = GET_AP_INFO(pMgntInfo);

	// This AP mode doesn't support WMM.
	if(!pApInfo->bSupportCountryIe || pApInfo->osCountryIe.Length == 0)
		return;

	PacketMakeElement(posFrame, EID_Country, pApInfo->osCountryIe);
}

//
// Description:
//	Append the Power Constraint IE for the AP mode.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context.
//	[out] posFrame -
//		The packet to be appended with the specified IE.
// Return:
//	None.
// By Bruce, 2010-05-14.
//
VOID
Ap_AppendPowerConstraintIE(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_AP_INFO		pApInfo = GET_AP_INFO(pMgntInfo);

	// This AP mode doesn't support WMM.
	if(!pApInfo->bSupportPowerConstraint || pApInfo->osPowerConstraintIe.Length == 0)
		return;

	PacketMakeElement(posFrame, EID_POWER_CONSTRAINT, pApInfo->osPowerConstraintIe);
}

//
// Description:
//	The callback function in AP mode for the tx packet feedback.
// Arguments:
//	[in] pAdapter -
//		The adapter address of the caller.
//	[in] pContext -
//		The information address of RT_TX_FEEDBACK_INFO.
// Return:
//	NULL
// Assumption:
//	The RT_TX_SPINLOCK is acquired.
// By Bruce, 2010-09-01.
//
VOID
Ap_PsTxFeedbackCallback(
	PADAPTER		pAdapter,
	const RT_TX_FEEDBACK_INFO * const pTxFeedbackInfo
)
{
	PRT_WLAN_STA			pEntry;
	BOOLEAN					WmmSp = FALSE;
	RT_STATUS				rtStatus;

	if(pTxFeedbackInfo == NULL)
		return;

	pEntry = (PRT_WLAN_STA)pTxFeedbackInfo->pContext;
	if(pEntry == NULL)
		return;
	
	RT_TRACE(COMP_AP, DBG_LOUD, ("Ap_PsTxFeedbackCallback()=====>\n"));

	// This tx packet feedback is the wmm ps packet.
	if(pTxFeedbackInfo->Reason & RT_TX_FEEDBACK_ID_AP_WMM_PS_PKT)
		WmmSp = TRUE;

	// The client isn't associated or not in power save mode.
	if(!pEntry->bAssociated || !pEntry->bPowerSave)
		return;

	// The packet with EOSP is sent successfully and we mark the flag of EOSP as ended.
	if((pTxFeedbackInfo->Reason & RT_TX_FEEDBACK_ID_AP_WMM_EOSP_ENDING) && pEntry->WmmEosp == RT_STA_EOSP_STATE_ENDING)
	{
		// RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "Ap_PsTxFeedbackCallback(): WMM state = RT_STA_EOSP_STATE_ENDED for sta = ", pEntry->MacAddr);
		pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDED;
		return;
	}

	if(WmmSp && pEntry->WmmEosp == RT_STA_EOSP_STATE_OPENED)
	{
		PRT_TCB				pTcb;
		OCTET_STRING		osMpdu;				

		if(!RTIsListEmpty(&pEntry->WmmPsQueue))
		{	
			pTcb = (PRT_TCB)RTRemoveHeadList(&pEntry->WmmPsQueue);
			FillOctetString(osMpdu, GET_FRAME_OF_FIRST_FRAG(pAdapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);

			if(RTIsListEmpty(&(pEntry->WmmPsQueue)))
			{
				// If this is the final packet, set the EOSP bit.
				pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDING;
			}


			if(TxFeedbackInstallTxFeedbackInfoForTcb(pAdapter, pTcb))
			{
				TxFeedbackFillTxFeedbackInfoUserConfiguration(
						pTcb, 
						(pEntry->WmmEosp == RT_STA_EOSP_STATE_OPENED) ? RT_TX_FEEDBACK_ID_AP_WMM_PS_PKT : RT_TX_FEEDBACK_ID_AP_WMM_EOSP_ENDING, 
						pTxFeedbackInfo->pAdapter, 
						Ap_PsTxFeedbackCallback, 
						(PVOID)pEntry
					);
			}
			else
			{
				if(pEntry->WmmEosp == RT_STA_EOSP_STATE_ENDING)
					pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDED;
			}


			// Set the more data if the EOSP ended so that notify the client we still queue packets.
			SET_80211_HDR_MORE_DATA(osMpdu.Octet, (pEntry->WmmEosp == RT_STA_EOSP_STATE_OPENED) ? 1 : 0);
			SET_QOS_CTRL_WMM_EOSP(osMpdu.Octet, (pEntry->WmmEosp == RT_STA_EOSP_STATE_OPENED) ? 0 : 1);

			// RT_TRACE_F(COMP_AP, DBG_LOUD, ("Tx Packet for WMM state = %d\n", pEntry->WmmEosp));
			rtStatus = TransmitTCB(pTxFeedbackInfo->pAdapter, pTcb);
			if(RT_STATUS_SUCCESS != rtStatus && RT_STATUS_PKT_DROP != rtStatus)
				RTInsertTailListWithCnt(Get_WAIT_QUEUE(pAdapter, pTcb->SpecifiedQueueID), &pTcb->List,GET_WAIT_QUEUE_CNT(pAdapter,pTcb->SpecifiedQueueID));
		}
		else
		{
			PRT_TX_LOCAL_BUFFER 	pBuf;
			u1Byte					AC = 0;
			
			if(GET_VO_UAPSD(pEntry->WmmStaQosInfo))
				AC = 7;
			else if(GET_VI_UAPSD(pEntry->WmmStaQosInfo))
				AC = 5;
			else if(GET_BE_UAPSD(pEntry->WmmStaQosInfo))
				AC = 3;
			else if(GET_BK_UAPSD(pEntry->WmmStaQosInfo))
				AC = 2;
			
			if(MgntGetBuffer(pTxFeedbackInfo->pAdapter, &pTcb, &pBuf))
			{
				ConstructNullFunctionData(
					pTxFeedbackInfo->pAdapter, 
					pBuf->Buffer.VirtualAddress,
					&pTcb->PacketLength,
					pEntry->MacAddr,
					TRUE,
					AC,
					TRUE,
					FALSE);


				if(TxFeedbackInstallTxFeedbackInfoForTcb(pAdapter, pTcb))
				{
					TxFeedbackFillTxFeedbackInfoUserConfiguration(
							pTcb, 
							RT_TX_FEEDBACK_ID_AP_WMM_EOSP_ENDING, 
							pTxFeedbackInfo->pAdapter, 
							Ap_PsTxFeedbackCallback, 
							(PVOID)pEntry
						);

					pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDING;
				}
				else
				{
					pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDED;
				}

				pTcb->bTxUseDriverAssingedRate = TRUE;
				pTcb->priority = AC;
				MgntSendPacket(pTxFeedbackInfo->pAdapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, pTxFeedbackInfo->pAdapter->MgntInfo.LowestBasicRate);
			}
		}
	}
}


VOID 
AP_AllPowerSaveDisable(
	IN PADAPTER		pAdapter
)
{	
	PADAPTER					Adapter  = GetDefaultAdapter(pAdapter);
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	IPSDisable(Adapter,FALSE, IPS_DISABLE_EXT_OPMODE);
	LeisurePSLeave(Adapter, LPS_DISABLE_AP_MODE);
	pPSC->bLeisurePs = FALSE;
	// We should not control bFwCtrlLPS here because it does not cause NIC enter PS mode,
	// it just setup the requirement for FwLPS mode. If disable bFwCtrlLPS, FwLPS mode
	// will fail in some cases as it resumes. 2010.06.24. Added by tynli.
	RT_TRACE(COMP_POWER, DBG_LOUD, ("AP_AllPowerSaveDisable()-----\n"));
}

VOID
AP_AllPowerSaveReturn(
	IN PADAPTER		pAdapter
)
{
	PADAPTER						Adapter  = GetDefaultAdapter(pAdapter);
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
		
	IPSReturn(Adapter, IPS_DISABLE_EXT_OPMODE);
	// Return LPS by UpdateLPSStatus(). by tynli. 2011.01.04.
	Adapter->HalFunc.UpdateLPSStatusHandler(
			Adapter, 
			pPSC->RegLeisurePsMode, 
			pPSC->RegPowerSaveMode);

	pPSC->bGpioRfSw = REGISTRY(Adapter, bRegGpioRfSw);

	RT_TRACE(COMP_POWER, DBG_LOUD, ("AP_AllPowerSaveReturn():pPSC->bInactivePs=%d, pPSC->bLeisurePs=%d,\n", pPSC->bInactivePs, pPSC->bLeisurePs));
}

//
// Description:
//	When any client enters the PS mode, this AP queues all multicast packets until the dtim arrives.
//	Here send all queued multicast packets by the specified queue id and fill the more data bit for each packet.
// Arguments:
//	[in] pAdapter -
//		The NIC adapter context of the AP mode.
//	[in] QueueID -
//		The HW queue where these multicast packets will be sent into.
// Return:
//	None
// Assumption:
//	Tx Spin Lock should not be acquired before calling this function.
// By Bruce, 2011-01-19.
//
VOID
AP_PS_SendAllMcastPkts(
	IN	PADAPTER	pAdapter,
	IN	u1Byte		QueueID
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_TCB	 		pTcb = NULL;
	RT_LIST_ENTRY	tmpList;
	OCTET_STRING	osMpdu;
	BOOLEAN			PreGroupTimSet = (BOOLEAN)pMgntInfo->TimBuf[2]; // Determine if the group traffic bit is set in the TIM info of the previous beacon.
	RT_STATUS		rtStatus;

	// Not AP mode.
	if(!ACTING_AS_AP(pAdapter))
		return;

	// The Group Traffic bit isn't set in TIM in the previous Beacon.
	if(!PreGroupTimSet)
		return;

	// No packets queued in the group queue.
	if(RTIsListEmpty(&(pMgntInfo->GroupPsQueue)))
		return;

	// RT_TRACE_F(COMP_AP, DBG_LOUD, ("Send all Mcst/Bcst pkts!\n"));

	RTInitializeListHead(&tmpList);
			
	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
	//
	// We remove all TCBs from the PsQueue and insert into the tmp list to prevent that the TCBs may be  recurcively 
	// inserted back into the PsQueue when the client enters and leaves PS mode frequently.
	// By Bruce, 2010-06-02.
	//
	while( !RTIsListEmpty(&(pMgntInfo->GroupPsQueue)) )
	{
		pTcb = (PRT_TCB)RTRemoveHeadList(&(pMgntInfo->GroupPsQueue));
		RTInsertTailList(&tmpList, &(pTcb->List));
	}
	while( !RTIsListEmpty(&tmpList) )
	{
		pTcb = (PRT_TCB)RTRemoveHeadList(&tmpList);
		FillOctetString(osMpdu, GET_FRAME_OF_FIRST_FRAG(pAdapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);

		// Send the multicast packet to the higer queue because we want these packets sent as soon as possible
		// after the beacon with TIM group bit set.
		pTcb->SpecifiedQueueID = QueueID;

		if(!RTIsListEmpty(&tmpList))
		{ // If there are still packets in queue, set more data bit.
			SET_80211_HDR_MORE_DATA(osMpdu.Octet, 1);
		}
		else
		{
			SET_80211_HDR_MORE_DATA(osMpdu.Octet, 0);
		}				

		rtStatus = TransmitTCB(pAdapter, pTcb);
		if(RT_STATUS_SUCCESS != rtStatus && RT_STATUS_PKT_DROP != rtStatus)
		{
			RTInsertTailListWithCnt(Get_WAIT_QUEUE(pAdapter, pTcb->SpecifiedQueueID), &pTcb->List,GET_WAIT_QUEUE_CNT(pAdapter,pTcb->SpecifiedQueueID));
		}
	}
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

/**********************************************************************/
//Function: AP_VHTCheckHTCap(): To Get bandwidth in VHT Case
//Author: Sherry
//Date: 20111227
/**********************************************************************/
VOID
AP_VHTResetSTAEntry	(
	PADAPTER		Adapter,
	PRT_WLAN_STA	pSTAEntry
)
{
	pSTAEntry->VHTInfo.bEnableVHT = FALSE;

	// LDPC support
	CLEAR_FLAGS(pSTAEntry->VHTInfo.LDPC);
	
	// STBC support
	CLEAR_FLAGS(pSTAEntry->VHTInfo.STBC);
}

BOOLEAN
AP_VHTCheckVHTCap(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry,
	IN	OCTET_STRING	asocpdu
)
{
	pu1Byte			pVHTCap;
	OCTET_STRING 	osVHTCap = PacketGetElement(asocpdu, EID_VHTCapability, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	//Check pEntry WirelessMode
	//1 Set Wireless Mode
	if(osVHTCap.Length != 0)
	{
		if((pEntry->WirelessMode != WIRELESS_MODE_N_24G) && (pEntry->WirelessMode != WIRELESS_MODE_N_5G))
			return FALSE;

		pEntry->VHTInfo.bEnableVHT = TRUE;
		pVHTCap = (pu1Byte)osVHTCap.Octet;
	
		if(pEntry->WirelessMode == WIRELESS_MODE_N_24G)
			pEntry->WirelessMode = WIRELESS_MODE_AC_24G;
		else
			pEntry->WirelessMode = WIRELESS_MODE_AC_5G;
	}
	else
	{
		pEntry->VHTInfo.bEnableVHT = FALSE;
		return TRUE;
	}

	VHTCheckVHTCap(Adapter, pEntry, pVHTCap);
	return TRUE;
}


VOID
AP_GetBandWidth(
	IN		PADAPTER	Adapter,
	OUT	 	CHANNEL_WIDTH*	pChnlBW,
	OUT		EXTCHNL_OFFSET* pExtChnlOffset
)
{
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(Adapter);

	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PMGNT_INFO 		pDefaultMgntInfo = GetDefaultMgntInfo(Adapter);

	PRT_CHANNEL_INFO pDefChnlInfo = GET_CHNL_INFO(pDefaultMgntInfo);

	
	EXTCHNL_OFFSET	ExtChnlOffset= EXTCHNL_OFFSET_NO_EXT;
	CHANNEL_WIDTH	ChnlWidth;


	ChnlWidth = (CHANNEL_WIDTH)AP_CheckBwWidth(pDefaultAdapter);
	RT_TRACE(COMP_AP, DBG_LOUD, ("AP_GetBandWidth(): set to %d mode Ch=%d\n", ChnlWidth, pMgntInfo->dot11CurrentChannelNumber));				

	//[Win 7] Two Port Issue	
	// Extension Port Must Set Offset Accroding to Default Port's if he connected
	// by Bohn, 2009.06.05	
	if(!ACTING_AS_AP(pDefaultAdapter) && pDefaultMgntInfo->bMediaConnect)
	{
		RT_TRACE(COMP_AP, DBG_LOUD, ("Def Connected.\n"));
		ExtChnlOffset = pDefChnlInfo->Ext20MHzChnlOffsetOf40MHz;
	}
	else
	{
		RT_TRACE(COMP_AP, DBG_LOUD, ("Def Not Connected.\n"));

		if(ChnlWidth == CHANNEL_WIDTH_80 && pMgntInfo->dot11CurrentChannelNumber == 165)
			ChnlWidth = CHANNEL_WIDTH_40;

		if(ChnlWidth == CHANNEL_WIDTH_20)
			ExtChnlOffset = EXTCHNL_OFFSET_NO_EXT;
		else
		{
			if(IS_WIRELESS_MODE_5G(Adapter))
				ExtChnlOffset = CHNL_GetExt20OffsetOf5G(pMgntInfo->dot11CurrentChannelNumber);
			else if(ChnlWidth == CHANNEL_WIDTH_80)
			{
				if(pMgntInfo->dot11CurrentChannelNumber == 1 || pMgntInfo->dot11CurrentChannelNumber == 5)
					ExtChnlOffset = EXTCHNL_OFFSET_UPPER;
				else 
					ExtChnlOffset = EXTCHNL_OFFSET_LOWER;
			}
			else 
				ExtChnlOffset =(pMgntInfo->dot11CurrentChannelNumber<=7)? EXTCHNL_OFFSET_UPPER:EXTCHNL_OFFSET_LOWER;
		}	
	}
	RT_TRACE(COMP_AP , DBG_LOUD, ("ExtChnlOffset=%d\n", ExtChnlOffset));				


	*pChnlBW =  ChnlWidth;
	*pExtChnlOffset = ExtChnlOffset;
}

VOID
AP_SetBandWidth(
	IN	PADAPTER	Adapter
)
{
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(Adapter);

	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PMGNT_INFO 		pDefaultMgntInfo = GetDefaultMgntInfo(Adapter);
	
	EXTCHNL_OFFSET	ExtChnlOffset= EXTCHNL_OFFSET_NO_EXT;
	CHANNEL_WIDTH	ChnlWidth;

	AP_GetBandWidth(Adapter, &ChnlWidth, &ExtChnlOffset);
	RT_TRACE(COMP_AP, DBG_LOUD,("AP_SetBandWidth: ChnlWidth %d \n", ChnlWidth));
			
	if(Adapter->bInHctTest )
	{
		if(!((pDefaultMgntInfo->mAssoc || pDefaultMgntInfo->mIbss) && (!ACTING_AS_AP(pDefaultAdapter))))
		{
			CHNL_SetBwChnl(Adapter,  pMgntInfo->dot11CurrentChannelNumber, ChnlWidth, ExtChnlOffset);
			CHNL_SetChnlInfoFromDestPort(pDefaultAdapter, Adapter);
		}
		else
		{
			CHNL_SetBwChnl(Adapter,  pMgntInfo->dot11CurrentChannelNumber, ChnlWidth, ExtChnlOffset);
	        }
	}
	else
	{
		CHNL_SetBwChnl(Adapter,  pMgntInfo->dot11CurrentChannelNumber, ChnlWidth, ExtChnlOffset);	
	}
}


u1Byte
AP_CheckBwWidth	(
	PADAPTER	Adapter
)
{	
	u1Byte ret = (u1Byte)CHANNEL_WIDTH_80;
	
	// XP, Win7 Follow default port first. (Two port)
	// XP, Vista bCurBW40MHz  <- pHTInfo->bRegBW40MHz (One port)
	GetTwoPortSharedResource(Adapter,TWO_PORT_SHARED_OBJECT__BW,NULL,(pu1Byte)(&ret));

	if(Adapter->bInHctTest)
		ret = CHANNEL_WIDTH_20;

	if (Adapter->MgntInfo.bRegVht24g == 1)
		ret = CHANNEL_WIDTH_40;
	
	RT_TRACE(COMP_AP, DBG_TRACE, ("AP_CheckBwWidth(): set to %s mode\n", (ret==CHANNEL_WIDTH_80)?"80MHz":"20_40MHz"));
	return ret;

}

//
// Description: Handle incoming Association Request.
// Output:		Return TRUE if we will response the incoming auth frame, FALSE o.w.. 
//
// 2005.06.01, by rcnjko.
//
BOOLEAN
AP_OnAsocReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	OCTET_STRING	asocpdu
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	pu1Byte			asSta;
	PRT_WLAN_STA	pEntry = NULL;
	u2Byte			AsocStatusCode;
	OCTET_STRING	osSupRates,	osExSupRates, osQosInfo, osSsid;
	int				i;
	u1Byte			rate;
	BOOLEAN			bQosSTA = FALSE, bIndicate = TRUE;
	BOOLEAN			bSendRsp = TRUE;
	u8Byte			curTime = PlatformGetCurrentTime();
	

	FunctionIn(COMP_MLME);
	// Check Bssid.
	if(!eqMacAddr(pMgntInfo->Bssid, Frame_Addr3(asocpdu)))
	{
		RT_PRINT_ADDR(COMP_MLME, DBG_WARNING, "AP_OnAsocReq(): [WARNING] Mismatch Bssid = ", Frame_Addr3(asocpdu));
		return FALSE;
	}

	// Check ssid.
	osSsid = PacketGetElement(asocpdu, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(!CompareSSID(osSsid.Octet, osSsid.Length, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length))
	{
		RT_PRINT_STR(COMP_MLME, DBG_WARNING, "AP_OnAsocReq(): [WARNING] Mismatch SSID = ", osSsid.Octet, osSsid.Length);
		return FALSE;
	}

	// Initialize default value for Aassociation Response.
	AsocStatusCode = StatusCode_Unspecified_failure;

	// Gather information from incoming Associate Request frame.
	asSta = Frame_Addr2(asocpdu);
	RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "AP_OnAsocReq(): Associating Addr = ", asSta);

	// Check if the STA is authenticated.
	pEntry = AsocEntry_GetEntry(pMgntInfo, asSta);
	if(pEntry == NULL)
	{
		RT_TRACE_F(COMP_MLME, DBG_WARNING, ("[WARNING] pEntry == NULL\n"));
		return FALSE;
	}
	if(	!((pEntry->AuthAlg == OPEN_SYSTEM && pEntry->AuthPassSeq == 2) ||
		  (pEntry->AuthAlg == SHARED_KEY && pEntry->AuthPassSeq == 4)    ) )
	{
		RT_TRACE_F(COMP_MLME, DBG_WARNING, ("[WARNING] !((pEntry->AuthAlg == OPEN_SYSTEM && pEntry->AuthPassSeq == 2\n"));
		return FALSE;
	}

	// Check RSN-IE if necessary.
	if(   pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPAPSK ||	
		pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPA2PSK )
	{
		if( !AP_CheckRSNIE(Adapter, pEntry, asocpdu) )
		{
			RT_TRACE_F(COMP_AP, DBG_LOUD, ("AP_CheckRSNIE return fail\n"));
			// If under WDI, let WDI decide whether to accept or not
			if (WPS_OnAsocReq(Adapter, &bIndicate) == FALSE)
			{
				if( !OS_SUPPORT_WDI(Adapter) )
				{
					RT_TRACE_F(COMP_AP, DBG_LOUD, ("Skip this association req because AP_CheckRSNIE returns failure and WPS_OnAsocReq is false\n"));
					return	FALSE;
				}
			}
		}
	}

	if(pEntry->bAssociated)
	{
		//
		// If the STA failed to receive the AssocRsp previously sent, 
		// it may keep retry but we have already indicate asoc complete to OS and
		// 4-way is started. In this case, we indicate disassociation and process the AsocReq
		// to prevent association failure. 2011.05.31, haich.
		//
		if(GET_80211_HDR_RETRY(asocpdu.Octet))
		{
			RT_TRACE_F(COMP_MLME, DBG_WARNING, ("[WARNING] Received Association Req from associated clietn with retry bit, skip this packet!\n"));
			return FALSE;
		}
		pMgntInfo->pCurrentSta = pEntry;
		DrvIFIndicateDisassociation(Adapter, unspec_reason, pEntry->MacAddr);
		RT_TRACE_F(COMP_MLME, DBG_WARNING, ("[WARNING] Received Association Req from associated clietn without retry bit, idnicate disassociation event!\n"));
		//return FALSE;
	}

	//vivi add this if condition, if we have get AID before, do not get different AID second time, 20110719
	if(pEntry->AID == 0)
	{
		// Update association table.
		pEntry->AID = AsocEntry_AssignAvailableAID(pMgntInfo, asSta);
	}

	pEntry->AssociatedMacId = (u1Byte) MacIdRegisterMacIdForAssociatedID(Adapter, MAC_ID_OWNER_INFRA_AP, pEntry->AID);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("pentry AID %d AssociatedMacId %d\n", pEntry->AID, pEntry->AssociatedMacId));

	//YJ, add for TX RPT, 111213
	if(pMgntInfo->MaxMACID < pEntry->AssociatedMacId)
	{
		pMgntInfo->MaxMACID = pEntry->AssociatedMacId;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_TX_RPT_MAX_MACID, (pu1Byte)(&(pMgntInfo->MaxMACID)));
	}
//}
	pEntry->bAssociated = TRUE;
	AsocEntry_UpdateTimeStamp(pEntry);

	// Get the supported rate of the STA.
	pEntry->bdSupportRateEXLen = 0;
	osSupRates = PacketGetElement(asocpdu, EID_SupRates, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(osSupRates.Length != 0)
	{
		CopyMem( pEntry->bdSupportRateEXBuf, osSupRates.Octet, osSupRates.Length);
		pEntry->bdSupportRateEXLen = osSupRates.Length;
	}
	osExSupRates = PacketGetElement(asocpdu, EID_ExtSupRates, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(osExSupRates.Length != 0)
	{
		CopyMem( pEntry->bdSupportRateEXBuf+pEntry->bdSupportRateEXLen, osExSupRates.Octet, osExSupRates.Length);
		pEntry->bdSupportRateEXLen += osExSupRates.Length;
	}

	// Get highest supported rate of the STA.
	pEntry->HighestOperaRate = pMgntInfo->LowestBasicRate & 0x7f;
	for(i = 0; i < pEntry->bdSupportRateEXLen; i++)
	{
		//
		// Check if this rate is supported by ourself
		// If we do not support this rate, just not add to support rate list.
		//
		if(!MgntIsRateSupport(pEntry->bdSupportRateEXBuf[i], pMgntInfo->dot11OperationalRateSet))
			continue;
	
		if( (pEntry->bdSupportRateEXBuf[i] & 0x7f) > pEntry->HighestOperaRate)
			pEntry->HighestOperaRate = pEntry->bdSupportRateEXBuf[i] & 0x7f;
	}

	// Update WirelessMode. 1st part handle the 2.4G band, 2nd part handle 5G band.
	if(IS_WIRELESS_MODE_24G(Adapter))
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

	// STA is associated sucessfully.
	AsocStatusCode = StatusCode_success;

	// AP QOS mode select
	if((pMgntInfo->pStaQos->CurrentQosMode & QOS_WMM) 
		||(pMgntInfo->pStaQos->CurrentQosMode & QOS_WMMSA))
	{
		osQosInfo = PacketGetElement(asocpdu, EID_Vendor, OUI_SUB_WMM, OUI_SUBTYPE_WMM_INFO);
		if(osQosInfo.Length != 0)
		{
			pEntry->QosMode = QOS_WMM | QOS_WMMSA;
			bQosSTA = TRUE;
			pEntry->WmmStaQosInfo = GET_WMM_INFO_ELE_QOS_INFO_FIELD(osQosInfo.Octet);
		}
		else
		{
			pEntry->QosMode = QOS_DISABLE;
			bQosSTA = FALSE;
		}
	}
	else if(pMgntInfo->pStaQos->CurrentQosMode >= QOS_EDCA)
	{
		osQosInfo = PacketGetElement(asocpdu, EID_QoSCap, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
		if(osQosInfo.Length != 0)
		{
			pEntry->QosMode = QOS_EDCA | QOS_HCCA;
			bQosSTA = TRUE;
		}
		else
		{
			pEntry->QosMode = QOS_DISABLE;
			bQosSTA = FALSE;
		}
	}

	// Update STA HT capability, and supported rate sets for all of the STA.
	if(pMgntInfo->pHTInfo->bCurrentHTSupport && pMgntInfo->pStaQos->CurrentQosMode >= QOS_WMM)
	{
		if(AP_HTCheckHTCap(Adapter, pEntry, asocpdu) == FALSE)
			return FALSE;

		if(pMgntInfo->pVHTInfo->bCurrentVHTSupport)
		{
			if(AP_VHTCheckVHTCap(Adapter, pEntry, asocpdu) == FALSE)
				return FALSE;
		}
	}


	// Enable protection mode and Disable short slot time if an [B-mode STA joined] or[ G-mode Connect to Soft N AP]		
	// Do not update bUseProtection bit when G mode STA add or leave. 2010.11.24. by tynli.
	if(pEntry->WirelessMode == WIRELESS_MODE_B)
	{
		ActUpdate_ProtectionMode(Adapter, TRUE);
		
		if(pEntry->WirelessMode == WIRELESS_MODE_B)
			pMgntInfo->mCap &= (~cShortSlotTime);
		else
			pMgntInfo->mCap |= (cShortSlotTime);
		ActUpdate_mCapInfo(Adapter, pMgntInfo->mCap);
	}

	// Update the Operation mode field in HT Info element.
	if(IS_WIRELESS_MODE_N(Adapter))
	{
		if(pEntry->WirelessMode == WIRELESS_MODE_B || pEntry->WirelessMode == WIRELESS_MODE_G)
			pMgntInfo->pHTInfo->CurrentOpMode = HT_OPMODE_MIXED;
		else
		{
			if(CHNL_RUN_ABOVE_40MHZ(pMgntInfo) && (pEntry->WirelessMode == WIRELESS_MODE_N_24G))
				pMgntInfo->pHTInfo->CurrentOpMode = HT_OPMODE_40MHZ_PROTECT;
			else
				pMgntInfo->pHTInfo->CurrentOpMode = HT_OPMODE_NO_PROTECT;
		}
	}


	//
	// Association Request.
	//
	AsocEntry_UpdateAsocInfo(Adapter, 
		pEntry->MacAddr, 
		NULL, 
		0, 
		ALLOCATE_ASOC_REQ);
	AsocEntry_UpdateAsocInfo(Adapter, 
		pEntry->MacAddr, 
		asocpdu.Octet, 
		asocpdu.Length, 
		UPDATE_ASOC_REQ);

	//
	// IE from OS that is to be carried in Association Response.
	//
	AsocEntry_UpdateAsocInfo(Adapter, 
		pEntry->MacAddr, 
		NULL, 
		0, 
		ALLOCATE_ASOC_RSP_IE_FROM_OS);
	
	if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_VWIFI_AP && bIndicate)
	{
		//
		// OS will make the direct OID call OID_DOT11_INCOMING_ASSOCIATION_DECISION
		// in the status indication thread.
		// So after the status indication returns, the association decision shall be
		// cached in the station entry already. We need to process it now.
		//
		pMgntInfo->pCurrentSta = pEntry;
		DrvIFIndicateIncommingAssociationStart(Adapter);		
		DrvIFIndicateIncommingAssocReqRecv(Adapter);
		if(OS_SUPPORT_WDI(Adapter))
		{// let WDI to decide whether to send the rsp
			bSendRsp = FALSE;
		}
		else 
		if(!pEntry->bOsDecisionMade)
		{
			RT_TRACE_F(COMP_INDIC, DBG_WARNING, 
				("[WARNING] OS did not make decision whether to accept the AsocReq.\n"));

			AsocStatusCode = StatusCode_Unspecified_failure;
		}
		else if(pEntry->bNotAcceptedByOs)
		{	
			RT_TRACE_F(COMP_INDIC, DBG_WARNING, 
				("[WARNING] the AssocReq is not accepted by OS.\n"));

			AsocStatusCode = pEntry->OsReasonCode;
			pEntry->bNotAcceptedByOs = FALSE; // clear
		}
	}
#if (WPS_SUPPORT == 1)
	pEntry->SimpleConfigInfo = PacketGetElement( asocpdu, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUBTYPE_DONT_CARE);
#endif	// #if (WPS_SUPPORT == 1)

	// Save parameters ---------------------------------------------
	// This client is associated our AP successfully.
	// These actions must be updated before sending assoication response because there may be some packets will be
	// received immediately after sending the response packet.
	if(AsocStatusCode == StatusCode_success)	
	{ 
		// Change the last scan complete time to block scan request for a while from the default port.
		// If the scan process is executed, the next handshake may fail because this port is not in the
		// correct channel now.
		MultiportRecordLastScanTime(Adapter);
		Adapter->LastConnectionActionTime = curTime;
		pEntry->Capability = GET_ASOC_REQ_CAPABILITY_INFO(asocpdu.Octet);
		pEntry->usListenInterval = GET_ASOC_REQ_LISTEN_INTERVAL(asocpdu.Octet);
		pEntry->AssociationUpTime = curTime;

		P2P_OnAssocReqAccept(Adapter, pRfd, &asocpdu);
		WFD_OnAssocReqAccept(Adapter, pRfd, &asocpdu);
	}

	if(bSendRsp)
	{
		AP_SendAsocRsp(Adapter, pEntry, PacketGetType(asocpdu) == Type_Reasoc_Req, bQosSTA, AsocStatusCode);
	}

	AsocEntry_UpdateAsocInfo(Adapter,
			pEntry->MacAddr,
			NULL,
			0,
			FREE_ASOC_REQ);

	AsocEntry_UpdateAsocInfo(Adapter,
			pEntry->MacAddr,
			NULL,
			0,
			FREE_ASOC_RSP_IE_FROM_OS);	
	
	if(pEntry->HTInfo.bEnableHT)
	{
		u4Byte	Length = 0;
		OCTET_STRING 		BroadcomElement;
		FillOctetString(BroadcomElement, NULL, 0);

		BroadcomElement = PacketGetElement( asocpdu, EID_Vendor, OUI_SUB_BROADCOM_IE_1, OUI_SUBTYPE_DONT_CARE);
		Length += BroadcomElement.Length;
		BroadcomElement = PacketGetElement( asocpdu, EID_Vendor, OUI_SUB_BROADCOM_IE_2, OUI_SUBTYPE_DONT_CARE);
		Length += BroadcomElement.Length;
		BroadcomElement = PacketGetElement( asocpdu, EID_Vendor, OUI_SUB_BROADCOM_IE_3, OUI_SUBTYPE_DONT_CARE);
		Length += BroadcomElement.Length;
		
		if(	pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40||
			pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP104
			||pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_TKIP)
		{
			//DbgPrint("WEP DON't Use AMSDU\n");
			pEntry->IOTAction = 0;
		}
		
		OnMimoPs(Adapter, pRfd,&asocpdu);
		
	}

	FunctionOut(COMP_MLME);

	return TRUE;
}

VOID
AP_SendAsocRsp(
	IN  ADAPTER					*pAdapter,
	IN  RT_WLAN_STA				*pEntry,
	IN  BOOLEAN					bReassoc,
	IN  BOOLEAN					bQosSta,
	IN  u2Byte					status
	)
{
	MGNT_INFO					*pMgntInfo = &pAdapter->MgntInfo;
	MGNT_INFO 					*pDefaultMgntInfo = GetDefaultMgntInfo(pAdapter);
	BOOLEAN						bUseRAMask = FALSE;
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(pAdapter);
	PDM_ODM_T					pDM_Odm = &pHalData->DM_OutSrc;

	// Send AssocRsp.
	SendAssociateRsp(
		pAdapter,
		pEntry->MacAddr, // asocStaAddr
		pMgntInfo->mCap, // asocCap
		status, // asocStatusCode
		pEntry->AID, // asocID
		bReassoc, // bReAssocRsp
		bQosSta
		);

	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_USE_RA_MASK, &bUseRAMask);
	if(bUseRAMask)
	{
		AP_InitRateAdaptiveState(pAdapter, pEntry);
		pAdapter->HalFunc.UpdateHalRAMaskHandler(
									pAdapter,
									pEntry->AssociatedMacId,
									pEntry,	// have to consider we are G but sta are N
									0
									);
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

	if(!pMgntInfo->bWiFiConfg && !pAdapter->bInHctTest)
		pAdapter->HalFunc.HalRxAggrHandler(pAdapter, TRUE);

	// Added by Annie, 2005-06-30.
	if( pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPAPSK  ||  pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPA2PSK)
	{
#if (WPS_SUPPORT == 1)	
		PSIMPLE_CONFIG_T pSimpleConfig = GET_SIMPLE_CONFIG(pDefaultMgntInfo);
		BOOLEAN bWpsInProgress = pSimpleConfig->bEnabled;
#else
		BOOLEAN bWpsInProgress = FALSE;
#endif
		if( (MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_NORMAL ||
			MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_IBSS_EMULATED ||
			MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_EXT_AP) &&
			!bWpsInProgress)
				Authenticator_OnAuthenticationRequest(pAdapter, &pEntry->perSTAKeyInfo);
	}

	
	//
	// 061227, rcnjko: 
	// Just Indicate association start and association complete events,
	// connection start and connection complete events will be indicate up 
	// when UI set the fake profile before.
	//
	if(MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_IBSS_EMULATED
		 || MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_LINUX)
	{
		MgntUpdateAsocInfo(pAdapter, UpdateAsocPeerAddr, pEntry->MacAddr, 6); // 061204, rcnjko: this is a MUST before indicating association related event.
		DrvIFIndicateAssociationStart(pAdapter);
		DrvIFIndicateAssociationComplete(pAdapter, RT_STATUS_SUCCESS);
	}
	else if(MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_VWIFI_AP)
	{
		pMgntInfo->pCurrentSta = pEntry;
		DrvIFIndicateIncommingAssociationComplete(pAdapter, status);
	}
	
	AsocEntry_UpdateAsocInfo(pAdapter,
			pEntry->MacAddr,
			NULL,
			0,
			FREE_ASOC_RSP);
	
}

#else


#endif	
