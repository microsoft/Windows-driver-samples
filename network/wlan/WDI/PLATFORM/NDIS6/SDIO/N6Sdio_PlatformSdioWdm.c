/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	N6Sdio_PlatformSdioWdm.c
	
Abstract:
	Implement PlatformSdioXXX() on WDM.
	Prototype of WdmSdio_XXX() implemented via WDM SDIO.
	These function shall be only use under Platform\.	   
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2012-01-13 Roger            Create.
	
--*/

#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "N6Sdio_PlatformSdioWdm.tmh"
#endif

//================================================================================
//	Constant.
//================================================================================



//================================================================================
//	Prototype of protected function.
//================================================================================
RT_STATUS
N6SdioTxComplete(	
	IN	PVOID			Context,
	IN	BOOLEAN		bResult
	);


NTSTATUS
N6SdioHandleInterrupt(
	PVOID	Context, 
	u4Byte		InterruptType
);


//================================================================================
//	Bulk IN.
//================================================================================

//
//	Description:
//		Allocate URB and IRP for this SDIO_OUT_CONTEXT object.
//		It return TRUE on success, FALSE otherwise.
//
BOOLEAN 
PlatformSdioInitInContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_IN_CONTEXT			pContext
	)
{
	PRT_SDIO_DEVICE	pDevice = GET_RT_SDIO_DEVICE(pAdapter);	

	pContext->PlatformReserved[0] = pAdapter;	
	pContext->PlatformReserved[1] = NULL;	
	pContext->PlatformReserved[2] = NULL;	
	pContext->PlatformReserved[3] = NULL;	

	return TRUE;
}


//
//	Description:
//		Free URB and IRP allocated for this USB_IN_CONTEXT object.
//
VOID 
PlatformSdioDeInitInContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_IN_CONTEXT			pContext
	)
{
	pContext->PlatformReserved[0] = NULL;	
	pContext->PlatformReserved[1] = NULL;	
	pContext->PlatformReserved[2] = NULL;	
	pContext->PlatformReserved[3] = NULL;	
}


//================================================================================
//	SDIO Tx Transfer
//================================================================================

//
//	Description:
//		Allocate resources for this SDIO_OUT_CONTEXT object.
//		It return TRUE on success, FALSE otherwise.
//
BOOLEAN 
PlatformSdioInitTxContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext
	)
{
	PRT_SDIO_DEVICE	pDevice = GET_RT_SDIO_DEVICE(pAdapter);

	pContext->PlatformReserved[0] = pAdapter;	
	pContext->PlatformReserved[1] = NULL;	
	pContext->PlatformReserved[2] = NULL;	
	pContext->PlatformReserved[3] = NULL;	

	return TRUE;
}



//
//	Description:
//		Free allocated resources for this SDIO_OUT_CONTEXT object.
//
VOID 
PlatformSdioDeInitTxContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext
	)
{
	pContext->PlatformReserved[0] = NULL;	
	pContext->PlatformReserved[1] = NULL;	
	pContext->PlatformReserved[2] = NULL;	
	pContext->PlatformReserved[3] = NULL;	
}



//
//	Description:
//		This SDIO platform dependent routine enqueue a specific SDIO Data transfer context into the busy queue
//	and waits for post processing.
//
//	Assumption:
//		RT_TX_SPINLOCK is acquired.
//
//	Created by Roger, 2011.01.20.
//
BOOLEAN
PlatformSdioTxEnqueue(
	IN	PADAPTER			pAdapter,
	IN	PSDIO_OUT_CONTEXT	pContext
	)
{
	PRT_SDIO_DEVICE		pDevice 	= GET_RT_SDIO_DEVICE(pAdapter);
	BOOLEAN				bResult 	= TRUE;
	u1Byte				TxQueueIdx	= pContext->TxQueueIndex;
	PRT_SDIO_TX_QUEUE	pTxQueue 	= &(pDevice->RtTxQueue[TxQueueIdx]);	
	PRT_TCB	 			pTcb 		= pContext->pTcb;

	RT_TRACE(COMP_SEND, DBG_TRACE, ("===>PlatformSdioTxEnqueue(): Context(%p)\n", pContext));

	// If IrpPendingCount is 0, we shall take it as a signal that SDIO Tx Queue is disabled.
	if ((pTxQueue->IrpPendingCount < 1) || (!pTxQueue->bEnabled))
	{
		RT_TRACE(COMP_SEND, DBG_LOUD, 
			("PlatformSdioTxEnqueue() exit because of Tx Queue is not enabled or IrpPendingCount == 0 !!\n"));
		return FALSE;
	}

	if(RT_SDIO_CANNOT_TX(pAdapter))
	{
		bResult = FALSE;
		return bResult;
	}

	RTInsertTailList( &(pTxQueue->ContextBusyList), &(pContext->List));
	pTxQueue->IrpPendingCount++;
	pTcb->sysTime[1] = PlatformGetCurrentTime();
	pAdapter->firstTcbSysTime[TxQueueIdx] = pTcb->sysTime[1];
	
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

	// Releases the specified Tx semaphore object for SDIO Data transfer.
	PlatformReleaseSemaphore(&pDevice->TxSemaphore);
	//PlatformReleaseSemaphore(&pAdapter->txGen.txGenSemaphore);
	
	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	RT_TRACE(COMP_SEND, DBG_TRACE, ("<===PlatformSdioTxEnqueue(): Context(%p)\n", pContext));
	
	return bResult;
}

//
//	Description:
//		This SDIO platform dependent routine dequeue a specific SDIO Data transfer context from the busy queue.
//
//	Assumption:
//		RT_TX_SPINLOCK is acquired.
//
//	Created by Roger, 2011.01.20.
//
PSDIO_OUT_CONTEXT
PlatformSdioTxDequeue(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				QueueIdx
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)pAdapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);	
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;
	PSDIO_OUT_CONTEXT	pContext = NULL;	
	
	pTxQueue= &(sdiodevice->RtTxQueue[QueueIdx]);		
	RT_ASSERT((!RTIsListEmpty(&(pTxQueue->ContextBusyList))), ("Tx Queue is empty!!"));
	pContext = (PSDIO_OUT_CONTEXT)RTRemoveHeadList(&(pTxQueue->ContextBusyList));

	return pContext;	
}


//
//	Description:
//		This SDIO platform dependent routine for SDIO CMD53 block mode data transfer.
//
//	Assumption:
//		RT_TX_SPINLOCK is NOT acquired. and driver must refresh Tx free page number by reading 
//	FREE_TXPG page Register to make sure that the free page is enough.
//
//	Created by Roger, 2011.01.20.
//
BOOLEAN
PlatformSdioTxTransfer(
	IN	PADAPTER			pAdapter,
	IN	PSDIO_OUT_CONTEXT	pContext
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)pAdapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);	
	u1Byte		DeviceID = 0;
	u4Byte		offset = 0; // Aggregation length [12:0] is zero.
	BOOLEAN		bResult = TRUE;
	BOOLEAN		bMacPwrCtrlOn;
	RT_RF_POWER_STATE rfState;
	RT_STATUS	status = RT_STATUS_SUCCESS;
	PRT_TCB	 pTcb = pContext->pTcb;


	RT_TRACE(COMP_SEND, DBG_TRACE, ("--->PlatformSdioTxTransfer(): Context(%p), BufLen(%#x), TxQueueIndex(%d)\n", 
				pContext, pContext->BufLen, pContext->TxQueueIndex));
	//RT_PRINT_DATA(COMP_SEND, DBG_TRACE, ("TxBuffer:"), pContext->Buffer, pContext->BufLen);

	if(RT_SDIO_CANNOT_TX(pAdapter))
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("PlatformSdioTxTransfer(): SDIO not allow to TX, Return Fail.\n"));
		return FALSE;
	}

	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));	
	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));

	// Do not drop Tx packet which is from BCN queue during RF off state to prevent from ROM download FW
	// failed in RF on progress. Only protect power off case by "bMacPwrCtrlOn". 2013.11.21, tynli.
	if(((rfState == eRfOff) && (pTcb->SpecifiedQueueID != BEACON_QUEUE)) || (bMacPwrCtrlOn != TRUE))
	{// This packet should not be sent if RF is OFF, drop it !!		
		RT_TRACE(COMP_SEND, DBG_TRACE, ("<---PlatformSdioTxTransfer(): RF Off or Power Off!!\n"));
		return FALSE;
	}

	// Transfer I/O Bus domain address mapping.
	switch(pContext->TxQueueIndex)
	{
		case 0: // HIQ
			DeviceID = WLAN_TX_HIQ_DEVICE_ID;
			break;

		case 1: // MIQ
			DeviceID = WLAN_TX_MIQ_DEVICE_ID;
			break;

		case 2: // LOQ
			DeviceID = WLAN_TX_LOQ_DEVICE_ID;
			break;
		case 3:
			DeviceID = WLAN_TX_EXTQ_DEVICE_ID;
			break;
		default:
			RT_ASSERT(FALSE, ("Incorrect SDIO TxQueue Index(%d)\n", pContext->TxQueueIndex));
			DeviceID = WLAN_TX_LOQ_DEVICE_ID;
			break;
	}
	
	// Beacon queue is not related to HI, LOW, NORMAL and PUBLIC queue. It will be sent to reserved page directly
	// so we just need to check the reserved page size in Tx buffer. 2013.01.04. by tynli.
	
	RT_ASSERT((pContext->BufLen + (pContext->BufLen%4?(4-pContext->BufLen%4):0) <= pAdapter->MAX_TRANSMIT_BUFFER_SIZE), ("PlatformSdioTxTransfer(): Over MAX_TRANSMIT_BUFFER_SIZE\n"));
	
	if(pTcb->SpecifiedQueueID != BEACON_QUEUE	)
	{
		if((pContext->BufLen + (pContext->BufLen%4?(4-pContext->BufLen%4):0) > pAdapter->MAX_TRANSMIT_BUFFER_SIZE))
			return FALSE;
	}
	
	//Using Burst Tx mode to transfer multiple blocks.
	status = PlatformSdioCmd53ReadWrite(
						sdiodevice, 
						DeviceID,
						sdiodevice->SdioFuncNum, 
						pContext->BufLen + (pContext->BufLen%4?(4-pContext->BufLen%4):0), 
						((pContext->BufLen/4) + (pContext->BufLen%4?1:0)& 0x1FFF),// Aggregation length [12:0]
						TRUE,
						pContext->Buffer);	
	
	RT_ASSERT(((pContext->BufLen + (pContext->BufLen%4?(4-pContext->BufLen%4):0)) % ((pContext->BufLen/4) + (pContext->BufLen%4?1:0)& 0x1FFF)) ==0, 
		("PlatformSdioTxTransfer(): Invalid transfer length!!\n"));
	
	if(status != RT_STATUS_SUCCESS)	
		return FALSE;	

	RT_TRACE(COMP_SEND, DBG_TRACE, ("<---PlatformSdioTxTransfer(): Context(%p), TxQueueIndex(%d), bResult(%d)\n", 
				pContext, pContext->TxQueueIndex, bResult));

	return bResult;
}


//
//	Description:
//		This SDIO platform dependent routine for SDIO CMD53 block mode RX data transfer.
//
//	Assumption:
//		RT_RX_SPINLOCK is NOT acquired. We whould Query the length of RX Request ready in RXPKTBUF0
//	first and transfer corrsponding buffer length.
//
//	Created by Roger, 2011.01.20.
//
BOOLEAN
PlatformSdioRxTransfer(
	IN	PADAPTER	pAdapter,
	IN	PRT_RFD		pRfd,
	IN	u4Byte		Rx0ReqLength	
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)pAdapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	RT_RF_POWER_STATE rfState;
	BOOLEAN		bMacPwrCtrlOn;
	u2Byte		RxDesc2Bytes= 0;
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;	
	u1Byte			FwPSState;
	BOOLEAN		bRxTransSuccess = TRUE;

	RT_TRACE(COMP_RECV, DBG_TRACE, ("--->PlatformSdioRxTransfer()\n"));
	
	if(RT_SDIO_CANNOT_RX(pAdapter))
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("PlatformSdioTxTransfer(): SDIO not allow to RX, Return Fail.\n"));
		return FALSE;
	}
	
	PlatformAcquireSpinLock(pAdapter, RT_RX_SPINLOCK);
	RT_SDIO_INC_RX_TRANS_REF(sdiodevice);
	PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);

	do
	{
		PlatformAcquireSpinLock(pAdapter, RT_RX_SPINLOCK);
		if(sdiodevice->bStopRxTransfer)
		{
			PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);
			RT_TRACE(COMP_POWER, DBG_LOUD, ("<---PlatformSdioRxTransfer(): Return becuase of bStopRxTransfer\n"));
			//return FALSE;
			break;
		}
		PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);
		
		pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_RF_STATE, (pu1Byte)(&rfState));	
		pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));

		if(((rfState == eRfOff) && (!RT_IN_PS_LEVEL(pAdapter, RT_RF_OFF_LEVEL_FW_IPS_32K))) || (bMacPwrCtrlOn != TRUE))
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("<---PlatformSdioRxTransfer(): RF Off or Power Off!!\n"));
			bRxTransSuccess = FALSE;
			break;
		}	

		// Leave 32K when there is an Rx packet to receive. It is a workaround to patch FW enters 32K when
		// driver is handling DMA packet then cause Rx cmd fail and Rx0 packet length always zero issue.
		// Added by tynli, 2014.09.09. Suggested by SD1 Kaiyuan.
		if(pDefaultAdapter->bRxPsWorkAround)
		{
			if(pDefaultAdapter->bFWReady)
			{
				pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);		
				if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState))
				{
					RT_TRACE(COMP_POWER, DBG_LOUD, ("PlatformSdioRxTransfer(): In 32K ---> Wake up Hw. \n"));
					pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
				}
			}
		}

		//Using Burst Rx mode to transfer multiple blocks.
		rtStatus = PlatformSdioCmd53ReadWrite(
							sdiodevice, 
							WLAN_RX0FF_DEVICE_ID,
							sdiodevice->SdioFuncNum, 
							Rx0ReqLength, 
							(ULONG)(RT_SDIO_GET_RX_SEQ_NUM(pAdapter)%4), //SEQ[1:0] in RX_RX0FF, Shift Bits = 0
							FALSE,
							pRfd->Buffer.VirtualAddress);

		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_POWER, DBG_WARNING, ("<---PlatformSdioRxTransfer(): status fail\n"));
			PlatformAcquireSpinLock(pAdapter, RT_RX_SPINLOCK);
			RT_SDIO_INC_RX_SEQ_NUM(pAdapter);
			PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);
			bRxTransSuccess = FALSE;
			break;
		}
		
		PlatformAcquireSpinLock(pAdapter, RT_RX_SPINLOCK);

		// Increase SDIO Rx sequence number
		RT_SDIO_INC_RX_SEQ_NUM(pAdapter);


		// [13:0]: PktLen, [14]: CRC32 and [15]: ICVERR, total 2Bytes should not contain zero content
		if(PlatformCompareMemory(pRfd->Buffer.VirtualAddress, &RxDesc2Bytes, sizeof(RxDesc2Bytes)) == 0)
		{
			PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);
			bRxTransSuccess = FALSE;
			break;
		}

		PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);
	}while(FALSE);


	PlatformAcquireSpinLock(pAdapter, RT_RX_SPINLOCK);
	RT_SDIO_DEC_RX_TRANS_REF(sdiodevice);
	if(RT_SDIO_GET_RX_TRANS_REF(sdiodevice) == 0)
	{
		PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);
		NdisSetEvent(&sdiodevice->AllSdioRxTransCompleteEvent);
		RT_TRACE(COMP_RECV, DBG_LOUD, ("--->NdisSetEvent: AllSdioRxTransCompleteEvent\n"));
		PlatformAcquireSpinLock(pAdapter, RT_RX_SPINLOCK);
	}
	PlatformReleaseSpinLock(pAdapter, RT_RX_SPINLOCK);

	RT_TRACE(COMP_RECV, DBG_TRACE, ("<---PlatformSdioRxTransfer()\n"));
	return TRUE;

}

//
//	Description:
//		Check if Sdio Tx busy queue is empty.
//	Input: 
//		QueueID - Specified ID, ex: VO, VI, BE, BK
//
//	Created by tynli, 2014.07.14.
//
BOOLEAN
PlatformSdioTxQueueIdxEmpty(
	IN  PADAPTER 	Adapter,
	IN  u1Byte		QueueID
)
{
	PRT_SDIO_DEVICE 	sdiodevice 		= GET_RT_SDIO_DEVICE(Adapter);
	PRT_SDIO_TX_QUEUE	pTxQueue 		= NULL;
	u1Byte				SdioTxQueueIdx	= QueueID;	// Only search for those 3 queue in sdio
	BOOLEAN				bEmpty 			= TRUE;
	
	//SdioTxQueueIdx = MapTxQueueToOutPipe(Adapter, QueueID);// Get index to out pipe from specified QueueID.

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	pTxQueue= &(sdiodevice->RtTxQueue[SdioTxQueueIdx]);
	if(!RTIsListEmpty(&(pTxQueue->ContextBusyList)))
	{
		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
		RT_TRACE(COMP_SEND, DBG_LOUD, ("SdioTxQueueIdx[%d] is not empty!!!\n", SdioTxQueueIdx));
		bEmpty = FALSE;
	}
	else
	{
		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
	}

	return bEmpty;
}


//
//	Description:
//		Check if Sdio Tx busy queue and Awb I/O queue is empty.
//
//	Created by tynli, 2011.09.01.
//
BOOLEAN
PlatformSdioTxAndAwbQueueEmpty(
	IN  PADAPTER 	Adapter
)
{
	PRT_SDIO_DEVICE 		sdiodevice = GET_RT_SDIO_DEVICE(Adapter);
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;
	u1Byte				QueueID;
	BOOLEAN				bEmpty = TRUE;
	
	// Check if Tx queue empty.
	for(QueueID=0; QueueID<SDIO_MAX_TX_QUEUE; QueueID++)
	{
		PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
		pTxQueue= &(sdiodevice->RtTxQueue[QueueID]);
		if(!RTIsListEmpty(&(pTxQueue->ContextBusyList)))
		{
			PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
			RT_TRACE(COMP_POWER, DBG_LOUD, ("QueueID[%d] is not empty!!!\n", QueueID));
			bEmpty = FALSE;
			break;
		}
		else
		{
			PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
		}
	}

	// Check Awb I/O queue empty.
	PlatformAcquireSpinLock(Adapter, RT_AWB_SPINLOCK);
	if(!RTIsListEmpty( &(sdiodevice->AwbWaitQueue)))
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("AwbWaitQueue is not empty!!!\n"));
		bEmpty = FALSE;
	}
	PlatformReleaseSpinLock(Adapter, RT_AWB_SPINLOCK);

	// Check sync IO empty.
	NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	if(sdiodevice->SyncIoInProgressCount > 0)
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("SyncIoInProgressCount(%d) is not empty!!!\n", sdiodevice->SyncIoInProgressCount));
		bEmpty = FALSE;
	}
	NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

	return bEmpty;
}

//
//	Description:
//		Callback function for the post processing of PlatformSdioTxEnqueue from SDIO Tx thread.
//
//	Assumption:
//		RT_TX_SPINLOCK is NOT acquired.
//
//	Created by Roger, 2011.02.01.
//
RT_STATUS
N6SdioTxComplete(	
	IN	PVOID			Context,
	IN	BOOLEAN			bResult
	) 
{
	PSDIO_OUT_CONTEXT	pThisContext = (PSDIO_OUT_CONTEXT)Context; 
	PRT_SDIO_DEVICE	pDevice = NULL;
	PADAPTER			pAdapter = NULL;
	u1Byte				QueueIndex = 0;
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;
	PADAPTER 			pTargetAdapter = NULL;
	PRT_SDIO_DEVICE		pTargetDevice = NULL;
	
	RT_ASSERT(pThisContext != NULL, ("N6SdioTxComplete(): pThisContext should not be NULL!!!\n"));
	QueueIndex = pThisContext->TxQueueIndex;

	pAdapter = (PADAPTER)pThisContext->PlatformReserved[0];
	RT_ASSERT(pAdapter != NULL, ("N6SdioTxComplete(%d): pAdapter should not be null!!!\n", QueueIndex));

	pDevice = GET_RT_SDIO_DEVICE(pAdapter);
	pTxQueue = &pDevice->RtTxQueue[QueueIndex];

	RT_TRACE(COMP_SEND, DBG_TRACE, ("===>N6SdioTxComplete(): Context(%p), QueueIndex(%d), IrpPendingCount(%d)\n", pThisContext, QueueIndex, pTxQueue->IrpPendingCount));

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(pThisContext->bTxPending)
	{
		pTxQueue->IrpPendingCount--; // Decrease IrpPendingCount which had been increased in PlatformSdioTxEnqueue().
	}
	else
	{
		RT_TRACE(COMP_SEND, DBG_SERIOUS, ("N6SdioTxComplete(): Context is already returned!!\n") );
	}
	
	if(pThisContext->bTxPending)
		pAdapter->HalFunc.HalSdioTxCompleteHandler(pAdapter, pThisContext, (bResult ? HAL_SDIO_COMPLETED_OK : HAL_SDIO_COMPLETED_ERROR));		

	PlatformReleaseSemaphore(&pAdapter->txGen.txGenSemaphore);
	//
	// Handle NBL packet for each port
	//
	pTargetAdapter = GetDefaultAdapter(pAdapter);
	pTargetDevice = GET_RT_SDIO_DEVICE(pTargetAdapter);

	while(pTargetAdapter != NULL)
	{
		if(N6SDIO_CANNOT_TX(pTargetAdapter))
		{
			//RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioTxComplete(): break for RT_SDIO_CANNOT_TX(%d), N6C_GET_MP_DRIVER_STATE(%d), NicIFGetLinkStatus(%d)\n",
			//	RT_SDIO_CANNOT_TX(pTargetAdapter),
			//	N6C_GET_MP_DRIVER_STATE(pTargetAdapter),
			//	NicIFGetLinkStatus(pTargetAdapter)));
			break;
		}

		do
		{
			if(PlatformAtomicExchange(&pTargetAdapter->IntrNBLRefCount, TRUE)==TRUE)
				break;

			//
			// Handle NBLs buffered. 
			//
			if(pTargetDevice->SendingNetBufferList == NULL) // Add this to prevent from Tx out-of-order.
			{
				PNET_BUFFER_LIST	pNetBufferList;

				while(TRUE)
				{
					PlatformAcquireSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
					if(N6CIsNblWaitQueueEmpty(pTargetAdapter->pNdisCommon->TxNBLWaitQueue))
					{
						PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
						break;
					}

					if(!pTargetAdapter->pNdisCommon->bReleaseNblWaitQueueInProgress)
					{
						pNetBufferList = N6CGetHeadNblWaitQueue(pTargetAdapter->pNdisCommon->TxNBLWaitQueue);
						PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);

						RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioTxComplete(): pTargetAdapter(%p) handle the NBL buffered(%p)\n", pTargetAdapter, pNetBufferList ));
						if( !N6SdioSendSingleNetBufferList(
								pTargetAdapter, 
								pNetBufferList,
								TRUE) ) // bFromQueue
						{
							RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioTxComplete(): N6usbSendSingleNetBufferList() returns FALSE\n"));
							break;
						}
					}
					else
					{
						RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioTxComplete(): bReleaseNblWaitQueueInProgress\n"));
						PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
					}
				}
			}

			PlatformAtomicExchange(&pTargetAdapter->IntrNBLRefCount, FALSE);

		}
		while(FALSE);

		// Get the next adapter
		pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
		if(pTargetAdapter != NULL)
		{
			pTargetDevice = GET_RT_SDIO_DEVICE(pTargetAdapter);
		}
	}


	//
	// Check if driver is going to unload.
	//
	if(pTxQueue->IrpPendingCount == 0)
	{ 
		RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioTxComplete(%d): IrpPendingCount==0 =>  Driver is going to unload.\n", QueueIndex));
		PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
		NdisSetEvent(&pTxQueue->AllIrpReturnedEvent);
	}
	else
	{
		PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
	}

	RT_TRACE(COMP_SEND, DBG_TRACE, ("<===N6SdioTxComplete(): Context(%p)\n", pThisContext));
	
	return RT_STATUS_SUCCESS;
}


//
//	Description: 
//		whether any context is pended in ContextBusyList.
//
//	Assumption:
//		RT_TX_SPINLOCK is NOT acquired.
//
//	2010.02.17, created by Roger.
//
BOOLEAN
N6SdioTxQueuePending(
	IN  PADAPTER 	Adapter
	)
{
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(Adapter);	
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;	
	BOOLEAN		bTxQueuePending = FALSE;
	u1Byte	i = 0;

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	
	for (i=0; i<SDIO_MAX_TX_QUEUE; i++)
	{
		pTxQueue= &(sdiodevice->RtTxQueue[i]);					
		
		if(!RTIsListEmpty(&(pTxQueue->ContextBusyList)))
		{
			bTxQueuePending = TRUE;
			break;											
		}
	}	
	
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

	RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioTxQueuePending(): return (%d)\n", bTxQueuePending));
	
	return bTxQueuePending;
}



//
//	Description: 
//		Release corresponding context in each ContextBusyList while driver is going to unload. 
//
//	Assumption:
//		RT_TX_SPINLOCK is NOT acquired.
//
//	2010.02.17, created by Roger.
//
BOOLEAN
N6SdioReleaseTxQueuePending(
	IN  PADAPTER 	Adapter
	)
{
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(Adapter);	
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;	
	BOOLEAN		bTxQueuePending = FALSE;
	PSDIO_OUT_CONTEXT	pOutContextToRelease = NULL;
	u1Byte	i = 0;

		
	for (i=0; i<SDIO_MAX_TX_QUEUE; i++)
	{
		PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
		
		pTxQueue= &(sdiodevice->RtTxQueue[i]);			
		
		while(!RTIsListEmpty(&(pTxQueue->ContextBusyList)))
		{
			pOutContextToRelease = (PSDIO_OUT_CONTEXT)RTRemoveHeadList( &(pTxQueue->ContextBusyList) );
			PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
			N6SdioTxComplete(pOutContextToRelease, FALSE);	
			PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
		}

		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
	}			

	RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioReleaseTxQueuePending(): return (%d)\n", bTxQueuePending));
	
	return bTxQueuePending;
}


#if RTL8723_SDIO_IO_THREAD_ENABLE 

//
//	Description: 
//		This user created thread handles the SDIO block synchronous write operation at PASSIVE_LEVEL.		
//
//	2011.06.23, created by Roger.
//
VOID
N6SdioIOThreadCallback(
	IN	PVOID	pContext
	)
{
	
	PADAPTER	Adapter = ((PRT_THREAD)pContext)->Adapter;
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(Adapter);		
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;	
	PSDIO_OUT_CONTEXT	pOutContextToSend = NULL;
	PRT_AWB 	pAwb = NULL;	
	PRT_AWB 	pAwb2Complete = NULL;
	BOOLEAN		bResult = TRUE;
	u4Byte		TargetAddr = 0;
	BOOLEAN		bMacPwrCtrlOn = FALSE;
	u1Byte		FwPSState = 0;
	BOOLEAN		bUpdatePreRpwm = FALSE;
	BOOLEAN		bFwClkChangeInProgress = FALSE;


	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		RT_ASSERT(FALSE, ("N6SdioIOThreadCallback() in PASSIVE_LEVEL is not allowed!!\n"));
		return;
	}	
	
	while(TRUE)
	{		
		
		//
		// We only need to take care of surprise removal or driver unload event to prevent IO thread function disable by 
		// Function BIT disable, e.g., DF_IO_BIT
		// 
		if( RT_DRIVER_HALT(Adapter) )
			break;
		
		if(!Adapter->bInitComplete)
			continue;
		
		if( PlatformAcquireSemaphore(&sdiodevice->IOSemaphore) != RT_STATUS_SUCCESS )
			break;

		//
		// <Roger_Notes> We need to take care of surprise removal, driver unload event and DF_IO_BIT to pause IO handling, 
		// and then return to previous waiting state.
		// 
		if( RT_USB_CANNOT_IO(Adapter) )
			continue;

		//2Check AWB Wait Queue.	
		PlatformAcquireSpinLock(Adapter, RT_AWB_SPINLOCK);
		if( !RTIsListEmpty( &(sdiodevice->AwbWaitQueue)))
		{
			PlatformReleaseSpinLock(Adapter, RT_AWB_SPINLOCK);			
			
			// Retrieve the first AWB to perform specific write operation
			PlatformAcquireSpinLock(Adapter, RT_AWB_SPINLOCK);			
			pAwb = (PRT_AWB)RTGetHeadList( &sdiodevice->AwbWaitQueue);			
			PlatformReleaseSpinLock(Adapter, RT_AWB_SPINLOCK);
			
			RT_TRACE(COMP_IO, DBG_TRACE, ("N6SdioIOThreadCallback(): Dequeue AWB, DeviceID(%d), Offset(%#x), ByteCnt(%#x), NumIdleAwb(%d)\n", pAwb->DeviceID, pAwb->Offset, pAwb->ByteCnt, sdiodevice->NumIdleAwb));
			RT_PRINT_DATA(COMP_IO, DBG_TRACE, "N6SdioIOThreadCallback(): Buffer:\n", 
				pAwb->DataBuf, pAwb->ByteCnt);

			//2<Roger_TODO> Check whether underlying HW is in power saving 32k mode.
			if(Adapter->bFWReady)
			{
				Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_FW_PS_STATE, &FwPSState);
				if(IS_IN_LOW_POWER_STATE(Adapter, FwPSState)  && 
					!IS_SDIO_POWER_ON_IO_REG(pAwb->DeviceID, pAwb->Offset))
				{		
					RT_TRACE(COMP_POWER, DBG_LOUD, 
						("N6SdioIOThreadCallback(): CANNOT IO---> Wake up Hw. DeviceID(%d), offset(%#X)\n", pAwb->DeviceID, pAwb->Offset));
					Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&Adapter));
				}
			}
			
			bUpdatePreRpwm = FALSE;
			
			// tynli_test. Check the register and update driver maintain variables. 2011.09.02.
			if(pAwb->DeviceID == SDIO_LOCAL_DEVICE_ID)
			{
				if(pAwb->Offset == SDIO_REG_HRPWM1)
				{
					if(pAwb->ByteCnt == 1)
					{
						bUpdatePreRpwm = TRUE;
					}
					else
					{
						RT_TRACE(COMP_IO, DBG_LOUD, ("N6SdioIOThreadCallback(): Cannot update pHalData->PreRpwmVal value and FwPSState!!!\n"));
					}
				}
			}


			// Perform write operatrion in specific device ID and offset
			PlatformIOSyncWriteNByte(Adapter, pAwb->DeviceID, pAwb->Offset, pAwb->ByteCnt, pAwb->DataBuf);	

			if(bUpdatePreRpwm)
			{				
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_PRE_RPWM, pAwb->DataBuf);
				bFwClkChangeInProgress = FALSE;
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_FW_CLK_CHANGE_STATE, (pu1Byte)(&bFwClkChangeInProgress));
				
				RT_TRACE(COMP_INIT, DBG_LOUD, ("N6SdioIOThreadCallback(): RPWM write OK. Release bFwClkChangeInProgress!!!\n"));
			}

			// Dequeue the single AWB we sent before and complete it.
			PlatformAcquireSpinLock(Adapter, RT_AWB_SPINLOCK);
			pAwb2Complete = (PRT_AWB)RTRemoveHeadListWithCnt( &(sdiodevice->AwbWaitQueue), &(sdiodevice->NumWaitAwb));
			RT_ASSERT((pAwb2Complete == pAwb), ("N6SdioIOThreadCallback(): AWB mismatched!!\n"));
			ReturnSdioAWB(sdiodevice, pAwb2Complete);
			PlatformReleaseSpinLock(Adapter, RT_AWB_SPINLOCK);
		}
		else
		{	
			PlatformReleaseSpinLock(Adapter, RT_AWB_SPINLOCK);
			RT_TRACE(COMP_IO, DBG_LOUD, ("N6SdioIOThreadCallback(): AWB QueueIdx is empty!!\n"));			
		}			
	}

	// Release all buffered AWBs before leaving this thread.
	PlatformAcquireSpinLock(Adapter, RT_AWB_SPINLOCK);
	while( !RTIsListEmpty(&(sdiodevice->AwbWaitQueue))  ) 
	{
		pAwb = (PRT_AWB)RTRemoveHeadListWithCnt( &(sdiodevice->AwbWaitQueue), &(sdiodevice->NumWaitAwb));
		ReturnSdioAWB(sdiodevice, pAwb);
	}
	PlatformReleaseSpinLock(Adapter, RT_AWB_SPINLOCK);	
}
#endif


//
//	Description: 
//		This user created thread handles the CMD53 Data Transfer on SDIO bus.
// 	Preparing for SDIO Data transfer:
// 	1. If all Context Queue is empty, then break.
// 	2. If leaving 32k power saving mode is fail, then break.
//
// 	SDIO Data transfer:
// 	1. Check Tx FIFO free page size, if available then dequeue from Context Queue and perform SDIO Data transfer.
// 	2. If all Context Queue is empty, then break. Back to 32k power saving mode if needed.
// 	3. if any Context Queue is in pendding status then continue.
//
//	2011.01.17, created by Roger.
//
VOID
N6SdioTxThreadCallback(
	IN	PVOID	pContext
	)
{
	
	PADAPTER	Adapter = ((PRT_THREAD)pContext)->Adapter;
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(Adapter);		
	PRT_SDIO_TX_QUEUE	pTxQueue = NULL;	
	PSDIO_OUT_CONTEXT	pOutContextToSend;
	PSDIO_OUT_CONTEXT	pOutContextToComplete = NULL;
	u1Byte		queueIdx = 0;
	BOOLEAN		bResult = TRUE;
	u1Byte		FwPSState;
	PRT_TCB	 	pTcb = NULL;
	u2Byte		busyCount = 0;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		RT_ASSERT(FALSE, ("N6SdioTxThreadCallback() in PASSIVE_LEVEL is not allowed!!\n"));
		return;
	}	
	
	while(TRUE)
	{
	
		//2<Roger_TODO> Packet Transmission processing on SDIO bus.
		
		//
		// We only need to take care of surprise removal or driver unload event to prevent Tx thread function disable by 
		// Function BIT disable, e.g., DF_TX_BIT
		// 
		if( RT_DRIVER_HALT(Adapter) )
			break;
		
		if(!Adapter->bInitComplete)
			continue;
		
		if( PlatformAcquireSemaphore(&sdiodevice->TxSemaphore) != RT_STATUS_SUCCESS )
			break;

		Adapter->TxStats.NumTxSemaphore++;
		//
		// SDIO Tx context processing status here.
		//
		do
		{
			busyCount = 0;
			
			if( RT_SDIO_CANNOT_TX(Adapter) )
				break;

			//2Check Context busy Queue.
			if( N6SdioTxQueuePending(Adapter) == FALSE )
				break;			

			//2Dequeue the context from specific queue and Transfer data.
			
			for ( queueIdx=0; queueIdx<SDIO_MAX_TX_QUEUE;)
			{
				
				if( RT_SDIO_CANNOT_TX(Adapter) )
					break;				

				RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioTxThreadCallback(): Check QueueIdx(%d)\n", queueIdx));
				
				//2<Roger_TODO> Check whether underlying HW is in power saving 32k mode.
				PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
				pTxQueue= &(sdiodevice->RtTxQueue[queueIdx]);
				if(!RTIsListEmpty(&(pTxQueue->ContextBusyList)))
				{	//tynli_test_32k 2011.02.25.
					PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
				
					Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_FW_PS_STATE, &FwPSState);										
					if(IS_IN_LOW_POWER_STATE(Adapter, FwPSState))
					{						
						RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioTxThreadCallback(): CANNOT TX ---> Wake up Hw.\n"));
						Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&Adapter));
					}
				}
				else
				{
					PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
				}
								
				PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

				// Retrieve Busy Context from specific Tx Queue
				pTxQueue= &(sdiodevice->RtTxQueue[queueIdx]);				
				if(!RTIsListEmpty(&(pTxQueue->ContextBusyList)))
				{
					pOutContextToSend = (PSDIO_OUT_CONTEXT)RTGetHeadList( &(pTxQueue->ContextBusyList));
					pTcb = pOutContextToSend->pTcb;
					pTcb->sysTime[2] = PlatformGetCurrentTime();
				}
				else
				{	
					PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
					RT_TRACE(COMP_POWER, DBG_TRACE, ("N6SdioTxThreadCallback(): QueueIdx(%d) is empty check next queue index\n", queueIdx));
					queueIdx++; // Next Tx FIFO Queue index				
					continue;
				}	
				
				//
				//Check whether the number of free pages in TxFIFO is available to perform SDIO data transfer.				
				//
				if( !Adapter->HalFunc.HalSdioQueryTxBufferAvailableHandler(
									Adapter, 
									pOutContextToSend))
				{	
					pOutContextToSend = NULL;
					PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
					RT_TRACE(COMP_SEND, DBG_WARNING, ("N6SdioTxThreadCallback(): TxQueueIdx(%d) Tx FIFO is full!!\n", queueIdx));
					//
					// We should use stall execution to replace NdisMSleep to prevent from the thread is swapping out for too long.
					// It will cause "iperf Tx" throughput very bad when Tx aggregation number is set to default value (6).
					// Because the probability of no availible free page is higher in a larger TxAggNum setting, then the thread will be
					// swapping out more often. 2015.04.30, by tynli. [SDIO-287]
					//
					PlatformStallExecution(1);
					
					busyCount++;
					if(busyCount > 100)		// Prevent Tx starvation
						queueIdx++;
					continue;
				}

				//
				// Check whether Fw state is in 32k. 2011.05.10. by tynli.
				//		
				Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_FW_PS_STATE, &FwPSState);			
				if(IS_IN_LOW_POWER_STATE(Adapter, FwPSState))
				{		
					RT_TRACE(COMP_POWER, DBG_TRACE, ("N6SdioTxThreadCallback(): Hw still in 32k. Return packet. <----\n"));
					pOutContextToSend = NULL;
					PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
					// Prefast warning C28121: The function 'NdisMSleep' is not permitted to be called at the current IRQ level.
					// False positive, irql should be restored by PlatformReleaseSpinLock.
#pragma warning( disable:28121 )
					PlatformSleepUs(1);
					continue;
				}
			
				//
				// SDIO Tx FIFO is available to perform data transfer.
				//				
				pTxQueue= &(sdiodevice->RtTxQueue[queueIdx]);				
				if(!RTIsListEmpty(&(pTxQueue->ContextBusyList)))
				{
					// Retrieve the first context to send
					pOutContextToSend = (PSDIO_OUT_CONTEXT)RTGetHeadList( &(pTxQueue->ContextBusyList));				
					PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

					bResult = PlatformSdioTxTransfer(Adapter, pOutContextToSend);

					PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
					pOutContextToComplete = (PSDIO_OUT_CONTEXT)RTGetHeadList( &(pTxQueue->ContextBusyList));				
					// Check the head item is the same or not, to avoid the TxQueue being cleaned by other thread outside the scope of TX_LOCK
					if((pOutContextToSend==pOutContextToComplete) && !RTIsListEmpty(&(pTxQueue->ContextBusyList)))
					{
						// Remove the first context we sent before from busy queue.
						pOutContextToComplete = PlatformSdioTxDequeue(Adapter, queueIdx);
						RT_ASSERT((pOutContextToSend == pOutContextToComplete), ("N6SdioTxThreadCallback(): SDIO Context mismatched!!"));
						PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);

						N6SdioTxComplete(pOutContextToComplete, bResult);
					}
					else	// Empty, nop and release lock.
					{
						RT_TRACE(COMP_SEND, DBG_LOUD, ("N6SdioTxThreadCallback(): pTxQueue->ContextBusyList be cleaned by other threads\n"));
						PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
					}
				}
				else
				{
					RT_ASSERT(FALSE, ("TxQueue(%d) ContextBusyList is empty!!\n", queueIdx));
					PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
				}				

				if( N6SdioTxQueuePending(Adapter) == FALSE )
				{
					RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioTxThreadCallback(): All Queues are empty, exit!!\n"));
					break;	
				}
				
				queueIdx++; // Next Tx FIFO Queue
			}						

			//Check Context busy Queue here, if all busy contexts are cleared and then break.
			if( N6SdioTxQueuePending(Adapter) == FALSE )
			{
				RT_TRACE(COMP_SEND, DBG_TRACE, ("N6SdioTxThreadCallback(): All busy contexts are cleared\n"));
				break;
			}

		}while(TRUE); // To make sure that we can handle each context busy queue sequentially, need to check the counting of this semaphore further

		//WDI_TxCreditCheck(Adapter);
	}

	// Release Tx Queue buffered context if needed.
	N6SdioReleaseTxQueuePending(Adapter);	
}


//
//	Description: 
//		The bus driver calls the SD card driver's callback routine whenever the card indicates an interrupt. 
// 	The callback routine must send the appropriate device commands to handle and clear the interrupt on the card. 
// 	After it completes the series of I/O operations, the SD card driver should acknowledge the interrupt.
//
//	Assumption:
//		PASSIVE_LEVEL
//
//	2010.12.14, created by Roger.
//
NTSTATUS
N6SdioHandleInterrupt(
	PVOID		Context, 
	u4Byte		InterruptType
)
{
	PADAPTER	Adapter = (PADAPTER)Context;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(Adapter);	
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	HAL_INT_MODE	sdioIntType = HAL_INT_MODE_LOCAL;
	PADAPTER		pBcnAdapter = Adapter;
	PMGNT_INFO		pBcnMgntInfo = pMgntInfo;
	BOOLEAN			bContinuallyHandleIntr = FALSE;
	u2Byte			PollingCnt = 0;

	Adapter->numInterrupt++;
	//
	// <Roger_TODO> Needs to perform SDIO Interrupt handling as follows.
	// 1. Read HISR : offset 0x18 , 4 byte 
	// 2. Get Packet Size : offset 0x1C, 2 byte
	// 3. Read from Buffer : addr: WLAN_FIFO_RX , RX_BUFFER_SZ
	// 4. Acknowledge this SDIO host generated interrupt if needed.
	//

	//
	// <Roger_Notes> When a card interrupt occurs during normal operation, the bus driver masks the function's interrupt 
	// using the interrupt-enable bit in the CCCR, and it then calls the function driver's callback routine. 
	// The function driver can then proceed to handle the interrupt. Once our driver has serviced the interrupt on our respective function, 
	// we must call the SdbusAcknowledgeInterrupt routine. When the bus driver receives this call, it re-enables the function's interrupt, 
	// and operation can proceed.
	// 2010.06.17.
	//

#if (RK_PLATFORM_SUPPORT == 1)
	PlatformAcquireMutex(&pDevice->RxHandleIntMutex);
#endif
	
	do{

		// We should return interrupt contorl immediately if driver is stopped.
		if(Adapter->bDriverStopped)
		{
			break;
		}
		
		if(RT_SDIO_CANNOT_RX(Adapter))
		{
			break;
		}

		while(pBcnAdapter !=NULL)
		{
			if(IN_SEND_BEACON_MODE(pBcnAdapter))
			{
				pBcnMgntInfo = &pBcnAdapter->MgntInfo;
				break;
			}
			pBcnAdapter = GetNextExtAdapter(pBcnAdapter);
		}

		if(pBcnAdapter == NULL)
		{
			pBcnAdapter = Adapter;
			pBcnMgntInfo = pMgntInfo;
		}
	
		sdioIntType = HAL_INT_MODE_LOCAL;
		// Prefast warning C28182: Dereferencing NULL pointer. 'Adapter'
		if(Adapter != NULL && Adapter->bInitComplete &&
			NicIFInterruptRecognized(Adapter, &sdioIntType, sizeof(HAL_INT_MODE)))
		{
			// Handle Rx Interrupt
			if(Adapter->HalFunc.GetInterruptHandler(Adapter, HAL_INT_TYPE_RX_OK))
			{
				if(!Adapter->HalFunc.RxHandleInterruptHandler(Adapter))
				{
					RT_TRACE(COMP_RECV, DBG_TRACE, ("N6SdioHandleInterrupt(): Fail to handle Rx Interrupt!!\n"));
				}
			}

			// Handle beacon early interrupt
			if((pBcnMgntInfo->mIbss) ||
				(ACTING_AS_AP(pBcnAdapter) && pBcnMgntInfo->mAssoc))
			{
				if(Adapter->HalFunc.GetInterruptHandler(Adapter, HAL_INT_TYPE_BcnInt))
				{			
					RT_TRACE(COMP_BEACON, DBG_TRACE, ("N6SdioHandleInterrupt(): Handle BCN early interrupt!!\n"));
					UpdateBeaconFrame(pBcnAdapter);
				}
			}

#if (P2P_SUPPORT == 1)
			if(Adapter->HalFunc.GetInterruptHandler(Adapter, HAL_INT_TYPE_TSF_BIT32_TOGGLE))
			{
				PADAPTER	pP2PPort = Adapter;
				PP2P_INFO	pP2PInfo = NULL;

				while(pP2PPort !=NULL)
				{
					pP2PInfo = GET_P2P_INFO(pP2PPort);
					if(P2P_ENABLED(pP2PInfo) && ((P2P_GO == pP2PInfo->Role) || (P2P_CLIENT == pP2PInfo->Role)))
					{
						RT_TRACE(COMP_P2P, DBG_LOUD, ("Current port %p with p2p enable = TRUE\n", pP2PPort));
						break;
					}
					pP2PPort = GetNextExtAdapter(pP2PPort);
				}
				if(pP2PPort == NULL)
					pP2PPort = Adapter;

				// Prefast warning C28182: Dereferencing NULL pointer. 'pP2PPort'
				if(pP2PPort != NULL)
					P2PPsTsf_Bit32_Toggle(GET_P2P_INFO(pP2PPort));
			}
#endif

			// Handle host system interrupt status registers
			if(Adapter->HalFunc.GetInterruptHandler(Adapter, HAL_INT_TYPE_HSISR_IND))
			{
				sdioIntType = HAL_INT_MODE_SYSTEM;
				RT_TRACE(COMP_RF, DBG_LOUD, ("N6SdioHandleInterrupt(): Handle host system interrupt!!\n"));
				if(NicIFInterruptRecognized(Adapter, &sdioIntType, sizeof(HAL_INT_MODE)))
				{					
					//Handle the status change of Power down(RF Off) or RF On
					if(Adapter->HalFunc.GetInterruptHandler(Adapter, HAL_INT_TYPE_GPIO9_INT) | 
						Adapter->HalFunc.GetInterruptHandler(Adapter, HAL_INT_TYPE_SYS_PDNINT) | 
						Adapter->HalFunc.GetInterruptHandler(Adapter, HAL_INT_TYPE_SDIO_RON_INT_EN) 
						)
					{
						BOOLEAN		bRfStatusChanged = TRUE;				
						Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_RF_STATUS_INT, (pu1Byte)(&bRfStatusChanged));					
					}
				}
			}
		}

		if(pMgntInfo->bSdioPollIntrHandler)
		{
			PollingCnt++;
			// Prefast warning C28182: Dereferencing NULL pointer. 'Adapter'
			if (Adapter != NULL)
			{
				bContinuallyHandleIntr = (Adapter->HalFunc.GetInterruptHandler(Adapter, HAL_INT_TYPE_RX_OK) && \
					(PollingCnt < pMgntInfo->SdioIntrPollingLimit)) ? TRUE : FALSE;
			}
		}
		else
		{
			bContinuallyHandleIntr = FALSE;
		}
	}while(bContinuallyHandleIntr);

#if (RK_PLATFORM_SUPPORT == 1)
	PlatformReleaseMutex(&pDevice->RxHandleIntMutex);
#endif

	// To acknowledge the bus driver that we have already finished processing the interrupt.
       if(pDevice->Sdbusinterface.AcknowledgeInterrupt)
		status = (pDevice->Sdbusinterface.AcknowledgeInterrupt)(pDevice->Sdbusinterface.Context);


	return status;

}


//
//	Description:
//		Enable all SDIO Tx Queues.
//
//	Assumption:
//		PASSIVE_LEVEL
//
VOID
N6WdmSdioTx_Enable(
	IN  PADAPTER 	Adapter
	)
{
	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(Adapter);
	int i;

	for (i = 0; i < pDevice->RtNumTxQueue; i++)
	{
		N6SdioStartTxQueue(Adapter, i);
	}
}


//
//	Description:
//		Enable and reset SDIO CMD related control event.
//
//	Assumption:
//		PASSIVE_LEVEL
//
VOID
N6WdmSdio_Enable(
	IN  PADAPTER 	Adapter
	)
{
	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(Adapter);

	NdisResetEvent(&pDevice->AllSdioCmdReturnedEvent);
	NdisAcquireSpinLock( &(pDevice->IrpSpinLock) );
	RT_SDIO_INC_CMD_REF(pDevice);	
	NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );
}


//
//	Description:
//		Cancel and wait all outstanding transfers completd.
//
//	Assumption:
//		PASSIVE_LEVEL
//
VOID
N6WdmSdio_Disable(
	IN  PADAPTER 	Adapter
	)
{
	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(Adapter);
	int i;
	
	// SDIO interface do NOT need to cancel any Pending IN IRPs.	
	
	// Cancel Pending out IRPs if needed.
	for (i = 0; i < pDevice->RtNumTxQueue; i++)
	{
		N6SdioStopTxQueue(Adapter, i);
	}

#if !RTL8723_SDIO_IO_THREAD_ENABLE 
	// Cancel Asyn IO Pending IRP.
	RTsdioCancelAsynIoPendingIrp( Adapter );	
#endif

	NdisAcquireSpinLock( &(pDevice->IrpSpinLock) );
	RT_SDIO_DEC_CMD_REF(pDevice);
	if (RT_SDIO_GET_CMD_REF(pDevice) > 0)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6WdmSdio_Disable(): wait for all SDIO Cmd returned (%d)\n", RT_SDIO_GET_CMD_REF(pDevice)));
		NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );

		while (TRUE)
		{
			if ( NdisWaitEvent(&pDevice->AllSdioCmdReturnedEvent, 2000) )
			{
				break;
			}
		}

		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6WdmSdio_Disable(): end of waiting returned SDIO Cmd\n"));
	}
	else
	{
		NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6WdmSdio_Disable(): All SDIO CMDs are returned, Refcnt(%#x)\n", RT_SDIO_GET_CMD_REF(pDevice)));
	}

	
}

//
//	Description:
//		Initialize SDIO bus and device on NDIS6-WDM framework.
//
//	Assumption:
//	
//	2010.12.09, added by Roger. 
//
NTSTATUS
N6WdmSdio_Initialize(
	IN  PADAPTER		Adapter
)
{	
	NTSTATUS		ntStatus = STATUS_SUCCESS;
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(Adapter);
	u1Byte	U1bData = 0;	
	

	//
   	// Open an interface to the SD bus driver
    	//
	ntStatus = SdBusOpenInterface (
				sdiodevice->pPhysDevObj,
				&sdiodevice->Sdbusinterface,
				sizeof(SDBUS_INTERFACE_STANDARD),
				SDBUS_INTERFACE_VERSION);			
	
    	if(NT_SUCCESS(ntStatus)) 
	{	
		//
		// Contains the information necessary to initialize a Secure Digital (SD) card bus interface.
		//
		SDBUS_INTERFACE_PARAMETERS interfaceParameters = {0};
		interfaceParameters.Size = sizeof(SDBUS_INTERFACE_PARAMETERS);
		interfaceParameters.TargetObject =sdiodevice->pSdioDevObj; 
		interfaceParameters.DeviceGeneratesInterrupts = TRUE; // The SD device generates interrupts
		interfaceParameters.CallbackAtDpcLevel = FALSE; //  Run at PASSIVE_LEVEL
		interfaceParameters.CallbackRoutine = N6SdioHandleInterrupt; // The bus driver calls when a device interrupt occurs

		interfaceParameters.CallbackRoutineContext = (PVOID )Adapter;
		if (sdiodevice->Sdbusinterface.InitializeInterface) 
		{
			// Sets initialization parameters on the interface.
			ntStatus = (sdiodevice->Sdbusinterface.InitializeInterface)
				(sdiodevice->Sdbusinterface.Context, &interfaceParameters);

			if (!NT_SUCCESS(ntStatus)) 
			{
				RT_TRACE(COMP_INIT, DBG_WARNING, ("N6WdmSdio_Initialize(): Init SD Bus fail!!\n"));
			}
		}
	}

	N6SdioDummyIO(sdiodevice);

	//
	// Configure corresponding SDIO properties.
	//
	if ( N6SdioConfigureDevice(sdiodevice) != STATUS_SUCCESS )		
		return STATUS_INSUFFICIENT_RESOURCES;

	sdiodevice->SdioTxBlockMode = TRUE;
	sdiodevice->SdioRxBlockMode = TRUE;

#if (RK_PLATFORM_SUPPORT == 1)
	sdiodevice->IoRegDirectAccess = TRUE; // Use CMD52 Direct IO R/W for 1, 2 Bytes access in default.	
#else
	sdiodevice->IoRegDirectAccess = FALSE;	
#endif
        
        // Reset SDIO Rx sequence number on initialization process
	RT_SDIO_RESET_RX_SEQ_NUM(Adapter);

	return ntStatus;
}

VOID
PlatformSdioEnableRxTransfer(
	IN	PADAPTER	Adapter
)
{
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);

	NdisResetEvent(&sdiodevice->AllSdioRxTransCompleteEvent);

	PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	sdiodevice->bStopRxTransfer = FALSE;
	RT_SDIO_INC_RX_TRANS_REF(sdiodevice);
	PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);	
}

VOID
PlatformSdioDisableRxTransfer(
	IN	PADAPTER	Adapter
)
{
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);

	PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	RT_SDIO_DEC_RX_TRANS_REF(sdiodevice);
	if (RT_SDIO_GET_RX_TRANS_REF(sdiodevice) > 0)
	{
		RT_TRACE(COMP_RECV, DBG_LOUD, ("PlatformSdioDisableRxTransfer(): wait for all SDIO Rx Transfer progress complete (%d)\n", RT_SDIO_GET_RX_TRANS_REF(sdiodevice)));
		PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

		while (TRUE)
		{
			// Prefast warning C28121: The function 'NdisWaitEvent' is not permitted to be called at the current IRQ level.
			// Prefast warning C28156: The actual IRQL 2 is inconsistent with the required IRQL 0
			// False positive, irql should be restored by PlatformReleaseSpinLock.
#pragma warning( disable:28121 )
#pragma warning( disable:28156 )
			if(NdisWaitEvent(&sdiodevice->AllSdioRxTransCompleteEvent, 2000))
			{
				break;
			}
		}

		RT_TRACE(COMP_RECV, DBG_LOUD, ("PlatformSdioDisableRxTransfer(): end of waiting SDIO Rx Transfer complete\n"));
	}
	else
	{
		PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
		RT_TRACE(COMP_RECV, DBG_LOUD, ("PlatformSdioDisableRxTransfer(): All SDIO Rx transfer are completed, Refcnt(%#x)\n", RT_SDIO_GET_RX_TRANS_REF(sdiodevice)));
	}
	
	PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	sdiodevice->bStopRxTransfer = TRUE;
	PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
	
}

