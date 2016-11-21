#ifndef __INC_PLATFORMSDIOWDM_H
#define __INC_PLATFORMSDIOWDM_H

/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	N6Sdio_PlatformSdioWdm.h
	
Abstract:
	Prototype of WdmSdio_XXX() implemented via WDM SDIO.
	These function shall be only use under Platform\.	

Added by Roger, 2012.01.01.   

	
--*/

VOID
N6WdmSdioTx_Enable(
	IN  PADAPTER 	Adapter
	);

NTSTATUS
N6WdmSdio_Initialize(
	IN  PADAPTER			Adapter
	);

VOID
N6WdmSdio_Disable(
	IN  PADAPTER 	Adapter
	);

VOID
N6WdmSdio_Enable(
	IN  PADAPTER 	Adapter
	);

BOOLEAN
N6SdioTxQueuePending(
	IN  PADAPTER 	Adapter
	);
	
BOOLEAN
N6SdioReleaseTxQueuePending(
	IN  PADAPTER 	Adapter
	);
	
#if RTL8723_SDIO_IO_THREAD_ENABLE 
VOID
N6SdioIOThreadCallback(
	IN	PVOID	pContext
	);
#endif
	
VOID
N6SdioTxThreadCallback(
	IN	PVOID	pContext
	);

#endif // __INC_PLATFORMSDIOWDM_H

