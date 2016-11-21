#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "BAGen.tmh"
#endif

VOID
ActivateBAEntry(
	PADAPTER		Adapter,
	PBA_RECORD		pBA,
	u2Byte			Time
	)
{
	RT_TRACE(COMP_HT, DBG_LOUD, ("pBA->bValid = TRUE\r\n"));	
	pBA->bValid = TRUE;
	if(Time != 0)
		PlatformSetTimer(Adapter, &pBA->Timer, Time);
}

VOID
DeActivateBAEntry(
	PADAPTER		Adapter,
	PBA_RECORD		pBA
)
{
	RT_TRACE(COMP_HT, DBG_LOUD, ("pBA->bValid = FALSE\r\n"));
	pBA->bValid = FALSE;
	PlatformCancelTimer(Adapter, &pBA->Timer);
}

VOID
ExtendBAEntry(
	PADAPTER		Adapter,
	PBA_RECORD		pBA
	)
{
//	if(pBA->bValid && pBA->LifeTime!=0)
//	{
//		PlatformCancelTimer(Adapter, &pBA->Timer);
//		PlatformSetTimer(Adapter, &pBA->Timer, pBA->LifeTime);
//	}
}

BOOLEAN
TxTsDeleteBA(
	PADAPTER		Adapter,
	PTX_TS_RECORD	pTxTs
	)
{
	PBA_RECORD		pAdmittedBa = &pTxTs->TxAdmittedBARecord;
	PBA_RECORD		pPendingBa = &pTxTs->TxPendingBARecord;
	BOOLEAN			bSendDELBA = FALSE;

	// Delete pending BA
	if(pPendingBa->bValid)
	{
		DeActivateBAEntry(Adapter, pPendingBa);
		bSendDELBA = TRUE;
	}

	// Delete admitted BA
	if(pAdmittedBa->bValid)
	{
		DeActivateBAEntry(Adapter, pAdmittedBa);
		bSendDELBA = TRUE;
	}

	return bSendDELBA;
}

BOOLEAN
RxTsDeleteBA(
	PADAPTER		Adapter,
	PRX_TS_RECORD	pRxTs
	)
{
	PBA_RECORD		pBa = &pRxTs->RxAdmittedBARecord;
	BOOLEAN			bSendDELBA = FALSE;

	if(pBa->bValid)
	{
		DeActivateBAEntry(Adapter, pBa);
		bSendDELBA = TRUE;
	}

	return bSendDELBA;
}


VOID
ResetBaEntry(
	PBA_RECORD		pBA
	)
{
	pBA->bValid					= FALSE;
	pBA->BaParamSet		= 0;
	pBA->BaTimeoutValue			= 0;
	pBA->DialogToken			= 0;
	pBA->BaStartSeqCtrl	= 0;
}

VOID
ConstructADDBAReq(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr,
	IN 	PBA_RECORD		pBA,
	OUT	pu1Byte			Buffer,
	OUT	pu4Byte			pLength
	)
{
	OCTET_STRING		osADDBAFrame, tmp;

	FillOctetString(osADDBAFrame, Buffer, 0);
	*pLength = 0;

	ConstructMaFrameHdr(
					Adapter, 
					Addr, 
					ACT_CAT_BA, 
					ACT_ADDBAREQ, 
					&osADDBAFrame	);

	// Dialog Token
	FillOctetString(tmp, &pBA->DialogToken, 1);
	PacketAppendData(&osADDBAFrame, tmp);

	// BA Parameter Set
	FillOctetString(tmp, &pBA->BaParamSet, 2);
	PacketAppendData(&osADDBAFrame, tmp);

	// BA Timeout Value
	FillOctetString(tmp, &pBA->BaTimeoutValue, 2);
	PacketAppendData(&osADDBAFrame, tmp);

	// BA Start SeqCtrl
	FillOctetString(tmp, &pBA->BaStartSeqCtrl, 2);
	PacketAppendData(&osADDBAFrame, tmp);

	*pLength = osADDBAFrame.Length;
}

VOID
ConstructADDBARsp(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr,
	IN 	PBA_RECORD		pBA,
	IN	u2Byte			StatusCode,
	OUT	pu1Byte			Buffer,
	OUT	pu4Byte			pLength
	)
{
	OCTET_STRING	osADDBAFrame, tmp;

	FillOctetString(osADDBAFrame, Buffer, 0);
	*pLength = 0;

	ConstructMaFrameHdr(
					Adapter, 
					Addr, 
					ACT_CAT_BA, 
					ACT_ADDBARSP, 
					&osADDBAFrame	);

	// Dialog Token
	FillOctetString(tmp, &pBA->DialogToken, 1);
	PacketAppendData(&osADDBAFrame, tmp);

	// Status Code
	FillOctetString(tmp, &StatusCode, 2);
	PacketAppendData(&osADDBAFrame, tmp);

	// BA Parameter Set
	FillOctetString(tmp, &pBA->BaParamSet, 2);
	PacketAppendData(&osADDBAFrame, tmp);

	// BA Timeout Value
	FillOctetString(tmp, &pBA->BaTimeoutValue, 2);
	PacketAppendData(&osADDBAFrame, tmp);

	*pLength = osADDBAFrame.Length;
}

VOID
ConstructDELBA(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr,
	IN 	PBA_RECORD		pBA,
	IN	TR_SELECT		TxRxSelect,
	IN	u2Byte			ReasonCode,
	OUT	pu1Byte			Buffer,
	OUT	pu4Byte			pLength
	)
{
	OCTET_STRING		osDELBAFrame, tmp;
	u2Byte			DelbaParamSet=0;

	SET_DELBA_PARAM_SET_FIELD_INITIATOR(&DelbaParamSet,  (TxRxSelect==TX_DIR)?1:0 );
	SET_DELBA_PARAM_SET_FIELD_TID(&DelbaParamSet, GET_BA_PARAM_SET_FIELD_TID(&(pBA->BaParamSet))) ;

	FillOctetString(osDELBAFrame, Buffer, 0);
	*pLength = 0;

	ConstructMaFrameHdr(
					Adapter, 
					Addr, 
					ACT_CAT_BA, 
					ACT_DELBA, 
					&osDELBAFrame	);

	// DELBA Parameter Set
	FillOctetString(tmp, &DelbaParamSet, 2);
	PacketAppendData(&osDELBAFrame, tmp);

	// Reason Code
	FillOctetString(tmp, &ReasonCode, 2);
	PacketAppendData(&osDELBAFrame, tmp);

	*pLength = osDELBAFrame.Length;	
}

VOID
SendADDBAReq(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr,
	IN	PBA_RECORD		pBA
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER		pBuf;
	u1Byte					DataRate = MgntQuery_MgntFrameTxRate(Adapter, Addr);

	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>SendADDBAReq()\n"));

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructADDBAReq(Adapter, Addr, pBA, pBuf->Buffer.VirtualAddress, &pTcb->PacketLength);

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

VOID
SendADDBARsp(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr,
	IN 	PBA_RECORD		pBA,
	IN	u2Byte			StatusCode
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER		pBuf;
	u1Byte					DataRate = MgntQuery_MgntFrameTxRate(Adapter, Addr);

	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>SendADDBARsp()\n"));
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructADDBARsp(
					Adapter, 
					Addr,
					pBA,
					StatusCode, 
					pBuf->Buffer.VirtualAddress, 
					&pTcb->PacketLength	);

		if(pTcb->PacketLength != 0)
			MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}
	
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}

VOID
SendDELBA(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Addr,
	IN 	PBA_RECORD			pBA,
	IN	TR_SELECT			TxRxSelect,
	IN	u2Byte				ReasonCode
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER		pBuf;
	u1Byte					DataRate = MgntQuery_MgntFrameTxRate(Adapter, Addr);

	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>SendDELBA()\n"));
	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructDELBA(
				Adapter, 
				Addr,
				pBA,
				TxRxSelect, 
				ReasonCode, 
				pBuf->Buffer.VirtualAddress, 
				&pTcb->PacketLength	);

		if(pTcb->PacketLength != 0)
			MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}
	
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
}


RT_STATUS
OnADDBAReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	mmpdu
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PBA_RECORD			pBA;
	PRX_TS_RECORD		pTS;
	pu1Byte				Addr = Frame_Addr2(*mmpdu);
	u2Byte				StatusCode;
	RT_TRACE(COMP_QOS, DBG_TRACE, ("=====>OnADDBAReq()\n"));
	// CategoryCode(1) + ActionCode(1) + DialogToken(1) + ParameterSet(2) + TimeoutValue(2) + StartSequence(2)
	if(mmpdu->Length < sMacHdrLng + 9)
	{
		RT_TRACE(COMP_QOS, DBG_SERIOUS, ("OnADDBAReq(): Invalid length(%d) of frame\n", mmpdu->Length));
		return RT_STATUS_MALFORMED_PKT;
	}
	//
	// Check Block Ack support.
	// Reject if there is no support of Block Ack.
	//
	if(	(pMgntInfo->pStaQos->CurrentQosMode == QOS_DISABLE) ||
		(pMgntInfo->pHTInfo->bCurrentHTSupport == FALSE) ||
		(pMgntInfo->pHTInfo->bAcceptAddbaReq == FALSE) ||
		(pMgntInfo->IOTAction & HT_IOT_ACT_REJECT_ADDBA_REQ)	)
	{
		StatusCode = ADDBA_STATUS_REFUSED;
		goto OnADDBAReq_Fail;
	}
	
	// Add by hpfan: reject addba when 4-way is not finished
	if(pMgntInfo->bWiFiConfg && 
		pMgntInfo->SecurityInfo.SecLvl > RT_SEC_LVL_NONE && !SecIsTxKeyInstalled(Adapter, pMgntInfo->Bssid))
	{
		StatusCode = ADDBA_STATUS_REFUSED;
		goto OnADDBAReq_Fail;
	}
	
	//
	// Search for related traffic stream.
	// If there is no matched TS, reject the ADDBA request.
	//
	if(	!GetTs(
			Adapter, 
			(PTS_COMMON_INFO*)(&pTS), 
			Addr, 
			GET_BA_FRAME_PARAM_SET_TID(mmpdu->Octet),
			RX_DIR,
			TRUE)	)
	{
		StatusCode = ADDBA_STATUS_REFUSED;
		goto OnADDBAReq_Fail;
	}
	pBA = &pTS->RxAdmittedBARecord;


	//
	// To Determine the ADDBA Req content
	// We can do much more check here, including BufferSize, AMSDU_Support, Policy, StartSeqCtrl...
	// I want to check StartSeqCtrl to make sure when we start aggregation!!!
	//
	if(GET_BA_FRAME_PARAM_SET_BA_POLICY(mmpdu->Octet) == BA_POLICY_DELAYED)
	{
		StatusCode = ADDBA_STATUS_INVALID_PARAM;
		goto OnADDBAReq_Fail;
	}

	//
	// Clear all packets queued in driver for RxReorder and prepare to receive packet from new
	// new sequence start!!
	// This fix the protocol IOT issue with Netgear WRT3500.
	//

	FlushRxTsPendingPkts(Adapter, pTS);

	//
	// Admit the ADDBA Request
	//
	DeActivateBAEntry(Adapter, pBA);	
	PlatformMoveMemory(&(pBA->DialogToken) , GET_BA_FRAME_DIALOG_TOKEN(mmpdu->Octet), 1);
	PlatformMoveMemory(&(pBA->BaParamSet) , GET_BA_FRAME_PARAM_SET(mmpdu->Octet), 2);
	PlatformMoveMemory(&(pBA->BaTimeoutValue), GET_BA_FRAME_TIMEOUT_VALUE(mmpdu->Octet), 2); 
	PlatformMoveMemory(&(pBA->BaStartSeqCtrl), GET_BAREQ_FRAME_START_SQECTRL(mmpdu->Octet), 2); 

	{
		SET_BA_PARAM_SET_FIELD_BUF_SIZE( &(pBA->BaParamSet), 64); // At least, forced by SPEC
		RT_TRACE(COMP_QOS, DBG_LOUD, ("OnADDBAReq(): SET_FIELD_BUF_SIZE = 64\n"));
	}

	ActivateBAEntry(Adapter, pBA, 0);
	SendADDBARsp(Adapter, Addr, pBA, ADDBA_STATUS_SUCCESS);

	// End of procedure.
	return RT_STATUS_SUCCESS;

OnADDBAReq_Fail:
	{
		BA_RECORD	BA;
		PlatformMoveMemory(&(BA.BaParamSet), GET_BA_FRAME_PARAM_SET(mmpdu->Octet), 2);
		PlatformMoveMemory(&(BA.BaTimeoutValue), GET_BA_FRAME_TIMEOUT_VALUE(mmpdu->Octet), 2);
		PlatformMoveMemory(&(BA.DialogToken), GET_BA_FRAME_DIALOG_TOKEN(mmpdu->Octet), 1);		
		SET_BA_PARAM_SET_FIELD_BA_POLICY(&(BA.BaParamSet), BA_POLICY_IMMEDIATE);
		SendADDBARsp(Adapter, Addr, &BA, StatusCode);
		RT_TRACE(COMP_QOS, DBG_LOUD, ("OnADDBAReq(): OnADDBAReq_Fail\n"));
	}
	return RT_STATUS_SUCCESS;
}

RT_STATUS
OnADDBARsp(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	mmpdu
    )
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PBA_RECORD			pPendingBA, pAdmittedBA;
	PTX_TS_RECORD		pTS;
	pu1Byte				Addr = Frame_Addr2(*mmpdu);
	pu1Byte				pDialogToken =  GET_BA_FRAME_DIALOG_TOKEN(mmpdu->Octet);
	pu1Byte				pStatusCode =  GET_BA_FRAME_STATUS_CODE(mmpdu->Octet);
	u2Byte				ReasonCode = ADDBA_STATUS_INVALID_PARAM;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>OnADDBARsp()\n"));
	// CategoryCode(1) + ActionCode(1) + DialogToken(1) + StatusCode(2) + ParameterSet(2) + TimeoutValue(2)
	if(mmpdu->Length < sMacHdrLng + 9)
	{
		RT_TRACE(COMP_QOS, DBG_SERIOUS, ("OnADDBARsp(): Invalid length(%d) of frame\n", mmpdu->Length));
		return RT_STATUS_MALFORMED_PKT;
	}

	//
	// Check the capability
	// Since we can always receive A-MPDU, we just check if it is under HT mode.
	//
	if(pMgntInfo->pStaQos->CurrentQosMode == QOS_DISABLE ||
		pMgntInfo->pHTInfo->bCurrentHTSupport == FALSE ||
		pMgntInfo->pHTInfo->bCurrentAMPDUEnable == FALSE )
	{
		ReasonCode = DELBA_REASON_UNKNOWN_BA;
		goto OnADDBARsp_Reject;
	}
	
	//
	// Search for related TS.
	// If there is no TS found, we wil reject ADDBA Rsp by sending DELBA frame.
	//
	if (!GetTs(
			Adapter, 
			(PTS_COMMON_INFO*)(&pTS), 
			Addr, 
			GET_BA_FRAME_PARAM_SET_TID(mmpdu->Octet),
			TX_DIR,
			FALSE)	)
	{
		ReasonCode = DELBA_REASON_UNKNOWN_BA;
		goto OnADDBARsp_Reject;
	}
	pTS->bAddBaReqInProgress = FALSE;
	pPendingBA = &pTS->TxPendingBARecord;
	pAdmittedBA = &pTS->TxAdmittedBARecord;


	//
	// Check if related BA is waiting for setup.
	// If not, reject by sending DELBA frame.
	//
	if((pAdmittedBA->bValid==TRUE))
	{
		// Since BA is already setup, we ignore all other ADDBA Response.
		RT_TRACE(COMP_QOS, DBG_LOUD, ("OnADDBARsp(): Recv ADDBA Rsp. Drop because already admit it! \n"));
		return RT_STATUS_PKT_DROP;
	}
	else if((pPendingBA->bValid == FALSE) ||(*pDialogToken != pPendingBA->DialogToken))
	{
		RT_TRACE(COMP_QOS, DBG_LOUD, ("OnADDBARsp(): Recv ADDBA Rsp. BA invalid, DELBA! \n"));
		ReasonCode = DELBA_REASON_UNKNOWN_BA;
		goto OnADDBARsp_Reject;
	}
	else
	{
		RT_TRACE(COMP_QOS, DBG_LOUD, ("OnADDBARsp(): Recv ADDBA Rsp. BA is admitted! Status code:%X\n", (*pStatusCode) ));
		DeActivateBAEntry(Adapter, pPendingBA);
	}


	if((*pStatusCode) == ADDBA_STATUS_SUCCESS)
	{
		//
		// Determine ADDBA Rsp content here.
		// We can compare the value of BA parameter set that Peer returned and Self sent.
		// If it is OK, then admitted. Or we can send DELBA to cancel BA mechanism.
		//
		if(GET_BA_FRAME_PARAM_SET_BA_POLICY(mmpdu->Octet) == BA_POLICY_DELAYED)
		{
			// Since this is a kind of ADDBA failed, we delay next ADDBA process.
			pTS->bAddBaReqDelayed = TRUE;
		
			DeActivateBAEntry(Adapter, pAdmittedBA);
			ReasonCode = DELBA_REASON_END_BA;
			goto OnADDBARsp_Reject;
		}


		//
		// Admitted condition
		//
		PlatformMoveMemory(&(pAdmittedBA->DialogToken) , GET_BA_FRAME_DIALOG_TOKEN(mmpdu->Octet), 1);
		PlatformMoveMemory(&(pAdmittedBA->BaTimeoutValue), GET_BA_FRAME_TIMEOUT_VALUE(mmpdu->Octet), 2);
		PlatformMoveMemory(&(pAdmittedBA->BaStartSeqCtrl), &(pPendingBA->BaStartSeqCtrl), 2);
		PlatformMoveMemory(&(pAdmittedBA->BaParamSet) , GET_BA_FRAME_PARAM_SET(mmpdu->Octet), 2);
		pAdmittedBA->BufferSize = GET_BA_FRAME_PARAM_SET_BUF_SIZE(mmpdu->Octet);		

		if(0 == pAdmittedBA->BufferSize)
		{
			RT_PRINT_ADDR(COMP_HT, DBG_WARNING, "[WARNING]OnADDBARsp(): Incorrect BufferSize (0) in AddBaRsp, skip this BlockAck process for Addr = ", Addr);
			ReasonCode = ADDBA_STATUS_INVALID_PARAM;
			goto OnADDBARsp_Reject;
		}
		DeActivateBAEntry(Adapter, pAdmittedBA);
		ActivateBAEntry(Adapter, pAdmittedBA, 0 );
	}
	else
	{
		// Delay next ADDBA process.
		pTS->bAddBaReqDelayed = TRUE;
	}

	// End of procedure
	if(Adapter->TDLSSupport)
	{
		TDLS_OnAddBaRsp(Adapter, mmpdu);
	}
	
	return RT_STATUS_SUCCESS;

OnADDBARsp_Reject:
	RT_TRACE(COMP_QOS, DBG_LOUD, ("OnADDBARsp(): Rejected!!!!!!!!ReasonCode=%d\n", ReasonCode));
	{
		BA_RECORD	BA;
		PlatformMoveMemory(&(BA.BaParamSet), GET_BA_FRAME_PARAM_SET(mmpdu->Octet), 2);
		SendDELBA(Adapter, Addr, &BA, TX_DIR, ReasonCode);
	}
	return RT_STATUS_SUCCESS;
}


RT_STATUS
OnDELBA(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	mmpdu
    )
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	pu1Byte				Addr = Frame_Addr2(*mmpdu);
	BA_RECORD			BA;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>OnDELBA()\n"));
	if(pMgntInfo->pStaQos->CurrentQosMode == QOS_DISABLE ||
		pMgntInfo->pHTInfo->bCurrentHTSupport == FALSE )
		return RT_STATUS_SUCCESS;

	// CategoryCode(1) + ActionCode(1) + ParameterSet(2) + ReasonCode(2)
	if(mmpdu->Length < sMacHdrLng + 6)
	{
		RT_TRACE(COMP_QOS, DBG_SERIOUS, ("OnDELBA(): Invalid length(%d) of frame\n", mmpdu->Length));
		return RT_STATUS_MALFORMED_PKT;
	}

	if(GET_DELBA_FRAME_PARAM_SET_INITIATOR(mmpdu->Octet) == 1)
	{
		PRX_TS_RECORD 	pRxTs;

		if( !GetTs(
				Adapter, 
				(PTS_COMMON_INFO*)&pRxTs, 
				Addr, 
				GET_DELBA_FRAME_PARAM_SET_TID(mmpdu->Octet),
				RX_DIR,
				FALSE)	)
		{
			return RT_STATUS_FAILURE;
		}
		
		//BA Process Error when link with Netgear3500 v1
		//Fix do S3/S4 long run error 
		//by sherry 20101124
		if(pMgntInfo->IOTPeer == HT_IOT_PEER_MARVELL)
		{
			PlatformMoveMemory(&(BA.BaParamSet), GET_BA_FRAME_PARAM_SET(mmpdu->Octet), 2);
			SendDELBA(Adapter, Addr, &BA, RX_DIR, 1);
		}

		RxTsDeleteBA(Adapter, pRxTs);
	}
	else
	{
		PTX_TS_RECORD	pTxTs;

		if(!GetTs(
			Adapter, 
			(PTS_COMMON_INFO*)&pTxTs, 
			Addr, 
			GET_DELBA_FRAME_PARAM_SET_TID(mmpdu->Octet),
			TX_DIR,
			FALSE)	)
		{
			return RT_STATUS_FAILURE;
		}
		
		pTxTs->bUsingBa = FALSE;
		pTxTs->bAddBaReqInProgress = FALSE;
		pTxTs->bAddBaReqDelayed = FALSE;
		PlatformCancelTimer(Adapter, &pTxTs->TsAddBaTimer);
		TxTsDeleteBA(Adapter, pTxTs);
	}

	return RT_STATUS_SUCCESS;
}

//
// ADDBA initiate. This can only be called by TX side.
//
VOID
TsInitAddBA(
	PADAPTER		Adapter,
	PTX_TS_RECORD	pTS,
	u1Byte			Policy,
	BOOLEAN			bOverwritePending
	)
{
	PBA_RECORD			pBA = &pTS->TxPendingBARecord;
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>TsInitAddBA()\n"));
	
	if(pBA->bValid==TRUE && bOverwritePending==FALSE)
		return;

	// Set parameters to "Pending" variable set
	DeActivateBAEntry(Adapter, pBA);
	
	pBA->DialogToken++;						// DialogToken: Only keep the latest dialog token
	SET_BA_PARAM_SET_FIELD_AMSDU_SUPPORT(&(pBA->BaParamSet), 0);	// Do not support A-MSDU with A-MPDU now!!
	SET_BA_PARAM_SET_FIELD_BA_POLICY(&(pBA->BaParamSet), Policy);	// Policy: Delayed or Immediate
	SET_BA_PARAM_SET_FIELD_TID(&(pBA->BaParamSet), GET_TSPEC_BODY_TSINFO_TSID(pTS->TsCommonInfo.TSpec));	// TID
	// BufferSize: This need to be set according to A-MPDU vector

	{
		SET_BA_PARAM_SET_FIELD_BUF_SIZE(&(pBA->BaParamSet), 64);	// BufferSize: This need to be set according to A-MPDU vector
	}

	// TODO: Enable AMSDU may not be aware of AMPDU
	if(HT_AMSDU_WITHIN_AMPDU == pHTInfo->ForcedAMSDUMode)
	{
		SET_BA_PARAM_SET_FIELD_AMSDU_SUPPORT(&(pBA->BaParamSet), 1);
	}

	pBA->BaTimeoutValue = 0;					// Timeout value: Set 0 to disable Timer
	SET_BA_START_SQECTRL_FIELD_SEQ_NUM(&(pBA->BaStartSeqCtrl), ((pTS->TxCurSeq) % 4096)); 	

	ActivateBAEntry(Adapter, pBA, BA_SETUP_TIMEOUT);

	SendADDBAReq(Adapter, pTS->TsCommonInfo.Addr, pBA);

	if(!ACTING_AS_AP(Adapter))
	{
		// Negoitate with current SeqNum, but active this BA after 3 packets Txed
		SET_BA_START_SQECTRL_FIELD_SEQ_NUM(&(pBA->BaStartSeqCtrl), ((pTS->TxCurSeq + 3) % 4096)); 	
	}
}

VOID
TsInitDelBA(
	PADAPTER			Adapter,
	PTS_COMMON_INFO	pTsCommonInfo,
	TR_SELECT			TxRxSelect
	)
{
	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>TsInitDelBA()\n"));
	
	if(TxRxSelect == TX_DIR)
	{
		PTX_TS_RECORD	pTxTs = (PTX_TS_RECORD)pTsCommonInfo;

		if(TxTsDeleteBA(Adapter, pTxTs))
			SendDELBA(
				Adapter, 
				pTsCommonInfo->Addr, 
				(pTxTs->TxAdmittedBARecord.bValid)?(&pTxTs->TxAdmittedBARecord):(&pTxTs->TxPendingBARecord), 
				TxRxSelect, 
				DELBA_REASON_END_BA	);
	}
	else if(TxRxSelect == RX_DIR)
	{
		PRX_TS_RECORD	pRxTs = (PRX_TS_RECORD)pTsCommonInfo;
		if(RxTsDeleteBA(Adapter, pRxTs))
			SendDELBA(
				Adapter, 
				pTsCommonInfo->Addr, 
				&pRxTs->RxAdmittedBARecord, 
				TxRxSelect, 
				DELBA_REASON_END_BA	);
	}
}

VOID
BaSetupTimeOut(
	PRT_TIMER		pTimer
	)
{
	PTX_TS_RECORD	pTxTs = (PTX_TS_RECORD)pTimer->Context;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>BaSetupTimeOut()\n"));
	pTxTs->bAddBaReqInProgress = FALSE;
	pTxTs->bAddBaReqDelayed = TRUE;
	pTxTs->TxPendingBARecord.bValid = FALSE;
}

VOID
TxBaInactTimeout(
	PRT_TIMER		pTimer
	)
{
	PADAPTER	Adapter = (PADAPTER)pTimer->Adapter;
	PTX_TS_RECORD	pTxTs = (PTX_TS_RECORD)pTimer->Context;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>TxBaInactTimeout()\n"));
	TxTsDeleteBA(Adapter, pTxTs);
	SendDELBA(
		Adapter, 
		pTxTs->TsCommonInfo.Addr, 
		&pTxTs->TxAdmittedBARecord,
		TX_DIR, 
		DELBA_REASON_TIMEOUT	);
}

VOID
RxBaInactTimeout(
	PRT_TIMER		pTimer
	)
{
	PADAPTER	Adapter = (PADAPTER)pTimer->Adapter;
	PRX_TS_RECORD	pRxTs = (PRX_TS_RECORD)pTimer->Context;
	
	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====>RxBaInactTimeout()\n"));
	if(ACTING_AS_AP(Adapter))
		return;
		
	RxTsDeleteBA(Adapter, pRxTs);
	SendDELBA(
		Adapter, 
		pRxTs->TsCommonInfo.Addr, 
		&pRxTs->RxAdmittedBARecord,
		RX_DIR, 
		DELBA_REASON_TIMEOUT	);
}

VOID
RxBACheck(
	PADAPTER		Adapter,
	PRT_RFD			pRfd,
	POCTET_STRING	pFrame
	)
{
	
}

//
// Description: Parse BlockAckReq frame
//
// 2015.08.19. Added by tynli.
//
RT_STATUS
OnBAReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	OCTET_STRING	ospdu
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	pu1Byte				pBARFrame = ospdu.Octet;
	u1Byte				BARAckPolicy, MultiTID, CompressedBitmap, GCR, TIDInfo = 0;
	u2Byte				StartSeqNum, LatestSeqNum;
	PRX_TS_RECORD		pTS = NULL;
	pu1Byte 			Addr = Frame_Addr2(ospdu);
	BAR_TYPE			BARType = BAR_TYPE_BASIC_BAR;

	RT_TRACE(COMP_QOS, DBG_LOUD, ("=====> OnBAReq()\n"));

	// Header (16) + BAR control (2)
	if(ospdu.Length < 16 + 2)
	{
		RT_TRACE(COMP_QOS, DBG_SERIOUS, ("OnBAReq(): Invalid length(%d) of frame\n", ospdu.Length));
		return RT_STATUS_MALFORMED_PKT;
	}

	//
	// Check Block Ack Req support.
	// Reject if there is no support of Block Ack Req.
	//
	if((pMgntInfo->pStaQos->CurrentQosMode == QOS_DISABLE) ||
		(pMgntInfo->pHTInfo->bCurrentHTSupport == FALSE))
	{
		//StatusCode = ADDBA_STATUS_REFUSED;
		return RT_STATUS_FAILURE;
	}

	// Get the BAR control field offset
	pBARFrame += 16;
	BARAckPolicy = GET_BAR_PARAM_CTRL_FIELD_BAR_ACK_POLICY(pBARFrame);
	if(BARAckPolicy == BAR_NORMAL_ACK)
	{
		// Normal Ack
		MultiTID = GET_BAR_PARAM_CTRL_FIELD_MULTI_TID(pBARFrame);
		CompressedBitmap = GET_BAR_PARAM_CTRL_FIELD_COMPRESSED_BITMAP(pBARFrame);
		GCR = GET_BAR_PARAM_CTRL_FIELD_GCR(pBARFrame);
		TIDInfo = GET_BAR_PARAM_CTRL_FIELD_TID_INFO(pBARFrame);

		if(MultiTID == 0 && CompressedBitmap == 0 && GCR == 0)
		{
			// Basic BlockAckReq
			BARType = BAR_TYPE_BASIC_BAR;
		}		
		else if(MultiTID == 0 && CompressedBitmap == 1 && GCR == 0)
		{
			// Compressed BlockAckReq
			BARType = BAR_TYPE_COMPRESSED_BAR;
		}
		else if(MultiTID == 1 && CompressedBitmap == 0 && GCR == 0)
		{
			// Extended Compressed BlockAckReq
			BARType = BAR_TYPE_EXT_COMPRESSED_BAR;
		}
		else if(MultiTID == 1 && CompressedBitmap == 1 && GCR == 0)
		{
			// Multi-TID BlockAckReq
			BARType = BAR_TYPE_MULTI_TID_BAR;
		}
		else if(MultiTID == 0 && CompressedBitmap == 1 && GCR == 1)
		{
			// GCR BlockAckReq
			BARType = BAR_TYPE_GCR_BAR;
		}
		else
		{
			// Reserved
			RT_TRACE(COMP_QOS, DBG_LOUD, ("OnBAReq(): BAR type is reserved.\n"));
			
		}
	}
	else
	{
		// No Ack
		RT_TRACE(COMP_QOS, DBG_LOUD, ("OnBAReq(): BAR ACK policy is No ACK. Not handle now!!\n"));
	}


	//
	// Search for related traffic stream.
	//
	if(!GetTs(
			Adapter, 
			(PTS_COMMON_INFO*)(&pTS), 
			Addr, 
			TIDInfo,
			RX_DIR,
			TRUE))
	{
		return RT_STATUS_FAILURE;
	}

	if(BARType == BAR_TYPE_BASIC_BAR || BARType == BAR_TYPE_COMPRESSED_BAR ||
		BARType == BAR_TYPE_EXT_COMPRESSED_BAR || BARType == BAR_TYPE_GCR_BAR)
	{
		PRT_LIST_ENTRY	pList = &pTS->RxPendingPktList;

		// Get BAR Info
		pBARFrame += 2;		
		StartSeqNum = GET_BAR_PARAM_INFO_FIELD_STARTING_SEQ_NUM(pBARFrame);

		// Check the last received sequence number
		if(!RTIsListEmpty(pList))
		{
			LatestSeqNum = ((PRX_REORDER_ENTRY)pList->Blink)->SeqNum;
		}
		else
		{
			// Pending list is empty!
			LatestSeqNum = pTS->RxIndicateSeq;
		}

		// Update Rx indicate seq num for TS.
		if(!(SN_LESS(StartSeqNum, LatestSeqNum) || SN_EQUAL(StartSeqNum, LatestSeqNum)))
		{
			//
			// Indicate packets queued in driver for RxReorder and prepare to receive packet from new
			// new sequence start!!
			//			
			if(pTS->RxIndicateState==RXTS_INDICATE_BATCH)
				IndicateRxReorderList(Adapter, pTS, FALSE);
			else if(pTS->RxIndicateState==RXTS_INDICATE_REORDER)
				IndicateRxReorderList(Adapter, pTS, TRUE);

			if(!(SN_LESS(StartSeqNum, pTS->RxIndicateSeq) || SN_EQUAL(StartSeqNum, pTS->RxIndicateSeq)))
			{
				pTS->RxIndicateSeq = StartSeqNum;
				RT_TRACE(COMP_QOS, DBG_LOUD, ("OnBAReq(): Update Rx indcate starting sequence (%#X)!!\n", StartSeqNum));
			}
		}
		//RT_TRACE(COMP_QOS, DBG_LOUD, ("OnBAReq(): BARAckPolicy=%d, MultiTID=%d, CompressedBitmap=%#X, GCR=%d, TIDInfo=%d, StartSeqNum=%#X\n", 
		//			BARAckPolicy, MultiTID, CompressedBitmap, GCR, TIDInfo, StartSeqNum));
	}
	else if(BARType == BAR_TYPE_MULTI_TID_BAR)
	{
		RT_TRACE(COMP_QOS, DBG_LOUD, ("OnBAReq(): Multi-TID BAR is not supported now!!\n"));
	}

	//RT_PRINT_DATA(COMP_QOS, DBG_LOUD, "OnBAReq() Frame:\n", ospdu.Octet, ospdu.Length);

	return RT_STATUS_SUCCESS;
}
