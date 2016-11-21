#ifndef __INC_N6SDIO_MAIN_H
#define __INC_N6SDIO_MAIN_H

// Forward declaration.
typedef	struct _ADAPTER	ADAPTER, *PADAPTER;
typedef	struct _RT_TCB	RT_TCB, *PRT_TCB;
typedef	struct _RT_RFD	RT_RFD, *PRT_RFD;


//sherry added here for dual mac 20110627
extern	RT_LIST_ENTRY     GlobalAdapterList;
extern	BOOLEAN	 GlobalFirstConfigurationForNormalChip;
extern	ULONG    GlobalMutexForGlobalAdapterList;
extern	GLOBAL_SPINLOCK    GlobalSpinlockForGlobalAdapterList;
extern	ULONG		GlobalMutexForPowerAndEfuse;
extern	ULONG		GlobalMutexForPowerOnAndPowerOff;
extern	ULONG		GlobalMutexForFwDownload;
extern	ULONG		GlobalMutexForMac0_2G_Mac1_5G;
extern	GLOBAL_SPINLOCK		GlobalSpinLockForDusc;

#define	N6SDIO_GET_MINIPORT_HANDLE(_pAdapter)	(GET_RT_SDIO_DEVICE(_pAdapter)->hNdisAdapter)


//-------------------------------------------------------------------------
// Tx Related. 
//-------------------------------------------------------------------------
#define N6SDIO_CANNOT_TX(__pAdapter) \
	(	RT_SDIO_CANNOT_TX(__pAdapter) || \
		N6C_GET_MP_DRIVER_STATE(__pAdapter) <= MINIPORT_PAUSING || \
		NicIFGetLinkStatus((__pAdapter)) != RT_MEDIA_CONNECT	)

#define N6SDIO_INC_TX_RESET_COUNT(__pDevice) ++((pDevice)->TxResetCnt)
#define N6SDIO_DEC_TX_RESET_COUNT(__pDevice) --((pDevice)->TxResetCnt)

#define N6SDIO_CANNOT_RX(__pAdapter) \
	(	(__pAdapter)->bDriverStopped || \
		(__pAdapter)->MgntInfo.bDriverIsGoingToSleep  || \
		RT_IS_FUNC_DISABLED((__pAdapter), DF_RX_BIT) || \
		N6C_GET_MP_DRIVER_STATE(__pAdapter) <= MINIPORT_PAUSING )


#if defined(__cplusplus)
extern "C"
{
#endif

typedef enum  {
  SDIO_LOCAL       = 0,
  SDIO_WLAN_TX_HIQ	= 1,
  SDIO_WLAN_TX_MIQ      	= 2,
  SDIO_WLAN_TX_LOQ      	= 3,
  SDIO_WLAN_RX_FF        	= 4,
  SDIO_IOREG      		= 5
} SDIO_DEVICE_ID;

NDIS_STATUS
DriverEntry(
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PUNICODE_STRING	RegistryPath
	);
#if defined(__cplusplus)
};
#endif
	

NDIS_STATUS
N6SdioInitialize(
	IN  NDIS_HANDLE                        MiniportAdapterHandle,
	IN  NDIS_HANDLE                        MiniportDriverContext,
	IN  PNDIS_MINIPORT_INIT_PARAMETERS     MiniportInitParameters
	);

VOID
N6SdioUpdateDefaultSetting(
	IN	PADAPTER		pAdapter
	);

RT_STATUS
N6SdioDumpDeviceProperty(
	IN PRT_SDIO_DEVICE	pDevice
);

NTSTATUS
N6SdioConfigureBusCLKByWorkAround(
	IN PRT_SDIO_DEVICE	pDevice
);

NTSTATUS
N6SdioConfigureBusCLK_SDR12(
	IN PRT_SDIO_DEVICE	pDevice
);

NTSTATUS
N6SdioConfigureDevice(
	IN PRT_SDIO_DEVICE	pDevice
);
               
RT_STATUS
N6SdioQueryCISArea(
	IN PRT_SDIO_DEVICE	pDevice
	);

RT_STATUS
N6SdioFindCisTuple(
	IN PRT_SDIO_DEVICE	pDevice,
	IN u1Byte	Tuple,
	IN OUT pu4Byte	pTupleScanAddress,
	OUT pu1Byte	pBuffer,
	IN OUT pu1Byte	pLength	
	);
               
NTSTATUS
N6SdioInitTxQueue(
	IN  PADAPTER 	Adapter
);

NDIS_STATUS
N6SdioRestart(
	IN  NDIS_HANDLE                         MiniportAdapterContext,
	IN  PNDIS_MINIPORT_RESTART_PARAMETERS   MiniportRestartParameters
	);

NDIS_STATUS
N6SdioPause(
	IN  NDIS_HANDLE                         MiniportAdapterContext,
	IN  PNDIS_MINIPORT_PAUSE_PARAMETERS     MiniportPauseParameters
	);

VOID
N6SdioShutdown(
	IN  NDIS_HANDLE             MiniportAdapterContext,
	IN  NDIS_SHUTDOWN_ACTION    ShutdownAction
	);

VOID
N6SdioPnPEventNotify(
	IN NDIS_HANDLE                  MiniportAdapterContext,
	IN PNET_DEVICE_PNP_EVENT        NetDevicePnPEvent
	);


NDIS_STATUS
N6SdioPowerSettingCallback(
	IN 	CONST LPGUID 	SettingGuid,
    	IN 	PULONG      		 Value,
    	IN	ULONG       		 ValueLength,
	IN 	PVOID       		 MiniportAdapterContext
	);

NDIS_STATUS
N6SdioWLANPowerModeCallback(
	IN 	CONST LPGUID 	SettingGuid,
    	IN 	PULONG      		 Value,
    	IN	ULONG       		 ValueLength,
	IN 	PVOID       		 MiniportAdapterContext
	);

VOID
N6SdioPowerSettingChangeNotify(
	IN     PADAPTER	Adapter
	);


VOID
N6SdioHalt(
	IN  NDIS_HANDLE             MiniportAdapterContext,
	IN  NDIS_HALT_ACTION        HaltAction
	);

VOID
N6SdioUnload(
	IN  PDRIVER_OBJECT  DriverObject
	);

NDIS_STATUS
N6SdioOidRequest(
	IN  NDIS_HANDLE         MiniportAdapterContext,
	IN  PNDIS_OID_REQUEST   NdisRequest
	);

VOID
N6SdioCancelOidRequest(
	IN NDIS_HANDLE hMiniportAdapterContext,
	IN PVOID       RequestId
	);

NDIS_STATUS
N6SdioSetOptions(
	IN NDIS_HANDLE  NdisMiniportDriverHandle,
	IN NDIS_HANDLE  MiniportDriverContext
	);

VOID
N6SdioSendNetBufferLists(
	IN  NDIS_HANDLE         MiniportAdapterContext,
	IN  PNET_BUFFER_LIST    pNetBufferLists,
	IN  NDIS_PORT_NUMBER    PortNumber,
	IN  ULONG               SendFlags
	);

BOOLEAN
N6SdioSendSingleNetBufferList(
	IN  PADAPTER			Adapter,
	IN  PNET_BUFFER_LIST	pNetBufferList,
	IN	BOOLEAN				bFromQueue
	);

BOOLEAN
N6SdioGetPacketBuffers(
	IN	PADAPTER		Adapter,
	IN	PNET_BUFFER		pNetBuffer,
	OUT	PRT_TCB			pTcb
	);

VOID
N6SdioSendCoalescedNetBuffer(
	IN PADAPTER			Adapter,
	IN PNET_BUFFER		pNetBuffer,
	OUT PRT_TCB			pTcb
	);

VOID
N6SdioReturnAllPendingTxPackets(
	IN 	PADAPTER 		pAdapter
	);

VOID
N6SdioCancelSend(
	IN    NDIS_HANDLE              MiniportAdapterContext,
	IN    PVOID                    CancelId
	);

VOID 
N6SdioReturnNetBufferLists(
	IN  NDIS_HANDLE         MiniportAdapterContext,
	IN  PNET_BUFFER_LIST    NetBufferLists,
	IN  ULONG               ReturnFlags
	);

BOOLEAN
N6SdioCheckForHang(
	IN NDIS_HANDLE MiniportAdapterContext
	);

NDIS_STATUS
N6SdioReset(
	IN  NDIS_HANDLE     MiniportAdapterContext,
	OUT PBOOLEAN        AddressingReset
	);



//
// NDIS5 Legacy -------------------------------------------------------------------
//
VOID
N6SdioFill80211PhyAttributes(
	IN	PADAPTER		Adapter,
	IN	PNDIS_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES	pDot11Attributes
	);

NDIS_STATUS
N6SdioAllocateNative80211MIBs(
	IN	PADAPTER		Adapter
	);

VOID
N6SdioFreeNative80211MIBs(
	IN	PADAPTER		Adapter
	);

NDIS_STATUS
N6SdioReadRegParameters(
	IN  PRT_SDIO_DEVICE		device
	);

VOID 
N6SdioFreeTxQueueRes( 
	IN PADAPTER		pAdapter
	);

VOID 
N6SdioDereferInterface( 
	IN PRT_SDIO_DEVICE		sdiodevice
	);

VOID 
N6SdioFreeDevice( 
	IN PRT_SDIO_DEVICE		usbdevice
	);

VOID
N6SdioCancelAllTimer(
	IN	PADAPTER	Adapter
	);

VOID
N6SdioFreeAll(
	IN	PADAPTER	Adapter
	);

VOID
N6SdioStartTxQueue(
	IN	PADAPTER		pAdapter,
	IN	int				nTxQueueIdx
	);

VOID
N6SdioStopTxQueue(
	IN	PADAPTER		pAdapter,
	IN	int				nTxQueueIdx
	);

NDIS_STATUS
N6SdioAllocateRxNetBufferListPool(
	IN	PADAPTER	Adapter
	);

VOID
N6SdioFreeRxNetBufferListPool(
	IN	PADAPTER	Adapter
	);

VOID
N6SdioEnableDeviceInterrupt(
	IN	PADAPTER	Adapter
);

VOID
N6SdioDisableDeviceInterrupt(
	IN	PADAPTER	Adapter
);

VOID
PnpWorkItemCallback(
	IN PVOID			pContext);

#if POWER_MAN
NDIS_STATUS
N6Sdio_Mgnt_SetPower(
	PADAPTER	            	Adapter,
	IN	PVOID	            	InformationBuffer,
	IN	ULONG            	InformationBufferLength,
	OUT	PULONG          	BytesNeeded,
	OUT	PULONG            	BytesRead
);
#endif

VOID
N6SdioCleanUpInitializedResourcesEx(
	IN	PADAPTER	Adapter,
	IN	RES_MON_OBJ	ResMonObj
	);


VOID
N6SdioCleanUpInitializedResources(
	IN	PADAPTER	Adapter,
	IN	RM_RES_TYPE	ResType
	);

VOID
FillPnpCapabilities(
	IN PADAPTER							pAdapter,
	IN OUT PNDIS_PNP_CAPABILITIES		pPnpCapabilities
	);

VOID
FillPmCapabilities(
	IN PADAPTER							pAdapter,
	IN OUT PNDIS_PM_CAPABILITIES		pPmCapabilities
);

VOID
N6SdioAllocRes(
	IN	PADAPTER	Adapter
	);

VOID
N6SdioFreeRes(
	IN	PADAPTER	Adapter
	);

// TODO: add for WDI

NDIS_STATUS
N6sdioAllocateAdapter(
	OUT	PADAPTER		*pAdapter,
	IN	NDIS_HANDLE		MiniportAdapterHandle
	);

RT_STATUS
N6SdioSyncCurrentSdBusClk(
	IN PRT_SDIO_DEVICE	pDevice
);

VOID
N6SdioDummyIO(
	IN PRT_SDIO_DEVICE	pDevice
);

#endif // #ifndef __INC_N6SDIO_MAIN_H


