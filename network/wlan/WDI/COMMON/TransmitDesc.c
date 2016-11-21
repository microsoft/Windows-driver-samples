#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "TransmitDesc.tmh"
#endif

/**
* Function:	MgntQuery_ProtectionFrame
*
* Overview:	Decide whether protection mode is required to be enabled
*			If protection is required to be enabled, decide how to send protection frame
* 
* Input:		
*		PADAPTER		Adapter,
*		PRT_TCB			pTcb
*
* Output:				
*		PRT_TCB			pTcb
*
*			(Following content of TCB will be changed)
*			pTcb->bRTSEnable
*			pTcb->bCTSEnable
*			pTcb->RTSRate
*			pTcb->RTSSC
*			pTcb->RTSCCA
*			pTcb->RTSBW
*			pTcb->bRTSSTBC
*			pTcb->bRTSShort
*			pTcb->bNeedAckReply
*			pTcb->bNeedCRCAppend
*
* Return:     	None
*/
BOOLEAN
_IOT_ProtectionFrame(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	)
{
	BOOLEAN		Ret = FALSE;
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;

	if(pMgntInfo->IOTAction & HT_IOT_ACT_FORCED_RTS)
	{
		pTcb->bRTSEnable 	= TRUE;
		pTcb->bCTSEnable 	= FALSE;
		pTcb->RTSRate		= MGN_11M;
		Ret = TRUE;
	}
	else if(pMgntInfo->IOTAction & HT_IOT_ACT_FORCED_CTS2SELF)
	{
		pTcb->bRTSEnable 	= FALSE;
		pTcb->bCTSEnable 	= TRUE;
		Ret = TRUE;
	}
	
	return Ret; 
}

void _RTS_CTS_settingframe(
					PADAPTER		Adapter,
					PRT_TCB			pTcb
)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;

	if(_IOT_ProtectionFrame(Adapter, pTcb))
		return;
	
	if(Adapter->RegWirelessMode <= WIRELESS_MODE_G)
	{
		pTcb->bRTSEnable 	= pMgntInfo->bForcedProtectRTSFrame;
		pTcb->bCTSEnable 	= pMgntInfo->bForcedProtectCTSFrame;
	}
	else
	{
		pTcb->bRTSEnable 	= TRUE;
		pTcb->bCTSEnable 	= FALSE;
	}
}


BOOLEAN
_HT_ProtectionFrame(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
)
{
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT 		pHTInfo = pMgntInfo->pHTInfo;
	u1Byte 				HTOpMode = pHTInfo->CurrentOpMode;
	BOOLEAN 			bIsPeerDynamicMimoPs	= FALSE;

	if(pHTInfo->bCurrentHTSupport == FALSE)
		return FALSE;

	//3 Check HTInfo Operation Mode
	if((HTOpMode == HT_OPMODE_MIXED) && 
		(CHNL_RUN_ABOVE_40MHZ(pMgntInfo)  || 
		!CHNL_RUN_ABOVE_40MHZ(pMgntInfo)))
	{
		pTcb->RTSRate = MGN_24M; // Rate is 24Mbps.
		pTcb->bRTSEnable = TRUE;
		return TRUE;
	}
	

	//3 Check Dynamic MIMO Power save condition
	if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
	{
		PRT_WLAN_STA pEntry = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);

		if(pEntry != NULL)
		{
			if(pEntry->HTInfo.MimoPs == MIMO_PS_DYNAMIC)
				bIsPeerDynamicMimoPs = TRUE;
		}
	}
	else
	{
		if(pHTInfo->PeerMimoPs == MIMO_PS_DYNAMIC)
			bIsPeerDynamicMimoPs = TRUE;
	}

	if(bIsPeerDynamicMimoPs)
	{
		pTcb->RTSRate = MGN_24M; // Rate is 24Mbps.
		pTcb->bRTSEnable = TRUE;
		return TRUE;
	}

	//3 Check A-MPDU aggregation for TXOP
	if(pTcb->bAMPDUEnable)
	{
		pTcb->RTSRate = MGN_24M; // Rate is 24Mbps.
		// According to 8190 design, firmware sends CF-End only if RTS/CTS is enabled. However, it degrads
		// throughput around 10M, so we disable of this mechanism. 2007.08.03 by Emily
		if(DISABLE_RTS_ON_AMPDU(Adapter))
			pTcb->bRTSEnable = FALSE;
		else
		{
			pTcb->bRTSEnable = TRUE;
		
			if(IsFirstGoAdapter(Adapter) && 
				(AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress) != NULL))
			{
				pTcb->bRTSEnable = FALSE;
				pTcb->bCTSEnable = TRUE;
				pTcb->bHwProtection = TRUE;
				if(!Adapter->RASupport)
					pTcb->RTSRate = MGN_24M; // Rate is 24Mbps.				
			}
		}
		
		return (pTcb->bRTSEnable||pTcb->bCTSEnable);
	}
	else
	{
		if(IsFirstGoAdapter(Adapter) && 
			(AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress) != NULL))
		{				
			pTcb->bRTSEnable = FALSE;
			pTcb->bCTSEnable = TRUE;
			if(!Adapter->RASupport)
				pTcb->RTSRate = MGN_24M; // Rate is 24Mbps.

			return (pTcb->bRTSEnable||pTcb->bCTSEnable);
		}
	}

	return FALSE;
}


VOID
MgntQuery_ProtectionFrame(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	)
{
	pu1Byte 		pFrame = GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb);
	PMGNT_INFO	pMgntInfo = &Adapter->MgntInfo;
	u1Byte 		RtsRate = 0;

	// Common Settings
	pTcb->bRTSSTBC		= FALSE;
	pTcb->bRTSShort		= FALSE; // Since protection frames are always sent by legacy rate, ShortGI will never be used.
	pTcb->bCTSEnable	= FALSE; // Most of protection using RTS/CTS
	pTcb->RTSSC			= 0;		// 20MHz: Don't care;  40MHz: Duplicate.
	pTcb->RTSCCA			= 0;
	pTcb->RTSBW			= 0; // RTS frame bandwidth is always 20MHz


	//When Bt only, we do not need to send RTS/CTS for better TP.
	//But when connect to N mode AP with cuncurrent, we need send RTS/CTS
	if(pTcb->bBTTxPacket && pMgntInfo->mAssoc)
	{	
		pTcb->bRTSSTBC			= FALSE;
		pTcb->bRTSShort			= FALSE; // Since protection frames are always sent by legacy rate, ShortGI will never be used.
		pTcb->bCTSEnable		= FALSE; // Most of protection using RTS/CTS
		pTcb->RTSSC			= 0;		// 20MHz: Don't care;  40MHz: Duplicate.
		pTcb->RTSCCA			= 0;
		pTcb->RTSBW			= 0; // RTS frame bandwidth is always 20MHz	
		//pTcb->bRTSEnable		=TRUE;
		//pTcb->RTSRate			=MGN_11M;
		return;
	}
	else if(IsFrameTypeCtrl(pFrame))
	{	// Check Control type frame: Control frame do not need protection!!
		pTcb->bNeedAckReply		= FALSE;
		pTcb->bNeedCRCAppend	= FALSE;
		goto NO_PROTECTION;
	}
	else if(pTcb->bBroadcast || pTcb->bMulticast)
	{	// Check multicast/broadcast: Protection doesn't apply to broadcast and multicast frame!!
		pTcb->bNeedAckReply		= FALSE;
		pTcb->bNeedCRCAppend	= TRUE;
		goto NO_PROTECTION;
	}	
	else
	{	// Unicast packet case
		pTcb->bNeedAckReply		= TRUE;
		pTcb->bNeedCRCAppend	= TRUE;

		if(MacAddr_isBcst((pFrame+16)))
			goto NO_PROTECTION;

		// hpfan_add for debug dynamic 20MHz with marvell
		else if(pTcb->specialDataType != PACKET_NORMAL)
		{
			goto NO_PROTECTION;
		}

		//3 Forced Protection mode
		if(pMgntInfo->ForcedProtectionMode == PROTECTION_MODE_FORCE_ENABLE)
		{				
			_RTS_CTS_settingframe(Adapter, pTcb);
			pTcb->RTSBW		= pMgntInfo->ForcedProtectBW;
			pTcb->RTSSC		= pMgntInfo->ForcedProtectSC;
			pTcb->RTSCCA		= pMgntInfo->ForcedProtectCCA;
			pTcb->RTSRate		= pMgntInfo->ForcedProtectRate;				
			return;
		}
		else if(pMgntInfo->ForcedProtectionMode == PROTECTION_MODE_FORCE_DISABLE)
		{			
			goto NO_PROTECTION;
		}

		 //3  RTS Threshold, but think about AMSDU in N mode
		if ( (pTcb->FragLength[0] > MgntActQuery_802_11_RTS_THRESHOLD(Adapter)) && (pTcb->bAggregate == FALSE))
		{
			if(IS_WIRELESS_MODE_N(Adapter))
			{				
				pTcb->RTSRate = MGN_24M; // Rate is 24Mbps.
			}	
			else
			{
				pTcb->RTSRate = ComputeAckRate( pMgntInfo->mBrates, pMgntInfo->HighestBasicRate );
			}

			pTcb->bRTSEnable = TRUE;
		}
		 //3 ERP Use_Protection
		else if(pMgntInfo->bUseProtection && IS_CCK_RATE(pTcb->DataRate) == FALSE)
		{
			pTcb->RTSRate = MGN_11M; // Rate is 11Mbps.
			_RTS_CTS_settingframe(Adapter, pTcb);
		}
		//3 11n High throughput case.
		else
		{
			while(TRUE)
			{
				// Temporarily disable IOT protection setting
				if(_IOT_ProtectionFrame(Adapter, pTcb))
					break;

				if(_HT_ProtectionFrame(Adapter, pTcb))
					break;

				// Totally no protection case!!
				goto NO_PROTECTION;
			}
		}
	}

	if(IS_88E_SERIES(Adapter))
	{
		//
		// <Roger_TODO> We should take RTL8723B into consideration, 2012.10.08
		//
		Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_INIT_RTS_RATE, (pu1Byte)(&RtsRate));
		pTcb->RTSRate = Adapter->HalFunc.GetHwRateFromMRateHandler(RtsRate);
	}

	
	// Determine RTS frame preamble mode.
	if(pTcb->RTSRate == MGN_1M)
		pTcb->bRTSShort = FALSE;
	else
		pTcb->bRTSShort = MgntIsShortPreambleMode( Adapter, pTcb );	

	return;

NO_PROTECTION:
	pTcb->bRTSEnable	= FALSE;
	pTcb->bCTSEnable	= FALSE;
	pTcb->RTSRate		= 0;
	pTcb->RTSSC		= 0;
	pTcb->RTSCCA		= 0;
	pTcb->RTSBW		= 0;
}

/**
* Function:	MgntQuery_PreambleMode
*
* OverView:	This Function will refer to Data Rate, so call this function after the rate is decided.
*			This function will refer to protection mode, so call this function after MgntQuery_ProtectionFrame().
*			The preamble mode will be returned and set in TCB.
*
* Input:
*			PADAPTER		Adapter,
*			PRT_TCB			pTcb
*
* Output:
*			PRT_TCB			pTcb
*
*				(Following content of TCB will be changed)
*				pTcb->bUseShortPreamble
*
* Return:		None
*/
VOID
MgntQuery_PreambleMode(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	)
{
	// 1M can only use Long Preamble. Ref. 802.11b 18.2.2.2. 2005.01.18, by rcnjko.
	if(pTcb->DataRate == MGN_1M) 
		pTcb->bUseShortPreamble = FALSE;
	else
		pTcb->bUseShortPreamble = MgntIsShortPreambleMode( Adapter, pTcb );
}


/**
* Function:	MgntQuery_TxTime
*
* OverView:	Calculate Tx duration and RTS duration.
*			This function needs to refer to protection mode, so calling this function after 
*			protection mode is determined.
*			This function needs to refer to preamble mode, so calling this function after 
*			protection mode is determined.
* Input:
*			PADAPTER		Adapter,
*			PRT_TCB			pTcb,
*			u1Byte			FragIndex
*
* Output:
*			PRT_TCB			pTcb
*
*				pTcb->TxDescDuration[FragIndex]
*				pTcb->DurationFieldVal[FragIndex]
*
* Return:		None
*/
VOID
MgntQuery_TxTime(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	)
{
	u2Byte	ThisFrameTime;
	u1Byte	AckRate	= 0, CtsRate 	= 0;
	u2Byte	AckTime= 0,	RtsTime	= 0, CtsTime	= 0;
	u1Byte	FragIndex = 0;

	if(pTcb->bTxCalculatedSwDur == FALSE)
		return;

	if(pTcb->bNeedAckReply)
	{
		// Figure out ACK rate according to BSS basic rate and Tx rate, 2006.03.08 by rcnjko.
		AckRate = ComputeAckRate( Adapter->MgntInfo.mBrates, (u1Byte)(pTcb->DataRate) );
		// Figure out ACK time according to the AckRate and assume long preamble is used on receiver, 2006.03.08, by rcnjko.
		AckTime = ComputeTxTime( sAckCtsLng/8, AckRate, FALSE, FALSE);			
	}

	
	for(FragIndex = 0; FragIndex < pTcb->FragCount; FragIndex++)
	{
		//2 Initialize
		pTcb->TxDescDuration[FragIndex] 	= 0;
		pTcb->DurationFieldVal[FragIndex]	= 0;

		//2 Frame transmission time (Frame Body)
		ThisFrameTime = ComputeTxTime(
							pTcb->FragLength[FragIndex]+((pTcb->bNeedCRCAppend)?sCrcLng:0),
							pTcb->DataRate, 
							FALSE, 
							pTcb->bUseShortPreamble);

		pTcb->TxDescDuration[FragIndex] += ThisFrameTime;

		// Add this fragment TxTime to the duration field of previous fragment
		if(FragIndex > 0)
			pTcb->DurationFieldVal[FragIndex-1] += (ThisFrameTime + 2*aSifsTime + AckTime);

			
		//2 Ack transmission time (SIFS + ACK)
		if(pTcb->bNeedAckReply)
		{
			pTcb->TxDescDuration[FragIndex] += (aSifsTime + AckTime);
			pTcb->DurationFieldVal[FragIndex] += (aSifsTime + AckTime);
		}			

		//2Protection frame time (RTS + SIFS + CTS + SIFS)
		if(pTcb->bRTSEnable)
		{
			if(pTcb->bCTSEnable)
			{
				CtsTime = ComputeTxTime( sAckCtsLng/8, pTcb->RTSRate, FALSE, FALSE);

				pTcb->TxDescDuration[FragIndex] += (CtsTime + aSifsTime);
			}
			else
			{
				RtsTime	= ComputeTxTime( sAckCtsLng/8, pTcb->RTSRate, FALSE, FALSE);
				CtsRate	= ComputeAckRate( Adapter->MgntInfo.mBrates, (u1Byte)(pTcb->RTSRate) );
				CtsTime	= ComputeTxTime( sAckCtsLng/8, CtsRate, FALSE, FALSE);

				pTcb->TxDescDuration[FragIndex] += (RtsTime + 2*aSifsTime);
			}
		}
	}
}



/**
* Function:	MgntQuery_AggregationCapability
* 
* Overview:	Query whether the packet is allowed to be AMPDU aggregated.
*			Query the Aggregation Capability. Including MPDUFactor and AMPDUDensity.
* 
* Input:		
* 		PADAPTER	Adapter
*		pu1Byte		dstaddr
*		PRT_TCB		pTcb
* 			
* Output:		
*		PRT_TCB		pTcb
* Return:     	
*		None
*/
VOID
MgntQuery_AggregationCapability(
	PADAPTER	Adapter,
	pu1Byte		dstaddr,
	PRT_TCB		pTcb
	)
{

	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);	
	pu1Byte 					pFrame = GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb);
	PTX_TS_RECORD			pTxTs;
	PRT_WLAN_STA			pEntry = NULL;
#if (P2P_SUPPORT == 1)	
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(Adapter);
#endif

	pTcb->bAMPDUEnable = FALSE;

	if(!pHTInfo->bCurrentHTSupport)
	{
		if(!TDLS_CheckPeer(Adapter, dstaddr))
			return;
	}

	if(GET_SIMPLE_CONFIG_ENABLED(pMgntInfo))
		return;

	if(pTcb->bBTTxPacket || pTcb->bBroadcast || pTcb->bMulticast)
		return;

	//
	// The packet should not be aggregated in legacy power saving mode. By Bruce, 2009-10-12.
	//
	//if(pMgntInfo->dot11PowerSaveMode != eActive)
		//return;
	
	if(!IsQoSDataFrame(pFrame) || IsSecProtEapol(pTcb->EncInfo.SecProtInfo) )
		return;
		
	// Joseph 20090617: Fix TP low due to DHCP frame transmission.
	// Some weird Ralink AP lease IP address only 60s. This produces a lot of DHCP frames.
	// We just stop A-MPDU aggregation within first 4s after connection.
	// At the rest of time, we do not aggregate DHCP and ARP packets and do not affect other packets.
	if(	(pTcb->specialDataType == PACKET_DHCP||pTcb->specialDataType == PACKET_ARP||pTcb->specialDataType == PACKET_EAPOL)||//pTcb->specialDataType != PACKET_NORMAL || //change by ylb 20141222, Fix ICMP cannot aggregate AMPDU
		((GetDefaultAdapter(Adapter)==Adapter) && 
		 (GetDefaultAdapter(Adapter)->MgntInfo.CntAfterLink<2))	)
		return;
		
	if(pMgntInfo->IOTAction & HT_IOT_ACT_TX_NO_AGGREGATION)
		return;
	
	// For RTL819X, if pairwisekey = wep/tkip, we don't aggrregation.
	if(!Adapter->HalFunc.GetNmodeSupportBySecCfgHandler(Adapter))
		return; 

	if(WAPI_QuerySetVariable(Adapter, WAPI_QUERY, WAPI_VAR_NOTSETENCMACHEADER, 0))
		return;

	// Note : pHTInfo->bCurrentAMPDUEnable should be enabled only when AMPDU is enabled 
	// in registry and current operation mode is HT. 
	if(pHTInfo->bCurrentAMPDUEnable == FALSE)
		return;

	pTcb->AMPDUBufferSize = 0;

	if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
	{
		pEntry = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);
		if(pEntry==NULL)
		{
			RT_PRINT_ADDR(COMP_SEND, DBG_LOUD, "Unknown entry address\n", pTcb->DestinationAddress);
			RT_TRACE(COMP_SEND, DBG_LOUD, ("A-MPDU setting failed 3\n"));
			return;
		}
		else if(pEntry->WirelessMode < WIRELESS_MODE_N_24G)
			return;
		else if(pEntry->IOTAction & HT_IOT_ACT_AMSDU_ENABLE)
			return;
		else if(pTcb->PacketLength >= 4096)
		{
			if(pEntry->IOTAction & HT_IOT_ACT_AMSDU_AMPDU)
				;
			else 
				return;
		}
	}
	else if(pTcb->PacketLength >= 4096)
	{
		if(pMgntInfo->IOTAction & HT_IOT_ACT_AMSDU_AMPDU)
			;
		else 
			return;
	}


	//
	// This part shall be integrate to other function.
	//
	RT_TRACE(COMP_QOS, DBG_TRACE, ("GetTs=%d\r\n", pMgntInfo->SecurityInfo.SecLvl));
	
	if(GetTs(Adapter, (PTS_COMMON_INFO*)(&pTxTs), dstaddr, pTcb->priority, TX_DIR, TRUE))
	{
		if(pTxTs->TxAdmittedBARecord.bValid == FALSE)
		{
			//
			// Caution: Now we just establish ADDBA protocol and use A-MPDU for 
			// every transmission.
			//
			// For Vwifi to support AMPDU, while keep the default port behavior, by Bohn , 2009.10.01
			//For Vwifi to support Tx AMPDU, 2009.11.20, by Bohn
			BOOLEAN bSecIsTxKeyInstalled;
			if (ACTING_AS_AP(Adapter))
			{// AP mode
				if(!MacAddr_isMulticast(pTcb->DestinationAddress))
				{
					//For AP mode, now is able to accept AddBaReq and other Action Frame. 2009.08.20, by Bohn
					PRT_WLAN_STA	pEntry;
					pEntry = AsocEntry_GetEntry(pMgntInfo,pTcb->DestinationAddress);
					if(pEntry == NULL)
						bSecIsTxKeyInstalled = FALSE;
					else if(pEntry->keyindex != 0)
						bSecIsTxKeyInstalled = TRUE;
					else
						bSecIsTxKeyInstalled = FALSE;
				}
				else
				{						
					bSecIsTxKeyInstalled = FALSE;
				}
			}
			else
			{// STA mode or Ad hoc mode
				bSecIsTxKeyInstalled = SecIsTxKeyInstalled(Adapter, pMgntInfo->Bssid);
			}
			
			if(pMgntInfo->SecurityInfo.SecLvl > RT_SEC_LVL_NONE && !bSecIsTxKeyInstalled)
			{
				RT_TRACE(COMP_QOS, DBG_TRACE, ("pMgntInfo->SecurityInfo.SecLv=%d\r\n", pMgntInfo->SecurityInfo.SecLvl));
			}
			else if(pHTInfo->ForcedAMPDUMode != HT_AMPDU_DISABLE)
			{
				RT_TRACE(COMP_QOS, DBG_LOUD, ("pTxTs->TxAdmittedBARecord.bValid == FALSE)\r\n"));
				TsStartAddBaProcess(Adapter, pTxTs);
			}

			goto FORCED_AGG_SETTING;
		}
		else if(pTxTs->bUsingBa == FALSE)
		{
			if(SN_LESS(GET_BA_START_SQECTRL_FIELD_SEQ_NUM(&(pTxTs->TxAdmittedBARecord.BaStartSeqCtrl)), (pTxTs->TxCurSeq+1)%4096))
				pTxTs->bUsingBa = TRUE;
			else
				goto FORCED_AGG_SETTING;
		}


		//2 //(1)Decide whether aggregation is required according to protocol handshake		
		if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
		{
			PRT_WLAN_STA pEntry = AsocEntry_GetEntry(pMgntInfo, dstaddr);
			if(pEntry && pEntry->HTInfo.bEnableHT)
			{
				pTcb->bAMPDUEnable = TRUE;
				pTcb->AMPDUFactor = pEntry->HTInfo.AMPDU_Factor;
				pTcb->AMPDUDensity = pEntry->HTInfo.MPDU_Density;
				pTcb->AMPDUBufferSize = pTxTs->TxAdmittedBARecord.BufferSize;
			}
			else
			{
				pTcb->bAMPDUEnable = FALSE;
			}
		}
		else
		{
			if(	(pTcb->bAggregate) && 
				((pMgntInfo->IOTAction & HT_IOT_ACT_AMSDU_AMPDU) == 0))
				goto FORCED_AGG_SETTING;
		
			pTcb->bAMPDUEnable = TRUE;
			pTcb->AMPDUFactor = pHTInfo->CurrentAMPDUFactor;
			pTcb->AMPDUDensity = pHTInfo->CurrentMPDUDensity;
			pTcb->AMPDUBufferSize = pTxTs->TxAdmittedBARecord.BufferSize;

			if(TDLS_IsTxTDLSPacket(Adapter, pTcb))
			{
				TDLS_GetHtAMPDU(Adapter, dstaddr, pTcb);
			}
		}
		
	}
	else if(pMgntInfo->IOTAction & HT_IOT_ACT_AMSDU_ENABLE)
	{
		// If driver does not use AMPDU by default but use A-MSDU instead, and packet 
		// size is small. We do this because we hope TCP ack packet can be aggregated
		if(pTcb->bAggregate == FALSE && pTcb->PacketLength <= 200 &&
			pMgntInfo->IOTPeer!= HT_IOT_PEER_REALTEK_92SE &&
			pMgntInfo->IOTPeer!=HT_IOT_PEER_REALTEK	)	
		{			
			pTcb->bAMPDUEnable = TRUE;
			pTcb->AMPDUFactor = pHTInfo->CurrentAMPDUFactor;
			pTcb->AMPDUDensity = pHTInfo->CurrentMPDUDensity;
		}
	}
	else if(TDLS_IsTxTDLSPacket(Adapter, pTcb))
	{
		TDLS_CheckAggregation(Adapter, dstaddr, pTcb);	
	}
	
FORCED_AGG_SETTING:
	//2//
	//2 //(2) The OID control may overwrite protocol handshake 
	//2//
	switch(pHTInfo->ForcedAMPDUMode )
	{
		case HT_AMPDU_AUTO:
				// Do Nothing
				if((pMgntInfo->IOTPeerSubtype == HT_IOT_PEER_LINKSYS_E4200_V1) && IS_WIRELESS_MODE_N_5G(Adapter))
				{
					// Temply used to fix long run chariot may drop half ,as AMPDU not enable(Add BA not success).zhiyuan 2012/05/09
					pTcb->bAMPDUEnable = TRUE;
					pTcb->AMPDUDensity = pHTInfo->ForcedMPDUDensity;
					pTcb->AMPDUFactor = pHTInfo->ForcedAMPDUFactor;		
				}				
				break;
				
		case HT_AMPDU_ENABLE:
				pTcb->bAMPDUEnable = TRUE;
				pTcb->AMPDUDensity = pHTInfo->ForcedMPDUDensity;
				pTcb->AMPDUFactor = pHTInfo->ForcedAMPDUFactor;
				break;
				
		case HT_AMPDU_DISABLE:
				pTcb->bAMPDUEnable = FALSE;
				pTcb->AMPDUDensity = 0;
				pTcb->AMPDUFactor = 0;
				break;

		default:
			break;
			
	}	
}


BOOLEAN 
MgntQuery_RA_ShortGI(	
	IN	PADAPTER			Adapter,
	IN	u1Byte				macId,
	IN	PRT_WLAN_STA		pEntry,
	IN	WIRELESS_MODE		WirelessMode,
	IN	CHANNEL_WIDTH		ChnlBW
)
{	
	BOOLEAN						bShortGI;
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT			pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	BOOLEAN	bShortGI20MHz = FALSE,bShortGI40MHz = FALSE, bShortGI80MHz = FALSE;
	
	if(IS_N_WIRELESS_MODE(WirelessMode))
	{
		if(macId == MAC_ID_STATIC_FOR_BROADCAST_MULTICAST)
			bShortGI20MHz = bShortGI40MHz = FALSE;
		else if(pEntry != NULL)
		{
			bShortGI20MHz = pEntry->HTInfo.bShortGI20M;
			bShortGI40MHz = pEntry->HTInfo.bShortGI40M;
		}
		else
		{
			bShortGI20MHz = pHTInfo->bCurShortGI20MHz;
			bShortGI40MHz = pHTInfo->bCurShortGI40MHz;
		}
	}

	if(IS_AC_WIRELESS_MODE(WirelessMode))
	{
		if(macId == MAC_ID_STATIC_FOR_BROADCAST_MULTICAST)
			bShortGI80MHz = FALSE;
		else if(pEntry != NULL)
			bShortGI80MHz = pEntry->VHTInfo.bShortGI80M;
		else
			bShortGI80MHz = pVHTInfo->bCurShortGI80MHz;
	}

	switch(ChnlBW){
		case CHANNEL_WIDTH_40:
			bShortGI = bShortGI40MHz;
			break;
		case CHANNEL_WIDTH_80:
			bShortGI = bShortGI80MHz;
			break;
		default:case CHANNEL_WIDTH_20:
			bShortGI = bShortGI20MHz;
			break;
	}

	return bShortGI;
}	


VOID
MgntQuery_ShortGI(
	PADAPTER	Adapter,
	PRT_TCB		pTcb)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);

	if(pHTInfo->bCurrentHTSupport == FALSE)
		pTcb->bUseShortGI = FALSE;
	else if(pHTInfo->ForcedShortGI==SHORTGI_FORCE_ENABLE)
		pTcb->bUseShortGI = TRUE;
	else if(pHTInfo->ForcedShortGI==SHORTGI_FORCE_DISABLE)
		pTcb->bUseShortGI = FALSE;
	else if(MgntIsDataOnlyFrame(Adapter, pTcb) == FALSE)
		pTcb->bUseShortGI = FALSE;
	else
	{
		u1Byte bUseSGI;

		if(Adapter->RASupport)
		{
			bUseSGI = pTcb->macId; 
			Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_RA_SGI, (pu1Byte)&bUseSGI);
			if(bUseSGI)
				pTcb->bUseShortGI = TRUE;
			else
				pTcb->bUseShortGI = FALSE;
		}
		else
		{
			OCTET_STRING				osMpdu;
			FillOctetString(osMpdu, GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);

			if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
			{
				pu1Byte 			pRaddr = Frame_pRaddr(osMpdu);
				PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, pRaddr);
				if(pEntry == NULL)
					pTcb->bUseShortGI = FALSE;
				else	 if(pTcb->bTxUseDriverAssingedRate && pTcb->bSpecifShortGI == TRUE)
					;
				else
					 pTcb->bUseShortGI = MgntQuery_RA_ShortGI(Adapter, pTcb->macId, pEntry, pEntry->WirelessMode, pEntry->BandWidth);
			}
			else
				pTcb->bUseShortGI = MgntQuery_RA_ShortGI(Adapter, pTcb->macId, NULL, pMgntInfo->dot11CurrentWirelessMode, pMgntInfo->dot11CurrentChannelBandWidth);
		}
	}
}	/* MgntQuery_ShortGI */


VOID
MgntQuery_Tx_LDPC(
	PADAPTER	Adapter,
	PRT_TCB		pTcb)
{
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	OCTET_STRING				osMpdu;

	FillOctetString(osMpdu, GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);

	if(pTcb->bMulticast || pTcb->bBroadcast || pTcb->DataRate < MGN_MCS0)
		pTcb->bLDPC = FALSE;
	else if(IsFrameTypeCtrl(osMpdu.Octet) || IsFrameTypeMgnt(osMpdu.Octet))
		pTcb->bLDPC = FALSE;
	else

	if(pVHTInfo->VhtLdpcCap & LDPC_VHT_TEST_TX_ENABLE)
		pTcb->bLDPC = TRUE;
	else if(pHTInfo->bCurrentHTSupport == FALSE)
		pTcb->bLDPC = FALSE;
	else if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
	{ 
		pu1Byte 		pRaddr = Frame_pRaddr(osMpdu);
		PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, pRaddr);	

		if(pEntry == NULL)
			pTcb->bLDPC = FALSE;
 		else if(IS_AC_WIRELESS_MODE(pEntry->WirelessMode))
 		{
			if(TEST_FLAG(pEntry->VHTInfo.LDPC, LDPC_VHT_ENABLE_TX))
				pTcb->bLDPC = TRUE;
			else
				pTcb->bLDPC = FALSE;
 		}
		else if(IS_N_WIRELESS_MODE(pEntry->WirelessMode))
		{
			if(TEST_FLAG(pEntry->HTInfo.LDPC, LDPC_HT_ENABLE_TX))
				pTcb->bLDPC = TRUE;
			else 
				pTcb->bLDPC = FALSE;
		}
		else
			pTcb->bLDPC = FALSE;
	}
	else
	{
		if(IS_WIRELESS_MODE_AC(Adapter))
		{
			if(TEST_FLAG(pVHTInfo->VhtCurLdpc, LDPC_VHT_ENABLE_TX) )
				pTcb->bLDPC =TRUE;
			else
				pTcb->bLDPC = FALSE;
		}	
		else
		{
			if(TEST_FLAG(pHTInfo->HtCurLdpc, LDPC_HT_ENABLE_TX) )
				pTcb->bLDPC =TRUE;
			else
				pTcb->bLDPC = FALSE;
		}	
	}	
}	/* MgntQuery_LDPC */


VOID
MgntSet_TX_LDPC(
	PADAPTER		Adapter,
	u1Byte			MacId,
	BOOLEAN			bLDPC
	)
{
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT			pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	MAC_ID_OWNER_TYPE			MacIdOwnerType = MacIdGetOwnerType(Adapter, MacId);

	if(MacIdOwnerType == MAC_ID_OWNER_INFRA_AP || MacIdOwnerType == MAC_ID_OWNER_AD_HOC || MacIdOwnerType == MAC_ID_OWNER_TDLS)
	{
		PRT_WLAN_STA	pEntry = AsocEntry_GetEntryByMacId(pMgntInfo, MacId);

		if(pEntry == NULL)
			return;

		if(pEntry->WirelessMode == WIRELESS_MODE_AC_5G || pEntry->WirelessMode == WIRELESS_MODE_AC_24G)
		{
			if(bLDPC && TEST_FLAG(pEntry->VHTInfo.LDPC, LDPC_VHT_CAP_TX))
				SET_FLAG(pEntry->VHTInfo.LDPC, LDPC_VHT_ENABLE_TX);
			else
				CLEAR_FLAG(pEntry->VHTInfo.LDPC, LDPC_VHT_ENABLE_TX);
		}
		else if(pEntry->WirelessMode == WIRELESS_MODE_N_5G || pEntry->WirelessMode == WIRELESS_MODE_N_24G)
		{
			if(bLDPC && TEST_FLAG(pEntry->HTInfo.LDPC, LDPC_HT_CAP_TX))
				SET_FLAG(pEntry->HTInfo.LDPC, LDPC_HT_ENABLE_TX);
			else
				CLEAR_FLAG(pEntry->HTInfo.LDPC, LDPC_HT_ENABLE_TX);

		}
	}
	else if(MacIdOwnerType == MAC_ID_OWNER_DEFAULT_PORT || MacIdOwnerType == MAC_ID_OWNER_INFRA_STA)
	{
		if(IS_WIRELESS_MODE_AC(Adapter))
		{
			if(bLDPC && TEST_FLAG(pVHTInfo->VhtCurLdpc, LDPC_VHT_CAP_TX))
				SET_FLAG(pVHTInfo->VhtCurLdpc, LDPC_VHT_ENABLE_TX);
			else
				CLEAR_FLAG(pVHTInfo->VhtCurLdpc, LDPC_VHT_ENABLE_TX);
		}	
		else
		{
			if(bLDPC && TEST_FLAG(pHTInfo->HtCurLdpc, LDPC_HT_CAP_TX))
				SET_FLAG(pHTInfo->HtCurLdpc, LDPC_HT_ENABLE_TX);
			else
				CLEAR_FLAG(pHTInfo->HtCurLdpc, LDPC_HT_ENABLE_TX);
		}
	}

	RT_DISP(FIOCTL, IOCTL_STATE, ("MacId %d bLDPC %d\n", MacId, bLDPC));
}



VOID
MgntQuery_Tx_STBC(
	IN		PADAPTER	Adapter,
	IN OUT	PRT_TCB		pTcb
	)
{
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT			pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	OCTET_STRING				osMpdu;
	u1Byte						Beamform_cap = 0;

	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T					pDM_Odm = &pHalData->DM_OutSrc;

	FillOctetString(osMpdu, GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);

	if(pTcb->bMulticast || pTcb->bBroadcast || pTcb->DataRate < MGN_MCS0)
		pTcb->bSTBC = FALSE;
	else if(IsFrameTypeCtrl(osMpdu.Octet) || IsFrameTypeMgnt(osMpdu.Octet))
		pTcb->bSTBC = FALSE;
	else

	if(TEST_FLAG(pVHTInfo->VhtStbcCap, STBC_VHT_TEST_TX_ENABLE)) // Test Mode
		pTcb->bSTBC = TRUE;
	else if(pHTInfo->bCurrentHTSupport == FALSE)
		pTcb->bSTBC = FALSE;
	else if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
	{ 
		pu1Byte			pRaddr = Frame_pRaddr(osMpdu);
		PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, pRaddr);	

		if(pEntry == NULL)
			pTcb->bSTBC = FALSE;
 		else
		{ 
			if(IS_AC_WIRELESS_MODE(pEntry->WirelessMode))
 			{
				if(pEntry->VHTInfo.STBC)
				{
					pTcb->bSTBC = TRUE;
				}
				else
					pTcb->bSTBC = FALSE;
	 		}
			else if(IS_N_WIRELESS_MODE(pEntry->WirelessMode))
			{
				if(pEntry->HTInfo.STBC)
				{
					pTcb->bSTBC = TRUE;
				}	
				else 
					pTcb->bSTBC = FALSE;
			}
			else
				pTcb->bSTBC = FALSE;
		}
	}
	else
	{
		if(IS_WIRELESS_MODE_AC(Adapter))
		{
			if(TEST_FLAG(pVHTInfo->VhtCurStbc, STBC_VHT_ENABLE_TX) )
			{
				pTcb->bSTBC = TRUE;
			}
			else
				pTcb->bSTBC = FALSE;
		}	
		else
		{
			if(TEST_FLAG(pHTInfo->HtCurStbc, STBC_HT_ENABLE_TX) )
			{
				pTcb->bSTBC =TRUE;
			}	
			else
				pTcb->bSTBC = FALSE;
		}	
	}
}	


VOID
MgntQuery_BandwidthMode(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	)
{
	PMGNT_INFO					pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT			pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);
	OCTET_STRING				osMpdu;

	FillOctetString(osMpdu, GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb), (u2Byte)pTcb->BufferList[0].Length);
	
	if(pHTInfo->bCurrentHTSupport == FALSE)
		return;
	else if(IsFrameTypeCtrl(osMpdu.Octet) || IsFrameTypeMgnt(osMpdu.Octet))
		return;
	else if(pTcb->bMulticast || pTcb->bBroadcast || pTcb->DataRate < MGN_MCS0)
		return;
	else if(pMgntInfo->IOTAction & HT_IOT_ACT_DISABLE_TX_40_MHZ)
		return;

	if(pVHTInfo->bFixedRTSBW)
	{
		pTcb->BWOfPacket = pVHTInfo->DynamicRTSBW;
		return;
	}

	if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
	{
		pu1Byte 		pRaddr = Frame_pRaddr(osMpdu);
		PRT_WLAN_STA	pEntry = AsocEntry_GetEntry(pMgntInfo, pRaddr);	

		if(pEntry == NULL)
			pTcb->BWOfPacket = CHANNEL_WIDTH_20;
		else
		{
			if((GetDefaultAdapter(Adapter)->MgntInfo.bForceGoTxData20MBw == TRUE) && IsFirstGoAdapter(Adapter))
				pTcb->BWOfPacket = CHANNEL_WIDTH_20;	
			else
				pTcb->BWOfPacket = pEntry->BandWidth;
		}

	}
	else
		pTcb->BWOfPacket = pMgntInfo->dot11CurrentChannelBandWidth;

	if(pTcb->DataRate >= MGN_MCS0 && pTcb->DataRate <= MGN_MCS31)
	{
		if(pTcb->BWOfPacket > CHANNEL_WIDTH_40)
			pTcb->BWOfPacket = CHANNEL_WIDTH_40;
	}

}


VOID
MgntSet_RA_Ratr_Index(
	IN	PMGNT_INFO			pMgntInfo,	
	IN	MAC_ID_OWNER_TYPE	MacIdOwnerType,
	IN	u1Byte				ratr_index,
	IN	PRT_WLAN_STA		pEntry
	)
{
	switch(MacIdOwnerType){
		
	case MAC_ID_OWNER_BT:
		pMgntInfo->BT_RatrInx = ratr_index;
		break;
	case MAC_ID_OWNER_INFRA_STA: 
	case MAC_ID_OWNER_DEFAULT_PORT:
		pMgntInfo->ratr_index = ratr_index;
		break;
	case MAC_ID_OWNER_BROADCAST_MULTICAST:
		pMgntInfo->Multi_RatrInx = ratr_index;
		break;
	case MAC_ID_OWNER_AD_HOC:
	case MAC_ID_OWNER_INFRA_AP:
	case MAC_ID_OWNER_TDLS:
		if(pEntry != NULL)
			pEntry->ratr_index = ratr_index;
		break;
	}
}



VOID
MgntQuery_TxRateSelectMode(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	PRT_WLAN_STA	pEntry;
	pu1Byte			pFrame = GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb);

	if(!IsDataFrame(pFrame))
	{
		pTcb->bTxDisableRateFallBack = TRUE;
		pTcb->bTxUseDriverAssingedRate = TRUE;
	}
	else if(pMgntInfo->ForcedDataRate!= 0)
	{
		if(pMgntInfo->ForcedTxDisableRateFallBack == 2)
			pTcb->bTxDisableRateFallBack = FALSE;
		else
			pTcb->bTxDisableRateFallBack = TRUE;

		pTcb->bTxUseDriverAssingedRate = TRUE;
	}
	else if( (pMgntInfo->ForcedBstDataRate != 0) && (pTcb->bBroadcast || pTcb->bMulticast))
	{
		pTcb->bTxDisableRateFallBack = TRUE;
		pTcb->bTxUseDriverAssingedRate = TRUE;
	}

	// Obtain MacID for This Tx Packet -----------------
	pTcb->macId = (u1Byte) MacIdGetTxMacId(Adapter, pTcb);

	if(pTcb->macId == MAC_ID_STATIC_UNSPECIFIED_MACID)
		return;
	// ---------------------------------------------

	if(!pTcb->bBTTxPacket)
	{
		// STA mode
		if(pMgntInfo->mAssoc && !ACTING_AS_AP(Adapter))
		{		
			//Sinda 20150827, should assign a value to OdmAid to avoud ODM access NULL.
			// 0 mean station entry of default port.
			pTcb->OdmAid = 0;
		}
		else if(pMgntInfo->mIbss || ACTING_AS_AP(Adapter))
		{
			pEntry = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);
			
			if(pEntry != NULL)
			{				
				// This is used for ODM Luke's requirement, they want to use AID corresponding SUM ID num for
				// collect RX PHY status info, we should take multi-port consideration,
				// revised by Roger, 2013.10.15.
				pTcb->OdmAid = pEntry->MultiPortStationIdx;
			}
			else
			{
				//Sinda 20150827, should assign a value to OdmAid to avoud ODM access NULL.
				// 0 mean station entry of default port.
				pTcb->OdmAid = 0;
			}
			
			if(pTcb->bTxUseDriverAssingedRate == FALSE)
			{
				if(pTcb->bBroadcast || pTcb->bMulticast)
				{
						pTcb->DataRate = pMgntInfo->LowestBasicRate; 
					
					pTcb->bTxUseDriverAssingedRate = TRUE;
				}
				else if(pEntry != NULL)
				{
					if((pEntry->AssociatedMacId >= MacIdHalGetMaxHWMacId(Adapter))  && (pEntry->DataRate != 0))
					{	
						pTcb->DataRate = pEntry->DataRate;
						pTcb->bSpecifShortGI = TRUE;
						pTcb->bUseShortGI = pEntry->bUseShortGI;
						pTcb->bTxUseDriverAssingedRate = TRUE;
					}
				}
			}
		}
		else
		{
			pTcb->OdmAid = 0;
		}
	}
	else
	{
		//Sinda 20150827, should assign a value to OdmAid to avoud ODM access NULL.
		// 0 mean station entry of default port.
		pTcb->OdmAid = 0;
	}

	//3 Assign RATR Index
	if(pTcb->bTxDisableRateFallBack == FALSE)
	{
		u1Byte	ratr_index = pMgntInfo->Multi_RatrInx;
		
		if(pTcb->bBTTxPacket)
			ratr_index = pMgntInfo->BT_RatrInx;
		else if(ACTING_AS_AP(Adapter) || ACTING_AS_IBSS(Adapter))
		{//AP
			pEntry = AsocEntry_GetEntry(pMgntInfo, pTcb->DestinationAddress);
			if(pEntry != NULL)
				ratr_index = pEntry->ratr_index;
		}
		else
		{//STA
			ratr_index = pMgntInfo->ratr_index;

			if(TDLS_IsTxTDLSPacket(Adapter, pTcb))
			{
				TDLS_GetTxRatrIndex(Adapter, pTcb->DestinationAddress, &ratr_index);
			}
		}

		pTcb->RATRIndex = ratr_index;

	}
}


/**
* Function:	FillFrameField
* 
* Overview:	Filling the per frame information into each frame header.
*			Now we only insert duration field and seqeunce number in this function.
* 
* Input:		
*	PRT_TCB		pTcb,
*	u2Byte		SeqNum
* 			
* Output:		
*	PRT_TCB		pTcb
*				(Frame in the Buffer of pTcb will be modified)
* 		
* Return:     	
*		None
*/
VOID
FillFrameField(
	PADAPTER	Adapter,	
	PRT_TCB		pTcb,
	u2Byte		SeqNum
	)
{
	u1Byte 	FragIndex, BufferIndex;
	pu1Byte	pFrame = GET_FRAME_OF_FIRST_FRAG(Adapter, pTcb);
	u1Byte	HwDescOffset =0;

	if(TEST_FLAG(pTcb->tcbFlags, RT_TCB_FLAG_RAW_DATA))
		return;

	HwDescOffset =  Adapter->TXPacketShiftBytes; // Default HW Tx packet offset.

	for(FragIndex=0, BufferIndex=0; FragIndex < pTcb->FragCount; FragIndex++)
	{
		if(pTcb->bTxCalculatedSwDur)
		{
			SET_80211_HDR_DURATION(pTcb->BufferList[BufferIndex].VirtualAddress+HwDescOffset, pTcb->DurationFieldVal[FragIndex]);
		}

		// Seq numbers are not assigned to control frames. 
		if(IsCtrlFrame(pFrame) == FALSE)
		{
			SET_80211_HDR_SEQUENCE(pTcb->BufferList[BufferIndex].VirtualAddress+HwDescOffset, SeqNum);
		}	

		BufferIndex += pTcb->FragBufCount[FragIndex];
	}	
}
