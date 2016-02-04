#ifndef __INC_PROTOCOL802_11_H
#define __INC_PROTOCOL802_11_H

#define MAX_DOT11_POWER_CONSTRAINT_IE_LEN	1

typedef struct _RT_RFD RT_RFD, *PRT_RFD;


// This enumeration is the list of the content appending type.
typedef	u4Byte	CONTENT_PKT_TYPE, *PCONTENT_PKT_TYPE;
#define	CONTENT_PKT_TYPE_802_11			BIT0
#define	CONTENT_PKT_TYPE_IES			BIT1
#define	CONTENT_PKT_TYPE_CLIENT			BIT2
#define	CONTENT_PKT_TYPE_AP				BIT3
#define	CONTENT_PKT_TYPE_IBSS			BIT4



#define sAckCtsLng					112		// bits in ACK and CTS frames
#define sCrcLng						4		// octets for crc32 (FCS, ICV)
#define aSifsTime					((IS_WIRELESS_MODE_5G(Adapter)|| IS_WIRELESS_MODE_N_24G(Adapter))? 16 : 10)
#define sMacHdrLng					24		// octets in data header, no WEP
#define sMaxMpduLng					2346

// This value is tested by WiFi 11n Test Plan 5.2.3.
// This test verifies the WLAN NIC can update the NAV through sending the CTS with large duration.
#define	WiFiNavUpperUs				30000	// 30 ms

#define Frame_ProtocolVer(f)				( GET_80211_HDR_PROTOCOL_VERSION ((f).Octet) )
#define Frame_Type(f)					( GET_80211_HDR_TYPE ((f).Octet) )
#define Frame_Subtype(f)				( GET_80211_HDR_SUBTYPE ((f).Octet)	)
#define Frame_ToDS(f)					( GET_80211_HDR_TO_DS ((f).Octet)	)
#define Frame_FromDS(f)					( GET_80211_HDR_FROM_DS ((f).Octet)	)
#define Frame_MoreFrag(f)				( GET_80211_HDR_MORE_FRAG ((f).Octet)	)
#define Frame_Retry(f)					( GET_80211_HDR_RETRY ((f).Octet)	)
#define Frame_PwrMgt(f)					( GET_80211_HDR_PWR_MGNT ((f).Octet)	)
#define Frame_MoreData(f)				( GET_80211_HDR_MORE_DATA ((f).Octet)	)
#define Frame_WEP(f)					( GET_80211_HDR_WEP ((f).Octet)	)
#define Frame_Order(f)					( GET_80211_HDR_ORDER ((f).Octet)	)
#define Frame_DurID(f)					( EF2Byte( *((UNALIGNED pu2Byte)((f).Octet+2)) ) )
#define Frame_Addr1(f)					( (f).Octet+4 )									// return pu1Byte
#define Frame_Addr2(f)					( (f).Octet+10 )								// return pu1Byte
#define Frame_Addr3(f)					( (f).Octet+16 )								// return pu1Byte
#define Frame_FragNum(f)				( GET_80211_HDR_FRAGMENT( (f).Octet ))
#define Frame_SeqNum(f)					( GET_80211_HDR_SEQUENCE( (f).Octet ))
#define Frame_Addr4(f)					( (f).Octet+24 )								// return pu1Byte
#define Frame_ValidAddr4(f)				( Frame_ToDS(f)&&Frame_FromDS(f) )
#define Frame_ContainQc(f) 				( ( IsDataFrame((f).Octet) ) && (Frame_Subtype(f) & 0x08) )

#define Frame_ContainHTC(f) 				( 	(( IsQoSDataFrame((f).Octet) ) && (Frame_Order(f) & 0x1) ) || \
											(( IsMgntFrame((f).Octet) ) && (Frame_Order(f) & 0x1) ) )


#define Frame_FrameBody(f)				( Frame_Addr4(f)+6*Frame_ValidAddr4(f) )	// return pu1Byte
#define Frame_FrameHdrLng(f)			( sMacHdrLng + 6*Frame_ValidAddr4(f) +2*IsQoSDataFrame((f).Octet) )	// return lenght of 802.11 data frame header.

#define Frame_GAS_QueryReq(f)			( Frame_FrameBody(f) + 1 + 1 + 1 + 4 + 2 ) 
//Category(1 byte), Action(1 byte), DialogToken(1 byte), Advertisement protocol IE(4 bytes), Query Request Length(2 bytes)

#define Frame_GAS_QueryReq_OUI(f)		( Frame_GAS_QueryReq(f) + 2 + 2 ) 
//Info ID(2 bytes), Length(2 bytes)

#define Frame_GAS_QueryReq_Type(f)		( Frame_GAS_QueryReq_OUI(f) + 3 ) 
//OUI(3 bytes)

#define Frame_GAS_QueryRsp(f)			( Frame_FrameBody(f) + 1 + 1 + 1 + 2 + 2 + 4 + 2 ) 
//Category(1 byte), Action(1 byte), DialogToken(1 byte), Status code(2 bytes), GAS Comeback Delay(2 bytes), Advertisement protocol IE(4 bytes), Query Response Length(2 bytes)

#define Frame_GAS_QueryRsp_OUI(f)		( Frame_GAS_QueryRsp(f) + 2 + 2 ) 
//Info ID(2 bytes), Length(2 bytes)

#define Frame_GAS_QueryRsp_Type(f)		( Frame_GAS_QueryRsp_OUI(f) + 3 ) 
//OUI(3 bytes)

#define Frame_GAS_ComebackRsp(f)		( Frame_FrameBody(f) + 1 + 1 + 1 + 2 + 1 + 2 + 4 + 2 ) 
//Category(1 byte), Action(1 byte), DialogToken(1 byte), Status code(2 bytes), GAS Query Response Fragment ID(1 byte), GAS Comeback Delay(2 bytes), Advertisement protocol IE(4 bytes), Query Response Length(2 bytes)

#define Frame_GAS_ComebackRsp_OUI(f)	( Frame_GAS_ComebackRsp(f) + 2 + 2 ) 
//Info ID(2 bytes), Length(2 bytes)

#define Frame_GAS_ComebackRsp_Type(f)	( Frame_GAS_ComebackRsp_OUI(f) + 3 ) 
//OUI(3 bytes)

#define Frame_QoSTID(f, offset)				(EF1Byte( *((f).Octet +offset )) & 0x0f)
#define Frame_IVKeyID(f,offset)				( EF1Byte(*((f).Octet+offset)) <<6 & 0x03)

#define Frame_AuthAlgorithmNum(f)			EF2Byte( *((UNALIGNED pu2Byte)( Frame_FrameBody(f) + (Frame_WEP(f)?4:0) )) )
#define Frame_AuthTransactionSeqNum(f)			EF2Byte( *((UNALIGNED pu2Byte)( Frame_FrameBody(f) + 2 + (Frame_WEP(f)?4:0)) ) )
#define Frame_AuthStatusCode(f)				EF2Byte( *((UNALIGNED pu2Byte)( Frame_FrameBody(f) + 4 + (Frame_WEP(f)?4:0)) ) )
#define Frame_AuthChallengeText(f)			( Frame_FrameBody(f) + 6 + (Frame_WEP(f)?4:0) )					// return pu1Byte

#define Frame_AssocCap(f)				EF2Byte( *(	(UNALIGNED pu2Byte)(Frame_FrameBody(f)) ) )
#define Frame_AssocStatusCode(f)			EF2Byte( *(	(UNALIGNED pu2Byte)(Frame_FrameBody(f)+2) ) )
#define Frame_AssocAID(f)				EF2Byte( *(	(UNALIGNED pu2Byte)(Frame_FrameBody(f)+4) ) & 0x3FFF )

#define Frame_DeassocReasonCode(f)			EF2Byte( *( (UNALIGNED pu2Byte)(Frame_FrameBody(f)) ) )
#define Frame_DeauthReasonCode(f)			EF2Byte( *( (UNALIGNED pu2Byte)(Frame_FrameBody(f)) ) )


#define Frame_Addr1OS(a,f)				{ CopyMem((a).Octet, (f).Octet+4, 6); (a).Length=6; }
#define Frame_Addr2OS(a,f)				{ CopyMem((a).Octet, (f).Octet+10, 6); (a).Length=6; }
#define Frame_Addr3OS(a,f)				{ CopyMem((a).Octet, (f).Octet+16, 6); (a).Length=6; }


#define FillOctetString(_os,_octet,_len)		\
	(_os).Octet=(pu1Byte)(_octet);			\
	(_os).Length=(_len);

#define IsFrameTypeMgnt(pframe)			( ((EF1Byte(pframe[0]) & 0x0C)==0x00) ? TRUE : FALSE )
#define IsFrameTypeCtrl(pframe)			( ((EF1Byte(pframe[0]) & 0x0C)==0x04) ? TRUE : FALSE )
#define IsFrameTypeData(pframe)			( ((EF1Byte(pframe[0]) & 0x0C)==0x08) ? TRUE : FALSE )
#define IsFrameWithAddr4(pframe)		( ((EF1Byte(pframe[1]) & 0x03)==0x03) ? TRUE : FALSE )

#define PacketGetType(_packet)			(EF1Byte((_packet).Octet[0]) & 0xFC)

#define Frame_pRaddr(pduOS)			Frame_Addr1(pduOS)
#define Frame_pTaddr(pduOS)			Frame_Addr2(pduOS)

#define	Frame_pDaddr(pduOS)			( (pduOS).Octet + ( Frame_ToDS(pduOS) ? 16 : 4 ) )
#define	Frame_pSaddr(pduOS)			( (pduOS).Octet + \
									((Frame_FromDS(pduOS) == 0) ? \
										10 : ((Frame_ToDS(pduOS) == 0) ? \
												16 : 24)) )

#define	Frame_pBssid(pduOS)			( (pduOS).Octet + \
									(Frame_ToDS(pduOS) ? \
										4 : (Frame_FromDS(pduOS) ? \
												10 : 16)) )

#define FrameAction_Cat_Act(f)			EF2Byte( *(	(UNALIGNED pu2Byte)(Frame_FrameBody(f)) ) )

// Time Unit (TU): 1024 us
#define sTU 1024
// Convert TU to the unit of 4us
#define	TU_TO_US4(__TU)		((__TU) << 8)
// Convert the unit of 4us to TU
#define	US4_TO_TU(__US)		((__US) >> 8)

#define MMPDU_BODY(__pMmpduBuf) ((pu1Byte)(__pMmpduBuf) + sMacHdrLng)
#define MMPDU_BODY_LEN(__mmpduLen) (((__mmpduLen) > sMacHdrLng) ? ((__mmpduLen) - sMacHdrLng) : 0)

//----------------------------------------------------------------------------
//      802.11 Management frame Status Code field
//----------------------------------------------------------------------------
typedef struct _OCTET_STRING{
	pu1Byte		Octet;
	u2Byte		Length;
	BOOLEAN		bDefaultStr;
}OCTET_STRING, *POCTET_STRING;


#define	SIZE_EID_AND_LEN	2	// Size of Element ID and Length. Added by Roger, 2006.12.07.
#define	SIZE_OUI			3	// Size of OUI. Added by Roger, 2006.12.07.
#define	SIZE_OUI_TYPE		1	// Size of OUI type.
#define	SIZE_TYPE_AND_LEN	2	// Size of "8186 Realtek Information Element" Type and Length. Added by Roger, 2006.12.07.

//
// Max length of variable element defined in 802.11 spec.
//
#define MAX_IE_LEN			0xFF
#define MAX_SSID_LEN		32
#define MAX_SUP_RATE_LEN	8
#define MAX_FH_PARM_LEN		5
#define MAX_DS_PARM_LEN		1
#define MAX_CF_PARM_LEN		6
#define MAX_TIM_PARM_LEN	254
#define MAX_IBSS_PARM_LEN	2
#define MAX_QBSS_LOAD_LEN	5
#define MAX_EDCA_PARM_LEN	18
#define MAX_TSPEC_LEN		55
#define MAX_SCHEDULE_LEN	14
#define MAX_CTEXT_LEN		253
#define MAX_ERP_INFO_LEN	1
#define MAX_TS_DELAY_LEN	4
#define MAX_TC_PROC_LEN		1
#define MAX_HT_CAP_LEN		26
#define MAX_HT_INFO_LEN         22
#define MAX_QOS_CAP			1
#define MAX_EXT_SUP_RATE_LEN 255
#define MAX_LINKID_LEN		18
//#define MAX_EXTCAP_LEN		8	// Do not define ext capabilities length because it is a variable in 802.11 sepc.
#define MAX_SUPCHNL_LEN	127
#define MAX_SUPREGULATORY_LEN	255
#define MAX_SECONDARYOFFSET_LEN	1
#define MAX_CHNLSWITCHTIMING_LEN	4
#define DEAUTH_REASON_SIZE		2
#define DISASSOC_REASON_SIZE	2
#define MAX_VHT_CAP_LEN	12
#define MAX_WAPI_IE_LEN		255
#define	MAX_MD_IE_LEN			3
#define	MAX_FT_IE_LEN			255
#define	MIN_FT_IE_LEN			82

static u1Byte WFA_OUI[SIZE_OUI] = {0x50, 0x6F, 0x9A};


static u1Byte MAC_BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};



typedef	enum _ELEMENT_ID{
	EID_SsId			= 0, 	/* service set identifier (0:32) */
	EID_SupRates		= 1,	/* supported rates (1:8) */
	EID_FHParms		= 2, 	/* FH parameter set (5) */
	EID_DSParms		= 3, 	/* DS parameter set (1) */
	EID_CFParms		= 4, 	/* CF parameter set (6) */
	EID_Tim				= 5,	/* Traffic Information Map (4:254) */
	EID_IbssParms		= 6,	/* IBSS parameter set (2) */
	EID_Country			= 7,	/* */

	// Form 7.3.2: Information elements in 802.11E/D13.0, page 46.
	EID_QBSSLoad		= 11,
	EID_EDCAParms		= 12,
	EID_TSpec			= 13,
	EID_TClass			= 14,
	EID_Schedule		= 15,
	//
	
	EID_Ctext			= 16,	/* challenge text*/
	EID_POWER_CONSTRAINT = 32,	/* Power Constraint*/

	//vivi for WIFITest, 802.11h AP, 20100427
	// 2010/12/26 MH The definition we can declare always!!
	EID_PowerCap			= 33,
	EID_SupportedChannels 	= 36,
	EID_ChlSwitchAnnounce 	= 37,

	EID_MeasureRequest		= 38, // Measurement Request
	EID_MeasureReport		= 39, // Measurement Report
	
	EID_ERPInfo 			= 42,

	// Form 7.3.2: Information elements in 802.11E/D13.0, page 46.
	EID_TSDelay			= 43,
	EID_TCLASProc			= 44,
	EID_HTCapability		= 45,
	EID_QoSCap				= 46,
	//
	
	EID_WPA2				= 48,
	EID_ExtSupRates		= 50,

	EID_FTIE				= 55,	// Defined in 802.11r
	EID_Timeout			= 56,	// Defined in 802.11r
	
	EID_SupRegulatory		= 59,	// Supported Requlatory Classes 802.11y
	EID_HTInfo 				= 61,
	EID_SecondaryChnlOffset	= 62,
	EID_WAPI				= 68,
	
	EID_BSSCoexistence		= 72, 	// 20/40 BSS Coexistence
	EID_BSSIntolerantChlReport = 73,
	EID_OBSS				= 74,	// Overlapping BSS Scan Parameters
	
	EID_LinkIdentifier		= 101,	// Defined in 802.11z
	EID_WakeupSchedule	= 102,	// Defined in 802.11z
	EID_ChnlSwitchTimeing	= 104,	// Defined in 802.11z
	EID_PTIControl			= 105,	// Defined in 802.11z
	EID_PUBufferStatus		= 106,	// Defined in 802.11z
	
	EID_EXTCapability		= 127,	// Extended Capabilities
	// From S19:Aironet IE and S21:AP IP address IE in CCX v1.13, p16 and p18.
	EID_Aironet				= 133,	// 0x85: Aironet Element for Cisco CCX
	EID_CiscoIP				= 149,	// 0x95: IP Address IE for Cisco CCX

	EID_CellPwr				= 150,	// 0x96: Cell Power Limit IE. Ref. 0x96.

	EID_CCKM				= 156, 

	EID_VHTCapability 		= 191,	// Based on 802.11ac D2.0
	EID_VHTOperation 		= 192,	// Based on 802.11ac D2.0
	EID_OpModeNotification	= 199,	// Based on 802.11ac D3.0

	EID_Vendor				= 221,	// 0xDD: Vendor Specific
}ELEMENT_ID, *PELEMENT_ID;

/* Action frame categories (IEEE 802.11-2007, 7.3.1.11, Table 7-24) */
#define WLAN_ACTION_SPECTRUM_MGMT 0
#define WLAN_ACTION_QOS 1
#define WLAN_ACTION_DLS 2
#define WLAN_ACTION_BLOCK_ACK 3
#define WLAN_ACTION_PUBLIC 4
#define WLAN_ACTION_RADIO_MEASUREMENT 5
#define WLAN_ACTION_FT 6
#define WLAN_ACTION_HT 7
#define WLAN_ACTION_SA_QUERY 8
#define WLAN_ACTION_PROTECTED_DUAL 9
#define WLAN_ACTION_WNM 10
#define WLAN_ACTION_UNPROTECTED_WNM 11
#define WLAN_ACTION_TDLS 12
#define WLAN_ACTION_WMM 17 /* WMM Specification 1.1 */
#define WLAN_ACTION_VENDOR_SPECIFIC 127

/* Public action codes */
#define WLAN_PA_20_40_BSS_COEX 0
#define WLAN_PA_VENDOR_SPECIFIC 9
#define WLAN_PA_GAS_INITIAL_REQ 10
#define WLAN_PA_GAS_INITIAL_RESP 11
#define WLAN_PA_GAS_COMEBACK_REQ 12
#define WLAN_PA_GAS_COMEBACK_RESP 13
#define WLAN_TDLS_DISCOVERY_RESPONSE 14

/* Protected Dual of Public Action frames */
#define WLAN_PROT_DSE_ENABLEMENT 1
#define WLAN_PROT_DSE_DEENABLEMENT 2
#define WLAN_PROT_EXT_CSA 4
#define WLAN_PROT_MEASUREMENT_REQ 5
#define WLAN_PROT_MEASUREMENT_REPORT 6
#define WLAN_PROT_DSE_POWER_CONSTRAINT 8
#define WLAN_PROT_VENDOR_SPECIFIC 9
#define WLAN_PROT_GAS_INITIAL_REQ 10
#define WLAN_PROT_GAS_INITIAL_RESP 11
#define WLAN_PROT_GAS_COMEBACK_REQ 12
#define WLAN_PROT_GAS_COMEBACK_RESP 13

typedef struct _RT_DOT11_IE{
	u1Byte			Id;
	OCTET_STRING	Content;
}RT_DOT11_IE, *PRT_DOT11_IE;


//
// Define the OUI sub type enumeration to distinguish WPA-IE and WMM-IE.
// Added by Annie, 2005-11-08.
//
typedef	enum _OUI_SUBTYPE{
	OUI_SUB_DONT_CARE,
	OUI_SUB_WPA,
	OUI_SUB_WMM,
	OUI_SUB_CCX_RM_CAP, 		// For CCX 2 S36, Radio Management Capability element, 2006.05.15, by rcnjko.
	OUI_SUB_CCX_VER_NUM, 		// For CCX 2 S38, WLAN Device Version Number element. Annie, 2006-08-20.
	OUI_SUB_REALTEK_TURBO,		// (Asked by SD4). Added for 8186 AutoTurbo mode. Annie, 2005-12-27.
	OUI_SUB_REALTEK_AGG,
	OUI_SUB_SimpleConfig,
	OUI_SUB_EPIG_IE,			// 00:90:4c
	OUI_SUB_11N_EWC_HT_CAP,		// 00:90:4c:33
	OUI_SUB_11N_EWC_HT_INFO,	// 00:90:4c:34
	OUI_SUB_11AC_EPIG_VHT_CAP,	// 00:90:4c:04:08
	OUI_SUB_BROADCOM_IE_1, 		// 208/01/25 MH to make sure broadcom AP only
	OUI_SUB_BROADCOM_IE_2,
	OUI_SUB_BROADCOM_IE_3,
	OUI_SUB_BROADCOM_LINKSYSE4200_IE_1,	// 2012/04/28 zhiyuan to make sure Linksys E4200 AP 5G only
	OUI_SUB_BROADCOM_LINKSYSE4200_IE_2,
	OUI_SUB_BROADCOM_LINKSYSE4200_IE_3,	
	OUI_SUB_CISCO_IE,					// 2008/05/12 Emily. Recognize Cisco AP
	OUI_SUB_MERU_IE,
	OUI_SUB_RALINK_IE,
	OUI_SUB_ATHEROS_IE_1,
	OUI_SUB_ATHEROS_IE_2,
	OUI_SUB_WPA2GTK,
	OUI_SUB_MARVELL,
	OUI_SUB_AIRGO,
	OUI_SUB_SSIDL,						// For CCX v4 s53 SSIDL,070702 by CCW  
	OUI_SUB_CCX_TSM,					// For CCX4 S56, Traffic Stream Metrics, 070615, by rcnjko.
	OUI_SUB_CCX_SFA,					// For CCX5, CCX Support Features Advertisement
	OUI_SUB_CCX_DIAG_REQ_REASON,		// For CCX5, CCX Diagnostic Channel Request Reason
	OUI_SUB_CCX_MFP_MHDR,				// For CCX5, CCX Manggement frame Header IE (MHDRIE)

	OUI_SUB_WIFI_DIRECT,
	OUI_SUB_WIFI_DISPLAY,				// WiFi-Display
	OUI_SUB_REALTEK_TDLS,				// For 802.11z TDLS realtek tag
	OUI_SUB_REALTEK_BT_IOT,
	OUI_SUB_REALTEK_BT_HS,
	OUI_SUB_NAN,
} OUI_TYPE,*POUI_TYPE;

// define OUI subtype for WMM and WMMSA
#define OUI_SUBTYPE_DONT_CARE		255
#define OUI_SUBTYPE_WMM_INFO		0
#define OUI_SUBTYPE_WMM_PARAM	1
#define OUI_SUBTYPE_TSPEC			2
#define OUI_SUBTYPE_QOS_CAPABI	5
#define OUI_SUBTYPE_TCLAS			6
#define OUI_SUBTYPE_TCLAS_PROC	7
#define OUI_SUBTYPE_TS_DELAY		8
#define OUI_SUBTYPE_SCHEDULE		9
#define OUI_SUBTYPE_TSINFO			10

typedef enum _CAPABILITY{
	cESS			= 0x0001,
	cIBSS			= 0x0002,
	cPollable		= 0x0004,
	cPollReq		= 0x0008,
	cPrivacy		= 0x0010,
	cShortPreamble	= 0x0020,
	cPBCC			= 0x0040,
	cChannelAgility	= 0x0080,
	cSpectrumMgnt	= 0x0100,
	cQos			= 0x0200,	// For HCCA, use with CF-Pollable and CF-PollReq
	cShortSlotTime	= 0x0400,
	cAPSD			= 0x0800,
	cRM				= 0x1000,	// RRM (Radio Request Measurement)
	cDSSS_OFDM		= 0x2000,
	cDelayedBA		= 0x4000,
	cImmediateBA	= 0x8000,
}CAPABILITY, *PCAPABILITY;

typedef	enum	_TYPE_SUBTYPE{
	// Management Frame
	Type_Asoc_Req		= 0x00,
	Type_Asoc_Rsp		= 0x10,
	Type_Reasoc_Req	= 0x20,
	Type_Reasoc_Rsp	= 0x30,
	Type_Probe_Req		= 0x40,
	Type_Probe_Rsp		= 0x50,
	Type_Beacon		= 0x80,
	Type_Atim			= 0x90,
	Type_Disasoc		= 0xa0,
	Type_Auth			= 0xb0,
	Type_Deauth		= 0xc0,
	Type_Action			= 0xd0,
	Type_Action_No_Ack	= 0xe0,
	
	// Control Frame
	Type_Beamforming_Report_Poll = 0x44, //= MkString(S8(0,0,1,0,0,0,1,0));
	Type_NDPA			= 0x54,//= MkString(S8(0,0,1,0,1,0,1,0));
	Type_BlockAckReq	= 0x84,//= MkString(S8(0,0,1,0,0,0,0,1));
	Type_BlockAck		= 0x94,//= MkString(S8(0,0,1,0,1,0,0,1));
	Type_PS_poll		= 0xa4,//= MkString(S8(0,0,1,0,0,1,0,1));
	Type_RTS			= 0xb4,//= MkString(S8(0,0,1,0,1,1,0,1));
	Type_CTS			= 0xc4,//= MkString(S8(0,0,1,0,0,0,1,1));
	Type_Ack			= 0xd4,//= MkString(S8(0,0,1,0,1,0,1,1));
	Type_Cfend			= 0xe4,//= MkString(S8(0,0,1,0,0,1,1,1));
	Type_Cfend_Ack		= 0xf4,//= MkString(S8(0,0,1,0,1,1,1,1));

	// Data Frame
	Type_Data			= 0x08,//= MkString(S8(0,0,0,1,0,0,0,0));
	Type_Data_Ack		= 0x18,//= MkString(S8(0,0,0,1,1,0,0,0));
	Type_Data_Poll		= 0x28,//= MkString(S8(0,0,0,1,0,1,0,0));
	Type_Data_Poll_Ack	= 0x38,//= MkString(S8(0,0,0,1,1,1,0,0));
	Type_Null_Frame	= 0x48,//= MkString(S8(0,0,0,1,0,0,1,0));
	Type_Cfack			= 0x58,//= MkString(S8(0,0,0,1,1,0,1,0));
	Type_Cfpoll			= 0x68,//= MkString(S8(0,0,0,1,0,1,1,0));
	Type_Cfpoll_Ack		= 0x78,//= MkString(S8(0,0,0,1,1,1,1,0));
	Type_QosData		= 0x88,//= MkString(S8(0,0,0,1,0,0,0,1));
	Type_QData_Ack		= 0x98,//= MkString(S8(0,0,0,1,1,0,0,1));
	Type_QData_Poll		= 0xa8,//= MkString(S8(0,0,0,1,0,1,0,1));
	Type_QData_Poll_Ack= 0xb8,//= MkString(S8(0,0,0,1,1,1,0,1));
	Type_QosNull		= 0xc8,//= MkString(S8(0,0,0,1,0,0,1,1));
	// Note: 0xd8 is reserved in 11e/13.0.
	Type_QosCfpoll		= 0xe8,//= MkString(S8(0,0,0,1,0,1,1,1));
	Type_QosCfpoll_Ack	= 0xf8,//= MkString(S8(0,0,0,1,1,1,1,1));
}TYPE_SUBTYPE, *PTYPE_SUBTYPE;


typedef enum _ERP_T{
	ERP_NonERPpresent	= 0x01,
	ERP_UseProtection	= 0x02,
	ERP_BarkerPreambleMode = 0x04,
} ERP_T;

typedef	enum _AUTH_ALGORITHM{
	OPEN_SYSTEM = 0,
	SHARED_KEY      = 1,
	AUTH_ALG_FT = 2,
	NETWORK_EAP   = 0x80,
}AUTH_ALGORITHM, *PAUTH_ALGORITHM;

typedef enum _ACT_CATEGORY{
	ACT_CAT_SPECTRUM_MGNT = 0,		// Spectrum management
	ACT_CAT_QOS	= 1,				// Qos
	ACT_CAT_DLS	= 2,				// Direct Link Protocol (DLS)
	ACT_CAT_BA = 3,					// Block Ack
	ACT_CAT_PUBLIC = 4,				// Public
	ACT_CAT_RM = 5,					// Radio Measurement (RM)
	ACT_CAT_FT = 6,					// Fast BSS Transition
	ACT_CAT_HT = 7,					// High Throughput
	ACT_CAT_SAQ = 8,				// Security Association Query
	ACT_CAT_SAQ_PD_PUBLIC = 9,		// Protected Dual of Public Action
	ACT_CAT_TDLS 	= 12,				// Tunneled Direct Link Setup
	ACT_CAT_WMM	= 17,				// WMM
	ACT_CAT_VHT	= 21, 				// VHT
	ACT_CAT_VENDOR_PROTECT = 126,	// Vendor-specific Protected
	ACT_CAT_VENDOR = 127,			// Vendor-specific
} ACT_CATEGORY, *PACT_CATEGORY;

typedef enum _ACT_QOS_ACTION
{
	ACT_QOS_ADDTSREQ	= 0,
	ACT_QOS_ADDTSRSP	= 1,
	ACT_QOS_DELTS		= 2,
	ACT_QOS_SCHEDULE	= 3,
}ACT_QOS_ACTION, *PACT_QOS_ACTION;

typedef enum _ACT_DLS_ACTION{
	ACT_DLS_REQ 			= 0,
	ACT_DLS_RSP 			= 1,
	ACT_DLS_TEARDOWN		= 2,
} ACT_DLS_ACTION, *PACT_DLS_ACTION;

typedef enum _BA_ACTION{
	ACT_ADDBAREQ 	= 0,
	ACT_ADDBARSP 	= 1,
	ACT_DELBA		= 2,
} BA_ACTION, *PBA_ACTION;

typedef enum _PUBLIC_ACTION{
	ACT_PUBLIC_BSSCOEXIST = 0, // 20/40 BSS Coexistence
	ACT_PUBLiC_MP = 7, // Measurement Pilot
	ACT_PUBLIC_TDLS_DISCOVERYRSP = 14,
}PUBLIC_ACTION, *PPUBLIC_ACTION;

typedef enum _HT_ACTION{
	ACT_RECOMMAND_WIDTH		= 0,
	ACT_MIMO_PWR_SAVE 		= 1,
	ACT_PSMP					= 2,
	ACT_SET_PCO_PHASE		= 3,
	ACT_MIMO_CHL_MEASURE	= 4,
	ACT_RECIPROCITY_CORRECT	= 5,
	ACT_MIMO_CSI_MATRICS		= 6,
	ACT_MIMO_NOCOMPR_STEER	= 7,
	ACT_MIMO_COMPR_STEER		= 8,
	ACT_ANTENNA_SELECT		= 9,
} HT_ACTION, *PHT_ACTION;

// SA Query Action frame
typedef enum _SA_ACTION{
	ACT_SA_REQ		= 0,
	ACT_SA_RSP 	= 1,
} SA_ACTION, *PSA_ACTION;


typedef enum _RM_ACTION{
	ACT_RM_REQUEST = 0, // Radio Measurement Request
	ACT_RM_REPORT = 1, // Radio Measurement Report
	ACT_RM_LINK_MEASURE_REQUEST = 2, // Link Measurement Request
	ACT_RM_LINK_MEASURE_REPORT = 3, // Link Measurement Report
	ACT_RM_NEIGHBOR_RRT_REQUEST = 4, // Neighbor Report Request
	ACT_RM_NEIGHBOT_RPT_RESPONSE = 5, // Neighbor Report Response
}RM_ACTION, *PRM_ACTION;

typedef enum _TDLS_ACTION{
	ACT_TDLS_SETUPREQ		= 0,
	ACT_TDLS_SETUPRSP			= 1,
	ACT_TDLS_SETUPCONFIRM	= 2,
	ACT_TDLS_TEARDOWN		= 3,
	ACT_TDLS_PEERTRAFFICIND	= 4,
	ACT_TDLS_CHNLSWITCHREQ	= 5,
	ACT_TDLS_CHNLSWITCHRSP	= 6,
	ACT_TDLS_PEERPSMREQ		= 7,
	ACT_TDLS_PEERPSMRSP		= 8,
	ACT_TDLS_PEERTRAFFICRSP	= 9,
	ACT_TDLS_DISCOVERYREQ	= 10,
	ACT_TDLS_MAX				= 11,
}TDLS_ACTION, *PTDLS_ACTION;


//
// WMM Management Action Frame Codes
//
typedef enum _ACTION_CODE{
	ADDTS_REQ = 0,
	ADDTS_RSP = 1,
	DELTS = 2,
}ACTION_CODE, *PACTION_CODE;

typedef enum _VHT_ACTION{
	ACT_VHT_COMPRESSED_BEAMFORMING	= 0,
	ACT_VHT_GROUPID_MANAGEMENT			= 1,
	ACT_VHT_OPMODE_NOTIFICATION		= 2,
}VHT_ACTION, *PVHT_ACTION;


// Driver Defined Action Frame Type
typedef enum _ACT_PKT_TYPE
{
	ACT_PKT_TYPE_UNKNOWN,	// Invalid or unrecognized frame
		
	// Qos (0x1)
	ACT_PKT_QOS_ADDTSREQ,
	ACT_PKT_QOS_ADDTSRSP,
	ACT_PKT_QOS_DELTS,
	ACT_PKT_QOS_SCHEDULE,
	// DLS (0x2)
	ACT_PKT_DLS_DLSREQ,
	ACT_PKT_DLS_DLSRSP,
	ACT_PKT_DLS_DLSTEARDOWN,
	// BA (0x3)
	ACT_PKT_BA_ADDBAREQ,
	ACT_PKT_BA_ADDBARSP,
	ACT_PKT_BA_DELBA,
	// Public (0x4)
	ACT_PKT_BSS_COEXIST,
	ACT_PKT_TDLS_DISC_RSP,
	ACT_PKT_P2P_GO_NEG_REQ,		// P2P Go Negotiation Request
	ACT_PKT_P2P_GO_NEG_RSP,		// P2P Go Negotiation Response
	ACT_PKT_P2P_GO_NEG_CONF,	// P2P Go Negotiation Confirm
	ACT_PKT_P2P_INVIT_REQ,		// P2P Invitation Request
	ACT_PKT_P2P_INVIT_RSP,		// P2P Invitation Response
	ACT_PKT_P2P_DEV_DISCOVERABILITY_REQ,	// P2P Device Discoverability Request
	ACT_PKT_P2P_DEV_DISCOVERABILITY_RSP,	// P2P Device Discoverability Response
	ACT_PKT_P2P_PROV_DISC_REQ,
	ACT_PKT_P2P_PROV_DISC_RSP,
	ACT_PKT_NAN_SDF,
	ACT_PKT_GAS_INT_REQ,
	ACT_PKT_GAS_INT_RSP,
	ACT_PKT_GAS_COMEBACK_REQ,
	ACT_PKT_GAS_COMEBACK_RSP,
	// Radio Resource Measurement (0x5)
	ACT_PKT_RM_RM_REQ,	// Radio Measurement Request
	ACT_PKT_RM_RM_RPT,	// Radio Measurement Report
	ACT_PKT_RM_LM_REQ,	// Link Measurement Request
	ACT_PKT_RM_LM_RPT,	// Link Measurement Report
	ACT_PKT_RM_NR_REQ,	// Neighbor Report Request
	ACT_PKT_RM_NR_RSP,	// Neighbor Report Response
	// High Throughput (0x7)
	ACT_PKT_HT_NOTI_CHNL_WIDTH,	// Notify Channel Width
	ACT_PKT_HT_SM_PS,	// SM Power Save
	ACT_PKT_HT_PSMP,	// PSMP
	ACT_PKT_HT_SET_PCO_PHASE,	// Set PCO Phase
	ACT_PKT_HT_CSI,	// CSI
	ACT_PKT_HT_NON_COMPRESSED_BEAMFORMING,	// Noncompressed Beamforming
	ACT_PKT_HT_COMPRESSED_BEAMFORMING,	// Compressed Beamforming
	ACT_PKT_HT_ASEL_FEEDBACK,	// ASEL Indices Feedback
	// SA Query (0x8)
	ACT_PKT_SA_QUERY_REQ, // SA query request
	ACT_PKT_SA_QUERY_RSP, // Sa query response
	// TDLS-11z (0x0C)
	ACT_PKT_TDLS_REQ,
	ACT_PKT_TDLS_RSP,
	ACT_PKT_TDLS_CONFIRM,
	ACT_PKT_TDLS_TEARDOWN,
	ACT_PKT_TDLS_PEER_TRAFIC_IND,
	ACT_PKT_TDLS_CHNL_SW_REQ,
	ACT_PKT_TDLS_CHNL_SW_RSP,
	ACT_PKT_TDLS_PEER_PSM_REQ,
	ACT_PKT_TDLS_PEER_PSM_RSP,
	ACT_PKT_TDLS_PEER_TRAFIC_RSP,
	ACT_PKT_TDLS_DISC_REQ,
	// WMM (0x11)
	ACT_PKT_WMM_ADDTSREQ,
	ACT_PKT_WMM_ADDTSRSP,
	ACT_PKT_WMM_DELTS,
	// VHT (0x15)
	ACT_PKT_VHT_COMPRESSED_BEAMFORMING,
	ACT_PKT_VHT_GROUPID_MANAGEMENT,
	ACT_PKT_VHT_OPMODE_NOTIFICATION,
	//Vendor Specific (0x7F)
	ACT_PKT_P2P_NOA,
	ACT_PKT_P2P_PRESENCE_REQ,
	ACT_PKT_P2P_PRESENCE_RSP,
	ACT_PKT_P2P_GO_DISCOVERABILITY_REQ,
}ACT_PKT_TYPE, *PACT_PKT_TYPE;

// Encapsulated data frame type
typedef enum _ENCAP_DATA_PKT_TYPE
{
	ENCAP_DATA_PKT_UNKNOWN = 0,

	// TDLS
	ENCAP_DATA_PKT_TDLS_SETUP_REQ,		// TDLS Setup Request
	ENCAP_DATA_PKT_TDLS_SETUP_RSP,		// TDLS Setup Response
	ENCAP_DATA_PKT_TDLS_SETUP_CONFIRM,	// TDLS Setup Confirm
	ENCAP_DATA_PKT_TDLS_TEARDOWN,		// TDLS Teardown
	ENCAP_DATA_PKT_TDLS_TRAFFIC_INDI,	// TDLS Peer Traffic Indication
	ENCAP_DATA_PKT_TDLS_CHNL_SW_REQ,	// TDLS Channel Switch Request
	ENCAP_DATA_PKT_TDLS_CHNL_SW_RSP,	// TDLS Channel Switch Response
	ENCAP_DATA_PKT_TDLS_PSM_REQ,		// TDLS Peer PSM Request
	ENCAP_DATA_PKT_TDLS_PSM_RSP,		// TDLS Peer PSM Response
	ENCAP_DATA_PKT_TDLS_TRAFFIC_RSP,	// TDLS Peer Traffic Response
	ENCAP_DATA_PKT_TDLS_DISC_REQ,		// TDLS Discovery Request
	ENCAP_DATA_PKT_TDLS_PROBE_REQ,		// TDLS Probe request
	ENCAP_DATA_PKT_TDLS_PROBE_RSP,		// TDLS Probe response

	// CCX IAPP
	ENCAP_DATA_PKT_CCX_IAPP,		// CCX IAPP
}ENCAP_DATA_PKT_TYPE, *PENCAP_DATA_PKT_TYPE;

//
// Pattern structure for pattern parser.
//
typedef struct	_PKT_PATTERN_MAP
{
	u4Byte			PktType;	// Packet Type
	u2Byte			PatternLen;	// The length in byte of the Pattern 
	pu1Byte			Pattern;	// Point to the pattern array
}PKT_PATTERN_MAP, *PPKT_PATTERN_MAP;

//
// Packet and IE offset map
//
typedef struct _PKT_IE_OFFSET_MAP
{
	u4Byte			PktType;	// Packet Type
	u2Byte			IeOffset;	// IE Offset
}PKT_IE_OFFSET_MAP, *PPKT_IE_OFFSET_MAP;


#define SET_80211_HDR_PROTOCOL_VERSION(_hdr, _val)			SET_BITS_TO_LE_2BYTE(_hdr, 0, 2, _val)
#define SET_80211_HDR_TYPE(_hdr, _val)					SET_BITS_TO_LE_2BYTE(_hdr, 2, 2, _val)
#define SET_80211_HDR_SUBTYPE(_hdr, _val)				SET_BITS_TO_LE_2BYTE(_hdr, 4, 4, _val)
#define SET_80211_HDR_TO_DS(_hdr, _val)					SET_BITS_TO_LE_2BYTE(_hdr, 8, 1, _val)
#define SET_80211_HDR_FROM_DS(_hdr, _val)				SET_BITS_TO_LE_2BYTE(_hdr, 9, 1, _val)
#define SET_80211_HDR_MORE_FRAG(_hdr, _val)				SET_BITS_TO_LE_2BYTE(_hdr, 10, 1, _val)
#define SET_80211_HDR_RETRY(_hdr, _val)					SET_BITS_TO_LE_2BYTE(_hdr, 11, 1, _val)
#define SET_80211_HDR_PWR_MGNT(_hdr, _val)				SET_BITS_TO_LE_2BYTE(_hdr, 12, 1, _val)
#define SET_80211_HDR_MORE_DATA(_hdr, _val)				SET_BITS_TO_LE_2BYTE(_hdr, 13, 1, _val)
#define SET_80211_HDR_WEP(_hdr, _val)					SET_BITS_TO_LE_2BYTE(_hdr, 14, 1, _val)
#define SET_80211_HDR_ORDER(_hdr, _val)					SET_BITS_TO_LE_2BYTE(_hdr, 15, 1, _val)
#define SET_80211_HDR_QOS_EN(_hdr, _val)				SET_BITS_TO_LE_2BYTE(_hdr, 7, 1, _val)

#define GET_80211_HDR_PROTOCOL_VERSION(_hdr)				LE_BITS_TO_2BYTE(_hdr, 0, 2)
#define GET_80211_HDR_TYPE(_hdr)					LE_BITS_TO_2BYTE(_hdr, 2, 2)
#define GET_80211_HDR_SUBTYPE(_hdr)					LE_BITS_TO_2BYTE(_hdr, 4, 4)
#define GET_80211_HDR_TO_DS(_hdr)					LE_BITS_TO_2BYTE(_hdr, 8, 1)
#define GET_80211_HDR_FROM_DS(_hdr)					LE_BITS_TO_2BYTE(_hdr, 9, 1)
#define GET_80211_HDR_MORE_FRAG(_hdr)					LE_BITS_TO_2BYTE(_hdr, 10, 1)
#define GET_80211_HDR_RETRY(_hdr)					LE_BITS_TO_2BYTE(_hdr, 11, 1)
#define GET_80211_HDR_PWR_MGNT(_hdr)					LE_BITS_TO_2BYTE(_hdr, 12, 1)
#define GET_80211_HDR_MORE_DATA(_hdr)					LE_BITS_TO_2BYTE(_hdr, 13, 1)
#define GET_80211_HDR_WEP(_hdr)						LE_BITS_TO_2BYTE(_hdr, 14, 1)
#define GET_80211_HDR_ORDER(_hdr)					LE_BITS_TO_2BYTE(_hdr, 15, 1)
#define GET_80211_HDR_QOS_EN(_hdr)					LE_BITS_TO_2BYTE(_hdr, 7, 1)

#define SET_80211_HDR_FRAME_CONTROL(_hdr, _val)				WriteEF2Byte(_hdr, _val)
#define SET_80211_HDR_TYPE_AND_SUBTYPE(_hdr, _val)			WriteEF1Byte(_hdr, _val)


// Byte[0] BIT7 in Frame Control is reserved for QoS. Added by Annie, 2005-12-02. 
#define	FC_QOS_BIT				BIT7

// Byte[0] BIT6 in Frame Control seems to be reserved for "No Data". Added by Annie, 2006-01-06.
#define	FC_NO_DATA_BIT		BIT6




#define FRAME_OFFSET_FRAME_CONTROL		0
#define FRAME_OFFSET_DURATION			2
#define FRAME_OFFSET_ADDRESS1			4
#define FRAME_OFFSET_ADDRESS2			10
#define FRAME_OFFSET_ADDRESS3			16
#define FRAME_OFFSET_SEQUENCE			22
#define FRAME_OFFSET_ADDRESS4			24

#define SET_80211_HDR_DURATION(_hdr, _val)	\
	WriteEF2Byte((pu1Byte)(_hdr)+FRAME_OFFSET_DURATION, _val)
#define SET_80211_HDR_ADDRESS1(_hdr, _val)	\
	cpMacAddr((pu1Byte)(_hdr)+FRAME_OFFSET_ADDRESS1, (pu1Byte)(_val))
#define SET_80211_HDR_ADDRESS2(_hdr, _val) \
	cpMacAddr((pu1Byte)(_hdr)+FRAME_OFFSET_ADDRESS2, (pu1Byte)(_val))
#define SET_80211_HDR_ADDRESS3(_hdr, _val) \
	cpMacAddr((pu1Byte)(_hdr)+FRAME_OFFSET_ADDRESS3, (pu1Byte)(_val))
#define SET_80211_HDR_FRAGMENT_SEQUENCE(_hdr, _val) \
	WriteEF2Byte((pu1Byte)(_hdr)+FRAME_OFFSET_SEQUENCE, _val)
#define SET_80211_HDR_ADDRESS4(_hdr, _val) \
	cpMacAddr((pu1Byte)(_hdr)+FRAME_OFFSET_ADDRESS4, (pu1Byte)(_val))
#define GET_80211_HDR_DURATION(_hdr) \
	ReadEF2Byte((pu1Byte)(_hdr)+FRAME_OFFSET_DURATION)
#define GET_80211_HDR_ADDRESS1(_hdr, _val) \
	cpMacAddr((pu1Byte)(_val), (pu1Byte)(_hdr)+FRAME_OFFSET_ADDRESS1)
#define GET_80211_HDR_ADDRESS2(_hdr, _val) \
	cpMacAddr((pu1Byte)(_val), (pu1Byte)(_hdr)+FRAME_OFFSET_ADDRESS2)
#define GET_80211_HDR_ADDRESS3(_hdr, _val) \
	cpMacAddr((pu1Byte)(_val), (pu1Byte)(_hdr)+FRAME_OFFSET_ADDRESS3)
#define GET_80211_HDR_FRAGMENT_SEQUENCE(_hdr) \
	ReadEF2Byte((pu1Byte)(_hdr)+FRAME_OFFSET_SEQUENCE)
#define GET_80211_HDR_ADDRESS4(_hdr, _val) \
	cpMacAddr((pu1Byte)(_val), (pu1Byte)(_hdr)+FRAME_OFFSET_ADDRESS4)

#define SET_80211_HDR_FRAGMENT(_hdr, _val)		SET_BITS_TO_LE_2BYTE(((pu1Byte)(_hdr))+FRAME_OFFSET_SEQUENCE, 0, 4, _val)
#define SET_80211_HDR_SEQUENCE(_hdr, _val)		SET_BITS_TO_LE_2BYTE(((pu1Byte)(_hdr))+FRAME_OFFSET_SEQUENCE, 4, 12, _val)
#define GET_80211_HDR_FRAGMENT(_hdr)			LE_BITS_TO_2BYTE(((pu1Byte)(_hdr))+FRAME_OFFSET_SEQUENCE, 0, 4)
#define GET_80211_HDR_SEQUENCE(_hdr)			LE_BITS_TO_2BYTE(((pu1Byte)(_hdr))+FRAME_OFFSET_SEQUENCE, 4, 12)

//
// PS Poll packet.
//
// Revised by Bruce for 802.11 layer packet.
#define SET_80211_PS_POLL_AID(_hdr, _val)		WriteEF2Byte(((pu1Byte)(_hdr))+2, _val)
#define GET_80211_PS_POLL_AID(_hdr)			ReadEF2Byte(((pu1Byte)(_hdr))+2)
#define SET_80211_PS_POLL_BSSID(_hdr, _val)		cpMacAddr(((pu1Byte)(_hdr))+4, (pu1Byte)(_val))
#define GET_80211_PS_POLL_BSSID(_hdr, _val)		cpMacAddr((pu1Byte)(_val), ((pu1Byte)(_hdr))+4)
#define SET_80211_PS_POLL_TA(_hdr, _val)		cpMacAddr(((pu1Byte)(_hdr))+10, (pu1Byte)(_val))
#define GET_80211_PS_POLL_TA(_hdr, _val)		cpMacAddr((pu1Byte)(_val), ((pu1Byte)(_hdr))+10)

//
// Beacon/Probe Response frame.
//
#define GET_BEACON_PROBE_RSP_TIME_STAMP_LOW(__pHeader) 			ReadEF4Byte(((pu1Byte)(__pHeader)) + 24)
#define SET_BEACON_PROBE_RSP_TIME_STAMP_LOW(__pHeader, __Value) 	WriteEF4Byte(((pu1Byte)(__pHeader)) + 24, __Value)

#define GET_BEACON_PROBE_RSP_TIME_STAMP_HIGH(__pHeader) 		ReadEF4Byte(((pu1Byte)(__pHeader)) + 28)
#define SET_BEACON_PROBE_RSP_TIME_STAMP_HIGH(__pHeader, __Value) 	WriteEF4Byte(((pu1Byte)(__pHeader)) + 28, __Value)

#define GET_BEACON_PROBE_RSP_BEACON_INTERVAL(__pHeader) 		ReadEF2Byte(((pu1Byte)(__pHeader)) + 32)
#define SET_BEACON_PROBE_RSP_BEACON_INTERVAL(__pHeader, __Value) 	WriteEF2Byte(((pu1Byte)(__pHeader)) + 32, __Value)

#define GET_BEACON_PROBE_RSP_CAPABILITY_INFO(__pHeader) 		ReadEF2Byte(((pu1Byte)(__pHeader)) + 34)
#define SET_BEACON_PROBE_RSP_CAPABILITY_INFO(__pHeader, __Value) 	WriteEF2Byte(((pu1Byte)(__pHeader)) + 34, __Value)
#define UNION_BEACON_PROBE_RSP_CAPABILITY_INFO(__pHeader, __Value) \
			SET_BEACON_PROBE_RSP_CAPABILITY_INFO(\
				__pHeader, \
				(GET_BEACON_PROBE_RSP_CAPABILITY_INFO(__pHeader) | (__Value)))

#define MASK_BEACON_PROBE_RSP_CAPABILITY_INFO(__pHeader, __Value) \
			SET_BEACON_PROBE_RSP_CAPABILITY_INFO(\
				__pHeader, \
				(GET_BEACON_PROBE_RSP_CAPABILITY_INFO(__pHeader) & (~(__Value))))

//
// Authentication frame.
// 
#define GET_AUTH_FRAME_AUTH_ALG_NUM(__pHeader) 				ReadEF2Byte(((pu1Byte)(__pHeader)) + 24)
#define SET_AUTH_FRAME_AUTH_ALG_NUM(__pHeader, __Value) 		WriteEF2Byte(((pu1Byte)(__pHeader)) + 24, __Value)

#define GET_AUTH_FRAME_AUTH_SEQ_NUM(__pHeader) 				ReadEF2Byte(((pu1Byte)(__pHeader)) + 26)
#define SET_AUTH_FRAME_AUTH_SEQ_NUM(__pHeader, __Value) 		WriteEF2Byte(((pu1Byte)(__pHeader)) + 26, __Value)

#define GET_AUTH_FRAME_STATUS_CODE(__pHeader) 				ReadEF2Byte(((pu1Byte)(__pHeader)) + 28)
#define SET_AUTH_FRAME_STATUS_CODE(__pHeader, __Value) 			WriteEF2Byte(((pu1Byte)(__pHeader)) + 28, __Value)

#define GET_AUTH_FRAME_CHALLENG_TEXT(__pHeader) 			(((pu1Byte)(__pHeader)) + 30)

//
// Associate Request frame.
// 
#define GET_ASOC_REQ_CAPABILITY_INFO(__pHeader) 			ReadEF2Byte(((pu1Byte)(__pHeader)) + 24)
#define SET_ASOC_REQ_CAPABILITY_INFO(__pHeader, __Value) 		WriteEF2Byte(((pu1Byte)(__pHeader)) + 24, __Value)

#define GET_ASOC_REQ_LISTEN_INTERVAL(__pHeader) 			ReadEF2Byte(((pu1Byte)(__pHeader)) + 26)
#define SET_ASOC_REQ_LISTEN_INTERVAL(__pHeader, __Value) 		WriteEF2Byte(((pu1Byte)(__pHeader)) + 26, __Value)

//
// Associate Reponse frame.
// 
#define GET_ASOC_RSP_CAPABILITY_INFO(__pHeader) 			ReadEF2Byte(((pu1Byte)(__pHeader)) + 24)
#define SET_ASOC_RSP_CAPABILITY_INFO(__pHeader, __Value) 		WriteEF2Byte(((pu1Byte)(__pHeader)) + 24, __Value)

#define GET_ASOC_RSP_STATUS_CODE(__pHeader) 				ReadEF2Byte(((pu1Byte)(__pHeader)) + 26)
#define SET_ASOC_RSP_STATUS_CODE(__pHeader, __Value) 			WriteEF2Byte(((pu1Byte)(__pHeader)) + 26, __Value)

#define GET_ASOC_RSP_AID(__pHeader) 					ReadEF2Byte(((pu1Byte)(__pHeader)) + 28)
#define SET_ASOC_RSP_AID(__pHeader, __Value) 				WriteEF2Byte(((pu1Byte)(__pHeader)) + 28, __Value)

//
// ReAssociation Request
//
#define GET_REASOC_REQ_CAPABILITY_INFO(__pHeader) 			ReadEF2Byte(((pu1Byte)(__pHeader)) + 24)
#define SET_REASOC_REQ_CAPABILITY_INFO(__pHeader, __Value) 		WriteEF2Byte(((pu1Byte)(__pHeader)) + 24, __Value)

#define GET_REASOC_REQ_LISTEN_INTERVAL(__pHeader) 			ReadEF2Byte(((pu1Byte)(__pHeader)) + 26)
#define SET_REASOC_REQ_LISTEN_INTERVAL(__pHeader, __Value) 		WriteEF2Byte(((pu1Byte)(__pHeader)) + 26, __Value)

#define GET_REASOC_REQ_CURR_AP_ADDR(__pHeader) 				(((pu1Byte)(__pHeader)) + 28)

//
// Management action frame
//
#define GET_ACTFRAME_CATEGORY_CODE(__pHeader) 				ReadEF1Byte( ((pu1Byte)(__pHeader)) + 24)
#define SET_ACTFRAME_CATEGORY_CODE(__pHeader, __Value) 			WriteEF1Byte( ((pu1Byte)(__pHeader)) + 24, __Value)

#define GET_ACTFRAME_ACTION_CODE(__pHeader) 				ReadEF1Byte( ((pu1Byte)(__pHeader)) + 25)
#define SET_ACTFRAME_ACTION_CODE(__pHeader, __Value) 			WriteEF1Byte( ((pu1Byte)(__pHeader)) + 25, __Value)

#define GET_ACTFRAME_DIALOG_TOKEN(__pHeader) 				ReadEF1Byte( ((pu1Byte)(__pHeader)) + 26)
#define SET_ACTFRAME_DIALOG_TOKEN(__pHeader, __Value) 			WriteEF1Byte( ((pu1Byte)(__pHeader)) + 26, __Value)

#define GET_ACTFRAME_STATUS_CODE(__pHeader) 				ReadEF1Byte( ((pu1Byte)(__pHeader)) + 27)
#define SET_ACTFRAME_STATUS_CODE(__pHeader, __Value) 			WriteEF1Byte( ((pu1Byte)(__pHeader)) + 27, __Value)

#define	MIN_ACTION_SA_QUERY_FRAME_LEN	28
#define	GET_ACTFRAME_SA_QUERY_ID(__pHeader)					ReadEF2Byte( ((pu1Byte)(__pHeader)) + 26)
#define SET_ACTFRAME_SA_QUERY_ID(__pHeader, __Value) 		WriteEF2Byte( ((pu1Byte)(__pHeader)) + 26, __Value)



#define	SET_SSID_DUMMY(_pSsid, __SsidLen)	\
{	\
	PlatformZeroMemory(_pSsid, MAX_SSID_LEN);	\
	__SsidLen = MAX_SSID_LEN;	\
}

//
// ARP packet
//
// LLC Header
#define GET_ARP_PKT_LLC_TYPE(__pHeader) 					ReadEF2Byte( ((pu1Byte)(__pHeader)) + 6)

//ARP element
#define GET_ARP_PKT_OPERATION(__pHeader) 				ReadEF2Byte( ((pu1Byte)(__pHeader)) + 6)
#define GET_ARP_PKT_SENDER_MAC_ADDR(__pHeader, _val) 	cpMacAddr((pu1Byte)(_val), ((pu1Byte)(__pHeader))+8)
#define GET_ARP_PKT_SENDER_IP_ADDR(__pHeader, _val) 		cpIpAddr((pu1Byte)(_val), ((pu1Byte)(__pHeader))+14)
#define GET_ARP_PKT_TARGET_MAC_ADDR(__pHeader, _val) 	cpMacAddr((pu1Byte)(_val), ((pu1Byte)(__pHeader))+18)

#define SET_ARP_PKT_HW(__pHeader, __Value)  				WriteEF2Byte( ((pu1Byte)(__pHeader)) + 0, __Value)
#define SET_ARP_PKT_PROTOCOL(__pHeader, __Value)  			WriteEF2Byte( ((pu1Byte)(__pHeader)) + 2, __Value)
#define SET_ARP_PKT_HW_ADDR_LEN(__pHeader, __Value)  		WriteEF1Byte( ((pu1Byte)(__pHeader)) + 4, __Value)
#define SET_ARP_PKT_PROTOCOL_ADDR_LEN(__pHeader, __Value)  	WriteEF1Byte( ((pu1Byte)(__pHeader)) + 5, __Value)
#define SET_ARP_PKT_OPERATION(__pHeader, __Value) 		WriteEF2Byte( ((pu1Byte)(__pHeader)) + 6, __Value)
#define SET_ARP_PKT_SENDER_MAC_ADDR(__pHeader, _val) 	cpMacAddr(((pu1Byte)(__pHeader))+8, (pu1Byte)(_val))
#define SET_ARP_PKT_SENDER_IP_ADDR(__pHeader, _val) 		cpIpAddr(((pu1Byte)(__pHeader))+14, (pu1Byte)(_val))
#define SET_ARP_PKT_TARGET_MAC_ADDR(__pHeader, _val) 	cpMacAddr(((pu1Byte)(__pHeader))+18, (pu1Byte)(_val))
#define SET_ARP_PKT_TARGET_IP_ADDR(__pHeader, _val) 		cpIpAddr(((pu1Byte)(__pHeader))+24, (pu1Byte)(_val))


#define MAX_NUM_RATES	264 // Max num of support rates element: 8,  Max num of ext. support rate: 255. 061122, by rcnjko.


BOOLEAN
EqualOS(
	IN	OCTET_STRING	os1,
	IN	OCTET_STRING	os2
	);

BOOLEAN
IsSSIDAny(
	IN	OCTET_STRING	ssid
	);

BOOLEAN
IsSSIDDummy(
	IN	OCTET_STRING	ssid
	);

BOOLEAN
IsSSIDNdisTest(
	IN	OCTET_STRING	ssid
	);

ACT_PKT_TYPE
PacketGetActionFrameType(
	IN	POCTET_STRING	posMpdu
	);

ENCAP_DATA_PKT_TYPE
PacketGetEncapDataFrameType(
	IN	POCTET_STRING	posDataContent
	);

BOOLEAN
PacketCheckIEValidity(
	IN	OCTET_STRING	packet,
	IN	PRT_RFD			pRfd
	);

BOOLEAN
PacketGetIeOffset(
	IN	POCTET_STRING	posMpdu,
	OUT	pu2Byte			pOffset
	);

u1Byte
PacketGetElementNum(
	IN	OCTET_STRING	packet,
	IN	ELEMENT_ID		ID,
	IN	OUI_TYPE		OUIType,
	IN	u1Byte			OUISubType
	);

OCTET_STRING
PacketGetElement(
	IN	OCTET_STRING	packet,
	IN	ELEMENT_ID		ID,
	IN	OUI_TYPE		OUIType,
	IN	u1Byte			OUISubType
	);

VOID
PacketMakeElement(
	IN	POCTET_STRING	packet,
	IN	ELEMENT_ID		ID,
	IN	OCTET_STRING	info
	);

VOID
PacketAppendData(
	IN	POCTET_STRING	packet,
	IN	OCTET_STRING	data
	);

BOOLEAN
TimGetBcMcBit(
	IN	OCTET_STRING	Tim
	);

BOOLEAN	
TimGetAIDBit( 
	IN	OCTET_STRING	Tim, 
	IN	u2Byte			AID
	);

u2Byte 
ComputeTxTime(
	IN	u2Byte			FrameLength,
	IN	u4Byte			DataRate,
	IN	BOOLEAN			bManagementFrame,
	IN	BOOLEAN			bShortPreamble
	);


BOOLEAN
IsHiddenSsid(
	OCTET_STRING		ssid
	);

BOOLEAN
BeHiddenSsid(
	pu1Byte	ssidbuf,
	u1Byte	ssidlen
	);

BOOLEAN
NullSSID(
	OCTET_STRING	bcnPkt
	);

BOOLEAN
CompareSSID(
	pu1Byte	ssidbuf1,
	u2Byte	ssidlen1,
	pu1Byte	ssidbuf2,
	u2Byte	ssidlen2
);

#define CopySsid(__pDstBuf, __DstLen, __pSrcBuf, __SrcLen) \
	{(__DstLen) = ( ((__SrcLen) > MAX_SSID_LEN) ? MAX_SSID_LEN : (__SrcLen) ); \
	CopyMem((__pDstBuf), (__pSrcBuf), (__DstLen));}	

#define CopySsidOS(__osDst, __osSrc) \
	CopySsid((__osDst).Octet, (__osDst).Length, (__osSrc).Octet, (__osSrc).Length)	

u1Byte
ComputeAckRate(
	IN	OCTET_STRING	BSSBasicRateSet,
	IN	u1Byte			DataRate
	);

BOOLEAN
HasNextIE(
	IN	POCTET_STRING	posMpdu,
	IN	u4Byte			Offset
	);

RT_DOT11_IE
AdvanceToNextIE(
	IN		POCTET_STRING	posMpdu,
	IN OUT	pu4Byte			pOffset
	);

u1Byte
IEGetElementNum(
	IN	OCTET_STRING	IEs,
	IN	ELEMENT_ID		ID,
	IN	OUI_TYPE		OUIType,
	IN	u1Byte			OUISubType
	);

OCTET_STRING 
IEGetElement(
	IN	OCTET_STRING	IEs,
	IN	ELEMENT_ID		ID,
	IN	OUI_TYPE		OUIType,
	IN	u1Byte			OUISubType
	);

BOOLEAN
IsIELengthValid(
	IN	u1Byte		IDIE,
	IN	u1Byte		IELength
	);

BOOLEAN
IsSSIDNdisTest(
	IN	OCTET_STRING	ssid
	);

#endif // #ifndef __INC_PROTOCOL802_11_H

