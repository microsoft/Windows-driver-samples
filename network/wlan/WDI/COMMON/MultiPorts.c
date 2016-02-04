#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "MultiPorts.tmh"
#endif

//============================================================================
// All Type Port Supported 
//============================================================================


VOID
MultiPortSetAllPortsHWReadyStatus(
	PADAPTER pAdapter,
	BOOLEAN	 bReady
)
{
	PADAPTER pLoopAdapter = GetDefaultAdapter(pAdapter);
	
	RT_TRACE(COMP_INIT, DBG_LOUD, ("MultiPortSetAllPortsHWReadyStatus(), set pAdapter->bHWInitReady=%d\n", (u4Byte)bReady));
	while(pLoopAdapter != NULL)
	{
		pLoopAdapter->bHWInitReady = bReady;
		pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
	}
}


#if (MULTIPORT_SUPPORT == 1)

//============================================================================
// Multiple Port Supported 
//============================================================================

// Please do not use this function outside of this module
static PMULTIPORT_COMMON_CONTEXT
MultiPortGetCommonContext(
	PADAPTER pAdapter
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);

	RT_ASSERT(
			sizeof(pDefaultAdapter->pPortCommonInfo->MultiPortCommon) == sizeof(MULTIPORT_COMMON_CONTEXT),
			("MultiPort Common Context Memory Allocation Size Mismatch !")
		);

	return ((PMULTIPORT_COMMON_CONTEXT) pDefaultAdapter->pPortCommonInfo->MultiPortCommon);
}

// Please do not use this function outside of this module
static PMULTIPORT_PORT_CONTEXT
MultiPortGetPortContext(
	PADAPTER pAdapter
)
{
	if(pAdapter == NULL) return NULL;
	
	RT_ASSERT(
			sizeof(pAdapter->MultiPort) == sizeof(MULTIPORT_PORT_CONTEXT),
			("MultiPort Port Context Memory Allocation Size Mismatch !")
		);

	return ((PMULTIPORT_PORT_CONTEXT) pAdapter->MultiPort);
}

static PADAPTER
GetAdapterByListEntry(
	PRT_LIST_ENTRY	pListEntry
)
{
	
	u4Byte offset1 = FIELD_OFFSET(ADAPTER, MultiPort);
	u4Byte offset2 = FIELD_OFFSET(MULTIPORT_PORT_CONTEXT, MultiList);

	//====================================
	// NOTE: You can also find pAdapter by pMultiPort->pAdapter
	//====================================
#if 0
	PADAPTER pAdapter=(PADAPTER)(((pu1Byte) pListEntry) - offset1 - offset2);
	PMULTIPORT_PORT_CONTEXT pMultiPort=(PMULTIPORT_PORT_CONTEXT)pListEntry;

	if(pAdapter != pMultiPort->pAdapter)
		DbgPrint("GetAdapterByListEntry() Adapter mismatch!!, pAdapter=0x%x, pMultiPort->pAdapter=0x%x\n", pAdapter, pMultiPort->pAdapter);
	return pAdapter;
#endif

	return (PADAPTER)(((pu1Byte) pListEntry) - offset1 - offset2);
}

PADAPTER
GetNextExtAdapter(
	PADAPTER	pAdapter
)	
{
	// Active Extension Adapter Only ----------------------------------------------
	PMULTIPORT_PORT_CONTEXT pMultiPort = MultiPortGetPortContext(pAdapter);
	PRT_LIST_ENTRY	pListEntry = RTNextEntryList(&pMultiPort->MultiList);
	PADAPTER	pExtAdapter = NULL;

	if( pListEntry == NULL )
		return NULL;
	else
		pExtAdapter = GetAdapterByListEntry(pListEntry);

	while(pExtAdapter != NULL)
	{
		pMultiPort = MultiPortGetPortContext(pExtAdapter);
		
		if (IsDefaultAdapter(pExtAdapter))
			break;

		if(pMultiPort->bActiveAdapter == TRUE) 
		{
			return pExtAdapter;
		}

		pListEntry = RTNextEntryList(&pMultiPort->MultiList);
		pExtAdapter = GetAdapterByListEntry(pListEntry);
	}
	
	return NULL;
}
	

PADAPTER
GetFirstAPAdapter(
	PADAPTER	pAdapter
)	
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pAPAdapter = pDefaultAdapter;

	while(pAPAdapter != NULL)
	{
		if(ACTING_AS_AP(pAPAdapter))
			break;
		pAPAdapter = GetNextExtAdapter(pAPAdapter);			
	}

	return pAPAdapter;
}

PADAPTER
GetFirstDevicePort(
	PADAPTER	pAdapter
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pDevicePort = NULL;
	
	pDevicePort = pDefaultAdapter;
	
	while(pDevicePort != NULL)
	{

	#if P2P_SUPPORT == 1
		if(P2P_ENABLED(GET_P2P_INFO(pDevicePort)))
		{
			if(P2P_ADAPTER_OS_SUPPORT_P2P(pDevicePort) && GET_P2P_INFO(pDevicePort)->Role == P2P_DEVICE)
				break;
		}
	#endif
	
		pDevicePort = GetNextExtAdapter(pDevicePort);
	}

	return pDevicePort;
}

PADAPTER
GetFirstGOPort(
	PADAPTER	pAdapter
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pGOPort = NULL;
	
	pGOPort = pDefaultAdapter;
	
	while(pGOPort != NULL)
	{

	#if P2P_SUPPORT == 1
		if(P2P_ENABLED(GET_P2P_INFO(pGOPort)))
		{
			if(GET_P2P_INFO(pGOPort)->Role == P2P_GO)
			{
				if(P2P_ADAPTER_RTK_SUPPORT_P2P(pGOPort))
				{
					pGOPort = GetFirstAPAdapter(pDefaultAdapter);
				}
				break;
			}
		}
	#endif
	
		pGOPort = GetNextExtAdapter(pGOPort);
	}

	return pGOPort;
}

PADAPTER
GetFirstClientPort(
	PADAPTER	pAdapter
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pClientPort = NULL;
	
	pClientPort = pDefaultAdapter;
	
	while(pClientPort != NULL)
	{

	#if P2P_SUPPORT == 1
		if(P2P_ENABLED(GET_P2P_INFO(pClientPort)))
		{
			if(GET_P2P_INFO(pClientPort)->Role == P2P_CLIENT)
				break;
		}
	#endif
	
		pClientPort = GetNextExtAdapter(pClientPort);
	}

	return pClientPort;
}

PADAPTER
MultiPortGetIdleExtAdapter(
	PADAPTER pAdapter
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PMULTIPORT_PORT_CONTEXT pMultiPort = MultiPortGetPortContext(pDefaultAdapter);
	PRT_LIST_ENTRY	pListEntry = RTNextEntryList(&pMultiPort->MultiList);
	PADAPTER	pExtAdapter = GetAdapterByListEntry(pListEntry);

	while(!IsDefaultAdapter(pExtAdapter))
	{
		pMultiPort = MultiPortGetPortContext(pExtAdapter);
			
		if(pMultiPort->bActiveAdapter == FALSE)
		{
			return pExtAdapter;
		}

		pListEntry = RTNextEntryList(&pMultiPort->MultiList);
		pExtAdapter = GetAdapterByListEntry(pListEntry);
	}

	return NULL;
}

PADAPTER
MultiPortRemoveExtAdapter(
	PADAPTER pAdapter
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PMULTIPORT_PORT_CONTEXT pMultiPortDefault = MultiPortGetPortContext(pDefaultAdapter);
	PADAPTER pExtAdapter = NULL;
	PRT_LIST_ENTRY pListEntry = NULL;
	
	if(!RTIsListEmpty(&pMultiPortDefault->MultiList))
	{
		pListEntry = RTRemoveHeadList(&pMultiPortDefault->MultiList);
		pExtAdapter = GetAdapterByListEntry(pListEntry);
	}
	
	return pExtAdapter;
}



BOOLEAN
MultiPortInsertIntoTargetAdapterList(
	IN PADAPTER			TargetAdapter, 
	OUT PADAPTER		TargetList[],
	OUT pu4Byte 			puCurrentTarget,
	IN u4Byte			uMaxTarget
)
{
	u4Byte i = 0;
	BOOLEAN  bExist = FALSE;
	
	for(i = 0; i < *puCurrentTarget; i++)
	{
		if(TargetList[i] == TargetAdapter)
		{
			bExist = TRUE;
			break;
		}
	}

	// New item found
	if(bExist == FALSE)
	{
		if(*puCurrentTarget >= uMaxTarget)
		{
			RT_TRACE(COMP_INIT, DBG_LOUD, ("%s: Error: TargetList is Full!\n", __FUNCTION__));
			return FALSE;
		}
		else
		{
			TargetList[*puCurrentTarget] = TargetAdapter;
			*puCurrentTarget = *puCurrentTarget + 1;
		}
	}

	// TargetAdapter will be in TargetList[]
	
	return TRUE;
}

u4Byte
MultiPortGetTargetAdapterList(
	PADAPTER		pAdapter,
	PRT_RFD			pRfd,
	OCTET_STRING	osFrame,
	PADAPTER		TargetList[],
	u4Byte			uMaxTarget
)
{
	// Modified based on N62CSelectReceiveAdapter()
	u4Byte		uTargetAdapter = 0;
	PADAPTER	pExtAdapter = NULL;
	PMGNT_INFO	pExtMgntInfo = NULL;
	BOOLEAN 	bMatch = FALSE;

	if(	pRfd->Status.PacketReportType == TX_REPORT2 ||
		pRfd->Status.PacketReportType == C2H_PACKET)
	{
		pExtAdapter = GetDefaultAdapter(pAdapter);
		MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
		return uTargetAdapter;
	}

#if (P2P_SUPPORT == 1)
	// Based on the frame content, determine if this adapter accepts the frame
	if(P2PAdapterAcceptFrame(GetDefaultAdapter(pAdapter), osFrame))
	{
		MultiPortInsertIntoTargetAdapterList(GetDefaultAdapter(pAdapter), TargetList, &uTargetAdapter, uMaxTarget);
	}
	// Win8: Send packets to Device port --------------------------------------------------------------------
	pExtAdapter = GetFirstDevicePort(pAdapter);
	if(pExtAdapter != NULL)
	{
		// Based on the frame content, determine if this adapter accepts the frame
		if(P2PAdapterAcceptFrame(pExtAdapter, osFrame))
		{
			MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
		}
	}



	if(PacketGetActionFrameType(&osFrame) == ACT_PKT_GAS_INT_REQ)
	{// Service Discovery Request: Only Send to Device Port 

		// Here should have more consideration.

		// Win7
		pExtAdapter = GetDefaultAdapter(pAdapter);

		if(P2P_ENABLED(GET_P2P_INFO(pExtAdapter)))
		{
			//if(GET_P2P_INFO(pExtAdapter)->Role == P2P_DEVICE) // this blocks the SD req when we are GO under Win7
			{
				MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
			}
		}

		// Win8 
		pExtAdapter = GetFirstDevicePort(pAdapter);

		if(pExtAdapter != NULL)
		{
			MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
		}

		return uTargetAdapter;
	}
#endif

	// Probe Request Filter 
	if(PacketGetType(osFrame) == Type_Probe_Req)
	{	
		// Normal AP
		pExtAdapter = GetDefaultAdapter(pAdapter);
		while(pExtAdapter != NULL)
		{
			if(IN_SEND_BEACON_MODE(pExtAdapter ) || pExtAdapter->MgntInfo.bNetMonitorMode)
			{
				MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
			}

		// Workaround for NdisTest-v8150 WFD_Group_ext test due to the SUT's not cleaning the resources ----------------
		//  The WFD_Performance_ext test will therefore have two device ports, which is not valid.
		//  We let it can be discovered for passing the test.
		#if (P2P_SUPPORT == 1) 
			if(P2P_ENABLED(GET_P2P_INFO(pExtAdapter)))
			{
				if(GET_P2P_INFO(pExtAdapter)->Role == P2P_DEVICE)
				{
					MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
				}
			}
		#endif
		// --------------------------------------------------------------------------------------------------

			pExtAdapter = GetNextExtAdapter(pExtAdapter);
		}

		return uTargetAdapter;
	}	

	// Probe Response Filter 
	RT_TRACE(COMP_P2P, DBG_TRACE, ("MultiPortGetTargetAdapterList: check probrsp\n"));	
	
	if(PacketGetType(osFrame) == Type_Probe_Rsp)
	{
		pExtAdapter = GetDefaultAdapter(pAdapter);
		
		while(pExtAdapter != NULL)
		{
			pExtMgntInfo = &(pExtAdapter->MgntInfo);

			if( ((pExtMgntInfo->state_Synchronization_Sta >= STATE_Act_Receive) ||
				(pExtMgntInfo->state_Synchronization_Sta <= STATE_Act_Listen)) &&
				eqMacAddr(Frame_pRaddr(osFrame), pExtAdapter->CurrentAddress)				
			)
			{
				RT_TRACE(COMP_P2P, DBG_TRACE, ("MultiPortGetTargetAdapterList listen state: port number %d\n", pExtAdapter->pNdis62Common->PortNumber));
				MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
				bMatch = TRUE;
			}				
			pExtAdapter = GetNextExtAdapter(pExtAdapter);
		}
	
	}

	if(bMatch)
		return uTargetAdapter;	

	if(PacketGetType(osFrame) == Type_Beacon)
	{
		// Broadcast Packet Filter  , Beacon
		pExtAdapter = GetDefaultAdapter(pAdapter);
		while(pExtAdapter != NULL)
		{
			RT_TRACE(COMP_P2P, DBG_TRACE, ("MultiPortGetTargetAdapterList Broadcast\n"));
			pExtMgntInfo = &(pExtAdapter->MgntInfo);
			if( (pExtMgntInfo->state_Synchronization_Sta >= STATE_Act_Receive) ||
				(pExtMgntInfo->state_Synchronization_Sta <= STATE_Act_Listen)
			)
				MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
			
			pExtAdapter = GetNextExtAdapter(pExtAdapter);
		}
		return uTargetAdapter;
	}

	bMatch = FALSE;
	// BSSID Filter
	RT_PRINT_ADDR(COMP_P2P, DBG_TRACE, "MultiPortGetTargetAdapterList(): receive Bss BSSID:", Frame_pBssid(osFrame));	
	pExtAdapter = GetDefaultAdapter(pAdapter);
	while(pExtAdapter != NULL)
	{
		RT_PRINT_ADDR(COMP_P2P, DBG_TRACE, "MultiPortGetTargetAdapterList(): new Bss BSSID:", pExtAdapter->MgntInfo.Bssid);

		RT_TRACE(COMP_P2P, DBG_TRACE, ("MultiPortGetTargetAdapterList: port number %d\n", pExtAdapter->pNdis62Common->PortNumber));
		if(eqMacAddr(Frame_pBssid(osFrame), pExtAdapter->MgntInfo.Bssid))
		{		
			RT_TRACE(COMP_P2P, DBG_TRACE, ("MultiPortGetTargetAdapterList eqMacAddr: port number %d\n", pExtAdapter->pNdis62Common->PortNumber));	
			MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
			bMatch = TRUE;
		}
			
		pExtAdapter = GetNextExtAdapter(pExtAdapter);
	}

	if(bMatch)
		return uTargetAdapter;

	// Broadcast Packet Filter  
	pExtAdapter = GetDefaultAdapter(pAdapter);
	while(pExtAdapter != NULL)
	{
		MultiPortInsertIntoTargetAdapterList(pExtAdapter, TargetList, &uTargetAdapter, uMaxTarget);
		pExtAdapter = GetNextExtAdapter(pExtAdapter);
	}
	return uTargetAdapter;
}


PADAPTER
MultiPortFeedPacketToMultipleAdapter(
	PADAPTER	pAdapter,
	PRT_RFD		pRfd
)
{
	// NOTE: --------------------------------------------------------------
	//  If only single adapter is needed, return that adapter.
	// --------------------------------------------------------------------

	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PMULTIPORT_COMMON_CONTEXT pMultiPortCommon = MultiPortGetCommonContext(pDefaultAdapter);
	PADAPTER pExtAdapter = NULL;
	PRT_RFD pExtRfd = NULL;
	u4Byte i = 0;
	RT_STATUS rtStatus = RT_STATUS_SUCCESS;
	
	// Assertion Check Variable
	u4Byte uCurrentCloneRFDs;
	
	// Target List 
	u4Byte uTargetAdapter = 0;
	PADAPTER TargetList[10];


	// Single MPDU
	OCTET_STRING frame = {NULL, 0};
	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

	if(pRfd->Status.bHwError)
	{
		RT_TRACE(COMP_RECV, DBG_TRACE, ("MultiPortFeedPacketToMultipleAdapter(): Return because bHwError is true.\n"));
		return NULL;
	}

	// Information Source: pRfd Status Checking -------------------------------------------------------
	RT_ASSERT(pRfd->Buffer.VirtualAddress != NULL, ("Error: pRfd->Buffer.VirtualAddress is NULL!\n"));
	RT_ASSERT(pRfd->Buffer.Length != 0, ("Error: pRfd->Buffer.Length is 0!\n"));
	RT_ASSERT(pRfd->PacketLength <= pRfd->Buffer.Length, ("Error: Packet Too Long!\n"));
	// ------------------------------------------------------------------------------------------

	// Clone RFD Status Checking ----------------------------------------------------------------------------------------------------------------------
	PlatformAcquireSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
	uCurrentCloneRFDs = pMultiPortCommon->uCloneRfdIdleQueueSize + pMultiPortCommon->uCloneRfdBusyQueueSize;
	RT_ASSERT(uCurrentCloneRFDs == pMultiPortCommon->uNumberOfCloneRfds, 	("Failure: Some Clone RFDs are Lost!uCurrentCloneRFDs=%d\n", uCurrentCloneRFDs));
	PlatformReleaseSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
	// ---------------------------------------------------------------------------------------------------------------------------------------------


	// Get the target adapter list -----------------------------------------------------------------------------
	uTargetAdapter = MultiPortGetTargetAdapterList(pAdapter, pRfd, frame, TargetList, sizeof(TargetList) / sizeof(PADAPTER));
	//RT_TRACE(COMP_INIT, DBG_TRACE, ("%s: uTargetAdapter: %d \n", __FUNCTION__, uTargetAdapter));
	
	if(uTargetAdapter == 0)
	{
		// Free the original RFD since the RFD is not necessary
		RT_TRACE(COMP_INIT, DBG_TRACE, ("%s: No Target Adapter Found!\n", __FUNCTION__));
		return NULL;
	}
	else if(uTargetAdapter == 1)
	{
		// Single adapter is needed. Do not free the original RFD, and run the original path
		return TargetList[0];
	}
	// ----------------------------------------------------------------------------------------------------


	// Send to each adapter
	for(i = 0; i < uTargetAdapter; i++)
	{
		// Get the target adapter element
		pExtAdapter = TargetList[i];

		PlatformAcquireSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
		if(RTIsListEmpty(&pMultiPortCommon->CloneRfdIdleQueue))
		{			
			PlatformReleaseSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("%s: No enough Clone RFD!\n", __FUNCTION__));
			break;
		}

		// Acquire an idle Clone RFD and initialize the Clone RFD -----------------------------------------
		pExtRfd = (PRT_RFD) RTRemoveHeadListWithCnt(
				&pMultiPortCommon->CloneRfdIdleQueue, 
				&pMultiPortCommon->uCloneRfdIdleQueueSize
			);

		// + Clone the original information
		PlatformZeroMemory(pExtRfd, sizeof(RT_RFD));
		PlatformMoveMemory(pExtRfd, pRfd, sizeof(RT_RFD));	

		// + Record the needed memory length 
		pExtRfd->mbCloneRfdDataBuffer.Length = pRfd->Buffer.Length;

		// + Allocate the memory based on the needed memory length above
		rtStatus = DrvIFAssociateRFD(pDefaultAdapter, pExtRfd);
		
		if(rtStatus != RT_STATUS_SUCCESS)
		{
			// Return the CloneRFD resource
			RTInsertTailListWithCnt(
				&pMultiPortCommon->CloneRfdIdleQueue, 
				&pExtRfd->List, 
				&pMultiPortCommon->uCloneRfdIdleQueueSize
			);
			PlatformReleaseSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
			//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("%s: No enough memory!\n", __FUNCTION__));
			break;
		}

		//Sinda 20150903, Should assign Buffer's address according to it is clone RFD.
		if(IsCloneRFD(pDefaultAdapter, pExtRfd))
		{
			// + Attach the memory to the CloneRFD
			pExtRfd->Buffer.VirtualAddress = pExtRfd->mbCloneRfdDataBuffer.Buffer;
			pExtRfd->Buffer.Length = pExtRfd->mbCloneRfdDataBuffer.Length;
		}

#if RX_AGGREGATION
		//   + No Next RFD
		pExtRfd->NextRfd = NULL;

		//  + Only Single MPDU Packet
		pExtRfd->nTotalFrag = 1;

		//   + No Parent RFD
		pExtRfd->ParentRfd = NULL;

		//   + Not in the USB temp RFD list: pAdapter->RfdTmpList
		pExtRfd->bIsTemp = FALSE;
#endif

		//	Please be careful to handle pRfd->Buffer.VirtualAddress offset.
		//	+ Move data
		PlatformMoveMemory(
				pExtRfd->Buffer.VirtualAddress,
				pRfd->Buffer.VirtualAddress - pAdapter->HalFunc.GetRxPacketShiftBytesHandler(pRfd), 
				(pRfd->PacketLength + pAdapter->HalFunc.GetRxPacketShiftBytesHandler(pRfd))>pAdapter->MAX_RECEIVE_BUFFER_SIZE? pAdapter->MAX_RECEIVE_BUFFER_SIZE:(pRfd->PacketLength + pAdapter->HalFunc.GetRxPacketShiftBytesHandler(pRfd))
			);

		//   + Get shifted bytes of starting address of 802.11 header (Sync the memory offset)
		pExtRfd->Buffer.VirtualAddress += pAdapter->HalFunc.GetRxPacketShiftBytesHandler(pRfd);
	 	// -------------------------------------------------------------------------------------

		// Insert into busy Clone RFD queue
		RTInsertHeadListWithCnt(
				&pMultiPortCommon->CloneRfdBusyQueue, 
				&pExtRfd->List, 
				&pMultiPortCommon->uCloneRfdBusyQueueSize
			);

		PlatformReleaseSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
		// Iteration Flag
		pExtRfd->bFeedPacketToSingleAdapter = TRUE;

		// The pExtRfd will be free in ProcessReceivedPacket()
		ProcessReceivedPacketForEachPortSpecific(pExtAdapter, pExtRfd);
	}


	// Free the original RFD since the CloneRFD is adopted.
	return NULL;
}

VOID
MultiPortReturnCloneRFD(
	PADAPTER	pAdapter,
	PRT_RFD		pRfd
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PMULTIPORT_COMMON_CONTEXT pMultiPortCommon = MultiPortGetCommonContext(pDefaultAdapter);
	
	if(IsCloneRFD(pDefaultAdapter, pRfd))
	{
		RT_ASSERT(pRfd->bFeedPacketToSingleAdapter == TRUE, ("Error: Clone RFD may be returned again!\n"));
	
		// Return the Multiport Clone RFDs -------------------------------------------
		if(pRfd->bFeedPacketToSingleAdapter)
		{
			// Clean the MultiPort Flag
			pRfd->bFeedPacketToSingleAdapter = FALSE;

			// Return the data buffer
			RT_ASSERT(pRfd->mbCloneRfdDataBuffer.Buffer != NULL, ("Multiport Clone RFD should have a data buffer!\n"));
			DrvIFDisassociateRFD(pDefaultAdapter, pRfd);
		
			PlatformAcquireSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
			// Remove the RFD from Busy Queue
			RTRemoveEntryListWithCnt(
					&pRfd->List, 
					&pMultiPortCommon->uCloneRfdBusyQueueSize
				);

			// Return the RFD to the Idle Queue
			RTInsertTailListWithCnt(
					&pMultiPortCommon->CloneRfdIdleQueue, 
					&pRfd->List, 
					&pMultiPortCommon->uCloneRfdIdleQueueSize
				);
			PlatformReleaseSpinLock(pDefaultAdapter, RT_RFD_SPINLOCK);
		}
	}
	else
	{
		RT_TRACE(COMP_RECV, DBG_WARNING, ("This RFD is not CloneRFD: 0x%p!\n", pRfd));
	}
}
BOOLEAN
IsMultiPortAllowDisableHWSecurity(
	PADAPTER	Adapter
	)
{
	PADAPTER  pAdapter =  GetDefaultAdapter(Adapter);
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);;

	while(pAdapter != NULL)
	{		
		if(!pMgntInfo->SecurityInfo.SWTxEncryptFlag || !pMgntInfo->SecurityInfo.SWRxDecryptFlag)
			return FALSE;

		pAdapter = GetNextExtAdapter(pAdapter);
		pMgntInfo = &(pAdapter->MgntInfo);
	}
	return TRUE;
}


BOOLEAN
IsActiveAPModeExist(
	PADAPTER	Adapter
)
{
	PADAPTER  pAdapter =  GetDefaultAdapter(Adapter);

	while(pAdapter != NULL)
	{
		if(AP_DetermineAlive(pAdapter))
			return TRUE;

		pAdapter = GetNextExtAdapter(pAdapter);
	}

	return FALSE;
}

BOOLEAN
IsAPModeExist(
	PADAPTER	Adapter
)
{
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);
	PADAPTER	pAdapter =  pDefaultAdapter;
	while(pAdapter != NULL)
	{
		if(ACTING_AS_AP(pAdapter))
			return TRUE;
		pAdapter = GetNextExtAdapter(pAdapter);
	}

 
	return FALSE;
}

BOOLEAN
IsExtAPModeExist(
 PADAPTER Adapter
)
{
	 PADAPTER pAdapter =  GetFirstExtAdapter(Adapter);
	 
	 while(pAdapter != NULL)
	 {
		if(ACTING_AS_AP(pAdapter))
			return TRUE;
		pAdapter = GetNextExtAdapter(pAdapter);
	 }
	 
	 return FALSE;
}

BOOLEAN
IsDevicePortDiscoverable(
	PADAPTER pAdapter
)
{
#if P2P_SUPPORT == 1

	PADAPTER pDevicePort = GetFirstDevicePort(pAdapter);
	PP2P_INFO pDeviceP2PInfo = NULL;
	
	if(pDevicePort)
	{
		pDeviceP2PInfo = GET_P2P_INFO(pDevicePort);

		if(pDeviceP2PInfo->uListenStateDiscoverability)
			return TRUE;
	}
#endif

	return FALSE;
}

BOOLEAN
IsCloneRFD(
	PADAPTER pAdapter,
	PRT_RFD pRfd
)
{
	PMULTIPORT_COMMON_CONTEXT pMultiPortCommon = MultiPortGetCommonContext(pAdapter);
	PRT_RFD pRfdStart = (PRT_RFD) pMultiPortCommon->CloneRfdMemoryBuffer.Buffer;
	u4Byte IndexMax = pMultiPortCommon->uNumberOfCloneRfds;
	
	if(pRfd >= pRfdStart && pRfd <= pRfdStart + IndexMax - 1)
	{
		return TRUE;
	}

	return FALSE;
}

VOID
MultiPortDumpPortStatus(
	PADAPTER pAdapter
)
{
	PADAPTER			pTargetAdapter = pAdapter;
	PMGNT_INFO			pMgntInfo = &(pTargetAdapter->MgntInfo);

	u4Byte				portNumber = pTargetAdapter->pNdis62Common->PortNumber;
	MP_PORT_TYPE		portType = pTargetAdapter->pNdis62Common->PortType;
	MP_PORT_OP_STATE	portState = pTargetAdapter->pNdis62Common->CurrentOpState;

	RT_TRACE(COMP_MLME, DBG_LOUD, ("PORT_NUMBER: %d\n", portNumber));
	RT_TRACE(COMP_MLME, DBG_LOUD, ("pAdapter: %p\n", pAdapter));
	RT_TRACE(COMP_MLME, DBG_LOUD, (", MP_PORT_TYPE: %s\n", 
			(portType == HELPER_PORT) ? "HELPER_PORT" :
			(portType == EXTSTA_PORT) ? "EXTSTA_PORT" :
			(portType == EXTAP_PORT) ? "EXTAP_PORT" :
			(portType == EXT_P2P_DEVICE_PORT) ? "EXT_P2P_DEVICE_PORT" :
			(portType == EXT_P2P_ROLE_PORT) ? "EXT_P2P_ROLE_PORT" :
			"ERROR"
		));

#if (P2P_SUPPORT == 1)
	if(pMgntInfo->pP2PInfo)
	{
		PP2P_INFO			pP2PInfo = (PP2P_INFO)(pMgntInfo->pP2PInfo);
		
		RT_TRACE(COMP_P2P, DBG_LOUD, (", P2PSupport Type: %d\n", pTargetAdapter->P2PSupport));
				
		RT_TRACE(COMP_P2P, DBG_LOUD, (", P2P_ROLE: %s\n", 
				(pP2PInfo->Role == P2P_NONE) ? "P2P_NONE" :
				(pP2PInfo->Role == P2P_DEVICE) ? "P2P_DEVICE" :
				(pP2PInfo->Role == P2P_CLIENT) ? "P2P_CLIENT" :
				(pP2PInfo->Role == P2P_GO) ? "P2P_GO" : 
				"ERROR"
			));
		RT_TRACE(COMP_P2P, DBG_LOUD, (", P2P_STATE: %d\n", 
				pP2PInfo->State
			));
		
		P2PSvc_Dump(pTargetAdapter);
	}
#endif

	RT_TRACE(COMP_MLME, DBG_LOUD, (", MP_PORT_OP_STATE: %s \n", 
			(portState == INIT_STATE) ? "INIT_STATE" :
			(portState == OP_STATE) ? "OP_STATE" : 
			"ERROR"
		));

}


RT_STATUS
MultiPortAllocateCloneRfdBuffer(
	PADAPTER pAdapter,
	PRT_RFD pRfd
)
{
	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	u4Byte	MaxLen=pRfd->mbCloneRfdDataBuffer.Length;
	
	// Correctness Checking --------------------------------------
	RT_ASSERT(
			pRfd->mbCloneRfdDataBuffer.Length != 0 && 
			pRfd->mbCloneRfdDataBuffer.Buffer == NULL, 
			("DrvIFAssociateRFD: Wrong Parameters Observed!\n")
		);

	// Get the non-shared memory ---------------------------------
	rtStatus = PlatformAllocateMemory(
			pAdapter, 
			(PVOID*)&(pRfd->mbCloneRfdDataBuffer.Buffer), 
			MaxLen
		);


	return rtStatus;
}

VOID
MultiPortReleaseCloneRfdBuffer(
	PRT_RFD pRfd
)
{

	// Correctness Checking --------------------------------------
	RT_ASSERT(
			pRfd->mbCloneRfdDataBuffer.Length != 0 && 
			pRfd->mbCloneRfdDataBuffer.Buffer != NULL, 
			("DrvIFDisassociateRFD: Wrong Parameters Observed!\n")
		);

	RT_ASSERT(
			pRfd->bFeedPacketToSingleAdapter == FALSE, 
			("Error: Clone RFD in use should not be returned!\n")
		);

	// Free the non-shared memory --------------------------------
	PlatformFreeMemory(
			pRfd->mbCloneRfdDataBuffer.Buffer, 
			pRfd->mbCloneRfdDataBuffer.Length
		);
}

VOID
MultiPortInitializeCloneRfdQueue(
	PADAPTER	pAdapter
)
{
	PMULTIPORT_COMMON_CONTEXT pMultiPortCommon = MultiPortGetCommonContext(pAdapter);
	
	PlatformAcquireSpinLock(pAdapter, RT_RFD_SPINLOCK);
	pMultiPortCommon->uCloneRfdIdleQueueSize = 0;
	RTInitializeListHead(&pMultiPortCommon->CloneRfdIdleQueue);
	
	pMultiPortCommon->uCloneRfdBusyQueueSize = 0;
	RTInitializeListHead(&pMultiPortCommon->CloneRfdBusyQueue);
	PlatformReleaseSpinLock(pAdapter, RT_RFD_SPINLOCK);
}

RT_STATUS
MultiPortPrepareCloneRfd(
	PADAPTER	pAdapter
)
{
	RT_STATUS rtStatus = RT_STATUS_FAILURE;
	PRT_RFD	 pRfd = NULL;
	u4Byte i = 0;
	PMULTIPORT_COMMON_CONTEXT pMultiPortCommon = MultiPortGetCommonContext(pAdapter);
		
	const u4Byte SIZE_OF_CLONE_RFD_QUEUE = 256;

	do
	{
		// Allocate memory for the Multiport Clone RFD -------------------------------------------------
		pMultiPortCommon->CloneRfdMemoryBuffer.Length = SIZE_OF_CLONE_RFD_QUEUE * sizeof(RT_RFD);

		rtStatus = PlatformAllocateMemory(pAdapter, 
				&pMultiPortCommon->CloneRfdMemoryBuffer.Buffer,
				pMultiPortCommon->CloneRfdMemoryBuffer.Length
			);

		if(rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("%s: MultiPort Clone RFD: Memory Allocation Failure !\n", __FUNCTION__));
			break;
		}
		// ---------------------------------------------------------------------------------------

		// Insert the Clone RFDs into the queue --------------------------------------------------------
		pMultiPortCommon->uNumberOfCloneRfds = SIZE_OF_CLONE_RFD_QUEUE;

		PlatformZeroMemory(
				pMultiPortCommon->CloneRfdMemoryBuffer.Buffer, 
				pMultiPortCommon->CloneRfdMemoryBuffer.Length
			);
		PlatformAcquireSpinLock(pAdapter,RT_RFD_SPINLOCK);
		pRfd = (PRT_RFD) pMultiPortCommon->CloneRfdMemoryBuffer.Buffer;
		
		for(i = 0; i < pMultiPortCommon->uNumberOfCloneRfds; i++)
		{
			RTInsertTailListWithCnt(
					&pMultiPortCommon->CloneRfdIdleQueue, 
					&pRfd[i].List, 
					&pMultiPortCommon->uCloneRfdIdleQueueSize
				);
		}
		RT_TRACE(COMP_INIT, DBG_LOUD, ("MultiPortPrepareCloneRfd(), pMultiPortCommon->uCloneRfdIdleQueueSize = %d\n",
			pMultiPortCommon->uCloneRfdIdleQueueSize));
		PlatformReleaseSpinLock(pAdapter,RT_RFD_SPINLOCK);
		// ---------------------------------------------------------------------------------------
	} while(FALSE);

	return rtStatus;
}

VOID
MultiPortFreeCloneRfd(
	PADAPTER	pAdapter
)
{
	PRT_RFD pRfd = NULL;
	PMULTIPORT_COMMON_CONTEXT pMultiPortCommon = MultiPortGetCommonContext(pAdapter);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("\n==== MultiPort Clone RFD Status ====\n"));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("Clone RFD Allocated: %d\n", pMultiPortCommon->uNumberOfCloneRfds));
	RT_TRACE(COMP_INIT, DBG_LOUD, ("Clone RFD Freed: %d\n", pMultiPortCommon->uCloneRfdIdleQueueSize));
	RT_ASSERT(pMultiPortCommon->uCloneRfdBusyQueueSize == 0, ("No Busy Clone RFD when in free status !!"));
	RT_ASSERT(pMultiPortCommon->uNumberOfCloneRfds ==pMultiPortCommon->uCloneRfdIdleQueueSize, ("Freed RFD less than allocated!!\n"));	

	RT_TRACE(COMP_INIT, DBG_LOUD, ("====================================\n\n"));

	PlatformAcquireSpinLock(pAdapter, RT_RFD_SPINLOCK);
	while(!RTIsListEmpty(&pMultiPortCommon->CloneRfdIdleQueue))
	{
		pRfd = (PRT_RFD) RTRemoveHeadListWithCnt(&pMultiPortCommon->CloneRfdIdleQueue, &pMultiPortCommon->uCloneRfdIdleQueueSize);
	}
	PlatformReleaseSpinLock(pAdapter, RT_RFD_SPINLOCK);

	PlatformFreeMemory(
			pMultiPortCommon->CloneRfdMemoryBuffer.Buffer, 
			pMultiPortCommon->CloneRfdMemoryBuffer.Length
		);
}

VOID
MultiPortInitializeContext(
	PADAPTER pDefaultAdapter
)
{
	PMULTIPORT_COMMON_CONTEXT pMultiPortCommon = MultiPortGetCommonContext(pDefaultAdapter);
	PMULTIPORT_PORT_CONTEXT	pMultiPortDefault = MultiPortGetPortContext(pDefaultAdapter);

	pMultiPortCommon->uNumberOfActiveExtAdapters = 0;
	
	RTInitializeListHead(&pMultiPortDefault->MultiList);
	pMultiPortDefault->bActiveAdapter = TRUE;
	pMultiPortDefault->pAdapter = pDefaultAdapter;
}

//
// Description:
//	Record the current scan complete time to all adapters.
// Arguments:
//	[in] pAdapter -
//		One of adapter list.
// Return:
//	None
// Remark:
//	This function save the current time as the scan complete mark into each
//	adapter.
//	The input adapter can be any one of the adapter list.
// By Bruce, 2012-03-02.
//
VOID
MultiportRecordLastScanTime(
	IN	PADAPTER	pAdapter
	)
{
	PADAPTER	pLoopAdapter = GetDefaultAdapter(pAdapter);
	u8Byte		CurrTime = PlatformGetCurrentTime();

	while(pLoopAdapter)
	{
		pLoopAdapter->LastScanCompleteTime = CurrTime;
		pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
	}
}


BOOLEAN
MultiPortCanCheckCheckBssid(
	PADAPTER	pAdapter
)
{
	BOOLEAN		bCanCheckBssid = TRUE;

	do
	{
		if(pAdapter->MgntInfo.bWiFiConfg)
		{
			break;
		}
		
		if(GetFirstClientPort(pAdapter))
		{
			bCanCheckBssid = FALSE;
			break;
		}

		if(IsDevicePortDiscoverable(pAdapter))
		{
			bCanCheckBssid = FALSE;
			break;
		}

		if(MultiChannelSwitchNeeded(pAdapter))
		{
			RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("MultiChannelSwitchNeeded() return FALSE!\n"));
			bCanCheckBssid = FALSE;
			break;
		}
	}while(FALSE);
		

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bCanCheckBssid: %d\n", bCanCheckBssid));

	return bCanCheckBssid;
}

VOID
MultiPortInsertIdleExtAdapter(
	PADAPTER	pDefaultAdapter,
	PADAPTER 	pAdapter
)
{
	PMULTIPORT_PORT_CONTEXT pMultiPortDefault = MultiPortGetPortContext(pDefaultAdapter);
	PMULTIPORT_PORT_CONTEXT pMultiPort = MultiPortGetPortContext(pAdapter);

	RT_TRACE(COMP_INIT, DBG_LOUD, ("Insert idle Ext Adapter %p\n", pAdapter));

	pMultiPort->bActiveAdapter = FALSE;
	pMultiPort->pAdapter = pAdapter;

	// + MultiPort Insert the Adapter into the Adapter Queue
	RTInsertTailList(
			&pMultiPortDefault->MultiList, 
			&pMultiPort->MultiList
		);
}

VOID
MultiPortChangeExtAdapterActiveState(
	PADAPTER 	pAdapter,
	BOOLEAN		bActive
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PMULTIPORT_COMMON_CONTEXT pMultiPortCommon = MultiPortGetCommonContext(pDefaultAdapter);
	PMULTIPORT_PORT_CONTEXT pMultiPort = MultiPortGetPortContext(pAdapter);	
	
	RT_ASSERT(pMultiPort->bActiveAdapter != bActive, ("Same state changing is illegal !"));

	pMultiPort->bActiveAdapter = bActive;

	if(bActive)
	{
		pMultiPortCommon->uNumberOfActiveExtAdapters++;
	}
	else
	{
		pMultiPortCommon->uNumberOfActiveExtAdapters--;
	}
}

u4Byte
MultiPortGetNumberOfActiveExtAdapters(
	PADAPTER pAdapter
)
{
	PMULTIPORT_COMMON_CONTEXT pMultiPortCommon = MultiPortGetCommonContext(pAdapter);	

	return pMultiPortCommon->uNumberOfActiveExtAdapters;
}

BOOLEAN
MultiPortIsPreallocatedExtAdapterExist(
	PADAPTER pDefaultAdapter
)
{
	PMULTIPORT_PORT_CONTEXT pMultiPortDefault = MultiPortGetPortContext(pDefaultAdapter);
	
	return (!RTIsListEmpty(&pMultiPortDefault->MultiList));
}

BOOLEAN
MultiPortIsActiveExtAdapter(
	PADAPTER pAdapter
)
{
	PMULTIPORT_PORT_CONTEXT pMultiPort = MultiPortGetPortContext(pAdapter);

	return pMultiPort->bActiveAdapter;
}

//
// Description:
//	Get the latest time of connection action from current.
// Arguments:
//	[in] pAdapter -
//		One of adapter list.
// Return:
//	Time in micro-seconds for the last connection action.
// Remark:
//	If the returned value is 0, there's no recorded time of connection.
// By Bruce, 2015-02-10.
//
u8Byte
MultiportGetLastConnectionActionTime(
	IN	PADAPTER	pAdapter
	)
{
	PADAPTER	pLoopAdapter = GetDefaultAdapter(pAdapter);
	u8Byte		tmpConnectionActionTime = 0;

	while(pLoopAdapter)
	{
		if(pLoopAdapter->LastConnectionActionTime > tmpConnectionActionTime)
			tmpConnectionActionTime = pLoopAdapter->LastConnectionActionTime;
		
		pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
	}
	return tmpConnectionActionTime;
}

#else 

//============================================================================
// Single Port Supported 
//============================================================================

VOID
MultiportRecordLastScanTime(
	IN	PADAPTER	pAdapter
	)
{
	u8Byte		CurrTime = PlatformGetCurrentTime();
	PADAPTER	DefAdapter = GetDefaultAdapter(pAdapter);

	DefAdapter->LastScanCompleteTime = CurrTime;

}


#endif
