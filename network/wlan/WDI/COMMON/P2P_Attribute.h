//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_P2P_ATTRIBUTE_H
#define __INC_P2P_ATTRIBUTE_H

VOID
P2PAttr_Update_AttrHdrLen(
	IN  FRAME_BUF				*pBuf,
	IN  const u1Byte			*pLen
	);

VOID
P2PAttr_Make_Status(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte 					status
	);

VOID
P2PAttr_Make_MinorReasonCode(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte 					minorReasonCode
	);

VOID
P2PAttr_Make_Capability(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					devCap,
	IN  u1Byte					grpCap
	);

VOID
P2PAttr_Make_DevId(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			devAddr
	);

VOID
P2PAttr_Make_GoIntent(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					intent
	);

VOID 
P2PAttr_Make_ConfigTimeout(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte 					goTimeout,
	IN  u1Byte 					cliTimeout
	);

VOID
P2PAttr_Make_ListenChannel(
	IN  FRAME_BUF				*pBuf,
	IN  const char *			country,
	IN  u1Byte					regClass,
	IN  u1Byte					channel
	);

VOID 
P2PAttr_Make_GroupBssid(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			grpBssid
	);

VOID 
P2PAttr_Make_ExtListenTiming(
	IN  FRAME_BUF				*pBuf,
	IN  u2Byte					period,
	IN  u2Byte					interval
	);

VOID 
P2PAttr_Make_IntendedIntfAddr(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			intfAddr
	);

VOID
P2PAttr_Make_Manageability(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					manageability
	);

VOID
P2PAttr_Make_ChannelEntry(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					regClass,
	IN  u1Byte					nChannels,
	IN  const u1Byte			*channelList
	);

VOID
P2PAttr_Make_ChannelList(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO				*pP2PInfo,
	IN  const P2P_CHANNELS		*channels
	);

VOID 
P2PAttr_Make_Noa(
	IN  FRAME_BUF 				*pBuf,
	IN  u1Byte					noaIEIndex,
	IN  BOOLEAN 				bOppPS,
	IN  u1Byte 					ctWindow,
	IN  u1Byte					noaDescNum,
	IN  const P2P_NOA_DESCRIPTOR *noaDesc
	);


VOID
P2PAttr_Make_NoaFromPsSet(
	IN  FRAME_BUF				*pBuf,
	IN  const P2P_POWERSAVE_SET	*pPs
	);

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
	);

VOID
P2PAttr_Make_GroupInfo(
	IN  FRAME_BUF				*pBuf,
	IN  const RT_WLAN_STA		*pCliList
	);

VOID 
P2PAttr_Make_GroupId(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			grpDevAddr,
	IN  const pu1Byte			grpSsid,
	IN  u1Byte					grpSsidLen
	);

VOID 
P2PAttr_Make_Interface(
	IN  FRAME_BUF				*pBuf,
	IN  const pu1Byte			devAddr,
	IN  u1Byte					numIntfAddr,
	IN  const pu1Byte			intfAddrList
	);

VOID
P2PAttr_Make_OperatingChannel(
	IN  FRAME_BUF				*pBuf,
	IN  const char *			country,
	IN  u1Byte					regClass,
	IN  u1Byte					channel
	);

VOID 
P2PAttr_Make_InvitationFlags(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					flags
	);

#endif	// #ifndef __INC_P2P_ATTRIBUTE_H
