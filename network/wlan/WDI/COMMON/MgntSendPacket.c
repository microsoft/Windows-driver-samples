#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "MgntSendPacket.tmh"
#endif

//
// Description: 
//	Send SW Beacon:
//   1. SW auto beacon whose TBTT maintained by HW and beacon update timing maintained by driver (-> TbttPollingWorkItemCallback() )
//       ex. USB & SDIO Beacon (SwBeaconType == BEACON_SEND_AUTO_SW, send to Beacon Queue) 
//
//	2. Manual Beacon whose TBTT and beacon update timing maintained by driver (-> TbttPollingWorkItemCallback() )
//       ex. Driver SW Beacon (SwBeaconType == BEACON_SEND_MANUAL, send to AC/Mgnt/High Queue)
//		
// Arguments:
//	[in] pAdapter -
//		The adapter context.
//	[in] QueueIndex -
//		The target Queue for beacon
//	[in] bcnlen -
//		Beacon Length
//	[in] TxRate -
//		Beacon Tx Rate
// Remark: 
//	Please do not use this function outside of this module
// by Hana, 2015-08-05
//
static VOID
SendSWBeacon(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				QueueIndex,
	IN	u4Byte				bcnlen,
	IN	u2Byte				TxRate
	)
{
	PMGNT_INFO	pMgntInfo = &pAdapter->MgntInfo;
	u4Byte		phyaddr;
	pu1Byte		virtualaddr;
	PRT_TCB	 	pTcb;
	PRT_TX_LOCAL_BUFFER	pBuf;
	RT_STATUS	rtStatus;

	phyaddr = pMgntInfo->BcnSharedMemory.PhysicalAddressLow;
	virtualaddr = pMgntInfo->BcnSharedMemory.VirtualAddress;

	if(MgntGetBuffer(pAdapter, &pTcb, &pBuf))
	{
		PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

		// Set up beacon content.
		PlatformMoveMemory(pBuf->Buffer.VirtualAddress, virtualaddr, bcnlen); 

		// Set up TCB
		pTcb->BufferList[0] = pBuf->Buffer;
		pTcb->BufferList[0].Length = bcnlen; 
		pTcb->BufferCount = 1;
		pTcb->PacketLength = pMgntInfo->beaconframe.Length;
		pTcb->Tailer.Length = 0;

		pTcb->ProtocolType = RT_PROTOCOL_802_11;
		pTcb->BufferType = RT_TCB_BUFFER_TYPE_LOCAL;
		pTcb->SpecifiedQueueID = QueueIndex;
		pTcb->DataRate = TxRate; 
		pTcb->bFromUpperLayer = FALSE;
		pTcb->Reserved = pBuf;
		pTcb->FragCount = 1;
		pTcb->TSID = DEFAULT_TSID;
		pTcb->FragLength[0] = (u2Byte)pTcb->PacketLength;
		pTcb->FragBufCount[0] = pTcb->BufferCount;

		SET_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_PACKET_SHIFT);
		pTcb->SourceAdapt = pAdapter;

		// Prevent Tx stuck, 2005.07.14, by rcnjko.
		if( PlatformIsTxQueueAvailable(pAdapter, QueueIndex, 1) )
		{
			rtStatus = TransmitTCB(pAdapter, pTcb);
			if(RT_STATUS_SUCCESS != rtStatus && RT_STATUS_PKT_DROP != rtStatus)
			{
				// Insert it to tail of wait queue.
				RT_TRACE(COMP_AP, DBG_LOUD, ("SendBeaconFrame(): !TransmitTCB() => Insert to wait queue of QueueIndex: %d\n", QueueIndex));
				RTInsertTailListWithCnt(Get_WAIT_QUEUE(pAdapter, pTcb->SpecifiedQueueID), &pTcb->List, GET_WAIT_QUEUE_CNT(pAdapter,pTcb->SpecifiedQueueID));
			}
		}
		else
		{
			// Insert it to tail of wait queue.
			RT_TRACE(COMP_SEND, DBG_TRACE, ("SendBeaconFrame(): Insert to wait queue of QueueIndex: %d\n", QueueIndex));
			RTInsertTailListWithCnt(Get_WAIT_QUEUE(pAdapter, pTcb->SpecifiedQueueID), &pTcb->List, GET_WAIT_QUEUE_CNT(pAdapter,pTcb->SpecifiedQueueID));
		}
		
		PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
	}
	else
	{
		RT_TRACE(COMP_AP, DBG_SERIOUS, ("SendBeaconFrame(): out of pTcb or Local Buffer !!!\n"));
	}


}

//
// Description: 
//	Send HW Beacon whose TBTT maintained by HW and beacon update timing are handled by Beacon Early interrupt.
//   ex.	PCIE Beacon (SwBeaconType == BEACON_SEND_AUTO_HW, send to Beacon Queue) 
//
// Arguments:
//	[in] pAdapter -
//		The adapter context.
//	[in] QueueIndex -
//		The target Queue for beacon
//	[in] bcnlen -
//		Beacon Length
//	[in] TxRate -
//		Beacon Tx Rate
// Remark: 
//	Please do not use this function outside of this module
// by Hana, 2015-08-05
//

static VOID
SendHWBeacon(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				QueueIndex,
	IN	u4Byte				bcnlen,
	IN	u2Byte				TxRate
	)
{
	PMGNT_INFO	pMgntInfo = &pAdapter->MgntInfo;
	u4Byte		phyaddr;
	pu1Byte		virtualaddr;
	PRT_TCB	 	pTcb;
	PRT_GEN_TEMP_BUFFER pGenBufTcb;

	pGenBufTcb = GetGenTempBuffer (pAdapter, sizeof(RT_TCB));
	pTcb = (RT_TCB *)pGenBufTcb->Buffer.Ptr;

	PlatformZeroMemory(pTcb, sizeof(RT_TCB));


	phyaddr = pMgntInfo->BcnSharedMemory.PhysicalAddressLow;
	virtualaddr = pMgntInfo->BcnSharedMemory.VirtualAddress;

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	pTcb->SpecifiedQueueID	= QueueIndex;
	pTcb->BufferCount		= 1;
	pTcb->FragCount			= 1;
	pTcb->DataRate			= TxRate;
	pTcb->bMulticast			= TRUE;
	pTcb->bBroadcast			= TRUE;
	pTcb->bUseShortPreamble	= FALSE;
	pTcb->bUseShortGI		= FALSE;
	pTcb->bRTSEnable			= FALSE;
	pTcb->bCTSEnable		= FALSE;
	pTcb->bEncrypt			= FALSE;
	pTcb->TxDescDuration[0]	= 0;

	// TODO: For experiment only It sholud be removed!!! 2007.1.10, Emily
	pTcb->bTxDisableRateFallBack = TRUE;		//it should be 1 by default
	pTcb->bTxUseDriverAssingedRate = TRUE; 	//it should be 1 by default
	pTcb->bTxCalculatedSwDur = TRUE;
	pTcb->bTxEnableSwCalcDur = TRUE;		//it sholud be 0 by default.

	// Rate adaptive
	pTcb->RATRIndex = 0;

	// Protection frame settings. No protection for Beacon frame!!
	pTcb->bRTSSTBC = 0;
	pTcb->RTSRate = 0;
	pTcb->RTSSC = 0;
	pTcb->RTSCCA = 0;
	pTcb->RTSBW = 0;
	pTcb->bRTSShort = 0;

	// A-MPDU Aggregation
	pTcb->bAMPDUEnable = FALSE;
	
	pTcb->FragBufCount[0]		= 1;
	pTcb->FragLength[0]		= (u2Byte)bcnlen;
	pTcb->BufferList[0].Length	= bcnlen;
	pTcb->BufferList[0].VirtualAddress 		= virtualaddr;
	pTcb->BufferList[0].PhysicalAddressLow 	= phyaddr;

	pTcb->SourceAdapt = pAdapter;

	pAdapter->HalFunc.TxFillDescriptorHandler(pAdapter, pTcb, 0, 0, 0, 0);
	ReturnGenTempBuffer(pAdapter, pGenBufTcb);

	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

	
}


//
//	Description: Send Beacon Frame.
//	2005.06.14, by rcnjko.
//
VOID
SendBeaconFrame(
	PADAPTER	Adapter,
	u1Byte		QueueIndex
	)
{
	PMGNT_INFO  pMgntInfo = &Adapter->MgntInfo;
	u4Byte		bcnlen;
	u2Byte		TxRate;


#if DBG_CMD
	if (DBG_Var.DBG_Beacon_Stop == 0)
		return;
#endif

	if(pMgntInfo->BeaconState == BEACON_STOP)
		return;

	pMgntInfo->BeaconState = BEACON_STARTED;

	bcnlen = pMgntInfo->beaconframe.Length;

	bcnlen += Adapter->TXPacketShiftBytes;
	
	if(bcnlen > pMgntInfo->MaxBeaconSize)
		pMgntInfo->MaxBeaconSize = bcnlen;

	if(	IS_WIRELESS_MODE_5G(Adapter) || 
		(IS_WIRELESS_MODE_HT_24G(Adapter) && !pMgntInfo->pHTInfo->bCurSuppCCK))
	{
		TxRate = MGN_6M;		// 6M
		RT_TRACE(COMP_SEND, DBG_TRACE, ("SendBeaconFrame(): Tx rate = 6 mbps!\n"));
	}
	else
	{
		RT_TRACE(COMP_SEND, DBG_TRACE, ("SendBeaconFrame(): Tx rate = 1 mbps!\n"));
		TxRate = MGN_1M;		// 1M
	}

	if(pMgntInfo->bDisableCck && IS_CCK_RATE(TxRate))
	{
		TxRate = MGN_6M;
	}

#if (P2P_SUPPORT == 1)	
	if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
	{
		PP2P_INFO		pP2PInfo = (GET_P2P_INFO(Adapter));
		if(pP2PInfo->Role == P2P_GO)
			TxRate = P2P_LOWEST_RATE;
	}
#endif

	{
		RT_TRACE(COMP_AP,DBG_TRACE,("SendBeaconFrame(): send SW auto/Manual beacon(send to beacon/Mgnt queue)\n"));
		SendSWBeacon(Adapter, QueueIndex, bcnlen, TxRate);
	}


}

VOID
SendProbeReq(
	PADAPTER	Adapter,
	BOOLEAN		bAnySsid,
	u2Byte		DataRate,
	BOOLEAN		bForcePowerSave 
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;

#if DBG_CMD
	if (DBG_Var.DBG_ProbeReq_Send == 0)
		return;
#endif	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructProbeRequest(
				Adapter, 
				pBuf->Buffer.VirtualAddress, 
				&pTcb->PacketLength,
				TRUE,
				bAnySsid,
				bForcePowerSave);

		RT_TRACE(COMP_SEND, DBG_TRACE, ("SendProbeReq(): Tx rate = %d\n", DataRate));
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
}

//
//	Description:
//		Advanced version of SendProbeReq. Calling this function will send dedicate 
//		probe request with SSID in pMgntInfo->SsidsToScan one by one, and send
//		an probe request with broadcast SSID at the last.
//	2006.12.20, by shien chang.
//
VOID
SendProbeReqEx(
	PADAPTER	Adapter,
	u2Byte		DataRate,
	BOOLEAN		bForcePowerSave
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u1Byte					index;
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_SSIDS_TO_SCAN		pSsidsToScan = &(pMgntInfo->SsidsToScan);

#if DBG_CMD
	if (DBG_Var.DBG_ProbeReq_Send == 0)
		return;
#endif	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	// Send probe request with SSID in pMgntInfo.SsidsToScan.
	for (index=0; index<pSsidsToScan->NumSsid; index++)
	{
		if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
		{
			ConstructProbeRequestEx(
				Adapter, 
				pBuf->Buffer.VirtualAddress, 
				&pTcb->PacketLength,
				pSsidsToScan->Ssid[index],
				bForcePowerSave);

			RT_TRACE(COMP_SEND, DBG_TRACE, ("SendProbeReqEx(): Tx rate = %d\n", DataRate));
			MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
		}
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
}

VOID
SendAuthenticatePacket(
	PADAPTER		Adapter,
	pu1Byte			auStaAddr,
	u1Byte			AuthAlg,
	u1Byte			AuthSeq,
	u1Byte			AuthStatusCode,
	OCTET_STRING	AuthChallengetext
	)
{
	// AuthSeq can be 1~4
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte	DataRate;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> SendAuthenticatePacket()\n"));
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructAuthenticatePacket(
								Adapter,
								pBuf->Buffer.VirtualAddress,
								&pTcb->PacketLength,
								auStaAddr,
								AuthAlg,
								AuthSeq,
								AuthStatusCode,
								AuthChallengetext
								);

		if(1 == AuthSeq)
		{
			MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, pMgntInfo->Bssid, 6);
			MgntUpdateAsocInfo(Adapter, UpdateAuthSeq1, pBuf->Buffer.VirtualAddress, pTcb->PacketLength);
		}

		DataRate = (u2Byte)MgntQuery_MgntFrameTxRate( Adapter, auStaAddr );
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== SendAuthenticatePacket()\n"));
}




VOID
SendAssociateReq(
	PADAPTER	Adapter,
	pu1Byte		asocStaAddr,
	u2Byte		asocCap,
	u2Byte		asocListenInterval
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte		DataRate;
	BOOLEAN		FlagReAsoc = FALSE;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> SendAssociateReq()\n"));
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructAssociateReq(
							Adapter,
							pBuf->Buffer.VirtualAddress,
							&pTcb->PacketLength,
							asocStaAddr,
							asocCap,
							asocListenInterval,
							pMgntInfo->Ssid,
							pMgntInfo->SupportedRates // 2005.11.25, by rcnjko.
							);

		MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, pMgntInfo->Bssid, 6);
		MgntUpdateAsocInfo(Adapter, UpdateAsocReq, pBuf->Buffer.VirtualAddress, pTcb->PacketLength);
		MgntUpdateAsocInfo(Adapter, UpdateFlagReAsocReq, (UCHAR*)&FlagReAsoc, sizeof(BOOLEAN));
		
		DataRate = (u2Byte)MgntQuery_MgntFrameTxRate( Adapter, asocStaAddr );
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== SendAssociateReq()\n"));
}



VOID
SendReassociateReq(
	PADAPTER	Adapter,
	pu1Byte		ReasocStaAddr,
	u2Byte		ReasocCap,
	u2Byte		asocListenInterval,	
	pu1Byte		CurrentasocStaAddr
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte	DataRate;
	BOOLEAN		FlagReAsoc = TRUE;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> SendReassociateReq()\n"));
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructReAssociateReq(
							Adapter,
							pBuf->Buffer.VirtualAddress,
							&pTcb->PacketLength,
							ReasocStaAddr,
							ReasocCap,
							asocListenInterval,
							CurrentasocStaAddr,
							Adapter->MgntInfo.Ssid,
							Adapter->MgntInfo.SupportedRates // 2005.11.25, by rcnjko.
							//Adapter->MgntInfo.Regdot11OperationalRateSet
							);

		MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, pMgntInfo->Bssid, 6);
		MgntUpdateAsocInfo(Adapter, UpdateAsocReq, pBuf->Buffer.VirtualAddress, pTcb->PacketLength);
		MgntUpdateAsocInfo(Adapter, UpdateFlagReAsocReq, (UCHAR*)&FlagReAsoc, sizeof(BOOLEAN));
		
		DataRate = (u2Byte)MgntQuery_MgntFrameTxRate( Adapter, ReasocStaAddr );
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== SendReassociateReq()\n"));
}


VOID
SendAssociateRsp(
	PADAPTER		Adapter,
	pu1Byte			asocStaAddr,
	u2Byte			asocCap,
	u2Byte			asocStatusCode,
	u2Byte			asocID,
	BOOLEAN			bReAssocRsp,
	BOOLEAN			bQosSTA
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte	DataRate;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> SendAssociateRsp()\n"));
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructAssociateRsp(
							Adapter,
							pBuf->Buffer.VirtualAddress,
							&pTcb->PacketLength,
							asocStaAddr,
							asocCap,
							asocStatusCode,
							asocID,
							bReAssocRsp,
							bQosSTA
							);

		DataRate = (u2Byte)MgntQuery_MgntFrameTxRate( Adapter, asocStaAddr );
		
		AsocEntry_UpdateAsocInfo(Adapter, 
			asocStaAddr, 
			NULL, 
			pTcb->PacketLength, 
			ALLOCATE_ASOC_RSP);

		//vivi add this for vitrual ap.as pcie add 8 byte before mac header as early mode, 20101130
		if(pTcb->bInsert8BytesForEarlyMode)
		{	
			AsocEntry_UpdateAsocInfo(Adapter, 
				asocStaAddr,
				pBuf->Buffer.VirtualAddress+8,
				pTcb->PacketLength-8,
				UPDATE_ASOC_RSP);
		}
		else
		{
			AsocEntry_UpdateAsocInfo(Adapter, 
				asocStaAddr,
				pBuf->Buffer.VirtualAddress,
				pTcb->PacketLength,
				UPDATE_ASOC_RSP);		
		}
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== SendAssociateRsp()\n"));
}



VOID
SendDeauthentication(
	PADAPTER		Adapter,
	pu1Byte			auSta,
	u1Byte			asRsn
)
{

	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte	DataRate;
	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> SendDeauthentication()\n"));

#if (MULTICHANNEL_SUPPORT == 1)
	if(Adapter == GetFirstClientPort(Adapter) && MultiChannelGetPortConnected20MhzChannel(Adapter) != 0)
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("ChangeWirelessModeHandler\n"));
	
		HalChangeWirelessMode(Adapter, MultiChannelGetPortConnected20MhzChannel(Adapter));
	
		MgntActSet_802_11_CHANNEL_AND_BANDWIDTH(
				Adapter, 
				MultiChannelGetPortConnected20MhzChannel(Adapter), 
				CHANNEL_WIDTH_20, 
				EXTCHNL_OFFSET_NO_EXT, 
				EXTCHNL_OFFSET_NO_EXT, 
				0
			);	
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Channel %d\n", MultiChannelGetPortConnected20MhzChannel(Adapter)));
	}
#endif

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	
	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructDeauthenticatePacket( 
			Adapter, 
			pBuf->Buffer.VirtualAddress, 
			&pTcb->PacketLength,
			auSta, 
			asRsn );

		DataRate = (u2Byte)MgntQuery_MgntFrameTxRate( Adapter, auSta );

	#if (MULTICHANNEL_SUPPORT == 1)
		if(MultiChannelSwitchNeeded(Adapter))
		{ // Let the Deauthentication go into AC Queue: MultiChannel Scheduler will let it go into right channel
			MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, BE_QUEUE, DataRate);
		}
		else
	#endif
		{
			MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
		}
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	

	// Win8: For sending packets in the correct channel ----
	delay_ms(50);
	// ----------------------------------------------
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== SendDeauthentication()\n"));
}




VOID
SendDisassociation(
	PADAPTER		Adapter,
	pu1Byte			asSta,
	u1Byte			asRsn
)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte	DataRate;
	u1Byte		TxPauseCommand = 0;
	u4Byte		MacIdPause = 0;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("===> SendDisassociation()\n"));

#if (MULTICHANNEL_SUPPORT == 1)
	if(GetDefaultMgntInfo(Adapter)->RegMultiChannelFcsMode == 0)
	{
		if(Adapter == GetFirstClientPort(Adapter) && MultiChannelGetPortConnected20MhzChannel(Adapter) !=0  
			&& (GET_HAL_DATA(Adapter)->CurrentChannel != MultiChannelGetPortConnected20MhzChannel(Adapter))) //ylb 20121029 add "&& (GET_HAL_DATA(Adapter)->CurrentChannel != MultiChannelGetPortConnected20MhzChannel(Adapter))"		
		{
			RT_TRACE_F(COMP_MLME, DBG_LOUD, ("ChangeWirelessModeHandler\n"));
		
			HalChangeWirelessMode(Adapter, MultiChannelGetPortConnected20MhzChannel(Adapter));		
			MgntActSet_802_11_CHANNEL_AND_BANDWIDTH(
					Adapter, 
					MultiChannelGetPortConnected20MhzChannel(Adapter), 
					CHANNEL_WIDTH_20, 
					EXTCHNL_OFFSET_NO_EXT, 
					EXTCHNL_OFFSET_NO_EXT, 
					0
				);
			
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Channel %d\n", MultiChannelGetPortConnected20MhzChannel(Adapter)));
			{	
				int i = 0;
				for( i = 0; i < 100; i++)  //add by ylb 20121029: Win8 WFD Test: Fix GO Cannot Receive Disassociation send by Client, Because Client send it at Wrong Channel
				{			
					if((GET_HAL_DATA(Adapter)->SwChnlInProgress == FALSE)
						&&(GET_HAL_DATA(Adapter)->CurrentChannel == MultiChannelGetPortConnected20MhzChannel(Adapter)))
						break;
					RT_TRACE(COMP_P2P, DBG_LOUD, ("SendDisassociation: Delay 100 us, Waiting for Switch Channel Complete(%d vs %d)!\n", 
						GET_HAL_DATA(Adapter)->CurrentChannel, MultiChannelGetPortConnected20MhzChannel(Adapter)));	
					delay_us(100);
				}
			}

		}
	}
#endif


//wait for Client is active when Go sending disassociation
//by sherry 20130117
	{
		int i;
		
		if((Adapter == GetFirstGOPort(Adapter)))
		{
			for(i=0;i<20;i++)
			{
				if(!Adapter->bRecvEnterPSForGo) //ylb add "|| Adapter == GetFirstClientPort(Adapter)"
				{
					RT_TRACE(COMP_MLME,DBG_TRACE,("STA is active send immediately \n"));
					break;
				}
				else 
				{


					RT_TRACE(COMP_MLME,DBG_LOUD,("STA is active send wait for sta in active: i= %d	\n",i));	
					PlatformStallExecution(10000);
				}
			}	
		}
	}

	//Enable BE Queue and VI Queue
	//by sherry 20130117
	if(GetDefaultMgntInfo(Adapter)->RegMultiChannelFcsMode == 0)
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_TX_PAUSE, (pu1Byte)(&TxPauseCommand));		
	else
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_MACID_PKT_SLEEP, (pu1Byte)(&MacIdPause));

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructDisassociatePacket(
			Adapter, 
			pBuf->Buffer.VirtualAddress, 
			&pTcb->PacketLength,
			asSta, 
			asRsn );

		DataRate = (u2Byte)MgntQuery_MgntFrameTxRate( Adapter, asSta );
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
	Adapter->LastConnectionActionTime = PlatformGetCurrentTime();

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<=== SendDisassociation()\n"));
}

//
// Description: 
//	Send ProbeRsp.
// Arguments:
//	[in] Adapter -
//		The adapter context.
//	[in] StaAddr -
//		The target mac address.
//	[in] bHideSSID -
//		Hide our SSID in the SSID element.
//	[in[ pRfdProbeReq -
//		The RFD contains the probe request. This field may be NULL.
//	[in] posProbeReq -
//		The location of OCTET_STRING for the probe request. This field may be NULL if no probe request as input.
// 2004.10.07, by rcnjko.
// Revise by Bruce, 2012-06-08.
//
VOID
SendProbeRsp(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				StaAddr,
	IN	BOOLEAN				bHideSSID,
	IN	PRT_RFD				pRfdProbeReq,
	IN	POCTET_STRING		posProbeReq
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructProbeRsp(
			Adapter, 
			pBuf->Buffer.VirtualAddress,
			&pTcb->PacketLength,
			StaAddr,
			bHideSSID,
			pRfdProbeReq,
			posProbeReq);

		if( !(ACTING_AS_AP(Adapter)) )
		{ 
			// For IBSS mode, we want to use the lowest basic rate.
			if(	IS_WIRELESS_MODE_5G(Adapter) ||
				(IS_WIRELESS_MODE_HT_24G(Adapter) &&!pMgntInfo->pHTInfo->bCurSuppCCK))			
				DataRate = MGN_6M;
			else
				DataRate = pMgntInfo->LowestBasicRate;
			
		}
		else
		{
			DataRate = (u2Byte)MgntQuery_MgntFrameTxRate(Adapter, StaAddr);
#if (P2P_SUPPORT == 1)			
			if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
			{
				PP2P_INFO	    pP2PInfo = GET_P2P_INFO(Adapter);
				if(pP2PInfo->Role == P2P_GO)
					DataRate = P2P_LOWEST_RATE;
			}
#endif			
		}
		
		RT_TRACE(COMP_SEND, DBG_TRACE, ("==SendProbeRsp(): Tx rate = %d\n", DataRate));
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
}

//
//	Description: Send a customized Data frame.
//	2004.12.05, by rcnjko.
//
VOID
SendCustomizedData(
	PADAPTER		Adapter,
	u2Byte			pktLength,
	u1Byte			txRate
)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructCustomizedData(
			Adapter, 
			pBuf->Buffer.VirtualAddress,
			&pTcb->PacketLength,
			pktLength
			);

		// by Owen on 05/01/10 to fix the following issue: 
		//	The AP Netgear WGT634U do not respond to RTL8187 hereafter after scanning. 
		//	ps: WGT634U only acks to RTL8187.
		/*
		// If payload is zero-legth, some AP may not handle the packet
		// with WEP correctly, Netgear 11b/g WGT634U.
		// 2004.12.05, by rcnjko.
		pTcb->PacketLength += 50; 
		*/
		//((pu1Byte)pBuf->Buffer.VirtualAddress)[pTcb->PacketLength] = 0xaa;
		//((pu1Byte)pBuf->Buffer.VirtualAddress)[pTcb->PacketLength+1] = 0xaa;
		//pTcb->PacketLength += 2;
		//

		//DataRate = pMgntInfo->CurrentOperaRate; 
		pTcb->bTxUseDriverAssingedRate = TRUE;
		DataRate = txRate;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, BE_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
}


//
//	Description: Send a Null function Data frame in client mode.
//	2005.03.23, by rcnjko.
//
VOID
SendNullFunctionData(
	PADAPTER		Adapter,
	pu1Byte			StaAddr,
	BOOLEAN			bForcePowerSave
)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{

		//Add bQoS parameter for QoS Null by Isaiah 2006-07-31
		ConstructNullFunctionData(
			Adapter, 
			pBuf->Buffer.VirtualAddress,
			&pTcb->PacketLength,
			StaAddr,
			FALSE,
			0,
			0,
			bForcePowerSave);

		DataRate = pMgntInfo->LowestBasicRate;			// Annie, 2005-03-31		
		pTcb->bTxUseDriverAssingedRate = TRUE;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}



//
//	Description: Send PS-Poll.
//	2005.02.15, by rcnjko.
//
VOID
SendPSPoll(
	PADAPTER		Adapter
)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructPSPoll(
			Adapter, 
			pBuf->Buffer.VirtualAddress,
			&pTcb->PacketLength);

		DataRate = pMgntInfo->LowestBasicRate;			// Annie, 2005-03-31		

		pTcb->bTxCalculatedSwDur = TRUE;
		pTcb->bTxEnableSwCalcDur = TRUE;

		// Note: We need to send PS-poll in HIGH QUEUE and fw will revise AID number to fix previous fw bug. 2009.03.26 by tynli.
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, MGNT_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
}

//
//	Description: Send a magic packet.
//	2005.06.27, by rcnjko.
//
VOID
SendMagicPacket(
	PADAPTER		Adapter,
	pu1Byte			StaAddr
)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;

	RT_TRACE(COMP_SEND, DBG_LOUD, ("SendMagicPacket(): StaAddr: %02X-%02X-%02X-%02X-%02X-%02X\n", 
			StaAddr[0], StaAddr[1], StaAddr[2], StaAddr[3], StaAddr[4], StaAddr[5] ));

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructMagicPacket(
			Adapter, 
			pBuf->Buffer.VirtualAddress,
			&pTcb->PacketLength,
			StaAddr);

		DataRate = pMgntInfo->LowestBasicRate;
		pTcb->bTxUseDriverAssingedRate = TRUE;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}

//
//	Description: Send EAPOL-Key packet.
//	2005.07.01, by rcnjko.
//
VOID
SendEapolKeyPacket(
	PADAPTER		Adapter,
	pu1Byte			StaAddr,

	pu1Byte			pKCK, // Pointer to KCK (EAPOL-Key Confirmation Key).
	pu1Byte			pKEK, // Pointer to KEK (EAPOL-Key Encryption Key).

	KeyType			eKeyType, // EAPOL-Key Information field: Key Type bit: type_Group or type_Pairwise.
	BOOLEAN			bInstall, // EAPOL-Key Information field: Install Flag.
	BOOLEAN			bKeyAck, // EAPOL-Key Information field: Key Ack bit.
	BOOLEAN			bKeyMIC, // EAPOL-Key Information field: Key MIC bit. If true, we will calculate EAPOL MIC and fill it into Key MIC field. 
	BOOLEAN			bSecure, // EAPOL-Key Information field: Secure bit.
	BOOLEAN			bError, // EAPOL-Key Information field: Error bit. True for MIC failure report.
	BOOLEAN			bRequest, // EAPOL-Key Information field: Requst bit.
	
	u8Byte			u8bKeyReplayCounter, // EAPOL-KEY Replay Counter field.
	pu1Byte			pKeyNonce, // EAPOL-Key Key Nonce field (32-byte).
	u8Byte			u8bKeyRSC, // EAPOL-Key Key RSC field (8-byte).

	POCTET_STRING	posRSNIE, // Key Data field: Pointer to RSN IE, NULL if 
	pu1Byte			pGTK // Key Data field: Pointer to GTK, NULL if Key Data Length = 0.
)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;
#if 1 //Added by Jay 0712 for
	PRT_WLAN_STA			pEntry;
	BOOLEAN					bEncrypt = FALSE;
#endif

	RT_PRINT_ADDR( (COMP_SEND|COMP_AUTHENTICATOR), DBG_LOUD, ("SendEapolKeyPacket(): StaAddr:\n"), StaAddr );

	if( ACTING_AS_AP(Adapter) )
	{	// AP mode.
		pEntry = AsocEntry_GetEntry(pMgntInfo, StaAddr);

		if(pEntry == NULL)
			return;
			
		if(pEntry->perSTAKeyInfo.TxMICKey != NULL)
			bEncrypt = TRUE;

		// AP WPA AES ,CCW
		if( pEntry->perSTAKeyInfo.AESKeyBuf[0] != 0)
		{
			bEncrypt = TRUE;
			//RT_TRACE(COMP_WPAAES, DBG_LOUD, ("=====> Packet to Encrypt start"));
		}
	}
	else
	{	// STA mode. Added by Annie, 2005-07-25.
		// Follow the same condition in TranslateHeader().
		if(	Adapter->MgntInfo.SecurityInfo.KeyLen[0]
			+Adapter->MgntInfo.SecurityInfo.KeyLen[1]
			+Adapter->MgntInfo.SecurityInfo.KeyLen[2]
			+Adapter->MgntInfo.SecurityInfo.KeyLen[3]
			+Adapter->MgntInfo.SecurityInfo.KeyLen[4] != 0)
		{
			bEncrypt = TRUE;
		}
	}

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructEapolKeyPacket(
			Adapter,
			pBuf->Buffer.VirtualAddress,
			&(pTcb->PacketLength),
			StaAddr,
			pKCK, // Pointer to KCK (EAPOL-Key Confirmation Key).
			pKEK, // Pointer to KEK (EAPOL-Key Encryption Key).
			eKeyType, // EAPOL-Key Information field: Key Type bit: type_Group or type_Pairwise.
			bInstall, // EAPOL-Key Information field: Install Flag.
			bKeyAck, // EAPOL-Key Information field: Key Ack bit.
			bKeyMIC, // EAPOL-Key Information field: Key MIC bit. If true, we will calculate EAPOL MIC and fill it into Key MIC field. 
			bSecure, // EAPOL-Key Information field: Secure bit.
			bError, // EAPOL-Key Information field: Error bit. True for MIC failure report.
			bRequest, // EAPOL-Key Information field: Requst bit.
			u8bKeyReplayCounter, // EAPOL-KEY Replay Counter field.
			pKeyNonce, // EAPOL-Key Key Nonce field (32-byte).
			u8bKeyRSC, // EAPOL-Key Key RSC field (8-byte).
			posRSNIE, // Key Data field: Pointer to RSN IE, NULL if 
			pGTK, // Key Data field: Pointer to GTK, NULL if Key Data Length = 0.
			bEncrypt	// encrypt the packet or not.
			);

//		if(eKeyType == type_Group)
		if( bEncrypt )
		{
			pTcb->EncInfo.SecProtInfo = RT_SEC_EAPOL_AFTER_KEY_INSTALLED;
		}
		else
		{
			pTcb->EncInfo.SecProtInfo = RT_SEC_EAPOL_BEFORE_KEY_INSTALLED;
		}

		// Check if this is a MIC failure report packet. 2005.09.06, by rcnjko.
		if(bRequest && bError)
		{
			pTcb->EncInfo.SecProtInfo |= RT_SEC_MIC_FAILURE_REPORT;	
		}

		DataRate = pMgntInfo->LowestBasicRate;
		pTcb->bTxUseDriverAssingedRate = TRUE;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, LOW_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}

//
// Description:
//	Send the raw packet to HW. This function may not work properly for all ICs,
//	there are only 8192C series are verified.
//	In normal sending case, use MgntSendSepcificPacket() instead of this function.
//	We use this function test the duration and special packets.
//	We simulate the CTS/NullData generation with the required duration for WiFi 11n
//	TestPlan 5.2.3.
// Argumetns:
//	[in] Adapter -
//		The NIC adapter context.
//	[in] pocPacketToSend -
//		The raw packet to be sent in the air.
//	[in] u1bTxRate -
//		The sending rate for this packet.
// Return:
//	If sending succeeds, return TRUE, and return FALSE otherwise.
// By Bruce, 2011-07-21.
//
BOOLEAN
MgntSendRawPacket(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pocPacketToSend,
	IN	u1Byte			u1bTxRate
	)
{
	BOOLEAN					bResult = FALSE;	
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;
	u4Byte					Crc32 = 0;
	pu1Byte					pHeader = NULL;
	
	if(	pocPacketToSend == NULL || 
		pocPacketToSend->Length > 1600 )
	{		
		return bResult;
	}

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	do
	{
		if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
		{
			pHeader = pBuf->Buffer.VirtualAddress;
			
			PlatformMoveMemory(pHeader, pocPacketToSend->Octet, pocPacketToSend->Length);		
			pTcb->PacketLength = pocPacketToSend->Length;			

			// Caculate CRC
			// The 8192C HW would not append the CRC for this packet when the RAW Packet bit is set in the Tx Desc.
			Crc32 = crc32(pHeader, pTcb->PacketLength);
			WriteEF4Byte(pHeader + pTcb->PacketLength, Crc32);
			pTcb->PacketLength += 4;

			SET_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_RAW_DATA);
			DataRate = u1bTxRate;
			pTcb->bTxUseDriverAssingedRate = TRUE;
			pTcb->bTxCalculatedSwDur = TRUE;
			pTcb->bTxEnableSwCalcDur = TRUE;

			MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
			RT_PRINT_DATA(COMP_SEND, DBG_LOUD, "MgntSendRawPacket() Content:\n", pHeader, pTcb->PacketLength);
			bResult = TRUE;
		}
	}while(FALSE);
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	return bResult;
}

//
//	Description: Send an 802.11 frame whose content is specified in ocOctet.
//	2005.09.23, by rcnjko.
//
BOOLEAN
MgntSendSpecificPacket(
	PADAPTER		Adapter,
	POCTET_STRING	pocPacketToSend,
	u1Byte			u1bTxRate
)
{
	PMGNT_INFO				pMgntInfo = NULL;
	PSTA_QOS				pStaQos = NULL;
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;
	BOOLEAN					bResult = FALSE;	
	PRT_SECURITY_T			pSecInfo = NULL;
	PADAPTER				pSendAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO 				pDefaultMgntInfo = GetDefaultMgntInfo(Adapter);

	//RT_TRACE(COMP_P2P, DBG_LOUD, ("MgntSendSpecificPacket(): send pkt with rate: %u\n", u1bTxRate));

	if(	pocPacketToSend == NULL || 
		pocPacketToSend->Length > 1600 )
	{		
		return bResult;
	}


	// check each adapter and find the one matching this packet
	do
	{
		// If the client is not assciated or connected.
		if(pSendAdapter->MgntInfo.mAssoc && eqMacAddr(Frame_pBssid((*pocPacketToSend)), pSendAdapter->MgntInfo.Bssid))
			break;;
	}while(NULL != (pSendAdapter = GetNextExtAdapter(pSendAdapter)));
	// If no adapter matches this packet, use default adapter.
	if(pSendAdapter == NULL)
		pSendAdapter = GetDefaultAdapter(Adapter);
	
	// Adopt the variables from sender adapter.
	pMgntInfo = &(pSendAdapter->MgntInfo);
	pStaQos = pMgntInfo->pStaQos;
	pSecInfo = &(pMgntInfo->SecurityInfo);

	PlatformAcquireSpinLock(pSendAdapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(pSendAdapter, &pTcb, &pBuf))
	{
		if(IsDataFrame(pocPacketToSend->Octet) && pocPacketToSend->Length > sMacHdrLng)
		{
			u2Byte	offset = 0;

			offset = sMacHdrLng;

			pTcb->PacketLength = 0;
			
			PlatformMoveMemory(pBuf->Buffer.VirtualAddress, pocPacketToSend->Octet, offset);
			pTcb->PacketLength += sMacHdrLng;

			//-------------------------------------------------------------------------
			// Qos Header: leave space for it if necessary.
			//-------------------------------------------------------------------------
			if(pStaQos->CurrentQosMode > QOS_DISABLE)
			{
				SET_80211_HDR_QOS_EN(pBuf->Buffer.VirtualAddress, 1);
				PlatformZeroMemory((pBuf->Buffer.VirtualAddress + offset), sQoSCtlLng);
				offset += sQoSCtlLng;
				pTcb->PacketLength += sQoSCtlLng;
			}

			//-------------------------------------------------------------------------
			// Security Header: leave space for it if necessary.
			// For WPS AP mode, We won't make EAP Packet with Security IE
			//-------------------------------------------------------------------------
			if((Frame_WEP(*pocPacketToSend) && pSecInfo->EncryptionHeadOverhead > 0 && WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0)) ||
				(pSecInfo->EncryptionHeadOverhead > 0 && (!GET_SIMPLE_CONFIG_ENABLED(pDefaultMgntInfo)) && FALSE == WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0)) )
			{
				PlatformZeroMemory((pBuf->Buffer.VirtualAddress + offset), pSecInfo->EncryptionHeadOverhead);
				offset += pSecInfo->EncryptionHeadOverhead;
				pTcb->PacketLength += pSecInfo->EncryptionHeadOverhead;
			}
			PlatformMoveMemory((pBuf->Buffer.VirtualAddress + offset), (pocPacketToSend->Octet + sMacHdrLng), (pocPacketToSend->Length - sMacHdrLng));
			pTcb->PacketLength += (pocPacketToSend->Length - sMacHdrLng);
		}
		else
		{
			PlatformMoveMemory(pBuf->Buffer.VirtualAddress, pocPacketToSend->Octet, pocPacketToSend->Length);
			pTcb->PacketLength = pocPacketToSend->Length;
		}

		DataRate = u1bTxRate;
		pTcb->bTxUseDriverAssingedRate = TRUE;

		MgntSendPacket(pSendAdapter, pTcb, pBuf, pTcb->PacketLength, BE_QUEUE, DataRate);
		bResult = TRUE;
	}

	PlatformReleaseSpinLock(pSendAdapter, RT_TX_SPINLOCK);

	return bResult;
}


//
//	Description: Send EAPOL-Start packet.
//
VOID
SendEAPOLStarPacket(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN				bEncrypt 
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;


	RT_TRACE(COMP_SEND, DBG_TRACE, ("+ SendCcxRmReport()\n"));

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructEAPOLStartPacket(
			Adapter,
			pBuf->Buffer.VirtualAddress,
			&(pTcb->PacketLength),
			bEncrypt
			);

		if( bEncrypt )
		{
			pTcb->EncInfo.SecProtInfo = RT_SEC_EAPOL_AFTER_KEY_INSTALLED;
		}
		else
		{
			pTcb->EncInfo.SecProtInfo = RT_SEC_EAPOL_BEFORE_KEY_INSTALLED;
		}

		RT_PRINT_DATA(COMP_SEC , DBG_LOUD , "==>  Send EAPOL Start \n" , pBuf->Buffer.VirtualAddress , pTcb->PacketLength);
		
		DataRate = pMgntInfo->LowestBasicRate;
		pTcb->bTxUseDriverAssingedRate = TRUE;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, LOW_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

}

//
//	Description: Send CCX Radio Measure Report packet.
//	2006.05.07, by rcnjko.
//
VOID
SendCcxRmReport(
	IN	PADAPTER			Adapter,
	IN	PRT_RM_REPORTS		pRmReports
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;


	RT_TRACE(COMP_SEND, DBG_TRACE, ("+ SendCcxRmReport()\n"));

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructCcxRmReport(
			Adapter,
			pBuf->Buffer.VirtualAddress,
			&(pTcb->PacketLength),
			pRmReports
			);

		DataRate = pMgntInfo->LowestBasicRate;
		pTcb->bTxUseDriverAssingedRate = TRUE;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, LOW_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	RT_TRACE(COMP_SEND, DBG_TRACE, ("- SendCcxRmReport()\n"));
}

//
// Description:
//	Send Dot11k radiio measurement report.
// Argument:
//	pRmRequests - The rm request information.
//	pRmReports - The rm report information.
// By Bruce, 2009-07-23.
//
VOID
SendDot11kRmReport(
	IN	PADAPTER			Adapter,
	IN	PRT_RM_REQUESTS		pRmRequests,
	IN	PRT_RM_REPORTS		pRmReports
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;


	RT_TRACE(COMP_SEND, DBG_TRACE, ("+ SendCcxRmReport()\n"));

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructDot11kRmReport(
			Adapter,
			pBuf->Buffer.VirtualAddress,
			&(pTcb->PacketLength),
			pRmRequests,
			pRmReports
			);

		pTcb->bTxUseDriverAssingedRate = TRUE;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, pMgntInfo->LowestBasicRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	RT_TRACE(COMP_SEND, DBG_TRACE, ("- SendCcxRmReport()\n"));
}


//
//	Description:
//		Send ADDTS packet
//	2007.06.25, by shien chang.
//
VOID
SendQosAddTs(
	IN	PADAPTER			Adapter,
	IN	PQOS_TSTREAM		pTs,
	IN	u4Byte				numTspec
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS				pStaQos = pMgntInfo->pStaQos;
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("===> SendQosAddTs()\n"));
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructQosADDTSPacket(
			Adapter,
			pBuf->Buffer.VirtualAddress,
			&(pTcb->PacketLength),
			(++ pStaQos->DialogToken),
			pTs,
			numTspec
			);

		DataRate = pMgntInfo->LowestBasicRate;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	RT_TRACE(COMP_QOS, DBG_TRACE, ("<=== SendQosAddTs()\n"));
}

//
//	Description:
//		Send DELTS packet
//	2007.06.25, by shien chang.
//
VOID
SendQosDelTs(
	IN	PADAPTER			Adapter,
	IN	PQOS_TSTREAM		pTs,
	IN	u4Byte				numTspec
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("===> SendQosDelTs()\n"));
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructQosDELTSPacket(
			Adapter,
			pBuf->Buffer.VirtualAddress,
			&(pTcb->PacketLength),
			pTs,
			numTspec
			);

		DataRate = pMgntInfo->LowestBasicRate;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	RT_TRACE(COMP_QOS, DBG_LOUD, ("<=== SendQosDelTs()\n"));
}

//
// Description:
//	AP Send Qos Null packet with EOSP bit set.
//Arguments:
//	[in] Adapter -
//		The NIC adapter.
//	[in] StaAddr -
//		The MAC address who will receive this qos null packet.
//	[in] bQos -
//		Determine if this packet is qos (wmm) packet.
//	[in] AC -
//		The qos AC queue filled in this Qos NULL fumction data.
//	[in] bEosp -
//		End of service bit indicates this packet ends the service period.
// Return:
//	None.
// By Bruce, 2010-04-26.
//
VOID
ApSendNullPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			StaAddr,
	IN	BOOLEAN			bQos,
	IN	u1Byte			AC,
	IN	BOOLEAN			bEosp
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;

	RT_TRACE(COMP_AP, DBG_TRACE, ("===> ApSendNullPacket()\n"));
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		//Add bQoS parameter for QoS Null by Isaiah 2006-07-31
		ConstructNullFunctionData(
			Adapter, 
			pBuf->Buffer.VirtualAddress,
			&pTcb->PacketLength,
			StaAddr,
			bQos,
			AC,
			bEosp,
			FALSE);

		DataRate = pMgntInfo->LowestBasicRate;			// Annie, 2005-03-31		
		pTcb->bTxUseDriverAssingedRate = TRUE;
		pTcb->priority = AC;
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	RT_TRACE(COMP_AP, DBG_TRACE, ("<=== ApSendNullPacket()\n"));
}

//
// Description:
//	Send SA Query request packet.
//Arguments:
//	[in] Adapter -
//		The NIC adapter.
//	[in] pTargetSta -
//		Target STA which receives this frame.
//	[in] Identifier -
//		The identifier in the SA query request frame.
// Return:
//	None.
// By Bruce, 2014-12-31.
//
VOID
SendSAQueryReq(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pTargetSta,
	IN	u2Byte			Identifier
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructSAQeuryReq(
			Adapter, 
			pBuf->Buffer.VirtualAddress,
			&pTcb->PacketLength,
			pTargetSta,
			Identifier);

		DataRate = pMgntInfo->LowestBasicRate;			// Annie, 2005-03-31		
		pTcb->bTxUseDriverAssingedRate = TRUE;
		RT_PRINT_DATA(COMP_SEC, DBG_TRACE, "SendSAQueryReq(): Content:\n", pBuf, pTcb->PacketLength);
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}


//
// Description:
//	Send SA Query response packet.
//Arguments:
//	[in] Adapter -
//		The NIC adapter.
//	[in] pTargetSta -
//		Target STA which receives this frame.
//	[in] Identifier -
//		The identifier from the SA query request frame to fill in the SA query response.
// Return:
//	None.
// By Bruce, 2014-12-25.
//
VOID
SendSAQueryRsp(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pTargetSta,
	IN	u2Byte			Identifier
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;
	
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructSAQeuryRsp(
			Adapter, 
			pBuf->Buffer.VirtualAddress,
			&pTcb->PacketLength,
			pTargetSta,
			Identifier);

		DataRate = pMgntInfo->LowestBasicRate;			// Annie, 2005-03-31		
		pTcb->bTxUseDriverAssingedRate = TRUE;
		RT_PRINT_DATA(COMP_SEC, DBG_TRACE, "SendSAQueryRsp(): Content:\n", pBuf, pTcb->PacketLength);
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}


