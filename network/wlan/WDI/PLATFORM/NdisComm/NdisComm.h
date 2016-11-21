#ifndef __INC_NDIS_COMMON_H
#define __INC_NDIS_COMMON_H


#define N6_LLC_SIZE			8
#define MAX_STALL_TIME		50		//us. If the delay time is larger than 50us, it's better to call NdisMSleep 


#define NDIS_ADAPTER_TYPE RT_SDIO_DEVICE
#define PNDIS_ADAPTER_TYPE PRT_SDIO_DEVICE
#define GET_NDIS_ADAPTER(_Adapter)	&_Adapter->NdisSdioDev

#ifndef	MAX_PATH
#define	MAX_PATH	260		// From DDK.
#endif // #ifndef	MAX_PATH


#define REGISTRY(_Adapter, _VAR)		(_Adapter->pNdisCommon->##_VAR)

#define SetRegLdpc(_Adapter, _RegLdpc)	(_Adapter->pNdisCommon->RegLdpc= _RegLdpc)
#define SetRegStbc(_Adapter, _RegStbc)	(_Adapter->pNdisCommon->RegStbc= _RegStbc)


//
// <Roger_DBG> Using following state flag to trace vWiFi and power transaction event for some vWiFi bus BSOD issue.
// 2013.1213
//
#define DrvStateNull				0
#define DrvStateNdisInitialized	BIT0
#define DrvStateHwInitialized		BIT1
#define DrvStateHwHalting		BIT2
#define DrvStateHwHalted			BIT3
#define DrvStateMacCreating		BIT4
#define DrvStateMacCreated		BIT5
#define DrvStateMacDeleting		BIT6
#define DrvStateMacDeleted		BIT7

//
// Following flags are abnormal cases
//
#define DrvStateMacCreate_NO_DO			BIT8
#define	DrvStateMacCreate_TMR_FAIL		BIT9
#define	DrvStateMacCreate_Fail			BIT10
#define DrvStateMacDelete_NO_DO			BIT11
#define	DrvStateMacDelete_TMR_FAIL		BIT12
#define	DrvStateMacDelete_Fail			BIT13
#define	DrvStateMacCreDel_WI_FAIL		BIT14

#define	DrvStateMacCreatDispatch		BIT15
#define	DrvStateMacCreatWait			BIT16

#define DrvStateNdisReqPended			BIT22
#define DrvStateNdisReqPended_Error0	BIT23
#define DrvStateMacPendReq_Error0		BIT24
#define DrvStateMacPendReq_Error1		BIT25
#define DrvStateMacPendReq_Error2		BIT26
#define DrvStateResetting					BIT27

#define DrvStateWI_Error0				BIT28
#define DrvStateWI_Error1				BIT29
#define DrvStateWI_Error2				BIT30
#define DrvStateWI_Error3				BIT31

#if DRV_LOG_REGISTRY
#define RT_GET_DRV_STATE(__pAdapter)				(GetDefaultAdapter(__pAdapter)->DrvState)
#define RT_GET_DRV_STATE_ERROR(__pAdapter)		(GetDefaultAdapter(__pAdapter)->bDrvStateError)
#define RT_CLEAR_DRV_STATE_ERROR(__pAdapter)		(GetDefaultAdapter(__pAdapter)->bDrvStateError = FALSE)

// Reset all driver state for initialization
#define RT_INIT_DRV_STATE(__pAdapter)\
{\
	u4Byte	DrvState;\
	RT_TRACE(COMP_INIT, DBG_TRACE, ("RT_INIT_DRV_STATE: \n"));\
	PlatformAcquireSpinLock(__pAdapter, RT_DRV_STATE_SPINLOCK);\
	GetDefaultAdapter(__pAdapter)->DrvState= DrvStateNull;\
	DrvState = RT_GET_DRV_STATE(__pAdapter);\
	PlatformReleaseSpinLock(__pAdapter, RT_DRV_STATE_SPINLOCK);\
	if(N6WriteRegDriverState(__pAdapter, DrvState) == NDIS_STATUS_FAILURE)\
	{\
		GetDefaultAdapter(__pAdapter)->bDrvStateError = TRUE;\
	}\
}

// Set specific driver state
#define RT_SET_DRV_STATE(__pAdapter, __StaBits)	\
{	\
	u4Byte	DrvState;\
	RT_TRACE(COMP_INIT, DBG_TRACE, ("RT_SET_DRV_STATE: %#x\n", __StaBits));\
	PlatformAcquireSpinLock(__pAdapter, RT_DRV_STATE_SPINLOCK);\
	(GetDefaultAdapter(__pAdapter))->DrvState |= ((RT_DF_TYPE)(__StaBits));\
	DrvState = RT_GET_DRV_STATE(__pAdapter);\
	PlatformReleaseSpinLock(__pAdapter, RT_DRV_STATE_SPINLOCK);\
	if(N6WriteRegDriverState(__pAdapter, DrvState) == NDIS_STATUS_FAILURE)\
	{\
		GetDefaultAdapter(__pAdapter)->bDrvStateError = TRUE;\
	}\
}

// Clear corresponding driver state
#define RT_CLEAR_DRV_STATE(__pAdapter, __StaBits)\
{\
	u4Byte	DrvState;\
	RT_TRACE(COMP_INIT, DBG_TRACE, ("RT_CLEAR_DRV_STATE: %#x\n", __StaBits));\
	PlatformAcquireSpinLock(__pAdapter, RT_DRV_STATE_SPINLOCK);\
	(GetDefaultAdapter(__pAdapter))->DrvState &= (~((RT_DF_TYPE)(__StaBits)));\
	DrvState = RT_GET_DRV_STATE(__pAdapter);\
	PlatformReleaseSpinLock(__pAdapter, RT_DRV_STATE_SPINLOCK);\
	if(N6WriteRegDriverState(__pAdapter, DrvState) == NDIS_STATUS_FAILURE)\
	{\
		GetDefaultAdapter(__pAdapter)->bDrvStateError = TRUE;\
	}\
}
#else
#define RT_GET_DRV_STATE(__pAdapter)	0
#define RT_GET_DRV_STATE_ERROR(__pAdapter)	0
#define RT_CLEAR_DRV_STATE_ERROR(__pAdapter)
#define RT_INIT_DRV_STATE(__pAdapter)
#define RT_SET_DRV_STATE(__pAdapter, __StaBits)
#define RT_CLEAR_DRV_STATE(__pAdapter, __StaBits)
#endif


typedef struct  __RT_THREAD_PLATFORM_EXT{
	NDIS_EVENT	CompleteEvent;
	NDIS_EVENT		TrigerEvent;
	u4Byte			Period;
	NDIS_SPIN_LOCK	Lock;


	PKTHREAD		Thread;

}RT_THREAD_PLATFORM_EXT, *PRT_THREAD_PLATFORM_EXT;


// Add for windwos common binary.
typedef struct _NIC_MUTEX
{
    ULONG                   Counter;
    ULONG                   ModuleAndLine;  // useful for debugging

} NIC_MUTEX, *PNIC_MUTEX;

//
// 2011/02/25 MH Moev the global spin lock and and mutex for global variable between two adapter
// we need to define all the MAC and structure for dedicated OS.
//
// Add for 92D Dual MAC communication protection.
typedef struct GLOBAL_SPINLOCK
{
    ULONG                   SpinlockCounter;

    KIRQL                    SpinlockIrql;

} GLOBAL_SPINLOCK, *PGLOBAL_SPINLOCK;



//
// Global Mutex: can only be used at PASSIVE level.
//
#define ACQUIRE_GLOBAL_MUTEX(_MutexCounter)                              \
{                                                               \
    while (NdisInterlockedIncrement((PULONG)&(_MutexCounter)) != 1)\
    {                                                           \
        NdisInterlockedDecrement((PULONG)&(_MutexCounter));        \
        NdisMSleep(10000);                          \
    }                                                           \
}

#define RELEASE_GLOBAL_MUTEX(_MutexCounter)                              \
{                                                               \
    NdisInterlockedDecrement((PULONG)&(_MutexCounter));              \
}

//
// Global Spinlock: can be used at PASSIVE and DISPATCH level.
//

#define ACQUIRE_GLOBAL_SPINLOCK(_pSpinlock)                         \
{                                                                                               \
    KIRQL                    TempIrql;                                                    \
    TempIrql = KeGetCurrentIrql();                                                 \
    if (TempIrql == PASSIVE_LEVEL)                                                  \
    {                                                                                           \
    	  KeRaiseIrql(DISPATCH_LEVEL, &TempIrql);	                          \
    }                                                                                           \
    while (NdisInterlockedIncrement((PULONG)&((_pSpinlock)->SpinlockCounter)) != 1)\
    {                                                                                                             \
        NdisInterlockedDecrement((PULONG)&((_pSpinlock)->SpinlockCounter));        \
        NdisStallExecution(40);                                                                            \
    }                                                                                                             \
    (_pSpinlock)->SpinlockIrql = TempIrql;                                                          \
}

#define RELEASE_GLOBAL_SPINLOCK(_pSpinlock)                                           \
{                                                                                                                \
    KIRQL                    TempIrql;                                                                      \
    TempIrql = (_pSpinlock)->SpinlockIrql;                                                         \
    NdisInterlockedDecrement((PULONG)&((_pSpinlock)->SpinlockCounter));          \
    if (TempIrql == PASSIVE_LEVEL)                                                                  \
        KeLowerIrql(TempIrql);                                                                           \
}


//===============================================
// The following is for Windows pIRP definition
//===============================================
typedef struct _IRP_ENTRY
{
	RT_LIST_ENTRY List;
	PIRP  	          pIrp;
}IRP_ENTRY, *PIRP_ENTRY;


//
// Data structure used in DbgWorkItem related OID.
//
typedef struct _RT8187DBGWIPARAM
{
	ULONG		bDbgActCompleted;
	ULONG		eDbgActType;
	ULONG		IoOffset;
	ULONG		IoValue;	
	ULONG		outLen;
	UCHAR		outBuf[200];
}RT8187DBGWIPARAM, *PRT8187DBGWIPARAM;

//===============================================

NDIS_STATUS
NdisStatusFromRtStatus(
	IN	u4Byte	rtStatus
	);


VOID
Dot11_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
);


VOID
HT_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
);


VOID
VHT_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
);

VOID 
PSC_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
);

VOID 
HAL_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
);

NTSTATUS
PlatformReadCommonDwordRegistry(
	IN	PWCHAR			registryName,
	OUT	pu4Byte			Data);

NTSTATUS
PlatformWriteCommonDwordRegistry(
	IN	PWCHAR			registryName,
	IN	pu4Byte			Data);

NTSTATUS
PlatformWriteBTAntPosDwordRegistry(
	IN	PWCHAR			registryName,
	IN	pu4Byte			Data);

NTSTATUS
PlatformReadBTFWLoaderDwordRegistry(
	IN	PWCHAR			registryName,
	OUT	pu1Byte			resultValue);


#endif
