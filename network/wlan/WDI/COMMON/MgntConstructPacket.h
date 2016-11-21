#ifndef __INC_MGNTCONSTRUCTPACKET_H
#define __INC_MGNTCONSTRUCTPACKET_H

#include "1x_kmsm_eapolkey.h"
#include "RmDef.h"

VOID
SelectSupportedRatesElement(
	IN	WIRELESS_MODE	wirelessmode,
	IN	OCTET_STRING	SuppRates,
	IN	BOOLEAN		bFilterCck,
	OUT	POCTET_STRING	pSuppRates,
	OUT	POCTET_STRING	pExtSuppRates
	);


VOID
ConstructBeaconFrame(
	PADAPTER		Adapter
	);

VOID
UpdateBeaconFrame(
	PADAPTER		Adapter
	);

BOOLEAN
Isdot11acAvailable(
	PADAPTER		Adapter
	);

VOID
ConstructProbeRequest(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	BOOLEAN			bBroadcastBssid,
	BOOLEAN			bAnySsid,
	BOOLEAN			bForcePowerSave
	);

VOID
ConstructProbeRequestEx(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	OCTET_STRING	Ssid,
	BOOLEAN			bForcePowerSave
	);

VOID
ConstructAuthenticatePacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			auStaAddr,
	u1Byte			AuthAlg,
	u1Byte			AuthSeq,
	u1Byte			AuthStatusCode,
	OCTET_STRING	AuthChallengetext
	);

VOID
ConstructAssociateReq(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			asocStaAddr,
	u2Byte			asocCap,
	u2Byte			asocListenInterval,
	OCTET_STRING	asocSsid,
	OCTET_STRING	asocSuppRates
	);

VOID
ConstructReAssociateReq(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			ReasocStaAddr,
	u2Byte			ReasocCap,
	u2Byte			ReasocListenInterval,
	pu1Byte			CurrentasocStaAddr,
	OCTET_STRING	ReasocSsid,
	OCTET_STRING	ReasocSuppRates
	);

VOID
ConstructAssociateRsp(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			asocStaAddr,
	u2Byte			asocCap,
	u2Byte			asocStatusCode,
	u2Byte			asocID,
	BOOLEAN			bReAssocRsp,
	BOOLEAN			bQosSta
	);

VOID
ConstructDeauthenticatePacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			auSta,
	u1Byte			asRsn
	);

VOID
ConstructDisassociatePacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			asSta,
	u1Byte			asRsn
	);

VOID
ConstructProbeRsp(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			StaAddr,
	IN	BOOLEAN			bHideSSID,
	IN	PRT_RFD			pRfdProbeReq,
	IN	POCTET_STRING	posProbeReq
	);

VOID
ConstructCustomizedData(
	IN PADAPTER		Adapter,
	IN pu1Byte			Buffer,
	OUT pu4Byte			pLength,
	IN u2Byte			pktLength
	);

VOID
ConstructNullFunctionData(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			StaAddr,
	IN	BOOLEAN			bQoS,
	IN	u1Byte			AC,
	IN	BOOLEAN			bEosp,
	IN	BOOLEAN			bForcePowerSave
	);

VOID
ConstructBtNullFunctionData(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			StaAddr,
	IN	BOOLEAN			bQoS,
	IN	u1Byte			AC,
	IN	BOOLEAN			bEosp,
	IN	BOOLEAN			bForcePowerSave
	);

VOID
ConstructPSPoll(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength
	);

VOID
ConstructMagicPacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
	pu1Byte			StaAddr
	);

VOID
ConstructEapolKeyPacket(
	PADAPTER		Adapter,
	pu1Byte			Buffer,
	pu4Byte			pLength,
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
	pu1Byte			pGTK, // Key Data field: Pointer to GTK.
	BOOLEAN			bEncrypt		// encrypt the packet or not. added by Annie, 2005-07-11.
);


VOID
ConstructCcxRmReport(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN	PRT_RM_REPORTS		pRmReports
	);

VOID
ConstructDot11kRmReport(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN	PRT_RM_REQUESTS		pRmRequests,
	IN	PRT_RM_REPORTS		pRmReports
	);

VOID
ConstructCcxROGUEAPReport(
	PADAPTER			Adapter,
	pu1Byte				Buffer,
	pu4Byte				pLength,
	int                             index
);


//
// Add for all management action frame to filling frame header.
//
VOID
ConstructMaFrameHdr(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			pAddr,
	IN	u1Byte			Category,
	IN	u1Byte			Action,
	OUT	POCTET_STRING	posMaFrame
	);

VOID
AppendAdditionalIEs(
	OUT		POCTET_STRING	packet,
	IN		OCTET_STRING	ies
);

VOID
ConstructQosADDTSPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	u1Byte			DialogToken,
	IN	PQOS_TSTREAM	pTs,
	IN	u4Byte			numTspec
	);

VOID
ConstructQosDELTSPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	PQOS_TSTREAM	pTs,
	IN	u4Byte			numTspec
	);

VOID
ConstructCcxNeighborPoll(
	IN	PADAPTER			Adapter,
	OUT	pu1Byte				Buffer,
	OUT	pu4Byte				pLength
	);

VOID
AppendQoSElement(
	OUT		POCTET_STRING	packet,
	IN		POCTET_STRING	element,
	IN		u1Byte			OUI_Subtype
	);

VOID
ConstructEAPOLStartPacket(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN   BOOLEAN				bEncrypt
	);

VOID
ConstructARPResponse(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			pIPAddres
	);

VOID
ConstructCtsPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			pStaAddr
	);

VOID
ConstructNSPacket(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Buffer,
	IN	pu4Byte			pLength,
	IN	pu1Byte			pTAAddress
	);

VOID
ConstructSAQeuryReq(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN	pu1Byte				pTargetSta,
	IN	u2Byte				Identifier
	);

VOID
ConstructSAQeuryRsp(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Buffer,
	IN	pu4Byte				pLength,
	IN	pu1Byte				pTargetSta,
	IN	u2Byte				Identifier
	);

RT_STATUS
FT_AppendMdIE(
	IN	PADAPTER				pAdapter,
	IN	CONTENT_PKT_TYPE		contentType,
	IN	PVOID					pInfoBuf,
	IN	u4Byte					InfoBufLen,
	IN	u4Byte					maxBufLen,
	OUT POCTET_STRING			posOutContent
	);

RT_STATUS
FT_AppendFtIE(
	IN	PADAPTER				pAdapter,
	IN	CONTENT_PKT_TYPE		contentType,
	IN	PVOID					pInfoBuf,
	IN	u4Byte					InfoBufLen,
	IN	u4Byte					maxBufLen,
	OUT POCTET_STRING			posOutContent
	);

#endif // #ifndef __INC_MGNTCONSTRUCTPACKET_H

