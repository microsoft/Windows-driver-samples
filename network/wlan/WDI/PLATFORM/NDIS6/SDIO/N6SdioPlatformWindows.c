#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "N6SdioPlatformWindows.tmh"
#endif

// Forward declaration.
typedef	struct _ADAPTER	ADAPTER, *PADAPTER;
typedef	struct _SDIO_OUT_CONTEXT	SDIO_OUT_CONTEXT, *PSDIO_OUT_CONTEXT;


//
//	Description: 
//		This routine is exported for SDIO WLAN IOREG I/O Bus Domain(Host),
//		Host Address Mapping Range is 0x1026_0000~0x1026_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
VOID
PlatformIOWrite1Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u1Byte		data
	)
{

 	PADAPTER		pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	BOOLEAN			bMacPwrCtrlOn, bCmd52Available = TRUE;
	RT_STATUS		rtstatus = RT_STATUS_SUCCESS;
	u1Byte			FwPSState;

	LARGE_INTEGER	StartTime, EndTime;
	
	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return;

	// For PnP IO Working Time Calculation
	if( pDefaultAdapter->bCtrlPnPTime )
		StartTime = KeQueryPerformanceCounter(NULL);

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if(DeviceID == WLAN_IOREG_DEVICE_ID)
		bCmd52Available = pDefaultAdapter->HalFunc.HalSdioIoRegCmd52AvailableHandler(pDefaultAdapter, offset);

	if ((KeGetCurrentIrql() > PASSIVE_LEVEL) ||
		(GlobalSdioDbg & SDIO_DBG_ASYN_IO))
	{	
#if RTL8723_SDIO_IO_THREAD_ENABLE
		SdioAsynIOWriteEnqueue(sdiodevice, DeviceID, sdiodevice->SdioFuncNum, 1, (u2Byte)offset, &data);
#else
		SdioAsynIOWrite(sdiodevice, DeviceID, sdiodevice->SdioFuncNum, 1, (u2Byte)offset, &data);
#endif
	}
	else
	{
		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		sdiodevice->SyncIoInProgressCount++;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );	
		
	
		//------------------------------------------------------------
		// tynli add for 32k. 2011.02.25.
		if(pDefaultAdapter->bFWReady)
		{
			pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);		
			if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState) &&
				!IS_SDIO_POWER_ON_IO_REG(DeviceID, offset))
			{
				RT_TRACE(COMP_POWER, DBG_LOUD, ("CANNOT IO---> Wake up Hw. PlatformIOWrite1Byte(): DeviceID=%d, offset=%#X, value: %#X\n", DeviceID, offset, data));
				pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
			}
		}
		//------------------------------------------------------------

		if( bCmd52Available && (sdiodevice->IoRegDirectAccess || 
			((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) ||
			!bMacPwrCtrlOn) )
		{	
			rtstatus = PlatformSdioCmd52ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum,
								1, 
								(ULONG)offset, 
								TRUE,
								&data);	
		}
		else
		{
			// Use I/O RW extended command as default.
			rtstatus = PlatformSdioCmd53ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								1, 
								(ULONG)offset, 
								TRUE,
								&data);		
		}

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	}

	if( pDefaultAdapter->bCtrlPnPTime )
	{
		EndTime = KeQueryPerformanceCounter(NULL);
		pDefaultAdapter->PnPIOTime.QuadPart += (EndTime.QuadPart - StartTime.QuadPart);
	}
}


//
//	Description: 
//		This routine is exported for SDIO WLAN IOREG I/O Bus Domain(Host),
//		Host Address Mapping Range is 0x1026_0000~0x1026_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
VOID
PlatformIOWrite2Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u2Byte		data
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE		sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	BOOLEAN			bMacPwrCtrlOn, bCmd52Available = TRUE;
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;
	u1Byte	FwPSState;

	LARGE_INTEGER	StartTime, EndTime;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return;

	// For PnP IO Working Time Calculation
	if( pDefaultAdapter->bCtrlPnPTime )
		StartTime = KeQueryPerformanceCounter(NULL);

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if(DeviceID == WLAN_IOREG_DEVICE_ID)
		bCmd52Available = pDefaultAdapter->HalFunc.HalSdioIoRegCmd52AvailableHandler(pDefaultAdapter, offset);

	if ((KeGetCurrentIrql() > PASSIVE_LEVEL) ||
		(GlobalSdioDbg & SDIO_DBG_ASYN_IO))
	{
#if RTL8723_SDIO_IO_THREAD_ENABLE
		SdioAsynIOWriteEnqueue(sdiodevice, DeviceID, sdiodevice->SdioFuncNum, 2, (u2Byte)offset, &data);
#else		
		SdioAsynIOWrite(sdiodevice, DeviceID, sdiodevice->SdioFuncNum, 2, (u2Byte)offset, &data);
#endif
	}
	else
	{
		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		sdiodevice->SyncIoInProgressCount++;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		
		//------------------------------------------------------------
		// tynli add for 32k. 2011.02.25.
		if(pDefaultAdapter->bFWReady)
		{
			pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);		
			if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState) &&
				!IS_SDIO_POWER_ON_IO_REG(DeviceID, offset))
			{
				RT_TRACE(COMP_POWER, DBG_TRACE, ("CANNOT IO---> Wake up Hw. PlatformIOWrite2Byte(): DeviceID=%d, offset=%#X, value: %#X\n", DeviceID, offset, data));
				pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
			}
		}
		//------------------------------------------------------------

		if( bCmd52Available && (sdiodevice->IoRegDirectAccess || 
			((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
			!bMacPwrCtrlOn) )
		{
			rtstatus = PlatformSdioCmd52ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum,
								2, 
								(ULONG)offset, 
								TRUE,
								&data);	
		}
		else
		{
			// Use I/O RW extended command as default.
			rtstatus = PlatformSdioCmd53ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum,
								2, 
								(ULONG)offset, 
								TRUE,
								&data);	
		}

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	}
	
	if( pDefaultAdapter->bCtrlPnPTime )
	{
		EndTime = KeQueryPerformanceCounter(NULL);
		pDefaultAdapter->PnPIOTime.QuadPart += (EndTime.QuadPart - StartTime.QuadPart);
	}
}

//
//	Description: 
//		This routine is exported for SDIO WLAN IOREG I/O Bus Domain(Host),
//		Host Address Mapping Range is 0x1026_0000~0x1026_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
VOID
PlatformIOWrite4Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u4Byte		data
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE		sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	BOOLEAN			bMacPwrCtrlOn, bCmd52Available = TRUE;
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;
	u1Byte			FwPSState;

	LARGE_INTEGER	StartTime, EndTime;
	
	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return;

	// For PnP IO Working Time Calculation
	if( pDefaultAdapter->bCtrlPnPTime )
		StartTime = KeQueryPerformanceCounter(NULL);

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if(DeviceID == WLAN_IOREG_DEVICE_ID)
		bCmd52Available = pDefaultAdapter->HalFunc.HalSdioIoRegCmd52AvailableHandler(pDefaultAdapter, offset);

	if ((KeGetCurrentIrql() > PASSIVE_LEVEL) ||
		(GlobalSdioDbg & SDIO_DBG_ASYN_IO))
	{
#if RTL8723_SDIO_IO_THREAD_ENABLE
		SdioAsynIOWriteEnqueue(sdiodevice, DeviceID, sdiodevice->SdioFuncNum, 4, (u2Byte)offset, &data);
#else
		SdioAsynIOWrite(sdiodevice, DeviceID, sdiodevice->SdioFuncNum, 4, (u2Byte)offset, &data);
#endif
	}
	else
	{	
		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		sdiodevice->SyncIoInProgressCount++;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		
		//------------------------------------------------------------
		// tynli add for 32k. 2011.02.25.
		if(pDefaultAdapter->bFWReady)
		{
			pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);		
			if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState) &&
				!IS_SDIO_POWER_ON_IO_REG(DeviceID, offset))
			{
				RT_TRACE(COMP_POWER, DBG_TRACE, ("CANNOT IO---> Wake up Hw. PlatformIOWrite4Byte(): DeviceID=%d, offset=%#X, value: %#X\n", DeviceID, offset, data));
				pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
			}
		}
		//------------------------------------------------------------
#if (RK_PLATFORM_SUPPORT ==1)
		if( bCmd52Available && ((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100) || 
			!bMacPwrCtrlOn) )
#else
		if( bCmd52Available && (sdiodevice->IoRegDirectAccess || 
			((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
			!bMacPwrCtrlOn) )
#endif
		{
			rtstatus = PlatformSdioCmd52ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								4, 
								(ULONG)offset, 
								TRUE,
								&data);	
		}
		else
		{
			// Use I/O RW extended command as default.
			rtstatus = PlatformSdioCmd53ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum,
								4, 
								(ULONG)offset, 
								TRUE,
								&data);	
		}

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	}

	if( pDefaultAdapter->bCtrlPnPTime )
	{
		EndTime = KeQueryPerformanceCounter(NULL);
		pDefaultAdapter->PnPIOTime.QuadPart += (EndTime.QuadPart - StartTime.QuadPart);
	}
}

//
//	Description: 
//		This routine is exported for SDIO WLAN IOREG I/O Bus Domain(Host),
//		Host Address Mapping Range is 0x1026_0000~0x1026_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
VOID
PlatformIOWriteNByte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u4Byte		count,	//data length
	pu1Byte		pdata
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE		sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	BOOLEAN			bMacPwrCtrlOn = FALSE, bCmd52Available = TRUE;
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;
	u1Byte			FwPSState;

	LARGE_INTEGER	StartTime, EndTime;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return;

	// For PnP IO Working Time Calculation
	if( pDefaultAdapter->bCtrlPnPTime )
		StartTime = KeQueryPerformanceCounter(NULL);

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if(DeviceID == WLAN_IOREG_DEVICE_ID)
		bCmd52Available = pDefaultAdapter->HalFunc.HalSdioIoRegCmd52AvailableHandler(pDefaultAdapter, offset);

	if ((KeGetCurrentIrql() > PASSIVE_LEVEL) ||
		(GlobalSdioDbg & SDIO_DBG_ASYN_IO))
	{	
		RT_PRINT_DATA(COMP_INIT|COMP_IO, DBG_LOUD, "PlatformIOWriteNByte(): Sync IO Write N byte:\n", 
				pdata, count);
		SdioAsynIOWrite(sdiodevice, DeviceID, sdiodevice->SdioFuncNum, (u2Byte)count, (u2Byte)offset, pdata);// Issue CMD53 Asyn CMD as default.
	}
	else
	{
		RT_PRINT_DATA(COMP_INIT|COMP_IO, DBG_LOUD, "PlatformIOWriteNByte(): ASync IO Write N byte:\n", 
				pdata, count);	

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		sdiodevice->SyncIoInProgressCount++;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		
		//------------------------------------------------------------
		// tynli add for 32k. 2011.02.25.
		if(pDefaultAdapter->bFWReady)
		{
			pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);		
			if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState) &&
				!IS_SDIO_POWER_ON_IO_REG(DeviceID, offset))
			{
				RT_TRACE(COMP_POWER, DBG_LOUD, ("CANNOT IO---> Wake up Hw. PlatformIOWriteNByte(): DeviceID=%d, offset=%#X, value: %p\n", DeviceID, offset, pdata));
				pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
			}
		}
		//------------------------------------------------------------

		if( bCmd52Available && (sdiodevice->IoRegDirectAccess || 
			((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
			!bMacPwrCtrlOn) )
		{
			rtstatus = PlatformSdioCmd52ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								count, 
								(ULONG)offset, 
								TRUE,
								pdata);	
		}
		else
		{
			// Use I/O RW extended command as default.
			rtstatus = PlatformSdioCmd53ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								count, 
								(ULONG)offset, 
								TRUE,
								pdata);
		}

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	}

	if( pDefaultAdapter->bCtrlPnPTime )
	{
		EndTime = KeQueryPerformanceCounter(NULL);
		pDefaultAdapter->PnPIOTime.QuadPart += (EndTime.QuadPart - StartTime.QuadPart);
	}
}


//
//	Description: 
//		This routine is exported for SDIO WLAN IOREG I/O Bus Domain(Host),
//		Host Address Mapping Range is 0x1026_0000~0x1026_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
VOID
PlatformIOSyncWriteNByte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u4Byte		count,	//data length
	pu1Byte		pdata
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE		sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	BOOLEAN			bMacPwrCtrlOn = FALSE, bCmd52Available = TRUE;
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return;

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));		

	if(DeviceID == WLAN_IOREG_DEVICE_ID)
		bCmd52Available = pDefaultAdapter->HalFunc.HalSdioIoRegCmd52AvailableHandler(pDefaultAdapter, offset);

	RT_PRINT_DATA(COMP_INIT|COMP_IO, DBG_TRACE, "PlatformIOSyncWriteNByte(): Sync IO Write N byte:\n", 
				pdata, count);

#if (RK_PLATFORM_SUPPORT==1)
	if( bCmd52Available && ( (count <4) || 
		((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
		!bMacPwrCtrlOn) )
#else
	if( bCmd52Available && (sdiodevice->IoRegDirectAccess || 
		((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
		!bMacPwrCtrlOn) )
#endif
	{	
		rtstatus = PlatformSdioCmd52ReadWrite(
							sdiodevice, 
							DeviceID,
							sdiodevice->SdioFuncNum, 
							count, 
							(ULONG)offset, 
							TRUE,
							pdata);	
	}
	else
	{
		// Use I/O RW extended command as default.
		rtstatus = PlatformSdioCmd53ReadWrite(
							sdiodevice, 
							DeviceID,
							sdiodevice->SdioFuncNum, 
							count, 
							(ULONG)offset, 
							TRUE,
							pdata);
	}
	
}


//
//	Description: 
//		This routine is exported for SDIO WLAN IOREG I/O Bus Domain(Host),
//		Host Address Mapping Range is 0x1026_0000~0x1026_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
u1Byte
PlatformIORead1Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset
	)
{
	u1Byte				Data = 0;
	u1Byte	BytesRead = 0;
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	BOOLEAN			bMacPwrCtrlOn;
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;
	u1Byte	FwPSState;

	LARGE_INTEGER	StartTime, EndTime;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return 0xff;

	// For PnP IO Working Time Calculation
	if( pDefaultAdapter->bCtrlPnPTime )
		StartTime = KeQueryPerformanceCounter(NULL);

	NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	sdiodevice->SyncIoInProgressCount++;
	NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

	// tynli add for 32k. 2011.02.25.
	if(pDefaultAdapter->bFWReady)
	{
		pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);
		if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState) && 
			!IS_SDIO_POWER_ON_IO_REG(DeviceID, offset))
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("CANNOT IO ---> Wake up Hw. PlatformIORead1Byte(): DeviceID=%d, offset=%#X\n", DeviceID, offset));
			pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
		}
	}

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	
	{
		RT_ASSERT(FALSE, ("PlatformIORead1Byte() > PASSIVE_LEVEL is not allowed! DeviceID=%d, offset=%#X\n", DeviceID, offset));

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

		return 0xff;
	}
	else
	{
		if( sdiodevice->IoRegDirectAccess || 
			((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
			!bMacPwrCtrlOn )
		{
			rtstatus = PlatformSdioCmd52ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								1, 
								(ULONG)offset, 
								FALSE,
								&Data);	
		}
		else
		{
			// Use I/O RW extended command as default.
			rtstatus = PlatformSdioCmd53ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								1, 
								(ULONG)offset, 
								FALSE,
								&Data);	
		}			
		
		if(rtstatus != RT_STATUS_SUCCESS)
			Data = 0xff;	

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

		if( pDefaultAdapter->bCtrlPnPTime )
		{
			EndTime = KeQueryPerformanceCounter(NULL);
			pDefaultAdapter->PnPIOTime.QuadPart += EndTime.QuadPart - (StartTime.QuadPart);
		}

		return Data;
	}
}

//
//	Description: 
//		This routine is exported for SDIO WLAN IOREG I/O Bus Domain(Host),
//		Host Address Mapping Range is 0x1026_0000~0x1026_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
u2Byte
PlatformIORead2Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset
	)
{
	u2Byte				Data = 0;
	u1Byte	BytesRead = 0;
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	BOOLEAN			bMacPwrCtrlOn;
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;
	u1Byte	FwPSState;

	LARGE_INTEGER	StartTime, EndTime;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return 0xffff;

	// For PnP IO Working Time Calculation
	if( pDefaultAdapter->bCtrlPnPTime )
		StartTime = KeQueryPerformanceCounter(NULL);

	NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	sdiodevice->SyncIoInProgressCount++;
	NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

	// tynli add for 32k. 2011.02.25.		
	if(pDefaultAdapter->bFWReady)
	{	
		pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);
		if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState) && 
			!IS_SDIO_POWER_ON_IO_REG(DeviceID, offset))
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("CANNOT IO---> Wake up Hw. PlatformIORead2Byte(): DeviceID=%d, offset=%#X\n", DeviceID, offset));
			pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
		}
	}

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		RT_ASSERT(FALSE, ("PlatformIORead2Byte() > PASSIVE_LEVEL is not allowed! DeviceID=%d, offset=%#X\n", DeviceID, offset));

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

		return 0xffff;
	}
	else
	{
		if( sdiodevice->IoRegDirectAccess || 
			((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
			!bMacPwrCtrlOn )
		{
			rtstatus = PlatformSdioCmd52ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								2, 
								(ULONG)offset, 
								FALSE,
								&Data);	
		}
		else
		{
			// Use I/O RW extended command as default.
			rtstatus = PlatformSdioCmd53ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								2, 
								(ULONG)offset, 
								FALSE,
								&Data);
		}		
		
		if(rtstatus != RT_STATUS_SUCCESS)
			Data = 0xffff;	

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

		if( pDefaultAdapter->bCtrlPnPTime )
		{
			EndTime = KeQueryPerformanceCounter(NULL);
			pDefaultAdapter->PnPIOTime.QuadPart += (EndTime.QuadPart - StartTime.QuadPart);
		}
		
		return Data;
	}
}

//
//	Description: 
//		This routine is exported for SDIO WLAN IOREG I/O Bus Domain(Host),
//		Host Address Mapping Range is 0x1026_0000~0x1026_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
u4Byte
PlatformIORead4Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset
	)
{
	u4Byte				Data = 0;
	u1Byte	BytesRead = 0;
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	BOOLEAN			bMacPwrCtrlOn;
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;
	u1Byte	FwPSState;

	LARGE_INTEGER	StartTime, EndTime;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return 0xffffffff;

	// For PnP IO Working Time Calculation
	if( pDefaultAdapter->bCtrlPnPTime )
		StartTime = KeQueryPerformanceCounter(NULL);

	NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	sdiodevice->SyncIoInProgressCount++;
	NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

	// tynli add for 32k. 2011.02.25.
	if(pDefaultAdapter->bFWReady)
	{	
		pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);
		if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState) && 
			!IS_SDIO_POWER_ON_IO_REG(DeviceID, offset))
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("CANNOT IO---> Wake up Hw. PlatformIORead4Byte(): DeviceID=%d, offset=%#X\n", DeviceID, offset));
			pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
		}
	}

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		RT_ASSERT(FALSE, ("PlatformIORead4Byte() > PASSIVE_LEVEL is not allowed. DeviceID=%d, offset=%#X\n", DeviceID, offset));

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

		return 0xffffffff;
	}
	else
	{

#if (RK_PLATFORM_SUPPORT==1)
		if( ((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
			!bMacPwrCtrlOn )
#else
		if( sdiodevice->IoRegDirectAccess || 
			((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
			!bMacPwrCtrlOn )
#endif
		{
			rtstatus = PlatformSdioCmd52ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								4, 
								(ULONG)offset, 
								FALSE,
								&Data);	
		}
		else
		{
			// Use I/O RW extended command as default.
			rtstatus = PlatformSdioCmd53ReadWrite(
								sdiodevice,
								DeviceID,
								sdiodevice->SdioFuncNum, 
								4, 
								(ULONG)offset, 
								FALSE,
								&Data);
		}		
		
		if(rtstatus != RT_STATUS_SUCCESS)
			Data = 0xffffffff;	

		NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
		if(sdiodevice->SyncIoInProgressCount>0)
			sdiodevice->SyncIoInProgressCount--;
		else
			sdiodevice->SyncIoInProgressCount = 0;
		NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

		if( pDefaultAdapter->bCtrlPnPTime )
		{
			EndTime = KeQueryPerformanceCounter(NULL);
			pDefaultAdapter->PnPIOTime.QuadPart += (EndTime.QuadPart - StartTime.QuadPart);
		}
		
		return Data;
	}
}



//
//	Description: 
//		This routine is exported for SDIO WLAN IOREG or SDIO_LOCAL_DEVICE_ID I/O Bus Domain(Host),
//		Host Address Mapping Range is 0x1025_0000~0x1025_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2011.03.07, added by Roger.
//
VOID
PlatformIOReadNByte(
	IN	PVOID		Adapter,
	IN	u1Byte		DeviceID,
	IN	u4Byte		offset,
	IN	u4Byte		count,	//data length
	OUT	pu1Byte		pBuffer
	)
{	
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	BOOLEAN			bMacPwrCtrlOn;
	RT_STATUS	rtstatus = RT_STATUS_SUCCESS;
	u1Byte	FwPSState;

	LARGE_INTEGER	StartTime, EndTime;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
	{
		RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformIOReadNByte(): SDIO not allow to IO, Return\n"));
		PlatformFillMemory((PVOID)pBuffer, count, 0xff);
	}

	// For PnP IO Working Time Calculation
	if( pDefaultAdapter->bCtrlPnPTime )
		StartTime = KeQueryPerformanceCounter(NULL);

	NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	sdiodevice->SyncIoInProgressCount++;
	NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

	// tynli add for 32k. 2011.02.25.
	if(pDefaultAdapter->bFWReady)
	{	
		pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_FW_PS_STATE, &FwPSState);
		if(IS_IN_LOW_POWER_STATE(pDefaultAdapter, FwPSState) && 
			!IS_SDIO_POWER_ON_IO_REG(DeviceID, offset))
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("CANNOT IO---> Wake up Hw. PlatformIOReadNByte(): DeviceID=%d, offset=%#X\n", DeviceID, offset));
			pDefaultAdapter->HalFunc.SetHwRegHandler(pDefaultAdapter, HW_VAR_RESUME_CLK_ON, (pu1Byte)(&pDefaultAdapter));
		}
	}

	pDefaultAdapter->HalFunc.GetHwRegHandler(pDefaultAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{	
		RT_ASSERT(FALSE, ("PlatformIOReadNByte() > PASSIVE_LEVEL is not allowed. DeviceID=%d, offset=%#X\n", DeviceID, offset));		
		PlatformFillMemory((PVOID)pBuffer, count, 0xff);
	}
	else
	{		
		if( sdiodevice->IoRegDirectAccess || 
			((DeviceID == WLAN_IOREG_DEVICE_ID)&&(offset < 0x100)) || 
			!bMacPwrCtrlOn )
		{
			rtstatus = PlatformSdioCmd52ReadWrite(
								sdiodevice, 
								DeviceID,
								sdiodevice->SdioFuncNum, 
								count, 
								(ULONG)offset, 
								FALSE,
								pBuffer);	
		}
		else
		{
			// Use I/O RW extended command as default.
			rtstatus = PlatformSdioCmd53ReadWrite(
								sdiodevice,
								DeviceID,
								sdiodevice->SdioFuncNum, 
								count, 
								(ULONG)offset, 
								FALSE,
								pBuffer);
		}		
		
		if(rtstatus != RT_STATUS_SUCCESS)
			PlatformFillMemory((PVOID)pBuffer, count, 0xff);		
		
	}

	NdisAcquireSpinLock( &(sdiodevice->SyncIoCntSpinLock) );
	if(sdiodevice->SyncIoInProgressCount>0)
		sdiodevice->SyncIoInProgressCount--;
	else
		sdiodevice->SyncIoInProgressCount = 0;
	NdisReleaseSpinLock( &(sdiodevice->SyncIoCntSpinLock) );

	if( pDefaultAdapter->bCtrlPnPTime )
	{
		EndTime = KeQueryPerformanceCounter(NULL);
		pDefaultAdapter->PnPIOTime.QuadPart += (EndTime.QuadPart - StartTime.QuadPart);
	}
}



//
//	Description:
//		Retrieve SDIO bus related properties.
//
//	Assumption:
//		 A Secure Digital (SD) card bus interface has been initialized.	
//
//	2010.12.09, added by Roger. 
//
NTSTATUS
PlatformSdioGetProperty(
		IN PRT_SDIO_DEVICE	pDevice,
		IN SDBUS_PROPERTY Property,
              IN PVOID Buffer,
              IN ULONG Length
               )
{      	
     	PSDBUS_REQUEST_PACKET psdrp = NULL;
	NTSTATUS	status;
	
	psdrp = (PSDBUS_REQUEST_PACKET)ExAllocatePoolWithTag(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET), '3278');

	if(!psdrp)
		return STATUS_INSUFFICIENT_RESOURCES;
	
    	RtlZeroMemory(psdrp, sizeof(SDBUS_REQUEST_PACKET));

    	psdrp->RequestFunction = SDRF_GET_PROPERTY;
    	psdrp->Parameters.GetSetProperty.Property = Property;
    	psdrp->Parameters.GetSetProperty.Buffer = Buffer;
    	psdrp->Parameters.GetSetProperty.Length = Length;

	NdisAcquireSpinLock( &(pDevice->IrpSpinLock) );
	RT_SDIO_INC_CMD_REF(pDevice);
	NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );

	//
    	// Send the IO request down to the bus driver
    	//
    	status = SdBusSubmitRequest(pDevice->Sdbusinterface.Context, psdrp);

	NdisAcquireSpinLock( &(pDevice->IrpSpinLock) );
	RT_SDIO_DEC_CMD_REF(pDevice);
	NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );

	if(!NT_SUCCESS(status))
	{
		RT_TRACE(COMP_IO, DBG_SERIOUS, ("PlatformSdioGetProperty(): Fail to get SDIO property!!\n"));
	}
   	ExFreePoolWithTag(psdrp, '3278');
   
    	return status;
}


//
//	Description:
//		Configure sdio bus related properties.
//
//	Assumption:
//		 A Secure Digital (SD) card bus interface has been initialized.	
//
//	2010.12.09, added by Roger. 
//
NTSTATUS
PlatformSdioSetProperty(
               IN PRT_SDIO_DEVICE	pDevice,
               IN SDBUS_PROPERTY Property,
               IN PVOID Buffer,
               IN ULONG Length
               )
{

	PSDBUS_REQUEST_PACKET psdrp = NULL;
	NTSTATUS	status;
	
	psdrp = (PSDBUS_REQUEST_PACKET)ExAllocatePoolWithTag(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET), '3278');

	if(!psdrp)
		return STATUS_INSUFFICIENT_RESOURCES;

	RtlZeroMemory(psdrp, sizeof(SDBUS_REQUEST_PACKET));

    	psdrp->RequestFunction = SDRF_SET_PROPERTY;
    	psdrp->Parameters.GetSetProperty.Property = Property;
    	psdrp->Parameters.GetSetProperty.Buffer = Buffer;
    	psdrp->Parameters.GetSetProperty.Length = Length;    	

	NdisAcquireSpinLock( &(pDevice->IrpSpinLock) );
	RT_SDIO_INC_CMD_REF(pDevice);
	NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );

	//
    	// Send the IO request down to the bus driver
    	//
	status = SdBusSubmitRequest(pDevice->Sdbusinterface.Context, psdrp);

	NdisAcquireSpinLock( &(pDevice->IrpSpinLock) );
	RT_SDIO_DEC_CMD_REF(pDevice);
	NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );

	if(!NT_SUCCESS(status))
	{
		RT_TRACE(COMP_IO, DBG_SERIOUS, ("PlatformSdioSetProperty(): Fail to set SDIO property!!\n"));
	}
   	ExFreePoolWithTag(psdrp, '3278');
   
    	return status;
}



//
//	Description: 
//		Schedule IO_RW_EXTENDED Command (CMD53) to perform Read Write I/O operation 
//		on specific function number. This command allows the reading of a large number of I/O 
//		registers with a single command. We should decide what kind of mode(Block or byte count mode) 
//		to use.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling this function.
//
//	2010.12.14, added by Roger.
//
RT_STATUS
PlatformSdioCmd53ReadWrite(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			DeviceID,
	UCHAR			funcNum,			
	ULONG			byteCount,
	ULONG			registerIndex,
	BOOLEAN			writeFlag,
	PVOID			buffer
    )
{
	u4Byte		TargetAddr = registerIndex;
	PADAPTER	pAdapter = (PADAPTER)sdiodevice->pAdapter;
	RT_STATUS	rtstatus = RT_STATUS_FAILURE;
	u1Byte	value8 = 0;
	BOOLEAN		bMacPwrCtrlOn;
	u1Byte		SDRPErrHandleType;

	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	
	
	if(!bMacPwrCtrlOn)
	{
		RT_TRACE(COMP_POWER, DBG_WARNING, ("PlatformSdioCmd53ReadWrite(): registerIndex(%#x), power is off return!!\n", registerIndex));
		return rtstatus;
	}

	if( funcNum == sdiodevice->SdioFuncNum )
		pAdapter->HalFunc.HalSdioGetCmdAddressHandler( pAdapter, DeviceID, registerIndex, &TargetAddr );

	//
	// <Roger_Notes> We should not use block mode while the number of byte count is less than functional block size.
	// Because the length field(Indicates the length, in bytes, of the response data) in DeviceCommand 
	// will be the multiple of block size, which might cause memory corruption while using MDL description.
	// 2011.03.09.
	//
	if((sdiodevice->SdioTxBlockMode || sdiodevice->SdioRxBlockMode)
		&& (byteCount >= sdiodevice->SdioFuncBlockSize ))
	{
		//Block mode
		rtstatus = PlatformSdioCmd53ReadWriteBlock(
							sdiodevice, 
							funcNum, 
							byteCount, 
							(ULONG)TargetAddr, 
							writeFlag,
							buffer);
	}
	else
	{
		//Byte count mode
		rtstatus = PlatformSdioCmd53ReadWriteByte(
							sdiodevice, 
							funcNum, 
							byteCount, 
							(ULONG)TargetAddr, 
							writeFlag,
							buffer);
	}

	// For debug info.
	if(rtstatus != RT_STATUS_SUCCESS)
	{
		if(rtstatus == RT_STATUS_IO_EXCEPTION)
		{
			sdiodevice->FailureCount++;
			if(sdiodevice->FailureCount == 1) // To avoid infinite loop
			{
				//Check SD_CLK and Power OK 
				RT_TRACE(COMP_INIT, DBG_LOUD, ("[DBG] Cmd52 R REG 0x0[31:0]: %#x, 0x100[31:0]: %#x,\n", 
					PlatformEFSdioCmd52Read4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0),
					PlatformEFSdioCmd52Read4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0x100)));

				//Check CMD53 and MAC power on
				RT_TRACE(COMP_INIT, DBG_LOUD, ("[DBG] Cmd53 R REG 0x100[31:0]: %#x\n", 
					PlatformEFSdioCmd53Read4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, 0x100)));
				
				//Check CMD3 Write
				PlatformEFSdioCmd53Write4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, 0x1b8, 0x12345678); 		 
				RT_TRACE(COMP_INIT, DBG_LOUD, ("[DBG] Cmd53 W => Cmd52 R REG 0x1b8[31:0]: %#x\n", 
					PlatformEFSdioCmd52Read4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0x1b8)));
			}

			// Set bSurpriseRemoved flag depends on IO function type.
			SDRPErrHandleType = HAL_GetSDRPErrorHandlingType(pAdapter, TargetAddr);
			if(SDRPErrHandleType == SDRP_ERROR_SURPRISE_REMOVED ||
				SDRPErrHandleType == SDRP_ERROR_RETRY)
			{
				if(sdiodevice->bRegSurpriseRemovedEnable)
				{
					if(!(GlobalSdioDbg & SDIO_DBG_LA_TRIGER))
						pAdapter->bSurpriseRemoved = TRUE;
				}
			}

		}

		if(GlobalSdioDbg & SDIO_DBG_LA_TRIGER)
		{
			RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformSdioCmd53ReadWrite(): Trigger debug info!!\n"));
			value8 = PlatformEFSdioCmd52Read1Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0x61);
			PlatformEFSdioCmd52Write1Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0x61, value8|BIT0);
			RT_TRACE(COMP_IO, DBG_WARNING, ("0xC1(%#x)\n", PlatformEFSdioLocalCmd52Read2Byte(pAdapter, 0xc1)));
			RT_TRACE(COMP_IO, DBG_WARNING, ("0x80(%#x)\n", PlatformEFSdioLocalCmd52Read4Byte(pAdapter, 0x80)));
			pAdapter->bSurpriseRemoved = TRUE;
		}
	}
	else
	{
		sdiodevice->FailureCount = 0;
	}
	
	return rtstatus;
}


	//
//	Description: 
//		Schedule IO_RW_EXTENDED Command (CMD53) to perform Read Write I/O operation 
//		on specific function number. This command allows the reading of a large number of I/O 
//		registers with a single command.
	//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling this function.
//
//	2010.12.14, added by Roger.
//
RT_STATUS
PlatformSdioCmd53ReadWriteBlock(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			funcNum,			
	ULONG			byteCount,
	ULONG			registerIndex,
	BOOLEAN			writeFlag,
	PVOID			buffer
)
	{

	PMDL pmdl = NULL;	
	RT_STATUS	status = RT_STATUS_FAILURE;
	u4Byte		nBytesForMDL = 0;	
	

	//
	// We shall keep identity of bytes count between MDL allocation and corresponding SDBUS_REQUEST_PACKET we are going to send.
	// Revised by Roger, 2014.01.02.
	//
	nBytesForMDL = ((byteCount/sdiodevice->SdioFuncBlockSize)+((byteCount%sdiodevice->SdioFuncBlockSize?1:0)))*sdiodevice->SdioFuncBlockSize;
	

	//
	// First get a MDL to map the data. We asume the caller passed a buffer to non pageed pool.
	//
	pmdl = IoAllocateMdl((pu1Byte)buffer, nBytesForMDL , FALSE, FALSE, NULL);	
	
	if(pmdl == NULL)
		return status;
	
	MmBuildMdlForNonPagedPool(pmdl);

	status = PlatformSdioCmd53ReadWriteMDL(
								sdiodevice, 
								funcNum, 
								pmdl,
								nBytesForMDL, 
								(ULONG)registerIndex, 
								writeFlag,
								TRUE);
	
	if(pmdl){
		IoFreeMdl(pmdl);
	}

	return status;
}


//
//	Description: 
//		Schedule IO_RW_EXTENDED Command (CMD53) to perform Read Write I/O operation 
//		on specific function number. This command allows the reading of a large number of I/O 
//		registers with a single command.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling this function.
//
//	2010.12.14, added by Roger.
//
RT_STATUS
PlatformSdioCmd53ReadWriteByte(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			funcNum,			
	ULONG			byteCount,
	ULONG			registerIndex,
	BOOLEAN			writeFlag,
	PVOID			buffer
)
{
	ULONG 	TotalLength = 0, RwLength = 0;
	PUCHAR	VirtualAddr = (PUCHAR)buffer;
	PMDL pmdl = NULL;	
	RT_STATUS	rtstatus = RT_STATUS_FAILURE;

	TotalLength = byteCount; // Total length needs to transfer

	do{
		
		RwLength = (TotalLength > sdiodevice->SdioFuncBlockSize) ? sdiodevice->SdioFuncBlockSize : TotalLength;
		pmdl = IoAllocateMdl((pu1Byte)VirtualAddr, RwLength , FALSE, FALSE, NULL);	
		
		if(pmdl == NULL)
			break;
		
		MmBuildMdlForNonPagedPool(pmdl);

		rtstatus = PlatformSdioCmd53ReadWriteMDL(
								sdiodevice, 
								funcNum, 
								pmdl,
								RwLength, 
								(ULONG)registerIndex, 
								writeFlag,
								FALSE);

		if( rtstatus != RT_STATUS_SUCCESS)
		{
			IoFreeMdl(pmdl);
			break;
		}

		VirtualAddr += RwLength;
		TotalLength -= RwLength;
		IoFreeMdl(pmdl);		
		
	}while(TotalLength > 0);
	
	return rtstatus;
}


//
//	Description: 
//		Schedule IO_RW_EXTENDED Command (CMD53) to perform Read Write I/O operation 
//		on specific function number. This command allows the reading of a large number of I/O 
//		registers with a single command.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling this function.
//
//	2010.12.14, added by Roger.
//
RT_STATUS
PlatformSdioCmd53ReadWriteMDL(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			funcNum,	
	PMDL			pmdl,
	ULONG			byteCount,
	ULONG			registerIndex,
	BOOLEAN			writeToDevice,	
	BOOLEAN			blockMode
)
{

	SD_RW_EXTENDED_ARGUMENT extendedArgument;
	PSDBUS_REQUEST_PACKET  psdrp;	
	NTSTATUS	status = STATUS_UNSUCCESSFUL ;
	RT_STATUS	rtstatus = RT_STATUS_FAILURE;
	BOOLEAN		bSdBusReqSent = FALSE;
	PADAPTER	Adapter = sdiodevice->pAdapter;
	BOOLEAN		bMacPwrCtrlOn;
	u1Byte		RetryCnt, SDRPErrHandleType;
	u1Byte		FwPSState;
	BOOLEAN		bIOFailIn32k = FALSE;

	const SDCMD_DESCRIPTOR WriteIoExtendedDesc = {
							SDCMD_IO_RW_EXTENDED,
                                                 SDCC_STANDARD,
                                                 SDTD_WRITE,
                                                 SDTT_SINGLE_BLOCK,
                                                 SDRT_5};

	const SDCMD_DESCRIPTOR ReadIoExtendedDesc = {
							SDCMD_IO_RW_EXTENDED, 
							SDCC_STANDARD, 
							SDTD_READ, 
							SDTT_SINGLE_BLOCK, 
							SDRT_5}; 									

	LARGE_INTEGER	StartTime, EndTime, Freq;
	u1Byte			RecordIndex = sdiodevice->BusTimeRecordIndex;

	//
	// Return immediately if SDIO can NOT workig properly anymore.
	//
	if(RT_SDIO_CANNOT_IO(Adapter))
		return rtstatus;			

	//
	// Now allocate a request packet for the arguments of the command.
	//	
	psdrp = ExAllocatePoolWithTag(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET), '3278');
	if(!psdrp) 
	{
		RT_TRACE(COMP_IO, DBG_SERIOUS, ("PlatformSdioCmd53ReadWriteMDL(): Allocate sdrp fail!!\n"));
		return rtstatus;
	}
		
	RtlZeroMemory(psdrp, sizeof(SDBUS_REQUEST_PACKET));

	psdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	psdrp->Parameters.DeviceCommand.Mdl = pmdl;

	//
	// Set up the argument and command descriptor.
	//
	extendedArgument.u.AsULONG = 0;
	
	if (blockMode)
	{ // Block Mode.

		extendedArgument.u.bits.OpCode  = 1; //Increment address
		
		extendedArgument.u.bits.Count = 
			(byteCount/sdiodevice->SdioFuncBlockSize) + 
			((byteCount%sdiodevice->SdioFuncBlockSize?1:0));
		
		psdrp->Parameters.DeviceCommand.Length = 
			(extendedArgument.u.bits.Count)*sdiodevice->SdioFuncBlockSize;
	}
	else
	{ // Byte Count Mode.
		if( byteCount == sdiodevice->SdioFuncBlockSize)
			extendedArgument.u.bits.Count = 0;
	else
			extendedArgument.u.bits.Count = byteCount;
		
		psdrp->Parameters.DeviceCommand.Length = extendedArgument.u.bits.Count;
	}
	
	extendedArgument.u.bits.Function = funcNum;
	extendedArgument.u.bits.Address = registerIndex;	
	
	if (writeToDevice) {
        	extendedArgument.u.bits.WriteToDevice = 1;
        	psdrp->Parameters.DeviceCommand.CmdDesc  = WriteIoExtendedDesc;
    	} 
	else {
        	psdrp->Parameters.DeviceCommand.CmdDesc  = ReadIoExtendedDesc;
    	}


    	if (blockMode) {
		extendedArgument.u.bits.BlockMode = blockMode;
       	psdrp->Parameters.DeviceCommand.CmdDesc.TransferType = SDTT_MULTI_BLOCK_NO_CMD12;
    	}
		
	psdrp->Parameters.DeviceCommand.Argument = extendedArgument.u.AsULONG;	

	//
	// <Roger_TODO> To make sure that only one request is pending in SDIO Host.
	// 2011.01.10.
	//
	NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );
	sdiodevice->SyncIoWaitingCount++;
	NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );

	while(TRUE)
	{
		
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	
		
		if(RT_SDIO_CANNOT_IO(Adapter) || 
			(bMacPwrCtrlOn == FALSE))
		{ // Driver is going to unload or power is going to turn off

			NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );
			sdiodevice->SyncIoWaitingCount--;
			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );		
			RT_TRACE(COMP_IO, DBG_LOUD, ("PlatformSdioCmd53ReadWriteMDL(): Return bMacPwrCtrlOn=%d\n", bMacPwrCtrlOn));
			break;
		}

		NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );

		if(sdiodevice->nIrpPendingCnt == 0)
		{ // No IRP pending in SDIO host driver.

			sdiodevice->nIrpPendingCnt++;
			RT_SDIO_INC_CMD_REF(sdiodevice);

			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );

			RetryCnt = 0;
			SDRPErrHandleType = HAL_GetSDRPErrorHandlingType(Adapter, registerIndex);

			do
			{
				// ----- For IO Working Time Measurement -----
				StartTime = KeQueryPerformanceCounter(&Freq);
				sdiodevice->SdBusSubmitTime[RecordIndex].AbTime = PlatformGetCurrentTime();
				//--------------------------------------------
			
				//
			    // Send the IO request down to the bus driver
			    //
				status = SdBusSubmitRequest(sdiodevice->Sdbusinterface.Context, psdrp);	

				// ----- For IO Working Time Measurement -----
				EndTime = KeQueryPerformanceCounter(NULL);
				sdiodevice->SdBusSubmitTime[RecordIndex].SdBusWorkDuration.QuadPart = (EndTime.QuadPart - StartTime.QuadPart)*1000000 / Freq.QuadPart;
				sdiodevice->BusTimeRecordIndex = (RecordIndex<6? RecordIndex+1 : 0);

				if( Adapter->bCtrlPnPTime )
					sdiodevice->PnPSdBusWorkTime.QuadPart += (EndTime.QuadPart - StartTime.QuadPart);
				//--------------------------------------------

				if( !NT_SUCCESS(status) )
				{
					RetryCnt++;

					if( status == STATUS_IO_TIMEOUT )
					{
						RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformSdioCmd53ReadWriteMDL(): Offset(%#x) WriteOrRead(%d), STATUS_IO_TIMEOUT\n", 
							registerIndex, writeToDevice));
					}
					else if( status == STATUS_CRC_ERROR )
					{ 
						RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformSdioCmd53ReadWriteMDL(): Offset(%#x) WriteOrRead(%d), STATUS_CRC_ERROR\n", 
							registerIndex, writeToDevice)); 				
					}
					else
					{
						RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformSdioCmd53ReadWriteMDL(): Offset(%#x) WriteOrRead(%d) fail!!status(%#x)\n", 
								registerIndex, writeToDevice, status));
						// 0xC000009C. STATUS_DEVICE_DATA_ERROR
					}
					
					if(GlobalSdioDbg & SDIO_DBG_CMD)
					{
						RT_ASSERT(FALSE, ("Fail to submit CMD53 SD Bus request!! status(%x)\n", status));
					}

					// Check if HW is able to enter 32K state. It is an workaround and sould be removed later.
					Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_FW_PS_STATE, &FwPSState);
					if(IS_IN_LOW_POWER_STATE(Adapter, FwPSState))
					{
						bIOFailIn32k = TRUE;
						RT_TRACE(COMP_POWER, DBG_LOUD, ("<--- Skip CMD53 IO fail because Hw is in 32K state.\n"));
						break;
					}

					if(SDRPErrHandleType == SDRP_ERROR_SKIP ||
						SDRPErrHandleType == SDRP_ERROR_SURPRISE_REMOVED)
					{
						break;
					}
					else if(SDRPErrHandleType == SDRP_ERROR_RETRY && RetryCnt >= SDRP_MAX_ERROR_RETRY_CNT)
					{
						break;
					}
					
					delay_us(2);
				}
			}while(!NT_SUCCESS(status));

			NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );
			RT_SDIO_DEC_CMD_REF(sdiodevice);
			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );

			bSdBusReqSent = TRUE;
			break;
		}
		else if(sdiodevice->nIrpPendingCnt == 1) 
		{ // There is one IRP pending in SDIO host driver. 

			KEVENT VoidEvent;
			LARGE_INTEGER VoidEventWaitTime;
			NTSTATUS VoidEventWaitStatus;

			RT_TRACE(COMP_DBG, DBG_TRACE, ("***** SynIo is going to wait for a while *****\n"));

			// SyncIo Method 2 		
			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );
			VoidEventWaitStatus = KeWaitForSingleObject(&sdiodevice->SyncIoEvent, Suspended, KernelMode, FALSE, NULL);
			if(VoidEventWaitStatus == STATUS_SUCCESS)
			{
				KeClearEvent( &sdiodevice->SyncIoEvent );
			}
			else
			{
				RT_ASSERT(FALSE, ("VoidEventWaitStatus: %x !!!\n", VoidEventWaitStatus));
			}			
		}
		else
		{ // Error condition.
			RT_ASSERT(FALSE, ("PlatformSdioCmd52ReadWriteByte(): sdiodevice->nIrpPendingCnt: %d !!!\n", 
				sdiodevice->nIrpPendingCnt));
			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );
			break;
		}
	}

	if( bSdBusReqSent == TRUE )
	{
		NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );
		if(sdiodevice->SyncIoWaitingCount>0)
			sdiodevice->SyncIoWaitingCount--;
		else
			sdiodevice->SyncIoWaitingCount=0;
		NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );

		if(NT_SUCCESS(status))
		{
			SdioIOComplete(sdiodevice);
			rtstatus = RT_STATUS_SUCCESS;
		}
		else
		{
			SdioIOComplete(sdiodevice);
			
			//
			// <Roger_TODO> We should perform necessary error handling here.
			// e.g., Surprise removed or driver stopped control flag.
			//
			if(bIOFailIn32k)
				rtstatus = RT_STATUS_FAILURE;
			else
				rtstatus = RT_STATUS_IO_EXCEPTION;
		}

	}

	ExFreePoolWithTag(psdrp, '3278');		 

	return rtstatus;		
}



//
//	Description: 
//		Schedule a serial IO_RW_DIRECT Command (CMD52) to read or write bytes within the total 128K 
//		of register space in any I/O function, including the common I/O area(CIA). This command 
//		reads or writes 1 byte using only 1 command/response pair.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling this function.
//
//	2010.12.14, created by Roger.
//
RT_STATUS
PlatformSdioCmd52ReadWrite(
	PRT_SDIO_DEVICE	sdiodevice,
	u1Byte		DeviceID,
	UCHAR			funcNum,	
	ULONG			byteCount,
	ULONG			registerIndex,
	BOOLEAN			writeToDevice,
	PVOID			buffer
    )
{
	u1Byte	i = 0;
	u4Byte	TargetAddr = registerIndex;
	PADAPTER	pAdapter = (PADAPTER)sdiodevice->pAdapter;
	RT_STATUS	status = RT_STATUS_FAILURE;
	u1Byte		SDRPErrHandleType;


	if(RT_SDIO_CANNOT_IO(pAdapter))
		return status;
	
	if(IS_VENDOR_8723B_D_CUT(pAdapter))
	{
		if((DeviceID == WLAN_IOREG_DEVICE_ID) && (registerIndex == 0x948) && writeToDevice)
		{
			RT_TRACE(COMP_INIT, DBG_WARNING, ("Return ==> PlatformSdioCmd52ReadWrite(): Offset(%#x) WriteOrRead(%d), byteCount(%d), Data[0]=%p\n", registerIndex, writeToDevice, byteCount, (PUCHAR)buffer));	
			return status;
		}
	}
	
	if( funcNum == sdiodevice->SdioFuncNum )
		pAdapter->HalFunc.HalSdioGetCmdAddressHandler(pAdapter, DeviceID, registerIndex, &TargetAddr);	
	
	
	for( i=0 ; i<byteCount ; i++ )
	{	
		status = PlatformSdioCmd52ReadWriteByte(
							sdiodevice, 
							funcNum, 							
							(ULONG)TargetAddr+i, //Reg offset 							
							(PUCHAR)buffer+i, //Data
							writeToDevice);

		if( status != RT_STATUS_SUCCESS )	
		{
			RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformSdioCmd52ReadWrite(): Offset(%#x) WriteOrRead(%d) fail!!\n", TargetAddr, writeToDevice));

			if(status == RT_STATUS_IO_EXCEPTION)
			{
				sdiodevice->FailureCount++;
				if(sdiodevice->FailureCount == 1) // To avoid infinite loop
				{
					//Check SD_CLK and Power OK 
					RT_TRACE(COMP_INIT, DBG_LOUD, ("[DBG] Cmd52 R REG 0x0[31:0]: %#x, 0x100[31:0]: %#x,\n", 
						PlatformEFSdioCmd52Read4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0),
						PlatformEFSdioCmd52Read4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0x100)));
					
					//Check CMD53 and MAC power on
					RT_TRACE(COMP_INIT, DBG_LOUD, ("[DBG] Cmd53 R REG 0x100[31:0]: %#x\n", 
						PlatformEFSdioCmd53Read4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, 0x100)));
					
					//Check CMD3 Write
					PlatformEFSdioCmd53Write4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, 0x1b8, 0x12345678);		 
					RT_TRACE(COMP_INIT, DBG_LOUD, ("[DBG] Cmd53 W => Cmd52 R REG 0x1b8[31:0]: %#x\n", 
						PlatformEFSdioCmd52Read4Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0x1b8)));
				}

				// Set bSurpriseRemoved flag depends on IO function type.
				SDRPErrHandleType = HAL_GetSDRPErrorHandlingType(pAdapter, TargetAddr+i);
				if(SDRPErrHandleType == SDRP_ERROR_SURPRISE_REMOVED ||
					SDRPErrHandleType == SDRP_ERROR_RETRY)
				{
					if(sdiodevice->bRegSurpriseRemovedEnable)
					{
						if(!(GlobalSdioDbg & SDIO_DBG_LA_TRIGER))
							pAdapter->bSurpriseRemoved = TRUE;
					}
				}

			}

			if(GlobalSdioDbg & SDIO_DBG_LA_TRIGER)
			{
				u1Byte	value8;
				RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformSdioCmd52ReadWrite(): Trigger debug info!!\n"));
				value8 = PlatformEFSdioCmd52Read1Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0x61);
				PlatformEFSdioCmd52Write1Byte(pAdapter, WLAN_IOREG_DEVICE_ID, sdiodevice->SdioFuncNum, 0x61, value8|BIT0);
				RT_TRACE(COMP_IO, DBG_WARNING, ("0xC1(%#x)\n", PlatformEFSdioLocalCmd52Read2Byte(pAdapter, 0xc1)));
				RT_TRACE(COMP_IO, DBG_WARNING, ("0x80(%#x)\n", PlatformEFSdioLocalCmd52Read4Byte(pAdapter, 0x80)));
				pAdapter->bSurpriseRemoved = TRUE;
			}			
			break;		
		}
		else
		{
			sdiodevice->FailureCount = 0;
		}
	}	

	return status;
}



	//
//	Description: 
//		Schedule IO_RW_DIRECT Command (CMD52) to read or write a single register within the total 128K 
//		of register space in any I/O function, including the common I/O area(CIA). This command 
//		reads or writes 1 byte using only 1 command/response pair.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling this function.
//
//	2010.12.14, added by Roger.
	//	
RT_STATUS
PlatformSdioCmd52ReadWriteByte(
	PRT_SDIO_DEVICE	sdiodevice,
	UCHAR			funcNum,
	ULONG			registerIndex,
	PUCHAR			data,
	BOOLEAN			writeToDevice
)
{

	SDBUS_REQUEST_PACKET       sdrp;
   	SD_RW_DIRECT_ARGUMENT        directArgument;
	PADAPTER	Adapter = sdiodevice->pAdapter;
	NTSTATUS	status = STATUS_UNSUCCESSFUL ;
	RT_STATUS	rtstatus = RT_STATUS_FAILURE;
	BOOLEAN		bSdBusReqSent = FALSE;
	u1Byte		RetryCnt, SDRPErrHandleType;

	const SDCMD_DESCRIPTOR ReadIoDirectDesc = {
		  					SDCMD_IO_RW_DIRECT,
							SDCC_STANDARD,
							SDTD_READ,
							SDTT_CMD_ONLY,
							SDRT_5};

	const SDCMD_DESCRIPTOR WriteIoDirectDesc = {
							SDCMD_IO_RW_DIRECT, 
							SDCC_STANDARD, 
							SDTD_WRITE, 
							SDTT_CMD_ONLY, 
							SDRT_5};

	LARGE_INTEGER	StartTime, EndTime, Freq;
	u1Byte			RecordIndex = sdiodevice->BusTimeRecordIndex;
	
	RtlZeroMemory(&sdrp, sizeof(SDBUS_REQUEST_PACKET));
	RtlZeroMemory(&directArgument, sizeof(SD_RW_DIRECT_ARGUMENT));
	
	
	sdrp.RequestFunction = SDRF_DEVICE_COMMAND;	

	directArgument.u.AsULONG = 0;
	directArgument.u.bits.Address = registerIndex;
	directArgument.u.bits.Function = funcNum;

	 if (writeToDevice) {
        	directArgument.u.bits.WriteToDevice = 1;
        	directArgument.u.bits.Data = *data;
        	sdrp.Parameters.DeviceCommand.CmdDesc  = WriteIoDirectDesc;
    	} 
	 else {
        	sdrp.Parameters.DeviceCommand.CmdDesc  = ReadIoDirectDesc;
    	}
	
	sdrp.Parameters.DeviceCommand.Argument = directArgument.u.AsULONG;

	//
	// <Roger_TODO> To make sure that only one request is pending in SDIO Host.
	// 2011.01.10.
	//
	NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );
	sdiodevice->SyncIoWaitingCount++;
	NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );

	while(TRUE)
	{
		
		if(RT_SDIO_CANNOT_IO(Adapter))
		{ // Driver is going to unload.

			NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );
			sdiodevice->SyncIoWaitingCount--;
			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );		
			break;
		}

		NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );

		if(sdiodevice->nIrpPendingCnt == 0)
		{ // No IRP pending in SDIO host driver.

			sdiodevice->nIrpPendingCnt++;
			RT_SDIO_INC_CMD_REF(sdiodevice);
			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );

			RetryCnt = 0;
			SDRPErrHandleType = HAL_GetSDRPErrorHandlingType(Adapter, registerIndex);
			
			do
			{
				// ----- For IO Working Time Measurement -----
				StartTime = KeQueryPerformanceCounter(&Freq);
				sdiodevice->SdBusSubmitTime[RecordIndex].AbTime = PlatformGetCurrentTime();
				//--------------------------------------------
			
				//
		    	// Send the IO request down to the bus driver
		    	//
				status = SdBusSubmitRequest(sdiodevice->Sdbusinterface.Context, &sdrp);

				// ----- For IO Working Time Measurement -----
				EndTime = KeQueryPerformanceCounter(NULL);
				sdiodevice->SdBusSubmitTime[RecordIndex].SdBusWorkDuration.QuadPart = (EndTime.QuadPart - StartTime.QuadPart)*1000000 / Freq.QuadPart;
				sdiodevice->BusTimeRecordIndex = (RecordIndex<6? RecordIndex+1 : 0);

				if( Adapter->bCtrlPnPTime )
					sdiodevice->PnPSdBusWorkTime.QuadPart += (EndTime.QuadPart - StartTime.QuadPart);
				//--------------------------------------------

				if( !NT_SUCCESS(status) )
				{
					RetryCnt++;

					if( status == STATUS_IO_TIMEOUT )
					{
						RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformSdioCmd52ReadWriteByte(): registerIndex(%#x) WriteOrRead(%d) STATUS_IO_TIMEOUT\n", 
							registerIndex, writeToDevice));
					}
					else if( status == STATUS_CRC_ERROR )
					{
						RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformSdioCmd52ReadWriteByte(): registerIndex(%#x) WriteOrRead(%d) STATUS_CRC_ERROR\n", 
							registerIndex, writeToDevice));
					}
					else
					{
						RT_TRACE(COMP_IO, DBG_WARNING, ("PlatformSdioCmd52ReadWriteByte(): Offset(%#x) WriteOrRead(%d) fail!!status(%#x)\n", 
								registerIndex, writeToDevice, status));
					}
					
					if(GlobalSdioDbg & SDIO_DBG_CMD)
					{
						RT_ASSERT(FALSE, ("Fail to submit CMD52 SD Bus request!! status(%x)\n", status));
					}

					if(SDRPErrHandleType == SDRP_ERROR_SKIP ||
						SDRPErrHandleType == SDRP_ERROR_SURPRISE_REMOVED)
					{
						break;
					}
					else if(SDRPErrHandleType == SDRP_ERROR_RETRY && RetryCnt >= SDRP_MAX_ERROR_RETRY_CNT)
					{
						break;
					}
			
					delay_us(2);
				}
			}while(!NT_SUCCESS(status));
			
			NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );
			RT_SDIO_DEC_CMD_REF(sdiodevice);
			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );

			bSdBusReqSent = TRUE;
			
			if (NT_SUCCESS(status)  && !writeToDevice) {
		        	*data = sdrp.ResponseData.AsUCHAR[0];
			}

			break;
		}
		else if(sdiodevice->nIrpPendingCnt == 1) 
		{ // There is one IRP pending in SDIO host driver. 
		
			KEVENT VoidEvent;
			LARGE_INTEGER VoidEventWaitTime;
			NTSTATUS VoidEventWaitStatus;

			RT_TRACE(COMP_DBG, DBG_TRACE, ("***** SynIo is going to wait for a while *****\n"));

			// SyncIo Method 2 		
			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );
			VoidEventWaitStatus = KeWaitForSingleObject(&sdiodevice->SyncIoEvent, Suspended, KernelMode, FALSE, NULL);
			if(VoidEventWaitStatus == STATUS_SUCCESS)
			{
				KeClearEvent( &sdiodevice->SyncIoEvent );
			}
			else
			{
				RT_ASSERT(FALSE, ("VoidEventWaitStatus: %x !!!\n", VoidEventWaitStatus));
			}			
		}
		else
		{ // Error condition.
			RT_ASSERT(FALSE, ("PlatformSdioCmd52ReadWriteByte(): sdiodevice->nIrpPendingCnt: %d !!!\n", 
				sdiodevice->nIrpPendingCnt));
			NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );
			break;
		}
	}
	
	if( bSdBusReqSent == TRUE )
	{
		NdisAcquireSpinLock( &(sdiodevice->IrpSpinLock) );
		if(sdiodevice->SyncIoWaitingCount>0)
			sdiodevice->SyncIoWaitingCount--;
		else
			sdiodevice->SyncIoWaitingCount=0;
		NdisReleaseSpinLock( &(sdiodevice->IrpSpinLock) );

		if(NT_SUCCESS(status))
		{
			SdioIOComplete(sdiodevice);
			rtstatus = RT_STATUS_SUCCESS;
		}
		else
		{
			SdioIOComplete(sdiodevice);
			
			//
			// <Roger_TODO> We should perform necessary error handling here.
			// e.g., Surprise removed or driver stopped control flag.
			//
			rtstatus = RT_STATUS_IO_EXCEPTION;
		}

	}
	
	return rtstatus;
}



//
//	Description: 
//		This routine is exported for specific I/O Bus Domain(Host) and Device ID.
//		Host Address Mapping Range is 0x1025_0000~0x1027_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//		PASSIVE_LEVEL
//
//	2010.12.10, added by Roger.
//
u1Byte
PlatformSdioCmd52Read1Byte(
	IN	PVOID		Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	u1Byte	Data;
	RT_STATUS	status = RT_STATUS_SUCCESS;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return 0xff;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	
	{		
		RT_ASSERT(FALSE, ("PlatformSdioCmd52Read1Byte() in PASSIVE_LEVEL is not allowed!!!\n"));
		return 0xff;		
	}
	else
	{
		status = PlatformSdioCmd52ReadWrite(
								sdiodevice, 
								DeviceID,
								FuncNum, 
								1, 
								(ULONG)offset, 
								FALSE,
								&Data);	
	}
	return Data;
	
	}


//
//	Description: 
//		This routine is exported for specific I/O Bus Domain(Host) and Device ID.
//		Host Address Mapping Range is 0x1025_0000~0x1027_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
u2Byte
PlatformSdioCmd52Read2Byte(
	IN	PVOID		Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	u2Byte	Data = 0;
	RT_STATUS	status = RT_STATUS_SUCCESS;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return 0xffff;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	
	{
		RT_ASSERT(FALSE, ("PlatformSdioCmd52Read2Byte() in PASSIVE_LEVEL is not allowed!!!\n"));		
	}
	else
		{ 
		status = PlatformSdioCmd52ReadWrite(
									sdiodevice, 
									DeviceID,
									FuncNum, 
									2, 
									(ULONG)offset, 
									FALSE,
									&Data);	
		}
	return Data;
	
	}


	//
//	Description: 
//		This routine is exported for specific I/O Bus Domain(Host) and Device ID.
//		Host Address Mapping Range is 0x1025_0000~0x1027_FFFF.
	//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//		Called in PASSIVE_LEVEL
//
//	2010.12.10, added by Roger.
//
u4Byte
PlatformSdioCmd52Read4Byte(
	IN	PVOID		Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	u4Byte	Data = 0;
	RT_STATUS	status = RT_STATUS_SUCCESS;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return 0xffffffff;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	
	{		
		RT_ASSERT(FALSE, ("PlatformSdioCmd52Read4Byte() in PASSIVE_LEVEL is not allowed!!!\n"));		
	}
	else
	{
		status = PlatformSdioCmd52ReadWrite(
									sdiodevice, 
									DeviceID,
									FuncNum, 
									4, 
									(ULONG)offset, 
									FALSE,
									&Data);	
	}
	return Data;
	
}


//
//	Description: 
//		This routine is exported for specific I/O Bus Domain(Host) and Device ID.
//		Host Address Mapping Range is 0x1025_0000~0x1027_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//		PASSIVE_LEVEL
//
//	2010.12.10, added by Roger.
//
VOID
PlatformSdioCmd52Write1Byte(
	PVOID			Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset,
	IN	u1Byte		data
    )
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	RT_STATUS	status = RT_STATUS_SUCCESS;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	
	{		
		RT_ASSERT(FALSE, ("PlatformSdioCmd52Write1Byte() in PASSIVE_LEVEL is not allowed!!!\n"));		
	}
	else
	{
		status = PlatformSdioCmd52ReadWrite(
									sdiodevice, 
									DeviceID,
									FuncNum, 
									1, 
									(ULONG)offset, 
									TRUE,
									&data);
	}
}


//
//	Description: 
//		This routine is exported for specific I/O Bus Domain(Host) and Device ID.
//		Host Address Mapping Range is 0x1025_0000~0x1027_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//
//	2010.12.10, added by Roger.
//
VOID
PlatformSdioCmd52Write2Byte(
	PVOID			Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset,
	IN	u2Byte		data
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	RT_STATUS	status = RT_STATUS_SUCCESS;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	
	{ 
		RT_ASSERT(FALSE, ("PlatformSdioCmd52Write2Byte() in PASSIVE_LEVEL is not allowed!!!\n"));		
	}
	else
	{
		status = PlatformSdioCmd52ReadWrite(
									sdiodevice, 
									DeviceID,
									FuncNum, 
									2, 
									(ULONG)offset, 
									TRUE,
									&data);
	}
}

	
	//
//	Description: 
//		This routine is exported for specific I/O Bus Domain(Host) and Device ID.
//		Host Address Mapping Range is 0x1025_0000~0x1027_FFFF.
//
//	Assumption:
//		Proper I/O bus domain address should be converted before calling any 
//		IO_RW_DIRECT or IO_RW_EXTENDED Commands.
//		PASSIVE_LEVEL
	//
//	2010.12.10, added by Roger.
//
VOID
PlatformSdioCmd52Write4Byte(
	PVOID			Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset,
	IN	u4Byte		data
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	RT_STATUS	status = RT_STATUS_SUCCESS;

	if(RT_SDIO_CANNOT_IO(pDefaultAdapter))
		return;

	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	
	{		
		RT_ASSERT(FALSE, ("PlatformSdioCmd52Write4Byte() in PASSIVE_LEVEL is not allowed!!!\n"));		
	}
	else
	{
		status = PlatformSdioCmd52ReadWrite(
									sdiodevice, 
									DeviceID,
									FuncNum, 
									4, 
									(ULONG)offset, 
									TRUE,
									&data);
	}
}


RT_STATUS
PlatformAllocateSharedMemory(
	PVOID				Adapter,
	PSHARED_MEMORY		pSharedMemory,
	u4Byte				length
	)
{
	//RTL8187_TODO
	//NDIS_STATUS		ndisstatus;
	RT_STATUS		rtstatus = RT_STATUS_SUCCESS;

	rtstatus = PlatformAllocateMemory( Adapter, (PVOID*)&pSharedMemory->VirtualAddress, length);

	if(pSharedMemory->VirtualAddress==NULL)
		return RT_STATUS_FAILURE;
	
	pSharedMemory->Length=length;

	NdisZeroMemory(pSharedMemory->VirtualAddress,length);

	return RT_STATUS_SUCCESS;

}

VOID
PlatformFreeSharedMemory(
	PVOID				Adapter,
	PSHARED_MEMORY		pSharedMemory
	)
{
	if(pSharedMemory->VirtualAddress==NULL)
		return;

	PlatformFreeMemory(
		pSharedMemory->VirtualAddress, 
		pSharedMemory->Length);

	pSharedMemory->VirtualAddress=NULL;

}


RT_STATUS
PlatformAllocateAlignedSharedMemory(
	PVOID					Adapter,
	PALIGNED_SHARED_MEMORY	pAlignedSharedMemory,
	u4Byte						length
	)
{
	//
	// Note:	This function is platform independent.
	//		So you just need to copy this function to your platform.c
	//
	RT_STATUS	status;
	u4Byte		Offset;

	pAlignedSharedMemory->Length=length;
	length+=0x00;

	status=PlatformAllocateSharedMemory(
		Adapter,
		&pAlignedSharedMemory->OriginalSharedMemory,
		length);
	
	if(status==RT_STATUS_SUCCESS)
	{
		pAlignedSharedMemory->PhysicalAddressHigh=pAlignedSharedMemory->OriginalSharedMemory.PhysicalAddressHigh;
		pAlignedSharedMemory->PhysicalAddressLow = ((pAlignedSharedMemory->OriginalSharedMemory.PhysicalAddressLow+0xff) & 0xFFFFFF00);
		Offset = pAlignedSharedMemory->PhysicalAddressLow - pAlignedSharedMemory->OriginalSharedMemory.PhysicalAddressLow;

		pAlignedSharedMemory->VirtualAddress= pAlignedSharedMemory->OriginalSharedMemory.VirtualAddress+ Offset;
	}
	else
		pAlignedSharedMemory->VirtualAddress=NULL;
	
	return status;
}

VOID
PlatformFreeAlignedSharedMemory(
	PVOID					Adapter,
	PALIGNED_SHARED_MEMORY	pAlignedSharedMemory
	)
{
	//
	// Note:	This function is platform independent.
	//		So you just need to copy this function to your platform.c
	//
	if(pAlignedSharedMemory->VirtualAddress)
	{
		PlatformFreeSharedMemory(Adapter,&pAlignedSharedMemory->OriginalSharedMemory);

		pAlignedSharedMemory->VirtualAddress=NULL;
	}
}


VOID
PlatformIndicateMediaStatus(
	PVOID			Adapter,
	RT_MEDIA_STATUS	mstatus
	)
{
	PADAPTER			pAdapter = (PADAPTER)Adapter;
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	NDIS_LINK_STATE		LinkState;

	//
	// We DON'T indicate media status accroding to 
	// document Native 802.11 Wireless Lan,
	// section General Connection Operation Guidelines.
	// 2007.08.06, by shien chang.
	//

	N6_ASSIGN_OBJECT_HEADER(
		LinkState.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		NDIS_LINK_STATE_REVISION_1,
		sizeof(NDIS_LINK_STATE));
	
	if( mstatus == RT_MEDIA_CONNECT )
	{
		LinkState.MediaConnectState = MediaConnectStateConnected;
	}
	else
	{
//		LinkState.MediaConnectState = MediaConnectStateDisconnected;
		pAdapter->NdisSdioDev.bNdisStatusIndicated=TRUE;//for HCT test
		return;
	}
	LinkState.MediaDuplexState = MediaDuplexStateHalf;

	LinkState.RcvLinkSpeed = (MgntActQuery_RT_11N_USER_SHOW_RATES(pAdapter , pMgntInfo->bForcedShowRxRate, TRUE)/2)*1000000;	
	LinkState.XmitLinkSpeed = (MgntActQuery_RT_11N_USER_SHOW_RATES(pAdapter , pMgntInfo->bForcedShowRxRate, FALSE)/2)*1000000;	

	//RT_TRACE(COMP_INDIC, DBG_LOUD, ("PlatformIndicateMediaStatus  %d\n",LinkState.XmitLinkSpeed));
	
	LinkState.PauseFunctions = NdisPauseFunctionsUnsupported;
	//LinkState.AutoNegotiationFlags = NDIS_LINK_STATE_DUPLEX_AUTO_NEGOTIATED;
	LinkState.AutoNegotiationFlags = NDIS_LINK_STATE_RCV_LINK_SPEED_AUTO_NEGOTIATED ;
	
	N6IndicateStatus(
					pAdapter,
					NDIS_STATUS_LINK_STATE,
					&LinkState,
					sizeof(NDIS_LINK_STATE));


	pAdapter->NdisSdioDev.bNdisStatusIndicated=TRUE;

}


NTSTATUS
SdioAsynIORead(
	PRT_SDIO_DEVICE	device, 
	u2Byte			Count,
	u2Byte			Index
)
{
	PIO_STACK_LOCATION		pNextStack;    
	NTSTATUS				NtStatus;    
	PIRP						pIrp;    

	RT_ASSERT(FALSE, ("SdioAsynIORead()!!!\n"));

    return STATUS_UNSUCCESSFUL;
}


//
// Send an IRP for Asyn IO Write to SDIO host driver.
// 2010.12.30, by Roger.
//
NTSTATUS
IssueIrpForAsynSdioIOWrite(
	PRT_SDIO_DEVICE	device, 
	u2Byte			Count,
	u4Byte			Index,
	PVOID			pOutRegisterData
)
{
	PIRP		pIrp = NULL;  
	PMDL 	pmdl = NULL;	 
	PSDBUS_REQUEST_PACKET psdrp = NULL;	   	
	SD_RW_EXTENDED_ARGUMENT 	extendedArgument;
	PIO_STACK_LOCATION		pNextStack;			
	NTSTATUS			NtStatus = STATUS_SUCCESS;    
	
	const SDCMD_DESCRIPTOR WriteIoExtendedDesc = {
							SDCMD_IO_RW_EXTENDED,
                                                 SDCC_STANDARD,
                                                 SDTD_WRITE,
                                                 SDTT_SINGLE_BLOCK,
                                                 SDRT_5};

	NdisAcquireSpinLock( &(device->IrpSpinLock) );

	//
	// Retrieve corresponding SD Request Packet and MDL.
	//
	psdrp = device->pAsynIoWriteSdrp;
	pmdl = device->pAsynIoWriteMDL;

	RtlZeroMemory(psdrp, sizeof(SDBUS_REQUEST_PACKET));
	
	RT_ASSERT(device->bAsynIoWritePending == FALSE, ("IssueIrpForAsynIOWrite(): bAsynIoWritePending: %x !!!", device->bAsynIoWritePending));
	device->bAsynIoWritePending = TRUE;

	device->AsynIoWriteOffset = Index;
	device->AsynIoWriteByteCnt = Count;
	// Prefast warning ignore for false positive
#pragma warning( disable:6064 )
	RT_ASSERT(Count <= MAX_AWB_DATA_SIZE, ("IssueIrpForAsynIOWrite()>: Count: %d > MAX_AWB_DATA_SIZE!!!\n, Count"));
	PlatformMoveMemory((PVOID)(device->AsynIoWriteDataBuf), pOutRegisterData, Count);

	//
	// Allocate IRP
	//
	pIrp = IoAllocateIrp(device->NextDeviceStackSize, FALSE);
	if(pIrp == NULL) 
	{
		NdisReleaseSpinLock( &(device->IrpSpinLock) );
		return STATUS_INSUFFICIENT_RESOURCES;
		}


	//
	// Set up the argument and command descriptor.
	//
	psdrp->RequestFunction = SDRF_DEVICE_COMMAND;
	psdrp->Parameters.DeviceCommand.CmdDesc = WriteIoExtendedDesc;
	psdrp->Parameters.DeviceCommand.Length = Count;  	
	
	extendedArgument.u.AsULONG = 0;
	extendedArgument.u.bits.Count = Count;
	extendedArgument.u.bits.Address = Index;
	extendedArgument.u.bits.OpCode  = 1;
	extendedArgument.u.bits.Function = device->SdioFuncNum;
	extendedArgument.u.bits.WriteToDevice = TRUE;
	

	pNextStack = IoGetNextIrpStackLocation(pIrp);
	RT_ASSERT(pNextStack != NULL, ("IssueIrpForAsynIOWrite(): pNextStack should not be NULL!!!\n"));
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_SD_SUBMIT_REQUEST;
	pNextStack->Parameters.DeviceIoControl.Type3InputBuffer = psdrp;	 
	pNextStack->Parameters.DeviceIoControl.OutputBufferLength = Count;

	psdrp->Parameters.DeviceCommand.Argument = extendedArgument.u.AsULONG;
	psdrp->Parameters.DeviceCommand.Mdl = pmdl;

	device->pAsynIoWriteIrp = pIrp;

	NdisReleaseSpinLock( &(device->IrpSpinLock) );
	
	//
	// Sends an asynchronous Secure Digital (SD) request to the bus driver interface.
	//	
	NtStatus = SdBusSubmitRequestAsync (
							device->Sdbusinterface.Context,
							psdrp,
							pIrp,
							&SdioAsynIOWriteComplete,
							(PVOID)device);		

	if( NtStatus != STATUS_PENDING ) 
		{ 
	
		//
		// <Roger_TODO> We should handle other SD Bus error status here to prevent SD Bus halted issue.
		// 2010.12.30.
		//
		RT_TRACE(COMP_IO, DBG_SERIOUS, ("IssueIrpForAsynIOWrite(): SdBusSubmitRequestAsync failed!!! (0x%X)\n", NtStatus));		
		}		
	
	return NtStatus;
	}

#if RTL8723_SDIO_IO_THREAD_ENABLE

// 
//	Description: 
//		This routine just inserts the asynchronous IO Write Block(AWB) in busy queue, and then the AWB will be removed 
//	and perform corresponding IO operation in specific IO processing thread.
//
//	2011.06.23, created by Roger.
//
VOID
SdioAsynIOWriteEnqueue(
	PRT_SDIO_DEVICE	device, 
	u1Byte		DeviceID,
	UCHAR		FuncNum,	
	u2Byte			Count,
	u4Byte		Index,
	PVOID			pOutRegisterData
)
	{
	PRT_AWB 	pAwb = NULL;
	u4Byte		TargetAddr = Index;
	PADAPTER	pAdapter = device->pAdapter;
	NTSTATUS	NtStatus = STATUS_SUCCESS;    
	BOOLEAN		bMacPwrCtrlOn = FALSE;
	BOOLEAN		bIoWriteEnqueue = FALSE;

	
	if(RT_SDIO_CANNOT_IO(pAdapter))
		return;		

	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if(!bMacPwrCtrlOn)
	{
		RT_TRACE(COMP_IO, DBG_WARNING, ("SdioAsynIOWriteEnqueue(): power is off return!!\n"));
		return;
	        }
	
	PlatformAcquireSpinLock(pAdapter, RT_AWB_SPINLOCK);
	
	if(!RTIsListEmpty( &(device->AwbIdleQueue) )) 
	{
		pAwb = (PRT_AWB)RTRemoveHeadListWithCnt( &(device->AwbIdleQueue), &(device->NumIdleAwb));
	
		if(Count <= MAX_AWB_DATA_SIZE)
		{
			pAwb->Offset = TargetAddr;
			pAwb->ByteCnt = Count;
			pAwb->DeviceID = DeviceID;
			pAwb->FuncNum = FuncNum;
			PlatformMoveMemory((PVOID)(pAwb->DataBuf), pOutRegisterData, Count);
			RTInsertTailListWithCnt(&(device->AwbWaitQueue), &(pAwb->List), &(device->NumWaitAwb));
			bIoWriteEnqueue = TRUE;
			
			RT_TRACE(COMP_IO, DBG_TRACE, ("SdioAsynIOWriteEnqueue(): Offset(%#x), ByteCnt(%#x), NumIdleAwb(%d)\n", pAwb->Offset, pAwb->ByteCnt, device->NumIdleAwb));
			RT_PRINT_DATA(COMP_IO, DBG_TRACE, "SdioAsynIOWriteEnqueue(): Buffer:\n", 
				pAwb->DataBuf, pAwb->ByteCnt);
	        }	
	        else
		{
			RT_ASSERT(FALSE, ("SdioAsynIOWriteEnqueue(): Invalid data size: %d for AWB, so this AsynIoWrite will be discard!!!\n", Count));
			ReturnSdioAWB(device, pAwb);
		}

	        }
	else
	{
		RT_ASSERT(FALSE, ("SdioAsynIOWriteEnqueue(): No AWB in AwbIdleQueue NumIdleAwb(%d), so this AsynIoWrite will be discard!!!\n", 
			device->NumIdleAwb));
	}

	PlatformReleaseSpinLock(pAdapter, RT_AWB_SPINLOCK);

	if(bIoWriteEnqueue == TRUE)
	{
		// Releases the specified IO semaphore object for SDIO Register Access.
		PlatformReleaseSemaphore(&device->IOSemaphore);
	}
	}
#endif

//
//	Description: 
//		This routine performs asynchronous IO Write operation in DISPATCH_LEVLEL.
//
//	2011.01.17, created by Roger.
//
NTSTATUS
SdioAsynIOWrite(
	PRT_SDIO_DEVICE	device, 
	u1Byte		DeviceID,
	UCHAR		FuncNum,	
	u2Byte			Count,
	u4Byte		Index,
	PVOID			pOutRegisterData
)
{
	u4Byte		TargetAddr = Index;
	PADAPTER	pAdapter = device->pAdapter;
	NTSTATUS			NtStatus = STATUS_SUCCESS;    
	BOOLEAN		bMacPwrCtrlOn;
	u1Byte		FwPSState;


	if(RT_SDIO_CANNOT_IO(pAdapter))
		return NtStatus;		

	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	

	if(!bMacPwrCtrlOn)
	{
		RT_TRACE(COMP_POWER, DBG_TRACE, ("SdioAsynIOWrite(): power is off return!!\n"));
		return NtStatus;
	}

	if(pAdapter->bFWReady)
	{	
		pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_FW_PS_STATE, &FwPSState);
		if(IS_IN_LOW_POWER_STATE(pAdapter, FwPSState) && 
			!IS_SDIO_POWER_ON_IO_REG(DeviceID, Index))
	{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Return----- SdioAsynIOWrite(): %#X\n", Index));
			return NtStatus;
		}
	}

	NdisAcquireSpinLock( &(device->IrpSpinLock) );

	if( FuncNum == device->SdioFuncNum )
		pAdapter->HalFunc.HalSdioGetCmdAddressHandler( pAdapter, DeviceID, Index, &TargetAddr );

	if(device->nIrpPendingCnt == 0)
	{ // No IRP pending in USB host driver.

		device->nIrpPendingCnt++;
		NdisReleaseSpinLock( &(device->IrpSpinLock) );

		// Issue an IRP for AsynIo write.
		NtStatus = IssueIrpForAsynSdioIOWrite(device, Count, TargetAddr, pOutRegisterData);

		// <RJ_TODO_8187B> Handle the failed if IssueIrpForAsynIOWrite failed.
	}
	else if(device->nIrpPendingCnt == 1) 
	{ // There is one IRP pending in SDIO host driver. 
		PRT_AWB pAwb;

		NdisReleaseSpinLock( &(device->IrpSpinLock) );
		
		RT_TRACE(COMP_DBG, DBG_TRACE, ("***** AsynIoWrite should be inserted to queue *****\n"));

		PlatformAcquireSpinLock(pAdapter, RT_AWB_SPINLOCK);
		if(!RTIsListEmpty( &(device->AwbIdleQueue) )) 
		{
			pAwb = (PRT_AWB)RTRemoveHeadListWithCnt( &(device->AwbIdleQueue), &(device->NumIdleAwb));
		
			if(Count <= MAX_AWB_DATA_SIZE)
			{
				pAwb->Offset = TargetAddr;
				pAwb->ByteCnt = Count;
				PlatformMoveMemory((PVOID)(pAwb->DataBuf), pOutRegisterData, Count);
				RTInsertTailListWithCnt(&(device->AwbWaitQueue), &(pAwb->List), &(device->NumWaitAwb));
			}
			else
			{
				RT_ASSERT(FALSE, ("SdioAsynIOWrite(): Invalid data size: %d for AWB, so this AsynIoWrite will be discard!!!\n", Count));
				ReturnSdioAWB(device, pAwb);
			}
		}
		else
		{
			RT_ASSERT(FALSE, ("SdioAsynIOWrite(): No AWB in AwbIdleQueue NumIdleAwb(%d), so this AsynIoWrite will be discard!!!\n",
				device->NumIdleAwb));
		}
		PlatformReleaseSpinLock(pAdapter, RT_AWB_SPINLOCK);
	}
	else
	{ // Error condition.
		RT_ASSERT(FALSE, ("SdioAsynIOWrite(): device->nIrpPendingCnt: %d !!!\n", device->nIrpPendingCnt));
		PlatformReleaseSpinLock(pAdapter, RT_AWB_SPINLOCK);
	}


	RT_TRACE(COMP_DBG, DBG_TRACE, ("<<<<<<<<<<< SdioAsynIOWrite(): %x\n", TargetAddr));

	return NtStatus;
}


//
// Post-processing for Asyn IO Write.
// 2010.12.29, by Roger.
//
VOID
SdioIOComplete(
	PRT_SDIO_DEVICE	device
)
{
	PADAPTER pAdapter = (PADAPTER)(device->pAdapter);
	BOOLEAN		bMacPwrCtrlOn;

	//RT_TRACE(COMP_DBG, DBG_TRACE, ("---> SdioIOComplete()\n"));

	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_APFM_ON_MAC, (pu1Byte)(&bMacPwrCtrlOn));	


	NdisAcquireSpinLock( &(device->IrpSpinLock) );

	device->nIrpPendingCnt--;

	if(device->nIrpPendingCnt == 0)
	{ // No IRP pending in SDIO host driver.
	
		if((pAdapter->bDriverStopped == FALSE) && bMacPwrCtrlOn)
		{
#if RTL8723_SDIO_IO_THREAD_ENABLE
			if(0)
#else
			if( !RTIsListEmpty( &(device->AwbWaitQueue) ) )
#endif				
			{ // Handle the request in AsynIoWriteWaitQ.
				PRT_AWB pAwb;
				NTSTATUS NtStatus;    

				device->nIrpPendingCnt++;
				NdisReleaseSpinLock( &(device->IrpSpinLock) );

				PlatformAcquireSpinLock(pAdapter, RT_AWB_SPINLOCK);
				// Issue an IRP for AsynIo write.
				pAwb = (PRT_AWB)RTRemoveHeadListWithCnt( &(device->AwbWaitQueue), &(device->NumWaitAwb));
				PlatformReleaseSpinLock(pAdapter, RT_AWB_SPINLOCK);
				RT_ASSERT(pAwb != NULL, ("SdioIOComplete(): pAwb should not be NULL!!! Race Condition?\n"));
				NtStatus = IssueIrpForAsynSdioIOWrite(device, pAwb->ByteCnt, pAwb->Offset, (PVOID)pAwb->DataBuf);

				PlatformAcquireSpinLock(pAdapter, RT_AWB_SPINLOCK);
				// Return the AWB since we had submitted the asyn IO write operation to USBD.
				ReturnSdioAWB(device, pAwb);
				PlatformReleaseSpinLock(pAdapter, RT_AWB_SPINLOCK);
			}
			// SyncIo Method 2.  
			else if(device->SyncIoWaitingCount>0)
			{ // If there is workitem waiting.
				RT_TRACE(COMP_DBG, DBG_TRACE,("SdioIOComplete(): there is workitem is waiting...\n"));
				NdisReleaseSpinLock( &(device->IrpSpinLock) );
				KeSetEvent( &(device->SyncIoEvent), 0, FALSE);
			}
			else
			{
				NdisReleaseSpinLock( &(device->IrpSpinLock) );
			}
		}
		else
		{ // Driver is going to unload.
			NdisReleaseSpinLock( &(device->IrpSpinLock) );

#if RTL8723_SDIO_IO_THREAD_ENABLE == 0
			while( !RTIsListEmpty(&(device->AwbWaitQueue)) && !bMacPwrCtrlOn)
			{
				PRT_AWB pAwb;
				
				RT_TRACE(COMP_INIT|COMP_IO|COMP_SEND, DBG_WARNING, ("SdioIOComplete(): power is off, and return AWB!!\n"));				
				PlatformAcquireSpinLock(pAdapter, RT_AWB_SPINLOCK);
				// Issue an IRP for AsynIo write.
				pAwb = (PRT_AWB)RTRemoveHeadListWithCnt( &(device->AwbWaitQueue), &(device->NumWaitAwb));
				// Return the AWB immediately
				ReturnSdioAWB(device, pAwb);
				PlatformReleaseSpinLock(pAdapter, RT_AWB_SPINLOCK);				
			}
#endif
			if(device->SyncIoWaitingCount>0)
			{ // If there is workitem waiting. 2005.01.25, by rcnjko.
				RT_TRACE(COMP_DBG, DBG_LOUD,("SdioIOComplete(): there is workitem is waiting...\n"));
				KeSetEvent( &(device->SyncIoEvent), 0, FALSE);
			}
		}
	}
	else
	{ // Error condition.
		RT_ASSERT(FALSE, ("SdioIOComplete(): device->nIrpPendingCnt: %d !!!\n", device->nIrpPendingCnt));
		NdisReleaseSpinLock( &(device->IrpSpinLock) );
	}

			//
	// Signal the AllSdioCmdReturnedEvent if all SDIO CMDs are returned.
	// 2011.04.01.
			//
	if( RT_SDIO_GET_CMD_REF(device) == 0 )
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("SdioIOComplete(): Signal the AllSdioCmdReturnedEvent\n"));
		NdisSetEvent(&device->AllSdioCmdReturnedEvent);
		}

	//RT_TRACE(COMP_DBG, DBG_TRACE, ("<--- SdioIOComplete()\n"));
	}


// 
// Callback function of when an Asyn IO Write IRP completed.
// 2010.12.30, by Roger.
//
NTSTATUS
SdioAsynIOWriteComplete(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			pIrp,
	PVOID			Context
)
{
	PRT_SDIO_DEVICE	device;    
	PADAPTER		pAdapter;
	
	device = (PRT_SDIO_DEVICE)Context;
	pAdapter = (PADAPTER)(device->pAdapter);

	NdisAcquireSpinLock( &(device->IrpSpinLock) );
	device->bAsynIoWritePending = FALSE;
		NdisReleaseSpinLock( &(device->IrpSpinLock) );

	if(pAdapter->bDriverStopped)
	{
		NdisSetEvent(&device->AllAsynIoIrpReturnedEvent);
	}

	switch(pIrp->IoStatus.Status)	
	{
		case STATUS_SUCCESS:	
			RT_TRACE(COMP_IO, DBG_LOUD, ("SdioAsynIOWriteComplete(): STATUS_SUCCESS\n"));
			break;

		case STATUS_TIMEOUT:	
			if(device->bRegSurpriseRemovedEnable)
				pAdapter->bSurpriseRemoved = TRUE;

			RT_TRACE(COMP_IO, DBG_LOUD, ("SdioAsynIOWriteComplete(): STATUS_TIMEOUT\n"));
			break;

		case STATUS_PENDING:	
			RT_TRACE(COMP_IO, DBG_LOUD, ("SdioAsynIOWriteComplete(): STATUS_PENDING\n"));
			break;

		case STATUS_UNSUCCESSFUL:	
			if(device->bRegSurpriseRemovedEnable)
				pAdapter->bSurpriseRemoved = TRUE;

			RT_TRACE(COMP_IO, DBG_LOUD, ("SdioAsynIOWriteComplete(): STATUS_UNSUCCESSFUL\n"));
			break;

		default:	
			RT_TRACE(COMP_IO, DBG_LOUD, ("SdioAsynIOWriteComplete(): Unknown: %x\n", pIrp->IoStatus.Status));
			break;		
	}

	IoFreeIrp(pIrp);

	SdioIOComplete(device);
	
	return STATUS_MORE_PROCESSING_REQUIRED;
}


//
// New IO:
// Cancel the pending Asyn IO Write in SDIO host driver.
// 2011.01.06, by Roger.
//
VOID
RTsdioCancelAsynIoPendingIrp(
	IN	PVOID		Adapter
)
{
	PRT_SDIO_DEVICE		device = GET_RT_SDIO_DEVICE((PADAPTER)Adapter);
	u1Byte				i;

	if(device->bAsynIoWritePending)
	{ 
		// Cancel the IRP for AsynIO write request.
		IoCancelIrp(device->pAsynIoWriteIrp);	

		// Wait until it completed by USB host driver.
		NdisWaitEvent( &(device->AllAsynIoIrpReturnedEvent), 2000);

		// Reset the event. 
		NdisResetEvent(&(device->AllAsynIoIrpReturnedEvent));
	}
}



BOOLEAN
PrepareSdioAWBs(
	PRT_SDIO_DEVICE	device
	)
{
	RT_STATUS status = RT_STATUS_FAILURE;
	u4Byte i;
	PADAPTER Adapter = (PADAPTER)device->pAdapter;
	PRT_AWB pAwb;
	device->pAsynIoWriteMDL = NULL;
	device->pAsynIoWriteSdrp = NULL;

	do
	{
		device->AwbBufferSize = device->NumAwb * sizeof(RT_AWB);
		status = PlatformAllocateMemory(Adapter, &(device->AwbBuffer), device->AwbBufferSize);
		if(status != RT_STATUS_SUCCESS)
			goto Error;

		PlatformZeroMemory(device->AwbBuffer, device->AwbBufferSize);

		pAwb = (PRT_AWB)device->AwbBuffer;
		PlatformAcquireSpinLock(Adapter, RT_AWB_SPINLOCK);
		for(i = 0; i < device->NumAwb; i++)
		{
			ReturnSdioAWB(device, &(pAwb[i]));
		}
		PlatformReleaseSpinLock(Adapter, RT_AWB_SPINLOCK);
	}while(FALSE);


	//
	// Allocate MDL for AsynIO Write buffer
	//
	device->pAsynIoWriteMDL = IoAllocateMdl((pu1Byte)device->AsynIoWriteDataBuf, MAX_AWB_DATA_SIZE , FALSE, FALSE, NULL);	
	if(device->pAsynIoWriteMDL == NULL)
		goto Error;
	
	MmBuildMdlForNonPagedPool(device->pAsynIoWriteMDL);

	//
	// Allocate SD Request Packet
	//
	device->pAsynIoWriteSdrp = (PSDBUS_REQUEST_PACKET)ExAllocatePoolWithTag(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET), '3278');
  	if(device->pAsynIoWriteSdrp == NULL)	
  		goto Error;
	
	return TRUE;

Error:
	if( device->pAsynIoWriteMDL )
		IoFreeMdl( device->pAsynIoWriteMDL );

	if( device->pAsynIoWriteSdrp )
		ExFreePoolWithTag( device->pAsynIoWriteSdrp, '3278' );

	return FALSE;
}


BOOLEAN
FreeSdioAWBs(
	PRT_SDIO_DEVICE	device, 
	BOOLEAN			bReset
	)
{
	PRT_AWB pAwb;
	PADAPTER Adapter = (PADAPTER)device->pAdapter;

	PlatformAcquireSpinLock(Adapter, RT_AWB_SPINLOCK);
	while( !RTIsListEmpty(&(device->AwbWaitQueue))  ) 
	{
		pAwb = (PRT_AWB)RTRemoveHeadListWithCnt( &(device->AwbWaitQueue), &(device->NumWaitAwb));

		ReturnSdioAWB(device, pAwb);
	}
	PlatformReleaseSpinLock(Adapter, RT_AWB_SPINLOCK);

	if(!bReset)
	{
		PlatformFreeMemory(device->AwbBuffer, device->AwbBufferSize);
	}

	if( device->pAsynIoWriteMDL )
		IoFreeMdl( device->pAsynIoWriteMDL );

	if( device->pAsynIoWriteSdrp )
		ExFreePool( device->pAsynIoWriteSdrp );

	device->pAsynIoWriteMDL = NULL;
	device->pAsynIoWriteSdrp = NULL;
	
	return TRUE;
}


VOID
ReturnSdioAWB(
	PRT_SDIO_DEVICE	device,
	PRT_AWB			pAwb
	)
{
	RTInsertTailListWithCnt(&(device->AwbIdleQueue), &(pAwb->List), &(device->NumIdleAwb));
}

//
//	Description:
//		Determine if we can send packet to specified QueueID.
//		It return TRUE if then IRP pending in corresponding pipe > 1.
//		Note that, this is platform dependent, the following implementation 
//		is  for WDM.
//
BOOLEAN
PlatformIsTxQueueAvailable(
	IN	PADAPTER	Adapter,
	IN	u1Byte		QueueID,
	IN	u2Byte		BufferCount)
{


	PADAPTER			pDefaultAdapter = GetDefaultAdapter(((PADAPTER)Adapter));
	PRT_SDIO_DEVICE		device = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	u1Byte				IdxQueue;
	PRT_SDIO_TX_QUEUE	pTxQueue;
	int					nIrpPendingCount = 0;

	IdxQueue = MapTxQueueToOutPipe(pDefaultAdapter, QueueID);
	pTxQueue = &(device->RtTxQueue[IdxQueue]);
	nIrpPendingCount = pTxQueue->IrpPendingCount;

	RT_TRACE(COMP_SEND, DBG_TRACE, ("PlatformIsTxQueueAvailable(): QueueID(%d) IrpPendingCount(%d)\n", IdxQueue, nIrpPendingCount));

	if(nIrpPendingCount == 1 && pTxQueue->bStalled == FALSE)
	{		
		return TRUE;
	}
	else
	{
		if(nIrpPendingCount >= MAX_NUM_REQUEST_PER_TX_QUEUE)
		{
			RT_TRACE(COMP_SEND, DBG_SERIOUS, ("PlatformIsTxQueueAvailable(): QueueID(%d) IrpPendingCount(%d) >= %d !!!\n", 
				IdxQueue, nIrpPendingCount, MAX_NUM_REQUEST_PER_TX_QUEUE));
			return FALSE;
		}
	}

	return TRUE;
}



#if 1
extern	PCHAR PlatformSystemPowerString(
	SYSTEM_POWER_STATE Type
    )
/*++

New Routine Description:
    PlatformSystemPowerString converts the system power state code of a power IRP to a
    text string that is more helpful when tracing the execution of power IRPs.

Parameters Description:
    Type
    Type specifies the system power state code of a power IRP.

Return Value Description:
    PlatformDevicePowerString returns a pointer to a string that represents the
    text description of the incoming system power state code.

--*/
{
	switch (Type)
	{
		case PowerSystemUnspecified:
		    	return "PowerSystemUnspecified";
		case PowerSystemWorking:
		    	return "PowerSystemWorking";
		case PowerSystemSleeping1:
		    	return "PowerSystemSleeping1";
		case PowerSystemSleeping2:
		    	return "PowerSystemSleeping2";
		case PowerSystemSleeping3:
		    	return "PowerSystemSleeping3";
		case PowerSystemHibernate:
		    	return "PowerSystemHibernate";
		case PowerSystemShutdown:
		    	return "PowerSystemShutdown";
		case PowerSystemMaximum:
		    	return "PowerSystemMaximum";
		default:
		    	return "UnKnown System Power State";
	}
 }

 
extern PCHAR PlatformDevicePowerString(
    	DEVICE_POWER_STATE Type
    )
/*++

New Routine Description:
    PlatformDevicePowerString converts the device power state code of a power IRP to a
    text string that is more helpful when tracing the execution of power IRPs.

Parameters Description:
    Type
    Type specifies the device power state code of a power IRP.

Return Value Description:
    PlatformDevicePowerString returns a pointer to a string that represents the
    text description of the incoming device power state code.

--*/
{
	switch (Type)
	{
		case PowerDeviceUnspecified:
		    	return "PowerDeviceUnspecified";
		case PowerDeviceD0:
		    	return "PowerDeviceD0";
		case PowerDeviceD1:
		    	return "PowerDeviceD1";
		case PowerDeviceD2:
		    	return "PowerDeviceD2";
		case PowerDeviceD3:
		    	return "PowerDeviceD3";
		case PowerDeviceMaximum:
		    	return "PowerDeviceMaximum";
		default:
		    	return "UnKnown Device Power State";
	}
}
#endif


VOID
PlatformSdioEnableTxQueues(
	IN	PADAPTER				Adapter
	)
{
	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(Adapter);
	int i;

	if( Adapter->bSurpriseRemoved )
		return;
	
	for ( i = 0; i < pDevice->RtNumTxQueue; i++ )
	{
		N6SdioStartTxQueue(Adapter, i);
	}		
}

VOID
PlatformSdioDisableTxQueues(
	IN	PADAPTER				Adapter
	)
{
	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(Adapter);
	int i;

	// Cancel Pending out IRPs.
	for (i = 0; i < pDevice->RtNumTxQueue; i++)
	{
		N6SdioStopTxQueue(Adapter, i);
	}
}


//
//	Description:
// 		Wait for all SD Request packets are completed.
// 	Added by Roger, 2010.09.30.
//
VOID
PlatformSdioWaitAllSDReqComplete(
	IN	PADAPTER				Adapter
	)
{

	PRT_SDIO_DEVICE pDevice = GET_RT_SDIO_DEVICE(Adapter);
	u1Byte	ChkCnt = 0;		
	
	while(TRUE)
	{			
		NdisAcquireSpinLock( &(pDevice->IrpSpinLock) );		
		if(pDevice->nIrpPendingCnt >0)
		{			
			RT_TRACE(COMP_IO, DBG_TRACE, ("PlatformSdioWaitAllSDReqComplete(): wait for all nIrpPendingCnt returned (%d)\n", 
				pDevice->nIrpPendingCnt));
			
			NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );
			PlatformStallExecution(100); // Wait for all Rx operation is down.	
			ChkCnt++;
			if ( ChkCnt>= 1000)
			{
				RT_TRACE(COMP_IO, DBG_TRACE, ("PlatformSdioWaitAllSDReqComplete(): Wait too long break!!\n"));
				break;
			}	
		}
		else		
		{
			NdisReleaseSpinLock( &(pDevice->IrpSpinLock) );
			break;
		}
	}
}	


VOID
PlatformEnableNetworkMonitorMode(
	IN	PADAPTER	Adapter
	)
{	
	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;
	pNdisCommon->dot11AutoConfigEnabled = FALSE;	// useless before ndis6	
}

VOID
PlatformDisableNetworkMonitorMode(
	IN	PADAPTER	Adapter
	)
{
	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;
	pNdisCommon->dot11AutoConfigEnabled = TRUE;	// useless before ndis6	
	
}

VOID
PlatformSetFwPsClkOffEvent(
	IN	PADAPTER	Adapter,
	IN	u1Byte		SetEvent
)
{
	PRT_SDIO_DEVICE sdiodevice = GET_RT_SDIO_DEVICE(Adapter);
	
	if(SetEvent)
		NdisSetEvent(&sdiodevice->FwPsClockOffEvent);
	else
		NdisResetEvent(&sdiodevice->FwPsClockOffEvent);
}

