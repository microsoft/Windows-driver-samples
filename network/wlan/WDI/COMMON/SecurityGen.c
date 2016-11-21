#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "SecurityGen.tmh"
#endif


//===========Define Local Function Protype===================

BOOLEAN
SecCheckMIC(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	OCTET_STRING	frame;
	u1Byte			DestAddr[6];
	RT_ENC_ALG		EncAlgorithm;
	PRT_SECURITY_T	pSec = &Adapter->MgntInfo.SecurityInfo;
	BOOLEAN			bResult = TRUE;

	
	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	
	//2004/07/12, kcwu, 
	//Check if the packet is encrypted
	if(!Frame_WEP(frame))
	{
		return bResult;
	}
	
	//2004/07/12, kcwu
	//Find the algorithm that should be used
	PlatformMoveMemory(DestAddr, Frame_Addr1(frame), ETHERNET_ADDRESS_LENGTH);
	EncAlgorithm = (MacAddr_isMulticast(DestAddr)?pSec->GroupEncAlgorithm:pSec->PairwiseEncAlgorithm);
	switch(EncAlgorithm){
		case RT_ENC_ALG_TKIP:
			bResult = SecCheckTKIPMIC(Adapter, pRfd);
			break;
		case RT_ENC_ALG_AESCCMP:
			//2004/07/12, kcwu
			//Check AES MIC
			break;
		default:
			bResult = TRUE;
	}
	return bResult;
			
}


//
//	Description: 
//		Figure out TKIP MIC from packet content and compare it to 
//		MIC of packet received.
//
//	2004.10.08, by rcnjko.
//
BOOLEAN
SecCheckTKIPMIC(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	u1Byte			DestAddr[6];
	u1Byte			SrcAddr[6];
	u1Byte			Keyidx = 0;
	u1Byte			Qos[4];
	MicData			micdata;
	u1Byte			mic_for_check[8], mic_from_packet[8];
	OCTET_STRING	frame = {0};
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PRT_SECURITY_T	pSec = &Adapter->MgntInfo.SecurityInfo;
	PRT_RFD			pPrevRfd = NULL;
	PRT_RFD		 	pCurrRfd = NULL;
	pu1Byte			pCurrBuffer = NULL;
	int				CurrBufferLen = 0;
	PRT_WLAN_STA	pEntry =NULL;
	u1Byte			IVOffset;			// Added by Annie, 2005-12-23.
	u1Byte			IVKeyIdOffset;

	// <RJ_TODO> This is just a workaround for HCT 12.1, 2005.07.11, by rcnjko.
	if(pMgntInfo->mIbss)
	{
		return TRUE;
	}

	// <RJ_TODO> This is just a workaround for WiFi 5.2.2.4 S2 might failed 
	// in unknown condition. For example, we think the packets received is TKIP MIC error 
	// during this test and finally trigger counter measure.  2005.09.06, by rcnjko. 
	// Prefast warning C28182: Dereferencing NULL pointer. 'pRfd' contains NULL.
	if(pRfd != NULL && pRfd->nTotalFrag > 1)
	{
		return TRUE;
	}	

	// Initialize first buffer of the packet to as header.
	// Prefast warning C6011: Dereferencing NULL pointer 'pRfd'.
	if (pRfd != NULL)
	{
		FillOctetString(frame, pRfd->Buffer.VirtualAddress, (u2Byte)pRfd->FragLength);
	}
	//PRINT_DATA("Recieved Packet==>", pRfd->Buffer.VirtualAddress, pRfd->FragLength);

	// Get offset. Added by Annie, 2005-12-22.
	if(pRfd != NULL && pRfd->Status.bIsQosData )
	{
		IVOffset = sMacHdrLng + sQoSCtlLng;
	}
	else
	{
		IVOffset = sMacHdrLng;
	}
	// Prefast warning C6011: Dereferencing NULL pointer 'pRfd'. 
	if(pRfd != NULL && pRfd->Status.bContainHTC)
		IVOffset += sHTCLng;

	IVKeyIdOffset = IVOffset + KEYID_POS_IN_IV;

	
	// Packet length check.
	if(pRfd != NULL && pRfd->PacketLength < IVOffset )
	{
		RT_TRACE(COMP_DBG, DBG_LOUD, ("SecCheckTKIPMIC(): pRfd->PacketLength < IVOffset\n"));
		return FALSE; // Drop the packet.
	}

	// Prefast warning C6011: Dereferencing NULL pointer 'frame.Octet'. 
	if (frame.Octet != NULL)
	{
		// Get DestAddr from header. //Modified by Jay 0713
		PlatformMoveMemory(DestAddr, Frame_pDaddr(frame), ETHERNET_ADDRESS_LENGTH);

		// Get SrcAddr from header.
		PlatformMoveMemory(SrcAddr, Frame_pSaddr(frame), ETHERNET_ADDRESS_LENGTH);
	}

	// Step 1. Set key.
	// Prefast warning C6011: Dereferencing NULL pointer 'frame.Octet'
	if(ACTING_AS_AP(Adapter) && frame.Octet != NULL)		//Added by Jay for SwAP 0708
	{ // AP Mode.
		pEntry = AsocEntry_GetEntry(pMgntInfo, Frame_pSaddr(frame));
		
		if(pEntry == NULL)			
			return FALSE;
		
		if(pEntry->perSTAKeyInfo.RxMICKey == NULL)			
			return FALSE;
		
		SecMICSetKey(&micdata, pEntry->perSTAKeyInfo.RxMICKey);
	}
	else if(pRfd != NULL && pRfd->bTDLPacket)
	{
		pEntry = AsocEntry_GetEntry(pMgntInfo, Frame_pSaddr(frame));
		
		if(pEntry == NULL)			
			return FALSE;
		
		if(pEntry->perSTAKeyInfo.TxMICKey == NULL)			
			return FALSE;
		
		SecMICSetKey(&micdata, pEntry->perSTAKeyInfo.TxMICKey);
	}
	else
	{ // STA Mode.
		// Get key index.
		// Prefast warning C6011: Dereferencing NULL pointer 'frame.Octet'
		if(frame.Octet != NULL)
			Keyidx = SecGetRxKeyIdx( Adapter, DestAddr, ((frame.Octet[IVKeyIdOffset]>>6)&0x03) );	// Changed by Annie, 2005-12-22.

		//RT_TRACE(COMP_DBG, DBG_LOUD, ("SecCheckTKIPMIC(): Keyidx: %d\n", Keyidx));
		if(Keyidx > 4)	
		{		
			RT_TRACE(COMP_SEC, DBG_LOUD, ("SecCheckTKIPMIC(): Keyidx: %d > 4\n", Keyidx));		
			return FALSE; // Drop the packet.	
		}
		
			SecMICSetKey(&micdata, pSec->KeyBuf[Keyidx]+TKIP_MICKEYTX_POS);

	}

	// Step 2. Append DestAddr and SrcAddr into micdata.
	SecMICAppend(&micdata, DestAddr, ETHERNET_ADDRESS_LENGTH);
	SecMICAppend(&micdata, SrcAddr, ETHERNET_ADDRESS_LENGTH);

	// Step 3. Append Qos into micdata.
	PlatformZeroMemory(Qos, 4);
	if(pRfd != NULL && pRfd->Status.bIsQosData )
	{
		// Parsing priority to calculate TKIP MIC. Annie, 2005-12-22.
		Qos[0] = GET_QOS_CTRL_WMM_UP(frame.Octet);
	}

	SecMICAppend(&micdata, Qos, 4);

	// Step 4. Append each buffers of the packet into micdata.
	pCurrRfd = pRfd;
	pPrevRfd = pRfd;	// 2010/04/30 MH Prevent BSOD when copy mem from NULL ptr at below case.
	//Exclude header of first buffer of the packet.
	// Prefast warning C28182: Dereferencing NULL pointer. 'pCurrRfd' contains NULL
	if (pCurrRfd != NULL)
	{
		pCurrBuffer = pCurrRfd->Buffer.VirtualAddress + (IVOffset + pSec->EncryptionHeadOverhead);
		CurrBufferLen = (int)(pCurrRfd->FragLength) - ((int)(IVOffset)+(int)(pSec->EncryptionHeadOverhead));
	}
	while(pCurrRfd != NULL)
	{	
		// Get MIC of the Last buffer and exclude MIC from it.
		if(pCurrRfd->NextRfd == NULL)
		{ 
			if(CurrBufferLen >= TKIP_MIC_LEN)
			{ // MIC is not fragmented.

				// Get MIC.
				PlatformMoveMemory(
					mic_from_packet, 
					pCurrBuffer + CurrBufferLen - TKIP_MIC_LEN,
					TKIP_MIC_LEN);

				// Adjust size to append into micdata.
				CurrBufferLen -= TKIP_MIC_LEN;
			}
			else
			{ // MIC is fragmented into two part.
				int cbMicInPrev = TKIP_MIC_LEN - CurrBufferLen;

				RT_ASSERT(pPrevRfd != NULL, ("SecCheckTKIPMIC(): pRevRfd should not be NULL in this case.\n"));

				// Get MIC.
				PlatformMoveMemory(
					mic_from_packet, 
					pPrevRfd->Buffer.VirtualAddress + pPrevRfd->FragLength - cbMicInPrev,
					cbMicInPrev);
				PlatformMoveMemory(
					mic_from_packet + cbMicInPrev, 
					pCurrBuffer,
					CurrBufferLen);

				// Adjust size to append into micdata.
				CurrBufferLen = 0;
			}
		}
		else if(pCurrRfd->NextRfd->NextRfd == NULL)
		{ // In case if MIC is fragmented, we must take care about the last two fragment.
				
			if(pCurrRfd->NextRfd->FragLength < TKIP_MIC_LEN)
			{ // MIC is fragmented.
				int cbMicInPrev = TKIP_MIC_LEN - pCurrRfd->NextRfd->FragLength;

				RT_ASSERT(CurrBufferLen >= TKIP_MIC_LEN, ("SecCheckTKIPMIC(): the length of last two buffer: %d\n", CurrBufferLen));
				// Adjust size to append into micdata.
				CurrBufferLen -= cbMicInPrev;
			}
		}

		// Append this buffer into micdata.
		if(CurrBufferLen > 0)
		{
			SecMICAppend(&micdata, pCurrBuffer, CurrBufferLen);
		}

		// Next one.
		pPrevRfd = pCurrRfd;
		pCurrRfd = pCurrRfd->NextRfd;	
		if(pCurrRfd != NULL)
		{
			pCurrBuffer = pCurrRfd->Buffer.VirtualAddress + pCurrRfd->FragOffset;
			CurrBufferLen = pCurrRfd->FragLength;
		}
		else
		{
			pCurrBuffer = NULL;
			CurrBufferLen = 0;
		}
	}
	
	// Figure out MIC from micdata.
	SecMICGetMIC(&micdata, mic_for_check);
//	PRINT_DATA((const void*)"mic_for_check===>", (const void*)mic_for_check, TKIP_MIC_LEN);
//	PRINT_DATA((const void*)"mic_from_packet===>", (const void*)mic_from_packet, TKIP_MIC_LEN);
	
	// Compare two MICs.
	if(PlatformCompareMemory(mic_for_check, mic_from_packet,TKIP_MIC_LEN))
	{ // Different!
		BOOLEAN			bMcstDest = MacAddr_isMulticast(DestAddr);

		// Prefast warning 6011: Dereferencing NULL pointer 'pRfd'.
		if (pRfd != NULL)
		{
			RT_TRACE(COMP_SEC, DBG_WARNING,
				("Rx:TKIP_MIC Error, Decrypted: %x, bMcstDest: %d\n",
					pRfd->Status.Decrypted, bMcstDest));
			RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "SecCheckTKIPMIC(): TKIP_MIC Error packet", pRfd->Buffer.VirtualAddress, pRfd->FragLength);
		}

		if (frame.Octet != NULL)
		{
			if (IsDataFrame(frame.Octet))
				CountRxTKIPLocalMICFailuresStatistics(Adapter, pRfd);
			else if (IsMgntFrame(frame.Octet))
				CountRxMgntTKIPLocalMICFailuresStatistics(Adapter, pRfd);
		}
		
		// Handle TKIP MIC error.
		if(ACTING_AS_AP(Adapter))		//Added by Jay 0708. 
		{ // AP Mode.
			//Enter integrity failure state...
			Authenticator_StateINTEGRITYFAILURE(Adapter, pEntry);
		}
		else if(pRfd != NULL && pRfd->bTDLPacket)
		{
			Authenticator_StateINTEGRITYFAILURE(Adapter, pEntry);
		}
		else
		{ // STA Mode.
			
			// We don't indication MIC ERROR in MFP Packet !!
			// Prefast warning C28182: Dereferencing NULL pointer. 'pRfd' contains the same NULL value as 'pCurrRfd' did.
			if(pRfd != NULL && !pRfd->Status.bRxMFPPacket )
				PlatformHandleTKIPMICError(Adapter, bMcstDest, Keyidx, SrcAddr);
		}

		return FALSE;
	}
	else
	{ // The same.

		RT_TRACE(COMP_DBG, DBG_LOUD, ("Rx:TKIP_MIC Ok\n"));	
		return TRUE;
	}
}



u1Byte
SecGetRxKeyIdx(
	PADAPTER	Adapter,
	pu1Byte		pDestAddr,
	u1Byte		IVKeyIdx
	)
{
	u1Byte			keyidx = PAIRWISE_KEYIDX;
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PRT_SECURITY_T	pSec = &Adapter->MgntInfo.SecurityInfo;
	//pairwise keyidx in our driver is 4


	if(MacAddr_isMulticast(pDestAddr))
	{
		//2004/07/07, kcwu, if it is a multicast packet
		//keyidx = ((pSec->KeyLen[pSec->GroupTransmitKeyIdx]==0)?5:pSec->GroupTransmitKeyIdx);

		// Use IV's Key ID for group key.
		// 2004.10.06, by rcnjko.
		if(pSec->KeyLen[IVKeyIdx] == 0)
		{
			keyidx = 5;
		}
		else
		{
			keyidx = IVKeyIdx;
		}
	}
	else
	{
		//2004/07/07, kcwu, if it is a unicast packet
		if(pSec->KeyLen[PAIRWISE_KEYIDX] == 0)
		{
			//2004/07/07, kcwu, if there is no pairwise key
			keyidx = ((pMgntInfo->mIbss&&pSec->KeyLen[pSec->GroupTransmitKeyIdx]!=0)?pSec->GroupTransmitKeyIdx:5);
		}
	}
	return keyidx;
}



/*2004/06/28, kcwu
We don't fill security header of all fragment here, the for statement should in the outside function,
not in this function. So just simply provide the start of packet.

*/
VOID
SecHeaderFillIV(
	PADAPTER	Adapter,
	pu1Byte		PacketStart
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSec = &Adapter->MgntInfo.SecurityInfo;
	OCTET_STRING	oFrame = {0,0};
	RT_ENC_ALG		SelectEncAlgorithm = pSec->PairwiseEncAlgorithm;
	BOOLEAN			show = FALSE;

	//Address 1 is always receiver's address
	pu1Byte			pRA = PacketStart+4;
	pu1Byte			pSecHeader = PacketStart + sMacHdrLng;

	if(pMgntInfo->SafeModeEnabled)
		return;

	if( IsQoSDataFrame(PacketStart) )
	{
		pSecHeader += sQoSCtlLng;
	}
	
	// Check address 4 valiaddr , add for BT
	// Note : AP WDS mode need to do ....
	oFrame.Octet = PacketStart;
	
	if( Frame_ValidAddr4( oFrame ) )
	{
		RT_TRACE( COMP_SEC , DBG_LOUD, ("===>SecHeaderFillIV() : 4 address data \n") );
		SelectEncAlgorithm = RT_ENC_ALG_AESCCMP;
		pSecHeader += 6;
		show = TRUE;
	}
	
	switch(SelectEncAlgorithm){
		case RT_ENC_ALG_WEP104:
		case RT_ENC_ALG_WEP40:
		{
			// CKIP should also go into this case.
			pSecHeader[0] = EF1Byte( (u1Byte) (((pSec->TxIV&0x00000000ffffffff) >>16) & 0xff) );
			pSecHeader[1] = EF1Byte( (u1Byte) (((pSec->TxIV&0x00000000ffffffff) >>8) & 0xff) );
			pSecHeader[2] = EF1Byte( (u1Byte) (((pSec->TxIV&0x00000000ffffffff) >>0) & 0xff) );


			pSec->TxIV = (pSec->TxIV == 0xffffff)?0:pSec->TxIV+1;
			//Default Key ID
			pSecHeader[3] = EF1Byte( (u1Byte)((pSec->DefaultTransmitKeyIdx<<6)&0xc0) );
		}
		break;
		
		case RT_ENC_ALG_TKIP:
		{
			if(!ACTING_AS_AP(Adapter))
			{ // STA.
				u2Byte u2IV16 = (u2Byte)(pSec->TxIV& UINT64_C(0x000000000000ffff) );
				u4Byte u4IV32 = (u4Byte)(((pSec->TxIV& UINT64_C(0x0000ffffffff0000) ) >> 16)& UINT64_C(0x00000000ffffffff) );
				pSec->TxIV++;
	
				//keyid is always 0 for STA
				TKIP_TSC_DECIMAL2ARRAY(u2IV16, u4IV32, 0, pSecHeader);
			}
			else
			{ // AP.
				if(!MacAddr_isMulticast(pRA))
				{ // Unicast.
					PMGNT_INFO	 pMgntInfo = &Adapter->MgntInfo;
					PRT_WLAN_STA pEntry;
					u2Byte       u2IV16;
					u4Byte       u4IV32;

					pEntry = AsocEntry_GetEntry(pMgntInfo, pRA);

					// Annie for debug, 2005-07-25.
					RT_ASSERT( pEntry!=NULL, ("SecHeaderFillIV(): pEntry is NULL!\n"));
					if( pEntry==NULL )
						return;
					u2IV16 = (u2Byte)(pEntry->perSTAKeyInfo.TxIV & UINT64_C(0x000000000000ffff) );
					u4IV32 = (u4Byte)(((pEntry->perSTAKeyInfo.TxIV& UINT64_C(0x0000ffffffff0000) ) >> 16)& UINT64_C(0x00000000ffffffff) );		
					pEntry->perSTAKeyInfo.TxIV++;
	
					//keyid is always 0 for STA
					TKIP_TSC_DECIMAL2ARRAY(u2IV16, u4IV32, 0, pSecHeader);
				}
				else
				{ // Mcst/Bcst
					u2Byte u2IV16 = (u2Byte)(pSec->TxIV& UINT64_C(0x000000000000ffff) );
					u4Byte u4IV32 = (u4Byte)(((pSec->TxIV& UINT64_C( 0x0000ffffffff0000) ) >> 16)& UINT64_C(0x00000000ffffffff));
					pSec->TxIV++;

					// <TODO> This is just a workaround, we assume group keyid=1.
					TKIP_TSC_DECIMAL2ARRAY(u2IV16, u4IV32, 1, pSecHeader);
				}
			}
		}
		break;

		case RT_ENC_ALG_AESCCMP:
			if(!pMgntInfo->bRSNAPSKMode && !ACTING_AS_AP(Adapter) )
			{
				if(show)
				{
					RT_TRACE( COMP_SEC , DBG_LOUD , ("===>ccw Ok \n") );
				}
				
				PN_DECIMAL2ARRAY((pSec->TxIV& UINT64_C(0xffffffff00000000) ), 
						(pSec->TxIV& UINT64_C(0x00000000ffffffff)), 
					PAIRWISE_KEYIDX,		//keyid = 0 for pairwise packet 
					pSecHeader);
				pSec->TxIV++;
			}
			else if( ACTING_AS_AP(Adapter) )
			{
				
				if(!MacAddr_isMulticast(pRA))
				{ // Pairwise traffic.

					PMGNT_INFO	 pMgntInfo = &Adapter->MgntInfo;
					PRT_WLAN_STA pEntry;

					pEntry = AsocEntry_GetEntry(pMgntInfo, pRA);
					
					if( pEntry==NULL )
						return;
					
					PN_DECIMAL2ARRAY((pEntry->perSTAKeyInfo.TxIV& UINT64_C(0xffffffff00000000)), 
							(pEntry->perSTAKeyInfo.TxIV& UINT64_C(0x00000000ffffffff)), 
							0,		//keyid = 0 for pairwise packet 
							pSecHeader);
					pEntry->perSTAKeyInfo.TxIV++;
				}
				else
				{ // Group traffic. 
					PN_DECIMAL2ARRAY((pSec->TxIV& UINT64_C(0xffffffff00000000)), 
							(pSec->TxIV& UINT64_C(0x00000000ffffffff)), 
							pSec->GroupTransmitKeyIdx,// 0,
							pSecHeader);
					pSec->TxIV++;
				}
					
			}
			else
			{
				if(!MacAddr_isMulticast(pRA))
				{ // Pairwise traffic.
					PN_DECIMAL2ARRAY((pSec->TxIV& UINT64_C(0xffffffff00000000) ), 
							(pSec->TxIV&0x00000000ffffffff), 
							PAIRWISE_KEYIDX,		//keyid = 0 for pairwise packet 
							pSecHeader);
				}
				else
				{ // Group traffic. 
					PN_DECIMAL2ARRAY((pSec->TxIV& UINT64_C(0xffffffff00000000) ), 
							(pSec->TxIV& UINT64_C(0x00000000ffffffff) ), 
							pSec->DefaultTransmitKeyIdx,	
							pSecHeader);
				}
				pSec->TxIV++;
			}
			
		break;

		case RT_ENC_ALG_SMS4:
			WAPI_SecFuncHandler(WAPI_SECHEADERFILLIV, Adapter, (PVOID)pRA,(PVOID)pSecHeader,WAPI_END);
			break;

		case RT_ENC_ALG_NO_CIPHER:
		default:
			return;
	}
}


//
//Note : SecInit() need to Used after SecClearAllKeys();
//          Because Clear CAM Key will check SecInfo Key Buffer Length 
//		If Length = 0, CAM entey will "NO" Clear ....

VOID
SecInit(
	PADAPTER	Adapter
	)
{
	PRT_SECURITY_T pSec = &Adapter->MgntInfo.SecurityInfo;	
	s1Byte	i = 0;

	pSec->PairwiseEncAlgorithm = pSec->GroupEncAlgorithm = RT_ENC_ALG_AESCCMP;

	pSec->DefaultTransmitKeyIdx = 0;
	pSec->GroupTransmitKeyIdx = 0;
	SecSetSwEncryptionDecryption(Adapter, FALSE, FALSE);
	pSec->SWRxAESMICFlag = FALSE;	
	pSec->bGroupKeyFixed = FALSE;

	pSec->AuthMode = RT_802_11AuthModeWPA2;
	pSec->EncryptionStatus = RT802_11EncryptionDisabled;
	pSec->TxIV = DEFAULT_INIT_TX_IV;

	// 2007.08.08 We must initialize this value, otherwise, software decryption
	// without initilization of AES mode cause Assertion in Vista. By Lanhsin
	pSec->AESCCMPMicLen = 8;
	
	PlatformZeroMemory(pSec->RSNIEBuf, MAXRSNIELEN);
	pSec->RSNIE.Length = 0;
	FillOctetString(pSec->RSNIE, pSec->RSNIEBuf, pSec->RSNIE.Length);

	pSec->SecLvl = RT_SEC_LVL_WPA2;

	//2004/09/07, kcwu, initialize key buffer
	for(i = 0;i<KEY_BUF_SIZE; i++){
		PlatformZeroMemory(pSec->AESKeyBuf[i], MAX_KEY_LEN);
		PlatformZeroMemory(pSec->KeyBuf[i], MAX_KEY_LEN);
		pSec->KeyLen[i] = 0;
	}

	//kcwu_todo: Clear key buffer

	//Point Pairwise key to appropriate index
	pSec->PairwiseKey = pSec->KeyBuf[PAIRWISE_KEYIDX];
	pSec->UseDefaultKey = FALSE;

	// WPA2 Pre-Authentication related. Added by Annie, 2006-05-07.
	pSec->EnablePreAuthentication = pSec->RegEnablePreAuth;		// Added by Annie, 2005-02-15.
	PlatformZeroMemory(pSec->PMKIDList, sizeof(RT_PMKID_LIST)*NUM_PMKID_CACHE);
	for( i=0; i<NUM_PMKID_CACHE; i++ )
	{
		FillOctetString( pSec->PMKIDList[i].Ssid, pSec->PMKIDList[i].SsidBuf, 0 );
	}

	PlatformZeroMemory(pSec->szCapability, 256);

	init_crc32();

	// Initialize TKIP MIC error related, 2004.10.06, by rcnjko.
	pSec->LastPairewiseTKIPMICErrorTime = 0;	
	pSec->bToDisassocAfterMICFailureReportSend = FALSE;
	for(i = 0; i < MAX_DENY_BSSID_LIST_CNT; i++)
	{
		pSec->DenyBssidList[i].bUsed = FALSE;
	}
	
}

/*

	IN	PADAPTER		Adapter,
	IN	u4Byte			KeyIndex,
	IN	pu1Byte			pMacAddr,
	IN	BOOLEAN			IsGroup,
	IN	u1Byte			EncAlgo,
	IN	BOOLEAN			IsWEPKey//if OID = OID_802_11_WEP
*/

//
//Note : SecClearAllKeys() need to Used before SecInit();
//          Because Clear CAM Key will check SecInfo Key Buffer Length 
//		If Length = 0, CAM entey will "NO" Clear ....

VOID
SecClearAllKeys(
	PADAPTER	Adapter
	)
{
	int 		i;
	PRT_SECURITY_T pSec = &Adapter->MgntInfo.SecurityInfo;

	PlatformZeroMemory(pSec->TxMICKey, TKIP_MIC_KEY_LEN);
	PlatformZeroMemory(pSec->RxMICKey, TKIP_MIC_KEY_LEN);
	pSec->DefaultTransmitKeyIdx = 0;
	Adapter->HalFunc.SetKeyHandler(Adapter, 0, 0, 0, 0, 0, TRUE);

	//
	// Clear keys used for RSNA IBSS.
	//
	for(i = 0; i < MAX_NUM_PER_STA_KEY; i++)
	{
		PlatformZeroMemory( &(pSec->PerStaDefKeyTable[i]), sizeof(PER_STA_DEFAULT_KEY_ENTRY) ); 
		PlatformZeroMemory( &(pSec->MAPKEYTable[i]), sizeof(PER_STA_MPAKEY_ENTRY) ); 
	}
}

VOID
SecClearGroupKeyByIdx(
	PADAPTER	Adapter,
	u1Byte		paraIndex
){
	PlatformZeroMemory(Adapter->MgntInfo.SecurityInfo.KeyBuf[paraIndex], MAX_KEY_LEN);
	Adapter->MgntInfo.SecurityInfo.KeyLen[paraIndex] = 0;
	Adapter->HalFunc.SetKeyHandler(Adapter, paraIndex, 0, TRUE, 
		Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm, FALSE, FALSE);
}

VOID
SecClearWEPKeyByIdx(
	PADAPTER	Adapter,
	u1Byte		paraIndex
	)
{
	PlatformZeroMemory(Adapter->MgntInfo.SecurityInfo.KeyBuf[paraIndex], MAX_KEY_LEN);
	Adapter->MgntInfo.SecurityInfo.KeyLen[paraIndex] = 0;
	Adapter->HalFunc.SetKeyHandler(Adapter, paraIndex, 0, TRUE, 
		Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm, FALSE, FALSE);
}
//for AP mode
VOID
SecClearPairwiseKeyByMacAddr(
	PADAPTER	Adapter,
	pu1Byte		paraMacAddr
	)
{
	//2004/09/15, kcwu
	PlatformZeroMemory(Adapter->MgntInfo.SecurityInfo.PairwiseKey, MAX_KEY_LEN);
	Adapter->MgntInfo.SecurityInfo.KeyLen[PAIRWISE_KEYIDX] = 0;
	//2004/09/07, kcwu, disable the key entry in cam
	Adapter->HalFunc.SetKeyHandler(Adapter, PAIRWISE_KEYIDX, paraMacAddr, FALSE, 
		Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm, FALSE, FALSE);
}

//
// Description:
//	Select one AKM suite according to the input self AKM suite and the target info.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] AuthMode -
//		The self AKM suite.
//	[in] contentType -
//		The type of pInfoBuf.
//	[in] pInfoBuf -
//		This field is determined by contentType and shall be casted to the corresponding type.
//		pInfoBuf is PRT_WLAN_BSS if contentType is CONTENT_PKT_TYPE_CLIENT.
//		This field may be NULL according to contentType.
//	[in] InfoBufLen -
//		Length in byte of pInfoBuf.
//	[OUT] pAKM -
//		Return the selected AKM.
// Return:
//	RT_STATUS_SUCCESS, if the content is appended withour error.
//	RT_STATUS_INVALID_DATA, if any of pInfoBuf or InfoBufLen is incorrect.
//	RT_STATUS_INVALID_STATE, if the security parameter is mismatched.
// Remark:
//	If targetAKMSuite is 0, this function ignores the target setting and only reference the self AuthMode to
//	determine the final AKM_SUITE_TYPE.
// By Bruce, 2015-12-16.
//
RT_STATUS
Sec_SelAkmFromAuthMode(
	IN	PADAPTER			pAdapter,
	IN	RT_AUTH_MODE		AuthMode,
	IN	CONTENT_PKT_TYPE	contentType,
	IN	PVOID				pInfoBuf,
	IN	u4Byte				InfoBufLen,
	OUT	PAKM_SUITE_TYPE		pAKM
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	BOOLEAN			bCCX8021xenable = FALSE;
	BOOLEAN			bAPSuportCCKM = FALSE;
	PRT_WLAN_BSS	pTargetBss = NULL;
	u4Byte			ftMDIeLen = 0;
	AKM_SUITE_TYPE	targetAKMSuites = AKM_SUITE_NONE;

	CHECK_NULL_RETURN_STATUS(pAKM);

	if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT))
	{
		PFT_INFO_ENTRY			pEntry = NULL;
		
		if(!pInfoBuf || InfoBufLen < sizeof(RT_WLAN_BSS))			
		{
			RT_TRACE_F(COMP_SEC, DBG_SERIOUS, ("Invalid input buffer or length, pInfoBuf = %p, InfoBufLen = %d\n", pInfoBuf, InfoBufLen));
			return RT_STATUS_INVALID_DATA;
		}
		pTargetBss = (PRT_WLAN_BSS)pInfoBuf;

		targetAKMSuites = pTargetBss->AKMsuit;

		CCX_QueryCCKMSupport(pAdapter, &bCCX8021xenable, &bAPSuportCCKM);

		// Check we shall choose Fast Transition as the AKM
		if(((TEST_FLAG(pTargetBss->AKMsuit, AKM_FT_1X) && TEST_FLAG(pAdapter->MgntInfo.RegCapAKMSuite, AKM_FT_1X)) ||
			(TEST_FLAG(pTargetBss->AKMsuit, AKM_FT_PSK) && TEST_FLAG(pAdapter->MgntInfo.RegCapAKMSuite, AKM_FT_PSK))) &&
			(pEntry = FtGetEntry(pAdapter, pTargetBss->bdBssIdBuf)))
		{
			ftMDIeLen = pEntry->MDELen;
		}
	}	

	RT_TRACE_F(COMP_SEC, DBG_LOUD, ("AuthMode = %d, RegCapAKMSuite = 0x%08X\n", AuthMode, pAdapter->MgntInfo.RegCapAKMSuite));

	switch (AuthMode)
	{
		default:
			rtStatus = RT_STATUS_NOT_RECOGNIZED;
			break;
			
		case RT_802_11AuthModeWPA:
			if(bCCX8021xenable && bAPSuportCCKM)
			{
				*pAKM = AKM_WPA_CCKM;
			}
			else
			{
				*pAKM = AKM_WPA_1X;
			}
			break;
			
		case RT_802_11AuthModeWPA2:
			if(bCCX8021xenable && bAPSuportCCKM)
			{
				*pAKM = AKM_WPA2_CCKM;
			}
			else if(ftMDIeLen > 0)
			{
				*pAKM = AKM_FT_1X;
			}
			else if(TEST_FLAG(targetAKMSuites, AKM_RSNA_1X_SHA256) && TEST_FLAG(pAdapter->MgntInfo.RegCapAKMSuite, AKM_RSNA_1X_SHA256))
			{
				*pAKM = AKM_RSNA_1X_SHA256;		
			}
			else
			{
				*pAKM = AKM_WPA2_1X;
			}
			break;

		case RT_802_11AuthModeWPAPSK:
			*pAKM = AKM_WPA_PSK;
			break;
			
		case RT_802_11AuthModeWPA2PSK:
			{
				if(ftMDIeLen > 0)
				{
					*pAKM = AKM_FT_PSK;
				}
				else if(TEST_FLAG(targetAKMSuites, AKM_RSNA_PSK_SHA256) && TEST_FLAG(pAdapter->MgntInfo.RegCapAKMSuite, AKM_RSNA_PSK_SHA256))
				{
					*pAKM = AKM_RSNA_PSK_SHA256;
				}
				else
				{
					*pAKM = AKM_WPA2_PSK;
				}
			}
			break;		
	}

	return rtStatus;
}

//
// Description:
//	Get the OUI and OUI type from the AKM suite.
// Arguments:
//	[in] AKMSuite -
//		The AKM suite.
//	[out] pOUI -
//		Return the OUI with 3 bytes if returned status is RT_STATUS_SUCCESS.
//		If AKMSuite is AKM_SUITE_NONE , return all 0 in pOUI.
//	[out] pOUIType -
//		Return the OUI type with 1 byte if returned status is RT_STATUS_SUCCESS.
//		If AKMSuite is AKM_SUITE_NONE , return 0 in pOUIType.
// Return:
//	RT_STATUS_SUCCESS, if the translation succeeds.
//	RT_STATUS_NOT_RECOGNIZED, if the AKMSuite is not recognized.
//
RT_STATUS
Sec_MapAKMSuiteToOUIType(
	IN	AKM_SUITE_TYPE	AKMSuite,
	OUT	pu1Byte			pOUI,
	OUT	pu1Byte			pOUIType
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	switch(AKMSuite)
	{
	default:
		rtStatus = RT_STATUS_NOT_RECOGNIZED;
		break;

	case AKM_SUITE_NONE:
		{
			PlatformZeroMemory(pOUI, SIZE_OUI);
			PlatformZeroMemory(pOUIType, SIZE_OUI_TYPE);
		}
		break;

	case AKM_WPA_1X:
		{
			PlatformMoveMemory(pOUI, WPA_OUI, SIZE_OUI);
			*pOUIType = AKM_WPA_1X_OUI_TYPE;
		}
		break;

	case AKM_WPA_PSK:
		{
			PlatformMoveMemory(pOUI, WPA_OUI, SIZE_OUI);
			*pOUIType = AKM_WPA_PSK_OUI_TYPE;
		}
		break;

	case AKM_WPA_CCKM:
		{
			PlatformMoveMemory(pOUI, CCKM_OUI, SIZE_OUI);
			*pOUIType = CCKM_OUI_TYPE;
		}
		break;

	case AKM_WPA2_1X:
		{
			PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
			*pOUIType = RSN_AKM_SUITE_OUI_TYPE_RSN_1X;
		}
		break;

	case AKM_WPA2_PSK:
		{
			PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
			*pOUIType = RSN_AKM_SUITE_OUI_TYPE_RSN_PSK;
		}
		break;

	case AKM_WPA2_CCKM:
		{
			PlatformMoveMemory(pOUI, CCKM_OUI, SIZE_OUI);
			*pOUIType = CCKM_OUI_TYPE;
		}
		break;

	case AKM_RSNA_1X_SHA256:
		{
			PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
			*pOUIType = RSN_AKM_SUITE_OUI_TYPE_1X_SHA256;
		}
		break;

	case AKM_RSNA_PSK_SHA256:
		{
			PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
			*pOUIType = RSN_AKM_SUITE_OUI_TYPE_PSK_SHA256;
		}
		break;

	case AKM_FT_1X:
		{
			PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
			*pOUIType = RSN_AKM_SUITE_OUI_TYPE_FT_1X;
		}
		break;

	case AKM_FT_PSK:
		{
			PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
			*pOUIType = RSN_AKM_SUITE_OUI_TYPE_FT_PSK;
		}
		break;	
	}

	return rtStatus;
}

//
// Description:
//	Get the AKM suite  in AKM_SUITE_TYPE from OUI and OUI type.
// Arguments:
//	[out] pOUI -
//		The OUI with 3 bytes.
//		If the input is all 0 in pOUI,  return AKM_SUITE_NONE.
//	[out] pOUIType -
//		The OUI type with 1 byte.
//		If the input is all 0 in pOUIType, return AKM_SUITE_NONE.
//	[in] secLevl -
//		Input the security level related to OUI and OUIType
//	[in] pAKMSuite -
//		Return The tcipher suite.
// Return:
//	RT_STATUS_SUCCESS, if the translation succeeds.
//	RT_STATUS_NOT_RECOGNIZED, if the pOUI/pOUIType is not recognized.
//	RT_STATUS_INVALID_DATA, if any of pOUI, pOUIType, or secLevl is invalid.
//
RT_STATUS
Sec_MapOUITypeToAKMSuite(
	IN	pu1Byte				pOUI,
	IN	pu1Byte				pOUIType,
	IN	RT_SECURITY_LEVEL	secLevl,
	OUT	PAKM_SUITE_TYPE		pAKMSuite
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	CHECK_NULL_RETURN_STATUS(pOUI);
	CHECK_NULL_RETURN_STATUS(pOUIType);
	CHECK_NULL_RETURN_STATUS(pAKMSuite);
	
	if(RT_SEC_LVL_NONE == secLevl)
	{
		*pAKMSuite = AKM_SUITE_NONE;
	}
	else if(RT_SEC_LVL_WPA == secLevl)
	{
		if(eqOUI(pOUI, WPA_OUI))
		{
			switch(*pOUIType)
			{
			default:
				rtStatus = RT_STATUS_NOT_RECOGNIZED;
				break;
				
			case AKM_WPA_1X_OUI_TYPE:
				*pAKMSuite = AKM_WPA_1X;
				break;

			case AKM_WPA_PSK_OUI_TYPE:
				*pAKMSuite = AKM_WPA_PSK;
				break;
			}			
		}
		else if(eqOUI(pOUI, CCKM_OUI) &&  CCKM_OUI_TYPE == *pOUIType)
		{
			*pAKMSuite = AKM_WPA_CCKM;
		}
		else
		{
			rtStatus = RT_STATUS_NOT_RECOGNIZED;
		}		
	}
	else if(RT_SEC_LVL_WPA2 == secLevl)
	{
		if(eqOUI(pOUI, RSN_OUI))
		{
			switch(*pOUIType)
			{
			default:
				rtStatus = RT_STATUS_NOT_RECOGNIZED;
				break;
				
			case RSN_AKM_SUITE_OUI_TYPE_RSN_1X:
				*pAKMSuite = AKM_WPA2_1X;
				break;

			case RSN_AKM_SUITE_OUI_TYPE_RSN_PSK:
				*pAKMSuite = AKM_WPA2_PSK;
				break;

			case RSN_AKM_SUITE_OUI_TYPE_FT_1X:
				*pAKMSuite = AKM_FT_1X;
				break;

			case RSN_AKM_SUITE_OUI_TYPE_FT_PSK:
				*pAKMSuite = AKM_FT_PSK;
				break;

			case RSN_AKM_SUITE_OUI_TYPE_1X_SHA256:
				*pAKMSuite = AKM_RSNA_1X_SHA256;
				break;

			case RSN_AKM_SUITE_OUI_TYPE_PSK_SHA256:
				*pAKMSuite = AKM_RSNA_PSK_SHA256;
				break;
			}			
		}
		else if(eqOUI(pOUI, CCKM_OUI) &&  CCKM_OUI_TYPE == *pOUIType)
		{
			*pAKMSuite = AKM_WPA2_CCKM;
		}
		else
		{
			rtStatus = RT_STATUS_NOT_RECOGNIZED;
		}
	}
	
	return RT_STATUS_NOT_SUPPORT;
}


//
// Description:
//	Get the OUI and OUI type from the cipher suite in RT_ENC_ALG.
// Arguments:
//	[in] cipherSuite -
//		The cipher suite.
//	[in] secLevl -
//		Security level such WPA or WPA2.
//	[out] pOUI -
//		Return the OUI with 3 bytes if returned status is RT_STATUS_SUCCESS.
//		If cipherSuite is RT_ENC_ALG_NO_CIPHER , return all 0 in pOUI.
//	[out] pOUIType -
//		Return the OUI type with 1 byte if returned status is RT_STATUS_SUCCESS.
//		If AKMSuite is RT_ENC_ALG_NO_CIPHER , return 0 in pOUIType.
// Return:
//	RT_STATUS_SUCCESS, if the translation succeeds.
//	RT_STATUS_NOT_RECOGNIZED, if the cipherSuite is not recognized.
//	RT_STATUS_INVALID_DATA, if the cipherSuite is invalid.
//
RT_STATUS
Sec_MapCipherSuiteToOUIType(
	IN	RT_ENC_ALG			cipherSuite,
	IN	RT_SECURITY_LEVEL	secLevl,
	OUT	pu1Byte				pOUI,
	OUT	pu1Byte				pOUIType
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;

	if(RT_SEC_LVL_NONE == secLevl)
	{
		PlatformZeroMemory(pOUI, SIZE_OUI);
		PlatformZeroMemory(pOUIType, SIZE_OUI_TYPE);
	}
	else if(RT_SEC_LVL_WPA == secLevl)
	{
		switch(cipherSuite)
		{
		default:
			rtStatus = RT_STATUS_NOT_RECOGNIZED;
			break;

		case RT_ENC_ALG_NO_CIPHER:
			{
				PlatformZeroMemory(pOUI, SIZE_OUI);
				PlatformZeroMemory(pOUIType, SIZE_OUI_TYPE);
			}
			break;

		case RT_ENC_ALG_TKIP:
			{
				PlatformMoveMemory(pOUI, WPA_OUI, SIZE_OUI);
				*pOUIType = WPA_CIPHER_SUITE_OUI_TYPE_TKIP;
			}
			break;

		case RT_ENC_ALG_AESCCMP:
			{
				PlatformMoveMemory(pOUI, WPA_OUI, SIZE_OUI);
				*pOUIType = WPA_CIPHER_SUITE_OUI_TYPE_CCMP;
			}
			break;

		case RT_ENC_ALG_WEP40:
			{
				PlatformMoveMemory(pOUI, WPA_OUI, SIZE_OUI);
				*pOUIType = WPA_CIPHER_SUITE_OUI_TYPE_WEP40;
			}
			break;

		case RT_ENC_ALG_WEP104:
			{
				PlatformMoveMemory(pOUI, WPA_OUI, SIZE_OUI);
				*pOUIType = WPA_CIPHER_SUITE_OUI_TYPE_WEP104;
			}
			break;

		case RT_ENC_ALG_WEP:
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Invalid Cipher suite RT_ENC_ALG_WEP to map OUI\n"));
				rtStatus = RT_STATUS_INVALID_DATA;
			}
			break;

		case RT_ENC_ALG_SMS4:
			{ // WAPI does NOT have the cipher suite OUI/Type
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Invalid Cipher suite RT_ENC_ALG_SMS4 to map OUI\n"));
				rtStatus = RT_STATUS_INVALID_DATA;
			}
			break;		
		}
	}
	else if(RT_SEC_LVL_WPA2 == secLevl)
	{
		switch(cipherSuite)
		{
		default:
			rtStatus = RT_STATUS_NOT_RECOGNIZED;
			break;

		case RT_ENC_ALG_NO_CIPHER:
			{
				PlatformZeroMemory(pOUI, SIZE_OUI);
				PlatformZeroMemory(pOUIType, SIZE_OUI_TYPE);
			}
			break;

		case RT_ENC_ALG_USE_GROUP:
			{
				PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
				*pOUIType = RSN_CIPHER_SUITE_OUI_TYPE_USE_GROUP_CIPHER;
			}
			break;

		case RT_ENC_ALG_TKIP:
			{
				PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
				*pOUIType = RSN_CIPHER_SUITE_OUI_TYPE_TKIP;
			}
			break;

		case RT_ENC_ALG_AESCCMP:
			{
				PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
				*pOUIType = RSN_CIPHER_SUITE_OUI_TYPE_CCMP;
			}
			break;

		case RT_ENC_ALG_WEP40:
			{
				PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
				*pOUIType = RSN_CIPHER_SUITE_OUI_TYPE_WEP40;
			}
			break;

		case RT_ENC_ALG_WEP104:
			{
				PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
				*pOUIType = RSN_CIPHER_SUITE_OUI_TYPE_WEP104;
			}
			break;

		case RT_ENC_ALG_WEP:
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Invalid Cipher suite RT_ENC_ALG_WEP to map OUI\n"));
				rtStatus = RT_STATUS_INVALID_DATA;
			}
			break;

		case RT_ENC_ALG_SMS4:
			{ // WAPI does NOT have the cipher suite OUI/Type
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Invalid Cipher suite RT_ENC_ALG_SMS4 to map OUI\n"));
				rtStatus = RT_STATUS_INVALID_DATA;
			}
			break;

		case RT_ENC_ALG_BIP:
			{
				PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
				*pOUIType = RSN_CIPHER_SUITE_OUI_TYPE_BIP;
			}
			break;

		case RT_ENC_ALG_GROUP_ADDR_TRAFFIC_DISALLOWED:
			{
				PlatformMoveMemory(pOUI, RSN_OUI, SIZE_OUI);
				*pOUIType = RSN_CIPHER_SUITE_OUI_TYPE_GRP_ADDR_DISALLOWED;
			}
			break;
		}
	}
	else
	{
		rtStatus = RT_STATUS_NOT_RECOGNIZED;
	}

	return rtStatus;
}

//
// Description:
//	Get the cipher suite  in RT_ENC_ALG from OUI and OUI type.
// Arguments:
//	[out] pOUI -
//		The OUI with 3 bytes.
//		If the input is all 0 in pOUI,  return RT_ENC_ALG_NO_CIPHER.
//	[out] pOUIType -
//		The OUI type with 1 byte.
//		If the input is all 0 in pOUIType, return RT_ENC_ALG_NO_CIPHER.
//	[in] secLevl -
//		Input the security level related to OUI and OUIType
//	[in] pCipherSuite -
//		Return The tcipher suite.
// Return:
//	RT_STATUS_SUCCESS, if the translation succeeds.
//	RT_STATUS_NOT_RECOGNIZED, if the pOUI/pOUIType is not recognized.
//	RT_STATUS_INVALID_DATA, if any of pOUI, pOUIType, or secLevl is invalid.
//
RT_STATUS
Sec_MapOUITypeToCipherSuite(
	IN	pu1Byte				pOUI,
	IN	pu1Byte				pOUIType,
	IN	RT_SECURITY_LEVEL	secLevl,
	OUT	PRT_ENC_ALG			pCipherSuite
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	CHECK_NULL_RETURN_STATUS(pOUI);
	CHECK_NULL_RETURN_STATUS(pOUIType);
	CHECK_NULL_RETURN_STATUS(pCipherSuite);
	
	if(RT_SEC_LVL_NONE == secLevl)
	{
		*pCipherSuite = RT_ENC_ALG_NO_CIPHER;
	}
	else if(RT_SEC_LVL_WPA == secLevl)
	{
		if(eqOUI(pOUI, WPA_OUI))
		{
			switch(*pOUIType)
			{
			default:
				rtStatus = RT_STATUS_NOT_RECOGNIZED;
				break;
				
			case WPA_CIPHER_SUITE_OUI_TYPE_NONE:
				*pCipherSuite = RT_ENC_ALG_NO_CIPHER;;
				break;

			case WPA_CIPHER_SUITE_OUI_TYPE_WEP40:
				*pCipherSuite = RT_ENC_ALG_WEP40;;
				break;

			case WPA_CIPHER_SUITE_OUI_TYPE_TKIP:
				*pCipherSuite = RT_ENC_ALG_TKIP;;
				break;

			case WPA_CIPHER_SUITE_OUI_TYPE_CCMP:
				*pCipherSuite = RT_ENC_ALG_AESCCMP;;
				break;

			case WPA_CIPHER_SUITE_OUI_TYPE_WEP104:
				*pCipherSuite = RT_ENC_ALG_WEP104;;
				break;				
			}			
		}
		else
		{
			rtStatus = RT_STATUS_NOT_RECOGNIZED;
		}		
	}
	else if(RT_SEC_LVL_WPA2 == secLevl)
	{
		if(eqOUI(pOUI, RSN_OUI))
		{
			switch(*pOUIType)
			{
			default:
				rtStatus = RT_STATUS_NOT_RECOGNIZED;
				break;

			case RSN_CIPHER_SUITE_OUI_TYPE_USE_GROUP_CIPHER:
				*pCipherSuite = RT_ENC_ALG_USE_GROUP;;
				break;

			case RSN_CIPHER_SUITE_OUI_TYPE_WEP40:
				*pCipherSuite = RT_ENC_ALG_WEP40;;
				break;

			case RSN_CIPHER_SUITE_OUI_TYPE_TKIP:
				*pCipherSuite = RT_ENC_ALG_TKIP;;
				break;

			case RSN_CIPHER_SUITE_OUI_TYPE_CCMP:
				*pCipherSuite = RT_ENC_ALG_AESCCMP;;
				break;

			case RSN_CIPHER_SUITE_OUI_TYPE_WEP104:
				*pCipherSuite = RT_ENC_ALG_WEP104;;
				break;

			case RSN_CIPHER_SUITE_OUI_TYPE_BIP:
				*pCipherSuite = RT_ENC_ALG_BIP;;
				break;

			case RSN_CIPHER_SUITE_OUI_TYPE_GRP_ADDR_DISALLOWED:
				*pCipherSuite = RT_ENC_ALG_GROUP_ADDR_TRAFFIC_DISALLOWED;;
				break;
			}
		}
		else
		{
			rtStatus = RT_STATUS_NOT_RECOGNIZED;
		}
	}
	
	return RT_STATUS_NOT_SUPPORT;
}

//
// Description:
//	Get the old (deprecated) cipher suite from current RT_ENC_ALGs.
//	This function is used to translate the old definition such as FW.
// Arguments:
//	[IN] newCipherSuite -
//		The new cipher suite defined in RT_ENC_ALG.
// Return:
//	The cipher suite defined in RT_ENC_ALG_DEP.
//
RT_ENC_ALG_DEP
Sec_MapNewCipherToDepCipherAlg(
	IN	RT_ENC_ALG			newCipherSuite
	)
{
	RT_ENC_ALG_DEP	depCipher = NO_Encryption;
	switch(newCipherSuite)
	{
	default:
		{			
			RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Unknown newCipherSuite(0x%08X) to map\n", newCipherSuite));
		}
		break;

	case RT_ENC_ALG_NO_CIPHER:
		depCipher = NO_Encryption;
		break;

	case RT_ENC_ALG_TKIP:
		depCipher = TKIP_Encryption;
		break;

	case RT_ENC_ALG_AESCCMP:
		depCipher = AESCCMP_Encryption;
		break;

	case RT_ENC_ALG_WEP40:
		depCipher = WEP40_Encryption;
		break;

	case RT_ENC_ALG_WEP104:
		depCipher = WEP104_Encryption;
		break;

	case RT_ENC_ALG_WEP:
		depCipher = WEP_Encryption;
		break;

	case RT_ENC_ALG_SMS4:
		depCipher = SMS4_Encryption;
		break;
	}

	return depCipher;
}


RT_ENC_ALG
SecGetEncryptionOverhead(
	PADAPTER	Adapter,
	UNALIGNED pu2Byte		pMPDUHead,
	UNALIGNED pu2Byte		pMPDUTail,
	UNALIGNED pu2Byte		pMSDUHead,
	UNALIGNED pu2Byte		pMSDUTail,
	BOOLEAN		bByPacket,
	BOOLEAN		bIsBroadcastPkt
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	RT_ENC_ALG		ulSwitchCondition = (bByPacket&&bIsBroadcastPkt)? \
						pMgntInfo->SecurityInfo.GroupEncAlgorithm:pMgntInfo->SecurityInfo.PairwiseEncAlgorithm;
	
	// Decide EncryptionOverhead according to Encryption algorithm
	switch( ulSwitchCondition )
	{
	case RT_ENC_ALG_NO_CIPHER:
		if(pMPDUHead){ *pMPDUHead=0; }
		if(pMPDUTail){ *pMPDUTail=0; }
		if(pMSDUHead){ *pMSDUHead=0; }
		if(pMSDUTail){ *pMSDUTail=0; }
		break;

	case RT_ENC_ALG_WEP40:
	case RT_ENC_ALG_WEP104:
		// CKIP should also go into this case.
		if(pMPDUHead){ *pMPDUHead=4; }
		if(pMPDUTail){ *pMPDUTail=4; }
		if(pMSDUHead){ *pMSDUHead=0; }
		if(pMSDUTail){ *pMSDUTail=0; }
		break;

	case RT_ENC_ALG_TKIP:
		if(pMPDUHead){ *pMPDUHead=8; }
		if(pMPDUTail){ *pMPDUTail=4; }
		if(pMSDUHead){ *pMSDUHead=0; }
		if(pMSDUTail){ *pMSDUTail=8; }
		break;

	case RT_ENC_ALG_AESCCMP:
		if(pMPDUHead){ *pMPDUHead=8; }
		if(pMPDUTail){ *pMPDUTail=8; }
		if(pMSDUHead){ *pMSDUHead=0; }
		if(pMSDUTail){ *pMSDUTail=0; }
		break;

	case RT_ENC_ALG_SMS4:	        
		if(pMPDUHead){ *pMPDUHead=18; }
		if(pMPDUTail){ *pMPDUTail=16; }
		if(pMSDUHead){ *pMSDUHead=0; }
		if(pMSDUTail){ *pMSDUTail=0; }
		break;		

	default:
		break;
	}

	return	ulSwitchCondition;

}

VOID
SecGetGroupCipherFromBeacon(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pbssDesc)
{
	PRT_SECURITY_T	pSecInfo = &(Adapter->MgntInfo.SecurityInfo);
	
	pSecInfo->GroupEncAlgorithm = pbssDesc->GroupCipherSuite;
	RT_TRACE( COMP_SEC, DBG_LOUD, ("SecGetGroupCipherFromBeacon(): GroupEncAlgorithm = %d\n", pSecInfo->GroupEncAlgorithm) );

	SecConstructRSNIE(Adapter);
}

VOID
SecConstructRSNIE(
	PADAPTER	Adapter
	)
{
//kcwu: NOTICE===>
//The frame format of WPA/WPA2 are different
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PRT_SECURITY_T	pSecInfo = &Adapter->MgntInfo.SecurityInfo;
	BOOLEAN			bCCX8021xenable = FALSE;
	BOOLEAN			bAPSuportCCKM = FALSE;
	pu1Byte			pIeHead = Adapter->MgntInfo.SecurityInfo.RSNIEBuf;
	u2Byte			IeLen = 0;
	u1Byte			tmpOUI[SIZE_OUI] = {0};
	u1Byte			tmpOUIType = 0;
	u2Byte			AddSize;
	AKM_SUITE_TYPE	selAKM = AKM_SUITE_NONE;

	Adapter->MgntInfo.SecurityInfo.RSNIE.Length = 0;

	// Select the AKM
	Sec_SelAkmFromAuthMode(
					Adapter,
					pSecInfo->AuthMode,
					CONTENT_PKT_TYPE_AP,
					NULL,
					0,
					&selAKM);
		
	if(pSecInfo->SecLvl == RT_SEC_LVL_WPA)
	{
		// WPA OUI and Type
		SET_WPA_IE_OUI_AND_TYPE(pIeHead);
		IeLen += 4;

		// WPA Version
		SET_WPA_IE_VERSION(pIeHead, WPA_VER1);
		IeLen += 2;

		//	
		// 2. Group Cipher Suite.
		//
		Sec_MapCipherSuiteToOUIType(pSecInfo->GroupEncAlgorithm, RT_SEC_LVL_WPA, tmpOUI, &tmpOUIType);
		SET_WPA_IE_GROUP_CIPHER_SUITE_OUI_W_TYPE(pIeHead, tmpOUI, tmpOUIType);
		IeLen += 4;

		// Pairwise cipher suite OUI/Type
		Sec_MapCipherSuiteToOUIType(pSecInfo->PairwiseEncAlgorithm, RT_SEC_LVL_WPA, tmpOUI, &tmpOUIType);

		// Initialize Pairwise Suite count
		SET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(pIeHead, 0);
		IeLen += 2;

		// Pairwise cipher suite. We may add more pairwise cipher suites here for AP mode.
		ADD_WPA_IE_PAIRWISE_CIPHER_SUITE_OUI_W_TYPE(pIeHead, tmpOUI, tmpOUIType);
		IeLen += 4;

		// Intialize AKM suite count
		SET_WPA_IE_AKM_SUITE_CNT(pIeHead, 0);
		IeLen += 2;

		// AKM suite list
		Sec_MapAKMSuiteToOUIType(selAKM, tmpOUI, &tmpOUIType);
		ADD_WPA_IE_AKM_SUITE_OUI_W_TYPE(pIeHead, tmpOUI, tmpOUIType);
		IeLen += 4;		

		// Capabilities
		SET_WPA_IE_CAP_PTKSA_REPLAY_COUNTER(pIeHead, 0);
		SET_WPA_IE_CAP_GTKSA_REPLAY_COUNTER(pIeHead, 0);
		IeLen += 2;
	}
	else
	{ // Default: WPA2. Changed from WPA by Annie, 2006-04-26.
		SET_RSN_IE_VERSION(pIeHead, RSN_VER1);
		IeLen += 2;

		// Group cipher suite OUI/Type
		Sec_MapCipherSuiteToOUIType(pSecInfo->GroupEncAlgorithm, RT_SEC_LVL_WPA2, tmpOUI, &tmpOUIType);
		SET_RSN_IE_GROUP_CIPHER_SUITE_OUI_W_TYPE(pIeHead, tmpOUI, tmpOUIType);
		IeLen += 4;		
		
		// Pairwise cipher suite OUI/Type
		Sec_MapCipherSuiteToOUIType(pSecInfo->PairwiseEncAlgorithm, RT_SEC_LVL_WPA2, tmpOUI, &tmpOUIType);
				
		// RT_TRACE_F(COMP_SEC, DBG_LOUD, ("PairwiseEncAlgorithm = 0x%08X, OUI = %02X-%02X-%02X-%02X\n", pSecInfo->PairwiseEncAlgorithm, tmpOUI[0], tmpOUI[1], tmpOUI[2], tmpOUIType));
		
		// Intialize pairwise count
		SET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(pIeHead, 0);
		IeLen += 2;

		// We may add more pairwise cipher suites here for AP mode.
		ADD_RSN_IE_PAIRWISE_CIPHER_SUITE_OUI_W_TYPE(pIeHead, tmpOUI, tmpOUIType);
		IeLen += 4;

		// Intialize AKM suite count
		SET_RSN_IE_AKM_SUITE_CNT(pIeHead, 0);
		IeLen += 2;

		Sec_MapAKMSuiteToOUIType(selAKM, tmpOUI, &tmpOUIType);
		ADD_RSN_IE_AKM_SUITE_OUI_W_TYPE(pIeHead, tmpOUI, tmpOUIType);
		IeLen += 4;		


		//
		//3 // RSN Capabilities.
		//

		// (1) Pre-Authentication (bit0)
		// WPA: Reserver.
		// WPA2: "A non-AP STA sets the Pre-authentication subfield to zero", in 802.11iD10.0, 7.3.2.25.3 RSN Capabilities, page34. Added by Annie, 2006-05-08.
		SET_RSN_IE_CAP_PREAUTH(pIeHead, 0);

		// (2) PairwiseAsDefaultKey (bit1)
		//No Pairwise = false
		SET_RSN_IE_CAP_NO_PAIRWISE(pIeHead, 0);

		//Replay Counter
		//kcwu?: Should we implement multiple replay counters?
		// (3) NumOfPTKSAReplayCounter (bit2,3)
		SET_RSN_IE_CAP_PTKSA_REPLAY_COUNTER(pIeHead, 0);
		// (4) NumOfGTKSAReplayCounter (bit4,5)
		SET_RSN_IE_CAP_GTKSA_REPLAY_COUNTER(pIeHead, 0);

		IeLen += 2;
	}
	
	Adapter->MgntInfo.SecurityInfo.RSNIE.Length = IeLen;
	
	RT_PRINT_DATA( COMP_SEC, DBG_LOUD, ("SecConstructRSNIE(): RSNIE:\n"), pSecInfo->RSNIE.Octet, pSecInfo->RSNIE.Length);	
}

//
// Description:
//	Append security IEs to this authentication packet.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] contentType -
//		The type of content to append this WPA IE. This field also indicates which type of posOutContent.
//		CONTENT_PKT_TYPE_xxx -
//	[in] pInfoBuf -
//		This field is determined by contentType and shall be casted to the corresponding type.
//		pInfoBuf is PRT_WLAN_BSS if contentType is CONTENT_PKT_TYPE_CLIENT.
//	[in] InfoBufLen -
//		Length in byte of pInfoBuf.
//	[in] maxBufLen -
//		The max length of posOutContent buffer in byte.
//	[out] posOutContent -
//		The OCTECT_STRING structure for the content buffer and length.
//		posOutContent is a 802.11 packet if contentType is CONTENT_PKT_TYPE_802_11.
// Return:
//	RT_STATUS_SUCCESS, if the content is appended withour error.
//	RT_STATUS_BUFFER_TOO_SHORT, if the max length of buffer is not long enough.
//	RT_STATUS_INVALID_STATE, if the security parameter is mismatched.
// Remark:
//	To determine the RSN content to append to the output content, the pTargetBss, contentType, and posOutContent
//	shall be filled when calling this function. If RSN IE will be appended to an association request frame in posOutContent,
//	the 802.11 packet type and MAC header shall be filled in posOutContent before calling this function.
// By Bruce, 2015-09-22.
//
RT_STATUS
Sec_AppendWPAIE(
	IN	PADAPTER				pAdapter,
	IN	CONTENT_PKT_TYPE		contentType,
	IN	PVOID					pInfoBuf,
	IN	u4Byte					InfoBufLen,
	IN	u4Byte					maxBufLen,
	OUT POCTET_STRING			posOutContent
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	PRT_SECURITY_T	pSecInfo = &pAdapter->MgntInfo.SecurityInfo;
	u1Byte			rsnBuf[MAX_IE_LEN - SIZE_EID_AND_LEN] = {0};
	OCTET_STRING	osRsn;
	u1Byte			pktType = 0xFF;
	AKM_SUITE_TYPE	akmSuite = AKM_SUITE_NONE;
	u1Byte			tmpOUI[SIZE_OUI] = {0};
	u1Byte			tmpOUIType = 0;
	PRT_WLAN_BSS	pBssDesc = NULL;
	AKM_SUITE_TYPE	selAKM = AKM_SUITE_NONE;

	FillOctetString(osRsn, rsnBuf, 0);

	do
	{
		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT | CONTENT_PKT_TYPE_AP | CONTENT_PKT_TYPE_IBSS))
		{
			if(RT_SEC_LVL_WPA != pSecInfo->SecLvl)
			{
				RT_TRACE_F(COMP_SEC, DBG_TRACE, ("SecLvl (%d) != RT_SEC_LVL_WPA\n", pSecInfo->SecLvl));
				rtStatus = RT_STATUS_INVALID_STATE;
				break;
			}
		}		

		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_802_11))
		{
			pktType = PacketGetType(*posOutContent);
		}

		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT))
		{
			if(NULL == pInfoBuf || InfoBufLen < sizeof(RT_WLAN_BSS))
			{
				// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("pInfoBuf (%p) = NULL or InfoBufLen < required of sizeof(RT_WLAN_BSS) = %d\n", pInfoBuf, sizeof(RT_WLAN_BSS)));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
			pBssDesc = (PRT_WLAN_BSS)pInfoBuf;

			if(RT_STATUS_SUCCESS != (rtStatus = 
									Sec_SelAkmFromAuthMode(
									pAdapter,
									pSecInfo->AuthMode,
									CONTENT_PKT_TYPE_CLIENT,
									pBssDesc,
									sizeof(RT_WLAN_BSS),
									&selAKM)))
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Failed (0x%08X) from Sec_SelAkmFromAuthMode(), pSecInfo->AuthMode = %d, pBssDesc->AKMsuit = 0x%08X\n",
					rtStatus, pSecInfo->AuthMode, pBssDesc->AKMsuit));
				break;
				
			}
			
			RT_TRACE_F(COMP_SEC, DBG_LOUD, ("selAKM = 0x%08X\n", selAKM));

			// Do not append RSN IE in default.
			rtStatus = RT_STATUS_INVALID_DATA;
			switch(pktType)
			{
			default:
				rtStatus = RT_STATUS_INVALID_DATA;
				break;			

			case Type_Asoc_Req:
			case Type_Reasoc_Req:
				rtStatus = RT_STATUS_SUCCESS;
				break;
			}

			if(RT_STATUS_SUCCESS != rtStatus)
			{
				// No need to append RSN IE
				RT_TRACE_F(COMP_SEC, DBG_TRACE, ("pktType = 0x%02X, no need to append RSN IE\n", pktType));
				break;
			}
		}

		// OUI + type
		SET_WPA_IE_OUI_AND_TYPE(osRsn.Octet);
		osRsn.Length += 4;

		// WPA version
		SET_WPA_IE_VERSION(osRsn.Octet, WPA_VER1);
		osRsn.Length += 2;

		// Group cipher suite
		if(RT_STATUS_SUCCESS != (
			rtStatus = Sec_MapCipherSuiteToOUIType(pSecInfo->GroupEncAlgorithm, RT_SEC_LVL_WPA, tmpOUI, &tmpOUIType)))
		{
			RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Fail (0x%08X) to get OUI/Type from group EncAlg(0x%08X)\n", rtStatus, pSecInfo->GroupEncAlgorithm));
			break;
		}
		SET_WPA_IE_GROUP_CIPHER_SUITE_OUI_W_TYPE(osRsn.Octet, tmpOUI, tmpOUIType);
		osRsn.Length += 4;

		// Pairwise cipher suite OUI/Type
		if(RT_STATUS_SUCCESS != 
			(rtStatus = Sec_MapCipherSuiteToOUIType(pSecInfo->PairwiseEncAlgorithm, RT_SEC_LVL_WPA, tmpOUI, &tmpOUIType)))
		{
			RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Fail (0x%08X) to get OUI/Type from pairwise EncAlg(0x%08X)\n", rtStatus, pSecInfo->PairwiseEncAlgorithm));
			break;
			
		}

		// Initialize Pairwise Suite count
		SET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(osRsn.Octet, 0);
		osRsn.Length += 2;

		// Pairwise cipher suite. We may add more pairwise cipher suites here for AP mode.
		ADD_WPA_IE_PAIRWISE_CIPHER_SUITE_OUI_W_TYPE(osRsn.Octet, tmpOUI, tmpOUIType);
		osRsn.Length += 4;

		// Intialize AKM suite count
		SET_WPA_IE_AKM_SUITE_CNT(osRsn.Octet, 0);
		osRsn.Length += 2;

		// AKM suite list
		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT))
		{			
			if(RT_STATUS_SUCCESS != (rtStatus = Sec_MapAKMSuiteToOUIType(selAKM, tmpOUI, &tmpOUIType)))
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Failed (0x%08X) from Sec_MapAKMSuiteToOUIType() for selAKM = 0x%08X, AuthMode = %d, targetAKMSuite = 0x%08X\n",
					rtStatus, selAKM, pSecInfo->AuthMode, pMgntInfo->targetAKMSuite));
				break;
			}
			ADD_WPA_IE_AKM_SUITE_OUI_W_TYPE(osRsn.Octet, tmpOUI, tmpOUIType);
			osRsn.Length += 4;
		}
		else
		{
			// ToDo
			rtStatus = RT_STATUS_NOT_SUPPORT;
			break;			
		}

		// Capabilities
		SET_WPA_IE_CAP_PTKSA_REPLAY_COUNTER(osRsn.Octet, 0);
		SET_WPA_IE_CAP_GTKSA_REPLAY_COUNTER(osRsn.Octet, 0);
		osRsn.Length += 2;
	}while(FALSE);

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		if((maxBufLen - posOutContent->Length) < osRsn.Length)
		{
			RT_TRACE_F(COMP_SEC, DBG_WARNING, 
				("maxBufLen (%d) - posOutContent.Length (%d) < osRsn.Length(%d)\n", maxBufLen, posOutContent->Length, osRsn.Length));
			rtStatus = RT_STATUS_BUFFER_TOO_SHORT;
		}
		else
		{
			RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "Append IE data:\n", osRsn.Octet, osRsn.Length);
			PacketMakeElement(posOutContent, EID_Vendor, osRsn);
		}
	}

	return rtStatus;
}

//
// Description:
//	Append RSN IE to this packet or content.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] contentType -
//		The type of content to append this RSN IE. This field also indicates which type of posOutContent.
//		CONTENT_PKT_TYPE_xxx -
//	[in] pInfoBuf -
//		This field is determined by contentType and shall be casted to the corresponding type.
//		pInfoBuf is PRT_WLAN_BSS if contentType is CONTENT_PKT_TYPE_CLIENT.
//	[in] InfoBufLen -
//		Length in byte of pInfoBuf.
//	[in] maxBufLen -
//		The max length of posOutContent buffer in byte.
//	[out] posOutContent -
//		The OCTECT_STRING structure for the content buffer and length.
//		posOutContent is a 802.11 packet if contentType is CONTENT_PKT_TYPE_802_11.
// Return:
//	RT_STATUS_SUCCESS, if the content is appended withour error.
//	RT_STATUS_BUFFER_TOO_SHORT, if the max length of buffer is not long enough.
//	RT_STATUS_INVALID_STATE, if the security parameter is mismatched.
// Remark:
//	To determine the RSN content to append to the output content, the pTargetBss, contentType, and posOutContent
//	shall be filled when calling this function. If RSN IE will be appended to an association request frame in posOutContent,
//	the 802.11 packet type and MAC header shall be filled in posOutContent before calling this function.
// By Bruce, 2015-09-22.
//
RT_STATUS
Sec_AppendRSNIE(
	IN	PADAPTER				pAdapter,
	IN	CONTENT_PKT_TYPE		contentType,
	IN	PVOID					pInfoBuf,
	IN	u4Byte					InfoBufLen,
	IN	u4Byte					maxBufLen,
	OUT POCTET_STRING			posOutContent
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	PRT_SECURITY_T	pSecInfo = &pAdapter->MgntInfo.SecurityInfo;
	u1Byte			rsnBuf[MAX_IE_LEN - SIZE_EID_AND_LEN] = {0};
	OCTET_STRING	osRsn;
	u1Byte			pktType = 0xFF;
	AKM_SUITE_TYPE	akmSuite = AKM_SUITE_NONE;
	u1Byte			tmpOUI[SIZE_OUI] = {0};
	u1Byte			tmpOUIType = 0;
	PRT_WLAN_BSS	pBssDesc = NULL;
	AKM_SUITE_TYPE	selAKM = AKM_SUITE_NONE;
	PFT_INFO_ENTRY	pFtEntry = NULL;

	CHECK_NULL_RETURN_STATUS(posOutContent);
	
	FillOctetString(osRsn, rsnBuf, 0);	

	do
	{
		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT | CONTENT_PKT_TYPE_AP | CONTENT_PKT_TYPE_IBSS))
		{
			if(RT_SEC_LVL_WPA2 != pSecInfo->SecLvl)
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("SecLvl (%d) != RT_SEC_LVL_WPA2\n", pSecInfo->SecLvl));
				rtStatus = RT_STATUS_INVALID_STATE;
				break;
			}
		}

		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_802_11))
		{
			pktType = PacketGetType(*posOutContent);
		}

		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT))
		{
			BOOLEAN		bFtIeFromEntry = FALSE;
			if(NULL == pInfoBuf || InfoBufLen < sizeof(RT_WLAN_BSS))
			{
				// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("pInfoBuf (%p) = NULL or InfoBufLen < required of sizeof(RT_WLAN_BSS) = %d\n", pInfoBuf, sizeof(RT_WLAN_BSS)));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
			pBssDesc = (PRT_WLAN_BSS)pInfoBuf;

			if(RT_STATUS_SUCCESS != (rtStatus = 
									Sec_SelAkmFromAuthMode(
									pAdapter,
									pSecInfo->AuthMode,
									CONTENT_PKT_TYPE_CLIENT,
									pBssDesc,
									sizeof(RT_WLAN_BSS),
									&selAKM)))
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Failed (0x%08X) from Sec_SelAkmFromAuthMode(), pSecInfo->AuthMode = %d, pBssDesc->AKMsuit = 0x%08X\n",
					rtStatus, pSecInfo->AuthMode, pBssDesc->AKMsuit));
				break;
				
			}

			RT_TRACE_F(COMP_SEC, DBG_LOUD, ("selAKM = 0x%08X\n", selAKM));

			// Check if the entry has PMKID
			if(AKM_FT_1X == selAKM || AKM_FT_PSK == selAKM)
			{
				pFtEntry = FtGetEntry(pAdapter, pBssDesc->bdBssIdBuf);
			}

			// Do not append RSN IE in default.
			rtStatus = RT_STATUS_INVALID_DATA;
			switch(pktType)
			{
			default:
				rtStatus = RT_STATUS_INVALID_DATA;
				break;

			case Type_Auth:
				{
					if(FtIsFtAuthReady(pAdapter, pBssDesc->bdBssIdBuf))
					{
						rtStatus = RT_STATUS_SUCCESS;
					}
				}
				break;

			case Type_Asoc_Req:
			case Type_Reasoc_Req:
				{
					if(FtIsFtAssocReqReady(pAdapter, pBssDesc->bdBssIdBuf))
					{
						// Prefast warning C6011: Dereferencing NULL pointer 'pFtEntry'.
						if(pFtEntry != NULL && pFtEntry->RSNELen > SIZE_EID_AND_LEN)
						{
							FillOctetString(osRsn, pFtEntry->RSNE + SIZE_EID_AND_LEN, (pFtEntry->RSNELen - SIZE_EID_AND_LEN));
							bFtIeFromEntry = TRUE;
							RT_TRACE_F(COMP_SEC, DBG_LOUD, ("RSN IE content from FT entry\n"));
						}
						
					}
					rtStatus = RT_STATUS_SUCCESS;
				}
				break;
			}

			if(RT_STATUS_SUCCESS != rtStatus)
			{
				// No need to append RSN IE
				RT_TRACE_F(COMP_SEC, DBG_TRACE, ("pktType = 0x%02X, no need to append RSN IE\n", pktType));
				break;
			}

			// The content has been filled.
			if(bFtIeFromEntry)
				break;
		}
		
		SET_RSN_IE_VERSION(osRsn.Octet, RSN_VER1);
		osRsn.Length += 2;

		// Group cipher suite OUI/Type
		if(RT_STATUS_SUCCESS != 
			(rtStatus = Sec_MapCipherSuiteToOUIType(pSecInfo->GroupEncAlgorithm, RT_SEC_LVL_WPA2, tmpOUI, &tmpOUIType)))
		{
			RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Fail (0x%08X) to get OUI/Type from group EncAlg(0x%08X)\n", rtStatus, pSecInfo->GroupEncAlgorithm));
			break;
			
		}
		SET_RSN_IE_GROUP_CIPHER_SUITE_OUI_W_TYPE(osRsn.Octet, tmpOUI, tmpOUIType);
		osRsn.Length += 4;
		
		// Pairwise cipher suite OUI/Type
		if(RT_STATUS_SUCCESS != 
			(rtStatus = Sec_MapCipherSuiteToOUIType(pSecInfo->PairwiseEncAlgorithm, RT_SEC_LVL_WPA2, tmpOUI, &tmpOUIType)))
		{
			RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Fail (0x%08X) to get OUI/Type from pairwise EncAlg(0x%08X)\n", rtStatus, pSecInfo->PairwiseEncAlgorithm));
			break;
			
		}
		
		// RT_TRACE_F(COMP_SEC, DBG_LOUD, ("PairwiseEncAlgorithm = 0x%08X, OUI = %02X-%02X-%02X-%02X\n", pSecInfo->PairwiseEncAlgorithm, tmpOUI[0], tmpOUI[1], tmpOUI[2], tmpOUIType));
		
		// Intialize pairwise count
		SET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(osRsn.Octet, 0);
		osRsn.Length += 2;

		// We may add more pairwise cipher suites here for AP mode.
		ADD_RSN_IE_PAIRWISE_CIPHER_SUITE_OUI_W_TYPE(osRsn.Octet, tmpOUI, tmpOUIType);
		osRsn.Length += 4;

		// Intialize AKM suite count
		SET_RSN_IE_AKM_SUITE_CNT(osRsn.Octet, 0);
		osRsn.Length += 2;
		
		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT))
		{			
			if(RT_STATUS_SUCCESS != (rtStatus = Sec_MapAKMSuiteToOUIType(selAKM, tmpOUI, &tmpOUIType)))
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("Failed (0x%08X) from Sec_MapAKMSuiteToOUIType() for selAKM = 0x%08X, AuthMode = %d, targetAKMSuite = 0x%08X\n",
					rtStatus, selAKM, pSecInfo->AuthMode, pMgntInfo->targetAKMSuite));
				break;
			}
			ADD_RSN_IE_AKM_SUITE_OUI_W_TYPE(osRsn.Octet, tmpOUI, tmpOUIType);
			osRsn.Length += 4;
		}
		else
		{
			// ToDo
			rtStatus = RT_STATUS_NOT_SUPPORT;
			break;			
		}


		//
		//3 // RSN Capabilities.
		//

		// (1) Pre-Authentication (bit0)
		// WPA: Reserver.
		// WPA2: "A non-AP STA sets the Pre-authentication subfield to zero", in 802.11iD10.0, 7.3.2.25.3 RSN Capabilities, page34. Added by Annie, 2006-05-08.
		SET_RSN_IE_CAP_PREAUTH(osRsn.Octet, 0);

		// (2) PairwiseAsDefaultKey (bit1)
		//No Pairwise = false
		SET_RSN_IE_CAP_NO_PAIRWISE(osRsn.Octet, 0);

		//Replay Counter
		//kcwu?: Should we implement multiple replay counters?
		// (3) NumOfPTKSAReplayCounter (bit2,3)
		SET_RSN_IE_CAP_PTKSA_REPLAY_COUNTER(osRsn.Octet, 0);
		// (4) NumOfGTKSAReplayCounter (bit4,5)
		SET_RSN_IE_CAP_GTKSA_REPLAY_COUNTER(osRsn.Octet, 0);

		osRsn.Length += 2;
		
		if(TEST_FLAG(contentType, CONTENT_PKT_TYPE_CLIENT))
		{
			pu1Byte		pPmkIdBuf = NULL;
			int			pmkIdx = -1;
			BOOLEAN		bGroupMgntCipher = FALSE;
						
			// Append PMKID
			if(pFtEntry)
			{ // FT PMKID	
				if(pFtEntry->PMKR0NameLen > 0)
				{
					pPmkIdBuf = pFtEntry->PMKR0Name;
				}
			}
			else if((pmkIdx = SecIsInPMKIDList(pAdapter, pBssDesc->bdBssIdBuf)) >= 0)
			{
				pPmkIdBuf = pSecInfo->PMKIDList[pmkIdx].PMKID;
			}

			// Group Management Cipher Suite (Protected Management Frame Protocol)
			if(pBssDesc->bMFPC && pMgntInfo->bInBIPMFPMode)
			{
				SET_RSN_IE_CAP_MFP_CAPABLE(osRsn.Octet, 1);
				if(RT_STATUS_SUCCESS == Sec_MapCipherSuiteToOUIType(RT_ENC_ALG_BIP, RT_SEC_LVL_WPA2, tmpOUI, &tmpOUIType))
				{
					bGroupMgntCipher = TRUE;					
				}				
			}

			// Intialize PMKID count if there is any more content to avoid IOT issue
			if(pPmkIdBuf || bGroupMgntCipher)
			{				
				SET_RSN_IE_PMKID_CNT(osRsn.Octet, 0);
				osRsn.Length += 2;
			}

			if(pPmkIdBuf)
			{
				ADD_RSN_IE_PMKID(osRsn.Octet, pPmkIdBuf);
				osRsn.Length += PMKID_LEN;
			}

			if(bGroupMgntCipher)
			{
				SET_RSN_IE_GROUP_MGNT_CIPHER_SUITE_OUI_W_TYPE(osRsn.Octet, tmpOUI, tmpOUIType);
				osRsn.Length += 4;
			}
		}
	}while(FALSE);

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		if((maxBufLen - posOutContent->Length) < osRsn.Length)
		{
			RT_TRACE_F(COMP_SEC, DBG_WARNING, 
				("maxBufLen (%d) - posOutContent.Length (%d) < osRsn.Length(%d)\n", maxBufLen, posOutContent->Length, osRsn.Length));
			rtStatus = RT_STATUS_BUFFER_TOO_SHORT;
		}
		else
		{
			RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "Append IE data:\n", osRsn.Octet, osRsn.Length);
			PacketMakeElement(posOutContent, EID_WPA2, osRsn);
		}
	}

	return rtStatus;
}


u1Byte
SecGetTxKeyIdx(
	PADAPTER	Adapter,
	pu1Byte		pMacAddr	// Not used now...
	)
{
	//pairwise keyidx in our driver is 0 (in WPA-PSK or WPA2-PSK.)
	PRT_SECURITY_T	pSec = &Adapter->MgntInfo.SecurityInfo;
	u1Byte			keyidx = PAIRWISE_KEYIDX;

	//2004/06/29, kcwu,
	//check if pairwise key exists
	if(pSec->KeyLen[keyidx] == 0)
	{
		if(Adapter->MgntInfo.mIbss)
		{ // Ad Hoc Mode.
			return (pSec->KeyLen[pSec->GroupTransmitKeyIdx]==0)?5:pSec->GroupTransmitKeyIdx;
		}
		else
		{ // Infrastructure Mode.
			return 5;
		}
	}
	else
	{
		return keyidx;
	}

}


VOID
SecCalculateMIC(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSec = &(pMgntInfo->SecurityInfo);

	switch(pSec->PairwiseEncAlgorithm)
	{
		case RT_ENC_ALG_WEP40:
		case RT_ENC_ALG_WEP104:
			if( pSec->pCkipPara->bIsMIC )
			{ // CKIP MIC case.
				SecCalculateCKIPMIC(Adapter, pTcb);
			}
			break;
		
		case RT_ENC_ALG_TKIP:
			SecCalculateTKIPMIC(Adapter, pTcb);
			break;

		case RT_ENC_ALG_AESCCMP:
			break;

		default:
			break;
	}
}


//
//	Descriptin: 
//		Append TKIP MIC of given tx packet to send.
//
//	Note:
//		1. TKIP MIC input: DA(6) + SA(6) + Priority(1) + Zero(3) + Data(n)
//		where Data field shall conform 802.2 LLC and 802.1 bridging.
//		For example, Data is begined with LLC/SNAP.
//	
//	Assumption:
//		1. The tx packet repsented by RT_TCB here is an 802.11 MPDU 
//		translated from an MSDU before doing fragmentation.
//
// Revision History:
//		061028, rcnjko: 
//			Generalize the way to retrive MSDU payload.
//
VOID
SecCalculateTKIPMIC(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSec = &(pMgntInfo->SecurityInfo);
	u4Byte	idx = 0;
	u1Byte	keyidx = SecGetTxKeyIdx(Adapter, pTcb->DestinationAddress);
	MicData	micdata;
	u1Byte	Qos[4];
	PRT_WLAN_STA	pEntry = NULL;

	pu1Byte	pHeader = pTcb->BufferList[0].VirtualAddress;
	u4Byte FrameHdrLng = sMacHdrLng + pSec->EncryptionHeadOverhead; 
	u4Byte OffsetToByPass = 0;


	//
	// MIC Key.
	//
	if(ACTING_AS_AP(Adapter)) 
	{
		if(MacAddr_isMulticast(pTcb->DestinationAddress))
		{
			//Check if set GTK was completed	
			if(pMgntInfo->globalKeyInfo.GTK[0] == 0)									
				return;

			//Set Tx Group Key 	
			SecMICSetKey(&micdata, &pMgntInfo->globalKeyInfo.GTK[16]);
		}
		else
		{
			//Check if STA within associated lsit table		
			pEntry = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);		
			if(pEntry == NULL)			
				return;
				
			//Check if set PTK was completed		
			if(pEntry->perSTAKeyInfo.TxMICKey == NULL)									
				return;
				
			//Set Tx Pairwise Key by destination Address			
			SecMICSetKey(&micdata, pEntry->perSTAKeyInfo.TxMICKey);
		}
	} 
	else if(pTcb->bTDLPacket)
	{
			//Check if STA within associated lsit table		
			pEntry = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);		
			if(pEntry == NULL)			
				return;
				
			//Check if set PTK was completed		
			if(pEntry->perSTAKeyInfo.TxMICKey == NULL)									
				return;
				
			//Set Tx Pairwise Key by destination Address			
			SecMICSetKey(&micdata, pEntry->perSTAKeyInfo.TxMICKey);
		}
	else 
	{	
		if(pSec->PairwiseEncAlgorithm != RT_ENC_ALG_TKIP) {
			RT_TRACE(COMP_SEC, DBG_LOUD, ("PairwiseEncAlg=%d\n", pSec->PairwiseEncAlgorithm) );
			return;
		}

		if(pMgntInfo->mAssoc)
		{	
				SecMICSetKey(&micdata, pSec->KeyBuf[keyidx]+TKIP_MICKEYRX_POS);
		}
		else 
		{
			SecMICSetKey(&micdata, pSec->KeyBuf[keyidx]+TKIP_MICKEYTX_POS);
		}

	}

	//
	// DA, SA.
	//
	SecMICAppend(&micdata, pTcb->DestinationAddress, ETHERNET_ADDRESS_LENGTH);
	SecMICAppend(&micdata, pTcb->SourceAddress, ETHERNET_ADDRESS_LENGTH);

	//
	// Qos
	//
	PlatformZeroMemory(Qos, 4);
	
	if(IsQoSDataFrame(pHeader))
	Qos[0] = pTcb->priority;	// Added by Annie, 2005-12-21.


	SecMICAppend(&micdata, Qos, 4);

		if(IsQoSDataFrame(pHeader))
			FrameHdrLng += sQoSCtlLng;

	//
	// Data.
	//
	OffsetToByPass = FrameHdrLng;
	for(idx = 0;idx < pTcb->BufferCount; idx++)
	{
		if(pTcb->BufferList[idx].Length <= OffsetToByPass)
		{
			OffsetToByPass -= pTcb->BufferList[idx].Length;
			continue;
		}
		else
		{
			SecMICAppend(
				&micdata, 
				pTcb->BufferList[idx].VirtualAddress + OffsetToByPass,
				pTcb->BufferList[idx].Length - OffsetToByPass);
		}
	}

	//
	// Append MIC to tail.
	//
	SecMICGetMIC(&micdata, pTcb->Tailer.VirtualAddress);
	pTcb->Tailer.Length = TKIP_MIC_LEN;

	pTcb->BufferList[pTcb->BufferCount] = pTcb->Tailer;
	pTcb->BufferCount++;
	pTcb->PacketLength += TKIP_MIC_LEN;

	//PRINT_DATA("TX_MIC======>", pTcb->Tailer.VirtualAddress, 8);
}



//
// Calculate CKIP MIC.
// Ported from CKIP code, Annie, 2006-08-11.
//
VOID
SecCalculateCKIPMIC(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	PRT_SECURITY_T	pSec = &Adapter->MgntInfo.SecurityInfo;
	u1Byte			keyidx = pSec->DefaultTransmitKeyIdx;
	u4Byte			idx = 0;
	pu1Byte			pucMIC;
//	u1Byte			MICdata[2000] = {0};
	pu1Byte			MICdata;
	int				datalen = 0;
	PRT_GEN_TEMP_BUFFER pGenBufMICdata;

	RT_TRACE( COMP_CKIP, DBG_TRACE, ("==> SecCalculateCKIPMIC()\n") );
	
	if( !pSec->pCkipPara->bIsMIC) // if not support MIC retun , added by CCW
		return;

	pGenBufMICdata = GetGenTempBuffer (Adapter, 2000);
	MICdata = (pu1Byte)pGenBufMICdata->Buffer.Ptr;
	
	//ckip_miccalc(Adapter, key, pDA, pSA, dcr, dcrlen, calcmic, mickeyid);
	// key  = wep key , dcr = payload (  | LLC | SNAP |MIC |SEQ |eth-payload |)
	for(idx=1;idx<pTcb->BufferCount;idx++){	// Only check first nonzero buffer
		if(pTcb->BufferList[idx].Length==0)
			continue;
		
		PlatformMoveMemory(
				MICdata + datalen, 
				pTcb->BufferList[idx].VirtualAddress,
				pTcb->BufferList[idx].Length );
		
		datalen += pTcb->BufferList[idx].Length;
	}

	pucMIC = pTcb->BufferList[1].VirtualAddress + 8 ; // Set MIC file point
	
	
	ckip_miccalc( Adapter , pSec->pCkipPara->CKIPKeyBuf[keyidx] , pTcb->DestinationAddress ,pTcb->SourceAddress
				, MICdata ,datalen , pucMIC , keyidx );

	ReturnGenTempBuffer (Adapter, pGenBufMICdata);

	RT_TRACE( COMP_CKIP, DBG_TRACE, ("<== SecCalculateCKIPMIC()\n") );
}

VOID
SecSoftwareEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	RT_ENC_ALG		SelectEncAlgorithm;
	PMGNT_INFO      pMgntInfo = &Adapter->MgntInfo;
	
	if(pMgntInfo->SafeModeEnabled)
	{
		RT_TRACE_F(COMP_SEND, DBG_LOUD, ("Do not use software encrypt when safemode enable\n"));
		return;
	}

	if( pTcb->bBTTxPacket )
	{
		if(pTcb->EncInfo.IsEncByHW)
		{
			RT_TRACE( COMP_SEC , DBG_LOUD , (" ===> BT used HW encrypt !!No enter Sw encrypt !!\n") );
			return;
		}
		else
			SelectEncAlgorithm = RT_ENC_ALG_AESCCMP;
	}
	else
		SelectEncAlgorithm = Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm;
		
	switch(SelectEncAlgorithm)
	{
		case RT_ENC_ALG_WEP40:
		case RT_ENC_ALG_WEP104:
			if( !Adapter->MgntInfo.SecurityInfo.pCkipPara->bIsKP )
			{ // WEP case.
			SecSWWEPEncryption(Adapter, pTcb);
			}
			else
			{ // CKIP case.
				SecSWCKIPEncryption(Adapter, pTcb);
			}
			break;
		case RT_ENC_ALG_TKIP:
			SecSWTKIPEncryption(Adapter, pTcb);
			break;
		case RT_ENC_ALG_AESCCMP:
			SecSWAESEncryption(Adapter, pTcb);
			break;

		case RT_ENC_ALG_SMS4:
			WAPI_SecFuncHandler(WAPI_SECSWSMS4ENCRYPTION, Adapter, (PVOID)pTcb, WAPI_END);
		      break;

		default:
			break;
	}
}


// From kcwu 2004/09/24.
VOID
SecSWTKIPEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	u4Byte				FragIndex = 0;
	u4Byte				BufferIndex = 0;
	u2Byte				FragBufferCount = 0;
	PRT_SECURITY_T		pSec = &Adapter->MgntInfo.SecurityInfo;
	//u1Byte				SecBuffer[2000];
	u4Byte				SpecificHeadOverhead = 0;
	u4Byte				SecBufLen = 0;
	u4Byte				SecBytesCopied = 0;
	u4Byte				SecBufIndex = 0;
	u4Byte				FragBufferIndexStart = 0;
	pu1Byte				SecPtr = 0;
	u4Byte				SecLenForCopy = 0;
	pu1Byte				pHeader = pTcb->BufferList[0].VirtualAddress;
	u1Byte				key[16];
	u2Byte				u2IV16;
	u4Byte				u4IV32;
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	PRT_WLAN_STA			pEntry = NULL;// = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);
	u1Byte				IVOffset;	// Added by Annie, 2005-12-23.
	u1Byte				DataOffset;	// Added by Annie, 2005-12-23.

	SpecificHeadOverhead = Adapter->TXPacketShiftBytes;

	pHeader += SpecificHeadOverhead;


	// AP mode: get key.
	if(ACTING_AS_AP(Adapter)) 
	{ // AP mode.
		if(MacAddr_isMulticast(pTcb->DestinationAddress))
		{
			// [AnnieNote] Current GTK is 12345678...12345678, so it's OK.
			// It's better to modify it by GTK length if we use PRF-X to generate GTK later. 2005-12-23.
			//
		
			//Check if set GTK was completed	
			if(Adapter->MgntInfo.globalKeyInfo.GTK[0] == 0)									
				return;
		}
		else
		{
			//Check if STA within associated lsit table				
			pEntry = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);		
			if(pEntry == NULL)			
				return;

			//Check if set PTK was completed		
			if(pEntry->perSTAKeyInfo.TempEncKey == NULL)									
				return;
		}
	}


	// Get offset. Annie, 2005-12-23.
	if( IsQoSDataFrame(pHeader) )
	{
		IVOffset = sMacHdrLng + sQoSCtlLng;
	}
	else
	{
		IVOffset = sMacHdrLng;
	}
	DataOffset = IVOffset + EXT_IV_LEN;


	// Get u2IV16 and u4IV32.
	u2IV16 =	((u2Byte)(*(pHeader+IVOffset + 0)) <<  8) +		\
			((u2Byte)(*(pHeader+IVOffset + 2)) <<  0);
	
	u4IV32 = ((u4Byte) (*(pHeader+IVOffset + 4)) <<  0) +		\
			((u4Byte) (*(pHeader+IVOffset + 5)) <<  8) +		\
			((u4Byte) (*(pHeader+IVOffset + 6)) << 16) +		\
			((u4Byte) (*(pHeader+IVOffset + 7)) << 24);


	if(ACTING_AS_AP(Adapter)) 
	{ // AP mode.
		if(MacAddr_isMulticast(pTcb->DestinationAddress))
		{
			TKIPGenerateKey(key, u4IV32, u2IV16, pMgntInfo->Bssid, &Adapter->MgntInfo.globalKeyInfo.GTK[0]);
		}
		else
		{
			if(pEntry != NULL)
				TKIPGenerateKey(key, u4IV32, u2IV16, pMgntInfo->Bssid, pEntry->perSTAKeyInfo.TempEncKey);
		}
	} 
	else
	{ // STA mode.
		TKIPGenerateKey(key, u4IV32, u2IV16, pTcb->SourceAddress, pSec->KeyBuf[SecGetTxKeyIdx(Adapter, pTcb->DestinationAddress)]);
	}
	//TKIP_TSC_DECIMAL2ARRAY(pSec->SWTKIPu2IV16, pSec->SWTKIPu4IV32, ulKeyIndex, pucIV);

	for(FragIndex = 0; FragIndex < pTcb->FragCount; FragIndex++)
	{
		FragBufferIndexStart = BufferIndex;
		SecBufLen = 0;

		//2004/09/14, kcwu, clear the Security Coalesce Buffer
		PlatformZeroMemory(pSec->SecBuffer, SW_ENCRYPT_BUF_SIZE);
		
		//2004/09/14, kcwu, to show how many buffers are there used by this fragment
		FragBufferCount = pTcb->FragBufCount[FragIndex];

		//2004/09/14, kcwu, data to be encrypted start at the second buffer
		if(!TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{
			BufferIndex = FragBufferIndexStart+1;
			FragBufferCount--; // 2005.11.04, by rcnjko.
		}

		// Concatenate buffers to be encrypted into the buffer, SecBuffer.
		// SecBuffer for TKIP does not include 802.11 header and IV.
		while(FragBufferCount-- > 0)
		{
			if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
			{
				if(pTcb->BufferList[BufferIndex].Length > (SpecificHeadOverhead+DataOffset))
				{
					SecPtr = pTcb->BufferList[BufferIndex].VirtualAddress + (SpecificHeadOverhead+DataOffset);
					SecLenForCopy = pTcb->BufferList[BufferIndex].Length - (SpecificHeadOverhead+DataOffset);
				}
			}
			else
			{
				SecPtr = pTcb->BufferList[BufferIndex].VirtualAddress;
				SecLenForCopy = pTcb->BufferList[BufferIndex].Length;
			}

			// 20110819 Joseph: Prevent from buffer overflow.
			if((SecBufLen + SecLenForCopy)>SW_ENCRYPT_BUF_SIZE)
			{
				SecLenForCopy = SW_ENCRYPT_BUF_SIZE - SecBufLen;
				FragBufferCount = 0;
				RT_TRACE( COMP_SEC, DBG_SERIOUS, ("SecSWTKIPEncryption(): Packet is too large. Truncate!!\n") );
			}

			PlatformMoveMemory(
				pSec->SecBuffer + SecBufLen, // 2005.11.04, by rcnjko.
				SecPtr,
				SecLenForCopy);

			SecBufLen += SecLenForCopy;

			BufferIndex++;
			//DbgPrint("FragBufferCount = %d, SecBufLen = %d, BufferIndex = %d\n", 
			//	FragBufferCount, SecBufLen, BufferIndex);
			
		}

		if(SecBufLen <= 0)
		{
			continue;
		}
		//PRINT_DATA("SecSWWEPEncryption_before_encrypted==>", SecBuffer, SecBufLen);
		
		EncodeWEP(key,
			16,
			pSec->SecBuffer,
			SecBufLen,
			pSec->SecBuffer);
		//PRINT_DATA("SecSWWEPEncryption_after_encrypted==>", SecBuffer, SecBufLen+4);

		// For WEP ICV.
		SecBufLen += 4;

		SecBytesCopied = 0;

		// 2005.11.04, by rcnjko.
		if(!TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{
			SecBufIndex = FragBufferIndexStart+1;
			FragBufferCount = 1; 
		}
		else
		{
			SecBufIndex = FragBufferIndexStart;
			FragBufferCount = 0;
		}
		//

		//2004/09/14, then copy SecBuffer back to the buffer

		// 2005.11.04, by rcnjko.
		if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{ 
			PlatformMoveMemory(
					pTcb->BufferList[SecBufIndex].VirtualAddress+(SpecificHeadOverhead+DataOffset),
					pSec->SecBuffer+SecBytesCopied,
					pTcb->BufferList[SecBufIndex].Length-(SpecificHeadOverhead+DataOffset)
					);
			SecBytesCopied += pTcb->BufferList[SecBufIndex].Length-(SpecificHeadOverhead+DataOffset);
			SecBufIndex++;
			FragBufferCount++;
		}
		//

		while(FragBufferCount < pTcb->FragBufCount[FragIndex]){
			PlatformMoveMemory(
				pTcb->BufferList[SecBufIndex].VirtualAddress,
				pSec->SecBuffer+SecBytesCopied,
				pTcb->BufferList[SecBufIndex].Length
				);
			SecBytesCopied += pTcb->BufferList[SecBufIndex].Length;
			SecBufIndex++;
			FragBufferCount++;
		}
		
		// Copy WEP ICV. 
		// 2005.11.04, by rcnjko.
		if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER) || pTcb->FragCount == 1)
		{
			PlatformMoveMemory(
					pTcb->BufferList[SecBufIndex-1].VirtualAddress+pTcb->BufferList[SecBufIndex-1].Length,
					pSec->SecBuffer+SecBytesCopied,
					pSec->EncryptionTailOverhead
					);
			pTcb->BufferList[SecBufIndex-1].Length += pSec->EncryptionTailOverhead;  //4;  //Fix MPE SUT System Hang,120715, by YJ.
			SecBytesCopied += pSec->EncryptionTailOverhead;
			pTcb->FragLength[FragIndex] += pSec->EncryptionTailOverhead;
			pTcb->PacketLength += pSec->EncryptionTailOverhead;
		}
		else
		{
			// TODO: non-coalesce and fragmneted case.
		}
	}
}

// From kcwu 2004/09/24.
VOID
SecSWWEPEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	u4Byte				FragIndex = 0;
	u4Byte				BufferIndex = 0;
	u2Byte				FragBufferCount = 0;
	u1Byte				key[16];
	u4Byte			wepIV=0;
	PRT_SECURITY_T		pSec = &Adapter->MgntInfo.SecurityInfo;
	//u1Byte				SecBuffer[2000]; // 3000 -> 2000 to prevent stack overflow problem in Win98, 2005.03.15, by rcnjko.
	u4Byte				SpecificHeadOverhead = 0;
	u4Byte				SecBufLen = 0;
	u4Byte				SecBytesCopied = 0;
	u4Byte				SecBufIndex = 0;
	u4Byte				FragBufferIndexStart = 0;
	pu1Byte				SecPtr = 0;
	u4Byte				SecLenForCopy = 0;
	pu1Byte				pHeader;
	u1Byte				IVOffset;	// Added by Annie, 2005-12-23.
	u1Byte				DataOffset;	// Added by Annie, 2005-12-23.

	pHeader = pTcb->BufferList[FragBufferIndexStart].VirtualAddress;

	SpecificHeadOverhead = Adapter->TXPacketShiftBytes;

	pHeader += SpecificHeadOverhead;


	// Get offset. Annie, 2005-12-23.
	if( IsQoSDataFrame(pHeader) )
	{
		IVOffset = sMacHdrLng + sQoSCtlLng;
	}
	else
	{
		IVOffset = sMacHdrLng;
	}
	DataOffset = IVOffset + WEP_IV_LEN;
	

	//2004/09/14, kcwu, the IV has been calculate, just get from current IV
	//wepIV.ul = (u4Byte) ((pSec->TxIV == 0)?0x00fffffe:((pSec->TxIV-1)&0x00ffffff));
	SET_WEP_IV_INITVECTOR(&wepIV, *(UNALIGNED pu4Byte)(pHeader+IVOffset));
	
	SET_WEP_IV_KEYID(& wepIV, pSec->DefaultTransmitKeyIdx);


	//PRINT_DATA("SecSWWEPEncryption==>", (pu1Byte)&wepIV, 4);
	
	PlatformMoveMemory(key, (pHeader+IVOffset), 3); 
	PlatformMoveMemory(key+3, pSec->KeyBuf[pSec->DefaultTransmitKeyIdx], 
		(pSec->PairwiseEncAlgorithm==RT_ENC_ALG_WEP104)?13:5);

	for(FragIndex = 0; FragIndex < pTcb->FragCount; FragIndex++)
	{
		FragBufferIndexStart = BufferIndex;
		SecBufLen = 0;

		//2004/09/14, kcwu, clear the Security Coalesce Buffer
		PlatformZeroMemory(pSec->SecBuffer, SW_ENCRYPT_BUF_SIZE); // 3000 -> 2000 to prevent stack overflow problem in Win98, 2005.03.15, by rcnjko.
		
		//2004/09/14, kcwu, to show how many buffers are there used by this fragment
		FragBufferCount = pTcb->FragBufCount[FragIndex];

		// For non-coalesce case, data to be encrypted start at the second buffer.
		if(!TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{
			BufferIndex = FragBufferIndexStart+1;
			FragBufferCount--; // 2005.11.07, by rcnjko. 
		}

		// Concatenate buffers to be encrypted into the buffer, SecBuffer.
		// SecBuffer for WEP does not include 802.11 header and IV.
		while(FragBufferCount-- > 0)
		{
			if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
			{
				if(pTcb->BufferList[BufferIndex].Length > (SpecificHeadOverhead+DataOffset))
				{
					SecPtr = pTcb->BufferList[BufferIndex].VirtualAddress + (SpecificHeadOverhead+DataOffset);
					SecLenForCopy = pTcb->BufferList[BufferIndex].Length - (SpecificHeadOverhead+DataOffset);
				}
			}
			else
			{
				SecPtr = pTcb->BufferList[BufferIndex].VirtualAddress;
				SecLenForCopy = pTcb->BufferList[BufferIndex].Length;
			}

			// 20110819 Joseph: Prevent from buffer overflow.
			if((SecBufLen + SecLenForCopy)>SW_ENCRYPT_BUF_SIZE)
			{
				SecLenForCopy = SW_ENCRYPT_BUF_SIZE - SecBufLen;
				FragBufferCount = 0;
				RT_TRACE( COMP_SEC, DBG_SERIOUS, ("SecSWWEPEncryption(): Packet is too large. Truncate!!\n") );
			}

			PlatformMoveMemory(
				pSec->SecBuffer + SecBufLen, // 2005.11.07, by rcnjko. 
				SecPtr,
				SecLenForCopy);

			SecBufLen += SecLenForCopy;

			BufferIndex++;
			//DbgPrint("FragBufferCount = %d, SecBufLen = %d, BufferIndex = %d\n", 
			//	FragBufferCount, SecBufLen, BufferIndex);
		}

		if(SecBufLen <= 0)
		{
			continue;
		}

		//PRINT_DATA("SecSWWEPEncryption_before_encrypted==>", pSec->SecBuffer, SecBufLen);
		EncodeWEP(key,
			(pSec->PairwiseEncAlgorithm==RT_ENC_ALG_WEP104)?16:8,
			pSec->SecBuffer,
			SecBufLen,
			pSec->SecBuffer);
		//PRINT_DATA("SecSWWEPEncryption_after_encrypted==>", pSec->SecBuffer, SecBufLen+4);
	
		// For WEP ICV.
		SecBufLen += 4;

		SecBytesCopied = 0;
		
		// 2005.11.07, by rcnjko. 
		if(!TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{
			SecBufIndex = FragBufferIndexStart+1;
			FragBufferCount = 1;
		}
		else
		{
			SecBufIndex = FragBufferIndexStart;
			FragBufferCount = 0;
		}
		//

		//2004/09/14, then copy SecBuffer back to the buffer
		// 2005.11.04, by rcnjko.	
		if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{ 
			PlatformMoveMemory(
					pTcb->BufferList[SecBufIndex].VirtualAddress+(SpecificHeadOverhead+DataOffset),
					pSec->SecBuffer+SecBytesCopied,
					pTcb->BufferList[SecBufIndex].Length-(SpecificHeadOverhead+DataOffset)
					);
			SecBytesCopied += pTcb->BufferList[SecBufIndex].Length-(SpecificHeadOverhead+DataOffset);
			SecBufIndex++;
			FragBufferCount++;
		}
		//
		
		while(FragBufferCount < pTcb->FragBufCount[FragIndex])
		{
			PlatformMoveMemory(
				pTcb->BufferList[SecBufIndex].VirtualAddress,
				pSec->SecBuffer+SecBytesCopied,
				pTcb->BufferList[SecBufIndex].Length
				);
			SecBytesCopied += pTcb->BufferList[SecBufIndex].Length;
			SecBufIndex++;
			FragBufferCount++;
		}

		// Copy WEP ICV. 
		// 2005.11.07, by rcnjko.
		if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{
			PlatformMoveMemory(
					pTcb->BufferList[SecBufIndex-1].VirtualAddress+pTcb->BufferList[SecBufIndex-1].Length,
					pSec->SecBuffer+SecBytesCopied,
					pSec->EncryptionTailOverhead
					);
			pTcb->BufferList[SecBufIndex-1].Length += pSec->EncryptionTailOverhead; // 4;  //Fix MPE SUT System Hang,120715, by YJ.
			SecBytesCopied += pSec->EncryptionTailOverhead;
			pTcb->FragLength[FragIndex] += pSec->EncryptionTailOverhead;
			pTcb->PacketLength += pSec->EncryptionTailOverhead;
		}
		else 
		{
			// For non-coalesce case, the WEP ICV cannot append to last buffer of the fragment directly,
			// because the buffer might belong to upper layer and we are NOT allow to change its length.
			if(pTcb->FragCount == 1)
			{
				PlatformMoveMemory(
						pTcb->Tailer.VirtualAddress,
						pSec->SecBuffer+SecBytesCopied,
						pSec->EncryptionTailOverhead
						);
				pTcb->Tailer.Length = pSec->EncryptionTailOverhead;
	
				pTcb->BufferList[pTcb->BufferCount] = pTcb->Tailer;
				pTcb->BufferCount++;
	
				pTcb->FragLength[FragIndex] += pSec->EncryptionTailOverhead;
				pTcb->FragBufCount[FragIndex]++;
				pTcb->PacketLength += pSec->EncryptionTailOverhead;
	
				SecBytesCopied += pSec->EncryptionTailOverhead;
			}
			else
			{
				// TODO: non-coalesce and fragmneted case.
			}
		}
		//
	}
}

VOID
SecSWAESEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	u4Byte				keyidx=0;
	u4Byte				FragIndex = 0;
	u4Byte				BufferIndex = 0;
	u2Byte				FragBufferCount = 0;
	PRT_SECURITY_T		pSec = &Adapter->MgntInfo.SecurityInfo;
	//u1Byte				SecBuffer[2000];
	u4Byte				SpecificHeadOverhead = 0;
	u4Byte				SecBufLen = 0;
	u4Byte				SecBytesCopied = 0;
	u4Byte				SecBufIndex = 0;
	u4Byte				FragBufferIndexStart = 0;
	pu1Byte				SecPtr = 0;
	u4Byte				SecLenForCopy = 0;
	pu1Byte				pHeader = pTcb->BufferList[0].VirtualAddress;
	u1Byte				IVOffset;	// Added by Annie, 2005-12-23.
	u1Byte				DataOffset;	// Added by Annie, 2005-12-23.
	PRT_WLAN_STA		pEntry;
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;	
	OCTET_STRING		frame;

	SpecificHeadOverhead = Adapter->TXPacketShiftBytes;

	pHeader += SpecificHeadOverhead;

	FillOctetString(frame, pHeader, (u2Byte)pTcb->PacketLength);

	// Get offset. Annie, 2005-12-23.
	if( IsQoSDataFrame(pHeader) )
	{
		IVOffset = sMacHdrLng + sQoSCtlLng;
	}
	else
	{
		IVOffset = sMacHdrLng;
	}

	// check address 4 
	if( Frame_ValidAddr4(frame) )
	{
		RT_TRACE( COMP_SEC , DBG_LOUD , ("====>ccw BT data SecSWAESEncryption \n") );
		IVOffset += ETHERNET_ADDRESS_LENGTH;
	}
	
	DataOffset = IVOffset + EXT_IV_LEN;


	if(ACTING_AS_AP(Adapter)) 
	{
		if(MacAddr_isMulticast(pTcb->DestinationAddress))
		{
			// TODO:
			//keyidx = 
		}
		else
		{
			// TODO:
			//keyidx = 
		}
	} 
	else
	{
		keyidx = SecGetTxKeyIdx(Adapter, pTcb->DestinationAddress);
	}

	for(FragIndex = 0; FragIndex < pTcb->FragCount; FragIndex++)
	{
		FragBufferIndexStart = BufferIndex;
		SecBufLen = 0;

		//2004/09/14, kcwu, clear the Security Coalesce Buffer
		PlatformZeroMemory(pSec->SecBuffer, SW_ENCRYPT_BUF_SIZE);
		
		//2004/09/14, kcwu, to show how many buffers are there used by this fragment
		FragBufferCount = pTcb->FragBufCount[FragIndex];

		// Concatenate buffers to be encrypted into the buffer, SecBuffer.
		// SecBuffer for AES contains 802.11 header and IV information to encrypt data.
		while(FragBufferCount-- > 0)
		{
			if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
			{
				if(pTcb->BufferList[BufferIndex].Length > SpecificHeadOverhead)
				{
					SecPtr = pTcb->BufferList[BufferIndex].VirtualAddress + SpecificHeadOverhead;
					SecLenForCopy = pTcb->BufferList[BufferIndex].Length - SpecificHeadOverhead;
				}
			}
			else
			{
				SecPtr = pTcb->BufferList[BufferIndex].VirtualAddress;
				SecLenForCopy = pTcb->BufferList[BufferIndex].Length;
			}

			// 20110819 Joseph: Prevent from buffer overflow.
			if((SecBufLen + SecLenForCopy)>SW_ENCRYPT_BUF_SIZE)
			{
				SecLenForCopy = SW_ENCRYPT_BUF_SIZE - SecBufLen;
				FragBufferCount = 0;
				RT_TRACE( COMP_SEC, DBG_SERIOUS, ("SecSWAESEncryption(): Packet is too large. Truncate!!\n") );
			}

			PlatformMoveMemory(
				pSec->SecBuffer + SecBufLen, // 2005.11.07, by rcnjko. 
				SecPtr,
				SecLenForCopy);

			SecBufLen += SecLenForCopy;

			BufferIndex++;
			//DbgPrint("FragBufferCount = %d, SecBufLen = %d, BufferIndex = %d\n", 
			//	FragBufferCount, SecBufLen, BufferIndex);
		}

		if(SecBufLen <= 0)
		{
			continue;
		}

#ifdef SW_TXENCRYPTION_DBG
		RT_TRACE(COMP_SEC, DBG_LOUD, ("\n\nKeyIndex = %d\n", SecGetTxKeyIdx(Adapter, pTcb->DestinationAddress)));
		PRINT_DATA("AESKeyBuf===>", pSec->AESKeyBuf[SecGetTxKeyIdx(Adapter, pTcb->DestinationAddress)], 16);
		PRINT_DATA("SecBuffer_before ====>", pSec->SecBuffer, SecBufLen);
#endif
		if(pTcb->bTDLPacket)
		{
			pEntry = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);		
			if(pEntry == NULL)			
				return;

			//Check if set PTK was completed		
			if(pEntry->perSTAKeyInfo.AESKeyBuf[0] == 0)									
				return;
			
			SecRSNAEncodeAESCCM(
				&Adapter->MgntInfo.SecurityInfo,
				(u4Byte *)pEntry->perSTAKeyInfo.AESKeyBuf,
				0,
				pSec->SecBuffer,
				IVOffset,
				SecBufLen
				);
		}
		else if( !Adapter->MgntInfo.bRSNAPSKMode && !ACTING_AS_AP(Adapter) )
		{   // NO RSNA-PSK and AP-mode
			if(pTcb->EncInfo.bMFPPacket && pMgntInfo->bInBIPMFPMode)
			{
				SecMFPEncodeAESCCM_1W(
					&Adapter->MgntInfo.SecurityInfo,
					(u4Byte *)Adapter->MgntInfo.SecurityInfo.AESKeyBuf[0],
					keyidx,
					pSec->SecBuffer,
					IVOffset,
					SecBufLen
					);
				//RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "BIP AES EncData:\n", pHeader, pTcb->PacketLength);
			}
			else
			{
				SecEncodeAESCCM(
					&Adapter->MgntInfo.SecurityInfo,
					(u4Byte *)Adapter->MgntInfo.SecurityInfo.AESKeyBuf[keyidx],
					pSec->SecBuffer,
					IVOffset,
					SecBufLen
					);
			}
		}
		//AP WPA AES, CCW
		else if( ACTING_AS_AP(Adapter) )
		{  // AP mode
			
			
			if(MacAddr_isMulticast(pTcb->DestinationAddress))
			{// Multicase packet	 , used AESGTK		
				//RT_TRACE(COMP_WPAAES, DBG_LOUD, ("<===== AP WPA AES Multicase\n"));
				//Check if set GTK was completed	
				if(Adapter->MgntInfo.globalKeyInfo.AESGTK[0] == 0)									
					return;
				SecRSNAEncodeAESCCM(
					&Adapter->MgntInfo.SecurityInfo,
					(u4Byte *)Adapter->MgntInfo.globalKeyInfo.AESGTK, 
					pSec->GroupTransmitKeyIdx,
					pSec->SecBuffer,
					IVOffset,
					SecBufLen
					);
				//RT_TRACE(COMP_WPAAES, DBG_LOUD, ("=====> AP WPA AES Multicase\n"));
			}
		else
			{ // unitcase packet , used Entry Pairwise key
				//RT_TRACE(COMP_WPAAES, DBG_LOUD, ("<===== AP WPA AES unitcase\n"));
				//Check if STA within associated lsit table				
				pEntry = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);		
				if(pEntry == NULL)			
					return;

				//Check if set PTK was completed		
				if(pEntry->perSTAKeyInfo.AESKeyBuf[0] == 0)									
					return;
				
				SecRSNAEncodeAESCCM(
					&Adapter->MgntInfo.SecurityInfo,
					(u4Byte *)pEntry->perSTAKeyInfo.AESKeyBuf,
					0,
					pSec->SecBuffer,
					IVOffset,
					SecBufLen
					);
				//RT_TRACE(COMP_WPAAES, DBG_LOUD, ("=====> AP WPA AES unitcase\n"));
			}
		}
		else
		{ // RSNA-PSK mode
			OCTET_STRING	pduOS; 
			pu1Byte	pRA;
			pu1Byte	pTA;
			u4Byte index;

			FillOctetString(pduOS, pHeader, IVOffset);
			pRA = Frame_Addr1(pduOS);
			pTA = Frame_Addr2(pduOS);

			RT_TRACE(COMP_RSNA, DBG_LOUD,( "=====>RSNA IBSS Send Encrypt packet \n"));
			RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, "RSNA RA : ", pRA);
			RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, "RSNA TA : ", pTA);
			if(MacAddr_isMulticast(pRA))
			{
				PPER_STA_DEFAULT_KEY_ENTRY         pDefKey;         // Add by CCW

				//
				// TODO: remvoe our group key from PerStaDefKeyTable[]
				//
	
				//Multicast packet
				//Find out Default KEY Entry  
				pDefKey = pSec->PerStaDefKeyTable;
				for( index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++, pDefKey++)
				{
					if( pDefKey->Valid && (PlatformCompareMemory(pDefKey->MACAdrss , pTA, 6)==0))
						break;
				}
	
				if(index != MAX_NUM_PER_STA_KEY)
				{
					RT_TRACE(COMP_RSNA, DBG_LOUD, ("DefaultTransmitKeyIdx: %d\n", pSec->DefaultTransmitKeyIdx));
					RT_PRINT_DATA(COMP_RSNA, DBG_LOUD, " Multicase Default key : ", pDefKey->DefKeyBuf[pSec->DefaultTransmitKeyIdx], 16);
					SecRSNAEncodeAESCCM(
						&Adapter->MgntInfo.SecurityInfo,
						(u4Byte *)pDefKey->DefKeyBuf[pSec->DefaultTransmitKeyIdx],
						pSec->DefaultTransmitKeyIdx,
						pSec->SecBuffer,
						IVOffset,
						SecBufLen
						);
				}
				else
				{ // no key found!
					RT_PRINT_ADDR(COMP_RSNA, DBG_WARNING, ("SecSWAESEncryption(): No group key found for pTA: \n"), pTA);
				}
			}
			else
			{
				PPER_STA_MPAKEY_ENTRY pMAPkey; // Add by CCW

				//unicast packet
				//Find out MAPPING Key Entry.
				pMAPkey = pSec->MAPKEYTable;
				for( index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++, pMAPkey++)
				{
					if( pMAPkey->Valid && (PlatformCompareMemory(pMAPkey->MACAdrss , pRA, 6)==0))
						break;
				}

				if(index != MAX_NUM_PER_STA_KEY)
				{
					RT_PRINT_DATA(COMP_RSNA, DBG_LOUD, " Unicase Key Mapping key : ", pMAPkey->MapKeyBuf , 16);
					SecRSNAEncodeAESCCM(
						&Adapter->MgntInfo.SecurityInfo,
						(u4Byte *)pMAPkey->MapKeyBuf,
						0,  // default = 0.
						pSec->SecBuffer,
						IVOffset,
						SecBufLen
						);
				}
				else
				{ // no key found!
					RT_PRINT_ADDR(COMP_RSNA, DBG_WARNING, ("SecSWAESEncryption(): No pairwse key found for pRA: \n"), pRA);
				}
			}	
		}
		// For AES MIC.
		SecBufLen += 8;

#ifdef SW_TXENCRYPTION_DBG
		PRINT_DATA("SecBuffer_after ====>", pSec->SecBuffer, SecBufLen);
		if(SecBufLen == 147)
		{
			RT_TRACE(COMP_SEC, DBG_LOUD, ("SecSWAESEncryption:SecBufLen = %d\n", SecBufLen));
		}
#endif

		SecBytesCopied = 0;
		
		// 2005.11.07, by rcnjko. 
		if(!TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{
			SecBufIndex = FragBufferIndexStart+1;
			FragBufferCount = 1;
		}
		else
		{
			SecBufIndex = FragBufferIndexStart;
			FragBufferCount = 0;
		}
		//

		//2004/09/14, then copy SecBuffer back to the buffer
		// 2005.11.07, by rcnjko.
		if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{
			PlatformMoveMemory(
					pTcb->BufferList[SecBufIndex].VirtualAddress + (SpecificHeadOverhead+DataOffset),
					pSec->SecBuffer+DataOffset+SecBytesCopied,
					pTcb->BufferList[SecBufIndex].Length - (SpecificHeadOverhead+DataOffset)
					);
			SecBytesCopied += pTcb->BufferList[SecBufIndex].Length - (SpecificHeadOverhead+DataOffset);
			SecBufIndex++;
			FragBufferCount++;
		}
		//
		
		while(FragBufferCount < pTcb->FragBufCount[FragIndex])
		{
			PlatformMoveMemory(
				pTcb->BufferList[SecBufIndex].VirtualAddress,
				pSec->SecBuffer+DataOffset+SecBytesCopied, // 2005.11.07, by rcnjko.
				pTcb->BufferList[SecBufIndex].Length
				);
			SecBytesCopied += pTcb->BufferList[SecBufIndex].Length;
			SecBufIndex++;
			FragBufferCount++;
		}

		// Copy AES MIC. 
		// 2005.11.07, by rcnjko.
		if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
		{
			PlatformMoveMemory(
					pTcb->BufferList[SecBufIndex-1].VirtualAddress+pTcb->BufferList[SecBufIndex-1].Length,
					pSec->SecBuffer+DataOffset+SecBytesCopied,
					8//pSec->EncryptionTailOverhead
					);
			pTcb->BufferList[SecBufIndex-1].Length += 8;//pSec->EncryptionTailOverhead;
			SecBytesCopied += 8;//pSec->EncryptionTailOverhead;
			pTcb->FragLength[FragIndex] += 8;//pSec->EncryptionTailOverhead;
			pTcb->PacketLength += 8;//pSec->EncryptionTailOverhead;
		}
		else
		{
			// For non-coalesce case, the AES MIC cannot append to last buffer of the fragment directly,
			// because the buffer might belong to upper layer and we are NOT allow to change its length.
			if(pTcb->FragCount == 1)
			{
				PlatformMoveMemory(
						pTcb->Tailer.VirtualAddress,
						pSec->SecBuffer+DataOffset+SecBytesCopied,
						8//pSec->EncryptionTailOverhead
						);
				pTcb->Tailer.Length = 8;//pSec->EncryptionTailOverhead;
	
				pTcb->BufferList[pTcb->BufferCount] = pTcb->Tailer;
				pTcb->BufferCount++;
	
				pTcb->FragLength[FragIndex] += 8;//pSec->EncryptionTailOverhead;
				pTcb->FragBufCount[FragIndex]++;
				pTcb->PacketLength +=  8;//pSec->EncryptionTailOverhead;
	
				SecBytesCopied += 8;//pSec->EncryptionTailOverhead;
			}
			else
			{
				// TODO: non-coalesce and fragmneted case.
			}
		}
		//
	}
}


//
// Software encryption for CKIP.
// Ported from CKIP code, Annie, 2006-08-11. [AnnieTODO]
//
VOID
SecSWCKIPEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	u4Byte			FragIndex = 0;
	u4Byte			BufferIndex = 0;
	u2Byte			FragBufferCount = 0;
	u4Byte			FragBufferIndexStart = 0;
	u4Byte			SecBufLen = 0;
	u4Byte			SecBufIndex = 0;
	pu1Byte			SecPtr = 0;
	u4Byte			SecLenForCopy = 0;
	pu1Byte			pucIV =0;
	PRT_SECURITY_T	pSec =	&Adapter->MgntInfo.SecurityInfo;
	ULONG			keyidx = pSec->DefaultTransmitKeyIdx;
	//UCHAR			key[16] = {0};
	pu1Byte			pHeader = pTcb->BufferList[0].VirtualAddress;
	//u1Byte			SecBuffer[2000] = {0};
	u4Byte			SpecificHeadOverhead = 0;
	u1Byte			IVOffset;
	u1Byte			DataOffset;
	u4Byte			SecBytesCopied = 0;

	RT_TRACE( COMP_CKIP, DBG_TRACE, ("==> SecSWCKIPEncryption()\n") );

	SpecificHeadOverhead = Adapter->TXPacketShiftBytes;

	pHeader += SpecificHeadOverhead;


	if( !pSec->pCkipPara->bIsKP )
	{ //Pure WEP
		RT_TRACE( COMP_CKIP, DBG_LOUD, ("SecSWCKIPEncryption(): Use pure WEP encryption.\n") );
		SecSWWEPEncryption(Adapter,pTcb);
	}
	else
	{ // Do Key Permutation
		RT_TRACE( COMP_CKIP, DBG_TRACE, ("SecSWCKIPEncryption(): CKIP case: Do Key Permutation.\n") );
		for(FragIndex = 0; FragIndex < pTcb->FragCount; FragIndex++)
		{
			FragBufferIndexStart = BufferIndex;
			SecBufLen = 0;

			if( IsQoSDataFrame(pHeader) )
			{
				IVOffset = sMacHdrLng + sQoSCtlLng;
			}
			else
			{
				IVOffset = sMacHdrLng;
			}		
			DataOffset = IVOffset + WEP_IV_LEN;  

			PlatformZeroMemory(pSec->SecBuffer, SW_ENCRYPT_BUF_SIZE);

			FragBufferCount = pTcb->FragBufCount[FragIndex];

			//2004/09/14, kcwu, data to be encrypted start at the second buffer
			if(!TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
			{
				BufferIndex = FragBufferIndexStart+1;
				FragBufferCount--; // 2005.11.04, by rcnjko.
			}

			// Concatenate buffers to be encrypted into the buffer, SecBuffer.
			// SecBuffer for TKIP does not include 802.11 header and IV.
			while(FragBufferCount-- > 0)
			{
				if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
				{
					if(pTcb->BufferList[BufferIndex].Length > (SpecificHeadOverhead+DataOffset))
					{
						SecPtr = pTcb->BufferList[BufferIndex].VirtualAddress + (SpecificHeadOverhead+DataOffset);
						SecLenForCopy = pTcb->BufferList[BufferIndex].Length - (SpecificHeadOverhead+DataOffset);
						// to get IV , by CCW
						pucIV = pTcb->BufferList[BufferIndex].VirtualAddress + (SpecificHeadOverhead+IVOffset);
					}
				}
				else
				{
					SecPtr = pTcb->BufferList[BufferIndex].VirtualAddress;
					SecLenForCopy = pTcb->BufferList[BufferIndex].Length;
				}

				// 20110819 Joseph: Prevent from buffer overflow.
				if((SecBufLen + SecLenForCopy)>SW_ENCRYPT_BUF_SIZE)
				{
					SecLenForCopy = SW_ENCRYPT_BUF_SIZE - SecBufLen;
					FragBufferCount = 0;
					RT_TRACE( COMP_SEC, DBG_SERIOUS, ("SecSWCKIPEncryption(): Packet is too large. Truncate!!\n") );
				}

				PlatformMoveMemory(
					pSec->SecBuffer + SecBufLen, // 2005.11.04, by rcnjko.
					SecPtr,
					SecLenForCopy);

				SecBufLen += SecLenForCopy;

				BufferIndex++;
				//DbgPrint("FragBufferCount = %d, SecBufLen = %d, BufferIndex = %d\n", 
				//	FragBufferCount, SecBufLen, BufferIndex);
				
			}

			if(SecBufLen <= 0)
			{
				continue;
			}

			ckip_encrypt(  pSec->pCkipPara->CKIPKeyBuf[keyidx] , pucIV , pSec->SecBuffer , SecBufLen );

			// For CKIP ICV.
			SecBufLen += 4;
			SecBytesCopied = 0;


			// 2005.11.04, by rcnjko.
			if(!TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
			{
				SecBufIndex = FragBufferIndexStart+1;
				FragBufferCount = 1; 
			}
			else
			{
				SecBufIndex = FragBufferIndexStart;
				FragBufferCount = 0;
			}
			//

			//2004/09/14, then copy SecBuffer back to the buffer

			// 2005.11.04, by rcnjko.
			if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
			{ 
				PlatformMoveMemory(
						pTcb->BufferList[SecBufIndex].VirtualAddress+(SpecificHeadOverhead+DataOffset),
						pSec->SecBuffer+SecBytesCopied,
						pTcb->BufferList[SecBufIndex].Length-(SpecificHeadOverhead+DataOffset)
						);
				SecBytesCopied += pTcb->BufferList[SecBufIndex].Length-(SpecificHeadOverhead+DataOffset);
				SecBufIndex++;
				FragBufferCount++;
			}
			//

			while(FragBufferCount < pTcb->FragBufCount[FragIndex]){
				PlatformMoveMemory(
					pTcb->BufferList[SecBufIndex].VirtualAddress,
					pSec->SecBuffer+SecBytesCopied,
					pTcb->BufferList[SecBufIndex].Length
					);
				SecBytesCopied += pTcb->BufferList[SecBufIndex].Length;
				SecBufIndex++;
				FragBufferCount++;
			}
		
			
			// Copy WEP ICV. 
			// 2005.11.07, by rcnjko.
			if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_USE_COALESCE_BUFFER))
			{
				PlatformMoveMemory(
						pTcb->BufferList[SecBufIndex-1].VirtualAddress+pTcb->BufferList[SecBufIndex-1].Length,
						pSec->SecBuffer+SecBytesCopied,
						pSec->EncryptionTailOverhead
						);
				pTcb->BufferList[SecBufIndex-1].Length += 4;
				SecBytesCopied += pSec->EncryptionTailOverhead;
				pTcb->FragLength[FragIndex] += pSec->EncryptionTailOverhead;
				pTcb->PacketLength += pSec->EncryptionTailOverhead;
			}
			else 
			{
				// For non-coalesce case, the WEP ICV cannot append to last buffer of the fragment directly,
				// because the buffer might belong to upper layer and we are NOT allow to change its length.
				if(pTcb->FragCount == 1)
				{
					PlatformMoveMemory(
							pTcb->Tailer.VirtualAddress,
							pSec->SecBuffer+SecBytesCopied,
							pSec->EncryptionTailOverhead
							);
					pTcb->Tailer.Length = pSec->EncryptionTailOverhead;
		
					pTcb->BufferList[pTcb->BufferCount] = pTcb->Tailer;
					pTcb->BufferCount++;
		
					pTcb->FragLength[FragIndex] += pSec->EncryptionTailOverhead;
					pTcb->FragBufCount[FragIndex]++;
					pTcb->PacketLength += pSec->EncryptionTailOverhead;
		
					SecBytesCopied += pSec->EncryptionTailOverhead;
				}
				else
				{
					// TODO: non-coalesce and fragmneted case.
				}
			}
		}
		
	}

	RT_TRACE( COMP_CKIP, DBG_TRACE, ("<== SecSWCKIPEncryption()\n") );
}


//
// Check IV increase
// 
// In Encrypt mode , IV is not increase ,and return FALSE.
//
RT_SEC_STATUS
SecRxCheckIV(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	RT_ENC_ALG		DecryptAlgorithm;
	RT_SEC_STATUS	secStatus = RT_SEC_STATUS_SUCCESS;
	pu1Byte			DestAddr;
	OCTET_STRING	Mpdu;
	BOOLEAN			bMulticastDest;
	u2Byte			EncryptionMPDUHeadOverhead, EncryptionMPDUTailOverhead;
	u2Byte			EncryptionMSDUHeadOverhead, EncryptionMSDUTailOverhead;
	u2Byte			MinEncPktSize;
	u1Byte			IVOffset;		// Added by Annie, 2005-12-23.
	PRT_SECURITY_T	pSec = &Adapter->MgntInfo.SecurityInfo;


	FillOctetString(Mpdu, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	DestAddr = Frame_pDaddr(Mpdu);
	bMulticastDest = MacAddr_isMulticast(DestAddr);
	
	if(!Frame_WEP(Mpdu))
	{ // Skip the non-encrypted frame, 2005.06.28, by rcnjko.
		return RT_SEC_STATUS_SUCCESS;
	}

	if(ACTING_AS_AP(Adapter))
		return RT_SEC_STATUS_SUCCESS;

	SecGetEncryptionOverhead(
		Adapter,
		&EncryptionMPDUHeadOverhead, 
		&EncryptionMPDUTailOverhead, 
		&EncryptionMSDUHeadOverhead, 
		&EncryptionMSDUTailOverhead,
		TRUE,
		bMulticastDest);
	MinEncPktSize = Frame_FrameHdrLng(Mpdu) + 
					EncryptionMPDUHeadOverhead + EncryptionMSDUHeadOverhead + 
					EncryptionMSDUTailOverhead + EncryptionMPDUTailOverhead;
	if(Mpdu.Length < MinEncPktSize)
	{ // 061214, rcnjo: Discard invalid length frame to prevent memory access violation.
		return RT_SEC_STATUS_INVALID_PKT_LEN;
	}


	if(bMulticastDest)
	{
		DecryptAlgorithm = Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm;
	}
	else
	{
		DecryptAlgorithm = Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm;
	}

	// Get offset. Annie, 2005-12-23.
	if( pRfd->Status.bIsQosData )
	{
		IVOffset = sMacHdrLng + sQoSCtlLng;
	}
	else
	{
		IVOffset = sMacHdrLng;
	}
	
	if(pRfd->Status.bContainHTC)
		IVOffset += sHTCLng;

	switch(DecryptAlgorithm)
	{
		case RT_ENC_ALG_WEP104:
		case RT_ENC_ALG_WEP40:
			{
				u4Byte			TempIV = 0;

				// Get IV
				TempIV = (((u4Byte)( Mpdu.Octet[IVOffset] )) << 16 ) + 
						 (((u4Byte)( Mpdu.Octet[IVOffset+1] )) << 8 ) +
						 (((u4Byte)( Mpdu.Octet[IVOffset+2] )) << 0 );

				// Check IV 
				// The 2 situations are not IV replay: (1) current IV of Pkt is greater than the rx IV (2) the current pkt is retry and IV is the same
				if(TempIV  > pSec->RXUntiIV || (Frame_Retry(Mpdu) && pSec->RXUntiIV == TempIV))
					pSec->RXUntiIV = TempIV;
				else
					secStatus = RT_SEC_STATUS_DATA_UNICAST_IV_REPLAY;
				
			}
			break;

		case RT_ENC_ALG_TKIP:
		case RT_ENC_ALG_AESCCMP:
			{
				u2Byte		u2IV16;
				u4Byte		u4IV32;
				u8Byte		TempIV;

				// Get u2IV16 and u4IV32.
				u2IV16 = ((u2Byte)(*(Mpdu.Octet+IVOffset + 0)) <<  8)+		\
						((u2Byte)(*(Mpdu.Octet+IVOffset + 2)) <<  0);
		
				u4IV32 = ((u4Byte) (*(Mpdu.Octet+IVOffset + 4)) <<  0)+		\
						((u4Byte) (*(Mpdu.Octet+IVOffset + 5)) <<  8)+		\
						((u4Byte) (*(Mpdu.Octet+IVOffset + 6)) << 16)+		\
						((u4Byte) (*(Mpdu.Octet+IVOffset + 7)) << 24);

				// Get IV
				TempIV = u2IV16 + ((u8Byte)u4IV32 << 16 );

				if( bMulticastDest )
				{
					// The 2 situations are not IV replay: (1) current IV of Pkt is greater than the rx IV (2) the current pkt is retry and IV is the same
					if( TempIV > pSec->RXMutiIV || (Frame_Retry(Mpdu) && pSec->RXMutiIV == TempIV))
						pSec->RXMutiIV = TempIV;
					else
						secStatus = RT_SEC_STATUS_DATA_MULTICAST_IV_REPLAY;
				}
				else
				{
					// The 2 situations are not IV replay: (1) current IV of Pkt is greater than the rx IV (2) the current pkt is retry and IV is the same
					if( TempIV > pSec->RXUntiIV || (Frame_Retry(Mpdu) && pSec->RXUntiIV == TempIV))
						pSec->RXUntiIV = TempIV;
					else
						secStatus = RT_SEC_STATUS_DATA_UNICAST_IV_REPLAY;
				}		
			}
			break;

		default:
			break;
	}

	if(secStatus != RT_SEC_STATUS_SUCCESS)
	{ // Count Relay statistics
		if(IsDataFrame(Mpdu.Octet))
		{
			switch(DecryptAlgorithm)
			{
			case RT_ENC_ALG_TKIP:
				CountRxTKIPRelpayStatistics(Adapter, pRfd);
				break;

			case RT_ENC_ALG_AESCCMP:
				CountRxCCMPRelpayStatistics(Adapter, pRfd);
				break;
			
			default: //for MacOSX Compiler warning.
				break;
			}
		}
		else if(IsMgntFrame(Mpdu.Octet))
		{
			secStatus = RT_SEC_STATUS_MGNT_IV_REPLAY;
			switch(DecryptAlgorithm)
			{
			case RT_ENC_ALG_TKIP:
				CountRxMgntTKIPRelpayStatistics(Adapter, pRfd);
				break;

			case RT_ENC_ALG_AESCCMP:
				CountRxMgntCCMPRelpayStatistics(Adapter, pRfd);
				break;
			
			default: //for MacOSX Compiler warning.
				break;		
			}
		}
	}
	
	return secStatus;
}


RT_SEC_STATUS
SecSoftwareDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	RT_ENC_ALG		DecryptAlgorithm;
	RT_SEC_STATUS	secStatus = RT_SEC_STATUS_SUCCESS;
	BOOLEAN			bResult= TRUE;
	pu1Byte			DestAddr;
	OCTET_STRING	Mpdu;
	BOOLEAN			bMulticastDest;
	u2Byte			EncryptionMPDUHeadOverhead, EncryptionMPDUTailOverhead;
	u2Byte			EncryptionMSDUHeadOverhead, EncryptionMSDUTailOverhead;
	u2Byte			MinEncPktSize;

	FillOctetString(Mpdu, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	DestAddr = Frame_pDaddr(Mpdu);
	bMulticastDest = MacAddr_isMulticast(DestAddr);

	do
	{
		if(!Frame_WEP(Mpdu))
		{ // Skip the non-encrypted frame, 2005.06.28, by rcnjko.
			secStatus = RT_SEC_STATUS_SUCCESS;
			break;
		}

		if(!pRfd->bRxBTdata)
		{
			if(!eqMacAddr(Frame_pBssid(Mpdu), Adapter->MgntInfo.Bssid))
			{				
				// RT_PRINT_ADDR(COMP_SEC, DBG_WARNING, "Mismatched BSSID (RT_SEC_STATUS_UNKNOWN_TA):\nFrame_pBssid: ", Frame_pBssid(Mpdu));
				// RT_PRINT_ADDR(COMP_SEC, DBG_WARNING, "MgntInfo.Bssid: ", Adapter->MgntInfo.Bssid);
				secStatus = RT_SEC_STATUS_UNKNOWN_TA;
				break;
			}
		}

		SecGetEncryptionOverhead(
			Adapter,
			&EncryptionMPDUHeadOverhead, 
			&EncryptionMPDUTailOverhead, 
			&EncryptionMSDUHeadOverhead, 
			&EncryptionMSDUTailOverhead,
			TRUE,
			bMulticastDest);
		MinEncPktSize = Frame_FrameHdrLng(Mpdu) + 
						EncryptionMPDUHeadOverhead + EncryptionMSDUHeadOverhead + 
						EncryptionMSDUTailOverhead + EncryptionMPDUTailOverhead;
		if(Mpdu.Length < MinEncPktSize)
		{ // 061214, rcnjo: Discard invalid length frame to prevent memory access violation.
			RT_TRACE_F(COMP_SEC, DBG_WARNING, ("RT_SEC_STATUS_INVALID_PKT_LEN, mpduLength(%d) < MinEncPktSize(%d)\n", Mpdu.Length, MinEncPktSize));
			secStatus = RT_SEC_STATUS_INVALID_PKT_LEN;
			break;
		}

		if(bMulticastDest)
		{
			DecryptAlgorithm = Adapter->MgntInfo.SecurityInfo.GroupEncAlgorithm;
		}
		else
		{
			DecryptAlgorithm = Adapter->MgntInfo.SecurityInfo.PairwiseEncAlgorithm;
		}

		if(pRfd->bRxBTdata)
		{
			DecryptAlgorithm = RT_ENC_ALG_AESCCMP;
		}

		switch(DecryptAlgorithm)
		{
			case RT_ENC_ALG_WEP104:
			case RT_ENC_ALG_WEP40:
				if( !Adapter->MgntInfo.SecurityInfo.pCkipPara->bIsKP )
				{ // WEP decryption
					bResult = SecSWWEPDecryption(Adapter, pRfd);
				}
				else
				{ // CKIP decryption
					bResult = SecSWCKIPDecryption(Adapter, pRfd);
				}
				break;

			case RT_ENC_ALG_TKIP:
				bResult = SecSWTKIPDecryption(Adapter, pRfd);
				break;

			case RT_ENC_ALG_AESCCMP:
				bResult = SecSWAESDecryption(Adapter, pRfd);
				break;

			case RT_ENC_ALG_SMS4:
				if(RT_STATUS_SUCCESS == WAPI_SecFuncHandler(WAPI_SECSWSMS4DECRYPTION, Adapter, (PVOID)pRfd, WAPI_END))
					bResult = TRUE;
			      break;

			default:
				break;
		}

		if(!bResult)
		{
			secStatus = RT_SEC_STATUS_ICV_ERROR;
			RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "SecSoftwareDecryption:() RT_SEC_STATUS_ICV_ERROR:\n", Mpdu.Octet, Mpdu.Length);
			if(Adapter->MgntInfo.uLastDecryptedErrorSeqNum == Frame_SeqNum(Mpdu))
			{
				break;
			}

			RT_TRACE(COMP_RECV, DBG_TRACE, ("SecSoftwareDecryption:() CountRxDecryptErrorStatistics\n"));

			CountRxDecryptErrorStatistics(Adapter, pRfd);
		
			switch(DecryptAlgorithm)
			{
				case RT_ENC_ALG_WEP104:
				case RT_ENC_ALG_WEP40:
					if( !Adapter->MgntInfo.SecurityInfo.pCkipPara->bIsKP )
					{ // WEP decryption
						CountWEPUndecryptableStatistics(Adapter, pRfd);
						CountWEPICVErrorStatistics(Adapter, pRfd);
					}

					break;

				case RT_ENC_ALG_TKIP:
					CountTKIPICVErrorStatistics(Adapter, pRfd);
					break;

				case RT_ENC_ALG_AESCCMP:
					CountRxCCMPDecryptErrorsStatistics(Adapter, pRfd);
					break;

				default:
					break;
			}

			Adapter->MgntInfo.uLastDecryptedErrorSeqNum = Frame_SeqNum(Mpdu);

			break;
		}
		else
			CountRxDecryptSuccessStatistics(Adapter, pRfd);	

		if(RT_SEC_STATUS_SUCCESS != secStatus)
			break;

		//3 // Check IV Replay
		//
		// In fact, we should discard this packet because of IV replay.
		// However, some packets may still be transmitted in disorder because the IV counter is filled by
		// driver but not HW. So these packets are indicated to the upper layer.
		// By Bruce, 2009-10-14.
		//
		
		secStatus = SecRxCheckIV(Adapter, pRfd);

		switch(secStatus)
		{
		case RT_SEC_STATUS_MGNT_IV_REPLAY:
			{
				RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "RT_SEC_STATUS_MGNT_IV_REPLAY:\n", Mpdu.Octet, Mpdu.Length);
			}
			break;

		default:
			// Skip normal case no matter if it is data IV replay.
			secStatus = RT_SEC_STATUS_SUCCESS;
			break;
		}
	}while(FALSE);
	
	return secStatus;
}

BOOLEAN
SecSWAESDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	u1Byte	keyidx;
	u4Byte 	micOK;
	u1Byte	IVOffset;		// Added by Annie, 2005-12-23.
	u1Byte	IVKeyIdOffset;	// Added by Annie, 2005-12-23.
	PRT_SECURITY_T		pSec = &Adapter->MgntInfo.SecurityInfo; // Add by CCW
	PRT_WLAN_STA		pEntry;
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	OCTET_STRING		frame;

	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

	IVOffset = sMacHdrLng;
	if( pRfd->Status.bIsQosData )
	{
		IVOffset += sQoSCtlLng;
	}
	if( pRfd->Status.bContainHTC)
		IVOffset += sHTCLng;

	// check address 4 
	if( Frame_ValidAddr4(frame) )
	{
		IVOffset += ETHERNET_ADDRESS_LENGTH;
	}
	
	IVKeyIdOffset = IVOffset + 3;
	
	keyidx = SecGetRxKeyIdx(
				Adapter,
				pRfd->Buffer.VirtualAddress+4,
				((pRfd->Buffer.VirtualAddress[IVKeyIdOffset]>>6)&0x03)
				);

	if(pRfd->bTDLPacket)
	{
		pu1Byte	pTA;
		OCTET_STRING	pduOS; 

		FillOctetString(pduOS, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
		pTA = Frame_Addr2(pduOS);
		
		pEntry = AsocEntry_GetEntry(pMgntInfo, pTA);		
		if(pEntry == NULL)	
			return FALSE;

		//Check if set PTK was completed		
		if(pEntry->perSTAKeyInfo.AESKeyBuf[0] == 0)
			return FALSE;
		
		micOK = SecDecodeAESCCM(
				&Adapter->MgntInfo.SecurityInfo,
				(u4Byte *)pEntry->perSTAKeyInfo.AESKeyBuf,
				pRfd->Buffer.VirtualAddress,
				IVOffset,
				pRfd->PacketLength
				);
	}
	else if( !Adapter->MgntInfo.bRSNAPSKMode && !ACTING_AS_AP(Adapter))
	{
		if ( pMgntInfo->bInBIPMFPMode && pRfd->Status.bRxMFPPacket )
		{
			//RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Decrypt Rx Mgnt Frame\n"));
			micOK = SecMFPDecodeAESCCM_1W(
					&Adapter->MgntInfo.SecurityInfo,
					(u4Byte *)Adapter->MgntInfo.SecurityInfo.AESKeyBuf[0],   // 802.11 MFP used Pawise key to encrypt !!
					pRfd->Buffer.VirtualAddress,
					IVOffset,
					pRfd->PacketLength
					);
			if(!micOK)
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("[WARNING] Decryption failed, keyIdx = %d\n", keyidx));
			}
		}
		else
		{
			micOK = SecDecodeAESCCM(
				&Adapter->MgntInfo.SecurityInfo,
				(u4Byte *)Adapter->MgntInfo.SecurityInfo.AESKeyBuf[keyidx],
				pRfd->Buffer.VirtualAddress,
				IVOffset,
				pRfd->PacketLength
				);
		}
	
			
	}
	else if( ACTING_AS_AP(Adapter) )
	{
		pu1Byte	pTA;
		OCTET_STRING	pduOS; 

		FillOctetString(pduOS, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
		pTA = Frame_Addr2(pduOS);
		
		pEntry = AsocEntry_GetEntry(pMgntInfo, pTA);		
		if(pEntry == NULL)			
			return FALSE;

		//Check if set PTK was completed		
		if(pEntry->perSTAKeyInfo.AESKeyBuf[0] == 0)									
			return FALSE;
		
		micOK = SecDecodeAESCCM(
				&Adapter->MgntInfo.SecurityInfo,
				(u4Byte *)pEntry->perSTAKeyInfo.AESKeyBuf,
				pRfd->Buffer.VirtualAddress,
				IVOffset,
				pRfd->PacketLength
				);
	}
	else
	{
		OCTET_STRING	pduOS; 
		pu1Byte	pRA;
		pu1Byte	pTA;
		u4Byte    index;

		FillOctetString(pduOS, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
		pRA = Frame_Addr1(pduOS);
		pTA = Frame_Addr2(pduOS);

		RT_TRACE(COMP_RSNA , DBG_LOUD , (" =====> RSNA IBSS Recive Encrypt Packet\n"));
		keyidx = ((pRfd->Buffer.VirtualAddress[IVKeyIdOffset]>>6)&0x03);
		RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, "RSNA RA : ", pRA);
		RT_PRINT_ADDR(COMP_RSNA, DBG_LOUD, "RSNA TA : ", pTA);
		RT_TRACE(COMP_RSNA , DBG_LOUD , (" KEY ID : %02x \n" ,keyidx ));
		if(MacAddr_isMulticast(pRA))
		{
			PPER_STA_DEFAULT_KEY_ENTRY         pDefKey;         // Add by CCW

			//Multicast packet
			//Find out Default KEY Entry  
			pDefKey = pSec->PerStaDefKeyTable;
			for( index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++, pDefKey++)
			{
				if( pDefKey->Valid && (PlatformCompareMemory(pDefKey->MACAdrss , pTA, 6)==0))
					break;
			}
			if(index == MAX_NUM_PER_STA_KEY)
			{ // no key found!
				RT_PRINT_ADDR(COMP_RSNA, DBG_WARNING, ("SecSWAESDecryption(): No group key found for pTA: \n"), pTA);
				return FALSE;
			}

			RT_PRINT_DATA(COMP_RSNA, DBG_LOUD, "Multicase Per-station key :  ", pDefKey->DefKeyBuf[keyidx], 16);
			RT_TRACE(COMP_RSNA , DBG_LOUD , (" KEY Valid : %x \n" ,pDefKey->Valid ));
			micOK = SecDecodeAESCCM(
					&Adapter->MgntInfo.SecurityInfo,
					(u4Byte *)pDefKey->DefKeyBuf[keyidx],
					pRfd->Buffer.VirtualAddress,
					IVOffset,
					pRfd->PacketLength
					);
			//micOK = TRUE;
		}
		else
		{
			PPER_STA_MPAKEY_ENTRY pMAPkey; // Add by CCW

			//unicast packet
			//Find out MAPPING Key Entry.
			pMAPkey = pSec->MAPKEYTable;
			for( index = 0 ; index < MAX_NUM_PER_STA_KEY; index++, pMAPkey++)
			{
				if( pMAPkey->Valid && (PlatformCompareMemory(pMAPkey->MACAdrss , pTA, 6)==0))
					break;
			}

 			if(index != MAX_NUM_PER_STA_KEY)
			{
				RT_PRINT_DATA(COMP_RSNA, DBG_LOUD, "Unicase Key-Mapping key :  ", pMAPkey->MapKeyBuf, 16);

				micOK = SecDecodeAESCCM(
						&Adapter->MgntInfo.SecurityInfo,
						(u4Byte *)pMAPkey->MapKeyBuf,
						pRfd->Buffer.VirtualAddress,
						IVOffset,
						pRfd->PacketLength
						);
			}
			else
			{ // no key found!
				RT_PRINT_ADDR(COMP_RSNA, DBG_WARNING, ("SecSWAESDecryption(): No pairwise key found for pTA: \n"), pTA);
				return FALSE;
			}


		}	
		RT_TRACE(COMP_RSNA, DBG_LOUD, (" micOK : %x\n", micOK));
	}
	
	return micOK?TRUE:FALSE;
}


BOOLEAN
SecSWTKIPDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	u1Byte					keyidx;
	u1Byte					SrcAddr[6];
	PRT_SECURITY_T			pSec = &Adapter->MgntInfo.SecurityInfo;
	u2Byte					u2IV16 = (u2Byte)((pSec->TxIV-1)& UINT64_C(0x000000000000ffff));
	u4Byte					u4IV32 = (u4Byte)((((pSec->TxIV-1)& UINT64_C(0x0000ffffffff0000)) >> 16)& UINT64_C(0x00000000ffffffff));
	u1Byte					key[16];
	pu1Byte					data;
	u4Byte					datalen;
	PMGNT_INFO				pMgntInfo=&Adapter->MgntInfo;
	PRT_WLAN_STA			pEntry = NULL;// = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);
	u1Byte					IVOffset;		// Added by Annie, 2005-12-23.
	u1Byte					IVKeyIdOffset;
	pu1Byte					pHeader = pRfd->Buffer.VirtualAddress;

	// Get offset. Annie, 2005-12-23.
	if( pRfd->Status.bIsQosData )
	{
		IVOffset = sMacHdrLng + sQoSCtlLng;
	}
	else
	{
		IVOffset = sMacHdrLng;
	}
	if(pRfd->Status.bContainHTC)
		IVOffset += sHTCLng;
	IVKeyIdOffset = IVOffset + KEYID_POS_IN_IV;

	// Get Key Index.
	keyidx = SecGetRxKeyIdx(Adapter, pHeader+4, ((pHeader[IVKeyIdOffset]>>6)&0x03) );	

	PlatformMoveMemory(SrcAddr, pHeader+10, ETHERNET_ADDRESS_LENGTH);

	if(ACTING_AS_AP(Adapter)) 
	{
		if(MacAddr_isMulticast(SrcAddr))
		{
			// [AnnieNote] Current GTK is 12345678...12345678, so it's OK.
			// It's better to modify it by GTK length if we use PRF-X to generate GTK later. 2005-12-23.
			//

			//Check if set GTK was completed	
			if(Adapter->MgntInfo.globalKeyInfo.GTK[0] == 0)
			{
				//RT_TRACE( COMP_AP, DBG_LOUD, ("SecSWTKIPDecryption(): GTK[0] == 0\n") );
				return FALSE;
			}
		}
		else
		{
			//Check if STA within associated lsit table					
			pEntry = AsocEntry_GetEntry(pMgntInfo, SrcAddr);		
			if(pEntry == NULL)					
				return FALSE;		
			
			//Check if set PTK was completed		
			if(pEntry->perSTAKeyInfo.TempEncKey == NULL)									
				return FALSE;
		}
	}
	else if(pRfd->bTDLPacket)
	{
			//Check if STA within associated lsit table					
			pEntry = AsocEntry_GetEntry(pMgntInfo, SrcAddr);		
			if(pEntry == NULL)					
				return FALSE;		
			
			//Check if set PTK was completed		
			if(pEntry->perSTAKeyInfo.TempEncKey == NULL)									
				return FALSE;
		}


	// Get u2IV16 and u4IV32.
	u2IV16 = ((u2Byte)(*(pHeader+IVOffset + 0)) <<  8)+		\
			((u2Byte)(*(pHeader+IVOffset + 2)) <<  0);
	
	u4IV32 = ((u4Byte) (*(pHeader+IVOffset + 4)) <<  0)+		\
			((u4Byte) (*(pHeader+IVOffset + 5)) <<  8)+		\
			((u4Byte) (*(pHeader+IVOffset + 6)) << 16)+		\
			((u4Byte) (*(pHeader+IVOffset + 7)) << 24);

#if 1 //Added by Jay 0712 for security IV  
	if( ACTING_AS_AP(Adapter) )	// Added by Annie. We should not enter here when STA mode. 2005-07-21.
	{
		u8Byte tempIV = 0;

		tempIV |= ((u8Byte)u4IV32) << 16;
		tempIV |= (u8Byte)u2IV16;

		// Prefast warning C6011: Dereferencing NULL pointer 'pEntry'
		if(pEntry != NULL &&
			(tempIV - pEntry->perSTAKeyInfo.RxIV) >= 1000000)
		{
			//RT_TRACE(COMP_SEC, DBG_LOUD, ("Error: tempIV = 0x%16"i64fmt"x\n", tempIV));
			//RT_TRACE(COMP_SEC, DBG_LOUD, ("Error: pEntry->perSTAKeyInfo.RxIV = 0x%16"i64fmt"x\n", pEntry->perSTAKeyInfo.RxIV));

			//Process IV Error...
		} 
		else
		{
			// Prefast warning C6011: Dereferencing NULL pointer 'pEntry'.
			if (pEntry != NULL)
			{
				if (tempIV - pEntry->perSTAKeyInfo.RxIV != 1)
				{
					//RT_TRACE(COMP_SEC, DBG_LOUD, ("minor issue: tempIV = 0x%16"i64fmt"x\n", tempIV));
					//RT_TRACE(COMP_SEC, DBG_LOUD, ("minor issue: pEntry->perSTAKeyInfo.RxIV = 0x%16"i64fmt"x\n", pEntry->perSTAKeyInfo.RxIV));
				}

				pEntry->perSTAKeyInfo.RxIV = tempIV;
				pEntry->perSTAKeyInfo.RxIV &= UINT64_C(0x0000ffffffffffff);
			}
		}
	}
	else if(pRfd->bTDLPacket)
	{
		u8Byte tempIV = 0;

		tempIV |= ((u8Byte)u4IV32) << 16;
		tempIV |= (u8Byte)u2IV16;

		// Prefast warning C6011: Dereferencing NULL pointer 'pEntry'.
		if (pEntry != NULL)
		{
			if (tempIV - pEntry->perSTAKeyInfo.RxIV >= 1000000)
			{
				//RT_TRACE(COMP_SEC, DBG_LOUD, ("Error: tempIV = 0x%16"i64fmt"x\n", tempIV));
				//RT_TRACE(COMP_SEC, DBG_LOUD, ("Error: pEntry->perSTAKeyInfo.RxIV = 0x%16"i64fmt"x\n", pEntry->perSTAKeyInfo.RxIV));

				//Process IV Error...
			}
			else
			{
				if (tempIV - pEntry->perSTAKeyInfo.RxIV != 1)
				{
					//RT_TRACE(COMP_SEC, DBG_LOUD, ("minor issue: tempIV = 0x%16"i64fmt"x\n", tempIV));
					//RT_TRACE(COMP_SEC, DBG_LOUD, ("minor issue: pEntry->perSTAKeyInfo.RxIV = 0x%16"i64fmt"x\n", pEntry->perSTAKeyInfo.RxIV));
				}

				pEntry->perSTAKeyInfo.RxIV = tempIV;
				pEntry->perSTAKeyInfo.RxIV &= UINT64_C(0x0000ffffffffffff);
			}
		}
	}
#endif	


	if(ACTING_AS_AP(Adapter)) 
	{ // AP Mode.
		if(MacAddr_isMulticast(SrcAddr))
		{
			TKIPGenerateKey(key, u4IV32, u2IV16, SrcAddr, &Adapter->MgntInfo.globalKeyInfo.GTK[0]);
		}
		else
		{
			if(pEntry != NULL)
				TKIPGenerateKey(key, u4IV32, u2IV16, SrcAddr, pEntry->perSTAKeyInfo.TempEncKey);
		}
	} 
	else if(pRfd->bTDLPacket)
	{
		// Prefast warning C6011: Dereferencing NULL pointer 'pEntry'
		if(pEntry != NULL)
			TKIPGenerateKey(key, u4IV32, u2IV16, SrcAddr, pEntry->perSTAKeyInfo.TempEncKey);
	}
	else
	{ // STA Mode.
			TKIPGenerateKey(key,u4IV32,u2IV16,SrcAddr,pSec->KeyBuf[keyidx]);
	}


	data = pHeader + IVOffset + pSec->EncryptionHeadOverhead;
	datalen = pRfd->PacketLength - (IVOffset + pSec->EncryptionHeadOverhead);
	//PRINT_DATA((const void*)"Decode: Cichpertext==>", data, datalen);
	if(!DecodeWEP(key,16,data,datalen,data))
	{
//		PRINT_DATA((const void*)"Decode: plaintext==>", data, datalen);
		RT_TRACE(COMP_SEC, DBG_LOUD,("Rx:TKIP Error\n"));

		if( ACTING_AS_AP(Adapter) )
		{
			if (pEntry != NULL)
			{
				pEntry->perSTAKeyInfo.WEPErrorCnt++;
				RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD, ("Rx:TKIP Error: %d\n", pEntry->perSTAKeyInfo.WEPErrorCnt));
			}
		}
		else if(pRfd->bTDLPacket)
		{
			if (pEntry != NULL)
			{
				pEntry->perSTAKeyInfo.WEPErrorCnt++;
				RT_TRACE(COMP_AUTHENTICATOR, DBG_LOUD, ("Rx:TKIP Error: %d\n", pEntry->perSTAKeyInfo.WEPErrorCnt));
			}
		}

		return FALSE;
	}
	RT_TRACE(COMP_SEC, DBG_LOUD,("Rx:TKIP OK\n"));
	return TRUE;
	
}
BOOLEAN
SecSWWEPDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	PRT_SECURITY_T			pSec = &Adapter->MgntInfo.SecurityInfo;
	u1Byte					keyidx;
	u1Byte					key[16];
	s4Byte					keysize;
	pu1Byte					data;
	u4Byte					datalen;
	u1Byte					IVOffset;	// Added by Annie, 2005-12-23.
	pu1Byte					pHeader = pRfd->Buffer.VirtualAddress;

	//Fix SW WEP descryption multicase fail. 2009,12,08
	
	u2Byte					EncryptionMPDUHeadOverhead, EncryptionMPDUTailOverhead;
	u2Byte					EncryptionMSDUHeadOverhead, EncryptionMSDUTailOverhead;

	pu1Byte					DestAddr;
	OCTET_STRING			Mpdu;
	BOOLEAN					bMulticastDest;
	RT_ENC_ALG				TempALg;

	FillOctetString(Mpdu, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	DestAddr = Frame_pDaddr(Mpdu);
	bMulticastDest = MacAddr_isMulticast(DestAddr);

	if( pSec->SecLvl == RT_SEC_LVL_NONE  )
		bMulticastDest = FALSE;
	
	SecGetEncryptionOverhead(
		Adapter,
		&EncryptionMPDUHeadOverhead, 
		&EncryptionMPDUTailOverhead, 
		&EncryptionMSDUHeadOverhead, 
		&EncryptionMSDUTailOverhead,
		TRUE,
		bMulticastDest
		 );

	if( !bMulticastDest )
		TempALg =  pSec->PairwiseEncAlgorithm;
	else
		TempALg = pSec->GroupEncAlgorithm; 

	// Get offset. Annie, 2005-12-23.
	if( pRfd->Status.bIsQosData )
	{
		IVOffset = sMacHdrLng + sQoSCtlLng;
	}
	else
	{
		IVOffset = sMacHdrLng;
	}
	if(pRfd->Status.bContainHTC)
		IVOffset += sHTCLng;

	
	// Get IV. (Because 802.11 and key both are little endian)
	PlatformMoveMemory(key, pHeader+IVOffset , 3);

	// Prepare Key.
	keyidx = GET_WEP_IV_KEYID(pHeader+IVOffset); 
	keysize = (TempALg==RT_ENC_ALG_WEP104) ? 16 : 8; 
	PlatformMoveMemory(key+3, pSec->KeyBuf[keyidx], keysize-3);

	// Set up input data.
	data = pHeader + IVOffset + 4;//pSec->EncryptionHeadOverhead;
	datalen = pRfd->PacketLength - (IVOffset +4);// pSec->EncryptionHeadOverhead);

	//RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "Decode: Ciphertext==>", data, datalen);
	if( !DecodeWEP( key, keysize, data, datalen, data ) )
	{
		RT_TRACE( COMP_SEC, DBG_LOUD, ("SecSWWEPDecryption(): WEP software decrypted Fail!!! (datalen=%d)\n", datalen) );
		RT_PRINT_DATA( COMP_SEC, DBG_TRACE, "WEP decoded packet", data, datalen );
		return FALSE;
	}

	return TRUE;
}

//
// Software decryption for CKIP.
// Ported from CKIP code, Annie, 2006-08-11. [AnnieTODO]
//
BOOLEAN
SecSWCKIPDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	)
{
	pu1Byte		pHeader = pRfd->Buffer.VirtualAddress;
	u1Byte	       keyidx;
	u1Byte		IVOffset;
	int                DEcok = 0;
	PRT_SECURITY_T			pSec = &Adapter->MgntInfo.SecurityInfo;

	// Get offset. Annie, 2005-12-23.
	if( pRfd->Status.bIsQosData )
	{
		IVOffset = sMacHdrLng + sQoSCtlLng;
	}
	else
	{
		IVOffset = sMacHdrLng;
	}
	if(pRfd->Status.bContainHTC)
		IVOffset += sHTCLng;
	
	//Get IV 
	keyidx= GET_WEP_IV_KEYID( pHeader+IVOffset);
	
	DEcok = DecodeCKIP( Adapter , (UCHAR *)pHeader , pRfd->PacketLength , pSec->pCkipPara->CKIPKeyBuf[keyidx] , keyidx );
	RT_TRACE( COMP_CKIP, DBG_LOUD, ("SecSWCKIPDecryption(): DEcok=0x%X\n", DEcok) );

	return	( (DEcok==1)? TRUE : FALSE );
}


//
//	Desription:
//		Used for deny link to MIC failure AP in 60 seconds.
//	2004.10.06, by rcnjo.
//
BOOLEAN 
SecInsertDenyBssidList (
 	PRT_SECURITY_T	pSec,
	u1Byte			BssidToDeny[6], 
	u8Byte			DenyStartTime
)
{
	PRT_DENY_BSSID	pBssidToDeny = NULL;
	u8Byte			EarlyestStartTime = PlatformGetCurrentTime();
	int 			EarlyestIdx = 0;
	int				i;

	for(i = 0; i < MAX_DENY_BSSID_LIST_CNT; i++)
	{
		if(pSec->DenyBssidList[i].bUsed)
		{
			if(pSec->DenyBssidList[i].StartTime < EarlyestStartTime)
			{
				EarlyestStartTime = pSec->DenyBssidList[i].StartTime;
				EarlyestIdx = i;
			}
		}
		else
		{
			pBssidToDeny = pSec->DenyBssidList + i;
			break;
		}
	}

	if(pBssidToDeny == NULL)
	{
		// Use the oldest one.
		pBssidToDeny = pSec->DenyBssidList + EarlyestIdx; 
	}

	PlatformMoveMemory(pBssidToDeny->Bssid, BssidToDeny, 6);
	pBssidToDeny->StartTime = DenyStartTime;
	pBssidToDeny->bUsed = TRUE;

	return TRUE;
}

//
//	Desription:
//		Used for deny link to MIC failure AP in 60 seconds.
//	2004.10.06, by rcnjo.
//
BOOLEAN 
SecIsInDenyBssidList (
 	PRT_SECURITY_T	pSec,
	u1Byte			BssidToCheck[6]
)
{
	BOOLEAN			bToDeny = FALSE;
	PRT_DENY_BSSID	pBssidToDeny;
	u8Byte			CurrTime = PlatformGetCurrentTime();		
	int				i;

	for(i = 0; i < MAX_DENY_BSSID_LIST_CNT; i++)
	{
		pBssidToDeny = pSec->DenyBssidList + i; 
		if(pBssidToDeny->bUsed)
		{
			// Check StartTime.
			if((CurrTime - pBssidToDeny->StartTime) < MIC_CHECK_TIME) // Diff < 60 seconds.
			{ 
				// Check BSSID.
				if( PlatformCompareMemory(pBssidToDeny->Bssid, BssidToCheck, 6) == 0 )
				{ // The same BSSID.
					bToDeny = TRUE;
					break;
				}
			}
			else
			{
				// Clear the entry if it is overdue.
				pBssidToDeny->bUsed = FALSE;
				continue;
			}
		}
	}

	return bToDeny;
}

//
//	Desription:
//		Fill up WEP bit and IV of each fragment in the TCB if necessary.	
//	Output:
//		1. Return TRUE if this packet has to be encrypted.
//		2. Set up pTcb->EncInfo.
//	2005.06.27, by rcnjo.
//
BOOLEAN 
SecFillHeader(
	PADAPTER	Adapter,
	PRT_TCB		pTcb)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	int			i;
	int				FragIndex = 0;
	int				FragBufferIndex = 0;
	BOOLEAN			bEncrypt;
	PSTA_ENC_INFO_T	pEncInfo = &pTcb->EncInfo;

	bEncrypt = MgntGetEncryptionInfo( Adapter, pTcb, pEncInfo,TRUE);

	if(WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_NOTSETENCMACHEADER, 0))
		return FALSE;
	
	if(!bEncrypt)
	{
		RT_TRACE( COMP_CKIP, DBG_TRACE, ("SecFillHeader(): !bEncrypt => return FALSE!\n"));
		return FALSE;
	}

	for(i = 0;i < pTcb->BufferCount; i++)
	{
		// Fill WEP bit, IV.
		if(FragBufferIndex == 0)
		{
			pu1Byte pFrame;

			pFrame = (pu1Byte)&pTcb->BufferList[i].VirtualAddress[Adapter->TXPacketShiftBytes];

			// WEP bit. 
			SET_80211_HDR_WEP(pFrame, 1);

			// IV. 
			SecHeaderFillIV(Adapter, pFrame);
		}

		FragBufferIndex++;
		if(FragBufferIndex == pTcb->FragBufCount[FragIndex])
		{
			FragIndex++;
			FragBufferIndex = 0;
		}
	}

	RT_TRACE( COMP_CKIP, DBG_TRACE, ("SecFillHeader(): retrun TRUE\n"));
	return TRUE;
}



//
// Return: Is the content of pPduOS an EAPOL frame or not. (88-8e)
// Added by Annie, 2005-12-25.
//
BOOLEAN
SecIsEAPOLPacket(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pPduOS
	)
{
	BOOLEAN		IsEAPOLPkt = FALSE;
	u1Byte		Offset_TypeEAPOL = sMacHdrLng + LLC_HEADER_SIZE;	// 30

	if( IsQoSDataFrame(pPduOS->Octet) )
	{
		Offset_TypeEAPOL += sQoSCtlLng;	// +2
	}

	if( Frame_WEP(*pPduOS))
	{
		Offset_TypeEAPOL += Adapter->MgntInfo.SecurityInfo.EncryptionHeadOverhead;	// 4 or 8
	}

	// Length Check
	if( pPduOS->Length < (Offset_TypeEAPOL+1) )
	{
		RT_TRACE( COMP_SEC, DBG_TRACE, ("SecIsEAPOLPacket(): invalid length(%d)\n", pPduOS->Length ) );
		return FALSE;
	}

	// 888e?
	if( (pPduOS->Octet[Offset_TypeEAPOL]==0x88) && (pPduOS->Octet[Offset_TypeEAPOL+1]==0x8e) )
	{
		IsEAPOLPkt = TRUE;
	}

	//RT_TRACE( COMP_SEC, DBG_LOUD, ("SecIsEAPOLPacket(): Recvd EAPOL frame. IsEAPOLPkt(%d)\n", IsEAPOLPkt ));
	return	IsEAPOLPkt;
}

//
// Return: Is the content of pPduOS an EAPOL frame or not. (88-8e)
// Added by Annie, 2005-12-25.
//
BOOLEAN
SecIsEAPOLKEYPacket(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pPduOS
	)
{
	BOOLEAN		IsEAPOLKeyPkt = FALSE;
	u1Byte		Offset_TypeEAPOL = sMacHdrLng + LLC_HEADER_SIZE;	// 30

	if( IsQoSDataFrame(pPduOS->Octet) )
	{
		Offset_TypeEAPOL += sQoSCtlLng;	// +2
	}

	if( Frame_WEP(*pPduOS))
	{
		Offset_TypeEAPOL += Adapter->MgntInfo.SecurityInfo.EncryptionHeadOverhead;	// 4 or 8
	}

	// Length Check
	if( pPduOS->Length < (Offset_TypeEAPOL+1) )
	{
		RT_TRACE( COMP_SEC, DBG_TRACE, ("SecIsEAPOLKEYPacket(): invalid length(%d)\n", pPduOS->Length ) );
		return FALSE;
	}

	// 888e && Type is Key
	if( (pPduOS->Octet[Offset_TypeEAPOL]==0x88) && (pPduOS->Octet[Offset_TypeEAPOL+1]==0x8e) 
		&& (pPduOS->Octet[Offset_TypeEAPOL+3]==0x03) )
	{
		IsEAPOLKeyPkt = TRUE;
	}

	RT_TRACE( COMP_SEC, DBG_TRACE, ("SecIsEAPOLKEYPacket(): Recvd EAPOL frame. IsEAPOLPkt(%d)\n", IsEAPOLKeyPkt ));
	//Here is for Verify 2008/03/20
	//RT_TRACE( COMP_SEC, DBG_LOUD, ("==m==>SecIsEAPOLKEYPacket(): Recvd EAPOL frame. IsEAPOLPkt(%d)<==m==\n", IsEAPOLKeyPkt ));
	return	IsEAPOLKeyPkt;
}


//
// Ported from 8185: WMacSetPMKID().
// Added by Annie, 2006-05-07.
//
VOID
SecSetPMKID(
	IN	PADAPTER				Adapter,
	IN	PN5_802_11_PMKID		pPMKid
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	PN5_BSSID_INFO			pBssidInfo = (PN5_BSSID_INFO)(pPMKid->BSSIDInfo);
	u4Byte				ulIndex, i, j;
	BOOLEAN				blInserted = FALSE;
	u1Byte				NullMacadress[6] = {0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00  };

	RT_TRACE( COMP_SEC, DBG_LOUD, ("SecSetPMKID(): Number Of PMKID on list = %d\n", pPMKid->BSSIDInfoCount) );


	//
	//	Clear all entry , when PMK-count = 1 and MAC address all zero !!
	//
	if( 	(pPMKid->BSSIDInfoCount == 1 )&&
		(PlatformCompareMemory(pBssidInfo->BSSID , NullMacadress , 6 )==0))
	{
		for( ulIndex=0; ulIndex<NUM_PMKID_CACHE; ulIndex++ )
		{ // Clear all PMK cash !!
			pSecInfo->PMKIDList[ulIndex].bUsed = FALSE;
			PlatformZeroMemory(pSecInfo->PMKIDList[ulIndex].Bssid, sizeof(pSecInfo->PMKIDList[ulIndex].Bssid));
			pSecInfo->PMKIDList[ulIndex].Ssid.Length = 0;
		
		}
		return;
	}

	//
	// Clear the entry with different SSID from the AP we are associating with.
	//
	for( ulIndex=0; ulIndex<NUM_PMKID_CACHE; ulIndex++ )
	{
		if(	!eqNByte(pMgntInfo->Ssid.Octet, pSecInfo->PMKIDList[ulIndex].SsidBuf, pMgntInfo->Ssid.Length) ||
			(pMgntInfo->Ssid.Length !=  pSecInfo->PMKIDList[ulIndex].Ssid.Length) )
		{ // SSID is not matched => Clear the entry!
			pSecInfo->PMKIDList[ulIndex].bUsed = FALSE;
			PlatformZeroMemory(pSecInfo->PMKIDList[ulIndex].Bssid, sizeof(pSecInfo->PMKIDList[ulIndex].Bssid));
			pSecInfo->PMKIDList[ulIndex].Ssid.Length = 0;
		}
	}


	//
	// Insert or cover with new PMKID.
	//
	for( i=0; i<pPMKid->BSSIDInfoCount; i++ )
	{
		if( i >= NUM_PMKID_CACHE )
		{
			RT_TRACE( COMP_SEC, DBG_WARNING, ("SecSetPMKID(): BSSIDInfoCount is more than NUM_PMKID_CACHE!%d\n", pPMKid->BSSIDInfoCount) );
			break;
		}

		blInserted = FALSE;
		for(j=0 ; j<NUM_PMKID_CACHE; j++)
		{
			if( pSecInfo->PMKIDList[j].bUsed && eqMacAddr(pSecInfo->PMKIDList[j].Bssid, pBssidInfo->BSSID) )
			{ // BSSID is matched, the same AP => rewrite with new PMKID.
				CopyMem(pSecInfo->PMKIDList[j].PMKID, pBssidInfo->PMKID, sizeof(pBssidInfo->PMKID));
				blInserted = TRUE;
				break;
			}
		}

		if(blInserted)
		{
			pBssidInfo ++;	// pointer to next BSSID_INFO.
			continue;
		}
		else 
		{
			// Find a new entry
			for( j=0 ; j<NUM_PMKID_CACHE; j++ )
			{
				if(pSecInfo->PMKIDList[j].bUsed == FALSE)
				{
					pSecInfo->PMKIDList[j].bUsed = TRUE;
					CopyMem(pSecInfo->PMKIDList[j].Bssid, pBssidInfo->BSSID, 6);
					CopyMem(pSecInfo->PMKIDList[j].PMKID, pBssidInfo->PMKID, PMKID_LEN);
					CopySsid(pSecInfo->PMKIDList[j].SsidBuf, pSecInfo->PMKIDList[j].Ssid.Length, pMgntInfo->SsidBuf, pMgntInfo->Ssid.Length);

					break;
				}
			}
		}

		pBssidInfo ++;	// pointer to next BSSID_INFO.
	}


	//
	// For debug purpose: Check current PMKID list.
	//
	for( ulIndex=0; ulIndex<NUM_PMKID_CACHE ; ulIndex++ )
	{
		if(pSecInfo->PMKIDList[ulIndex].bUsed)
		{
			RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
			RT_TRACE( COMP_SEC, DBG_LOUD, ("[PMKID %d]\n", ulIndex ) );
			RT_TRACE( COMP_SEC, DBG_LOUD, ("------------------------------------------\n") );
			RT_TRACE( COMP_SEC, DBG_LOUD, ("SecSetPMKID(): PMKIDList[%d].bUsed is TRUE\n", ulIndex) );
			RT_PRINT_STR( COMP_SEC, DBG_LOUD, "SSID", pSecInfo->PMKIDList[ulIndex].Ssid.Octet, pSecInfo->PMKIDList[ulIndex].Ssid.Length);
			RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "BSSID", pSecInfo->PMKIDList[ulIndex].Bssid, 6);
			RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "PMKID", pSecInfo->PMKIDList[ulIndex].PMKID, sizeof(pBssidInfo->PMKID));
		}
	}			
}



//
// Ported from 8185: IsInPreAuthKeyList(). (Renamed from SecIsInPreAuthKeyList(), 2006-10-13.)
// Added by Annie, 2006-05-07.
//
// Search by BSSID,
// Return Value:
//		-1 		:if there is no pre-auth key in the  table
//		>=0		:if there is pre-auth key, and   return the entry id
//
int
SecIsInPMKIDList(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		bssid
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);
	int			i;

	for( i=0; i<NUM_PMKID_CACHE; i++ )
	{
		if( pSecInfo->PMKIDList[i].bUsed && eqMacAddr(pSecInfo->PMKIDList[i].Bssid, bssid) )
		{
			break;
		}
		else
		{
			continue;
		}
	}

	if( i == NUM_PMKID_CACHE )
	{ // Could not find.
		i = -1;
	}
	else
	{ // There is one Pre-Authentication Key for the specific BSSID.
	}

	RT_TRACE( COMP_SEC, DBG_LOUD, ("SecIsInPMKIDList(): i=%d\n", i) );
	return i;
}


//
// Ported from 8185: WMacCatPMKID().
// Added by Annie, 2006-05-07.
//
VOID
SecCatPMKID(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			CurAPbssid,
	IN	PRT_WLAN_BSS	pBssDesc
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);
	pu1Byte			pIECurrent = NULL;
	int 				iEntry = 0;
	BOOLEAN			bCCX8021xenable = FALSE;
	BOOLEAN			bAPSuportCCKM = FALSE;
	
	u1Byte			BIPOui[4] = {0x00, 0x0f , 0xac , 0x06};
	
	// Note: PMKID can only be included in (Re)Association.
	if( ACTING_AS_AP(Adapter) || pMgntInfo->mIbss )
	{
		return;
	}

	// Note: PMKID fileds are defined in WPA2.
	if( pSecInfo->SecLvl < RT_SEC_LVL_WPA2 )
		return;

	if(WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_WAPISUPPORT, 0) && pSecInfo->SecLvl == RT_SEC_LVL_WAPI)
		return;
		
	//
	// CCKM mode  return.
	// Note:
	//	Putting PMKID and CCKM together in the reassociation request must confuse the AP and 
	//	make it no response after reassociation request packet.
	//
	CCX_QueryCCKMSupport(Adapter, &bCCX8021xenable, &bAPSuportCCKM);
	
	if( bCCX8021xenable && bAPSuportCCKM)
	{
		return;
	}

	//
	// 1. Search PMKID list (pSecInfo->PMKIDList[i]) by BSSID.
	//
	iEntry = SecIsInPMKIDList( Adapter, CurAPbssid );
	if( iEntry < 0 )
	{
		RT_TRACE( COMP_SEC, DBG_LOUD, ("SecCatPMKID(): AP is not in current PMKID List => do nothing.\n") );

		// Check Need to add MFP IE or not !! 
		if( pBssDesc->bMFPC && pMgntInfo->bInBIPMFPMode)
		{
			pIECurrent = pSecInfo->RSNIE.Octet + pSecInfo->RSNIE.Length - 2; // Point to RSN Capablite
			SET_RSN_CAP_MFP_CAPABLE(pIECurrent , 1 );
			
			if( pBssDesc->bMFPR )
			{
				// Removed by Bruce, 2014-12-08.
				// From MSDN:
				//	Management Frame Protection Required (MFPR) enforcement on Windows 8 is not supported. Hence miniport drivers should never set
				//	this bit in the RSN capabilities of RSN IE during an association request. For policy, the access point may advertise MFPR which will
				//	allow MFP-capable STA to associate. Access points not supporting MFP capability will fail association. If MFPR is set by an access point
				//	and STA is not MFP capable, Windows 8 will treat the network as mismatched in capability and not send an association request to the
				//	miniport. http://msdn.microsoft.com/en-us/library/windows/hardware/ff547688(v=vs.85).aspx
				// 
				// SET_RSN_IE_CAP_MFP_REQUIRED(pIECurrent, 1);
			}
			pIECurrent += 2;
			
			if( pBssDesc->bMFPBIP )
			{
				// Add PMKID counter = 0x00 0x00
				PlatformZeroMemory( pIECurrent , 2 );
				pIECurrent += 2;

				// Add BIP oui
				PlatformMoveMemory( pIECurrent ,BIPOui , 4 );

				pSecInfo->RSNIE.Length += 6;
			}
		}

		return;
	}

	//
	// 2. Cat PMKID to RSNIE.
	//
	pIECurrent = pSecInfo->RSNIE.Octet + pSecInfo->RSNIE.Length;
	((PDOT11_RSN_IE_PMKID)pIECurrent)->SuiteCount = NUM_CAT_PMKID;	// NUM_CAT_PMKID=1
	CopyMem(	((PDOT11_RSN_IE_PMKID)pIECurrent)->PMKList, 
				pSecInfo->PMKIDList[iEntry].PMKID,
				sizeof(pSecInfo->PMKIDList[iEntry].PMKID)
				);
	pSecInfo->RSNIE.Length += ( 	sizeof( ((PDOT11_RSN_IE_PMKID)pIECurrent)->SuiteCount ) +	// 2 bytes for PMKID count.
								PMKID_LEN*NUM_CAT_PMKID );								// 16*s bytes for PMKID List.

	//
	// Note : 802.11w sample RSN PMK and BIP support at same time !!
	//

	// Check Need to add MFP IE or not !! 
	if( pBssDesc->bMFPC && pMgntInfo->bInBIPMFPMode )
	{
		pIECurrent = pIECurrent - 2; // Point to RSN Capablite
		SET_RSN_IE_CAP_MFP_CAPABLE(pIECurrent , 1 );

		if( pBssDesc->bMFPR )
		{
			SET_RSN_CAP_MFP_REQUIRED(pIECurrent, 1);
		}
		
		pIECurrent = pSecInfo->RSNIE.Octet + pSecInfo->RSNIE.Length;

		if( pBssDesc->bMFPBIP )
		{
			// Add BIP oui
			PlatformMoveMemory( pIECurrent ,BIPOui , 4 );

			pSecInfo->RSNIE.Length += 4;
		}
	}

	RT_PRINT_DATA( COMP_SEC, DBG_LOUD, "SecCatPMKID(): New RSNIE", pSecInfo->RSNIE.Octet, pSecInfo->RSNIE.Length );
}


//
// If there is no key when security enabled, drop data frames.
// Added by Annie, 2006-08-15.
//
BOOLEAN 
SecDropForKeyAbsent(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);
	BOOLEAN			bDrop = FALSE;
	pu1Byte			pRa = NULL;
	BOOLEAN             bFindMatchPeer = FALSE;

	RT_TRACE( COMP_SEC, DBG_TRACE, ("==> SecDropForKeyAbsent()\n") );

  	GetTcbDestaddr(Adapter, pTcb, &pRa);

	//<TODO: How about in AP mode>
	if(ACTING_AS_AP(Adapter))
	{
		return bDrop;
	}

	if ( pSecInfo->SecLvl == RT_SEC_LVL_WAPI)
	{
		if(RT_STATUS_SUCCESS == WAPI_SecFuncHandler(WAPI_DROPFORSECKEYABSENT,Adapter, (PVOID)pRa, WAPI_END))
			bDrop = TRUE;
	}
	else
	{
		if(pMgntInfo->SafeModeEnabled || pMgntInfo->mIbss)
		{
			return bDrop;
		}
	
		if( pTcb->EncInfo.SecProtInfo == RT_SEC_NORMAL_DATA )
		{
			// Data frame, not EAPOL packet.
			if(RT_STATUS_SUCCESS == WAPI_SecFuncHandler(WAPI_DROPFORSECKEYABSENT,Adapter, (PVOID)pRa, WAPI_END))
				bDrop = TRUE;
		
			if( pSecInfo->SecLvl > RT_SEC_LVL_NONE )
			{ // WPA or WPA2.
				if( MacAddr_isMulticast(pRa) )
				{ // Need group key.
					if(pSecInfo->KeyLen[pSecInfo->GroupTransmitKeyIdx]==0)
					{
						bDrop = TRUE;
						RT_TRACE( COMP_SEC, DBG_LOUD, ("SecDropForKeyAbsent(): Tx Data Without Group Key %d\n", pSecInfo->GroupTransmitKeyIdx ) );
					}
				}
				else
				{ // Need pairwise key.
					if(pSecInfo->KeyLen[PAIRWISE_KEYIDX]==0)
					{
						bDrop = TRUE;
						RT_TRACE( COMP_SEC, DBG_LOUD, ("SecDropForKeyAbsent(): Tx Data Without Pairwise Key!\n") );
					}
				}
			}
			else
			{ // Check WEP
				if( pSecInfo->EncryptionStatus==RT802_11Encryption1Enabled )	// Is this OK?? Annie, 2006-05-04.
				{
					if(pSecInfo->KeyLen[pSecInfo->DefaultTransmitKeyIdx]==0)
					{
						bDrop = TRUE;
						RT_TRACE( COMP_SEC, DBG_LOUD, ("SecDropForKeyAbsent(): Tx Data Without WEP Key %d\n", pSecInfo->DefaultTransmitKeyIdx ) );
					}
				}
			}
		}
	}
	if(bDrop)
	{
		RT_TRACE( COMP_SEC, DBG_WARNING, ("SecDropForKeyAbsent(): Not to send data frame since there is no key!\n") );
		RT_PRINT_DATA(COMP_SEC, DBG_TRACE, "Discarded Packet", pTcb->BufferList[0].VirtualAddress, pTcb->BufferList[0].Length);
	}
	RT_TRACE( COMP_SEC, DBG_TRACE, ("<== SecDropForKeyAbsent(), bDrop=%d\n", bDrop) );

	return	bDrop;
}

VOID
SecSetSwEncryptionDecryption(
	PADAPTER			Adapter,
	BOOLEAN				bSWTxEncrypt,
	BOOLEAN				bSWRxDecrypt
	)
{
	PRT_SECURITY_T pSec = &Adapter->MgntInfo.SecurityInfo;

	// Tx Sw Encryption.
	if ( (pSec->RegSWTxEncryptFlag == EncryptionDecryptionMechanism_Auto) ||
		(pSec->RegSWTxEncryptFlag >= EncryptionDecryptionMechanism_Undefined) )
	{
		pSec->SWTxEncryptFlag = bSWTxEncrypt;
	}
	else if ( pSec->RegSWTxEncryptFlag == EncryptionDecryptionMechanism_Hw )
	{
		pSec->SWTxEncryptFlag = FALSE;
	}
	else if ( pSec->RegSWTxEncryptFlag == EncryptionDecryptionMechanism_Sw )
	{
		pSec->SWTxEncryptFlag = TRUE;
	}

	// If the encryption mechanism differs between driver determined and registry determined.
	// Print an warning message.
	if (pSec->RegSWTxEncryptFlag != bSWTxEncrypt)
	{
		RT_TRACE(COMP_SEC, DBG_WARNING, 
			("SecSetSwEncryptionDecryption(): Warning! User and driver determined encryption mechanism mismatch.\n"));
		RT_TRACE(COMP_SEC, DBG_WARNING, 
			("SecSetSwEncryptionDecryption(): RegSWTxEncryptFlag = %d, bSWTxEncrypt = %d\n", 
			pSec->RegSWTxEncryptFlag, bSWTxEncrypt));
	}

	// Rx Sw Decryption.
	if ( (pSec->RegSWRxDecryptFlag == EncryptionDecryptionMechanism_Auto) ||
		(pSec->RegSWRxDecryptFlag == EncryptionDecryptionMechanism_Undefined) )
	{
		pSec->SWRxDecryptFlag = bSWRxDecrypt;
	}
	else if ( pSec->RegSWRxDecryptFlag == EncryptionDecryptionMechanism_Hw )
	{
		pSec->SWRxDecryptFlag = FALSE;
	}
	else if ( pSec->RegSWRxDecryptFlag == EncryptionDecryptionMechanism_Sw)
	{
		pSec->SWRxDecryptFlag = TRUE;
	}

	// If the decryption mechanism differs between driver determined and registry determined.
	// Print an warning message.
	if (pSec->RegSWRxDecryptFlag != bSWRxDecrypt)
	{
		RT_TRACE(COMP_SEC, DBG_WARNING,
			("SecSetSwEncryptionDecryption(): Warning! User and driver determined decryption mechanism mismatch.\n"));
		RT_TRACE(COMP_SEC, DBG_WARNING, 
			("SecSetSwEncryptionDecryption(): RegSWRxDecryptFlag = %d, bSWRxDecrypt = %d\n",
			pSec->RegSWRxDecryptFlag, bSWRxDecrypt));
	}

}

//
//	Description:
//		Determine if the specifed key is available for RA
//
BOOLEAN
SecIsTxKeyInstalled(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			pRA
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);

	if (ACTING_AS_AP(pAdapter) && MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_LINUX )
	{// AP mode
		if(!MacAddr_isMulticast(pRA))
		{
			PRT_WLAN_STA	pEntry;
			pEntry = AsocEntry_GetEntry(pMgntInfo,pRA);
			if(pEntry == NULL)
				return FALSE;
			else if(pEntry->keyindex != 0)
				return TRUE;
			else
				return FALSE;
		}
		else
		{						
			return FALSE;
		}
	}

	if( !pMgntInfo->bRSNAPSKMode )
	{
		if(	pSecInfo->KeyLen[0] == 0 &&
			pSecInfo->KeyLen[1] == 0 &&
			pSecInfo->KeyLen[2] == 0 &&
			pSecInfo->KeyLen[3] == 0 &&
			pSecInfo->KeyLen[4] == 0 
			&& 0 == WAPI_QuerySetVariable(pAdapter, WAPI_QUERY, WAPI_VAR_ISCAMUSED, 0)
			)
		{
			return FALSE;
		}
		else
		{
			// Check Pawise Key only !!
			if(!ACTING_AS_AP(pAdapter) && pSecInfo->SecLvl > RT_SEC_LVL_NONE )
			{
				//
				// Check Tx key 
				// Note:
				//	This is special case for WPA-2 (RSN).
				//	When the CCX service (supplicant) receives the ANoce packet in the 1st packet of 4-way,
				//	the supplicant set the Group Key to driver and thus the driver then sends the SNonce packet "encrypted"
				//	to AP by an empty PTK just because the driver determine that the Keys has been installed
				//	from the procedure SecIsTxKeyInstalled().
				//	We find this behavior of CCX service in the CCX SDK version 1.1.13.
				//  Advised from CCW, and edited by Bruce, 2009-09-04.
				//
				if( pSecInfo->KeyLen[0] == 0 
					&& 0 == WAPI_QuerySetVariable(pAdapter, WAPI_QUERY, WAPI_VAR_ISCAMUSED, 0)
					)
				{
					RT_TRACE(COMP_SEC, DBG_LOUD, ("SecIsTxKeyInstalled(): pSecInfo->KeyLen[0] == 0, Tx Key is not installed!\n"));
					return FALSE;
				}
			}
			
			return TRUE;
		}
	}
	else
	{ // RSNA IBSS mode.
		u4Byte index;

		if(!MacAddr_isMulticast(pRA))
		{
			PPER_STA_MPAKEY_ENTRY pMapKey;

			for( index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++)
			{  
				pMapKey = &(pSecInfo->MAPKEYTable[index]);

				if( pMapKey->Valid && eqMacAddr(pMapKey->MACAdrss , pRA) )
				{
					return TRUE;
				}
			}

			return FALSE;
		}
		else
		{
			PPER_STA_DEFAULT_KEY_ENTRY pDefKey;

			//
			// TODO: remvoe our group key from PerStaDefKeyTable[]
			//
			for( index = 0 ; index < MAX_NUM_PER_STA_KEY ; index++)
			{
				pDefKey = &(pSecInfo->PerStaDefKeyTable[index]);

				if( pDefKey->Valid && eqMacAddr(pDefKey->MACAdrss, pAdapter->CurrentAddress) )
				{
					return TRUE;
				}
			}

			return FALSE;
		}
	}
}

//
// Description:
//	Descrypt the mgnt frame by SW and check if this packet is valid.
// Arguments:
//	[in] Adapter - 
//		The NIC context.
//	[in] pRfd -
//		The Rx buffer and information.
// Return:
//	RT_SEC_STATUS_SUCCESS if this packet is descrypted successfully, and otherwise if any error(this packet should be dropped).
// Assumption:
//	Before calling this function, be sure these conditions are correct,
//	(1) This packet is mgnt frame.
//	(2) MFP is enabled.
// By Bruce, 2009-10-08.
//
RT_SEC_STATUS
SecSWMFPDecryption(
	IN	PADAPTER	pAdapter,
	IN	PRT_RFD 	pRfd
	)
{
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_SECURITY_T	pSec = &(pMgntInfo->SecurityInfo);
	RT_SEC_STATUS	secStatus = RT_SEC_STATUS_SUCCESS;
	
	if(RT_SEC_STATUS_SUCCESS != (secStatus = SecSoftwareDecryption(pAdapter, pRfd)))
	{
		switch(pSec->PairwiseEncAlgorithm)
		{
		case RT_ENC_ALG_TKIP:
			CountRxMgntTKIPDecryptErrorsStatistics(pAdapter, pRfd);
			break;

		case RT_ENC_ALG_AESCCMP:
			CountRxMgntCCMPDecryptErrorsStatistics(pAdapter, pRfd);
			break;
		
		default: //for MacOSX Compiler warning.
			break;		
		}
			
		RT_TRACE(COMP_SEC , DBG_WARNING , ("SecSWMFPDecryption(): Fail (0x%08X) MFP packe decrypt !!\n", secStatus) );
		return secStatus;
	}
	else if( pMgntInfo->bInBIPMFPMode)
	{	// 802.11 MFP 
		OCTET_STRING	frame;
		
		FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	
		// Remove IV, but we haven't remove MIC
		if(frame.Length > (sMacHdrLng + pMgntInfo->SecurityInfo.EncryptionHeadOverhead))
		{
			PlatformMoveMemory((frame.Octet+ sMacHdrLng),
						(frame.Octet + sMacHdrLng + pMgntInfo->SecurityInfo.EncryptionHeadOverhead),
						(frame.Length - (sMacHdrLng + pMgntInfo->SecurityInfo.EncryptionHeadOverhead)));

			// Clear the WEP bit because we have removed the IV field.
			SET_80211_HDR_WEP(pRfd->Buffer.VirtualAddress, 0);
		}
		else
		{
			RT_TRACE(COMP_SEC , DBG_WARNING , ("SecSWMFPDecryption(): Check Packet Length failed!!\n") );
			return RT_SEC_STATUS_INVALID_PKT_LEN;
		}
	}
	
	else
	{
		OCTET_STRING	frame;
		
		FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);				

		// Check MHDR IE for TKIP...
		if(pSec->PairwiseEncAlgorithm == RT_ENC_ALG_TKIP)
		{
			// Remove the ICV so that we can caculate the MIC.
			MAKE_RFD_OFFSET_AT_BACK(pRfd, pMgntInfo->SecurityInfo.EncryptionTailOverhead);
			
			// Check the length is valid including MHDRIE and MIC
			if(frame.Length < (sMacHdrLng + pMgntInfo->SecurityInfo.EncryptionHeadOverhead + 2 + CCX_MFP_TKIP_MHDR_IE_LEN + TKIP_MIC_LEN)) // MHDRIE (2 + 12) + MIC (8)
				return RT_SEC_STATUS_INVALID_PKT_LEN;

			if(!CCX_VerifyRxMFP_MHDRIE(pAdapter, pRfd))
			{
				RT_PRINT_DATA(COMP_SEC, DBG_WARNING, " Verify MFP MHDRIE Error, content:\n", frame.Octet, frame.Length);
				CountRxMgntMFPTKIPMHDRStatistics(pAdapter, pRfd);
				return RT_SEC_STATUS_MFP_MGNT_MHDR_FAILURE;
			}

			RT_PRINT_DATA(COMP_CCX, DBG_LOUD, " Content:\n", pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

			// GET_80211_HDR_MORE_FRAG(frame.Octet) may need to chech more frg bit not set !!
			if(!SecCheckMIC( pAdapter , pRfd ))
			{
				return RT_SEC_STATUS_MGNT_MIC_FAILURE;
			}
		}
		
		// Remove IV, but we haven't remove MIC and MHDR IE
		if(frame.Length > (sMacHdrLng + pMgntInfo->SecurityInfo.EncryptionHeadOverhead))
		{
			PlatformMoveMemory((frame.Octet+ sMacHdrLng),
						(frame.Octet + sMacHdrLng + pMgntInfo->SecurityInfo.EncryptionHeadOverhead),
						(frame.Length - (sMacHdrLng + pMgntInfo->SecurityInfo.EncryptionHeadOverhead)));

			// Clear the WEP bit because we have removed the IV field.
			SET_80211_HDR_WEP(pRfd->Buffer.VirtualAddress, 0);
			
		}
		else
		{
			RT_TRACE(COMP_SEC , DBG_WARNING , ("SecSWMFPDecryption(): Check Packet Length failed!!\n") );
			return RT_SEC_STATUS_INVALID_PKT_LEN;
		}
	}
	RT_TRACE(COMP_SEC, DBG_LOUD , ("SecSWMFPDecryption(): SW Descryption OK!\n") );
	return RT_SEC_STATUS_SUCCESS;
}


//
// Description:
//	Handle 802.11w BIP MIC .
// Arguments:
//	[in] Adapter - 
//		The NIC context.
//	[in] pRfd -
//		The Rx buffer and information.
// Return:
//	RT_SEC_STATUS_SUCCESS if MIC and MMIE are OK , and otherwise if any error(this packet should be dropped and Do nothing !!). 
//

RT_SEC_STATUS
SecCheckMMIE(
	IN	PADAPTER	pAdapter,
	IN	PRT_RFD 		pRfd
	)
{
	OCTET_STRING	frame;
	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	PRT_SECURITY_T	pSec = &(pMgntInfo->SecurityInfo);
	u1Byte			Micdata[256] = {0};
	u4Byte			MACTextLength =  8;
	u1Byte			PlantData[256] = {0};
	pu1Byte			pCurrent = PlantData;
	u1Byte			PlantDataLen = 0;
	u1Byte			MMIE[2] = { 0x4c , 0x10 }; // ID and Len
	PMMIE_STRUC		pMMIE = NULL;
	u1Byte			index = 0;

	
	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	
	if( frame.Length > 256 || frame.Length < 18 )
		return RT_SEC_STATUS_MFP_MGNT_LEN_FAILURE;

	if( pSec->BIPKeyBuffer[0] == 0 )
	{
		RT_TRACE( COMP_SEC , DBG_TRACE , ("===> BIP key no install !!\n"));
		return RT_SEC_STATUS_SUCCESS;
	}
	//Check MMIE exit !!
	if( PlatformCompareMemory(  MMIE  , frame.Octet + frame.Length - 18, 2 ))
	{
		return RT_SEC_STATUS_MFP_MGNT_MMIE_FAILURE;
	}
	
	// Get MMIE 
	pMMIE = (PMMIE_STRUC)( frame.Octet + frame.Length - 18 );


	if(0 == PlatformCompareMemory(pMMIE->IPN, pSec->IPN, 6))
	{
		RT_PRINT_DATA(COMP_SEC, DBG_WARNING, "[WARNING] Skip this frame because we received the same IPN = \n", pMMIE->IPN, 6);
		return RT_SEC_STATUS_MGNT_IV_REPLAY;
	}
	// Check PN 
	for( index = 0 ; index < 6 ; index++  )
	{
		if(pMMIE->IPN[5 - index] > pSec->IPN[5- index])
			break;
		else if(pMMIE->IPN[5 - index] < pSec->IPN[5- index])
		{
			RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "MMIE PN Data:", pMMIE->IPN, 6);
			RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "My PN Data:", pSec->IPN, 6);
			return RT_SEC_STATUS_MFP_MGNT_MMIE_FAILURE; // PN error !!
		}
		else
			continue;
	}
	
	// Constr ADD | frame body 
	PlatformZeroMemory(  PlantData , 256 );
	PlatformMoveMemory( pCurrent ,  frame.Octet , 2 );  // FC
	// Reset Retry , PW_MGN and MORE bit
	SET_80211_HDR_RETRY(PlantData, 0);
	SET_80211_HDR_PWR_MGNT(PlantData, 0);
	SET_80211_HDR_MORE_DATA(PlantData, 0);
	pCurrent += 2;
	
	// A1 , A2 and A3
	PlatformMoveMemory( pCurrent , frame.Octet + 4 , 18 );
	pCurrent += 18;

	// Frame body 
	PlatformMoveMemory( pCurrent , frame.Octet + 24 , frame.Length - 24  );

	// PlantData length  = AAD + PacketLen - Hardlen
	PlantDataLen = 20 + frame.Length - 24 ;

	// Clear MIC Fild
	PlatformZeroMemory( (PlantData + PlantDataLen - 8 ) , 8);

	RT_PRINT_DATA(COMP_SEC , DBG_LOUD , " PlantData :\n"  , PlantData , PlantDataLen);

	// Calculate MIC 
	AES_CMAC_1W( PlantData  , PlantDataLen , pSec->BIPKeyBuffer , 16 , Micdata , &MACTextLength);

	// Compare MIC
	if(!PlatformCompareMemory( Micdata , pMMIE->MIC ,  8 ) )
	{
		PlatformMoveMemory(pSec->IPN, pMMIE->IPN, 6);
		return RT_SEC_STATUS_SUCCESS;
	}

	RT_PRINT_DATA(COMP_SEC, DBG_WARNING, "[WARNING] Mismatched MIC for MMIE=\n", pMMIE->MIC, 8);
	RT_PRINT_DATA(COMP_SEC, DBG_WARNING, "[WARNING] Mismatched MIC for Mine=\n", Micdata, MACTextLength);

	return RT_SEC_STATUS_MFP_MGNT_MMIE_MIC_FAILURE;
}


//
// Description:
//	Handle Rx Packet encryption status and descrypt it if need.
// Arguments:
//	[in] Adapter - 
//		The NIC context.
//	[in] pRfd -
//		The Rx buffer and information.
// Return:
//	RT_SEC_STATUS_SUCCESS if this packet is descrypted successfully, and otherwise if any error(this packet should be dropped). 
// By Bruce, 2009-10-15.
//
RT_SEC_STATUS
SecRxDescryption(
	IN	PADAPTER	pAdapter,
	IN	PRT_RFD 	pRfd
	)
{
	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	PRT_SECURITY_T	pSec = &(pMgntInfo->SecurityInfo);
	OCTET_STRING	frame;
	RT_SEC_STATUS	secStatus = RT_SEC_STATUS_SUCCESS;

	// Default mark this packet isn't MFP packet.
	pRfd->Status.bRxMFPPacket = FALSE;

	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);

	//3  // Check Frame WEP bit
	// This packet should not be descrypted.
	if(!Frame_WEP(frame))
	{
		if(!eqMacAddr(pMgntInfo->Bssid, Frame_Addr3(frame)))
			return RT_SEC_STATUS_SUCCESS;

		if(!((IsMgntDeauth(frame.Octet) || IsMgntDisasoc(frame.Octet)))) // Just check Deauth/Disassoc
			return RT_SEC_STATUS_SUCCESS;

		if(ACTING_AS_AP(pAdapter) || ACTING_AS_IBSS(pAdapter)) // Don't support in AP mode
			return RT_SEC_STATUS_SUCCESS;

		if(MacAddr_isMulticast(Frame_Addr1(frame)))
		{
			if(CCX_IS_MFP_ENABLED(pAdapter)) // CCX MFP does NOT support multicast
				return RT_SEC_STATUS_SUCCESS;
			else if(pMgntInfo->bInBIPMFPMode)
			{
				if(RT_SEC_STATUS_SUCCESS == (secStatus = SecCheckMMIE( pAdapter ,  pRfd )))
				{
					RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Check MMIE OK!\n"));
					pRfd->Status.bRxMFPPacket = TRUE;
					return RT_SEC_STATUS_SUCCESS;
				}
				else
				{
					RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Failed (0x%08X) from SecCheckMMIE()\n", secStatus));
					return secStatus;
				}
			}
			else
			{
				return RT_SEC_STATUS_SUCCESS;
			}
		}
		else // Unicast
		{
			if(CCX_IS_MFP_ENABLED(pAdapter))
			{				
				// Do nothing.
			}
			else if(pMgntInfo->bInBIPMFPMode)
			{
				if(((IsMgntDeauth(frame.Octet) && class3_err == Frame_DeauthReasonCode(frame))
					|| (IsMgntDisasoc(frame.Octet) && class3_err == Frame_DeassocReasonCode(frame))) &&
					RT_SA_QUERY_STATE_UNINITIALIZED == pSec->pmfSaState)
				{
					RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Received deauth/disassoc frame wit reason = 7 in PMF mode, start SA query process\n"));
					pSec->pmfSaState = RT_SA_QUERY_STATE_SA_DETECTED;
					PlatformSetTimer(pAdapter, &(pSec->SAQueryTimer), 100);
				}
			}
			else
			{
				return RT_SEC_STATUS_SUCCESS;
			}

			if(!SecIsTxKeyInstalled(pAdapter, pMgntInfo->Bssid))
				return RT_SEC_STATUS_SUCCESS;

			if(IsMgntDisasoc(frame.Octet) || IsMgntDeauth(frame.Octet) || (IsMgntAction(frame.Octet) && !CCX_IS_MFP_ENABLED(pAdapter)))
			{
				// CCXv5 S67 MFP enabled, these packets should be encrypted, if not, count the statistic.
				switch(pSec->PairwiseEncAlgorithm)
				{
				case RT_ENC_ALG_TKIP:
					CountRxMgntTKIPNoEncryptStatistics(pAdapter, pRfd);
					break;

				case RT_ENC_ALG_AESCCMP:
					CountRxMgntCCMPNoEncryptStatistics(pAdapter, pRfd);
					break;
				
				default: //for MacOSX Compiler warning.
					break;		
				}
				RT_PRINT_DATA(COMP_SEC, DBG_WARNING, "[WARNING] Received mgnt frame without protection bit\n", frame.Octet, frame.Length);
				return RT_SEC_STATUS_MGNT_FRAME_UNENCRYPT;
			}
		}	
		return RT_SEC_STATUS_SUCCESS;
	}


	//3 // Descryption for Mgnt Frame.
	if(IsMgntFrame(frame.Octet))
	{
		// Open Share Key mode, we do not handle this condition here.
		if(	IsMgntAuth(frame.Octet) && 
			(pSec->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40 || pSec->PairwiseEncAlgorithm == RT_ENC_ALG_WEP104))
		{
			return SecSoftwareDecryption(pAdapter, pRfd);
		}

		if(!eqMacAddr(pMgntInfo->Bssid, Frame_Addr3(frame)))
			return RT_SEC_STATUS_SUCCESS;		

		//
		// 802.11 MFP AES 
		//
		if(pMgntInfo->bInBIPMFPMode || CCX_IS_MFP_ENABLED(pAdapter))
		{
			// Mark this packet as MFP Packet to make sure the Descryption process fills the correct values in the fields.
			pRfd->Status.bRxMFPPacket = TRUE;
			RT_PRINT_DATA(COMP_SEC , DBG_LOUD, "SecRxDescryption(): Received Unicast Mgnt Frame with protection:\n" , frame.Octet, frame.Length );

			secStatus = SecSWMFPDecryption(pAdapter, pRfd);

			return secStatus;
		}
		else
		{
			RT_TRACE(COMP_SEC, DBG_WARNING, ("SecRxDescryption(): Error!!! Receive Mgnt Frame with WEP bit (Not support)!"));
			return RT_SEC_STATUS_PKT_TYPE_NOT_SUPPORT;
		}		
		
		return secStatus;
	}

	//3 // Data packet.
	if(IsDataFrame(frame.Octet))
	{	
		if(pSec->SWRxDecryptFlag)
		{
			if(pMgntInfo->SafeModeEnabled)
			{
				RT_TRACE_F(COMP_RECV, DBG_LOUD, ("Do not use SecSoftwareDecryption when SafeModeEnabled\n"));
				return RT_SEC_STATUS_SUCCESS;		
			}		

			if(RT_SEC_STATUS_SUCCESS != (secStatus = SecSoftwareDecryption(pAdapter, pRfd)))
			{
				RT_TRACE(COMP_SEC , DBG_LOUD , ("SecRxDescryption(): SW Descrypt Fail(0x%08X) for data packet\n", secStatus) );
				return secStatus;
			}
			return RT_SEC_STATUS_SUCCESS;
		}
	
		// Win7 SW Descryption Special Case
		if(  	pMgntInfo->NdisVersion >= RT_NDIS_VERSION_6_20  ||
			pMgntInfo->bConcurrentMode  ||
			pMgntInfo->bBTMode )
		{
			BOOLEAN	bSWSec = FALSE;

			if( (	MacAddr_isBcst(Frame_pDaddr(frame)) ||
				MacAddr_isMulticast(Frame_pDaddr(frame)) ) &&
				MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_NONE)
				bSWSec = TRUE;
			else if( pSec->PairwiseEncAlgorithm == RT_ENC_ALG_WEP104 ||
				     pSec->PairwiseEncAlgorithm == RT_ENC_ALG_WEP40)
				bSWSec = TRUE;
			else if( pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC &&
				     MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_NONE)
				bSWSec = TRUE;
			else if(!pMgntInfo->SecurityInfo.SWRxDecryptFlag && Frame_WEP(frame) && pRfd->Status.Decrypted == 0)
				bSWSec = TRUE;
			else
				bSWSec = FALSE;

			//for win7 FPGA Verification, Adhoc TP test
			if(pMgntInfo->bRegAdhocUseHWSec && pMgntInfo->Regdot11networktype == RT_JOIN_NETWORKTYPE_ADHOC)
				bSWSec = FALSE;
			
			if(bSWSec)
			{
				if(RT_SEC_STATUS_SUCCESS != (secStatus = SecSoftwareDecryption(pAdapter, pRfd)))
				{
					{
						RT_TRACE(COMP_SEC , DBG_TRACE , ("===>Fail Win7 SW decrypt\n") );
						return RT_SEC_STATUS_FAILURE;
					}		
				}
			}
		}
		
		// Normal Case with HW Descryption
		return RT_SEC_STATUS_SUCCESS;
	}

	RT_TRACE(COMP_SEC, DBG_WARNING, ("SecRxDescryption(): Error!!! Receive Frame(Type = 0x%02X) with WEP bit (Not support)!", frame.Octet[0]));
	return RT_SEC_STATUS_PKT_TYPE_NOT_SUPPORT;
}


//
// Description:
//		Append 802.11w MFP MMIE , just for AP mode !!
// Input :
//		posFrame : Action or mgnt frame , DA is broadcast !!
//		
void
SecAppenMMIE(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	)
{
	if( !(ACTING_AS_AP(pAdapter) ))
		return;

	// TO DO !!
	RT_TRACE(COMP_SEC, DBG_LOUD , ( " ===> AP mode support MFP ??\n" ));
	return;
}


BOOLEAN
SecStaGetANoseForS5(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pPduOS
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	u1Byte			Offset_TypeEAPOL = sMacHdrLng + LLC_HEADER_SIZE;	// 30
	OCTET_STRING	EapMessage;
	OCTET_STRING	EapKeyMessage;
	OCTET_STRING	ANonce;

	if( IsQoSDataFrame(pPduOS->Octet) )
	{
		Offset_TypeEAPOL += sQoSCtlLng;	// +2
	}

	if( Frame_WEP(*pPduOS))
	{
		Offset_TypeEAPOL += Adapter->MgntInfo.SecurityInfo.EncryptionHeadOverhead;	// 4 or 8
	}

	// Length Check
	if( pPduOS->Length < (Offset_TypeEAPOL+1) )
	{
		RT_TRACE( COMP_SEC, DBG_TRACE, ("SecGetANoseForS5(): invalid length(%d)\n", pPduOS->Length ) );
		return FALSE;
	}

	// 888e?
	if( (pPduOS->Octet[Offset_TypeEAPOL]==0x88) && (pPduOS->Octet[Offset_TypeEAPOL+1]==0x8e) )
	{
		//TYPE_LENGTH_FIELD_SIZE
		EapMessage.Octet = pPduOS->Octet+(Offset_TypeEAPOL+TYPE_LENGTH_FIELD_SIZE);
		EapMessage.Length = pPduOS->Length - (Offset_TypeEAPOL+TYPE_LENGTH_FIELD_SIZE);
		//RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "===> SecStaGetANoseForS5 \n", EapMessage.Octet, EapMessage.Length);

		if( (u1Byte)( *(EapMessage.Octet+1) ) == LIB1X_EAPOL_KEY )
		{
			FillOctetString(EapKeyMessage,	EapMessage.Octet+LIB1X_EAPOL_HDRLEN,	EapMessage.Length-LIB1X_EAPOL_HDRLEN );

			if( Message_KeyType(EapKeyMessage) == type_Pairwise &&
				Message_KeyMIC(EapKeyMessage) == FALSE) // 1st message of 4-Way
			{
				ANonce = Message_KeyNonce(EapKeyMessage);
				PlatformMoveMemory(pMgntInfo->mbS5ANose,ANonce.Octet,KEY_NONCE_LEN);
				RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "===> SecStaGetANoseForS5 \n", pMgntInfo->mbS5ANose,KEY_NONCE_LEN);
			}

		}
		
	}

	return FALSE;

}

BOOLEAN
SecStaGenPMKForS5(
	IN	PADAPTER		Adapter
	)
{
	
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);

	PasswordHash(pMgntInfo->mbPassphrase,pMgntInfo->mPasspharseLen, pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length, pMgntInfo->mbS5PMK);

	// Calcut PTK
	CalcPTK(pMgntInfo->Bssid , Adapter->CurrentAddress,
				pMgntInfo->mbS5ANose, pMgntInfo->mbS5SNose,
				pMgntInfo->mbS5PMK, PMK_LEN, pMgntInfo->mbS5PTK, PTK_LEN_TKIP);

	PlatformMoveMemory(pMgntInfo->PMDot11RSNRekeyPara.KCK , pMgntInfo->mbS5PTK , PTK_LEN_EAPOLMIC); //
	PlatformMoveMemory(pMgntInfo->PMDot11RSNRekeyPara.KEK , pMgntInfo->mbS5PTK+PTK_LEN_EAPOLMIC , PTK_LEN_EAPOLENC);

	RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "===>SecStaGenPMKForS5 PMK : \n", pMgntInfo->mbS5PMK, PMK_LEN);
	RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "===>SecStaGenPMKForS5 PTK : \n", pMgntInfo->mbS5PTK, PTK_LEN_TKIP);

	return TRUE;
}

// 
// Description:
//	Received the SA query packet from the STA.
//
RT_STATUS
OnSAQueryReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
    )
{
	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	
	RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "OnSAQueryReq(): Content:\n", posMpdu->Octet, posMpdu->Length);

	do
	{
		if(!pRfd->Status.bRxMFPPacket)
		{
			if(pMgntInfo->bInBIPMFPMode)
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("[WARNING] Received an unproteced SA query frame in PMF mode\n"));
			}
			rtStatus = RT_STATUS_PKT_DROP;
			break;
		}

		if(posMpdu->Length < MIN_ACTION_SA_QUERY_FRAME_LEN)
		{
			RT_TRACE_F(COMP_SEC, DBG_WARNING, ("[WARNING] Received Length (%d) of SA Query Req < %d, skip this frame\n", posMpdu->Length, MIN_ACTION_SA_QUERY_FRAME_LEN));
			rtStatus = RT_STATUS_INVALID_LENGTH;
		}

		SendSAQueryRsp(pAdapter, Frame_Addr2(*posMpdu), GET_ACTFRAME_SA_QUERY_ID(posMpdu->Octet));

		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Send SA Query Rsp with Identifier = 0x%04X\n", GET_ACTFRAME_SA_QUERY_ID(posMpdu->Octet)));
	}while(FALSE);


	return RT_STATUS_SUCCESS;
}

// 
// Description:
//	Received the SA query response packet from the STA.
//
RT_STATUS
OnSAQueryRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
    )
{
	PMGNT_INFO		pMgntInfo = &pAdapter->MgntInfo;
	PRT_SECURITY_T	pSecInfo = &(pMgntInfo->SecurityInfo);
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	
	RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "OnSAQueryRsp(): Content:\n", posMpdu->Octet, posMpdu->Length);

	do
	{
		if(!pRfd->Status.bRxMFPPacket)
		{
			if(pMgntInfo->bInBIPMFPMode)
			{
				RT_TRACE_F(COMP_SEC, DBG_WARNING, ("[WARNING] Received an unproteced SA query frame in PMF mode\n"));
			}
			rtStatus = RT_STATUS_PKT_DROP;
			break;
		}

		if(posMpdu->Length < MIN_ACTION_SA_QUERY_FRAME_LEN)
		{
			RT_TRACE_F(COMP_SEC, DBG_WARNING, ("[WARNING] Received Length (%d) of SA Query Req < %d, skip this frame\n", posMpdu->Length, MIN_ACTION_SA_QUERY_FRAME_LEN));
			rtStatus = RT_STATUS_INVALID_LENGTH;
		}

		if(!pMgntInfo->mAssoc)
		{
			RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Received SA query response without association, skip it\n"));
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		if(RT_SA_QUERY_STATE_SA_REQ_IN_PROGRESS != pSecInfo->pmfSaState)
		{
			RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Received SA query response mismaching my pmfState (%d), skip it\n", pSecInfo->pmfSaState));
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Idendifier = 0x%04X, my Identifier = 0x%04X\n", GET_ACTFRAME_SA_QUERY_ID(posMpdu->Octet), pSecInfo->SAReqIdentifier));

		if(GET_ACTFRAME_SA_QUERY_ID(posMpdu->Octet) != pSecInfo->SAReqIdentifier)
		{
			RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Received mismatched Idendifier = 0x%04X, my Identifier = 0x%04X, skip it\n", GET_ACTFRAME_SA_QUERY_ID(posMpdu->Octet), pSecInfo->SAReqIdentifier));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("SA Query process OK, cancel timer\n"));
		pSecInfo->pmfSaState = RT_SA_QUERY_STATE_UNINITIALIZED;
		PlatformCancelTimer(pAdapter, &(pSecInfo->SAQueryTimer));
	}while(FALSE);


	return RT_STATUS_SUCCESS;
}


//
// Description:
//	Fill and move IV and offset if this tx management frame shall be proteced.
// Arguments:
//	[in] Adapter - 
//		The NIC context.
//	[in] pTcb -
//		The context of tx frame.
// Return:
//	TRUE if this frame is filled and reserved with security content.
//	FALSE if this frame is returned without any change.
// Remark:
//	This function checks the association condition and determine if the current frame
//	needs to be proteced. If yes, this frame is filled with WEP bit set in the header.
// By Bruce-2014-12-26.
//
BOOLEAN
SecFillProtectTxMgntFrameHeader(
	IN	PADAPTER	pAdapter,
	IN	PRT_TCB		pTcb
	)
{
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo);
	PRT_SECURITY_T		pSecInfo = &(pMgntInfo->SecurityInfo);
	OCTET_STRING		osMpdu;
	BOOLEAN				bProtected = FALSE;
	
	FillOctetString(osMpdu, GET_FRAME_OF_FIRST_FRAG(pAdapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);

	do
	{
		// Currently, we only checks the action frame because some mgnt frames like deauth/disassociation are filled
		// with IV and WEP bit in the construction function, but others mgnt frames excluding deauth/disassociation/action
		// frame shall be sent without protection.
		// By Bruce, 2014-12-26.
		if(!IsMgntAction(osMpdu.Octet))
		{
			break;
		}

		if(!pMgntInfo->bInBIPMFPMode)
		{
			break;
		}

		// We only support AES in PMF.
		if(RT_ENC_ALG_AESCCMP != pSecInfo->PairwiseEncAlgorithm)
		{
			break;
		}

		// We don't support PMF in AP mode.
		if(MacAddr_isMulticast(Frame_Addr1(osMpdu)))
		{
			break;
		}

		if(0 == pSecInfo->EncryptionHeadOverhead)
			break;

		bProtected = TRUE;
	}while(FALSE);
	
	if(bProtected)
	{
		//3 // Determine different kind of frames before fill the IV
		// ToDo
		
		//RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "SecFillProtectTxMgntFrameHeader(): Before extend IV content:\n", osMpdu.Octet, osMpdu.Length);
		// Set WEP bit !!
		SET_80211_HDR_WEP(osMpdu.Octet, 1);

		PlatformMoveMemory(pSecInfo->SecBuffer, (osMpdu.Octet + sMacHdrLng), (osMpdu.Length - sMacHdrLng));
		
		PlatformMoveMemory((osMpdu.Octet+ sMacHdrLng + pSecInfo->EncryptionHeadOverhead),
									pSecInfo->SecBuffer,
									(osMpdu.Length - sMacHdrLng));
		PlatformZeroMemory(osMpdu.Octet + sMacHdrLng , pSecInfo->EncryptionHeadOverhead);
		osMpdu.Length += pSecInfo->EncryptionHeadOverhead;
		pTcb->BufferList[0].Length += pSecInfo->EncryptionHeadOverhead;
		pTcb->PacketLength += pSecInfo->EncryptionHeadOverhead;
		//RT_PRINT_DATA(COMP_SEC, DBG_LOUD, "SecFillProtectTxMgntFrameHeader(): After extend IV content:\n", osMpdu.Octet, osMpdu.Length);
	}

	return bProtected;
}

// Description:
//	Timer callback function to handle SA query mechanism.
VOID
SAQueryTimerCallback(
	IN	PRT_TIMER		pTimer
	)
{
	PADAPTER		pAdapter = (PADAPTER)pTimer->Adapter;
	PMGNT_INFO		pMgntInfo = &(pAdapter->MgntInfo);
	PRT_SECURITY_T	pSec = &pAdapter->MgntInfo.SecurityInfo;

	RT_TRACE_F(COMP_SEC, DBG_LOUD, ("pmfSaState = %d\n", pSec->pmfSaState));
	if(!pMgntInfo->mAssoc)
	{
		pSec->pmfSaState = RT_SA_QUERY_STATE_UNINITIALIZED;
		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("State is not associated, skip SA query\n"));
		return;
	}

	if(RT_SA_QUERY_STATE_SA_DETECTED == pSec->pmfSaState)
	{
		pSec->SAReqIdentifier =  (pSec->SAReqIdentifier + 1) % 0xFFFFFFFF;
		pSec->pmfSaState = RT_SA_QUERY_STATE_SA_REQ_IN_PROGRESS;
		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Send SA query request with ID = %04X to AP\n", pSec->SAReqIdentifier));
		SendSAQueryReq(pAdapter, pMgntInfo->Bssid, pSec->SAReqIdentifier);
		PlatformSetTimer(pAdapter, &(pSec->SAQueryTimer), 1000);
	}
	else if(RT_SA_QUERY_STATE_SA_REQ_IN_PROGRESS == pSec->pmfSaState)
	{
		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("Disconnect from the AP\n"));
		MgntDisconnectAP(pAdapter, unspec_reason);
		pSec->pmfSaState = RT_SA_QUERY_STATE_UNINITIALIZED;
	}
	else
	{
		RT_TRACE_F(COMP_SEC, DBG_LOUD, ("[WARNING] Unknown pmfstate = %d\n", pSec->pmfSaState));
	}
}


//
// 2011/03/04 MH Move all new CAN search metho to here forcommon binary . 
// We need to reorganize the locaton in he future.
// ---------------------------------------------------------------
//
#if (HW_EN_DE_CRYPTION_FOR_NEW_CAM_SEARCH_FLOW == 1)

VOID
SEC_AsocEntry_ResetEntry(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry
	)
{
	u1Byte	i;
	
	for( i = 4 ; i < Adapter->TotalCamEntry ; i++)
	{
		if( (Adapter->MgntInfo.SWCamTable[i].bUsed)
			&&(PlatformCompareMemory(pEntry->MacAddr, Adapter->MgntInfo.SWCamTable[i].macAddress, 6)==0))
			{
				CamDeleteOneEntry( Adapter , pEntry->MacAddr, i);
				PlatformZeroMemory(  &	Adapter->MgntInfo.SWCamTable[i] , sizeof(SW_CAM_TABLE) );
			}		
	}	
}

#endif // #if (HW_EN_DE_CRYPTION_FOR_NEW_CAM_SEARCH_FLOW == 1)

VOID
SecUpdateSWGroupKeyInfo(
	IN	PADAPTER		Adapter,
	IN	u4Byte			KeyIndex,
	IN	u4Byte			KeyLength,
	IN	pu1Byte			KeyMaterial
)
{
	PRT_SECURITY_T	pSecInfo = &Adapter->MgntInfo.SecurityInfo;
	RT_ENC_ALG		EncAlgorithm;

	EncAlgorithm = pSecInfo->GroupEncAlgorithm;
	RT_TRACE( COMP_INIT, DBG_LOUD, ("SecUpdateSWGroupKeyInfo(): SecLvl>0, Group EncAlgorithm=0x%08X, Pairwise EncAlgo=0x%08X\n", 
		EncAlgorithm, pSecInfo->PairwiseEncAlgorithm) );

	SecClearGroupKeyByIdx(Adapter, (u1Byte)KeyIndex);
	CopyMem(pSecInfo->KeyBuf[KeyIndex], KeyMaterial, KeyLength );
	pSecInfo->KeyLen[KeyIndex]= (u1Byte)KeyLength;

	if(EncAlgorithm == RT_ENC_ALG_TKIP)
	{
		if((KeyIndex & ADD_KEY_IDX_AS)){//kcwu: Key is set by an Authenticator, exchange Tx/Rx MIC
			u1Byte tmpbuf[TKIP_MIC_KEY_LEN];
			PlatformMoveMemory(tmpbuf, KeyMaterial+TKIP_ENC_KEY_LEN, TKIP_MIC_KEY_LEN);
			PlatformMoveMemory(KeyMaterial+TKIP_ENC_KEY_LEN, 
				KeyMaterial+TKIP_ENC_KEY_LEN+TKIP_MIC_KEY_LEN, TKIP_MIC_KEY_LEN);
			PlatformMoveMemory(KeyMaterial+TKIP_ENC_KEY_LEN+TKIP_MIC_KEY_LEN, tmpbuf, TKIP_MIC_KEY_LEN);
		}
		PlatformMoveMemory(pSecInfo->RxMICKey, KeyMaterial+TKIP_MICKEYRX_POS, TKIP_MIC_KEY_LEN);
		PlatformMoveMemory(pSecInfo->TxMICKey, KeyMaterial+TKIP_MICKEYTX_POS, TKIP_MIC_KEY_LEN);
	}
	else if(EncAlgorithm == RT_ENC_ALG_AESCCMP)
	{
		//2004/09/07, kcwu, AES Key Initialize
		AESCCMP_BLOCK   blockKey;
		PlatformMoveMemory(blockKey.x, pSecInfo->KeyBuf[KeyIndex],16);
		AES_SetKey(blockKey.x,
					AESCCMP_BLK_SIZE*8,
					(u4Byte *)pSecInfo->AESKeyBuf[KeyIndex]);     // run the key schedule
	}
	else if(EncAlgorithm==RT_ENC_ALG_WEP40 || EncAlgorithm==RT_ENC_ALG_WEP104)
	{
		RT_TRACE( COMP_SEC, DBG_LOUD, ("MgntActSet_802_11_ADD_KEY(): SecLvl>0, EncAlgorithm=%d\n", EncAlgorithm) );
	}
}

//
// ---------------------------------------------------------------
// 2011/03/04 MH Move all new CAN search metho to here forcommon binary . 
// We need to reorganize the locaton in he future.
//
