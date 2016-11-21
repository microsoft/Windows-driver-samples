#ifndef	__AUTHENTICATOR_H
#define __AUTHENTICATOR_H


//------------------------------------------------------------------------------
// Authenticator related operations.
//------------------------------------------------------------------------------

// On event

void 
Authenticator_OnDeauthenticationRequest(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt
	);


void 
Authenticator_OnAuthenticationRequest(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt
	);

void 
Authenticator_OnReAuthenticationRequest(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt
	);


void 
Authenticator_OnEAPOLKeyRecvd(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt,
	IN	OCTET_STRING			pdu
	);


OCTET_STRING 
EAPOLkeyGetRSNIE(
	IN	OCTET_STRING	eapolkeypkt,
	IN	ELEMENT_ID		ID
);


void
KeyMgntTimeout(
	PRT_TIMER		pTimer
);

void 
Authenticator_OnTimeoutCountExceeded(
	IN	PADAPTER				Adapter,
	IN	PAUTH_PKEY_MGNT_TAG	pKeyMgnt
	);

// Status handle

void 
Authenticator_StateDISCONNECT(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA,
	IN	u1Byte			ReasonCode
	);

void 
Authenticator_StateDISCONNECTED(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StateINITIALIZE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StateAUTHENTICATION(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StateAUTHENTICATION2(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);


void 
Authenticator_StatePTKSTART(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StatePTKINITNEGOTIATING(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StatePTKINITDONE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StateUPDATEKEYS(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StateINTEGRITYFAILURE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StateKEYUPDATE(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StateREKEYNEGOTIATING(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StateREKEYESTABLISHED(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA
	);

void 
Authenticator_StateKEYERROR(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pSTA,
	IN	u1Byte			ReasonCode
	);


void 
Authenticator_GlobalReset(
	IN	PADAPTER		Adapter
	);




#endif //  __AUTHENTICATOR_H
