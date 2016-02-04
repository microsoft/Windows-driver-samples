//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_P2P_DEV_LIST_H
#define __INC_P2P_DEV_LIST_H

RT_STATUS
p2p_DevList_Init(
	IN  P2P_DEV_LIST			*dlist
	);

RT_STATUS
p2p_DevList_Flush(
	IN  P2P_DEV_LIST			*dlist
	);

RT_STATUS
p2p_DevList_Free(
	IN  P2P_DEV_LIST			*dlist
	);

VOID
p2p_DevList_Lock(
	IN  P2P_DEV_LIST			*dlist
	);

VOID
p2p_DevList_Unlock(
	IN  P2P_DEV_LIST			*dlist
	);

RT_STATUS
p2p_DevList_RxUpdate(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_FRAME_TYPE			frameType,
	IN  const RT_RFD			*rfd,
	IN  u1Byte					*mac,
	IN  u1Byte					rxChnl,
	OUT P2P_DEV_LIST_ENTRY 		**ppDev
	);

RT_STATUS
p2p_DevList_FlushActionFrames(
	IN  P2P_DEV_LIST			*dlist,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE			devType
	);

RT_STATUS
p2p_DevList_TxUpdate(
	IN  P2P_DEV_LIST			*dlist,
	IN  P2P_FRAME_TYPE			frameType,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE			devType,
	IN  const FRAME_BUF 		*buf,
	IN  u1Byte					token,
	IN  u1Byte					channel
	);

VOID
p2p_DevList_DialogTokenUpdate(
	IN  P2P_DEV_LIST_ENTRY 		*pDev
	);

const P2P_DEV_LIST_ENTRY *
p2p_DevList_Find(
	IN  const P2P_DEV_LIST		*dlist,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE 			type
	);

P2P_DEV_LIST_ENTRY *
p2p_DevList_Get(
	IN  P2P_DEV_LIST			*dlist,
	IN  const u1Byte			*mac,
	IN  P2P_DEV_TYPE 			type
	);

P2P_DEV_LIST_ENTRY *
p2p_DevList_GetGo(
	IN  P2P_DEV_LIST			*dlist,
	IN  const u1Byte			*devAddr
	);

RT_STATUS
p2p_DevList_Translate(
	IN  P2P_DEV_LIST			*dlist,
	IN  u4Byte					maxDevices,
	OUT u4Byte					*pNumDevCopied,
	OUT P2P_DEVICE_DISCRIPTOR	*descList
	);

VOID
p2p_DevList_TranslateDev(
	IN  const P2P_DEV_LIST_ENTRY *pDev,
	OUT P2P_DEVICE_DISCRIPTOR	*desc
	);

VOID
p2p_DevList_Dump(
	IN  P2P_DEV_LIST			*dlist
	);

VOID
p2p_DevList_DumpDev(
	IN  const P2P_DEV_LIST_ENTRY *pDev
	);

u8Byte
p2p_DevList_RxFrameElapsedTimeMs(
	IN  P2P_DEV_LIST			*dlist,
	IN  const P2P_DEV_LIST_ENTRY *pDev,
	IN  P2P_FRAME_TYPE			frameType
	);

BOOLEAN
p2p_DevList_ActionProcessed(
	IN  const OCTET_STRING		*posMpdu,
	IN  P2P_DEV_LIST			*dlist,
	IN  const P2P_DEV_LIST_ENTRY *pDev,
	IN  P2P_FRAME_TYPE			frameType
	);

u1Byte
p2p_DevList_GetDevChnl(
	IN  P2P_DEV_LIST_ENTRY		*dev
	);

#endif	// #ifndef __INC_P2P_DEV_LIST_H
