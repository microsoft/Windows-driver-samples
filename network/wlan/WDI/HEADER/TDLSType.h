#ifndef __INC_TDLSTYPE_H
#define __INC_TDLSTYPE_H

#if (TDLS_SUPPORT==1)

#define MAX_TDLS_PAIR		16
#define MAX_BLOCKED_PEER	8

#define TDLSETUP_DELAY_LONG			60000
#define TDLSETUP_DELAY_SHORT			5000
#define TDLS_SETUP_TIMEOUT			5000
#define TDLS_PEER_REACHABLE_PERIOD	2000
#define TDLS_LINK_ALIVE_PERIOD			2000
#define TDLS_BLOCK_PEER_PERIOD		60000
#define TDLS_TEARDOWN_CHECK			100
#define TDLS_CHNLSW_TIMEOUT			5000
#define TDLS_PTI_TIMEOUT_CNT			10

#define TDLS_TYPE_NONE				0
#define TDLS_TYPE_SETUPREQ		1
#define TDLS_TYPE_SETUPRSP		2
#define TDLS_TYPE_SETUPCFRM		3
#define TDLS_TYPE_TEARDOWN		4
#define TDLS_TYPE_DISCOVERYREQ	5
#define TDLS_TYPE_DISCOVERYRSP	6
#define TDLS_TYPE_TRAFFICIND		7
#define TDLS_TYPE_TRAFFICRSP		8
#define TDLS_TYPE_CHNLSWITCHREQ	9
#define TDLS_TYPE_CHNLSWITCHRSP	10
#define TDLS_TYPE_UNKNOWN		15

#define TDLS_THRESHOLD_AP_HIGH	80
#define TDLS_THRESHOLD_PEER_LOW	10

#define MAX_TDLS_SLOT_NUM		5

#define TDLS_TIMEOUT_INTERVAL_TYPE	2
#define TDLS_TIMEOUT_KEY_LIFETIME		600		// seconds
#define TDLS_TIMEOUT_MIN_LIFETIME		300
#define TDLS_TIMEOUT_MAX_LIFETIME		0xffffffff

#define TDLS_MAX_GEN_KEY_LENGTH		64

#define SWITCH_CHANNEL_DELAY			2*sTU

typedef enum _TDLS_TIMER_CONTROL{
	TDLS_TIMER_CONTROL_INIT,
	TDLS_TIMER_CONTROL_SET,
	TDLS_TIMER_CONTROL_CANCEL,
	TDLS_TIMER_CONTROL_RELEASE
} TDLS_TIMER_CONTROL;

typedef enum _TDLS_LINK_STATUS{
	TDLS_LINK_STATUS_DISCONNECT		= 0x00,
	TDLS_LINK_STATUS_KEEP_CONNECT	= 0x01,
	TDLS_LINK_STATUS_PEER_SS_WEEK	= 0x02,
	TDLS_LINK_STATUS_PEER_DISAPPEAR	= 0x04,
	TDLS_LINK_STATUS_AP_SS_STRONG	= 0x08
} TDLS_LINK_STATUS;

typedef enum _TDLS_SETUP_PROGRESS_STAGE{
	TDLS_SETUP_PROGRESS_IDLE			= 0x00,
	TDLS_SETUP_PROGRESS_SEND_REQ	= 0x01,
	TDLS_SETUP_PROGRESS_RECV_REQ	= 0x02,
	TDLS_SETUP_PROGRESS_RECV_RSP	= 0x04,
	TDLS_SETUP_PROGRESS_DONE		= 0x08
} TDLS_SETUP_PROGRESS_STAGE;

typedef enum _TDLS_MAX_PEER_CAP{
	TDLS_MAX_PEER_CAP_B			= 0x00,
	TDLS_MAX_PEER_CAP_G			= 0x01
} TDLS_MAX_PEER_CAP;

typedef enum _TDLS_EXT_CAP_SUPPORT{
	TDLS_EXT_CAP_SUPPORT_NONE			= 0x00,
	TDLS_EXT_CAP_SUPPORT_BU_STA			= 0x01,
	TDLS_EXT_CAP_SUPPORT_SLEEP_STA		= 0x02,
	TDLS_EXT_CAP_SUPPORT_CHNLSWITCH	= 0x04
}TDLS_EXT_CAP_SUPPORT;

typedef enum _RT_TDLS_PS_STATE{
	RT_TDLS_PS_NONE				= 0,
	RT_TDLS_PS_SLEEP				= 1,
	RT_TDLS_PS_TRIGGER			= 2,
	RT_TDLS_PS_ACTIVE_PTI			= 3,
	RT_TDLS_PS_ACTIVE_AWAKE		= 4,
	RT_TDLS_PS_ENTER_SLEEP		= 5
}RT_TDLS_PS_STATE;

typedef enum _TDLS_TX_FEEDBACK_INDIC{
	TDLS_TX_FEEDBACK_NONE			= 0,
	TDLS_TX_FEEDBACK_TEARDOWN		= 1,
	TDLS_TX_FEEDBACK_CHNLSWITCH		= 2,
	TDLS_TX_FEEDBACK_NULL			= 3,
}TDLS_TX_FEEDBACK_INDIC;

typedef enum _TDLS_MSG_TIMER_CTRL{
	TDLS_MSG_TIMER_NONE			= 0,
	TDLS_MSG_TIMER_SETUPMSG		= 1,
	TDLS_MSG_TIMER_TEARDOWN	= 2,
	TDLS_MSG_TIMER_PTI			= 3,
	TDLS_MSG_TIMER_CS_REQ		= 4,
	TDLS_MSG_TIMER_CS_RSP		= 5,
	TDLS_MSG_TIMER_CS_NULL		= 6
}TDLS_MSG_TIMER_CTRL;

enum _TDLS_DELAY_LEVEL{
	TDLS_DELAY_LEVEL_NONE	= 0,
	TDLS_DELAY_LEVEL_SHORT	= 1,
	TDLS_DELAY_LEVEL_LONG	= 2
};

enum _TDLS_BLOCK_DIR{
	TDLS_BLOCK_NONE			= 0x00,
	TDLS_BLOCK_PASSIVE		= 0x01,
	TDLS_BLOCK_ACTIVE			= 0x02
};

enum _TDLS_ROLL{
	TDLS_ROLL_NONE			= 0,
	TDLS_ROLL_INITIATOR		= 1,
	TDLS_ROLL_RESPONDER		= 2,
};

enum _TDLS_STA_TEST_CHARACTER{
	TDLS_INITIATOR				= 0x01,
	TDLS_RESPONDER			= 0x02,
	TDLS_WRONGBSSID			= 0x04,
	TDLS_RESENDREQ			= 0x08,
	TDLS_REJECT_CFRM			= 0x10,
	TDLS_PROHIBIT				= 0x20,
	TDLS_KEY_LIFETIME			= 0x40,
	TDLS_IGNORE_TPK_TIMER	= 0x80,
	TDLS_RADIO_OFF			= 0x100,
	TDLS_CS_CHANNEL			= 0x200,
	TDLS_CS_BANDWIDTH		= 0x400,
	TDLS_CS_REJECTREQ			= 0x800,
	TDLS_CS_UNSOLIRSP			= 0x1000,
	TDLS_ACCEPT_WEAK_SEC	= 0x2000,
};

enum _TDLS_AGGR_RATIO{
	TDLS_AGGR_RATIO_NONE	= 0,
	TDLS_AGGR_RATIO_20MHZ	= 1,
	TDLS_AGGR_RATIO_40MHZ	= 2
};
	
enum _TDLSSecurityState{
	TDLSStateSuccess = 0,
	TDLSStateDiscard = 1,
	TDLSStateFail = 2	
};

enum _TDLS_CS_STAGE{
	TDLS_CS_OnBaseChannel		= 0,
	TDLS_CS_ToOffChannel		= 1,
	TDLS_CS_OnOffChannel		= 2,
	TDLS_CS_BackBaseChannel	= 3
};

#define	RT_TDLS_OP_CONFIG_DISALBE_TDLS			(u1Byte)0x00
#define	RT_TDLS_OP_CONFIG_ENABLE_TDLS			(u1Byte)0x01
#define	RT_TDLS_OP_CONFIG_DM_DISABLE			(u1Byte)0x02
#define	RT_TDLS_OP_CONFIG_DM_ENABLE				(u1Byte)0x03
#define	RT_TDLS_OP_CONFIG_SEND_DISC				(u1Byte)0x04
#define	RT_TDLS_OP_CONFIG_WIFI_TEST_MODE		(u1Byte)0x05
#define	RT_TDLS_OP_CONFIG_NORMAL_MODE			(u1Byte)0x06
#define	RT_TDLS_OP_CONFIG_START_TDLS_PEER		(u1Byte)0x07
#define	RT_TDLS_OP_CONFIG_TEAR_DOWN_PEER		(u1Byte)0x08
#define	RT_TDLS_OP_CONFIG_PEER_ENTER_UAPSD_PS	(u1Byte)0x09
#define	RT_TDLS_OP_CONFIG_ENABLE_CHNL_SWITCH	(u1Byte)0x0A
#define	RT_TDLS_OP_CONFIG_DISABLE_CHNL_SWITCH	(u1Byte)0x0B
#define	RT_TDLS_OP_CONFIG_PROBE_REQ				(u1Byte)0x0C
#define	RT_TDLS_OP_CONFIG_ACCEPT_WEAK_SEC		(u1Byte)0x0D


#define	TDLS_ENCAP_ACTION_HEADER_OFFSET			(3 + 5 + 1)	// LLC  + SNAP + Payload Type
#define	TDLS_ENCAP_MGNT_FRAME_HEADER_OFFSET		(3 + 5 + 1 + 1 + 3 + 1)	// LLC  + SNAP + Payload Type + Category + OUI + FrameBody Type


//3 Category
#define GET_TDLS_FRAME_CATEGORY_CODE(_pStart)			ReadEF1Byte( (pu1Byte)(_pStart) )
#define SET_TDLS_FRAME_CATEGORY_CODE(_pStart, _val) 		WriteEF1Byte( ((pu1Byte)(_pStart)), _val)

//3 Action Code
#define GET_TDLS_FRAME_ACTION_CODE(_pStart)				ReadEF1Byte( (pu1Byte)(_pStart)+1 )
#define SET_TDLS_FRAME_ACTION_CODE(_pStart, _val)			WriteEF1Byte( ((pu1Byte)(_pStart))+1, _val)

//3 Dialog Token
#define FRAME_OFFSET_TDLS_DIALOG_TOKEN(_pStart)			( ( (GET_TDLS_FRAME_ACTION_CODE(_pStart) == 0) || ( GET_TDLS_FRAME_ACTION_CODE(_pStart) == 0x04) || (GET_TDLS_FRAME_ACTION_CODE(_pStart) >= 0x09))?2:4 )
#define GET_TDLS_FRAME_DIALOG_TOKEN(_pStart)				( ((pu1Byte)(_pStart))+FRAME_OFFSET_TDLS_DIALOG_TOKEN(_pStart))

//3 Status Code
#define GET_TDLS_FRAME_STATUS_CODE(_pStart)				ReadEF2Byte( ((pu1Byte)(_pStart)) + 2)

//3 Capability
#define FRAME_OFFSET_TDLS_CAPABILITY(_pStart)				( ( GET_TDLS_FRAME_ACTION_CODE(_pStart) == 0)?23:25 )
#define GET_TDLS_SETUPREQ_FRAME_CAP(_pStart)				( ((pu1Byte)(_pStart))+FRAME_OFFSET_TDLS_CAPABILITY(_pStart))

//3 Extended Capability
#define GET_EXT_CAP_ELE_PEER_UAPSD_BUSTA(_pEleStart)		LE_BITS_TO_4BYTE((_pEleStart), 28, 1)
#define SET_EXT_CAP_ELE_PEER_UAPSD_BUSTA(_pEleStart, _val)				SET_BITS_TO_LE_4BYTE((_pEleStart), 28, 1, _val)
#define SET_EXT_CAP_ELE_PEER_PSM(_pEleStart, _val)			SET_BITS_TO_LE_4BYTE((_pEleStart), 29, 1, _val)
#define GET_EXT_CAP_ELE_CHNL_SWITCH(_pEleStart)			LE_BITS_TO_4BYTE((_pEleStart), 30, 1)
#define SET_EXT_CAP_ELE_CHNL_SWITCH(_pEleStart, _val)		SET_BITS_TO_LE_4BYTE((_pEleStart), 30, 1, _val)
#define GET_EXT_CAP_ELE_TDLS_SUPPORT(_pEleStart)			LE_BITS_TO_4BYTE((_pEleStart+4), 5, 1)
#define SET_EXT_CAP_ELE_TDLS_SUPPORT(_pEleStart, _val)		SET_BITS_TO_LE_4BYTE((_pEleStart+4), 5, 1,  _val)
#define GET_EXT_CAP_ELE_TDLS_PROHIBITED(_pEleStart)		LE_BITS_TO_4BYTE((_pEleStart+4), 6, 1)
#define SET_EXT_CAP_ELE_TDLS_PROHIBITED(_pEleStart, _val)	SET_BITS_TO_LE_4BYTE((_pEleStart+4), 6, 1, _val)
#define GET_EXT_CAP_ELE_CHNL_SWITCH_PROHIBITED(_pEleStart)	LE_BITS_TO_4BYTE((_pEleStart+4), 7, 1)
#define SET_EXT_CAP_ELE_CHNL_SWITCH_PROHIBITED(_pEleStart, _val)		SET_BITS_TO_LE_4BYTE((_pEleStart+4), 7, 1, _val)

//3 PU Buffer Status
#define GET_PU_BUFFER_STATUS_ELE_AC_BK_TRAFFIC(_pEleStart)		LE_BITS_TO_1BYTE((_pEleStart), 0, 1)
#define SET_PU_BUFFER_STATUS_ELE_AC_BK_TRAFFIC(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE(_pEleStart), 0, 1, _val)
#define GET_PU_BUFFER_STATUS_ELE_AC_BE_TRAFFIC(_pEleStart)		LE_BITS_TO_1BYTE((_pEleStart), 1, 1)
#define SET_PU_BUFFER_STATUS_ELE_AC_BE_TRAFFIC(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE((_pEleStart), 1, 1, _val)
#define GET_PU_BUFFER_STATUS_ELE_AC_VI_TRAFFIC(_pEleStart)		LE_BITS_TO_1BYTE((_pEleStart), 2, 1)
#define SET_PU_BUFFER_STATUS_ELE_AC_VI_TRAFFIC(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE((_pEleStart), 2, 1, _val)
#define GET_PU_BUFFER_STATUS_ELE_AC_VO_TRAFFIC(_pEleStart)		LE_BITS_TO_1BYTE((_pEleStart), 3, 1)
#define SET_PU_BUFFER_STATUS_ELE_AC_VO_TRAFFIC(_pEleStart, _val)	SET_BITS_TO_LE_1BYTE((_pEleStart), 3, 1, _val)

//3 Channel Switch
#define GET_TDLS_TARGET_CHANNEL(_pStart)					ReadEF1Byte( ((pu1Byte)(_pStart)) + 2)
#define GET_TDLS_REGULATORY_CLASS(_pStart)				ReadEF1Byte( ((pu1Byte)(_pStart)) + 3)
#define GET_TDLS_SWITCHTIMING_IE_TIME(_pStart)			ReadEF2Byte( (pu1Byte)(_pStart) )
#define SET_TDLS_SWITCHTIMING_IE_TIME(_pStart, _val)		WriteEF2Byte((pu1Byte)(_pStart), _val)
#define GET_TDLS_SWITCHTIMING_IE_TIMEOUT(_pStart)		ReadEF2Byte( ((pu1Byte)(_pStart)) + 2)
#define SET_TDLS_SWITCHTIMING_IE_TIMEOUT(_pStart, _val)	WriteEF2Byte( ((pu1Byte)(_pStart)) + 2, _val)

//
// Link Identifier Element format
//
// |ID 		| Length	| BSSID	| TDLS initiator	| TDLS responder	|
// |			|		|		| STA Address		| STA Address		|
// |1 Byte	| 1 Byte	| 6 Bytes	| 6 Bytes	 		| 6 Bytes			|
//
#define LINK_ID_SIZE		(6+6+6)
typedef u1Byte			LINK_IDENTIFIER[LINK_ID_SIZE], *PLINK_IDENTIFIER;


#define SET_TDLS_LINKID_BSSID(_pStart, _val) \
	cpMacAddr((pu1Byte)(_pStart), (pu1Byte)(_val))
#define GET_TDLS_LINKID_BSSID(_pStart, _val) \
	cpMacAddr((pu1Byte)(_val), (pu1Byte)(_pStart))
#define SET_TDLS_LINKID_INITIATOR(_pStart, _val) \
	cpMacAddr((pu1Byte)((_pStart)+6), (pu1Byte)(_val))
#define GET_TDLS_LINKID_INITIATOR(_pStart, _val) \
	cpMacAddr((pu1Byte)(_val), (pu1Byte)(_pStart+6))
#define SET_TDLS_LINKID_RESPONDER(_pStart, _val) \
	cpMacAddr((pu1Byte)((_pStart)+6+6), (pu1Byte)(_val))
#define GET_TDLS_LINKID_RESPONDER(_pStart, _val) \
	cpMacAddr((pu1Byte)(_val), (pu1Byte)(_pStart+6+6))

#define SET_TDLS_SETUP_REQ_CAP(_pStart, _val) 	WriteEF2Byte((pu1Byte)(_pStart), _val)


typedef struct _RT_REGULATORY_CLASS
{
	u1Byte	Channel[MAX_CHANNEL_NUM];
	u1Byte	Len;
}RT_REGULATORY_CLASS, *PRT_REGULATORY_CLASS;

typedef struct _CHNL_TUPLE{
	u1Byte	FirstChnl;
	u1Byte	NumChnls;
} CHNL_TUPLE, *PCHNL_TUPLE;

typedef struct _CHNLINFO_TUPLE{
	u1Byte				ChnlNum;
	BOOLEAN				bw40M;
	EXTCHNL_OFFSET	ChnlOffset;
} CHNLINFO_TUPLE, *PCHNLINFO_TUPLE;

typedef struct _CHNLSWITCH_TIMING{
	u2Byte				SwitchTime;
	u2Byte				SwitchTimeout;
} CHNLSWITCH_TIMING, *PCHNLSWITCH_TIMING;
typedef struct _LINK_CAP{
	u2Byte				WirelessMode;
	u2Byte				RateCap;
	u2Byte				AggregationCap;
} LINK_CAP, *PLINK_CAP;

typedef struct _TDLS_LINK_INFO{
	// Count Traffic
	u4Byte				NumRxDataCount;
	u4Byte				RxDataCount[MAX_TDLS_SLOT_NUM];
	u2Byte				SlotNum;
	u2Byte				SlotIndex;

	// Peer Capability on Direct Link
	BOOLEAN				bBW40MHz;
	BOOLEAN				bShortGIEnabled;
	LINK_CAP			DirectLinkCapInfo;
	RT_ENC_ALG			DirectLinkEncAlgorithm;

	// Peer Capability to AP
	BOOLEAN				BssbBW40MHz;
	BOOLEAN				BssbShortGIEnabled;
	LINK_CAP			BssCapInfo;
	RT_ENC_ALG			BssEncAlgorithm;
	
	// Link maintenance
	u2Byte				KeepConnectCount;

	// Link Status
	u2Byte				LinkStatus;
	BOOLEAN				bUseDirectLink;
	u4Byte				TimeoutValue;
} TDLS_LINK_INFO, *PTDLS_LINK_INFO;

typedef struct _TDLS_CHNLSWITCH_INFO
{
	CHNLINFO_TUPLE		CurrentChnl;
	CHNLINFO_TUPLE		TargetChnl;
	CHNLSWITCH_TIMING	SwitchTiming;
	CHNLSWITCH_TIMING	MaxSwitchTiming;
}TDLS_CHNLSWITCH_INFO, *PTDLS_CHNLSWITCH_INFO;

typedef struct _TDLS_BLOCKED_RECORD{
	u1Byte				Addr[6];
	u2Byte				BlockedLevel;
	u8Byte				StartBlockTime;
	u8Byte				BlockDuration;
} TDLS_BLOCKED_RECORD, *PTDLS_BLOCKED_RECORD;

#define GET_TDLS_RSNIE_VERSION(_pStart) \
	ReadEF2Byte((pu1Byte)(_pStart))
#define GET_TDLS_RSNIE_PAIRWISESUITECOUNT(_pStart)\
	ReadEF2Byte((pu1Byte)(_pStart)+6)
	

typedef struct _FAST_BSS_TRANSITION{
	u1Byte				MICControl[2];
	u1Byte				MIC[16];
	u1Byte				ANonce[32];
	u1Byte				SNonce[32];
}FAST_BSS_TRANSITION, *PFAST_BSS_TRANSITION;

#define SET_FT_MICControl(_pStart, _val)	WriteEF2Byte((pu1Byte)(_pStart), _val)


//
// Timeout Interval element format
//
// |ID 		| Length	| Timeout  		| Timeout		|
// |              |           	| Interval Type	| Interval Value	|
// |1Byte	| 1Byte	| 1 Byte			| 4 Byte			|
//
#define TIMEOUT_IE_SIZE		5
typedef u1Byte			TIMEOUT_IE[TIMEOUT_IE_SIZE], *PTIMEOUT_IE;

#define GET_TDLS_TIMEOUTIE_TIMEOUT_TYPE(_pStart) \
	ReadEF1Byte((pu1Byte)(_pStart))
#define SET_TDLS_TIMEOUTIE_TIMEOUT_TYPE(_pStart, _val) \
	WriteEF1Byte((pu1Byte)(_pStart), _val)
#define GET_TDLS_TIMEOUTIE_TIMEOUT_VALUE(_pStart) \
	ReadEF4Byte(((pu1Byte)(_pStart)) + 1)
#define SET_TDLS_TIMEOUTIE_TIMEOUT_VALUE(_pStart, _val) \
	WriteEF4Byte(((pu1Byte)(_pStart)) +1, _val)


typedef struct _TDLS_RECORD{
	RT_LIST_ENTRY			List;
	PADAPTER				Adapt;
	RT_TIMER				InitTDLSTimer;
	RT_TIMER				MsgTimer;
	RT_TIMER				BaseChannelTimer;
	RT_TIMER				CSTrafficTimer;
	TDLS_MSG_TIMER_CTRL		MsgTimerCtrl;
	u1Byte					Addr[6];
	BOOLEAN					bValid;
	BOOLEAN					bActive;
	BOOLEAN					bTDLSinProgress;
	BOOLEAN					bTeardownSent;
	u1Byte					ProgressStage;
	u1Byte					InitTDLSDelayedLevel;
	u1Byte					Roll;
	u1Byte					DialogToken;
	u1Byte					LinkDialogToken;
	LINK_IDENTIFIER			LinkID;
	u2Byte					Cap;
	RT_HTINFO_STA_ENTRY		HTInfo;
	u1Byte					ExtCapSupport;
	OCTET_STRING			RegulatoryIE;
	u1Byte					ListofRC[5];
	BOOLEAN					bRT2RTTurbo;
	u2Byte					SupWirelessMode;
	OCTET_STRING			RSNIE;
	u1Byte					RSNIEBuf[22];
	OCTET_STRING			FTIE;
	FAST_BSS_TRANSITION		FTIEContent;
	u1Byte					TPK_KCK[16];
	u1Byte					PTK[64];
	OCTET_STRING			TimeoutIE;
	TIMEOUT_IE				TimeoutInterval;
	OCTET_STRING			RT2RTIE;
	u1Byte					RT2RTLinkInfo[20];
	PRT_WLAN_STA			pWLanSTA;
	OCTET_STRING			WmmAcParaIE;
	u1Byte					WmmAcParaBuf[WMM_PARAM_ELEMENT_SIZE];	// Max WMM parameter size is 24 octets.
	OCTET_STRING			WmmAcInfoIE;
	u1Byte					WmmAcInfoBuf[WMM_INFO_ELEMENT_SIZE];
	u1Byte					PUBufferStatus;
	RT_TDLS_PS_STATE		BufferSTAPSState;
	u1Byte					SleepSTAPSState;
	u1Byte					PTICount;
	BOOLEAN					bEnableCS;
	u1Byte					RejectCnt;	// For marvell
	BOOLEAN					bSupportChnlSwitch;	// Support Channel Switch	
	TDLS_CHNLSWITCH_INFO	ChnlSwitchInfo;
	u2Byte					CSstate;
	u1Byte					TargetState;
	BOOLEAN					bCSRequester;
	RT_WORK_ITEM			OffChannelWorkitem;
	RT_WORK_ITEM			BaseChannelWorkitem;
	BOOLEAN					bSendPTIRsp;
	TDLS_TX_FEEDBACK_INDIC	TxReportFlag;
} TDLS_RECORD, *PTDLS_RECORD;


typedef struct _TDLS_TINFO{
	DECLARE_RT_OBJECT(_TDLS_TINFO);
	
	BOOLEAN				bTDLSEnable;
	BOOLEAN				bDMEnable;
	BOOLEAN				bEnableHT;
	u2Byte				NumOfTDL;
	TDLS_BLOCKED_RECORD	BlockedPeerList[MAX_BLOCKED_PEER];
	u2Byte				NumBlockedPeer;
	u4Byte				NumRxCount;
	BOOLEAN				bEDCAInTDLSMode;
	u1Byte				CurRSSIState;
	u2Byte				DiscoveryReqDialogToken;
	BOOLEAN				bTDLSChnlSwEnable;

	// Peer UAPSD related
	u2Byte				PSPeerNum;

	// Channel Switch related
	BOOLEAN				bCSProhibited;
	u1Byte				DTimCnt;
	BOOLEAN				bUseCS;
	BOOLEAN				bOffChannel;
	RT_LIST_ENTRY		CSQueue;

	// Dynamic Mechanism related (TDLS establish / TDLS teardown /EDCA)
	u8Byte				NumTDLSTxBytes;
	u8Byte				NumTDLSRxBytes;
	u2Byte				ToDSCap;
	u2Byte				FromDSCap;
	u1Byte				TxInitRate;	// tx to AP
	u2Byte				AggrRatio;	// tx to AP
	u2Byte				RxStatisticalRate;		// rx from AP

	// WiFi Test and TestBed related
	BOOLEAN				bTDLSTest;
	u2Byte				TestSTA;
	u1Byte				WrongBSSID[6];
	u4Byte				TestTimeout;
	BOOLEAN				bKeepTPKTimer;
	u1Byte				SwitchChannel;
	BOOLEAN				SwitchBandwidth40MHz;
	
	RT_LIST_ENTRY		TDLS_Admit_List;
	RT_LIST_ENTRY		TDLS_Unused_List;
	TDLS_RECORD			TDLSRecord[MAX_TDLS_PAIR];
	u1Byte				NumInAdmitList;
	
	BOOLEAN				bAcceptWeakSec;
}TDLS_TINFO, *PTDLS_TINFO;


#define	GET_TDLS_INFO(__pMgntInfo)				( (PTDLS_TINFO)((__pMgntInfo)->pTDLSTInfo) )
#define	GET_TDLS_ENABLED(__pMgntInfo)			( (((GET_TDLS_INFO(__pMgntInfo))->bTDLSEnable)==TRUE) ? TRUE : FALSE )
#define	SET_TDLS_ENABLED(__pMgntInfo, _value)	( ((GET_TDLS_INFO(__pMgntInfo))->bTDLSEnable) = (_value) )
#define	GET_TDLS_WIFITESTBED(__pMgntInfo)		( (GET_TDLS_INFO(__pMgntInfo))->TestSTA)
#define	GET_TDLS_WIFITEST(__pMgntInfo)		( (GET_TDLS_INFO(__pMgntInfo))->bTDLSTest)
#define	GET_TDLS_HTCAP(__pMgntInfo)			( (GET_TDLS_INFO(__pMgntInfo))->bEnableHT)
#define	IS_TDL_EXIST(_pMgntInfo) 				( (GET_TDLS_INFO(_pMgntInfo)->bTDLSEnable) && _pMgntInfo->mAssoc && (GET_TDLS_INFO(_pMgntInfo)->NumOfTDL > 0) )
#define	IS_TDLS_CONSTRUCTED(_pTDLS)			((_pTDLS)->ProgressStage == TDLS_SETUP_PROGRESS_DONE)
#define	IS_TDLS_PEER_UNDER_PS(_pTDLS)		( ((_pTDLS)->pWLanSTA->bPowerSave==TRUE)?TRUE:FALSE )
#define	GET_TDLS_WIFITESTBED_RADIO_OFF(__pMgntInfo)		( ((GET_TDLS_INFO(__pMgntInfo))->TestSTA) & TDLS_RADIO_OFF )

#endif


#endif
