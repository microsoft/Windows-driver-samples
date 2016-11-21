#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Attribute.tmh"
#endif

#if (P2P_SUPPORT == 1)
#include "P2P_Internal.h"

#ifdef MOD_TRACE_TAG
#undef MOD_TRACE_TAG
#endif
#define MOD_TRACE_TAG "P2PAttr"

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

static
VOID
p2pattr_make_DevType(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					catId,
	IN  const u1Byte			*oui,
	IN  u2Byte 					subCatId
	)
{
	FrameBuf_Add_be_u2(pBuf, catId);
	FrameBuf_Add_Data(pBuf, oui, 4);
	FrameBuf_Add_be_u2(pBuf, subCatId);
	
	return;
}

static
VOID
p2pattr_make_WpsDevType(
	IN  FRAME_BUF				*pBuf,
	IN  const P2P_WPS_ATTRIBUTES_DEVICE_TYPE *type
	)
{
	p2pattr_make_DevType(pBuf, type->CategoryId, type->Oui, type->SubCategoryId);
	
	return;
}

static
VOID
p2pattr_make_WpsSecDevTypeList(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					numSecDevType,
	IN  const P2P_WPS_ATTRIBUTES_DEVICE_TYPE *pSecDevTypeList 
	)
{
	u1Byte 						itTypeList = 0;
	
	FrameBuf_Add_u1(pBuf, numSecDevType);
	for(itTypeList = 0; itTypeList < numSecDevType; itTypeList++)
	{
		p2pattr_make_WpsDevType(pBuf, pSecDevTypeList + itTypeList);
	}

	return;
}

static
VOID
p2pattr_make_DevName(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					devNameLen,
	IN  const u1Byte			*devName
	)
{
	FrameBuf_Add_be_u2(pBuf, P2P_WPS_ATTR_TAG_DEVICE_NAME);
	FrameBuf_Add_be_u2(pBuf, devNameLen);
	FrameBuf_Add_Data(pBuf, devName, devNameLen);

	return;
}

static
VOID
p2pattr_make_CliInfoDesc(
	IN  FRAME_BUF				*pBuf,
	IN  const RT_WLAN_STA		*pCli
	)
{
	const P2P_CLIENT_INFO_DISCRIPTOR 	*pDesc = &pCli->P2PClientInfoDesc;
	const P2P_WPS_ATTRIBUTES			*pWps = &pDesc->WpsAttributes;
	u1Byte								*pLen = NULL;
	u2Byte								initBufLen = 0;

	if(NULL == (pLen = FrameBuf_Add(pBuf, 1))) return;

	initBufLen = FrameBuf_Length(pBuf);
	
	FrameBuf_Add_Data(pBuf, pDesc->DeviceAddress, 6);
	FrameBuf_Add_Data(pBuf, pDesc->InterfaceAddress, 6);
	FrameBuf_Add_u1(pBuf, pDesc->DeviceCapability);
	FrameBuf_Add_be_u2(pBuf, pWps->ConfigMethod);
	p2pattr_make_WpsDevType(pBuf, &pWps->PrimaryDeviceType);
	p2pattr_make_WpsSecDevTypeList(pBuf, pWps->SecondaryDeviceTypeLength, pWps->SecondaryDeviceTypeList);
	p2pattr_make_DevName(pBuf, pWps->DeviceNameLength, pWps->DeviceName);

	if(FrameBuf_Length(pBuf) - initBufLen > 0 && FrameBuf_Length(pBuf) - initBufLen < 0xFF)
	{
		*pLen = (u1Byte)(FrameBuf_Length(pBuf) - initBufLen);		
	}

	return;
}

static
VOID
p2pattr_make_CliInfoDescList(
	IN  FRAME_BUF				*pBuf,
	IN  const RT_WLAN_STA		*pCliList
	)
{
	u1Byte						itList = 0;
	
	for(itList = 0; itList < ASSOCIATE_ENTRY_NUM; itList++)
	{
		const RT_WLAN_STA 		*pEntry = pCliList + itList;
				
		if(!(pEntry->bUsed && pEntry->bAssociated && pEntry->bP2PClient))
			continue;

		p2pattr_make_CliInfoDesc(pBuf, pEntry);
	}
	
	return;
}

static
VOID
p2pattr_make_IntfAddrList(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					numIntfAddr,
	IN  const pu1Byte			intfAddrList
	)
{
	u1Byte						itList = 0;

	FrameBuf_Add_u1(pBuf, numIntfAddr);
	
	for(itList = 0; itList < numIntfAddr; itList++)
	{
		FrameBuf_Add_Data(pBuf, intfAddrList + (6 * itList), 6);
	}
	
	return;
}

VOID
p2pattr_Make_ChannelEntryList(
	IN  FRAME_BUF				*pBuf,
	IN  const P2P_CHANNELS		*commonChannels
	)
{
	u1Byte 						it = 0;

	for(it = 0; it < commonChannels->regClasses; it++)
	{
		if(!commonChannels->regClass[it].channels)
			continue;
		
		P2PAttr_Make_ChannelEntry(pBuf, 
			commonChannels->regClass[it].regClass, 
			commonChannels->regClass[it].channels, 
			commonChannels->regClass[it].channel);
	}

	return;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

VOID
P2PAttr_Update_AttrHdrLen(
	IN  FRAME_BUF				*pBuf,
	IN  const u1Byte			*pLen
	)
{
	WriteEF2Byte(pLen, (u2Byte)(FrameBuf_Tail(pBuf) - pLen - 2));
	return;
}

VOID
P2PAttr_Make_Status(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte 					status
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_STATUS);
	FrameBuf_Add_le_u2(pBuf, 1);
	FrameBuf_Add_u1(pBuf, status);
	
	//RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] statue: %u, channel: %u\n", MOD_TRACE_TAG, status));

	return;
}

VOID
P2PAttr_Make_MinorReasonCode(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte 					minorReasonCode
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_MINOR_REASON_CODE);
	FrameBuf_Add_le_u2(pBuf, 1);
	FrameBuf_Add_u1(pBuf, minorReasonCode);
	
	RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] minor reason code: %u\n", MOD_TRACE_TAG, minorReasonCode));

	return;
}

VOID
P2PAttr_Make_Capability(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					devCap,
	IN  u1Byte					grpCap
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_CAPABILITY);
	FrameBuf_Add_le_u2(pBuf, 2);
	FrameBuf_Add_u1(pBuf, devCap);
	FrameBuf_Add_u1(pBuf, grpCap);

	RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] devCap: %02X, grpCap: %02X\n", MOD_TRACE_TAG, devCap, grpCap));
	
	return;
}

VOID
P2PAttr_Make_DevId(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			devAddr
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_DEVICE_ID);
	FrameBuf_Add_le_u2(pBuf, 6);
	FrameBuf_Add_Data(pBuf, devAddr, 6);

	//RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] devAddr: " MACSTR "\n", MOD_TRACE_TAG, MAC2STR(devAddr)));

	return;
}

VOID
P2PAttr_Make_GoIntent(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					intent
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_GROUP_OWNER_INTENT);
	FrameBuf_Add_le_u2(pBuf, 1);
	FrameBuf_Add_u1(pBuf, intent);

	RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] intent: %u, tie: %u\n", MOD_TRACE_TAG, intent >> 1, intent & 0x01));

	return;
}

VOID 
P2PAttr_Make_ConfigTimeout(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte 					goTimeout,
	IN  u1Byte 					cliTimeout
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_CONFIGURATION_TIMEOUT);
	FrameBuf_Add_le_u2(pBuf, 2);
	FrameBuf_Add_u1(pBuf, goTimeout);
	FrameBuf_Add_u1(pBuf, cliTimeout);

	RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] goTimeout: %u (10ms), cliTimeout: %u (10ms)\n", MOD_TRACE_TAG, goTimeout, cliTimeout));

	return;
}

VOID
P2PAttr_Make_ListenChannel(
	IN  FRAME_BUF				*pBuf,
	IN  const char *			country,
	IN  u1Byte					regClass,
	IN  u1Byte					channel
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_LISTEN_CHANNEL);
	FrameBuf_Add_le_u2(pBuf, 5);
	FrameBuf_Add_Data(pBuf, country, 3);
	FrameBuf_Add_u1(pBuf, regClass);
	FrameBuf_Add_u1(pBuf, channel);

	RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] country: %c%c(0x%02x), reg class: %u, channel: %u\n", MOD_TRACE_TAG, country[0], country[1], country[2], regClass, channel));

	return;
}

VOID 
P2PAttr_Make_GroupBssid(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			grpBssid
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_GROUP_BSSID);
	FrameBuf_Add_le_u2(pBuf, 6);
	FrameBuf_Add_Data(pBuf, grpBssid, 6);

	//RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] grpBssid: " MACSTR "\n", MOD_TRACE_TAG, MAC2STR(grpBssid)));

	return;
}

VOID 
P2PAttr_Make_ExtListenTiming(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					period,
	IN  u2Byte					interval
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_EXT_LISTEN_TIMING);
	FrameBuf_Add_le_u2(pBuf, 4);
	FrameBuf_Add_le_u2(pBuf, period);
	FrameBuf_Add_le_u2(pBuf, interval);

	RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] period: %u (ms), interval: %u (ms)\n", MOD_TRACE_TAG, period, interval));

	return;
}

VOID 
P2PAttr_Make_IntendedIntfAddr(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			intfAddr
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_INTENDED_INTERFACE_ADDRESS);
	FrameBuf_Add_le_u2(pBuf, 6);
	FrameBuf_Add_Data(pBuf, intfAddr, 6);

	//RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] intended intf Addr: " MACSTR "\n", MOD_TRACE_TAG, MAC2STR(intfAddr)));

	return;
}

VOID
P2PAttr_Make_Manageability(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					manageability
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_MANAGEABILITY);
	FrameBuf_Add_le_u2(pBuf, 1);
	FrameBuf_Add_u1(pBuf, manageability);

	RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] manageability: %u\n", MOD_TRACE_TAG, manageability));

	return;
}

VOID
P2PAttr_Make_ChannelEntry(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					regClass,
	IN  u1Byte					nChannels,
	IN  const u1Byte			*channelList
	)
{
	u1Byte						it = 0;

	if(0 == nChannels)
		return;
	
	// Regulatory class
	FrameBuf_Add_u1(pBuf, regClass);

	// Number of channels
	FrameBuf_Add_u1(pBuf, nChannels);

	// Channel list
	for(it = 0; it < nChannels; it++) 
		FrameBuf_Add_u1(pBuf, channelList[it]);

	return;
}

VOID
P2PAttr_Make_ChannelList(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO				*pP2PInfo,
	IN  const P2P_CHANNELS		*channels
	)
{
	u1Byte 						*pLen = NULL;

	if(P2P_ADAPTER_OS_SUPPORT_P2P(pP2PInfo->pAdapter))
	{
		// If the default port is connected, then only use the default port channel ----------------------
		if(MgntLinkStatusQuery(GetDefaultAdapter(pP2PInfo->pAdapter)) == RT_MEDIA_CONNECT)
		{
		#if 0
			#if (MULTICHANNEL_SUPPORT == 1)	// For Compilation: Only PCI-E Support MultiChannel Now

			u1Byte uPortConnected20MhzChannel = MultiChannelGetPortConnected20MhzChannel(GetDefaultAdapter(pP2PInfo->pAdapter));
			
			MgntActSet_P2PChannelList(
					pP2PInfo->pAdapter, 
					1, 
					&uPortConnected20MhzChannel
				);
			
			#endif
		#endif
		}
		else
		{
			P2PInitializeChannelEntryList(pP2PInfo);
		}
	}
	// ----------------------------------------------------------------------------------

	FrameBuf_Add_u1(pBuf, P2P_ATTR_CHANNEL_LIST);
	pLen = FrameBuf_Add(pBuf, 2);
	FrameBuf_Add_Data(pBuf, pP2PInfo->CountryString, sizeof(pP2PInfo->CountryString));
	p2pattr_Make_ChannelEntryList(pBuf, channels);

	P2PAttr_Update_AttrHdrLen(pBuf, pLen);

	return;
}

VOID 
P2PAttr_Make_Noa(
	IN  FRAME_BUF 				*pBuf,
	IN  u1Byte					noaIEIndex,
	IN  BOOLEAN 				bOppPS,
	IN  u1Byte 					ctWindow,
	IN  u1Byte					noaDescNum,
	IN  const P2P_NOA_DESCRIPTOR *noaDesc
	)
{
	pu1Byte						pLen = NULL;
	u1Byte						oppPsAndCtWindow = 0;
	u1Byte						itNoaDesc = 0;

	FrameBuf_Add_u1(pBuf, P2P_ATTR_NOTICE_OF_ABSENCE);
	if(NULL == (pLen = FrameBuf_Add(pBuf, 2))) return;

	FrameBuf_Add_u1(pBuf, noaIEIndex);

	oppPsAndCtWindow = 0;
	oppPsAndCtWindow |= (bOppPS) ? BIT7 : 0;
	oppPsAndCtWindow |= (ctWindow & (~BIT7));
	FrameBuf_Add_u1(pBuf, oppPsAndCtWindow);

	for(itNoaDesc = 0; itNoaDesc < noaDescNum; itNoaDesc++)
	{
		if(!noaDesc[itNoaDesc].bValid) continue;

		FrameBuf_Add_u1(pBuf, noaDesc[itNoaDesc].CountOrType);
		FrameBuf_Add_le_u4(pBuf, noaDesc[itNoaDesc].Duration);
		FrameBuf_Add_le_u4(pBuf, noaDesc[itNoaDesc].Interval);
		FrameBuf_Add_le_u4(pBuf, noaDesc[itNoaDesc].StartTime);
	}

	P2PAttr_Update_AttrHdrLen(pBuf, pLen);

	return;
}

VOID
P2PAttr_Make_NoaFromPsSet(
	IN  FRAME_BUF				*pBuf,
	IN  const P2P_POWERSAVE_SET	*pPs
	)
{
	pu1Byte						pLen = NULL;
	u4Byte						itNoaDesc = 0;

	FrameBuf_Add_u1(pBuf, P2P_ATTR_NOTICE_OF_ABSENCE);
	if(NULL == (pLen = FrameBuf_Add(pBuf, 2))) return;

	for(itNoaDesc = 0; itNoaDesc < P2P_MAX_NUM_NOA_DESC; itNoaDesc++)
	{
		u1Byte					countType = 0;
		
		if(FALSE == pPs->NoASet[itNoaDesc].bNoAEn) continue;

		if(2 < pPs->NoASet[itNoaDesc].NoACnt || pPs->NoASet[itNoaDesc].NoACnt < 1)
			countType = 1;
		else 
			countType = pPs->NoASet[itNoaDesc].NoACnt;
			
		FrameBuf_Add_u1(pBuf, countType);
		FrameBuf_Add_le_u4(pBuf, pPs->NoASet[itNoaDesc].NoADur);
		FrameBuf_Add_le_u4(pBuf, pPs->NoASet[itNoaDesc].NoAInt);
	}
	
	P2PAttr_Update_AttrHdrLen(pBuf, pLen);

	return;
}

VOID
P2PAttr_Make_DevInfo(
	IN  FRAME_BUF				*pBuf,
	IN  const u1Byte			*devAddr,
	IN  u2Byte					configMethods,
	IN  const P2P_WPS_ATTRIBUTES_DEVICE_TYPE *pPrimDevType,
	IN  u1Byte 					numSecDevType,
	IN  const P2P_WPS_ATTRIBUTES_DEVICE_TYPE *pSecDevTypeList,
	IN  u1Byte					devNameLen,
	IN  const u1Byte			*devName
	)
{
	u1Byte						*pLen = NULL;
	u1Byte						itTypeList = 0;
	
	FrameBuf_Add_u1(pBuf, P2P_ATTR_DEVICE_INFO);
	if(NULL == (pLen = FrameBuf_Add(pBuf, 2))) return;

	FrameBuf_Add_Data(pBuf, devAddr, 6);
	FrameBuf_Add_be_u2(pBuf, configMethods);
	p2pattr_make_WpsDevType(pBuf, pPrimDevType);
	p2pattr_make_WpsSecDevTypeList(pBuf, numSecDevType, pSecDevTypeList);
	p2pattr_make_DevName(pBuf, devNameLen, devName);

	P2PAttr_Update_AttrHdrLen(pBuf, pLen);

	//FrameBuf_DumpFrom(pBuf, pLen - 1, COMP_P2P, FrameBuf_DbgLevel(pBuf), __FUNCTION__);

	return;
}

VOID
P2PAttr_Make_GroupInfo(
	IN  FRAME_BUF				*pBuf,
	IN  const RT_WLAN_STA		*pCliList
	)
{
	
	pu1Byte						pLen = NULL;
	FRAME_BUF					tBuf;

	FrameBuf_Init(FrameBuf_TailRoom(pBuf), 0, FrameBuf_MTail(pBuf), &tBuf);

	FrameBuf_Add_u1(&tBuf, P2P_ATTR_GROUP_INFO);
	if(NULL == (pLen = FrameBuf_Add(&tBuf, 2))) return;

	p2pattr_make_CliInfoDescList(&tBuf, pCliList);

	P2PAttr_Update_AttrHdrLen(&tBuf, pLen);

	if(0 == FrameBuf_Length(&tBuf))
	{
		// Clause 3.2.4: the p2p group owner shall not 
		// include a p2p group info subelement if it 
		// has zero connected p2p clients
		return;
	}

	FrameBuf_Add(pBuf, FrameBuf_Length(&tBuf));

	//FrameBuf_DumpFrom(pBuf, pLen - 1, COMP_P2P, FrameBuf_DbgLevel(pBuf), __FUNCTION__);

	return;
}

VOID 
P2PAttr_Make_GroupId(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			grpDevAddr,
	IN  const pu1Byte			grpSsid,
	IN  u1Byte					grpSsidLen
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_GROUP_ID);
	FrameBuf_Add_le_u2(pBuf, 6 + grpSsidLen);
	FrameBuf_Add_Data(pBuf, grpDevAddr, 6);
	FrameBuf_Add_Data(pBuf, grpSsid, grpSsidLen);

	//RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] grpDevAddr: " MACSTR "\n", MOD_TRACE_TAG, MAC2STR(grpDevAddr)));
	RT_PRINT_DATA(COMP_P2P, FrameBuf_DbgLevel(pBuf), "grpSsid", grpSsid, grpSsidLen);

	return;
}


VOID 
P2PAttr_Make_Interface(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			devAddr,
	IN  u1Byte					numIntfAddr,
	IN  const pu1Byte			intfAddrList
	)
{
	pu1Byte						pLen = NULL;

	FrameBuf_Add_u1(pBuf, P2P_ATTR_INTERFACE);
	if(NULL == (pLen = FrameBuf_Add(pBuf, 2))) return;

	FrameBuf_Add_Data(pBuf, devAddr, 6);
	p2pattr_make_IntfAddrList(pBuf, numIntfAddr, intfAddrList);

	P2PAttr_Update_AttrHdrLen(pBuf, pLen);

	//FrameBuf_DumpFrom(pBuf, pLen - 1, COMP_P2P, DBG_LOUD, __FUNCTION__);

	return;
}

VOID
P2PAttr_Make_OperatingChannel(
	IN  FRAME_BUF				*pBuf,
	IN  const char *			country,
	IN  u1Byte					regClass,
	IN  u1Byte					channel
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_OP_CHANNEL);
	FrameBuf_Add_le_u2(pBuf, 5);
	FrameBuf_Add_Data(pBuf, country, 3);
	FrameBuf_Add_u1(pBuf, regClass);
	FrameBuf_Add_u1(pBuf, channel);
	
	RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] country: 0x%02X%02X%02X, reg class: %u, channel: %u\n", MOD_TRACE_TAG, country[0], country[1], country[2], regClass, channel));

	return;
}

VOID 
P2PAttr_Make_InvitationFlags(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					flags
	)
{
	FrameBuf_Add_u1(pBuf, P2P_ATTR_INVITATION_FLAGS);
	FrameBuf_Add_le_u2(pBuf, 1);
	FrameBuf_Add_u1(pBuf, flags);

	RT_TRACE_F(COMP_P2P, FrameBuf_DbgLevel(pBuf), ("[%s] flags: 0x%02X\n", MOD_TRACE_TAG, flags));

	return;
}

#endif

#undef MOD_TRACE_TAG

#endif
