/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	WOLPattern.c
	
Abstract:
	1. This source file is going to be implemented the functions about Wake-on WLAN (WOL).

	    
Major Change History:
	When       		Who               What
	---------- ---------------   -------------------------------
	2009.06.15	tynli		Create version 0. Implement GetWOLWakeUpPattern().
	2009.06.16	tynli		Implement CalculateWOLPatternCRC(), CRC16_CCITT().
	2009.06.19	tynli		Implement ResetWoLPara().
--*/


#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "WOLPattern.tmh"
#endif


//
// Description: Get the wake up frame pattern for wake-on WLAN (WOL).
// 
// pWOLMaskToHW -It is an output. We must to set bit mask to HW in a reverse order, and 
//				sometimes to shift some bits because the payload offset from OS may 
//				be different from real wake up pattern.
// 2009.06.15. by tynli.
//
VOID
GetWOLWakeUpPattern(
	IN PADAPTER	pAdapter,
	IN pu1Byte	pWOLPatternMask,
	IN u4Byte		WOLPatternMaskSize,
	IN pu1Byte	pWOLPatternContent,
	IN u4Byte		WOLPatternContentSize,
	IN u1Byte		Index,
	IN BOOLEAN	bMgntFrame
)
{
	u4Byte	i=0, j=0;
	u4Byte	len=0, mask[4];
	u2Byte	CRCRemainder;
	u1Byte	MaskToHW[MAX_WOL_BIT_MASK_SIZE];
	u1Byte	WOLWakeupPattern[MAX_WOL_PATTERN_SIZE];
	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO pWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	
	// Set the pattern match flag.
	pWoLPatternInfo[Index].IsPatternMatch = 1;
	
	// <Case 1> 8723A Hw just can save 12 patterns mask and CRC in registers, for the DTM reqirement,
		// the device should support 16 patterns, so the remain 4 patterns will be handled by Fw.
		// Keep all the pattern masks and contents, then download them in HaltAdapter(). 2012.03.19. by tynli.
	// <Case 2> To support wake pattern parsing so we need to store the pattern info. 2013.01.10, by tynli.
	AddWoLPatternEntry(pAdapter, pWOLPatternMask, WOLPatternMaskSize, pWOLPatternContent, WOLPatternContentSize, Index);
	
	PlatformZeroMemory((pu1Byte)WOLWakeupPattern, MAX_WOL_PATTERN_SIZE);
	PlatformZeroMemory((pu1Byte)mask, MAX_WOL_BIT_MASK_SIZE);
	PlatformZeroMemory((pu1Byte)MaskToHW, MAX_WOL_BIT_MASK_SIZE);
	
	//RT_PRINT_DATA( (COMP_OID_QUERY|COMP_AP), DBG_LOUD, ("GetWOLWakeUpPattern() Mask: "), 
	//pWOLPatternMask, WOLPatternMaskSize);

	//RT_PRINT_DATA( (COMP_OID_QUERY|COMP_AP), DBG_LOUD, ("GetWOLWakeUpPattern() Pattern: "), 
	//pWOLPatternContent, WOLPatternContentSize);

	//1. Compare if DA  = our MAC ADDR

	
/*
	// 2009.07.09.
	//for DTM bit mask, it will macth from the highest bit of a byte, so we reverse each of byte.
	for(i=0; i<WOLPatternMaskSize; i++)
	{
		pWOLPatternMask[i] = ReverseBit(pWOLPatternMask[i]);
	}
*/


	if(bMgntFrame)
	{
		// The mask is begin from frame content, because Hw will ignore MAC header.
	 	for(i=0; i<WOLPatternMaskSize; i++)
		{
			MaskToHW[i] = pWOLPatternMask[i];
		}
		
		// Remove MAC header 24 bytes.
		for(i=0; i<WOLPatternMaskSize*8; i++)
		{
			if((pWOLPatternMask[i/8]>>(i%8))&0x01)
			{
				WOLWakeupPattern[len] = pWOLPatternContent[i+24];
				//DbgPrint("pWOLWakeupPattern[%d] = 0x%x\n", len, WOLWakeupPattern[len]);
				len++;
			}
		}
		
	}
	else
	{
		//2. To macth HW design, we need to change bit mask. HW catch payload which begin from LLC 
		//in 802.11 packet, but OS set the wake up pattern in 802.3 format (DA[6]|SA[6]|Type[2]|Data), 
		//so (1) we need to shift bit mask left for 6 bits to filter out DA[6], and the type is at the last 2 bits
		//in LLC[8], so (2) we also need to set bit 0-5 to zero in the new bit mask which means SA[6] 
		//to prevent CRC error (HW count from LLC, it is not a SA). by tynli. 2009.07.03.

	 	for(i=0; i<(WOLPatternMaskSize-1); i++)  //(1) Shift 6 bits
		{
			MaskToHW[i] = pWOLPatternMask[i]>>6;
			MaskToHW[i] |= (pWOLPatternMask[i+1]&0x3F)<<2;
		}
		MaskToHW[i] = (pWOLPatternMask[i]>>6)&0x3F;
		MaskToHW[0] &= 0xC0; //(2) Set bit 0-5 to zero

		//3. To get the wake up pattern from the mask.
		//We do not count first 12 bits which means DA[6] and SA[6] in the pattern to match HW design.
		for(i=12; i<WOLPatternMaskSize*8; i++)
		{
			if((pWOLPatternMask[i/8]>>(i%8))&0x01)
			{
				WOLWakeupPattern[len] = pWOLPatternContent[i];
				//DbgPrint("pWOLWakeupPattern[%d] = 0x%x\n", len, WOLWakeupPattern[len]);
				len++;
			}
		}
	}

	//4. Calculate CRC remainder
	CRCRemainder = CalculateWOLPatternCRC(WOLWakeupPattern, len);
	pWoLPatternInfo[Index].CrcRemainder = CRCRemainder;
	RT_TRACE(COMP_POWER, DBG_LOUD, 
		("GetWOLWakeUpPattern(): CrcRemainder = %x\n",pWoLPatternInfo[Index].CrcRemainder));

	//5. Change the byte order of the bit mask to macth HW design.
	for(i=0; i<= (MAX_WOL_BIT_MASK_SIZE-4); i=i+4, j++)
	{
		mask[j] = MaskToHW[i];
		mask[j] |= (MaskToHW[i+1]<<8);
		mask[j] |= (MaskToHW[i+2]<<16);
		mask[j] |= (MaskToHW[i+3]<<24);
		//DbgPrint("mask[%d] = %x\n", j, mask[j]);
		pWoLPatternInfo[Index].Mask[j] = mask[j];
		//DbgPrint("pWoLPatternInfo[Index].Mask[%d] = %x\n", j, pWoLPatternInfo[Index].Mask[j]);
	}

	{
		// Download them in HaltAdapter() on 8723A. 2012.07.17, by tynli.
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_WF_MASK, (pu1Byte)(&Index)); 
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_WF_CRC, (pu1Byte)(&Index)); 
	}
}


//
// Description: 
// 		To calculate the CRC remainder for the WOL wake up pattern.
// Input: 
//		A WOL Pattern, pattern length
// Output: 
//		The CRC remainder for the pattern
// 2009.06.16. by tynli.
//
u2Byte
CalculateWOLPatternCRC(
	pu1Byte	Pattern,
	u4Byte	PatternLength
)
{
//    unsigned char data[2]={0xC6,0xAA};
	u2Byte	CRC=0xffff;
	u4Byte	i;
	
	for(i=0; i<PatternLength; i++)
	{
		CRC=CRC16_CCITT(Pattern[i], CRC);
		//DbgPrint("Pattern[%d] = %x, ", i, Pattern[i]);
	}

	//DbgPrint("\n");

	CRC=~CRC; 
	//DbgPrint("CRC =%x\n",CRC);

	return CRC;

}

//
// Description: 
//		This is not a standard CRC16-CCITT algorithm, we re-write it to C code from
// 		VR code which is  from HW designer. 
// Input: 
//		1 byte data, CRC remainder
// Output: 
//		The CRC remainder for each byte
// 2009.06.16. by tynli and SD1 Isaac.
//
u2Byte
CRC16_CCITT(
	u1Byte	data,
	u2Byte	CRC
)
{
	u1Byte	shift_in, DataBit, CRC_BIT11, CRC_BIT4, CRC_BIT15 ;
	u1Byte	index;
	u2Byte	CRC_Result;

	for(index=0;index<8;index++)
	{
		CRC_BIT15=((CRC&BIT15) ? 1:0);
		DataBit  =(data&(BIT0<<index) ? 1:0);
		shift_in=CRC_BIT15^DataBit;
		//printf("CRC_BIT15=%d, DataBit=%d, shift_in=%d \n",CRC_BIT15,DataBit,shift_in);
		
		CRC_Result=CRC<<1;
		//set BIT0 
		//	printf("CRC =%x\n",CRC_Result);
		//CRC bit 0 =shift_in,
		if(shift_in==0)
			CRC_Result&=(~BIT0); 
		else
			CRC_Result|=BIT0;
		//printf("CRC =%x\n",CRC_Result);

		CRC_BIT11 = ((CRC&BIT11) ? 1:0)^shift_in;
		if(CRC_BIT11==0)
			CRC_Result&=(~BIT12); 
		else
			CRC_Result|=BIT12;
		//printf("bit12 CRC =%x\n",CRC_Result);

		CRC_BIT4 = ((CRC&BIT4) ? 1:0)^shift_in;
		if(CRC_BIT4==0)
			CRC_Result&=(~BIT5); 
		else
			CRC_Result|=BIT5;
		//printf("bit5 CRC =%x\n",CRC_Result);

		CRC=CRC_Result;
	}

	return CRC;
	
}

//
// Description: For wake-on WLAN, the CRC register value should be set to a non-zero value when
// 			there is no wake up frame comeing to calculate CRC result or it will hit the HW bug when CRC value = 0.
//
//	2009.06.19. by tynli.
VOID
ResetWoLPara(
	IN		PADAPTER		Adapter 
)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO	pPmWOLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	u1Byte	i;

	PlatformZeroMemory(pPmWOLPatternInfo, 
		sizeof(RT_PM_WOL_PATTERN_INFO)*(MAX_SUPPORT_WOL_PATTERN_NUM(Adapter)));
	for(i=0; i<MAX_SUPPORT_WOL_PATTERN_NUM(Adapter); i++) //reset structure content
	{
			pPmWOLPatternInfo[i].CrcRemainder = 0xffff;
			pPmWOLPatternInfo[i].PatternType = eUnknownType;  //YJ,add,110726
			pPmWOLPatternInfo[i].HwWFMIndex = 0xff;
	}

	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_RESET_WFCRC, 0); 
}

VOID
ConstructUserDefinedWakeUpPattern(
	IN		PADAPTER		Adapter
)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	//u1Byte		AuthBuf[100];
	u4Byte		AuthBufLen;
	OCTET_STRING	AuthChallengetext;
	u1Byte		AuthMaskBuf;
	u1Byte		AuthMaskBufLen;
	u1Byte		Index;
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	pu1Byte 		AuthBuf;
	PRT_GEN_TEMP_BUFFER pGenBufAuthPacket;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("===> ConstructUserDefinedWakeUpPattern()\n"));

	if(ACTING_AS_AP(Adapter) && pPSC->APOffloadEnable)
	{
		pGenBufAuthPacket = GetGenTempBuffer (Adapter, 100);
		AuthBuf = (u1Byte *)pGenBufAuthPacket->Buffer.Ptr;
		
		// 
		// 1. Auth
		//
		//Since this is always the 1st authentication frame
		AuthChallengetext.Length = 0;

		//Send authentication frame
		ConstructAuthenticatePacket(
			Adapter,
			AuthBuf,
			&AuthBufLen,
			pMgntInfo->Bssid,
			0,
			1,
			StatusCode_success,
			AuthChallengetext);

		RT_PRINT_DATA(COMP_POWER, DBG_TRACE, "ConstructUserDefinedWakeUpPattern(): Auth ",
			&AuthBuf, AuthBufLen);

		AuthMaskBuf = 0x0F;
		AuthMaskBufLen = 1;

		//Find the index of the first empty entry.
		for(Index=0; Index<MAX_SUPPORT_WOL_PATTERN_NUM(Adapter); Index++)
		{
			if(pPmWoLPatternInfo[Index].PatternId == 0)
				break;
		}
		
		if(Index >= MAX_SUPPORT_WOL_PATTERN_NUM(Adapter))
		{
			RT_TRACE(COMP_POWER, DBG_LOUD,
				("SET OID_PM_ADD_WOL_PATTERN: The number of wake up pattern is more than MAX_SUPPORT_WOL_PATTERN_NUM or the pattern Id is exist.\n"));
		}

		// Set the pattern information.
		pPmWoLPatternInfo[Index].PatternId = 0xFFFF; //for temp
		pPmWoLPatternInfo[Index].PatternType = eUnicastPattern;
		pPmWoLPatternInfo[Index].IsUserDefined = 1;

		GetWOLWakeUpPattern(
			Adapter,
			&AuthMaskBuf,
			AuthMaskBufLen,
			AuthBuf,
			AuthBufLen,
			Index,
			TRUE);

		pPSC->WoLPatternNum++;

		ReturnGenTempBuffer(Adapter, pGenBufAuthPacket);
	}
}

//
// Description:
//	When GTK is updated, check if the Wake on WLAN event happened before, and if yes indicate
//	event to the upper layer.
// Argumets:
//	[in] pAdapter -
//		The adapter context.
// Return:
//	NONE.
// By Bruce, 2011-06-09.
//
VOID
WolByGtkUpdate(
	IN	PADAPTER	pAdapter
	)
{
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL((&pAdapter->MgntInfo));

	if((pPSC->WakeUpReason & (WOL_REASON_GTK_UPDATE | WOL_REASON_PTK_UPDATE)) != 0) 
	{		
		if(PlatformGetCurrentTime() <= pPSC->LastWakeUpTime + 20000000) // 10 sec
		{			
			RT_TRACE_F(COMP_POWER, DBG_LOUD, ("Wake up by GTK and Indicate the WOL by GTK event!\n"));
			PlatformIndicateCustomStatus(
				pAdapter,
				RT_CUSTOM_EVENT_WOL_GTK,
				RT_CUSTOM_INDI_TARGET_IHV,
				&pPSC->SleepMode,
				sizeof(pPSC->SleepMode));
		}
		else
		{
			RT_TRACE_F(COMP_POWER, DBG_LOUD, ("The current timt of updating GTK is too long from the last WOL time! Skip indication....\n"));
		}
		pPSC->WakeUpReason &= ~(WOL_REASON_GTK_UPDATE | WOL_REASON_PTK_UPDATE);
	}
}

VOID
RemoveUserDefinedWoLPattern(
	IN		PADAPTER		Adapter
)
{
	PMGNT_INFO	pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO	pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	u1Byte		Index;

	for(Index=0; Index<MAX_SUPPORT_WOL_PATTERN_NUM(Adapter); Index++)
	{
		if(pPmWoLPatternInfo[Index].IsUserDefined == 1)
		{
			//Reset the structure and set WFCRC register to non-zero value.
			pPmWoLPatternInfo[Index].PatternId = 0;
			PlatformZeroMemory(pPmWoLPatternInfo[Index].Mask, sizeof(pPmWoLPatternInfo[Index].Mask));
			pPmWoLPatternInfo[Index].CrcRemainder = 0xffff;
			pPmWoLPatternInfo[Index].IsPatternMatch = 0;
			pPmWoLPatternInfo[Index].IsUserDefined = 0;
			pPmWoLPatternInfo[Index].IsSupportedByFW = 0;
			
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_WF_MASK, (pu1Byte)(&Index)); 
			Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_WF_CRC, (pu1Byte)(&Index)); 
			pPmWoLPatternInfo[Index].HwWFMIndex = 0xff; // reset the value after clear HW/CAM entry.
			
			pPSC->WoLPatternNum--;
		}
	}

}

//
// Description: Save the wake patten masks and contents.
//
VOID
AddWoLPatternEntry(
	IN PADAPTER	Adapter,
	IN pu1Byte	pWOLPatternMask,
	IN u4Byte		WOLPatternMaskSize,
	IN pu1Byte	pWOLPatternContent,
	IN u4Byte		WOLPatternContentSize,
	IN u1Byte		Index
)
{
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO	pWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	u1Byte	i, j;

	// Reset the entry.
	PlatformZeroMemory(&(pWoLPatternInfo[Index].FwPattern),sizeof(H2C_WOL_PATTERN_MATCH_INFO));

	//
	// Add bit mask entry.
	//
	for(i=0; i<WOLPatternMaskSize; i++)
	{
		pWoLPatternInfo[Index].FwPattern.BitMask[i] = pWOLPatternMask[i];
		for(j=0; j<8; j++)
		{
			if((pWoLPatternInfo[Index].FwPattern.BitMask[i]>>j)&0x01)
			{
				pWoLPatternInfo[Index].FwPattern.ValidBitNum++;
			}
		}

	}	
	
	//
	// Add pattern content entry.
	//
	for(i=0; i<WOLPatternContentSize; i++)
	{
		pWoLPatternInfo[Index].FwPattern.PatternContent[i] = pWOLPatternContent[i];
	}

	RT_PRINT_DATA(COMP_POWER, DBG_TRACE,"PatternContent: \n", &(pWoLPatternInfo[Index].FwPattern.PatternContent[0]), WOLPatternContentSize);
}

//
// Description: Get the entrties to download to Fw.
// Input:	
//	- EnteryNum: [total pattern number (set by the OID) - 12 (Hw capability)]
//
VOID
GetWoLPatternMatchOffloadEntries(
	IN PADAPTER	Adapter,
	IN u1Byte	EntryNum
)
{	
	PMGNT_INFO				pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO	pWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	u1Byte	i=0;
	u1Byte	MinNum = 0xFF, NextMinNum=0xFF;
	u1Byte	NumOfSelected=0;

	RT_TRACE(COMP_POWER, DBG_LOUD, ("GetWoLPatternMatchOffloadEntries(): Number of entries=%d\n", EntryNum));

	// Find the minimun number of bits.
	for(i=0; i<MAX_SUPPORT_WOL_PATTERN_NUM(Adapter); i++)
	{
		if(pWoLPatternInfo[i].FwPattern.ValidBitNum == 0)
			break;
		if(pWoLPatternInfo[i].FwPattern.ValidBitNum <= MinNum)
		{
			MinNum = pWoLPatternInfo[i].FwPattern.ValidBitNum;
		}
	}

	//
	// Find the smallest four entries.
	//
	do
	{
		NextMinNum = 0xFF;
		for(i=0; i<MAX_SUPPORT_WOL_PATTERN_NUM(Adapter); i++)
		{
			if(pWoLPatternInfo[i].FwPattern.ValidBitNum == 0)
				break;
			if(pWoLPatternInfo[i].FwPattern.ValidBitNum == MinNum)
			{
				if(NumOfSelected > EntryNum)
					break;
				pWoLPatternInfo[i].IsSupportedByFW = 1;
				NumOfSelected++;
				//DbgPrint("Find the littler index(%d)\n", i);
			}
			else if(pWoLPatternInfo[i].FwPattern.ValidBitNum > MinNum)
			{
				if(pWoLPatternInfo[i].FwPattern.ValidBitNum <= NextMinNum)
				{
					NextMinNum = pWoLPatternInfo[i].FwPattern.ValidBitNum;
				}
			}
		}
		//DbgPrint("NextMinNum=%d\n", NextMinNum);
		MinNum = NextMinNum;
	}
	while((NumOfSelected < EntryNum) && (NumOfSelected < pPSC->WoLPatternNum));

}

VOID
WoL_TranslateDot11FrameToDot3(
	PADAPTER	Adapter,
	PRT_RFD		pRfd,
	pu1Byte		pDot3Buffer,
	pu2Byte		pDot3BufLen
)
{
	OCTET_STRING	frame = {NULL, 0};
	pu1Byte			pHeader;
	BOOLEAN			bToDS, bFromDS;
	u1Byte			MacDestAddr[6]={0}, MacSrcAddr[6]={0};
	u1Byte			offset=0;

	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	pHeader = frame.Octet;

	RT_PRINT_DATA(COMP_INIT, DBG_TRACE, "WoL_TranlateDot11FrameToDot3(): frame \n",frame.Octet,frame.Length);

	bToDS = (Frame_ToDS(frame) ? TRUE : FALSE);
	bFromDS = (Frame_FromDS(frame) ? TRUE : FALSE);
	
	if(bToDS && !bFromDS)
	{
		GET_80211_HDR_ADDRESS3(pHeader, MacDestAddr); // DA
		GET_80211_HDR_ADDRESS2(pHeader, MacSrcAddr); // SA
	}
	else if(!bToDS && bFromDS)
	{
		GET_80211_HDR_ADDRESS1(pHeader, MacDestAddr);
		GET_80211_HDR_ADDRESS3(pHeader, MacSrcAddr);
	}
	else if(bToDS && bFromDS)
	{
		GET_80211_HDR_ADDRESS3(pHeader, MacDestAddr);
		GET_80211_HDR_ADDRESS4(pHeader, MacSrcAddr);
	}
	else
	{
		GET_80211_HDR_ADDRESS1(pHeader, MacDestAddr);
		GET_80211_HDR_ADDRESS2(pHeader, MacSrcAddr);
	}
	offset += sMacHdrLng;

	if( pRfd->Status.bIsQosData )
	{
		offset += sQoSCtlLng;
	}

	if( pRfd->Status.bContainHTC)
	{
		offset += sHTCLng;
	}

	if(Frame_ValidAddr4(frame))
	{
		offset += 6;
	}

	if(Frame_WEP(frame))
	{
		offset += Adapter->MgntInfo.SecurityInfo.EncryptionHeadOverhead;
	}

	if(frame.Length <= offset)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("WoL_TranslateDot11FrameToDot3(): Error frame length!\n"));
		return;
	}

	//
	if(IsDataFrame(frame.Octet))
	{
		// Remove LLC header length (6 bytes) to get IP type feild.]
		// LLC_HEADER_SIZE
		if((frame.Length-offset) > LLC_HEADER_SIZE) // check length
		{
			if(*(pHeader+offset) == 0xaa &&
				*(pHeader+offset+1) == 0xaa &&
				*(pHeader+offset+2) == 0x03)
			{
				offset += LLC_HEADER_SIZE;
				//DbgPrint("Find LLC header!!\n");
			}
		}
	}

	RT_TRACE(COMP_INIT, DBG_TRACE, ("WoL_TranslateDot11FrameToDot3(): offset = %d\n", offset));

	// Mapping to 802.3 packet format.
	PlatformMoveMemory(pDot3Buffer, MacDestAddr, 6); // DA
	PlatformMoveMemory(pDot3Buffer+6, MacSrcAddr, 6); // SA
	PlatformMoveMemory(pDot3Buffer+12, pHeader+offset, (frame.Length-offset)); //IP type+data
	*pDot3BufLen = 12 + frame.Length - offset;

	RT_PRINT_DATA(COMP_INIT, DBG_TRACE, "WoL_TranlateDot11FrameToDot3(): pDot3Buffer \n",pDot3Buffer, *pDot3BufLen);
}

//
// Description: Check the received packet if a magic packet.
// 2012.07.31, by tynli.
//
BOOLEAN
WoL_IsMagicPacket(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
)
{
	OCTET_STRING	frame = {NULL, 0};
	pu1Byte 			pHeader;
	u1Byte			offset=0, i, j;
	u2Byte			PayloadLen;
	BOOLEAN			bMatchPacket=FALSE;
	
	FillOctetString(frame, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	pHeader = frame.Octet;

	//RT_PRINT_DATA(COMP_RECV, DBG_LOUD,"WoL_IsMagicPacket(): frame \n",frame.Octet,frame.Length);

	// Get payload start offset.
	offset += sMacHdrLng;

	if( pRfd->Status.bIsQosData )
	{
		offset += sQoSCtlLng;
	}

	if( pRfd->Status.bContainHTC)
	{
		offset += sHTCLng;
	}

	if(Frame_ValidAddr4(frame))
	{
		offset += 6;
	}

	if(Frame_WEP(frame))
	{
		offset += Adapter->MgntInfo.SecurityInfo.EncryptionHeadOverhead;
	}

	if(frame.Length <= offset)
	{
		RT_TRACE(COMP_INIT, DBG_LOUD, ("WoL_IsMagicPacket(): Error frame length!\n"));
		return bMatchPacket;
	}

	RT_TRACE(COMP_RECV, DBG_TRACE, ("WoL_IsMagicPacket(): offset = %d\n", offset));

	pHeader += offset;
	PayloadLen = frame.Length - offset;

	// Search for magic packet pattern
	for(i = 0; i<PayloadLen; )
	{
		if(*(pHeader+i) == 0xFF)
		{
			if((i+6) > PayloadLen) // check remain buffer length
			{
				RT_TRACE(COMP_RECV, DBG_TRACE, ("WoL_IsMagicPacket(): Packet length error. 1\n"));
				break;
			}
			
			// Find FF FF FF FF FF FF pattern.
			if(*(pHeader+i+1) == 0xFF && *(pHeader+i+2) == 0xFF && 
				*(pHeader+i+3) == 0xFF && *(pHeader+i+4) == 0xFF &&
				*(pHeader+i+5) == 0xFF)
			{
				i += 6;
				if((i+16*6) > PayloadLen) // check remain buffer length
				{
					RT_TRACE(COMP_RECV, DBG_TRACE, ("WoL_IsMagicPacket(): Packet length error. 2\n"));
					break;
				}
				
				// Repeat STA addr 16 times.
				for(j=0; j<16; j++)
				{
					if(PlatformCompareMemory((pHeader+i+j*6),  Adapter->CurrentAddress, 6) == 0)
					{	// Match
						if(j == 15)
						{
							//Adapter->HwWakeUpEvent = 2;
							bMatchPacket =TRUE;
							RT_TRACE(COMP_POWER, DBG_LOUD, ("WoL_IsMagicPacket(): Find magic packet\n"));
						}
					}
					else
					{
						RT_TRACE(COMP_POWER, DBG_TRACE, ("WoL_IsMagicPacket(): Unmatch!!\n"));
						break;
					}
				}
			}
			else
			{
				i += 1;
			}
		}
		else
		{
			i += 1;
		}

	}

	return bMatchPacket;
}

//
// Description: Check the received packet if a pattern match packet.
//			  We should translate the reveived packet format to 802.3 format to match the pattern content
//			  set from the upper layer. If there is one pattern packet be matched, then return TRUE.
// 2012.07.31, by tynli.
//
BOOLEAN
WoL_IsPatternMatchPacket(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
)
{
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	PRT_PM_WOL_PATTERN_INFO	pPmWoLPatternInfo = &(pPSC->PmWoLPatternInfo[0]);
	u1Byte			Index, i, j;
	BOOLEAN			bMatchPacket=TRUE;
	u1Byte			offset = 0;
	pu1Byte			pDot3Packet;
	PRT_GEN_TEMP_BUFFER pGenBufPacket;
	u2Byte			Dot3PacketLen;

	pGenBufPacket = GetGenTempBuffer (Adapter, pRfd->PacketLength);
	pDot3Packet = (u1Byte *)pGenBufPacket->Buffer.Ptr;

	// Translate 802.11 packet format to 802.3 format to match the pattern match content set by OID.
	WoL_TranslateDot11FrameToDot3(Adapter, pRfd, pDot3Packet, &Dot3PacketLen);
	
	for(Index=0; Index<MAX_SUPPORT_WOL_PATTERN_NUM(Adapter); Index++)
	{
		bMatchPacket = TRUE;
		if(pPmWoLPatternInfo[Index].PatternId != 0)
		{
			for(i=0; i<16; i++)
			{
				for(j=0; j<8; j++)
				{
					offset = i*8+j;
					if(pPmWoLPatternInfo[Index].FwPattern.BitMask[i] & (0x01<<j)) // bitmask == 1
					{ 
						if(Dot3PacketLen < offset)
						{
							bMatchPacket = FALSE;
							break;
						}

						if(pDot3Packet[offset] != pPmWoLPatternInfo[Index].FwPattern.PatternContent[offset])
						{ 
							bMatchPacket = FALSE;
							break;
						}
					}
				}
				if(!bMatchPacket)
					break;
			}

			if(bMatchPacket)
			{
				//Adapter->HwWakeUpEvent = 3 | (Index<<4);
				RT_TRACE(COMP_POWER, DBG_LOUD, ("Find the pattern match wake packet!!Index(%d)\n", Index));
				break;
			}
		}
		else
		{
			bMatchPacket = FALSE;
		}
	}

	if(bMatchPacket)
	{
		RT_PRINT_DATA(COMP_DBG, DBG_TRACE, "Dump pattern bitmask \n", pPmWoLPatternInfo[Index].FwPattern.BitMask, 16);
		RT_PRINT_DATA(COMP_DBG, DBG_TRACE, "Dump pattern content \n", pPmWoLPatternInfo[Index].FwPattern.PatternContent, 128);
	}

	ReturnGenTempBuffer(Adapter, pGenBufPacket);

	return bMatchPacket;
}

//
// Description: Handle WoWLAN related Rx packets.
//
// Return value: TRUE - It is a wake packet.
//			    FALSE - It is a normal packet, and should be handled by normal Rx path.
//
BOOLEAN
WoL_HandleReceivedPacket(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
)
{
	//OCTET_STRING	frame = {NULL, 0};	
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
	//static BOOLEAN			bWakePacket = FALSE;
	PRT_RFD_STATUS				pRtRfdStatus = &pRfd->Status;
	BOOLEAN			bResult=FALSE;

	if(pPSC->bFindWakePacket)
	{
		RT_TRACE(COMP_POWER, DBG_TRACE, ("Already found the first wake packet and return!\n"));
		return FALSE;
	}

	if(HW_SUPPORT_PARSING_WAKE_PACKET(Adapter))
	{
		if(!pRtRfdStatus->WakeMatch)
		{
			RT_TRACE(COMP_POWER, DBG_TRACE, ("Not a wake packet and return!\n"));
			return FALSE;
		}
		bResult = TRUE;
		pPSC->bFindWakePacket = TRUE;

		if(pPSC->WakeUpReason == 0) // To prevent from covering Hw reason
		{
			if(pRtRfdStatus->WakeMatch & BIT0)
				pPSC->WakeUpReason = WOL_REASON_UNICAST_PKT;
			else if(pRtRfdStatus->WakeMatch & BIT1)
				pPSC->WakeUpReason = WOL_REASON_MAGIC_PKT;
			else if(pRtRfdStatus->WakeMatch & BIT2)
				pPSC->WakeUpReason = WOL_REASON_PATTERN_PKT;
		}
		RT_TRACE(COMP_POWER, DBG_LOUD, ("~~~~~~~~ It is a wake packet(%d)!\n", pRtRfdStatus->WakeMatch));
		//RT_PRINT_DATA(COMP_INIT, DBG_LOUD,"WoL_HandleReceivedPacket: \n", pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	
	}
	else
	{

	//	RT_PRINT_DATA(COMP_INIT, DBG_LOUD,"WoL_HandleReceivedPacket: \n",frame.Octet,frame.Length);

		// Find the wake packet.
		if(WoL_IsMagicPacket(Adapter, pRfd))
		{
			pPSC->bFindWakePacket = TRUE;
			bResult = TRUE;
			if(pPSC->WakeUpReason == 0) // To prevent from covering Hw reason
				pPSC->WakeUpReason = WOL_REASON_MAGIC_PKT;
		}
		else if(WoL_IsPatternMatchPacket(Adapter, pRfd))
		{
			pPSC->bFindWakePacket = TRUE;
			bResult = TRUE;
			if(pPSC->WakeUpReason == 0) // To prevent from covering Hw reason
				pPSC->WakeUpReason = WOL_REASON_PATTERN_PKT;
		}
	}
	
	// Indicate PM wake reason and packet to the OS.
	if(pPSC->bFindWakePacket)
	{
		PlatformIndicatePMWakeReason(Adapter, TRUE, pRfd->Buffer.VirtualAddress, pRfd->PacketLength);
	}

	return bResult;
}


