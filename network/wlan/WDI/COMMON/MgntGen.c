#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "MgntGen.tmh"
#endif

#define BEACON_FRAME_LEN	512

static VOID
TbttPollingWorkItemCallback(
	IN PVOID		pContext
	);


BOOLEAN
IsMgntNDPA(
	pu1Byte		pdu
)
{
	BOOLEAN ret = FALSE;
	if(IsMgntActionNoAck(pdu) && GET_80211_HDR_ORDER(pdu))
	{
		if(GET_HT_CTRL_NDP_ANNOUNCEMENT(pdu+sMacHdrLng) == 1)
			ret = TRUE;
	}
	return ret;
}


BOOLEAN
MgntIsRateSupport(
	u1Byte			nRate,
	OCTET_STRING	osRateSet
	)
{
	u1Byte	i;
	for(i = 0; i < osRateSet.Length; i++)
	{
		if((nRate & 0x7f) == (osRateSet.Octet[i] & 0x7f))
			return TRUE;
	}
	return FALSE;
}


//
//	Description:
//		Initialize RegSuppRateSets.
//
VOID
MgntInitRateSet(
	PADAPTER			Adapter
)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	POCTET_STRING	RegSuppRateSets = &(pMgntInfo->RegSuppRateSets[0]);
	pu1Byte			RegHTSuppRateSets = &(pMgntInfo->RegHTSuppRateSet[0]);
	pu1Byte			RegVHTSuppRateSets = &(pMgntInfo->RegVHTSuppRateSet[0]);
	u1Byte			i;

	FillOctetString( pMgntInfo->mBrates, pMgntInfo->mBratesBuf, 0 );
	FillOctetString( pMgntInfo->SupportRatesfromBCN, pMgntInfo->SupportRatesfromBCNBuf, 0 );
	FillOctetString( pMgntInfo->dot11OperationalRateSet, pMgntInfo->dot11OperationalRateBuf, 0 );
	FillOctetString( pMgntInfo->SupportedRates, pMgntInfo->SupportedRatesBuf, 0 );
	FillOctetString( pMgntInfo->Regdot11OperationalRateSet, pMgntInfo->Regdot11OperationalRateBuf, 0 );

	for(i = 0; i < MAX_WIRELESS_MODE_CNT; i++)
	{
		FillOctetString( RegSuppRateSets[i], pMgntInfo->RegSuppRateSetsBuf[i], 0 );
	}

 	// "|0x80" to tag it as basic rate.
 	
	// WIRELESS_MODE_A:
	// WIRELESS_MODE_N_5G:
	// WIRELESS_MODE_AC_5G:
	RegSuppRateSets[0].Octet[0] = MGN_6M	| 0x80;
	RegSuppRateSets[0].Octet[1] = MGN_9M;
	RegSuppRateSets[0].Octet[2] = MGN_12M	| 0x80;
	RegSuppRateSets[0].Octet[3] = MGN_18M;
	RegSuppRateSets[0].Octet[4] = MGN_24M	| 0x80;
	RegSuppRateSets[0].Octet[5] = MGN_36M;
	RegSuppRateSets[0].Octet[6] = MGN_48M;
	RegSuppRateSets[0].Octet[7] = MGN_54M;
	RegSuppRateSets[0].Length = 8;

	// WIRELESS_MODE_B:
	RegSuppRateSets[1].Octet[0] = MGN_1M 	| 0x80;
	RegSuppRateSets[1].Octet[1] = MGN_2M 	| 0x80;
	RegSuppRateSets[1].Octet[2] = MGN_5_5M	| 0x80;
	RegSuppRateSets[1].Octet[3] = MGN_11M	| 0x80;
	RegSuppRateSets[1].Length = 4;

	// WIRELESS_MODE_G:
	// WIRELESS_MODE_N_24G:
	// WIRELESS_MODE_AC_24G:
	RegSuppRateSets[2].Octet[0] = MGN_1M 	| 0x80;
	RegSuppRateSets[2].Octet[1] = MGN_2M 	| 0x80;
	RegSuppRateSets[2].Octet[2] = MGN_5_5M | 0x80;
	RegSuppRateSets[2].Octet[3] = MGN_11M	| 0x80;
	RegSuppRateSets[2].Octet[4] = MGN_6M;
	RegSuppRateSets[2].Octet[5] = MGN_9M;
	RegSuppRateSets[2].Octet[6] = MGN_12M;
	RegSuppRateSets[2].Octet[7] = MGN_18M;
	RegSuppRateSets[2].Octet[8] = MGN_24M;
	RegSuppRateSets[2].Octet[9] = MGN_36M;
	RegSuppRateSets[2].Octet[10] = MGN_48M;
	RegSuppRateSets[2].Octet[11] = MGN_54M;
	RegSuppRateSets[2].Length = 12;


	// TODO: may be different if we have differnt number of antenna
	RegHTSuppRateSets[0] = 0xFF;	//support MCS 0~7
	RegHTSuppRateSets[1] = 0xFF;	//support MCS 8~15
	RegHTSuppRateSets[2] = 0xFF;	//support MCS 16~23

	RegVHTSuppRateSets[0] = 0xff;	//Support 1SS~3SS MCS 0~9
	RegVHTSuppRateSets[1] = 0xff;	//Not support 4SS MCS 0~9
}


//
//	Description:
//		Update SupportedRates to that of current wirles mode.
//
VOID
MgntRefreshSuppRateSet(
	PADAPTER			Adapter
)
{
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	PRT_HIGH_THROUGHPUT			pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);

	switch(pMgntInfo->dot11CurrentWirelessMode)
	{
		case WIRELESS_MODE_A:
		case WIRELESS_MODE_N_5G:
		case WIRELESS_MODE_AC_5G:
		case WIRELESS_MODE_AC_ONLY:
			pMgntInfo->Regdot11OperationalRateSet = pMgntInfo->RegSuppRateSets[0];
		break;

		case WIRELESS_MODE_B:
			pMgntInfo->Regdot11OperationalRateSet = pMgntInfo->RegSuppRateSets[1];
		break;

		case WIRELESS_MODE_G:
		case WIRELESS_MODE_N_24G:
		case WIRELESS_MODE_AC_24G:
			pMgntInfo->Regdot11OperationalRateSet = pMgntInfo->RegSuppRateSets[2];
		break;
		
	default:
		pMgntInfo->Regdot11OperationalRateSet.Length = 0;	// No legacy rate support!!
		break;
	}	

	if(IS_WIRELESS_MODE_N(Adapter))
	{
		BOOLEAN		b1SSSupport = FALSE;
		u1Byte 		Rf_Type = RT_GetRFType(Adapter);

		PlatformMoveMemory(	pMgntInfo->Regdot11HTOperationalRateSet, 
								pMgntInfo->RegHTSuppRateSet, 
								16	);
		if(Rf_Type == RF_1T1R || Rf_Type == RF_1T2R)
			b1SSSupport = TRUE;
		else if(pHTInfo->nTxSPStream == 1 || pHTInfo->nRxSPStream == 1)
			b1SSSupport = TRUE;
		else if(Rf_Type == RF_2T2R  || Rf_Type == RF_2T3R || Rf_Type == RF_2T4R)
			pMgntInfo->Regdot11HTOperationalRateSet[2] = 0x00;

		if(b1SSSupport)
			pMgntInfo->Regdot11HTOperationalRateSet[1] = pMgntInfo->Regdot11HTOperationalRateSet[2] = 0x00;
	}	
	else
		PlatformZeroMemory(pMgntInfo->Regdot11HTOperationalRateSet, 16);

	if(IS_WIRELESS_MODE_AC(Adapter))
	{
		u1Byte 		Rf_Type = RT_GetRFType(Adapter);

		PlatformMoveMemory(	pMgntInfo->Regdot11VHTOperationalRateSet, 
								pMgntInfo->RegVHTSuppRateSet, 
								2	);
		if(Rf_Type == RF_1T1R || Rf_Type == RF_1T2R)
			pMgntInfo->Regdot11VHTOperationalRateSet[0] = 0xfe;	// Support 1SS MCS 0~9
		else if(pVHTInfo->nTxSPStream == 1 || pVHTInfo->nRxSPStream == 1)
			pMgntInfo->Regdot11VHTOperationalRateSet[0] = 0xfe;	// Support 1SS MCS 0~9
		else if(Rf_Type == RF_2T2R  || Rf_Type == RF_2T3R || Rf_Type == RF_2T4R)
			pMgntInfo->Regdot11VHTOperationalRateSet[0] = 0xfa;	//Support 1SS ~ 2SS MCS 0~9
		else if(Rf_Type == RF_3T3R)
			pMgntInfo->Regdot11VHTOperationalRateSet[0] = 0xea;	//Support 1SS ~ 3SS MCS 0~9

		if(pVHTInfo->bUpdateMcsCap)
			VHTMaskSuppDataRate(pMgntInfo->Regdot11VHTOperationalRateSet, pVHTInfo->nRxSPStream, pVHTInfo->McsCapability);

		pMgntInfo->RegVHTHighestOperaRate = VHTGetHighestMCSRate(	Adapter, 
																	pMgntInfo->Regdot11VHTOperationalRateSet);
	}	
	else
		PlatformZeroMemory(pMgntInfo->Regdot11VHTOperationalRateSet, 2);

	// Fill up SupportedRates as Regdot11OperationalRateSet.
	CopyMemOS( &pMgntInfo->SupportedRates, pMgntInfo->Regdot11OperationalRateSet, pMgntInfo->Regdot11OperationalRateSet.Length);
}

// Allocate a fixed size buffer for general purpose using.
BOOLEAN 
MgntAllocGenTempBuffer(
	IN	PADAPTER		pAdapter
	)
{
	RT_STATUS				Status;
	PRT_GEN_TEMP_BUFFER		pGenTmpBuf;
	u4Byte					i;

	pAdapter->GenTempBufferArray.Length = GEN_TEMP_BUFFER_NUM * sizeof(RT_GEN_TEMP_BUFFER);
	Status = PlatformAllocateMemory(pAdapter, &pAdapter->GenTempBufferArray.Ptr, pAdapter->GenTempBufferArray.Length);
	if(Status != RT_STATUS_SUCCESS)
		return FALSE;
	
	PlatformZeroMemory(pAdapter->GenTempBufferArray.Ptr, pAdapter->GenTempBufferArray.Length);
	RTInitializeListHead(&pAdapter->GenTempBufferQueue);
	
	pGenTmpBuf = (PRT_GEN_TEMP_BUFFER)pAdapter->GenTempBufferArray.Ptr;
	for(i=0;i<GEN_TEMP_BUFFER_NUM;i++)
	{
		Status = PlatformAllocateMemory(pAdapter, &pGenTmpBuf[i].Buffer.Ptr, GEN_TEMP_BUFFER_SIZE);
		if(Status != RT_STATUS_SUCCESS)
			break;
		pGenTmpBuf[i].Buffer.Length = GEN_TEMP_BUFFER_SIZE;
		RTInsertTailListWithCnt(&pAdapter->GenTempBufferQueue, &pGenTmpBuf[i].List, &pAdapter->NumGenTempBufferIdle);			
	}

	if (Status != RT_STATUS_SUCCESS)
		return FALSE;

	return TRUE;
}

BOOLEAN 
MgntFreeGenTempBuffer(
	IN	PADAPTER		pAdapter
	)
{
	u4Byte 					nFreed=0;
	PRT_GEN_TEMP_BUFFER		pGenTmpBuf;
	BOOLEAN					bReturn=TRUE;

	//
	// 2012/02/09 MH According to win7 driver verifier test, the setting for below items
	// A). Force pending I/O requests B). Low resource simulation C). IRP logging
	// We may init fail in allocating MGNT memory. At this time, we can not use Adpter
	// temp buffer quere to do any operation.
	//
	if(pAdapter->GenTempBufferArray.Ptr != NULL)
	{
		if(!RTIsListEmpty(&pAdapter->GenTempBufferQueue))
		{
			while(!RTIsListEmpty(&pAdapter->GenTempBufferQueue))
			{
				pGenTmpBuf = (PRT_GEN_TEMP_BUFFER)RTRemoveHeadListWithCnt(&pAdapter->GenTempBufferQueue, &pAdapter->NumGenTempBufferIdle);
		
				PlatformFreeMemory(pGenTmpBuf->Buffer.Ptr, GEN_TEMP_BUFFER_SIZE);
				nFreed++;
			}
			RT_ASSERT((nFreed==GEN_TEMP_BUFFER_NUM),("Freed General buffer(%d) less than allocated(%d)!!\n", nFreed, GEN_TEMP_BUFFER_NUM));
			bReturn = FALSE;
		}
		
		
		{
			PlatformFreeMemory(pAdapter->GenTempBufferArray.Ptr, pAdapter->GenTempBufferArray.Length);
		}
	}

	return bReturn;
}

PRT_GEN_TEMP_BUFFER
GetGenTempBuffer(
	IN PADAPTER	Adapter,
	IN	u4Byte	Length
	)
{
	PRT_GEN_TEMP_BUFFER pBuffer=NULL;
	RT_STATUS			Status;

	PlatformAcquireSpinLock(Adapter, RT_GEN_TEMP_BUF_SPINLOCK);
	if(RTIsListEmpty(&Adapter->GenTempBufferQueue) || (Length > GEN_TEMP_BUFFER_SIZE))
	{
		PlatformReleaseSpinLock(Adapter, RT_GEN_TEMP_BUF_SPINLOCK);
		/* No free General Buffer, try to allocate one dynamically now */
		Status = PlatformAllocateMemory(Adapter, (PVOID*)&pBuffer, sizeof(RT_GEN_TEMP_BUFFER)+Length);
		if(Status != RT_STATUS_SUCCESS)
		{
			RT_ASSERT(FALSE,("Get a General Buffer and Dynamical Allocate Failed!!\n"));
			return NULL;
		}
		else
		{
			pBuffer->isDynaAlloc = TRUE;
			pBuffer->Buffer.Ptr = (u1Byte *)pBuffer + sizeof(RT_GEN_TEMP_BUFFER);			
			pBuffer->Buffer.Length = Length;
			PlatformZeroMemory((u1Byte *)pBuffer->Buffer.Ptr, Length);			
		}
	}
	else
	{
		pBuffer=(PRT_GEN_TEMP_BUFFER)RTRemoveHeadListWithCnt(&Adapter->GenTempBufferQueue, &Adapter->NumGenTempBufferIdle);
		PlatformZeroMemory((u1Byte *)pBuffer->Buffer.Ptr, Length);
		pBuffer->isDynaAlloc = FALSE;
		PlatformReleaseSpinLock(Adapter, RT_GEN_TEMP_BUF_SPINLOCK);
	}

	return pBuffer;
}

VOID
ReturnGenTempBuffer(
	IN PADAPTER				Adapter,
	IN PRT_GEN_TEMP_BUFFER	pGenBuffer
	)
{
	if (pGenBuffer->isDynaAlloc)
	{
		PlatformFreeMemory((u1Byte *)pGenBuffer, sizeof(RT_GEN_TEMP_BUFFER)+pGenBuffer->Buffer.Length);
	}
	else
	{
		PlatformAcquireSpinLock(Adapter, RT_GEN_TEMP_BUF_SPINLOCK);
		RTInsertTailListWithCnt(&Adapter->GenTempBufferQueue, &pGenBuffer->List, &Adapter->NumGenTempBufferIdle);
		PlatformReleaseSpinLock(Adapter, RT_GEN_TEMP_BUF_SPINLOCK);
	}
}


//
//	Description:
//		Allocate memory of COMMON\. 
//
//	Usage:
//		Memory Allocation:
//			step 1. Add DECLARE_RT_OBJECT(OBJECT_TYPE) into the the type 
//			(OBJECT_TYPE) to allocated.
//			step 2. Use MGNT_ALLOC_RT_OBJECT(__ppObject, OBJECT_TYPE) to allocate memory 
//			for the type (OBJECT_TYPE) to specified address (__ppObject). 
//
//		Memory Release:
//			The object allocated in MgntAllocateMemory() will be release at 
//			MgntFreeMemory() automatically.
//
//	TODO: 
//		Allocate MGNT_INFO.
//
#define MGNT_ALLOC_RT_OBJECT(__pAdapter, __ObjectList, __ppObject, __OBJECT_TYPE) \
	ppObject = (__ppObject); \
	ALLOC_RT_OBJECT(__pAdapter, __ObjectList, __ppObject, __OBJECT_TYPE); \
	if(*ppObject == NULL) goto Error;

RT_STATUS
MgntAllocMemory(
	IN PADAPTER		pAdapter
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_LIST_ENTRY	pObjectList = &(pMgntInfo->ObjectList);
	RT_STATUS		status = RT_STATUS_FAILURE;
	PVOID*			ppObject;

	INIT_RT_OBJECT_LIST(pObjectList);
	do
	{
		MGNT_ALLOC_RT_OBJECT(pAdapter, pObjectList, (void**)&(pMgntInfo->pStaQos), STA_QOS);		
		MGNT_ALLOC_RT_OBJECT(pAdapter, pObjectList, (void**)&(pMgntInfo->SecurityInfo.pCkipPara),CKIP_PARAMETER);
#if (WPS_SUPPORT == 1)	
		MGNT_ALLOC_RT_OBJECT(pAdapter, pObjectList, &(pMgntInfo->pSimpleConfig), SIMPLE_CONFIG_T);
#endif
		MGNT_ALLOC_RT_OBJECT(pAdapter, pObjectList, &(pMgntInfo->pDot11dInfo), RT_DOT11D_INFO);
		MGNT_ALLOC_RT_OBJECT(pAdapter, pObjectList, (void **)&(pMgntInfo->pHTInfo), RT_HIGH_THROUGHPUT);
		MGNT_ALLOC_RT_OBJECT(pAdapter, pObjectList, (void **)&(pMgntInfo->pVHTInfo), RT_VERY_HIGH_THROUGHPUT);
		MGNT_ALLOC_RT_OBJECT(pAdapter, pObjectList, (void **)&(pMgntInfo->pChannelInfo), RT_CHANNEL_INFO);

		MGNT_ALLOC_RT_OBJECT(pAdapter, pObjectList, &(pMgntInfo->pChannelList), RT_CHANNEL_LIST);

		MGNT_ALLOC_RT_OBJECT(pAdapter, pObjectList, &(pMgntInfo->pApModeInfo), RT_AP_INFO);

		if(pAdapter == GetDefaultAdapter(pAdapter) && !CustomScan_AllocInfo(pAdapter))
			break;

		if( !AllocDrvLogMemory(pAdapter) ) 
			break;

		if( !MgntAllocHashTables(pAdapter) )
			break;

		if( !MgntAllocatePacketParser(pAdapter) )
			break;

		if( !MgntAllocGenTempBuffer(pAdapter) )
			break;

		if(RT_STATUS_SUCCESS != (status = P2P_AllocP2PInfo(pAdapter)))
			break;

		if(RT_STATUS_SUCCESS != (status = NAN_Allocate(pAdapter)))
			break;

		if(RT_STATUS_SUCCESS != (status = WFD_AllocateWfdInfo(pAdapter)))
			break;

		if(RT_STATUS_SUCCESS != (status = TDLS_AllocateMemory(pAdapter)))
			break;

		status = RT_STATUS_SUCCESS;
	}while(FALSE);

Error:

	if(status != RT_STATUS_SUCCESS)
	{
		MgntFreeMemory(pAdapter);
	}

	return status;
}


//
//	Description:
//		Free memory of COMMON\. 
//
//	TODO: 
//		Allocate MGNT_INFO.
//
VOID
MgntFreeMemory(
	IN PADAPTER		pAdapter
	)
{
	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	PRT_LIST_ENTRY	pObjectList = &(pMgntInfo->ObjectList);
	PRT_WLAN_STA	pEntry = NULL;
	u4Byte			i = 0;
	
	RT_ASSERT((pObjectList->Flink != NULL && pObjectList->Blink != NULL), 
		("MgntFreeMemory(): Flink(%p) Blink(%p)\n", pObjectList->Flink, pObjectList->Blink));
	
	WFD_FreeWfdInfo(pAdapter);

	P2PFreeAllocatedMemory(pAdapter);

	P2P_FreeP2PInfo(pAdapter);
	
	NAN_Free(pAdapter); 

	TDLS_FreeMemory(pAdapter);

	MgntFreePacketParser(pAdapter);

	MgntFreeHashTables(pAdapter);

	MgntFreeGenTempBuffer (pAdapter);

	if(pMgntInfo->AdditionalBeaconIESize > 0)
	{
		PlatformFreeMemory(pMgntInfo->AdditionalBeaconIEData, pMgntInfo->AdditionalBeaconIESize);
		pMgntInfo->AdditionalBeaconIESize = 0;
	}
	
	if(pMgntInfo->AdditionalResponseIESize > 0)
	{
		PlatformFreeMemory(pMgntInfo->AdditionalResponseIEData, pMgntInfo->AdditionalResponseIESize);
		pMgntInfo->AdditionalResponseIESize = 0;
	}

	if(pMgntInfo->AdditionalAssocReqIESize > 0)
	{
		PlatformFreeMemory(pMgntInfo->AdditionalAssocReqIEData, pMgntInfo->AdditionalAssocReqIESize);
		pMgntInfo->AdditionalAssocReqIESize= 0;
	}
	
	if (pMgntInfo->AdditionalProbeReqIESize > 0)
	{
		PlatformFreeMemory(pMgntInfo->AdditionalProbeReqIEData, pMgntInfo->AdditionalProbeReqIESize);
		pMgntInfo->AdditionalProbeReqIESize = 0;
	}
	
	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		pEntry = &(pMgntInfo->AsocEntry[i]);
		if(pEntry)
		{
			if(pEntry->AP_RecvAsocReqLength > 0)
				PlatformFreeMemory(pEntry->AP_RecvAsocReq, 
					pEntry->AP_RecvAsocReqLength);
			
			if(pEntry->AP_SendAsocRespLength > 0)
				PlatformFreeMemory(pEntry->AP_SendAsocResp, 
					pEntry->AP_SendAsocRespLength);
		}
	}

	// Release all allocated memory in the P2P_INFO structure ----
	//P2PFreeAllocatedMemory(pP2PInfo);
	//---------------------------------------------------

	while( HAS_RT_OBJECT(pObjectList) )
	{
		FREE_RT_OBJECT(pObjectList);
	}

	FreeDrvLogMemory(pAdapter);
}


VOID
MgntInitializeAllTimer(
	PADAPTER			Adapter
)
{
	PMGNT_INFO					pMgntInfo=&Adapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);	
	PSTA_QOS					pStaQos = pMgntInfo->pStaQos;
	PRT_CHANNEL_INFO			pChnlInfo = pMgntInfo->pChannelInfo;

#if (MULTICHANNEL_SUPPORT == 1)
{
	MultiChannelInitializeTimer(Adapter);
}
#endif

	PlatformInitializeTimer(Adapter, &pMgntInfo->ScanTimer, (RT_TIMER_CALL_BACK)ScanCallback, NULL, "ScanTimer");
	PlatformInitializeTimer(Adapter, &pMgntInfo->JoinTimer, (RT_TIMER_CALL_BACK)JoinTimeout, NULL, "JoinTimer");
	PlatformInitializeTimer(Adapter, &pMgntInfo->JoinConfirmTimer, (RT_TIMER_CALL_BACK)JoinConfirm, NULL, "JoinConfirm");
	PlatformInitializeTimer( Adapter, &pMgntInfo->JoinProbeReqTimer, (RT_TIMER_CALL_BACK)JoinProbeReq, NULL, "JoinProbeReq");
	PlatformInitializeTimer(Adapter, &pMgntInfo->AuthTimer, (RT_TIMER_CALL_BACK) AuthTimeout, NULL, "AuthTimer");
	PlatformInitializeTimer(Adapter, &pMgntInfo->AsocTimer, (RT_TIMER_CALL_BACK) AsocTimeout, NULL, "AsocTimer");
	PlatformInitializeTimer(Adapter, &pMgntInfo->SwBeaconTimer, (RT_TIMER_CALL_BACK)SwBeaconCallback, NULL, "SwBeaconTimer"); //use HW beacon Isaiah 
	PlatformInitializeTimer(Adapter, &pMgntInfo->globalKeyInfo.KeyMgntTimer, (RT_TIMER_CALL_BACK)KeyMgntTimeout, NULL, "globalKeyInfo.KeyMgntTimer");	// Added by Annie, 2005-06-29.
	PlatformInitializeTimer(Adapter, &pMgntInfo->AwakeTimer, (RT_TIMER_CALL_BACK)AwakeTimerCallback, NULL, "AwakeTimer");
	PlatformInitializeTimer(Adapter, &pPSC->InactivePsTimer, (RT_TIMER_CALL_BACK)InactivePsTimerCallback, NULL, "InactivePsTimer");
	PlatformInitializeTimer( Adapter, &pMgntInfo->PnpWakeUpJoinTimer, (RT_TIMER_CALL_BACK)PnPWakeUpJoinTimerCallback, NULL, "PnPWakeUpJoinTimerCallback");		
	PlatformInitializeTimer(Adapter, &pMgntInfo->LedTimer, (RT_TIMER_CALL_BACK)LedTimerCallback, NULL, "LedTimer");	
	PlatformInitializeTimer(Adapter, &(pStaQos->ACMTimer), (RT_TIMER_CALL_BACK)QosACMTimerCallback, NULL, "ACMTimer");
	PlatformInitializeTimer(Adapter, &(pStaQos->AddTsTimer), (RT_TIMER_CALL_BACK)QosAddTsTimerCallback, NULL, "AddTsTimer");	
	PlatformInitializeTimer(Adapter, &(pMgntInfo->DelaySendBeaconTimer), (RT_TIMER_CALL_BACK)DelaySendBeaconTimerCallback, NULL, "DelayStartTimer");
	PlatformInitializeTimer(Adapter, &(pMgntInfo->SecurityInfo.SAQueryTimer), (RT_TIMER_CALL_BACK)SAQueryTimerCallback, NULL, "SAQueryTimerCallback");

	DFS_TimerContrl(Adapter, DFS_TIMER_INIT);

#if (P2P_SUPPORT == 1)
{
	P2PInitializeTimer(Adapter);
}
#endif

	NAN_InitTimer(Adapter);


	PlatformInitializeTimer( Adapter, &pMgntInfo->WaitingKeyTimer, (RT_TIMER_CALL_BACK)WaitingKeyTimerCallback, NULL, "WaitingKeyTimerCallback");	

	//Move from HTInitializeHTInfo for avoiding defining duplicately. by wl 2012-01-13	
	PlatformInitializeTimer( Adapter, &(pChnlInfo->SwBwTimer), (RT_TIMER_CALL_BACK)CHNL_SetBwChnlCallback, NULL, "SwVHTBwTimer");

}


VOID
ResetPSCParameters(PADAPTER	Adapter)
{
	PMGNT_INFO				pMgntInfo=&Adapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	//pPSC->bSwRfProcessing = FALSE ; 
	pPSC->ReturnPoint = IPS_CALLBACK_NONE;
}

VOID
InitializeWatchDogTimer(
	PADAPTER	Adapter
)
{
	PlatformInitializeTimer(Adapter, &Adapter->MgntInfo.WatchDogTimer, (RT_TIMER_CALL_BACK)WatchDogTimerCallback, NULL, "WatchDogTimer");
}

VOID
CancelWatchDogTimer(
	PADAPTER	Adapter
)
{
	PlatformCancelTimer(Adapter, &Adapter->MgntInfo.WatchDogTimer);
}

VOID
ReleaseWatchDogTimer(
	PADAPTER	Adapter
)
{
	PlatformReleaseTimer(Adapter, &Adapter->MgntInfo.WatchDogTimer);
}

	
VOID
MgntCancelAllTimer(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO					pMgntInfo=&Adapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);	
	PSTA_QOS					pStaQos = pMgntInfo->pStaQos;
	PRT_CHANNEL_INFO			pChannelInfo = pMgntInfo->pChannelInfo;

	FunctionIn(COMP_MLME);
	
	if(MgntScanInProgress(pMgntInfo))
		MgntResetScanProcess(Adapter);

	PlatformCancelTimer(Adapter, &pMgntInfo->ScanTimer);
	{
		PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);
	
		while(pLoopAdapter)
		{
			pLoopAdapter->MgntInfo.bScanInProgress = FALSE;
			pLoopAdapter->MgntInfo.bDualModeScanStep = 0;
			
			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}

	PlatformCancelTimer(Adapter, &pMgntInfo->JoinTimer );
	PlatformCancelTimer(Adapter, &pMgntInfo->JoinConfirmTimer );
	PlatformCancelTimer(Adapter, &pMgntInfo->JoinProbeReqTimer );
	PlatformCancelTimer(Adapter, &pMgntInfo->AuthTimer);
	PlatformCancelTimer(Adapter, &pMgntInfo->AsocTimer);
	PlatformCancelTimer(Adapter, &pMgntInfo->SwBeaconTimer);
	PlatformCancelTimer(Adapter, &pMgntInfo->globalKeyInfo.KeyMgntTimer);	// Added by Annie, 2005-06-29.
	PlatformCancelTimer(Adapter, &pMgntInfo->AwakeTimer);	
	PlatformCancelTimer(Adapter, &pPSC->InactivePsTimer);	
	pPSC->ReturnPoint = IPS_CALLBACK_NONE;
	PlatformCancelTimer(Adapter, &pMgntInfo->PnpWakeUpJoinTimer);
	PlatformCancelTimer(Adapter, &pMgntInfo->LedTimer);	
	PlatformCancelTimer(Adapter, &(pStaQos->ACMTimer));
	PlatformCancelTimer(Adapter, &(pStaQos->AddTsTimer));
	PlatformCancelTimer(Adapter, &(pMgntInfo->DelaySendBeaconTimer));

	PlatformCancelTimer(Adapter, &(pMgntInfo->SecurityInfo.SAQueryTimer));

	DFS_TimerContrl(Adapter, DFS_TIMER_CANCEL);

#if (P2P_SUPPORT == 1)	
{
	P2PCancelTimer(Adapter);
}
#endif

	NAN_CancelTimer(Adapter);

	PlatformCancelTimer(Adapter, &pMgntInfo->WaitingKeyTimer);
	PlatformCancelTimer( Adapter, &(pChannelInfo->SwBwTimer));

	FunctionOut(COMP_MLME);
}

VOID
MgntReleaseAllTimer(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO					pMgntInfo =&Adapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
#if (P2P_SUPPORT == 1)
	PP2P_INFO					pP2PInfo = (PP2P_INFO)pMgntInfo->pP2PInfo;
#endif
	PSTA_QOS					pStaQos = pMgntInfo->pStaQos;
	PRT_CHANNEL_INFO			pChannelInfo = pMgntInfo->pChannelInfo;

#if (MULTICHANNEL_SUPPORT == 1)
{
	MultiChannelReleaseTimer(Adapter);
}
#endif

	PlatformReleaseTimer(Adapter, &pMgntInfo->ScanTimer);

	PlatformReleaseTimer(Adapter, &pMgntInfo->JoinTimer );
	PlatformReleaseTimer(Adapter, &pMgntInfo->JoinConfirmTimer );
	PlatformReleaseTimer(Adapter, &pMgntInfo->JoinProbeReqTimer );
	PlatformReleaseTimer(Adapter, &pMgntInfo->AuthTimer);
	PlatformReleaseTimer(Adapter, &pMgntInfo->AsocTimer);
	PlatformReleaseTimer(Adapter, &pMgntInfo->SwBeaconTimer);
	PlatformReleaseTimer(Adapter, &pMgntInfo->globalKeyInfo.KeyMgntTimer);
	PlatformReleaseTimer(Adapter, &pMgntInfo->AwakeTimer);
	PlatformReleaseTimer(Adapter, &pPSC->InactivePsTimer);
	PlatformReleaseTimer(Adapter, &pMgntInfo->PnpWakeUpJoinTimer);	
	PlatformReleaseTimer(Adapter, &pMgntInfo->DelaySendBeaconTimer);
	PlatformReleaseTimer(Adapter, &pMgntInfo->SecurityInfo.SAQueryTimer);
	PlatformReleaseTimer(Adapter, &pMgntInfo->LedTimer);
	PlatformReleaseTimer(Adapter, &(pStaQos->ACMTimer));
	PlatformReleaseTimer(Adapter, &(pStaQos->AddTsTimer));

	DFS_TimerContrl(Adapter, DFS_TIMER_RELEASE);
	
#if (P2P_SUPPORT == 1)
{
	P2PReleaseTimer(Adapter);
}
#endif

	NAN_ReleaseTimer(Adapter);


	PlatformReleaseTimer(Adapter, &pMgntInfo->WaitingKeyTimer);
	PlatformReleaseTimer( Adapter, &(pChannelInfo->SwBwTimer));
}

//
//	Description:
//		Initialize workitems at driver initialization. 
//
VOID
MgntInitializeAllWorkItem(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	
	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->IbssStartRequestWorkItem), 
		IbssStartRequestCallback, 
		(PVOID)Adapter,
		"IbssStartRequestWorkItem");

	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->ApStartRequestWorkItem), 
		AP_StartApRequest, 
		(PVOID)Adapter,
		"ApStartRequestWorkItem");

	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->DropPacketWorkItem), 
		DropPacketWorkitemCallback, 
		(PVOID)Adapter,
		"DropPacketWorkItem");


	PlatformInitializeWorkItem(
		Adapter, 
		&(pMgntInfo->ApSendDisassocWithOldChnlWorkitem),
		Ap_SendDisassocWithOldChnlWorkitemCallback,
		(PVOID)Adapter, 
		"Ap_SendDisassocWithOldChnlWorkitem");

	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->DozeWorkItem), 
		DozeWorkItemCallback, 
		(PVOID)Adapter,
		"DozeWorkItem");

	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->AwakeWorkItem), 
		AwakeWorkItemCallback, 
		(PVOID)Adapter,
		"AwakeWorkItem");

	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->TbttPollingWorkItem), 
		TbttPollingWorkItemCallback, 
		(PVOID)Adapter,
		"TbttPollingWorkItem");

	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->PowerSaveControl.InactivePsWorkItem),
		InactivePsWorkItemCallback,
		(PVOID)Adapter,
		"InactivePsWorkItem");

	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->UpdateTxPowerWorkItem), 
		UpdateTxPowerDbmWorkItemCallback, 
		(PVOID)Adapter,
		"UpdateTxPowerWorkItem");

	// 2008/05/16 MH Add to do IC verification on debug command mode.
#if DBG_CMD
	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->NICVerify),
		(RT_WORKITEM_CALL_BACK)DBG_Verify_Console,
		(PVOID)Adapter,
		"NIC Verify Console");	
#endif


	DFS_WorkItemContrl(Adapter, DFS_WORKITEM_INIT);
	
#if (P2P_SUPPORT == 1)	
{
	PP2P_INFO		pP2PInfo = (PP2P_INFO)pMgntInfo->pP2PInfo;
	
	PlatformInitializeWorkItem(
		Adapter,
		&(pP2PInfo->P2PPSWorkItem), 
		(RT_WORKITEM_CALL_BACK)P2PPsWorkItemCallback,
		(PVOID)pP2PInfo,
		"P2PPsTimeoutWorkItem");
}
#endif

#if (MULTICHANNEL_SUPPORT == 1)
{
	MultiChannelInitializeWorkItem(Adapter);
}
#endif

#if(AUTO_CHNL_SEL_NHM == 1)
	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->AutoChnlSel.AutoChnlSelWorkitem), 
		MgntAutoChnlSelWorkitemCallback, 
		(PVOID)Adapter,
		"MgntAutoChnlSelWorkitemCallback");
#endif

	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->PowerSaveControl.FwPsClockOnWorkitem), 
		FwPsClockOnWorkitemCallback, 
		(PVOID)Adapter,
		"FwPsClockOnWorkitemCallback");

	PlatformInitializeWorkItem(
		Adapter,
		&(pMgntInfo->ChangeBwChnlFromPeerWorkitem), 
		CHNL_ChangeBwChnlFromPeerWorkitemCallBack, 
		(PVOID)Adapter,
		"CHNL_ChangeBwChnlFromPeerWorkitemCallBack");	

}


//
//	Description:
//		Free workitems at driver unload. 
//
VOID
MgntFreeAllWorkItem(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);

	PlatformFreeWorkItem(&(pMgntInfo->IbssStartRequestWorkItem));
	PlatformFreeWorkItem(&(pMgntInfo->ApStartRequestWorkItem));
	PlatformFreeWorkItem(&(pMgntInfo->DropPacketWorkItem));
	PlatformFreeWorkItem(&(pMgntInfo->ApSendDisassocWithOldChnlWorkitem));
	PlatformFreeWorkItem(&(pMgntInfo->DozeWorkItem));
	PlatformFreeWorkItem(&(pMgntInfo->AwakeWorkItem));
	PlatformFreeWorkItem(&(pMgntInfo->TbttPollingWorkItem));
	PlatformFreeWorkItem(&(pMgntInfo->PowerSaveControl.InactivePsWorkItem));
	PlatformFreeWorkItem(&(pMgntInfo->UpdateTxPowerWorkItem));
	PlatformFreeWorkItem(&(pMgntInfo->PowerSaveControl.FwPsClockOnWorkitem));
	PlatformFreeWorkItem(&(pMgntInfo->ChangeBwChnlFromPeerWorkitem));

#if (AUTO_CHNL_SEL_NHM == 1)
	PlatformFreeWorkItem(&(pMgntInfo->AutoChnlSel.AutoChnlSelWorkitem));
#endif
	
#if DBG_CMD	
	PlatformFreeWorkItem(&(pMgntInfo->NICVerify));
#endif	
	DFS_WorkItemContrl(Adapter, DFS_WORKITEM_FREE);

	NAN_FreeAllWorkItem(Adapter);
	
#if (P2P_SUPPORT == 1)
{
	PP2P_INFO		pP2PInfo = (PP2P_INFO)pMgntInfo->pP2PInfo;
	PlatformFreeWorkItem(&(pP2PInfo->P2PPSWorkItem));
//	PlatformFreeWorkItem(&(pP2PInfo->P2POidPostProcessWorkItem));
}
#endif

#if (MULTICHANNEL_SUPPORT == 1)
{
	MultiChannelReleaseWorkItem(Adapter);
}
#endif

}

//
//	Description:
//		Reset current 802.11 management related setting.
//
VOID
ResetMgntVariables(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO	pMgntInfo =&(Adapter->MgntInfo);
	u1Byte		i;
	int		nIdx;

	FunctionIn(COMP_MLME);

	PlatformCancelTimer(Adapter, &pMgntInfo->JoinTimer );
	PlatformCancelTimer(Adapter, &pMgntInfo->JoinConfirmTimer );
	PlatformCancelTimer(Adapter, &pMgntInfo->JoinProbeReqTimer );
	PlatformCancelTimer(Adapter, &pMgntInfo->AuthTimer);
	PlatformCancelTimer(Adapter, &pMgntInfo->AsocTimer);

	pMgntInfo->OpMode = RT_OP_MODE_NO_LINK;
	pMgntInfo->AuthStatus = AUTH_STATUS_IDLE;
	pMgntInfo->mDisable = TRUE;
	pMgntInfo->mAssoc = FALSE;
	pMgntInfo->mIbss = FALSE;
	pMgntInfo->ListenInterval = 2; // The same as WG111T, 2005.04.08, by rcnjko.
	pMgntInfo->AsocTimestamp = 0; // Timestamp when associated, 2006.11.23, by shien chang.
	pMgntInfo->Regdot11networktype = RT_JOIN_NETWORKTYPE_INFRA; //set by user. select join network type
	pMgntInfo->bInBIPMFPMode = FALSE;
	CLEAR_FLAGS(pMgntInfo->targetAKMSuite);
	pMgntInfo->RequestFromUplayer = FALSE;
	pMgntInfo->bDisconnectRequest = FALSE;
	pMgntInfo->bPrepareRoaming = FALSE;
	pMgntInfo->PrepareRoamState = RT_PREPARE_ROAM_NONE;
	pMgntInfo->PrepareRoamingCount = 0;
	pMgntInfo->bStartDelayActionFrame = FALSE;
	pMgntInfo->CntAfterLink = 0;

#if 1
	if(GetFirstClientPort(Adapter) || GetFirstGOPort(Adapter))
	{
		pMgntInfo->bScanOnly = TRUE;
		//Set SSID as dummy to stop the current join process.
		SET_SSID_DUMMY(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);
	}
#endif

	//
	// We never stop the scan process, but the state_Synchronization_Sta should be "STATE_No_Bss" after scan completes.
	//
	if(!pMgntInfo->bScanInProgress)
		pMgntInfo->state_Synchronization_Sta = STATE_No_Bss;
	else
		pMgntInfo->state_Synchronization_Sta_BeforeScan = STATE_No_Bss;

	pMgntInfo->bJoinInProgress = FALSE;
	
	pMgntInfo->bMlmeStartReqRsn = MLMESTARTREQ_NONE;
	MgntResetRoamingState(pMgntInfo);
	MgntResetJoinCounter(pMgntInfo);

	ReleaseDataFrameQueued(Adapter);

	MgntLinkStatusResetRxBeacon(Adapter);

	for( i=0; i<RT_MAX_LD_SLOT_NUM; i++ ){
		pMgntInfo->LinkDetectInfo.RxBcnNum[i] = 0;
	}
	pMgntInfo->LinkDetectInfo.SlotNum = 2;
	pMgntInfo->LinkDetectInfo.SlotIndex = 0;
	
	MgntRefreshSuppRateSet( Adapter );

	// Intialize IBSS related setting. 2004.12.10, by rcnjko.
	pMgntInfo->dot11BeaconPeriod = pMgntInfo->Regdot11BeaconPeriod; // For 8187 WIFI, 2004.10.11, by rcnjko.
	pMgntInfo->dot11DtimPeriod = pMgntInfo->Regdot11DtimPeriod;

	// Initialize wireless mode of current BSS. 2005.02.17, by rcnjko.
	pMgntInfo->CurrentBssWirelessMode = WIRELESS_MODE_UNKNOWN;

	// Initialize SrcAddr, FragNum and LastRxUniSrcAddr of last received unicast packet. Annie, 2005-07-25.
	for( nIdx=0; nIdx<6; nIdx++ )
		pMgntInfo->LastRxUniSrcAddr[nIdx] = 0xff;

	pMgntInfo->LastRxUniFragNum = 0xff;
	pMgntInfo->LastRxUniSeqNum = 0xffff;
			

	// Reset QoS related variables. Added by Annie, 2005-11-07.
	QosInitializeSTA( Adapter );

	// Reset HT related variables.

	//[win7 Two Port BW40Mhz issue] Extention Port obey the rule of Default Port 
	//2009.05.20, by Bohn
	HTInitializeHTInfo( Adapter );


	//VHT
	VHTInitializeVHTInfo(Adapter);
	

	// Reset TS related variables.
	TSInitialize( Adapter );

	pMgntInfo->ExcludedMacAddrListLength = 0;
	pMgntInfo->bExcludeUnencrypted = TRUE;
	pMgntInfo->SafeModeEnabled = FALSE;

	//
	// Reset all association entry for AP mode and AdHoc mode.
	//
	AsocEntry_ResetAll(Adapter);

	TDLS_Reset(Adapter);

	// Get default hidden ssid setting. This value will affect the DTM test result, ex PacketFilter_ext.
	// By Bruce, 2008-09-26.
	pMgntInfo->bHiddenSSIDEnable = DEFAULT_HIDDEN_SSID;

	if(pMgntInfo->AdditionalBeaconIESize > 0)
	{
		PlatformFreeMemory(pMgntInfo->AdditionalBeaconIEData, pMgntInfo->AdditionalBeaconIESize);
		pMgntInfo->AdditionalBeaconIESize = 0;
	}
	
	if(pMgntInfo->AdditionalResponseIESize > 0)
	{
		PlatformFreeMemory(pMgntInfo->AdditionalResponseIEData, pMgntInfo->AdditionalResponseIESize);
		pMgntInfo->AdditionalResponseIESize = 0;
	}
	
	if(pMgntInfo->AdditionalAssocReqIESize > 0)
	{
		PlatformFreeMemory(pMgntInfo->AdditionalAssocReqIEData, pMgntInfo->AdditionalAssocReqIESize);
		pMgntInfo->AdditionalAssocReqIESize= 0;
	}

	//
	// Initialize SSID list to scan.
	// 
	MgntInitSsidsToScan(Adapter);

	pMgntInfo->bIndicateConnectEvent = FALSE;

	pMgntInfo->bInToSleep = FALSE;
//	pMgntInfo->bJoinInProgress = FALSE;

	if(Adapter->bSWInitReady)
	{
		ActUpdate_ProtectionMode(Adapter, FALSE);
	}

	DFS_StaMgntResetVars(Adapter);

	// Initialize TIE and 802.11 MFP 
	pMgntInfo->TIE.Octet 		 = pMgntInfo->TIEBuff;
	pMgntInfo->TIE.Length 	 = 0;
	pMgntInfo->bInBIPMFPMode  = FALSE;

	FunctionOut(COMP_MLME);

}

//
//	Description:
//		1. Initialize management related S/W resource.
//		2. Initialize default value of management related attributes, RegXXX, and	
//		reset 802.11 management related variables to initial state.
//
//	Assumption:
//		This function shall be invoke once at driver initialization.
//
VOID
InitializeMgntVariables(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;
	int			nIdx;

	Adapter->bFWReady = FALSE;
	// 2008/02/18 MH We execute IO for 9x series after LBUS is enabled.
	pMgntInfo->LPSDelayCnt= 0;
	Adapter->bDriverIsGoingToUnload = FALSE;
	// 09/08/17 MH From Clevo 830/840 test,	we need to forbid DM when we try to enter S3/S4.
	// The function os not enabled now!!!!!
	pMgntInfo->bPwrSaveState = FALSE;
	Adapter->bDriverIsGoingToPnpSetPowerSleep = FALSE;	
	Adapter->bHWSecurityInWoL = FALSE;
	Adapter->bEnterPnpSleep = FALSE;
	Adapter->bWakeFromPnpSleep = FALSE;
	Adapter->bDriverShutdown = FALSE;
	Adapter->bInWoWLANPowerState = FALSE;
	Adapter->CPWMTimeoutCount = 0;
	
	pMgntInfo->DelayLPSLastTimeStamp = 0;
	pMgntInfo->bReceiveSystemPSOID = FALSE;
	
	// Initialize timer and workitems.
	MgntInitializeAllTimer(Adapter);
	MgntInitializeAllWorkItem(Adapter);

	// Init watchdog timer
	InitializeWatchDogTimer(Adapter);
	
	//
	// Initialize default setting, which might be overrided as user desired ones.
	// For example, registry in Windows case.. 
	//

	FillOctetString( pMgntInfo->Ssid, pMgntInfo->SsidBuf, 0 );

#if 1	
	if(GetFirstClientPort(Adapter) || GetFirstGOPort(Adapter))
	{
		//Set SSID as dummy to stop the current join process.
		SET_SSID_DUMMY(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);
	}
#endif
	
	MgntInitRateSet(Adapter);

	pMgntInfo->FragThreshold=2347;
	pMgntInfo->dot11RtsThreshold = 2346;

	//
	// Max Raoming Count(Roaming Time = Roaming Count * Check_For_Hange_Period), by Bruce, 2009-02-13.
	//
	pMgntInfo->RegRoamingLimitCount = 2;

	pMgntInfo->Regdot11ChannelNumber = 1;
	pMgntInfo->Regdot11BeaconPeriod = 100; // Asked by Owen, 2004.10.11, by rcnjko.
	pMgntInfo->Regdot11DtimPeriod = 2;

	pMgntInfo->Regdot11networktype = RT_JOIN_NETWORKTYPE_INFRA; //set by user. select join network type
	pMgntInfo->RegmCap = cESS | cIBSS /*| cPrivacy | cShortPreamble*/;

	// Initialize AP mode related variables. 2005.06.21, by rcnjko.
	RTInitializeListHead( &(pMgntInfo->GroupPsQueue) );
	for(nIdx = 0; nIdx < ASSOCIATE_ENTRY_NUM; nIdx++)
	{
		RTInitializeListHead( &(pMgntInfo->AsocEntry[nIdx].PsQueue) );
		RTInitializeListHead( &(pMgntInfo->AsocEntry[nIdx].WmmPsQueue) );
	}

	// Initialzie Auto Select Channel related, 2006.1.26, by rcnjko.
	pMgntInfo->bAutoSelChnl = FALSE;
	pMgntInfo->ChnlWeightMode = 0;

	// Initialize AP mode hidden SSID and Locked STA Address List. Added by Annie, 2006-02-15.
	pMgntInfo->bHiddenSSID = FALSE;
	pMgntInfo->LockType = MAC_FILTER_DISABLE;
	pMgntInfo->LockedSTACount = 0;
	pMgntInfo->LockedSTACount_Reject= 0;
	PlatformZeroMemory( pMgntInfo->LockedSTAList, MAX_LOCKED_STA_LIST_NUM*6 );
	PlatformZeroMemory( pMgntInfo->LockedSTAList_Reject, MAX_LOCKED_STA_LIST_NUM*6 );
	pMgntInfo->bDefaultPermited = TRUE;
		// Initialize offset for querying cloud key
	pMgntInfo->cloud_key_offset = 0;
	pMgntInfo->StaCount = 0;
	
	
	// SW RF state. 2006.12.18, by shien chang.
	if(pMgntInfo->RegRfOff == TRUE)
		pMgntInfo->eSwRfPowerState = eRfOff;
	else
		pMgntInfo->eSwRfPowerState = eRfOn;

	pMgntInfo->LinkDetectInfo.NumRxUnicastTxOkThresh = 8;
	pMgntInfo->LinkDetectInfo.NumRxUnicastThresh = 2;
	pMgntInfo->LinkDetectInfo.NumRxOkThresh = 0xff;
	
	// For inactive power save mode, 2007.07.16, by shien chang.
	pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
	pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
	// Default AC mode, wait for pnp notify to change it.
	pMgntInfo->PowerSaveControl.PowerProfile = 1; 	
	// Default No power saving level, wait for oid notify to change it.
	pMgntInfo->PowerSaveControl.PowerSaveLevel = POWER_SAVING_NO_POWER_SAVING;
	pMgntInfo->PowerSaveControl.PowerMode = POWER_SAVING_NO_POWER_SAVING;
	pMgntInfo->PowerSaveControl.PowerPolicy = POWERCFG_MAX_POWER_SAVINGS;
	pMgntInfo->keepAliveLevel = 0;
	pMgntInfo->PowerSaveControl.bDisableLPSByOID = FALSE;
	pMgntInfo->PowerSaveControl.bEnterLPSDuringSuspend = FALSE;
	pMgntInfo->PowerSaveControl.bFindWakePacket = TRUE; // default set to TRUE
	
	// For network monitor mode, 2007.08.06, by shien chang.
	pMgntInfo->bNetMonitorMode = FALSE;
	
	pMgntInfo->ClientConfigPwrInDbm = UNSPECIFIED_PWR_DBM;

	pMgntInfo->RegCapAKMSuite = DEFAULT_SUPPORT_AKM_SUITE;

	SecInit(Adapter); // 070106, by rcnjko.

	//
	// Clear the rejected association AP, 2006.12.07, by shien chang.
	//
	MgntClearRejectedAsocAP(Adapter);
	
	Dot11d_Reset(Adapter); // 070315, by rcnjko.
	
	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_INIT, NULL, NULL);

	// Initialize AP Mode Info
	AP_InitializeVariables(Adapter);

	TDLS_Init(Adapter);


	//
	// Reset current management setting.
	//
	ResetMgntVariables(Adapter);

	// IHV support type
	pMgntInfo->IhvType = IHV_SUPPORT_NONE;

	pMgntInfo->bDumpRegs = TRUE;

	pMgntInfo->ResumeBeaconTime = DELAY_START_BEACON;

	//
	// 2010/12/28 MH We need to move the init start position.
	// 
	// =====================================

	WPS_Init(Adapter);
	// 2011/01/12 MH Revice for WAPI.
	WAPI_SecFuncHandler(WAPI_ENABLEWAPISUPPORT, Adapter, WAPI_END);
	
	// 2011/12/07 hpfan Add for Tcp Reorder
	TcpReorder_Init(Adapter);
	// =====================================	

	Adapter->bReceiveCpwmInt = FALSE;
#if (USB_TX_THREAD_ENABLE)
	Adapter->bUseUsbTxThread = FALSE;
#endif	

	
	GET_POWER_SAVE_CONTROL(pMgntInfo)->bOSSupportProtocolOffload = TRUE;

	if(Adapter == GetDefaultAdapter(Adapter))
		CustomScan_Init(pMgntInfo->pCustomScanInfo, GetDefaultAdapter(Adapter), BIT45, DBG_LOUD);
}



//
//	Description:
//		DeInitialize management related S/W resource.
//
//	Assumption:
//		This function shall be invoke once at driver unload.
//
VOID
DeInitializeMgntVariables(
	PADAPTER			Adapter
	)
{
	MgntCancelAllTimer(Adapter);
	MgntReleaseAllTimer(Adapter);
	MgntFreeAllWorkItem(Adapter);
	RemoveAllTS(Adapter);
	ReleaseAllTSTimer(Adapter);
	TDLS_Release(Adapter);



	TcpReorder_Release(Adapter);

	CancelWatchDogTimer(Adapter);
	ReleaseWatchDogTimer(Adapter);

	if(Adapter->MgntInfo.pCustomScanInfo)
	{
		CustomScan_Deinit(Adapter->MgntInfo.pCustomScanInfo);
		CustomScan_FreeInfo(Adapter->MgntInfo.pCustomScanInfo);
	}
}


BOOLEAN
MgntGetFWBuffer(
	PADAPTER			Adapter,
	PRT_TCB				*ppTcb,
	PRT_TX_LOCAL_BUFFER	*ppBuf
	)
{
	*ppBuf=GetLocalFWBuffer(Adapter);
	if(*ppBuf!=NULL)
	{
		if(!RTIsListEmpty(&Adapter->TcbIdleQueue))
		{
			*ppTcb=(PRT_TCB)RTRemoveHeadListWithCnt(&Adapter->TcbIdleQueue, &Adapter->NumIdleTcb);

			// Initialize the data rate. Annie, 2005-03-31
			(*ppTcb)->DataRate = UNSPECIFIED_DATA_RATE;

			return TRUE;
		}
		else
		{
			ReturnLocalFWBuffer(Adapter, *ppBuf);
		}
	}
	return FALSE;
}


BOOLEAN
MgntGetBuffer(
	PADAPTER			pAdapter,
	PRT_TCB				*ppTcb,
	PRT_TX_LOCAL_BUFFER	*ppBuf
	)
{
	PADAPTER pDefaultAdapter =  GetDefaultAdapter(pAdapter);

	*ppBuf=GetLocalBuffer(pDefaultAdapter);
	if(*ppBuf!=NULL)
	{
		if(!RTIsListEmpty(&pDefaultAdapter->TcbIdleQueue))
		{
			*ppTcb=(PRT_TCB)RTRemoveHeadListWithCnt(&pDefaultAdapter->TcbIdleQueue, &pDefaultAdapter->NumIdleTcb);

			// Initialize the data rate. Annie, 2005-03-31
			(*ppTcb)->DataRate = UNSPECIFIED_DATA_RATE;

			return TRUE;
		}
		else
		{			
			ReturnLocalBuffer(pDefaultAdapter, *ppBuf);
		}
	}
	return FALSE;
}

VOID
MgntSendPacket(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	PRT_TX_LOCAL_BUFFER	pBuf,
	u4Byte				Length,
	u1Byte				QueueIndex,
	u2Byte				DataRate
	)
{
	//2 Start with first buffer, because we don't need to translate header
	pTcb->BufferList[0]=pBuf->Buffer;
	pTcb->BufferList[0].Length=Length;
	pTcb->PacketLength=Length;
	pTcb->BufferCount=1;
	pTcb->Tailer.Length = 0;

	pTcb->ProtocolType=RT_PROTOCOL_802_11;
	pTcb->BufferType=RT_TCB_BUFFER_TYPE_LOCAL;
	pTcb->SpecifiedQueueID=QueueIndex;
	pTcb->DataRate=DataRate;
	pTcb->bFromUpperLayer = FALSE;
	pTcb->Reserved=pBuf;
	pTcb->FragCount=1;
	pTcb->TSID = DEFAULT_TSID;
	pTcb->pAdapter = Adapter;

	if((pTcb->SpecifiedQueueID == BE_QUEUE) ||
		(!ACTING_AS_AP(Adapter) &&
		(pTcb->SpecifiedQueueID != HIGH_QUEUE && pTcb->SpecifiedQueueID != BEACON_QUEUE)))
	pTcb->SpecifiedQueueID = NORMAL_QUEUE; // Always use normal queue to prevent corrupting BQ.

	// 20090406 Joseph: Send 6M instead of 1M when CCK is disable for IOT issue.
	if(Adapter->MgntInfo.IOTAction & HT_IOT_ACT_DISABLE_CCK_RATE)
	{
		if(pTcb->DataRate == MGN_1M)
			pTcb->DataRate = MGN_6M;
	}

	if(Adapter->MgntInfo.bDisableCck && IS_CCK_RATE(pTcb->DataRate))
	{
		pTcb->DataRate = MGN_6M;
	}

	// 5G band should not send cck rate
	if(IS_WIRELESS_MODE_5G(Adapter))
	{
		if(	pTcb->DataRate == MGN_1M ||pTcb->DataRate == MGN_2M ||
			pTcb->DataRate == MGN_5_5M ||pTcb->DataRate == MGN_11M)
			pTcb->DataRate = MGN_6M;
	}

	NicIFSendPacket(Adapter, pTcb);
}

BOOLEAN 
MgntRetryPacket(
	PADAPTER				pAdapter,
	OCTET_STRING		pduOS,
	u1Byte				packettype
	)
{
	pu1Byte				pDaddr;
	BOOLEAN				retryPkt=FALSE;
	PMGNT_INFO			pMgntInfo = &pAdapter->MgntInfo;

	u2Byte	CurrentSeqNum = (u2Byte)Frame_SeqNum(pduOS);	// 12 bits.

	pDaddr = Frame_Addr1(pduOS);
	//DbgPrint("CurrentSeqNum = %d\n", CurrentSeqNum, CurrentSeqNum);

	if(!MacAddr_isMulticast(pDaddr))
	{	
		if(Frame_Retry(pduOS))
		{
			switch(packettype)
			{
				case Type_Auth:
					if(CurrentSeqNum == pMgntInfo->last_auth_seq)
						retryPkt = TRUE;
					break;
				case Type_Asoc_Req:
					if(CurrentSeqNum == pMgntInfo->last_asoc_req_seq)
						retryPkt = TRUE;
					break;
				case Type_Asoc_Rsp:
					if(CurrentSeqNum == pMgntInfo->last_asoc_rsp_seq)
						retryPkt = TRUE;
					break;
				case Type_Disasoc:
					if(CurrentSeqNum == pMgntInfo->last_Disassoc_seq)
						retryPkt = TRUE;
					break;
				case Type_Deauth:
					if(CurrentSeqNum == pMgntInfo->last_Deauth_seq)
						retryPkt = TRUE;
					break;
				default:
					break;
			}
		}
//		else
		{
			switch(packettype)
			{
				case Type_Auth:
					pMgntInfo->last_auth_seq = CurrentSeqNum;
					break;
				case Type_Asoc_Req:
					pMgntInfo->last_asoc_req_seq = CurrentSeqNum;
					break;
				case Type_Asoc_Rsp:
					pMgntInfo->last_asoc_rsp_seq = CurrentSeqNum;
					break;
				case Type_Disasoc:
					pMgntInfo->last_Disassoc_seq = CurrentSeqNum;
					break;					
				case Type_Deauth:
					pMgntInfo->last_Deauth_seq = CurrentSeqNum;
					break;					
				default:
					break;
			}
		} 		
	}

	return retryPkt;
}

BOOLEAN
MgntDuplicatePacketDetection(
	PADAPTER			Adapter,
	PRT_RFD				pRfd
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	OCTET_STRING	pduOS;
	pu1Byte			pTaddr,  pRaddr;
	u1Byte			CurrentFragNum;	// 4 bits.
	u2Byte			CurrentSeqNum;		// 12 bits.
	
	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;

	pTaddr = Frame_Addr2(pduOS);
	pRaddr = Frame_Addr1(pduOS);

	if(MacAddr_isMulticast(pRaddr))
		return TRUE;

 	// by Owen on 05/01/10 for filtering duplicated data frames and dropping these packets
	//  Only consider unicast and retry packet. 2005.06.21, by rcnjko. 
	// 	To keep the SeqNum, FragNum, SrcAddr for every packets and filter the retry packet which already received.
	//	For WMM, these information should be keep for each AC
	//	For 802.11e, these information shold be keep for each TID.	Joseph, 2005-12-16
	//	Here we just handle the duplicate retry packet which is received immediately after last packets.
	//	For the condition that receiving duplicate packets with Block Ack, we just handle it in RxReorder process. 
	//	Joseph, 2007-09-27

	#if 0  // Sequence Number & Duplicate Packet Indication Debug
	{
		DbgPrint("\n====================================================\n");
		RT_PRINT_ADDR(COMP_INIT, DBG_LOUD, "Tx Address:", pTaddr);
		RT_PRINT_ADDR(COMP_INIT, DBG_LOUD, "Rx Address:", pRaddr);
		DbgPrint("Frame_SeqNum(frame): %d\n", Frame_SeqNum(pduOS));
		DbgPrint("pRfd->Status.pRxTS: %p\n", pRfd->Status.pRxTS);
		DbgPrint("====================================================\n");
	}
	#endif

	CurrentFragNum = (u1Byte)Frame_FragNum(pduOS);
	CurrentSeqNum = (u2Byte)Frame_SeqNum(pduOS);
	
	if(ACTING_AS_AP(Adapter))
	{
		PRT_WLAN_STA pEntry = AsocEntry_GetEntry(pMgntInfo, pTaddr);
		
		if(pEntry != NULL)
		{
			if(pRfd->Status.pRxTS==NULL)
			{
				if( 	Frame_Retry(pduOS) && 
					(CurrentFragNum == pEntry->LastRxUniFragNum) && 
					(CurrentSeqNum == pEntry->LastRxUniSeqNum)	)
				{
					return FALSE;
				}
				else
				{
					pEntry->LastRxUniFragNum = CurrentFragNum;
					pEntry->LastRxUniSeqNum = CurrentSeqNum;
				} 
			}
			else
			{
				if( 	Frame_Retry(pduOS) && 
				(CurrentFragNum == pRfd->Status.pRxTS->RxLastFragNum) && 
				(CurrentSeqNum == pRfd->Status.pRxTS->RxLastSeqNum)	)
				{
					return FALSE;
				}
				else
				{
					pRfd->Status.pRxTS->RxLastFragNum = CurrentFragNum;
					pRfd->Status.pRxTS->RxLastSeqNum = CurrentSeqNum;
				}
			}
		}
		else
		{
			//not retrun false,as 92d dual mac smart concurrent need.
			//example:mac1 soft ap reset,but have no right time to disconnect all sta, so client  still connect,need to send deauth fram as below process . 
			// zhiyuan 2011/11/18
			RT_TRACE(COMP_RECV,DBG_LOUD,("pEntry is null ,don't return false as 92d easy smart concurrent need\n"));
			//	return FALSE;
		}
	}
	else
	{
		if(pRfd->Status.pRxTS==NULL)
		{
			if( 	Frame_Retry(pduOS) && 
				PlatformCompareMemory(pMgntInfo->LastRxUniSrcAddr, pTaddr, 6)== 0 &&
				(CurrentFragNum == pMgntInfo->LastRxUniFragNum) && 
				(CurrentSeqNum == pMgntInfo->LastRxUniSeqNum)	)
			{
				//RT_TRACE(COMP_RECV, DBG_LOUD, ("[SNDBG] STA MgntFilterReceivedPacket():  filtering duplicated data frames and dropping these packets\n"));
				return FALSE;
			}
			else
			{
				pMgntInfo->LastRxUniFragNum = CurrentFragNum;
				pMgntInfo->LastRxUniSeqNum = CurrentSeqNum;
				PlatformMoveMemory( pMgntInfo->LastRxUniSrcAddr, pTaddr, 6 );
				//RT_TRACE(COMP_RECV, DBG_LOUD, ("[SNDBG] STA MgntFilterReceivedPacket():  Record it.\n" ));
			} 
		}
		else
		{
			if( 	Frame_Retry(pduOS) && 
			(CurrentFragNum == pRfd->Status.pRxTS->RxLastFragNum) && 
			(CurrentSeqNum == pRfd->Status.pRxTS->RxLastSeqNum)	)
			{
				return FALSE;
			}
			else
			{
				pRfd->Status.pRxTS->RxLastFragNum = CurrentFragNum;
				pRfd->Status.pRxTS->RxLastSeqNum = CurrentSeqNum;
			}
		}
	}
	
	return TRUE;
}


VOID
MgntWMMPSPacket(
	PADAPTER			Adapter,
	OCTET_STRING		pduOS
)
{
	pu1Byte		pdu = pduOS.Octet;
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;

	if((pMgntInfo->dot11PowerSaveMode !=eActive) && (!ACTING_AS_AP(Adapter)))
	{
		BOOLEAN		bTriggerAC=FALSE;

		switch( GET_QOS_CTRL_WMM_UP(pdu) )
		{
			case 0:
			case 3:
				bTriggerAC=GET_BE_UAPSD(pMgntInfo->pStaQos->Curr4acUapsd);
				break;
			case 1:
			case 2:
				bTriggerAC=GET_BK_UAPSD(pMgntInfo->pStaQos->Curr4acUapsd);
				break;
			case 4:
			case 5:
				bTriggerAC=GET_VI_UAPSD(pMgntInfo->pStaQos->Curr4acUapsd);
				break;
			case 6:
			case 7:
				bTriggerAC=GET_VO_UAPSD(pMgntInfo->pStaQos->Curr4acUapsd);
				break;
		}

		if(bTriggerAC)
		{
			// Temp solution for P2P Test Plan 6.1.13 and 7.1.5. By Bruce, 2010-06-30.
			pMgntInfo->pStaQos->bWmmMoreData = Frame_MoreData(pduOS) ? TRUE : FALSE;
			
			if( GET_QOS_CTRL_WMM_EOSP(pdu) )
			{
				RT_TRACE(COMP_POWER, DBG_TRACE, ("MgntFilterReceivedPacket(): <<<<<<<<<< Leave APSD service period <<<<<<<<<<\n"));
				pMgntInfo->pStaQos->bInServicePeriod=FALSE;
			}
		}
		else
		{
			if(!GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS) // by tynli
			{
				if(pMgntInfo->dot11PowerSaveMode != eActive && (!ACTING_AS_AP(Adapter)))
				{
					if((Frame_MoreData(pduOS)==TRUE))
					{	
						OnMoreData(Adapter);
					}
				}
			}
		}
	}
}



BOOLEAN
MgntFilterReceivedPacket(
	PADAPTER			Adapter,
	PRT_RFD				pRfd
	)
{
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	OCTET_STRING		pduOS;
	u1Byte				WAIPkt = 0;
	AUTH_STATUS_T		state_auth; 
	ASOC_STATUS_T		state_asoc; 
	BOOLEAN				bAcceptable = TRUE;
	BOOLEAN				bValidAddr4;
	BOOLEAN				bToOtherSTA = FALSE;
	PMGNT_INFO 			pDefaultMgntInfo = &(GetDefaultAdapter(Adapter)->MgntInfo);
	pu1Byte				pdu, pDaddr, pTaddr, pRaddr;
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T					pDM_Odm = &pHalData->DM_OutSrc;

	//1 Return FALSE if this RFD should be freed


	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pdu = pRfd->Buffer.VirtualAddress;
	pDaddr = Frame_pDaddr(pduOS);
	pTaddr = Frame_Addr2(pduOS);
	pRaddr = Frame_Addr1(pduOS);
	bValidAddr4 = Frame_ValidAddr4(pduOS);

	//2 Filter out error packet
	if( pRfd->Status.bHwError)
	{
		return FALSE;
	}

	//
	// Filter out unicast frames to other STA.
	// Note that, it is to prevent unicast mgnt frame change our mgnt state machine, 
	// for example, deauth frame to other STA should cause us become deauthenitacted.
	// 2006.01.11, by rcnjko.
	//
	if( !MacAddr_isMulticast(pRaddr) && !eqMacAddr(pRaddr, Adapter->CurrentAddress) )
	{ // Unicast frame to other STA.
	
		if(pMgntInfo->bPSPXlinkMode == TRUE)
		{
			bToOtherSTA = TRUE;
			pRfd->Status.bForward = TRUE;
		}
		else if(pMgntInfo->bNetMonitorMode == TRUE)
		{
			{
				bToOtherSTA = TRUE;
				pRfd->Status.bForward = TRUE;
			}
		}		
		else 
			return FALSE;
	}

	// Filter out data packet that is not AMSDU but length >= 2k
	if (Frame_ContainQc(pduOS) && (GET_QOS_CTRL_HC_CFP_USRSVD(pduOS.Octet)  != 1) && (pRfd->PacketLength>= 2048))
	{
		RT_TRACE(COMP_RECV, DBG_LOUD, 
				("MgntFilterReceivedPacket(): Not AMSDU, but packet lenght > 2k, Dropped!!!===\n"));
		return FALSE;
	}

	// 8192U - if data frames don't decryption by hw then decrypt by sw
	// 8190P/8192E - If data frames is not decrypted by hw then decrypt by sw
	//RxCheckSWDecryption(Adapter, pRfd);

	//====================================
	// Filter packet
	//----------------------------------------------
	// <1>Reject frames before connection established.
	// <2>Reject Encrypted Management frame except 3rd auth frame.
	// <3>Management frame filter 
	// <4>Control frame filter 
	// <5>filter frames from different BSS
	// <6>Data frame filter 
	//-------
	// return 
	//	 FALSE to free received frame
	//	 TRUE to Defrag Packet
	//----------------------------------------------
	// ----------------------------------------------------
	// <1>Reject frames before connection established.
	// ----------------------------------------------------

	if( IsMgntFrame(pdu) )
	{
		if(MgntRetryPacket(Adapter, pduOS, PacketGetType(pduOS)))
			return FALSE;
	}

	// ----------------------------------------------------
	// <2>Reject Encrypted Management frame except 3rd auth frame.
	// ----------------------------------------------------
	if( IsMgntFrame(pdu) ){
		if( Frame_WEP(pduOS) )
		{
			if( IsMgntAuth(pdu) && Frame_AuthTransactionSeqNum(pduOS)==3 && pMgntInfo->AuthReq_auAlg==SHARED_KEY)
			{
				Mgnt_Indicate(Adapter, pRfd, noerr);
			}
			else
			{
				RT_TRACE(COMP_RECV, DBG_LOUD, ("MgntFilterReceivedPacket(): Reject Encrypted Management frame except 3rd auth frame.\n"));
			}
			return FALSE;
		}
	}

	MgntSsInquiry( Adapter, pTaddr, &state_auth, &state_asoc );

	// ----------------------------------------------------
	// <3>Management frame filter 
	// ----------------------------------------------------
	if( IsMgntFrame(pdu) ){
		if (bToOtherSTA )
			return FALSE;
		else
		{
			if( IsMgntBeacon(pdu) || IsMgntProbeReq(pdu) || IsMgntAction(pdu) || IsMgntActionNoAck(pdu)){
				Mgnt_Indicate(Adapter, pRfd, noerr);
			}
			else
			{
				if( IsMgntAuth(pdu) || IsMgntDeauth(pdu) || IsMgntAtim(pdu) || IsMgntProbeRsp(pdu) ){
					if( !MgntIsMacAddrGroup( pDaddr ) ){
						Mgnt_Indicate(Adapter, pRfd, noerr);
					}
					else{
						if( MacAddr_isBcst( pDaddr ) ){
							if( IsMgntDeauth(pdu) ){
								Mgnt_Indicate(Adapter, pRfd, noerr);
							}
						}
					}
				}
				else if( IsMgntAsocReq(pdu) || IsMgntAsocRsp(pdu) || IsMgntReAsocReq(pdu) || IsMgntReAsocRsp(pdu) || IsMgntDisasoc(pdu) ){
					if( (state_auth == opensystem_auth) || (state_auth == sharedkey_auth) || (ft_auth == state_auth)){
						Mgnt_Indicate(Adapter, pRfd, noerr);
					}
					else{
						Mgnt_Indicate(Adapter, pRfd, class2);
					}
				}
			}
			return FALSE;
		}
	}

	// ----------------------------------------------------
	// <4>Control frame filter 
	// ----------------------------------------------------
	if( IsCtrlFrame(pdu) )
	{
		if(bToOtherSTA)
			return FALSE;
		else if( IsCtrlPSpoll(pdu))
		{
			if( ACTING_AS_AP(Adapter))
			{
				if( state_asoc == assoc )
					AP_PS_OnPSPoll(Adapter, pduOS);
				else
				{
					if( state_auth == not_auth )
						Mgnt_Indicate(Adapter, pRfd, class2);					
					else
						Mgnt_Indicate(Adapter, pRfd, class3);
				}
			}
		}
		else if( IsCtrlNDPA(pdu))
		{

			RT_PRINT_DATA(COMP_RECV, DBG_TRACE, "GetNDPAFrame:\n", pduOS.Octet, pduOS.Length);
		}
		else if(IsCtrlBFReportPoll(pdu))
		{
			RT_PRINT_DATA(COMP_RECV, DBG_TRACE, "Get BFReportPoll:\n", pduOS.Octet, pduOS.Length);
		}
		else if(IsCtrlBlockAckReq(pdu))
		{
			OnBAReq(Adapter, pRfd,pduOS);
		}
		return FALSE;
	}


	// ----------------------------------------------------
	// <5>filter frames from different BSS
	// ----------------------------------------------------
	if( !bValidAddr4 && 
		!eqMacAddr(pMgntInfo->Bssid, Frame_pBssid(pduOS)) )
	{
		RT_TRACE(COMP_RECV, DBG_LOUD, ("MgntFilterReceivedPacket(): filter frames from different BSS.\n"));
		RT_TRACE(COMP_RECV, DBG_LOUD, 
		("bValidAddr4 =%d BSSID=[%02x-%02x-%02x-%02x-%02x-%02x]\n", 
		bValidAddr4, pMgntInfo->Bssid[0], pMgntInfo->Bssid[1], pMgntInfo->Bssid[2], 
		pMgntInfo->Bssid[3], pMgntInfo->Bssid[4], pMgntInfo->Bssid[5]));

		return FALSE;
	}

	//
	//  Note : 
	//		This is to fix the issue that if we failed to receive AssocRsp and 
	//		the AP starts 4-way, indicating the EAPOL-KEY may crash the supplicant.
	//		Not sure if this has any impact on BT
	//
	if( IsDataFrame(pdu))
	{
		if(!ACTING_AS_AP(Adapter))
		{
			if( !pMgntInfo->bMediaConnect )
			{
				RT_TRACE(COMP_RECV, DBG_LOUD, ("MgntFilterReceivedPacket(): Media is Disconnect.\n"));
				return FALSE;
			}
				
		}
	}

	// When WPS Enable We save the Packet then Drop the packet we won't pass to OS	
	if(GET_SIMPLE_CONFIG_ENABLED(pDefaultMgntInfo) && SecIsEAPOLPacket( Adapter, &pduOS ))
	{
		WPS_CopyRxEAPPacket(Adapter, pRfd);
		return FALSE;
	}
		
	WAPI_SecFuncHandler(WAPI_SECISWAIPACKET,Adapter,(PVOID)&pduOS,(PVOID)&WAIPkt,WAPI_END);
	if(WAIPkt != 0)//WAI Pkt 
	{
		RT_TRACE(COMP_SEC,DBG_LOUD,("MgntFilterReceivedPacket WAIPkt = %d\n",WAIPkt));
		return TRUE;
	}
			
	if( ACTING_AS_AP(Adapter) )
	{	// AP mode

		// 070209, rcnjko: Update STA information, e.g. rate and power save state.
		AP_PS_UpdateStationPSState( Adapter, &pduOS);

		//
		// AP mode: parse the EAPOL packets. Added by Annie, 2005-07-10.
		// Our authenticator don't handle EAPOL on WDS, 2006.06.14, by rcnjko.
		//
		if( !bValidAddr4 && SecIsEAPOLPacket( Adapter, &pduOS ) && 
			MgntActQuery_ApType(Adapter) != RT_AP_TYPE_VWIFI_AP 
			&& MgntActQuery_ApType(Adapter) != RT_AP_TYPE_LINUX )
		{
			AP_OnEAPOL( Adapter, pRfd );
			return FALSE;
		}
	}
	else
	{	// STA mode		

		if(pMgntInfo->PowerSaveControl.WoWLANS5Support && SecIsEAPOLPacket(Adapter, &pduOS))
		{
			SecStaGetANoseForS5(Adapter, &pduOS);
		}
		
		if(IS_TDL_EXIST(pMgntInfo))
			TDLS_PS_UpdatePeerPSState(Adapter, &pduOS, FALSE);
	}


	if( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_NO_CIPHER )
	{ // station without Encryption Algrithm.
		if( Frame_WEP(pduOS) && !bValidAddr4 )
		{
			RT_TRACE(COMP_RECV, DBG_LOUD, ("MgntFilterReceivedPacket():  station without Encryption Algrithm.\n"));
			bAcceptable = FALSE;
		}
	}
	else
	{ // station with Encryption Algrithm. Only EAPOL packets allowed.
		if( Frame_WEP(pduOS) == FALSE && !bValidAddr4 )
		{
			BOOLEAN			bMoreFrag = (BOOLEAN)Frame_MoreFrag(pduOS);
			u1Byte			FragNum = (u1Byte)Frame_FragNum(pduOS); 
			BOOLEAN 		bWPSEnable = FALSE;

			bWPSEnable = Adapter->pNdis62Common->bWPSEnable;

			// EAPOL Request may be fragments. 2004.10.05, by rcnjko.
			if( (!bMoreFrag && FragNum == 0) && // Non-fragment  
				!SecIsEAPOLPacket(Adapter,&pduOS)
				)
			{
				RT_TRACE(COMP_RECV, DBG_LOUD, ("MgntFilterReceivedPacket():  station with Encryption Algrithm. Only EAPOL packets allowed.\n"));
				//for WLK1.5 softap_wps_ext
				if (pMgntInfo->bExcludeUnencrypted &&(!bWPSEnable || !Adapter->bInHctTest))					
				{
					bAcceptable = FALSE;

					CountRxExcludedUnencryptedStatistics(Adapter, pRfd);
				}
			}

			if(pMgntInfo->IOTPeer == HT_IOT_PEER_MERU)
			{
				
				// 
				// For Meru AP issue. by Neo and Emily, 20101019
				// 		Meru AP send first packet of eapol key right after association
				//		which may cause corruption of WZC stack. The root cause is
				//		driver should ignore all EAPOL packet until WZC is ready, but 
				//		here, driver just drop the first two 1st eapol packet
				if(SecIsEAPOLPacket(Adapter,&pduOS) && 
					pMgntInfo->AcceptFirstEAPOLPacket < 2 &&
					(pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPAPSK || pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPA2PSK))
				{				
					bAcceptable = FALSE;
					pMgntInfo->AcceptFirstEAPOLPacket++;
				}								
			}

		}
	}


	if( bAcceptable == FALSE ){
		return FALSE;
	}

	// 2011.09.22 - Use pRaddr to verify if this packet is unicast or broadcast since in the AP mode, the client will unicast a brocast packet to AP. 
	// Indicate Rx Traffic Stream related information.
	if(	eqMacAddr(pRaddr, Adapter->CurrentAddress) &&
		!MacAddr_isMulticast(pRaddr) &&
		pMgntInfo->pStaQos->CurrentQosMode!=QOS_DISABLE &&
		IsQoSDataFrame(pdu) )
	{
		RxTSIndicate(Adapter, pRfd);
	}
	else
	{
		pRfd->Status.pRxTS = NULL;
	}

	
#if WLAN_ETW_SUPPORT

	pRfd->RfdFrameUniqueueID = RT_INC_RX_FRAME_UNIQUE_ID(Adapter);

	//
	// <Roger_Notes> No activity needs to be associated with this event, the ActivityId is optional and can be NULL.
	// 2014.01.14.
	//
	EventWriteRxIndicationFromNIC(
			NULL, //Without associated with the event
			pRfd->RfdFrameUniqueueID, // FrameUniqueueID
			(IsQoSDataFrame(pduOS.Octet)?Frame_QoSTID(pduOS, sMacHdrLng):0), 	// TID
			0, 	// PeerID
			pRfd->Status.Seq_Num, 	//SequenceNumber
			pRfd->PacketLength, // PayloadLength in bytes
			0, 	// QueueLength
			Frame_Retry(pduOS),// Retransmit
			0,	// Status
			0,  	// CustomData1
			0, 	// CustomData2
			0);	// CustomData3
#endif
	
	// 2011.09.22 - Use pRaddr to verify if this packet is unicast or broadcast since in the AP mode, the client will unicast a brocast packet to AP. 
	if(MgntDuplicatePacketDetection(Adapter, pRfd) == FALSE)
		return FALSE;


	// ----------------------------------------------------
	// <6>Data frame filter 
	// ----------------------------------------------------
	if( IsDataFrame(pdu) )
	{
		if( IsMgntData(pdu) )
		{

			if( MgntIsMacAddrGroup( pDaddr ) && eqMacAddr(pTaddr, Adapter->CurrentAddress) )
			{
				// Filter broadcast frame from this station.
				RT_TRACE(COMP_RECV, DBG_LOUD, ("MgntFilterReceivedPacket(): Filter broadcast frame from this station.\n"));
				return FALSE;
			}
	
			// Our NIC send multicast to AP (ToDS), then AP reply to its clients (FromDS).
			if( eqMacAddr(Adapter->CurrentAddress, Frame_pSaddr(pduOS)) )
			{
				/* In network bridge case: 
				// To avoid 802.1 Bridge Spanning Tree packet infinite loop. 
				// We send Spanning Tree packet to AP, then AP forward our packet to its clients.
				// 2009-04-16, Isaiah 	
				*/	
				RT_TRACE(COMP_RECV, DBG_LOUD, ("MgntFilterReceivedPacket(): Filter Multicast from this station.\n"));
				return FALSE;
			}
	
			if(!GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS) // by tynli
			{
				//Isaiah for APSD TestPlan 2006-07-31
				if((pMgntInfo->dot11PowerSaveMode != eActive && !MacAddr_isMulticast(pRaddr))
					&& (!ACTING_AS_AP(Adapter)))
				{
					if(Frame_MoreData(pduOS)==TRUE)
					{
						OnMoreData(Adapter);
					}
						
				}
			}
		}//Data
		else if(IsQoSDataFrame(pdu))  //Qos Null or Qos data Isaiah 2006-07-25
			MgntWMMPSPacket(Adapter, pduOS);
		else if( IsMgntNullFrame(pdu) ){
			//if( state_auth == not_auth ){ Mgnt_Indicate(Adapter, pRfd, class2); return FALSE; }
		}
		else if( 	IsMgntDataAck(pdu) || IsMgntDataPoll(pdu) || IsMgntDataPoll_Ack(pdu) || 
				IsMgntCfack(pdu) || IsMgntCfpoll(pdu) || IsMgntCfpollAck(pdu) )
		{	//TODO:
		}

		if(!bValidAddr4)
		{ // Return FALSE if station is not associated.
			if( state_asoc != assoc )
			{
				if( state_auth == not_auth )
				{
					Mgnt_Indicate(Adapter, pRfd, class2);
				}
				else
				{
					Mgnt_Indicate(Adapter, pRfd, class3);
				}
				RT_TRACE(COMP_RECV, DBG_LOUD, ("MgntFilterReceivedPacket(): return FALSE if station is not associated.\n"));
				return FALSE;
			}
		} 
		else
		{ // Filter out invalid WDS packets here.
			if(!ACTING_AS_AP(Adapter))
			{
				return FALSE; 
			}

			if(pMgntInfo->WdsMode == WDS_DISABLED)
			{
				return FALSE; 
			}

			if(!eqMacAddr(pTaddr, pMgntInfo->WdsApAddr))
			{
				return FALSE; 
			}
		}
	} 
	
	//Here, ONLY data frame from peer station exists!!
	//====================================
	return TRUE;
}

//
//	Description:
//		Return FALSE if this packet should not be indicated(only forward).
//		If this packet have will be used in more than one purpose, 
//		we will copy it into local buffer.
//
//	<RJ_TODO_WDS>
//		Revise AP_ForwardPacketWithFromDS(), AP_FromBssToWds(), 
//		and AP_FromWdsToBss() if QAP is implemented.
//
//	070214, by rcnjko.
//
BOOLEAN
MgntCheckForwarding(
	PADAPTER			Adapter,
	PRT_RFD				pRfd
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	OCTET_STRING	mpdu;
	pu1Byte			pTaddr;
	pu1Byte			pDstAddr;
	BOOLEAN			bValidAddr4;
	BOOLEAN			bNeedToIndicate = TRUE;

	FillOctetString(mpdu, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	pTaddr = Frame_pTaddr(mpdu);
	pDstAddr = Frame_pDaddr(mpdu);
	bValidAddr4 = Frame_ValidAddr4(mpdu);

	if(!bValidAddr4)
	{
		if(AsocEntry_IsStationAssociated(pMgntInfo, pTaddr))
		{ // If TA is an associated STA.
			PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, pTaddr);

			// Update active time of the STA.
			AsocEntry_UpdateTimeStamp(pEntry);

			if(!eqMacAddr(pDstAddr, Adapter->CurrentAddress))
			{ // If destination address is not AP.

				if(MacAddr_isMulticast(pDstAddr))
				{ // If destination address is bcst/mcst address, 
					// Forward the packe to WDS AP if required.
					if(pMgntInfo->WdsMode != WDS_DISABLED)
					{
						AP_FromBssToWds(Adapter, pRfd, TRUE);
					}
					
				}
#if (P2P_SUPPORT == 1)	
				else if(P2P_ENABLED(GET_P2P_INFO(Adapter)) &&
					!((GET_P2P_INFO(Adapter))->GroupCapability & gcIntraBSSDistribution))
				{// intra bss (forwarding) capability is disabled
					// In this case, we only forward multicast frames. 2010.04.16, haich.
					ReturnRFDList(Adapter, pRfd);
					bNeedToIndicate = FALSE;
				}
#endif				
				else if(AsocEntry_IsStationAssociated(pMgntInfo, pDstAddr))
				{ // If destination is a STA associated.				
					// Forward the packet.
					bNeedToIndicate = TRUE;
				}
				else
				{
					// In WDS mode: unicast frame not to BSS or us will beforwad to WDS AP and don't indicate it up.
					if(pMgntInfo->WdsMode != WDS_DISABLED)
					{
						AP_FromBssToWds(Adapter, pRfd, TRUE);
					}

					// Peer is not associated -----------------------------------
					ReturnRFDList(Adapter, pRfd);
					bNeedToIndicate = FALSE;
					// -----------------------------------------------------
				}
			}
		}
	}
	else
	{ // WDS frame received: Forward it to current BSS if required.
	  // Note that, if we fall into this case, pMgntInfo->bWdsMode should not be WDS_DISABLED and 
	  // TA is a valid WDS AP, see also MgntFilterReceivedPacket().
		
		if(!eqMacAddr(pDstAddr, Adapter->CurrentAddress))
		{ // If destination address is not AP.
			if(MacAddr_isMulticast(pDstAddr))
			{ // If destination address is bcst/mcst address, we forard it to BSS and also indicate up.  
	
				// Forward the packet to BSS.
				AP_FromWdsToBss(Adapter, pRfd, TRUE);

				// We will indicate it up later.
			}
			else if(AsocEntry_IsStationAssociated(pMgntInfo, pDstAddr))
			{ // If destination is a STA associated.
	
				// Forward the packet to BSS.
				if(AP_FromWdsToBss(Adapter, pRfd, TRUE) == FALSE)
				{
					// If we failed to forward it, so just return the RFD here.
					RT_TRACE(COMP_AP, DBG_WARNING, ("MgntCheckForwarding(): Failed to forward packet from WDS to BSS!\n"));
				}
	
				// Don't indicate it up.
				ReturnRFDList(Adapter, pRfd);
				bNeedToIndicate = FALSE;
			}
			else
			{
				// Don't indicate it up.
				ReturnRFDList(Adapter, pRfd);
				bNeedToIndicate = FALSE;
			}
		}

		// If this is a unicast frame to us in AP mode, we'll indicate it up later.
	}

	return bNeedToIndicate;
}


BOOLEAN
MgntFilterTransmitPacket(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);

	if( pMgntInfo->mDisable==TRUE &&  pMgntInfo->bMediaConnect==FALSE )
	{
		//
		// Reject data frame if we are disconnected or mDisable.
		//
		if(pTcb->ProtocolType == RT_PROTOCOL_802_3)
		{
			return FALSE;
		}
		else if(pTcb->ProtocolType == RT_PROTOCOL_802_11)
		{
			if( IsFrameTypeData(pTcb->BufferList[0].VirtualAddress) )
				return FALSE;
		}
	}

	if( ACTING_AS_AP(Adapter) )
	{
		pu1Byte pRaddr = NULL;
		BOOLEAN bDataFrame = FALSE;
		BOOLEAN bWithAddr4 = FALSE;
		
		//
		// Get destination address and frame type.
		//
		if(pTcb->ProtocolType == RT_PROTOCOL_802_3)
		{
			pRaddr = pTcb->BufferList[2].VirtualAddress;
			RT_ASSERT(pRaddr!=NULL, ("MgntFilterTransmitPacket(): 802.3 pRaddr==NULL\n"));

			bDataFrame = TRUE;
		}
		else if(pTcb->ProtocolType == RT_PROTOCOL_802_11)
		{
			pu1Byte pHeader = pTcb->BufferList[0].VirtualAddress;
			RT_ASSERT(pHeader !=NULL, ("MgntFilterTransmitPacket(): 802.11 pHeader==NULL\n"));
		
			pRaddr = pHeader + 4;
			RT_ASSERT(pRaddr!=NULL, ("MgntFilterTransmitPacket(): 802.11 pRaddr==NULL\n"));
			
			if( IsFrameTypeData(pHeader) )
				bDataFrame = TRUE;

			if( IsFrameWithAddr4(pHeader) )
				bWithAddr4 = TRUE;
		}

		//
		// Reject data frame dest STA is not associated.
		//			
		if(pRaddr != NULL && !MacAddr_isMulticast(pRaddr) && bDataFrame)
		{ // Data frame with unicast destination.
			if(!bWithAddr4)
			{
				if( pMgntInfo->WdsMode == WDS_DISABLED && 
					!AsocEntry_IsStationAssociated(pMgntInfo, pRaddr) )
				{ // Destination STA is not associated.
					RT_TRACE( COMP_AP, DBG_LOUD,
						("MgntFilterTransmitPacket(): Destination STA is not associated, pRaddr: %02X-%02X-%02X-%02X-%02X-%02X!\n",
					pRaddr[0],pRaddr[1],pRaddr[2],pRaddr[3],pRaddr[4],pRaddr[5]) );
					return FALSE;
				}
			}
			else
			{
				if(pMgntInfo->WdsMode == WDS_DISABLED)
				{
					RT_TRACE( COMP_AP, DBG_LOUD, ("MgntFilterTransmitPacket(): bWithAddr4 but WdsMode is WDS_DISABLED!\n"));
					return FALSE;
				}

				if(!eqMacAddr(pRaddr, pMgntInfo->WdsApAddr))
				{
					RT_TRACE( COMP_AP, DBG_LOUD,
						("MgntFilterTransmitPacket(): unknown WDS AP, pRaddr: %02X-%02X-%02X-%02X-%02X-%02X!\n",
						pRaddr[0],pRaddr[1],pRaddr[2],pRaddr[3],pRaddr[4],pRaddr[5]) );
					return FALSE;
				}
			}
		}
	}
	
	return TRUE;
}


RT_STATUS
MgntAllocateBeaconBuf(
	PADAPTER			Adapter
	)
{
	RT_STATUS	status;
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;

	status = PlatformAllocateSharedMemory( 
			Adapter, 
			&pMgntInfo->BcnSharedMemory, 
			BEACON_FRAME_LEN 
			);

	if( status == RT_STATUS_SUCCESS ){
		pMgntInfo->beaconframe.Octet = pMgntInfo->BcnSharedMemory.VirtualAddress;
		pMgntInfo->beaconframe.Octet += Adapter->TXPacketShiftBytes;
		pMgntInfo->beaconframe.Length = 0;
	}

	return status;
}


VOID
MgntFreeBeaconBuf(
	PADAPTER			Adapter
	)
{
	PlatformFreeSharedMemory( Adapter, &Adapter->MgntInfo.BcnSharedMemory );
}


BOOLEAN 
MgntIsMacAddrGroup( 
	pu1Byte addr 
	)
{
	if( (addr[0]&0x01) && (MacAddr_isBcst(addr)==FALSE) )
		return TRUE;
	else
		return FALSE;
}


VOID
MgntSsInquiry( 
	PADAPTER			Adapter,
	pu1Byte				pSaddr, 
	PAUTH_STATUS_T		state_auth, 
	PASOC_STATUS_T		state_asoc 
	)
{
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;

	*state_auth = not_auth;
	*state_asoc = disassoc;

	if(!ACTING_AS_AP(Adapter))
	{ // STA mode.
		if( pMgntInfo->AuthStatus == AUTH_STATUS_SUCCESSFUL ){
			if( pMgntInfo->AuthReq_auAlg == OPEN_SYSTEM ){
				*state_auth = opensystem_auth;
			}
			else if(  pMgntInfo->AuthReq_auAlg == SHARED_KEY ){
				*state_auth = sharedkey_auth;
			}
			else if(AUTH_ALG_FT == pMgntInfo->AuthReq_auAlg)
			{
				*state_auth = ft_auth;
			}
		}
	
		if( pMgntInfo->mAssoc || pMgntInfo->mIbss ){
			*state_asoc = assoc;
		}
	}
	else
	{ // AP mode.
		PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, pSaddr);

		if(pEntry != NULL)
		{
			if(pEntry->AuthAlg == OPEN_SYSTEM && pEntry->AuthPassSeq == 2) 
			{
				*state_auth = opensystem_auth;
			}
			else if(pEntry->AuthAlg == SHARED_KEY && pEntry->AuthPassSeq == 4) 
			{
				*state_auth = sharedkey_auth;
			}
			
			if(*state_auth != not_auth)
			{
				*state_asoc = (pEntry->bAssociated) ? assoc : disassoc; 
			}	
		}
	}
}




// return TRUE if encryption invoked.
BOOLEAN
MgntGetEncryptionInfo(
	IN	PADAPTER			Adapter,
	IN	PRT_TCB				pTcb,
	IN	PSTA_ENC_INFO_T		pEncInfo,
	IN	BOOLEAN				bIncHead
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;	
	pu1Byte 		pDestaddr = NULL;
	pu1Byte		pheader = pTcb->BufferList[0].VirtualAddress;	

	if(pMgntInfo->bScanWithMagicPacket)
	{ // We don't want ot encrypt magic packet, so any frames sending in this period won't be encrypted. 2005.06.27, by rcnjok.
		return FALSE;
	}

	if( bIncHead )
		//xiong
		//pheader += USB_HWDESC_HEADER_LEN;
	{
		pheader += Adapter->TXPacketShiftBytes;
	}

	pEncInfo->bMFPPacket = FALSE;

	GetTcbDestaddr(Adapter, pTcb, &pDestaddr);

	if( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm!= RT_ENC_ALG_NO_CIPHER ) 
	{
		if( !IsDataFrame(pheader) )
		{
			if( IsMgntFrame(pheader) )
			{
				OCTET_STRING osMgntFrame;

				FillOctetString(osMgntFrame, pheader, sMacHdrLng);
				// Only 3rd auth management frame had set WEP bit, see also ConstructAuthenticatePacket(), 2005.08.18, by rcnjko.
				if( IsMgntAuth(pheader) && Frame_WEP(osMgntFrame))
				{
					// Encrypt it via SW encryption. 2005.06.27, by rcnjko.
					pEncInfo->IsEncByHW = FALSE;
				}
				// MFP Case !!
				else if(Frame_WEP(osMgntFrame) )
				{
					pEncInfo->IsEncByHW = FALSE;
					pEncInfo->bMFPPacket = TRUE;
				}
				else
				{
					return FALSE;
				}
			}
			else
			{
				return FALSE;
			}
		}
		else 
		{
			OCTET_STRING pTxFrame;

			// Windows lets us do no encryption: DOT11_EXEMPT_ALWAYS
			if( pEncInfo->ExemptionActionType == 2)
			{
				pEncInfo->IsEncByHW = FALSE;
				return FALSE;
			}
			
			if( IsNoDataFrame(pheader))
			{
				// We SHOULD NOT encrypted such packet.
				pEncInfo->IsEncByHW = FALSE;
				return FALSE;
			}

			// <RJ_TODO_WDS> We had not yet implement WDS encryption/decryption. 2006.06.12, by rcnjko.
			if(IsFrameWithAddr4(pheader))
			{
				return FALSE;
			}
			
			pEncInfo->IsEncByHW = TRUE;

			FillOctetString(pTxFrame, pheader, (u2Byte)pTcb->BufferList[0].Length);
			// Only send EAP packet in open when AP do WPS Message exchange 
			
			if(GET_SIMPLE_CONFIG_ENABLED(GetDefaultMgntInfo(Adapter)) && SecIsEAPOLPacket( Adapter, &pTxFrame ))
			{
				RT_TRACE(COMP_WPS,DBG_LOUD,("In WPS mode, IsEncByHW = false!\n"));
				pEncInfo->IsEncByHW = FALSE;
				return FALSE;
			}

			if(!IsSecProtEapol(pEncInfo->SecProtInfo))
			{ // Non-EAPOL packet.
				// Set IsEncByHW to TRUE to let pMgntInfo->SecurityInfo.SWTxEncryptFlag to determine if this packet shall be encrypted by HW or SW.
				pEncInfo->IsEncByHW = TRUE;
			}
			else
			{ // EAPOL packet.
				if(IsSecProtEapolBeforeKeyInstalled(pEncInfo->SecProtInfo))
				{ // Before PTK installed.
					//RT_ASSERT(!IsSecProtEapolAfterKeyInstalled(pEncInfo->SecProtInfo), ("%s(): Invalid pEncInfo->SecProtInfo: %#X\n", __FUNCTION__, pEncInfo->SecProtInfo) );
					RT_ASSERT(!IsSecProtEapolAfterKeyInstalled(pEncInfo->SecProtInfo), ("MgntGetEncryptionInfo(): Invalid pEncInfo->SecProtInfo: %#lX\n", pEncInfo->SecProtInfo) );
					// We CANNOT encrypt the EAPOL packet before PTK installed.
					pEncInfo->IsEncByHW = FALSE;
					return FALSE;
				}
				else
				{ // After PTK installed.
					// We want to encrypt EAPOL packet after PTK installed by "SW encryption".
					pEncInfo->IsEncByHW = FALSE;
				}
			}

			//For WEP 802.1x Roam... EAPOL-Packet can't encode ...
			{
				BOOLEAN		bAPSuportCCKM = FALSE;
				BOOLEAN		bCCX8021xenable = FALSE;

				CCX_QueryCCKMSupport(Adapter, &bCCX8021xenable, &bAPSuportCCKM);
				
				if( ( pMgntInfo->SecurityInfo.PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 || 
					pMgntInfo->SecurityInfo.PairwiseEncAlgorithm ==RT_ENC_ALG_WEP104 ) &&  // WEP mode 
					pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeOpen &&    // Open mode
					!( bCCX8021xenable && bAPSuportCCKM )      // Not in CCKM mode
					)
				{
					
					if( IsSecProtEapol(pEncInfo->SecProtInfo) )
					{			
						RT_TRACE(COMP_SEC, DBG_LOUD, ("====> WEP EAPOL Packet No Encode ....\n"));
						pEncInfo->IsEncByHW = FALSE;
						return FALSE;
					}
				}
			}

			RT_TRACE(COMP_SEC, DBG_TRACE, ("====> MgntGetEncryptionInfo() ExemptionActionType = %x \n",pEncInfo->ExemptionActionType));
		}

		// 2008.04.01
		if(Adapter->HalFunc.GetNmodeSupportBySWSecHandler(Adapter))
		{
			pEncInfo->IsEncByHW = FALSE;	
		}
		else if( MacAddr_isBcst(pDestaddr) )
		{//broadcast frame

			if( 	pMgntInfo->NdisVersion  >= RT_NDIS_VERSION_6_20 ||
				pMgntInfo->bConcurrentMode ||
				pMgntInfo->bBTMode)
				pEncInfo->IsEncByHW = FALSE;
		}
		else if( MacAddr_isMulticast(pDestaddr) )
		{//multicast frame
		
			if( pMgntInfo->NdisVersion  >= RT_NDIS_VERSION_6_20 ||
				pMgntInfo->bConcurrentMode  ||
				pMgntInfo->bBTMode)
				pEncInfo->IsEncByHW = FALSE;	
		}
		else
		{//Unicast frame
	
			//
			//  In Win7 WEP and AD mdoe always used SW encrypt !!
			//
			if( 	pMgntInfo->NdisVersion  >= RT_NDIS_VERSION_6_20 ||
				pMgntInfo->bConcurrentMode ||
				pMgntInfo->bBTMode)
			{
				
				if (	(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm== RT_ENC_ALG_WEP40) ||
					(pMgntInfo->SecurityInfo.PairwiseEncAlgorithm== RT_ENC_ALG_WEP104)||
					pMgntInfo->bRSNAPSKMode )
				{
					pEncInfo->IsEncByHW = FALSE;
				}

				if( pMgntInfo->OpMode == RT_OP_MODE_IBSS )
				{
					pEncInfo->IsEncByHW = FALSE;
				}
			}
		}
		
		//just for FPGA Verification: Adhoc throughput tset
		//multicast, broadcast, unicast data frames all use HW encrypt
		if(pMgntInfo->bRegAdhocUseHWSec && pMgntInfo->OpMode == RT_OP_MODE_IBSS)
		{
			pEncInfo->IsEncByHW = TRUE;
		}
		
		pEncInfo->EncAlg = pMgntInfo->SecurityInfo.PairwiseEncAlgorithm; 
		pEncInfo->keyId = pMgntInfo->SecurityInfo.DefaultTransmitKeyIdx;
		pEncInfo->keylen = pMgntInfo->SecurityInfo.KeyLen[pEncInfo->keyId];
		pEncInfo->keybuf = pMgntInfo->SecurityInfo.KeyBuf[pEncInfo->keyId];

		return TRUE;
	}

	return FALSE;
}



u1Byte
MgntQuery_MgntFrameTxRate(
	PADAPTER		Adapter,
	pu1Byte			dstaddr
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	u1Byte		rate;

	if(pMgntInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)
	{
		rate = MgntQuery_TxRateExcludeCCKRates(pMgntInfo->mBrates);
		RT_DISP(FDM, WA_IOT, ("MgntQuery_MgntFrameTxRate(), rate = 0x%x\n", rate));
	}
	else
		rate = pMgntInfo->CurrentBasicRate & 0x7f;
	
	if(rate == 0){
		// 2005.01.26, by rcnjko.
		if(	IS_WIRELESS_MODE_5G(Adapter) ||
			(IS_WIRELESS_MODE_HT_24G(Adapter) &&!pMgntInfo->pHTInfo->bCurSuppCCK))
			rate = MGN_6M;
		else
			rate = MGN_1M;
	}

	return rate;
}

u2Byte
MgntQuery_DataFrameTxRate(
	PADAPTER		Adapter,
	pu1Byte			dstaddr
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	u2Byte		rate = MGN_1M;
	PADAPTER	pDefAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO	pDefMgntInfo = &pDefAdapter->MgntInfo;

	if(pDefMgntInfo->ForcedDataRate != 0)
		rate = pMgntInfo->ForcedDataRate;
	else if(MacAddr_isMulticast(dstaddr))
	{ // Bcst/Mcst.
		if(pMgntInfo->ForcedBstDataRate != 0)
			rate = pMgntInfo->ForcedBstDataRate;
		else
			rate = pMgntInfo->LowestBasicRate & 0x7f;
	}
	else if( ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
	{
		PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, dstaddr);

		if(Adapter->RASupport)
		{
			if(pEntry != NULL)
			{
				u1Byte DecisionRate = pEntry->AssociatedMacId;
				Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_RA_DECISION_RATE, (pu1Byte)&DecisionRate);
				rate = Adapter->HalFunc.GetHwRateFromMRateHandler(DecisionRate);
			}
		}
		else
		{
			rate = CURRENT_RATE(	pMgntInfo->dot11CurrentWirelessMode, 
									pMgntInfo->CurrentOperaRate, 
									pMgntInfo->HTHighestOperaRate);

			if(pEntry != NULL)
			{
				u1Byte			PeerHighestOpRate;

				if(rate >= MGN_MCS0)
					PeerHighestOpRate = pEntry->HTInfo.HTHighestOperaRate;
				else
					PeerHighestOpRate = pEntry->HighestOperaRate;

				if( rate > PeerHighestOpRate )
				{
					rate = PeerHighestOpRate;
				}
			}
			else
			{
				if(	pMgntInfo->WdsMode != WDS_DISABLED &&
					eqMacAddr(dstaddr, pMgntInfo->WdsApAddr) )
				{
					rate = pMgntInfo->CurrentOperaRate & 0x7f;
				}
				else
				{
					RT_PRINT_ADDR(COMP_SEND, DBG_WARNING, 
						"MgntQuery_DataFrameTxRate(): nonassociated STA:\n", dstaddr);

					rate = pMgntInfo->LowestBasicRate & 0x7f;
				}
			}
		}		
	}
	else
	{//STA mode.
		if(Adapter->RASupport)
		{
			u1Byte DecisionRate = 0; //0
			Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_RA_DECISION_RATE, (pu1Byte)&DecisionRate);
			rate = Adapter->HalFunc.GetHwRateFromMRateHandler(DecisionRate);
		}
		else
		{
			rate = CURRENT_RATE(pMgntInfo->dot11CurrentWirelessMode, 
									pMgntInfo->CurrentOperaRate, 
									pMgntInfo->HTHighestOperaRate);	

		}
	}

	if(rate == 0)
	{ 
		// 2005.01.13, by rcnjko.
		if(	IS_WIRELESS_MODE_5G(Adapter) ||
			(IS_WIRELESS_MODE_HT_24G(Adapter) && !pMgntInfo->pHTInfo->bCurSuppCCK))
			rate = MGN_6M;
		else
			rate = MGN_1M;
	}

	return rate;
}

u2Byte
MgntQuery_FrameTxRate(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	)
{
	u2Byte		rate =MGN_1M;
	pu1Byte		pRa = NULL;

	// We cannot use GeTcbDestaddr() here because it is not coalensed, 2005.07.05, by rcnjko.
	RT_ASSERT(pTcb->ProtocolType==RT_PROTOCOL_802_11, ("MgntQuery_FrameTxRate(): ProtocolType: %x\n", pTcb->ProtocolType));
	GetTcbDestaddr(Adapter, pTcb, &pRa);

	if( IsFrameTypeData( pTcb->BufferList[0].VirtualAddress ) )
	{
		if(	(Adapter->MgntInfo.NdisVersion>=RT_NDIS_VERSION_6_20) 
			&& Adapter->bInHctTest && ACTING_AS_AP(Adapter))
		{
			if(!MacAddr_isMulticast(pRa))
				pTcb->DataRate=MGN_54M;
			else
				pTcb->DataRate=MGN_1M;

			pTcb->bTxUseDriverAssingedRate=TRUE;
			rate = pTcb->DataRate;
		}
		else if(  pTcb->DataRate == UNSPECIFIED_DATA_RATE  )
			rate = MgntQuery_DataFrameTxRate( Adapter, pRa );
		else
			rate = pTcb->DataRate;
	}
	else if( IsFrameTypeMgnt( pTcb->BufferList[0].VirtualAddress ) ){

		// If data rate is already assigned, we don't have to query it again. Annie, 2005-03-31
		if(  pTcb->DataRate == UNSPECIFIED_DATA_RATE  )
			rate = MgntQuery_MgntFrameTxRate( Adapter, pRa );
		else
			rate = pTcb->DataRate;
	}
	else{
		
		if(  pTcb->DataRate == UNSPECIFIED_DATA_RATE  )
		{
			 if(	IS_WIRELESS_MODE_5G(Adapter) ||
				(IS_WIRELESS_MODE_HT_24G(Adapter)&& !Adapter->MgntInfo.pHTInfo->bCurSuppCCK))
				rate = MGN_6M;
			else
				rate = MGN_1M;
		}	
		else
			rate = pTcb->DataRate;
	}

	return rate;
}


u1Byte
MgntQuery_NssTxRate(
	IN	u2Byte			Rate
	)
{
	u1Byte	NssNum = RF_TX_NUM_NONIMPLEMENT;
	
	if ( ( Rate >= MGN_MCS8 && Rate <= MGN_MCS15 ) || 
		 ( Rate >= MGN_VHT2SS_MCS0 && Rate <= MGN_VHT2SS_MCS9 ) )
		NssNum = RF_2TX;
	else if ( ( Rate >= MGN_MCS16 && Rate <= MGN_MCS23 ) || 
		 ( Rate >= MGN_VHT3SS_MCS0 && Rate <= MGN_VHT3SS_MCS9 ) )
		NssNum = RF_3TX;
	else if ( ( Rate >= MGN_MCS24 && Rate <= MGN_MCS31 ) || 
		 ( Rate >= MGN_VHT4SS_MCS0 && Rate <= MGN_VHT4SS_MCS9 ) )
		NssNum = RF_4TX;
	else
		NssNum = RF_1TX;
		
	return NssNum;
}


u2Byte
MgntQuery_SequenceNumber(
	PADAPTER		Adapter,
	pu1Byte			pFrame,
	BOOLEAN			bBTTxPacket,
	pu1Byte			pDestAddr,
	u1Byte			TID
)
{
	u2Byte		SeqNum;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	if(IsCtrlNDPA(pFrame))
	{
		SeqNum = pMgntInfo->SoundingSequence;
		if( pMgntInfo->SoundingSequence >= 0x3f )
			pMgntInfo->SoundingSequence = 0;
		else
			pMgntInfo->SoundingSequence++;
	}
	else if(	bBTTxPacket ||
			IsFrameTypeMgnt(pFrame)|| IsLegacyDataFrame(pFrame) || 
			MacAddr_isBcst(pDestAddr) ||MacAddr_isMulticast(pDestAddr) )
	{ // Management and Legacy Data
		SeqNum = pMgntInfo->SequenceNumber;
		if( pMgntInfo->SequenceNumber >= 0x0fff )
			pMgntInfo->SequenceNumber = 0;
		else
			pMgntInfo->SequenceNumber++;

		//RT_DISP(FQoS, QoS_INIT,  ("MGNT_LEGACY SEQ=%d \r\n", pMgntInfo->SequenceNumber));		
	}
	else 
	{ // QoS Data
		PTX_TS_RECORD pTS;
		if(GetTs(Adapter, ((PTS_COMMON_INFO *)&pTS), pDestAddr,  TID, TX_DIR, TRUE))
		{
			SeqNum = pTS->TxCurSeq;		
			//RT_DISP(FQoS, QoS_INIT,  ("pTS->TxCurSeq=%04d\r\n", pTS->TxCurSeq));	
			pTS->TxCurSeq = (SeqNum + 1) % 4096;
			RT_DISP(FQoS, QoS_INIT,  ("Qos data SEQ=%04d TID = %d\r\n", SeqNum, TID));
		}else
			SeqNum = 0;
	}
	
	return SeqNum;
}


BOOLEAN
MgntIsShortPreambleMode(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
)
{
	PMGNT_INFO  pMgntInfo = &Adapter->MgntInfo;
	BOOLEAN		IsShortPreamble = FALSE;
	pu1Byte 		pDestaddr = NULL;

	GetTcbDestaddr(Adapter, pTcb, &pDestaddr);

	if( ACTING_AS_AP(Adapter))
	{
	}
	else
	{
		// 2005.01.06, by rcnjko.
		if( pMgntInfo->dot11CurrentPreambleMode == PREAMBLE_SHORT ) 
		{
			IsShortPreamble = TRUE;
		}
	}

	if(pTcb->bBTTxPacket)
	{
		IsShortPreamble = TRUE;
	}

	return IsShortPreamble;


}



BOOLEAN
MgntIsMulticastFrame(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
)
{
	pu1Byte 		pDestaddr = NULL;

	GetTcbDestaddr(Adapter, pTcb, &pDestaddr);

	if( (pDestaddr[0]&0x01) && (MacAddr_isBcst(pDestaddr)==FALSE) )
		return TRUE;
	else
		return FALSE;
}





BOOLEAN
MgntIsBroadcastFrame(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
)
{
	pu1Byte 		pDestaddr = NULL;

	GetTcbDestaddr(Adapter, pTcb, &pDestaddr);

	if( MacAddr_isBcst(pDestaddr) )
		return TRUE;
	else
		return FALSE;
}


//
// We do not record QosNull and Null Frame to reduce traffic,
// then it will not enter busy traffic state frequently. 2009.04.08 by tynli.
//
BOOLEAN
MgntIsDataOnlyFrame(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
)
{
	pu1Byte pHeader = GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb);

	if(pTcb->bInsert8BytesForEarlyMode)
		pHeader += 8;
		
	if(IsDataFrame(pHeader) && 
		(!IsMgntQosNull(pHeader)) && 
		(!IsMgntNullFrame(pHeader)))
	{
		return TRUE;
	}
	else
		return FALSE;
}


BOOLEAN
MgntIsRateValidForWirelessMode(
	u1Byte	rate,		
	u1Byte	wirelessmode
	)
{
	BOOLEAN bReturn = FALSE;

	switch(wirelessmode)
	{
	case WIRELESS_MODE_A:
		if((rate >= MGN_6M) && (rate <= MGN_54M) && (rate != MGN_11M))
			bReturn = TRUE;
		break;

	case WIRELESS_MODE_B:
		if((rate <= MGN_11M) && (rate != MGN_6M) && (rate != MGN_9M))
			bReturn = TRUE;		
		break;

	case WIRELESS_MODE_G:
		if((rate >= MGN_1M) && (rate <= MGN_54M))
			bReturn = TRUE;
		break;

	case WIRELESS_MODE_N_24G:
		if((rate >= MGN_MCS0) && (rate <= MGN_MCS31)) 
			bReturn = TRUE;
		else if((rate >= MGN_1M) && (rate <= MGN_54M))
			bReturn = TRUE;
		break;

	case WIRELESS_MODE_N_5G:
		if((rate >= MGN_MCS0) && (rate <= MGN_MCS31)) 
			bReturn = TRUE;
		else if((rate >= MGN_6M) && (rate <= MGN_54M) && (rate != MGN_11M))
			bReturn = TRUE;
		break;

	case WIRELESS_MODE_AC_5G:
		if((rate >= MGN_MCS0) && (rate <= MGN_MCS31)) 
			bReturn = TRUE;
		else if((rate >= MGN_6M) && (rate <= MGN_54M) && (rate != MGN_11M))
			bReturn = TRUE;
		else if((rate >= MGN_VHT1SS_MCS0) && (rate <= MGN_VHT4SS_MCS9))
			bReturn = TRUE;
		break;

	case WIRELESS_MODE_AC_24G:	
		if((rate >= MGN_MCS0) && (rate <= MGN_MCS31)) 
			bReturn = TRUE;
		else if((rate >= MGN_VHT1SS_MCS0) && (rate <= MGN_VHT4SS_MCS9))
			bReturn = TRUE;
		else if((rate >= MGN_1M) && (rate <= MGN_54M))
			bReturn = TRUE;
		break;

	case WIRELESS_MODE_AUTO:
		RT_ASSERT( FALSE, ("MgntIsRateValidForWirelessMode(): wirelessmode should not be WIRELESS_MODE_AUTO\n"));
		break;

	default:
		RT_ASSERT( FALSE, ("MgntIsRateValidForWirelessMode(): Unknown wirelessmode: %d\n", wirelessmode));
		break;
	}

	return bReturn;
}



u4Byte
MgntGetChannelFrequency(
	u4Byte		channel
	)
{	// Return in MHz

	// 11b channel
	if(channel <= 14)
		return DSSS_Freq_Channel[channel];

	// 11a channel
	if(channel < 184)
		return 5000+5*channel;

	// 11j channel
	return 5000-5*(200-channel);
}

VOID
MgntUpdateAsocInfo(
	PADAPTER					Adapter,
	UPDATE_ASOC_INFO_ACTION	Action,
	pu1Byte						Content,
	u4Byte						ContentLength
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PASOC_INFO	pAsocInfo = &(pMgntInfo->AsocInfo);

	if (ContentLength > ASOC_INFO_IE_SIZE)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("MgntUpdateAsocInfo:: The Memory Length(%d) of Action(%d) is exceed %d\n", ContentLength, Action, ASOC_INFO_IE_SIZE));

		ContentLength= ASOC_INFO_IE_SIZE;
	}
	
	switch (Action)
	{
	case UpdateAuthSeq1:
		if(ContentLength <= sMacHdrLng)
		{
			RT_TRACE_F(COMP_MLME, DBG_SERIOUS, ("Incorrect ContentLength = %d by UpdateAuthSeq1\n", ContentLength));
			break;
		}
		PlatformMoveMemory(
			pAsocInfo->AuthSeq1,
			Content,
			ContentLength);
		pAsocInfo->AuthSeq1Length = ContentLength;
		break;

	case UpdateAuthSeq2:
		if(ContentLength <= sMacHdrLng)
		{
			RT_TRACE_F(COMP_MLME, DBG_SERIOUS, ("Incorrect ContentLength = %d by UpdateAuthSeq2\n", ContentLength));
			break;
		}
		
		PlatformMoveMemory(
			pAsocInfo->AuthSeq2,
			Content,
			ContentLength);
		pAsocInfo->AuthSeq2Length = ContentLength;
		break;
		
	case UpdateAsocReq:
		PlatformMoveMemory(
			pAsocInfo->AsocReq,
			Content,
			ContentLength);
		pAsocInfo->AsocReqLength = ContentLength;
		break;

	case UpdateAsocResp:
		PlatformMoveMemory(
			pAsocInfo->AsocResp,
			Content,
			ContentLength);
		pAsocInfo->AsocRespLength = ContentLength;
		break;

	case UpdateAsocBeacon:
		PlatformMoveMemory(
			pAsocInfo->Beacon,
			Content,
			ContentLength);
		pAsocInfo->BeaconLength = ContentLength;
		break;

	case UpdateAsocPeerAddr:
		PlatformMoveMemory(
			pAsocInfo->PeerAddr,
			Content,
			6);
		break;

	case UpdateFlagReAsocReq:
		pAsocInfo->FlagReAsocReq = *((PBOOLEAN)Content);
		break;

	case UpdateFlagReAsocResp:
		pAsocInfo->FlagReAsocResp = *((PBOOLEAN)Content);
		break;
		
	case UpdateDeauthAddr:
		cpMacAddr(pAsocInfo->DeauthAddr, (pu1Byte)Content); 
		pAsocInfo->bDeauthAddrValid = TRUE;
		break;
		
	default:
		break;
	}
}

BOOLEAN
MgntIsInExcludedMACList(
	PADAPTER					Adapter,
	pu1Byte						pAddr
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	ULONG i;

	for (i=0; i<pMgntInfo->ExcludedMacAddrListLength; i++)
	{
		if (PlatformCompareMemory(
			&pMgntInfo->ExcludedMacAddr[i],
			pAddr,
			6) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

VOID
MgntInitSsidsToScan(
	PADAPTER					Adapter
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	PRT_SSIDS_TO_SCAN	pSsidsToScan = &(pMgntInfo->SsidsToScan);
	u1Byte i;

	PlatformZeroMemory(pSsidsToScan->SsidBuf, MAX_SSID_TO_SCAN*MAX_SSID_LEN);
	pSsidsToScan->NumSsid = 0;
	for (i=0; i<MAX_SSID_TO_SCAN; i++)
	{
		pSsidsToScan->Ssid[i].Octet = pSsidsToScan->SsidBuf[i];
		pSsidsToScan->Ssid[i].Length = 0;
	}
}

BOOLEAN
MgntAddSsidsToScan(
	PADAPTER					Adapter,
	OCTET_STRING				Ssid
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	PRT_SSIDS_TO_SCAN	pSsidsToScan = &(pMgntInfo->SsidsToScan);
	u1Byte i;

	if(Ssid.Length == 0)
	{
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("MgntAddSsidsToScan SSid length zero!\n"));
		return FALSE;
	}
	
	// Check if the SSID is already in.
	for (i=0; i<pSsidsToScan->NumSsid; i++)
	{
		if ( CompareSSID(pSsidsToScan->Ssid[i].Octet, pSsidsToScan->Ssid[i].Length,
						Ssid.Octet, Ssid.Length) == TRUE)
		{
			return TRUE;
		}
	}
	
	if (pSsidsToScan->NumSsid < MAX_SSID_TO_SCAN)
	{
		CopySsidOS(pSsidsToScan->Ssid[pSsidsToScan->NumSsid], Ssid);
		RT_PRINT_STR(COMP_MLME, DBG_LOUD, "===> MgntAddSsidsToScan(): dest", pSsidsToScan->Ssid[pSsidsToScan->NumSsid].Octet, pSsidsToScan->Ssid[pSsidsToScan->NumSsid].Length);
		
		pSsidsToScan->NumSsid++;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOLEAN
MgntRemoveSsidsToScan(
	PADAPTER					Adapter,
	OCTET_STRING				Ssid
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	PRT_SSIDS_TO_SCAN	pSsidsToScan = &(pMgntInfo->SsidsToScan);
	u1Byte i, j;
	
	for (i=0; i<pSsidsToScan->NumSsid; i++)
	{
		if ( CompareSSID(pSsidsToScan->Ssid[i].Octet, pSsidsToScan->Ssid[i].Length,
						Ssid.Octet, Ssid.Length) == TRUE)
		{
			for (j=i; j<pSsidsToScan->NumSsid-1; j++)
			{
				CopySsidOS(pSsidsToScan->Ssid[j], pSsidsToScan->Ssid[j+1]);
			}
			pSsidsToScan->NumSsid--;
			return TRUE;
		}
	}	

	return FALSE;
}

VOID
MgntClearSsidsToScan(
	PADAPTER					Adapter
	)
{
	MgntInitSsidsToScan(Adapter);
}

// The timer delay is based on the windows ndis thread scheduled delay time.
// By Bruce, 2010-12-27.
#define		TBTT_POLLING_TIMER_DELAY_US	(16 * sTU)	// ms
//
//	Description:
//		Poll MAC's TSF register to simulate TBTT notification.
//
//	Assumption:
//		1. Timer may be fired a little eariler or later than expected. 
//		2. Overhead of Timer and wokritem is less than one Beacon Period.
//
//	070124, by rcnjko.
//
VOID
TbttPollingWorkItemCallback(
	IN PVOID		pContext
	)
{
	PADAPTER			pAdapter = (PADAPTER)pContext;
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	u8Byte				Now;
	u4Byte				TimeToSleepInUs = 0;
	u4Byte				BcnPeriodUs = (pMgntInfo->dot11BeaconPeriod * sTU);
	RT_RF_POWER_STATE 	rfState;
	u4Byte				timeSlotUs;
	u1Byte				SwBeaconType = BEACON_SEND_AUTO_SW;
	pu1Byte 			pBeaconHeader = pMgntInfo->beaconframe.Octet;
	u1Byte				BeaconQueueID = BEACON_QUEUE;
	u1Byte				nMultiplier = 0; // For IBSS and BEACON_SEND_MANUAL
	BOOLEAN				PreGroupTimSet = FALSE; // Determine if the group traffic bit is set in the TIM info of the previous beacon.

	RT_TRACE(COMP_BEACON, DBG_TRACE, ("TbttPollingWorkItemCallback()====>\n"));

	if(RT_DRIVER_STOP(pAdapter))
	{
		RT_TRACE(COMP_BEACON, DBG_WARNING, 
			("TbttPollingWorkItemCallback(): bDriverStopped = %d, bSurpriseRemoved = %d, bDriverIsGoingToUnload = %d; return this workitem!!!\n", 
			pAdapter->bDriverStopped, pAdapter->bSurpriseRemoved, pAdapter->bDriverIsGoingToUnload));
		return;	
	}
	

	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));
	
	if(rfState != eRfOn)
	{
		RT_TRACE(COMP_BEACON, DBG_WARNING, ("rfState = %d; return this workitem!!!\n", rfState));
		return;
	}

	if(GetDefaultAdapter(pAdapter)->MgntInfo.bStartApDueToWakeup)
	{
		RT_TRACE(COMP_BEACON, DBG_WARNING, ("Tbtt bStartApDueToWakeup ==TRUE, return\n"));
		return;
	}



	// The Beacon state has been marked  "stopped".
	if(pMgntInfo->BeaconState == BEACON_STOP)
	{
		RT_TRACE(COMP_BEACON, DBG_WARNING, ("BeaconState = BEACON_STOP; return this workitem!!!\n"));
		return;	
	}

	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_HW_BEACON_SUPPORT, (pu1Byte)&SwBeaconType);
	// Set the default queue id for beacon.
	BeaconQueueID = (SwBeaconType <= BEACON_SEND_AUTO_SW) ? BEACON_QUEUE : NORMAL_QUEUE;
	// The beacon queue may be different by the different ICs or buses.
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_BEACON_QUEUE, (pu1Byte)&BeaconQueueID);

	if(pMgntInfo->mIbss && SwBeaconType == BEACON_SEND_MANUAL && pMgntInfo->bMediaConnect)
	{ // Randomize beacon interval if we are not the only one of the IBSS, 2005.08.23, by rcnjko.
		// To fair the Beacon distribution in IBSS, and prevent too many Beacons in the air.
		// Only apply to the manual sending beacon beacuse in other cases the HW can fair the Beacon.
		nMultiplier = (u1Byte)GetRandomNumber(0, 2);
	}
	
	//
	// Poll TSFR to determine the time to send S/W Beacon via ep2.
	//
	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_TSF_TIMER, (pu1Byte)&Now);

	// 
	// The Beacon sending by SW timer is very complex, so we seperate the beacon intrval into
	// several parts A-B, B-C, and C-D as the following.
	//   TBTT                                                      Next TBTT
	//    || <------------------ Beacon Period --------------------- > ||
	//    A              B                              C              D
	//    || <- 16 ms -> | <- Beacon Intval - (16*2) -> | <- 16 ms ->  ||
	//    || Send Mcast  | Do Nothing                   | Update Becon ||	
	//    || --------------------------------------------------------  ||
	// AP mode
	//	a. The HW can send the Beacon in TBTT automatically (BEACON_SEND_AUTO_SW)
	//     (1) A-B: Check if it can send multicast packets now.
	//     (2) C-D: Prepare the beacon content and send it to the beacon queue.
	//   b. The SW should check the timing to send the Beacon packet by sw timer (BEACON_SEND_MANUAL)
	//     (1) A-B: Prepare the Beacon content and send it to the specificed queue.
	//     (2) A-B: After sending the Beacon, check if it can send multicasts now into the queue where the beacon sent into.
	// IBSS mode -
	//	a. The HW can send the Beacon in TBTT automatically (BEACON_SEND_AUTO_SW)
	//     (1) C-D: Prepare the beacon content and send it to the beacon queue.
	//   b. The SW should check the timing to send the Beacon packet by sw timer (BEACON_SEND_MANUAL)
	//     (1) A-B: Prepare the Beacon content and send it to the specificied queue.
	// Note:
	//	The reason why we use 16 ms is the max delay time when the workitem is woke up in Ndis.
	//	We use sTU(1024us) for each partition buffer.
	// By Bruce, 2010-12-27.
	//

	// Position where the current time is at which slot of beacon period.
	timeSlotUs = (u4Byte)PlatformModular64(Now, (u8Byte)BcnPeriodUs);

	//RT_TRACE(COMP_AP, DBG_TRACE, ("TbttPollingWorkItemCallback(): Now: %"i64fmt"d >= timeSlot: %d\n", Now, timeSlotUs));

	//3 // 1st case, A-B
	if(timeSlotUs < (TBTT_POLLING_TIMER_DELAY_US + sTU))
	{
		if(SwBeaconType == BEACON_SEND_AUTO_SW)
		{
			// AP mode
			if(ACTING_AS_AP(pAdapter))
			{ // We need to check the multicast packets.
				AP_PS_SendAllMcastPkts(pAdapter, NORMAL_QUEUE);
			}
			// Wake up at point C before TBTT to update Beacon buffer
			TimeToSleepInUs = (u4Byte)(BcnPeriodUs - TBTT_POLLING_TIMER_DELAY_US - timeSlotUs);
		}		
		else if(SwBeaconType == BEACON_SEND_MANUAL)		
		{			
			// We need to update Beacon content and send the Beacon
			ConstructBeaconFrame(pAdapter);

			
			// Set Timestamp according to current value of our TSFR.
			// Beacuse we shall send this Beacon by ourself in normal queue and the HW won't fill the 
			// timestamp for this Beacon, we need to fill the timestamp by ourself.
			SET_BEACON_PROBE_RSP_TIME_STAMP_LOW(pBeaconHeader, ((u4Byte)(Now & 0xFFFFFFFF)));
			SET_BEACON_PROBE_RSP_TIME_STAMP_HIGH(pBeaconHeader, ((u4Byte)(Now >> 32)));

			// Note: In this case, this beacon packet may not be sent by HW Beacon Queue but any other normal queue.
			SendBeaconFrame(pAdapter, BeaconQueueID);			

			if(pMgntInfo->bDisSwDmaBcn)		
			{
				pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_DIS_SW_BCN, (pu1Byte)(&pAdapter));
				pMgntInfo->bDisSwDmaBcn = FALSE;
			}
			
			if(ACTING_AS_AP(pAdapter) && (pMgntInfo->mDtimCount == 0))
			{
				// Note: The Beacon Queue ID shall not be the HW Beacon Queue, or they shall be lost.
				// Why we send the multicast packets by this queue beacause we want these packets
				// are sent after the previous Beacon. By Bruce, 2011-01-19.
				AP_PS_SendAllMcastPkts(pAdapter, BeaconQueueID);
			}
			// Wake up at point D at TBTT to update/send the next Beacon
			TimeToSleepInUs = (u4Byte)(BcnPeriodUs - timeSlotUs + (nMultiplier * BcnPeriodUs));
		}
		else
		{			
			RT_TRACE(COMP_AP, DBG_WARNING, ("TbttPollingWorkItemCallback(): Wrong SwBeaconType(%d)! Return!\n", SwBeaconType));
			return;
		}
	}
	//3 // 2nd Case C-D: Prepare beacon content and send bacon
	else if(timeSlotUs >= (BcnPeriodUs - TBTT_POLLING_TIMER_DELAY_US - sTU))
	{
		if(SwBeaconType == BEACON_SEND_AUTO_SW)		
		{
			ConstructBeaconFrame(pAdapter);
			// Note: We don't need to update timestamp because the HW will fill that.

			SendBeaconFrame(pAdapter, BeaconQueueID);			

			if(pMgntInfo->bDisSwDmaBcn)		
			{
				pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_DIS_SW_BCN, (pu1Byte)(&pAdapter));
				pMgntInfo->bDisSwDmaBcn = FALSE;
			}

			PreGroupTimSet = (BOOLEAN)pMgntInfo->TimBuf[2];
			
			if(ACTING_AS_AP(pAdapter) && PreGroupTimSet) // If the Group Tim bit in AP mode was set, we need to send the multicast packets after TBTT.
			{
				// Wake up at point D if we have group traffic to send
				TimeToSleepInUs = (u4Byte)(BcnPeriodUs - timeSlotUs);
			}
			else
			{
				// Wake up at next point C before next TBTT.
				TimeToSleepInUs = (u4Byte)((BcnPeriodUs - timeSlotUs) + (BcnPeriodUs - TBTT_POLLING_TIMER_DELAY_US));
			}
		} 
		else if(SwBeaconType == BEACON_SEND_MANUAL)
		{
			// Wake up at point D at TBTT to update/send the next Beacon
			TimeToSleepInUs = (u4Byte)(BcnPeriodUs - timeSlotUs + (nMultiplier * BcnPeriodUs));
		}
		else
		{
			RT_TRACE(COMP_AP, DBG_WARNING, ("TbttPollingWorkItemCallback(): Wrong SwBeaconType(%d)! Return!\n", SwBeaconType));
			return;
		}		
	}
	//3 // 3rd case B-C: Do Nothing and reschedule the next timer.
	else
	{
		if(SwBeaconType == BEACON_SEND_AUTO_SW)
		{
			// Wake up at point C
			TimeToSleepInUs = (u4Byte)(BcnPeriodUs - TBTT_POLLING_TIMER_DELAY_US - timeSlotUs);
		}
		else if(SwBeaconType == BEACON_SEND_MANUAL)
		{
			// Wake up at point D at TBTT to update/send the next Beacon
			TimeToSleepInUs = (u4Byte)(BcnPeriodUs - timeSlotUs + (nMultiplier * BcnPeriodUs));
		}
		else
		{
			RT_TRACE(COMP_AP, DBG_WARNING, ("TbttPollingWorkItemCallback(): Wrong SwBeaconType(%d)! Return!\n", SwBeaconType));
			return;
		}							
	}

	if(!(RT_DRIVER_HALT(pAdapter)))
	{
		PlatformSetTimer(
			pAdapter, 
			&(pMgntInfo->SwBeaconTimer), 
			(TimeToSleepInUs/1000));

		RT_TRACE(COMP_BEACON, DBG_LOUD, ("TbttPollingWorkItemCallback(): SetTimer(%d ms)! Return!\n", TimeToSleepInUs/sTU));
	}
}

//
//	Description:
//		Enable network monitor mode, all rx packets will be received.
//	2007.08.06, by shien chang.
//
VOID
MgntEnableNetMonitorMode(
	PADAPTER					Adapter,
	BOOLEAN						bInitState
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

	pMgntInfo->bNetMonitorMode = TRUE;

	if(Adapter->bHWInitReady)
		Adapter->HalFunc.AllowAllDestAddrHandler(Adapter, TRUE, !bInitState);

	IPSDisable(Adapter,FALSE, IPS_DISABLE_PROXIMITY);

	PlatformEnableNetworkMonitorMode(Adapter);
}


//
//	Description:
//		Disable network network monitor mode, only packets destinated to
//		us will be received.
//	2007.08.06, by shien chang.
//
VOID
MgntDisableNetMonitorMode(
	PADAPTER					Adapter,
	BOOLEAN						bInitState
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

	pMgntInfo->bNetMonitorMode = FALSE;

	if(Adapter->bHWInitReady)
		Adapter->HalFunc.AllowAllDestAddrHandler(Adapter, FALSE, !bInitState);
	
	IPSReturn(Adapter, IPS_DISABLE_PROXIMITY);
	PlatformDisableNetworkMonitorMode(Adapter);
}


//
//	Description:
//		Allocate hash tables.
//	By Bruce, 2008-02-25.
//
BOOLEAN 
MgntAllocHashTables(
	IN	PADAPTER		pAdapter
	)
{
	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);	
	PSTA_QOS				pStaQos = pMgntInfo->pStaQos;
	RT_HASH_TABLE_HANDLE	hTmp;


	// Qos TStream
	hTmp = RtAllocateHashTable( 
			pAdapter, 
			MAX_AP_TS_COUNT, // Capacity
			sizeof(QOS_TSTREAM), // ValueSize
			QOS_TSTREAM_KEY_SIZE, // KeySize
			QosTsHash); // pfHash
	pStaQos->hApTsTable = hTmp; 
	if(hTmp == NULL)
		return FALSE;

	if(hTmp == NULL)
		return FALSE;
	return TRUE;
}

//
//	Description:
//		Free hash tables allocated in MgntAllocHashTables().
//
VOID 
MgntFreeHashTables(
	IN	PADAPTER		pAdapter
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);	
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;

	if(NULL != pStaQos)
		RtFreeHashTable(pStaQos->hApTsTable);
}


u1Byte
MgntQuery_TxRateExcludeCCKRates(
	IN	OCTET_STRING	BSSBasicRateSet)
{
	u2Byte	i;
	u1Byte	QueryRate = 0;
	u1Byte	BasicRate;

	//
	// Find the lowest rate in the BSSBasicRateSet except CCK rates.
	//
	
	for( i = 0; i < BSSBasicRateSet.Length; i++)
	{
		BasicRate = BSSBasicRateSet.Octet[i] & 0x7F; 
		RT_DISP(FDM, WA_IOT, ("MgntQuery_TxRateExcludeCCKRates(), BasicRate = 0x%x\n", BasicRate));
		if(!IS_CCK_RATE(BasicRate))
		{
			if(QueryRate == 0)
			{
				QueryRate = BasicRate;
				RT_DISP(FDM, WA_IOT, ("Find first BasicRate = 0x%x\n", QueryRate));
			}
			else
			{
				if(BasicRate < QueryRate)
				{
					QueryRate = BasicRate;
					RT_DISP(FDM, WA_IOT, ("Find small BasicRate = 0x%x\n", QueryRate));
				}
			}
		}		
	}

	if(QueryRate == 0)
	{
		QueryRate = MGN_6M;	// 6M
		RT_DISP(FDM, WA_IOT, ("No BasicRate found!!\n"));
	}
	RT_DISP(FDM, WA_IOT, ("Final QueryRate = 0x%x\n", QueryRate));
	return QueryRate;
}

//
//	Description:
//		Allocate packet parser for advanced analyze.
//
BOOLEAN
MgntAllocatePacketParser(
	IN	PADAPTER					Adapter
	)
{
	BOOLEAN 	bReturn;

	bReturn = GPAllocateParser(Adapter,
							GP_VOWLAN_SIP,
							"CCX4_CAC SIP parser",
							GPAllocateParserVoWlanSIP,
							GPParserGateVoWlanSIP,
							GPParserActionVoWlanSIP,
							TRUE);
	
	return bReturn;
}

//
//	Description:
//		Free packet parser allocated at MgntAllocatePacketParser().
//
VOID
MgntFreePacketParser(
	IN	PADAPTER		Adapter
	)
{
	GPFreeParser(Adapter,
				GP_VOWLAN_SIP,
				GPFreeParserVoWlanSIP);
}

//
//	Description:
// 		Check this packet if Power Management bit is invoked.
//	By Bruce, 2007-10-26.
//
BOOLEAN
MgntGetPwrMgntInfo(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	BOOLEAN				bIncHead
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	pu1Byte			pheader = GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb);


	// ========================================================
	// Set PwrMgnt bit according to the following condition (arrange by priority):
	// (1) Not associate to AP: False 
	// (2) IBSS or AP mode: False (We do not enter PS in IBSS now)
	// (3) PS-Poll frame: True
	// (4) Management Frame: False
	// (5) Control frame: False
	// (6) NULL Data: According to the original packet setting
	// (7) Security Handshake process
	// (8) WMM Test Plan(dot11PowerSaveMode & APSD) Data frame: True
	// (9) TDLS PU Sleep STA (same as WMM-PS): True
	// (10) Normal data frame, don't care PS mode: False
	// By Bruce, 2007-11-09.
	// =======================================================

	// (1) Not associate to AP 
	// (2) IBSS or AP mode
	if(!pMgntInfo->mAssoc || pMgntInfo->mIbss ||  ACTING_AS_AP(Adapter))
		return FALSE;
	
	// (3) PS-Poll: True
	if(IsCtrlPSpoll(pheader))
		return TRUE;

	// (4) Management Frame: False
	// (5) Control frame: False
	if( IsMgntFrame(pheader) || IsCtrlFrame(pheader))
		return FALSE;

	// (6)NULL Data: According to the TCB setting
	if(IsMgntNullFrame(pheader) || IsMgntQosNull(pheader))
	{
		return (GET_80211_HDR_PWR_MGNT(pheader))? TRUE : FALSE;
	}
	
	// (7) Security Handshake process
	if(pMgntInfo->SecurityInfo.SecLvl > RT_SEC_LVL_NONE && !SecIsTxKeyInstalled(Adapter, pMgntInfo->Bssid))
		return FALSE;

	// FW control LPS.
	// Always return false becuase Fw will fill this bit by itself during PS mode after 8723A.
	if(IS_FW_POWER_SAVE(pMgntInfo))
		return FALSE;
	
	// (8) WMM Test Plan(dot11PowerSaveMode & APSD) Data frame: True
	if(IS_WIFI_POWER_SAVE(pMgntInfo) )
	{
		return TRUE;
	}

	// (9) TDLS PU Sleep STA
	if(TDLS_GetPwrMgntInfo(Adapter, pTcb))
	{
		pTcb->bSleepPkt = TRUE;
			return TRUE;
	}
	
	// (10) Normal data frame, don't care PS mode: False
	return FALSE;
}


VOID
LedTimerCallback(
	PRT_TIMER		pTimer
	)
{
	PADAPTER	Adapter=(PADAPTER)pTimer->Adapter;
	PMGNT_INFO  pMgntInfo = &Adapter->MgntInfo;
	u2Byte		bLedTxCntBack,bLedRxBack;

	if (pMgntInfo->mAssoc != TRUE && pMgntInfo->mIbss != TRUE)
	{
		return;
	}

	bLedTxCntBack = Adapter->LedTxCnt ;
	if (Adapter->LedTxCnt > 0)
	{
		Adapter->HalFunc.LedControlHandler( Adapter, LED_CTL_TX );
		Adapter->LedTxCnt = 0;
	}

	bLedRxBack = Adapter->LedRxCnt;
	if (Adapter->LedRxCnt > 0)
	{
		Adapter->HalFunc.LedControlHandler( Adapter, LED_CTL_RX );
		Adapter->LedRxCnt = 0;
	}

}


RT_STATUS
ChnlCommAllocateMemory(
	PADAPTER pDefaultAdapter
)
{
	RT_STATUS rtStatus = RT_STATUS_SUCCESS;

	rtStatus = PlatformAllocateMemory(pDefaultAdapter, (void **)&pDefaultAdapter->pPortCommonInfo->pChnlCommInfo, sizeof(CHANNEL_COMMON_CONTEXT));
	if(rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));

		return RT_STATUS_FAILURE;
	}

	PlatformZeroMemory(pDefaultAdapter->pPortCommonInfo->pChnlCommInfo, sizeof(CHANNEL_COMMON_CONTEXT));
	// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
	RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: Allocate BatchIndicationInfo Memory: Size: %d\n", __FUNCTION__, sizeof(CHANNEL_COMMON_CONTEXT)));

	return RT_STATUS_SUCCESS;
}


VOID
ChnlCommFreeMemory(
	PADAPTER pDefaultAdapter
)
{
	// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
	RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: Free pChnlCommInfo Memory: Size: %d\n", __FUNCTION__, sizeof(CHANNEL_COMMON_CONTEXT)));
	PlatformFreeMemory(pDefaultAdapter->pPortCommonInfo->pChnlCommInfo, sizeof(CHANNEL_COMMON_CONTEXT));
}


// Let each port (Adapter) have common context
RT_STATUS
PortCommonInfoAllocateMemoryWithCriticalInitialization(
	PADAPTER pDefaultAdapter
)
{
	RT_STATUS rtStatus = RT_STATUS_SUCCESS;

	rtStatus = PlatformAllocateMemory(pDefaultAdapter, (void **)&pDefaultAdapter->pPortCommonInfo, sizeof(PORT_COMMON_INFO));
	if(rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));

		return RT_STATUS_FAILURE;
	}
	else
	{
		PlatformZeroMemory(pDefaultAdapter->pPortCommonInfo, sizeof(PORT_COMMON_INFO));

		pDefaultAdapter->pPortCommonInfo->pDefaultAdapter = pDefaultAdapter;
		
		// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: Allocate PortCommonInfo Memory: Size: %d\n", __FUNCTION__, sizeof(PORT_COMMON_INFO)));
	}
	
	rtStatus = ChnlCommAllocateMemory(pDefaultAdapter);
	if(rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		PortCommonInfoFreeMemory(pDefaultAdapter);

		return RT_STATUS_FAILURE;
	}	

	// Initialize MultiPort Feature -------------
	MultiPortInitializeContext(pDefaultAdapter);
	// -----------------------------------


	return RT_STATUS_SUCCESS;	

}

VOID
PortCommonInfoFreeMemory(
	PADAPTER pDefaultAdapter
)
{
	// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
	RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: Free PortCommonInfo Memory: Size: %d\n", __FUNCTION__, sizeof(PORT_COMMON_INFO)));

	ChnlCommFreeMemory(pDefaultAdapter);
	PlatformFreeMemory(pDefaultAdapter->pPortCommonInfo, sizeof(PORT_COMMON_INFO));
}

//
// Description: Check whether WoWLAN capability is enabled.
//			The function only returns that whether we need to indicate WoWLAN capabilities to NDIS.
//
// Return value: TRUE, FALSE
//
BOOLEAN
MgntIsWoWLANCapabilityEnable(
	PADAPTER 	pAdapter
)
{	
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	BOOLEAN			bResult;
	
	//
	// 2009.09.02. Modified by tynli. 
	// Add condition "Adapter->bUnloadDriverwhenS3S4" and "pPSC->WoWLANMode".
	//
	if(pAdapter->bUnloadDriverwhenS3S4)
	{	// If driver is unload when S3/S4, it cannot support WoWLAN.
		bResult = FALSE;
	}
	else
	{
		if(pPSC->WoWLANMode) // Registry WoWLANMode=1~3
			bResult = TRUE;
		else //WoWLANMode=0
			bResult = FALSE;
	}
	
//	RT_TRACE(COMP_INIT, DBG_TRACE, ("MgntIsWoWLANCapabilityEnable(): bUnloadDriverwhenS3S4 = %d, WoWLANMode = %d\n",
//		pAdapter->bUnloadDriverwhenS3S4, pPSC->WoWLANMode));

	return bResult;
}

//
// Description: The function returns the result that if we allow the device to wake up host by wake event.
//			  Enable/disable WoWLAN Hw related actions will be controlled by the function
//
BOOLEAN
MgntIsSupportRemoteWakeUp(
	PADAPTER 	pAdapter
)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	BOOLEAN			bResult;
	
	if(pPSC->bFakeWoWLAN || !(MgntIsWoWLANCapabilityEnable(pAdapter)) || !pPSC->bSupportWakeUp)
		bResult = FALSE;
	else
		bResult = TRUE;

	return bResult;
}

#if(AUTO_CHNL_SEL_NHM == 1)
//
//	Description: 
//		NHM measurement for Auto channel selection mechanism.
//
//	2014.05.15, created by Roger.
//
VOID
MgntAutoChnlSelWorkitemCallback(
	IN	PVOID	Context
	)
{

	PADAPTER		Adapter = (PADAPTER)Context;	
	PMGNT_INFO		pMgntInfo = &(GetDefaultAdapter(Adapter)->MgntInfo);
	u1Byte			AutoChnlSelOp = 1;

	if(!IS_AUTO_CHNL_SUPPORT(Adapter)){
		RT_TRACE(COMP_P2P, DBG_TRACE, ("[ACS] MgntAutoChnlSelWorkitemCallback(): Do not support ACS function, so return!!\n"));	
		return;
	}

	//
	// To check another sync control flag which has already been assigned, 2014.07.31.
	//
	while(TRUE){
		
		PlatformAcquireSpinLock(Adapter, RT_ACS_SPINLOCK);
		if(!IS_AUTO_CHNL_IN_PROGRESS(Adapter)){
			RT_TRACE(COMP_P2P, DBG_WARNING, ("[ACS] MgntAutoChnlSelWorkitemCallback(): Waiting for the control flag which has already been assigned in another thread!!\n"));
			PlatformReleaseSpinLock(Adapter, RT_ACS_SPINLOCK);
			delay_ms(1);
		}
		else{
			PlatformReleaseSpinLock(Adapter, RT_ACS_SPINLOCK);
			break;
		}
	}
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("---> [ACS] MgntAutoChnlSelWorkitemCallback(): OperationChnl(%d)\n", RT_GetChannelNumber(GetDefaultAdapter(Adapter))));	

	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AUTO_CHNL_SEL, (pu1Byte)&AutoChnlSelOp);

	SET_AUTO_CHNL_STATE(Adapter, ACS_AFTER_NHM);// Calculation is done	
	SET_AUTO_CHNL_PROGRESS(Adapter, FALSE);
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("<--- [ACS] MgntAutoChnlSelWorkitemCallback(): AutoChnlSelCalInProgress(%d)\n", pMgntInfo->AutoChnlSel.AutoChnlSelCalInProgress));
}
#endif

