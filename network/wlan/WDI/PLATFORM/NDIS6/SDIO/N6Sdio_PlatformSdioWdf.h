#ifndef __INC_N6PLATFORMSDIOWDF_H
#define __INC_N6PLATFORMSDIOWDF_H

/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	N6Sdio_PlatformSdioWdf.h
	
Abstract:
	Prototype of WdfSdio_XXX() implemented via WDF SDIO.
	These function shall be only use under Platform\.	
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2012-01-16 Roger            Create.
	
--*/

#if USE_WDF_SDIO

VOID
WdfSdio_CancelAsyncSDReq(
	IN	PADAPTER		pAdapter
	);


NTSTATUS
WdfSdio_Initialize(
	IN  PADAPTER		Adapter
	);

VOID
WdfSdio_Halt(
	IN  PADAPTER		Adapter
	);

VOID
WdfSdio_Disable(
	IN  PADAPTER		pAdapter
	);

VOID
WdfSdioTx_Enable(
	IN  PADAPTER		pAdapter
	);

NTSTATUS
WdfSdio_InitAsyncSDRequest(
	IN	PADAPTER		pAdapter
	);


VOID
WdfSdio_DeInitAsyncSDRequest(
	IN	PADAPTER		pAdapter
	);

BOOLEAN
WdfSdio_AsyncVendorRequestWrite(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					bReq,				// 1 byte bReq field of setup token.
	IN	u2Byte					wValue, 			// 2 byte wValue field of setup token.
	IN	u2Byte					wIndex,				// 2 byte wIndex field of setup token. 
	IN	pu1Byte					pBuffer,			// Pointer to buffer to transfer in data-phase.
	IN	u4Byte					BufferLength		// # of bytes to OUT or maximal # of bytes to IN.
	);

VOID
WdfSdio_EnableAllTxQueues(
	IN	PADAPTER				pAdapter
	);

VOID
WdfSdio_DisableAllTxQueues(
	IN	PADAPTER				pAdapter
	);

#endif // end of USE_WDF_SDIO

#endif
