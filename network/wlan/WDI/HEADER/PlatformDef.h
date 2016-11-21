#ifndef __INC_PLATFORMDEF_H
#define __INC_PLATFORMDEF_H

// Forward declaration.
typedef struct _RT_FILE_HANDLER RT_FILE_HANDLER, *PRT_FILE_HANDLER;

typedef enum _PRE_AUTH_INDICATION_REASON  PRE_AUTH_INDICATION_REASON;  //xcode c++ compiler error

typedef struct _ADAPTER ADAPTER, *PADAPTER;

//

// If the destination buffer size is smaller than the source, only copy the last string to destination.
#define	COPY_ANSI_LAST_STRING(__pDest, __MaxSize, __pSource)	\
	memcpy(__pDest, &(__pSource[(strlen(__pSource) > (__MaxSize - 1)) ? (strlen(__pSource) - (__MaxSize - 1)) : 0]), (strlen(__pSource) > (__MaxSize - 1)) ? (__MaxSize - 1) : strlen(__pSource) + 1)

// =================================================================================== 
//      Define RT_EVENT for all platform here
// ===================================================================================
	typedef NDIS_EVENT	RT_EVENT;
	#define	PLATFORM_INIT_RT_EVENT(__RtEvent)					NdisInitializeEvent(&(__RtEvent))
	#define	PLATFORM_FREE_RT_EVENT(__RtEvent)				// No Need to Free the Event in Windows
	#define	PLATFORM_SET_RT_EVENT(__RtEvent)					NdisSetEvent(&(__RtEvent))
	#define	PLATFORM_RESET_RT_EVENT(__RtEvent)				NdisResetEvent(&(__RtEvent))
	#define	PLATFORM_WAIT_RT_EVENT(__RtEvent, __MsToWait)	NdisWaitEvent(&(__RtEvent), (__MsToWait))

// ===================================================================================


// =================================================================================== 
//      Define RT_SPIN_LOCK for all platform here
// ===================================================================================

	typedef NDIS_SPIN_LOCK	RT_SPIN_LOCK;
	#define	PLATFORM_INIT_RT_SPINLOCK(__RtSpinLock)		NdisAllocateSpinLock(&(__RtSpinLock))
	#define	PLATFORM_FREE_RT_SPINLOCK(__RtSpinLock)		NdisFreeSpinLock(&(__RtSpinLock))
	#define	PLATFORM_ACQUIRE_RT_SPINLOCK(__RtSpinLock)	NdisAcquireSpinLock(&(__RtSpinLock))
	#define	PLATFORM_RELEASE_RT_SPINLOCK(__RtSpinLock)	NdisReleaseSpinLock(&(__RtSpinLock))
	
// ===================================================================================



#define		RT_SSID_LENGTH				36


	#define PlatformEFIOWrite1Byte(_a,_b,_c)		\
			PlatformIOWrite1Byte(_a,WLAN_IOREG_DEVICE_ID,_b,_c)
	#define PlatformEFIOWrite2Byte(_a,_b,_c)		\
			PlatformIOWrite2Byte(_a,WLAN_IOREG_DEVICE_ID,_b,EF2Byte(_c))
	#define PlatformEFIOWrite4Byte(_a,_b,_c)		\
			PlatformIOWrite4Byte(_a,WLAN_IOREG_DEVICE_ID,_b,EF4Byte(_c))
	#define PlatformEFIOSyncWriteNByte(_a,_b,_c,_d)		\
			PlatformIOSyncWriteNByte(_a,WLAN_IOREG_DEVICE_ID,_b,_c,_d)		

	
		#define PlatformEFIORead1Byte(_a,_b)		\
				PlatformIORead1Byte(_a,WLAN_IOREG_DEVICE_ID,_b)
		#define PlatformEFIORead2Byte(_a,_b)		\
				EF2Byte(PlatformIORead2Byte(_a,WLAN_IOREG_DEVICE_ID,_b))
		#define PlatformEFIORead4Byte(_a,_b)		\
				EF4Byte(PlatformIORead4Byte(_a,WLAN_IOREG_DEVICE_ID,_b))
		#define PlatformEFIOReadNByte(_a,_b,_c,_d)		\
				PlatformIOReadNByte(_a,WLAN_IOREG_DEVICE_ID,_b,_c,_d)
	
#define GET_SHARED_MEMORY_WITH_OFFSET(_src, _dest, _offset, _length)		\
			(_dest)->VirtualAddress=(_src)->VirtualAddress + (_offset);			\
			(_dest)->PhysicalAddressHigh=(_src)->PhysicalAddressHigh;			\
			(_dest)->PhysicalAddressLow=(_src)->PhysicalAddressLow + (_offset);	\
			if ((_dest)->PhysicalAddressLow < (_src)->PhysicalAddressLow)			\
				(_dest)->PhysicalAddressHigh++;								\
			(_dest)->Length=(_length);

#define MAKE_SHARED_MEMORY_OFFSET_AT_FRONT(_buf, _offset)				\
			{																\
				u4Byte AddrLow;												\
				AddrLow=(_buf)->PhysicalAddressLow;							\
				(_buf)->VirtualAddress +=_offset;								\
				(_buf)->PhysicalAddressLow += _offset;							\
				if((_buf)->PhysicalAddressLow < AddrLow)						\
					(_buf)->PhysicalAddressHigh++;								\
				(_buf)->Length-=_offset;										\
			}

#define RT_TIMER_STATUS_INITIALIZED		BIT0
#define RT_TIMER_STATUS_SET				BIT1
#define RT_TIMER_STATUS_PERIODIC		BIT2
#define RT_TIMER_STATUS_FIRED			BIT3 // This timer had been fired.
#define RT_TIMER_STATUS_CANCEL_NG		BIT4 // Timer cannot be canceled.
#define	RT_TIMER_STATUS_RELEASED		BIT5 // TImer had been released.
#define RT_TIMER_STATUS_SUSPENDED		BIT6 // Timer has been canceled and suspended till resumption


#define RT_THREAD_STATUS_INITIALIZED		BIT0
#define RT_THREAD_STATUS_SET			BIT1
#define RT_THREAD_STATUS_PERIODIC		BIT2
#define RT_THREAD_STATUS_FIRED			BIT3 // This timer had been fired.
#define RT_THREAD_STATUS_CANCEL			BIT4 // Timer cannot be canceled.
#define RT_THREAD_STATUS_CANCEL_NG		BIT5 // Timer cannot be canceled.
#define RT_THREAD_STATUS_RELEASED		BIT6 // TImer had been released.


typedef  	UCHAR	RT_THREAD_LEVEL;

#define RT_THREAD_LEVEL_PASSIVE		BIT0
#define RT_THREAD_LEVEL_APC			BIT1
#define RT_THREAD_LEVEL_DISPATCH	BIT2

//
// PlatformSleepUs() will put the thread into wait queue for specified period 
// in us and OS can reschedule now.
// So, we MUST make sure we are in proper context to use this function.
//

	#define PlatformSleepUs					NdisMSleep

	#define INT8_C(v)    (v)
	#define INT16_C(v)   (v)
	#define INT32_C(v)   (v)
	#define INT64_C(v)   (v)

	#define UINT8_C(v)   (v)
	#define UINT16_C(v)  (v)
	#define UINT32_C(v)  (v)
	#define UINT64_C(v)  (v)

	#define INTMAX_C(v)  (v)
	#define UINTMAX_C(v) (v)
	
	#define i64fmt		"I64"
	
// Macro for getting Ibss Addition IEs. By Bruce, 2007-08-13.

	#define	GetIbssAdditionalData(_pAdapter, _pOctetString)												\
			if(((_pAdapter)->pNdisCommon->dot11IbssParams.AdditionalIESize) < 0xFFFF)					\
			{																							\
				(_pOctetString)->Octet = (pu1Byte)((_pAdapter)->pNdisCommon->dot11IbssParams.AdditionalIEData);	\
				(_pOctetString)->Length = (u2Byte)((_pAdapter)->pNdisCommon->dot11IbssParams.AdditionalIESize);	\
			}																							\
			else																						\
			{																							\
				(_pOctetString)->Octet = NULL;															\
				(_pOctetString)->Length = 0;															\
			}

	#define GetIbssbJoinOnly(	_pAdapter)	(_pAdapter->pNdisCommon->dot11IbssParams.bDot11IbssJoinOnly)			
	//
	// Note:
	//	According to Ndis6 WDK, this default value should be "FALSE", and XP in DTM should be
	//	"TRUE" because it doesn't support this mode.
	// By Bruce, 2008-06-27.
	#define	DEFAULT_HIDDEN_SSID		(FALSE)

#ifndef PCI_TYPE1_ADDRESS
	#define PCI_CONF_ADDRESS   0x0CF8   // PCI Configuration Space Address 
	#define PCI_CONF_DATA		0x0CFC   // PCI Configuration Space Data 
#endif


// Place Holder for Private Data ---------------------
typedef u1Byte PRIVATE_DATA_ZONE;
// ---------------------------------------------

// Vaild Pairs ------------------------------------------------------------------
typedef struct _RT_VALID_PAIR_BOOLEAN
{
	BOOLEAN 	bVaild;
	BOOLEAN 	Data;
	
} RT_VALID_PAIR_BOOLEAN, *PRT_VALID_PAIR_BOOLEAN; 

typedef struct _RT_VALID_PAIR_U4BYTE
{
	BOOLEAN 	bVaild;
	u4Byte		Data;
	
} RT_VALID_PAIR_U4BYTE, *PRT_VALID_PAIR_U4BYTE;
// ---------------------------------------------------------------------------


typedef struct _MEMORY_BUFFER {
	pu1Byte	Buffer;
	u4Byte	Length;
}MEMORY_BUFFER, *PMEMORY_BUFFER;

typedef struct _VIRTUAL_MEMORY{
	PVOID				Ptr;
	u4Byte				Length;
}VIRTUAL_MEMORY,*PVIRTUAL_MEMORY;
	
typedef struct _PHYSICAL_MEMORY{
	u4Byte				AddressLow;
	u4Byte				AddressHigh;
	u4Byte				Length;
}PHYSICAL_MEMORY,*PPHYSICAL_MEMORY;

typedef struct _SHARED_MEMORY{
	pu1Byte			VirtualAddress;
	u4Byte			PhysicalAddressHigh;
	u4Byte			PhysicalAddressLow;
	u4Byte			Length;
}SHARED_MEMORY,*PSHARED_MEMORY;

typedef struct _ALIGNED_SHARED_MEMORY{
	SHARED_MEMORY	OriginalSharedMemory;
	pu1Byte			VirtualAddress;
	u4Byte			PhysicalAddressHigh;
	u4Byte			PhysicalAddressLow;
	u4Byte			Length;
}ALIGNED_SHARED_MEMORY,*PALIGNED_SHARED_MEMORY;



typedef VOID
(*RT_THREAD_CALL_BACK)(
	PVOID   Adapter
	);

typedef struct __RT_THREAD
{
	PADAPTER					Adapter;
	u1Byte					Status;
	PVOID					pPlatformExt;
	PVOID					pContext;
	RT_THREAD_CALL_BACK		CallbackFunc;	
	RT_THREAD_CALL_BACK		PreTheradExitCb;
	u2Byte					RefCnt;
	u2Byte					ScheduleCnt;	// only for debug information
	BOOLEAN					bEventTriger;
	char						szID[36]; 
	u1Byte					Argc;
	u1Byte					Argv[1];
}RT_THREAD, *PRT_THREAD;


typedef VOID
(*RT_TIMER_CALL_BACK)(
	PVOID			pRtTimer
	);

typedef struct _RT_TIMER{
	RT_TIMER_HANDLE		Handle; // Platform-dependent handle for this workitem, e.g. Ndis Workitem object.
	PVOID			Adapter; // Pointer to Adapter object.
	RT_TIMER_CALL_BACK	CallBackFunc; // Callback function of the workitem.
	u4Byte			Status; // Status of the timer. see RT_TIMER_STATUS_XXX for possible value.
	u4Byte			PreviousStatus; // Previous status of the timer, i.e., status before timer suspended
	u4Byte			msDelay; // The interval ms to call CallBackFunc after setting timer. Inialize as 0 and will be udpated when timer set.
	PVOID			Context; // Timer specific context.
	u2Byte			RefCnt; // Per timer scheduled counter. by Roger ,2007.04.27.
	u2Byte			TimerStallSlotCount; // For Timer stall checking mechanism. Added by Roger, 2007.04.27.
	char				szID[36]; // An identity string of this timer.
}RT_TIMER, *PRT_TIMER;

typedef VOID
(*RT_WORKITEM_CALL_BACK)(
	PVOID			pContext
	);

typedef struct _RT_WORK_ITEM
{
	RT_WORKITEM_HANDLE			Handle;			// Platform-dependent handle for this workitem, e.g. Ndis Workitem object.
	PVOID						Adapter;		// Pointer to Adapter object.
	PVOID						pContext;		// Parameter to passed to CallBackFunc(). 
	RT_WORKITEM_CALL_BACK		CallbackFunc;	// Callback function of the workitem.
	u1Byte						RefCount;		// 0: driver is going to unload, 1: No such workitem scheduled, 2: one workitem is schedueled. 
	PVOID						pPlatformExt;	// Pointer to platform-dependent extension.	
	BOOLEAN						bFree;
	char						szID[36];		// An identity string of this workitem.
}RT_WORK_ITEM, *PRT_WORK_ITEM;


typedef enum _RT_MEDIA_STATUS{
	RT_MEDIA_DISCONNECT = 0,
	RT_MEDIA_CONNECT       = 1
}RT_MEDIA_STATUS;


typedef enum _RT_SPINLOCK_TYPE{
	RT_TX_SPINLOCK = 1,
	RT_RX_SPINLOCK = 2,
	RT_RM_SPINLOCK = 3,
	RT_CAM_SPINLOCK = 4,
	RT_SCAN_SPINLOCK = 5,
	RT_LOG_SPINLOCK = 7, 
	RT_BW_SPINLOCK = 8,
	RT_CHNLOP_SPINLOCK = 9,
	RT_RF_OPERATE_SPINLOCK = 10,
	RT_INITIAL_SPINLOCK = 11,
	RT_RF_STATE_SPINLOCK = 12, // For RF state. Added by Bruce, 2007-10-30.
#if VISTA_USB_RX_REVISE
	RT_USBRX_CONTEXT_SPINLOCK = 13,
	RT_USBRX_POSTPROC_SPINLOCK = 14, // protect data of Adapter->IndicateW/ IndicateR
#endif
	//Shall we define Ndis 6.2 SpinLock Here ?
	RT_PORT_SPINLOCK=16,
	RT_H2C_SPINLOCK = 20, // For H2C cmd. Added by tynli. 2009.11.09.
	RT_IOTHREAD_SPINLOCK = 21,
	RT_BTData_SPINLOCK=25,

	RT_WAPI_OPTION_SPINLOCK=26,
	RT_WAPI_RX_SPINLOCK=27,

      // add for 92D CCK control issue  
	RT_CCK_PAGEA_SPINLOCK = 28,
	RT_BUFFER_SPINLOCK = 29,
	RT_CHANNEL_AND_BANDWIDTH_SPINLOCK = 30,
	RT_GEN_TEMP_BUF_SPINLOCK = 31,
	RT_AWB_SPINLOCK = 32,
	RT_FW_PS_SPINLOCK = 33,
	RT_HW_TIMER_SPIN_LOCK = 34,
	RT_MPT_WI_SPINLOCK = 35,
	RT_P2P_SPIN_LOCK = 36,	// Protect P2P context
	RT_DBG_SPIN_LOCK = 37,
	RT_IQK_SPINLOCK = 38,
	RT_PENDED_OID_SPINLOCK = 39,
	RT_CHNLLIST_SPINLOCK = 40,	
	RT_INDIC_SPINLOCK = 41,		//protect indication		
	RT_DRV_STATE_SPINLOCK = 42,
	RT_RFD_SPINLOCK = 43,	
	RT_ACS_SPINLOCK = 44,	// Auto Channel Switching
	RT_RX_REF_CNT_SPINLOCK = 45,
	RT_SYNC_IO_CNT_SPINLOCK = 46,
	RT_CUSTOM_SCAN_SPINLOCK = 47,
	RT_RX_CONTROL_SPINLOCK = 48,
	RT_RX_QUEUE_SPINLOCK = 49,
	RT_DYN_TXPWRTBL_SPINLOCK = 50,
	RT_LAST_SPINLOCK,
}RT_SPINLOCK_TYPE;

typedef enum _RT_EVENTLOG_TYPE{
	RT_FW_DOWLOAD_F = 0,
	RT_INIT_HANDLER_FAIL = 1,
	RT_INIT_FAIL = 2,
	RT_MUTUAL_AUTHENTICATION_FAIL=3,
	RT_INIT_OK			=4,
	RT_SCAN_START		=5,
	RT_SCAN_COMPLETE	=6,
	RT_SCAN_NUM		=7,
	RT_TKIP_MIC_ERROR	= 8,
	RT_HW_AIRPLANE_MODE_TRIGGERED = 9,
}RT_EVENTLOG_TYPE;

// DMA buffer catch sync diection
typedef enum _RT_DMA_DIRECTION {
	RT_DMA_BIDIRECTIONAL=0,
	RT_DMA_TO_DEVICE=1,
	RT_DMA_FROM_DEVICE=2,
	RT_DMA_NONE
} RT_DMA_DIRECTION, *PRT_DMA_DIRECTION;

typedef enum _RT_RX_INDICATION_LEVEL {
	RT_RX_INDICATION_DPC, 		//In a DPC
	RT_RX_INDICATION_GENERAL,	//Include DISPATCH/PASSIVE level
	RT_RX_INDICATION_RESUME		//WDI resume Rx
} RT_RX_INDICATION_LEVEL;

//
// Sinda, 20150511
// Redefine CCX_ASOC_REASON for prepare roaming type in Win10 to decide to skip scan from WDI or not.
//
typedef enum _RT_PREPARE_ROAM_TYPE{
	RT_PREPARE_ROAM_UNSPECIFIED = 0,
	RT_PREPARE_ROAM_NORMAL_ROAM_POOR_LINK = 1,
	RT_PREPARE_ROAM_NORMAL_ROAM_LOAD_BALANCING = 2,
	RT_PREPARE_ROAM_INSUFFICIENT_CAPACITY = 3,
	RT_PREPARE_ROAM_DIRECT_ROAM = 4,
	RT_PREPARE_ROAM_FIRST_ASSOCIATION = 5,
	RT_PREPARE_ROAM_ROAM_FROM_WAN = 6,
	RT_PREPARE_ROAM_ROAM_TO_WAN = 7,
	RT_PREPARE_ROAM_NORMAL_ROAM_BETTER_AP = 8,
	RT_PREPARE_ROAM_DEAUTH_DISASSOC = 9,
	RT_PREPARE_ROAM_NONE,
} RT_PREPARE_ROAM_TYPE, *PRT_PREPARE_ROAM_TYPE;


/*
 *	Note:	1. Each platform should have its implementation of all following functions.
 *			2. PlatformEFIOxxxxxxByte is used to read/write integer to hardware
 *			3. PlatformIOxxxxxxByte is used to read/write raw data
 *
*/

VOID
PlatformIOWrite1Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u1Byte		data
	);

VOID
PlatformIOWrite2Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u2Byte		data
	);

VOID
PlatformIOWrite4Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u4Byte		data
	);

VOID
PlatformIOSyncWriteNByte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u4Byte		count,
	pu1Byte		pdata
	);

VOID
PlatformIOWriteNByte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset,
	u4Byte		count,
	pu1Byte		pdata
	);

u1Byte
PlatformIORead1Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset
	);

u2Byte
PlatformIORead2Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset
	);

u4Byte
PlatformIORead4Byte(
	PVOID		Adapter,
	u1Byte		DeviceID,
	u4Byte		offset
	);

VOID
PlatformIOReadNByte(
	IN	PVOID	Adapter,
	IN	u1Byte	DeviceID,
	IN	u4Byte	offset,
	IN	u4Byte	count,
	OUT pu1Byte	pBuffer
	);

RT_STATUS
PlatformAllocateMemoryWithTag(
	IN      PVOID		Adapter,
	IN      u4Byte      Tag,
	OUT     PVOID		*pPtr,
	IN      u4Byte		length
	);
    
VOID
PlatformFreeMemory(
	IN PVOID		ptr,
	IN u4Byte		length
	);

RT_STATUS
PlatformAllocateMemoryWithZero(
							   IN	PVOID		Adapter,
							   OUT	PVOID		*pPtr,
							   IN	u4Byte		length
							   );

#define RtDbgInitMem()
#define RtDbgDeinitMem()
#define	PlatformAllocateMemory(__Adapter, __Ptr, __Len) \
			PlatformAllocateMemoryWithTag(__Adapter, GenTag(__FUNCTION__), __Ptr, __Len)
#define	PlatformFreeMemory(ptr, length) \
			PlatformFreeMemory(ptr, length)			
#define	PlatformAllocateMemoryWithZero(__Adapter, __Ptr, __Len) \
			PlatformAllocateMemoryWithZero(__Adapter, __Ptr, __Len)



RT_STATUS
PlatformAllocateSharedMemory(
	PVOID				Adapter,
	PSHARED_MEMORY		pSharedMemory,
	u4Byte				length
	);

VOID
PlatformFreeSharedMemory(
	PVOID				Adapter,
	PSHARED_MEMORY		pSharedMemory
	);

RT_STATUS
PlatformAllocateAlignedSharedMemory(
	PVOID						Adapter,
	PALIGNED_SHARED_MEMORY	pAlignedSharedMemory,
	u4Byte						length
	);

VOID
PlatformFreeAlignedSharedMemory(
	PVOID						Adapter,
	PALIGNED_SHARED_MEMORY	pAlignedSharedMemory
	);

VOID
PlatformZeroMemory(
	PVOID		ptr,
	u4Byte		length
	);

VOID
PlatformMoveMemory(
	PVOID		pDest,
	PVOID		pSrc,
	u4Byte		length
	);

s4Byte
PlatformCompareMemory(
	PVOID		pBuf1,
	PVOID		pBuf2,
	u4Byte		length
	);

VOID
PlatformFillMemory(
	PVOID		Buf,
	u4Byte		Length,
	u1Byte		Fill
	);

VOID
PlatformStallExecution(
	u4Byte		usDelay
	);
VOID
PlatformForceStallExecution(
	u4Byte		usDelay
	);

#define PlatformAllocateAlignedDmaMemory		PlatformAllocateAlignedSharedMemory
#define PlatformFreeAlignedDmaMemory			PlatformFreeAlignedSharedMemory
#define PlatformDmaMapSingle
#define PlatformDmaUnMapSingle
#define smp_wmb()

//
// 2009/04/17 MH Vista timer has s strange condition. If we delay
// more than 50 us in dispatch level. Our driver will be scheduled out and
// then delay time will be more than what we require. For example, delay
// 100us, real delay time will be 15ms!!!
// Be carefule!!!!. We need to use a macro to execute the special delay
// process. If you use a function, Vista will also delay more than what you
// need. You need to plus a useless I/O operation to prevent the strange
// Vista scheduler behavior.
// We can fix SE IPS power on delay time from 200ms to 5ms.
// Test more than 4 days IPS and S3/S4 resume OK on 8 platforms..
//

#define	PLATFORM_STALLEXE_DISPATCH(pAdapter, usDelay)	\
{															\
	u1Byte		u1bTmp, i;									\
	for (i = 0; i < (usDelay/MAX_STALL_TIME); i++)				\
	{														\
		PlatformStallExecution(MAX_STALL_TIME-1);			\
		u1bTmp = PlatformEFIORead1Byte(pAdapter, 0x0);		\
	}														\
}


VOID
PlatformInitializeTimer(
	PVOID				Adapter,
	PRT_TIMER			pTimer,
	RT_TIMER_CALL_BACK	CallBackFunc, 
	PVOID				pContext,
	const char*				szID
	);

BOOLEAN
PlatformSetTimer(
	PVOID				Adapter,
	PRT_TIMER			pTimer,
	u4Byte				msDelay
	);

BOOLEAN
PlatformSetPeriodicTimer(
	PVOID				Adapter,
	PRT_TIMER			pTimer,
	u4Byte				msDelay
	);

BOOLEAN
PlatformCancelTimer(
	PVOID				Adapter,
	PRT_TIMER			pTimer
	);


BOOLEAN
PlatformReleaseTimer(
	PVOID				Adapter,
	PRT_TIMER			pTimer
	);
	
BOOLEAN
PlatformSuspendTimer(
	IN	PVOID				Adapter,
	IN	PRT_TIMER			pTimer
	);

BOOLEAN
PlatformResumeTimer(
	IN	PVOID				Adapter,
	IN	PRT_TIMER			pTimer
	);	

RT_STATUS
PlatformInitializeWorkItem(
	PVOID						Adapter,
	PRT_WORK_ITEM				pRtWorkItem,
	RT_WORKITEM_CALL_BACK		NdisWorkItemCallback,
	PVOID						pContext,
	const char*						szID
);

VOID
PlatformStartWorkItem(
	PRT_WORK_ITEM				pRtWorkItem
);

VOID
PlatformStopWorkItem(
	PRT_WORK_ITEM				pRtWorkItem
);

VOID
PlatformFreeWorkItem(
	PRT_WORK_ITEM				pRtWorkItem
);

BOOLEAN 
PlatformScheduleWorkItem(
	PRT_WORK_ITEM				pRtWorkItem
);

VOID
PlatformLeaveCallbackOfWorkItem(
	PRT_WORK_ITEM	pRtWorkItem
);
	
BOOLEAN
PlatformIsWorkItemScheduled(
	PRT_WORK_ITEM				pRtWorkItem
	);

u8Byte
PlatformGetCurrentTime(
	VOID
	);

VOID
_PlatformInitializeSpinLock(
	PVOID				Adapter,
	RT_SPINLOCK_TYPE	type
	);

VOID
_PlatformFreeSpinLock(
	PVOID				Adapter,
	RT_SPINLOCK_TYPE	type
	);

#pragma warning( disable: 28167 ) // Prefast says we don't annotated the change.
VOID
_IRQL_raises_(DISPATCH_LEVEL)
_PlatformAcquireSpinLock(
	PVOID				Adapter,
	RT_SPINLOCK_TYPE	type
	);

#pragma warning( disable: 28167 ) // Prefast says we don't annotated the change.
VOID
_IRQL_requires_(DISPATCH_LEVEL)
_PlatformReleaseSpinLock(
	PVOID				Adapter,
	RT_SPINLOCK_TYPE	type
	);

#define	RtDbgInitSpinlock()
#define RtDbgDeinitSpinlock()
#define	PlatformInitializeSpinLock(__Adapter, __Type)	\
			_PlatformInitializeSpinLock(__Adapter, __Type)
#define PlatformAcquireSpinLock(__Adapter, __Type)	\
            _PlatformAcquireSpinLock(__Adapter, __Type)
#define PlatformReleaseSpinLock(__Adapter, __Type)	\
			_PlatformReleaseSpinLock(__Adapter, __Type)
#define PlatformFreeSpinLock(__Adapter, __Type)	\
			_PlatformFreeSpinLock(__Adapter, __Type)


u4Byte
PlatformAtomicExchange(
	pu4Byte				target,
	u4Byte				newValue
	);

u4Byte
PlatformAtomicAnd(
	pu1Byte				target,
	u1Byte				value
	);

u4Byte
PlatformAtomicOr(
	pu1Byte				target,
	u1Byte				value
	);

VOID
PlatformInitializeMutex(
	IN PPlatformMutex	Mutex
	);
VOID
PlatformAcquireMutex(
	IN PPlatformMutex	Mutex
	);
VOID
PlatformReleaseMutex(
	IN PPlatformMutex	Mutex
	);
#define	RtDbgInitMutex()
#define RtDbgDeinitMutex()


VOID
PlatformIndicateMediaStatus(
	PVOID				Adapter,
	RT_MEDIA_STATUS	mstatus
	);

RT_STATUS
PlatformIndicateCustomStatus(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		event,
	IN	u4Byte		target,
	IN	PVOID		pInfoBuffer,
	IN	u4Byte		InfoBruuferLen
	);

RT_STATUS
PlatformIndicateActionFrame(
	IN	PADAPTER		pAdapter,
	IN	PVOID			posMpdu
	);

VOID
PlatformIndicatePMWakeReason(
	IN PADAPTER		Adapter,
	IN BOOLEAN		bWakePacket,
	IN pu1Byte		pBuffer,
	IN u2Byte		BufferLen
	);

VOID
PlatformSetCheckForHangTimer(
	IN PADAPTER		Adapter
	);

VOID
PlatformHandleTKIPMICError(
	IN	PVOID			Adapter,
	IN	BOOLEAN			bMcstDest,
	IN	u1Byte			Keyidx,
	IN	pu1Byte			pSrcAddr
	);

VOID
PlatformHandleNLOnScanCancel(
	IN	PVOID			pvAdapter
);

VOID
PlatformHandleNLOnScanComplete(
	IN	PVOID			pvAdapter
);

VOID
PlatformHandleNLOnScanRequest(
	IN	PVOID			pvAdapter
);


VOID
PlatformHandlePwiseKeyUpdata(
	IN	PVOID				Adapter
);

VOID
PlatformRequestPreAuthentication(
	PVOID							Adapter,
	PRE_AUTH_INDICATION_REASON		Reason
	);

RT_STATUS
PlatformReadFile(
	IN		PVOID		Adapter,
	IN		ps1Byte		szFileName,
	IN OUT	pu1Byte		pBufOfLines,
	IN		s4Byte		nMaxNumLine,
	IN		s4Byte		nMaxByteCntLine,
	OUT		ps4Byte		pnNumLines	
	);

RT_STATUS
PlatformOpenFile(
	IN		PVOID		Adapter,
	IN		ps1Byte		szFileName,
	IN OUT	PRT_FILE_HANDLER		pFileHandler
	);

RT_STATUS
PlatformMapFile(
	IN OUT	PRT_FILE_HANDLER	pFileHandler
	);

VOID
PlatformUnMapFile(
	IN OUT	PRT_FILE_HANDLER	pFileHandler
	);

VOID
PlatformCloseFile(
	IN OUT	PRT_FILE_HANDLER	pFileHandler
	);

RT_STATUS
PlatformReadAndMapFile(
	IN		PVOID		Adapter,
	IN		ps1Byte		szFileName,
	IN OUT	pu1Byte		pOutFile,
	IN OUT	pu4Byte	UNALIGNED	pFileSize
	);

BOOLEAN
PlatformIsTxQueueAvailable(
	IN	PADAPTER	Adapter,
	IN	u1Byte		QueueID,
	IN	u2Byte		BufferCount);

VOID
PlatformReleaseDataFrameQueued(
	IN	PADAPTER	Adapter
	);

BOOLEAN
PlatformSetPMCSR(
	IN		PADAPTER	Adapter,
	IN 		u1Byte		value,
	IN		BOOLEAN		bTempSetting
	);

u4Byte
PlatformResetPciSpace(
	IN		PADAPTER	Adapter,
	IN 		u1Byte		Value
	);
	
BOOLEAN
PlatformSwitchClkReq(
	IN		PADAPTER	Adapter,
	IN 		u1Byte		value
	);

BOOLEAN
PlatformSwitchDevicePciASPM(
	IN		PADAPTER	Adapter,
	IN 		u1Byte		value
	);

VOID
PlatformSetPciConfiguration(
	IN		PADAPTER	Adapter
	);


VOID 
PlatformBackupPciCfgSpaceReg(
	IN	PADAPTER Adapter
	);


VOID 
PlatformRestorePciCfgSpaceReg(
	IN	PADAPTER Adapter
	);


VOID
PlatformEnableASPM(
	IN		PADAPTER	Adapter
	);

VOID
PlatformDisableASPM(
	IN		PADAPTER	Adapter
	);

VOID
PlatformDbgYB(
	IN		PADAPTER	Adapter,
	IN		u2Byte		Offset
	);


// Strange function declared here!!
BOOLEAN
PlatformEnable92CEDMA64(
	IN		PADAPTER	Adapter
	);

VOID
PlatformSetPciPMEEnable(
	IN		PADAPTER		Adapter	
	);

VOID
PlatformClearPciPMEStatus(
	IN		PADAPTER		Adapter	
	);

u4Byte
PlatformEnableMSIHandler(
	IN		PADAPTER	Adapter,
	IN 		u1Byte		Value
	);

VOID
PlatformInitializeEvent(
	IN 	PPlatformEvent	pEvent
	);

VOID
PlatformResetEvent(
	IN 	PPlatformEvent	pEvent
	);

VOID
PlatformSetEvent(
	IN	PPlatformEvent	pEvent
	);

VOID
PlatformFreeEvent(
	IN 	PPlatformEvent	pEvent
	);

BOOLEAN
PlatformWaitEvent(
	IN 	PPlatformEvent	pEvent,
	IN	u4Byte			msToWait
	);


#define	PlatformIndicateBTEvent(Adapter, pEvntData, dataLen)			RT_STATUS_FAILURE
#define	PlatformIndicateBTACLData(Adapter, pData, dataLen, EntryNum)	RT_STATUS_FAILURE
#define	PlatformProcessHCICommands(pContext)

u8Byte
PlatformDivision64(
	IN u8Byte	x,
	IN u8Byte	y
);

u8Byte
PlatformModular64(
	IN u8Byte	x,
	IN u8Byte	y
);

VOID
PlatformEnableNetworkMonitorMode(
	IN	PADAPTER	Adapter
	);

VOID
PlatformDisableNetworkMonitorMode(
	IN	PADAPTER	Adapter
	);

VOID
PlatformSetFwPsClkOffEvent(
	IN	PADAPTER	Adapter,
	IN	u1Byte		SetEvent
	);

RT_STATUS
PlatformInitializeThread(
	IN	PADAPTER			Adapter,
	IN	PRT_THREAD			pRtThread,
	IN	RT_THREAD_CALL_BACK	CallBackFunc, 
	IN	const char*			szID,
	IN	BOOLEAN				EventTriger,
	IN	u4Byte				Period,
	IN	PVOID				pContext
	);

RT_STATUS
PlatformInitializeThreadEx(
	IN	PADAPTER			Adapter,
	IN	PRT_THREAD			pRtThread,
	IN	RT_THREAD_CALL_BACK	CallBackFunc, 
	IN  RT_THREAD_CALL_BACK PreThreadExitCb,
	IN	const char*			szID,
	IN	BOOLEAN				EventTriger,
	IN	u4Byte				Period,
	IN	PVOID				pContext
	);

RT_STATUS
PlatformSetEventTrigerThread(
	IN	PADAPTER		Adapter,
	IN	PRT_THREAD	pRtThread,
	IN 	RT_THREAD_LEVEL 	Priority,
	IN	PVOID		pContext
    );

VOID
PlatformSetEventToTrigerThread(
	IN	PADAPTER		Adapter,
	IN	PRT_THREAD	pRtThread
);

VOID
PlatformWaitThreadEnd(
	IN	PADAPTER	pAdapter,
	IN	PRT_THREAD pRtThread);


VOID
PlatformCancelThread(
	IN	PADAPTER	pAdapter,
	IN	PRT_THREAD pRtThread);

RT_STATUS
PlatformRunThread(
	IN	PADAPTER		Adapter,
	IN	PRT_THREAD	pRtThread,
	IN 	RT_THREAD_LEVEL 	Priority,
	IN	PVOID		pContext
    );

VOID
PlatformReleaseThread(
	IN	PADAPTER	pAdapter,
	IN	PRT_THREAD pRtThread);


#define SEMA_UPBND	(0x7FFFFFFF)

VOID
PlatformInitializeSemaphore(	
	IN 	PPlatformSemaphore	Sema,
	IN	u4Byte	InitCnt
	);

RT_STATUS
PlatformAcquireSemaphore(	
	IN 	PPlatformSemaphore	Sema
	);

VOID
PlatformReleaseSemaphore(	
	IN 	PPlatformSemaphore	Sema
	);

VOID
PlatformFreeSemaphore(	
	IN 	PPlatformSemaphore	Sema
	);

VOID
PlatformCriticalSectionLeave(
							 IN PADAPTER			pAdapter,
							 IN PPlatformMutex	Mutex,
							 IN RT_SPINLOCK_TYPE	Spinlock
							 );
VOID
PlatformCriticalSectionEnter(
							 IN PADAPTER			pAdapter,
							 IN PPlatformMutex	Mutex,
							 IN RT_SPINLOCK_TYPE	Spinlock
							 );

VOID
PlatformExtApSupport(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bSupport
	);

BOOLEAN
PlatformInitReady(
	IN	PADAPTER	Adapter
	);


//
// <Roger_Notes> The following Export SDIO Commands are for IO_RW_DIRECT Command (CMD52) 
// and IO_RW_EXTENDED Command (CMD53) to read or write 
// corresponding offset and data for specific device ID.
// 2010.12.23.
//
#define PlatformEFSdioCmd52Read1Byte(_a,_b,_c,_d)		\
			PlatformSdioCmd52Read1Byte(_a,_b,_c,_d)
#define PlatformEFSdioCmd52Read2Byte(_a,_b,_c,_d)		\
			EF2Byte(PlatformSdioCmd52Read2Byte(_a,_b,_c,_d))
#define PlatformEFSdioCmd52Read4Byte(_a,_b,_c,_d)		\
			EF4Byte(PlatformSdioCmd52Read4Byte(_a,_b,_c,_d))
#define PlatformEFSdioCmd52Write1Byte(_a,_b,_c,_d,_e)		\
			PlatformSdioCmd52Write1Byte(_a,_b,_c,_d,_e)
#define PlatformEFSdioCmd52Write2Byte(_a,_b,_c,_d,_e)		\
			PlatformSdioCmd52Write2Byte(_a,_b,_c,EF2Byte(_d), _e)
#define PlatformEFSdioCmd52Write4Byte(_a,_b,_c,_d,_e)		\
			PlatformSdioCmd52Write4Byte(_a,_b,_c,EF4Byte(_d),_e)
#define PlatformEFSdioCmd53Read1Byte(_a,_b,_c)		\
			PlatformIORead1Byte(_a,_b,_c)
#define PlatformEFSdioCmd53Read2Byte(_a,_b,_c)		\
			EF2Byte(PlatformIORead2Byte(_a,_b,_c))
#define PlatformEFSdioCmd53Read4Byte(_a,_b,_c)		\
			EF4Byte(PlatformIORead4Byte(_a,_b,_c))
#define PlatformEFSdioCmd53ReadNByte(_a,_b,_c,_d,_e)		\
			PlatformIOReadNByte(_a,_b,_c,_d,_e)
#define PlatformEFSdioCmd53Write1Byte(_a,_b,_c,_d)		\
			PlatformIOWrite1Byte(_a,_b,_c,_d)
#define PlatformEFSdioCmd53Write2Byte(_a,_b,_c,_d)		\
			PlatformIOWrite2Byte(_a,_b,_c,EF2Byte(_d))
#define PlatformEFSdioCmd53Write4Byte(_a,_b,_c,_d)		\
			PlatformIOWrite4Byte(_a,_b,_c,EF4Byte(_d))

//
// Use CMD52 to perform SDIO Local I/O Bus domain Read/Write for Function 1 space.
//
#define PlatformEFSdioLocalCmd52Read1Byte(_a,_b)		\
			PlatformEFSdioCmd52Read1Byte(_a,SDIO_LOCAL_DEVICE_ID,1,_b)
#define PlatformEFSdioLocalCmd52Read2Byte(_a,_b)		\
			PlatformEFSdioCmd52Read2Byte(_a,SDIO_LOCAL_DEVICE_ID,1,_b)
#define PlatformEFSdioLocalCmd52Read4Byte(_a,_b)		\
			PlatformEFSdioCmd52Read4Byte(_a,SDIO_LOCAL_DEVICE_ID,1,_b)
#define PlatformEFSdioLocalCmd52Write1Byte(_a,_b,_c)		\
			PlatformSdioCmd52Write1Byte(_a,SDIO_LOCAL_DEVICE_ID,1,_b,_c)
#define PlatformEFSdioLocalCmd52Write2Byte(_a,_b,_c)		\
			PlatformSdioCmd52Write2Byte(_a,SDIO_LOCAL_DEVICE_ID,1,_b,EF2Byte(_c))
#define PlatformEFSdioLocalCmd52Write4Byte(_a,_b,_c)		\
			PlatformSdioCmd52Write4Byte(_a,SDIO_LOCAL_DEVICE_ID,1,_b,EF4Byte(_c))
			
//
// Use CMD53 to perform SDIO Local I/O Bus domain Read/Write for Function 1 space.
// We will NOT use CMD53 to access SDIO Function 0 space.
//
#define PlatformEFSdioLocalCmd53Read1Byte(_a,_b)		\
			PlatformEFSdioCmd53Read1Byte(_a,SDIO_LOCAL_DEVICE_ID,_b)
#define PlatformEFSdioLocalCmd53Read2Byte(_a,_b)		\
			PlatformEFSdioCmd53Read2Byte(_a,SDIO_LOCAL_DEVICE_ID,_b)
#define PlatformEFSdioLocalCmd53Read4Byte(_a,_b)		\
			PlatformEFSdioCmd53Read4Byte(_a,SDIO_LOCAL_DEVICE_ID,_b)
#define PlatformEFSdioLocalCmd53ReadNByte(_a,_b,_c,_d)		\
			PlatformEFSdioCmd53ReadNByte(_a,SDIO_LOCAL_DEVICE_ID,_b,_c,_d)
#define PlatformEFSdioLocalCmd53Write1Byte(_a,_b,_c)		\
			PlatformEFSdioCmd53Write1Byte(_a,SDIO_LOCAL_DEVICE_ID,_b,_c)
#define PlatformEFSdioLocalCmd53Write2Byte(_a,_b,_c)		\
			PlatformEFSdioCmd53Write2Byte(_a,SDIO_LOCAL_DEVICE_ID,_b,EF2Byte(_c))
#define PlatformEFSdioLocalCmd53Write4Byte(_a,_b,_c)		\
			PlatformEFSdioCmd53Write4Byte(_a,SDIO_LOCAL_DEVICE_ID,_b,EF4Byte(_c))
			
//20130422 KaiYuan add for 8814 available page read
#define PlatformEFMacLocalCmd53ReadNByte(_a,_b,_c,_d)		\
			PlatformEFSdioCmd53ReadNByte(_a,WLAN_IOREG_DEVICE_ID,_b,_c,_d)

u1Byte
PlatformSdioCmd52Read1Byte(
	IN	PVOID		Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset
	);

u2Byte
PlatformSdioCmd52Read2Byte(
	IN	PVOID		Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset
	);

u4Byte
PlatformSdioCmd52Read4Byte(
	IN	PVOID		Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset
	);

VOID
PlatformSdioCmd52Write1Byte(
	PVOID			sAdapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset,
	IN	u1Byte		data
	);

VOID
PlatformSdioCmd52Write2Byte(
	PVOID			Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset,
	IN	u2Byte		data
	);

VOID
PlatformSdioCmd52Write4Byte(
	PVOID			Adapter,
	IN	u1Byte		DeviceID,
	IN	u1Byte		FuncNum,
	IN	u4Byte		offset,
	IN	u4Byte		data
	);



// For P2P Platform-dependant Indication ------------------------------------
VOID
PlatformIndicateP2PEvent(
	PVOID pvP2PInfo, 
	u4Byte EventID,
	PMEMORY_BUFFER pInformation
);

// For Return TX pending packet case.
VOID
PlatformReturnAllPendingTxPackets(
	IN 	PADAPTER 		pAdapter
	);

// For Dual mac restore last init setting.
VOID
PlatformRestoreLastInitSetting(
	IN 	PADAPTER 		pAdapter
	);

BOOLEAN
PlatformIsOverrideAddress(
	IN	PADAPTER	Adapter
	);

pu1Byte
PlatformGetOverrideAddress(
	IN	PADAPTER	Adapter
	);
//---------------------------------------------------------------------


#endif // #ifndef __INC_PLATFORMDEF_H
