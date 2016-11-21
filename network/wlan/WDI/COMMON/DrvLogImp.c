//-----------------------------------------------------------------------------
//	File:
//		DrvLogImp.c
//
//	Description:
//		Driver log event mechanism.	
//
//-----------------------------------------------------------------------------


#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "DrvLogImp.tmh"
#endif

#if DRV_LOG


//
//	Description:
//		Table of attribute of type of logs. 
//
//		Note that: 
//		1. the order MUST be the same as enum _DRV_LOG_TYPE_E declared.
//		2. Size of Description must < MAX_LOG_DESC_LEN.
//		
#define DEFAULT_LOG_CNT_PWR 5 // 2^5 = 32 
DRV_LOG_TYPE_ATTR_IMP_T g_LogTypes[LTYPE_TOTAL_COUNT] = 
{
	// MaxLogCountPwr, Description
	{DEFAULT_LOG_CNT_PWR, "Error"}, // LTYPE_ERROR
	{DEFAULT_LOG_CNT_PWR, "DOT11D"}, // LTYPE_DOT11D
};



//
//	Description:
//		Table of attribute of logs support.
//		Each entry contains:
//		- Type: value enum _DRV_LOG_TYPE_E, which declare catagory of this log.
//		- Description: null-terminated char string, one MUST restrict size of it smaller than MAX_LOG_DESC_LEN.
//
//		Note that: 
//		1. the order MUST be the same as enum _DRV_LOG_ID_E declared.
//		2. Size of Description must < MAX_LOG_DESC_LEN.
//
DRV_LOG_ATTR_IMP_T g_LogAttributes[LID_TOTAL_COUNT] = 
{
	// Type, Description 
	{LTYPE_DOT11D, "Entering a regulatory domain"}, // LID_DOT11D_RESET
	{LTYPE_DOT11D, "Get a valid country IE"}, // LID_DOT11D_GET_COUNTRY_IE
	{LTYPE_DOT11D, "Active scan: Channels"}, // LID_DOT11D_ACTIVE_SCAN_CHNL
	{LTYPE_DOT11D, "Active scan: Max Tx Power (dBm)"}, // LID_DOT11D_ACTIVE_SCAN_TXPWR
	{LTYPE_DOT11D, "Passive scan: Channels"}, // LID_DOT11D_PASSIVE_SCAN_CHNL
};




//
//	Description:
//		Allocate memory for driver log mechansim. 
//
//	Assumption:
//		Target address had been initalized to NULL pointer.
//
BOOLEAN
AllocDrvLogMemory(
	IN	PADAPTER		pAdapter
	)
{
	u4Byte i;
	RT_STATUS status;
	PVOID pTmp;
	u4Byte Size;

	//
	// Allocate memory block to store pointers to DRV_LOG_POOL_T objects.
	//
	Size = LTYPE_TOTAL_COUNT * sizeof(PVOID);
	status = PlatformAllocateMemory(pAdapter, &pTmp, Size);
	if( RT_STATUS_SUCCESS != status )
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("AllocDrvLogMemory(): failed to allocate DRV_LOG_POOLS_HANDLE!!!\n"));
		return FALSE;
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("AllocDrvLogMemory(): %p is allocated for DRV_LOG_POOLS_HANDLE\n", pTmp));
		PlatformZeroMemory(pTmp, Size);
		SET_DRV_LOG_POOLS_HANDLE(pAdapter, (DRV_LOG_POOL_T**)pTmp);
	}

	//
	// Allocate DRV_LOG_POOL_T objects.
	//
	for(i = 0; i < (u4Byte)LTYPE_TOTAL_COUNT; i++)
	{
		Size = sizeof(DRV_LOG_POOL_T) + ( sizeof(DRV_LOG_DATA_IMP_T) << g_LogTypes[i].MaxLogCountPwr); 
		status = PlatformAllocateMemory(pAdapter, &pTmp, Size);
		if( RT_STATUS_SUCCESS != status )
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("AllocDrvLogMemory(): failed driver log pool %d!!!\n", i));
			return FALSE;
		}
		else
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("AllocDrvLogMemory(): %p is allocated for driver log pool %d\n", pTmp, i));
			PlatformZeroMemory(pTmp, Size);
			SET_DRV_LOG_POOL(pAdapter, i, pTmp);
			GET_DRV_LOG_POOL(pAdapter, i)->pLogDataRing = (PDRV_LOG_DATA_IMP_T)((pu1Byte)pTmp + sizeof(DRV_LOG_POOL_T)); 

			RT_PRINT_DATA(COMP_INIT, DBG_TRACE, "driver log pool: ", (pu1Byte)pTmp, sizeof(DRV_LOG_POOL_T));
		}
	}
	
	return TRUE;
}


//
//	Description:
//		Free memory block for driver log mechansim. 
//
//	Assumption:
//		Target address is NOT NULL pointer if it had been allocated sucessfully. 
//
VOID
FreeDrvLogMemory(
	IN	PADAPTER		pAdapter
	)
{
	u4Byte i;
	PVOID pTmp;
	u4Byte Size;

	if(GET_DRV_LOG_POOLS_HANDLE(pAdapter) == NULL)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("FreeDrvLogMemory(): DRV_LOG_POOLS_HANDLE is NULL!\n"));
		return;
	}

	//
	// Free DRV_LOG_POOL_T objects.
	//
	for(i = 0; i < (u4Byte)LTYPE_TOTAL_COUNT; i++)
	{
		pTmp = (PVOID)GET_DRV_LOG_POOL(pAdapter, i);
		if(pTmp != NULL)
		{
			Size = sizeof(DRV_LOG_POOL_T) + ( sizeof(DRV_LOG_DATA_IMP_T) << g_LogTypes[i].MaxLogCountPwr); 
			RT_TRACE(COMP_INIT, DBG_LOUD, ("FreeDrvLogMemory(): free %p, log pool %d.\n", pTmp, i));
			PlatformFreeMemory(pTmp, Size);
		}
	}

	//
	// Free memory block to store pointers to DRV_LOG_POOL_T objects.
	//
	Size = LTYPE_TOTAL_COUNT * sizeof(PVOID);
	RT_TRACE(COMP_INIT, DBG_LOUD, ("FreeDrvLogMemory(): free DRV_LOG_POOLS_HANDLE: %p\n", GET_DRV_LOG_POOLS_HANDLE(pAdapter)));
	PlatformFreeMemory(GET_DRV_LOG_POOLS_HANDLE(pAdapter), Size);
	SET_DRV_LOG_POOLS_HANDLE(pAdapter, NULL);
}

//
//	Description:
//		Add one log to corresponding pool.
//
VOID
AddDrvLog(
	IN	PADAPTER		pAdapter,
	IN	DRV_LOG_ID_E	eLogId,
	IN	pu1Byte			pBuffer,
	IN	u4Byte			BufferLen	
	)
{
	DRV_LOG_TYPE_E eLogType = (DRV_LOG_TYPE_E)g_LogAttributes[eLogId].Type;
	PDRV_LOG_POOL_T pLogPool = GET_DRV_LOG_POOL(pAdapter, eLogType); 
	u8Byte CurrTime = PlatformGetCurrentTime();
	u4Byte MaxLogCount = ((u4Byte)1 << g_LogTypes[eLogType].MaxLogCountPwr);
	u4Byte LogCountMask = MaxLogCount - 1;
	PDRV_LOG_DATA_IMP_T pLogData;

	PlatformAcquireSpinLock(pAdapter, RT_LOG_SPINLOCK);

	if( pLogPool->LogCountUsed < MaxLogCount )
	{
		pLogData = pLogPool->pLogDataRing + 
					( (pLogPool->LogStartIndex + pLogPool->LogCountUsed) & LogCountMask );
		pLogPool->LogCountUsed++;
	}
	else
	{ // full! age out oldest one.
		pLogData = pLogPool->pLogDataRing + pLogPool->LogStartIndex;
		pLogPool->LogStartIndex = (pLogPool->LogStartIndex + 1) & LogCountMask;
	}

	pLogData->Id = eLogId;
	pLogData->BufferLenUsed = (BufferLen <= MAX_LOG_BUFFER_LEN) ? BufferLen: MAX_LOG_BUFFER_LEN;
	PlatformMoveMemory(pLogData->Buffer, pBuffer, pLogData->BufferLenUsed);
	pLogData->TimeStampLow = (u4Byte)CurrTime;
	pLogData->TimeStampHigh = (u4Byte)(CurrTime >> 32);

	PlatformReleaseSpinLock(pAdapter, RT_LOG_SPINLOCK);
}


//
//	Description:
//		Retrive one log data from specified pool
//
//	Assumption:
//		pDstLogData	must pointer to a buffer with enough space, 
//		e.g. sizeof(DRV_LOG_DATA_IMP_T).
//
BOOLEAN
RemoveDrvLog(
	IN	PADAPTER		pAdapter,
	IN	DRV_LOG_TYPE_E	eLogType,
	OUT	PDRV_LOG_DATA_T	pDstLogData
	)
{
	PDRV_LOG_POOL_T pLogPool = GET_DRV_LOG_POOL(pAdapter, eLogType); 

	PlatformAcquireSpinLock(pAdapter, RT_LOG_SPINLOCK);
	if(pLogPool->LogCountUsed == 0)
	{
		PlatformReleaseSpinLock(pAdapter, RT_LOG_SPINLOCK);
		return FALSE;
	}
	else
	{
		PDRV_LOG_DATA_IMP_T pSrcLogData = pLogPool->pLogDataRing + pLogPool->LogStartIndex;
		u4Byte MaxLogCount = ((u4Byte)1 << g_LogTypes[eLogType].MaxLogCountPwr);
		u4Byte LogCountMask = MaxLogCount - 1;

		PlatformMoveMemory(
			pDstLogData, 
			pSrcLogData, 
			sizeof(DRV_LOG_DATA_T) + pSrcLogData->BufferLenUsed);
		pLogPool->LogStartIndex = (pLogPool->LogStartIndex + 1) & LogCountMask;
		pLogPool->LogCountUsed--;

		PlatformReleaseSpinLock(pAdapter, RT_LOG_SPINLOCK);
		return TRUE;
	}
}


#endif
