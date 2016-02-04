#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "TSGen.tmh"
#endif

VOID
TsAddBaProcess(
	PRT_TIMER	pTimer
	)
{
	PADAPTER		Adapter = (PADAPTER)pTimer->Adapter;
	PTX_TS_RECORD	pTxTs = (PTX_TS_RECORD)pTimer->Context;

	TsInitAddBA(Adapter, pTxTs, BA_POLICY_IMMEDIATE, FALSE);
	RT_TRACE(COMP_QOS, DBG_LOUD, ("TsAddBaProcess(): ADDBA Req is started!! \n"));
}


VOID
ResetTsCommonInfo(
	PTS_COMMON_INFO	pTsCommonInfo
	)
{
	PlatformZeroMemory(pTsCommonInfo->Addr, 6);
	PlatformZeroMemory(&pTsCommonInfo->TSpec, WMM_TSPEC_BODY_LENGTH);
	PlatformZeroMemory(&pTsCommonInfo->TClass, sizeof(QOS_TCLAS)*TCLAS_NUM);
	pTsCommonInfo->TClasProc = 0;
	pTsCommonInfo->TClasNum = 0;
}

VOID
ResetTxTsEntry(
	PTX_TS_RECORD	pTS
	)
{
	ResetTsCommonInfo(&pTS->TsCommonInfo);
	pTS->TxCurSeq = 0;
	pTS->bAddBaReqInProgress = FALSE;
	pTS->bAddBaReqDelayed = FALSE;
	pTS->bUsingBa = FALSE;
	ResetBaEntry(&pTS->TxAdmittedBARecord); //For BA Originator
	ResetBaEntry(&pTS->TxPendingBARecord);
}

VOID
ResetRxTsEntry(
	PRX_TS_RECORD	pTS
	)
{
	ResetTsCommonInfo(&pTS->TsCommonInfo);
	pTS->RxIndicateSeq = 0xffff; // This indicate the RxIndicateSeq is not used now!!
	pTS->RxIndicateState = 0; // Reset indicate state!!
	ResetBaEntry(&pTS->RxAdmittedBARecord);	  // For BA Recepient

	//Init it for avoid drop first packet. 
	pTS->RxLastFragNum = 0xff;
	pTS->RxLastSeqNum = 0xffff;	
}

VOID
TSInitialize(
	PADAPTER	Adapter
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PTX_TS_RECORD		pTxTS = pMgntInfo->TxTsRecord;
	PRX_TS_RECORD		pRxTS = pMgntInfo->RxTsRecord;
	PRX_REORDER_ENTRY	pRxReorderEntry = pMgntInfo->RxReorderEntry;
	u2Byte				count = 0;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("===>TSInitialize()\r\n"));

	// Initialize Tx TS related info.
	RTInitializeListHead(&pMgntInfo->Tx_TS_Admit_List);
	RTInitializeListHead(&pMgntInfo->Tx_TS_Pending_List);
	RTInitializeListHead(&pMgntInfo->Tx_TS_Unused_List);
	for(count = 0; count < TOTAL_TS_NUM; count++)
	{
		//
		// The timers for the operation of Traffic Stream and Block Ack.
		// DLS related timer will be add here in the future!!
		//
		PlatformInitializeTimer(
			Adapter, 
			&pTxTS->TsAddBaTimer, 
			(RT_TIMER_CALL_BACK)TsAddBaProcess, 
			(PVOID)pTxTS, 
			"TxTsAddBaTimer" );

		PlatformInitializeTimer(
			Adapter,
			&pTxTS->TxPendingBARecord.Timer,
			(RT_TIMER_CALL_BACK)BaSetupTimeOut, 
			(PVOID)pTxTS, 
			"TxPendingBARecordTimer");

		PlatformInitializeTimer(
			Adapter,
			&pTxTS->TxAdmittedBARecord.Timer,
			(RT_TIMER_CALL_BACK)TxBaInactTimeout, 
			(PVOID) pTxTS, 
			"TxAdmittedBARecordTimer" );
		
		ResetTxTsEntry(pTxTS);
		RTInsertTailList(&pMgntInfo->Tx_TS_Unused_List, &pTxTS->TsCommonInfo.List);
		pTxTS++;
	}

	// Initialize Rx TS related info.
	RTInitializeListHead(&pMgntInfo->Rx_TS_Admit_List);
	RTInitializeListHead(&pMgntInfo->Rx_TS_Pending_List);
	RTInitializeListHead(&pMgntInfo->Rx_TS_Unused_List);
	for(count = 0; count < TOTAL_TS_NUM; count++)
	{
		RTInitializeListHead(&pRxTS->RxPendingPktList);

		PlatformInitializeTimer(
			Adapter,
			&pRxTS->RxAdmittedBARecord.Timer,
			(RT_TIMER_CALL_BACK)RxBaInactTimeout, 
			(PVOID)pRxTS, 
			"RxAdmittedBARecordTimer");
	
		PlatformInitializeTimer(
			Adapter,
			&pRxTS->RxPktPendingTimer,
			(RT_TIMER_CALL_BACK)RxPktPendingTimeout, 
			(PVOID)pRxTS, 
			"RxPktPendingTimer");

		ResetRxTsEntry(pRxTS);
		RTInsertTailList(&pMgntInfo->Rx_TS_Unused_List, &pRxTS->TsCommonInfo.List);
		pRxTS++;
	}

	// Initialize unused Rx Reorder List.
	RTInitializeListHead(&pMgntInfo->RxReorder_Unused_List);
	for(count = 0; count < REORDER_ENTRY_NUM; count++)
	{
		RTInsertTailList(&pMgntInfo->RxReorder_Unused_List, &pRxReorderEntry->List);
		pRxReorderEntry++;
	}
}

VOID
AdmitTS(
	PADAPTER			Adapter,
	PTS_COMMON_INFO	pTsCommonInfo,
	u4Byte				InactTime
	)
{
}

VOID
ExtendRxTS(
	PADAPTER			Adapter,
	PRX_TS_RECORD		pRxTS
	)
{
	// TS part needs to be implemented in the future.
}

PRT_LIST_ENTRY
SearchTSList(
	PRT_LIST_ENTRY		pTSList,
	pu1Byte				Addr,
	u1Byte				TID,
	DIRECTION_VALUE		Dir
	)
{
	BOOLEAN				bIsFound = FALSE;
	PTS_COMMON_INFO	pTS_Record = (PTS_COMMON_INFO) RTGetHeadList(pTSList);

	while(&pTS_Record->List != pTSList)
	{
		if( PlatformCompareMemory(pTS_Record->Addr, Addr, 6) == 0 )
		{
			if(GET_TSPEC_BODY_TSINFO_TSID(pTS_Record->TSpec) == TID)
			{
				if(GET_TSPEC_BODY_TSINFO_DIRECTION(pTS_Record->TSpec) == Dir)
				{
					bIsFound = TRUE;
					break;
				}
			}
		}
		pTS_Record = (PTS_COMMON_INFO)RTNextEntryList(&pTS_Record->List);
	}

	if(bIsFound)
		return &pTS_Record->List;
	else
		return NULL;
}


PTS_COMMON_INFO
SearchAdmitTRStream(
	PMGNT_INFO	pMgntInfo,
	pu1Byte		Addr,
	u1Byte		TID,
	TR_SELECT	TxRxSelect //Tx:0, Rx:1
)
{
	//DIRECTION_VALUE 	dir;
	u1Byte 	dir;
	BOOLEAN				search_dir[4] = {0, 0, 0, 0};
	PRT_LIST_ENTRY		psearch_list = NULL;
	PTS_COMMON_INFO		pRet=NULL;
		
	if(pMgntInfo->ApType > 0)
	{
		if(TxRxSelect == TX_DIR)
		{
			search_dir[DIR_DOWN] = TRUE;
			search_dir[DIR_BI_DIR]= TRUE;
		}
		else
		{
			search_dir[DIR_UP] 	= TRUE;
			search_dir[DIR_BI_DIR]= TRUE;
		}
	}
	else if(pMgntInfo->mIbss)
	{
		if(TxRxSelect == TX_DIR)
			search_dir[DIR_UP] 	= TRUE;
		else
			search_dir[DIR_DOWN] = TRUE;
	}
	else
	{
		if(TxRxSelect == TX_DIR)
		{
			search_dir[DIR_UP] 	= TRUE;
			search_dir[DIR_BI_DIR]= TRUE;
			search_dir[DIR_DIRECT]= TRUE;
		}
		else
		{
			search_dir[DIR_DOWN] = TRUE;
			search_dir[DIR_BI_DIR]= TRUE;
			search_dir[DIR_DIRECT]= TRUE;
		}
	}

	if(TxRxSelect == TX_DIR)
		psearch_list = &pMgntInfo->Tx_TS_Admit_List;
	else
		psearch_list = &pMgntInfo->Rx_TS_Admit_List;
	
	//for(dir = DIR_UP; dir <= DIR_BI_DIR; dir++)
	for(dir = 0; dir <= DIR_BI_DIR; dir++)
	{
		if(search_dir[dir] ==FALSE )
			continue;

		pRet = (PTS_COMMON_INFO)SearchTSList(psearch_list, Addr, TID, (DIRECTION_VALUE)dir);

		if(pRet  != NULL)
			break;
	}

	if(pRet  != NULL)
		return pRet ;
	else 
		return NULL;
}

VOID
MakeTSEntry(
	OUT	PTS_COMMON_INFO	pTsCommonInfo,
	IN	pu1Byte				Addr,
	IN	pu1Byte				pTSPEC,
	IN	PQOS_TCLAS			pTCLAS,
	IN	u1Byte				TCLAS_Num,
	IN	u1Byte				TCLAS_Proc
	)
{
	u1Byte	count;

	if(pTsCommonInfo == NULL)
		return;
	
	PlatformMoveMemory(pTsCommonInfo->Addr, Addr, 6);

	if(pTSPEC != NULL)
		PlatformMoveMemory((pu1Byte)(&(pTsCommonInfo->TSpec)), (pu1Byte)pTSPEC, WMM_TSPEC_BODY_LENGTH);

	for(count = 0; count < TCLAS_Num; count++)
		PlatformMoveMemory((pu1Byte)(&(pTsCommonInfo->TClass[count])), (pu1Byte)pTCLAS, sizeof(QOS_TCLAS));

	pTsCommonInfo->TClasProc = TCLAS_Proc;
	pTsCommonInfo->TClasNum = TCLAS_Num;
}


BOOLEAN
GetTs(
	PADAPTER			Adapter,
	PTS_COMMON_INFO	*ppTS,
	pu1Byte				Addr,
	u1Byte				TID,
	TR_SELECT			TxRxSelect,  //Rx:1, Tx:0
	BOOLEAN				bAddNewTs
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	u1Byte			UP=0;

	//
	// We do not build any TS for Broadcast or Multicast stream.
	// So reject these kinds of search here.
	//
	if(MacAddr_isBcst(Addr) || MacAddr_isMulticast(Addr))
	{
		RT_TRACE(COMP_QOS, DBG_LOUD, ("Broad or multicast return\r\n"));
		return FALSE;
	}

	if(pMgntInfo->pStaQos->CurrentQosMode == QOS_DISABLE)
	{//only use one TS	
		UP = 0; 
	} 
	else if(pMgntInfo->pStaQos->CurrentQosMode & QOS_WMM)
	{
		// In WMM case: we use 4 TID only
	
		//
		// <Roger_Notes> We use 7 TIDs to fit differentiated service from IP layer 
		// to meet different implementation of APs.
		// 2008.12.18.
		//
		UP = TID;		
	}

	*ppTS = SearchAdmitTRStream(
			pMgntInfo, 
			Addr,
			UP, 
			TxRxSelect);
	
	if(*ppTS != NULL)
	{
		RT_TRACE(COMP_QOS, DBG_TRACE, ("SearchAdmitTRStream get ppTS\r\n"));
		return TRUE;
	}
	else
	{
		if(bAddNewTs == FALSE)
		{
			RT_TRACE(COMP_QOS, DBG_LOUD, ("bAddNewTs == FALSE\r\n"));
			return FALSE;
		}
		else
		{
			// 
			// Create a new Traffic stream for current Tx/Rx
			// This is for EDCA and WMM to add a new TS.
			// For HCCA or WMMSA, TS cannot be addmit without negotiation.
			//
			u1Byte			TSpec[WMM_TSPEC_BODY_LENGTH]={0};
			
			pu1Byte			pTSInfo= TSpec;
			PRT_LIST_ENTRY	pUnusedList = 
								(TxRxSelect == TX_DIR)?
								(&pMgntInfo->Tx_TS_Unused_List):
								(&pMgntInfo->Rx_TS_Unused_List);
								
			PRT_LIST_ENTRY	pAddmitList = 
								(TxRxSelect == TX_DIR)?
								(&pMgntInfo->Tx_TS_Admit_List):
								(&pMgntInfo->Rx_TS_Admit_List);

			DIRECTION_VALUE		Dir = 
								(ACTING_AS_AP(Adapter))?
								((TxRxSelect==TX_DIR)?DIR_DOWN:DIR_UP):
								((TxRxSelect==TX_DIR)?DIR_UP:DIR_DOWN);
			RT_TRACE(COMP_QOS, DBG_TRACE, ("GetTs(): Add New TS %s Entry.\n", TxRxSelect?"Rx":"Tx"));
			RT_PRINT_ADDR(COMP_QOS, DBG_TRACE, "PeerAddress:", Addr);
			RT_TRACE(COMP_QOS, DBG_TRACE, ("UserPriority: %d\n", UP));
			if(!RTIsListEmpty(pUnusedList))
			{
				(*ppTS) = (PTS_COMMON_INFO)RTRemoveHeadList(pUnusedList);

				if(TxRxSelect==TX_DIR)
					ResetTxTsEntry((PTX_TS_RECORD) *ppTS);
				else
					ResetRxTsEntry((PRX_TS_RECORD) *ppTS);

				// Prepare TS Info releated field
				SET_TSPEC_BODY_TSINFO_TRAFFIC_TYPE(pTSInfo, 0); // Traffic type: WMM is reserved in this field
				SET_TSPEC_BODY_TSINFO_TSID(pTSInfo, UP);	// TSID
				SET_TSPEC_BODY_TSINFO_DIRECTION(pTSInfo, Dir);	// Direction: if there is DirectLink, this need additional consideration.
				SET_TSPEC_BODY_TSINFO_ACCESS_POLICY(pTSInfo, 1); // Access policy
				SET_TSPEC_BODY_TSINFO_AGGREGATION(pTSInfo, 0);	 // Aggregation
				SET_TSPEC_BODY_TSINFO_PSB(pTSInfo, 0);		// Aggregation
				SET_TSPEC_BODY_TSINFO_UP(pTSInfo, UP);		// User priority
				SET_TSPEC_BODY_TSINFO_ACK_POLICY(pTSInfo, 0);	 // Ack policy
				SET_TSPEC_BODY_TSINFO_SCHEDULE(pTSInfo, 0);		 // Schedule
					
				MakeTSEntry(*ppTS, Addr, TSpec, NULL, 0, 0);
				AdmitTS(Adapter, *ppTS, 0);
				RTInsertTailList(pAddmitList, &((*ppTS)->List));

				// if there is DirectLink, we need to do additional operation here!!


				return TRUE;
			}
			else
			{
				RT_ASSERT(FALSE, ("ManageTSInformation(): There is not enough TS record to be used!!"));
				return FALSE;
			}		
		}		
	}

}

VOID
RemoveTsEntry(
	PADAPTER			Adapter,
	PTS_COMMON_INFO		pTs,
	TR_SELECT			TxRxSelect
	)
{
	BOOLEAN 	bInRxProgress = FALSE;

	TsInitDelBA(Adapter, pTs, TxRxSelect);

	RT_TRACE(COMP_QOS, DBG_TRACE, ("RemoveTsEntry(): Remove %s Entry.\n", TxRxSelect?"Rx":"Tx"));
	RT_PRINT_ADDR(COMP_QOS, DBG_TRACE, "PeerAddress:", pTs->Addr);

	if(TxRxSelect == RX_DIR)
	{
		PRX_REORDER_ENTRY	pRxReorderEntry;
		PRX_TS_RECORD 		pRxTS = (PRX_TS_RECORD)pTs;
		PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;

		if( PlatformCancelTimer(Adapter, &pRxTS->RxPktPendingTimer) == FALSE )
		{
			pRxTS->RxIndicateState = RXTS_INDICATE_IDLE;
		}

		if(!GetDefaultAdapter(Adapter)->bRemoveTsInRxPath)
			PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
		else
			bInRxProgress = TRUE;
			
		while(!RTIsListEmpty(&pRxTS->RxPendingPktList))
		{					
			pRxReorderEntry = (PRX_REORDER_ENTRY)RTRemoveHeadList(&pRxTS->RxPendingPktList);
			pRxTS->RxBatchCount--;
			ReturnRFDList(Adapter, pRxReorderEntry->pRfd);
			RTInsertTailList(&pMgntInfo->RxReorder_Unused_List, &pRxReorderEntry->List);
		}

		if(!bInRxProgress)
			PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
	}
	else
	{
		PTX_TS_RECORD pTxTS = (PTX_TS_RECORD)pTs;
		PlatformCancelTimer(Adapter, &pTxTS->TsAddBaTimer);
	}
}

VOID
RemovePeerTS(
	PADAPTER	Adapter,
	pu1Byte		Addr
	)
{
	PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;
	PTS_COMMON_INFO	pTS, pTmpTS;

	pTS = (PTS_COMMON_INFO)RTGetHeadList(&pMgntInfo->Tx_TS_Pending_List);
	while(&pTS->List != &pMgntInfo->Tx_TS_Pending_List)
	{
		if(PlatformCompareMemory(pTS->Addr, Addr, 6) == 0)
		{
			pTmpTS = pTS;
			pTS = (PTS_COMMON_INFO)RTNextEntryList(&pTS->List);
			RTRemoveEntryList(&pTmpTS->List);
			RemoveTsEntry(Adapter, pTmpTS, TX_DIR);
//			RemoveMAEntry(Adapter, &pTmpTS->Ts_MAInfo);
			RTInsertTailList(&pMgntInfo->Tx_TS_Unused_List, &pTmpTS->List);
		}
		else
		{
			pTS = (PTS_COMMON_INFO)RTNextEntryList(&pTS->List);
		}
	}

	pTS = (PTS_COMMON_INFO)RTGetHeadList(&pMgntInfo->Tx_TS_Admit_List);
	while(&pTS->List != &pMgntInfo->Tx_TS_Admit_List)
	{
		if(PlatformCompareMemory(pTS->Addr, Addr, 6) == 0)
		{
			pTmpTS = pTS;
			pTS = (PTS_COMMON_INFO)RTNextEntryList(&pTS->List);
			RTRemoveEntryList(&pTmpTS->List);
			RemoveTsEntry(Adapter, pTmpTS, TX_DIR);
//			RemoveMAEntry(Adapter, &pTmpTS->Ts_MAInfo);
			RTInsertTailList(&pMgntInfo->Tx_TS_Unused_List, &pTmpTS->List);
		}
		else
		{
			pTS = (PTS_COMMON_INFO)RTNextEntryList(&pTS->List);
		}
	}

	pTS = (PTS_COMMON_INFO)RTGetHeadList(&pMgntInfo->Rx_TS_Pending_List);
	while(&pTS->List != &pMgntInfo->Rx_TS_Pending_List)
	{
		if(PlatformCompareMemory(pTS->Addr, Addr, 6) == 0)
		{
			pTmpTS = pTS;
			pTS = (PTS_COMMON_INFO)RTNextEntryList(&pTS->List);
			RTRemoveEntryList(&pTmpTS->List);
			RemoveTsEntry(Adapter, pTmpTS, RX_DIR);
//			RemoveMAEntry(Adapter, &pTmpTS->Ts_MAInfo);
			RTInsertTailList(&pMgntInfo->Rx_TS_Unused_List, &pTmpTS->List);
		}
		else
		{
			pTS = (PTS_COMMON_INFO)RTNextEntryList(&pTS->List);
		}
	}

	pTS = (PTS_COMMON_INFO)RTGetHeadList(&pMgntInfo->Rx_TS_Admit_List);
	while(&pTS->List != &pMgntInfo->Rx_TS_Admit_List)
	{
		if(PlatformCompareMemory(pTS->Addr, Addr, 6) == 0)
		{
			pTmpTS = pTS;
			pTS = (PTS_COMMON_INFO)RTNextEntryList(&pTS->List);
			RTRemoveEntryList(&pTmpTS->List);
			RemoveTsEntry(Adapter, pTmpTS, RX_DIR);
//			RemoveMAEntry(Adapter, &pTmpTS->Ts_MAInfo);
			RTInsertTailList(&pMgntInfo->Rx_TS_Unused_List, &pTmpTS->List);
		}
		else
		{
			pTS = (PTS_COMMON_INFO)RTNextEntryList(&pTS->List);
		}
	}		
}

VOID
ReleaseAllTSTimer(
	PADAPTER	Adapter)
{
	PMGNT_INFO 			pMgntInfo = &Adapter->MgntInfo;
	PTX_TS_RECORD		pTxTS = pMgntInfo->TxTsRecord;
	PRX_TS_RECORD		pRxTS = pMgntInfo->RxTsRecord;
	u1Byte					count = 0;
	
	for(count = 0; count < TOTAL_TS_NUM; count++)
		{
			//
			// The timers for the operation of Traffic Stream and Block Ack.
			// DLS related timer will be add here in the future!!
			//
			PlatformReleaseTimer(
				Adapter, 
				&pTxTS->TsAddBaTimer);

			PlatformReleaseTimer(
				Adapter,
				&pTxTS->TxPendingBARecord.Timer);

			PlatformReleaseTimer(
				Adapter,
				&pTxTS->TxAdmittedBARecord.Timer);
		
		pTxTS++;
	}

	for(count = 0; count < TOTAL_TS_NUM; count++)
		{
			PlatformReleaseTimer(
				Adapter,
				&pRxTS->RxAdmittedBARecord.Timer);
		
			PlatformReleaseTimer(
				Adapter,
				&pRxTS->RxPktPendingTimer);
			
			pRxTS++;
		}
}



VOID
RemoveAllTS(
	PADAPTER	Adapter
	)
{
	PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;
	PTS_COMMON_INFO 	pTS;
	
	while(!RTIsListEmpty(&pMgntInfo->Tx_TS_Pending_List))
	{
		pTS = (PTS_COMMON_INFO)RTRemoveHeadList(&pMgntInfo->Tx_TS_Pending_List);
		RemoveTsEntry(Adapter, pTS, TX_DIR);
//		RemoveMAEntry(Adapter, &pTS->Ts_MAInfo);
		RTInsertTailList(&pMgntInfo->Tx_TS_Unused_List, &pTS->List);
	}

	while(!RTIsListEmpty(&pMgntInfo->Tx_TS_Admit_List))
	{
		pTS = (PTS_COMMON_INFO)RTRemoveHeadList(&pMgntInfo->Tx_TS_Admit_List);
		RemoveTsEntry(Adapter, pTS, TX_DIR);
//		RemoveMAEntry(Adapter, &pTS->Ts_MAInfo);
		RTInsertTailList(&pMgntInfo->Tx_TS_Unused_List, &pTS->List);
	}

	while(!RTIsListEmpty(&pMgntInfo->Rx_TS_Pending_List))
	{
		pTS = (PTS_COMMON_INFO)RTRemoveHeadList(&pMgntInfo->Rx_TS_Pending_List);
		RemoveTsEntry(Adapter, pTS, RX_DIR);
//		RemoveMAEntry(Adapter, &pTS->Ts_MAInfo);
		RTInsertTailList(&pMgntInfo->Rx_TS_Unused_List, &pTS->List);
	}

	while(!RTIsListEmpty(&pMgntInfo->Rx_TS_Admit_List))
	{
		pTS = (PTS_COMMON_INFO)RTRemoveHeadList(&pMgntInfo->Rx_TS_Admit_List);
		RemoveTsEntry(Adapter, pTS, RX_DIR);
//		RemoveMAEntry(Adapter, &pTS->Ts_MAInfo);
		RTInsertTailList(&pMgntInfo->Rx_TS_Unused_List, &pTS->List);
	}		
}

VOID
TsStartAddBaProcess(
	PADAPTER		Adapter,
	PTX_TS_RECORD	pTxTS
	)
{
	if(Adapter->MgntInfo.SafeModeEnabled)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("TsStartAddBaProcess ABORT becuase SafeModeEnable\n"));
		return;
	}
	
	if(pTxTS->bAddBaReqInProgress == FALSE)
	{
		pTxTS->bAddBaReqInProgress = TRUE;

		if(pTxTS->bAddBaReqDelayed)
		{
			RT_TRACE(COMP_QOS, DBG_TRACE, ("TsStartAddBaProcess(): Delayed Start ADDBA after 60 sec!!\n"));
			PlatformSetTimer(Adapter, &pTxTS->TsAddBaTimer, TS_ADDBA_DELAY);
		}
		else
		{
			RT_TRACE(COMP_QOS, DBG_TRACE, ("TsStartAddBaProcess(): Immediately Start ADDBA now!!\n"));
			PlatformSetTimer(Adapter, &pTxTS->TsAddBaTimer, 0);
		}
	}
}

VOID
RxTSIndicate(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
	)
{
	OCTET_STRING	frame;
	u1Byte			tid;
	u2Byte			SeqNum;

	frame.Octet = pRfd->Buffer.VirtualAddress;
	frame.Length = pRfd->PacketLength;
	tid = Frame_QoSTID(frame, sMacHdrLng);
	SeqNum = Frame_SeqNum(frame);

	if(GetTs(
		Adapter, 
		(PTS_COMMON_INFO*) &(pRfd->Status.pRxTS),
		Frame_Addr2(frame),
		tid,
		RX_DIR,
		TRUE))
	{
		ExtendRxTS(Adapter, pRfd->Status.pRxTS);

		if(pRfd->Status.bIsAMPDU)
			ExtendBAEntry(Adapter, &(pRfd->Status.pRxTS->RxAdmittedBARecord));

		// Reserve for later usage by RxReorder.
		pRfd->Status.Seq_Num = SeqNum;

		// For 819X series turbo mode!!
		// 2012/09/13 MH This should move to HAL layer.
		// 2012/09/14 MH IN a special case, TPLINK WR741ND Atheros AR9341 AP.
		// When tested in win7 OS, every 45-50s, AP will send a TID=6 packet to
		// NIC for 3 times(6 sec). Then RTWLANU will disable EDCA turbo mode temporarily
		// So the throughput will downgrade. We need to ignore the special case.
		{			
			if(tid!=0 && tid!=3)
			{				
				HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
				if (Adapter->MgntInfo.bWiFiConfg)
				{
					pHalData->bIsAnyNonBEPkts = TRUE;
				}
				else
				{
					//
					// How to Judge,? In busy trafic and then BE packets > VI/VO more than 90%.
					// Then in busy traffic and VI/VO > 1M??
					//
				}
				Adapter->MgntInfo.NumNonBePkt++;
			}
			else
			{
				Adapter->MgntInfo.NumBePkt++;
			}
		}
	}
	else
	{
		pRfd->Status.pRxTS = NULL;

		RT_TRACE(COMP_RECV, DBG_LOUD, ("RxTSIndicate(): No TS!! Skip the check!!\n"));
		RT_ASSERT(FALSE, ("RxTSIndicate(): No TS allocated!!"));
	}
}
