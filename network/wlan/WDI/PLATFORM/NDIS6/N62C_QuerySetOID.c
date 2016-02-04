#include "Mp_Precomp.h"

NDIS_STATUS
N62CAPResetRequest(
	IN	PADAPTER	Adapter,
	OUT	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	IN	ULONG		OutputBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
	)
{
	PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;
	PRT_NDIS62_COMMON	pNdis62Common = Adapter->pNdis62Common;

	PDOT11_RESET_REQUEST pDot11ResetRequest = InformationBuffer;
	PDOT11_STATUS_INDICATION pDot11StatusIndication = InformationBuffer;
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	
	RT_TRACE( (COMP_OID_QUERY | COMP_OID_SET | COMP_MLME | COMP_AP), DBG_LOUD, ("===>AP mode Query/Set OID_DOT11_RESET_REQUEST: \n"));

	//
	// Should fail the request if the dotResetType is not dot11_reset_type_phy_and_mac.
	//
	if(pDot11ResetRequest->dot11ResetType != dot11_reset_type_phy_and_mac)
	{
		RT_TRACE( (COMP_OID_QUERY | COMP_OID_SET | COMP_MLME | COMP_AP), DBG_LOUD, 
			("Invalid reset type: %u.\n", pDot11ResetRequest->dot11ResetType));
		return NDIS_STATUS_INVALID_DATA;
	}

	//
	// Fail the query request if the value of this InformationBufferLength member of the MiniportOidRequest function 
	// is less than the length of the DOT11_STATUS_INDICATION structure
	//
	if ( OutputBufferLength < sizeof(DOT11_STATUS_INDICATION) )
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("OutputBufferLength too short\n"));
		*BytesNeeded = sizeof(DOT11_STATUS_INDICATION);
		return NDIS_STATUS_BUFFER_OVERFLOW;
	}

	//
	// Validate the buffer length
	//		
	if ( InputBufferLength < sizeof(DOT11_RESET_REQUEST) )
	{
		RT_TRACE(COMP_OID_SET, DBG_LOUD, ("InputBufferLength too short\n"));
		*BytesNeeded = sizeof(DOT11_RESET_REQUEST);
		return NDIS_STATUS_INVALID_LENGTH;
	}


	//
	// <Roger_Notes> When the reset operation is complete, the miniport driver must return a DOT11_STATUS_INDICATION 
	//structure to confirm the reset operation, so do NOT return ndisStatus immediately without status indication.
	// 2014.05.02.
	//	
	do
	{
		pMgntInfo->bResetInProgress = TRUE;

		if ( GetAPState(Adapter) ==AP_STATE_STARTED || GetAPState(Adapter) == AP_STATE_STARTING)
		{
			SetAPState(Adapter, AP_STATE_STOPPING);

			AP_DisassociateAllStation(Adapter, unspec_reason);
			
			SecSetSwEncryptionDecryption(Adapter, FALSE, FALSE);
			
			MgntActSet_ApType(Adapter, FALSE);
			AP_Reset(Adapter);	

			SetAPState(Adapter, AP_STATE_STOPPED);

			PlatformZeroMemory(&Adapter->pNdisCommon->dot11DesiredSSIDList, sizeof(DOT11_SSID_LIST));
			
			AP_AllPowerSaveReturn(Adapter);		
		}	

		MgntActSet_AdditionalBeaconIE(Adapter, NULL, 0);
		MgntActSet_AdditionalProbeRspIE(Adapter, NULL, 0);
		pNdis62Common->bWPSEnable = FALSE;

		if(Adapter->bvWifiStopBeforeSleep)
		{
			N62CApIndicateCanSustainAp(Adapter);
			Adapter->bvWifiStopBeforeSleep = FALSE;
			RT_TRACE_F(COMP_AP, DBG_LOUD, ("vWifi mode Done RESET.\n"));
		}

		//if(pDot11ResetRequest->bSetDefaultMIB)
		//	N6InitializeNative80211MIBs(Adapter);
		
		//
		// DDK: The NIC shall clear privacy exemption list on every reset 
		// regardless of the rest requests parameters.
		// 2008.10.17, haich
		//
		Adapter->pNdisCommon->PrivacyExemptionEntrieNum = 0;

	}while(FALSE);
	
	//
	// <Roger_Notes> After reset completed, return the status indication. The miniport driver must not set the value 
	// of the BytesWritten member of the OidRequest parameter, such set operation has been removed by me, 2014.05.02.
	//	
	PlatformZeroMemory(pDot11StatusIndication, sizeof(DOT11_STATUS_INDICATION));
	pDot11StatusIndication->uStatusType = DOT11_STATUS_RESET_CONFIRM;
	pDot11StatusIndication->ndisStatus = ndisStatus;		
	*BytesRead = sizeof(DOT11_RESET_REQUEST);
	*BytesWritten = sizeof(DOT11_STATUS_INDICATION);
	
	//
	// Reset network type. 
	//
	pMgntInfo->Regdot11networktype = RT_JOIN_NETWORKTYPE_INFRA;

	pMgntInfo->bResetInProgress = FALSE;
	
	RT_TRACE( (COMP_OID_QUERY | COMP_OID_SET | COMP_MLME | COMP_AP), DBG_LOUD, 
		("<===AP mode Query/Set OID_DOT11_RESET_REQUEST: %u\n", ndisStatus));
	return ndisStatus;
}

NDIS_STATUS
N62C_METHOD_OID_PM_GET_PROTOCOL_OFFLOAD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InputBufferLength,
	IN	ULONG			OutputBufferLength,
	IN	ULONG			MethodId,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
)
{
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	u4Byte	ProtocolOffloadId = *((ULONG *)InformationBuffer);
	NDIS_PM_PROTOCOL_OFFLOAD PMProtocolOffload;
	PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(&(pTargetAdapter->MgntInfo));
	PRT_AOAC_REPORT		pAOACReport = &(pPSC->AOACReport);

	if(OutputBufferLength < sizeof(NDIS_PM_PROTOCOL_OFFLOAD))
	{
		*BytesNeeded = sizeof(NDIS_PM_PROTOCOL_OFFLOAD);
		*BytesWritten = 0;
		//	DbgPrint("OutputBufferLength = %d < sizeof(NDIS_PM_PROTOCOL_OFFLOAD\n", OutputBufferLength);
		return NDIS_STATUS_BUFFER_TOO_SHORT;
	}
			
	PMProtocolOffload.ProtocolOffloadParameters.Dot11RSNRekeyParameters.KeyReplayCounter = pAOACReport->ReplayCounterOfEapolKey;

	PlatformMoveMemory(InformationBuffer, &PMProtocolOffload, sizeof(NDIS_PM_PROTOCOL_OFFLOAD));

	RT_PRINT_DATA( (COMP_OID_QUERY|COMP_POWER), DBG_LOUD, 
		("QUERY/SET OID_PM_GET_PROTOCOL_OFFLOAD: "), InformationBuffer, InputBufferLength );
	
	*BytesWritten = sizeof(NDIS_PM_PROTOCOL_OFFLOAD);	

	return ndisStatus;
}	
