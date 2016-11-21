#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "NdisComm.tmh"
#endif


//
// Description:
//	Translate RT_STATUS to Ndis Status.
// Arguments:
//	[in] rtStatus -
//		RT defined status.
// Return:
//	NDIS_STATU_XXX
// Remark:
//	This function translates some known RT status to NDIS_STATUS_XXX.
//	If the input RT status is unrecognized, return NDIS_STATUS_FAILURE.
// By Bruce, 2012-03-07.
//
NDIS_STATUS
NdisStatusFromRtStatus(
	IN	u4Byte	rtStatus
	)
{	
	switch(rtStatus)
	{
	default:
		return NDIS_STATUS_FAILURE;
		
	case RT_STATUS_SUCCESS:
		return NDIS_STATUS_SUCCESS;

	case RT_STATUS_FAILURE:
		return NDIS_STATUS_FAILURE;

	case RT_STATUS_PENDING:
		return NDIS_STATUS_PENDING;

	case RT_STATUS_RESOURCE:
		return NDIS_STATUS_RESOURCES;

	case RT_STATUS_INVALID_CONTEXT:
	case RT_STATUS_INVALID_PARAMETER:
	case RT_STATUS_MALFORMED_PKT:
		return NDIS_STATUS_INVALID_DATA;

	case RT_STATUS_NOT_SUPPORT:
		return NDIS_STATUS_NOT_SUPPORTED;

	case RT_STATUS_BUFFER_TOO_SHORT:
		return NDIS_STATUS_BUFFER_TOO_SHORT;

	case RT_STATUS_INVALID_LENGTH:
		return NDIS_STATUS_INVALID_LENGTH;

	case RT_STATUS_NOT_RECOGNIZED:
		return NDIS_STATUS_NOT_RECOGNIZED;

	case RT_STATUS_MEDIA_BUSY:
		return NDIS_STATUS_MEDIA_BUSY;

	case RT_STATUS_INVALID_DATA:
		return NDIS_STATUS_INVALID_DATA;
	}
}


VOID
_PlatformInitializeSpinLock(
	PVOID				Adapter,
	RT_SPINLOCK_TYPE	type
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter((PADAPTER)Adapter);
	PNDIS_ADAPTER_TYPE	pDevice = GET_NDIS_ADAPTER(pDefaultAdapter);

	switch(type)
	{
	case RT_RX_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->RxSpinLock) );
		pDefaultAdapter->bRxLocked = FALSE;
		break;

	case RT_RX_CONTROL_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->RxControlSpinLock));
		break;

	case RT_RX_QUEUE_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->RxQueueSpinLock));
		break;

	case RT_TX_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->TxSpinLock) );
		pDefaultAdapter->bTxLocked = FALSE;
		break;

	case RT_RM_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->RmSpinLock) );
		break;

	case RT_CAM_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->CamSpinLock) );
		break;

	case RT_SCAN_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->ScanSpinLock) );
		break;
		
	case RT_LOG_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->LogSpinLock) );
		break;
		
	case RT_BW_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->BwSpinLock) );
		break;

	case RT_CHNLOP_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->ChnlOpSpinLock) );
		break;

	case RT_RF_OPERATE_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->RFWriteSpinLock));
		break;

	case RT_INITIAL_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->InitialSpinLock));
		break;
		
	case RT_RF_STATE_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->RFStateSpinLock));
		break;	

	case RT_PORT_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->PortLock) );
		break;
		
	case RT_H2C_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->H2CSpinLock));
		break;
		
#if VISTA_USB_RX_REVISE
	case RT_USBRX_CONTEXT_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->UsbRxContextLock) );
		break;

	case RT_USBRX_POSTPROC_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->UsbRxIndicateLock) );
		break;
#endif

	case RT_WAPI_OPTION_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->WapiOptionSpinLock));
		break;
	case RT_WAPI_RX_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->WapiRxSpinLock));
		break;

	case RT_BUFFER_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->BufferSpinLock));
		break;

	case RT_GEN_TEMP_BUF_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->GenBufSpinLock));
		break;
		
	case RT_AWB_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->AwbSpinLock));
		break;
		
	case RT_FW_PS_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->FwPSSpinLock));
		break;

	case RT_HW_TIMER_SPIN_LOCK:
		NdisAllocateSpinLock(&(pDevice->HwTimerSpinLock));
		break;
        
	case RT_P2P_SPIN_LOCK:
		NdisAllocateSpinLock(&(pDevice->P2PSpinLock));
		break;

    case RT_MPT_WI_SPINLOCK:
        NdisAllocateSpinLock(&(pDevice->MptWorkItemSpinLock));
		break;

	case RT_DBG_SPIN_LOCK:
		NdisAllocateSpinLock(&(pDevice->ndisDbgWorkItemSpinLock));
		break;

	case RT_IQK_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->IQKSpinLock));
		break;

	case RT_DYN_TXPWRTBL_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->DynTxPwrTblSpinLock));
		break;

	case RT_CHNLLIST_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->ChnlListSpinLock));
		break;

	case RT_PENDED_OID_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->PendedOidSpinLock));
		break;
		
	case RT_INDIC_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->IndicSpinLock));
		break;
		
	case RT_RFD_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->RfdSpinLock));
		break;
		
#if DRV_LOG_REGISTRY
	case RT_DRV_STATE_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->DrvStateSpinLock));
		break;
#endif

#if (AUTO_CHNL_SEL_NHM == 1)			
	case RT_ACS_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->AcsSpinLock));
		break;	
#endif

	case RT_RX_REF_CNT_SPINLOCK:
		NdisAllocateSpinLock(&(pDevice->RxRefCntSpinLock));
		break;

	case RT_SYNC_IO_CNT_SPINLOCK:
		NdisAllocateSpinLock( &(pDevice->SyncIoCntSpinLock));
		break;

	default:
		break;
	}
}


VOID
_PlatformFreeSpinLock(
	PVOID				Adapter,
	RT_SPINLOCK_TYPE	type
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter((PADAPTER)Adapter);

	PNDIS_ADAPTER_TYPE	pDevice = GET_NDIS_ADAPTER(pDefaultAdapter);

	switch(type)
	{
	case RT_RX_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->RxSpinLock) );
		break;

	case RT_RX_CONTROL_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->RxControlSpinLock));
		break;

	case RT_RX_QUEUE_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->RxQueueSpinLock));
		break;

	case RT_TX_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->TxSpinLock) );
		break;

	case RT_RM_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->RmSpinLock) );
		break;

	case RT_CAM_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->CamSpinLock) );
		break;

	case RT_SCAN_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->ScanSpinLock) );
		break;

	case RT_LOG_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->LogSpinLock) );
		break;

	case RT_BW_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->BwSpinLock) );
		break;

	case RT_CHNLOP_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->ChnlOpSpinLock) );
		break;

	case RT_RF_OPERATE_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->RFWriteSpinLock));
		break;

	case RT_INITIAL_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->InitialSpinLock));
		break;
		
	case RT_RF_STATE_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->RFStateSpinLock));
		break;	


	case RT_PORT_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->PortLock) );
		break;

	case RT_H2C_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->H2CSpinLock));
		break;

#if VISTA_USB_RX_REVISE
	case RT_USBRX_CONTEXT_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->UsbRxContextLock) );
		break;

	case RT_USBRX_POSTPROC_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->UsbRxIndicateLock) );
		break;
#endif

	case RT_WAPI_OPTION_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->WapiOptionSpinLock));
		break;
	case RT_WAPI_RX_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->WapiRxSpinLock));
		break;

	case RT_BUFFER_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->BufferSpinLock) );
		break;
		
	case RT_AWB_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->AwbSpinLock));
		break;

	case RT_FW_PS_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->FwPSSpinLock));
		break;

	case RT_HW_TIMER_SPIN_LOCK:
		NdisFreeSpinLock(&(pDevice->HwTimerSpinLock));
		break;

	case RT_P2P_SPIN_LOCK:
		NdisFreeSpinLock(&(pDevice->P2PSpinLock));
		break;

	case RT_MPT_WI_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->MptWorkItemSpinLock));
		break;

	case RT_DBG_SPIN_LOCK:
		NdisFreeSpinLock(&(pDevice->ndisDbgWorkItemSpinLock));
		break;

	case RT_IQK_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->IQKSpinLock));
		break;

	case RT_DYN_TXPWRTBL_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->DynTxPwrTblSpinLock));
		break;

	case RT_CHNLLIST_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->ChnlListSpinLock));
		break;		
		
	case RT_PENDED_OID_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->PendedOidSpinLock));
		break;
	case RT_RFD_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->RfdSpinLock));
		break;
				
	case RT_INDIC_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->IndicSpinLock));
		break;		

#if DRV_LOG_REGISTRY
	case RT_DRV_STATE_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->DrvStateSpinLock));
		break;
#endif	
				
#if (AUTO_CHNL_SEL_NHM == 1)	
	case RT_ACS_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->AcsSpinLock));
		break;
#endif
				
case RT_RX_REF_CNT_SPINLOCK:
		NdisFreeSpinLock(&(pDevice->RxRefCntSpinLock));
		break;

	case RT_SYNC_IO_CNT_SPINLOCK:
		NdisFreeSpinLock( &(pDevice->SyncIoCntSpinLock));
		break;
		
	default:
		break;
	}
}

#pragma warning( disable: 28167 ) // Prefast says we don't annotated the change.
_IRQL_raises_(DISPATCH_LEVEL)
VOID
_PlatformAcquireSpinLock(
	PVOID				Adapter,
	RT_SPINLOCK_TYPE	type
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter((PADAPTER)Adapter);
	PNDIS_ADAPTER_TYPE	pDevice = GET_NDIS_ADAPTER(pDefaultAdapter);

	switch(type)
	{
	case RT_RX_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->RxSpinLock) );
		pDefaultAdapter->bRxLocked = TRUE;
		break;

	case RT_RX_CONTROL_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->RxControlSpinLock));
		break;

	case RT_RX_QUEUE_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->RxQueueSpinLock));
		break;

	case RT_TX_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->TxSpinLock) );
		pDefaultAdapter->bTxLocked = TRUE;
		break;

	case RT_RM_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->RmSpinLock) );
		break;

	case RT_CAM_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->CamSpinLock) );
		break;

	case RT_SCAN_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->ScanSpinLock) );
		break;

	case RT_LOG_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->LogSpinLock) );
		break;

	case RT_BW_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->BwSpinLock) );
		break;

	case RT_CHNLOP_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->ChnlOpSpinLock) );
		break;

	case RT_RF_OPERATE_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->RFWriteSpinLock));
		break;

	case RT_INITIAL_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->InitialSpinLock));
		break;
		
	case RT_RF_STATE_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->RFStateSpinLock));
		break;	

	
	case RT_PORT_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->PortLock) );
		break;

	case RT_H2C_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->H2CSpinLock) );
		break;	
		
#if VISTA_USB_RX_REVISE
	case RT_USBRX_CONTEXT_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->UsbRxContextLock) );
		break;

	case RT_USBRX_POSTPROC_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->UsbRxIndicateLock) );
		break;
#endif

	case RT_WAPI_OPTION_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->WapiOptionSpinLock));
		break;
	case RT_WAPI_RX_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->WapiRxSpinLock));
		break;

	case RT_BUFFER_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->BufferSpinLock) );
		break;

	case RT_GEN_TEMP_BUF_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->GenBufSpinLock));
		break;

	case RT_AWB_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->AwbSpinLock));
		break;	
		
	case RT_FW_PS_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->FwPSSpinLock));
		break;

	case RT_HW_TIMER_SPIN_LOCK:
		NdisAcquireSpinLock(&(pDevice->HwTimerSpinLock));
		break;

	case RT_P2P_SPIN_LOCK:
		NdisAcquireSpinLock(&(pDevice->P2PSpinLock));
        break;

    case RT_MPT_WI_SPINLOCK:
        NdisAcquireSpinLock(&(pDevice->MptWorkItemSpinLock));
		break;
		
	case RT_DBG_SPIN_LOCK:
		NdisAcquireSpinLock(&(pDevice->ndisDbgWorkItemSpinLock));
		break;
		
	case RT_IQK_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->IQKSpinLock));
		break;

	case RT_DYN_TXPWRTBL_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->DynTxPwrTblSpinLock));
		break;

	case RT_CHNLLIST_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->ChnlListSpinLock));
		break;

	case RT_PENDED_OID_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->PendedOidSpinLock));
		break;
		
	case RT_INDIC_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->IndicSpinLock));
		break;

	case RT_RFD_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->RfdSpinLock));
		break;

#if DRV_LOG_REGISTRY
	case RT_DRV_STATE_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->DrvStateSpinLock));
		break;	
#endif
			
#if (AUTO_CHNL_SEL_NHM == 1)				
	case RT_ACS_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->AcsSpinLock));
		break;
#endif
			
	case RT_RX_REF_CNT_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->RxRefCntSpinLock));
		break;

	case RT_SYNC_IO_CNT_SPINLOCK:
		NdisAcquireSpinLock( &(pDevice->SyncIoCntSpinLock));
		break;


	case RT_CUSTOM_SCAN_SPINLOCK:
		NdisAcquireSpinLock(&(pDevice->CustomScanSpinLock));
		break;
			
	default:
		break;
	}
}

#pragma warning( disable: 28167 ) // Prefast says we don't annotated the change.
_IRQL_requires_(DISPATCH_LEVEL)
VOID
_PlatformReleaseSpinLock(
	PVOID				Adapter,
	RT_SPINLOCK_TYPE	type
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter((PADAPTER)Adapter);
	PNDIS_ADAPTER_TYPE	pDevice = GET_NDIS_ADAPTER(pDefaultAdapter);

	switch(type)
	{
	case RT_RX_SPINLOCK:
		pDefaultAdapter->bRxLocked = FALSE;	
		NdisReleaseSpinLock( &(pDevice->RxSpinLock) );
		break;

	case RT_RX_CONTROL_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->RxControlSpinLock));
		break;

	case RT_RX_QUEUE_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->RxQueueSpinLock));
		break;

	case RT_TX_SPINLOCK:
		pDefaultAdapter->bTxLocked = FALSE;	
		NdisReleaseSpinLock( &(pDevice->TxSpinLock) );
		break;

	case RT_RM_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->RmSpinLock) );
		break;

	case RT_CAM_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->CamSpinLock) );
		break;

	case RT_SCAN_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->ScanSpinLock) );
		break;

	case RT_LOG_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->LogSpinLock) );
		break;

	case RT_BW_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->BwSpinLock) );
		break;

	case RT_CHNLOP_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->ChnlOpSpinLock) );
		break;

	case RT_RF_OPERATE_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->RFWriteSpinLock));
		break;

	case RT_INITIAL_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->InitialSpinLock));
		break;
		
	case RT_RF_STATE_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->RFStateSpinLock));
		break;


	case RT_PORT_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->PortLock) );
		break;

	case RT_H2C_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->H2CSpinLock));
		break;
		
#if VISTA_USB_RX_REVISE
	case RT_USBRX_CONTEXT_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->UsbRxContextLock) );
		break;

	case RT_USBRX_POSTPROC_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->UsbRxIndicateLock) );
		break;
#endif

	case RT_WAPI_OPTION_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->WapiOptionSpinLock));
		break;
	case RT_WAPI_RX_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->WapiRxSpinLock));
		break;

	case RT_BUFFER_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->BufferSpinLock) );
		break;

	case RT_GEN_TEMP_BUF_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->GenBufSpinLock));
		break;

	case RT_AWB_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->AwbSpinLock));
		break;	

	case RT_FW_PS_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->FwPSSpinLock));
		break;

	case RT_HW_TIMER_SPIN_LOCK:
		NdisReleaseSpinLock(&(pDevice->HwTimerSpinLock));
		break;

	case RT_P2P_SPIN_LOCK:
		NdisReleaseSpinLock(&(pDevice->P2PSpinLock));
		break;

	case RT_MPT_WI_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->MptWorkItemSpinLock));
		break;
		
	case RT_DBG_SPIN_LOCK:
		NdisReleaseSpinLock(&(pDevice->ndisDbgWorkItemSpinLock));
		break;
		
	case RT_IQK_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->IQKSpinLock));
		break;

	case RT_DYN_TXPWRTBL_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->DynTxPwrTblSpinLock));
		break;

	case RT_CHNLLIST_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->ChnlListSpinLock));
		break;

	case RT_PENDED_OID_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->PendedOidSpinLock));
		break;

	case RT_RFD_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->RfdSpinLock));
		break;
		
	case RT_INDIC_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->IndicSpinLock));
		break;

#if DRV_LOG_REGISTRY
	case RT_DRV_STATE_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->DrvStateSpinLock));
		break;
#endif
		
#if (AUTO_CHNL_SEL_NHM == 1)		
	case RT_ACS_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->AcsSpinLock));
		break;
#endif
		
	case RT_RX_REF_CNT_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->RxRefCntSpinLock));
		break;

	case RT_SYNC_IO_CNT_SPINLOCK:
		NdisReleaseSpinLock( &(pDevice->SyncIoCntSpinLock));
		break;


	case RT_CUSTOM_SCAN_SPINLOCK:
		NdisReleaseSpinLock(&(pDevice->CustomScanSpinLock));
		break;
		
	default:
		break;
	}
}

u4Byte
PlatformAtomicExchange(
	pu4Byte				target,
	u4Byte				value
	)
{
	return InterlockedExchange(target, value);
}

// 20100211 Joseph: Since there is no support to InterlockedAnd() and InterlockedOr() function for XP platform,
// we just implement the same function as DDK does.
u4Byte
PlatformAtomicAnd(
	pu1Byte				target,
	u1Byte				value
	)
{
    u4Byte i, j;
    u4Byte tmp = (0xffffff00|value);

    j = *target;
    do {
	i = j;
	j = InterlockedCompareExchange((pu4Byte)target, (i&tmp), i);
    } while (i != j);

    return j;
}

// 20100211 Joseph: Since there is no support to InterlockedAnd() and InterlockedOr() function for XP platform,
// we just implement the same function as DDK does.
u4Byte
PlatformAtomicOr(
	pu1Byte				target,
	u1Byte				value
	)
{
    u4Byte i, j;
    u4Byte tmp = (0x00000000|value);

    j = *target;
    do {
	i = j;
	j = InterlockedCompareExchange((pu4Byte)target, (i|tmp), i);
    } while (i != j);

    return j;
}


VOID
PlatformInitializeMutex(
	IN PPlatformMutex	Mutex
	)
{
	KeInitializeMutex(Mutex, 0);
}

VOID
PlatformAcquireMutex(
	IN PPlatformMutex	Mutex
	)
{
	LARGE_INTEGER lTimeout;

	if (KeGetCurrentIrql() == DISPATCH_LEVEL) 
	{
		lTimeout.QuadPart = 0;
		KeWaitForMutexObject(Mutex, Executive, KernelMode, FALSE, &lTimeout);
	}
	else 
		KeWaitForMutexObject(Mutex, Executive, KernelMode, FALSE, NULL);
}


VOID
PlatformReleaseMutex(
	IN PPlatformMutex	Mutex
	)
{
	KeReleaseMutex(Mutex, FALSE);
}

VOID
PlatformCriticalSectionEnter(
	IN PADAPTER			pAdapter,
	IN PPlatformMutex	Mutex,
	IN RT_SPINLOCK_TYPE	Spinlock
	)
{
	PlatformAcquireMutex(Mutex);
}

VOID
PlatformCriticalSectionLeave(
	IN PADAPTER			pAdapter,
	IN PPlatformMutex	Mutex,
	IN RT_SPINLOCK_TYPE	Spinlock
	)
{
	PlatformReleaseMutex(Mutex);
}



//=========================================================================
//
// Get the unused instance index, if there is no index found, return 0xff
//
//=========================================================================


u8Byte
PlatformDivision64(
	IN u8Byte	x,
	IN u8Byte	y
)
{
	return ((x) / (y));
}

u8Byte
PlatformModular64(
	IN u8Byte	x,
	IN u8Byte	y
)
{
	return ((x) % (y));
}

//==========================================================================
//Platform function about Thread Create and Free
//==========================================================================

VOID
Ndis6EventTrigerThreadCallback(
	IN	PVOID	pContext
	)
{
	PRT_THREAD	pRtThread = (PRT_THREAD)pContext;
	PADAPTER	Adapter =  ((PRT_THREAD)pContext)->Adapter;
	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PRT_THREAD_PLATFORM_EXT	pPlatformExt = (PRT_THREAD_PLATFORM_EXT)pRtThread->pPlatformExt;

	if( pPlatformExt == NULL )
		return;

	NdisAcquireSpinLock(&(pPlatformExt->Lock));
	
	if(pRtThread->Status & RT_THREAD_STATUS_CANCEL)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Ndis6EventTrigerThreadCallback CANCEL! \n"));
		NdisReleaseSpinLock(&(pPlatformExt->Lock)); 
		goto Exit;
	}
	pRtThread->Status |=RT_THREAD_STATUS_FIRED;

	NdisReleaseSpinLock(&(pPlatformExt->Lock)); 

	while(!RT_DRIVER_STOP(Adapter))
	{
		NdisAcquireSpinLock(&(pPlatformExt->Lock));
		if(pRtThread->Status & RT_THREAD_STATUS_CANCEL)
		{
			NdisReleaseSpinLock(&(pPlatformExt->Lock));
			break;
		}
		else
			NdisReleaseSpinLock(&(pPlatformExt->Lock));
		
		if(NdisWaitEvent(&pPlatformExt->TrigerEvent, pPlatformExt->Period))
		{
			NdisAcquireSpinLock(&(pPlatformExt->Lock));
			
			NdisResetEvent(&(pPlatformExt->TrigerEvent));

			if(pRtThread->Status & RT_THREAD_STATUS_CANCEL)
			{
				NdisReleaseSpinLock(&(pPlatformExt->Lock));
				break;
			}
			else
				NdisReleaseSpinLock(&(pPlatformExt->Lock));

			pRtThread->CallbackFunc(pRtThread);
			
			NdisAcquireSpinLock(&(pPlatformExt->Lock));
			pRtThread->ScheduleCnt++;
			NdisReleaseSpinLock(&(pPlatformExt->Lock));
		}
	}
	
Exit:

	if(pRtThread->PreTheradExitCb != NULL)
	{			 
		pRtThread->PreTheradExitCb(pRtThread);	
	}
	
	NdisAcquireSpinLock(&(pPlatformExt->Lock));
	pRtThread->RefCnt--;
	pRtThread->Status &= ~RT_THREAD_STATUS_FIRED;
	pRtThread->Status &= ~RT_THREAD_STATUS_SET;
	NdisSetEvent(&(pPlatformExt->CompleteEvent));
	NdisReleaseSpinLock(&(pPlatformExt->Lock));

     if(Adapter->bSWInitReady)
      {
        	NdisAcquireSpinLock(&(pNdisCommon->ThreadLock));
        	
        	pNdisCommon->OutStandingThreadCnt--;

        	//
        	// Check if driver is waiting us to unload.
        	//	
        	if(pNdisCommon->OutStandingThreadCnt == 0)
        	{
        		NdisReleaseSpinLock(&(pNdisCommon->ThreadLock));
        		NdisSetEvent(&(pNdisCommon->AllThreadCompletedEvent));
        	}
        	else
        	{
        		NdisReleaseSpinLock(&(pNdisCommon->ThreadLock));
        	}	
     }

	PsTerminateSystemThread(STATUS_SUCCESS);// terminate the thread.

}

VOID
Ndis6ThreadCallback(
	IN	PVOID	pContext
	)
{
	PRT_THREAD	pRtThread = (PRT_THREAD)pContext;
	PADAPTER	Adapter =  ((PRT_THREAD)pContext)->Adapter;
	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PRT_THREAD_PLATFORM_EXT	pPlatformExt = (PRT_THREAD_PLATFORM_EXT)pRtThread->pPlatformExt;

	if( pPlatformExt == NULL )
		return;

	NdisAcquireSpinLock(&(pPlatformExt->Lock));
	
	if(pRtThread->Status & RT_THREAD_STATUS_CANCEL)
	{
		NdisReleaseSpinLock(&(pPlatformExt->Lock)); 
		goto Exit;
	}
	pRtThread->Status |=RT_THREAD_STATUS_FIRED;
	NdisReleaseSpinLock(&(pPlatformExt->Lock)); 


	if(!RT_DRIVER_STOP(Adapter))
	{
	
		pRtThread->CallbackFunc(pRtThread);
		
		NdisAcquireSpinLock(&(pPlatformExt->Lock));
		pRtThread->ScheduleCnt++;
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
	}
	
Exit:	
	NdisAcquireSpinLock(&(pPlatformExt->Lock));
	pRtThread->RefCnt--;
	pRtThread->Status &= ~RT_THREAD_STATUS_FIRED;
	pRtThread->Status &= ~RT_THREAD_STATUS_SET;
	NdisSetEvent(&(pPlatformExt->CompleteEvent));
	NdisReleaseSpinLock(&(pPlatformExt->Lock)); 

	NdisAcquireSpinLock(&(pNdisCommon->ThreadLock));
	pNdisCommon->OutStandingThreadCnt--;

	//
	// Check if driver is waiting us to unload.
	//	
	if(pNdisCommon->OutStandingThreadCnt == 0)
	{
		NdisReleaseSpinLock(&(pNdisCommon->ThreadLock));
		NdisSetEvent(&(pNdisCommon->AllThreadCompletedEvent));
	}
	else
	{
		NdisReleaseSpinLock(&(pNdisCommon->ThreadLock));
	}	

	PsTerminateSystemThread(STATUS_SUCCESS);// terminate the thread.
}

RT_STATUS
PlatformInitializeThread(
	IN	PADAPTER			Adapter,
	IN	PRT_THREAD			pRtThread,
	IN	RT_THREAD_CALL_BACK	CallBackFunc, 
	IN	const char*			szID,
	IN	BOOLEAN				EventTriger,
	IN	u4Byte				Period,
	IN	PVOID				pContext
	)
{
	PRT_NDIS_COMMON	pNdisCommon  = Adapter->pNdisCommon;
	PRT_THREAD_PLATFORM_EXT	pPlatformExt =NULL;
	RT_STATUS			status = RT_STATUS_SUCCESS;


	pRtThread->pPlatformExt = NULL;
	status = PlatformAllocateMemory(Adapter, (PVOID*)(&pPlatformExt), sizeof(RT_THREAD_PLATFORM_EXT) );
	if (status != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_DBG, DBG_SERIOUS, ("PlatformInitializeThread(): failed to allocate memory: %s\n", szID));
		return RT_STATUS_RESOURCE;
	}
	PlatformZeroMemory( pPlatformExt, sizeof(RT_THREAD_PLATFORM_EXT) );
	pRtThread->pPlatformExt = pPlatformExt;


	NdisAllocateSpinLock(&(pPlatformExt->Lock));
	NdisInitializeEvent(&(pPlatformExt->CompleteEvent));
	NdisSetEvent(&(pPlatformExt->CompleteEvent));
	NdisInitializeEvent(&(pPlatformExt->TrigerEvent));
	
	NdisSetEvent(&(pPlatformExt->TrigerEvent));

	pPlatformExt->Period = Period;
	
	pRtThread->bEventTriger  = EventTriger;
	pRtThread->Adapter= Adapter;
	pRtThread->Status = RT_THREAD_STATUS_INITIALIZED;
	pRtThread->RefCnt = 1;
	pRtThread->ScheduleCnt = 0;
	pRtThread->CallbackFunc = CallBackFunc;
	pRtThread->PreTheradExitCb = NULL;
	
	if(szID != NULL)
	{
		ASCII_STR_COPY(pRtThread->szID, szID, 36);
	}

	pRtThread->Argc = 0;
	pRtThread->pContext = pContext;
	
	return RT_STATUS_SUCCESS;
}

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
	)
{
	RT_STATUS				status = RT_STATUS_SUCCESS;
	
	status = PlatformInitializeThread(Adapter, pRtThread, CallBackFunc, szID, EventTriger, Period, pContext);

	if(RT_STATUS_SUCCESS == status)
	{
		pRtThread->PreTheradExitCb = PreThreadExitCb;
	}
	
	return status;
}

VOID
PlatformSetEventToTrigerThread(
	IN	PADAPTER		Adapter,
	IN	PRT_THREAD	pRtThread
)
{
	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;
        PRT_THREAD_PLATFORM_EXT	pPlatformExt = (PRT_THREAD_PLATFORM_EXT)pRtThread->pPlatformExt;

	if( pPlatformExt == NULL )
		return;

	NdisAcquireSpinLock(&(pPlatformExt->Lock));
	
	if(pRtThread->Status & RT_THREAD_STATUS_CANCEL)
	{
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		return;
	}
	NdisSetEvent(&(pPlatformExt->TrigerEvent));

	NdisReleaseSpinLock(&(pPlatformExt->Lock));
	return;
	
}

RT_STATUS
PlatformSetEventTrigerThread(
	IN	PADAPTER		Adapter,
	IN	PRT_THREAD	pRtThread,
	IN 	RT_THREAD_LEVEL 	Priority,
	IN	PVOID		pContext
    )
{
	HANDLE              ThreadHandle;
	NDIS_STATUS         Status;
	OBJECT_ATTRIBUTES   ObjectAttribs;

	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;
        PRT_THREAD_PLATFORM_EXT	pPlatformExt = (PRT_THREAD_PLATFORM_EXT)pRtThread->pPlatformExt;

	if( pPlatformExt == NULL )
		return RT_STATUS_FAILURE;

	if(RT_DRIVER_HALT(Adapter))
	{
		RT_TRACE(COMP_SYSTEM, DBG_LOUD, 
			("driver is halted pRtThread(%s) cannot schedule !!!\n", pRtThread->szID));
		return RT_STATUS_FAILURE;
	}
	
	NdisAcquireSpinLock(&(pPlatformExt->Lock));
	if(pRtThread->Status & RT_THREAD_STATUS_RELEASED)
	{
		RT_TRACE(COMP_SYSTEM, DBG_WARNING, 
			("pRtThread(%s) is alreadyreleased!!!\n", pRtThread->szID));
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		return RT_STATUS_FAILURE;
	}

	if(!(pRtThread->Status & RT_THREAD_STATUS_INITIALIZED))
	{
		RT_TRACE(COMP_SYSTEM, DBG_WARNING,
			("pRtThread(%s) is not yet initialized!!! Status 0x%x\n", pRtThread->szID, pRtThread->Status));
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		return RT_STATUS_FAILURE;
	}

	NdisReleaseSpinLock(&(pPlatformExt->Lock));

	if(!NdisWaitEvent(&(pPlatformExt->CompleteEvent),2000))
	{
		RT_TRACE(COMP_SYSTEM, DBG_LOUD, 
			("pRtThread(%s) is not Complete, cannot set again!!!\n", pRtThread->szID));
		return RT_STATUS_FAILURE;
	}

	NdisAcquireSpinLock(&(pPlatformExt->Lock));			

	NdisResetEvent(&(pPlatformExt->CompleteEvent));

	NdisReleaseSpinLock(&(pPlatformExt->Lock));
	
  	do
	{

		InitializeObjectAttributes(&ObjectAttribs, 
		                           NULL,              // ObjectName
		                           OBJ_KERNEL_HANDLE, 
		                           NULL,              // RootDir 
		                           NULL);             // Security Desc

		NdisAcquireSpinLock(&(pPlatformExt->Lock));
		pRtThread->RefCnt++;
		pRtThread->Status |= RT_THREAD_STATUS_SET;
		pRtThread->pContext = pContext;
		NdisReleaseSpinLock(&(pPlatformExt->Lock));

		NdisAcquireSpinLock(&(pNdisCommon->ThreadLock));
		pNdisCommon->OutStandingThreadCnt++;
		NdisReleaseSpinLock(&(pNdisCommon->ThreadLock));

		Status = PsCreateSystemThread(&ThreadHandle,
		                              THREAD_ALL_ACCESS,
		                              &ObjectAttribs,
		                              NULL,           // ProcessHandle
		                              NULL,           // ClientId
		                              Ndis6EventTrigerThreadCallback,
		                              pRtThread);
		
		if (!NT_SUCCESS(Status))
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Worker Thread creation failed with 0x%lx\n", Status));
			NdisAcquireSpinLock(&(pPlatformExt->Lock));
			pRtThread->RefCnt--;
			pRtThread->Status &= ~RT_THREAD_STATUS_SET;
			NdisReleaseSpinLock(&(pPlatformExt->Lock));

                   if(Adapter->bSWInitReady)
                   {
        			NdisAcquireSpinLock(&(pNdisCommon->ThreadLock));
        			pNdisCommon->OutStandingThreadCnt--;

        			if(pNdisCommon->OutStandingThreadCnt == 0)
        				NdisSetEvent(&(pNdisCommon->AllThreadCompletedEvent));
        			
        			NdisReleaseSpinLock(&(pNdisCommon->ThreadLock));
                   }

			NdisAcquireSpinLock(&(pPlatformExt->Lock));
			NdisSetEvent(&(pPlatformExt->CompleteEvent));
			NdisReleaseSpinLock(&(pPlatformExt->Lock));

	
			break;
		}

		Status = ObReferenceObjectByHandle(ThreadHandle,
		                                   THREAD_ALL_ACCESS,
						   NULL,
		                                   KernelMode,
		                                   &pPlatformExt->Thread,
		                                   NULL);
		//Need to check status return;	                                

		if (Priority != 0)
		{
			KeSetPriorityThread(pPlatformExt->Thread, (KPRIORITY)Priority);
		}

		ZwClose(ThreadHandle);

	}while(FALSE);


	return RT_STATUS_SUCCESS;
}

RT_STATUS
PlatformRunThread(
	IN	PADAPTER		Adapter,
	IN	PRT_THREAD	pRtThread,
	IN 	RT_THREAD_LEVEL 	Priority,
	IN	PVOID		pContext
    )
{
	HANDLE              ThreadHandle;
	NDIS_STATUS         Status;
	OBJECT_ATTRIBUTES   ObjectAttribs;

	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;
        PRT_THREAD_PLATFORM_EXT	pPlatformExt = (PRT_THREAD_PLATFORM_EXT)pRtThread->pPlatformExt;

	if( pPlatformExt == NULL )
		return RT_STATUS_FAILURE;

	if(RT_DRIVER_HALT(Adapter))
	{
		RT_TRACE(COMP_SYSTEM, DBG_LOUD, 
			("driver is halted pRtThread(%s) cannot schedule !!!\n", pRtThread->szID));
		return RT_STATUS_FAILURE;
	}
	
	NdisAcquireSpinLock(&(pPlatformExt->Lock));

	if(pRtThread->Status & RT_THREAD_STATUS_RELEASED)
	{
		RT_TRACE(COMP_SYSTEM, DBG_WARNING, ("pRtThread(%s) is alreadyreleased!!!\n", pRtThread->szID));
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		return RT_STATUS_FAILURE;
	}

	if(!(pRtThread->Status & RT_THREAD_STATUS_INITIALIZED))
	{
		RT_ASSERT(FALSE, ("pRtThread(%s) is not yet initialized!!! Status 0x%x\n", pRtThread->szID, pRtThread->Status));
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		return RT_STATUS_FAILURE;
	}

	NdisReleaseSpinLock(&(pPlatformExt->Lock));

	if(!NdisWaitEvent(&(pPlatformExt->CompleteEvent),2000))
	{
		RT_TRACE(COMP_SYSTEM, DBG_WARNING, 
			("pRtThread(%s) is not Complete, cannot set again!!!\n", pRtThread->szID));
		return RT_STATUS_FAILURE;
	}

	NdisAcquireSpinLock(&(pPlatformExt->Lock));

	NdisResetEvent(&(pPlatformExt->CompleteEvent));

	NdisReleaseSpinLock(&(pPlatformExt->Lock));
	
  	do
	{
		InitializeObjectAttributes(&ObjectAttribs, 
		                           NULL,              // ObjectName
		                           OBJ_KERNEL_HANDLE, 
		                           NULL,              // RootDir 
		                           NULL);             // Security Desc
	
		NdisAcquireSpinLock(&(pPlatformExt->Lock));
		pRtThread->RefCnt++;
		pRtThread->Status |= RT_THREAD_STATUS_SET;
		pRtThread->pContext = pContext;
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		NdisAcquireSpinLock(&(pNdisCommon->ThreadLock));
		pNdisCommon->OutStandingThreadCnt++;
		NdisReleaseSpinLock(&(pNdisCommon->ThreadLock));

		Status = PsCreateSystemThread(&ThreadHandle,
		                              THREAD_ALL_ACCESS,
		                              &ObjectAttribs,
		                              NULL,           // ProcessHandle
		                              NULL,           // ClientId
		                              Ndis6ThreadCallback,
		                              pRtThread);

		if (!NT_SUCCESS(Status))
		{
			RT_TRACE(COMP_SYSTEM, DBG_SERIOUS, ("Worker Thread creation failed with 0x%lx\n", Status));
			NdisAcquireSpinLock(&(pPlatformExt->Lock));
			pRtThread->RefCnt--;
			pRtThread->Status &= ~RT_THREAD_STATUS_SET;
			NdisReleaseSpinLock(&(pPlatformExt->Lock));

			NdisAcquireSpinLock(&(pNdisCommon->ThreadLock));
			pNdisCommon->OutStandingThreadCnt--;

			if(pNdisCommon->OutStandingThreadCnt == 0)
				NdisSetEvent(&(pNdisCommon->AllThreadCompletedEvent));
				
			NdisReleaseSpinLock(&(pNdisCommon->ThreadLock));
			NdisAcquireSpinLock(&(pPlatformExt->Lock));
			NdisSetEvent(&(pPlatformExt->CompleteEvent));
			NdisReleaseSpinLock(&(pPlatformExt->Lock));

	
			break;
		}

		Status = ObReferenceObjectByHandle(ThreadHandle,
		                                   THREAD_ALL_ACCESS,
		                                   NULL,
		                                   KernelMode,
		                                   &pPlatformExt->Thread,
		                                   NULL);
		//Need to check status return;	                                

		if (Priority != 0)
		{
			KeSetPriorityThread(pPlatformExt->Thread, (KPRIORITY)Priority);
		}

		ZwClose(ThreadHandle);

	}while(FALSE);


	return RT_STATUS_SUCCESS;
}



VOID
PlatformCancelThread(
	IN	PADAPTER	pAdapter,
	IN	PRT_THREAD pRtThread)
{
	PRT_NDIS_COMMON	pNdisCommon = pAdapter->pNdisCommon;
	PRT_THREAD_PLATFORM_EXT	pPlatformExt = (PRT_THREAD_PLATFORM_EXT)pRtThread->pPlatformExt;

	if( pPlatformExt == NULL )
		return ;

	NdisAcquireSpinLock(&(pPlatformExt->Lock));

	if(pRtThread->Status & RT_THREAD_STATUS_RELEASED || 
		(!(pRtThread->Status & RT_THREAD_STATUS_INITIALIZED)))
	{
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		return;
	}
	
	if(pRtThread->Status & RT_THREAD_STATUS_FIRED) // already fired and not finish. waiting for the thread complete.
	{
		pRtThread->Status |= RT_THREAD_STATUS_CANCEL;
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		NdisSetEvent(&(pPlatformExt->TrigerEvent));
		NdisWaitEvent(&(pPlatformExt->CompleteEvent),0);
	}
	else if(pRtThread->Status & RT_THREAD_STATUS_SET) // set but not fired. This thread would not be fired
	{
		pRtThread->Status |= RT_THREAD_STATUS_CANCEL;
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		NdisSetEvent(&(pPlatformExt->TrigerEvent));
		NdisWaitEvent(&(pPlatformExt->CompleteEvent),0);
	}
	else
	{	
		NdisReleaseSpinLock(&(pPlatformExt->Lock));

		// thread been canceled
		NdisAcquireSpinLock(&(pNdisCommon->ThreadLock));
		
		if(pNdisCommon->OutStandingThreadCnt == 0)
			NdisSetEvent(&(pNdisCommon->AllThreadCompletedEvent));
		
		NdisReleaseSpinLock(&(pNdisCommon->ThreadLock));
	}
}

VOID
PlatformWaitThreadEnd(
	IN	PADAPTER	pAdapter,
	IN	PRT_THREAD pRtThread)
{
	PRT_THREAD_PLATFORM_EXT	pPlatformExt = (PRT_THREAD_PLATFORM_EXT)pRtThread->pPlatformExt;

	u4Byte i = 0;

	if( pPlatformExt == NULL )
		return ;

	NdisAcquireSpinLock(&(pPlatformExt->Lock));

	if(pRtThread->Status & RT_THREAD_STATUS_RELEASED || 
		(!(pRtThread->Status & RT_THREAD_STATUS_INITIALIZED)))
	{
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		return;
	} 

	if( !(pRtThread->Status & RT_THREAD_STATUS_SET) )
	{
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		return;
	}

	NdisReleaseSpinLock(&(pPlatformExt->Lock));

	KeWaitForSingleObject(pPlatformExt->Thread,Executive, KernelMode, FALSE, NULL);
	ObDereferenceObject(pPlatformExt->Thread);

}

VOID
PlatformReleaseThread(
	IN	PADAPTER	pAdapter,
	IN	PRT_THREAD pRtThread)
{
	PRT_THREAD_PLATFORM_EXT	pPlatformExt = (PRT_THREAD_PLATFORM_EXT)pRtThread->pPlatformExt;	

	if( pPlatformExt == NULL )
		return ;

	NdisAcquireSpinLock(&(pPlatformExt->Lock));

	if(pRtThread->Status & RT_THREAD_STATUS_RELEASED || 
		(!(pRtThread->Status & RT_THREAD_STATUS_INITIALIZED)))
	{
		NdisReleaseSpinLock(&(pPlatformExt->Lock));
		return;
	}

	pRtThread->Status |= RT_THREAD_STATUS_RELEASED;

	//Fix S3 BSOD, by YJ,120719
	pRtThread->pPlatformExt = NULL;	
	
	NdisReleaseSpinLock(&(pPlatformExt->Lock));
	NdisFreeSpinLock(&(pPlatformExt->Lock));  // may have some issue?

	PlatformFreeMemory( pPlatformExt, sizeof(RT_THREAD_PLATFORM_EXT) );
	
}

//
//	Description:
//		This routine  initializes a semaphore object with a specified count and 
//	specifies an upper limit that the count can attain.
//
//	Created by Roger, 2011.01.17.
//
VOID
PlatformInitializeSemaphore(
	IN 	PPlatformSemaphore	Sema,
	IN	u4Byte	InitCnt
	)
{

	//
	// We create this  semaphore with an initial count of zero to block access to the protected resource 
	// while the application is being initialized. After initialization, we can use ReleaseSemaphore to increment 
	// the count to the maximum value.
	//
	KeInitializeSemaphore(
					Sema, 
					InitCnt, // Count
					SEMA_UPBND); // Limit						
}


//
//	Description:
//		This routine waits for the semaphore object state to become signaled.
//
//	Created by Roger, 2011.01.17.
//
RT_STATUS
PlatformAcquireSemaphore(
	IN 	PPlatformSemaphore	Sema
	)
{	
	RT_STATUS	status = RT_STATUS_FAILURE;
	
	if( STATUS_SUCCESS == KeWaitForSingleObject(Sema, Executive, KernelMode, TRUE, NULL) )
		status = RT_STATUS_SUCCESS;	

	return status;	
}


//
//	Description:
//		This routine increases a semaphore's count by a specified amount.
//
//	Created by Roger, 2011.01.17.
//
VOID
PlatformReleaseSemaphore(
	IN 	PPlatformSemaphore	Sema
	)
{
	if (Sema->Header.Type == 0)
		return;
    
	KeReleaseSemaphore(
					Sema, 
					IO_NETWORK_INCREMENT, // Priority increment
					1,  // Adjustment, a value to be added
					FALSE ); // Will  not be followed immediately by a call to one of the KeWaitXxx routines
}

//
//	Description:
//		This routine frees a semaphore object 
//
//	Created by Roger, 2011.01.17.
//
VOID
PlatformFreeSemaphore(
	IN 	PPlatformSemaphore	Sema
	)
{
	if (Sema->Header.Type == 0)
		return;
}

//
//	Description:
//		This routine  initializes a event object 
//
//	Created by Cosa, 2012.01.09.
//
VOID
PlatformInitializeEvent(
	IN 	PPlatformEvent	pEvent
	)
{
	NdisInitializeEvent(pEvent);
}

VOID
PlatformResetEvent(
	IN 	PPlatformEvent	pEvent
	)
{
	NdisResetEvent(pEvent);
}

VOID
PlatformSetEvent(
	IN 	PPlatformEvent	pEvent
	)
{
	NdisSetEvent(pEvent);
}

VOID
PlatformFreeEvent(
	IN 	PPlatformEvent	pEvent
	)
{

}

BOOLEAN
PlatformWaitEvent(
	IN 	PPlatformEvent	pEvent,
	IN	u4Byte			msToWait
	)
{
	return NdisWaitEvent(pEvent, msToWait);
}


//
// 2011/04/08 MH Merge for return pending packet for WINDOWS.
//
VOID
PlatformReturnAllPendingTxPackets(
	IN 	PADAPTER 		pAdapter
	)
{
	N6SdioReturnAllPendingTxPackets(pAdapter);
}


VOID
PlatformRestoreLastInitSetting(
	IN 	PADAPTER 		pAdapter
	)
{
	N6RestoreLastInitSettingAterWakeUP(pAdapter);
}

BOOLEAN
PlatformIsOverrideAddress(
	IN	PADAPTER	Adapter
	)
{
	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;

	if(pNdisCommon->bOverrideAddress)
	{
		u1Byte	nulladdr[6] = {0, 0, 0, 0, 0, 0};
		if(PlatformCompareMemory(nulladdr, pNdisCommon->CurrentAddress, 6) == 0)
			return FALSE;
	}

	return pNdisCommon->bOverrideAddress;
}

pu1Byte
PlatformGetOverrideAddress(
	IN	PADAPTER	Adapter
	)
{
	PRT_NDIS_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	return pNdisCommon->CurrentAddress;
}	

//===================================
// If we are goting to stop vwifi support, then it will drop 
// all connections connected with the vwifi and indicate 
// to OS that can't support anymore. Otherwise, when we
// need to support vwifi again, we need to 
// indicate to OS that we can sustain vwifi.
//===================================
VOID
PlatformExtApSupport(
	IN	PADAPTER	Adapter,
	IN	BOOLEAN		bSupport
	)
{
	PADAPTER		pExtAdapter;
	pExtAdapter = GetFirstExtAdapter(Adapter);

	if( NULL != pExtAdapter)
	{
		if(!bSupport)
		{
			N62CStopApMode(pExtAdapter);
		}
		else
		{
			N62CApIndicateCanSustainAp(pExtAdapter);
		}
	}
}

BOOLEAN
PlatformInitReady(
	IN	PADAPTER	Adapter
	)
{
	if(!N6_INIT_READY(Adapter))
	{
		return FALSE;
	}
	else
		return TRUE;
}

VOID
Dot11_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);	
	PRT_CHANNEL_INFO	pChnlInfo = pMgntInfo->pChannelInfo;
	PRT_NDIS_COMMON		pNdisCommon = pAdapter->pNdisCommon;
	EXTCHNL_OFFSET		extchnl = EXTCHNL_OFFSET_NO_EXT;

	// SSID.
	CopySsid(pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, pNdisCommon->RegSSID.Octet, pNdisCommon->RegSSID.Length);

	// Default wireless mode.
	pAdapter->RegOrigWirelessMode	= (WIRELESS_MODE)(pNdisCommon->RegWirelessMode); 
	pAdapter->RegHTMode		  		= (HT_MODE)(pNdisCommon->RegHTMode);
	
	if(pAdapter->RegHTMode == HT_MODE_UNDEFINED)
	{
		pAdapter->RegWirelessMode		= pAdapter->RegOrigWirelessMode;
		pAdapter->RegOrigWirelessMode	= 0xFFFF;
		pAdapter->RegHTMode				= HT_MODE_VHT;
	}
	else
	{
		if(IS_WIRELESS_MODE_A_ONLY(pAdapter->RegOrigWirelessMode))	// A only
		{
			if(IS_VHT_SUPPORTED(pAdapter))
				pAdapter->RegWirelessMode = WIRELESS_MODE_AC_5G;
			else if(IS_HT_SUPPORTED(pAdapter))
				pAdapter->RegWirelessMode = WIRELESS_MODE_N_5G;
			else
				pAdapter->RegWirelessMode = WIRELESS_MODE_A;
		}
		else if(IS_WIRELESS_MODE_B_ONLY(pAdapter->RegOrigWirelessMode))	// B only
		{
			pAdapter->RegWirelessMode	= WIRELESS_MODE_B;
			pAdapter->RegHTMode			= HT_MODE_DISABLE;
		}
		else if(IS_WIRELESS_MODE_G_AND_BG(pAdapter->RegOrigWirelessMode))	// G only, B/G
		{
			if(IS_HT_SUPPORTED(pAdapter))
				pAdapter->RegWirelessMode = WIRELESS_MODE_N_24G;
			else
				pAdapter->RegWirelessMode = WIRELESS_MODE_G;
		}
		else
		{
			pAdapter->RegWirelessMode = WIRELESS_MODE_AUTO;	// A/G, A/B/G
		}

	}

	{
		pMgntInfo->Regdot11ChannelNumber = (u1Byte)(pNdisCommon->RegChannel);

		if(pMgntInfo->Regdot11ChannelNumber <= 14)
		{
			if(IS_5G_WIRELESS_MODE(pAdapter->RegWirelessMode))
				pMgntInfo->Regdot11ChannelNumber = 36;
		}
		else
		{
			if(IS_24G_WIRELESS_MODE(pAdapter->RegWirelessMode))
				pMgntInfo->Regdot11ChannelNumber = 10;
		}
	}

	if(GetDefaultAdapter(pAdapter)->bInHctTest)
		pMgntInfo->Regdot11ChannelNumber = 10;

	pMgntInfo->dot11CurrentChannelNumber = pMgntInfo->Regdot11ChannelNumber;
	pChnlInfo->PrimaryChannelNumber = pMgntInfo->dot11CurrentChannelNumber;
	pChnlInfo->CurrentChannelCenterFrequency = pMgntInfo->dot11CurrentChannelNumber;


	if(pAdapter->bInHctTest)
		pChnlInfo->RegBWSetting = CHANNEL_WIDTH_20;
	else
		pChnlInfo->RegBWSetting = pNdisCommon->RegBWSetting;

	// Auto select channel.
	pMgntInfo->bAutoSelChnl = (BOOLEAN)pNdisCommon->RegAutoSelChnl;
	pMgntInfo->ChnlWeightMode = (u1Byte)pNdisCommon->RegChnlWeight;
	
	// Force Tx rate.
	pMgntInfo->ForcedDataRate = pNdisCommon->RegForcedDataRate;

	// Set the BeaconType By Bruce, 2011-01-18.
	pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_HW_BEACON_SUPPORT, (pu1Byte)&(pNdisCommon->RegEnableSwBeacon));
	
	// AP mode related. 2005.05.30, by rcnjko.
	pMgntInfo->Regdot11BeaconPeriod = (u2Byte)(pNdisCommon->RegBeaconPeriod);
	pMgntInfo->dot11BeaconPeriod = pMgntInfo->Regdot11BeaconPeriod;
	pMgntInfo->Regdot11DtimPeriod = (u1Byte)(pNdisCommon->RegDtimPeriod); 
	pMgntInfo->dot11DtimPeriod = pMgntInfo->Regdot11DtimPeriod; 
	pMgntInfo->RegPreambleMode = (u1Byte)(pNdisCommon->RegPreambleMode); 
	pMgntInfo->dot11CurrentPreambleMode = pMgntInfo->RegPreambleMode;

#if 0
	// Channel / BW setting
	if(pChnlInfo->RegBWSetting == CHANNEL_WIDTH_20)
		extchnl = EXTCHNL_OFFSET_NO_EXT;
	else
	{
		if(CHNL_IsLegal5GChannel(pAdapter, pMgntInfo ->Regdot11ChannelNumber))
			extchnl = CHNL_GetExt20OffsetOf5G(pMgntInfo->dot11CurrentChannelNumber);
		else if(pChnlInfo->RegBWSetting == CHANNEL_WIDTH_80)
		{
			if(pMgntInfo->dot11CurrentChannelNumber == 1 || pMgntInfo->dot11CurrentChannelNumber == 9)
				extchnl = EXTCHNL_OFFSET_UPPER;
			else 
				extchnl = EXTCHNL_OFFSET_LOWER;
		}
		else 
			extchnl =(pMgntInfo->dot11CurrentChannelNumber<=7)? EXTCHNL_OFFSET_UPPER:EXTCHNL_OFFSET_LOWER;
	}	
	pMgntInfo->SettingBeforeScan.ChannelNumber = pMgntInfo->Regdot11ChannelNumber;
	pMgntInfo->SettingBeforeScan.ChannelBandwidth = pChnlInfo->RegBWSetting;
	pMgntInfo->SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz = extchnl;
	pMgntInfo->SettingBeforeScan.Ext40MHzChnlOffsetOf80MHz = extchnl;
	pMgntInfo->SettingBeforeScan.CenterFrequencyIndex1 = CHNL_GetCenterFrequency(pMgntInfo->SettingBeforeScan.ChannelNumber, pChnlInfo->RegBWSetting, extchnl);
#endif

}	


VOID
HT_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
)
{
	PMGNT_INFO					pMgntInfo = &(pAdapter->MgntInfo);	
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_NDIS_COMMON			pNdisCommon = pAdapter->pNdisCommon;

	BOOLEAN						bHwLDPCSupport = FALSE, bHwSTBCSupport = FALSE;
	BOOLEAN						bHwSupportBeamformer = FALSE, bHwSupportBeamformee = FALSE;

	if(pMgntInfo->bWiFiConfg)
		pHTInfo->bBssCoexist = TRUE;
	else
		pHTInfo->bBssCoexist = FALSE;

	pHTInfo->bRegBW40MHzFor2G = pNdisCommon->bRegBW40MHzFor2G;
	pHTInfo->bRegBW40MHzFor5G = pNdisCommon->bRegBW40MHzFor5G;

	// 11n adhoc support
	pMgntInfo->bReg11nAdhoc = pNdisCommon->bReg11nAdhoc;

	// 11ac Bcm 256QAM support
	pMgntInfo->bRegIOTBcm256QAM = pNdisCommon->bRegIOTBcm256QAM;
	
	// CCK rate support in 40MHz channel
	if(pNdisCommon->RegBWSetting)
		pHTInfo->bRegSuppCCK = pNdisCommon->bRegHT_EnableCck;
	else
		pHTInfo->bRegSuppCCK = TRUE;

	// AMSDU related
	pHTInfo->nAMSDU_MaxSize = (u2Byte)pNdisCommon->RegAMSDU_MaxSize;
	pHTInfo->bAMSDU_Support = (BOOLEAN)pNdisCommon->RegAMSDU;
	
	pHTInfo->bHWAMSDU_Support = (BOOLEAN)pNdisCommon->RegHWAMSDU;

	// AMPDU related
	if(pAdapter->bInHctTest)
		pHTInfo->bAMPDUEnable = FALSE;
	else	
		pHTInfo->bAMPDUEnable = (BOOLEAN)pNdisCommon->bRegAMPDUEnable;

	pHTInfo->AMPDU_Factor = (u1Byte)pNdisCommon->RegAMPDU_Factor;
	pHTInfo->MPDU_Density = (u1Byte)pNdisCommon->RegMPDU_Density;

	// Accept ADDBA Request
	pHTInfo->bAcceptAddbaReq = (BOOLEAN)(pNdisCommon->bRegAcceptAddbaReq);

	// MIMO Power Save
	pHTInfo->SelfMimoPs = (u1Byte)pNdisCommon->RegMimoPs;
	if(pHTInfo->SelfMimoPs == 2)
		pHTInfo->SelfMimoPs = 3;
	
	// For Rx Reorder Control
	// bRegRxReorderEnable default is FALSE if Rxreorder is swithed by TP since TP should be low as media just connect. 
	pHTInfo->bRegRxReorderEnable = ((pMgntInfo->RegRxReorder = pNdisCommon->RegRxReorder) ==2) ? FALSE : pNdisCommon->RegRxReorder;
	pHTInfo->RxReorderWinSize = pMgntInfo->RegRxReorder_WinSize = pNdisCommon->RegRxReorder_WinSize;
	pHTInfo->RxReorderPendingTime = pMgntInfo->RegRxReorder_PendTime = pNdisCommon->RegRxReorder_PendTime;
	pHTInfo->bRegShortGI40MHz = TEST_FLAG(pNdisCommon->RegShortGISupport, BIT1) ? TRUE : FALSE;
	pHTInfo->bRegShortGI20MHz = TEST_FLAG(pNdisCommon->RegShortGISupport, BIT0) ? TRUE : FALSE;

	pHTInfo->b40Intolerant = pNdisCommon->bReg40Intolerant;
	pHTInfo->bAMPDUManual = pNdisCommon->bRegAMPDUManual;
	pHTInfo->nTxSPStream = pNdisCommon->RegTxSPStream;

	//IOT issue with Atheros AP. If set incorrect, the Rx rate will always be MCS4. 
	if(pHTInfo->nRxSPStream == 0)
	{
		u1Byte 	Rf_Type = RT_GetRFType(pAdapter);
			
		if(Rf_Type == RF_1T2R || Rf_Type == RF_2T2R)
			pHTInfo->nRxSPStream = 2;
		else if(Rf_Type == RF_3T3R || Rf_Type == RF_2T4R || Rf_Type == RF_4T4R || Rf_Type == RF_2T3R)
			pHTInfo->nRxSPStream = 3;
		else
			pHTInfo->nRxSPStream = 1;
	}

	if(pHTInfo->nTxSPStream == 0)
	{
		u1Byte 	Rf_Type = RT_GetRFType(pAdapter);

		if(Rf_Type == RF_1T2R || Rf_Type == RF_1T1R)
			pHTInfo->nTxSPStream = 1;
		else if(Rf_Type == RF_3T3R || Rf_Type == RF_4T4R)
			pHTInfo->nTxSPStream = 3;
		else
			pHTInfo->nTxSPStream = 2;
	
	}

	// LDPC support
	CLEAR_FLAGS(pHTInfo->HtLdpcCap);
	
	// LDPC - Tx
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_TX_LDPC, (PBOOLEAN)&bHwLDPCSupport);
	if(bHwLDPCSupport)
	{
		if(TEST_FLAG(pNdisCommon->RegLdpc, BIT5))
			SET_FLAG(pHTInfo->HtLdpcCap, LDPC_HT_ENABLE_TX);
	}
	// LDPC - Rx
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_RX_LDPC, (PBOOLEAN)&bHwLDPCSupport);
	if(bHwLDPCSupport)
	{
		if(TEST_FLAG(pNdisCommon->RegLdpc, BIT4))
			SET_FLAG(pHTInfo->HtLdpcCap, LDPC_HT_ENABLE_RX);

	}
	RT_TRACE_F(COMP_HT, DBG_LOUD, ("[HT] Support LDOC = 0x%02X\n", pHTInfo->HtLdpcCap));

	// STBC
	if(pMgntInfo->bWiFiConfg)
		bHwSTBCSupport = FALSE;
	else
		pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_TX_STBC, (PBOOLEAN)&bHwSTBCSupport);
	CLEAR_FLAGS(pHTInfo->HtStbcCap);
	if(bHwSTBCSupport)
	{
		if(TEST_FLAG(pNdisCommon->RegStbc, BIT5))
			SET_FLAG(pHTInfo->HtStbcCap, STBC_HT_ENABLE_TX);

	}
	
	if(pMgntInfo->bWiFiConfg)
		bHwSTBCSupport = FALSE;
	else
		pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_RX_STBC, (PBOOLEAN)&bHwSTBCSupport);
	if(bHwSTBCSupport)
	{
		if(TEST_FLAG(pNdisCommon->RegStbc, BIT4))
			SET_FLAG(pHTInfo->HtStbcCap, STBC_HT_ENABLE_RX);
	}

	RT_TRACE_F(COMP_HT, DBG_LOUD, ("[HT] Support STBC = 0x%02X\n", pHTInfo->HtStbcCap));



}


VOID
VHT_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
)
{
	PMGNT_INFO					pMgntInfo = &(pAdapter->MgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	PRT_NDIS_COMMON			pNdisCommon = pAdapter->pNdisCommon;

	BOOLEAN						bHwLDPCSupport = FALSE, bHwSTBCSupport = FALSE;
	BOOLEAN						bHwSupportBeamformer = FALSE, bHwSupportBeamformee = FALSE;
	BOOLEAN						bHwSupportMuMimoAp = FALSE, bHwSupportMuMimoSta = FALSE;

	if(pNdisCommon->RegBWSetting == 2)
	{
		pVHTInfo->bRegBW80MHz = TRUE;
		pVHTInfo->bRegShortGI80MHz = TEST_FLAG(pNdisCommon->RegShortGISupport, BIT2) ? TRUE : FALSE;
	}
	else
	{
		pVHTInfo->bRegBW80MHz = FALSE;
		pVHTInfo->bRegShortGI80MHz = FALSE;
	}

	//pVHTInfo->bRegShortGI80MHz = TEST_FLAG(pNdisCommon->RegShortGISupport, BIT2) ? TRUE : FALSE;
	RT_TRACE(COMP_INIT, DBG_LOUD, ("bRegShortGI80MHz = %d\n", pVHTInfo->bRegShortGI80MHz));

	pVHTInfo->nTxSPStream = pNdisCommon->RegTxSPStream;
	pVHTInfo->nRxSPStream = pNdisCommon->RegRxSPStream;

	//IOT issue with Atheros AP. If set incorrect, the Rx rate will always be MCS4. 
	if(pVHTInfo->nRxSPStream == 0)
	{
		u1Byte 	Rf_Type = RT_GetRFType(pAdapter);
			
		if(Rf_Type == RF_1T2R || Rf_Type == RF_2T2R)
			pVHTInfo->nRxSPStream = 2;
		else if(Rf_Type == RF_3T3R || Rf_Type == RF_2T4R || Rf_Type == RF_4T4R || Rf_Type == RF_2T3R)
			pVHTInfo->nRxSPStream = 3;
		else
			pVHTInfo->nRxSPStream = 1;
	}

	if(pVHTInfo->nTxSPStream == 0)
	{
		u1Byte 	Rf_Type = RT_GetRFType(pAdapter);

		if(Rf_Type == RF_1T2R || Rf_Type == RF_1T1R)
			pVHTInfo->nTxSPStream = 1;
		else if(Rf_Type == RF_3T3R || Rf_Type == RF_4T4R)
			pVHTInfo->nTxSPStream = 3;
		else
			pVHTInfo->nTxSPStream = 2;
	
	}

	// LDPC support
	CLEAR_FLAGS(pVHTInfo->VhtLdpcCap);

	// LDPC - Tx
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_TX_LDPC, (PBOOLEAN)&bHwLDPCSupport);
	if(bHwLDPCSupport)
	{
		if(TEST_FLAG(pNdisCommon->RegLdpc, BIT1))
			SET_FLAG(pVHTInfo->VhtLdpcCap, LDPC_VHT_ENABLE_TX);
	}
	// LDPC - Rx
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_RX_LDPC, (PBOOLEAN)&bHwLDPCSupport);
	if(bHwLDPCSupport)
	{
		if(TEST_FLAG(pNdisCommon->RegLdpc, BIT0))
			SET_FLAG(pVHTInfo->VhtLdpcCap, LDPC_VHT_ENABLE_RX);
	}
	RT_TRACE_F(COMP_INIT, DBG_LOUD, ("[VHT] Support LDPC = 0x%02X\n", pVHTInfo->VhtLdpcCap));

	// STBC
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_TX_STBC, (PBOOLEAN)&bHwSTBCSupport);
	CLEAR_FLAGS(pVHTInfo->VhtStbcCap);
	if(bHwSTBCSupport)
	{
		if(TEST_FLAG(pNdisCommon->RegStbc, BIT1))
			SET_FLAG(pVHTInfo->VhtStbcCap, STBC_VHT_ENABLE_TX);

	}
	pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_RX_STBC, (PBOOLEAN)&bHwSTBCSupport);
	if(bHwSTBCSupport)
	{
		if(TEST_FLAG(pNdisCommon->RegStbc, BIT0))
			SET_FLAG(pVHTInfo->VhtStbcCap, STBC_VHT_ENABLE_RX);
	}
	RT_TRACE_F(COMP_INIT, DBG_LOUD, ("[VHT] Support STBC = 0x%02X\n", pVHTInfo->VhtStbcCap));


}

VOID 
PSC_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
)
{
	PMGNT_INFO					pMgntInfo = &(pAdapter->MgntInfo);	
	PRT_NDIS_COMMON			pNdisCommon = pAdapter->pNdisCommon;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	// For SDIO DPC ISR WHQL test setting.
	// We should enable power saving mode to pass DPC USR test.
	pMgntInfo->bSdioDpcIsr = (BOOLEAN)pNdisCommon->RegSdioDpcIsr;

	//3 All Power Save Mechanism.
	if((pAdapter->bInHctTest  || pMgntInfo->bWiFiConfg) && !pMgntInfo->bSdioDpcIsr)
	{
#if 0	
		if(pAdapter->bDPCISRTest)
		{
			pPSC->bInactivePs = TRUE;
			pPSC->bLeisurePs = TRUE;
			pMgntInfo->keepAliveLevel =pNdisCommon->RegKeepAliveLevel;
			pPSC->bFwCtrlLPS = TRUE;
			pPSC->FWCtrlPSMode = FW_PS_MIN_MODE;
			DbgPrint("test set leisurePS and inactivePS to TRUE\n");
		}
		else
#endif			
		{
			pPSC->bInactivePs = FALSE;
			pNdisCommon->RegInactivePsMode = eIpsOff;
			
			pPSC->bLeisurePs = FALSE;
			pNdisCommon->RegLeisurePsMode = eLpsOff;			
				
			pMgntInfo->keepAliveLevel =0;

			pPSC->bFwCtrlLPS = FALSE;
			pNdisCommon->bRegFwCtrlLPS =0;
			pPSC->bLeisurePsModeBackup = FALSE;
			
			pPSC->FWCtrlPSMode = FW_PS_ACTIVE_MODE;
		}
	}	
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("pNdisCommon->RegInactivePsMode = %d\n", pNdisCommon->RegInactivePsMode));	
		if(pNdisCommon->RegInactivePsMode == eIpsAuto)
			pPSC->bInactivePs = FALSE;
		else
			pPSC->bInactivePs = pNdisCommon->RegInactivePsMode;

		pPSC->bLeisurePsModeBackup = FALSE;

		//2008.08.25 
		if( (pNdisCommon->RegLeisurePsMode==eLpsOn) && (pMgntInfo->dot11PowerSaveMode == eActive) ) 
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("LeisurePs On!!\n"));
			pPSC->bLeisurePs = TRUE;
		}
		else	 if( (pNdisCommon->RegLeisurePsMode==eLpsAuto) && (pMgntInfo->dot11PowerSaveMode == eActive) )
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("LeisurePs Off!!\n"));
			pPSC->bLeisurePs = FALSE;
		}
		else
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("LeisurePs Off!!\n"));
			pPSC->bLeisurePs = FALSE;
		}
		pMgntInfo->keepAliveLevel =pNdisCommon->RegKeepAliveLevel;
	
		//Fw Control LPS mode
		if(pNdisCommon->bRegFwCtrlLPS == 0)
			pPSC->bFwCtrlLPS = FALSE;
		else
			pPSC->bFwCtrlLPS = TRUE;
		
		// We need to assign the FW PS mode value even if we do not enable LPS mode. 2012.03.05. by tynli.
		if(pNdisCommon->bRegFwCtrlLPS == 0)
			pPSC->FWCtrlPSMode = FW_PS_ACTIVE_MODE;
		else if(pNdisCommon->bRegFwCtrlLPS == 1)
			pPSC->FWCtrlPSMode = FW_PS_MIN_MODE;
		else if(pNdisCommon->bRegFwCtrlLPS == 2)
			pPSC->FWCtrlPSMode = FW_PS_MAX_MODE;
		else if(pNdisCommon->bRegFwCtrlLPS == 3)
			pPSC->FWCtrlPSMode = FW_PS_SELF_DEFINED_MODE;

		
//LPS 32K Support and Enable Deep Sleep
//by sherry 20150910
		if((pNdisCommon->RegLeisurePsMode != eLpsOff) && (pNdisCommon->bRegLowPowerEnable) && (pNdisCommon->bRegLPSDeepSleepEnable))
		{
			RT_TRACE(COMP_INIT,DBG_LOUD,("PSC_UpdateDefaultSetting: Enable LPS Deep Sleep \n"));
			FwLPSDeepSleepInit(pAdapter);
		}	
	}

	pPSC->bIPSModeBackup = pPSC->bInactivePs;
	pPSC->bLeisurePsModeBackup = pPSC->bLeisurePs;
	pPSC->RegMaxLPSAwakeIntvl = pNdisCommon->RegLPSMaxIntvl;
	pPSC->RegAdvancedLPs = pNdisCommon->RegAdvancedLPs;

	pPSC->bGpioRfSw = pNdisCommon->bRegGpioRfSw;
	
	pPSC->RegLeisurePsMode = pNdisCommon->RegLeisurePsMode;
	pPSC->RegPowerSaveMode = pNdisCommon->RegPowerSaveMode;

	#if((USB_TX_THREAD_ENABLE && RTL8723_USB_IO_THREAD_ENABLE) || RTL8723_SDIO_IO_THREAD_ENABLE)
		pPSC->bLowPowerEnable = (BOOLEAN)pNdisCommon->bRegLowPowerEnable;
	#else
		pPSC->bLowPowerEnable = 0;
	#endif

	//Set WoWLAN mode. 2009.09.01. by tynli
	if(pNdisCommon->bRegWakeOnMagicPacket)
	{
		if(pNdisCommon->bRegWakeOnPattern)
			pPSC->WoWLANMode = eWakeOnBothTypePacket;
		else
			pPSC->WoWLANMode = eWakeOnMagicPacketOnly;
	}
	else if(pNdisCommon->bRegWakeOnPattern)
	{
		pPSC->WoWLANMode = eWakeOnPatternMatchOnly;
	}
	else
	{
		pPSC->WoWLANMode = eWoWLANDisable;	
	}

	pPSC->bWakeOnDisconnect = pNdisCommon->bRegWakeOnDisconnect;

	if(pNdisCommon->bRegPacketCoalescing)
		pMgntInfo->bSupportPacketCoalescing = TRUE;
	else
		pMgntInfo->bSupportPacketCoalescing = FALSE;
	
	pPSC->APOffloadEnable = pNdisCommon->RegAPOffloadEnable;
	pPSC->WoWLANLPSLevel = pNdisCommon->RegWoWLANLPSLevel ;
	pPSC->WoWLANS5Support = pNdisCommon->RegWoWLANS5Support;
	pPSC->D2ListenIntvl = pNdisCommon->RegD2ListenIntvl;
	pPSC->bFakeWoWLAN =  pNdisCommon->bRegFakeWoWLAN;
	
	pPSC->RegARPOffloadEnable = pNdisCommon->bRegARPOffloadEnable;
	pPSC->RegGTKOffloadEnable = pNdisCommon->bRegGTKOffloadEnable ;	// support GTK offload after NDIS620.
	pPSC->ProtocolOffloadDecision = pNdisCommon->RegProtocolOffloadDecision;
	pPSC->RegNSOffloadEnable = pNdisCommon->bRegNSOffloadEnable;

	pPSC->FSSDetection = pNdisCommon->RegFSSDetection;
	pMgntInfo->bIntelPatchPNP =  pNdisCommon->RegIntelPatchPNP;
	pPSC->DxNLOEnable = pNdisCommon->RegNLOEnable;
	pMgntInfo->bRegPnpKeepLink = (BOOLEAN)pNdisCommon->RegPnpKeepLink;

	pPSC->bFwCtrlPwrOff = (BOOLEAN)pNdisCommon->RegFwCtrlPwrOff;
	pPSC->CardDisableInLowClk = (BOOLEAN)pNdisCommon->RegCardDisableInLowClk;
	pPSC->FwIPSLevel = pNdisCommon->RegFwIPSLevel;

	// WoWLAN setting for DTM test.
	if(pAdapter->bInHctTest)
	{
		// Do not enable WoWLAN LPS in DTM test.
		pPSC->WoWLANLPSLevel = 0;
	}

	pAdapter->bRxPsWorkAround = (BOOLEAN)pNdisCommon->RegRxPsWorkAround;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("bRegFwCtrlLPS %d bRegLeisurePs %d", pNdisCommon->bRegFwCtrlLPS, pNdisCommon->RegLeisurePsMode));
	RT_TRACE(COMP_POWER, DBG_LOUD, ("bFwCtrlLPS %d bLeisurePs %d", pPSC->bFwCtrlLPS, pPSC->bLeisurePs));
}


//
//	Description:
//		Read common registry value under LOCAL_MACHINE\SYSTEM\CurrentControlSet\Service\RtlWlanx
//	2014.12.15, by Sean.
//
//    PASSIVE_LEVEL ONLY!!!!!!!!!!!!!!!!!!!!!!!!!
NTSTATUS
PlatformReadCommonDwordRegistry(
	IN	PWCHAR			registryName,
	OUT	pu4Byte			resultValue)
{

	OBJECT_ATTRIBUTES	objectAttributes = {0};
	HANDLE				driverKey = NULL;
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	UNICODE_STRING		ValueName;
	UNICODE_STRING		RegistryPath;
	KEY_VALUE_PARTIAL_INFORMATION	result;
	ULONG				resultLength;
	

	do
	{
		RtlInitUnicodeString(&RegistryPath,GlobalRtDriverContext.NdisContext.RegistryPath);
		//DbgPrint("PATH: %wZ length:%d MAX :%d \n",&RegistryPath,RegistryPath.Length,RegistryPath.MaximumLength);

		InitializeObjectAttributes(&objectAttributes,
		                           &RegistryPath,
		                           OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		                           NULL,
		                           NULL);
	    
		if(STATUS_SUCCESS != (ntStatus = ZwOpenKey(&driverKey, KEY_READ, &objectAttributes)))
		{
			DbgPrint("ZwOpenKey Failed = 0x%x\n", ntStatus);
			break;
		}
		RtlInitUnicodeString(&ValueName, registryName);

		ntStatus = ZwQueryValueKey(driverKey,&ValueName,KeyValuePartialInformation,&result,sizeof(KEY_VALUE_PARTIAL_INFORMATION),&resultLength);

		if(STATUS_SUCCESS != ntStatus)
		{
			DbgPrint("ZwQueryValueKey Failed = 0x%x\n", ntStatus);
			break;
		}
		memcpy(resultValue,result.Data,sizeof(u4Byte));
		//DbgPrint("Test result:%d Length:%d type:%d\n",*resultValue, result.DataLength, result.Type);    
	}while(FALSE);

	if(driverKey)
	{
		ZwClose(driverKey);
		driverKey = NULL;
	}

	return ntStatus;

}


//
//	Description:
//		Write common registry value under LOCAL_MACHINE\SYSTEM\CurrentControlSet\Service\RtlWlanx
//	2014.12.16, by Sean.
//
//    PASSIVE_LEVEL ONLY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
NTSTATUS
PlatformWriteCommonDwordRegistry(
	IN	PWCHAR			registryName,
	IN	pu4Byte			Value)
{

	OBJECT_ATTRIBUTES	objectAttributes = {0};
	HANDLE				driverKey = NULL;
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	UNICODE_STRING		ValueName;
	UNICODE_STRING		RegistryPath;
	

	do
	{
		RtlInitUnicodeString(&RegistryPath,GlobalRtDriverContext.NdisContext.RegistryPath);
		InitializeObjectAttributes(&objectAttributes,
		                           &RegistryPath,
		                           OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		                           NULL,
		                           NULL);
	    
		if(STATUS_SUCCESS != (ntStatus = ZwOpenKey(&driverKey, KEY_WRITE, &objectAttributes)))
		{
			DbgPrint("ZwOpenKey Failed = 0x%x\n", ntStatus);
			break;
		}
		RtlInitUnicodeString(&ValueName, registryName);

		ntStatus = ZwSetValueKey(driverKey,&ValueName,0,REG_DWORD,Value,sizeof(u4Byte));

		if(STATUS_SUCCESS != ntStatus)
		{
			DbgPrint("ZwSetValueKey Failed = 0x%x\n", ntStatus);
			break;
		}
	
	}while(FALSE);

	if(driverKey)
	{
		ZwClose(driverKey);
		driverKey = NULL;
	}

	return ntStatus;

}

NTSTATUS
PlatformWriteBTAntPosDwordRegistry(
	IN	PWCHAR			registryName,
	IN	pu4Byte			Value)
{

	OBJECT_ATTRIBUTES	objectAttributes = {0};
	HANDLE				driverKey = NULL;
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	UNICODE_STRING		ValueName;
	UNICODE_STRING		RegistryPath;
	ANSI_STRING 		AnsiString;  
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("### test\n"));

	do
	{
		//RtlInitUnicodeString(&RegistryPath,registryPath);
		RtlInitString(&AnsiString,"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Services\\RtlWlans\\Parameters");
		RtlAnsiStringToUnicodeString(&RegistryPath,&AnsiString,TRUE);
		RT_TRACE(COMP_INIT, DBG_LOUD, ("### PlatformWriteBTAntPosDwordRegistry(): RegistryPath = %wZ\n", &RegistryPath));
		
		InitializeObjectAttributes(&objectAttributes,
		                           &RegistryPath,
		                           OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		                           NULL,
		                           NULL);
	    
		if(STATUS_SUCCESS != (ntStatus = ZwOpenKey(&driverKey, KEY_WRITE, &objectAttributes)))
		{
			DbgPrint("ZwOpenKey Failed = 0x%x\n", ntStatus);
			break;
		}
		RtlInitUnicodeString(&ValueName, registryName);

		ntStatus = ZwSetValueKey(driverKey,&ValueName,0,REG_DWORD,Value,sizeof(u4Byte));

		if(STATUS_SUCCESS != ntStatus)
		{
			DbgPrint("ZwSetValueKey Failed = 0x%x\n", ntStatus);
			break;
		}
	
	}while(FALSE);

	if(driverKey)
	{
		ZwClose(driverKey);
		driverKey = NULL;
	}

	RtlFreeUnicodeString(&RegistryPath); 
	return ntStatus;

}

#if READ_BT_REGISTRY
NTSTATUS
PlatformReadBTFWLoaderDwordRegistry(
	IN	PWCHAR			registryName,
	OUT	pu1Byte			resultValue)
{

	OBJECT_ATTRIBUTES	objectAttributes = {0};
	HANDLE				driverKey = NULL;
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	UNICODE_STRING		ValueName;
	UNICODE_STRING		RegistryPath;
	ANSI_STRING 		AnsiString;
	KEY_VALUE_PARTIAL_INFORMATION	result;
	ULONG				resultLength;
	

	do
	{
		RtlInitString(&AnsiString,"\\REGISTRY\\MACHINE\\Software\\Realtek\\Bluetooth\\service");
		RtlAnsiStringToUnicodeString(&RegistryPath,&AnsiString,TRUE);
		RT_TRACE(COMP_INIT, DBG_LOUD, ("### PlatformReadBTFWLoaderDwordRegistry(): RegistryPath = %wZ\n", &RegistryPath));

		InitializeObjectAttributes(&objectAttributes,
		                           &RegistryPath,
		                           OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		                           NULL,
		                           NULL);
	    
		if(STATUS_SUCCESS != (ntStatus = ZwOpenKey(&driverKey, KEY_READ, &objectAttributes)))
		{
			DbgPrint("ZwOpenKey Failed = 0x%x\n", ntStatus);
			break;
		}
		RtlInitUnicodeString(&ValueName, registryName);

		ntStatus = ZwQueryValueKey(driverKey,&ValueName,KeyValuePartialInformation,&result,sizeof(KEY_VALUE_PARTIAL_INFORMATION),&resultLength);

		if(STATUS_SUCCESS != ntStatus)
		{
			DbgPrint("ZwQueryValueKey Failed = 0x%x\n", ntStatus);
			break;
		}
		memcpy(resultValue,result.Data,sizeof(u4Byte));
		//DbgPrint("Test result:%d Length:%d type:%d\n",*resultValue, result.DataLength, result.Type);    
	}while(FALSE);

	if(driverKey)
	{
		ZwClose(driverKey);
		driverKey = NULL;
	}

	return ntStatus;

}
#endif


