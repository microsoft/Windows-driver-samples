#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "DriverInterface.tmh"
#endif

RT_STATUS
NicIFAssociateNIC(
	PADAPTER		Adapter,
	u2Byte			HardwareType
	)
{
	Adapter->HardwareType=HardwareType;

	//
	// We need to enable common feature support selection after HW type recognize.
	// Otherwise. the common module relative resource will not be handled correctly.
	//
	// Must be called before MgntInitializeAllTimer()/MgntInitializeAllWorkItem() Because we
	// might enable the feature support and use some timer/workitem/os resource ....
	//
	// If you need to re-initialize when every connect/disconnect. You need to move all resource
	// init in FEATURE_Init() and be called in InitializeMgntVariables(). Take CCX & TDLS for example.
	//

	if (HalAssociateNic(Adapter, TRUE) == RT_STATUS_FAILURE)
		return	RT_STATUS_FAILURE;

	// We will perform following flows after HW type is set for SDIO interface.
	if( HardwareType == HARDWARE_TYPE_MAX )
		return	RT_STATUS_SUCCESS;

	return	RT_STATUS_SUCCESS;
}

VOID
NicIFDisassociateNIC(
	PADAPTER		Adapter
)	
{
	HalDisassociateNic(Adapter);
}


// 
// 2010/07/06 MH Seperate the HAL mempro and correspond resource allocaton.
// All the HAL/common resource and variable shoule be allocated after the HAL memory
// pointer is allocated.
//
VOID
NicIFInitResource(
	PADAPTER		Adapter
	)
{
	//2 Put all variable initialize code here

	// Initialize the MacIdCommon -----------------------------------
	MacIdInitializeCommonContext(Adapter);
	// ----------------------------------------------------------

	// Initialize the ActionTimerCommon ------------------------------
	ActionTimerInitializeCommonContext(Adapter);
	// ----------------------------------------------------------

	Hal_InitVars(Adapter);
	
	Hal_InitCamEntry(Adapter);
	
	DFS_Init(Adapter);

	HAL_DiffTXDummyLen(Adapter);

	HAL_DiffTXRXLen(Adapter);	
	
	Adapter->HalFunc.InitializeVariablesHandler(Adapter);

	// 2015/03/10 Hana, Init Tx Feedback, 
	// the Tx Feedback capability of each IC depends on the HAL define variable HAL_DEF_TX_FEEDBACK_SUPPORT
	TxFeedbackInitialize(Adapter);

	ADCSmp_Init(Adapter);
	InitializeTxVariables(Adapter);
	InitializeRxVariables(Adapter);
	InitializeMgntVariables(Adapter);
	TX_InitializeVariables(Adapter);
	FW_InitializeVariables(Adapter);
	// 2011/07/15 Sinda Init Tx shortcut

}	// NicIFInitMgntRsc


VOID
NicIFDeInitResource(
	PADAPTER		Adapter
)	
{
	ADCSmp_DeInit(Adapter);

	Adapter->HalFunc.DeInitializeVariablesHandler(Adapter);

	//
	// Free all timer and workitem before HalDisassociateNic, because some workitem or timer under
	// Mgnt Variables (such as ips workitem) may access Hal variables.
	// By Bruce, 2007-10-17.
	//
	DeInitializeMgntVariables(Adapter);

	DeInitializeRxVariables(Adapter);
	
	// DeInitialize the ActionTimerCommon ----------------------------
	// init ActionTimer list.
	ActionTimerDeInitializeCommonContext(Adapter);
	// ----------------------------------------------------------

	// DeInitialize the MacIdCommon --------------------------------
	MacIdDeInitializeCommonContext(Adapter);
	// ----------------------------------------------------------

}	//NicIFDeInitMgntRsc

VOID
NicIFReadAdapterInfo(
	PADAPTER		Adapter
	)
{
	// <20130227, Kordan> Initialize the HW-independant data structure for EFUSE related operation. 
	HAL_CmnInitPGData(Adapter);
		
	// Read EEPROM size before call any EEPROM function
	Adapter->EepromAddressSize=Adapter->HalFunc.GetEEPROMSizeHandler(Adapter);
	// 2011/03/09 MH Add description. This is used to define different EEPROm offset of different
	// IC. If new HW support different offset, we need add the value in HAL structure.
	//Adapter->HalFunc.ReadEfuseOffsetHandler(Adapter);
	// 2011/03/09 MH Add description. According to predefined offset to capture EFUSE content.
	Adapter->HalFunc.ReadAdapterInfoHandler(Adapter);

	// 2013/09/12 MH When registry channel plan is upadted, we need to update regulatory again, 
	// otherwise, it will use WW table as default.	
	PHY_MapChnlPlanRegulatory(Adapter);
	
}

RT_STATUS
NicIFAllocateMemory(
	PADAPTER		Adapter
	)
{
	RT_STATUS	status = RT_STATUS_FAILURE;
	PRT_SDIO_DEVICE	sdiodevice = GET_RT_SDIO_DEVICE(Adapter);
	int nRxQueueNum = PLATFORM_GET_RT_NUM_SDIO_RX_QUEUE(sdiodevice); // RX FIFO0
	int nTxQueueNum = PLATFORM_GET_RT_NUM_SDIO_TX_QUEUE(sdiodevice); // TX_HIQ, TX_MIQ and TX_LOQ

	if(RT_DRIVER_STOP(Adapter))
		return status; 
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("NicIFAllocateMemory() ==>\n"));

	do
	{
		// TODO: Allocate all other memory blocks including shared memory
		// TODO: Init send/receive engine(Software only)
		RT_TRACE(COMP_INIT, DBG_LOUD, ("Number of Tx Queues %d, Number of Rx Queues %d\n", nTxQueueNum, nRxQueueNum));
		if(HalSdioAllocResource(Adapter, nTxQueueNum, nRxQueueNum))
		{
			status = RT_STATUS_SUCCESS;
		}
		else
		{
			status = RT_STATUS_FAILURE;
			break;
		}
		RT_TRACE_F(COMP_INIT, DBG_TRACE, ("before PrepareRFDs\n"));						

		RT_TRACE(COMP_INIT, DBG_LOUD, ("PrepareRFDs() ==>\n"));
		status=PrepareRFDs(Adapter);
		RT_TRACE(COMP_INIT, DBG_LOUD, ("PrepareRFDs() <==\n"));
		if(status!=RT_STATUS_SUCCESS)
		{			
			RT_TRACE_F(COMP_INIT, DBG_LOUD, ("PrepareRFDs fail\n"));
			FreeRFDs(Adapter, FALSE);
			break;
		}

		RT_TRACE_F(COMP_INIT, DBG_TRACE, ("before PrepareTCBs\n"));								

		status=PrepareTCBs(Adapter);
		if(status!=RT_STATUS_SUCCESS)
		{
			RT_TRACE_F(COMP_INIT, DBG_LOUD, ("PrepareTCBs fail\n"));	
			FreeTCBs(Adapter, FALSE);
			break;
		}

		RT_TRACE_F(COMP_INIT, DBG_TRACE, ("before MgntAllocateBeaconBuf\n"));					

		status = MgntAllocateBeaconBuf( Adapter );
		if(status!=RT_STATUS_SUCCESS)
		{
			RT_TRACE_F(COMP_INIT, DBG_LOUD, ("MgntAllocateBeaconBuf fail\n"));				
			MgntFreeBeaconBuf(Adapter);
			break;
		}
		RT_TRACE_F(COMP_INIT, DBG_TRACE, ("before FW_AllocateMemory\n"));						

		status = FW_AllocateMemory(Adapter);
		if(status!=RT_STATUS_SUCCESS)
		{
			RT_TRACE_F(COMP_INIT, DBG_LOUD, ("FW_AllocateMemory fail\n"));						
			FW_FreeMemory(Adapter);
			break;		
		}
	}while(FALSE);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("NicIFAllocateMemory() <==\n"));
	return status;
}

VOID
NicIFFreeMemory(
	PADAPTER		Adapter
	)
{
	PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	// Return all packets queued in AP mode. 2005.06.21, by rcnjko.
	AP_PS_ReturnAllQueuedPackets(Adapter, FALSE);

	MgntFreeBeaconBuf( Adapter	);

	// Free Qos related resources.
	QosDeinitializeSTA(Adapter);
	
	//2 Note: TCB must be freed before RFD
	FreeTCBs(Adapter, FALSE);
	FreeRFDs(Adapter, FALSE);

	WAPI_SecFuncHandler(WAPI_FREEALLSTAINFO,Adapter, WAPI_END);

	FW_FreeMemory(Adapter);

	HalSdioFreeResource(Adapter);

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
	PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
}

VOID
NicIFResetMemory(
	PADAPTER		Adapter
)
{
	PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	// <3> Reset RFDs
	FreeRFDs( Adapter, TRUE);

	// <4> Reset TCBs
	FreeTCBs( Adapter, TRUE);

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
	PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

	// <5> Reset all state machine
	{
		PADAPTER pLoopAdapter = GetDefaultAdapter(Adapter);

		while(pLoopAdapter)
		{
			pLoopAdapter->MgntInfo.bScanInProgress = FALSE;
			pLoopAdapter->MgntInfo.bHaltInProgress = FALSE;
			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}

	Adapter->MgntInfo.bDualModeScanStep=0;
	RT_ResetSwChnlProgress(Adapter);
}

#define	MAX_INIT_RETRY_CNT	10

RT_STATUS
driverIFInitializeAdapter(
	PADAPTER		Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;

	MultiPortSetAllPortsHWReadyStatus(Adapter, FALSE);

	//2 Put buffer into Rx Desc
	PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	PrepareAllRxDescBuffer(Adapter);
	PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

	rtStatus = Adapter->HalFunc.InitializeAdapterHandler(Adapter, pMgntInfo->dot11CurrentChannelNumber);

	MultiPortSetAllPortsHWReadyStatus(Adapter, TRUE);

	PlatformSetCheckForHangTimer(Adapter);

	return 	rtStatus;
}

RT_STATUS
NicIFInitializeAdapter(
	PADAPTER		Adapter
	)
{
	u1Byte		init_retry = 0;
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);

	//2 Initialize Hardware
	// 20090402 Joseph: Retry initialize process when failed.
	while(init_retry < MAX_INIT_RETRY_CNT)
	{

		RT_TRACE(COMP_INIT, DBG_LOUD, ("NicIFInitializeAdapter() init_retry(%#x)\n", init_retry));

		if(OS_SUPPORT_WDI(Adapter))
		{
		if((ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT)))
		{
			Adapter->bInitializeInProgress=TRUE;
			rtStatus = driverIFInitializeAdapter(Adapter);
			Adapter->bInitializeInProgress=FALSE;
		}
		else
		{
			rtStatus = driverIFInitializeAdapter(Adapter);
		}
		}
		// 2009/10/13 MH For Clevo X8100 model 3D mark issue. We can not delay
		// Adapter initialize operation more than 1060ms. Otherwise, the Nvidia SLI
		// may fail to judge the OS resource when system is rebooted. We still not
		// investigate the real reason for the conflict between nvida SLI and rtl8191se
		// initialize sequence. But we decide to disable all velocity modification for Clevo
		// package now.
		//
		else if((ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT))
			&& !pMgntInfo->bRegClevoDriver && pMgntInfo->bRegVelocity)
		{
			MultiPortSetAllPortsHWReadyStatus(Adapter, FALSE);

			// 2010/12/17 MH After Code base Lable 977, we need to move the flag here, otherwise, 
			// HCT card init sequence will be BSOD when transfer adapter in InitializeAdapterHandler. 
			Adapter->bInitializeInProgress=TRUE;

			if(!ACTING_AS_AP(Adapter))
			{
				RT_TRACE(COMP_INIT, DBG_LOUD, ("InitializeAdapterHandler Delay\n"));			
			}
			else
			{
				//2 Put buffer into Rx Desc
				PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
				PrepareAllRxDescBuffer(Adapter);
				PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

				rtStatus = Adapter->HalFunc.InitializeAdapterHandler(Adapter, pMgntInfo->dot11CurrentChannelNumber);
				#if (VISTA_RX_BATCH_INDICATE)
					InitBatchIndication(Adapter);
				#endif
				Adapter->bInitializeInProgress=FALSE;

				MultiPortSetAllPortsHWReadyStatus(Adapter, TRUE);

				#if (UNDER_LOW_PWR_SOC_PLATFORM && NDIS_SUPPORT_NDIS630)
					PlatformSetCheckForHangTimer(Adapter);
				#else
					Adapter->MgntInfo.bSetWatchDogTimerByDriver = FALSE;
				#endif
			}
		}
		else
		{
			//2 Put buffer into Rx Desc
			PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
			PrepareAllRxDescBuffer(Adapter);
			PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

			rtStatus = Adapter->HalFunc.InitializeAdapterHandler(Adapter, pMgntInfo->dot11CurrentChannelNumber);
			#if (VISTA_RX_BATCH_INDICATE)
				InitBatchIndication(Adapter);
			#endif		
			MultiPortSetAllPortsHWReadyStatus(Adapter, TRUE);

			#if (UNDER_LOW_PWR_SOC_PLATFORM && NDIS_SUPPORT_NDIS630)
				PlatformSetCheckForHangTimer(Adapter);
			#else
				Adapter->MgntInfo.bSetWatchDogTimerByDriver = FALSE;
			#endif
		}		


		if(rtStatus == RT_STATUS_SUCCESS)
		{
			break;
		}
		else
			init_retry++;
	}

	// Get current time as driver up time since the device has been started in InitializeAdapterHandler().
	Adapter->DriverUpTime = PlatformGetCurrentTime();

	// UPdate AMSDU forced parameters.
	AMSDU_UpdateForcedValueByReg(Adapter);

	return rtStatus;
}

VOID
NicIFHandleInterrupt(
	PADAPTER		Adapter
	)
{
}

/**
*	This function is called by Checkforhang to check whether we should ask OS to reset driver
*	
*	\param pAdapter	The adapter context for this miniport
*
*	Note:NIC with USB interface sholud not call this function because we cannot scan descriptor
*	to judge whether there is tx stuck.
*	Note: This function may be required to be rewrite for Vista OS.
*	<<<Assumption: Tx spinlock has been acquired >>>
*	
*	8185 and 8185b does not implement this function. This is added by Emily at 2006.11.24
*/
RESET_TYPE
NicIFCheckResetOrNot(
	PADAPTER		Adapter
	)
{
	RESET_TYPE	TxResetType = RESET_TYPE_NORESET;
	RESET_TYPE	RxResetType = RESET_TYPE_NORESET;

	if(Adapter->HalFunc.TxCheckStuckHandler(Adapter))
		TxResetType = RESET_TYPE_SILENT;

	if(Adapter->HalFunc.RxCheckStuckHandler(Adapter))
		RxResetType = RESET_TYPE_SILENT;

	if(TxResetType==RESET_TYPE_SILENT || RxResetType==RESET_TYPE_SILENT)
		return RESET_TYPE_SILENT;
	else
		return RESET_TYPE_NORESET;
}


VOID
NicIFCoalesceReceivedPacketAndFreeUnusedRFD(
	PADAPTER		Adapter,
	PRT_RFD			pRfd	
	)
{
	PRT_RFD		frag;

	if(pRfd->nTotalFrag<2)
		return;

	//2 Setup RFD list from 2nd RFD
	frag=pRfd->NextRfd;
	frag->nTotalFrag=pRfd->nTotalFrag-1;

	while(frag)
	{
		//
		// 061122, rcnjko: prevent malicious attack.
		//
		if(pRfd->FragLength + frag->FragLength > Adapter->MAX_RECEIVE_BUFFER_SIZE )
		{
			break;
		}

		//2 Copy all data to first fragment
		PlatformMoveMemory(
			pRfd->Buffer.VirtualAddress + pRfd->FragOffset + pRfd->FragLength,
			frag->Buffer.VirtualAddress + frag->FragOffset,
			frag->FragLength);
		
		pRfd->FragLength += frag->FragLength;
	
		frag=frag->NextRfd;
	}

	//2 Make sure packet size is equal to sum of all fragment size
	RT_ASSERT(pRfd->PacketLength==pRfd->FragLength, ("Coalesce fail: Packet length not equal to total fragment length !!\n"));

	//2 Free RFD list from 2nd RFD
	ReturnRFDList(Adapter, pRfd->NextRfd);

	//2 Set total fragment number first fragment to 1
	pRfd->NextRfd=NULL;
	pRfd->nTotalFrag=1;

	//2 Prepare RFD since some RFDs become available
	PrepareAllRxDescBuffer(Adapter);
}

VOID
NicIFReturnPacket(
	PADAPTER		Adapter,
	PRT_RFD			pRfd	
	)
{	

#if WLAN_ETW_SUPPORT
	//
	// <Roger_Notes> No activity needs to be associated with this event, the ActivityId is optional and can be NULL.
	// 2014.01.14.
	//	
	PlatformAcquireSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
	EventWriteRxReturnToDriver(
		NULL, //Without associated with the event
		pRfd->RfdFrameUniqueueID, // FrameUniqueueID
		0,	// QueueLength
		(u2Byte)RT_GET_RCV_REF(Adapter),// RxBacklog
		0, // CustomData1
		0, // CustomData2
		0);// CustomData3	

	PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
#endif

	ReturnRFDList(Adapter, pRfd);
	PrepareAllRxDescBuffer(Adapter);
}

VOID
NicIFCancelAllTimer(
	PADAPTER		Adapter
	)
{
	//Redundant operation, MgntCancelAllTimer() would be called in RTUsbFreeAll(). Isaiah 2007.3.7
	//MgntCancelAllTimer(Adapter);

	Adapter->HalFunc.CancelAllTimerHandler(Adapter);
	Adapter->HalFunc.ReleaseAllTimerHandler(Adapter); // For MacOS compatible
}

RT_STATUS
NicIFDisableNIC(
	PADAPTER		Adapter
)
{
	RT_STATUS	status = RT_STATUS_SUCCESS;

	// <1> Disable Interrupt
	NicIFDisableInterrupt(Adapter);

	// <2> Stop all timer
	MgntCancelAllTimer(Adapter);

	// <3> Disable Adapter
	Adapter->HalFunc.HaltAdapterHandler(Adapter, TRUE);
	return status;
}

RT_STATUS
NicIFEnableNIC(
	PADAPTER		Adapter
)
{
	RT_STATUS	status = RT_STATUS_SUCCESS;

	// <1> Reset memory: descriptor, buffer,..
	NicIFResetMemory(Adapter);

	// <2> Enable Adapter
	status = NicIFInitializeAdapter(Adapter);
	if(status != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD,("InitializeAdapter Fail\n"));
		return status;
	}
	RT_CLEAR_PS_LEVEL(Adapter, RT_RF_OFF_LEVL_HALT_NIC);
	// <3> Enable Interrupt
	NicIFEnableInterrupt(Adapter);

	return status;
}

RT_STATUS
NicIFResetNIC(
	PADAPTER		Adapter
)
{
	RT_STATUS	status = RT_STATUS_SUCCESS;
       PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;

	PlatformAcquireSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
	if(pMgntInfo->RFChangeInProgress)
	{
		PlatformReleaseSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
		return RT_STATUS_SUCCESS;
	}
	pMgntInfo->RFChangeInProgress = TRUE;
	PlatformReleaseSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
	
	status = NicIFDisableNIC( Adapter );
	if( status != RT_STATUS_SUCCESS ){
		goto END;
	}
	RT_SET_PS_LEVEL(Adapter, RT_RF_OFF_LEVL_HALT_NIC);

		if(Adapter->MgntInfo.bSetWatchDogTimerByDriver)
			CancelWatchDogTimer(Adapter);

	status = NicIFEnableNIC( Adapter );
	if( status != RT_STATUS_SUCCESS ){
		goto END;
	}
	RT_CLEAR_PS_LEVEL(Adapter, RT_RF_OFF_LEVL_HALT_NIC);

END:
	PlatformAcquireSpinLock(Adapter, RT_RF_STATE_SPINLOCK);
	pMgntInfo->RFChangeInProgress = FALSE;
	PlatformReleaseSpinLock(Adapter, RT_RF_STATE_SPINLOCK);

	return status;
}



