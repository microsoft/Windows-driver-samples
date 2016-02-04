#ifndef __INC_N6SDIO_TYPEDEF_H
#define __INC_M6SDIO_TYPEDEF_H


/*++

Copyright (c) 2012 Realtek Semiconductor Corporation

Module Name:
	usb.h

Abstract:
	Miniport generic portion header file
	Roger 2012.01.19

*/


#undef RTL8187_FPGA_CODE

//--- Annie for NDIS6 EXPR, 2006-09-11. ---
#ifdef NDIS60_MINIPORT
#define NDIS_MAJOR_VERSION		6
#define NDIS_MINOR_VERSION		0
#define NDIS_MINOR_VERSION_2	20
#define NDIS_MINOR_VERSION_3	30
#define NDIS_MINOR_VERSION_4	40
#define NDIS_MINOR_VERSION_5	50

#define DRIVER_MAJOR_VERSION	1
#define DRIVER_MINOR_VERSION	0
#endif

#ifndef MIN
#define MIN(a, b)	((a) > (b) ? b: a)
#endif


//--------------------------------------
// Macros specific to miniport adapter structure 
//--------------------------------------
#define MP_SET_PACKET_RFD(_p, _rfd)	*((PRT_RFD *)&(_p)->MiniportReserved[1]) = _rfd
#define MP_GET_PACKET_RFD(_p)		*((PRT_RFD *)&(_p)->MiniportReserved[1])

#define MP_SET_PACKET_RFD_RESERVED_0(_p, _rfd)		*((PWDI_FRAME_METADATA *)&(_p)->MiniportReserved[0]) = _rfd
#define MP_GET_PACKET_RFD_RESERVED_0(_p)		*((PWDI_FRAME_METADATA *)&(_p)->MiniportReserved[0])

#define MAKE_DRIVER_VERSION(_MainVer,_MinorVer)	((((ULONG)(_MainVer))<<16)+_MinorVer)
#define NIC_VENDOR_DRIVER_VERSION       MAKE_DRIVER_VERSION(0,001)

// The CheckForHang intervals before we decide the send is stuck
#define NIC_SEND_HANG_THRESHOLD		4

#define MAX_NUM_PIPES 15


#if USE_WDF_USB 

//
// Obtain desired GUID for SSNDIS miniport binding, added by Roger, 2010.03.23.
//
DEFINE_GUID(GUID_KMDFIDLER_INTERFACE_STANDARD,
0xefad15a2, 0xa6f8, 0x4e60, 0x99, 0xc7, 0x2b, 0x92, 0x62, 0x4d, 0xdc, 0x25);
// {EFAD15A2-A6F8-4e60-99C7-2B92624DDC25}


typedef
VOID
(*PF_STOP_IDLE)(
    	PVOID		Context
    	);

typedef
VOID
(*PF_RESUME_IDLE)(
	PVOID		Context
    	);

typedef
VOID
(*PF_GET_IDLER_ENABLE_CNT)(
    PVOID			Context,
    PUCHAR 		RefCnt
    );

typedef
VOID
(*PF_GET_IDLER_PWR_STATE)(
    PVOID		Context,
    PDEVICE_POWER_STATE 	pDevPwrState,
    PSYSTEM_POWER_STATE 	pSysPwrState
    );

typedef
VOID
(*SS_POWER_STATE_CHANGE_CALLBACK)(
    PVOID		Context,
    PDEVICE_POWER_STATE 	pDevPwrState,
    PSYSTEM_POWER_STATE 	pSysPwrState
    );
typedef
VOID
(*PF_GET_IDLER_WAKEUP_CNT)(
    PVOID		Context,
    PUCHAR 	WakeupComplete
    );

//
// Interface function handler which SSNDIS stack exported, added by Roger, 2010.03.23.
//
typedef struct _KMDFIDLER_INTERFACE_STANDARD {
	INTERFACE                 InterfaceHeader;
	PF_STOP_IDLE              StopIdle; // WDF stop idle exported handler.
	PF_RESUME_IDLE            ResumeIdle; // WDF resume idle exported handler.
	PF_GET_IDLER_ENABLE_CNT		IdlerRefCnt; //Current idler enable counter.
	PF_GET_IDLER_PWR_STATE		IdlerPwrState; // Current drvier stack power stack.
	PF_GET_IDLER_WAKEUP_CNT	IdlerWakeupCnt;	
} KMDFIDLER_INTERFACE_STANDARD, *PKMDFIDLER_INTERFACE_STANDARD;

#endif

//
// The number of Async SDIO IO Write Block. Revised by Roger, 2011.12.30.
//
#define MAX_AWB_DATA_SIZE 768

typedef struct _RT_AWB
{
	RT_LIST_ENTRY	List;
	u4Byte			Offset;
	u2Byte			ByteCnt; 
	u1Byte			DataBuf[MAX_AWB_DATA_SIZE];
#if RTL8723_SDIO_IO_THREAD_ENABLE
	u1Byte			DeviceID;
	u1Byte			FuncNum;
#endif
}RT_AWB, *PRT_AWB;

//
// The platform extension of RT_WORK_ITEM. It is platform dependent.
// 2006.09.28, by shien chang.
//
typedef struct _PLATFORM_EXT {
	NDIS_EVENT		Event;			// Event used to handle driver unload. 
	NDIS_SPIN_LOCK	Lock;			// Lock to protect object of this class.
} PLATFORM_EXT, *PPLATFORM_EXT;


// Manufacturer ID tuple, added by Roger, 2011.10.10.
typedef struct _SDIO_MANFID_TPL {
	u2Byte  	ManufacturerCode; // Manufacture code
	u2Byte  	ManufacturerInfo; // manufacturer information
}SDIO_MANFID_TPL, *PSDIO_MANFID_TPL;


#define MAX_MCAST_LIST_NUM					32		// multicast list size

//
// Resource monitor for driver initialization. 2007.02.02, by shien chang.
//
typedef enum _RM_RES_TYPE {
	InitRM_Undefined,
	InitRM_AllocAdapter,	// 0x1
	InitRM_AllocRxNBLPool,
	InitRM_SpinLock,
	InitRM_AWBs,
	InitRM_AsocNIC,			// 0x5
	InitRM_AllocMem,
	InitRM_SdioInit,
	InitRM_SdioTxRes,
	InitRM_N62CInit,		//added for SDIO WDI
	InitRM_AllocNWifi,		// 0xa
	InitRM_AllocSdioRes,
	InitRM_BTIODevice,
	InitRM_CommonRS,
	InitRM_MaxResItems,
} RM_RES_TYPE, *PRM_RES_TYPE;

typedef struct _RES_MON_OBJ {
	u1Byte			nResToCheck;
	RM_RES_TYPE		ResItems[InitRM_MaxResItems];
} RES_MON_OBJ, *PRES_MON_OBJ;

#define INIT_RES_MON_OBJ(MonObj)	\
	(MonObj).nResToCheck = 0;
#define ADD_RES_TO_MON(MonObj, ResToMon)	\
	(MonObj).ResItems[(MonObj).nResToCheck++] = ResToMon;
#define CLEANUP_RES_IN_MON(MonObj, Adapter)		\
	{												\
		u1Byte ResIndexPlueOne=0;						\
		for (ResIndexPlueOne=(MonObj).nResToCheck;	\
			ResIndexPlueOne != 0;						\
			ResIndexPlueOne --)						\
		{											\
			N6SdioCleanUpInitializedResources(Adapter, (MonObj).ResItems[ResIndexPlueOne-1]); \
		}											\
		(MonObj).nResToCheck = 0;					\
	}


#define	SDRP_MAX_ERROR_RETRY_CNT	3 // SDRP retry count

//
// SDbus request packet error handling type.
//
typedef enum	_SDRP_ERROR_HANDLING_TYPE{
	SDRP_ERROR_RETRY = 0,
	SDRP_ERROR_SKIP = 1,
	SDRP_ERROR_SURPRISE_REMOVED = 2,
}SDRP_ERROR_HANDLING_TYPE, *PSDRP_ERROR_HANDLING_TYPE;


//
// SDIO Two direction transfer type.
//
typedef enum	_RF_SDIO_TRANSFER_TYPE{
	SDIO_TRANSFER_TX = 0,
	SDIO_TRANSFER_RX = 1,	
}RF_SDIO_TRANSFER_TYPE, *PRF_SDIO_TRANSFER_TYPE;

//
// SDIO SdSubmit Time Record.
//
typedef struct _RT_SDIO_BUS_TIME_RECORD
{
	u8Byte			AbTime;
	LARGE_INTEGER	SdBusWorkDuration;
} RT_SDIO_TIME_R, *PRT_SDIO_TIME_R;

//
// SDIO Data Transfer Queue information for Tx.
//
typedef struct _RT_SDIO_TX_QUEUE
{
	u4Byte					TransferType; // 0:Transfer Tx, 1:Transfer Rx
	RT_LIST_ENTRY			ContextBusyList; // A link list to keep the USB_OUT_CONTEXT pending in USBD. We can canceled the IRP sent via enumerate the list.
	u4Byte					IrpPendingCount; // Initialize as 1, +1 when we send a IRP down, -1 if the IRP completed.
	NDIS_EVENT				AllIrpReturnedEvent; // Event used to signal MiniportHalt if it is waiting for all IRP pending on this pipe completed.
	BOOLEAN					bStalled; // TRUE if this queue is stalled
	BOOLEAN					bEnabled; // TRUE if this pipe is in active state, false otherwise.
} RT_SDIO_TX_QUEUE, *PRT_SDIO_TX_QUEUE;


//
// SDIO Data Transfer Queue information for Tx.
//
typedef struct _RT_SDIO_RX_QUEUE
{
	u4Byte					TransferType; // 0:Transfer Tx, 1:Transfer Rx
	RT_LIST_ENTRY			ContextBusyList; // A link list to keep the USB_IX_CONTEXT pending in USBD.
	int						IrpPendingCount; // Initialize as 1, +1 when we send a IRP down, -1 if the IRP completed.
	NDIS_EVENT				AllIrpReturnedEvent; // Event used to signal MiniportHalt if it is waiting for all IRP pending on this pipe completed.
	u4Byte					RestartPeriod; // In ms.
	BOOLEAN					bEnabled; // TRUE if this pipe is in active state, false otherwise.
}RT_SDIO_RX_QUEUE, *PRT_SDIO_RX_QUEUE;

typedef struct _RT_SDIO_DEVICE {
	//  NDIS handles
	NDIS_HANDLE		hNdisAdapter;			//NDISMiniportAdapterHandle
	NDIS_HANDLE		hNdisConfiguration;

	//  related device objects
	PDEVICE_OBJECT	pPhysDevObj;
	PDEVICE_OBJECT	FunctionalDeviceObject;
	PDEVICE_OBJECT	pSdioDevObj;			// next device object
	SDBUS_INTERFACE_STANDARD	Sdbusinterface; //structure holds pointers to the SD bus interface routines

	s1Byte	NextDeviceStackSize;

	//3 SDIO related Bus info and properitie.
	u1Byte	SdioFuncNum; // Function number on SD card
	u2Byte	SdioBusDriverVer; 
	u1Byte	SdioRevNum; // CCCRx Format Version Number and SDIOx Specification Revision Number. 
	u1Byte	SdioFormatVer; // SD Format Version Number.
	u2Byte	SdioFuncType;	
	u1Byte	SdioBusWidth;
	u4Byte	SdioBusClk; 
	u2Byte	SdioHostBlockSize; // Maximum block length that is supported by the controller
	u2Byte	SdioFuncBlockSize;
	u4Byte			SdioRxSeqNum;
	u4Byte	SdioCommonCISPtr;
	u1Byte	SdioRegDbgCtrl;
	
	BOOLEAN		SdioTxBlockMode;
	BOOLEAN		SdioRxBlockMode;
	BOOLEAN		IoRegDirectAccess;
	
	// SDIO Card Metaformat for CIS
	SDIO_MANFID_TPL	Sdiomanfid; // Manufacture Identification String Tuple
	u2Byte			SDIO_ManufacturerCode; //Manufacture Code
    	u2Byte 			SDIO_ManufacturerInfo; //manf-specific ID

	PVOID			pAdapter;
	//enum _PLATFORM_NDIS_VERSION		NdisVersion;
	
	NDIS_SPIN_LOCK			SpinLock;
	NDIS_SPIN_LOCK			TxSpinLock;
	NDIS_SPIN_LOCK			RxSpinLock;
	NDIS_SPIN_LOCK			RxControlSpinLock;
	NDIS_SPIN_LOCK			RxQueueSpinLock;
	NDIS_SPIN_LOCK			RmSpinLock; // For RT_RM_SPINLOCK, Radio Measurement spinlock.
	NDIS_SPIN_LOCK			RxRefCntSpinLock;
	NDIS_SPIN_LOCK			CamSpinLock;	// For RT_CAM_SPINLOCK, 87B CAM and Tx stuck issue. 2006-07-17.
	NDIS_SPIN_LOCK			ScanSpinLock;	// For scan, 2006.12.08, by shien chang.
	NDIS_SPIN_LOCK			LogSpinLock;	// For driver log mechanism, 070301, by rcnjko.
	NDIS_SPIN_LOCK			BwSpinLock;		// For Bandwidth Switching, 070608, by Joseph.
	NDIS_SPIN_LOCK			ChnlOpSpinLock;
	NDIS_SPIN_LOCK			RFWriteSpinLock;
	NDIS_SPIN_LOCK			H2CSpinLock;// For H2C. Added by tynli.
	NDIS_SPIN_LOCK			BTDataSpinLock;	// For BT Data, 2010 01.23 by cosa
	NDIS_SPIN_LOCK			GenBufSpinLock;		//General puporse templorary buffer link list lock


	// Ndis 6.2 Releated Lock
	NDIS_SPIN_LOCK			PortLock;	// For New Port, 080418, by Maddest	

	NDIS_SPIN_LOCK			InitialSpinLock;// For Initialize 2007.11.5 by cosa
	NDIS_SPIN_LOCK			RFStateSpinLock; // For set RF on/off 
	
	NDIS_SPIN_LOCK			WapiOptionSpinLock;// For wapi 2009.05.21 by sherry
	NDIS_SPIN_LOCK			WapiRxSpinLock;// For wapi 2009.05.21 by sherry
	NDIS_SPIN_LOCK			BufferSpinLock;	// For NetBufferLists
	NDIS_SPIN_LOCK			FwPSSpinLock;	// For LPS 32k. by tynli.
	NDIS_SPIN_LOCK			MptWorkItemSpinLock;    // For MP, 20120321 by Kordan
    NDIS_SPIN_LOCK			P2PSpinLock;	// Protect P2P context.
	NDIS_SPIN_LOCK			ndisDbgWorkItemSpinLock;    // 20120525 by cosa
	NDIS_SPIN_LOCK			IQKSpinLock;				// For IQK. 20130807, Added by VincentL.
	NDIS_SPIN_LOCK			DynTxPwrTblSpinLock;
	NDIS_SPIN_LOCK			PendedOidSpinLock;// Synchronization method for OID pending, added by Roger, 2013.07.11.   	
	NDIS_SPIN_LOCK			ChnlListSpinLock;
	NDIS_SPIN_LOCK			IndicSpinLock;	// Protect indic context.	
	NDIS_SPIN_LOCK			RfdSpinLock;
	
#if (AUTO_CHNL_SEL_NHM == 1)	
	NDIS_SPIN_LOCK			AcsSpinLock;	// Auto Channel Select
#endif

#if DRV_LOG_REGISTRY
	NDIS_SPIN_LOCK			DrvStateSpinLock;
#endif	

	NDIS_SPIN_LOCK			CustomScanSpinLock;

	u4Byte					Flags;

	// Tx related.
	PNET_BUFFER_LIST		SendingNetBufferList; // Pointer to the NBL working in N6SdioSendSingleNetBufferList().
	BOOLEAN					bWaitingSendingNBL; // TRUE if one thread is waiting evtSendingNBLCompleted.
	NDIS_EVENT				evtSendingNBLCompleted; // Event is signaled when the NBL, SendingNetBufferList, is completed during driver halt/reset.
	u4Byte					TxResetCnt; // # of thread doing N6SdioReturnAllPendingTxPackets().

	// Rx releated.
	int						nMaxBulkInCnt; // Number of Bulk in issued at initialization, it shall falls in [1,# of RFD allocated].
	NDIS_HANDLE				RxNetBufferListPool;

	// SDIO Tx Queue related information.
	PRT_SDIO_TX_QUEUE	RtTxQueue;		// A ring of Tx queues.
	int					RtNumTxQueue;	// Number of Tx queue in the ring	

	// SDIO Rx Queue related information.
	int					RtNumRxQueue;	// Number of Rx queues in the ring

	// LED.
	BOOLEAN					bRegLedCtrl;

	BOOLEAN					bNdisStatusIndicated;
	u2Byte					NdisStatusIndicateCount;

	//----------------------------------------------------------------------------
	// IO Method Related. 2005.01.06, by rcnjko. 
	NDIS_SPIN_LOCK			IrpSpinLock;	// To make sure at most one IRP sent to USB host driver.
	NDIS_SPIN_LOCK			AwbSpinLock;
	NDIS_SPIN_LOCK			HwTimerSpinLock;
	NDIS_SPIN_LOCK			SyncIoCntSpinLock; // For Sync IO count. 2014.10.02, by tynli.

	// Tx related Semaphore
	PlatformSemaphore		TxSemaphore;

#if RTL8723_SDIO_IO_THREAD_ENABLE 
	// IO related Semaphore
	PlatformSemaphore		IOSemaphore;
#endif
	
	int						nIrpPendingCnt;	// #IRP pending in USB host, it MUST be protected by IrpSpinLock.
	RT_LIST_ENTRY			AwbIdleQueue;	// Queue for non-used AWBs.
	RT_LIST_ENTRY			AwbWaitQueue;	// Queue for AWB waiting to be sent down.
	u4Byte					NumAwb;			// #Awb allocated
	PVOID					AwbBuffer;		// Pointer to the block of memory for AWBs.
	u4Byte					AwbBufferSize;	// Size of AwbBuffer. 
	u4Byte					NumIdleAwb; // Num of idle Awb
	u4Byte					NumWaitAwb; // Num of waiting Awb

	// SyncIo Method 2.  
//	BOOLEAN					bSyncIoWaiting; // Indicate if one or more Sync IO are waitting.
	int						SyncIoWaitingCount;	
	KEVENT					SyncIoEvent;	// Event used to notify the waiting Sync IO.
	int						nIoStuckCnt;	// # of IO stuck.
	int						SyncIoInProgressCount;
	
	NDIS_EVENT	AllCtrlTransCompleteEvent;

	PIRP		pAsynIoWriteIrp;
	PSDBUS_REQUEST_PACKET	pAsynIoWriteSdrp;
	PMDL	pAsynIoWriteMDL;
	NDIS_EVENT	AllAsynIoIrpReturnedEvent;
	u1Byte		AsynIoIrpPendingCount;
	BOOLEAN		bAsynIoWritePending;
	u4Byte		AsynIoWriteOffset;
	u2Byte		AsynIoWriteByteCnt;
	u1Byte		AsynIoWriteDataBuf[MAX_AWB_DATA_SIZE];
	//----------------------------------------------------------------------------
#if 0	
	// e.g. IO monitor. 2005.04.21, by rcnjko.
	RT_8187DBG_CONTEXT	DbgCtx;
#endif
#if USE_WDF_SDIO
	//
	// Handles for WDF SDIO.
	//
	WDFDEVICE			hWdfDevice;
#endif

	
#if POWER_MAN
	// @@@ 20061205 david power management
	RT_WORK_ITEM				PnpWorkItem; 
	NDIS_DEVICE_POWER_STATE		CurrentPowerState;
#endif //power_man
	
	BOOLEAN				bSupportRemoteWakeUp;

	// 2011/09/07 MH Add for differen channel plan tx power offset.
	u4Byte			RegGainOffsetFCCLow;
	u4Byte			RegGainOffsetFCC;
	u4Byte			RegGainOffsetFCCHigh;
	u4Byte			RegGainOffsetMKKLow;
	u4Byte			RegGainOffsetMKK;
	u4Byte			RegGainOffsetMKKHigh;
	u4Byte			RegGainOffsetETSILow;
	u4Byte			RegGainOffsetETSI;
	u4Byte			RegGainOffsetETSIHigh;

	// 2011/11/25 MH Add for Sercom requirement temporarily.
	u1Byte			RegInPipeCnt;

		
	// For Usb selective suspend feature, 2009.07.03.
	BOOLEAN				RegUsbSS;
	//
	// <Roger_Notes> Fix Bug Check code 0x1d(i.e., NIC might be unplugged 
	// before system resume or under initialization). 2009.09.24.
	//
	NDIS_EVENT			SetPnpChkForHangEvent;
	BOOLEAN				bChkForHangAfterPnP;
	DEVICE_POWER_STATE 	DevPwrState;

	u4Byte	DeviceAddress;
	u4Byte	InterfaceIndex;

	// SDIO SD Request reference control
	NDIS_EVENT			AllSdioCmdReturnedEvent;
	u4Byte				SdioCmdRefCnt; // Number of SDIO CMD we submitted.

	// SDIO submit failure count
	u2Byte				FailureCount;

	//
	BOOLEAN				bSdioBusClkRaised;

	NDIS_EVENT			FwPsClockOffEvent;

	// SDIO Rx transfer handling
	BOOLEAN					bStopRxTransfer;
	u4Byte					SdioRxTransInProgressCnt;
	NDIS_EVENT				AllSdioRxTransCompleteEvent;

	// SDIO Host Compatibility Conrtol
	BOOLEAN					bRegSdioSpeedSDR25;			// Set SDIO bus speed to 0: 25MHz, 1: 50MHz 
	BOOLEAN					bRegSdioDrivingEnable;		// Enable/Disable Custom Driving Setting
	u1Byte					RegSdioDriving;
	BOOLEAN					bRegSurpriseRemovedEnable;	// Enable/Disable SurprisedRemove while facing I/O error

	// HostBus Working Time Measurement.
	u1Byte					BusTimeRecordIndex;
	RT_SDIO_TIME_R			SdBusSubmitTime[7];	
	LARGE_INTEGER			PnPSdBusWorkTime;

#if (RK_PLATFORM_SUPPORT == 1)
	// Mutex for handling SDIO Rx interrupt routine
	PlatformMutex				RxHandleIntMutex;
#endif

} RT_SDIO_DEVICE , *PRT_SDIO_DEVICE ;

#define GET_RT_SDIO_DEVICE(__pAdapter) (&((__pAdapter)->NdisSdioDev))

// 
// Keep the number of SDIO CMDs we have already schedled to the SDBus. Added by Roger, 2011.04.01.
// 2011.04.01.
//
#define RT_SDIO_INC_CMD_REF(__pDevice) ((__pDevice)->SdioCmdRefCnt++)
#define RT_SDIO_DEC_CMD_REF(__pDevice) (((__pDevice)->SdioCmdRefCnt>0)?((__pDevice)->SdioCmdRefCnt--):((__pDevice)->SdioCmdRefCnt=0))
#define RT_SDIO_GET_CMD_REF(__pDevice) ((__pDevice)->SdioCmdRefCnt)

//
// Record the number of SDIO Rx transfer that is going to submit to SDbus. Added by tynli. 2014.11.19.
// Default is set to 1 for the purpose of event wait in disable transfer function. 
//
#define RT_SDIO_INC_RX_TRANS_REF(__pDevice) ((__pDevice)->SdioRxTransInProgressCnt++)
#define RT_SDIO_DEC_RX_TRANS_REF(__pDevice) (((__pDevice)->SdioRxTransInProgressCnt>0)?((__pDevice)->SdioRxTransInProgressCnt--):((__pDevice)->SdioRxTransInProgressCnt=0))
#define RT_SDIO_GET_RX_TRANS_REF(__pDevice) ((__pDevice)->SdioRxTransInProgressCnt)

// 
// Retrieve specific SDIO Queue IRP pending count. Added by Roger, 2011.02.09.
//
#define GET_IRP_PENDING_CNT_WITH_TX_QUEUE_IDX(__pAdapter, __IdxQueue)	\
	GET_RT_SDIO_DEVICE(__pAdapter)->RtTxQueue[__IdxQueue].IrpPendingCount

//
// Retrieve SDIO Rx sequence number
//
#define RT_SDIO_GET_RX_SEQ_NUM(__pAdapter)\
	GET_RT_SDIO_DEVICE(__pAdapter)->SdioRxSeqNum

//
// Increase SDIO Rx sequence number
//
#define RT_SDIO_INC_RX_SEQ_NUM(__pAdapter)\
{ \
	if(GET_RT_SDIO_DEVICE(__pAdapter)->SdioRxSeqNum == 0xffffffff)\
		GET_RT_SDIO_DEVICE(__pAdapter)->SdioRxSeqNum = 0;\
	else\
		GET_RT_SDIO_DEVICE(__pAdapter)->SdioRxSeqNum++;\
}
	
//
// Reset SDIO Rx sequence number
//
#define RT_SDIO_RESET_RX_SEQ_NUM(__pAdapter)\
{\
	GET_RT_SDIO_DEVICE(__pAdapter)->SdioRxSeqNum = 0;\
}

#endif // #ifndef __INC_N6SDIO_TYPEDEF_H
