#ifndef __INC_SECURITYTYPE_H
#define __INC_SECURITYTYPE_H

#define		MAX_KEY_LEN				61

#define		KEY_BUF_SIZE			5

#define		TKIP_ENC_KEY_LEN		16		// Added by Annie, 2005-07-05.		
#define		TKIP_MIC_KEY_LEN		8
#define		TKIP_MIC_LEN			8
#define		TKIP_KEY_LEN			(TKIP_ENC_KEY_LEN+TKIP_MIC_KEY_LEN*2)
#define		BIP_KEY_LEN				16

#define		MAXRSNIELEN				256
#define		WEP_IV_LEN				4		// Added by Annie, 2005-12-23.
#define		EXT_IV_LEN				8		// Added by Annie, 2005-12-23.
#define		KEYID_POS_IN_IV		3		// Added by Annie, 2005-12-23.

#define		WEP_ICV_LEN			4		// Added by Annie, 2006-10-19.
#define		TKIP_MIC_LEN			8		// Added by Annie, 2006-10-19.
#define		AES_MIC_LEN			8		// Added by Annie, 2006-10-19.

#define         SMS4_MIC_LEN                16
#define         WAPI_EXT_LEN                18

// Added by Annie, 2005-07-05.
#define		KEK_LEN				16
#define		KCK_LEN				16
#define		TKIP_ENC_KEY_POS		32		//(KEK_LEN+KEK_LEN)
#define		TKIP_MIC_KEY_POS		48		//(TKIP_ENC_KEY_POS+TKIP_ENC_KEY_LEN)

#define		GTK_MIC_TX_POS		16		//GTK Temporal Encryption key length
#define		GTK_MIC_RX_POS		24		//GTK Temporal Encryption key length + TKIP_MIC_KEY_LEN

// CKIP related.
#define		CKIP_KEY_LEN			16		// Added by Annie, 2006-08-17.

#define		MAX_CIPHER_SUITE_NUM		4
#define		MAX_AUTH_SUITE_NUM	4

#define		TWOWAY_HANDSHAKE_WPATKIP_LEN	183
#define		TWOWAY_HANDSHAKE_WPAAES_LEN	187
#define		TWOWAY_HANDSHAKE_2ND_LEN	151

#define		MAX_CCKM_IE_REQ_LEN			32
#define		MAX_CCKM_IE_LEN				512

#define		SW_ENCRYPT_BUF_SIZE			2400

static u1Byte WPA_OUI[SIZE_OUI] = {0x00, 0x50, 0xf2};
static u1Byte RSN_OUI[SIZE_OUI] = {0x00, 0x0F, 0xAC};

#define	WPAIE_OUI_TYPE	1


// HW/SW encryption/decryption related. 2006.09.29, by shien chang.
enum _EnryptionDecryption_Mechanism
{
	EncryptionDecryptionMechanism_Auto	= 0,
	EncryptionDecryptionMechanism_Hw	= 1,
	EncryptionDecryptionMechanism_Sw	= 2,
	EncryptionDecryptionMechanism_Undefined = 3,
};

//
// For check chiper and auth type , add by CCW 2008/09/18
//
enum _PAIRWISE_CHIPER
{
	NONE_WPA 		=  0x00,
	WPA_TKIP		=  0x02,
	WPA_AES			=  0x04,
	WPA2_TKIP		=  0x20,
	WPA2_AES		=  0x40,
};

typedef u4Byte	AKM_SUITE_TYPE, *PAKM_SUITE_TYPE;
#define	AKM_SUITE_NONE			(AKM_SUITE_TYPE)0
#define	AKM_WPA_1X				(AKM_SUITE_TYPE)0x00000001
#define	AKM_WPA_PSK				(AKM_SUITE_TYPE)0x00000002
#define	AKM_WPA_CCKM			(AKM_SUITE_TYPE)0x00000004
#define	AKM_WPA2_1X				(AKM_SUITE_TYPE)0x00000010
#define	AKM_WPA2_PSK			(AKM_SUITE_TYPE)0x00000020
#define	AKM_WPA2_CCKM			(AKM_SUITE_TYPE)0x00000040
#define	AKM_RSNA_1X_SHA256		(AKM_SUITE_TYPE)0x00000080
#define	AKM_RSNA_PSK_SHA256		(AKM_SUITE_TYPE)0x00000100
#define	AKM_FT_1X				(AKM_SUITE_TYPE)0x00000200
#define	AKM_FT_PSK				(AKM_SUITE_TYPE)0x00000400
#define	AKM_1X_CCKM				(AKM_SUITE_TYPE)0x00000800

#define	IS_AKM_SUPPORT_SHA_256(_AkmSuite) TEST_FLAG(_AkmSuite, AKM_RSNA_1X_SHA256 | AKM_RSNA_PSK_SHA256 | AKM_FT_1X | AKM_FT_PSK)

#if(OS_WIN_FROM_WIN10(OS_VERSION))
	#define	DEFAULT_SUPPORT_AKM_SUITE	(AKM_WPA_1X | AKM_WPA_PSK | AKM_WPA2_1X | AKM_WPA2_PSK | AKM_WPA_CCKM | AKM_WPA2_CCKM | AKM_RSNA_1X_SHA256 | AKM_RSNA_PSK_SHA256 | AKM_FT_1X | AKM_FT_PSK)
#elif (OS_WIN_FROM_WIN8(OS_VERSION))
	#define	DEFAULT_SUPPORT_AKM_SUITE	(AKM_WPA_1X | AKM_WPA_PSK | AKM_WPA2_1X | AKM_WPA2_PSK | AKM_WPA_CCKM | AKM_WPA2_CCKM | AKM_RSNA_1X_SHA256 | AKM_RSNA_PSK_SHA256)
#elif (RT_PLATFORM==PLATFORM_WINDOWS)
	#define	DEFAULT_SUPPORT_AKM_SUITE	(AKM_WPA_1X | AKM_WPA_PSK | AKM_WPA2_1X | AKM_WPA2_PSK | AKM_WPA_CCKM | AKM_WPA2_CCKM)
#else
	#define	DEFAULT_SUPPORT_AKM_SUITE	(AKM_WPA_1X | AKM_WPA_PSK | AKM_WPA2_1X | AKM_WPA2_PSK)
#endif


#define	AKM_WPA_1X_OUI_TYPE		1
#define	AKM_WPA_PSK_OUI_TYPE	2

//======================================================================================
// AES_CCMP specific
//======================================================================================

#define AESMSK_FC_DEFAULT			0xC78F	// default value of AES MASK for Frame Control Field. (2 bytes)
#define AESMSK_SC_DEFAULT			0x000F	// default value of AES MASK for Sequence Control Field. (2 bytes)
#define AESMSK_QC_DEFAULT			0x000F	// default value of AES MASK for QoS Control Field. (2 bytes)

enum
{
	AESCCMP_BLK_SIZE			=   16,     // # octets in an AES block
	//AESCCMP_MAX_PACKET 		=   4*512,  // largest packet size   //YJ,test for DTM MPE,120218
	AESCCMP_MAX_PACKET 		=   5*512,  // largest packet size
	AESCCMP_N_RESERVED		=   0,      // reserved nonce octet value
	AESCCMP_A_DATA			=   0x40,   // the Adata bit in the flags
	AESCCMP_M_SHIFT			=   3,      // how much to shift the 3-bit M field
	AESCCMP_L_SHIFT			=   0,      // how much to shift the 3-bit L field
	AESCCMP_L_SIZE			=   2,       // size of the l(m) length field (in octets)
	AESCCMP_OFFSET_SC		=	22,
	AESCCMP_OFFSET_DURATION	=	4,
	AESCCMP_OFFSET_A2		=	10,
	AESCCMP_OFFSET_A4		=	24,
	AESCCMP_QC_TID_MASK		=	0x0f,
	AESCCMP_BLK_SIZE_TOTAL	=   16*16,     // Added by Annie for CKIP AES MIC BSOD, 2006-08-17.
											// 16*8 < 4*60  Resove to 16*16
};
typedef union _AESCCMP_BLOCK		// AES cipher block
{
	u4Byte	x[AESCCMP_BLK_SIZE/4];		// access as 8-bit octets or 32-bit words
	u1Byte	b[AESCCMP_BLK_SIZE];
}AESCCMP_BLOCK;

typedef struct _AESCCMP_PACKET
{
	BOOLEAN			encrypted;		 // TRUE if encrypted
	u1Byte			TA[6];			// xmit address
	u4Byte			micLength;		// # octets of MIC appended to plaintext (M)
	u4Byte			ivLength;
	u4Byte			clrCount;		// # cleartext octets covered by MIC
	u4Byte			pktNum[2];		// unique packet sequence number (like WEP IV)
	AESCCMP_BLOCK	key;			// the encryption key (K)
	u4Byte			length;			// # octets in data[]
	u1Byte			*data;
	u1Byte			dataArray[AESCCMP_MAX_PACKET+ 2*AESCCMP_BLK_SIZE];		// packet contents
}AESCCMP_PACKET;

// Ref: IEEE 802.11 2012 8.4.2.27.2 Cipher suites
typedef u4Byte	RT_ENC_ALG,*PRT_ENC_ALG;
#define	RT_ENC_ALG_NO_CIPHER						0x00000000
#define	RT_ENC_ALG_USE_GROUP						0x00000001	// Use Group Key
#define	RT_ENC_ALG_TKIP								0x00000002
#define	RT_ENC_ALG_AESCCMP							0x00000004
#define	RT_ENC_ALG_WEP40							0x00000008
#define	RT_ENC_ALG_WEP104							0x00000010
#define	RT_ENC_ALG_WEP								(RT_ENC_ALG_WEP40 | RT_ENC_ALG_WEP104)
#define	RT_ENC_ALG_SMS4								0x00000020	// WAPI
#define	RT_ENC_ALG_BIP								0x00000040	// PMF multicast protected management frame
#define	RT_ENC_ALG_GROUP_ADDR_TRAFFIC_DISALLOWED	0x00000080

// Ref: 802.11i sepc D10.0 7.3.2.25.1 Cipher Suites
//Encryption Algorithms
//2 // Note: The following encryption algorithm is deprecated, do not use again.
typedef enum{
	NO_Encryption			= 0,
	WEP40_Encryption		= 1,
	TKIP_Encryption			= 2,
	Reserved_Encryption		= 3,
	AESCCMP_Encryption 	= 4,
	WEP104_Encryption		= 5,
	WEP_Encryption    		= 6, // WEP + WPA
	SMS4_Encryption             =7,
	Max_Encryption			=0xFF
	//WEP_Encryption = 7,

}RT_ENC_ALG_DEP, *PRT_ENC_ALG_DEP;


typedef enum _RT_AUTH_MODE
{
    RT_802_11AuthModeOpen,
    RT_802_11AuthModeShared,
    RT_802_11AuthModeAutoSwitch,
    RT_802_11AuthModeWPA,
    RT_802_11AuthModeWPAPSK,
    RT_802_11AuthModeWPANone,
    RT_802_11AuthModeWPA2,
    RT_802_11AuthModeWPA2PSK,
    RT_802_11AuthModeCCKM,
    RT_802_11AuthModeWAPI,
    RT_802_11AuthModeCertificateWAPI, 
    //===== Add new Mode above this line=====//
    RT_802_11AuthModeMax,

	RT_NETWORK_EAP = 0x80,
    
} RT_AUTH_MODE, *PRT_AUTH_MODE;

typedef enum _RT_802_11_WEP_STATUS
{
    RT802_11WEPEnabled,
    RT802_11Encryption1Enabled = RT802_11WEPEnabled,
    RT802_11WEPDisabled,
    RT802_11EncryptionDisabled = RT802_11WEPDisabled,
    RT802_11WEPKeyAbsent,
    RT802_11Encryption1KeyAbsent = RT802_11WEPKeyAbsent,
    RT802_11WEPNotSupported,
    RT802_11EncryptionNotSupported = RT802_11WEPNotSupported,
    RT802_11Encryption2Enabled, 
    RT802_11Encryption2KeyAbsent,
    RT802_11Encryption3Enabled, 
    RT802_11Encryption3KeyAbsent,
    Wapi_Encryption,
    Wapi_Certificate

} RT_802_11_WEP_STATUS, *PRT_802_11_WEP_STATUS, RT_802_11_ENCRYPTION_STATUS, *PRT_802_11_ENCRYPTION_STATUS;

typedef enum _RT_SECURITY_LEVEL
{
	RT_SEC_LVL_NONE = 0,
	RT_SEC_LVL_WPA,
	RT_SEC_LVL_WPA2,
	RT_SEC_LVL_WAPI

}RT_SECURITY_LEVEL, *PRT_SECURITY_LEVEL;

typedef struct _RT_PMKID_TYPE
{
	u1Byte flag;
}RT_PMKID_TYPE, *PRT_PMKID_TYPE;

#define MAX_DENY_BSSID_LIST_CNT 20
typedef struct _RT_DENY_BSSID
{
	BOOLEAN	bUsed;
	u8Byte	StartTime;
	u1Byte	Bssid[6];
}RT_DENY_BSSID, *PRT_DENY_BSSID;


// WPA2 Parameters
#define NUM_PRE_AUTH_KEY	16					// Reference NumOfPreAuthKey in 8185. Added by Annie, 2006-05-07.
#define NUM_PMKID_CACHE	NUM_PRE_AUTH_KEY	// Added by Annie, 2006-05-07.
#define NUM_CAT_PMKID		1					// Added by Annie, 2006-05-07.
#define PMKID_LEN			16

typedef struct _RT_PMKID_LIST
{
	BOOLEAN						bUsed;
	u1Byte 						Bssid[6];
	u1Byte						PMKID[PMKID_LEN];
	u1Byte						SsidBuf[33];
	OCTET_STRING				Ssid;
} RT_PMKID_LIST, *PRT_PMKID_LIST;

typedef struct _DOT11_RSN_IE_PMKID
{
	u2Byte		SuiteCount;
	u1Byte		PMKList[1];
} DOT11_RSN_IE_PMKID, *PDOT11_RSN_IE_PMKID;


//
// CCX 1 S13: CKIP Related Parameters.
// Revised by Annie, 2006-08-14.
//
#define IE_CISCO_CWMIN_POSITION	0x04
#define IE_CISCO_CWMAX_POSITION	0x06
#define IE_CISCO_FLAG_POSITION		0x08	// Flag byte: byte 8, numbered from 0.
#define SUPPORT_CKIP_MIC			0x08	// bit3
#define SUPPORT_CKIP_PK			0x10	// bit4
typedef struct _CKIP_PARAMETER
{
	DECLARE_RT_OBJECT(CKIP_PARAMETER);

	BOOLEAN				bIsKP;
	BOOLEAN				bIsMIC;
	BOOLEAN				bCkipSupported;		// Aironet IE support CKIP or not.
	ULONG				ulSeqUpLink;
	u1Byte				CKIPKeyBuf[KEY_BUF_SIZE][CKIP_KEY_LEN];
} CKIP_PARAMETER, *PCKIP_PARAMETER;


//
// Add for RSNA Per-station default Key ,by CCW
// Per-Sta default key is used for group traffic.
//
typedef struct _PER_STA_DEFAULT_KEY_ENTRY
{
	u1Byte                      MACAdrss[6];
	BOOLEAN                  Valid;
	u1Byte			    DefKeyBuf[4][AESCCMP_BLK_SIZE_TOTAL];
	BOOLEAN                  DefkeyValid[4];
}PER_STA_DEFAULT_KEY_ENTRY,*PPER_STA_DEFAULT_KEY_ENTRY;

//
// Add for RSNA Per-station mapping Key ,by CCW
// Per-Sta key mapping key for unicast traffic.
//
typedef struct _PER_STA_MPAKEY_ENTRY
{
	u1Byte                      MACAdrss[6];
	BOOLEAN                  Valid;
	u1Byte                      MapKeyBuf[AESCCMP_BLK_SIZE_TOTAL];
}PER_STA_MPAKEY_ENTRY,*PPER_STA_MPAKEY_ENTRY;

#define MAX_NUM_PER_STA_KEY		12
// TSC in TKIP and PN in AES shall be initialized to 1 in 802.11i.
// The spec doesn't determine the initialized value so we set the same value.
// When the temporal key (including wep) is reset, we need to refresh the Tx IV.
// By Bruce, 2010-11-04.
#define	DEFAULT_INIT_TX_IV			1
// 0 means we don't check the rx IV as the IV replay attack.
#define	DEFAULT_INIT_RX_IV			0

typedef enum _RT_SA_QUERY_STATE
{
	RT_SA_QUERY_STATE_UNINITIALIZED = 0,
	RT_SA_QUERY_STATE_SA_DETECTED = 0,
	RT_SA_QUERY_STATE_SA_REQ_IN_PROGRESS,
	//=== Insert new method above this line === //
	RT_SA_QUERY_STATE_MAX,	// Reserved
	RT_SA_QUERY_STATE_FORCE_UINT32 = 0xFFFFFFFF	//Reserved, force this enumeration to compile to 32 bits in size (by Win SDK 8.0).
}RT_SA_QUERY_STATE, *PRT_SA_QUERY_STATE;
//Security information structure
typedef struct _RT_SECURITY_T{

	//Encryption Algorithm for Unicast Packet
	RT_ENC_ALG		PairwiseEncAlgorithm;

	//Encryption Algorithm for Brocast/Multicast
	RT_ENC_ALG		GroupEncAlgorithm;
	RT_ENC_ALG		GroupEncAlgorithmCap;	// Capability/Combination of group cipher suites

	BOOLEAN			UseDefaultKey;		// Set if default key used

	//The Index of Default Key(WEP)
	u1Byte			DefaultTransmitKeyIdx;

	//The index of Group Transmit Key
	u1Byte			GroupTransmitKeyIdx;

	//Key buffer
	u1Byte			KeyBuf[KEY_BUF_SIZE][MAX_KEY_LEN];
	u1Byte			KeyLen[KEY_BUF_SIZE];

	//2004/09/15, for AES
	u1Byte			AESKeyBuf[KEY_BUF_SIZE][AESCCMP_BLK_SIZE_TOTAL];

	//The pointer of Pairwise Key, it always points to KeyBuf[4]
	pu1Byte			PairwiseKey;
	
	u1Byte			TxMICKey[TKIP_MIC_KEY_LEN];		//  Tx MIC Key
	u1Byte			RxMICKey[TKIP_MIC_KEY_LEN];		//  Rx MIC Key

	BOOLEAN			SWTxEncryptFlag;	// Software send encrypt flag
	BOOLEAN			SWRxDecryptFlag;	// Software recv decrypt flag
	BOOLEAN			SWRxAESMICFlag;			// Software TKIP MIC flag
	BOOLEAN			bGroupKeyFixed;	// Can not modify group key
	// Manual config read from system to force HW or SW encryption/decryption.
	// 0: Auto.  1: HW enc/dec.  2: SW enc/dec. 2006.09.29, by shien chang.
	u1Byte			RegSWTxEncryptFlag;
	u1Byte			RegSWRxDecryptFlag;

	u1Byte			EncryptionHeadOverhead;
	u1Byte			EncryptionTailOverhead;

	RT_AUTH_MODE						AuthMode;

	RT_802_11_ENCRYPTION_STATUS		EncryptionStatus;

	OCTET_STRING	RSNIE;
	u1Byte			RSNIEBuf[MAXRSNIELEN];

	u8Byte			TxIV;

	RT_SECURITY_LEVEL	SecLvl;

	u1Byte					AssocInfo[512];
	
	//---------------------------------------------------------------------------
	// For MIC failure.
	//---------------------------------------------------------------------------
	u8Byte			LastPairewiseTKIPMICErrorTime;	// Last pairewise key TKIP MIC error happen time or 0 if it has not happed in 60 seconds.
	BOOLEAN			bToDisassocAfterMICFailureReportSend;	// Disassociate from the AP for twice MIC failure in 60 seconds.
	RT_DENY_BSSID	DenyBssidList[MAX_DENY_BSSID_LIST_CNT]; // List of BSS denied by us for MIC failure. 	  


	//---------------------------------------------------------------------------
	// For WPA2 Pre-Authentication.
	//---------------------------------------------------------------------------
	BOOLEAN				RegEnablePreAuth;				// Default value: Pre-Authentication enabled or not, from registry "EnablePreAuth". Added by Annie, 2005-11-01.
	BOOLEAN				EnablePreAuthentication;			// Current Value: Pre-Authentication enabled or not.
	RT_PMKID_LIST		PMKIDList[NUM_PMKID_CACHE];	// Renamed from PreAuthKey[NUM_PRE_AUTH_KEY]. Annie, 2006-10-13.
	u4Byte				PMKIDCount;						// Added by Annie, 2006-10-13.
	u1Byte				szCapability[256];				// For WPA2-PSK using zero-config, by Annie, 2005-09-20.


	//---------------------------------------------------------------------------
	// For Software AES Encryption.
	//---------------------------------------------------------------------------
	AESCCMP_PACKET	AESCCMP_Packet;
	AESCCMP_PACKET 	RX_AESCCMP_Packet;
	u4Byte			AESCCMPMicLen ;
	
	//
	// For CKIP. Revised by Annie, 2006-08-14.
	//
	PCKIP_PARAMETER	pCkipPara;
	//---------------------------------------
	//For CCKM KRK key , by CCW
	//---------------------------------------
	u1Byte                 KRK[128];
	OCTET_STRING	CCKMReqIE;
	u1Byte			CCKMReqIEBuf[MAX_CCKM_IE_REQ_LEN];
	OCTET_STRING	CCKMIE;
	u1Byte			CCKMIEBuf[MAX_CCKM_IE_LEN];
	
	//--------------------------------------
	// For NDIS6 RSNA, by CCW
	//--------------------------------------
	// TODO: remvoe our group key from PerStaDefKeyTable[]
	PER_STA_DEFAULT_KEY_ENTRY		PerStaDefKeyTable[MAX_NUM_PER_STA_KEY]; // Keep group key of each STA in the IBSS.
	PER_STA_MPAKEY_ENTRY			MAPKEYTable[MAX_NUM_PER_STA_KEY]; // Keep pairwise key of each STA in the IBSS.
	
	//
	// For Check IV 
	//
	u8Byte			RXMutiIV;	// Rx multicast IV for IV replay check, and 0 means do not check IV replay.
	u8Byte			RXUntiIV;	// Rx unicast IV for IV replay check, and 0 means do not check IV replay.

	u1Byte			SecBuffer[SW_ENCRYPT_BUF_SIZE];

	// 802.11w BIP
	u1Byte				BIPKeyBuffer[MAX_KEY_LEN];
	u1Byte				IPN[6];
	RT_TIMER			SAQueryTimer;
	RT_SA_QUERY_STATE	pmfSaState;
	u2Byte				SAReqIdentifier;
}RT_SECURITY_T, *PRT_SECURITY_T;



//------------------------------------------------------------------------------
// Authenticator related operations. Added by Annie, 2005-06-28.
//------------------------------------------------------------------------------

//
// Key Length
//
// Reference:
// 1. Code from AP team: auth\include\1x_kmsm_keydef.h
//

#define LIB1X_EAPOL_HDRLEN		4	// defined in auth\include\1x_eapol.h, from AP team.

#define PASSPHRASE_LEN			64	// modified by Annie, 2005-07-01.

#define PMK_LEN					32
#define PTK_LEN					64
#define PTK_LEN_TKIP			64
#define PTK_LEN_NO_TKIP		48      //for CCMP, WRAP, WEP
#define PTK_LEN_CCMP			48
#define PTK_LEN_WRAP			48
#define PTK_LEN_WEP			48

#define PTK_LEN_EAPOLMIC		16
#define PTK_LEN_EAPOLENC		16

#define GMK_LEN					32
#define GTK_LEN					32
#define GTK_LEN_TKIP			32      //32 for TKIP and 16 for CCMP, WRAP, WEP
#define GTK_LEN_NO_TKIP		16
#define GTK_LEN_CCMP			16
#define GTK_LEN_WRAP			16
#define GTK_LEN_WEP			16


#define INFO_ELEMENT_SIZE		128

#define EAP_HDR_LEN				(sMacHdrLng+LLC_HEADER_SIZE+TYPE_LENGTH_FIELD_SIZE)		// added by Annie, 2005-07-04.

#define MAX_EAPOLMSG_LEN		512
#define MAX_EAPOLKEYMSG_LEN	(MAX_EAPOLMSG_LEN-(ETHERNET_HEADER_SIZE+LIB1X_EAPOL_HDRLEN))
#define EAPOLMSG_HDRLEN		95      //EAPOL-key payload length without KeyData

#define KEY_RC_LEN				8
#define KEY_NONCE_LEN			32
#define KEY_IV_LEN				16
#define KEY_RSC_LEN				8
#define KEY_ID_LEN				8
#define KEY_MIC_LEN				16
#define KEY_MATERIAL_LEN		2

#define DescTypePos				0
#define KeyInfoPos				1
#define KeyLenPos				3
#define ReplayCounterPos			5
#define KeyNoncePos				13
#define KeyIVPos					45
#define KeyRSCPos				61
#define KeyIDPos					69
#define KeyMICPos				77
#define KeyDataLenPos			93
#define KeyDataPos				95

#define MAX_TIMEOUT_CNT		3

//
// Key management
//
// Reference:
// 1. IEEE 802.11i/D3.0  8.5.6.1 & state machine diagram in Figure 53(p.113)
// 2. Code from AP team: auth\include\1x_kmsm.h
//

// Definition of events in ASM (Authenticator state machine)
typedef enum _ASM_EVENT_ID { 
	ASMEID_NoEvent,
	ASMEID_AssociationRequest,
	ASMEID_ReAssociationRequest,
	ASMEID_AuthenticationRequest,
	ASMEID_ReAuthenticationRequest,
	ASMEID_AuthenticationSuccess,
	ASMEID_Disconnect,
	ASMEID_DeauthenticationRequest,
	ASMEID_Init,
	ASMEID_Disassociate,
	ASMEID_IntegrityFailure,
	ASMEID_EAPOLKeyRecvd,
	ASMEID_TimeOut,
	ASMEID_TimeOutExceeded,
} ASM_EVENT_ID, *PASM_EVENT_ID;



// Definition of PTK key state in ASM
// Ref: 8.5.6, page 111, 802.11 D3.0.
typedef enum _ASM_PAIRWISEKEY_STATE {
	//ASMPS_DEAUTHENTICATE,
	ASMPS_INITIALIZE,
	ASMPS_DISCONNECTE,
	ASMPS_DISCONNECTED,
	ASMPS_AUTHENTICATION,
	ASMPS_AUTHENTICATION2,		// described on Fig.53. 802.11i D3.0.
	ASMPS_INITPMK, 
	ASMPS_INITPSK,
	ASMPS_PTKSTART,
	ASMPS_PTKINITNEGOTIATING,
	ASMPS_PTKINITDONE,
	ASMPS_UPDATEKEYS,
	ASMPS_MICFAILURE,
	ASMPS_INTEGRITYFAILURE,		// Added by Annie, 2005-07-18.
	ASMPS_KEYUPDATE,
	ASMPS_ERRORHANDLE,			// Added states 
}ASM_PAIRWISEKEY_STATE, *PASM_PAIRWISEKEY_STATE;


// Definition of PTK group key (GTK) state in ASM
// Ref: 8.5.6, page 112, 802.11 D3.0.
typedef enum _ASM_GROUPKEY_STATE {
	ASMGS_INITIALIZE,
	ASMGS_REKEYNEGOTIATING,
	ASMGS_REKEYESTABLISHED,
	ASMGS_KEYERROR,
//	ASMGS_SETKEYS,
//	ASMGS_SETKEYSDONE,
} ASM_GROUPKEY_STATE, *PASM_GROUPKEY_STATE;

typedef enum _PORT_MODE_TYPE {
//	pmt_ForceUnauthorized,		// Rewrited as pmt_Disable by Annie
//	pmt_ForceAuthorized,		// Rewrited as pmt_Enable by Annie
	pmt_Disable,
	pmt_Enable,
	pmt_Auto
} PORT_MODE_TYPE, *PPORT_MODE_TYPE;

// defined by Annie, 2005-07-01.
typedef enum _PORT_CONTROL_TYPE {
	pct_Unauthorized,
	pct_Authorized,
	pct_Auto
} PORT_CONTROL_TYPE, *PPORT_CONTROL_TYPE;

// defined by Annie, 2005-07-01.
typedef enum _PORT_SECURE_TYPE {
	psec_Unauthorized,
	psec_Authorized
} PORT_SECURE_TYPE, *PPORT_SECURE_TYPE;


typedef struct _AUTH_PKEY_MGNT_TAG{
	
	//2 machine state

       ASM_PAIRWISEKEY_STATE	PrState;
	ASM_GROUPKEY_STATE		GrState;
	ASM_EVENT_ID			EvntID;

	//2 Variables

	struct _RT_WLAN_STA*	pWLanSTA;		// added by Annie

	//2004/06/29, kcwu, for WPA RSN-IE
//	u1Byte						IEBuf[1024];
//	OCTET_STRING				IE;
	RT_SECURITY_LEVEL			SecLvl;
	u1Byte						SecIEBuf[64];
	u1Byte						SecIELen;
	u2Byte						SecVersion;
	RT_ENC_ALG					GroupCipherSuite;
	RT_ENC_ALG					PairwiseCipherSuite[MAX_CIPHER_SUITE_NUM];
	u2Byte						PairwiseCipherCount;
	AKM_SUITE_TYPE				AuthSuite[MAX_AUTH_SUITE_NUM];
	u2Byte						AuthSuiteCount;	

	BOOLEAN						bPreAuth;
	u1Byte						NumOfPTKReplayCounter;
	u1Byte						NumOfGTKReplayCounter;

	PORT_MODE_TYPE				portMode;
	PORT_CONTROL_TYPE			portControl;
	PORT_SECURE_TYPE			portSecure;	// moved to global. Annie, 2005-07-01.

	u1Byte			TimeoutCtr;
	u8Byte			TimeSlot_sendstart;			// Record the time slot at sending 1st message in 4-way handshake.
	u8Byte			TimeSlot_lastsend;			// Record the time slot at received last EAPOL-key.
// 	//sc_yang
//	int				TickCnt;
//	BOOLEAN			L2Failure;
	BOOLEAN			MICVerified;

	BOOLEAN			Pair;					    // If Unicast cipher supported by Authenticator, set it TRUE. Jay 0708
	BOOLEAN			GInitAKeys;					// It is set to TRUE when the Group key update state machine is required. Jay 0708
	BOOLEAN			PInitAKeys;					// It is set to TRUE when the Authenticator is ready to send a Group key to its Supplicant after initialization. Jay 0708
	                                            // I use it to replace bPTKInstalled according to sepc. Annie, 2005-07-02.
//	OCTET_STRING	ANonce;
//	OCTET_STRING	SNonce; //added by Emily
	u8Byte			KeyReplayCounter;
	u1Byte			SNonce[KEY_NONCE_LEN];
	u1Byte			PTK[PTK_LEN];
	u1Byte			PTK_update[PTK_LEN];			// Used in key updateding. Added by Annie, 2005-07-12.
	u1Byte			ANonce[KEY_NONCE_LEN];

	pu1Byte			TempEncKey;				// Temporal Encryption Key
	pu1Byte			TxMICKey;					// Tx MIC Key
	pu1Byte			RxMICKey;					// Rx MIC Key
	
#if 1 //Added by Jay 0712 for security IV
	u8Byte			TxIV;
	u8Byte			RxIV;
	u8Byte			KeyRSC;
#endif
#if 1//Added by Jay 0713 for processing integrity failure
	u8Byte			TimeSlot_lastIntegrityFailed; 
#endif

	int				MicErrorCnt;		// Added by Annie for debug, 2005-07-25.
	int				WEPErrorCnt;

	//AP-WPA AES ,CCW
	u1Byte			AESKeyBuf[AESCCMP_BLK_SIZE_TOTAL];

}AUTH_PKEY_MGNT_TAG, *PAUTH_PKEY_MGNT_TAG;


#define	NumGroupKey	1	// Group key number: currently we use only one group key. Annie, 2005-06-28.


typedef struct _AUTH_GLOBAL_KEY_TAG
{

	int					currentId;		// Id for current authentication session
	int					receivedId;
	
        OCTET_STRING		EAPOLMsgRecvd;			//The Overall 802.1x message
        OCTET_STRING		EapolKeyMsgRecvd;		//The start point of eapol-key payload

	// Added by Annie, 2005-06-28.
	char					Passphrase[PASSPHRASE_LEN];		// define in char from u1Byte. Annie, 2005-07-01.
	u1Byte				PassphraseLen;
	u1Byte				PMK[PMK_LEN];
//	u1Byte				GMK[GMK_LEN];			//marked by Annie: It seems that we can use GTK directly.
	u1Byte				GTK[GTK_LEN];
	pu1Byte				TxMICKey;				// Group Tx MIC Key		// Added by Annie, 2005-07-06.
	pu1Byte				RxMICKey;				// Group Rx MIC Key
	u1Byte				AESGTK[AESCCMP_BLK_SIZE_TOTAL];
	
//	AUTH_GKEY_MGNT_TAG	groupKeyInfo;			//marked by Annie: It seems not necessary.
	u1Byte				ANonce[KEY_NONCE_LEN];
	u1Byte				GNonce[KEY_NONCE_LEN];
//	u8Byte				GroupReplayCounter;		// Added by Annie, 2005-07-02.
	u8Byte				KeyRSC;					// Added by Annie, 2005-07-02
	// End of Annie add.
	

	u1Byte				MaxRetryCounts;
	u1Byte				EventId;
	u1Byte				DescriptorType; //initialize to 254 in RSN
	u1Byte				KeyDescriptorVer;
//	u1Byte				CurrentAddress[ETHERNET_HEADER_SIZE];
	BOOLEAN				bMacAuthEnabled;

	RT_TIMER			KeyMgntTimer;			// added by Annie, 2005-06-29.
	u8Byte				CurrentTimeSlot;			// increase 1 per-second. Added by Annie, 2005-07-12.

	//RSNIE related variable
//	struct _DOT11_RSN_SUPPLICANT_VARIABLE   RSNVariable;
//	Dot11RSNConfigEntry		Dot11RSNConfig;

//	struct _Dot1x_Authenticator	*auth;
	//End Added WPA
#if 1 //Added by Jay 0713 for processing integrity failure
	u8Byte				TimeSlot_IntegrityFail2;			
#endif
} AUTH_GLOBAL_KEY_TAG, *PAUTH_GLOBAL_KEY_TAG;


#define 	KeyMgntStateIsWaitingEAPOLKey(_pKeyMgnt)									\
	(																		\
		( (_pKeyMgnt)->PrState == ASMPS_PTKSTART )				||			\
		( (_pKeyMgnt)->PrState == ASMPS_PTKINITNEGOTIATING )		||			\
		( (_pKeyMgnt)->GrState == ASMGS_REKEYNEGOTIATING )		||			\
		( (_pKeyMgnt)->GrState == ASMGS_REKEYESTABLISHED)     				\
	)

//
// Higher layer meaning and related context information for security purpose. 
// 2006.01.08, by rcnjko.
//
typedef	u4Byte	RT_SEC_PROT_INFO;
#define RT_SEC_NORMAL_DATA 0x00000000 // Default case.

// Note that! RT_SEC_EAPOL_BEFORE_KEY_INSTALLED and RT_SEC_EAPOL_AFTER_KEY_INSTALLED are mutually exclusive.
#define RT_SEC_EAPOL_BEFORE_KEY_INSTALLED 0x80000000 // EAPOL packet before PTK installed, e.g. 4-way handshake before PTK installed and so on.
#define RT_SEC_EAPOL_AFTER_KEY_INSTALLED 0x40000000 // EAPOL packet after PTK installed, e.g. 4-way handshake after PTK installed, 2-way handshake, MIC failure report and so on.

#define RT_SEC_MIC_FAILURE_REPORT 0x00000001 // MIC failure report frame.


//
// Helper macros to maniupate RT_SEC_PROT_INFO.
// 2006.01.08, by rcnjko.
//
#define IsSecProtEapolBeforeKeyInstalled(_SecProtInfo) \
	((_SecProtInfo & RT_SEC_EAPOL_BEFORE_KEY_INSTALLED) ? TRUE : FALSE)

#define IsSecProtEapolAfterKeyInstalled(_SecProtInfo) \
	((_SecProtInfo & RT_SEC_EAPOL_AFTER_KEY_INSTALLED) ? TRUE : FALSE)

#define IsSecProtEapol(_SecProtInfo) \
	((IsSecProtEapolAfterKeyInstalled(_SecProtInfo) || IsSecProtEapolBeforeKeyInstalled(_SecProtInfo)) ? TRUE : FALSE)

#define IsSecProtMicFailureReport(_SecProtInfo) \
	((_SecProtInfo & RT_SEC_MIC_FAILURE_REPORT) ? TRUE : FALSE)


//
// Per-packe encryption information structure.
// 2006.01.08, by rcnjko.
//
typedef struct _STA_ENC_INFO_T{
	RT_ENC_ALG			EncAlg;
	BOOLEAN				IsEncByHW;		// FALSE if this packet SHOULD NOT be encrypted by HW, TRUE otherwise. Note that, it is used in conjuntion with pMgntInfo->SecurityInfo.SWTxEncryptFlag to determine if we shall encrypt the packet by HW or SW.
	u1Byte				keyId;
	u2Byte                       ExemptionActionType; //0:Auto 1:encrypt 2:no encrypt 3: key UNAVAILABLE
	BOOLEAN				bMFPPacket;
	u4Byte				keylen;
	pu1Byte				keybuf;
	RT_SEC_PROT_INFO	SecProtInfo;	// Keep higher layer meaning and context information about this packet, see definition of RT_SEC_PROT_INFO for details.
}STA_ENC_INFO_T, *PSTA_ENC_INFO_T;

#define	IsCkipEnabled(_pMgntInfo) \
	( (_pMgntInfo->SecurityInfo.pCkipPara->bIsMIC || _pMgntInfo->SecurityInfo.pCkipPara->bIsKP)? TRUE : FALSE )


#endif // #ifndef __INC_SECURITYTYPE_H
