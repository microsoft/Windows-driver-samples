#ifndef __INC_WAPI_H
#define __INC_WAPI_H

   //add by ylb for WAPI IHV Support 20111114
#define DOT11_AUTH_ALGO_WAPI_PSK                           DOT11_AUTH_ALGO_IHV_START   + 0x00000010L
#define DOT11_AUTH_ALGO_WAPI_CERTIFICATE              DOT11_AUTH_ALGO_IHV_START   + 0x00000011L
#define DOT11_CIPHER_ALGO_WAPI_SMS4                       DOT11_CIPHER_ALGO_IHV_START + 0x00000010L

#define WAPI_END  	NULL
typedef enum _RT_WAPI_CALLBACK_FUNC_DEF
{
	//One Param
	WAPI_ENABLEWAPISUPPORT=0,
	WAPI_INIT=1,
	WAPI_RETURNALLCAMENTRY=2,	
	WAPI_RETURNALLSTAINFO=3,
	WAPI_FREEALLSTAINFO=4,
	WAPI_CONSTRUCTBEACON=5,
	//Two Param
	WAPI_CONSTRUCTASSOCIATEREQ=6,
	WAPI_CONSTRUCTPROBERSP=7,
	WAPI_ENABLEHWSECCONFIG=8,
	WAPI_MLMEONASSOCOK=9,
	WAPI_PRETRANSMITTCB=10,
	WAPI_PROCESSRECEIVEDPACKET=11,
	WAPI_DROPFORRXREORDER=12,
	WAPI_DROPFORSECKEYABSENT= 13,
	WAPI_RETURNONESTAINFO=14,
	WAPI_RETURNONESTACAMENTRY=15,
	WAPI_SECSWSMS4ENCRYPTION= 16,
	WAPI_SECSWSMS4DECRYPTION=17,
	//Three Param
	WAPI_SECHEADERFILLIV = 18,
	WAPI_SECISWAIPACKET = 19,
	WAPI_SETAUTHENCRYPTSTATE = 20,
	//four Param
	WAPI_SETCAMENTRY = 21,
	
	WAPI_MAX =100,
}RT_WAPI_FUNC_E;


#if (WAPI_SUPPORT == 1)
#define WAPI_MAXARGS  3
#define WAPI_QUERY   0
#define WAPI_SET       1
typedef enum _RT_WAPI_QUERYSET_VARIABLE
{
	WAPI_VAR_WAPISUPPORT,
	WAPI_VAR_WAPIENABLE,
	WAPI_VAR_WAPIIHVSUPPORT,
	WAPI_VAR_ISCAMUSED,
	WAPI_VAR_FIRSTAUTHENTIATEINPROGRESS,
	WAPI_VAR_NOTSETENCMACHEADER,
	WAPI_VAR_WAPIPSK,
	
	WAPI_VAR_MAX =255,
}RT_WAPI_QUERYSET_VARIABLE;

typedef enum _WAPI_REQUEST_TYPE
{
     WAPI_REQUEST_QUERY,  
     WAPI_REQUEST_SET,
     WAPI_REQUEST_METHOD,
} WAPI_REQUEST_TYPE, *PWAPI_REQUEST_TYPE;


typedef union _WAPI_REQUEST_DATA
{
    struct _WAPI_QUERY_INFO
    {
        ULONG               Oid;
        ULONG               BytesWritten;
        ULONG               BytesNeeded;
        ULONG               InformationBufferLength;
        UCHAR               InformationBuffer[1];
    } WAPI_QUERY_INFO;

    struct _WAPI_SET_INFO
    {
        ULONG               Oid;
        ULONG               BytesRead;
        ULONG               BytesNeeded;
        ULONG               InformationBufferLength;
        UCHAR                InformationBuffer[1];
    } WAPI_SET_INFO;

    struct _WAPI_METHOD_INFO
    {
        ULONG               Oid;
        ULONG               BytesWritten;
        ULONG               BytesNeeded;
        ULONG               BytesRead;
        ULONG               InputBufferLength;
        ULONG               InformationBufferLength;
        UCHAR               InformationBuffer[1];
    } WAPI_METHOD_INFO;

} WAPI_REQUEST_DATA, *PWAPI_REQUEST_DATA;


typedef struct _WAPI_REQUEST
{
    ULONG                   wapiRequestLen; // length of entire structure
    UCHAR                   wapiRequestOUI[4];     // vendor specific OUI value
    WAPI_REQUEST_TYPE            wapiRequestType;
    WAPI_REQUEST_DATA            wapiRequestData;
} WAPI_REQUEST, *PWAPI_REQUEST;


   //add by ylb for WAPI IHV Support 20111114
#define DOT11_AUTH_ALGO_WAPI_PSK                           DOT11_AUTH_ALGO_IHV_START   + 0x00000010L
#define DOT11_AUTH_ALGO_WAPI_CERTIFICATE              DOT11_AUTH_ALGO_IHV_START   + 0x00000011L
#define DOT11_CIPHER_ALGO_WAPI_SMS4                       DOT11_CIPHER_ALGO_IHV_START + 0x00000010L
 

#define WAPI_LITTLE_ENDIAN   
#define ENCRYPT  0     
#define DECRYPT  1 
typedef unsigned char muint8;
typedef unsigned short muint16;
typedef unsigned int muint32;

static const muint8 Sbox_WAPI[256] = {
0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05,
0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99,
0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62,
0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6,
0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8,
0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35,
0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87,
0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e,
0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1,
0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3,
0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f,
0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51,
0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8,
0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0,
0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84,
0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48
};

static const muint32 CK[32] = {
	0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
	0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
	0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
	0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
	0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
	0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
	0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
	0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279 };

#define Rotl(_x, _y) (((_x) << (_y)) | ((_x) >> (32 - (_y))))

#define ByteSub(_A) (Sbox_WAPI[(_A) >> 24 & 0xFF] << 24 ^ \
                     Sbox_WAPI[(_A) >> 16 & 0xFF] << 16 ^ \
                     Sbox_WAPI[(_A) >>  8 & 0xFF] <<  8 ^ \
                     Sbox_WAPI[(_A) & 0xFF])

#define L1(_B) ((_B) ^ Rotl(_B, 2) ^ Rotl(_B, 10) ^ Rotl(_B, 18) ^ Rotl(_B, 24))
#define L2(_B) ((_B) ^ Rotl(_B, 13) ^ Rotl(_B, 23))


/*---------------------------------------------------------------------------
WAPI Related OID
-----------------------------------------------------------------------------*/
#define	OID_RT_WAPI_GET_INFO							0xFFEDC201
#define	OID_RT_WAPI_SET_KEY							0xFFEDC202
#define	OID_RT_WAPI_GET_PN							0xFFEDC203
#define	OID_RT_WAPI_RECEIVE_PACKET						0xFFEDC204
#define	OID_RT_WAPI_SET_DESTNATION_ADDRESS 				        0xFFEDC205
#define	OID_RT_WAPI_SEND_WAI_PACKET						0xFFEDC206
#define	OID_WAPI_ASUE_CREATE_THREAD						0xFFEDC207
#define	OID_WAPI_ASUE_SET_MUTICAST_PN					        0xFFEDC208
#define OID_RT_WAPI_SUPPORT							0xFFEDC209
#define	OID_RT_WAPI_AE_START							0xFFEDC20A
#define	OID_RT_WAPI_IHV_SUPPORT		0xFFEDC20B //For WAPI IHV service support add  by ylb 20111114
#define	OID_RT_WAPI_ENABLE			0xFFEDC20C //For WAPI Enbale. add  by YJ, 20121112

#define	WAPI_MAX_BKID_NUM			64
#define	WAPI_MAX_RECEIVE_NUM		128
#define	WAPI_MAX_OPTION_NUM			64
#define	WAPI_CAM_ENTRY_NUM			14	// 28/2=14

//
// 2011/01/11 MH Add for WAPI callback function table.
//
typedef struct tag_Wapi_Handler_Def
{
	//char		*name;				/* Wapi function string */
	//char		*desc;				/* Description */
	//char		*usage;				/* Usage of the command */
	//int		directory;     			/* Directory */
	INT32		wapi_func_name; 	/* Use enum to define the function name. */
	RT_STATUS	(*wapi_func1)
				(	IN	PADAPTER		pAdapter); 
	RT_STATUS	(*wapi_func2)
				(	IN	PADAPTER		pAdapter,
					IN	PVOID			pWapiParam);
	RT_STATUS	(*wapi_func3)
				(	IN	PADAPTER		pAdapter,
					IN	PVOID			pWapiParam1,
					IN	PVOID			pWapiParam2);
	RT_STATUS	(*wapi_func4)
				(	IN	PADAPTER		pAdapter,
					IN	PVOID			pWapiParam1,
					IN	PVOID			pWapiParam2,
					IN	PVOID			pWapiParam3);
					/* WAPI function pointer */
}WAPI_T;

// 2011/01/12 MH Define All the WAPI functio from the entry point
//	typedef enum _RT_WAPI_CALLBACK_FUNC_DEF
//	{
//		//One Param
//		WAPI_ENABLEWAPISUPPORT=0,
//		WAPI_INIT=1,
//		WAPI_RETURNALLCAMENTRY=2,	
//		WAPI_RETURNALLSTAINFO=3,
//		WAPI_FREEALLSTAINFO=4,
//		WAPI_CONSTRUCTBEACON=5,
//		//Two Param
//		WAPI_CONSTRUCTASSOCIATEREQ=6,
//		WAPI_CONSTRUCTPROBERSP=7,
//		WAPI_ENABLEHWSECCONFIG=8,
//		WAPI_MLMEONASSOCOK=9,
//		WAPI_PRETRANSMITTCB=10,
//		WAPI_PROCESSRECEIVEDPACKET=11,
//		WAPI_DROPFORRXREORDER=12,
//		WAPI_DROPFORSECKEYABSENT= 13,
//		WAPI_RETURNONESTAINFO=14,
//		WAPI_RETURNONESTACAMENTRY=15,
//		WAPI_SECSWSMS4ENCRYPTION= 16,
//		WAPI_SECSWSMS4DECRYPTION=17,
//		//Three Param
//		WAPI_SECHEADERFILLIV = 18,
//		WAPI_SECISWAIPACKET = 19,
//		WAPI_SETAUTHENCRYPTSTATE = 20,
//		//four Param
//		WAPI_SETCAMENTRY = 21,
//		
//		WAPI_MAX =100,
//	}RT_WAPI_FUNC_E;


typedef struct  _RT_WAPI_BKID
{
	RT_LIST_ENTRY	list;
	u1Byte			bkid[16];
}RT_WAPI_BKID,*PRT_WAPI_BKID;

typedef struct  _RT_WAPI_KEY
{
	u1Byte			dataKey[16];
	u1Byte			micKey[16];
	u1Byte			keyId;
	BOOLEAN			bSet;
	BOOLEAN             bTxEnable;

	//For recording the write-cam idx.
	u1Byte			CamIdx;
}RT_WAPI_KEY,*PRT_WAPI_KEY;

typedef struct  _RT_WAPI_RECEIVE
{
	RT_LIST_ENTRY	list;
	BOOLEAN			bAuthenticator;
	u1Byte			recvBuff[1500];
	u2Byte			recvBuffLength;
	u1Byte			destAddr[6];
}RT_WAPI_RECEIVE,*PRT_WAPI_RECEIVE;

typedef  struct	 _RT_WAPI_OPTION
{
	RT_LIST_ENTRY	list;
	u1Byte			option;
	u1Byte			macAddr[6];
}RT_WAPI_OPTION,*PRT_WAPI_OPTION;

typedef enum _RT_WAPI_AE_OPTION
{
	WAPI_AE_NONE=0,
	WAPI_AE_RECV_WAI_PACKET=1,
	WAPI_AE_UPDATE_BK=2,
	WAPI_AE_UPDATE_USK=3,
	WAPI_AE_UPDATE_MSK=4,
	WAPI_AE_FIRST_AUTHENTICATOR=5,
	WAPI_AE_DISCONNECT=6
}RT_WAPI_AE_OPTION;

typedef enum _RT_WAPI_ASUE_OPTION
{
	WAPI_ASUE_NONE=0,
	WAPI_ASUE_RECV_WAI_PACKET=1,
	WAPI_ASUE_UPDATE_BK=2,
	WAPI_ASUE_UPDATE_USK=3,
	WAPI_ASUE_DISCONNECT=4
}RT_WAPI_ASUE_OPTION;

typedef enum _RT_WAPI_PACKET_TYPE
{
	WAPI_NONE = 0,
	WAPI_PREAUTHENTICATE=1,
	WAPI_STAKEY_REQUEST=2,
	WAPI_AUTHENTICATE_ACTIVE=3,
	WAPI_ACCESS_AUTHENTICATE_REQUEST=4,
	WAPI_ACCESS_AUTHENTICATE_RESPONSE=5,
	WAPI_CERTIFICATE_AUTHENTICATE_REQUEST=6,
	WAPI_CERTIFICATE_AUTHENTICATE_RESPONSE=7,
	WAPI_USK_REQUEST=8,
	WAPI_USK_RESPONSE=9,
	WAPI_USK_CONFIRM=10,
	WAPI_MSK_NOTIFICATION=11,
	WAPI_MSK_RESPONSE=12
}RT_WAPI_PACKET_TYPE;

typedef struct	_RT_WAPI_STA_INFO
{
	RT_LIST_ENTRY		list;
	u1Byte				PeerMacAddr[6];	
	RT_WAPI_KEY		      wapiUsk;
	RT_WAPI_KEY		      wapiUskUpdate;
	RT_WAPI_KEY		      wapiMsk;
	RT_WAPI_KEY		      wapiMskUpdate;	
	u1Byte				lastRxUnicastPN[16];
	u1Byte                       lastTxUnicastPN[16];
	u1Byte                       lastRxMulticastPN[16];
	u1Byte				lastRxUnicastPNBEQueue[16];
	u1Byte				lastRxUnicastPNBKQueue[16];
	u1Byte				lastRxUnicastPNVIQueue[16];
	u1Byte				lastRxUnicastPNVOQueue[16];
	BOOLEAN				bSetkeyOk;
	BOOLEAN				bAuthenticateInProgress;
	BOOLEAN				bAuthenticatorInUpdata;
	BOOLEAN				bAESetkeyOk;
}RT_WAPI_STA_INFO,*PRT_WAPI_STA_INFO;

//Added for HW wapi en/decryption
typedef struct _RT_WAPI_CAM_ENTRY{
	//RT_LIST_ENTRY		list;
	u1Byte			IsUsed;
	u1Byte			entry_idx;//for cam entry
	u1Byte			keyidx;	// 0 or 1,new or old key
	u1Byte			PeerMacAddr[6];
	u1Byte			type;	//should be 110,wapi
}RT_WAPI_CAM_ENTRY,*PRT_WAPI_CAM_ENTRY;

typedef struct _RT_WAPI_T
{
//WAPI IE
   	u1Byte		assoReqWapiIE[256];
	u1Byte		assoReqWapiIELength;
	u1Byte		assoRspWapiIE[256];
	u1Byte		assoRspWapiIELength;
	u1Byte		sendbeaconWapiIE[256];
	u1Byte		sendbeaconWapiIELength;
//BKID 
	RT_WAPI_BKID		wapiBKID[WAPI_MAX_BKID_NUM];
	RT_LIST_ENTRY		wapiBKIDIdleList;
	RT_LIST_ENTRY  		wapiBKIDStoreList;
//Key for Tx Multicast/Broadcast
	RT_WAPI_KEY		      wapiTxMsk;
//	RT_WAPI_KEY		      wapiTxMskUpdate;

//Option
	RT_WAPI_OPTION		wapiAEOption[WAPI_MAX_OPTION_NUM];
	RT_LIST_ENTRY		wapiAEOptionIdleList;
	RT_LIST_ENTRY		wapiAEOptionUsedList;
	RT_WAPI_OPTION		wapiASUEOption[WAPI_MAX_OPTION_NUM];
	RT_LIST_ENTRY		wapiASUEOptionIdleList;
	RT_LIST_ENTRY		wapiASUEOptionUsedList;
//Receive Packet
	RT_WAPI_RECEIVE		wapiRecvPacket[WAPI_MAX_RECEIVE_NUM];
	RT_LIST_ENTRY		wapiRxIdleList;
	RT_LIST_ENTRY		wapiRxBusyList;
	u1Byte				wapiDestMacAddr[6];
	BOOLEAN				bAuthenticator;
//sec related
	u1Byte				lastTxMulticastPN[16];
	RT_WAPI_STA_INFO	wapiSta[WAPI_MAX_OPTION_NUM];
	RT_LIST_ENTRY		wapiSTAIdleList;
	RT_LIST_ENTRY		wapiSTAUsedList;
//
	BOOLEAN				bWapiEnable;
	BOOLEAN				bUpdateUsk;
	BOOLEAN				bUpdateMsk;
//store WAPI IE
	u1Byte				wapiIE[256];
	u1Byte				wapiIELength;
	BOOLEAN				bWapiNotSetEncMacHeader;
	BOOLEAN				bWapiPSK;
	BOOLEAN				bFirstAuthentiateInProgress;
//last sequece number for wai packet
	u2Byte				wapiSeqnumAndFragNum;

//For HW wapi
	RT_WAPI_CAM_ENTRY	wapiCamEntry[WAPI_CAM_ENTRY_NUM];
	//RT_LIST_ENTRY		wapiCamIdleList;
	//RT_LIST_ENTRY		wapiCamUsedList;

// For WAPI IHV Support 
	u1Byte	 bIsWapiIHVSupport; 
	
}RT_WAPI_T,*PRT_WAPI_T;


typedef struct _WLAN_HEADER_WAPI_EXTENSION
{
    u1Byte      KeyIdx;
    u1Byte      Reserved;
    u1Byte      PN[16];

} WLAN_HEADER_WAPI_EXTENSION, *PWLAN_HEADER_WAPI_EXTENSION;

NDIS_STATUS
WAPI_EventQuerySetInformation(
    IN    PADAPTER                  pAdapter,
    IN    NDIS_OID                  Oid,
    IN    PVOID                     InformationBuffer,
    IN    ULONG                     InputBufferLength,
    IN    ULONG                     OutputBufferLength,
    IN    ULONG                     MethodId,
    OUT   PULONG                    BytesWritten,
    OUT   PULONG                    BytesRead,
    OUT   PULONG                    BytesNeeded
    );

void 
WAPI_OptionHandler(
	PADAPTER	Adapter,
	pu1Byte		DestAddr,
	BOOLEAN		bUpdateBK,
	BOOLEAN		bUpdateUSK,
	BOOLEAN		bUpdateMSK,
	BOOLEAN		bAERecvPacket,
	BOOLEAN		bASUERecvPacket,
	BOOLEAN		bDisconnect
);

RT_STATUS
WAPI_SecFuncHandler(
	IN	UINT32		WapiFunc,
	IN	PVOID		pWapiParam,
	...
);

RT_STATUS
WAPI_OidHandler(
	IN	PADAPTER		pAdapter,
	IN    u1Byte			PlatformFlag,
	IN    u1Byte			SetQuery,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWrittenOrRead,
	OUT	PULONG			BytesNeeded
	);

u1Byte 	
WAPI_QuerySetVariable(
	IN  	PADAPTER	Adapter,
	IN	u1Byte		QuerySet,
	IN  	u1Byte		VariableName,
	IN	u1Byte		VariableParam
	);

#else
#define WAPI_OptionHandler(Adapter, DestAddr, bUpdateBK, bUpdateUSK, bUpdateMSK, bAERecvPacket, bASUERecvPacket, bDisconnect)
#define WAPI_OidHandler(pAdapter,PlatformFlag,SetQuery,Oid,InformationBuffer,InformationBufferLength,BytesWrittenOrRead,BytesNeeded)  NDIS_STATUS_NOT_RECOGNIZED
#define WAPI_EventQuerySetInformation(pAdapter,Oid,InformationBuffer,InputBufferLength,OutputBufferLength,MethodId,BytesWritten,BytesRead,BytesNeeded) NDIS_STATUS_INVALID_OID
#define WAPI_QuerySetVariable(pAdapter, QuerySet,VariableName,VariableParam) 0


__inline RT_STATUS WAPI_SecFuncHandler(UINT32 WapiFunc, PVOID pWapiParam,...){ return RT_STATUS_NOT_SUPPORT;}

#endif // #ifndef __INC_SUPPLICANT_H
#endif 

