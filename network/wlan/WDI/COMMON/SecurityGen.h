#ifndef __INC_SECURITYGEN_H
#define __INC_SECURITYGEN_H

#define	PAIRWISE_KEYIDX		0

/* 
	WDK TKIP key format defintion:
	Bit(28) of Key Index	|Bit 0-127			|Bit 128-191	|Bit 192-255
	------------------------+-------------------+---------------+-----------
	0(supplicant)			|Encryption key		|Rx MIC key		|Tx MIC Key
	1(Authenticator)		|Rncryption key		|Tx MIC key		|Rx MIC Key

	Linux TKIP key format defintion:
	Bit 0-127		|Bit 128-191	|Bit 192-255
	----------------+---------------+-----------
	Encryption key 	|Tx MIC key 	|Rx MIC Key
*/
#define	TKIP_ENCKEY_POS		0
#define	TKIP_MICKEYTX_POS	TKIP_ENC_KEY_LEN	// for station mode, this is for RX MIC key
#define	TKIP_MICKEYRX_POS	(TKIP_ENC_KEY_LEN+TKIP_MIC_KEY_LEN)		// for station mode, this is for TX MIC key

#define NDIS_802_11_AI_REQFI_CAPABILITIES      1
#define NDIS_802_11_AI_REQFI_LISTENINTERVAL    2
#define NDIS_802_11_AI_REQFI_CURRENTAPADDRESS  4

#define NDIS_802_11_AI_RESFI_CAPABILITIES      1
#define NDIS_802_11_AI_RESFI_STATUSCODE        2
#define NDIS_802_11_AI_RESFI_ASSOCIATIONID     4

typedef	u4Byte	RT_SEC_STATUS;

#define	RT_SEC_STATUS_SUCCESS						(RT_SEC_STATUS)0x00000000	// Encryption/Decryption Success
#define	RT_SEC_STATUS_FAILURE						(RT_SEC_STATUS)0x00000001	// Unspecified Failure
#define	RT_SEC_STATUS_PKT_TYPE_NOT_SUPPORT			(RT_SEC_STATUS)0x00000002	// The type of packet is invalid to encrypt or decrypt.
#define	RT_SEC_STATUS_INVALID_PKT_LEN				(RT_SEC_STATUS)0x00000003	// The length of packet is invalid to encrypt or decrypt.
#define	RT_SEC_STATUS_MGNT_FRAME_UNENCRYPT			(RT_SEC_STATUS)0x00000004	// This is unencrypted packet, but the encryption type is enbled.
#define	RT_SEC_STATUS_DATA_UNICAST_IV_REPLAY		(RT_SEC_STATUS)0x00000005	// This is unicast data packet with IV replay.
#define	RT_SEC_STATUS_DATA_MULTICAST_IV_REPLAY		(RT_SEC_STATUS)0x00000006	// This is multicast data packet with IV replay.
#define	RT_SEC_STATUS_MGNT_IV_REPLAY				(RT_SEC_STATUS)0x00000007	// This is mgnt frame with IV replay.
#define	RT_SEC_STATUS_DATA_MIC_FAILURE				(RT_SEC_STATUS)0x00000008	// TKIP data packet is MIC failure
#define	RT_SEC_STATUS_MGNT_MIC_FAILURE				(RT_SEC_STATUS)0x00000009	// TKIP mgnt packet is MIC failure
#define	RT_SEC_STATUS_MFP_MGNT_MHDR_FAILURE			(RT_SEC_STATUS)0x0000000A	// CCXv5 MFP TKIP MHDRIE is invalid
#define	RT_SEC_STATUS_MFP_MGNT_LEN_FAILURE			(RT_SEC_STATUS)0x0000000B	// 802.11w BIP data to long 
#define	RT_SEC_STATUS_MFP_MGNT_MMIE_FAILURE			(RT_SEC_STATUS)0x0000000C   // 802.11w MMIE fail
#define	RT_SEC_STATUS_MFP_MGNT_MMIE_MIC_FAILURE		(RT_SEC_STATUS)0x0000000D   // 802.11w MMIE Mic fail
#define	RT_SEC_STATUS_UNKNOWN_TA					(RT_SEC_STATUS)0x0000000E   // Unknow TA, no Key to decrypt
#define	RT_SEC_STATUS_ICV_ERROR						(RT_SEC_STATUS)0x0000000F   // Decryption fail, ICV error




#define PN_DECIMAL2ARRAY(ul16, ul32, keyid, pucOut)\
	pucOut[0] = (UCHAR)(ul32 >>  0) & 0xff;\
	pucOut[1] = (UCHAR)(ul32 >>  8) & 0xff;\
	pucOut[2] = 0x0;\
	pucOut[3] = (0x20 | (UCHAR)((keyid << 6) & 0xc0) );\
	pucOut[4] = (UCHAR)(ul32 >> 16) & 0xff;\
	pucOut[5] = (UCHAR)(ul32 >> 24) & 0xff;\
	pucOut[6] = (UCHAR)(ul16 >>  0) & 0xff;\
	pucOut[7] = (UCHAR)(ul16 >>  8) & 0xff;

#define TKIP_TSC_DECIMAL2ARRAY(IV16, IV32, keyid, TSC)\
	TSC[0] = EF1Byte( (UCHAR)((IV16>>8) & 0xff) );\
	TSC[1] = EF1Byte( (UCHAR)(0x20 | (TSC[0]&0x7f)) );\
	TSC[2] = EF1Byte( (UCHAR)((IV16>>0) & 0xff) );\
	TSC[3] = EF1Byte( (UCHAR)(0x20 | (keyid<<6)) );\
	TSC[4] = EF1Byte( (UCHAR)((IV32>>0) & 0xff) );\
	TSC[5] = EF1Byte( (UCHAR)((IV32>>8) & 0xff) );\
	TSC[6] = EF1Byte( (UCHAR)((IV32>>16) & 0xff) );\
	TSC[7] = EF1Byte( (UCHAR)((IV32>>24) & 0xff) );

//add for CKIP , by CCW
#define CKIP_SEQ_DECIMAL2ARRAY(ul32,pucOut)\
	pucOut[0] = EF1Byte( (UCHAR)((ul32 >> 24) & 0xff) );\
	pucOut[1] = EF1Byte( (UCHAR)((ul32 >> 16) & 0xff) );\
	pucOut[2] = EF1Byte( (UCHAR)((ul32 >>  8) & 0xff) );\
	pucOut[3] = EF1Byte( (UCHAR)((ul32 >>  0) & 0xff) );

/*
//#define CKIP_IV_DECIMAL2ARRAY(ul32,pucOut, keyid)\
//	pucOut[0] = (UCHAR)((ul32 >> 16) & 0xff);\
//	pucOut[1] = (UCHAR)((ul32 >>  8) & 0xff);\
//	pucOut[2] = (UCHAR)((ul32 >>  0) & 0xff);\
//	pucOut[3] = (UCHAR)((keyid<<  6) & 0xff);
*/

//  MFP Support type !!
typedef enum _MFP_SECURITY_TYPE{
	MFP_NONE				=	0x0,
	MFP_CCX				=	0x1,
	MFP_80211_W			=	0x2,
}MFP_SECURITY_TYPE,*PMFP_SECURITY_TYPE;

#define ENCRYPTION_MAX_OVERHEAD		128 // Follow 8185, 2005.04.14, by rcnjko.

typedef struct
{
	u4Byte  K0, K1;         // Key
	u4Byte  L, R;           // Current state
	u4Byte  M;              // Message accumulator (single word)
	u4Byte     nBytesInM;      // # bytes in M
}MicData;


//
// Redefine NDIS5 PMKID related data structure for multi-platform consideration.
// 070418, by rcnjko.
//
typedef struct _N5_BSSID_INFO
{
	u1Byte BSSID[6];
	u1Byte PMKID[16];
} N5_BSSID_INFO, *PN5_BSSID_INFO;

typedef struct _N5_802_11_PMKID
{
	u4Byte Length;
	u4Byte BSSIDInfoCount;
	N5_BSSID_INFO BSSIDInfo[1];
} N5_802_11_PMKID, *PN5_802_11_PMKID;
//---------------------------------
// Add For 802.1w AES CMAC
//---------------------------------

/* AES definition & structure */
#define AES_STATE_ROWS_1W 4     /* Block size: 4*4*8 = 128 bits */
#define AES_STATE_COLUMNS_1W 4
#define AES_BLOCK_SIZES_1W AES_STATE_ROWS_1W*AES_STATE_COLUMNS_1W
#define AES_KEY_ROWS_1W 4
#define AES_KEY_COLUMNS_1W 8    /*Key length: 4*{4,6,8}*8 = 128, 192, 256 bits */
#define AES_KEY128_LENGTH_1W 16
#define AES_KEY192_LENGTH_1W 24
#define AES_KEY256_LENGTH_1W 32
#define AES_CBC_IV_LENGTH_1W 16


// 
// Need to move TypeDef.h .....
//
typedef struct _MMIE_STRUC{
	u1Byte		ID;
	u1Byte		Length;
	u1Byte		KeyID[2];
	u1Byte		IPN[6];
	u1Byte		MIC[8];
}MMIE_STRUC,*PMMIE_STRUC;

typedef struct {
    u1Byte State[AES_STATE_ROWS_1W][AES_STATE_COLUMNS_1W];
    u1Byte KeyWordExpansion[AES_KEY_ROWS_1W][AES_KEY_ROWS_1W*((AES_KEY256_LENGTH_1W >> 2) + 6 + 1)];
} AES_CTX_STRUC, *PAES_CTX_STRUC;


/* AES operations */
VOID 
AES_KeyExpansion_1W(
    u1Byte			Key[],
    u4Byte 			KeyLength,
    AES_CTX_STRUC 	*paes_ctx
);

VOID 
AES_Encrypt_1W(
    u1Byte 		PlainBlock[],
    u4Byte 		PlainBlockSize,
    u1Byte 		Key[],
    u4Byte 		KeyLength,
    u1Byte 		CipherBlock[],
    u4Byte 		*CipherBlockSize
);


/* AES-CMAC operations */
VOID 
AES_CMAC_GenerateSubKey_1W(
    u1Byte Key[],
    u4Byte KeyLength,
    u1Byte SubKey1[],
    u1Byte SubKey2[]
);

VOID AES_CMAC_1W (
    u1Byte PlainText[],
    u4Byte PlainTextLength,
    u1Byte Key[],
    u4Byte KeyLength,
    u1Byte MACText[],
    u4Byte *MACTextLength
);

void SecMFPEncodeAESCCM_1W(
	PRT_SECURITY_T   pSec,
	u4Byte			pulKey[],
	u4Byte 			keyid,
	u1Byte			*plaintext,
	u4Byte			headerLen,
	u4Byte 			totalLen
);

int SecMFPDecodeAESCCM_1W(
	PRT_SECURITY_T   pSec,
	u4Byte			pulKey[],
	u1Byte 			*data,
	u4Byte			headerLen,
	u4Byte 			totalLen
);


//---------------------------------
// End of 802.1w 
//---------------------------------

void
TKIPGenerateKey(
	UCHAR		*RC4KEY,
	ULONG		IV32,
	USHORT		IV16,
	const UCHAR	*TA,
	const UCHAR	*TK
);

void 
SecEncodeAESCCM(
	PRT_SECURITY_T   pSec,
	u4Byte			pulKey[],
	u1Byte			*plaintext,
	u4Byte			headerLen,
	u4Byte 			totalLen
);

//------------------------------------------------
// Name : SecRSNAEncodeAESCCM
// Function : Add for RSNA AES-CCMP Encode
// input : plaintext = [Header] + [IV] + [Data] + [AES MIC]
//-------------------------------------------------
void SecRSNAEncodeAESCCM(
	PRT_SECURITY_T   pSec,
	u4Byte			pulKey[],
	u4Byte 			keyid,
	u1Byte			*plaintext,
	u4Byte			headerLen,
	u4Byte 			totalLen
	
);

//------------------------------------------------
// Name : SecMFPEncodeAESCCM
// Function : Add for RSNA AES-CCMP Encode
// input : plaintext = [Header] + [IV] + [Data] + [AES MIC]
//-------------------------------------------------
void SecMFPEncodeAESCCM(
	PRT_SECURITY_T   pSec,
	u4Byte			pulKey[],
	u4Byte 			keyid,
	u1Byte			*plaintext,
	u4Byte			headerLen,
	u4Byte 			totalLen
);

int 
SecDecodeAESCCM(
	PRT_SECURITY_T   pSec,
	u4Byte			pulKey[],
	u1Byte 			*data,
	u4Byte			headerLen,
	u4Byte 			totalLen
);

int SecMFPDecodeAESCCM(
	PRT_SECURITY_T   pSec,
	u4Byte			pulKey[],
	u1Byte 			*data,
	u4Byte			headerLen,
	u4Byte 			totalLen
);

VOID
SecHeaderFillIV(
	PADAPTER	Adapter,
	pu1Byte		PacketStart
);

BOOLEAN
SecCheckMIC(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
);

VOID
SecGetGroupCipherFromBeacon(
	PADAPTER		Adapter,
	PRT_WLAN_BSS	pbssDesc
);

VOID
SecConstructRSNIE(
	PADAPTER	Adapter
);

RT_STATUS
Sec_AppendWPAIE(
	IN	PADAPTER				pAdapter,
	IN	CONTENT_PKT_TYPE		contentType,
	IN	PVOID					pInfoBuf,
	IN	u4Byte					InfoBufLen,
	IN	u4Byte					maxBufLen,
	OUT POCTET_STRING			posOutContent
	);

RT_STATUS
Sec_AppendRSNIE(
	IN	PADAPTER				pAdapter,
	IN	CONTENT_PKT_TYPE		contentType,
	IN	PVOID					pInfoBuf,
	IN	u4Byte					InfoBufLen,
	IN	u4Byte					maxBufLen,
	OUT POCTET_STRING			posOutContent
	);

RT_ENC_ALG
SecGetEncryptionOverhead(
	PADAPTER	Adapter,
	UNALIGNED pu2Byte		pMPDUHead,
	UNALIGNED pu2Byte		pMPDUTail,
	UNALIGNED pu2Byte		pMSDUHead,
	UNALIGNED pu2Byte		pMSDUTail,
	BOOLEAN		bByPacket,
	BOOLEAN		bIsBroadcastPkt
	);

VOID
SecInit(
	PADAPTER	Adapter
);

BOOLEAN
SecCheckTKIPMIC(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
);

//BOOLEAN
//SecCheckCKIPMIC(
//	PADAPTER	Adapter,
//	PRT_RFD		pRfd
//);

BOOLEAN
SecSWWEPDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
);

BOOLEAN
SecSWTKIPDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
);

BOOLEAN
SecSWAESDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
);

BOOLEAN
SecSWCKIPDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
);

VOID
SecClearAllKeys(
	PADAPTER	Adapter
);

VOID
SecClearGroupKeyByIdx(
	PADAPTER	Adapter,
	u1Byte		paraIndex
);

VOID
SecClearWEPKeyByIdx(
	PADAPTER	Adapter,
	u1Byte		paraIndex
);



//for AP mode
VOID
SecClearPairwiseKeyByMacAddr(
	PADAPTER	Adapter,
	pu1Byte		paraMacAddr
);

RT_STATUS
Sec_SelAkmFromAuthMode(
	IN	PADAPTER			pAdapter,
	IN	RT_AUTH_MODE		AuthMode,
	IN	CONTENT_PKT_TYPE	contentType,
	IN	PVOID				pInfoBuf,
	IN	u4Byte				InfoBufLen,
	OUT	PAKM_SUITE_TYPE		pAKM
	);

RT_STATUS
Sec_MapAKMSuiteToOUIType(
	IN	AKM_SUITE_TYPE	AKMSuite,
	OUT	pu1Byte			pOUI,
	OUT	pu1Byte			pOUIType
	);

RT_STATUS
Sec_MapOUITypeToAKMSuite(
	IN	pu1Byte				pOUI,
	IN	pu1Byte				pOUIType,
	IN	RT_SECURITY_LEVEL	secLevl,
	OUT	PAKM_SUITE_TYPE		pAKMSuite
	);

RT_STATUS
Sec_MapCipherSuiteToOUIType(
	IN	RT_ENC_ALG			cipherSuite,
	IN	RT_SECURITY_LEVEL	secLevl,
	OUT	pu1Byte				pOUI,
	OUT	pu1Byte				pOUIType
	);

RT_STATUS
Sec_MapOUITypeToCipherSuite(
	IN	pu1Byte				pOUI,
	IN	pu1Byte				pOUIType,
	IN	RT_SECURITY_LEVEL	secLevl,
	OUT	PRT_ENC_ALG			pCipherSuite
	);

RT_ENC_ALG_DEP
Sec_MapNewCipherToDepCipherAlg(
	IN	RT_ENC_ALG			newCipherSuite
	);

u1Byte
SecGetTxKeyIdx(
	PADAPTER	Adapter,
	pu1Byte		pMacAddr
);

u1Byte
SecGetRxKeyIdx(
	PADAPTER	Adapter,
	pu1Byte		pDestAddr,
	u1Byte		IVKeyIdx
);

#define GET_WEP_IV_INITVECTOR(__pWepIv)		LE_BITS_TO_4BYTE(__pWepIv, 0, 24)
#define SET_WEP_IV_INITVECTOR(__pWepIv, _val)	SET_BITS_TO_LE_4BYTE(__pWepIv, 0, 24, _val)

#define GET_WEP_IV_KEYID(__pWepIv)		( (u1Byte)LE_BITS_TO_4BYTE(__pWepIv, 30, 2) )
#define SET_WEP_IV_KEYID(__pWepIv, _val)	SET_BITS_TO_LE_4BYTE(__pWepIv, 30, 2, _val)

VOID SecCalculateMIC(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
);

VOID SecSWTKIPEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
);

VOID SecSWWEPEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
);

VOID SecSWAESEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
);

VOID SecSWCKIPEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
);

VOID SecSoftwareEncryption(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
);

RT_SEC_STATUS
SecSoftwareDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
);

VOID SecCalculateTKIPMIC(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
);

VOID SecCalculateCKIPMIC(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
);

RT_SEC_STATUS
SecRxCheckIV(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

typedef enum{
	RSNIE_TYPE_WPA = 1,
	RSNIE_TYPE_WPA2 = 2
}RSNIE_TYPE;

//typedef enum { 
//	ERROR_BUFFER_TOO_SMALL = -1, 
//	ERROR_INVALID_PARA = -2, 
//	ERROR_INVALID_RSNIE = -3,
//	ERROR_INVALID_MULTICASTCIPHER = -4, 
//	ERROR_INVALID_UNICASTCIPHER = -5,
//	ERROR_INVALID_AUTHKEYMANAGE = -6,
//	ERROR_UNSUPPORTED_RSNEVERSION = -7,
//} RSNIE_ERROR;

#define RSN_STRERROR_BUFFER_TOO_SMALL		"WPA Info Element : Input Buffer too small"
#define RSN_STRERROR_INVALID_PARAMETER		"WPA Info Element : Invalid RSN Parameter"
#define RSN_STRERROR_INVALID_RSNIE			"WPA Info Element : Invalid RSNIE"
#define RSN_STRERROR_INVALID_MULTICASTCIPHER "WPA Info Element : Invalid MulticastCipher"
#define RSN_STRERROR_UNSUPPORTED_RSNEVERSION "WPA Info Element : Unsupported RSN Version"

#define RSN_ELEMENT_ID				0xDD
#define RSN_VER1					0x01
#define	WPA_VER1					0x01
#define DOT11_MAX_CIPHER_ALGORITHMS	0x0a

//
// For pairwise cipher suite and 
// Authentication and Key Management Suite List in RSN IE.
//
typedef struct _DOT11_RSN_IE_SUITE{
	u1Byte	OUI[3];
	u1Byte	Type;
}DOT11_RSN_IE_SUITE;

// Minimum length of RSN IE:
//	Version(2) + Group Cipher OUI/Type (4) + Pairwise Cipher Cnt (2) + Pairwise Cipher Suite * 1 (4) + AKM Cnt (2) + AKM * 1 (4) + RSN Capability (2)
#define	RSN_IE_MIN_LEN		20

#define GET_RSNIE_SUITE_COUNT(__pPtr) 			ReadEF2Byte(__pPtr)
#define SET_RSNIE_SUITE_COUNT(__pPtr, __Value) 		WriteEF2Byte(__pPtr, __Value)
#define GET_RSNIE_SUITE_PTR(__pPtr, __Idx) 		(((DOT11_RSN_IE_SUITE *)((pu1Byte)(__pPtr) + 2)) + (__Idx))

//
// For pairwise cipher suite and 
// Authentication and Key Management Suite List in RSN IE.
//
#define GET_RSN_CAP_PREAUTH(__pPtr) 				LE_BITS_TO_2BYTE(__pPtr, 0, 1)
#define SET_RSN_CAP_PREAUTH(__pPtr, __Value) 		SET_BITS_TO_LE_2BYTE(__pPtr, 0, 1, __Value)
#define GET_RSN_CAP_NO_PAIRWISE(__pPtr) 			LE_BITS_TO_2BYTE(__pPtr, 1, 1)
#define SET_RSN_CAP_NO_PAIRWISE(__pPtr, __Value) 		SET_BITS_TO_LE_2BYTE(__pPtr, 1, 1, __Value)
#define GET_RSN_CAP_PTKSA_REPLAY_COUNTER(__pPtr) 		LE_BITS_TO_2BYTE(__pPtr, 2, 2)
#define SET_RSN_CAP_PTKSA_REPLAY_COUNTER(__pPtr, __Value) 	SET_BITS_TO_LE_2BYTE(__pPtr, 2, 2, __Value)
#define GET_RSN_CAP_GTKSA_REPLAY_COUNTER(__pPtr) 		LE_BITS_TO_2BYTE(__pPtr, 4, 2)
#define SET_RSN_CAP_GTKSA_REPLAY_COUNTER(__pPtr, __Value) 	SET_BITS_TO_LE_2BYTE(__pPtr, 4, 2, __Value)
#define GET_RSN_CAP_PEERKEY_ENABLED(__pPtr)			LE_BITS_TO_2BYTE(__pPtr, 9, 1)
#define SET_RSN_CAP_PEERKEY_ENABLED(__pPtr, __Value)	SET_BITS_TO_LE_2BYTE(__pPtr, 9, 1, __Value)
#define GET_RSN_CAP_MFP_CAPABLE(__pPtr) 			LE_BITS_TO_2BYTE(__pPtr, 7, 1)
#define SET_RSN_CAP_MFP_CAPABLE(__pPtr, __Value) 	SET_BITS_TO_LE_2BYTE(__pPtr, 7, 1, __Value)
#define GET_RSN_CAP_MFP_REQUIRED(__pPtr) 			LE_BITS_TO_2BYTE(__pPtr, 6, 1)
#define SET_RSN_CAP_MFP_REQUIRED(__pPtr, __Value) 	SET_BITS_TO_LE_2BYTE(__pPtr, 6, 1, __Value)


//2 // Note:
//2 //     The RSN setting macros shall be set sequentially to make sure that each offset/count is correct.
// RSN IE format with bytes count:
// | Version (2) | Group Ciher Suite (4) | Pairwise Cipher Suite Cnt (2) | Pairwise Cipher Suite List (4 * m) | AKM Suite Cnt (2) | AKM Suite List (4 * n) | RSN Cap (2) | PMK-ID (16 * s) |
#define	GET_RSN_IE_VERSION(_pStart)					ReadEF2Byte((UNALIGNED pu1Byte)(_pStart))
#define	SET_RSN_IE_VERSION(_pStart, _Value)			WriteEF2Byte((UNALIGNED pu1Byte)(_pStart), _Value)

#define	GET_RSN_IE_GROUP_CIPHER_SUITE(_pStart)			((UNALIGNED pu1Byte)(_pStart) + 2)
#define	SET_RSN_IE_GROUP_CIPHER_SUITE_OUI_W_TYPE(_pStart, _OUI, _Type)	SET_OUI_WITH_TYPE(((UNALIGNED pu1Byte)(_pStart) + 2), _OUI, _Type)

#define	GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart)	ReadEF2Byte((UNALIGNED pu1Byte)(_pStart) + 6)
#define	SET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart, _Value)			WriteEF2Byte((UNALIGNED pu1Byte)(_pStart) + 6, _Value)

#define	GET_RSN_IE_PAIRWISE_CIPHER_SUITE_LIST(_pStart, _Idx)	((UNALIGNED pu1Byte)(_pStart) + 8 + ((_Idx) * 4))
#define	ADD_RSN_IE_PAIRWISE_CIPHER_SUITE_OUI_W_TYPE(_pStart, _OUI, _Type)	\
{	\
	SET_OUI_WITH_TYPE(((UNALIGNED pu1Byte)(_pStart) + 8 + GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4), _OUI, _Type);	\
	SET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart, GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) + 1);	\
}

#define	GET_RSN_IE_AKM_SUITE_CNT(_pStart)	ReadEF2Byte((UNALIGNED pu1Byte)(_pStart) + 8 + GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4)
#define	SET_RSN_IE_AKM_SUITE_CNT(_pStart, _Value)	\
	WriteEF2Byte((UNALIGNED pu1Byte)(_pStart) + 8 + GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4, _Value)

#define	GET_RSN_IE_AKM_SUITE_LIST(_pStart, _Idx)	((UNALIGNED pu1Byte)(_pStart) + 10 + (GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + ((_Idx) * 4))
#define	ADD_RSN_IE_AKM_SUITE_OUI_W_TYPE(_pStart, _OUI, _Type)	\
{	\
	SET_OUI_WITH_TYPE((UNALIGNED pu1Byte)(_pStart) + 10 + \
		(GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + \
		(GET_RSN_IE_AKM_SUITE_CNT(_pStart) * 4), \
		_OUI, _Type);	\
	SET_RSN_IE_AKM_SUITE_CNT(_pStart, GET_RSN_IE_AKM_SUITE_CNT(_pStart) + 1);	\
}


#define	GET_RSN_IE_CAP_PTR(_pStart)								((UNALIGNED pu1Byte)(_pStart) + 10 + (GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + (GET_RSN_IE_AKM_SUITE_CNT(_pStart) * 4))

#define GET_RSN_IE_CAP_PREAUTH(_pStart) 						GET_RSN_CAP_PREAUTH(GET_RSN_IE_CAP_PTR(_pStart))
#define SET_RSN_IE_CAP_PREAUTH(_pStart, __Value) 				SET_RSN_CAP_PREAUTH(GET_RSN_IE_CAP_PTR(_pStart), __Value)

#define GET_RSN_IE_CAP_NO_PAIRWISE(_pStart)						GET_RSN_CAP_NO_PAIRWISE(GET_RSN_IE_CAP_PTR(_pStart))
#define SET_RSN_IE_CAP_NO_PAIRWISE(_pStart, __Value)			SET_RSN_CAP_NO_PAIRWISE(GET_RSN_IE_CAP_PTR(_pStart), __Value)

#define GET_RSN_IE_CAP_PTKSA_REPLAY_COUNTER(_pStart)			GET_RSN_CAP_PTKSA_REPLAY_COUNTER(GET_RSN_IE_CAP_PTR(_pStart))
#define SET_RSN_IE_CAP_PTKSA_REPLAY_COUNTER(_pStart, __Value)	SET_RSN_CAP_PTKSA_REPLAY_COUNTER(GET_RSN_IE_CAP_PTR(_pStart), __Value)

#define GET_RSN_IE_CAP_GTKSA_REPLAY_COUNTER(_pStart)			GET_RSN_CAP_GTKSA_REPLAY_COUNTER(GET_RSN_IE_CAP_PTR(_pStart))
#define SET_RSN_IE_CAP_GTKSA_REPLAY_COUNTER(_pStart, __Value)	SET_RSN_CAP_GTKSA_REPLAY_COUNTER(GET_RSN_IE_CAP_PTR(_pStart), __Value)

#define GET_RSN_IE_CAP_PEERKEY_ENABLED(_pStart)					GET_RSN_CAP_PEERKEY_ENABLED(GET_RSN_IE_CAP_PTR(_pStart))
#define SET_RSN_IE_CAP_PEERKEY_ENABLED(_pStart, __Value)		SET_RSN_CAP_PEERKEY_ENABLED(GET_RSN_IE_CAP_PTR(_pStart), __Value)

#define GET_RSN_IE_CAP_MFP_CAPABLE(_pStart)						GET_RSN_CAP_MFP_CAPABLE(GET_RSN_IE_CAP_PTR(_pStart))
#define SET_RSN_IE_CAP_MFP_CAPABLE(_pStart, __Value)			SET_RSN_CAP_MFP_CAPABLE(GET_RSN_IE_CAP_PTR(_pStart), __Value)

#define GET_RSN_IE_CAP_MFP_REQUIRED(_pStart)					GET_RSN_CAP_MFP_REQUIRED(GET_RSN_IE_CAP_PTR(_pStart))
#define SET_RSN_IE_CAP_MFP_REQUIRED(_pStart, __Value)			SET_RSN_CAP_MFP_REQUIRED(GET_RSN_IE_CAP_PTR(_pStart), __Value)


#define	GET_RSN_IE_PMKID_CNT(_pStart)				ReadEF2Byte((UNALIGNED pu1Byte)(_pStart) + 12 + (GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + (GET_RSN_IE_AKM_SUITE_CNT(_pStart) * 4))
#define	SET_RSN_IE_PMKID_CNT(_pStart, _Value)	\
	WriteEF2Byte((UNALIGNED pu1Byte)(_pStart) + 12 + (GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + (GET_RSN_IE_AKM_SUITE_CNT(_pStart) * 4), _Value)

#define	GET_RSN_IE_PMKID_LIST(_pStart, _Idx)	((UNALIGNED pu1Byte)(_pStart) + 14 + (GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + (GET_RSN_IE_AKM_SUITE_CNT(_pStart) * 4) + ((_Idx) * 16))
#define	ADD_RSN_IE_PMKID(_pStart, _pPMKID)	\
{	\
	PlatformMoveMemory(GET_RSN_IE_PMKID_LIST(_pStart, GET_RSN_IE_PMKID_CNT(_pStart)), _pPMKID, PMKID_LEN);	\
	SET_RSN_IE_PMKID_CNT(_pStart, GET_RSN_IE_PMKID_CNT(_pStart) + 1);	\
}

#define	GET_RSN_IE_GROUP_MGNT_CIPHER_SUITE(_pStart)			((UNALIGNED pu1Byte)(_pStart) + 14 + (GET_RSN_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + (GET_RSN_IE_AKM_SUITE_CNT(_pStart) * 4) + (GET_RSN_IE_PMKID_CNT(_pStart) * 16))
#define	SET_RSN_IE_GROUP_MGNT_CIPHER_SUITE_OUI_W_TYPE(_pStart, _OUI, _Type)	SET_OUI_WITH_TYPE(GET_RSN_IE_GROUP_MGNT_CIPHER_SUITE(_pStart), _OUI, _Type)


// Note:
//2 //     The WPA setting macros shall be set sequentially to make sure that each offset/count is correct.
// WPA IE format with bytes count:
// |OUI/Type (4) | Version (2) | Group Ciher Suite (4) | Pairwise Cipher Suite Cnt (2) | Pairwise Cipher Suite List (4 * m) | AKM Suite Cnt (2) | AKM Suite List (4 * n) | Cap (2)

#define	WPA_IE_MIN_LEN	24


#define	SET_WPA_IE_OUI_AND_TYPE(_pStart)		SET_OUI_WITH_TYPE((UNALIGNED pu1Byte)(_pStart), WPA_OUI, WPAIE_OUI_TYPE)

#define	GET_WPA_IE_VERSION(_pStart)					ReadEF2Byte((UNALIGNED pu1Byte)(_pStart) + 4)
#define	SET_WPA_IE_VERSION(_pStart, _Value)			WriteEF2Byte((UNALIGNED pu1Byte)(_pStart) + 4, _Value)

#define	GET_WPA_IE_GROUP_CIPHER_SUITE(_pStart)			((UNALIGNED pu1Byte)(_pStart) + 6)
#define	SET_WPA_IE_GROUP_CIPHER_SUITE_OUI_W_TYPE(_pStart, _OUI, _Type)	SET_OUI_WITH_TYPE(((UNALIGNED pu1Byte)(_pStart) + 6), _OUI, _Type)

#define	GET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart)	ReadEF2Byte((UNALIGNED pu1Byte)(_pStart) + 10)
#define	SET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart, _Value)			WriteEF2Byte((UNALIGNED pu1Byte)(_pStart) + 10, _Value)

#define	GET_WPA_IE_PAIRWISE_CIPHER_SUITE_LIST(_pStart, _Idx)	((UNALIGNED pu1Byte)(_pStart) + 12 + ((_Idx) * 4))
#define	ADD_WPA_IE_PAIRWISE_CIPHER_SUITE_OUI_W_TYPE(_pStart, _OUI, _Type)	\
{	\
	SET_OUI_WITH_TYPE(((UNALIGNED pu1Byte)(_pStart) + 12 + GET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4), _OUI, _Type);	\
	SET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart, GET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) + 1);	\
}

#define	GET_WPA_IE_AKM_SUITE_CNT(_pStart)	ReadEF2Byte((UNALIGNED pu1Byte)(_pStart) + 12 + GET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4)
#define	SET_WPA_IE_AKM_SUITE_CNT(_pStart, _Value)	\
	WriteEF2Byte((UNALIGNED pu1Byte)(_pStart) + 12 + GET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4, _Value)

#define	GET_WPA_IE_AKM_SUITE_LIST(_pStart, _Idx)	((UNALIGNED pu1Byte)(_pStart) + 14 + (GET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + ((_Idx) * 4))
#define	ADD_WPA_IE_AKM_SUITE_OUI_W_TYPE(_pStart, _OUI, _Type)	\
{	\
	SET_OUI_WITH_TYPE((UNALIGNED pu1Byte)(_pStart) + 14 + \
		(GET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + \
		(GET_WPA_IE_AKM_SUITE_CNT(_pStart) * 4), \
		_OUI, _Type);	\
	SET_WPA_IE_AKM_SUITE_CNT(_pStart, GET_WPA_IE_AKM_SUITE_CNT(_pStart) + 1);	\
}

#define	GET_WPA_IE_CAP_PTR(_pStart)								((UNALIGNED pu1Byte)(_pStart) + 14 + (GET_WPA_IE_PAIRWISE_CIPHER_SUITE_CNT(_pStart) * 4) + (GET_WPA_IE_AKM_SUITE_CNT(_pStart) * 4))

#define GET_WPA_CAP_PTKSA_REPLAY_COUNTER(__pPtr) 		LE_BITS_TO_2BYTE(__pPtr, 2, 2)
#define SET_WPA_CAP_PTKSA_REPLAY_COUNTER(__pPtr, __Value) 	SET_BITS_TO_LE_2BYTE(__pPtr, 2, 2, __Value)
#define GET_WPA_CAP_GTKSA_REPLAY_COUNTER(__pPtr) 		LE_BITS_TO_2BYTE(__pPtr, 4, 2)
#define SET_WPA_CAP_GTKSA_REPLAY_COUNTER(__pPtr, __Value) 	SET_BITS_TO_LE_2BYTE(__pPtr, 4, 2, __Value)

#define GET_WPA_IE_CAP_PTKSA_REPLAY_COUNTER(_pStart)			GET_WPA_CAP_PTKSA_REPLAY_COUNTER(GET_WPA_IE_CAP_PTR(_pStart))
#define SET_WPA_IE_CAP_PTKSA_REPLAY_COUNTER(_pStart, __Value)	SET_WPA_CAP_PTKSA_REPLAY_COUNTER(GET_WPA_IE_CAP_PTR(_pStart), __Value)

#define GET_WPA_IE_CAP_GTKSA_REPLAY_COUNTER(_pStart)			GET_WPA_CAP_GTKSA_REPLAY_COUNTER(GET_WPA_IE_CAP_PTR(_pStart))
#define SET_WPA_IE_CAP_GTKSA_REPLAY_COUNTER(_pStart, __Value)	SET_WPA_CAP_GTKSA_REPLAY_COUNTER(GET_WPA_IE_CAP_PTR(_pStart), __Value)


// Reference: 802.11i D10.0 Table23, Authentication and Key Management Suite Selectors, page33.
typedef enum{
	DOT11_AuthKeyType_None = 0,
	DOT11_AuthKeyType_RSN = 1,		// 802.1x.
	DOT11_AuthKeyType_RSNPSK = 2,		// Pre-Shared Key.
	DOT11_AuthKeyType_FT_OVER_1X = 3, // FT authentication negotiated over IEEE 802.1X
	DOT11_AuthKeyType_FT_OVER_PSK = 4, // FT authentication using PSK
	DOT11_AuthKeyType_1X_SHA256 = 5, // Authentication negotiated over IEEE 802.1X or using PMKSA caching with SHA256 Key Derivation
	DOT11_AuthKeyType_PSK_SHA256 = 6, // PSK with SHA256 Key Derivation
	DOT11_AuthKeyType_TPK = 7,		// 802.11z TDLS Peer Key Handshake
	DOT11_AuthKeyType_SAE_SHA256 = 8, // SAE Authentication with SHA-256 or using PMKSA caching with SHA-256 key derivation
	DOT11_AuthKeyType_FT_OVER_SAE_SHA256 = 9, // FT authentication over SAE with SHA-256
} DOT11_AuthKey_TYPE;

// AKM OUI type with OUI 0x00 0F AC
#define	RSN_AKM_SUITE_OUI_TYPE_RSN_1X			1	// 802.1x.
#define	RSN_AKM_SUITE_OUI_TYPE_RSN_PSK			2	// Pre-Shared Key.
#define	RSN_AKM_SUITE_OUI_TYPE_FT_1X			3	// FT authentication negotiated over IEEE 802.1X
#define	RSN_AKM_SUITE_OUI_TYPE_FT_PSK			4	// FT authentication using PSK
#define	RSN_AKM_SUITE_OUI_TYPE_1X_SHA256		5	// Authentication negotiated over IEEE 802.1X or using PMKSA caching with SHA256 Key Derivation
#define	RSN_AKM_SUITE_OUI_TYPE_PSK_SHA256		6	// PSK with SHA256 Key Derivation
#define	RSN_AKM_SUITE_OUI_TYPE_TPK				7	// 802.11z TDLS Peer Key Handshake
#define	RSN_AKM_SUITE_OUI_TYPE_SAE_SHA256		8	// SAE Authentication with SHA-256 or using PMKSA caching with SHA-256 key derivation
#define	RSN_AKM_SUITE_OUI_TYPE_FT_SAE_SHA256	9	// FT authentication over SAE with SHA-256

// RSN Cipher suite OUI type with OUI 0x00 0F AC
#define	RSN_CIPHER_SUITE_OUI_TYPE_USE_GROUP_CIPHER		0	// Use group cipher suite
#define	RSN_CIPHER_SUITE_OUI_TYPE_WEP40					1	// WEP-40
#define	RSN_CIPHER_SUITE_OUI_TYPE_TKIP					2	// TKIP
#define	RSN_CIPHER_SUITE_OUI_TYPE_RESERVED				3	// Reserved
#define	RSN_CIPHER_SUITE_OUI_TYPE_CCMP					4	// CCMP ¡V default pairwise cipher suite and default group cipher suite for data frames in an RSNA
#define	RSN_CIPHER_SUITE_OUI_TYPE_WEP104				5	// WEP-104
#define	RSN_CIPHER_SUITE_OUI_TYPE_BIP					6	// BIP¡Xdefault group management cipher suite in an RSNA with management frame protection enabled
#define	RSN_CIPHER_SUITE_OUI_TYPE_GRP_ADDR_DISALLOWED	7	// Group addressed traffic not allowed

// WPA Cipher suite OUI type with OUI 0x00 0F AC
#define	WPA_CIPHER_SUITE_OUI_TYPE_NONE					0	// None
#define	WPA_CIPHER_SUITE_OUI_TYPE_WEP40					1	// WEP-40
#define	WPA_CIPHER_SUITE_OUI_TYPE_TKIP					2	// TKIP
#define	WPA_CIPHER_SUITE_OUI_TYPE_WRAP					3	// WRAP
#define	WPA_CIPHER_SUITE_OUI_TYPE_CCMP					4	// CCMP ¡V default pairwise cipher suite and default group cipher suite for data frames in an RSNA
#define	WPA_CIPHER_SUITE_OUI_TYPE_WEP104				5	// WEP-104


//2004/06/29, kcwu TKIP_MIC implement in TKIP_MIC.c
u4Byte
SecMICGetUINT32( 
	pu1Byte p
 );

VOID
SecMICPutUINT32(
	pu1Byte p,
	u4Byte val
);

VOID 
SecMICClear(
	MicData *micdata
);

VOID 
SecMICSetKey(
	MicData *micdata, 
	pu1Byte key
);

VOID
SecMICAppendByte(
	MicData *micdata,
	u1Byte b
);

VOID
SecMICAppend(
	MicData *micdata,
	pu1Byte src,
	s4Byte nBytes
);

VOID 
SecMICGetMIC(
	MicData *micdata,
	pu1Byte dst
);
//end MIC

//2004/09/15, kcwu
//u4Byte *AES_SetKey(const u4Byte in_key[], const u4Byte key_len, u4Byte e_key[]);
pu4Byte
AES_SetKey(
	const u4Byte	in_key[],
	const u4Byte	key_len,
	u4Byte		e_key[]
);

void AES_WRAP(PADAPTER	Adapter,
			  u1Byte* plain, int plain_len, 
			  u1Byte * iv,	int iv_len,
			  u1Byte * kek,	int kek_len,
			  u1Byte *cipher, u2Byte *cipher_len
);



void AES_UnWRAP(PADAPTER Adapter,
				u1Byte * cipher, int cipher_len, 
			    u1Byte * kek,	int kek_len,
			    u1Byte * plain);


void TestAES_WRAP(PADAPTER	Adapter);
//Add for CKIP , by CCW

//----------------------------------------------------------------------------
//CKIP.c
//----------------------------------------------------------------------------
void 
CKIP_TEST(
	ADAPTER * Adapter
	);

int 
DecodeCKIP(
	ADAPTER * Adapter, 
	UCHAR *pkt,
	int pktlen, 
	UCHAR *key, 
	int mickeyid
	);

int 
EncodeCKIP(
	ADAPTER * Adapter, 
	UCHAR *pkt,
	int pktlen, 
	UCHAR *hdr, 
	UCHAR *key, 
	int keylen, 
	int mickeyid
	);

int 
ckip_miccalc(
	ADAPTER * Adapter, 
	UCHAR	*CK, 
	UCHAR	*pDA, 
	UCHAR	*pSA, 
	UCHAR	*payload, 
	int		payloadlen, 
	UCHAR	*calcmic,
	int		mickeyid 	
	);

int 
ckip_encrypt(
	UCHAR *CK, 
	UCHAR *iv, 
	UCHAR *ecr, 
	int ecrlen
	); 

void 
AES_Encrypt(
	const ULONG in_blk[4], 
	ULONG out_blk[4], 
	ULONG e_key[]
	);


BOOLEAN 
SecInsertDenyBssidList(
 	PRT_SECURITY_T	pSec,
	u1Byte			BssidToDeny[6],
	u8Byte			DenyStartTime
);

BOOLEAN 
SecIsInDenyBssidList(
 	PRT_SECURITY_T	pSec,
	u1Byte			BssidToCheck[6]
);

BOOLEAN 
SecFillHeader(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
);


BOOLEAN
SecIsEAPOLPacket(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pPduOS
);

BOOLEAN
SecIsEAPOLKEYPacket(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pPduOS
	);

VOID
SecSetPMKID(
	IN	PADAPTER	Adapter,
	IN	PN5_802_11_PMKID		pPMKid
);


int
SecIsInPMKIDList(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		bssid
);


VOID
SecCatPMKID(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			CurAPbssid,
	IN	PRT_WLAN_BSS	pBssDesc
);


BOOLEAN 
SecDropForKeyAbsent(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
);

VOID
SecSetSwEncryptionDecryption(
	PADAPTER			Adapter,
	BOOLEAN				bSWTxEncrypt,
	BOOLEAN				bSWRxDecrypt
);

BOOLEAN
SecIsTxKeyInstalled(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			pRA
);

void 
Supplicant_AES_UnWRAP(
	PADAPTER Adapter,
	u1Byte	* cipher, 
	int		cipher_len, 
	u1Byte	* kek,	
	int		kek_len,
	u1Byte	* plain
);

RT_SEC_STATUS
SecSWMFPDecryption(
	IN	PADAPTER	pAdapter,
	IN	PRT_RFD 	pRfd
	);

RT_SEC_STATUS
SecRxDescryption(
	IN	PADAPTER	pAdapter,
	IN	PRT_RFD 	pRfd
	);

void
SecAppenMMIE(
	IN	PADAPTER		pAdapter,
	OUT POCTET_STRING	posFrame
	);

BOOLEAN
SecStaGetANoseForS5(
	IN	PADAPTER		Adapter,
	IN	POCTET_STRING	pPduOS
	);

BOOLEAN
SecStaGenPMKForS5(
	IN	PADAPTER		Adapter
	);

RT_STATUS
OnSAQueryReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
    );

RT_STATUS
OnSAQueryRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
    );

BOOLEAN
SecFillProtectTxMgntFrameHeader(
	IN	PADAPTER	pAdapter,
	IN	PRT_TCB		pTcb
	);

VOID
SAQueryTimerCallback(
	IN	PRT_TIMER		pTimer
	);

//
// ---------------------------------------------------------------
// 2011/03/04 MH Move all new CAN search metho to here forcommon binary . 
// We need to reorganize the locaton in he future.
//

VOID
SecUpdateSWGroupKeyInfo(
	IN	PADAPTER		Adapter,
	IN	u4Byte			KeyIndex,
	IN	u4Byte			KeyLength,
	IN	pu1Byte			KeyMaterial
	);

#if (HW_EN_DE_CRYPTION_FOR_NEW_CAM_SEARCH_FLOW == 1)
#define	IS_EN_DE_CRYPTION_NEW_CAM_SUPPORT()	TRUE

VOID
SEC_AsocEntry_ResetEntry(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_STA	pEntry
	);

#else //#if (HW_EN_DE_CRYPTION_FOR_NEW_CAM_SEARCH_FLOW == 1)
#define	IS_EN_DE_CRYPTION_NEW_CAM_SUPPORT()	FALSE
#define	SEC_AsocEntry_ResetEntry(_Adapter, _pEntry)

#endif //#if (HW_EN_DE_CRYPTION_FOR_NEW_CAM_SEARCH_FLOW == 1)


//
// 2011/03/04 MH Move all new CAN search metho to here forcommon binary . 
// We need to reorganize the locaton in he future.
// ---------------------------------------------------------------
//

//#define	HW_EN_DE_CRYPTION_FOR_NEW_CAM_SEARCH_FLOW		1


#endif // #ifndef __INC_SECURITYGEN_H
