#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "Authenticator.tmh"
#endif

//------------------------------------------------------------------------------
// Authenticator related operations.
//------------------------------------------------------------------------------


// On event


// Description:
// Output:
// Modify: 
void 
Authenticator_OnDeauthenticationRequest(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt
	)
{
	pKeyMgnt->EvntID = ASMEID_DeauthenticationRequest;

	Authenticator_StateDISCONNECTED(Adapter, pKeyMgnt->pWLanSTA);
}


// Description:
// Output:
// Modify: 
void 
Authenticator_OnAuthenticationRequest(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt
	)
{
	pKeyMgnt->EvntID = ASMEID_AuthenticationRequest;

	Authenticator_StateAUTHENTICATION(Adapter, pKeyMgnt->pWLanSTA);
}


// Description:
// Output:
// Modify: 
void 
Authenticator_OnReAuthenticationRequest(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt
	)
{
	pKeyMgnt->EvntID = ASMEID_ReAuthenticationRequest;

	Authenticator_StateAUTHENTICATION2(Adapter, pKeyMgnt->pWLanSTA);
}




// Description: According received EAPOL-key, enter the next state.
// Output: void
// Modify: Annie, 2005-07-02
//		Discard using condition pKeyMgnt->bPTKInstalled.
//		Instead, I add a macro KeyMgntStateIsWaitingEAPOLKey to check the state.
void 
Authenticator_OnEAPOLKeyRecvd(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt,
	IN	OCTET_STRING			pdu
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PAUTH_GLOBAL_KEY_TAG	pGlInfo = &pMgntInfo->globalKeyInfo;
	PRT_WLAN_STA		pEntry = pKeyMgnt->pWLanSTA;
	pu1Byte				pSTA_addr = Frame_pSaddr(pdu);
	pu1Byte				pAP_addr = Frame_pDaddr(pdu);
	PEAPOL_KEY_STRUCT	eapol_key_recvd;
	OCTET_STRING		SNonce;
	OCTET_STRING		RSNIE;
	MsgType				msg_type = type_unknow;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_OnEAPOLKeyRecvd()\n") );

	pKeyMgnt->EvntID = ASMEID_EAPOLKeyRecvd;

	FillOctetString(pGlInfo->EapolKeyMsgRecvd,								\
		pGlInfo->EAPOLMsgRecvd.Octet+LIB1X_EAPOL_HDRLEN,					\
		pGlInfo->EAPOLMsgRecvd.Length-LIB1X_EAPOL_HDRLEN );				\

	eapol_key_recvd = (PEAPOL_KEY_STRUCT)pGlInfo->EapolKeyMsgRecvd.Octet;

	//PRINT_DATA( ("EapolKeyMsgRecvd: "), pGlInfo->EapolKeyMsgRecvd.Octet, pGlInfo->EapolKeyMsgRecvd.Length);
	RSNIE.Octet = NULL;
	RSNIE.Length = 0;
	
	// Get the message number.
	if( Message_KeyType(pGlInfo->EapolKeyMsgRecvd) == type_Pairwise )
	{
		if( (Message_Error(pGlInfo->EapolKeyMsgRecvd) == 1) &&
			(Message_Request(pGlInfo->EapolKeyMsgRecvd) == 1))
		{			
			//Enter integrity failure state...			
			Authenticator_StateINTEGRITYFAILURE(Adapter, pEntry);	
		}

		if( (eapol_key_recvd->key_info[0]==0x01 && eapol_key_recvd->key_info[1]==0x09) ||
		    ( eapol_key_recvd->key_info[0]==0x01 && eapol_key_recvd->key_info[1]==0x0a) ||
		    ( eapol_key_recvd->key_info[0]==0x03 && eapol_key_recvd->key_info[1]==0x0a) ||
		    ( eapol_key_recvd->key_info[0]==0x03 && eapol_key_recvd->key_info[1]==0x09) )
		{
			if( pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA)
			RSNIE = EAPOLkeyGetRSNIE( pGlInfo->EapolKeyMsgRecvd, EID_Vendor );
			else if( pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA2)
				RSNIE = EAPOLkeyGetRSNIE( pGlInfo->EapolKeyMsgRecvd, EID_WPA2 );
				
			if( RSNIE.Length != 0 )
				msg_type = type_4way2nd;		// with RSNIE: msg 2 (159 or 161)
			else
				msg_type = type_4way4th;		// msg 4 (135)
		}
		else
		{
			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("unknow pairwise EAPOL-key: info=0x%X-0x%X\n", eapol_key_recvd->key_info[0], eapol_key_recvd->key_info[1]) );
		}
	}
	else
	{
		// [AnnieNote] Windows zero-config may send 2-way message as 03-01.
		//
		//if( eapol_key_recvd->key_info[0]==0x03 && eapol_key_recvd->key_info[1]==0x11 )	// if group key index is fixed 1, key information is 03-11.
		//	msg_type = type_2way2nd;			// group key msg2 (155)
		//else
		//	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("unknow group EAPOL-key: info=0x%X-0x%X\n", eapol_key_recvd->key_info[0], eapol_key_recvd->key_info[1]) );
		
		msg_type = type_2way2nd;
	}

	// Check state.
	if( KeyMgntStateIsWaitingEAPOLKey(pKeyMgnt) )
	{

		if( 	(pKeyMgnt->PrState==ASMPS_PTKSTART && msg_type==type_4way2nd ) ||
			( pKeyMgnt->PrState==ASMPS_PTKINITNEGOTIATING && msg_type==type_4way2nd ))
		{
			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Recvd 4-way message 2\n"));
			pKeyMgnt->TimeoutCtr = 0;

			//  AnnieTODO: if (1)k is pairwise and (2)MICVerified , then enter ASMPS_PTKINITNEGOTIATING state
			//  TODO: MIC Verify
			SNonce = Message_KeyNonce( pGlInfo->EapolKeyMsgRecvd );
			CopyMem( pKeyMgnt->SNonce, SNonce.Octet, KEY_NONCE_LEN );

			
			CalcPTK( pAP_addr, pSTA_addr, pKeyMgnt->ANonce, pKeyMgnt->SNonce,
					 pGlInfo->PMK, PMK_LEN, pKeyMgnt->PTK_update, PTK_LEN );

			if(!CheckEapolMIC(Adapter , pGlInfo->EAPOLMsgRecvd , pKeyMgnt->PTK_update , KEY_MIC_LEN ))
			{
				SendDeauthentication( Adapter, pSTA_addr , mic_failure );
				PlatformStallExecution(100);
				RT_TRACE_F(COMP_AP, DBG_TRACE, ("AsocEntry_RemoveStation\n"));
				
				AsocEntry_RemoveStation( Adapter , pSTA_addr);
				RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("MIC erroe\n"));
				return;
			 }


			Authenticator_StatePTKINITNEGOTIATING(Adapter, pEntry);
		}
		else if( pKeyMgnt->PrState==ASMPS_PTKINITNEGOTIATING && msg_type==type_4way4th )
		{
			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Recvd 4-way message 4\n"));
			pKeyMgnt->TimeoutCtr = 0;

			// if (1)k is pairwise and (2)MICVerified , then enter ASMPS_PTKINITDONE state
			if(!CheckEapolMIC(Adapter , pGlInfo->EAPOLMsgRecvd , pKeyMgnt->PTK_update , KEY_MIC_LEN ))
			{
				SendDeauthentication( Adapter, pSTA_addr , mic_failure );
				PlatformStallExecution(100);
				RT_TRACE_F(COMP_AP, DBG_TRACE, ("AsocEntry_RemoveStation case 2\n"));
				
				AsocEntry_RemoveStation( Adapter , pSTA_addr);
				RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("MIC erroe\n"));
				return;
			 }

			PlatformMoveMemory(&pEntry->perSTAKeyInfo.RxIV, &((PEAPOL_KEY_STRUCT)eapol_key_recvd)->key_rsc[0], 6);
			pEntry->perSTAKeyInfo.RxIV &= UINT64_C(0x0000ffffffffffff);
			//RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("pEntry->perSTAKeyInfo.RxIV = 0x%16"i64fmt"x", pEntry->perSTAKeyInfo.RxIV));

			Authenticator_StatePTKINITDONE(Adapter, pEntry);		
		}
		else if(  pKeyMgnt->GrState == ASMGS_REKEYNEGOTIATING && msg_type==type_2way2nd )
		{
			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Recvd 2-way message 2\n"));
			pKeyMgnt->TimeoutCtr = 0;

			//  if (1)k is group and (2)MICVerified , then enter ASMGS_REKEYESTABLISHED state
			// 2012/01/17 CCW If 4-way check is ok, we need not to check 2-way again.
			/*
			if(!CheckEapolMIC(Adapter , pGlInfo->EAPOLMsgRecvd , pKeyMgnt->PTK_update , KEY_MIC_LEN ))
			{
				SendDeauthentication( Adapter, pSTA_addr , mic_failure );
				PlatformStallExecution(100);
				AsocEntry_RemoveStation( Adapter , pSTA_addr);
				RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("MIC erroe\n"));
				return;
			}
			*/
			Authenticator_StateREKEYESTABLISHED(Adapter, pEntry);
		}
		else
		{
			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Authenticator_OnEAPOLKeyRecvd(): Unexpected case: PrState=%d, GrState=%d, msg_type=%d\n",
											pKeyMgnt->PrState, pKeyMgnt->GrState, msg_type ) );
		}

	}
	else
	{
		RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD, ("Authenticator_OnEAPOLKeyRecvd(): Unexpected State!!\n"));
		RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD, ("--- TimeoutCounter:%d, PairwiseKeyState:%d, GroupKeyState:%d ---\n", pKeyMgnt->TimeoutCtr, pKeyMgnt->PrState, pKeyMgnt->GrState));
	}

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_OnEAPOLKeyRecvd()\n") );

}


// Description: Get RSNIE in parsing EAPOL-key packet.
// Output: OCTET_STRING: RSNIE string.
// Modify: Annie, 2005-07-11
//		Reference: PacketGetElement(), which is only used for management frame.
//		In fact, this two function should be merged and reused. <AnnieTodo>
//
OCTET_STRING 
EAPOLkeyGetRSNIE(
	IN	OCTET_STRING	eapolkeypkt,
	IN	ELEMENT_ID		ID
	)
{
	u2Byte			offset;
	//u2Byte		length = eapolkeypkt.Length;
	OCTET_STRING	ret={0,0};
	u1Byte			WPATag[] = {0x00, 0x50, 0xf2, 0x01};
	u1Byte			WPA2Tag[] = {0x00, 0x0f, 0xac};
	u1Byte			temp;

	offset = 95;
	temp = eapolkeypkt.Octet[offset];

	if( (temp==ID) && (!PlatformCompareMemory(eapolkeypkt.Octet+offset+2, WPATag, sizeof(WPATag) )) )
	{
		ret.Length = eapolkeypkt.Octet[offset+1];		// RSNIE header
		ret.Octet = eapolkeypkt.Octet + offset +2;		// RSNIE Length field
	}

	if( (temp==ID) && (!PlatformCompareMemory(eapolkeypkt.Octet+offset+4, WPA2Tag, sizeof(WPA2Tag) )) )
	{
		ret.Length = eapolkeypkt.Octet[offset+1];		// RSNIE header
		ret.Octet = eapolkeypkt.Octet + offset +2;		// RSNIE Length field
	}

	RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD, ("PacketGetRSNIE(): Length=%d\n", ret.Length ));

	return ret;
}



void 
KeyMgntTimeout(
	PRT_TIMER		pTimer
)
{
	PADAPTER	Adapter=(PADAPTER)pTimer->Adapter;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	PAUTH_PKEY_MGNT_TAG	pKeyMgnt;
	int	i;
	int AsocNum = 0;
	PRT_WLAN_STA	pEntry;

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning!!] KeyMgntTimeout(): STA mode\n"));
		return;
	}
	else
	{
		//RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("KeyMgntTimeout(): AP mode, CurrentTimeSlot=%d\n", pMgntInfo->globalKeyInfo.CurrentTimeSlot ));
	}

	if( pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPAPSK ||
	    pMgntInfo->SecurityInfo.AuthMode == RT_802_11AuthModeWPA2PSK ||
	    pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA2)
	{
		// Query all stations' state and their timeout count.
		for( i=0; i<ASSOCIATE_ENTRY_NUM; i++ )
		{
			if( !pMgntInfo->AsocEntry[i].bUsed )
				continue;

			pKeyMgnt = &pMgntInfo->AsocEntry[i].perSTAKeyInfo;

			// Print Dbg Message. 2005-07-25, Annie.------------
			AsocNum++;
			pEntry = pKeyMgnt->pWLanSTA;
			RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[STA:%d] %02X-%02X-%02X-%02X-%02X-%02X, MicErrorCnt=%d, WEPErrorCnt=%d\n",
						i, pEntry->MacAddr[0], pEntry->MacAddr[1], pEntry->MacAddr[2], pEntry->MacAddr[3], pEntry->MacAddr[4], pEntry->MacAddr[5],
						pKeyMgnt->MicErrorCnt, pKeyMgnt->WEPErrorCnt ) );
			//---------------------------------------------

			if( KeyMgntStateIsWaitingEAPOLKey(pKeyMgnt)  &&  pKeyMgnt->GrState != ASMGS_REKEYESTABLISHED )
			{
				if( pKeyMgnt->TimeoutCtr >= MAX_TIMEOUT_CNT )
				{
					pKeyMgnt->TimeoutCtr = 0;	
					Authenticator_OnTimeoutCountExceeded(Adapter, pKeyMgnt);
				}
				else
				{
					if( pMgntInfo->globalKeyInfo.CurrentTimeSlot != pKeyMgnt->TimeSlot_lastsend )	// consider 1st retry should wait for at least 1 second. Annie, 2005-07-12.
					{
						//RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD, ("KeyMgntTimeout(): Retry! [TimeSlot:%"i64fmt"d, TimeoutCtr:%d]\n",
							//		pMgntInfo->globalKeyInfo.CurrentTimeSlot, (int)pKeyMgnt->TimeoutCtr ));
						
						pKeyMgnt->TimeoutCtr ++;

						if( pKeyMgnt->PrState == ASMPS_PTKSTART  )		// waiting for 2nd msg in 4-way handshake
						{
							Authenticator_StatePTKSTART(Adapter, &pMgntInfo->AsocEntry[i]);
						}
						else if( pKeyMgnt->PrState == ASMPS_PTKINITNEGOTIATING )	// waiting for 4th msg in 4-way handshake
						{
							Authenticator_StatePTKINITNEGOTIATING(Adapter, &pMgntInfo->AsocEntry[i]);
						}
						else if( pKeyMgnt->GrState == ASMGS_REKEYNEGOTIATING )	// waiting for 2nd msg in 2-way handshake
						{
							Authenticator_StateREKEYNEGOTIATING(Adapter, &pMgntInfo->AsocEntry[i]);
						}	
					}
					else
					{
						RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("KeyMgntTimeout(): equal TimeSlot. Not to retry.\n") );
					}

				}

			}
			
		}

		//RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("KeyMgntTimeout: AsocNum=%d, CurrentTimeSlot=%"i64fmt"d\n", AsocNum, pMgntInfo->globalKeyInfo.CurrentTimeSlot ) );

		// Continue setting the timer only when RT_802_11AuthModeWPAPSK.
		pMgntInfo->globalKeyInfo.CurrentTimeSlot ++;
		PlatformSetTimer( Adapter, &pMgntInfo->globalKeyInfo.KeyMgntTimer, KEY_MGNT_INTERVAL );
		
	}
}




// Description:
// Output: void
// Modify: Annie, 2005-07-12
void 
Authenticator_OnTimeoutCountExceeded(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt
	)
{
	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Authenticator_OnTimeoutCountExceeded!\n") );
	pKeyMgnt->EvntID = ASMEID_TimeOutExceeded;
	
	//pKeyMgnt->TimeoutCtr = 0;	// it should be cleared in KeyMgntTimeout().

	if( pKeyMgnt->PrState == ASMPS_PTKSTART || pKeyMgnt->PrState == ASMPS_PTKINITNEGOTIATING )
	{
		Authenticator_StateDISCONNECT( Adapter, pKeyMgnt->pWLanSTA, four_way_tmout );

	}
	else if( pKeyMgnt->GrState == ASMGS_REKEYNEGOTIATING )
	{
		Authenticator_StateKEYERROR( Adapter, pKeyMgnt->pWLanSTA, two_way_tmout );
	}
	else
	{
		RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD, ("Authenticator_OnTimeoutCountExceeded(): Unexpected State!!\n"));
		RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD, ("--- PairwiseKeyState:%d, GroupKeyState:%d ---\n", pKeyMgnt->PrState, pKeyMgnt->GrState));
	}	
}


// Status handle


// Description: Disconnect the station
// Output: void
// Modify: Annie, 2005-07-12.
void 
Authenticator_StateDISCONNECT(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA,
	IN	u1Byte			ReasonCode
	)
{
	PAUTH_PKEY_MGNT_TAG	pKeyMgnt = &pSTA->perSTAKeyInfo;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Authenticator_StateDISCONNECT()\n") );

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}
	
	pSTA->perSTAKeyInfo.PrState = ASMPS_DISCONNECTE;

	if( AsocEntry_IsStationAssociated( &Adapter->MgntInfo, pSTA->MacAddr ) )
	{
		RT_TRACE(COMP_AP, DBG_LOUD, ("Authenticator_StateDISCONNECT()  \n"));
		AP_DisassociateStation(Adapter, pKeyMgnt->pWLanSTA, ReasonCode);
	}

	// UCT
	//Authenticator_StateDISCONNECTED(Adapter, pSTA);	// I'll call it in AP_DisassociateStation(). Marked by Annie, 2005-07-15.

}

// Description:
// Output:
// Modify: 
void 
Authenticator_StateDISCONNECTED(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Authenticator_StateDISCONNECTED()\n") );

	if(!ACTING_AS_AP(Adapter))
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}

	pSTA->perSTAKeyInfo.PrState = ASMPS_DISCONNECTED;

	// [AnnieTODO] GNoStations --


	// [Note] We don't have to clear PMK, ANonce and GTK here.

	// UCT
	Authenticator_StateINITIALIZE(Adapter, pSTA);
}

// Description: Initialize the pairwise key data on input STA.
// Output: void
// Modify: Annie, 2005-07-02
//		Discard using condition pKeyMgnt->bPTKInstalled.
//		Instead, I add a macro KeyMgntStateIsWaitingEAPOLKey to check the state.

void 
Authenticator_StateINITIALIZE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	PAUTH_PKEY_MGNT_TAG	pKeyMgnt = &pSTA->perSTAKeyInfo;
	u1Byte	RdmBuf[20], NonceBuf[KEY_NONCE_LEN];
	u1Byte	i = 0;

	pKeyMgnt->pWLanSTA = pSTA;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_StateINITIALIZE()\n") );

	if( !ACTING_AS_AP(Adapter)
		&& !GET_TDLS_ENABLED(&(Adapter->MgntInfo)) 
		)
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}

	PlatformZeroMemory( pKeyMgnt->SNonce, KEY_NONCE_LEN );

	pKeyMgnt->TimeoutCtr = 0;
	pKeyMgnt->TimeSlot_sendstart = 0;
	pKeyMgnt->TimeSlot_lastsend = 0;

	if( Adapter->MgntInfo.SecurityInfo.AuthMode == RT_802_11AuthModeWPAPSK )
	{
		// 3. 802.1x::PortMode = Disable;
		pKeyMgnt->portMode = pmt_Disable;	// [TODO] other auth mode
		
		// 4. 802.1x::PortSecure = 0;
		pKeyMgnt->portSecure= psec_Unauthorized;	// [TODO] other auth mode
	}

	
	// 1. MSK = 0 ...?
	
	// 2. GNoStations = 0
	//	...it's for group key update. I don't do it currently. Annie, 2005-07-01.

	// Rest ANonce
	GetRandomBuffer( RdmBuf );
	for( i=0; i<16; i++ )
	{
		NonceBuf[i] = RdmBuf[i];
		NonceBuf[16+i] = RdmBuf[19-i];
	}

	PlatformMoveMemory( pKeyMgnt->ANonce , NonceBuf , KEY_NONCE_LEN );

	
	// 5. RemovePTK
	PlatformZeroMemory( pKeyMgnt->PTK, PTK_LEN );

	pKeyMgnt->TempEncKey = NULL;
	pKeyMgnt->TxMICKey   = NULL;
	pKeyMgnt->RxMICKey   = NULL;

	//AP-WPA AES ,CCW
	PlatformZeroMemory( pKeyMgnt->AESKeyBuf , AESCCMP_BLK_SIZE_TOTAL );
	
	//pKeyMgnt->bPTKInstalled = FALSE;
	pKeyMgnt->PInitAKeys = FALSE;
	pKeyMgnt->GInitAKeys = FALSE;
	pKeyMgnt->Pair = TRUE;				// [AnnieNote] Why not FALSE?? 2005-07-18.

	// TODO: 6. Revome key from CAM
	// [AnnieNote]
	// (1) We can only clear the MAC address (instead of total 6 double-word) in per CAM entry.
	// (2) When called by Authenticator_GlobalReset(), it takes a lot of I/O, and is H/W depended.
	//       Should we do it here? Or use workitem... ?

	//Remove  key from SW/HW CAM table, Add by CCW
	AP_RemoveKey( Adapter , pSTA );
	
	// 7. Reset ReplayCounter
	pKeyMgnt->KeyReplayCounter = 0;
	
	// 8. Reset SNonce
	PlatformZeroMemory( pKeyMgnt->SNonce, KEY_NONCE_LEN );

	// 9. Initialize TimeSlot_lastIntegrityFailed.
	pKeyMgnt->TimeSlot_lastIntegrityFailed = 0;

	pKeyMgnt->RxIV   = DEFAULT_INIT_RX_IV;
	pKeyMgnt->TxIV   = DEFAULT_INIT_TX_IV;
	pKeyMgnt->KeyRSC = pKeyMgnt->TxIV; 

	// Added by Annie for debug, 2005-07-25.
	pKeyMgnt->MicErrorCnt = 0;
	pKeyMgnt->WEPErrorCnt = 0;

	pKeyMgnt->PrState = ASMPS_INITIALIZE;
	pKeyMgnt->GrState = ASMGS_INITIALIZE;


	pSTA->keyindex = 0;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_StateINITIALIZE()\n") );

}

// Description:
// Output:
// Modify: 
void 
Authenticator_StateAUTHENTICATION(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}

	pSTA->perSTAKeyInfo.PrState = ASMPS_AUTHENTICATION;

	// [AnnieTODO]
	// 1. GNoStations ++
	// 2. PTK = 0
	PlatformZeroMemory( pSTA->perSTAKeyInfo.PTK, PTK_LEN);

	PlatformZeroMemory(pSTA->perSTAKeyInfo.AESKeyBuf, AESCCMP_BLK_SIZE_TOTAL);
	//pSTA->perSTAKeyInfo.bPTKInstalled = FALSE;
	pSTA->perSTAKeyInfo.PInitAKeys = FALSE;

	// 3. 802.1x::PortControl = Auto
	pSTA->perSTAKeyInfo.portControl = pct_Auto;

	// 4. 802.1x::PortMode = Enabled
	pSTA->perSTAKeyInfo.portMode = pmt_Enable;
	
	// UCT
	Authenticator_StateAUTHENTICATION2( Adapter, pSTA );
}

// Description:
// Output:
// Modify: 
void 
Authenticator_StateAUTHENTICATION2(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_StateAUTHENTICATION2()\n") );

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}


	pSTA->perSTAKeyInfo.PrState = ASMPS_AUTHENTICATION2;

	// ANonce = Counter ++
	Adapter->MgntInfo.globalKeyInfo.ANonce[31] ++;	//... [AnnieTODO] It needs a real addition!

	// PSK
	Authenticator_StatePTKSTART(Adapter, pSTA);

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_StateAUTHENTICATION2()\n") );

}

// Description:
// Output:
// Modify: 
void 
Authenticator_StatePTKSTART(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	PAUTH_GLOBAL_KEY_TAG	pGlInfo = &pMgntInfo->globalKeyInfo;
	PAUTH_PKEY_MGNT_TAG	pKeyMgnt = &pSTA->perSTAKeyInfo;
	u8Byte					KeyReplayCounter = 0;
	u1Byte					temp[8] = {0};
	u1Byte					indexi = 0;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_StatePTKSTART()\n") );

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}

	pKeyMgnt->PrState = ASMPS_PTKSTART;
	pKeyMgnt->PInitAKeys = FALSE;			// To consider EAPOL-start case.

	// [AnnieTODO]
	// 1. Construct 1st message in 4-way handshake.
	// 	EAPOL(0, 0, 1, 0, 0, p, ANonce, 0, 0)
	// 2. Send 1st msg 

	pKeyMgnt->KeyReplayCounter ++;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Send 4-way message 1\n"));
	//RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD , (" KeyReplayCounter = %08"i64fmt"x \n",pKeyMgnt->KeyReplayCounter  ));

	
	for( indexi = 0 ; indexi < 8 ; indexi++)
		temp[indexi] =  (u1Byte)((pKeyMgnt->KeyReplayCounter >>( (7-indexi) *8)) &0xff );

	PlatformMoveMemory( &KeyReplayCounter , temp , 8 );
	
	SendEapolKeyPacket(
		Adapter, 
		pSTA->MacAddr,	//StaAddr, 
		NULL, // Pointer to KCK (EAPOL-Key Confirmation Key).
		NULL, // 
		type_Pairwise, // EAPOL-Key Information field: Key Type bit: type_Group or type_Pairwise.
		FALSE, // EAPOL-Key Information field: Install Flag.
		TRUE, // EAPOL-Key Information field: Key Ack bit.
		FALSE, // EAPOL-Key Information field: Key MIC bit. If true, we will calculate EAPOL MIC and fill it into Key MIC field. 
		FALSE, // EAPOL-Key Information field: Secure bit.
		FALSE, // EAPOL-Key Information field: Error bit. True for MIC failure report.
		FALSE, // EAPOL-Key Information field: Requst bit.
		KeyReplayCounter, // EAPOL-KEY Replay Counter field.  //pSTA->perSTAKeyInfo.KeyReplayCounter
		pKeyMgnt->ANonce, // EAPOL-Key Key Nonce field (32-byte).
		0, // EAPOL-Key Key RSC field (8-byte).
		NULL, // Key Data field: Pointer to RSN IE, NULL if 
		NULL // Key Data field: Pointer to GTK, NULL if Key Data Length = 0.
	);

	pKeyMgnt->TimeSlot_sendstart = pGlInfo->CurrentTimeSlot;	// added by Annie, 2005-07-12.
	pKeyMgnt->TimeSlot_lastsend = pGlInfo->CurrentTimeSlot;

	// [Note] Don't do pSTA->perSTAKeyInfo.TimeoutCtr++ here!
	// The counter is controlled in KeyMgntTimeout.

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_StatePTKSTART()\n") );
}

// Description:
// Output:
// Modify: 
void 
Authenticator_StatePTKINITNEGOTIATING(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	PAUTH_GLOBAL_KEY_TAG	pGlInfo = &pMgntInfo->globalKeyInfo;
	PAUTH_PKEY_MGNT_TAG	pKeyMgnt = &pSTA->perSTAKeyInfo;

	u8Byte					KeyReplayCounter = 0;
	u1Byte					temp[8] = {0};
	u1Byte					indexi = 0;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_StatePTKINITNEGOTIATING()\n") );

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}


	pSTA->perSTAKeyInfo.PrState = ASMPS_PTKINITNEGOTIATING;

	// [AnnieTODO]
	// 1. Construct 3rd message in 4-way handshake.
	// 	EAPOL(0, 1, 1, Pair,0, P, ANonce, MIC(PTK_update), 0)
	// 2. Send 3rd msg 

	pKeyMgnt->KeyReplayCounter ++;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Send 4-way message 3\n"));
	//RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD , (" KeyReplayCounter = %08"i64fmt"x \n",pKeyMgnt->KeyReplayCounter  ));

	for( indexi = 0 ; indexi < 8 ; indexi++)
		temp[indexi] =  (u1Byte)((pKeyMgnt->KeyReplayCounter >>( (7-indexi) *8)) &0xff );

	PlatformMoveMemory( &KeyReplayCounter , temp , 8 );
	
	if( pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA )
	{
	SendEapolKeyPacket(
		Adapter, 
		pSTA->MacAddr, 
		pSTA->perSTAKeyInfo.PTK_update, // Pointer to KCK (EAPOL-Key Confirmation Key).
		pSTA->perSTAKeyInfo.PTK_update + 16,
		type_Pairwise, // EAPOL-Key Information field: Key Type bit: type_Group or type_Pairwise.
		TRUE, // EAPOL-Key Information field: Install Flag.
		TRUE, // EAPOL-Key Information field: Key Ack bit.
		TRUE, // EAPOL-Key Information field: Key MIC bit. If true, we will calculate EAPOL MIC and fill it into Key MIC field. 
		FALSE, // EAPOL-Key Information field: Secure bit.
		FALSE, // EAPOL-Key Information field: Error bit. True for MIC failure report.
		FALSE, // EAPOL-Key Information field: Requst bit.
		KeyReplayCounter, //pSTA->perSTAKeyInfo.KeyReplayCounter, // EAPOL-KEY Replay Counter field.
				pKeyMgnt->ANonce, // EAPOL-Key Key Nonce field (32-byte).
		pSTA->perSTAKeyInfo.KeyRSC, // perSTA EAPOL-Key Key RSC field (8-byte).
		&(pMgntInfo->SecurityInfo.RSNIE), // Key Data field: Pointer to RSN IE, NULL if 
		NULL // Key Data field: Pointer to GTK, NULL if Key Data Length = 0.
	);
	}
	else if( pMgntInfo->SecurityInfo.SecLvl == RT_SEC_LVL_WPA2 )
	{
		SendEapolKeyPacket(
			Adapter, 
			pSTA->MacAddr, 
			pSTA->perSTAKeyInfo.PTK_update, // Pointer to KCK (EAPOL-Key Confirmation Key).
			pSTA->perSTAKeyInfo.PTK_update + 16,
			type_Pairwise, // EAPOL-Key Information field: Key Type bit: type_Group or type_Pairwise.
			TRUE, // EAPOL-Key Information field: Install Flag.
			TRUE, // EAPOL-Key Information field: Key Ack bit.
			TRUE, // EAPOL-Key Information field: Key MIC bit. If true, we will calculate EAPOL MIC and fill it into Key MIC field. 
			TRUE, // EAPOL-Key Information field: Secure bit.
			FALSE, // EAPOL-Key Information field: Error bit. True for MIC failure report.
			FALSE, // EAPOL-Key Information field: Requst bit.
			KeyReplayCounter,//pSTA->perSTAKeyInfo.KeyReplayCounter, // EAPOL-KEY Replay Counter field.
			pKeyMgnt->ANonce, // EAPOL-Key Key Nonce field (32-byte).
			pSTA->perSTAKeyInfo.KeyRSC, // perSTA EAPOL-Key Key RSC field (8-byte).
			&(pMgntInfo->SecurityInfo.RSNIE), // Key Data field: Pointer to RSN IE, NULL if 
			pGlInfo->GTK  // Key Data field: Pointer to GTK, NULL if Key Data Length = 0.
		);
	}

	pKeyMgnt->TimeSlot_lastsend = pGlInfo->CurrentTimeSlot;	// Added by Annie, 2005-07-12.

	// [Note] Don't do pSTA->perSTAKeyInfo.TimeoutCtr++ here!
	// The counter is controlled in KeyMgntTimeout.

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_StatePTKINITNEGOTIATING()\n") );

}

// Description:
// Output:
// Modify: 
void 
Authenticator_StatePTKINITDONE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	PAUTH_PKEY_MGNT_TAG	pKeyMgnt = &pSTA->perSTAKeyInfo;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_StatePTKINITDONE()\n") );


	// TODO: SetKey to CAM


	if( !ACTING_AS_AP(Adapter))
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}

	pSTA->perSTAKeyInfo.PrState = ASMPS_PTKINITDONE;
	
	// TODO: Check SetKey completed, these lines were moved from Authenticator_OnEAPOLKeyRecvd() by Jay 
	if (pSTA->perSTAKeyInfo.Pair)
	{
		u4Byte  ucIndex = 0;
		CopyMem(pSTA->perSTAKeyInfo.PTK, pSTA->perSTAKeyInfo.PTK_update, PTK_LEN);	// Added by Annie, 2005-07-12.
		if( Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm != RT_ENC_ALG_AESCCMP )
		{
			pSTA->perSTAKeyInfo.TempEncKey = pKeyMgnt->PTK+TKIP_ENC_KEY_POS;
			pSTA->perSTAKeyInfo.TxMICKey = pKeyMgnt->PTK+(TKIP_MIC_KEY_POS);	
			pSTA->perSTAKeyInfo.RxMICKey = pKeyMgnt->PTK+(TKIP_MIC_KEY_POS+TKIP_MIC_KEY_LEN);

			//Add for AP mode HW enc,by CCW		
			ucIndex = AP_FindFreeEntry(Adapter , pSTA->MacAddr );
			if(ucIndex == Adapter->TotalCamEntry)
			{
				RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] Authenticator_StatePTKINITDONE: Cam Entry is FULL!!!\n"));
				return;
			}
		
			//set key
			AP_Setkey(  Adapter , 
					      pSTA->perSTAKeyInfo.pWLanSTA->MacAddr,
					      ucIndex,  // Entey  index 
					      CAM_TKIP,
					      0,  // Parise key 
					      pSTA->perSTAKeyInfo.TempEncKey);	

			pSTA->keyindex  = ucIndex;
		}else{  // AES mode AP-WPA AES,CCW
		
			AESCCMP_BLOCK		blockKey;
			//RT_TRACE( COMP_WPAAES, DBG_LOUD, ("====> Set Station Key."));
			//Add for AP mode HW enc,by CCW		
			ucIndex = AP_FindFreeEntry(Adapter , pSTA->MacAddr);
			if(ucIndex == Adapter->TotalCamEntry)
			{
				RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] Authenticator_StatePTKINITDONE: Cam Entry is FULL!!!\n"));
				return;
			}
			
			//Set Key 
			PlatformMoveMemory( blockKey.x , pKeyMgnt->PTK+TKIP_ENC_KEY_POS , 16);
			AES_SetKey(blockKey.x, AESCCMP_BLK_SIZE*8, (pu4Byte)pSTA->perSTAKeyInfo.AESKeyBuf);
			//set hw key
			AP_Setkey(  Adapter , 
					      pSTA->perSTAKeyInfo.pWLanSTA->MacAddr,
					      ucIndex,  // Entey  index 
					      CAM_AES,
					      0,  // Parise key 
					     pSTA->perSTAKeyInfo.PTK+TKIP_ENC_KEY_POS);	
			pSTA->keyindex  = ucIndex;
		}
		
	}
	//pSTA->perSTAKeyInfo.bPTKInstalled = TRUE;
	pSTA->perSTAKeyInfo.GInitAKeys = TRUE;
	pSTA->perSTAKeyInfo.PInitAKeys = TRUE;

	// Begin 2-way handshake
	if( Adapter->MgntInfo.SecurityInfo.SecLvl  == RT_SEC_LVL_WPA )
		Authenticator_StateREKEYNEGOTIATING(Adapter, pSTA);  // To do 2-way
	if( Adapter->MgntInfo.SecurityInfo.SecLvl  == RT_SEC_LVL_WPA2 )
	{
		Authenticator_StateREKEYESTABLISHED(Adapter, pSTA);  // No to do 2-way
		pKeyMgnt->TimeoutCtr = 0;
	}
	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_StatePTKINITDONE()\n") );
}

// Description:
// Output:
// Modify: 
void 
Authenticator_StateUPDATEKEYS(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Authenticator_StateUPDATEKEYS()\n") );

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}


	pSTA->perSTAKeyInfo.PrState = ASMPS_UPDATEKEYS;

	Authenticator_StatePTKSTART(Adapter, pSTA);

}

// Description:
// Output:
// Modify: 
void 
Authenticator_StateINTEGRITYFAILURE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
#if 1 //Added by Jay 0713 for process integrity failure 
	u8Byte			DiffTimeSlot;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
#endif

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}


	//pSTA->perSTAKeyInfo.PrState = ASMPS_MICFAILURE;
	pSTA->perSTAKeyInfo.PrState = ASMPS_INTEGRITYFAILURE;		// Modified by Annie, 2005-07-18.

	pSTA->perSTAKeyInfo.MicErrorCnt ++;	// Added by Annie for debug, 2005-07-25.

#if 1 //Added by Jay 0713 for process integrity failure 
	//RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("IntegrityFail(): AP mode, TimeSlot_lastIntegrityFailed=%"i64fmt"d\n", pSTA->perSTAKeyInfo.TimeSlot_lastIntegrityFailed ));
	//RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("IntegrityFail(): AP mode, CurrentTimeSlot             =%"i64fmt"d\n", pMgntInfo->globalKeyInfo.CurrentTimeSlot ));

	DiffTimeSlot = pMgntInfo->globalKeyInfo.CurrentTimeSlot - 
					pSTA->perSTAKeyInfo.TimeSlot_lastIntegrityFailed;
		
	pSTA->perSTAKeyInfo.TimeSlot_lastIntegrityFailed = 
		pMgntInfo->globalKeyInfo.CurrentTimeSlot;

	if(DiffTimeSlot > 60)
	{
#if SUPPORT_WPA_VERSION_D3
		//update the PTK with this STA and GTK with all STAs	
		Authenticator_StateKEYUPDATE(Adapter, pSTA);
#endif
	}
	else
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("IntegrityFail(): AP mode, disconnects all associated STAs\n"));

		// Rewrited by Annie: use AP_DisassociateAllStation(). 2005-07-18.
		AP_DisassociateAllStation( Adapter, mic_failure );


		pMgntInfo->globalKeyInfo.TimeSlot_IntegrityFail2 = pMgntInfo->globalKeyInfo.CurrentTimeSlot;
	}
#else
	// UCT
	Authenticator_StateKEYUPDATE(Adapter, pSTA);
#endif
}

// Description:
// Output:
// Modify: 
void 
Authenticator_StateKEYUPDATE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Authenticator_StateKEYUPDATE()\n") );

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}


	pSTA->perSTAKeyInfo.PrState = ASMPS_KEYUPDATE;

	// [AnnieTODO]
	// ANonce = Counter ++;
	// GNonve = Counter ++;


	// UCT
	Authenticator_StatePTKSTART(Adapter, pSTA);
}

// Description:
// Output:
// Modify: 
void 
Authenticator_StateREKEYNEGOTIATING(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	PAUTH_GLOBAL_KEY_TAG	pGlInfo = &pMgntInfo->globalKeyInfo;
	PAUTH_PKEY_MGNT_TAG	pKeyMgnt = &pSTA->perSTAKeyInfo;

	u8Byte					KeyReplayCounter = 0;
	u1Byte					temp[8] = {0};
	u1Byte					indexi = 0;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_StateREKEYNEGOTIATING()\n") );

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}

	pKeyMgnt->GrState = ASMGS_REKEYNEGOTIATING;

	// [AnnieTODO]
	// 1. Construct 1st message in 2-way handshake.
	// 	EAPOL(1, 1, 1, !Pair, GN, G, GNonce, MIC(PTK), GTK[GN] )
	// 2. Send 1st msg in 2-way handshake.

	pKeyMgnt->KeyReplayCounter ++;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("Send 2-way message 1\n"));

	for( indexi = 0 ; indexi < 8 ; indexi++)
		temp[indexi] =  (u1Byte)((pKeyMgnt->KeyReplayCounter >>( (7-indexi) *8)) &0xff );

	PlatformMoveMemory( &KeyReplayCounter , temp , 8 );
	
	SendEapolKeyPacket(
		Adapter, 
		pSTA->MacAddr, 
		pSTA->perSTAKeyInfo.PTK, // Pointer to KCK (EAPOL-Key Confirmation Key).
		pSTA->perSTAKeyInfo.PTK + 16,	// [AnnieWorkaround]
		type_Group, // EAPOL-Key Information field: Key Type bit: type_Group or type_Pairwise.
		FALSE, // EAPOL-Key Information field: Install Flag.
		TRUE, // EAPOL-Key Information field: Key Ack bit.
		TRUE, // EAPOL-Key Information field: Key MIC bit. If true, we will calculate EAPOL MIC and fill it into Key MIC field. 
		TRUE, // EAPOL-Key Information field: Secure bit.
		FALSE, // EAPOL-Key Information field: Error bit. True for MIC failure report.
		FALSE, // EAPOL-Key Information field: Requst bit.
		KeyReplayCounter,//pSTA->perSTAKeyInfo.KeyReplayCounter, // EAPOL-KEY Replay Counter field.
		pGlInfo->GNonce, // EAPOL-Key Key Nonce field (32-byte).
		pGlInfo->KeyRSC, // EAPOL-Key Key RSC field (8-byte).
		NULL, // Key Data field: Pointer to RSN IE, NULL if 
		pGlInfo->GTK  // Key Data field: Pointer to GTK, NULL if Key Data Length = 0.
	);

	pKeyMgnt->TimeSlot_lastsend = pGlInfo->CurrentTimeSlot;	// added by Annie, 2005-07-12.

	// [Note] Don't do pKeyMgnt->TimeoutCtr++ here!
	// The counter is controlled in KeyMgntTimeout.

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_StateREKEYNEGOTIATING()\n") );

}

// Description:
// Output:
// Modify: 
void 
Authenticator_StateREKEYESTABLISHED(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_StateREKEYESTABLISHED()\n") );


	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}


	pSTA->perSTAKeyInfo.GrState = ASMGS_REKEYESTABLISHED;

	//  TODO: 1. Check MIC(PTK)

	//  TODO: 2. GKeyDoneStations --;

	//  3. Timeout counter is reset by Authenticator_OnEAPOLKeyRecvd().
	//pSTA->perSTAKeyInfo.TimeoutCtr  ... we don't need to write it here.

	// 4. 802.1x::PortMode = 1;
	pSTA->perSTAKeyInfo.portMode = pmt_Enable;

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_StateREKEYESTABLISHED()\n") );
}

// Description:
// Output:
// Modify: 
void 
Authenticator_StateKEYERROR(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA,
	IN	u1Byte			ReasonCode
	)
{
	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_StateKEYERROR()\n") );

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}


	pSTA->perSTAKeyInfo.GrState = ASMGS_KEYERROR;

	// [AnnieTODO]  GKeyDoneStations --


	// UCT
	Authenticator_StateDISCONNECT( Adapter, pSTA, ReasonCode );

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_StateKEYERROR()\n") );
}


// Description: Initialize the global key data in Authenticator.
// Output: void
// Modify: Annie, 2005-07-02
//		I check the data struct again, and discard using pMgntInfo->globalKeyInfo.groupKeyInfo.
//		Now Global/group key data (PMK, GTK, ANonce): all kept in pMgntInfo->globalKeyInfo.
//		global key state: recorded in pEntry->perSTAKeyInfo.GrState. (I think it should be kept in per station.)
//
void 
Authenticator_GlobalReset(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);
	PAUTH_GLOBAL_KEY_TAG	pGlInfo = &(pMgntInfo->globalKeyInfo);
	PRT_WLAN_STA	pEntry;
	int 		i;
	u1Byte	RdmBuf[20], NonceBuf[KEY_NONCE_LEN];
	static u1Byte	CAM_CONST_BROAD[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	AESCCMP_BLOCK		blockKey;	


	//--- [AnnieWorkaround] See 11i D3.0 page91, GTK should be generated by PRF-X.
	u1Byte	TmpGTK[] = "12345678123456781234567812345678";
	//---

	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("===> Authenticator_GlobalReset()\n") );

	if( !ACTING_AS_AP(Adapter) )
	{
		RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("[Warning] current: STA mode, return."));
		return;
	}

	pGlInfo->currentId = 0;

	if(pSecInfo->SecLvl == RT_SEC_LVL_WPA)
		pGlInfo->DescriptorType = desc_type_RSN;
	else
		pGlInfo->DescriptorType = desc_type_WPA2;

	GetRandomBuffer( RdmBuf );
	for( i=0; i<16; i++ )
	{
		NonceBuf[i] = RdmBuf[i];
		NonceBuf[16+i] = RdmBuf[19-i];
	}
	NonceBuf[KEY_NONCE_LEN-1] = 0;	//[AnnieWorkaround] Remove it if ANonce addition is ready. 2005-11-25.
	RT_PRINT_DATA( COMP_AUTHENTICATOR, DBG_LOUD, "Authenticator_GlobalReset(): NonceBuf", NonceBuf, KEY_NONCE_LEN );	

	// 1. Install PMK
	if( pGlInfo->PassphraseLen < 64 ){
	PasswordHash(pGlInfo->Passphrase, pGlInfo->PassphraseLen,
		pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, pGlInfo->PMK );
	}
	else
	{
		// Add for direct to set PMK 64-Hex mode...
		if( pGlInfo->PassphraseLen == 64 )
			PlatformMoveMemory(pGlInfo->PMK, pGlInfo->Passphrase , 32 );
	}
	// 2. Install GTK

        //
        // 2010/12/15 Neo Jou check in
        // When in Linux AP mode, hostapd will set down GTK before Authenticator_GlobalReset()
        // Thus for Linux AP mode case, we don't reset GTK here
        //
	PlatformZeroMemory( pGlInfo->GTK, GTK_LEN );
	PlatformMoveMemory( pGlInfo->GTK, TmpGTK, GTK_LEN );
	pGlInfo->TxMICKey = pGlInfo->GTK + GTK_MIC_TX_POS;
	pGlInfo->RxMICKey = pGlInfo->GTK + GTK_MIC_RX_POS;

	//AP WPA AES,CCW	
	PlatformMoveMemory( blockKey.x , pGlInfo->GTK , 16);
	AES_SetKey(blockKey.x, AESCCMP_BLK_SIZE*8, (pu4Byte)pGlInfo->AESGTK);
	//
	pSecInfo->GroupTransmitKeyIdx = 1;
	
			

	// 3. Install ANonce
//	CopyMem( pGlInfo->ANonce, NonceBuf, KEY_NONCE_LEN );
	PlatformMoveMemory(pGlInfo->ANonce, NonceBuf, KEY_NONCE_LEN );

	// 4. Install GNonce
//	CopyMem( pGlInfo->GNonce, NonceBuf, KEY_NONCE_LEN );
	PlatformMoveMemory(pGlInfo->GNonce, NonceBuf, KEY_NONCE_LEN );

	// 5. Reset KeyRSC
	pGlInfo->KeyRSC = 0;
	
	// 6. Reset time slot.
	pGlInfo->CurrentTimeSlot = 0;

#if 1 //Addedby Jay 0713
	pGlInfo->TimeSlot_IntegrityFail2 = 0;
#endif

	// 7. IV
#if 1 //Added by Jay 0712 for security IV
	pSecInfo->TxIV = DEFAULT_INIT_TX_IV;
#endif
	pMgntInfo->bAPGlobRest = TRUE;
	// Reset key information of each station.
	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		pEntry = &(pMgntInfo->AsocEntry[i]);
		Authenticator_StateINITIALIZE(Adapter, pEntry);
	}
	pMgntInfo->bAPGlobRest = FALSE;

	//reset SWCamTabe and HWCamtable ,add by CCW
	AP_ClearAllKey(Adapter);
	
	if( (MgntActQuery_ApType(Adapter) == RT_AP_TYPE_NORMAL ||
	MgntActQuery_ApType(Adapter) == RT_AP_TYPE_IBSS_EMULATED 
		 || MgntActQuery_ApType(Adapter) == RT_AP_TYPE_LINUX) && 
     	( pMgntInfo->NdisVersion  < RT_NDIS_VERSION_6_20 ))
	{
	switch( pSecInfo->PairwiseEncAlgorithm )
	{
	case RT_ENC_ALG_TKIP:
		AP_Setkey(  Adapter , 
			     CAM_CONST_BROAD,
			     1,  // Index entry
			     CAM_TKIP,
			     1,  // Set Group Key
			     pGlInfo->GTK);
		break;

	case RT_ENC_ALG_AESCCMP:
		AP_Setkey(  Adapter , 
			     	CAM_CONST_BROAD,
			     	1,  // Index entry
			     	CAM_AES,
			     	1,  // Set Group Key
			     	pGlInfo->GTK);
		break;

	case RT_ENC_ALG_WEP40: 
	case RT_ENC_ALG_WEP104:
		{
			static u1Byte	CAM_CONST_ADDR[4][6] = {
				{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
				{0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
				{0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
				{0x00, 0x00, 0x00, 0x00, 0x00, 0x03}};
				u1Byte EncAlgo = ((pSecInfo->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40) ? CAM_WEP40 : CAM_WEP104);
	
			for(i = 0; i < 4; i++)
			{
				if(pSecInfo->KeyLen[i] > 0)
				{
					AP_Setkey(
						Adapter , 
						CAM_CONST_ADDR[i],
						i,  // Index entry
						EncAlgo,
						1,
						pSecInfo->KeyBuf[i]);
				}
			}
		}
		break;

	default:
		break;
	}
	}
	
	RT_TRACE( COMP_AUTHENTICATOR, DBG_LOUD, ("<=== Authenticator_GlobalReset()\n") );
	
}




