//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_P2P_PUBLIC_H
#define __INC_P2P_PUBLIC_H

#if (P2P_SUPPORT == 1)

//-----------------------------------------------------------------------------
// P2P_Build_Mgnt
//-----------------------------------------------------------------------------

VOID
P2P_Append_GoProbeRspIe(
	IN  OCTET_STRING			*posFrame,
	IN  ADAPTER					*pAdapter,
	IN  u1Byte					reason,
	IN  RT_RFD					*rfdProbeReq,
	IN  OCTET_STRING			*posProbeReq
	);

VOID
P2P_Append_BeaconIe(
	IN  ADAPTER					*pAdapter
	);

VOID
P2P_Append_AssociationReqIe(
	IN  OCTET_STRING			*posFrame,
	IN  ADAPTER					*pAdapter
	);

VOID
P2P_Append_AssociationRspIe(
	IN  OCTET_STRING			*posFrame,
	IN  ADAPTER					*pAdapter
	);

VOID
P2P_Append_ProbeReqIe(
	IN  OCTET_STRING			*posFrame,
	IN  P2P_INFO 				*pP2PInfo
	);

//-----------------------------------------------------------------------------
// P2P_Receive
//-----------------------------------------------------------------------------
BOOLEAN
p2p_validate_Beacon(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_ProbeReq(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_ProbeRsp(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_AssocReq(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_AssocRsp(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_Deauth(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_GoNegReq(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_GoNegRsp(
	IN  const P2P_INFO			*info,
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_GoNegConfirm(
	IN  BOOLEAN					go,
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_InvitationReq(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_InvitationRsp(
	IN  P2P_ROLE				invitorRole,
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_DevDiscReq(
	IN  P2P_MESSAGE				*msg
	);
	
BOOLEAN
p2p_validate_DevDiscRsp(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_PDReq(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_PDRsp(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_PresenceReq(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_PresenceRsp(
	IN  P2P_MESSAGE				*msg
	);

BOOLEAN
p2p_validate_GoDiscoverabilityReq(
	IN  P2P_MESSAGE				*msg
	);

#else

#define	P2P_Append_GoProbeRspIe(_posFrame, _pAdapter, _reason, _rfdProbeReq, _posProbeReq)
#define	P2P_Append_BeaconIe(_pAdapter)
#define	P2P_Append_AssociationReqIe(_posFrame, _pAdapter)
#define	P2P_Append_AssociationRspIe(_posFrame, _pAdapter)
#define	P2P_Append_ProbeReqIe(_posFrame, _pP2PInfo)

#endif
#endif	// #ifndef __INC_P2P_PUBLIC_H
