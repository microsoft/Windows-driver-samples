#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "Transmit.tmh"
#endif

#define		RT_PS_TX_STATUS				u4Byte
#define		RT_PS_TX_STATUS_SEND_PKT		0
#define		RT_PS_TX_STATUS_PKT_BUFFERED	1
#define		RT_PS_TX_STATUS_PKT_DROPPED		2


RT_PS_TX_STATUS
VerifyPsOnTx(
	PADAPTER			pAdapter,
	PRT_TCB				pTcb
	)
{
	PMGNT_INFO 			pMgntInfo = &(pAdapter->MgntInfo);
	RT_PS_TX_STATUS		txPsStauts = RT_PS_TX_STATUS_SEND_PKT;
	OCTET_STRING		osMpdu;
	pu1Byte 			pRaddr;

	FillOctetString(osMpdu, GET_FRAME_OF_FIRST_FRAG(pAdapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);

	if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_PS_PRE_TRANSMIT))
	{
		CLEAR_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_PS_PRE_TRANSMIT);
		PreTransmitTCB(pAdapter, pTcb);
		return RT_PS_TX_STATUS_PKT_BUFFERED;
	}

	if(!IsDataFrame(osMpdu.Octet))
		return txPsStauts;

	pRaddr = Frame_pRaddr(osMpdu);

	if(ACTING_AS_AP(pAdapter))
	{
		if( !MacAddr_isMulticast(pRaddr) )
		{ // Unicast.
			PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, pRaddr);	

			// No entry.
			if(pEntry == NULL)
			{
				// Prefast warning C6011 : Dereferencing NULL pointer 'pEntry'.
				//RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "[WARNING] VerifyPsOnTx(): No Entry for client = ", pEntry->MacAddr);
				RT_TRACE(COMP_AP, DBG_LOUD, ("[WARNING] VerifyPsOnTx(): No Entry for client!\n"));
				return txPsStauts;
			}
			
			// Queue the unicast packet if dest STA is in power-save mode.
			if(pEntry->bPowerSave)
			{
				BOOLEAN		bEospPkt = FALSE;
				BOOLEAN		bUAPSD = FALSE;
				if(IsQoSDataFrame(osMpdu.Octet))
				{
					// Check the UP for this packet and the UAPSD info from the assoication.
					switch( GET_QOS_CTRL_WMM_UP(osMpdu.Octet) )
					{
						case 0:
						case 3:
							bUAPSD = GET_BE_UAPSD(pEntry->WmmStaQosInfo);
							break;
						case 1:
						case 2:
							bUAPSD = GET_BK_UAPSD(pEntry->WmmStaQosInfo);
							break;
						case 4:
						case 5:
							bUAPSD = GET_VI_UAPSD(pEntry->WmmStaQosInfo);
							break;
						case 6:
						case 7:
							bUAPSD = GET_VO_UAPSD(pEntry->WmmStaQosInfo);
							break;
					}
				}
				bEospPkt = GET_QOS_CTRL_WMM_EOSP(osMpdu.Octet);

				if(bUAPSD) 
				{
					if(bEospPkt)
					{
						// It's an ESOP packet, we shall send it.
					}
					else if(pEntry->WmmEosp != RT_STA_EOSP_STATE_OPENED || !RTIsListEmpty(&(pEntry->WmmPsQueue)))
					{
						// 
						// If the packets from the upper layer are queued, such as ping, the protocol layer or the socket
						// will block the packets sent until these queued packets are returned. It causes the ping rate with
						// large packet size low and fail in the P2P Test item 6.1.12/6.1.13/7.1.5.
						// To speed up the sending rate of packets, we copy the system packet to local buffer and indicate
						// to os (Ndis) as early.
						// By Bruce, 2010-10-15.
						//
						TCB_CopySystemPacketToLocal(pAdapter, pTcb);
						// The SP is ended or the WmmPsQueue is not empty, the following packets shall be queued.
						RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "VerifyPsOnTx(): Queue ps pkt for wmm client = ", pEntry->MacAddr);
						RTInsertTailList(&(pEntry->WmmPsQueue), &(pTcb->List));
						txPsStauts = RT_PS_TX_STATUS_PKT_BUFFERED;
					}
					else
					{ // We can send packet immediately because the service period is opend.
						BOOLEAN				bSupportTxFeedback = FALSE;

						// Check if this chip supports per tx packet feedback.
						bSupportTxFeedback = TxFeedbackInstallTxFeedbackInfoForTcb(pAdapter, pTcb);
		
						if(bSupportTxFeedback)
						{
							TxFeedbackFillTxFeedbackInfoUserConfiguration(
									pTcb, 
									RT_TX_FEEDBACK_ID_AP_WMM_EOSP_ENDING, 
									pAdapter, 
									Ap_PsTxFeedbackCallback, 
									(PVOID) pEntry
								);
						}

						if(bSupportTxFeedback)
							pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDING;
						else
							pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDED;

						SET_QOS_CTRL_WMM_EOSP(osMpdu.Octet, 1);
						RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "VerifyPsOnTx(): Send packet directly wmm open client = ", pEntry->MacAddr);
					}
				}
				else
				{
					if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_PS_REPLY_PS_POLL))
					{ // This packet is polled by ps-poll, we shoud send it immediately.
						if(!RTIsListEmpty(&(pEntry->PsQueue)))
						{ // There are still packets queued in the queue, set the more data bit.
							SET_80211_HDR_MORE_DATA(osMpdu.Octet, 1);
						}
						CLEAR_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_PS_REPLY_PS_POLL);
					}
					else
					{
						// Legacy PS.
						// RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "VerifyPsOnTx(): Queue ps pkt for legacy client = ", pEntry->MacAddr);
						RTInsertTailList(&(pEntry->PsQueue), &(pTcb->List));
						txPsStauts = RT_PS_TX_STATUS_PKT_BUFFERED;
					}
				}
			}
		}
		else
		{ // Bcst/Mcst.
			if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_PS_MCAST_DTIM))
			{ // DTIM
				if(!RTIsListEmpty(&(pMgntInfo->GroupPsQueue)))
				{ // There are still packets queued in the queue, set the more data bit.
					SET_80211_HDR_MORE_DATA(osMpdu.Octet, 1);
				}
				CLEAR_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_PS_MCAST_DTIM);
			}
			// Queue the Mcst/Bcst packet if one ore more STA is in power-save mode.
			else if(pMgntInfo->PowerSaveStationNum > 0)
			{
				// RT_TRACE_F(COMP_AP, DBG_LOUD, ("Queue Mcst/Bcst pkt\n"));
				RTInsertTailList(&(pMgntInfo->GroupPsQueue), &(pTcb->List));
				txPsStauts = RT_PS_TX_STATUS_PKT_BUFFERED;
			}
		}
	}
	else
	{
		PRT_WLAN_STA	pTDLSPsEntry=TDLS_PS_CheckPsTx(pAdapter, pTcb);

		if(pTDLSPsEntry!=NULL)
		{
			if(TDLS_PS_OnTx(pAdapter, pTDLSPsEntry, pTcb))
				txPsStauts = RT_PS_TX_STATUS_PKT_BUFFERED;

			return txPsStauts;
		}
		
		// Set PwrMgnt bit, by Bruce, 2007-11-09.
		//RT_PRINT_DATA(COMP_MLME, DBG_LOUD, "packet:\n", osMpdu.Octet, osMpdu.Length);
		SET_80211_HDR_PWR_MGNT(osMpdu.Octet, MgntGetPwrMgntInfo(pAdapter, pTcb, TRUE));
		//
		// Move from PreTransmitTCB() becsauce the TCBs in the wait queues sent without passing
		// through PreTransmitTCB(). 
		// Only those packets without pwr mgnt bit set are considered as sent in awake
		// By Bruce, 2008-01-18.
		//
		if(!GET_80211_HDR_PWR_MGNT(osMpdu.Octet))
			pMgntInfo->bAwakePktSent = TRUE;
		if(pMgntInfo->mPss != eAwake)
		{
			do
			{
				// Note, if the NULL frame without pwr mgnt bit set, the packet still should be inserted into the queue.
				if(IsMgntNullFrame(osMpdu.Octet) && GET_80211_HDR_PWR_MGNT(osMpdu.Octet))
					break;
				
				if(IN_LEGACY_POWER_SAVE(pMgntInfo->pStaQos))
					LPS_OnTx(pAdapter, pTcb);
				else
					WMMPS_OnTx(pAdapter, pTcb);
				txPsStauts = RT_PS_TX_STATUS_PKT_BUFFERED;
			}while(FALSE);
		}
		else
		{
			//
			// If we have sent the trigger frame of the current APSD, set up service period to remain awake. By Bruce, 2007-11-15.
			//
			if(!IN_LEGACY_POWER_SAVE(pMgntInfo->pStaQos))
			{
				if(IsStaQosTriggerFrame(&osMpdu, pMgntInfo->pStaQos->Curr4acUapsd))
				{
					pMgntInfo->pStaQos->bInServicePeriod = TRUE;
				}					
			}
		}
	}

	/*
	if(txPsStauts == RT_PS_TX_STATUS_SEND_PKT)
	{
		// Set PwrMgnt bit, by Bruce, 2007-11-09.
		SET_80211_HDR_PWR_MGNT(osMpdu.Octet, MgntGetPwrMgntInfo(pAdapter, pTcb, TRUE));
		
		//
		// Move from PreTransmitTCB() becsauce the TCBs in the wait queues sent without passing
		// through PreTransmitTCB(). 
		// Only those packets without pwr mgnt bit set are considered as sent in awake
		// By Bruce, 2008-01-18.
		//
		if(!GET_80211_HDR_PWR_MGNT(osMpdu.Octet))
			pMgntInfo->bAwakePktSent = TRUE;
	}
	*/

	return txPsStauts;
}




VOID
InitializeTxVariables(
	PADAPTER			Adapter
	)
{
	u2Byte i;
	BOOLEAN bSupportEarlyMode;

	// Initialize each queue with the same number of TxDesc, 
	// except for BEACON_QUEUE. 2005.11.14, by rcnjko.
	for(i = 0; i < MAX_TX_QUEUE;i++)
	{
		Adapter->NumTxDesc[i] = Adapter->RT_TXDESC_NUM;
	}
	// TODO: Emily 2006.11.15. If we are implmemting multiple BSSID feature, more 
	// TODO: than two descriptors may be required.
	Adapter->NumTxDesc[BEACON_QUEUE] = 2;	// BEACON_QUEUE
	Adapter->NumTxDesc[BE_QUEUE] = Adapter->RT_TXDESC_NUM_BE_QUEUE;	// BE queue need more descriptor for performance consideration

	Adapter->NumTcb = Adapter->RT_TCB_NUM;
	Adapter->NumLocalBuffer = Adapter->RT_LOCAL_BUF_NUM;
	Adapter->NumLocalFWBuffer = Adapter->RT_LOCAL_FW_BUF_NUM;


	bSupportEarlyMode = FALSE;
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_EARLY_MODE_SUPPORT, (pu1Byte)(&bSupportEarlyMode));
	if(bSupportEarlyMode)
	{
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_EARLY_MODE_THRESHOLD, &(Adapter->EarlyMode_Threshold));
		for(i=0 ; i<8 ; i++)
			Adapter->EarlyMode_QueueNum[i] = 0;
		
	}
	
	RTInitializeListHead(&Adapter->TcbIdleQueue);
	for(i=0;i<MAX_TX_QUEUE;i++)
	{
		RTInitializeListHead(&Adapter->TcbWaitQueue[i]);
		RTInitializeListHead(&Adapter->TcbBusyQueue[i]);
		RTInitializeListHead(&Adapter->TcbAggrQueue[i]);
		Adapter->TcbCountInAggrQueue[i] = 0;
	}
	RTInitializeListHead(&Adapter->LocalBufferQueue);
	RTInitializeListHead(&Adapter->LocalFWBufferQueue);
	
	ResetTxStatistics(Adapter);
	
}

RT_STATUS
PrepareTCBs(
	PADAPTER			Adapter
	)
{
	u2Byte					i,j;
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER		pLocalBuffer;
	RT_STATUS				status;
	u4Byte					BufferLength,offset;
	BufferLength	= MAX_LLC_LENGTH + 
				MAX_802_11_HEADER_LENGTH*MAX_FRAGMENT_COUNT + 
				MAX_802_11_TRAILER_LENGTH + 
				MAX_FIRMWARE_INFORMATION_SIZE + 
				AMSDU_SUBHEADER_LENGTH;
	do{
		//2 Allocate TCBs
		Adapter->TcbBuffer.Length=Adapter->NumTcb*sizeof(RT_TCB);
		status=PlatformAllocateMemory(Adapter, &Adapter->TcbBuffer.Ptr,Adapter->TcbBuffer.Length);
		if(status!=RT_STATUS_SUCCESS)
			return status;

		PlatformZeroMemory(Adapter->TcbBuffer.Ptr,Adapter->TcbBuffer.Length);

		pTcb=(PRT_TCB)Adapter->TcbBuffer.Ptr;
		for(i=0;i<Adapter->NumTcb;i++)
		{
			status = PlatformAllocateSharedMemory(Adapter, &pTcb[i].Buffer, BufferLength);
			if(status!=RT_STATUS_SUCCESS)
				return status;
			PlatformZeroMemory(pTcb[i].Buffer.VirtualAddress , BufferLength);
		
			GET_SHARED_MEMORY_WITH_OFFSET(
				&pTcb[i].Buffer,
				&pTcb[i].LLC,
				0,
				MAX_LLC_LENGTH);
			offset=MAX_LLC_LENGTH;
			for(j=0;j<MAX_FRAGMENT_COUNT;j++)
			{
				GET_SHARED_MEMORY_WITH_OFFSET(
					&pTcb[i].Buffer,
					&pTcb[i].Header[j],
					offset,
					MAX_802_11_HEADER_LENGTH);
				offset += MAX_802_11_HEADER_LENGTH;
			}
			GET_SHARED_MEMORY_WITH_OFFSET(
				&pTcb[i].Buffer,
				&pTcb[i].Tailer,
				offset,
				MAX_802_11_TRAILER_LENGTH);
			offset+=MAX_802_11_TRAILER_LENGTH;
			GET_SHARED_MEMORY_WITH_OFFSET(
				&pTcb[i].Buffer,
				&pTcb[i].FirewareInfo,
				offset,
				MAX_FIRMWARE_INFORMATION_SIZE);
			offset+=MAX_FIRMWARE_INFORMATION_SIZE;
			GET_SHARED_MEMORY_WITH_OFFSET(
				&pTcb[i].Buffer,
				&pTcb[i].AMSDU_SubHeader,
				offset,
				AMSDU_SUBHEADER_LENGTH);

			RTInsertTailListWithCnt(&Adapter->TcbIdleQueue, &pTcb[i].List, &Adapter->NumIdleTcb);
		}

		//2 Allocate local buffers
		Adapter->LocalBufferArray.Length=Adapter->NumLocalBuffer*sizeof(RT_TX_LOCAL_BUFFER);
		status=PlatformAllocateMemory(Adapter, &Adapter->LocalBufferArray.Ptr,Adapter->LocalBufferArray.Length);
		if(status!=RT_STATUS_SUCCESS)
			return status;

		PlatformZeroMemory(Adapter->LocalBufferArray.Ptr,Adapter->LocalBufferArray.Length);

		pLocalBuffer=(PRT_TX_LOCAL_BUFFER)Adapter->LocalBufferArray.Ptr;
		for(i=0;i<Adapter->NumLocalBuffer;i++)
		{
			status = PlatformAllocateSharedMemory(Adapter, &pLocalBuffer[i].Buffer, Adapter->MAX_TRANSMIT_BUFFER_SIZE);
			if(status!=RT_STATUS_SUCCESS)
				return status;
			
			RTInsertTailListWithCnt(&Adapter->LocalBufferQueue, &pLocalBuffer[i].List, &Adapter->NumLocalBufferIdle);			
		}

		//2 Allocate Firmware local buffers
		Adapter->LocalFWBufferArray.Length=Adapter->NumLocalFWBuffer*sizeof(RT_TX_LOCAL_BUFFER);
		status=PlatformAllocateMemory(Adapter, &Adapter->LocalFWBufferArray.Ptr,Adapter->LocalFWBufferArray.Length);
		if(status!=RT_STATUS_SUCCESS)
			return status;

		PlatformZeroMemory(Adapter->LocalFWBufferArray.Ptr,Adapter->LocalFWBufferArray.Length);

		pLocalBuffer=(PRT_TX_LOCAL_BUFFER)Adapter->LocalFWBufferArray.Ptr;
		
		for(i=0;i<Adapter->NumLocalFWBuffer;i++)
		{
			status = PlatformAllocateSharedMemory(Adapter, &pLocalBuffer[i].Buffer, RT_LOCAL_FW_BUF_SIZE+Adapter->HWDescHeadLength);

			if(status!=RT_STATUS_SUCCESS)
			{
				return status;
			}
			RTInsertTailListWithCnt(&Adapter->LocalFWBufferQueue, &pLocalBuffer[i].List, &Adapter->NumLocalFWBufferIdle);		
		}
	}while(FALSE);
	
	return status;
}

//
// Assumption: RT_TX_SPINLOCK is acquired.
//
VOID
FreeTCBs(
	PADAPTER			Adapter,
	BOOLEAN				bReset
	)
{
	u1Byte					QueueID;
	u4Byte					nFreed;
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER		pLocalBuffer;

	RT_ASSERT(IS_TX_LOCKED(Adapter) == TRUE, ("FreeTCBs(): bTxLocked(%x) should be TRUE\n!", Adapter->bTxLocked));

	//2 Return all TCB to Idle queue first
	//2 And free Packet, RFD or local buffer hide in reserved field
	for(QueueID = 0; QueueID < MAX_TX_QUEUE; QueueID++)
	{
		// 2004.08.11, revised by rcnjko.
		while(!RTIsListEmpty(&Adapter->TcbBusyQueue[QueueID]))
		{
			pTcb=(PRT_TCB)RTRemoveHeadList(&Adapter->TcbBusyQueue[QueueID]);
			ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
		}
		Adapter->nBufInTxDesc[QueueID] = 0; 
		
		while(!RTIsListEmpty(&Adapter->TcbWaitQueue[QueueID]))
		{
			pTcb=(PRT_TCB)RTRemoveHeadList(&Adapter->TcbWaitQueue[QueueID]);
			ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
		}

		// Joseph test for A-MSDU aggregation
		while(!RTIsListEmpty(&Adapter->TcbAggrQueue[QueueID]))
		{
			pTcb=(PRT_TCB)RTRemoveHeadList(&Adapter->TcbAggrQueue[QueueID]);
			Adapter->TcbCountInAggrQueue[QueueID]--;
			ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
		}
	}

	if(!bReset)
	{
		//2 Free local buffers
		if(!RTIsListEmpty(&Adapter->LocalBufferQueue))
		{
			nFreed=0;
			while(!RTIsListEmpty(&Adapter->LocalBufferQueue))
			{
				pLocalBuffer=(PRT_TX_LOCAL_BUFFER)RTRemoveHeadListWithCnt(&Adapter->LocalBufferQueue, &Adapter->NumLocalBufferIdle);
	
				PlatformFreeSharedMemory(Adapter, &pLocalBuffer->Buffer);
				nFreed++;
			}
			RT_ASSERT(nFreed==Adapter->NumLocalBuffer,("Freed local buffer(%ld) less than allocated(%ld)!!\n", nFreed, Adapter->NumLocalBuffer));
		}
		if(Adapter->LocalBufferArray.Ptr != NULL)
		{
			PlatformFreeMemory(Adapter->LocalBufferArray.Ptr,Adapter->LocalBufferArray.Length);
		}

		//2 Free Firmware local buffers
		if(!RTIsListEmpty(&Adapter->LocalFWBufferQueue))
		{
			nFreed=0;
			while(!RTIsListEmpty(&Adapter->LocalFWBufferQueue))
			{
				pLocalBuffer=(PRT_TX_LOCAL_BUFFER)RTRemoveHeadListWithCnt(&Adapter->LocalFWBufferQueue, &Adapter->NumLocalFWBufferIdle);
	
				PlatformFreeSharedMemory(Adapter, &pLocalBuffer->Buffer);
				nFreed++;
			}
			RT_ASSERT(nFreed==Adapter->NumLocalFWBuffer,("Freed FW local buffer(%ld) less than allocated(%ld)!!\n", nFreed, Adapter->NumLocalFWBuffer));
		}
		if(Adapter->LocalFWBufferArray.Ptr != NULL)
		{
			PlatformFreeMemory(Adapter->LocalFWBufferArray.Ptr,Adapter->LocalFWBufferArray.Length);
		}


		//2 Free idle queue
		if(!RTIsListEmpty(&Adapter->TcbIdleQueue))
		{
			nFreed=0;
			while(!RTIsListEmpty(&Adapter->TcbIdleQueue))
			{
				pTcb=(PRT_TCB)RTRemoveHeadListWithCnt(&Adapter->TcbIdleQueue, &Adapter->NumIdleTcb);
	
				PlatformFreeSharedMemory(Adapter, &pTcb->Buffer);
				nFreed++;
			}
			RT_ASSERT(nFreed==Adapter->NumTcb,("Freed TCB(%ld) less than allocated(%ld)!!\n", nFreed, Adapter->NumTcb));
		}
		if(Adapter->TcbBuffer.Ptr != NULL)
		{
			PlatformFreeMemory(Adapter->TcbBuffer.Ptr,Adapter->TcbBuffer.Length);
		}
	}
}

VOID
RemoveZeroLengthBuffer(
	PRT_TCB				pTcb
	)
{
	u2Byte 	i,j;
	u2Byte	k = 0;

	for(i=0,j=0;i<pTcb->BufferCount;i++)
	{
		if(pTcb->BufferList[i].Length==0)
			continue;

		if(i!=j)
		{
			pTcb->BufferList[j]=pTcb->BufferList[i];
			
			if(IS_REMOVE_ZERO_LEN_BUF(pTcb->SourceAdapt))
			{
				if(pTcb->SubHdrIndexAry[k] == (u1Byte)i)
				{
					pTcb->SubHdrIndexAry[k] = (u1Byte)j;
					k++;
				}
			}
		}
		j++;
	}
	pTcb->BufferCount=j;
}

BOOLEAN
RetrieveSegmentDataFromTCB(
	PRT_TCB				pTcb,
	u1Byte				SkipBuffers,
	u4Byte				DataOffset,
	PVOID				pDataBuffer,
	u4Byte				DataBufLength
	)
{
	BOOLEAN		bResult = TRUE;
	u4Byte		BytesRead = 0;
	u4Byte		i;
	u4Byte		offset = 0;
	pu1Byte		pBuffer = (pu1Byte)pDataBuffer;

	for(i=SkipBuffers;i<pTcb->BufferCount;i++)
	{
		if(pTcb->BufferList[i].Length==0)
			continue;

		for (; BytesRead!=DataBufLength; BytesRead++)
		{
			if ( (pTcb->BufferList[i].Length + offset) > (DataOffset + BytesRead) )
			{
				pBuffer[BytesRead] = pTcb->BufferList[i].VirtualAddress[DataOffset+BytesRead-offset];
			}
			else
				break;
		}

		if (BytesRead == DataBufLength) break;
		
		offset +=pTcb->BufferList[i].Length;
	}

	if (BytesRead != DataBufLength)
	{
		RT_TRACE(COMP_SEND, DBG_LOUD, ("RetrieveSegmentDataFromTCB(): some data can not be retrived\n"));
		bResult = FALSE;
	}

	return bResult;
}

/**
* Function:	TcbGetTOSField
* 
* Overview:	Get TOS field of IP header in TCB buffer.
*			Input the TCB pointer and indicate SkipBuffer number for this function to get the 
*			Ethernet packet sent from upper layer.
*			Using this function before packet conversion. (TranslateHeader())
* 
* Input:		
*		PRT_TCB		pTcb
*		u1Byte		SkipBuffers
* 			
* Output:		
*		None
* 		
* Return:     	
*		TOS value
*/
u1Byte
TcbGetTOSField(
	PRT_TCB		pTcb,
	u1Byte		SkipBuffers
	)
{
	u2Byte	TypeLength;
	u1Byte	TypeLengthBuf[2];
	u1Byte	TOS = 0;

	RetrieveSegmentDataFromTCB(
		pTcb,
		SkipBuffers,
		12,
		TypeLengthBuf,
		sizeof(TypeLengthBuf));

			TypeLength =  (((u2Byte)TypeLengthBuf[0])<<8) + TypeLengthBuf[1];

	if(TypeLength == 0x0800)
	{
		RetrieveSegmentDataFromTCB(
			pTcb,
			SkipBuffers,
			15,
			&TOS,
			sizeof(TOS));
		TOS = TOS>>5;
	}

	return TOS;
}


VOID
TranslateHeader(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	)
{
	u2Byte				i;
	u4Byte				offset=0;
	u2Byte				TypeLength;
	u1Byte				TypeLengthBuf[2];
	u1Byte				TOS;
	static u1Byte			SnapBridgeTunnel[6] = { 0xAA, 0xAA, 0x03, 0x00, 0x00, 0xF8 };
	static u1Byte			Snap802_1H[6] = { 0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00 };
	static u1Byte			Snap_CKIP_MIC[8] = { 0xAA, 0xAA, 0x03, 0x00, 0x40, 0x96, 0x00, 0x02 };
	pu1Byte				pHeader;
	u2Byte				DAOffset,SAOffset,BSSIDOffset;
	u1Byte				PAEGroupAddress[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x03};
	u1Byte				seqv[4];
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	BOOLEAN				bCCX8021xenable = FALSE;
	BOOLEAN				bAPSuportCCKM = FALSE;
	PRT_SECURITY_T			pSecInfo = &(pMgntInfo->SecurityInfo);
	PRT_WLAN_STA			pSTA=NULL;
	u1Byte				IP_Protocol, IP_HeaderLen, DHCP_Port[4];
	u2Byte				DHCP_Src_Port, DHCP_Dst_Port;
	u1Byte				Addr1[6];
	u1Byte				additionalHeaderLen = ETHERNET_ADDRESS_LENGTH*2 + ETHERNET_HEADER_SIZE + WAPI_EXT_LEN + 2 + 16 - 14;

	//2 Fill wireless header
	pHeader=pTcb->BufferList[0].VirtualAddress;
	
	//1 Note:	BufferList[0] is reserved for 802.11 header
	//1		BufferList[1] is reserved for LLC

	pTcb->bBroadcast	= MacAddr_isBcst(pTcb->BufferList[2].VirtualAddress);
	if( pTcb->bBroadcast == FALSE )
		pTcb->bMulticast = MacAddr_isMulticast(pTcb->BufferList[2].VirtualAddress);
	else
		pTcb->bMulticast = FALSE;

	if( (CheckFragment(Adapter, pTcb, Adapter->TXPacketShiftBytes) == FALSE) &&
		( (pTcb->PacketLength + additionalHeaderLen ) <= FRAGMENT_THRESHOLD(Adapter) ) )
	{	

	}

	{
		SET_80211_HDR_FRAME_CONTROL(pHeader, 0);
		SET_80211_HDR_TYPE_AND_SUBTYPE(pHeader, Type_Data);
		
		pTcb->bTDLPacket = FALSE;
				
		switch(pMgntInfo->OpMode)
		{
			case RT_OP_MODE_IBSS:				// 0,0
				DAOffset=4;
				SAOffset=10;
				BSSIDOffset=16;
				break;
			case RT_OP_MODE_AP:				// 0,1
				SET_80211_HDR_FROM_DS(pHeader, 1);
				DAOffset=4;
				SAOffset=16;
				BSSIDOffset=10;
				break;
			case RT_OP_MODE_INFRASTRUCTURE:	// 1,0
				DAOffset=16;
				SAOffset=10;
				BSSIDOffset=4;
				SET_80211_HDR_TO_DS(pHeader, 1);
				break;
			default:
				RT_ASSERT(FALSE, ("TranslateHeader(): Unknown OpMode: %d\n", pMgntInfo->OpMode));
				// Handle it as IBSS, 2004.08.21, by rcnjko.
				DAOffset=4;
				SAOffset=10;
				BSSIDOffset=16;
				break;
		}
		SET_80211_HDR_DURATION(pHeader, 0);
		PlatformMoveMemory(&pTcb->BufferList[0].VirtualAddress[BSSIDOffset], pMgntInfo->Bssid, ETHERNET_ADDRESS_LENGTH);

		// If in PSP XLink mode, pass the source address in 802.3 header rather than current MAC address.
		// 2006.09.04, by shien chang.
		if (pMgntInfo->bPSPXlinkMode)
		{
			PlatformMoveMemory(&pTcb->BufferList[0].VirtualAddress[SAOffset], &pTcb->BufferList[2].VirtualAddress[6], ETHERNET_ADDRESS_LENGTH);
			PlatformMoveMemory(pTcb->SourceAddress, &pTcb->BufferList[2].VirtualAddress[6], ETHERNET_ADDRESS_LENGTH);
		}
		else
		{
			//2004/07/22, kcwu
			PlatformMoveMemory(&pTcb->BufferList[0].VirtualAddress[SAOffset], Adapter->CurrentAddress, ETHERNET_ADDRESS_LENGTH);
			PlatformMoveMemory(pTcb->SourceAddress, Adapter->CurrentAddress, ETHERNET_ADDRESS_LENGTH);
		}
		
		PlatformMoveMemory(&pTcb->BufferList[0].VirtualAddress[DAOffset], pTcb->BufferList[2].VirtualAddress, ETHERNET_ADDRESS_LENGTH);
		//2004/07/22, kcwu
		PlatformMoveMemory(pTcb->DestinationAddress, pTcb->BufferList[2].VirtualAddress, ETHERNET_ADDRESS_LENGTH);
		SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);
	}

	//Get Address1 for later use. 2007.04.03, by shien chang.
	GET_80211_HDR_ADDRESS1(pHeader, Addr1);

#if TX_TCP_SEQ_CHECK
	TxTcpSeqCheck(Adapter, pTcb, 2);
#endif

	//2 Add LLC if need
	// Find TypeLength, 2006.10.27, refined by shien chang.
	RetrieveSegmentDataFromTCB(
		pTcb,	
		2,						// The number of buffer in bufferlist to skip.
		12, 						// The TypeLength offset.
		TypeLengthBuf,			// 2 Byte buffer for TypeLength data.
		sizeof(TypeLengthBuf));		// Buffer length.

	TypeLength=(((u2Byte)EF1Byte(TypeLengthBuf[0]))<<8) + EF1Byte(TypeLengthBuf[1]);

	if(TypeLength == 0x888e)
	{ // EAPOL packet.
		int nTmpOffset = 0;
		u1Byte btTmp = 0;

		// For delaying enter PS mode for FW control LPS. by tynli.
		if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS && 
			GET_POWER_SAVE_CONTROL(pMgntInfo)->bLeisurePs)
		{
			//DbgPrint("EAPOL packet----->\n");
			pMgntInfo->DelayLPSLastTimeStamp = PlatformGetCurrentTime();
			
			// 20100902 Joseph: Since we send NULL frame in LeisurePSLeave() function, TX_SPINLOCK
			// shall be release for all product and OS. Originally, TX_SPINLOCK is raised twice here and
			// system hang immediately.
			// We should call LeisurePSLeave to turn the RF on to receive EAPOL/DHCP/ARP packets, 
			// and release TX spinlock first beacuse we will acquire TX spinlock for FW H2C commands in 92su.
			PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
			LeisurePSLeave(Adapter, LPS_DISABLE_TX_EAPOL_PKT);
			PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
		}
		
		if(	!SecIsTxKeyInstalled(Adapter, Addr1) )
		{
			pTcb->EncInfo.SecProtInfo = RT_SEC_EAPOL_BEFORE_KEY_INSTALLED;
		}
		else
		{
			pTcb->EncInfo.SecProtInfo = RT_SEC_EAPOL_AFTER_KEY_INSTALLED;
		}

		// Send in lowest basic rate to get more time for adding key. 2005.04.19, by rcnjko.
		if(pMgntInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)	
		{
			pTcb->DataRate = MgntQuery_TxRateExcludeCCKRates(pMgntInfo->mBrates);//0xc;//ofdm 6m
			pTcb->bTxDisableRateFallBack = FALSE;
		}
		else
		{
			pTcb->DataRate = Adapter->MgntInfo.LowestBasicRate;
			//disable rate fallback for EAPOL packate to resolve 5G ad hoc cck hang. zhiyuan 2012/04/28
			if(IS_WIRELESS_MODE_A(Adapter) || IS_WIRELESS_MODE_N_5G(Adapter))
				pTcb->bTxDisableRateFallBack = TRUE;			
		}

		RT_DISP(FDM, WA_IOT, ("EAPOL TranslateHeader(), pTcb->DataRate = 0x%x\n", pTcb->DataRate));
		pTcb->bTxUseDriverAssingedRate = TRUE;

		// Check if this is a MIC failure report packet. 2005.09.06, by rcnjko.
		for(i = 2; i < pTcb->BufferCount; i++)
		{
			if(pTcb->BufferList[i].Length == 0)
				continue;
			
			if(pTcb->BufferList[i].Length + nTmpOffset > 19)
			{
				btTmp = EF1Byte(pTcb->BufferList[i].VirtualAddress[19-nTmpOffset]);
				
				if(btTmp == 0x0d || btTmp == 0x0f)
				{
					pTcb->EncInfo.SecProtInfo |= RT_SEC_MIC_FAILURE_REPORT;	
				}

				break;
			}
		
			nTmpOffset += pTcb->BufferList[i].Length;
		}
	}
	else
	{ // Non-EAPOL packets.
		pTcb->EncInfo.SecProtInfo = RT_SEC_NORMAL_DATA;
	}

	CCX_QueryCCKMSupport(Adapter, &bCCX8021xenable, &bAPSuportCCKM);
	
	{
		// Insert LLC in need
		if(TypeLength > (MAXIMUM_ETHERNET_PACKET_SIZE-ETHERNET_HEADER_SIZE) )
		{
			if ((TypeLength == 0x8137) || (TypeLength == 0x80F3)) 
			{ // Bridge tunneling
				PlatformMoveMemory(
					pTcb->BufferList[1].VirtualAddress,
					SnapBridgeTunnel,
					LLC_HEADER_SIZE);

				pTcb->BufferList[1].Length=LLC_HEADER_SIZE;
				pTcb->PacketLength += LLC_HEADER_SIZE;
			} 
			//else if( pSecInfo->pCkipPara->bIsMIC) It will add SNAP to EAPOL packet in 802.1x ,2006.10.04 CCW
			else if(pSecInfo->pCkipPara->bIsMIC && (pTcb->EncInfo.SecProtInfo != RT_SEC_EAPOL_BEFORE_KEY_INSTALLED)
					&& !(( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 || 
					pSecInfo->PairwiseEncAlgorithm ==RT_ENC_ALG_WEP104 ) &&  // WEP mode 
					pSecInfo->AuthMode == RT_802_11AuthModeOpen &&    // Open mode
					!( bCCX8021xenable && bAPSuportCCKM )      && // Not in CCKM mode
					IsSecProtEapol(pTcb->EncInfo.SecProtInfo) == TRUE)				// Is EPAOL Packet 							
				  )
			{ // CKIP MIC SNAP
				RT_TRACE( COMP_CKIP, DBG_LOUD, ("[CKIP] TranslateHeader(): CMIC snap case!\n") );
			
			      // Set CKIP Snap 
				PlatformMoveMemory(
							pTcb->BufferList[1].VirtualAddress,
							Snap_CKIP_MIC,
							8);
				// Set MIC and SEQ to zero
				PlatformZeroMemory(
							pTcb->BufferList[1].VirtualAddress + 8,
							8);

				//Set SEQ 
				CKIP_SEQ_DECIMAL2ARRAY( pSecInfo->pCkipPara->ulSeqUpLink, seqv );
				PlatformMoveMemory(
					pTcb->BufferList[1].VirtualAddress + 12,
					seqv,
					4);
				pSecInfo->pCkipPara->ulSeqUpLink += 2;
				
				pTcb->BufferList[1].Length=16;
				pTcb->PacketLength += 16;
			} 
			else 
			{ // 802.1h
				PlatformMoveMemory(
					pTcb->BufferList[1].VirtualAddress,
					Snap802_1H,
					LLC_HEADER_SIZE);

				pTcb->BufferList[1].Length=LLC_HEADER_SIZE;
				pTcb->PacketLength += LLC_HEADER_SIZE;
			}

			offset=ETHERNET_HEADER_SIZE - TYPE_LENGTH_FIELD_SIZE;
		}
		else
		{
			offset=ETHERNET_HEADER_SIZE;
		}
	}

	//
	// Parsing the DSCP in IP header TOS field.
	// Set default priority for the packets other than IP. Revised by Joseph
	//
	if( TypeLength == 0x0800 )		// IP header
	{
		RetrieveSegmentDataFromTCB(
			pTcb,
			2,
			15,
			&TOS,
			sizeof(TOS));		
		pTcb->priority = TOS>>5;
		RT_DISP(FQoS, QoS_INIT,  ("Translate Header pTcb->priority = %d\r\n", pTcb->priority));
	}
	else
	{
		pTcb->priority = 0;
	}

	//*************************************************************************
	// cosa added 03/12/2008
	// The following is for DHCP and ARP packet, we use cck1M to tx these packets to prevent
	// DHCP protocol fail
	if( TypeLength == 0x0800 )		// IP header
	{
		RetrieveSegmentDataFromTCB(
			pTcb,
			2,
			14+9,
			&IP_Protocol,
			sizeof(IP_Protocol));

		if(IP_Protocol == 0x11)		// UDP protocol
		{
			//DbgPrint("UDP Protocol !!\n");
			RetrieveSegmentDataFromTCB(
			pTcb,
			2,
			14,				// 14 is the start of the IP Header
			&IP_HeaderLen,	// each unit = 4 bytes.
			sizeof(IP_HeaderLen));
			IP_HeaderLen &= 0xf; 	//bit0~3
			IP_HeaderLen *= 4;	//Bytes of IP Header.
			//DbgPrint("IP_HeaderLen = %d\n", IP_HeaderLen);

			RetrieveSegmentDataFromTCB(
			pTcb,
			2,
			14+IP_HeaderLen,	// 14 is the start of the IP Header
			DHCP_Port,	// each unit = 4 bytes.
			sizeof(DHCP_Port));
			//DbgPrint("DHCP_Port = [0]=%x,[1]=%x, [2]=%x, [3]=%x \n", DHCP_Port[0], DHCP_Port[1], DHCP_Port[2], DHCP_Port[3]);
			DHCP_Src_Port = (((u2Byte)DHCP_Port[0])<<8) + DHCP_Port[1];
			DHCP_Dst_Port = (((u2Byte)DHCP_Port[2])<<8) + DHCP_Port[3];
			//DbgPrint("DHCP_Src_Port = %d, DHCP_Dst_Port = %d \n", DHCP_Src_Port, DHCP_Dst_Port);
			if((DHCP_Src_Port == 68 && DHCP_Dst_Port == 67) ||
			    (DHCP_Src_Port == 67 && DHCP_Dst_Port == 68))
			{
				OCTET_STRING		osMpdu;
				pu1Byte 			pRaddr = NULL;

				//Note:
				// osMpdu may not include the packet content but just 802.11 header.
				FillOctetString(osMpdu, pTcb->BufferList[0].VirtualAddress, (u2Byte)pTcb->BufferList[0].Length);
				pRaddr = Frame_pRaddr(osMpdu);	
							
				//
				// Translate the DHCP offer packet from multicast packet to unicast so that the client
				// can receive the DHCP response easier. By Bruce, 2011-01-06.
				//
				if(ACTING_AS_AP(pTcb->SourceAdapt) && MacAddr_isMulticast(pRaddr))
				{
					u1Byte			ClientMac[6];
					PRT_WLAN_STA	pEntry;

					// Get the Mac address in DHCP packet.
					RetrieveSegmentDataFromTCB(
						pTcb,
						2,
						IP_HeaderLen + 14 + 8 + 28, // UDP header(8) + Client Mac Address offset (28)
						ClientMac,	// each unit = 4 bytes.
						6);

					// Check if this mac address is belong to one of the clients.
					pEntry = AsocEntry_GetEntry(&pTcb->SourceAdapt->MgntInfo, ClientMac);
					
					if(pEntry != NULL)
					{
						RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "Replace the DHCP receiver address from multicast to ", ClientMac);
						PlatformMoveMemory(pRaddr, ClientMac, 6);
						PlatformMoveMemory(pTcb->DestinationAddress, ClientMac, 6);
					}
				}						
				// 68 : UDP BOOTP client
				// 67 : UDP BOOTP server
				//DbgPrint("DHCP Protocol !!\n");
				// Use low rate to send DHCP packet.
				if(pMgntInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)	
				{
					pTcb->DataRate = MgntQuery_TxRateExcludeCCKRates(pMgntInfo->mBrates);//0xc;//ofdm 6m
					pTcb->bTxDisableRateFallBack = FALSE;
				}
				else
					pTcb->DataRate = Adapter->MgntInfo.LowestBasicRate; 
				RT_DISP(FDM, WA_IOT, ("DHCP TranslateHeader(), pTcb->DataRate = 0x%x\n", pTcb->DataRate));
				pTcb->bTxUseDriverAssingedRate = TRUE;
				pTcb->specialDataType = PACKET_DHCP;
				pMgntInfo->LPSDelayCnt= 
					GET_POWER_SAVE_CONTROL(pMgntInfo)->LPSAwakeIntvl*2;
				// For delaying enter PS mode for FW control LPS. by tynli.
				if(	GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS &&
					GET_POWER_SAVE_CONTROL(pMgntInfo)->bLeisurePs)
				{ //by tynli
					pMgntInfo->DelayLPSLastTimeStamp = PlatformGetCurrentTime();

					// 20100902 Joseph: Since we send NULL frame in LeisurePSLeave() function, TX_SPINLOCK
					// shall be release for all product and OS. Originally, TX_SPINLOCK is raised twice here and
					// system hang immediately.
					PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
					LeisurePSLeave(Adapter, LPS_DISABLE_TX_DHCP_PKT);
					PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
				}
			}
		}
	}
	if( TypeLength == 0x0806 )		// IP ARP packet
	{
		//DbgPrint("IP ARP !!\n");
		pMgntInfo->LPSDelayCnt = pMgntInfo->mDtimCount;
		if(pMgntInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)	
		{
			pTcb->DataRate = MgntQuery_TxRateExcludeCCKRates(pMgntInfo->mBrates);//0xc;//ofdm 6m
			pTcb->bTxDisableRateFallBack = FALSE;
		}
		else
			pTcb->DataRate = pMgntInfo->LowestBasicRate; 
		RT_DISP(FDM, WA_IOT, ("ARP TranslateHeader(), pTcb->DataRate = 0x%x\n", pTcb->DataRate));
		pTcb->bTxUseDriverAssingedRate = TRUE;
		pTcb->specialDataType = PACKET_ARP;
		if(	GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS &&
			GET_POWER_SAVE_CONTROL(pMgntInfo)->bLeisurePs)
		{ // by tynli
			//DbgPrint("ARP packet----->\n");
			pMgntInfo->DelayLPSLastTimeStamp = PlatformGetCurrentTime();
			
			// 20100902 Joseph: Since we send NULL frame in LeisurePSLeave() function, TX_SPINLOCK
			// shall be release for all product and OS. Originally, TX_SPINLOCK is raised twice here and
			// system hang immediately.
			PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
			LeisurePSLeave(Adapter, LPS_DISABLE_TX_ARP_PKT);
			PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
		}
	}
	//*************************************************************************

	TDLS_TxTranslateHeader(Adapter, pTcb, RT_PROTOCOL_802_3);

	// Remove 802.3 header
	pTcb->PacketLength -= offset;

	//3 Add SecurityHeader
	if(!IsSecProtEapolBeforeKeyInstalled(pTcb->EncInfo.SecProtInfo))
	{
		u1Byte	CurrCcxVerNumber = 0;

		CCX_QueryVersionNum(Adapter, &CurrCcxVerNumber);
		
		if(!(( 	pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 || 
				pSecInfo->PairwiseEncAlgorithm ==RT_ENC_ALG_WEP104 ) &&  // WEP mode 
				pSecInfo->AuthMode == RT_802_11AuthModeOpen &&    // Open mode
				!( bCCX8021xenable && bAPSuportCCKM && CurrCcxVerNumber >= 2 )      && // Not in CCKM mode
				IsSecProtEapol(pTcb->EncInfo.SecProtInfo) == TRUE							// Is EPAOL Packet 
		))
		{
			pTcb->BufferList[0].Length = (sMacHdrLng + pSecInfo->EncryptionHeadOverhead);
		}
		else
		{
			if(!PlatformCompareMemory(PAEGroupAddress, pTcb->DestinationAddress, ETHERNET_ADDRESS_LENGTH))
			{ // <RJ_TODO> Does this case happen after PTK installed?
				PlatformMoveMemory(pTcb->DestinationAddress, pMgntInfo->Bssid, ETHERNET_ADDRESS_LENGTH);
			}

			pTcb->BufferList[0].Length = sMacHdrLng;
		}
	}
	else
	{
		if(!PlatformCompareMemory(PAEGroupAddress, pTcb->DestinationAddress, ETHERNET_ADDRESS_LENGTH))
		{ // <RJ_TODO> Does this case happen after PTK installed?
			PlatformMoveMemory(pTcb->DestinationAddress, pMgntInfo->Bssid, ETHERNET_ADDRESS_LENGTH);
		}

		pTcb->BufferList[0].Length = sMacHdrLng;
	}

	//3 Added QoS Control
	if(ACTING_AS_AP(Adapter) || Adapter->MgntInfo.mIbss )
		pSTA = AsocEntry_GetEntry(pMgntInfo, Addr1);

	RT_DISP(FQoS, QoS_INIT,  ("TranslateHeader QoS header\r\n"));

	if(	(pMgntInfo->pStaQos->CurrentQosMode > QOS_DISABLE )	&&
		(((ACTING_AS_AP(Adapter) || pMgntInfo->mIbss) && pSTA != NULL)?((pSTA->QosMode)>QOS_DISABLE):TRUE) &&
		(MacAddr_isMulticast(Addr1)==FALSE) &&
		(MacAddr_isBcst(Addr1)==FALSE)	)
	{
		// 1. Reserve 2-bytes QoS Control Field for unicast QoS Data frame.
		pTcb->BufferList[0].Length += sQoSCtlLng;

		if(pMgntInfo->pHTInfo->bRDGEnable)
		{
			pTcb->BufferList[0].Length += 4; //HTC len
			SET_80211_HDR_ORDER(pHeader,1);
			pTcb->BufferList[0].VirtualAddress[24+2+3] = 0x80;
		}

		// 2. Change SubType in Frame Control.
		SET_80211_HDR_QOS_EN(pHeader, 1);

		// 4. Use highest priority to send EPAOL-Key packet. Annie, 2005-12-22.
		// Ref: 802.11e/D13.0, 8.5.2, in page 67:
		// "When priority processing of data frames is supported, a STA's SME should send EAPOL-Key frames at the highest priority."
		if( TypeLength == 0x888e )
		{
			pTcb->priority = 7;
		}
	}

	pTcb->PacketLength += pTcb->BufferList[0].Length;

	for(i=2;i<pTcb->BufferCount && offset > 0;i++)
	{
		if(pTcb->BufferList[i].Length==0)
			continue;

		if(pTcb->BufferList[i].Length >= offset)
		{
			MAKE_SHARED_MEMORY_OFFSET_AT_FRONT(&pTcb->BufferList[i], offset);
			offset=0;
		}
		else
		{
			offset-=pTcb->BufferList[i].Length;
			pTcb->BufferList[i].Length=0;
		}
	}

	// If original 802.3 header size is small than 4 byte, append it to LLC header and remove it.
	if(pTcb->BufferList[1].Length > 0)
	{
		for(i=2;i<pTcb->BufferCount;i++)
		{	// Only check first nonzero buffer
			if(pTcb->BufferList[i].Length==0)
				continue;

			if(pTcb->BufferList[i].Length < MIN_DESC_BUFFER_LENGTH)
			{
				PlatformMoveMemory(
					&pTcb->BufferList[1].VirtualAddress[pTcb->BufferList[1].Length],
					pTcb->BufferList[i].VirtualAddress,
					pTcb->BufferList[i].Length);
				pTcb->BufferList[1].Length += pTcb->BufferList[i].Length;
				pTcb->BufferList[i].Length=0;
			}
			break;
		}
	}

}

// 20100309 Joseph: Revise special frame parsing in Tx path.
// Also handling the packets saved in LOCAL_BUFFER. The buffer offset and index is different from the
// buffer in SYSTEM_BUFFER.
BOOLEAN
CheckSpecialTxPktContentFromHighLayer(
	PRT_TCB		pTcb
	)
{
	u1Byte	IP_Protocol, IP_HeaderLen;
	u2Byte	IP_Src_Port = 0, IP_Dst_Port = 0;
	u2Byte	u2Tmp = 0;

	u1Byte	Check_Buf_Index, Check_Buf_Offset;	

	if(pTcb->BufferType==RT_TCB_BUFFER_TYPE_LOCAL)
	{
		// 20100309 Joseph: Add packet parsing for LOCAL BUFFER case.
		// Payload is saved in buffer 1.
		// Since LLC header is coalesced with oter payload in buffer 1, we just skip first 8 bytes LLC
		// in front of buffer to get the paylaod.
		Check_Buf_Index = 1; 
		Check_Buf_Offset = 8; 
	}
	else
	{
		// 20100309 Joseph: Normal case.
		// Payload is saved in buffer 2 and LLC is separate with it in buffer 1.
		Check_Buf_Index = 2; 
		Check_Buf_Offset = 0; 
	}

	RetrieveSegmentDataFromTCB(
		pTcb,
		Check_Buf_Index, 
		9+Check_Buf_Offset,	// Get payload offset 9 to get protocol type.
		&IP_Protocol,
		sizeof(IP_Protocol));

	// DbgPrint("IP_Protocol = %d\n", IP_Protocol);
	RetrieveSegmentDataFromTCB(
		pTcb,
		Check_Buf_Index,
		0+Check_Buf_Offset,		// Offset 0 is the start of the IP Header
		&IP_HeaderLen,	// each unit = 4 bytes.
		sizeof(IP_HeaderLen));
		IP_HeaderLen &= 0xf; 	//bit0~3
		IP_HeaderLen *= 4;	//Bytes of IP Header.
		//DbgPrint("IP_HeaderLen = %d\n", IP_HeaderLen);

	if(0x11 == IP_Protocol || 0x06 == IP_Protocol)
	{
		// Get source port
		RetrieveSegmentDataFromTCB(
				pTcb,
				Check_Buf_Index,
				IP_HeaderLen+Check_Buf_Offset, 
				&u2Tmp,
				2);
		IP_Src_Port = H2N2BYTE(u2Tmp);

		// Get source port
		RetrieveSegmentDataFromTCB(
				pTcb,
				Check_Buf_Index,
				IP_HeaderLen + Check_Buf_Offset + 2, 
				&u2Tmp,
				2);
		IP_Dst_Port = H2N2BYTE(u2Tmp);
	}	
		
	if(IP_Protocol == 0x11)		// UDP protocol
	{
		pTcb->IPType = TX_IPTYPE_UDP;
		
		//DbgPrint("DHCP_Src_Port = %d, DHCP_Dst_Port = %d \n", DHCP_Src_Port, DHCP_Dst_Port);
		if((IP_Src_Port == 68 && IP_Dst_Port == 67) ||
		    (IP_Src_Port == 67 && IP_Dst_Port == 68))
		{
			OCTET_STRING		osMpdu;
			pu1Byte 			pRaddr = NULL;

			//Note:
			// osMpdu may not include the packet content but just 802.11 header.
			FillOctetString(osMpdu, pTcb->BufferList[0].VirtualAddress, (u2Byte)pTcb->BufferList[0].Length);
			pRaddr = Frame_pRaddr(osMpdu);	
						
			//
			// Translate the DHCP offer packet from multicast packet to unicast so that the client
			// can receive the DHCP response easier. By Bruce, 2011-01-06.
			//
			if(ACTING_AS_AP(pTcb->SourceAdapt) && MacAddr_isMulticast(pRaddr))
			{
				u1Byte			ClientMac[6];
				PRT_WLAN_STA	pEntry;

				// Get the Mac address in DHCP packet.
				RetrieveSegmentDataFromTCB(
					pTcb,
					Check_Buf_Index,
					IP_HeaderLen + Check_Buf_Offset + 8 + 28, // UDP header(8) + Client Mac Address offset (28)
					ClientMac,	// each unit = 4 bytes.
					6);

				// Check if this mac address is belong to one of the clients.
				pEntry = AsocEntry_GetEntry(&pTcb->SourceAdapt->MgntInfo, ClientMac);				
				
				if(pEntry != NULL)
				{					
					RT_PRINT_ADDR(COMP_AP, DBG_LOUD, "Replace the DHCP receiver address from multicast to ", ClientMac);
					// Copy the client mac address to the ra.
					PlatformMoveMemory(pRaddr, ClientMac, 6);	
					PlatformMoveMemory(pTcb->DestinationAddress, ClientMac, 6);
				}
			}						

			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else if(0x06 == IP_Protocol) // TCP
	{
		pTcb->IPType = TX_IPTYPE_TCP;
	
		// DbgPrint("TCP src port = %d, dst port = %d\n", IP_Src_Port, IP_Dst_Port);
		if(IP_Src_Port == 7236 || IP_Dst_Port == 7236 ||
		    IP_Src_Port == 8554 || IP_Dst_Port == 8554)
		{
			u1Byte	offset = Check_Buf_Index;
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("[MIRACAST/P2P] Tx Display Packet:\n"));
			for(offset = Check_Buf_Index; offset < pTcb->BufferCount; offset ++)
			{
				if(0 == pTcb->BufferList[offset].Length)
					continue;
				RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "",
					pTcb->BufferList[offset].VirtualAddress, pTcb->BufferList[offset].Length);
			}			
		}		
	}
	return FALSE;
}

//
// For NDIS6 802.11 Tx frame handling.
// Added by Annie, 2006-10-20.
//
VOID
FillPartialHeader(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	)
{
	u2Byte 				TypeLength;
	u1Byte 				TypeLengthBuf[2];
	pu1Byte				pHeader = pTcb->BufferList[0].VirtualAddress;
	u1Byte				PAEGroupAddress[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x03};
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	u1Byte				Addr1[6];
	BOOLEAN				bIsSpecialPkt = FALSE;

	//2 Fill wireless header (Some field is filled by upper layer)
	SET_80211_HDR_DURATION(pHeader, 0);
	SET_80211_HDR_FRAGMENT_SEQUENCE(pHeader, 0);

	pTcb->bTDLPacket = FALSE;

	// <SC_TODO: to take care of PSP X-Link mode>
	if( GET_80211_HDR_TO_DS(pHeader)==0 && GET_80211_HDR_FROM_DS(pHeader)==0)
	{
		// IBSS
		GET_80211_HDR_ADDRESS2(pHeader, pTcb->SourceAddress);
		GET_80211_HDR_ADDRESS1(pHeader, pTcb->DestinationAddress);

		if(	MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED
			|| MgntActQuery_ApType(Adapter) == RT_AP_TYPE_LINUX)
		{
			//
			// 061227, rcnjko: 
			// Translate header for NDIS6 faked AP mode (UI make upper layer take us as AdHoc mode). 
			//
			if( pMgntInfo->OpMode == RT_OP_MODE_AP && IsFrameTypeData((pTcb->BufferList[0].VirtualAddress)) )
			{
				u1Byte	tmpAddr[6];	
				u1Byte	tmpAddr2[6];

				SET_80211_HDR_TO_DS(pHeader, 0);
				SET_80211_HDR_FROM_DS(pHeader, 1);

				GET_80211_HDR_ADDRESS2(pHeader,tmpAddr);
				GET_80211_HDR_ADDRESS3(pHeader,tmpAddr2);
				SET_80211_HDR_ADDRESS2(pHeader,tmpAddr2);
				SET_80211_HDR_ADDRESS3(pHeader,tmpAddr);
			}
		}
	}
	else if (GET_80211_HDR_TO_DS(pHeader) == 1 && GET_80211_HDR_FROM_DS(pHeader) == 0)
	{
		// From STA to AP.
		GET_80211_HDR_ADDRESS2(pHeader,pTcb->SourceAddress);
		GET_80211_HDR_ADDRESS3(pHeader,pTcb->DestinationAddress);

	}
	else if (GET_80211_HDR_TO_DS(pHeader) == 0 && GET_80211_HDR_FROM_DS(pHeader) == 1)
	{
		// From AP to STA.
		GET_80211_HDR_ADDRESS3(pHeader,pTcb->SourceAddress);
		GET_80211_HDR_ADDRESS1(pHeader,pTcb->DestinationAddress);
	}
	else
	{
		// WDS
		// <SC_TODO:>
	}

	pTcb->bBroadcast	= MacAddr_isBcst(pTcb->DestinationAddress);
	if( pTcb->bBroadcast == FALSE )
		pTcb->bMulticast = MacAddr_isMulticast(pTcb->DestinationAddress);
	else
		pTcb->bMulticast = FALSE;
			
	GET_80211_HDR_ADDRESS1(pHeader, Addr1);
	
	// Find TypeLength, 2006.10.27, refined by shien chang.
	RetrieveSegmentDataFromTCB(
		pTcb,	
		0,						// The number of buffer in bufferlist to skip.
		30, 						// The TypeLength offset. 24 for Wireless header, 6 for LLC
		TypeLengthBuf,			// 2 Byte buffer for TypeLength data.
		sizeof(TypeLengthBuf));		// Buffer length.	
		
	TypeLength=(((u2Byte)TypeLengthBuf[0])<<8) + TypeLengthBuf[1];

	if(TypeLength == 0x888e)
	{ // EAPOL packet.

		bIsSpecialPkt = TRUE;
		pTcb->specialDataType = PACKET_EAPOL;

		if(!SecIsTxKeyInstalled(Adapter, Addr1) )
		{
			pTcb->EncInfo.SecProtInfo = RT_SEC_EAPOL_BEFORE_KEY_INSTALLED;
		}
		else
		{
			pTcb->EncInfo.SecProtInfo = RT_SEC_EAPOL_AFTER_KEY_INSTALLED;
		}
	}
	else
	{ // Non-EAPOL packets.
		pTcb->EncInfo.SecProtInfo = RT_SEC_NORMAL_DATA;
	}

#if TX_TCP_SEQ_CHECK
	TxTcpSeqCheck(Adapter, pTcb, 2);
#endif


	//*************************************************************************
	// cosa added 03/12/2008
	// The following is for DHCP and ARP packet, we use cck1M to tx these packets to prevent
	// DHCP protocol fail
	// 20100309 Joseph: Revise special frame parsing in Tx path.
	if( TypeLength == 0x0800 )		// IP header
	{
		bIsSpecialPkt = CheckSpecialTxPktContentFromHighLayer(pTcb);
		if(bIsSpecialPkt)
			pTcb->specialDataType = PACKET_DHCP;
	}
	else if( TypeLength == 0x0806 )	// IP ARP packet
	{
		bIsSpecialPkt = TRUE;
		pTcb->specialDataType = PACKET_ARP;
	}

	if(bIsSpecialPkt)
	{
		// Use low rate to send DHCP packet.
		if(pMgntInfo->IOTAction & HT_IOT_ACT_WA_IOT_Broadcom)	
		{
			pTcb->DataRate = MgntQuery_TxRateExcludeCCKRates(pMgntInfo->mBrates);//0xc;//ofdm 6m
			pTcb->bTxDisableRateFallBack = FALSE;
		}
		else
		{
			pTcb->DataRate = pMgntInfo->HighestBasicRate; 
			//disable rate fallback for EAPOL packate to resolve 5G ad hoc cck hang. zhiyuan 2012/04/28
			//SUT disconnect,DUT will indication OS start association succuss,and send eapol packet in AES MODE.this will cause cck hang on 5G.
			if(IS_WIRELESS_MODE_A(Adapter) || IS_WIRELESS_MODE_N_5G(Adapter))
				pTcb->bTxDisableRateFallBack = TRUE;
		}

		RT_DISP(FDM, WA_IOT, ("DHCP TranslateHeader(), pTcb->DataRate = 0x%x\n", pTcb->DataRate));
		pTcb->bTxUseDriverAssingedRate = TRUE;

		// For delaying enter PS mode for FW control LPS. by tynli.
		pMgntInfo->LPSDelayCnt= GET_POWER_SAVE_CONTROL(pMgntInfo)->LPSAwakeIntvl*2;
		if(GET_POWER_SAVE_CONTROL(pMgntInfo)->bFwCtrlLPS &&
			GET_POWER_SAVE_CONTROL(pMgntInfo)->bLeisurePs)
		{ //by tynli
			pMgntInfo->DelayLPSLastTimeStamp = PlatformGetCurrentTime();
			
			// 20100902 Joseph: Since we send NULL frame in LeisurePSLeave() function, TX_SPINLOCK
			// shall be release for all product and OS. Originally, TX_SPINLOCK is raised twice here and
			// system hang immediately.
			if(!(pPSC->RegAdvancedLPs && (pTcb->specialDataType == PACKET_ARP)))
			{
				PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
				LeisurePSLeave(Adapter, LPS_DISABLE_TX_SPECIAL_PKT);
				PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
			}
		}
	}
	//*************************************************************************
	

	//3 Add SecurityHeader
	if(!IsSecProtEapolBeforeKeyInstalled(pTcb->EncInfo.SecProtInfo) && MgntGetEncryptionInfo(Adapter,pTcb,&pTcb->EncInfo,FALSE)
		&& !pMgntInfo->SafeModeEnabled)
	{
		pTcb->BufferList[0].Length += pSecInfo->EncryptionHeadOverhead;
		pTcb->PacketLength += pSecInfo->EncryptionHeadOverhead;
	}
	else
	{
		if(!PlatformCompareMemory(PAEGroupAddress, pTcb->DestinationAddress, ETHERNET_ADDRESS_LENGTH))
		{ // <RJ_TODO> Does this case happen after PTK installed?
			PlatformMoveMemory(pTcb->DestinationAddress, Adapter->MgntInfo.Bssid, ETHERNET_ADDRESS_LENGTH);
		}

		pTcb->BufferList[0].Length = sMacHdrLng;
	}

	RT_DISP(FQoS, QoS_INIT,  ("FillPartialHeader CurrentQosMode=%d\r\n", pMgntInfo->pStaQos->CurrentQosMode));
	
	//3 Added QoS Control
	if(!MacAddr_isMulticast(Addr1) && (pMgntInfo->pStaQos->CurrentQosMode > QOS_DISABLE ))
	{
		BOOLEAN bInsertQoS = FALSE;
		if(ACTING_AS_AP(Adapter)|| ACTING_AS_IBSS(Adapter))
		{
			pu1Byte dstaddr = (pu1Byte)pTcb->DestinationAddress;
			PRT_WLAN_STA pSTA = AsocEntry_GetEntry( pMgntInfo, dstaddr);
			
			if(pSTA!=NULL)
			{
				if(pSTA->QosMode > QOS_DISABLE )
					bInsertQoS = TRUE;
			}
		}
		else// if(pMgntInfo->mActingAsAp && pSTA!=NULL)
			bInsertQoS = TRUE;
		
		if(bInsertQoS == TRUE)
		{
			pTcb->BufferList[0].Length += sQoSCtlLng;
			pTcb->PacketLength += sQoSCtlLng;  // 2006.10.21, by Annie.

			// 2. Change SubType in Frame Control.
			SET_80211_HDR_QOS_EN(pHeader, 1);

			// 4. Use highest priority to send EPAOL-Key packet. Annie, 2005-12-22.
			// Ref: 802.11e/D13.0, 8.5.2, in page 67:
			// "When priority processing of data frames is supported, a STA's SME should send EAPOL-Key frames at the highest priority."
			if( TypeLength == 0x888e )
			{
				pTcb->priority = 7;
			}
		}
	}

	TDLS_TxTranslateHeader(Adapter, pTcb, RT_PROTOCOL_802_11);
}


#define	MAC_BUFFER0_LENGTH				200
#define	MAX_SUBFRAME_HEADER_LENGTH		18	// header 14 + padding 4


BOOLEAN
TxDoCoalescePciE(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	)
{
	return FALSE;
}


pu1Byte
GET_FRAME_OF_FIRST_FRAG(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
)
{
	return	(pu1Byte)&pTcb->BufferList[0].VirtualAddress[Adapter->TXPacketShiftBytes];
}

pu1Byte
DOT11_HEADER_FROM_TCB(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
)
{
	return pTcb->BufferList[0].VirtualAddress + Adapter->TXPacketShiftBytes;
}

VOID
GetTcbDestaddr(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	pu1Byte     *    	ppDestaddr
)
{
	if( pTcb->ProtocolType==RT_PROTOCOL_802_3 )
	{ 
		*ppDestaddr =  pTcb->BufferList[0].VirtualAddress; 
	} 
	else
	{ 
		*ppDestaddr =  pTcb->BufferList[0].VirtualAddress+4; 
	} 
	if(	TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER) || 
		TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_PACKET_SHIFT))
		*ppDestaddr += Adapter->TXPacketShiftBytes;
	
}


/*
* Attention!! We will do Coalesce if we using SW encryption!!
* But for A-MSDU, the packet length may larger than local buffer size.
* SW encryption + A-MSDU may cause error!!
*/

BOOLEAN
PreTransmitTxDoCoalesce(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	pu1Byte 				*pHeader// '*' add by ylb 20130508 for pHeader is wrong when enable AMSDU
)
{
	BOOLEAN    				bColoaseResult = TRUE;

	if(pTcb->ColoaseMethod == 2)
		bColoaseResult = TxDoCoalescePciE(Adapter, pTcb);
	else 
	{
		bColoaseResult = TxDoCoalesce(Adapter, pTcb);
	}
	
	if(!bColoaseResult)
	{
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
		return FALSE;
	}

	// pTcb->BufferList[0] is changed after coalesced.
	// 8187 must be coalesced and inerted Tx descriptor into beginning of buffer.
	*pHeader = GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb);// '*' add by ylb 20130508 for pHeader is wrong when enable AMSDU
	return TRUE;
}

BOOLEAN
PreTransmitWaitQueue(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	PBOOLEAN			bImmediateComplete
)
{
	if(!RTIsListEmpty(Get_WAIT_QUEUE(Adapter, pTcb->SpecifiedQueueID)))
	{
		RT_TRACE( COMP_SEND, DBG_TRACE, 
			("PreTransmitTCB(): there is already packets queued in wait queue(%d)\n", 
			 pTcb->SpecifiedQueueID));
		RTInsertTailListWithCnt(Get_WAIT_QUEUE(Adapter, pTcb->SpecifiedQueueID), &pTcb->List, GET_WAIT_QUEUE_CNT(Adapter,pTcb->SpecifiedQueueID));

		// If buffer is coalesced, packet should be returned immediately in Platform.
		// 2007.07.17, by shien chang.
		if ( (ACTING_AS_AP(Adapter) == FALSE) &&
			(pTcb->BufferType == RT_TCB_BUFFER_TYPE_SYSTEM) &&
			(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER)) &&
			(Adapter->NumIdleTcb < TCB_RESOURCE_THRESHOLD) )
		{
			if(!pTcb->bAggregate && !pTcb->bAMSDUProcessed)
				*bImmediateComplete = TRUE;
		}
		return TRUE;
	}
	return FALSE;
}




BOOLEAN
PreTransmitTCB(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	)
{
	//1 Note: Buffer list in pTcb may contain zero-length buffer(include first one)
	PMGNT_INFO 		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	pu1Byte 		pHeader = pTcb->BufferList[0].VirtualAddress;
	BOOLEAN 		bMicFailureReport = FALSE;
	RT_RF_POWER_STATE	rtState;
	BOOLEAN 		bCheckLocalToWds = FALSE;
	BOOLEAN	 		bImmediateComplete = FALSE;
	RT_PS_TX_STATUS		psTxStatus;
	pu1Byte			pRevAddr=NULL;
	PADAPTER 		DefAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO 		pDefaultMgntInfo =&(DefAdapter->MgntInfo);
	RT_STATUS		rtStatus;
		
	pTcb->SourceAdapt = Adapter;
	
	Adapter->ShortcutIndex = 0;

	if( Adapter->SNForShortcut >= 255 )
		Adapter->SNForShortcut = 1;
	else
		Adapter->SNForShortcut++;
	pTcb->SNForShortcut = Adapter->SNForShortcut;

	if(RT_DRIVER_STOP(Adapter))
	{
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
		return FALSE;
	}

	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));	

	if(rtState == eRfOff)
	{	// This packet should not be sent if RF is OFF, drop it !!
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
		RT_TRACE(COMP_SEND, DBG_LOUD, ("PreTransmitTCB(): RF is off, returnTCB!\n"));
		return FALSE;
	}
//Go skip sending probeRsp when scanning
//by sherry 20130117
	if((GetFirstGOPort(Adapter)!= NULL) && (MgntScanInProgress(&DefAdapter->MgntInfo)) && IsMgntProbeRsp(pTcb->BufferList[0].VirtualAddress)&& GetDefaultAdapter(Adapter)->bInHctTest)
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("===>PreTransmitTCB(): GO do not send any packet during default port is in scan\n"));
		RT_PRINT_DATA(COMP_MLME,DBG_LOUD,"send data content: \n",pTcb->BufferList[0].VirtualAddress,24);
		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
		return FALSE;
	}

	do{
		
		{
			if(pTcb->bAMSDUProcessed == FALSE)
			{

				if(!MgntFilterTransmitPacket(Adapter, pTcb) && !GET_SIMPLE_CONFIG_ENABLED(pDefaultMgntInfo))
				{	// This packet should not be sent, drop it !!
					ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
					break;
				}

				//2 Set all unspecified data to valid value
				if(pTcb->ProtocolType==RT_PROTOCOL_802_3)
				{
					RT_DISP(FQoS, QoS_INIT,  ("RT_PROTOCOL_802_3 PreTransmitTCB\n"));

					TranslateHeader(Adapter, pTcb);

					pTcb->ProtocolType=RT_PROTOCOL_802_11;
					
					// We only check if GroupTransmitKey exists when 
					// sending non-EAPPacket in Ad-Hoc mode with AES or TKIP encryption.
					// 2004.11.30, by rcnjko.
					if( pMgntInfo->mIbss && !IsSecProtEapol(pTcb->EncInfo.SecProtInfo) )
					{
						if( pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP ||
							pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP ) 
						{
							if(pSecInfo->KeyLen[pSecInfo->DefaultTransmitKeyIdx] == 0)
							{
								ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
								break;								
							}
						}
					}

					// Not to send packet if there is no key. Added by Annie , 2006-04-27.
					if( SecDropForKeyAbsent( Adapter, pTcb ) )
					{
						ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);							
						break;
					}
				
					bCheckLocalToWds = TRUE;
				}
				else
				{
					if( ACTING_AS_AP(Adapter) )
					{
						OCTET_STRING	pduOS;
						pduOS.Octet = pTcb->BufferList[0].VirtualAddress;
						pduOS.Length = (u2Byte)pTcb->BufferList[0].Length;
						CopyMem(pTcb->DestinationAddress, Frame_pDaddr(pduOS), ETHERNET_ADDRESS_LENGTH);
						CopyMem(pTcb->SourceAddress, Frame_pSaddr(pduOS), ETHERNET_ADDRESS_LENGTH);
					}

					if (pTcb->bFromUpperLayer == TRUE)
					{
						RT_DISP(FQoS, QoS_INIT,  ("Data from upper layer  FillPartialHeader\n"));
						// <SC_TODO: always data ?>
						if( pTcb->BufferList[0].VirtualAddress[0] == Type_Data )
						{
							FillPartialHeader( Adapter, pTcb );
							bCheckLocalToWds = TRUE;

							if(SecDropForKeyAbsent( Adapter, pTcb ) )
							{
								RT_TRACE(COMP_SEC,DBG_LOUD,("PreTransmit :SecDropForKeyAbsent: drop data \n"));
								ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);	
								break;
							}
						}
					}
					else
					{
						OCTET_STRING	pduOS;
						pduOS.Octet = pTcb->BufferList[0].VirtualAddress;
						pduOS.Length = (u2Byte)pTcb->BufferList[0].Length;
						CopyMem(pTcb->DestinationAddress, Frame_pDaddr(pduOS), ETHERNET_ADDRESS_LENGTH);
						CopyMem(pTcb->SourceAddress, Frame_pSaddr(pduOS), ETHERNET_ADDRESS_LENGTH);

						MgntGetEncryptionInfo(Adapter, pTcb,  &pTcb->EncInfo, FALSE);
					}
				}

				//
				// Our parser only apply to data frame from upper layer.
				// So as admission control. 2007.06.28, by shien chang.
				//
				if( IsDataFrame(pHeader) && (pTcb->bFromUpperLayer == TRUE))
				{
					BOOLEAN		bCcxCACEnable = FALSE;
					u1Byte		CurrCcxVerNumber = 0;

					CCX_QueryVersionNum(Adapter, &CurrCcxVerNumber);
					CCX_QueryCACSupport(Adapter, &bCcxCACEnable);					
						
					if((bCcxCACEnable) && CurrCcxVerNumber >= 4)
					{
						TR_ACTION			trAction = TR_ACTION_CONTINUE;
						// Parse the packet and decide which action should be take.
						trAction = GPParseTCB(Adapter, pTcb, PROTO_WLAN_80211);
						if (trAction != TR_ACTION_CONTINUE)
						{
							// Note: We assume packet were handled in GPParseTCB.
							RT_TRACE(COMP_SEND, DBG_TRACE, ("PreTransmitTCB(): Packet were handled\n"));
							if (trAction == TR_ACTION_DROPPED)
							{
								ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
							}
							break;
						}
					}

					// Admission control by SW. By Bruce, 2009-03-25.
					if(pMgntInfo->pStaQos->AcmMethod == eAcmWay2_SW)
					{
						//
						// In admission cotrolled queue, the packet should be 
						// (1) sent as the UP defined, or
						// (2) dropped, if the TSID is defined but the TS is invalid/rejected/non-used, or
						// (3) buffered, if the TSID and TS is valid but the used time exceeds the admitted time, or
						// (4) mapped to the lower UP that does not require ACM (WiFi test).
						// Note:
						//	If the pMgntInfo->pStaQos->AcmMethod is set eAcmWay2_SW, the ACM is absolutely 
						//	handled by SW and the actions follow the 4 items described of the above.
						// By Bruce, 2009-03-25.
						//
						if (QosAdmissionControl(Adapter, pTcb) )
						{
							break;
						}
					}

					if(RT_SA_QUERY_STATE_UNINITIALIZED != pSecInfo->pmfSaState && !ACTING_AS_AP(Adapter))
					{
						ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
						break;
					}
				}
				
				if(AMSDUTransmitTCB(Adapter, &pTcb, pHeader) == FALSE)
					return FALSE;
		}

		SecFillProtectTxMgntFrameHeader(Adapter, pTcb);

		// <ToDo>
		// We need to move this code to TDLS.
		if(Adapter->TDLSSupport && pTcb->bTDLPacket && pTcb->bNeedTxReport)
			{
				if(!TDLS_PrepareTxFeedback(Adapter, pTcb))
				{
					RT_TRACE_F(COMP_TDLS, DBG_WARNING, ("TDLS_PrepareTxFeedback failed!\n"));
				}
			}
			
			// Forward the frame from local machine to WDS if necessary.
			if(bCheckLocalToWds && ACTING_AS_AP(Adapter) && pMgntInfo->WdsMode != WDS_DISABLED)
			{
				AP_WdsTx(Adapter, pTcb); 
			}

			bMicFailureReport = IsSecProtMicFailureReport(pTcb->EncInfo.SecProtInfo);

			// Query management for data rate 
			pTcb->DataRate = MgntQuery_FrameTxRate( Adapter, pTcb );

			// Calculate TKIP MIC if this packet will be encrypted by HW or SW.
			if(	(IsFrameTypeData(pHeader) && !IsMgntNullFrame(pHeader)) && // Do not calc MIC for Null Function Data frame. 2005.03.23, by rcnjko.
				(!IsSecProtEapolBeforeKeyInstalled(pTcb->EncInfo.SecProtInfo)) && // Do not calc MIC for EAPOL packet before PTK installed.
				(!IsFrameWithAddr4(pHeader)) ) // Don't calc MIC for WDS frame since we don't plan to implement TKIP for WDS. 2006.06.12, by rcnjko.
			{
				SecCalculateMIC(Adapter, pTcb);
			}else if( pTcb->EncInfo.bMFPPacket )
			{
				// TKIP MFP case 
				RT_TRACE( COMP_CCX , DBG_LOUD , ("===> TKIP MFP Calculate MIC \n") );
				SecCalculateMIC(Adapter, pTcb);
			}

			RemoveZeroLengthBuffer(pTcb);
			//2 After this point, the TCB should never contain zero-length buffer
			//2 Because all zero-length buffers are reserved for header translation
			if( IsFrameTypeData(pHeader) ) 
			{
				if(RT_STATUS_SUCCESS == WAPI_SecFuncHandler(WAPI_PRETRANSMITTCB, Adapter, (PVOID)pTcb, WAPI_END))
					break;
			}

		}// end if(pTcb->bDrvAggrProcessed == FALSE)

		if(TxCheckCoalesce(Adapter, pTcb))
		{
			if(PreTransmitTxDoCoalesce(Adapter, pTcb, &pHeader) == FALSE)// '&' add by ylb 20130508 for pHeader is wrong when enable AMSDU
				break;
		}
		
		//1 Caution:	Don't add any code between coalesce and fragment.
		//1 			Because the length information may be wrong.
		if(CheckFragment(Adapter, pTcb, Adapter->TXPacketShiftBytes))	
		{
			FragmentTCB(Adapter, pTcb);
		}


		//1 Note:	In fragment case, we should use FragLength instead of PacketLength

		// Update fragment information for non-fragment case. 2005.03.08, by rcnjko.
		if(pTcb->FragCount==1)
		{			
			pTcb->FragLength[0]=(u2Byte)pTcb->PacketLength;
			pTcb->FragBufCount[0]=pTcb->BufferCount;			
		}

		//
		// Do not enter QosFillHeader if the frame is NULL frame, because the function will modify
		// the QueueID of the TCB. By Bruce, 2008-01-18.
		//
		if( IsDataFrame(pHeader) && !IsMgntNullFrame(pHeader) && (pMgntInfo->bNicVerifyPkt!=TRUE))
		{
			QosFillHeader( Adapter, pTcb );
		}

		RT_ASSERT( pTcb->SpecifiedQueueID!=UNSPECIFIED_QUEUE_ID, ("PreTransmitTCB(): %d: SpecifiedQueueID should not be UNSPECIFIED!!\n", pTcb->SpecifiedQueueID ) );

		// Fill up WEP bit and IV of each fragment in the TCB if necessary.	
		if (SecFillHeader(Adapter, pTcb))
		{
			// Do software encryption if necessary.
			if(	!pTcb->EncInfo.IsEncByHW || pSecInfo->SWTxEncryptFlag ) 
			{ 
				SecSoftwareEncryption(Adapter, pTcb);
			}
		}


		// Try to Get SNose for S5
		if( !ACTING_AS_AP(Adapter) && pMgntInfo->PowerSaveControl.WoWLANS5Support)
		{
			pu1Byte			pEapMess = NULL;
			u4Byte			EapPkLen =0;
			BOOLEAN			bEncrypt = MgntGetEncryptionInfo( Adapter, pTcb, &pTcb->EncInfo,TRUE);
			// We need call this after Coalesce , and EAPoL all need to Coalesced
			// EAPoL can't be Fragment !!
			if( IsSecProtEapol(pTcb->EncInfo.SecProtInfo) )
			{
				// Get 4-Way 2nd Packet!! EAP_HDR_LEN	
				pEapMess = pTcb->BufferList[0].VirtualAddress + EAP_HDR_LEN;
				EapPkLen	 = pTcb->PacketLength - EAP_HDR_LEN;
				if( bEncrypt )
				{
					pEapMess = pEapMess + pMgntInfo->SecurityInfo.EncryptionHeadOverhead;
					EapPkLen = EapPkLen - pMgntInfo->SecurityInfo.EncryptionHeadOverhead;
				}

				if(IsQoSDataFrame(pHeader))
				{
					pEapMess = pEapMess + sQoSCtlLng;
					EapPkLen = EapPkLen - sQoSCtlLng;
				}
				//RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "S5 EAPOL remove HDR \n", pEapMess, EapPkLen);
				if( (u1Byte)( *(pEapMess+1) ) == LIB1X_EAPOL_KEY)
				{
					OCTET_STRING	pEapKeyMess;

					pEapKeyMess.Octet = pEapMess + LIB1X_EAPOL_HDRLEN;
					pEapKeyMess.Length = (u2Byte)(EapPkLen- LIB1X_EAPOL_HDRLEN);
					

					if( Message_KeyType(pEapKeyMess) == type_Pairwise )
					{
						pu1Byte		mbS5SNose = NULL;
						u1Byte		indexSn = 0;

						mbS5SNose = pEapKeyMess.Octet + KeyNoncePos;
						for(indexSn = 0 ; indexSn < KEY_NONCE_LEN ; indexSn++)
						{
							// Check SNonse != all 0
							if( mbS5SNose[indexSn] != 0 )
							{
								PlatformMoveMemory(pMgntInfo->mbS5SNose, mbS5SNose , KEY_NONCE_LEN);
								break;
							}
						}

					}
				
				}

				RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "S5 mbS5SNose \n", pMgntInfo->mbS5SNose , KEY_NONCE_LEN);
				
			}
		}

		// <ToDo>
		// This packet is queued, and just return immediately.
		// This is a temporal solution. A known issue between tx aggregation and PS may cause TCBs corrupted.
		// Do not check-in this code into the 92SU project.
		// By Bruce, 2010-06-02.
		psTxStatus = VerifyPsOnTx(Adapter, pTcb);
		if(psTxStatus == RT_PS_TX_STATUS_PKT_DROPPED)
		{
			ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
			break;
		}
		else if(psTxStatus == RT_PS_TX_STATUS_PKT_BUFFERED)
		{
			// RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "PreTransmitTCB(): Packet Buffered pTcb->DestinationAddress: ", pTcb->DestinationAddress);
			break;
		}
		
		//
		// Map channel switch pkt to power save condition
		// TRUE: buffer pkt
		// FALSE: send pkt immediately
		if(TDLS_CS_BufferCSTx(Adapter, pTcb))
		{
			break;
		}
//move redirect Queue for TransmitTCB to here, to fix queue is error after scan
//by sherry 20130117
#if (MULTICHANNEL_SUPPORT == 1)
		{ // Redirect packets to specific queue in order to use TxPause to cease packet transmission for a client easily
			u1Byte	QueueID;
			if(
				MgntLinkStatusQuery(GetFirstGOPort(Adapter)) == RT_MEDIA_CONNECT ||
				MgntLinkStatusQuery(GetFirstClientPort(Adapter)) == RT_MEDIA_CONNECT
			)
			{

				// Redirect to Specific Queue -------------------------------------------------------

				//
				// <Roger_Notes> We should not redirect queue ID for upper Miracast application, it might cause 
				// compatibility issue for multi-queue transmission on DMP site. And we need to decide TCB priority for HW-AC mapping
				// as well to prevent incorrect TXDMA mapping, e.g., VI_QUEUE(LOW_QUEUE for Non-Qos and default specific queue in AP mode) 
				// map to QSLT_BE.
				// 2013.10.14.
				//
				if(GetDefaultAdapter(Adapter)->bInHctTest)
					QueueID = pTcb->SpecifiedQueueID = MultiChannelRedirectPacketToSpecificQueue(pTcb);
				else
					QueueID = pTcb->SpecifiedQueueID;

				// ------------------------------------------------------------------------------
				
				// SW-AC Queue and HW-AC Queue Coherence (cf. MapHwQueueToFirmwareQueue) ---
				switch(QueueID)
				{
					case BE_QUEUE:	pTcb->priority = QSLT_BE;	break;
					case BK_QUEUE:	pTcb->priority = QSLT_BK;	break;
					case VI_QUEUE:	pTcb->priority = QSLT_VI;	break;
					case VO_QUEUE:	pTcb->priority = QSLT_VO;	break;
				}
				// ------------------------------------------------------------------------
			}
			RT_TRACE(COMP_MULTICHANNEL, DBG_TRACE,("pretransmitTCB: pTcb->SpecifiedQueueID = %d \n",pTcb->SpecifiedQueueID));

			//
			// Drop miracast RTP packet if necessary
			//
			{
				PMULTICHANNEL_PORT_CONTEXT pPortContext = (PMULTICHANNEL_PORT_CONTEXT)(pTcb->SourceAdapt->MultiChannel); 
				if(TEST_FLAG(pPortContext->FCSPortState, FCS_STATE_PKT_DROPPED) && (TX_IPTYPE_UDP ==  pTcb->IPType) )
				{
					ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
					break;
				}
			}
		}
#endif
	
		if(pHalData->bAutoAMPDUBurstMode && pTcb->SpecifiedQueueID < BEACON_QUEUE)
		{
			if(pMgntInfo->LinkDetectInfo.bBusyTrafficAccordingTP && pMgntInfo->bMediaConnect && (Adapter->bInHctTest==FALSE))
			{
				if( IsFrameTypeData(pHeader))
				{				
					RTInsertTailListWithCnt(Get_WAIT_QUEUE(Adapter, pTcb->SpecifiedQueueID), &pTcb->List, GET_WAIT_QUEUE_CNT(Adapter,pTcb->SpecifiedQueueID));
					return FALSE;
				}			
			}

			if(!pMgntInfo->LinkDetectInfo.bBusyTrafficAccordingTP && (*GET_WAIT_QUEUE_CNT(Adapter,pTcb->SpecifiedQueueID))> 0)
			{
				HAL_HW_TIMER_TYPE TimerType = HAL_TIMER_EARLYMODE;
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_HW_REG_TIMER_RESTART,  (pu1Byte)(&TimerType));
			}		
		}
	
		// <RJ_TODO> 
		// 1. We should insert the packet into tail of TcbWaitQueue, 
		// and transmit it TxHandleInterrupt() or UsbIoCompeleteSentXXX() 
		// with TransmitTCB().
		// 2. Other conditions:
		// Out of TCB, TxDesc, NetworkSetupInProgress(). 
		if(	(IsFrameTypeData(pHeader) &&  
			
			((pMgntInfo->bScanInProgress) || GET_HAL_DATA(Adapter)->bNeedQueuePacketInIQKProgress) &&  // Marked: To shorten the Ping roundtrip time. Use TxPause to hold packets.

			!IsMgntNullFrame(pHeader) &&
			!pMgntInfo->bScanWithMagicPacket ) ||
			FwLPSCheckChangeTxBoundary(Adapter)) // For WOL feature, we must pass data frame in this case. 2005.06.27, by rcnjko.
		{

			// We must queue the packet to send during scanning, otherwise
			// scan with ping will loss packet. 2004.11.30, by rcnjko.
			RTInsertTailListWithCnt(Get_WAIT_QUEUE(Adapter, pTcb->SpecifiedQueueID), &pTcb->List,GET_WAIT_QUEUE_CNT(Adapter,pTcb->SpecifiedQueueID));

			// If buffer is coalesced, packet should be returned immediately in Platform.
			// 2007.07.17, by shien chang.
			if ( (ACTING_AS_AP(Adapter) == FALSE) &&
				(pTcb->BufferType == RT_TCB_BUFFER_TYPE_SYSTEM) &&
				(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER)) &&
				(Adapter->NumIdleTcb < TCB_RESOURCE_THRESHOLD) )
			{
				{
					if(!pTcb->bAggregate && !pTcb->bAMSDUProcessed)
						bImmediateComplete = TRUE;
				}
			}
			break;
		}

		// Buffer this MPDU if there is already packets in wait queue. 2006.09.22, by rcnjko.
		if(PreTransmitWaitQueue(Adapter, pTcb, &bImmediateComplete) == TRUE)
			break;

		if(GET_HAL_DATA(Adapter)->AMPDUBurstMode == RT_AMPDU_BRUST_92D || 
			(GET_HAL_DATA(Adapter)->AMPDUBurstMode == RT_AMPDU_BRUST_8814A )||
			(GET_HAL_DATA(Adapter)->AMPDUBurstMode == RT_AMPDU_BRUST_8822B))
		{
			GetTcbDestaddr(Adapter, pTcb, &pRevAddr);
			// Check if the A-MPDU aggreagtion is allowed and get its aggregation capabilities/
			MgntQuery_AggregationCapability(Adapter, pRevAddr, pTcb);
			//If we use 92D early mode, just insert into WaitQueue for Qos Data. pTcb->bAMPDUEnable not used.
			if( pTcb->bAMPDUEnable && Adapter->EarlyMode_QueueNum[pTcb->priority] > Adapter->EarlyMode_Threshold)
			{
				RTInsertTailList(&Adapter->TcbWaitQueue[pTcb->SpecifiedQueueID], &pTcb->List);
				RT_TRACE(COMP_SEND,DBG_LOUD,("InsertTailList(&Adapter->TcbWaitQueue--3.Adapter->EarlyMode_QueueNum[%d]:%d\n",pTcb->priority,Adapter->EarlyMode_QueueNum[pTcb->priority]));
				break;
			}
		}

		if( IsFrameTypeData(pHeader) ) 
		{
			// Start Timer for LED Tx/Rx blinking
			if(Adapter->bInHctTest)
			{
				Adapter->HalFunc.LedControlHandler( Adapter, LED_CTL_TX);
			}
			else
			{
				if (Adapter->LedTxCnt == 0)
					PlatformSetTimer(Adapter, &pMgntInfo->LedTimer, 50);
			}
			
			Adapter->LedTxCnt++;
		}

		//
		// NOTE! DO NOT ACCESS pTcb after TransmitTCB() return sucessfully.
		// Otherwise, it will cause Tx dead locked at USB path.
		// 061122, by rcnjko.
		//		
		rtStatus = TransmitTCB(Adapter, pTcb);
		if(RT_STATUS_SUCCESS != rtStatus && RT_STATUS_PKT_DROP != rtStatus)
		{			
			RT_TRACE(COMP_SEND, DBG_LOUD, ("TransmitTcb fail\n"));
			RTInsertTailListWithCnt(Get_WAIT_QUEUE(Adapter, pTcb->SpecifiedQueueID), &pTcb->List, GET_WAIT_QUEUE_CNT(Adapter,pTcb->SpecifiedQueueID));

			// If buffer is coalesced, packet should be returned immediately in Platform.
			// 2007.07.17, by shien chang.
			if ( (ACTING_AS_AP(Adapter) == FALSE) &&
				(pTcb->BufferType == RT_TCB_BUFFER_TYPE_SYSTEM) &&
				(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER)) &&
				(Adapter->NumIdleTcb < TCB_RESOURCE_THRESHOLD) )
			{
				{
					if(!pTcb->bAggregate && !pTcb->bAMSDUProcessed)
					{
						//pTcb->Reserved = NULL;
						bImmediateComplete = TRUE;
					}
				}
				//return TRUE;
			}
		}

		//
		// 041005, rcnjko: 
		// Disassociate from the AP after MIC failure report sent 
		// if twice MIC error occured in 60 seconds. 
		//
		if(	bMicFailureReport && // Replaced with a local varible to prevent access pTcb after TransmitTCB(), 061122, by rcnjko.
			pSecInfo->bToDisassocAfterMICFailureReportSend )		// Modified by Annie for WiFi WPA2 Test in SGS, 2006-06-14.
		{
			// For debug purpose. 
			RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "PreTransmitTCB(): Send MIC Report", pTcb->BufferList[0].VirtualAddress, pTcb->BufferList[0].Length);

			// Reset bToDisassocAfterMICFailureReportSend.
			// <RJ_TODO> bToDisassocAfterMICFailureReportSend has better to be reset 
			// if the MIC failure report is not sent by upper layer after a period.
			pSecInfo->bToDisassocAfterMICFailureReportSend = FALSE;	
			
			// Disassociate from AP. 
			// Note that, we must relase Tx spinlock before calling MgntActSet_802_11_DISASSOCIATE()
			// because SendDisassociation() will acquire it again, which will cause OS hang. 
			// 2005.08.02, by rcnjko.
			PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
			MgntActSet_802_11_DEAUTHENTICATION(Adapter, mic_failure);	// Modified for WiFi Mandatory Testplan v1.2: We shall send Deauthentication packet instead of Disassociation packet. Annie, 2006-05-04.
			PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
		}
	}while(FALSE);
	return bImmediateComplete;
}

BOOLEAN
TxCheckCoalesce(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	)
{
	return TRUE;
}

BOOLEAN
TxDoCoalesce(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	)
{
	u2Byte				i ,BytesCopied=0;
	u2Byte				FragThreshold, FragLength, Offset, Length;
	u1Byte				QosCtrlLen = 0;		// Added by Annie, 2006-01-08.
	BOOLEAN				bFragment;
	PRT_TX_LOCAL_BUFFER	pCoalseceBuffer;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);

	//2 Note: Fragment header and encryption header should be reserved here.

	pCoalseceBuffer=GetLocalBuffer(Adapter);
	if(pCoalseceBuffer==NULL)
		return FALSE;

	// Note that! we have not yet reserved Tx Desc for 8187, so header offset is 0.
	bFragment=CheckFragment(Adapter, pTcb, 0); 
	BytesCopied = Adapter->TXPacketShiftBytes;

	// Consider QoS data frame. Added by Annie, 2006-01-08.
	if( IsQoSDataFrame( pTcb->BufferList[0].VirtualAddress ) )
	{
		QosCtrlLen = sQoSCtlLng;
	}

	//2 Coalesce data
	if(!bFragment)
	{
		for(i=0;i<pTcb->BufferCount;i++)
		{
			PlatformMoveMemory(
				pCoalseceBuffer->Buffer.VirtualAddress+BytesCopied, 
				pTcb->BufferList[i].VirtualAddress, 
				pTcb->BufferList[i].Length);
			BytesCopied += (u2Byte)pTcb->BufferList[i].Length;
		}

		RT_ASSERT(BytesCopied==(pTcb->PacketLength+Adapter->TXPacketShiftBytes), ("Coalesce length mismatch !!\n"));
	}
	else
	{
		FragThreshold=FRAGMENT_THRESHOLD(Adapter);
		Offset=0;
		FragLength=0;
		for(i=0;i<pTcb->BufferCount;)
		{
			if(((pTcb->BufferList[i].Length - Offset) + FragLength) > FragThreshold)
				Length=FragThreshold - FragLength;
			else
				Length=(u2Byte)(pTcb->BufferList[i].Length - Offset);

			PlatformMoveMemory(
				pCoalseceBuffer->Buffer.VirtualAddress+BytesCopied,
				pTcb->BufferList[i].VirtualAddress + Offset,
				Length);
			BytesCopied += Length;
			FragLength += Length;
			Offset += Length;

			if(Offset==pTcb->BufferList[i].Length)
			{
				Offset=0;
				i++;
			}

			if(FragLength==FragThreshold && i!=pTcb->BufferCount)
			{
				BytesCopied += Adapter->TXPacketShiftBytes;

				// Add for ICV or MIC , 2007/12/28 by CCW
				BytesCopied += pMgntInfo->SecurityInfo.EncryptionTailOverhead;

				BytesCopied += (sMacHdrLng + QosCtrlLen + pMgntInfo->SecurityInfo.EncryptionHeadOverhead);
				FragLength = (sMacHdrLng + QosCtrlLen + pMgntInfo->SecurityInfo.EncryptionHeadOverhead);
			}
		}
	}
	
	//2 Modify buffer list
	pTcb->BufferList[0]=pCoalseceBuffer->Buffer;
	pTcb->BufferList[0].Length=BytesCopied;
	pTcb->BufferCount=1;

	pTcb->pCoalesceBuffer=pCoalseceBuffer;
	SET_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER);

	return TRUE;
}

BOOLEAN
CheckFragment(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	int					nHdrOffset // This is the offset of 802.11 header, e.g. 8185=>0, 8187=>USB_HWDESC_HEADER_LEN.
	)
{
	PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;
	pu1Byte pHeader = NULL;
	pu1Byte pRaddr = NULL; 

	pHeader = pTcb->BufferList[0].VirtualAddress+ nHdrOffset;

	 pRaddr = pHeader + 4; 

	
	if(pTcb->bBTTxPacket)
		return FALSE;

	//2 Check length
	if(pTcb->PacketLength <= FRAGMENT_THRESHOLD(Adapter))
	{
		return FALSE;
	}

	//2 Check multicast/broadcast
	if(MacAddr_isMulticast(pRaddr))
	{
		return FALSE;
	}

	// Don't perform fragmentation on WDS frame.
	if(IsFrameWithAddr4(pHeader))
	{
		return FALSE;
	}

	//2 Check A-MPDU and A-MSDU aggregation condition
	if(	pMgntInfo->pHTInfo->bCurrentHTSupport &&
		(pMgntInfo->pHTInfo->bCurrentAMPDUEnable ||
		pMgntInfo->pHTInfo->bCurrent_AMSDU_Support	 ||
		pMgntInfo->pHTInfo->ForcedAMPDUMode  == HT_AMPDU_ENABLE ||
		(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter)))
		)
	{
		if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
		{
			PRT_WLAN_STA pEntry = AsocEntry_GetEntry( pMgntInfo,  pTcb->DestinationAddress);
			if(pEntry != NULL)
			{	
				if(pEntry->IOTAction & HT_IOT_ACT_AMSDU_ENABLE)	
					return FALSE;
				else if(pEntry->IOTAction & HT_IOT_ACT_AMSDU_AMPDU)
					return FALSE;
			}	
		}
		else
			return FALSE;
	}

	// Don't do fragmentation on EAP Packet
	if( IsSecProtEapol(pTcb->EncInfo.SecProtInfo)== TRUE )
	{
		return FALSE;
	}

	return TRUE;
}

VOID
FragmentTCB(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	)
{
	u2Byte			i, AddLen, Length, Offset=0;
	u2Byte			FragThreshold;
	u2Byte			FragIndex=0, FragLen, FragBufferCount;
	u2Byte			ReadBufferIndex, WriteBufferIndex=0;
	u2Byte			BufferCount;
	SHARED_MEMORY	CoalesceBuffer, BufferList[MAX_PER_PACKET_BUFFER_LIST_LENGTH];
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	u1Byte			QosCtrlLen = 0;		// Added by Annie, 2006-01-09.

	if(CheckFragment(Adapter, pTcb, Adapter->TXPacketShiftBytes) == FALSE)
		return;

	FragThreshold=FRAGMENT_THRESHOLD(Adapter);

	if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
	{	// Coalesce case
		//2 Note: In this case, header and encryption overhead is already reserved.

		CoalesceBuffer=pTcb->BufferList[0];

		FragThreshold += Adapter->TXPacketShiftBytes;

		do{
			if((CoalesceBuffer.Length-Offset) > FragThreshold)
			{

				Length = FragThreshold;

			}
			else
			{
				Length=(u2Byte)(CoalesceBuffer.Length-Offset);
			}
			
			GET_SHARED_MEMORY_WITH_OFFSET(
				&CoalesceBuffer, 
				&pTcb->BufferList[FragIndex], 
				Offset, 
				Length);

			pTcb->FragLength[FragIndex]=Length;
			pTcb->FragBufCount[FragIndex]=1;
			
			Offset+=Length;

			// offset ICV or MIC fild ,2007/12/28 by CCW
			Offset += pMgntInfo->SecurityInfo.EncryptionTailOverhead;
			
			FragIndex++;
		}while(Offset<CoalesceBuffer.Length);
		
		pTcb->FragCount=FragIndex;
		pTcb->BufferCount=FragIndex;
	}
	else
	{
		// Consider QoS data frame. Added by Annie, 2006-01-09.
		if( IsQoSDataFrame( pTcb->BufferList[0].VirtualAddress ) )
		{
			QosCtrlLen = sQoSCtlLng;
		}

		//2 Duplicate buffer list
		BufferCount=pTcb->BufferCount;
		PlatformMoveMemory(BufferList, pTcb->BufferList, BufferCount*sizeof(SHARED_MEMORY));

		FragLen=0;
		FragBufferCount=0;

		//2 Get new buffer list and generate fragment information
		for(ReadBufferIndex=0; ReadBufferIndex < BufferCount;)
		{
			if(FragLen + BufferList[ReadBufferIndex].Length > FragThreshold)
			{
				AddLen=FragThreshold-FragLen;
				
				pTcb->BufferList[WriteBufferIndex]=BufferList[ReadBufferIndex];
				pTcb->BufferList[WriteBufferIndex].Length=AddLen;
				
				MAKE_SHARED_MEMORY_OFFSET_AT_FRONT(
					&BufferList[ReadBufferIndex],
					AddLen);
			}
			else
			{
				pTcb->BufferList[WriteBufferIndex]=BufferList[ReadBufferIndex];
				ReadBufferIndex++;
			}
			FragLen +=(u2Byte)pTcb->BufferList[WriteBufferIndex].Length;
			WriteBufferIndex++;
			FragBufferCount++;

			if(FragLen==FragThreshold || ReadBufferIndex == BufferCount)
			{
				pTcb->FragLength[FragIndex]=FragLen;
				pTcb->FragBufCount[FragIndex++]=FragBufferCount;

				if(ReadBufferIndex != BufferCount)
				{	//2 Insert header for 2~ fragment
					pTcb->Header[FragIndex].Length=sMacHdrLng+QosCtrlLen+pMgntInfo->SecurityInfo.EncryptionHeadOverhead;
					pTcb->BufferList[WriteBufferIndex++]=pTcb->Header[FragIndex];
				
					FragLen=sMacHdrLng+QosCtrlLen+pMgntInfo->SecurityInfo.EncryptionHeadOverhead;
					FragBufferCount=1;
				}
			}
		}
		pTcb->FragCount=FragIndex;
		pTcb->BufferCount=WriteBufferIndex;
	}

	//2 Duplicate header and set fragment number and more frag
	SET_80211_HDR_MORE_FRAG(pTcb->BufferList[0].VirtualAddress+Adapter->TXPacketShiftBytes, 1); // Set more frag

	FragIndex=pTcb->FragBufCount[0];
	for(i=1;i<pTcb->FragCount;i++)
	{
		PlatformMoveMemory(
			pTcb->BufferList[FragIndex].VirtualAddress + Adapter->TXPacketShiftBytes, 
			pTcb->BufferList[0].VirtualAddress + Adapter->TXPacketShiftBytes,
			sMacHdrLng);

		if(i==(pTcb->FragCount-1))
		{
			SET_80211_HDR_MORE_FRAG(pTcb->BufferList[FragIndex].VirtualAddress+Adapter->TXPacketShiftBytes, 0);	// Clear more frag for last fragment
		}
		
		SET_80211_HDR_FRAGMENT(pTcb->BufferList[FragIndex].VirtualAddress+Adapter->TXPacketShiftBytes, (u1Byte)i);

		FragIndex+=pTcb->FragBufCount[i];
	}
}

/*-----------------------------------------------------------------------------
// Procedure:   Transmit one packet 
//
    Description:   	Emily 2006/04/30, For 8190 PCI
				When configure tx descriptor under 8190Pci hardware, reserve the 
				first one descriptor for Firmware Info(not insert, cause the descriptor 
				address nust be contineous). Then, fill descriptor for each packet 
				buffer from the second descriptor. Configure first descriptor and 
				pTcb->nDescUsed++ before set Own bit.

				Joseph 2006/10/13, Revision
				Integrate the procedure of protection mode determination and duration 
				calculation into single function respectively.
				TransmitTcb() handle the information of each MSDU.
				For further improvement, we can add an function like "HandleMPDUInfo"
				to handle specific information of every fragment of MSDU. But now there 
				are only TxTime calculation is specific to each MPDU, so just leave this 
				idea for future extension.
	Return value:
				TransmitTCB returns RT_STATUS_RESOURCE if TransmitTCB occur some error and cannot Tx.
				TransmitTCB returns RT_STATUS_PKT_DROP if The current link was teardown, and need to drop packet here.
				TransmitTCB returns RT_STATUS_SUCCESS if TransmitTCB OK.
//-----------------------------------------------------------------------------*/
RT_STATUS
TransmitTCB(
	PADAPTER			pAdapter,
	PRT_TCB				pTcb
	)
{
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);

	s2Byte		i;
	u2Byte		firstDesc=0,curDesc=0;
	u2Byte		FragIndex=0, FragBufferIndex=0, FragBufCount=0;
	u2Byte		SequenceNumber = 0;
	pu1Byte 	pFrame = GET_FRAME_OF_FIRST_FRAG(pAdapter, pTcb);
	u1Byte		QueueID = pTcb->SpecifiedQueueID;
	pu1Byte		pRecvAddr = NULL; 
	s2Byte		BufferCount;
	RT_RF_POWER_STATE 	rfState;
	PADAPTER 	Adapter =  GetDefaultAdapter(pAdapter);


	pMgntInfo = &(Adapter->MgntInfo);	
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));

	if(rfState != eRfOn)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("TransmitTCB return by rfoff, return RT_STATUS_RESOURCE\n"));
		return RT_STATUS_RESOURCE;
	}
	else if (Adapter->ResetProgress != RESET_TYPE_NORESET)
	{ 	//cosa add for debug use. 08/23/2007
		return RT_STATUS_RESOURCE;
	}

{	//tynli_test_32k 2011.02.25.
	u1Byte	FwPSState;
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_FW_PS_STATE, &FwPSState);		
	if(IS_IN_LOW_POWER_STATE(pAdapter, FwPSState))
	{		
		RT_TRACE(COMP_SEND, DBG_LOUD, ("TransmitTCB(): CANNOT TX---> Wake up Hw. return RT_STATUS_RESOURCE\n"));
		if( PlatformIsWorkItemScheduled(&(pMgntInfo->PowerSaveControl.FwPsClockOnWorkitem)) == FALSE)
		{
			RT_TRACE(COMP_SEND, DBG_TRACE, ("ScheduleWorkItem -> FwPsClockOnWorkitem.\n"));
			PlatformScheduleWorkItem(&(pMgntInfo->PowerSaveControl.FwPsClockOnWorkitem));
		}
		return RT_STATUS_RESOURCE;
	}
}

	Adapter->TxStats.NumTransmitTCBCalled[QueueID]++;

	GetTcbDestaddr(Adapter, pTcb, &pRecvAddr);

	//
	//<Roger_Notes> Revise following condition to check specific Tx queue for aggregation case.
	//
#if TX_AGGREGATION
	if(FALSE == pTcb->outPending)
	{
		if(FALSE == PlatformIsTxQueueAvailable(Adapter, QueueID, pTcb->BufferCount))
		{
			Adapter->TxStats.NumTransmitTCBNotAvailable[QueueID]++;
			return RT_STATUS_RESOURCE;
		}
	}

	if(TRUE == pTcb->isUsbTxAgg){
		Adapter->HalFunc.TxFillDescriptorHandler(Adapter, pTcb, 0, 0, 0, 0);
		return RT_STATUS_SUCCESS;
	}
	
#else
	if(FALSE == PlatformIsTxQueueAvailable(Adapter, QueueID, pTcb->BufferCount))
	{
		Adapter->TxStats.NumTransmitTCBNotAvailable[QueueID]++;
		return RT_STATUS_RESOURCE;
	}
#endif
 

	// Note:
	//	To prevent lossing tx report from the HW restriction, make sure only one tcb 
	//	with tx feedback info in HW queue.
	if(TxFeedbackIsTxFeedbackInfoInstalled(pTcb))
	{
		// If this tcb needs tx report, check all queues if there is one tcb with tx
		// feedback already in HW tx queues.
		if(TxFeedbackExistActiveTxFeedbackInfo(Adapter))
		{
			RT_TRACE_F(COMP_CMD, DBG_LOUD, ("Already one tcb with tx feedback in Tx Queue, return FALSE to insert this TCB into wait queue! return RT_STATUS_RESOURCE\n"));
			return RT_STATUS_RESOURCE;
		}
	}
	
	if(RT_IN_PS_LEVEL(Adapter, RT_RF_LPS_DISALBE_2R))
	{
		BOOLEAN		Disable2R = FALSE;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_RF_2R_DISABLE, (pu1Byte)&Disable2R);
	}

	//
	// Fill the settings into TCB
	//
	pTcb->bBroadcast	= MgntIsBroadcastFrame( Adapter, pTcb );
	pTcb->bMulticast 	= MgntIsMulticastFrame( Adapter, pTcb );
	pTcb->bEncrypt = pTcb->EncInfo.IsEncByHW;

	{
		MgntQuery_PreambleMode(Adapter, pTcb);

		MgntQuery_TxRateSelectMode(pTcb->SourceAdapt, pTcb);

		if(pTcb->macId == MAC_ID_STATIC_UNSPECIFIED_MACID)
		{
			RT_TRACE_F(COMP_CMD, DBG_LOUD, ("Return TRUE because Entry was release and the data frame needs to drop, return RT_STATUS_RESOURCE\n"));
			ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
			return RT_STATUS_PKT_DROP;
		}//if	

		// Check if the A-MPDU aggreagtion is allowed and get its aggregation capabilities/
		MgntQuery_AggregationCapability(pTcb->SourceAdapt, pRecvAddr, pTcb);

		/* 2007/03/06 MH This function check if short GI is enabled, and current 
		transmit bandwidth for TX firmware info in RTL8190. */
		MgntQuery_ShortGI(pTcb->SourceAdapt, pTcb);

		// Get the state if this packet sent by LDPC
		MgntQuery_Tx_LDPC(pTcb->SourceAdapt, pTcb);

		// Get the state if this packet sent by STBC
		MgntQuery_Tx_STBC(pTcb->SourceAdapt, pTcb);

		// Choose Bandwidth and sub-channel settings
		MgntQuery_BandwidthMode(pTcb->SourceAdapt, pTcb);

		// Check if 802.11z is supported
		if(pTcb->SourceAdapt->TDLSSupport)
			TDLS_QueryCapability(pTcb->SourceAdapt, pTcb);

		

		//
		// This function will determine if we need to send protection frame and how to send protection frame.
		// And save the result in TCB.
		// Although protection mode is determined for each MSDU. This function will use the first fragment of
		// the MSDU to make decision and apply the decision to other fragment in this MSDU.
		//
		MgntQuery_ProtectionFrame(pTcb->SourceAdapt, pTcb);

	}
	//
	// Sequence number should be the same for every fragment of this packet. 2005.04.14, by rcnjko.
	// Management frame and Legacy Data frame use the same number sequence.
	// Qos Data frame will use individual number sequence according to destination address and TID.
	//
	SequenceNumber = MgntQuery_SequenceNumber(
						pTcb->SourceAdapt, 
						pFrame,
						pTcb->bBTTxPacket,
						pRecvAddr,
						pTcb->priority
						);
	//
	// This function will calculate the duration for each fragmentation(MPDU) of this MSDU.
	// The result will save in TCB.
	//
	MgntQuery_TxTime(pTcb->SourceAdapt, pTcb);

	//
	// Filling the information into frame header.
	// Duration field and SeqNum field is now handled in this function.
	//
	FillFrameField(Adapter, pTcb, SequenceNumber);
					
#if WLAN_ETW_SUPPORT
	//
	// Assign sequence number
	//
	pTcb->TcbSequence = SequenceNumber;
#endif

	//cosa add for debug use. 08/23/2007
	Adapter->TxStats.NumTransmitTCBAvailable[QueueID]++;
	
	//
	// Fill Tx Descriptor
	//
	if(IS_RW_PTR(Adapter))
		firstDesc=0;
	else
		firstDesc = curDesc = Adapter->NextTxDescToFill[QueueID];
	
	pTcb->nDescUsed=0;
	pTcb->nFragSent = 0;
	pTcb->nFragCompleted = 0;
	Adapter->nFragDescUsed=0;

	BufferCount = pTcb->BufferCount;	

	for(i=0;i< pTcb->BufferCount;i++)
	{
		BOOLEAN bFirstSeg, bLastSeg;

		if(IS_RW_PTR(Adapter))
		{
			if(FragBufferIndex == 0)
			{
				FragBufCount = pTcb->FragBufCount[FragIndex];
				pTcb->nDescUsed++;
				Adapter->nFragDescUsed++;
			}
		}
		else
		{
			if(FragBufferIndex == 0)
				FragBufCount = pTcb->FragBufCount[FragIndex];
			Adapter->nFragDescUsed = 1;
			pTcb->nDescUsed++;
		}

#if !defined(MUTIPLE_BULK_OUT)
		//
		// 050413, rcnjko: 
		// For 8187, we must keep at most one Irp Pending in each endpoint,
		// so, we send one fragment in TransmitTCB() and will handle other fragment 
		// in the complete handler of this endpoint.
		//
		if(pTcb->nDescUsed == 1)
#endif
		{
			if(FragBufferIndex == (FragBufCount-1))
				pTcb->nFragSent++;

			if(IS_RW_PTR(Adapter))
				curDesc = Adapter->CurrentTXWritePoint[QueueID];
	
#if WLAN_ETW_SUPPORT

			//
			// <Roger_Notes> No activity needs to be associated with this event, the ActivityId is optional and can be NULL.
			// 2014.01.14.
			//
			EventWriteTxSubmissionToNIC(
				NULL, //Without associated with the event
				pTcb->TcbFrameUniqueueID, //FrameUniqueueID
				0, 	// QueueLength
				0, 	// QueueState
				0,	// Status
				0,	// CustomData1
				0,	// CustomData2
				0);	// CustomData3
#endif

			Adapter->HalFunc.TxFillDescriptorHandler(Adapter, 	pTcb, i, FragIndex,  FragBufferIndex, curDesc);
		}

		bFirstSeg	= (FragBufferIndex == 0);
		bLastSeg	= (FragBufferIndex==(pTcb->FragBufCount[FragIndex]-1) );

		FragBufferIndex++;
		if(FragBufferIndex == FragBufCount)
		{
			FragIndex++;
			FragBufferIndex=0;
		}

		if(!IS_RW_PTR(Adapter))
			curDesc=(curDesc+1)%Adapter->NumTxDesc[QueueID];
		
	}

	return RT_STATUS_SUCCESS;
}





VOID
ResetTxStatistics(
	PADAPTER	Adapter
	)
{
	PlatformZeroMemory( &Adapter->TxStats, sizeof(RT_TX_STATISTICS) );
}



VOID
CountTxStatistics(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	RT_RF_POWER_STATE 	rtState;

	Adapter->HalFunc.GetHwRegHandler(GetDefaultAdapter(Adapter), HW_VAR_RF_STATE, (pu1Byte)(&rtState));

	// When RF is off, we should not count the packet for hw/sw synchronize
	// reason, ie. there may be a duration while sw switch is changed and hw
	// switch is being changed. 2006.12.04, by shien chang.
	if (rtState == eRfOff)
	{
		return;
	}
	
	if( pTcb->status.TOK )
	{

		//Just record data packets to reduce the traffic flow. 2009.02.02 by tynli.
		if(MgntIsDataOnlyFrame(Adapter, pTcb))
		{
			Adapter->TxStats.NumTxOkTotal++;
			if((pMgntInfo->StaCount > 0)&&(ACTING_AS_AP(Adapter)))
				Adapter->TxStats.NumTxOkBytesTotal += pTcb->PacketLength;
			else if(!ACTING_AS_AP(Adapter))
				Adapter->TxStats.NumTxOkBytesTotal += pTcb->PacketLength;
			pMgntInfo->LinkDetectInfo.NumTxOkInPeriod++;
		}

		if(MgntIsMulticastFrame(Adapter, pTcb))
		{
			Adapter->TxStats.NumTxMulticast++;
			Adapter->TxStats.NumTxBytesMulticast += pTcb->PacketLength;
		}
		else if(MgntIsBroadcastFrame(Adapter, pTcb))
		{
			Adapter->TxStats.NumTxBroadcast++;
			Adapter->TxStats.NumTxBytesBroadcast += pTcb->PacketLength;
		}
		else
		{
			Adapter->TxStats.NumTxUnicast++;
			Adapter->TxStats.NumTxBytesUnicast += pTcb->PacketLength;

			TDLS_CountTxStatistics(Adapter, pTcb);
		}
	}
	else
	{
		Adapter->TxStats.NumTxErrTotal++;
		Adapter->TxStats.NumTxErrBytesTotal += pTcb->PacketLength;

		if(MgntIsMulticastFrame(Adapter, pTcb))
		{
			Adapter->TxStats.NumTxErrMulticast++;
		}
		else if(MgntIsBroadcastFrame(Adapter, pTcb))
		{
			Adapter->TxStats.NumTxErrBroadcast++;
		}
		else
		{
			Adapter->TxStats.NumTxErrUnicast++;
		}
	}
	Adapter->TxStats.NumTxRetryCount += pTcb->status.DataRetryCount;

}

/*
	Function:UpdateEarlyModeInfo
	Decide the packet whether using Early Mode and the max mumber of packet can be pre-connected.
	Just for 92D.by wl.
*/
BOOLEAN
UpdateEarlyModeInfo(
	IN	PADAPTER			Adapter,
	IN	PRT_TCB				pTcb,
	IN	PRT_LIST_ENTRY		plistHead
	)
{
	PRT_TCB  		pNextTcb=NULL;
	PRT_LIST_ENTRY 	pEntry=NULL;
	pu1Byte 			pFrame = GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb);
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	RT_ENC_ALG		ulSwitchCondition =pMgntInfo->SecurityInfo.PairwiseEncAlgorithm;
	u1Byte			additionLen=0, maxPktNum = EARLY_MODE_MAX_PKT_NUM;

	switch(GET_HAL_DATA(Adapter)->AMPDUBurstMode){
		case RT_AMPDU_BRUST_92D:
			maxPktNum = EARLY_MODE_MAX_PKT_NUM_92D;
			break;
		case RT_AMPDU_BRUST_88E:
			maxPktNum = EARLY_MODE_MAX_PKT_NUM_88E;
			break;
		case RT_AMPDU_BRUST_8812_4:
			maxPktNum = 4;
			break;
		case RT_AMPDU_BRUST_8812_8:
			maxPktNum = 8;
			break;
		case RT_AMPDU_BRUST_8812_12:
			maxPktNum = 12;
			break;
		case RT_AMPDU_BRUST_8812_15:
			maxPktNum = EARLY_MODE_MAX_PKT_NUM_8812;
			break;
		case RT_AMPDU_BRUST_8723B:
			maxPktNum = EARLY_MODE_MAX_PKT_NUM_8723B;
			break;
		case RT_AMPDU_BRUST_8814A:
			maxPktNum = EARLY_MODE_MAX_PKT_NUM_8814A;
			break;
		case RT_AMPDU_BRUST_8822B:
			maxPktNum = EARLY_MODE_MAX_PKT_NUM_8822B;
			break;
		default:
			maxPktNum = 0;
			break;	
	}

	if(GET_HAL_DATA(Adapter)->AMPDUBurstNum < maxPktNum)
		maxPktNum = GET_HAL_DATA(Adapter)->AMPDUBurstNum;

	switch( ulSwitchCondition )
	{
		case RT_ENC_ALG_NO_CIPHER:
			additionLen = 4;
			break;

		case RT_ENC_ALG_WEP40:
		case RT_ENC_ALG_WEP104:
			additionLen = 8;
			break;

		case RT_ENC_ALG_TKIP:
			additionLen = 16;
			break;

		case RT_ENC_ALG_AESCCMP:
			additionLen = 12;
			break;

		case RT_ENC_ALG_SMS4:
			additionLen = 20;
			break;		

		case RT_ENC_ALG_WEP:
		default:
			break;		
	}

	if(MacAddr_isBcst(pTcb->DestinationAddress) ||MacAddr_isMulticast(pTcb->DestinationAddress) || !IsQoSDataFrame(pFrame))
		return FALSE;

	// Prefast warning 6011
	if(plistHead != NULL && RTIsListEmpty(plistHead))
		return FALSE;
	
	// Prefast warning 6011
	if (plistHead != NULL)
	{
		pEntry = RTGetHeadList(plistHead);
	}
	
	pTcb->EMPktNum = 0;
	do{
		pNextTcb = (PRT_TCB)pEntry;
		// Prefast warning C6011: Dereferencing NULL pointer 'pNextTcb'.
		if((pNextTcb != NULL) && (PlatformCompareMemory(pTcb->DestinationAddress,pNextTcb->DestinationAddress,ETHERNET_ADDRESS_LENGTH) == 0)
				&& (pTcb->priority == pNextTcb->priority))
		{ 
				pTcb->EMPktLen[pTcb->EMPktNum] =pNextTcb->FragLength[0]+additionLen;
				pTcb->EMPktNum ++;
		}
		else
			break;
		
		// Prefast warning C6011: Dereferencing NULL pointer 'pEntry'.
		if (pEntry != NULL)
		{
			pEntry = pEntry->Flink;
		}
	}while(pEntry && pEntry != plistHead && pTcb->EMPktNum < maxPktNum);

	return TRUE;
}

/*
*	<Assumption: RT_TX_SPINLOCK is acquired.>
*	First added: 2006.11.19 by emily
*/
RESET_TYPE
TxCheckStuck(
	IN	PADAPTER		Adapter
	)
{
	BOOLEAN			bCheckFwTxCnt = FALSE;
	u1Byte			QueueID;
	PMGNT_INFO 		pMgntInfo = &Adapter->MgntInfo;
	PRT_TCB			pTcb;
	u1Byte			ResetThreshold=NIC_SEND_HANG_THRESHOLD_NORMAL;
	
	//
	// Decide Stuch threshold according to current power save mode
	//

	switch (pMgntInfo->dot11PowerSaveMode)
	{
		// The threshold value  may required to be adjusted .
		case eActive:		// Active/Continuous access.
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_NORMAL;
			break;
		case eMaxPs:		// Max power save mode.
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
		case eFastPs:	// Fast power save mode.
			ResetThreshold = NIC_SEND_HANG_THRESHOLD_POWERSAVE;
			break;
		default: //for MacOS comipler warning: "eAutoPs" not handled in switch
			break;
	}
			
	//
	// Check whether specific tcb has been queued for a specific time
	//
	for(QueueID = 0; QueueID < MAX_TX_QUEUE; QueueID++)
	{
		
		if(RTIsListEmpty(&Adapter->TcbBusyQueue[QueueID]))
			continue;
		else
		{
			pTcb = (PRT_TCB)RTGetHeadList(&Adapter->TcbBusyQueue[QueueID]);
			pTcb->nStuckCount++;
			
			if(pTcb->nStuckCount > ResetThreshold)
			{
				RT_TRACE( COMP_SEND, DBG_SERIOUS, ("<== TxCheckStuck()\n") );
				return RESET_TYPE_SILENT;
			}

			bCheckFwTxCnt = TRUE;
		}
	}


	if(bCheckFwTxCnt)
	{
		if(Adapter->HalFunc.TxCheckStuckHandler(Adapter))
		{
			RT_TRACE(COMP_SEND, DBG_LOUD, ("TxCheckStuck(): Fw indicates no Tx condition! \n"));
				return RESET_TYPE_SILENT;
		}
	}

	return RESET_TYPE_NORESET;
}
#if TX_AGGREGATION
//
// Assumption: RT_TX_SPINLOCK is acquired.
//
VOID
_ReturnTCB(
	PADAPTER			pAdapter,
	PRT_TCB				pTcb,
	RT_STATUS			status
	)
{
	PADAPTER Adapter = GetDefaultAdapter(pAdapter); 
	RT_ASSERT(IS_TX_LOCKED(Adapter) == TRUE, ("ReturnTCB(): bTxLocked(%x) should be TRUE\n!", Adapter->bTxLocked));

	if(pTcb == NULL)
	{
		// Prevent access violation, 2005.03.06, by rcnjko.
		RT_TRACE(COMP_SEND, DBG_SERIOUS, ("ReturnTCB(): pTcb==NULL !!! \n"));
		return;
	}

	if (TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER) && (pTcb->pCoalesceBuffer != NULL))
	{
		ReturnLocalBuffer(Adapter, pTcb->pCoalesceBuffer);
	}

	switch(pTcb->BufferType)
	{
	case RT_TCB_BUFFER_TYPE_SYSTEM:
		// We MUST release tx spin lock acquired before DrvIFCompletePacket(). 
		// Otherwise, nested lock acquiration might happen.
		// For example, when we plug two 8187 in one machine, it will cause 
		// OS hangs at RtUsbSend().  2005.11.17, by rcnjko.
		if(pTcb->Reserved != NULL)
		{
                PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
            DrvIFCompletePacket(Adapter, pTcb, status);
                PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
		}
		break;

	case RT_TCB_BUFFER_TYPE_LOCAL:
		ReturnLocalBuffer(Adapter, (PRT_TX_LOCAL_BUFFER)pTcb->Reserved);
		break;

	case RT_TCB_BUFFER_TYPE_BT_LOCAL:
		break;
		
	case RT_TCB_BUFFER_TYPE_RFD:
		ReturnRFDList(Adapter, (PRT_RFD)pTcb->Reserved);
		break;

	case RT_TCB_BUFFER_TYPE_FW_LOCAL:
		ReturnLocalFWBuffer(Adapter, (PRT_TX_LOCAL_BUFFER)pTcb->Reserved);
		break;

	default:
		RT_ASSERT(FALSE, ("ReturnTCB: unknow pTcb->BufferType: 0x%x!!!\n", pTcb->BufferType));
		break;
	}

	if(pTcb->bAggregate)
	{
		u1Byte	i;
		for(i = 0; i < pTcb->AggregateNum; i++)
			ReturnTCB(Adapter, pTcb->AggregatedTCB[i], RT_STATUS_SUCCESS);
	}

	// Initialize TCB before insert it into TcbIdleQueue, 2005.07.06, by rcnjko.
	CLEAR_FLAGS(pTcb->tcbFlags);	
	pTcb->DataRate=UNSPECIFIED_DATA_RATE;
	pTcb->bSpecifShortGI = FALSE;
	pTcb->bUseShortGI = FALSE;
	pTcb->bLDPC = FALSE;
	pTcb->bSTBC = FALSE;
	pTcb->PacketLength = 0;
	pTcb->BufferCount = 0;
	pTcb->Reserved = NULL;
	pTcb->priority = 0;
	pTcb->EncInfo.SecProtInfo = RT_SEC_NORMAL_DATA; 
	pTcb->EncInfo.IsEncByHW = FALSE;	// Null Function Data Len_Adjust debug, Annie, 2006-01-06.
	pTcb->bFromUpperLayer = FALSE;
	pTcb->TSID = DEFAULT_TSID;
	pTcb->BWOfPacket = CHANNEL_WIDTH_20;
	pTcb->bAggregate = FALSE;
	pTcb->bAMSDUProcessed = FALSE;
	pTcb->nStuckCount = 0;
	pTcb->bAMPDUEnable = FALSE;
	pTcb->AMPDUDensity = 0;
	pTcb->AMPDUFactor = 0;
	pTcb->bTxDisableRateFallBack = FALSE;
	pTcb->bTxUseDriverAssingedRate = FALSE;
	pTcb->bTxCalculatedSwDur = FALSE;
	pTcb->bTxEnableSwCalcDur = FALSE;
	pTcb->specialDataType = PACKET_NORMAL;
	pTcb->macId = 0;
	pTcb->P_AID = 0;
	pTcb->G_ID = 0;
	pTcb->bBTTxPacket = FALSE;
	pTcb->BT_macId = 0;
	pTcb->sysTime[0] = 0;
	pTcb->sysTime[1] = 0;
	pTcb->sysTime[2] = 0;
	pTcb->sysTime[3] = 0;
	pTcb->TxBFPktType = RT_BF_PKT_TYPE_NONE;

	pTcb->TcpOffloadMode = 0;
	pTcb->MACHeardLen = 0;

	pTcb->outPending = FALSE;
	pTcb->isUsbTxAgg = FALSE;
	pTcb->aggNum = 0;

	pTcb->bNeedTxReport = FALSE;
	pTcb->bTDLPacket = FALSE;
	pTcb->bSendPTIRsp = FALSE;
	pTcb->bSleepPkt = FALSE;
	if(GET_HAL_DATA(Adapter)->AMPDUBurstMode)
	{
		pTcb->bEnableEarlyMode = FALSE;
		pTcb->EMPktNum = 0;
		PlatformZeroMemory(pTcb->EMPktLen,sizeof(pTcb->EMPktLen));
	}
	RESET_CCX_PACKET_TX_INFO(pTcb);

	TxFeedbackDeInstallTxFeedbackInfoForTcb(Adapter, pTcb);
	
	RTInsertTailListWithCnt(&Adapter->TcbIdleQueue, &pTcb->List, &Adapter->NumIdleTcb);
	RT_TRACE(COMP_SEND, DBG_LOUD, ("_ReturnTCB(), Adapter->NumIdleTcb = %d\n", Adapter->NumIdleTcb));
}


VOID
ReturnTCB(
	PADAPTER			pAdapter,
	PRT_TCB				pTcb,
	RT_STATUS			status
	)
{	
	if(pTcb->isUsbTxAgg)
	{
		RT_LIST_ENTRY aggList;

		if(RTIsListEmpty(&pTcb->aggListHead)){
			_ReturnTCB(pAdapter, pTcb, status);
			return;
		}

		RTInitializeListHead(&aggList);
		RTSwitchListHead(&pTcb->aggListHead, &aggList);
		_ReturnTCB(pAdapter, pTcb, status);

		while(RTIsListNotEmpty(&aggList))
		{
			PRT_TCB pAggTcb = (PRT_TCB)RTRemoveHeadList(&aggList);
			_ReturnTCB(pAdapter, pAggTcb, status);			
		}
	}
	else
		_ReturnTCB(pAdapter, pTcb, status);
}


#else
//
// Assumption: RT_TX_SPINLOCK is acquired.
//
VOID
ReturnTCB(
	PADAPTER			pAdapter,
	PRT_TCB				pTcb,
	RT_STATUS			status
	)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pDefaultAdapter);	

	RT_ASSERT(IS_TX_LOCKED(pDefaultAdapter) == TRUE, ("ReturnTCB(): bTxLocked(%x) should be TRUE\n!", pDefaultAdapter->bTxLocked));

	if(pTcb == NULL)
	{
		// Prevent access violation, 2005.03.06, by rcnjko.
		RT_TRACE(COMP_SEND, DBG_SERIOUS, ("ReturnTCB(): pTcb==NULL !!! \n"));
		return;
	}

	if (TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
	{		
		ReturnLocalBuffer(pDefaultAdapter, pTcb->pCoalesceBuffer);		// coalesece buffer is belong to each adapter.
	}

	switch(pTcb->BufferType)
	{
	case RT_TCB_BUFFER_TYPE_SYSTEM:

		if(pTcb->ColoaseMethod == 2)
		{
			u1Byte i=0;
			
			for (i=0; i<pTcb->BufferBackCount; i++)
			{
				//pTcb->BufferList[i+1] = pTcb->BufferListBack[i];
				//pTcb->BufferList[i+1].Length= pTcb->BufferListBack[i].Length;
				pTcb->BufferList[i] = pTcb->BufferListBack[i];
			}
		}

		// We MUST release tx spin lock acquired before DrvIFCompletePacket(). 
		// Otherwise, nested lock acquiration might happen.
		// For example, when we plug two 8187 in one machine, it will cause 
		// OS hangs at RtUsbSend().  2005.11.17, by rcnjko.
		if(pTcb->Reserved != NULL)
		{
			PlatformReleaseSpinLock(pDefaultAdapter, RT_TX_SPINLOCK); 
			DrvIFCompletePacket(pDefaultAdapter, pTcb, status);
			PlatformAcquireSpinLock(pDefaultAdapter, RT_TX_SPINLOCK);
		}
		break;

	case RT_TCB_BUFFER_TYPE_LOCAL:
		if(pTcb->bTxCompleteLater)
		{
			PlatformReleaseSpinLock(pDefaultAdapter, RT_TX_SPINLOCK); 
			DrvIFCompletePacket(pDefaultAdapter, pTcb, RT_STATUS_SUCCESS);
			PlatformAcquireSpinLock(pDefaultAdapter, RT_TX_SPINLOCK);
		}		
		ReturnLocalBuffer(pDefaultAdapter, (PRT_TX_LOCAL_BUFFER)pTcb->Reserved);
		break;

	case RT_TCB_BUFFER_TYPE_BT_LOCAL:
		BT_ReturnDataBuffer(pDefaultAdapter, (PRT_TX_LOCAL_BUFFER)pTcb->Reserved);
		break;

	case RT_TCB_BUFFER_TYPE_RFD:
		ReturnRFDList(pDefaultAdapter, (PRT_RFD)pTcb->Reserved);
		break;

	case RT_TCB_BUFFER_TYPE_FW_LOCAL:
		ReturnLocalFWBuffer(pDefaultAdapter, (PRT_TX_LOCAL_BUFFER)pTcb->Reserved);
		break;

	default:
		RT_ASSERT(FALSE, ("ReturnTCB: unknow pTcb->BufferType: 0x%x!!!\n", pTcb->BufferType));
		break;
	}

	if(pTcb->bAggregate)
	{
		u1Byte	i;
		for(i = 0; i < pTcb->AggregateNum; i++)
			ReturnTCB(pDefaultAdapter, pTcb->AggregatedTCB[i], RT_STATUS_SUCCESS);
	}

	// Initialize TCB before insert it into TcbIdleQueue, 2005.07.06, by rcnjko.
	CLEAR_FLAGS(pTcb->tcbFlags);
	if(pTcb->bTxCompleteLater)
	{
		pTcb->bTxCompleteLater = FALSE;
		pTcb->ReservedBack = NULL;
	}
	pTcb->DataRate=UNSPECIFIED_DATA_RATE;
	pTcb->bSpecifShortGI = FALSE;
	pTcb->bUseShortGI = FALSE;
	pTcb->bLDPC = FALSE;
	pTcb->bSTBC = FALSE;
	pTcb->PacketLength = 0;
	pTcb->BufferCount = 0;
	pTcb->BufferType = RT_TCB_BUFFER_TYPE_NONE;
	pTcb->Reserved = NULL;
	pTcb->priority = 0;
	pTcb->EncInfo.SecProtInfo = RT_SEC_NORMAL_DATA; 
	pTcb->EncInfo.IsEncByHW = FALSE;	// Null Function Data Len_Adjust debug, Annie, 2006-01-06.
	pTcb->bFromUpperLayer = FALSE;
	pTcb->TSID = DEFAULT_TSID;
	pTcb->BWOfPacket = CHANNEL_WIDTH_20;
	pTcb->bAggregate = FALSE;
	pTcb->bAMSDUProcessed = FALSE;
	pTcb->nStuckCount = 0;
	pTcb->bAMPDUEnable = FALSE;
	pTcb->AMPDUDensity = 0;
	pTcb->AMPDUFactor = 0;
	pTcb->bTxDisableRateFallBack = FALSE;
	pTcb->bTxUseDriverAssingedRate = FALSE;
	pTcb->bTxCalculatedSwDur = FALSE;
	pTcb->bTxEnableSwCalcDur = FALSE;
	pTcb->specialDataType = PACKET_NORMAL;
	pTcb->macId = 0;
	pTcb->P_AID = 0;
	pTcb->G_ID = 0;
	pTcb->bBTTxPacket = FALSE;
	pTcb->BT_macId = 0;
	pTcb->EncInfo.bMFPPacket = FALSE; 	// CCX V65 Reset 
	pTcb->sysTime[0] = 0;
	pTcb->sysTime[1] = 0;
	pTcb->sysTime[2] = 0;
	pTcb->sysTime[3] = 0;

	pTcb->TcpOffloadMode = 0;
	pTcb->MACHeardLen = 0;
	pTcb->IPType = 0;


#if TX_AGGREGATION
	pTcb->outPending = FALSE;
	pTcb->isUsbTxAgg = FALSE;
#endif

	pTcb->bNeedTxReport = FALSE;
	pTcb->bTDLPacket = FALSE;
	pTcb->bSendPTIRsp = FALSE;
	pTcb->bSleepPkt = FALSE;
	pTcb->bInsert8BytesForEarlyMode = FALSE;
	pTcb->status.TOK=FALSE;
	pTcb->CurrentWritePoint = 0;
	pTcb->bHwProtection = FALSE;

#if WLAN_ETW_SUPPORT
	//
	// <MIRACAST_TODO>
	//
	pTcb->TcbFrameUniqueueID = 0;
#endif
	
	if(GET_HAL_DATA(pDefaultAdapter)->AMPDUBurstMode)
	{	
		pTcb->bEnableEarlyMode = FALSE;
		pTcb->EMPktNum = 0;
		PlatformZeroMemory(pTcb->EMPktLen,sizeof(pTcb->EMPktLen));
	}

	RESET_CCX_PACKET_TX_INFO(pTcb);

	TxFeedbackDeInstallTxFeedbackInfoForTcb(pDefaultAdapter, pTcb);
	
	RTInsertTailListWithCnt(&pDefaultAdapter->TcbIdleQueue, &pTcb->List, &pDefaultAdapter->NumIdleTcb);
	RT_TRACE(COMP_SEND, DBG_LOUD, ("ReturnTCB(), pAdapter->NumIdleTcb = %d\n", pDefaultAdapter->NumIdleTcb));
}
#endif

PRT_TX_LOCAL_BUFFER
GetLocalFWBuffer(
	PADAPTER			Adapter
	)
{
	PRT_TX_LOCAL_BUFFER pBuffer;
	PADAPTER pDefaultAdapter = GetDefaultAdapter(Adapter);

	if(RTIsListEmpty(&pDefaultAdapter->LocalFWBufferQueue))
		return NULL;

	pBuffer=(PRT_TX_LOCAL_BUFFER)RTRemoveHeadListWithCnt(&pDefaultAdapter->LocalFWBufferQueue, &pDefaultAdapter->NumLocalFWBufferIdle);

	return pBuffer;
}

PRT_TX_LOCAL_BUFFER
GetLocalBuffer(
	PADAPTER			pAdapter
	)
{
	PRT_TX_LOCAL_BUFFER pBuffer;
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);

	if(RTIsListEmpty(&pDefaultAdapter->LocalBufferQueue))
		return NULL;

	pBuffer=(PRT_TX_LOCAL_BUFFER)RTRemoveHeadListWithCnt(&pDefaultAdapter->LocalBufferQueue, &pDefaultAdapter->NumLocalBufferIdle);

	return pBuffer;
}

VOID
ReturnLocalBuffer(
	PADAPTER			pAdapter,
	PRT_TX_LOCAL_BUFFER	pLocalBuffer
	)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);	
	//RTInsertTailList(&Adapter->LocalBufferQueue, &pLocalBuffer->List);	
	RTInsertTailListWithCnt(&pDefaultAdapter->LocalBufferQueue, &pLocalBuffer->List, &pDefaultAdapter->NumLocalBufferIdle);
	
}

VOID
ReturnLocalFWBuffer(
	PADAPTER			Adapter,
	PRT_TX_LOCAL_BUFFER	pLocalBuffer
	)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(Adapter);	
	RTInsertTailListWithCnt(&pDefaultAdapter->LocalFWBufferQueue, &pLocalBuffer->List, &pDefaultAdapter->NumLocalFWBufferIdle);
}

//
// Description:
//	Wait all the packets queued in the tx busy queues until these packets completed or timeout.
// Return Value:
//	TRUE: All packets are completed.
//	FALSE: Timeout, but not all packets are completed.
// Assumption:
//	PASSIVE_LEVEL
// Note:
//	(1) If this procedure should be separated by HAL Code based, move it to HAL.
// By Bruce, 2008-01-17.
//
BOOLEAN
WaitTxBusyQueueComplete(
	PADAPTER			Adapter
	)
{
	BOOLEAN			bResult = TRUE;
	u1Byte			QueueID;
	int				WaitingCnt;
	
	for(QueueID = 0, WaitingCnt = 0; QueueID < MAX_TX_QUEUE; )
	{
		if(RTIsListEmpty(&Adapter->TcbBusyQueue[QueueID]))
		{
			QueueID++;
			continue;
		}
		else
		{
			RT_TRACE(COMP_POWER, DBG_TRACE, ("WaitTxBusyQueueComplete(): %d times TcbBusyQueue[%d] !=0 !\n", (WaitingCnt+1), QueueID));
			PlatformSleepUs(10);
			WaitingCnt ++;
		}

		if(WaitingCnt >= MAX_DOZE_WAITING_TIMES)
		{
			RT_TRACE(COMP_POWER, DBG_WARNING, ("\n\nWaitTxBusyQueueComplete(): %d times TcbBusyQueue[%d] != 0 !!!\n\n\n", MAX_DOZE_WAITING_TIMES, QueueID));
			bResult = FALSE;
			break;
		}
	}
	return bResult;
}

//
// Description:
//	Release all the packets from Tx/Rx Wait queues and return them to the upper layer.
// Assumption:
//	RT_TX_SPIN_LOCK is not acquired.
// By Bruce, 2008-11-28.
//
VOID
ReleaseDataFrameQueued(
	IN	PADAPTER	pAdapter
	)
{
	u1Byte			QueueID;
	PRT_TCB			pTcb;
	PRT_LIST_ENTRY	pEntry;
	u4Byte			RtnCnt=0;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	// Return all packets in Tcb wait queues.
	for(QueueID = 0; QueueID < MAX_TX_QUEUE; QueueID++)	
	{
		pEntry = RTGetHeadList(Get_WAIT_QUEUE(pAdapter, QueueID));

		while(!RTIsListHead((Get_WAIT_QUEUE(pAdapter,QueueID)), pEntry))
		{
			pTcb = (PRT_TCB)pEntry;
			pEntry = RTNextEntryList(pEntry);
			
			if(pAdapter == pTcb->SourceAdapt)
			{
				RTRemoveEntryListWithCnt((PRT_LIST_ENTRY)pTcb, GET_WAIT_QUEUE_CNT(pAdapter, QueueID));
				RtnCnt++;
				ReturnTCB(pAdapter, pTcb, RT_STATUS_SUCCESS);	
			}
		}
	}

	RT_TRACE(COMP_SEND, DBG_LOUD, ("ReleaseDataFramemQueued(), free %d pTcb from TcbWaitQueue[].\n", RtnCnt));
	
	if(!OS_SUPPORT_WDI(pAdapter))
	{
		// Return all packets in platform wait queue.
		PlatformReleaseDataFrameQueued(pAdapter);
	}
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
}

//
// Description:
//	Copy the packet from the os to the local buffer.
// Arguments:
//	[in] pAdapter -
//		The adpater context.
//	[in, out] pTcb -
//		The TCB to colesce.
// Return:
//	Return TRUE if coalescence is performed, or return FALSE.
// Note:
//	RT_TX_SPINLOCK is already acquired.
// By Bruce, 2010-10-14.
//
BOOLEAN
TCB_CopySystemPacketToLocal(
	IN		PADAPTER	pAdapter,
	IN OUT	PRT_TCB		pTcb
	)
{
	PRT_TX_LOCAL_BUFFER		pBuf;
	u1Byte					i = 0;
	SHARED_MEMORY			tmpBufferList[MAX_PER_PACKET_BUFFER_LIST_LENGTH];
	u4Byte					localBufOffset = 0;

	// If the buffer type is already local buffer, do nothing.
	if(pTcb->BufferType != RT_TCB_BUFFER_TYPE_SYSTEM)
		return FALSE;

	pBuf = GetLocalBuffer(pAdapter);
	if(pBuf == NULL)
	{
		RT_TRACE_F(COMP_MLME, DBG_WARNING, ("[WARNING] No local buffer to copy this packet to local!\n"));
		return FALSE;
	}

	for(i = 0; i < pTcb->BufferCount; i ++)
	{
		// Point to the local buffer.
		tmpBufferList[i].VirtualAddress = pBuf->Buffer.VirtualAddress + localBufOffset;
		tmpBufferList[i].PhysicalAddressLow = pBuf->Buffer.PhysicalAddressLow + localBufOffset;
		tmpBufferList[i].PhysicalAddressHigh = pBuf->Buffer.PhysicalAddressHigh;
		if(tmpBufferList[i].PhysicalAddressLow < pBuf->Buffer.PhysicalAddressLow)
			tmpBufferList[i].PhysicalAddressHigh ++;

		if(pTcb->BufferList[i].Length > 0)
		{
			PlatformMoveMemory(tmpBufferList[i].VirtualAddress, pTcb->BufferList[i].VirtualAddress, pTcb->BufferList[i].Length);
		}
		tmpBufferList[i].Length = pTcb->BufferList[i].Length;

		localBufOffset += pTcb->BufferList[i].Length;
	}

	// Complete this packet to OS.
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
	DrvIFCompletePacket(pAdapter, pTcb, RT_STATUS_SUCCESS);
	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	// Resotre the content buffer
	for(i = 0; i < pTcb->BufferCount; i ++)
	{
		pTcb->BufferList[i] = tmpBufferList[i];
	}
	pTcb->BufferType = RT_TCB_BUFFER_TYPE_LOCAL;
	pTcb->Reserved = pBuf;

	return TRUE;
}





#if 1
//==================================================
// For general tx handle thread
//==================================================
VOID
tx_generalThreadCallback(
	IN	PVOID	pContext
	)
{
	PADAPTER	Adapter=((PRT_THREAD)pContext)->Adapter;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		RT_ASSERT(FALSE, ("[TX], tx_generalThreadCallback() can only run under PASSIVE_LEVEL!!\n"));
		return;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("[TX], Tx general thread is started!!!\n"));
	}

	while(1)
	{
		if(RT_DRIVER_HALT(Adapter))
		{			
			RT_TRACE(COMP_SEND, DBG_LOUD,  ("[TX], Tx general thread is stopped because bDriverStopped(%d) or bSurpriseRemoved(%d)!!!\n",
				Adapter->bDriverStopped, Adapter->bSurpriseRemoved));
			break;
		}

		if( PlatformAcquireSemaphore(&Adapter->txGen.txGenSemaphore) != RT_STATUS_SUCCESS )
		{
			RT_TRACE(COMP_SEND, DBG_LOUD, ("[TX], Acquire Tx general thread semaphore not successful !!!\n"));
			break;
		}
		else
		{
			RT_TRACE(COMP_SEND, DBG_LOUD, ("[TX], Tx general thread to execute normal routine !!!\n"));
			DrvIFCheckTxCredit(Adapter);
		}
	}

	RT_TRACE(COMP_INIT, DBG_LOUD, ("[TX], Tx general thread is ended!!!\n"));
}


VOID
TX_InitThreads(
	IN	PADAPTER	Adapter
	)
{
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[TX], TX_InitThreads()\n"));

	PlatformInitializeSemaphore(&Adapter->txGen.txGenSemaphore, 0);

	PlatformInitializeThread(
		Adapter,  
		&Adapter->txGen.txGenThread,  
		(RT_THREAD_CALL_BACK)tx_generalThreadCallback,  
		"tx_generalThreadCallback",
		FALSE,
		0,
		NULL);

	PlatformRunThread(
		Adapter, 
		&Adapter->txGen.txGenThread, 
		PASSIVE_LEVEL,
		NULL);
}

VOID
TX_DeInitThreads(
	IN	PADAPTER	Adapter
	)
{
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[TX], TX_DeInitThreads()\n"));

	PlatformReleaseSemaphore(&Adapter->txGen.txGenSemaphore);
	PlatformFreeSemaphore(&Adapter->txGen.txGenSemaphore);
	
	PlatformWaitThreadEnd(Adapter, &(Adapter->txGen.txGenThread));
	PlatformCancelThread(Adapter, &(Adapter->txGen.txGenThread));
	PlatformReleaseThread(Adapter, &(Adapter->txGen.txGenThread));
}

VOID
TX_InitializeVariables(
	IN	PADAPTER	Adapter
	)
{
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[TX], TX_InitializeVariables()\n"));
}

VOID
TX_InitializeSpinlock(
	IN	PADAPTER	Adapter
	)
{
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[TX], TX_InitializeSpinlock()\n"));

}

VOID
TX_FreeSpinlock(
	IN	PADAPTER	Adapter
	)
{
	RT_TRACE(COMP_INIT, DBG_LOUD, ("[TX], TX_FreeSpinlock()\n"));

}

#endif



