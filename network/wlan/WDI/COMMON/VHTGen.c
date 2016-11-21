
#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "VHTGen.tmh"
#endif

#if (VHT_SUPPORT == 1)

// 				20/40/80,	ShortGI,	MCS Rate 
const u2Byte VHT_MCS_DATA_RATE[3][2][30] = 
	{	{	{13, 26, 39, 52, 78, 104, 117, 130, 156, 156,
			 26, 52, 78, 104, 156, 208, 234, 260, 312, 312,
			 39, 78, 117, 156, 234, 312, 351, 390, 468, 520},			// Long GI, 20MHz
			{14, 29, 43, 58, 87, 116, 130, 144, 173, 173,
			29, 58, 87, 116, 173, 231, 260, 289, 347, 347,
			43,	87, 130, 173, 260, 347,390,	433,	520, 578}	},		// Short GI, 20MHz
		{	{27, 54, 81, 108, 162, 216, 243, 270, 324, 360, 
			54, 108, 162, 216, 324, 432, 486, 540, 648, 720,
			81, 162, 243, 324, 486, 648, 729, 810, 972, 1080}, 		// Long GI, 40MHz
			{30, 60, 90, 120, 180, 240, 270, 300,360, 400, 
			60, 120, 180, 240, 360, 480, 540, 600, 720, 800,
			90, 180, 270, 360, 540, 720, 810, 900, 1080, 1200}},		// Short GI, 40MHz
		{	{59, 117,  176, 234, 351, 468, 527, 585, 702, 780,
			117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560,
			176, 351, 527, 702, 1053, 1404, 1580, 1755, 2106, 2340}, 	// Long GI, 80MHz
			{65, 130, 195, 260, 390, 520, 585, 650, 780, 867, 
			130, 260, 390, 520, 780, 1040, 1170, 1300, 1560,1734,
			195, 390, 585, 780, 1170, 1560, 1755, 1950, 2340, 2600}	}	// Short GI, 80MHz
	};

VOID
VHTDebugVHTCapability(
	u4Byte			DebugLevel,
	PADAPTER		Adapter,
	POCTET_STRING	pocCap,
	pu1Byte			TitleString
	
)
{
	pu1Byte 		pCapELE =  (pu1Byte)&pocCap->Octet[0];
	pu1Byte		pMCS;
	
	pMCS= GET_VHT_CAPABILITY_ELE_RX_MCS(pCapELE);
	
	RT_TRACE(COMP_HT, DebugLevel, ("<Log VHT Capability>. Called by %s\n", TitleString ));

	RT_TRACE(COMP_HT, DebugLevel, ("\tSupported Channel Width = %d\n", (GET_VHT_CAPABILITY_ELE_CHL_WIDTH(pCapELE) )));
	RT_TRACE(COMP_HT, DebugLevel, ("\tSupport Short GI for 80M = %s\n", (GET_VHT_CAPABILITY_ELE_SHORT_GI80M(pCapELE))?"YES": "NO"));
	RT_TRACE(COMP_HT, DebugLevel, ("\tSupport TX STBC = %s\n", (GET_VHT_CAPABILITY_ELE_TX_STBC(pCapELE))?"YES": "NO"));
	RT_TRACE(COMP_HT, DebugLevel, ("\tMax MPDU Size = %s\n", (GET_VHT_CAPABILITY_ELE_MAX_MPDU_LENGTH(pCapELE))?"3839": "7935"));
	RT_TRACE(COMP_HT, DebugLevel, ("\tMPDU Density = %d\n", GET_HT_CAPABILITY_ELE_MPDU_DENSITY(pCapELE)));
	RT_TRACE(COMP_HT, DebugLevel, ("\tRx MCS Rate Set = [%x][%x][%x][%x]\n", pMCS[0], pMCS[1], pMCS[2], pMCS[3]));
	RT_TRACE(COMP_HT, DebugLevel, ("\tRx LDPC = %d\n", GET_VHT_CAPABILITY_ELE_RX_LDPC(pCapELE)));
	RT_TRACE(COMP_HT, DebugLevel, ("\n"));		
		
}


u2Byte
VHTMcsToDataRate(
	PADAPTER		Adapter,
	u2Byte			VHTMcsRate
	)
{
	BOOLEAN						isShortGI = FALSE;
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	PRT_HIGH_THROUGHPUT		pHTInfo = pMgntInfo->pHTInfo;
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = pMgntInfo->pVHTInfo;

	VHTMcsRate -=MGN_VHT1SS_MCS0;
	
	switch(pMgntInfo->dot11CurrentChannelBandWidth){
		case CHANNEL_WIDTH_20:
			isShortGI = pHTInfo->bCurShortGI20MHz?1:0;
			break;
		case CHANNEL_WIDTH_40:
			isShortGI = pHTInfo->bCurShortGI40MHz?1:0;
			break;
		case CHANNEL_WIDTH_80:
			isShortGI = pVHTInfo->bCurShortGI80MHz?1:0;
			break;
	}

	return VHT_MCS_DATA_RATE[pMgntInfo->dot11CurrentChannelBandWidth][isShortGI][VHTMcsRate];
}


BOOLEAN
VHTFilterMCSRate(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pSupportMCS,
	OUT	pu1Byte			pOperateMCS
)
{
	u1Byte		i = 0, j = 0;
	u1Byte		RegRate, dot11Rate, OpRate;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	
	for(i = 0; i < 2; i++)		
	{
		pOperateMCS[i] = 0;
		
		for(j = 0; j < 8; j+=2)
		{
			RegRate = (pMgntInfo->Regdot11VHTOperationalRateSet[i] >> j) & 3;
			dot11Rate = (pSupportMCS[i] >> j) & 3;
			if(RegRate == 3 || dot11Rate == 3)  //0x3 indicates not supported that num of SS
				OpRate = 3;
			else if(RegRate > dot11Rate)
				OpRate = dot11Rate;
			else	
				OpRate = RegRate;

			pOperateMCS[i] |= (OpRate << j);
		}
	}
	
	return TRUE;
}


u1Byte	
RateToSpatialStream(
	pu1Byte			pVHTRate
)
{
	u1Byte	i,j , tmpRate;
	u1Byte	NumSS = 0;
		
	for(i = 0; i < 2; i++)		
	{
		for(j = 0; j < 8; j+=2)
		{
			tmpRate = (pVHTRate[i] >> j) & 3;

			switch(tmpRate){
			case 2:
			case 1:
			case 0:
				NumSS++;
				break;

			default:
				break;
			}
		}
			
	}
	RT_TRACE(COMP_HT, DBG_LOUD, ("RateToSpatialStream(): Number of Spatial Stream supported = %d\n", NumSS));

	return NumSS;
}


VOID
SpatialStreamToRate(
	PADAPTER		Adapter,
	u1Byte			NumSS,
	pu1Byte			pVHTRate
)
{
	u1Byte		i = 0, j = 0;
	u1Byte		RegRate, dot11Rate, OpRate;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	
	for(i = 0; i < 2; i++)		
	{
		pVHTRate[i] = 0;
		
		for(j = 0; j < 8; j+=2)
		{
			RegRate = (pMgntInfo->Regdot11VHTOperationalRateSet[i] >> j) & 3;
			dot11Rate = (pVHTRate[i] >> j) & 3;
			if(RegRate == 3)  //0x3 indicates not supported that num of SS
				OpRate = 3;
			else if(NumSS <= j)
				OpRate = 3;
			else if(RegRate > dot11Rate)
				OpRate = dot11Rate;
			else	
				OpRate = RegRate;

			pVHTRate[i] |= (OpRate << j);
		}
	}
}


u1Byte 
VHTIOTActIsAMsduEnable(
	PADAPTER		Adapter
)
{
	HT_IOT_ACTION_E	retValue = (HT_IOT_ACTION_E)0;
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;

	if(pMgntInfo->pHTInfo->ForcedAMSDUMode == HT_AMSDU_ENABLE)
		retValue = HT_IOT_ACT_AMSDU_ENABLE;
	else if(pMgntInfo->pHTInfo->ForcedAMSDUMode == HT_AMSDU_WITHIN_AMPDU)
		retValue = HT_IOT_ACT_AMSDU_AMPDU;

	return retValue;
}



VOID
VHTConstructOpModeNotification(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr,
	OUT	pu1Byte			Buffer,
	OUT	pu4Byte			pLength
	)
{
	PMGNT_INFO      				pMgntInfo = &Adapter->MgntInfo;
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	u1Byte						opmode = 0;
	OCTET_STRING				osNotifFrame, tmp;

	FillOctetString(osNotifFrame, Buffer, 0);
	*pLength = 0;

	ConstructMaFrameHdr(
					Adapter, 
					Addr, 
					ACT_CAT_VHT, 
					ACT_VHT_OPMODE_NOTIFICATION, 
					&osNotifFrame);

	// Operating Mode
	SET_VHT_OPERATING_MODE_FIELD_CHNL_WIDTH(&opmode, pVHTInfo->BWToSwitch);

	SET_VHT_OPERATING_MODE_FIELD_RX_NSS(&opmode, (pVHTInfo->RxSSToSwitch-1));
	SET_VHT_OPERATING_MODE_FIELD_RX_NSS_TYPE(&opmode, pVHTInfo->RxSSTypeBfmeeToSwitch);

	FillOctetString(tmp, &opmode, 1);
	PacketAppendData(&osNotifFrame, tmp);

	*pLength = osNotifFrame.Length;
}


/*
	The setting used in this functoin shall be our HT Rx capability that we want to inform the peer STA.
	Both AP and STA Attach the Capailbity Element
	AP/STA should include this element only if HTEnable is turned on.
*/
VOID
VHTConstructCapabilityElement(
	PADAPTER		Adapter,
	POCTET_STRING	posVHTCap,
	BOOLEAN			bAssoc
)
{	
	PMGNT_INFO      			pMgntInfo = &Adapter->MgntInfo;
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	pu1Byte					pCapELE = NULL, pCapVHTMCS;
	u2Byte					HighestRate;

	u1Byte					VHtSTBC = 0;

	PlatformZeroMemory(posVHTCap->Octet, posVHTCap->Length);
	
	pCapELE = (pu1Byte)posVHTCap->Octet;
	posVHTCap->Length = 12;

	// B0 B1 Maximum MPDU Length
	// in Current AC AP, we can only declare 4K MPDU support, the performance will be better.
	// Need to add IOT pattern later.
	//SET_VHT_CAPABILITY_ELE_MAX_MPDU_LENGTH(pCapELE, 2); 


	// B2 B3 Supported Channel Width Set
	SET_VHT_CAPABILITY_ELE_CHL_WIDTH(pCapELE, 0);  //indicate we don't support neither 160M nor 80+80M bandwidth. by page

	// B4 Rx LDPC
	if(TEST_FLAG(pVHTInfo->VhtLdpcCap, LDPC_VHT_ENABLE_RX))
	{
		SET_VHT_CAPABILITY_ELE_RX_LDPC(pCapELE, 1); 
	}
	
	// B5 ShortGI for 80MHz
	SET_VHT_CAPABILITY_ELE_SHORT_GI80M(pCapELE, pVHTInfo->bRegShortGI80MHz ? 1 : 0); // We can receive Short GI of 80M
	// B6 ShortGI for 160MHz
	SET_VHT_CAPABILITY_ELE_SHORT_GI160M(pCapELE, 0); // We can not receive 160M Short GI

	// B7 Tx STBC
	if(TEST_FLAG(pVHTInfo->VhtStbcCap, STBC_VHT_ENABLE_TX))
	{
		SET_VHT_CAPABILITY_ELE_TX_STBC(pCapELE, 1);  //now HW support both tx/rx STBC. by page 20111108
	}

	// B8 B9 B10 Rx STBC
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_RX_STBC, (pu1Byte)&VHtSTBC);
	if(TEST_FLAG(pVHTInfo->VhtStbcCap, STBC_VHT_ENABLE_RX))
	{
		SET_VHT_CAPABILITY_ELE_RX_STBC(pCapELE, VHtSTBC);
	}

	// B11 SU Beamformer Capable
	if(TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_BEAMFORMER_ENABLE))
	{
		SET_VHT_CAPABILITY_ELE_SU_BFER(pCapELE, TRUE);
		SET_VHT_CAPABILITY_ELE_SOUNDING_DIMENSIONS(pCapELE, HAL_QueryBeamformerCap(Adapter));
	}

	// B12 SU Beamformee Capable
	if(TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_BEAMFORMEE_ENABLE))
	{
		SET_VHT_CAPABILITY_ELE_SU_BFEE(pCapELE, TRUE);
		SET_VHT_CAPABILITY_ELE_BFER_ANT_SUPP(pCapELE, HAL_QueryBeamformeeCap(Adapter));
	}
	

	// B19 MU Beamformer Capable
	if (TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_MU_MIMO_AP_ENABLE)){
		SET_VHT_CAPABILITY_ELE_MU_BFER(pCapELE, TRUE);
		SET_VHT_CAPABILITY_ELE_SOUNDING_DIMENSIONS(pCapELE, HAL_QueryBeamformerCap(Adapter));
	}
	// B20 MU Beamformee Capable
	if  (TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_MU_MIMO_STA_ENABLE)){
		SET_VHT_CAPABILITY_ELE_MU_BFEE(pCapELE, TRUE);
		SET_VHT_CAPABILITY_ELE_BFER_ANT_SUPP(pCapELE, HAL_QueryBeamformeeCap(Adapter));
	}
	// B21 VHT TXOP PS
	SET_VHT_CAPABILITY_ELE_TXOP_PS(pCapELE, 0);
	// B22 +HTC-VHT Capable
	SET_VHT_CAPABILITY_ELE_HTC_VHT(pCapELE, 1);
	// B23 24 25 Maximum A-MPDU Length Exponent
	if (pMgntInfo->RegAMfactor < 0x7)
	{
		SET_VHT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(pCapELE, pMgntInfo->RegAMfactor);  // 8812A can receive AMPDU that has no length limit. by page, 20111111
	}
	else
	{
		SET_VHT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(pCapELE, 7);  // 8812A can receive AMPDU that has no length limit. by page, 20111111
	}

	// B26 27 VHT Link Adaptation Capable
	SET_VHT_CAPABILITY_ELE_LINK_ADAPTION(pCapELE, 0);

	pCapVHTMCS = GET_VHT_CAPABILITY_ELE_RX_MCS(pCapELE);
	PlatformMoveMemory(pCapVHTMCS, pMgntInfo->Regdot11VHTOperationalRateSet, 2);

	pCapVHTMCS = GET_VHT_CAPABILITY_ELE_TX_MCS(pCapELE);
	PlatformMoveMemory(pCapVHTMCS, pMgntInfo->Regdot11VHTOperationalRateSet, 2);

	HighestRate = VHT_MCS_DATA_RATE[pMgntInfo->dot11CurrentChannelBandWidth][FALSE][((pMgntInfo->RegVHTHighestOperaRate - MGN_VHT1SS_MCS0)&0x3f)];
	HighestRate = (HighestRate+1) >> 1;	

	SET_VHT_CAPABILITY_ELE_MCS_RX_HIGHEST_RATE(pCapELE, HighestRate);     //indicate we support highest rx rate is 600Mbps.
	if(pVHTInfo->bAssignTxLGIRate)
		HighestRate = pVHTInfo->highestTxLGIRate;
	SET_VHT_CAPABILITY_ELE_MCS_TX_HIGHEST_RATE(pCapELE, HighestRate); //indicate we support highest tx rate is 600Mbps.
		
	RT_PRINT_DATA(COMP_HT, DBG_TRACE, "Construct VHTCap in Beacon/Assoc/ReAssoc", posVHTCap->Octet, posVHTCap->Length);
	VHTDebugVHTCapability(DBG_TRACE, Adapter, posVHTCap, (pu1Byte)"VHTConstructCapability()");
}


VOID
VHTConstructOperationElement(
	PADAPTER		Adapter,
	POCTET_STRING	posVHTOperation
)
{
	PMGNT_INFO      			pMgntInfo = &Adapter->MgntInfo;
	pu1Byte					pOpELE = NULL;
	CHANNEL_WIDTH			ChnlWidth;	

	pOpELE = (pu1Byte)posVHTOperation->Octet;
	posVHTOperation->Length = 5;

	//Set BandWidth
	if(	(pMgntInfo->dot11CurrentChannelBandWidth == CHANNEL_WIDTH_20) ||
		(pMgntInfo->dot11CurrentChannelBandWidth == CHANNEL_WIDTH_40) )
		ChnlWidth = (CHANNEL_WIDTH)0;
	else if(pMgntInfo->dot11CurrentChannelBandWidth == CHANNEL_WIDTH_80)
		ChnlWidth = (CHANNEL_WIDTH)1;
	
	SET_VHT_OPERATION_ELE_CHL_WIDTH(pOpELE, ChnlWidth);
	//center frequency
	SET_VHT_OPERATION_ELE_CHL_CENTER_FREQ1(pOpELE,pMgntInfo->pChannelInfo->CurrentChannelCenterFrequency);
	SET_VHT_OPERATION_ELE_CHL_CENTER_FREQ2(pOpELE,0);
	SET_VHT_OPERATION_ELE_BASIC_MCS_SET(pOpELE, 0xFFFF);
}

VOID
VHTSendOperatingModeNotification(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER		pBuf;
	u1Byte					DataRate = MgntQuery_MgntFrameTxRate(Adapter, Addr);

	RT_TRACE(COMP_HT, DBG_LOUD, ("=====>Send Operating Mode Notification()\n"));

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		VHTConstructOpModeNotification(Adapter, Addr, pBuf->Buffer.VirtualAddress, &pTcb->PacketLength);

		if(pTcb->PacketLength != 0)
		{
			if(ACTING_AS_AP(Adapter))
				MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, BE_QUEUE, DataRate);
			else
				MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
		}
	}
	
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}


RT_STATUS
VHT_OnOpModeNotify(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	PMGNT_INFO      		pMgntInfo = &Adapter->MgntInfo;
	pu1Byte				opmodenotif = NULL;
	CHANNEL_WIDTH		targetbw = (CHANNEL_WIDTH)0;
	u1Byte				targetrxss = 0, currentrxss = 0;
	BOOLEAN				bUpdateRA = FALSE;

	// CategoryCode(1) + ActionCode(1) + OpModeNotification(1)
	if(posMpdu->Length < sMacHdrLng + 3)
	{
		RT_TRACE(COMP_HT, DBG_SERIOUS, ("VHT_OnOpModeNotify(): Invalid length(%d) of frame\n", posMpdu->Length));
		return RT_STATUS_INVALID_DATA;
	}

	opmodenotif = posMpdu->Octet + sMacHdrLng + 2;
	targetbw = 	(CHANNEL_WIDTH)GET_VHT_OPERATING_MODE_FIELD_CHNL_WIDTH(opmodenotif);
	targetrxss = (GET_VHT_OPERATING_MODE_FIELD_RX_NSS(opmodenotif) + 1);
	RT_TRACE(COMP_MLME, DBG_LOUD, ("VHT_OnOpModeNotify(): bw = %d, ss = %d\n", targetbw, targetrxss));

	if(targetbw != pMgntInfo->currentRABw)
	{
		RT_TRACE(COMP_HT, DBG_LOUD, ("VHT_OnOpModeNotify(): operating mode notification change BW\n"));
		bUpdateRA = TRUE;
	}

	currentrxss = RateToSpatialStream(pMgntInfo->dot11VHTOperationalRateSet);
	if( (targetrxss != currentrxss) || bUpdateRA )
	{
		RT_TRACE(COMP_HT, DBG_LOUD, ("VHT_OnOpModeNotify(): operating mode notification change tx spatial stream\n"));

		SpatialStreamToRate(Adapter, targetrxss, pMgntInfo->dot11VHTOperationalRateSet);

		pMgntInfo->Ratr_State = 0;
		Adapter->HalFunc.UpdateHalRAMaskHandler(Adapter, pMgntInfo->mMacId, NULL, pMgntInfo->Ratr_State);

	}
	return RT_STATUS_SUCCESS;
}

VOID
VHTOnAssocRsp(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	asocpdu
)
{
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	pu1Byte						pPeerVHTCap, pPeerVHTInfo, pCapMCS;
	OCTET_STRING				osTmp;

	RT_TRACE(COMP_MLME, DBG_LOUD,("==============>VHTOnAssocRsp \n "));
	
	if( pVHTInfo->bCurrentVHTSupport == FALSE )
	{
		RT_TRACE( COMP_MLME, DBG_LOUD, ("<=== VHTOnAssocRsp(): VHT_DISABLE\n") );
		return;
	}
	
	PlatformZeroMemory(pVHTInfo->PeerVHTCapBuf, sizeof(pVHTInfo->PeerVHTCapBuf));
	PlatformZeroMemory(pVHTInfo->PeerVHTInfoBuf, sizeof(pVHTInfo->PeerVHTInfoBuf));
		
	// Get VHT CAP and copy into buffer
	osTmp= PacketGetElement( asocpdu, EID_VHTCapability, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);

	if(pMgntInfo->bRegIOTBcm256QAM && osTmp.Length == 0)
	{
		osTmp = PacketGetElement(asocpdu, EID_Vendor, OUI_SUB_11AC_EPIG_VHT_CAP, OUI_SUBTYPE_DONT_CARE);
		if(osTmp.Length != 0)
		{
			osTmp.Octet += 7;
			osTmp.Length = MAX_VHT_CAP_LEN;
		}
	}
		
	osTmp.Length = osTmp.Length > sizeof(pVHTInfo->PeerVHTCapBuf)?\
		sizeof(pVHTInfo->PeerVHTCapBuf):osTmp.Length;	//prevent from overflow
		
	if(osTmp.Length > 0)
		CopyMem(pVHTInfo->PeerVHTCapBuf, osTmp.Octet, osTmp.Length);
	
	// Get VHT Operation and copy into buffer
	osTmp= PacketGetElement( asocpdu, EID_VHTOperation, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);

	if(pMgntInfo->bRegIOTBcm256QAM && osTmp.Length == 0)
	{
		OCTET_STRING	EpigIE = PacketGetElement(asocpdu, EID_Vendor, OUI_SUB_EPIG_IE, OUI_SUBTYPE_DONT_CARE);
		if(EpigIE.Length == 26)
		{
			osTmp.Octet = EpigIE.Octet+21;
			osTmp.Length = 5;
		}
	}

	osTmp.Length = osTmp.Length>sizeof(pVHTInfo->PeerVHTInfoBuf)?\
		sizeof(pVHTInfo->PeerVHTInfoBuf):osTmp.Length;	//prevent from overflow
		
	if(osTmp.Length > 0)
		CopyMem(pVHTInfo->PeerVHTInfoBuf, osTmp.Octet, osTmp.Length);
	
	pPeerVHTCap = (pu1Byte)(pVHTInfo->PeerVHTCapBuf);

	pPeerVHTInfo = (pu1Byte)(pVHTInfo->PeerVHTInfoBuf);

	RT_DISP(FMLME, LINK_STS, 
	("VHTOnAssocRsp(): call VHTSetBandWidthOnAsocRsp VHT_CAP=%04x VHT_INFO=%04x\n", 
	EF2Byte(*((UNALIGNED pu2Byte)pPeerVHTCap)), EF2Byte(*((UNALIGNED pu2Byte)pPeerVHTInfo))) );

	// B4 Rx LDPC
	if(TEST_FLAG(pVHTInfo->VhtLdpcCap, LDPC_VHT_ENABLE_TX))
	{
		SET_FLAG(pVHTInfo->VhtCurLdpc, GET_VHT_CAPABILITY_ELE_RX_LDPC(pPeerVHTCap) ? (LDPC_VHT_ENABLE_TX | LDPC_VHT_CAP_TX) : 0);
		RT_TRACE_F(COMP_HT, DBG_LOUD, ("Target BSS support VHT LDPC, enable Tx LDPC!\n"));
	}
	
	// B5 Short GI for 80 MHz
	pVHTInfo->bCurShortGI80MHz = (GET_VHT_CAPABILITY_ELE_SHORT_GI80M(pPeerVHTCap) & pVHTInfo->bRegShortGI80MHz) ? TRUE : FALSE;
	RT_TRACE_F(COMP_HT, DBG_LOUD, ("Current ShortGI80MHz = %d\n", pVHTInfo->bCurShortGI80MHz));

	// B8 B9 B10 Rx STBC
	if(	TEST_FLAG(pVHTInfo->VhtStbcCap, STBC_VHT_ENABLE_TX) && GET_VHT_CAPABILITY_ELE_RX_STBC(pPeerVHTCap))
	{
		SET_FLAG(pVHTInfo->VhtCurStbc, (STBC_VHT_ENABLE_TX | STBC_VHT_CAP_TX) );
	}

	// B11 SU Beamformer Capable, the target supports Beamformer and we are Beamformee
	if(	TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_BEAMFORMER_ENABLE) &&
		GET_VHT_CAPABILITY_ELE_SU_BFEE(pPeerVHTCap))
	{
		SET_FLAG(pVHTInfo->VhtCurBeamform, BEAMFORMING_VHT_BEAMFORMEE_ENABLE);
		// Shift to BEAMFORMING_VHT_BEAMFORMER_STS_CAP
		SET_FLAG(pVHTInfo->VhtCurBeamform, GET_VHT_CAPABILITY_ELE_SU_BFEE_STS_CAP(pPeerVHTCap)<<8);
	}

	// B12 SU Beamformee Capable, the target supports Beamformee and we are Beamformer
	if(	TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_BEAMFORMEE_ENABLE) &&
		GET_VHT_CAPABILITY_ELE_SU_BFER(pPeerVHTCap))
	{
		SET_FLAG(pVHTInfo->VhtCurBeamform, BEAMFORMING_VHT_BEAMFORMER_ENABLE);
		// Shit to BEAMFORMING_VHT_BEAMFORMEE_SOUND_DIM
		SET_FLAG(pVHTInfo->VhtCurBeamform, GET_VHT_CAPABILITY_ELE_SU_BFER_SOUND_DIM_NUM(pPeerVHTCap)<<12);
	}

	// B19 MU Beamformer Capable, the target supports Beamformer and we are Beamformee
	if(	TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_MU_MIMO_AP_ENABLE) &&
		GET_VHT_CAPABILITY_ELE_MU_BFEE(pPeerVHTCap))
	{
		SET_FLAG(pVHTInfo->VhtCurBeamform, BEAMFORMING_VHT_MU_MIMO_STA_ENABLE);
		// Shift to BEAMFORMING_VHT_BEAMFORMER_STS_CAP
		SET_FLAG(pVHTInfo->VhtCurBeamform, GET_VHT_CAPABILITY_ELE_SU_BFEE_STS_CAP(pPeerVHTCap)<<8);
	}

	// B20 MU Beamformee Capable, the target supports Beamformee and we are Beamformer
	if(	TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_MU_MIMO_STA_ENABLE) &&
		GET_VHT_CAPABILITY_ELE_MU_BFER(pPeerVHTCap))
	{
		SET_FLAG(pVHTInfo->VhtCurBeamform, BEAMFORMING_VHT_MU_MIMO_AP_ENABLE);
		// Shit to BEAMFORMING_VHT_BEAMFORMEE_SOUND_DIM
		SET_FLAG(pVHTInfo->VhtCurBeamform, GET_VHT_CAPABILITY_ELE_SU_BFER_SOUND_DIM_NUM(pPeerVHTCap)<<12);
	}

	RT_TRACE_F(COMP_HT, DBG_LOUD, ("Current VHT Beamforming Setting = %02X\n", pVHTInfo->VhtCurBeamform));

	// B23 B24 B25 Maximum A-MPDU Length Exponent
	pVHTInfo->AMPDU_Len = GET_VHT_CAPABILITY_ELE_MAX_RXAMPDU_FACTOR(pPeerVHTCap);
	if(pVHTInfo->AMPDU_Len > pHTInfo->CurrentAMPDUFactor)
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_FACTOR, &pVHTInfo->AMPDU_Len);
	
	pCapMCS = GET_VHT_CAPABILITY_ELE_RX_MCS(pPeerVHTCap);
	VHTFilterMCSRate(Adapter, pCapMCS, pMgntInfo->dot11VHTOperationalRateSet);
	pMgntInfo->VHTHighestOperaRate = VHTGetHighestMCSRate(
													Adapter, 
													pMgntInfo->dot11VHTOperationalRateSet);
	
	pVHTInfo->PeerChnlBW = (CHANNEL_WIDTH)GET_VHT_OPERATION_ELE_CHL_WIDTH(pVHTInfo->PeerVHTInfoBuf);
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_MAX_TIME, &pMgntInfo->VHTHighestOperaRate);
}


/*
*  This function is called when
*  (1) MPInitialization Phase
*  (2) Receiving of Deauthentication from AP
*/

// TODO: Should this funciton be called when receiving of Disassociation?
VOID
VHTInitializeVHTInfo(
	IN PADAPTER	Adapter
)
{
	PRT_VERY_HIGH_THROUGHPUT 	pVHTInfo = Adapter->MgntInfo.pVHTInfo;

	// These parameters will be reset when receiving deauthentication packet 
	pVHTInfo->bCurrentVHTSupport = FALSE;

	// LDPC support
	CLEAR_FLAGS(pVHTInfo->VhtCurLdpc);

	// STBC support
	CLEAR_FLAGS(pVHTInfo->VhtCurStbc);

	// Short GI support
	pVHTInfo->bCurShortGI80MHz = FALSE;

	// Sounding support 
	CLEAR_FLAGS(pVHTInfo->VhtCurBeamform);

	// Initialize all of the parameters related to 11n	
	PlatformZeroMemory((PVOID)(&(pVHTInfo->SelfVHTCap)), sizeof(pVHTInfo->SelfVHTCap));
	PlatformZeroMemory((PVOID)(&(pVHTInfo->SelfVHTInfo)), sizeof(pVHTInfo->SelfVHTInfo));
	PlatformZeroMemory((PVOID)(&(pVHTInfo->PeerVHTCapBuf)), sizeof(pVHTInfo->PeerVHTCapBuf));
	PlatformZeroMemory((PVOID)(&(pVHTInfo->PeerVHTInfoBuf)), sizeof(pVHTInfo->PeerVHTInfoBuf));
}


VOID
VHTParsingVHTCapElement(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	VHTCapIE,
	OUT	PRT_WLAN_BSS	pBssDesc
)
{
	if( VHTCapIE.Length > sizeof(pBssDesc->BssVHT.bdVHTCapBuf) )
	{
		RT_TRACE( COMP_HT, DBG_LOUD, ("VHTParsingVHTCapElement(): VHT Capability Element length is too long!\n") );
		return;
	}

	VHTCapIE.Length = VHTCapIE.Length > sizeof(pBssDesc->BssVHT.bdVHTCapBuf)?\
		sizeof(pBssDesc->BssVHT.bdVHTCapBuf):VHTCapIE.Length;	//prevent from overflow
		
	CopyMem(pBssDesc->BssVHT.bdVHTCapBuf, VHTCapIE.Octet, VHTCapIE.Length);
	pBssDesc->BssVHT.bdVHTCapLen = VHTCapIE.Length;

}

VOID
VHTParsingVHTOperationElement(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	VHTOperIE,
	OUT	PRT_WLAN_BSS	pBssDesc
)
{
	if( VHTOperIE.Length > sizeof(pBssDesc->BssVHT.bdVHTOperBuf) )
	{
		RT_TRACE( COMP_HT, DBG_LOUD, ("HTParsingHTCapElement(): VHT Operation Element length is too long!\n") );
		return;
	}

	VHTOperIE.Length = VHTOperIE.Length > sizeof(pBssDesc->BssVHT.bdVHTOperBuf)?\
		sizeof(pBssDesc->BssVHT.bdVHTOperBuf):VHTOperIE.Length;	//prevent from overflow
		
	CopyMem(pBssDesc->BssVHT.bdVHTOperBuf, VHTOperIE.Octet, VHTOperIE.Length);
	pBssDesc->BssVHT.bdVHTOperLen = VHTOperIE.Length;
}


VOID
VHTParsingVHTOpModeNotification(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	VHTNotifIE,
	OUT	PRT_WLAN_BSS	pBssDesc
)
{
	if( VHTNotifIE.Length > sizeof(pBssDesc->BssVHT.bdVHTOpModeNotifBuf) )
	{
		RT_TRACE( COMP_HT, DBG_LOUD, ("VHTParsingVHTOpModeNotification(): VHT Operating Mode Notification length is too long!\n") );
		return;
	}

	VHTNotifIE.Length = VHTNotifIE.Length > sizeof(pBssDesc->BssVHT.bdVHTOpModeNotifBuf)?\
		sizeof(pBssDesc->BssVHT.bdVHTOpModeNotifBuf):VHTNotifIE.Length;	//prevent from overflow
		
	CopyMem(pBssDesc->BssVHT.bdVHTOpModeNotifBuf, VHTNotifIE.Octet, VHTNotifIE.Length);
	pBssDesc->BssVHT.bdVHTOpModeNotifLen = VHTNotifIE.Length;
}


VOID
VHTUseDefaultSettingFromReg(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);

	pVHTInfo->bCurShortGI80MHz= TRUE;

	pVHTInfo->VhtCurLdpc = pVHTInfo->VhtLdpcCap;
	pVHTInfo->VhtCurStbc = pVHTInfo->VhtStbcCap;
	pVHTInfo->VhtCurBeamform = pVHTInfo->VhtBeamformCap;

	pVHTInfo->AMPDU_Len = VHT_AGG_SIZE_1024K;
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_FACTOR, &pVHTInfo->AMPDU_Len);

	PlatformMoveMemory(pMgntInfo->dot11VHTOperationalRateSet, pMgntInfo->Regdot11VHTOperationalRateSet, 2);

	pMgntInfo->VHTHighestOperaRate = pMgntInfo->RegVHTHighestOperaRate;
	
	pMgntInfo->dot11CurrentChannelBandWidth = CHNL_GetRegBWSupport(Adapter);
}

VOID
VHTUseDefaultSettingFromDefault(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);

	PADAPTER					DefAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO					pDefMgntInfo = &DefAdapter->MgntInfo;
	PRT_VERY_HIGH_THROUGHPUT 	pDefVHTInfo = pDefMgntInfo->pVHTInfo;
	
	//=====   The ones that not follow   ======	
	pVHTInfo->bCurShortGI80MHz = TRUE;
	pVHTInfo->VhtCurLdpc = pVHTInfo->VhtLdpcCap;
	pVHTInfo->VhtCurStbc = pVHTInfo->VhtStbcCap;
	pVHTInfo->VhtCurBeamform = pVHTInfo->VhtBeamformCap;

	PlatformMoveMemory(pMgntInfo->dot11VHTOperationalRateSet, pMgntInfo->Regdot11VHTOperationalRateSet, 2);

	pMgntInfo->VHTHighestOperaRate = pMgntInfo->RegVHTHighestOperaRate;
	//==============================	

	pVHTInfo->AMPDU_Len = pDefVHTInfo->AMPDU_Len;
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_FACTOR, &pVHTInfo->AMPDU_Len);

	pMgntInfo->dot11CurrentChannelBandWidth = pDefMgntInfo->dot11CurrentChannelBandWidth;
}
			

VOID
VHTUseDefaultSetting(
	PADAPTER			Adapter
	)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	u1Byte				TwoPortStatus =(u1Byte) TWO_PORT_STATUS__DEFAULT_ONLY;
			
	if(pVHTInfo->bEnableVHT)
	{
		pVHTInfo->bCurrentVHTSupport = TRUE;

		//[Win7] [Vista/XP] Cross Platform Capbility Enable!, 2009.07.31, by Bohn
		GetTwoPortSharedResource(Adapter,TWO_PORT_SHARED_OBJECT__STATUS,NULL,&TwoPortStatus);

		//[win7 Two Port BW40Mhz issue] Extention Port Use Reg's Default Value. 2009.05.20, by bohn
		if(	TwoPortStatus==TWO_PORT_STATUS__DEFAULT_ONLY 		||
			TwoPortStatus==TWO_PORT_STATUS__EXTENSION_ONLY	||
			TwoPortStatus==TWO_PORT_STATUS__WITHOUT_ANY_ASSOCIATE)		
		{
			VHTUseDefaultSettingFromReg(Adapter);		
		}
		else if(TwoPortStatus==TWO_PORT_STATUS__EXTENSION_FOLLOW_DEFAULT)
		{	
			//[win7 Two Port BW40Mhz issue] Extention Port obey the rule of Default Port. 2009.05.20, by bohn
			VHTUseDefaultSettingFromDefault(Adapter);
		}
		else if(	TwoPortStatus==TWO_PORT_STATUS__DEFAULT_G_EXTENSION_N20	||
				TwoPortStatus==TWO_PORT_STATUS__DEFAULT_A_EXTENSION_N20	||
				TwoPortStatus==TWO_PORT_STATUS__ADHOC)
		{
			//[Win7] Now is the Default-G Extension-N20 case, we need to reset all HT value and forced to N20
			// 2009.07.09, by Bohn
			VHTUseDefaultSettingFromReg(Adapter);
		}
	}
	else
	{
		pVHTInfo->bCurrentVHTSupport = FALSE;
	}
}

/*
*	Description:
*		This function will get the highest speed rate in input MCS set.
*
*	/param 	Adapter			Pionter to Adapter entity
*			pMCSRateSet		Pointer to MCS rate bitmap
*			pMCSFilter		Pointer to MCS rate filter
*	
*	/return	Highest MCS rate included in pMCSRateSet and filtered by pMCSFilter.
*
*/
u1Byte
VHTGetHighestMCSRate(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pVHTMCSRateSet
	)
{
	u1Byte		i, j;
	u1Byte		bitMap;
	u1Byte		VHTMcsRate = 0;
	
	for(i = 0; i < 2; i++)
	{
		if(pVHTMCSRateSet[i] != 0xff)
		{
			for(j = 0; j < 8; j += 2)
			{
				bitMap = (pVHTMCSRateSet[i] >> j) & 3;
				
				if(bitMap != 3)
					VHTMcsRate = MGN_VHT1SS_MCS7 + 10*j/2 + i*40 + bitMap;  //VHT rate indications begin from 0x90
			}
		}
	}
	return VHTMcsRate;
}


VOID
VHTCheckVHTCap(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry,
	IN	pu1Byte			pVHTCap
)
{
	PMGNT_INFO					pMgntInfo = &(Adapter->MgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);

	u1Byte		VHtSTBC = 0;

	// B2 B3 Supported Channel Width Set
	if(CHNL_RUN_ABOVE_80MHZ(pMgntInfo) && pEntry->BandWidth == CHANNEL_WIDTH_40)
		pEntry->BandWidth = CHANNEL_WIDTH_80;

	// B4 Rx LDPC
	if(TEST_FLAG(pVHTInfo->VhtLdpcCap, LDPC_VHT_ENABLE_TX))
		SET_FLAG(pEntry->VHTInfo.LDPC, GET_VHT_CAPABILITY_ELE_RX_LDPC(pVHTCap) ? (LDPC_VHT_ENABLE_TX | LDPC_VHT_CAP_TX) : 0);

	// B5 Short GI for 80 MHz
	pEntry->VHTInfo.bShortGI80M = GET_VHT_CAPABILITY_ELE_SHORT_GI80M(pVHTCap);

	// B8 B9 B10 Rx STBC
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_TX_STBC, (pu1Byte)&VHtSTBC);
	if(!TEST_FLAG(pVHTInfo->VhtStbcCap, STBC_VHT_ENABLE_TX))
		VHtSTBC = 0;

	if(VHtSTBC < GET_HT_CAPABILITY_ELE_RX_STBC(pVHTCap))
		pEntry->VHTInfo.STBC = VHtSTBC;
	else
		pEntry->VHTInfo.STBC = GET_HT_CAPABILITY_ELE_RX_STBC(pVHTCap);

	CLEAR_FLAGS(pEntry->VHTInfo.VhtCurBeamform);
	// B11 SU Beamformer Capable, we are Beamformee
	if(	TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_BEAMFORMEE_ENABLE) && 
		GET_VHT_CAPABILITY_ELE_SU_BFER(pVHTCap))
	{
		SET_FLAG(pEntry->VHTInfo.VhtCurBeamform, BEAMFORMING_VHT_BEAMFORMER_ENABLE);
		// Shift to BEAMFORMING_VHT_BEAMFORMER_STS_CAP
		SET_FLAG(pVHTInfo->VhtCurBeamform, GET_VHT_CAPABILITY_ELE_SU_BFEE_STS_CAP(pVHTCap)<<8);
	}

	// B12 SU Beamformee Capable, we are Beamformer
	if(	TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_BEAMFORMER_ENABLE) && 
		GET_VHT_CAPABILITY_ELE_SU_BFEE(pVHTCap))
	{
		SET_FLAG(pEntry->VHTInfo.VhtCurBeamform, BEAMFORMING_VHT_BEAMFORMEE_ENABLE);
		// Shit to BEAMFORMING_VHT_BEAMFORMEE_SOUND_DIM
		SET_FLAG(pVHTInfo->VhtCurBeamform, GET_VHT_CAPABILITY_ELE_SU_BFER_SOUND_DIM_NUM(pVHTCap)<<12);
	}

	// B19 MU Beamformer Capable, we are Beamformee
	if(	TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_MU_MIMO_STA_ENABLE) && 
		GET_VHT_CAPABILITY_ELE_MU_BFER(pVHTCap))
	{
		SET_FLAG(pEntry->VHTInfo.VhtCurBeamform, BEAMFORMING_VHT_MU_MIMO_AP_ENABLE);
		// Shift to BEAMFORMING_VHT_BEAMFORMER_STS_CAP
		SET_FLAG(pVHTInfo->VhtCurBeamform, GET_VHT_CAPABILITY_ELE_SU_BFEE_STS_CAP(pVHTCap)<<8);
	}

	// B20 MU Beamformee Capable, we are Beamformer
	if(	TEST_FLAG(pVHTInfo->VhtBeamformCap, BEAMFORMING_VHT_MU_MIMO_AP_ENABLE) && 
		GET_VHT_CAPABILITY_ELE_MU_BFEE(pVHTCap))
	{
		SET_FLAG(pEntry->VHTInfo.VhtCurBeamform, BEAMFORMING_VHT_MU_MIMO_STA_ENABLE);
		// Shit to BEAMFORMING_VHT_BEAMFORMEE_SOUND_DIM
		SET_FLAG(pVHTInfo->VhtCurBeamform, GET_VHT_CAPABILITY_ELE_SU_BFER_SOUND_DIM_NUM(pVHTCap)<<12);
	}
	
	RT_DISP(FBEAM, FBEAM_FUN, ("Client VHT Beaforming Cap = 0x%02X\n", pEntry->VHTInfo.VhtCurBeamform));

	VHTFilterMCSRate(Adapter, GET_VHT_CAPABILITY_ELE_RX_MCS(pVHTCap), pEntry->VHTInfo.VHTRateSet);
}


/***********************************************************
Function: Recovery to Initialize  value
Auther: sherry
Date: 20111117
************************************************************/
VOID 
VHTInitializeBssDesc(
	IN	PBSS_VHT		pBssVHT
	)
{
	pBssVHT->bdSupportVHT = FALSE;
	PlatformZeroMemory(pBssVHT->bdVHTCapBuf, sizeof(pBssVHT->bdVHTCapBuf));
	pBssVHT->bdVHTCapLen = 0;
	PlatformZeroMemory(pBssVHT->bdVHTOperBuf, sizeof(pBssVHT->bdVHTOperBuf));
	pBssVHT->bdVHTOperLen = 0;
}


BOOLEAN
VHTModeSupport(
	PADAPTER			Adapter,
	POCTET_STRING		pSRCmmpdu
)
{
	OCTET_STRING	VHTCapIE, mmpdu;

	mmpdu.Octet = pSRCmmpdu->Octet;
	mmpdu.Length = pSRCmmpdu->Length;
	// Get VHT Capability IE:
	VHTCapIE = PacketGetElement(mmpdu, EID_VHTCapability, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(VHTCapIE.Length != 0)
	{
		return	TRUE;
	}
	else
		return	FALSE;
}


/***********************************************************
Function: To parsing VHT Related IE from Beacon or ProbeRsp
Auther: Page and sherry
Date: 20111117
************************************************************/
VOID
VHTGetValueFromBeaconOrProbeRsp(
	PADAPTER			Adapter,
	POCTET_STRING		pSRCmmpdu,
	PRT_WLAN_BSS		bssDesc
)
{
	OCTET_STRING	VHTCapIE, VHTOperIE, VHTNotifIE, mmpdu;

	mmpdu.Octet = pSRCmmpdu->Octet;
	mmpdu.Length = pSRCmmpdu->Length;

	//Initialize VHT BSSDesc
	VHTInitializeBssDesc(&bssDesc->BssVHT);

	if(Adapter->bInHctTest)
		return;
	
	// Get VHT Capability IE
	VHTCapIE = PacketGetElement(mmpdu, EID_VHTCapability, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	
	if(VHTCapIE.Length != 0)
		VHTParsingVHTCapElement(Adapter, VHTCapIE, bssDesc);
	else if(Adapter->MgntInfo.bRegIOTBcm256QAM)
	{
		VHTCapIE = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_11AC_EPIG_VHT_CAP, OUI_SUBTYPE_DONT_CARE);
		
		if(VHTCapIE.Length != 0)
		{
			VHTCapIE.Octet += 7;
			VHTCapIE.Length = MAX_VHT_CAP_LEN;
			VHTParsingVHTCapElement(Adapter, VHTCapIE, bssDesc);
		}
	}
	
	RT_DISP(FBEACON, BCN_SHOW, 	("EID_VHTCapability VHTCapIE.Length=%d\r\n", VHTCapIE.Length));

	// Get VHT Opration IE
	VHTOperIE = PacketGetElement(mmpdu, EID_VHTOperation, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	
	if(VHTOperIE.Length != 0)
		VHTParsingVHTOperationElement(Adapter, VHTOperIE, bssDesc);
	else if(Adapter->MgntInfo.bRegIOTBcm256QAM)
	{
		OCTET_STRING	EpigIE = PacketGetElement(mmpdu, EID_Vendor, OUI_SUB_EPIG_IE, OUI_SUBTYPE_DONT_CARE);
		if(EpigIE.Length >= 26)
		{
			VHTOperIE.Octet = EpigIE.Octet+21;
			VHTOperIE.Length = 5;
		}
		
		if(VHTOperIE.Length != 0)
			VHTParsingVHTOperationElement(Adapter, VHTOperIE, bssDesc);
	}

	if(VHTCapIE.Length != 0)
	{
		bssDesc->BssVHT.bdSupportVHT = TRUE;
	}	

	// Operating Mode Notification
	VHTNotifIE = PacketGetElement(mmpdu, EID_OpModeNotification, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	if(VHTNotifIE.Length != 0)
		VHTParsingVHTOpModeNotification(Adapter, VHTNotifIE, bssDesc);

	//2<4>Check peer configuration
	if(VHTOperIE.Length!=0 && VHTOperIE.Octet != NULL)
	{
		CHANNEL_WIDTH	ChnlBW = (CHANNEL_WIDTH)(GET_VHT_OPERATION_ELE_CHL_WIDTH(VHTOperIE.Octet));
		if(ChnlBW == 1)
			bssDesc->bdBandWidth = CHANNEL_WIDTH_80;
		else if(ChnlBW == 2)
			bssDesc->bdBandWidth = CHANNEL_WIDTH_160;
		else if(ChnlBW == 3)
			bssDesc->bdBandWidth = CHANNEL_WIDTH_80_80;
	}
}

/*************************************************************************
Fuction: To Restore VHT setting
Author: Sherry
Date: 20111117
*************************************************************************/
VOID
VHTResetSelfAndSavePeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc
)
{

	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);

	//
	//  Save Peer Setting before Association
	//
	if( pVHTInfo->bEnableVHT &&  pBssDesc->BssVHT.bdSupportVHT)
	{
		pVHTInfo->bCurrentVHTSupport = TRUE;

		// Save VHT Cap and VHT Operation information Element
		if(pBssDesc->BssVHT.bdVHTCapLen > 0 && 	pBssDesc->BssVHT.bdVHTCapLen <= sizeof(pVHTInfo->PeerVHTCapBuf))
			CopyMem(pVHTInfo->PeerVHTCapBuf, pBssDesc->BssVHT.bdVHTCapBuf, pBssDesc->BssVHT.bdVHTCapLen);

		if(pBssDesc->BssVHT.bdVHTOperLen> 0 && pBssDesc->BssVHT.bdVHTOperLen <= sizeof(pVHTInfo->PeerVHTInfoBuf))
			CopyMem(pVHTInfo->PeerVHTInfoBuf,pBssDesc->BssVHT.bdVHTOperBuf, pBssDesc->BssVHT.bdVHTOperLen);
		
		if(pBssDesc->BssVHT.bdVHTOperLen != 0)
			pVHTInfo->PeerChnlBW = (CHANNEL_WIDTH)GET_VHT_OPERATION_ELE_CHL_WIDTH(pBssDesc->BssVHT.bdVHTOperBuf);
		else
			pVHTInfo->PeerChnlBW = (CHANNEL_WIDTH)CHANNEL_WIDTH_20;
				
		pMgntInfo->IOTAction |= VHTIOTActIsAMsduEnable(Adapter);
	}
	else
	{
		pVHTInfo->bCurrentVHTSupport = FALSE;
		pVHTInfo->PeerChnlBW = CHANNEL_WIDTH_20;
	}
}


VOID
VHTUpdateSelfAndPeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc
	)
{
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);

	if(pVHTInfo->bCurrentVHTSupport)
	{
		if(pBssDesc->BssVHT.bdVHTOperLen != 0)
			pVHTInfo->PeerChnlBW = (CHANNEL_WIDTH)GET_VHT_OPERATION_ELE_CHL_WIDTH(pBssDesc->BssVHT.bdVHTOperBuf);
		
		if(pBssDesc->BssVHT.bdVHTOpModeNotifLen > 0)
		{
			CHANNEL_WIDTH		chnlBW = CHANNEL_WIDTH_20;
			u1Byte				targetrxSS, currentrxSS;
			BOOLEAN				bUpdateRA = FALSE;

			chnlBW = (CHANNEL_WIDTH)GET_VHT_OPERATING_MODE_FIELD_CHNL_WIDTH(pBssDesc->BssVHT.bdVHTOpModeNotifBuf);
			targetrxSS = (GET_VHT_OPERATING_MODE_FIELD_RX_NSS(pBssDesc->BssVHT.bdVHTOpModeNotifBuf) + 1);
			RT_TRACE(COMP_MLME, DBG_LOUD, ("VHTUpdateSelfAndPeerSetting(): bw = %d, ss = %d\n", chnlBW, targetrxSS));

			if(chnlBW != pMgntInfo->currentRABw)
			{
				RT_TRACE(COMP_HT, DBG_LOUD, ("VHTUpdateSelfAndPeerSetting(): operating mode notification change bandwidth\n"));				
				bUpdateRA = TRUE;
			}

			currentrxSS = RateToSpatialStream(pMgntInfo->dot11VHTOperationalRateSet);
			if(targetrxSS != currentrxSS )
			{
				RT_TRACE(COMP_HT, DBG_LOUD, ("VHTUpdateSelfAndPeerSetting(): operating mode notification change tx spatial stream\n"));

				SpatialStreamToRate(Adapter, targetrxSS, pMgntInfo->dot11VHTOperationalRateSet);

				pMgntInfo->Ratr_State = 0;
				Adapter->HalFunc.UpdateHalRAMaskHandler(Adapter, pMgntInfo->mMacId, NULL, pMgntInfo->Ratr_State);

			}
		}
	}
}


//
// Description: 
//	Update VHT Rate mask Regdot11OperationalRateSet.
//
// Arguments:
//	[in] pVHTRate -
//		Pointer of Regdot11OperationalRateSet
//	[in] maxSS -
//		Maximum spatial stream that will be modified
//	[in] mcsCap -
//		MCS capability of each spatial 0:0-7/1:0-8/2:0-9
//
VOID
VHTMaskSuppDataRate(
	pu1Byte			pVHTRate,
	u1Byte			maxSS,
	u1Byte			mcsCap
	)
{
	u1Byte	i=0, j=0, nSS = 0;
	u1Byte	RegRate = 0;

	for (i=0; i<2; i++)
	{
		for(j = 0; j < 8; j+=2)
		{
			RegRate = (pVHTRate[i] >> j) & 3;

			if(nSS < maxSS)
			{
				if(RegRate != 3 && mcsCap <= RegRate)
					pVHTRate[i] = (pVHTRate[i] & ~(0x03 << j) ) | (mcsCap << j);
			}
			else
			{
				return;
			}
			nSS++;
		}
	}
}


#else
u2Byte
VHTMcsToDataRate(
	PADAPTER		Adapter,
	u2Byte			nMcsRate
)
{
	return 0;
}


VOID
VHTConstructCapabilityElement(
	PADAPTER			Adapter,
	POCTET_STRING		posVHTInfo,
	BOOLEAN				bAssoc)
{}

VOID
VHTConstructOperationElement(
	PADAPTER		Adapter,
	POCTET_STRING	posVHTOperation
)
{}


u1Byte 
VHTIOTActIsAMsduEnable(
	PADAPTER		Adapter
)
{return 0;}


VOID
VHTSendOperatingModeNotification(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr
	)
{}

RT_STATUS
VHT_OnOpModeNotify(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	return RT_STATUS_SUCCESS;
}

u1Byte
VHTGetHighestMCSRate(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pMCSRateSet
)
{
	return 0;
}

VOID
VHTCheckVHTCap(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry,
	IN	pu1Byte			pVHTCap
)
{
}

VOID
VHTOnAssocRsp(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	asocpdu
)
{}

VOID
VHTInitializeVHTInfo(
	PADAPTER	Adapter)
{}


VOID
VHTGetValueFromBeaconOrProbeRsp(
	PADAPTER			Adapter,
	POCTET_STRING		pSRCmmpdu,
	PRT_WLAN_BSS		bssDesc
)
{}


VOID
VHTResetSelfAndSavePeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc
)
{}


VOID
VHTUseDefaultSetting(
	PADAPTER			Adapter
)
{}


VOID
VHTUpdateSelfAndPeerSetting(
	PADAPTER			Adapter,
	PRT_WLAN_BSS		pBssDesc
)
{}

VOID
VHTMaskSuppDataRate(
	pu1Byte			pVHTRate,
	u1Byte			maxSS,
	u1Byte			mcsCap
	)
{
}

BOOLEAN
VHTModeSupport(
	PADAPTER			Adapter,
	POCTET_STRING		pSRCmmpdu
)
{
	return FALSE;
}

#endif

