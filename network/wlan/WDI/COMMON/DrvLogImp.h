#ifndef __INC_DRV_LOG_IMP_H
#define __INC_DRV_LOG_IMP_H

#include "DrvLog.h"

#if DRV_LOG

#define MAX_LOG_DESC_LEN 64
typedef struct _DRV_LOG_TYPE_ATTR_IMP_T
{
	unsigned int	MaxLogCountPwr; // 2's power, reprent for max number of logs of this type kept in driver, e.g. 5 means at most 2^5 logs kept.
	char			Description[MAX_LOG_DESC_LEN]; // Null terminated string.
}DRV_LOG_TYPE_ATTR_IMP_T, *PDRV_LOG_TYPE_ATTR_IMP_T;

typedef struct _DRV_LOG_ATTR_IMP_T
{
	unsigned int	Type; // Key to assoicated with DRV_LOG_TYPE_ATTRIBUTE_T.
	char			Description[MAX_LOG_DESC_LEN]; // Null terminated string.
}DRV_LOG_ATTR_IMP_T, *PDRV_LOG_ATTR_IMP_T;

#define MAX_LOG_BUFFER_LEN 256 // 070227, rcnjko: This is for max IE size is 255.
typedef struct _DRV_LOG_DATA_IMP_T
{
	unsigned int	Id; // Key to associated with DRV_LOG_ATTRIBUTE_T.
	unsigned int	TimeStampLow;
	unsigned int	TimeStampHigh;
	unsigned int	BufferLenUsed; 
	unsigned char	Buffer[MAX_LOG_BUFFER_LEN];
}DRV_LOG_DATA_IMP_T, *PDRV_LOG_DATA_IMP_T;

//
//	Description:
//		ID for each catagory of log. 
//		To add new type, one must also update g_LogTypes[].
//		We will allocate an DRV_LOG_POOL_T object for each type.
//
//
typedef enum _DRV_LOG_TYPE_E
{
	LTYPE_ERROR,
	LTYPE_DOT11D,

	LTYPE_TOTAL_COUNT, // Assumption: Always keep it in last one.
}DRV_LOG_TYPE_E;


//
//	Description:
//		ID for each log instance.
//		To add new log, one MUST also udpate g_LogAttributes[].
//
typedef enum _DRV_LOG_ID_E
{
	LID_DOT11D_RESET,
	LID_DOT11D_GET_COUNTRY_IE,
	LID_DOT11D_ACTIVE_SCAN_CHNL,
	LID_DOT11D_ACTIVE_SCAN_TXPWR,
	LID_DOT11D_PASSIVE_SCAN_CHNL,

	LID_TOTAL_COUNT, // Assumption: Always keep it in last one.
}DRV_LOG_ID_E;

extern DRV_LOG_TYPE_ATTR_IMP_T g_LogTypes[LTYPE_TOTAL_COUNT];
extern DRV_LOG_ATTR_IMP_T g_LogAttributes[LID_TOTAL_COUNT];

typedef struct _DRV_LOG_POOL_T
{
	unsigned int		LogStartIndex;
	unsigned int		LogCountUsed;
	PDRV_LOG_DATA_IMP_T	pLogDataRing;
}DRV_LOG_POOL_T, *PDRV_LOG_POOL_T;

#define GET_DRV_LOG_POOLS_HANDLE(__pAdapter) (DRV_LOG_POOL_T **)((__pAdapter)->ppLogPools)
//#define SET_DRV_LOG_POOLS_HANDLE(__pAdapter, __pTmp) GET_DRV_LOG_POOLS_HANDLE(__pAdapter) = (__pTmp)
#define SET_DRV_LOG_POOLS_HANDLE(__pAdapter, __pTmp) __pAdapter->ppLogPools = (void**)(__pTmp)


//
// Get pointer to specified log pool
//
#define GET_DRV_LOG_POOL(__pAdapter, __idx) (*(GET_DRV_LOG_POOLS_HANDLE(__pAdapter)+(__idx)))

//
// Set up address of specified pool.
//
#define SET_DRV_LOG_POOL(__pAdapter, __idx, __pTmp) GET_DRV_LOG_POOL(__pAdapter, __idx) = (PDRV_LOG_POOL_T)(__pTmp)

//
// Get current number of log of specified type. 
//
#define GetDrvLogCnt(__pAdapter, __eLogType) GET_DRV_LOG_POOL(pAdapter, eLogType)->LogCountUsed


BOOLEAN
AllocDrvLogMemory(
	IN	PADAPTER		pAdapter
	);

VOID
FreeDrvLogMemory(
	IN	PADAPTER		pAdapter
	);

VOID
AddDrvLog(
	IN	PADAPTER		pAdapter,
	IN	DRV_LOG_ID_E	eLogId,
	IN	pu1Byte			pBuffer,
	IN	u4Byte			BufferLen	
	);

BOOLEAN
RemoveDrvLog(
	IN	PADAPTER		pAdapter,
	IN	DRV_LOG_TYPE_E	eLogType,	
	OUT	PDRV_LOG_DATA_T	pDstLogData
	);


#else

#define AllocDrvLogMemory(pAdapter) TRUE 
#define FreeDrvLogMemory(pAdapter) 
#define AddDrvLog(pAdapter, eLogId, pBuffer, BufferLen)  
#define RemoveDrvLog(pAdapter, eLogType, pDstLogData) FALSE 
#define GetDrvLogCnt(__pAdapter, __eLogType) 0 

#endif // #if DRV_LOG

#endif // #ifndef __INC_DRV_LOG_IMP_H
