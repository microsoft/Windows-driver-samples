//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_P2P_INTERNAL_H
#define __INC_P2P_INTERNAL_H

#include "P2P_ProtocolSpec.h"
#include "P2P_Attribute.h"
#include "WPS_Def.h"

#include "P2P_DevList.h"
#include "P2P_Indication.h"

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

//---------------------------------------------------------------------------
// P2P_Util
//---------------------------------------------------------------------------
VOID
p2p_MoveMemory(
	IN  VOID				*dest,
	IN  const VOID			*src,
	IN  u4Byte				len
	);
	
int
p2p_go_det(
	IN  u1Byte					selfVal, 
	IN  u1Byte					peerVal
	);

BOOLEAN
p2p_ActingAs_Go(
	IN  P2P_INFO 				*pP2PInfo
	);


BOOLEAN
p2p_Doing_Provisioning(
	IN  const P2P_INFO 			*pP2PInfo
	);

BOOLEAN
p2p_Check_GroupLimitReached(
	IN  P2P_INFO 				*pP2PInfo
	);

P2P_DEV_TYPE
p2p_MessageSrcType(
	IN  const P2P_MESSAGE		*msg,
	IN  P2P_FRAME_TYPE			frameType
	);

BOOLEAN
p2p_FindClientInfoByDevAddr(
	IN  const u1Byte			*devAddr,
	IN  const P2P_MESSAGE		*msg,
	OUT const u1Byte			**ppCliInfo
	);

//---------------------------------------------------------------------------
// P2P_Build
//---------------------------------------------------------------------------

u1Byte *
p2p_add_IEHdr(
	IN  FRAME_BUF				*pBuf
	);

VOID
p2p_update_IeHdrLen(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					*pLen
	);

VOID
p2p_add_ActionFrameMacHdr(
	IN  FRAME_BUF				*pBuf,
	IN  const u1Byte			*dest,
	IN  const u1Byte			*src,
	IN  const u1Byte			*bssid
	);

VOID
p2p_add_MgntFrameMacHdr(
	IN  FRAME_BUF				*pBuf,
	IN  TYPE_SUBTYPE			typeSubtype,
	IN  const u1Byte			*dest,
	IN  const u1Byte			*src,
	IN  const u1Byte			*bssid
	);

VOID
p2p_add_PublicActionHdr(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					actionCode,
	IN  u1Byte					dialogToken
	);

//---------------------------------------------------------------------------
// P2P_Build_PublicAction
//---------------------------------------------------------------------------

VOID
p2p_add_P2PPublicActionHdr(
	IN  FRAME_BUF				*pBuf,
	IN  u1Byte					subtype,
	IN  u1Byte					dialogToken
	);
	
VOID
p2p_Construct_GoNegReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte					dialogToken
	);

VOID
p2p_Construct_GoNegRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	);

VOID
p2p_Construct_GoNegConf(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	);

VOID
p2p_Construct_InvitationReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken
	);

VOID
p2p_Construct_InvitationRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	);

VOID
p2p_Construct_DevDiscReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  const u1Byte			*bssid
	);

VOID
p2p_Construct_DevDiscRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte					status
	);

VOID
p2p_Construct_PDReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte					dialogToken,
	IN  u2Byte					configMethod
	);

VOID
p2p_Construct_PDRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  u1Byte 					dialogToken,
	IN  OCTET_STRING			*posP2PAttrs,
	IN  u2Byte					configMethod,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	);

VOID
p2p_build_FakeInvitationRspIe(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte 			*da
	);

RT_STATUS
p2p_Construct_FakePDRsp(
	IN	P2P_INFO				*pP2PInfo,
	IN  P2P_DEV_LIST_ENTRY		*pRspDev,
	IN  const u1Byte 			*da,
	IN  FRAME_BUF 				*pBuf
	);


//---------------------------------------------------------------------------
// P2P_Build_Action
//---------------------------------------------------------------------------
VOID
p2p_Construct_PresenceReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const P2P_POWERSAVE_SET	*pP2pPs,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken,
	IN  FRAME_BUF 				*pBuf
	);

VOID
p2p_Construct_PresenceRsp(
	IN  P2P_INFO				*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte					dialogToken,
	IN  u1Byte					status,
	IN  FRAME_BUF 				*pBuf
	);

VOID
p2p_Copnstruct_GoDiscoverabilityReq(
	IN  P2P_INFO				*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte					dialogToken,
	IN  FRAME_BUF 				*pBuf
	);

VOID 
p2p_Construct_SDReq(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da
	);

VOID 
p2p_Construct_SDRsp(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  P2P_SD_STATUS_CODE		status,
	IN  BOOLEAN					bFrag
	);

VOID 
p2p_Construct_SDComebackReq(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken
	);

VOID
p2p_Construct_SDComebackRsp(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  FRAME_BUF 				*pBuf,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  u2Byte					bytesToCopy,
	IN  BOOLEAN					bMoreData
	);


VOID
p2p_Construct_AnqpQueryRspField(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  P2P_SD_RSP_CONTEXT 		*pRspCtx,
	OUT P2P_SD_CONTEXT			*pSdCtx
	);

//---------------------------------------------------------------------------
// P2P_Build_Mgnt
//---------------------------------------------------------------------------

VOID
p2p_Construct_ProbeReqEx(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					ssidLen,
	IN  const u1Byte			*ssidBuf
	);

VOID
p2p_Construct_ProbeReq(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo
	);

BOOLEAN
p2p_Construct_ProbeRsp(
	IN  FRAME_BUF				*pBuf,
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  RT_RFD					*rfdProbeReq,
	IN  OCTET_STRING			*posProbeReq,
	IN  OCTET_STRING			*posAttrs
	);

//---------------------------------------------------------------------------
// P2P_SendAction: public action
//---------------------------------------------------------------------------

VOID
p2p_Send_GoNegReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  BOOLEAN					bSend
	);

VOID
p2p_Send_GoNegRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	);

VOID
p2p_Send_GoNegConfirm(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken,
	OUT PBOOLEAN 				pbSupportTxReport
	);

VOID
p2p_Send_InvitationReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da
	);

VOID
p2p_Send_InvitationRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	);

VOID
p2p_Send_DevDiscReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*bssid,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	);;

VOID
p2p_Send_DevDiscRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken,
	IN  u1Byte					status
	);

VOID
p2p_Send_PDReq(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*mac,
	IN  BOOLEAN					bGo,
	IN  u1Byte					dialogToken,
	IN  u2Byte					configMethod
	);

VOID
p2p_Send_PDRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  OCTET_STRING 			*posP2PAttrs, // TODO: make it constant
	IN  u2Byte 					configMethod,
	OUT PBOOLEAN 				pbSupportTxReport
	);

//---------------------------------------------------------------------------
// P2P_SendAction: P2P action
//---------------------------------------------------------------------------

VOID
p2p_Send_PresenceReq(
	IN  P2P_INFO				*pP2PInfo,
	IN  const P2P_POWERSAVE_SET	*pPs,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	);

VOID
p2p_Send_PresenceRsp(
	IN  P2P_INFO				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken,
	IN  u1Byte					status
	);

VOID
p2p_Send_GoDiscoverabilityReq(
	IN  P2P_INFO				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  u1Byte					dialogToken
	);

VOID
p2p_Send_SDReq(
	IN  const P2P_INFO			*pP2PInfo,
	IN  const u1Byte 			*da
	);

VOID
p2p_Send_SDRsp(
	IN  const P2P_INFO			*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  P2P_SD_STATUS_CODE		status,
	IN  BOOLEAN					bFrag
	);

VOID
p2p_Send_SDComebackReq(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken
	);

VOID
p2p_Send_SDComebackRsp(
	IN  const P2P_INFO 			*pP2PInfo,
	IN  const u1Byte 			*da,
	IN  u1Byte 					dialogToken,
	IN  u2Byte					bytesToCopy,
	IN  BOOLEAN					bMoreData
	);

//---------------------------------------------------------------------------
// P2P_SendMgnt
//---------------------------------------------------------------------------

VOID
p2p_Send_ProbeRsp(
	IN  P2P_INFO 				*pP2PInfo,
	IN  const u1Byte			*da,
	IN  RT_RFD					*rfdProbeReq,
	IN  OCTET_STRING			*posProbeReq,
	IN  OCTET_STRING			*posAttrs
	);

//---------------------------------------------------------------------------
// P2P_Parse
//---------------------------------------------------------------------------
VOID
p2p_parse_VendorIEConcat(
	IN  const u1Byte		*ies,
	IN  u2Byte				iesLen,
	IN  OUI_TYPE			ouiType,
	OUT FRAME_BUF			*pBuf
	);

VOID
p2p_parse_AssembleIe(
	IN  u1Byte					*pMpdu,
	IN  u2Byte					mpduLen,
	IN  OUI_TYPE				ouiType,
	OUT FRAME_BUF				*pBuf
	);

VOID
p2p_parse_FreeAssembledIe(
	IN  FRAME_BUF				*pBuf
	);

VOID
p2p_parse_FreeMessage(
	IN  P2P_MESSAGE				*msg
	);

RT_STATUS
p2p_parse_ValidateAttribute(
	IN  const u1Byte			*pos,
	IN  const u1Byte			*end,
	OUT u2Byte					*len
	);

RT_STATUS
p2p_parse_P2PIe(
	IN  const FRAME_BUF			*pAttr,		// a series of P2P attributes
	OUT P2P_MESSAGE				*msg 
	);

RT_STATUS
p2p_parse_WpsIe(
	IN  const FRAME_BUF			*pAttr,
	OUT P2P_MESSAGE				*msg 
	);

RT_STATUS
p2p_parse_Ies(
	IN  const OCTET_STRING		*posMpdu, 
	IN  u4Byte					dbgLevel,
	OUT P2P_MESSAGE				*msg 
	);

RT_STATUS
p2p_parse_Action(
	IN  const OCTET_STRING		*posMpdu, 
	IN  u4Byte					dbgLevel,
	OUT P2P_MESSAGE				*msg 
	);

RT_STATUS
p2p_parse_UpdateDevDesc(
	IN	const u1Byte			*pHdr,
	IN  const P2P_MESSAGE		*msg,
	IN  u1Byte					sigStrength,
	OUT P2P_DEVICE_DISCRIPTOR	*pDesc
	);

RT_STATUS
p2p_parse_ChannelEntryList(
	IN  u2Byte					len,
	IN  const u1Byte			*data,
	OUT P2P_CHANNELS			*channels	
	);
	
//---------------------------------------------------------------------------
// P2P_Channel
//---------------------------------------------------------------------------

BOOLEAN
p2p_Channel_Add(
	IN  P2P_CHANNELS			*channels,
	IN  u1Byte					regClass,
	IN  u1Byte					nChannels,
	IN  const u1Byte			*channelList
	);

VOID
p2p_Channel_Reset(
	IN  P2P_CHANNELS			*channels
	);

VOID
p2p_Channel_Intersect(
	IN  const P2P_CHANNELS		*a,
	IN  const P2P_CHANNELS		*b,
	OUT P2P_CHANNELS			*res
	);
	
BOOLEAN
p2p_Channel_ChannelListAttrToChannels(
	IN  u2Byte					chnlListAttrLen,
	IN  const u1Byte			*chnlListAttr,
	OUT P2P_CHANNELS			*pChannels
	);

BOOLEAN
p2p_Channel_InChannelEntryList(
	IN  u1Byte					channel,
	IN  const P2P_CHANNELS		*pChannels
	);

#endif	// #ifndef __INC_P2P_INTERNAL_H
