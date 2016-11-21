#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "N62C_SetOID.tmh"
#endif

NDIS_STATUS	
N62C_SET_OID_DOT11_INCOMING_ASSOCIATION_DECISION(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	PRT_WLAN_STA 	pEntry = NULL;
	
	// OS-given structure
	PDOT11_INCOMING_ASSOC_DECISION_V2 pIncomAssocDecision  = (PDOT11_INCOMING_ASSOC_DECISION_V2) InformationBuffer;

	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	{
		if (InformationBufferLength < sizeof(DOT11_INCOMING_ASSOC_DECISION_V2) && InformationBufferLength < sizeof(DOT11_INCOMING_ASSOC_DECISION))
		{
			// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
			RT_TRACE( COMP_OID_SET, DBG_LOUD,
					("Set OID_DOT11_INCOMING_ASSOCIATION_DECISION: (DOT11_INCOMING_ASSOC_DECISION_V2) Invalid Length %d right length v2 %d right length %d\n", 
					InformationBufferLength, sizeof(DOT11_INCOMING_ASSOC_DECISION_V2), sizeof(DOT11_INCOMING_ASSOC_DECISION)));

			ndisStatus = NDIS_STATUS_INVALID_LENGTH;
			if(!GetDefaultAdapter(pTargetAdapter)->bInHctTest)
				return ndisStatus;
		}
	}	

	if (GetAPState(pTargetAdapter) != AP_STATE_STARTED)
	{
		ndisStatus = NDIS_STATUS_INVALID_STATE;
		return ndisStatus;
	}
			
	pEntry = AsocEntry_GetEntry(&pTargetAdapter->MgntInfo, pIncomAssocDecision->PeerMacAddr);

	if(pEntry)
	{
		pEntry->bOsDecisionMade = TRUE;
		pEntry->bNotAcceptedByOs = !pIncomAssocDecision->bAccept;
		pEntry->OsReasonCode = (u2Byte)pIncomAssocDecision->usReasonCode;
	}

	RT_PRINT_DATA(COMP_MLME|COMP_AP, DBG_LOUD, 
			"AsocRsp IE from OS:\n", 
			((pu1Byte)pIncomAssocDecision)+pIncomAssocDecision->uAssocResponseIEsOffset, 
			pIncomAssocDecision->uAssocResponseIEsLength
		);

	if (pEntry)
	{
		AsocEntry_UpdateAsocInfo(pTargetAdapter,
			pEntry->MacAddr,
			((pu1Byte)pIncomAssocDecision) + pIncomAssocDecision->uAssocResponseIEsOffset,
			pIncomAssocDecision->uAssocResponseIEsLength,
			UPDATE_ASOC_RSP_IE_FROM_OS
			);
	}

{
	if(InformationBufferLength < sizeof(DOT11_INCOMING_ASSOC_DECISION_V2))
		GET_P2P_INFO(pTargetAdapter)->Status = 0;	//Workaround: pIncomAssocDecision->WFDStatus0;
	else
		GET_P2P_INFO(pTargetAdapter)->Status = pIncomAssocDecision->WFDStatus;
	RT_TRACE_F(COMP_OID_SET, DBG_LOUD, ("Wifi-Direct: pIncomAssocDecision->WFDStatus: %d\n", pIncomAssocDecision->WFDStatus));
}
				
	*BytesRead = InformationBufferLength;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== N62C_SET_OID_DOT11_INCOMING_ASSOCIATION_DECISION accept %d\n", pIncomAssocDecision->bAccept) );
				
	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_DOT11_WPS_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	
	// OS-given structure

	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	if(InformationBufferLength < sizeof(BOOLEAN))
	{
		ndisStatus = NDIS_STATUS_INVALID_LENGTH;
		*BytesNeeded = sizeof(u1Byte);
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set AP mode OID_DOT11_WPS_ENABLED: Invalid length\n"));
		return ndisStatus;
	}

	pTargetAdapter->pNdis62Common->bWPSEnable = *((PBOOLEAN)InformationBuffer);
			
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<===Set AP mode OID_DOT11_WPS_ENABLED: %d\n", pTargetAdapter->pNdis62Common->bWPSEnable));

	return ndisStatus;
}


NDIS_STATUS	
N62C_SET_OID_DOT11_ADDITIONAL_IE(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	PVOID newBeaconIeData = NULL; 
	PVOID newResponseIeData = NULL;
	
	// OS-given structure
	PDOT11_ADDITIONAL_IE AdditionalIe = (PDOT11_ADDITIONAL_IE)InformationBuffer;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	if (!MP_VERIFY_NDIS_OBJECT_HEADER_DEFAULT(
			AdditionalIe->Header,
			NDIS_OBJECT_TYPE_DEFAULT,
			DOT11_ADDITIONAL_IE_REVISION_1,
			sizeof(DOT11_ADDITIONAL_IE)
		)
	)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set AP mode OID_DOT11_ADDITIONAL_IE: Invalid length\n"));
		ndisStatus = NDIS_STATUS_INVALID_DATA;
		return ndisStatus;
	}
				
	// 
	// allocate memory for the new IE data and copy the IEs
	//
	MgntActSet_AdditionalBeaconIE(pTargetAdapter, 
		(pu1Byte)((pu1Byte)AdditionalIe + AdditionalIe->uBeaconIEsOffset), 
		AdditionalIe->uBeaconIEsLength);
			
	MgntActSet_AdditionalProbeRspIE(pTargetAdapter, 
		(pu1Byte)((pu1Byte)AdditionalIe + AdditionalIe->uResponseIEsOffset), 
		AdditionalIe->uResponseIEsLength);
			
	RT_TRACE(COMP_OID_SET, DBG_LOUD, 
		("<=== Set AP mode OID_DOT11_ADDITIONAL_IE: BeaconIELen = %u, ProbeRspIELen = %u.\n", AdditionalIe->uBeaconIEsLength, AdditionalIe->uResponseIEsLength));

	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_DOT11_START_AP_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	RT_RF_POWER_STATE eRfPowerState;
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(pTargetAdapter);
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	if (GetAPState(pTargetAdapter) != AP_STATE_STOPPED)
	{
		ndisStatus = NDIS_STATUS_INVALID_STATE;
		RT_TRACE(COMP_AP, DBG_LOUD, ("AP is not in INIT STATE\n"));
		return ndisStatus;
	}

	pTargetAdapter->HalFunc.GetHwRegHandler(pTargetAdapter, HW_VAR_RF_STATE, (pu1Byte)(&eRfPowerState));

	//
	// We should fail the start request if the radio is OFF
	if((eRfPowerState == eRfOff) && (pDefaultAdapter->MgntInfo.RfOffReason >= RF_CHANGE_BY_HW))
	{
		RT_TRACE(COMP_RF, DBG_LOUD, ("RF is OFF, return NDIS_STATUS_DOT11_POWER_STATE_INVALID.\n"));
		ndisStatus = NDIS_STATUS_DOT11_POWER_STATE_INVALID;
		return ndisStatus;
	}
				
	ndisStatus=N62CStartApMode(pTargetAdapter);

	pTargetAdapter->pNdis62Common->CurrentOpState = OP_STATE;

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set AP mode OID_DOT11_START_AP_REQUEST.\n"));

	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_DOT11_DISASSOCIATE_PEER_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	PDOT11_DISASSOCIATE_PEER_REQUEST pDisassociationRequest = NULL;
	PRT_WLAN_STA	pEntry = NULL;
	PMGNT_INFO	pMgntInfo = &pTargetAdapter->MgntInfo;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	pDisassociationRequest = (PDOT11_DISASSOCIATE_PEER_REQUEST)InformationBuffer;
						
	if (!MP_VERIFY_NDIS_OBJECT_HEADER_DEFAULT(
                   pDisassociationRequest->Header,
                   NDIS_OBJECT_TYPE_DEFAULT,
                   DOT11_DISASSOCIATE_PEER_REQUEST_REVISION_1,
                   sizeof(DOT11_DISASSOCIATE_PEER_REQUEST))
	)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Set AP mode OID_DOT11_DISASSOCIATE_PEER_REQUEST: Invalid length\n"));
		ndisStatus = NDIS_STATUS_INVALID_DATA;
		return ndisStatus;
        }

	if (GetAPState(pTargetAdapter) != AP_STATE_STARTED)
	{
		ndisStatus = NDIS_STATUS_INVALID_STATE;
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("AP is not in OP STATE\n"));
		return ndisStatus;
	}

	if(MultiChannel_IsFCSInProgress(pTargetAdapter))
	{
		if(MacAddr_isBcst(pDisassociationRequest->PeerMacAddr))
		{
			MultiChannelDisconnectGo(pTargetAdapter);
		}
		RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("<=== Return Set AP mode OID_DOT11_DISASSOCIATE_PEER_REQUEST since MCC\n"));
		return ndisStatus;
	}

	if(MacAddr_isBcst(pDisassociationRequest->PeerMacAddr))
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Disassociate all peers.\n"));
				
		AP_DisassociateAllStation(pTargetAdapter, unspec_reason);	
		AsocEntry_ResetAll(pTargetAdapter);
	}
	else
	{
		RT_PRINT_ADDR(COMP_OID_SET, DBG_LOUD, "Dissociate peer:", pDisassociationRequest->PeerMacAddr);

		pEntry = AsocEntry_GetEntry(pMgntInfo, pDisassociationRequest->PeerMacAddr);
		if(pEntry)
		{
			AP_DisassociateStation(pTargetAdapter, pEntry, DOT11_DISASSOC_REASON_OS);

			//
			// Now the state of the STA is authenticated but not associated.
			// If an STA is disassociated because of this OID, it shall not be reported when
			// OID_DOT11_ENUM_PEER_INFO is requested.
			// In the sample code, deauth is sent and the STA entry is removed.
			// We remove the STA entry so that it won't be reported when OID_DOT11_ENUM_PEER_INFO 
			// is requested.
			//
			RT_TRACE_F(COMP_AP, DBG_TRACE, ("AsocEntry_RemoveStation\n"));
			
			AsocEntry_RemoveStation(pTargetAdapter, pDisassociationRequest->PeerMacAddr);
		}
	}

	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set AP mode OID_DOT11_DISASSOCIATE_PEER_REQUEST:\n"));	

	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_DOT11_ASSOCIATION_PARAMS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	PDOT11_ASSOCIATION_PARAMS pAssocParams = (PDOT11_ASSOCIATION_PARAMS)InformationBuffer;
			
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);
	
        if (!MP_VERIFY_NDIS_OBJECT_HEADER_DEFAULT(
                    pAssocParams->Header,
                    NDIS_OBJECT_TYPE_DEFAULT,
                    DOT11_ADDITIONAL_IE_REVISION_1,
                    sizeof(DOT11_ASSOCIATION_PARAMS))
	)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Invalid length\n"));
		ndisStatus = NDIS_STATUS_INVALID_DATA;
		return ndisStatus;
	}
				
	// 
	// allocate memory for the new IE data and copy the IEs
	//
	MgntActSet_AdditionalAssocReqIE(pTargetAdapter, 
			(pu1Byte)((pu1Byte)pAssocParams + pAssocParams->uAssocRequestIEsOffset), 
			pAssocParams->uAssocRequestIEsLength
		);

	//RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, "AssocReqIE", 
	//	(pu1Byte)((pu1Byte)pAssocParams + pAssocParams->uAssocRequestIEsOffset), pAssocParams->uAssocRequestIEsLength);
			
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("<=== Set OID_DOT11_ASSOCIATION_PARAMS.\n"));
		
	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_PM_ADD_WOL_PATTERN(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	NDIS_PM_WOL_PATTERN WOLPatternStructure = *((PNDIS_PM_WOL_PATTERN)InformationBuffer);
	u1Byte	WoLBitMapPatternMask[MAX_WOL_BIT_MASK_SIZE];
	u1Byte	WoLBitMapPatternContent[MAX_WOL_PATTERN_SIZE];
	u1Byte	WoLBitMapPattern[MAX_WOL_PATTERN_SIZE];
	u1Byte	Index;
	PMGNT_INFO				pMgntInfo = &(pTargetAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO	pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
			
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	pPSC->bSupportWakeUp = TRUE;

	PlatformZeroMemory(WoLBitMapPatternMask, sizeof(WoLBitMapPatternMask));
	PlatformZeroMemory(WoLBitMapPatternContent, sizeof(WoLBitMapPatternContent));

	if(WOLPatternStructure.WoLPacketType == NdisPMWoLPacketBitmapPattern)
	{
		if(WOLPatternStructure.WoLPattern.WoLBitMapPattern.MaskSize <= MAX_WOL_BIT_MASK_SIZE)
		{
			PlatformMoveMemory(WoLBitMapPatternMask, (pu1Byte)InformationBuffer+WOLPatternStructure.WoLPattern.WoLBitMapPattern.MaskOffset, WOLPatternStructure.WoLPattern.WoLBitMapPattern.MaskSize);						
			RT_PRINT_DATA( COMP_OID_SET | COMP_POWER, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: WoLBitMapPatternMask...\n"), 
			WoLBitMapPatternMask, sizeof(WoLBitMapPatternMask));
		}	
		if(WOLPatternStructure.WoLPattern.WoLBitMapPattern.PatternSize <= MAX_WOL_PATTERN_SIZE)
		{
			PlatformMoveMemory(WoLBitMapPatternContent, (pu1Byte)InformationBuffer+WOLPatternStructure.WoLPattern.WoLBitMapPattern.PatternOffset, WOLPatternStructure.WoLPattern.WoLBitMapPattern.PatternSize);
			RT_PRINT_DATA( COMP_OID_SET | COMP_POWER, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: WoLBitMapPatternContent...\n"), 
			WoLBitMapPatternContent, sizeof(WoLBitMapPatternContent));
		}
					
		//Find the index of the first empty entry.
		for(Index=0; Index<MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter); Index++)
		{
			if(pPmWoLPatternInfo[Index].PatternId == WOLPatternStructure.PatternId)
			{
				Index = MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter);
				break;
			}

			if(pPmWoLPatternInfo[Index].PatternId == 0)
				break;
		}
					
		if(Index >= MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter))
		{
			ndisStatus = NDIS_STATUS_RESOURCES;
			RT_TRACE(COMP_POWER, DBG_LOUD,
				("SET OID_PNP_ADD_WOL_PATTERN: Return status(%#X). The number of wake up pattern is more than %d or the pattern Id is exist.\n", 
				ndisStatus, MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter)));
			return ndisStatus;
		}
		else
		{
			u1Byte	BROADCAST_ADDR[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
			u1Byte	DONTCARE_ADDR[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			u1Byte	MULTICAST_ADDR[2]={0x33, 0x33};

			pPmWoLPatternInfo[Index].PatternType = eUnknownType;
			pPmWoLPatternInfo[Index].PatternId = WOLPatternStructure.PatternId;
			pPmWoLPatternInfo[Index].IsUserDefined = 0;

			//packet type.
			if(PlatformCompareMemory(WoLBitMapPatternContent, pTargetAdapter->PermanentAddress, sizeof(pTargetAdapter->PermanentAddress)) == 0 )
			{	//unicast
				//Skip IPv4/IPv6 TCP SYN pattern because it will hit HW bug for the CRC.
				// We offload this pattern to FW.
				pPmWoLPatternInfo[Index].PatternType = eUnicastPattern;
				RT_TRACE(COMP_OID_SET, DBG_LOUD, 
						("OID_PM_ADD_WOL_PATTERN: IPv4/IPv6 TCP SYN pattern.\n"));
				
			}
			else if(PlatformCompareMemory(WoLBitMapPatternContent, MULTICAST_ADDR, sizeof(MULTICAST_ADDR)) == 0)
			{	//Multicast
				pPmWoLPatternInfo[Index].PatternType = eMulticastPattern;
			}
			else if(PlatformCompareMemory(WoLBitMapPatternContent, BROADCAST_ADDR, sizeof(BROADCAST_ADDR)) == 0 )
			{	//broadcast
				pPmWoLPatternInfo[Index].PatternType = eBroadcastPattern;
			}
			else if(PlatformCompareMemory(WoLBitMapPatternContent, DONTCARE_ADDR, sizeof(DONTCARE_ADDR)) == 0 )
			{
				pPmWoLPatternInfo[Index].PatternType = eDontCareDA;
			}
			//DbgPrint("PatternType = %d\n", pPmWoLPatternInfo[Index].PatternType);
				
			{
				GetWOLWakeUpPattern(pTargetAdapter, 
						(pu1Byte)&WoLBitMapPatternMask, 
						WOLPatternStructure.WoLPattern.WoLBitMapPattern.MaskSize, 
						(pu1Byte)&WoLBitMapPatternContent, 
						WOLPatternStructure.WoLPattern.WoLBitMapPattern.PatternSize,
						Index,
						FALSE
					);

				pPSC->WoLPatternNum++;
			}
	  	}
	}
	else if(WOLPatternStructure.WoLPacketType == NdisPMWoLPacketEapolRequestIdMessage)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: Support EapolRequestIdMessage.\n"));
		
		if(!pPSC->EapolRequestIdMessagePatternId)
		{
			pPSC->EapolRequestIdMessagePatternId = WOLPatternStructure.PatternId;
			pPSC->WoLPktNoPtnNum++;
		}
	}
	else if(WOLPatternStructure.WoLPacketType == NdisPMWoLPacketMagicPacket)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: Support magic packet.\n"));

		if(!pPSC->MagicPacketPatternId)
		{
			pPSC->MagicPacketPatternId = WOLPatternStructure.PatternId;
			pPSC->WoLPktNoPtnNum++;
			
			// OS adds magic packet pattern to present that the user picks up the check box "Allow
			// this device to wake the computer", so we could according to the pattern to support
			// some patch functions, ex. Enable PME_En before S5.
			// 2013.06.28. by tynli.
			//pPSC->bSupportWakeUp = TRUE;
		}
		
	}
	else if(WOLPatternStructure.WoLPacketType == NdisPMWoLPacketIPv4TcpSyn)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: Support IPv4 TCP SYN.\n"));
		
		if(!pPSC->IPv4TcpSynPatternId)
		{
			pPSC->IPv4TcpSynPatternId = WOLPatternStructure.PatternId;
			pPSC->WoLPktNoPtnNum++;
		}
	}
	else if(WOLPatternStructure.WoLPacketType == NdisPMWoLPacketIPv6TcpSyn)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: Support IPv6 TCP SYN.\n"));
		
		if(!pPSC->IPv6TcpSynPatternId)
		{
			pPSC->IPv6TcpSynPatternId = WOLPatternStructure.PatternId;
			pPSC->WoLPktNoPtnNum++;
		}
	}
	else
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: Unspecified type(%#X).\n", WOLPatternStructure.WoLPacketType));
	}
				
	RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_WOL_PATTERN: \n"), 
			InformationBuffer, InformationBufferLength
		);

	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_PM_REMOVE_WOL_PATTERN(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	u4Byte	RemovePatternId = *((pu4Byte)InformationBuffer);
	u1Byte	Index;
	PMGNT_INFO				pMgntInfo = &(pTargetAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO	pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
		
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);
	
	if(RemovePatternId == pPSC->EapolRequestIdMessagePatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET_OID_PM_REMOVE_WOL_PATTERN: Remove EapolRequestIdMessage.\n"));
		
		pPSC->EapolRequestIdMessagePatternId = 0;
		pPSC->WoLPktNoPtnNum--;
	}
	else if(RemovePatternId == pPSC->MagicPacketPatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET_OID_PM_REMOVE_WOL_PATTERN: Remove MagicPacket.\n"));
		
		pPSC->MagicPacketPatternId = 0;
		pPSC->WoLPktNoPtnNum--;
		
		// Clear PME enable support because OS removes it.
		//pPSC->bSupportWakeUp = FALSE;
		
		//RT_TRACE_F(COMP_POWER, DBG_LOUD, ("Do not allow PME_En by the upper layer!\n"));
	}
	else if(RemovePatternId == pPSC->IPv4TcpSynPatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET_OID_PM_REMOVE_WOL_PATTERN: Remove IPv4TcpSyn.\n"));
		
		pPSC->IPv4TcpSynPatternId = 0;
		pPSC->WoLPktNoPtnNum--;
	}
	else if(RemovePatternId == pPSC->IPv6TcpSynPatternId)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET_OID_PM_REMOVE_WOL_PATTERN: Remove IPv6TcpSyn.\n"));
		
		pPSC->IPv6TcpSynPatternId = 0;
		pPSC->WoLPktNoPtnNum--;
	}
	
	for(Index=0; Index<MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter); Index++)
	{
		if(pPmWoLPatternInfo[Index].PatternId == RemovePatternId)
			break;
	}
	if(Index >= MAX_SUPPORT_WOL_PATTERN_NUM(pTargetAdapter))
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD,("SET OID_PM_REMOVE_WOL_PATTERN: Cannot find the wake up pattern Id(%08X).\n", RemovePatternId));
	}
	else
	{
		//Reset the structure and set WFCRC register to non-zero value.
		pPmWoLPatternInfo[Index].PatternId = 0;
		PlatformZeroMemory(pPmWoLPatternInfo[Index].Mask, sizeof(pPmWoLPatternInfo[Index].Mask));
		pPmWoLPatternInfo[Index].CrcRemainder = 0xffff;
		pPmWoLPatternInfo[Index].IsPatternMatch = 0;
		pPmWoLPatternInfo[Index].IsSupportedByFW = 0;
		pPSC->WoLPatternNum--;
					
		pTargetAdapter->HalFunc.SetHwRegHandler(pTargetAdapter, HW_VAR_WF_MASK, (pu1Byte)(&Index)); 
		pTargetAdapter->HalFunc.SetHwRegHandler(pTargetAdapter, HW_VAR_WF_CRC, (pu1Byte)(&Index)); 
		pPmWoLPatternInfo[Index].HwWFMIndex = 0xff; // reset the value after clear HW/CAM entry.
	}

	//RT_PRINT_DATA( (COMP_OID_QUERY|COMP_AP), DBG_LOUD, ("SET OID_PM_REMOVE_WOL_PATTERN: "), 
	//InformationBuffer, InformationBufferLength );

	if(pPSC->WoLPktNoPtnNum + pPSC->WoLPatternNum)
		pPSC->bSupportWakeUp = TRUE;
	else
		pPSC->bSupportWakeUp = FALSE;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("WoLPktNoPtnNum = %d.\n", pPSC->WoLPktNoPtnNum));
	RT_TRACE(COMP_POWER, DBG_LOUD, ("WoLPatternNum = %d.\n", pPSC->WoLPatternNum));

	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_PM_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	PNDIS_PM_PARAMETERS pPMParameters = (PNDIS_PM_PARAMETERS)InformationBuffer;
	PMGNT_INFO					pMgntInfo = &(pTargetAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	BOOLEAN						bSupportARPOffload=FALSE, bSupportGTKOffload=FALSE;
			
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);
				
	RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_PARAMETERS: "), 
			InformationBuffer, InformationBufferLength );
				
				
	if(pPMParameters->EnabledProtocolOffloads & NDIS_PM_PROTOCOL_OFFLOAD_ARP_ENABLED)
	{
		bSupportARPOffload = TRUE;
		RT_TRACE(COMP_POWER , DBG_LOUD , ("OID_PM_PARAMETERS: NDIS_PM_PROTOCOL_OFFLOAD_ARP_ENABLED\n") );
		if(pPSC->RegARPOffloadEnable)
			pPSC->ARPOffloadEnable = TRUE;
	}
	else
	{
		// We should not disable ARPOffloadEable flag besides HCT test, because we need to support 
		// ARP offload in RealWoW. Our service will set ARP info to driver instead of NDIS OID, so 
		// we do not accord to NIDS OID to decide whether we support ARP offload. 2013.10.08. by tynli.
		if(pTargetAdapter->bInHctTest)
			pPSC->ARPOffloadEnable = FALSE;
	}

	if(pPMParameters->EnabledProtocolOffloads & NDIS_PM_PROTOCOL_OFFLOAD_NS_ENABLED)
	{
		bSupportARPOffload = TRUE;
		RT_TRACE(COMP_POWER , DBG_LOUD , ("OID_PM_PARAMETERS: NDIS_PM_PROTOCOL_OFFLOAD_NS_ENABLED\n") );
		if(pPSC->RegNSOffloadEnable)
			pPSC->NSOffloadEnable = TRUE;
	}
	else
	{
		// We should not disable ARPOffloadEable flag besides HCT test, because we need to support 
		// ARP offload in RealWoW. Our service will set ARP info to driver instead of NDIS OID, so 
		// we do not accord to NIDS OID to decide whether we support ARP offload. 2013.10.08. by tynli.
		if(pTargetAdapter->bInHctTest)
			pPSC->NSOffloadEnable = FALSE;
	}

	if(pPMParameters->EnabledProtocolOffloads & NDIS_PM_PROTOCOL_OFFLOAD_80211_RSN_REKEY_ENABLED)
	{
		bSupportGTKOffload = TRUE;
		RT_TRACE(COMP_POWER , DBG_LOUD , ("OID_PM_PARAMETERS: NDIS_PM_PROTOCOL_OFFLOAD_80211_RSN_REKEY_ENABLED\n") );
		if(pPSC->RegGTKOffloadEnable)
			pPSC->GTKOffloadEnable = TRUE;
	}
	else
	{
		// We should not disable GTKOffloadEable flag besides HCT test, because we need to support 
		// GTK offload in RealWoW and S5 wake up. We do not accord to NIDS OID to decide whether
		// we support GTK offload. 2013.10.08. by tynli.		
		if(pTargetAdapter->bInHctTest)
			pPSC->GTKOffloadEnable = FALSE;
	}

	if(pPSC->ProtocolOffloadDecision)
	{
		if(pPSC->RegARPOffloadEnable || pPSC->RegGTKOffloadEnable)
		{
			if(bSupportARPOffload || bSupportGTKOffload)
				pPSC->bOSSupportProtocolOffload = TRUE;
			else
				pPSC->bOSSupportProtocolOffload = FALSE;
		}
		else
		{
			pPSC->bOSSupportProtocolOffload = TRUE;
		}
	}
	else
	{
		pPSC->bOSSupportProtocolOffload = TRUE;
	}			

	pPSC->bSetPMParameters = TRUE;
				
	*BytesRead  = InformationBufferLength;
	//RT_PRINT_DATA( (COMP_OID_QUERY|COMP_AP), DBG_LOUD, ("SET OID_PM_PARAMETERS: pPMParameters "), 
	//pPMParameters->EnabledProtocolOffloads , sizeof(NDIS_PM_PARAMETERS) );

	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_PM_ADD_PROTOCOL_OFFLOAD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	PNDIS_PM_PROTOCOL_OFFLOAD pPMProtocolOffload = (PNDIS_PM_PROTOCOL_OFFLOAD)InformationBuffer;
	RT_PM_PROTOCOL_OFFLOAD	PmPO;
	PMGNT_INFO pMgntInfo = &pTargetAdapter->MgntInfo;
	BOOLEAN						bNextPMEntryExit = FALSE;
	u1Byte						IPv6NSIndex =0;
	u4Byte						mPOoffset = RTL_SIZEOF_THROUGH_FIELD(NDIS_PM_PROTOCOL_OFFLOAD, NextProtocolOffloadOffset) + sizeof(ULONG);
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);
	
	PlatformZeroMemory(&PmPO, sizeof(RT_PM_PROTOCOL_OFFLOAD));

	//DbgPrint("pPMProtocolOffload->ProtocolOffloadType = 0x%x\n", pPMProtocolOffload->ProtocolOffloadType);
	//DbgPrint("pPMProtocolOffload->ProtocolOffloadId = 0x%x\n", pPMProtocolOffload->ProtocolOffloadId);	

	// Grab the information from Informationbuffer.
	do{
		// Init flag !!
		bNextPMEntryExit = FALSE;
		if(pPMProtocolOffload->ProtocolOffloadType == NdisPMProtocolOffload80211RSNRekey)
		{
			pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eGTKOffloadIdx] = pPMProtocolOffload->ProtocolOffloadId;
			
			//Copy kck, kek
			PlatformMoveMemory(&PmPO, (pu1Byte)InformationBuffer+mPOoffset, 36); 
			PmPO.ProtocolOffloadParameters.Dot11RSNRekeyParameters.KeyReplayCounter = 
			pPMProtocolOffload->ProtocolOffloadParameters.Dot11RSNRekeyParameters.KeyReplayCounter;
			PlatformMoveMemory(&(pMgntInfo->PMDot11RSNRekeyPara), (pu1Byte)&PmPO, sizeof(RT_PM_DOT11_RSN_REKEY_PARAMETERS)); 
		}
		else if(pPMProtocolOffload->ProtocolOffloadType == NdisPMProtocolOffloadIdIPv4ARP)
		{
			pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eARPOffloadIdx] = pPMProtocolOffload->ProtocolOffloadId;
			
			PlatformMoveMemory(&PmPO, (pu1Byte)InformationBuffer+mPOoffset, sizeof(PmPO.ProtocolOffloadParameters.IPv4ARPParameters)); 
				
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM RemoteIPv4Address:\n"), 
			pPMProtocolOffload->ProtocolOffloadParameters.IPv4ARPParameters.RemoteIPv4Address, 4);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("RemoteIPv4Address:\n"), 
			PmPO.ProtocolOffloadParameters.IPv4ARPParameters.RemoteIPv4Address, 4);

			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM HostIPv4Address:\n"), 
			pPMProtocolOffload->ProtocolOffloadParameters.IPv4ARPParameters.HostIPv4Address, 4);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("HostIPv4Address:\n"), 
			PmPO.ProtocolOffloadParameters.IPv4ARPParameters.HostIPv4Address, 4);

			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM MacAddress:\n"), 
			pPMProtocolOffload->ProtocolOffloadParameters.IPv4ARPParameters.MacAddress, 6);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("MacAddress:\n"), 
			PmPO.ProtocolOffloadParameters.IPv4ARPParameters.MacAddress, 6);

			PlatformMoveMemory(&(pMgntInfo->PMIPV4ARPPara), &PmPO, sizeof(RT_PM_IPV4_ARP_PARAMETERS)); 
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("==> HostIPv4Address:\n"), 
			pMgntInfo->PMIPV4ARPPara.HostIPv4Address, 4);
		}
		else if(pPMProtocolOffload->ProtocolOffloadType == NdisPMProtocolOffloadIdIPv6NS)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_PROTOCOL_OFFLOAD. (NS IPv6)\n"));
			if((pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] != 0) &&
				(pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx2] != 0))
			{
				ndisStatus = NDIS_STATUS_RESOURCES;
				RT_TRACE( COMP_OID_SET, DBG_LOUD, ("Insufficient NS offload number!! Status(%#X) <==\n", ndisStatus));
			}
			else
			{

				if( pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] ==  0)
				{
					pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] = pPMProtocolOffload->ProtocolOffloadId;
					IPv6NSIndex = 0;
					RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==> eNSOffloadIdx1: \n"));
				}
				else
				{
					pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx2] = pPMProtocolOffload->ProtocolOffloadId;
					IPv6NSIndex = 1;
					RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==> eNSOffloadIdx2: \n"));
				}
						
				pMgntInfo->PMIPV6NSPara[IPv6NSIndex].Flags = pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.Flags;

				PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].RemoteIPv6Address, 
									pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.RemoteIPv6Address, 16);
				PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].SolicitedNodeIPv6Address, 
									pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.SolicitedNodeIPv6Address, 16);
				PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].MacAddress, 
									pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.MacAddress, 6);
				PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].TargetIPv6Addresses[0], 
									pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[0], 16);
				PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].TargetIPv6Addresses[1], 
									pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[1], 16);
				RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM RemoteIPv6Address: \n"), 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.RemoteIPv6Address ,16);
				RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM SolicitedNodeIPv6Address: \n"), 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.SolicitedNodeIPv6Address ,16);
				RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM MacAddress: \n"), 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.MacAddress ,6);
				RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM TargetIPv6Addresses[0]: \n"), 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[0] ,16);
				
				RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM TargetIPv6Addresses[1]: \n"), 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[1] ,16);
			}
		}

		// Check Next entry !!
		if(pPMProtocolOffload->NextProtocolOffloadOffset  != 0)
		{
			bNextPMEntryExit = TRUE;
			// Point to Next Entry !!
			pPMProtocolOffload = (PNDIS_PM_PROTOCOL_OFFLOAD)((pu1Byte)InformationBuffer + pPMProtocolOffload->NextProtocolOffloadOffset);
			
		}
		
	}while(bNextPMEntryExit);
				
	RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET N62C_SET_OID_PM_ADD_PROTOCOL_OFFLOAD: \n"), 
				InformationBuffer, InformationBufferLength );

	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_PM_REMOVE_PROTOCOL_OFFLOAD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 	ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO 		pMgntInfo = &pTargetAdapter->MgntInfo;
	u4Byte			ProtocolOffloadId  = 0, MatchId=0;
	BOOLEAN			bMatch=FALSE;
	
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);

	ProtocolOffloadId = *((ULONG *)InformationBuffer);

	for(MatchId=0; MatchId<4; MatchId++)
	{
		if(ProtocolOffloadId == pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[MatchId])
		{
			bMatch = TRUE;
			RT_TRACE(COMP_POWER, DBG_LOUD, ("Find match protocol offload ID idx (%d)\n", MatchId));
			//break;

			if(bMatch)
			{
				if(MatchId == eGTKOffloadIdx)
				{
					PlatformZeroMemory(&(pMgntInfo->PMDot11RSNRekeyPara), sizeof(RT_PM_DOT11_RSN_REKEY_PARAMETERS)); 
				}
				else if(MatchId == eARPOffloadIdx)
				{
					PlatformZeroMemory(&(pMgntInfo->PMIPV4ARPPara), sizeof(RT_PM_IPV4_ARP_PARAMETERS)); 
				}
				else if(MatchId == eNSOffloadIdx1 )
				{	
					// Disable eNSOffloadIdx1 , set ID = 0 !!
					pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] = 0;
					RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==>REMOVE eNSOffloadIdx1: \n"));
				}
				else if(MatchId == eNSOffloadIdx2)
				{
					// Disable eNSOffloadIdx2 , set ID = 0 !!
					pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx2] = 0;
					RT_TRACE( COMP_OID_SET, DBG_LOUD, ("==>REMOVE eNSOffloadIdx2: \n"));
				}
			}
		}
	}
	RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_REMOVE_PROTOCOL_OFFLOAD (%x)\n", ProtocolOffloadId));

	return ndisStatus;
}


NDIS_STATUS	
N62C_SET_OID_PM_ADD_PROTOCOL_OFFLOAD_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 				ndisStatus = NDIS_STATUS_SUCCESS;
	PNDIS_PM_PROTOCOL_OFFLOAD 	pPMProtocolOffload = (PNDIS_PM_PROTOCOL_OFFLOAD)InformationBuffer;
	RT_PM_PROTOCOL_OFFLOAD	PmPO;
	PMGNT_INFO 					pMgntInfo = &pTargetAdapter->MgntInfo;
	BOOLEAN						bNextPMEntryExit = FALSE;
	u1Byte						IPv6NSIndex =0;
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);
	
	PlatformZeroMemory(&PmPO, sizeof(RT_PM_PROTOCOL_OFFLOAD));

	//DbgPrint("pPMProtocolOffload->ProtocolOffloadType = 0x%x\n", pPMProtocolOffload->ProtocolOffloadType);
	//DbgPrint("pPMProtocolOffload->ProtocolOffloadId = 0x%x\n", pPMProtocolOffload->ProtocolOffloadId);	
	RT_PRINT_DATA(COMP_OID_SET, DBG_LOUD, ("SET N62C_SET_OID_PM_ADD_PROTOCOL_OFFLOAD: "), 
			InformationBuffer, InformationBufferLength );

	// Grab the information from Informationbuffer.
	do{
		// Init flag !!
		bNextPMEntryExit = FALSE;
		if(pPMProtocolOffload->ProtocolOffloadType == NdisPMProtocolOffload80211RSNRekey)
		{
			pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eGTKOffloadIdx] = pPMProtocolOffload->ProtocolOffloadId;
			
			//Copy kck, kek
			PlatformMoveMemory(&PmPO, (pu1Byte)pPMProtocolOffload+NDIS_SIZEOF_NDIS_PM_PROTOCOL_OFFLOAD_REVISION_1, 36); 
			PmPO.ProtocolOffloadParameters.Dot11RSNRekeyParameters.KeyReplayCounter = 
			pPMProtocolOffload->ProtocolOffloadParameters.Dot11RSNRekeyParameters.KeyReplayCounter;
			PlatformMoveMemory(&(pMgntInfo->PMDot11RSNRekeyPara), (pu1Byte)&PmPO, sizeof(RT_PM_DOT11_RSN_REKEY_PARAMETERS)); 
		}
		else if(pPMProtocolOffload->ProtocolOffloadType == NdisPMProtocolOffloadIdIPv4ARP)
		{
			pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eARPOffloadIdx] = pPMProtocolOffload->ProtocolOffloadId;
			
			PlatformMoveMemory(&PmPO, (pu1Byte)pPMProtocolOffload+NDIS_SIZEOF_NDIS_PM_PROTOCOL_OFFLOAD_REVISION_1, sizeof(PmPO.ProtocolOffloadParameters.IPv4ARPParameters)); 
				
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM RemoteIPv4Address: "), 
			pPMProtocolOffload->ProtocolOffloadParameters.IPv4ARPParameters.RemoteIPv4Address, 4);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("RemoteIPv4Address: "), 
			PmPO.ProtocolOffloadParameters.IPv4ARPParameters.RemoteIPv4Address, 4);

			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM HostIPv4Address: "), 
			pPMProtocolOffload->ProtocolOffloadParameters.IPv4ARPParameters.HostIPv4Address, 4);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("HostIPv4Address: "), 
			PmPO.ProtocolOffloadParameters.IPv4ARPParameters.HostIPv4Address, 4);

			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM MacAddress: "), 
			pPMProtocolOffload->ProtocolOffloadParameters.IPv4ARPParameters.MacAddress, 6);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("MacAddress: "), 
			PmPO.ProtocolOffloadParameters.IPv4ARPParameters.MacAddress, 6);

			PlatformMoveMemory(&(pMgntInfo->PMIPV4ARPPara), &PmPO, sizeof(RT_PM_IPV4_ARP_PARAMETERS)); 
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("==> HostIPv4Address: "), 
			pMgntInfo->PMIPV4ARPPara.HostIPv4Address, 4);
		}
		else if(pPMProtocolOffload->ProtocolOffloadType == NdisPMProtocolOffloadIdIPv6NS)
		{
			RT_TRACE(COMP_OID_SET, DBG_LOUD, ("SET OID_PM_ADD_PROTOCOL_OFFLOAD. (NS IPv6)\n"));
			if( pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] ==  0)
			{
				pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx1] = pPMProtocolOffload->ProtocolOffloadId;
				IPv6NSIndex = 0;
			}
			else
			{
				pMgntInfo->PowerSaveControl.PMProtocolOffloadIDs[eNSOffloadIdx2] = pPMProtocolOffload->ProtocolOffloadId;
				IPv6NSIndex = 1;
			}

			pMgntInfo->PMIPV6NSPara[IPv6NSIndex].Flags = pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.Flags;

			PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].RemoteIPv6Address, 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.RemoteIPv6Address, 16);
			PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].SolicitedNodeIPv6Address, 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.SolicitedNodeIPv6Address, 16);
			PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].MacAddress, 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.MacAddress, 6);
			PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].TargetIPv6Addresses[0], 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[0], 16);
			PlatformMoveMemory(pMgntInfo->PMIPV6NSPara[IPv6NSIndex].TargetIPv6Addresses[1], 
								pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[1], 16);

			
			//PlatformMoveMemory(&PmPO, (pu1Byte)pPMProtocolOffload+NDIS_SIZEOF_NDIS_PM_PROTOCOL_OFFLOAD_REVISION_1, sizeof(PmPO.ProtocolOffloadParameters.IPv6NSParameters));
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM RemoteIPv6Address: "), 
							pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.RemoteIPv6Address ,16);
			//RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("RemoteIPv6Address: "), 
			//				PmPO.ProtocolOffloadParameters.IPv6NSParameters.RemoteIPv6Address ,16);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM SolicitedNodeIPv6Address: "), 
							pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.SolicitedNodeIPv6Address ,16);
			//RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SolicitedNodeIPv6Address: "), 
			//				PmPO.ProtocolOffloadParameters.IPv6NSParameters.SolicitedNodeIPv6Address ,16);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM MacAddress: "), 
							pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.MacAddress ,6);
			//RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("MacAddress: "), 
			//				PmPO.ProtocolOffloadParameters.IPv6NSParameters.MacAddress ,6);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM TargetIPv6Addresses[0]: "), 
							pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[0] ,16);
			//RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("TargetIPv6Addresses[0]: "), 
			//				PmPO.ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[0] ,16);
			RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET PM TargetIPv6Addresses[1]: "), 
							pPMProtocolOffload->ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[1] ,16);
			//RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("TargetIPv6Addresses[1]: "), 
			//				PmPO.ProtocolOffloadParameters.IPv6NSParameters.TargetIPv6Addresses[1] ,16);
			
			//PlatformMoveMemory(&(pMgntInfo->PMIPV6NSPara[IPv6NSIndex]),  &PmPO, sizeof(RT_PM_IPV6_NS_PARAMETERS) );
			
		}

		// Check Next entry !!
		
		if(pPMProtocolOffload->NextProtocolOffloadOffset  != 0)
		{
			bNextPMEntryExit = TRUE;
			// Point to Next Entry !!
			pPMProtocolOffload = (PNDIS_PM_PROTOCOL_OFFLOAD)((pu1Byte)InformationBuffer + pPMProtocolOffload->NextProtocolOffloadOffset);
			
		}
		
	}while(bNextPMEntryExit);
				
	RT_PRINT_DATA( COMP_OID_SET, DBG_LOUD, ("SET N62C_SET_OID_PM_ADD_PROTOCOL_OFFLOAD: "), 
				InformationBuffer, InformationBufferLength );

	return ndisStatus;
}

NDIS_STATUS	
N62C_SET_OID_RECEIVE_FILTER_CLEAR_FILTER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS	 								ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO 									pMgntInfo = &pTargetAdapter->MgntInfo;
	PNDIS_RECEIVE_FILTER_CLEAR_PARAMETERS		pFilterClear = (PNDIS_RECEIVE_FILTER_CLEAR_PARAMETERS)InformationBuffer;
	PRT_DO_COALESICING_FILTER_PARAMETER		pCurrRTCoPa = NULL;
	u4Byte										Removeid = 0;
	u1Byte										CurrIndex = 0;

	FunctionIn(COMP_OID_SET);
	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	if(InformationBufferLength < NDIS_SIZEOF_RECEIVE_FILTER_CLEAR_PARAMETERS_REVISION_1)
	{
		*BytesNeeded = NDIS_SIZEOF_RECEIVE_FILTER_CLEAR_PARAMETERS_REVISION_1;
		ndisStatus = NDIS_STATUS_INVALID_LENGTH;
		return ndisStatus;
	}
	
	Removeid = pFilterClear->FilterId;
	
	for(CurrIndex = 0 ; CurrIndex < 10 ; CurrIndex++)
	{
		pCurrRTCoPa = &(pMgntInfo->mRtD0ColesFilterInfo.dFilterArry[CurrIndex]);

		if( pCurrRTCoPa->FilterID == Removeid )
		{
			PlatformZeroMemory(pCurrRTCoPa , sizeof(RT_DO_COALESICING_FILTER_PARAMETER));
			RT_TRACE(COMP_POWER, DBG_LOUD, ("====>Remove Currindex (%d)\n",CurrIndex));
			break;
		}
		
	}

	if( CurrIndex == 10   )
	{
		RT_TRACE(COMP_POWER, DBG_LOUD, ("====>Fail Removeid (%d)\n",Removeid));
		ndisStatus = NDIS_STATUS_INVALID_PARAMETER;
	}

	return ndisStatus;
}

NDIS_STATUS	
N62C_QUERYSET_OID_RECEIVE_FILTER_SET_FILTER(
	IN	PADAPTER		pTargetAdapter,
	IN    NDIS_OID		Oid,
	IN    PVOID			InformationBuffer,
	IN    ULONG			InputBufferLength,
	IN    ULONG			OutputBufferLength,
	IN    ULONG			MethodId,
	OUT   PULONG			BytesWritten,
	OUT   PULONG			BytesRead,
	OUT   PULONG			BytesNeeded	
)
{
	NDIS_STATUS	 								ndisStatus = NDIS_STATUS_SUCCESS;
	PMGNT_INFO 									pMgntInfo = &pTargetAdapter->MgntInfo;
	PNDIS_RECEIVE_FILTER_PARAMETERS			pRxFilterPara = NULL;
	PNDIS_RECEIVE_FILTER_FIELD_PARAMETERS		pRxFilterFieldPara = NULL;
	u1Byte										Currindex =0;
	PRT_DO_COALESICING_FILTER_PARAMETER		pCurrRTCoPa = NULL;
	PRT_DO_COALESICING_FIELD_INFO				pCurrRTCoField = NULL;
	ULONG										FieldOffset;
	pu1Byte										pFieldParametersArray;
	ULONG										FieldElementSize;
	//FieldParametersArrayElementSize

	// Output variables (Currently no use)
	*BytesRead = 0;
	*BytesNeeded = 0;

	FunctionIn(COMP_OID_SET);
	pRxFilterPara = (PNDIS_RECEIVE_FILTER_PARAMETERS)InformationBuffer;
	pFieldParametersArray =(pu1Byte)InformationBuffer + pRxFilterPara->FieldParametersArrayOffset;
	FieldOffset = pRxFilterPara->FieldParametersArrayOffset;
	FieldElementSize = pRxFilterPara->FieldParametersArrayElementSize;

	if(InputBufferLength < (RTL_SIZEOF_THROUGH_FIELD(NDIS_RECEIVE_FILTER_FIELD_PARAMETERS, ResultValue)))
	{
		*BytesNeeded = (RTL_SIZEOF_THROUGH_FIELD(NDIS_RECEIVE_FILTER_FIELD_PARAMETERS, ResultValue));
		ndisStatus = NDIS_STATUS_INVALID_LENGTH;
		return ndisStatus;
	}
	
#if (NDIS_SUPPORT_NDIS630)
	// Check filter type !!
	if(pRxFilterPara->FilterType != NdisReceiveFilterTypePacketCoalescing)
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Query/Set NDIS_RECEIVE_FILTER_PARAMETERS: Invalid FilterType(%x)\n",pRxFilterPara->FilterType));
		ndisStatus = NDIS_STATUS_INVALID_PARAMETER;
		return ndisStatus;
	}
#endif

	if(pRxFilterPara->FieldParametersArrayNumElements > 5 )
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("Query/Set NDIS_RECEIVE_FILTER_PARAMETERS: Invalid FieldParametersArrayNumElements(%x)\n",pRxFilterPara->FieldParametersArrayNumElements));
		ndisStatus = NDIS_STATUS_INVALID_PARAMETER;
		return ndisStatus;
	}
	

	// Get Free Entry of FILTER_PARAMETER
	for(Currindex =  0 ; Currindex < 10 ; Currindex++)
	{
		if( pMgntInfo->mRtD0ColesFilterInfo.dFilterArry[Currindex].FilterID == 0)
		{
			RT_TRACE(COMP_POWER, DBG_LOUD, ("====>PARAMETER Currindex (%d)\n",Currindex));
			break;
		}
	}

	if( Currindex < 10 )
	{
		pCurrRTCoPa = &(pMgntInfo->mRtD0ColesFilterInfo.dFilterArry[Currindex]);
		pCurrRTCoPa->FilterID = pRxFilterPara->FilterId;
		pCurrRTCoPa->NumOfElem = pRxFilterPara->FieldParametersArrayNumElements;
		RT_TRACE(COMP_POWER, DBG_LOUD, ("====> pCurrRTCoPa->NumOfElem (%d)\n",pCurrRTCoPa->NumOfElem));
#if (NDIS_SUPPORT_NDIS630)
		pCurrRTCoPa->Delaytime = pRxFilterPara->MaxCoalescingDelay;
#else
		pCurrRTCoPa->Delaytime = 200;  // Default set 200 for win7 ~~
#endif
	}
	else
	{
		ndisStatus = NDIS_STATUS_INVALID_PARAMETER;
		return ndisStatus;
	}

	RT_PRINT_DATA(COMP_POWER, DBG_LOUD, "===> N62C_QUERYSET_OID_RECEIVE_FILTER_SET_FILTER :\n ", InformationBuffer, InputBufferLength);

	// Save Filed 
	Currindex = 0;
	while( Currindex < pCurrRTCoPa->NumOfElem )
	{
		// Get entry !!
		pRxFilterFieldPara = (PNDIS_RECEIVE_FILTER_FIELD_PARAMETERS)(pFieldParametersArray + (FieldElementSize*Currindex));
		pCurrRTCoField = &(pCurrRTCoPa->dFieldArry[Currindex]);
		
		// Save data
		PlatformMoveMemory(pCurrRTCoField, pRxFilterFieldPara, FieldElementSize);
/*
		// For Dbg show message !! 
		RT_TRACE(COMP_POWER, DBG_LOUD, ("====>Filed Currindex (%d)\n",Currindex));
		if( pRxFilterFieldPara->FrameHeader == NdisFrameHeaderArp)
		{
			RT_PRINT_DATA(COMP_POWER, DBG_LOUD, "===>pRxFilterFieldPara: \n", pRxFilterFieldPara, FieldElementSize);
			RT_PRINT_DATA(COMP_POWER, DBG_LOUD, "===>pCurrRTCoField: \n", pCurrRTCoField, sizeof(RT_DO_COALESICING_FIELD_INFO));
			RT_TRACE(COMP_POWER, DBG_LOUD , ("pRxFilterFieldPara->HeaderField.ArpHeaderField (%d)\n",pRxFilterFieldPara->HeaderField.ArpHeaderField));
			RT_TRACE(COMP_POWER, DBG_LOUD , ("pCurrRTCoField->HeaderField.ArpHeaderField (%d)\n",pCurrRTCoField->HeaderField.ArpHeaderField));

			if( pCurrRTCoField->HeaderField.ArpHeaderField ==  RTARPHeaderFieldOperation )
			{
				RT_TRACE(COMP_POWER, DBG_LOUD , ("pRxFilterFieldPara->FieldValue.ResultShortValue (%d)\n",pRxFilterFieldPara->FieldValue.FieldShortValue));
				RT_TRACE(COMP_POWER, DBG_LOUD , ("pCurrRTCoField->FieldValue.ResultShortValue (%d)\n",pCurrRTCoField->FieldValue.FieldShortValue));
				//FieldShortValue
			}
		}
*/
		Currindex++;
	}
	
	pMgntInfo->mRtD0ColesFilterInfo.bEnable = TRUE;

	return ndisStatus;
}
