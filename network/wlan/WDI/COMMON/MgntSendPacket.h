#ifndef __INC_MGNTSENDPACKET_H
#define __INC_MGNTSENDPACKET_H

#include "RmDef.h"

VOID
SendBeaconFrame(
	PADAPTER	Adapter, 
	u1Byte		QueueIndex
	);

VOID
SendProbeReq(
	PADAPTER	Adapter,
	BOOLEAN		bAnySsid,
	u2Byte		DataRate,
	BOOLEAN		bForcePowerSave
	);

VOID
SendProbeReqEx(
	PADAPTER	Adapter,
	u2Byte		DataRate,
	BOOLEAN		bForcePowerSave
	);

VOID
SendAuthenticatePacket(
	PADAPTER		Adapter,
	pu1Byte			auStaAddr,
	u1Byte			AuthAlg,
	u1Byte			AuthSeq,
	u1Byte			AuthStatusCode,
	OCTET_STRING	AuthChallengetext
	);


VOID
SendAssociateReq(
	PADAPTER	Adapter,
	pu1Byte		asocStaAddr,
	u2Byte		asocCap,
	u2Byte		asocListenInterval
	);

VOID
SendReassociateReq(
	PADAPTER	Adapter,
	pu1Byte		ReasocStaAddr,
	u2Byte		ReasocCap,
	u2Byte		asocListenInterval,	
	pu1Byte		CurrentasocStaAddr
	);

VOID
SendAssociateRsp(
	PADAPTER		Adapter,
	pu1Byte			asocStaAddr,
	u2Byte			asocCap,
	u2Byte			asocStatusCode,
	u2Byte			asocID,
	BOOLEAN			bReAssocRsp,
	BOOLEAN			bQosSTA
	);

VOID
SendDeauthentication(
	PADAPTER		Adapter,
	pu1Byte			auSta,
	u1Byte			asRsn
	);



VOID
SendDisassociation(
	PADAPTER		Adapter,
	pu1Byte			asSta,
	u1Byte			asRsn
	);

VOID
SendProbeRsp(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				StaAddr,
	IN	BOOLEAN				bHideSSID,
	IN	PRT_RFD				pRfdProbeReq,
	IN	POCTET_STRING		posProbeReq
	);

VOID
SendCustomizedData(
	PADAPTER		Adapter,
	u2Byte 		pktLength,
	u1Byte 		txRate
	);


VOID
SendNullFunctionData(
	PADAPTER		Adapter,
	pu1Byte			StaAddr,
	BOOLEAN			bForcePowerSave
	);


VOID
SendPSPoll(
	PADAPTER		Adapter
	);

VOID
SendMagicPacket(
	PADAPTER		Adapter,
	pu1Byte			StaAddr
	);

VOID
SendEapolKeyPacket(
	PADAPTER		Adapter,
	pu1Byte			StaAddr,

	pu1Byte			pKCK, // Pointer to KCK (EAPOL-Key Confirmation Key).
	pu1Byte			pKEK, // Pointer to KEK (EAPOL-Key Encryption Key).

	KeyType			eKeyType, // EAPOL-Key Information field: Key Type bit: type_Group or type_Pairwise.
	BOOLEAN			bInstall, // EAPOL-Key Information field: Install Flag.
	BOOLEAN			bKeyAck, // EAPOL-Key Information field: Key Ack bit.
	BOOLEAN			bKeyMIC, // EAPOL-Key Information field: Key MIC bit. If true, we will calculate EAPOL MIC and fill it into Key MIC field. 
	BOOLEAN			bSecure, // EAPOL-Key Information field: Secure bit.
	BOOLEAN			bError, // EAPOL-Key Information field: Error bit. True for MIC failure report.
	BOOLEAN			bRequest, // EAPOL-Key Information field: Requst bit.
	
	u8Byte			u8bKeyReplayCounter, // EAPOL-KEY Replay Counter field.
	pu1Byte			pKeyNonce, // EAPOL-Key Key Nonce field (32-byte).
	u8Byte			u8bKeyRSC, // EAPOL-Key Key RSC field (8-byte).

	POCTET_STRING	posRSNIE, // Key Data field: Pointer to RSN IE.
	pu1Byte			pGTK // Key Data field: Pointer to GTK.
	);

BOOLEAN
MgntSendRawPacket(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pocPacketToSend,
	IN	u1Byte			u1bTxRate
	);

BOOLEAN
MgntSendSpecificPacket(
	PADAPTER		Adapter,
	POCTET_STRING	pocPacketToSend,
	u1Byte			u1bTxRate
	);

VOID
SendCcxRmReport(
	IN	PADAPTER			Adapter,
	IN	PRT_RM_REPORTS		pRmReports
	);

VOID
SendDot11kRmReport(
	IN	PADAPTER			Adapter,
	IN	PRT_RM_REQUESTS		pRmRequests,
	IN	PRT_RM_REPORTS		pRmReports
	);

VOID
SendCcxLinkTestReply(
	IN PADAPTER				Adapter,
	IN POCTET_STRING		posLinkRequest,
	IN u2Byte				FrameNumber,
	IN u1Byte				LastReplyRetryCount,
	IN u1Byte				RxPower
	);


VOID
ConstructCcxLinkTestReply(
	IN PADAPTER				Adapter,
	IN POCTET_STRING		posLinkRequest,
	IN u2Byte				FrameNumber,
	IN u1Byte				LastReplyRetryCount,
	IN u1Byte				RxPower,
	OUT pu1Byte				Buffer,
	OUT pu4Byte				pLength
);

VOID
SendQosAddTs(
	IN	PADAPTER			Adapter,
	IN	PQOS_TSTREAM		pTs,
	IN	u4Byte				numTspec
	);

VOID
SendQosDelTs(
	IN	PADAPTER			Adapter,
	IN	PQOS_TSTREAM		pTs,
	IN	u4Byte				numTspec
	);

VOID
ApSendNullPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			StaAddr,
	IN	BOOLEAN			bQos,
	IN	u1Byte			AC,
	IN	BOOLEAN			bEosp
	);


VOID
SendEAPOLStarPacket(
	IN	PADAPTER			Adapter,
	IN	BOOLEAN				bEncrypt 
	);

VOID
SendSAQueryReq(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pTargetSta,
	IN	u2Byte			Identifier
	);

VOID
SendSAQueryRsp(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pTargetSta,
	IN	u2Byte			Identifier
	);

#endif // #ifndef __INC_MGNTSENDPACKET_H
