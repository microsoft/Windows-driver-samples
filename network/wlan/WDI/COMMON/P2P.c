#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P.tmh"
#endif

#include "P2P_Internal.h"

#if (P2P_SUPPORT == 1)

u8Byte	RTL_P2P_AllocateMemory_count ;
u8Byte	RTL_P2P_FreeMemory_count ;
u8Byte	RTL_P2P_AllocateMemory_Len;
u8Byte	RTL_P2P_FreeMemory_Len;

//======================================================================
// Wi-Fi Direct Start
//======================================================================

static u1Byte	BroadcastAddress[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static u1Byte P2PSocialChannels[] = 
{
	1,
	6,
	11,
};

static u1Byte P2PWildcardSsid[] = {'D', 'I', 'R','E', 'C', 'T', '-'};

//======================================================================
// Private Functions (Starting from Win8) (Keyword Static)
//======================================================================


static BOOLEAN
P2PAdapterAcceptActionFrame(
	PADAPTER pAdapter,
	OCTET_STRING osPacket
)
{	
	BOOLEAN 		bReturnValue = FALSE;
	ACT_PKT_TYPE	pktType = PacketGetActionFrameType(&osPacket);

	switch(pktType)
	{
	default:
		break;

	// For Win8, these P2P handshake packets shall transfer to the device port.
	case ACT_PKT_P2P_GO_NEG_REQ:
	case ACT_PKT_P2P_GO_NEG_RSP:
	case ACT_PKT_P2P_GO_NEG_CONF:
	case ACT_PKT_P2P_PROV_DISC_REQ:
	case ACT_PKT_P2P_PROV_DISC_RSP:
	case ACT_PKT_P2P_INVIT_REQ:
	case ACT_PKT_P2P_INVIT_RSP:
		if(GET_P2P_INFO(pAdapter)->Role == P2P_DEVICE) bReturnValue = TRUE; // NOT Ready:
		break;
	}

	return bReturnValue;
}


static BOOLEAN
P2PAdapterAcceptMgntFrame(
	PADAPTER pAdapter,
	OCTET_STRING osPacket
)
{
	// Accept all management frames
	
	BOOLEAN bStatus = FALSE;

	switch(P2P_FRAME_GET_TYPE(osPacket.Octet))
	{
		case Type_Probe_Req:
		case Type_Probe_Rsp:
		case Type_Beacon:
		case Type_Asoc_Req:
		case Type_Reasoc_Req:
		case Type_Asoc_Rsp:
		case Type_Reasoc_Rsp:
		case Type_Deauth:
		case Type_Disasoc:
			bStatus = TRUE;
			break;
	}

	return bStatus;
}

BOOLEAN
P2PAdapterAcceptFrame(
	PADAPTER pAdapter,
	OCTET_STRING osPacket
)
{
	BOOLEAN bStatus = FALSE;

	if(IsMgntAction(osPacket.Octet)) 
	{
		bStatus = P2PAdapterAcceptActionFrame(pAdapter, osPacket);
	}
	else if(IsMgntFrame(osPacket.Octet))
	{
		bStatus = P2PAdapterAcceptMgntFrame(pAdapter, osPacket);
	}

	return bStatus;
}

// The operations of the structure MEMORY_BUFFER
static VOID
P2PMemoryBufferFree(
	PMEMORY_BUFFER pMemoryBuffer
)
{
	if(pMemoryBuffer->Length > 0)
	{
		PlatformFreeMemory(pMemoryBuffer->Buffer, pMemoryBuffer->Length);

		RTL_P2P_FreeMemory_count++;
		RTL_P2P_FreeMemory_Len += pMemoryBuffer->Length;
	}
	
	PlatformZeroMemory(pMemoryBuffer, sizeof(MEMORY_BUFFER));
	pMemoryBuffer->Length = 0;
}

// The operations of the structure MEMORY_BUFFER
static BOOLEAN
P2PMemoryBufferClone(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pDestination,
	PMEMORY_BUFFER pSource
)
{
	BOOLEAN bStatus = FALSE;
	RT_STATUS rtStatus = RT_STATUS_FAILURE;

	if(pDestination->Length > 0)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: With Unloaded Memory!\n", __FUNCTION__));
		bStatus = FALSE;
	}
	else if(pSource->Length > 0 && pSource->Buffer == NULL)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Illegal Source Memory!\n", __FUNCTION__));
		bStatus = FALSE;
	}
	else if(pSource->Length == 0)
	{
		*pDestination = *pSource;
		bStatus = TRUE;
	}
	else
	{
		rtStatus = PlatformAllocateMemory(
				pP2PInfo->pAdapter, 
				&pDestination->Buffer,
				pSource->Length
			);

		if(rtStatus == RT_STATUS_SUCCESS)
		{
			pDestination->Length = pSource->Length;
			
			PlatformMoveMemory(
					pDestination->Buffer, 
					pSource->Buffer, 
					pDestination->Length
				);
			
			bStatus = TRUE;

			RTL_P2P_AllocateMemory_count ++;
			RTL_P2P_AllocateMemory_Len += pSource->Length;
		}
		else 
		{
			PlatformZeroMemory(pDestination, sizeof(MEMORY_BUFFER));
			pDestination->Length = 0;
			bStatus = FALSE;

			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Allocation Failure!\n", __FUNCTION__));
		}
	}

	return bStatus;
}


// The operations of the structure MEMORY_BUFFER
static BOOLEAN
P2PMemoryBufferCopy(
	PP2P_INFO pP2PInfo,
	PMEMORY_BUFFER pDestination,
	PMEMORY_BUFFER pSource
)
{
	BOOLEAN bStatus = FALSE;
	RT_STATUS rtStatus = RT_STATUS_FAILURE;

	if(pDestination->Length > 0 && pSource->Length > 0)
	{
		if(pDestination->Length == pSource->Length)
		{
			if(PlatformCompareMemory(pDestination->Buffer, pSource->Buffer, pSource->Length) != 0)
				CopyMem(pDestination->Buffer, pSource->Buffer, pSource->Length);
			bStatus = TRUE;
		}
	}	

	return bStatus;
}


static VOID
P2PDeviceListDump(
	PP2P_DEVICE_LIST pDeviceList
)
{
	u4Byte i = 0;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("\n"));

	for(i = 0; i < pDeviceList->uNumberOfDevices; i++)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("=======================================================\n"));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("DeviceList: Entry(%u)\n", i));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("=======================================================\n"));
		
		RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "MacAddress: ", pDeviceList->DeviceEntry[i].MacAddress);
		if(pDeviceList->DeviceEntry[i].DeviceAddress != NULL)
		RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "DeviceAddress: ", pDeviceList->DeviceEntry[i].DeviceAddress);
		RT_TRACE(COMP_P2P, DBG_LOUD, ("ChannelNumber: %d\n", pDeviceList->DeviceEntry[i].ChannelNumber));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("RecvSignalPower: %d\n", pDeviceList->DeviceEntry[i].RecvSignalPower));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("SignalStrength: %d\n", pDeviceList->DeviceEntry[i].SignalStrength));

		RT_TRACE(COMP_P2P, DBG_LOUD, ("ProbeResponseHostTimestamp: 0x%08X%08X\n", 
				(u4Byte)(pDeviceList->DeviceEntry[i].ProbeResponseHostTimestamp >> 32), 
				(u4Byte) pDeviceList->DeviceEntry[i].ProbeResponseHostTimestamp)
			);

		RT_TRACE(COMP_P2P, DBG_LOUD, ("BeaconHostTimestamp: 0x%08X%08X\n", 
				(u4Byte)(pDeviceList->DeviceEntry[i].BeaconHostTimestamp >> 32), 
				(u4Byte) pDeviceList->DeviceEntry[i].BeaconHostTimestamp)
			);
	}
	RT_TRACE(COMP_P2P, DBG_LOUD, ("=======================================================\n"));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Totally %d items in Device List dumped\n", i));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("=======================================================\n"));

	RT_TRACE(COMP_P2P, DBG_LOUD, ("\n"));
}

// Indicate Fake Invitation rsp frame to OS and make the state machine in OS go to next state.
//
BOOLEAN
P2PIndicateFakeInvitRsp(
	IN PP2P_INFO pP2PInfo
	)
{
	PADAPTER				pAdapter = pP2PInfo->pAdapter;
	PRT_GEN_TEMP_BUFFER 	pGenBuf = NULL;
	PP2P_DEVICE_LIST_ENTRY	pListEntry = NULL;
	OCTET_STRING			osWFDIE, osProbeRspPkt;
	FRAME_BUF				fbuf;

	pListEntry = P2PDeviceListFind(&pP2PInfo->DeviceList, pP2PInfo->InvitationContext.InvitedDevice.DeviceAddress);
	if(pListEntry == NULL)
	{
		return FALSE;
	}
	
	if(NULL == (pGenBuf = GetGenTempBuffer(pAdapter, GEN_TEMP_BUFFER_SIZE)))
	{
		RT_TRACE_F(COMP_WFD, DBG_SERIOUS, ("[ERROR] Memory allocation failed!\n"));
		return FALSE;
	}

	FrameBuf_Init(GEN_TEMP_BUFFER_SIZE, 0, (pu1Byte)pGenBuf->Buffer.Ptr, &fbuf);

	// MAC Header
	p2p_add_ActionFrameMacHdr(&fbuf, pP2PInfo->DeviceAddress, pP2PInfo->InvitationContext.InvitedDevice.DeviceAddress, pP2PInfo->DeviceAddress);

	// Action Header
	p2p_add_P2PPublicActionHdr(&fbuf, P2P_INVITATION_RSP, pP2PInfo->InvitationContext.DialogToken);

	// P2P IE
	p2p_build_FakeInvitationRspIe(&fbuf, pP2PInfo, pP2PInfo->InvitationContext.InvitedDevice.DeviceAddress);

	FillOctetString(osProbeRspPkt, pListEntry->ProbeResponsePacket.Buffer, (u2Byte)pListEntry->ProbeResponsePacket.Length);

	osWFDIE = PacketGetElement(osProbeRspPkt, EID_Vendor, OUI_SUB_WIFI_DISPLAY, OUI_SUB_DONT_CARE);

	if(osWFDIE.Length > 0)
	{
		PacketMakeElement(&fbuf.os, EID_Vendor, osWFDIE);
	}

	p2p_IndicateActionFrameReceived(pP2PInfo, 
		P2P_EVENT_RECEIVED_INVITATION_RESPONSE, 
		RT_STATUS_SUCCESS, 
		fbuf.os.Octet, 
		fbuf.os.Length);

	ReturnGenTempBuffer(pAdapter, pGenBuf);
	return TRUE;
}


static VOID
P2PDeviceListClear(
	PP2P_DEVICE_LIST pDeviceList
)
{
	u4Byte i = 0;

	for(i = 0; i < pDeviceList->uNumberOfDevices; i++)
	{
		P2PMemoryBufferFree(&pDeviceList->DeviceEntry[i].ProbeResponsePacket);
		P2PMemoryBufferFree(&pDeviceList->DeviceEntry[i].BeaconPacket);
	}

	PlatformZeroMemory(pDeviceList, sizeof(P2P_DEVICE_LIST));
}

PP2P_DEVICE_LIST_ENTRY
P2PDeviceListFind(
	IN PP2P_DEVICE_LIST pDeviceList,
	IN pu1Byte pMacAddress
)
{
	u4Byte i = 0;
		
	for(i = 0; i < pDeviceList->uNumberOfDevices; i++)
	{
		if(eqMacAddr(pDeviceList->DeviceEntry[i].MacAddress, pMacAddress))
		{
			return &pDeviceList->DeviceEntry[i];
		}
	}
	
	return NULL;
}

static BOOLEAN
P2PDeviceListInsertTail(
	IN PP2P_INFO pP2PInfo,
	OUT PP2P_DEVICE_LIST pDeviceList,
	IN PRT_RFD pRfd,
	IN pu1Byte pMacAddress,
	IN pu1Byte pDeviceAddress,
	IN MEMORY_BUFFER mbPacket
)
{
//	PMGNT_INFO pMgntInfo = &pP2PInfo->pAdapter->MgntInfo;
	BOOLEAN bStatus = FALSE;
	u1Byte index = pDeviceList->uNumberOfDevices;
	pu1Byte header = (pu1Byte) mbPacket.Buffer;
		
	if(index < P2P_MAX_DEVICE_LIST)
	{
		cpMacAddr(pDeviceList->DeviceEntry[index].MacAddress, pMacAddress);
	
		if(pDeviceAddress) cpMacAddr(pDeviceList->DeviceEntry[index].DeviceAddress, pDeviceAddress);
		
		pDeviceList->DeviceEntry[index].ChannelNumber= P2PGetChannel(pP2PInfo);
		
		pDeviceList->DeviceEntry[index].RecvSignalPower = pRfd->Status.RecvSignalPower;
		pDeviceList->DeviceEntry[index].SignalStrength= pRfd->Status.SignalStrength;


		if(IsMgntProbeRsp(header))
		{
			pDeviceList->DeviceEntry[index].ProbeResponseHostTimestamp = PlatformGetCurrentTime();
			bStatus = P2PMemoryBufferClone(pP2PInfo, &pDeviceList->DeviceEntry[index].ProbeResponsePacket, &mbPacket);
		}
		else if(IsMgntBeacon(header))
		{
			pDeviceList->DeviceEntry[index].BeaconHostTimestamp = PlatformGetCurrentTime();
			bStatus = P2PMemoryBufferClone(pP2PInfo, &pDeviceList->DeviceEntry[index].BeaconPacket, &mbPacket);
		}
		else
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Wrong Mgnt Packet Type \n", __FUNCTION__));
			return FALSE;
		}
		
		if(bStatus)
		{
			pDeviceList->uNumberOfDevices++;
		}
		else 
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Clone Failure!\n", __FUNCTION__));
		}
	}
	else 
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Size Limit Reached!\n", __FUNCTION__));
	}

	return bStatus;
}

static BOOLEAN 
P2PGetWpsSelectedRegistrar(
	OCTET_STRING osWpsIE
)
{
	u1Byte	WPS_OUI[] = {0x00, 0x50, 0xF2, 0x04};
	u4Byte 	offset = 0;
	u1Byte 	SelectedRegistrarID[] = {0x10, 0x41};
	
	if(!eqNByte(osWpsIE.Octet, WPS_OUI, sizeof(WPS_OUI)))
	{
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Not a valid good WPS IE!\n", osWpsIE.Octet, osWpsIE.Length);
		return FALSE;
	}

	offset += sizeof(WPS_OUI);

	while(offset + 4 < osWpsIE.Length)
	{
		if(eqNByte(osWpsIE.Octet + offset, SelectedRegistrarID, sizeof(SelectedRegistrarID)))
		{
			// Length 2 Bytes and ID 2 Bytes
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Found: WpsSelectedRegistrar: %d\n", __FUNCTION__, osWpsIE.Octet[offset + 4]));
			return (BOOLEAN)(osWpsIE.Octet[offset + 4]);
		}
		
		// Length 2 Bytes and ID 2 Bytes
		offset += 4 + P2P_WPS_ATTR_READ_EF_2_BYTE(osWpsIE.Octet + offset + 2);
	}

	return FALSE;
}

static VOID
P2PDeviceListUpdate (
	IN PP2P_INFO pP2PInfo,
	OUT PP2P_DEVICE_LIST pDeviceList,
	IN PRT_RFD pRfd, 
	IN pu1Byte pMacAddress,
	IN pu1Byte pDeviceAddress,
	IN MEMORY_BUFFER mbPacket
)
{
	PP2P_DEVICE_LIST_ENTRY	pListEntry = NULL;
	BOOLEAN	 bStatus = FALSE;
	MEMORY_BUFFER mbObject = {NULL, 0};
	pu1Byte header = (pu1Byte) mbPacket.Buffer;

	
#if 0	// Debug MAC Address Filter
{
	#define MAX_MAX_FILTER 10
	
	u1Byte Macfilter[MAX_MAX_FILTER][6] = {
			{0x00, 0xe0, 0x4c, 0x78, 0x01, 0x1c},
			{0x00, 0xe0, 0x4c, 0x78, 0x00, 0x02},
			{0x00, 0xe0, 0x4c, 0x00, 0xf0, 0x2b},
			{0x00, 0xe0, 0x4c, 0x00, 0x50, 0x2b}
		};

	BOOLEAN update = TRUE;
	u1Byte i = 0;
	
	for(i = 0; i < MAX_MAX_FILTER; i++)
	{
		if(eqMacAddr(pMacAddress, Macfilter[i]))
			update = FALSE;
	}

	if(update == FALSE) return;
}
#endif


	// For the OID_DOT11_WFD_CONNECT_TO_GROUP_REQUEST and OID_DOT11_WFD_GROUP_START_PARAMETERS -------------------------
	if(P2P_CLIETN_JOIN_GROUP_WPS_STATE_SCANNING == pP2PInfo->ClientJoinGroupContext.WpsState)
	{
		OCTET_STRING osPacket = {mbPacket.Buffer, (u2Byte) mbPacket.Length};
		OCTET_STRING osSsid = {NULL, 0};
		OCTET_STRING osWpsIE = {NULL, 0};
		
		osSsid = PacketGetElement(osPacket, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);

		if(CompareSSID(pP2PInfo->GroupTargetSSID, (u2Byte) pP2PInfo->uGroupTargetSSIDLength, osSsid.Octet, osSsid.Length))
		{
			osWpsIE = PacketGetElement(osPacket, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUB_DONT_CARE);

			if(osWpsIE.Length != 0 && P2PGetWpsSelectedRegistrar(osWpsIE) == TRUE)
			{
				RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Target GO WPS IE:\n", osWpsIE.Octet, osWpsIE.Length);
				// Mark the WPS of the target is ready to join
				pP2PInfo->ClientJoinGroupContext.WpsState = P2P_CLIENT_JOIN_GROUP_WPS_STATE_GO_READY;
				P2PScanListCeaseScan(pP2PInfo);
			}
			else
			{
				RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Target GO WPS IE: No Found! Keep waiting!\n", osWpsIE.Octet, osWpsIE.Length);
			}
		}
	}
	// --------------------------------------------------------------------------------------------------------------------


	pListEntry = P2PDeviceListFind(pDeviceList, pMacAddress);

	if(pListEntry == NULL)
	{
		bStatus = P2PDeviceListInsertTail(pP2PInfo, pDeviceList, pRfd, pMacAddress, pDeviceAddress, mbPacket);
		
		if(bStatus == FALSE)
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Insert Tail Failure!\n", __FUNCTION__));
		}
	}
	else
	{
		// Fill the list entry structure
		if(pDeviceAddress) cpMacAddr(pListEntry->DeviceAddress, pDeviceAddress);
		

		{// Query for the Correct Channel Information
			OCTET_STRING osPacket = {mbPacket.Buffer, (u2Byte) mbPacket.Length};
			OCTET_STRING DsPmBeacon = {NULL, 0};

			DsPmBeacon = PacketGetElement(osPacket, EID_DSParms, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
			if(DsPmBeacon.Length != 0)
			{
				pListEntry->ChannelNumber= EF1Byte( *(u1Byte *)(DsPmBeacon.Octet) );
			}
			else
			{
				pListEntry->ChannelNumber= RT_GetChannelNumber(pP2PInfo->pAdapter);
			}
		}

		
		pListEntry->RecvSignalPower = pRfd->Status.RecvSignalPower;
		pListEntry->SignalStrength= pRfd->Status.SignalStrength;

		if(IsMgntProbeRsp(header))
		{
			pListEntry->ProbeResponseHostTimestamp = PlatformGetCurrentTime();

			bStatus = P2PMemoryBufferCopy(pP2PInfo, &pListEntry->ProbeResponsePacket, &mbPacket);

			if(!bStatus)
			{
				bStatus = P2PMemoryBufferClone(pP2PInfo, &mbObject, &mbPacket);

				if(bStatus == TRUE)
				{
					P2PMemoryBufferFree(&pListEntry->ProbeResponsePacket);
					pListEntry->ProbeResponsePacket = mbObject;
				}
				else 
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Clone Failure!\n", __FUNCTION__));
				}
			}
		}
		else if(IsMgntBeacon(header))
		{
			pListEntry->BeaconHostTimestamp = PlatformGetCurrentTime();

			bStatus = P2PMemoryBufferCopy(pP2PInfo, &pListEntry->BeaconPacket, &mbPacket);

			if(!bStatus)
			{
				bStatus = P2PMemoryBufferClone(pP2PInfo, &mbObject, &mbPacket);

				if(bStatus == TRUE)
				{
					P2PMemoryBufferFree(&pListEntry->BeaconPacket);
					pListEntry->BeaconPacket= mbObject;
				}
				else 
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Memory Clone Failure!\n", __FUNCTION__));
				}
			}
		}
		else
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Wrong Mgnt Packet Type \n", __FUNCTION__));
		}
	}
}

static VOID
P2PDeviceListCopyWithTimeStamp(
	IN PP2P_INFO pP2PInfo,
	OUT PP2P_DEVICE_LIST pDestination,
	IN PP2P_DEVICE_LIST pSource
)
{
	u4Byte i = 0;	//source 
	u4Byte j = 0;	//destination

	u4Byte usTimeoutLimit = 3 * 60 * 1000 * 1000;	// 3 minutes
		
	BOOLEAN bBeaconStatus = FALSE;
	BOOLEAN bProbeResponseStatus = FALSE;
	
	MEMORY_BUFFER mbBeacon = {NULL, 0};
	MEMORY_BUFFER mbProbeResponse = {NULL, 0};
	
	u8Byte usCurrentTime = PlatformGetCurrentTime();
	u8Byte usMaxHostTimestamp = 0;
		
	P2PDeviceListClear(pDestination);

	pDestination->uNumberOfDevices = 0;

	for(i = 0; i < pSource->uNumberOfDevices; i++)
	{	

		// Remove the stale device item ----------------------------------------------------------
		//	+ Find MAX
		usMaxHostTimestamp = MAX(
				pSource->DeviceEntry[i].BeaconHostTimestamp, 
				pSource->DeviceEntry[i].ProbeResponseHostTimestamp
			);

		//	+ Check the timestamp
		if(usCurrentTime - usMaxHostTimestamp > usTimeoutLimit) 
		{
			//RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: usCurrentTime: %lld\n", __FUNCTION__, usCurrentTime));
			//RT_TRACE(COMP_P2P, DBG_LOUD, ("usMaxHostTimestamp: %lld\n", usMaxHostTimestamp));
			//RT_TRACE(COMP_P2P, DBG_LOUD, ("usCurrentTime - usMaxHostTimestamp: %lld, usTimeoutLimit: %d\n", 
			//		usCurrentTime - usMaxHostTimestamp, usTimeoutLimit)
			//	);
			
			continue;
		}
		// -----------------------------------------------------------------------------------

		bBeaconStatus = FALSE;
		bProbeResponseStatus = FALSE;

		pDestination->DeviceEntry[j] = pSource->DeviceEntry[i];

		// Beacon Packet
		PlatformZeroMemory(
				&pDestination->DeviceEntry[j].BeaconPacket, 
				sizeof(MEMORY_BUFFER)
			);

		PlatformZeroMemory(&mbBeacon , sizeof(MEMORY_BUFFER));
		bBeaconStatus = P2PMemoryBufferClone(pP2PInfo, &mbBeacon, &pSource->DeviceEntry[i].BeaconPacket);


		// Probe Response Packet
		PlatformZeroMemory(
				&pDestination->DeviceEntry[j].ProbeResponsePacket, 
				sizeof(MEMORY_BUFFER)
			);

		PlatformZeroMemory(&mbProbeResponse , sizeof(MEMORY_BUFFER));
		bProbeResponseStatus = P2PMemoryBufferClone(pP2PInfo, &mbProbeResponse, &pSource->DeviceEntry[i].ProbeResponsePacket);
			

		// Successful
		if(bBeaconStatus && bProbeResponseStatus)
		{
			pDestination->DeviceEntry[j].BeaconPacket = mbBeacon;
			pDestination->DeviceEntry[j].ProbeResponsePacket= mbProbeResponse;

			j++;
			pDestination->uNumberOfDevices = (u1Byte) j;
		}
		else
		{
			// Error Recovery
			if(bBeaconStatus == TRUE)
			{
				P2PMemoryBufferFree(&mbBeacon);
				RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Beacon Memory Clone Failure!\n", __FUNCTION__));
			}
			if(bProbeResponseStatus == TRUE)
			{
				P2PMemoryBufferFree(&mbProbeResponse);
				RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Probe Response Memory Clone Failure!\n", __FUNCTION__));
			}
		}
	}
}

//======================================================================
// P2P Utility
//======================================================================
PADAPTER
P2PGetGoAdapter(
	IN PP2P_INFO pP2PInfo
	)
{
	PADAPTER pAdapter = pP2PInfo->pAdapter;

	RT_AP_TYPE DefApType = MgntActQuery_ApType(GetDefaultAdapter(pAdapter));
	RT_AP_TYPE ExtApType = MgntActQuery_ApType(GetFirstExtAdapter(pAdapter));

	if(P2P_GO != pP2PInfo->Role) return pAdapter;

	//RT_ASSERT(!(0 < DefApType && 0 < ExtApType), ("Both Default port and ext port are acting as AP\n"));
	//RT_ASSERT(!(0 == DefApType && 0 == ExtApType), ("Both Default port and ext port are NOT acting as AP but role is GO\n"));

	return (0 < DefApType) ? (GetDefaultAdapter(pAdapter)) : (GetFirstExtAdapter(pAdapter));
}

BOOLEAN
P2PIsN24GSupported(
	IN PP2P_INFO pP2PInfo
	)
{
	PADAPTER pDefAdapter = pP2PInfo->pAdapter;
	u2Byte SupportedWirelessMode = pDefAdapter->HalFunc.GetSupportedWirelessModeHandler(pDefAdapter);
	
	//RT_TRACE(COMP_P2P, DBG_LOUD, 
	//		("P2PIsN24GSupported(): SupportedWirelessMode: %u\n", 
	//		SupportedWirelessMode));
	
	if(IS_24G_WIRELESS_MODE(SupportedWirelessMode))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

VOID
P2PGOSetBeaconInterval(
	IN PP2P_INFO pP2PInfo,
	u2Byte u2BeaconPeriod
	)
{
	// Note that if under Win7, ext port is used and if not,
	// ADJUST_TO_ADAPTIVE_ADAPTER(pAdapter, FALSE) 
	// always return the only one adapter.
	PADAPTER pAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? pP2PInfo->pAdapter : GetFirstGOPort(pP2PInfo->pAdapter);

	PMGNT_INFO pMgntInfo = &(pAdapter->MgntInfo);

	//
	// Note that if beacon interval is changed, 
	// it is not recovered by only re-enabled P2P mode.
	//
	if(u2BeaconPeriod != pMgntInfo->dot11BeaconPeriod)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("P2PGOSetBeaconInterval(): set beacon interval from (%u) to (%u)\n", 
			pMgntInfo->dot11BeaconPeriod,
			u2BeaconPeriod));
		
		pMgntInfo->Regdot11BeaconPeriod = u2BeaconPeriod;
		pMgntInfo->dot11BeaconPeriod = pMgntInfo->Regdot11BeaconPeriod;
		
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, 
			HW_VAR_BEACON_INTERVAL, 
			(pu1Byte)(&pMgntInfo->dot11BeaconPeriod));
	}
}

BOOLEAN
P2PProvisioning(
	IN PP2P_INFO pP2PInfo
	)
{
#if (WPS_SUPPORT == 1)
	PADAPTER pDefAdapter = GetDefaultAdapter(pP2PInfo->pAdapter);	

	return GET_SIMPLE_CONFIG_ENABLED(&pDefAdapter->MgntInfo);
#endif
	return	FALSE;
}

BOOLEAN
P2PDefaultPortConnected(
	IN PP2P_INFO pP2PInfo
)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pP2PInfo->pAdapter);
	return (MgntLinkStatusQuery(pDefaultAdapter) == RT_MEDIA_CONNECT) ? (TRUE) : (FALSE);
}

BOOLEAN
P2PIsSocialChannel(
	IN u1Byte Channel
	)
{
	u1Byte i;
	
	for(i = 0; i < sizeof(P2PSocialChannels); i++)
	{
		if(P2PSocialChannels[i] == Channel)
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN
P2PTestU2SingleBitSet(
	IN u2Byte u2Val
	)
{
	u2Byte i = sizeof(u2Byte) * 8;
	u2Byte j;
	BOOLEAN bHasBitSet = 0;
	
	for(j = 0; j < i; j++)
	{
		if(u2Val & 1)
		{
			if(bHasBitSet)
			{
				return FALSE;
			}
			else
			{
				bHasBitSet = 1;
			}
		}
		u2Val = u2Val >> 1;
	}
	return TRUE;

}

u1Byte
P2PGetChannel(
	IN PP2P_INFO pP2PInfo
	)
{
	return RT_GetChannelNumber(pP2PInfo->pAdapter);
}

VOID
P2PSetChannel(
	IN PP2P_INFO pP2PInfo,
	IN u1Byte Channel
	)
{
	PADAPTER pAdapter = pP2PInfo->pAdapter;
	pAdapter->HalFunc.SwChnlByTimerHandler(pAdapter, Channel);	
}

VOID
P2PSetRole(
	IN PP2P_INFO pP2PInfo,
	IN P2P_ROLE Role
	)
{
	pP2PInfo->Role = Role;
	P2PIndicateCurrentRole(pP2PInfo, Role);
}

VOID
P2PSetOperatingState(
	IN PP2P_INFO pP2PInfo
	)
{
	if(!P2P_ENABLED(pP2PInfo))
	{
		return;
	}

	if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
	{// Doing P2P Device Discovery
		P2PScanListCeaseScan(pP2PInfo);
		P2PDeviceDiscoveryComplete(pP2PInfo, FALSE); // P2P State is restored in this function.
	}

	pP2PInfo->State = P2P_STATE_OPERATING;
	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PSetOperatingState()\n"));
}


VOID
P2PStopResumeGOBeaconning(
	IN PP2P_INFO pP2PInfo,
	IN BOOLEAN bToStartBeaconning
	)
{
	//
	// Sometimes beacon cant' resume immediately,
	// so we don't do beacon stop/resume now or WPS 
	// is hare to get succeed.
	//
	return; 
}
VOID
P2PGetRandomSeries(
	IN u1Byte MinIndex,
	IN u1Byte MaxIndex,
	IN u1Byte nRandomSeriesToGen,
	OUT pu1Byte SeriesBuf
	)
{
	u1Byte Hashed[20];
	u1Byte iRdm, interval;
	u1Byte i;

	// Input check
	interval = MaxIndex - MinIndex;
	if( interval <= 0 )
	{
		RT_ASSERT( FALSE, ("P2PGetRandomSeries(): Invalid Index!\n") );
		return;
	}

	if(nRandomSeriesToGen > 20) 
	{
		RT_ASSERT( FALSE, ("P2PGetRandomSeries(): nRandomSeriesToGen shall be less than 20!\n") );
		return;
	}

	GetRandomBuffer(Hashed);

	for(i = 0; i < nRandomSeriesToGen; i++) 
	{
		iRdm = *( (pu1Byte)Hashed + i);		// usd first 1 bytes only
		iRdm = iRdm % interval;			// Now iRdm is in [0, interval)
		iRdm += MinIndex;				// Now iRdm is in [MinIndex, MaxIndex)
		SeriesBuf[i] = iRdm;
	}

	return;
}

BOOLEAN
P2PGetRandomChars(
	IN u1Byte nRandomCharToGen,
	OUT char* SeriesBuf
	)
{
	//
	// Get random num from upper case letters, lower case letters and numbers.
	// Total possibility: 26+26+10 = 62 => index from 0 to 61
	//
	
	//int i = GetRandomNumber(0, 26 + 26 + 10);
	u1Byte u1RandomArray[20];
	u1Byte i, j;

	if(nRandomCharToGen > 20) 
	{
		return FALSE;
	}
	
	//
	// i to random char mapping:
	// i = 0 to 9 => number
	// i = 10 to 35 => lower case letter
	// i = 36 to 61 => upper case letter
	//
	P2PGetRandomSeries(0, 26 + 26 + 10, nRandomCharToGen, u1RandomArray);
	for(j = 0; j < nRandomCharToGen; j++)
	{	
		char temp;
		
		i = u1RandomArray[j];
		
		if(i >= 0 && i <= 9)
		{// number
			temp = '0' + (i);
		}
		else if(i >= 10 && i <= 35)
		{// lower case
			temp = 'a' + (i - 10);
		}
		else if(i >= 36 && i <= 61)
		{// upper case
			temp = 'A' + (i - 36);
		}
		else 
		{
			return FALSE;
		}

		SeriesBuf[j] = temp;
	}

	return TRUE;
}

BOOLEAN
P2PDetermineGOSsid(
	IN PP2P_INFO pP2PInfo,
	OUT pu1Byte SsidBuf,
	OUT pu1Byte pSsidLen
	)
{
	//
	// Clause 3.2.1:
	// Each SSID shall begin with the ASCII characters "TBD-", 
	// where TBD is a short version of the technology name to 
	// be determined by the Marketing TG. This SSID requirement 
	// may enable users of Legacy Clients to differentiate between 
	// a P2P Group and an infrastructure network. 
	// 
	// Following "TBD-" the SSID shall contain two ASCII characters 
	// randomly selected with a unified distribution from the following character set: 
	// upper case letters, lower case letters and numbers. 
	//
	// This SSID requirement makes the probability low that a Legacy Client 
	// encounters two P2P Groups with the same SSID and mistakenly 
	// attempt to roam between them.
	//
	static u1Byte P2PTechName[] = {'D', 'I', 'R','E', 'C', 'T', '-'};
	//static u1Byte P2PTechName[] = {'P', '2', 'P', '-'};
	char RandomChar[2];
	u1Byte index = 0;

	*pSsidLen = 0;

	if(RT_STATUS_SUCCESS == P2PSvc_DetermineGOSSID(pP2PInfo->pAdapter, SsidBuf, pSsidLen))
	{
		return TRUE;
	}

	if(pP2PInfo->regSSIDLen > 0)
	{
		// Note:
		//	This SSID may not include the prefix "DIRECT-" defined in P2P spec, but we do not
		//	limit and let our driver test by other SSIDs.
		PlatformMoveMemory(SsidBuf, pP2PInfo->regSSIDBuf, pP2PInfo->regSSIDLen);
		*pSsidLen = pP2PInfo->regSSIDLen;
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2PDetermineGOSsid(): Using Reg SSID = \n", SsidBuf, *pSsidLen)
		return TRUE;
	}

	P2PGetRandomChars(2, RandomChar);

	//
	// Copy P2PTechName and the 2 random series
	//
	*pSsidLen += (sizeof(P2PTechName) + sizeof(RandomChar));

	PlatformMoveMemory(SsidBuf + index, P2PTechName, sizeof(P2PTechName));
	index += sizeof(P2PTechName);

	PlatformMoveMemory(SsidBuf + index, RandomChar, sizeof(RandomChar));
	index += sizeof(RandomChar);

	//
	// Append SSID Postfix
	//
	*pSsidLen += (pP2PInfo->SSIDPostfixLen);
	PlatformMoveMemory(SsidBuf + index, pP2PInfo->SSIDPostfixBuf, pP2PInfo->SSIDPostfixLen);
	index += pP2PInfo->SSIDPostfixLen;

	RT_PRINT_STR(COMP_P2P, DBG_LOUD, "P2PDetermineGOSsid(): Random SSID:\n", SsidBuf, *pSsidLen);
	
	return TRUE;
	
}

BOOLEAN
P2PIsWildcardSsid(
	IN OCTET_STRING osSsid
	)
{
	u1Byte i;
	if(osSsid.Length != sizeof(P2PWildcardSsid)) 
	{
		return FALSE;
	}
	for(i = 0; i < sizeof(P2PWildcardSsid); i++)
	{
		if(osSsid.Octet[i] != P2PWildcardSsid[i]) 
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOLEAN
P2PIsChnlInChnlList(
	IN pu1Byte pChnlList,
	IN u1Byte ChnlListSize,
	IN u1Byte Chnl
	)
{
	u1Byte i;

	for(i = 0; i < ChnlListSize; i++)
	{
		if(pChnlList[i] == Chnl)
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN
P2PIsChnlInChnlEntryList(
	IN P2P_CHANNELS *pChannelEntryList,
	IN u1Byte Chnl
	)
{
	u1Byte i = 0, j = 0;
	
	for(i = 0; i < pChannelEntryList->regClasses; i++)
	{
		if(pChannelEntryList->regClass[i].channels> 0)
		{
			for(j = 0; j < pChannelEntryList->regClass[i].channels; j++)
			{
				if(Chnl == pChannelEntryList->regClass[i].channel[j])
				{
					return TRUE;
				}
			}
		}
	}
	
	return FALSE;
}

BOOLEAN
P2PIsDoingGroupFormation(
	IN PP2P_INFO pP2PInfo
	)
{
	RT_TRACE(COMP_P2P, DBG_LOUD, 
		("P2PIsDoingGroupFormation(): state: %u, bPreGroupFormation: %u\n",
		pP2PInfo->State, pP2PInfo->bPreGroupFormation));
	
	if((pP2PInfo->State >= P2P_STATE_GO_NEGO_REQ_SEND &&
		pP2PInfo->State <= P2P_STATE_GO_NEGO_COMPLETE) ||
		pP2PInfo->bPreGroupFormation)
	{
		return TRUE;
	}
	return FALSE;
}

OCTET_STRING 
P2PWpsIEGetAttribute(
	IN OCTET_STRING osWpsIEAttributes,
	IN BOOLEAN bWpsAttributesInBE,
	IN u2Byte Tag	// note that this is BE
	)
{
	//
	// Note that this function accepts osWpsIEAttributes in BE only.
	//
	u2Byte			offset = 0;
	u2Byte			length = osWpsIEAttributes.Length;
	OCTET_STRING	ret={0, 0};	// used for return
	u2Byte			CurrentTag;
	u2Byte			CurrentLen;
	BOOLEAN 		bTagMatched = FALSE;

	//
	// Assumed format:
	// 		[Tag (2) + Length (2) + Value (n)]*
	// Return:
	//		OCTET_STRING: {pValue, Length}
	// Example: get Device Name
	// 		P2PWpsIEGetAttribute(osWpsAttributes, 0x1011)
	// 		Will return a pointer to "Realtek xxx" and the length of the name
	//

	do
	{
		if((offset + sizeof(CurrentTag) + sizeof(CurrentLen)) > length)
		{// can't read Tag and Length
			return ret;
		}

		if(bWpsAttributesInBE)
		{// BE
			CurrentTag = N2H2BYTE(*((pu2Byte)(osWpsIEAttributes.Octet + offset)));
			offset += sizeof(CurrentTag);

			CurrentLen = N2H2BYTE(*((pu2Byte)(osWpsIEAttributes.Octet + offset)));
			offset += sizeof(CurrentLen);
		}
		else
		{// LE
			CurrentTag = ReadEF2Byte((pu2Byte)(osWpsIEAttributes.Octet + offset));
			offset += sizeof(CurrentTag);

			CurrentLen = ReadEF2Byte((pu2Byte)(osWpsIEAttributes.Octet + offset));
			offset += sizeof(CurrentLen);
		}
		
		if(CurrentTag == Tag)
		{
			bTagMatched = TRUE;
		}

		if(bTagMatched && (length >= offset + CurrentLen))
		{ 
			FillOctetString(ret, osWpsIEAttributes.Octet + offset, CurrentLen);
			break;
		}
		else
		{
			offset += CurrentLen;
		}
	}while(1);

	return ret;
}

BOOLEAN
P2PDeviceTypeMatches(
	IN PP2P_INFO pP2PInfo,
	IN OCTET_STRING osWpsIE
	)
{
	OCTET_STRING ReqType;
	OCTET_STRING osWpsAttributes;
	u1Byte nRequestedDeviceTypeChecked = 0;

	if(osWpsIE.Octet && osWpsIE.Length > 4)
	{
		FillOctetString(osWpsAttributes, osWpsIE.Octet + 4, osWpsIE.Length - 4); // offset the OUI (4)
	}
	else
	{// no Requested Device Type to check
		return TRUE;
	}
		
	do
	{
#if P2P_WPS_ATTR_TRANSMITTED_USING_BE
		ReqType = P2PWpsIEGetAttribute(osWpsAttributes, TRUE, P2P_WP2_ATTR_TAG_REQUESTED_DEVICE_TYPE);
#else
		ReqType = P2PWpsIEGetAttribute(osWpsAttributes, FALSE, P2P_WP2_ATTR_TAG_REQUESTED_DEVICE_TYPE);
#endif
		if(ReqType.Length == 2 + 4 + 2) // cat id(2) + oui(4) + sub cat id(2) = 8
		{
			u2Byte CatId = P2P_WPS_ATTR_READ_EF_2_BYTE((pu2Byte)(ReqType.Octet));
			u2Byte SubCatId = P2P_WPS_ATTR_READ_EF_2_BYTE((pu2Byte)(ReqType.Octet + 2 + 4));

			nRequestedDeviceTypeChecked++;

			//
			// Check primary device type
			//
			if(CatId == pP2PInfo->WpsAttributes.PrimaryDeviceType.CategoryId &&
				SubCatId == pP2PInfo->WpsAttributes.PrimaryDeviceType.SubCategoryId)
			{
				return TRUE;
			}
			else
			{
				//
				// Check secondary device types
				//
				u1Byte i;
				for(i = 0; i < pP2PInfo->WpsAttributes.SecondaryDeviceTypeLength; i++)
				{
					if(CatId == pP2PInfo->WpsAttributes.SecondaryDeviceTypeList[i].CategoryId &&
					SubCatId == pP2PInfo->WpsAttributes.SecondaryDeviceTypeList[i].SubCategoryId)
					{
						return TRUE;
					}
				}
			}
		}
	}while(FALSE);//while(ReqType.Length > 0);

	if(nRequestedDeviceTypeChecked > 0)
	{// not matched
		return FALSE;
	}
	else
	{// no Requested Device Type to check
		return TRUE;
	}
	
}

BOOLEAN
P2PDeviceIDMatches(
	IN PP2P_INFO pP2PInfo,
	IN const P2P_MESSAGE *msg
	)
{
	if(msg->_devId)
	{
		if(eqMacAddr(msg->devIdDevAddr, pP2PInfo->DeviceAddress))
		{
			return TRUE;
		}
		else
		{
			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, 
				"P2PDeviceIDMatches(): DeviceID specified not match:\n", msg->devIdDevAddr);
			return FALSE;
		}
	}
	else
	{
		return TRUE;
	}
}

RT_STATUS
P2P_SetWpsIe(
	IN  const ADAPTER 			*pAdapter,
	IN  u2Byte 					iePayloadLen,
	IN  u1Byte 					*pIePayload
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	P2P_INFO 					*pP2PInfo = GET_P2P_INFO(pAdapter);
	FRAME_BUF					fbuf;
	P2P_MESSAGE					msg;
	P2P_WPS_ATTRIBUTES			*w = NULL;

	if(!P2P_ENABLED(pP2PInfo)) return RT_STATUS_INVALID_STATE;

	// Payload shall contain OUI and subtype, totally 4 bytes

	if(iePayloadLen < 3 + 1) return RT_STATUS_INVALID_DATA;

	FrameBuf_Init(iePayloadLen - 4, iePayloadLen - 4, pIePayload + 4, &fbuf);

	PlatformZeroMemory(&msg, sizeof(msg));
	msg.dbgLevel = DBG_LOUD;

	if(RT_STATUS_SUCCESS != (status = p2p_parse_WpsIe(&fbuf, &msg)))
			{
		return status;
			}

	w = &pP2PInfo->WpsAttributes;

	if(msg._wpsConfigMethods)
		w->ConfigMethod = msg.wpsConfigMethods;

	if(msg._wpsDevName)
	{
		w->DeviceNameLength = (u1Byte)msg.wpsDevNameLen;
		p2p_MoveMemory(w->DeviceName, msg._wpsDevName, msg.wpsDevNameLen);
			}

	if(msg._wpsDevPasswordId)
			{
		pP2PInfo->WpsDevPasswdId = msg.wpsDevPasswordId;

		w->DevicePasswdId = msg.wpsDevPasswordId;
		
				//
		// Notify UI the current device password ID so that it can determine whether provisioning info is available
		// Note that UI determines which peer config method we shall use for doing WPS
				//
		P2PIndicateCurrentDevPasswdId(pP2PInfo, pP2PInfo->WpsDevPasswdId);	
			}

	if(msg._wpsPrimaryDevType)
			{
		u2Byte 					cat = ReadN2H2BYTE(msg._wpsPrimaryDevType + 0);
		u2Byte					subCat = ReadN2H2BYTE(msg._wpsPrimaryDevType + 6);
				
		// We do this check because the WPS module set us a WPS IE with this attribute all 0
		if(0 != cat && 0 != subCat)
			{
			w->PrimaryDeviceType.CategoryId = cat;
			p2p_MoveMemory(w->PrimaryDeviceType.Oui, msg._wpsPrimaryDevType + 2, 4);
			w->PrimaryDeviceType.SubCategoryId = subCat;
				}
				}

	if(msg._wpsSecDevTypeList && msg.wpsSecDevTypeNum)
			{
		const u1Byte			*pos = msg._wpsSecDevTypeList;
		const u1Byte			*end = msg._wpsSecDevTypeList + msg.wpsSecDevTypeListLen;

		w->SecondaryDeviceTypeLength = msg.wpsSecDevTypeNum;

		while(pos < end)
					{
			P2P_WPS_ATTRIBUTES_DEVICE_TYPE *s = &w->SecondaryDeviceTypeList[w->SecondaryDeviceTypeLength];

			s->CategoryId = ReadN2H2BYTE(pos + 0);
			p2p_MoveMemory(s->Oui, pos + 2, 4);
			s->SubCategoryId = ReadN2H2BYTE(pos + 6);

			pos += 8;
					}
				}

	P2PDumpWpsAttributes(&pP2PInfo->WpsAttributes);

	return status;
}

//======================================================================
// Sub IE handlers
//======================================================================

BOOLEAN 
P2POnP2PNoticeOfAbsence(
	IN PP2P_INFO pP2PInfo,
	IN OCTET_STRING osPacket,
	IN P2P_MESSAGE *msg
	)
{
	u1Byte				ID = P2P_ATTR_NOTICE_OF_ABSENCE;
	u2Byte				Index = 0;
	u1Byte				nNoADesc = 0;
	u1Byte				i;
	P2P_POWERSAVE_SET	p2pPsSet;
	PMGNT_INFO			pMgntInfo = &(pP2PInfo->pAdapter->MgntInfo);

	// Win8: NdisTest Trick for WFD_Concurrent_ext: ------------------------------
	//	+ SUT-Client -> DUT-GO and DUT-Client -> SUT-GO are in the same channel 
	//	+ Use TxPause in AP_PS_UpdateStationPSState() instead
	if(pP2PInfo->pAdapter->bInHctTest) 
		return TRUE;
	// ----------------------------------------------------------------------


	// Our role isn't client.
	if(pP2PInfo->Role != P2P_CLIENT || !pMgntInfo->mAssoc)
		return TRUE;

	// BSSID mismatch
	// RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "P2POnP2PNoticeOfAbsence(): BSSID in packet = ", Frame_pBssid(osPacket));
	if(!eqMacAddr(Frame_pBssid(osPacket), pP2PInfo->pAdapter->MgntInfo.Bssid))
		return TRUE;

	//
	// Length check
	//
	if((msg->noaLen - 2) % 13 != 0) // Len of a NoA Desc is 13
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("P2POnP2PNoticeOfAbsence(): invalid length: %u\n", msg->noaLen));
		return FALSE;
	}
	else
	{
		nNoADesc = (msg->noaLen - 2) / 13;
	}

	// The index hasn't changed.		
	if(pP2PInfo->bUpdateFromBeacon && msg->noaIndex == pP2PInfo->NoAIEIndex)
		return TRUE;

	pP2PInfo->bUpdateFromBeacon = TRUE;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2POnP2PNoticeOfAbsence(): Role = %d, IE index = %d, MyCurIdx = %d\n", pP2PInfo->Role, msg->noaIndex, pP2PInfo->NoAIEIndex));

	// CTWindow
	p2pPsSet.bOppPs = msg->noaCtWindowAndOppPsParam >> 7;
	p2pPsSet.CTWindow = msg->noaCtWindowAndOppPsParam & (~BIT7);

	Index = 2;
	
	// NoA Descriptors
	for(i = 0; i < nNoADesc; i++)
	{
		// This NoA is valid.
		p2pPsSet.NoASet[i].bNoAEn = TRUE;
	
		// Count/Type
		p2pPsSet.NoASet[i].NoACnt = ReadEF1Byte(msg->_noa + Index);
		Index += 1;

		// Duration
		p2pPsSet.NoASet[i].NoADur = ReadEF4Byte(msg->_noa + Index);
		Index += 4;

		// Interval
		p2pPsSet.NoASet[i].NoAInt= ReadEF4Byte(msg->_noa + Index);
		Index += 4;

		// Start Time
		p2pPsSet.NoASet[i].bUseStartTime = TRUE;
		p2pPsSet.NoASet[i].u4StartTime = ReadEF4Byte(msg->_noa + Index);
		Index += 4;
	}

	pP2PInfo->bUpdatePsParameter = TRUE;
	P2PSetPowerSaveMode(pP2PInfo, &p2pPsSet, msg->noaIndex, FALSE);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("<=======\n"));

	return TRUE;
}

//======================================================================
// Dump Routine
//======================================================================
VOID 
P2PDumpWpsAttributes(
	IN PP2P_WPS_ATTRIBUTES pP2PWpsAttributes
	)
{
	u1Byte i;
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("WPS Attributes:\n"));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Config Method: 0x%X\n", pP2PWpsAttributes->ConfigMethod));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Device Passwd ID: 0x%X\n", pP2PWpsAttributes->DevicePasswdId));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("PrimaryDevType: %u\n", pP2PWpsAttributes->PrimaryDeviceType.SubCategoryId));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("SecDevType Length: %u\n", pP2PWpsAttributes->SecondaryDeviceTypeLength));
	if(pP2PWpsAttributes->SecondaryDeviceTypeLength)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("SecDevTypes: \n"));
		for(i = 0; i < pP2PWpsAttributes->SecondaryDeviceTypeLength; i++)
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%u\n", pP2PWpsAttributes->SecondaryDeviceTypeList[i].SubCategoryId));
		}
	}
	if(pP2PWpsAttributes->DeviceNameLength > 0) 
	{
		RT_PRINT_STR(COMP_P2P, DBG_LOUD, "DeviceName: ", pP2PWpsAttributes->DeviceName, pP2PWpsAttributes->DeviceNameLength);	
	}
	else
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("DeviceName: has a invalid length: %u\n", pP2PWpsAttributes->DeviceNameLength));
	}
}

VOID 
P2PDumpDeviceCapability(
	IN u1Byte DeviceCapability
	)
{
	RT_TRACE(COMP_P2P, DBG_LOUD, ("=== Device Capability ===\n"));
	
	if(DeviceCapability & dcServiceDiscovery)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("dcServiceDiscovery\n"));
	}
	if(DeviceCapability & dcP2PClientDiscoverability)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("dcP2PClientDiscoverability\n"));
	}
	if(DeviceCapability & dcConcurrentOperation)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("dcConcurrentOperation\n"));
	}
	if(DeviceCapability & dcP2PInfrastructureManaged)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("dcP2PInfrastructureManaged\n"));
	}
	if(DeviceCapability & dcP2PDeviceLimit)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("dcP2PDeviceLimit\n"));
	}
	if(DeviceCapability & dcP2PInvitationProcedure)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("dcP2PInvitationProcedure\n"));
	}
	
}

VOID 
P2PDumpGroupCapability(
	IN u1Byte GroupCapability
	)
{
	RT_TRACE(COMP_P2P, DBG_LOUD, ("=== Group Capability ===\n"));
	
	if(GroupCapability & gcP2PGroupOwner)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("gcP2PGroupOwner\n"));
	}
	if(GroupCapability & gcPersistentP2PGroup)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("gcPersistentP2PGroup\n"));
	}
	if(GroupCapability & gcP2PGroupLimit)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("gcP2PGroupLimit\n"));
	}
	if(GroupCapability & gcIntraBSSDistribution)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("gcIntraBSSDistribution\n"));
	}
	if(GroupCapability & gcCrossConnection)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("gcCrossConnection\n"));
	}
	if(GroupCapability & gcPersistentReconnect)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("gcPersistentReconnect\n"));
	}
	if(GroupCapability & gcGroupFormation)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("gcGroupFormation\n"));
	}
}

VOID 
P2PDumpClientInfoDescList(
	IN PP2P_CLIENT_INFO_DISCRIPTOR P2PClientInfoDescList,
	IN u1Byte P2PClientInfoDescListSize
	)
{
	u1Byte i;

	for(i = 0; i < P2PClientInfoDescListSize; i++)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("=== Client %d ===\n", i));
		RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "DevAddr: ", P2PClientInfoDescList[i].DeviceAddress);
		RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "InterfaceAddr: ", P2PClientInfoDescList[i].InterfaceAddress);
		P2PDumpDeviceCapability(P2PClientInfoDescList[i].DeviceCapability);
		P2PDumpWpsAttributes(&(P2PClientInfoDescList[i].WpsAttributes));
	}
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Totally %d Clients dumped\n", i));
}

VOID 
P2PDumpScanList(
	IN PP2P_DEVICE_DISCRIPTOR pDevDescriptors,
	IN u4Byte nDevDescriptors
	)
{
	u1Byte i;

	for(i = 0; i < nDevDescriptors; i++)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("====================================\n"));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("Scan List: %u\n", i));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("====================================\n"));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("Role: %u\n", pDevDescriptors[i].Role));
		P2PDumpGroupCapability(pDevDescriptors[i].GroupCapability);
		P2PDumpDeviceCapability(pDevDescriptors[i].DeviceCapability);
		RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "DevAddr: ", pDevDescriptors[i].DeviceAddress);
		RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "IntAddr: ", pDevDescriptors[i].IntendedP2PInterfaceAddress);
		RT_PRINT_STR(COMP_P2P, DBG_LOUD, "SSID: ", pDevDescriptors[i].SsidBuf, pDevDescriptors[i].SsidLen);
		RT_TRACE(COMP_P2P, DBG_LOUD, ("ListenChnl: %u\n", pDevDescriptors[i].ListenChannel));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("OpChnl: %u\n", pDevDescriptors[i].OperatingChannel));
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Channel List:\n", pDevDescriptors[i].ChannelPlanChannel, pDevDescriptors[i].ChannelPlanLength);
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "UUID:\n", pDevDescriptors[i].WPS_UUID, 16);
		P2PDumpWpsAttributes(&(pDevDescriptors[i].WpsAttributes));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("Num of Clients: %x\n", pDevDescriptors[i].P2PClientDescriptorListLength));
		P2PDumpClientInfoDescList(pDevDescriptors[i].P2PClientDescriptorList, pDevDescriptors[i].P2PClientDescriptorListLength);
		RT_TRACE(COMP_P2P, DBG_LOUD, ("ExtListenTimingPeriod: %u\n", pDevDescriptors[i].ExtendedListenTimingPeriod));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("ExtListenTimingDuration: %u\n", pDevDescriptors[i].ExtendedListenTimingDuration));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("SingalStrength: %d\n", pDevDescriptors[i].SignalStrength));
		RT_PRINT_STR(COMP_P2P, DBG_LOUD, "Manufacturer name: ", pDevDescriptors->manufacturerName, 65);
	}
	RT_TRACE(COMP_P2P, DBG_LOUD, ("====================================\n"));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Totally %d items in Scan List dumped\n", i));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("====================================\n"));
}

VOID
P2PDumpGroupFormationResult(
	PP2P_INFO pP2PInfo
	)
{
	RT_TRACE(COMP_P2P, DBG_LOUD, ("====================================\n"));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Group Formation Result:\n"));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("====================================\n"));
	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "DevAddr: ", pP2PInfo->ConnectionContext.ConnectingDevice.DeviceAddress);
	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "IntAddr: ", pP2PInfo->ConnectionContext.ConnectingDevice.IntendedP2PInterfaceAddress);
	if(pP2PInfo->ConnectionContext.bGoingToBeGO)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("I'm GO\n"));
	}
	else
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("I'm Client\n"));
	}
	RT_TRACE(COMP_P2P, DBG_LOUD, ("OpChnl: %u\n", pP2PInfo->OperatingChannel));
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "Channel List:\n", pP2PInfo->ConnectionContext.ConnectingDevice.ChannelPlanChannel, pP2PInfo->ConnectionContext.ConnectingDevice.ChannelPlanLength);
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Status: %u\n", pP2PInfo->ConnectionContext.Status));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("DialogToken: %u\n", pP2PInfo->ConnectionContext.DialogToken));
	RT_PRINT_STR(COMP_P2P, DBG_LOUD, "SSID = ", pP2PInfo->ConnectionContext.ConnectingDevice.SsidBuf, pP2PInfo->ConnectionContext.ConnectingDevice.SsidLen);
}

VOID
P2PDumpPacketType(
	u1Byte TypeSubType,
	u1Byte Category,
	u1Byte OUISubType
	)
{
	switch(TypeSubType) 
	{
		case Type_Beacon:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): Type_Beacon\n"));
			break;
		case Type_Probe_Req:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): Type_Probe_Req\n"));
			break;
		case Type_Probe_Rsp:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): Type_Probe_Rsp\n"));
			break;
		case Type_Asoc_Req:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): Type_Asoc_Req\n"));
			break;
		case Type_Asoc_Rsp:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): Type_Asoc_Rsp\n"));
			break;
		case Type_Reasoc_Req:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): Type_Reasoc_Req\n"));
			break;
		case Type_Reasoc_Rsp:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): Type_Reasoc_Rsp\n"));
			break;
		case Type_Action:
			{
				if(Category == WLAN_ACTION_PUBLIC)
				{
					if(OUISubType == P2P_PUB_ACT_GO_NEGO_REQ)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): GONReq\n"));
					if(OUISubType == P2P_PUB_ACT_GO_NEGO_RSP)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): GONRsp\n"));
					if(OUISubType == P2P_PUB_ACT_GO_NEGO_CONFIRM)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): GONConfirm\n"));
					if(OUISubType == P2P_PUB_ACT_INVITATION_REQ)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): InvitationReq\n"));
					if(OUISubType == P2P_PUB_ACT_INVITATION_RSP)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): InvitationRsp\n"));
					if(OUISubType == P2P_PUB_ACT_DEV_DISCOVERABILITY_REQ)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): DevDiscoverabilityReq\n"));
					if(OUISubType == P2P_PUB_ACT_DEV_DISCOVERABILITY_RSP)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): DevDiscoverabilityRsp\n"));
					if(OUISubType == P2P_PUB_ACT_PROVISION_DISCOVERY_REQ)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): ProvisionDiscoveryReq\n"));
					if(OUISubType == P2P_PUB_ACT_PROVISION_DISCOVERY_RSP)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): ProvisionDiscoveryRsp\n"));
					if(OUISubType == P2P_PUB_ACT_GAS_INITIAL_REQ)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): ServiceDiscoveryReq (GAS Initial Req)\n"));
					if(OUISubType == P2P_PUB_ACT_GAS_INITIAL_RSP)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): ServiceDiscoveryRsp (GAS Initial Rsp)\n"));
					if(OUISubType == P2P_PUB_ACT_GAS_COMEBACK_REQ)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): ServiceDiscoveryReq (GAS Comeback Req)\n"));
					if(OUISubType == P2P_PUB_ACT_GAS_COMEBACK_RSP)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): ServiceDiscoveryRsp (GAS Comeback Rsp)\n"));
				}
				else if(Category == WLAN_ACTION_VENDOR_SPECIFIC)
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): TODO: P2P_ACTION_CATEGORY\n"));
					if(OUISubType == P2P_ACT_NOTICE_OF_ABSENCE)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): NoticeOfAbsence\n"));
					if(OUISubType == P2P_PUB_PRESENCE_REQ)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): PresenceReq\n"));
					if(OUISubType == P2P_PUB_PRESENCE_RSP)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): PresenceRsp\n"));
					if(OUISubType == P2P_PUB_GO_DISCOVERABILITY_REQ)
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): GODiscoverabilityReq\n"));
				}
				else
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): unrecognized Action Category: %u\n", Category));
				}
			}
			break;
		default:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDumpPacketType(): unrecognized TypeSubType: %u\n", TypeSubType));
			break;
	}
}

//======================================================================
// Client Info List Hander
//======================================================================
u1Byte
P2PClientInfoGetCount(
	IN PP2P_INFO pP2PInfo
	)
{
	PADAPTER pExtAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? pP2PInfo->pAdapter : GetFirstGOPort(pP2PInfo->pAdapter);

	PMGNT_INFO pExtMgntInfo = &pExtAdapter->MgntInfo;
	PRT_WLAN_STA pEntry = NULL;
	u1Byte nP2PClients = 0;
	u1Byte i = 0;

	if(pExtAdapter == NULL || pExtMgntInfo == NULL)
	{
		return 0;
	}
	
	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		pEntry = &(pExtMgntInfo->AsocEntry[i]);
		if(pEntry->bUsed &&
			pEntry->bAssociated &&
			pEntry->bP2PClient)
		{
			nP2PClients++;
		}
	}

	return nP2PClients;
}

PP2P_CLIENT_INFO_DISCRIPTOR 
P2PClientInfoFindByInterfaceAddress(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte InterfaceAddress
	)
{
	BOOLEAN bFound = FALSE;

	PADAPTER pExtAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? pP2PInfo->pAdapter : GetFirstGOPort(pP2PInfo->pAdapter);

	PMGNT_INFO pExtMgntInfo = &pExtAdapter->MgntInfo;
	PRT_WLAN_STA pEntry = NULL;
	u1Byte i = 0;

	if(pExtAdapter == NULL || pExtMgntInfo == NULL)
	{
		return 0;
	}
	
	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		pEntry = &(pExtMgntInfo->AsocEntry[i]);
		if(pEntry->bUsed &&
			pEntry->bAssociated &&
			pEntry->bP2PClient)
		{
			if(eqMacAddr(pEntry->P2PClientInfoDesc.InterfaceAddress, InterfaceAddress))
			{
				bFound = TRUE;
				return &(pEntry->P2PClientInfoDesc);
			}
		}
	}

	return NULL;
}

PP2P_CLIENT_INFO_DISCRIPTOR 
P2PClientInfoFindByDeviceAddress(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte DeviceAddress
	)
{
	BOOLEAN bFound = FALSE;

	PADAPTER pExtAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? pP2PInfo->pAdapter : GetFirstGOPort(pP2PInfo->pAdapter);

	PMGNT_INFO pExtMgntInfo = &pExtAdapter->MgntInfo;
	PRT_WLAN_STA pEntry = NULL;
	u1Byte i = 0;

	if(pExtAdapter == NULL || pExtMgntInfo == NULL)
	{
		return 0;
	}
	
	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		pEntry = &(pExtMgntInfo->AsocEntry[i]);
		if(pEntry->bUsed &&
			pEntry->bAssociated &&
			pEntry->bP2PClient)
		{
			if(eqMacAddr(pEntry->P2PClientInfoDesc.DeviceAddress, DeviceAddress))
			{
				bFound = TRUE;
				return &(pEntry->P2PClientInfoDesc);
			}
		}
	}

	return NULL;
}

PP2P_CLIENT_INFO_DISCRIPTOR 
P2PClientInfoEnumClients(
	IN PP2P_INFO pP2PInfo,
	IN u1Byte StartIndex,
	OUT pu1Byte pIndex
	)
{
	BOOLEAN bFound = FALSE;
	
	PADAPTER pExtAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? pP2PInfo->pAdapter : GetFirstGOPort(pP2PInfo->pAdapter);


	PMGNT_INFO pExtMgntInfo = &pExtAdapter->MgntInfo;
	PRT_WLAN_STA pEntry = NULL;
	u1Byte i = 0;

	if(StartIndex > ASSOCIATE_ENTRY_NUM - 1)
	{
		goto exit_P2PClientInfoEnumClients;
	}

	if(pExtAdapter == NULL || pExtMgntInfo == NULL)
	{
		return 0;
	}
	
	for(i = StartIndex; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		pEntry = &(pExtMgntInfo->AsocEntry[i]);
		if(pEntry->bUsed &&
			pEntry->bAssociated &&
			pEntry->bP2PClient)
		{

			bFound = TRUE;
			if(pIndex) *pIndex = i;
			return &(pEntry->P2PClientInfoDesc);
		}
	}

exit_P2PClientInfoEnumClients:
	
	if(pIndex) *pIndex = (u1Byte)(-1);
	return NULL;
}

//======================================================================
// Scan List Hander
//======================================================================
PP2P_DEVICE_DISCRIPTOR 
P2PScanListFind(
	IN PP2P_DEVICE_DISCRIPTOR pScanList,
	IN u4Byte ScanListSize,
	IN pu1Byte DeviceAddress, 
	IN pu1Byte InterfaceAddress, 
	OUT pu4Byte pScanListIndex
	)
{
	u1Byte i;
	BOOLEAN bFound = FALSE;
	pu1Byte KeyAddr = (DeviceAddress) ? (DeviceAddress) : ((InterfaceAddress) ? (InterfaceAddress) : (NULL));
	BOOLEAN bDevAddressAsKey = (DeviceAddress) ? (TRUE) : (FALSE);

	if(!KeyAddr)
	{
		return NULL;
	}

	for(i = 0; i < ScanListSize; i++) 
	{
		pu1Byte AddrToCompare = (bDevAddressAsKey) ? 
			((pScanList[i]).DeviceAddress) : 
			((pScanList[i]).IntendedP2PInterfaceAddress);
		if(eqMacAddr(AddrToCompare, KeyAddr))
		{
			bFound = TRUE;
			if(pScanListIndex)
			{
				*pScanListIndex = i;
			}
			return &(pScanList[i]);
		}
	}
	
	return NULL;
}

BOOLEAN
P2PScanListAllFound(
	IN PP2P_INFO pP2PInfo
	)
{
	u1Byte i = 0,j=0,uNumOfDeviceFoud=0;
	BOOLEAN bAllFound = FALSE;

	if(pP2PInfo->ScanDeviceIDs.uNumOfDeviceIDs == 0 ||pP2PInfo->DeviceListForQuery.uNumberOfDevices ==0)
		return FALSE;
	
	for(i=0;i<(pP2PInfo->ScanDeviceIDs.uNumOfDeviceIDs);i++)//
	{
		for(j = 0; j < (pP2PInfo->DeviceListForQuery.uNumberOfDevices); j++)
		{
			if(pP2PInfo->DeviceListForQuery.DeviceEntry[j].DeviceAddress != NULL)
			{
			if(eqMacAddr(pP2PInfo->ScanDeviceIDs.DeviceIDs[i], pP2PInfo->DeviceListForQuery.DeviceEntry[j].DeviceAddress) 
				&&pP2PInfo->DeviceListForQuery.DeviceEntry[j].ProbeResponseHostTimestamp != 0)
			{
				uNumOfDeviceFoud ++;
			}
		}
	}
	}

	if(uNumOfDeviceFoud == pP2PInfo->ScanDeviceIDs.uNumOfDeviceIDs)
	{	
		bAllFound =TRUE;
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: All of DeviceID Found : Terminate Device Discovery!! !\n", __FUNCTION__));						
	}
	
	return bAllFound;
}

BOOLEAN 
P2PScanListFindClient(
	IN PP2P_DEVICE_DISCRIPTOR pScanList,
	IN u4Byte ScanListSize,
	IN pu1Byte DeviceAddress, 
	IN pu1Byte InterfaceAddress,
	OUT PP2P_DEVICE_DISCRIPTOR *ppGODeviceDesc,
	OUT PP2P_CLIENT_INFO_DISCRIPTOR *ppClientInfoDesc
	)
{
	u1Byte i, j;
	BOOLEAN bFound = FALSE;
	pu1Byte KeyAddr = (DeviceAddress) ? (DeviceAddress) : ((InterfaceAddress) ? (InterfaceAddress) : (NULL));
	BOOLEAN bDevAddressAsKey = (DeviceAddress) ? (TRUE) : (FALSE);

	*ppGODeviceDesc = NULL;
	*ppClientInfoDesc = NULL;

	for(i = 0; i < ScanListSize; i++) 
	{
		PP2P_CLIENT_INFO_DISCRIPTOR pClientInfo = pScanList[i].P2PClientDescriptorList;
		u1Byte P2PClientDescriptorListLength = pScanList[i].P2PClientDescriptorListLength;

		for(j = 0; j < P2PClientDescriptorListLength; j++)
		{
			pu1Byte AddrToCompare = (bDevAddressAsKey) ? 
				((pClientInfo[j]).DeviceAddress) : 
				((pClientInfo[j]).InterfaceAddress);
			if(KeyAddr != NULL && eqMacAddr(AddrToCompare, KeyAddr))
			{
				bFound = TRUE;
				*ppGODeviceDesc = (PP2P_DEVICE_DISCRIPTOR)(&(pScanList[i]));
				*ppClientInfoDesc = (PP2P_CLIENT_INFO_DISCRIPTOR)(&(pClientInfo[j]));
				goto exit_P2PScanListFindClient;
			}
		}
		
	}

exit_P2PScanListFindClient:	
	return bFound;
}

PP2P_DEVICE_DISCRIPTOR 
P2PScanListAdd(
	IN OUT PP2P_DEVICE_DISCRIPTOR pScanList,
	IN OUT pu4Byte pScanListSize
	)
{
	PP2P_DEVICE_DISCRIPTOR pP2PDeviceDesc = NULL;

	
	if(*pScanListSize == P2P_MAX_SCAN_LIST) 
	{// no more space => return the last one
		pP2PDeviceDesc = &(pScanList[*pScanListSize - 1]);
	}
	else 
	{
		pP2PDeviceDesc = &(pScanList[*pScanListSize]);
		(*pScanListSize)++;
	}
	
	return pP2PDeviceDesc;
}

VOID 
P2PScanListClear(
	IN  P2P_INFO				*pP2PInfo
	)
{
	pP2PInfo->ScanListSize = 0;
	PlatformZeroMemory(pP2PInfo->ScanList, sizeof(pP2PInfo->ScanList));

	pP2PInfo->ScanList4QuerySize = 0;
	PlatformZeroMemory(pP2PInfo->ScanList4Query, sizeof(pP2PInfo->ScanList4Query));

	p2p_DevList_Flush(&pP2PInfo->devList);

	return;
}

VOID 
P2PScanListCopy(
	IN OUT PP2P_DEVICE_DISCRIPTOR pScanListDest,
	IN OUT pu4Byte pScanListDestSize,
	IN PP2P_DEVICE_DISCRIPTOR pScanListSrc,
	IN u4Byte ScanListSrcSize
	)
{
	*pScanListDestSize = ScanListSrcSize;
	PlatformMoveMemory(pScanListDest, 
		pScanListSrc, 
		(ScanListSrcSize * sizeof(P2P_DEVICE_DISCRIPTOR)));
}

BOOLEAN
P2PScanListDumplicate(
	IN PP2P_DEVICE_DISCRIPTOR pScanList,
	IN u4Byte ScanListSize,
	IN PP2P_DEVICE_DISCRIPTOR pDevDesc
	)
{
	//
	// Test if pDevDesc is a duplicate item in pScanList.
	//
	
	PP2P_DEVICE_DISCRIPTOR pTmpDevDesc = NULL;

	pTmpDevDesc = P2PScanListFind(pScanList, ScanListSize, pDevDesc->DeviceAddress, NULL, NULL);
	if(pTmpDevDesc)
	{
		if(PlatformCompareMemory(pTmpDevDesc, 
			pDevDesc, sizeof(P2P_DEVICE_DISCRIPTOR)) == 0)
		{// desc the same
			return TRUE;
		}
		else
		{// dese not the same
			return FALSE;
		}
	}
	else
	{// no desc found
		return FALSE;
	}

	return FALSE;
}

BOOLEAN
P2PScanListEqual(
	IN PP2P_DEVICE_DISCRIPTOR pScanList1,
	IN u4Byte ScanList1Size,
	IN PP2P_DEVICE_DISCRIPTOR pScanList2,
	IN u4Byte ScanList2Size
	)
{
	u4Byte i;

	//RT_TRACE(COMP_P2P, DBG_LOUD, 
	//		("P2PScanListEqual(): nlist1(%u), nlist2(%u)\n", ScanList1Size, ScanList2Size));

	if(ScanList1Size != ScanList2Size)
	{
		return FALSE;
	}

	//
	// For each entry in scan list 1, check if the entry is duplicate in scan list 2,
	// if all entries are duplicate, return TRUE;
	//
	for(i = 0; i < ScanList1Size; i++) 
	{
		if(!P2PScanListDumplicate(pScanList2, ScanList2Size, &(pScanList1[i])))
		{
			return FALSE; // this entry is not duplicate
		}
	}

	return TRUE; // all entries are duplicate
}

BOOLEAN
P2PScanListIsGo(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte InterfaceAddr
	)
{
	PP2P_DEVICE_DISCRIPTOR pP2PDeviceDesc = NULL;
	pP2PDeviceDesc = P2PScanListFind(pP2PInfo->ScanList, pP2PInfo->ScanListSize, NULL, InterfaceAddr, NULL);
	if(pP2PDeviceDesc)
	{
		if(pP2PDeviceDesc->Role == P2P_GO)
		{
			return TRUE;
		}
	}
	return FALSE;
}

VOID
P2PScanListEnterScanCompleteImmediately(
	IN PP2P_INFO pP2PInfo
	)
{
	PADAPTER pAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? pP2PInfo->pAdapter : GetDefaultAdapter(pP2PInfo->pAdapter);
	PADAPTER pLoopAdapter = GetDefaultAdapter(pAdapter);
	PRT_CHANNEL_INFO		pChnlInfo;
	PMGNT_INFO pMgntInfo = &pAdapter->MgntInfo;

	if(!MgntScanInProgress(pMgntInfo))
	{
		return;
	}

	{

		while(pLoopAdapter)
		{
			pMgntInfo = &pLoopAdapter->MgntInfo;
			pChnlInfo = pMgntInfo->pChannelInfo;
			if(pChnlInfo->ChnlOp == CHNLOP_SCAN)
				break;
			else			
				pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}

	if(pLoopAdapter == NULL)
	{
		pLoopAdapter = pAdapter;
		pMgntInfo = &pLoopAdapter->MgntInfo;
	}

	if(pMgntInfo->ScanStep != 2)
	{
		pMgntInfo->ScanStep = 0;
	}
	
	PlatformCancelTimer(pLoopAdapter, &(pMgntInfo->ScanTimer));
	PlatformSetTimer(pLoopAdapter, &(pMgntInfo->ScanTimer), 0);
}

//
// This function is called to cease the current scan.
// The purpose is to stop the current scan to stay on the current channel for sending/receiving frames.
// If called in operating state, the current channel (op channel) shall be preserved.
//
u1Byte
P2PScanListCeaseScan(
	IN PP2P_INFO pP2PInfo
	)
{
	u1Byte CurrentChnl;
	PADAPTER pAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? pP2PInfo->pAdapter : GetDefaultAdapter(pP2PInfo->pAdapter);

	EXTCHNL_OFFSET		hwBW40MOffset = EXTCHNL_OFFSET_NO_EXT;
	EXTCHNL_OFFSET		hwBW80MOffset = EXTCHNL_OFFSET_NO_EXT;

	// Get current channel. We want to know on what channel we cease the scan. 
	CurrentChnl = P2PGetChannel(pP2PInfo);
	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_BW40MHZ_EXTCHNL, (pu1Byte)(&hwBW40MOffset));
	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_BW80MHZ_EXTCHNL, (pu1Byte)(&hwBW80MOffset));

	if(!pAdapter->MgntInfo.bScanInProgress)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PScanListCeaseScan(): bScanInProgress is false\n"));
		return CurrentChnl;
	}

		RT_TRACE(COMP_SCAN,DBG_LOUD, ("===>P2PScanListCeaseScan portnumber %d\n", pAdapter->pNdis62Common->PortNumber));

	if(pP2PInfo->bPreGroupFormation || (pP2PInfo->State == P2P_STATE_GO_NEGO_REQ_RECVD))
	{
		PADAPTER pLoopAdapter = GetDefaultAdapter(pAdapter);

		CustomScan_TermReq(GET_CUSTOM_SCAN_INFO(pP2PInfo->pAdapter), TRUE);

		while(pLoopAdapter)
		{

			pLoopAdapter->MgntInfo.SettingBeforeScan.Ext20MHzChnlOffsetOf40MHz = hwBW40MOffset;

			if(EXTCHNL_OFFSET_NO_EXT != hwBW80MOffset)
			{
				pLoopAdapter->MgntInfo.SettingBeforeScan.ChannelBandwidth = CHANNEL_WIDTH_80;
			}
			else if(EXTCHNL_OFFSET_NO_EXT != hwBW40MOffset)
			{
				pLoopAdapter->MgntInfo.SettingBeforeScan.ChannelBandwidth = CHANNEL_WIDTH_40;
			}
			else
			{
				pLoopAdapter->MgntInfo.SettingBeforeScan.ChannelBandwidth = CHANNEL_WIDTH_20;
			}
				
			pLoopAdapter->MgntInfo.SettingBeforeScan.ChannelNumber = CurrentChnl;
			pLoopAdapter->MgntInfo.SettingBeforeScan.CenterFrequencyIndex1 = CurrentChnl;
			
			pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
		}
	}
	else
	{
		CustomScan_TermReq(GET_CUSTOM_SCAN_INFO(pP2PInfo->pAdapter), FALSE);
	}
		
	//Cancel current scan timer, which may be triggered after a long time
	// and set it again with no delay so that it will enter ScanComplete soon.
	//P2PScanListEnterScanCompleteImmediately(pP2PInfo);

	//
	// Extended listening may be ceased, so we have to clear the flag.
	//
	pP2PInfo->bExtendedListening = FALSE;

	pP2PInfo->bDeviceDiscoveryInProgress = FALSE;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("<===P2PScanListCeaseScan()\n"));

	return CurrentChnl;
}

static
VOID
p2p_ScanStateCb(
	IN  CUSTOM_SCAN_STATE		state,
	IN  VOID					*pCtx
	)
{
	ADAPTER						*pAdapter = (ADAPTER *)pCtx;

	if(CUSTOM_SCAN_STATE_COMPLETED == state)
	{
		P2P_UpdateScanList(pAdapter);
	}

	return;
}

BOOLEAN
P2PConstructScanList(
	IN 	P2P_INFO 				*pP2PInfo,
	IN  VOID					*req,
	IN  u1Byte					rate,
	IN  FRAME_BUF				*probeReqBuf
	)
{
	RT_SCAN_TYPE				scanType = SCAN_PASSIVE;
	u2Byte						duration = 0;

	// Scan period
	if(pP2PInfo->State == P2P_STATE_LISTEN)
	{
		scanType = SCAN_PASSIVE;

		if(
			pP2PInfo->bPreGroupFormation || 
			pP2PInfo->ProvisionDiscoveryContext.bDoingProvisionDiscovery ||
			pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe ||
			pP2PInfo->SDContext.bDoingServiceDiscovery
		)
		{
			duration = 50;
		}
		else
		{
			duration = (u2Byte)(100 * GetRandomNumber(1, 3 + 1) - 50);
		}
	}
	else if(pP2PInfo->State == P2P_STATE_INITIALIZED) // Extended Listen
	{
		if(pP2PInfo->bSendProbeReqInExtendedListen)
		{
			scanType = SCAN_ACTIVE;
		}
		else 
		{
			scanType = SCAN_PASSIVE;
		}

		duration = pP2PInfo->ExtListenTimingDuration;
		//*pDuration = (pP2PInfo->ExtListenTimingDuration / 2) * ListenStateDurationMultiplier;
		RT_TRACE(COMP_P2P, DBG_TRACE, ("Extended Listening Duration: %u\n", duration));
	}
	else if(pP2PInfo->State == P2P_STATE_SEARCH)
	{
		scanType = SCAN_ACTIVE;
		duration = 20; 				// P2P_SCAN_PERIOD_SEARCH;
	}
	else if(pP2PInfo->State == P2P_STATE_SCAN)
	{
		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			scanType = pP2PInfo->ScanType;
			duration = 50;
		}
		else
		{
			scanType = SCAN_ACTIVE;
			duration = 50;
		}
	}
	else if(pP2PInfo->State == P2P_STATE_INVITATION_REQ_SEND)
	{
		scanType = SCAN_ACTIVE;
		duration = P2P_INVITATION_FRAME_TIMEOUT;
	}
	else if(pP2PInfo->State == P2P_STATE_PROVISION_DISCOVERY_REQ_SEND)
	{
		scanType = SCAN_ACTIVE;
		duration = P2P_PROVISION_DISCOVERY_TIMEOUT;
	}
	else if(pP2PInfo->State == P2P_STATE_SERVICE_DISCOVERY_REQ_SEND)
	{
		scanType = SCAN_ACTIVE;
		duration = P2P_SERVICE_DISCOVERY_TIMEOUT;
	}
	else if(pP2PInfo->State == P2P_STATE_DEVICE_DISCOVERABILITY_REQ_SEND)
	{
		scanType = SCAN_ACTIVE;
		duration = P2P_DEVICE_DISCOVERABILITY_FRAME_TIMEOUT;
	}
	else
	{
		return FALSE;
	}

	// Decide channels to scan
	if(pP2PInfo->State == P2P_STATE_LISTEN ||
		pP2PInfo->State == P2P_STATE_INITIALIZED) // Extended Listen
	{// the listen channel
		CustomScan_AddScanChnl(req, pP2PInfo->ListenChannel, 1, scanType, duration, rate, probeReqBuf);
	}
	else if(pP2PInfo->State == P2P_STATE_SCAN)
	{
		// TODO: is this right?

		u1Byte i = 0;
		PRT_CHANNEL_LIST pChList = MgntActQuery_ChannelList(pP2PInfo->pAdapter);
		u2Byte SupportedWirelessMode = pP2PInfo->pAdapter->HalFunc.GetSupportedWirelessModeHandler(pP2PInfo->pAdapter);
		WIRELESS_MODE CurrentWirelessModeBackup = pP2PInfo->pAdapter->MgntInfo.dot11CurrentWirelessMode;

		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2P_STATE_SCAN: pChList->ChannelLen: %d\n", pChList->ChannelLen));
		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2P_STATE_SCAN: SupportedWirelessMode: 0x%x CurrentWirelessMode 0x%x\n", SupportedWirelessMode, CurrentWirelessModeBackup));

		pP2PInfo->pAdapter->MgntInfo.dot11CurrentWirelessMode = SupportedWirelessMode;
		RtActChannelList(pP2PInfo->pAdapter, RT_CHNL_LIST_ACTION_CONSTRUCT, NULL, NULL);
		RtActChannelList(pP2PInfo->pAdapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST, NULL, &pChList);
		pP2PInfo->pAdapter->MgntInfo.dot11CurrentWirelessMode = CurrentWirelessModeBackup;

		// Instert the Listen Channel (Listen channel first due to fast discovery of NdisTest-WFD_discover_ext)
		CustomScan_AddScanChnl(req, pP2PInfo->ListenChannel, 1, scanType, duration, rate, probeReqBuf);
		
		for(i = 0; i < pChList->ChannelLen; i++)
		{
			if(pChList->ChnlListEntry[i].ChannelNum != pP2PInfo->ListenChannel)
			{
				CustomScan_AddScanChnl(req, pChList->ChnlListEntry[i].ChannelNum, 1, scanType, duration, rate, probeReqBuf);
			}
		}		
	}
	else if(pP2PInfo->State == P2P_STATE_INVITATION_REQ_SEND)
	{
		PP2P_DEVICE_DISCRIPTOR pDevDesc = &pP2PInfo->InvitationContext.InvitedDevice;
		if(pDevDesc->Role == P2P_DEVICE)
		{// GO/Client inviting a Dev, Instert current channel since we start to send InvitationReq upon ProbeReq or ProbeRsp is received from the Dev
			if(0 != pP2PInfo->InvitationContext.Channel)
			{
				CustomScan_AddScanChnl(req, pP2PInfo->InvitationContext.Channel, 1, scanType, duration, rate, probeReqBuf);
			}
			else
			{
				CustomScan_AddScanChnl(req, 1, 1, scanType, duration, rate, probeReqBuf);
				CustomScan_AddScanChnl(req, 6, 1, scanType, duration, rate, probeReqBuf);
				CustomScan_AddScanChnl(req, 11, 1, scanType, duration, rate, probeReqBuf);
				CustomScan_AddScanChnl(req, pP2PInfo->ListenChannel, 1, scanType, duration, rate, probeReqBuf);
					}
				}
		else if(pDevDesc->Role == P2P_CLIENT)
		{// Instert peer Op Channel
			CustomScan_AddScanChnl(req, pDevDesc->OperatingChannel, 1, scanType, duration, rate, probeReqBuf);
		}
		else if(pDevDesc->Role == P2P_GO)
		{// Instert peer Op Channel
			CustomScan_AddScanChnl(req, pDevDesc->OperatingChannel, 1, scanType, duration, rate, probeReqBuf);
		}
	}
	else if(pP2PInfo->State == P2P_STATE_PROVISION_DISCOVERY_REQ_SEND)
	{
		CustomScan_AddScanChnl(req, pP2PInfo->ProvisionDiscoveryContext.Channel, 1, scanType, duration, rate, probeReqBuf);
	}
	else if(pP2PInfo->State == P2P_STATE_SERVICE_DISCOVERY_REQ_SEND)
	{
		CustomScan_AddScanChnl(req, pP2PInfo->SDContext.Channel, 1, scanType, duration, rate, probeReqBuf);
	}
	else if(pP2PInfo->State == P2P_STATE_DEVICE_DISCOVERABILITY_REQ_SEND)
	{
		CustomScan_AddScanChnl(req, pP2PInfo->DeviceDiscoverabilityContext.GOChannel, 1, scanType, duration, rate, probeReqBuf);
	}
	else //P2P_STATE_SEARCH
	{
		//
		// If Device Discovery is used for finding when the target is active on its listen channel,
		// we can search only its listen channel to perserve time.
		//
		if(pP2PInfo->bPreGroupFormation)
		{//DbgPrint("a: %u, %u\n", pP2PInfo->ConnectionContext.ConnectingDevice.Channel, *pNChannels);
			CustomScan_AddScanChnl(req, pP2PInfo->ConnectionContext.ConnectingDevice.ListenChannel, 5, scanType, duration, rate, probeReqBuf);
		}
		else if(pP2PInfo->ProvisionDiscoveryContext.bDoingProvisionDiscovery)
		{
			CustomScan_AddScanChnl(req, pP2PInfo->ProvisionDiscoveryContext.Channel, 5, scanType, duration, rate, probeReqBuf);
		}
		else if(pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe && 
			pP2PInfo->InvitationContext.InvitedDevice.ListenChannel != 0)
		{
			CustomScan_AddScanChnl(req, pP2PInfo->InvitationContext.InvitedDevice.ListenChannel, 5, scanType, duration, rate, probeReqBuf);
		}
		else if(pP2PInfo->SDContext.bDoingServiceDiscovery)
		{
			CustomScan_AddScanChnl(req, pP2PInfo->SDContext.Channel, 5, scanType, duration, rate, probeReqBuf);
		}
		else
		{// scan all social channels
			CustomScan_AddScanChnl(req, 1, 5, scanType, duration, rate, probeReqBuf);
			CustomScan_AddScanChnl(req, 6, 5, scanType, duration, rate, probeReqBuf);
			CustomScan_AddScanChnl(req, 11, 5, scanType, duration, rate, probeReqBuf);

			if( GetDefaultMgntInfo(pP2PInfo->pAdapter)->WFDPeerOpChannel > 0 )
			{
				CustomScan_AddScanChnl(req, GetDefaultMgntInfo(pP2PInfo->pAdapter)->WFDPeerOpChannel, 2, scanType, duration, rate, probeReqBuf);
			}		
				}
			}

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("LsnCh: %u, OpCh: %u\n", pP2PInfo->ListenChannel, pP2PInfo->OperatingChannel));

	if(!CustomScan_NumAddedChnl(req))
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Error: no channel in the list, state: %u\n", pP2PInfo->State));
		return FALSE;
	}
	else
	{
		CustomScan_SetupCbCtx(req, p2p_ScanStateCb, pP2PInfo->pAdapter);
		return TRUE;
	}
}

//======================================================================
// Wi-Fi Direct Packet Handler
//======================================================================

PP2P_DEVICE_DISCRIPTOR
P2PProcessSubElements(
	IN  PP2P_INFO 				pP2PInfo,
	IN  PRT_RFD					pRfd,
	IN  pu1Byte 				DeviceAddress,
	IN  P2P_MESSAGE 			*msg,
	IN  BOOLEAN 				bCreateIfNotExist
	)
{
	P2P_DEVICE_DISCRIPTOR 		*pDesc = NULL;
	
	if(NULL == (pDesc = P2PScanListFind(pP2PInfo->ScanList, pP2PInfo->ScanListSize, DeviceAddress, NULL, NULL))) 
	{
		if(!bCreateIfNotExist)
			return NULL;
		
		if(NULL == (pDesc = P2PScanListAdd(pP2PInfo->ScanList, &pP2PInfo->ScanListSize)))
			return NULL;

		PlatformZeroMemory(pDesc, sizeof(*pDesc));
	}

	p2p_parse_UpdateDevDesc(pRfd->Buffer.VirtualAddress, msg, pRfd->Status.SignalStrength, pDesc);

	return pDesc;
}

VOID
p2pUpdateBssDescManageability(
	IN  ADAPTER					*pAdapter,
	IN  u1Byte					*bssid,
	IN  const P2P_MESSAGE		*msg
	)
{
	if(msg->_manageability)
	{
		PRT_WLAN_BSS	pBssDesc = BssDescDupByBssid(pAdapter, bssid);
		if(pBssDesc)
		{
			pBssDesc->P2PManagedInfo = msg->manageability;
		}
	}
	else
	{
		RT_TRACE_F(COMP_P2P, DBG_TRACE, ("Managed P2P Device Info NULL\n"));
	}

	return;
}

//
// Description:
//	Handle Beacon packet for P2P function.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	This function updates the P2P scan list and check the WPS state for join.
//
RT_STATUS
P2P_OnBeacon(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL;
	OCTET_STRING			ssidBeacon;

	P2P_DEV_LIST_ENTRY		*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;

	MEMORY_BUFFER			mb = {NULL, 0};

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) && pP2PInfo->bForceScanLegacyNetworks)
	{
		ssidBeacon = PacketGetElement(*posMpdu, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);		
		if(IsSSIDNdisTest(ssidBeacon) || !pAdapter->bInHctTest)
		{			
			MEMORY_BUFFER	mbPacket = {NULL, 0};
			mbPacket.Buffer = posMpdu->Octet;
			mbPacket.Length = posMpdu->Length;
			RT_PRINT_STR(COMP_P2P, DBG_LOUD, "P2P_OnBeacon IsSSIDNdisTest:", ssidBeacon.Octet, ssidBeacon.Length);
			
			P2PDeviceListUpdate(pP2PInfo, &pP2PInfo->DeviceList, pRfd, Frame_pBssid(*posMpdu), NULL, mbPacket);
		}
	}

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_BEACON, pRfd, Frame_pBssid(*posMpdu), 
						RT_GetChannelNumber(pP2PInfo->pAdapter), &pDev))
			)
		{
			break;
		}

		msg = pDev->rxFrames[P2P_FID_BEACON]->msg;

		mb.Buffer = (u1Byte *)pDev;
		mb.Length = sizeof(*pDev);
		PlatformIndicateP2PEvent(pP2PInfo, P2P_EVENT_DEV_FOUND, &mb);
		
		if(!p2p_validate_Beacon(msg))
		{
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}

		// Update Managed P2P Device Info.
		p2pUpdateBssDescManageability(pP2PInfo->pAdapter, Frame_pBssid(*posMpdu), msg);

		// Parse all Sub IEs
		pP2PDeviceDesc = P2PProcessSubElements(pP2PInfo, pRfd, msg->devAddr, msg, TRUE);
		if(pP2PDeviceDesc == NULL)
		{
			RT_TRACE_F(COMP_P2P, DBG_TRACE, 
				("reject because of invalid P2P Sub IE or the corresponding entry is not in the scan list\n"));
			break;
		}

		if(msg->_noa)
			P2POnP2PNoticeOfAbsence(pP2PInfo, *posMpdu, msg);

		// Update the device lists --------------------------------------------------------------------------
		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			MEMORY_BUFFER	mbPacket = {NULL, 0};
			mbPacket.Buffer = posMpdu->Octet;
			mbPacket.Length = posMpdu->Length;
			P2PDeviceListUpdate(pP2PInfo, &pP2PInfo->DeviceList, pRfd, msg->bssid, msg->devAddr, mbPacket);
		}

		// Wait for join
		if(pP2PInfo->DeviceDiscoverabilityContext.bWaitingBeaconFromGO && 
			pP2PInfo->State == P2P_STATE_DEVICE_DISCOVERABILITY_WAIT_BEACON &&
			eqMacAddr(pP2PInfo->DeviceDiscoverabilityContext.GoBssid, msg->bssid))
		{
			if(p2p_FindClientInfoByDevAddr(pP2PInfo->DeviceDiscoverabilityContext.ClientDeviceAddress, msg, NULL))
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("going to send DeviceDiscoverabilityReq to the GO\n"));

				pP2PInfo->DeviceDiscoverabilityContext.bWaitingBeaconFromGO = FALSE;
				
				pP2PInfo->State = P2P_STATE_DEVICE_DISCOVERABILITY_REQ_SEND;
				PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
				PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);	
			}
		}
	}while(FALSE);

	p2p_DevList_Unlock(&pP2PInfo->devList);

	return rtStatus;
}

BOOLEAN
P2PHasNon11BRateAsSupportingRate(
	IN OCTET_STRING osPacket
	)
{
	OCTET_STRING BratesBeacon, ExtratesBeacon;
	BOOLEAN bHasNon11bRate = FALSE;
	u1Byte i;

	//
	// Note: if no supported rate IE in osPacket, FALSE is returned.
	//
	
	// Check Supported rate
	BratesBeacon = PacketGetElement(osPacket, EID_SupRates, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
	if( BratesBeacon.Length != 0)
	{
		for(i = 0; i < BratesBeacon.Length; i++)
		{
			if(!IS_CCK_RATE((BratesBeacon.Octet)[i]))
			{
				bHasNon11bRate = TRUE;
				break;
			}
		}
	}

	if(!bHasNon11bRate)
	{
		// 19.Extended supported rates
		ExtratesBeacon = PacketGetElement(osPacket, EID_ExtSupRates, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE );
		if( ExtratesBeacon.Length != 0)
		{
			for(i = 0; i < ExtratesBeacon.Length; i++)
			{
				if(!IS_CCK_RATE((ExtratesBeacon.Octet)[i]))
				{
					bHasNon11bRate = TRUE;
					break;
				}
			}
		}
	}
	
	return bHasNon11bRate;
}

BOOLEAN
P2PAcceptProbeReq(
	IN PP2P_INFO pP2PInfo,
	IN const OCTET_STRING *posMpdu,
	IN const P2P_MESSAGE *msg
	)
{
	BOOLEAN bRet = FALSE;
	//OCTET_STRING osSsid = PacketGetElement(osPacket, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUB_DONT_CARE);
	
	//
	// Check if we shall respond to the ProbeReq.
	// Note that when in Op mode acting as GO, 
	// ProbeReq is not dealt with here.
	//
	do
	{
		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			// Win8: Device port discoverability checking ------------------------------------------------------------
			if(GetFirstDevicePort(pP2PInfo->pAdapter) == pP2PInfo->pAdapter)
			{
				if(GET_P2P_INFO(GetFirstDevicePort(pP2PInfo->pAdapter))->uListenStateDiscoverability == 0)
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("Reject Probe Request: Device Port: pP2PInfo->uListenStateDiscoverability == 0\n"));
					break;
				}
			}
		}
		
		if(pP2PInfo->State == P2P_STATE_OPERATING)
		{
			if(P2P_ACTING_AS_GO(pP2PInfo))
			{
				// check requested device type of the WPS IE matched
				if(!P2PDeviceTypeMatches(pP2PInfo, msg->wpsAttributes.os))
					{//DbgPrint("i\n");
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2PDeviceTypeMatches\n"));
						break;
					}
					
					// check Device ID matched
				if(!P2PDeviceIDMatches(pP2PInfo, msg))
					{//DbgPrint("j\n");
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2PDeviceIDMatches\n"));					
						break;
					}
				}
			else 
			{// operating but not GO => client
				if(FALSE == P2PSvc_Enabled(pP2PInfo->pP2PSvcInfo))
				{// client does not reply to a ProbeReq, unless P2PSvc is enabled
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: operating but not GO\n"));
					break; 
			}
		}
		}
		else if(pP2PInfo->State == P2P_STATE_INITIALIZED)
		{
			// don't reject
		}
#if 0
		else if(pP2PInfo->State == P2P_STATE_INITIALIZED && 
			pP2PInfo->bExtendedListening)
		{// doing extended listen
			// don't reject
		}
#endif
		else if(pP2PInfo->State < P2P_STATE_DEV_DISC_START ||
			pP2PInfo->State > P2P_STATE_DEV_DISC_COMPLETE)
		{// only accept probe req when doing dev discovery, note that probe req of GO is dealt with by ConstructProbeRsp().
			// Comment out because we hope to reduce extended listen timing while not to degrade the responsiveness
			//break;
		}
		
		//
		// Clause 3.1.2.1.2
		//
		if(pP2PInfo->State == P2P_STATE_SCAN) 
		{//DbgPrint("a\n");
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2P_STATE_SCAN\n"));								
		
			break;
		}
		if(pP2PInfo->Role == P2P_CLIENT) 
		{//DbgPrint("b\n");
			if(FALSE == P2PSvc_Enabled(pP2PInfo->pP2PSvcInfo))
			{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2P_CLIENT\n"));										
			break;
		}
		}
		//if(osP2PIE.Length != 0 && P2P_WILDCARD_SSID(osSsid) && pP2PInfo->Role != P2P_DEVICE)
		//{// the ProbeReq is intended only for P2P Devices
		//DbgPrint("c\n");
		//	break;
		//}

		//
		// Clause 3.1.2.1.3
		//
		if( pP2PInfo->State == P2P_STATE_SEARCH) 
		{//DbgPrint("d\n");
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2P_STATE_SEARCH\n"));												
			break;
		}

		//
		// Clause 3.1.2.1.1
		//
		if(pP2PInfo->State == P2P_STATE_LISTEN || pP2PInfo->State == P2P_STATE_INITIALIZED) 
		{
			// This has already been checked.
			//if(osP2PIE.Length == 0) 
			//{// no P2P IE
			//DbgPrint("e\n");
			//	break;
			//}
			if(!P2P_WILDCARD_MAC_ADDR(msg->bssid))
			{//DbgPrint("g\n");
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2P_WILDCARD_MAC_ADDR pBssid\n"));												
			
				break;
			}
			if(!(P2P_WILDCARD_MAC_ADDR(msg->da) || eqMacAddr(msg->da, pP2PInfo->DeviceAddress)))
			{//DbgPrint("h\n");
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2P_WILDCARD_MAC_ADDR pDaddr\n"));												
			
				break;
			}

			// check requested device type of the WPS IE matched
			if(!P2PDeviceTypeMatches(pP2PInfo, msg->wpsAttributes.os))
			{//DbgPrint("i\n");
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2PDeviceTypeMatches case 2\n"));												
			
				break;
			}
			
			// check Device ID matched
			if(!P2PDeviceIDMatches(pP2PInfo, msg))
			{//DbgPrint("j\n");
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2PDeviceIDMatches case 2\n"));												
			
				break;
			}
		}

		//
		// Clause 2.4.1: P2P Devices shall not respond to Probe Request frames that indicate support for 11b rates only.
		//
		if(!P2PHasNon11BRateAsSupportingRate(*posMpdu))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reject Probe Request: P2PHasNon11BRateAsSupportingRate \n"));												
		
			break;
		}

		bRet = TRUE;
	} while(FALSE);

	return bRet;
}

/*
BOOLEAN
P2PToSendProvisionDiscoveryReq(
	IN u2Byte ConfigMethod
	)
{
	//
	// If peer's ConfigMethods is Display, Keypad or PBC, send ProvisionDiscoveryReq.
	//

	BOOLEAN bRet = FALSE;
	
	switch(ConfigMethod)
	{
		case P2P_WPS_CONFIG_METHODS_LABEL:
			break;
		case P2P_WPS_CONFIG_METHODS_DISPLAY:
			bRet = TRUE;
			break;
		case P2P_WPS_CONFIG_METHODS_PUSHBUTTON:
			bRet = TRUE;
			break;
		case P2P_WPS_CONFIG_METHODS_KEYPAD:
			bRet = TRUE;
			break;
		default:
			break;
	}

	return bRet;
}


WPS_CONFIG_METHODS
P2PDetermineConfigMethodToUse(
	IN WPS_CONFIG_METHODS PeerConfigMethod
	)
{
	WPS_CONFIG_METHODS RetValue = 0xFF; // an impossible value

	switch(PeerConfigMethod)
	{
		case P2P_WPS_CONFIG_METHODS_LABEL: // use peer's label
			RetValue = P2P_WPS_CONFIG_METHODS_KEYPAD;
			break;
		case P2P_WPS_CONFIG_METHODS_DISPLAY: // use peer's  displayed PIN
			RetValue = P2P_WPS_CONFIG_METHODS_KEYPAD;
			break;
		case P2P_WPS_CONFIG_METHODS_KEYPAD: // peer will enter my PIN
			RetValue = P2P_WPS_CONFIG_METHODS_DISPLAY;
			break;
		case P2P_WPS_CONFIG_METHODS_PUSHBUTTON: // PBC
			RetValue = P2P_WPS_CONFIG_METHODS_PUSHBUTTON;
			break;
		default:
			break;
	}

	return RetValue;
}
*/

BOOLEAN
P2PDetermineDevPasswdIdCompatible(
	IN u2Byte PeerDevicePasswdID,
	IN u2Byte SelfDevicePasswdID
	)
{
	WPS_DEVICE_PASSWD_ID selfDpid = (WPS_DEVICE_PASSWD_ID)SelfDevicePasswdID;
	WPS_DEVICE_PASSWD_ID peerDpid = (WPS_DEVICE_PASSWD_ID)PeerDevicePasswdID;
	
	switch(selfDpid)
	{
		case P2P_WPS_DEV_PASSWD_ID_DEFAULT:
			//
			// Marked: Win8 GO Negotiation: Accept all since the Listener is at this state when in initialization
			//if(PeerDevicePasswdID != P2P_WPS_DEV_PASSWD_ID_DEFAULT)
			//{
			//	return FALSE;
			//}
			//
			break;
		case P2P_WPS_DEV_PASSWD_ID_USER_SPEC:
			if(peerDpid != P2P_WPS_DEV_PASSWD_ID_DEFAULT &&
				peerDpid != P2P_WPS_DEV_PASSWD_ID_REG_SPEC)
			{
				return FALSE;
			}
			break;
		case P2P_WPS_DEV_PASSWD_ID_PBC:
			if(peerDpid != P2P_WPS_DEV_PASSWD_ID_PBC)
			{
				return FALSE;
			}
			break;
		case P2P_WPS_DEV_PASSWD_ID_REG_SPEC:
			if(peerDpid != P2P_WPS_DEV_PASSWD_ID_USER_SPEC)
			{
				return FALSE;
			}
			break;
		case P2P_WPS_DEV_PASSWD_ID_WFDS_DEFAULT_PIN:
			//if(peerDpid != P2P_WPS_DEV_PASSWD_ID_WFDS_DEFAULT_PIN)
			//{
			//	return FALSE;
			//}
			return TRUE;
			break;
		default:
			return FALSE;
			break;
	}

	return TRUE;
}

BOOLEAN
P2PDetermineGONegoDevPasswdIdCompaible(
	IN u2Byte selfDpid,
	IN u2Byte peerDpid
	)
{
	//
	// Check Device Password ID compatibility
	//
	RT_TRACE(COMP_P2P, DBG_LOUD, 
			("P2PDetermineGONegoDevPasswdIdCompaible(): recvd Dev Passwd ID (%u), mine (%u)\n",
			peerDpid, selfDpid));
	
	if(!P2PDetermineDevPasswdIdCompatible(peerDpid, selfDpid))
	{// dev passwd id incompatible
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("P2PDetermineGONegoDevPasswdIdCompaible(): DevPasswdId Incompatible\n"));
		return FALSE;
	}

	return TRUE;
}

//
// Description:
//	Handle Probe Request packet for P2P function.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_PKT_DROP if this packet is handled by P2P and can be droppd.
//	If the returned status is  RT_STATUS_SUCCESS, the next normal process should be executed.
// Remark:
//	When the current state is P2P Go or AP mode, the probe request is handled by AP mode.
//	This function only handles the probe request when the current state is P2P device mode.
//
RT_STATUS
P2P_OnProbeReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL;
	OCTET_STRING			osWpsIE = PacketGetElement(*posMpdu, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUB_DONT_CARE);
	BOOLEAN					bHandled = FALSE;
	P2P_ROLE				p2pRole = pP2PInfo->Role;

	P2P_DEV_LIST_ENTRY		*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	// If the state is AP mode or IBSS mode, we don't handle probe request here.
	if(pAdapter == GetFirstGOPort(pAdapter) ||
		pMgntInfo->mIbss && !pMgntInfo->mDisable && pMgntInfo->bIbssCoordinator)
	{
		//RT_TRACE(COMP_P2P, DBG_LOUD, ("This pkt will be handled by GO!\n"));
		return RT_STATUS_SUCCESS;
	}

	if(P2P_DEVICE != pP2PInfo->Role)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("Return because Not Device port!\n"));		
		return RT_STATUS_SUCCESS;		
	}
		
	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_PROBE_REQ, 
						pRfd, Frame_pSaddr(*posMpdu), 
						RT_GetChannelNumber(pP2PInfo->pAdapter), &pDev))
			)
		{
			// RT_TRACE(COMP_P2P, DBG_LOUD, ("Failed (0x%08X) from p2p_DevList_RxUpdate!\n", rtStatus));
			break;
		}

		msg = pDev->rxFrames[P2P_FID_PROBE_REQ]->msg;
		
		if(!p2p_validate_ProbeReq(msg))
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("RT_STATUS_MALFORMED_PKT!\n"));
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}
		
		if(!P2PAcceptProbeReq(pP2PInfo, posMpdu, msg))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("rejected by P2PAcceptProbeReq()\n"));
			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Transmitter Address: ", Frame_pTaddr(*posMpdu));
			RT_TRACE(COMP_P2P, DBG_LOUD, ("pP2PInfo->State: %d\n", pP2PInfo->State));		
			break;
		}

		if(P2P_DEVICE == p2pRole
			&& pP2PInfo->State > P2P_STATE_GO_NEGO_COMPLETE 
			&& FALSE == P2PSvc_Enabled(pP2PInfo->pP2PSvcInfo)
			)
		{// when operating, ProbeReq is handled in OnProbeReq(), not here.
			break;
		}

		if(pP2PInfo->bExtendedListening)
		{
			//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("recv ProbeReq in Extended Listen\n"));
			//RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "P2P_OnProbeReq(): recv ProbeReq in Extended Listen\n", DeviceAddress);
		}

		pP2PDeviceDesc = P2PProcessSubElements(pP2PInfo, pRfd, msg->sa, msg, TRUE);

		bHandled = TRUE;
	}while(FALSE);

	// Debug Device Discovery ------------------------------------------------------------
	pP2PInfo->ProbeRequestSequenceNum = (u2Byte) Frame_SeqNum(*posMpdu);	// 12 bits.
	// --------------------------------------------------------------------------------


	RT_TRACE_F(COMP_P2P, DBG_TRACE, ("PlatformGetCurrentTime(): %d\n",  (u4Byte)PlatformGetCurrentTime()));
	RT_TRACE_F(COMP_P2P, DBG_TRACE, ("pDevicePortP2PInfo->TimeStartToStopSendingProbeResponse: %d\n", (u4Byte)pP2PInfo->TimeStartToStopSendingProbeResponse));

	//
	// Send ProbeRsp
	if(bHandled)
	{
		if(pP2PInfo->TimeStartToStopSendingProbeResponse == 0 || (s8Byte) (PlatformGetCurrentTime()) - pP2PInfo->TimeStartToStopSendingProbeResponse < 0)
		{
			p2p_Send_ProbeRsp(pP2PInfo, msg->sa, pRfd, posMpdu, &msg->p2pAttributes.os);

			//
			// After sending probe request, return "RT_STATUS_PKT_DROP" to notify that this pakcet was handled
			// here and it must be dropped.
			//
			rtStatus = RT_STATUS_PKT_DROP;

		}
		else if((s8Byte)(PlatformGetCurrentTime()) - pP2PInfo->TimeStartToStopSendingProbeResponse > 2 * P2P_RESERVED_TIME_FOR_ACTION_FRAME_MS * 1000)
		{
			RT_TRACE_F(COMP_P2P, DBG_TRACE, ("Reset pP2PInfo->TimeStartToStopSendingProbeResponse: from %d to 0\n", (u4Byte)pP2PInfo->TimeStartToStopSendingProbeResponse));	
			pP2PInfo->TimeStartToStopSendingProbeResponse = 0;		
			rtStatus = RT_STATUS_PKT_DROP;
		}
		else
		{
			RT_TRACE_F(COMP_P2P, DBG_TRACE, ("No probe rsp\n"));	
		}
	}

	p2p_DevList_Unlock(&pP2PInfo->devList);

	return rtStatus;
}

//
// Description:
//	Handle probe response packet for P2P function.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	This function updates the P2P scan list and check the WPS state for join.
//
RT_STATUS
P2P_OnProbeRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL;

	P2P_DEV_LIST_ENTRY		*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;

	MEMORY_BUFFER			mb = {NULL, 0};

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) && pP2PInfo->bForceScanLegacyNetworks)
	{
		MEMORY_BUFFER	mbPacket = {NULL, 0};
		mbPacket.Buffer = posMpdu->Octet;
		mbPacket.Length = posMpdu->Length;
		P2PDeviceListUpdate(pP2PInfo, &pP2PInfo->DeviceList, pRfd, Frame_pBssid(*posMpdu), NULL, mbPacket);
	}	

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		// RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "P2P_OnProbeRsp(): ", Frame_pSaddr(*posMpdu));
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_PROBE_RSP, 
						pRfd, Frame_pBssid(*posMpdu), 
						RT_GetChannelNumber(pP2PInfo->pAdapter), &pDev))
			)
		{
			break;
		}

		msg = pDev->rxFrames[P2P_FID_PROBE_RSP]->msg;

		mb.Buffer = (u1Byte *)pDev;
		mb.Length = sizeof(*pDev);
		PlatformIndicateP2PEvent(pP2PInfo, P2P_EVENT_DEV_FOUND, &mb);
		
		if(!p2p_validate_ProbeRsp(msg))
		{
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}

		// Update Managed P2P Device Info.
		p2pUpdateBssDescManageability(pP2PInfo->pAdapter, msg->bssid, msg);

		if(NULL == (pP2PDeviceDesc = P2PProcessSubElements(pP2PInfo, pRfd, msg->devAddr, msg, TRUE)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Reject because of invalid P2P Sub IE\n"));
			break;
		}

		if(msg->_noa)
			P2POnP2PNoticeOfAbsence(pP2PInfo, *posMpdu, msg);
		
		// Update the device lists --------------------------------------------------------------------------
		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			MEMORY_BUFFER	mbPacket = {NULL, 0};
			mbPacket.Buffer = posMpdu->Octet;
			mbPacket.Length = posMpdu->Length;
			P2PDeviceListUpdate(pP2PInfo, &pP2PInfo->DeviceList, pRfd, msg->bssid, msg->devAddr, mbPacket);
		
		
			if(pP2PInfo->ScanDeviceIDs.uNumOfDeviceIDs > 0)
			{ // Finding some device now
				if(eqMacAddr(msg->devAddr, pP2PInfo->ScanDeviceIDs.DeviceIDs[0]))
				{ // Device is Found: Complete the Scan Immediately for speeding up GO Negotiation
					RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] cease scan for dev id matched\n"));
					p2p_DevList_Unlock(&pP2PInfo->devList);
					P2PScanListCeaseScan(pP2PInfo);
					p2p_DevList_Lock(&pP2PInfo->devList);
				}
			}
		}
		// ----------------------------------------------------------------------------------------------

		//
		// In this case, we are in pre GO Nego phase. We will start a GO Nego procedure.
		//
		if(P2PCommonChannelArrived(pP2PInfo, pDev, *posMpdu))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("common channel arrived\n"));
			//return;
		}

		if(pP2PInfo->DeviceDiscoverabilityContext.bWaitingBeaconFromGO && 
			pP2PInfo->State == P2P_STATE_DEVICE_DISCOVERABILITY_WAIT_BEACON &&
			eqMacAddr(pP2PInfo->DeviceDiscoverabilityContext.GoBssid, msg->bssid))
		{
			if(p2p_FindClientInfoByDevAddr(pP2PInfo->DeviceDiscoverabilityContext.ClientDeviceAddress, msg, NULL))
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("going to send DeviceDiscoverabilityReq to the GO\n"));

				pP2PInfo->DeviceDiscoverabilityContext.bWaitingBeaconFromGO = FALSE;
				
				pP2PInfo->State = P2P_STATE_DEVICE_DISCOVERABILITY_REQ_SEND;
				PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
				PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);	
			}
		}

		P2PSvc_OnProbeRsp(pP2PInfo->pP2PSvcInfo, msg->devAddr, &msg->p2pAttributes.os);
	}while(FALSE);

	p2p_DevList_Unlock(&pP2PInfo->devList);

	return rtStatus;
}

//
// Description:
//	Handle association packet for P2P function.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	This function is called after this client is determined successfully associated with our AP mode.
//	This function checks the P2P IEs from the client and update it to the client list if it is a p2p client.
// By Bruce, 2012-03-12.
//
RT_STATUS
P2P_OnAssocReqAccept(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	pu1Byte					InterfaceAddress = Frame_pSaddr(*posMpdu);
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL;
	PRT_WLAN_STA			pAsocEntry = NULL;

	PMGNT_INFO pExtMgntInfo = &(pAdapter->MgntInfo);

	P2P_MESSAGE				msg;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	do
	{
		if(RT_STATUS_SUCCESS == (rtStatus = p2p_parse_Ies(posMpdu, DBG_LOUD, &msg)))
			if(!p2p_validate_AssocReq(&msg))
			{
				rtStatus = RT_STATUS_MALFORMED_PKT;
				break;
			}
		
		if(NULL == (pP2PDeviceDesc = P2PProcessSubElements(pP2PInfo, pRfd, msg.devAddr, &msg, TRUE)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] reject because of invalid P2P Sub IE\n"));
			break;
		}

		pAsocEntry = AsocEntry_GetEntry(&(pAdapter->MgntInfo), InterfaceAddress);
		if(pAsocEntry)
		{
			pAsocEntry->bP2PClient = TRUE;
			cpMacAddr(pAsocEntry->P2PClientInfoDesc.InterfaceAddress, pP2PDeviceDesc->IntendedP2PInterfaceAddress);
			cpMacAddr(pAsocEntry->P2PClientInfoDesc.DeviceAddress, pP2PDeviceDesc->DeviceAddress);
			pAsocEntry->P2PClientInfoDesc.DeviceCapability = pP2PDeviceDesc->DeviceCapability;
			PlatformMoveMemory(&pAsocEntry->P2PClientInfoDesc.WpsAttributes, &(pP2PDeviceDesc->WpsAttributes), sizeof(P2P_WPS_ATTRIBUTES));
			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "P2P_OnAssocReqAccept(): Copy into list with client = ", pAsocEntry->P2PClientInfoDesc.InterfaceAddress);
		}
	}while(FALSE);

	p2p_parse_FreeMessage(&msg);

	FunctionOut(COMP_P2P);

	return rtStatus;
}

//
// Description:
//	Handle deauth/disassoc packet in client mode for P2P function.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	This function updates the P2P capability from the reason code.
//
RT_STATUS
P2P_ClientOnDeauth(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);

	P2P_MESSAGE				msg;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	FunctionIn(COMP_P2P);

	do
	{
		if(RT_STATUS_SUCCESS == (rtStatus = p2p_parse_Ies(posMpdu, DBG_LOUD, &msg)))
			if(!p2p_validate_Deauth(&msg))
			{
				rtStatus = RT_STATUS_MALFORMED_PKT;
				break;
			}
		
		// TODO: we may check whether this is from an AP (either WLAN AP or P2P GO) that has manageability attribute

		if(msg._minorReasonCode)
		{// has valid minor reason code
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("minor rsn code: %u\n", msg.minorReasonCode));
			
			switch(msg.minorReasonCode)
			{
				default: // Reserved
					break;
					
				case P2P_MINOR_RESON_CROSS_CONNECTION:
					pP2PInfo->bWlanApRejection_CrossConnection = TRUE;
					
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("disable cross connection\n"));
					pP2PInfo->GroupCapability &= ~gcCrossConnection; // disable cross connection

					//
					// After disable cross connection, we shall reconnect to the wlan AP 
					// without corss connection bit set.
					//
					
					break;
					
				case P2P_MINOR_RESON_MANAGED_BIT: 
					if(!(pP2PInfo->DeviceCapability & dcP2PInfrastructureManaged))
					{
						pP2PInfo->bWlanApRejection_Unmanaged = TRUE;

						RT_TRACE_F(COMP_P2P, DBG_LOUD, 
							("disable P2P mode because of minor reason code 2 => should not send any P2P IE in ProbeReq and (Re)AssocReq\n"));
						MgntActSet_P2PMode(pP2PInfo->pAdapter, FALSE, FALSE, 1, 1, 1);
					}
					else
					{
						RT_TRACE_F(COMP_P2P, DBG_LOUD, 
							("Receive deauth with minor rsn code 2 when managed P2P is enabled => ignore it\n"));

					}
					break;
					
				case P2P_MINOR_RESON_CONCURRENT_COEXISTENCE:
					if(pP2PInfo->DeviceCapability & dcP2PInfrastructureManaged)
					{
						pu1Byte			pBssid = Frame_pBssid(*posMpdu);
						PRT_WLAN_BSS	pBssDesc = NULL;
						pP2PInfo->bWlanApRejection_IncompatibleCoexistenceParameters = TRUE;
		
						if(pP2PInfo->Role != P2P_GO)
							break;

						PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_SCAN_SPINLOCK);
						pBssDesc = BssDescDupByBssid(pP2PInfo->pAdapter, pBssid);

						if(pBssDesc)
						{
							P2PUpdateWlanApManagedInfo(pP2PInfo, pBssDesc);
						}

						PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_SCAN_SPINLOCK);
						//RT_TRACE_F(COMP_P2P, DBG_LOUD, 
						//	("disable P2P mode because of minor reason code 3\n"));
						//P2PDisconnect(pP2PInfo);
						//MgntActSet_P2PMode(pP2PInfo->pAdapter, FALSE, FALSE, 1, 1, 1);
					}
					break;
					
				case P2P_MINOR_RESON_INFRASTRUCTURE_MANAGED_BIT: 
					if(pP2PInfo->DeviceCapability & dcP2PInfrastructureManaged)
					{
						pP2PInfo->bWlanApRejection_IncompatibleP2POperation = TRUE;

						RT_TRACE_F(COMP_P2P, DBG_LOUD, 
							("disable P2P mode because of minor reason code 4 and I'm a managed P2P Device\n"));
						MgntActSet_P2PMode(pP2PInfo->pAdapter, FALSE, FALSE, 1, 1, 1);
					}
					else
					{
						RT_TRACE_F(COMP_P2P, DBG_LOUD, 
							("ignore the deauth with minor reason code 4 bceause I'm NOT a managed P2P Device\n"));

					}
					break;
			}
		}
	}while(FALSE);

	p2p_parse_FreeMessage(&msg);

	return rtStatus;
}

//
// Description:
//	Handle deauth/disassoc packet in client mode for P2P function.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	This function updates the P2P capability from the reason code.
//
RT_STATUS
P2P_ClientOnDisassoc(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	return P2P_ClientOnDeauth(pAdapter, pRfd, posMpdu);
}

//
// Description:
//	Handle P2P Go Negotiation Reqeust packet for P2P function.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnGONReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PMGNT_INFO				pMgntInfo = &pAdapter->MgntInfo;	
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL;
	int						go;
	P2P_STATE				CurrentState = pP2PInfo->State;	// For bakcup state when neg failed.

	P2P_DEV_LIST_ENTRY		*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;
	

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	FunctionIn(COMP_P2P);

	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2P_OnGONReq():\n", posMpdu->Octet, posMpdu->Length);

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_GO_NEG_REQ, 
				pRfd, Frame_pSaddr(*posMpdu), 
				pP2PInfo->ListenChannel, &pDev))
			)
		{
			break;
		}
				
		msg = pDev->rxFrames[P2P_FID_GO_NEG_REQ]->msg;
		
		if(!p2p_validate_GoNegReq(msg))
		{
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}

		pP2PDeviceDesc = P2PProcessSubElements(pP2PInfo, pRfd, msg->devAddr, msg, TRUE);
		if(pP2PDeviceDesc == NULL)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] reject because of invalid P2P Sub IE\n"));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		//
		// Check state
		//
		
		if(pP2PInfo->Role == P2P_CLIENT)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Receive GoReq when we are client\n"));
			P2PIndicateDeviceReqClientGoFormation(pP2PInfo, pP2PDeviceDesc);
			pP2PDeviceDesc->Status = P2P_STATUS_FAIL_REJECTED_BY_USER;
			pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
			goto exit_P2POnGONReq;			
		}
		else if(!P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter)
			&& pP2PInfo->State > P2P_STATE_DEV_DISC_COMPLETE)
		{
			RT_TRACE(COMP_P2P, DBG_WARNING, ("[WARNING] recv in an invalid state: %u\n", pP2PInfo->State));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		//
		// v1.07:
		// If a Status attribute is present in the received GO Negotiation Request frame, 
		// the request is being used to signal user rejection of an earlier GO Negotiation Request 
		// sent by the receiving P2P Device and shall be discarded with no further action.
		//
		if(msg->_status)
		{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, 
					("[WARNING] has Status attr in GONegoReq, Status: %u\n", pP2PInfo->State));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
		
		// Copy device descriptor
		PlatformMoveMemory(&pP2PInfo->ConnectionContext.ConnectingDevice, 
			pP2PDeviceDesc, 
			sizeof(P2P_DEVICE_DISCRIPTOR));

		// Reject overllapped GONego Req
		if(P2PIsDoingGroupFormation(pP2PInfo) &&
			!cpMacAddr(pP2PInfo->ConnectionContext.ConnectingDevice.DeviceAddress, msg->sa))
		{// not to update the state machine
			pP2PDeviceDesc->Status = P2P_STATUS_FAIL_UNABLE_TO_ACCOMODATE_REQUEST;
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] reject overllaped GONego Req\n"));
			goto exit_P2POnGONReq;
		}

		pP2PInfo->State = P2P_STATE_GO_NEGO_REQ_RECVD;
		
		if( P2P_DOING_DEVICE_DISCOVERY(pP2PInfo) )
		{
			p2p_DevList_Unlock(&pP2PInfo->devList);
			P2PScanListCeaseScan(pP2PInfo);
			p2p_DevList_Lock(&pP2PInfo->devList);
			P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
		}

		// Determine GO
		go = p2p_go_det(pP2PInfo->GOIntent, pP2PDeviceDesc->GOIntent);
		if(go) 
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("I'm going to be the GO\n"));
			
			pP2PInfo->ConnectionContext.bGoingToBeGO = TRUE;

			// Determine GO SSID
			P2PDetermineGOSsid(pP2PInfo, pP2PInfo->SSIDBuf, &pP2PInfo->SSIDLen);	
		}
		else if(0 == go)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("the target device is going to be the GO\n"));
			pP2PInfo->ConnectionContext.bGoingToBeGO = FALSE;
		} 
		else
		{// shall update the state machine
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] P2P_STATUS_FAIL_BOTH_P2P_DEVICES_INDICATED_GO_INTENT_OF_15\n"));
			pP2PDeviceDesc->Status = P2P_STATUS_FAIL_BOTH_P2P_DEVICES_INDICATED_GO_INTENT_OF_15;
			pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
			goto exit_P2POnGONReq;
		}

		// Check Device Password ID compatibility
		{	
			if(P2P_ADAPTER_RTK_SUPPORT_P2P(pAdapter) &&
				!P2PDetermineGONegoDevPasswdIdCompaible(pP2PInfo->WpsDevPasswdId, msg->wpsDevPasswordId))
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Fail information is unavailable\n"));
				pP2PDeviceDesc->Status = P2P_STATUS_FAIL_INFORMATION_IS_UNAVAILABLE;
				pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;

				//
				// Check if peer is waiting for our user input
				//
				if(msg->wpsDevPasswordId == P2P_WPS_DEV_PASSWD_ID_PBC || 
					msg->wpsDevPasswordId == P2P_WPS_DEV_PASSWD_ID_DEFAULT ||
					msg->wpsDevPasswordId == P2P_WPS_DEV_PASSWD_ID_REG_SPEC)
				{
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Start waiting for user input for 2 min\n"));
					pP2PInfo->bReinitiateConnection = TRUE;
					cpMacAddr(pP2PInfo->DevAddrToReConnect, msg->devAddr);
					pP2PInfo->TimeStartWaitingForReinitiate = PlatformGetCurrentTime();
				}
				
				goto exit_P2POnGONReq;
			}
		}
		
		// Check if channel is compatible
		if(pP2PInfo->ConnectionContext.bGoingToBeGO)
		{			
			p2p_Channel_Intersect(&pP2PInfo->ChannelEntryList, 
				&pDev->p2p->channels, 
				&pDev->p2p->commonChannels);

			if(pDev->p2p->commonChannels.regClasses)
			{ // Has Common Channel
				// Check if our op channel is one of channels supported by peer
				if(!p2p_Channel_InChannelEntryList(pP2PInfo->OperatingChannel, &pDev->p2p->commonChannels))
				{
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("going to be GO, change op ch from: %d to %d\n", 
						pP2PInfo->OperatingChannel,
						pDev->p2p->commonChannels.regClass[0].channel[0]));
					pP2PInfo->OperatingChannel = pDev->p2p->commonChannels.regClass[0].channel[0]; // alwyas use the first one
				}
				
			}
			// Win8 Go Negotiation Trick -----------------------------------------------------------------------------------------------------------------------
			else if(MgntLinkStatusQuery(GetDefaultAdapter(pP2PInfo->pAdapter)) == RT_MEDIA_CONNECT)
			{
				#if (MULTICHANNEL_SUPPORT == 1)	// For Compilation: Only PCI-E Support MultiChannel Now

				pP2PInfo->OperatingChannel = MultiChannelGetPortConnected20MhzChannel(GetDefaultAdapter(pP2PInfo->pAdapter));

				#endif
			
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("WHCK Test Trick: Going to be GO, change op ch to default port connected channel: %d\n", pP2PInfo->OperatingChannel));				
			}
			// ---------------------------------------------------------------------------------------------------------------------------------------------
			else // No Common Channel
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] no common channels\n"));
				RT_PRINT_DATA(COMP_P2P, DBG_WARNING, "[WARNING] peer supported op chnl:\n", 
					pP2PDeviceDesc->ChannelPlanChannel, 
					pP2PDeviceDesc->ChannelPlanLength);

				pP2PDeviceDesc->Status = P2P_STATUS_FAIL_NO_COMMON_CHANNELS;
				pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
				goto exit_P2POnGONReq;
			}
		}

		// After here, we shall send a GONRsp of status success
		pP2PDeviceDesc->Status = P2P_STATUS_SUCCESS;

		//
		// Fill ConnectionContext, note that bGoingToBeGO has already been filled
		//
		pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
		pP2PInfo->ConnectionContext.DialogToken = pP2PDeviceDesc->DialogToken;
		pP2PInfo->ConnectionContext.FindPhaseLoopTimes = P2P_SCAN_FIND_PHASE_LOOP_TIMES; // no use

		// ========== exit_P2POnGONReq ==========
exit_P2POnGONReq:
	
		if(MgntRoamingInProgress(pMgntInfo))
		{
			if(MgntResetOrPnPInProgress(pAdapter))
			{
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("reset in progress case 1\n"));											
				return RT_STATUS_FAILURE;
			}	
		
			MgntRoamComplete(pAdapter, MlmeStatus_refused);
		}
		// < Note> If failed, we shall not modify the original state machine state.
		if(P2P_STATUS_SUCCESS == pP2PDeviceDesc->Status)
			WFD_OnP2PActionFrame(pAdapter, pRfd, posMpdu);
		
		// If failed, indicate as early as possible
		if(pP2PDeviceDesc->Status != P2P_STATUS_SUCCESS &&
			FrameBuf_Length(&msg->wpsAttributes))
		{
			pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)pP2PInfo->ConnectionContext.Status;
			P2PIndicateGOFormatedInfo(pP2PInfo, 
				pP2PInfo->ConnectionContext.Status, 
				pP2PInfo->ConnectionContext.bGoingToBeGO, 
				&pP2PInfo->ConnectionContext.ConnectingDevice);
		}
		else
		{
			if(!P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter)) // use OffChnlTx, don't extend dwell time here
				CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), P2P_GO_NEGO_FRAME_TIMEOUT);
		}

		// For Win8: Indicate the reception of the GO negotiation request -------------------------
		//	Wait for the command of OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE
		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			//in case OID not set
			pP2PInfo->State = P2P_STATE_GO_NEGO_RSP_SEND;
		}
		else
		{
			// Send GONRsp
			pP2PInfo->Status = pP2PInfo->ConnectionContext.Status;

			p2p_Send_GoNegRsp(pP2PInfo, pP2PDeviceDesc->DeviceAddress,pP2PDeviceDesc->DialogToken);
		
			RT_TRACE_F(COMP_P2P, DBG_LOUD, 
				("send GORsp with status = %u, intent = %u, tie breaker: %u\n",
				pP2PInfo->ConnectionContext.Status,
				pP2PInfo->GOIntent >> 1,
				pP2PInfo->GOIntent & 0x01));

			// Update state machine
			if(pP2PInfo->ConnectionContext.Status == P2P_STATUS_SUCCESS)
			{
				pP2PInfo->State = P2P_STATE_GO_NEGO_RSP_SEND;
			}
			else if(!FrameBuf_Length(&msg->wpsAttributes))
			{
				pP2PInfo->State = CurrentState;
			}
			else if(pP2PInfo->Role != P2P_CLIENT)
			{
				pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
			}

			if( P2P_DOING_DEVICE_DISCOVERY(pP2PInfo) )
			{
				p2p_DevList_Unlock(&pP2PInfo->devList);
				P2PScanListCeaseScan(pP2PInfo);
				p2p_DevList_Lock(&pP2PInfo->devList);
				P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
			}
		}
	}while(FALSE);

	p2p_DevList_Unlock(&pP2PInfo->devList);

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		p2p_IndicateActionFrameReceived(pP2PInfo, 
			P2P_EVENT_RECEIVED_GO_NEGOTIATION_REQUEST, 
			RT_STATUS_SUCCESS, 
			posMpdu->Octet, 
			posMpdu->Length);
	}

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("<=== status: %u, next state: %u\n", pP2PInfo->ConnectionContext.Status, pP2PInfo->State));

	return rtStatus;
}

//
// Description:
//	Handle P2P Go Negotiation Response packet for P2P function.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnGONRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL; 
	int						go;

	P2P_DEV_LIST_ENTRY		*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	FunctionIn(COMP_P2P);
	
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2P_OnGONRsp():\n", posMpdu->Octet, posMpdu->Length);

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_GO_NEG_RSP, 
						pRfd, Frame_pSaddr(*posMpdu), 
						RT_GetChannelNumber(pP2PInfo->pAdapter), &pDev))
			)
		{
			break;
		}

		msg = pDev->rxFrames[P2P_FID_GO_NEG_RSP]->msg;
		
		if(!p2p_validate_GoNegRsp(pP2PInfo, msg))
		{
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}
		
		// Store the info in the P2P IE into pP2PDeviceDesc
		if(!P2PScanListFind(pP2PInfo->ScanList, pP2PInfo->ScanListSize, msg->devAddr, NULL, NULL))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("peer is not in our scan list\n"));
			P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
		}
		
		pP2PDeviceDesc = P2PProcessSubElements(pP2PInfo, pRfd, msg->devAddr, msg, FALSE);
		if(pP2PDeviceDesc == NULL)
		{	
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] reject because of invalid P2P Sub IE\n"));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(!P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter)
			&& pP2PInfo->State != P2P_STATE_GO_NEGO_RSP_WAIT)
		{// althoug incorrect state, we still record info form the GONegoRsp
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("[WARNING] our state (%d) not in P2P_STATE_GO_NEGO_RSP_WAIT state\n", pP2PInfo->State));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		//
		// Update ConnectionContext.ConnectingDevice since after P2PProcessSubElements,
		// its ChannelList or other elements may be modified.
		//
		PlatformMoveMemory(&pP2PInfo->ConnectionContext.ConnectingDevice, 
			pP2PDeviceDesc, 
			sizeof(P2P_DEVICE_DISCRIPTOR));

		// Compare DialogToken
		if(!P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter)
			&& pP2PDeviceDesc->DialogToken != pP2PInfo->ConnectionContext.DialogToken)
		{// Dialog Token mismatch
			RT_TRACE_F(COMP_P2P, DBG_WARNING, 
				("[WARNING] dialog token mismatch, recvd: %u, self: %u\n", 
					pP2PDeviceDesc->DialogToken,
					pP2PInfo->ConnectionContext.DialogToken));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
		else
		{
			pP2PInfo->ConnectionContext.DialogToken++;
		}

		if(pP2PDeviceDesc->Status != P2P_STATUS_SUCCESS)
		{// Group Formation ends on reception of a GONRsp with status other than success
			pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
			pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
			RT_TRACE_F(COMP_P2P, DBG_WARNING, 
				("[WARNING] status (%u) of the GONRsp is not success\n", pP2PDeviceDesc->Status));

			PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
			PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);

			if(RUNTIME_OS_WIN_FROM_WIN8(pP2PInfo->pAdapter))
			{
				break; // break with status success to indicate go neg rsp 
			}
				
			pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)pP2PInfo->ConnectionContext.Status;
			P2PIndicateGOFormatedInfo(pP2PInfo, 
				pP2PInfo->ConnectionContext.Status, 
				pP2PInfo->ConnectionContext.bGoingToBeGO, 
				&pP2PInfo->ConnectionContext.ConnectingDevice);
			
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		//
		// Reject overlapped GONego Req
		//
		if(P2PIsDoingGroupFormation(pP2PInfo) &&
			!cpMacAddr(pP2PInfo->ConnectionContext.ConnectingDevice.DeviceAddress, msg->sa))
		{// not to update the state machine
	 		pP2PDeviceDesc->Status = P2P_STATUS_FAIL_UNABLE_TO_ACCOMODATE_REQUEST;
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Reject overlapped GONego Req\n"));
			goto exit_P2POnGONRsp;
		}

		// Determine GO
		go = p2p_go_det(pP2PInfo->GOIntent, pP2PDeviceDesc->GOIntent);
		if(go) 
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("I'm going to be the GO\n"));
			
			pP2PInfo->ConnectionContext.bGoingToBeGO = TRUE;

			// Determine GO SSID
			P2PDetermineGOSsid(pP2PInfo, pP2PInfo->SSIDBuf, &pP2PInfo->SSIDLen);	
		}
		else if(0 == go)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, 
				("the target device is going to be the GO\n"));

			pP2PInfo->ConnectionContext.bGoingToBeGO = FALSE;
		} 
		else
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] P2P_STATUS_FAIL_BOTH_P2P_DEVICES_INDICATED_GO_INTENT_OF_15\n"));
			pP2PDeviceDesc->Status = P2P_STATUS_FAIL_BOTH_P2P_DEVICES_INDICATED_GO_INTENT_OF_15;
			pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
			goto exit_P2POnGONRsp;
		}

		// Check Device Password ID compatibility
		if(!P2PDetermineGONegoDevPasswdIdCompaible(pP2PInfo->WpsDevPasswdId, msg->wpsDevPasswordId))
		{
			pP2PDeviceDesc->Status = P2P_STATUS_FAIL_INCOMPATIBLE_PROVISION_METHOD;
			pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
			goto exit_P2POnGONRsp;
		}

		// Check if channel is compatible

		p2p_Channel_Intersect(&pP2PInfo->ChannelEntryList, 
			&pDev->p2p->channels, 
			&pDev->p2p->commonChannels);
		
		if(pP2PInfo->ConnectionContext.bGoingToBeGO)
		{
			if(pDev->p2p->commonChannels.regClasses) // use P2PMakeP2PChannelEntryList to test if we have common channels
			{
				// Check if our op channel is one of channels supported by peer
				if(!p2p_Channel_InChannelEntryList(pP2PInfo->OperatingChannel, &pDev->p2p->commonChannels))
				{
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("going to be GO, change op ch from: %d to %d\n", 
						pP2PInfo->OperatingChannel,
						pDev->p2p->commonChannels.regClass[0].channel[0]));
					// TODO: we may not support the 1st channel supported by peer
					pP2PInfo->OperatingChannel = pDev->p2p->commonChannels.regClass[0].channel[0]; // alwyas use the first one
				}
				
			}
			else // no common channel
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] no common channels: %p\n", &pP2PInfo->ConnectionContext.ConnectingDevice));
				RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ChnlList Recvd:\n", 
					pP2PDeviceDesc->ChannelPlanChannel, 
					pP2PDeviceDesc->ChannelPlanLength);
				pP2PDeviceDesc->Status = P2P_STATUS_FAIL_NO_COMMON_CHANNELS;
				pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
				goto exit_P2POnGONRsp;
			}
		}
		else
		{// Client
			if(pDev->p2p->commonChannels.regClasses) // use P2PMakeP2PChannelEntryList to test if we have common channels
			{
				// Common Channel found
			}
			else // No common channel
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] no common channel\n"));
				pP2PDeviceDesc->Status = P2P_STATUS_FAIL_NO_COMMON_CHANNELS;
				pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
				goto exit_P2POnGONRsp;
			}

			pP2PInfo->OperatingChannel = pP2PDeviceDesc->OperatingChannel;
		}

		// Fill ConnectionContext, note that bGoingToBeGO has already been filled
		pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
		pP2PInfo->ConnectionContext.DialogToken = pP2PDeviceDesc->DialogToken; // no use
		pP2PInfo->ConnectionContext.FindPhaseLoopTimes = P2P_SCAN_FIND_PHASE_LOOP_TIMES; // no use

	
exit_P2POnGONRsp:

		if(P2P_STATUS_SUCCESS == pP2PDeviceDesc->Status)
			WFD_OnP2PActionFrame(pAdapter, pRfd, posMpdu);
		//
		// If failed, indicate as early as possible
		//
		if(pP2PDeviceDesc->Status != P2P_STATUS_SUCCESS)
		{
			pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)pP2PInfo->ConnectionContext.Status;
			P2PIndicateGOFormatedInfo(pP2PInfo, 
				pP2PInfo->ConnectionContext.Status, 
				pP2PInfo->ConnectionContext.bGoingToBeGO, 
				&pP2PInfo->ConnectionContext.ConnectingDevice);
		}

		{
			BOOLEAN bSupportTxReport = FALSE;

			//
			// Send GONConfirm
			//
			if(FALSE == P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
			{
				pP2PInfo->Status = pP2PInfo->ConnectionContext.Status;
				p2p_Send_GoNegConfirm(pP2PInfo, pP2PDeviceDesc->DeviceAddress, pP2PDeviceDesc->DialogToken, &bSupportTxReport);

				//
				// Update state machine
				//
				if(bSupportTxReport)
				{
					pP2PInfo->msTimeWaitGoNegoConfirmSent = 0;
					pP2PInfo->State = P2P_STATE_GO_NEGO_CONFIRM_SENT_WAIT;
					PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
					PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				}
				else
				{
					pP2PInfo->msTimeWaitGoNegoConfirmSent = 0;
					pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
					PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
					PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 200); // in case P2P_STATE_GO_NEGO_COMPLETE, if w are GO, channel is switched to the op channel and the Confirm may be sent on the op channel instead of the listen channel
				}
			}
		}
	}while(FALSE); 

	p2p_DevList_Unlock(&pP2PInfo->devList);

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		p2p_IndicateActionFrameReceived(pP2PInfo, 
				P2P_EVENT_RECEIVED_GO_NEGOTIATION_RESPONSE, 
				RT_STATUS_SUCCESS, 
				posMpdu->Octet, 
				posMpdu->Length);
	}

	//P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("status: %u\n", pP2PInfo->ConnectionContext.Status));

	return rtStatus;
}

//
// Description:
//	Handle P2P Go Negotiation Confirm packet for P2P function.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnGONConfirm(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL;

	P2P_DEV_LIST_ENTRY		*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	FunctionIn(COMP_P2P);
	
	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2P_OnGONConfirm():\n", posMpdu->Octet, posMpdu->Length);

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_GO_NEG_CONF, 
						pRfd, Frame_pSaddr(*posMpdu), 
						RT_GetChannelNumber(pP2PInfo->pAdapter), &pDev))
			)
		{
			break;
		}
		
		msg = pDev->rxFrames[P2P_FID_GO_NEG_CONF]->msg;
		
		if(!p2p_validate_GoNegConfirm(pP2PInfo->ConnectionContext.bGoingToBeGO, msg))
		{
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}

		// Check state
		// Possible states are confirmWait and RspSend
		if(!OS_SUPPORT_WDI(pP2PInfo->pAdapter)
			&& !(pP2PInfo->State == P2P_STATE_GO_NEGO_CONFIRM_WAIT || pP2PInfo->State == P2P_STATE_GO_NEGO_RSP_SEND))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] state (%u) is not P2P_STATE_GO_NEGO_CONFIRM_WAIT or P2P_STATE_GO_NEGO_RSP_SEND\n", pP2PInfo->State));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		pP2PDeviceDesc = P2PProcessSubElements(pP2PInfo, pRfd, msg->sa, msg, FALSE);
		if(pP2PDeviceDesc == NULL)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("[WARNING] reject because of invalid P2P Sub IE\n"));
			rtStatus = RT_STATUS_INVALID_CONTEXT;
			break;
		}

		// Check DialogToken
		if(pP2PDeviceDesc->DialogToken != pP2PInfo->ConnectionContext.DialogToken)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, 
				("[WARNING] dialog token mismatch: mine(%u), peer(%u)\n",
				pP2PInfo->ConnectionContext.DialogToken,
				pP2PDeviceDesc->DialogToken));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
		else
		{
			pP2PInfo->ConnectionContext.DialogToken++;
		}

		if(pP2PDeviceDesc->Status != P2P_STATUS_SUCCESS)
		{	
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WRNING] status of the GONConfirm is not success\n"));
			pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
			pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;

			if(RUNTIME_OS_WIN_FROM_WIN8(pP2PInfo->pAdapter))
			{
				break; // break with status success to indicate go neg conf
			}
				
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		PlatformMoveMemory(&pP2PInfo->ConnectionContext.ConnectingDevice, 
							pP2PDeviceDesc, 
							sizeof(P2P_DEVICE_DISCRIPTOR));
		
		// Check if channel is compatible
		if(pP2PInfo->ConnectionContext.bGoingToBeGO)
		{
			// Check if our op channel is one of channels supported by peer
			if(!P2PIsChnlInChnlList(pP2PDeviceDesc->ChannelPlanChannel, 
				pP2PDeviceDesc->ChannelPlanLength, 
				pP2PInfo->OperatingChannel))
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("going to be GO, change op ch from: %d to %d\n", 
					pP2PInfo->OperatingChannel,
					pP2PDeviceDesc->ChannelPlanChannel[0]));
				// TODO: we may not support the 1st channel supported by peer
				pP2PInfo->OperatingChannel = pP2PDeviceDesc->ChannelPlanChannel[0]; // alwyas use the first one
			}
		}
		else
		{// Client
			if(!P2PIsChnlInChnlEntryList(&pP2PInfo->ChannelEntryList, 
				pP2PDeviceDesc->OperatingChannel))
			{// the intended chnl of the GO is not supported
				RT_TRACE_F(COMP_P2P, DBG_LOUD, 
					("the intended channel (%u) of the GO is not supported\n", 
					pP2PDeviceDesc->OperatingChannel));
				pP2PDeviceDesc->Status = P2P_STATUS_FAIL_NO_COMMON_CHANNELS;
				pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;
				goto exit_P2POnGONConfirm;
			}
			else
			{// use the intended op chnl of the GO
				pP2PInfo->OperatingChannel = pP2PDeviceDesc->OperatingChannel;				

				RT_PRINT_STR(COMP_P2P, DBG_LOUD, "P2P_OnGONConfirm(): I'm client, the target SSID = ", pP2PInfo->ConnectionContext.ConnectingDevice.SsidBuf, pP2PInfo->ConnectionContext.ConnectingDevice.SsidLen);
			}			
		}

		pP2PDeviceDesc->Status = P2P_STATUS_SUCCESS;

exit_P2POnGONConfirm:

		if(P2P_STATUS_SUCCESS == pP2PDeviceDesc->Status)
		{		
			WFD_OnP2PActionFrame(pAdapter, pRfd, posMpdu);
		}		
		else
		{
			pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)pP2PInfo->ConnectionContext.Status;
			P2PIndicateGOFormatedInfo(pP2PInfo, 
				pP2PInfo->ConnectionContext.Status, 
				pP2PInfo->ConnectionContext.bGoingToBeGO, 
				&pP2PInfo->ConnectionContext.ConnectingDevice);
		}

		// Fill ConnectionContext
		pP2PInfo->ConnectionContext.Status = pP2PDeviceDesc->Status;

		// Update state machine
		pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);
	}while(FALSE);

	p2p_DevList_Unlock(&pP2PInfo->devList);

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		p2p_IndicateActionFrameReceived(pP2PInfo, 
				P2P_EVENT_RECEIVED_GO_NEGOTIATION_CONFIRM, 
				RT_STATUS_SUCCESS, 
				posMpdu->Octet, 
				posMpdu->Length);
	}
 
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("status: %u\n", pP2PInfo->ConnectionContext.Status));
	return rtStatus;
}

//
// Description:
//	Handle P2P Go Invitation request packet for P2P function.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnInvitationReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PMGNT_INFO				pMgntInfo = &pAdapter->MgntInfo;
	PP2P_DEVICE_DISCRIPTOR	pClientInvitationInfo = NULL; 
	static u8Byte			TimeStamp = 0;
	static u2Byte			PreviousToken = 0xFFFF;
	u8Byte					CurrentTime = 0;
	u4Byte					ConfigurationTimeout = 0; // in ms, note that the configuration time out in the P2P IE is defined as units of 10ms
	P2P_ROLE				PeerRole = 0;
	P2P_LIB_INVITATION_REQ_CONTEXT LibInvitationReq;
	P2P_STATE				StateBackup;
	PRT_GEN_TEMP_BUFFER 	pGenBufClientInvitationInfo = NULL;
	BOOLEAN 				bBackwardInvite = FALSE;
	const u1Byte 			*pGrpBssid = NULL;

	P2P_DEV_LIST_ENTRY		*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	FunctionIn(COMP_P2P);

	// We should not accept InvitationReq too often.
	CurrentTime = PlatformGetCurrentTime();
	if(CurrentTime - TimeStamp < 2000000 && TimeStamp != 0 &&
		(0xFFFF != PreviousToken && (u1Byte)PreviousToken == *((pu1Byte)posMpdu->Octet + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN)))
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, 
			("[WARNING] recv InvitationReq too often => reject, time diff: %d, token = %d\n", 
			(u4Byte)(CurrentTime - TimeStamp), *((pu1Byte)posMpdu->Octet + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN)));
		return RT_STATUS_MEDIA_BUSY;
	}

	// we get here at the first time (since TimeStamp is 0), and we get the correct TimeStamp
	TimeStamp = CurrentTime;
	PreviousToken = *((pu1Byte)posMpdu->Octet + FRAME_OFFSET_P2P_PUB_ACT_DIALOG_TOKEN);
	
	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2P_OnInvitationReq():\n", posMpdu->Octet, posMpdu->Length);

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_INV_REQ, 
						pRfd, Frame_pSaddr(*posMpdu), 
						RT_GetChannelNumber(pP2PInfo->pAdapter), &pDev))
			)
		{
			break;
		}

		msg = pDev->rxFrames[P2P_FID_INV_REQ]->msg;
		
		if(!p2p_validate_InvitationReq(msg))
		{
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}
			
		pGenBufClientInvitationInfo = GetGenTempBuffer (pAdapter, sizeof(P2P_DEVICE_DISCRIPTOR));
		pClientInvitationInfo = (P2P_DEVICE_DISCRIPTOR *)pGenBufClientInvitationInfo->Buffer.Ptr;

		if(pP2PInfo->InvitationContext.bWaitingBackwardInvite &&
			eqMacAddr(pP2PInfo->InvitationContext.InvitedDevice.DeviceAddress, msg->devAddr))
		{
			bBackwardInvite = TRUE;
		}
		
		if(!bBackwardInvite) PlatformZeroMemory(&(pP2PInfo->InvitationContext), sizeof(P2P_INVITATION_CONTEXT));
		PlatformZeroMemory((pClientInvitationInfo), sizeof(P2P_DEVICE_DISCRIPTOR));

		// TODO: check capability

		// If we are not P2P Device, reject, since the invitor can communicate with us by join our group.
		if(!P2P_ACTING_AS_DEVICE(pP2PInfo))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, 
				("[WARNING] reject because current role is: %u, state is: %u\n", 
				pP2PInfo->Role, pP2PInfo->State));
			pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_INVALID_PARAMETERS;
			break;
		}

		//
		// Determine whether we are invited by a GO, Client or device.
		// We also determine whether its a persistent invitation here.
		//
		pP2PInfo->InvitationContext.InvitorRole = eqMacAddr(msg->grpDevAddr, msg->devAddr) ? (P2P_GO) : (P2P_CLIENT);
		pP2PInfo->InvitationContext.bPersistentInvitation = TEST_FLAG(msg->invitationFlags, BIT0);
		cpMacAddr(pP2PInfo->InvitationContext.GODeviceAddress, msg->grpDevAddr);
		CopySsid(pP2PInfo->InvitationContext.SsidBuf, pP2PInfo->InvitationContext.SsidLen, msg->grpSsid, msg->grpSsidLen); 
		PeerRole = pP2PInfo->InvitationContext.InvitorRole;

		// Parse the subelements and store the info in P2PDeviceDesc
		p2p_parse_UpdateDevDesc(pRfd->Buffer.VirtualAddress, msg, pRfd->Status.SignalStrength, pClientInvitationInfo);

		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bAcceptInvitation: %u\n", pP2PInfo->bAcceptInvitation));
		RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "AccpetInvitationDeviceAddress", pP2PInfo->AccpetInvitationDeviceAddress);

		if(bBackwardInvite																// accept backward invitation
			|| FALSE == pP2PInfo->InvitationContext.bPersistentInvitation	// if pure invitation, always accept it
			)
		{
			pP2PInfo->InvitationContext.Status = P2P_STATUS_SUCCESS;
		}
		else if(pP2PInfo->bAcceptInvitation)
		{// upper layer wants to accept invitation from a specific device
			if(eqMacAddr(pP2PInfo->AccpetInvitationDeviceAddress, msg->devAddr))
			{
				pP2PInfo->InvitationContext.Status = P2P_STATUS_SUCCESS;
			}
			else if(MacAddr_isBcst(pP2PInfo->AccpetInvitationDeviceAddress))
			{// to accept all
				pP2PInfo->InvitationContext.Status = P2P_STATUS_SUCCESS;
			}
			else
			{
				pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_INFORMATION_IS_UNAVAILABLE;
			}
		}
		else
		{
			pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_INFORMATION_IS_UNAVAILABLE;
		}

		if(PeerRole == P2P_CLIENT && pP2PInfo->InvitationContext.bPersistentInvitation)
		{// I'm the persistent GO, in this case, the invitation req may not have Group Bssid
			pGrpBssid = pP2PInfo->InterfaceAddress;
		}
		else
		{
			pGrpBssid = pClientInvitationInfo->GroupBssid;
		}

		if(0x00000004 <= P2P_VERSION										// don't check profile if upper layer registered a lower version
			&& P2P_ADAPTER_RTK_SUPPORT_P2P(pP2PInfo->pAdapter)	// don't check profile if running P2P natively supported by OS
			)
		{
			if(pP2PInfo->InvitationContext.bPersistentInvitation)
			{
				if(pP2PInfo->pProfileList)
				{
					u4Byte idxProfile = 0;

					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Profile list: %u profiles\n", pP2PInfo->pProfileList->nProfiles));

					for(idxProfile = 0; idxProfile < pP2PInfo->pProfileList->nProfiles; idxProfile++)
					{
						RT_PRINT_STR(COMP_P2P, DBG_LOUD, "Compare with SSID", pP2PInfo->pProfileList->profileList[idxProfile].ssidBuf, pP2PInfo->pProfileList->profileList[idxProfile].ssidBufLen);
						if(CompareSSID(pP2PInfo->pProfileList->profileList[idxProfile].ssidBuf, pP2PInfo->pProfileList->profileList[idxProfile].ssidBufLen, pClientInvitationInfo->SsidBuf, pClientInvitationInfo->SsidLen)
							&& eqMacAddr(pP2PInfo->pProfileList->profileList[idxProfile].grpBssid, pGrpBssid)
							)
						{
							RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Profile matched\n"));
							break;
						}
					}

					if(idxProfile == pP2PInfo->pProfileList->nProfiles)
					{
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("No matching profile\n"));
						pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_UNKNOWN_P2P_GROUP;
						break;
					}
				}
				else
				{
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("No matching profile (pProfileList is NULL)\n"));
					pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_UNKNOWN_P2P_GROUP;
					break;
				}
			}
		}

		//
		// Decide overlapped channel list and Op Channel
		//
		{
			p2p_Channel_Intersect(&pP2PInfo->ChannelEntryList, &pDev->p2p->channels, &pDev->p2p->commonChannels);

			if(pDev->p2p->commonChannels.regClasses)
			{ // Has Common Channel
				// Check if our op channel is one of channels supported by peer
				if(PeerRole == P2P_CLIENT && pP2PInfo->InvitationContext.bPersistentInvitation)
				{// I'm persistent GO
					if(!p2p_Channel_InChannelEntryList(pP2PInfo->OperatingChannel, &pDev->p2p->channels))
					{// the opChannel we are going to use does not exist in peer's channel list
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("The opChannel (%u) we are going to use does not exist in peer's channel list\n", pP2PInfo->OperatingChannel));
						RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "peer channel list:\n", pClientInvitationInfo->ChannelPlanChannel, pClientInvitationInfo->ChannelPlanLength);
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Changed to 1st channel (%d) in overlapped list\n", pDev->p2p->commonChannels.regClass[0].channel[0]));
						pP2PInfo->OperatingChannel = pDev->p2p->commonChannels.regClass[0].channel[0];
					}
				}
			}
			else // No Common Channel
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] no common channels\n"));
				RT_PRINT_DATA(COMP_P2P, DBG_WARNING, "[WARNING] peer supported op chnl:\n", 
					pClientInvitationInfo->ChannelPlanChannel, 
					pClientInvitationInfo->ChannelPlanLength);

				pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_NO_COMMON_CHANNELS;
				break;
			}
		}

		if(MgntRoamingInProgress(pMgntInfo))
			MgntRoamComplete(pAdapter, MlmeStatus_refused);
				

		//
		// Copy required info for constructing InvitationRsp
		pP2PInfo->InvitationContext.InvitorRole = PeerRole;
		pP2PInfo->InvitationContext.DialogToken = msg->dialogToken;
		PlatformMoveMemory(&pP2PInfo->InvitationContext.InvitedDevice, pClientInvitationInfo, sizeof(P2P_DEVICE_DISCRIPTOR));
		//pP2PInfo->InvitationContext.Status = pClientInvitationInfo->Status;
		pP2PInfo->InvitationContext.bInvitor = FALSE;
		PlatformMoveMemory(pP2PInfo->InvitationContext.SsidBuf, pClientInvitationInfo->SsidBuf, pClientInvitationInfo->SsidLen);
		pP2PInfo->InvitationContext.SsidLen = pClientInvitationInfo->SsidLen;
		cpMacAddr(pP2PInfo->InvitationContext.GroupBssid, pGrpBssid);
		if(PeerRole == P2P_CLIENT && pP2PInfo->InvitationContext.bPersistentInvitation)
		{// I'm the persistent GO, in this case, the invitation req may not have Group Bssid
			pP2PInfo->InvitationContext.OpChannel = pP2PInfo->OperatingChannel;
		}
		else
		{
			pP2PInfo->InvitationContext.OpChannel = pClientInvitationInfo->OperatingChannel;
		}
		pP2PInfo->InvitationContext.Channel = P2PGetChannel(pP2PInfo);

		if(PeerRole == P2P_GO)
		{
			PP2P_DEVICE_DISCRIPTOR pP2PDeviceDesc = NULL;
		
			pP2PDeviceDesc = P2PScanListFind(pP2PInfo->ScanList, pP2PInfo->ScanListSize, msg->devAddr, NULL, NULL);
			if(NULL != pP2PDeviceDesc)
			{
				pP2PDeviceDesc->Role = PeerRole;
				pP2PDeviceDesc->OperatingChannel = pClientInvitationInfo->OperatingChannel;
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Update peer role GO, chnl to: %u\n", pClientInvitationInfo->OperatingChannel));
			}
		}

		if(P2P_ADAPTER_RTK_SUPPORT_P2P(pP2PInfo->pAdapter)
			&& (TRUE == pP2PInfo->InvitationContext.bPersistentInvitation && PeerRole == P2P_GO
				|| FALSE == pP2PInfo->InvitationContext.bPersistentInvitation)
			)
		{
			if(RT_MEDIA_CONNECT == MgntLinkStatusQuery(GetDefaultAdapter(pAdapter)))
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] invited/reinvoke by GO but default port is connected, assume no client port\n"));
				pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_UNABLE_TO_ACCOMODATE_REQUEST;
				break;
			}
		}

		if(P2P_STATUS_SUCCESS == pP2PInfo->InvitationContext.Status
			&& FALSE == pP2PInfo->InvitationContext.bPersistentInvitation
			)
		{// Invited by a P2P Group member to join the group
			if(FALSE == P2PIsChnlInChnlEntryList(&pP2PInfo->ChannelEntryList, 
				pClientInvitationInfo->OperatingChannel))
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] peer GO op chnl (%u) not supported, reply no common channels\n", pClientInvitationInfo->OperatingChannel));

				pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_NO_COMMON_CHANNELS;
				pP2PInfo->ConnectionContext.Status = pP2PInfo->InvitationContext.Status;	
				break;
			}
		}


		if(P2P_STATUS_SUCCESS == pP2PInfo->InvitationContext.Status
			&& TRUE == pP2PInfo->InvitationContext.bPersistentInvitation
			&& P2P_CLIENT == PeerRole
			)
		{// reinvoked as a GO
			BOOLEAN bToIndicDevDiscComp = FALSE;
			
			if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
			{// Doing P2P Device Discovery
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("P2P_DOING_DEVICE_DISCOVERY: P2PScanListCeaseScan\n"));

				p2p_DevList_Unlock(&pP2PInfo->devList);
				P2PScanListCeaseScan(pP2PInfo);
				p2p_DevList_Lock(&pP2PInfo->devList);
				P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
				bToIndicDevDiscComp = TRUE;
			}
			else if(pP2PInfo->bExtendedListening)
			{// Doing extended listening
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("bExtendedListening: P2PScanListCeaseScan\n"));
				CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), P2P_INVITATION_FRAME_TIMEOUT);
				ConfigurationTimeout = P2P_INVITATION_FRAME_TIMEOUT;
				// P2PScanListCeaseScan(pP2PInfo);
				// P2PExtendedListenComplete(pP2PInfo);
			}
			else if(MgntScanInProgress(&pP2PInfo->pAdapter->MgntInfo)) 
			{// Doing normal scan
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("bScanInProgress: P2PScanListCeaseScan\n"));
				p2p_DevList_Unlock(&pP2PInfo->devList);
				P2PScanListCeaseScan(pP2PInfo);
				p2p_DevList_Lock(&pP2PInfo->devList);
			}

			if(bToIndicDevDiscComp && P2P_DOING_PURE_DEVICE_DISCOVERY(pP2PInfo))
			{
				P2PIndicateDeviceDiscoveryComplete(pP2PInfo);
			}
		}

		//
		// If failed, we shall not modify the original state machine state.
		//

		WFD_OnP2PActionFrame(pAdapter, pRfd, posMpdu);
	}while(FALSE);

	if(FALSE == P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		//
		// Fill the request for indicate to upper layer
		//
		LibInvitationReq.DialogToken = msg->dialogToken;
		LibInvitationReq.InvitorRole = PeerRole;
		PlatformMoveMemory(LibInvitationReq.TargetDeviceAddress, msg->devAddr, 6);
		LibInvitationReq.bPersistent = pP2PInfo->InvitationContext.bPersistentInvitation;
		LibInvitationReq.OpChannel = pP2PInfo->InvitationContext.OpChannel;
		PlatformMoveMemory(LibInvitationReq.GroupBssid, pP2PInfo->InvitationContext.GroupBssid, 6);
		PlatformMoveMemory(LibInvitationReq.GroupDeviceAddress, pP2PInfo->InvitationContext.GODeviceAddress, 6);
		LibInvitationReq.GroupSsidLen = pP2PInfo->InvitationContext.SsidLen;
		PlatformMoveMemory(LibInvitationReq.GroupSsidBuf, pP2PInfo->InvitationContext.SsidBuf, pP2PInfo->InvitationContext.SsidLen);		

		//
		// Send Invitation Rsp
		//
		StateBackup = pP2PInfo->State;
		pP2PInfo->State = P2P_STATE_INVITATION_REQ_RECVD;
		pP2PInfo->Status = pP2PInfo->InvitationContext.Status;
		CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);
		p2p_Send_InvitationRsp(pP2PInfo, msg->devAddr, LibInvitationReq.DialogToken);

		if(P2P_STATUS_SUCCESS == pP2PInfo->InvitationContext.Status 
			|| P2P_STATUS_FAIL_INFORMATION_IS_UNAVAILABLE == pP2PInfo->InvitationContext.Status
			|| P2P_STATUS_FAIL_UNABLE_TO_ACCOMODATE_REQUEST == pP2PInfo->InvitationContext.Status
			)
		{
			// Indicate ScanList first to make sure that the SDK is having the list.
			P2PIndicateScanList(pP2PInfo);
			P2PIndicateOnInvitationReq(pP2PInfo, &LibInvitationReq);
		}

		RT_TRACE_F(COMP_P2P, DBG_LOUD, 
			(" send InvitationRsp with status = %u\n",
			pP2PInfo->InvitationContext.Status));

		pP2PInfo->State = StateBackup;
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, ConfigurationTimeout);
	}

	if(pGenBufClientInvitationInfo)
		ReturnGenTempBuffer (pAdapter, pGenBufClientInvitationInfo);

	p2p_DevList_Unlock(&pP2PInfo->devList);

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		p2p_IndicateActionFrameReceived(pP2PInfo, 
				P2P_EVENT_RECEIVED_INVITATION_REQUEST, 
				RT_STATUS_SUCCESS, 
				posMpdu->Octet, 
				posMpdu->Length);

	}
	
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("<===	status: %u\n", pP2PInfo->InvitationContext.Status));	

	return rtStatus;
}

//
// Description:
//	Handle P2P Go Invitation response packet for P2P function.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnInvitationRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL; 
	static u8Byte			TimeStamp = 0;
	u4Byte					ConfigurationTimeout = 0; // in ms, note that the configuration time out in the P2P IE is defined as units of 10ms

	P2P_DEV_LIST_ENTRY		*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2POnInvitationRsp(): port number %d state %d\n", pP2PInfo->pAdapter->pNdis62Common->PortNumber, pP2PInfo->State));

	FunctionIn(COMP_P2P);
	
	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2P_OnInvitationRsp():\n", posMpdu->Octet, posMpdu->Length);

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_INV_RSP, 
						pRfd, Frame_pSaddr(*posMpdu), 
						RT_GetChannelNumber(pP2PInfo->pAdapter), &pDev))
			)
		{
			break;
		}
		
		msg = pDev->rxFrames[P2P_FID_INV_RSP]->msg;
		
		if(!p2p_validate_InvitationRsp(pP2PInfo->InvitationContext.InvitorRole, msg))
		{
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}

		// Check state
		if(!P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter)
			&& pP2PInfo->State != P2P_STATE_INVITATION_RSP_WAIT)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, 
				("[WARNING] recv InvitationRsp in an invalid state: %u\n", pP2PInfo->State));
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}
		
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("dialog token: %u\n", msg->dialogToken));
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2P_OnInvitationRsp():\n", posMpdu->Octet, posMpdu->Length);

		// Process subelements so that the status element would be stored in the device descriptor.
		pP2PDeviceDesc = P2PProcessSubElements(pP2PInfo, pRfd, msg->sa, msg, TRUE);
		if(pP2PDeviceDesc == NULL)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] reject because of invalid P2P Sub IE\n"));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// Update ConnectionContext.ConnectingDevice since after P2PProcessSubElements,
		// its ChannelList or other elements may be modified.
		PlatformMoveMemory(&pP2PInfo->InvitationContext.InvitedDevice, 
							pP2PDeviceDesc, 
							sizeof(P2P_DEVICE_DISCRIPTOR));

		// If we'll be the client, we shall wait for AP configuration timeout
		pP2PInfo->InvitationContext.Status = (P2P_STATUS_CODE)pP2PDeviceDesc->Status;
		if(pP2PInfo->InvitationContext.Status == P2P_STATUS_SUCCESS)
		{
			WFD_OnP2PActionFrame(pAdapter, pRfd, posMpdu);

			if(eqMacAddr(pP2PInfo->InvitationContext.GODeviceAddress, pP2PInfo->DeviceAddress))
			{// I'm going to be the GO
				ConfigurationTimeout = 0; //(10 * MAX(pP2PInfo->GOConfigurationTimeout, pP2PInfo->InvitationContext.InvitedDevice.ClientConfigurationTimeout));
			}
			else 
			{// I'm going to be the Client => wait for peer to be ready for connection
				ConfigurationTimeout = (10 * MAX(pP2PInfo->ClientConfigurationTimeout, pP2PInfo->InvitationContext.InvitedDevice.GOConfigurationTimeout));
			}
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("ConfigurationTimeout: %d (ms)\n", ConfigurationTimeout));
		}
		else
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING]status: %u\n", pP2PInfo->InvitationContext.Status));
		}

		if(P2P_STATUS_FAIL_INFORMATION_IS_UNAVAILABLE == pP2PInfo->InvitationContext.Status)
		{
			pP2PInfo->InvitationContext.bWaitingBackwardInvite = TRUE;
		}

		// For Win8: Indicate the reception of the invitation response ---------------------------
		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			pP2PInfo->State = P2P_STATE_INVITATION_COMPLETE;
			PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
			PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, ConfigurationTimeout);
			break;
				}
		// -----------------------------------------------------------------------------

		P2PScanListEnterScanCompleteImmediately(pP2PInfo);
		
		pP2PInfo->State = P2P_STATE_INVITATION_COMPLETE;
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, ConfigurationTimeout);
	}while(FALSE);

	p2p_DevList_Unlock(&pP2PInfo->devList);

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		p2p_IndicateActionFrameReceived(pP2PInfo, 
			P2P_EVENT_RECEIVED_INVITATION_RESPONSE, 
			RT_STATUS_SUCCESS, 
			posMpdu->Octet, 
			posMpdu->Length);
	}
 
	return rtStatus;
}

//
// Description:
//	Handle P2P Device Discoverability Request packet for P2P function.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnDeviceDiscoverabilityReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pAdapter); 
	pu1Byte			StaInterfaceAddress = NULL;
	u1Byte			i = 0;
	PRT_WLAN_STA	pEntry = NULL;
	u1Byte			Status = P2P_STATUS_SUCCESS;

	PADAPTER pExtAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? pP2PInfo->pAdapter : GetFirstGOPort(pP2PInfo->pAdapter);

	PMGNT_INFO pExtMgntInfo = &pExtAdapter->MgntInfo;

	P2P_MESSAGE				msg;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
			return RT_STATUS_SUCCESS;

	if(!P2P_ACTING_AS_GO(pP2PInfo))
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Not act as GO!\n"));
		return RT_STATUS_INVALID_STATE;
	}
	
	FunctionIn(COMP_P2P);

	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2P_OnDeviceDiscoverabilityReq():\n", posMpdu->Octet, posMpdu->Length);

	do
	{
		if(RT_STATUS_SUCCESS == (rtStatus = p2p_parse_Action(posMpdu, DBG_LOUD, &msg)))
			if(!p2p_validate_DevDiscReq(&msg))
			{
				rtStatus = RT_STATUS_MALFORMED_PKT;
				break;
			}
		
		// Find STA Interface Address
		for(i = 0;i < ASSOCIATE_ENTRY_NUM; i++)
		{
			pEntry = &(pExtMgntInfo->AsocEntry[i]);
			if(pEntry->bUsed &&
				pEntry->bAssociated &&
				pEntry->bP2PClient)
			{
				if(eqMacAddr(pEntry->P2PClientInfoDesc.DeviceAddress, msg.devIdDevAddr))
				{
					StaInterfaceAddress = pEntry->P2PClientInfoDesc.InterfaceAddress;
					break;
				}
			}
		}

		if(StaInterfaceAddress == NULL)
		{
			Status = P2P_STATUS_FAIL_UNABLE_TO_ACCOMODATE_REQUEST;
			goto send_DevDiscRsp;
		}

		// Send Go Discoverability Request to our client.
		p2p_Send_GoDiscoverabilityReq(pP2PInfo, StaInterfaceAddress, 1);

send_DevDiscRsp:

		// Send DeviceDiscoverabilityRsp to the SA of DeviceDiscoverabilityReq
		p2p_Send_DevDiscRsp(pP2PInfo, msg.sa, msg.dialogToken, Status);
	}while(FALSE);

	p2p_parse_FreeMessage(&msg);
 
	FunctionOut(COMP_P2P);
	return rtStatus;
}

//
// Description:
//	Handle P2P Device Discoverability Response packet for P2P function.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnDeviceDiscoverabilityRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter); 
	PP2P_CLIENT_INFO_DISCRIPTOR		pClient = NULL;
	PP2P_DEVICE_DISCRIPTOR	pGO = NULL;

	P2P_MESSAGE				msg;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	FunctionIn(COMP_P2P);
	
	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2P_OnDeviceDiscoverabilityRsp():\n", posMpdu->Octet, posMpdu->Length);

	do
	{
		if(RT_STATUS_SUCCESS == (rtStatus = p2p_parse_Action(posMpdu, DBG_LOUD, &msg)))
			if(!p2p_validate_DevDiscRsp(&msg))
			{
				rtStatus =RT_STATUS_MALFORMED_PKT;
				break;
			}
	
		// Check state
		if(pP2PInfo->State != P2P_STATE_DEVICE_DISCOVERABILITY_RSP_WAIT)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Mismatch state = %d\n", pP2PInfo->State));
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		// Check DialogToken
		if(msg.dialogToken != pP2PInfo->DeviceDiscoverabilityContext.DialogToken)
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
		else
		{
			pP2PInfo->DeviceDiscoverabilityContext.DialogToken++;
		}

		// Get status
		pP2PInfo->DeviceDiscoverabilityContext.Status = (P2P_STATUS_CODE)msg.status;

		// Find the client and its GO
		if(!P2PScanListFindClient(pP2PInfo->ScanList, pP2PInfo->ScanListSize, 
			pP2PInfo->DeviceDiscoverabilityContext.ClientDeviceAddress, 
			NULL, 
			&pGO, &pClient))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("client not found\n"));
			break;
		}

		if(!pClient)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("client not found\n"));
			break;
		}

		//
		// Check Capability
		//
		if(!(pClient->DeviceCapability & dcP2PClientDiscoverability))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] dcP2PClientDiscoverability bit in client is not set!\n"));
			break;
		}
		PlatformMoveMemory(&(pP2PInfo->ConnectionContext.ConnectingDevice.DeviceAddress), 
			pP2PInfo->DeviceDiscoverabilityContext.ClientDeviceAddress, 6);
		pP2PInfo->ConnectionContext.Status = P2P_STATUS_SUCCESS;
		pP2PInfo->ConnectionContext.DialogToken = IncreaseDialogToken(pP2PInfo->DialogToken);
		pP2PInfo->ConnectionContext.FindPhaseLoopTimes = P2P_SCAN_FIND_PHASE_LOOP_TIMES;
		pP2PInfo->ConnectionContext.bGoingToBeGO = FALSE;

		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ConnectionContext.ConnectingDevice:\n", &pP2PInfo->ConnectionContext.ConnectingDevice, sizeof(P2P_DEVICE_DISCRIPTOR));

		pP2PInfo->State = P2P_STATE_DEVICE_DISCOVERABILITY_COMPLETE;
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);
	}while(FALSE);

	p2p_parse_FreeMessage(&msg);
 
	return rtStatus;
}

//
// Description:
//	The client recived the GO discoverability request packet, leave PS mode for while.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnGODiscoverabilityReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PMGNT_INFO				pMgntInfo = &(pP2PInfo->pAdapter->MgntInfo);
	PRT_POWER_SAVE_CONTROL	pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);

	FunctionIn(COMP_P2P);
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2P_OnGODiscoverabilityReq():\n", 
		posMpdu->Octet, posMpdu->Length);
	if(pPSC->bLeisurePs || pMgntInfo->dot11PowerSaveMode != eActive)
	{
		pMgntInfo->LPSDelayCnt = (pMgntInfo->LPSDelayCnt < 10) ? 10 : pMgntInfo->LPSDelayCnt;
		// Send one null frame to notify AP that we are awkae.
		MgntActSet_802_11_PowerSaveMode(pP2PInfo->pAdapter, eActive);
	}
	return rtStatus;
}

//
// Description:
//	Handle P2P Provision Discovery Request packet for P2P function.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnProvisionDiscoveryReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO			pP2PInfo = GET_P2P_INFO(pAdapter); 
	u8Byte				CurrentTime = PlatformGetCurrentTime();
	static u8Byte		LastTime = 0;
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL;
	BOOLEAN 				bToSendPDRsp = TRUE;
	u2Byte				rspConfigMethod = 0;

	P2P_DEV_LIST_ENTRY 		*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;
	

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	FunctionIn(COMP_P2P);
	
	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2P_OnDeviceDiscoverabilityRsp():\n", posMpdu->Octet, posMpdu->Length);

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		BOOLEAN					bFirstSeen = TRUE;

		bFirstSeen = (NULL == p2p_DevList_Find(&pP2PInfo->devList, Frame_pSaddr(*posMpdu), P2P_DEV_TYPE_DEV));

		p2p_DevList_FlushActionFrames(&pP2PInfo->devList, Frame_pSaddr(*posMpdu), P2P_DEV_TYPE_DEV);
		
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_PD_REQ, 
						pRfd, Frame_pSaddr(*posMpdu), 
						RT_GetChannelNumber(pP2PInfo->pAdapter), &pDev))
			)
		{
			break;
		}
		
		msg = pDev->rxFrames[P2P_FID_PD_REQ]->msg;
		
		if(!p2p_validate_PDReq(msg))
		{
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}
		
		if(CurrentTime - LastTime < 1000000)
		{// this is to prevent the device from showing notifications to user too often
			rtStatus = RT_STATUS_PKT_DROP;
			break;
		}
		else
		{
			LastTime = CurrentTime;
		}

		pP2PDeviceDesc = P2PProcessSubElements(pP2PInfo, pRfd, msg->devAddr, msg, TRUE);
		if(pP2PDeviceDesc == NULL)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] reject because of invalid P2P Sub IE\n"));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// Check ConfigMethod compatible
		if(pP2PInfo->WpsAttributes.ConfigMethod & msg->wpsConfigMethods)
		{
			rspConfigMethod = msg->wpsConfigMethods;
		}
		else
		{// config method not compatible
			RT_TRACE_F(COMP_P2P, DBG_LOUD, 
				("[WARNING] recvd CM (%u) not compatible with my CM (%u)\n",
				msg->wpsConfigMethods, pP2PInfo->WpsAttributes.ConfigMethod));
			rspConfigMethod = 0; // set to NULL to indicate failure
		}
		
		// Indicate OnPDReq before P2PSvc processes it
		if(bFirstSeen)
		{
			// Indicate ScanList first to make sure that the SDK is having the list.
			P2PIndicateScanList(pP2PInfo);
		}

			// Indicate to upper layer
		P2PIndicateOnProvisionDiscoveryReq(pP2PInfo, rspConfigMethod, msg->devAddr, &pP2PDeviceDesc->WpsAttributes);

		P2PSvc_OnPDReq(pP2PInfo->pP2PSvcInfo, msg->devAddr, msg->wpsConfigMethods, &msg->p2pAttributes.os, &bToSendPDRsp);
		if(bToSendPDRsp && 0 == pP2PDeviceDesc->ListenChannel)
		{// We need to send PD rsp, and we don't have peer's Listen Channel
			if(msg->_listenChannel)
				pP2PDeviceDesc->ListenChannel = msg->listenChannel;
			}
		
		// Update the latest scan list to upper layer.
		P2PScanListCopy(pP2PInfo->ScanList4Query, &pP2PInfo->ScanList4QuerySize, 
			pP2PInfo->ScanList, pP2PInfo->ScanListSize);
		P2PIndicateScanList(pP2PInfo);

		WFD_OnP2PActionFrame(pAdapter, pRfd, posMpdu);

		if(FALSE == P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			if(bToSendPDRsp)
			{
				BOOLEAN bSupportTxReport = FALSE;
				
				//
				// Send ProvisionDiscoveryRsp
				//
				if(P2P_DEVICE == pP2PInfo->Role)
				{
					CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);
				}
				p2p_Send_PDRsp(pP2PInfo, msg->devAddr, msg->dialogToken, &msg->p2pAttributes.os, rspConfigMethod, &bSupportTxReport);

				if(FALSE == bSupportTxReport)
				{
					P2PSvc_OnPDRspSent(pP2PInfo->pP2PSvcInfo, TRUE);
				}
			}
		}
	}while(FALSE);

	p2p_DevList_Unlock(&pP2PInfo->devList);

	if(rtStatus == RT_STATUS_SUCCESS)
	{
		p2p_IndicateActionFrameReceived(pP2PInfo, 
			P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_REQUEST, 
			RT_STATUS_SUCCESS, 
			posMpdu->Octet, 
			posMpdu->Length);
	}

	FunctionOut(COMP_P2P);

	return rtStatus;
}

//
// Description:
//	Handle P2P Provistion Discovery Response packet for P2P function.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnProvisionDiscoveryRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO			pP2PInfo = GET_P2P_INFO(pAdapter); 

	P2P_DEV_LIST_ENTRY 	*pDev = NULL;
	P2P_MESSAGE				*msg = NULL;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	FunctionIn(COMP_P2P);
	
	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2P_OnDeviceDiscoverabilityRsp():\n", posMpdu->Octet, posMpdu->Length);

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = p2p_DevList_RxUpdate(&pP2PInfo->devList, 
						P2P_FID_PD_RSP, 
						pRfd, 
						pP2PInfo->ProvisionDiscoveryContext.go 
							? pP2PInfo->ProvisionDiscoveryContext.goBssid 
							: pP2PInfo->ProvisionDiscoveryContext.devAddr, 
						RT_GetChannelNumber(pP2PInfo->pAdapter), &pDev))
			)
		{
			break;
		}
		
		msg = pDev->rxFrames[P2P_FID_PD_RSP]->msg;
		
		if(!p2p_validate_PDRsp(msg))
		{
			rtStatus = RT_STATUS_MALFORMED_PKT;
			break;
		}
		
		// Check DialogToken
		if(!P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter)
			&& msg->dialogToken != pDev->txFrames[P2P_FID_PD_REQ]->token
			)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, 
				("[WARNING] dialog token mismatch: peer(%u), mine(%u)\n",
				msg->dialogToken, pDev->txFrames[P2P_FID_PD_REQ]->token));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		p2p_DevList_DialogTokenUpdate(pDev);

		RT_TRACE_F(COMP_P2P, DBG_LOUD, 
			("CM requested: %u, CM responded: %u\n", 
			pDev->p2p->pdConfigMethod, msg->wpsConfigMethods));

		if(!P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
		if(pDev->p2p->pdConfigMethod == msg->wpsConfigMethods)
		{
			pP2PInfo->ConnectionContext.Status = P2P_STATUS_SUCCESS;
		}
		else
		{
			pP2PInfo->ConnectionContext.Status = P2P_STATUS_FAIL_PROVISION_DISCOVERY_FAILED;
		}
		}

		p2p_DevList_Unlock(&pP2PInfo->devList);
		CustomScan_TermReq(GET_CUSTOM_SCAN_INFO(pAdapter), FALSE);
		p2p_DevList_Lock(&pP2PInfo->devList);

		P2PSvc_OnPDRsp(pP2PInfo->pP2PSvcInfo, 
			pDev->mac,
			pDev->p2p->pdConfigMethod,
			msg->wpsConfigMethods, 
			&msg->p2pAttributes.os
			);

		pP2PInfo->State = P2P_STATE_PROVISION_DISCOVERY_COMPLETE;
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 1);		
	}while(FALSE);
 
	RT_TRACE_F(COMP_P2P, DBG_LOUD, (" %u\n", pP2PInfo->ConnectionContext.Status));

	p2p_DevList_Unlock(&pP2PInfo->devList);

	if(rtStatus == RT_STATUS_SUCCESS)
	{
		p2p_IndicateActionFrameReceived(pP2PInfo, 
			P2P_EVENT_RECEIVED_PROVISION_DISCOVERY_RESPONSE, 
			RT_STATUS_SUCCESS, 
			posMpdu->Octet, 
			posMpdu->Length);
	}

	return rtStatus;
}

//
// Description:
//	Handle Service Discovery Request packet.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnSDReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pAdapter);
	pu1Byte			SrcAddr = Frame_pSaddr(*posMpdu); // this shall be the Device Address
	u1Byte			DialogToken;
	PP2P_SERVICE_REQ_TLV pSDReqRecvd = NULL;
	pu1Byte			pSDReqRecvdSize = NULL;
	pu1Byte			pTLVStart = posMpdu->Octet + FRAME_OFFSET_GAS_INIT_REQ_SERVICE_REQ_TLV;
	pu1Byte			pTLVCurrent = NULL;
	BOOLEAN			bFragmentSDRsp = FALSE;
	u1Byte 			i;
	BOOLEAN			bToSendSDRsp = TRUE;

	FunctionIn(COMP_P2P);

	if(pP2PInfo->SDContext.bDoingServiceDiscovery)
	{// we are doing service disocvery, ignore further requests
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] we are doing service disocvery, ignore further requests!\n"));
		return RT_STATUS_INVALID_STATE;
	}

	pP2PInfo->SDContext.bDoingServiceDiscovery = TRUE;

	//
	// Make sure we can read all the mandatory field of the SDReq
	//
	if(posMpdu->Length < FRAME_OFFSET_GAS_INIT_REQ_SERVICE_REQ_TLV)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, 
			("[WARNING] frame lengh (%u) < min SDReq lenght (%u)\n",
			posMpdu->Length, FRAME_OFFSET_GAS_INIT_REQ_SERVICE_REQ_TLV));
		return RT_STATUS_MALFORMED_PKT;
	}
	
	//RT_PRINT_DATA(COMP_P2P, DBG_LOUD, 
	//	"The ServiceDiscoveryReq Content : ", 
	//	osPacket.Octet, osPacket.Length);

	//
	// Set the Status to success
	//
	pP2PInfo->SDContext.Status = P2P_SD_STATUS_SUCCESS;

	//
	// Store Source Device Address
	//
	cpMacAddr(pP2PInfo->SDContext.TargetDeviceAddress, SrcAddr);

	//
	// Store Dialog Token
	//
	DialogToken = *((pu1Byte)(posMpdu->Octet + FRAME_OFFSET_GAS_INIT_REQ_DIALOG_TOKEN));
	pP2PInfo->SDContext.DialogToken = DialogToken;

	//
	// Reset the buffer we used to store the received SDReq
	//
	pSDReqRecvd = pP2PInfo->SDContext.ServiceReqRecvd;
	pSDReqRecvdSize = &pP2PInfo->SDContext.ServiceReqRecvdSize;
	
	PlatformZeroMemory(pSDReqRecvd, sizeof(P2P_SERVICE_REQ_TLV) * P2P_SD_MAX_SERVICES);
	*pSDReqRecvdSize = 0;
	
	//
	// Get ALL Service Req TLVs
	//
	pTLVCurrent = pTLVStart;
	while(TRUE)
	{
		u2Byte TLVLenField;
		u2Byte QueryDataLen;

		// Check if we have enough room for the TLV
		if(*pSDReqRecvdSize >= P2P_SD_MAX_SERVICES)
		{
			break;
		}

		// Validate the TLV length
		if((pTLVCurrent + OFFSET_SERVICE_REQ_TLV_QUERY_DATA) > 
			(posMpdu->Octet + posMpdu->Length))
		{
			//pP2PInfo->SDContext.Status = P2P_SD_STATUS_BAD_REQUEST;
			break;
		}

		// Read the length field of the TLV
		TLVLenField = ReadEF2Byte(pTLVCurrent + OFFSET_SERVICE_REQ_TLV_LEN);
		if(TLVLenField < (u2Byte)(OFFSET_SERVICE_REQ_TLV_QUERY_DATA - OFFSET_SERVICE_REQ_TLV_SERVICE_PROT_TYPE))
		{// invalid TLV len
			pP2PInfo->SDContext.Status = P2P_SD_STATUS_BAD_REQUEST;
			break;
		}
		if((pTLVCurrent + 2 + TLVLenField) > (posMpdu->Octet + posMpdu->Length))
		{// invalid TLV len
			pP2PInfo->SDContext.Status = P2P_SD_STATUS_BAD_REQUEST;
			break;
		}

		// Read Sercive Protocol Type
		pSDReqRecvd[*pSDReqRecvdSize].ServiceDesc.ServiceType = (P2P_SD_PROTOCOL)
			( *((pu1Byte)(pTLVCurrent + OFFSET_SERVICE_REQ_TLV_SERVICE_PROT_TYPE)) );

		// Read Service Transaction ID
		pSDReqRecvd[*pSDReqRecvdSize].TransactionID = 
			*((pu1Byte)(pTLVCurrent + OFFSET_SERVICE_REQ_TLV_SERVICE_TRANSACTION_ID));
		
		// Get Query Data length
		QueryDataLen = TLVLenField - 2; // exclude ServiceProtType(1) and ServiceTransactionID(1)

		// Check if we can copy the Query Data to our buffer
		if(QueryDataLen > P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN)
		{
			pP2PInfo->SDContext.Status = P2P_SD_STATUS_BAD_REQUEST;
			break;
		}

		// Copy Query Data
		PlatformMoveMemory(pSDReqRecvd[*pSDReqRecvdSize].ServiceDesc.Buffer, 
			pTLVCurrent + OFFSET_SERVICE_REQ_TLV_QUERY_DATA, 
			QueryDataLen);
		pSDReqRecvd[*pSDReqRecvdSize].ServiceDesc.BufferLength = QueryDataLen;

		(*pSDReqRecvdSize)++;

		if(*pSDReqRecvdSize == P2P_SD_MAX_SERVICES)
		{
			break;
		}

		// Update pTLVCurrent
		pTLVCurrent += (2 + TLVLenField);
	}

	if(pP2PInfo->SDContext.ServiceReqRecvdSize == 0)
	{
		pP2PInfo->SDContext.Status = P2P_SD_STATUS_BAD_REQUEST;
	}

	if(pP2PInfo->SDContext.Status != P2P_SD_STATUS_SUCCESS)
	{
		goto Exit_P2POnSDReq;
	}

	P2PSvc_OnSDReq(pP2PInfo->pP2PSvcInfo, SrcAddr, DialogToken, *pSDReqRecvdSize, pSDReqRecvd, &bToSendSDRsp);
	if(!bToSendSDRsp)
	{
		pP2PInfo->SDContext.bDoingServiceDiscovery = FALSE;
		goto Exit_P2POnSDReqNoRsp;
	}

	//
	// Print all Service Req TLVs recvd
	//
	for(i = 0; i < *pSDReqRecvdSize; i++)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("Servicd Req TLV %u: Protocol: %u\n", 
			i, pSDReqRecvd[i].ServiceDesc.ServiceType));
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, 
			"Query Data", 
			pSDReqRecvd[i].ServiceDesc.Buffer, pSDReqRecvd[i].ServiceDesc.BufferLength);
	}
	
Exit_P2POnSDReq:

	if(pP2PInfo->SDContext.Status == P2P_SD_STATUS_SUCCESS)
	{
		CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), P2P_SERVICE_DISCOVERY_TIMEOUT);
		P2PIndicateOnSDReq(pP2PInfo, &pP2PInfo->SDContext);
	}
	else
	{
		p2p_Send_SDRsp(pP2PInfo, SrcAddr, DialogToken, pP2PInfo->SDContext.Status, bFragmentSDRsp);
	}

Exit_P2POnSDReqNoRsp:
	RT_TRACE_F(COMP_P2P, DBG_LOUD, 
		(" Status: %u, DialogToken: %u\n", 
		pP2PInfo->SDContext.Status,
		DialogToken));

	return rtStatus;	
}

//
// Description:
//	Handle Service Discovery Response packet.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnSDRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pAdapter);
	PP2P_SD_CONTEXT	pSDContext = &pP2PInfo->SDContext;
	pu1Byte			pTLVStart = posMpdu->Octet + FRAME_OFFSET_GAS_INIT_RSP_TLV;
	pu1Byte			pTLVCurrent = NULL;
	u2Byte			QueryRspLen;
	u2Byte			TLVLen = 0;
	BOOLEAN			bFragment = FALSE;

	FunctionIn(COMP_P2P);

	// Check state
	//if(pP2PInfo->State != P2P_STATE_SERVICE_DISCOVERY_RSP_WAIT)
	//{
	//	RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] current state (%d) is invalid!\n", pP2PInfo->State));
	//	return RT_STATUS_INVALID_STATE;
	//}

	// Check if the SDRsp is from the intended peer
	if(!eqMacAddr(pSDContext->TargetDeviceAddress, 
		Frame_pSaddr(*posMpdu)))
	{
		RT_PRINT_ADDR(COMP_P2P, DBG_WARNING, "P2P_OnSDRsp():[WARNING] mismatch addr = ", pSDContext->TargetDeviceAddress);
		return RT_STATUS_INVALID_DATA;
	}

	// Check length
	if(posMpdu->Length < FRAME_OFFSET_GAS_INIT_RSP_QUERY_DATA)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Invalid length of packet!\n"));
		return RT_STATUS_MALFORMED_PKT;
	}

	//
	// Check Dialog Token
	// After Check length, we are safe to read Dialog Token
	//
	if(*((pu1Byte)(posMpdu->Octet + FRAME_OFFSET_GAS_INIT_RSP_DIALOG_TOKEN)) != pSDContext->DialogToken)

	{// Dialog Token mismatch
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] mismatch token target (%d) and mine (%d)!\n", 
					*((pu1Byte)(posMpdu->Octet + FRAME_OFFSET_GAS_INIT_RSP_DIALOG_TOKEN)), pSDContext->DialogToken));
		return RT_STATUS_INVALID_DATA;
	}

	//
	// Clear ServiceRspRecvd for storing the Rsp we recvd
	//
	PlatformZeroMemory(pSDContext->ServiceRspRecvd, sizeof(P2P_SERVICE_RSP_TLV) * P2P_SD_MAX_SERVICES);
	pSDContext->ServiceRspRecvdSize = 0;

	//
	// Get SDRsp fields
	//
	{
		// Status (11u)
		// Ignore
		
		// GAS Comeback Delay
		if(ReadEF2Byte(posMpdu->Octet + FRAME_OFFSET_GAS_INIT_RSP_COMEBACK_DELAY))
		{
			bFragment = TRUE;
			goto Exit_P2POnSDRsp; // the SDRsp shall be empty in this case

			// Fragment is the only case that there is no TLV in the SDRsp
			// So below we can assume that we shall have at lease 1 TLV.
		}

		// AD Prot IE
		// Ignore

		// Query Rsp Len
		QueryRspLen = ReadEF2Byte(posMpdu->Octet + FRAME_OFFSET_GAS_INIT_RSP_LEN); // ignore

		// Service update indicator
		pSDContext->ServiceUpdateIndicator = ReadEF2Byte(posMpdu->Octet + FRAME_OFFSET_GAS_INIT_RSP_SERVICE_UPDATE_INDICATOR); // ignore
		
		//
		// Read the TLVs, a valid Rsp shall have at lease 1 TLV
		//
		{
			u1Byte index;
			
			pTLVCurrent = pTLVStart;

			PlatformZeroMemory(pP2PInfo->SDContext.ServiceRspRecvd, sizeof(P2P_SERVICE_RSP_TLV) * P2P_SD_MAX_SERVICES);
			pP2PInfo->SDContext.ServiceRspRecvdSize = 0;
			index = pP2PInfo->SDContext.ServiceRspRecvdSize;

			while(TRUE)
			{
				u2Byte ResponseDataLen = 0;
				u1Byte TransactionId = 0;

				// Check if we have enough room for the TLV
				if(index >= P2P_SD_MAX_SERVICES)
				{
					break;
				}
				
				// Make sure we can read until the end of Status Code filed, which is right before RSP_DATA
				if((pTLVCurrent + OFFSET_SERVICE_RSP_TLV_RSP_DATA) > 
					(posMpdu->Octet + posMpdu->Length))
				{
					pP2PInfo->SDContext.Status = P2P_SD_STATUS_BAD_RESPONSE_1;
					break;
				}
				
				// Length
				TLVLen = (ReadEF2Byte(pTLVCurrent + OFFSET_SERVICE_RSP_TLV_LEN));
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("TLVLen: %u\n", TLVLen));
				if(TLVLen < (u2Byte)(OFFSET_SERVICE_RSP_TLV_RSP_DATA - OFFSET_SERVICE_RSP_TLV_SERVICE_PROT_TYPE))
				{// invalid TLV len
					pP2PInfo->SDContext.Status = P2P_SD_STATUS_BAD_RESPONSE_2;
					break;
				}
				if((pTLVCurrent + 2 + TLVLen) > (posMpdu->Octet + posMpdu->Length))
				{// invalid TLV len
					pP2PInfo->SDContext.Status = P2P_SD_STATUS_BAD_RESPONSE_3;
					break;
				}

				// Service Protocol Type
				(pP2PInfo->SDContext.ServiceRspRecvd)[index].ServiceDesc.ServiceType = (P2P_SD_PROTOCOL)
					(*((pu1Byte)(pTLVCurrent + OFFSET_SERVICE_RSP_TLV_SERVICE_PROT_TYPE)) );

				// Transactio ID
				TransactionId = (*((pu1Byte)(pTLVCurrent + OFFSET_SERVICE_RSP_TLV_SERVICE_TRANSACTION_ID)));

				// Status Code
				(pP2PInfo->SDContext.ServiceRspRecvd)[index].Status = (P2P_SD_STATUS_CODE)
					(*((pu1Byte)(pTLVCurrent + OFFSET_SERVICE_RSP_TLV_STATUS)) );
				// For the last status (status of the entire SDRsp), we currently get it from the last TLV
				pP2PInfo->SDContext.Status = (pP2PInfo->SDContext.ServiceRspRecvd)[index].Status;

				// Response Data
				if((pP2PInfo->SDContext.ServiceRspRecvd)[index].Status == P2P_SD_STATUS_SUCCESS)
				{
					ResponseDataLen = TLVLen - 3;
					if(ResponseDataLen > P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN)
					{// buffer overflow
						RT_TRACE(COMP_P2P, DBG_LOUD, 
							("Buffer size overflow: (%u) > (%u) \n",
							ResponseDataLen,
							P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN));
						break;
					}

					PlatformMoveMemory((pP2PInfo->SDContext.ServiceRspRecvd)[index].ServiceDesc.Buffer, 
						pTLVCurrent + OFFSET_SERVICE_RSP_TLV_RSP_DATA, 
						ResponseDataLen);

					(pP2PInfo->SDContext.ServiceRspRecvd)[index].ServiceDesc.BufferLength = ResponseDataLen;

					RT_PRINT_STR(COMP_P2P, DBG_LOUD, 
						"Recv TLV: ", 
						(pP2PInfo->SDContext.ServiceRspRecvd)[index].ServiceDesc.Buffer, 
						(pP2PInfo->SDContext.ServiceRspRecvd)[index].ServiceDesc.BufferLength);
				}

				if(P2P_SD_PROTOCOL_P2PSVC == (pP2PInfo->SDContext.ServiceRspRecvd)[index].ServiceDesc.ServiceType)
				{
					BOOLEAN bNeedFurtherProcess = FALSE;
					P2PSvc_OnSDRsp(pP2PInfo->pP2PSvcInfo, Frame_pSaddr(*posMpdu), TransactionId, pP2PInfo->SDContext.ServiceRspRecvd + index, &bNeedFurtherProcess);
					if(!bNeedFurtherProcess)
					{
						goto Exit_P2POnSDRsp_SilentExit; // so current dev disc will not be ceased
					}
				}

				pP2PInfo->SDContext.ServiceRspRecvdSize++;
				index++;

				// Go to next TLV
				pTLVCurrent += (5 + ResponseDataLen); // 5 = length(2) + SvcProtType(1), SvcTransactionID(1), Status(1)

				// Check if this is the last TLV
				if(pTLVCurrent >= (posMpdu->Octet+ posMpdu->Length))
				{
					break;
				}
			}
		}
	}

Exit_P2POnSDRsp:

	if(bFragment)
	{	
		// Update SDContext
		pSDContext->bDoingServiceDiscovery = TRUE;
		pSDContext->bFragment = TRUE;
		pSDContext->FragmentID = 0; // we are expecting FragmentID 0
		pSDContext->ServiceRspRecvdSize = 0;

		pSDContext->ANQPQueryRspFieldToSendSize = 0;
		pSDContext->ANQPQueryRspFieldToSendOffset = 0;

		//
		// Since P2PMgntTimerCallback() checks bScanInProgress before it enters the state machine,
		// we can't use the state machine to deal with the comeback procedure.
		//
		p2p_Send_SDComebackReq(pP2PInfo, 
			pP2PInfo->SDContext.TargetDeviceAddress,
			pP2PInfo->SDContext.DialogToken);

		pP2PInfo->State = P2P_STATE_SERVICE_DISCOVERY_COMEBACK_RSP_WAIT;
	}
	else
	{
		pP2PInfo->State = P2P_STATE_SERVICE_DISCOVERY_COMPLETE;

		if(pSDContext->Status == P2P_SD_STATUS_SUCCESS)
		{
			P2PIndicateOnSDRsp(pP2PInfo, pSDContext);
		}

		// Entery ScanComplete immediately
		P2PScanListEnterScanCompleteImmediately(pP2PInfo);

		// Entery P2P_STATE_SERVICE_DISCOVERY_COMPLETE (almost) right after ScanComplete
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 100);
	}
	
Exit_P2POnSDRsp_SilentExit:	
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("status: %u, bFragment: %u\n", pSDContext->Status, bFragment));

	return rtStatus;
}

//
// Description:
//	Handle Service Discovery Request comback packet.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnSDComebackReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pAdapter);
	pu1Byte			SrcAddr = Frame_pSaddr(*posMpdu);
	u1Byte			DialogToken = 0;
	BOOLEAN			bMoreData = FALSE;
	u2Byte			BytesToCopy;
	u2Byte			RemainingBytes;

	FunctionIn(COMP_P2P);
	//
	// Check status
	//
	if(!(pP2PInfo->SDContext.bDoingServiceDiscovery &&
		pP2PInfo->SDContext.bFragment))
	{// not expecting Comeback Req
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Not expect comeback request pkt!\n"));
		return RT_STATUS_INVALID_STATE;
	}

	//
	// Check peer
	//
	if(!eqMacAddr(pP2PInfo->SDContext.TargetDeviceAddress, SrcAddr))
	{
		RT_PRINT_ADDR(COMP_P2P, DBG_WARNING, "P2P_OnSDComebackReq():[WARNING] mismatch addr = ", SrcAddr);
		return RT_STATUS_INVALID_DATA;
	}	
	
	//
	// Check Length
	//
	if(posMpdu->Length < FRAME_OFFSET_GAS_COMEBACK_REQ_DIALOG_TOKEN + 1)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, 
			("[WARNING] frame lengh (%u) < min SDReq lenght (%u)\n",
			posMpdu->Length, FRAME_OFFSET_GAS_COMEBACK_REQ_DIALOG_TOKEN + 1));
		return RT_STATUS_MALFORMED_PKT;
	}
	
	//
	// Check DialogToken
	//
	DialogToken = *((pu1Byte)posMpdu->Octet + FRAME_OFFSET_GAS_COMEBACK_REQ_DIALOG_TOKEN);
	if(DialogToken != pP2PInfo->SDContext.DialogToken)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, 
			("[WARNING] Target DialogToken (%d) and mine (%d) are mismatch\n", DialogToken, pP2PInfo->SDContext.DialogToken));
		return RT_STATUS_INVALID_DATA;
	}
	
	//
	// Decide value of bMoreData
	//
	if(pP2PInfo->SDContext.ANQPQueryRspFieldToSendOffset >= 
		pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, 
			("[WARNING] no more data to send\n"));
		return RT_STATUS_INVALID_STATE;
	}
	else if(pP2PInfo->SDContext.ANQPQueryRspFieldToSendOffset< 
		pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize)
	{
		u2Byte BytesSent = pP2PInfo->SDContext.ANQPQueryRspFieldToSendOffset;
		u2Byte TotalBytes = pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize;
		
		RemainingBytes = TotalBytes - BytesSent;

		if(RemainingBytes > pP2PInfo->SDRspFragmtntThreshold)
		{
			bMoreData = TRUE;
			BytesToCopy = pP2PInfo->SDRspFragmtntThreshold;
		}
		else
		{
			bMoreData = FALSE;
			BytesToCopy = RemainingBytes;
		}
	}

	RT_TRACE_F(COMP_P2P, DBG_LOUD, 
		("Remaining bytes(%u), Bytes to Copy(%u), bMoreData(%u)\n", 
			RemainingBytes, 
			BytesToCopy,
			bMoreData));

	pP2PInfo->SDWaitForComebackReqSlotCount = 0;
	
	p2p_Send_SDComebackRsp(pP2PInfo, 
		SrcAddr, 
		DialogToken, 
		BytesToCopy, 
		bMoreData);
	
	pP2PInfo->SDContext.ANQPQueryRspFieldToSendOffset += BytesToCopy;
	pP2PInfo->SDContext.FragmentID++;
	
	if(!bMoreData)
	{
		PlatformZeroMemory(&pP2PInfo->SDContext, sizeof(P2P_SD_CONTEXT));
		CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), P2P_SERVICE_DISCOVERY_COMEBACK_TIMEOUT);
	}
	
	RT_TRACE_F(COMP_P2P, DBG_LOUD, 
		("bMoreData: %u\n", bMoreData));

	return rtStatus;
}

P2P_SD_STATUS_CODE
P2POnANQPQueryRspField(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte ANQPQueryRspFieldBuf,
	IN u2Byte ANQPQueryRspFieldSize,
	IN OUT PP2P_SERVICE_RSP_TLV ServiceRspTLVList,
	IN OUT pu1Byte pServiceRspTLVListSize
	)
{
	//
	// ANQP Query Rsp Field format:
	// InfoID(2), Length(2), OI(3), OUISubType(1), SvcUpdateIndic(2), TLVs
	//
	// Note that we can store maximul  P2P_SD_MAX_SERVICES TLVs
	//
	 
	pu1Byte pTLVStart = ANQPQueryRspFieldBuf + 2 + 2 + 3 + 1 + 2;
	pu1Byte pTLVCurrent = pTLVStart;
	u2Byte TLVLen;

	P2P_SD_STATUS_CODE Ret = P2P_SD_STATUS_SUCCESS;

	//
	// Reset the output TLV list
	//
	*pServiceRspTLVListSize = 0;

	while(TRUE)
	{
		u2Byte ResponseDataLen = 0;

		// Check if we have enough room for the TLV
		if(*pServiceRspTLVListSize >= P2P_SD_MAX_SERVICES)
		{
			break;
		}
		
		// Validate the TLV length
		if((pTLVCurrent + OFFSET_SERVICE_RSP_TLV_RSP_DATA) > 
			(ANQPQueryRspFieldBuf + ANQPQueryRspFieldSize))
		{
			Ret = P2P_SD_STATUS_BAD_RESPONSE_1;
			break;
		}
		
		// Length
		TLVLen = (ReadEF2Byte(pTLVCurrent + OFFSET_SERVICE_RSP_TLV_LEN));
		if(TLVLen < (u2Byte)(OFFSET_SERVICE_RSP_TLV_RSP_DATA - OFFSET_SERVICE_RSP_TLV_SERVICE_PROT_TYPE))
		{// invalid TLV len
			Ret = P2P_SD_STATUS_BAD_RESPONSE_2;
			break;
		}
//		if((u8Byte)(pTLVCurrent + 2 + TLVLen) > (u8Byte)(ANQPQueryRspFieldBuf + ANQPQueryRspFieldSize))
		if((pTLVCurrent + 2 + TLVLen) > (ANQPQueryRspFieldBuf + ANQPQueryRspFieldSize))
		{// invalid TLV len
			Ret = P2P_SD_STATUS_BAD_RESPONSE_3;
			break;
		}

		// Service Protocol Type
		(ServiceRspTLVList)[*pServiceRspTLVListSize].ServiceDesc.ServiceType = 
			(P2P_SD_PROTOCOL) *((pu1Byte)(pTLVCurrent + OFFSET_SERVICE_RSP_TLV_SERVICE_PROT_TYPE));

		// Status Code
		(ServiceRspTLVList)[*pServiceRspTLVListSize].Status = 
			(P2P_SD_STATUS_CODE) *((pu1Byte)(pTLVCurrent + OFFSET_SERVICE_RSP_TLV_STATUS));
		// For the last status (status of the entire SDComebackRsp), we currently get it from the last TLV
		Ret = (ServiceRspTLVList)[*pServiceRspTLVListSize].Status;
		//DbgPrint("Status: %u\n", Ret);

		// Response Data
		if((ServiceRspTLVList)[*pServiceRspTLVListSize].Status == P2P_SD_STATUS_SUCCESS)
		{
			ResponseDataLen = TLVLen - 3;
			if(ResponseDataLen > P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN)
			{// buffer overflow
				RT_TRACE(COMP_P2P,DBG_LOUD, 
					("Buffer size overflow: (%u) > (%u) \n",
					ResponseDataLen,
					P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN));
				break;
			}

			PlatformMoveMemory((ServiceRspTLVList)[*pServiceRspTLVListSize].ServiceDesc.Buffer, 
				pTLVCurrent + OFFSET_SERVICE_RSP_TLV_RSP_DATA, 
				ResponseDataLen);
			(ServiceRspTLVList)[*pServiceRspTLVListSize].ServiceDesc.BufferLength = ResponseDataLen;
			//DbgPrint("ResponseDataLen: %u\n", ResponseDataLen);
		}

		(*pServiceRspTLVListSize)++;

		// Go to next TLV
		pTLVCurrent += (5 + ResponseDataLen); // 5 = length(2) + SvcProtType(1), SvcTransactionID(1), Status(1)

		// Check if this is the last TLV
//		if((u8Byte)pTLVCurrent >= (u8Byte)(ANQPQueryRspFieldBuf + ANQPQueryRspFieldSize))
		if(pTLVCurrent >= (ANQPQueryRspFieldBuf + ANQPQueryRspFieldSize))
		{
			break;
		}
	}

	return Ret;
}

//
// Description:
//	Handle Service Discovery Response comback packet.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	None
//
RT_STATUS
P2P_OnSDComebackRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS		rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(pAdapter);
	BOOLEAN			bMoreData = FALSE;
	u1Byte			FragmentID = 0;
	u2Byte			QueryRspLen = 0;
	BOOLEAN			bBufferOut = FALSE;

	FunctionIn(COMP_P2P);
	//
	// Check state
	//
	if(pP2PInfo->State != P2P_STATE_SERVICE_DISCOVERY_COMEBACK_RSP_WAIT)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Invalid state (%d)!\n", pP2PInfo->State));
		return RT_STATUS_INVALID_STATE;
	}

	// Check if the SDRsp is from the intended peer
	if(!eqMacAddr(pP2PInfo->SDContext.TargetDeviceAddress, Frame_pSaddr(*posMpdu)))
	{
		RT_PRINT_ADDR(COMP_P2P, DBG_WARNING, "P2P_OnSDComebackRsp():[WARNING] Mismatch for src addr = ", Frame_pSaddr(*posMpdu));
		return RT_STATUS_INVALID_DATA;
	}

	//
	// Check Length
	//
	if(posMpdu->Length < FRAME_OFFSET_GAS_COMEBACK_RSP_TLV)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Invalid pkt length!\n"));
		return RT_STATUS_MALFORMED_PKT;
	}

	//
	// Check Dialog Token
	//
	if(*((pu1Byte)posMpdu->Octet + FRAME_OFFSET_GAS_COMEBACK_RSP_DIALOG_TOKEN) != 
		pP2PInfo->SDContext.DialogToken)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Pkt DialogToken (%d) and mine (%d) are mismatch\n",
				*((pu1Byte)posMpdu->Octet + FRAME_OFFSET_GAS_COMEBACK_RSP_DIALOG_TOKEN), pP2PInfo->SDContext.DialogToken));
		return RT_STATUS_INVALID_DATA;
	}

	//
	// Get FragmentID field (including MoreData bit and FragmentID)
	//
	{
		u1Byte GASQueryRspFragmentID = *((pu1Byte)posMpdu->Octet + FRAME_OFFSET_GAS_COMEBACK_RSP_FRAG_ID);

		RT_TRACE(COMP_P2P, DBG_LOUD, ("GASQueryRspFragmentID: %u\n", GASQueryRspFragmentID));
		
		if(GASQueryRspFragmentID & 0x80)
		{
			bMoreData = TRUE;
		}
		FragmentID = GASQueryRspFragmentID & (0x7F);
	}
	
	//
	// Check FragmentID
	//
	if(FragmentID != pP2PInfo->SDContext.FragmentID)
	{
		RT_TRACE(COMP_P2P, DBG_WARNING, 
			("[WARNING] expected FragmentID mismatch: expected (%u), recvd (%u)\n",
			pP2PInfo->SDContext.FragmentID, FragmentID));
		return RT_STATUS_INVALID_DATA;
	}
	else
	{
		pP2PInfo->SDContext.FragmentID++; // indicates that we are expecting the next fragment
	}
	
	//
	// Get SDComebackRsp fields
	//
	{
		// Status (11u)
		// Ignore

		// GAS Rsp Fragment ID
		// Already processed
		
		// GAS Comeback Delay
		// Ignore

		// AD Prot IE
		// Ignore

		// Query Rsp Len
		QueryRspLen = ReadEF2Byte(posMpdu->Octet + FRAME_OFFSET_GAS_COMEBACK_RSP_LEN); // ignore
		
		//
		// Defrag ANQP Query Rsp Field
		//
		if(FRAME_OFFSET_GAS_COMEBACK_RSP_QUERY_RSP + QueryRspLen <= posMpdu->Length)
		{
			if(pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize + QueryRspLen <= P2P_MAX_SERVICE_BUF_SIZE)
			{
				PlatformMoveMemory(pP2PInfo->SDContext.ANQPQueryRspFieldToSendBuf + pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize, 
					posMpdu->Octet + FRAME_OFFSET_GAS_COMEBACK_RSP_QUERY_RSP, 
					QueryRspLen);
				pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize += QueryRspLen;
			}
			else
			{
				// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
				RT_TRACE_F(COMP_P2P, DBG_SERIOUS, 
					("[ERROR] no enough buf for the ComebackRsp: buflen(%u) + QueryRspLen(%u) > bufsize(%u)\n", 
					pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize,
					QueryRspLen,
					P2P_MAX_SERVICE_BUF_SIZE));
				bBufferOut = TRUE;
			}
		}

		// Service update indicator
		if(FragmentID == 0)
		{
			pP2PInfo->SDContext.ServiceUpdateIndicator = 
				ReadEF2Byte(posMpdu->Octet + FRAME_OFFSET_GAS_COMEBACK_RSP_SERVICE_UPDATE_INDICATOR);
		}
		
		if(QueryRspLen == 0)
		{
			return rtStatus;
		}
	}

	if(bMoreData)
	{
		p2p_Send_SDComebackReq(pP2PInfo, 
			pP2PInfo->SDContext.TargetDeviceAddress,
			pP2PInfo->SDContext.DialogToken);
	}
	else
	{
		if(!bBufferOut)
		{
			RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ANQP Defrag result: ", 
				pP2PInfo->SDContext.ANQPQueryRspFieldToSendBuf, 
				pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize);
			
			pP2PInfo->SDContext.Status = P2POnANQPQueryRspField(pP2PInfo, 
				pP2PInfo->SDContext.ANQPQueryRspFieldToSendBuf, 
				pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize, 
				pP2PInfo->SDContext.ServiceRspRecvd, 
				&pP2PInfo->SDContext.ServiceRspRecvdSize);

			if(pP2PInfo->SDContext.Status == P2P_SD_STATUS_SUCCESS)
			{
				P2PIndicateOnSDRsp(pP2PInfo, &pP2PInfo->SDContext);
			}
		}

		pP2PInfo->State = P2P_STATE_SERVICE_DISCOVERY_COMPLETE;
		P2PScanListEnterScanCompleteImmediately(pP2PInfo);
	}

	RT_TRACE_F(COMP_P2P, DBG_LOUD, 
		("Status: %u, Recvd FragID: %u, nRspTLVs got from defragmented ANQP: %u\n", 
		pP2PInfo->SDContext.Status, FragmentID, pP2PInfo->SDContext.ServiceRspRecvdSize));

	return rtStatus;
} 

BOOLEAN
P2PCommonChannelArrived(
	IN PP2P_INFO pP2PInfo,
	IN P2P_DEV_LIST_ENTRY *pDev,
	IN OCTET_STRING osPacket
	)
{
	BOOLEAN bPacketTypeValid = FALSE;
	BOOLEAN bCommonChannelArrived = FALSE;
	P2P_STATE NextState = P2P_STATE_INITIALIZED;
	PP2P_DEVICE_LIST_ENTRY pP2PDeviceListEntry = NULL;
	u4Byte	BoostInitGainValue = 0;

	RT_TRACE_F(COMP_MLME, DBG_LOUD, ("p2p state 0x%x bPreGroupFormation %d bDoingProvisionDiscovery %d bToSendInvitationReqOnProbe %d\n", pP2PInfo->State, pP2PInfo->bPreGroupFormation, pP2PInfo->ProvisionDiscoveryContext.bDoingProvisionDiscovery, pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe));

	if(pP2PInfo->bPreGroupFormation)
	{// In this case, we are in pre GO Nego phase. We will start a GO Nego procedure.
		switch(P2P_FRAME_GET_TYPE(osPacket.Octet))
		{
			case Type_Probe_Req:
				bPacketTypeValid = TRUE;
				break;
			case Type_Probe_Rsp:
				bPacketTypeValid = TRUE;
				break;
			default:
				break;
		}

		if(!bPacketTypeValid)
		{
			return FALSE;
		}

		//
		// PF #3:
		//	if the target is a GO, then it replies ProbeRsp with its Interface Addr,
		//	so here, we have to compare using its Interface Address instead of Device Address.
		//	2010.05.11, haich.
		//
		if(pP2PInfo->ConnectionContext.ConnectingDevice.Role == P2P_GO)
		{
			if(!eqMacAddr(pP2PInfo->ConnectionContext.ConnectingDevice.IntendedP2PInterfaceAddress, pDev->mac))
			{
				return FALSE;
			}
		}
		else
		{
			if(!eqMacAddr(pP2PInfo->ConnectionContext.ConnectingDevice.DeviceAddress, pDev->mac))
			{
				return FALSE;
			}
		}
		
		RT_TRACE_F(COMP_MLME, DBG_LOUD, ("P2PScanListCeaseScan\n"));
		p2p_DevList_Unlock(&pP2PInfo->devList);
		P2PScanListCeaseScan(pP2PInfo);
		p2p_DevList_Lock(&pP2PInfo->devList);

		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("P2PCommonChannelArrived(): Recv ProbeRsp from the target device = > start Group Formation\n"));
		pP2PInfo->bPreGroupFormation = FALSE;

		if(pP2PInfo->State == P2P_STATE_GO_NEGO_REQ_RECVD ||
			pP2PInfo->State == P2P_STATE_GO_NEGO_RSP_SEND ||
			pP2PInfo->State == P2P_STATE_GO_NEGO_CONFIRM_WAIT ||
			pP2PInfo->State == P2P_STATE_GO_NEGO_COMPLETE)
		{
			RT_TRACE_F(COMP_MLME, DBG_LOUD, ("already do GONego!!\n"));
			pP2PInfo->bPreGroupFormation = FALSE;
			return FALSE;
		}

		NextState = P2P_STATE_GO_NEGO_REQ_SEND;

		bCommonChannelArrived = TRUE;
	}

	if(pP2PInfo->ProvisionDiscoveryContext.bDoingProvisionDiscovery)
	{
		u1Byte *pdTargetMac = NULL;
		
		if(pP2PInfo->ProvisionDiscoveryContext.go)
			pdTargetMac = pP2PInfo->ProvisionDiscoveryContext.goBssid;
		else
			pdTargetMac = pP2PInfo->ProvisionDiscoveryContext.devAddr;
		
		if(eqMacAddr(pDev->mac, pdTargetMac))
		{// source is the device we are trying to invite
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("P2PCommonChannelArrived(): Recv Probe from the provision discovery request target, state: %u\n", 
				pP2PInfo->State));			

			//
			// <Roger_Notes> Retrieve specific P2P device info. from Win8 Specific Device Information Pool to initiate boost initial gain
			// 2014.03.23.
			//
			pP2PDeviceListEntry = P2PDeviceListFind(&pP2PInfo->DeviceList, pDev->mac);
			if(pP2PDeviceListEntry)
			{	
				if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
				{
					BoostInitGainValue = TRUE;
					BoostInitGainValue = (BoostInitGainValue | ((pP2PDeviceListEntry->RecvSignalPower+110) << 8));
					pP2PInfo->pAdapter->HalFunc.SetHwRegHandler(pP2PInfo->pAdapter, HW_VAR_BOOST_INIT_GAIN_OS, (pu1Byte)&BoostInitGainValue);
					RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] P2P_STATE_PROVISION_DISCOVERY_REQ_SEND: SignalStrength(%#x)\n", pP2PDeviceListEntry->RecvSignalPower));
				}
			}

			pP2PInfo->State = P2P_STATE_PROVISION_DISCOVERY_REQ_SEND;
			CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pP2PInfo->pAdapter), 
				P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) ? P2P_PROVISION_DISCOVERY_SHORT_TIMEOUT : P2P_PROVISION_DISCOVERY_TIMEOUT
				);

			IncreaseDialogToken(pP2PInfo->DialogToken);
			p2p_Send_PDReq(pP2PInfo, pDev->mac, P2P_DEV_TYPE_GO == pDev->type, pP2PInfo->DialogToken, pDev->p2p->pdConfigMethod);

			pP2PInfo->ProvisionDiscoveryContext.bDoingProvisionDiscovery = FALSE;
			
			NextState = P2P_STATE_PROVISION_DISCOVERY_RSP_WAIT;
			
			bCommonChannelArrived = TRUE;
		}
	}

	if(pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe)
	{
		
		if(eqMacAddr(pDev->mac, pP2PInfo->InvitationContext.InvitedDevice.DeviceAddress))
		{// source is the device we are trying to invite
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("P2PCommonChannelArrived(): Recv Probe from the invitee device = > to send invitation req, state: %u\n", 
				pP2PInfo->State));

			pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe = FALSE;
			//pP2PInfo->InvitationContext.Channel = P2PScanListCeaseScan(pP2PInfo);

			pP2PInfo->State = P2P_STATE_INVITATION_REQ_SEND;			

			//
			// <Roger_Notes> Retrieve specific P2P device info. from Win8 Specific Device Information Pool to initiate boost initial gain
			// 2013.10.15.
			//
			pP2PDeviceListEntry = P2PDeviceListFind(&pP2PInfo->DeviceList, pDev->mac);
			if(pP2PDeviceListEntry)
			{	
				if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
				{
					BoostInitGainValue = TRUE;
					BoostInitGainValue = (BoostInitGainValue | ((pP2PDeviceListEntry->RecvSignalPower+110) << 8));
					pP2PInfo->pAdapter->HalFunc.SetHwRegHandler(pP2PInfo->pAdapter, HW_VAR_BOOST_INIT_GAIN_OS, (pu1Byte)&BoostInitGainValue);
					RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] P2P_STATE_INVITATION_REQ_SEND: SignalStrength(%#x)\n", pP2PDeviceListEntry->RecvSignalPower));
				}
			}
						
			CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pP2PInfo->pAdapter), P2P_INVITATION_FRAME_TIMEOUT);

			p2p_Send_InvitationReq(pP2PInfo, pDev->mac);

			NextState = P2P_STATE_INVITATION_RSP_WAIT;

			bCommonChannelArrived = TRUE;
		}
	}

	if(pP2PInfo->SDContext.bDoingServiceDiscovery)
	{
		PP2P_SD_CONTEXT pServiceDiscvoery = &(pP2PInfo->SDContext);
		
		if(eqMacAddr(pServiceDiscvoery->TargetDeviceAddress, pDev->mac))
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("P2PCommonChannelArrived(): Recv Probe from the service discovery request target, state: %u\n", 
				pP2PInfo->State));
			pP2PInfo->SDContext.bDoingServiceDiscovery = FALSE;

			//pP2PInfo->SDContext.Channel = P2PScanListCeaseScan(pP2PInfo);
			pP2PInfo->State = P2P_STATE_SERVICE_DISCOVERY_REQ_SEND;
			
			CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pP2PInfo->pAdapter), P2P_SERVICE_DISCOVERY_TIMEOUT);

			p2p_Send_SDReq(pP2PInfo, pDev->mac);
			
			NextState = P2P_STATE_SERVICE_DISCOVERY_RSP_WAIT;

			bCommonChannelArrived = TRUE;
		}
	}

	//
	// Go to the intended state immediately if common channel arrived.
	// 
	if(bCommonChannelArrived)
	{
		BOOLEAN bScanInProgress = MgntScanInProgress(&pP2PInfo->pAdapter->MgntInfo);
		u4Byte msDelayStart = 0;
		
		// Wait more time for the response packet.
		if(NextState == P2P_STATE_PROVISION_DISCOVERY_RSP_WAIT ||
			NextState == P2P_STATE_INVITATION_RSP_WAIT ||
			NextState == P2P_STATE_SERVICE_DISCOVERY_RSP_WAIT)
		{
			msDelayStart += 100; // Wait 100 ms for the rsp packet
		}
		
		if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
		{// Doing P2P Device Discovery
			msDelayStart += 10; // if scan in progress, we have to wait until ScanComplete() finishes
			//P2PScanListCeaseScan(pP2PInfo);
			P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
		}
		else if(pP2PInfo->bExtendedListening)
		{// Doing extended listening
			//P2PScanListCeaseScan(pP2PInfo);
			P2PExtendedListenComplete(pP2PInfo);
		}
		else if(bScanInProgress) 
		{// Doing normal scan
			msDelayStart += 10; // if scan in progress, we have to wait until ScanComplete() finishes
			//P2PScanListCeaseScan(pP2PInfo);
		}

		pP2PInfo->State = NextState;

		// PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer); 
		// PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, msDelayStart);
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Set P2PMgntTimer %d ms!\n", msDelayStart));
	}

	return bCommonChannelArrived;
}

//======================================================================
// P2P Managmement
//======================================================================

VOID
P2PInitializeChannelEntryList(
	PP2P_INFO pP2PInfo
)
{
	//
	// Channel entry list.
	// Set to the channels we support by default.
	//
	
	PADAPTER pAdapter = pP2PInfo->pAdapter;
	u1Byte i;
	u2Byte SupportedWirelessMode = pAdapter->HalFunc.GetSupportedWirelessModeHandler(pAdapter);
	WIRELESS_MODE CurrentWirelessModeBackup = pAdapter->MgntInfo.dot11CurrentWirelessMode;
	PRT_CHANNEL_LIST pChannelList = NULL;
	u1Byte chList[CHANNEL_MAX_NUMBER] = {0};
	u1Byte nChList = 0;

	pAdapter->MgntInfo.dot11CurrentWirelessMode = SupportedWirelessMode;
	RtActChannelList(pAdapter, RT_CHNL_LIST_ACTION_CONSTRUCT, NULL, NULL);
	RtActChannelList(pAdapter, RT_CHNL_LIST_ACTION_GET_CHANNEL_LIST, NULL, &pChannelList);
	pAdapter->MgntInfo.dot11CurrentWirelessMode = CurrentWirelessModeBackup;

	//
	// Init channel entry list
	//
	pAdapter->MgntInfo.ChannelPlan = GetDefaultAdapter(pAdapter)->MgntInfo.ChannelPlan;
		
	PlatformZeroMemory(&pP2PInfo->ChannelEntryList, sizeof(pP2PInfo->ChannelEntryList));

	pChannelList = GET_RT_CHANNEL_LIST(&pAdapter->MgntInfo);//MgntActQuery_ChannelList(pP2PInfo->pAdapter);
		
	for(i = 0; i < pChannelList->ChannelLen; i++)
	{
		chList[nChList++] = pChannelList->ChnlListEntry[i].ChannelNum;
	}

	MgntActSet_P2PChannelList(pAdapter, nChList, chList);
}

VOID
P2PInitialize(
	IN PP2P_INFO pP2PInfo,
	IN PADAPTER pAdapter,
	IN u1Byte ListenChannel,
	IN u1Byte IntendedOpChannel,
	IN u1Byte GOIntent
	) 
{
	BOOLEAN		bBoostIgi = FALSE;
	BOOLEAN		bSupportHwP2pPs = FALSE;
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);

	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PInitialize(): \n"));
	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "CurrentAddress: ", pAdapter->CurrentAddress);

	pP2PInfo->P2PVersion = 1;
	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PVersion initialized to %u\n", pP2PInfo->P2PVersion));

	if(pAdapter == GetDefaultAdapter(pAdapter)) // Default adapter means this is Realtek P2P
	{
		pAdapter->P2PSupport = P2P_SUPPORT_STATE_RTK_SUPPORT;
	}
	else
	{
		pAdapter->P2PSupport = P2P_SUPPORT_STATE_OS_SUPPORT;
	}
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2P Support Type = %s\n", (P2P_SUPPORT_STATE_RTK_SUPPORT == pAdapter->P2PSupport) ? "RTK_SUPPORT" : "OS_SUPPORT"));
	// Win8: Point to self adapter
	pP2PInfo->pAdapter = pAdapter;	

	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PInitialize(): port number %d\n", pP2PInfo->pAdapter->pNdis62Common->PortNumber));

	P2PSetRole(pP2PInfo, P2P_NONE);
 	pP2PInfo->State = P2P_STATE_INITIALIZED;
	pP2PInfo->PreviouslyIndicatedState = P2P_STATE_MAX;
	pP2PInfo->PreviouslyIndicateStateTime = 0;

	if(P2P_ADAPTER_RTK_SUPPORT_P2P(pAdapter))
	{
		bBoostIgi = TRUE;
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_BOOST_INIT_GAIN, &bBoostIgi);
	}

	cpMacAddr(pP2PInfo->DeviceAddress, pAdapter->CurrentAddress);
	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "pP2PInfo->DeviceAddress: ", pP2PInfo->DeviceAddress);

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
	{
		// Win8: Let the interface address be the permanent address
		cpMacAddr(pP2PInfo->InterfaceAddress, pAdapter->PermanentAddress);
	}
	else 
	{
		// In Win7: pAdapter->CurrentAddress == pAdapter->PermanentAddress
		cpMacAddr(pP2PInfo->InterfaceAddress, pAdapter->CurrentAddress);
	}

	//
	// Capability
	//
	pP2PInfo->DeviceCapability = 0;
	pP2PInfo->DeviceCapability |= dcServiceDiscovery;
	pP2PInfo->DeviceCapability |= dcP2PClientDiscoverability;
	pP2PInfo->DeviceCapability |= dcP2PInvitationProcedure;
	//pP2PInfo->DeviceCapability |= dcP2PInfrastructureManaged;

	pP2PInfo->GroupCapability = 0;
	pP2PInfo->GroupCapability |= gcCrossConnection;
	pP2PInfo->GroupCapability |= gcIntraBSSDistribution;
	pP2PInfo->GroupCapability |= gcPersistentP2PGroup;

	//
	// GO Intent
	//
	pP2PInfo->GOIntent = GOIntent;

	//
	// Channel
	//
	pP2PInfo->RegulatoryClass = 81;

	if(0 == MultiChannelGetConnectedChannels(pAdapter, &pP2PInfo->OperatingChannel, 1))
	{
		pP2PInfo->OperatingChannel = IntendedOpChannel;
	}
	
	pP2PInfo->ListenChannel = ListenChannel;

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("GOIntent = %d, OP Chnl = %d, ListenChnl = %d\n", pP2PInfo->GOIntent, pP2PInfo->OperatingChannel, pP2PInfo->ListenChannel));

	if(FALSE == P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter) || pAdapter->pNdis62Common->PortType == EXT_P2P_DEVICE_PORT)
	{
		MgntActSet_P2PListenChannel(pAdapter, ListenChannel);
	}
	
	pP2PInfo->CountryString[0] = 0x55; //US
	pP2PInfo->CountryString[1] = 0x53;
	pP2PInfo->CountryString[2] = 0x04;

	//
	// Channel entry list.
	// Set to the channels we support by default.
	//
	P2PInitializeChannelEntryList(pP2PInfo);

	//
	//WPS
	//
	{
		//
		// Not to clear the WPS IE because we should use the one set by normal UI,
		// The testing UI does not know the exact WPS IE.
		// 
		//pP2PInfo->WpsInfoLen = 0;

		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PInitialize(): primary dev type: %u\n", 
			pP2PInfo->WpsAttributes.PrimaryDeviceType.CategoryId));
		RT_PRINT_STR(COMP_P2P, DBG_LOUD, "P2PInitialize(): Device Name:\n", 
			pP2PInfo->WpsAttributes.DeviceName, pP2PInfo->WpsAttributes.DeviceNameLength);
	}

	// Configuration Timeout, in units of 10ms
	pP2PInfo->GOConfigurationTimeout = P2P_GO_CONFIG_TIMEOUT;
	pP2PInfo->ClientConfigurationTimeout = P2P_CLIENT_CONFIG_TIMEOUT;

	//
	// Scan List
	//
	pP2PInfo->ScanListSize = 0;
	PlatformZeroMemory(pP2PInfo->ScanList, P2P_MAX_SCAN_LIST * sizeof(P2P_DEVICE_DISCRIPTOR));
	pP2PInfo->ScanList4QuerySize = 0;
	PlatformZeroMemory(pP2PInfo->ScanList4Query, P2P_MAX_SCAN_LIST * sizeof(P2P_DEVICE_DISCRIPTOR));
	pP2PInfo->ForceScanListIndicateSlotCount = 0;
	pP2PInfo->bDeviceDiscoveryInProgress = 0;

	pP2PInfo->P2PGONoClientSlotCount = 0;
	pP2PInfo->P2PClientDisconnectedSlotCount = 0;
	pP2PInfo->ExtListenTimingPeriodSlotCount = 0;
	pP2PInfo->bExtendedListening = FALSE;

	pP2PInfo->DialogToken = 1;

	PlatformZeroMemory(&pP2PInfo->ConnectionContext, sizeof(P2P_CONNECTION_CONTEXT));
	pP2PInfo->bReinitiateConnection= FALSE;
	pP2PInfo->TimeStartWaitingForReinitiate = 0;
	pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)0;
	
	PlatformZeroMemory(&pP2PInfo->InvitationContext, sizeof(P2P_INVITATION_CONTEXT));
	pP2PInfo->bAcceptInvitation = FALSE;
	PlatformZeroMemory(pP2PInfo->AccpetInvitationDeviceAddress, 6);
	PlatformZeroMemory(&pP2PInfo->DeviceDiscoverabilityContext, sizeof(P2P_DEVICE_DISCOVERABILITY_CONTEXT));

	pP2PInfo->ExtListenTimingPeriod = P2P_EXT_LISTEN_TIMING_PERIOD_MS;
	pP2PInfo->ExtListenTimingDuration = P2P_EXT_LISTEN_TIMING_DURATION_MS;

	pP2PInfo->bSendProbeReqInExtendedListen = FALSE;

	PlatformZeroMemory(&pP2PInfo->ProvisionDiscoveryContext, sizeof(P2P_PROVISION_DISCOVERY_CONTEXT));

	// Service Discovery
	PlatformZeroMemory(&pP2PInfo->SDContext, sizeof(P2P_SD_CONTEXT));
	pP2PInfo->SDRspFragmtntThreshold = P2P_SERVICE_MAX_RSP_FRAG_THRESHOLD;
	pP2PInfo->SDWaitForComebackReqSlotCount = 0;
		
	//PlatformZeroMemory(&pP2PInfo->PersistentProfile, sizeof(P2P_PERSISTENT_PROFILE));

	// ICS
	pP2PInfo->CurrentIcsStatus = P2P_CURRENT_ICS_STATUS_UNKNOWN;

	// Profile list
	pP2PInfo->profileListLen = 0;
	pP2PInfo->pProfileList = NULL;

	//======================================================================
	// Power Save related
	//======================================================================
	// Clear the PS level first.
	RT_CLEAR_PS_LEVEL(pAdapter, RT_RF_LPS_P2P_PS);
	pP2PInfo->bUpdateFromBeacon = FALSE;
	pP2PInfo->bUpdatePsParameter = FALSE;
	pP2PInfo->PsFlag = 0;
	pP2PInfo->NextTimeout = 0;
	pP2PInfo->P2pPsState = P2P_PS_AWAKE;
	pP2PInfo->NoAIEIndex = 0;
	pP2PInfo->CTWindow = 0;
	pP2PInfo->bOppPS = 0;
	//pP2PInfo->SSIDLen = 0;
	PlatformZeroMemory(pP2PInfo->NoADescriptors, sizeof(P2P_NOA_DESCRIPTOR) * 2);

	// Determine whether this chip can support P2P ps mode.
	pP2PInfo->psExeType = RT_P2P_PS_EXE_BY_NONE;
	if(!pAdapter->HalFunc.GetHalDefVarHandler(pAdapter, HAL_DEF_HW_P2P_PS_SUPPORT, (pu1Byte)&bSupportHwP2pPs))
	{
		bSupportHwP2pPs = FALSE;
	}
	if(bSupportHwP2pPs && pAdapter->MgntInfo.bWiFiConfg)
	{
		pP2PInfo->psExeType = RT_P2P_PS_EXE_BY_HW;
	}
	else if(bSupportHwP2pPs && GetDefaultAdapter(pAdapter)->MgntInfo.RegMultiChannelFcsMode >= MULTICHANNEL_FCS_SUPPORT_GO)
	{
		pP2PInfo->psExeType = RT_P2P_PS_EXE_BY_HW;
	}
	else
	{
		pP2PInfo->psExeType = RT_P2P_PS_EXE_BY_SW_TIMER;
	}

	if(pP2PInfo->psExeType > RT_P2P_PS_EXE_BY_HW)
	{
		PRT_POWER_SAVE_CONTROL		pPSC = GET_POWER_SAVE_CONTROL(pMgntInfo);
		pPSC->bFwCtrlLPS = FALSE;
	}

	//======================================================================
	// P2P Managed
	//======================================================================
	pP2PInfo->bWlanApRejection_CrossConnection = FALSE;
	pP2PInfo->bWlanApRejection_Unmanaged = FALSE;
	pP2PInfo->bWlanApRejection_IncompatibleCoexistenceParameters = FALSE;
	pP2PInfo->bWlanApRejection_IncompatibleP2POperation = FALSE;

	//======================================================================
	// Other
	//======================================================================

	P2PSetPsState(pP2PInfo, P2P_PS_CANCEL_ALL_PS, P2P_PS_AWAKE, 0);

	if(pAdapter->MgntInfo.bDisableCck)
	{
		SelectRateSet(pAdapter, pAdapter->MgntInfo.Regdot11OperationalRateSet);
	}

	//
	// Ask for disconnection, note that these function can only be called when P2PInfo is initialized
	//
	//P2PIndicateStopApRequest(pP2PInfo);
	//P2PIndicateDisconnectClientRequest(pP2PInfo);

	P2P_AddIe_Init(&pP2PInfo->AdditionalIEs);

	p2p_DevList_Init(&pP2PInfo->devList);

	// For Win 8: OID_DOT11_WFD_LISTEN_STATE_DISCOVERABILITY  -----------------------------
	//	The macro P2P_ENABLED() will be modified due to the Win 8.
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
	{
		pP2PInfo->uListenStateDiscoverability = 0;		// Zero for not discoverable
	}
	else
	{
		pP2PInfo->uListenStateDiscoverability = 100;		// Set a non-zero value
	}
	// -----------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_DISCOVER_REQUEST ------------------------------------------------------
	pP2PInfo->DiscoverSequence = P2P_DISCOVERY_SCAN_PHASE | P2P_DISCOVERY_FIND_PHASE;
	pP2PInfo->ScanType = SCAN_ACTIVE;
	P2PClearScanDeviceID(&pP2PInfo->ScanDeviceIDs);
	pP2PInfo->bForceScanLegacyNetworks = FALSE;
	pP2PInfo->bDeviceDiscoveryIndicateToOS = FALSE;
	pP2PInfo->bDiscoverForSpecificChannels = FALSE;
	PlatformZeroMemory(pP2PInfo->DiscoverForSpecificChannels, sizeof(pP2PInfo->DiscoverForSpecificChannels));
	pP2PInfo->uNumberOfDiscoverForSpecificChannels = 0;
	pP2PInfo->uNumberOfDiscoverRounds = 0;
	
	// + Win8 Trick to Speed up GO Negotiation Procedure
	pP2PInfo->LastDeviceDiscoveryOidIssueTime = 0;
	pP2PInfo->LastDeviceDiscoveryIndicatedTime = 0;
	// ------------------------------------------------------------------------------------------------

	// Device List -------------------------------------------------------------------------
	PlatformZeroMemory(&pP2PInfo->DeviceList, sizeof(P2P_DEVICE_LIST));
	PlatformZeroMemory(&pP2PInfo->DeviceListForQuery, sizeof(P2P_DEVICE_LIST));
	//-----------------------------------------------------------------------------------
	
	// For Win 8: OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_REQUEST --------------------------------------	
	pP2PInfo->ProvisionRequestGroupCapability = 0;
	pP2PInfo->bProvisionRequestUseGroupID = FALSE;
	PlatformZeroMemory(pP2PInfo->ProvisionRequestGroupIDDeviceAddress, 6);
	PlatformZeroMemory(pP2PInfo->ProvisionRequestGroupIDSSID, MAX_SSID_LEN);
	pP2PInfo->uProvisionRequestGroupIDSSIDLength = 0;

	// -------------------------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_PROVISION_DISCOVERY_RESPONSE --------------------------------------	
	PlatformZeroMemory(pP2PInfo->ProvisionResponseReceiverDeviceAddress, 6);
	pP2PInfo->ProvisionResponseDialogToken = 0;
	// -------------------------------------------------------------------------------------------------

	// For delaying operations from the OID command ---------------------------------------------------------
	pP2PInfo->bPostoneP2POidPostProcessWorkItem = FALSE;
	// RT_WORK_ITEM P2POidPostProcessWorkItem; Initialized in MgntInitializeAllWorkItem and MgntFreeAllWorkItem
	pP2PInfo->OidOperation = OID_OPERATION_NO_OPERATION;
	pP2PInfo->PacketSentInWorkItemCallback = (P2P_PUBLIC_ACTION_TYPE) 0xFF;		// Send no packet
	//-------------------------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_GO_NEGOTIATION_REQUEST ---------------------------
	//PlatformZeroMemory(pP2PInfo->NegotiationRequestIntendedInterfaceAddress, 6);
	pP2PInfo->NegotiationRequestGOIntent = 0;
	pP2PInfo->NegotiationRequestGroupCapability = 0;
	// ---------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_GO_NEGOTIATION_RESPONSE ------------------------
	PlatformZeroMemory(pP2PInfo->NegotiationResponsePeerDeviceAddress, 6);
	pP2PInfo->NegotiationResponseDialogToken = 0;
	pP2PInfo->NegotiationResponseStatus = 0;
	pP2PInfo->NegotiationResponseGroupOwnerIntent = 0;
	//PlatformZeroMemory(pP2PInfo->NegotiationResponseIntendedInterfaceAddress, 6);
	pP2PInfo->NegotiationResponseGroupCapability = 0;
	PlatformZeroMemory(pP2PInfo->NegotiationResponseGroupIDDeviceAddress, 6);
	PlatformZeroMemory(pP2PInfo->NegotiationResponseGroupIDSSID, MAX_SSID_LEN);
	pP2PInfo->uNegotiationResponseGroupIDSSIDLength = 0;
	pP2PInfo->bNegotiationResponseUseGroupID = FALSE;

	//---------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_GO_NEGOTIATION_CONFIRM ---------------------------
	PlatformZeroMemory(pP2PInfo->NegotiationConfirmPeerDeviceAddress, 6);
	pP2PInfo->NegotiationConfirmDialogToken = 0;
	pP2PInfo->NegotiationConfirmStatus = 0;
	pP2PInfo->NegotiationConfirmGroupCapability = 0;
	PlatformZeroMemory(pP2PInfo->NegotiationConfirmGroupIDDeviceAddress, 6);
	PlatformZeroMemory(pP2PInfo->NegotiationConfirmGroupIDSSID, MAX_SSID_LEN);
	pP2PInfo->uNegotiationConfirmGroupIDSSIDLength = 0;
	pP2PInfo->bNegotiationConfirmUseGroupID = FALSE;

	//---------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_INVITATION_REQUEST -------------------------------
	PlatformZeroMemory(pP2PInfo->InvitationRequestGroupBSSID, 6);
	pP2PInfo->bInvitationRequestUseGroupBSSID = FALSE;
	pP2PInfo->uInvitationRequestOperatingChannelNumber = 0;
	pP2PInfo->bInvitationRequestUseSpecifiedOperatingChannel = FALSE;
	PlatformZeroMemory(pP2PInfo->InvitationRequestGroupIDDeviceAddress, 6);
	PlatformZeroMemory(pP2PInfo->InvitationRequestGroupIDSSID, MAX_SSID_LEN);
	pP2PInfo->uInvitationRequestGroupIDSSIDLength = 0;
	pP2PInfo->bInvitationRequestLocalGO = FALSE;

	//---------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_SEND_INVITATION_RESPONSE ------------------------------
	PlatformZeroMemory(pP2PInfo->InvitationResponseReceiverDeviceAddress, 6);
	pP2PInfo->InvitationResponseDialogToken = 0;
	pP2PInfo->InvitationResponseStatus = 0;
	PlatformZeroMemory(pP2PInfo->InvitationResponseGroupBSSID, 6);
	pP2PInfo->bInvitationResponseUseGroupBSSID = FALSE;
	pP2PInfo->uInvitationResponseOperatingChannelNumber = 0;
	pP2PInfo->bInvitationResponseUseSpecifiedOperatingChannel = FALSE;
	pP2PInfo->bInvitationResponseIndicateToOS = FALSE;
	//---------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_GROUP_JOIN_PARAMETERS ------------------------------------
	P2PResetClientJoinGroupContext(pP2PInfo);
	// -----------------------------------------------------------------------------------

	// For Win 8: OID_DOT11_WFD_DESIRED_GROUP_ID ------------------------------------------
	PlatformZeroMemory(pP2PInfo->DesiredTargetMacAddress, sizeof(pP2PInfo->DesiredTargetMacAddress));
	pP2PInfo->uGroupTargetSSIDLength = 0;
	PlatformZeroMemory(pP2PInfo->GroupTargetSSID, sizeof(pP2PInfo->GroupTargetSSID));
	// -----------------------------------------------------------------------------------


	// Debug Device Discovery  -------------------
	pP2PInfo->ProbeRequestSequenceNum = 0;
	// ----------------------------------------
	
	// Trick for knowing next channel switch time -----------
	pP2PInfo->TimeStartToStopSendingProbeResponse = 0;
	// -----------------------------------------------


	// Start Mgnt Timer -----------------------------------------------------------------
	PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
	if(!pP2PInfo->pAdapter->bInHctTest)
		PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);
	// --------------------------------------------------------------------------------

	P2PSetRole(pP2PInfo, P2P_DEVICE);

	RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PInitialize()\n"));
}

VOID
P2PResetCommonChannelArrivingProcess(
	IN PP2P_INFO pP2PInfo
	)
{
	FunctionIn(COMP_P2P);
	// connection
	pP2PInfo->bPreGroupFormation = FALSE;
	PlatformZeroMemory(&pP2PInfo->ConnectionContext, sizeof(P2P_CONNECTION_CONTEXT));

	// invitation
	pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe = FALSE;
	PlatformZeroMemory(&pP2PInfo->InvitationContext, sizeof(P2P_INVITATION_CONTEXT));

	// provision discovery
	pP2PInfo->ProvisionDiscoveryContext.bDoingProvisionDiscovery = FALSE;
	PlatformZeroMemory(&pP2PInfo->ProvisionDiscoveryContext, sizeof(P2P_PROVISION_DISCOVERY_CONTEXT));

	// sd
	pP2PInfo->SDContext.bDoingServiceDiscovery = FALSE;
	PlatformZeroMemory(&pP2PInfo->SDContext, sizeof(P2P_SD_CONTEXT));

	FunctionOut(COMP_P2P);	
}

//
// This function guarantees to preserve:
//	* P2P State
//	* Channel
//
BOOLEAN
P2PDeviceDiscovery(
	IN PP2P_INFO pP2PInfo,
	IN u1Byte FindPhaseLoopTimes
	)
{
	BOOLEAN bScanInProgress;
	u4Byte msDelayStart = 0;
	BOOLEAN bToIndicDevDiscComp = FALSE;
	
	if(!P2P_ENABLED(pP2PInfo))
	{
		return FALSE;
	}
	
	//
	// Stop any existing scan
	//
	bScanInProgress = MgntScanInProgress(&pP2PInfo->pAdapter->MgntInfo);

	RT_TRACE(COMP_P2P, DBG_LOUD, 
			("P2PDeviceDiscovery(): State: %u, bScanInProgres: %u port number %d\n",
			pP2PInfo->State, bScanInProgress, pP2PInfo->pAdapter->pNdis62Common->PortNumber));

	if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
	{// Doing P2P Device Discovery
		msDelayStart += 10; // if scan in progress, we have to wait until ScanComplete() finishes
		P2PScanListCeaseScan(pP2PInfo);
		P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
		bToIndicDevDiscComp = TRUE;
	}
	else if(pP2PInfo->bExtendedListening)
	{// Doing extended listening
		P2PScanListCeaseScan(pP2PInfo);
		P2PExtendedListenComplete(pP2PInfo);
	}
	else if(bScanInProgress) 
	{// Doing normal scan
		msDelayStart += 10; // if scan in progress, we have to wait until ScanComplete() finishes
		P2PScanListCeaseScan(pP2PInfo);
	}

	if(FindPhaseLoopTimes == P2P_DEV_DISC_STOP)
	{
		if(bToIndicDevDiscComp && P2P_DOING_PURE_DEVICE_DISCOVERY(pP2PInfo))
		{
			P2PIndicateDeviceDiscoveryComplete(pP2PInfo);
		}
		return TRUE;
	}

	pP2PInfo->bDeviceDiscoveryInProgress = TRUE;
	pP2PInfo->StateBeforeScan = pP2PInfo->State;

	if(pP2PInfo->Role == P2P_GO)
	{
		P2PStopResumeGOBeaconning(pP2PInfo, FALSE);	
	}

	//
	// Clear scan list before scan.
	// If we are a GO, we shall not clear our scan list since when a client associates, 
	// it provides its info to us in AssocReq, and we record the info in our scan list.
	// If we clear this info, we will lost the info of the associated clients.
	//
	if(pP2PInfo->Role != P2P_GO &&
		P2P_DOING_PURE_DEVICE_DISCOVERY(pP2PInfo)) // Can't use P2P_ACTING_AS_GO since the state now is P2P_STATE_SCAN, not operating
	{
		P2PScanListClear(pP2PInfo);
	}
	
	PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_P2P_SPIN_LOCK);
	P2P_INC_REF_CNT(pP2PInfo);
	PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_P2P_SPIN_LOCK);
	
//	pP2PInfo->StateBeforeScan = pP2PInfo->State;
	
	pP2PInfo->State = P2P_STATE_DEV_DISC_START;
//	pP2PInfo->bDeviceDiscoveryInProgress = TRUE;

	pP2PInfo->ConnectionContext.FindPhaseLoopTimes = FindPhaseLoopTimes;

	RT_TRACE(COMP_P2P, DBG_LOUD, 
		("P2PDeviceDiscovery(): delay start after %d (ms), StateBeforeScan: %u\n", 
		msDelayStart, pP2PInfo->StateBeforeScan));
	
	//if(MgntActQuery_802_11_CHANNEL_NUMBER((GetDefaultAdapter(pP2PInfo->pAdapter))) != pP2PInfo->ListenChannel)
	//{
	//	// Reset Listen Channel to see if we have chance to switch current channel back to the Listen Channel.
	//	MgntActSet_P2PListenChannel(pP2PInfo->pAdapter, pP2PInfo->ListenChannel);
	//}

	PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);

	PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_P2P_SPIN_LOCK);
	P2P_DEC_REF_CNT(pP2PInfo);
	PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_P2P_SPIN_LOCK);
	PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, msDelayStart);
	return TRUE;
}

VOID
P2PDeviceDiscoverForSpecificChannels(
	PP2P_INFO pP2PInfo,
	pu1Byte ChannelList,
	u1Byte uNumberOfChannel
)
{
	VOID						*customScanInfo = GET_CUSTOM_SCAN_INFO(pP2PInfo->pAdapter);
	VOID 						*req = NULL;
	FRAME_BUF					*probeReqBuf = NULL;
	u4Byte						itChnl = 0;

	if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
	{
		return;
	}

	probeReqBuf = CustomScan_GetProbeReqBuf(req);
	p2p_Construct_ProbeReq(probeReqBuf, pP2PInfo);

	for(itChnl = 0; itChnl < uNumberOfChannel; itChnl++)
	{
		CustomScan_AddScanChnl(req, ChannelList[itChnl], 3, SCAN_ACTIVE, 20, MGN_6M, probeReqBuf);
	}

	CustomScan_SetupCbCtx(req, p2p_ScanStateCb, pP2PInfo->pAdapter);
	CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "spec chnl disc");

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2PDeviceDiscoverForSpecificChannels issued\n"));	

	return;		
}

VOID
P2PDeviceDiscoveryComplete(
	IN PP2P_INFO pP2PInfo,
	IN BOOLEAN	bRecoverState
	)
{	
	// 
	// We do some clean up task here.
	//
	
	// For Win8 P2P Indication ------------------------------------------------------------
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) && pP2PInfo->bDeviceDiscoveryIndicateToOS == TRUE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: P2P_EVENT_DEVICE_DISCOVERY_COMPLETE\n", __FUNCTION__));
		PlatformIndicateP2PEvent(pP2PInfo, P2P_EVENT_DEVICE_DISCOVERY_COMPLETE, NULL);

		// Trick Adopted for Speeding up the Go Negotiation
		pP2PInfo->LastDeviceDiscoveryIndicatedTime = PlatformGetCurrentTime();

		P2PClearScanDeviceID(&pP2PInfo->ScanDeviceIDs);
		pP2PInfo->bDeviceDiscoveryIndicateToOS = FALSE;

		pP2PInfo->bDiscoverForSpecificChannels = FALSE;
		PlatformZeroMemory(
				pP2PInfo->DiscoverForSpecificChannels, 
				sizeof(pP2PInfo->DiscoverForSpecificChannels)
			);
		pP2PInfo->uNumberOfDiscoverForSpecificChannels = 0;
		pP2PInfo->uNumberOfDiscoverRounds = 0;
	}
	// --------------------------------------------------------------------------------


	if(pP2PInfo->Role == P2P_GO)
	{
		P2PStopResumeGOBeaconning(pP2PInfo, TRUE);
	}
	
	
	if(bRecoverState)
	{
		// discovery complete, back to original state
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("P2PDeviceDiscoveryComplete(): P2P_STATE_DEV_DISC_COMPLETE: back to state (%u) from (%u)\n",
			pP2PInfo->StateBeforeScan, pP2PInfo->State));
		pP2PInfo->State = pP2PInfo->StateBeforeScan;
	}

	pP2PInfo->bDeviceDiscoveryInProgress = FALSE;

	P2PSvc_OnDevDiscComplete(pP2PInfo->pP2PSvcInfo);

}
	

VOID
P2PExtendedListenStart(
	IN PP2P_INFO pP2PInfo
	)
{
	PADAPTER pAdapter = pP2PInfo->pAdapter;

	VOID						*customScanInfo = GET_CUSTOM_SCAN_INFO(pAdapter);
	VOID 						*req = NULL;
	FRAME_BUF					*probeReqBuf = NULL;
	BOOLEAN						bActiveScan = pP2PInfo->bSendProbeReqInExtendedListen;

	if(MgntIsLinkInProgress(GetDefaultMgntInfo(pP2PInfo->pAdapter)))
	{
		return;
	}
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PExtendedListenStart()\n"));

#if (MULTICHANNEL_SUPPORT == 1)
	// Do not support 3 channel swtich: This will mess up our multichannl swtich -----------------------------------------------
	if(MultiChannelSwitchNeeded(pAdapter))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Do not support extended listening on MultiChannel environment\n", __FUNCTION__));
		FunctionOut(COMP_P2P);
		return;
	}
	// -------------------------------------------------------------------------------------------------------------
#endif

	// Do not use extended listening if group is constructed  ------------------------------------------------------
	if(GetFirstGOPort(pAdapter) || GetFirstClientPort(pAdapter))
	{
		ADAPTER *cli = GetFirstClientPort(pAdapter);
		
		if(cli && GET_P2P_INFO(cli)
			&& (P2P_CLIETN_JOIN_GROUP_WPS_STATE_SCANNING == (GET_P2P_INFO(cli))->ClientJoinGroupContext.WpsState
			|| P2P_CLIENT_JOIN_GROUP_WPS_STATE_GO_READY == (GET_P2P_INFO(cli))->ClientJoinGroupContext.WpsState
			|| P2P_CLIETN_JOIN_GROUP_WPS_STATE_ASSOCIATING == (GET_P2P_INFO(cli))->ClientJoinGroupContext.WpsState
			))
		{// don't do scan in this case
			RT_TRACE_F(COMP_P2P, DBG_LOUD, 
				("do not use extended listen when cli port is doing WPS join group, state: %u\n", 
				(GET_P2P_INFO(cli))->ClientJoinGroupContext.WpsState));
			FunctionOut(COMP_P2P);
			return;
		}
	}
	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		ADAPTER *pDev = GetFirstDevicePort(pP2PInfo->pAdapter);

		if(pDev)
		{
			P2P_INFO *p2p = GET_P2P_INFO(pP2PInfo->pAdapter);

			if(!p2p->uListenStateDiscoverability)
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("%s:Do not use extended listening if not set to be discoverable\n", __FUNCTION__));
				FunctionOut(COMP_P2P);
				return;
			}
		}
	}
	// --------------------------------------------------------------------------------------------------

	RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: listen channel: %d, Current Channel: %d\n", 
			__FUNCTION__, pP2PInfo->ListenChannel, RT_GetChannelNumber(pAdapter))
		);

	// At the same channel, no need to scan and switch channel.
	if(pP2PInfo->ListenChannel == RT_GetChannelNumber(pAdapter))
	{
		// RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: In the Same Channel: %d\n", __FUNCTION__, pP2PInfo->ListenChannel));
		return;
	}

	pP2PInfo->bExtendedListening = TRUE; // set to false in ScanComplete()

	if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
	{
		return;
	}

	probeReqBuf = CustomScan_GetProbeReqBuf(req);

	p2p_Construct_ProbeReq(probeReqBuf, pP2PInfo);

	CustomScan_AddScanChnl(req, 
		pP2PInfo->ListenChannel, 1, 
		(pP2PInfo->bSendProbeReqInExtendedListen) ? SCAN_ACTIVE : SCAN_PASSIVE, 
		pP2PInfo->ExtListenTimingDuration, P2P_LOWEST_RATE, probeReqBuf);

	CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "ext listen");
	
	FunctionOut(COMP_P2P);
	
}

VOID
P2PExtendedListenResetCounter(
	IN PP2P_INFO pP2PInfo
)
{
	FunctionIn(COMP_P2P);
	pP2PInfo->ExtListenTimingPeriodSlotCount = 0;
}

VOID
P2PExtendedListenComplete(
	IN PP2P_INFO pP2PInfo
	)
{
	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PExtendedListenComplete()\n"));
	pP2PInfo->bExtendedListening = FALSE;

	if(MgntActQuery_802_11_CHANNEL_NUMBER((GetDefaultAdapter(pP2PInfo->pAdapter))) != pP2PInfo->ListenChannel && 
		!MgntIsLinkInProgress(GetDefaultMgntInfo(pP2PInfo->pAdapter)))
	{
		// Reset Listen Channel to see if we have chance to switch current channel back to the Listen Channel.
		MgntActSet_P2PListenChannel(pP2PInfo->pAdapter, pP2PInfo->ListenChannel);
	}

	FunctionOut(COMP_P2P);
}

BOOLEAN
P2PProvisionDiscovery(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte MacAddress, // shall be the device address
	IN u2Byte ConfigMethod // should have exactly 1 bit set
	)
{
	PP2P_PROVISION_DISCOVERY_CONTEXT pProvisionDiscContext = &pP2PInfo->ProvisionDiscoveryContext;
	P2P_DEV_LIST_ENTRY *pDev = NULL;

	//
	// Check state
	//
	if(pP2PInfo->State > P2P_STATE_DEV_DISC_COMPLETE
		&& FALSE == P2P_ACTING_AS_GO(pP2PInfo)
		)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PDeviceDiscovery(): Invalid state: %u\n",pP2PInfo->State));
		return FALSE;
	}

	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PProvisionDiscovery(): ConfigMethod: 0x%X\n", ConfigMethod));

	p2p_DevList_Lock(&pP2PInfo->devList);

	do
	{
		P2P_DEVICE_DISCRIPTOR desc;
		const u1Byte *devAddr = NULL;

		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			if(pP2PInfo->bProvisionRequestUseGroupID)
				pDev = p2p_DevList_GetGo(&pP2PInfo->devList, MacAddress);
			else
				pDev = p2p_DevList_Get(&pP2PInfo->devList, MacAddress, P2P_DEV_TYPE_DEV);

			if(NULL == pDev)
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PProvisionDiscovery(): device entry not found\n"));
				p2p_DevList_Dump(&pP2PInfo->devList);
				break;
			}
		}
		else
		{
		if(NULL == (pDev = p2p_DevList_GetGo(&pP2PInfo->devList, MacAddress)))
		{
			if(NULL == (pDev = p2p_DevList_Get(&pP2PInfo->devList, MacAddress, P2P_DEV_TYPE_DEV)))
			{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PProvisionDiscovery(): device entry not found\n"));
				p2p_DevList_Dump(&pP2PInfo->devList);
				break;
				}
			}
		}
		
		p2p_DevList_TranslateDev(pDev, &desc);

		devAddr = desc.DeviceAddress;
	
	//
	// For Sigma
	//
	if(ConfigMethod == 0)
	{
			if(desc.WpsAttributes.DevicePasswdId == P2P_WPS_DEV_PASSWD_ID_PBC)
			ConfigMethod = P2P_WPS_CONFIG_METHODS_PUSHBUTTON;
			else if(desc.WpsAttributes.DevicePasswdId == P2P_WPS_DEV_PASSWD_ID_USER_SPEC)
			ConfigMethod = P2P_WPS_CONFIG_METHODS_KEYPAD;
			else if(desc.WpsAttributes.DevicePasswdId == P2P_WPS_DEV_PASSWD_ID_REG_SPEC)
				ConfigMethod = P2P_WPS_CONFIG_METHODS_DISPLAY;
			else
		{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PProvisionDiscovery(): ConfigMethod is 0 and we don't know peer's DPID\n"));
				break;
			}	
		}

	if(!P2PTestU2SingleBitSet(ConfigMethod))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("<=== P2PProvisionDiscovery(): ConfigMethod has multiple bit set: %u\n", ConfigMethod));
			p2p_DevList_Unlock(&pP2PInfo->devList);
		return FALSE;
	}

	//
	// Fill Provision Discovery Context
	//
	P2PResetCommonChannelArrivingProcess(pP2PInfo);
		cpMacAddr(pProvisionDiscContext->devAddr, devAddr);
		pProvisionDiscContext->go = (P2P_DEV_TYPE_GO == pDev->type);
		if(P2P_DEV_TYPE_GO == pDev->type)
		{
			cpMacAddr(pProvisionDiscContext->goBssid, pDev->mac);
			CopySsid(pProvisionDiscContext->SsidBuf, pProvisionDiscContext->SsidLen, 
				desc.SsidBuf, desc.SsidLen);
		}
	pProvisionDiscContext->bDoingProvisionDiscovery = TRUE;
		pDev->p2p->pdStatus = P2P_SC_SUCCESS;
		pDev->p2p->pdConfigMethod = ConfigMethod;
		if(P2P_DEV_TYPE_GO == pDev->type)
	{
			if(0 != desc.OperatingChannel)
				pProvisionDiscContext->Channel = desc.OperatingChannel;
		else
				pProvisionDiscContext->Channel = desc.ListenChannel; 
		}
	else
	{
			pProvisionDiscContext->Channel = desc.ListenChannel;
	}

	p2p_DevList_FlushActionFrames(&pP2PInfo->devList, pDev->mac, pDev->type);

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		if(&pP2PInfo->pAdapter->bInHctTest)
			P2PDeviceDiscovery(pP2PInfo, 12);
		else
			P2PDeviceDiscovery(pP2PInfo, 36);
	}
	else
	{
		if(&pP2PInfo->pAdapter->bInHctTest)
			P2PDeviceDiscovery(pP2PInfo, 12);
		else
			P2PDeviceDiscovery(pP2PInfo, P2P_SCAN_FIND_PHASE_LOOP_TIMES);
	}
	}while(FALSE);

	p2p_DevList_Unlock(&pP2PInfo->devList);

	RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PProvisionDiscovery(): ch %u\n", pProvisionDiscContext->Channel));
	return TRUE;
}

VOID 
P2PServiceDiscoveryReq(
	PP2P_INFO pP2PInfo,
	PP2P_SD_REQ_CONTEXT pServiceQueryContent
	)
{
	PP2P_DEVICE_DISCRIPTOR pDevDesc;
	PP2P_SD_CONTEXT pSDContext = &(pP2PInfo->SDContext);

	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PServiceDiscoveryReq(): TLVs: %u\n", pServiceQueryContent->ServiceReqTLVSize));

	if(!P2P_ENABLED(pP2PInfo))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("<=== P2PServiceDiscoveryReq(): P2P NOT enabled\n"));
		return;
	}

	if(pP2PInfo->SDContext.bDoingServiceDiscovery)
	{// note that bDoingServiceDiscovery is cleared when common channel arrived
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("<=== P2PServiceDiscoveryReq(): SD in progress\n"));
		return;
	}

	if(P2P_DOING_SERVICE_DISCOVERY_REQ(pP2PInfo))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("<=== P2PServiceDiscoveryReq(): SD in progress (frame exhange started)\n"));
		return;
	}

	//
	// Validate TLV list length
	//
	if(pServiceQueryContent->ServiceReqTLVSize < 1) // must have at least 1 query TLV
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("<=== P2PServiceDiscoveryReq(): error: must have at least 1 query TLV\n"));
		return;
	}

	if(pServiceQueryContent->ServiceReqTLVSize > P2P_SD_MAX_SERVICES)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("<=== P2PServiceDiscoveryReq(): error: can have at most %u TLVs\n", P2P_SD_MAX_SERVICES));
		return;
	}

	RT_PRINT_ADDRS(COMP_P2P,DBG_LOUD, 
		("P2PServiceDiscoveryReq(): target DevAddr: "), 
		pServiceQueryContent->TargetDeviceAddress, 1);

	pDevDesc = P2PScanListFind(pP2PInfo->ScanList,
		pP2PInfo->ScanListSize,
		pServiceQueryContent->TargetDeviceAddress, 
		NULL, 
		NULL);
	if(pDevDesc == NULL)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("<=== P2PServiceDiscoveryReq(): target device not found in the scan list\n"));
		return;
	}

	//
	// Fill SDContext
	//
	P2PResetCommonChannelArrivingProcess(pP2PInfo);
	{
		//
		// Copy the SD req
		//
		cpMacAddr(pSDContext->UserSDReq.TargetDeviceAddress, pServiceQueryContent->TargetDeviceAddress);
		pSDContext->UserSDReq.ServiceReqTLVSize = pServiceQueryContent->ServiceReqTLVSize;
		
		PlatformMoveMemory(&pSDContext->UserSDReq.ServiceReqTLVList, 
			pServiceQueryContent->ServiceReqTLVList, 
			sizeof(P2P_SERVICE_REQ_TLV) * pServiceQueryContent->ServiceReqTLVSize);
	
		// Store target listen channel, although we will store the channel when 
		// ProbeRsp is received, the target listen channel
		// can still be a hint for where to search for its ProbeRsp
		pSDContext->Channel = pDevDesc->ListenChannel;

		// Set to Start Service Dicovery 
		pSDContext->bDoingServiceDiscovery = TRUE;

		pSDContext->bRequester = TRUE;

		pSDContext->DialogToken = IncreaseDialogToken(pP2PInfo->DialogToken);

		pSDContext->TransactionID = pServiceQueryContent->ServiceReqTLVList[0].TransactionID;

		pSDContext->FragmentID = 0;
		pSDContext->ANQPQueryRspFieldToSendOffset = 0;

		cpMacAddr(pSDContext->TargetDeviceAddress, pServiceQueryContent->TargetDeviceAddress);
	}

	//
	// Do device discovery to find out when the target device is in Listen State.
	//
	P2PDeviceDiscovery(pP2PInfo, P2P_SCAN_FIND_PHASE_LOOP_TIMES);
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PServiceDiscoveryReq()\n"));

}

VOID 
P2PServiceDiscoveryRsp(
	PP2P_INFO pP2PInfo,
	PP2P_SD_RSP_CONTEXT pServiceRspContent
	)
{
	BOOLEAN bFragmentSDRsp = FALSE;
	PP2P_SD_CONTEXT pSDContext = &pP2PInfo->SDContext;

	if(!P2P_ENABLED(pP2PInfo))
	{
		return;
	}

	if(!pSDContext->bDoingServiceDiscovery)
	{
		return;
	}

	//
	// Copy service info if we have enough space for the TLVs
	//
	if(pServiceRspContent->ServiceRspTLVSize > P2P_SD_MAX_SERVICES)
	{
		return;
	}

	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PServiceDiscoveryRsp()\n"));
	
	//
	// Construct Rsp TLV list from pP2PInfo->SDRspContext
	//
	p2p_Construct_AnqpQueryRspField(pP2PInfo, pServiceRspContent, pSDContext);

	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PServiceDiscoveryRsp(): ANQPQueryRsp Size: %u, Frag Threshold: %u\n", 
		pSDContext->ANQPQueryRspFieldToSendSize,
		pP2PInfo->SDRspFragmtntThreshold));
	//RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ANQPQueryRsp constructed: ", 
	//	pP2PInfo->SDContext.ANQPQueryRspFieldToSendBuf, 
	//	pP2PInfo->SDContext.ANQPQueryRspFieldToSendSize);

	// To send from offset 0
	pSDContext->ANQPQueryRspFieldToSendOffset = 0;
	
	if(pSDContext->ANQPQueryRspFieldToSendSize > pP2PInfo->SDRspFragmtntThreshold)
	{	
		bFragmentSDRsp = TRUE;

		//
		// Fill P2P_SD_CONTEXT for doing fragmentation
		// We've already filled DialogToken, TransactionID and ServiceReqRecvd.
		//
		pSDContext->Status = pServiceRspContent->SDStatus;
		pSDContext->bFragment = TRUE;
		pSDContext->FragmentID = 0; // we are going to send ComebackRsp with FragmentID 0
		pSDContext->bDoingServiceDiscovery = TRUE;
		pSDContext->bRequester = FALSE;
		pSDContext->Channel = P2PGetChannel(pP2PInfo);
		cpMacAddr(pSDContext->TargetDeviceAddress, pServiceRspContent->SourceDeviceAddress);
	}

	p2p_Send_SDRsp(pP2PInfo, 
		pSDContext->TargetDeviceAddress, 
		pSDContext->DialogToken, 
		pSDContext->Status, 
		bFragmentSDRsp);

	if(bFragmentSDRsp)
	{
		pP2PInfo->SDWaitForComebackReqSlotCount = 0;
	}
	else
	{
		PlatformZeroMemory(&pP2PInfo->SDContext, sizeof(P2P_SD_CONTEXT));
		CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pP2PInfo->pAdapter), 
			P2P_SERVICE_DISCOVERY_COMEBACK_TIMEOUT);
	}

	RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PServiceDiscoveryRsp()\n"));

}

BOOLEAN
P2PConnect(
	IN PP2P_INFO pP2PInfo,
	IN pu1Byte DeviceAddress
	)
{
	PADAPTER pAdapter = pP2PInfo->pAdapter;

	PP2P_DEVICE_DISCRIPTOR pP2PDeviceDesc = NULL;
	
	// For the case when we are connecting to a P2P Client
	PP2P_DEVICE_DISCRIPTOR pGODeviceDesc = NULL;
	PP2P_CLIENT_INFO_DISCRIPTOR pClientInfoDesc = NULL;
	
	if(!P2P_ENABLED(pP2PInfo))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PConnect(): P2P Disabled\n"));
		return FALSE;
	}

	//
	// Find the descriptor in scan list (GO and Device are in this list)
	//
	pP2PDeviceDesc = P2PScanListFind(pP2PInfo->ScanList, pP2PInfo->ScanListSize, DeviceAddress, NULL, NULL);

	//
	// Find the descriptor in client info list of each GO if it can't be found in the scan list
	//
	if(pP2PDeviceDesc == NULL)
	{
		if(!P2PScanListFindClient(pP2PInfo->ScanList, 
			pP2PInfo->ScanListSize, 
			DeviceAddress, 
			NULL, 
			&pGODeviceDesc, 
			&pClientInfoDesc))
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, 
				("P2PConnect(): can't find the device in either the scan list or the client info lists\n"));
			return FALSE;
		}

		//
		// The target device is a P2P Client
		//
		pP2PDeviceDesc = pGODeviceDesc;
	}

	//
	// Before calling this function, the upper layer shall make sure that 
	// 1. Device Name is included in WPS IE
	// 2. Device Password ID is included in WPS IE (Clause 3.1.2.4.1)
	//

	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PConnect()\n"));
	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Target Device: ", DeviceAddress);

	if(pP2PInfo->State > P2P_STATE_DEV_DISC_COMPLETE)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PConnect(): State: %u\n", pP2PInfo->State));
		return FALSE;
	}

	if(pP2PDeviceDesc == NULL)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PConnect(): Target device entry not found\n"));
		return FALSE;
	}

	if(pP2PDeviceDesc->Role == P2P_DEVICE)
	{
		// Fill ConnectionContext
		P2PResetCommonChannelArrivingProcess(pP2PInfo);
		PlatformMoveMemory(&pP2PInfo->ConnectionContext.ConnectingDevice, 
			pP2PDeviceDesc, 
			sizeof(P2P_DEVICE_DISCRIPTOR));
		pP2PInfo->ConnectionContext.Status = P2P_STATUS_SUCCESS;
		pP2PInfo->ConnectionContext.DialogToken = IncreaseDialogToken(pP2PInfo->DialogToken);

		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
			pP2PInfo->ConnectionContext.FindPhaseLoopTimes = 36;
		else
			pP2PInfo->ConnectionContext.FindPhaseLoopTimes = P2P_SCAN_FIND_PHASE_LOOP_TIMES;

		pP2PInfo->ConnectionContext.bGoingToBeGO = FALSE;

		pP2PInfo->bPreGroupFormation = TRUE;

		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			if(&pP2PInfo->pAdapter->bInHctTest)
				P2PDeviceDiscovery(pP2PInfo, 12);
			else
				P2PDeviceDiscovery(pP2PInfo, 36);
		}
		else
		{
			if(&pP2PInfo->pAdapter->bInHctTest)
				P2PDeviceDiscovery(pP2PInfo, 12);
			else
				P2PDeviceDiscovery(pP2PInfo, P2P_SCAN_FIND_PHASE_LOOP_TIMES);
		}

		// GO Formation should be initiated in P2POnProbeReq()
	}
	else if(pP2PDeviceDesc->Role == P2P_GO)
	{// note that in this case, the mac addr is the interface addr of the GO
		if(pClientInfoDesc)
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PConnect(): peer is a P2P Client\n"));
			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, 
				"GO DevAddr:\n", pP2PDeviceDesc->DeviceAddress);

			//
			// Do discoverability request and then GO formation
			//
			P2PDeviceDiscoverabilityReq(pP2PInfo, DeviceAddress, TRUE);
		}
		else
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PConnect(): peer is a GO\n"));
			
			//
			// Now we shall have the dev addr and interface addr of the GO
			//
			// PF #2, Atheros device use different Dev and Int Addr.
			// UI send DevAddr as the MacAddress and we shall not use
			// it as the IntAddr.
			/*
			pP2PDeviceDesc = P2PScanListFind(pP2PInfo->ScanList, 
				pP2PInfo->ScanListSize, 
				NULL, 
				MacAddress,
				NULL); // we use intended addr here
			*/
			
			if(pP2PDeviceDesc && pP2PDeviceDesc->SsidLen)
			{
				//P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
				BOOLEAN bScanInProgress = MgntScanInProgress(&pP2PInfo->pAdapter->MgntInfo);

				P2PResetCommonChannelArrivingProcess(pP2PInfo);
				if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
				{// Doing P2P Device Discovery
					P2PScanListCeaseScan(pP2PInfo);
					P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
				}
				else if(pP2PInfo->bExtendedListening)
				{// Doing extended listening
					P2PScanListCeaseScan(pP2PInfo);
					P2PExtendedListenComplete(pP2PInfo);
				}
				else if(bScanInProgress) 
				{// Doing normal scan
					P2PScanListCeaseScan(pP2PInfo);
				}

				// Fill ConnectionContext
				PlatformMoveMemory(&pP2PInfo->ConnectionContext.ConnectingDevice, 
					pP2PDeviceDesc, 
					sizeof(P2P_DEVICE_DISCRIPTOR));
				pP2PInfo->ConnectionContext.Status = P2P_STATUS_SUCCESS;
				pP2PInfo->ConnectionContext.DialogToken = 0;
				pP2PInfo->ConnectionContext.FindPhaseLoopTimes = 0;
				pP2PInfo->ConnectionContext.bGoingToBeGO = FALSE;

				pP2PInfo->bPreGroupFormation = FALSE;

				pP2PInfo->State = P2P_STATE_PRE_PROVISIONING;
				PlatformCancelTimer(pAdapter, &pP2PInfo->P2PMgntTimer);
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 100);
			}
			else
			{// failed to scan the GO
				RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PConnect(): the target is not in the scan list\n"));
				//P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
				
				pP2PInfo->State = P2P_STATE_INITIALIZED;
			}
		}
	}
	else
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, 
			("<=== P2PConnect() peer role is incorrect: %u\n", pP2PDeviceDesc->Role));
		//P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
		return FALSE;
	}
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PConnect()\n"));
	return TRUE;
}

VOID 
P2PGOStartAutomously(
	IN PP2P_INFO pP2PInfo
	)
{
	P2P_PROFILE P2PProfile;
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PGOStartAutomously()\n"));

	P2PProfile.OpChannel = pP2PInfo->OperatingChannel;
	P2PDetermineGOSsid(pP2PInfo, P2PProfile.SsidBuf, &P2PProfile.SsidBufLen);	
	CopySsid(pP2PInfo->SSIDBuf, pP2PInfo->SSIDLen, P2PProfile.SsidBuf, P2PProfile.SsidBufLen);
	
	P2PSetRole(pP2PInfo, P2P_GO);
	P2PSetOperatingState(pP2PInfo);
	pP2PInfo->bGOStartedAutonomously = TRUE;
	pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)P2P_STATUS_MAX;
	
	P2PIndicateStartApRequest(pP2PInfo, &P2PProfile);

	//
	// For PF #1, if we do this, legacy STA without WPS can't associate with us 
	// (associated, but the STA keeps sending AssocReq) when we are an autonomous GO.
	// This may not be necessary to notify the UI that we are an GO now.
	//
	/*P2PIndicateGOFormatedInfo(pP2PInfo, 
		P2P_STATUS_SUCCESS, 
		TRUE, 
		NULL);
	*/

	PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);

	RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PGOStartAutomously()\n"));
}

VOID
P2PDisconnect(
	IN PP2P_INFO pP2PInfo
	)
{
	BOOLEAN bScanInProgress;
	
	if(!P2P_ENABLED(pP2PInfo))
	{
		return;
	}

	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PDisconnect()\n"));
	
	//
	// Stop any existing scan
	//
	P2PResetCommonChannelArrivingProcess(pP2PInfo);
	
	bScanInProgress = MgntScanInProgress(&pP2PInfo->pAdapter->MgntInfo);

	RT_TRACE(COMP_P2P, DBG_LOUD, 
			("P2PDisconnect(): State: %u, bScanInProgres: %u\n",
			pP2PInfo->State, bScanInProgress));
	
	if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
	{// Doing P2P Device Discovery
		P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
	}
	else if(pP2PInfo->bExtendedListening)
	{// Doing extended listening
		P2PExtendedListenComplete(pP2PInfo);
	}
	else if(bScanInProgress) 
	{// Doing normal scan
	}

	// Reset disconnect slot count
	pP2PInfo->P2PGONoClientSlotCount = 0;
	pP2PInfo->P2PClientDisconnectedSlotCount = 0;

	pP2PInfo->bOppPS = FALSE;
	PlatformZeroMemory(pP2PInfo->NoADescriptors, sizeof(P2P_NOA_DESCRIPTOR) * P2P_MAX_NUM_NOA_DESC);

	P2PSetRole(pP2PInfo, P2P_DEVICE);

	pP2PInfo->State = P2P_STATE_INITIALIZED;
	PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);

	P2PSvc_OnDisconnect(pP2PInfo->pP2PSvcInfo);

	RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PDisconnect()\n"));

}

//
// Description:
//	Send the device discoverability request to ask the client in the group to be discoverable.
//	The receiver for this packet is the GO which the client belongs to.
// Arguments:
//	[in] pP2PInfo - 
//		The P2P context.
//	[in] DeviceAddress -
//		The target address of P2P client that this request sent to.
//	[in] bConnect -
//		TRUE if go to connect the target in the next step; otherwise, just send device discoverability request.
// Return:
//	TRUE if this action is accepted.
//
BOOLEAN
P2PDeviceDiscoverabilityReq(
	IN 	PP2P_INFO	pP2PInfo,
	IN 	pu1Byte		DeviceAddress,
	IN	BOOLEAN		bConnect
	)
{
	//
	// This function is to send a DevDiscoverabilityReq to a P2P Client 
	// so that it will keep available for a duration of at least 100TU.
	//
	PP2P_CLIENT_INFO_DISCRIPTOR pClient = NULL;
	PP2P_DEVICE_DISCRIPTOR pGO = NULL;
	BOOLEAN bScanInProgress;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PDeviceDiscoverabilityReqStart()\n"));

	PlatformZeroMemory(&pP2PInfo->DeviceDiscoverabilityContext, sizeof(P2P_DEVICE_DISCOVERABILITY_CONTEXT));
	
	//
	// Find the client and its GO
	//
	if(!P2PScanListFindClient(pP2PInfo->ScanList, pP2PInfo->ScanListSize, 
		DeviceAddress, 
		NULL, 
		&pGO, &pClient))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PDeviceDiscoverabilityReqStart(): client not found\n"));
		return FALSE;
	}

	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, 
		"P2PDeviceDiscoverabilityReqStart(): to send DeviceDiscoverabilityReq to the GO:\n", 
		pGO->DeviceAddress);

	//
	// Record GO info for constructing P2P GroupID Attribute
	//
	cpMacAddr(pP2PInfo->DeviceDiscoverabilityContext.GODeviceAddr, pGO->DeviceAddress);
	CopySsid(pP2PInfo->DeviceDiscoverabilityContext.GOSsidBuf, pP2PInfo->DeviceDiscoverabilityContext.GOSsidLen, 
		pGO->SsidBuf, pGO->SsidLen);

	//
	// Check Capability
	//
	if(!(pClient->DeviceCapability & dcP2PClientDiscoverability))
	{
		return FALSE;
	}

	//
	// Stop any existing scan
	//
	bScanInProgress = MgntScanInProgress(&pP2PInfo->pAdapter->MgntInfo);
	if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
	{// Doing P2P Device Discovery
		P2PScanListCeaseScan(pP2PInfo);
		P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
	}
	else if(pP2PInfo->bExtendedListening)
	{// Doing extended listening
		P2PScanListCeaseScan(pP2PInfo);
		P2PExtendedListenComplete(pP2PInfo);
	}
	else if(bScanInProgress) 
	{// Doing normal scan
		P2PScanListCeaseScan(pP2PInfo);
	}

	//
	// Fill the context
	//
	cpMacAddr(pP2PInfo->DeviceDiscoverabilityContext.ClientDeviceAddress, pClient->DeviceAddress);
	cpMacAddr(pP2PInfo->DeviceDiscoverabilityContext.GoBssid, pGO->IntendedP2PInterfaceAddress);
	pP2PInfo->DeviceDiscoverabilityContext.DialogToken = IncreaseDialogToken(pP2PInfo->DialogToken);
	pP2PInfo->DeviceDiscoverabilityContext.Status = P2P_STATUS_SUCCESS;
	pP2PInfo->DeviceDiscoverabilityContext.GOChannel = pGO->OperatingChannel;
	pP2PInfo->DeviceDiscoverabilityContext.OriginalChannel = P2PGetChannel(pP2PInfo);
	pP2PInfo->DeviceDiscoverabilityContext.OriginalState = pP2PInfo->State;
	pP2PInfo->DeviceDiscoverabilityContext.bWaitingBeaconFromGO = TRUE;
	pP2PInfo->DeviceDiscoverabilityContext.bGoConnect = bConnect;
	
	//
	// Send Device Discoverability to the GO of the client when beacon is received
	//
	P2PSetChannel(pP2PInfo, pGO->OperatingChannel);
	
	pP2PInfo->State = P2P_STATE_DEVICE_DISCOVERABILITY_WAIT_BEACON;
	PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
	PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, P2P_DEVICE_DISCOVERABILITY_BEACON_TIMEOUT);

	RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PDeviceDiscoverabilityReqStart(): GO Ch: %u, \n", pP2PInfo->DeviceDiscoverabilityContext.GOChannel));
	return TRUE;
}

BOOLEAN 
P2PInvitePeerStart(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_LIB_INVITATION_REQ_CONTEXT pLibInvittionContext
	)
{	
	//
	// At current stage, 
	//	1. IntAddr
	//	2. SSID 
	// are valid only when bInvokePersistent is TRUE.
	//
	
	BOOLEAN bRet = FALSE;
	PP2P_DEVICE_DISCRIPTOR pDevDesc = NULL;
	
	// Win8: NdisTest --------------------------------------------------------------------------------
	//	+ If the peer does not exist, still do the invitation procedure as device
	PRT_GEN_TEMP_BUFFER pRtBuffer = NULL;
	PP2P_DEVICE_DISCRIPTOR	pPseudoDevicePeer = NULL;

	pRtBuffer = GetGenTempBuffer(GetDefaultAdapter(pP2PInfo->pAdapter), sizeof(P2P_DEVICE_DISCRIPTOR));
	pPseudoDevicePeer  = (PP2P_DEVICE_DISCRIPTOR) pRtBuffer->Buffer.Ptr;
	//----------------------------------------------------------------------------------------------

	RT_TRACE(COMP_P2P, DBG_LOUD, ("===> P2PInvitePeerStart():() port number %d state %d\n", pP2PInfo->pAdapter->pNdis62Common->PortNumber, pP2PInfo->State));
	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "DevAddr:\n", pLibInvittionContext->TargetDeviceAddress);
	RT_TRACE(COMP_P2P, DBG_LOUD, ("bPersistent: %u\n", pLibInvittionContext->bPersistent));

	//
	// Check state
	//
	if(!P2P_ENABLED(pP2PInfo))
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PInvitePeerStart(): P2P not enabled\n"));
		goto exit_P2PInvitePeerStart;
	}
	
	//
	// Clear invitation context
	//
	PlatformZeroMemory(&(pP2PInfo->InvitationContext), sizeof(P2P_INVITATION_CONTEXT));

	//
	// Check self capability
	//
	if(!(pP2PInfo->DeviceCapability & dcP2PInvitationProcedure))
	{// this device does not support invitation procedure
		RT_TRACE(COMP_P2P, DBG_LOUD, ("error: my DeviceCapability is %u\n", pP2PInfo->DeviceCapability));
		goto exit_P2PInvitePeerStart;
	}

	//
	// Get the target descriptor
	//
	pDevDesc = P2PScanListFind(pP2PInfo->ScanList, pP2PInfo->ScanListSize, pLibInvittionContext->TargetDeviceAddress, NULL, NULL);
	if(pDevDesc == NULL)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("Peer not found in the scan list\n"));

		// Win8: For passing NdisTest: WFD_Group_ext-Invitation ---------------------
		PlatformZeroMemory(pPseudoDevicePeer, sizeof(P2P_DEVICE_DISCRIPTOR));
		pDevDesc = pPseudoDevicePeer;
		pDevDesc->Role = P2P_DEVICE;
		pDevDesc->ListenChannel = P2P_DEFAULT_LISTEN_CHANNEL;
		cpMacAddr(pDevDesc->DeviceAddress, pLibInvittionContext->TargetDeviceAddress);
		// -------------------------------------------------------------------

		//goto exit_P2PInvitePeerStart;
	}


// Win8: Mark for passing the NdisTest: WFD_Group_ext-Invitation ------------------------------------------
#if 0	
	//
	// Check peer device capability
	// 
	if(!(pDevDesc->DeviceCapability & dcP2PInvitationProcedure))
	{// peer device does not support invitation procedure
		RT_TRACE(COMP_P2P, DBG_LOUD, ("error: peer DeviceCapability is %u, does not support invitation\n", 
			pDevDesc->DeviceCapability));
		goto exit_P2PInvitePeerStart;
	}
#endif  
// ------------------------------------------------------------------------------------------------


	//
	// If to invoke persistent, check peer group capability
	//
	if(pLibInvittionContext->bPersistent)
	{
	/* group cap is valid only when the device is a GO
		if(!(pDevDesc->GroupCapability & gcPersistentP2PGroup))
		{// peer device does not support persistent group
			RT_TRACE(COMP_P2P, DBG_LOUD, ("error: peer GroupCapability is %u, does not support persistent\n", 
				pDevDesc->DeviceCapability));
			goto exit_P2PInvitePeerStart;
		}
	*/	
	}

	//
	// Fill invitation context
	//
	P2PResetCommonChannelArrivingProcess(pP2PInfo);
	pP2PInfo->InvitationContext.InvitorRole= pLibInvittionContext->InvitorRole;
	pP2PInfo->InvitationContext.bPersistentInvitation = pLibInvittionContext->bPersistent;
	pP2PInfo->InvitationContext.DialogToken = IncreaseDialogToken(pP2PInfo->DialogToken);
	PlatformMoveMemory(&pP2PInfo->InvitationContext.InvitedDevice, pDevDesc, sizeof(P2P_DEVICE_DISCRIPTOR));
	pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe = FALSE;
	pP2PInfo->InvitationContext.bInvitor = TRUE;
	pP2PInfo->InvitationContext.OpChannel = pLibInvittionContext->OpChannel;
	RT_TRACE(COMP_P2P, DBG_LOUD, ("DialogToken: %u, state: %u\n", 
		pP2PInfo->InvitationContext.DialogToken, pP2PInfo->State));

	//
	// Customization for different peer roles
	//
	RT_TRACE(COMP_P2P, DBG_LOUD, ("Peer Role: %u, My role: %u\n", pDevDesc->Role, pP2PInfo->Role));
	if(pDevDesc->Role == P2P_DEVICE)
	{// need to send InvitationReq when common chnl arrive
		RT_TRACE(COMP_P2P, DBG_LOUD, ("Set bToSendInvitationReqOnProbe\n"));
		pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe = TRUE;
	}
	else if(pDevDesc->Role == P2P_CLIENT)
	{// need to make sure that the client can receive our InvitationReq
		pP2PInfo->InvitationContext.bToUseDeviceDiscoverability = TRUE;
	}
	else if(pDevDesc->Role == P2P_GO)
	{// need to make sure that the GO is awake for receiving our InvitationReq
	}
	else
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("error: unknown peer role: %u\n", pDevDesc->Role));
		goto exit_P2PInvitePeerStart;
	}

	//
	// Fill Group ID (Device Addr and SSID) and Group BSSID
	//
	cpMacAddr(pP2PInfo->InvitationContext.GroupBssid, pLibInvittionContext->GroupBssid);
	cpMacAddr(pP2PInfo->InvitationContext.GODeviceAddress, pLibInvittionContext->GroupDeviceAddress);
	CopySsid(pP2PInfo->InvitationContext.SsidBuf, pP2PInfo->InvitationContext.SsidLen, 
		pLibInvittionContext->GroupSsidBuf, pLibInvittionContext->GroupSsidLen); 

	//
	// Decide when to send the InvitationReq
	//
	if(pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe)
	{// in this case, we send the req when ProbeReq from the peer is recvd
		if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
		{
			if(pP2PInfo->pAdapter->bInHctTest)
				P2PDeviceDiscovery(pP2PInfo, 6);
			else
				P2PDeviceDiscovery(pP2PInfo, 36);
		}
		else
		{
			if(pP2PInfo->pAdapter->bInHctTest)
				P2PDeviceDiscovery(pP2PInfo, 6);
			else
				P2PDeviceDiscovery(pP2PInfo, P2P_SCAN_FIND_PHASE_LOOP_TIMES);
		}

		bRet = TRUE;
	}
	else
	{// we send the req immediately
		pP2PInfo->State = P2P_STATE_INVITATION_REQ_SEND;
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);
	}
	
exit_P2PInvitePeerStart:

	ReturnGenTempBuffer(GetDefaultAdapter(pP2PInfo->pAdapter), pRtBuffer);
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("<=== P2PInvitePeerStart()\n"));
	return bRet;
}

VOID
P2PInvitePeerComplete(
	IN PP2P_INFO pP2PInfo
	)
{
	if(P2P_ACTING_AS_GO(pP2PInfo))
	{// GO
		// TODO:
	}
	else if(P2P_ACTING_AS_CLIENT(pP2PInfo))
	{// P2P Client
		SendNullFunctionData(pP2PInfo->pAdapter, pP2PInfo->pAdapter->MgntInfo.Bssid, FALSE);
	}
	else
	{// P2P Device
		
	}
}

//======================================================================
// Customized Indications
//======================================================================

VOID
P2PIndicateOnProvisionDiscoveryRsp(
	IN PP2P_INFO pP2PInfo,
	IN u2Byte ConfigMethod,
	IN pu1Byte DeviceAddress
	)
{
	pu1Byte pBuf = NULL;
	u4Byte BufSize = sizeof(u2Byte) + 6;
	PlatformAllocateMemory(pP2PInfo->pAdapter, (void**)&pBuf, BufSize);
	if(pBuf == NULL) return;
	PlatformMoveMemory(pBuf, &ConfigMethod, sizeof(u2Byte));
	PlatformMoveMemory(pBuf + sizeof(u2Byte), DeviceAddress, 6);
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2PIndicateOnProvisionDiscoveryRsp", pBuf, BufSize);
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_ON_PROVISION_DISC_RSP,
		RT_CUSTOM_INDI_TARGET_IHV,
		pBuf,
		sizeof(u2Byte) + 6);
	PlatformFreeMemory(pBuf, BufSize);
}

VOID
P2PIndicateOnInvitationReq(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_LIB_INVITATION_REQ_CONTEXT pLibInvitationReq
	)
{
	//P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_ON_INVITATION_REQ,
		RT_CUSTOM_INDI_TARGET_IHV,
		pLibInvitationReq,
		sizeof(P2P_LIB_INVITATION_REQ_CONTEXT));
}

VOID
P2PIndicateOnInvitationRsp(
	IN PP2P_INFO pP2PInfo,
	IN P2P_STATUS_CODE Status,
	IN BOOLEAN bPersistent,
	IN u1Byte OpChannel,
	IN P2P_ROLE Role,
	IN u1Byte GroupSsidLen,
	IN pu1Byte GroupSsidBuf
	)
{
	P2P_LIB_INVITATION_RSP_CONTEXT LibInvitationRsp;
	LibInvitationRsp.Role= Role;
	LibInvitationRsp.bPersistent = bPersistent;
	LibInvitationRsp.OpChannel = OpChannel;
	LibInvitationRsp.Status = Status;
	CopySsid(LibInvitationRsp.GroupSsidBuf, LibInvitationRsp.GroupSsidLen, GroupSsidBuf, GroupSsidLen);
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_ON_INVITATION_RSP,
		RT_CUSTOM_INDI_TARGET_IHV,
		&LibInvitationRsp,
		sizeof(LibInvitationRsp));
}

VOID
P2PIndicateOnProvisionDiscoveryReq(
	IN PP2P_INFO pP2PInfo,
	IN u2Byte ConfigMethod,
	IN pu1Byte DeviceAddress,
	IN PP2P_WPS_ATTRIBUTES pWpsAttributes
	)
{
	pu1Byte pBuf = NULL;
	u4Byte BufSize = sizeof(u2Byte) + 6 + sizeof(P2P_WPS_ATTRIBUTES);
	PlatformAllocateMemory(pP2PInfo->pAdapter, (void**)&pBuf, BufSize);
	if(pBuf == NULL) return;
	PlatformMoveMemory(pBuf, &ConfigMethod, sizeof(u2Byte));
	PlatformMoveMemory(pBuf + sizeof(u2Byte), DeviceAddress, 6);
	PlatformMoveMemory(pBuf + sizeof(u2Byte) + 6, pWpsAttributes, sizeof(P2P_WPS_ATTRIBUTES));
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2PIndicateOnProvisionDiscoveryReq", pBuf, BufSize);	
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_ON_PROVISION_DISC_REQ,
		RT_CUSTOM_INDI_TARGET_IHV,
		pBuf,
		BufSize);
	PlatformFreeMemory(pBuf, BufSize);
}

VOID
P2PIndicateStartApRequest(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_PROFILE pP2PProfile
	)
{
	P2P_PROFILE P2PProfile;
	BOOLEAN bDefPortConnected = P2PDefaultPortConnected(pP2PInfo);
	BOOLEAN bCrossConnectionEnabled = pP2PInfo->GroupCapability & gcCrossConnection;
	BOOLEAN bToEnableICS = FALSE;

	P2PProfile.OpChannel = pP2PProfile->OpChannel;
	P2PProfile.SsidBufLen = pP2PProfile->SsidBufLen;
	PlatformMoveMemory(P2PProfile.SsidBuf, pP2PProfile->SsidBuf, pP2PProfile->SsidBufLen);
	
	RT_TRACE(COMP_P2P, DBG_LOUD, 
		("P2PIndicateStartApRequest(): Indicate NDIS_STATUS_P2P_START_AP_REQ, OpChannel: %u\n",
		pP2PProfile->OpChannel));
	RT_PRINT_STR(COMP_P2P, DBG_LOUD, "SSID:\n", 
		pP2PProfile->SsidBuf, pP2PProfile->SsidBufLen);

	RT_TRACE(COMP_P2P, DBG_LOUD, 
		("P2PIndicateStartApRequest(): bDefPortConnected: %u, bCrossConnectionEnabled: %u\n",
		bDefPortConnected, bCrossConnectionEnabled));

	if(bDefPortConnected && bCrossConnectionEnabled)
	{
		bToEnableICS = TRUE;
	}

	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_ENABLE_ICS,
		RT_CUSTOM_INDI_TARGET_IHV,
		&bToEnableICS,
		sizeof(BOOLEAN));
	
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_START_AP_REQ,
		RT_CUSTOM_INDI_TARGET_IHV,
		&P2PProfile,
		sizeof(P2P_PROFILE));
	
}

VOID
P2PIndicateStopApRequest(
	IN PP2P_INFO pP2PInfo
	)
{
	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateStopApRequest()\n"));
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_STOP_AP_REQ,
		RT_CUSTOM_INDI_TARGET_IHV,
		NULL,
		0);
}

VOID
P2PIndicateDisconnectClientRequest(
	IN PP2P_INFO pP2PInfo
	)
{
	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateDisconnectClientRequest()\n"));
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_DISCONNECT_CLIENT_REQ,
		RT_CUSTOM_INDI_TARGET_IHV,
		NULL,
		0);
}

VOID
P2PIndicateGOFormatedInfo(
	IN PP2P_INFO pP2PInfo,
	IN u1Byte Status,
	IN BOOLEAN bGoingToBeGO,
	IN PP2P_DEVICE_DISCRIPTOR pDevDesc
	)
{
//	P2P_GO_FORMATED_INFO P2PGOFormatedInfo;
	PP2P_GO_FORMATED_INFO pP2PGOFormatedInfo;
	PRT_GEN_TEMP_BUFFER pGenBufP2PGOFormatedInfo;
	PADAPTER Adapter = pP2PInfo->pAdapter;

	pGenBufP2PGOFormatedInfo = GetGenTempBuffer (Adapter, sizeof(P2P_GO_FORMATED_INFO));
	pP2PGOFormatedInfo = (P2P_GO_FORMATED_INFO *)pGenBufP2PGOFormatedInfo->Buffer.Ptr;
	
	pP2PGOFormatedInfo->p2pRole = (Status != P2P_STATUS_SUCCESS) ? (P2P_DEVICE) : ((bGoingToBeGO) ? (P2P_GO) : (P2P_CLIENT));
	if(pDevDesc)
	{
		PlatformMoveMemory(&(pP2PGOFormatedInfo->targetDesc), 
			pDevDesc, 
			sizeof(P2P_DEVICE_DISCRIPTOR));
		pP2PGOFormatedInfo->targetDesc.Status = Status;

		if(P2P_GO == pP2PGOFormatedInfo->p2pRole)
		{
			pP2PGOFormatedInfo->targetDesc.SsidLen = pP2PInfo->SSIDLen;
			PlatformMoveMemory(pP2PGOFormatedInfo->targetDesc.SsidBuf, pP2PInfo->SSIDBuf, pP2PInfo->SSIDLen);
			pP2PGOFormatedInfo->targetDesc.OperatingChannel = pP2PInfo->OperatingChannel;
		}
		
		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateGOFormatedInfo(): Status: %u, Role: %u\n",
			Status,
			pP2PGOFormatedInfo->p2pRole));
		//P2PDumpScanList(pDevDesc, 1);
	}
	else
	{
		PlatformZeroMemory(&(pP2PGOFormatedInfo->targetDesc), sizeof(P2P_DEVICE_DISCRIPTOR));
	}
	
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_GO_FORMATED,
		RT_CUSTOM_INDI_TARGET_IHV,
		pP2PGOFormatedInfo,
		sizeof(P2P_GO_FORMATED_INFO));

	ReturnGenTempBuffer (Adapter, pGenBufP2PGOFormatedInfo);
}

VOID
P2PIndicateCurrentState(
	IN PP2P_INFO pP2PInfo,
	IN P2P_STATE CurrentState
	)
{
	u8Byte CurrentTime = PlatformGetCurrentTime();
	BOOLEAN bToIndicate = FALSE;
	u8Byte TimeDiff = CurrentTime - pP2PInfo->PreviouslyIndicateStateTime;

	if(pP2PInfo->PreviouslyIndicatedState != CurrentState) // to prevent redundent indication
	{
		bToIndicate = TRUE;
	}
	else if(TimeDiff > 4000000) // 4sec
	{
		bToIndicate = TRUE;
	}

	if(bToIndicate)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateCurrentState(): State: %u, Role: %u, nScanList: %u, nScanList4Q: %u, IntfIndex: %u portnumber %d\n", 
			CurrentState,
			pP2PInfo->Role,
			pP2PInfo->ScanListSize,
			pP2PInfo->ScanList4QuerySize,
			pP2PInfo->pAdapter->interfaceIndex,
			pP2PInfo->pAdapter->pNdis62Common->PortNumber));

		PlatformIndicateCustomStatus(
			pP2PInfo->pAdapter,
			RT_CUSTOM_EVENT_P2P_INDICATE_CURRENT_STATE,
			RT_CUSTOM_INDI_TARGET_IHV,
			&CurrentState,
			sizeof(P2P_STATE));
		pP2PInfo->PreviouslyIndicatedState = CurrentState;
		pP2PInfo->PreviouslyIndicateStateTime = PlatformGetCurrentTime();
	}
}

VOID
P2PIndicateCurrentRole(
	IN PP2P_INFO pP2PInfo,
	IN P2P_ROLE CurrentRole
	)
{
	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateCurrentRole(): Role: %u state %d\n", pP2PInfo->Role, pP2PInfo->State));
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_CURRENT_ROLE,
		RT_CUSTOM_INDI_TARGET_IHV,
		&CurrentRole,
		sizeof(P2P_ROLE));
}

VOID
P2PIndicateCurrentDevPasswdId(
	IN PP2P_INFO pP2PInfo,
	IN u2Byte CurrentDevPasswdId
	)
{
	//RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateCurrentDevPasswdId(): DevPasswdId: %u\n", CurrentDevPasswdId));
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_CURRENT_DEV_PASSWD_ID,
		RT_CUSTOM_INDI_TARGET_IHV,
		&CurrentDevPasswdId,
		sizeof(u2Byte));
}

//
// Description:
//	Indicate that one device wants to do GO formation when we are the P2P client.
// Arguments:
//	[in] pP2PInfo - 
//		The P2P context.
//	[in] pDevDesc -
//		The descriptor info for this request of device.
//
VOID
P2PIndicateDeviceReqClientGoFormation(
	IN	PP2P_INFO				pP2PInfo,
	IN	PP2P_DEVICE_DISCRIPTOR	pDevDesc
	)
{
	if(pDevDesc == NULL)
		return;

	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "P2PIndicateDeviceReqClientGoFormation(): ", pDevDesc->DeviceAddress);
	//P2PDumpScanList(pDevDesc, 1);

	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_DEV_REQ_CLIENT_GO_FORMATION,
		RT_CUSTOM_INDI_TARGET_IHV,
		pDevDesc,
		sizeof(P2P_DEVICE_DISCRIPTOR));
}

VOID
P2PIndicateClientConnected(
	IN	PP2P_INFO				pP2PInfo,
	IN	pu1Byte					ClientInterfaceAddr
	)
{	
	if(ClientInterfaceAddr == NULL)
		return;
	
	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "P2PIndicateClientConnected(): ", ClientInterfaceAddr);

	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_CLIENT_CONNECTED,
		RT_CUSTOM_INDI_TARGET_IHV,
		ClientInterfaceAddr,
		6);
}

VOID
P2PIndicateClientDisconnected(
	IN	PP2P_INFO				pP2PInfo,
	IN	pu1Byte					ClientInterfaceAddr
	)
{
	if(ClientInterfaceAddr == NULL)
		return;

	RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "P2PIndicateClientDisconnected(): ", ClientInterfaceAddr);
	
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_CLIENT_DISCONNECTED,
		RT_CUSTOM_INDI_TARGET_IHV,
		ClientInterfaceAddr,
		6);
}

VOID
P2PIndicateDeviceDiscoveryComplete(
	IN PP2P_INFO pP2PInfo
	)
{
	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateDeviceDiscoveryComplete()\n"));
	//P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_DEV_DISC_COMPLETE,
		RT_CUSTOM_INDI_TARGET_IHV,
		NULL,
		0);
}

VOID
P2PIndicateOnSDRsp(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_SD_CONTEXT pSDContext
	)
{
	u1Byte i = 0;
	u4Byte BufSize = FIELD_OFFSET(P2P_SD_RSP_CONTEXT, ServiceRspTLVList) + 
		pSDContext->ServiceRspRecvdSize * sizeof(P2P_SERVICE_RSP_TLV);
	PP2P_SD_RSP_CONTEXT pSDRspContext = NULL;

	PlatformAllocateMemory(pP2PInfo->pAdapter, (void**)&pSDRspContext, BufSize);

	if(!pSDRspContext) return;
	if(!pSDContext->bRequester) return;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateOnSDRsp: Recv Service Rsp TLVs:\n"));

	// Fill the SDRspContext
	pSDRspContext->SDStatus= pSDContext->Status;
	cpMacAddr(pSDRspContext->SourceDeviceAddress, pSDContext->TargetDeviceAddress);
	pSDRspContext->ServiceUpdateIndicator = pSDContext->ServiceUpdateIndicator;
	pSDRspContext->ServiceRspTLVSize = pSDContext->ServiceRspRecvdSize;
	for(i = 0; i < pSDContext->ServiceRspRecvdSize; i++)
	{// copy each Rsp TLV
		RT_TRACE(COMP_P2P, DBG_LOUD, ("TLV %u, Protocol ID: %u\n", 
			i, pSDContext->ServiceRspRecvd[i].ServiceDesc.ServiceType));
		RT_PRINT_STR(COMP_P2P, DBG_LOUD, "Recv Service Rsp TLV:", 
			pSDContext->ServiceRspRecvd[i].ServiceDesc.Buffer, 
			pSDContext->ServiceRspRecvd[i].ServiceDesc.BufferLength);
		
		PlatformMoveMemory(&(pSDRspContext->ServiceRspTLVList[i]), 
			&(pSDContext->ServiceRspRecvd[i]), 
			sizeof(P2P_SERVICE_RSP_TLV));
	}

	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_SD_RSP,
		RT_CUSTOM_INDI_TARGET_IHV,
		pSDRspContext,
		BufSize);

	PlatformFreeMemory(pSDRspContext, BufSize);
}

VOID
P2PIndicateOnSDReq(
	IN PP2P_INFO pP2PInfo,
	IN PP2P_SD_CONTEXT pSDContext
	)
{
	u1Byte i = 0;
	u4Byte BufSize = FIELD_OFFSET(P2P_SD_REQ_CONTEXT, ServiceReqTLVList) + 
		pSDContext->ServiceReqRecvdSize * sizeof(P2P_SERVICE_REQ_TLV);
	PP2P_SD_REQ_CONTEXT pSDReqContext = NULL;

	if(pSDContext->bRequester) return;

	PlatformAllocateMemory(pP2PInfo->pAdapter, (void**)&pSDReqContext, BufSize);

	if(!pSDReqContext) return;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateOnSDReq()\n"));

	// Fill the SDReqContext
	cpMacAddr(pSDReqContext->TargetDeviceAddress, pSDContext->TargetDeviceAddress);
	pSDReqContext->ServiceReqTLVSize = pSDContext->ServiceReqRecvdSize;
	for(i = 0; i < pSDContext->ServiceReqRecvdSize; i++)
	{// copy each Rsp TLV
		RT_TRACE(COMP_P2P, DBG_LOUD, ("TLV %u, Protocol ID: %u\n", 
			i, pSDContext->ServiceRspRecvd[i].ServiceDesc.ServiceType));
		RT_PRINT_STR(COMP_P2P, DBG_LOUD, "Recv Service Req TLV:", 
			pSDContext->ServiceRspRecvd[i].ServiceDesc.Buffer, 
			pSDContext->ServiceRspRecvd[i].ServiceDesc.BufferLength);
		
		PlatformMoveMemory(&(pSDReqContext->ServiceReqTLVList[i]), 
			&(pSDContext->ServiceReqRecvd[i]), 
			sizeof(P2P_SERVICE_REQ_TLV));
	}

	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_INDICATE_SD_REQ,
		RT_CUSTOM_INDI_TARGET_IHV,
		pSDReqContext,
		BufSize);

	PlatformFreeMemory(pSDReqContext, BufSize);
}

VOID
P2PIndicateScanList(
	IN PP2P_INFO pP2PInfo
	)
{
	u4Byte BufSize = 0;
	pu1Byte pBuf = NULL;
	u4Byte idxScanList = 0;
	BOOLEAN bAlloc = TRUE;

	do
	{
		//p2p_DevList_Translate(&pP2PInfo->devList, P2P_MAX_SCAN_LIST, &pP2PInfo->ScanList4QuerySize, pP2PInfo->ScanList4Query);
		
		if(0 == pP2PInfo->ScanList4QuerySize)
		{
			break;
		}
		
		if(0x00000000 == pP2PInfo->P2PVersion)
		{// v0
			PP2P_DEVICE_DESCRIPTOR_V0 pDesc = NULL;
			
			BufSize = sizeof(P2P_DEVICE_DESCRIPTOR_V0) * pP2PInfo->ScanList4QuerySize;
			if(0 == BufSize) break;
			PlatformAllocateMemory(pP2PInfo->pAdapter, &pBuf, BufSize);
			if(NULL == pBuf) break;

			pDesc = (PP2P_DEVICE_DESCRIPTOR_V0)pBuf;

			for(idxScanList = 0; idxScanList < pP2PInfo->ScanList4QuerySize; idxScanList++)
			{
				memcpy(pDesc + idxScanList, &pP2PInfo->ScanList4Query[idxScanList], sizeof(P2P_DEVICE_DESCRIPTOR_V0));
			}
		}
		else if(0x00000001 == pP2PInfo->P2PVersion)
		{// v1
			PP2P_DEVICE_DESCRIPTOR_V1 pDesc = NULL;
			
			BufSize = sizeof(P2P_DEVICE_DESCRIPTOR_V1) * pP2PInfo->ScanList4QuerySize;
			if(0 == BufSize) break;
			PlatformAllocateMemory(pP2PInfo->pAdapter, &pBuf, BufSize);
			if(NULL == pBuf) break;

			pDesc = (PP2P_DEVICE_DESCRIPTOR_V1)pBuf;

			for(idxScanList = 0; idxScanList < pP2PInfo->ScanList4QuerySize; idxScanList++)
			{
				memcpy(pDesc + idxScanList, &pP2PInfo->ScanList4Query[idxScanList], sizeof(P2P_DEVICE_DESCRIPTOR_V1));
			}		
		}
		else
		{// v2/v3
			pBuf = (pu1Byte)pP2PInfo->ScanList4Query;
			BufSize = sizeof(P2P_DEVICE_DESCRIPTOR_V2) * pP2PInfo->ScanList4QuerySize;
			bAlloc = FALSE;
		}
	}while(FALSE);

	if(pBuf || (NULL == pBuf && 0 == BufSize))
	{
		PlatformIndicateCustomStatus(
			pP2PInfo->pAdapter,
			RT_CUSTOM_EVENT_P2P_INDICATE_SCAN_LIST,
			RT_CUSTOM_INDI_TARGET_IHV,
			pBuf, BufSize);

		if(bAlloc && pBuf) PlatformFreeMemory(pBuf, BufSize);
	}
}

VOID
P2PIndicateChangingApChnl(
	IN  PP2P_INFO 		pP2PInfo,
	IN  u1Byte			oldChnl,
	IN  u1Byte			newChnl
	)
{
	u1Byte				buf[2];
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PIndicateChangingApChnl(): %u -> %u\n", oldChnl, newChnl));

	buf[0] = oldChnl;
	buf[1] = newChnl;

	PlatformIndicateCustomStatus(
		pP2PInfo->pAdapter,
		RT_CUSTOM_EVENT_P2P_CHANGING_AP_CHANNEL,
		RT_CUSTOM_INDI_TARGET_IHV,
		buf,
		sizeof(buf));
}

//======================================================================
// P2P Managmement
//======================================================================
VOID
P2PMgntTimerCallback(
	IN PRT_TIMER		pTimer
	)
{
	PADAPTER pAdapter = (PADAPTER)pTimer->Adapter;
	PMGNT_INFO pMgntInfo = &(pAdapter->MgntInfo);
	PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
	BOOLEAN bToFireTimer = TRUE;
	PADAPTER pLoopAdapter = NULL;
	PP2P_DEVICE_LIST_ENTRY pP2PDeviceListEntry = NULL;
	u4Byte	BoostInitGainValue = 0;

	BOOLEAN bScanInProgress;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

	VOID						*customScanInfo = GET_CUSTOM_SCAN_INFO(pAdapter);
	VOID 						*req = NULL;
	FRAME_BUF					*probeReqBuf = NULL;
	
	pP2PInfo->LastTimerFired = PlatformGetCurrentTime();

	if(!P2P_ENABLED(pP2PInfo))
	{
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		return;
	}

	if(RT_DRIVER_STOP(pAdapter))
	{
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		return;
	}

	PlatformAcquireSpinLock(pAdapter, RT_P2P_SPIN_LOCK);
	if(P2P_GET_REF_CNT(pP2PInfo) > 0)
	{
		PlatformReleaseSpinLock(pAdapter, RT_P2P_SPIN_LOCK);
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("The P2P command is setting in progress (RefCnt = %d), skip timer and set later!\n", pP2PInfo->RefCnt));
		PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_DEFAULT_MGNT_PERIOD);
		goto exit_P2PMgntTimerCallback;
	}
	PlatformReleaseSpinLock(pAdapter, RT_P2P_SPIN_LOCK);


	// Let the default port can connect to AP -----------------------------------------------------------
	{
		PADAPTER pClientPort = GetFirstClientPort(pP2PInfo->pAdapter);
		PADAPTER pDefaultPort = GetDefaultAdapter(pP2PInfo->pAdapter);

		if(pClientPort == NULL) pClientPort = pDefaultPort;
		
		if(MgntIsLinkInProgress(&pDefaultPort->MgntInfo) || MgntIsLinkInProgress(&pClientPort->MgntInfo))
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("MgntIsLinkInProgress(): default/client port link in progress\n"));

			return;
		}
	}
	// -------------------------------------------------------------------------------------------


	//
	// We assume that the optional Privision Discovery has already been done by the WPS module in the upper layer.
	//

	//
	// Indicate P2P State to IHVSvc
	//
	if(P2P_ADAPTER_RTK_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		P2PIndicateCurrentState(pP2PInfo, pP2PInfo->State);

		//
		// Force scan list indication
		//
		pP2PInfo->ForceScanListIndicateSlotCount++;
		if(pP2PInfo->ForceScanListIndicateSlotCount == P2P_FORCE_SCAN_LIST_INDICATE_PERIOD_SC)
		{	
			//RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Forced scan list indication\n"));
			pP2PInfo->ForceScanListIndicateSlotCount = 0;
			PlatformIndicateCustomStatus(
					pAdapter,
					RT_CUSTOM_EVENT_P2P_INDICATE_SCAN_LIST,
					RT_CUSTOM_INDI_TARGET_IHV,
					pP2PInfo->ScanList4Query,
					(sizeof(P2P_DEVICE_DISCRIPTOR) * pP2PInfo->ScanList4QuerySize));
		}
	}

	//
	// Notify UI the current device password ID so that it can determine whether provisioning info is available
	// Note that UI determines which peer config method we shall use for doing WPS
	//// TODO: this may be too regular, but note that UI may counting on this indication for determine provision info  timeout
	//P2PIndicateCurrentDevPasswdId(pP2PInfo, pP2PInfo->WpsDevPasswdId);	

	if(pP2PInfo->State == P2P_STATE_INITIALIZED)
	{
		pP2PInfo->ExtListenTimingPeriodSlotCount++;
	}
	else
	{// start counting in only initialized state
		pP2PInfo->ExtListenTimingPeriodSlotCount = 0;
	}

	if(pP2PInfo->bReinitiateConnection)
	{
		if(PlatformGetCurrentTime() - pP2PInfo->TimeStartWaitingForReinitiate > 120000000)
		{// 120 sec timeout
			pP2PInfo->bReinitiateConnection = FALSE;
			pP2PInfo->TimeStartWaitingForReinitiate = 0;
		}
	}

	// Timeout mechanism for Comeback Request
	if(pP2PInfo->SDContext.bDoingServiceDiscovery && pP2PInfo->SDContext.bFragment && !pP2PInfo->SDContext.bRequester)
	{
		pP2PInfo->SDWaitForComebackReqSlotCount++;
		if(pP2PInfo->SDWaitForComebackReqSlotCount == P2P_SD_COMEBACK_REQ_TIMEOUT_SC)
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): timeout waiting for ComebackReq\n"));
			PlatformZeroMemory(&pP2PInfo->SDContext, sizeof(P2P_SD_CONTEXT));
			pP2PInfo->SDWaitForComebackReqSlotCount = 0;
		}
	}

	/*RT_TRACE(COMP_P2P, DBG_LOUD, 
		("P2PMgntTimerCallback(): dc: %u, gc: %u, ExtSlot: %u, bExtListen: %u, bPreGroupFormation: %u\n", 
		pP2PInfo->DeviceCapability, 
		pP2PInfo->GroupCapability, 
		pP2PInfo->ExtListenTimingPeriodSlotCount,
		pP2PInfo->bExtendedListening,
		pP2PInfo->bPreGroupFormation));
	*/
	
	// Add "pHalData->SwChnlInProgress" to protect the 92D IQK process.
	if(pHalData->SwChnlInProgress)
	{
		PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_SWITCH_CHNL_PERIOD);
		goto exit_P2PMgntTimerCallback;	
	}	
	
	bScanInProgress = MgntScanInProgress(pMgntInfo);

	//RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): state: %u\n", pP2PInfo->State));
	
	if(bScanInProgress) 
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): scan in progress\n"));
		
		if(pP2PInfo->State == P2P_STATE_GO_NEGO_REQ_SEND ||
			pP2PInfo->State == P2P_STATE_PROVISION_DISCOVERY_REQ_SEND ||
			pP2PInfo->State == P2P_STATE_INVITATION_REQ_SEND ||
			pP2PInfo->State == P2P_STATE_SERVICE_DISCOVERY_REQ_SEND)
		{
			//
			// For these state, we shall enter them ASAP so that when the packet is sent,
			// the peer is still on the listen channel.
			// However, we can't arbitrarily change the timer because that will make extended listen 
			// timing incorrect.
			//
			PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_SWITCH_CHNL_PERIOD);
			goto exit_P2PMgntTimerCallback;
		}
		else
		{
			PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_SCAN_PERIOD_SCAN);
			goto exit_P2PMgntTimerCallback;
		}
		
		
	}
	else
	{// this is by referencing InactivePsTimerCallback()
		PADAPTER pDefaultAdapter = GetDefaultAdapter(pP2PInfo->pAdapter);
		PMGNT_INFO pDefaultMgntInfo = &pDefaultAdapter->MgntInfo;
		PRT_POWER_SAVE_CONTROL pPSC = GET_POWER_SAVE_CONTROL(pDefaultMgntInfo);

		//
		// If processing, we may have issued a customized scan and InactivePsWorkItemCallback
		// is still processing. In this case, bScanInProgress is false (but we expect it to be true).
		// If we let it go down, the P2P state machine will go to next state immediately, 
		// and the action should be taken in the previous state will not be done.
		//
		if (pPSC->bSwRfProcessing)
		{
			//RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback():bSwRfProcessing\n"));
			PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_SCAN_PERIOD_SCAN);
			goto exit_P2PMgntTimerCallback;
		}
	}

	switch(pP2PInfo->State) 
	{
		case P2P_STATE_INITIALIZED:
			{
				u8Byte curTime = PlatformGetCurrentTime();
				
				if(!P2PIsN24GSupported(pP2PInfo))
				{// don't do extended listening if 2.4G is not supported
					 break;
				}
				
				if(pP2PInfo->bExtendedListening)
				{
					P2PExtendedListenComplete(pP2PInfo);
				}

	
				if(pP2PInfo->ExtListenTimingPeriodSlotCount >= (u4Byte)P2P_EXT_LISTEN_TIMING_PERIOD_SC
					&& MultiportGetLastConnectionActionTime(pAdapter) + P2P_BLOCK_NORMAL_SCAN_PERIOD <= curTime
					)
				{// enter Listen State for pP2PInfo->ExtListenTimingDuration
					pP2PInfo->ExtListenTimingPeriodSlotCount = 1; // this slot also be the first slot

					if(pP2PInfo->ExtListenTimingDuration > 0)
					{
						P2PExtendedListenStart(pP2PInfo);

						RT_TRACE(COMP_P2P, DBG_LOUD, 
							("P2PMgntTimerCallback(): Wait for ExtListen: P2P_EXT_LISTEN_TIMING_PERIOD_SC = %d\n", 
							P2P_EXT_LISTEN_TIMING_PERIOD_SC));
						
						//
						// Next time we get in P2P_STATE_INITIALIZED is when Extended Listen complete.
						//
					}
	
					break;
				}
			}
			break;

		//======================================================================
		// Device Discovery
		//======================================================================
		case P2P_STATE_DEV_DISC_START:
			{	
				if(!P2PIsN24GSupported(pP2PInfo))
				{// don't do find and listen phase on 5G
					pP2PInfo->State = P2P_STATE_SCAN;

					if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
						break;
					probeReqBuf = CustomScan_GetProbeReqBuf(req);
					p2p_Construct_ProbeReq(probeReqBuf, pP2PInfo);
					P2PConstructScanList(pP2PInfo, req, P2P_LOWEST_RATE, probeReqBuf);
					CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "dev disc start");
					
					pP2PInfo->State = P2P_STATE_DEV_DISC_COMPLETE;
					break;
				}
				
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Start Device Discovery: loop times: %u\n", 
					pP2PInfo->ConnectionContext.FindPhaseLoopTimes));
	
				pP2PInfo->State = P2P_STATE_SCAN; // enter Scan Phase

				if(pP2PInfo->bPreGroupFormation||
					pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe ||
					pP2PInfo->ProvisionDiscoveryContext.bDoingProvisionDiscovery ||
					pP2PInfo->SDContext.bDoingServiceDiscovery ||
					pP2PInfo->ConnectionContext.FindPhaseLoopTimes == 0xFC)
				{// this is a discovery issued by invitation procedure => 
					//don't need the scan phase, enter listen state directly
					pP2PInfo->State = P2P_STATE_SEARCH; 
					PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
					bToFireTimer = FALSE;
					break;
				}


				if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
				{
					if(pP2PInfo->bDiscoverForSpecificChannels)
					{
						P2PDeviceDiscoverForSpecificChannels(
								pP2PInfo, 
								pP2PInfo->DiscoverForSpecificChannels, 
								pP2PInfo->uNumberOfDiscoverForSpecificChannels
							);
					
						pP2PInfo->State = P2P_STATE_SPECIAL_PEER_SEARCH;
						PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 50);
						bToFireTimer = FALSE;
						break;
					}
				}

				if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
				{
					// For Win 8: OID_DOT11_WFD_DISCOVER_REQUEST ------------------------------------------
					if(pP2PInfo->DiscoverSequence & P2P_DISCOVERY_SCAN_PHASE)
					{
						RT_TRACE(COMP_P2P,DBG_LOUD, ("DiscoverSequence & P2PDiscovery_scan_phase\n"));
					}
					else 
					{
						//Jump to the search state directly since the scan is not needed
						pP2PInfo->State = P2P_STATE_SEARCH; 
						PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
						bToFireTimer = FALSE;
						break;
					}
					// ------------------------------------------------------------------------------------
				}
//Sinda: temporarily mark. Because OS does not care bForceScanLegacyNetworks and it is always 0. 
//So at this condition, we do not check bForceScanLegacyNetworks and always do legacy scan.
#if 0
				if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
				{
					if(pP2PInfo->bForceScanLegacyNetworks == FALSE)
					{
						RT_TRACE(COMP_P2P,DBG_LOUD, ("pP2PInfo->bForceScanLegacyNetworks is FALSE and goto search state to scan social channel\n"));

						//Jump to the search state directly since the scan is not needed
						pP2PInfo->State = P2P_STATE_SEARCH; 
						PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
						bToFireTimer = FALSE;
						break;
					}
				}
#endif			
			}
			/*FALL THROUGH*/
		case P2P_STATE_SCAN:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Scan Phase\n"));

				if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
						break;
				probeReqBuf = CustomScan_GetProbeReqBuf(req);
				p2p_Construct_ProbeReq(probeReqBuf, pP2PInfo);
				P2PConstructScanList(pP2PInfo, req, MGN_1M, probeReqBuf);
				CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "scan phase");
					
				pP2PInfo->State = P2P_STATE_LISTEN; // enter Listen State
			}
			break;
		case P2P_STATE_LISTEN:
			{
				if(pP2PInfo->ScanDeviceIDs.uNumOfDeviceIDs > 0)
				{
					u1Byte i = 0;
					BOOLEAN bFound = FALSE;
			
					for(i = 0; i < pP2PInfo->DeviceListForQuery.uNumberOfDevices; i++)
					{
						if(pP2PInfo->DeviceListForQuery.DeviceEntry[i].DeviceAddress != NULL)
						{
							if( // Make sure the probe response is received 
								eqMacAddr(pP2PInfo->ScanDeviceIDs.DeviceIDs[0], pP2PInfo->DeviceListForQuery.DeviceEntry[i].DeviceAddress) &&
								pP2PInfo->DeviceListForQuery.DeviceEntry[i].ProbeResponseHostTimestamp != 0
							)
							{
								bFound = TRUE;
							}
						}
					}

					if(bFound == TRUE)
					{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: DeviceID Found : Terminate Search State !\n", __FUNCTION__));
						pP2PInfo->State = P2P_STATE_DEV_DISC_COMPLETE; 
						PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
						bToFireTimer = FALSE;
						break;
					}
				}

				// For Win 8: OID_DOT11_WFD_DISCOVER_REQUEST ------------------------------------------
				if(!(pP2PInfo->DiscoverSequence & P2P_DISCOVERY_FIND_PHASE) && P2PScanListAllFound(pP2PInfo))
				{
					//Bypass the find phase 
					RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Find device\n"));
					pP2PInfo->State = P2P_STATE_DEV_DISC_COMPLETE; 
					PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
					bToFireTimer = FALSE;
					break;
				}
				// ------------------------------------------------------------------------------------
			
				if(pP2PInfo->ConnectionContext.FindPhaseLoopTimes != 0) 
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Listen State\n"));
					

					if(P2P_SKIP_SCAN_LOOP_CNT > 0)
					{
						BOOLEAN		portConnected = FALSE;
						pLoopAdapter = GetDefaultAdapter(pAdapter);

						pP2PInfo->findPhaseSkipCnt = (pP2PInfo->findPhaseSkipCnt + 1) % (P2P_SKIP_SCAN_LOOP_CNT + 1);
						while(pLoopAdapter)
						{
							if(pLoopAdapter->MgntInfo.mAssoc)
							{
								portConnected = TRUE;								
								break;
							}							
							pLoopAdapter = GetNextExtAdapter(pLoopAdapter);
						}
						if(portConnected && P2P_SKIP_SCAN_LOOP_CNT == pP2PInfo->findPhaseSkipCnt)
						{
							RT_TRACE_F(COMP_P2P, DBG_LOUD, ("[P2P] Skip next P2P (%d) scan\n", pP2PInfo->findPhaseSkipCnt));
							PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, P2P_SKIP_SCAN_DURATION_MS);
							bToFireTimer = FALSE;
							break;
						}
					}

					if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
						break;
					probeReqBuf = CustomScan_GetProbeReqBuf(req);
					p2p_Construct_ProbeReq(probeReqBuf, pP2PInfo);
					P2PConstructScanList(pP2PInfo, req, P2P_LOWEST_RATE, probeReqBuf);
					CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "listen state");
					
					pP2PInfo->State = P2P_STATE_SEARCH; // enter Search State
					break;
				}
				else
				{
					pP2PInfo->State = P2P_STATE_DEV_DISC_COMPLETE;
					/*FALL THROUGH*/
				}
			}
		case P2P_STATE_DEV_DISC_COMPLETE:
			{
				P2P_STATE NextState = pP2PInfo->StateBeforeScan;
				BOOLEAN bFailedToArriveCommonChannel = FALSE;
				
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Device Discovery Complete\n"));
				if(pP2PInfo->bPreGroupFormation)
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Pre Group Formation Failed\n"));
					pP2PInfo->ConnectionContext.Status = P2P_STATUS_FAIL_COMMON_CHANNEL_NOT_ARRIVED;
					bFailedToArriveCommonChannel = TRUE;
					NextState = pP2PInfo->StateBeforeScan;

					pP2PInfo->bPreGroupFormation = FALSE;

					//
					// GO Nego failed, indicate to UI
					//
					pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)pP2PInfo->ConnectionContext.Status;
					P2PIndicateGOFormatedInfo(pP2PInfo, 
						pP2PInfo->ConnectionContext.Status, 
						pP2PInfo->ConnectionContext.bGoingToBeGO, 
						&pP2PInfo->ConnectionContext.ConnectingDevice);
					
					PlatformZeroMemory(&pP2PInfo->ConnectionContext, sizeof(pP2PInfo->ConnectionContext));
					
					p2p_IndicateActionFrameSendComplete(pP2PInfo, 
						P2P_EVENT_GO_NEGOTIATION_REQUEST_SEND_COMPLETE, 
						RT_STATUS_FAILURE, 
						NULL, 0);
				}
				else if(pP2PInfo->InvitationContext.bToSendInvitationReqOnProbe)
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Failed to send InvitationReq to peer\n"));
					pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_COMMON_CHANNEL_NOT_ARRIVED;
					bFailedToArriveCommonChannel = TRUE;
					NextState = P2P_STATE_INVITATION_COMPLETE;

				// For Win8: OID_DOT11_WFD_SEND_INVITATION_REQUEST -----------------------------------------------------
				if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
				{
					MEMORY_BUFFER mbObject = {NULL, 0};
					P2P_EVENT_DATA	eventData;
	
					PlatformZeroMemory(&eventData, sizeof(P2P_EVENT_DATA));
					eventData.rtStatus = RT_STATUS_FAILURE;
	
					mbObject.Buffer = (pu1Byte) &eventData;
					mbObject.Length = sizeof(P2P_EVENT_DATA);

					PlatformIndicateP2PEvent(pP2PInfo, P2P_EVENT_INVITATION_REQUEST_SEND_COMPLETE, &mbObject);
				}
				//------------------------------------------------------------------------------------------------------
				}
				else if(pP2PInfo->ProvisionDiscoveryContext.bDoingProvisionDiscovery)
				{
					P2P_DEV_LIST_ENTRY *pDev = NULL;

					if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
					{
						if(pP2PInfo->bProvisionRequestUseGroupID)
							pDev = p2p_DevList_GetGo(&pP2PInfo->devList, pP2PInfo->ProvisionDiscoveryContext.devAddr);
						else
							pDev = p2p_DevList_Get(&pP2PInfo->devList, pP2PInfo->ProvisionDiscoveryContext.devAddr, P2P_DEV_TYPE_DEV);
					}
					else
					{
						if(NULL == (pDev = p2p_DevList_GetGo(&pP2PInfo->devList, pP2PInfo->ProvisionDiscoveryContext.devAddr)))
							pDev = p2p_DevList_Get(&pP2PInfo->devList, pP2PInfo->ProvisionDiscoveryContext.devAddr, P2P_DEV_TYPE_DEV);						
					}

					if(pDev){
						pDev->p2p->pdStatus = P2P_SC_FAIL_COMMON_CHANNEL_NOT_ARRIVED;
					}
				
					RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Failed to send ProvisionDiscReq to peer\n"));
					
					bFailedToArriveCommonChannel = TRUE;
					NextState = P2P_STATE_PROVISION_DISCOVERY_COMPLETE;

					p2p_IndicateActionFrameSendComplete(pP2PInfo, 
						P2P_EVENT_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE, 
						RT_STATUS_FAILURE, 
						NULL, 0);
				}
				else if (pP2PInfo->SDContext.bDoingServiceDiscovery)
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Failed to send ServiceDiscoveryReq to peer\n"));
					pP2PInfo->SDContext.Status = P2P_SD_STATUS_COMMON_CHANNEL_NOT_ARRIVED;
					bFailedToArriveCommonChannel = TRUE;
					NextState = P2P_STATE_SERVICE_DISCOVERY_COMPLETE;
				}
				else
				{// the loop between listen and find phase has been completed 
					//P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
					
				}

				if(P2P_DOING_PURE_DEVICE_DISCOVERY(pP2PInfo))
				{
					P2PIndicateDeviceDiscoveryComplete(pP2PInfo);
				}

				P2PDeviceDiscoveryComplete(pP2PInfo, TRUE);

				if(bFailedToArriveCommonChannel)
				{
					pP2PInfo->State = NextState;
					PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
					bToFireTimer = FALSE;
					break;
				}
			}
			break;


		case P2P_STATE_SPECIAL_PEER_SEARCH:
			{
				if(pP2PInfo->ScanDeviceIDs.uNumOfDeviceIDs > 0)
				{
					u1Byte i = 0;
					BOOLEAN bFound = FALSE;
			
					for(i = 0; i < pP2PInfo->DeviceListForQuery.uNumberOfDevices; i++)
					{
						if(pP2PInfo->DeviceListForQuery.DeviceEntry[i].DeviceAddress != NULL)
						{
					if( // Make sure the probe response is received 
						eqMacAddr(pP2PInfo->ScanDeviceIDs.DeviceIDs[0], pP2PInfo->DeviceListForQuery.DeviceEntry[i].DeviceAddress) &&
						pP2PInfo->DeviceListForQuery.DeviceEntry[i].ProbeResponseHostTimestamp != 0
					)
						{
							bFound = TRUE;
						}
					}
					}

					if(bFound == TRUE)
					{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: DeviceID Found : Terminate Search State !\n", __FUNCTION__));
						pP2PInfo->State = P2P_STATE_DEV_DISC_COMPLETE; 
						PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
						bToFireTimer = FALSE;
						break;
					}
				}

				if(pP2PInfo->uNumberOfDiscoverRounds == 0)
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: pP2PInfo->uNumberOfDiscoverRounds == 0: Terminate Search State!\n", __FUNCTION__));
				
					pP2PInfo->State = P2P_STATE_DEV_DISC_COMPLETE;
					PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
					bToFireTimer = FALSE;
					break;
				}
				else
				{
					pP2PInfo->uNumberOfDiscoverRounds--;
					
					P2PDeviceDiscoverForSpecificChannels(
							pP2PInfo, 
							pP2PInfo->DiscoverForSpecificChannels, 
							pP2PInfo->uNumberOfDiscoverForSpecificChannels
						);

					pP2PInfo->State = P2P_STATE_SPECIAL_PEER_SEARCH;
					PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 50);
					bToFireTimer = FALSE;
					break;
				}
			}
			break;


		case P2P_STATE_SEARCH:
			{	
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Search State\n"));
				// For Win 8: OID_DOT11_WFD_DISCOVER_REQUEST ------------------------------------------
				if(!(pP2PInfo->DiscoverSequence & P2P_DISCOVERY_FIND_PHASE)&& P2PScanListAllFound(pP2PInfo))
				{
					//Bypass the find phase
					RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Find device\n"));
					pP2PInfo->State = P2P_STATE_DEV_DISC_COMPLETE; 
					PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
					bToFireTimer = FALSE;
					break;
				}
			// ------------------------------------------------------------------------------------

				if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
						break;
				probeReqBuf = CustomScan_GetProbeReqBuf(req);
				p2p_Construct_ProbeReq(probeReqBuf, pP2PInfo);
				P2PConstructScanList(pP2PInfo, req, P2P_LOWEST_RATE, probeReqBuf);
				CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "search state");
			
				pP2PInfo->State = P2P_STATE_LISTEN; // enter Listen State

				if(pP2PInfo->ConnectionContext.FindPhaseLoopTimes > 0) 
				{
					if(pP2PInfo->ConnectionContext.FindPhaseLoopTimes == 0xFF ||
						pP2PInfo->ConnectionContext.FindPhaseLoopTimes == 0xFC)  // i can't set it to 0xff in d.exe
					{// will loop infinitely
					}
					else
					{
						pP2PInfo->ConnectionContext.FindPhaseLoopTimes--;
					}
				}
					}
			break;
		//======================================================================
		// Provision Discovery
		//======================================================================
		case P2P_STATE_PROVISION_DISCOVERY_REQ_SEND:
			p2p_DevList_Lock(&pP2PInfo->devList);
			{
				P2P_DEV_LIST_ENTRY *pDev = NULL;

				RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PMgntTimerCallback(): Sending Provision Discovery Req\n"));

				pDev = p2p_DevList_Get(&pP2PInfo->devList, 
						pP2PInfo->ProvisionDiscoveryContext.go ? pP2PInfo->ProvisionDiscoveryContext.goBssid : pP2PInfo->ProvisionDiscoveryContext.devAddr, 
						pP2PInfo->ProvisionDiscoveryContext.go ? P2P_DEV_TYPE_GO : P2P_DEV_TYPE_DEV);
				
				if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
						break;
				probeReqBuf = CustomScan_GetProbeReqBuf(req);
				p2p_Construct_PDReq(pP2PInfo, probeReqBuf, pDev->mac, pDev->p2p->dialogToken, pDev->p2p->pdConfigMethod);
				P2PConstructScanList(pP2PInfo, req, P2P_LOWEST_RATE, probeReqBuf);
				CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "to send pd req");
				
				pP2PInfo->State = P2P_STATE_PROVISION_DISCOVERY_RSP_WAIT;
				PlatformSetTimer(
					pAdapter,
					&pP2PInfo->P2PMgntTimer,
					P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter) ? P2P_PROVISION_DISCOVERY_SHORT_TIMEOUT : P2P_PROVISION_DISCOVERY_TIMEOUT);
				bToFireTimer = FALSE;
			}
			p2p_DevList_Unlock(&pP2PInfo->devList);
			break;
		case P2P_STATE_PROVISION_DISCOVERY_RSP_WAIT:
			p2p_DevList_Lock(&pP2PInfo->devList);
			{
				P2P_DEV_LIST_ENTRY *pDev = NULL;

				pDev = p2p_DevList_Get(&pP2PInfo->devList, 
						pP2PInfo->ProvisionDiscoveryContext.go ? pP2PInfo->ProvisionDiscoveryContext.goBssid : pP2PInfo->ProvisionDiscoveryContext.devAddr, 
						pP2PInfo->ProvisionDiscoveryContext.go ? P2P_DEV_TYPE_GO : P2P_DEV_TYPE_DEV);
				
				RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PMgntTimerCallback(): Timeout waiting for Provision Discovery Rsp\n"));

				// Comment out because we use off chnl tx to do pd retry if OS support P2P natively
				if(0)//if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
				{
					pP2PInfo->ProvisionReqRetryCnt ++;
					if(pP2PInfo->ProvisionReqRetryCnt < P2P_PROVISION_DISCOVERY_RETRY_CNT)
					{
						RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Retry ProvisionReq!, retry = %d\n", pP2PInfo->ProvisionReqRetryCnt));
						pP2PInfo->State = P2P_STATE_INITIALIZED;
						p2p_DevList_Unlock(&pP2PInfo->devList);
						if(P2PProvisionDiscovery(
								pP2PInfo, 
								pP2PInfo->ProvisionDiscoveryContext.devAddr, 
								pDev->p2p->pdConfigMethod)
								)
						{
							PlatformSetTimer(
								pAdapter,
								&pP2PInfo->P2PMgntTimer,
								P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter) ? P2P_PROVISION_DISCOVERY_SHORT_TIMEOUT : P2P_PROVISION_DISCOVERY_TIMEOUT);
							bToFireTimer = FALSE;
							p2p_DevList_Unlock(&pP2PInfo->devList);
							break;
						}
					}					
				}
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_PROVISION_DISCOVERY_RSP!, retry = 0\n"));
				pP2PInfo->ProvisionReqRetryCnt = 0;
				pP2PInfo->State = P2P_STATE_PROVISION_DISCOVERY_COMPLETE;
				pDev->p2p->pdStatus = P2P_SC_FAIL_TIMEOUT_WAITING_FOR_PROVISION_DISCOVERY_RSP;
				/* FALL THROUGH */
			}
			p2p_DevList_Unlock(&pP2PInfo->devList);
			//break;
		case P2P_STATE_PROVISION_DISCOVERY_COMPLETE:
			{
				BOOLEAN bExtLsn = FALSE;
				
				p2p_DevList_Lock(&pP2PInfo->devList);
				{
					BOOLEAN bSucceed = FALSE;
					P2P_DEV_LIST_ENTRY *pDev = NULL;

					pDev = p2p_DevList_Get(&pP2PInfo->devList, 
							pP2PInfo->ProvisionDiscoveryContext.go ? pP2PInfo->ProvisionDiscoveryContext.goBssid : pP2PInfo->ProvisionDiscoveryContext.devAddr, 
							pP2PInfo->ProvisionDiscoveryContext.go ? P2P_DEV_TYPE_GO : P2P_DEV_TYPE_DEV);

					if(NULL == pDev)
					{
						RT_TRACE_F(COMP_P2P, DBG_SERIOUS, ("pDev = NULL\n"));
						RT_ASSERT(FALSE, ("pDev = NULL"));					
						PlatformZeroMemory(&pP2PInfo->ProvisionDiscoveryContext, sizeof(pP2PInfo->ProvisionDiscoveryContext));
						PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
						bToFireTimer = FALSE;
						p2p_DevList_Unlock(&pP2PInfo->devList);
						break;
					}

					if(NULL == pDev->p2p)
					{
						RT_TRACE_F(COMP_P2P, DBG_SERIOUS, ("pDev->p2p = NULL\n"));
						RT_ASSERT(FALSE, ("pDev->p2p = NULL"));					
						PlatformZeroMemory(&pP2PInfo->ProvisionDiscoveryContext, sizeof(pP2PInfo->ProvisionDiscoveryContext));
						PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
						bToFireTimer = FALSE;
						p2p_DevList_Unlock(&pP2PInfo->devList);
						break;
					}

					bSucceed = (P2P_SC_SUCCESS == pDev->p2p->pdStatus);

					pP2PInfo->ProvisionReqRetryCnt = 0;
					//
					// <Roger_Notes> Retrieve specific P2P device info. from Win8 Specific Device Information Pool to initiate boost initial gain
					// 2014.03.23.
					//
					if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
					{
						u2Byte	boostDelaySec = WPS_HANDSHAKE_TIMEOUT_SEC;
					
						McDynamicMachanismSet(pAdapter, MC_DM_INIT_GAIN_BOOST_END_DELAY_SEC, &boostDelaySec, sizeof(u2Byte));
						RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] P2P_STATE_PROVISION_DISCOVERY_COMPLETE:\n"));						
					}
					
					if(bSucceed)
					{
						RT_TRACE(COMP_P2P, DBG_LOUD, 
							("P2PMgntTimerCallback(): Provision Discovery succeed: %u\n", 
							pDev->p2p->pdStatus));

						P2PIndicateOnProvisionDiscoveryRsp(pP2PInfo, 
							pDev->p2p->pdConfigMethod,
							pDev->mac); // Set the ConfigMethod parameter to FFFF to indicate timeout

						P2PDeviceDiscoveryComplete(pP2PInfo, TRUE);
						bExtLsn = TRUE;

						if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
						{
							// this is now handled in the call back from OffChnlTx
							//p2p_IndicatePdReqSent(pDev, pP2PInfo);
							//p2p_IndicatePdRspReceived(pDev, pP2PInfo);
						}///if
					}
					else if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter) && pDev->txFrames[P2P_FID_PD_REQ])
					{// Note if we use OffChnlTx to send PD, it won't get here
						pDev->p2p->pdStatus = P2P_SC_SUCCESS;
						p2p_IndicatePdReqSent(pDev, pP2PInfo);
						p2p_IndicateFakePdRspReceived(pDev, pP2PInfo);
						P2PDeviceDiscoveryComplete(pP2PInfo, TRUE);
						bExtLsn = TRUE;
					}
					else
					{
						if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
						{
							p2p_IndicateActionFrameSendComplete(pP2PInfo,
								P2P_EVENT_PROVISION_DISCOVERY_REQUEST_SEND_COMPLETE,
								RT_STATUS_FAILURE,
								NULL,
								0
								);
						}
						RT_TRACE(COMP_P2P, DBG_LOUD, 
							("P2PMgntTimerCallback(): Provision Discovery failed: %u\n", pDev->p2p->pdStatus));

						if(pDev->p2p->pdStatus == P2P_SC_FAIL_COMMON_CHANNEL_NOT_ARRIVED)
						{
							pDev->p2p->pdConfigMethod = 0xFE;
						}
						else if(pDev->p2p->pdStatus == P2P_SC_FAIL_TIMEOUT_WAITING_FOR_PROVISION_DISCOVERY_RSP)
						{
							pDev->p2p->pdConfigMethod = 0xFF;
						}
						
						P2PSvc_OnSendPDReqFailure(pP2PInfo->pP2PSvcInfo, 
							pDev->mac,
							pDev->p2p->pdStatus
							);
						
						P2PIndicateOnProvisionDiscoveryRsp(pP2PInfo, 
							pDev->p2p->pdConfigMethod,
							pDev->mac); // Set the ConfigMethod parameter to FFFF to indicate timeout

						if(pDev->p2p->pdStatus != P2P_STATUS_FAIL_COMMON_CHANNEL_NOT_ARRIVED)
						{
							P2PDeviceDiscoveryComplete(pP2PInfo, TRUE); // state is recovered
						}
						else
						{
							pP2PInfo->State = pP2PInfo->StateBeforeScan;
						}
					}

					PlatformZeroMemory(&pP2PInfo->ProvisionDiscoveryContext, sizeof(pP2PInfo->ProvisionDiscoveryContext));
					PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
					bToFireTimer = FALSE;
				}
				p2p_DevList_Unlock(&pP2PInfo->devList);

				if(bExtLsn)
				{
					P2PExtendedListenStart(pP2PInfo);
				}
			}
			
			break;

		//======================================================================
		// Group Formation
		//======================================================================
		case P2P_STATE_GO_NEGO_REQ_SEND:
			p2p_DevList_Lock(&pP2PInfo->devList);
			{
				P2P_DEV_LIST_ENTRY *pDev = NULL;

				pDev = p2p_DevList_Get(&pP2PInfo->devList, 
								pP2PInfo->ConnectionContext.ConnectingDevice.DeviceAddress, 
								P2P_DEV_TYPE_DEV);
				
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Group Formating\n"));
				// Pause default adapter's traffic if group formating
				MultichannelHandlePacketDuringScan(GetDefaultAdapter(pAdapter), TRUE);

				//
				// <Roger_Notes> Retrieve specific P2P device info. from Win8 Specific Device Information Pool to initiate boost initial gain
				// 2014.04.03.
				//
				pP2PDeviceListEntry = P2PDeviceListFind(&pP2PInfo->DeviceList, pDev->mac);
				if(pP2PDeviceListEntry)				
				{						
					if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
					{						
						BoostInitGainValue = TRUE;
						BoostInitGainValue = (BoostInitGainValue | ((pP2PDeviceListEntry->RecvSignalPower+110) << 8));
						pP2PInfo->pAdapter->HalFunc.SetHwRegHandler(pP2PInfo->pAdapter, HW_VAR_BOOST_INIT_GAIN_OS, (pu1Byte)&BoostInitGainValue);
						RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] P2P_STATE_GO_NEGO_REQ_SEND: SignalStrength(%#x)\n", pP2PDeviceListEntry->RecvSignalPower));
					}
				}

				if(FALSE == P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
					pP2PInfo->GOIntent = ((pP2PInfo->GOIntent & 0xFE) | !(pP2PInfo->GOIntent & 0x01));
				
				p2p_Send_GoNegReq(pP2PInfo, pDev->mac, TRUE);

				pP2PInfo->State = P2P_STATE_GO_NEGO_RSP_WAIT;
				PlatformCancelTimer(pAdapter, &pP2PInfo->P2PMgntTimer);
				pP2PInfo->lastStateu8Time = PlatformGetCurrentTime();
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_GO_NEGO_FRAME_TIMEOUT);
				bToFireTimer = FALSE;
			}
			p2p_DevList_Unlock(&pP2PInfo->devList);
			break;
		case P2P_STATE_GO_NEGO_RSP_SEND:
			{
				pP2PInfo->State = P2P_STATE_GO_NEGO_CONFIRM_WAIT;
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_GO_NEGO_FRAME_TIMEOUT);
				bToFireTimer = FALSE;
			}
			break;
		case P2P_STATE_GO_NEGO_RSP_WAIT:
			{
				if(PlatformGetCurrentTime() < (pP2PInfo->lastStateu8Time + (P2P_GO_NEGO_FRAME_TIMEOUT * 1000)))
				{
					//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("CurTime=%"i64fmt"d < lastTime = %"i64fmt"d + %d, skip this timeout!\n", PlatformGetCurrentTime(), pP2PInfo->lastStateu8Time, (P2P_GO_NEGO_FRAME_TIMEOUT * 1000)));
					break;
				}
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Timeout waiting for GO Nego Rsp\n"));

				//
				// <Roger_Notes> Retrieve specific P2P device info. from Win8 Specific Device Information Pool to initiate boost initial gain
				// 2014.04.03.
				//
				if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
				{
					BoostInitGainValue = FALSE;
					pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_BOOST_INIT_GAIN_OS, (pu1Byte)&BoostInitGainValue);
					RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_GON_RSP:\n"));
				}

				pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
				pP2PInfo->ConnectionContext.Status = P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_GON_RSP;
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				bToFireTimer = FALSE;

				pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)pP2PInfo->ConnectionContext.Status;
				P2PIndicateGOFormatedInfo(pP2PInfo, 
					pP2PInfo->ConnectionContext.Status, 
					pP2PInfo->ConnectionContext.bGoingToBeGO, 
					&pP2PInfo->ConnectionContext.ConnectingDevice);
			}
			break;
		case P2P_STATE_GO_NEGO_CONFIRM_WAIT:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Timeout waiting for GO Nego Confirm\n"));
				
				pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
				pP2PInfo->ConnectionContext.Status = P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_GON_CONFIRM;
					PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
					bToFireTimer = FALSE;

					pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)pP2PInfo->ConnectionContext.Status;
					P2PIndicateGOFormatedInfo(pP2PInfo, 
						pP2PInfo->ConnectionContext.Status, 
						pP2PInfo->ConnectionContext.bGoingToBeGO, 
						&pP2PInfo->ConnectionContext.ConnectingDevice);
				}
			break;
		case P2P_STATE_GO_NEGO_COMPLETE:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): GO Nego Complete: %u\n",
					pP2PInfo->ConnectionContext.Status));

				//
				// <Roger_Notes> Retrieve specific P2P device info. from Win8 Specific Device Information Pool to initiate boost initial gain
				// 2014.04.03.
				//
				if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
				{
					u2Byte	boostDelaySec = WPS_HANDSHAKE_TIMEOUT_SEC;
					
					McDynamicMachanismSet(pAdapter, MC_DM_INIT_GAIN_BOOST_END_DELAY_SEC, &boostDelaySec, sizeof(u2Byte));
					RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] P2P_STATE_GO_NEGO_COMPLETE:\n"));
				}

				if(P2P_DOING_DEVICE_DISCOVERY(pP2PInfo))
				{
					P2PScanListCeaseScan(pP2PInfo);
					P2PDeviceDiscoveryComplete(pP2PInfo, FALSE);
				}

				pP2PInfo->PreviousGONegoResult = (P2P_STATUS_CODE)pP2PInfo->ConnectionContext.Status;
				
				if(pP2PInfo->ConnectionContext.Status == P2P_STATUS_SUCCESS)
				{
					//
					// Indicate GO formated info to ask UI to start provisioning
					//
					if(pP2PInfo->ConnectionContext.bGoingToBeGO)
					{ // Only copy the infomration in GO from our P2P info because these info shall be from ours but not the target (client).
						pP2PInfo->ConnectionContext.ConnectingDevice.OperatingChannel = pP2PInfo->OperatingChannel;
						CopySsid(pP2PInfo->ConnectionContext.ConnectingDevice.SsidBuf, pP2PInfo->ConnectionContext.ConnectingDevice.SsidLen, pP2PInfo->SSIDBuf, pP2PInfo->SSIDLen); 
					}
					//P2PDumpScanList(pP2PInfo->ScanList, pP2PInfo->ScanListSize);
					P2PDumpGroupFormationResult(pP2PInfo);
					P2PIndicateGOFormatedInfo(pP2PInfo, 
						pP2PInfo->ConnectionContext.Status, 
						pP2PInfo->ConnectionContext.bGoingToBeGO, 
						&pP2PInfo->ConnectionContext.ConnectingDevice);

					if(pP2PInfo->ConnectionContext.bGoingToBeGO)
					{// Indicate to the upper layer to start a P2P Role Port of type GO
						P2P_PROFILE P2PProfile;

						P2PProfile.OpChannel = pP2PInfo->OperatingChannel;

						//
						// GO SSID is determined when GONegoReq or GONegoRsp is received
						// and it is stored in pP2PInfo->SSIDBuf
						//
						CopySsid(P2PProfile.SsidBuf, P2PProfile.SsidBufLen, pP2PInfo->SSIDBuf, pP2PInfo->SSIDLen); 

						//
						// We use this to modify the SSID of the SoftAP
						//
						P2PIndicateStartApRequest(pP2PInfo, &P2PProfile);
					}

					pP2PInfo->State = P2P_STATE_INITIALIZED;
				}
				else
				{
					//
					// GO Nego failed, indicate to UI
					// For the failure case, indicate earlier when we determine the negotiation is failed.
					//
					//P2PIndicateGOFormatedInfo(pP2PInfo, 
					//	pP2PInfo->ConnectionContext.Status, 
					//	pP2PInfo->ConnectionContext.bGoingToBeGO, 
					//	&pP2PInfo->ConnectionContext.ConnectingDevice);
					
					PlatformZeroMemory(&pP2PInfo->ConnectionContext, sizeof(pP2PInfo->ConnectionContext));
					pP2PInfo->State = P2P_STATE_INITIALIZED;
				}
				
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				bToFireTimer = FALSE;		
			}
			break;
			
		//======================================================================
		// WPS
		//======================================================================
		case P2P_STATE_PRE_PROVISIONING:
			{
				u4Byte ConfigurationTimeout; // in ms, note that the configuration time out in the P2P IE is defined as units of 10ms

				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): P2P_STATE_PRE_PROVISIONING\n"));

				// Reset WPS related flags
				pP2PInfo->ConnectionContext.bDoingProvisioning = FALSE;

				// 
				// Clause 3.1.4.3:
				// Group Formation bit in the P2P Group Cap shall be set to 1 until Provisioning succeeds. 
				//
				pP2PInfo->GroupCapability |= gcGroupFormation; // set the bit

				if(pP2PInfo->ConnectionContext.bGoingToBeGO && 
					!P2P_ACTING_AS_GO(pP2PInfo)) // for the case that we invite a Dev to join our group, we shall not change SSID or restart AP
				{// start AP mode
					P2PSetRole(pP2PInfo, P2P_GO);

					//
					// GO does not check if provisioning succeed.
					//
					pP2PInfo->ConnectionContext.bDoingProvisioning = TRUE;
					pP2PInfo->ConnectionContext.ProvisioningResult = P2P_PROVISIONING_RESULT_SUCCEED;
				}
				else
				{//P2P Client, will connect to the GO later
					pP2PInfo->ConnectionContext.bDoingProvisioning = TRUE;
					pP2PInfo->ConnectionContext.ProvisioningResult = P2P_PROVISIONING_RESULT_UNKNOWN;
				}

				// 
				// Clause 3.1.3.4:
				// To allow for P2P Device configuration, P2P Devices may delay starting the Provisioning phase
				// until the expiration of the max of the P2P GO's GO Config Time and the Client's Client Config Time.
				//
				if(pP2PInfo->ConnectionContext.bGoingToBeGO)
				{// I'm going to be the GO => just enter provisioning state without waiting for configuration timeout 
					ConfigurationTimeout = 0; //(10 * MAX(pP2PInfo->GOConfigurationTimeout, pP2PInfo->ConnectionContext.ConnectingDevice.ClientConfigurationTimeout));
				}
				else 
				{// I'm going to be the Client
					ConfigurationTimeout = (10 * MAX(pP2PInfo->ClientConfigurationTimeout, pP2PInfo->ConnectionContext.ConnectingDevice.GOConfigurationTimeout));
				}


			if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
			{
				// For Win 8: Return back to the initialization state -----------------------------------------
				PlatformZeroMemory(&pP2PInfo->ConnectionContext, sizeof(pP2PInfo->ConnectionContext));
				pP2PInfo->Role = P2P_DEVICE;
				pP2PInfo->State = P2P_STATE_INITIALIZED;
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				bToFireTimer = FALSE;	
				// ----------------------------------------------------------------------------------
			}
			else
			{
				//
				// Wait for configuration before starting provisioning
				//
				pP2PInfo->State = P2P_STATE_PROVISIONING;
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, ConfigurationTimeout); // wait for configuration
				bToFireTimer = FALSE;	
			}
		}
			break;
		case P2P_STATE_PROVISIONING:
			{	
				//RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): P2P_STATE_PROVISIONING\n"));
			
				//if(pP2PInfo->ConnectionContext.bDoingProvisioning)
				{// doing provisioning
					//
					// During the time waiting for provisioning complete, we keep in P2P_STATE_PROVISIONING.
					// Once ProvisioningResult is set, we infer that provisioning completed.
					//

					if(pP2PInfo->ConnectionContext.ProvisioningResult == P2P_PROVISIONING_RESULT_SUCCEED)
					{// Provisioning succeed
						P2PSetOperatingState(pP2PInfo);
						if(pP2PInfo->ConnectionContext.bGoingToBeGO)
						{	
						}
						else
						{// set client role
							P2PSetRole(pP2PInfo, P2P_CLIENT);
							RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): provisioning succeed\n"));
						}
						
						PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
						bToFireTimer = FALSE;	
					}
					else if(pP2PInfo->ConnectionContext.ProvisioningResult == P2P_PROVISIONING_RESULT_FAILED)
					{// Provisioning failed => indicate Group Formation failed
						//
						// Clause 3.1.4.3: End the P2P Group session as described in Clause 3.2.9
						//
						SendDeauthentication(pP2PInfo->pAdapter, BroadcastAddress, deauth_lv_ss);
						
						pP2PInfo->State = P2P_STATE_INITIALIZED;
						PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_DEFAULT_MGNT_PERIOD);
						bToFireTimer = FALSE;	

						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): provisioning failed\n"));
					}
					else
					{// Provisioning in progress => keep waiting
						//RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): wait for provisioning\n"));
						PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_DEFAULT_MGNT_PERIOD);
						bToFireTimer = FALSE;
						break;
					}

					if(pP2PInfo->ConnectionContext.ProvisioningResult != P2P_PROVISIONING_RESULT_UNKNOWN)
					{// Provisioning has been done, either failed or succeed
						// 
						// Clause 3.1.4.3:
						// Group Formation bit in the P2P Group Cap shall be set to 1 until Provisioning succeeds. 
						//
						pP2PInfo->GroupCapability &= ~gcGroupFormation; // clear the bit
						RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): provisioning done\n"));

						//
						// Reset ConnectionContext since this connection session is completed
						//
						PlatformZeroMemory(&pP2PInfo->ConnectionContext, sizeof(pP2PInfo->ConnectionContext));
					}
				}
			}
			break;
			
		//======================================================================
		// Group Operation
		//======================================================================
		case P2P_STATE_OPERATING:
			{

			if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
			{
					// Do nothing: Just Waiting for Win8 OID
			}
			else
			{
				//RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): P2P_STATE_OPERATING\n"));
				if(P2P_ACTING_AS_GO(pP2PInfo))
				{

					if(P2PClientInfoGetCount(pP2PInfo) == 0 && 
							!pP2PInfo->bGOStartedAutonomously) // not to check this if GO started autonomously
					{// no client
						pP2PInfo->P2PGONoClientSlotCount++;
						RT_TRACE(COMP_P2P, DBG_LOUD, 
							("P2PMgntTimerCallback(): no client slot count: %d\n", 
							pP2PInfo->P2PGONoClientSlotCount));
					}
					else
					{
						pP2PInfo->ExtListenTimingPeriodSlotCount = 0;
					}
					
					if(pP2PInfo->P2PGONoClientSlotCount == P2P_NO_CLIENT_PERIOD_SC)
					{			
						P2PDisconnect(pP2PInfo); // to stop the SoftAP and back to init state
						pP2PInfo->P2PGONoClientSlotCount = 0;

						// Comment out because these are handled in P2PDisconnect.
						//pP2PInfo->State = P2P_STATE_INITIALIZED;
						//pP2PInfo->Role = P2P_DEVICE;
						
						bToFireTimer = FALSE;
						
						RT_TRACE(COMP_P2P, DBG_LOUD, 
							("P2PMgntTimerCallback(): Stop GO and back to P2P_STATE_INITIALIZED\n"));
					}
				}
				else if(P2P_ACTING_AS_CLIENT(pP2PInfo))
				{// operating as a client, check if we are still connected
					if(!P2PDefaultPortConnected(pP2PInfo) && !P2PProvisioning(pP2PInfo))
					{
						pP2PInfo->P2PClientDisconnectedSlotCount++;
						RT_TRACE(COMP_P2P, DBG_LOUD, 
							("P2PMgntTimerCallback(): P2P Client disconnected slot count: %d\n", 
							pP2PInfo->P2PClientDisconnectedSlotCount));
					}
					else
					{// def port connected or doing provisioning
						pP2PInfo->P2PClientDisconnectedSlotCount = 0;
					}
					
					if(pP2PInfo->P2PClientDisconnectedSlotCount == P2P_CLIENT_DISCONNECTED_PERIOD_SC)
					{
						P2PDisconnect(pP2PInfo); // will back to init state
						pP2PInfo->P2PClientDisconnectedSlotCount = 0;
						pP2PInfo->State = P2P_STATE_INITIALIZED;
						P2PSetRole(pP2PInfo, P2P_DEVICE);
						RT_TRACE(COMP_P2P, DBG_LOUD, 
							("P2PMgntTimerCallback(): back to P2P_STATE_INITIALIZED because we are acting as Client but default port is disconnected\n"));

					}
				}
			}

				
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_DEFAULT_MGNT_PERIOD);
				bToFireTimer = FALSE;	
			}
			break;
		//======================================================================
		// Invitation
		//======================================================================
		case P2P_STATE_INVITATION_REQ_SEND:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): P2P_STATE_INVITATION_REQ_SEND\n"));

				if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
						break;
				probeReqBuf = CustomScan_GetProbeReqBuf(req);
				p2p_Construct_InvitationReq(pP2PInfo, probeReqBuf, 
					pP2PInfo->InvitationContext.InvitedDevice.DeviceAddress, 
					pP2PInfo->InvitationContext.DialogToken);
				P2PConstructScanList(pP2PInfo, req, P2P_LOWEST_RATE, probeReqBuf);
				CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "to send inv req");

				pP2PInfo->State = P2P_STATE_INVITATION_RSP_WAIT;
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_INVITATION_FRAME_TIMEOUT);
				bToFireTimer = FALSE;
			}
			break;
		case P2P_STATE_INVITATION_RSP_WAIT:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Timeout waiting for InvitationRsp\n"));
				pP2PInfo->State = P2P_STATE_INVITATION_COMPLETE;
				pP2PInfo->InvitationContext.Status = P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_INVITATION_RSP;

				if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter) && !pP2PInfo->pAdapter->bInHctTest)
				{
					if(P2PIndicateFakeInvitRsp(pP2PInfo))
					{
						pP2PInfo->InvitationContext.Status = P2P_STATUS_SUCCESS;
					}
				}
				/*FALL THROUGH*/
			}
			//break;
		case P2P_STATE_INVITATION_COMPLETE:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PMgntTimerCallback(): P2P_STATE_INVITATION_COMPLETE, status: %u, persistent: %u, ch: %u\n",
					pP2PInfo->InvitationContext.Status, pP2PInfo->InvitationContext.bPersistentInvitation, pP2PInfo->InvitationContext.OpChannel));

				//
				// <Roger_Notes> Resume boost initial gain for P2P invitation response timeout or process completion.
				// 2013.10.15.
				//
				if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
				{
					u2Byte	boostDelaySec = ASSOC_HANDSHAKE_DHCP_DELAY_SEC;
					
					McDynamicMachanismSet(pAdapter, MC_DM_INIT_GAIN_BOOST_END_DELAY_SEC, &boostDelaySec, sizeof(u2Byte));
					RT_TRACE(COMP_P2P, DBG_LOUD, ("[BOOST_INIT_GAIN_OS] P2P_STATE_INVITATION_COMPLETE:\n"));
				}

				//
				// We use P2PDeviceDiscovery() to arrive common chnll before sendding InvitationReq,
				// In P2PDeviceDiscovery(), beacon is stopped, so we have to resume beaconning here.
				//
			 	// If P2P_STATUS_FAIL_COMMON_CHANNEL_NOT_ARRIVED, 
			 	// P2PDeviceDiscoveryComplete() is called in P2P_STATE_DEV_DISC_COMPLETE case,
			 	// so we don' t have to call it again
			 	//
			 	if(pP2PInfo->InvitationContext.Status != P2P_STATUS_FAIL_COMMON_CHANNEL_NOT_ARRIVED)
			 	{
					P2PDeviceDiscoveryComplete(pP2PInfo, TRUE);
			 	}
				else
				{
					pP2PInfo->State = pP2PInfo->StateBeforeScan;
				}
				
				P2PInvitePeerComplete(pP2PInfo);

				P2PIndicateOnInvitationRsp(pP2PInfo, 
					pP2PInfo->InvitationContext.Status, 
					pP2PInfo->InvitationContext.bPersistentInvitation,
					pP2PInfo->InvitationContext.OpChannel,
					pP2PInfo->InvitationContext.InvitorRole, 
					pP2PInfo->InvitationContext.SsidLen, 
					pP2PInfo->InvitationContext.SsidBuf);

				// Indicating InvitationRsp may trigger a connect request which may further triger a reset request,
				// and reset request resets all timers including the P2P mgnt timer.
				// Indicate current state immediately so that upper layers will not show "inviting" for a long time.
				//P2PIndicateCurrentState(pP2PInfo, pP2PInfo->State);
				
				if(!pP2PInfo->InvitationContext.bWaitingBackwardInvite)
				{
					PlatformZeroMemory(&(pP2PInfo->InvitationContext), sizeof(P2P_INVITATION_CONTEXT));
				}

				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				bToFireTimer = FALSE;
			}
			break;

		//======================================================================
		// Device Discoverability
		//======================================================================
		case P2P_STATE_DEVICE_DISCOVERABILITY_WAIT_BEACON:
			{// timeout waiting for beacon from the GO
				pP2PInfo->DeviceDiscoverabilityContext.Status = P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_GO_BEACON;
				RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PMgntTimerCallback(): P2P_STATE_DEVICE_DISCOVERABILITY_WAIT_BEACON: Timeout waiting for GO Beacon\n"));
				pP2PInfo->State = P2P_STATE_DEVICE_DISCOVERABILITY_COMPLETE;
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				bToFireTimer = FALSE;
			}
			break;
			
		case P2P_STATE_DEVICE_DISCOVERABILITY_REQ_SEND:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): P2P_STATE_INVITATION_REQ_SEND\n"));

				p2p_Send_DevDiscReq(pP2PInfo, pP2PInfo->DeviceDiscoverabilityContext.GoBssid,
									pP2PInfo->DeviceDiscoverabilityContext.ClientDeviceAddress,
									pP2PInfo->DeviceDiscoverabilityContext.DialogToken);
									

				pP2PInfo->State = P2P_STATE_DEVICE_DISCOVERABILITY_RSP_WAIT;
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_DEVICE_DISCOVERABILITY_FRAME_TIMEOUT);
				bToFireTimer = FALSE;
			}
			break;
			
		case P2P_STATE_DEVICE_DISCOVERABILITY_RSP_WAIT:
			{// timeout waithing for DeviceDiscoverabilityRsp
				pP2PInfo->DeviceDiscoverabilityContext.Status = P2P_STATUS_FAIL_TIMEOUT_WAITING_FOR_DEVICE_DISCOVERABILITY_RSP;
				RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PMgntTimerCallback(): P2P_STATE_DEVICE_DISCOVERABILITY_RSP_WAIT: Timeout waiting for DeviceDiscoverabilityRsp\n"));
			}
			/*FALL THROUGH*/
			
		case P2P_STATE_DEVICE_DISCOVERABILITY_COMPLETE:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PMgntTimerCallback(): P2P_STATE_DEVICE_DISCOVERABILITY_COMPLETE: Status(%u)\n",
					pP2PInfo->DeviceDiscoverabilityContext.Status));


				pP2PInfo->DeviceDiscoverabilityContext.bWaitingBeaconFromGO = FALSE;

				if(!pP2PInfo->DeviceDiscoverabilityContext.bGoConnect)
				{
					pP2PInfo->State = P2P_STATE_INITIALIZED;
					P2PSetChannel(pP2PInfo, pP2PInfo->DeviceDiscoverabilityContext.OriginalChannel);
				}
				else
				{
					pP2PInfo->State = P2P_STATE_GO_NEGO_REQ_SEND;
				}
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				bToFireTimer = FALSE;
			}
			break;
		//======================================================================
		// Service Discovery
		//======================================================================
		case P2P_STATE_SERVICE_DISCOVERY_REQ_SEND:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PMgntTimerCallback(): P2P_STATE_SERVICE_DISCOVERY_REQ_SEND\n"));

				/* we will never reach here, see P2PCommonChannelArrived */
			}
			break;
			
		case P2P_STATE_SERVICE_DISCOVERY_COMEBACK_RSP_WAIT:	
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Timeout waiting for ComebackRsp\n"));

				pP2PInfo->SDContext.Status = P2P_SD_STATUS_COMEBACK_RSP_TIMEOUT;

				pP2PInfo->State = P2P_STATE_SERVICE_DISCOVERY_COMPLETE;
				PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				bToFireTimer = FALSE;
			}
			break;
			
		case P2P_STATE_SERVICE_DISCOVERY_RSP_WAIT:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PMgntTimerCallback(): Timeout waiting for SDRsp\n"));

				pP2PInfo->SDContext.Status = P2P_SD_STATUS_SERVICE_RSP_TIMEOUT;

				pP2PInfo->State = P2P_STATE_SERVICE_DISCOVERY_COMPLETE;
				PlatformSetTimer(pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				bToFireTimer = FALSE;
			}
			break;
			
		case P2P_STATE_SERVICE_DISCOVERY_COMPLETE:
			{
				RT_TRACE(COMP_P2P, DBG_LOUD, 
					("P2PMgntTimerCallback(): SD Complete: Status (%u)\n",
					pP2PInfo->SDContext.Status));

				if(pP2PInfo->SDContext.Status != P2P_SD_STATUS_COMMON_CHANNEL_NOT_ARRIVED)
				{
					P2PDeviceDiscoveryComplete(pP2PInfo, TRUE);
				}
				else
				{
					pP2PInfo->State = pP2PInfo->StateBeforeScan;
				}

				if(!(pP2PInfo->SDContext.Status == P2P_SD_STATUS_SUCCESS)) // handled when status is determined to be success
				{
					// Notify the SDRsp
					P2PIndicateOnSDRsp(pP2PInfo, &pP2PInfo->SDContext);
				}

				// Clear SDContext
				PlatformZeroMemory(&pP2PInfo->SDContext, sizeof(P2P_SD_CONTEXT));
				
				PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				bToFireTimer = FALSE;
			}
			break;

		default:
			break;
	}

	if(pP2PInfo->Role == P2P_NONE)
	{
		bToFireTimer = FALSE;
	}

	if(bToFireTimer)
	{
		PlatformSetTimer( pAdapter, &pP2PInfo->P2PMgntTimer, P2P_DEFAULT_MGNT_PERIOD);
	}
	
exit_P2PMgntTimerCallback:
	return;
	
}

VOID
P2PMlmeAssociateRequest(
	PADAPTER		Adapter,
	pu1Byte			asocStaAddr,
	u4Byte			asocTmot,
	u2Byte			asCap,
	u2Byte			asListenInterval,
	BOOLEAN			Reassociate	
	)
{
	PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);
	PRT_WLAN_BSS pBssDesc ;
	
	if(!P2P_ENABLED(GET_P2P_INFO(Adapter)))
	{
		return;
	}

	pBssDesc = BssDescDupByBssid(pP2PInfo->pAdapter, asocStaAddr);

	if(pBssDesc)
	{
		if(!(pBssDesc->P2PManagedInfo & maP2PCrossConnectionPermitted) &&
			(pP2PInfo->GroupCapability & gcCrossConnection))
		{// we support cross conn but the connecting ap does not allow that
			RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, 
				"P2PMlmeAssociateRequest(): Cross connection is not allowed by the AP => disable cross connection\n", asocStaAddr);
			pP2PInfo->GroupCapability &= ~gcCrossConnection;
		}
	}

	return;
}

//
// Description:
//	Handle association response packet with status "suceess" for P2P function.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] pRfd -
//		The packet container including the information of packet.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
//		
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// Remark:
//	This function updates the listen channel and operating channel if the client is assocating to
//	the legacy AP.
//
RT_STATUS
P2P_OnAssocOK(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS				rtStatus = RT_STATUS_SUCCESS;	
	PP2P_INFO				pP2PInfo = GET_P2P_INFO(pAdapter);
	PMGNT_INFO				pMgntInfo = &(pAdapter->MgntInfo);
	pu1Byte					DeviceAddress = NULL;
	pu1Byte					InterfaceAddress = Frame_pSaddr(*posMpdu);
	PP2P_DEVICE_DISCRIPTOR	pP2PDeviceDesc = NULL;  
	PADAPTER 				pLoopAdapter = NULL;
	PP2P_INFO				pLoopP2PInfo = NULL;

	P2P_MESSAGE				msg;	
			
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("connected with Legacy AP\n"));		

	FunctionIn(COMP_P2P);

	PlatformZeroMemory(&msg, sizeof(P2P_MESSAGE));
	
	do
	{
		BOOLEAN				bGo = FALSE;

		for(pLoopAdapter = GetDefaultAdapter(pAdapter), pLoopP2PInfo = (pLoopAdapter ? GET_P2P_INFO(pLoopAdapter) : NULL);
			pLoopAdapter && pLoopP2PInfo;
			pLoopAdapter = GetNextExtAdapter(pLoopAdapter),
			pLoopP2PInfo = (pLoopAdapter ? GET_P2P_INFO(pLoopAdapter) : NULL))
		{
			if(!P2P_ENABLED(pLoopP2PInfo))
				continue;
			
			if(P2P_CLIENT != pLoopP2PInfo->Role && P2P_GO != pLoopP2PInfo->Role)
			{
				pLoopP2PInfo->OperatingChannel = pMgntInfo->dot11CurrentChannelNumber;
				if(P2PIsSocialChannel(pLoopP2PInfo->OperatingChannel))
					pLoopP2PInfo->ListenChannel = pLoopP2PInfo->OperatingChannel;
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2P op chnl changed to %u, listenChnl = %d for adapter (%p)\n",
					pLoopP2PInfo->OperatingChannel, pLoopP2PInfo->ListenChannel, pLoopAdapter));
			}
		}

		// Prefast warning C28182: Dereferencing NULL pointer. '((pAdapter))->pPortCommonInfo->pDefaultAdapter' contains the same NULL value as 'pLoopAdapter' did.
		if(((pAdapter))->pPortCommonInfo->pDefaultAdapter != NULL && !P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		{
			rtStatus = RT_STATUS_SUCCESS;
			break;
		}
		
		if(RT_STATUS_SUCCESS == (rtStatus = p2p_parse_Ies(posMpdu, DBG_LOUD, &msg)))
			if(!p2p_validate_AssocRsp(&msg))
			{
				rtStatus = RT_STATUS_MALFORMED_PKT;
				break;
			}

		//
		// Clause 4.2.5, At least 1 P2P IE is always included in the AssocRsp frame.
		//
		// osP2PIE = PacketGetElement(asocpdu, EID_Vendor, OUI_SUB_WIFI_DIRECT, OUI_SUB_DONT_CARE);
		bGo = (NULL != FrameBuf_Head(&msg.p2pAttributes));
		
		// Update scan list for query so that upper layers can query the most up to date scan list to determine
		// whether the connecting AP is a GO.
		P2P_UpdateScanList(pAdapter);

		//
		// Check if the connecting AP is actually a GO
		// Note that the P2P IE may contain no P2P attributes, so we don't check osP2PIE.Length > 0
		//
		if(bGo)
		{// the AssocRsp is from a P2P Device
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("connected with GO: Role is %d, State is %d\n", pP2PInfo->Role, pP2PInfo->State));
			if(pP2PInfo->Role == P2P_DEVICE &&
				pP2PInfo->State != P2P_STATE_PROVISIONING)
			{
				P2PSetRole(pP2PInfo, P2P_CLIENT);
				P2PSetOperatingState(pP2PInfo);
				PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
				PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);
				P2PIndicateCurrentState(pP2PInfo, pP2PInfo->State); // update op-state immediately
			}
		}
		else
		{// connect to a legacy AP
			if(P2PIsSocialChannel(pMgntInfo->dot11CurrentChannelNumber))
			{
				pP2PInfo->OperatingChannel = MgntActQuery_802_11_CHANNEL_NUMBER(pP2PInfo->pAdapter);
				pP2PInfo->ListenChannel = MgntActQuery_802_11_CHANNEL_NUMBER(pP2PInfo->pAdapter);
	
				// Also adjust GO intent to 15 so that we won't become a P2P Client after negotiation.
				{
					pP2PInfo->GOIntent = ((15 << 1) | (pP2PInfo->GOIntent & 0x01));
				}

				//
				// Readiness event: for 7.1.5
				//
				if(pP2PInfo->Role == P2P_CLIENT)
				{// we are a P2P Client and now connecting to a legacy AP => back to device role
					P2PSetRole(pP2PInfo, P2P_DEVICE);
					pP2PInfo->State = P2P_STATE_INITIALIZED;
					PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
					PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);

				}
	
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2P listen chnl changed to %u\n", pMgntInfo->dot11CurrentChannelNumber));
			}	
		}
	
		PlatformSetTimer(pAdapter, &(pP2PInfo->P2PMgntTimer), 0);	
	}while(FALSE);

	p2p_parse_FreeMessage(&msg);
 
	return rtStatus;
}

//
// Description:
//	Set P2P Power Save mode for GO.
// Arguments:
//	[in] pP2PInfo - 
//		The P2P context.
//	[in] pP2pPs -
//		The address of P2P power save setting.
//	[in] NoAIndex -
//		The NoA index, only applied to Client.
//	[in] psReason -
//		It't the reason why or who updates the power save configuration.
// Return:
//	If success, return RT_STATUS_SUCCESS; Otherwise, return an error code.
// Note:
//	The current setting is only applied to the PCI interface since we must get the TSF timer
//	directly from the NIC's register.
// By Bruce, 2011-02-10.
//
u4Byte
P2PSetPowerSaveMode(
	IN	PP2P_INFO				pP2PInfo,
	IN	PP2P_POWERSAVE_SET		pP2pPs,
	IN	u1Byte					NoAIndex,
	IN	P2P_PS_UPDATE_REASON	psReason
	)
{
	PADAPTER			pAdapter = NULL;

	if(!P2P_ENABLED(pP2PInfo))
		return RT_STATUS_FAILURE;

	if(!pP2pPs || !pP2PInfo)
		return RT_STATUS_FAILURE;

	pAdapter = pP2PInfo->pAdapter;

	// Clear the PS level first.
	RT_CLEAR_PS_LEVEL(pAdapter, RT_RF_LPS_P2P_PS);

	// Invalid role
	if((pP2PInfo->Role != P2P_GO) && (pP2PInfo->Role != P2P_CLIENT))
		return RT_STATUS_FAILURE;

	P2PSetPsState(pP2PInfo, P2P_PS_CANCEL_ALL_PS, P2P_PS_AWAKE, 0);	

	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PSetPowerSaveMode(): Reason = %d\n", psReason));		

	PlatformMoveMemory(&pP2PInfo->P2pPsConfgSet, pP2pPs, sizeof(P2P_POWERSAVE_SET));

	if(psReason == P2P_PS_UPDATE_BY_USER && pP2PInfo->Role == P2P_CLIENT)
	{
		// This is a presence request in p2p client.
		return P2PPresenceReq(pP2PInfo, &pP2PInfo->P2pPsConfgSet);
	}

	if(pP2PInfo->Role == P2P_GO)
		pP2PInfo->NoAIEIndex = ((u2Byte)(pP2PInfo->NoAIEIndex + 1) > 0xFF) ? 0 : (pP2PInfo->NoAIEIndex + 1);
	else
		pP2PInfo->NoAIEIndex = NoAIndex;

	pP2PInfo->bUpdatePsParameter = TRUE;

		// For non-PCI interface, we always schedule workitem to update P2P PS mode.
		if(!PlatformIsWorkItemScheduled(&(pP2PInfo->P2PPSWorkItem)))
			PlatformScheduleWorkItem(&(pP2PInfo->P2PPSWorkItem));
		return RT_STATUS_SUCCESS;

	P2PUpdatePowerSavePara(pP2PInfo);

	if(pP2PInfo->psExeType == RT_P2P_PS_EXE_BY_SW_HW_TIMER)
	{ // Driver Timer
		P2PPsTimeout(pAdapter);
	}
	else if(pP2PInfo->psExeType == RT_P2P_PS_EXE_BY_HW && pAdapter->MgntInfo.bWiFiConfg)
	{ // HW caculate			
		P2PSetPsState(pP2PInfo, P2P_PS_CANCEL_ALL_PS, P2P_PS_AWAKE, 0);
	}
	else
	{
		// Not support.
		return RT_STATUS_FAILURE;
	}

	return RT_STATUS_SUCCESS;
}

//
// Description:
//	Update the P2P power save setting from the configuration.
// Arguments:
//	[in] pP2PInfo - 
//		The P2P context.
// Return:
//	If success, return RT_STATUS_SUCCESS; Otherwise, return an error code.
// Assumption:
//	Before calling this function, make sure the bus interface so that we can directly access
//	the HW register, such as passive level.
// By Bruce, 2011-02-11.
//
u4Byte
P2PUpdatePowerSavePara(
	IN	PP2P_INFO				pP2PInfo
	)
{
	PADAPTER			pAdapter = pP2PInfo->pAdapter;
	u8Byte				curTsf = 0, u8StartTime = 0, nextAdvancedTsf = 0, usBeaconInterval = 0, usCtWindow = 0;
	u1Byte				i = 0;

	if(!P2P_ENABLED(pP2PInfo))
		return RT_STATUS_FAILURE;

	if(!pP2PInfo->bUpdatePsParameter)
		return RT_STATUS_FAILURE;
	
	usBeaconInterval = pP2PInfo->pAdapter->MgntInfo.dot11BeaconPeriod * sTU;

	pP2PInfo->bOppPS = pP2PInfo->P2pPsConfgSet.bOppPs;
	usCtWindow = pP2PInfo->P2pPsConfgSet.CTWindow * sTU;

	// Check if the CTWindow is not valid
	if(usCtWindow < (P2P_PS_MIN_CTWIN * sTU) || usCtWindow > usBeaconInterval)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("CTWindow is not valid (%d)\n", pP2PInfo->P2pPsConfgSet.CTWindow));
		pP2PInfo->bOppPS = FALSE;
		pP2PInfo->CTWindow = 0;
	}
	else
	{
		pP2PInfo->CTWindow = pP2PInfo->P2pPsConfgSet.CTWindow;
	}


	// We don't accept both NoA with Count = 1
	if(pP2PInfo->P2pPsConfgSet.NoASet[0].bNoAEn && pP2PInfo->P2pPsConfgSet.NoASet[1].bNoAEn &&
		pP2PInfo->P2pPsConfgSet.NoASet[0].NoACnt == 1 && pP2PInfo->P2pPsConfgSet.NoASet[1].NoACnt == 1)
	{
		RT_TRACE(COMP_P2P, DBG_WARNING, ("Don't accept bith NoA with count = 1, mark NoA1 as disable\n"));
		pP2PInfo->P2pPsConfgSet.NoASet[1].bNoAEn = FALSE;
	}

	pAdapter->HalFunc.GetHwRegHandler(pAdapter, HW_VAR_TSF_TIMER, (pu1Byte)(&curTsf));

	// Update NoA
	for(i = 0; i < P2P_MAX_NUM_NOA_DESC; i ++)
	{
		pP2PInfo->NoADescriptors[i].bValid = pP2PInfo->P2pPsConfgSet.NoASet[i].bNoAEn;
		
		// If the interval is not long enough to get into awake state, mark it as no NoA.
		if((pP2PInfo->P2pPsConfgSet.NoASet[i].NoADur < 4 * P2P_PS_TIMEOUT_TOLERANCE) ||
			((pP2PInfo->P2pPsConfgSet.NoASet[i].NoADur + 10000) > pP2PInfo->P2pPsConfgSet.NoASet[i].NoAInt && pP2PInfo->P2pPsConfgSet.NoASet[i].NoACnt != 1 && pP2PInfo->NoADescriptors[i].bValid))
		{
			pP2PInfo->NoADescriptors[i].bValid = FALSE;
			RT_TRACE(COMP_P2P, DBG_WARNING, (" Setting Values are invalid: Duration (0x%08X), Interval (0x%08X) and count = %d\n", 
					pP2PInfo->P2pPsConfgSet.NoASet[i].NoADur, pP2PInfo->P2pPsConfgSet.NoASet[i].NoAInt, pP2PInfo->P2pPsConfgSet.NoASet[i].NoACnt));
		}
		
		pP2PInfo->NoADescriptors[i].CountOrType = (pP2PInfo->NoADescriptors[i].bValid) ? pP2PInfo->P2pPsConfgSet.NoASet[i].NoACnt : 0;
		pP2PInfo->NoADescriptors[i].Duration = (pP2PInfo->NoADescriptors[i].bValid) ? pP2PInfo->P2pPsConfgSet.NoASet[i].NoADur : 0;
		pP2PInfo->NoADescriptors[i].Interval = (pP2PInfo->NoADescriptors[i].bValid) ? pP2PInfo->P2pPsConfgSet.NoASet[i].NoAInt : 0;
		if(pP2PInfo->NoADescriptors[i].bValid)
		{			
			// Modify the start time to be the mod of beacon interval.
			// If the user didn't specify the shift time, using the defualt value 500 ms.
			u8StartTime = (pP2PInfo->P2pPsConfgSet.NoASet[i].bUseStartTime) ? 
							((curTsf & UINT64_C(0xFFFFFFFF00000000)) | pP2PInfo->P2pPsConfgSet.NoASet[i].u4StartTime) : PlatformDivision64(curTsf, usBeaconInterval) * usBeaconInterval + 5 * usBeaconInterval;

			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bUseStartTime = %d, u8StartTime = 0x%08X %08X, div = 0x%08X\n", pP2PInfo->P2pPsConfgSet.NoASet[i].bUseStartTime, (u4Byte)(u8StartTime >> 32), (u4Byte)u8StartTime, (u4Byte)PlatformDivision64(curTsf, usBeaconInterval)));

			// Store the exact TSF timer for timeout reference.
			pP2PInfo->NoADescriptors[i].u8StartTime = u8StartTime;

			// Extract the low 4 bytes.			
			pP2PInfo->NoADescriptors[i].StartTime = (u4Byte)(u8StartTime & 0xFFFFFFFF);

			nextAdvancedTsf = (pP2PInfo->NoADescriptors[i].u8StartTime & UINT64_C(0xFFFFFFFF00000000)) + UINT64_C(0x0000000100000000);

			if(pP2PInfo->NoADescriptors[i].CountOrType == 0xFF)
			{ // count is 255
				pP2PInfo->NoADescriptors[i].u8EndTime = UINT64_C(0xFFFFFFFFFFFFFFFF);
			}
			else if(pP2PInfo->NoADescriptors[i].CountOrType == 1)
			{
				pP2PInfo->NoADescriptors[i].u8EndTime = (u8Byte)(pP2PInfo->NoADescriptors[i].u8StartTime + pP2PInfo->NoADescriptors[i].Duration);
			}
			else
			{
				pP2PInfo->NoADescriptors[i].u8EndTime = (u8Byte)(pP2PInfo->NoADescriptors[i].u8StartTime + 
									(pP2PInfo->NoADescriptors[i].CountOrType * pP2PInfo->NoADescriptors[i].Interval) - 
									(pP2PInfo->NoADescriptors[i].Interval - pP2PInfo->NoADescriptors[i].Duration));
			}

			if(pP2PInfo->NoADescriptors[i].u8EndTime <= curTsf + 10 * sTU) // The end time is too closed to the current TSF, skip this NoA.
			{
				pP2PInfo->NoADescriptors[i].bValid = FALSE;
			}
		}

		// Reset all variables if this NoA is invalid.
		if(!pP2PInfo->NoADescriptors[i].bValid)
		{
			PlatformZeroMemory(&pP2PInfo->NoADescriptors[i], sizeof(P2P_NOA_DESCRIPTOR));
		}
	}

	// Swap the two descriptors to let the 2nd NoA descriptor end always first.
	if(pP2PInfo->NoADescriptors[1].u8EndTime > pP2PInfo->NoADescriptors[0].u8EndTime)
	{
		P2P_NOA_DESCRIPTOR	tmpP2pNoADesc;

		PlatformMoveMemory(&tmpP2pNoADesc, &pP2PInfo->NoADescriptors[1], sizeof(P2P_NOA_DESCRIPTOR));
		PlatformMoveMemory(&pP2PInfo->NoADescriptors[1], &pP2PInfo->NoADescriptors[0], sizeof(P2P_NOA_DESCRIPTOR));
		PlatformMoveMemory(&pP2PInfo->NoADescriptors[0], &tmpP2pNoADesc, sizeof(P2P_NOA_DESCRIPTOR));
	}

	RT_TRACE(COMP_P2P, DBG_LOUD, ("bOppPs = %d, CTWin = %X\n", pP2PInfo->bOppPS, pP2PInfo->CTWindow));
	for(i = 0; i < 2; i ++)
	{
		RT_TRACE(COMP_P2P, DBG_LOUD, ("-----NoA[%d]-----\nbValid = %d, Count = %X, Duration = 0x%08X (Dec = %d), Interval = 0x%08X (Dec = %d)\nu8StartTime = 0x%08X %08X (Low Dec = %d), \nu8EndTime = 0x%08X %08X (Low Dec = %d)\n------------\n",
				i,
				pP2PInfo->NoADescriptors[i].bValid, 
				pP2PInfo->NoADescriptors[i].CountOrType,
				pP2PInfo->NoADescriptors[i].Duration,
				pP2PInfo->NoADescriptors[i].Duration,
				pP2PInfo->NoADescriptors[i].Interval,
				pP2PInfo->NoADescriptors[i].Interval,
				(u4Byte)(pP2PInfo->NoADescriptors[i].u8StartTime >> 32),
				(u4Byte)(pP2PInfo->NoADescriptors[i].u8StartTime),
				(u4Byte)(pP2PInfo->NoADescriptors[i].u8StartTime),
				(u4Byte)(pP2PInfo->NoADescriptors[i].u8EndTime >> 32),
				(u4Byte)(pP2PInfo->NoADescriptors[i].u8EndTime),
				(u4Byte)(pP2PInfo->NoADescriptors[i].u8EndTime)));
	}
	// Reset the timeout to pass this information to timeout function.
	pP2PInfo->NextTimeout = 0;

	pP2PInfo->bUpdatePsParameter = FALSE;

	if(pP2PInfo->bOppPS || pP2PInfo->NoADescriptors[0].bValid || pP2PInfo->NoADescriptors[1].bValid)
	{
		RT_SET_PS_LEVEL(pAdapter, RT_RF_LPS_P2P_PS);
	}

	return RT_STATUS_SUCCESS;
}

#define		P2P_PS_TIME_STATE_NO_PS						0		// No State
#define		P2P_PS_TIME_STATE_IN_CTWIN					BIT0	// In CTWin, we shall keep awake.
#define		P2P_PS_TIME_STATE_OUT_CTWIN					BIT1	// Out of CTWin, we shall enter PS.
#define		P2P_PS_TIME_STATE_OUT_CTWIN_CLIENT_AWAKE	BIT2	// Out of CTWin, but we must keep awake for the reason of GO with at least one acitve client or our client in active mode.
#define		P2P_PS_TIME_STATE_IN_1TIME_NOA0				BIT3	// In one time NoA in desc 0
#define		P2P_PS_TIME_STATE_IN_1TIME_NOA1				BIT4	// In one time NoA in desc 1
#define		P2P_PS_TIME_STATE_IN_NOA0					BIT5	// In periodic NoA in desc 0 of PS. (sleep)
#define		P2P_PS_TIME_STATE_IN_NOA1					BIT6	// In periodic NoA in desc 1 of PS. (sleep)
#define		P2P_PS_TIME_STATE_OUT_NOA0					BIT7	// Out of periodic NoA in desc 0 of PS. (awake)
#define		P2P_PS_TIME_STATE_OUT_NOA1					BIT8	// Out of periodic NoA in desc 1 of PS. (awake)
#define		P2P_PS_TIME_STATE_IN_TBTT					BIT9	// In TBTT time, we shall keep active to send the Beacon

//
// Description:
//	This P2P PowerSave workitem is only scheudled for usb interface because the ps procedure
//	needs to read/write the register. When the P2P ps mode is periodically occurs, it needs the
//	while loop and to sleep for simulating the timeout and triggering the event to perform on/off.
//	When the P2P ps mode is disabled, this workitem function is closed and scheduled new one
//	at next time whenever the ps mode is set again.
// Arguments:
//	[in] - pContext -
//		The P2P context address.
// Return:
//	None
// By Bruce, 2010-12-13.
//
VOID
P2PPsWorkItemCallback(
	IN	PVOID	pContext
	)
{
	PP2P_INFO			pP2PInfo = (PP2P_INFO)pContext;

	if(!pP2PInfo)
		return;

	if(!P2P_ENABLED(pP2PInfo))
		return;

	do
	{
		if(RT_CANNOT_IO(pP2PInfo->pAdapter))
			break;
		
		pP2PInfo->usSleepTime = 0;
		if(pP2PInfo->bUpdatePsParameter)
		{
			P2PUpdatePowerSavePara(pP2PInfo);
		}

		if(pP2PInfo->psExeType == RT_P2P_PS_EXE_BY_SW_TIMER)
		{ // Driver Timer
			P2PPsTimeout(pP2PInfo->pAdapter);
		}
		else if(pP2PInfo->psExeType == RT_P2P_PS_EXE_BY_HW)
		{ // HW caculate			
			P2PSetPsState(pP2PInfo, 0, 0, 0);
			break;
		}
		else
		{ // Unknown state
			break;
		}

		// We don't need work item any more.
		if(pP2PInfo->usSleepTime == 0)
			break;

		if(pP2PInfo->usSleepTime > P2P_PS_TIMEOUT_TOLERANCE)
			PlatformSleepUs((pP2PInfo->usSleepTime - P2P_PS_TIMEOUT_TOLERANCE));
	}while(TRUE);
}

//
// Description:
//	On receiving a P2P PS timeout event, review the TSF timer and check all the condition whether the state is going into.
// Arguemtns:
//	[in] pP2PInfo - 
//		The P2P context.
// Return:
//	None.
// By Bruce, 2010-03-18.
//
VOID
P2PPsTimeout(
	IN	PADAPTER			pAdapter
	)
{	
	PMGNT_INFO			pMgntInfo = &(pAdapter->MgntInfo); // should be the default mgnt info
	PP2P_INFO			pP2PInfo = pMgntInfo->pP2PInfo;
	PADAPTER			pExtAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter) ? pAdapter : GetFirstExtAdapter(pAdapter);
	BOOLEAN				bNoA0Valid = FALSE, bNoA1Valid = FALSE;
	BOOLEAN				bUpdatePsMode = FALSE;
	u8Byte				curTsf = 0, preTbtt = 0, nextTbtt = 0, timeDiff = 0;
	u8Byte				nextNoA0Start = 0, nextNoA1Start = 0, nextNoA0End = 0, nextNoA1End = 0, next1TimeNoAStart = 0;
	u8Byte				tmpNoAStartTime = 0, tmpNextDozeTime = 0;
	u4Byte				tmpNoAIntvl = 0, tmpNoADur = 0;
	u8Byte				usBeaconInterval = 0, usCTWindow = 0;
	P2P_POWERSAVE_SET	P2pPsSet;
	u1Byte				i = 0;
	u4Byte				P2pPsTimeState = P2P_PS_TIME_STATE_NO_PS;
	u4Byte				psStateToSet = P2P_PS_AWAKE;
	u4Byte				psSourceToSet = P2P_PS_CANCEL_ALL_PS;
	u8Byte				psTimeoutToSet = 0;
	

	FunctionIn(COMP_P2P);

	// The role is incorrect
	if(pP2PInfo->Role != P2P_GO && pP2PInfo->Role != P2P_CLIENT)
		return;
		

	if(P2P_CLIENT == pP2PInfo->Role && !pMgntInfo->mAssoc)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] P2P_CLIENT but not associated! Skip...\n"));
		return;
	}

	// No P2P PS mode and the power state is awake.
	if(pP2PInfo->PsFlag == P2P_PS_CANCEL_ALL_PS && pP2PInfo->P2pPsState == P2P_PS_AWAKE && pP2PInfo->CTWindow == 0 && 
		pP2PInfo->NoADescriptors[0].u8StartTime == 0 && pP2PInfo->NoADescriptors[1].u8StartTime == 0)
	{
		return;
	}

	usBeaconInterval = pP2PInfo->pAdapter->MgntInfo.dot11BeaconPeriod * sTU;
	usCTWindow = pP2PInfo->CTWindow * sTU;

	if(usBeaconInterval < P2P_PS_TIMEOUT_TOLERANCE)
	{
		//RT_TRACE(COMP_P2P, DBG_WARNING, ("Error: usBeaconInterval(%"i64fmt"d) < %d\n", usBeaconInterval, P2P_PS_TIMEOUT_TOLERANCE));
		return;
	}

	// If the interface is usb, we cannot do the p2p process directly, but schedule the workitem to
	// do it instead.
	// By Bruce, 2010-12-13.

		if(!PlatformIsWorkItemScheduled(&(pP2PInfo->P2PPSWorkItem)))
		{
			PlatformScheduleWorkItem(&(pP2PInfo->P2PPSWorkItem));
			return;
		}
		// The workitem is scheduled, just wait the workitem to do it.
		if(pP2PInfo->usSleepTime != 0)
			return;

	//
	// Cleint is in the awake state, the CTWindow doen't apply now.
	// if(pP2PInfo->Role == P2P_CLIENT && 
	//	(pP2PInfo->pAdapter->MgntInfo.dot11PowerSaveMode == eActive))
	//{
	//		usCTWindow = 0;
	//}

	pP2PInfo->pAdapter->HalFunc.GetHwRegHandler(pP2PInfo->pAdapter, HW_VAR_TSF_TIMER, (pu1Byte)(&curTsf));

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("TSF = 0x%08X %08X (Low Dec = %d)\n", (u4Byte)(curTsf >> 32), (u4Byte)curTsf, (u4Byte)curTsf));

	if(pP2PInfo->NextTimeout != 0 && (curTsf + P2P_PS_TIMEOUT_TOLERANCE) < pP2PInfo->NextTimeout)
	{
		// Ahead of time
		psStateToSet = pP2PInfo->P2pPsState;
		psSourceToSet = pP2PInfo->PsFlag;
		psTimeoutToSet = pP2PInfo->NextTimeout;
		goto setPs;
	}

	// Evaluate the current TBTT and next TBTT.
	if(PlatformModular64(curTsf, usBeaconInterval) > (usBeaconInterval - P2P_PS_TIMEOUT_TOLERANCE))
	{
		preTbtt = (PlatformDivision64(curTsf, usBeaconInterval) + 1) * usBeaconInterval;
		nextTbtt = (PlatformDivision64(curTsf, usBeaconInterval) + 2) * usBeaconInterval;
	}
	else
	{
		preTbtt = PlatformDivision64(curTsf, usBeaconInterval) * usBeaconInterval;
		nextTbtt = (PlatformDivision64(curTsf, usBeaconInterval) + 1) * usBeaconInterval;
	}

	PlatformZeroMemory(&P2pPsSet, sizeof(P2P_POWERSAVE_SET));

	P2pPsSet.bOppPs = pP2PInfo->bOppPS;
	P2pPsSet.CTWindow = pP2PInfo->CTWindow;

	// We shall verify if the current NoA is valid now.
	for(i = 0; i < P2P_MAX_NUM_NOA_DESC; i ++)
	{
		P2pPsSet.NoASet[i].bNoAEn = pP2PInfo->NoADescriptors[i].bValid;
		P2pPsSet.NoASet[i].NoADur = pP2PInfo->NoADescriptors[i].Duration;
		P2pPsSet.NoASet[i].NoAInt = pP2PInfo->NoADescriptors[i].Interval;
		P2pPsSet.NoASet[i].NoACnt = pP2PInfo->NoADescriptors[i].CountOrType;
		P2pPsSet.NoASet[i].bUseStartTime = TRUE;
		P2pPsSet.NoASet[i].u4StartTime = pP2PInfo->NoADescriptors[i].StartTime;
		
		if(pP2PInfo->NoADescriptors[i].bValid)
		{
			// Time 's up
			if(curTsf >= pP2PInfo->NoADescriptors[i].u8EndTime - P2P_PS_TIMEOUT_TOLERANCE)
			{
				P2pPsSet.NoASet[i].bNoAEn = FALSE;
				bUpdatePsMode = TRUE;
			}

			// The upper 4 TSF timer changed.
			if((curTsf & UINT64_C(0xFFFFFFFF00000000)) != (pP2PInfo->NoADescriptors[i].u8StartTime & UINT64_C(0xFFFFFFFF00000000)))
			{
				if(pP2PInfo->NoADescriptors[i].CountOrType != 0xFF)
				{ // In this case, disable this NoA no matter the end of NoA reaches.
					P2pPsSet.NoASet[i].bNoAEn = FALSE;
				}
				else
				{ // It is continuous NoA, the start time can be set a new one.
					P2pPsSet.NoASet[i].u4StartTime = 0;
				}

				bUpdatePsMode = TRUE;
			}
		}
	}

	if(bUpdatePsMode)
	{
		P2PSetPowerSaveMode(pP2PInfo, &P2pPsSet, pP2PInfo->NoAIEIndex, FALSE);
		return;
	}

	//3 //==============Evaluate the Current Time State===============//
	//4 // (1) NoA count == 1
	if(pP2PInfo->NoADescriptors[0].bValid && 
		(pP2PInfo->NoADescriptors[0].u8StartTime <= (curTsf + P2P_PS_TIMEOUT_TOLERANCE)))
	{
		bNoA0Valid = TRUE;
	}

	if(pP2PInfo->NoADescriptors[1].bValid && 
		(pP2PInfo->NoADescriptors[1].u8StartTime <= (curTsf + P2P_PS_TIMEOUT_TOLERANCE)))
	{
		bNoA1Valid = TRUE;
	}

	// Note:
	//	We assum that no two 1 Time NoA can run concurrently.
	// 1 Time NoA
	if(pP2PInfo->NoADescriptors[0].CountOrType == 1)
	{
		if(bNoA0Valid)
		{ // 1Time NoA had Started
			P2pPsTimeState |= P2P_PS_TIME_STATE_IN_1TIME_NOA0;
		}
		else if(P2pPsSet.NoASet[0].bNoAEn)
		{
			next1TimeNoAStart = pP2PInfo->NoADescriptors[0].u8StartTime;
		}
	}
	else if(pP2PInfo->NoADescriptors[1].CountOrType == 1)
	{
		if(bNoA0Valid)
		{ // 1Time NoA had Started
			P2pPsTimeState |= P2P_PS_TIME_STATE_IN_1TIME_NOA1;
		}
		else if(P2pPsSet.NoASet[1].bNoAEn)
		{
			next1TimeNoAStart = pP2PInfo->NoADescriptors[1].u8StartTime;
		}
	}

	//4 // (2) TBTT
	// Under this case, the NIC just sends the Beacon Frame only, but no data frames.
	if((PlatformModular64(curTsf, usBeaconInterval) < P2P_PS_TIMEOUT_TOLERANCE) ||
		(PlatformModular64(curTsf, usBeaconInterval) > usBeaconInterval - P2P_PS_TIMEOUT_TOLERANCE))
	{
		P2pPsTimeState |= P2P_PS_TIME_STATE_IN_TBTT;
	}

	//4 // (3) CTWindow
	if(usCTWindow > 0)
	{ // Caculate if we are out of CTWindow
		if((PlatformModular64(curTsf, usBeaconInterval) < (usCTWindow - P2P_PS_TIMEOUT_TOLERANCE)) ||
			(PlatformModular64(curTsf, usBeaconInterval) > (usBeaconInterval - P2P_PS_TIMEOUT_TOLERANCE)))
		{
			P2pPsTimeState |= P2P_PS_TIME_STATE_IN_CTWIN;
		}		
		else
		{
			// We are out of CTWin
			P2pPsTimeState |= P2P_PS_TIME_STATE_OUT_CTWIN;

			// <Note> We need to check if any client is in active mode (including ourself if we are client mode).			
			if(pP2PInfo->Role == P2P_GO)
			{ // Check if the GO shall keep awake.
				PRT_WLAN_STA	pEntry = NULL;

				// Find out if any station is in active mode;
				for(i = 0; i < ASSOCIATE_ENTRY_NUM; i ++)
				{
					pEntry = AsocEntry_EnumStation(pExtAdapter, i);

					if(!pEntry)
						continue;

					if(!pEntry->bPowerSave)
					{ // There is one client in active mode, don't enter PS.
						P2pPsTimeState |= P2P_PS_TIME_STATE_OUT_CTWIN_CLIENT_AWAKE;
						break;
					}
				}
			}
			else if(pP2PInfo->Role == P2P_CLIENT && 
				(pP2PInfo->pAdapter->MgntInfo.dot11PowerSaveMode == eActive))
			{ // Cleint is in the awake state, the CTWindow doen't apply now.
				// <Note> If there exists periodic NoA, we shall follow the rule of NoA.
				P2pPsTimeState |= P2P_PS_TIME_STATE_OUT_CTWIN_CLIENT_AWAKE;
			}
		}
	}

	//4 // (4) NoA Count > 1
	if(bNoA0Valid && (pP2PInfo->NoADescriptors[0].CountOrType > 1))
	{
		tmpNoAStartTime = pP2PInfo->NoADescriptors[0].u8StartTime;
		tmpNoAIntvl = pP2PInfo->NoADescriptors[0].Interval;
		tmpNoADur = pP2PInfo->NoADescriptors[0].Duration;
		
		timeDiff = (curTsf < tmpNoAStartTime) ? 0: (curTsf - tmpNoAStartTime);
			
		if(PlatformModular64(timeDiff, tmpNoAIntvl) < (tmpNoADur - P2P_PS_TIMEOUT_TOLERANCE))
		{
			P2pPsTimeState |= P2P_PS_TIME_STATE_IN_NOA0;
			nextNoA0End = tmpNoAStartTime + (PlatformDivision64(timeDiff, tmpNoAIntvl) * tmpNoAIntvl + tmpNoADur);
		}
		else if(PlatformModular64(timeDiff, tmpNoAIntvl) > (tmpNoAIntvl - P2P_PS_TIMEOUT_TOLERANCE))
		{
			P2pPsTimeState |= P2P_PS_TIME_STATE_IN_NOA0;
			nextNoA0End = tmpNoAStartTime + ((PlatformDivision64(timeDiff, tmpNoAIntvl) + 1) * tmpNoAIntvl + tmpNoADur);
		}
		else
		{
			P2pPsTimeState |= P2P_PS_TIME_STATE_OUT_NOA0;
			nextNoA0Start = tmpNoAStartTime + ((PlatformDivision64(timeDiff, tmpNoAIntvl) + 1) * tmpNoAIntvl);
		}
	}
	if(bNoA1Valid && (pP2PInfo->NoADescriptors[1].CountOrType > 1))
	{
		tmpNoAStartTime = pP2PInfo->NoADescriptors[1].u8StartTime;
		tmpNoAIntvl = pP2PInfo->NoADescriptors[1].Interval;
		tmpNoADur = pP2PInfo->NoADescriptors[1].Duration;
		
		timeDiff = (curTsf < tmpNoAStartTime) ? 0: (curTsf - tmpNoAStartTime);
			
		if(PlatformModular64(timeDiff, tmpNoAIntvl) < (tmpNoADur - P2P_PS_TIMEOUT_TOLERANCE))
		{
			P2pPsTimeState |= P2P_PS_TIME_STATE_IN_NOA1;
			nextNoA1End = tmpNoAStartTime + (PlatformDivision64(timeDiff, tmpNoAIntvl) * tmpNoAIntvl + tmpNoADur);
		}
		else if(PlatformModular64(timeDiff, tmpNoAIntvl) > (tmpNoAIntvl - P2P_PS_TIMEOUT_TOLERANCE))
		{
			P2pPsTimeState |= P2P_PS_TIME_STATE_IN_NOA1;
			nextNoA1End = tmpNoAStartTime + ((PlatformDivision64(timeDiff, tmpNoAIntvl) + 1) * tmpNoAIntvl + tmpNoADur);
		}
		else
		{
			P2pPsTimeState |= P2P_PS_TIME_STATE_OUT_NOA1;
			nextNoA1Start = tmpNoAStartTime + ((PlatformDivision64(timeDiff, tmpNoAIntvl) + 1) * tmpNoAIntvl);
		}
	}

	//3 //==============Evaluate the Ps State and Timeout value===============//

	//4 // 1-Time NoA
	if(P2pPsTimeState & P2P_PS_TIME_STATE_IN_1TIME_NOA0)
	{
		psTimeoutToSet = pP2PInfo->NoADescriptors[0].u8EndTime;
		psSourceToSet = P2P_PS_ENTER_PS_BY_1TIME_NOA;
		psStateToSet = P2P_PS_DOZE_SEND_MGNT;
		goto setPs;
	}

	if(P2pPsTimeState & P2P_PS_TIME_STATE_IN_1TIME_NOA1)
	{
		psTimeoutToSet = pP2PInfo->NoADescriptors[1].u8EndTime;
		psSourceToSet = P2P_PS_ENTER_PS_BY_1TIME_NOA;
		psStateToSet = P2P_PS_DOZE_SEND_MGNT;
		goto setPs;
	}

	//4 // CTWindow.
	if(P2pPsTimeState & P2P_PS_TIME_STATE_IN_CTWIN)
	{
		tmpNextDozeTime = preTbtt + usCTWindow;

		// Next 1 Time NoA is earlier than the End of CTWin.
		if(next1TimeNoAStart > 0 && next1TimeNoAStart < tmpNextDozeTime)
			tmpNextDozeTime = next1TimeNoAStart;

		psTimeoutToSet = tmpNextDozeTime;
		psSourceToSet = P2P_PS_LEAVE_PS_BY_CTWIN_TBTT;
		psStateToSet = P2P_PS_AWAKE;
		goto setPs;
	}
	else if(P2pPsTimeState & P2P_PS_TIME_STATE_OUT_CTWIN)
	{
		if(!(P2pPsTimeState & P2P_PS_TIME_STATE_OUT_CTWIN_CLIENT_AWAKE))
		{ // Out of CTWin (the priority is higher than periodic NoA), Next TBTT must wake up.				
			psTimeoutToSet = nextTbtt;
			psSourceToSet = P2P_PS_ENTER_PS_BY_CTWIN;
			psStateToSet = P2P_PS_DOZE;
			goto setPs;
		}
	}

	//4 // Periodic NoA
	if(P2pPsTimeState & (P2P_PS_TIME_STATE_IN_NOA0 | P2P_PS_TIME_STATE_IN_NOA1))
	{
		if(P2pPsTimeState & P2P_PS_TIME_STATE_IN_TBTT)
		{ // After sending Beacon (Mgnt frame), go back dozed.
			if(PlatformModular64(curTsf, usBeaconInterval) < P2P_PS_TIMEOUT_TOLERANCE)
			{
				tmpNextDozeTime = (PlatformDivision64(curTsf, usBeaconInterval) * usBeaconInterval + 5000);
			}
			else
			{
				tmpNextDozeTime = ((PlatformDivision64(curTsf, usBeaconInterval) + 1) * usBeaconInterval + 5000);
			}
			psTimeoutToSet = tmpNextDozeTime;
			psSourceToSet = P2P_PS_LEAVE_PS_BY_TBTT;
			psStateToSet = P2P_PS_DOZE_SEND_MGNT;
			goto setPs;
		}
		else
		{ // Enter dozed state and caculate the next awake time.
			u8Byte	tmpNextAwakeTime = nextNoA0End;

			// If the NoA1 and NoA2 are overlapped, caculate the latest end time of NoA.
			if(tmpNextAwakeTime < nextNoA1End)
				tmpNextAwakeTime = nextNoA1End;

			// Do not cross the TBTT
			if(tmpNextAwakeTime > nextTbtt)
				tmpNextAwakeTime = nextTbtt;

			psTimeoutToSet = tmpNextAwakeTime;
			psSourceToSet = P2P_PS_ENTER_PS_BY_PERIODIC_NOA;
			psStateToSet = P2P_PS_DOZE;
			goto setPs;
		}
	}
	else if(P2pPsTimeState & (P2P_PS_TIME_STATE_OUT_NOA0 | P2P_PS_TIME_STATE_OUT_NOA1))
	{ // Enter Awake state and caculate the next doze time.
		if(pP2PInfo->Role == P2P_CLIENT)
		{
			// Retrigger if the previous SP is ended by the NoA.
			if(!IN_LEGACY_POWER_SAVE(pMgntInfo->pStaQos) && pP2PInfo->bEospByNoA)
			{
				u1Byte		AC = 0;
				if(GET_VO_UAPSD(pMgntInfo->pStaQos->Curr4acUapsd))
					AC = 7;
				else if(GET_VI_UAPSD(pMgntInfo->pStaQos->Curr4acUapsd))
					AC = 5;
				else if(GET_BE_UAPSD(pMgntInfo->pStaQos->Curr4acUapsd))
					AC = 3;
				else if(GET_BK_UAPSD(pMgntInfo->pStaQos->Curr4acUapsd))
					AC = 2;
					
				SendQoSNullFunctionData(pP2PInfo->pAdapter, pMgntInfo->Bssid, AC, TRUE);
				pMgntInfo->pStaQos->bInServicePeriod = TRUE;
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PPSTimeout(): Re-trigger the SP!\n"));
			}
		}
		pP2PInfo->bEospByNoA = FALSE;
		
		tmpNextDozeTime = nextNoA0Start;
		// Find the earliet NoA start time
		if(nextNoA1Start > 0 && tmpNextDozeTime > nextNoA1Start)
			tmpNextDozeTime = nextNoA1Start;

		// If the next doze time cross TBTT, set the timeout to TBTT
		if(tmpNextDozeTime > 0 && tmpNextDozeTime > nextTbtt)
			tmpNextDozeTime = nextTbtt;

		psTimeoutToSet = tmpNextDozeTime;
		psSourceToSet = P2P_PS_LEAVE_PS_BY_NOA;
		psStateToSet = P2P_PS_AWAKE;
		goto setPs;
	}
	else
	{ // No one Periodic NoA is valid, and this state shall be kept awake until next CTWin comes, or all clients enter dozed, or one NoA becomes valid.
		// If next CTWin exists, set the next doze time as next CTWin because we will keep awake cross the next TBTT until the end of CTWin.
		tmpNextDozeTime = (usCTWindow > 0 && pP2PInfo->bOppPS) ? (nextTbtt + usCTWindow) : 0;
		
		// Next NoA0 start time.
		if(pP2PInfo->NoADescriptors[0].bValid && pP2PInfo->NoADescriptors[0].u8StartTime > curTsf)
		{
			if(tmpNextDozeTime == 0 || pP2PInfo->NoADescriptors[0].u8StartTime < tmpNextDozeTime)
				tmpNextDozeTime = pP2PInfo->NoADescriptors[0].u8StartTime;
		}

		// Next NoA1 start time comes earlier than NoA1
		if(pP2PInfo->NoADescriptors[1].bValid && pP2PInfo->NoADescriptors[1].u8StartTime > curTsf)
		{
			if(tmpNextDozeTime == 0 || pP2PInfo->NoADescriptors[1].u8StartTime < tmpNextDozeTime)
				tmpNextDozeTime = pP2PInfo->NoADescriptors[1].u8StartTime;
		}

		if(P2pPsTimeState & P2P_PS_TIME_STATE_OUT_CTWIN_CLIENT_AWAKE)
		{
			psTimeoutToSet = tmpNextDozeTime;
			psSourceToSet = P2P_PS_LEAVE_PS_BY_OUT_CTWIN_ClIENT_AWAKE;
			psStateToSet = P2P_PS_AWAKE;
			goto setPs;
		}
		else
		{
			psTimeoutToSet = tmpNextDozeTime;
			psSourceToSet = P2P_PS_CANCEL_ALL_PS;
			psStateToSet = P2P_PS_AWAKE;
			goto setPs;
		}
	}

setPs:
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2pPsTimeState = 0x%X, TimeDiff = %d\n", P2pPsTimeState, (u4Byte)(psTimeoutToSet - curTsf)));
	pP2PInfo->usSleepTime = (u4Byte)(psTimeoutToSet - curTsf);
	if(psTimeoutToSet < (curTsf + P2P_PS_TIMEOUT_TOLERANCE) && psTimeoutToSet != 0)
	{
		RT_TRACE(COMP_P2P, DBG_WARNING, ("P2PPsTimeout(): psTimeoutToSet = Timeout is less than TSF\n"));
	}

	// Temp solution for P2P Test Plan 6.1.13 and 7.1.5. By Bruce, 2010-06-30.
	if(P2pPsTimeState & (P2P_PS_TIME_STATE_IN_NOA0 | P2P_PS_TIME_STATE_IN_NOA1) || psSourceToSet & P2P_PS_ENTER_PS_BY_PERIODIC_NOA)
	{
		// Check the WMM-PS SPs are ended by the NoAs.
		// Temp solution for P2P Test Plan 6.1.13 and 7.1.5. By Bruce, 2010-06-30.
		if(pP2PInfo->Role == P2P_GO)
		{
			PRT_WLAN_STA	pEntry = NULL;

			// Find out if any station in active mode;
			// If any client of P2P client in WMM-PS mode, end its SP.
			if(P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter))
			{
				while(pAdapter  != NULL)
				{
					if(pAdapter->pNdis62Common->PortType == EXT_P2P_ROLE_PORT && pP2PInfo->Role == P2P_GO)
						break;
					pExtAdapter = GetNextExtAdapter(pAdapter);
				}
				if(pExtAdapter == NULL)
					return;
			}

			for(i = 0; i < ASSOCIATE_ENTRY_NUM; i ++)
			{
				pEntry = AsocEntry_EnumStation(pExtAdapter, i);
				if(!pEntry)
					break;

				if(pEntry->bPowerSave && ((pEntry->WmmStaQosInfo & 0xF) != 0))
				{
					pEntry->WmmEosp = RT_STA_EOSP_STATE_ENDED;
					RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "P2PPsTimeout(): End SP for the client: ", pEntry->MacAddr);
					// if(RTIsListNotEmpty(&(pEntry->WmmPsQueue)))
					// {
					// 	RT_TRACE(COMP_P2P, DBG_LOUD, ("Some WMM packets for this client, need trigger frame!\n"));
					// }
				}
			}
		}
		else if(pP2PInfo->Role == P2P_CLIENT)
		{
			// End the SP, and mark the SP is ended by the NoA, and we shall re-trigger if the NoA is in presence period.
			if(!IN_LEGACY_POWER_SAVE(pMgntInfo->pStaQos) && IS_WIFI_POWER_SAVE(pMgntInfo) && pMgntInfo->pStaQos->bInServicePeriod && pMgntInfo->pStaQos->bWmmMoreData)
			{
				pMgntInfo->pStaQos->bInServicePeriod = FALSE;
				pP2PInfo->bEospByNoA = TRUE;
				RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PPsTimeout(): Cleint SP is ended by the NoA\n"));
			}
		}
	}
		
	P2PSetPsState(pP2PInfo, psSourceToSet, (P2P_PS_STATE)psStateToSet, psTimeoutToSet);
	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PPsTimeout(): <=====\n"));
	return;
}

VOID
P2PInitializeTimer(
	PADAPTER pAdapter
)
{

	PMGNT_INFO				pMgntInfo = &pAdapter->MgntInfo;
	PP2P_INFO				pP2PInfo = (PP2P_INFO)pMgntInfo->pP2PInfo;

	PlatformInitializeTimer(pAdapter, &(pP2PInfo->P2PMgntTimer), (RT_TIMER_CALL_BACK)P2PMgntTimerCallback, NULL, "P2PMgntTimer");
	PlatformInitializeTimer(pAdapter, &(pP2PInfo->ClientJoinGroupContext.P2PWaitForWpsReadyTimer), (RT_TIMER_CALL_BACK)P2PWaitForWpsReadyTimerCallback, NULL, "P2PWaitForWpsReadyTimer");
	PlatformInitializeTimer(pAdapter, &(pP2PInfo->P2POidPostProcessTimer), (RT_TIMER_CALL_BACK)P2POidPostProcessTimerCallback, NULL, "P2POidPostProcessTimer");	

	pP2PInfo->P2PActionTimer = ActionTimerAllocate(
						pAdapter, 
						"P2PActionTimer",
						HW_TSF_CLOCK,
						HW_TSF_CLOCK_PS_TIMER
				);
}

VOID
P2PCancelTimer(
	PADAPTER pAdapter
)
{

	PMGNT_INFO				pMgntInfo = &pAdapter->MgntInfo;
	PP2P_INFO				pP2PInfo = (PP2P_INFO)pMgntInfo->pP2PInfo;
	
	PlatformCancelTimer(pAdapter, &(pP2PInfo->P2PMgntTimer));	
	PlatformCancelTimer(pAdapter, &(pP2PInfo->ClientJoinGroupContext.P2PWaitForWpsReadyTimer));
	PlatformCancelTimer(pAdapter, &(pP2PInfo->P2POidPostProcessTimer));		
}

VOID
P2PReleaseTimer(
	PADAPTER pAdapter
)
{

	PMGNT_INFO				pMgntInfo = &pAdapter->MgntInfo;
	PP2P_INFO				pP2PInfo = (PP2P_INFO)pMgntInfo->pP2PInfo;
	
	PlatformReleaseTimer(pAdapter, &(pP2PInfo->P2PMgntTimer));
	PlatformReleaseTimer(pAdapter, &(pP2PInfo->ClientJoinGroupContext.P2PWaitForWpsReadyTimer));
	PlatformReleaseTimer(pAdapter, &(pP2PInfo->P2POidPostProcessTimer));

	ActionTimerRelease(pAdapter, pP2PInfo->P2PActionTimer);
}

static VOID
P2PActionTimerCallback(
	const ACTION_TIMER_ITEM * const	pOneShotActionItem
)
{
	PADAPTER pAdapter = (PADAPTER) pOneShotActionItem->pContext;
	
	P2PPsTimeout(pAdapter);
}

// 
// Description:
//	Save the P2P PS state and time value and configure the HW setting.
// Arguments:
//	[in] pP2PInfo -
//		The P2P context.
//	[in] Source -
//		Identify the reason why the PS state is going.
//	[in] P2pState -
//		The P2P power save state to set.
//	[in] Timeout -
//		The next timeout value.
// Return:
//	None.
//
VOID
P2PSetPsState(
	IN	PP2P_INFO		pP2PInfo,
	IN	u4Byte			Source,
	IN	P2P_PS_STATE	P2pState,
	IN	u8Byte			Timeout	
	)
{
	PADAPTER			pAdapter = NULL;
	ACTION_TIMER_ITEM	ActionItem;
	PMGNT_INFO			pMgntInfo = NULL;
	
	if(!P2P_ENABLED(pP2PInfo))
		return;

	pAdapter = pP2PInfo->pAdapter;
	pMgntInfo = &pAdapter->MgntInfo;

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Source = 0x%X, P2pState = 0x%X, Timeout = 0x%08X %08X (Low Dec = %d)\n", Source, P2pState, (u4Byte)(Timeout >> 32), (u4Byte)Timeout, (u4Byte)Timeout));
	
	pP2PInfo->P2pPsState = P2pState;
	pP2PInfo->PsFlag = Source;
	pP2PInfo->NextTimeout = Timeout;

	if(pMgntInfo->bWiFiConfg)
	{
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_P2P_PS_MODE, (pu1Byte)pP2PInfo);
	}
	else
	{
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_FCS_NOA, (pu1Byte)pP2PInfo);
	}

	// Set up the HW timer
	if(RT_P2P_PS_EXE_BY_SW_HW_TIMER == pP2PInfo->psExeType)
	{
		// + Flush the previous Action Item
		ActionTimerFlushActionItem(pAdapter, pP2PInfo->P2PActionTimer, ACTION_TYPE_P2P_POWERSAVE);
		
		PlatformZeroMemory(&ActionItem, sizeof(ActionItem));

		ActionItem.ActionType = ACTION_TYPE_P2P_POWERSAVE;
		ActionItem.CallbackFunc = P2PActionTimerCallback;
		ActionItem.pContext = pP2PInfo->pAdapter;
		ActionItem.usTimeout = pP2PInfo->NextTimeout;
	
		ActionTimerRegisterActionItem(pAdapter, &ActionItem, pP2PInfo->P2PActionTimer);
	}
	
}

//
// Description:
//	When the TSD BIT32 toggled (which means this bit changed), call this function to reset the
//	P2P NoA power save parameter.
// Arguments:
//	[in] pP2PInfo -
//		The P2P context.
// Return:
//	None.
// By Bruce, 2011-03-03.
//
VOID
P2PPsTsf_Bit32_Toggle(
	IN	PP2P_INFO		pP2PInfo
	)
{
	P2P_POWERSAVE_SET	P2pPsSet;
	u1Byte				i = 0;
	BOOLEAN				bUpdatePsMode = FALSE;

	if(!P2P_ENABLED(pP2PInfo))
		return;
	
	if((P2P_GO != pP2PInfo->Role) && (P2P_CLIENT != pP2PInfo->Role))
		return;

	PlatformZeroMemory(&P2pPsSet, sizeof(P2P_POWERSAVE_SET));

	P2pPsSet.bOppPs = pP2PInfo->bOppPS;
	P2pPsSet.CTWindow = pP2PInfo->CTWindow;

	// We shall verify if the current NoA is valid now.
	for(i = 0; i < P2P_MAX_NUM_NOA_DESC; i ++)
	{
		if(P2P_GO == pP2PInfo->Role)
		{
			// Re-enable the NoA.
			// If the role is client, no need to set noa parameters.
			P2pPsSet.NoASet[i].bNoAEn = pP2PInfo->NoADescriptors[i].bValid;
			P2pPsSet.NoASet[i].NoADur = pP2PInfo->NoADescriptors[i].Duration;
			P2pPsSet.NoASet[i].NoAInt = pP2PInfo->NoADescriptors[i].Interval;
			P2pPsSet.NoASet[i].NoACnt = pP2PInfo->NoADescriptors[i].CountOrType;

			if(MultiChannel_IsFCSInProgress(pP2PInfo->pAdapter))
			{
				// Re-calculate NoAStartTime for FCS
				u1Byte		NoAStart = 0;
				u8Byte		uCurrentTsf = 0;
				u8Byte		uNoAStartTsf = 0;
				PMGNT_INFO	pMgntInfo = &(pP2PInfo->pAdapter->MgntInfo);

				pP2PInfo->pAdapter->HalFunc.GetHwRegHandler(pP2PInfo->pAdapter, HW_VAR_TSF_TIMER, (pu1Byte)(&uCurrentTsf));
				MultiChannelGetFcsCommonInfo(pP2PInfo->pAdapter, MULTICHANNEL_FCS_COMMON_NOA_STARTTIME, &NoAStart);
				uNoAStartTsf = uCurrentTsf -  PlatformModular64(uCurrentTsf, pMgntInfo->dot11BeaconPeriod*sTU);
				uNoAStartTsf += NoAStart*sTU;
				uNoAStartTsf += (pMgntInfo->dot11BeaconPeriod)*sTU;	// next beacon
				RT_TRACE(COMP_MULTICHANNEL, DBG_LOUD, ("Start NoA TSF 0x%08X %08X modular %d\n", (u4Byte)(uNoAStartTsf>>32), (u4Byte)uNoAStartTsf, (u4Byte)PlatformModular64(uNoAStartTsf, pMgntInfo->dot11BeaconPeriod*sTU)));

				P2pPsSet.NoASet[i].bUseStartTime = TRUE;
				P2pPsSet.NoASet[i].u4StartTime = (u4Byte)uNoAStartTsf & 0xffffffff;
			}
			else
			{
				P2pPsSet.NoASet[i].bUseStartTime = FALSE;
				P2pPsSet.NoASet[i].u4StartTime = pP2PInfo->NoADescriptors[i].StartTime;
			}
		}
		
		if(pP2PInfo->NoADescriptors[i].bValid)
		{
			bUpdatePsMode = TRUE;
		}
	}

	if(bUpdatePsMode)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Reset the P2P PS for NoA\n"));
		if(MultiChannel_IsFCSInProgress(pP2PInfo->pAdapter))
		{
			P2PSetPowerSaveMode(pP2PInfo, &P2pPsSet, 0, 0);
			pP2PInfo->pAdapter->HalFunc.SetHwRegHandler(pP2PInfo->pAdapter, HW_VAR_FCS_NOA, (pu1Byte)pP2PInfo);
		}
		else
		{
			P2PSetPowerSaveMode(pP2PInfo, &P2pPsSet, pP2PInfo->NoAIEIndex, FALSE);
		}
	}
	return;
}

//
// Description:
//	Notify the client has changed its power save state and we shall update it so that the
//	P2P GO can enter PS or leave PS for CTWin.
VOID
P2PNotifyClientPSChange(
	IN	PP2P_INFO		pP2PInfo
	)
{
	PADAPTER			pAdapter = NULL, pExtAdapter = NULL;
	u1Byte				i = 0;
	PRT_WLAN_STA		pEntry = NULL;
	BOOLEAN				bAllClientSleep = TRUE;

	if(!P2P_ENABLED(pP2PInfo))
		return;

	pAdapter = pP2PInfo->pAdapter;

	pExtAdapter = P2P_ADAPTER_OS_SUPPORT_P2P(pAdapter) ? pP2PInfo->pAdapter : GetFirstExtAdapter(pP2PInfo->pAdapter);

	
	if(pP2PInfo->Role != P2P_GO)
		return;

	if(!pP2PInfo->bOppPS)
		return;

	if(pP2PInfo->psExeType == RT_P2P_PS_EXE_BY_HW)
	{
		// Find out if any station is in active mode;
		for(i = 0; i < ASSOCIATE_ENTRY_NUM; i ++)
		{
			pEntry = AsocEntry_EnumStation(pExtAdapter, i);

			if(!pEntry)
				continue;

			if(!pEntry->bAssociated)
				continue;

			if(!pEntry->bPowerSave)
			{ // There is one client in active mode, don't enter PS.
				bAllClientSleep = FALSE;
				break;
			}
		}
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_P2P_CLIENT_PS_STATE, (pu1Byte)&bAllClientSleep);
	}
}


//
// Description:
//	The P2P watch dog for monitoring the status on P2P.
// Arguments:
//	[in] pP2PInfo -
//		The P2P context.
// Return:
//	None.
// By Bruce, 2010-03-20.
//
VOID
P2POnLinkStatusWatchdog(
	IN		PADAPTER		Adapter
	)
{
	PP2P_INFO		pP2PInfo = GET_P2P_INFO(Adapter);
	u8Byte 			CurrentTime = 0;
	BOOLEAN			bFirstDevicePortEnabled = FALSE;
	PADAPTER		pDevAadpter = NULL;


	if(!pP2PInfo)
		return;
	else if(!P2P_ENABLED(pP2PInfo))
		return;

	P2PSvc_OnWatchdog(pP2PInfo->pP2PSvcInfo);
	pDevAadpter = GetFirstDevicePort(Adapter);
	if(!pDevAadpter)
		return;
	else
	{
		PP2P_INFO	pP2PDevInfo = GET_P2P_INFO(pDevAadpter);
		bFirstDevicePortEnabled = (pP2PDevInfo->uListenStateDiscoverability == 0)?FALSE:TRUE;
	}
		
	if(P2P_ADAPTER_OS_SUPPORT_P2P(Adapter) && GetFirstGOPort(Adapter) == NULL && GetFirstClientPort(Adapter) == NULL && bFirstDevicePortEnabled == FALSE)
	{
		return;
	}
	
	if(pP2PInfo->Role == P2P_GO && pP2PInfo->pAdapter->MgntInfo.mAssoc)
	{
		//if(pP2PInfo->ManagedUpdateCnt == 0)
		{ // Update the P2P managed info
			PRT_WLAN_BSS	pBssDesc = NULL;
			
			PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_SCAN_SPINLOCK);
			pBssDesc = BssDescDupByBssid(pP2PInfo->pAdapter, pP2PInfo->pAdapter->MgntInfo.Bssid);

			if(pBssDesc)
			{
				if(pBssDesc->P2PManagedInfo & maP2PDeviceManagement)
					P2PUpdateWlanApManagedInfo(pP2PInfo, pBssDesc);
			}			

			PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_SCAN_SPINLOCK);
			pP2PInfo->ManagedUpdateCnt = P2P_UPDATE_MANAGED_INFO_PERIOD;
		}
		//else
		//	pP2PInfo->ManagedUpdateCnt --;
	}

	CurrentTime = PlatformGetCurrentTime();
	
	if(CurrentTime - pP2PInfo->LastTimerFired > 2000000) // 2sec
	{// the timer callback has not been executed for more than 2 sec
		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2POnLinkStatusWatchdog(): re-fire timer\n"));
		PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);
	}

	if(pP2PInfo->bOppPS || pP2PInfo->NoADescriptors[0].bValid || pP2PInfo->NoADescriptors[1].bValid)
	{
		if(RT_P2P_PS_EXE_BY_SW_HW_TIMER == pP2PInfo->psExeType)
			P2PPsTimeout(Adapter);
	}
}

//
// Description:
//	Issue a client's presence request to the AP for the specified NoA.
// Arguments:
//	[in] pP2PInfo - 
//		The P2P context.
//	[in] pP2pPs -
//		The address of P2P power save setting.
// Return:
//	If success, return RT_STATUS_SUCCESS; Otherwise, return an error code.
// By Bruce, 2010-05-06.
//
u4Byte
P2PPresenceReq(
	IN	PP2P_INFO			pP2PInfo,
	IN	PP2P_POWERSAVE_SET	pP2pPs
	)
{
	if(pP2PInfo == NULL || pP2pPs == NULL)
		return RT_STATUS_FAILURE;

	// Only sent by p2p client.
	if(pP2PInfo->Role != P2P_CLIENT)
		return RT_STATUS_FAILURE;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PPresenceReq()=====>\n"));
	
	p2p_Send_PresenceReq(pP2PInfo, pP2pPs, pP2PInfo->pAdapter->MgntInfo.Bssid, 1);
	return RT_STATUS_SUCCESS;
}

//
// Description:
//	Handle the P2P presence request from the client in the GO. If the GO accept the NoA, it will change its NoA attribute
//	and broadcast this info in the Beacon frame. The GO then sends the Presence Response with status which determines
//	the GO accept the client's request or not.
// Arguments:
//	See P2P_OnBeacon()
// Return:
//	Return RT_STATUS_SUCCESS if the parsing succeeds.
// By Bruce, 2010-05-07.
//
RT_STATUS
P2P_OnPresenceReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	)
{
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO			pP2PInfo = GET_P2P_INFO(pAdapter); 
	P2P_POWERSAVE_SET	tmpP2PPsSet;
	u1Byte				Status = P2P_STATUS_SUCCESS;
	u1Byte				NoADesc = 0, PreType, DiagToken, i;
	u4Byte				PreNoADuration, PreNoAInterval;
	BOOLEAN				bMyClient = TRUE;
	pu1Byte				pSaAddr = Frame_pTaddr(*posMpdu);

	P2P_MESSAGE			msg;

	if(!P2P_ENABLED(GET_P2P_INFO(pAdapter)))
		return RT_STATUS_SUCCESS;

	FunctionIn(COMP_P2P);

	if(pP2PInfo->Role != P2P_GO)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] The role is not Go!\n"));
		return RT_STATUS_INVALID_STATE;
	}
	
	RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2P_OnDeviceDiscoverabilityRsp():\n", posMpdu->Octet, posMpdu->Length);

	do
	{
		if(RT_STATUS_SUCCESS == (rtStatus = p2p_parse_Action(posMpdu, DBG_LOUD, &msg)))
			if(!p2p_validate_PresenceReq(&msg))
			{
				rtStatus = RT_STATUS_MALFORMED_PKT;
				break;
			}
		
		// Check if this is our client's request.
		if(P2PClientInfoFindByInterfaceAddress(pP2PInfo, pSaAddr) == NULL)
		{
			bMyClient = FALSE;
			RT_PRINT_ADDR(COMP_P2P, DBG_WARNING, "P2P_OnPresenceReq():[WARNING] not my client: ", pSaAddr);
			// return;
		}

		PlatformZeroMemory(&tmpP2PPsSet, sizeof(P2P_POWERSAVE_SET));

		// Fill the CTWin. The Presence request does not apply in this field.
		tmpP2PPsSet.bOppPs = pP2PInfo->bOppPS;
		tmpP2PPsSet.CTWindow = pP2PInfo->CTWindow;
		
		NoADesc = (msg.noaLen == (13 + 2)) ? 1 : (msg.noaLen == (2 * 13 + 2)) ? 2 : 0;
		
		// Invalid Length
		if(msg.noaLen != 2 && NoADesc == 0)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Invalid Length!!\n"));
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
	
		DiagToken = GET_P2P_VENDOR_ACT_FRAME_DIALOG_TOKEN(posMpdu->Octet);

		for(i = 0; i < NoADesc; i ++)
		{
			PreType = ReadEF1Byte(msg._noa + (13 * i) + 2);
			PreNoADuration = ReadEF4Byte(msg._noa + (13 * i) + 3);
			PreNoAInterval = ReadEF4Byte(msg._noa + (13 * i) + 7);
			
			if(PreType < 1 || PreType > 2)
			{ // NoA Type is invalid.
				Status = P2P_STATUS_FAIL_INVALID_PARAMETERS;
				break;
			}

			// The duration or interval is not acceptable.
			if(PreNoADuration < 1024 || PreNoADuration > PreNoAInterval)
			{ // NoA count is invalid.
				Status = P2P_STATUS_FAIL_UNABLE_TO_ACCOMODATE_REQUEST;
				break;
			}

			tmpP2PPsSet.NoASet[i].bNoAEn = ((PreNoAInterval - PreNoADuration) > 0) ? TRUE : FALSE; // If the PreNoAInterval equals to duration, that means the client needs always be awake.
			tmpP2PPsSet.NoASet[i].NoACnt = 0xFF;
			tmpP2PPsSet.NoASet[i].NoADur = (PreNoAInterval - PreNoADuration);
			tmpP2PPsSet.NoASet[i].NoAInt = PreNoAInterval;
		}

		// We don't accpet the presence request for any client, because we do not support NoA/Opps in all ICs.
		// By Bruce, 2010-12-15.
		Status = P2P_STATUS_FAIL_UNABLE_TO_ACCOMODATE_REQUEST;

		if(Status == P2P_STATUS_SUCCESS)
			P2PSetPowerSaveMode(pP2PInfo, &tmpP2PPsSet, 0, TRUE);

		p2p_Send_PresenceRsp(pP2PInfo, pSaAddr, DiagToken, Status);
	}while(FALSE);

	p2p_parse_FreeMessage(&msg);
 
	return rtStatus;
}

//
// Description:
//	Update the managed information from the AP which supports P2P managed device.
// Arguments:
//	[in] pP2PInfo - 
//		The P2P context.
//	[in] pBssDesc -
//		The BSS descriptor from the scan list.
// Return:
//	None.
// By Bruce, 2010-05-14.
//
VOID
P2PUpdateWlanApManagedInfo(
	IN	PP2P_INFO		pP2PInfo,
	IN	PRT_WLAN_BSS	pBssDesc
	)
{
	PRT_AP_INFO 	pApInfo = NULL;
	
	// We need check if the BSS support P2P device management, but now the WLAN AP may not have this info in Beacon,
	// and it just disassociates us with the P2P deauth reason. Check this bit if the test plan has modified. By Bruce, 2010-05-14.
	// if(!(pBssDesc->P2PManagedInfo & maP2PDeviceManagement))
	//	return;

	PADAPTER	pDefaultAdapter = GetDefaultAdapter(pP2PInfo->pAdapter);
	PADAPTER	pAdapter = pDefaultAdapter;

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		while(pAdapter!=NULL)
		{
			if(pAdapter->pNdis62Common->PortType == EXT_P2P_ROLE_PORT && pP2PInfo->Role == P2P_GO)
				break;
			pAdapter = GetNextExtAdapter(pAdapter);
		}
		if(pAdapter == NULL)
			return;
	}
	
	pApInfo = (PRT_AP_INFO)(pAdapter->MgntInfo.pApModeInfo);
	
	if(pBssDesc->osWmmAcParaIE.Length == WMM_PARAM_ELEMENT_SIZE)
	{
		CopyMemOS(&pApInfo->osWmmAcParaIE, pBssDesc->osWmmAcParaIE, WMM_PARAM_ELEMENT_SIZE)
		pApInfo->bSupportWmm = TRUE;
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2PUpdateWlanApManagedInfo() osWmmAcParaIE:\n", pApInfo->osWmmAcParaIE.Octet, pApInfo->osWmmAcParaIE.Length);
	}
	if(pBssDesc->bdCountryIE.Length > 0)
	{
		CopyMemOS(&pApInfo->osCountryIe, pBssDesc->bdCountryIE, pBssDesc->bdCountryIE.Length);
		pApInfo->bSupportCountryIe = TRUE;
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2PUpdateWlanApManagedInfo() osCountryIe:\n", pApInfo->osCountryIe.Octet, pApInfo->osCountryIe.Length);
	}
	if(pBssDesc->osPowerConstraintIe.Length == MAX_DOT11_POWER_CONSTRAINT_IE_LEN)
	{
		CopyMemOS(&pApInfo->osPowerConstraintIe, pBssDesc->osPowerConstraintIe, MAX_DOT11_POWER_CONSTRAINT_IE_LEN);
		pApInfo->bSupportPowerConstraint= TRUE;
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2PUpdateWlanApManagedInfo() osCountryIe:\n", pApInfo->osPowerConstraintIe.Octet, pApInfo->osPowerConstraintIe.Length);
	}

	
}

//
// Description:
//	Disable the P2P mode and clear the related variables.
// Arguments:
// Return:
//	None.
// By Bruce, 2010-05-14.
//
VOID
P2PDisable(
	IN	PADAPTER		Adapter
	)
{
	BOOLEAN 		bBoostIgi = FALSE;
	PP2P_INFO 	pP2PInfo = GET_P2P_INFO(Adapter);
	RT_TRACE(COMP_P2P, DBG_LOUD, ("======>P2PDisable()\n"));

	P2PSetRole(pP2PInfo, P2P_NONE);	
		p2p_DevList_Free(&pP2PInfo->devList);

	PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);

	pP2PInfo->State = P2P_STATE_DISABLED;
	pP2PInfo->bUpdateFromBeacon = FALSE;
	pP2PInfo->bUpdatePsParameter = FALSE;
	pP2PInfo->PsFlag = 0;
	pP2PInfo->NextTimeout = 0;
	pP2PInfo->usSleepTime = 0;
	pP2PInfo->P2pPsState = P2P_PS_AWAKE;
	pP2PInfo->NoAIEIndex = 0;
	pP2PInfo->bOppPS = FALSE;
	pP2PInfo->CTWindow = 0;
	PlatformZeroMemory(pP2PInfo->NoADescriptors, sizeof(P2P_NOA_DESCRIPTOR) * P2P_MAX_NUM_NOA_DESC);
	pP2PInfo->bEospByNoA = FALSE;

	if(pP2PInfo->pProfileList)
	{
		PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_P2P_SPIN_LOCK);
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Freeing previous profile list: %u profiles\n", pP2PInfo->pProfileList->nProfiles));
		PlatformFreeMemory(pP2PInfo->pProfileList, pP2PInfo->profileListLen);
		pP2PInfo->profileListLen = 0;
		pP2PInfo->pProfileList = NULL;
		PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_P2P_SPIN_LOCK);
	}

	// Make sure the PS related HW configuration is cleared.
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_P2P_PS_MODE, (pu1Byte)pP2PInfo);

	if(P2P_ADAPTER_RTK_SUPPORT_P2P(Adapter))
	{
		P2PIndicateCurrentState(pP2PInfo, pP2PInfo->State);
	}

	// Free allocated memory ---------------------
	P2PFreeAllocatedMemory(pP2PInfo->pAdapter);
	// -----------------------------------------

	if(P2P_ADAPTER_RTK_SUPPORT_P2P(Adapter))
	{
		bBoostIgi = FALSE;
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_BOOST_INIT_GAIN, &bBoostIgi);
	}
	
	RT_TRACE(COMP_P2P, DBG_LOUD, ("<======P2PDisable()\n"));
}

VOID
P2PTranslateP2PInfoToDevDescV0(
	IN PP2P_INFO pP2PInfo,
	OUT PP2P_DEVICE_DESCRIPTOR_V0 pDevDesc
	)
{
	u1Byte i = 0, j = 0;

	PlatformFillMemory(pDevDesc, sizeof(PP2P_DEVICE_DESCRIPTOR_V0), 0);
		
	pDevDesc->Role = pP2PInfo->Role;
	pDevDesc->DeviceCapability = pP2PInfo->DeviceCapability;
	pDevDesc->GroupCapability = pP2PInfo->GroupCapability;
	cpMacAddr(pDevDesc->DeviceAddress, pP2PInfo->DeviceAddress);
	PlatformMoveMemory(&pDevDesc->WpsAttributes, &pP2PInfo->WpsAttributes, sizeof(P2P_WPS_ATTRIBUTES));
	pDevDesc->RegulatoryClass = pP2PInfo->RegulatoryClass;
	pDevDesc->ListenChannel = pP2PInfo->ListenChannel;
	pDevDesc->OperatingChannel = pP2PInfo->OperatingChannel;
	PlatformMoveMemory(pDevDesc->CountryString, pP2PInfo->CountryString, 3);
	if(RT_STATUS_SUCCESS != MgntActQuery_P2PChannelList(pP2PInfo->pAdapter, sizeof(pDevDesc->ChannelPlanChannel), &pDevDesc->ChannelPlanLength, pDevDesc->ChannelPlanChannel))
	{
		RT_TRACE(COMP_P2P, DBG_WARNING, ("P2PTranslateP2PInfoToDevDesc(): MgntActQuery_P2PChannelList returns error\n"));
	}

	{
		u1Byte StartIndex = 0;
		u1Byte CurrentIndex = 0;
		PP2P_CLIENT_INFO_DISCRIPTOR pClientInfoDesc = NULL;

		pDevDesc->P2PClientDescriptorListLength = 0;
		
		while((pClientInfoDesc = P2PClientInfoEnumClients(pP2PInfo, StartIndex, &CurrentIndex)) != NULL)
		{
			PlatformMoveMemory(&(pDevDesc->P2PClientDescriptorList[pDevDesc->P2PClientDescriptorListLength]), 
				pClientInfoDesc, 
				sizeof(P2P_CLIENT_INFO_DISCRIPTOR));
			pDevDesc->P2PClientDescriptorListLength++;

			StartIndex = CurrentIndex + 1;
			pClientInfoDesc = NULL;
		}	

		RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PTranslateP2PInfoToDevDesc(): %u clients enumerated\n", pDevDesc->P2PClientDescriptorListLength));
	}

	PlatformMoveMemory(pDevDesc->SsidBuf, pP2PInfo->SSIDBuf, pP2PInfo->SSIDLen);
	pDevDesc->SsidLen = pP2PInfo->SSIDLen;
	pDevDesc->GOIntent = pP2PInfo->GOIntent;
	pDevDesc->GOConfigurationTimeout = pP2PInfo->GOConfigurationTimeout;
	pDevDesc->ClientConfigurationTimeout = pP2PInfo->ClientConfigurationTimeout;
	pDevDesc->DialogToken = pP2PInfo->DialogToken;
	//cpMacAddr(pDevDesc->GroupBssid, pP2PInfo->GroupBssid);
	cpMacAddr(pDevDesc->IntendedP2PInterfaceAddress, pP2PInfo->InterfaceAddress);
	pDevDesc->Status = pP2PInfo->Status;
	pDevDesc->MinorReasonCode = pP2PInfo->MinorReasonCode;
	pDevDesc->ExtendedListenTimingPeriod = pP2PInfo->ExtListenTimingPeriod;
	pDevDesc->ExtendedListenTimingDuration = pP2PInfo->ExtListenTimingDuration;
}

VOID
P2PTranslateP2PInfoToDevDescV1(
	IN PP2P_INFO pP2PInfo,
	OUT PP2P_DEVICE_DESCRIPTOR_V1 pDevDesc
	)
{
	P2PTranslateP2PInfoToDevDescV0(pP2PInfo, (PP2P_DEVICE_DESCRIPTOR_V0)pDevDesc);
	pDevDesc->SignalStrength = 0;
}

u4Byte
P2PTranslateP2PInfoToDevDesc(
	IN PP2P_INFO pP2PInfo,
	OUT PVOID pvDevDesc
	)
{
	u4Byte BytesWritten = 0;
	
	if(0x00000000 == pP2PInfo->P2PVersion)
	{// v0
		P2PTranslateP2PInfoToDevDescV0(pP2PInfo, (PP2P_DEVICE_DESCRIPTOR_V0)pvDevDesc);
		BytesWritten = sizeof(P2P_DEVICE_DESCRIPTOR_V0);
	}
	else if(0x00000001 == pP2PInfo->P2PVersion)
	{// v1
		P2PTranslateP2PInfoToDevDescV1(pP2PInfo, (PP2P_DEVICE_DESCRIPTOR_V1)pvDevDesc);
		BytesWritten = sizeof(P2P_DEVICE_DESCRIPTOR_V1);
	}
	else
	{// v2/v3
		PP2P_DEVICE_DESCRIPTOR_V2 pLatestDevDesc = pvDevDesc;
		P2PTranslateP2PInfoToDevDescV1(pP2PInfo, (PP2P_DEVICE_DESCRIPTOR_V1)pvDevDesc);
		pLatestDevDesc->version = pP2PInfo->P2PVersion;
		PlatformFillMemory(pLatestDevDesc->manufacturerName, 65, 0);
		BytesWritten = sizeof(P2P_DEVICE_DESCRIPTOR_V2);
	}

	return BytesWritten;
}

VOID
P2P_StartApRequest(
	IN	PADAPTER	Adapter
	)
{
	if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
	{//DbgPrint("AP_StartApRequest: fire timer again\n");
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Fire P2PMgntTimer!\n"));
		PlatformSetTimer( Adapter, &((GET_P2P_INFO(Adapter))->P2PMgntTimer), 0);
	}
}

VOID
P2P_APRemoveKey(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	)
{
	if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
	{
		if(/*pSTA->bAssociated && bAssociated has been cleared*/ pSTA->perSTAKeyInfo.SecLvl == RT_SEC_LVL_WPA2 && pSTA->perSTAKeyInfo.PairwiseCipherSuite[0] == RT_ENC_ALG_AESCCMP)
		{// P2P Clients are always WPA2AES

			P2PIndicateClientDisconnected(GET_P2P_INFO(Adapter), pSTA->MacAddr);
		}
	}
}


VOID
P2P_APSetkey(
	IN	PADAPTER		Adapter,
	IN	u1Byte 			*pucMacAddr, 
	IN	u4Byte 			ulEncAlg
	)
{
	if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
	{
		if(ulEncAlg == CAM_AES)

		{
			P2PIndicateClientConnected(GET_P2P_INFO(Adapter),
				pucMacAddr);
		}
	}
}


VOID
P2P_FilerCck(
	IN		PADAPTER		Adapter,
	IN	OUT	BOOLEAN			*pbFilterCck
	)
{
	if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
	{
		*pbFilterCck = TRUE;
	}
}

VOID
P2P_ConstructAssociateReqFilterCck(
	IN		PADAPTER		Adapter,
	IN		OCTET_STRING	AsocReq,
	IN	OUT	BOOLEAN			*pbFilterCck
	)
{	
	if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
	{
		if(P2PScanListIsGo(GET_P2P_INFO(Adapter), Frame_pDaddr(AsocReq)))
		{// to P2P GO
			*pbFilterCck = TRUE;
		}
		else
		{// to wlan AP
			*pbFilterCck = FALSE;
		}
	}
	
}

//
// Description:
//	Determine if we shall accept this probe request when the current state is P2P GO mode.
// Arguments:
//	[in] pAdapter -
//		The location of target adapter.
//	[in] posMpdu -
//		The location address of full 802.11 frame.
//	[in] posSsidToScan -
//		The SSID in this probe request.
// Return:
//	Return TRUE if 
//		a. we shall send probe response for this probe reqeust in P2P GO mode, or
//		b. this action is ignored if P2P is not enabled.
//	Return FALSE if the P2P inforamtion in this packet is mismatch and we shall not send probe response.
// Remark:
//	When this function returns FALSE, it means the probe requst is a P2P probe request but the check information
//	is not match for our P2P GO. So this probe request must be dropped and not be responsed.
//	if the SSID is P2P Wildcard SSID (DIRECT-), this function changes the length of SSID to 0 so that our
//	AP can response this request by passing SSID check.
//
BOOLEAN
P2P_IsGoAcceptProbeReq(
	IN		PADAPTER		pAdapter,
	IN		POCTET_STRING	posMpdu,
	IN		POCTET_STRING	posSsidToScan
	)
{
	BOOLEAN		bAccept = TRUE;
	if(P2P_ENABLED(GET_P2P_INFO(pAdapter))) 
	{
		RT_STATUS		rtStatus = RT_STATUS_SUCCESS;		
		P2P_MESSAGE		msg;

		if((GetDefaultAdapter(pAdapter)->MgntInfo.RegMultiChannelFcsMode >= MULTICHANNEL_FCS_SUPPORT_GO) &&
			(MultiChannelSwitchNeeded(pAdapter)))
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("Reject p2p probe request because fw switching is running!\n"));
			bAccept = FALSE;
			return bAccept;
		}

		if(RT_STATUS_SUCCESS != (rtStatus = p2p_parse_Ies(posMpdu, DBG_TRACE, &msg)))
		{
			return bAccept;
		}

		GET_P2P_INFO(pAdapter)->bProbeReqByLegacyClient = (msg.p2pAttributes.os.Length) ? (FALSE) : (TRUE);
		if(!GET_P2P_INFO(pAdapter)->bProbeReqByLegacyClient)
		{
			OCTET_STRING	osWpsIE = PacketGetElement(*posMpdu, EID_Vendor, OUI_SUB_SimpleConfig, OUI_SUB_DONT_CARE);
			if(P2PIsWildcardSsid(*posSsidToScan))
			{// Clause 3.2.2: P2P Wildcard SSID shall be treated the same as the Wildcard SSID
				posSsidToScan->Length = 0; // so that it does not return false when SSID is neither any nor our SSID
			}
			
			if(!P2PAcceptProbeReq(GET_P2P_INFO(pAdapter), posMpdu, &msg))
			{
				bAccept = FALSE;
			}
		}
		
		p2p_parse_FreeMessage(&msg);
	}
	return	bAccept;
}

VOID
P2P_UpdateScanList(
	IN		PADAPTER		Adapter
	)
{
	if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
	{
		PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);

		if(P2P_ADAPTER_OS_SUPPORT_P2P(Adapter))
		{

			// Update the device list for the OS query ------------------
			P2PDeviceListActionInterface(
					pP2PInfo, 
					P2P_DEVICE_LIST_ACTION_COPY_TO_QUERY_LIST,
					NULL, NULL, NULL
				);

			//P2PDeviceListActionInterface(
			//		pP2PInfo, 
			//		P2P_DEVICE_LIST_ACTION_DUMP,
			//		&pP2PInfo->DeviceList, 
			//		NULL, NULL
			//	);
			
			P2PDeviceListActionInterface(
					pP2PInfo, 
					P2P_DEVICE_LIST_ACTION_DUMP,
					&pP2PInfo->DeviceListForQuery, 
					NULL, NULL
				);
			//----------------------------------------------------
		}

		if(P2PScanListEqual(pP2PInfo->ScanList, pP2PInfo->ScanListSize, 
			pP2PInfo->ScanList4Query, pP2PInfo->ScanList4QuerySize))
		{
			//RT_TRACE(COMP_P2P, DBG_LOUD, 
			//	("ScanComplete(): scan list the same, not to update ScanList4Query\n"));
		}
		else
		{
			P2PScanListCopy(pP2PInfo->ScanList4Query, 
				&pP2PInfo->ScanList4QuerySize, 
				pP2PInfo->ScanList, 
				pP2PInfo->ScanListSize);

			P2PIndicateScanList(pP2PInfo);
		}

		P2PSvc_OnP2PScanComplete(pP2PInfo->pP2PSvcInfo);
	}
	
}


BOOLEAN
P2P_ScanCallback(
	IN		PADAPTER		Adapter
	)
{
	if(P2P_ENABLED(GET_P2P_INFO(Adapter)))
	{
		PP2P_INFO pP2PInfo = GET_P2P_INFO(Adapter);

		//
		// Send 2 ProbeReq only when we are in Search State or Scan Phase.
		// This is to prevent the case, for example, when  we are using customized 
		// scan to send a ProvisionDiscReq to a  P2P Device, we don't send two 
		// ProvisionDiscReq.
		//
		if(pP2PInfo->State != P2P_STATE_SEARCH &&
			pP2PInfo->State != P2P_STATE_SCAN)
		{
			//break;
			return	FALSE;
		}
	}
	return	TRUE;
}


VOID
P2P_InactivePsCallback(
	IN	PADAPTER			pAdapter,
	IN	u1Byte				InactievPsState
	)
{
	if(P2P_ENABLED(GET_P2P_INFO(pAdapter)))
	{
		//
		// This workitem cancels all timer (including P2P Mgnt Timer),
		// and this has an effect on our Extended Listening Counter,
		// so we have to re set the timer here.
		//
		PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);

		// TODO: Should not set timer in workitem 
		if(!GetDefaultAdapter(pAdapter)->bDriverIsGoingToUnload)
		{
			//if(pPSC->eInactivePowerState == eRfOff)
			if(InactievPsState == eRfOff)
				PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0);	
		}
	}	
}

// 
// Description:
//	Set the service response fragment threshold.
// Arguments:
//	[in] pAdapter -
//		The NIC context pointer.
//	[in] serviceThreshold -
//		The fragment threshold for the service threshold.
// Return:
//	RT_STATUS_SUCCESS if success. Otherwise return the RT_STATUS error code.
// By Bruce, 2011-05-31.
//
RT_STATUS
P2PSetServiceFragThreshold(
	IN	PADAPTER	pAdapter,
	IN	u2Byte		serviceThreshold
	)
{
	PP2P_INFO 	pP2PInfo = GET_P2P_INFO(pAdapter);
	
	if(!pP2PInfo)
		return RT_STATUS_INVALID_CONTEXT;

	if(serviceThreshold < P2P_SERVICE_MIN_RSP_FRG_THRESHOLD || serviceThreshold > P2P_SERVICE_MAX_RSP_FRAG_THRESHOLD)
		return RT_STATUS_INVALID_PARAMETER;

	pP2PInfo->SDRspFragmtntThreshold = serviceThreshold;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("P2PSetServiceFragThreshold(): Set threshold = %d\n", pP2PInfo->SDRspFragmtntThreshold));
	return RT_STATUS_SUCCESS;
}

RT_STATUS
P2PSetProfileList(
	IN	PADAPTER	pAdapter,
	IN	PP2P_PROFILE_LIST pProfileList
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO 	pP2PInfo = GET_P2P_INFO(pAdapter);
	u4Byte		allocSize = FIELD_OFFSET(P2P_PROFILE_LIST, profileList) + pProfileList->nProfiles * sizeof(P2P_PROFILE_LIST_ENTRY);
	
	if(!pP2PInfo)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("RT_STATUS_INVALID_CONTEXT\n"));
		return RT_STATUS_INVALID_CONTEXT;
	}

	PlatformAcquireSpinLock(pP2PInfo->pAdapter, RT_P2P_SPIN_LOCK);

	do
	{
		if(pP2PInfo->pProfileList)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Freeing previous profile list: %u profiles\n", pP2PInfo->pProfileList->nProfiles));
			PlatformFreeMemory(pP2PInfo->pProfileList, pP2PInfo->profileListLen);
			pP2PInfo->profileListLen = 0;
			pP2PInfo->pProfileList = NULL;
		}
		
		if(RT_STATUS_SUCCESS != (rtStatus = PlatformAllocateMemory(pAdapter, &pP2PInfo->pProfileList, allocSize)))
		{
			break;
		}
		
		pP2PInfo->profileListLen = allocSize;
		PlatformMoveMemory(pP2PInfo->pProfileList, pProfileList, allocSize);
		
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Set profile list: %u profiles\n", pP2PInfo->pProfileList->nProfiles));

	}while(FALSE);

	PlatformReleaseSpinLock(pP2PInfo->pAdapter, RT_P2P_SPIN_LOCK);
	
	return rtStatus;
}

// 
// Description:
//	Get the service response fragment threshold.
// Arguments:
//	[in] pAdapter -
//		The NIC context pointer.
//	[in] pServiceThreshold -
//		The fragment threshold pointer for the service threshold.
// Return:
//	RT_STATUS_SUCCESS if success. Otherwise return the RT_STATUS error code.
// By Bruce, 2011-05-31.
//
RT_STATUS
P2PGetServiceFragThreshold(
	IN	PADAPTER	pAdapter,
	IN	pu2Byte		pServiceThreshold
	)
{
	PP2P_INFO 	pP2PInfo = GET_P2P_INFO(pAdapter);
	
	if(!pP2PInfo || !pServiceThreshold)
		return RT_STATUS_INVALID_CONTEXT;

	*pServiceThreshold = pP2PInfo->SDRspFragmtntThreshold;

	return RT_STATUS_SUCCESS;
}

#if 1
//-------------------------------------------------------------------------------
// For delaying sending the peer response packet from the OID command
//-------------------------------------------------------------------------------
VOID
P2POidPostProcessTimerCallback(
	IN PRT_TIMER		pTimer
)
{
	PADAPTER pAdapter = (PADAPTER)pTimer->Adapter;
	PMGNT_INFO pMgntInfo = &(pAdapter->MgntInfo);
	PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
	u1Byte i = 0;
	
	// For IRQL Maintaince ------------------------------
//	BOOLEAN bIrqlRestoreRequired = FALSE; 
//	KIRQL BackupIrql = PASSIVE_LEVEL;
	// -----------------------------------------------

	FunctionIn(COMP_P2P);

	if(RT_DRIVER_STOP(pAdapter))
	{
		PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
		return;
	}

	// ----------------------------------------------------------------------
	RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: P2PGetChannel: %d\n", __FUNCTION__, P2PGetChannel(pP2PInfo)));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: KeGetCurrentIrql(): %d\n", __FUNCTION__, KeGetCurrentIrql()));

	switch(pP2PInfo->OidOperation)
	{
		case OID_OPERATION_INDICATE_DISCOVERY_COMPLETE:
		{
#if 0			
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Delay 10 ms to avoid too fast indication for WlanSvc!\n"));
			delay_ms(10);
#endif			
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: P2P_EVENT_DEVICE_DISCOVERY_COMPLETE\n", __FUNCTION__));
			PlatformIndicateP2PEvent(pP2PInfo, P2P_EVENT_DEVICE_DISCOVERY_COMPLETE, NULL);
		} break;

		case OID_OPERATION_SEND_PACKET:
		{
				p2p_DevList_Lock(&pP2PInfo->devList);
				
				if(pP2PInfo->PacketSentInWorkItemCallback == P2P_PUB_ACT_PROVISION_DISCOVERY_RSP)
				{
					// Start sending provision discovery response ---------------------------------------------------
					CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);	// In order not to switch channel
					p2p_Send_PDRsp(
							pP2PInfo,
							pP2PInfo->ProvisionResponseReceiverDeviceAddress,
							pP2PInfo->ProvisionResponseDialogToken,
							NULL,
							0, 			// It is no use in Win8 since WPS IEs are generated by the OS
							NULL
						);
					
					delay_ms(10);
				}
				else if(pP2PInfo->PacketSentInWorkItemCallback == P2P_PUB_ACT_INVITATION_RSP)
				{
					P2P_STATE  StateBackup = pP2PInfo->State;
					
					pP2PInfo->State = P2P_STATE_INVITATION_REQ_RECVD;
					
					// Start sending invitation response -----------------------------------------------------------
					//CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);	// In order not to switch channel
					p2p_Send_InvitationRsp(
							pP2PInfo, 
							pP2PInfo->InvitationResponseReceiverDeviceAddress,
							pP2PInfo->InvitationResponseDialogToken
						);

					pP2PInfo->State = StateBackup;
					
					// Postpone the extended listening
					P2PExtendedListenResetCounter(pP2PInfo);
						
					// Update the state machine immediately 
					PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
					PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0); 
					
				}
				else if(pP2PInfo->PacketSentInWorkItemCallback == P2P_PUB_ACT_GO_NEGO_RSP)
				{
					// Start sending GO negotiation response ---------------------------------------------------
					CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);	// In order not to switch channel
					p2p_Send_GoNegRsp(
							pP2PInfo,
							pP2PInfo->NegotiationResponsePeerDeviceAddress,
							pP2PInfo->NegotiationResponseDialogToken
						);

					// Sync the status between Win8 and Win7
					pP2PInfo->ConnectionContext.Status = pP2PInfo->NegotiationResponseStatus;
					
					// + Update the state machine
					if(pP2PInfo->ConnectionContext.Status == P2P_STATUS_SUCCESS)
					{
						pP2PInfo->State = P2P_STATE_GO_NEGO_RSP_SEND;
					}
					else if(pP2PInfo->Role != P2P_CLIENT)
					{
						pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
					}

					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2P state 0x%x\n", pP2PInfo->State));

					// Update the state machine immediately 
					PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
					PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0); 
					
				}
				else if(pP2PInfo->PacketSentInWorkItemCallback == P2P_PUB_ACT_GO_NEGO_CONFIRM)
				{
					// Start sending GO negotiation confirmation ----------------------------------------------------
					//CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);	// In order not to switch channel
					BOOLEAN bSupportTxReport = FALSE;
					p2p_Send_GoNegConfirm(
								pP2PInfo,
								pP2PInfo->NegotiationConfirmPeerDeviceAddress,
							pP2PInfo->NegotiationConfirmDialogToken,
								&bSupportTxReport
							);

					// + Update the state machine
					pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
					
					// Update the state machine immediately 
					PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
					PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 100); // In order not to switch channel

					delay_ms(10);
				}
				else
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Illegal Packet Type!\n", __FUNCTION__));
				}

				// Reset the packet type -------------------------------------------------
				pP2PInfo->PacketSentInWorkItemCallback = (P2P_PUBLIC_ACTION_TYPE) 0xFF;
				// --------------------------------------------------------------------

				p2p_DevList_Unlock(&pP2PInfo->devList);
		} break;
			
		default:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Wrong Operation Mode in WorkItem!\n", __FUNCTION__));
			break;
	}

#if 0
	// Restore the IRQL Back ---------------------
	if(bIrqlRestoreRequired) KeLowerIrql(BackupIrql);
	// ----------------------------------------
#endif

	FunctionOut(COMP_P2P);
}

#else

//-------------------------------------------------------------------------------
// For delaying sending the peer response packet from the OID command
//-------------------------------------------------------------------------------
VOID
P2POidPostProcessWorkItemCallback(
	IN	PVOID	pContext
)
{
	PP2P_INFO pP2PInfo = (PP2P_INFO) pContext;
	u1Byte i = 0;

	// For IRQL Maintaince ------------------------------
	BOOLEAN bIrqlRestoreRequired = FALSE; 
	KIRQL BackupIrql = PASSIVE_LEVEL;
	// -----------------------------------------------

	FunctionIn(COMP_P2P);
	
	for(i = 0; i < 10; i++)
	{

		if(pP2PInfo->bPostoneP2POidPostProcessWorkItem)
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: delay_ms(5) for other PASSIVE_LEVEL thread running~\n", __FUNCTION__));			
			delay_ms(5);
		}	
		else
			break;
	}
	
	// Let this OID routine run faster than the P2POidPostProcessWorkItemCallback ------
	if(KeGetCurrentIrql() == PASSIVE_LEVEL)
	{
		bIrqlRestoreRequired = TRUE;
		KeRaiseIrql(DISPATCH_LEVEL, &BackupIrql);
	}
	// ----------------------------------------------------------------------

	RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: P2PGetChannel: %d\n", __FUNCTION__, P2PGetChannel(pP2PInfo)));
	RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: KeGetCurrentIrql(): %d\n", __FUNCTION__, KeGetCurrentIrql()));

	switch(pP2PInfo->OidOperation)
	{
		case OID_OPERATION_INDICATE_DISCOVERY_COMPLETE:
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Delay 10 ms to avoid too fast indication for WlanSvc!\n"));
			delay_ms(10);
			
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: P2P_EVENT_DEVICE_DISCOVERY_COMPLETE\n", __FUNCTION__));
			if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
				PlatformIndicateP2PEvent(pP2PInfo, P2P_EVENT_DEVICE_DISCOVERY_COMPLETE, NULL);
		} break;

		case OID_OPERATION_SEND_PACKET:
		{
				if(pP2PInfo->PacketSentInWorkItemCallback == P2P_PUB_ACT_PROVISION_DISCOVERY_RSP)
				{
					// Start sending provision discovery response ---------------------------------------------------
					CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);	// In order not to switch channel
					p2p_Send_PDRsp(
							pP2PInfo,
							pP2PInfo->ProvisionResponseReceiverDeviceAddress,
							pP2PInfo->ProvisionResponseDialogToken,
							0 			// It is no use in Win8 since WPS IEs are generated by the OS
						);
				}
				else if(pP2PInfo->PacketSentInWorkItemCallback == P2P_PUB_ACT_INVITATION_RSP)
				{
					P2P_STATE  StateBackup = pP2PInfo->State;
					
					pP2PInfo->State = P2P_STATE_INVITATION_REQ_RECVD;
					
					// Start sending invitation response -----------------------------------------------------------
					//CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);	// In order not to switch channel
					p2p_Send_InvitationReq(
							pP2PInfo, 
							pP2PInfo->InvitationResponseReceiverDeviceAddress,
							pP2PInfo->InvitationResponseDialogToken
						);

					pP2PInfo->State = StateBackup;
					
					// Postpone the extended listening
					P2PExtendedListenResetCounter(pP2PInfo);
						
					// Update the state machine immediately 
					PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
					PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0); 
					
				}
				else if(pP2PInfo->PacketSentInWorkItemCallback == P2P_PUB_ACT_GO_NEGO_RSP)
				{
					// Start sending GO negotiation response ---------------------------------------------------
					CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);	// In order not to switch channel
					p2p_Send_GoNegRsp(
							pP2PInfo,
							pP2PInfo->NegotiationResponsePeerDeviceAddress,
							pP2PInfo->NegotiationResponseDialogToken
						);

					// Sync the status between Win8 and Win7
					pP2PInfo->ConnectionContext.Status = pP2PInfo->NegotiationResponseStatus;
					
					// + Update the state machine
					if(pP2PInfo->ConnectionContext.Status == P2P_STATUS_SUCCESS)
					{
						pP2PInfo->State = P2P_STATE_GO_NEGO_RSP_SEND;
					}
					else if(pP2PInfo->Role != P2P_CLIENT)
					{
						pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
					}

					// Update the state machine immediately 
					PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
					PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 0); 
					
				}
				else if(pP2PInfo->PacketSentInWorkItemCallback == P2P_PUB_ACT_GO_NEGO_CONFIRM)
				{
					BOOLEAN bSupportTxReport = FALSE;
					// Start sending GO negotiation confirmation ----------------------------------------------------
					//CustomScan_ExtendDwellTime(GET_CUSTOM_SCAN_INFO(pAdapter), 100);	// In order not to switch channel
					p2p_Send_GoNegConfirm(
							pP2PInfo,
							pP2PInfo->NegotiationConfirmPeerDeviceAddress,
							pP2PInfo->NegotiationConfirmDialogToken,
							&bSupportTxReport
						);

					// + Update the state machine
					pP2PInfo->State = P2P_STATE_GO_NEGO_COMPLETE;
					
					// Update the state machine immediately 
					PlatformCancelTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer);
					PlatformSetTimer(pP2PInfo->pAdapter, &pP2PInfo->P2PMgntTimer, 100); // In order not to switch channel
				}
				else
				{
					RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Illegal Packet Type!\n", __FUNCTION__));
				}

				// Reset the packet type -------------------------------------------------
				pP2PInfo->PacketSentInWorkItemCallback = (P2P_PUBLIC_ACTION_TYPE) 0xFF;
				// --------------------------------------------------------------------
		} break;
			
		default:
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Wrong Operation Mode in WorkItem!\n", __FUNCTION__));
			break;
	}


	// Restore the IRQL Back ---------------------
	if(bIrqlRestoreRequired) KeLowerIrql(BackupIrql);
	// ----------------------------------------
	
	FunctionOut(COMP_P2P);
}
#endif

// -----------------------------------------
// Reset the Client Join Group Context
// -----------------------------------------
VOID
P2PResetClientJoinGroupContext(
	PP2P_INFO pP2PInfo
)
{
	pP2PInfo->ClientJoinGroupContext.bInGroupFormation = FALSE;
	pP2PInfo->ClientJoinGroupContext.WpsState = P2P_CLIETN_JOIN_GROUP_WPS_STATE_NONE;
	pP2PInfo->ClientJoinGroupContext.uWaitForWpsSlotCount = 0;			// 100 ms per slot

	PlatformCancelTimer(
			pP2PInfo->pAdapter, 
			&pP2PInfo->ClientJoinGroupContext.P2PWaitForWpsReadyTimer
		);
}

//-------------------------------------------------------------------------------
// For delaying connection to wait for GO WPS ready
//-------------------------------------------------------------------------------
static 
VOID
p2pWaitForWpsReady_CustomScanCb(
	IN  CUSTOM_SCAN_STATE		state,
	IN  VOID					*pCtx
	)
{
	P2P_INFO					*info = (P2P_INFO *)pCtx;
	MGNT_INFO					*mgnt = &info->pAdapter->MgntInfo;
	u4Byte						it = 0;

	if(!P2P_ENABLED(info))
		return;
	
	if(CUSTOM_SCAN_STATE_COMPLETED == state)
	{
		if(P2P_CLIETN_JOIN_GROUP_WPS_STATE_NONE != info->ClientJoinGroupContext.WpsState)
		{	
			PlatformSetTimer(info->pAdapter, &info->ClientJoinGroupContext.P2PWaitForWpsReadyTimer, 1);
		}

		mgnt->NumBssDesc4Query = mgnt->NumBssDesc;
		for(it = 0; it < mgnt->NumBssDesc4Query; it++)
		{
			CopyWlanBss(mgnt->bssDesc4Query + it, mgnt->bssDesc + it);
		}
	}

	return;
}

VOID
P2PWaitForWpsReadyTimerCallback(
	IN PRT_TIMER		pTimer
)
{
	PADAPTER pAdapter = (PADAPTER) pTimer->Adapter;
	PMGNT_INFO pMgntInfo = &(pAdapter->MgntInfo);
	PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);
	NDIS_STATUS		ndisStatus = NDIS_STATUS_SUCCESS;
	
	VOID						*customScanInfo = GET_CUSTOM_SCAN_INFO(pAdapter);
	VOID 						*req = NULL;
	FRAME_BUF					*probeReqBuf = NULL;

	FunctionIn(COMP_P2P);

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		if(MgntScanInProgress(pMgntInfo))
		{
			PlatformSetTimer(pAdapter, &pP2PInfo->ClientJoinGroupContext.P2PWaitForWpsReadyTimer, 16);
			return;
		}

		switch(pP2PInfo->ClientJoinGroupContext.WpsState)
		{
		default:
			// Reset the variable in OID_DOT11_WFD_GROUP_START_PARAMETERS ------------------
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Unknown wps state = %d\n", pP2PInfo->ClientJoinGroupContext.WpsState));
			P2PResetClientJoinGroupContext(pP2PInfo);
			break;
			
		case P2P_CLIETN_JOIN_GROUP_WPS_STATE_SCANNING:
			{
				if(0 == pP2PInfo->ClientJoinGroupContext.uWaitForWpsSlotCount)
				{
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("WPS slot count is 0, stop scan!\n"));
					// Reset the variable in OID_DOT11_WFD_GROUP_START_PARAMETERS ------------------
					P2PResetClientJoinGroupContext(pP2PInfo);
					break;
				}
				pP2PInfo->ClientJoinGroupContext.uWaitForWpsSlotCount --;

				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Call Customzied Scan to find out WPS Group!\n"));

				if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
					return;

				// Call customized scan to scan the WPS group.												
				if(pP2PInfo->uGroupTargetSSIDLength > 0 && pP2PInfo->uGroupTargetSSIDLength <= 32)
				{
					u4Byte	bufLen = 0;
					
					PlatformMoveMemory(pMgntInfo->Ssid.Octet, pP2PInfo->GroupTargetSSID, pP2PInfo->uGroupTargetSSIDLength);
					pMgntInfo->Ssid.Length = (u2Byte)(pP2PInfo->uGroupTargetSSIDLength);
					
					// Send probe req with the interface address as the SA and we shall append P2P IE in this case,
					// otherwise the ProbeRsp would be regarded as from a legacy device and would not be processed.
					probeReqBuf = CustomScan_GetProbeReqBuf(req);
					ConstructProbeRequest(pAdapter, FrameBuf_MHead(probeReqBuf), &bufLen, TRUE, FALSE, FALSE);
					FrameBuf_Add(probeReqBuf, (u2Byte)bufLen);
				}
				
				CustomScan_AddScanChnl(req, pP2PInfo->OperatingChannel, 1, SCAN_ACTIVE, P2P_SCAN_WPS_GROUP_PERIOD, MGN_6M, probeReqBuf);
				CustomScan_AddScanChnl(req, pP2PInfo->ListenChannel, 1, SCAN_ACTIVE, 100, MGN_6M, probeReqBuf);
				CustomScan_SetRepeatCount(req, 16);
				CustomScan_SetupCbCtx(req, p2pWaitForWpsReady_CustomScanCb, pP2PInfo);
				CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "cli join grp wps scanning");
			}
			break;
			
		case P2P_CLIENT_JOIN_GROUP_WPS_STATE_GO_READY:
			{
				OCTET_STRING	ssidStr = {NULL, 0};
				PRT_WLAN_BSS	pRtBss = NULL;
				RT_JOIN_ACTION	joinAction = RT_NO_ACTION;
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2P_CLIENT_JOIN_GROUP_WPS_STATE_GO_READY\n"));

//Sinda: temporarily mark. If OS care bForceScanLegacyNetworks flag 
//and un-mark the flag checking at case P2P_STATE_DEV_DISC_START of P2PMgntTimerCallback function.
//We need to do legacy scan to update scan list to avoid to get out-of-date information of GO.
#if 0
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Target WPS info is ready but the BSS maybe out-of-date in the scan list, try to customized scan...\n"));
				// Call customized scan to scan the WPS group.												
				if(pP2PInfo->uGroupTargetSSIDLength > 0 && pP2PInfo->uGroupTargetSSIDLength <= 32)
				{
					u4Byte	bufLen = 0;
					PlatformMoveMemory(pMgntInfo->Ssid.Octet, pP2PInfo->GroupTargetSSID, pP2PInfo->uGroupTargetSSIDLength);
					pMgntInfo->Ssid.Length = (u2Byte)(pP2PInfo->uGroupTargetSSIDLength);
					ConstructProbeRequest(pAdapter, &(pScanReq->ProbeReqBuf[0]), (pu4Byte)&bufLen, TRUE, FALSE, FALSE);

					pScanReq->ProbeReqLen = (u2Byte)bufLen;
				}			
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("P2P_CLIENT_JOIN_GROUP_WPS_STATE_GO_READY: MgntActSet_802_11_CustomizedScanRequest\n"));
				MgntActSet_802_11_CustomizedScanRequest(pAdapter, pScanReq);	
#endif

				if(PlatformAllocateMemory(pAdapter, (PVOID*)&pRtBss, sizeof(RT_WLAN_BSS)) != RT_STATUS_SUCCESS)
					return;	

				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2P_CLIENT_JOIN_GROUP_WPS_STATE_GO_READY: try to connect selected network by SSID(%s)\n", pP2PInfo->GroupTargetSSID));
				
				FillOctetString(ssidStr, pP2PInfo->GroupTargetSSID, (u2Byte)(pP2PInfo->uGroupTargetSSIDLength));		
				joinAction = SelectNetworkBySSID(pAdapter, &ssidStr, FALSE, pRtBss);
				if(pRtBss)
					PlatformFreeMemory(pRtBss, sizeof(RT_WLAN_BSS));
				
				if(RT_JOIN_INFRA == joinAction)
				{ // The target is in the scan list, try to join it.
					pP2PInfo->ClientJoinGroupContext.WpsState = P2P_CLIETN_JOIN_GROUP_WPS_STATE_ASSOCIATING;
					PlatformSetTimer(pAdapter, &pP2PInfo->ClientJoinGroupContext.P2PWaitForWpsReadyTimer, 0);
					return;
				}

				if(NULL == (req = CustomScan_AllocReq(customScanInfo, NULL, NULL)))
					return;

				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Target WPS info is ready but the BSS is not in the scan list, try to customized scan...\n"));
				// Call customized scan to scan the WPS group.												
				if(pP2PInfo->uGroupTargetSSIDLength > 0 && pP2PInfo->uGroupTargetSSIDLength <= 32)
				{
					u4Byte	bufLen = 0;
					PlatformMoveMemory(pMgntInfo->Ssid.Octet, pP2PInfo->GroupTargetSSID, pP2PInfo->uGroupTargetSSIDLength);
					pMgntInfo->Ssid.Length = (u2Byte)(pP2PInfo->uGroupTargetSSIDLength);

					probeReqBuf = CustomScan_GetProbeReqBuf(req);
					ConstructProbeRequest(pAdapter, FrameBuf_MHead(probeReqBuf), &bufLen, TRUE, FALSE, FALSE);
					FrameBuf_Add(probeReqBuf, (u2Byte)bufLen);
				}			
				RT_TRACE_F(COMP_MLME, DBG_LOUD, ("P2P_CLIENT_JOIN_GROUP_WPS_STATE_GO_READY: CustomScan_IssueReq\n"));
				
				CustomScan_AddScanChnl(req, pP2PInfo->OperatingChannel, 2, SCAN_ACTIVE, P2P_SCAN_WPS_GROUP_PERIOD, MGN_6M, probeReqBuf);
				CustomScan_AddScanChnl(req, pP2PInfo->ListenChannel, 2, SCAN_ACTIVE, P2P_SCAN_WPS_GROUP_PERIOD, MGN_6M, probeReqBuf);
				CustomScan_SetupCbCtx(req, p2pWaitForWpsReady_CustomScanCb, pP2PInfo);
				CustomScan_IssueReq(customScanInfo, req, CUSTOM_SCAN_SRC_TYPE_P2P, "cli join grp wps go ready");
			}
			break;

		case P2P_CLIETN_JOIN_GROUP_WPS_STATE_ASSOCIATING:
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2P_CLIETN_JOIN_GROUP_WPS_STATE_ASSOCIATING\n"));
				pAdapter->pNdis62Common->CurrentOpState = OP_STATE;

				ndisStatus = N6CSet_DOT11_CONNECT_REQUEST(
						pAdapter,
						NULL, 0, 0, 0		// Don't Care
					);
				pP2PInfo->ClientJoinGroupContext.WpsState = P2P_CLIETN_JOIN_GROUP_WPS_STATE_HANDSHAKING;
			}
			break;

		case P2P_CLIETN_JOIN_GROUP_WPS_STATE_HANDSHAKING:
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("P2P_CLIETN_JOIN_GROUP_WPS_STATE_HANDSHAKING\n"));
			}
			break;
		}
	}

}

//-------------------------------------------------------------------------------
// Public P2P Utility Functions
//-------------------------------------------------------------------------------

// The public interface for operating the device lists outside this file
BOOLEAN
P2PDeviceListActionInterface(
	PP2P_INFO pP2PInfo,
	IN u1Byte uAction,
	IN PP2P_DEVICE_LIST 	pDeviceList,
	IN PMEMORY_BUFFER	pInputBuffer,
	OUT PMEMORY_BUFFER pOutputBuffer
)
{
	BOOLEAN bStatus = FALSE;
	
	switch(uAction)
	{
		case P2P_DEVICE_LIST_ACTION_COPY_TO_QUERY_LIST:
		{
			PlatformAcquireSpinLock(pP2PInfo->pAdapter,	RT_P2P_SPIN_LOCK);
			P2PDeviceListCopyWithTimeStamp(
					pP2PInfo,
					&pP2PInfo->DeviceListForQuery,
					&pP2PInfo->DeviceList
				);

			PlatformReleaseSpinLock(pP2PInfo->pAdapter,	RT_P2P_SPIN_LOCK);
			
			bStatus = TRUE;
		} break;

		case P2P_DEVICE_LIST_ACTION_CLEAR:
		{
			PlatformAcquireSpinLock(pP2PInfo->pAdapter,	RT_P2P_SPIN_LOCK);
			P2PDeviceListClear(pDeviceList);
			PlatformReleaseSpinLock(pP2PInfo->pAdapter,	RT_P2P_SPIN_LOCK);
			bStatus = TRUE;
		} break;	

		case P2P_DEVICE_LIST_ACTION_DUMP:
		{
			P2PDeviceListDump(pDeviceList);
			bStatus = TRUE;
		} break;
	
		default:
		{
			RT_TRACE(COMP_P2P, DBG_LOUD, ("%s: Illegal Action Failure!\n", __FUNCTION__));
		} break;
	}

	return bStatus;
}

BOOLEAN
P2PAddScanDeviceID(
	PP2P_DEVICE_ID_TO_SCAN	pScanDeviceIDs,
	pu1Byte	pDeviceID
)
{
	BOOLEAN		bStatus = FALSE;
	ULONG		i;
	BOOLEAN		bDuplicate;

	do
	{
		if(pScanDeviceIDs->uNumOfDeviceIDs > P2P_MAX_DEVICE_ID_TO_SCAN)
			break;

		bDuplicate = FALSE;
	
		for(i = 0; i < pScanDeviceIDs->uNumOfDeviceIDs; i++)
		{
			if(eqMacAddr(pDeviceID, pScanDeviceIDs->DeviceIDs[i])) 
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Found the MAC. Index: %d\n", i));
				bDuplicate = TRUE;
			}
		}

		if(bDuplicate == FALSE)
		{
			cpMacAddr(
					pScanDeviceIDs->DeviceIDs[pScanDeviceIDs->uNumOfDeviceIDs],
					pDeviceID
				);

			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Add MAC to Device list. Index: %d\n", pScanDeviceIDs->uNumOfDeviceIDs));

			pScanDeviceIDs->uNumOfDeviceIDs++;
		}

		bStatus = TRUE;
	
	}while (FALSE);

	return bStatus;
}

VOID
P2PClearScanDeviceID(
	PP2P_DEVICE_ID_TO_SCAN	pScanDeviceIDs
)
{
	FunctionIn(COMP_P2P);
	PlatformZeroMemory(pScanDeviceIDs, sizeof(P2P_DEVICE_ID_TO_SCAN));
	FunctionOut(COMP_P2P);
}

// Free all allocated memory: PVOID or MEMORY_BUFFER
VOID
P2PFreeAllocatedMemory(
	IN PADAPTER	pAdapter
)
{
	PP2P_INFO 	pP2PInfo = pAdapter->MgntInfo.pP2PInfo;
	
	P2P_AddIe_Free(&pP2PInfo->AdditionalIEs);

	// Free the allocated memory of the device list -----------------------------------------------------------------
	P2PDeviceListClear(&pP2PInfo->DeviceList);
	P2PDeviceListClear(&pP2PInfo->DeviceListForQuery);
	// ------------------------------------------------------------------------------------------------------
}

//
// Description:
//	Free P2P information memory allocated by P2P_AllocP2PInfo().
// Arguments:
//	[in] pAdapter -
//		The adapter context which the P2P info belongs to.
// Return:
//	None.
//
VOID
P2P_FreeP2PInfo(
	IN	PADAPTER	pAdapter
	)
{
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);
	PP2P_INFO	pP2PInfo = NULL;

	FunctionIn(COMP_P2P);
	
	if(NULL == pMgntInfo->pP2PInfo)
		return;

	pP2PInfo = (PP2P_INFO)pMgntInfo->pP2PInfo;

	if(pP2PInfo->pProfileList)
	{
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Freeing previous profile list: %u profiles\n", pP2PInfo->pProfileList->nProfiles));
		PlatformFreeMemory(pP2PInfo->pProfileList, pP2PInfo->profileListLen);
		pP2PInfo->profileListLen = 0;
		pP2PInfo->pProfileList = NULL;
	}

	P2PSvc_Free_P2PSvcInfo(pMgntInfo->pP2PInfo);

	PlatformFreeMemory(pP2PInfo, sizeof(P2P_INFO));

	pMgntInfo->pP2PInfo = NULL;

	FunctionOut(COMP_P2P);
}

//
// Description:
//	Allocate the P2P information memory which contains all P2P variables.
// Arguments:
//	[in] pAdapter -
//		The adapter context which the P2P info belongs to.
// Return:
//	If allocation process succeeded, return RT_STATUS_SUCESS. If the system cannot allocate enough memory
//	the context, return RT_STATUS_RESOURCE. Otherwise, return the RT_STATUS error code.
// Remark:
//	This function allocates memory by structure "P2P_INFO" and initialize the default context. The caller must call
//	P2P_FreeP2PInfo() to free the allocated memory.
//
RT_STATUS
P2P_AllocP2PInfo(
	IN	PADAPTER	pAdapter
	)
{
	RT_STATUS	rtStatus = RT_STATUS_SUCCESS;
	PMGNT_INFO	pMgntInfo = &(pAdapter->MgntInfo);
	PP2P_INFO	pP2pInfo = NULL;

	FunctionIn(COMP_P2P);
	do
	{
		pMgntInfo->pP2PInfo = NULL;
		if(RT_STATUS_SUCCESS != (rtStatus = PlatformAllocateMemory(pAdapter, &(pMgntInfo->pP2PInfo), sizeof(P2P_INFO))))
			break;

		PlatformZeroMemory(pMgntInfo->pP2PInfo, sizeof(P2P_INFO));
		pAdapter->P2PSupport = P2P_SUPPORT_STATE_UNINITIALIZED;
		pP2pInfo = (PP2P_INFO)(pMgntInfo->pP2PInfo);
		// Pointer to the default adapter, then after OS/RTL initialized the info, the adapter pointer will be 
		// asigned to the corresponding context.
		pP2pInfo->pAdapter = GetDefaultAdapter(pAdapter);

		pP2pInfo->pP2PSvcInfo = NULL;
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocP2PSvcInfo(pMgntInfo->pP2PInfo)))
			break;
		
	}while(FALSE);

	if(RT_STATUS_SUCCESS != rtStatus)
	{
		RT_TRACE_F(COMP_WFD, DBG_SERIOUS, ("[ERROR] Allocation failed with error (%d)!!!\n", rtStatus));
		P2PSvc_Free_P2PSvcInfo(pMgntInfo->pP2PInfo);
		P2P_FreeP2PInfo(pAdapter);
	}

	FunctionOut(COMP_P2P);

	return rtStatus;
}

//
// Description:
//	Set the SSID which will be the GO SSID used in the auto GO or Go negotiation.
// Arguments:
//	[in] pAdapter -
//		The adapter context which the P2P info belongs to.
//	[in] pSsidBuf -
//		The location points the SSID buffer.
//	[in] ssidLen -
//		The length for the SSID in pSsidBuf.
// Return:
//	If set process succeeds, return RT_STATUS_SUCESS.
// Remark:
//	This function does NOT check the valid SSID as defined in P2P spec (such as DIRECT-xxx).
//	If the ssidLen is 0, the SSID will be set as random.
//
RT_STATUS
P2P_SetP2PGoFullSSID(
	IN	PADAPTER	pAdapter,
	IN	pu1Byte		pSsidBuf,
	IN	u4Byte		ssidLen
	)
{
	PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);

	if(ssidLen > MAX_SSID_LEN)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Invalid SSID length = %d\n", ssidLen));
		return RT_STATUS_INVALID_DATA;
	}

	pP2PInfo->regSSIDLen = (u1Byte)ssidLen;

	if(pP2PInfo->regSSIDLen > 0)
	{
		PlatformMoveMemory(pP2PInfo->regSSIDBuf, pSsidBuf, pP2PInfo->regSSIDLen);
	}

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2P_SetP2PGoFullSSID(): SSID =\n", pP2PInfo->regSSIDBuf, pP2PInfo->regSSIDLen);

	return RT_STATUS_SUCCESS;
}

//
// Description:
//	Get the SSID which will be the GO SSID used in the auto GO or Go negotiation.
// Arguments:
//	[in] pAdapter -
//		The adapter context which the P2P info belongs to.
//	[in] maxSsidBufLen -
//		The max length in pSsidBuf.
//	[out] pSsidBuf -
//		The location points the SSID buffer.
//	[out] pSsidLen -
//		The length for the SSID in pSsidBuf.
// Return:
//	If set process succeeds, return RT_STATUS_SUCESS.
// Remark:
//	This function does NOT check the valid SSID as defined in P2P spec (such as DIRECT-xxx).
//	If the ssidLen is 0, the SSID will be set as random.
//
RT_STATUS
P2P_GetP2PGoFullSSID(
	IN	PADAPTER	pAdapter,
	IN	u4Byte		maxSsidBufLen,
	IN	pu1Byte		pSsidBuf,
	IN	pu4Byte		pSsidLen
	)
{
	PP2P_INFO pP2PInfo = GET_P2P_INFO(pAdapter);

	if(maxSsidBufLen < (u4Byte)pP2PInfo->regSSIDLen)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] maxSsidBufLen = %d < SSID Len (%d)\n", maxSsidBufLen, pP2PInfo->regSSIDLen));
		return RT_STATUS_BUFFER_TOO_SHORT;
	}

	*pSsidLen = (u4Byte)pP2PInfo->regSSIDLen;

	if(pP2PInfo->regSSIDLen > 0)
	{
		PlatformMoveMemory(pSsidBuf, pP2PInfo->regSSIDBuf, pP2PInfo->regSSIDLen);
	}

	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2P_GetP2PGoFullSSID(): SSID =\n", pSsidBuf, *pSsidLen);

	return RT_STATUS_SUCCESS;
}

//
// Description:
//	Check if any p2p device exists.
//	[in] pAdapter -
//		The adapter context for any port.
// Return:
//	Return TRUE if any p2p port is found.
// Remark:
//	The input adapter can be from any port and this function checks each port adapter.
//
BOOLEAN
IsP2PDeviceExisting(
	IN	PADAPTER	pAdapter
	)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pDevicePort = NULL;
	
	pDevicePort = pDefaultAdapter;
	
	while(pDevicePort != NULL)
	{
		if(P2P_ENABLED(GET_P2P_INFO(pDevicePort)))
		{			
			return TRUE;
		}	
		pDevicePort = GetNextExtAdapter(pDevicePort);
	}

	return FALSE;
}

//
// Description:
//	Check if the RTK P2P device exists.
//	[in] pAdapter -
//		The adapter context for any port.
// Return:
//	Return TRUE if any RTK p2p port is found.
// Remark:
//	The input adapter can be from any port and this function checks each port adapter.
//
BOOLEAN
IsRTKP2PDeviceExisting(
	IN	PADAPTER	pAdapter
	)
{
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pDevicePort = NULL;
	
	pDevicePort = pDefaultAdapter;
	
	while(pDevicePort != NULL)
	{
		// If any RTK P2P exists, return FALSE.
		if(P2P_ENABLED(GET_P2P_INFO(pDevicePort)) && P2P_ADAPTER_RTK_SUPPORT_P2P(pDevicePort))
		{			
			return TRUE;
		}

		// ToDo, check other P2P conditions.
		pDevicePort = GetNextExtAdapter(pDevicePort);
	}

	return FALSE;
}


//
// Description:
//	Check if the IPS is accessible.
//	[in] pAdapter -
//		The adapter context for any port.
// Return:
//	Return TRUE if IPS is accessible for P2P mode.
// Remark:
//	The input adapter can be from any port and this function checks each port adapter.
//
BOOLEAN
P2P_IPS_Accessible(
	IN	PADAPTER	pAdapter
	)
{
	if(IsRTKP2PDeviceExisting(pAdapter))
	{
		return FALSE;
	}

	return TRUE;
}

VOID
P2P_Construct_ProbeReqEx(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					ssidLen,
	IN  const u1Byte			*ssidBuf
	)
{
	p2p_Construct_ProbeReqEx(pBuf, pP2PInfo, da, ssidLen, ssidBuf);
	return;
}

VOID
P2P_Construct_ProbeReq(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	)
{
	p2p_Construct_ProbeReq(pBuf, pP2PInfo);
}

//
// Description:
//	Change the primary (and sub) device category if the setting in P2P info is unknown.
//	Sometimes the OS may just set the primary category as 0 (reserved in WPS spec) if
//	there is one application applying P2P or miracast sink function,
//	and we shall change the device category.
// Arguments:
//	[in] pAdapter -
//		The adapter context for any port.
// Return:
//	If setting process succeeds, return RT_STATUS_SUCESS.
//
RT_STATUS
P2P_CorrectDeviceCategory(
	IN	PADAPTER	pAdapter
	)
{
	RT_STATUS			rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO			pP2PInfo = GET_P2P_INFO(pAdapter);
	WFD_DEV_TYPE		devType = WFD_DEV_TYPE_SOURCE;
	const FRAME_BUF		*pAddIeBuf = NULL;
	u4Byte				wfdInfoLen = 0;
	u2Byte				priCategory = WPS_PRI_DEV_TYPE_CATEGORY_PC;		
	
	if(!P2P_ENABLED(pP2PInfo))
		return RT_STATUS_INVALID_STATE;

	do
	{
		if(WPS_PRI_DEV_TYPE_CATEGORY_UNKNOWN == pP2PInfo->WpsAttributes.PrimaryDeviceType.CategoryId ||
			WPS_PRI_DEV_TYPE_CATEGORY_PC == pP2PInfo->WpsAttributes.PrimaryDeviceType.CategoryId)
		{
			// Change it when the category is unknown or PC
		}
		else
		{
			// Do nothing
			break;
		}

		if(NULL != (pAddIeBuf = P2P_AddIe_Get(&pP2PInfo->AdditionalIEs, P2P_ADD_IE_PROBE_RESPONSE)))
		{
			OCTET_STRING	osTemp = pAddIeBuf->os;
			
			wfdInfoLen = sizeof(WFD_DEV_TYPE);
			if(RT_STATUS_SUCCESS == (rtStatus = WFD_GetInfoOnIEs(pAdapter, WFD_INFO_DEVICE_TYPE, &osTemp, (pu1Byte)&devType, &wfdInfoLen)))
			{
				if(WFD_DEV_TYPE_SOURCE == devType)
				{
					priCategory = WPS_PRI_DEV_TYPE_CATEGORY_PC;
				}
				else
				{
					priCategory = WPS_PRI_DEV_TYPE_CATEGORY_DISPLAY;
				}
			}
		}

		pP2PInfo->WpsAttributes.PrimaryDeviceType.CategoryId = priCategory;
		pP2PInfo->WpsAttributes.PrimaryDeviceType.SubCategoryId = WPS_PRI_DEV_TYPE_SUB_CATEGORY_NO_SUB;
		PlatformMoveMemory(pP2PInfo->WpsAttributes.PrimaryDeviceType.Oui,
							WPS_NO_SUB_CAT_OUI, 4);
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Changed WPS Category = 0x%04X, SubCategory = 0x%04X\n",
			pP2PInfo->WpsAttributes.PrimaryDeviceType.CategoryId,
			pP2PInfo->WpsAttributes.PrimaryDeviceType.SubCategoryId));
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2P_CorrectDeviceCategory(): OUI =\n",
			pP2PInfo->WpsAttributes.PrimaryDeviceType.Oui, 4);
	}while(FALSE);

	return rtStatus;
}

//======================================================================
// Wi-Fi Direct End
//======================================================================

#endif // #if (P2P_SUPPORT == 1)

