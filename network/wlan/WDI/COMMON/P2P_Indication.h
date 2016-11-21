//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_P2P_INDICATION_H
#define __INC_P2P_INDICATION_H

VOID
p2p_IndicateActionFrameSendComplete(
	IN  P2P_INFO				*pP2PInfo,
	IN  u4Byte					eventId,
	IN  RT_STATUS				rtStatus,
	IN  u1Byte					*pFrameBuf,
	IN  u4Byte					frameLen
	);

VOID
p2p_IndicateActionFrameSendCompleteWithToken(
	IN  P2P_INFO				*pP2PInfo,
	IN  u4Byte					eventId,
	IN  RT_STATUS				rtStatus,
	IN  u1Byte					*pFrameBuf,
	IN  u4Byte					frameLen,
	IN  u1Byte					token
	);

VOID
p2p_IndicateActionFrameReceived(
	IN  P2P_INFO				*pP2PInfo,
	IN  u4Byte					eventId,
	IN  RT_STATUS				rtStatus,
	IN  u1Byte					*pFrameBuf,
	IN  u4Byte					frameLen
	);

VOID
p2p_IndicatePdReqSent(
	IN  P2P_DEV_LIST_ENTRY		*pDev,
	IN  P2P_INFO 				*pP2PInfo
	);

VOID
p2p_IndicatePdRspReceived(
	IN  P2P_DEV_LIST_ENTRY		*pDev,
	IN  P2P_INFO 				*pP2PInfo
	);

RT_STATUS
p2p_IndicateFakePdRspReceived(
	IN  P2P_DEV_LIST_ENTRY		*pDev,
	IN  P2P_INFO 				*pP2PInfo
	);
#endif	// #ifndef __INC_P2P_INDICATION_H