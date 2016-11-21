#include "Mp_Precomp.h"


NDIS_STATUS
N62CQueryInterruptModerationSettings(
	IN PADAPTER			Adapter,
	IN PVOID				InformationBuffer,
	IN ULONG			InformationBufferLength
    )
{
	
	NDIS_STATUS                 ndisStatus = NDIS_STATUS_SUCCESS;
	PNDIS_INTERRUPT_MODERATION_PARAMETERS      intModParams;
	
	UNREFERENCED_PARAMETER(Adapter);
	
	NdisZeroMemory(InformationBuffer, InformationBufferLength);	

	intModParams = (PNDIS_INTERRUPT_MODERATION_PARAMETERS)InformationBuffer;      

	intModParams->Header.Type=NDIS_OBJECT_TYPE_DEFAULT;
	intModParams->Header.Revision=NDIS_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
	intModParams->Header.Size=sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS);
	
	intModParams->Flags = 0;
	intModParams->InterruptModeration = NdisInterruptModerationNotSupported;

	return ndisStatus;
}

NDIS_STATUS
N62CQueryLinkParameters(
	IN PADAPTER			Adapter,
	IN PVOID				InformationBuffer,
	IN ULONG			InformationBufferLength
    )
{
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	NDIS_STATUS                 		ndisStatus = NDIS_STATUS_SUCCESS;
	PNDIS_LINK_PARAMETERS		linkParams;

	
	NdisZeroMemory(InformationBuffer, InformationBufferLength);

	linkParams = (PNDIS_LINK_PARAMETERS)InformationBuffer;

	linkParams->Header.Type=NDIS_OBJECT_TYPE_DEFAULT;
	linkParams->Header.Revision=NDIS_LINK_PARAMETERS_REVISION_1;
	linkParams->Header.Size=sizeof(NDIS_LINK_PARAMETERS);

	linkParams->MediaDuplexState = MediaDuplexStateHalf;
	linkParams->XmitLinkSpeed = MgntActQuery_802_11_TX_RATES(Adapter)*1000000/2;
	linkParams->RcvLinkSpeed = pMgntInfo->HighestOperaRate*1000000/2;
	linkParams->PauseFunctions = NdisPauseFunctionsUnsupported;
	linkParams->AutoNegotiationFlags = NDIS_LINK_STATE_DUPLEX_AUTO_NEGOTIATED;

	return ndisStatus;

}

NDIS_STATUS
N62CQueryAdditionalIE(
	IN 	PADAPTER				Adapter,
	OUT PDOT11_ADDITIONAL_IE	AdditionalIe,
	IN	u4Byte					InformationBufferLength,
	OUT	pu4Byte					BytesWritten,
	OUT	pu4Byte					BytesNeeded	
	)
{
	u4Byte AdditionalBeaconIESize = 0;
	u4Byte AdditionalResponseIESize = 0;

	ULONG requiredSize = 0;
	ULONG beaconIeOffset = sizeof(DOT11_ADDITIONAL_IE);
	ULONG responseIeOffset = 0;
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	
	do
	{
		//
		// Query the size of the additional IEs.
		//
		MgntActQuery_AdditionalBeaconIE(Adapter, NULL, &AdditionalBeaconIESize);
		MgntActQuery_AdditionalProbeRspIE(Adapter, NULL, &AdditionalResponseIESize);

		RT_TRACE(COMP_OID_SET, DBG_LOUD, 
			("N62CQueryAdditionalIE: BeaconIELen = %u, ProbeRspIELen = %u.\n", AdditionalBeaconIESize, AdditionalResponseIESize));

		
		// 
		// add required size for beacon IEs
		//
		if (RtlULongAdd(beaconIeOffset, AdditionalBeaconIESize, &responseIeOffset) != NDIS_STATUS_SUCCESS)
		{
			// 
			// This shall not happen because we validated the IEs before
			//
			Status = NDIS_STATUS_INVALID_DATA;
			break;
		}

		// 
		// add required size for response IEs
		//
		if (RtlULongAdd(responseIeOffset, AdditionalResponseIESize, &requiredSize) != NDIS_STATUS_SUCCESS)
		{
			// 
			// This shall not happen because we validated the IEs before
			//
			Status = NDIS_STATUS_INVALID_DATA;
			break;
		}

		if (InformationBufferLength < requiredSize)
		{
			// 
			// the buffer is not big enough
			//
			*BytesNeeded = (requiredSize);
			Status=NDIS_STATUS_INVALID_LENGTH;
			break;
		}

		// 
		// the buffer is big enough, copy the IEs
		//
		MgntActQuery_AdditionalBeaconIE(Adapter, 
			(pu1Byte)AdditionalIe + beaconIeOffset, 
			(pu4Byte)&AdditionalBeaconIESize);
		AdditionalIe->uBeaconIEsLength = AdditionalBeaconIESize;
		AdditionalIe->uBeaconIEsOffset = beaconIeOffset;
		
		MgntActQuery_AdditionalProbeRspIE(Adapter, 
			(pu1Byte)AdditionalIe + responseIeOffset,
			(pu4Byte)&AdditionalResponseIESize);
		AdditionalIe->uResponseIEsLength = AdditionalResponseIESize;
		AdditionalIe->uResponseIEsOffset = responseIeOffset;		

		*BytesWritten = requiredSize;

		// assign NDIS header
		N6_ASSIGN_OBJECT_HEADER(
		    AdditionalIe->Header,
		    NDIS_OBJECT_TYPE_DEFAULT,
		    DOT11_ADDITIONAL_IE_REVISION_1,
		    sizeof(DOT11_ADDITIONAL_IE)
		    );		
	}while(FALSE);

	return Status;
	
}

NDIS_STATUS
N62CApEnumPeerInfo(
	IN	PADAPTER				Adapter,
	OUT	PDOT11_PEER_INFO_LIST	PeerListInfo,
	IN	u4Byte					InformationBufferLength,
	OUT	pu4Byte 					BytesWritten,
	OUT	pu4Byte 					BytesNeeded
	)
{
	PRT_NDIS62_COMMON	pNdis62Common = Adapter->pNdis62Common;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;	
	ULONG ulBufRequired;
	u4Byte i;
	PRT_WLAN_STA pCurrAsocEntry;
	PDOT11_PEER_INFO pPeerInfo;
	u2Byte staEntryCount=0;

	//
	// We report only "ASSOCIATED" entries up.
	// Note that if an STA is disassociated by OID_DOT11_DISASSOCIATE_PEER_REQUEST,
	// its corresponding entry shall not be reported up to NDIS.
	//
	do
	{
		//
		// Determine buffer length required.
		//
		for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
			if(pMgntInfo->AsocEntry[i].bUsed && pMgntInfo->AsocEntry[i].bAssociated)
				staEntryCount++;
		ulBufRequired = staEntryCount * sizeof(DOT11_PEER_INFO) +
						sizeof(DOT11_PEER_INFO_LIST);

		//
		// Verify given buffer length.
		//
		if(InformationBufferLength < ulBufRequired) 
		{
			*BytesNeeded = ulBufRequired;
			*BytesWritten = 0;
			// Prefast warning ignore for false positive
#pragma warning( disable:6273 )
			RT_TRACE(COMP_OID_QUERY, DBG_LOUD, 
				("<=== Query OID_DOT11_ENUM_PEER_INFO: BytesNeeded: %u !!!\n", BytesNeeded));
			return NDIS_STATUS_INVALID_LENGTH;
		}

		//
		// Assign NDIS header.
		//
		N6_ASSIGN_OBJECT_HEADER(
			PeerListInfo->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_PEER_INFO_LIST_REVISION_1,
			sizeof(DOT11_PEER_INFO_LIST)
		);

		//
		// Fill up each entry.
		//
		PeerListInfo->uNumOfEntries = 0;
		PeerListInfo->uTotalNumOfEntries = staEntryCount;
		
		for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
		{	
			//
			// Report each STA that has been successfully authenticated to NDIS.
			//
			pCurrAsocEntry = &(pMgntInfo->AsocEntry[i]);
			if(pCurrAsocEntry->bUsed && pCurrAsocEntry->bAssociated > 0)
			{
				pPeerInfo = &PeerListInfo->PeerInfo[PeerListInfo->uNumOfEntries];
				PeerListInfo->uNumOfEntries++;
				PlatformZeroMemory(pPeerInfo, sizeof(DOT11_PEER_INFO));

				RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, 
					"Report STA:", pCurrAsocEntry->MacAddr);
				RT_TRACE(COMP_MLME, DBG_LOUD, 
					("bAssociated = %u.\n", pCurrAsocEntry->bAssociated));

				//
				// Fields for ALL authenticated STAs.
				//
				
				//mac addr
				PlatformMoveMemory(pPeerInfo->MacAddress, 
					pCurrAsocEntry->MacAddr, 
					6);
				
				// association state
				pPeerInfo->AssociationState = (pCurrAsocEntry->bAssociated) ? 
					dot11_assoc_state_auth_assoc :
					dot11_assoc_state_auth_unassoc;

				// power mode
				pPeerInfo->PowerMode = (pCurrAsocEntry->bPowerSave) ?
					dot11_power_mode_powersave :
					dot11_power_mode_active;

				// association ID
				pPeerInfo->usAssociationID = 0xFFFF;

				//
				// Fields only for STAs that are associated.
				//
				if(pCurrAsocEntry->bAssociated)
				{
					// capability
					pPeerInfo->usCapabilityInformation = 
						(USHORT)pCurrAsocEntry->Capability;

					// auth/cipher, report that of the SoftAP instead of the STA.
					{
						RT_AUTH_MODE authmode;
						PRT_SECURITY_T	pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
						
						MgntActQuery_802_11_AUTHENTICATION_MODE( Adapter, &authmode );
						pPeerInfo->AuthAlgo = N6CAuthModeToDot11(&authmode);

						pPeerInfo->MulticastCipherAlgo = 
							N6CEncAlgorithmToDot11(&(pSecInfo->GroupEncAlgorithm));
						pPeerInfo->UnicastCipherAlgo = 
							N6CEncAlgorithmToDot11(&(pSecInfo->PairwiseEncAlgorithm));
					}

					// TODO: WPS enabled?
					pPeerInfo->bWpsEnabled = FALSE;

					// listen interval
					pPeerInfo->usListenInterval = pCurrAsocEntry->usListenInterval;
					
					// supported rates
					PlatformMoveMemory(
						pPeerInfo->ucSupportedRates,
						pCurrAsocEntry->bdSupportRateEXBuf,
						pCurrAsocEntry->bdSupportRateEXLen
						);

					// association ID
					pPeerInfo->usAssociationID = pCurrAsocEntry->AID;

					// association up time
					pPeerInfo->liAssociationUpTime.QuadPart = 
						pCurrAsocEntry->AssociationUpTime;
					//PlatformMoveMemory(&pPeerInfo->liAssociationUpTime, 
					//	&pCurrAsocEntry->AssociationUpTime,
					//	sizeof(LARGE_INTEGER));

					// TODO: Fill DOT11_PEER_STATISTICS.

				}
			}
		}
			
		PeerListInfo->uTotalNumOfEntries = staEntryCount;
		*BytesWritten = ulBufRequired; 

	} while (FALSE);

	return Status;
}

NDIS_STATUS
N62CQuery_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	)
{
	PDOT11_PHY_ID_LIST	pPhyIdList = (PDOT11_PHY_ID_LIST)InformationBuffer;

	*BytesNeeded = sizeof(DOT11_PHY_ID_LIST);
	if ( InformationBufferLength < *BytesNeeded )
	{
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	N6_ASSIGN_OBJECT_HEADER(
		pPhyIdList->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_PHY_ID_LIST_REVISION_1,
		sizeof(DOT11_PHY_ID_LIST));

	pPhyIdList->uNumOfEntries = 1;
	pPhyIdList->uTotalNumOfEntries = 1;
	pPhyIdList->dot11PhyId[0] = 1;

	*BytesWritten = *BytesNeeded;
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
N62C_QUERY_OID_DOT11_WPS_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	BOOLEAN			bEnabled = FALSE;
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	RT_TRACE(COMP_OID_QUERY | COMP_AP, DBG_LOUD, ("===>Query AP mode OID_DOT11_WPS_ENABLED: %d\n", pTargetAdapter->pNdis62Common->bWPSEnable));

	if ( InformationBufferLength < sizeof(BOOLEAN) )
	{
		*BytesWritten = 0;
		*BytesNeeded = sizeof(BOOLEAN);
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}

	bEnabled = pTargetAdapter->pNdis62Common->bWPSEnable;	
	PlatformMoveMemory(InformationBuffer, &bEnabled, sizeof(BOOLEAN));
	*BytesWritten = sizeof(BOOLEAN);
	
	RT_TRACE(COMP_OID_QUERY | COMP_AP, DBG_LOUD, ("<===Query AP mode OID_DOT11_WPS_ENABLED: %d\n", bEnabled));

	return ndisStatus;
}

NDIS_STATUS
N62C_QUERY_OID_DOT11_ADDITIONAL_IE(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query AP mode OID_DOT11_ADDITIONAL_IE:\n"));
	
	ndisStatus = N62CQueryAdditionalIE(
			pTargetAdapter,
			(PDOT11_ADDITIONAL_IE)InformationBuffer,
			InformationBufferLength,
			BytesWritten,
			BytesNeeded
		);
		
	return ndisStatus;
}

NDIS_STATUS
N62C_QUERY_OID_DOT11_ENUM_PEER_INFO(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	RT_TRACE(COMP_OID_QUERY, DBG_LOUD, ("Query AP mode OID_DOT11_ENUM_PEER_INFO:\n"));

	// let the strong type API check buffer size
	// because the size of association info list is not known in advance
	ndisStatus = N62CApEnumPeerInfo(
			pTargetAdapter, 
			(PDOT11_PEER_INFO_LIST)InformationBuffer,
			InformationBufferLength,
			BytesWritten,
			BytesNeeded
		);			
		
	return ndisStatus;
}

NDIS_STATUS
N62C_QUERY_OID_DOT11_AVAILABLE_CHANNEL_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG			ulInfo = 0;
	PVOID			pInfo = (PVOID) &ulInfo;
	ULONG			ulInfoLen = sizeof(ulInfo);
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	*BytesWritten = ulInfoLen;
	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
		
	return ndisStatus;
}

NDIS_STATUS
N62C_QUERY_OID_DOT11_AVAILABLE_FREQUENCY_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	ULONG			ulInfo = 0;
	PVOID			pInfo = (PVOID) &ulInfo;
	ULONG			ulInfoLen = sizeof(ulInfo);
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	*BytesWritten = ulInfoLen;
	PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
		
	return ndisStatus;
}

NDIS_STATUS
N62C_QUERY_OID_PM_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	NDIS_PM_PARAMETERS	PMParameters;
	NDIS_PM_CAPABILITIES	PmCapabilities;

	ULONG			ulInfo = 0;
	PVOID			pInfo = (PVOID) &ulInfo;
	ULONG			ulInfoLen = sizeof(ulInfo);
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);
				
	if(InformationBufferLength < sizeof(NDIS_PM_PARAMETERS))
	{
		*BytesNeeded = sizeof(NDIS_PM_PARAMETERS);
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}			
				
	FillPmCapabilities(pTargetAdapter, &PmCapabilities);

#if NDIS_SUPPORT_NDIS630
	PMParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	PMParameters.Header.Revision = NDIS_PM_CAPABILITIES_REVISION_2;
	PMParameters.Header.Size = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_2;
#else
	PMParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	PMParameters.Header.Revision = NDIS_PM_CAPABILITIES_REVISION_1;
	PMParameters.Header.Size = NDIS_SIZEOF_NDIS_PM_CAPABILITIES_REVISION_1;
#endif
			
	PMParameters.EnabledWoLPacketPatterns = PmCapabilities.SupportedWoLPacketPatterns;
	PMParameters.EnabledProtocolOffloads = PmCapabilities.SupportedProtocolOffloads;

		PMParameters.WakeUpFlags = 
					/*NDIS_PM_WAKE_ON_LINK_CHANGE_ENABLED |
					NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_ENABLED |*/
					0;

#if NDIS_SUPPORT_NDIS630
	PMParameters.MediaSpecificWakeUpEvents = PmCapabilities.MediaSpecificWakeUpEvents;
#endif
				
	pInfo = (PVOID)&PMParameters;
	ulInfoLen = sizeof(PMParameters);
				
	if(ulInfoLen <= InformationBufferLength)
	{
		// Copy result into InformationBuffer
		if(ulInfoLen)
		{
			*BytesWritten = ulInfoLen;
			PlatformMoveMemory(InformationBuffer, pInfo, ulInfoLen);
		}
	}
	else
	{
		// Buffer too short
		*BytesNeeded = ulInfoLen;
		ndisStatus = NDIS_STATUS_BUFFER_TOO_SHORT;
	}
				
	RT_PRINT_DATA( (COMP_OID_QUERY|COMP_POWER), DBG_LOUD, ("QUERY OID_PM_PARAMETERS: "), 
		InformationBuffer, InformationBufferLength );
				
	return ndisStatus;
}

NDIS_STATUS
N62C_QUERY_OID_GEN_INTERRUPT_MODERATION(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	if(InformationBufferLength < sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS))
	{
		*BytesNeeded = sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS);
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}

	ndisStatus = N62CQueryInterruptModerationSettings(
			pTargetAdapter, 
			InformationBuffer, 
			InformationBufferLength
		);			

	*BytesWritten = sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS);
				
	return ndisStatus;
}

NDIS_STATUS
N62C_QUERY_OID_PACKET_COALESCING_FILTER_MATCH_COUNT(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);

	return ndisStatus;
}


NDIS_STATUS
N62C_QUERY_OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS							ndisStatus = NDIS_STATUS_SUCCESS;
	PNDIS_RECEIVE_FILTER_CAPABILITIES	pRxFilterCapabilities = (PNDIS_RECEIVE_FILTER_CAPABILITIES)InformationBuffer;

	
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);
				
	if(InformationBufferLength < sizeof(NDIS_RECEIVE_FILTER_CAPABILITIES))
	{
		*BytesNeeded = sizeof(NDIS_RECEIVE_FILTER_CAPABILITIES);
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}			
				
	

#if (NDIS_SUPPORT_NDIS630)
	N6_ASSIGN_OBJECT_HEADER(
			pRxFilterCapabilities->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			NDIS_RECEIVE_FILTER_CAPABILITIES_REVISION_2,
			NDIS_SIZEOF_RECEIVE_FILTER_CAPABILITIES_REVISION_2);
#else
	N6_ASSIGN_OBJECT_HEADER(
			pRxFilterCapabilities->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			NDIS_RECEIVE_FILTER_CAPABILITIES_REVISION_1,
			NDIS_SIZEOF_RECEIVE_FILTER_CAPABILITIES_REVISION_1);
#endif

			
	pRxFilterCapabilities->Flags = 0;

	pRxFilterCapabilities->SupportedHeaders = NDIS_RECEIVE_FILTER_MAC_HEADER_SUPPORTED;

#if (NDIS_SUPPORT_NDIS630)
	pRxFilterCapabilities->EnabledFilterTypes = NDIS_RECEIVE_FILTER_PACKET_COALESCING_FILTERS_ENABLED;

	pRxFilterCapabilities->SupportedQueueProperties = NDIS_RECEIVE_FILTER_PACKET_COALESCING_SUPPORTED_ON_DEFAULT_QUEUE;
	
	pRxFilterCapabilities->SupportedFilterTests = NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_EQUAL_SUPPORTED |
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_MASK_EQUAL_SUPPORTED|
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_NOT_EQUAL_SUPPORTED; //0;

	pRxFilterCapabilities->SupportedHeaders |= (NDIS_RECEIVE_FILTER_ARP_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_IPV4_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_IPV6_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_UDP_HEADER_SUPPORTED);
	
	pRxFilterCapabilities->SupportedMacHeaderFields = NDIS_RECEIVE_FILTER_MAC_HEADER_DEST_ADDR_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PROTOCOL_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PACKET_TYPE_SUPPORTED;
	
	pRxFilterCapabilities->SupportedARPHeaderFields = NDIS_RECEIVE_FILTER_ARP_HEADER_OPERATION_SUPPORTED|
												NDIS_RECEIVE_FILTER_ARP_HEADER_SPA_SUPPORTED|
												NDIS_RECEIVE_FILTER_ARP_HEADER_TPA_SUPPORTED ;
	
	pRxFilterCapabilities->SupportedIPv4HeaderFields = NDIS_RECEIVE_FILTER_IPV4_HEADER_PROTOCOL_SUPPORTED;
	
	pRxFilterCapabilities->SupportedIPv6HeaderFields = NDIS_RECEIVE_FILTER_IPV6_HEADER_PROTOCOL_SUPPORTED;
	
	pRxFilterCapabilities->SupportedUdpHeaderFields = NDIS_RECEIVE_FILTER_UDP_HEADER_DEST_PORT_SUPPORTED;
	
	pRxFilterCapabilities->MaxFieldTestsPerPacketCoalescingFilter = 5; // should be >= 5 
	
	pRxFilterCapabilities->MaxPacketCoalescingFilters = 10; // should be >= 10
	//pRxFilterCapabilities->NidsReserved = 0;
#else
	pRxFilterCapabilities->EnabledFilterTypes = 0;

	pRxFilterCapabilities->SupportedQueueProperties = 0;
	
	pRxFilterCapabilities->SupportedFilterTests = NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_EQUAL_SUPPORTED |
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_MASK_EQUAL_SUPPORTED;
	
	pRxFilterCapabilities->SupportedMacHeaderFields = NDIS_RECEIVE_FILTER_MAC_HEADER_DEST_ADDR_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PROTOCOL_SUPPORTED;
#endif

	pRxFilterCapabilities->EnabledQueueTypes = 0; //NDIS_RECEIVE_FILTER_VM_QUEUES_ENABLED;
	pRxFilterCapabilities->NumQueues = 0;
	pRxFilterCapabilities->MaxMacHeaderFilters = 2;
	pRxFilterCapabilities->MaxQueueGroups = 0;
	pRxFilterCapabilities->MaxQueuesPerQueueGroup = 0;
	pRxFilterCapabilities->MinLookaheadSplitSize = 0;
	pRxFilterCapabilities->MaxLookaheadSplitSize = 0;

	RT_PRINT_DATA( (COMP_OID_QUERY|COMP_POWER), DBG_LOUD, ("QUERY N62C_QUERY_OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES: "), 
		InformationBuffer, InformationBufferLength );
				
	return ndisStatus;
}

NDIS_STATUS
N62C_QUERY_OID_RECEIVE_FILTER_CURRENT_CAPABILITIES(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS							ndisStatus = NDIS_STATUS_SUCCESS;
	PNDIS_RECEIVE_FILTER_CAPABILITIES	pRxFilterCapabilities = (PNDIS_RECEIVE_FILTER_CAPABILITIES)InformationBuffer;

	
		
	// Clean output variables -----------------------------------
	*BytesWritten = 0;
	*BytesNeeded = 0;
	//-------------------------------------------------------

	FunctionIn(COMP_OID_QUERY);
				
	if(InformationBufferLength < sizeof(NDIS_RECEIVE_FILTER_CAPABILITIES))
	{
		*BytesNeeded = sizeof(NDIS_RECEIVE_FILTER_CAPABILITIES);
		*BytesWritten = 0;
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}			
				
	

#if (NDIS_SUPPORT_NDIS630)
	N6_ASSIGN_OBJECT_HEADER(
			pRxFilterCapabilities->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			NDIS_RECEIVE_FILTER_CAPABILITIES_REVISION_2,
			NDIS_SIZEOF_RECEIVE_FILTER_CAPABILITIES_REVISION_2);
#else
	N6_ASSIGN_OBJECT_HEADER(
			pRxFilterCapabilities->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			NDIS_RECEIVE_FILTER_CAPABILITIES_REVISION_1,
			NDIS_SIZEOF_RECEIVE_FILTER_CAPABILITIES_REVISION_1);
#endif

			
	pRxFilterCapabilities->Flags = 0;

	pRxFilterCapabilities->SupportedHeaders = NDIS_RECEIVE_FILTER_MAC_HEADER_SUPPORTED;

#if (NDIS_SUPPORT_NDIS630)
	pRxFilterCapabilities->EnabledFilterTypes = NDIS_RECEIVE_FILTER_PACKET_COALESCING_FILTERS_ENABLED;

	pRxFilterCapabilities->SupportedQueueProperties = NDIS_RECEIVE_FILTER_PACKET_COALESCING_SUPPORTED_ON_DEFAULT_QUEUE;
	
	pRxFilterCapabilities->SupportedFilterTests = NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_EQUAL_SUPPORTED |
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_MASK_EQUAL_SUPPORTED|
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_NOT_EQUAL_SUPPORTED; //0;

	pRxFilterCapabilities->SupportedHeaders |= (NDIS_RECEIVE_FILTER_ARP_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_IPV4_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_IPV6_HEADER_SUPPORTED|
											NDIS_RECEIVE_FILTER_UDP_HEADER_SUPPORTED);
	
	pRxFilterCapabilities->SupportedMacHeaderFields = NDIS_RECEIVE_FILTER_MAC_HEADER_DEST_ADDR_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PROTOCOL_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PACKET_TYPE_SUPPORTED;
	
	pRxFilterCapabilities->SupportedARPHeaderFields = NDIS_RECEIVE_FILTER_ARP_HEADER_OPERATION_SUPPORTED|
												NDIS_RECEIVE_FILTER_ARP_HEADER_SPA_SUPPORTED|
												NDIS_RECEIVE_FILTER_ARP_HEADER_TPA_SUPPORTED ;
	
	pRxFilterCapabilities->SupportedIPv4HeaderFields = NDIS_RECEIVE_FILTER_IPV4_HEADER_PROTOCOL_SUPPORTED;
	
	pRxFilterCapabilities->SupportedIPv6HeaderFields = NDIS_RECEIVE_FILTER_IPV6_HEADER_PROTOCOL_SUPPORTED;
	
	pRxFilterCapabilities->SupportedUdpHeaderFields = NDIS_RECEIVE_FILTER_UDP_HEADER_DEST_PORT_SUPPORTED;
	
	pRxFilterCapabilities->MaxFieldTestsPerPacketCoalescingFilter = 5; // should be >= 5 
	
	pRxFilterCapabilities->MaxPacketCoalescingFilters = 10; // should be >= 10
	//pRxFilterCapabilities->NidsReserved = 0;
#else
	pRxFilterCapabilities->EnabledFilterTypes = 0;

	pRxFilterCapabilities->SupportedQueueProperties = 0;
	
	pRxFilterCapabilities->SupportedFilterTests = NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_EQUAL_SUPPORTED |
											NDIS_RECEIVE_FILTER_TEST_HEADER_FIELD_MASK_EQUAL_SUPPORTED;
	
	pRxFilterCapabilities->SupportedMacHeaderFields = NDIS_RECEIVE_FILTER_MAC_HEADER_DEST_ADDR_SUPPORTED|
											NDIS_RECEIVE_FILTER_MAC_HEADER_PROTOCOL_SUPPORTED;
#endif

	pRxFilterCapabilities->EnabledQueueTypes = 0; //NDIS_RECEIVE_FILTER_VM_QUEUES_ENABLED;
	pRxFilterCapabilities->NumQueues = 0;
	pRxFilterCapabilities->MaxMacHeaderFilters = 2;
	pRxFilterCapabilities->MaxQueueGroups = 0;
	pRxFilterCapabilities->MaxQueuesPerQueueGroup = 0;
	pRxFilterCapabilities->MinLookaheadSplitSize = 0;
	pRxFilterCapabilities->MaxLookaheadSplitSize = 0;

	RT_PRINT_DATA( (COMP_OID_QUERY|COMP_POWER), DBG_LOUD, ("QUERY N62C_QUERY_OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES: "), 
		InformationBuffer, InformationBufferLength );
				
	return ndisStatus;
}
//