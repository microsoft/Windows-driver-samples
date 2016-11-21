#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "P2P_Parse.tmh"
#endif

#include "P2P_Internal.h"

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
static
RT_STATUS
p2p_parse_Status(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 1) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_status = data;
	msg->status = data[0];

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("%u\n", msg->status));

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_MinorReasonCode(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 1) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_minorReasonCode= data;
	msg->minorReasonCode = data[0];

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("%u\n", msg->minorReasonCode));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_Capability(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 2) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_capability= data;
	msg->devCap = data[0];
	msg->grpCap = data[1];

	// pDevDescriptor->Role = P2P_GO;

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("devCap: 0x%02X, grpCap: 0x%02X\n", msg->devCap, msg->grpCap));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_DevId(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 6) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_devId = data;
	msg->devIdDevAddr = data;

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, (MACSTR "\n", MAC2STR(msg->devIdDevAddr)));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_GoIntent(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 1) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_goIntent = data;
	msg->goIntent = data[0];

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("intent: %u, tie: %u\n", msg->goIntent >> 1, msg->goIntent & 0x01));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_ConfigTimeout(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 2) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_configTimeout = data;
	msg->goConfigTimeout = data[0];
	msg->cliConfigTimeout = data[1];

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("goConfigTimeout: %u, cliConfigTimeout: %u\n", msg->goConfigTimeout, msg->cliConfigTimeout));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_ListenChannel(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 5) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_listenChannel = data;
	msg->listenChannel = data[4];

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("country: %c%c(0x%02x), class: %u, channel: %u\n", 
		data[0], data[1], data[2],
		data[3],
		msg->listenChannel));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_GrpBssid(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 6) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_grpBssid = data;
	msg->grpBssid = data;

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, (MACSTR "\n", MAC2STR(msg->grpBssid)));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_ExtListenTiming(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 4) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_extListenTiming = data;
	msg->extListenPeriod = ReadEF2Byte(data);
	msg->extListenInterval = ReadEF2Byte(data + 2);

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("period: %u (ms), interval: %u (ms)\n", msg->extListenPeriod, msg->extListenInterval));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_IntendedIntfAddr(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 6) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_intendedIntfAddr = data;
	msg->intfAddr = data;

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, (MACSTR "\n", MAC2STR(msg->intfAddr)));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_Manageability(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 1) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_manageability = data;
	msg->manageability = data[0];

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("%u\n", msg->manageability));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_ChannelList(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	const u1Byte				*pos = data;
	
	if(len < 3) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_channelList = data;
	msg->channelListLen = len;
	
	msg->channelListCountry = pos;
	pos += 3;
	
	msg->channelEntryList = pos;
	msg->channelEntryListLen = (u2Byte)(data + len - pos);

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("country: %c%c(0x%02x)\n", 
		msg->channelListCountry[0], msg->channelListCountry[1], msg->channelListCountry[2]));

	RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "ChannelEntryList:\n", msg->channelEntryList, msg->channelEntryListLen);
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_NoA(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	const u1Byte				*pos = data;
	
	if(len < 2) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_noa = data;
	msg->noaLen = len;

	msg->noaIndex = pos[0];
	pos += 1;
	
	msg->noaCtWindowAndOppPsParam = pos[0];
	pos += 1;

	msg->noaDescriptors = pos;
	msg->noaDescriptorLen = (u2Byte)(data + len - pos);

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("index: %u, ctWindowAndOppPsParam: 0x%02X\n", msg->noaIndex, msg->noaCtWindowAndOppPsParam));

	RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "NoA Descriptors:\n", msg->noaDescriptors, msg->noaDescriptorLen);
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_DevInfo(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	const u1Byte				*pos = data;
	u1Byte						devNameLen = 0;
	
	if(len < 6 + 2 + 8 + 1) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_devInfo = data;

	msg->devInfoDevAddr = pos;
	pos += 6;

	msg->configMethod = ReadN2H2BYTE(pos); // In BE
	pos += 2;

	msg->primDevType = pos;
	pos += 8;

	msg->numSecDevType = pos[0];
	pos += 1;

	if(data + len - pos < 8 * msg->numSecDevType)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u, buf can't contain all sec dev types: %u\n", len, msg->numSecDevType));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->secDevTypeList = (0 < msg->numSecDevType) ? (pos) : (NULL);
	pos += (msg->numSecDevType * 8);

	if(data + len - pos < 4) // WPS attr type (2) and length (2)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u, buf can't contain WPS dev name attr\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	if(ATTR_DEV_NAME != ReadN2H2BYTE(pos))
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid WPS attr ID: 0x%02X, expecting: 0x%02X\n", N2H2BYTE(*(u2Byte *)pos), ATTR_DEV_NAME));
		return RT_STATUS_INVALID_DATA;
	}

	pos += 2;

	devNameLen = ReadN2H2BYTE(pos);
	pos += 2;

	if(data + len - pos < devNameLen || 32 < devNameLen)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid dev name len: %u, remain buf len: %u\n", devNameLen, (u4Byte)(data + len - pos)));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->devNameLen = devNameLen;
	msg->devName = pos;

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("devAddr: " MACSTR ", primDevType(id-oui-subid): %u-0x%08X-%u, configMethod: 0x%X\n",
		//MAC2STR(msg->devInfoDevAddr),
		//ReadN2H2BYTE(msg->primDevType + 0), ReadN2H4BYTE(msg->primDevType + 2), ReadN2H2BYTE(msg->primDevType + 6),
		//msg->configMethod));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_GrpInfo(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 0) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_grpInfo = data;
	msg->cliInfoDescList = data;
	msg->cliInfoDescListLen = len;

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, (""));
	RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "Group Info:\n", msg->cliInfoDescList, msg->cliInfoDescListLen);
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_GrpId(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	const u1Byte				*pos = data;
	
	if(len < 6 || 6 + 32 < len) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_grpId = data;

	msg->grpDevAddr = pos;
	pos += 6;

	msg->grpSsidLen = (u1Byte)len - 6;

	if(data + len - pos < msg->grpSsidLen)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid ssid len: %u, remain buf len: %u\n", msg->grpSsidLen, (u4Byte)(data + len - pos)));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->grpSsid = (msg->grpSsidLen) ? (pos) : (NULL);

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("grpDevAddr: " MACSTR "\n", MAC2STR(msg->grpDevAddr)));
	RT_PRINT_STR(COMP_P2P, msg->dbgLevel, "grpSsid:\n", msg->grpSsid, msg->grpSsidLen);
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_Interface(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	const u1Byte				*pos = data;
	u1Byte						itIntfAddr = 0;
	
	if(len < 6 + 1) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_intf = data;
	
	msg->intfDevAddr = pos;
	pos += 6;

	msg->intfAddrCount = pos[0];
	pos += 1;

	if(data + len - pos < 6 * msg->intfAddrCount)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid intf addr count: %u, length: %u\n", msg->intfAddrCount, len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->intfAddrList = pos;

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("intfDevAddr: " MACSTR "\n", MAC2STR(msg->intfDevAddr)));
	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("intf addr list:\n"));
	//for(itIntfAddr = 0; itIntfAddr < msg->intfAddrCount; itIntfAddr++)
		//RT_TRACE_F(COMP_P2P, msg->dbgLevel, (MACSTR "\n", MAC2STR(msg->intfAddrList + 6 * itIntfAddr)));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_OpChannel(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 5) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_opChannel = data;
	msg->opChannel = data[4];

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("country: %c%c(0x%02x), class: %u, channel: %u\n", 
		data[0], data[1], data[2],
		data[3],
		msg->opChannel));
	
	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_InvitationFlags(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 1) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_invitationFlags = data;
	msg->invitationFlags = data[0];

	// pP2PInfo->InvitationContext.bPersistentInvitation = (*(pu1Byte)osP2PSubIEs.Octet & ifP2PInvitationType) ? (TRUE) : (FALSE);

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("%u\n", msg->invitationFlags));

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_SvcHash(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	u2Byte						itSvcHash = 0;
	if(len < 6) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_svcHash = data;
	msg->svcHashLen = len;
	msg->svcHashNum = len / 6;
	msg->svcHashList = data;

	//for(itSvcHash = 0; itSvcHash < msg->svcHashNum; itSvcHash++)
	//{
		//RT_TRACE_F(COMP_P2P, msg->dbgLevel, (MACSTR "\n", MAC2STR(msg->svcHashList + 6 * itSvcHash)));
	//}

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_SessionInfoDataInfo(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 0) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_sessionInfoDataInfo = data;
	msg->sessionInfoLen = len;
	msg->sessionInfo = data;

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, (""));
	RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "Session Info:\n", msg->sessionInfo, msg->sessionInfoLen);

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_ConnCap(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 1) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_connCap = data;
	msg->connCap = data[0];

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("connCap: %u\n", msg->connCap));

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_AdvId(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	const u1Byte				*pos = data;
	
	if(len < 4 + 6) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_advId = data;
	
	msg->advId = ReadEF4Byte(pos);
	pos += 4;

	msg->svcMac = pos;

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("advId: %u (%08X), svcMac: " MACSTR "\n", msg->advId, msg->advId, MAC2STR(msg->svcMac)));

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_AdvSvcDesc(
	IN  const P2P_MESSAGE		*msg,
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT u4Byte					*pAdvId,
	OUT u2Byte					*pConfigMethod,
	OUT u1Byte					*pSvcNameLen,
	OUT const u1Byte			**ppSvcName,
	OUT u2Byte					*lenRead
	)
{
	const u1Byte				*pos = data;
	
	if(len < 4 + 2 + 1) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	*pAdvId = ReadEF4Byte(pos);
	pos += 4;

	*pConfigMethod = ReadN2H2BYTE(pos);
	pos += 2;

	*pSvcNameLen = pos[0];
	pos += 1;

	if(data + len - pos < *pSvcNameLen)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("svc name overflow: svc name len: %u, remain buf len: %u\n", *pSvcNameLen, (u4Byte)(data + len - pos)));
		return RT_STATUS_INVALID_LENGTH;
	}

	*ppSvcName = pos;
	pos += *pSvcNameLen;

	*lenRead = (u2Byte)(pos - data);

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("advId: %u (%08X), configMethod: %04X\n", *pAdvId, *pAdvId, *pConfigMethod));
	RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "service name:\n", *ppSvcName, *pSvcNameLen);

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_AdvSvc(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	const u1Byte				*pos = data;
	
	if(len < 0) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_advSvc = data;
	msg->advSvcLen = len;
	msg->advSvcNum = 0;

	while(0 < data + len - pos)
	{
		RT_STATUS				status = RT_STATUS_SUCCESS;
		u4Byte					advId = 0;
		u2Byte					configMethod = 0;
		u1Byte					svcNameLen = 0;
		const u1Byte			*pSvcName = NULL;
		u2Byte					lenRead = 0;

		if(RT_STATUS_SUCCESS != (status = p2p_parse_AdvSvcDesc(msg, (u2Byte)(data + len - pos), pos, &advId, &configMethod, &svcNameLen, &pSvcName, &lenRead)))
			return status;
		pos += lenRead;
		msg->advSvcNum++;
	}

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_SessionId(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	const u1Byte				*pos = data;
	
	if(len < 4 + 6) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_sessionId = data;

	msg->sessionId = ReadEF4Byte(pos);
	pos += 4;

	msg->sessionMac = pos;

	//RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("sessionId: %u (%08X), sessionMac: " MACSTR "\n", msg->sessionId, msg->sessionId, MAC2STR(msg->sessionMac)));

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_FeatureCap(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg	
	)
{
	if(len < 2) 
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length: %u\n", len));
		return RT_STATUS_INVALID_LENGTH;
	}

	msg->_featureCap = data;
	msg->featureCap = ReadEF2Byte(data);

	RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("connCap: 0x%04X\n", msg->featureCap));

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_Attributes(
	IN  u1Byte					id,
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg 
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	
	switch(id)
	{
	default:
		break;
		
	case P2P_ATTR_STATUS:
		status = p2p_parse_Status(len, data, msg); break;
		
	case P2P_ATTR_MINOR_REASON_CODE:
		status = p2p_parse_MinorReasonCode(len, data, msg); break;
		
	case P2P_ATTR_CAPABILITY:
		status = p2p_parse_Capability(len, data, msg); break;
		
	case P2P_ATTR_DEVICE_ID:
		status = p2p_parse_DevId(len, data, msg); break;
		
	case P2P_ATTR_GROUP_OWNER_INTENT:
		status = p2p_parse_GoIntent(len, data, msg); break;
		
	case P2P_ATTR_CONFIGURATION_TIMEOUT:
		status = p2p_parse_ConfigTimeout(len, data, msg); break;
		
	case P2P_ATTR_LISTEN_CHANNEL:
		status = p2p_parse_ListenChannel(len, data, msg); break;
		
	case P2P_ATTR_GROUP_BSSID:
		status = p2p_parse_GrpBssid(len, data, msg); break;
		
	case P2P_ATTR_EXT_LISTEN_TIMING:
		status = p2p_parse_ExtListenTiming(len, data, msg); break;
		
	case P2P_ATTR_INTENDED_INTERFACE_ADDRESS:
		status = p2p_parse_IntendedIntfAddr(len, data, msg); break;
		
	case P2P_ATTR_MANAGEABILITY:
		status = p2p_parse_Manageability(len, data, msg); break;
		
	case P2P_ATTR_CHANNEL_LIST:
		status = p2p_parse_ChannelList(len, data, msg); break;
		
	case P2P_ATTR_NOTICE_OF_ABSENCE:
		status = p2p_parse_NoA(len, data, msg); break;
		
	case P2P_ATTR_DEVICE_INFO:
		status = p2p_parse_DevInfo(len, data, msg); break;
		
	case P2P_ATTR_GROUP_INFO:
		status = p2p_parse_GrpInfo(len, data, msg); break;
		
	case P2P_ATTR_GROUP_ID:
		status = p2p_parse_GrpId(len, data, msg); break;
		
	case P2P_ATTR_INTERFACE:
		status = p2p_parse_Interface(len, data, msg); break;
		
	case P2P_ATTR_OP_CHANNEL:
		status = p2p_parse_OpChannel(len, data, msg); break;
		
	case P2P_ATTR_INVITATION_FLAGS:
		status = p2p_parse_InvitationFlags(len, data, msg); break;

	case P2P_ATTR_SVC_HASH:
		status = p2p_parse_SvcHash(len, data, msg); break;
		
	case P2P_ATTR_SESSION_INFO_DATA_INFO:
		status = p2p_parse_SessionInfoDataInfo(len, data, msg); break;
		
	case P2P_ATTR_CONN_CAP_INFO:
		status = p2p_parse_ConnCap(len, data, msg); break;
		
	case P2P_ATTR_ADV_ID_INFO:
		status = p2p_parse_AdvId(len, data, msg); break;
		
	case P2P_ATTR_ADV_SVC_INFO:
		status = p2p_parse_AdvSvc(len, data, msg); break;
		
	case P2P_ATTR_SESSION_ID_INFO:
		status = p2p_parse_SessionId(len, data, msg); break;
		
	case P2P_ATTR_FEATURE_CAP_INFO:
		status = p2p_parse_FeatureCap(len, data, msg); break;
	}

	if(RT_STATUS_SUCCESS != status)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("parse attr id %u failed, status: 0x%X, attr content:\n", id, status));
		RT_PRINT_DATA(COMP_P2P, DBG_WARNING, __FUNCTION__, data, len);
	}
	
	return status;
}

static
RT_STATUS
p2p_parse_WpsAttributes(
	IN  u2Byte					type,
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_MESSAGE				*msg 
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	WPS_ATTRIBUTE				wpsAttr = (WPS_ATTRIBUTE)type;
	const u1Byte				*pos = data;
	const u1Byte				*end = pos + len;

	switch(type)
	{
	default: 
		break;

	case ATTR_CONFIG_METHODS:
		if(len < 2)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length for config methods: %u\n", len));
			return RT_STATUS_INVALID_LENGTH;
		}
		msg->_wpsConfigMethods = data;
		msg->wpsConfigMethods = ReadN2H2BYTE(data);
		RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("config Method: 0x%04X\n", msg->wpsConfigMethods));
		break;

	case ATTR_DEV_NAME:
		if(32 < len)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length for device name: %u\n", len));
			return RT_STATUS_INVALID_LENGTH;
		}
		msg->_wpsDevName = data;
		msg->wpsDevNameLen = len;
		RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("Device Name:\n"));
		RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "", msg->_wpsDevName, msg->wpsDevNameLen);
		break;
		
	case ATTR_DEV_PASSWORD_ID:
		if(len < 2)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length for dev password id: %u\n", len));
			return RT_STATUS_INVALID_LENGTH;
		}
		msg->_wpsDevPasswordId = data;
		msg->wpsDevPasswordId = ReadN2H2BYTE(data);
		RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("dev password id: 0x%04X\n", msg->wpsDevPasswordId));
		break;
	
	case ATTR_MANUFACTURER:
		if(64 < len)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length for manufacturer: %u\n", len));
			return RT_STATUS_INVALID_LENGTH;
		}
		msg->_wpsManufacturer = data;
		msg->wpsManufacturerLen = len;
		RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("Manufacturer:\n"));
		RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "", msg->_wpsManufacturer, msg->wpsManufacturerLen);
		break;
	
	case ATTR_MODEL_NAME:
		if(32 < len)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length for model name: %u\n", len));
			return RT_STATUS_INVALID_LENGTH;
		}
		msg->_wpsModelName = data;
		msg->wpsModelNameLen = len;
		RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("Model Name:\n"));
		RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "", msg->_wpsModelName, msg->wpsModelNameLen);
		break;
		
	case ATTR_MODEL_NUMBER:
		if(32 < len)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length for model number: %u\n", len));
			return RT_STATUS_INVALID_LENGTH;
		}
		msg->_wpsModelNumber = data;
		msg->wpsModelNumberLen = len;
		RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("Model Number:\n"));
		RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "", msg->_wpsModelNumber, msg->wpsModelNumberLen);
		break;
	
	case ATTR_SERIAL_NUMBER:
		if(32 < len)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length for serial number: %u\n", len));
			return RT_STATUS_INVALID_LENGTH;
		}
		msg->_wpsSerialNumber = data;
		msg->wpsSerialNumberLen = len;
		RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("Serial Number:\n"));
		RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "", msg->_wpsSerialNumber, msg->wpsSerialNumberLen);
		break;
	
	case ATTR_PRIMARY_DEV_TYPE:
		if(len < 8)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length for primary device type: %u\n", len));
			return RT_STATUS_INVALID_LENGTH;
		}
		msg->_wpsPrimaryDevType = data;
		msg->wpsPrimaryDevTypeLen = len;
		RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("PrimDevType(id-oui-subid): %u-0x%08X-%u\n",
			ReadN2H2BYTE(msg->_wpsPrimaryDevType + 0), 
			ReadN2H4BYTE(msg->_wpsPrimaryDevType + 2), 
			ReadN2H2BYTE(msg->_wpsPrimaryDevType + 6)));
		break;
		
	case ATTR_SECONDARY_DEV_TYPE_LIST:
		msg->_wpsSecDevTypeList = data;
		msg->wpsSecDevTypeListLen = len;
		msg->wpsSecDevTypeNum = 0;
		
		while(pos + 8 <= end)
		{
			RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("SecDevType(id-oui-subid)[%u]: %u-0x%08X-%u\n",
				msg->wpsSecDevTypeNum,
				ReadN2H2BYTE(pos + 0), 
				ReadN2H4BYTE(pos + 2), 
				ReadN2H2BYTE(pos + 6)));
			msg->wpsSecDevTypeNum++;

			pos += 8;
		}
		break;

	case ATTR_UUID_E:
		if(16 < len)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid length for UUID-E: %u\n", len));
			return RT_STATUS_INVALID_LENGTH;
		}
		msg->_wpsUuidE = data;
		msg->wpsUuidELen = len;
		RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "UUID-E:\n", msg->_wpsUuidE, msg->wpsUuidELen);
		break;
	}

	return status;
}

static
u1Byte
p2p_parse_WpsDevType(
	IN  const u1Byte			*pType,
	OUT u2Byte					*catId,
	OUT u1Byte					*oui,
	OUT u2Byte					*subCatId
	)
{
	*catId = ReadN2H2BYTE(pType);
	p2p_MoveMemory(oui, pType + 2, 4);
	*subCatId = ReadN2H2BYTE(pType + 2 + 4);

	return 2 + 4 + 2;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------
VOID
p2p_parse_VendorIEConcat(
	IN  const u1Byte			*ies,
	IN  u2Byte					iesLen,
	IN  OUI_TYPE				ouiType,
	OUT FRAME_BUF				*pBuf
	)
{
	OCTET_STRING				osIEs;
	OCTET_STRING				osie;
	const u1Byte				*end, *pos;				

	pos = ies;
	end = ies + iesLen;

	FillOctetString(osIEs, pos, (u2Byte)(end - pos));

	// Get the payload of the IE, ID and length is not included
	osie = IEGetElement(osIEs, EID_Vendor, ouiType, OUI_SUBTYPE_DONT_CARE);

	while(osie.Length)
	{
		if(4 <= osie.Length)
		{// for case length equals to 4, FrameBuf_Add_Data will simply copies nothing 
			// Copy P2P Attributes, skip OUI (3) and OUI type (1)
			FrameBuf_Add_Data(pBuf, osie.Octet + 4, osie.Length - 4);
		}
		else
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("concatenating invalid ie (len: %u)\n", osie.Length));
			RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "ie:\n", osie.Octet, osie.Length);
		}

		pos = osie.Octet + osie.Length;
		
		FillOctetString(osIEs, pos, (u2Byte)(end - pos));

		osie = IEGetElement(osIEs, EID_Vendor, ouiType, OUI_SUBTYPE_DONT_CARE);
	}
	
	return;
}

VOID
p2p_parse_AssembleIe(
	IN  u1Byte					*pMpdu,
	IN  u2Byte					mpduLen,
	IN  OUI_TYPE				ouiType,
	OUT FRAME_BUF				*pBuf
	)
{
	OCTET_STRING				osMpdu;
	u2Byte						ieOffset = 0;
	u1Byte						*buf = NULL;
	u2Byte						allocSize = mpduLen;

	FillOctetString(osMpdu, pMpdu, mpduLen);

	if(!PacketGetIeOffset(&osMpdu, &ieOffset))
	{
		return;
	}

	if(osMpdu.Length < ieOffset)
	{// make sure osMpdu.Length - ieOffset won't underflow
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("rx frame with invalid len: %u", osMpdu.Length));
		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "frame:\n", osMpdu.Octet, osMpdu.Length);
		return;
	}

	PlatformAllocateMemory(NULL, &buf, allocSize);

	if(!buf)
	{
		return;
	}

	FrameBuf_Init(allocSize, 0, buf, pBuf);

	p2p_parse_VendorIEConcat(osMpdu.Octet + ieOffset, osMpdu.Length - ieOffset, ouiType, pBuf);

	return;
}

VOID
p2p_parse_FreeAssembledIe(
	IN  FRAME_BUF				*pBuf
	)
{
	PlatformFreeMemory(FrameBuf_MHead(pBuf), FrameBuf_Cap(pBuf));

	return;
}

VOID
p2p_parse_FreeMessage(
	IN  P2P_MESSAGE				*msg
	)
{
	if(FrameBuf_Head(&msg->wpsAttributes))
		p2p_parse_FreeAssembledIe(&msg->wpsAttributes);
	FrameBuf_Init(0, 0, NULL, &msg->wpsAttributes);

	if(FrameBuf_Head(&msg->p2pAttributes))
		p2p_parse_FreeAssembledIe(&msg->p2pAttributes);
	FrameBuf_Init(0, 0, NULL, &msg->p2pAttributes);
}

RT_STATUS
p2p_parse_ValidateAttribute(
	IN  const u1Byte			*pos,
	IN  const u1Byte			*end,
	OUT u2Byte					*len
	)
{
	u2Byte					attrLen = 0;
	
	// Check if we can read the length field
	if(end < pos + 3)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("rx invalid P2P attr: malformed attribute: can't read length field\n"));
		return RT_STATUS_MALFORMED_PKT;
	}

	// Read attr len
	attrLen = ReadEF2Byte(pos + 1);

	// Check if the attribute actually contains enough length as declared in its length field
	if(end < pos + 3 + attrLen)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("rx invalid attr: invalid length field value (%u)\n", attrLen));
		return RT_STATUS_MALFORMED_PKT;
	}

	*len = attrLen;

	return RT_STATUS_SUCCESS;
}

static
RT_STATUS
p2p_parse_P2PIe(
	IN  const FRAME_BUF			*pAttr,		// a series of P2P attributes
	OUT P2P_MESSAGE				*msg 
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	const u1Byte				*pos = FrameBuf_Head(pAttr);
	const u1Byte				*end = pos + FrameBuf_Length(pAttr);

	while(pos < end)
	{
		u2Byte					attrLen = 0;
		
		// Check if we can read the length field
		if(end < pos + 3)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("rx invalid P2P attr: malformed attribute: can't read length field\n"));
			FrameBuf_DumpFrom(pAttr, pos, 0, DBG_WARNING, __FUNCTION__);
			status = RT_STATUS_MALFORMED_PKT;
			break;
		}

		// Read attr len
		attrLen = ReadEF2Byte(pos + 1);

		// Check if the attribute actually contains enough length as declared in its length field
		if(end < pos + 3 + attrLen)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("rx invalid attr: invalid length field value (%u)\n", attrLen));
			FrameBuf_DumpFrom(pAttr, pos, 0, DBG_WARNING, __FUNCTION__);
			status = RT_STATUS_MALFORMED_PKT;
			break;
		}

		// Parse the attribute
		if(RT_STATUS_SUCCESS != (status = p2p_parse_Attributes(pos[0], attrLen, pos + 3, msg)))
		{
			break;
		}

		// Parse next attribute
		pos += 3 + attrLen;
	}
	
	return status;
}

RT_STATUS
p2p_parse_WpsIe(
	IN  const FRAME_BUF			*pAttr,
	OUT P2P_MESSAGE				*msg 
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	const u1Byte				*pos = FrameBuf_Head(pAttr);
	const u1Byte				*end = pos + FrameBuf_Length(pAttr);

	while(pos < end)
	{
		u2Byte					type = 0;
		u2Byte					len = 0;
		
		// Check if we can read the length field
		if(end < pos + 4)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("rx invalid WPS attr: malformed attribute: can't read length field\n"));
			FrameBuf_DumpFrom(pAttr, pos, 0, DBG_WARNING, __FUNCTION__);
			status = RT_STATUS_MALFORMED_PKT;
			break;
		}

		// Read type
		type = ReadN2H2BYTE(pos);
		pos += 2;

		// Read len
		len = ReadN2H2BYTE(pos);
		pos += 2;

		// Check if the attribute actually contains enough length as declared in its length field
		if(end < pos + len)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("rx invalid WPS attr: invalid length field value (%u)\n", len));
			FrameBuf_DumpFrom(pAttr, pos, 0, DBG_WARNING, __FUNCTION__);
			status = RT_STATUS_MALFORMED_PKT;
			break;
		}

		// Parse the attribute
		if(RT_STATUS_SUCCESS != (status = p2p_parse_WpsAttributes(type, len, pos, msg)))
		{
			break;
		}

		// Parse next attribute
		pos += len;
	}
	
	return status;
}


RT_STATUS
p2p_parse_Ies(
	IN  const OCTET_STRING		*posMpdu,
	IN  u4Byte					dbgLevel,
	OUT P2P_MESSAGE				*msg 
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	OCTET_STRING				os;

	PlatformZeroMemory(msg, sizeof(*msg));

	msg->dbgLevel = dbgLevel;

	/* MAC header */
	cpMacAddr(msg->da, Frame_pDaddr(*posMpdu));
	cpMacAddr(msg->sa, Frame_pSaddr(*posMpdu));
	cpMacAddr(msg->bssid, Frame_pBssid(*posMpdu));

	do
	{
		/* WPS IE */
		p2p_parse_AssembleIe(posMpdu->Octet, posMpdu->Length, OUI_SUB_SimpleConfig, &msg->wpsAttributes);
		
		if(FrameBuf_Length(&msg->wpsAttributes))
		{
			if(RT_STATUS_SUCCESS != (status = p2p_parse_WpsIe(&msg->wpsAttributes, msg)))
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("failed to parse WPS IE\n"));
				FrameBuf_Dump(&msg->wpsAttributes, 0, DBG_WARNING, __FUNCTION__);
				break;
			}
		}
		
		/* P2P IE */
		p2p_parse_AssembleIe(posMpdu->Octet, posMpdu->Length, OUI_SUB_WIFI_DIRECT, &msg->p2pAttributes);
		
		if(FrameBuf_Length(&msg->p2pAttributes))
		{
			if(RT_STATUS_SUCCESS == (status = p2p_parse_P2PIe(&msg->p2pAttributes, msg)))
			{
				if(msg->_devInfo)
					msg->devAddr = FrameBuf_MHead(&msg->p2pAttributes) + (msg->devInfoDevAddr - FrameBuf_Head(&msg->p2pAttributes));
				else if(msg->_capability && TEST_FLAG(msg->grpCap, P2P_GROUP_CAP_GROUP_OWNER) // go
					&& msg->_devId
					)
				{// note that dev id in probe req specifies the target dev to search, not its own dev addr
					msg->devAddr = FrameBuf_MHead(&msg->p2pAttributes) + (msg->devIdDevAddr - FrameBuf_Head(&msg->p2pAttributes));
				}
				else
					msg->devAddr = msg->sa;
			}
			else
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("failed to parse P2P IE\n"));
				FrameBuf_Dump(&msg->p2pAttributes, 0, DBG_WARNING, __FUNCTION__);
				break;
			}
		}
		else
		{// note that PD rsp may not have P2P IE
			msg->devAddr = msg->sa;
		}
	}while(FALSE);
	
	if(RT_STATUS_SUCCESS != status)
	{
		p2p_parse_FreeMessage(msg);
		return status;
	}

	/* SSID */
	os = PacketGetElement(*posMpdu, EID_SsId, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);

	if(os.Length)
	{
		if(os.Length <= 32)
		{
			p2p_MoveMemory(msg->ssidBuf, os.Octet, os.Length);
		
			msg->_ssid = msg->ssidBuf;
			msg->ssidLen = (u1Byte)os.Length;
			RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("ssid:\n"));
			RT_PRINT_DATA(COMP_P2P, msg->dbgLevel, "", msg->_ssid, msg->ssidLen);
		}
		else
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid ssid len: %u\n", os.Length));
		}
	}

	/* DS Param */
	os = PacketGetElement(*posMpdu, EID_DSParms, OUI_SUB_DONT_CARE, OUI_SUBTYPE_DONT_CARE);
	
	if(os.Length)
	{
		if(os.Length == 1)
		{
			p2p_MoveMemory(msg->dsParamBuf, os.Octet, os.Length);
			
			msg->_dsParam = msg->dsParamBuf;
			msg->dsParam = os.Octet[0];
			RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("DS Param: %u\n", msg->dsParam));
		}
		else
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid ds param len: %u\n", os.Length));
		}
	}
	
	return status;
}

RT_STATUS
p2p_parse_Action(
	IN  const OCTET_STRING		*posMpdu, 
	IN  u4Byte					dbgLevel,
	OUT P2P_MESSAGE				*msg 
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;

	if(RT_STATUS_SUCCESS != (status = p2p_parse_Ies(posMpdu, dbgLevel, msg)))
	{
		return status;
	}

	msg->_dialogToken = msg->dialogTokenBuf;
	p2p_MoveMemory(msg->dialogTokenBuf, posMpdu->Octet + sMacHdrLng + 1 + 1 + 3 + 1 + 1, 1);
	msg->dialogToken = ReadEF1Byte(msg->_dialogToken);

	if(msg->_dialogToken)
	{
		RT_TRACE_F(COMP_P2P, msg->dbgLevel, ("dialog token: %u\n", msg->dialogToken));
	}
	
	return status;
}

RT_STATUS
p2p_parse_UpdateDevDesc(
	IN	const u1Byte			*pHdr,
	IN  const P2P_MESSAGE		*msg,
	IN  u1Byte					sigStrength,
	OUT P2P_DEVICE_DISCRIPTOR	*pDesc
	)
{
	u4Byte						it = 0;
	RT_STATUS					status = RT_STATUS_SUCCESS;

	if(msg->_wpsUuidE)
		p2p_MoveMemory(pDesc->WPS_UUID, msg->_wpsUuidE, msg->wpsUuidELen);

	if(msg->_capability)
	{
		pDesc->Role = TEST_FLAG(msg->grpCap, P2P_GROUP_CAP_GROUP_OWNER) ? (P2P_GO) : (P2P_DEVICE);
		pDesc->DeviceCapability = msg->devCap;
		pDesc->GroupCapability = msg->grpCap;
	}
	
	cpMacAddr(pDesc->DeviceAddress, msg->devAddr);

	if(msg->_wpsConfigMethods)
		pDesc->WpsAttributes.ConfigMethod = msg->wpsConfigMethods;
	
	if(msg->_wpsDevPasswordId)
		pDesc->WpsAttributes.DevicePasswdId = msg->wpsDevPasswordId;

	if(msg->_devInfo)
	{
		P2P_WPS_ATTRIBUTES		*w = &pDesc->WpsAttributes;

		p2p_parse_WpsDevType(msg->primDevType, 
			&w->PrimaryDeviceType.CategoryId, 
			w->PrimaryDeviceType.Oui, 
			&w->PrimaryDeviceType.SubCategoryId);
		
		w->SecondaryDeviceTypeLength = msg->numSecDevType;
		for(it = 0; it < msg->numSecDevType; it++)
		{
			P2P_WPS_ATTRIBUTES_DEVICE_TYPE *t = w->SecondaryDeviceTypeList + it;

			p2p_parse_WpsDevType(msg->secDevTypeList + it * 6, 
				&t->CategoryId, 
				t->Oui, 
				&t->SubCategoryId);
		}
		w->DeviceNameLength = msg->devNameLen;
		p2p_MoveMemory(w->DeviceName, msg->devName, msg->devNameLen);
	}

	if(msg->_listenChannel)
	{
		pDesc->ListenChannel = msg->listenChannel;
		p2p_MoveMemory(pDesc->CountryString, msg->_listenChannel, 3);
		pDesc->RegulatoryClass = msg->_listenChannel[3];
	}

	if(msg->_opChannel)
	{
		pDesc->OperatingChannel = msg->opChannel;
		p2p_MoveMemory(pDesc->CountryString, msg->_opChannel, 3);
		pDesc->RegulatoryClass = msg->_opChannel[3];
	}

	if(msg->_channelList)
	{
		const u1Byte *pos = msg->channelEntryList;
		const u1Byte *end = msg->channelEntryList + msg->channelEntryListLen;

		pDesc->ChannelPlanLength = 0;
		PlatformZeroMemory(pDesc->ChannelPlanChannel, sizeof(pDesc->ChannelPlanChannel));

		while(pos < end)
		{
			u1Byte itChnl = 0;
			u1Byte nChnls = 0;
			u1Byte opClass = 0;
			
			if(end <= pos + 2) break;

			opClass = pos[0];
			pos += 1;

			nChnls = pos[0];
			pos += 1;

			if(end < pos + nChnls) break;

			for(itChnl = 0; itChnl < nChnls; itChnl++)
			{
				if(sizeof(pDesc->ChannelPlanChannel) <= pDesc->ChannelPlanLength)
				{
					// Prefast warning C6328: Size mismatch ignore
#pragma warning (disable: 6328)
					RT_TRACE_F(COMP_P2P, DBG_WARNING, 
						("Unable to record all the channels in the channel entry list, max is %u, discarding channel: %u\n", 
						sizeof(pDesc->ChannelPlanChannel), pos[itChnl]));
					continue;
				}

				if(DFS_5G_RADAR_CHANNEL(pos[itChnl])) 
				{
					//RT_TRACE_F(COMP_P2P, DBG_WARNING, 
						//("Discarding radar channel channel: %u\n", 
						//sizeof(pDesc->ChannelPlanChannel), pos[itChnl]));
					continue;
				}
				
				pDesc->ChannelPlanChannel[pDesc->ChannelPlanLength] = pos[itChnl];
				pDesc->ChannelPlanLength++;
			}

			pos += nChnls;
		}
	}

	if(msg->_grpInfo && TEST_FLAG(msg->grpCap, P2P_GROUP_CAP_GROUP_OWNER))
	{
		const u1Byte *pos = msg->cliInfoDescList;
		const u1Byte *end = msg->cliInfoDescList + msg->cliInfoDescListLen;
		
		pDesc->P2PClientDescriptorListLength = 0;
		PlatformZeroMemory(pDesc->P2PClientDescriptorList, sizeof(pDesc->P2PClientDescriptorList));

		while(pos < end)
		{
			u1Byte 							n = pDesc->P2PClientDescriptorListLength;
			P2P_CLIENT_INFO_DISCRIPTOR 		*p = pDesc->P2PClientDescriptorList + n;
			P2P_WPS_ATTRIBUTES				*w = &p->WpsAttributes;
			u1Byte							len = 0;
			const u1Byte 					*infoend = NULL;

			len = pos[0];
			pos += 1;
			
			infoend = pos + len;
			if(end < infoend)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("client info length field overflow: %u\n", len));
				break;
			}

			if(len < 6 + 6 + 1 + 2 + 8 + 1)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid client info length field: %u\n", len));
				break;
			}

			cpMacAddr(p->DeviceAddress, pos);
			pos += 6;

			cpMacAddr(p->InterfaceAddress, pos);
			pos += 6;

			p->DeviceCapability = pos[0];
			pos += 1;

			w->ConfigMethod = ReadN2H2BYTE(pos);
			pos += 2;

			pos += p2p_parse_WpsDevType(pos, 
				&w->PrimaryDeviceType.CategoryId, 
				w->PrimaryDeviceType.Oui, 
				&w->PrimaryDeviceType.SubCategoryId);

			w->SecondaryDeviceTypeLength = pos[0];
			pos += 1;

			if(infoend < pos + w->SecondaryDeviceTypeLength * 8)
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("client info sec dev type list overflow\n"));
				break;
			}

			for(it = 0; it < w->SecondaryDeviceTypeLength; it++)
			{
				P2P_WPS_ATTRIBUTES_DEVICE_TYPE *t = w->SecondaryDeviceTypeList + it;

				pos += p2p_parse_WpsDevType(pos, 
					&t->CategoryId, 
					t->Oui, 
					&t->SubCategoryId);
			}

			w->DeviceNameLength = (u1Byte)(infoend - pos);
			p2p_MoveMemory(w->DeviceName, pos, w->DeviceNameLength);
			pos += w->DeviceNameLength;
		}
	}

	if(msg->_grpId)
	{
		if(0 == pDesc->DeviceAddress[0]
			&& 0 == pDesc->DeviceAddress[1]
			&& 0 == pDesc->DeviceAddress[2]
			&& 0 == pDesc->DeviceAddress[3]
			&& 0 == pDesc->DeviceAddress[4]
			&& 0 == pDesc->DeviceAddress[5]
			)
		{
			cpMacAddr(pDesc->DeviceAddress, msg->grpDevAddr);
		}
		CopySsid(pDesc->SsidBuf, pDesc->SsidLen, msg->grpSsid, msg->grpSsidLen);
	}

	if(msg->_ssid)
	{
		pDesc->SsidLen = msg->ssidLen;
		p2p_MoveMemory(pDesc->SsidBuf, msg->_ssid, msg->ssidLen);
	}

	if(msg->_goIntent)
		pDesc->GOIntent = msg->goIntent;

	if(msg->_configTimeout)
	{
		pDesc->GOConfigurationTimeout = msg->goConfigTimeout;
		pDesc->ClientConfigurationTimeout = msg->cliConfigTimeout;
	}

	if(msg->_dialogToken)
		pDesc->DialogToken = msg->dialogToken;
	
	if(msg->_grpBssid)
		cpMacAddr(pDesc->GroupBssid, msg->grpBssid);

	if(msg->_intendedIntfAddr)
		cpMacAddr(pDesc->IntendedP2PInterfaceAddress, msg->intfAddr);

	if(msg->_status)
		pDesc->Status = msg->status;

	if(msg->_minorReasonCode)
		pDesc->MinorReasonCode = msg->minorReasonCode;

	if(msg->_extListenTiming)
	{	
		pDesc->ExtendedListenTimingDuration = msg->extListenInterval;
		pDesc->ExtendedListenTimingPeriod = msg->extListenPeriod;
	}

	if(IsMgntBeacon(pHdr) || IsMgntProbeRsp(pHdr))
	{
		// Update only if the signal strength is changed over 5%.
		if(0 == pDesc->SignalStrength ||
			(sigStrength > pDesc->SignalStrength + 5) || 
			(pDesc->SignalStrength >= 5 &&  sigStrength < pDesc->SignalStrength - 5))
		{
			pDesc->SignalStrength = sigStrength;
		}
	}		

	if(msg->_wpsManufacturer)
	{
		PlatformZeroMemory(pDesc->manufacturerName, sizeof(pDesc->manufacturerName));
		p2p_MoveMemory(pDesc->manufacturerName, msg->_wpsManufacturer, msg->wpsManufacturerLen);

	}

	if(IsMgntBeacon(pHdr)) // from P2P_OnBeacon
	{
		// Get Operating Channel, parse DS Parameter for the channel
		if(msg->_dsParam) 
			pDesc->OperatingChannel = msg->dsParam;
		
		cpMacAddr(pDesc->IntendedP2PInterfaceAddress, msg->sa);
	}

	if(IsMgntProbeRsp(pHdr)) // from P2P_OnProbeRsp
	{
		if(P2P_GO == pDesc->Role)
			cpMacAddr(pDesc->IntendedP2PInterfaceAddress, msg->sa);

		if(msg->_dsParam)
		{// Get Listen/Operating Channel, parse DS Parameter for the channel
			if(pDesc->Role == P2P_GO) 
				pDesc->OperatingChannel = msg->dsParam;
			else if(pDesc->Role == P2P_DEVICE)
				pDesc->ListenChannel= msg->dsParam;
		}
	}

	if(IsMgntAsocReq(pHdr)) // from P2P_OnAssocReqAccept
	{	
		cpMacAddr(pDesc->IntendedP2PInterfaceAddress, msg->sa);
	}

	return status;
}

RT_STATUS
p2p_parse_ChannelEntryList(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_CHANNELS			*channels	
	)
{
	RT_STATUS					status = RT_STATUS_SUCCESS;
	const u1Byte				*pos = data;
	const u1Byte				*end = data + len;

	while(pos < end)
	{
		if(data + len - pos < 2) 
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("no valid channel entry list, remain len is: %u\n", (u4Byte)(data + len - pos)));
			status = RT_STATUS_INVALID_LENGTH;
			break;
		}

		if(data + len - pos < 2 + pos[1])
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, 
				("no valid channel entry list, #chanels is: %u, remain len is: %u\n", pos[1], (u4Byte)(data + len - pos)));
			status = RT_STATUS_INVALID_LENGTH;
			break;
		}

		p2p_Channel_Add(channels, pos[0], pos[1], pos + 2);

		pos += (2 + pos[1]);
	}

	return RT_STATUS_SUCCESS;
}

#endif
