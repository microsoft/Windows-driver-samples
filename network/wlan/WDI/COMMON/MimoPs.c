#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "MimoPs.tmh"
#endif

VOID
SetSelfMimoPsMode(
	IN	PADAPTER 	Adapter, 
	IN	u1Byte 		NewMimoPsMode
	)
{
	if((NewMimoPsMode == 2) || (NewMimoPsMode > 3))
		return;

	if(NewMimoPsMode == Adapter->MgntInfo.pHTInfo->SelfMimoPs)
		return;


	Adapter->MgntInfo.pHTInfo->SelfMimoPs = NewMimoPsMode;

	UpdateBeaconFrame(Adapter);
}

RT_STATUS
OnMimoPs(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	mmpdu
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	u1Byte			MimoPs = MIMO_PS_NOLIMIT;
	pu1Byte			pSrcAddr = Frame_Addr2(*mmpdu);
	PRT_WLAN_STA	pEntry;
	pu1Byte			pMcsFilter;
	BOOLEAN			bUseRAMask = FALSE;

	switch(PacketGetType(*mmpdu))
	{
	case Type_Action:
		// CategoryCode(1) + ActionCode(1) + SMPSControlField(1)
		if(mmpdu->Length < sMacHdrLng + 3)
		{
			RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] Invalid length(%d) of frame\n", mmpdu->Length));
			return RT_STATUS_MALFORMED_PKT;
		}
		MimoPs = (GET_MIMOPS_FRAME_PS_ENABLE(mmpdu->Octet)==0)? MIMO_PS_NOLIMIT:(GET_MIMOPS_FRAME_PS_MODE(mmpdu->Octet));
		break;

	case Type_Asoc_Req:
	case Type_Reasoc_Req:
		{
			OCTET_STRING	HTCapIe = {0, 0};

			HTCapIe = PacketGetElement(*mmpdu, EID_HTCapability, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
			if(HTCapIe.Length == 0)
			{
				RT_TRACE_F(COMP_HT, DBG_WARNING, ("[WARNING] Cannot get HT Capability IE from (Re-)Assoc Packet!\n"));
				return RT_STATUS_INVALID_DATA;
			}
			MimoPs = GET_HT_CAPABILITY_ELE_MIMO_PWRSAVE(HTCapIe.Octet);			
		}
		break;
	}

	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_USE_RA_MASK, &bUseRAMask);
	
	if(ACTING_AS_AP(Adapter) || pMgntInfo->mIbss)
	{
		pEntry = AsocEntry_GetEntry(pMgntInfo, pSrcAddr);
		if(pEntry != NULL)
		{
			RT_PRINT_ADDR(COMP_AP | COMP_HT, DBG_LOUD, "OnMimoPs(): Client = ", pEntry->MacAddr);
			RT_TRACE(COMP_HT, DBG_LOUD, ("Entry Mimo PS = %d\n", MimoPs));
			pEntry->HTInfo.MimoPs = MimoPs;

			if(pEntry->HTInfo.MimoPs <= MIMO_PS_DYNAMIC)
				pMcsFilter = MCS_FILTER_1SS;
			else
				pMcsFilter = MCS_FILTER_ALL;
			
			pEntry->HTInfo.HTHighestOperaRate = HTGetHighestMCSRate(
																Adapter,
																pEntry->HTInfo.McsRateSet,
																pMcsFilter	);

			if(bUseRAMask)
			{
				AP_InitRateAdaptiveState(Adapter, pEntry);
				pMgntInfo->Ratr_State = 0;
				Adapter->HalFunc.UpdateHalRAMaskHandler(Adapter, pEntry->AssociatedMacId, pEntry, pMgntInfo->Ratr_State);
			}
			else
			{
				Adapter->HalFunc.UpdateHalRATRTableHandler(
											Adapter, 
											&pMgntInfo->dot11OperationalRateSet,
											pMgntInfo->dot11HTOperationalRateSet,pEntry);
			}
		}
	}
	else
	{
		if(MgntLinkStatusQuery(Adapter) != RT_MEDIA_CONNECT)
		{
			RT_TRACE_F(COMP_HT, DBG_LOUD, ("Rx MimoPs when disconnected\n"));
			return RT_STATUS_FAILURE;
		}

		pMgntInfo->pHTInfo->PeerMimoPs = MimoPs;

		if(pMgntInfo->pHTInfo->PeerMimoPs <= MIMO_PS_STATIC)
			pMcsFilter = MCS_FILTER_1SS;
		else
			pMcsFilter = MCS_FILTER_ALL;
		
		pMgntInfo->HTHighestOperaRate = HTGetHighestMCSRate(
														Adapter,
														pMgntInfo->dot11HTOperationalRateSet,
														pMcsFilter	);
		if(bUseRAMask)
		{
			// we only need to set rate mask
			pMgntInfo->Ratr_State = 0;
			Adapter->HalFunc.UpdateHalRAMaskHandler(Adapter, pMgntInfo->mMacId, NULL, pMgntInfo->Ratr_State);
		}
		else
		{
			Adapter->HalFunc.UpdateHalRATRTableHandler(
										Adapter, 
										&pMgntInfo->dot11OperationalRateSet,
										pMgntInfo->dot11HTOperationalRateSet,
										NULL);
		}
	}

	return RT_STATUS_SUCCESS;
}

VOID
ConstructMimoPsFrame(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pAddr,
	IN	u1Byte			NewMimoPsMode,
	OUT	pu1Byte			pBuffer,
	OUT	pu4Byte			pLength
	)
{
	OCTET_STRING	osMimoPsFrame, osTemp;
	u1Byte			Content;

	FillOctetString(osMimoPsFrame, pBuffer, 0);
	*pLength = 0;

	if(NewMimoPsMode==3)
		Content = 0;
	else if(NewMimoPsMode==0)
		Content = 1;
	else if(NewMimoPsMode==1)
		Content = 3;
	else 
		return;

	ConstructMaFrameHdr(Adapter, pAddr, ACT_CAT_HT, ACT_MIMO_PWR_SAVE, &osMimoPsFrame);

	FillOctetString(osTemp, &Content, 1);
	PacketAppendData(&osMimoPsFrame, osTemp);
	*pLength = osMimoPsFrame.Length;
}

VOID
SendMimoPsFrame(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		pAddr,
	IN	u1Byte		NewMimoPsMode
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER		pBuf;
	u1Byte					DataRate = MgntQuery_MgntFrameTxRate(Adapter, pAddr);

	if((NewMimoPsMode!=0) && (NewMimoPsMode!=1) && (NewMimoPsMode!=3))
		return;

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructMimoPsFrame(Adapter, pAddr ,NewMimoPsMode, pBuf->Buffer.VirtualAddress, &pTcb->PacketLength);

		if(pTcb->PacketLength != 0)
			MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}
	
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);
}
