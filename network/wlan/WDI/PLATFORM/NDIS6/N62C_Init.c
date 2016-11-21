#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "N62C_Init.tmh"
#endif

VOID
N62CCopyDefaultVariablesToExt(
    IN  PADAPTER                Adapter
    )
{
	Adapter->HWDescHeadLength = 
		GetDefaultAdapter(Adapter)->HWDescHeadLength;
	
	Adapter->TXPacketShiftBytes = 
		GetDefaultAdapter(Adapter)->TXPacketShiftBytes;

	Adapter->MAX_TRANSMIT_BUFFER_SIZE = 
		GetDefaultAdapter(Adapter)->MAX_TRANSMIT_BUFFER_SIZE;

	Adapter->RT_TCB_NUM = GetDefaultAdapter(Adapter)->RT_TCB_NUM;
	
	Adapter->RT_LOCAL_BUF_NUM = 
		GetDefaultAdapter(Adapter)->RT_LOCAL_BUF_NUM;
	
	Adapter->RT_LOCAL_FW_BUF_NUM = 
		GetDefaultAdapter(Adapter)->RT_LOCAL_FW_BUF_NUM;
	
	Adapter->RT_TXDESC_NUM = GetDefaultAdapter(Adapter)->RT_TXDESC_NUM;
	
	Adapter->RT_TXDESC_NUM_BE_QUEUE = 
		GetDefaultAdapter(Adapter)->RT_TXDESC_NUM_BE_QUEUE;

	Adapter->MAX_RECEIVE_BUFFER_SIZE = 
		GetDefaultAdapter(Adapter)->MAX_RECEIVE_BUFFER_SIZE;

	Adapter->MAX_NUM_RFD = GetDefaultAdapter(Adapter)->MAX_NUM_RFD;
	Adapter->MAX_NUM_RX_DESC = GetDefaultAdapter(Adapter)->MAX_NUM_RX_DESC;	
}


NDIS_STATUS
N62CInitializeExtAdapter(
	IN	PADAPTER		pAdapter,	
       IN    NDIS_HANDLE         MiniportAdapterHandle,
    	IN  PNDIS_MINIPORT_INIT_PARAMETERS     MiniportInitParameters
	)
{
	// return this status 
	NDIS_STATUS			ndisStatus = NDIS_STATUS_SUCCESS;
	
	PADAPTER 			pDefaultAdapter = GetDefaultAdapter(pAdapter);
	RT_STATUS			RtStatus = NDIS_STATUS_FAILURE;
	PMGNT_INFO			pMgntInfo = NULL;
	PRT_NDIS6_COMMON	pNdisCommon = NULL;

	
	// N62C Specific Resource Elements --------------------------
	BOOLEAN				bMgntVariableInitialized = FALSE,
						bRxVariableInitialized = FALSE,
						bBeaconBufferInitialized = FALSE;
	// ------------------------------------------------------

	RES_MON_OBJ		ResMonObj;
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(pAdapter);
	PRT_SDIO_DEVICE	Defualtsdiodevice = GET_RT_SDIO_DEVICE(pDefaultAdapter);
	
	RT_TRACE(COMP_DBG, DBG_TRACE, ("===> N62CInitializeExtAdapter()	SDIO\n"));

	INIT_RES_MON_OBJ(ResMonObj);
	
	pAdapter->HardwareType = pDefaultAdapter->HardwareType;
	
	sdiodevice->pAdapter = pAdapter;		
	sdiodevice->hNdisAdapter = MiniportAdapterHandle;
	pMgntInfo = &(pAdapter->MgntInfo);
	pNdisCommon = pAdapter->pNdisCommon;	

	pNdisCommon->NdisVersion = pDefaultAdapter->pNdisCommon->NdisVersion;
	RT_TRACE_F(COMP_INIT, DBG_LOUD, ("NdisVersion direct get 0x%x\n", pNdisCommon->NdisVersion));

	sdiodevice->FunctionalDeviceObject =Defualtsdiodevice->FunctionalDeviceObject;
	sdiodevice->pSdioDevObj = Defualtsdiodevice->pSdioDevObj;
	sdiodevice->pPhysDevObj = Defualtsdiodevice->pPhysDevObj;
#if USE_WDF_SDIO
	sdiodevice->hWdfDevice = Defualtsdiodevice->hWdfDevice;
#endif
	sdiodevice->RxNetBufferListPool = Defualtsdiodevice->RxNetBufferListPool;

	pAdapter->NdisSdioDev.pAdapter=pAdapter;
	pAdapter->pNdisCommon->hNdisAdapter= MiniportAdapterHandle;


	pAdapter->pNdisCommon->WDISupport = pDefaultAdapter->pNdisCommon->WDISupport;

	N62CCopyDefaultVariablesToExt(pAdapter);	
	
	// Initialize event.
	NdisInitializeEvent(&(sdiodevice->evtSendingNBLCompleted));

	// SyncIo Method 2.  
	KeInitializeEvent( &(sdiodevice->SyncIoEvent), NotificationEvent, TRUE);
	
	//To avoid allociate pfirmware again.
	pAdapter->bInitByExtPort=TRUE;
	//For debug by Maddest
	HalAssociateNic(pAdapter, FALSE);
	// ExtPort do not need this!!
	//ADD_RES_TO_MON(ResMonObj, InitRM_AsocNIC); // Add to resource monitor.
	sdiodevice->nIrpPendingCnt = 0;

	// Read the registry parameters
	ndisStatus= N6SdioReadRegParameters(sdiodevice);
	if (ndisStatus != NDIS_STATUS_SUCCESS) 
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6SdioReadRegParameters(X): Read Registry Parameter Failed!\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto error;
	} 

	InitializeTxVariables(pAdapter);

	InitializeRxVariables(pAdapter);
	bRxVariableInitialized = TRUE;
	
	InitializeMgntVariables(pAdapter);
	bMgntVariableInitialized = TRUE;

	
		
	//----------------------------------------------------------------------------
	// Update the parameter read from registery to coresponding ones in MGNT_INFO.
	// NOTE! These modification should be after InitializeMgntVariables() which is called by 
	// NicIFAssociateNIC().
	// 2005.01.13, by rcnjko.
	//
	N6UpdateDefaultSetting(pAdapter);

	pAdapter->bInHctTest = pDefaultAdapter->bInHctTest;
	// TODO: We should not set HW Setting Here

	//----------------------------------------------------------------------------

	pNdisCommon->MaxPktSize=NIC_MAX_PACKET_SIZE;

	// Read adapter information such as MAC address from EEPROM
	//
	// Read data from EEPROM and do some customization 
	// according to custermer ID read from EEPROM.
	//
	//NicIFReadAdapterInfo(pAdapter);
	Dot11_UpdateDefaultSetting(pAdapter);
	HT_UpdateDefaultSetting(pAdapter);
	VHT_UpdateDefaultSetting(pAdapter);

	{// make sure that DefaultAdapter is currently available.
		PHAL_DATA_TYPE  pExtHalData = GET_HAL_DATA(pAdapter); // the same one
		PHAL_DATA_TYPE	pDefaultHalData = GET_HAL_DATA(pDefaultAdapter);

		PMGNT_INFO		pDefaultMgntInfo = &(pDefaultAdapter->MgntInfo);
		PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
		
		pAdapter->EepromAddressSize = pDefaultAdapter->EepromAddressSize;

		PlatformMoveMemory(pAdapter->PermanentAddress, pDefaultAdapter->PermanentAddress, 6);
		pAdapter->bInHctTest = pDefaultAdapter->bInHctTest;
		pMgntInfo->bSupportTurboMode = pDefaultMgntInfo->bSupportTurboMode;
		pMgntInfo->bAutoTurboBy8186 =  pDefaultMgntInfo->bAutoTurboBy8186;
//		pMgntInfo->bInactivePs = pDefaultMgntInfo->bInactivePs;
//		pMgntInfo->bIPSModeBackup = pDefaultMgntInfo->bIPSModeBackup;
//		pMgntInfo->bLeisurePs = pDefaultMgntInfo->bLeisurePs;
		pMgntInfo->pStaQos->QosCapability = pDefaultMgntInfo->pStaQos->QosCapability;
		pMgntInfo->SecurityInfo.RegSWTxEncryptFlag = pDefaultMgntInfo->SecurityInfo.RegSWTxEncryptFlag;
		pMgntInfo->SecurityInfo.RegSWRxDecryptFlag = pDefaultMgntInfo->SecurityInfo.RegSWRxDecryptFlag;
	}

	DefragInitialize(pAdapter);


	RtStatus = MgntAllocateBeaconBuf(pAdapter);

	if(RtStatus!=RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("MgntAllocateBeaconBuf(pAdapter) failed: %#X\n", RtStatus));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto error;
	}
	else bBeaconBufferInitialized = TRUE;

	if(pNdisCommon->bOverrideAddress)
	{
		USHORT nIdx;
		for(nIdx = 0; nIdx < 6; nIdx++)
		{
			//PlatformEFIOWrite1Byte(pAdapter, (IDR0+nIdx), pNdisCommon->CurrentAddress[nIdx]);
			pAdapter->CurrentAddress[nIdx] = pNdisCommon->CurrentAddress[nIdx];
		}
	}
	else
	{
		ETH_COPY_NETWORK_ADDRESS(pNdisCommon->CurrentAddress, pAdapter->PermanentAddress);
	}

	//
	// Before setting of attributes, update the native 802.11 related variable.
	// 2006.10.09, by shien chang.
	//
	ndisStatus = N6SdioAllocateNative80211MIBs(pAdapter);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N6AllocateNative80211MIBs failed\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto error;
	}
	
	ADD_RES_TO_MON(ResMonObj, InitRM_AllocNWifi);

	N6InitializeNative80211MIBs(pAdapter);


	// Initialize common NDIS resource in PRT_NDIS6_COMMON. 2006.05.07, by rcnjko.
	InitNdis6CommonResources(pAdapter);		
	ADD_RES_TO_MON(ResMonObj, InitRM_CommonRS);


	pMgntInfo->NdisVersion = MgntTranslateNdisVersionToRtNdisVersion(pNdisCommon->NdisVersion);

#if POWER_MAN
	sdiodevice->CurrentPowerState = NdisDeviceStateD0;
#endif

	//
	// Mark the miniport driver as paused state. 
	// Note that, please keep this action as last one in MiniportInitializeEx().
	//
	N6C_SET_MP_DRIVER_STATE(pAdapter, MINIPORT_PAUSED);
		
	RT_TRACE(COMP_DBG, DBG_TRACE, ("<=== N62CInitializeExtAdapter(), Initialized Successfully!\n"));
	return ndisStatus;

error:

	RT_TRACE(COMP_INIT, DBG_LOUD, ("<== N62CInitializeExtAdapter(), Initialize failed, clean up resources.\n"));

	if(bBeaconBufferInitialized) 	MgntFreeBeaconBuf(pAdapter);
	if(bMgntVariableInitialized) 	DeInitializeMgntVariables(pAdapter);
	if(bRxVariableInitialized) 	DeInitializeRxVariables(pAdapter);

	CLEANUP_RES_IN_MON(ResMonObj, pAdapter);	
	return ndisStatus;	

}


/*
 *2008/05/09 Add by Mars
 *
 * Win 7 Initialize Here, for tell N62 initial process we use compile flag
 * Here Initialize everything releated N62
 *
 * @param	pAdapter is driver main strucutre.  
*/

NDIS_STATUS
N62CInitialize(
	IN	PADAPTER		pAdapter,	
       IN    NDIS_HANDLE         MiniportAdapterHandle,
    	IN  	PNDIS_MINIPORT_INIT_PARAMETERS     MiniportInitParameters
	)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	u1Byte 			i;
	PADAPTER		pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER		pExtAdapter = NULL;
	
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("===>N62CInitialize()\n"));


	// Allocate Ndis 6.20+ PortCommonComponent -----------------------------------------------------------
	ndisStatus = N62CAllocatePortCommonComponent(pDefaultAdapter);
	if(ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: N62CAllocatePortCommonComponent: Failure!\n", __FUNCTION__));
		return NDIS_STATUS_FAILURE;
	}
	// ------------------------------------------------------------------------------------------------


	// Allocate Ndis 6.20+ PortSpecificComponent ------------------------------------------------------------
	ndisStatus = N62CAllocatePortSpecificComponent(pDefaultAdapter);
	if(ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: N62CAllocatePortSpecificComponent: Failure!\n", __FUNCTION__));
		N62CFreePortCommonComponent(pDefaultAdapter);
		return NDIS_STATUS_FAILURE;
	}
	// ------------------------------------------------------------------------------------------------


	// Initialize variables for Ndis 6.20+ ---
	N62CInitVariable(pDefaultAdapter);
	// -------------------------------
	

	// Prepare Each Extension Port ------------------------------------------------------------------------
	for( i = 1; i < MP_DEFAULT_NUMBER_OF_PORT; i++)
	{
		ndisStatus = N62CAllocateExtAdapter(pDefaultAdapter, &pExtAdapter, MiniportAdapterHandle);

		if(ndisStatus != NDIS_STATUS_SUCCESS)
		{

			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("%s: N62CAllocateExtAdapter: Failure!\n", __FUNCTION__));
			break;	
		}

		cpMacAddr(pExtAdapter->CurrentAddress,pDefaultAdapter->CurrentAddress);

		PlatformMoveMemory(
				pExtAdapter->pNdis62Common, 
				pDefaultAdapter->pNdis62Common, 
				sizeof(RT_NDIS62_COMMON)
			);
		
		pExtAdapter->pNdis62Common->PortNumber= 0;
		pExtAdapter->bHWInitReady = pDefaultAdapter->bHWInitReady;
		RT_TRACE(COMP_INIT, DBG_LOUD, ("N62CInitialize(), set pExtAdapter->bHWInitReady=pDefaultAdapter->bHWInitReady = %d\n",
			pDefaultAdapter->bHWInitReady));
		pExtAdapter->bSWInitReady = FALSE;

		ndisStatus = N62CInitializeExtAdapter(
				pExtAdapter, 
				MiniportAdapterHandle, 
				MiniportInitParameters
			);
		
		if(ndisStatus != NDIS_STATUS_SUCCESS)
		{	
			// Release the resource allocated in N62CAllocateExtAdapter
			N62CFreePortSpecificComponent(pExtAdapter);
			N6FreeAdapter(pExtAdapter);
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("%s: N62CInitializeExtAdapter: Failure!\n", __FUNCTION__));
			break;
		}


		//=======================================================
		// Check Point: All Resources are Prepared for Extension Port
		//=======================================================

		// MultiPort Adapter Initialization ------------------------------
		MultiPortInsertIdleExtAdapter(pDefaultAdapter, pExtAdapter);
		// --------------------------------------------------------

		pExtAdapter->pNdis62Common->PortType=EXTSTA_PORT;
		pExtAdapter->pNdis62Common->CurrentOpState=INIT_STATE;
		pExtAdapter->pNdis62Common->bWPSEnable=FALSE;
		pExtAdapter->bSWInitReady=TRUE;
	}
	// -----------------------------------------------------------------------------------------------


	if(ndisStatus != NDIS_STATUS_SUCCESS)
	{
		// Free All Extension Ports
		NDIS_6_2_FREE_EXTENSION_COMPONENT(pDefaultAdapter);
	}
	else
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("%d Adapters have been allocated <===N62CInitialize ()\n", i));	
	}
	
	return ndisStatus;
}

NDIS_STATUS
N62CAllocateExtAdapter(
	IN	PADAPTER		pDefaultAdapter,
	IN	PADAPTER 		*ppHelperAdapter,
	IN	NDIS_HANDLE	MiniportAdapterHandle
	)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;

	do 
	{
		//
		// Allocate the adapter structure.
		//
		rtStatus = PlatformAllocateMemory(
				pDefaultAdapter, 
				(PVOID*)&(*ppHelperAdapter), 
				sizeof(ADAPTER)
			);
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N62CAllocateExtAdapter(): failed to allocate ExtAdapter!!!\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}
		else
		{
			PlatformZeroMemory((*ppHelperAdapter), sizeof(ADAPTER));
		}
		
		//
		// Allocate RT_NDIS6_COMMON.
		//
		rtStatus = PlatformAllocateMemory(
				pDefaultAdapter, 
				(PVOID)&((*ppHelperAdapter)->pNdisCommon), 
				sizeof(RT_NDIS6_COMMON)
			);
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N62CAllocateExtAdapter(): failed to allocate RT_NDIS6_COMMON for the ExtAdapter!!!\n"));
			PlatformFreeMemory(*ppHelperAdapter, sizeof(ADAPTER));

			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}
		else
		{
			PlatformZeroMemory((*ppHelperAdapter)->pNdisCommon, sizeof(RT_NDIS6_COMMON));
		}

		
		//
		// Store Handle to the ext adapter.
		//
		(*ppHelperAdapter)->pNdisCommon->hNdisAdapter = MiniportAdapterHandle;



		// Port Common Info -------------------------------------------------------
		(*ppHelperAdapter)->pPortCommonInfo = pDefaultAdapter->pPortCommonInfo;
		// ----------------------------------------------------------------------


		//
		// Allocate mgnt.
		//
		PlatformZeroMemory(&((*ppHelperAdapter)->MgntInfo), sizeof(MGNT_INFO));
		rtStatus = MgntAllocMemory((*ppHelperAdapter));
		if(rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, 
				("N62CAllocateExtAdapter(): failed to allocate MGNT_INFO for the ExtAdapter!!!\n"));

			PlatformFreeMemory((*ppHelperAdapter)->pNdisCommon, sizeof(RT_NDIS6_COMMON));
			
			PlatformFreeMemory((*ppHelperAdapter), sizeof(ADAPTER));
			
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}


		
		ndisStatus = N62CAllocatePortSpecificComponent(*ppHelperAdapter);
		if (ndisStatus != NDIS_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, 
				("N62CAllocateExtAdapter(): failed to allocate RT_NDIS6_COMMON for the ExtAdapter!!!\n"));

			MgntFreeMemory((*ppHelperAdapter));

			PlatformFreeMemory((*ppHelperAdapter)->pNdisCommon, sizeof(RT_NDIS6_COMMON));

			PlatformFreeMemory((*ppHelperAdapter), sizeof(ADAPTER));
			
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}
		
	}while(FALSE);

	return ndisStatus;
	
}

//
// Modified from N6PciHalt().
// by haich.
//
VOID
N62CFreeExtAdapter(
	IN PADAPTER pExtAdapter
)
{
	PADAPTER		pDefaultAdapter= GetDefaultAdapter(pExtAdapter);
	PRT_NDIS6_COMMON pNdisCommon = pExtAdapter->pNdisCommon;

	RT_TRACE(COMP_INIT, DBG_LOUD, ("===> N62CFreeExtAdapter()\n"));
	
	pExtAdapter->bDriverStopped = TRUE;
	
	// For AP mode. 2005.06.30, by rcnjko.
	if(ACTING_AS_AP(pExtAdapter))
	{
		AP_DisassociateAllStation(pExtAdapter, unspec_reason);
		NdisMSleep(500); // Sleep for a while here to allow disasoc completed sending out.
	}

	
	// Release common NDIS resource
	ReleaseNdis6CommonResources(pExtAdapter);

	N6SdioFreeNative80211MIBs(pExtAdapter);

	DeInitializeMgntVariables(pExtAdapter);
		
	DeInitializeRxVariables(pExtAdapter);


	PlatformAcquireSpinLock(pExtAdapter, RT_RX_SPINLOCK);
	PlatformAcquireSpinLock(pExtAdapter, RT_TX_SPINLOCK);

	// Return all packets queued in AP mode. 2005.06.21, by rcnjko.
	AP_PS_ReturnAllQueuedPackets(pExtAdapter, FALSE);

	MgntFreeBeaconBuf( pExtAdapter	);

	PlatformReleaseSpinLock(pExtAdapter, RT_TX_SPINLOCK);
	PlatformReleaseSpinLock(pExtAdapter, RT_RX_SPINLOCK);
		
	N62CFreePortSpecificComponent(pExtAdapter);
		
	N6FreeAdapter(pExtAdapter);
		
	RT_TRACE(COMP_INIT, DBG_LOUD, ("<=== N62CFreeExtAdapter()\n"));
}

NDIS_STATUS
N62CAllocatePortCommonComponent(
	IN	PADAPTER		pAdapter
)
{
	// Allocate Port Common Component for Ndis 6.20+
	
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;

	PPORT_COMMON_INFO pPortCommonInfo = pAdapter->pPortCommonInfo;
	
	do
	{
		// Allocate the helper structure that is shared by every adapter ------------------------------------------------------------------------
		rtStatus = PlatformAllocateMemory(
				pAdapter,
				&(pPortCommonInfo->pPortHelper), 
				sizeof(PORT_HELPER)
			);
		
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N62CAllocatePortCommonComponent(): failed to allocate memory for the HELPER structure.\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}
		else 
		{	
			PlatformZeroMemory(pPortCommonInfo->pPortHelper, sizeof(PORT_HELPER));
		}
		// ---------------------------------------------------------------------------------------------------------------------------



		// Allocate the WorkItem to Create and Delete Ports ------------------------------------------------------------------------
		rtStatus = PlatformInitializeWorkItem(pAdapter, 
				&(pPortCommonInfo->pPortHelper->CreateDeleteMacWorkitem),
				( RT_WORKITEM_CALL_BACK) N62CCreateDeleteMacWorkItemCallback,
				(PVOID) pAdapter, 
				"Ndis62CreateDeleteWorkitem"
			);


		PlatformInitializeTimer(
							pAdapter, 
							&(pPortCommonInfo->pPortHelper->CreateDeleteMacTimer), 
							(RT_TIMER_CALL_BACK)N62CCreateDeleteMacTimerCallback, 
							NULL, 
							"CreateDeleteMacTimer");


		if(rtStatus != RT_STATUS_SUCCESS)
		{
			PlatformFreeMemory(pPortCommonInfo->pPortHelper, sizeof(PORT_HELPER));
			RT_TRACE(COMP_INIT, DBG_LOUD, ("N62CAllocatePortCommonComponent(): WorkItem Creation Failure.\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}
		// ------------------------------------------------------------------------------------------------------------------
	} while(FALSE);
	
	return	ndisStatus;
}

VOID
N62CFreePortCommonComponent(
	IN	PADAPTER		pAdapter
)
{
	PPORT_COMMON_INFO pPortCommonInfo = pAdapter->pPortCommonInfo;

	PlatformFreeWorkItem( &(pPortCommonInfo->pPortHelper->CreateDeleteMacWorkitem));
	PlatformReleaseTimer(pAdapter, &(pPortCommonInfo->pPortHelper->CreateDeleteMacTimer));
	PlatformFreeMemory(pPortCommonInfo->pPortHelper, sizeof(PORT_HELPER));	
}

NDIS_STATUS
N62CAllocatePortSpecificComponent(
	IN	PADAPTER		pAdapter
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	
	do
	{
		//
		// Allocate Ndis62Common
		//
		rtStatus = PlatformAllocateMemory(
				pAdapter, 
				&(pAdapter->pNdis62Common), 
				sizeof(RT_NDIS62_COMMON)
			);
		
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("N62CAllocatePortSpecificComponent(): failed to allocate memory for pNdis62Common.\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			break;
		}
		else 
		{	
			PlatformZeroMemory(pAdapter->pNdis62Common, sizeof(RT_NDIS62_COMMON));
		}
		
	} while(FALSE);
	
	return	ndisStatus;
}

VOID
N62CFreePortSpecificComponent(
	IN	PADAPTER		pAdapter
)
{
	PRT_NDIS62_COMMON pNdis62Common = pAdapter->pNdis62Common;
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("===>N62CFreePortSpecificComponent()\n"));

	PlatformFreeMemory(pAdapter->pNdis62Common, sizeof(RT_NDIS62_COMMON));
	pAdapter->pNdis62Common = NULL;
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("<===N62CFreePortSpecificComponent()\n"));
}


/*
 *2008/05/09 Add by Mars
 * 
 * To Register the AP and Ndis 6.2 Releated Attribute 
 *
 * @param	pAdapter is driver main strucutre.  
*/

NDIS_STATUS
N62CSet80211Attributes(
	IN	PADAPTER		pAdapter
	)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_NDIS62_COMMON  pNdis62Common = pAdapter->pNdis62Common;
	NDIS_MINIPORT_ADAPTER_ATTRIBUTES		Dot11Attributes;
	PRT_NDIS6_COMMON	pNdisCommon = pAdapter->pNdisCommon;
    	PDOT11_VWIFI_ATTRIBUTES pVWiFiAttribs = NULL;
	RT_STATUS		rtStatus;
	NIC_SUPPORTED_AUTH_CIPHER_PAIRS	SupportedAuthCipherAlgs = {0};
	u4Byte						BytesWritten, BytesNeeded;
	u1Byte                                    tempAuthType = 0;

        ULONG                       vwifiAttrSize = 0;
	PHAL_DATA_TYPE	    pHalData = GET_HAL_DATA(pAdapter);       
	BOOLEAN			    bStartVwifi = TRUE;
	PMGNT_INFO		    pMgntInfo = &(pAdapter->MgntInfo);	


	PlatformZeroMemory(&Dot11Attributes, sizeof(NDIS_MINIPORT_ADAPTER_ATTRIBUTES));

	//Modify by Maddest for support Win 7, 2008,04,21
	//For Platform Independent We will set Attribute in WIN 7 function
	Dot11Attributes.Native_802_11_Attributes.Header.Type=NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES;
		//NDIS 6
	Dot11Attributes.Native_802_11_Attributes.Header.Revision=NDIS_MINIPORT_ADAPTER_802_11_ATTRIBUTES_REVISION_2;
	Dot11Attributes.Native_802_11_Attributes.Header.Size=NDIS_SIZEOF_MINIPORT_ADAPTER_NATIVE_802_11_ATTRIBUTES_REVISION_2;	

	Dot11Attributes.Native_802_11_Attributes.OpModeCapability = pNdisCommon->dot11OperationModeCapability.uOpModeCapability;
	Dot11Attributes.Native_802_11_Attributes.NumOfTXBuffers = pNdisCommon->dot11OperationModeCapability.uNumOfTXBuffers;
	Dot11Attributes.Native_802_11_Attributes.NumOfRXBuffers = pNdisCommon->dot11OperationModeCapability.uNumOfRXBuffers;
	Dot11Attributes.Native_802_11_Attributes.MultiDomainCapabilityImplemented = pNdisCommon->dot11MultiDomainCapabilityImplemented;
	Dot11Attributes.Native_802_11_Attributes.NumSupportedPhys = pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries;//NATIVE_802_11_CURR_NUM_PHY_TYPES;//pNdisCommon->pDot11SupportedPhyTypes->uNumOfEntries;
	
	//
	// SupportedPhyAttributes attributes.
	//
	if (Dot11Attributes.Native_802_11_Attributes.NumSupportedPhys)
	{
		rtStatus = PlatformAllocateMemory(
			pAdapter, 
			&(Dot11Attributes.Native_802_11_Attributes.SupportedPhyAttributes),
			Dot11Attributes.Native_802_11_Attributes.NumSupportedPhys * sizeof(DOT11_PHY_ATTRIBUTES)
			);
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, 
				("N6usbSet80211Attributes(): failed to allocate memory for SupportedPhyAttributes\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			goto Exit;
		}

		PlatformZeroMemory(Dot11Attributes.Native_802_11_Attributes.SupportedPhyAttributes, 
				Dot11Attributes.Native_802_11_Attributes.NumSupportedPhys * sizeof(DOT11_PHY_ATTRIBUTES));
		N6SdioFill80211PhyAttributes(pAdapter, &(Dot11Attributes.Native_802_11_Attributes));
	}

    vwifiAttrSize = sizeof(DOT11_VWIFI_ATTRIBUTES) + 
                (NUM_SUPPORTED_VWIFI_COMBINATIONS) * sizeof(DOT11_VWIFI_COMBINATION);

	rtStatus = PlatformAllocateMemory(
		pAdapter,
		&(Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes),
		vwifiAttrSize
		);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N62CSet80211Attributes(): failed to allocate memory for VWiFiAttributes(HVL)\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

        //pVWiFiAttribs = (PDOT11_VWIFI_ATTRIBUTES)&(Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes);
        
	PlatformZeroMemory(Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes, vwifiAttrSize);
        
        
        N6_ASSIGN_OBJECT_HEADER(
            Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes->Header, 
            NDIS_OBJECT_TYPE_DEFAULT,
            DOT11_VWIFI_ATTRIBUTES_REVISION_1,
            sizeof(DOT11_VWIFI_ATTRIBUTES));

        Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes->uTotalNumOfEntries = NUM_SUPPORTED_VWIFI_COMBINATIONS;

        // support for Infra-Infra
        N6_ASSIGN_OBJECT_HEADER(
            Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes->Combinations[0].Header, 
            NDIS_OBJECT_TYPE_DEFAULT,
            DOT11_VWIFI_COMBINATION_REVISION_1,
            sizeof(DOT11_VWIFI_COMBINATION));

        Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes->Combinations[0].uNumInfrastructure = 1;
        
        // support for Infra-SoftAP
        // TODO: Sample Driver do this, but a little weird
        N6_ASSIGN_OBJECT_HEADER(
            Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes->Combinations[0].Header, 
            NDIS_OBJECT_TYPE_DEFAULT,
            DOT11_VWIFI_COMBINATION_REVISION_1,
            sizeof(DOT11_VWIFI_COMBINATION));

        Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes->Combinations[1].uNumInfrastructure = 1;
        Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes->Combinations[1].uNumSoftAP = 1;
        

	//
	// ExtSTAAttributes attributes.
	//
	rtStatus = PlatformAllocateMemory(
		pAdapter,
		&(Dot11Attributes.Native_802_11_Attributes.ExtSTAAttributes),
		sizeof(DOT11_EXTSTA_ATTRIBUTES)
		);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6usbSet80211Attributes(): failed to allocate memory for ExtSTAAttributes\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	PlatformZeroMemory(Dot11Attributes.Native_802_11_Attributes.ExtSTAAttributes,
						sizeof(DOT11_EXTSTA_ATTRIBUTES));
	
	//
	// Get the supported authentication and cipher algorithm.
	//
	PlatformZeroMemory(&SupportedAuthCipherAlgs, sizeof(NIC_SUPPORTED_AUTH_CIPHER_PAIRS));

	// Store current mode and switch to infrastructure mode.
	// <SC_TODO:>
	
	tempAuthType = pAdapter->MgntInfo.Regdot11networktype;
	pAdapter->MgntInfo.Regdot11networktype = RT_JOIN_NETWORKTYPE_INFRA;
	
	// Unicast & Infrastructure.
	ndisStatus = N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
				pAdapter,
				SupportedAuthCipherAlgs.pInfraUcastAuthCipherList,
				0,
				&BytesWritten,
				&BytesNeeded);
	rtStatus = PlatformAllocateMemory(
				pAdapter, 
				&(SupportedAuthCipherAlgs.pInfraUcastAuthCipherList), 
				BytesNeeded);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, 
			("N6usbSet80211Attributes(): no buffer for N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR under infra mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}
	
	PlatformZeroMemory(SupportedAuthCipherAlgs.pInfraUcastAuthCipherList, BytesNeeded);

	ndisStatus = N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
				pAdapter,
				SupportedAuthCipherAlgs.pInfraUcastAuthCipherList,
				BytesNeeded,
				&BytesWritten,
				&BytesNeeded);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6usbSet80211Attributes(): failed to query unicast auth cipher pairs under infra mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	// Multicast & Infrastructure.
	ndisStatus = N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
				pAdapter,
				SupportedAuthCipherAlgs.pInfraMcastAuthCipherList,
				0,
				&BytesWritten,
				&BytesNeeded);
	rtStatus = PlatformAllocateMemory(
				pAdapter,
				&(SupportedAuthCipherAlgs.pInfraMcastAuthCipherList),
				BytesNeeded);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6usbSet80211Attributes(): no buffer for N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR under infra mode.\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}
	PlatformZeroMemory(SupportedAuthCipherAlgs.pInfraMcastAuthCipherList, BytesNeeded);

	ndisStatus = N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
				pAdapter,
				SupportedAuthCipherAlgs.pInfraMcastAuthCipherList,
				BytesNeeded,
				&BytesWritten,
				&BytesNeeded);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6usbSet80211Attributes(): failed to query multicast auth cipher pairs under infra mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	// Switch to adhoc mode.
	// <SC_TODO:>
	pAdapter->MgntInfo.Regdot11networktype = RT_JOIN_NETWORKTYPE_ADHOC;
	// Unicast & Adhoc.
	ndisStatus = N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
				pAdapter,
				SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList,
				0,
				&BytesWritten,
				&BytesNeeded);
	rtStatus = PlatformAllocateMemory(
				pAdapter, 
				&(SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList), 
				BytesNeeded);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS, 
			("N6usbSet80211Attributes(): no buffer for N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR under adhoc mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}
	
	PlatformZeroMemory(SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList, BytesNeeded);

	ndisStatus = N6CQuery_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR(
				pAdapter,
				SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList,
				BytesNeeded,
				&BytesWritten,
				&BytesNeeded);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6usbSet80211Attributes(): failed to query unicast auth cipher pairs under adhoc mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	// Multicast & Adhoc.
	ndisStatus = N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
				pAdapter,
				SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList,
				0,
				&BytesWritten,
				&BytesNeeded);
	rtStatus = PlatformAllocateMemory(
				pAdapter,
				&(SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList),
				BytesNeeded);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6usbSet80211Attributes(): no buffer for N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR under adhoc mode.\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}
	PlatformZeroMemory(SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList, BytesNeeded);

	ndisStatus = N6CQuery_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR(
				pAdapter,
				SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList,
				BytesNeeded,
				&BytesWritten,
				&BytesNeeded);
	if (ndisStatus != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_SERIOUS,
			("N6usbSet80211Attributes(): failed to query multicast auth cipher pairs under adhoc mode\n"));
		ndisStatus = NDIS_STATUS_FAILURE;
		goto Exit;
	}

	// Switch back to origin mode.
	// <SC_TODO:>
	pAdapter->MgntInfo.Regdot11networktype = tempAuthType;

	N6Fill80211ExtStaAttributes(pAdapter, &(Dot11Attributes.Native_802_11_Attributes), &SupportedAuthCipherAlgs);

	//
	// Register Ndis 6.2 AP Mode 
	//
	//Set Extension AP Mode
	
	if(bStartVwifi)
	{
        Dot11Attributes.Native_802_11_Attributes.OpModeCapability |= DOT11_OPERATION_MODE_EXTENSIBLE_AP;

		PlatformAllocateMemory(pAdapter, 
			&(Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes), 
			sizeof(DOT11_EXTAP_ATTRIBUTES));

		if(Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes == NULL)
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Allocate Ndis 6.2 ExtAPAttributes Fail\n"));
			ndisStatus = NDIS_STATUS_FAILURE;
			goto Exit;
		}

		PlatformZeroMemory(Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes, sizeof(DOT11_EXTAP_ATTRIBUTES));

		N6_ASSIGN_OBJECT_HEADER(
	            Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->Header, 
	            NDIS_OBJECT_TYPE_DEFAULT,
	            DOT11_EXTAP_ATTRIBUTES_REVISION_1,
	            sizeof(DOT11_EXTAP_ATTRIBUTES));

	        Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uScanSSIDListSize = AP_SCAN_SSID_LIST_MAX_SIZE;
	        Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uPrivacyExemptionListSize = NATIVE_802_11_MAX_PRIVACY_EXEMPTION;
		 // TODO: Get the real Size Some day
	        Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uDefaultKeyTableSize = DOT11_MAX_NUM_DEFAULT_KEY;//VNic11DefaultKeyTableSize(vnic);
	        Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uWEPKeyValueMaxLength = ( 104 / 8);//VNic11WEP104Implemented(vnic) ? 
	                                                // 104 / 8 : (VNic11WEP40Implemented(vnic) ? 40 / 8 : 0);

	        Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uDesiredSSIDListSize = AP_DESIRED_SSID_LIST_MAX_SIZE;
	        Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->bStrictlyOrderedServiceClassImplemented = AP_STRICTLY_ORDERED_SERVICE_CLASS_IMPLEMENTED;
	        Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uAssociationTableSize = AP_DEFAULT_ALLOWED_ASSOCIATION_COUNT;

	        //
	        // 11d stuff.
	        //
	        Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uNumSupportedCountryOrRegionStrings = 0;
	        Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->pSupportedCountryOrRegionStrings = NULL;
			
		// In Ndis 6.0 We use Ad hoc to simulize AP mode So the Ad hoc Security will be the security we support
		// TODO: Win 7 , We should support more security in here for Win 7 V3 AP Mode
		if(pAdapter->bInHctTest)
		{
			// In Ndis 6.0 We use Ad hoc to simulize AP mode So the Ad hoc Security will be the security we support
			Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uInfraNumSupportedUcastAlgoPairs = 2;
			Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->pInfraSupportedUcastAlgoPairs =
				SupportedAuthCipherAlgs.pInfraUcastAuthCipherList->AuthCipherPairs;

			Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uInfraNumSupportedMcastAlgoPairs = 2;
			Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->pInfraSupportedMcastAlgoPairs =
				SupportedAuthCipherAlgs.pInfraMcastAuthCipherList->AuthCipherPairs;
		}
		else
		{
			// In Ndis 6.0 We use Ad hoc to simulize AP mode So the Ad hoc Security will be the security we support
			Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uInfraNumSupportedUcastAlgoPairs = 
				SupportedAuthCipherAlgs.pInfraUcastAuthCipherList	->uNumOfEntries;
			Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->pInfraSupportedUcastAlgoPairs =
				SupportedAuthCipherAlgs.pInfraUcastAuthCipherList->AuthCipherPairs;

			Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->uInfraNumSupportedMcastAlgoPairs = 
				SupportedAuthCipherAlgs.pInfraMcastAuthCipherList->uNumOfEntries;
			Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes->pInfraSupportedMcastAlgoPairs =
				SupportedAuthCipherAlgs.pInfraMcastAuthCipherList->AuthCipherPairs;
		}
		
	}
	//
	// Ok, now egister the attributes.
	//
	ndisStatus = NdisMSetMiniportAttributes(pAdapter->pNdisCommon->hNdisAdapter,
								(PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&Dot11Attributes
								);	

Exit:
	// Memory cleanup.

	if (SupportedAuthCipherAlgs.pInfraUcastAuthCipherList != NULL)
	{
		PlatformFreeMemory(
			SupportedAuthCipherAlgs.pInfraUcastAuthCipherList,
			SupportedAuthCipherAlgs.pInfraUcastAuthCipherList->uNumOfEntries * sizeof(DOT11_AUTH_CIPHER_PAIR) + 
				FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs));
	}

	if (SupportedAuthCipherAlgs.pInfraMcastAuthCipherList != NULL)
	{
		PlatformFreeMemory(
			SupportedAuthCipherAlgs.pInfraMcastAuthCipherList,
			SupportedAuthCipherAlgs.pInfraMcastAuthCipherList->uNumOfEntries * sizeof(DOT11_AUTH_CIPHER_PAIR) + 
				FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs));
	}	

	if (SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList != NULL)
	{
		PlatformFreeMemory(
			SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList,
			SupportedAuthCipherAlgs.pAdhocUcastAuthCipherList->uNumOfEntries * sizeof(DOT11_AUTH_CIPHER_PAIR) + 
				FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs));
	}

	if (SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList != NULL)
	{
		PlatformFreeMemory(
			SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList,
			SupportedAuthCipherAlgs.pAdhocMcastAuthCipherList->uNumOfEntries * sizeof(DOT11_AUTH_CIPHER_PAIR) + 
				FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs));
	}

	if (Dot11Attributes.Native_802_11_Attributes.ExtSTAAttributes != NULL)
	{
		PlatformFreeMemory(Dot11Attributes.Native_802_11_Attributes.ExtSTAAttributes,
						sizeof(DOT11_EXTSTA_ATTRIBUTES));
	}

	if (Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes != NULL)
	{
		PlatformFreeMemory(Dot11Attributes.Native_802_11_Attributes.ExtAPAttributes,
						sizeof(DOT11_EXTAP_ATTRIBUTES));
	}

	if (Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes != NULL)
	{
		PlatformFreeMemory(Dot11Attributes.Native_802_11_Attributes.VWiFiAttributes,
						vwifiAttrSize);
	}

	if (Dot11Attributes.Native_802_11_Attributes.NumSupportedPhys && Dot11Attributes.Native_802_11_Attributes.SupportedPhyAttributes != NULL)
	{
		PlatformFreeMemory(Dot11Attributes.Native_802_11_Attributes.SupportedPhyAttributes,
					Dot11Attributes.Native_802_11_Attributes.NumSupportedPhys * sizeof(DOT11_PHY_ATTRIBUTES));
	}

	return ndisStatus;
}

VOID
N62CInitVariable(
	IN	PADAPTER		pAdapter
	)
{
	PRT_NDIS62_COMMON pNdis62Common = pAdapter->pNdis62Common;
	PRT_NDIS6_COMMON pNdisCommon	= pAdapter->pNdisCommon;
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PPORT_HELPER pPortHelper =pDefaultAdapter->pPortCommonInfo->pPortHelper;
	PMGNT_INFO pMgntInfo = &(pAdapter->MgntInfo);
		
	//===================Default Adapter variable initialize===================
	pNdisCommon->dot11CurrentOperationMode.uCurrentOpMode=DOT11_OPERATION_MODE_UNKNOWN;
	pNdis62Common->PortNumber=0;
	pNdis62Common->PortType=EXTSTA_PORT;
	pNdis62Common->CurrentOpState=INIT_STATE;
	pNdis62Common->bWPSEnable=FALSE;

	//===================initialize Helper variable========================
	pPortHelper->bCreateMac=FALSE;
	pPortHelper->bDeleteMac=FALSE;

	//===================initialize VWiFi AP variable========================
	GetDefaultMgntInfo(pAdapter)->ApType = RT_AP_TYPE_NONE;
	
}

PADAPTER
GetAdapterByPortNum(
	PADAPTER	Adapter, 
	u1Byte PortNum
	)
{
	PADAPTER	pAdapter = GetDefaultAdapter((Adapter));

	while(pAdapter!=NULL) 	
	{
		if(pAdapter->pNdis62Common->PortNumber == PortNum)
			break;
		pAdapter = GetNextExtAdapter(pAdapter);
	}
	
	if(pAdapter == NULL)
		pAdapter = GetDefaultAdapter(Adapter);
	
	return pAdapter;
}


