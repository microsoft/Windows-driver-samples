#ifndef __INC_PLATFORMSDIO_H
#define __INC_PLATFORMSDIO_H

/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	PlatformSdio.h
	
Abstract:
	Prototype of PlatformSdioXXX(). 
	Each platform shall implement the functions exported in 
	this file.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------	
	2010-12-20 Roger			Create.
	
--*/

//================================================================================
//	Prototype of functions to export.
//================================================================================

//
//	SDIO Initialization and DeInitialization interfaces.
//

//
//	Bulk IN.
//
BOOLEAN 
PlatformSdioInitInContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_IN_CONTEXT			pContext
	);

VOID 
PlatformSdioDeInitInContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_IN_CONTEXT			pContext
	);


//
//	SDIO Tx Transfer
//
BOOLEAN 
PlatformSdioInitTxContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext
	);

VOID 
PlatformSdioDeInitTxContext(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext
	);

BOOLEAN
PlatformSdioTxEnqueue(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext
	);

PSDIO_OUT_CONTEXT
PlatformSdioTxDequeue(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					QueueIdx
	);

BOOLEAN
PlatformSdioTxTransfer(
	IN	PADAPTER				pAdapter,
	IN	PSDIO_OUT_CONTEXT		pContext
	);

BOOLEAN
PlatformSdioRxTransfer(
	IN	PADAPTER				pAdapter,
	IN	PRT_RFD					pRfd,
	IN	u4Byte					Rx0ReqLength
	);

VOID
PlatformSdioEnableRxTransfer(
	IN	PADAPTER	Adapter
	);

VOID
PlatformSdioDisableRxTransfer(
	IN	PADAPTER	Adapter
	);

VOID
PlatformSdioEnableTxQueues(
	IN	PADAPTER				Adapter
	);

VOID
PlatformSdioDisableTxQueues(
	IN	PADAPTER				Adapter
	);

BOOLEAN
PlatformSdioTxQueueIdxEmpty(
	IN  PADAPTER 	Adapter,
	IN  u1Byte		QueueID
	);

BOOLEAN
PlatformSdioTxAndAwbQueueEmpty(
	IN  PADAPTER 	Adapter
	);

VOID
PlatformSdioWaitAllSDReqComplete(
	IN  PADAPTER 	Adapter
	);

VOID
PlatformSdioCancelSdReq(
	PADAPTER	Adapter
);

#endif //#ifndef __INC_PLATFORMUSB_H
