#ifndef __INC_N6CSDIO_WDIMAIN_H
#define __INC_N6CSDIO_WDIMAIN_H

#define	WDI_TX_RESOURCES_WARNING		64		// when pTcb is less than 64, means we are going to run out of tx resources

MINIPORT_WDI_ALLOCATE_ADAPTER N6SdioWdi_AllocateAdapter;

MINIPORT_WDI_FREE_ADAPTER N6SdioWdi_FreeAdapter;

MINIPORT_WDI_OPEN_ADAPTER N6SdioWdi_OpenAdapter;

MINIPORT_WDI_CLOSE_ADAPTER N6SdioWdi_CloseAdapter;

MINIPORT_WDI_START_ADAPTER_OPERATION	N6SdioWdi_StartOperation;

MINIPORT_WDI_STOP_ADAPTER_OPERATION N6SdioWdi_StopOperation;

MINIPORT_WDI_ADAPTER_HANG_DIAGNOSE N6SdioWdi_HangDiagnose;

MINIPORT_WDI_TAL_TXRX_INITIALIZE N6SdioWdi_TalTxRxInitialize;

MINIPORT_WDI_TAL_TXRX_DEINITIALIZE N6SdioWdi_TalTxRxDeinitialize;

MINIPORT_WDI_TX_ABORT                   N6SdioWdi_TxAbort;

MINIPORT_WDI_TX_TARGET_DESC_INIT        N6SdioWdi_TxTargetDescInit;
MINIPORT_WDI_TX_TARGET_DESC_DEINIT      N6SdioWdi_TxTargetDescDeInit;

MINIPORT_WDI_TX_DATA_SEND               N6SdioWdi_TxDataSend;
MINIPORT_WDI_TX_TAL_SEND                N6SdioWdi_TxTalSend;

MINIPORT_WDI_TX_TAL_SEND_COMPLETE       N6SdioWdi_TxTalSendComplete;

MINIPORT_WDI_TX_TAL_QUEUE_IN_ORDER      N6SdioWdi_TxTalQueueInOrder;

MINIPORT_WDI_TX_PEER_BACKLOG            N6SdioWdi_TxPeerBacklog;

MINIPORT_WDI_RX_STOP                    N6SdioWdi_RxStop;
MINIPORT_WDI_RX_FLUSH                   N6SdioWdi_RxFlush;

MINIPORT_WDI_RX_RESTART                 N6SdioWdi_RxRestart;

MINIPORT_WDI_RX_GET_MPDUS               N6SdioWdi_RxGetMpdus;

MINIPORT_WDI_RX_RETURN_FRAMES           N6SdioWdi_RxReturnFrames;

MINIPORT_WDI_RX_RESUME                  N6SdioWdi_RxResume;

MINIPORT_WDI_RX_THROTTLE                N6SdioWdi_RxThrottle;

MINIPORT_WDI_RX_PPDU_RSSI               N6SdioWdi_RxPpduRssi;

MINIPORT_WDI_TAL_TXRX_START             N6SdioWdi_TalTxRxStart;
MINIPORT_WDI_TAL_TXRX_STOP              N6SdioWdi_TalTxRxStop;

MINIPORT_WDI_TAL_TXRX_ADD_PORT          N6SdioWdi_TalTxRxAddPort;
MINIPORT_WDI_TAL_TXRX_DELETE_PORT       N6SdioWdi_TalTxRxDeletePort;
MINIPORT_WDI_TAL_TXRX_SET_PORT_OPMODE   N6SdioWdi_TalTxRxSetPortOpMode;
MINIPORT_WDI_TAL_TXRX_RESET_PORT        N6SdioWdi_TalTxRxResetPort;

MINIPORT_WDI_TAL_TXRX_PEER_CONFIG       N6SdioWdi_TalTxRxPeerConfig;
MINIPORT_WDI_TAL_TXRX_PEER_DELETE_CONFIRM   N6SdioWdi_TalTxRxPeerDeleteConfirm;

RT_STATUS
N6SdioWdi_TargetDescInit(
	_In_  PADAPTER				pAdapter,
	_In_  PNET_BUFFER_LIST		pNBL
    );

NDIS_STATUS
N6SdioWdi_TaskOpen(
	_In_  PADAPTER			pAdapter,
	_In_  PNDIS_MINIPORT_INIT_PARAMETERS      MiniportInitParameters
	);
	
NDIS_STATUS
N6SdioWdi_TaskClose(
	_In_  PADAPTER			pAdapter
	);	

NDIS_STATUS
N6SdioWdi_AdapterCapabilities(
	_In_  PADAPTER			pAdapter,
	_In_  PRT_OID_HANDLER		pOidHandle
	);

VOID
N6SdioWdi_PnPSetPower(
	_In_  PADAPTER			Adapter
    );

NDIS_STATUS
N6SdioWdi_Mgnt_SetPower(
	_In_  PADAPTER			pAdapter,
	_In_  PRT_OID_HANDLER 	pOidHandle
	);

#endif //#ifndef __INC_N6CSDIO_WDIMAIN_H
