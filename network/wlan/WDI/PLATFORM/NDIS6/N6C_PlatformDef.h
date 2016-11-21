/*++

Copyright (c) Realtek Corporation. All rights reserved.

Module Name:
	N6C_PlatformDef.h

Abstract:
	Mediate Data structures referenced by PlatformDef.h for NDIS6.

Major Change History:
      When        Who	What
    ----------	------	----------------------------------------------
    2006-09-29	S.Chang	Created

Notes:

--*/
#ifndef __INC_N6C_PLATFORMDEF_H
#define __INC_N6C_PLATFORMDEF_H

/*--------------------------Define -------------------------------------------*/
#define VOID void
#define PVOID void * //for Source insight keyword highlight.

// Add this type define if your platform doesn't have BOOLEAN type
typedef UCHAR		BOOLEAN,*PBOOLEAN, boolean;
typedef UCHAR		u1Byte,*pu1Byte;
typedef USHORT		u2Byte,*pu2Byte;
typedef ULONG		u4Byte,*pu4Byte;
typedef ULONGLONG	u8Byte,*pu8Byte;

typedef CHAR				s1Byte,*ps1Byte, s8;
typedef SHORT			s2Byte,*ps2Byte, s16;
typedef LONG				s4Byte,*ps4Byte, s32;
typedef LONGLONG	s8Byte,*ps8Byte;

typedef const unsigned char cu8;
typedef unsigned __int64  u64;
typedef __int64  	s64;

typedef unsigned char	u8;	 
typedef unsigned short	u16;	
typedef unsigned long	u32;

typedef struct _RT_TIMER_HANDLE{
	RT_LIST_ENTRY			List;

	NDIS_HANDLE					NdisTimerHandle;
	NDIS_TIMER_CHARACTERISTICS	TimerCharacteristics;
}RT_TIMER_HANDLE, *PRT_TIMER_HANDLE;


typedef NDIS_HANDLE	RT_WORKITEM_HANDLE, *PRT_WORKITEM_HANDLE;

#define	DEBUG_MAX_NAME_LEN		20


typedef KMUTEX			PlatformMutex, *PPlatformMutex;


typedef KSEMAPHORE		PlatformSemaphore, *PPlatformSemaphore;
typedef NDIS_EVENT		PlatformEvent, *PPlatformEvent;

#define	rsprintf		RtlStringCbPrintfA
#define	rstrncat		RtlStringCbCatNA



/*--------------------------Define MACRO--------------------------------------*/

#define GET_TCB_IDLE_QUEUE(_Adapter) (PRT_LIST_ENTRY)&(GetDefaultAdapter(_Adapter)->TcbIdleQueue)
#define GET_RFD_IDLE_QUEUE(_Adapter) (PRT_LIST_ENTRY)&(GetDefaultAdapter(_Adapter)->RfdIdleQueue)
#define Get_WAIT_QUEUE(_Adapter,_QueueID) (PRT_LIST_ENTRY)&(GetDefaultAdapter(_Adapter)->TcbWaitQueue[_QueueID])
#define GET_WAIT_QUEUE_CNT(_Adapter,_QueueID) &(GetDefaultAdapter(_Adapter)->TcbWaitQueueCnt[_QueueID])
#define Get_NUM_IDLE_TCB(_Adapter) &(GetDefaultAdapter(_Adapter)->NumIdleTcb)
#define GET_NUM_IDLE_RFD(_Adapter) &(GetDefaultAdapter(_Adapter)->NumIdleRfd)
#define GET_NUM_BUSY_RFD(_Adapter, _QueueNum) &(GetDefaultAdapter(_Adapter)->NumBusyRfd[_QueueNum])
#define GET_LOW_RFD_THRESHOLD(_Adapter) &(GetDefaultAdapter(_Adapter)->LowRfdThreshold)

#define IS_TX_LOCKED(_Adapter) ((_Adapter->pNdis62Common) ? (GetDefaultAdapter(_Adapter)->bTxLocked) : (_Adapter->bTxLocked))
#define IS_RX_LOCKED(_Adapter) ((_Adapter->pNdis62Common) ? (GetDefaultAdapter(_Adapter)->bRxLocked) : (_Adapter->bRxLocked))

#define IS_ADAPTER_SENDS_BEACON(__Adapter) IsSendingBeacon(__Adapter)

#define GET_PORT_NUMBER(_Adapter)								\
		 _Adapter->pNdis62Common->PortNumber	

#define N6XC_SET_802_11_ATTRIBUTES(_Adapter) N6Set80211Attributes(_Adapter)
#define NDIS_6_2_INITIALIZE_EXTENSION_COMPONENT(_Adapter,_MiniportAdapterHandle,_MiniportInitParameters) 												\
		N62CInitialize(_Adapter,_MiniportAdapterHandle,_MiniportInitParameters)



#define NDIS_6_2_FREE_EXTENSION_COMPONENT(_Adapter)					\
	N62CFreePortCommonComponent(_Adapter);							\
	N62CFreePortSpecificComponent(_Adapter);							\
	while(MultiPortIsPreallocatedExtAdapterExist(_Adapter))						\
	{																	\
		N62CFreeExtAdapter(MultiPortRemoveExtAdapter(_Adapter));		\
	}																	\


#define NDIS_6_2_EXT_ADAPTER_HANDLE_NBL_IN_WAIT_QUEUE(_Adapter) 									\
		N62CExtAdapterHandleNBLInWaitQueue(_Adapter)

#define NDIS_6_2_AP_CLEAR_BEFORE_SLEEP(_Adapter)													\
		N62CAPClearStateBeforeSleep(_Adapter);


#define EXT_AP_START_AP_MODE(_Adapter)
#define EXT_AP_NOTIFY_STA_LINK_CHANGE(_pMgntInfo)	\
		{_pMgntInfo->bSwitchingSTAStateInProgress = TRUE;}	
#define EXT_AP_DISASSOCATE_AFTER_STA_LINK_TO_DIFFERENT_CHNNEL_AP(_Adapter)			\
		AP_DisconnectAfterSTANewConnected(_Adapter)
#define REMOVE_WCN_IE_FROM_BSS_DESC(_Adapter,_bssDesc,_pSimpleConfig,_mmpdu)					\
		RemoveWCNIE(_Adapter,_bssDesc,_pSimpleConfig,_mmpdu)	

#define PLATFORM_GET_PHY_ID_READY(_Adapter)	N6CQueryPhyIdReady(_Adapter)
#define PLATFORM_GET_FRAGOFFSET(_pRfd) _pRfd->FragOffset
#define PLATFORM_WRITE_EVENT_LOG(_Adapter, _EVENT, _NUM)	N6WriteEventLog(_Adapter, _EVENT, _NUM)
#define PLATFORM_WLAN_IN_MPDU_TOKEN_NUM 6
#define PLATFORM_WLAN_IN_INTERRUPT_TOKEN_NUM 2
#define PLATFORM_RESTORE_LAST_INIT_SETTING(_Adapter) N6RestoreLastInitSettingAterWakeUP(_Adapter)



#define PLATFORM_GET_RT_NUM_PIPES(_pDevice) GET_WDF_USB_DEVICE_CONTEXT(_pDevice)->RtNumPipes
#define PLATFORM_GET_RT_NUM_IN_PIPES(_pDevice) GET_WDF_USB_DEVICE_CONTEXT(_pDevice)->RtNumInPipes
#define PLATFORM_GET_RT_NUM_OUT_PIPES(_pDevice) GET_WDF_USB_DEVICE_CONTEXT(_pDevice)->RtNumOutPipes
#define PLATFORM_GET_PRT_IN_PIPES(_pDevice) GET_WDF_USB_DEVICE_CONTEXT(_pDevice)->RtInPipes
#define PLATFORM_GET_PRT_OUT_PIPES(_pDevice) GET_WDF_USB_DEVICE_CONTEXT(_pDevice)->RtOutPipes

// For SDIO related interface. Added by Roger. 2011.10.10.
#define PLATFORM_GET_RT_NUM_SDIO_RX_QUEUE(_pDevice) _pDevice->RtNumRxQueue
#define PLATFORM_GET_RT_NUM_SDIO_TX_QUEUE(_pDevice) _pDevice->RtNumTxQueue
#define PLATFORM_GET_PRT_SDIO_TX_QUEUE(_pDevice) _pDevice->RtTxQueue

/*------------------------------Define Struct---------------------------------*/

/*------------------------------Define Enum-----------------------------------*/

/*------------------------Export global variable------------------------------*/

/*------------------------------Funciton declaration--------------------------*/
VOID
N6InitTimerSync(
	IN	PVOID	Adapter
);

VOID
N6WaitTimerSync(
	IN	PVOID	Adapter
);

VOID
N6DeInitTimerSync(
	IN	PVOID	Adapter
);


VOID
N6CInitThread(
	IN	PVOID		pContext
	);

VOID
N6CDeInitThread(
	IN	PVOID	Adapter
	);


extern u8Byte	RTL_AllocateMemory_count ;
extern u8Byte	RTL_FreeMemory_count ;
extern u8Byte	RTL_AllocateMemory_Len;
extern u8Byte	RTL_FreeMemory_Len;


#endif //end of #ifndef __INC_N6C_PLATFORMDEF_H
