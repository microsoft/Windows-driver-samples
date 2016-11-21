#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "N6Sdio_DrvIF.tmh"
#endif

/*---------------------------Define Local Constant---------------------------*/
// For AMSDU test
// For 802.11 header pseudo sequence number test.
#define		N6_PSDUDO_SEQ_NUM				0
//#define		N6_LLC_SIZE						8
#define		N6_MAX_AMSDU_SUBF_NUM			20

#if (N6_PSDUDO_SEQ_NUM == 1)
static	UINT16		Pseudo_Seq_Num = 0;
#endif

RT_STATUS
DrvIFAssociateRFD(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	PRT_SDIO_DEVICE		device = NULL;

	device = &Adapter->NdisSdioDev;

	if(IsCloneRFD(Adapter, pRfd))
	{
		rtStatus = MultiPortAllocateCloneRfdBuffer(Adapter, pRfd);
	}
	else
	{
		rtStatus=PlatformAllocateAlignedSharedMemory(
				Adapter,
				&(pRfd->Buffer),
				Adapter->MAX_RECEIVE_BUFFER_SIZE
			);
		
		pRfd->Buffer.Length = Adapter->MAX_RECEIVE_BUFFER_SIZE;
	}

	return rtStatus;
}

VOID
DrvIFDisassociateRFD(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
	)
{
	if(pRfd->Buffer.VirtualAddress==NULL)
		return;

	if(IsCloneRFD(Adapter, pRfd))
	{
		MultiPortReleaseCloneRfdBuffer(pRfd);
	}
	else
	{
		PlatformFreeAlignedSharedMemory(Adapter, &(pRfd->Buffer));
	}
}

BOOLEAN
FillN6NBLInfo(
	PADAPTER			Adapter,
	PNET_BUFFER_LIST	pNBL,
	PRT_RFD				pRfd
	)
{
	PDOT11_EXTSTA_RECV_CONTEXT	pRecvContext=NULL;
	NDIS_NET_BUFFER_LIST_8021Q_INFO	NBL8021qInfo;
	RT_STATUS	rtStatus=RT_STATUS_FAILURE;

	if(pNBL==NULL || pRfd==NULL)
		return FALSE;

	if( OS_SUPPORT_WDI(Adapter) )
	{
	rtStatus = WDIAllocateMetaData(Adapter, pNBL, pRfd);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate WifiRxMetaData\n"));
		return FALSE;
	}
	}
	else
	{
		rtStatus = PlatformAllocateMemory(
					Adapter,
					&pRecvContext,
					sizeof(DOT11_EXTSTA_RECV_CONTEXT));
		if (rtStatus != RT_STATUS_SUCCESS)
		{
			RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate DOT11_EXTSTA_RECV_CONTEXT\n"));
			return FALSE;
		}

		PlatformZeroMemory(pRecvContext, sizeof(DOT11_EXTSTA_RECV_CONTEXT));
		N6_ASSIGN_OBJECT_HEADER(
			pRecvContext->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_EXTSTA_RECV_CONTEXT_REVISION_1,
			sizeof(DOT11_EXTSTA_RECV_CONTEXT));
		pRecvContext->uReceiveFlags = 0;  // set to zero if in ExtSta mode.
		pRecvContext->uPhyId = N6CQuery_DOT11_OPERATING_PHYID(Adapter);
		pRecvContext->uChCenterFrequency = 
		MgntGetChannelFrequency(RT_GetChannelNumber(Adapter));
		pRecvContext->usNumberOfMPDUsReceived = pRfd->nTotalFrag;
		//pRecvContext->lRSSI = Adapter->RxStats.SignalStrength; // Report the smoothed signal strength.
		pRecvContext->lRSSI = pRfd->Status.RecvSignalPower; //Report real RSSI to OS, fix maximum -45 dbm for Network monitor, 201502 Sean
		pRecvContext->ucDataRate = pRfd->Status.DataRate;
		pRecvContext->uSizeMediaSpecificInfo = 0;
		pRecvContext->pvMediaSpecificInfo = NULL;
		pRecvContext->ullTimestamp = ((u8Byte)pRfd->Status.TimeStampHigh << 32) + pRfd->Status.TimeStampLow;
	
		NET_BUFFER_LIST_INFO(pNBL, MediaSpecificInformation) = pRecvContext;
	}

	if(Adapter->MgntInfo.pStaQos->CurrentQosMode != QOS_DISABLE)
	{
		NBL8021qInfo.Value = NET_BUFFER_LIST_INFO(pNBL, Ieee8021QNetBufferListInfo);
		NBL8021qInfo.WLanTagHeader.WMMInfo = pRfd->Status.UserPriority;
		NET_BUFFER_LIST_INFO(pNBL, Ieee8021QNetBufferListInfo) = NBL8021qInfo.Value;
	}
	
	return TRUE;
}


VOID
FreeN6NBLInfo(
	PADAPTER			Adapter,
	PNET_BUFFER_LIST	pNBL
	)
{
	if( OS_SUPPORT_WDI(Adapter) )
	{
	WDIFreeMetaData(Adapter, pNBL);
}
	else
	{
		PDOT11_EXTSTA_RECV_CONTEXT				pRecvContext = NULL;
		
		pRecvContext = (PDOT11_EXTSTA_RECV_CONTEXT)NET_BUFFER_LIST_INFO(pNBL, MediaSpecificInformation);
	
		if( pRecvContext != NULL )
			PlatformFreeMemory(pRecvContext, sizeof(DOT11_EXTSTA_RECV_CONTEXT));
	}
}

VOID
DrvIFIndicatePackets(
	PADAPTER				Adapter,
	PRT_RFD					*pRfd_array,
	u2Byte					Num
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(Adapter);
	PRT_SDIO_DEVICE	usbdevice;
	NDIS_HANDLE			NBLPoolHandle;
	PMDL				pMdl = NULL;
	PMDL				pCurrMdl = NULL, pNextMdl = NULL;
	PNET_BUFFER_LIST	pNetBufferList = NULL;
	PNET_BUFFER_LIST	pLastNetBufferList = NULL;
	PNET_BUFFER_LIST	pFirstNetBufferList = NULL;
	PRT_RFD				pTmpRfd = NULL;
	ULONG				RecvFlags = 0;

	u2Byte	rfd_index = 0;
	u2Byte	subframe_hdr_idx = 0;
	u2Byte	subframe_index = 0;
	u2Byte	packet_index = 0;
	PRT_RFD	pCurRfd = NULL;


	// Indicate if the RFD is returned or will be indicated to OS ---
	pu1Byte pRfdReturnOK = NULL;		
	PRT_GEN_TEMP_BUFFER pGenTempBuffer = NULL;
	// --------------------------------------------------
	
	// Resource Management --------------------
	u4Byte 			ResourceMointor = 0;
	const u4Byte		RESOURCE_MDL = BIT1;
	const u4Byte		RESOURCE_NBL = BIT2;
	const u4Byte		RESOURCE_INFO = BIT3;
	// ---------------------------------------

	// For WDI architecture ---------------------
	u4Byte			PeerId;
	BOOLEAN			bDataInQueue[MAX_PEER_NUM] = {0};
	u2Byte			index = 0;
	// ---------------------------------------
	
	if(Num==0 || pRfd_array==NULL)
		return;

	// Check if the RFD is returned or indicated to the OS ------
	pGenTempBuffer = GetGenTempBuffer(Adapter, Num);
	pRfdReturnOK = pGenTempBuffer->Buffer.Ptr;
	PlatformZeroMemory(pRfdReturnOK, Num);
	// -------------------------------------------------
	
	usbdevice = &Adapter->NdisSdioDev;
	NBLPoolHandle = usbdevice->RxNetBufferListPool;

	for(rfd_index = 0; rfd_index < Num; rfd_index++)
	{
		pCurRfd = pRfd_array[rfd_index];

    		if(RT_STATUS_SUCCESS == WAPI_SecFuncHandler(WAPI_DROPFORRXREORDER,Adapter,(PVOID)pCurRfd, WAPI_END))
    		{
    			RT_TRACE(COMP_SEC,DBG_LOUD,("=====>Rx Reorder Drop case \n"));
    			ReturnRFDList(Adapter, pCurRfd);
			pRfdReturnOK[rfd_index] = TRUE;
    			continue;
    		}

		if( !N6ReceiveIndicateFilter(Adapter, pCurRfd) ||
			N6SDIO_CANNOT_RX(Adapter) )
		{
			ReturnRFDList(Adapter, pCurRfd);
			pRfdReturnOK[rfd_index] = TRUE;
			continue;
		}

#if RX_TCP_SEQ_CHECK
		RxTcpSeqCheck(Adapter, pCurRfd);
#endif

		if(pCurRfd->nTotalSubframe > 0)
		{
			// Check if the resource is clear -----------------------------------
			RT_ASSERT(ResourceMointor == 0, ("Error: Resource is Not Clear!\n"));
			// ------------------------------------------------------------
			
			// By Lanhsin. Copy subframe of AMSDU into one contineous buffer before 
			// indication. Otherwise, F-Secure anti-virus program cannot accept a
			// packet with more than one buffer. 2008.07.10 (Vista only)

			for(subframe_index = 0; subframe_index < pCurRfd->nTotalSubframe; subframe_index++, packet_index++,subframe_hdr_idx++)
			{
				

				Adapter->AmsduIndex = Adapter->AmsduIndex % Adapter->MAX_SUBFRAME_TOTAL_COUNT;

				PlatformMoveMemory(Adapter->pAMSDU[Adapter->AmsduIndex].VirtualAddress, 
									pCurRfd->Buffer.VirtualAddress + pCurRfd->FragOffset, 
									sMacHdrLng);		

				PlatformMoveMemory((Adapter->pAMSDU[Adapter->AmsduIndex].VirtualAddress + 24), pCurRfd->SubframeArray[subframe_index], 
									pCurRfd->SubframeLenArray[subframe_index]);


				// Allocate MDL of 802.11 header. We use the same header for all AMSDU 
				// subframe. Sequence number are the same for all sub packet.
				// The first MDL must contain 802.11 header and LLC.
				pMdl = NdisAllocateMdl(	N6SDIO_GET_MINIPORT_HANDLE(GetDefaultAdapter(Adapter)), (PVOID)Adapter->pAMSDU[Adapter->AmsduIndex].VirtualAddress, 
											sMacHdrLng+pCurRfd->SubframeLenArray[subframe_index]);

				if (pMdl == NULL)
				{
					RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate MDL\n"));			
					goto POST_PROCESS;
				}
				else
				{
					ResourceMointor |= RESOURCE_MDL;
				}
				
				//
				// Allocate NetBufferList for receive indication, 2006.10.04, by shien chang.
				//
				pNetBufferList = NdisAllocateNetBufferAndNetBufferList(
						N6_GET_RX_NBL_POOL(Adapter),	// PoolHandle
						0,								// ContextSize
						0,								// ContextBackFill
						pMdl,							// MdlChain
						0,								// DataOffset
						pCurRfd->SubframeLenArray[subframe_index]+sMacHdrLng);	//DataLength

				if (pNetBufferList == NULL)
				{
					RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate NetBufferList\n"));
					goto POST_PROCESS;
				}
				else
				{
					ResourceMointor |= RESOURCE_NBL;
				}

				//
				// Now prepare necessary information such as SourceHandle, OOB data for indication.
				//
				pNetBufferList->SourceHandle = N6SDIO_GET_MINIPORT_HANDLE(Adapter); 

				// Filling information in NetBufferList.
				if(FillN6NBLInfo(Adapter, pNetBufferList, pCurRfd) == FALSE)
				{
					RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to set information to NetBufferList\n"));
					goto POST_PROCESS;
				}
				else
				{
					ResourceMointor |= RESOURCE_INFO;
				}

				if(pFirstNetBufferList == NULL)
				{
					pFirstNetBufferList = pLastNetBufferList = pNetBufferList;
				}
				else
				{
					NET_BUFFER_LIST_NEXT_NBL(pLastNetBufferList) = pNetBufferList;
					pLastNetBufferList = pNetBufferList;
				}

				if(subframe_index == pCurRfd->nTotalSubframe - 1)
				{
					// The RFD will be indicated to the OS and returned ----------
					MP_SET_PACKET_RFD(pNetBufferList, pCurRfd);
					pRfdReturnOK[rfd_index] = TRUE;
					// ---------------------------------------------------
				}
				else
				{
					MP_SET_PACKET_RFD(pNetBufferList, NULL);
				}


				// Clean the Resource Mointor ----------
				ResourceMointor = 0;
				// ---------------------------------

				Adapter->AmsduIndex++;

			}
		}
		else
		{
			// Check if the resource is clear -----------------------------------
			RT_ASSERT(ResourceMointor == 0, ("Error: Resource is Not Clear!\n"));
			// ------------------------------------------------------------
			
			pMdl = NULL;
			pTmpRfd = pCurRfd;
		
			//
			// Allocate MDLChain. 2006.10.04, by shien chang.
			//
			do{
				if (pMdl == NULL)
				{
					pMdl = NdisAllocateMdl(
							N6SDIO_GET_MINIPORT_HANDLE(GetDefaultAdapter(Adapter)), 
							(PVOID)(pCurRfd->Buffer.VirtualAddress+pCurRfd->FragOffset),
							pCurRfd->FragLength);
					pCurrMdl = pMdl;
				}
				else
				{
					NDIS_MDL_LINKAGE(pCurrMdl) = NdisAllocateMdl(
							N6SDIO_GET_MINIPORT_HANDLE(GetDefaultAdapter(Adapter)), 
							(PVOID)(pTmpRfd->Buffer.VirtualAddress+pTmpRfd->FragOffset),
							pTmpRfd->FragLength);
					pCurrMdl = NDIS_MDL_LINKAGE(pCurrMdl);
						
				}

				if (pCurrMdl == NULL)
				{
					RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate MDL\n"));
					goto POST_PROCESS;
				}
				else
				{
					ResourceMointor |= RESOURCE_MDL;
				}

				pTmpRfd=pTmpRfd->NextRfd;
			}while(pTmpRfd);

			//
			// Allocate NetBufferList for receive indication, 2006.10.04, by shien chang.
			//
			pNetBufferList = NdisAllocateNetBufferAndNetBufferList(
							N6_GET_RX_NBL_POOL(Adapter),		// PoolHandle
							0,					// ContextSize
							0,					// ContextBackFill
							pMdl,				// MdlChain
							0,					// DataOffset
							pCurRfd->PacketLength);	//DataLength
			if (pNetBufferList == NULL)
			{
				RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate NetBufferList\n"));
				goto POST_PROCESS;
			}
			else
			{
				ResourceMointor |= RESOURCE_NBL;
			}

			//
			// Now prepare necessary information such as SourceHandle, OOB data for indication.
			//
			pNetBufferList->SourceHandle = N6SDIO_GET_MINIPORT_HANDLE(Adapter);

			// Filling information in NetBufferList.
			if(FillN6NBLInfo(Adapter, pNetBufferList, pCurRfd) == FALSE)
			{
				RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to set information to NetBufferList\n"));
				goto POST_PROCESS;
			}
			else
			{
				ResourceMointor |= RESOURCE_INFO;
			}

			if(pFirstNetBufferList == NULL)
			{
				pFirstNetBufferList = pLastNetBufferList = pNetBufferList;
			}
			else
			{
				NET_BUFFER_LIST_NEXT_NBL(pLastNetBufferList) = pNetBufferList;
				pLastNetBufferList = pNetBufferList;
			}

			// The RFD will be indicated to the OS and returned -------
			MP_SET_PACKET_RFD(pNetBufferList, pCurRfd);
			pRfdReturnOK[rfd_index] = TRUE;
			// ------------------------------------------------
			
			packet_index++;

			// Clean the Resource Mointor ---------
			ResourceMointor = 0;
			// ---------------------------------
		}
		
		if( OS_SUPPORT_WDI(Adapter) )
		{
		// PeerId is 1-based, 0 use to represent failure
		PeerId = WDI_InsertDataInQueue(Adapter, pCurRfd, pFirstNetBufferList);

		if( PeerId != 0 )
			bDataInQueue[PeerId - 1] = TRUE;
		else
			WDI_FreeRxFrame(Adapter, pFirstNetBufferList);

		pFirstNetBufferList = NULL;
	}
	}

POST_PROCESS:

	// Free information for NBL
	if( ResourceMointor & RESOURCE_INFO )
	{
		FreeN6NBLInfo(Adapter, pNetBufferList);
	}

	// Free MDL allocated before.
	if(ResourceMointor & RESOURCE_MDL)
	{
		for (pCurrMdl = pMdl; pCurrMdl != NULL; pCurrMdl = pNextMdl)
		{
			pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
			NdisFreeMdl(pCurrMdl);
		}
	}

	// Free NBL allocated before
	if(ResourceMointor & RESOURCE_NBL)
	{
		NdisFreeNetBufferList(pNetBufferList);
	}

	// Check if we shall reserve current Rfd since some parts of pCurRFD will be indicated to OS
	if(pCurRfd!= NULL && pCurRfd->nTotalSubframe != 0 && subframe_index != 0)
	{
		MP_SET_PACKET_RFD(pLastNetBufferList, pCurRfd);
		pRfdReturnOK[rfd_index] = TRUE;

		// Allocate MDL/NBL/NBLInfo during parsing sub frames.
		if( OS_SUPPORT_WDI(Adapter) )
		{
		// PeerId is 1-based, 0 use to represent failure
		PeerId = WDI_InsertDataInQueue(Adapter, pCurRfd, pFirstNetBufferList);

		if( PeerId != 0 )
			bDataInQueue[PeerId - 1] = TRUE;
		else
			WDI_FreeRxFrame(Adapter, pFirstNetBufferList);

		pFirstNetBufferList = NULL;
	}
	}

	// Indicate as much packets as we can.
	if(packet_index != 0)
	{
		RT_ASSERT(IS_RX_LOCKED(Adapter) == TRUE, ("DrvIFIndicatePacket(): bRxLocked should be TRUE\n"));

		PlatformAcquireSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		RT_INC_N_RCV_REF(GetDefaultAdapter(Adapter), packet_index);
		PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);

		PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
		if( OS_SUPPORT_WDI(Adapter) )
		{			
		for(index = 0; index < MAX_PEER_NUM; index++)
		{
			if( bDataInQueue[index] == TRUE )
			{
				//Rx notify thread will check all queue, so we only set event one time.
				PlatformReleaseSemaphore(&(pDefaultAdapter->pNdisCommon->RxNotifySemaphore));
				break;
			}
		}
		}
		else
		{
			RecvFlags = NDIS_CURRENT_IRQL()?NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL:0;
			// Prefast warning 6011: Dereferencing NULL pointer 'Adapter->pNdis62Common'.
			if (Adapter->pNdis62Common != NULL)
			{
				NdisMIndicateReceiveNetBufferLists(
					Adapter->pNdisCommon->hNdisAdapter,
					pFirstNetBufferList,
					GET_PORT_NUMBER(Adapter),
					packet_index,
					RecvFlags);
			}
		}
		PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	}

	// Return the remaining non-processed RFDs --------------
	for(rfd_index = 0; rfd_index < Num; rfd_index++)
	{
		if(pRfdReturnOK[rfd_index] == FALSE)
		{
			ReturnRFDList(Adapter, pRfd_array[rfd_index]);
		}
	}
	// -------------------------------------------------

	// Return the allocated memory ----------------------
	ReturnGenTempBuffer(Adapter, pGenTempBuffer);
	// ----------------------------------------------
}

VOID
DrvIFIndicateMultiplePackets(
	PADAPTER				Adapter,
	PRT_RFD					pRfd
	)
{	
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(Adapter);
	PRT_SDIO_DEVICE	usbdevice;
	NDIS_HANDLE			NBLPoolHandle;
	PMDL				pMdl = NULL;
	PNET_BUFFER_LIST	pNetBufferList = NULL;
	PNET_BUFFER_LIST	pLastNetBufferList = NULL;
	PNET_BUFFER_LIST	pFirstNetBufferList = NULL;
	u1Byte				index;		
	pu1Byte				pHeader;
	// For WDI architecture ---------------------
	u4Byte				PeerId = 0;
	// ---------------------------------------

	usbdevice = &Adapter->NdisSdioDev;
	NBLPoolHandle = usbdevice->RxNetBufferListPool;

	// TODO: For A-MSDU in AP mode, we shall check each subframe for filtering!!
	if( !N6ReceiveIndicateFilter(Adapter, pRfd)  ||
		N6SDIO_CANNOT_RX(Adapter) )
	{
		ReturnRFDList(Adapter, pRfd);
		return;
	}

#if RX_TCP_SEQ_CHECK
	RxTcpSeqCheck(Adapter, pRfd);
#endif

	for(index = 0; index < pRfd->nTotalSubframe; index++)
	{
		/*if (index > N6_MAX_AMSDU_SUBF_NUM)
		{
			RT_TRACE(COMP_RECV, DBG_SERIOUS, (">AMSDU Max Subframe\n\r"));	
			index = N6_MAX_AMSDU_SUBF_NUM;
			break;
		}*/
		
		pHeader = &pMgntInfo->header[index][0];
		PlatformMoveMemory(pHeader, 
							pRfd->Buffer.VirtualAddress + pRfd->FragOffset, 
							sMacHdrLng);		
		PlatformMoveMemory((pHeader + sMacHdrLng), pRfd->SubframeArray[index], 
						N6_LLC_SIZE);
		#if (N6_PSDUDO_SEQ_NUM == 1)
			// Increase 802.11 header sequence number of every AMSDU sub frame
			Pseudo_Seq_Num+=16;
			SET_80211_HDR_SEQUENCE(pHeader, Pseudo_Seq_Num);
			if (Pseudo_Seq_Num == (16*4000))
				Pseudo_Seq_Num = 0;
		#endif
		
		//
		// Prevent unexpected MDL length requirement, which might cause system 
		// using improper addresses, added by Roger, 2010.05.26.
		//
		if(pRfd->SubframeLenArray[index] <= N6_LLC_SIZE)
		{
			RT_ASSERT(FALSE, ("A-MSDU Subframe length is too small, drop it!!\n"));
			status = NDIS_STATUS_FAILURE;
			break;
		}

		// Allocate MDL of 802.11 header. We use the same header for all AMSDU 
		// subframe. Sequence number are the same for all sub packet.
		// The first MDL must contain 802.11 header and LLC.
		pMdl = NdisAllocateMdl(	N6SDIO_GET_MINIPORT_HANDLE(GetDefaultAdapter(Adapter)), (PVOID)(pHeader), 
								sMacHdrLng+N6_LLC_SIZE);
		
		//
		// Allocate MDLChain. We must filter the header of AMSDU subframe
		//
		NDIS_MDL_LINKAGE(pMdl) 
		= NdisAllocateMdl(	N6SDIO_GET_MINIPORT_HANDLE(GetDefaultAdapter(Adapter)), 
							(PVOID)(pRfd->SubframeArray[index]+N6_LLC_SIZE),
							pRfd->SubframeLenArray[index]-N6_LLC_SIZE);

		// AMSDU debug log 24 bytes header and sub-frame data info
		//PlatformMoveMemory(&nheader[index][0], pHeader, sMacHdrLng);
		//PlatformMoveMemory(&llcheader[index][0], pRfd->SubframeArray[index], sMacHdrLng);
		
		if (pMdl == NULL || NDIS_MDL_LINKAGE(pMdl) == NULL)
		{
			RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate MDL\n"));			
			status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		// Allocate NetBufferList for receive indication, 2006.10.04, by shien chang.
		//
		pNetBufferList = NdisAllocateNetBufferAndNetBufferList(
						N6_GET_RX_NBL_POOL(Adapter),		// PoolHandle
						0,								// ContextSize
						0,								// ContextBackFill
						pMdl,							// MdlChain
						0,								// DataOffset
						pRfd->SubframeLenArray[index]+sMacHdrLng);	//DataLength

		if (pNetBufferList == NULL)
		{
			RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate NetBufferList\n"));
			status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		// Now prepare necessary information such as SourceHandle, OOB data for indication.
		//
		pNetBufferList->SourceHandle = Adapter->pNdisCommon->hNdisAdapter;

		// Filling information in NetBufferList.
		if(FillN6NBLInfo(Adapter, pNetBufferList, pRfd) == FALSE)
		{
			RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to set information to NetBufferList\n"));
			
			status = NDIS_STATUS_FAILURE;
			break;
		}
		
		if(index == 0)
		{
			pFirstNetBufferList = pLastNetBufferList = pNetBufferList;
		}
		else
		{
			NET_BUFFER_LIST_NEXT_NBL(pLastNetBufferList) = pNetBufferList;
			pLastNetBufferList = pNetBufferList;
		}

		MP_SET_PACKET_RFD(pNetBufferList, NULL);
	}	// for(index = 0; index < pRfd->nTotalSubframe; index++)

	if (index != 0)
	{
		RT_ASSERT(Adapter->bRxLocked == TRUE, ("DrvIFIndicateMultiplePackets(): bRxLocked should be TRUE\n"));

		MP_SET_PACKET_RFD(pLastNetBufferList, pRfd);

		PlatformAcquireSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		RT_INC_N_RCV_REF(GetDefaultAdapter(Adapter), index);
		PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		
		if( OS_SUPPORT_WDI(Adapter) )
		{
		// PeerId is 1-based, 0 use to represent failure
		PeerId = WDI_InsertDataInQueue(Adapter, pRfd, pFirstNetBufferList);

		PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
		
		if( PeerId != 0 )
		{
			PlatformReleaseSemaphore(&(pDefaultAdapter->pNdisCommon->RxNotifySemaphore));
		}
		else
		{
			WDI_FreeRxFrame(Adapter, pFirstNetBufferList);
		}
		}
		else
		{			
			PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

			NdisMIndicateReceiveNetBufferLists(
				Adapter->pNdisCommon->hNdisAdapter,
				pFirstNetBufferList,
				GET_PORT_NUMBER(Adapter),
				index,
				0);
		}
		PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	}
	else
	{
		ReturnRFDList(Adapter, pRfd);
	}
	
	if (status != NDIS_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_RECV, DBG_SERIOUS, ("DrvIFIndicateMultiplePackets(): failed to allocate resource! status: %d\n", status));

		FreeN6NBLInfo(Adapter, pNetBufferList);
		
		if(pMdl != NULL && NDIS_MDL_LINKAGE(pMdl) != NULL)
			NdisFreeMdl(NDIS_MDL_LINKAGE(pMdl));

		if(pMdl != NULL)
			NdisFreeMdl(pMdl);

		if(pNetBufferList != NULL)
			NdisFreeNetBufferList(pNetBufferList);
	}
	
}

VOID
DrvIFIndicatePacket(
	PADAPTER				Adapter,
	PRT_RFD					pRfd
	)
{
	NDIS_STATUS			status = NDIS_STATUS_SUCCESS;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(Adapter);
	PRT_SDIO_DEVICE	sdiodevice;
	NDIS_HANDLE			NBLPoolHandle;
	PMDL				pMdl = NULL;
	PMDL				pCurrMdl = NULL, pNextMdl = NULL;
	PNET_BUFFER_LIST	pNetBufferList = NULL;
	PNET_BUFFER_LIST	pNetBufferListToFree = NULL;
	PRT_RFD				pTmpRfd = pRfd;
	ULONG				ReceiveFlags = 0;
	// For WDI architecture ---------------------
	u4Byte				PeerId = 0;
	// ---------------------------------------

	//RTL8187_TODO
	sdiodevice = &Adapter->NdisSdioDev;
	NBLPoolHandle = sdiodevice->RxNetBufferListPool;

	if(pRfd->nTotalSubframe >= 1)
	{
		DrvIFIndicateMultiplePackets(Adapter, pRfd);
		return;
	}

	if( !N6ReceiveIndicateFilter(Adapter, pRfd) ||
		N6SDIO_CANNOT_RX(Adapter) )
	{
		ReturnRFDList(Adapter, pRfd);
		return;
	}

	if(pMgntInfo->bSupportPacketCoalescing)
	{
		GPGetParseRFDInfo(Adapter , pRfd , &(pRfd->D0FilterCoalPktInfo));

		if(N6ReceiveCoalescingFilter(Adapter, pRfd))
		{
			RT_TRACE(COMP_TEST, DBG_LOUD , ("=====> N6ReceiveCoalescingFilter(True)\n") );
			return;
		}
	}

#if RX_TCP_SEQ_CHECK
	RxTcpSeqCheck(Adapter, pRfd);
#endif
	
	do{
		//
		// Allocate MDLChain. 2006.10.04, by shien chang.
		//
		do{
			if (pMdl == NULL)
			{
				pMdl = NdisAllocateMdl(
						N6SDIO_GET_MINIPORT_HANDLE(GetDefaultAdapter(Adapter)), 
						(PVOID)(pTmpRfd->Buffer.VirtualAddress+pTmpRfd->FragOffset),
						pTmpRfd->FragLength);
				pCurrMdl = pMdl;
			}
			else
			{
				NDIS_MDL_LINKAGE(pCurrMdl) = NdisAllocateMdl(
						N6SDIO_GET_MINIPORT_HANDLE(GetDefaultAdapter(Adapter)), 
						(PVOID)(pTmpRfd->Buffer.VirtualAddress+pTmpRfd->FragOffset),
						pTmpRfd->FragLength);
				pCurrMdl = NDIS_MDL_LINKAGE(pCurrMdl);
			}

			if (pCurrMdl == NULL)
			{
				RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate MDL\n"));
				
				status = NDIS_STATUS_FAILURE;
				break;
			}

			pTmpRfd=pTmpRfd->NextRfd;			
		}while(pTmpRfd);

		if (status != NDIS_STATUS_SUCCESS)
		{
			for (pCurrMdl = pMdl;
				pCurrMdl != NULL;
				pCurrMdl = pNextMdl)
			{
				pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
				NdisFreeMdl(pCurrMdl);
			}
			break;
		}

		//
		// Allocate NetBufferList for receive indication, 2006.10.04, by shien chang.
		//
		pNetBufferList = NdisAllocateNetBufferAndNetBufferList(
						N6_GET_RX_NBL_POOL(Adapter),		// PoolHandle
						0,					// ContextSize
						0,					// ContextBackFill
						pMdl,				// MdlChain
						0,					// DataOffset
						pRfd->PacketLength);	//DataLength
		if (pNetBufferList == NULL)
		{
			RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate NetBufferList\n"));

			//
			// Free MDL allocated before.
			//
			for (pCurrMdl = pMdl;
				pCurrMdl != NULL;
				pCurrMdl = pNextMdl)
			{
				pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
				NdisFreeMdl(pCurrMdl);
			}
			
			status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		// Now prepare necessary information such as SourceHandle, OOB data for indication.
		//
		pNetBufferList->SourceHandle = N6SDIO_GET_MINIPORT_HANDLE(Adapter);

		if( FillN6NBLInfo(Adapter, pNetBufferList, pRfd) == FALSE )
		{
			//
			// Free MDL allocated before.
			//
			for (pCurrMdl = pMdl;
				pCurrMdl != NULL;
				pCurrMdl = pNextMdl)
			{
				pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
				NdisFreeMdl(pCurrMdl);
			}
				
			NdisFreeNetBufferList(pNetBufferList);
			
			status = NDIS_STATUS_FAILURE;
			break;
		}

		MP_SET_PACKET_RFD(pNetBufferList, pRfd);
	}while(FALSE);

	
	if(status==NDIS_STATUS_SUCCESS)
	{
		RT_ASSERT(IS_RX_LOCKED(Adapter)  == TRUE, ("DrvIFIndicatePacket(): bRxLocked should be TRUE\n"));

		//
		// Set the status on the packet, either resources or success.
		// <NOTE> We must do tell OS if we are out of Rx resource, 
		// otherwise, in some condition: 
		//		example 1: HCT 12.1 2c_PerformanceBlast).
		//		example 2: too many fragments received.
		// we might stop to receive packets. 2005.12.09, by rcnjko. 
		// 
		if((*GET_NUM_IDLE_RFD(Adapter)  + *GET_NUM_BUSY_RFD(Adapter,RX_MPDU_QUEUE)) > 0)
		{
			ReceiveFlags = 0;
		}
		else
		{ // We don't have enough RFD now.
			ReceiveFlags = NDIS_RECEIVE_FLAGS_RESOURCES;
			pNetBufferListToFree = pNetBufferList;
		}

		PlatformAcquireSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		RT_INC_RCV_REF(GetDefaultAdapter(Adapter));
		PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		
		if( OS_SUPPORT_WDI(Adapter) )
		{
		// PeerId is 1-based, 0 use to represent failure
		PeerId = WDI_InsertDataInQueue(Adapter, pRfd, pNetBufferList);
		
		PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
		
		if( PeerId != 0 )
		{
			PlatformReleaseSemaphore(&(pDefaultAdapter->pNdisCommon->RxNotifySemaphore));
		}
		else
			pNetBufferListToFree = pNetBufferList;
		}
		else
		{			
			PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);

			// Prefast warning C6011: Dereferencing NULL pointer 'Adapter->pNdis62Common'.
			if (Adapter->pNdis62Common != NULL)
			{
				NdisMIndicateReceiveNetBufferLists(
					N6SDIO_GET_MINIPORT_HANDLE(Adapter),
					pNetBufferList,
					GET_PORT_NUMBER(Adapter),
					1,
					ReceiveFlags);
			}
		}
		PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
		//
		// <NOTE> 2005.12.09, by rcnjko.
		// If we had claimed pPacketToFree is NDIS_STATUS_RESOURCES, 
		// we are free to access the it after NdisMIndicateReceivePacket() and
		// MiniportReturnPacket() won't be called.
		// So, we shall release the NDIS_PACKET pPacketToFree and associated NDIS_BUFFER and 
		// then reuse the RFD if we had claimed pPacketToFree is NDIS_STATUS_RESOURCES. 
		//
		// <RJ_TODO> It is time consuming to allocate NDIS_PACKEAT and NDIS_BUFFER dynamically.
		//
		if(pNetBufferListToFree != NULL)
		{
			pTmpRfd = MP_GET_PACKET_RFD(pNetBufferListToFree);

			FreeN6NBLInfo(Adapter, pNetBufferListToFree);
			
			for (pCurrMdl = NET_BUFFER_FIRST_MDL( NET_BUFFER_LIST_FIRST_NB(pNetBufferListToFree) );
				pCurrMdl != NULL;
				pCurrMdl = pNextMdl)
			{
				pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
				NdisFreeMdl(pCurrMdl);
			}
		
			NdisFreeNetBufferList(pNetBufferListToFree);
		
			ReturnRFDList(Adapter, pTmpRfd);

			PlatformAcquireSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
			RT_DEC_RCV_REF(GetDefaultAdapter(Adapter));
			PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		}
	}
	else
	{
		RT_TRACE(COMP_RECV, DBG_SERIOUS, ("DrvIFIndicatePacket(): failed to allocate resource! status: %d\n", status));
		ReturnRFDList(Adapter, pRfd);
	}
}

VOID
DrvIFD0RxIndicatePackets(
	PADAPTER				Adapter,
	PRT_RFD					*pRfd_array,
	u2Byte					Num
	)
{
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(Adapter);
	PRT_SDIO_DEVICE	usbdevice;
	NDIS_HANDLE			NBLPoolHandle;
	PMDL				pMdl = NULL;
	PMDL				pCurrMdl = NULL, pNextMdl = NULL;
	PNET_BUFFER_LIST	pNetBufferList = NULL;
	PNET_BUFFER_LIST	pLastNetBufferList = NULL;
	PNET_BUFFER_LIST	pFirstNetBufferList = NULL;
	PRT_RFD				pTmpRfd = NULL;
	ULONG				RecvFlags = 0;

	u2Byte				rfd_index = 0;
	u2Byte				packet_index = 0;
	PRT_RFD				pCurRfd = NULL;


	// Indicate if the RFD is returned or will be indicated to OS ---
	pu1Byte pRfdReturnOK = NULL;		
	PRT_GEN_TEMP_BUFFER pGenTempBuffer = NULL;
	// --------------------------------------------------
	
	// Resource Management --------------------
	u4Byte 			ResourceMointor = 0;
	const u4Byte		RESOURCE_MDL = BIT1;
	const u4Byte		RESOURCE_NBL = BIT2;
	const u4Byte		RESOURCE_INFO = BIT3;
	// ---------------------------------------

	// For WDI architecture ---------------------
	u4Byte			PeerId;
	BOOLEAN			bDataInQueue[MAX_PEER_NUM] = {0};
	u2Byte			index = 0;
	// ---------------------------------------
	
	if(Num==0 || pRfd_array==NULL)
		return;

	// Check if the RFD is returned or indicated to the OS ------
	pGenTempBuffer = GetGenTempBuffer(Adapter, Num);
	pRfdReturnOK = pGenTempBuffer->Buffer.Ptr;
	PlatformZeroMemory(pRfdReturnOK, Num);
	// -------------------------------------------------
	
	usbdevice = &Adapter->NdisSdioDev;
	NBLPoolHandle = usbdevice->RxNetBufferListPool;

	for(rfd_index = 0; rfd_index < Num; rfd_index++)
	{
		pCurRfd = pRfd_array[rfd_index];

 
		// Check if the resource is clear -----------------------------------
		RT_ASSERT(ResourceMointor == 0, ("Error: Resource is Not Clear!\n"));
		// ------------------------------------------------------------
		
		pMdl = NULL;
		pTmpRfd = pCurRfd;
	
		//
		// Allocate MDLChain. 2006.10.04, by shien chang.
		//
		do{
			if (pMdl == NULL)
			{
				pMdl = NdisAllocateMdl(
						N6SDIO_GET_MINIPORT_HANDLE(GetDefaultAdapter(Adapter)), 
						(PVOID)(pCurRfd->Buffer.VirtualAddress+pCurRfd->FragOffset),
						pCurRfd->FragLength);
				pCurrMdl = pMdl;
			}
			else
			{
				NDIS_MDL_LINKAGE(pCurrMdl) = NdisAllocateMdl(
						N6SDIO_GET_MINIPORT_HANDLE(GetDefaultAdapter(Adapter)), 
						(PVOID)(pTmpRfd->Buffer.VirtualAddress+pTmpRfd->FragOffset),
						pTmpRfd->FragLength);
				pCurrMdl = NDIS_MDL_LINKAGE(pCurrMdl);
					
			}

			if (pCurrMdl == NULL)
			{
				RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate MDL\n"));
				goto POST_PROCESS;
			}
			else
			{
				ResourceMointor |= RESOURCE_MDL;
			}

			pTmpRfd=pTmpRfd->NextRfd;
		}while(pTmpRfd);

		//
		// Allocate NetBufferList for receive indication, 2006.10.04, by shien chang.
		//
		pNetBufferList = NdisAllocateNetBufferAndNetBufferList(
						N6_GET_RX_NBL_POOL(Adapter),		// PoolHandle
						0,					// ContextSize
						0,					// ContextBackFill
						pMdl,				// MdlChain
						0,					// DataOffset
						pCurRfd->PacketLength);	//DataLength
		if (pNetBufferList == NULL)
		{
			RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to allocate NetBufferList\n"));
			goto POST_PROCESS;
		}
		else
		{
			ResourceMointor |= RESOURCE_NBL;
		}

		//
		// Now prepare necessary information such as SourceHandle, OOB data for indication.
		//
		pNetBufferList->SourceHandle = N6SDIO_GET_MINIPORT_HANDLE(Adapter);

		// Filling information in NetBufferList.
		if(FillN6NBLInfo(Adapter, pNetBufferList, pCurRfd) == FALSE)
		{
			RT_TRACE(COMP_RECV, DBG_SERIOUS, ("Failed to set information to NetBufferList\n"));
			goto POST_PROCESS;
		}
		else
		{
			ResourceMointor |= RESOURCE_INFO;
		}

		if(pFirstNetBufferList == NULL)
		{
			pFirstNetBufferList = pLastNetBufferList = pNetBufferList;
		}
		else
		{
			NET_BUFFER_LIST_NEXT_NBL(pLastNetBufferList) = pNetBufferList;
			pLastNetBufferList = pNetBufferList;
		}

		// The RFD will be indicated to the OS and returned -------
		MP_SET_PACKET_RFD(pNetBufferList, pCurRfd);
		pRfdReturnOK[rfd_index] = TRUE;
		// ------------------------------------------------
		
		packet_index++;

		// Clean the Resource Mointor ---------
		ResourceMointor = 0;
		// ---------------------------------

		if( OS_SUPPORT_WDI(Adapter) )
		{
		// PeerId is 1-based, 0 use to represent failure
		PeerId = WDI_InsertDataInQueue(Adapter, pCurRfd, pFirstNetBufferList);

		if( PeerId != 0 )
			bDataInQueue[PeerId - 1] = TRUE;
		else
			WDI_FreeRxFrame(Adapter, pFirstNetBufferList);

		pFirstNetBufferList = NULL;
	}
	}
POST_PROCESS:

	// Free information for NBL
	if( ResourceMointor & RESOURCE_INFO )
	{
		FreeN6NBLInfo(Adapter, pNetBufferList);
	}

	// Free MDL allocated before.
	if(ResourceMointor & RESOURCE_MDL)
	{
		for (pCurrMdl = pMdl; pCurrMdl != NULL; pCurrMdl = pNextMdl)
		{
			pNextMdl = NDIS_MDL_LINKAGE(pCurrMdl);
			NdisFreeMdl(pCurrMdl);
		}
	}

	// Free NBL allocated before
	if(ResourceMointor & RESOURCE_NBL)
	{
		NdisFreeNetBufferList(pNetBufferList);
	}

	// Indicate as much packets as we can.
	if(packet_index != 0)
	{
		RT_ASSERT(IS_RX_LOCKED(Adapter) == TRUE, ("DrvIFIndicatePacket(): bRxLocked should be TRUE\n"));

		PlatformAcquireSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);
		RT_INC_N_RCV_REF(GetDefaultAdapter(Adapter), packet_index);
		PlatformReleaseSpinLock(Adapter, RT_RX_REF_CNT_SPINLOCK);

		PlatformReleaseSpinLock(Adapter, RT_RX_SPINLOCK);
				
		if( OS_SUPPORT_WDI(Adapter) )
		{			
		for(index = 0; index < MAX_PEER_NUM; index++)
		{
			if( bDataInQueue[index] == TRUE )
			{
				//Rx notify thread will check all queue, so we only set event one time.
				PlatformReleaseSemaphore(&(pDefaultAdapter->pNdisCommon->RxNotifySemaphore));
				break;
			}
		}
		}
		else
		{
			RecvFlags = NDIS_CURRENT_IRQL()?NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL:0;
			// Prefast warning C6011: Dereferencing NULL pointer 'Adapter->pNdis62Common'.
			if (Adapter->pNdis62Common != NULL)
			{
				NdisMIndicateReceiveNetBufferLists(
					Adapter->pNdisCommon->hNdisAdapter,
					pFirstNetBufferList,
					GET_PORT_NUMBER(Adapter),
					packet_index,
					RecvFlags
					);
			}
		}
		PlatformAcquireSpinLock(Adapter, RT_RX_SPINLOCK);
	}


	// Return the remaining non-processed RFDs --------------
	for(rfd_index = 0; rfd_index < Num; rfd_index++)
	{
		if(pRfdReturnOK[rfd_index] == FALSE)
		{
			ReturnRFDList(Adapter, pRfd_array[rfd_index]);
		}
	}
	// -------------------------------------------------

	// Return the allocated memory ----------------------
	ReturnGenTempBuffer(Adapter, pGenTempBuffer);
	// ----------------------------------------------
	
	FunctionOut(COMP_RECV);
	return;
}

VOID
DrvIFCompletePacket(
	PADAPTER		Adapter,
	PRT_TCB			pTcb,
	RT_STATUS		status
	)
{
	PRT_SDIO_DEVICE device = &(Adapter->NdisSdioDev);
	PNET_BUFFER_LIST pNBL = (PNET_BUFFER_LIST)(pTcb->Reserved);
	PNET_BUFFER_LIST	pCurrNetBufferList, pNextNetBufferList;

        if(!OS_SUPPORT_WDI(Adapter))
        {
		PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);	
		
		//
		// For pending NBL issue.
		//
		RT_ASSERT(GetDefaultAdapter(Adapter)->MgntInfo.OutstandingNdisPackets > 0, 
			("DrvIFCompletePacket(): OutstandingNdisPackets not greater than 0.\n"));

		if (pNBL != NULL && RT_NBL_GET_REF_CNT(pNBL) != 0)
		{
			RT_NBL_DECREASE_REF_CNT(pNBL);

			if (RT_NBL_GET_REF_CNT(pNBL) == 0)
			{
				GetDefaultAdapter(Adapter)->MgntInfo.OutstandingNdisPackets--;
				GetDefaultAdapter(Adapter)->MgntInfo.CompleteFlag = 0;	
				for(pCurrNetBufferList = pNBL;
					pCurrNetBufferList != NULL;
					pCurrNetBufferList = pNextNetBufferList)
				{
					pNextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(pCurrNetBufferList);
					NET_BUFFER_LIST_STATUS(pCurrNetBufferList) = NDIS_STATUS_SUCCESS;
				}			
				PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
				NdisMSendNetBufferListsComplete(
					device->hNdisAdapter,
					pNBL,
					((NDIS_CURRENT_IRQL()==DISPATCH_LEVEL) ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0));
				PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
				pTcb->Reserved = NULL;
			}
		}

		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
        }
        else
	{
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	if (pNBL != NULL && RT_WDI_NBL_GET_REF_CNT(pNBL) != 0)
	{
		RT_WDI_NBL_DECREASE_REF_CNT(pNBL);

		if (RT_WDI_NBL_GET_REF_CNT(pNBL) == 0)
		{
			PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
			WDI_CompletePacket(Adapter, pNBL);
			PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
			pTcb->Reserved = NULL;
		}
	}
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
	}
}

VOID
DrvIFIndicateScanStart(
	PADAPTER		Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("==> DrvIFIndicateScanStart()\n"));

	if (pNdisCommon->bToIndicateScanComplete == FALSE)
	{
		pNdisCommon->bToIndicateScanComplete = TRUE;
	}
	
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("<== DrvIFIndicateScanStart()\n"));
}

VOID
DrvIFIndicateScanComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	
	RT_TRACE(COMP_SCAN, DBG_LOUD, ("==> DrvIFIndicateScanComplete(): status = 0x%x\n", status));

	//
	// In NDIS6, we need tell the upper layer driver that scan operation
	// is completed.
	//
	if (pNdisCommon->bToIndicateScanComplete)
	{
		RT_TRACE(COMP_INDIC, DBG_LOUD, ("Indicate a scan complete event.\n"));
		N6IndicateScanComplete(Adapter, status);

		if( pMgntInfo->bCheckScanTime == TRUE )
			pMgntInfo->bCheckScanTime = FALSE;

		pNdisCommon->bToIndicateScanComplete = FALSE;		
	}

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("<== DrvIFIndicateScanComplete()\n"));
}

VOID
DrvIFIndicateConnectionStart(
	PADAPTER		Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateConnectionStart()\n"));

	//
	// In NDIS6, we need to indicate the upper layer driver in starting connection.
	//
	N6PushIndicateStateMachine(Adapter, N6_STATE_CONNECT_START, 0);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateConnectionStart()\n"));
}

VOID
DrvIFIndicateConnectionComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateConnectionComplete(): status = %d\n", status));

	if(OS_SUPPORT_WDI(Adapter))
	{
	WDI_IndicateConnectionComplete(Adapter, status);
	}

	// In NDIS6, we need to indicate the upper layer driver when a connection request completed.
	N6PushIndicateStateMachine(Adapter, N6_STATE_CONNECT_COMPLETE, status);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateConnectionComplete()\n"));
}

VOID
DrvIFIndicateAssociationStart(
	PADAPTER		Adapter
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateAssociationStart()\n"));

	// In NDIS6, we need indicate the upper layer driver when association is starting.
	N6PushIndicateStateMachine(Adapter, N6_STATE_ASOC_START, 0);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateAssociationStart()\n"));
}

VOID
DrvIFIndicateAssociationComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateAssociationComplete()\n"));

	if(OS_SUPPORT_WDI(Adapter))
	{
	WDI_IndicateAssociationComplete(Adapter, status);
	}

	// In NDIS6, we need indicate the upper layer driver when association request is completed.
	N6PushIndicateStateMachine(Adapter, N6_STATE_ASOC_COMPLETE, status);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateAssociationComplete()\n"));
}

VOID
DrvIFIndicateDisassociation(
	PADAPTER	Adapter,
	u2Byte		reason,
	pu1Byte		pAddr
)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateDisassociation()\n"));

	if(OS_SUPPORT_WDI(Adapter))
	{
	WDI_IndicateDisassociation(Adapter, reason, pAddr);
	}
    
	pMgntInfo->DisconnectCount++;
	N6PushIndicateStateMachine(Adapter, N6_STATE_DISASOC, reason);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateDisassociation()\n"));
}

VOID
DrvIFIndicateRoamingStart(
	PADAPTER		Adapter
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateRoamingStart()\n"));

	N6PushIndicateStateMachine(Adapter, N6_STATE_ROAM_START, 0);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateRoamingStart()\n"));
}

VOID
DrvIFIndicateRoamingComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
	)
{
	PRT_NDIS6_COMMON	pNdisCommon = Adapter->pNdisCommon;
	PMGNT_INFO	pMgntInfo=&Adapter->MgntInfo;
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateRoamingComplete()\n"));
	if(OS_SUPPORT_WDI(Adapter))
	{
		WDI_IndicateRoamingComplete(Adapter);
	}
	//for UniWill Roaming case by Maddest, 070823
	pMgntInfo->JoinRetryCount=0;
	//pMgntInfo->LinkRetryforRoaming=0;
	pMgntInfo->RoamingCount++;
	N6PushIndicateStateMachine(Adapter, N6_STATE_ROAM_COMPLETE, status);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateRoamingComplete()\n"));
}

VOID
DrvIFIndicateCurrentPhyStatus(
	PADAPTER		Adapter
	)
{
	PRT_NDIS6_COMMON	pExtNdisCommon = NULL;
	PADAPTER			pAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO			pDefaultMgntInfo = &pAdapter->MgntInfo;
	PMGNT_INFO			pExtMgntInfo;
	PRT_NDIS6_COMMON	pDefaultNdisCommon = pAdapter->pNdisCommon;
	u1Byte				i;
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateCurrentPhyStatus()\n"));

	if(OS_SUPPORT_WDI(Adapter))
	{
	WDI_IndicateCurrentPhyStatus(Adapter);
	}

	//
	// Ndis6 can assign multiple phy IDs to one NIC for each phy, thus we should indicate the state for each phy ID.
	// By Bruce, 2009-02-20
	//
	while(pAdapter != NULL)
	{
		pExtNdisCommon = pAdapter->pNdisCommon;
		pExtMgntInfo = &pAdapter->MgntInfo;
		pExtMgntInfo->eSwRfPowerState = pDefaultMgntInfo->eSwRfPowerState;
		pExtMgntInfo->RfOffReason = pDefaultMgntInfo->RfOffReason;

		pExtNdisCommon->eRfPowerStateToSet = pDefaultNdisCommon->eRfPowerStateToSet;		

		// If the uPhyId member is set to DOT11_PHY_ID_ANY, the NDIS_STATUS_DOT11_PHY_STATE_CHANGED 
		// indication applies to all PHYs in the msDot11SupportedPhyTypes MIB object.
		N6IndicateCurrentPhyPowerState(pAdapter, DOT11_PHY_ID_ANY);

		pAdapter = GetNextExtAdapter(pAdapter);
	}

	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateCurrentPhyStatus()\n"));

}

VOID
DrvIFIndicateIncommingAssociationStart(
	PADAPTER		Adapter
	)
{
	if(OS_SUPPORT_WDI(Adapter))
	{
	//<Roger_TODO>
}
	else
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateIncommingAssociationStart()\n"));
		N6PushIndicateStateMachine(Adapter, RT_STA_STATE_AP_INCOMING_ASOC_STARTED, 0);
		RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateIncommingAssociationStart()\n"));
	}
}

VOID
DrvIFIndicateIncommingAssociationComplete(
	PADAPTER		Adapter,
	u2Byte			status
	)
{
	if(OS_SUPPORT_WDI(Adapter))
	{
	RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("Indicate association rsp sent with status = 0x%X, association rsp len: %u\n", status, Adapter->MgntInfo.pCurrentSta->AP_SendAsocRespLength));
	if(Adapter->MgntInfo.pCurrentSta->AP_SendAsocRespLength)
	{// Indicate task completion event only if we have sent an association response			
		WdiExt_IndicateApAssocRspSent(Adapter, status);
	}
}
    else
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateIncommingAssociationComplete()\n"));
		N6PushIndicateStateMachine(Adapter, RT_STA_STATE_AP_INCOMING_ASOC_COMPLETE, status);
		RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateIncommingAssociationComplete()\n"));
	}
}

VOID
DrvIFIndicateIncommingAssocReqRecv(
	PADAPTER		Adapter
	)
{

	if(OS_SUPPORT_WDI(Adapter))
	{
	RT_WLAN_STA		*sta = Adapter->MgntInfo.pCurrentSta;
			
	WdiExt_IndicateApAssocReqReceived(
		Adapter, 
		sta->AP_RecvAsocReqLength,
		sta->AP_RecvAsocReq);
}
	else
	{
		RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateIncommingAssocReqRecv()\n"));
		N6PushIndicateStateMachine(Adapter, RT_STA_STATE_AP_INCOMING_ASOC_REQ_RECVD, 0);
		RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateIncommingAssocReqRecv()\n"));
	}
}

VOID
DrvIFDeleteDatapathPeer(
	IN	PADAPTER	pAdapter,
	IN	pu1Byte		pAddr
	)
{
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFDeleteDatapathPeer()\n"));

	WDI_DeleteDatapathPeer(pAdapter, pAddr);

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFDeleteDatapathPeer()\n"));
}

VOID
DrvIFCheckTxCredit(
	PADAPTER		pAdapter
	)
{
	WDI_TxCreditCheck(pAdapter);
}

VOID
DrvIFIndicateDataInQueue(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					ExTid,
	IN	u2Byte					PeerId,
	IN	RT_RX_INDICATION_LEVEL	level
	)
{
	WDI_NotifyDataInQueue(pAdapter, ExTid, PeerId, level);
}

VOID
DrvIFIndicateRoamingNeeded(
	PADAPTER				pAdapter,
	RT_PREPARE_ROAM_TYPE	IndicationReason
	)
{
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateRoamingNeeded()\n"));

	WDI_IndicateRoamingNeeded(pAdapter, IndicationReason);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateRoamingNeeded()\n"));
}

VOID
DrvIFIndicateLinkStateChanged(
	PADAPTER		pAdapter,
	BOOLEAN			bForceLinkQuality,
	u1Byte			ucLinkQuality
	)
{
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateLinkStateChanged()\n"));

	WDI_IndicateLinkStateChanged(pAdapter, bForceLinkQuality, ucLinkQuality);
	
	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateLinkStateChanged()\n"));
}

VOID
DrvIFIndicateFWStalled(
	PADAPTER		Adapter
	)
{
	RT_TRACE(COMP_MLME, DBG_LOUD, ("==> DrvIFIndicateFWStalled()\n"));

	if(OS_SUPPORT_WDI(Adapter))
	{
		if(MgntLinkStatusQuery(Adapter)==RT_MEDIA_CONNECT)
		{
			//if(TODO: check TX hang here)
			{
				WDI_IndicateFWStalled(Adapter);
			}
		}
	}

	RT_TRACE(COMP_MLME, DBG_LOUD, ("<== DrvIFIndicateFWStalled()\n"));
}

