#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "N62C_Port.tmh"
#endif

BOOLEAN
N62CCompletePendingCreateDeleteMacOidRequest(
	IN PADAPTER			pAdapter,
	IN  NDIS_STATUS		NdisStatus
	)
{
	PRT_NDIS6_COMMON	pNdisCommon=pAdapter->pNdisCommon;
	PPORT_HELPER			pPortHelper = pAdapter->pPortCommonInfo->pPortHelper;
	BOOLEAN				bCompleteStatus = FALSE;

	//
	// Complete this OID to the OS
	//
	PlatformAcquireSpinLock(pAdapter, RT_PORT_SPINLOCK);

#if DRV_LOG_REGISTRY
	if(pPortHelper->pCreateDeleteOID == NULL)
	{
		PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
		RT_SET_DRV_STATE(pAdapter, DrvStateMacPendReq_Error0);
		return FALSE;
	}

	PlatformAcquireSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
	if((pNdisCommon->PendedRequest != pPortHelper->pCreateDeleteOID))
	{
		PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
		RT_SET_DRV_STATE(pAdapter, DrvStateMacPendReq_Error1);
		PlatformAcquireSpinLock(pAdapter, RT_PORT_SPINLOCK);
	}
	else
		PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
#endif
	
	// 20130917 Joseph:
	// Pending OID is restore in 2 different place.
	// This shall be revised and add a new OID pending queue for OID completion.
	// Prefast warning ignore for false positive
#pragma warning( disable:6273 )
	RT_ASSERT(pNdisCommon->PendedRequest != NULL, ("N62CCompletePendingCreateDeleteMacOidRequest(): CreateDeleteMac OID shall also be queued in NdisCommon!! pAdapter=%x\n", pAdapter));
	RT_ASSERT(pNdisCommon->PendedRequest == pPortHelper->pCreateDeleteOID, ("N62CCompletePendingCreateDeleteMacOidRequest(): CreateDeleteMac OID shall also be the same in NdisCommon and PortHelper!!\n"));

	// WDI completion
	if(OS_SUPPORT_WDI(pAdapter))
	{
	bCompleteStatus = WDI_CompleteCreateDeleteMac(pAdapter, NdisStatus);
	}
	
	pPortHelper->pCreateDeleteOID = NULL;
	PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);

	if(!bCompleteStatus)
		bCompleteStatus = N6CompletePendedOID(pAdapter, RT_PENDED_OID_CREATE_DELETE_MAC, NdisStatus);


	return bCompleteStatus;
}

NDIS_STATUS
N62C_OID_DOT11_CREATE_MAC(
	IN  PADAPTER				pAdapter,
	IN  PNDIS_OID_REQUEST   	NdisRequest
	)
{

	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PRT_NDIS62_COMMON	pNdis62Common = pAdapter->pNdis62Common;

	PPORT_HELPER		pPortHelper = pAdapter->pPortCommonInfo->pPortHelper;
	PNDIS_OID_REQUEST	OidRequest = (PNDIS_OID_REQUEST)NdisRequest;
	KIRQL 				irql = NDIS_CURRENT_IRQL();
	u4Byte				createPollingCnt=0x0;	

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_CREATE_MAC(): Oid(%#x)--->\n", NdisRequest->DATA.METHOD_INFORMATION.Oid));
	N6C_DOT11_DUMP_OID(NdisRequest->DATA.METHOD_INFORMATION.Oid);

#if DRV_LOG_REGISTRY
	// Reset all MAC deletion registry settings
	RT_CLEAR_DRV_STATE(pAdapter, (DrvStateMacDeleting|DrvStateMacDeleted));
	
	// Reset all MAC creation registry settings
	RT_CLEAR_DRV_STATE(pAdapter, (DrvStateMacCreating|DrvStateMacCreated));

	// Set specific driver state
	RT_SET_DRV_STATE(pAdapter, DrvStateMacCreating);
#endif
	if(irql == DISPATCH_LEVEL)
	{
		RT_SET_DRV_STATE(pAdapter, DrvStateMacCreatDispatch);
	}
	while(1)
	{
		PlatformAcquireSpinLock(pAdapter, RT_PORT_SPINLOCK);
		if(pPortHelper->bCreateMac || pPortHelper->bDeleteMac)
		{
			PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
			createPollingCnt++;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_CREATE_MAC(): pollCnt/bCreateMac/bDeleteMac= %d/ %d/ %d\n", 
				createPollingCnt, pPortHelper->bCreateMac, pPortHelper->bDeleteMac));
			if(createPollingCnt >= 1000)
				break;
			else
				delay_ms(1);
		}
		else
		{
			PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
			break;
		}
	}

	if(createPollingCnt)
	{
		RT_SET_DRV_STATE(pAdapter, DrvStateMacCreatWait);
	}

	if( irql < DISPATCH_LEVEL ){
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_CREATE_MAC(): Raise IRQL!!\n"));
		KeRaiseIrql(DISPATCH_LEVEL, &irql);
	}

	do{

		NdisRequest->DATA.METHOD_INFORMATION.BytesWritten = 0;
		NdisRequest->DATA.METHOD_INFORMATION.BytesRead = 0;
		NdisRequest->DATA.METHOD_INFORMATION.BytesNeeded = 0;
		
		if(!pAdapter->bInHctTest &&	!pDefaultAdapter->pNdisCommon->bRegVWifiSupport)
		{
			ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
			break;
		}

		if (NdisRequest->RequestType != NdisRequestMethod)
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("Invalid request type %d for OID_DOT11_CREATE_MAC\n", NdisRequest->RequestType));
			ndisStatus =NDIS_STATUS_NOT_SUPPORTED;
			RT_SET_DRV_STATE(pAdapter, DrvStateMacCreate_NO_DO);
			break;
		}

		if (NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength < sizeof(DOT11_MAC_INFO))
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("The buffer being passed into OID_DOT11_CREATE_MAC is too small(%d)\n", 
				NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength));
			NdisRequest->DATA.METHOD_INFORMATION.BytesNeeded = sizeof(DOT11_MAC_INFO);
			RT_SET_DRV_STATE(pAdapter, DrvStateMacCreate_NO_DO);
			ndisStatus =  NDIS_STATUS_INVALID_LENGTH;
			break;
		}

		//
		// Since OID calls are serialized, we do not expect the NumberOfPorts to change
		// while we are checking the following until this OID is completed. So we do not need 
		// to protect the NumberOfPorts in any way
		//
		
		// The default port should be considered --------------------------------------------
		if (MultiPortGetNumberOfActiveExtAdapters(pAdapter) + 1 >= MP_MAX_NUMBER_OF_PORT)
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("Number of existing ports exceed max supported. Failing new port creation\n"));      
			ndisStatus =  NDIS_STATUS_OPEN_LIST_FULL;
			RT_SET_DRV_STATE(pAdapter, DrvStateMacCreate_NO_DO);
			break;
		}

		PlatformAcquireSpinLock(pAdapter, RT_PORT_SPINLOCK);
		if(pPortHelper->pCreateDeleteOID != NULL)
		{// Another CreateDeleteOID has already pended, maybe from another port number		
			PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
			
//			RT_ASSERT(FALSE, ("N62C_OID_DOT11_CREATE_MAC(): Another CreateDeleteOID has already pended!!\n"));
			RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("N62C_OID_DOT11_CREATE_MAC(): Another CreateDeleteOID has already pended!!\n"));
			ndisStatus =  NDIS_STATUS_FAILURE; // Need to check this return status
			RT_SET_DRV_STATE(pAdapter, DrvStateMacCreate_NO_DO);
			break;
		}
		else
		{
			// Done creating the new virtual adapter
			pPortHelper->pCreateDot11MacInfo = (PDOT11_MAC_INFO)OidRequest->DATA.METHOD_INFORMATION.InformationBuffer;
			NdisMoveMemory(pPortHelper->pCreateDot11MacInfo->MacAddr, pAdapter->CurrentAddress, sizeof(DOT11_MAC_ADDRESS));
			OidRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(DOT11_MAC_INFO);
		
			// Save corresponding NdisRequest for MAC Creation and Deletion
			pPortHelper->bCreateMac=TRUE;
			pPortHelper->pCreateDeleteOID = NdisRequest;

//			if(!PlatformScheduleWorkItem( &pPortHelper->CreateDeleteMacWorkitem))
			{
				//RT_ASSERT(FALSE, ("N62C_OID_DOT11_CREATE_MAC(): Cannot schedule workitem!!Timer scheduled\n"));
				if(!PlatformSetTimer(pAdapter, &pPortHelper->CreateDeleteMacTimer, 15))
				{
					RT_ASSERT(FALSE, ("N62C_OID_DOT11_CREATE_MAC(): Cannot schedule Timer!!Failed\n"));
					ndisStatus = NDIS_STATUS_FAILURE;
					pPortHelper->bCreateMac=FALSE;
					pPortHelper->pCreateDeleteOID = NULL;
					RT_SET_DRV_STATE(pAdapter, DrvStateMacCreate_TMR_FAIL);
				}
				else
				{
					ndisStatus = NDIS_STATUS_PENDING;
				}
			}
//			else
//			{
//				ndisStatus = NDIS_STATUS_PENDING;
//			}
			
			PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);	
		}

			
		RT_ASSERT(pPortHelper->pCreateDeleteOID, ("N62C_OID_DOT11_CREATE_MAC(): pCreateDeleteOID has already completed!!\n"));
		

	}
	while(FALSE);
	
	if( irql < DISPATCH_LEVEL ){
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_CREATE_MAC(): Restore IRQL!!\n"));
		KeLowerIrql(irql);	
	}

#if DRV_LOG_REGISTRY
	if( !pAdapter->bDriverStopped && RT_GET_DRV_STATE_ERROR(pAdapter)){
		RT_TRACE(COMP_INIT, DBG_WARNING, ("N62C_OID_DOT11_CREATE_MAC(): Update Driver state!!\n"));
		RT_SET_DRV_STATE(pAdapter, DrvStateNull);
		RT_CLEAR_DRV_STATE_ERROR(pAdapter);
	}
#endif
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_CREATE_MAC(): <---\n"));
	
	return ndisStatus;
}

NDIS_STATUS
N62C_OID_DOT11_DELETE_MAC(
	IN  PADAPTER				pAdapter,
	IN  PNDIS_OID_REQUEST   	NdisRequest
	)
{

	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	PDOT11_MAC_INFO		MacInfo = NULL;
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PRT_NDIS62_COMMON	pNdis62Common = pAdapter->pNdis62Common;
	PPORT_HELPER		pPortHelper = pAdapter->pPortCommonInfo->pPortHelper;
	KIRQL 				irql = NDIS_CURRENT_IRQL();
	u4Byte				deletePollingCnt=0x0;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_DELETE_MAC(): Oid(%#x)--->\n", NdisRequest->DATA.METHOD_INFORMATION.Oid));
	N6C_DOT11_DUMP_OID(NdisRequest->DATA.METHOD_INFORMATION.Oid);

#if DRV_LOG_REGISTRY
	// Reset all MAC deletion registry settings
	RT_CLEAR_DRV_STATE(pAdapter, (DrvStateMacDeleting|DrvStateMacDeleted));

	// Reset all MAC creation registry settings
	RT_CLEAR_DRV_STATE(pAdapter, (DrvStateMacCreating|DrvStateMacCreated));

	// Set specific driver state
	RT_SET_DRV_STATE(pAdapter, DrvStateMacDeleting);
#endif

	while(1)
	{
		PlatformAcquireSpinLock(pAdapter, RT_PORT_SPINLOCK);
		if(pPortHelper->bCreateMac || pPortHelper->bDeleteMac)
		{
			PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
			deletePollingCnt++;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_DELETE_MAC(): pollCnt/bCreateMac/bDeleteMac= %d/ %d/ %d\n", 
				deletePollingCnt, pPortHelper->bCreateMac, pPortHelper->bDeleteMac));
			if(deletePollingCnt >= 1000)
				break;
			else
				delay_ms(1);
		}
		else
		{
			PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
			break;
		}
	}

	if( irql < DISPATCH_LEVEL ){
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_DELETE_MAC(): Raise IRQL!!\n"));
		KeRaiseIrql(DISPATCH_LEVEL, &irql);	
	}

	do
	{
		NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
		NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 0;

		if(!pAdapter->bInHctTest &&!pDefaultAdapter->pNdisCommon->bRegVWifiSupport)
		{
			ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
			break;
		}

		if (NdisRequest->RequestType != NdisRequestSetInformation)
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("Invalid request type %d for OID_DOT11_DELETE_MAC\n", 
				NdisRequest->RequestType));
			RT_SET_DRV_STATE(pAdapter, DrvStateMacDelete_NO_DO);
			ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
			break;
		}

		if (NdisRequest->DATA.SET_INFORMATION.InformationBufferLength < sizeof(DOT11_MAC_INFO))
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("The buffer being passed into OID_DOT11_DELETE_MAC is too small(%d)", 
				NdisRequest->DATA.SET_INFORMATION.InformationBufferLength));
			NdisRequest->DATA.SET_INFORMATION.BytesNeeded = sizeof(DOT11_MAC_INFO);
			ndisStatus = NDIS_STATUS_INVALID_LENGTH;
			RT_SET_DRV_STATE(pAdapter, DrvStateMacDelete_NO_DO);
			break;
		}

		MacInfo = (PDOT11_MAC_INFO)NdisRequest->DATA.SET_INFORMATION.InformationBuffer;

		if (!OS_SUPPORT_WDI(pAdapter) && // WDI port number is not sequential, it's bitmap instead
			MacInfo->uNdisPortNumber > MP_MAX_NUMBER_OF_PORT ||
			MacInfo->uNdisPortNumber == DEFAULT_NDIS_PORT_NUMBER ||
			MacInfo->uNdisPortNumber == HELPER_PORT_PORT_NUMBER)
		{
			RT_TRACE(COMP_OID_SET, DBG_WARNING, ("The port number (%d) being passed in is invalid", MacInfo->uNdisPortNumber));
			ndisStatus = NDIS_STATUS_INVALID_DATA;
			RT_SET_DRV_STATE(pAdapter, DrvStateMacDelete_NO_DO);
			break;
		}

		PlatformAcquireSpinLock(pAdapter, RT_PORT_SPINLOCK);
		if(pPortHelper->pCreateDeleteOID != NULL)
		{// Another CreateDeleteOID has already pended, maybe from another port number

			PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
			
//			RT_ASSERT(FALSE, ("N62C_OID_DOT11_DELETE_MAC(): Another CreateDeleteOID has already pended!!\n"));
			RT_TRACE(COMP_OID_SET, DBG_SERIOUS, ("N62C_OID_DOT11_DELETE_MAC(): Another CreateDeleteOID has already pended!!\n"));
			ndisStatus = NDIS_STATUS_FAILURE; // Need to check this return status
			RT_SET_DRV_STATE(pAdapter, DrvStateMacDelete_NO_DO);
			break;
		}
		else
		{
			pPortHelper->bDeleteMac=TRUE;
			pPortHelper->pCreateDot11MacInfo= (PDOT11_MAC_INFO)NdisRequest->DATA.SET_INFORMATION.InformationBuffer;	
			NdisRequest->DATA.SET_INFORMATION.BytesRead = sizeof(DOT11_MAC_INFO);	

			// Save corresponding NdisRequest for MAC Creation and Deletion
			pPortHelper->pCreateDeleteOID = NdisRequest;

//			if(!PlatformScheduleWorkItem( &pPortHelper->CreateDeleteMacWorkitem))
			{
				//RT_ASSERT(FALSE, ("N62C_OID_DOT11_DELETE_MAC(): Cannot schedule workitem!!Timer scheduled\n"));
				if(!PlatformSetTimer(pAdapter, &pPortHelper->CreateDeleteMacTimer, 15))
				{
					RT_ASSERT(FALSE, ("N62C_OID_DOT11_DELETE_MAC(): Cannot schedule Timer!!Failed\n"));
					ndisStatus = NDIS_STATUS_FAILURE;
					pPortHelper->bDeleteMac=FALSE;
					pPortHelper->pCreateDeleteOID = NULL;
					RT_SET_DRV_STATE(pAdapter, DrvStateMacDelete_TMR_FAIL);
				}
				else
				{
					ndisStatus = NDIS_STATUS_PENDING;
				}
			}
//			else
//			{
//				ndisStatus = NDIS_STATUS_PENDING;
//			}

			PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);	
		}

		RT_ASSERT(pPortHelper->pCreateDeleteOID, ("N62C_OID_DOT11_DELETE_MAC(): pCreateDeleteOID is not completed!!\n"));

	}while (FALSE);
	

	if( irql < DISPATCH_LEVEL ){
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_DELETE_MAC(): Restore IRQL!!\n"));
		KeLowerIrql(irql);	
	}

#if DRV_LOG_REGISTRY
	if( !pAdapter->bDriverStopped && RT_GET_DRV_STATE_ERROR(pAdapter)){
		RT_TRACE(COMP_INIT, DBG_WARNING, ("N62C_OID_DOT11_DELETE_MAC(): Update Driver state!!\n"));
		RT_SET_DRV_STATE(pAdapter, DrvStateNull);
		RT_CLEAR_DRV_STATE_ERROR(pAdapter);
	}
#endif
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62C_OID_DOT11_DELETE_MAC():< ---\n"));
	
	return ndisStatus;
}

VOID
N62CCreateDeleteMacTimerCallback(
	IN PRT_TIMER			pTimer
	)
{
	PADAPTER pAdapter = (PADAPTER)pTimer->Adapter;
	PPORT_HELPER		pPortHelper = pAdapter->pPortCommonInfo->pPortHelper;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("====> N62CCreateDeleteMacTimerCallback()\n"));

	PlatformAcquireSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
	if(pAdapter->pNdisCommon->PendedRequest == NULL){
		PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		RT_SET_DRV_STATE(pAdapter, DrvStateMacPendReq_Error2);
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("N62CCreateDeleteMacTimerCallback(), PendedRequest is NULL, Re-schedule CreateDeleteMacTimer!!!\n"));
		PlatformSetTimer(pAdapter, &pPortHelper->CreateDeleteMacTimer, 1);
		return;
	}
	else
		PlatformReleaseSpinLock(pAdapter, RT_PENDED_OID_SPINLOCK);
		
	// debug if workitem scheduled successfully or not
	//pPortHelper->CreateDeleteMacWorkitem.bWriteRegistry = TRUE;
	if( !PlatformScheduleWorkItem( &(pPortHelper->CreateDeleteMacWorkitem)) )
	{
		RT_SET_DRV_STATE(pAdapter, DrvStateMacCreDel_WI_FAIL);

		RT_TRACE(COMP_OID_SET, DBG_WARNING, ("N62CCreateDeleteMacTimerCallback():(): Fail to schedule WI!!\n"));   
		N62CCompletePendingCreateDeleteMacOidRequest(pAdapter, NDIS_STATUS_REQUEST_ABORTED);
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<==== N62CCreateDeleteMacTimerCallback()\n"));
}

void
N62CCreateDeleteMacWorkItemCallback(
	IN PVOID			pContext
	)
{
	PADAPTER pAdapter = (PADAPTER)pContext;
	PRT_NDIS62_COMMON	pNdis62Common = pAdapter->pNdis62Common;
	PPORT_HELPER				pPortHelper = pAdapter->pPortCommonInfo->pPortHelper;	
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("====> N62CCreateDeleteMacWorkItemCallback()\n"));

	PlatformAcquireSpinLock(pAdapter, RT_PORT_SPINLOCK);
	if(pPortHelper->bCreateMac)
	{
		PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
		ndisStatus=N62CCreateMac(pAdapter);

		RT_SET_DRV_STATE(pAdapter, DrvStateMacCreated);
		
		pPortHelper->bCreateMac=FALSE;
		if(ndisStatus !=NDIS_STATUS_SUCCESS)
		{
			RT_SET_DRV_STATE(pAdapter, DrvStateMacCreate_Fail);
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("bCreateMac   Fail\n"));		
		}
		else
		{
			// Re-hook association entry to keep fresh when delete port.
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
			PDM_ODM_T		 pDM_OutSrc = &pHalData->DM_OutSrc;
			if( pDM_OutSrc->odm_ready == TRUE )
				ODM_AsocEntry_Init(pDM_OutSrc);
		}
	}
	else if(pPortHelper->bDeleteMac)
	{
		PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
		ndisStatus=N62CDeleteMac(pAdapter);

		RT_SET_DRV_STATE(pAdapter, DrvStateMacDeleted);
		
		pPortHelper->bDeleteMac=FALSE;
		if(ndisStatus !=NDIS_STATUS_SUCCESS)
		{
			RT_SET_DRV_STATE(pAdapter, DrvStateMacDelete_Fail);
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("bdeleteMac   Fail\n"));	
		}		
		else
		{
			// Re-hook association entry to keep fresh when delete port.
			HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
			PDM_ODM_T		 pDM_OutSrc = &pHalData->DM_OutSrc;
			if( pDM_OutSrc->odm_ready == TRUE )
				ODM_AsocEntry_Init(pDM_OutSrc);
		}
	}
	else
	{
		PlatformReleaseSpinLock(pAdapter, RT_PORT_SPINLOCK);
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("UnKnow Operation\n"));	
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<==== N62CCreateDeleteMacWorkItemCallback()\n"));
}

u4Byte	logValue[10]={0};
u4Byte	logCnt=0x0;
NDIS_STATUS
N62CCreateMac(
	IN  PADAPTER		Adapter
	)
{
	PADAPTER				pDefaultAdapter = GetDefaultAdapter(Adapter);
	PRT_NDIS62_COMMON		Ndis62Common = pDefaultAdapter->pNdis62Common;
	PPORT_HELPER			pPortHelper = pDefaultAdapter->pPortCommonInfo->pPortHelper;		
	NDIS_STATUS				ndisStatus = NDIS_STATUS_SUCCESS;
	BOOLEAN					ndisPortAllocated = FALSE, ndisPortActivated = FALSE;
	NDIS_PORT_NUMBER		portNumber = DEFAULT_NDIS_PORT_NUMBER;
	PADAPTER				pAdapter = pDefaultAdapter;
	PADAPTER				pTargetAdapter = NULL;
	PADAPTER				pAllocFailAdapter = NULL;
	BOOLEAN					bCompleteStatus = FALSE;
	
    RT_TRACE(COMP_OID_SET, DBG_LOUD, ("====> N62CCreateMac()\n"));

	logCnt++;
	if(logCnt >= 10)
		logCnt = 10;
	logValue[logCnt-1] |= BIT1;
	
	/*
		Pause the miniport before we modify the adapter's port list. 
		This is an expensive way to achieve this but it is simple to implement
	*/
	do
	{
		ndisStatus = N62CHandleMiniportPause(pAdapter, NULL);			
	
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, (" N62CCreateMac(), N62CHandleMiniportPause() FAIL!!!\n"));
			break;
		}	  
	
		pAdapter = GetNextExtAdapter(pAdapter);
	} while(pAdapter!=NULL);

	do
	{
		//
		// Allocate an NDIS port and the adapter instance
		//

		// MultiPort Activate a New Port -------------------------------------
		pAdapter = MultiPortGetIdleExtAdapter(Adapter);
		RT_ASSERT(pAdapter != NULL, ("Need to allocate more adapters! \n"));

		// Error handle to prevent from BSOD 0x7E of NULL pointer issue. 2014.11.03, by tynli.
		if(!pAdapter)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Status fail: Need to allocate more adapters!\n"));
			ndisStatus = NDIS_STATUS_OPEN_LIST_FULL;
			break;
		}

		MultiPortChangeExtAdapterActiveState(pAdapter, TRUE);
		// --------------------------------------------------------------
//LE do not need to allocate NDIS port in WDI driver
#if 0
		if(!OS_SUPPORT_WDI)
		{
		ndisStatus = N62CHelperAllocateNdisPort(pAdapter, &portNumber);
		}
		else
#endif
		{
			PWDI_DATA_STRUCT	pWdi = &(pAdapter->pPortCommonInfo->WdiData);
			PRT_OID_HANDLER 	pOidHandle = &pWdi->TaskHandle;

			portNumber = pOidHandle->tlvParser.parsedTlv.paramCreatePort->CreatePortParameters.NdisPortNumber;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("UE pass down NdisPortNumber:%d\n", portNumber));
		}

		if( ndisStatus != NDIS_STATUS_SUCCESS )
		{
			pAllocFailAdapter = pAdapter;
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Allocate Ndis Port fail\n"));
			break;
		}
		
		pTargetAdapter = pAdapter;
		pPortHelper->pCreateDot11MacInfo->uNdisPortNumber = portNumber;					
		ndisPortAllocated = TRUE;

		pAdapter->pNdisCommon->dot11CurrentOperationMode.uCurrentOpMode = DOT11_OPERATION_MODE_EXTENSIBLE_STATION;
		pAdapter->pNdis62Common->PortType = EXTSTA_PORT;
		pAdapter->pNdis62Common->CurrentOpState=INIT_STATE;
		pAdapter->pNdis62Common->PortNumber = portNumber;
		pAdapter->bHWInitReady = TRUE;
		pAdapter->bSWInitReady = TRUE;
		pAdapter->bInitializeInProgress = FALSE;
	 	pAdapter->MgntInfo.dot11CurrentWirelessMode = pDefaultAdapter->MgntInfo.dot11CurrentWirelessMode;
		pAdapter->TotalCamEntry = pDefaultAdapter->TotalCamEntry;
		pAdapter->HalfCamEntry = pDefaultAdapter->HalfCamEntry;
		pAdapter->CamExtApMaxEntery = pDefaultAdapter->CamExtApMaxEntery;
		pAdapter->CamExtApP2pStartIndex = pDefaultAdapter->CamExtApP2pStartIndex;
		pAdapter->CamExtApP2pMaxEntry = pDefaultAdapter->CamExtApP2pMaxEntry;
		pAdapter->CamExtStaPairwiseKeyPosition = pDefaultAdapter->CamExtStaPairwiseKeyPosition;
		pAdapter->CamBtStartIndex = pDefaultAdapter->CamBtStartIndex;
		pAdapter->BtHwCamStart = pDefaultAdapter->BtHwCamStart;
		SetAPState(pAdapter, AP_STATE_STOPPED);
		
	 	if(IS_WIRELESS_MODE_N(pAdapter))
	 		pAdapter->MgntInfo.pHTInfo->bEnableHT = TRUE;
	 	else
	 		pAdapter->MgntInfo.pHTInfo->bEnableHT = FALSE;
		
	 	MgntRefreshSuppRateSet( pAdapter );
		
		N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_PAUSED);

		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Created a new Port \n" ));
		
	} while (FALSE);
    
	pAdapter = pDefaultAdapter;

	// Restart the miniport driver
	do{
		if(pAllocFailAdapter != pAdapter)
		{
			if (N62CHelperHandleMiniportRestart(pAdapter, NULL) != NDIS_STATUS_SUCCESS)
			{
				RT_TRACE(COMP_INIT, DBG_LOUD, (" N62CCreateMac(), N62CHelperHandleMiniportRestart() FAIL!!!\n"));
				break;
			} 
		}
		pAdapter = GetNextExtAdapter(pAdapter);
	}while(pAdapter!=NULL);

	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		if (N62CHelperHandleMiniportRestart(pDefaultAdapter, NULL) != NDIS_STATUS_SUCCESS)
		{
			 RT_TRACE(COMP_OID_SET, DBG_LOUD, ("HelperPortHandleMiniportRestart failed"));
		}
		//4: Need do any error handle ? Neo
	}

	bCompleteStatus = N62CCompletePendingCreateDeleteMacOidRequest(Adapter, ndisStatus);
	RT_TRACE(COMP_INIT, DBG_LOUD, (" N62CCreateMac(), N62CCompletePendingCreateDeleteMacOidRequest() bCompleteStatus=%d.\n", bCompleteStatus));
	
	//
	// We can now activate/deactivate the port. We cannot do this while we are processing
	// the OID, else the OS would deadlock. 
	//
	if (ndisStatus == NDIS_STATUS_SUCCESS && bCompleteStatus)
	{
		logValue[logCnt-1] |= BIT2;
		// If this port has been allocated with NDIS, activate it. This notification
		// goes upto the OS and it would handle the request
		RT_TRACE(COMP_INIT, DBG_LOUD, (" N62CCreateMac(), before N62CHelperActivateNdisPort().\n"));
#if 0
		if(!OS_SUPPORT_WDI)
		ndisStatus = N62CHelperActivateNdisPort(pTargetAdapter, portNumber);
#endif
		RT_TRACE(COMP_INIT, DBG_LOUD, (" N62CCreateMac(), after N62CHelperActivateNdisPort().\n"));
	}
	else
	{
		logValue[logCnt-1] |= BIT3;
		RT_TRACE(COMP_INIT, DBG_LOUD, (" N62CCreateMac(), ndisStatus=0x%x\n", ndisStatus));
		if (IS_ALLOCATED_PORT_NUMBER(portNumber))
		{
			// If activated, deactivate the NDIS port
			if (ndisPortActivated)
			{
#if 0
				if(!OS_SUPPORT_WDI)
				N62CHelperDeactivateNdisPort(pDefaultAdapter, portNumber);
#endif
			}

			// If allocated free the NDIS port
			if (ndisPortAllocated)
			{
#if 0
				if(!OS_SUPPORT_WDI)
				N62CHelperFreeNdisPort(pDefaultAdapter, portNumber);
#endif
			}
		}
	}
	logValue[logCnt-1] |= BIT4;
    	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<==== N62CCreateMac()\n"));
	return ndisStatus;
}

NDIS_STATUS
N62CDeleteMac(
	IN PADAPTER				pAdapter
	)
{
	NDIS_STATUS                 ndisStatus = NDIS_STATUS_SUCCESS;
	PNDIS_OID_REQUEST           OidRequest = (PNDIS_OID_REQUEST)NdisOidRequest;
	PDOT11_MAC_INFO             MacInfo = NULL;
	PMP_PORT                    destinationPort = NULL;
	NDIS_PORT_NUMBER            portNumber;
	BOOLEAN                     destroyPortNumber = FALSE;
	PRT_NDIS62_COMMON  	pNdis62Common = pAdapter->pNdis62Common;	
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PPORT_HELPER	pPortHelper = pAdapter->pPortCommonInfo->pPortHelper;
	PADAPTER	pTargetAdapter = NULL;
	PADAPTER pLoopAdapter = NULL;
	BOOLEAN	bCompleteStatus = FALSE;
	
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("====> N62CDeleteMac()\n"));
	do
	{
		portNumber = pPortHelper->pCreateDot11MacInfo->uNdisPortNumber;

		//
		// First we would need to translate from the NDIS_PORT_NUMBER
		// to our port structure. This is done by walking the PortList
		// Since OID calls are serialized, we do not expect the Portlist to change
		// while we are trying to find the port or for the port to get deleted
		// until this OID is completed. So we do not need to protect the Port/PortList
		// in any way
		//

		if (IS_ALLOCATED_PORT_NUMBER(portNumber))
		{
			// This port has been allocate with NDIS. When we are done, delete the
			// port
			destroyPortNumber = TRUE;
		}
		pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)portNumber);

		if(!IsDefaultAdapter(pTargetAdapter))
		{
			/*
			Pause the miniport before we modify the adapter's port list. 
			This is an expensive way to achieve this but it is simple to implement
			*/
			pLoopAdapter = GetDefaultAdapter(pAdapter);
			
			do
			{
				N6C_SET_MP_DRIVER_STATE_BACKUP(pLoopAdapter, N6C_GET_MP_DRIVER_STATE(pLoopAdapter));
			
				if(N6C_GET_MP_DRIVER_STATE(pLoopAdapter) == MINIPORT_RUNNING)
				{				
				ndisStatus = N62CHandleMiniportPause(pLoopAdapter, NULL);			

				if (ndisStatus != NDIS_STATUS_SUCCESS)
				{
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("HelperPortHandleMiniportPause failed 0x%x\n", ndisStatus));
					break;
				}        
				}

				pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
			} while(pLoopAdapter!=NULL);
			

			// MultiPort Disable the Port ----------------------------------------
			MultiPortChangeExtAdapterActiveState(pTargetAdapter, FALSE);
			
			//sherry added for revise to default for fix wlk1.6 error
			pTargetAdapter->pNdis62Common->CurrentOpState = INIT_STATE;
			SetAPState(pTargetAdapter, AP_STATE_STOPPED);
			// --------------------------------------------------------------

			#if (P2P_SUPPORT == 1)
			{
				// Stop the Device or Role Port mode: Last 3 parameters are don't care
				if(pTargetAdapter->pNdis62Common->PortType > EXTAP_PORT && pTargetAdapter->pNdis62Common->PortType <= EXT_P2P_ROLE_PORT)
				{
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("PortNum = %d, PortType = %d, disable P2P Mode!\n", pTargetAdapter->pNdis62Common->PortNumber, pTargetAdapter->pNdis62Common->PortType));
					MgntActSet_P2PMode(pTargetAdapter, FALSE, FALSE, 0, 0, 0);
				}
			}
			#endif


			//
			// <Roger_EXP> We should release all queued data frame for each port.
			// 2013.07.08.
			//
			PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
			PlatformReleaseDataFrameQueued(pTargetAdapter);
			PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
			
			pTargetAdapter->bSWInitReady = FALSE;

			/*
			Restart so that normal operation may continue
			*/
			// TODO: should do deactive port.
			pLoopAdapter = GetDefaultAdapter(pAdapter);
			
			do{
				if(N6C_GET_MP_DRIVER_STATE_BACKUP(pLoopAdapter) == MINIPORT_RUNNING)
				{
				ndisStatus = N62CHelperHandleMiniportRestart(pLoopAdapter, NULL);
				if (ndisStatus != NDIS_STATUS_SUCCESS)
				{
					RT_TRACE(COMP_OID_SET, DBG_LOUD, ("HelperPortHandleMiniportRestart failed 0x%x", ndisStatus));
					break;
				}        
				}
				pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
			}while(pLoopAdapter!=NULL);

			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Deleted the Mac with port number %d", portNumber));
		}
		else
		{
			PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
			PlatformReleaseDataFrameQueued(pTargetAdapter);
			PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);
		}
		
	} while (FALSE);

	bCompleteStatus = N62CCompletePendingCreateDeleteMacOidRequest(pAdapter, NDIS_STATUS_SUCCESS);	

	//
	// We can now deactivate the port. We cannot do this while we are processing
	// the OID, else the OS would deadlock. 
	//
	if ((ndisStatus == NDIS_STATUS_SUCCESS) && (destroyPortNumber == TRUE) && bCompleteStatus)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Deactivate & free the NDIS_PORT  \n"));
#if 0
		if(!OS_SUPPORT_WDI)
		{
		N62CHelperDeactivateNdisPort(pTargetAdapter, portNumber);
		N62CHelperFreeNdisPort(pTargetAdapter, portNumber);			
	}
#endif
	}
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<==== N62CDeleteMac()\n"));
	return ndisStatus;
}

NDIS_STATUS
N62CHelperAllocateNdisPort(
	IN PADAPTER					pAdapter,
	OUT PNDIS_PORT_NUMBER		AllocatedPortNumber
	)
{
	NDIS_STATUS                 ndisStatus = NDIS_STATUS_SUCCESS;
	NDIS_PORT_CHARACTERISTICS   portChar;

	// Call NDIS to allocate the port
	NdisZeroMemory(&portChar, sizeof(NDIS_PORT_CHARACTERISTICS));

	N6_ASSIGN_OBJECT_HEADER(portChar.Header, NDIS_OBJECT_TYPE_DEFAULT,
	NDIS_PORT_CHARACTERISTICS_REVISION_1, sizeof(NDIS_PORT_CHARACTERISTICS));

	portChar.Flags = NDIS_PORT_CHAR_USE_DEFAULT_AUTH_SETTINGS;
	portChar.Type = NdisPortTypeUndefined;
	portChar.MediaConnectState = MediaConnectStateConnected;
	portChar.XmitLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
	portChar.RcvLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
	portChar.Direction = NET_IF_DIRECTION_SENDRECEIVE;
	portChar.SendControlState = NdisPortControlStateUnknown;
	portChar.RcvControlState = NdisPortControlStateUnknown;
	portChar.SendAuthorizationState = NdisPortControlStateUncontrolled; // Ignored
	portChar.RcvAuthorizationState = NdisPortControlStateUncontrolled; // Ignored


	ndisStatus = NdisMAllocatePort(pAdapter->pNdisCommon->hNdisAdapter, &portChar);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Failed to allocate NDIS port. Status = 0x%08x\n",
		ndisStatus));
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Associated Port Number %d with allocated port\n", 
		portChar.PortNumber));

		// Return the NDIS port number that has been allocated to this port
		*AllocatedPortNumber = portChar.PortNumber;
	}

	return ndisStatus;
}

// MiniportOidRequest - Called from MP layer to Port
NDIS_STATUS 
N62CStaOidHandler(
	IN  PADAPTER			pAdapter,
	IN  PMP_PORT                Port,
	IN  PNDIS_OID_REQUEST       OidRequest
	)
{
	UNREFERENCED_PARAMETER(Port);
	UNREFERENCED_PARAMETER(OidRequest);
	return NDIS_STATUS_SUCCESS;
}

/* See prototype for documentation */
NDIS_STATUS 
N62CStaSendEventHandler(
    IN  PMP_PORT                Port,
   // IN  PMP_TX_MSDU             PacketList,
    IN  ULONG                   SendFlags
    )
{
	UNREFERENCED_PARAMETER(Port);
	//UNREFERENCED_PARAMETER(PacketList);
	UNREFERENCED_PARAMETER(SendFlags);

	return NDIS_STATUS_SUCCESS;
}

/* See prototype for documentation */
NDIS_STATUS
N62CStaReceiveEventHandler(
    IN  PMP_PORT                Port,
   // IN  PMP_RX_MSDU             PacketList,
    IN  ULONG                   ReceiveFlags
    )
{
	UNREFERENCED_PARAMETER(Port);
	//UNREFERENCED_PARAMETER(PacketList);
	UNREFERENCED_PARAMETER(ReceiveFlags);
	return NDIS_STATUS_SUCCESS;
}

// TODO: Win 7 we should add some vnic function.
NDIS_STATUS
N62CHelperHandleMiniportRestart(
	IN PADAPTER								pAdapter,
	IN  PNDIS_MINIPORT_RESTART_PARAMETERS   MiniportRestartParameters
	)
/*++

Routine Description:

	NDIS calls the miniport driver's MiniportRestart function to cause
	the miniport to return the adapter to the Running state.
	During the restart, the miniport driver must complete any tasks
	that are required to resume send and receive operations before
	completing the restart request.

Argument:

	MiniportAdapterContext  Pointer to our adapter

Return Value:

	NDIS_STATUS_SUCCESS
	NDIS_STATUS_PENDING  Can it return pending
	NDIS_STATUS_XXX      The driver fails to restart


--*/
{
	N6C_MP_DRIVER_STATE CurrDriverState;

	UNREFERENCED_PARAMETER(MiniportRestartParameters);
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("==> N62CHelperHandleMiniportRestart(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));

	//
	// Reset rx packet waiting mechanism.
	//
	N6CStartWaitReturnPacketMechanism(pAdapter);

	//
	// Mark the miniport driver as running state to accept tx/rx request.
	//
	//CurrDriverState = N6C_GET_MP_DRIVER_STATE(pAdapter);
	//RT_ASSERT(	(CurrDriverState == MINIPORT_PAUSED),
	//			("N62CHelperHandleMiniportRestart(): unexpected state (%#d)!!!\n", CurrDriverState));

	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_RUNNING);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<== N62CHelperHandleMiniportRestart(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N62CHelperHandleMiniportPause(
	IN PADAPTER								pAdapter,
	IN  PNDIS_MINIPORT_PAUSE_PARAMETERS     MiniportPauseParameters
	)
{
	N6C_MP_DRIVER_STATE CurrDriverState;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("==> N62CHelperHandleMiniportPause(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));

	//
	// Mark the miniport driver as pausing state to reject tx/rx request.
	//
	CurrDriverState = N6C_GET_MP_DRIVER_STATE(pAdapter);
/*
	RT_ASSERT(	(CurrDriverState < MINIPORT_PAUSED ||
				CurrDriverState > MINIPORT_PAUSING),
				("MiniportPause(): unexpected state (%#d)!!!\n", CurrDriverState));
*/
	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_PAUSING);

	//
	// Wait all NBL indicated up returned.
	//
	N6CWaitForReturnPacket(pAdapter);

	//
	// Mark the miniport driver as paused state.
	//
	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_PAUSED);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<== N62CHelperHandleMiniportPause(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N62CHelperActivateNdisPort(
	IN PADAPTER				  pAdapter,
	IN  NDIS_PORT_NUMBER        PortNumberToActivate
	)
{
	NDIS_STATUS                 ndisStatus = NDIS_STATUS_SUCCESS;
	NET_PNP_EVENT_NOTIFICATION  netPnpEventNotification;
	NDIS_PORT                   ndisPort;
	PNDIS_PORT_CHARACTERISTICS  portChar;

	NdisZeroMemory(&netPnpEventNotification, sizeof(NET_PNP_EVENT_NOTIFICATION));
	NdisZeroMemory(&ndisPort, sizeof(NDIS_PORT));

	N6_ASSIGN_OBJECT_HEADER(netPnpEventNotification.Header, NDIS_OBJECT_TYPE_DEFAULT,
	NET_PNP_EVENT_NOTIFICATION_REVISION_1, sizeof(NET_PNP_EVENT_NOTIFICATION));

	netPnpEventNotification.NetPnPEvent.NetEvent = NetEventPortActivation;

	// Refill the characteristics structure for the port
	portChar = &(ndisPort.PortCharacteristics);
	
	N6_ASSIGN_OBJECT_HEADER(portChar->Header, NDIS_OBJECT_TYPE_DEFAULT,
	NDIS_PORT_CHARACTERISTICS_REVISION_1, sizeof(NDIS_PORT_CHARACTERISTICS));

	portChar->Flags = NDIS_PORT_CHAR_USE_DEFAULT_AUTH_SETTINGS;
	portChar->Type = NdisPortTypeUndefined;
	portChar->MediaConnectState = MediaConnectStateConnected;
	portChar->XmitLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
	portChar->RcvLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
	portChar->Direction = NET_IF_DIRECTION_SENDRECEIVE;
	portChar->SendControlState = NdisPortControlStateUnknown;
	portChar->RcvControlState = NdisPortControlStateUnknown;
	portChar->SendAuthorizationState = NdisPortControlStateUncontrolled; // Ignored
	portChar->RcvAuthorizationState = NdisPortControlStateUncontrolled; // Ignored
	portChar->PortNumber = PortNumberToActivate;

	// Single port is being activated
	ndisPort.Next = NULL;

	// We need to save a pointer to the NDIS_PORT in the NetPnPEvent::Buffer field
	netPnpEventNotification.NetPnPEvent.Buffer = (PVOID)&ndisPort;
	netPnpEventNotification.NetPnPEvent.BufferLength = sizeof(NDIS_PORT);
	
	ndisStatus = NdisMNetPnPEvent(pAdapter->pNdisCommon->hNdisAdapter, &netPnpEventNotification);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Failed to activate NDIS port %d. Status = 0x%08x\n", 
		PortNumberToActivate, ndisStatus));
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Activated Port Number %d\n", PortNumberToActivate));
	} 

	return ndisStatus;
}

VOID
N62CHelperDeactivateNdisPort(
	IN PADAPTER				pAdapter,
    IN  NDIS_PORT_NUMBER        PortNumberToDeactivate
    )
{
	NDIS_STATUS                 ndisStatus = NDIS_STATUS_SUCCESS;
	NET_PNP_EVENT_NOTIFICATION  netPnpEventNotification;
	NDIS_PORT_NUMBER            portNumberArray[1];

	NdisZeroMemory(&netPnpEventNotification, sizeof(NET_PNP_EVENT_NOTIFICATION));

	N6_ASSIGN_OBJECT_HEADER(netPnpEventNotification.Header, NDIS_OBJECT_TYPE_DEFAULT,
	NET_PNP_EVENT_NOTIFICATION_REVISION_1, sizeof(NET_PNP_EVENT_NOTIFICATION));

	netPnpEventNotification.NetPnPEvent.NetEvent = NetEventPortDeactivation;

	// We need to save a pointer to the NDIS_PORT_NUMBER in the NetPnPEvent::Buffer field
	portNumberArray[0] = PortNumberToDeactivate;            
	netPnpEventNotification.NetPnPEvent.Buffer = (PVOID)portNumberArray;
	netPnpEventNotification.NetPnPEvent.BufferLength = sizeof(NDIS_PORT_NUMBER);

	ndisStatus = NdisMNetPnPEvent(pAdapter->pNdisCommon->hNdisAdapter, &netPnpEventNotification);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Failed to deactivate NDIS port %d. Status = 0x%08x\n", 
		PortNumberToDeactivate, ndisStatus));
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Deactivated Port Number %d\n", PortNumberToDeactivate));
	}        
}

VOID
N62CHelperFreeNdisPort(
    IN PADAPTER				pAdapter,
    IN  NDIS_PORT_NUMBER        PortNumberToFree
    )
{
	NDIS_STATUS                 ndisStatus;

	// Free the NDIS port

	ndisStatus = NdisMFreePort(pAdapter->pNdisCommon->hNdisAdapter, PortNumberToFree);
	RT_TRACE(COMP_INIT, DBG_LOUD, ("N62CHelperFreeNdisPort\n"));
}

NDIS_STATUS
N62CHandleMiniportPause(
	IN PADAPTER								pAdapter,
	IN  PNDIS_MINIPORT_PAUSE_PARAMETERS	MiniportPauseParameters
	)
{
	N6C_MP_DRIVER_STATE CurrDriverState;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("==> N62CHandleMiniportPause(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));

	//
	// Mark the miniport driver as pausing state to reject tx/rx request.
	//
	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_PAUSING);

	//
	// Wait all NBL indicated up returned.
	//
	N6CWaitForReturnPacket(pAdapter);

	//
	// Mark the miniport driver as paused state.
	//
	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_PAUSED);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<== N62CHandleMiniportPause(), DriverState(%d)\n", N6C_GET_MP_DRIVER_STATE(pAdapter)));

	return NDIS_STATUS_SUCCESS;	
}





MP_PORT_TYPE
N62CGetPortTypeByOpMode(
    IN  ULONG                   OpMode
    )
{
    switch (OpMode)
    {
        case DOT11_OPERATION_MODE_EXTENSIBLE_STATION:
        case DOT11_OPERATION_MODE_NETWORK_MONITOR:
            return EXTSTA_PORT;

        case DOT11_OPERATION_MODE_EXTENSIBLE_AP:
            return EXTAP_PORT;
			
	case DOT11_OPERATION_MODE_WFD_DEVICE:
		return EXT_P2P_DEVICE_PORT;
	case DOT11_OPERATION_MODE_WFD_GROUP_OWNER:
	case DOT11_OPERATION_MODE_WFD_CLIENT:
		return EXT_P2P_ROLE_PORT;

        default:
            return EXTSTA_PORT;
    }
}


NDIS_STATUS
N62CSetOperationMode(
	IN  PADAPTER			pAdapter,
	IN  ULONG			OpMode
    	)
{
	PRT_NDIS6_COMMON pNdisCommon = pAdapter->pNdisCommon;
	if (OpMode == DOT11_OPERATION_MODE_NETWORK_MONITOR)
	{
		// Disable autoconfig when in Netmon mode
		pNdisCommon->dot11AutoConfigEnabled = FALSE;
	}
	else
	{
		// By default autoconfig is enabled
		pNdisCommon->dot11AutoConfigEnabled = TRUE;
	}

	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
N62CPortSetOperationMode(
    IN  PADAPTER		   pAdapter,
    IN  ULONG                   OpMode
    )
{
	NDIS_STATUS                 ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_NDIS62_COMMON	pNdis62Common = pAdapter->pNdis62Common;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;

	//pNdisCommon->dot11CurrentOperationMode.uCurrentOpMode = OpMode;
	switch (pNdis62Common->PortType)
	{
		case EXTSTA_PORT:
			ndisStatus = N62CSetOperationMode(pAdapter, OpMode);
			break;

		case EXTAP_PORT:
			ndisStatus = NDIS_STATUS_SUCCESS;
			break;

		default:
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Attempting to notify op mode change event to unrecognized port type %d\n", pNdis62Common->PortType));
			ndisStatus = NDIS_STATUS_INVALID_PARAMETER;
			break;
	}

	//Add to set the active port when this oid is set, for avoid ap send beacon on the wrong channel.
	//when ap mode is set on the ext port, by Maddest 080716.
	//GetActiveAdapter(GetDefaultAdapter(pAdapter))=pAdapter;
	//GetActiveAdapter(GetExtAdapter(pAdapter))=pAdapter;
	return ndisStatus;
}

void
N62CDeleteVirtualPort(
    IN  PADAPTER		   pAdapter
	)
{
	PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
	PlatformReleaseDataFrameQueued(pAdapter);
	PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

	N62CHelperDeactivateNdisPort(pAdapter, pAdapter->pNdis62Common->PortNumber);
	N62CHelperFreeNdisPort(pAdapter, pAdapter->pNdis62Common->PortNumber);
}

NDIS_STATUS
N62CChangePortTypeByOpMode(
	IN 	PADAPTER pAdapter,
	IN	PDOT11_CURRENT_OPERATION_MODE	dot11OpMode
	)
{
	PRT_NDIS62_COMMON	pNdis62Common = pAdapter->pNdis62Common;
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	MP_PORT_TYPE		newPortType = N62CGetPortTypeByOpMode(dot11OpMode->uCurrentOpMode);	
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===>N62CChangePortTypeByOpMode(): Port Number: %d\n", pNdis62Common->PortNumber));
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("===>N62CChangePortTypeByOpMode(): MP_PORT_TYPE: newPortType %d,  DOT11_CURRENT_OPERATION_MODE: newOpMode %d\n", newPortType, dot11OpMode->uCurrentOpMode));

	// Win8: Change Port Type
	// 	+ If Port Type is Role Port, always change the mode and reset the internal variables
	if (pNdis62Common->PortType != newPortType || pNdis62Common->PortType == EXT_P2P_ROLE_PORT)
	{
		// First we pause the current port
		ndisStatus = N62CHandleMiniportPause(pAdapter, NULL);
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, 
				("Failed to pause port %d to change Op Mode. Status = 0x%08x\n", 
				pNdis62Common->PortNumber, ndisStatus));

			return ndisStatus;
		}
				
		// ExtAP and ExtSTA are handled here
 		N62CChangePortType(pAdapter, pNdis62Common->PortType, newPortType);
		
		pNdis62Common->PortType = newPortType;


{
		P2P_ROLE PortRole = P2P_NONE;

		switch(dot11OpMode->uCurrentOpMode)
		{
			case DOT11_OPERATION_MODE_WFD_DEVICE:
				PortRole = P2P_DEVICE;
				break;
			case DOT11_OPERATION_MODE_WFD_GROUP_OWNER:
				PortRole = P2P_GO;
				break;
			case DOT11_OPERATION_MODE_WFD_CLIENT:
				PortRole = P2P_CLIENT;
				break;

			default:
				RT_TRACE(COMP_OID_SET, DBG_LOUD, 
					("Unknown Port Type Observed:  dot11OpMode->uCurrentOpMode %d!\n", 
					dot11OpMode->uCurrentOpMode)
				);
				break;
		}

		if(PortRole != P2P_NONE)
		{ 
			ndisStatus = N63CResetWifiDirectPorts(pAdapter, PortRole, RESET_LEVEL_FULL);
		}
}

		//
		// Restart the adapter.
		//
		ndisStatus = N62CHelperHandleMiniportRestart(pAdapter, NULL);
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, 
				("Failed to restart port %d after change of Op Mode. Status = 0x%08x\n", 
			pNdis62Common->PortNumber, ndisStatus));
		}

	}

#if (P2P_SUPPORT == 1)
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== %s: MP_PORT_TYPE: pNdis62Common->PortType: %d, P2P_ROLE: pP2PInfo->Role: %d\n", __FUNCTION__, pNdis62Common->PortType, GET_P2P_INFO(pAdapter)->Role));
#endif
	return ndisStatus;
}

VOID
N62CChangePortType(
	IN  PADAPTER			pAdapter,
	IN  MP_PORT_TYPE		OldPortType,
	IN  MP_PORT_TYPE		NewPortType
	)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_NDIS62_COMMON pNdis62Common = pAdapter->pNdis62Common;
	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	do
	{
		if(OldPortType==NewPortType)
		{
			break;	
		}
		switch(NewPortType)

		{
			case EXTSTA_PORT:
				{
					pNdis62Common->CurrentOpState=INIT_STATE;
					MgntActSet_ApType(pAdapter, FALSE);
					N62CResetAPVariables(pAdapter, FALSE);
					ResetMgntVariables(pAdapter);

					#if P2P_SUPPORT == 1
					if(OldPortType > EXTAP_PORT && OldPortType <= EXT_P2P_ROLE_PORT)
					{
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Change to EXTSTA_PORT, PortNum = %d, OldPortType = %d, disable P2P Mode!\n", pAdapter->pNdis62Common->PortNumber, OldPortType));
						// Stop P2P Mode: Last 3 parameters are don't care
						MgntActSet_P2PMode(pAdapter, FALSE, FALSE, 0, 0, 0);
					}
					#endif
					
					break;
				}
			case EXTAP_PORT:
				{
					pNdis62Common->CurrentOpState=INIT_STATE;
					N62CResetAPVariables(pAdapter, TRUE);
					SetAPState(pAdapter, AP_STATE_STOPPED);

					#if P2P_SUPPORT == 1
					if(OldPortType > EXTAP_PORT && OldPortType <= EXT_P2P_ROLE_PORT)
					{
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Change to EXTAP_PORT, PortNum = %d, OldPortType = %d, disable P2P Mode!\n", pAdapter->pNdis62Common->PortNumber, OldPortType));
						// Stop P2P Mode: Last 3 parameters are don't care
						MgntActSet_P2PMode(pAdapter, FALSE, FALSE, 0, 0, 0);
					}
					#endif
					
					break;
				}
			default:
			break;
		}
	}while(FALSE);
	return ;	
}

VOID
N62CExtAdapterHandleNBLInWaitQueue(
	IN PADAPTER Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon;
	PRT_SDIO_DEVICE		pDevice;
	PNET_BUFFER_LIST	pNetBufferList;
	PADAPTER			pAdapter = GetFirstExtAdapter(Adapter);

	while(pAdapter != NULL)
	{
		pNdisCommon = pAdapter->pNdisCommon;
		pDevice = GET_RT_SDIO_DEVICE(pAdapter);

		if(PlatformAtomicExchange(&pAdapter->IntrNBLRefCount, TRUE)==TRUE)
			return;
	
		//
		// Send NBL in wait queue.
		//
		PlatformAcquireSpinLock(pAdapter, RT_TX_SPINLOCK);
		while(pDevice->SendingNetBufferList == NULL)
		{
			PlatformAcquireSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
			if(N6CIsNblWaitQueueEmpty(pNdisCommon->TxNBLWaitQueue))
			{
				PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
				break;
			}

			if(!pNdisCommon->bReleaseNblWaitQueueInProgress)
			{
				pNetBufferList = N6CGetHeadNblWaitQueue(pNdisCommon->TxNBLWaitQueue);
				PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
				RT_TRACE(COMP_SEND, DBG_TRACE, ("N6PciHandleInterrupt(): N6CGetHeadNblWaitQueue(): pCurrNetBufferList(%p)\n", pNetBufferList ));

				if( N6SDIO_CANNOT_TX(pAdapter) )
				{
					RT_TRACE(COMP_SEND, DBG_TRACE, ("N6PciHandleInterrupt(): bread for CANNOT_TX()\n"));
					break;
				}

				if( !N6SdioSendSingleNetBufferList(
						pAdapter, 
						pNetBufferList,
						TRUE)) // bFromQueue
				{
					RT_TRACE(COMP_SEND, DBG_TRACE, ("N6PciHandleInterrupt(): N6PciSendSingleNetBufferList() returns FALSE\n"));
					break;
				}
			}
			else
			{
				PlatformReleaseSpinLock(pAdapter, RT_BUFFER_SPINLOCK);
				RT_TRACE(COMP_SEND, DBG_TRACE, ("N6PciHandleInterrupt(): bReleaseNblWaitQueueInProgress\n"));
			}
		}
		PlatformReleaseSpinLock(pAdapter, RT_TX_SPINLOCK);

		PlatformAtomicExchange(&pAdapter->IntrNBLRefCount, FALSE);
	}
}


