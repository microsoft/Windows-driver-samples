#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "QosGen.tmh"
#endif

//=============================================================================
//	Public function for Debugging Qos.
//=============================================================================
#if DBG
//
//	Description:
//		Dump the TSPEC IE content.
//
VOID
QosParsingDebug_TspecIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer
	)
{
	pu1Byte pIe = pOsBuffer->Octet;
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("======= TSPEC IE =======\n"));

	RT_TRACE( COMP_QOS, DBG_LOUD, ("TrafficType:  %d\n", GET_TSPEC_TSINFO_TRAFFIC_TYPE(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("TSID:  %d\n", GET_TSPEC_TSINFO_TSID(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("Direction:  %d\n", GET_TSPEC_TSINFO_DIRECTION(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("AccessPolicy:  %d\n", GET_TSPEC_TSINFO_ACCESS_POLICY(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("Aggregation:  %d\n", GET_TSPEC_TSINFO_AGGREGATION(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("PSB:  %d\n", GET_TSPEC_TSINFO_PSB(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("UP:  %d\n", GET_TSPEC_TSINFO_UP(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("AckPolicy:  %d\n", GET_TSPEC_TSINFO_ACK_POLICY(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("Schedule:  %d\n", GET_TSPEC_TSINFO_SCHEDULE(pIe) ));
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("NominalMsduSize:  %d\n", GET_TSPEC_NOMINAL_MSDU_SIZE(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("MaxMsduSize:  %d\n", GET_TSPEC_MAX_MSDU_SIZE(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("MinServiceInterval:  %d\n", GET_TSPEC_MIN_SERVICE_INTERVAL(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("MaxServiceInterval:  %d\n", GET_TSPEC_MAX_SERVICE_INTERVAL(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("InactivityInterval:  %d\n", GET_TSPEC_INACTIVITY_INTERVAL(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("SupensionInterval:  %d\n", GET_TSPEC_SUSPENSION_INTERVAL(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("ServiceStartTime:  %d\n", GET_TSPEC_SERVICE_START_TIME(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("MinDataRate:  %d\n", GET_TSPEC_MIN_DATA_RATE(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("MeanDataRate:  %d\n", GET_TSPEC_MEAN_DATA_RATE(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("PeakDataRate:  %d\n", GET_TSPEC_PEAK_DATA_RATE(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("MaxBurstSize:  %d\n", GET_TSPEC_MAX_BURST_SIZE(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("DelayBound:  %d\n", GET_TSPEC_DELAY_BOUND(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("MinPhyRate:  %d\n", GET_TSPEC_MIN_PHY_RATE(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("SurplusBandwith:  %d\n", GET_TSPEC_SURPLUS_BANDWITH_ALLOWANCE(pIe) ));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("MediumTime:  %d\n", GET_TSPEC_MEDIUM_TIME(pIe) ));
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("===== end of TSPEC IE =====\n"));
}

//
//	Description:
//		Dump the TSRS IE content
//
VOID
QosParsingDebug_TsrsIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer
	)
{

	u1Byte	TSID;
	u1Byte	Length;
	u1Byte	i;
	u1Byte	rate;
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("======= TSRS IE =======\n"));

	Length = (*(pOsBuffer->Octet + 1)) - 5;
	TSID = *(pOsBuffer->Octet + 6);
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("TSID:  %d\n", TSID));
	for (i = 0; i < Length; i ++)
	{
		rate = *(pOsBuffer->Octet + 7 + i);
		RT_TRACE( COMP_QOS, DBG_LOUD, ("Rate:  0x%x\n", rate));
	}
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("===== end of TSRS IE =====\n"));

}

//
//	Description:
//		Dump the MSDU Lifetime IE content
//
VOID
QosParsingDebug_MsduLifetimeIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer
	)
{
	u1Byte	TSID;
	u2Byte	MsduLifetime;
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("======= MSDU Lifetime IE =======\n"));

	TSID = *(pOsBuffer->Octet+6);
	MsduLifetime = N2H2BYTE( *((UNALIGNED pu2Byte)(pOsBuffer->Octet+7)) );

	RT_TRACE( COMP_QOS, DBG_LOUD, ("TSID:  %d\n", TSID));
	RT_TRACE( COMP_QOS, DBG_LOUD, ("MsduLifetime:  %d\n", MsduLifetime));	
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("===== end of MSDU Lifetime IE =====\n"));

}

#endif

//
// Initialize QoS parameters.
// Ref: RTL8185B_InitQoSPara() in 8185 QoS code.
// 
VOID 
QosInitializeSTA(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;

	u1Byte	szQoSOUI[] = {0x00, 0x50, 0xf2, 0x02};
	
	pStaQos->CurrentQosMode = QOS_DISABLE;

	pStaQos->bInServicePeriod = FALSE; //[APSD] Isaiah 2006-07-24
	PlatformZeroMemory( pStaQos->WMMIEBuf, sizeof(pStaQos->WMMIEBuf) );
	pStaQos->WMMIE.Octet = pStaQos->WMMIEBuf;
	pStaQos->WMMIE.Length = 0;
	pStaQos->pWMMInfoEle = pStaQos->WMMIEBuf;
	
	// Default OUI & type: 00-50-F2-02
//	CopyMem( pStaQos->WMMIE.Octet, szQoSOUI, 4 );
	PlatformMoveMemory(pStaQos->WMMIE.Octet, szQoSOUI, 4);
	SET_WMM_INFO_ELE_OUI_SUBTYPE(pStaQos->pWMMInfoEle, 0x00); // WMM-IE
	SET_WMM_INFO_ELE_VERSION(pStaQos->pWMMInfoEle, 0x01);
	SET_WMM_INFO_ELE_QOS_INFO_FIELD(pStaQos->pWMMInfoEle,0);

	pStaQos->WMMIE.Length = 7;
	
	// Decide ACM method. Annie, 2005-12-13.
	pStaQos->AcmMethod = eAcmWay2_SW;

	// Reset TStream related, 070614, by rcnjko.
	QosResetAllTs(Adapter);

	RT_TRACE( COMP_QOS, DBG_LOUD, ("QosInitializeSTA(): pStaQos->CurrentQosMode = %d\n", pStaQos->CurrentQosMode ) );
}

//
// Deinitialize all resources allocated at QosInitializeSTA 
//
VOID 
QosDeinitializeSTA(
	IN	PADAPTER		Adapter
	)
{
	QosRemoveAllTs(Adapter);
}



VOID 
QosInitializeBssDesc(
	IN	PBSS_QOS		pBssQos
	)
{
	//RT_TRACE( COMP_QOS, DBG_LOUD, ("QosInitializeBssDesc()\n") );

	pBssQos->bdQoSMode = QOS_DISABLE;

	PlatformZeroMemory( pBssQos->bdWMMIEBuf, sizeof(pBssQos->bdWMMIEBuf) );
	pBssQos->bdWMMIE.Octet = pBssQos->bdWMMIEBuf;
	pBssQos->bdWMMIE.Length = 0;

	pBssQos->EleSubType = QOSELE_TYPE_INFO;
	pBssQos->pWMMInfoEle = NULL;
	pBssQos->pWMMParamEle = NULL;
}


//
// Parsing WMM Information element or parameter element.
// Ref: RTL8185B_InitQoSPara() in 8185 QoS code.
// 
VOID 
QosParsingQoSElement(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			bEDCAParms,
	IN	OCTET_STRING	WMMElement,
	OUT	PRT_WLAN_BSS	pBssDesc
	)
{
	u1Byte	qosinfo = 0;

	if( WMMElement.Length > MAX_WMMELE_LENGTH )
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("QosParsingWMMElement(): WMM Element length is too long!\n") );
		return;
	}

	pBssDesc->BssQos.bdQoSMode |= QOS_WMM; // [Isaiah] when to pBssDesc->BssQos.bdQoSMode &= ~QOS_WMM ??


	if(WMMElement.Length >0)
	{
		// AC parameter maybe disorder
		CopyMem( pBssDesc->BssQos.bdWMMIEBuf, WMMElement.Octet, WMMElement.Length );
		FillOctetString( pBssDesc->BssQos.bdWMMIE, pBssDesc->BssQos.bdWMMIEBuf, WMMElement.Length );
	}

	if(bEDCAParms)
		pBssDesc->BssQos.bdQoSMode |= QOS_EDCA;

	PlatformZeroMemory(&qosinfo, sizeof(qosinfo));

	if(WMMElement.Octet != NULL)
		pBssDesc->BssQos.EleSubType = (QOS_ELE_SUBTYPE)EF1Byte(WMMElement.Octet[4]);

	switch( pBssDesc->BssQos.EleSubType )
	{
		case QOSELE_TYPE_INFO:		
			pBssDesc->BssQos.pWMMInfoEle = pBssDesc->BssQos.bdWMMIE.Octet;
			qosinfo= GET_WMM_INFO_ELE_QOS_INFO_FIELD(pBssDesc->BssQos.pWMMInfoEle);
			break;

		case QOSELE_TYPE_PARAM:
			pBssDesc->BssQos.pWMMParamEle = pBssDesc->BssQos.bdWMMIE.Octet;
			qosinfo= GET_WMM_PARAM_ELE_QOS_INFO_FIELD(pBssDesc->BssQos.pWMMParamEle);
			break;

		default:
			break;
	}


	//[APSD] Indicate QosMode whether WMM AP supports WMM Power Save or not.  
	if(Adapter->MgntInfo.pStaQos->QosCapability & QOS_WMM_UAPSD)
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("[APSD] qosinfo=%x\n", qosinfo));
		if(qosinfo & 0x80 ) //U_APSD (bit 7)
			pBssDesc->BssQos.bdQoSMode |= QOS_WMM_UAPSD;
		else
			pBssDesc->BssQos.bdQoSMode &=~ QOS_WMM_UAPSD;
	}


	//RT_TRACE( COMP_QOS, DBG_LOUD, ("[WMM] AP bdQoSMode=%x\n", pBssDesc->BssQos.bdQoSMode));
	
	// For debugging ONLY. Annie, 2005-11-12.
	//QosParsingDebug_BssDesc( pBssDesc );

}


VOID 
QosSetLegacyWMMParamWithHT(
	IN	PADAPTER		Adapter,
	OUT	PRT_WLAN_BSS	pBssDesc
	)
{
	// [AnnieWorkaround] Is this OK? 2005-11-12.
	pBssDesc->BssQos.bdQoSMode |= QOS_WMM; // [Isaiah] when to pBssDesc->BssQos.bdQoSMode &= ~QOS_WMM ??

	pBssDesc->BssQos.bdQoSMode &=~ QOS_WMM_UAPSD;
	
	FillOctetString(pBssDesc->BssQos.bdWMMIE, pBssDesc->BssQos.bdWMMIEBuf, WMM_PARAM_ELEMENT_SIZE);
	pBssDesc->BssQos.pWMMParamEle = pBssDesc->BssQos.bdWMMIE.Octet;
	SET_WMM_PARAM_ELE_AC_PARAMS(pBssDesc->BssQos.pWMMParamEle, Adapter->STA_EDCA_PARAM);

	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("[WMM] QosSetLegacyWMMParamWithHT(): AP bdQoSMode=%x\n", pBssDesc->BssQos.bdQoSMode));
	
	// For debugging ONLY. Annie, 2005-11-12.
	// QosParsingDebug_BssDesc( pBssDesc );

}



//
//
//
VOID
QosSetLegacyACParam(
	IN	PADAPTER	Adapter
	)
{
	// [Win7] pChnlAccessSetting use the common part for both adapters. 2009.07.02, by Bohn
	PCHANNEL_ACCESS_SETTING pChnlAccessSetting = &(GetDefaultMgntInfo(Adapter)->ChannelAccessSetting);	
	AC_CODING	eACI;
	u4Byte		AcParam=0;
	pu1Byte		pTmp_WMMParamEle=NULL;

	//
	// 1. Follow 802.11 seeting to AC parameter, all AC shall use the same parameter.
	// 2. Note that, 85B's throughput with Cisco1231 and Broadcom testbed are less than 20M, so we change the contention window from 0xA5 to 0x73.
	//     With CW=0x73, the throughput are 21M~22M.
	//     If there in any side event with 0x73, we should test 0xA4 first (following spec). Annie, 2006-04-04.
	//		
	SET_WMM_AC_PARAM_AIFSN(&AcParam, 2);	// Follow 802.11 DIFS.
	SET_WMM_AC_PARAM_ACM(&AcParam, 0);
	SET_WMM_AC_PARAM_ECWMIN(&AcParam, pChnlAccessSetting->CWminIndex);
	SET_WMM_AC_PARAM_ECWMAX(&AcParam, pChnlAccessSetting->CWmaxIndex);
	SET_WMM_AC_PARAM_TXOP_LIMIT(&AcParam, 0);
	

	pTmp_WMMParamEle = (pu1Byte)GetTwoPortSharedResource(Adapter,TWO_PORT_SHARED_OBJECT__SET_OF_pStaQos_WMMParamEle
		,Adapter->MgntInfo.pStaQos->WMMParamEle,NULL);
	
	RT_ASSERT(pTmp_WMMParamEle != NULL, ("QosSetLegacyACParam(): Get Null WMMParamEle.\n"));

	for(eACI = 0; eACI < AC_MAX; eACI++)
	{
		if(eACI == AC3_VO)
		{ 
			// For Wifi-Direct Device Discovery: MgntQueue should be fast ----------------------------------------
			u4Byte		VOQueueAcParam = 0;
			SET_WMM_AC_PARAM_AIFSN(&VOQueueAcParam, 0);
			SET_WMM_AC_PARAM_ACM(&VOQueueAcParam, 0);
			SET_WMM_AC_PARAM_ECWMIN(&VOQueueAcParam, 2);
			SET_WMM_AC_PARAM_ECWMAX(&VOQueueAcParam, 3);
			SET_WMM_AC_PARAM_TXOP_LIMIT(&VOQueueAcParam, (u1Byte) 0x2f);

			SET_WMM_AC_PARAM_ACI(&VOQueueAcParam, (u1Byte)eACI );
			SET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pTmp_WMMParamEle, eACI, &VOQueueAcParam);
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, (pu1Byte)(&VOQueueAcParam));
			// -------------------------------------------------------------------------------------------
		}
		else
		{
		SET_WMM_AC_PARAM_ACI(&AcParam, (u1Byte)eACI );
		SET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pTmp_WMMParamEle, eACI, &AcParam);
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, (pu1Byte)(&AcParam));
	}
}
}


//
// Parsing WMM Information element or parameter element.
// Ref: QOS_OnAssociationResponse() in 8185 QoS code:
// TODO:
// 	(1)Config QoS mode, MSR reister for EDCA/HCCA mode
// 	(2) Config ACM reqirement	
// 		(2.1) Config driver parameter for ACM control
// 		(2.2) Configure WME parameter register
// 		(2.3) Configure WiFi mode ACM
// 	(3) Config Queue Size Request Mode
// 	(4) Turn on accumulated duration field if required
//	(5)Create AddTspec action management frame
// 
VOID
QosOnAssocRsp(
	IN	PADAPTER		Adapter,
	IN	OCTET_STRING	asocpdu
)
{
	OCTET_STRING	WMMParaEle;
	PSTA_QOS		pStaQos = Adapter->MgntInfo.pStaQos;
	u1Byte			index;
	pu1Byte			AcParamPtr;
	AC_CODING		eACI = 0;

	RT_TRACE( COMP_QOS, DBG_LOUD, ("===> QosOnAssocRsp()\n") );

	if( pStaQos->CurrentQosMode == QOS_DISABLE )
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("<=== QosOnAssocRsp(): QOS_DISABLE\n") );
		return;
	}

	if( asocpdu.Length == 0 )
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("<=== QosOnAssocRsp(): No input frame!!\n") );
		return;
	}

	WMMParaEle = PacketGetElement( asocpdu, EID_Vendor, OUI_SUB_WMM, OUI_SUBTYPE_WMM_PARAM);

	if(WMMParaEle.Length ==0 ||  WMMParaEle.Length > MAX_WMMELE_LENGTH )
	{
		// There is no valid WMM parameter!! Using WMM parameter we retrieved from BEACON.
		if(pStaQos->WMMIE.Length<=8)
			SET_WMM_PARAM_ELE_AC_PARAMS(pStaQos->WMMParamEle, Adapter->STA_EDCA_PARAM);

		FillOctetString(WMMParaEle, pStaQos->WMMParamEle, WMM_PARAM_ELEMENT_SIZE);
	}
	
	//copy OUI ~ Reserved (8 bytes), because Joseph say AC Parameters may disorder.
	//They would store later.
	CopyMem( pStaQos->WMMParamEle, WMMParaEle.Octet, 8 );

	// For debugging ONLY. Annie, 2005-11-12.
	QosParsingDebug_STA( Adapter );

	// Update AC parameters to HW.
	for(index = 0; index < AC_MAX; index++)
	{
		AcParamPtr = GET_WMM_PARAM_ELE_SINGLE_AC_PARAM(WMMParaEle.Octet, index);
		if(AcParamPtr != NULL)
			eACI = GET_WMM_AC_PARAM_ACI(AcParamPtr); 
	
		// Filter out ACI and ACM which is not related to EDCA parameters and check if the
		// value is all NULL. If the value is all NULL, we shall use EDCA default value.
		//Prefast warning C28182: Dereferencing NULL pointer. 'AcParamPtr' contains the same NULL value as 'WMMParaEle.Octet' did. 
		if(AcParamPtr != NULL &&
			EF4Byte( *((UNALIGNED pu4Byte)AcParamPtr) & 0xffffff0f)==0)
			SET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pStaQos->WMMParamEle, eACI, &(Adapter->STA_EDCA_PARAM[eACI]) );
		else
			SET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pStaQos->WMMParamEle, eACI, AcParamPtr);

		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, GET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pStaQos->WMMParamEle, eACI));

		pStaQos->acm[eACI].UsedTime = 0;
		pStaQos->acm[eACI].MediumTime = 0;
		pStaQos->acm[eACI].HwAcmCtl = FALSE;
	}


	// TODO: ACM is not yet implemented.

	RT_TRACE( COMP_QOS, DBG_LOUD, ("<=== QosOnAssocRsp()\n") );
}


//
// Update AC parameter and hardware registers when QoS Info field (parameter set count) changed.
// Added by Annie, 2005-12-06.
//
VOID
QosOnBeaconUpdateParameter(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pBssDesc
)
{
	PSTA_QOS		pStaQos = Adapter->MgntInfo.pStaQos;

	RT_ASSERT( pStaQos->QosCapability, ("QosOnBeaconUpdateParameter(): QosCapability == WMM_Disable!!!\n"));	

	if(pBssDesc->BssQos.EleSubType != QOSELE_TYPE_PARAM)
		return;
	else	
	{

		pu1Byte			pWMMPE_STA= pStaQos->WMMParamEle;
		u1Byte			index = 0;
		pu1Byte			staAcParamPtr;
		pu1Byte			bssAcParamPtr;
		u4Byte			staAcParam;
		u4Byte			bssAcParam;
		u1Byte			AcIdx;

		for(index=0; index<4; index++)
		{
			bssAcParamPtr = GET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pBssDesc->BssQos.pWMMParamEle, index);
			AcIdx = GET_WMM_AC_PARAM_ACI(bssAcParamPtr);
		
			staAcParamPtr = GET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pWMMPE_STA, AcIdx);
		
			staAcParam = EF4Byte(*((UNALIGNED pu4Byte)staAcParamPtr));
			bssAcParam = EF4Byte(*((UNALIGNED pu4Byte)bssAcParamPtr));

			if((staAcParam != bssAcParam) && ((bssAcParam & 0xffffff0f)!=0))
			{
				// Update EDCA parameters.
				SET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pWMMPE_STA, AcIdx, bssAcParamPtr);
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AC_PARAM, GET_WMM_PARAM_ELE_SINGLE_AC_PARAM(pWMMPE_STA, AcIdx) );

				RT_TRACE( COMP_QOS, DBG_LOUD, ("QosOnBeaconUpdateParameter(): Update WMMPE" ) );
			}
		}
	}
}



//
// QoS Data header fill.
//
VOID
QosFillHeader(
	IN	PADAPTER	Adapter,
	IN	PRT_TCB		pTcb
)
{
	u1Byte		i;
	u2Byte		FragIndex = 0;
	u2Byte		FragBufferIndex = 0;
	pu1Byte		pHeader = (pu1Byte)(GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb));
	PSTA_QOS	pStaQos = Adapter->MgntInfo.pStaQos;
	
	// Note: subtype of QoS data has already changed in TranslateHeader().
	// Thus I use FC byte[0] bit7 to determine filling QoS control field or not. 
	// Annie, 2005-12-06.
	if( IsQoSDataFrame(pHeader) && !IsMgntQosNull(pHeader))
	{		
		u2Byte	QosCtrl;
		// 1. Fill QoS Control Field
		SET_QOS_CTRL(pHeader, 0);
		SET_QOS_CTRL_STA_DATA_AMSDU(pHeader, ((pTcb->bAggregate)?1:0) );
		SET_QOS_CTRL_WMM_UP(pHeader, pTcb->priority);
		SET_QOS_CTRL_WMM_ACK_POLICY(pHeader, 
			(WMMUP_TO_RT_AC_BIT(pTcb->priority) & pStaQos->AcNoAck) ? 1 : 0); // Set Ack Policy by the UP setting
		
		// 2. Consider fragmentation case.
		QosCtrl=GET_QOS_CTRL(pHeader);
	
		for( i=0; i<pTcb->BufferCount; i++ )
		{
			if( FragBufferIndex == 0 )	// fragmentation header
			{
				pu1Byte		pFrame;

				pFrame = (pu1Byte)&pTcb->BufferList[i].VirtualAddress[Adapter->TXPacketShiftBytes];


				// Fill QoS control field
				SET_QOS_CTRL(pFrame, QosCtrl);
			}

			FragBufferIndex++;
			if( FragBufferIndex == pTcb->FragBufCount[FragIndex] )
			{ // Next Fragment...
				FragIndex++;
				FragBufferIndex = 0;
			}
		}

		// 3. Queue Mapping
		switch( pTcb->priority )
		{
		case 0:
		case 3:
			pTcb->SpecifiedQueueID = BE_QUEUE;	// 1
			break;
			
		case 1:
		case 2:
			pTcb->SpecifiedQueueID = BK_QUEUE;	// 0
			break;
			
		case 4:
		case 5:			
			pTcb->SpecifiedQueueID = VI_QUEUE;	// 2
			break;

		case 6:
		case 7:
			pTcb->SpecifiedQueueID = VO_QUEUE;	// 3
			break;

		default:
			RT_ASSERT( FALSE, ("QosFillHeader(): Invalid UP: %d !!!\n", pTcb->priority ) );
			break;
		}
	}
	else
	{
		pTcb->SpecifiedQueueID = LOW_QUEUE;
	}

}


//
// For QoS data parsing and checking.
// Return value (BOOLEAN): Is pFrame a QoS data frame or not.
// Added by Annie, 2005-12-23.
//
BOOLEAN
QosDataCheck(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pFrame
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;

	// Check 1: Is it QoS data?
	if( IsQoSDataFrame(pFrame->Octet) )
	{
		// Check 2: Is it unicast?
		if( MacAddr_isMulticast(Frame_Addr1(*pFrame)) )
		{
			RT_TRACE( COMP_QOS, DBG_LOUD, ("QosDataCheck(): [Warning] Mcst/Bcst Qos Data!!\n") );
		}

		// Check 3: Current QoS Mode.
		if( pMgntInfo->pStaQos->CurrentQosMode == QOS_DISABLE )
		{
			RT_TRACE( COMP_QOS, DBG_LOUD, ("QosDataCheck(): [Warning] Recvd Qos Data in QOS_DISABLE mode!\n") );
		}

		// Note: "Check 4: QoS bit in Rx status descriptor" should be verified in HAL.
		// I write it in QueryRxDescStatus8185().

		return	TRUE;
	}
	else
	{
		return	FALSE;
	}
}

//
// Get Qos user priority from packet.
// 2008.01.10, by shien chang.
//
u1Byte
QosGetUserPriority(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pFrame
	)
{
	u1Byte QosCtl;

	if( IsQoSDataFrame(pFrame->Octet) )
	{
		QosCtl = GET_QOS_CTRL_WMM_UP(pFrame->Octet);
		return QosCtl;
	}
	else
	{
		return 0;
	}
}


//
// QoS element parsing debug on Bss Descriptor. Added by Annie, 2005-11-12.
//
VOID
QosParsingDebug_BssDesc(
	IN	PRT_WLAN_BSS	pBssDesc
	)
{
	if( pBssDesc->BssQos.bdQoSMode == QOS_DISABLE )
	{
		return;
	}

	RT_TRACE( COMP_QOS, DBG_LOUD, ("-----------------------------------------\n" ) );	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("===> QosParsingDebug_BssDesc()\n") );
	RT_PRINT_STR( COMP_QOS, DBG_LOUD, "SSID", pBssDesc->bdSsIdBuf, pBssDesc->bdSsIdLen );

	RT_TRACE( COMP_QOS, DBG_LOUD, ("<=== QosParsingDebug_BssDesc()\n") );
}



//
// QoS element parsing debug on station. Added by Annie, 2005-11-12.
//
VOID
QosParsingDebug_STA(
	IN	PADAPTER		Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;

	RT_TRACE( COMP_QOS, DBG_LOUD, ("===> QosParsingDebug_STA()\n") );

	RT_TRACE( COMP_QOS, DBG_LOUD, ("QosParsingDebug_STA(): CurrentQosMode is %d\n", pStaQos->CurrentQosMode ) );
	if( pStaQos->CurrentQosMode == QOS_DISABLE )
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("<=== QosParsingDebug_STA(): QOS_DISABLE.\n") );	
		return;
	}
	
	// 0. Show current SSID.
	RT_PRINT_STR( COMP_QOS, DBG_LOUD, "SSID", pMgntInfo->Ssid.Octet, pMgntInfo->Ssid.Length );

	// 1. WMM Information Element.
	RT_TRACE( COMP_QOS, DBG_LOUD, ("--------------- WMM Information Element Content ---------------\n") );


	// 2. WMM Parameter Element.
	RT_TRACE( COMP_QOS, DBG_LOUD, ("--------------- WMM Parameter Element Content ----------------\n") );

	QosParsingDebug_ParaElement( pStaQos->WMMParamEle );
	
	RT_TRACE( COMP_QOS, DBG_LOUD, ("------------------------------------------------------------\n") );
	RT_TRACE( COMP_QOS, DBG_LOUD, ("<=== QosParsingDebug_STA()\n") );
}






//
// QoS parameter element parsing debug. Added by Annie, 2005-11-12.
//

VOID
QosParsingDebug_ParaElement(
	IN	pu1Byte		pWMMParamEle
	)
{
	AC_CODING	eACI;
	pu1Byte		pAcParam;
	pu1Byte		oui;

	if( pWMMParamEle == NULL )
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("[Warning] QosParsingDebug_ParaElement(): pWMMParaEle is NULL!return!") );
		return;
	}

	oui = GET_WMM_PARAM_ELE_OUI(pWMMParamEle);
	RT_TRACE( COMP_QOS, DBG_LOUD, ("OUI: %02X- %02X- %02X\n", oui[0], oui[1], oui[2] ));

	RT_TRACE( COMP_QOS, DBG_LOUD, ("OUI Type: 0x%02X\n",	GET_WMM_PARAM_ELE_OUI_TYPE(pWMMParamEle)) );
	RT_TRACE( COMP_QOS, DBG_LOUD, ("OUI sub Type: 0x%02X\n",GET_WMM_PARAM_ELE_OUI_SUBTYPE(pWMMParamEle))  );
	RT_TRACE( COMP_QOS, DBG_LOUD, ("Version: 0x%02X\n",		GET_WMM_PARAM_ELE_VERSION(pWMMParamEle))  );
	RT_TRACE( COMP_QOS, DBG_LOUD, ("QosInfo: 0x%02X\n",		GET_WMM_PARAM_ELE_QOS_INFO_FIELD(pWMMParamEle))  );
	
	pAcParam = GET_WMM_PARAM_ELE_AC_PARAMS(pWMMParamEle);
	for( eACI=0; eACI<AC_MAX; eACI++ )
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("[ AC %d ]\t", eACI ) );
		QosParsingDebug_AcParam(pAcParam+(4*eACI));
	}
}


//
// Print AC parameter for debug purpose.
//
VOID
QosParsingDebug_AcParam(
	IN	pu1Byte		pAcParam
	)
{
	if( pAcParam == NULL )
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("[Warning] QosParsingDebug_AcParam(): pAcParam is NULL!return!") );
		return;
	}

	RT_TRACE( COMP_QOS, DBG_LOUD, ("[0x%08X]\n",	EF4Byte(*(UNALIGNED pu4Byte)pAcParam)) );
	RT_TRACE( COMP_QOS, DBG_TRACE, ("	   - AIFSN(4bit)=0x%X\n",	GET_WMM_AC_PARAM_AIFSN(pAcParam)) );
	RT_TRACE( COMP_QOS, DBG_TRACE, ("	   - ACM(1bit)=0x%X\n",		GET_WMM_AC_PARAM_ACM(pAcParam)) );
	RT_TRACE( COMP_QOS, DBG_TRACE, ("       - ACI(2bit)=0x%X\n",		GET_WMM_AC_PARAM_ACI(pAcParam))  );
	
	RT_TRACE( COMP_QOS, DBG_TRACE, ("       - ECWmin(4bit)=0x%X\n",	GET_WMM_AC_PARAM_ECWMIN(pAcParam)) );
	RT_TRACE( COMP_QOS, DBG_TRACE, ("       - ECWmax(4bit)=0x%X\n",	GET_WMM_AC_PARAM_ECWMAX(pAcParam)) );
	
	RT_TRACE( COMP_QOS, DBG_TRACE, ("     TXOPLimit = 0x%04X\n",		GET_WMM_AC_PARAM_TXOP_LIMIT(pAcParam)) );
}


//
// Print QoS Control Field for debug purpose.
// Added by Annie, 2005-12-05.
//
VOID
QosParsingDebug_QosCtrlField(
	IN	pu1Byte	pFrameHeader
	)
{
	if( pFrameHeader == NULL )
	{
		RT_TRACE( COMP_QOS, DBG_LOUD, ("[Warning] QosParsingDebug_QosCtrlField(): pQCField is NULL!return!") );
		return;
	}

	RT_TRACE( COMP_QOS, DBG_LOUD, ("QosCtrlField = 0x%04X\n", GET_QOS_CTRL(pFrameHeader)) );
	RT_TRACE( COMP_QOS, DBG_LOUD, ("       - UP(3bit)        = 0x%X\n", GET_QOS_CTRL_WMM_UP(pFrameHeader)) );
	RT_TRACE( COMP_QOS, DBG_LOUD, ("       - EOSP(1bit)      = 0x%X\n", GET_QOS_CTRL_WMM_EOSP(pFrameHeader)) );
	RT_TRACE( COMP_QOS, DBG_LOUD, ("       - AckPolicy(2bit) = 0x%X\n", GET_QOS_CTRL_WMM_ACK_POLICY(pFrameHeader)) );
}


//
//	Description: Send a QoS Null function Data frame for WMM UAPSD.
//	2006.06.22, by Isaiah.
//
VOID
SendQoSNullFunctionData(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			StaAddr,
	IN	u1Byte			AC,
	IN	BOOLEAN			bForcePowerSave
	)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER 	pBuf;
	u2Byte					DataRate;

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructNullFunctionData(
			Adapter, 
			pBuf->Buffer.VirtualAddress,
			&pTcb->PacketLength,
			StaAddr,
			TRUE,
			AC,
			FALSE,
			bForcePowerSave);

		DataRate = pMgntInfo->LowestBasicRate;			// Annie, 2005-03-31
		
		pTcb->priority = AC;
		
		MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, LOW_QUEUE, DataRate);	
	}

	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}



VOID
QosConstructEDCAParamElem(
	IN	PADAPTER		Adapter,
	OUT	POCTET_STRING	posEDCAParamElem	
	)
{
	//1REWRITE this part!!!
	PlatformZeroMemory(posEDCAParamElem->Octet, 2); //(Qos Info field + reserved) for WMM
	
	CopyMem(posEDCAParamElem->Octet + 2, Adapter->STA_EDCA_PARAM, AC_MAX * AC_PARAM_SIZE);

	posEDCAParamElem->Length = WMM_PARAM_ELE_BODY_LEN;
}

//
//	Description:
//		Utility function to construct TSPEC IE.
//		(include EID and length)
//
VOID
QosConstructTSPEC(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pBuffer,
	OUT pu4Byte			pLength,
	IN	u1Byte			TID,
	IN	u1Byte			Direction,
	IN	BOOLEAN			bPSB,
	IN	BOOLEAN			bSchedule,
	IN	u1Byte			AccessPolicy,
	IN	BOOLEAN			bAggregation,
	IN	u1Byte			AckPolicy,
	IN	u1Byte			TrafficType,
	IN	u1Byte			UserPriority,
	IN	u2Byte			NominalMsduSize,
	IN	u2Byte			MaxMsduSize,
	IN	u4Byte			MinServiceItv,
	IN	u4Byte			MaxServiceItv,
	IN	u4Byte			InactivityItv,
	IN	u4Byte			SuspensionItv,
	IN	u4Byte			ServiceStartTime,
	IN	u4Byte			MinDataRate,
	IN	u4Byte			MeanDataRate,
	IN	u4Byte			PeakDataRate,
	IN	u4Byte			MaxBurstSize,
	IN	u4Byte			DelayBound,
	IN	u4Byte			MinPhyRate,
	IN	u2Byte			SurplusBandwithAllow,
	IN	u2Byte			MediumTime
	)
{
	pu1Byte		pTspec = pBuffer;
	u1Byte		WmmOUI[] = { 0x00, 0x50, 0xf2 };

	// TSPEC element
	SET_TSPEC_ID(pTspec, EID_Vendor);
	SET_TSPEC_LENGTH(pTspec, 61); // OUI(3) + Type(1) + SubType(1) + Version (1) + TSPEC Body(55)
	SET_TSPEC_OUI(pTspec, WmmOUI);
	SET_TSPEC_OUI_TYPE(pTspec, 2);
	SET_TSPEC_OUI_SUBTYPE(pTspec, 2);
	SET_TSPEC_VERSION(pTspec, 1);
	
	// TSINFO
	SET_TSPEC_TSINFO_TSID(pTspec, TID);
	SET_TSPEC_TSINFO_DIRECTION(pTspec, Direction);
	SET_TSPEC_TSINFO_PSB(pTspec, (bPSB ? 1:0));
	SET_TSPEC_TSINFO_SCHEDULE(pTspec, (bSchedule ? 1:0));
	SET_TSPEC_TSINFO_ACCESS_POLICY(pTspec, AccessPolicy);
	SET_TSPEC_TSINFO_AGGREGATION(pTspec, (bAggregation ? 1:0));
	SET_TSPEC_TSINFO_ACK_POLICY(pTspec, AckPolicy);
	SET_TSPEC_TSINFO_TRAFFIC_TYPE(pTspec, TrafficType);
	SET_TSPEC_TSINFO_UP(pTspec, UserPriority);

	// TSPEC Body
	SET_TSPEC_NOMINAL_MSDU_SIZE(pTspec, NominalMsduSize);
	SET_TSPEC_MAX_MSDU_SIZE(pTspec, MaxMsduSize);
	SET_TSPEC_MIN_SERVICE_INTERVAL(pTspec, MinServiceItv);
	SET_TSPEC_MAX_SERVICE_INTERVAL(pTspec, MaxServiceItv);
	SET_TSPEC_INACTIVITY_INTERVAL(pTspec, InactivityItv);
	SET_TSPEC_SUSPENSION_INTERVAL(pTspec, SuspensionItv);
	SET_TSPEC_SERVICE_START_TIME(pTspec, ServiceStartTime);
	SET_TSPEC_MIN_DATA_RATE(pTspec, MinDataRate);
	SET_TSPEC_MEAN_DATA_RATE(pTspec, MeanDataRate);
	SET_TSPEC_PEAK_DATA_RATE(pTspec, PeakDataRate);
	SET_TSPEC_MAX_BURST_SIZE(pTspec, MaxBurstSize);
	SET_TSPEC_DELAY_BOUND(pTspec, DelayBound);
	SET_TSPEC_MIN_PHY_RATE(pTspec, MinPhyRate);
	SET_TSPEC_SURPLUS_BANDWITH_ALLOWANCE(pTspec, SurplusBandwithAllow);
	SET_TSPEC_MEDIUM_TIME(pTspec, MediumTime);

	*pLength = TSPEC_SIZE;
}


//
//	Description:
//		Initialize a TS object.
//
//	Assumption:
//		RT_QOS_SPINLOCK is acquired.
//
VOID
QosInitTs(
	IN PADAPTER			Adapter,
	IN PQOS_TSTREAM		pTs,
	IN u1Byte			TSID, 
	IN PWMM_TSPEC		pTSpec
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS 	pStaQos = pMgntInfo->pStaQos;
	
	RT_ASSERT(pTs != NULL, ("QosInitTs(): pTs should NOT be NULL!!!\n"));
	RT_ASSERT(pTSpec != NULL, ("QosInitTs(): pTSpecElm should NOT be NULL!!!\n"));
	RT_ASSERT(TSID < MAX_STA_TS_COUNT, ("QosInitTs(): invalid TSID(%d)!!!\n", TSID));

	pTs->bUsed = TRUE;
	pTs->bEstablishing = TRUE;
	pTs->DialogToken = pStaQos->DialogToken;
	RTInitializeListHead(&(pTs->BufferedPacketList));
	pTs->MsduLifetime = 0;
	pTs->TimeSlotCount = 0;
	PlatformZeroMemory(&(pTs->TSpec), sizeof(WMM_TSPEC));
	PlatformMoveMemory(&(pTs->OutStandingTSpec), pTSpec, sizeof(WMM_TSPEC));
	pTs->NominalPhyRate = (u1Byte) QOS_BPS_TO_RATE( GET_TSPEC_MIN_PHY_RATE(pTSpec) );
	pTs->UserPriority = GET_TSPEC_TSINFO_UP(pTSpec);
	

	RT_TRACE(COMP_QOS, DBG_LOUD, ("========================================\n"));
	RT_TRACE(COMP_QOS, DBG_LOUD, ("QosInitTs(): pTs(%p), TSID(%d)\n", pTs, TSID));
	RT_PRINT_DATA(COMP_QOS, DBG_LOUD, "TSPEC: ", &(pTs->TSpec), sizeof(pTs->TSpec));
	RT_TRACE(COMP_QOS, DBG_LOUD, ("========================================\n"));
}


//
//	Description:
//		Flush buffered list in TS (ONLY for STA)
//
//	Assumption:
//		RT_TX_SPINLOCK is acquired.
//		RT_QOS_SPINLOCK is not acquired.
//
VOID
QosFlushTs(
	IN PADAPTER			Adapter,
	IN PQOS_TSTREAM		pTs
	)
{
	PRT_TCB		pTcb;
	
	if(!ACTING_AS_AP(Adapter))
	{
		while ( !RTIsListEmpty(&(pTs->BufferedPacketList)) )
		{
			pTcb = (PRT_TCB)RTRemoveHeadList(&(pTs->BufferedPacketList));
			
			pTcb->bFromUpperLayer = FALSE;
			NicIFSendPacket(Adapter, pTcb);

		}
	}
}

//
//	Description:
//		Add a TS object to using pool for new created traffic stream.
//		If the TS object is already there, we will update TSPEC.
//
//	Input:
//		TSID: Traffic stream ID, WMM: 0-7, CAC: 8,9, 11e: 8-15
//		RA: Receiver MAC address. 
//		TA: Transmitter MAC address.
//		pTSpec: TSPEC of the traffic stream, its content will be copied into the QOS_TSTREAM object.
//
//	Output:
//		Pointer to the TS object for new created traffic stream if succeeded, 
//		NULL otherwise.
//
//	Assumption:
//		1. RT_QOS_SPINLOCK is NOT acquired.
//
PQOS_TSTREAM 
QosAddTs(
	IN PADAPTER			Adapter,
	IN u1Byte			TSID, 
	IN pu1Byte			RA, 
	IN pu1Byte			TA,
	IN PWMM_TSPEC		pTSpec
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	PQOS_TSTREAM	pTs = NULL;

	RT_ASSERT(RA != NULL, ("QosAddTs(): RA cannot be NULL!!!\n"));
	RT_ASSERT(TA != NULL, ("QosAddTs(): TA cannot be NULL!!!\n"));


	if(!ACTING_AS_AP(Adapter))
	{
		if(TSID < MAX_STA_TS_COUNT)
		{
			pTs = &(pStaQos->StaTsArray[TSID]); 
		}
	}
	else
	{
		u1Byte Key[QOS_TSTREAM_KEY_SIZE];

		Key[0] = TSID;
		PlatformMoveMemory(Key+1, RA, 6);
		PlatformMoveMemory(Key+7, TA, 6);
		pTs = (PQOS_TSTREAM)RtPutKeyToHashTable(pStaQos->hApTsTable, Key);
	}

	if(pTs != NULL)
	{
		if (pTs->bUsed) 
			QosUpdateTs(Adapter, pTs, pTSpec);
		else 
			QosInitTs(Adapter, pTs, TSID, pTSpec);
	}
	else
	{
		RT_ASSERT(FALSE, ("QosAddTs(): mActingAsAp(%d), TSID(%d) => pTs is NULL!!!\n", ACTING_AS_AP(Adapter), TSID));
	}

	return pTs;
}

//
//	Description:
//		Get a TS object according to given (TSID, RA, TA).	
//
//	Input:
//		TSID: Traffic stream ID, WMM: 0-7, CAC: 8,9, 11e: 8-15
//		RA: Receiver MAC address. It can be NULL for STA mode.
//		TA: Transmitter MAC address. It can be NULL for STA mode.
//
//	Output:
//		Pointer to a TS object if found, NULL otherwise.
//
//	Assumption:
//		1. We MIGHT acquired RT_QOS_SPINLOCK before calling this function, 
//		so, we should not acquire it in this function.
//
PQOS_TSTREAM 
QosGetTs(
	IN PADAPTER			Adapter,
	IN u1Byte			TSID, 
	IN pu1Byte			RA, 
	IN pu1Byte			TA
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	PQOS_TSTREAM	pTs = NULL;
	
	if(!ACTING_AS_AP(Adapter))
	{
		if(	TSID < MAX_STA_TS_COUNT && 
			pStaQos->StaTsArray[TSID].bUsed )
		{
			pTs = &(pStaQos->StaTsArray[TSID]); 
		}
	}
	else
	{
		u1Byte Key[QOS_TSTREAM_KEY_SIZE];

		RT_ASSERT(RA != NULL, ("QosGetTs(): RA cannot be NULL!!!\n"));
		RT_ASSERT(TA != NULL, ("QosGetTs(): TA cannot be NULL!!!\n"));

		Key[0] = TSID;
		PlatformMoveMemory(Key+1, RA, 6);
		PlatformMoveMemory(Key+7, TA, 6);
		pTs = (PQOS_TSTREAM)RtGetValueFromHashTable(pStaQos->hApTsTable, Key);
	}

	return pTs;
}

//
//	Assumption:
//		Only invoked by QosAddTs().
//
VOID
QosUpdateTs(
	IN PADAPTER				Adapter,
	IN PQOS_TSTREAM			pTs,
	IN PWMM_TSPEC			pTSpec
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS 	pStaQos = pMgntInfo->pStaQos;
	
	RT_ASSERT(pTs != NULL, ("QosUpdateTs(): pTs should NOT be NULL!!!\n"));
	RT_ASSERT(pTSpec != NULL, ("QosUpdateTs(): pTSpec should NOT be NULL!!!\n"));

	pTs->DialogToken = pStaQos->DialogToken;
	pTs->TimeSlotCount = 0;
	PlatformMoveMemory(&(pTs->OutStandingTSpec), pTSpec, sizeof(WMM_TSPEC));
	pTs->UserPriority = GET_TSPEC_TSINFO_UP(pTSpec);
	pTs->NominalPhyRate = (u1Byte) QOS_BPS_TO_RATE( GET_TSPEC_MIN_PHY_RATE(pTSpec) );
}

//
//	Description:
//		Actually send QOS ADDTS frame.
//
VOID
QosSendAddTs(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs,
	IN	u4Byte			numTs
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS 			pStaQos = pMgntInfo->pStaQos;
	PQOS_TSTREAM		pTStream = pTs;
	u4Byte i;
	
	for (i = 0; i < numTs; i ++)
	{
		if (pTStream && pTStream->bUsed)
			pTStream->TimeSlotCount = 1;
		pTStream ++;
	}

	PlatformSetTimer(Adapter, 
					&(pStaQos->AddTsTimer), 
					ADDTS_TIME_SLOT);
	SendQosAddTs(Adapter, pTs, numTs);

}

//
//	Description:
//		Actually send QOS DELTS frame.
//
VOID
QosSendDelTs(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs,
	IN	u4Byte			numTs
	)
{
	if (pTs && (numTs > 0))
		SendQosDelTs(Adapter, pTs, numTs);
}


//
//	Description:
//		Hash from (TSID, RA, TA) to [0, MAX_AP_TS_COUNT-1].
//
//	Input:
//		Key: 13-byte array: 1-byte TSID, 6-byte RA, and 6-byte TA.
//
UINT
QosTsHash(
	IN RT_HASH_KEY			Key
	)
{
	UINT			result = 0;
	u4Byte			idx;

	RT_PRINT_DATA(COMP_QOS, DBG_TRACE, "TSID, RA, TA: ", Key, RM_STA_RX_POWER_KEY_SIZE);

	for(idx = 1; idx < QOS_TSTREAM_KEY_SIZE; idx ++) 
		result += Key[idx];

	result = result % MAX_AP_TS_COUNT;
	RT_TRACE(COMP_QOS, DBG_TRACE, ("QosTsHash() result: %d\n", result));

	return result;
}

//
//	Description:
//		ADDTS req timer callback, if ADDTS rsp not received
//		after we sent ADDTS req, we remove the TS.
//		AP mode doesn't handle this case.
//
VOID
QosAddTsTimerCallback(
	IN PRT_TIMER		pTimer
	)
{
	PADAPTER		Adapter = (PADAPTER)pTimer->Adapter;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	u1Byte			i;
	PQOS_TSTREAM	pTs;
	BOOLEAN			bRescheduleTimer = FALSE;

	if (!ACTING_AS_AP(Adapter))
	{
		for (i = 0; i < MAX_STA_TS_COUNT; i ++)
		{
			pTs = &(pStaQos->StaTsArray[i]);
			if (pTs && pTs->bUsed)
			{
				// If ADDTS has sent out, TimeSlotCount should not be zero.
				if (pTs->TimeSlotCount)
					pTs->TimeSlotCount ++;
				
				if (pTs->TimeSlotCount == (ADDTS_TIMEOUT / ADDTS_TIME_SLOT) )
				{ // Time is up, remove this ts, do not request ts.
					RT_TRACE(COMP_QOS, DBG_LOUD, ("QosAddTsTimerCallback(): ADDTS Timeout!\n"));
					if (pTs->bEstablishing)
						QosRemoveTs(Adapter, pTs);
					pTs->TimeSlotCount = 0;
				}
				else if (pTs->TimeSlotCount != 0)
				{
					bRescheduleTimer = TRUE;
				}
			}
		}

		if (bRescheduleTimer)
		{
			PlatformSetTimer(Adapter,
							&(pStaQos->AddTsTimer),
							ADDTS_TIME_SLOT);
		}
	}
	
}

//
//	Description:
//		Remove a TS object from using pool when a traffic stream tore down.
//
//	Input:
//		pTs: Pointer to the TS obejct to remove.
//
//	Assumption:
//		RT_QOS_SPINLOCK is NOT acquired.
//
VOID
QosRemoveTs(
	IN PADAPTER			Adapter,
	IN PQOS_TSTREAM		pTs
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS	pStaQos = pMgntInfo->pStaQos;

	RT_ASSERT(pTs != NULL, ("QosRemoveTs(): pTS should NOT be NULL!!!\n"));
	RT_ASSERT(pTs->bUsed, ("QosRemoveTs(): pTS(%p) is not used!!!\n", pTs));

	QosResetTs(Adapter, pTs);
	if(ACTING_AS_AP(Adapter))
	{
		u1Byte Key[QOS_TSTREAM_KEY_SIZE];

		PlatformMoveMemory(Key, pTs->__HashEntry.Key, QOS_TSTREAM_KEY_SIZE);
		RtRemvoeKeyFromVaHashTable(pStaQos->hApTsTable, Key);
	}

}

//
//	Description:
//		Reset an TS object.
//
//	Assumption:
//		RT_QOS_SPINLOCK is acquired.
//
VOID
QosResetTs(
	IN PADAPTER			Adapter,
	IN PQOS_TSTREAM		pTs
	)
{
	PRT_TCB		pTcb;
	
	RT_ASSERT(pTs != NULL, ("QosResetTs(): pTs should NOT be NULL!!!\n"));

	pTs->bUsed = FALSE;
	pTs->bEstablishing = FALSE;
	pTs->DialogToken = 0;
	pTs->TimeSlotCount = 0;
	pTs->MsduLifetime = 0;
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_MSDU_LIFE_TIME, (pu1Byte)pTs);
	PlatformZeroMemory(pTs->TSpec, sizeof(WMM_TSPEC));
	PlatformZeroMemory(pTs->OutStandingTSpec, sizeof(WMM_TSPEC));

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
	while ( !RTIsListEmpty(&(pTs->BufferedPacketList)) )
	{
		pTcb = (PRT_TCB)RTRemoveHeadList(&(pTs->BufferedPacketList));

		ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
	}
		PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}

//
//	Description:
//		Reset TS related variables to initiailzed state.
//
//	Assumption:
//		RT_QOS_SPINLOCK is NOT acquired.
//
BOOLEAN 
QosResetAllTs(
	IN PADAPTER			Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	PQOS_TSTREAM	pTs;
	u4Byte			idx;
	PRT_LIST_ENTRY	pList;
	PRT_LIST_ENTRY	pEntry;


	for(idx = 0; idx < MAX_STA_TS_COUNT; idx++)
	{
		pTs = &(pStaQos->StaTsArray[idx]);
		if(pTs->bUsed)
			QosResetTs(Adapter, pTs);
	}

	pList = RtGetAllValuesFromHashTable(pStaQos->hApTsTable);
	for(pEntry = pList->Flink;
		pEntry != pList;
		pEntry = pEntry->Flink)
	{
		pTs = (PQOS_TSTREAM)pEntry;
		RT_ASSERT(pTs->bUsed, ("QosResetAllTs(): pTs(%p) bUsed==FALSE !!!\n", pTs));
		QosResetTs(Adapter, pTs);
	}
	RtResetHashTable(pStaQos->hApTsTable);


	return FALSE;
}

//
//	Description:
//		Remove all TS added.
//
//	Assumption:
//		RT_QOS_SPINLOCK is NOT acquired.
//
VOID
QosRemoveAllTs(
	IN PADAPTER			Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	PQOS_TSTREAM	pTs;
	PRT_LIST_ENTRY	pList, pEntry;
	u1Byte			i;
	

	for (i = 0; i < MAX_STA_TS_COUNT; i ++)
	{
		pTs = &(pStaQos->StaTsArray[i]);
		if (pTs->bUsed)
			QosRemoveTs( Adapter, pTs );
	}

	pList = RtGetAllValuesFromHashTable(pStaQos->hApTsTable);
	for(pEntry = pList->Flink;
		pEntry != pList;
		pEntry = pEntry->Flink)
	{
		pTs = (PQOS_TSTREAM)pEntry;
		RT_ASSERT(pTs->bUsed, ("QosRemoveAllTs(): pTs(%p) bUsed==FALSE !!!\n", pTs));
		QosRemoveTs(Adapter, pTs);
	}
	RtResetHashTable(pStaQos->hApTsTable);	
	
}



//
//	Description:
//		Handle the reception of MSDU Lifetime IE.
//
VOID
QosRecvMsduLifetimeIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer,
	IN	QOSIE_SOURCE	source,
	IN	BOOLEAN			rspStatus
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	u1Byte			TSID;
	PQOS_TSTREAM	pTs = NULL;
	u1Byte			Length;

	Length = *(pOsBuffer->Octet + 1);
	if (Length != 7)
		return;

	if (!ACTING_AS_AP(Adapter))
	{
		if ( (source==QOSIE_SRC_ADDTSRSP) ||
			(source==QOSIE_SRC_REASOCRSP) )
		{
			TSID = *(pOsBuffer->Octet + 6);
			pTs = &(pStaQos->StaTsArray[TSID]);
			if (pTs && pTs->bUsed)
			{
				pTs->MsduLifetime = EF2Byte( *((UNALIGNED pu2Byte)(pOsBuffer->Octet + 7)) );
				RT_TRACE(COMP_QOS, DBG_LOUD, ("QosRecvMsduLifetimeIE(): Receive Life Time %d TUs for TSID (%d)\n", pTs->MsduLifetime, TSID));
				Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_MSDU_LIFE_TIME, (pu1Byte)pTs);
			}
		}
	}
}



//
//	Description:
//		Parse the Qos traffic stream related IE and handle it.
//
VOID
QosParsingTrafficStreamIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer,
	IN	u4Byte			offset,
	IN	QOSIE_SOURCE	source,
	IN	BOOLEAN			rspStatus
	)
{
	static u1Byte	TspecOui[] = { 0x00, 0x50, 0xF2, 0x02, 0x02, 0x01 };
	static u1Byte	TsrsOui[] = { 0x00, 0x40, 0x96, 0x08 };
	static u1Byte	MsduLifeOui[] = { 0x00, 0x40, 0x96, 0x09 };
	static u1Byte	CcxTsmOui[] = {0x00, 0x40, 0x96, 0x07}; // For CCX4 S56, Traffic Stream Metrics, 070615, by rcnjko.
	u4Byte			BytesRead = offset;
	OCTET_STRING	osIE;
	RT_DOT11_IE		Dot11Ie;

	while (HasNextIE(pOsBuffer, BytesRead))
	{
		Dot11Ie = AdvanceToNextIE(pOsBuffer, &BytesRead);
		if (Dot11Ie.Id == 0xDD)
		{
			osIE.Octet = Dot11Ie.Content.Octet - 2;
			osIE.Length = Dot11Ie.Content.Length + 2;
			
			if (PlatformCompareMemory(
						Dot11Ie.Content.Octet,
						TspecOui,
						sizeof(TspecOui)) == 0)
			{
				QosRecvTspecIE(Adapter, &osIE, source, rspStatus);
				QosParsingDebug_TspecIE(Adapter, &osIE);
			}
			else if (PlatformCompareMemory(
						Dot11Ie.Content.Octet,
						TsrsOui,
						sizeof(TsrsOui)) == 0)
			{
				QosRecvTsrsIE(Adapter, &osIE, source, rspStatus);	
				QosParsingDebug_TsrsIE(Adapter, &osIE);
			}
			else if (PlatformCompareMemory(
						Dot11Ie.Content.Octet,
						MsduLifeOui,
						sizeof(MsduLifeOui)) == 0)
			{
				QosRecvMsduLifetimeIE(Adapter, &osIE, source, rspStatus);
				QosParsingDebug_MsduLifetimeIE(Adapter, &osIE);
			}
			else if (PlatformCompareMemory(
						Dot11Ie.Content.Octet, 
						CcxTsmOui,
						sizeof(CcxTsmOui)) == 0)
			{
				
			}
		}
	}
}

//
//	Description:
//		Handle the reception of TSPEC IE.
//
VOID
QosRecvTspecIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer,
	IN	QOSIE_SOURCE	source,
	IN	BOOLEAN			rspStatus
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	u1Byte			TSID;
	PQOS_TSTREAM	pTs = NULL;

	if (!ACTING_AS_AP(Adapter))
	{
		if ( ((source == QOSIE_SRC_ADDTSRSP) ||
			(source == QOSIE_SRC_REASOCRSP)) &&
			(rspStatus == TRUE) )
		{
			TSID = GET_TSPEC_TSINFO_TSID(pOsBuffer->Octet);
			pTs = &(pStaQos->StaTsArray[TSID]);
			if (pTs && pTs->bUsed)
			{
				pTs->TimeSlotCount = 0;

				// Update TS parameter.
				RT_ASSERT( (pOsBuffer->Length == sizeof(WMM_TSPEC)), 
					("QosRecvTspecIE(): Invalid TSPEC IE length\n"));
				PlatformMoveMemory(&(pTs->TSpec),
									pOsBuffer->Octet,
									sizeof(WMM_TSPEC));
				QosIncAdmittedTime(Adapter, pTs);
							
				if (pTs->bEstablishing)
				{
					u1Byte	CurrCcxVerNumber = 0;
			
					CCX_QueryVersionNum(Adapter, &CurrCcxVerNumber);
					
					pTs->bEstablishing = FALSE;

					if (CurrCcxVerNumber >= 4)
					{
						//if ((pStaQos->StaTsArray[0].bUsed) &&
						//	(!pStaQos->StaTsArray[0].bEstablishing) &&
						//	(pStaQos->StaTsArray[1].bUsed) &&
						//	(!pStaQos->StaTsArray[1].bEstablishing))
						{
							PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
							CCX_FlushAllTs(Adapter);
							PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
						}
					}
					else
					{
						PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);
						QosFlushTs(Adapter, pTs);
						PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
					}
					
				}
			}			
		}
		else if (source == QOSIE_SRC_DELTS)
		{
			TSID = GET_TSPEC_TSINFO_TSID(pOsBuffer->Octet);
			
			QosDecAdmittedTime(Adapter, pTs);
			pTs = &(pStaQos->StaTsArray[TSID]);	
			if (pTs && pTs->bUsed)
			{
				QosRemoveTs(Adapter, pTs);
			}
		}
		
	}
	else
	{
		// TODO: AP mode.
	}
}

//
//	Description:
//		Handle the reception of TSRS IE.
//
VOID
QosRecvTsrsIE(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pOsBuffer,
	IN	QOSIE_SOURCE	source,
	IN	BOOLEAN			rspStatus
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	u1Byte			TSID;
	PQOS_TSTREAM	pTs = NULL;
	u1Byte			Length;
	u1Byte			i;
	u1Byte			rate;

	Length = (*(pOsBuffer->Octet+1)) -5;
	if ( (Length==0) || (Length>8) )
		return;
	TSID = *(pOsBuffer->Octet+6);

	if (!ACTING_AS_AP(Adapter))
	{
		if ( (source == QOSIE_SRC_ADDTSRSP) ||
			(source == QOSIE_SRC_REASOCRSP) )
		{
			pTs = &(pStaQos->StaTsArray[TSID]);
			
			for (i = 0; i < Length; i ++)
			{
				rate = *(pOsBuffer->Octet + 7 + i);
				if (rate & 0x80)
				{
					rate &= ~0x80;
					if (rate != QosGetNPR(Adapter, pTs))
					{
						RT_TRACE(COMP_QOS, DBG_LOUD, 
							("QosRecvTsrsIE(): returned NPR (%d) is not one we requested (%d) \n", rate, QosGetNPR(Adapter, pTs)));
						QosSetNPR(Adapter, pTs, rate);

						if (!rspStatus)
						{
							// TODO: resent ADDTS request.
						}

						break;
					}
				}
			}
		}
	}
	else
	{
		// TODO: AP mode.
	}
}


//
//	Description:
//		Construct traffic stream rate set element.
//
VOID
QosConstructTSRS(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs,
	IN	POCTET_STRING	pOsAddTsPkt
	)
{
	//
	// TS Rate Set Format defined in CCXv4 S54.2.6 as the following:
	//	Fields			Value
	//	-----			-----
	//	ElementID		221 (Vendor Specific)
	//	Length		6-13
	//	OUI			00:40:96 (Cisco)
	//	OUI Type		8 (TS rate set)
	//	TSID			ID in ranage of 0-7
	//	TS Rates		TS Rate Values (1-8 Bytes)
	// By Bruce, 2008-03-17.
	//
	u1Byte	TSRS[] = { 0xdd, 0x06, 0x00, 0x40, 0x96, 0x08, 0x00, 0x00 };
	u1Byte	TSID;
	
	if (pTs && pTs->bUsed)
	{
		TSID = GET_TSPEC_TSINFO_TSID(pTs->TSpec);
		TSRS[6] = TSID;
		TSRS[7] = pTs->NominalPhyRate;
		PlatformMoveMemory(pOsAddTsPkt->Octet + pOsAddTsPkt->Length,
							TSRS,
							sizeof(TSRS));
		pOsAddTsPkt->Length += sizeof(TSRS);
	}
}

//
//	Description:
//		Increase admitted time of AC which contains in pTs.
//	2007.09.03, by shien chang.
//
VOID
QosIncAdmittedTime(
	IN	PADAPTER			Adapter,
	IN	PQOS_TSTREAM		pTs
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS 		pStaQos = pMgntInfo->pStaQos;
	u1Byte			UserPriority;
	u2Byte			MediumTime;
	AC_CODING		eACI = 0;
	BOOLEAN			bAcm;

	RT_ASSERT(pTs != NULL, ("QosIncAdmittedTime(): pTs should not be NULL\n"));
	if (!pTs->bUsed)
		return;

	bAcm = (BOOLEAN)GET_WMM_AC_PARAM_ACM(pTs->TSpec);
	if (!bAcm)
		return;
	
	UserPriority = GET_TSPEC_TSINFO_UP(pTs->TSpec);
	MediumTime = GET_TSPEC_MEDIUM_TIME(pTs->TSpec);
	
	switch (UserPriority)
	{
		case 0:
		case 3:
			eACI = AC0_BE;
			break;
		case 1:
		case 2:
			eACI = AC1_BK;
			break;
		case 4:
		case 5:
			eACI = AC2_VI;
			break;
		case 6:
		case 7:
			eACI = AC3_VO;
			break;
	}
	//RT_TRACE(COMP_QOS, DBG_LOUD, ("QosIncAdmittedTime(): ADDTS: AC(%d) AdmitTime(%"i64fmt"d) MediumTime(%d)\n",
		//		eACI, pStaQos->acm[eACI].MediumTime, MediumTime));
	pStaQos->acm[eACI].MediumTime += MediumTime;
}

//
//	Description:
//		Decrease admitted time of AC which contains in pTs.
//	2007.09.03, by shien chang.
//
VOID
QosDecAdmittedTime(
	IN	PADAPTER			Adapter,
	IN	PQOS_TSTREAM		pTs
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS 		pStaQos = pMgntInfo->pStaQos;
	u1Byte			UserPriority;
	u2Byte			MediumTime;
	AC_CODING		eACI = 0;

	RT_ASSERT(pTs != NULL, ("QosIncAdmittedTime(): pTs should not be NULL\n"));
	if (!pTs->bUsed)
		return;
	
	UserPriority = GET_TSPEC_TSINFO_UP(pTs->TSpec);
	MediumTime = GET_TSPEC_MEDIUM_TIME(pTs->TSpec);
	
	switch (UserPriority)
	{
		case 0:
		case 3:
			eACI = AC0_BE;
			break;
		case 1:
		case 2:
			eACI = AC1_BK;
			break;
		case 4:
		case 5:
			eACI = AC2_VI;
			break;
		case 6:
		case 7:
			eACI = AC3_VO;
			break;
	}
	//RT_TRACE(COMP_QOS, DBG_LOUD, ("QosDecAdmittedTime(): DELTS: AC(%d) AdmitTime(%"i64fmt"d) MediumTime(%d)\n",
		//		eACI, pStaQos->acm[eACI].MediumTime, MediumTime));			
	if (pStaQos->acm[eACI].MediumTime > MediumTime)
		pStaQos->acm[eACI].MediumTime -= MediumTime;
	else
		pStaQos->acm[eACI].MediumTime = 0;
}

//
//	Description:
//		ACM timer callback.
//	2007.08.21, by shien chang.
//
VOID
QosACMTimerCallback(
	IN PRT_TIMER		pTimer
	)
{
	PADAPTER	Adapter = (PADAPTER)pTimer->Adapter;
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS	pStaQos = pMgntInfo->pStaQos;
	AC_CODING	eACI;
	pu1Byte 	pacParam;
	BOOLEAN 	bAcm;
	
	for(eACI = 0; eACI < AC_MAX; eACI ++)
	{
		pacParam = (pu1Byte)(GET_WMM_PARAM_ELE_AC_PARAM(pStaQos->WMMParamEle)+(4 * eACI));
		bAcm = (BOOLEAN)GET_WMM_AC_PARAM_ACM(pacParam);
		if (bAcm)
		{
			if (pStaQos->acm[eACI].MediumTime > pStaQos->acm[eACI].UsedTime)
			{
				pStaQos->acm[eACI].UsedTime = 0;
			}
			else
			{
				pStaQos->acm[eACI].UsedTime = pStaQos->acm[eACI].UsedTime -
											  pStaQos->acm[eACI].MediumTime;
			}
		}	
	}

	CCX_UpdateUsedTime( Adapter );

	if (!RT_DRIVER_HALT(Adapter) &&
		pMgntInfo->pStaQos->CurrentQosMode > QOS_DISABLE &&
		pMgntInfo->bMediaConnect &&
		!Adapter->bInHctTest)
	{
		PlatformSetTimer(Adapter, &pStaQos->ACMTimer, ACM_TIMEOUT);
	}
}

//
//	Description:
//		Get nominal phy rate of the TS.
//
u1Byte
QosGetNPR(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs
	)
{
	if (pTs && pTs->bUsed)
		return pTs->NominalPhyRate;
	return 0;
}

//
//	Description:
//		Set nominal phy rate for the TS.
//
VOID
QosSetNPR(
	IN	PADAPTER		Adapter,
	IN	PQOS_TSTREAM	pTs,
	IN	u1Byte			rate
	)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	u4Byte		MinPhyRate = 0;
	
	if (!MgntIsRateValidForWirelessMode(rate, pMgntInfo->dot11CurrentWirelessMode))
	{
		RT_TRACE(COMP_QOS, DBG_WARNING, ("QosSetNPR(): Invalid data rate to set!\n"));
		return;
	}
	
	if (pTs && pTs->bUsed)
	{
		MinPhyRate = GET_TSPEC_MIN_PHY_RATE(pTs->TSpec);
		if ((MinPhyRate == 0) ||
			(QOS_RATE_TO_BPS(rate) < MinPhyRate) )
		{
			RT_TRACE(COMP_QOS, DBG_LOUD, ("QosSetNPR(): Invalid normal phy rate!"));
			return;
		}

		pTs->NominalPhyRate = rate;
	}
}

//
//	Description:
//		Search established TS and if 
//		1. No TS -- drop the packet.
//		2. TS is establishing -- buffer the packet.
//		3. TS is established -- transmit the packet.
//
//	Assumption:
//		RT_TX_SPINLOCK is acquired.
//
BOOLEAN
QosAdmissionControl(
	IN PADAPTER			Adapter,
	IN PRT_TCB			pTcb
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PQOS_TSTREAM		pTs;
	BOOLEAN				bHandled = FALSE;
	u1Byte				CurrCcxVerNumber = 0;
			
	CCX_QueryVersionNum(Adapter, &CurrCcxVerNumber);	
	
	if (CurrCcxVerNumber >= 4)
	{
		return CCX_CallAdmissionControl(Adapter, pTcb);
	}
	
	if (ACTING_AS_AP(Adapter)) return FALSE;
	
	if (pTcb->TSID <= MAX_TSPEC_TSID)
	{
		pTs = QosGetTs(Adapter, 
							pTcb->TSID, 
							pMgntInfo->Bssid,
							Adapter->CurrentAddress);

		// No TS, drop the packet.
		if (pTs == NULL || !pTs->bUsed)
		{
			RT_TRACE(COMP_QOS, DBG_LOUD, ("QosAdmissionControl(): Drop packet for invalid TSID\n"));
			ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
			bHandled = TRUE;
		}
		else
		{
			if (pTs->bEstablishing)
			{
				// If TS is establishing, queue the packet.
				RT_TRACE(COMP_QOS, DBG_LOUD, ("QosAdmissionControl(): Queue packet for establishing TS\n"));
				RTInsertTailList(&(pTs->BufferedPacketList), &(pTcb->List));
				bHandled = TRUE;
			}
			else
			{
				// If Ts is found, transmit the packet.
				bHandled = FALSE;
			}
		}
	}

	if (!bHandled)
	{
		BOOLEAN bAdmit;
		bAdmit = QosCalcUsedTimeAndAdmitPacket(Adapter, pTcb);
		if (!bAdmit)
		{
			ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
			bHandled = TRUE;
		}
	}
						
	return bHandled;
}

//
//	Description:
//		Calc used time and admit packet.
//	2007.08.23, by shien chang.
//
BOOLEAN
QosCalcUsedTimeAndAdmitPacket(
	IN	PADAPTER			Adapter,
	IN	PRT_TCB				pTcb
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS 		pStaQos = pMgntInfo->pStaQos;
	AC_CODING 		eACI;
	pu1Byte			pacParam;
	BOOLEAN			bAcm;
	u8Byte			UsedTime;
	BOOLEAN			bDetermined = FALSE;
	u8Byte			AccUsedTime;
	BOOLEAN			bReturn = TRUE;

	if (pStaQos->CurrentQosMode == QOS_DISABLE) 
		return TRUE;
	
	switch (pTcb->priority)
	{
		case 0:
		case 3:
			eACI = AC0_BE;
			break;
		case 1:
		case 2:
			eACI = AC1_BK;
			break;
		case 4:
		case 5:
			eACI = AC2_VI;
			break;
		case 6:
		case 7:
			eACI = AC3_VO;
			break;
		default:
			RT_ASSERT(FALSE, ("QosCalcUsedTimeAndAdmitPacket(): invalid pTcb->priority: %d!!!\n", pTcb->priority));
			eACI = AC0_BE;
			break;
	}

	do
	{
		pacParam = (pu1Byte)(GET_WMM_PARAM_ELE_AC_PARAM(pStaQos->WMMParamEle)+(4*eACI));
		bAcm = (BOOLEAN)GET_WMM_AC_PARAM_ACM(pacParam);

		if (bAcm)
		{
			// 1. Calc used time
			UsedTime = ((pTcb->PacketLength * 8 * 2) / (pTcb->DataRate)) / 32;
			AccUsedTime = pStaQos->acm[eACI].UsedTime + UsedTime;
			
			// 2. Admit packet.
			if (AccUsedTime >= pStaQos->acm[eACI].MediumTime)
			{
				RT_TRACE(COMP_QOS, DBG_LOUD, ("QosCalcUsedTimeAndAdmitPacket(): Used time > Medium time\n"));

				switch (eACI)
				{
					case AC3_VO:
						eACI = AC2_VI;
						pTcb->priority = 5;
						break;
					case AC2_VI:
						eACI = AC0_BE;
						pTcb->priority = 3;
						break;
					case AC1_BK:
						bReturn = FALSE;
						bDetermined = TRUE;						
						break;
					case AC0_BE:
						eACI = AC1_BK;
						pTcb->priority = 2;
						break;	
				}
				
			}
			else
			{
				pStaQos->acm[eACI].UsedTime = AccUsedTime;
				bDetermined = TRUE;
			}
		}
		else
			bDetermined = TRUE;

	} while (!bDetermined);

	return bReturn;
}

//
//	Description:
//		Return all pending tx msdu buffered during ADDTS period.
//
//	Assumption:
//		RT_TX_SPINLOCK is acquired.
//
VOID
QosReturnAllPendingTxMsdu(
	IN PADAPTER			Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	PQOS_TSTREAM	pTs;
	PRT_LIST_ENTRY	pList, pEntry;
	u1Byte			i;
	PRT_TCB			pTcb;
	

	for(i = 0; i < MAX_STA_TS_COUNT; i ++)
	{
		pTs = &(pStaQos->StaTsArray[i]);
		if(pTs->bUsed)
		{
			while ( !RTIsListEmpty(&(pTs->BufferedPacketList)) )
			{
				pTcb = (PRT_TCB)RTRemoveHeadList(&(pTs->BufferedPacketList));

				ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
			}
		}
	}

	pList = RtGetAllValuesFromHashTable(pStaQos->hApTsTable);
	for(pEntry = pList->Flink;
		pEntry != pList;
		pEntry = pEntry->Flink)
	{
		pTs = (PQOS_TSTREAM)pEntry;
		RT_ASSERT(pTs->bUsed, ("QosReturnAllPendingTxMsdu(): pTs(%p) bUsed==FALSE !!!\n", pTs));
		while ( !RTIsListEmpty(&(pTs->BufferedPacketList)) )
		{
			pTcb = (PRT_TCB)RTRemoveHeadList(&(pTs->BufferedPacketList));

			ReturnTCB(Adapter, pTcb, RT_STATUS_SUCCESS);
		}
	}
	
}

//
// Description:
//	WMM Add Ts(Tspec) request packet.
//
RT_STATUS
OnAddTsReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	pu1Byte			pTaddr;
	pu1Byte			pRaddr;

	if (!ACTING_AS_AP(Adapter))
	{
		RT_TRACE_F(COMP_QOS, DBG_WARNING, ("[WARNING] Not AP mode!\n"));
		return RT_STATUS_INVALID_STATE;
	}

	// WMM fixed fields: CategoryCode(1) + ActionCode(1) + DialogToken(1) + StatusCode(1)
	if(posMpdu->Length < sMacHdrLng + 4)
	{
		RT_TRACE(COMP_QOS, DBG_SERIOUS, ("OnAddTsReq(): Invalid length(%d) of frame\n", posMpdu->Length));
		return RT_STATUS_MALFORMED_PKT;
	}

	pTaddr = Frame_Addr2(*posMpdu);
	pRaddr = Frame_Addr1(*posMpdu);

	QosParsingTrafficStreamIE(Adapter, 
								posMpdu, 
								(sMacHdrLng + 4), // The first IE in this action frame.
								QOSIE_SRC_ADDTSREQ, 
								FALSE );

	return RT_STATUS_SUCCESS;
}


//
//	Description:
//		Handle the ADDDTS rsp received.
//	2007.06.26, by shien chang.
//
RT_STATUS
OnAddTsRsp(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PSTA_QOS		pStaQos = pMgntInfo->pStaQos;
	PQOS_TSTREAM	pTs;
	u1Byte			i;
	u1Byte			DialogToken, StatusCode;
	u1Byte				CurrCcxVerNumber = 0;
	
	if(ACTING_AS_AP(Adapter))
		return RT_STATUS_SUCCESS;

	// WMM fixed fields: CategoryCode(1) + ActionCode(1) + DialogToken(1) + StatusCode(1)
	if(posMpdu->Length < sMacHdrLng + 4)
	{
		RT_TRACE(COMP_QOS, DBG_SERIOUS, ("OnAddTsRsp(): Invalid length(%d) of frame\n", posMpdu->Length));
		return RT_STATUS_MALFORMED_PKT;
	}

	DialogToken = GET_ACTFRAME_DIALOG_TOKEN(posMpdu->Octet);
	StatusCode = GET_ACTFRAME_STATUS_CODE(posMpdu->Octet);

	RT_TRACE(COMP_QOS, DBG_LOUD, ("===> OnAddTsRsp(): StatusCode(%d)\n", StatusCode));

	// Cancel the ADDTS timer
	for (i = 0; i < MAX_STA_TS_COUNT; i ++)
	{
		pTs = &(pStaQos->StaTsArray[i]);
		if (pTs && pTs->bUsed)
		{
			if ( pTs->DialogToken==DialogToken )
				pTs->TimeSlotCount = 0;
		}
	}
		
	CCX_QueryVersionNum(Adapter, &CurrCcxVerNumber);

	if (CurrCcxVerNumber >= 4)
	{
		if (CCX_QosReturnAllPendingTxMsdu(Adapter))
			return RT_STATUS_SUCCESS;
	}

	if (StatusCode == 0)
	{ // Success.
		QosParsingTrafficStreamIE(Adapter, 
									posMpdu, 
									(sMacHdrLng + 4), 
									QOSIE_SRC_ADDTSRSP, 
									TRUE );
	}
	else
	{ // Failed.
		if (CurrCcxVerNumber >= 4)
		{			
			RT_TRACE(COMP_QOS, DBG_LOUD, ("OnAddTsRsp(): ADDTS failed, try to roaming...\n"));

			CCX_OnAddTsRspSet(Adapter);
			
			MgntActSet_802_11_BSSID_LIST_SCAN(Adapter);			
		}
		else
		{
			for (i = 0; i < MAX_STA_TS_COUNT; i ++)
			{
				pTs = &(pStaQos->StaTsArray[i]);
				if (pTs && pTs->bUsed)
				{
					if ( pTs->DialogToken==DialogToken )
					{
						pTs->TimeSlotCount = 0;
						if (pTs->bEstablishing)
							QosRemoveTs(Adapter, pTs);					
					}				
				}
			}
		}

	}
			
	//return (StatusCode == 0);
	return RT_STATUS_SUCCESS;
}

//
//	Description:
//		Handle the DELTS received.
//	2007.06.26, by shien chang.
//
RT_STATUS
OnDelTs(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	pu1Byte			pTaddr;
	pu1Byte			pRaddr;

	// WMM fixed fields: CategoryCode(1) + ActionCode(1) + DialogToken(1) + StatusCode(1)
	if(posMpdu->Length < sMacHdrLng + 4)
	{
		RT_TRACE(COMP_QOS, DBG_SERIOUS, ("OnDelTs(): Invalid length(%d) of frame\n", posMpdu->Length));
		return RT_STATUS_MALFORMED_PKT;
	}

	pTaddr = Frame_Addr2(*posMpdu);
	pRaddr = Frame_Addr1(*posMpdu);

	QosParsingTrafficStreamIE(Adapter, 
								posMpdu, 
								(sMacHdrLng + 4), 
								QOSIE_SRC_DELTS, 
								FALSE );

	return RT_STATUS_SUCCESS;
}

//
// Description:
//	Determine if this packet is the trigger frame for the specified sta UAPSD setting.
// Arguments:
//	[in] pMpduOS -
//		The packet to be checked.
//	[in] StaUapsd -
//		The UAPSD setting for the client.
// Return:
//	TRUE if this packet is the trigger frame.
//	FALSE if this packet isn't the trigger frame.
// By Bruce, 2010-10-05.
//
BOOLEAN
IsStaQosTriggerFrame(
	IN	POCTET_STRING		pOSMpdu,
	IN	AC_UAPSD			StaUapsd
	)
{
	BOOLEAN		bTriggerAC = FALSE;
	if(!IsQoSDataFrame(pOSMpdu->Octet))
		return FALSE;

	switch(GET_QOS_CTRL_WMM_UP(pOSMpdu->Octet))
	{
	case 0:
	case 3:
		bTriggerAC = GET_BE_UAPSD(StaUapsd);
		break;
	case 1:
	case 2:
		bTriggerAC=GET_BK_UAPSD(StaUapsd);
		break;
	case 4:
	case 5:
		bTriggerAC = GET_VI_UAPSD(StaUapsd);
		break;
	case 6:
	case 7:
		bTriggerAC = GET_VO_UAPSD(StaUapsd);
		break;
	}

	return bTriggerAC;
}

//
// Description: Reset Rx TS for initializing sequence number by sending DELBA.
//
// Added by tynli. 2015.01.22.
//
VOID
QosResetRxTS(
	IN	PADAPTER		Adapter
)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT 	pHTInfo = GET_HT_INFO(pMgntInfo);
	PRX_TS_RECORD			pRxTs = NULL;

	if(pHTInfo->bAcceptAddbaReq)
	{
		if(GetTs(Adapter, (PTS_COMMON_INFO*)(&pRxTs), pMgntInfo->Bssid, 0, RX_DIR, FALSE))
		{
			RT_TRACE(COMP_QOS, DBG_LOUD, ("QosResetRxTS(): call TsInitDelBA()\n"));
			TsInitDelBA(Adapter, (PTS_COMMON_INFO)pRxTs, RX_DIR);
		}
	}
}

