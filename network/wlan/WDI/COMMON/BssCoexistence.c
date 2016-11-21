#include "Mp_Precomp.h"


#if WPP_SOFTWARE_TRACE
#include "BssCoexistence.tmh"
#endif

VOID
BSS_ParsingOBSSInfoElement(
	PADAPTER		Adapter,
	OCTET_STRING	osFrame,
	PRT_WLAN_BSS	pBssDesc
	)
{
	pBssDesc->BssHT.OBSSScanInterval = 
	GET_OBSS_PARAM_ELE_SCAN_INTERVAL(osFrame.Octet);
}

VOID
BSS_ParsingBSSCoexistElement(
	PADAPTER		Adapter,
	OCTET_STRING	osFrame,
	PRT_WLAN_BSS	pBssDesc
	)
{
	pBssDesc->BssHT.bdOBSSExemption = 
	GET_BSS_COEXISTENCE_ELE_OBSS_EXEMPTION_GRT(osFrame.Octet);
}

VOID
BSS_AppendExentedCapElement(
	PADAPTER		pAdapter,
	POCTET_STRING	posFrame
	)
{
	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	OCTET_STRING			Info;
	u1Byte					InfoContent[8] = {0};

	FillOctetString(Info, InfoContent, sizeof(InfoContent));
	
	if(pHTInfo->bBssCoexist)
	{
		SET_EXT_CAPABILITY_ELE_BSS_COEXIST(InfoContent, 1);
	}

	if(pMgntInfo->pVHTInfo->bCurrentVHTSupport)
	{
		SET_EXT_CAPABILITY_ELE_OP_MODE_NOTIF(InfoContent, 1);
	}

	PacketMakeElement( posFrame, EID_EXTCapability, Info);
}


VOID
BSS_AppendBSSCoexistReportElement(
	PADAPTER		Adapter,
	POCTET_STRING	posFrame
	)
{
	u2Byte 			i,j,k;
	PRT_WLAN_BSS	pRtBss = NULL;
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	OCTET_STRING	Info;
	u1Byte			InfoContent[16] = {0};
//	u1Byte			ICS[255][15];
	//pu1Byte			*ICS;
	pu1Byte			ICS;
	PRT_GEN_TEMP_BUFFER pGenBufICS;

	//
	// 2011/05/25 Justin/MH Two dimenssion array can not use below method. You need
	// to allocate two layers pointer for array ICS[255][15]. To simplize the problem. We will only
	// support one dimenssion arry to simulate 2/3/.. layer array as below.
	//
	
	pGenBufICS = GetGenTempBuffer (Adapter, 255*15);
	//ICS = (u1Byte **)pGenBufICS->Buffer.Ptr;
	ICS = (u1Byte *)pGenBufICS->Buffer.Ptr;

	//PlatformZeroMemory (*ICS, 255*15);
	for (i=0; i<pMgntInfo->NumBssDesc4Query; i++)
	{
		pRtBss = pMgntInfo->bssDesc4Query+i;
		
		if(pRtBss->BssHT.bdSupportHT == FALSE)
		{
			//ICS[pRtBss->RegulatoryClass][pRtBss->ChannelNumber]=1;
			//if(ICS[pRtBss->RegulatoryClass][0] == 0)
				//ICS[pRtBss->RegulatoryClass][0] = 1;
			ICS[pRtBss->RegulatoryClass*15+pRtBss->ChannelNumber]=1;
			if(ICS[pRtBss->RegulatoryClass*15+0] == 0)
				ICS[pRtBss->RegulatoryClass*15+0] = 1;
		}
	}

	for(i= 0;i<255;i++)
	{
		if(posFrame->Length > sMaxMpduLng)
			break;
		//if(ICS[i][0] == 1)
		if(ICS[i*15+0] == 1)
		{
			k = 0;
			SET_BSS_INTOLERANT_ELE_REG_CLASS(InfoContent,i);
			for(j=1;j<=14;j++)
			{
				//if(ICS[i][j]==1)
				if(ICS[i*15+j]==1)
				{
					if(k<16)
					{
						SET_BSS_INTOLERANT_ELE_CHANNEL(InfoContent+k, j);
						k++;
					}	
				}	
			}	
			FillOctetString(Info, InfoContent, 2+k-1);
			//PRINT_DATA("", Info.Octet, Info.Length);
			PacketMakeElement(posFrame, EID_BSSIntolerantChlReport, Info);
		}			
	}
	ReturnGenTempBuffer (Adapter, pGenBufICS);
}


VOID
ConstructBSSCoexistPacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	BOOLEAN			TE_A,
	BOOLEAN			TE_B
	)
{
	PMGNT_INFO      	pMgntInfo = &Adapter->MgntInfo;
	PRT_HIGH_THROUGHPUT	pHTInfo = GET_HT_INFO(pMgntInfo);
	OCTET_STRING		osBSSCOEXISTFrame;
	OCTET_STRING		Info;
	u1Byte				InfoContent[1] = {0};

	FillOctetString(osBSSCOEXISTFrame, Buffer, 0);
	*pLength = 0;

	ConstructMaFrameHdr(
					Adapter, 
					Adapter->MgntInfo.Bssid, 
					ACT_CAT_PUBLIC, 
					ACT_PUBLIC_BSSCOEXIST, 
					&osBSSCOEXISTFrame);	

	if(IS_WIRELESS_MODE_5G(Adapter))
	{
		SET_BSS_COEXISTENCE_ELE_FORTY_INTOLERANT(InfoContent, 0);
	}
	else
	{
		SET_BSS_COEXISTENCE_ELE_FORTY_INTOLERANT(InfoContent, pHTInfo->b40Intolerant);
	}
	
	if(TE_B)
		SET_BSS_COEXISTENCE_ELE_20_WIDTH_REQ(InfoContent, 1);
	
	FillOctetString(Info, InfoContent, 1);
	PacketMakeElement(&osBSSCOEXISTFrame, EID_BSSCoexistence, Info);	

	if(TE_A)
		BSS_AppendBSSCoexistReportElement(Adapter, &osBSSCOEXISTFrame);

	*pLength = osBSSCOEXISTFrame.Length;	
}


VOID
SendBSSCoexistPacket(
	PADAPTER		Adapter,
	BOOLEAN			TE_A,
	BOOLEAN			TE_B
	)
{
	PRT_TCB					pTcb;
	PRT_TX_LOCAL_BUFFER		pBuf;
	u1Byte					DataRate = MgntQuery_MgntFrameTxRate(Adapter, Adapter->MgntInfo.Bssid);

	PlatformAcquireSpinLock(Adapter, RT_TX_SPINLOCK);

	if(MgntGetBuffer(Adapter, &pTcb, &pBuf))
	{
		ConstructBSSCoexistPacket(
				Adapter, 
				pBuf->Buffer.VirtualAddress, 
				&pTcb->PacketLength,
				TE_A,
				TE_B
				);

		if(pTcb->PacketLength != 0)
			MgntSendPacket(Adapter, pTcb, pBuf, pTcb->PacketLength, NORMAL_QUEUE, DataRate);
	}
	
	PlatformReleaseSpinLock(Adapter, RT_TX_SPINLOCK);	
}

RT_STATUS
OnBssCoexistence(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	return RT_STATUS_NOT_SUPPORT;
}

VOID
BSS_OnScanComplete(
	IN	PADAPTER		Adapter
	)
{
	u2Byte 					i;
	PRT_WLAN_BSS			pRtBss = NULL;
	BOOLEAN					TE_A = FALSE, TE_B = FALSE;
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	u8Byte					curSystemTimeUs = 0;

	RT_TRACE(COMP_SCAN, DBG_LOUD, ("bCurrentHTSupport %d bPeerBssCoexistence %d, bPeer40MHzIntolerant %d, bCurOBSSScanExemptionGrt %d  CurOBSSScanInterval %d\n", pHTInfo->bCurrentHTSupport, pHTInfo->bPeerBssCoexistence, pHTInfo->bPeer40MHzIntolerant, pHTInfo->bCurOBSSScanExemptionGrt, pHTInfo->CurOBSSScanInterval));

	do
	{
		// Only collect information to associated ap
		if(!pMgntInfo->mAssoc)
			break;
		// Only sent message to FC HT AP
		else if(pHTInfo->bCurrentHTSupport == FALSE || pHTInfo->bPeer40MHzCap == FALSE)
			break;
		else if(pHTInfo->bBssCoexist == FALSE || pHTInfo->bPeerBssCoexistence == FALSE)
			break;
		else if(!CHNL_GetRegBWSupport(Adapter))
			break;
		else if(TRUE == pHTInfo->bCurOBSSScanExemptionGrt)
			break;

		for (i=0; i<pMgntInfo->NumBssDesc4Query; i++)
		{
			pRtBss = pMgntInfo->bssDesc4Query+i;
			
			if(pRtBss->BssHT.bdSupportHT == FALSE)
				TE_A = TRUE;
			else if(pRtBss->BssHT.bd40Intolerant == TRUE)
				TE_B = TRUE;

			if(TE_A && TE_B)
				break;
		}
		
		pHTInfo->IdleOBSSScanCnt = 0;
		curSystemTimeUs = PlatformGetCurrentTime();

		// Check the conditions and the interval to send the report (we don't send the report repeatly as soon)
		if((TE_A || TE_B) &&
			(0 == pHTInfo->lastTimeSentObssRptUs || 
			((curSystemTimeUs - pHTInfo->lastTimeSentObssRptUs) > ((pHTInfo->CurOBSSScanInterval * 2 / 3) * 1000000))))
		{			
			RT_TRACE_F(COMP_HT, DBG_LOUD, 
				("SendBSSCoexistPacket() TE_A(%d), TE_B(%d), (curSystemTimeUs - pHTInfo->lastTimeSentObssRptUs) = %d \n", TE_A, TE_B, (u4Byte)(curSystemTimeUs - pHTInfo->lastTimeSentObssRptUs)));
			pHTInfo->lastTimeSentObssRptUs = curSystemTimeUs;
			SendBSSCoexistPacket(Adapter, TE_A, TE_B);
		}
	}while(FALSE);

	RT_TRACE_F(COMP_HT, DBG_LOUD, ("<== IdleOBSSScanCnt = %d, pHTInfo->lastTimeSentObssRptUs = %d\n", pHTInfo->IdleOBSSScanCnt, (u4Byte)pHTInfo->lastTimeSentObssRptUs));
}

//
// Description:
//	The wachdog monitor the BSS coexistance in 40MHz. The OBSS information asks the client
//	to periodically scan the 40MHz intolerant APs.
// Arguments:
//	[in] pAdapter -
//		The current Adapter context.
// Return:
//	None
// By Bruce, 2011-07-29.
//
VOID
BSS_IdleScanWatchDog(
	IN	PADAPTER		pAdapter
	)
{
	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);
	PRT_HIGH_THROUGHPUT		pHTInfo = GET_HT_INFO(pMgntInfo);
	u2Byte					ObssScanInterfaval = 0;

	// Only collect information to associated ap 
	if(!pMgntInfo->mAssoc)
		return;
	
	// Only sent message to FC HT AP
	if(pHTInfo->bCurrentHTSupport == FALSE)
		return;
	// We are not in 40MHz
	else if(!CHNL_RUN_ABOVE_40MHZ(pMgntInfo))
		return;
	else if(pHTInfo->bBssCoexist == FALSE || pHTInfo->bPeerBssCoexistence == FALSE) 
		return;
	else if(pHTInfo->bCurOBSSScanExemptionGrt == TRUE)
		return;
	else if(0 == pHTInfo->CurOBSSScanInterval)
		return;
	else if(pMgntInfo->LinkDetectInfo.bHigherBusyTraffic)
		return;
	
	// We don't want the scan interval too small.
	// By Bruce, 2011-07-28.
	if(pHTInfo->CurOBSSScanInterval < RT_OBSS_MIN_TRIGGER_SCAN_INTERVAL)
		ObssScanInterfaval = RT_OBSS_MIN_TRIGGER_SCAN_INTERVAL;
	else
		ObssScanInterfaval = pHTInfo->CurOBSSScanInterval;

	pHTInfo->IdleOBSSScanCnt ++;

	if((pHTInfo->IdleOBSSScanCnt * RT_CHECK_FOR_HANG_PERIOD) >= ObssScanInterfaval)
	{
		RT_TRACE_F(COMP_HT, DBG_LOUD, ("Reaches the OBSS scan interval, start to scan....\n"));
		pHTInfo->IdleOBSSScanCnt = 0;
		
		MgntActSet_802_11_BSSID_LIST_SCAN(pAdapter);
	}
}
