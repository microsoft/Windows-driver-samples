#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "RxReorder.tmh"
#endif


BOOLEAN
InsertRxReorderList(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	PRX_TS_RECORD	pTS,
	IN	u2Byte			SeqNum
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PRX_REORDER_ENTRY 	pReorderEntry;
	PRT_LIST_ENTRY	pList = &pTS->RxPendingPktList;

	if(!RTIsListEmpty(&pMgntInfo->RxReorder_Unused_List))
	{
		pReorderEntry = (PRX_REORDER_ENTRY)RTRemoveHeadList(&pMgntInfo->RxReorder_Unused_List);

		// Make a reorder entry and insert into a the packet list.
		pReorderEntry->SeqNum = SeqNum;
		pReorderEntry->pRfd = pRfd;

		while(pList->Blink != &pTS->RxPendingPktList)
		{
			if( SN_LESS(pReorderEntry->SeqNum, ((PRX_REORDER_ENTRY)pList->Blink)->SeqNum) )
			{
				// Not reach correct position yet!!
				pList = pList->Blink;
			}
			else if( SN_EQUAL(pReorderEntry->SeqNum, ((PRX_REORDER_ENTRY)pList->Blink)->SeqNum) )
			{
				// Duplicate entry is found!! Do not insert current entry.
				RT_TRACE(COMP_RX_REORDER, DBG_WARNING, ("InsertRxReorderList(): Duplicate packet is dropped!! IndicateSeq: %d, NewSeq: %d\n", pTS->RxIndicateSeq, SeqNum));

				RTInsertTailList(&pMgntInfo->RxReorder_Unused_List, &pReorderEntry->List);
				return FALSE;
			}
			else
			{
				break;
			}
		}

		pReorderEntry->List.Blink = pList->Blink;
		pReorderEntry->List.Blink->Flink = &pReorderEntry->List;
		pReorderEntry->List.Flink = pList;
		pList->Blink = &pReorderEntry->List;

		RT_TRACE(COMP_RX_REORDER, DBG_TRACE, ("InsertRxReorderList(): Pkt insert into buffer!! IndicateSeq: %d, NewSeq: %d\n", pTS->RxIndicateSeq, SeqNum));
		return TRUE;
	}
	else
	{
		// This part shall be modified!! We can just indicate all the packets in buffer and get reorder entries.
		RT_TRACE(COMP_RX_REORDER, DBG_WARNING, ("InsertRxReorderList(): There is no reorder entry!! Packet is dropped!!\n"));
		return FALSE;
	}
}

VOID
IndicateRxReorderList(
	IN	PADAPTER				Adapter,
	IN	PRX_TS_RECORD			pTS,
	IN	BOOLEAN					bForced
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = pMgntInfo->pHTInfo;
	PRX_REORDER_ENTRY 	pReorderEntry = NULL;
	u2Byte				index = 0;
	BOOLEAN				bPktInBuf = FALSE;
//	PRT_RFD				pRfdIndicateArray[REORDER_WIN_SIZE];
	PRT_RFD				*pRfdIndicateArray;
	PRT_GEN_TEMP_BUFFER 	pGenBuf;

	Adapter->rxReorderIndEnterCnt++;

	if(PlatformAtomicExchange(&Adapter->rxReorderRefCount, TRUE) == TRUE)
	{
		Adapter->rxReorderIndRejectCnt[0]++;
		RT_TRACE(COMP_INIT, DBG_LOUD, ("IndicateRxReorderList(): There is already another thread running by AtomicExchange, happened %d times!!!\n", Adapter->rxReorderIndRejectCnt[0]));
		return;
	}

	// Check if there is any other indication thread running.
	if(pTS->RxIndicateState == RXTS_INDICATE_PROCESSING)
	{
		PlatformAtomicExchange(&Adapter->rxReorderRefCount, FALSE);
		Adapter->rxReorderIndRejectCnt[1]++;
		RT_TRACE(COMP_INIT, DBG_LOUD, ("IndicateRxReorderList(): There is already another thread running by RXTS_INDICATE_PROCESSING, happened %d times!!!\n", Adapter->rxReorderIndRejectCnt[1]));
		return;
	}

	// Handling some condition for forced indicate case.
	if(bForced)
	{
		if(RTIsListEmpty(&pTS->RxPendingPktList))
		{
			PlatformAtomicExchange(&Adapter->rxReorderRefCount, FALSE);
			Adapter->rxReorderIndRejectCnt[2]++;
			RT_TRACE(COMP_INIT, DBG_LOUD, ("IndicateRxReorderList(): There is already another thread running by ListEmpty, happened %d times!!!\n", Adapter->rxReorderIndRejectCnt[2]));
			return;
		}
		else
		{
			pReorderEntry = (PRX_REORDER_ENTRY)RTGetHeadList(&pTS->RxPendingPktList);
			pTS->RxIndicateSeq = pReorderEntry->SeqNum;
		}
	}

	Adapter->rxReorderIndAllowCnt++;

	pGenBuf = GetGenTempBuffer (Adapter, sizeof(PRT_RFD)*REORDER_WIN_SIZE);
	pRfdIndicateArray = (PRT_RFD *)pGenBuf->Buffer.Ptr;

	// Prepare indication list and indication.
	do{
		// Check if there is any packet need indicate.
		while(!RTIsListEmpty(&pTS->RxPendingPktList))
		{
			pReorderEntry = (PRX_REORDER_ENTRY)RTGetHeadList(&pTS->RxPendingPktList);

			if(!SN_LESS(pTS->RxIndicateSeq, pReorderEntry->SeqNum))
			{
				// This protect buffer from overflow.
				if(index >= REORDER_WIN_SIZE)
				{
					RT_ASSERT(FALSE, ("IndicateRxReorderList(): Buffer overflow!! \n"));
					bPktInBuf = TRUE;
					break;
				}
				if(index > 0)
				{
					if(PlatformCompareMemory(pReorderEntry->pRfd->Address3,pRfdIndicateArray[index-1]->Address3,6) != 0)		
					{
						bPktInBuf = TRUE;
						break;
					}
				}
			
				pReorderEntry = (PRX_REORDER_ENTRY)RTRemoveHeadList(&pTS->RxPendingPktList);

				if(SN_EQUAL(pReorderEntry->SeqNum, pTS->RxIndicateSeq))
					pTS->RxIndicateSeq = (pTS->RxIndicateSeq + 1) % 4096;

				RT_TRACE(COMP_RX_REORDER, DBG_LOUD, ("RxReorderIndicatePacket(): Packets indication!! IndicateSeq: %d\n",  pReorderEntry->SeqNum));
				pRfdIndicateArray[index] = pReorderEntry->pRfd;
				index++;
				
				RTInsertTailList(&pMgntInfo->RxReorder_Unused_List, &pReorderEntry->List);
			}
			else
			{
				bPktInBuf = TRUE;
				break;
			}
		}

		// Handling pending timer. Set this timer to prevent from long time Rx buffering.
		if(index>0)
		{
			// Cancel previous pending timer.
			{
				PlatformCancelTimer(Adapter, &pTS->RxPktPendingTimer);

				// Set this as a lock to make sure that only one thread is indicating packet.
				pTS->RxIndicateState = RXTS_INDICATE_PROCESSING;
			}

			// Indicate packets
			RT_ASSERT((index<=REORDER_WIN_SIZE), ("RxReorderIndicatePacket(): Rx Reorder buffer full!! \n"));

			// Packets indication.
			DrvIFIndicatePackets(Adapter, pRfdIndicateArray, index);

			// Update local variables.
			//bPktInBuf = FALSE;
			index = 0;
		}
		else
		{
			break;
		}
	}while(TRUE);

	ReturnGenTempBuffer(Adapter, pGenBuf);

	// Release the indication lock and set to new indication step.
	if(bPktInBuf)
	{
		u1Byte	set_penf_timer=FALSE;
		
		{
			if(pTS->RxIndicateState != RXTS_INDICATE_REORDER)
				set_penf_timer = TRUE;
		}

		if (set_penf_timer == TRUE)
		{
			// Set new pending timer.
			pTS->RxIndicateState = RXTS_INDICATE_REORDER;
			PlatformSetTimer(Adapter, &pTS->RxPktPendingTimer, pHTInfo->RxReorderPendingTime);
		}
	}
	else
	{
		pTS->RxIndicateState = RXTS_INDICATE_IDLE;
	}

	PlatformAtomicExchange(&Adapter->rxReorderRefCount, FALSE);
}


BOOLEAN
ISinWindow(
	IN	u2Byte	NewSeqNum,
	IN	u2Byte	RxIndicateSeq
	)
{
	if( NewSeqNum ==   RxIndicateSeq)
		return TRUE;

	if( NewSeqNum > RxIndicateSeq )
	{
		if( ( NewSeqNum  - RxIndicateSeq ) < REORDER_WIN_SIZE )
			return TRUE;
		else
			return FALSE;
	}
	else 
	{
		if( ( NewSeqNum  + 4096 - RxIndicateSeq ) < REORDER_WIN_SIZE )
			return TRUE;
		else
			return FALSE;
	}
}

BOOLEAN
CheckRxTsIndicateSeq(
	IN	PADAPTER		Adapter,
	IN	PRX_TS_RECORD	pTS,
	IN	u2Byte			NewSeqNum
	)
{
	PRT_HIGH_THROUGHPUT 	pHTInfo = Adapter->MgntInfo.pHTInfo;
	u1Byte					WinSize = pHTInfo->RxReorderWinSize;
	u2Byte					WinEnd = (pTS->RxIndicateSeq + WinSize -1)%4096;

	// Rx Reorder initialize condition.
	if(pTS->RxIndicateSeq == 0xffff)
		pTS->RxIndicateSeq = NewSeqNum;

	// Drop out the packet which SeqNum is smaller than WinStart
	if( SN_LESS(NewSeqNum, pTS->RxIndicateSeq ) )
	{
		RT_TRACE(COMP_RX_REORDER, DBG_WARNING, ("CheckRxTsIndicateSeq(): Packet Drop! IndicateSeq: %d, NewSeq: %d\n", pTS->RxIndicateSeq, NewSeqNum));
		return FALSE;
	}

	//
	//	SeqNum "MUST" in Window Size !!
	//
	if( !ISinWindow(NewSeqNum ,  pTS->RxIndicateSeq ) )
	{
		RT_TRACE(COMP_RX_REORDER, DBG_WARNING, ("CheckRxTsIndicateSeq(): out-off squence !! Packet Drop! IndicateSeq: %d, NewSeq: %d\n", pTS->RxIndicateSeq, NewSeqNum));
		return FALSE;
	}
	
	//
	// Sliding window manipulation. Conditions includes:
	// 1. Incoming SeqNum is equal to WinStart =>Window shift 1
	// 2. Incoming SeqNum is larger than the WinEnd => Window shift N
	//
	if( SN_EQUAL(NewSeqNum, pTS->RxIndicateSeq) )
	{
		pTS->RxIndicateSeq = (pTS->RxIndicateSeq + 1) % 4096;
	}
	else if(SN_LESS(WinEnd, NewSeqNum))
	{
		RT_TRACE(COMP_RX_REORDER, DBG_WARNING, ("CheckRxTsIndicateSeq(): Window Shift! IndicateSeq: %d, NewSeq: %d\n", pTS->RxIndicateSeq, NewSeqNum));
		if(NewSeqNum >= (WinSize - 1))
			pTS->RxIndicateSeq = NewSeqNum + 1 -WinSize;
		else
			pTS->RxIndicateSeq = 4095 - (WinSize - (NewSeqNum +1)) + 1;
	}

	return TRUE;
}

VOID
FlushRxTsPendingPkts(
	IN	PADAPTER 				Adapter,
	IN	PRX_TS_RECORD			pTS
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PRX_REORDER_ENTRY	pRxReorderEntry;
//	PRT_RFD				RfdArray[REORDER_WIN_SIZE];
	PRT_RFD				*RfdArray;
	u2Byte				RfdCnt = 0;
	PRT_GEN_TEMP_BUFFER 	pGenBuf;


	if(RTIsListEmpty(&pTS->RxPendingPktList))
	{
		pTS->RxIndicateSeq = 0xffff;
		pTS->RxIndicateState = RXTS_INDICATE_IDLE;
		return;
	}
	
	pGenBuf = GetGenTempBuffer (Adapter, sizeof(PRT_RFD)*REORDER_WIN_SIZE);
	RfdArray = (PRT_RFD *)pGenBuf->Buffer.Ptr;

	PlatformCancelTimer(Adapter, &pTS->RxPktPendingTimer);
	while(!RTIsListEmpty(&pTS->RxPendingPktList))
	{
		pRxReorderEntry = (PRX_REORDER_ENTRY)RTRemoveHeadList(&pTS->RxPendingPktList);
		RfdArray[RfdCnt] = pRxReorderEntry->pRfd;
		RfdCnt = RfdCnt + 1;
		RTInsertTailList(&pMgntInfo->RxReorder_Unused_List, &pRxReorderEntry->List);
	}
	DrvIFIndicatePackets(Adapter, RfdArray, RfdCnt);

	ReturnGenTempBuffer(Adapter, pGenBuf);
	
	pTS->RxIndicateSeq = 0xffff;
	pTS->RxIndicateState = RXTS_INDICATE_IDLE;
}

VOID
FlushAllRxTsPendingPkts(
	IN	PADAPTER 				Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PRX_TS_RECORD	pTS_Record = (PRX_TS_RECORD) RTGetHeadList(&pMgntInfo->Rx_TS_Admit_List);

	while((&pTS_Record->TsCommonInfo.List) != (&pMgntInfo->Rx_TS_Admit_List))
	{
		FlushRxTsPendingPkts(Adapter, pTS_Record);

		pTS_Record = (PRX_TS_RECORD)RTNextEntryList(&pTS_Record->TsCommonInfo.List);
	}
}

#if RX_AGGREGATION
VOID
RxReorderAggrBatchIndicate(
	IN	PADAPTER				Adapter,
	IN	PRT_RFD					pRfd
	)
{
	PMGNT_INFO 			pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = pMgntInfo->pHTInfo;
	PRX_TS_RECORD		pTS = pRfd->Status.pRxTS;
	u2Byte				SeqNum = pRfd->Status.Seq_Num;
	u1Byte TmpCnt;

	if(!CheckRxTsIndicateSeq(Adapter, pTS, SeqNum))
	{
		pHTInfo->RxReorderDropCounter++;
		ReturnRFDList(Adapter, pRfd);
		return;
	}

	//3 Current packet is going to be inserted into pending list.
	if(!InsertRxReorderList(Adapter, pRfd, pTS, SeqNum))
	{
		ReturnRFDList(Adapter, pRfd);
	}
	else
	{
		TmpCnt = 0;
		while((TmpCnt<pMgntInfo->IndicateTsCnt) && (pMgntInfo->IndicateTsArray[TmpCnt]!=pTS))
			TmpCnt++;

		if(TmpCnt == pMgntInfo->IndicateTsCnt)
		{
			pMgntInfo->IndicateTsArray[pMgntInfo->IndicateTsCnt] = pTS;
			pMgntInfo->IndicateTsCnt++;
		}
	}

	if(pRfd->RxAggrInfo.bIsLastPkt)
	{
		for(TmpCnt=0; TmpCnt<pMgntInfo->IndicateTsCnt; TmpCnt++)
			IndicateRxReorderList(Adapter, pMgntInfo->IndicateTsArray[TmpCnt], FALSE);

		pMgntInfo->IndicateTsCnt = 0;
	}
}

VOID
RxReorderAggrBatchFlushBuf(
	IN	PADAPTER				Adapter
	)
{
	PMGNT_INFO 	pMgntInfo = &Adapter->MgntInfo;
	u1Byte 		TmpCnt;

	for(TmpCnt=0; TmpCnt<pMgntInfo->IndicateTsCnt; TmpCnt++)
		IndicateRxReorderList(Adapter, pMgntInfo->IndicateTsArray[TmpCnt], FALSE);

	pMgntInfo->IndicateTsCnt = 0;
}
#endif


VOID
RxReorderIndicatePacket(
	IN	PADAPTER				Adapter,
	IN	PRT_RFD					pRfd
	)
{
	PMGNT_INFO 			pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = pMgntInfo->pHTInfo;
	PRX_TS_RECORD		pTS = pRfd->Status.pRxTS;
	u2Byte				SeqNum = pRfd->Status.Seq_Num;

	RT_TRACE(COMP_RX_REORDER, DBG_TRACE, ("==>RxReorderIndicatePacket()\n"));

	if(!CheckRxTsIndicateSeq(Adapter, pTS, SeqNum))
	{
		pHTInfo->RxReorderDropCounter++;
		ReturnRFDList(Adapter, pRfd);
		RT_TRACE(COMP_RX_REORDER, DBG_TRACE, ("<==RxReorderIndicatePacket(): Packet Drop!!\n"));
		return;
	}

	// Insert all packet into Reorder Queue to maintain its ordering.
	if(!InsertRxReorderList(Adapter, pRfd, pTS, SeqNum))
		ReturnRFDList(Adapter, pRfd);

	//
	// Indication process.
	// After Packet dropping and Sliding Window shifting as above, we can now just indicate the packets
	// with the SeqNum smaller than latest WinStart and buffer other packets.
	//
	// For Rx Reorder condition:
	// 1. All packets with SeqNum smaller than WinStart => Indicate
	// 2. All packets with SeqNum larger than or equal to WinStart => Buffer it.
	//
	IndicateRxReorderList(Adapter, pTS, FALSE);

	RT_TRACE(COMP_RX_REORDER, DBG_TRACE, ("<==RxReorderIndicatePacket():\n"));
}

VOID
RxPktPendingTimeout(
	PRT_TIMER	pTimer
	)
{
	PADAPTER			Adapter = (PADAPTER)pTimer->Adapter;
	PRX_TS_RECORD 		pRxTS = (PRX_TS_RECORD)pTimer->Context;
	BOOLEAN				bSupportAvoidRxDpcWatchdogVoilation = FALSE;
	
	RT_TRACE(COMP_RX_REORDER, DBG_WARNING, ("==>RxPktPendingTimeout()\n"));
	
	//Only 8814AE set bSupportAvoidRxDpcWatchdogVoilation ture now.
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_AVOID_RX_DPC_WATCHDOG_VIOLATION, (pu1Byte)(&bSupportAvoidRxDpcWatchdogVoilation));
	if(bSupportAvoidRxDpcWatchdogVoilation)
	{
		if(Adapter->IntrInterruptRefCount == TRUE)
		{
			PlatformSetTimer(Adapter, &pRxTS->RxPktPendingTimer, 1);
			return;
		}
		if(PlatformAtomicExchange(&Adapter->RxPktPendingTimeoutRefCount, TRUE) == TRUE)
		{
			PlatformSetTimer(Adapter, &pRxTS->RxPktPendingTimer, 1);
			return;
		}
	}
	
	PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);

	// Only do indication process in Batch and Reorder state in this function.
	if(pRxTS->RxIndicateState==RXTS_INDICATE_BATCH)
		IndicateRxReorderList(Adapter, pRxTS, FALSE);
	else if(pRxTS->RxIndicateState==RXTS_INDICATE_REORDER)
		IndicateRxReorderList(Adapter, pRxTS, TRUE);

	PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

	if(bSupportAvoidRxDpcWatchdogVoilation)
		PlatformAtomicExchange(&Adapter->RxPktPendingTimeoutRefCount, FALSE);

	RT_TRACE(COMP_RX_REORDER, DBG_WARNING, ("<==RxPktPendingTimeout()\n"));

}

