/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	N6C_PlatformDef.c
	
Abstract:
	Common implemetation of function defined in PlatformDef.h 
	for NDIS6 PCI and USB. 
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2007-03-29 Rcnjko            Create.
	2007-05-18 Rcnjko            Move common implementation from USB\ and PCI\ to here.
	
--*/

#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "N6C_PlatformDef.tmh"
#endif

RT_DRIVER_CONTEXT	GlobalRtDriverContext = {0};

u8Byte	RTL_AllocateMemory_count = 0;
u8Byte	RTL_AllocateMemory_Len = 0;
u8Byte	RTL_FreeMemory_count = 0;
u8Byte	RTL_FreeMemory_Len = 0;

//=============================================================================
//	Prototype of protected function.
//=============================================================================
VOID
ndis6TimerCallback(
	IN	PVOID		SystemSpecific1,
	IN	PVOID		FunctionContext,
	IN	PVOID		SystemSpecific2,
	IN	PVOID		SystemSpecific3
	);

VOID
Ndis6WorkItemCallback(
	IN	PVOID				pWrapperContext,
	IN	NDIS_HANDLE			Handle
	);

VOID
Ndis6LeaveCallbackOfWorkItem(
	IN	PRT_WORK_ITEM	pRtWorkItem
	);

//=============================================================================
//	End of Prototype of protected function.
//=============================================================================

//=============================================================================
// Private	function.
//============================================================================

//
// Description:
//	The callback routine when NdisSetTimer is executed.
//	This routine has various settings to prevent race condition of executing the user specified timer routine.
// Sync from 818xB. By Bruce, 2008-03-11.
//
VOID
ndis6TimerCallback(
	IN	PVOID		SystemSpecific1,
	IN	PVOID		FunctionContext,
	IN	PVOID		SystemSpecific2,
	IN	PVOID		SystemSpecific3
	)
{
	PRT_TIMER			pTimer = (PRT_TIMER)FunctionContext;
	PADAPTER			Adapter=(PADAPTER)pTimer->Adapter;
	PPORT_COMMON_INFO	pPortCommonInfo = Adapter->pPortCommonInfo;
	BOOLEAN 			bTimerCanceled = FALSE;

	RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("=====>Ndis6TimerCallback(%s), status(%#x), ActiveTimerCallbackCount(%d)\n", pTimer->szID, pTimer->Status, pPortCommonInfo->ActiveTimerCallbackCount));
		
	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	
	if(pTimer->Status & (RT_TIMER_STATUS_RELEASED))
	{
		RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("ndis6TimerCallback(): TimerName: %s: This timer has been released! \n", pTimer->szID));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock)); 		
		return ;
	}
	//
	// Set RT_TIMER_STATUS_FIRED.
	//
	pTimer->Status |= RT_TIMER_STATUS_FIRED;

	//
	// Clear RT_TIMER_STATUS_SET flag before pTimer->CallBackFunc(pTimer), 
	// because we might schedule the same timer again there.
	//
	pTimer->Status &= ( ~(RT_TIMER_STATUS_SET | RT_TIMER_STATUS_CANCEL_NG) );

	pTimer->TimerStallSlotCount = 0;// For timer stall checking mechanism, added by Roger, 2007.05.16.


	//
	// Execute callback function.
	//
	if(!Adapter->bDriverStopped)
	{
		pPortCommonInfo->ActiveTimerCallbackCount++;
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));
		
		pTimer->CallBackFunc(pTimer);
		
		NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
		pPortCommonInfo->ActiveTimerCallbackCount--;
	}

	
	//
	// Clear RT_TIMER_STATUS_FIRED	
	//
	pTimer->Status &= (~RT_TIMER_STATUS_FIRED);
	
	//
	// Check if driver is waiting us to unload.
	//	
	if(pPortCommonInfo->ActiveTimerCallbackCount == 0)
		NdisSetEvent(&(pPortCommonInfo->AllTimerCompletedEvent));

	if(pTimer->Status & RT_TIMER_STATUS_PERIODIC)
	{
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));
		PlatformSetPeriodicTimer(pTimer->Adapter, pTimer, pTimer->msDelay);
	}
	else
	{
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));
	}

	RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("<=====Ndis6TimerCallback(%s), status(%#x), ActiveTimerCallbackCount(%d)\n", pTimer->szID, pTimer->Status, pPortCommonInfo->ActiveTimerCallbackCount));
}

//
// Description:
//	The callback routine when NdisScheduleWorkitem is executed.
// Sync from 818xB, by Bruce, 2008-12-01.
//
VOID
Ndis6WorkItemCallback(
	IN	PVOID				pWrapperContext,
	IN	NDIS_HANDLE			Handle
	)
{
	PRT_WORK_ITEM	pRtWorkItem = (PRT_WORK_ITEM)pWrapperContext;

	pRtWorkItem->CallbackFunc(pRtWorkItem->pContext);

	Ndis6LeaveCallbackOfWorkItem(pRtWorkItem);
}

//
// Description:
//	Post processing of before leaving callback function of a work time.
// <RJ_TODO> Maybe we can handle queued requests of this workitem here.
// 2006.09.28, by shien chang.
//
VOID
Ndis6LeaveCallbackOfWorkItem(
	IN	PRT_WORK_ITEM	pRtWorkItem
	)
{
	PPLATFORM_EXT	pPlatformExt = pRtWorkItem->pPlatformExt;

	if( pPlatformExt == NULL )
		return ;
	
	NdisAcquireSpinLock( &(pPlatformExt->Lock) );

	pRtWorkItem->RefCount--;

	if(pRtWorkItem->RefCount == 0)
		NdisSetEvent( &(pPlatformExt->Event) );
	
	NdisReleaseSpinLock( &(pPlatformExt->Lock) );
}


//=============================================================================
// End of Private  function.
//============================================================================


//=============================================================================
// Public  function.
//============================================================================



//=============================================================================
// Timer  function.
//============================================================================

//
// Description:
//	Initialize the timer and associate the callback routine to thsi timer.
// Sync from 818xB. By Bruce, 2008-12-01.
//
VOID
PlatformInitializeTimer(
	IN	PVOID				Adapter,
	IN	PRT_TIMER			pTimer,
	IN	RT_TIMER_CALL_BACK	CallBackFunc, 
	IN	PVOID				pContext,
	IN	const char*			szID
	)
{
	PADAPTER			pAdapter = (PADAPTER)Adapter;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	PRT_TIMER_HANDLE	pHandle;
	PPORT_COMMON_INFO 	pPortCommonInfo = pAdapter->pPortCommonInfo;
	
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;

	//
	// 060120, rcnjko: 
	// NdisMInitializeTimer() is noly allowed in context of PASSIVE level, 
	// otherwise, it might cause OS deadlock. 2006.01.20, by rcnjko.
	//
	// 061024, rcnjko: 
	// Set RT_TIMER_STATUS_INITIALIZED before NdisMInitializeTimer() to 
	// prevent race condition.
	//
	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	pTimer->Status &= (~RT_TIMER_STATUS_RELEASED);
	if( !(pTimer->Status & RT_TIMER_STATUS_INITIALIZED) )
	{
		pHandle = &(pTimer->Handle);
		
		pTimer->Adapter = pAdapter;
		pTimer->CallBackFunc = CallBackFunc;
		pTimer->Status = RT_TIMER_STATUS_INITIALIZED;
		pTimer->msDelay = 0;
		pTimer->Context = pContext;
		pTimer->TimerStallSlotCount = 0;

		PlatformZeroMemory(pTimer->szID, 36);
		if(szID != NULL)
		{
			ASCII_STR_COPY(pTimer->szID, szID, 36);
		}

		N6_ASSIGN_OBJECT_HEADER(
				pHandle->TimerCharacteristics.Header,
				NDIS_OBJECT_TYPE_TIMER_CHARACTERISTICS,
				NDIS_TIMER_CHARACTERISTICS_REVISION_1,
				NDIS_SIZEOF_TIMER_CHARACTERISTICS_REVISION_1
			);

		// This should use ASCII code for tagging -------------
		pHandle->TimerCharacteristics.AllocationTag = '2918';
		pHandle->TimerCharacteristics.TimerFunction = ndis6TimerCallback;
		pHandle->TimerCharacteristics.FunctionContext = pTimer;
		
		
		ndisStatus = NdisAllocateTimerObject(
				pNdisCommon->hNdisAdapter,
				&pHandle->TimerCharacteristics,
				&pHandle->NdisTimerHandle
			);

		if(ndisStatus == NDIS_STATUS_SUCCESS)
		{
			DebugResourceTimerCreateTimerMirror(
					pAdapter, 
					pPortCommonInfo->uNumberOfAllocatedTimers, 
					pTimer
				);
			
			pPortCommonInfo->uNumberOfAllocatedTimers++;

			N6CTimerResourceInsert(pAdapter, pTimer); // This timer has been inserted for further handling
		}	
		else
		{
			pTimer->Status |= RT_TIMER_STATUS_RELEASED;
		}
	}
	else
	{
		RT_ASSERT(pTimer->Status&RT_TIMER_STATUS_INITIALIZED,
			("Timer %s have been assert!!!!\n",pTimer->szID));
	}
	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));
}


BOOLEAN
PlatformSetPeriodicTimer(
	PVOID				Adapter,
	PRT_TIMER			pTimer,
	u4Byte				msDelay
	)
{
	PADAPTER				pAdapter = (PADAPTER)Adapter;
	PPORT_COMMON_INFO		pPortCommonInfo = pAdapter->pPortCommonInfo;	
	LARGE_INTEGER           fireTime;

	RT_TRACE(COMP_SYSTEM, DBG_TRACE, ("--->PlatformSetTimer() msDelay(%d)\n",msDelay));

	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	
	if(pTimer->Status & RT_TIMER_STATUS_RELEASED)
	{
		RT_TRACE(COMP_SYSTEM, DBG_WARNING, ("PlatformSetTimer(): timer(%s) is alreadyreleased!!!\n", pTimer->szID));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	if(!(pTimer->Status & RT_TIMER_STATUS_INITIALIZED))
	{
		RT_ASSERT(FALSE, ("PlatformSetTimer(): timer(%s) is not yet initialized!!! Status 0x%x\n", pTimer->szID, pTimer->Status));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	if(RT_DRIVER_HALT(pAdapter))
	{
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	if(pTimer->Status & RT_TIMER_STATUS_SUSPENDED)
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("PlatformSetPeriodicTimer(): timer(%s) is paused!!!\n", pTimer->szID));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}
/*
	//fix the bug that outstandingcount cannot be zero caused by cancel timer failure, vivi, 20100526
	//We encounter a condition on 92du like this(BlinkTimer):                       
	//set timer -->cancel timer( but failed)-->set timer-->callback function-->set timer-->callback function;
	//In this flow, the value of outstandingcount will be: 1-->1-->2-->1-->2-->1, 
	//the value can not be zero finally, then driver will wait for outstandingcount to be zero, and then timerout
	//now we fix this issue by this method, when RT_TIMER_STATUS_CANCEL_NG has been set, we can not settimer,
	//until callback function has been executed
	if(pTimer->Status&RT_TIMER_STATUS_CANCEL_NG)
		return FALSE;
*/
	//
	// 061024, rcnjko: 
	// We shall schedule the timer before setting RT_TIMER_STATUS_SET 
	// to make sure this flag is set only iff corresponding timer object 
	// had been  scheduled (i.e. placed in timer queued) and 
	// is not yet fired (i.e. execute its callback function). 
	//
	// 2007.04.30, Roger:
	// In Win98Me, Sometimes we could not set timer successful due to system out resource,
	// but we have no idea about that, i.e, NdisMSetTimer() success or not. So we should monitor some 
	// timer's resolution time to avoid this problem.
	//

	pTimer->msDelay = msDelay;
	pTimer->TimerStallSlotCount = 0;

	// This timer is counted by the previous timer or not.
	if(!(pTimer->Status & RT_TIMER_STATUS_SET))
	{
		pTimer->Status |= RT_TIMER_STATUS_PERIODIC | RT_TIMER_STATUS_SET;
	}

	RT_TRACE(COMP_SYSTEM, DBG_TRACE, ("PlatformSetTimer(%s): Status(%#x)\n",
		pTimer->szID, pTimer->Status
		));

	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		

	//
	// We schedule timer after timer status are configured 
	// to prevent callback routine happen before PlatformSetTimer() done.
	//

	fireTime.QuadPart = Int32x32To64((LONG)msDelay, -10000);
	return NdisSetTimerObject(pTimer->Handle.NdisTimerHandle, fireTime, 0,NULL);
}


//
// Description:
//	Schedule the timer by the following reles.
//	(1) If the previous timer had been scheduled and is not fired yet: 
//	    NdisSetTimer() cancel the previous one and schedule the current input timer according to the new 
//	    timer interval.
//	(2) If the previous timer had been fired and the callback routine is not complete yet:
//	    The input timer will be inserted into the system queue. After the previous callback routine is complete, then
//	    (2.1) the system schedule the new timer if the msDelay of the new timer had been expired; or
//	    (2.2) the system wait until the specified interval is expired.
// Note:
//	According to the rules described above, the timer can set twice at the same at most, one is running in 
//	routine and another is in the queue. So we should maintain the number of timers carefully.
// By Bruce, 2008-12-01.
//
BOOLEAN
PlatformSetTimer(
	IN	PVOID				Adapter,
	IN	PRT_TIMER			pTimer,
	IN	u4Byte				msDelay
	)
{
	PADAPTER			pAdapter = (PADAPTER)Adapter;
	PPORT_COMMON_INFO	pPortCommonInfo = pAdapter->pPortCommonInfo;
	BOOLEAN bIsQueued = FALSE;
	LARGE_INTEGER               fireTime;

	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	
	RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("=====>PlatformSetTimer(%s), status(%#x), msDelay(%d)\n", pTimer->szID, pTimer->Status, msDelay));

	if(pTimer->Status & RT_TIMER_STATUS_RELEASED)
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("PlatformSetTimer(): timer(%s) is alreadyreleased!!!\n", pTimer->szID));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	if(!(pTimer->Status & RT_TIMER_STATUS_INITIALIZED))
	{
		RT_ASSERT(FALSE, ("PlatformSetTimer(): timer(%s) is not yet initialized!!! Status 0x%x\n", pTimer->szID, pTimer->Status));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	if(pTimer->Status & RT_TIMER_STATUS_SUSPENDED)
	{
		RT_TRACE(COMP_INIT, DBG_WARNING, ("PlatformSetTimer(): timer(%s) is paused!!!\n", pTimer->szID));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	if(RT_DRIVER_HALT(pAdapter) && ! HAS_REQUEST_TO_HANDLE(pAdapter))
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("PlatformSetTimer(): return because bDriverStopped=%d or bSurpriseRemoved=%d\n", pAdapter->bDriverStopped, pAdapter->bSurpriseRemoved));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	//
	// 061024, rcnjko: 
	// We shall schedule the timer before setting RT_TIMER_STATUS_SET 
	// to make sure this flag is set only iff corresponding timer object 
	// had been  scheduled (i.e. placed in timer queued) and 
	// is not yet fired (i.e. execute its callback function). 
	//
	// 2007.04.30, Roger:
	// In Win98Me, Sometimes we could not set timer successful due to system out resource,
	// but we have no idea about that, i.e, NdisMSetTimer() success or not. So we should monitor some 
	// timer's resolution time to avoid this problem.
	//

	pTimer->msDelay = msDelay;
	pTimer->TimerStallSlotCount = 0;

	// This timer is counted by the previous timer or not.
	if(!(pTimer->Status & RT_TIMER_STATUS_SET))
	{
		pTimer->Status |= RT_TIMER_STATUS_SET;
	}

	RT_TRACE(COMP_SYSTEM, DBG_TRACE, ("PlatformSetTimer(%s): Status(%#x)\n",
		pTimer->szID,
		pTimer->Status));

	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		

	//
	// We schedule timer after timer status are configured 
	// to prevent callback routine happen before PlatformSetTimer() done.
	//
	fireTime.QuadPart = Int32x32To64((LONG)msDelay, -10000);
	bIsQueued = NdisSetTimerObject(pTimer->Handle.NdisTimerHandle, fireTime, 0,NULL);

	RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("<=====PlatformSetTimer(%s), status(%#x), bIsQueued(%d)\n", pTimer->szID, pTimer->Status, bIsQueued));
	return TRUE;
}

//
// Description:
//	Cancel the input timer.
// Note:
//	This function only cancel the immediately preceding call to NdisMSetTimer if the interval 
//	given to NdisMSetTimer has not yet expired.
//	A call to NdisMCancelTimer while the MiniportTimer function is running has 
//	no effect on the execution of MiniportTimer. It continues to run until it returns control.
//	In other words, PlatformCancelTimer() cannot cancel the running thread.
// Sync from 818xB. By Bruce, 2008-12-01.
//
BOOLEAN
PlatformCancelTimer(
	IN	PVOID				Adapter,
	IN	PRT_TIMER			pTimer
	)
{
	PADAPTER			pAdapter = (PADAPTER)Adapter;
	PPORT_COMMON_INFO 	pPortCommonInfo = pAdapter->pPortCommonInfo;
	BOOLEAN				bCanceled = FALSE;	
	
	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));

//	RT_ASSERT((pTimer->Status & RT_TIMER_STATUS_INITIALIZED), ("Cancel timer without initialize !!\n"))

	if( !(pTimer->Status & RT_TIMER_STATUS_INITIALIZED) )
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: This timer does not initialized! \n", __FUNCTION__));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}
	
	if(pTimer->Status & (RT_TIMER_STATUS_RELEASED))
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: TimerName: %s: This timer has been released! \n", __FUNCTION__, pTimer->szID));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return TRUE;
	}
		
	//
	// 061024, rcnjko: 
	// We reset RT_TIMER_STATUS_SET before canceling the timer object 
	// to make sure we only try to cancel is only cancel once.
	// Besides, OutStandingTimerCnt is decreased here if we cancel the 
	// timer object sucessfully, otherwise, it will be decreased in the 
	// end of callback function.
	//
	// 2007.04.30, Roger:
	// In Win98Me, Sometimes we could not set timer successful due to system out resource,
	// but we have no idea about that and still set status to RT_TIMER_STATUS_SET in PlatformSetTimer().
	// So we should pay attention to this condition and set correct reference-count and outstanding-count.
	//
	if((pTimer->Status & (RT_TIMER_STATUS_SET) || pTimer->Status & (RT_TIMER_STATUS_PERIODIC)))
	{
		RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("=====>PlatformCancelTimer(%s), status(%#x)\n", pTimer->szID, pTimer->Status));

		//
		// Clear RT_TIMER_STATUS_SET flag to make sure we only cancel it once after set.
		//
		//pTimer->Status &= (~RT_TIMER_STATUS_SET); //YJ,move,120112
		pTimer->Status &= (~RT_TIMER_STATUS_PERIODIC); 
		//NdisReleaseSpinLock(&(pNdisCommon->TimerLock)); //YJ,del,120112
	
		bCanceled = NdisCancelTimerObject(pTimer->Handle.NdisTimerHandle);	

		//NdisAcquireSpinLock(&(pNdisCommon->TimerLock));   //YJ,del,120112
		if(!bCanceled)
		{ // Timer had already been fired or was not set successful before, by Roger. 2007.04.30.

			pTimer->Status |= RT_TIMER_STATUS_CANCEL_NG;
			NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));
			RT_TRACE(COMP_SYSTEM, DBG_WARNING, ("<=====PlatformCancelTimer(%s), cancel NG, status(%#x)\n", pTimer->szID, pTimer->Status));
		}
		else
		{ // Successfully cancel the timer. 
			pTimer->Status &= (~RT_TIMER_STATUS_SET);   //YJ,move,120111

			pTimer->TimerStallSlotCount = 0; // For Timer stall checking mechanism, by Roger. 2007.05.16.	

			NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));

			RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("<=====PlatformCancelTimer(%s), cancel OK, status(%#x)\n", pTimer->szID, pTimer->Status));
		}
	}
	else
	{
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));
		RT_TRACE(COMP_SYSTEM, DBG_WARNING, ("PlatformCancelTimer(%s), timer is NOT set case: status(%#x)!!!\n", pTimer->szID, pTimer->Status));
	}
	
	return bCanceled;
}


//
// Description:
//	Release this timer and prohibit from setting timer.
//	In general, we can release timer to set released flag to prohibit setting a timer except 
//	the timer is initialized again.
// By Bruce, 2008-12-01.
//
BOOLEAN
PlatformReleaseTimer(
	IN	PVOID				Adapter,
	IN	PRT_TIMER			pTimer
	)
{
	u4Byte 				i = 0;
	BOOLEAN				bDoRelease = FALSE;
	BOOLEAN				bCanceled = FALSE;
	PADAPTER			pAdapter = (PADAPTER)Adapter;
	PPORT_COMMON_INFO	pPortCommonInfo = pAdapter->pPortCommonInfo;
	
	RT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL, ("Timer Releasing Should be Called in PASSIVE_LEVEL!\n"));

	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
	
	if( !(pTimer->Status & RT_TIMER_STATUS_INITIALIZED) )
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: This timer does not initialized! \n", __FUNCTION__));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}
	
	if(!(pTimer->Status & RT_TIMER_STATUS_RELEASED))
	{
		bDoRelease = TRUE;
		// Clean System Timer Queue
		if( NdisCancelTimerObject(pTimer->Handle.NdisTimerHandle) == FALSE )	
		{
			NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));	
			// Clean System DPC Queue for All Processors
			KeFlushQueuedDpcs();
			NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));
		}	
	}
	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));	

	if(bDoRelease)
	{
		DebugResourceTimerReleaseTimerMirror(pAdapter, pTimer);

		N6CTimerResourceRemove(pAdapter, pTimer);

		// Release System Timer
		NdisFreeTimerObject(pTimer->Handle.NdisTimerHandle);

		pTimer->Status = RT_TIMER_STATUS_RELEASED;	
	
		pPortCommonInfo->uNumberOfReleasedTimers++;
	}

	return TRUE;
}

	
//
// Description:
//		- Timer resource management for SoC off on Windows Mobile platform
//		- This routine will cancel and suspend specific timer
//
//	Assumption:
//		- Resource allocation for the TimerLock is required before calling this function.
//		- TimerLock is not acquired before calling this function
//
//	2016.01.15, created by Roger.
//
BOOLEAN
PlatformSuspendTimer(
	IN	PVOID				Adapter,
	IN	PRT_TIMER			pTimer
	)
{
	PADAPTER			pAdapter = (PADAPTER)Adapter;
	PPORT_COMMON_INFO 	pPortCommonInfo = pAdapter->pPortCommonInfo;
	BOOLEAN				bCanceled = FALSE;
	
		
	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));

	pTimer->PreviousStatus = pTimer->Status;

	if( !(pTimer->Status & RT_TIMER_STATUS_INITIALIZED) )
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: This timer does not initialized! \n", __FUNCTION__));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	if(pTimer->Status & (RT_TIMER_STATUS_RELEASED))
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: TimerName: %s: This timer has been released! \n", __FUNCTION__, pTimer->szID));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
	return TRUE;
}

	if((pTimer->Status & (RT_TIMER_STATUS_SET) || pTimer->Status & (RT_TIMER_STATUS_PERIODIC)))
	{
		RT_TRACE(COMP_SYSTEM, DBG_TRACE, ("=====>PlatformSuspendTimer(%s), status(%#x)\n", pTimer->szID, pTimer->Status));
		
		pTimer->Status &= (~RT_TIMER_STATUS_PERIODIC);


	#if (IS_OS_WINCE)	// WinCE7 only supports N5 Timer Interface	
		NdisMCancelTimer(&(pTimer->Handle.NdisTimerHandle), &bCanceled);	
	#else	
		bCanceled = NdisCancelTimerObject(pTimer->Handle.NdisTimerHandle);	
	#endif		

		if(!bCanceled)
		{ // Timer had already been fired or was not set successful before, by Roger. 2007.04.30.

			pTimer->Status |= RT_TIMER_STATUS_CANCEL_NG;
			NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));
			RT_TRACE(COMP_SYSTEM, DBG_WARNING, ("<=====PlatformSuspendTimer(%s), cancel NG, status(%#x)\n", pTimer->szID, pTimer->Status));
		}
		else
		{ // Successfully cancel the timer. 

			//
			// Clear RT_TIMER_STATUS_SET flag to make sure we only cancel it once after set.
			//
			pTimer->Status &= (~RT_TIMER_STATUS_SET);   //YJ,move,120111

			pTimer->TimerStallSlotCount = 0; // For Timer stall checking mechanism, by Roger. 2007.05.16.	

			NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));

			RT_TRACE(COMP_SYSTEM, DBG_TRACE, ("<=====PlatformSuspendTimer(%s), cancel OK, status(%#x)\n", pTimer->szID, pTimer->Status));
		}		
		pTimer->Status |= RT_TIMER_STATUS_SUSPENDED;
	}
	else
	{
		pTimer->Status |= RT_TIMER_STATUS_SUSPENDED;
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));
		RT_TRACE(COMP_SYSTEM, DBG_TRACE, ("PlatformSuspendTimer(%s), timer is NOT set case: status(%#x)!!!\n", pTimer->szID, pTimer->Status));
	}
	
	return bCanceled;
}


//
//	Description: 
//		- Timer resource management for SoC off on Windows Mobile platform
//		- This routine will resume suspended timer
//		- Periodic timer will be rescheduing within this routine according to previous satus
//
//	Assumption:
//		- Resource allocation for the TimerLock is required before calling this function.
//		- TimerLock is not acquired before calling this function
//
//	2016.01.15, created by Roger.
//
BOOLEAN
PlatformResumeTimer(
	IN	PVOID				Adapter,
	IN	PRT_TIMER			pTimer
	)
{
	PADAPTER			pAdapter = (PADAPTER)Adapter;
	PPORT_COMMON_INFO 	pPortCommonInfo = pAdapter->pPortCommonInfo;
	BOOLEAN				bCanceled = TRUE;	
	u4Byte				PreviousTimerStatus = 0;

		
	NdisAcquireSpinLock(&(pPortCommonInfo->TimerLock));	

	if( !(pTimer->Status & RT_TIMER_STATUS_INITIALIZED) )
	{
		RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("%s: This timer does not initialized! \n", __FUNCTION__));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}
	
	if(pTimer->Status & (RT_TIMER_STATUS_RELEASED))
	{
		RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("%s: TimerName: %s: This timer has been released! \n", __FUNCTION__, pTimer->szID));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	if(!(pTimer->Status & (RT_TIMER_STATUS_SUSPENDED)))
	{
		RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("%s: TimerName: %s: This timer had not been paused before, so timer resumption is not required.\n", __FUNCTION__, pTimer->szID));
		NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));		
		return FALSE;
	}

	PreviousTimerStatus = pTimer->PreviousStatus;
	
	pTimer->Status &= ~RT_TIMER_STATUS_SUSPENDED;
	pTimer->PreviousStatus = pTimer->Status;
	
	NdisReleaseSpinLock(&(pPortCommonInfo->TimerLock));	

	if(PreviousTimerStatus & RT_TIMER_STATUS_PERIODIC){
		RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("Reschedule Periodic Timer(%s), status(%#x)\n", pTimer->szID, PreviousTimerStatus));
		PlatformSetPeriodicTimer(pTimer->Adapter, pTimer, pTimer->msDelay);
	}
	
	return bCanceled;
}
	
//
// Description:
//	Initialize the event and queue associated to the all timers.
// Assumption:
//	PASSIVE_LEVEL
// Sync from 818xB. By Bruce, 2008-03-11.
//
VOID
N6InitTimerSync(
	IN	PVOID	Adapter
	)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter((PADAPTER)Adapter);
	PPORT_COMMON_INFO pPortCommonInfo = pDefaultAdapter->pPortCommonInfo;
	
	RT_TRACE(COMP_INIT, DBG_TRACE, ("---> PlatformInitTimerSync()\n"));

	pPortCommonInfo->ActiveTimerCallbackCount = 1;	// Set default number of outstanding timer as 1.
	
	RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("=====>N6InitTimerSync(): Set ActiveTimerCallbackCount to 1 !\n"));

	PLATFORM_INIT_RT_SPINLOCK(pPortCommonInfo->TimerLock);
	PLATFORM_INIT_RT_EVENT(pPortCommonInfo->AllTimerCompletedEvent);

	RT_ASSERT(pPortCommonInfo->uNumberOfAllocatedTimers == 0, ("N6InitTimerSync: pPortCommonInfo->uNumberOfAllocatedTimers != 0\n"));
	RT_ASSERT(pPortCommonInfo->uNumberOfReleasedTimers == 0, ("N6InitTimerSync: pPortCommonInfo->uNumberOfReleasedTimers != 0\n"));

	pPortCommonInfo->uNumberOfAllocatedTimers = 0;
	pPortCommonInfo->uNumberOfReleasedTimers = 0;
	
	N6CTimerResourceInit(Adapter); // Initialization for Timer resource handling

	RT_TRACE(COMP_INIT, DBG_TRACE, ("<--- PlatformInitTimerSync()\n"));
}

VOID
N6WaitTimerSync(
	IN	PVOID	Adapter
)
{
	PPORT_COMMON_INFO pPortCommonInfo = ((PADAPTER)Adapter)->pPortCommonInfo;
	BOOLEAN				bAllTimerCompleted = FALSE;
	u4Byte				i = 0;
	
	RT_TRACE(COMP_INIT, DBG_TRACE, ("---> N6WaitTimerSync()\n"));


	RT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL, ("N6WaitTimerSync Should be Called in PASSIVE_LEVEL!\n"));

	// Clean System DPC Queue for All Processors
	KeFlushQueuedDpcs();

	PLATFORM_ACQUIRE_RT_SPINLOCK(pPortCommonInfo->TimerLock);
	
	pPortCommonInfo->ActiveTimerCallbackCount--;	// Since we set default ActiveTimerCallbackCount as 1 N6InitTimerSync()
	
	RT_TRACE(COMP_SYSTEM, DBG_LOUD, ("=====>N6WaitTimerSync(): ActiveTimerCallbackCount = %d!!!\n", pPortCommonInfo->ActiveTimerCallbackCount));

	RT_ASSERT(pPortCommonInfo->ActiveTimerCallbackCount >= 0, ("pPortCommonInfo->ActiveTimerCallbackCount should not be negative !\n"));

	if(pPortCommonInfo->ActiveTimerCallbackCount > 0)
	{// Wait for outstanding timer to complete all of timer callback functions

		RT_TRACE(COMP_INIT, DBG_LOUD, ("N6WaitTimerSync(): %d timer is outstanding, waiting...\n", pPortCommonInfo->ActiveTimerCallbackCount));

		PLATFORM_RELEASE_RT_SPINLOCK(pPortCommonInfo->TimerLock);

		bAllTimerCompleted = PLATFORM_WAIT_RT_EVENT(pPortCommonInfo->AllTimerCompletedEvent, 2000);

		if(!bAllTimerCompleted)
		{
			RT_TRACE(COMP_INIT, DBG_WARNING, ("N6WaitTimerSync(): %d timer is outstanding, waiting time out !\n", pPortCommonInfo->ActiveTimerCallbackCount));
		}

		PLATFORM_RESET_RT_EVENT(pPortCommonInfo->AllTimerCompletedEvent);
	}
	else
	{
		PLATFORM_RELEASE_RT_SPINLOCK(pPortCommonInfo->TimerLock);
	}

	PLATFORM_ACQUIRE_RT_SPINLOCK(pPortCommonInfo->TimerLock);
	
	if(((PADAPTER)Adapter)->MgntInfo.RegSuspendTimerInLowPwr)
		pPortCommonInfo->ActiveTimerCallbackCount = 1;	// Since sync method was finished, so reset default number of outstanding timer as 1.

	PLATFORM_RELEASE_RT_SPINLOCK(pPortCommonInfo->TimerLock);
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("N6WaitTimerSync(): bAllTimerCompleted: %d, pPortCommonInfo->ActiveTimerCallbackCount: %d\n", bAllTimerCompleted, pPortCommonInfo->ActiveTimerCallbackCount));

	RT_TRACE(COMP_INIT, DBG_TRACE, ("<--- N6WaitTimerSync()\n"));
}

VOID
N6DeInitTimerSync(
	IN	PVOID	Adapter
	)
{
	PADAPTER pAdapter = (PADAPTER) Adapter;
	PPORT_COMMON_INFO pPortCommonInfo = pAdapter->pPortCommonInfo;
	BOOLEAN				bAllTimerCompleted = TRUE;

	FunctionIn(COMP_INIT);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("pPortCommonInfo->uNumberOfAllocatedTimers: %d\n", pPortCommonInfo->uNumberOfAllocatedTimers));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("pPortCommonInfo->uNumberOfReleasedTimers: %d\n", pPortCommonInfo->uNumberOfReleasedTimers));
	RT_ASSERT(pPortCommonInfo->uNumberOfAllocatedTimers == pPortCommonInfo->uNumberOfReleasedTimers, ("Timer Leakage: Use DEBUG_RESOURCE_TIMER Flag to Debug!\n"));
	if(pPortCommonInfo->uNumberOfAllocatedTimers != pPortCommonInfo->uNumberOfReleasedTimers) {
		DebugResourceTimerDumpUnreleasedTimer(pAdapter);
	}
	PLATFORM_FREE_RT_SPINLOCK(pPortCommonInfo->TimerLock);
	PLATFORM_FREE_RT_EVENT(pPortCommonInfo->AllTimerCompletedEvent);

	N6CTimerResourceDump(Adapter);
}



//=============================================================================
// End timer  function.
//============================================================================



VOID
N6CInitThread(
	IN	PVOID	pContext)
{
	PADAPTER			Adapter = (PADAPTER)pContext;
	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;
	BOOLEAN			bSupportUsbTxThread;
	BOOLEAN			bSupportUsbIOThread;
	
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USB_TX_THREAD, (PBOOLEAN)&bSupportUsbTxThread);
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USB_IO_THREAD, (PBOOLEAN)&bSupportUsbIOThread);
	
	NdisAllocateSpinLock(&(pNdisCommon->ThreadLock));
	NdisInitializeEvent(&(pNdisCommon->AllThreadCompletedEvent));

	PlatformInitializeThread(
		Adapter,  
		&pNdisCommon->InitializeAdapterThread,  
		(RT_THREAD_CALL_BACK)InitializeAdapterThread,  
		"InitializeAdapterThread",
		FALSE,
		0,
		NULL);
	TX_InitThreads(Adapter);

	//
	// SDIO Tx thread initialization
	//
	PlatformInitializeThread(
		Adapter,  
		&pNdisCommon->TxHandleThread,  
		(RT_THREAD_CALL_BACK)N6SdioTxThreadCallback,  
		"N6SdioTxThreadCallback",
		FALSE,
		0,
		NULL);

	PlatformRunThread(
		Adapter, 
		&pNdisCommon->TxHandleThread, 
		PASSIVE_LEVEL,
		NULL);
#if RTL8723_SDIO_IO_THREAD_ENABLE 
	//
	// SDIO Register IO  thread initialization
	//
	PlatformInitializeThread(
		Adapter,  
		&pNdisCommon->IOHandleThread,  
		(RT_THREAD_CALL_BACK)N6SdioIOThreadCallback,  
		"N6SdioIOThreadCallback",
		FALSE,
		0,
		NULL);

	PlatformRunThread(
		Adapter, 
		&pNdisCommon->IOHandleThread, 
		PASSIVE_LEVEL,
		NULL);
#endif

	CustomScan_Start(GET_CUSTOM_SCAN_INFO(Adapter));
}


VOID
N6CDeInitThread(
	IN	PVOID	pContext
	)
{
	PADAPTER		Adapter = (PADAPTER)pContext;
	
	PRT_NDIS6_COMMON pNdisCommon = Adapter->pNdisCommon;
	BOOLEAN		bSupportUsbTxThread;
	BOOLEAN		bSupportUsbIOThread;

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USB_TX_THREAD, (PBOOLEAN)&bSupportUsbTxThread);
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USB_IO_THREAD, (PBOOLEAN)&bSupportUsbIOThread);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("N6CDeInitThread()\n"));
	
	PlatformWaitThreadEnd(Adapter,&(pNdisCommon->InitializeAdapterThread));
	PlatformCancelThread(Adapter, &(pNdisCommon->InitializeAdapterThread));
	PlatformReleaseThread(Adapter, &(pNdisCommon->InitializeAdapterThread));

	TX_DeInitThreads(Adapter);

	// Cancel and release Tx handle thread
	PlatformWaitThreadEnd(Adapter, &(pNdisCommon->TxHandleThread));
	PlatformCancelThread(Adapter, &(pNdisCommon->TxHandleThread));
	PlatformReleaseThread(Adapter, &(pNdisCommon->TxHandleThread));
#if RTL8723_SDIO_IO_THREAD_ENABLE 
	// Cancel and release IO handle thread
	PlatformWaitThreadEnd(Adapter, &(pNdisCommon->IOHandleThread));	
	PlatformCancelThread(Adapter, &(pNdisCommon->IOHandleThread));	
	PlatformReleaseThread(Adapter, &(pNdisCommon->IOHandleThread));
#endif	

	PlatformFreeWorkItem( &(Adapter->pPortCommonInfo->WdiData.TaskWorkItem) );
	PlatformFreeWorkItem( &(Adapter->pPortCommonInfo->WdiData.PropertyWorkItem) );
	
	PlatformWaitThreadEnd(Adapter, &(Adapter->pPortCommonInfo->WdiData.TaskThread));
	PlatformCancelThread(Adapter, &(Adapter->pPortCommonInfo->WdiData.TaskThread));
	PlatformReleaseThread(Adapter, &(Adapter->pPortCommonInfo->WdiData.TaskThread));

	PlatformWaitThreadEnd(Adapter, &(Adapter->pPortCommonInfo->WdiData.PropertyThread));
	PlatformCancelThread(Adapter, &(Adapter->pPortCommonInfo->WdiData.PropertyThread));
	PlatformReleaseThread(Adapter, &(Adapter->pPortCommonInfo->WdiData.PropertyThread));

	PlatformWaitThreadEnd(Adapter, &(Adapter->RxNotifyThread));
	PlatformCancelThread(Adapter, &(Adapter->RxNotifyThread));
	PlatformReleaseThread(Adapter, &(Adapter->RxNotifyThread));

	CustomScan_Stop(GET_CUSTOM_SCAN_INFO(Adapter));

	NdisWaitEvent(&(pNdisCommon->AllThreadCompletedEvent),0);
	NdisResetEvent(&(pNdisCommon->AllThreadCompletedEvent));
	NdisFreeSpinLock(&(pNdisCommon->ThreadLock));
}



//=============================================================================
// Workitem  function.
//============================================================================

//
// Description:
//	Initialize an RT_WORK_ITEM object.
// Sync from 818xB, by Bruce, 2008-03-13.
//
RT_STATUS
PlatformInitializeWorkItem(
	IN	PVOID						Adapter,
	IN	PRT_WORK_ITEM				pRtWorkItem,
	IN	RT_WORKITEM_CALL_BACK		RtWorkItemCallback,
	IN	PVOID						pContext,
	IN	const char*					szID
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = ((PADAPTER)Adapter)->pNdisCommon;
	PPLATFORM_EXT		pPlatformExt = NULL;
	RT_STATUS			status = RT_STATUS_SUCCESS;

	RT_TRACE(COMP_DBG, DBG_LOUD, ("PlatformInitializeWorkItem(): %s\n", szID));

	pRtWorkItem->Adapter = Adapter;
	pRtWorkItem->pContext = pContext;
	pRtWorkItem->CallbackFunc = RtWorkItemCallback;
	pRtWorkItem->Handle = NdisAllocateIoWorkItem(pNdisCommon->hNdisAdapter);
	if (pRtWorkItem->Handle == NULL)
	{
		PlatformZeroMemory( pRtWorkItem, sizeof(RT_WORK_ITEM) );
		pRtWorkItem->bFree = TRUE;
		RT_TRACE(COMP_DBG, DBG_SERIOUS, ("PlatformInitializeWorkItem(): failed to NdisAllocateIoWorkItem: %s\n", szID));
		return RT_STATUS_RESOURCE;
	}

	pRtWorkItem->RefCount = 1;

	//
	// Allocate the platform dependent extension.
	// 2006.09.28, by shien chang.
	//
	pRtWorkItem->pPlatformExt = NULL;
	status = PlatformAllocateMemory( Adapter, (PVOID*)(&pPlatformExt), sizeof(PLATFORM_EXT) );
	if (status != RT_STATUS_SUCCESS)
	{
		NdisFreeIoWorkItem(pRtWorkItem->Handle);
		PlatformZeroMemory( pRtWorkItem, sizeof(RT_WORK_ITEM) );
		pRtWorkItem->bFree = TRUE;
		RT_TRACE(COMP_DBG, DBG_SERIOUS, ("PlatformInitializeWorkItem(): failed to allocate platform extensions: %s\n", szID));
		return RT_STATUS_RESOURCE;
	}

	PlatformZeroMemory( pPlatformExt, sizeof(PLATFORM_EXT) );
	pRtWorkItem->pPlatformExt = pPlatformExt;
	
	NdisInitializeEvent( &(pPlatformExt->Event) );

	NdisAllocateSpinLock( &(pPlatformExt->Lock) );

	PlatformZeroMemory(pRtWorkItem->szID, 36);
	if(szID != NULL)
	{
		ASCII_STR_COPY(pRtWorkItem->szID, szID, 36);
	}

	pRtWorkItem->bFree =  FALSE;
	return status;
}





//
// Description:
//	Start RT_WORK_ITEM operation.
//	This function only be done when RT_WORK_ITEM is stopeed and initialized.
//
VOID
PlatformStartWorkItem(
	IN	PRT_WORK_ITEM	pRtWorkItem
	)
{
	PPLATFORM_EXT	pPlatformExt = (PPLATFORM_EXT)pRtWorkItem->pPlatformExt;

	if( pPlatformExt == NULL )
		return;

	NdisAcquireSpinLock( &(pPlatformExt->Lock) );
	// Check if Workitem is initialized.
	if(pRtWorkItem->Adapter== NULL || pRtWorkItem->CallbackFunc==NULL || pPlatformExt==NULL)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
		return;
	}

	// Check if Workitem is started.
	if(pRtWorkItem->RefCount > 0)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
		return;
	}

	if(pRtWorkItem->bFree)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
		return;
	}

	// Enable Workitem.
	pRtWorkItem->RefCount = 1;
	NdisReleaseSpinLock( &(pPlatformExt->Lock) );
}


// 
// Description:
//	Wait the workitem complete if it is running and stop the workitem from scheduling it.
//
VOID
PlatformStopWorkItem(
	PRT_WORK_ITEM	pRtWorkItem
	)
{
	PPLATFORM_EXT	pPlatformExt = (PPLATFORM_EXT)pRtWorkItem->pPlatformExt;

	if( pPlatformExt == NULL )
		return;
	
	NdisAcquireSpinLock( &(pPlatformExt->Lock) );
	
	// Check if Workitem is initialized.
	if(pRtWorkItem->Adapter== NULL || pRtWorkItem->CallbackFunc==NULL || pPlatformExt==NULL ||	pRtWorkItem->bFree)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
		return;
	}

	if(pRtWorkItem->bFree)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
		return;
	}

	RT_TRACE(COMP_DBG, DBG_LOUD, ("===> PlatformStopWorkItem(): %s\n", pRtWorkItem->szID));

	// This workitem has never been initialized or stopped already.
	if(pRtWorkItem->RefCount == 0)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
		return;
	}

	pRtWorkItem->RefCount --;
	if(pRtWorkItem->RefCount > 0)
	{
		RT_TRACE(COMP_DBG, DBG_LOUD, ("PlatformStopWorkItem(): %s, some workitem is pending, RefCount: %d\n", pRtWorkItem->szID, pRtWorkItem->RefCount));

		NdisReleaseSpinLock( &(pPlatformExt->Lock) );

		while(TRUE)
		{
			if( NdisWaitEvent(&(pPlatformExt->Event), 50) )
			{
				NdisAcquireSpinLock( &(pPlatformExt->Lock) );
				NdisResetEvent(&(pPlatformExt->Event));
				NdisReleaseSpinLock( &(pPlatformExt->Lock) );
				break;
			}
		}
	}
	else
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
	}
	RT_TRACE(COMP_DBG, DBG_LOUD, ("<=== PlatformStopWorkItem(): %s\n", pRtWorkItem->szID));
}

//
// Description:
//	DeInitialize an RT_WORK_ITEM object.
// 2006.09.28, by shien chang.
//
VOID
PlatformFreeWorkItem(
	IN	PRT_WORK_ITEM	pRtWorkItem
	)
{
	PPLATFORM_EXT	pPlatformExt = pRtWorkItem->pPlatformExt;

	if( pPlatformExt == NULL )
		return;

	RT_TRACE(COMP_DBG, DBG_LOUD, ("===> PlatformFreeWorkItem(): %s\n", pRtWorkItem->szID));

	PlatformStopWorkItem(pRtWorkItem);
	
	NdisAcquireSpinLock( &(pPlatformExt->Lock) );
	if(pRtWorkItem->bFree)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
		return;
	}
	NdisReleaseSpinLock( &(pPlatformExt->Lock) );

	NdisFreeSpinLock( &(pPlatformExt->Lock) );

	//
	// Deallocate the platform extension.
	// 2006.09.28, by shien chang.
	//
	if(pPlatformExt)
		PlatformFreeMemory( pPlatformExt, sizeof(PLATFORM_EXT) );

	NdisFreeIoWorkItem(pRtWorkItem->Handle);
	pRtWorkItem->bFree = TRUE;
	
	RT_TRACE(COMP_DBG, DBG_LOUD, ("<=== PlatformFreeWorkItem(): %s\n", pRtWorkItem->szID));
}

//
// Description:
//	Schedule a work item. 
// 2006.09.28, by shien chang.
//

BOOLEAN 
PlatformScheduleWorkItem(
	IN	PRT_WORK_ITEM	pRtWorkItem
)
{
	BOOLEAN			bResult = FALSE;
	PPLATFORM_EXT	pPlatformExt = pRtWorkItem->pPlatformExt;

	if( pPlatformExt == NULL )
		return bResult;

	NdisAcquireSpinLock( &(pPlatformExt->Lock) );
	
	if(pRtWorkItem->bFree)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
		return bResult;
	}
	

	// Make sure at most one such workitem executed.
	if(pRtWorkItem->RefCount == 1)
	{
		pRtWorkItem->RefCount ++;
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );

		NdisQueueIoWorkItem( pRtWorkItem->Handle, 
							Ndis6WorkItemCallback, 
							pRtWorkItem);
		bResult = TRUE;
	}
	else if(pRtWorkItem->RefCount == 0)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
	}
	else
	{
		// <RJ_TODO> We should do something to handle this case.
		// For example, should we queue requests of such workitem,
		// and schedule them one after another.
		RT_TRACE(COMP_MLME, DBG_LOUD, ("PlatformScheduleWorkItem(): %s Failed, RefCount: %d!!!\n", pRtWorkItem->szID, pRtWorkItem->RefCount));

		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
	}

	return bResult;
}

//
// Description:
//	Check if the work item has been scheduled.
// 2006.09.29, by shien chang.
//
BOOLEAN
PlatformIsWorkItemScheduled(
	IN	PRT_WORK_ITEM				pRtWorkItem
	)
{
	PPLATFORM_EXT	pPlatformExt = pRtWorkItem->pPlatformExt;
	BOOLEAN			bScheduled;

	if( pPlatformExt == NULL )
		return FALSE;

	NdisAcquireSpinLock( &(pPlatformExt->Lock) );
	
	if(pRtWorkItem->bFree)
	{
		NdisReleaseSpinLock( &(pPlatformExt->Lock) );
		return FALSE;
	}
	

	// We define both the RefCount 0 and 2 as scheduled.
	// 2006.09.29, by shien chang.
	bScheduled = !(pRtWorkItem->RefCount == 1);
	
	NdisReleaseSpinLock( &(pPlatformExt->Lock) );

	return bScheduled;
}


//=============================================================================
// End workitem  function.
//=============================================================================



//=============================================================================
// Memory function.
//=============================================================================

//
// Description:
//	Fill all the input memory with 0.
//
VOID
PlatformZeroMemory(
	IN	PVOID		ptr,
	IN	u4Byte		length
	)
{
	if(ptr)
		NdisZeroMemory(ptr,length);
}

//
// Description:
//	Memory manipulation routines.
//
RT_STATUS
PlatformAllocateMemoryWithTag(
	IN	PVOID		Adapter,
	IN  u4Byte      Tag,
	OUT	PVOID		*pPtr,
	IN	u4Byte		length
	)
{
	RT_STATUS		rtstatus = RT_STATUS_SUCCESS;
	NDIS_HANDLE		MiniportAdapterHandle = GlobalRtDriverContext.NdisContext.Ndis6MiniportDriverHandle;

	*pPtr = NdisAllocateMemoryWithTagPriority( MiniportAdapterHandle, 
											length, 
											Tag,
											NormalPoolPriority);
	if (*pPtr == NULL)
	{
		return RT_STATUS_FAILURE;
	}

	RTL_AllocateMemory_count ++;
	RTL_AllocateMemory_Len += length;

	return rtstatus;
}

//
// Description:
//	Free the allocated memory of the input pointer.
//
VOID
PlatformFreeMemory(
	IN	PVOID		ptr,
	IN	u4Byte		length
	)
{
	if(ptr)
	{
		NdisFreeMemory( ptr, length, 0);
		ptr = NULL;
		RTL_FreeMemory_count ++;
		RTL_FreeMemory_Len += length;

//		RTL_AllocateMemory_Len -= length;
	}
}

//
// Description:
//	Memory manipulation routines.
//
RT_STATUS
PlatformAllocateMemoryWithZero(
	IN	PVOID		Adapter,
	OUT	PVOID		*pPtr,
	IN	u4Byte		length
	)
{
	RT_STATUS		rtstatus = RT_STATUS_SUCCESS;
	NDIS_HANDLE		MiniportAdapterHandle = ((PADAPTER)Adapter)->pNdisCommon->hNdisAdapter;

	*pPtr = NdisAllocateMemoryWithTagPriority( MiniportAdapterHandle, 
											length, 
											'7818',
											NormalPoolPriority);
	if (*pPtr == NULL)
	{
		return RT_STATUS_FAILURE;
	}

	RTL_AllocateMemory_count ++;
	RTL_AllocateMemory_Len += length;

	NdisZeroMemory(*pPtr,length);
	
	return rtstatus;
}



//
// Description:
//	Copy a specified number of bytes from the source location to the destination.
//
VOID
PlatformMoveMemory(
	IN	PVOID		pDest,
	IN	PVOID		pSrc,
	IN	u4Byte		length
	)
{
	if(pDest != NULL && pSrc != NULL && length > 0)
		NdisMoveMemory(pDest,pSrc,length);
}

//
// Description:
//	Compare characters in two buffers.
//
s4Byte
PlatformCompareMemory(
	IN	PVOID		pBuf1,
	IN	PVOID		pBuf2,
	IN	u4Byte		length
	)
{
	return memcmp(pBuf1,pBuf2,length);
}

//
// Description:
//	Fills a caller-supplied buffer with the given character.
//
VOID
PlatformFillMemory(
	IN	PVOID		Buf,
	IN	u4Byte		Length,
	IN	u1Byte		Fill
	)
{
	if(Buf)
		NdisFillMemory(Buf, Length, Fill);
}

//=============================================================================
// End memory  function.
//=============================================================================



VOID
PlatformStallExecution(
	u4Byte		usDelay
	)
{
	ULONG	i = 0;

	if(usDelay <= MAX_STALL_TIME)
	{
		NdisStallExecution(usDelay);
	}
	else
	{
		if(NDIS_CURRENT_IRQL()==PASSIVE_LEVEL)
		{
			NdisMSleep(usDelay);
		}
		else
		{
			for(i=0; i<(usDelay)/MAX_STALL_TIME; i++)
			{
				NdisStallExecution(MAX_STALL_TIME);
			}
		}
	}
}

VOID
PlatformForceStallExecution(
	u4Byte		usDelay
	)
{
	ULONG	i;

	if(usDelay <= MAX_STALL_TIME)
	{
	    NdisStallExecution(usDelay);
    }
	else
	{
		for(i=0; i<(usDelay)/MAX_STALL_TIME; i++)
		{
			NdisStallExecution(MAX_STALL_TIME);
		}
	}
}

//
// Description:
//	Get the system time and return by microsecond.
//
u8Byte
PlatformGetCurrentTime(
	VOID
	)
{
	// The return value is in microsecond
	u8Byte			ret;
	LARGE_INTEGER	CurrentTime;
	
	NdisGetCurrentSystemTime(&CurrentTime);	// driver version
	ret=CurrentTime.QuadPart/10;
	
	return ret;
}

//
//	Description:
//		Handle TKIP MIC error. 
//		For example, indicate the MIC error event to supplicant.
//
//	Note:
//		From ProcessMICError() of 8185 NDIS driver.
//
//	2004.10.05, by rcnjko.
//
VOID
PlatformHandleTKIPMICError(
	IN	PVOID				Adapter,
	IN	BOOLEAN				bMcstDest,
	IN	u1Byte				Keyidx,
	IN	pu1Byte				pSrcAddr
	)
{
	PADAPTER				pAdapter;
	PMGNT_INFO				pMgntInfo;
	PRT_SECURITY_T			pSec;
	DOT11_TKIPMIC_FAILURE_PARAMETERS	TKIPFailureParam;

	// Set up context variables about the adpater from input.
	pAdapter = (PADAPTER)Adapter;
	pMgntInfo = &(pAdapter->MgntInfo);
	pSec = &(pMgntInfo->SecurityInfo);

	// Initialize event to indicate up.
	PlatformZeroMemory(&TKIPFailureParam, sizeof(DOT11_TKIPMIC_FAILURE_PARAMETERS));

	N6_ASSIGN_OBJECT_HEADER(
		TKIPFailureParam.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_TKIPMIC_FAILURE_PARAMETERS_REVISION_1,
		sizeof(DOT11_TKIPMIC_FAILURE_PARAMETERS));

	TKIPFailureParam.bDefaultKeyFailure = FALSE; //<SC_TODO>
	TKIPFailureParam.uKeyIndex = 0;//<SC_TODO>
	PlatformMoveMemory(
		TKIPFailureParam.PeerMac,
		pMgntInfo->Bssid,
		sizeof(DOT11_MAC_ADDRESS));

// For WiFi WPA2 Test: Let group MIC error packet be handled by the same way as pairwise packet. Annie, 2006-05-04.
//
//	if(!bMcstDest)
	{ // Pairewise key.
		u8Byte CurrTime = PlatformGetCurrentTime(); // In micro-second.
		u8Byte DiffTime = CurrTime - pSec->LastPairewiseTKIPMICErrorTime; // In micro-second.

		if(DiffTime > MIC_CHECK_TIME)
		{ // Fisrt MIC Error happend in 60 seconds.
			RT_TRACE(COMP_SEC, DBG_LOUD, ("PlatformHandleTKIPMICError(): First TKIP Pairewise MIC error.\n"));
			// Update MIC error time.
			pSec->LastPairewiseTKIPMICErrorTime = CurrTime;
		}
		else
		{ // Second MIC Error happend in 60 seconds.
			RT_TRACE(COMP_SEC, DBG_LOUD, ("PlatformHandleTKIPMICError(): Second TKIP Pairewise MIC error.\n"));
			// Reset MIC error time.
			pSec->LastPairewiseTKIPMICErrorTime = 0;
			// Set the flag to disassocate AP after MIC failure report send to AP.
			pSec->bToDisassocAfterMICFailureReportSend = TRUE;
			// Insert current BSSID to denied list.
			SecInsertDenyBssidList(pSec, pMgntInfo->Bssid, CurrTime);
		}

		if(OS_SUPPORT_WDI(pAdapter))
		{
		WDI_IndicateTKIPMICFailure(pAdapter);
	}
		else
		{
			N6IndicateStatus(
					Adapter,
					NDIS_STATUS_DOT11_TKIPMIC_FAILURE,
					&TKIPFailureParam,
					sizeof(DOT11_TKIPMIC_FAILURE_PARAMETERS));
		}
		

	}
//	else
//	{ // Group key.
//		// TODO: We should handle MIC error for group key in WPA2.
//	}
	PLATFORM_WRITE_EVENT_LOG(Adapter, RT_TKIP_MIC_ERROR, bMcstDest ? 2 : 1);
}

//
// Description:
//	
// Added by Annie, 2005-09-23,
// Ref: 8185 WMacRequestPreAuthentication().
//
VOID
PlatformRequestPreAuthentication(
	IN	PVOID							pvAdapter,
	IN	PRE_AUTH_INDICATION_REASON		Reason
	)
{
	PADAPTER			Adapter = (PADAPTER)pvAdapter;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &pMgntInfo->SecurityInfo;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	u1Byte				j;
	u2Byte				i;
	int					iNumBssid=0;
	BOOLEAN				bMatch;
	BOOLEAN				bRequireIndication = FALSE;
	u1Byte			uArray[sizeof(DOT11_PMKID_CANDIDATE_LIST_PARAMETERS ) + \
						(NUM_PRE_AUTH_KEY+1)*sizeof(DOT11_BSSID_CANDIDATE ) +1 ];
	pu1Byte				pucCurPtr;
	PRT_WLAN_BSS		pBssDesc = NULL;
	PDOT11_PMKID_CANDIDATE_LIST_PARAMETERS 	pPMKID;
	PDOT11_BSSID_CANDIDATE					pCandidate;



	RT_TRACE( COMP_SEC, DBG_TRACE, ("===> PlatformRequestPreAuthentication()\n" ) );

	//
	// Condition Check
	//
	if(	!pSecInfo->RegEnablePreAuth ||		// "EnablePreAuth" in NicRegTable[].
		!(pSecInfo->SecLvl == RT_SEC_LVL_WPA2)	||
		!pMgntInfo->mAssoc
		)
	{
		RT_TRACE( COMP_SEC, DBG_TRACE, ("PlatformRequestPreAuthentication(): return for EnablePreAuthentication=%d, SecLvl=%d, mAssoc=%d\n", pSecInfo->RegEnablePreAuth, pSecInfo->SecLvl, pMgntInfo->mAssoc ) );
		return;
	}

	//
	// Initialize the buffer for status indication.
	//
	PlatformZeroMemory( uArray, sizeof uArray );
	pucCurPtr = &uArray[0];
	
	pPMKID = (PDOT11_PMKID_CANDIDATE_LIST_PARAMETERS)pucCurPtr;
	pPMKID->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	pPMKID->Header.Revision = DOT11_PMKID_CANDIDATE_LIST_PARAMETERS_REVISION_1;
	pPMKID->Header.Size = sizeof(DOT11_PMKID_CANDIDATE_LIST_PARAMETERS);

	pucCurPtr = pucCurPtr + sizeof(DOT11_PMKID_CANDIDATE_LIST_PARAMETERS); // Offset to DOT11_BSSID_CANDIDATE buffer

	//------------------------------------------------------------------
	// (1) Include associated current BSSID into list
	//------------------------------------------------------------------
	
	// The current associated BSSID should always be in the StatusBuffer.		
	pCandidate = (PDOT11_BSSID_CANDIDATE)pucCurPtr;
	pCandidate->uFlags  =  DOT11_PMKID_CANDIDATE_PREAUTH_ENABLED;
	
	PlatformMoveMemory( pCandidate->BSSID, pMgntInfo->Bssid, 6 );
	iNumBssid++;	
	pucCurPtr += sizeof(DOT11_BSSID_CANDIDATE);		// pointer to next PMKID_CANDIDATE.

	// WPA2 beta site: It makes sense to clear the driver PMKID cache at the time of Disconnect.
	if(Reason == PRE_AUTH_INDICATION_REASON_ASSOCIATION)
	{
		bRequireIndication = TRUE;
	}
	

	//------------------------------------------------------------------
	// (2) Include other BSSID for roaming candidate list (with the same SSID)
	//------------------------------------------------------------------	
	for( i=0; i<pMgntInfo->NumBssDesc; i++ )	// reference enumeration method as SelectNetworkBySSID().
	{	
		pBssDesc = &(pMgntInfo->bssDesc[i]);
		
		// The number of entries indicated in the CandidateList should not be greater 
		//than the size of the PMKID cache supported by the driver and exposed through  
		//OID_802_11_CAPABILITY value NoOfPMKIDs
		if( iNumBssid >= NUM_PRE_AUTH_KEY )
		{
			break;
		}

		if( pBssDesc->SecLvl < RT_SEC_LVL_WPA2 )
		{
			RT_TRACE( COMP_SEC, DBG_TRACE, ("PlatformRequestPreAuthentication(): AP SecLvl=%d is not enough!\n", pBssDesc->SecLvl) );
			RT_PRINT_STR( COMP_SEC, DBG_TRACE, ("SSID"), pBssDesc->bdSsIdBuf, pBssDesc->bdSsIdLen );
			continue;
		}

		// Don't include current BSSID. It already exists.
		if( eqMacAddr(pBssDesc->bdBssIdBuf, pMgntInfo->Bssid) )
		{
			RT_TRACE( COMP_SEC, DBG_TRACE, ("PlatformRequestPreAuthentication(): \n") );
			RT_PRINT_ADDR( COMP_SEC, DBG_TRACE, ("BSSID is the same"), pBssDesc->bdBssIdBuf );
			continue;
		}


		// 1. Indicate to upper layer only if they have the same ssid.
		// 2. The NIC should only request preauthentication when the NIC is evaluating whether to roam to another BSSID.
		if(	( pBssDesc->bdSsIdLen == pMgntInfo->Ssid.Length )	&&
			eqNByte( pBssDesc->bdSsIdBuf, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length)
			)
		{
			// Add the bssid onto bssid list
			pCandidate = (PDOT11_BSSID_CANDIDATE)pucCurPtr;

			if( pBssDesc->bPreAuth )
				pCandidate->uFlags = DOT11_PMKID_CANDIDATE_PREAUTH_ENABLED;
			else
				pCandidate->uFlags = 0x0;

			CopyMem( pCandidate->BSSID, pBssDesc->bdBssIdBuf, 6 );
			iNumBssid++;
			pucCurPtr += sizeof(DOT11_BSSID_CANDIDATE);

			// Indicate to upper layer only if there is new AP that does not have PMKID kept in driver
			bMatch = FALSE;
			for( j=0; j<NUM_PMKID_CACHE; j++ )
			{
				if( pSecInfo->PMKIDList[j].bUsed )
				{
					if( eqMacAddr(pSecInfo->PMKIDList[j].Bssid, pMgntInfo->Bssid) )
					{
						RT_PRINT_ADDR( COMP_SEC, DBG_TRACE, ("PlatformRequestPreAuthentication(): BSSID exists => Do not indicate to upper layer"), pSecInfo->PMKIDList[j].Bssid );
						bMatch = TRUE;
					}
				}
			}
			if(!bMatch)
			{
				RT_TRACE( COMP_SEC, DBG_TRACE, ("PlatformRequestPreAuthentication(): New BSSID => Indicate to upper layer.\n"));
				bRequireIndication = TRUE;
			}
		}
	}

	//pPMKID->NumCandidates = iNumBssid;

	//------------------------------------------------------------------
	// (3) Indicate to upper layer.
	//------------------------------------------------------------------
	if(bRequireIndication)
	{
		UINT	UsedSize =	sizeof(DOT11_PMKID_CANDIDATE_LIST_PARAMETERS)	+	\
							(iNumBssid)*sizeof(DOT11_BSSID_CANDIDATE);
		
		pPMKID->uCandidateListSize = iNumBssid;
		pPMKID->uCandidateListOffset = sizeof(DOT11_PMKID_CANDIDATE_LIST_PARAMETERS);;

		RT_TRACE( COMP_SEC, DBG_LOUD, ("PlatformRequestPreAuthentication(): Indicate Buffer Size = %d\n", UsedSize) );
		RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "PlatformRequestPreAuthentication(): Indicate Buffer", uArray, UsedSize );

		N6IndicateStatus(
					Adapter,
					NDIS_STATUS_DOT11_PMKID_CANDIDATE_LIST,
					(PVOID)uArray,
					UsedSize
					);
	}
	
	RT_TRACE( COMP_SEC, DBG_LOUD, ("<=== PlatformRequestPreAuthentication()\n" ) );	

}

VOID
PlatformHandlePwiseKeyUpdata(
	IN	PVOID				Adapter
	)
{
}

// Description:
//		Handle Network Offload  
//		Try to Cancel Sacn
//
//	Added by CCW. 2013.04.17.
VOID
PlatformHandleNLOnScanCancel(
	IN	PVOID			pvAdapter
	)
{
	PADAPTER			Adapter = (PADAPTER)pvAdapter;
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;

	// Reset Indicate State !!
	pNdisCommon->bToIndicateNLOScanComplete = FALSE;
}


//
// Description:
//		Handle Network Offload  
//		Try to found out AP in Off load  List , and Indicate to upper layer
//
VOID
PlatformHandleNLOnScanComplete(
	IN	PVOID			pvAdapter
)
{
	PADAPTER			Adapter = (PADAPTER)pvAdapter;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PRT_WLAN_BSS		pCurrDesc = NULL;
	u4Byte				CurrBssIndex = 0;
	BOOLEAN				NeedIndicate = FALSE;
	PRT_OFFLOAD_NETWORK	pCurrNetwork = NULL;
	u4Byte				CurrWorkLoadIndex = 0;
	u1Byte				uArray[8] = {0};
	PRT_NLO_INFO		pNLOInfo = &(pMgntInfo->NLOInfo);

	RT_TRACE(COMP_MLME , DBG_LOUD , (" =====> PlatformHandleNLOnScanComplete  !!\n") );

	if( pNLOInfo->NumDot11OffloadNetwork == 0)
	{
		RT_TRACE(COMP_MLME , DBG_LOUD , ("return: pNdisCommon->NumDot11OffloadNetwork == 0 !\n"));		
		return;
	}
	
	if(pNdisCommon->bToIndicateNLOScanComplete == FALSE)
	{
		// Return to prevent from OS sets RESET_REQUEST & CONNECT_REQUEST after indicating
		// NDIS_STATUS_DOT11_OFFLOAD_NETWORK_STATUS_CHANGED during normal (non-NLO)
		// scan request flow. It will cause that the reconnect flow is interrupted by OID RESET_REQUEST
		// & CONNECT_REQUEST such that the first reconnect (to the default AP) action fail after PNP wake.
		RT_TRACE(COMP_MLME , DBG_LOUD , ("Return: NOT NLO scan request!\n"));		
		return;
	}
	
	// Need to check : bHiddenSSIDEnable can used now ??
	pMgntInfo->bHiddenSSIDEnable = TRUE;
	
	if(!OS_SUPPORT_WDI(Adapter))
	{

		// HCT Test alway to say OK !!!
		if( Adapter->bInHctTest )
		{
			NeedIndicate = TRUE;
		}

		for( CurrWorkLoadIndex = 0 ; ((CurrWorkLoadIndex < pNLOInfo->NumDot11OffloadNetwork) && (NeedIndicate == FALSE) ); CurrWorkLoadIndex++)
		{
			pCurrNetwork = &(pNLOInfo->dDot11OffloadNetworkList[CurrWorkLoadIndex]);
			for( CurrBssIndex = 0 ; ( (CurrBssIndex < pMgntInfo->NumBssDesc) && (NeedIndicate == FALSE)); CurrBssIndex++)
			{
				pCurrDesc = &(pMgntInfo->bssDesc[CurrBssIndex]);
				if(CompareSSID(pCurrDesc->bdSsIdBuf, pCurrDesc->bdSsIdLen, pCurrNetwork->ssidbuf, (u2Byte)pCurrNetwork->ssidlen))
				{
					// Security !!
					if( pCurrNetwork->bPrivacy == (pCurrDesc->bdCap & cPrivacy) && ( pCurrNetwork->chiper & pCurrDesc->PairwiseChiper) )
					{
						NeedIndicate = TRUE;
					}
				
				}
			}
		}

		N6IndicateStatus(
				Adapter,
				NDIS_STATUS_DOT11_OFFLOAD_NETWORK_STATUS_CHANGED,
				(PVOID)uArray,
				4
		);
	}
	else
	{		
	WDI_IndicateNLODiscovery(Adapter);
	}

	pNdisCommon->bToIndicateNLOScanComplete = FALSE;
	
	RT_TRACE( COMP_MLME , DBG_LOUD, (" <===== PlatformHandleNLOnScanComplete  !!\n") );

	return;
}


// 
//   Description:
//		Handle Network Offload  scan state 
//		Check need scan or not 
//
VOID
PlatformHandleNLOnScanRequest(
	IN	PVOID			pvAdapter
)
{
	PADAPTER				Adapter 	= (PADAPTER)pvAdapter;
	PMGNT_INFO				pMgntInfo 	= &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC		= GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_NDIS6_COMMON		pNdisCommon = Adapter->pNdisCommon;
	PRT_NLO_INFO			pNLOInfo 	= &(pMgntInfo->NLOInfo);
	BOOLEAN					bNeedScan 	= FALSE;

	// check enable NOL mode 
	if( pNLOInfo->NumDot11OffloadNetwork == 0 )
	{
		RT_TRACE(COMP_MLME , DBG_TRACE , ("<=== return: pNdisCommon->NumDot11OffloadNetwork == 0 !\n"));
		return;
	}

	RT_TRACE(COMP_MLME , DBG_LOUD, ("===> PlatformHandleNLOnScanRequest \n ") );
	RT_TRACE(COMP_MLME, DBG_TRACE, ("Oidcounter = %d \n",pNdisCommon->Oidcounter));
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("pNdisCommon->ScanPeriod: %d\n", pNdisCommon->ScanPeriod));
	RT_TRACE(COMP_MLME, DBG_LOUD, ("pNdisCommon->FastScanPeriod: %d\n", pNLOInfo->FastScanPeriod));	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("pNdisCommon->FastScanIterations: %d\n", pNLOInfo->FastScanIterations));
	RT_TRACE(COMP_MLME, DBG_LOUD, ("pNdisCommon->SlowScanPeriod: %d\n", pNLOInfo->SlowScanPeriod));

	// Check need to scan !!
	if( pNdisCommon->ScanPeriod > RT_CHECK_FOR_HANG_PERIOD )
	{
		pNdisCommon->ScanPeriod =  pNdisCommon->ScanPeriod - RT_CHECK_FOR_HANG_PERIOD;
	}
	else
	{
		bNeedScan = TRUE;
	}

	pNdisCommon->Oidcounter = 0;

	// Set up NEW ScanPeriod 
	if( bNeedScan )
	{
		if( pNLOInfo->FastScanIterations > 0 )
		{
			pNLOInfo->FastScanIterations --;
		}

		if( pNLOInfo->FastScanIterations > 0 )
		{
			pNdisCommon->ScanPeriod = pNLOInfo->FastScanPeriod;
		}
		else
		{
			pNdisCommon->ScanPeriod = pNLOInfo->SlowScanPeriod;
		}
	}

	// Scan state 
	// Note : 
	// 	Check NIC "CAN" scan or not 
	if( bNeedScan )
	{
		if( pNdisCommon->bFilterHiddenAP )
		{
			pMgntInfo->bHiddenSSIDEnable = FALSE;

		}

		pNdisCommon->bToIndicateNLOScanComplete = TRUE;
		RT_TRACE(COMP_MLME, DBG_LOUD, ("pNdisCommon->bToIndicateNLOScanComplete set to %s\n", pNdisCommon->bToIndicateNLOScanComplete?"TRUE":"FALSE"));

		//
		// Marked since the NLO may be initialized when disconnected. 
		//
		
		// Note : May need check BusyTraffic for ... 
		//if(pMgntInfo->bMediaConnect && !MgntRoamingInProgress(pMgntInfo)
		//&& !MgntScanInProgress(pMgntInfo) && !MgntIsLinkInProgress(pMgntInfo) &&
		//!GET_RM_INFO(pMgntInfo)->bGoingOn)
		{
			/*	MgntLinkRequest(
					Adapter,
					TRUE,							//bScanOnly
					pNdisCommon->bNLOActiveScan,		//bActiveScan,
					pNdisCommon->bFilterHiddenAP,		//FilterHiddenAP // Asked by Netgear's Lancelot for 8187 should look like their damn wg111v1, 2005.02.01, by rcnjko.
					FALSE,							// Update parameters
					NULL,							//ssid2scan
					0,								//NetworkType,
					0,								//ChannelNumber,
					0,								//BcnPeriod,
					0,								//DtimPeriod,
					0,								//mCap,
					NULL,							//SuppRateSet,
					NULL								//yIbpm,
					);*/
			NDIS_OID_REQUEST NdisRequest;
			VOID *req = NULL;

			RT_TRACE(COMP_MLME, DBG_LOUD, ("N6C_OID_DOT11_FLUSH_BSS_LIST\n"));
			NdisRequest.RequestType = NdisRequestSetInformation;
			N6C_OID_DOT11_FLUSH_BSS_LIST(Adapter, &NdisRequest);
		
			//Win8: Restart the passive scan ----------------------------------------------
			if(NULL == (req = CustomScan_AllocReq(GET_CUSTOM_SCAN_INFO(Adapter), NULL, NULL)))
			{
				RT_TRACE(COMP_MLME , DBG_LOUD, ("CustomScan_AllocReq == NULL, no Scan issue\n ") );
				RT_TRACE(COMP_MLME , DBG_LOUD, ("<=== PlatformHandleNLOnScanRequest \n ") );
				return;
			}

			CustomScan_AddScanChnl(req, 1, 1, SCAN_ACTIVE, (u2Byte)pNdisCommon->ScanPeriod, MGN_6M, NULL);
			CustomScan_AddScanChnl(req, 6, 1, SCAN_ACTIVE, (u2Byte)pNdisCommon->ScanPeriod, MGN_6M, NULL);
			CustomScan_AddScanChnl(req, 11, 1, SCAN_ACTIVE, (u2Byte)pNdisCommon->ScanPeriod, MGN_6M, NULL);

			CustomScan_IssueReq(GET_CUSTOM_SCAN_INFO(Adapter), req, CUSTOM_SCAN_SRC_TYPE_UNSPECIFIED, "nlo scan req");			
		}
	}

	RT_TRACE(COMP_MLME , DBG_LOUD, ("<=== PlatformHandleNLOnScanRequest \n ") );

	return;
}


//
// Description:
//	Release the MSDU buffered from the upper layer..
//
// Assumption:
//	TX lock acquired.
// By Bruce, 2008-11-28.
//
VOID
PlatformReleaseDataFrameQueued(
	IN	PADAPTER	pAdapter
	)
{
	PRT_NDIS6_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	PNET_BUFFER_LIST		pNetBufferList;
	PNDIS_ADAPTER_TYPE		pDevice = GET_NDIS_ADAPTER(pAdapter);
	PNET_BUFFER_LIST		pCurrNetBufferList, pNextNetBufferList;	
	BOOLEAN 			bUseWorkItem = FALSE;
	KIRQL			OldIrql;

	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

	if(pNdisCommon->RegNblRacingWA)
	{
		OldIrql = KeGetCurrentIrql();
		bUseWorkItem = (OldIrql > PASSIVE_LEVEL)? TRUE:FALSE;

		if(bUseWorkItem)
		{
			PlatformScheduleWorkItem(&(pNdisCommon->ReleaseDataFrameQueuedWorkItem));
		}
		else
		{
			N6CReleaseDataFrameQueuedWorkItemCallback(pAdapter);
		}

	}
	else
	{ // Old method
		
		while(TRUE)
		{
			PlatformAcquireSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
			if(N6CIsNblWaitQueueEmpty(pNdisCommon->TxNBLWaitQueue))
			{
				PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
				break;
			}
			pNetBufferList = N6CRemoveNblWaitQueue(&pNdisCommon->TxNBLWaitQueue, TRUE);

			if(pNetBufferList != NULL)
				RT_NBL_SET_REF_CNT(pNetBufferList, 0);   //YJ,test for MPE,120221

			for (pCurrNetBufferList = pNetBufferList;
				pCurrNetBufferList != NULL;
				pCurrNetBufferList = pNextNetBufferList)
			{
				pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);
				NET_BUFFER_LIST_STATUS(pCurrNetBufferList) = NDIS_STATUS_SUCCESS;
			}
			PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);

			// Prefast warning C6387: 'pNetBufferList' could be '0':  this does not adhere to the specification for the function 'NdisMSendNetBufferListsComplete'.
			if (pNetBufferList != NULL)
			{
				NdisMSendNetBufferListsComplete(
					pDevice->hNdisAdapter,
					pNetBufferList,
					((NDIS_CURRENT_IRQL() == DISPATCH_LEVEL) ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0));
			}
		}
	}

	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
}


// Implement the indication of the P2P event
//	Currently, only Win8 uses this indication function
// this function should move to N63C
VOID
PlatformIndicateP2PEvent(
	PVOID pvP2PInfo, 
	u4Byte EventID,
	PMEMORY_BUFFER pInformation
) 
{
	N63CIndicateP2PEvent(pvP2PInfo, EventID, pInformation);
}
//
// Description:
//	Indicate the specific (non-common for all platform) event/status to the target.
//	We define the target because we may need different indication methods for different
//	target. In Windows, the status may be indicated through NdisMIndicateStatusEx() for
//	Ndis events or CompleteIRP for IRP besed events (such as BT).
// Arguments:
//	[in] pAdapter -
//		The adapter context.
//	[in] event -
//		The event of the indication.
//	[in] target -
//		The receiver for this event. There may be more than 1 targets to receive this event.
//		This parameter should be set a combination for these targets.
//	[in] pInfoBuffer -
//		The pointer to the input information. It can be NULL.
//	[in] InfoBruuferLen -
//		The length in byte for pInfoBuffer.
// Return:
//	It may return RT_STATUS_SUCCESS if this function completes successfully, or RT_STATUS_PENDING
//	for the asynchrous process.
// By Bruce, 2011-06-09.
//
RT_STATUS
PlatformIndicateCustomStatus(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		event,
	IN	u4Byte		target,
	IN	PVOID		pInfoBuffer,
	IN	u4Byte		InfoBruuferLen
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	
	if(target == RT_CUSTOM_INDI_TARGET_NONE)
	{
		RT_TRACE_F(COMP_INDIC, DBG_WARNING, ("No Target (RT_CUSTOM_INDI_TARGET_NONE)!\n"));
		return RT_STATUS_INVALID_PARAMETER;
	}

	if(target & RT_CUSTOM_INDI_TARGET_IHV)
	{
		RT_TRACE_F(COMP_INDIC, DBG_TRACE, ("Indicate Event (0x%08X) to IHV.\n", event));
	}

	if(TEST_FLAG(target, RT_CUSTOM_INDI_TARGET_IRP))
	{
		rtStatus = RT_STATUS_NOT_SUPPORT;
	}

	if(TEST_FLAG(target, RT_CUSTOM_INDI_TARGET_WDI))
	{
		rtStatus = WDI_IndicateGeneralEvent(pAdapter, event, pInfoBuffer, InfoBruuferLen);
	}

	return rtStatus;
}

RT_STATUS
PlatformIndicateActionFrame(
	IN	PADAPTER		pAdapter,
	IN	PVOID			posMpdu
	)
{
	return WDI_IndicateActionFrame(pAdapter, (POCTET_STRING)posMpdu);
}

VOID
PlatformIndicatePMWakeReason(
	IN PADAPTER		Adapter,
	IN BOOLEAN		bWakePacket,
	IN pu1Byte		pBuffer,
	IN u2Byte		BufferLen
)
{
	if(OS_SUPPORT_WDI(Adapter))
	{
	WDI_IndicateWakeReason(Adapter, bWakePacket, pBuffer, BufferLen);
}
	else
	{
		PNDIS_PM_WAKE_REASON	pPMWakeReason;
		PRT_GEN_TEMP_BUFFER pGenBufPMWakeReason;
		u4Byte		u4bTmp;
		u2Byte		WakeReasonInfoLen;
		PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
		PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
		BOOLEAN		bIndicate = TRUE;

		WakeReasonInfoLen = sizeof(NDIS_PM_WAKE_REASON) + BufferLen;

		pGenBufPMWakeReason = GetGenTempBuffer (Adapter, WakeReasonInfoLen);
		pPMWakeReason = (NDIS_PM_WAKE_REASON *)pGenBufPMWakeReason->Buffer.Ptr;

		PlatformZeroMemory(pPMWakeReason, WakeReasonInfoLen);

		// Header
		pPMWakeReason->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
		pPMWakeReason->Header.Revision = NDIS_PM_WAKE_REASON_REVISION_1;
		pPMWakeReason->Header.Size = NDIS_SIZEOF_PM_WAKE_REASON_REVISION_1;

		// Set wake packet buffer offset.
		if(bWakePacket)
		{
			pPMWakeReason->WakeReason = NdisWakeReasonPacket;
			pPMWakeReason->InfoBufferOffset = sizeof(NDIS_PM_WAKE_REASON);
			pPMWakeReason->InfoBufferSize = BufferLen;
			// Prefast warning C6305: Potential mismatch between sizeof and countof quantities. Use sizeof() to scale byte sizes
			// False positive, this one should be safe and disable it here.
#pragma warning ( disable:6305 )
			PlatformMoveMemory((pPMWakeReason+sizeof(NDIS_PM_WAKE_REASON)), pBuffer, BufferLen);

			RT_PRINT_DATA(COMP_POWER, DBG_LOUD, "IndicatePMWakeReason(): packet\n",(pPMWakeReason+sizeof(NDIS_PM_WAKE_REASON)), BufferLen);
		}
		else
		{
			if(pPSC->WakeUpReason == WOL_REASON_AP_LOST ||
				pPSC->WakeUpReason == WOL_REASON_DEAUTH ||
				pPSC->WakeUpReason == WOL_REASON_DISASSOC)
			{
				pPMWakeReason->WakeReason = NdisWakeReasonWlanAPAssociationLost;
			}
			else if(pPSC->WakeUpReason == WOL_REASON_NLO_SSID_MATCH)
			{
				pPMWakeReason->WakeReason = NdisWakeReasonWlanNLODiscovery;
			}
			else if(pPSC->WakeUpReason == WOL_REASON_PTK_UPDATE)
			{
				pPMWakeReason->WakeReason = NdisWakeReasonWlan4WayHandshakeRequest;
			}
			else
			{
				pPMWakeReason->WakeReason = NdisWakeReasonUnspecified;
				bIndicate = FALSE;
			}
		}
		
		if(bIndicate)
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("IndicatePMWakeReason(): WakeReason(%#X)\n", pPMWakeReason->WakeReason));

			N6IndicateStatus(
				Adapter,
				NDIS_STATUS_PM_WAKE_REASON,
				pPMWakeReason,
				WakeReasonInfoLen);
		}	

		ReturnGenTempBuffer(Adapter, pGenBufPMWakeReason);
	}
}

//
// Description: Starting with NDIS 6.30, MiniPortCheckForHang function must not be registered 
//		for drivers running on low power SoC platforms to avoid negative power impact caused
// 		by the periodic Check-for-Hang activity. <MSDN>
//		So we should set watchdog timer by driver to support on low power SoC platforms.
//
// 2013.09.13, by tynli.
//
VOID
PlatformSetCheckForHangTimer(
	IN PADAPTER		Adapter
)
{
	RT_TRACE(COMP_INIT, DBG_TRACE, ("PlatformSetCheckForHangTimer() ===>\n"));
	Adapter->MgntInfo.bSetWatchDogTimerByDriver = TRUE;
	PlatformSetTimer(Adapter, &Adapter->MgntInfo.WatchDogTimer , RT_CHECK_FOR_HANG_PERIOD * 1000);
}


