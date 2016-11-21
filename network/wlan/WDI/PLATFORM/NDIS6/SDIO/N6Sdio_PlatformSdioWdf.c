/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	N6Sdio_PlatformSdioWdf.c
	
Abstract:
	Implement PlatforSdioXXX() and WdfSdio_XXX() on WDF SDIO.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2012-01-10 Roger             Create.
                          
	
--*/

#include "Mp_Precomp.h"


//================================================================================
//	Constant.
//================================================================================

#if USE_WDF_SDIO

//================================================================================
//	Prototype of protected function.
//================================================================================

//================================================================================
//	SDIO Init / DeInit.	
//================================================================================

//
//	Description:
//		Select 1st interface and initialize all pipes on it.
//		Note, we don't start IoTargets of all pipes here 
//		because HW is not properly configured.
//
//	Assumption:
//		Only invoked in MiniportInitialize() context.
//
NTSTATUS
WdfSdio_Initialize(
	IN  PADAPTER		Adapter
)
{
	NTSTATUS					ntStatus = STATUS_SUCCESS;
	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(Adapter);
	
	//
	// <Roger_TODO> We should implement SDIO initialization process on WDF framework if needed.
	// 1. Async SD Request initiallization and corresponding event
	// 2. All Sync SD Request initialization for IO or TxRx engine
	// 2012.01.19.
	//
	
	//
	// Initialize event for control request.
	//
	NdisInitializeEvent(&(pDevice->AllCtrlTransCompleteEvent));

	//
	// Allocate WDFREQUEST for async vendor request (I/O).
	//
	ntStatus = WdfSdio_InitAsyncSDRequest(Adapter);
	if (!NT_SUCCESS(ntStatus))
	{
		RT_TRACE(COMP_INIT, DBG_LOUD , ("Could not create request: status(0x%08X)", ntStatus));
		return FALSE;
	}
	return ntStatus;
}


//
//	Descriptor:
//		Stop all USB activity and release allocated in WdfSdio_Initialize().
//
VOID
WdfSdio_Halt(
	IN  PADAPTER		pAdapter
	)
{

	//
	// <Roger_TODO>
	//
	
	WdfSdio_Disable(pAdapter);
	WdfSdio_DeInitAsyncSDRequest(pAdapter);

}


//
//	Description:
//		Start all Tx queues	
//
VOID
WdfSdioTx_Enable(
	IN  PADAPTER		pAdapter
	)
{
	//
	// <Roger_TODO>
	// Refer to the NDIS5 WdmSdioTx_Enable routine
	//
}



//
//	Description:
//		Tell framework to stop all pipes and wait until 
//		all pending requests completed.
//
VOID
WdfSdio_Disable(
	IN  PADAPTER		pAdapter
	)
{

	//
	// <Roger_TODO>
	//
	
	//
	// Cancel and wait all async SD request completed. 
	//
	WdfSdio_CancelAsyncSDReq(pAdapter);

}


//
//	Description:
//		Allocate and set up resource for async vendor reqeust.
//
NTSTATUS
WdfSdio_InitAsyncSDRequest(
	IN	PADAPTER		pAdapter
	)
{
	NTSTATUS					ntStatus = STATUS_SUCCESS;

	//
	// <Roger_TODO>
	//

	return ntStatus;
}


//
//	Description:
//		Release resource allocated.
//
VOID
WdfSdio_DeInitAsyncSDRequest(
	IN	PADAPTER		pAdapter
	)
{

	//
	// <Roger_TODO>
	//
}


//
//	Description:
//		Cancel and wait all async vendor request completed. 
//
VOID
WdfSdio_CancelAsyncSDReq(
	IN	PADAPTER		pAdapter
	)
{
	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(pAdapter);
	PWDF_USB_DEVICE_CONTEXT pWdfUsbDevCtx = GET_WDF_USB_DEVICE_CONTEXT(pDevice);
	WDFREQUEST AsyncVendorRequest;

	NdisAcquireSpinLock( &(pDevice->IrpSpinLock) );
	if(pDevice->nIrpPendingCnt > 0)
	{
		if(pDevice->bAsynIoWritePending)
		{ 
			RT_TRACE(COMP_IO, DBG_LOUD, ("WdfSdio_CancelAsyncSDReq(): AsynIoWritePending => cancel it....\n"));
			AsyncVendorRequest = pWdfUsbDevCtx->AsyncVendorRequest;
			NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );
	
			//
			// Cancel the WDFREQUEST for AsynIO write request.
			//
			WdfRequestCancelSentRequest(AsyncVendorRequest);
		}
		else
		{
			NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );
			RT_TRACE(COMP_IO, DBG_LOUD, ("WdfSdio_CancelAsyncSDReq(): sync request is pending....\n"));
		}

		//
		// Wait until it completed by USB host driver.
		//
		while( !NdisWaitEvent(&(pDevice->AllCtrlTransCompleteEvent), 50) )
		{
			RT_TRACE(COMP_IO, DBG_LOUD, ("waiting AllIoIrpReturnedEvent...\n"));
		}
		NdisResetEvent( &(pDevice->AllCtrlTransCompleteEvent) );
	}
	else
	{
		NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );
		RT_TRACE(COMP_IO, DBG_LOUD, ("WdfSdio_CancelAsyncSDReq(): no pending request....\n"));
	}
}


//
//	Description:
//		Callback function of WdfSdio_AsyncVendorRequestWrite().
//
VOID
WdfSdio_AsyncVendorRequestWriteComplete(
	IN WDFREQUEST		Request,
	IN WDFIOTARGET		Target,
	IN PWDF_REQUEST_COMPLETION_PARAMS	CompletionParams,
	IN WDFCONTEXT		Context
	)
{
	//
	// <Roger_TODO>
	//
}


//
//	Description:
//		Issue an vendor request from host to device in asynchronous manner.
//
//	Assumption:
//		We at most allow one async vendor request pending.
//		So, this function must not be re-entry.
//
BOOLEAN
WdfSdio_AsyncVendorRequestWrite(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					bReq,				// 1 byte bReq field of setup token.
	IN	u2Byte					wValue, 			// 2 byte wValue field of setup token.
	IN	u2Byte					wIndex,				// 2 byte wIndex field of setup token. 
	IN	pu1Byte					pBuffer,			// Pointer to buffer to transfer in data-phase.
	IN	u4Byte					BufferLength		// # of bytes to OUT or maximal # of bytes to IN.
	)
{
	//
	// <Roger_TODO>
	//
	
	return TRUE;
}


//
//	Description:
//		Initialize PlatformReserved[] of this SDIO_OUT_CONTEXT object.
//		It return TRUE on success, FALSE otherwise.
//
BOOLEAN 
PlatformSdioInitTxContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext
	)
{
	BOOLEAN bResult = TRUE;

	//
	// <Roger_TODO>
	//
	
	return bResult;
}



//
//	Description:
//		Free URB and IRP allocated for this SDIO_OUT_CONTEXT object.
//
VOID 
PlatformSdioDeInitTxContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext
	)
{
	//
	// <Roger_TODO>
	//
	
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
	IN	PSDIO_OUT_CONTEXT		pContext
	)
{

	BOOLEAN						bResult = TRUE;
 
	//
	// <Roger_TODO>
	//

	return bResult;
}


//
//	Description:
//		Tell framework we are ready to issue Tx transfer on 
//		all Tx queues we selected.
//
//	Assumption:
//		RT_TX_SPINLOCK is acquired.
//
VOID
WdfSdio_EnableAllTxQueues(
	IN	PADAPTER				pAdapter
	)
{

	//
	// <Roger_TODO>
	//

}

//
//	Description:
//		Tell framework to stop ongoing OUT transfers on 
//		all IN pipes we selected.
//
//	Assumption:
//		RT_TX_SPINLOCK is acquired.
//
VOID
WdfSdio_DisableAllTxQueues(
	IN	PADAPTER				pAdapter
	)
{
	PRT_USB_DEVICE pDevice = GET_RT_USB_DEVICE(pAdapter);
	PWDF_USB_DEVICE_CONTEXT pWdfUsbDevContext = GET_WDF_USB_DEVICE_CONTEXT(pDevice);
	int idx;
	PRT_USB_OUT_PIPE pRtOutPipe;
	WDFUSBPIPE hPipe;
	WDF_IO_TARGET_STATE  ioTargetState;

	for(idx = 0; idx < pWdfUsbDevContext->RtNumOutPipes; idx++)
	{
		pRtOutPipe = &(pWdfUsbDevContext->RtOutPipes[idx]);
		hPipe = pRtOutPipe->hPipe;

		//
		// Change IO Target to Stopped state to make it reject further request.
		//
		ioTargetState = WdfIoTargetGetState(WdfUsbTargetPipeGetIoTarget(hPipe));
		if(ioTargetState == WdfIoTargetStarted) 
		{
			pRtOutPipe->IrpPendingCount--;

			//
			// Stop pipe and cancel pending requets.
			//
			PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
			RT_TRACE(COMP_SEND, DBG_LOUD, ("WdfSdio_DisableAllTxQueues(): stopping %d pipe ..........\n", idx));
			WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(hPipe), WdfIoTargetCancelSentIo);		
			PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);

			//
			// Wait outstanding requests completed.
			//	
			RT_TRACE(COMP_SEND, DBG_LOUD, ("WdfSdio_DisableAllTxQueues(): nInPipeIdx(%d) IrpPendingCounts=%d\n", idx, pRtOutPipe->IrpPendingCount));
			if(	!RTIsListEmpty( &(pRtOutPipe->ContextBusyList) ) )
			{	
				PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
				while( !NdisWaitEvent(&(pRtOutPipe->AllIrpReturnedEvent), 50) )
				{
					RT_TRACE(COMP_SEND, DBG_LOUD, ("WdfSdio_DisableAllTxQueues(): waiting AllIrpReturnedEvent...\n"));
				}
				NdisResetEvent( &(pRtOutPipe->AllIrpReturnedEvent) );
				PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
			}
			RT_TRACE(COMP_SEND, DBG_LOUD, ("OUT Pipe(%d) is stopped.-------\n", idx));
		}
		else
		{
			RT_TRACE(COMP_SEND, DBG_LOUD, 
				("Try to stop OUT piep(%d) already in stopped state(%#X)\n", 
				idx, ioTargetState));
		}
	}
}


VOID
PlatformSdioEnableTxQueues(
	IN PADAPTER Adapter
	)
{
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	WdfSdio_EnableAllTxQueues(Adapter);
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}

VOID
PlatformSdioDisableTxQueues(
	IN PADAPTER Adapter
	)
{
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	WdfSdio_DisableAllTxQueues(Adapter);
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}

VOID
PlatformSdioCancelSdReq(
	PADAPTER	Adapter
)
{
	WdfSdio_CancelAsyncSDReq(Adapter);
}

#endif

