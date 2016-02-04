#include "Mp_Precomp.h"

PMAC_HASH_ENTRY
lookupMacHashTable(
    __in PCMAC_HASH_TABLE Table,
    __in const DOT11_MAC_ADDRESS * MacKey
    )
/*++

Routine Description:

    Look up hash table
    Note: spinlock should held

Arguments:
    Table: The MAC hash table, must not be NULL.
    MacKey: The MAC address
    
Return Value:
    The found entry
    NULL Not found

--*/
{
    const LIST_ENTRY * head;
    PLIST_ENTRY entry;
    ULONG hash = HASH_MAC(*MacKey);
    PMAC_HASH_ENTRY hashEntry = NULL;
    PMAC_HASH_ENTRY macEntry;

    head = &Table->Buckets[hash];
    entry = head->Flink;
    while(head != entry) {
        macEntry = CONTAINING_RECORD(entry, MAC_HASH_ENTRY, Linkage); 
        if (memcmp(macEntry->MacKey, *MacKey, sizeof(DOT11_MAC_ADDRESS)) == 0) {
            hashEntry = macEntry;
            break;
        }
        entry = entry->Flink;
    }

    return hashEntry;
}

VOID
enumMacEntry(
    __in PMAC_HASH_TABLE Table,
    __in PENUM_MAC_ENTRY_CALLBACK CallbackFn,
    __in PVOID CallbackCtxt
    )
/*++

Routine Description:

    Enumerate hash table
    Note: spinlock should held

Arguments:
    Table: The MAC hash table, must not be NULL.
    CallbackFn: Pointer to the callback function, must not be NULL.
    CallbackCtxt: Pointer to the callback function context, will be passed to the callback function

Return Value:
    None
    
--*/
{
    int i;
    PLIST_ENTRY entry;
    PLIST_ENTRY head;
    PMAC_HASH_ENTRY macEntry;

    for(i = 0; i < MAC_HASH_BUCKET_NUMBER; i++) {
        head = Table->Buckets + i;
        entry = head->Flink;

        while(entry != head) {
            macEntry = CONTAINING_RECORD(entry, MAC_HASH_ENTRY, Linkage); 

            // Get the next entry before calling CallbackFn.
            //
            // CallbackFn may dereference macStateEntry and remove it from the
            // hash table!
            entry = entry->Flink;

            if (!CallbackFn(Table, macEntry, CallbackCtxt)) {
                return;
            }
        }
    }
}


#if 0
/** Reference AP port */
LONG
N62CAPRefPort(
	IN	PADAPTER		pAdapter
	)
{
	return InterlockedIncrement(&pAdapter->pNdis62Common->RefCount);
}

/** Dereference AP port */
LONG
N62CAPDerefPort(
	IN	PADAPTER		pAdapter
	)
{
	return InterlockedDecrement(&pAdapter->pNdis62Common->RefCount);
}
#endif

NDIS_STATUS
N62CStopApMode(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO pMgntInfo = &Adapter->MgntInfo;
	PRT_NDIS62_COMMON	pNdis62Common = Adapter->pNdis62Common;

	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;

	RT_TRACE( COMP_AP, DBG_LOUD, ("N62CStopApMode()====>\n") );

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

	//if(pDot11ResetRequest->bSetDefaultMIB)
	//	N6InitializeNative80211MIBs(Adapter);
	
	//
	// DDK: The NIC shall clear privacy exemption list on every reset 
	// regardless of the rest requests parameters.
	// 2008.10.17, haich
	//
	Adapter->pNdisCommon->PrivacyExemptionEntrieNum = 0;

	//
	// Beacon mode.
	//
	//pNdis62Common->bAPBeaconMode = FALSE;

	//
	// Reset network type. 
	//
	pMgntInfo->Regdot11networktype = RT_JOIN_NETWORKTYPE_INFRA;

	// indicate to OS that NOT support AP now.
	N62CApIndicateStopAp(Adapter);

	RT_TRACE( COMP_AP, DBG_LOUD, ("N62CStopApMode()<====\n") );
	return ndisStatus;
}

NDIS_STATUS
N62CStartApMode(
	IN PADAPTER	Adapter
	)
{
	PMGNT_INFO pMgntInfo = &(Adapter->MgntInfo);
	PADAPTER	DefAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO	pDefaultMgntInfo = GetDefaultMgntInfo(Adapter);
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);
	NDIS_STATUS ndisStatus = NDIS_STATUS_SUCCESS;

	FunctionIn(COMP_AP);
	
	if(!ACTING_AS_AP(Adapter))
	{ 
		AP_AllPowerSaveDisable(Adapter);
	}
	
	MgntActSet_ApType(Adapter, TRUE);

	if( pSecInfo->SecLvl > RT_SEC_LVL_NONE && !Adapter->bInHctTest)
	{// hw
		SecSetSwEncryptionDecryption(Adapter, FALSE , FALSE);
	}
	else
	{// sw
		SecSetSwEncryptionDecryption(Adapter, TRUE, TRUE);
	}

	SetAPState(Adapter, AP_STATE_STARTING);
	
	N6InitializeIndicateStateMachine(Adapter);
	
	//
	// This is for Win7 DTM.
	//
	{
		PDOT11_SSID_LIST pSsidList = &(Adapter->pNdisCommon->dot11DesiredSSIDList);
		CopyMem(pMgntInfo->Ssid.Octet, pSsidList->SSIDs[0].ucSSID, pSsidList->SSIDs[0].uSSIDLength);
			pMgntInfo->Ssid.Length =(u1Byte) pSsidList->SSIDs[0].uSSIDLength;		
	}

#if 0
	DefAdapter->MgntInfo.bStartApDueToWakeup=FALSE;	// don't need to do for ext
#else 	 //change by ylb 20111124 to Fix DTM error: Vertify Beacon after WakeUp
		{
			PADAPTER pTargetAdapter = GetDefaultAdapter(Adapter);

			while(pTargetAdapter != NULL)
			{
				pTargetAdapter->MgntInfo.bStartApDueToWakeup=FALSE;
				
				pTargetAdapter = GetNextExtAdapter(pTargetAdapter);
			}
		}
#endif

	AP_Reset(Adapter);

	SetAPState(Adapter, AP_STATE_STARTED);

/*	Shoule we setting cahnnel num before configure AP mode ?
Neo Test 123
	if(IsDefaultAdapter(Adapter))
	{
		if((GetpExtMgntInfo(Adapter)->mAssoc || GetpExtMgntInfo(Adapter)->mIbss) && (!ACTING_AS_AP(GetExtAdapter(Adapter))))
		{
			pDefaultMgntInfo->dot11CurrentChannelNumber = GetpExtMgntInfo(Adapter)->dot11CurrentChannelNumber;
		}
// 		else ??   Neo Test 123
	}
	else
	{
		if((pDefaultMgntInfo->mAssoc || pDefaultMgntInfo->mIbss) && (!ACTING_AS_AP(GetDefaultAdapter(Adapter))))
		{
			pDefaultMgntInfo->dot11CurrentChannelNumber =pDefaultMgntInfo->dot11CurrentChannelNumber;
		}
//		else ??  Neo Test 123

	}
*/

	N62CAPIndicateFrequencyAdopted(Adapter);

	FunctionOut(COMP_AP);

	
	return ndisStatus;

}

VOID
N62CApIndicateStatus(
	IN	PADAPTER		pAdapter,
	IN	NDIS_STATUS		GeneralStatus,
	IN	PVOID			RequestID,
	IN	PVOID			StatusBuffer,
	IN	UINT			StatusBufferSize
	)
{
	NDIS_HANDLE		MiniportAdapterHandle = pAdapter->pNdisCommon->hNdisAdapter;
	NDIS_STATUS_INDICATION		StatusIndication;

	PlatformZeroMemory(&StatusIndication, sizeof(NDIS_STATUS_INDICATION));

	N6_ASSIGN_OBJECT_HEADER(
		StatusIndication.Header,
		NDIS_OBJECT_TYPE_STATUS_INDICATION,
		NDIS_STATUS_INDICATION_REVISION_1,
		sizeof(NDIS_STATUS_INDICATION));

	StatusIndication.PortNumber = pAdapter->pNdis62Common->PortNumber;
	StatusIndication.SourceHandle = MiniportAdapterHandle;
	StatusIndication.StatusCode = GeneralStatus;
	
	StatusIndication.DestinationHandle = NULL;
	StatusIndication.RequestId = RequestID;	
	
	StatusIndication.StatusBuffer = StatusBuffer;
	StatusIndication.StatusBufferSize = StatusBufferSize;

	RT_TRACE(COMP_INDIC, DBG_LOUD, ("N62CApIndicateStatus(): StatusIndication.PortNumber: %d\n", StatusIndication.PortNumber));

	NdisMIndicateStatusEx(MiniportAdapterHandle, &StatusIndication);
}

VOID 
N62CApIndicateStopAp(
	IN	PADAPTER		Adapter
    )
{
	DOT11_STOP_AP_PARAMETERS	dot11StopApParameters;

	RT_TRACE((COMP_AP|COMP_INDIC), DBG_LOUD, ("===> N62CApIndicateStopAp()\n"));
	
	NdisZeroMemory(&dot11StopApParameters, sizeof(DOT11_STOP_AP_PARAMETERS));

	N6_ASSIGN_OBJECT_HEADER(
		dot11StopApParameters.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_STOP_AP_PARAMETERS_REVISION_1,
		sizeof(DOT11_STOP_AP_PARAMETERS));

	dot11StopApParameters.ulReason = DOT11_STOP_AP_REASON_CHANNEL_NOT_AVAILABLE;
    
	N62CApIndicateStatus(
			Adapter,
			NDIS_STATUS_DOT11_STOP_AP,
			NULL,
			&dot11StopApParameters,
			sizeof(DOT11_STOP_AP_PARAMETERS));

	RT_TRACE((COMP_AP|COMP_INDIC), DBG_LOUD, ("<=== N62CApIndicateStopAp()\n"));
}

VOID 
N62CApIndicateCanSustainAp(
	IN	PADAPTER		Adapter
    )
{
	DOT11_CAN_SUSTAIN_AP_PARAMETERS dot11CanSustainApParameters;

	RT_TRACE((COMP_AP|COMP_INDIC), DBG_LOUD, ("===> N62CApIndicateCanSustainAp()\n"));
	
	NdisZeroMemory(&dot11CanSustainApParameters, sizeof(DOT11_CAN_SUSTAIN_AP_PARAMETERS));

	N6_ASSIGN_OBJECT_HEADER(
		dot11CanSustainApParameters.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_CAN_SUSTAIN_AP_PARAMETERS_REVISION_1,
		sizeof(DOT11_CAN_SUSTAIN_AP_PARAMETERS));

	dot11CanSustainApParameters.ulReason = DOT11_CAN_SUSTAIN_AP_REASON_IHV_START;
    
	N62CApIndicateStatus(
			Adapter,
			NDIS_STATUS_DOT11_CAN_SUSTAIN_AP,
			NULL,
			&dot11CanSustainApParameters,
			sizeof(DOT11_CAN_SUSTAIN_AP_PARAMETERS));

	RT_TRACE((COMP_AP|COMP_INDIC), DBG_LOUD, ("<=== N62CApIndicateCanSustainAp()\n"));
}


/*
 *2008/08/22 Add by Mars
 * 
 * Indicate Channel to NDIS for Win7 V3
 *
 * @param	pAdapter is driver main strucutre.  
*/
VOID 
N62CAPIndicateFrequencyAdopted(
	IN	PADAPTER		Adapter
    )
{
    	DOT11_PHY_FREQUENCY_ADOPTED_PARAMETERS params;
	PRT_NDIS62_COMMON pN62COMMON = Adapter->pNdis62Common;
 
    	// send out the NDIS_STATUS_DOT11_PHY_FREQUENCY_ADOPTED status indication
    	NdisZeroMemory(&params, sizeof(DOT11_PHY_FREQUENCY_ADOPTED_PARAMETERS));

    	N6_ASSIGN_OBJECT_HEADER(
        	params.Header,
        	NDIS_OBJECT_TYPE_DEFAULT,
        	DOT11_PHY_FREQUENCY_ADOPTED_PARAMETERS_REVISION_1,
        	sizeof(DOT11_PHY_FREQUENCY_ADOPTED_PARAMETERS)
        	);

    	params.ulChannel = Adapter->MgntInfo.dot11CurrentChannelNumber;
    	params.ulPhyId     = 0;
    
    	N62CApIndicateStatus(
        	Adapter, 
        	NDIS_STATUS_DOT11_PHY_FREQUENCY_ADOPTED, 
        	NULL,                   // no request ID
        	&params, 
        	sizeof(params)
        	);
}

//
// Assumption:
//		pMgntInfo->pCurrentSta is updated.
//
VOID
N62CAPIndicateIncomAssocStart(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PADAPTER	pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO 	pDefMgntInfo =GetDefaultMgntInfo(Adapter);
	PRT_WLAN_STA	pCurrentSta = pMgntInfo->pCurrentSta;
	PDOT11_INCOMING_ASSOC_STARTED_PARAMETERS	pAssoStartParam;
	RT_STATUS		rtStatus;
	ULONG			AllocSize = sizeof(DOT11_INCOMING_ASSOC_STARTED_PARAMETERS);

	RT_TRACE(COMP_INDIC, DBG_LOUD, ("===> N62CAPIndicateIncomAssocStart ()\n"));
	RT_PRINT_ADDR(COMP_INDIC, DBG_LOUD, "", pCurrentSta->MacAddr);

	if(PlatformAllocateMemory(Adapter, &pAssoStartParam, AllocSize) != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INDIC, DBG_WARNING, 
			("N62CAPIndicateIncomAssocStart() failed to allocate memory for DOT11_INCOMING_ASSOC_STARTED_PARAMETERS\n"));
		return;
	}
	
	PlatformZeroMemory(pAssoStartParam, sizeof(DOT11_INCOMING_ASSOC_STARTED_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
		pAssoStartParam->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_INCOMING_ASSOC_STARTED_PARAMETERS_REVISION_1,
		sizeof(DOT11_INCOMING_ASSOC_STARTED_PARAMETERS));
	
	PlatformMoveMemory(
		(pAssoStartParam->PeerMacAddr),
		pCurrentSta->MacAddr,
		sizeof(DOT11_MAC_ADDRESS));

	N62CApIndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_INCOMING_ASSOC_STARTED,
		NULL,
		pAssoStartParam,
		sizeof(DOT11_INCOMING_ASSOC_STARTED_PARAMETERS));

	PlatformFreeMemory(pAssoStartParam, AllocSize);
	RT_TRACE(COMP_INDIC, DBG_LOUD, ("<=== N62CAPIndicateIncomAssocStart ()\n"));	
}

//
// Assumption:
//		pMgntInfo->pCurrentSta is updated.
//
VOID
N62CAPIndicateIncomAssocComplete(
	IN	PADAPTER		Adapter,
	IN	RT_STATUS		status
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO 		pDefMgntInfo =	GetDefaultMgntInfo(Adapter);
	PRT_WLAN_STA	pCurrentSta = pMgntInfo->pCurrentSta;
	PRT_SECURITY_T	pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
	PRT_NDIS62_COMMON	pNdis62Common = Adapter->pNdis62Common;
	RT_AUTH_MODE	authmode;
	PDOT11_INCOMING_ASSOC_COMPLETION_PARAMETERS	pAssoCompleteParam;
	RT_STATUS		rtStatus;
	ULONG			AllocSize;
	ULONG			IndicateSize;

	pu1Byte pInfoStart = NULL;
	int nInfoOffset = 0;
	
	RT_TRACE(COMP_INDIC, DBG_LOUD, ("===> N62CAPIndicateIncomAssocComplete ()\n"));	
	RT_PRINT_ADDR(COMP_INDIC, DBG_LOUD, "", pCurrentSta->MacAddr);

	//
	// Allocate memory for Phy IE, Asoc req and Asoc rsp.
	// Note that if we don't have Asoc req or Asoc rsp, the spaces for them are not allocated.
	// 
	AllocSize = sizeof(DOT11_INCOMING_ASSOC_COMPLETION_PARAMETERS) +		// The structure itself.
		sizeof(u4Byte)	+													// PHY ID.
		MMPDU_BODY_LEN(pCurrentSta->AP_RecvAsocReqLength) +				// Asoc Req.
		MMPDU_BODY_LEN(pCurrentSta->AP_SendAsocRespLength)+				// Asoc Rsp.
		MMPDU_BODY_LEN(pMgntInfo->beaconframe.Length);					// Beacon.

	//
	// Note that in NDISTest, we should not indicate pAssoCompleteParam with size greater than 1024.
	// 2008.11.21, haich.
	//
	//RT_ASSERT(AllocSize <= 1024, 
	//	("N62CAPIndicateIncomAssocComplete(): Indicate a buffer with size greater than 1024.\n"));
	if(AllocSize > 1024)
	{// ignore this indication.
		RT_TRACE(COMP_MLME, DBG_WARNING, 
			("<=== DrvIFIndicateAssociationComplete(): AllocSize > 1024, ignore indication.\n"));
		return;
	}

	rtStatus = PlatformAllocateMemory(
				Adapter,
				&pAssoCompleteParam,
				AllocSize);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_MLME, DBG_WARNING, 
			("<=== DrvIFIndicateAssociationComplete(): failed to allocate memory for indication.\n"));
		return;
	}

	//
	// Clear all fields.
	//
	PlatformZeroMemory(pAssoCompleteParam, AllocSize);
	
	//
	// Fill Header.
	//
	N6_ASSIGN_OBJECT_HEADER(
		pAssoCompleteParam->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_INCOMING_ASSOC_COMPLETION_PARAMETERS_REVISION_1,
		sizeof(DOT11_INCOMING_ASSOC_COMPLETION_PARAMETERS));

	//
	// Fill MAC address.
	//
	PlatformMoveMemory(
		pAssoCompleteParam->PeerMacAddr,
		pCurrentSta->MacAddr,
		sizeof(DOT11_MAC_ADDRESS));
	//RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "Peer:", pAssoCompleteParam->PeerMacAddr);

	//
	// Fill status.
	//
	pAssoCompleteParam->uStatus = (status == RT_STATUS_SUCCESS) ? 
		(DOT11_ASSOC_STATUS_SUCCESS) : 
		(DOT11_ASSOC_STATUS_FAILURE);

	if(status == RT_STATUS_SUCCESS)
	{// fill fields that are valid only when association succeed in this code section.
		//
		// Is Re association req/rsp. 
		// We may get here because we received a Deauth.
		// For this case, AP_RecvAsocReq and AP_SendAsocResp are all NULL since
		// they are released right before we get out of AP_OnAsocReq().
		// 2008.11.21, haich.
		//
		if(pCurrentSta->AP_RecvAsocReqLength)
		{
			OCTET_STRING osResvAssocReq;
			osResvAssocReq.Length = (u2Byte)pCurrentSta->AP_RecvAsocReqLength;
			osResvAssocReq.Octet = pCurrentSta->AP_RecvAsocReq;
			pAssoCompleteParam->bReAssocReq = 
				(PacketGetType(osResvAssocReq) == Type_Reasoc_Req) ? (TRUE) : (FALSE);
		}
		else
		{
			pAssoCompleteParam->bReAssocReq = FALSE;
		}
		
		if(pCurrentSta->AP_SendAsocRespLength)
		{
			OCTET_STRING osResvAssocReq;
			osResvAssocReq.Length = (u2Byte)pCurrentSta->AP_RecvAsocReqLength;
			osResvAssocReq.Octet = pCurrentSta->AP_RecvAsocReq;
			pAssoCompleteParam->bReAssocResp =
				(PacketGetType(osResvAssocReq) == Type_Reasoc_Req) ? (TRUE) : (FALSE);
		}
		else 
		{
			pAssoCompleteParam->bReAssocResp = FALSE;
		}
		
		// DOT11_AUTH_ALGORITHM  AuthAlgo;
//		MgntActQuery_802_11_AUTHENTICATION_MODE( Adapter, &authmode );
//		pAssoCompleteParam->AuthAlgo = N6CAuthModeToDot11( &authmode );
//		pAssoCompleteParam->AuthAlgo = N6CAuthAlgToDot11(&(pCurrentSta->AuthAlg));
		pAssoCompleteParam->AuthAlgo = N6CAuthModeToDot11(&(pCurrentSta->AuthMode));

		// DOT11_CIPHER_ALGORITHM  UnicastCipher;
		pAssoCompleteParam->UnicastCipher = N6CEncAlgorithmToDot11( &(pCurrentSta->perSTAKeyInfo.PairwiseCipherSuite[0]) );

		// DOT11_CIPHER_ALGORITHM  MulticastCipher;
		pAssoCompleteParam->MulticastCipher = N6CEncAlgorithmToDot11( &(pCurrentSta->perSTAKeyInfo.GroupCipherSuite) );

		//
		// Set up Extra information.
		//
		pInfoStart = (pu1Byte)pAssoCompleteParam;
		nInfoOffset = sizeof(DOT11_INCOMING_ASSOC_COMPLETION_PARAMETERS);

		//
		// PHY ID. Filled after DOT11_INCOMING_ASSOC_COMPLETION_PARAMETERS structure. 
		//
		pAssoCompleteParam->uActivePhyListSize = sizeof(u4Byte);
		pAssoCompleteParam->uActivePhyListOffset = nInfoOffset;
		{
			PULONG	pLong = (PULONG)((PUCHAR)pAssoCompleteParam + sizeof(DOT11_INCOMING_ASSOC_COMPLETION_PARAMETERS));
			*pLong = 0;
		}				
		nInfoOffset += pAssoCompleteParam->uActivePhyListSize;
		
		//
		// Association Request, filled immediately after PHY ID.
		//
		if(pCurrentSta->AP_RecvAsocReqLength)
		{
			pAssoCompleteParam->uAssocReqOffset = nInfoOffset;
			pAssoCompleteParam->uAssocReqSize = MMPDU_BODY_LEN(pCurrentSta->AP_RecvAsocReqLength); 			
            PlatformMoveMemory((pInfoStart+nInfoOffset), 
                               MMPDU_BODY(pCurrentSta->AP_RecvAsocReq), 
                               pAssoCompleteParam->uAssocReqSize);
			nInfoOffset += pAssoCompleteParam->uAssocReqSize;

			RT_PRINT_DATA(COMP_INDIC, DBG_LOUD, "AsocReq", 
				MMPDU_BODY(pCurrentSta->AP_RecvAsocReq), 
				MMPDU_BODY_LEN(pCurrentSta->AP_RecvAsocReqLength));
		}
		else
		{
			pAssoCompleteParam->uAssocReqOffset = 0;
			pAssoCompleteParam->uAssocReqSize = 0;
		}
		
		//
		// Association Response.
		//
		if(pCurrentSta->AP_SendAsocRespLength)
		{
			pAssoCompleteParam->uAssocRespOffset = nInfoOffset;
			pAssoCompleteParam->uAssocRespSize = MMPDU_BODY_LEN(pCurrentSta->AP_SendAsocRespLength);
			PlatformMoveMemory((pInfoStart+nInfoOffset), 
							MMPDU_BODY(pCurrentSta->AP_SendAsocResp), 
							pAssoCompleteParam->uAssocRespSize);
			nInfoOffset += pAssoCompleteParam->uAssocRespSize;

			RT_PRINT_DATA(COMP_INDIC, DBG_LOUD, "AsocRsp", 
				MMPDU_BODY(pCurrentSta->AP_SendAsocResp), 
				MMPDU_BODY_LEN(pCurrentSta->AP_SendAsocRespLength));
		}
		else
		{
			pAssoCompleteParam->uAssocRespOffset = 0;
			pAssoCompleteParam->uAssocRespSize = 0;
		}

		//
		// Beacon frame.
		//
		if(pMgntInfo->beaconframe.Length)
		{
			//vivi add this for vitrual ap.as pcie add 8 byte before mac header as early mode, 20101130
			HAL_DATA_TYPE		*pHalData		= GET_HAL_DATA(Adapter);
			if(	pHalData->AMPDUBurstMode && IS_NEED_OFFSET_ON_AMPDU(Adapter))
			{	
				pAssoCompleteParam->uBeaconOffset = nInfoOffset;
				pAssoCompleteParam->uBeaconSize = MMPDU_BODY_LEN(pMgntInfo->beaconframe.Length-8);
				PlatformMoveMemory((pInfoStart+nInfoOffset), 
								MMPDU_BODY(pMgntInfo->beaconframe.Octet+8), 
								pAssoCompleteParam->uBeaconSize-8);
				nInfoOffset += (pAssoCompleteParam->uBeaconSize-8);

				RT_PRINT_DATA(COMP_INDIC, DBG_LOUD, "Beacon", 
					MMPDU_BODY(pMgntInfo->beaconframe.Octet+8), 
					MMPDU_BODY_LEN(pMgntInfo->beaconframe.Length-8));			
			}	
			else
			{
			        pAssoCompleteParam->uBeaconOffset = nInfoOffset;
			        pAssoCompleteParam->uBeaconSize = MMPDU_BODY_LEN(pMgntInfo->beaconframe.Length);
			        PlatformMoveMemory((pInfoStart+nInfoOffset), 
							MMPDU_BODY(pMgntInfo->beaconframe.Octet), 
							pAssoCompleteParam->uBeaconSize);
			        nInfoOffset += pAssoCompleteParam->uBeaconSize;

			        RT_PRINT_DATA(COMP_INDIC, DBG_LOUD, "Beacon", 
				        MMPDU_BODY(pMgntInfo->beaconframe.Octet), 
				        MMPDU_BODY_LEN(pMgntInfo->beaconframe.Length));
		        }
		}
		else
		{
			pAssoCompleteParam->uBeaconOffset = 0;
			pAssoCompleteParam->uBeaconSize = 0;
		}
	}
	else
	{// fill fields that are valid only when association failed in this code section.
		// TODO: Win7 error source.
		pAssoCompleteParam->ucErrorSource = DOT11_ASSOC_ERROR_SOURCE_OS;
	}
	
	N62CApIndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_INCOMING_ASSOC_COMPLETION,
		NULL,
		pAssoCompleteParam,
		AllocSize);

	//2 NOTE: Do NOT access pAssoCompleteParam hereafter.
	PlatformFreeMemory(pAssoCompleteParam, AllocSize);
	
	RT_TRACE(COMP_INDIC, DBG_LOUD, ("<=== N62CAPIndicateIncomAssocComplete (): status(%u)\n", status));	
}

//
// Assumption:
//		pMgntInfo->pCurrentSta is updated.
//
VOID
N62CAPIndicateIncomAssocReqRecv(
	IN	PADAPTER		Adapter	
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO 			pDefMgntInfo = GetDefaultMgntInfo(Adapter);
	PRT_WLAN_STA		pCurrentSta = pMgntInfo->pCurrentSta;
	pu1Byte				Framebody=NULL;
	u4Byte				FrameSize=0;
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;
	PDOT11_INCOMING_ASSOC_REQUEST_RECEIVED_PARAMETERS	pAssoReqRecvParam;
	ULONG				 requiredAssocReqParaSize = 0;
	
	RT_TRACE(COMP_INDIC, DBG_LOUD, ("===> N62CAPIndicateIncomAssocReqRecv ()\n"));
	RT_PRINT_ADDR(COMP_INDIC, DBG_LOUD, "", pCurrentSta->MacAddr);

	FrameSize = MMPDU_BODY_LEN(pCurrentSta->AP_RecvAsocReqLength); 
	requiredAssocReqParaSize = sizeof(DOT11_INCOMING_ASSOC_REQUEST_RECEIVED_PARAMETERS)+FrameSize;

	if(requiredAssocReqParaSize > 1024)
	{// not to indicate with a too large buffer. 2008.11.21, haich.
		RT_TRACE(COMP_INDIC, DBG_LOUD, 
			("<=== DrvIFIndicateAssociationComplete(): requiredAssocReqParaSize > 1024, ignore indication.\n"));
		return;
	}

	rtStatus = PlatformAllocateMemory(
				Adapter,
				&pAssoReqRecvParam,
				requiredAssocReqParaSize);
	if (rtStatus != RT_STATUS_SUCCESS)
	{
		RT_TRACE(COMP_INDIC, DBG_LOUD, 
			("<=== DrvIFIndicateAssociationComplete(): failed to allocate memory for indication.\n"));
		return;
	}

	PlatformZeroMemory(pAssoReqRecvParam,requiredAssocReqParaSize);
	
	N6_ASSIGN_OBJECT_HEADER(
		pAssoReqRecvParam->Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_INCOMING_ASSOC_REQUEST_RECEIVED_PARAMETERS_REVISION_1,
		sizeof(DOT11_INCOMING_ASSOC_REQUEST_RECEIVED_PARAMETERS));

	{
		OCTET_STRING osResvAssocReq;
		osResvAssocReq.Length = (u2Byte)pCurrentSta->AP_RecvAsocReqLength;
		osResvAssocReq.Octet = pCurrentSta->AP_RecvAsocReq;
	pAssoReqRecvParam->bReAssocReq =
			(PacketGetType(osResvAssocReq) == Type_Reasoc_Req) ? (TRUE) : (FALSE);
	}
	PlatformMoveMemory(
		pAssoReqRecvParam->PeerMacAddr,
		pCurrentSta->MacAddr,
		sizeof(DOT11_MAC_ADDRESS));

	
	pAssoReqRecvParam->uAssocReqSize = FrameSize;
	pAssoReqRecvParam->uAssocReqOffset = sizeof(DOT11_INCOMING_ASSOC_REQUEST_RECEIVED_PARAMETERS);

	PlatformMoveMemory(
		Add2Ptr(pAssoReqRecvParam, pAssoReqRecvParam->uAssocReqOffset),
		MMPDU_BODY(pCurrentSta->AP_RecvAsocReq),
		FrameSize);
	
	//RT_PRINT_ADDR(COMP_MLME, DBG_LOUD, "Peer:", pAsocInfo->PeerAddr);
	//RT_PRINT_STR(COMP_MLME, DBG_LOUD, "SSID", pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length);
	//RT_PRINT_DATA(COMP_INDIC, 
	//	DBG_LOUD, 
	//	"Indicated data:", 
	//	pAssoReqRecvParam, 
	//	(int)(sizeof(DOT11_INCOMING_ASSOC_REQUEST_RECEIVED_PARAMETERS)+FrameSize));
	//
	N62CApIndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_INCOMING_ASSOC_REQUEST_RECEIVED,
		NULL,
		pAssoReqRecvParam,
		sizeof(DOT11_INCOMING_ASSOC_REQUEST_RECEIVED_PARAMETERS)+FrameSize);

	PlatformFreeMemory(pAssoReqRecvParam, requiredAssocReqParaSize);
	RT_TRACE(COMP_INDIC, DBG_LOUD, ("<=== N62CAPIndicateIncomAssocReqRecv ()\n"));	

}

VOID
N62CAPIndicateDisassociation(
	IN	PADAPTER		Adapter,
	IN	u2Byte			reason
	)
{
	DOT11_DISASSOCIATION_PARAMETERS DisAssoParam;
	PADAPTER			pDefaultAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PMGNT_INFO 			pDefMgntInfo = GetDefaultMgntInfo(Adapter);
	
	PRT_WLAN_STA		pCurrentSta = pMgntInfo->pCurrentSta;
	ULONG	i;

	RT_TRACE(COMP_INDIC, DBG_LOUD, ("===> N62CAPIndicateDisassociation()\n"));

	PlatformZeroMemory(&DisAssoParam, sizeof(DOT11_DISASSOCIATION_PARAMETERS));
	
	N6_ASSIGN_OBJECT_HEADER(
		DisAssoParam.Header,
		NDIS_OBJECT_TYPE_DEFAULT,
		DOT11_DISASSOCIATION_PARAMETERS_REVISION_1,
		sizeof(DOT11_DISASSOCIATION_PARAMETERS));

	RT_ASSERT(pMgntInfo->OpMode == RT_OP_MODE_AP, 
		("N62CAPIndicateDisassociation(): not in AP mode."));

	if(pCurrentSta)
	{
		PlatformMoveMemory(
			DisAssoParam.MacAddr,
			pCurrentSta->MacAddr,
			sizeof(DOT11_MAC_ADDRESS));
	}
	else
	{
		N6_MAKE_WILDCARD_MAC_ADDRESS(DisAssoParam.MacAddr);
	}

	if(reason == disas_lv_ss)
		DisAssoParam.uReason = DOT11_ASSOC_STATUS_PEER_DISASSOCIATED | reason;
	else if (reason == deauth_lv_ss)
	DisAssoParam.uReason = DOT11_ASSOC_STATUS_PEER_DEAUTHENTICATED | reason;
	else
		DisAssoParam.uReason = reason;

	DisAssoParam.uIHVDataOffset = 0;
	DisAssoParam.uIHVDataSize = 0;

	N6IndicateStatus(
		Adapter,
		NDIS_STATUS_DOT11_DISASSOCIATION,
		&DisAssoParam,
		sizeof(DOT11_DISASSOCIATION_PARAMETERS));

	RT_TRACE(COMP_INDIC, DBG_LOUD, ("<=== N62CAPIndicateDisassociation()\n"));

}

BOOLEAN
N62CAPInComingAssocDecsion(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	asocpdu	
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	BOOLEAN			allowed=TRUE;
	u4Byte			idx=0;
	
	if( pMgntInfo->LockedSTACount != 0 )
	{
		allowed = TRUE;
		for( idx=0; idx<pMgntInfo->LockedSTACount; idx++ )
		{
			if( eqMacAddr(pMgntInfo->LockedSTAList[idx], Frame_Addr3(asocpdu)) )
			{
				RT_PRINT_ADDR( COMP_AP, DBG_LOUD, ("Mached Locked STA Address"), pMgntInfo->LockedSTAList[idx] );
				allowed = FALSE;
				break;
			}
		}

		if( !allowed )
		{
			RT_TRACE( COMP_AP, DBG_LOUD, ("AP_OnAuthOdd(): unknow STA MAC address!\n") );
			return FALSE;
		}
	}
	return TRUE;
}

VOID
N62CResetAPVariables(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			IsApMode
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	int				i;
	
	// Added by Annie, 2006-01-26.
	MgntCancelAllTimer(Adapter);
	RemoveAllTS(Adapter);
	pMgntInfo->dot11CurrentChannelNumber = pMgntInfo->Regdot11ChannelNumber;

	// <RJ_TODO> We shall randomize it? 
	for(i = 0; i < 128; i++)
	{
		pMgntInfo->arChalng[i] = (u1Byte)i;
	}

	// Reset Association Table.
	AsocEntry_ResetAll(Adapter);

	Authenticator_GlobalReset(Adapter);	// added by Annie

	if(IsApMode)
	{ // AP mode.
		if(MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED)
		{
			static u1Byte DummySta[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

			//
			// step 1. Indicate wildcard disassocation event to 
			// disassociate all STA associated before.
			//
			/*****************************************************************
			20150416 Sinda. 
			We need to delete peer in WDI architecture, but peer had been deleted before.
			pass NULL to skip this case to avoid delete incorrect peer.
			*****************************************************************/
			DrvIFIndicateDisassociation(Adapter, unspec_reason, NULL); 

			//
			// step 2. Indicate dummy STA assocation event.
			//
			// 061228, rcnjko: UI need fake AP mode to enter connected 
			// state immediate for ICS easy implementation.
			//
			MgntUpdateAsocInfo(Adapter, UpdateAsocPeerAddr, DummySta, 6);
			DrvIFIndicateAssociationStart(Adapter);
			DrvIFIndicateAssociationComplete(Adapter, RT_STATUS_SUCCESS);
		}

		//
		// Clean up additional beacon ie and response ie data.
		//
		MgntActSet_AdditionalBeaconIE(Adapter, NULL, 0);
		MgntActSet_AdditionalProbeRspIE(Adapter, NULL, 0);
	}
}


VOID
N62CAPClearStateBeforeSleep(
		PADAPTER	Adapter
	)
{
		PADAPTER	pAdapter = GetFirstExtAdapter(Adapter);


		while(pAdapter != NULL)
		{
			if ( GetAPState(pAdapter) ==AP_STATE_STARTED || GetAPState(pAdapter) == AP_STATE_STARTING)
			{
				SetAPState(pAdapter, AP_STATE_STOPPING);
				AP_DisassociateAllStation(pAdapter, unspec_reason);
				SecSetSwEncryptionDecryption(pAdapter, FALSE, FALSE);
				MgntActSet_ApType(pAdapter, FALSE);			
				AP_Reset(pAdapter);	
				SetAPState(pAdapter, AP_STATE_STOPPED);
				pAdapter->pNdis62Common->CurrentOpState=INIT_STATE;
				PlatformZeroMemory(&pAdapter->pNdisCommon->dot11DesiredSSIDList, sizeof(DOT11_SSID_LIST));
			}	
			
			if(pAdapter->MgntInfo.AdditionalBeaconIEData)
			{
				PlatformFreeMemory(pAdapter->MgntInfo.AdditionalBeaconIEData, pAdapter->MgntInfo.AdditionalBeaconIESize);
				pAdapter->MgntInfo.AdditionalBeaconIEData = NULL;
				pAdapter->MgntInfo.AdditionalBeaconIESize=0;
			}
			
			if(pAdapter->MgntInfo.AdditionalResponseIEData)
			{
				PlatformFreeMemory(pAdapter->MgntInfo.AdditionalResponseIEData, pAdapter->MgntInfo.AdditionalResponseIESize);
				pAdapter->MgntInfo.AdditionalResponseIEData = NULL;
				pAdapter->MgntInfo.AdditionalResponseIESize=0;
			}
			pAdapter = GetNextExtAdapter(pAdapter);
		}		

}

