#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "Receive.tmh"
#endif

/*
 *	Note:	Encryption overhead handling
 *				1. MPDU tail overhead will be removed before defragment(ex. ICV)
 *				2. MPDU head overhead(except first fragment) will be removed in defragment (ex. IV)
 *				3. MSDU tail overhead will be removed after defragment (ex. MIC)
 *				4. MSDU head overhead will be removed in TranslateRxPacketHeader (ex. IV for first fragment, CKIP head overhead)
*/

// 20100311 Joseph: Move from HAL folder to COMMON\Receive.c.
// This kind of modification may be used in all PCI solution.

VOID
InitializeRxVariables(
	PADAPTER	Adapter
	)
{
	u2Byte QueueID;

	Adapter->NumRfd = Adapter->MAX_NUM_RFD;
	for(QueueID=0 ; QueueID < MAX_RX_QUEUE ; QueueID++)
		Adapter->NumRxDesc[QueueID]= Adapter->MAX_NUM_RX_DESC;	

#if (MAX_RX_QUEUE ==2)
		Adapter->NumRxDesc[RX_CMD_QUEUE] = RX_DESC_NUM;
#endif

#if (MULTIPORT_SUPPORT == 1)
{
	// Workaround: MultiPortInitializeCloneRfdQueue should be called only once and only by the default adapter
	//	- Currently - The extension adapter will call this function

	if(IsDefaultAdapter(Adapter))
	{
		MultiPortInitializeCloneRfdQueue(Adapter);
	}
}
#endif

	PlatformAcquireSpinLock(Adapter, RT_RFD_SPINLOCK);
	RTInitializeListHead(&Adapter->RfdIdleQueue);
	for(QueueID=0 ; QueueID < MAX_RX_QUEUE ; QueueID++)
		RTInitializeListHead(&Adapter->RfdBusyQueue[QueueID]);
	PlatformReleaseSpinLock(Adapter, RT_RFD_SPINLOCK);

	ResetRxStatistics(Adapter);

	Adapter->MgntInfo.NumforRxFastbatch = 12;

}

// 20100311 Joseph: Add new function release Rx Timer and Workitem.
VOID
DeInitializeRxVariables(
	PADAPTER	Adapter
	)
{
}

//
// Description: For the purpose of RFD list debugging.
// Added by Roger, 2008.06.20.
//
BOOLEAN
ChkValidRFDs(
	PADAPTER	Adapter,
	pu4Byte		pRfdAddr
	)
{	
	BOOLEAN		bValid = FALSE;
	u2Byte		RfdIdx;
	
	
	for(RfdIdx = 0; RfdIdx < Adapter->NumRfd + 1; RfdIdx++)
	{
		if(Adapter->RfdListAddr[RfdIdx] == pRfdAddr)
			return TRUE;
	}
	return	bValid;
}

//
// Description: For the purpose of VirtualAddress in RFD list debugging.
// Added by Roger, 2008.06.20.
//
BOOLEAN
ChkValidVAs(
	PADAPTER	Adapter,
	pu4Byte		pVirtualAddr
	)
{	
	BOOLEAN		bValid = FALSE;
	u2Byte		RfdIdx;
	
	for(RfdIdx = 0; RfdIdx<Adapter->NumRfd; RfdIdx++)
	{
		if(Adapter->RfdVirtualAddr[RfdIdx] == pVirtualAddr)
			return TRUE;
	}
	return	bValid;
}

RT_STATUS
PrepareRFDs(
	PADAPTER	Adapter
	)
{
	
	u2Byte		i;
	PRT_RFD		pRfd;
	RT_STATUS	status;

#if (MULTIPORT_SUPPORT == 1)
{ // Allocate the Multiport Clone RFDs

	RT_TRACE_F(COMP_INIT, DBG_TRACE, ("before MultiPortPrepareCloneRfd \n"));

	status = MultiPortPrepareCloneRfd(Adapter);

	if(status != RT_STATUS_SUCCESS)
	{
		RT_TRACE_F(COMP_INIT, DBG_TRACE, ("MultiPortPrepareCloneRfd fail\n"));
		MultiPortFreeCloneRfd(Adapter);
		return status;
	}
}
#endif

	do{
		Adapter->RfdBuffer.Length=Adapter->NumRfd*sizeof(RT_RFD);
	
		RT_TRACE_F(COMP_INIT, DBG_TRACE, ("before RfdBuffer\n"));

		status=PlatformAllocateMemory(Adapter, &Adapter->RfdBuffer.Ptr,Adapter->RfdBuffer.Length);
		if(status!=RT_STATUS_SUCCESS)
		{
			RT_TRACE_F(COMP_INIT, DBG_TRACE, ("RfdBuffer fail\n"));		
			return status;
		}
		PlatformZeroMemory(Adapter->RfdBuffer.Ptr,Adapter->RfdBuffer.Length);

#if DBG_CMD
		DBG_Var.PseudoRfd.Ptr = Adapter->RfdBuffer.Ptr;
		DBG_Var.PseudoRfd.Length = Adapter->NumRfd*sizeof(RT_RFD);
#endif

		pRfd=(PRT_RFD)Adapter->RfdBuffer.Ptr;
		
		RT_TRACE_F(COMP_INIT, DBG_TRACE, ("before DrvIFAssociateRFD\n"));
		
		for(i=0;i<Adapter->NumRfd;i++)
		{
			status=DrvIFAssociateRFD(Adapter,&pRfd[i]);
			pRfd[i].SeqNum = i;
			Adapter->RfdListAddr[i] = (pu4Byte)&pRfd[i];
			Adapter->RfdVirtualAddr[i] = (pu4Byte)pRfd[i].Buffer.VirtualAddress;		

			if(status!=RT_STATUS_SUCCESS)
			{
				RT_TRACE_F(COMP_INIT, DBG_TRACE, ("DrvIFAssociateRFD fail\n"));			
				return status;
			}
			PlatformAcquireSpinLock(Adapter, RT_RFD_SPINLOCK);
			RTInsertTailListWithCnt(&Adapter->RfdIdleQueue, &pRfd[i].List, &Adapter->NumIdleRfd);
			PlatformReleaseSpinLock(Adapter, RT_RFD_SPINLOCK);
		}
		// <Roger_EXP> To keep track the head of RfdIdleQueue. 2008.06.03. 
		Adapter->RfdListAddr[i] = (pu4Byte)&Adapter->RfdIdleQueue;

		RT_TRACE_F(COMP_INIT, DBG_TRACE, ("before pAMSDU\n"));

		status = PlatformAllocateMemory(Adapter, (PVOID *)&Adapter->pAMSDU, Adapter->MAX_SUBFRAME_TOTAL_COUNT*sizeof(ALIGNED_SHARED_MEMORY));
		if(status != RT_STATUS_SUCCESS)
		{
			RT_TRACE_F(COMP_INIT, DBG_TRACE, ("pAMSDU fail\n"));		
			return status;
		}
		
		RT_TRACE_F(COMP_INIT, DBG_TRACE, ("before MAX_SUBFRAME_TOTAL_COUNT pAMSDU\n"));
		
		for(i=0;i < Adapter->MAX_SUBFRAME_TOTAL_COUNT;i++)
		{
			status=PlatformAllocateAlignedSharedMemory(Adapter, Adapter->pAMSDU+i, NIC_MAX_PACKET_SIZE);
			if( status != RT_STATUS_SUCCESS )
			{
				RT_TRACE_F(COMP_INIT, DBG_TRACE, ("MAX_SUBFRAME_TOTAL_COUNT pAMSDU fail\n"));			
				return status;
		}
		}
		
	}while(FALSE);

	DefragInitialize(Adapter);
	
#if RX_AGGREGATION
	PlatformAcquireSpinLock(Adapter, RT_RFD_SPINLOCK);
	RTInitializeListHead(&Adapter->RfdTmpList);
	PlatformReleaseSpinLock(Adapter, RT_RFD_SPINLOCK);
	for(i=0; i<MAX_PKT_AGG_NUM; i++)
	{
		pRfd = &(Adapter->RfdTmpArray[i]);
		pRfd->nChildCnt = 0;
		pRfd->ParentRfd = NULL;
		PlatformAcquireSpinLock(Adapter, RT_RFD_SPINLOCK);
		RTInsertTailList(&Adapter->RfdTmpList, &pRfd->List);
		PlatformReleaseSpinLock(Adapter, RT_RFD_SPINLOCK);
	}
#endif

#if( WDI_SUPPORT == TRUE )
	{
		status = PlatformAllocateMemory(Adapter, &Adapter->pThrottle, sizeof(NDIS_RECEIVE_THROTTLE_PARAMETERS));
		if( status == RT_STATUS_SUCCESS )
		{
			Adapter->pThrottle->MaxNblsToIndicate = AMSDU_MAX_NUM;
		}
	}
#endif

	return status;
}

VOID
FreeRFDs(
	PADAPTER	Adapter,
	BOOLEAN		bReset
	)
{
	int 			i;
	u4Byte		nFreed;
	PRT_RFD		pRfd;
	u1Byte		QueueID;

	if(Adapter->bInChaosTest)
		return;

	DefragReset(Adapter);

	//2 Put all RFD into idle queue

	for(QueueID = 0; QueueID < MAX_RX_QUEUE; QueueID++)
	{
		u2Byte	rfdbusy[MAX_RX_QUEUE] = {0};

		RT_ASSERT(QueueID<MAX_RX_QUEUE, ("FreeRFDs(): invalid rx queue id!\n"));

		PlatformAcquireSpinLock(Adapter, RT_RFD_SPINLOCK);
		while(!RTIsListEmpty(&Adapter->RfdBusyQueue[QueueID]))
		{
			pRfd=(PRT_RFD)RTRemoveHeadListWithCnt(&Adapter->RfdBusyQueue[QueueID], &Adapter->NumBusyRfd[QueueID]);
			RT_ASSERT(ChkValidRFDs(Adapter, (pu4Byte)&(pRfd->List)), ("(1)FreeRFDs(): Invalid RFD\n"));
			RTInsertTailListWithCnt(&Adapter->RfdIdleQueue, &pRfd->List, &Adapter->NumIdleRfd);
			
			if (rfdbusy[QueueID]++ > Adapter->MAX_NUM_RX_DESC)
			{
				// 2011/03/30 MH This is an ASSERTION CASE need debug for RFD busy queue pointer!!!
				// We already found two case may cause incorrect RFD busy queue pointer A). ISR DPC is 
				// executed after driver is goinfto unload or surpeised removes. B). DTM card we will indicate
				// packets twice and return RFD, this may cause incorrect idle RFD pointer!!! We need to
				// declare dould RFD number more than RX_DESC. We ae afraid of third case, so add a 
				// workaround method for unknow case temporarily.
				// If we escape from the case, the RFD aligned share memory may not be released. after a 
				// period of long time. resource may be insufficient.				
				RT_ASSERT(TRUE, ("FreeRFDs(): Something Wrong!\n"));
				break;
			}
		}
		PlatformReleaseSpinLock(Adapter, RT_RFD_SPINLOCK);
	}

	if(!bReset)
	{
		//2 Free idle queue
		PlatformAcquireSpinLock(Adapter, RT_RFD_SPINLOCK);
		if(!RTIsListEmpty(&Adapter->RfdIdleQueue))
		{
			nFreed=0;
			while(!RTIsListEmpty(&Adapter->RfdIdleQueue))
			{
				pRfd=(PRT_RFD)RTRemoveHeadListWithCnt(&Adapter->RfdIdleQueue, 
														&Adapter->NumIdleRfd);
				RT_ASSERT(ChkValidRFDs(Adapter, (pu4Byte)&(pRfd->List)), ("(2)FreeRFDs(): Invalid RFD\n"));
	
				// If starting address of rx buffer is shifted because of the existence of QoS 
				// control(in 818xb and 8190) or the existence of RxDrvInfo(in 8190), driver
				// sholud shift it back, otherwise, it returns wrong address. 2006.1.5 by emily
				if(pRfd->Status.bShift)
				{					
					pRfd->Buffer.VirtualAddress -= Adapter->HalFunc.GetRxPacketShiftBytesHandler(pRfd);			
					pRfd->Status.bShift = 0;
				}
				
				DrvIFDisassociateRFD(Adapter,pRfd);
                
				nFreed++;
			}
			if(nFreed != Adapter->NumRfd){
				RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Freed RFD less than allocated!nFreed:%d alloc:%d .\n",nFreed,Adapter->NumRfd));			
			}
			RT_ASSERT(nFreed==Adapter->NumRfd,("Freed RFD less than allocated!!\n"));
		}
		PlatformReleaseSpinLock(Adapter, RT_RFD_SPINLOCK);
		
		if(Adapter->RfdBuffer.Ptr != NULL)
		{
			PlatformFreeMemory(Adapter->RfdBuffer.Ptr,Adapter->RfdBuffer.Length);
		}
		
		if(Adapter->pAMSDU != NULL)
		{
		for(i = 0;i <Adapter->MAX_SUBFRAME_TOTAL_COUNT; i++)
		{
			PlatformFreeAlignedSharedMemory(Adapter, Adapter->pAMSDU+i);
		}

		PlatformFreeMemory((PVOID)Adapter->pAMSDU, Adapter->MAX_SUBFRAME_TOTAL_COUNT*sizeof(ALIGNED_SHARED_MEMORY));		
		}

	}


#if (MULTIPORT_SUPPORT == 1)
{ // Only Free the Multiport Clone RFDs: No Need to Reset (Busy Clone RFD is used by OS Protocol Stack)

	if(!bReset)
	{
		MultiPortFreeCloneRfd(Adapter);
	}
}
#endif

#if (WDI_SUPPORT == 1)
	if(!bReset)
	{
		PlatformFreeMemory(Adapter->pThrottle, sizeof(NDIS_RECEIVE_THROTTLE_PARAMETERS));
	}
#endif
}

VOID
PrepareAllRxDescBuffer(
	PADAPTER	Adapter
	)
{
}


VOID
SpareRxDesc(
	PADAPTER	Adapter
	)
{
	DefragRemoveOldest(Adapter);
	PrepareAllRxDescBuffer(Adapter);
}



VOID 
ResetRxStatistics(
	PADAPTER	Adapter
	)
{
	// Joseph 20090703: Restore last sniff value.
	u4Byte	temp = Adapter->RxStats.LastRxSniffMediaPktCnt;
	
	PlatformZeroMemory( &Adapter->RxStats, sizeof(RT_RX_STATISTICS) );

	// Joseph 20090703: Restore last sniff value.
	Adapter->RxStats.LastRxSniffMediaPktCnt = temp;
	Adapter->RxStats.LastLinkQuality = 0xFF;
	
}

VOID
CountRxFragmentStatistics(
	PADAPTER	Adapter
	)
{
	RT_RF_POWER_STATE rtState;
	// When RF is off, we should not count the packet for hw/sw synchronize
	// reason, ie. there may be a duration while sw switch is changed and hw
	// switch is being changed. 2006.12.04, by shien chang.
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));
	if (rtState == eRfOff)
	{
		return;
	}
	
	Adapter->RxStats.NumRxFramgment++;

}

VOID 
CountRxStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING	pduOS;
	pu1Byte			pDaddr;
	RT_RF_POWER_STATE rtState;
	pu1Byte			pdu;
	BOOLEAN			bLedBlinking=TRUE;
	u4Byte			rateIndex;    //Add by Jacken 2008/03/12
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	pu1Byte			pTaddr;

	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));

	// When RF is off, we should not count the packet for hw/sw synchronize
	// reason, ie. there may be a duration while sw switch is changed and hw
	// switch is being changed. 2006.12.04, by shien chang.
	if (rtState == eRfOff)
	{
		return;
	}
	
	if(!pMgntInfo->bMediaConnect || MgntRoamingInProgress(pMgntInfo))
		Adapter->RxStats.LastLinkQuality = 0xFF;
	//
	// Note:
	// If Vista, the packet is 802.11 and we shift for header translation just as Qos/Security, so the packet header of offset should be shifted.
	// In other platforms, the FragOffset indicates the 802.3 header, and the virtual address indicates the 802.11 header.
	// This procedure is a temporal solution and should be improved later like adding the indication of 802.11 or 802.3 header,
	// and the FragOffset should be re-defined more precisely. By Bruce, 2008-09-24.
	//

	FillOctetString(pduOS, pRfd->Buffer.VirtualAddress + PLATFORM_GET_FRAGOFFSET(pRfd), pRfd->PacketLength);
	pdu = pRfd->Buffer.VirtualAddress + PLATFORM_GET_FRAGOFFSET(pRfd);

	pDaddr = Frame_pDaddr(pduOS);	
	pTaddr = Frame_Addr2(pduOS);
	
	if( pRfd->Status.bCRC || pRfd->Status.bICV || pRfd->Status.bHwError )
	{
		// Statistics for error packets are now handled in CountRxErrStatistics().
		// 2004.06.10, by rcnjko.
	}
		//
	//Add by Maddest for DTM 1.0c test, 070823
	//We should spearate the Date and Mgnt packet to avoid dectet roaming case in watchdog(no packet receive)
	//
	else if (IsMgntFrame(pdu))
	{	
		Adapter->RxStats.NumRxMgntPacket++;
		bLedBlinking=FALSE;
	}	
	else if(IsDataFrame(pdu))
	{
		Adapter->RxStats.NumRxOkTotal ++;
		Adapter->RxStats.NumRxOkBytesTotal += pRfd->PacketLength;
		Adapter->MgntInfo.LinkDetectInfo.NumRecvDataInPeriod++;
		Adapter->MgntInfo.LinkDetectInfo.NumRxOkInPeriod++;

		if(pRfd->PacketLength < 500)
			Adapter->RxStats.NumRxOKSmallPacket++;
		else if(pRfd->PacketLength < 1000)
			Adapter->RxStats.NumRxOKMiddlePacket++;
		else 
			Adapter->RxStats.NumRxOKLargePacket++;

		rateIndex = pRfd->Status.DataRateIdx;
		// We get the rate index and save it when do QueryRxStatus
	
		Adapter->RxStats.ReceivePacketDataRateCounter[rateIndex]++;   //for Rx Data Rate Counter , by Jacken 2008/03/12
				
		if(GET_TDLS_ENABLED(pMgntInfo) && pMgntInfo->bMediaConnect && pMgntInfo->mAssoc)
		{
			if(PlatformCompareMemory(pTaddr, pMgntInfo->Bssid, 6)==0)
				Adapter->RxStats.ReceivedRateFromAPHistogram[rateIndex]++;
		}
				
	}

	if( MacAddr_isBcst( pDaddr ) ){
		Adapter->RxStats.NumRxBroadcast ++;
		Adapter->RxStats.NumRxBytesBroadcast += pRfd->PacketLength ;
	}
	else if( MacAddr_isMulticast( pDaddr ) ){
		Adapter->RxStats.NumRxMulticast ++;
		Adapter->RxStats.NumRxBytesMulticast += pRfd->PacketLength ;
	}
	else{		
		if(pRfd->bIsAggregateFrame)
			Adapter->RxStats.NumRxUnicast += pRfd->nTotalSubframe;
		else
			Adapter->RxStats.NumRxUnicast ++;
		Adapter->RxStats.NumRxBytesUnicast += pRfd->PacketLength;
		Adapter->RxStats.NumRxDecryptSuccessUnicast++;

		if(IsDataFrame(pdu))
		{
			if(pRfd->bIsAggregateFrame)
				Adapter->MgntInfo.LinkDetectInfo.NumRxUnicastOkInPeriod+=pRfd->nTotalSubframe;
			else
				Adapter->MgntInfo.LinkDetectInfo.NumRxUnicastOkInPeriod++;
			
			// 2009.03.03 Leave DC mode immediately when detect high traffic
			if(pMgntInfo->bMediaConnect && !MgntInitAdapterInProgress(pMgntInfo))
			{
				if(	((pMgntInfo->LinkDetectInfo.NumRxUnicastOkInPeriod + pMgntInfo->LinkDetectInfo.NumTxOkInPeriod) > 8 ) ||
					(pMgntInfo->LinkDetectInfo.NumRxUnicastOkInPeriod > 2) )
				{
					if(!pPSC->RegAdvancedLPs && !pMgntInfo->bSdioDpcIsr)
					{
						LeisurePSLeave(Adapter, LPS_DISABLE_RX_DETECT_BUSY);
					}
				}
			}
		}	
	}

	if( pRfd->Status.BandWidth == 1)
		
		Adapter->RxStats.NumRx40MHzPacket++;
	else
		Adapter->RxStats.NumRx20MHzPacket++;

	
	//To avoid led always blink when in rx, by Maddest 20080307;
	if(bLedBlinking)
	{
		if(Adapter->bInHctTest)
		{
			Adapter->HalFunc.LedControlHandler( Adapter, LED_CTL_RX );
		}
		else
		{
			if (Adapter->LedRxCnt == 0)
				PlatformSetTimer(Adapter, &pMgntInfo->LedTimer, 50);
		}
		Adapter->LedRxCnt++;
	}
}



VOID 
CountRxErrStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	RT_RF_POWER_STATE rtState;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);
	OCTET_STRING		pduOS;
	pu1Byte 			pRaddr;
	pu1Byte				pBssid;
	BOOLEAN				bAddrMatch = FALSE;
	BOOLEAN				bIsMulticast = FALSE;
	
	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;

	pRaddr = Frame_pRaddr(pduOS);
	pBssid = Frame_pBssid(pduOS);
	bAddrMatch = (PlatformCompareMemory(Adapter->CurrentAddress, pRaddr, ETHERNET_ADDRESS_LENGTH)) ? FALSE : TRUE;
	bIsMulticast = (MacAddr_isMulticast(pRaddr));


	if(pRfd->Status.bCRC)
		Adapter->RxStats.NumRxCrxErrorPacket++;
	
	// When RF is off, we should not count the packet for hw/sw synchronize
	// reason, ie. there may be a duration while sw switch is changed and hw
	// switch is being changed. 2006.12.04, by shien chang.
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_RF_STATE, (pu1Byte)(&rtState));
	if (rtState == eRfOff)
	{
		return;
	}

	// Check BSSID
	if(!eqMacAddr(Adapter->MgntInfo.Bssid, pBssid))
	{ // BSSID match.
		return;
	}

	// Check if this packet is belong to our NIC or multicast.
	if(!bAddrMatch && !bIsMulticast)
	{
		return;
	}

	if(pRfd->Status.bCRC || pRfd->Status.bICV || pRfd->Status.bHwError)
	{
		if(bAddrMatch)
			Adapter->RxStats.NumRxErrTotalUnicast++;
		else if(bIsMulticast)
			Adapter->RxStats.NumRxErrTotalMulticast++;
		if ( (pSecInfo->GroupEncAlgorithm == RT_ENC_ALG_AESCCMP) ||
			(pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_AESCCMP) )
		{
			CountRxCCMPDecryptErrorsStatistics(Adapter, pRfd);
		}
	}
	
	if(pRfd->Status.bCRC)		
	{ // CRC error packet.
		if(pRfd->PacketLength < 500)
			Adapter->RxStats.NumRxCrcErrSmallPacket++;
		else if(pRfd->PacketLength < 1000)
			Adapter->RxStats.NumRxCrcErrMiddlePacket++;
		else 
			Adapter->RxStats.NumRxCrcErrLargePacket++;
	}
	else 
	{ // CRC OK packet.
		if( IsFrameTypeData(pduOS.Octet) &&
			Frame_Retry(pduOS))
		{ // Type data && Retry bit set.
			Adapter->RxStats.NumRxRetryCount++;

			
			if(pRfd->PacketLength < 500)
				Adapter->RxStats.NumRxRetrySmallPacket++;
			else if(pRfd->PacketLength < 1000)
				Adapter->RxStats.NumRxRetryMiddlePacket++;
			else 
				Adapter->RxStats.NumRxRetryLargePacket++;
		}
	}

	if(pRfd->Status.bICV)
	{ // ICV error.
		RT_ENC_ALG	alg = RT_ENC_ALG_NO_CIPHER;
	
		Adapter->RxStats.NumRxIcvErr ++;

		alg = (bAddrMatch) ? pSecInfo->PairwiseEncAlgorithm : pSecInfo->GroupEncAlgorithm;

		switch(alg)
		{
			case RT_ENC_ALG_WEP40:
			case RT_ENC_ALG_WEP104:
				CountWEPICVErrorStatistics(Adapter, pRfd);
				break;

			case RT_ENC_ALG_TKIP:
				CountTKIPICVErrorStatistics(Adapter, pRfd);
				break;
			default: //Isaiah for MacOS compiler warning "ACESCCMP_Encryption" ...not handled in switch
				break;

		}
	}

	if(!pMgntInfo->SecurityInfo.SWRxDecryptFlag)
	{
		if(!pRfd->Status.Decrypted)
		{
			RT_TRACE(COMP_RECV, DBG_LOUD, ("CountRxErrStatistics:() CountRxDecryptErrorStatistics\n"));
		
			CountRxDecryptErrorStatistics(Adapter, pRfd);

			if(pSecInfo->GroupEncAlgorithm == RT_ENC_ALG_WEP40 ||
						pSecInfo->GroupEncAlgorithm == RT_ENC_ALG_WEP104 ||
						pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 ||
						pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP104)
			{
				CountWEPUndecryptableStatistics(Adapter, pRfd);		
			}
		}	
		else
			CountRxDecryptSuccessStatistics(Adapter, pRfd);		
	}
}

VOID
CountRxMediaStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	PADAPTER DefaultAdapter = GetDefaultAdapter(Adapter);
	if(DefaultAdapter->MgntInfo.bScanInProgress)
		DefaultAdapter->RxStats.CurRxSniffMediaPktCnt++;
}

VOID
CountRxExcludedUnencryptedStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte 			pRaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pRaddr = Frame_pRaddr(pduOS);

	if ( MacAddr_isBcst(pRaddr) )
	{
		Adapter->RxStats.NumRxExcludeUnencryptedBroadcast ++;
	}
	else if ( MacAddr_isMulticast(pRaddr) )
	{
		Adapter->RxStats.NumRxExcludeUnencryptedMulticast ++;
	}
	else
	{
		Adapter->RxStats.NumRxExcludeUnencryptedUnicast ++;
	}

}

VOID
CountRxTKIPLocalMICFailuresStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte 			pRaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pRaddr = Frame_pRaddr(pduOS);

	if ( MacAddr_isBcst(pRaddr) )
	{
		Adapter->RxStats.NumRxTKIPLocalMICFailuresBroadcast ++;
	}
	else if ( MacAddr_isMulticast(pRaddr) )
	{
		Adapter->RxStats.NumRxTKIPLocalMICFailuresMulticast ++;
	}
	else
	{
		Adapter->RxStats.NumRxTKIPLocalMICFailuresUnicast ++;
	}	
}

//
// Description:
//	Count the Rx TKIP MIC error  for mgnt packets.
// By Bruce, 2009-10-16.
//
VOID
CountRxMgntTKIPLocalMICFailuresStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	Adapter->RxStats.NumRxMgntTKIPMICFailures ++;
}


//
// Description:
//	Count the statistics of TKIP decryption with ICV errors.
// By Bruce, 2009-09-09.
//
VOID
CountRxTKIPDecryptErrorsStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte 			pRaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pRaddr = Frame_pRaddr(pduOS);

	if ( MacAddr_isBcst(pRaddr) )
	{
		Adapter->RxStats.NumRxTKIPICVErrorBroadcast ++;
	}
	else if ( MacAddr_isMulticast(pRaddr) )
	{
		Adapter->RxStats.NumRxTKIPICVErrorMulticast ++;
	}
	else
	{
		Adapter->RxStats.NumRxTKIPICVErrorUnicast ++;
	}	

}


VOID
CountRxCCMPDecryptErrorsStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte 			pRaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pRaddr = Frame_pRaddr(pduOS);

	if ( MacAddr_isBcst(pRaddr) )
	{
		Adapter->RxStats.NumRxCCMPDecryptErrorsBroadcast ++;
	}
	else if ( MacAddr_isMulticast(pRaddr) )
	{
		Adapter->RxStats.NumRxCCMPDecryptErrorsMulticast ++;
	}
	else
	{
		Adapter->RxStats.NumRxCCMPDecryptErrorsUnicast ++;
	}	

}

VOID
CountWEPICVErrorStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte 			pRaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pRaddr = Frame_pRaddr(pduOS);

	if ( MacAddr_isBcst(pRaddr) )
	{
		Adapter->RxStats.NumRxWEPICVErrorBroadcast ++;
	}
	else if ( MacAddr_isMulticast(pRaddr) )
	{
		Adapter->RxStats.NumRxWEPICVErrorMulticast ++;
	}
	else
	{
		Adapter->RxStats.NumRxWEPICVErrorUnicast ++;
	}	

}


VOID
CountTKIPICVErrorStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte				pDaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pDaddr = Frame_pDaddr(pduOS);

	if ( MacAddr_isBcst(pDaddr) )
	{
		Adapter->RxStats.NumRxTKIPICVErrorBroadcast ++;
	}
	else if ( MacAddr_isMulticast(pDaddr) )
	{
		Adapter->RxStats.NumRxTKIPICVErrorMulticast ++;
	}
	else
	{
		Adapter->RxStats.NumRxTKIPICVErrorUnicast ++;
	}	

}




VOID
CountRxDecryptSuccessStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte				pDaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pDaddr = Frame_pDaddr(pduOS);

	if ( MacAddr_isBcst(pDaddr) )
	{
		Adapter->RxStats.NumRxDecryptSuccessBroadcast++;
	}
	else if ( MacAddr_isMulticast(pDaddr) )
	{
		Adapter->RxStats.NumRxDecryptSuccessMulticast++;
	}
}



VOID
CountRxDecryptErrorStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte				pDaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pDaddr = Frame_pDaddr(pduOS);

	if ( MacAddr_isBcst(pDaddr) )
	{
		Adapter->RxStats.NumRxDecryptFailureBroadcast++;
	}
	else if ( MacAddr_isMulticast(pDaddr) )
	{
		Adapter->RxStats.NumRxDecryptFailureMulticast++;
	}
	else
	{
		Adapter->RxStats.NumRxDecryptFailureUnicast++;
	}	

}

VOID
CountWEPUndecryptableStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte				pDaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pDaddr = Frame_pDaddr(pduOS);

	if ( MacAddr_isBcst(pDaddr) )
	{
		Adapter->RxStats.NumRxWEPUndecryptableBroadcast++;
	}
	else if ( MacAddr_isMulticast(pDaddr) )
	{
		Adapter->RxStats.NumRxWEPUndecryptableMulticast++;
	}
	else
	{
		Adapter->RxStats.NumRxWEPUndecryptableUnicast++;
	}	

}



//
// Description:
//	Count the IV Replay condition for TKIP.
// By Bruce, 2009-09-09.
//
VOID
CountRxTKIPRelpayStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte 			pRaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pRaddr = Frame_pRaddr(pduOS);

	if(MacAddr_isMulticast(pRaddr))
	{
		Adapter->RxStats.NumRxTKIPReplayMulticast ++;
	}
	else
	{
		Adapter->RxStats.NumRxTKIPReplayUnicast ++;
	}	

}

//
// Description:
//	Count the IV Replay condition for CCMP.
// By Bruce, 2009-09-09.
//
VOID
CountRxCCMPRelpayStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING		pduOS;
	pu1Byte 			pRaddr;

	pduOS.Octet = pRfd->Buffer.VirtualAddress;
	pduOS.Length = pRfd->PacketLength;
	pRaddr = Frame_pRaddr(pduOS);

	if(MacAddr_isMulticast(pRaddr))
	{
		Adapter->RxStats.NumRxCCMPReplayMulticast ++;
	}
	else
	{
		Adapter->RxStats.NumRxCCMPReplayUnicast ++;
	}	

}

//
// Description:
//	Count the IV Replay condition for TKIP mgnt packets.
// By Bruce, 2009-10-16.
//
VOID
CountRxMgntTKIPRelpayStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	Adapter->RxStats.NumRxMgntTKIPReplay ++;
}

//
// Description:
//	Count the IV Replay condition for AES mgnt packets.
// By Bruce, 2009-10-16.
//
VOID
CountRxMgntCCMPRelpayStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	Adapter->RxStats.NumRxMgntCCMPReplay ++;
}

//
// Description:
//	Count the number of unencrypted mgnt TKIP packets for encryption mode is enabled (CCXv5 S67 MFP).
// Arguments:
//	[in] Adapter - 
//		The NIC context.
//	[in] pRfd -
//		The Rx buffer and information. 
// By Bruce, 2009-10-15.
//
VOID
CountRxMgntTKIPNoEncryptStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD 	pRfd
	)
{
	Adapter->RxStats.NumRxMgntTKIPNoEncrypt ++;
}

//
// Description:
//	Count the number of unencrypted mgnt AES packets for encryption mode is enabled (CCXv5 S67 MFP).
// Arguments:
//	[in] Adapter - 
//		The NIC context.
//	[in] pRfd -
//		The Rx buffer and information. 
// By Bruce, 2009-10-15.
//
VOID
CountRxMgntCCMPNoEncryptStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD 	pRfd
	)
{
	Adapter->RxStats.NumRxMgntCCMPNoEncrypt ++;
}

//
// Description:
//	Count the number of CCXv5 MFP TKIP MHDRIE invalid.
// Arguments:
//	[in] Adapter - 
//		The NIC context.
//	[in] pRfd -
//		The Rx buffer and information. 
// By Bruce, 2009-10-15.
//
VOID
CountRxMgntMFPTKIPMHDRStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD 	pRfd
	)
{
	Adapter->RxStats.NumRxMgntTKIPMHDRError ++;
}

//
// Description:
//	Count the number of mgnt TKIP packet descrypted error.
// Arguments:
//	[in] Adapter - 
//		The NIC context.
//	[in] pRfd -
//		The Rx buffer and information. 
// By Bruce, 2009-10-16.
//
VOID
CountRxMgntTKIPDecryptErrorsStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	Adapter->RxStats.NumRxMgntTKIPICVError ++;
}

//
// Description:
//	Count the number of mgnt AES packet descrypted error.
// Arguments:
//	[in] Adapter - 
//		The NIC context.
//	[in] pRfd -
//		The Rx buffer and information. 
// By Bruce, 2009-10-16.
//
VOID
CountRxMgntCCMPDecryptErrorsStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	Adapter->RxStats.NumRxMgntCCMPDecryptError ++;
}

VOID
RxStatisticsWatchdog(
	IN	PADAPTER	Adapter
	)
{
	u1Byte	idx = 0, slct_idx = 0;
	u4Byte	slct_cnt = 0;
	u1Byte	legacy_rate[12] = {0x02 , 0x04 , 0x0b , 0x16 , 0x0c , 0x12 , 0x18 , 0x24 , 0x30 , 0x48 , 0x60 , 0x6c};
	static u1Byte	rate_1M_delay = 0;

	Adapter->RxStats.CurRxOfdmNum = 0;
	Adapter->RxStats.CurRxCckNum = 0;

	for(idx=0; idx<77; idx++)
	{
		if(slct_cnt <= Adapter->RxStats.ReceivePacketDataRateCounter[idx])
		{
			slct_idx = idx;
			slct_cnt = Adapter->RxStats.ReceivePacketDataRateCounter[idx];
		}

		if(idx < 4)
			Adapter->RxStats.CurRxCckNum++;
		else
			Adapter->RxStats.CurRxOfdmNum++;

		Adapter->RxStats.ReceivePacketDataRateCounter[idx] = 0;
	}

	if(slct_cnt == 0 || slct_idx == 76)
	{
		Adapter->RxStats.CurRxDataRate.RawValue= 0xffff;
		rate_1M_delay = 0;
	}
	else
	{
		if(slct_idx == 0)
		{
			rate_1M_delay++;

			if(rate_1M_delay==2)
			{
				Adapter->RxStats.CurRxDataRate.RawValue = MGN_1M;
				rate_1M_delay = 0;
			}

		}
		else
		{
			rate_1M_delay = 0;

			if(slct_idx >= 12)
			{
				Adapter->RxStats.CurRxDataRate.HT = 1;
				Adapter->RxStats.CurRxDataRate.RateIdx = ((slct_idx-12)%16);
				Adapter->RxStats.CurRxDataRate.Bw = ((slct_idx-12)/32);
				Adapter->RxStats.CurRxDataRate.SGI = (((slct_idx-12)/16))%2;
				Adapter->RxStats.CurRxDataRate.Rsvd = 0;
			}
			else
			{
				Adapter->RxStats.CurRxDataRate.RawValue= legacy_rate[slct_idx];
			}
			
		}
	}

	//DbgPrint("Current Rx Rate = %x\n", Adapter->RxStats.CurRxDataRate);
}



#if SW_CRC_CHECK
/**
* Function:	SwCrcCheck
* 
* Overview:	This function do Crc check to received packets. 
*			It returns FALSE if the SW CRC check is not identical to HW CRC check.
* 
* Input:		
*		PRT_RFD		pRfd
* 			
* Output:		
*		None
* 		
* Return:     	
*		TRUE: 	SW CRC check is identical to HW CRC check.
*		FALSE:	SW CRC check is not identical to HW CRC check.
*/
BOOLEAN
SwCrcCheck(
	PRT_RFD		pRfd
	)
{
	pu1Byte	pFCS = pRfd->Buffer.VirtualAddress + pRfd->PacketLength;
	u4Byte	CRC	= crc32(pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	u4Byte	FCS = EF4Byte(*((UNALIGNED pu4Byte)(pFCS)));

	if(FCS != CRC)
	{
		if(!pRfd->Status.bCRC)
		{
			RT_TRACE(COMP_RECV, DBG_LOUD, ("RxDesc CRC: %x \n", (pRfd->Status.bCRC)?1:0));
			RT_TRACE(COMP_RECV, DBG_LOUD, ("HwFCS: %x, SwCRC: %x\n", FCS, CRC));
			return FALSE;
		}
	}

	return TRUE;
}
#endif


u1Byte
AMSDU_ReassemblePacket(
	PADAPTER 	Adapter,
	PRT_RFD		pRfd,
	PRT_RFD		*PRfd_Array
	)
{
	u1Byte			index = 0;
	OCTET_STRING	frame;
	u1Byte			nHeaderSize = 0;
	PRT_RFD 		pTmpRfd;

	FillOctetString(frame, pRfd->Buffer.VirtualAddress + pRfd->FragOffset, pRfd->FragLength);
	nHeaderSize	= 26 + (Frame_ValidAddr4(frame)?6:0);

	PlatformAcquireSpinLock(Adapter, RT_RFD_SPINLOCK);
	for(index = 0; index < pRfd->nTotalSubframe; index++)
	{
		if(!RTIsListEmpty(GET_RFD_IDLE_QUEUE(Adapter)))
		{
			pTmpRfd = (PRT_RFD)RTRemoveHeadListWithCnt(GET_RFD_IDLE_QUEUE(Adapter), GET_NUM_IDLE_RFD(Adapter));
			RT_ASSERT(ChkValidRFDs(GetDefaultAdapter(Adapter), (pu4Byte)&(pTmpRfd->List)), ("AMSDU_ReassemblePacket(): Invalid RFD\n"));
		}
		else
		{
			RT_ASSERT(FALSE, ("AMSDU_Deaggregation(): there is not enough rfd!!\n"));
			break;
		}

		pTmpRfd->Status.bIsQosData = pRfd->Status.bIsQosData;
		pTmpRfd->Status.bContainHTC = pRfd->Status.bContainHTC;
		pTmpRfd->Status.bCRC = pRfd->Status.bCRC;
		pTmpRfd->Status.bICV = pRfd->Status.bICV;
		pTmpRfd->Status.bHwError = pRfd->Status.bHwError;
		pTmpRfd->NextRfd = NULL;
		pTmpRfd->FragOffset = 0;
		pTmpRfd->PacketLength = nHeaderSize + pRfd->SubframeLenArray[index];
		pTmpRfd->FragLength = pTmpRfd->PacketLength;

		PlatformMoveMemory(pTmpRfd->Buffer.VirtualAddress, frame.Octet, nHeaderSize);
		PlatformMoveMemory(pTmpRfd->Buffer.VirtualAddress + nHeaderSize, pRfd->SubframeArray[index], pRfd->SubframeLenArray[index]);

		PRfd_Array[index] = pTmpRfd;
	}
	PlatformReleaseSpinLock(Adapter, RT_RFD_SPINLOCK);

	return index;
}

u1Byte
ParseSubframe(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING	frame;
	u2Byte			LLCOffset=sMacHdrLng;
	u2Byte			EncryptionMPDUHeadOverhead, EncryptionMSDUHeadOverhead, EncryptionHeadOverhead=0;
	u2Byte			ChkLength;
	BOOLEAN			bIsAggregateFrame = FALSE;
	u2Byte			nRemain_Length;
	u2Byte			nSubframe_Length;
	u1Byte			nPadding_Length = 0;
	u2Byte			SeqNum=0;

	FillOctetString(frame, pRfd->Buffer.VirtualAddress + pRfd->FragOffset, pRfd->FragLength);

	SeqNum = (u2Byte)Frame_SeqNum(frame);

	// Added for QoS control length. Annie, 2005-12-22.
	if( pRfd->Status.bIsQosData )
	{
		LLCOffset += sQoSCtlLng;

		if(GET_QOS_CTRL_HC_CFP_USRSVD(frame.Octet) == 1)
			bIsAggregateFrame = TRUE;
	}

	if(pRfd->Status.bContainHTC)
		LLCOffset += sHTCLng;

	// Null packet, don't indicate it to upper layer
	ChkLength =	LLCOffset + (Frame_WEP(frame)!=0 ?Adapter->MgntInfo.SecurityInfo.EncryptionHeadOverhead:0);

	if( pRfd->PacketLength <= ChkLength )
	{
		return 0;
	}

	//
	// Record AMSDU size if it is a valid AMSDU packet
	// 
	AMSDU_UpdateRxAMSDUSizeHistogram(Adapter, pRfd->PacketLength);

	
	if(Frame_WEP(frame)!=0)
	{	// For MPDU and MSDU head overhead in first frag
		SecGetEncryptionOverhead(
			Adapter,
			&EncryptionMPDUHeadOverhead, 
			NULL, 
			&EncryptionMSDUHeadOverhead, 
			NULL,
			TRUE,
			MacAddr_isMulticast(Frame_pDaddr(frame)));
		
		EncryptionHeadOverhead=EncryptionMPDUHeadOverhead;// + EncryptionMSDUHeadOverhead;
	}

	LLCOffset +=EncryptionHeadOverhead;
	nRemain_Length = pRfd->PacketLength - LLCOffset;

	pRfd->bIsAggregateFrame = bIsAggregateFrame;
	
	if(!bIsAggregateFrame)
	{
		pRfd->nTotalSubframe = 1;
		pRfd->SubframeArray[0] = frame.Octet + LLCOffset;
		pRfd->SubframeLenArray[0] = pRfd->FragLength - LLCOffset;
		return 1;
	}
	else
	{	
		pRfd->nTotalSubframe = 0;
		while(nRemain_Length > ETHERNET_HEADER_SIZE)
		{
			nSubframe_Length = N2H2BYTE(*((UNALIGNED pu2Byte)(frame.Octet + LLCOffset + 12)));

			//
			// Prevent unexpected MDL length requirement when packet indicateion, which might 
			// cause system using improper addresses, added by Roger, 2010.05.26.
			//
			if(nSubframe_Length<1)
			{
				RT_ASSERT(FALSE, ("Invalid A-MSDU subframe size: %d, drop it!!\n", nSubframe_Length));
				break;
			}
			
			if(nRemain_Length<(ETHERNET_HEADER_SIZE + nSubframe_Length))
			{
			#if 0//cosa
				RT_ASSERT(
					(nRemain_Length>=(ETHERNET_HEADER_SIZE + nSubframe_Length)), 
					("ParseSubframe(): A-MSDU subframe parse error!! Subframe Length: %d\n", nSubframe_Length) );
			#endif
				//DbgPrint("ParseSubframe(): A-MSDU subframe parse error!! pRfd->nTotalSubframe : %d\n", pRfd->nTotalSubframe);
				//DbgPrint("ParseSubframe(): A-MSDU subframe parse error!! Subframe Length: %d\n", nSubframe_Length);
				//DbgPrint("nRemain_Length is %d and nSubframe_Length is : %d\n",nRemain_Length,nSubframe_Length);
				//DbgPrint("The Packet SeqNum is %d\n",SeqNum);
				return 0;
			}
			
			LLCOffset += ETHERNET_HEADER_SIZE;
			pRfd->SubframeArray[pRfd->nTotalSubframe] = frame.Octet + LLCOffset;
			pRfd->SubframeLenArray[pRfd->nTotalSubframe] = nSubframe_Length;
			pRfd->nTotalSubframe++;

			if(pRfd->nTotalSubframe >= MAX_SUBFRAME_COUNT)
			{
				RT_TRACE(COMP_RECV, DBG_LOUD, ("ParseSubframe(): Too many Subframes! Packets dropped!\n"));
				break;
			}

			nRemain_Length = nRemain_Length - ETHERNET_HEADER_SIZE - nSubframe_Length;
			if(nRemain_Length != 0)
			{
				nPadding_Length = 4 - ((nSubframe_Length + ETHERNET_HEADER_SIZE) % 4);
				if(nPadding_Length == 4)
					nPadding_Length = 0;
				

				if(nRemain_Length < nPadding_Length)
				{
					RT_ASSERT(
						(nRemain_Length >= nPadding_Length), 
						("ParseSubframe(): A-MSDU subframe parse error!!! Remain Length: %d\n", nRemain_Length));
					return 0;
				}
				
				nRemain_Length -= nPadding_Length;
				LLCOffset = LLCOffset + nSubframe_Length + nPadding_Length;
			}			
		}
		AMSDU_UpdateRxAMSDUNumHistogram(Adapter, pRfd->nTotalSubframe);
		return pRfd->nTotalSubframe;
	}
}

/*  RxCheckSWDecryption
     Return False if decryption error
*/
BOOLEAN
RxCheckSWDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	BOOLEAN			bFreeRfd = FALSE;
	OCTET_STRING	frame;

	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

	if(pMgntInfo->SecurityInfo.SWRxDecryptFlag)
	{
		RT_SEC_STATUS	secStatus = RT_SEC_STATUS_SUCCESS;
		//
		// Case : Used SW Decrypt all Packet
		//
		
		if(RT_SEC_STATUS_SUCCESS != (secStatus = SecSoftwareDecryption(Adapter, pRfd)))
		{
			RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Failed (0x%08X) from SecSoftwareDecryption()\n", secStatus));
			bFreeRfd = TRUE;
		}
	}	// 2008.06.12
	else if(Adapter->HalFunc.GetNmodeSupportBySWSecHandler(Adapter))
	{
		//
		// Case : SWRxDecryptFlag == False , in WEP or TKIP & 24N-mode & bHalfWiirelessN24GMode  
		//
	
		if(!SecSoftwareDecryption(Adapter, pRfd))
		{
			bFreeRfd = TRUE;
		}
	}

	return bFreeRfd;
}



	

VOID
ProcessReceivedPacket(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
			)
{

// =========================================================================
// Here handles and monitors the system/driver state for Rx path. Please place codes here.
// =========================================================================

	// Get shifted bytes of Starting address of 802.11 header. 2006.09.28, by Emily	
	pRfd->Buffer.VirtualAddress += Adapter->HalFunc.GetRxPacketShiftBytesHandler(pRfd);

	pRfd->bRxBTdata = FALSE;

// =========================================================================
// Here starts to enable sniffer mode. Please let all packets get into this, including error packets.
// =========================================================================

// =========================================================================
// Here after starts to feed to each port. Please do not change the coding structure.
// =========================================================================
#if (MULTIPORT_SUPPORT == 1)
{
	// NOTE: -----------------------------------------------------------------------
	// 	This function is a packet distributor for each port. Please distrubute your packet herein.
	
	PADAPTER pSingleAdapter = MultiPortFeedPacketToMultipleAdapter(Adapter, pRfd);

	if(pSingleAdapter == NULL)
	{
		ReturnRFDList(Adapter, pRfd);
		return;
	}
	else
	{
		// Change the adapter to the target adapter
		Adapter = pSingleAdapter;
	}
	// -----------------------------------------------------------------------------
}
#endif

	ProcessReceivedPacketForEachPortSpecific(Adapter, pRfd);

}

VOID
ProcessReceivedPacketForEachPortSpecific(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
)
{
	BOOLEAN bFreeRfd = FALSE;
	BOOLEAN bQueued = FALSE;
	OCTET_STRING	frame = {NULL, 0};
	PMGNT_INFO		pMgntInfo = NULL;
	u1Byte			index = 0;
	// [Get Frame Element After Translation] After the do{}while(FALSE), the "frame" STRING would be "translate" to 802.3
	//  and could be no longer used. Therefore, we will keep some frame element. 2009.08.26, by Bohn
	BOOLEAN			bSecEAPOLPacket = FALSE; // Remenber the IsSecEAPOL, 2009.08.26, by Bohn
	BOOLEAN			bIhvFrameLogMode = FALSE;
	pu1Byte 			pSaddr_ori = NULL;
	pu1Byte 			pSaddr = NULL;
	u1Byte			Saddr[6];
	RT_SEC_STATUS	secStatus = RT_SEC_STATUS_SUCCESS;
	BOOLEAN 		bSupportRemoteWakeUp;

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_WOWLAN , &bSupportRemoteWakeUp);	

	/* 2007/01/16 MH Add RX command packet handle here. */
	/* 2007/03/01 MH We have to release RFD and return if rx pkt is cmd pkt. */
	if (Adapter->HalFunc.RxCommandPacketHandler(Adapter, pRfd))
	{	
		ReturnRFDList(Adapter, pRfd);
		return;
	}

	//
	// Check and parse wake packet
	//
	if(Adapter->bWakeFromPnpSleep && bSupportRemoteWakeUp)
	{
		if(WoL_HandleReceivedPacket(Adapter, pRfd))
		{
			ReturnRFDList(Adapter, pRfd);
			return;
		}
	}
	
	// =========================================================================
	// Here defines and handles signle MPDU for a single port. Please let it be the first line
	// =========================================================================
	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	// ----------------------------------------------------------------------------------

	if(!Adapter->bSWInitReady)
	{
		ReturnRFDList(Adapter, pRfd);
		return;
	}
	
	GetDefaultAdapter(Adapter)->bRemoveTsInRxPath = TRUE;
	
	bSecEAPOLPacket = SecIsEAPOLPacket( Adapter, &frame);// Remenber the IsSecEAPOL, 2009.08.26, by Bohn

	pSaddr_ori= Frame_pSaddr(frame);
	PlatformMoveMemory(Saddr, pSaddr_ori, 6);
	PlatformMoveMemory(pRfd->Address3,Frame_Addr3(frame),6);
	
	pSaddr = Saddr;
	pMgntInfo = &Adapter->MgntInfo;
	
	//Go determines Client is active or not by sherry 20130117
	if((IsMgntNullFrame(frame.Octet)) && eqMacAddr(Frame_pRaddr(frame), Adapter->CurrentAddress))
	{
		if(Adapter == GetFirstGOPort(Adapter))
		{
			// RT_TRACE(COMP_MLME,DBG_LOUD,("It is Go port receive packet \n"));
			// RT_PRINT_DATA(COMP_MLME, DBG_LOUD,"recv null function data: \n",frame.Octet,frame.Length);
			pMgntInfo->LinkDetectInfo.RxNullDataNum++;
			
			if(Frame_PwrMgt(frame))
				Adapter->bRecvEnterPSForGo = TRUE;
			else
				Adapter->bRecvEnterPSForGo	= FALSE;
		}
			
	}

#if (STATISTIC_SUPPORT == 1)
	CountRxMediaStatistics(Adapter, pRfd);
	CountRxErrStatistics(Adapter, pRfd);
	CountRxFragmentStatistics(Adapter);
#endif


#if SW_CRC_CHECK
	SwCrcCheck();
#endif

	do{
		// For QoS data checking. Annie, 2005-12-23.
		pRfd->Status.bIsQosData = QosDataCheck( Adapter, pRfd, &frame );
		pRfd->Status.UserPriority = QosGetUserPriority(Adapter, pRfd, &frame);
		pRfd->Status.bContainHTC = HTCCheck(	Adapter, pRfd, &frame	);

		if(pRfd->Status.bHwError)
		{		
			bFreeRfd = TRUE;
			break;
		}

		//
		//  Check Length  need call after QosDataCheck , QosGetUserPriority , HTCCheck, modify by Maddest suggest by CCW
		//
		if( !RxCheckLength(Adapter,pRfd))
		{
			bFreeRfd = TRUE;
			break;
		}

		if(TDLS_IsRxTDLSPacket(Adapter, pRfd))
			pRfd->bTDLPacket = TRUE;
		else
			pRfd->bTDLPacket = FALSE;

		//
		// Check descryption and descrypt the packet here if need, and return error if the descryption checking process fails.
		// By Bruce, 2009-10-09.
		//
		secStatus = SecRxDescryption(Adapter, pRfd);
		if(secStatus != RT_SEC_STATUS_SUCCESS)
		{
			RT_TRACE_F(COMP_SEC, DBG_TRACE, ("Drop this packet from the sec status (0x%08X)\n", secStatus));
			bFreeRfd = TRUE;
			break;
		}

		if (pMgntInfo->bNetMonitorMode)
		{
				break;
		}
		
		CCX_QueryIHVSupport(Adapter, &bIhvFrameLogMode);

		if(!MgntFilterReceivedPacket(Adapter, pRfd) && !bIhvFrameLogMode)
		{
			if (IsMgntFrame(frame.Octet)) 
				break;
			bFreeRfd=TRUE;
			break;
		}

		if(bSecEAPOLPacket &&  Frame_WEP(frame))
			RT_TRACE(COMP_SEC  , DBG_LOUD , ("====> 2-way EAPOL Packet !!\n") );


		// TODO: SecurityCheckMPDU

		if(!DefragPacket(Adapter, &pRfd, &bQueued))
		{		
			bFreeRfd=TRUE;
			break;
		}
		else if(bQueued)
			break;

		if(RT_STATUS_PKT_DROP == EncapDataFrame_ParsePkt(Adapter, pRfd))
		{
			RT_TRACE_F(COMP_RECV, DBG_LOUD, ("EncapDataFrame_ParsePkt(): RT_STATUS_PKT_DROP!\n"));
			bFreeRfd = TRUE;
			break;
		}


		if(IS_TDL_EXIST(pMgntInfo))
			TDLS_PS_UpdatePeerPSState(Adapter, &frame, FALSE);

		if(RT_STATUS_SUCCESS == WAPI_SecFuncHandler(WAPI_PROCESSRECEIVEDPACKET, Adapter, (PVOID)pRfd, WAPI_END))
		{
			bFreeRfd = TRUE;
			break;
		}
	
		// TODO: SecurityCheckMSDU
		
		if(Frame_ContainQc(frame) && GET_QOS_CTRL_HC_CFP_USRSVD(frame.Octet)  == 1)
		{ 	//2Here we handle the condition of multiple packets in single frame
			if(ParseSubframe(Adapter, pRfd) == 0)
			{
				bFreeRfd=TRUE;
				break;
			}			
			if(pMgntInfo->OpMode==RT_OP_MODE_AP)
			{
				PRT_RFD PRFD_Array[MAX_SUBFRAME_COUNT];
				BOOLEAN	bIndicate_Array[MAX_SUBFRAME_COUNT] = {0};
				u1Byte	nIndicateCnt = 0;
				u1Byte	nReassembleCnt = 0;

				// 20100611 Joseph: Handle Packet Forwarding and Rx A-MSDU more reasonable.
				// We shall reassemble as many packets as possible and forword them.
				nReassembleCnt = AMSDU_ReassemblePacket(Adapter, pRfd, PRFD_Array);

				// 20100611 Joseph: Forwarding reassembled packets.
				// Recording the information about which subframe shall be indicated later.
				for(index = 0; index < nReassembleCnt; index++)
				{
					if(!MgntCheckForwarding(Adapter, PRFD_Array[index]))
					{	// In here, this packet only need forward(no indication)
						bIndicate_Array[index] = FALSE;
					}
					else
					{	// In this case, we return duplicated packets in PRFD_Array, and indicate original pRfd later.
						bIndicate_Array[index] = TRUE;
						ReturnRFDList(Adapter, PRFD_Array[index]);
					}
				}

				// 20100611 Joseph: Drop the subframes that do not need to be indicated.
				for(index=0; index<nReassembleCnt; index++)
				{
					if(bIndicate_Array[index])
					{
						pRfd->SubframeArray[nIndicateCnt] = pRfd->SubframeArray[index];
						pRfd->SubframeLenArray[nIndicateCnt] = pRfd->SubframeLenArray[index];
						nIndicateCnt++;
					}
				}

				// 20100611 Joseph: Since "nReassembleCnt" may be less than "pRfd->nTotalSubframe" due to
				// out of RFD in AMSDU_ReassemblePacket() function, we need to handle this condition.
				// Indicate all other subframes that do not checkforwarding.
				for(index=nReassembleCnt; index<pRfd->nTotalSubframe; index++)
				{
					pRfd->SubframeArray[nIndicateCnt] = pRfd->SubframeArray[index];
					pRfd->SubframeLenArray[nIndicateCnt] = pRfd->SubframeLenArray[index];
					nIndicateCnt++;
				}

				pRfd->nTotalSubframe = nIndicateCnt;

				//workaround for BSOD when MSDU packets received in vwifi mode
				if(Adapter->bStartVwifi )
				{
					bFreeRfd = TRUE;
					break;
				}
				
				if(nIndicateCnt == 0 )
				{
					bFreeRfd = TRUE;
					break;
				}
			}
		
			if(!TranslateHandleRxDot11FrameHeader(Adapter, pRfd))
			{
				bFreeRfd=TRUE;
				break;
			}
		}
		else
		{	//2Here is normal condition.

			pRfd->nTotalSubframe = 0; // This indicates that this is not an aggregation frame.
			pRfd->bIsAggregateFrame = FALSE;
			
			if(pMgntInfo->OpMode==RT_OP_MODE_AP)
			{
				//
				// Win7 handles intra-BSS bridging through the regular reception and transmission path, 
				// i.e. miniport does not have to build the logic of forwarding intra-BSS frames to appropriate peers.
				//
				if(!MgntCheckForwarding(Adapter, pRfd))
				{	// In here, this packet only need forward(no indication)
					// MgntCheckForwarding() will forward it if necessary We can now return this packet. 2005.06.03, by rcnjko.
					bQueued = TRUE;
					RT_DISP(FRX, RX_PATH_AP_MODE, ("AP mode NOT need to indicate\n"));
					//
					// 2010/06/07 Simple modification for AP mode rereder consideration.
					// When we forward the packet between clients, we woll always indicated the
					// packet from client to AP immediately.
					// We need to consider forwarding RX reorder between clients and AP later.
					//
					if(ACTING_AS_AP(Adapter))
					{
						// Flush pending Rx Packets.
						FlushAllRxTsPendingPkts(Adapter);
					}
					break;
				}
				else
				{
					RT_DISP(FRX, RX_PATH_AP_MODE, ("AP mode need to indicate\n"));
				}
					
			}

			// Parse the RFD.
			if (CCX_ParseRfd(Adapter, pRfd, frame, &bFreeRfd))
			{
				break;
			}		

			if(!TranslateHandleRxDot11FrameHeader(Adapter, pRfd))
			{		
				bFreeRfd=TRUE;
				break;
			}
		}
	}while(FALSE);

	if(bFreeRfd)
	{
#if RX_AGGREGATION
		if(pRfd->RxAggrInfo.bIsRxAggr && pRfd->RxAggrInfo.bIsLastPkt)
			RxReorderAggrBatchFlushBuf(Adapter);
#endif
		// Management packet should set bFreeRfd to TRUE
		ReturnRFDList(Adapter, pRfd);
	}
	else if(!bQueued)
	{
		BOOLEAN 	bReorder = FALSE;
		BOOLEAN 	bStopRxreorder = FALSE;
	
		CountRxStatistics(Adapter, pRfd);

		if(ACTING_AS_AP(Adapter))
		{
			// For Win8 NdisTest - WFD_DataTransmission_ext:
			if(	pRfd->Status.pRxTS != NULL 
				&& (!bSecEAPOLPacket) 
				&& pMgntInfo->pHTInfo->bCurRxReorderEnable )
			{
				bReorder = TRUE;
			}
		}
		else
		{
			if(	pMgntInfo->pHTInfo->bCurrentHTSupport 
				&& pMgntInfo->pHTInfo->bCurRxReorderEnable 
				&& pRfd->Status.pRxTS!=NULL && (!bSecEAPOLPacket))
				bReorder = TRUE;

			// If driver detect the peer AP use AMSDU aggregation, then disable AMSDU and indicates all of
			// the packets buffered (becuase of RX-Reordering mechanism) to upper layer. CCW, 2008/12/16
			if(	pRfd->nTotalSubframe >= 1 &&  // AMSDU found !!
				pMgntInfo->pHTInfo->bCurRxReorderEnable && // Now we used Rxreorder !!
				pRfd->Status.pRxTS != NULL
				)
			{
				if(pRfd->Status.pRxTS->RxTotalSubframeCount+pRfd->nTotalSubframe >= Adapter->MAX_SUBFRAME_TOTAL_COUNT)
				{
					bStopRxreorder = TRUE;
					bReorder = FALSE;
				}
			}
		}

		GetDefaultAdapter(Adapter)->bRemoveTsInRxPath = FALSE;
		
		{
			if(bReorder)
			{
#if RX_AGGREGATION 
				if(pRfd->RxAggrInfo.bIsRxAggr)
					RxReorderAggrBatchIndicate(Adapter, pRfd);
				else
#endif
					RxReorderIndicatePacket(Adapter, pRfd);
			}
			else 
			{
#if RX_AGGREGATION
				if(pRfd->RxAggrInfo.bIsRxAggr && pRfd->RxAggrInfo.bIsLastPkt)
					RxReorderAggrBatchFlushBuf(Adapter);
#endif
				// We Inicate All Packet in RxReorderList !!
				if( bStopRxreorder )
				{
					RT_TRACE(COMP_RECV , DBG_LOUD , ("=======> Stop Rx Reorder !! \n") );

					// Flush pending Rx Packets.
					FlushAllRxTsPendingPkts(Adapter);
				}
				
				DrvIFIndicatePacket(Adapter, pRfd);
				if(pRfd->Status.pRxTS!=NULL)
					pRfd->Status.pRxTS->RxIndicateSeq = (pRfd->Status.Seq_Num+1)%4096;					
			}
		}
	}

}

BOOLEAN
DefragPacket(
	PADAPTER	Adapter,
	PRT_RFD		*ppRfd,
	PBOOLEAN	pbQueued
	)
{
	//1 Return FALSE if this RFD should be freed

	OCTET_STRING	frame;
	PRT_RFD		RetMsdu = NULL;
	pu1Byte			pSenderAddr;
	u1Byte			TID;
	u2Byte			SeqNum;
	u1Byte			FragNum;
	BOOLEAN		bMoreFrag;
	BOOLEAN		bEncrypted;
	u2Byte			EncryptionOverhead;
	u4Byte			i;
	BOOLEAN		bReturn = TRUE;
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;

	FillOctetString(frame, (*ppRfd)->Buffer.VirtualAddress + (*ppRfd)->FragOffset, (*ppRfd)->FragLength);

	pSenderAddr=Frame_Addr2(frame);
	TID=((Frame_ContainQc(frame)?Frame_QoSTID(frame, sMacHdrLng):16));
	SeqNum=Frame_SeqNum(frame);
	FragNum=(u1Byte)Frame_FragNum(frame);
	bMoreFrag=(BOOLEAN)Frame_MoreFrag(frame);
	bEncrypted=(BOOLEAN)Frame_WEP(frame);
	
	RT_TRACE_F(COMP_RECV, DBG_TRACE, ("SeqNum %d FragNum %d bMoreFrag %d bEncrypted %d\n", SeqNum, FragNum, bMoreFrag, bEncrypted));
	
	*pbQueued=FALSE;

	if(bEncrypted && !pMgntInfo->SafeModeEnabled)
	{
		//2 Remove MPDU tail overhead(ex. ICV)
		SecGetEncryptionOverhead(
			Adapter,
			NULL,
			&EncryptionOverhead,
			NULL,
			NULL,
			TRUE,
			MacAddr_isMulticast(Frame_pDaddr(frame)));
		
		MAKE_RFD_OFFSET_AT_BACK(*ppRfd, EncryptionOverhead);
	}

	//2 Do Defragment
	if((bMoreFrag || FragNum > 0) && !pMgntInfo->SafeModeEnabled)
	{
		RetMsdu=DefragAddRFD(Adapter,
			*ppRfd,
			pSenderAddr,
			TID,
			SeqNum,
			FragNum,
			bMoreFrag,
			bEncrypted);

		if(RetMsdu)
		{
			*ppRfd=RetMsdu;

			//2 We must do this again, because ppRfd is changed
			FillOctetString(frame, (*ppRfd)->Buffer.VirtualAddress + (*ppRfd)->FragOffset, (*ppRfd)->FragLength);
		}
		else
			*pbQueued=TRUE;
	}
	
	// At this point, we check the number of free RFDs. 
	// If it is lower than LowRfdThreshold, just release the LRU entry of DefragArray and
	// return associate RFDs. 2006.07.25, by shien chang
	// Note 1: The reason we didn't recycle RFDs at the above codes is even no fragment
	//		  received, the # of RFDs is probably run low.
	// Note 2: If RetMsdu is TRUE, we didn't recycle RFD. Because it is dangurous if the
	//		  recycled entry is just the RFDs of returned MSDU.
	if (RetMsdu == NULL)
	{
		for (i=0; i<*GET_LOW_RFD_THRESHOLD(Adapter); i++)
		{
			// for 92se test we disable to prevent the check			
			//Alghouth there many be more than one rx queuem but there should be only one 
			//queue for received data. 2006.1.3, by Emily
			//if (Adapter->NumIdleRfd+Adapter->NumBusyRfd[RX_MPDU_QUEUE] < Adapter->LowRfdThreshold)
			if ((*GET_NUM_IDLE_RFD(Adapter)+*GET_NUM_BUSY_RFD(Adapter, RX_MPDU_QUEUE))< *GET_LOW_RFD_THRESHOLD(Adapter))
			{
				RT_TRACE(COMP_RECV, DBG_LOUD, 
					("DefragPacket(): Number of RFDs (%d) is lower than threshold (%d).",
					Adapter->NumIdleRfd, Adapter->LowRfdThreshold));
				DefragRecycleRFD(Adapter);
			}
			else
				break;
		}
	}
	
	if(!(*pbQueued))
	{
		//1 Note:	We must get all ppRfd information again
		//1		At this point, this RFD list contain complete MSDU

		// Should be the same for all fragment
		bEncrypted=(BOOLEAN)Frame_WEP(frame);
	
		if(bEncrypted && !pMgntInfo->SafeModeEnabled)
		{
			//2 Check MIC and remove MIC
			
			//2004/07/07, kcwu, check mic
			bReturn = SecCheckMIC(Adapter, *ppRfd);

			SecGetEncryptionOverhead(
				Adapter,
				NULL,
				NULL,
				NULL,
				&EncryptionOverhead,
				TRUE,
				MacAddr_isMulticast(Frame_pDaddr(frame)));		
			MAKE_RFD_LIST_OFFSET_AT_BACK(Adapter, *ppRfd, EncryptionOverhead);
		}
	}
	return bReturn;
}

BOOLEAN
TranslateRxPacketHeader(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	u2Byte			i;
	OCTET_STRING	frame;
	BOOLEAN			RemoveLLCFlag=FALSE;
	u2Byte			LLCOffset;
	u2Byte			offset;	
	u2Byte			EncryptionMPDUHeadOverhead, EncryptionMSDUHeadOverhead, EncryptionHeadOverhead=0;
	u1Byte			SrcAddr[6];
	u1Byte			DestAddr[6];

	u2Byte			NeedCheckTypes=1;
	static u1Byte		LLCHeader[1][5]={	// Remember to check NeedCheckTypes
										{0x00,0x00,0x00,0x81,0x37},
									};
//	BOOLEAN			bIPARPPacket = FALSE;
	u2Byte			ChkLength;
	static u1Byte		SNAP_CKIP[5] = {0x00,0x40,0x96,0x00,0x02};	// For CKIP MIC case, added by Annie, 2006-08-18.

	FillOctetString(frame, pRfd->Buffer.VirtualAddress + pRfd->FragOffset, pRfd->FragLength);

	//
	// Figure out "LLCOffset", and 
	// "offset", the offset to advance from 802.11 frame to 802.3.
	//
	LLCOffset = Frame_FrameHdrLng(frame);
	offset = Frame_FrameHdrLng(frame) - ETHERNET_HEADER_SIZE; // In No security and QoS case 802.11 hdr is 24, eth header is 14, 24-14=10(keep LLC case).

	//
	// HTC check shall be modified as Qos control!!
	//
	if(pRfd->Status.bContainHTC)
	{
		LLCOffset += sHTCLng;
		offset += sHTCLng;
	}

	// Null packet, don't indicate it to upper layer
	ChkLength =	LLCOffset + (Frame_WEP(frame)!=0 ?Adapter->MgntInfo.SecurityInfo.EncryptionHeadOverhead:0);

	if( pRfd->PacketLength <= ChkLength )
	{
		return FALSE;
	}

	if(Frame_WEP(frame)!=0)
	{	// For MPDU and MSDU head overhead in first frag
		SecGetEncryptionOverhead(
			Adapter,
			&EncryptionMPDUHeadOverhead, 
			NULL, 
			&EncryptionMSDUHeadOverhead, 
			NULL,
			TRUE,
			MacAddr_isMulticast(Frame_pDaddr(frame)));
		
		EncryptionHeadOverhead=EncryptionMPDUHeadOverhead;// + EncryptionMSDUHeadOverhead;
	}

	
	LLCOffset +=EncryptionHeadOverhead;
	offset +=EncryptionHeadOverhead;

	if(	EF1Byte( frame.Octet[LLCOffset + 0] )==0xaa	&& 
		EF1Byte( frame.Octet[LLCOffset + 1] )==0xaa	&& 
		EF1Byte( frame.Octet[LLCOffset + 2] )==0x03
		)
	{	// check if need remove LLC header
	
		RemoveLLCFlag=TRUE;
		for(i=0;i<NeedCheckTypes;i++)
		{
			if(eqNByte(&frame.Octet[LLCOffset + 3],LLCHeader[i],5))
			{
				RemoveLLCFlag=FALSE;
				break;
			}
		}
		if(RemoveLLCFlag)
		{
			offset+=8;

			// Check CKIP MIC SNAP, added by Annie, 2006-08-18.
			if( eqNByte(&frame.Octet[LLCOffset+3], SNAP_CKIP, 5) )
			{
				offset += 10;		// 10 = CkipSnap_00-02(2) + CKIP_MIC(4) + CKIP_SEQ(4).
			}
		}
	}
		
	//
	// Setup Address Fields in ethernet header.
	//
	cpMacAddr(DestAddr, Frame_pDaddr(frame));
	cpMacAddr(SrcAddr, Frame_pSaddr(frame));
	cpMacAddr(frame.Octet+offset	,DestAddr);
	cpMacAddr(frame.Octet+offset+6	,SrcAddr);


	//
	// Insert length field if LLC is not removed.
	//
	if(!RemoveLLCFlag)
	{
		u2Byte len=pRfd->PacketLength-LLCOffset;

		(frame.Octet+offset)[12]=( (u1Byte)(len>>8) );
		(frame.Octet+offset)[13]=( (u1Byte)(len&0xff) );
	}
	
	//
	// Advance offset.
	//
	MAKE_RFD_LIST_OFFSET_AT_FRONT(pRfd, offset);

	return TRUE;
}



//
// For NDIS6 802.11 Rx frame handling.
// Added by Annie, 2006-10-20.
//
BOOLEAN
TranslateHandleRxDot11FrameHeader(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING	frame;
	//BOOLEAN		RemoveLLCFlag=FALSE;
	u2Byte			LLCOffset=sMacHdrLng;
	u2Byte			offset=0;			// 24-24=0
	u2Byte			EncryptionMPDUHeadOverhead, EncryptionMSDUHeadOverhead, EncryptionHeadOverhead=0;
	u2Byte			ChkLength;
	pu1Byte			pHeader;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);


	FillOctetString(frame, pRfd->Buffer.VirtualAddress + pRfd->FragOffset, pRfd->FragLength);
	pHeader = frame.Octet;

	// Added for QoS control length. Annie, 2005-12-22.
	if( pRfd->Status.bIsQosData )
	{
		LLCOffset += sQoSCtlLng;
		offset += sQoSCtlLng;

		SET_80211_HDR_QOS_EN(pHeader, 0);
	}

	if( pRfd->Status.bContainHTC)
	{
		LLCOffset += sHTCLng;
		offset += sHTCLng;
	}

	if(Frame_ValidAddr4(frame))
	{
		LLCOffset += 6;
		offset += 6;
	}

	// Null packet, don't indicate it to upper layer
	ChkLength =	LLCOffset + (Frame_WEP(frame)!=0 ?Adapter->MgntInfo.SecurityInfo.EncryptionHeadOverhead:0);

	if( pRfd->PacketLength <= ChkLength )
	{
		return FALSE;
	}

	if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED
		|| MgntActQuery_ApType(Adapter) == RT_AP_TYPE_LINUX )
	{
	//
	// 061227, rcnjko: 
	// Translate header for NDIS6 faked AP mode (UI make upper layer take us as AdHoc mode). 
	//
	if( pMgntInfo->OpMode == RT_OP_MODE_AP ) 
	{
		if(GET_80211_HDR_TO_DS(pHeader) == 1 && GET_80211_HDR_FROM_DS(pHeader) == 0)
		{ // Translate (1,0) to (0,0)
			u1Byte	tmpAddr[6];	
			u1Byte	tmpAddr2[6];

			SET_80211_HDR_FROM_DS(pHeader, 0);
			SET_80211_HDR_TO_DS(pHeader, 0);

			GET_80211_HDR_ADDRESS1(pHeader, tmpAddr);
			GET_80211_HDR_ADDRESS3(pHeader, tmpAddr2);
			SET_80211_HDR_ADDRESS1(pHeader, tmpAddr2);
			SET_80211_HDR_ADDRESS3(pHeader, tmpAddr);
		}
		else if(GET_80211_HDR_TO_DS(pHeader) == 1 && GET_80211_HDR_FROM_DS(pHeader) == 1)
		{ // Translate (1,1) to (0,0)
			SET_80211_HDR_FROM_DS(pHeader, 0);
			SET_80211_HDR_TO_DS(pHeader, 0);


			cpMacAddr(Frame_Addr1(frame), Frame_Addr3(frame));
			cpMacAddr(Frame_Addr2(frame), Frame_Addr4(frame));
			cpMacAddr(Frame_Addr3(frame), pMgntInfo->Bssid);
		}
	}
	}
	else if(IS_TDL_EXIST(pMgntInfo))
	{
		TDLS_RxTranslateHeader(Adapter, frame);
	}

	if(Frame_WEP(frame)!=0 && !pMgntInfo->SafeModeEnabled)
	{	// For MPDU and MSDU head overhead in first frag
		SecGetEncryptionOverhead(
			Adapter,
			&EncryptionMPDUHeadOverhead, 
			NULL, 
			&EncryptionMSDUHeadOverhead, 
			NULL,
			TRUE,
			MacAddr_isMulticast(Frame_pDaddr(frame)));
		
		EncryptionHeadOverhead=EncryptionMPDUHeadOverhead;// + EncryptionMSDUHeadOverhead;
	}
	
	LLCOffset +=EncryptionHeadOverhead;
	offset +=EncryptionHeadOverhead;

	if ( offset != 0)
	{
		u1Byte	buffer[32];
		u4Byte	BufLength = LLCOffset - offset;

		PlatformMoveMemory( buffer, frame.Octet, BufLength );
		PlatformMoveMemory( frame.Octet+offset, buffer, BufLength );
	}

	MAKE_RFD_LIST_OFFSET_AT_FRONT(pRfd, offset);
	return TRUE;
}

VOID
TranslateRxLLCHeaders(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	u2Byte			i, j;
	BOOLEAN			RemoveLLCFlag=FALSE;
	pu1Byte			pLLC, pFrame;
	u1Byte			srcaddr[6];
	u1Byte			DestinationAddress[6];
	u2Byte			NeedCheckTypes=1;
	static u1Byte		LLCHeader[1][5]={	// Remember to check NeedCheckTypes
										{0x00,0x00,0x00,0x81,0x37},
									};

	for(j = 0; j < pRfd->nTotalSubframe; j++)
	{
		pLLC = pRfd->SubframeArray[j];

		// Get Source Address and Destination Address
		cpMacAddr(DestinationAddress, pLLC - ETHERNET_HEADER_SIZE);
		cpMacAddr(srcaddr, pLLC - ETHERNET_HEADER_SIZE + ETHERNET_ADDRESS_LENGTH);

		
		pFrame = pLLC -ETHERNET_HEADER_SIZE;
		if(	*(pLLC + 0) == 0xaa	&& 
			*(pLLC + 1) == 0xaa	&& 
			*(pLLC + 2) == 0x03
			)
		{	// check if need remove LLC header
		
			RemoveLLCFlag=TRUE;
			for(i=0;i<NeedCheckTypes;i++)
			{
				if(eqNByte((pLLC + 3), LLCHeader[i], 5))
				{
					RemoveLLCFlag=FALSE;
					break;
				}
			}
			if(RemoveLLCFlag)
				pFrame += 8;
		}
			
		cpMacAddr(pFrame	,DestinationAddress);
		cpMacAddr(pFrame+6	,srcaddr);

		//2 Insert length field if LLC is not removed
		if(!RemoveLLCFlag)
		{
			u2Byte len=pRfd->SubframeLenArray[j];

			*(pFrame + 12)=(u1Byte)(len>>8);
			*(pFrame + 13)=(u1Byte)(len&0xff);
		}

		pRfd->SubframeArray[j] = pFrame;
		pRfd->SubframeLenArray[j] = pRfd->SubframeLenArray[j] + ETHERNET_HEADER_SIZE - ((RemoveLLCFlag)?8:0);
	}

}



VOID
ReturnRFD(
	PADAPTER	pAdapter,
	PRT_RFD		pRfd
)
{
	PADAPTER pDefaultAdapter =  GetDefaultAdapter(pAdapter);

#if RX_AGGREGATION
	PRT_RFD		pParentRfd = NULL;
#endif

#if RX_AGGREGATION
	pParentRfd = pRfd->ParentRfd;
	pRfd->ParentRfd = NULL;

	if(pRfd->bIsTemp)
	{
		PlatformAcquireSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
		RTInsertTailList(&pDefaultAdapter->RfdTmpList, &pRfd->List);
		PlatformReleaseSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
	}
	else		// Fall-Through
#endif
	if(IsCloneRFD(pDefaultAdapter,pRfd))
	{
		MultiPortReturnCloneRFD(pAdapter, pRfd);
	}
	else
	{
		RT_ASSERT(ChkValidRFDs(pDefaultAdapter, (pu4Byte)&(pRfd->List)), ("ReturnRFDList(): Invalid RFD\n"));
		PlatformAcquireSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
		RTInsertTailListWithCnt(&(pDefaultAdapter->RfdIdleQueue), &(pRfd->List), &pDefaultAdapter->NumIdleRfd);
		PlatformReleaseSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
	}

#if RX_AGGREGATION
	if(pParentRfd != NULL)
	{
		RT_ASSERT(pParentRfd->nChildCnt>0, ("ReturnRFDList(): Error handle Parent Rfd\n"));
	
		pParentRfd->nChildCnt--;

		if(pParentRfd->nChildCnt==0)
			ReturnRFDList(pAdapter, pParentRfd);
	}
#endif
}


VOID
ReturnRFDList(
	PADAPTER	pAdapter,
	PRT_RFD		pRfd
)
{
	u2Byte	nTotalRFD,nRFDFreed = 0;
	PRT_RFD	NextRfd = pRfd;
	PADAPTER pDefaultAdapter =  GetDefaultAdapter(pAdapter);

	if(pRfd->bReturnDirectly)
	{
		// Clean the return the flag
		pRfd->bReturnDirectly = FALSE;
		ReturnRFD(pAdapter, pRfd);
		return;
	}
	else if(pDefaultAdapter->bInChaosTest)
		return;

	nTotalRFD = pRfd->nTotalFrag;

	do{
		pRfd = NextRfd;
		NextRfd = pRfd->NextRfd;

		//2004/09/03, kcwu
		pRfd->NextRfd = NULL;

		ReturnRFD(pAdapter, pRfd);
		nRFDFreed++;
	}while(NextRfd);

	if(nTotalRFD != 0)
		RT_ASSERT(nTotalRFD==nRFDFreed, ("nTotalRFD not equal to nRFDFreed !!\n"));
}

VOID
MakeRFDListOffsetAtBack(
	PADAPTER	Adapter,
	PRT_RFD		pRfd, 
	u2Byte		offset
	)
{
	PRT_RFD	pCurrentRfd, pPreviousRfd;

	if(pRfd->nTotalFrag==1)
	{
		MAKE_RFD_OFFSET_AT_BACK(pRfd, offset);
	}
	else
	{
		pPreviousRfd=pRfd;
		pCurrentRfd=pRfd->NextRfd;

		//2 Find last 2 RFDs in list
		while(pCurrentRfd->NextRfd)
		{
			pPreviousRfd=pCurrentRfd;
			pCurrentRfd=pCurrentRfd->NextRfd;
		}

		if(pCurrentRfd->FragLength > offset)
		{
			pCurrentRfd->FragLength-=offset;
		}
		else
		{	
			//2 We need to remove last RFD
			pPreviousRfd->FragLength-=(offset - pCurrentRfd->FragLength);

			pCurrentRfd->bReturnDirectly = TRUE;
			ReturnRFDList(Adapter, pCurrentRfd);

			pRfd->nTotalFrag--;
			pPreviousRfd->NextRfd=NULL;			
		}
		
		pRfd->PacketLength-= offset;
	}
}

RESET_TYPE
RxCheckStuck(
	IN	PADAPTER		Adapter
	)
{
	{
		if(Adapter->HalFunc.RxCheckStuckHandler(Adapter))
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("RxStuck Condition\n"));
			return RESET_TYPE_SILENT;
		}
	}
	return RESET_TYPE_NORESET;
}

VOID
RxCheckStuckDbg(
	IN	PADAPTER		Adapter
	)
{
}

BOOLEAN
RxCheckResource(
	PADAPTER	pAdapter
	)
{
	if( (*GET_NUM_IDLE_RFD(pAdapter) + *GET_NUM_BUSY_RFD(pAdapter,RX_MPDU_QUEUE)) > RFD_LOWER_BOUND )
		return TRUE;
	else
		return FALSE;
}
BOOLEAN
RxCheckLength(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	PRT_SECURITY_T		pSec = &Adapter->MgntInfo.SecurityInfo;
	OCTET_STRING	frame;
	u4Byte			NeedLength = 0;


	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

	//
	// 2013/07/01 MH TPLINK report a strange bug the packet length is over rx max buffer?
	// We can only temporarily igore the packet. The packet length seems overflow. But only
	// happend once and by USB XHCI??
	//
	if (pRfd->PacketLength > 32768)	// 32K max rx aggregation packet.
		return	FALSE;

	if(!IsDataFrame(frame.Octet))
		return TRUE;	

	if(IsMgntNullFrame(frame.Octet))
		return TRUE;

	if(IsMgntQosNull(frame.Octet))
		return TRUE;


	NeedLength = sMacHdrLng;

	if(pRfd->bRxBTdata)
		return TRUE;

	// 
	if( pRfd->Status.bIsQosData )
	{
		NeedLength += sQoSCtlLng;
	}
	
	if( pRfd->Status.bContainHTC)
		NeedLength += sHTCLng;

	if( Frame_WEP(frame))
	{
		RT_ENC_ALG		EncAlgorithm_M/*, EncAlgorithm_B*/;
		u1Byte			DestAddr[6];
		//SecGetEncryptionOverhead(PADAPTER Adapter, pu2Byte pMPDUHead, pu2Byte pMPDUTail, pu2Byte pMSDUHead, pu2Byte pMSDUTail, BOOLEAN bByPacket, BOOLEAN bIsBroadcastPkt)

		NeedLength += pSec->EncryptionHeadOverhead +  pSec->EncryptionTailOverhead;
		
		// 2010/04/30 MH Add TKIP MIC length check.
		// 2010/05/13 MH Add broadcast tkip length check. Multicast will corver broadcast.
		PlatformMoveMemory(DestAddr, Frame_Addr1(frame), ETHERNET_ADDRESS_LENGTH);
		EncAlgorithm_M = (MacAddr_isMulticast(DestAddr)?pSec->GroupEncAlgorithm:pSec->PairwiseEncAlgorithm);
		//EncAlgorithm_B = (MacAddr_isBcst(DestAddr)?pSec->GroupEncAlgorithm:pSec->PairwiseEncAlgorithm);
		if (EncAlgorithm_M == RT_ENC_ALG_TKIP/* || EncAlgorithm_B == RT_ENC_ALG_TKIP*/)
			NeedLength += TKIP_MIC_LEN;
	}

	return ( pRfd->PacketLength >= NeedLength ) ? TRUE:FALSE;

	
}

//
// Description:
//	This thread handles all Data/Mgnt frames notification on rx flow.
//
VOID
RxNotifyThreadCallback(
	IN	PVOID	pContext
	)
{
	PADAPTER			pAdapter = ((PRT_THREAD)pContext)->Adapter;
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER			pTargetAdapter = NULL;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	u4Byte				index = 0;

	while(TRUE)
	{
		if( PlatformAcquireSemaphore(&(pNdisCommon->RxNotifySemaphore)) != RT_STATUS_SUCCESS )
			break;
		
		if(pAdapter->bDriverIsGoingToUnload == TRUE)
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("[Rx Thread] Driver is going to unload\n"));
			break;
		}
		
		/****************************************************************************************************
		* 20150707 Sinda
		* WDI will check MAC address in frame header when TID is WDI_EXT_TID_UNKNOWN(0x1f) or PeerId is Wildcard(0xffff)
		* 1. We indicate frames with TID=0x10(Non-QoS) before connection establishment
		* 2. We indicate frames with TID=0x1f(WDI_EXT_TID_UNKNOWN) after connection establishment
		****************************************************************************************************/
		PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
		for(index=0; index < MAX_PEER_NUM; index++)
		{
			if( !N6CIsNblWaitQueueEmpty(pDefaultAdapter->RxPeerQueue[index]) )
			{
				if( pDefaultAdapter->RxPeerTable[index].bUsed == TRUE )
				{
					PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
					pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)pDefaultAdapter->RxPeerTable[index].uPortId);
					if( pTargetAdapter != NULL )
					{
						if( index < MP_DEFAULT_NUMBER_OF_PORT )
							DrvIFIndicateDataInQueue(pTargetAdapter, 0x10, (u2Byte)(index), RT_RX_INDICATION_GENERAL);
						else
							DrvIFIndicateDataInQueue(pTargetAdapter, WDI_EXT_TID_UNKNOWN, (u2Byte)(index), RT_RX_INDICATION_GENERAL);
					}
					PlatformAcquireSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
				}
				else
					RT_TRACE(COMP_RECV, DBG_WARNING, ("[Rx Thread] There are data in queue, but the entry is unused\n"));
			}
		}
		PlatformReleaseSpinLock(pAdapter, RT_RX_QUEUE_SPINLOCK);
	}			
}

