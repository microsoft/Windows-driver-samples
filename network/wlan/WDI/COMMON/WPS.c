/******************************************************************************

     (c) Copyright 2011, RealTEK Technologies Inc. All Rights Reserved.

 Module:	WPS.c	(RTL8190  Source C File)

 Note:		Declare some variable which will be used by any debug command.
 
 Function:	
 		 
 Export:	

 Abbrev:	

 History:
	Data		Who		Remark
	
	06/15/2011	MH_Chen	Create initial version.
						Move WPS relative functio to the files.
	
		
******************************************************************************/
/* Header Files. */
#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "WPS.tmh"
#endif

/*---------------------------Define Local Constant---------------------------*/
/*---------------------------Define Local Constant---------------------------*/


/*------------------------Define global variable-----------------------------*/
/*------------------------Define global variable-----------------------------*/


/*------------------------Define local variable------------------------------*/
/*------------------------Define local variable------------------------------*/


/*--------------------Define export function prototype-----------------------*/
/*--------------------Define export function prototype-----------------------*/

/*---------------------Define local function prototype-----------------------*/
/*---------------------Define local function prototype-----------------------*/
#if (WPS_SUPPORT == 1)

BOOLEAN
wps_IsWPSIEReady(
	IN	PADAPTER	Adapter
	)
{
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(&(GetDefaultAdapter(Adapter)->MgntInfo));
	BOOLEAN				wpsIEReady = FALSE;

	if(pSimpleConfig->ieBeaconLen == 0 || pSimpleConfig->ieAsocReqLen == 0 || pSimpleConfig->ieAsocRspLen == 0
		|| pSimpleConfig->ieProbeReqLen == 0 ||pSimpleConfig->ieProbeRspLen == 0)
	{
		return wpsIEReady;
	}

	wpsIEReady = TRUE;
	return wpsIEReady;
}


VOID
WPS_Init(
	IN	PADAPTER	Adapter
	)
{
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(&(GetDefaultAdapter(Adapter)->MgntInfo));
	
	Adapter->WPSSupport = TRUE;
	pSimpleConfig->WpsIeVersion = SUPPORT_WPS_INFO_VERSION;
}


BOOLEAN
WPS_OnAsocReq(
	IN		PADAPTER	Adapter,
	OUT		BOOLEAN		*pbIndicate
	)
{
//	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PMGNT_INFO 			pDefaultMgntInfo = &(GetDefaultAdapter(Adapter)->MgntInfo);
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(pDefaultMgntInfo);

	if(pSimpleConfig->bEnabled || pSimpleConfig->IELen > 0)
	{// don't check RSNIE whenever WPS is enabled, 20100528, haich.
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Support WPS, don't indicate to OS!\n"));
		*pbIndicate = FALSE;
	}
	else
	{
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("Check RSN-IE if necessary. return FALSE<====\n"));
		return FALSE;
	}
	return	TRUE;
	
}


BOOLEAN
WPS_MgntActSet_802_11_SSID(
	IN		PADAPTER	Adapter
	)
{
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(&(Adapter->MgntInfo));
	
	if( MgntIsLinkInProgress(pMgntInfo) && pSimpleConfig->bEnabled)
	{
		RT_TRACE(COMP_WPS, DBG_LOUD, ("MgntActSet_802_11_SSID(): In WPS process, do not connect to the same AP\n"));
		return TRUE;
	}	

	return	FALSE;
	
}


VOID
WPS_ConstructBeaconFrame(
	IN		PADAPTER	Adapter
	)
{
	//
	// Simple config IE. by CCW - copy from 818x
	//
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	PSIMPLE_CONFIG_T	pSimpleConfig ;
	OCTET_STRING		SimpleConfigInfo;
	PMGNT_INFO 			pDefaultMgntInfo;

	if(ACTING_AS_IBSS(Adapter))
		return;

	if(!ACTING_AS_AP(Adapter))
		return;
	
	pDefaultMgntInfo = &(GetDefaultAdapter(Adapter)->MgntInfo);

	pSimpleConfig = GET_SIMPLE_CONFIG(pDefaultMgntInfo);
	
	if( ((pSimpleConfig->WpsIeVersion < SUPPORT_WPS_INFO_VERSION) || (wps_IsWPSIEReady(Adapter) == FALSE)) && pSimpleConfig->IELen > 0)
	{	// Original method carrying WPS IE
		RT_TRACE(COMP_WPS, DBG_TRACE, ("AP Construct Beacon \n"));
		FillOctetString(SimpleConfigInfo, pSimpleConfig->IEBuf, pSimpleConfig->IELen);
		PacketMakeElement( &pMgntInfo->beaconframe, EID_Vendor, SimpleConfigInfo);
	}
	else if(pSimpleConfig->WpsIeVersion == SUPPORT_WPS_INFO_VERSION)
	{
		FillOctetString(SimpleConfigInfo, pSimpleConfig->ieBeaconBuf, pSimpleConfig->ieBeaconLen);
		if(pSimpleConfig->ieBeaconLen > 0)
			PacketAppendData(&pMgntInfo->beaconframe, SimpleConfigInfo);
	}
		
}


VOID
WPS_AppendElement(
	IN	PADAPTER			Adapter,
	IN	POCTET_STRING		posFrame,
	IN	BOOLEAN				bCheckFrag,
	IN	WPS_INFO_OPCODE	frameType
	)
{
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(&(GetDefaultAdapter(Adapter)->MgntInfo));
	OCTET_STRING		SimpleConfigInfo;	

	FunctionIn(COMP_WPS);
	#if 0
	if(bCheckFrag)
	{
		// WPS 2.0 Support IE Fragment	for Testbed function
		if(pSimpleConfig->bFragmentIE && pSimpleConfig->IELen <= MAX_SIMPLE_CONFIG_IE_LEN)
		{
			u1Byte tempBuf[MAX_SIMPLE_CONFIG_IE_LEN];
			pu1Byte currPtr;
			pu1Byte currPtrAftOui;
			RT_TRACE(COMP_WPS,DBG_LOUD,("ConstructProbeRequest: in Fragment IE\n"));
			PlatformZeroMemory(tempBuf, MAX_SIMPLE_CONFIG_IE_LEN);
			//Copy the OUI
			currPtr = pSimpleConfig->IEBuf;
			//Tesplan to copy the first octet in the first fragment
			PlatformMoveMemory(tempBuf, currPtr, SIZE_OUI + SIZE_OUI_TYPE);
			currPtr += (SIZE_OUI + SIZE_OUI_TYPE); 
			currPtrAftOui = &tempBuf[SIZE_OUI + SIZE_OUI_TYPE];

			// the first octet
			PlatformMoveMemory(currPtrAftOui, currPtr, 1);
			currPtr += 1;
			FillOctetString(SimpleConfigInfo,tempBuf,(SIZE_OUI + SIZE_OUI_TYPE +1));
			PacketMakeElement( posFrame, EID_Vendor, SimpleConfigInfo);
						
			// the rest octet			
			PlatformZeroMemory(currPtrAftOui, 1);
			PlatformMoveMemory(currPtrAftOui, currPtr, (pSimpleConfig->IELen-(SIZE_OUI + SIZE_OUI_TYPE)-1) );

			FillOctetString(SimpleConfigInfo,tempBuf,(pSimpleConfig->IELen-1));
			PacketMakeElement( posFrame, EID_Vendor, SimpleConfigInfo);

		}
		else if(pSimpleConfig->IELen > MAX_SIMPLE_CONFIG_IE_LEN)
		{
			u1Byte tempBuf[MAX_SIMPLE_CONFIG_IE_LEN];
			pu1Byte currPtr;
			pu1Byte currPtrAftOui;
			PlatformZeroMemory(tempBuf, MAX_SIMPLE_CONFIG_IE_LEN);
			//Copy the OUI
			currPtr = pSimpleConfig->IEBuf;
			//Tesplan to copy the first octet in the first fragment
			PlatformMoveMemory(tempBuf, currPtr, SIZE_OUI + SIZE_OUI_TYPE);
			currPtr += (SIZE_OUI + SIZE_OUI_TYPE); 
			currPtrAftOui = &tempBuf[SIZE_OUI + SIZE_OUI_TYPE];

			// the first fragment
			PlatformMoveMemory(currPtrAftOui, currPtr, (MAX_SIMPLE_CONFIG_IE_LEN - (SIZE_OUI + SIZE_OUI_TYPE)));
			currPtr += (MAX_SIMPLE_CONFIG_IE_LEN - (SIZE_OUI + SIZE_OUI_TYPE));
			FillOctetString(SimpleConfigInfo,tempBuf,(MAX_SIMPLE_CONFIG_IE_LEN - (SIZE_OUI + SIZE_OUI_TYPE)));
			PacketMakeElement( posFrame, EID_Vendor, SimpleConfigInfo);
						
			// the rest octet			
			PlatformZeroMemory(currPtrAftOui, (MAX_SIMPLE_CONFIG_IE_LEN - (SIZE_OUI + SIZE_OUI_TYPE)));
			PlatformMoveMemory(currPtrAftOui, currPtr, (pSimpleConfig->IELen-MAX_SIMPLE_CONFIG_IE_LEN) );

			FillOctetString(SimpleConfigInfo,tempBuf,(pSimpleConfig->IELen-MAX_SIMPLE_CONFIG_IE_LEN));
			PacketMakeElement( posFrame, EID_Vendor, SimpleConfigInfo);
			
		}
		else
		{
			FillOctetString(SimpleConfigInfo, pSimpleConfig->IEBuf, pSimpleConfig->IELen);
			PacketMakeElement( posFrame, EID_Vendor, SimpleConfigInfo);
		}
	}
	else
	#endif
	{
		if(((pSimpleConfig->WpsIeVersion < SUPPORT_WPS_INFO_VERSION) || (wps_IsWPSIEReady(Adapter) == FALSE)) && pSimpleConfig->IELen > 0)
		{
			FillOctetString(SimpleConfigInfo, pSimpleConfig->IEBuf, pSimpleConfig->IELen);
			PacketMakeElement( posFrame, EID_Vendor, SimpleConfigInfo);
		}
		else if(pSimpleConfig->WpsIeVersion == SUPPORT_WPS_INFO_VERSION)
		{
			switch(frameType)
			{
				case WPS_INFO_ASOCREQ_IE:
				{
					FillOctetString(SimpleConfigInfo, pSimpleConfig->ieAsocReqBuf, pSimpleConfig->ieAsocReqLen);
					if(pSimpleConfig->ieAsocReqLen > 0)
						PacketAppendData(posFrame, SimpleConfigInfo);
				}
				break;

				case WPS_INFO_ASOCRSP_IE:
				{
					FillOctetString(SimpleConfigInfo, pSimpleConfig->ieAsocRspBuf, pSimpleConfig->ieAsocRspLen);
					if(pSimpleConfig->ieAsocRspLen > 0)
						PacketAppendData(posFrame, SimpleConfigInfo);
				}
				break;
				
				case WPS_INFO_PROBEREQ_IE:
				{
					FillOctetString(SimpleConfigInfo, pSimpleConfig->ieProbeReqBuf, pSimpleConfig->ieProbeReqLen);
					if(pSimpleConfig->ieProbeReqLen > 0)
						PacketAppendData(posFrame, SimpleConfigInfo);
				}
				break;

				case WPS_INFO_PROBERSP_IE:
				{
					FillOctetString(SimpleConfigInfo, pSimpleConfig->ieProbeRspBuf, pSimpleConfig->ieProbeRspLen);
					if(pSimpleConfig->ieProbeRspLen > 0)
						PacketAppendData(posFrame, SimpleConfigInfo);
				}
				break;

				default: //for MacOS warning.
					break;

			}
		}
	}
}



VOID
WPS_CopyRxEAPPacket(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{
	PSIMPLE_CONFIG_T	pSimpleConfig = GET_SIMPLE_CONFIG(&(GetDefaultAdapter(Adapter)->MgntInfo));
	pu1Byte 				RecieveBufPtrt,RfdBufferPtr;
	
	pu1Byte BufferPtr = (pu1Byte)&(pRfd->PacketLength );
	RT_TRACE(COMP_WPS, DBG_LOUD, ("In WPS Copy EAP Packet\n"));
	pRfd->PacketLength -= 18;//18 is the length we don't want
	PlatformFillMemory(pSimpleConfig->RecieveBuf, 1500, 0);
	PlatformMoveMemory(pSimpleConfig->RecieveBuf,BufferPtr, 2);
	RecieveBufPtrt = (pu1Byte)pSimpleConfig->RecieveBuf+2;
	RfdBufferPtr	 =pRfd->Buffer.VirtualAddress+4; //skip Version DATA TYPE fream control
	PlatformMoveMemory(RecieveBufPtrt,RfdBufferPtr, ETHERNET_ADDRESS_LENGTH*2);
	RecieveBufPtrt += (ETHERNET_ADDRESS_LENGTH*2);
	RfdBufferPtr	 += (ETHERNET_ADDRESS_LENGTH*2);
	RfdBufferPtr	 += 14;//Skip LLC Until 888E
	if(pRfd->Buffer.VirtualAddress[0] & 0x80) {//Skip QoS
		RfdBufferPtr	 += 2;
		RT_TRACE(COMP_WPS, DBG_LOUD, ("The Data is %x and QoS is Set\n",RfdBufferPtr[0]));
		pRfd->PacketLength -= 2;//18 is the length we don't want
		PlatformMoveMemory(pSimpleConfig->RecieveBuf,BufferPtr, 2);
		
	}
	PlatformMoveMemory(RecieveBufPtrt,RfdBufferPtr,(pRfd->PacketLength -(ETHERNET_ADDRESS_LENGTH*2)) );
	pSimpleConfig->bRecievePacket = TRUE;
	pSimpleConfig->RecieveLength = (pRfd->PacketLength+2);// 2 is the packet length we reserved for report to lib
	RT_PRINT_DATA(COMP_WPS, DBG_LOUD, "EAP Packet Content:", pSimpleConfig->RecieveBuf, pRfd->PacketLength);
			
}


RT_STATUS
WPS_QueryRxEAPPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength,
	OUT	pu4Byte			BytesWritten,
	OUT	pu4Byte			BytesNeeded
	)
{
	PSIMPLE_CONFIG_T 	pSimpleConfig = GET_SIMPLE_CONFIG(&(Adapter->MgntInfo));
	RT_STATUS			status = RT_STATUS_SUCCESS;
	
	RT_TRACE(COMP_WPS, DBG_LOUD, ("WPS Check Recieve Buffer:\n"));
	if( InformationBufferLength < pSimpleConfig->RecieveLength)
	{
		*BytesNeeded = pSimpleConfig->RecieveLength;
		*BytesWritten = 0;
		return RT_STATUS_INVALID_CONTEXT;
	}
	
	if(!pSimpleConfig->bRecievePacket)
	{
		*BytesNeeded =0;
		//*BytesWritten = 0;
	}
	else
	{
		RT_PRINT_DATA(COMP_WPS, DBG_LOUD, "Driver report Info Buffer before copy:", InformationBuffer, 16);
		InformationBufferLength = pSimpleConfig->RecieveLength;//Set For packet length
		PlatformMoveMemory( InformationBuffer, pSimpleConfig->RecieveBuf, pSimpleConfig->RecieveLength);
		RT_PRINT_DATA(COMP_WPS, DBG_LOUD, "Driver report Info Buffer After copy:", InformationBuffer, 16);
		*BytesWritten = pSimpleConfig->RecieveLength;
		pSimpleConfig->bRecievePacket = FALSE;
		RT_PRINT_DATA(COMP_WPS, DBG_LOUD, "Driver report EAP Packet Content:", pSimpleConfig->RecieveBuf, pSimpleConfig->RecieveLength);
	}
	return status;
}

#else

VOID
WPS_Init(
	IN	PADAPTER	Adapter
	)
{
	Adapter->WPSSupport = FALSE;
}


BOOLEAN
WPS_OnAsocReq(
	IN		PADAPTER	Adapter,
	OUT		BOOLEAN		*pbIndicate
	)
{
	return	TRUE;
}


BOOLEAN
WPS_MgntActSet_802_11_SSID(
	IN		PADAPTER	Adapter
	)
{
	return	FALSE;
}


VOID
WPS_ConstructBeaconFrame(
	IN		PADAPTER	Adapter
	)
{

}

VOID
WPS_AppendElement(
	IN	PADAPTER			Adapter,
	IN	POCTET_STRING		posFrame,
	IN	BOOLEAN				bCheckFrag,
	IN	WPS_INFO_OPCODE	frameType
	)
{

}

VOID
WPS_CopyRxEAPPacket(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	)
{

}

RT_STATUS
WPS_QueryRxEAPPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength,
	OUT	pu4Byte			BytesWritten,
	OUT	pu4Byte			BytesNeeded
	)
{
	return RT_STATUS_SUCCESS;
}


#endif	// WPS_SUPPORT


/* End of WPS.c */

