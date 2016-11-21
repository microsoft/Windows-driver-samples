#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "ChannelInfo.tmh"
#endif

u1Byte	
CHNL_GetCenterFrequency(
	IN	u1Byte				Channel,
	IN	CHANNEL_WIDTH		ChnlBW,
	IN	EXTCHNL_OFFSET 		ExtChnlOffset
)
{
	u1Byte 	CenterFrequency = Channel;

	if(ChnlBW == CHANNEL_WIDTH_80)
	{
		if((Channel == 36) || (Channel == 40) || (Channel == 44) || (Channel == 48) )
			CenterFrequency = 42;
		if((Channel == 52) || (Channel == 56) || (Channel == 60) || (Channel == 64) )
			CenterFrequency = 58;
		if((Channel == 100) || (Channel == 104) || (Channel == 108) || (Channel == 112) )
			CenterFrequency = 106;
		if((Channel == 116) || (Channel == 120) || (Channel == 124) || (Channel == 128) )
			CenterFrequency = 122;
		if((Channel == 132) || (Channel == 136) || (Channel == 140) || (Channel == 144) )
			CenterFrequency = 138;
		if((Channel == 149) || (Channel == 153) || (Channel == 157) || (Channel == 161) )
			CenterFrequency = 155;
		else if(Channel <= 14)
			CenterFrequency = 7;
	}
	else if(ChnlBW == CHANNEL_WIDTH_40)
	{
		if(ExtChnlOffset == EXTCHNL_OFFSET_UPPER)
			CenterFrequency = Channel + 2;
		else
			CenterFrequency = Channel - 2;
	}
	return CenterFrequency;
}



EXTCHNL_OFFSET
CHNL_GetExt20OffsetOf5G(
	IN u1Byte		channel
	)
{
	u1Byte			i;
	EXTCHNL_OFFSET	ExtChnlOffset = EXTCHNL_OFFSET_LOWER;
	u1Byte			UpperChannelList_5G[11] = {36,44,52,60,100,108,116,124,132,149,157};

	for(i =0; i < 11;i++)
	{
		if(channel == UpperChannelList_5G[i])
			break;
	}
	if(i < 11)
			ExtChnlOffset = EXTCHNL_OFFSET_UPPER;

	RT_DISP(FCHNL, FCHNL_INFO,  ("%s channel=%d, ExtChnlOffset=%d\n", __FUNCTION__, channel, ExtChnlOffset));
	
	return ExtChnlOffset;
}


u1Byte
CHNL_IsLegalChannel(
	PADAPTER		Adapter,
	u8Byte			freq_channel
)
{
	PRT_CHANNEL_LIST	pChanneList = NULL;
	u2Byte				i;

	RtActChannelList(Adapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST, NULL, &pChanneList);

	//Skip check legal channel when doing fast USB switch, since we only have one channel in channel list, 20150408, Sean. 

		if (pChanneList->ChannelLen == 1)
		{
			return (u1Byte)freq_channel;
		}
	
	for(i = 0; i < pChanneList->ChannelLen; i ++)
	{
		if(freq_channel == pChanneList->ChnlListEntry[i].ChannelNum)
		{
			return pChanneList->ChnlListEntry[i].ChannelNum;
		}// TODO:  ending check 5G first
		else if(freq_channel == pChanneList->ChnlListEntry[i].ChannelNum + 2)
		{
			return (u1Byte)freq_channel;
		}
		if( (pChanneList->ChnlListEntry[i].ChannelNum <= 14) && (freq_channel == DSSS_Freq_Channel[pChanneList->ChnlListEntry[i].ChannelNum]*1000) )
		{
			return pChanneList->ChnlListEntry[i].ChannelNum;
		}
	}
	return 0;
}


BOOLEAN 
CHNL_IsLegal5GChannel(
	IN PADAPTER			Adapter,
	IN u1Byte			channel
)
{
	
	u1Byte i=0;
	u1Byte Channel_5G[45] = {36,38,40,42,44,46,48,50,52,54,56,58,
		60,62,64,100,102,104,106,108,110,112,114,116,118,120,122,
		124,126,128,130,132,134,136,138,140,149,151,153,155,157,159,
		161,163,165};
	for(i=0;i<sizeof(Channel_5G);i++)
		if(channel == Channel_5G[i])
			return TRUE;
	return FALSE;
}



BOOLEAN
CHNL_AcquireOpLock(
	PADAPTER	Adapter,
	CHNLOP		ChnlOp
	)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_CHANNEL_INFO		pChnlInfo = pMgntInfo->pChannelInfo;
	BOOLEAN					bGetLock = FALSE;

	PlatformAcquireSpinLock(Adapter, RT_CHNLOP_SPINLOCK);
	if(CHNL_OP_IN_PROGRESS(pChnlInfo))
	{
		bGetLock = FALSE;
	}
	else
	{
		pChnlInfo->ChnlOp = ChnlOp;
		bGetLock = TRUE;
	}
	PlatformReleaseSpinLock(Adapter, RT_CHNLOP_SPINLOCK);
	return bGetLock;
}

VOID
CHNL_ReleaseOpLock(
	PADAPTER	Adapter
	)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_CHANNEL_INFO		pChnlInfo = pMgntInfo->pChannelInfo;
	
	PlatformAcquireSpinLock(Adapter, RT_CHNLOP_SPINLOCK);
	pChnlInfo->ChnlOp = CHNLOP_NONE;
	PlatformReleaseSpinLock(Adapter, RT_CHNLOP_SPINLOCK);
}


BOOLEAN
CHNL_ValidForWirelessMode(
	u1Byte	channel,		
	u2Byte	wirelessmode
	)
{	

	BOOLEAN ret = FALSE;
	
	do
	{
		if(IS_5G_WIRELESS_MODE(wirelessmode) && channel > 14)
		{
			ret = TRUE;
			break;
		}
		
		if(IS_24G_WIRELESS_MODE(wirelessmode) && channel <= 14)
		{
			ret = TRUE;
			break;
		}
	}while(FALSE);

	return ret;
}


CHANNEL_WIDTH
CHNL_GetRegBWSupport(
	IN	PADAPTER	Adapter
)
{
	PMGNT_INFO				pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_CHANNEL_INFO		pChnlInfo = pMgntInfo->pChannelInfo;
	CHANNEL_WIDTH			retBW = 0;


	if(HAL_Support_BW_Advancesetting(Adapter))
	{
		if(IS_WIRELESS_MODE_24G(Adapter))
			retBW = (pHTInfo->bRegBW40MHzFor2G ? CHANNEL_WIDTH_40:CHANNEL_WIDTH_20) ;
		else if(IS_WIRELESS_MODE_5G(Adapter))
			retBW = (pHTInfo->bRegBW40MHzFor5G ? CHANNEL_WIDTH_40:CHANNEL_WIDTH_20) ;
	}
	else	
		retBW = pChnlInfo->RegBWSetting;
			
	
	return retBW;
}




VOID
chnl_GetBWFrom_IE(
	IN	PADAPTER		Adapter,
	OUT PCHANNEL_WIDTH	pChnlBW,
	OUT PEXTCHNL_OFFSET	pExtChnlOffset
)
{
	PMGNT_INFO					pMgntInfo=&Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo = GET_VHT_INFO(pMgntInfo);

	BOOLEAN 		bBW40MHz = FALSE, bBW80MHz = FALSE;
	CHANNEL_WIDTH	ChnlBW = CHANNEL_WIDTH_20;
	EXTCHNL_OFFSET	ExtChnlOffset = EXTCHNL_OFFSET_NO_EXT;

	if(pHTInfo->bCurrentHTSupport)
	{
		CHANNEL_WIDTH	PeerChnlBW = (CHANNEL_WIDTH)pHTInfo->bPeer40MHzCap;
		EXTCHNL_OFFSET	PeerExtChnlOffset = pHTInfo->PeerExtChnlOffset;

		ChnlBW = CHNL_GetRegBWSupport(Adapter);
		if(ChnlBW == CHANNEL_WIDTH_20)						// 20 STA -> 40 AP or 20 STA -> 20 AP
			ExtChnlOffset = EXTCHNL_OFFSET_NO_EXT;
		else if(PeerChnlBW == CHANNEL_WIDTH_20)				// 40 STA -> 20 AP
			ChnlBW = CHANNEL_WIDTH_20;
		else if(PeerExtChnlOffset == EXTCHNL_OFFSET_NO_EXT)
			ChnlBW = CHANNEL_WIDTH_20;
		else													// 40 STA -> 40 AP
		{
			if(	PeerExtChnlOffset == EXTCHNL_OFFSET_UPPER && 
				CHNL_IsLegalChannel(Adapter, pMgntInfo->dot11CurrentChannelNumber + 2))
				bBW40MHz = TRUE;
			else if(PeerExtChnlOffset== EXTCHNL_OFFSET_LOWER &&
				 CHNL_IsLegalChannel(Adapter, pMgntInfo->dot11CurrentChannelNumber - 2))
				bBW40MHz = TRUE;
			else
				bBW40MHz = FALSE;

			if(bBW40MHz == TRUE)
			{
				ChnlBW = PeerChnlBW;
				ExtChnlOffset = PeerExtChnlOffset;
			}	
			else
			{
				ChnlBW = CHANNEL_WIDTH_20;
				ExtChnlOffset = EXTCHNL_OFFSET_NO_EXT;
			}
		}
		RT_DISP(FCHNL, FCHNL_INFO, ("%s PeerChnlBW=%d, PeerExtChnlOffset=%d\n", __FUNCTION__, PeerChnlBW, PeerExtChnlOffset));
	}
	else
		ChnlBW = CHANNEL_WIDTH_20;

	
	if(bBW40MHz && pVHTInfo->bCurrentVHTSupport)
	{
		bBW80MHz = (CHNL_GetRegBWSupport(Adapter) >= CHANNEL_WIDTH_80)?(pVHTInfo->PeerChnlBW?1:0):0;

		if(bBW80MHz == 0)
			ChnlBW = CHANNEL_WIDTH_40;
		else
			ChnlBW = CHANNEL_WIDTH_80;
	}

	*pChnlBW = ChnlBW;
	*pExtChnlOffset = ExtChnlOffset;

	RT_DISP(FCHNL, FCHNL_INFO, ("%s ChnlBW=%d, ExtChnlOffset=%d, bEnableVHT = %d, bCurrentHTSupport = %d, bCurrentVHTSupport = %d,\n PeerChnlBW = %d, CHNL_GetRegBWSupport() = %d\n", __FUNCTION__, ChnlBW, ExtChnlOffset, pVHTInfo->bEnableVHT, pHTInfo->bCurrentHTSupport, pVHTInfo->bCurrentVHTSupport, pVHTInfo->PeerChnlBW, CHNL_GetRegBWSupport(Adapter)));
}


VOID
CHNL_ChangeBwChnlFromPeerWorkitemCallBack(
	IN	PVOID			Context
)
{
	PADAPTER			Adapter = (PADAPTER)Context;	
	CHANNEL_WIDTH		ChnlBW;
	EXTCHNL_OFFSET		ExtChnlOffset;
	PMGNT_INFO			pMgntInfo = &Adapter->MgntInfo;
	PRT_CHANNEL_INFO	pChnlInfo = GET_CHNL_INFO(pMgntInfo);
	PADAPTER			pExtAdapter = GetFirstAPAdapter(Adapter);
	PADAPTER			pGoAdapter = GetFirstGOPort(Adapter);

	if(pExtAdapter != NULL)
	{
		if(pExtAdapter->bAPChannelInprogressForNewconnected)
		{
			RT_DISP(FCHNL, FCHNL_ERROR, ("%s: AP change channel in progress , return \n", __FUNCTION__));
			return;

		}
	}

	if(pGoAdapter != NULL)
	{
		if(pGoAdapter->bAPChannelInprogressForNewconnected)
		{
			RT_DISP(FCHNL, FCHNL_ERROR, ("%s: AP change channel in progress , return \n", __FUNCTION__));
			return;
		}
	}

	if(GetDefaultMgntInfo(Adapter)->RegMultiChannelFcsMode != 0)
	{
		if(TRUE == MultiChannel_IsFCSInProgress(Adapter))
		{
			RT_TRACE(COMP_MULTICHANNEL, DBG_TRACE, ("CHNL_ChangeBwChnlFromPeerWorkitemCallBack(): skip change ch & bw due to MCC\n"));
			return;
		}
	}
	
#if 0
	if(GetDefaultAdapter(Adapter)->bInHctTest)
	{
		RT_DISP(FCHNL, FCHNL_ERROR, ("%s: hct test , return \n", __FUNCTION__));
		return;
	
	}
#endif	

	
	chnl_GetBWFrom_IE(Adapter, &ChnlBW, &ExtChnlOffset);

	if(	pChnlInfo->CurrentChannelBandWidth != ChnlBW ||
		pChnlInfo->Ext20MHzChnlOffsetOf40MHz != ExtChnlOffset)
	{
		//
		// Leave LPS and wait until RF is already on before switching channel. 2014.09.25, by tynli.
		//
		LPSLeaveAndCheckReady(Adapter);

		RT_TRACE(COMP_MLME, DBG_LOUD, ("CHNL_ChangeBwChnlFromPeerWorkitemCallBack(): CurrentChannel=%d, ChnlBW=%d, ExtChnlOffset=%d\n",
			pMgntInfo->dot11CurrentChannelNumber, ChnlBW, ExtChnlOffset));

		CHNL_SetBwChnl(Adapter, pMgntInfo->dot11CurrentChannelNumber, ChnlBW, ExtChnlOffset);
		// We should set RA H2C cmd because the BW in TxDesc will be filled again by Fw, we 
		// need to inform Fw the BW changed information. 2012.11.26. by tynli. 
		// Suggested by SD1 Alex Chou.
		if(!ACTING_AS_AP(Adapter) && !ACTING_AS_IBSS(Adapter))
		{
			// we only need to set rate mask
			Adapter->HalFunc.UpdateHalRAMaskHandler(Adapter, pMgntInfo->mMacId, NULL, 0);

		}
	}

}


VOID
CHNL_SetBwChnlFromPeer(
	IN	PADAPTER			Adapter
)
{
	PMGNT_INFO		pMgntInfo=&Adapter->MgntInfo;
	CHANNEL_WIDTH	ChnlBW;
	EXTCHNL_OFFSET	ExtChnlOffset;

	chnl_GetBWFrom_IE(Adapter, &ChnlBW, &ExtChnlOffset);

	CHNL_SetBwChnl(Adapter, pMgntInfo->dot11CurrentChannelNumber, ChnlBW, ExtChnlOffset);
}


BOOLEAN
CHNL_GetCurrnetChnlInfo(
	IN		PADAPTER	Adapter,
	OUT 	pu1Byte 		pBuf,
	IN OUT 	pu4Byte		pBufLen
)
{
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	PRT_CHANNEL_INFO	pChnlInfo = pMgntInfo->pChannelInfo;

	if(*pBufLen < 4)
	{
		*pBufLen = 4;
		return FALSE;
	}		
	
	*pBufLen = 4;

	*pBuf = pChnlInfo->CurrentChannelBandWidth;
	*(pBuf+1) = pChnlInfo->PrimaryChannelNumber;
	*(pBuf+2) = pChnlInfo->Ext20MHzChnlOffsetOf40MHz;
	*(pBuf+3) = pChnlInfo->CurrentChannelCenterFrequency;

	return TRUE;
}
	


VOID
chnl_HalSwBwChnl(
	PADAPTER	Adapter
	)
{
    PCHANNEL_COMMON_CONTEXT		pChnlCommInfo = Adapter->pPortCommonInfo->pChnlCommInfo;
	PHAL_DATA_TYPE				pHalData = GET_HAL_DATA(Adapter);

	FunctionIn(COMP_SCAN);

	if(HAL_ChipSupport80MHz(Adapter))
	{
		Adapter->HalFunc.SwChnlAndSetBWHandler(
			Adapter,
			TRUE,
			TRUE,
			pChnlCommInfo->CurrentChannelCenterFrequency,
			pChnlCommInfo->CurrentChannelBandWidth,
			pChnlCommInfo->Ext20MHzChnlOffsetOf40MHz,
			pChnlCommInfo->Ext40MHzChnlOffsetOf80MHz,
			pChnlCommInfo->CurrentChannelCenterFrequency
		);
	}
	else
	{
		u1Byte	offset;

		Mgnt_SwChnl(Adapter, pChnlCommInfo->CurrentChannelCenterFrequency, 1);		

		// last bandwidth offset 
		if(pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_UPPER)
			offset = EXTCHNL_OFFSET_LOWER;
		else if (pHalData->nCur40MhzPrimeSC == HAL_PRIME_CHNL_OFFSET_LOWER)
			offset = EXTCHNL_OFFSET_UPPER;
		else
			offset = EXTCHNL_OFFSET_NO_EXT;
		// 1.20M to 40M or 40M to 20M
		// 2.40M to 40M as offset different or bandtype different	
		if(	(pHalData->CurrentChannelBW != pChnlCommInfo->CurrentChannelBandWidth )||
			(pHalData->CurrentChannelBW == CHANNEL_WIDTH_40 && pChnlCommInfo->Ext20MHzChnlOffsetOf40MHz !=offset) ||
			(pHalData->LastBandType !=pHalData->CurrentBandType))
		{
			Adapter->HalFunc.SetBWModeHandler(
				Adapter,
				pChnlCommInfo->CurrentChannelBandWidth,
				pChnlCommInfo->Ext20MHzChnlOffsetOf40MHz
				);
		}	
	}

	
	FunctionOut(COMP_SCAN);
}	


VOID
CHNL_SetBwChnlCallback(
	IN	PRT_TIMER		pTimer
	)
{
	PADAPTER			Adapter=(PADAPTER)pTimer->Adapter;
	PMGNT_INFO			pMgntInfo=&Adapter->MgntInfo;
	PRT_CHANNEL_INFO	pChnlInfo = pMgntInfo->pChannelInfo;
	PHAL_DATA_TYPE	   	pHalData = GET_HAL_DATA(Adapter);
	BOOLEAN				bSwBW = TRUE;

	RT_DISP(FCHNL, FCHNL_FUN, ("===>%s \n", __FUNCTION__));

	if(RT_DRIVER_HALT(Adapter))
	{
		RT_DISP(FCHNL, FCHNL_ERROR, ("<===%s bDriverStopped %d bSurpriseRemoved %d bDriverIsGoingToUnload %d\n", __FUNCTION__, Adapter->bDriverStopped, Adapter->bSurpriseRemoved, Adapter->bDriverIsGoingToUnload));	
		return;
	}

	if(Adapter->bInSetPower && RT_CANNOT_IO(Adapter))
	
	{
		RT_DISP(FCHNL, FCHNL_ERROR, ("<===%s can NOT IO\n", __FUNCTION__));
		bSwBW = FALSE;
	}

	if(MgntResetOrPnPInProgress(Adapter))
	{
		RT_DISP(FCHNL, FCHNL_ERROR, ("<===%s MgntResetOrPnPInProgress\n", __FUNCTION__));			
		bSwBW = FALSE;
	}
	
	if(bSwBW == FALSE)
	{
		PlatformAcquireSpinLock(Adapter, RT_BW_SPINLOCK);
		pChnlInfo->bSwBwInProgress = FALSE;
		PlatformReleaseSpinLock(Adapter, RT_BW_SPINLOCK);
		return;
	}
	
	if(MgntInitAdapterInProgress(pMgntInfo))
	{
		PlatformSetTimer(Adapter, &pChnlInfo->SwBwTimer, 100);
		return;
	}
	
   if(RT_IsSwChnlAndBwInProgress(Adapter))
   { 
  		RT_DISP(FCHNL, FCHNL_FUN, ("<===%s pHalData->SwChnlInProgress: %d, pHalData->SetBWModeInProgress: %d, pHalData->bSwChnlAndSetBWInProgress: %d\n", 
				__FUNCTION__, pHalData->SwChnlInProgress, pHalData->SetBWModeInProgress, pHalData->bSwChnlAndSetBWInProgress));
	
       	PlatformSetTimer(Adapter, &pChnlInfo->SwBwTimer, 10);
		return;
   }

	if(pMgntInfo->bScanInProgress)
	{
		RT_DISP(FCHNL, FCHNL_FUN, ("<===%s bScanInProgress\n", __FUNCTION__));
		PlatformSetTimer(Adapter, &pChnlInfo->SwBwTimer, 10);
		return;
	}
	   
	if(!CHNL_AcquireOpLock(Adapter, CHNLOP_SWBW))
	{
		PlatformSetTimer(Adapter, &pChnlInfo->SwBwTimer, 10);
		return;
	}

	if(IsDefaultAdapter(Adapter))
	{
		RT_DISP(FCHNL, FCHNL_FUN, ("%s  Def Adapter \n", __FUNCTION__));
	}
	else
	{
		RT_DISP(FCHNL, FCHNL_FUN, ("%s  Ext Adapter \n", __FUNCTION__));
	}

	PlatformAcquireSpinLock(Adapter, RT_BW_SPINLOCK);
	pChnlInfo->bSwBwInProgress = TRUE;	
	PlatformReleaseSpinLock(Adapter, RT_BW_SPINLOCK);

	chnl_HalSwBwChnl(Adapter);
		
	// bandtype different only occur in 92d ; for 92c/92s  last band type is the same as current band type		
	pHalData->LastBandType = pHalData->CurrentBandType;  

	PlatformAcquireSpinLock(Adapter, RT_BW_SPINLOCK);
	pChnlInfo->bSwBwInProgress = FALSE;
	PlatformReleaseSpinLock(Adapter, RT_BW_SPINLOCK);

	CHNL_ReleaseOpLock(Adapter);	
	
	RT_DISP(FCHNL, FCHNL_FUN, ("<===%s\n", __FUNCTION__));
}

CHANNEL_WIDTH
CHNL_CheckChnlPlanWithBW(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				PrimaryChnl,	
	IN	CHANNEL_WIDTH		Bandwidth,	
	IN	EXTCHNL_OFFSET		BwOffset	
)
{
	CHANNEL_WIDTH finalBw = CHANNEL_WIDTH_20;
	PRT_CHANNEL_LIST	pChanneList = NULL;
	u2Byte				i;
	u1Byte				CentralChnl=PrimaryChnl;
	u1Byte				anotherPrimaryChnl=PrimaryChnl;
	u1Byte				findcnt=0;
	BOOLEAN				bFind=FALSE;



	if(Bandwidth == CHANNEL_WIDTH_40)
	{
		if(BwOffset == EXTCHNL_OFFSET_UPPER)
		{
			CentralChnl = PrimaryChnl + 2;
			anotherPrimaryChnl = CentralChnl+2;
		}
		else
		{
			CentralChnl = PrimaryChnl - 2;
			anotherPrimaryChnl = CentralChnl-2;
		}
	}


	RtActChannelList(pAdapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST, NULL, &pChanneList);

	if(PrimaryChnl>13)
	{
		if(!(pChanneList->WirelessMode & (WIRELESS_MODE_A|WIRELESS_MODE_AC_5G|WIRELESS_MODE_N_5G)))
		{
			finalBw =  Bandwidth;
			return finalBw;
		}
	}
	else
	{
		if(!(pChanneList->WirelessMode & (WIRELESS_MODE_B|WIRELESS_MODE_N_24G|WIRELESS_MODE_G|WIRELESS_MODE_AC_24G)))
		{
			finalBw =  Bandwidth;
			return finalBw;
		}
	}

	for(i=0;i<pChanneList->ChannelLen;i++)
	{
		if((anotherPrimaryChnl == pChanneList->ChnlListEntry[i].ChannelNum) ||
			(PrimaryChnl == pChanneList->ChnlListEntry[i].ChannelNum))
		{
			findcnt++;
		}

		if(findcnt==2)
		{
			bFind =TRUE;
			findcnt =0;
			break;			
		}
	}

	if(Bandwidth == CHANNEL_WIDTH_80)
		finalBw = CHANNEL_WIDTH_80;
	else
	{
		// remove, advised by Bryant, this will cause IOT issue with TP link Ap
		// TP link will be 40MHz, but NIC is 20MHz => problem.
#if 0	
		if( BT_1Ant(pAdapter) && BT_IsBtLinkExist(pAdapter) &&
			IS_WIRELESS_MODE_24G(pAdapter) )
		{
			finalBw = CHANNEL_WIDTH_20;
		}
		else
#endif
		{
			if(bFind)
				finalBw = CHANNEL_WIDTH_40;
			else 
				finalBw = CHANNEL_WIDTH_20;
		}
	}			

	return finalBw;
}


//
// This function set bandwidth mode in protocol layer.
//
VOID
CHNL_SetBwChnl(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				PrimaryChnl,
	IN	CHANNEL_WIDTH		Bandwidth,
	IN	EXTCHNL_OFFSET		BwOffset
	)
{
	PMGNT_INFO					pMgntInfo = &pAdapter->MgntInfo;
	PRT_CHANNEL_INFO			pChnlInfo = GET_CHNL_INFO(pMgntInfo);
	
	PADAPTER					DefAdapter = GetDefaultAdapter(pAdapter);
	PMGNT_INFO					pDefMgntInfo = &DefAdapter->MgntInfo;
	PRT_CHANNEL_INFO			pDefChnlInfo = GET_CHNL_INFO(pDefMgntInfo);
	PCHANNEL_COMMON_CONTEXT		pDefChnlCommInfo = DefAdapter->pPortCommonInfo->pChnlCommInfo;

	CHANNEL_WIDTH				ToSetBandWidth = CHANNEL_WIDTH_20;
	EXTCHNL_OFFSET				ToSTAExtChnlOffsetof40MHz= EXTCHNL_OFFSET_NO_EXT;
	EXTCHNL_OFFSET				ToSTAExtChnlOffsetof80MHz= EXTCHNL_OFFSET_NO_EXT;
	u1Byte						ToSTACenterFrequency;

	RT_TRACE_F(COMP_SCAN, DBG_LOUD, ("PrimaryChnl %d\n", PrimaryChnl));

	if(ACTING_AS_IBSS(DefAdapter))
	{
		{
			// 2013/10/15 MH Add for higher adhoc and VHT 2.4G mode.
			if (!pMgntInfo->bReg11nAdhoc && !pMgntInfo->bRegVht24g)
			{
				Bandwidth= CHANNEL_WIDTH_20;
				BwOffset = EXTCHNL_OFFSET_NO_EXT;
			}
		}
	}

	ToSetBandWidth = Bandwidth;
	ToSTAExtChnlOffsetof40MHz = BwOffset;
	ToSTACenterFrequency = CHNL_GetCenterFrequency(PrimaryChnl, Bandwidth, BwOffset);
	
	ToSetBandWidth = CHNL_CheckChnlPlanWithBW(pAdapter,PrimaryChnl,Bandwidth,BwOffset);

	if((ToSetBandWidth != Bandwidth) &&  ToSetBandWidth==CHANNEL_WIDTH_20)
		ToSTACenterFrequency =PrimaryChnl;
	
	if(Bandwidth == CHANNEL_WIDTH_80)
	{
		if(ToSTACenterFrequency > pMgntInfo->dot11CurrentChannelNumber)
			ToSTAExtChnlOffsetof80MHz = EXTCHNL_OFFSET_UPPER;
		else if(ToSTACenterFrequency < pMgntInfo->dot11CurrentChannelNumber)
			ToSTAExtChnlOffsetof80MHz = EXTCHNL_OFFSET_LOWER;
		else
			ToSTAExtChnlOffsetof80MHz = EXTCHNL_OFFSET_NO_EXT;		
	}

	RT_DISP(FCHNL, FCHNL_FUN,  ("Set ToSetBandWidth = %d, ToSTAExtChnlOffset = %d,ToSTAExtChnlOffsetof80MHz %d, ToSTACenterFrequency= %d \n", ToSetBandWidth, ToSTAExtChnlOffsetof40MHz,ToSTAExtChnlOffsetof80MHz,ToSTACenterFrequency));

	pMgntInfo->dot11CurrentChannelBandWidth = ToSetBandWidth;
	pChnlInfo->PrimaryChannelNumber = pDefChnlCommInfo->PrimaryChannelNumber = PrimaryChnl;
	pChnlInfo->CurrentChannelBandWidth = pDefChnlCommInfo->CurrentChannelBandWidth = ToSetBandWidth;
	pChnlInfo->CurrentChannelCenterFrequency = pDefChnlCommInfo->CurrentChannelCenterFrequency = ToSTACenterFrequency;
	pChnlInfo->Ext20MHzChnlOffsetOf40MHz = pDefChnlCommInfo->Ext20MHzChnlOffsetOf40MHz = ToSTAExtChnlOffsetof40MHz;
	pChnlInfo->Ext40MHzChnlOffsetOf80MHz = pDefChnlCommInfo->Ext40MHzChnlOffsetOf80MHz = ToSTAExtChnlOffsetof80MHz;

	// TODO: 2007.7.13 by Emily Wait 2000ms  in order to garantee that switching
	//   bandwidth is executed after scan is finished. It is a temporal solution
	//   because software should ganrantee the last operation of switching bandwidth
	//   is executed properlly. 
	if(!(RT_DRIVER_HALT(DefAdapter)))
		PlatformSetTimer(DefAdapter, &pDefChnlInfo->SwBwTimer, 0);

	FunctionOut(COMP_SCAN);
}



VOID
CHNL_SetChnlInfoFromDestPort(
	IN	PADAPTER			pDestAdapter,
	IN	PADAPTER			pSrcAdapter
)
{
	PMGNT_INFO			pDestMgntInfo = &pDestAdapter->MgntInfo;
	PRT_CHANNEL_INFO	pDestChnlInfo = GET_CHNL_INFO(pDestMgntInfo);
	PMGNT_INFO			pSrcMgntInfo = &pSrcAdapter->MgntInfo;
	PRT_CHANNEL_INFO 	pSrcChnlInfo = GET_CHNL_INFO(pSrcMgntInfo);

	pDestMgntInfo->dot11CurrentChannelNumber = pSrcMgntInfo->dot11CurrentChannelNumber;
	pDestMgntInfo->dot11CurrentChannelBandWidth = pSrcMgntInfo->dot11CurrentChannelBandWidth;

	// Best way is restart AP mode. 2012.01.08 lanhsin. 
	//pDestMgntInfo->CurrentBssWirelessMode = pDestMgntInfo->dot11CurrentWirelessMode = pSrcMgntInfo->dot11CurrentWirelessMode;
	
	pDestChnlInfo->PrimaryChannelNumber = pSrcChnlInfo->PrimaryChannelNumber;
	pDestChnlInfo->CurrentChannelBandWidth =  pSrcChnlInfo->CurrentChannelBandWidth;
	pDestChnlInfo->CurrentChannelCenterFrequency = pSrcChnlInfo->CurrentChannelCenterFrequency;

	pDestChnlInfo->Ext20MHzChnlOffsetOf40MHz = pSrcChnlInfo->Ext20MHzChnlOffsetOf40MHz;
	pDestChnlInfo->Ext40MHzChnlOffsetOf80MHz = pSrcChnlInfo->Ext40MHzChnlOffsetOf80MHz;

	RT_DISP(FCHNL, FCHNL_FUN, ("%s: pDestChnlInfo->CurrentChannelCenterFrequency=%d, pSrcChnlInfo->CurrentChannelCenterFrequency=%d\n", 
		__FUNCTION__, pDestChnlInfo->CurrentChannelCenterFrequency, pSrcChnlInfo->CurrentChannelCenterFrequency));
}


//sherry added 20130207
//to check switch channel and bandwidth in progress
BOOLEAN
CHNL_SwChnlAndSetBwInProgress(
	PADAPTER	Adapter
)
{
	PADAPTER			pDefAdapter = GetDefaultAdapter(Adapter);
	PMGNT_INFO			pDefMgntInfo = &pDefAdapter->MgntInfo;
	PRT_CHANNEL_INFO	pDefChnlInfo = GET_CHNL_INFO(pDefMgntInfo);

	if(pDefChnlInfo->bSwBwInProgress)
	{
		RT_DISP(FCHNL, FCHNL_INFO, ("%s: return TRUE \n", __FUNCTION__));
		return TRUE;
	}
	else
	{
		RT_DISP(FCHNL, FCHNL_INFO, ("%s: return FALSE \n", __FUNCTION__));
		return FALSE;
	}
}

//
// Description:
//	Get the primary channel from the channel information and hw configurations.
// Note:
//	There are two kinds of channel, one is kept in mgnt/channel info and another one is kept in hal.
//	They may be mismatched when the channel switching is in progress or scan in progress.
//	When the scan is performing, the channel in mgnt/channel is not updated.
//
//	If channels are different between HW and ChannelInfo, this function returns HW channel.
//
u1Byte
RT_GetChannelNumber(
	IN	PADAPTER	pAdapter
	)
{
	u1Byte			hwCenterChannel = 0;
	EXTCHNL_OFFSET	hwBW40MOffset = EXTCHNL_OFFSET_NO_EXT;
	EXTCHNL_OFFSET	hwBW80MOffset = EXTCHNL_OFFSET_NO_EXT;

	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_CUR_CENTER_CHNL, (pu1Byte)(&hwCenterChannel));
	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_BW40MHZ_EXTCHNL, (pu1Byte)(&hwBW40MOffset));
	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_BW80MHZ_EXTCHNL, (pu1Byte)(&hwBW80MOffset));

	if(0 == hwCenterChannel)
	{ // Please fix this case
		hwCenterChannel = GET_HAL_DATA(pAdapter)->CurrentChannel;
		RT_TRACE_F(COMP_SCAN, DBG_WARNING, ("[WARNING] Get hw channel = 0 by HW_VAR_CUR_CENTER_CHNL, access hal directly = %d\n",
			hwCenterChannel));
	}
	
	if(((GetDefaultAdapter(pAdapter)->pPortCommonInfo->pChnlCommInfo->CurrentChannelCenterFrequency) == hwCenterChannel))
	{
		return (GetDefaultAdapter(pAdapter)->pPortCommonInfo->pChnlCommInfo->PrimaryChannelNumber);
	}
	else
	{ // In this case, the returned channel may not be the right one because channel switching is in progress.
		if((GetDefaultAdapter(pAdapter)->pPortCommonInfo->pChnlCommInfo->CurrentChannelCenterFrequency) != 0)
		{
			RT_TRACE_F(COMP_MLME, DBG_WARNING, ("[WARNING] Mismatched channel info ceter = %d, primary = %d, hwCenterChannel = %d (40M = %d, 80M = %d), SwChnlInProgress = %d, ScanInProgress = %d\n",
				(GetDefaultAdapter(pAdapter)->pPortCommonInfo->pChnlCommInfo->CurrentChannelCenterFrequency),
				(GetDefaultAdapter(pAdapter)->pPortCommonInfo->pChnlCommInfo->PrimaryChannelNumber),
				hwCenterChannel, hwBW40MOffset, hwBW80MOffset, (RT_IsSwChnlAndBwInProgress(pAdapter) | CHNL_SwChnlAndSetBwInProgress(pAdapter)), MgntScanInProgress(&(pAdapter->MgntInfo))));
		}
		return hwCenterChannel;
	}
}

