//-----------------------------------------------------------------------------
//	File:
//		CcxType.h
//
//	Description:
//		CCX related information.	
//
//-----------------------------------------------------------------------------

#ifndef __INC_CCX_TYPE_H
#define __INC_CCX_TYPE_H

#ifndef	CCX_SUPPORT_VER
#define	CCX_SUPPORT_VER	0
#endif

//------------------------------------------------------------------------------
// CCX IAPP frame. 
//------------------------------------------------------------------------------
#define CISCO_AIRONET_SNAP_LENGTH 8

// Cisco Aironet SNAP Header 8 + IAPP ID & Length 2 + IAPP Type 1 + IAPP SubType 1 +
// Dest MAC Addr 6 + Src MAC Addr 6 + Dialog Token 2 + dummy Measurement Report Element 1
#define CISCO_RM_RPT_IAPP_LENGTH    27

// Cisco Aironet SNAP Header 8 + IAPP ID & Length 2 + IAPP Type 1 + IAPP SubType 1 +
// Dest MAC Addr 6 + Src MAC Addr 6 + dummy Adjacent Report Element 1
#define CISCO_ADJ_RPT_IAPP_LENGTH    25

#define RT_CCX_PACKET_TYPE u4Byte 
#define CPT_NONE					0 // Not a CCX IAPP Packet
#define CPT_UNKNOWN_IAPP			1
#define CPT_INVALID_IAPP			2
#define CPT_ADJACENT_AP_REPORT		3 // S30, Adjacent AP Report frame. 
#define CPT_RM_REQUEST				4 // S36, Radio Measurement Request frame.
#define CPT_NEIGHBOR_LIST_RESPONSE	5 // S51.2.2 Neighbor List Response frame.
#define CPT_DIRECT_ROAM				6 // S51.2.3 Direct Roam frame.
#define CPT_LINK_TEST_REQUEST		7 // S62.1, Link Test request.
#define CPT_LINK_TEST_REPLY			8 // S62.1, Link Test reply.
#define	CPT_EVENT_LOG_REQUEST		9 // S66, Roaming and Real-time Diagnostics Event log request
#define	CPT_EVENT_LOG_REPORT		10 // S66, Roaming and Real-time Diagnostics Event log report
#define	CPT_EVENT_DIAG_CHNL_REQUEST			11 // S64, Diagnostic Channel request
#define	CPT_EVENT_DIAG_CHNL_REPORT			12 // S64, Diagnostic Channel report
#define	CPT_EVENT_DIAG_CLIENT_RPT_REQUEST	13 // S65, Diagnostic Client Report request
#define	CPT_EVENT_DIAG_CLIENT_RPT_REPORT	14 // S65, Diagnostic Client Report report



#define	CCX_CCKM_REQ_IE_LEN			24 // CCKM request IE length
#define	CCX_ASSOC_INFO_TIME_OUT		2000 // ms for waiting CCX info

#define	CCX_MFP_TKIP_MHDR_IE_LEN	12 // CCXv5 S67 MFP MHDR IE for TKIP excluding element ID and length

#define	CCX_MIN_ROAM_THRESHOLD		-100	// The min of roam threshold.

// =================================================================== //
// CCX IAPP Type and SubType
// =================================================================== //
//
// Definition for IAPP Type and SubType.
//
#define IAPP_CONTROL_MSG			0x0000

#define CCX_ADJACENT_IE_BUF_LEN		70

//
// Definition for IAPP Type and SubType for Adjacent AP Report.
// Ref: CCXv2 S32.
//
#define CCX_AARPT_IAPP_TYPE			0x30
#define CCX_AARPT_IAPP_FUNCTION		0x00

//
// Definition for IAPP Type and SubType.
// Ref: CCXv2 S36.4, S36.5
//
#define CCX_RM_IAPP_TYPE			0x32
#define CCX_RM_REQUEST_IAPP_SUBTYPE	0x01
#define CCX_RM_REPORT_IAPP_SUBTYPE	0x81

//
// Definition of IappType and IappSubtype for 
// Neighbor List Response and  Direct Roam frames.
// Ref: CCXv4 S51.2.3, S51.2.4.
//
#define CCX_L2ROAM_IAPP_TYPE		0x33
#define CCX_NLR_IAPP_SUBTYPE		0x81
#define CCX_DR_IAPP_SUBTYPE			0x82

//
// Link Test.
// Ref: CCXv4 S62.
//
#define CCX_LINK_TEST_IAPP_TYPE		0x41
#define CCX_LT_REQUEST_IAPP_SUBTYPE	0x01
#define CCX_LT_REPLY_IAPP_SUBTYPE	0x81

//
// CCXv5 Diagnostic IAPP Type
//
#define	CCX_EVENT_DIAGNOSTIC_IAPP_TYPE			0x60


//
// CCXv5 S64 Diagnostic Channel
//
#define	CCX_EVENT_DIAGNOSTIC_REQ_IAPP_SUBTYPE	0x01
#define	CCX_EVENT_DIAGNOSTIC_RPT_IAPP_SUBTYPE	0x81

//
// CCXv5 S65 Client Report
//
#define	CCX_EVENT_DIAGNOSTIC_CLIENT_RPT_REQ_IAPP_SUBTYPE	0x02
#define	CCX_EVENT_DIAGNOSTIC_CLIENT_RPT_RPT_IAPP_SUBTYPE	0x82

//
// Roaming and Real-time Diagnostics
// Ref: CCXv5 S66
//
#define	CCX_EVENT_LOG_REQ_IAPP_SUBTYPE	0x03
#define	CCX_EVENT_LOG_RPT_IAPP_SUBTYPE	0x83

#define	CCX_PHY_RATE_MAX					5
// ==================================================================== //

#define	CCX_PKT_STATUS						u4Byte

#define	CCX_PKT_STATUS_UNKNOWN			0		// Cnannot recognize and handle this packet for CCX and follow the default driver flow to handle this packet.
#define	CCX_PKT_STATUS_DROP				BIT0	// Drop this packet and do not process it.
#define	CCX_PKT_STATUS_BY_DRIVER		BIT1	// Handle this packet in local driver and drop it after finishing.
#define	CCX_PKT_STATUS_BY_IHV			BIT2	// Indicate this packet to IHV service and do not process it in local driver.
#define	CCX_PKT_STATUS_BY_DRIVER_IHV	(CCX_PKT_STATUS_BY_DRIVER | CCX_PKT_STATUS_BY_IHV) // Handle this packet in local driver and then also indicate it to IHV service.

typedef struct _CCX_IAPP_HEADER{
	u1Byte				SnapHeader[CISCO_AIRONET_SNAP_LENGTH]; // Cisco Aironet SNAP header: AAAA 0300 4096 0000
	u2Byte				IappIdLen; 
	u1Byte				IappType;
	u1Byte				IappSubtype;
	u1Byte				DstAddr[6]; 
	u1Byte				SrcAddr[6]; 
}CCX_IAPP_HEADER, *PCCX_IAPP_HEADER;
#define CCX_IAPP_HEADER_SIZE 24

#define CCX_ADJ_RPT_HEADER_SIZE			25
#define CCX_ASSOC_REASON_ELEMENT_SIZE	9


#define GET_CCX_IAPPIDLEN(_pPkt)	( N2H2BYTE(((PCCX_IAPP_HEADER)_pPkt)->IappIdLen) )
#define SET_CCX_IAPPIDLEN(_pPkt, _IappIdLen)	( ((PCCX_IAPP_HEADER)_pPkt)->IappIdLen = H2N2BYTE(_IappIdLen) )

#define GET_CCX_IAPPTYPE(_pPkt)	( N2H1BYTE(((PCCX_IAPP_HEADER)_pPkt)->IappType) )
#define SET_CCX_IAPPTYPE(_pPkt, _IappType)	( ((PCCX_IAPP_HEADER)_pPkt)->IappType = H2N1BYTE(_IappType) )

#define GET_CCX_IAPPSUBTYPE(_pPkt)	( N2H1BYTE(((PCCX_IAPP_HEADER)_pPkt)->IappSubtype) )
#define SET_CCX_IAPPSUBTYPE(_pPkt, _IappSubtype)	( ((PCCX_IAPP_HEADER)_pPkt)->IappSubtype = H2N1BYTE(_IappSubtype) )

static u1Byte CCKM_OUI[SIZE_OUI] = {0x00,0x40,0x96};

#define	CCKM_OUI_TYPE	0


//------------------------------------------------------------------------------
// Layer 2 roaming related definition.
//------------------------------------------------------------------------------

//
// Ref: CCXv2 S32.
//
typedef struct _CCX_ADJACENT_REPORT{
	u1Byte				SnapHeader[CISCO_AIRONET_SNAP_LENGTH]; // Cisco Aironet SNAP header: AAAA 0300 4096 0000
	u2Byte				IappIdLen; 
	u1Byte				IappType; // 0x30.
	u1Byte				IappFunction; // 0x00.
	u1Byte				DstAddr[6]; // MAC address of the AP.
	u1Byte				SrcAddr[6]; // Reporting STA's MAC address. 
	u1Byte				Elements[1];
}CCX_ADJACENT_REPORT, *PCCX_ADJACENT_REPORT;

#define SET_AARPT_IAPP_SNAP_HEADER(_pPkt, _header, _len)	(PlatformMoveMemory( _pPkt, _header, _len))
#define SET_AARPT_IAPPIDLEN(_pPkt, _IappIdLen)	(PlatformMoveMemory((_pPkt + 8), _IappIdLen, 2 ))
#define SET_AARPT_IAPPTYPE(_pPkt, _IappType)	(*(_pPkt + 10) = H2N1BYTE(_IappType))
#define SET_AARPT_IAPPFUNCTION(_pPkt, _IappFunction)	( *(_pPkt + 11) = H2N1BYTE(_IappFunction) )
#define SET_AARPT_IAPPDEST(_pPkt, _dest, _len) (PlatformMoveMemory( (_pPkt + 12), _dest, _len) )
#define SET_AARPT_IAPPSRC(_pPkt, _dest, _len) (PlatformMoveMemory( (_pPkt + 18), _dest, _len) )
#define SET_AARPT_ElEMENT(_pPkt, _elemt, _len) (PlatformMoveMemory( (_pPkt + 24), _elemt, _len) )

#define		SET_AARPT_REASON_TAG(_pElmt, _tag) (PlatformMoveMemory( _pElmt, _tag, 2 ))
#define		SET_AARPT_REASON_LEN(_pElmt, _len) (PlatformMoveMemory( (_pElmt + 2), _len, 2 ))
#define		SET_AARPT_REASON_OUI(_pElmt, _airie, _len) (PlatformMoveMemory( (_pElmt + 4), _airie, _len ))
#define		SET_AARPT_REASON_REA(_pElmt, _reason)	( *(_pElmt + 8) = _reason )
//
// Ref: CCXv2 S32.
//
typedef struct _CCX_ADJ_AP_RPT_ELEMENT{
	u2Byte				Tag; // 0x009B
	u2Byte				Length;
	u1Byte				Aironet_OUI[4]; // 0x00 0x40 0x96 0x00
	u1Byte				AdjAPMAC[6]; // MAC address of the AP.
	u2Byte				Channel; // Channel  of the AP.
	u2Byte				SSIDLen; // SSID length of the AP.
	u1Byte				SSID[32];
	u1Byte				Seconds[2];
}CCX_ADJ_AP_RPT_ELEMENT, *PCCX_ADJ_AP_RPT_ELEMENT;

//
// Ref: CCXv4 S51.2.4, figure 51.3A.
//
typedef struct _CCX_ASSOC_REASON_ELEMENT{
	u2Byte				Tag; // 0x009C
	u2Byte				Length;
	u1Byte				Aironet_OUI[4]; // 0x00 0x40 0x96 0x00
	u1Byte				AssocReason; 
}CCX_ASSOC_REASON_ELEMENT, *PCCX_ASSOC_REASON_ELEMENT;

//
// Ref. CCXv4 S51.2.4, figure 51-4.
//
typedef enum _CCX_ASOC_REASON{
	CCX_AR_UNSPECIFIED = 0,
	CCX_AR_NORMAL_ROAM_POOR_LINK = 1,
	CCX_AR_NORMAL_ROAM_LOAD_BALANCING = 2,
	CCX_AR_INSUFFICIENT_CAPACITY = 3,
	CCX_AR_DIRECT_ROAM = 4,
	CCX_AR_FIRST_ASSOCIATION = 5,
	CCX_AR_ROAM_FROM_WAN = 6,
	CCX_AR_ROAM_TO_WAN = 7,
	CCX_AR_NORMAL_ROAM_BETTER_AP = 8,
	CCX_AR_DEAUTH_DISASSOC = 9,
}CCX_ASOC_REASON;

//
// Information from a neighbor report IE.
// Ref: CCXv4 S51.2.5.
//
#define CCX_NB_RFPARM_ID 0x01
#define CCX_NB_RFPARM_SIZE 6
#define CCX_NB_RFPARM_GET_MIN_RX_POWER(_pParam) (s1Byte)(*((pu1Byte)_pParam))
#define CCX_NB_RFPARM_GET_AP_TX_POWER(_pParam) (s1Byte)(*((pu1Byte)(_pParam) + 1))
#define CCX_NB_RFPARM_GET_CLIENT_TX_POWER(_pParam) (s1Byte)(*((pu1Byte)(_pParam) + 2))
#define CCX_NB_RFPARM_GET_ROAM_HYSTERSIS(_pParam) (s1Byte)(*((pu1Byte)(_pParam) + 3))
#define CCX_NB_RFPARM_GET_ADAPTIVE_SCAN_THRESHOLD(_pParam) (s1Byte)(*((pu1Byte)(_pParam) + 4))
#define CCX_NB_RFPARM_GET_TRANSITION_TIME(_pParam) (u1Byte)(*((pu1Byte)(_pParam) + 5))

//
// Roaming related configuration.
//
#define RT_DEF_MIN_RX_POWER -82 // in dBm. 
#define RT_DEF_ADAPTIVE_SCAN_THRESHOLD -72 // in dBm. 
#define RT_MIN_ROAM_HYSTERSIS 10 // in dBm. 
#define RT_DEFAULT_ROAM_HYSTERSIS	6
#define RT_MIN_ADAPTIVE_SCAN_CHECK_INTERVAL 1 // In number of times MgntLinkStatusWatchdog invoked.
#define RT_MAX_ADAPTIVE_SCAN_CHECK_INTERVAL 16 // In number of times MgntLinkStatusWatchdog invoked.
#define	RT_DEFAULT_RX_RETRY_RATIO_FOR_ROAM	100 // Default rx retry ratio to start roaming
#define RT_CCX_RX_RETRY_RATIO_FOR_ROAM		45 // CCX rx retry ratio to start roaming.


//
// Neighbor Report element.
// Ref: S51.2.5, Figure 51-5.
//
#define CCX_NBIE_ID 0x28
#define CCX_NBIE_GET_BSSID(_IE) ((_IE).Content.Octet)
#define CCX_NBIE_GET_CHNL(_IE) ReadEF1Byte((_IE).Content.Octet + 6)
#define CCX_NBIE_GET_BAND(_IE) ReadEF1Byte((_IE).Content.Octet + 7)
#define CCX_NBIE_GET_PHY_TYPE(_IE) ReadEF1Byte((_IE).Content.Octet + 8)

//
// Information about Neighbor.
//
typedef struct _RT_CCX_NEIGHBOR{
	//
	// We will use BSSID as key to hash.
	//
	DECLARE_RT_HASH_ENTRY;

	u1Byte				CurrentChannel;
	u1Byte				ChannelBand;
	u1Byte				PhyType;

	BOOLEAN				bRfParamValid;
	u1Byte				RfParam[CCX_NB_RFPARM_SIZE];

}RT_CCX_NEIGHBOR, *PRT_CCX_NEIGHBOR;

#define MAX_NUM_CCX_NEIGHBOR_LIST 32 
#define CCX_NEIGHBOR_KEY_SIZE 6

//
// CCXv4 roaming state.
//
typedef enum _RT_CCX_ROAMING_STATE{
	CRS_IDLE,
	CRS_TRY_TO_SCAN,
	CRS_TRY_TO_ROAM,
}RT_CCX_ROAMING_STATE;

#define CCX_LINK_TEST_SET_FRAME_NUM(_pEleOffset, _Value) (*((UNALIGNED pu2Byte)(_pEleOffset)) = H2N2BYTE(_Value)) 
#define CCX_LINK_TEST_SET_NUM_RETRY(_pEleOffset, _Value) (*((_pEleOffset)+8) = (_Value)) 
#define CCX_LINK_TEST_SET_RSSI(_pEleOffset, _Value) (*((_pEleOffset)+9) = (_Value)) 

//
// 1. must <= 1600, see definition of MAX_TRANSMIT_BUFFER_SIZE. 
// 2. 1534 = 1514(Eth) - 12 + 30(WLAN) + 2(QoS). 
// 3. 1536 is multiple of 4.
//
#define MAX_CCX_LINK_TEST_PACKET_SIZE 1536 

//------------------------------------------------------------------------------
// CCX Qos part.
//------------------------------------------------------------------------------

enum RT_CCX_ADMISSION_STATUS_CODE
{
	StatusCode_Accepted			= 0,			// ADDTS Rsp, Reasoc Rsp
	StatusCode_AddTs_InvalidParam	= 1,			// ADDTS Rsp
	StatusCode_AddTs_Refused		= 3,			// ADDTS Rsp
	StatusCode_Reasoc_Unspecified	= 0xC8,		// Reasoc Rsp
	StatusCode_PolicyConfFailure		= 0xC9,		// ADDTS Rsp, Reasoc Rsp
	StatusCode_Reasoc_Denied		= 0xCA,		// Reasoc Rsp
	StatusCode_Reasoc_InvalidParam = 0xCB		// Reasoc Rsp
};

typedef struct _RT_CCX_CAC_PHY_RATE {
	u1Byte		WirelessMode;
	u1Byte		NominalPhyRate;
	u2Byte		SurplusBandwith;
} RT_CCX_CAC_PHY_RATE, *PRT_CCX_CAC_PHY_RATE;



#define RT_CCX_CAC_MIN_PHY_RATE(_Adapter) \
	( ((_Adapter)->MgntInfo.CurrentBssWirelessMode==WIRELESS_MODE_B) ? 11 : 12)

#define CCX_CAC_VOICE_TSID			0
#define CCX_CAC_SIGNAL_TSID		1

//------------------------------------------------------------------------------
// CCX general/common part.
//------------------------------------------------------------------------------

//
// CCX Aironet IE.
// Ref:
// CCXv1 S19, Aironet IE.
// CCXv1 S13 (section A01.1 CKIP Negotiation).
// CCxv4 S61 Keep-Alive.
//
#define IE_CISCO_REFRESH_RATE_POSITION	0x03
#define IE_CISCO_CWMIN_POSITION	0x04
#define IE_CISCO_CWMAX_POSITION	0x06
#define IE_CISCO_FLAG_POSITION		0x08	// Flag byte: byte 8, numbered from 0.
#define SUPPORT_CKIP_MIC			0x08	// bit3
#define SUPPORT_CKIP_PK			0x10	// bit4
#define RT_KEEP_ALIVE_INTERVAL 20 // in seconds. 
#define RT_NEIGHBOR_CHECK_INTERVAL 60 // in seconds.

typedef struct _CCX_TSM_COUNTER{
	BOOLEAN				bValid; // TRUE if this counter is valid and shall be updated at packet sent.

	//
	// Counters updated at Tx transmit OK.
	//
	u4Byte				MediaDelay; // in us.
	u2Byte				MediaDelayHistogram[4];
	u4Byte				PacketLossCount;

	//
	// Counters updated at TxFillDescriptor.
	//
	u4Byte				PacketCount;
	u4Byte				TsPacketCount; // #packet sent for the TS.
	u2Byte				UsedTime;
}CCX_TSM_COUNTER, *PCCX_TSM_COUNTER;


//
// Structure to keep Link Test Reply frame retry count.
//
typedef struct _RT_LT_REP_RETRY
{
	BOOLEAN		bUpdated;
	u1Byte		RetryCnt;
	u2Byte		SeqNo;
	u2Byte		FrameNumber;
	u2Byte		PacketID;	// The ID of the HW Tx Desc usage. Only used under RTL819x serious. 
}RT_LT_REP_RETRY, *PRT_LT_REP_RETRY;
// 
// Change the size to 1 because the link test is a ping pong test and we don't need to
// queue more than 1 packet. By Bruce, 2010-07-13.
//
#define MAX_RT_LT_REP_RETRY_SET_SIZE 1

#define	SET_LT_REP_PKT_ID(_pRetry, _PktID)		((_pRetry)->PacketID = _PktID)

//
// Information about the Link Test Request received.
//
typedef struct _RT_LT_REQ
{
	s4Byte		RecvSignalPower; // In dBm
	u2Byte		Reserved;
	u2Byte		MpduLength;
	u1Byte		MpduBuf[MAX_CCX_LINK_TEST_PACKET_SIZE];
}RT_LT_REQ, *PRT_LT_REQ;
#define MAX_RT_LT_REQ_WAIT_QUEUE_SIZE	2
#define	MAX_CCX_SFA_IE_BUF_LEN			5
#define	MAX_CCX_DIAG_REQ_REASON_IE_BUF_LEN		5

//
// Structure to contain general CCX information.
//
typedef struct _RT_CCX_INFO{
	DECLARE_RT_OBJECT(RT_CCX_INFO);

	//
	// 1. CCX version and capability.
	//
	BOOLEAN					bCCXenable;
	u1Byte					BssCcxVerNumber; // 0: not a CCX AP, >0: CCX version of current BSS associated.
	u1Byte					CurrCcxVerNumber; // 0: not acting as CCX device. >0: the CCX version we are using. 
	BOOLEAN					bWithAironetIE;

	//
	// 2. Security.
	// LEAP, PEAP, FAST, CCKM, CKIP.
	//
	BOOLEAN					bCCX8021xenable;  //for fast roam enable or disable 
	BOOLEAN					bCCXwaitCCXInfoInProgess;   // For wait CCX_Info to be set or timeout call back contiune by Mars
	RT_TIMER				SetCCXInfoTimer;
	BOOLEAN					bCCX8021xResultenable;  //for fast roam can send CCKM IE, set by meeting house only.
	u8Byte					CCKMTstamp;
	u4Byte					CCKMRN;
	BOOLEAN					bAPSuportCCKM;
	OCTET_STRING			CCKMIE;
	u1Byte					CCKMIEBuf[64];
	
	//
	// 3. Layer 2 roaming.
	//
	CCX_ASOC_REASON			AssocReason; // Reason to associate current BSS.
	OCTET_STRING			AdjApRptElement;
	u1Byte					AdjApRptElementBuf[CCX_ADJACENT_IE_BUF_LEN];
	RT_HASH_TABLE_HANDLE	hNeighborTable; // Neighbor lists.
	RT_CCX_ROAMING_STATE	state_CcxRoam; // CCX roaming state.
	s4Byte					LowestBssPowerThreshold; // in dBm. We'll try to roam to the BSS if BSS's power is larger than it.
	u1Byte					AdaptiveScanCheckCounter;
	u1Byte					AdaptiveScanCheckInterval;
	BOOLEAN					bBssRfParamValid; // TRUE if BssRfParam is valid.
	u1Byte					BssRfParam[CCX_NB_RFPARM_SIZE]; // RF parameter subelement of current BSS.
	BOOLEAN					bTspecRejected;
	u1Byte					NeighborCheckCount;

	//
	// 4. Radio and link related. 
	//
	
	//
	// For TSM.
	//
	u8Byte					LastRoamStartTime; // in us.
	u2Byte					LastRoamDelay; // in ms.
	u1Byte					RecentRoamingCount; // Number of successfull roam in last TSM measurement interval. For CCX 4 S56.5.2.6, 070621, by rcnjko.
	BOOLEAN					bEnableTSM;			// Indicate enabling TSM or not. 0: disable, 1: enable.
	CCX_TSM_COUNTER			TsmCounters[MAX_STA_TS_COUNT];	// Tsm Info according to the specified TSID.
	// For CCXv5 Catastrophic Roaming Scenario, 4.1.9.4/4.1.9.5
	u1Byte					CatasRoamCnt;	
	u1Byte					CatasRoamLimit;

	//
	// For keep-alive mechanism. 
	//
	u1Byte					IdleCount;
	u8Byte					LastNumTxUnicast;
	u8Byte					LastNumRxUnicast;

	//
	// For roaming decision, record the rx retry info.
	//
	u8Byte					LastRxRetryCnt;
	u8Byte					LastRxUniCnt;
	u1Byte					LastRetryRatio;
	
	//
	// Link Test, which are protected by RT_RM_SPINLOCK.
	//
	u2Byte					LtFrameNumber;
	RT_LT_REP_RETRY			LtReplyRetrySet[MAX_RT_LT_REP_RETRY_SET_SIZE];
	u4Byte					LtReplyRetrySetStartIdx;
	u4Byte					LtReplyRetrySetSize;
	RT_LT_REQ				LtRequestWaitQueue[MAX_RT_LT_REQ_WAIT_QUEUE_SIZE];
	u4Byte					LtRequestWaitQueueStartIdx;
	u4Byte					LtRequestWaitQueueSize;
	BOOLEAN					bSendingLtReply;
	u1Byte					LtReqQueueCheckCount;	
	u1Byte					LtreplyRetryID;	// The ID to determine the link test packet for Tx descriptor usage.

	//
	// 5. QoS
	//
	RT_TIMER				SessionRetryTimer;
	BOOLEAN					SessionStartFrameRejected;
	BOOLEAN					bFakeDecreasingSignal;
	s4Byte					FakeRxSignalPower;
        
	//
	//7. SSIDL
	//
	RT_WLAN_BSS				ExtenSSIDBSS;
	RT_WLAN_BSS				JoinSSIDBSS;

	BOOLEAN					bCCXMIXenable;

	// CCX 2 S31, AP control of client transmit power. 060927, by rcnjko.
	BOOLEAN					bWithCcxCellPwr;
	u1Byte					CcxCellPwr; // Cell power limit in dBm.
	BOOLEAN					bUpdateCcxPwr; // Update CCX Tx Power to HW.

	// CCX CAC Enabled Flag
	BOOLEAN					bCcxCACEnable;

	// CCX v5 S64 Diagnostic Channel Mode
	BOOLEAN					bCcxDiagnosticMode;

	// CCX IHV Frame Logging mode
	BOOLEAN					bIhvFrameLogMode;

	// CCX Tx Frame Buffer for indication to IHV
	u1Byte					IhvTxIndicBuffer[sMaxMpduLng];

	// CCX V5 S67
	BOOLEAN					bCcxMFPEnable; // Enable MFP mode 
	u1Byte					MFPtk[MAX_KEY_LEN];
	u1Byte					MFPAESKeyBuf[AESCCMP_BLK_SIZE_TOTAL];

	// CCXv5 SFA IE
	OCTET_STRING			CCXSFAIE;
	u1Byte					CCXSFAIEBuf[MAX_CCX_SFA_IE_BUF_LEN];

	// CCXv5 S64 Diagnostic Request Reason IE
	OCTET_STRING			CCXDiagReqReasonIE;
	u1Byte					CCXDiagReqReasonIEBuf[MAX_CCX_DIAG_REQ_REASON_IE_BUF_LEN];

	BOOLEAN					bSendEAPStar;
	u1Byte					SendEAPStarCount;
	
	// CCX v2 Send CCX report by Driver !!
	BOOLEAN					bSendCCXReport;

	// Add for common binary, we can not use global variable.
	RT_CCX_CAC_PHY_RATE	RtCcxCacVoPhyRate[CCX_PHY_RATE_MAX];

	// Flag for marking the OID is coming from IHV
	BOOLEAN					bQueryFromIHV;
	
}RT_CCX_INFO, *PRT_CCX_INFO;


#define	CCX_MAX_PACKET_ID	4096

#define	SUPPORT_CCX(__pAdapter)		\
	(GET_CCX_INFO(&((__pAdapter)->MgntInfo)) ? ((GET_CCX_INFO(&((__pAdapter)->MgntInfo)))->bCCXenable ? TRUE : FALSE) : FALSE)

#define GET_CCX_INFO(__pMgntInfo) ( (PRT_CCX_INFO)((__pMgntInfo)->pCcxInfo) )

#define CCX_IS_MFP_ENABLED(___pAdapter)	(SUPPORT_CCX(___pAdapter) ? ((GET_CCX_INFO(&((___pAdapter)->MgntInfo)))->bCcxMFPEnable) : FALSE)

#define GET_CURR_CCX_VER(__pAdapter) ( ((PRT_CCX_INFO)((__pAdapter)->MgntInfo.pCcxInfo))->CurrCcxVerNumber ) 
#define GET_HW_CCX_VER(__pAdapter) ((__pAdapter)->HalFunc.CcxVerSupported) 


//
// CCX per-packet information embedded in RT_TCB.
//
#define RESET_CCX_PACKET_TX_INFO(_pTcb) \
		((_pTcb)->CcxIappPacketType = CPT_NONE); \
		((_pTcb)->CcxLtFrameNumber = 0)

#define IS_CCX_LINK_TEST_REPLY(_pTcb) ((_pTcb)->CcxIappPacketType == CPT_LINK_TEST_REPLY)
#define SET_CCX_LINK_TEST_REPLY(_pTcb) ((_pTcb)->CcxIappPacketType = CPT_LINK_TEST_REPLY)

#define GET_CCX_LINK_TEST_REPLY_FRAME_NUMBER(_pTcb) ((_pTcb)->CcxLtFrameNumber)
#define SET_CCX_LINK_TEST_REPLY_FRAME_NUMBER(_pTcb, _FrameNumber) ((_pTcb)->CcxLtFrameNumber = _FrameNumber)


typedef struct _CCX_SSIDL_SSID_FIELD{
	u1Byte				ExtenCapab;  // bit1: 1x bit2: WPS bit2~5 :MS reserve bit6.7: Cisco reserve 
	u1Byte				Capability[4];
	u1Byte				SSIDLen; // 0x00 0x40 0x96 0x00
	u1Byte				SSID[1]; 
}CCX_SSIDL_SSID_ELEMENT, *PCCX_SSIDL_SSID_ELEMENT;


// SSIDL Mutilcase-Chiper  
#define CCX_IS_MCHIPER_NONE( _Capability ) 		( _Capability[3] & 0x00 ?TRUE:FALSE)
#define CCX_IS_MCHIPER_WEP40( _Capability ) 		( _Capability[3] & 0x01 ?TRUE:FALSE)
#define CCX_IS_MCHIPER_WEP104( _Capability ) 	( _Capability[3] & 0x02 ?TRUE:FALSE)
#define CCX_IS_MCHIPER_TKIP( _Capability )  		( _Capability[3] & 0x03 ?TRUE:FALSE)
#define CCX_IS_MCHIPER_CCMP( _Capability ) 		( _Capability[3] & 0x04 ?TRUE:FALSE)
#define CCX_IS_MCHIPER_CKIP_CMIC( _Capability ) 	( _Capability[3] & 0x05 ?TRUE:FALSE)
#define CCX_IS_MCHIPER_CKIP( _Capability )  		( _Capability[3] & 0x06 ?TRUE:FALSE)
#define CCX_IS_MCHIPER_CMIC( _Capability )   		( _Capability[3] & 0x07 ?TRUE:FALSE)

// SSIDL Untilcase-Chiper  
#define CCX_IS_UCHIPER_NONE( _Capability ) 	   	(  _Capability[3] & 0x10 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_WEP40( _Capability ) 		(  _Capability[3] & 0x20 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_WEP104( _Capability )	(  _Capability[3] & 0x40 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_TKIP( _Capability ) 		(  _Capability[3] & 0x80 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_CCMP( _Capability )	 	(  _Capability[2] & 0x01 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_CKIP_CMIC( _Capability ) 	(  _Capability[2] & 0x02 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_CKIP( _Capability ) 		(  _Capability[2] & 0x04 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_CMIC( _Capability ) 		(  _Capability[2] & 0x08 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_WPA2_WEP40( _Capability ) 	( _Capability[2] & 0x10 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_WPA2_WEP104( _Capability ) 	(  _Capability[2] & 0x20 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_WPA2_TKIP( _Capability ) 	(  _Capability[2] & 0x40 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_WPA2_CCMP( _Capability ) 	(  _Capability[2] & 0x80 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_WPA2_CKIP_CMIC( _Capability ) 	(  _Capability[1] & 0x01 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_WPA2_CKIP( _Capability ) 		(  _Capability[1] & 0x02 ?TRUE:FALSE)
#define CCX_IS_UCHIPER_WPA2_CMIC( _Capability ) 		(  _Capability[1] & 0x04 ?TRUE:FALSE)

// SSIDL AKM
#define  CCX_IS_AKM_WPA1_1X( _Capability )  		(  _Capability[0] & 0x02 ?TRUE:FALSE)
#define  CCX_IS_AKM_WPA1_PSK( _Capability )  	(  _Capability[0] & 0x04 ?TRUE:FALSE)
#define  CCX_IS_AKM_WPA2_1X( _Capability )  		(  _Capability[0] & 0x08 ?TRUE:FALSE)
#define  CCX_IS_AKM_WPA2_PSK( _Capability )  	(  _Capability[0] & 0x10 ?TRUE:FALSE)
#define  CCX_IS_AKM_WPA1_CCKM( _Capability )  	(  _Capability[0] & 0x20 ?TRUE:FALSE)
#define  CCX_IS_AKM_WPA2_CCKM( _Capability )  	(  _Capability[0] & 0x40 ?TRUE:FALSE)

#endif
