#ifndef __INC_HAL8723BSDIOLED_H
#define __INC_HAL8723BSDIOLED_H
/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	Hal8192CUsbLed.h
	
Abstract:
	LED related data structure and interface to manipulate LED.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2006-12-27 Rcnjko            Create.
	
--*/

//================================================================================
// Interface to manipulate LED objects.
//================================================================================

VOID
SwLedOn_8723B(
	IN	PADAPTER			Adapter, 
	IN	PLED_SDIO		pLed
);

VOID
SwLedOff_8723B(
	IN	PADAPTER			Adapter, 
	IN	PLED_SDIO		pLed
);

VOID
SwLedInit_8723BS(
	IN	PADAPTER		Adapter
);

#endif

