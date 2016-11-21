//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_P2P_ADDITIONAL_IE_H
#define __INC_P2P_ADDITIONAL_IE_H

typedef enum _P2P_ADD_IE_ID
{
	P2P_ADD_IE_BEACON = 0,
	P2P_ADD_IE_PROBE_RESPONSE,
	P2P_ADD_IE_DEFAULT_REQUEST,
	P2P_ADD_IE_PROBE_REQUEST,
	P2P_ADD_IE_PROVISION_DISCOVERY_REQUEST,
	P2P_ADD_IE_PROVISION_DISCOVERY_RESPONSE,
	P2P_ADD_IE_GO_NEGOTIATION_REQUEST,
	P2P_ADD_IE_GO_NEGOTIATION_RESPONSE,
	P2P_ADD_IE_GO_NEGOTIATION_CONFIRM,
	P2P_ADD_IE_INVITATION_REQUEST,
	P2P_ADD_IE_INVITATION_RESPONSE,
	//------------------------------------------------------------------------
	P2P_ADD_IE_MAX
}P2P_ADD_IE_ID, *PP2P_ADD_IE_ID;

typedef struct _P2P_ADD_IES
{
	FRAME_BUF					*addIe[P2P_ADD_IE_MAX];
}P2P_ADD_IES, *PP2P_ADD_IES;

VOID
P2P_AddIe_Init(
	IN  PP2P_ADD_IES			addIes
	);

VOID
P2P_AddIe_Free(
	IN  PP2P_ADD_IES 			addIes
	);

BOOLEAN
P2P_AddIe_Set(
	IN  PP2P_ADD_IES			addIes,
	IN  P2P_ADD_IE_ID			id,
	IN  u4Byte					bufLen,
	IN  u1Byte					*pBuf
	);

const FRAME_BUF *
P2P_AddIe_Get(
	IN  const PP2P_ADD_IES		addIes,
	IN  P2P_ADD_IE_ID			id
	);

u2Byte
P2P_AddIe_Append(
	IN  const PP2P_ADD_IES		addIes,
	IN  P2P_ADD_IE_ID			id,
	IN  FRAME_BUF				*pBuf
	);
	
#endif	// #ifndef __INC_P2P_ADDITIONAL_IE_H
