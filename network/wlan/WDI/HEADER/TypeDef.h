#ifndef __INC_TYPEDEF_H
#define __INC_TYPEDEF_H


//=============================================================================
// MAX_TX_QUEUE Should be the MAX Tx queue number
//=============================================================================

#define MAX_TX_QUEUE							9	// BK, BE, VI, VO, HCCA, MANAGEMENT, COMMAND, HIGH, BEACON. 
#define MAX_RX_QUEUE							1	// MSDU packet queue, After C serie no Rx Command Queue
//

#define MAX_SUBFRAME_COUNT					64	// Max number of subframe per A-MSDU
#define MAX_FRAGMENT_COUNT					8	// Max number of MPDU per MSDU

#define MAX_PER_PACKET_VIRTUAL_BUF_NUM		24
#define MAX_PER_PACKET_PHYSICAL_BUF_NUM		MAX_PER_PACKET_VIRTUAL_BUF_NUM
#define MAX_PER_PACKET_BUFFER_LIST_LENGTH	(MAX_PER_PACKET_PHYSICAL_BUF_NUM+MAX_FRAGMENT_COUNT+4+16)		// +4 is for FirmwareInfo, Header, LLC and trailer, MAX_FRAGMENT_COUNT for max fragment overhead
														// +16 is for A-MSDU aggregation. Add this to aggregate more frames.

#define RTL8192EE_SEG_NUM	1 // 0:2 seg, 1: 4 seg, 2: 8 seg
#define RTL8814AE_SEG_NUM	1 // 0:2 seg, 1: 4 seg, 2: 8 seg
#define RTL8821BE_SEG_NUM	1 // 0:2 seg, 1: 4 seg, 2: 8 seg
#define RTL8822BE_SEG_NUM	1 // 0:2 seg, 1: 4 seg, 2: 8 seg
#define RTL8723DE_SEG_NUM	1 // 0:2 seg, 1: 4 seg, 2: 8 seg


#define	MAX_TX_REG_NUM		20
#define	MAX_RX_REG_NUM		20
//=============================================================
// TCB buffer management related definition 
//=============================================================
#define MAX_FIRMWARE_INFORMATION_SIZE		32 //2006/04/30 by Emily forRTL8190
// TODO: MAX_802_11_HEADER_LENGTH should be hardware platform independent
// Emily: The reason to set this value differently on different hardware platform 
// is because  if we set this value more than 40 , and use 8187b with TKIP mode, 
// system crash for some unknown reason. 2006.10.20. After we fix this bug, we
// should modify this definition

#define MAX_802_11_HEADER_LENGTH		40
#define	MAX_PKT_AGG_NUM		256

#define MAX_LLC_LENGTH							20
#define MAX_802_11_TRAILER_LENGTH				8	// 8(Sucurity trailer)

#define AMSDU_SUBHEADER_LENGTH				17  // 17(A-MSDU subheaders+A-MSDU paddings)
#define MAX_AMSDU_PADDING_LENGTH			3	// 3(A-MSDU paddings)


#define MAX_MCAST_LIST_NUM					32		// multicast list size
#define MAX_DEFRAG_PEER						16
#define RT_MAX_LD_SLOT_NUM					10

// Increase link retry count, fast to connect to ap within 10 second
#define MAX_JOIN_RETRY_COUNT					3 // 1 // 5


#define ASSOCIATE_ENTRY_NUM					64 // Max size of AsocEntry[].
#define MAX_LOCKED_STA_LIST_NUM				32	// Max size of LockedSTAList.
#define MAX_STA_DATARATE_LIST_NUM			ASSOCIATE_ENTRY_NUM	// Max size of ForcedDataRateStaList[]. 


// Excluded MAC address list.
#define MAX_EXCLUDED_MAC_ADDRESS_LIST		4
#define MAX_REJECTED_ACCESS_AP				4

#define AMSDU_MAX_NUM							16 // Test A-MSDU

// Number of SSID supported in scan process. 2006.12.19, by shien chang.
#define MAX_SSID_TO_SCAN						8

// Default check for hang period in second
#define	RT_CHECK_FOR_HANG_PERIOD			2 // 2 secs

#define	RT_ACTION_FRAME_DELAY_CNT		2	// based on check for hang timer 2 secs to count

#define	RX_REORDER_ENTRY_NUM		512  // m

#define	RT_SCAN_EMPTY_DUMP_REG_CNT		3


// The number of Max(ICs)
#define EARLY_MODE_MAX_PKT_NUM		15
#define EARLY_MODE_MAX_PKT_NUM_8812 15
#define EARLY_MODE_MAX_PKT_NUM_88E 	10
#define EARLY_MODE_MAX_PKT_NUM_92D 	5
#define EARLY_MODE_MAX_PKT_NUM_8723B 15
#define EARLY_MODE_MAX_PKT_NUM_8814A 15
#define EARLY_MODE_MAX_PKT_NUM_8821B 15
#define EARLY_MODE_MAX_PKT_NUM_8822B 15



//MAC0 will wirte PHY1
#define	MAC0_ACCESS_PHY1	0x4000
//MAC1 will wirte PHY0
#define	MAC1_ACCESS_PHY0	0x2000


#define MAX_H2C_CMD_DATA_SIZE		7 // The largest size of an H2C cmd is 8 bytes (after 88E series), the first bytes is CMDID.

#define	BATCH_INDICATION_LEVEL_NUM						10
#define	BATCH_INDICATION_MAX_SHAKE_NUM					8
#define	BATCH_INDICATION_SHAKE_TO_STABLE					2
#define	BATCH_INDICATION_STABLE_NUM						4
#define	BATCH_INDICATION_START_POINT						5000//3000  //YJ,mod for 88E,111229
#define	BATCH_INDICATION_PERCENTAGE_OF_DATA_RATE		38
#define	MAX_BATCH_INDICATION_NUMBER						24 
#define	MAX_BATCH_INDICATION_NUMBER_8814A				127 //add by ylb 20130729 

#define	MAX_TCP_SEQ_ENTRY_NUM	512

#define	TX_DESC_NUM_92E				512
#define	RX_DESC_NUM_92E				512

#define	TX_DESC_NUM_8723D				512
#define	RX_DESC_NUM_8723D				512

#define	TX_DESC_NUM_8814A				1024//2048 change by ylb 20130624
#define	RX_DESC_NUM_8814A				1024 //512 change by ylb 20130624
#define	EXT_TX_DESC_BLOCK_NUM_8814A			(768)//add by ylb 20141124

#define	TX_DESC_NUM_8821B				512
#define	RX_DESC_NUM_8821B				512

#define	TX_DESC_NUM_8822B				512
#define	RX_DESC_NUM_8822B				512

#define	RFD_LOWER_BOUND		32

#define RF_PATH_MAX_NUM					8

#define RTS_CTS       				0
#define CTS_TO_SELF                 1


typedef struct _RT_FILE_HANDLER {
	NDIS_HANDLE	FileHandler;
	pu1Byte		MappedFile;
	u4Byte		FileLength;

} RT_FILE_HANDLER, *PRT_FILE_HANDLER;

typedef struct _RT_SSIDS_TO_SCAN {
	OCTET_STRING		Ssid[MAX_SSID_TO_SCAN];
	u1Byte				SsidBuf[MAX_SSID_TO_SCAN][MAX_SSID_LEN];
	u1Byte				NumSsid;
} RT_SSIDS_TO_SCAN, *PRT_SSIDS_TO_SCAN;


typedef enum _WIRELESS_MODE {
	WIRELESS_MODE_UNKNOWN = 0x00,
	WIRELESS_MODE_A = 0x01,
	WIRELESS_MODE_B = 0x02,
	WIRELESS_MODE_G = 0x04,
	WIRELESS_MODE_AUTO = 0x08,
	WIRELESS_MODE_N_24G = 0x10,
	WIRELESS_MODE_N_5G = 0x20,
	WIRELESS_MODE_AC_5G = 0x40,
	WIRELESS_MODE_AC_24G  = 0x80,
	WIRELESS_MODE_AC_ONLY  = 0x100,
	WIRELESS_MODE_MAX  = 0x800,
	WIRELESS_MODE_ALL =0xFFFF //Isaiah for MacOS compiler
} WIRELESS_MODE;

typedef enum _HT_MODE {
	HT_MODE_UNDEFINED	= 0x00,
	HT_MODE_DISABLE		= 0x01,
	HT_MODE_HT			= 0x02,
	HT_MODE_VHT			= 0x04
} HT_MODE;


typedef enum _RESET_TYPE {
	RESET_TYPE_NORESET = 0x00,
	RESET_TYPE_NORMAL = 0x01,
	RESET_TYPE_SILENT = 0x02
} RESET_TYPE;

typedef enum _PS_BBREGBACKUP{
	PSBBREG_RF0 = 0,
	PSBBREG_RF1,
	PSBBREG_RF2,
	PSBBREG_AFE0,
	PSBBREG_MAX,
} PS_BBREGBACKUP;

typedef enum _PA_MODE {
	PA_MODE_EXTERNAL = 0x00,
	PA_MODE_INTERNAL_SP3T = 0x01,
	PA_MODE_INTERNAL_SPDT = 0x02	
} PA_MODE;

// Added for different registry settings to adjust TxPwr index. added by Roger, 2010.03.09.
typedef enum _TX_PWR_PERCENTAGE{
	TX_PWR_PERCENTAGE_0 = 0x01, // 12.5%
	TX_PWR_PERCENTAGE_1 = 0x02, // 25%
	TX_PWR_PERCENTAGE_2 = 0x04, // 50%
	TX_PWR_PERCENTAGE_3 = 0x08, //100%, default target output power.	
} TX_PWR_PERCENTAGE;


// Joseph TODO:
// This shall be modified for 8190
#define MAX_WIRELESS_MODE_CNT	3

#define IS_WIRELESS_MODE_A(_Adapter)		((_Adapter)->MgntInfo.dot11CurrentWirelessMode == WIRELESS_MODE_A)
#define IS_WIRELESS_MODE_B(_Adapter)		((_Adapter)->MgntInfo.dot11CurrentWirelessMode == WIRELESS_MODE_B)
#define IS_WIRELESS_MODE_G(_Adapter)		((_Adapter)->MgntInfo.dot11CurrentWirelessMode == WIRELESS_MODE_G)
#define IS_WIRELESS_MODE_N_24G(_Adapter)	((_Adapter)->MgntInfo.dot11CurrentWirelessMode == WIRELESS_MODE_N_24G)
#define IS_WIRELESS_MODE_N_5G(_Adapter)	((_Adapter)->MgntInfo.dot11CurrentWirelessMode == WIRELESS_MODE_N_5G)
#define IS_WIRELESS_MODE_AC_24G(_Adapter)	((_Adapter)->MgntInfo.dot11CurrentWirelessMode == WIRELESS_MODE_AC_24G)
#define IS_WIRELESS_MODE_AC_5G(_Adapter)	((_Adapter)->MgntInfo.dot11CurrentWirelessMode == WIRELESS_MODE_AC_5G)
#define IS_WIRELESS_MODE_AC_5G_ONLY(_Adapter)	((_Adapter)->MgntInfo.dot11CurrentWirelessMode == WIRELESS_MODE_AC_ONLY)


#define IS_WIRELESS_MODE_24G(_Adapter)	(IS_WIRELESS_MODE_B(_Adapter) || IS_WIRELESS_MODE_G(_Adapter) || IS_WIRELESS_MODE_N_24G(_Adapter) || IS_WIRELESS_MODE_AC_24G(_Adapter))
#define IS_WIRELESS_MODE_5G(_Adapter)		(IS_WIRELESS_MODE_A(_Adapter) || IS_WIRELESS_MODE_N_5G(_Adapter) || IS_WIRELESS_MODE_AC_5G(_Adapter) || IS_WIRELESS_MODE_AC_5G_ONLY(_Adapter))
#define IS_WIRELESS_MODE_N(_Adapter)		((_Adapter)->MgntInfo.dot11CurrentWirelessMode >= WIRELESS_MODE_N_24G)
#define IS_WIRELESS_MODE_HT_24G(_Adapter)	(IS_WIRELESS_MODE_N_24G(_Adapter) || IS_WIRELESS_MODE_AC_24G(_Adapter))
#define IS_WIRELESS_MODE_AC(_Adapter)		(IS_WIRELESS_MODE_AC_24G(_Adapter) || IS_WIRELESS_MODE_AC_5G(_Adapter) || IS_WIRELESS_MODE_AC_5G_ONLY(_Adapter))
#define IS_WIRELESS_OFDM_24G(_Adapter)	(IS_WIRELESS_MODE_G(_Adapter) || IS_WIRELESS_MODE_N_24G(_Adapter) || IS_WIRELESS_MODE_AC_24G(_Adapter))

#define IS_24G_WIRELESS_MODE(_WIRELESS_MODE)		((_WIRELESS_MODE)&(WIRELESS_MODE_B|WIRELESS_MODE_G|WIRELESS_MODE_N_24G|WIRELESS_MODE_AC_24G))
#define IS_5G_WIRELESS_MODE(_WIRELESS_MODE)			((_WIRELESS_MODE)&(WIRELESS_MODE_A|WIRELESS_MODE_N_5G|WIRELESS_MODE_AC_5G|WIRELESS_MODE_AC_ONLY))
#define IS_N_WIRELESS_MODE(_WIRELESS_MODE)			((_WIRELESS_MODE)&(WIRELESS_MODE_N_5G|WIRELESS_MODE_AC_5G|WIRELESS_MODE_N_24G|WIRELESS_MODE_AC_24G))
#define IS_AC_WIRELESS_MODE(_WIRELESS_MODE)			((_WIRELESS_MODE)&(WIRELESS_MODE_AC_5G|WIRELESS_MODE_AC_24G|WIRELESS_MODE_AC_ONLY))

// Add for advanced wireless mode
#define IS_WIRELESS_MODE_A_ONLY(_WIRELESS_MODE)		(_WIRELESS_MODE == WIRELESS_MODE_A)
#define IS_WIRELESS_MODE_B_ONLY(_WIRELESS_MODE)		(_WIRELESS_MODE == WIRELESS_MODE_B)
#define IS_WIRELESS_MODE_WITHOUT_B(_WIRELESS_MODE)	(!(_WIRELESS_MODE & WIRELESS_MODE_B))
#define IS_WIRELESS_MODE_G_AND_BG(_WIRELESS_MODE)	((_WIRELESS_MODE & WIRELESS_MODE_G) && !(_WIRELESS_MODE & WIRELESS_MODE_A))

#define IS_HT_SUPPORTED(_Adapter)		(_Adapter->RegHTMode >= HT_MODE_HT)
#define IS_VHT_SUPPORTED(_Adapter)		(_Adapter->RegHTMode >= HT_MODE_VHT)

//
// <Roger_Notes> Auto channel selection mechanism for P2P GO, i.e., Operation channel
// 2015.05.15.
//
#if (AUTO_CHNL_SEL_NHM ==1)	
#define IS_AUTO_CHNL_SUPPORT(_Adapter)	(GetDefaultAdapter(_Adapter)->MgntInfo.AutoChnlSel.bAutoChnlSel)
#define IS_AUTO_CHNL_IN_PROGRESS(_Adapter)	(_Adapter->MgntInfo.AutoChnlSel.AutoChnlSelCalInProgress)
#define GET_AUTO_CHNL_SELECTED_NUM(_Adapter)		(GetDefaultAdapter(_Adapter)->MgntInfo.AutoChnlSel.AutoChnlNumberSelected)
#define GET_AUTO_CHNL_STATE(_Adapter)		(GetDefaultAdapter(_Adapter)->MgntInfo.AutoChnlSel.AutoChnlStep)
#define SET_AUTO_CHNL_STATE(_Adapter, _State)\
{\
	PADAPTER pLoopAdapter = GetDefaultAdapter(_Adapter);\
	while(pLoopAdapter)\
	{\
		PlatformAcquireSpinLock(_Adapter, RT_ACS_SPINLOCK);\
		pLoopAdapter->MgntInfo.AutoChnlSel.AutoChnlStep= _State;\
		PlatformReleaseSpinLock(_Adapter, RT_ACS_SPINLOCK);\
		pLoopAdapter = GetNextExtAdapter(pLoopAdapter);\
	}\
}

#define SET_AUTO_CHNL_PROGRESS(_Adapter, _bInProgress)\
{\
	PADAPTER pLoopAdapter = GetDefaultAdapter(_Adapter);\
	while(pLoopAdapter)\
	{\
		PlatformAcquireSpinLock(_Adapter, RT_ACS_SPINLOCK);\
		pLoopAdapter->MgntInfo.AutoChnlSel.AutoChnlSelCalInProgress = _bInProgress;\
		PlatformReleaseSpinLock(_Adapter, RT_ACS_SPINLOCK);\
		pLoopAdapter = GetNextExtAdapter(pLoopAdapter);\
	}\
}

#define SET_AUTO_CHNL_SCAN_PERIOD(_Adapter, _Period)\
{\
	PADAPTER pLoopAdapter = GetDefaultAdapter(_Adapter);\
	while(pLoopAdapter)\
	{\
		PlatformAcquireSpinLock(_Adapter, RT_ACS_SPINLOCK);\
		pLoopAdapter->MgntInfo.AutoChnlSel.AutoChnlSelPeriod = _Period;\
		PlatformReleaseSpinLock(_Adapter, RT_ACS_SPINLOCK);\
		pLoopAdapter = GetNextExtAdapter(pLoopAdapter);\
	}\
}

typedef enum _AUTO_CHNL_SEL_STATE{
	ACS_BEFORE_NHM = 0,	// Before NHM measurement
	ACS_IN_NHM = 1,	// ACS is Calculating by NHM measurement
	ACS_AFTER_NHM = 2,		// ACS by NHM measurement is done
} AUTO_CHNL_SEL_STATE, *PAUTO_CHNL_SEL_STATE;

typedef   struct _RT_AUTO_CHNL_SEL
{	
	BOOLEAN			bAutoChnlSel;	
	RT_WORK_ITEM	AutoChnlSelWorkitem;
	u1Byte			AutoChnlSelCalInProgress;
	u1Byte			AutoChnlStep;	
	u2Byte			AutoChnlSelPeriod; // NHM measured period
	u1Byte			AutoChnlNumberSelected; // Channel number selected by NHM
}RT_AUTO_CHNL_SEL, *PRT_AUTO_CHNL_SEL;

#else
#define IS_AUTO_CHNL_SUPPORT(_Adapter)	FALSE
#define IS_AUTO_CHNL_IN_PROGRESS(_Adapter)	FALSE
#define GET_AUTO_CHNL_SELECTED_NUM(_Adapter)		1
#define SET_AUTO_CHNL_STATE(_Adapter, _State)
#endif

typedef enum _RT_TCB_BUFFER_TYPE{
	RT_TCB_BUFFER_TYPE_NONE,
	RT_TCB_BUFFER_TYPE_SYSTEM,				// Packet is from upper layer
	RT_TCB_BUFFER_TYPE_RFD,					// Receive forwarding packet
	RT_TCB_BUFFER_TYPE_LOCAL,				// Management
	RT_TCB_BUFFER_TYPE_FW_LOCAL,			// Firmware download buffer
	RT_TCB_BUFFER_TYPE_BT_LOCAL,				// BT 3.0
}RT_TCB_BUFFER_TYPE,*PRT_TCB_BUFFER_TYPE;

typedef enum _RT_PROTOCOL_TYPE{
	RT_PROTOCOL_802_3,
	RT_PROTOCOL_802_11,
	RT_PROTOCOL_COMMAND
}PROTOCOL_TYPE, *PPROTOCOL_TYPE;

typedef enum _RT_OP_MODE{
	RT_OP_MODE_AP,
	RT_OP_MODE_INFRASTRUCTURE,
	RT_OP_MODE_IBSS,
	RT_OP_MODE_NO_LINK,
}RT_OP_MODE, *PRT_OP_MODE;


typedef enum _RT_JOIN_NETWORKTYPE{
	RT_JOIN_NETWORKTYPE_INFRA = 1,
	RT_JOIN_NETWORKTYPE_ADHOC = 2,
	RT_JOIN_NETWORKTYPE_AUTO  = 3,
}RT_JOIN_NETWORKTYPE;

typedef enum _RT_JOIN_ACTION{
	RT_JOIN_INFRA   = 1,
	RT_JOIN_IBSS  = 2,
	RT_START_IBSS = 3,
	RT_NO_ACTION  = 4,
}RT_JOIN_ACTION;

typedef enum _RT_LED_ASSOC_STATE{
	LED_ASSOC_SECURITY_BEGIN	= 1,
	LED_ASSOC_SECURITY_END	= 2,	
	LED_ASSOC_SECURITY_NONE	= 3,
}RT_LED_ASSOC_STATE;

typedef enum _RT_802_11_RELOAD_DEFAULTS{
	RT802_11ReloadWEPKeys
}RT_802_11_RELOAD_DEFAULTS, *PRT_802_11_RELOAD_DEFAULTS;


typedef enum{
	MLMESTARTREQ_NONE  = 0,
	MLMESTARTREQ_INIT  = 1,
	MLMESTARTREQ_NWTYPE_CHANGE  = 2,
	MLMESTARTREQ_AUTH_CHANGE  = 3,
	MLMESTARTREQ_KEY_CHANGE  = 4,
	MLMESTARTREQ_PRIVACY_CHANGE  = 5,
}RT_MLMESTARTREQ_RSN;



typedef enum{
	ROAMINGSTATE_IDLE  = 0,
	ROAMINGSTATE_SCANNING  = 1,
	ROAMINGSTATE_AUTHENTICATION  = 2,
	ROAMINGSTATE_REASSOCIATION  = 3,
}RT_ROAMING_STATE;

//
// Indication of roaming status, by Bruce, 2008-05-16.
//
typedef enum _RT_ROAM_TYPE{
	RT_ROAMING_NONE = 0,	// Non-roaming. 
	RT_ROAMING_NORMAL,		// Standard 802.11 roaming procedure. (= bRoaming)
	RT_ROAMING_BY_DEAUTH,	// Not real 802.11 roaming, just for Ndis event to keep re-connecting to the original AP. ( = bRoamingByDeauth)
	RT_ROAMING_BY_SLEEP,	// Not real 802.11 roaming, just for Ndis event to keep re-connecting to the original AP after S3/S4. ( = bRoamingbySleep)
	RT_ROAMING_BY_DISCONNECT_POOR_LINK,	// Become disconnected by poor link, just find a better AP or try to join the original one.
} RT_ROAM_TYPE, *PRT_ROAM_TYPE;



//2004-04-07 for RTL8185
typedef struct _CHANNEL_ACCESS_SETTING
{
	u1Byte	CWminIndex;
	u1Byte	CWmaxIndex;
} CHANNEL_ACCESS_SETTING, *PCHANNEL_ACCESS_SETTING;

typedef enum _PROTECTION_MECHANISM {
	PROTECTION_MECHANISM_NONE = 0x00,
	PROTECTION_MECHANISM_RTS_CTS = 0x01,
	PROTECTION_MECHANISM_CTS_TO_SELF = 0x02,
} PROTECTION_MECHANISM;

//test joseph
typedef enum _IO_READ_WRITE_TYPE{
	IO_READ_WRITE_TYPE_BYTE,
	IO_READ_WRITE_TYPE_WORD,
	IO_READ_WRITE_TYPE_DWORD,
}IO_READ_WRITE_TYPE, *PIO_READ_WRITE_TYPE;

typedef enum _IO_READ_WITE_OPERATION{
	IO_READ_WRITE_OPERATION_READ = 0,
	IO_READ_WRITE_OPERATION_WRITE = 1,
}IO_READ_WRITE_OPERATION, *PIO_READ_WRITE_OPERATION;	
//end test joseph

typedef enum _MAC_FILTER_TYPE{
	MAC_FILTER_DISABLE,
	MAC_FILTER_ACCEPT,
	MAC_FILTER_REJECT,
	MAC_FILTER_LOCK,
}MAC_FILTER_TYPE, *PMAC_FLTER_TYPE;

typedef enum _ASOCENTRY_UPDATE_ASOC_INFO_ACTION
{
	ALLOCATE_ASOC_REQ,
	ALLOCATE_ASOC_RSP,
	ALLOCATE_ASOC_RSP_IE_FROM_OS,
	UPDATE_ASOC_REQ,
	UPDATE_ASOC_RSP,
	UPDATE_ASOC_RSP_IE_FROM_OS,
	FREE_ASOC_REQ,
	FREE_ASOC_RSP,
	FREE_ASOC_RSP_IE_FROM_OS
} ASOCENTRY_UPDATE_ASOC_INFO_ACTION, *PASOCENTRY_UPDATE_ASOC_INFO_ACTION;

typedef enum _NDIS_VERSION_BASE{
	NDIS_VERSION_BASE_6_50 	= 0x00060032,	// 6.50 : Threshold (Microsoft Temporarily Assigned, Not Published, 2014.08.19)
	NDIS_VERSION_BASE_6_40 	= 0x00060028,	// 6.40 : WinBlue (Microsoft Temporarily Assigned, Not Published, 2013.03.29)
	NDIS_VERSION_BASE_6_30 	= 0x0006001E,	// 6.30 : Win8
	NDIS_VERSION_BASE_6_20 	= 0x00060014,	// 6.20 : Win7
	NDIS_VERSION_BASE_6_1 	= 0x00060001,
	NDIS_VERSION_BASE_6_0 	= 0x00060000,
	NDIS_VERSION_BASE_5_2 	= 0x00050002,
	NDIS_VERSION_BASE_5_1 	= 0x00050001,
	NDIS_VERSION_BASE_5_0 	= 0x00050000,
} NDIS_VERSION_BASE, *PNDIS_VERSION_BASE;


typedef enum _RT_NDIS_VERSION{
	RT_NDIS_VERSION_NONE_NDIS,
	RT_NDIS_VERSION_5_0,
	RT_NDIS_VERSION_5_1,
	RT_NDIS_VERSION_6_0,
	RT_NDIS_VERSION_6_1,
	RT_NDIS_VERSION_6_20,
	RT_NDIS_VERSION_6_30,
	RT_NDIS_VERSION_6_40,	
	RT_NDIS_VERSION_6_50,	
}RT_NDIS_VERSION, *PRT_NDIS_VERSION;

typedef enum _RT_STA_INDICATE_STATE
{
	RT_STA_STATE_INITIAL = 			0x0001,
	RT_STA_STATE_DISASOC =			0x0020,
	RT_STA_STATE_AP_INCOMING_ASOC_STARTED = 0X0100,
	RT_STA_STATE_AP_INCOMING_ASOC_REQ_RECVD = 0X200,
	RT_STA_STATE_AP_INCOMING_ASOC_COMPLETE = 0X400
} RT_STA_INDICATE_STATE, *PRT_STA_INDICATE_STATE;

typedef struct _RT_STA_INDICATE_STATE_MACHINE
{
	RT_STA_INDICATE_STATE	CurrentState;
	u4Byte					NextState;
} RT_STA_INDICATE_STATE_MACHINE, *PRT_STA_INDICATE_STATE_MACHINE;


typedef struct _RT_TX_LOCAL_BUFFER{
	RT_LIST_ENTRY		List;
	SHARED_MEMORY		Buffer;
	u1Byte				RefCount;
}RT_TX_LOCAL_BUFFER, *PRT_TX_LOCAL_BUFFER;

typedef struct _RT_TX_AGG_COALES_BUF{
	PRT_TX_LOCAL_BUFFER	pCoalesceBuffer;
	PRT_TCB 			pLastTcb;
	u4Byte				Offset;
	u4Byte				Length;
	u1Byte				AggCnt;
}RT_TX_AGG_COALES_BUF, *PRT_TX_AGG_COALES_BUF;


typedef struct _RT_GEN_TEMP_BUFFER{
	RT_LIST_ENTRY		List;
	VIRTUAL_MEMORY		Buffer;
	BOOLEAN				isDynaAlloc;	// Dynamical Allocated or Static Allocated
}RT_GEN_TEMP_BUFFER, *PRT_GEN_TEMP_BUFFER;

typedef enum _TWO_PORT_STATUS
{
	TWO_PORT_STATUS__DEFAULT_ONLY,				// Win7, XP				
	TWO_PORT_STATUS__EXTENSION_ONLY,			// Win7, XP(AP)							
	TWO_PORT_STATUS__EXTENSION_FOLLOW_DEFAULT,	// Win7, XP				
	TWO_PORT_STATUS__DEFAULT_G_EXTENSION_N20,	// Win7, XP
	TWO_PORT_STATUS__DEFAULT_A_EXTENSION_N20,	// Win7, XP
	TWO_PORT_STATUS__ADHOC,						// Win7, XP				
	TWO_PORT_STATUS__WITHOUT_ANY_ASSOCIATE		// Win7, XP				
}TWO_PORT_STATUS;

typedef enum _TWO_PORT_SHARED_OBJECT
{
	TWO_PORT_SHARED_OBJECT__STATUS,//
	TWO_PORT_SHARED_OBJECT__SET_OF_pStaQos,
	TWO_PORT_SHARED_OBJECT__SET_OF_pStaQos_WMMParamEle,	//
	TWO_PORT_SHARED_OBJECT__BW, //
}TWO_PORT_SHARED_OBJECT;

enum _BWSGNL{
	BWSGNL_Auto = 0,
	BWSGNL_Enable = 1,
	BWSGNL_Disable = 2,
};

enum _TX_IPTYPE{
	TX_IPTYPE_UNKNOWN = 0,
	TX_IPTYPE_UDP = 1,
	TX_IPTYPE_TCP = 2,
};


typedef struct _RT_RFD_STATUS{
	//
	// The section must be the same as _ODM_Phy_Status_Info_ If someone add one
	// element in ODM structure you need to update the element in the RFD status 
	// structure and in the end of the relative area.
	//
	// _ODM_Phy_Status_Info_ Start
	//
	u4Byte			RxPWDBAll;	
	u1Byte			SignalQuality;	 		// in 0-100 index. 
	s1Byte			RxMIMOSignalQuality[4];	//per-path's EVM
	u1Byte			RxMIMOEVMdbm[4]; 		//per-path's EVM dbm
	u1Byte			RxMIMOSignalStrength[4];// in 0~100 index

	s2Byte			Cfo_short[4]; 			// per-path's Cfo_short
	s2Byte			Cfo_tail[4];				// per-path's Cfo_tail

	s1Byte			RxPower; 				// in dBm Translate from PWdB
	s1Byte			RecvSignalPower;		// Real power in dBm for this packet, no beautification and aggregation. Keep this raw info to be used for the other procedures.
	u1Byte			BTRxRSSIPercentage;	
	u1Byte			SignalStrength; 			// in 0-100 index.

	s1Byte			RxPwr[4];				//per-path's pwdb
	s1Byte			RxSNR[4];				//per-path's SNR	
#if ((RTL8822B_SUPPORT == 1) || (RTL8723D_SUPPORT == 1))
	u1Byte			RxCount:2;					/* RX path counter---*/
	u1Byte			BandWidth:2;
	u1Byte			rxsc:4;						/* sub-channel---*/
#else
	u1Byte			BandWidth;
#endif
	u1Byte			btCoexPwrAdjust;		// adjust rssi for bt coex(agc table on/1ant coex type, need to add the power offset back)
#if ((RTL8822B_SUPPORT == 1) || (RTL8723D_SUPPORT == 1))
	u1Byte			channel;						/* channel number---*/
	BOOLEAN			bMuPacket;					/* is MU packet or not---*/
	BOOLEAN			bBeamformed;				/* BF packet---*/
#endif
	//
	//	_ODM_Phy_Status_Info_ End
	//

	//
	// The section must be the same as _ODM_Per_Pkt_Info_ If someone add one
	// element in ODM structure you need to update the element in the RFD status 
	// structure and in the end of the relative area.
	//
	//	_ODM_Per_Pkt_Info_ Start
	//	
	u1Byte			RawData;		// Accordign Luke, ODM want to use raw data.
	u1Byte			StationID;
	BOOLEAN			bPacketMatchBSSID;
	BOOLEAN			bPacketToSelf;
	BOOLEAN			bPacketBeacon;
	BOOLEAN			bToSelf;
	//
	//	_ODM_Per_Pkt_Info_ End
	//

	u2Byte				Length;

	u1Byte				DataRate;
	BOOLEAN				bIsCCK;
	
	u2Byte				bHwError:1;
	u2Byte				bCRC:1;
	u2Byte				bICV:1;
	u2Byte				bShortPreamble:1;
	u2Byte				bShift:1;
	u2Byte				Decrypted:1;
	u2Byte				Wakeup:1;
	u2Byte				bSTBC:1;
	u2Byte				bLDPC:1;
	u2Byte				bIsAMPDU:1;
	u2Byte				bFirstMPDU:1;
	u2Byte				bContainHTC:1;
	u2Byte				bRxMFPPacket:1;
	u2Byte				bIndicateToIhv:1;		// Mark if this packet should be indicated to IHV service.
	u2Byte				Reserved0:1;
	u1Byte				AGC;
	u1Byte				DataRateIdx;			// The rate index in the Rx Desc
	u4Byte				TimeStampLow;
	u4Byte				TimeStampHigh;
	BOOLEAN				bIsQosData;		// Added by Annie, 2005-12-22.
	u1Byte				UserPriority;

	u1Byte				RxDrvInfoSize;
	u1Byte				RxBufShift;
	u1Byte				PacketReportType;	
	
	u2Byte				Seq_Num;
	PRX_TS_RECORD		pRxTS;
	
	u4Byte				MacID;
	u1Byte				WakeMatch;
	
	u4Byte				MacIDValidEntry[2];	// 64 bits present 64 entry.
	BOOLEAN				bForward;

	//
	// PLCP in phy status [32:7]
	//
	u1Byte				Vht_Group_Id;
	u2Byte				Vht_Nsts_Aid;
	u1Byte				Vht_Txop_Not_Allow:1;
	u1Byte				Vht_Nsym_Dis:1;
	u1Byte				Vht_Ldpc_Extra:1;
	u1Byte				Vht_Su_Mcs:4;
	u1Byte				Vht_Beamformed:1;
	//
	// End of PLCP phy status
	//

}RT_RFD_STATUS,*PRT_RFD_STATUS;


typedef struct _RT_TCB_STATUS{
	u2Byte	DataRetryCount:8;
	u2Byte	TOK:1;
	u4Byte	FrameLength:12;
}RT_TCB_STATUS,*PRT_TCB_STATUS;

typedef struct _RT_TCB{
	RT_LIST_ENTRY		List;
	u4Byte				tcbFlags;
	PADAPTER			pAdapter;
	PADAPTER			SourceAdapt;
	//2 Packet content
	RT_TCB_BUFFER_TYPE	BufferType;			// Tell you how to return this packet

	//2 Buffer description
	u4Byte				PacketLength;
	u2Byte				BufferCount;
	SHARED_MEMORY		BufferList[MAX_PER_PACKET_BUFFER_LIST_LENGTH];
	//2 Header and tail buffer
	SHARED_MEMORY		LLC;
	SHARED_MEMORY		Header[MAX_FRAGMENT_COUNT];
	SHARED_MEMORY		Tailer;				// Per MSDU trailer. If it is used, its address should be appended to buffer list.
	SHARED_MEMORY		FirewareInfo; 		        //Pass Transmission configuration for each packet to Firmware
	SHARED_MEMORY		AMSDU_SubHeader;	// A-MSDU subframe headers
	SHARED_MEMORY		Buffer;				// Shared memory used for headers and trailer
	//2 Information for returning packet
	PVOID				Reserved;			// This variable depends on BufferType
	PVOID				ReservedBack;			// This variable for Indicate to OS later
	BOOLEAN				bTxCompleteLater;		// This variable for Indicate to OS later

	//2 Tx Desc information
	u1Byte				nDescUsed;
	RT_TCB_STATUS		status;

	//2 Coalesce buffer(dynamic allocate)
	PRT_TX_LOCAL_BUFFER	pCoalesceBuffer;
	//2 Fragment information
	u2Byte				FragCount;
	u2Byte				FragLength[MAX_FRAGMENT_COUNT];
	u2Byte				FragBufCount[MAX_FRAGMENT_COUNT];
	u1Byte				nFragSent; // Number of fragment sent to lower layer. 2005.04.14, by rcnjko.
	u1Byte				nFragCompleted; // Number of fragment completed from lower layer. 070423, by rcnjko.

	//add by ylb for ExtTxBD
	BOOLEAN				isUseExtTxBDBlock;
	RT_LIST_ENTRY		ExtTxBDBlockUsedQueue;		

	//2004/07/22, kcwu
	STA_ENC_INFO_T		EncInfo;

	// CCX related.
	u4Byte				CcxIappPacketType;
	u2Byte				CcxLtFrameNumber;

	u1Byte			nStuckCount;

	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//1//1Attention Please!!!<11n or 8190 specific code should be put below this line>
	//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
	u2Byte			TxDescDuration[MAX_FRAGMENT_COUNT];
	u2Byte			DurationFieldVal[MAX_FRAGMENT_COUNT];


#if TX_AGGREGATION
	BOOLEAN			outPending;		// for fill tx descriptor and don't bulk out
	BOOLEAN			isUsbTxAgg;
	BOOLEAN			isLastAgg;
	RT_LIST_ENTRY	aggListHead;
	u1Byte			aggNum;
	u4Byte			aggOffset;
	u2Byte			aggRequiredTxPageNum; 	// Toal Tx page number of one aggregation packet.
#endif

	//2 Joseph: This can be List instead.
	PRT_TCB				AggregatedTCB[AMSDU_MAX_NUM];
	u1Byte				AggregateNum;


	//guangan add it for sending initial cmd packet on usb interface
	BOOLEAN			bLastInitPacket;

	// Joseph 20090617: Fix TP low due to DHCP issue.
	u1Byte			specialDataType;
	u1Byte			IPType;

	// Identify FW image and H2C CMD, added by Roger, 2009.11.11.
	u1Byte		 	DescPktType;

	//Added for 92D early mode
	BOOLEAN		bEnableEarlyMode;
	BOOLEAN		bInsert8BytesForEarlyMode;
	u1Byte		EMPktNum;
	u4Byte		EMPktLen[EARLY_MODE_MAX_PKT_NUM];//The max value by HW

	//Partial Coalesce for 8192DE & 8812
	u1Byte				ColoaseMethod;
	SHARED_MEMORY		BufferListBack[MAX_PER_PACKET_BUFFER_LIST_LENGTH];
	u2Byte				ColoaseBufferCount;
	u2Byte				BufferBackCount;
	u1Byte				SubHdrIndexAry[AMSDU_MAX_NUM];

	// TxFeedback Tcb Context Place Holder for Private Data: -----------------------------
	//	+ Only access this private data inside the module by TxFeedbackGetTcbZone()	
	PRIVATE_DATA_ZONE 	TxFeedbackTcbZone[TX_FEEDBACK_SIZE_OF_TCB_ZONE];
	// ----------------------------------------------------------------------------

//1 The section is used for shortcut entry. 
/*******************************************************************************************
Sinda 2011/12/09
1. All of these variable used by RT_TCB and TXSC_TCB_ENTRY.
2. If need to add new one which do not used by TXSC_TCB_ENTRY, do not add it here.Otherwise, add it here.
3. the sequence of these variables must be the same with structure of TXSC_TCB_ENTRY.
4. Please keep the first element is "ProtocolType" and the last element is "MACHeardLen"
*******************************************************************************************/

	PROTOCOL_TYPE		ProtocolType;		// If it is RT_PROTOCOL_802_11, its content will not be changed
	u2Byte				DataRate;			// In 0.5 Mbps
	u1Byte				SpecifiedQueueID;	// Only for management queue. Otherwise set it to UNSPECIFIED_QUEUE_ID
	u1Byte				priority;
	u1Byte				TSID;				// Traffic Stream ID. WMM: 0-7, CAC: 8/9, 11e: 8-15. 070614, by rcnjko.
	u1Byte				DestinationAddress[ETHERNET_ADDRESS_LENGTH];
	u1Byte				SourceAddress[ETHERNET_ADDRESS_LENGTH];

	//3 TX Setting
	u1Byte			bLDPC:1;
	u1Byte			bSTBC:1;
	u1Byte			bEncrypt:1;
	u1Byte			bUseShortPreamble:1; 
	u1Byte			bUseShortGI:1;	//This Match "SHORT" field in TxFwInfo in datasheet.
	u1Byte			bSpecifShortGI:1;
	u1Byte			bTxDisableRateFallBack:1;
	u1Byte			bTxUseDriverAssingedRate:1;
	u1Byte			bBroadcast:1;
	u1Byte			bMulticast:1; 
	u1Byte			bBTTxPacket:1;
	u1Byte			bFromUpperLayer:1;

	u1Byte			bNeedAckReply:1;
	u1Byte			bNeedCRCAppend:1;	
	u1Byte			bTxEnableSwCalcDur:1;
	u1Byte			bTxCalculatedSwDur:1;

	//3 Protection Mode
	u1Byte			RTSSC;
	u1Byte			RTSCCA;
	u1Byte			RTSBW;
	u2Byte			RTSRate; 
	u1Byte			bHwProtection:1;
	u1Byte			bCTSEnable:1;
	u1Byte			bRTSEnable:1;
	u1Byte			bRTSSTBC:1;
	u1Byte			bRTSShort:1;
	u1Byte			bRTSReserved:3;

	//3 AMSDU and AMPDU related
	u1Byte			bAggregate:1;
	u1Byte			bAggregateBreakForBufferCount:1;//add by ylb for test 201300802
	u1Byte			bAMSDUProcessed:1;
	u1Byte			bAMPDUEnable:1;
	u1Byte			bAMPDUReserved:4;
	u1Byte			AMPDUFactor;
	u1Byte			AMPDUDensity;
	u2Byte			AMPDUBufferSize;

	// Driver assign macId for rate adaptive table
	u1Byte			macId;
	u1Byte			OdmAid;		// For ODM Luke's requirement for antenna switch.
	u2Byte			P_AID;
	u1Byte			G_ID;	
	u1Byte			RATRIndex;

	// BT
	u1Byte			BT_macId;
	
	BOOLEAN			bTDLPacket;
	BOOLEAN			bNeedTxReport;
	BOOLEAN			bSendPTIRsp;
	BOOLEAN			bSleepPkt;

	// Add for TCpChecksum
	u4Byte			TcpOffloadMode;	
	u4Byte			MACHeardLen;
//1 The section is used for shortcut entry. 

	s1Byte			SNForShortcut;

	CHANNEL_WIDTH	BWOfPacket;
	
	u2Byte			CurrentWritePoint;

	u8Byte			sysTime[4];

#if WLAN_ETW_SUPPORT
	u2Byte			TcbSequence;
	u4Byte			TcbFrameUniqueueID;
#endif

	// Beamforming packet
	u1Byte			TxBFPktType;

}RT_TCB,*PRT_TCB;

typedef struct _RX_AGGR_INFO{
	u1Byte	bIsRxAggr:1;
	u1Byte	bIsLastPkt:1;
	u1Byte	Reserved:6;
} RX_AGGR_INFO, *PRX_AGGR_INFO;

typedef enum _RX_IPTYPE{
	TCP_PACKET		= 0,
	UDP_PACKET		= 1,
} RX_IPTYPE, *PRX_IPTYPE;

typedef enum _RX_IPVER{
	IPV4		= 0,
	IPV6		= 1,
} RX_IPVER, *PRX_IPVER;

typedef enum _RX_MAC_PACKET_TYPE { 
  	RXPacketTypeUndefined  	= 0,
  	RXPacketTypeUnicast    	= 1,
  	RXPacketTypeMulticast  	= 2,
  	RXPacketTypeBroadcast  	= 3,
  	RXPacketTypeMaximum    	= 4
} RX_MAC_PACKET_TYPE, *PRX_MAC_PACKET_TYPE;

typedef struct _RX_FILTER_INFO{
	// MAC
	pu1Byte		pDA;
	pu1Byte		pSA;
	u1Byte		PacketType;  // RX_PACKET_TYPE
	pu1Byte		pEtherType;  // 2 bytes : IPv4 , IPv6 , ARP ...  
	// IP
	pu1Byte		ARPOption;  // 2 bytes Request or Response 
	pu1Byte		ARPSPA;      // 4 bytes ARP Sender IP 
	pu1Byte		ARPTPA;      // 4 bytes ARP Target IP 
	u1Byte		Protocol;      // UDP : 0x11  , TCP : 0x06
	//UDP
	pu1Byte		pDestinationPort; // 2 bytes
}RX_FILTER_INFO,*PRX_FILTER_INFO;

typedef struct _RT_RFD{	
	RT_LIST_ENTRY			List;	
	u2Byte				PacketLength;			// Total packet length: Must equal to sum of all FragLength
	u2Byte				FragLength;			// FragLength should equal to PacketLength in non-fragment case
	u2Byte				FragOffset;			// Data offset for this fragment
	u2Byte				nTotalFrag;
	pu1Byte				SubframeArray[MAX_SUBFRAME_COUNT];
	u2Byte				SubframeLenArray[MAX_SUBFRAME_COUNT];
	u1Byte				nTotalSubframe;
	BOOLEAN				bIsAggregateFrame;
	RT_RFD_STATUS		Status;
	ALIGNED_SHARED_MEMORY  Buffer;			//SHARED_MEMORY		Buffer;
	PVOID				DriverReserved;
	// 2007/01/12 MH add a element to record RX queue ID for test
	u2Byte				queue_id;
	struct _RT_RFD			*NextRfd;	

	// If this flag is set, this RFD must contain bad data; therefore, insert into its own RFD queue
	BOOLEAN				bReturnDirectly;

	BOOLEAN				bIsTemp;
#if RX_AGGREGATION
	RX_AGGR_INFO		RxAggrInfo;
	u1Byte				nChildCnt;
	struct _RT_RFD			*ParentRfd;
#endif

	u2Byte				SeqNum;
	u2Byte				ValidPacketLength;
	BOOLEAN				bRxBTdata;
	BOOLEAN				bTDLPacket;
	u1Byte				Address3[6];

	u1Byte				WapiTempPN[16];
	u1Byte				WapiSrcAddr[6];
	BOOLEAN				bWapiCheckPNInDecrypt;

	// TCP Offload 
	BOOLEAN				bHWChecksum;
	RX_IPVER				IPver;
	RX_IPTYPE			IPType;
	BOOLEAN				bCheckErr;

	// D0 Parse Packet Information
	RX_FILTER_INFO 		D0FilterCoalPktInfo;

	//ODM_PHY_INFO_T			PhyInfo;//must discuss with MH
#if (MULTIPORT_SUPPORT == 1)
	// NOTE: ----------------------------------------------------------------
	// This flag is used to decide if FeedPacketToMultipleAdapter() should be executed.
	BOOLEAN				bFeedPacketToSingleAdapter;
	// ----------------------------------------------------------------------

	// Data Buffer --------------------------------
	MEMORY_BUFFER		mbCloneRfdDataBuffer;
	// ------------------------------------------
	
#endif


	u4Byte				SdioTransferLength;

#if WLAN_ETW_SUPPORT
	u4Byte			RfdFrameUniqueueID;
#endif

}RT_RFD,*PRT_RFD;

typedef struct _DEFRAG_ENTRY
{
	BOOLEAN				bUsed;
	u1Byte				SenderAddr[6];
	u1Byte				TID;
	u2Byte				SeqNum;
	u1Byte				LastFragNum;
	u8Byte				usMaxLifeTimeStamp;
	u8Byte				usLastArriveTimeStamp;
	PRT_RFD				pRfdHead;
	PRT_RFD				pRfdTail;

}DEFRAG_ENTRY, *PDEFRAG_ENTRY;

typedef struct _RX_REORDER_ENTRY
{
	RT_LIST_ENTRY	List;
	u2Byte			SeqNum;
	PRT_RFD			pRfd;
} RX_REORDER_ENTRY, *PRX_REORDER_ENTRY;


typedef	struct _RT_9X_TX_RATE_HISTORY {
	u4Byte		CCK[4];
	u4Byte		OFDM[8];
	// HT_MCS[0][]: BW=0 SG=0
	// HT_MCS[1][]: BW=1 SG=0
	// HT_MCS[2][]: BW=0 SG=1
	// HT_MCS[3][]: BW=1 SG=1
	u4Byte		HT_MCS[4][16];
}RT_TX_RAHIS_T, *PRT_TX_RAHIS_T;

typedef	struct _RT_SMOOTH_DATA {
	u4Byte	elements[100];	//array to store values
	u4Byte	index;			//index to current array to store
	u4Byte	TotalNum;		//num of valid elements
	u4Byte	TotalVal;		//sum of valid elements
}RT_SMOOTH_DATA, *PRT_SMOOTH_DATA;

typedef	struct _RT_SMOOTH_DATA_4RF {
	s1Byte	elements[4][100];//array to store values
	u4Byte	index;			//index to current array to store
	u4Byte	TotalNum;		//num of valid elements
	u4Byte	TotalVal[4];		//sum of valid elements
}RT_SMOOTH_DATA_4RF, *PRT_SMOOTH_DATA_4RF;


typedef union _MGN_RATE{
	u2Byte	RawValue;

	struct {
		u1Byte	RateIdx;
		u1Byte	Rsvd:5;
		u1Byte	SGI:1;
		u1Byte	Bw:1;
		u1Byte	HT:1;
	};
} MGN_RATE;

typedef enum _RATE_INDEX{
	RATE_CCK_1M			= 0,
	RATE_CCK_2M			= 1,
	RATE_CCK_5_5M		= 2,
	RATE_CCK_11M		= 3,
	RATE_OFDM_6M		= 4,
	RATE_OFDM_9M		= 5,
	RATE_OFDM_12M		= 6,
	RATE_OFDM_18M		= 7,
	RATE_OFDM_24M		= 8,
	RATE_OFDM_36M		= 9,
	RATE_OFDM_48M		= 10,
	RATE_OFDM_54M		= 11,
	RATE_MCS_0			= 12,
	RATE_MCS_1			= 13,
	RATE_MCS_2			= 14,
	RATE_MCS_3			= 15,
	RATE_MCS_4			= 16,
	RATE_MCS_5			= 17,
	RATE_MCS_6			= 18,
	RATE_MCS_7			= 19,
	RATE_MCS_8			= 20,
	RATE_MCS_9			= 21,
	RATE_MCS_10			= 22,
	RATE_MCS_11			= 23,
	RATE_MCS_12			= 24,
	RATE_MCS_13			= 25,
	RATE_MCS_14			= 26,
	RATE_MCS_15			= 27,
	RATE_MCS_16			= 28,
	RATE_MCS_17			= 29,
	RATE_MCS_18			= 30,
	RATE_MCS_19			= 31,
	RATE_MCS_20			= 32,
	RATE_MCS_21			= 33,
	RATE_MCS_22			= 34,
	RATE_MCS_23			= 35,
	RATE_MCS_24			= 36,
	RATE_MCS_25			= 37,
	RATE_MCS_26			= 38,
	RATE_MCS_27			= 39,
	RATE_MCS_28			= 40,
	RATE_MCS_29			= 41,
	RATE_MCS_30			= 42,
	RATE_MCS_31			= 43,
	RATE_VHT_1SS_MCS_0	= 44,
	RATE_VHT_1SS_MCS_1	= 45,	
	RATE_VHT_1SS_MCS_2	= 46,
	RATE_VHT_1SS_MCS_3	= 47,
	RATE_VHT_1SS_MCS_4	= 48,
	RATE_VHT_1SS_MCS_5	= 49,
	RATE_VHT_1SS_MCS_6	= 50, 
	RATE_VHT_1SS_MCS_7	= 51,
	RATE_VHT_1SS_MCS_8	= 52,
	RATE_VHT_1SS_MCS_9	= 53,
	RATE_VHT_2SS_MCS_0	= 54,
	RATE_VHT_2SS_MCS_1	= 55,
	RATE_VHT_2SS_MCS_2	= 56,
	RATE_VHT_2SS_MCS_3	= 57,
	RATE_VHT_2SS_MCS_4	= 58,
	RATE_VHT_2SS_MCS_5	= 59,
	RATE_VHT_2SS_MCS_6	= 60,
	RATE_VHT_2SS_MCS_7	= 61,
	RATE_VHT_2SS_MCS_8	= 62,
	RATE_VHT_2SS_MCS_9	= 63,
	RATE_VHT_3SS_MCS_0	= 64,
	RATE_VHT_3SS_MCS_1	= 65,
	RATE_VHT_3SS_MCS_2	= 66,
	RATE_VHT_3SS_MCS_3	= 67,
	RATE_VHT_3SS_MCS_4	= 68,
	RATE_VHT_3SS_MCS_5	= 69,
	RATE_VHT_3SS_MCS_6	= 70,
	RATE_VHT_3SS_MCS_7	= 71,
	RATE_VHT_3SS_MCS_8	= 72,
	RATE_VHT_3SS_MCS_9	= 73,
	RATE_VHT_4SS_MCS_0	= 74,
	RATE_VHT_4SS_MCS_1	= 75,
	RATE_VHT_4SS_MCS_2	= 76,
	RATE_VHT_4SS_MCS_3	= 77,
	RATE_VHT_4SS_MCS_4	= 78,
	RATE_VHT_4SS_MCS_5	= 79,
	RATE_VHT_4SS_MCS_6	= 80,
	RATE_VHT_4SS_MCS_7	= 81,
	RATE_VHT_4SS_MCS_8	= 82,
	RATE_VHT_4SS_MCS_9	= 83,
	RATE_MCS_32			= 84,
	RATE_UNKNOWN		= 85,	
	PKT_MGNT			= 86,
	PKT_CTRL			= 87,
	PKT_UNKNOWN			= 88,
	PKT_NOT_FOR_ME		= 89,
	RATE_MAX
} RATE_INDEX, *PRATE_INDEX;

typedef	struct _RT_TX_STATISTICS {
	u8Byte		NumTxMulticast;
	u8Byte		NumTxBroadcast;
	u8Byte		NumTxUnicast;
	u8Byte		NumTxOkTotal;
	u8Byte		NumTxBytesUnicast;
	u8Byte		NumTxBytesBroadcast;
	u8Byte		NumTxBytesMulticast;
	u8Byte		NumTxOkBytesTotal;
	u8Byte		NumTxErrTotal;
	u8Byte		NumTxErrBytesTotal;
	u8Byte		NumTxErrUnicast;
	u8Byte		NumTxErrBroadcast;
	u8Byte		NumTxErrMulticast;
	u8Byte		NumTxRetryCount;
	u4Byte		NumTxBeaconOk;
	u4Byte		NumTxBeaconErr;
	u4Byte		NumTxBeaconUpdate;
	//cosa add for debug 10/18/2007
	u4Byte		NumTxInterrupt;
	u4Byte		NumTxSemaphore;
	u4Byte		NumTxDescClosed[MAX_TX_QUEUE];
	u4Byte		NumTxDescFill[MAX_TX_QUEUE];
	//cosa add for debug 08/23/2007
	u4Byte		NumMpSendPacketsCalled;
	u4Byte		NumOutOfTCB;
	u4Byte		numTcbOk;
	u4Byte		NumTransmitTCBCalled[MAX_TX_QUEUE];
	u4Byte		NumTransmitTCBAvailable[MAX_TX_QUEUE];
	u4Byte		NumTransmitTCBNotAvailable[MAX_TX_QUEUE];
	u4Byte		NumTransmitTCBInsertTcbBusyQueue[MAX_TX_QUEUE];
	// for debug
	u4Byte		NumTxFeedBack;
	u4Byte		NumTxFeedBackRetry;
	u4Byte		NumTxFeedBackOk;
	u4Byte		NumTxFeedBackFail;

	u4Byte		TransmittedRateHistogram[RATE_MAX];	
	u4Byte		AllTransmittedRateHistogram[RATE_MAX];	
	u4Byte		TransmittedRateRetry[RATE_MAX];	
	u4Byte		AllTransmittedRateRetry[RATE_MAX];

	u4Byte		TxAMSDUSizeHistogram[5];		// level: (<1k), (1k~2k), (2k~4k), (4k~6k), (6K~8K)
	u4Byte		TxAMSDUNumHistogram[5];		// level: (<3), (3~5), (6~9), (10~19), (>20)
	
	// For AMSDU efficiency, added by Emily, 2007.10.01
	u4Byte		AMSDUTxBufferNotAvaliable;
	u4Byte		AMSDUTxDescNotAvaliable;

	u1Byte		LastPacketRate;	//cosa add for debug 11/19/2007

	// 2008/03/35 MH Collect all transmit rate history for 9x series.
	RT_TX_RAHIS_T	TxRate;		

	//for show Tx DataRate , by Jacken 2008/03/28
	u1Byte		CurrentInitTxRate;
	u4Byte		CurrentTxTP;
	u4Byte		CurrentTxTP_Kbps;

	//Added for remember last tx ok packet for every adapter 20130116
	//by sherry
	u4Byte		LastNumTxOkBytesTotal; 
	u8Byte		StartNumTxOkBytesTotal;
}RT_TX_STATISTICS, *PRT_TX_STATISTICS;

typedef struct _TX_GENERAL{
	RT_THREAD			txGenThread;
	PlatformSemaphore	txGenSemaphore;
	

}TX_GENERAL, *PTX_GENERAL;


typedef struct _RT_RX_STATISTICS {
	u8Byte		NumRxBroadcast;
	u8Byte		NumRxUnicast;
	u8Byte		NumRxMulticast;
	u8Byte		NumRxOkTotal;
	u8Byte		NumRxErrTotalUnicast;
	u8Byte		NumRxErrTotalMulticast;
	u8Byte		NumRxBytesUnicast;
	u8Byte		NumRxBytesMulticast;
	u8Byte		NumRxBytesBroadcast;
	u8Byte		NumRxOkBytesTotal;
	u8Byte		NumRxRetryCount;
	u8Byte		NumRxOKSmallPacket;
	u8Byte		NumRxOKMiddlePacket;
	u8Byte		NumRxOKLargePacket;
	u8Byte		NumRxRetrySmallPacket;
	u8Byte		NumRxRetryMiddlePacket;
	u8Byte		NumRxRetryLargePacket;
	u8Byte		NumRxCrcErrSmallPacket;
	u8Byte		NumRxCrxErrorPacket;
	u8Byte		NumRxCrcErrMiddlePacket;
	u8Byte		NumRxCrcErrLargePacket;
	u8Byte		NumRxIcvErr;
	u8Byte		NumRxExcludeUnencryptedUnicast;
	u8Byte		NumRxExcludeUnencryptedMulticast;
	u8Byte		NumRxExcludeUnencryptedBroadcast;
	u8Byte		NumRxTKIPLocalMICFailuresUnicast;
	u8Byte		NumRxTKIPLocalMICFailuresMulticast;
	u8Byte		NumRxTKIPLocalMICFailuresBroadcast;
	u8Byte		NumRxTKIPICVErrorUnicast;
	u8Byte		NumRxTKIPICVErrorMulticast;
	u8Byte		NumRxTKIPICVErrorBroadcast;
	u8Byte		NumRxCCMPDecryptErrorsUnicast;
	u8Byte		NumRxCCMPDecryptErrorsMulticast;
	u8Byte		NumRxCCMPDecryptErrorsBroadcast;
	u8Byte		NumRxWEPICVErrorUnicast;
	u8Byte		NumRxWEPICVErrorMulticast;
	u8Byte		NumRxWEPICVErrorBroadcast;
	u8Byte		NumRxWEPUndecryptableUnicast;	
	u8Byte		NumRxWEPUndecryptableMulticast;
	u8Byte		NumRxWEPUndecryptableBroadcast;
	u8Byte		NumRxTKIPReplayMulticast;
	u8Byte		NumRxTKIPReplayUnicast;
	u8Byte		NumRxCCMPReplayMulticast;
	u8Byte		NumRxCCMPReplayUnicast;
	u8Byte		NumRxDecryptSuccessUnicast;		//fake, for Ndistest
	u8Byte		NumRxDecryptSuccessMulticast;		//fake, for Ndistest
	u8Byte		NumRxDecryptSuccessBroadcast;		//fake, for Ndistest	
	u8Byte		NumRxDecryptFailureUnicast;		//fake, for Ndistest
	u8Byte		NumRxDecryptFailureMulticast;		//fake, for Ndistest
	u8Byte		NumRxDecryptFailureBroadcast;		//fake, for Ndistest
	u8Byte		NumRxMgntTKIPICVError;			// CCXv5, S67 MFP TKIP
	u8Byte		NumRxMgntTKIPNoEncrypt;			// CCXv5, S67 MFP TKIP
	u8Byte		NumRxMgntTKIPMICFailures;		// CCXv5, S67 MFP TKIP
	u8Byte		NumRxMgntTKIPMHDRError;			// CCXv5, S67 MFP TKIP
	u8Byte		NumRxMgntTKIPReplay;			// CCXv5, S67 MFP TKIP
	u8Byte		NumRxMgntCCMPDecryptError;		// CCXv5, S67 MFP AES
	u8Byte		NumRxMgntCCMPNoEncrypt;			// CCXv5, S67 MFP AES
	u8Byte		NumRxMgntCCMPReplay;			// CCXv5, S67 MFP AES
	u8Byte		NumRxMgntDisassocBroadcast;		// CCXv5, S67 MFP
	u8Byte		NumRxMgntDeauthBroadcast; 		// CCXv5, S67 MFP
	u8Byte		NumRxMgntActionBroadcast;		// CCXv5, S67 MFP
	u8Byte		NumRxReceivedFrameCount;
	u8Byte		NumRxCmdPkt[4];		//07/03/09 MH rx cmd element txfeedback/bcn report/cfg set/query
	u8Byte		NumRxMgntPacket;	//Add by Maddest for DTM 1.0c 070823
	s4Byte		SignalStrength; // Transformed, in dbm. Beautified signal strength for UI, not correct.
	s4Byte		RecvSignalPower;	// Correct smoothed ss in Dbm, only used in driver to report real power now.
	u4Byte		RssiOldCalculateCnt;	// counter for Old RSSI calculation for Antenna lock test
	u4Byte		RssiCalculateCnt;	// counter for RSSI calculation for Antenna lock test
	u4Byte		PWDBAllCnt;
	u4Byte		PWDBAllOldCnt;
	

	RT_SMOOTH_DATA 		ui_rssi;
	RT_SMOOTH_DATA 		ui_link_quality;

	s4Byte		LastSignalQualityInPercent; // In percentange, used for smoothing, e.g. Moving Average.
	u1Byte		LastLinkQuality;
	s4Byte		SignalQuality; // Transformed.
	u1Byte		QueryAfterFirstReset;		//20061208 for ndistest by David. 
	u8Byte		NumRxFramgment;
	u4Byte		ReceivedRateHistogram[RATE_MAX];	//2013/05/16 revised by cosa
	u4Byte		ReceivedRateHwErr[RATE_MAX];		//2013/08/14 cosa added for bt coex
	BOOLEAN		RxRateHistory;
		
	// used for bt coex, cosa add 2013/08/15
	u4Byte		ReceivedRateHistogramBtCoex[RATE_MAX];
	u4Byte		ReceivedRateHwErrBtCoex[RATE_MAX];
	
	u4Byte		RxAMPDUSizeHistogram[5]; // level: (<4K), (4K~8K), (8K~16K), (16K~32K), (32K~64K)
	u4Byte		RxAMPDUNumHistogram[5]; // level: (<5), (5~10), (10~20), (20~40), (>40)
	u4Byte		RxAMSDUSizeHistogram[5];		// level: (<1k), (1k~2k), (2k~4k), (4k~6k), (6K~8K)
	u4Byte		RxAMSDUNumHistogram[5];		// level: (<3), (3~5), (6~9), (10~19), (>20)
	// 2012/11/07 MH Add for BB team VHT debug
	u4Byte		PacketAbilityCnt[4];	// 0/1/2/3=

	s4Byte		RxSNRdB[4];
	u1Byte		RxRSSIPercentage[4];
	
	u1Byte		RxEVMdbm[4]; 		
	u1Byte		RxEVMPercentage[2];

	s2Byte		RxCfoShort[4];
	s2Byte		RxCfoTail[4];

	u4Byte		NumRx20MHzPacket;
	u4Byte		NumRx40MHzPacket;
	//10/18/2007 cosa add for rx statistics
	u4Byte		NumRxInterrupt;
	u4Byte		NumRxDescFilled[MAX_RX_QUEUE];
	u4Byte		NumRxDescReturn[MAX_RX_QUEUE];
	
	u8Byte		NumQryPhyStatus;		// debug use only.
	u8Byte		NumQryPhyStatusCCK;	// debug use only.
	u8Byte		NumQryPhyStatusHT;		// debug use only.
	u8Byte		NumPacketMatchBSSID;	// debug use only.
	u8Byte		NumPacketToSelf;		// debug use only.
	u8Byte		NumProcessPhyInfo;		// debug use only.
	u8Byte		NumNotCMPKRXQID;		// debug use only.
	u8Byte		NumCMPKRXQID;			// debug use only.

	//for show Rx DataRate , by Jacken 2008/03/12
	u4Byte		ReceivePacketDataRateCounter[77];
	MGN_RATE	CurRxDataRate;
	u4Byte		CurRxOfdmNum;
	u4Byte		CurRxCckNum;

	u4Byte		CurRxSniffMediaPktCnt;
	u4Byte		LastRxSniffMediaPktCnt;

	u4Byte		ReceivedRateFromAPHistogram[32];

	u4Byte		CurrentRxTP;
	u4Byte		CurrentRxTP_Kbps;
	
	//Added for remember last rx ok packet for every adapter 20130116
	//by sherry
	u4Byte		LastNumRxOkBytesTotal;
	u8Byte		StartNumRxOkBytesTotal;
}RT_RX_STATISTICS, *PRT_RX_STATISTICS;

//----------------------------------------------------------------------------
// For 8186 Realtek element parsing. Added by Roger, 2006.12.07.
//
typedef	struct _RT_TURBO_MODE_IE_VALUE{
	u1Byte		Type;
	u1Byte		Length;
	u1Byte		Value;
}RT_TURBO_MODE_IE_VALUE, *PRT_TURBO_MODE_IE_VALUE;

//
// For 8186 Realtek element parsing. Added by Roger, 2006.12.07.
//
typedef	enum _RT_IE_TYPES
{
	eRT_IE_TYPE1_TURBO_MODE = 1,
	eRT_IE_TYPE2_AGGREGATION_MODE =2,
	eRT_IE_TYPE_MAX
}RT_IE_TYPES, *PRT_IE_TYPES;


//
// For 8186 Realtek element parsing.
//
#define GET_RTIE_CAPABILITY_TURBO_MODE(_pStart)			((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pStart)), 0, 1))
#define SET_RTIE_CAPABILITY_TURBO_MODE(_pStart, _value)		SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pStart)), 0, 1, (u1Byte)(_value))

#define GET_RTIE_CAPABILITY_DISABLE_TURBO(_pStart)		((u1Byte)LE_BITS_TO_2BYTE( ((pu1Byte)(_pStart)), 1, 1))
#define SET_RTIE_CAPABILITY_DISABLE_TURBO(_pStart, _value)	SET_BITS_TO_LE_2BYTE( ((pu1Byte)(_pStart)), 1, 1, (u1Byte)(_value))

//----------------------------------------------------------------------------


typedef struct _RT_LINK_DETECT_T{

	u4Byte				NumRecvBcnInPeriod;
	u4Byte				NumRecvDataInPeriod;

	BOOLEAN 			bClientAskForFCS; //When Client is asking for FCS, GO shall skip scanning. 
	u4Byte				ClientAskForFCSNum;	// number of Client asking for FCS
	u4Byte				RxNullDataNum;	// number of Rx Null Data

	u4Byte				RxBcnNum[RT_MAX_LD_SLOT_NUM];	// number of Rx beacon / CheckForHang_period  to determine link status
	u4Byte				RxDataNum[RT_MAX_LD_SLOT_NUM];	// number of Rx data / CheckForHang_period  to determine link status
	u2Byte				SlotNum;	// number of CheckForHang period to determine link status
	u2Byte				SlotIndex;

	u4Byte				NumTxOkInPeriod;
	u4Byte				NumRxOkInPeriod;
	u4Byte				NumRxUnicastOkInPeriod;
	u1Byte				NumRxUnicastThresh;
	u1Byte				NumRxOkThresh;
	u1Byte				NumRxUnicastTxOkThresh;

	BOOLEAN				bBusyTraffic;
	u4Byte				continuousIdleCnt;
	BOOLEAN				bTxBusyTraffic;
	BOOLEAN				bRxBusyTraffic;
	BOOLEAN				bHigherBusyTraffic; // For interrupt migration purpose.
	BOOLEAN				bHigherBusyRxTraffic; // We may disable Tx interrupt according as Rx traffic.
	BOOLEAN				bVeryHigherBusyTraffic; // For interrupt migration purpose.
	BOOLEAN				bVeryHigherBusyRxTraffic; // We may disable Tx interrupt according as Rx traffic.
	BOOLEAN				bVeryHigherBusyTxTraffic; // We may disable Tx interrupt according as Rx traffic.
	u4Byte				LastNumTxOkInPeriod;
	u4Byte				LastNumRxOkInPeriod;
	BOOLEAN				bStableHigherBusyTraffic;	//by tynli.

	BOOLEAN				bBusyTrafficAccordingTP;
	BOOLEAN				bTxBusyTrafficAccordingTP;
	BOOLEAN				bRxBusyTrafficAccordingTP;

	u1Byte 				LastConnectedCenterFrequency;
	CHANNEL_WIDTH 		LastConnectedBandwidth;	
	u4Byte				ConnectButNoBcnInPeriodCnt;			//debug only
	u4Byte				ConnectButNoDataInPeriodCnt;		//debug only
	u4Byte				OnBeaconCnt_OnBeacon;				//debug only
	u4Byte				OnBeaconScanCnt_OnBeacon_Scan;	//debug only
	u4Byte				OnBeaconJoinCnt_OnBeacon_Join;		//debug only
	u4Byte				OnBeaconBssCnt_OnBeacon_Bss;		//debug only
	u4Byte				OnBeaconIbssCnt_OnBeacon_Ibss;	//debug only
	u4Byte				OnDeauthCnt;						//debug only
	u4Byte				OnDisassocCnt;						//debug only
	u4Byte				BecomeDisconnectedCnt;				//debug only
	u4Byte				InfraDisconnectRoamingStartCnt;		//debug only
	u4Byte				InfraDisconnectRoamingFailCnt;		//debug only
	u4Byte				AdhocDisconnectCnt;					//debug only
	u4Byte				ConnectResumeFromRoaming;		//debug only
	u4Byte				IndicateDisconnectCnt;				//debug only
	u4Byte				BecomeDisconnectedIndex;			//debug only
	u8Byte				BecomeDisconnectedTime[10];		//debug only
}RT_LINK_DETECT_T, *PRT_LINK_DETECT_T;



typedef struct _IbssParms{
	u2Byte   atimWin;
}IbssParms, *PIbssParms;


//
// For AP mode force per-sta data rate (ASUS WiFi-Phone project), 2006.04.18, by rcnjko.
//
typedef struct _RT_STA_DATARATE{
	u1Byte			StaAddr[6];
	u2Byte			DataRate; // in unit of 0.5 Mbps
}RT_STA_DATARATE, *PRT_STA_DATARATE;


#define MAX_BSS_DESC   512	// 40=>64, 2005.03.31, by rcnjko.

//
// 2010/12/09 MH When merging the linux code from SC, we find that the XP linker pop
// up error information. OnBeacon_Join can not be refernced in mgntengine.c object. It is 
// caused by the linker stack limitation. We declare too much local variable in the function.
// To solve the problem temporarily we decrease the max BSS_IE_BUF_LEN to 1024. Because
// linux declare two buffer as 256+256. The right way is not to use so much local buffer in stack
// We need to merge 92S code to fix the problem.
//
//#define BSS_IE_BUF_LEN   2048

#define BSS_IE_BUF_LEN   1024
#define WCN_IE_BUF_LEN 256
#define MAX_WPA_IE_BUF_LEN 256
#define MAX_RSN_IE_BUF_LEN 256


#define MAX_RFD_ARRAY_NUM 2048

//
// Define the receiving packet type in scanning. By Bruce, 2008-05-23.
//
#define	BSS_PKT_BEACON		BIT1
#define	BSS_PKT_PROBE_RSP	BIT2


typedef struct _ChnlSwitchAnnouncement{
	BOOLEAN				bWithCSA;
	u1Byte				channelSwtichMode;
	u1Byte				NewChnlNum;
	u1Byte				channelSwitchCnt;	
}ChnlSwitchAnnouncement, *PChnlSwitchAnnouncement;


typedef struct _RT_WLAN_BSS{
	u1Byte			bdBssIdBuf[6];
	u1Byte			bdSsIdBuf[33];
	u1Byte			bdSsIdLen;
	u2Byte			bdBcnPer;		 // beacon period in Time Units
	u1Byte			bdDtimPer;		// DTIM period in beacon periods
	u8Byte			bdTstamp;		// 8 Octets from ProbeRsp/Beacon
	u2Byte			bdCap;			// capability information

	u1Byte			bdMacAddressBuf[6];

	u1Byte			bdSupportRateEXBuf[MAX_NUM_RATES];
	u2Byte			bdSupportRateEXLen;

	IbssParms		bdIbssParms;	// empty if infrastructure BSS
	u1Byte			ChannelNumber;
	u1Byte			RegulatoryClass;	// Regulatory Class from 0 to 255
	CHANNEL_WIDTH	bdBandWidth;

	u1Byte			RSSI; // 0-100.
	s4Byte			RecvSignalPower; // In dBm. (equal to last recv beacon directly)
	s4Byte			CumRecvSignalPower;	// In dBm. (Cumulate with weighted ratio and a small in/decrease)
	u1Byte			SignalQuality; // 0-100
	u1Byte			BssPacketType; // The packet type received when scanning.
	s4Byte			Noise; // In dBm.

	u1Byte			bdERPInfo;
	BOOLEAN			bERPInfoValid;

	WIRELESS_MODE	wirelessmode;

	// Our TSF when receving thisp packet.
	u4Byte			RecvTsfLow;
	u4Byte			RecvTsfHigh;
	
	u2Byte						SecVersion;

	RT_ENC_ALG					GroupCipherSuite;
	RT_ENC_ALG					PairwiseCipherSuite[MAX_CIPHER_SUITE_NUM];
	u2Byte						PairwiseCipherCount;
	AKM_SUITE_TYPE				AuthSuite[MAX_AUTH_SUITE_NUM];
	u2Byte						AuthSuiteCount;	

	u2Byte						PMKIDCount;
	RT_PMKID_TYPE				PMKIDList[1];
	BOOLEAN						bPreAuth;
	u1Byte						NumOfPTKReplayCounter;
	u1Byte						NumOfGTKReplayCounter;

	u1Byte						bMFPR;
	u1Byte						bMFPC;
	u1Byte						bMFPBIP;

	// Qos related. Added by Annie, 2005-11-01.
	BSS_QOS						BssQos;
	u1Byte						LastChnlUpdatecount;


	// For 8186 auto-turbo advertising. Added by Roger, 2006.12.07.
	BOOLEAN						bRealtekCapType1Exist;	// Added for 8186 auto-turbo advertising.
	u2Byte						RealtekCap;
	BOOLEAN						bRealtekAggCapExist;

	//2004/06/29, kcwu, for WPA
	u1Byte						IEBuf[BSS_IE_BUF_LEN];
	OCTET_STRING				IE;
	RT_SECURITY_LEVEL			SecLvl;
	
	u1Byte						WpaIeBuf[MAX_WPA_IE_BUF_LEN];
	OCTET_STRING	 			WpaIe;

	u1Byte						RsnIeBuf[MAX_RSN_IE_BUF_LEN];
	OCTET_STRING	 			RsnIe;

#if (WPS_SUPPORT == 1)
	u1Byte						BeaconWcnIeBuf[WCN_IE_BUF_LEN];
	OCTET_STRING				osBeaconWcnIe;

	u1Byte						ProbeRspWcnIeBuf[WCN_IE_BUF_LEN];
	OCTET_STRING	 			osProbeRspWcnIe;

	//
	//david add 20061002 simple config IE
	//
	u1Byte						bdSimpleConfIEBuf[MAX_SIMPLE_CONFIG_IE_LEN_V2];	// Note: we should not allocate too large buffer, or 98 will encounter BSOD. Annie, 2006-10-19.
	OCTET_STRING				bdSimpleConfIE;
#endif

	//
	// 802.11d Country IE
	//
	u1Byte						bdCountryIEBuf[MAX_IE_LEN];
	OCTET_STRING				bdCountryIE;

	//
	// WMM parameter IE 
	//
	OCTET_STRING				osWmmAcParaIE;
	u1Byte						WmmAcParaBuf[WMM_PARAM_ELEMENT_SIZE];	// Max WMM parameter size is 24 octets.

	//
	// Power Constraint IE
	//
	OCTET_STRING				osPowerConstraintIe;
	u1Byte						PowerConstraintBuf[MAX_DOT11_POWER_CONSTRAINT_IE_LEN];

	// HT Related, by Emily, 2006.08.14
	BSS_HT						BssHT;
	BSS_VHT						BssVHT;
	u1Byte					 	Vender;	
	u1Byte						SubTypeOfVender;
	u8Byte						HistoryTime;
	u1Byte						HistoryCount;	
		
	//
	// For Chiper check !! add by CCW 2008/0918
	//
	u1Byte						PairwiseChiper;
	u1Byte						GroupChiper;
	u4Byte						AKMsuit;


#if (P2P_SUPPORT == 1)
	u1Byte						P2PManagedInfo; // Wlan AP supports P2P managed device capability.
#endif

#if (DFS_SUPPORT == 1)
	//
	// For 802.11h, dfs, channel switch announcement add by cosa 2010/09/02
	//
	ChnlSwitchAnnouncement		CSA;
#endif

}RT_WLAN_BSS, *PRT_WLAN_BSS;

typedef struct _RT_WLAN_RSSI{
	u1Byte			bdBssIdBuf[6];
	u1Byte			bdSsIdBuf[33];
	u1Byte			bdSsIdLen;

	s4Byte			CumRecvSignalPower;	// In dBm. (Cumulate with weighted ratio and a small in/decrease)
	
}RT_WLAN_RSSI, *PRT_WLAN_RSSI;

#pragma pack(4)
typedef struct _RT_WLAN_BSS_Customized{
	u1Byte			SsIdBuf[33];
	u1Byte			SsIdLen;
	u1Byte			BssPacketType; // The packet type received when scanning.
	u1Byte			ChannelNumber;
	s4Byte			RecvSignalPower; // In dBm. (equal to last recv beacon directly)
	u8Byte			TimeStamp;		// 8 Octets from ProbeRsp/Beacon
	s4Byte			Noise; // In dBm.
	u1Byte			BssIdBuf[6];
	u1Byte			Reserved[2];
}RT_WLAN_BSS_Customized, *PRT_WLAN_BSS_Customized;
#pragma pack()

//
// 061122, rcnjko: 
// We SHALL use this macro to change RT_WLA_BSS's IE to 
// prevent malicious buffer overflow attack. 
//
#define CopyRtWlanBssIE(__pRtWlanBss, __pBuf, __BufLen) \
			(__pRtWlanBss)->IE.Octet = (__pRtWlanBss)->IEBuf; \
			(__pRtWlanBss)->IE.Length = ( ((__BufLen) > BSS_IE_BUF_LEN) ? BSS_IE_BUF_LEN: (__BufLen) ); \
			CopyMem((__pRtWlanBss)->IE.Octet, (__pBuf), (__pRtWlanBss)->IE.Length)

typedef struct _SW_CAM_TABLE{
	
	u1Byte				macAddress[6];
	BOOLEAN			       bUsed;
	u1Byte				pucKey[PTK_LEN];
	u4Byte 				ulEncAlg;
	u4Byte				ulUseDK;
	u4Byte				ulKeyId;				// CAM entry in CAM
	u1Byte			        portNumber;			//vivi added for new cam search flow, 20091028...ID different port
	
}SW_CAM_TABLE,*PSW_CAM_TABLE;

typedef struct _RSSI_STA{
	s4Byte	UndecoratedSmoothedPWDB;
	s4Byte	UndecoratedSmoothedCCK;
	s4Byte					UndecoratedSmoothedOFDM;
	u8Byte					PacketMap;
	u4Byte					OFDM_pkt;
	u1Byte					ValidBit;
	s4Byte					RxRSSIPercentage[4];
	s4Byte					RSSI_CCK_Path[4];
	s4Byte					RSSI_CCK_Path_cnt[4];
	u4Byte					OFDM_Pkt_Cnt;
	u4Byte					CCK_Pkt_Cnt;
	
}RSSI_STA, *PRSSI_STA;

typedef enum	_RT_STA_EOSP_STATE
{
	RT_STA_EOSP_STATE_OPENED = 0,	// The SP is opend.
	RT_STA_EOSP_STATE_ENDING = 1, // The SP is ending.
	RT_STA_EOSP_STATE_ENDED = 2, // The SP is ended.
}RT_STA_EOSP_STATE, *PRT_STA_EOSP_STATE;

typedef struct _RT_WLAN_STA{
	BOOLEAN				bUsed;			// TRUE if this entry is used.
	BOOLEAN				bLocked;		// TRUE if this STA MAC Address is locked.
	u1Byte				Sum;			// Sum of MacAddr, used for hash purpose.
	u1Byte				MacAddr[6];		// Mac address of this STA.
	AUTH_ALGORITHM		AuthAlg;		// AuthAlg of the STA. 
	RT_AUTH_MODE		AuthMode;
	u1Byte				AuthPassSeq;	// 0: Not yet authenticated / 2: STA has receive an successful Auth2. / 4: STA has received an successful Auth4.
	BOOLEAN				bAssociated;	// TRUE if this STA is associated to this BSS.
	BOOLEAN				bDisassociated;
	u2Byte				AID;			// AID

	// HalMacID used for this peer ---------------------------
	u1Byte				AssociatedMacId;
	// --------------------------------------------------

	u1Byte				MultiPortStationIdx;
	
	u8Byte				LastActiveTime; // Last active time in ms. 
	u1Byte				WmmStaQosInfo;	// Qos Info field when sent from WMM STA in the (re-)association packet.
	BOOLEAN				bPowerSave;		// TRUE if the STA is in power save mode, FALSE is for active one.
	RT_LIST_ENTRY		PsQueue;		// Packet queued for this STA if it is in legacy power save mode.
	RT_LIST_ENTRY		WmmPsQueue;		// Packets belong to WMM APSD ACs queued.
	RT_STA_EOSP_STATE	WmmEosp;		// If true, the USP is end now for this STA.	

	AUTH_PKEY_MGNT_TAG	perSTAKeyInfo;	// WPA-PSK related variable

	u1Byte				bdSupportRateEXBuf[MAX_NUM_RATES]; // Rate supported by the STA.
	u2Byte				bdSupportRateEXLen; 	// Length of bdSupportRateEXBuf[].
	u1Byte				HighestOperaRate;		// Highest supported rate in bdSupportRateEXBuf.
	QOS_MODE			QosMode;				//STA QoS Mode

	WIRELESS_MODE		WirelessMode;			// Wireless mode of STA associated.
	CHANNEL_WIDTH		BandWidth;

	BOOLEAN				FillTIMFlags;
	BOOLEAN				FillTIM;

	u2Byte				DataRate;
	BOOLEAN				bUseShortGI;
	//add for HW enc ,By CCW
	u4Byte				keyindex;

	// add to Clear PS tcb 
	u1Byte				PSIdleCunt;


	RT_HTINFO_STA_ENTRY		HTInfo;
	RT_VHTINFO_STA_ENTRY	VHTInfo;
	u1Byte					ratr_index;
	u1Byte					Ratr_State;
	u4Byte					IOTAction;	
	u2Byte				IOTMaxMpduLen;


	// Win7
	pu1Byte				AP_RecvAsocReq;
	u4Byte				AP_RecvAsocReqLength;
	pu1Byte				AP_SendAsocResp;
	u4Byte				AP_SendAsocRespLength;
	
	pu1Byte				AP_AsocRspIEFromOS;
	u4Byte				AP_AsocRspIEFromOSLength;
	
	u2Byte				Capability;
	u2Byte				usListenInterval;
	u8Byte				AssociationUpTime;

	BOOLEAN 			bOsDecisionMade;
	BOOLEAN 			bNotAcceptedByOs; // Use "NotAccepted" instead of  "Accepted" because 0 is the default value we want after reset.
	u2Byte				OsReasonCode;
	
	RT_STA_INDICATE_STATE_MACHINE 	IndicationEngine;	

	RSSI_STA			rssi_stat;

	// For preventing duplicate packet indications ------
	u1Byte				LastRxUniFragNum;
	u2Byte				LastRxUniSeqNum;
	// ------------------------------------------
	
#if (P2P_SUPPORT == 1)
	BOOLEAN 			bP2PClient;
	P2P_CLIENT_INFO_DISCRIPTOR P2PClientInfoDesc;
#endif	//#if (P2P_SUPPORT == 1)

#if (WPS_SUPPORT == 1)
	OCTET_STRING		SimpleConfigInfo;
#endif


	//
	// ================ODM Relative Info=======================
	// Please be care, dont declare too much structure here. It will cost memory * STA support num.
	//
	//
	UINT8		IOTPeer;			// Enum value.	HT_IOT_PEER_E Every team define independent enum.

	// ODM Write
	//1 PHY_STATUS_INFO
	UINT8		RSSI_Path[4];		// 
	UINT8		RSSI_Ave;
	UINT8		RXEVM[4];
	UINT8		RXSNR[4];
	
}RT_WLAN_STA,*PRT_WLAN_STA;

typedef struct _RT_802_11_BSSID_LIST
{
	u4Byte			NumberOfItems;      // in list below, at least 1
	RT_WLAN_BSS	*pbssidentry;
} RT_802_11_BSSID_LIST, *PRT_802_11_BSSID_LIST;

typedef struct _RT_802_11_BSSID_LIST_Customized
{
	u4Byte					NumberOfItems;      // in list below, at least 1
	RT_WLAN_BSS_Customized	bssidentry[1024];
} RT_802_11_BSSID_LIST_Customized, *PRT_802_11_BSSID_LIST_Customized;

// WDS Mode, 2006.06.12, by rcnjko.
#define	WDS_DISABLED		0
#define	WDS_AP_MODE			1

//
// Customer ID, note that: 
// This variable is initiailzed through EEPROM or registry, 
// however, its definition may be different with that in EEPROM for 
// EEPROM size consideration. So, we have to perform proper translation between them.
// Besides, CustomerID of registry has precedence of that of EEPROM.
// defined below. 060703, by rcnjko.
//
typedef enum _RT_CUSTOMER_ID
{
	RT_CID_DEFAULT = 0,
	RT_CID_WHQL = 5,
	
}RT_CUSTOMER_ID, *PRT_CUSTOMER_ID;

//----------------------------------------
//add for ROGUE AP 2007.07.28 , by CCW
//----------------------------------------
typedef struct _MH_CCX_ROGUE_AP_ENTEY{
	u4Byte   code;
	u1Byte   bssid[6];
	BOOLEAN bused;
} MH_CCX_ROGUE_AP_ENTEY,*PMH_CCX_ROGUE_AP_ENTEY;


#define SIZE_ROGUE_AP_ENTEY		64
#define ASOC_INFO_IE_SIZE		2500

//
// Store the lastest association information. 2006.11.14, by shien chang.
//

typedef struct _ASOC_INFO{
	u1Byte				AuthSeq1[ASOC_INFO_IE_SIZE];
	u4Byte				AuthSeq1Length;
	u1Byte				AuthSeq2[ASOC_INFO_IE_SIZE];
	u4Byte				AuthSeq2Length;
	u1Byte				AsocReq[ASOC_INFO_IE_SIZE];
	u4Byte				AsocReqLength;
	u1Byte				AsocResp[ASOC_INFO_IE_SIZE];
	u4Byte				AsocRespLength;
	u1Byte				Beacon[ASOC_INFO_IE_SIZE];
	u4Byte				BeaconLength;
	u1Byte				PeerAddr[6];
	BOOLEAN				FlagReAsocReq;
	BOOLEAN				FlagReAsocResp;

	BOOLEAN				bDeauthAddrValid; // TRUE if the DeauAddr is valid and will be reset to FALSE in context of DrvIFIndicateDisassociation().
	u1Byte				DeauthAddr[6]; // TA of deauth frame we recevied.

} ASOC_INFO, *PASOC_INFO;

typedef enum _UPDATE_ASOC_INTO_ACTION{
	UpdateAuthSeq1,
	UpdateAuthSeq2,
	UpdateAsocReq,
	UpdateAsocResp,
	UpdateAsocBeacon,
	UpdateAsocPeerAddr,
	UpdateFlagReAsocReq,
	UpdateFlagReAsocResp,
	UpdateDeauthAddr,
} UPDATE_ASOC_INFO_ACTION, *PUPDATE_ASOC_INFO_ACTION;

//
//	Auto Select Channel related.
//
typedef	struct	_CHANNEL_INFO
{
	u1Byte	ChannelNumber;
	int		Weight;
}CHANNEL_INFO, *PCHANNEL_INFO;

typedef enum _CHANNEL_SELECT_MODE
{
	CH_SEL_DECREASE	= 0,
	CH_SEL_INCREASE	= 1,
	CH_SEL_RANDOM		= 2,
}CHANNEL_SELECT_MODE, *PCHANNEL_SELECT_MODE;

typedef enum _CHANNEL_START_MODE
{
	CH_START_MIN		= 0,
	CH_START_MIDDLE	= 1,
	CH_START_MAX		= 2,
}CHANNEL_START_MODE, *PCHANNEL_START_MODE;

//
// For Turbo mode. Added by Roger, 2006.12.07.
//
typedef struct _RT_TURBO_CA
{
	BOOLEAN		bEnabled; // TURE if TCA is ON, FALSE o.w..

	// Periodical timer for TCA checking.
	RT_TIMER	TcaCheckTimer;
	u4Byte		CheckCnt; // Min: 1, Max: CheckInterVal.
	u4Byte		CheckInterval;

	// Per-packet contention window for TCA on. 
	// Note that, this is one of means of TCA, 
	// we also can information of other means here.
	//u1Byte		ECWMin;	// For Restore.
	//u1Byte		ECWMax;//For Restore.

	// Statistics in last TCA_CHECK_INTERVAL. 
	RT_TX_STATISTICS	LastTxStats;
	RT_RX_STATISTICS	LastRxStats;
	u4Byte		LastTxRetryCnt;
	u4Byte		TxThroughput; // in KBps. 
	u4Byte		RxThroughput; // in KBps. 
	u4Byte		TotalThroughput; //in MBps

	// Statistics in last nomal channel access setting. 
	RT_TX_STATISTICS	LastNormalTxStats;
	RT_RX_STATISTICS	LastNormalRxStats;
	u4Byte		NormalTotalThroughput; // in KBps. 
}RT_TURBO_CA, *PRT_TURBO_CA;


// Add for Real WOW 
#define 	MAX_RS_PACKET_LEN		1024
#define 	MAX_RealWoW_KCP_SIZE 	124 //(100 + 24) 
#define 	MAX_RealWoW_KAP_SIZE 	48
#define 	MAX_RealWoW_Payload 		64
#pragma pack (1)	// Set our value.
typedef struct _TEREDOINFO
{
	 u1Byte			DIPv4[4];
	 u1Byte 			MacID[6];
	 u2Byte			TeredoPort;
	 u1Byte 			TeredoIP[16];
	 u1Byte 			HashPtn[16];
	 u1Byte 			HashAuth[16];
	 unsigned int 		DHCPExpireTime;
	 u2Byte 			RALostCnt;
}TEREDOINFO,*PTEREDOINFO;


typedef struct _KCPInfo{
	u4Byte			nId; // = id 
	int 				destAddr;
	u1Byte			MacID[6];
	u2Byte			desPort;
	u2Byte 			PKTLEN;
}KCPInfo, *PKCPInfo;

typedef struct _KCPContent{
	u4Byte 			id; // = id 
	u4Byte 			mSec; // = msec 
	u4Byte 			size; // =size
	u1Byte 			bPacket[MAX_RealWoW_KCP_SIZE]; // put packet here
} KCPContent, *PKCPContent;

typedef struct _RealWoWAckPktInfo { 
	u2Byte			ackLostCnt;
	u2Byte		 	patterntSize;
	u1Byte 			pattern[MAX_RealWoW_Payload];
}RealWoWAckPktInfo,*PRealWoWAckPktInfo;

typedef struct _RealWoWWPInfo { 
	u2Byte 			patterntSize;
	u1Byte 			pattern[MAX_RealWoW_Payload];
}RealWoWWPInfo,*PRealWoWWPInfo; 

typedef struct _RT_S5WakeUPInfo { 
	 u1Byte			PSK[PASSPHRASE_LEN];
	 u4Byte			PSKLen;
}RT_S5WakeUPInfo,*PRT_S5WakeUPInfo; 


#pragma pack()


typedef struct _WIRELESS_SETTING_BEFORE_SCAN
{
	/*WirelessModeScanInProgress is designed for driver to keep the previous band(2.4G/5G)
	   during scan. It is only meaningful for a/b/g device which require to switch band twice 
	   during scan (either 2.4->5->2.4 or 5->2.4->5*/
	WIRELESS_MODE		WirelessModeScanInProgress;	

	/*WirelessMode is designed to keep the original wireless mode setting before scan*/
	WIRELESS_MODE		WirelessMode;		

	/*ChannelNumber is designd to keep the original channel number before scan*/
	u1Byte				ChannelNumber;

	/*ChannelBandwidth is designed to keep the original channel bandwidth(20/40MHz) before scan*/
	CHANNEL_WIDTH		ChannelBandwidth;

	/* Extension channel offset before scan. This is useful when channel bandwidth is 40MHz. */
	EXTCHNL_OFFSET		Ext20MHzChnlOffsetOf40MHz;

	EXTCHNL_OFFSET		Ext40MHzChnlOffsetOf80MHz;

	u1Byte				CenterFrequencyIndex1;

	BOOLEAN			RestoreTo40MBW;	//for Dual band scan.

	u1Byte				WMMParamEle[WMM_PARAM_ELEMENT_SIZE];//add by ylb 20130627 for restore WMMParamEle after scan, Fix WMMParamEle changed during scan and set the wrong value of QOS Parameter( TxOP)
}WIRELESS_SETTING_BEFORE_SCAN, *PWIRELESS_SETTING_BEFORE_SCAN;

typedef struct _INIT_GAIN
{
	u1Byte				XAAGCCore1;
	u1Byte				XBAGCCore1;
	u1Byte				XCAGCCore1;
	u1Byte				XDAGCCore1;
	u1Byte				CCA;

} INIT_GAIN, *PINIT_GAIN;
/*---------------------Define local Tx BB Gain Table-----------------------*/
#define 	OFDM_TABLE_SIZE_92C	37
#define		CCK_TABLE_SIZE	33
#define		CCK_TABLE_SIZE_88F 21
#define		OFDM_TABLE_SIZE 	43

#define TxBBGainTableLength 37
#define	CCKTxBBGainTableLength 33
typedef struct _TXBBGain_Struct
{
	s4Byte	TxBBIQAmplifyGain;
	u4Byte	TXBBGainValue;
} TXBBGain_Struct, *PTXBBGain_Struct;
typedef struct _CCKTXBBGain_Struct
{
	//The Value is from a22 to a29 one Byte one time is much Safer 
	u1Byte	CCKTXBBValueArray[8];
} CCKTXBBGain_Struct,*PCCKTXBBGain_Struct;
/*---------------------End Define--------------------------------------*/


/*---------------------Define Power Save Related-----------------------*/
typedef enum _IPS_CALLBACK_FUNCION
{
	IPS_CALLBACK_NONE = 0,
	IPS_CALLBACK_MGNT_LINK_REQUEST = 1,
	IPS_CALLBACK_JOIN_REQUEST = 2,
}IPS_CALLBACK_FUNCION;

typedef enum _Fsync_State{
	Default_Fsync,
	HW_Fsync,
	SW_Fsync
}Fsync_State;	

typedef enum _RT_AP_TYPE
{
	RT_AP_TYPE_NONE = 0,		// NOT in AP mode
	RT_AP_TYPE_NORMAL,			// N5 and !n6_fake
	RT_AP_TYPE_IBSS_EMULATED,	// N6, N62 and n6_fake
	RT_AP_TYPE_VWIFI_AP,		// N62 and !n6_fake
	RT_AP_TYPE_EXT_AP,
	RT_AP_TYPE_LINUX,
} RT_AP_TYPE, *PRT_AP_TYPE;



/** Define AP states */
typedef enum _AP_STATE
{
    AP_STATE_STOPPED = 0,                   // AP INIT state
    AP_STATE_STARTING,
    AP_STATE_STARTED,
    AP_STATE_STOPPING,
    AP_STATE_INVALID = 0xFFFFFFFF           // When MP is not in AP mode
} AP_STATE, *PAP_STATE;

// RT_CUSTOM_EVENT
typedef	u4Byte	RT_CUSTOM_EVENT, *PRT_CUSTOM_EVENT;

//
// These events are realtek proprietary status indicated to varies of upper layer or receiver.
// They are common for all platform and may be mapped to the specified events to the
// platform besed events respectively.
// Different events also can be defined as the same number, because they may be indicated to
// different target.
// By Bruce, 2011-06-10.
//
#define	RT_CUSTOM_EVENT_WOL_GTK										((RT_CUSTOM_EVENT)0x00210001)	//	Wol by GTK updating
#define	RT_CUSTOM_EVENT_IRP_UNLOAD									((RT_CUSTOM_EVENT)0x00220000)	// IRP event indicated to the upper layer to notify the driver unload event

// TDLS
#define	RT_CUSTOM_EVENT_TDLS_START									((RT_CUSTOM_EVENT)0x00230000)	// TDLS peer Start event.
#define	RT_CUSTOM_EVENT_TDLS_PEER_SETUP								((RT_CUSTOM_EVENT)0x00230000)	// TDLS peer is setup.
#define	RT_CUSTOM_EVENT_TDLS_TEAR_DOWN_TO_PEER						((RT_CUSTOM_EVENT)0x00230001)	// TDLS peer is teared down.
#define	RT_CUSTOM_EVENT_TDLS_TEAR_DOWN_FROM_PEER					((RT_CUSTOM_EVENT)0x00230002)	// TDLS peer is teared down.
#define	RT_CUSTOM_EVENT_TDLS_SETUP_FAILURE							((RT_CUSTOM_EVENT)0x00230003)	// TDLS setup fails.
#define	RT_CUSTOM_EVENT_TDLS_STATUS_PEER_REJECT_CH_SWITCH			((RT_CUSTOM_EVENT)0x00230010)	// The target peer rejected the channel switch request.
#define	RT_CUSTOM_EVENT_TDLS_STATUS_CH_SWITCH_PROHIBITED			((RT_CUSTOM_EVENT)0x00230011)	// Channel switch is prohibited by AP.
#define	RT_CUSTOM_EVENT_TDLS_STATUS_PEER_NO_CH_SWITCH				((RT_CUSTOM_EVENT)0x00230012)	// Peer didn't support channel switch.
#define	RT_CUSTOM_EVENT_TDLS_STATUS_PEER_NO_PUAPSD					((RT_CUSTOM_EVENT)0x00230013)	// Target peer didn't support PUAPSD.
#define	RT_CUSTOM_EVENT_TDLS_END									((RT_CUSTOM_EVENT)0x00230FFF)	// TDLS peer End event

// WPS
#define	RT_CUSTOM_EVENT_WPS_RECV_PKT								((RT_CUSTOM_EVENT)0x00240000)	// Reveived the WPS packet

// Trouble Shooting Event
#define	RT_CUSTOM_EVENT_TRO_SH_START								((RT_CUSTOM_EVENT)0x00231000)	//	Trouble shooting event Start
#define	RT_CUSTOM_EVENT_TRO_SH_TX_RPT_HANG							((RT_CUSTOM_EVENT)0x00231001)	//	Tx Report Hang
#define	RT_CUSTOM_EVENT_TRO_SH_END									((RT_CUSTOM_EVENT)0x00231FFF)	//	Trouble shooting event End

// VHT
#define	RT_CUSTOM_EVENT_VHT_RECV_GID_MGNT_FRAME					((RT_CUSTOM_EVENT)0x00250000)		// BFee recv GID mgnt frame

#define RT_CUSTOM_EVENT_P2P_START_AP_REQ 							((RT_CUSTOM_EVENT)0x40040000)
#define RT_CUSTOM_EVENT_P2P_STOP_AP_REQ 							((RT_CUSTOM_EVENT)0x40040001)
#define RT_CUSTOM_EVENT_P2P_CONNECT_CLIENT_REQ 						((RT_CUSTOM_EVENT)0x40040002)
#define RT_CUSTOM_EVENT_P2P_DISCONNECT_CLIENT_REQ 					((RT_CUSTOM_EVENT)0x40040003)
#define RT_CUSTOM_EVENT_P2P_INDICATE_CURRENT_STATE					((RT_CUSTOM_EVENT)0x40040004)
#define RT_CUSTOM_EVENT_P2P_INDICATE_GO_FORMATED					((RT_CUSTOM_EVENT)0x40040005)
#define RT_CUSTOM_EVENT_P2P_INDICATE_ON_PROVISION_DISC_REQ			((RT_CUSTOM_EVENT)0x40040006)
#define RT_CUSTOM_EVENT_P2P_INDICATE_ON_PROVISION_DISC_RSP			((RT_CUSTOM_EVENT)0x40040007)
#define RT_CUSTOM_EVENT_P2P_INDICATE_ON_INVITATION_REQ				((RT_CUSTOM_EVENT)0x40040008)
#define RT_CUSTOM_EVENT_P2P_INDICATE_ON_INVITATION_RSP				((RT_CUSTOM_EVENT)0x40040009)
#define RT_CUSTOM_EVENT_P2P_INDICATE_CURRENT_DEV_PASSWD_ID			((RT_CUSTOM_EVENT)0x4004000A)
#define RT_CUSTOM_EVENT_P2P_INDICATE_DEV_REQ_CLIENT_GO_FORMATION	((RT_CUSTOM_EVENT)0x4004000B)
#define RT_CUSTOM_EVENT_P2P_INDICATE_ENABLE_ICS						((RT_CUSTOM_EVENT)0x4004000C)
#define RT_CUSTOM_EVENT_P2P_INDICATE_CLIENT_CONNECTED				((RT_CUSTOM_EVENT)0x4004000D)
#define RT_CUSTOM_EVENT_P2P_INDICATE_CLIENT_DISCONNECTED			((RT_CUSTOM_EVENT)0x4004000E)
#define RT_CUSTOM_EVENT_P2P_INDICATE_CURRENT_ROLE					((RT_CUSTOM_EVENT)0x4004000F)
#define RT_CUSTOM_EVENT_P2P_INDICATE_DEV_DISC_COMPLETE				((RT_CUSTOM_EVENT)0x40040010)
#define RT_CUSTOM_EVENT_P2P_INDICATE_SD_RSP							((RT_CUSTOM_EVENT)0x40040011)
#define RT_CUSTOM_EVENT_P2P_INDICATE_SD_REQ							((RT_CUSTOM_EVENT)0x40040012)
#define RT_CUSTOM_EVENT_P2P_INDICATE_SCAN_LIST						((RT_CUSTOM_EVENT)0x40040013)
#define RT_CUSTOM_EVENT_P2P_SVC										((RT_CUSTOM_EVENT)0x40040014)
#define RT_CUSTOM_EVENT_P2P_CHANGING_AP_CHANNEL						((RT_CUSTOM_EVENT)0x40040015)
#define RT_CUSTOM_EVENT_NAN_INDICATE_SDF							((RT_CUSTOM_EVENT)0x40040016)

// WDI ((RT_CUSTOM_EVENT)0xFF000000)
#define	RT_CUSTOM_EVENT_WDI_START									((RT_CUSTOM_EVENT)0xFF000000)	//	WDI event start
#define	RT_CUSTOM_EVENT_WDI_FT_ASSOC_NEEDED							((RT_CUSTOM_EVENT)0xFF000001)	//	FT association request parameter needed
#define	RT_CUSTOM_EVENT_WDI_END										((RT_CUSTOM_EVENT)0xFF0000FF)	//	WDI event end



// RT_CUSTOM_INDI_TARGET is the target indicated for RT_CUSTOM_EVENT
#define	RT_CUSTOM_INDI_TARGET_NONE			0
#define	RT_CUSTOM_INDI_TARGET_IHV			BIT0			// Indicate to IHV (for Ndis 6)
#define	RT_CUSTOM_INDI_TARGET_IRP			BIT1			// Indicate to all receivers who bound this adapter by IRP.
#define	RT_CUSTOM_INDI_TARGET_WDI			BIT2			// Indicate to Windows WDI



//
//About WOL pattern info, by tynli. 2009.06.18.
//
#define	MAX_SUPPORT_WOL_PATTERN_NUM_88E		22  //64 // 22 is on Win8.1 WHQL requirement
#define	MAX_SUPPORT_WOL_PATTERN_NUM_8723A	16
#define	MAX_SUPPORT_WOL_PATTERN_NUM_92C		8
#define	MAX_SUPPORT_WOL_PATTERN_NUM_8814A 	22 // add by ylb 20130812
#if 0
#define	MAX_SUPPORT_WOL_PATTERN_NUM_COMM	\
	((MAX_SUPPORT_WOL_PATTERN_NUM_88E > MAX_SUPPORT_WOL_PATTERN_NUM_92C)? (MAX_SUPPORT_WOL_PATTERN_NUM_88E) : (MAX_SUPPORT_WOL_PATTERN_NUM_92C))
#else 
#define 	MAX2(a, b) (a>b?a:b)
#define 	MAX4(a,b,c,d) (MAX2(MAX2(a,b), MAX2(c,d)))
#define	MAX_SUPPORT_WOL_PATTERN_NUM_COMM (MAX4(MAX_SUPPORT_WOL_PATTERN_NUM_88E,MAX_SUPPORT_WOL_PATTERN_NUM_8723A,MAX_SUPPORT_WOL_PATTERN_NUM_92C,MAX_SUPPORT_WOL_PATTERN_NUM_8814A))
#endif
// 88C supports 8 WOL patterns, 8723A supports 16 patterns, otherwise is the same as the pattern number of 88E. by tynli 2012.10.12.
#define MAX_SUPPORT_WOL_PATTERN_NUM(_Adapter)	\
	(IS_HARDWARE_TYPE_8814A(_Adapter) ? (MAX_SUPPORT_WOL_PATTERN_NUM_8814A): (MAX_SUPPORT_WOL_PATTERN_NUM_88E))

#define	RSVD_WOL_PATTERN_NUM				1	// pattern num for user defined

#define	WKFMCAM_ADDR_NUM		6
#define	WKFMCAM_SIZE			24

#define	MAX_WOL_BIT_MASK_SIZE		16 //unit: byte
#define	MAX_WOL_PATTERN_SIZE		128

#define	MAX_HW_PATTERN_MATCH_NUM_8723A	12 // 8723A Hw capability
#if 0
#define	USE_FW_PATTERN_MATCH(_pAdapter)	\
	(IS_HARDWARE_TYPE_8723A(_pAdapter) ? ((GET_POWER_SAVE_CONTROL(&(_pAdapter->MgntInfo))->WoLPatternNum > MAX_HW_PATTERN_MATCH_NUM_8723A) ? TRUE: FALSE): FALSE)
#else
#define	USE_FW_PATTERN_MATCH(_pAdapter)	FALSE	
#endif
typedef enum _WOLPATTERN_TYPE
{
	eUnicastPattern = 0,
	eMulticastPattern = 1,
	eBroadcastPattern = 2,
	eDontCareDA = 3,
	eUnknownType = 4,
}WOLPATTERN_TYPE;

// The H2C WoL pattern match info format is defined for Fw pattern match mechanism.
typedef struct _H2C_WOL_PATTERN_MATCH_INFO{
	u1Byte	PatternContent[128];
	u1Byte	BitMask[16];
	u1Byte	ValidBitNum;
}H2C_WOL_PATTERN_MATCH_INFO, *PH2C_WOL_PATTERN_MATCH_INFO;

typedef struct _RT_PM_WOL_PATTERN_INFO
{
	u4Byte	PatternId; // PatternId is set from N62 OID, N5/N6 can ignore it.
	u4Byte	Mask[4];	// The mask format is special for Hw.
	u2Byte	CrcRemainder;
	u1Byte	IsPatternMatch; // if the value is set to 1, the masks and CRC value will be download to Hw.
	WOLPATTERN_TYPE	PatternType;
	u1Byte	IsUserDefined;	// 0: from OS, 1: User defined

	// For 8723A Hw insufficient capability issue.------->
	u1Byte	IsSupportedByFW;	// Fw will parse some patterns that they cannot support by Hw.
	u1Byte	HwWFMIndex; // CAM index
	H2C_WOL_PATTERN_MATCH_INFO	FwPattern;
	// <------------------------------------------
}RT_PM_WOL_PATTERN_INFO, *PRT_PM_WOL_PATTERN_INFO;

typedef enum _PM_PROTOCOL_OFFLOAD_ID_IDX
{
	eGTKOffloadIdx = 0,
	eARPOffloadIdx = 1,
	eNSOffloadIdx1 = 2,
	eNSOffloadIdx2 = 3,
}PM_PROTOCOL_OFFLOAD_ID_IDX;


#define ACTING_AS_AP(_pAdapter) (MgntActQuery_ApType(_pAdapter) > 0)
#define ACTING_AS_IBSS(_pAdapter) (_pAdapter->MgntInfo.mIbss)

#define	RT_SET_PS_LEVEL(_pAdapter, _PS_FLAG)	(GET_POWER_SAVE_CONTROL(&(_pAdapter->MgntInfo))->CurPsLevel |= _PS_FLAG)
#define	RT_CLEAR_PS_LEVEL(_pAdapter, _PS_FLAG)	(GET_POWER_SAVE_CONTROL(&(_pAdapter->MgntInfo))->CurPsLevel &= (~(_PS_FLAG)))
#define	RT_GET_CUR_PS_LEVEL(_pAdapter)	(GET_POWER_SAVE_CONTROL(&(_pAdapter->MgntInfo))->CurPsLevel)
#define	RT_IN_PS_LEVEL(_pAdapter, _PS_FLAG)	((RT_GET_CUR_PS_LEVEL(_pAdapter) & _PS_FLAG) ? TRUE : FALSE)


typedef struct _RT_POWER_SAVE_CONTROL
{
	//
	// Inactive Power Save(IPS) : Disable RF when disconnected
	//		
	BOOLEAN				bInactivePs;
	// When AP mode stop in DC mode,  STA mode will return to IPS enable by this backup.
	BOOLEAN				bIPSModeBackup;
	BOOLEAN				bHaltAdapterClkRQ;
	BOOLEAN				bSwRfProcessing;
	RT_RF_POWER_STATE	eInactivePowerState;
	RT_WORK_ITEM		InactivePsWorkItem;
	RT_TIMER			InactivePsTimer;
	RT_IPS_STATE			IPSState;
	u1Byte				IPSDisableReason;

	// Return point for join action
	IPS_CALLBACK_FUNCION	ReturnPoint;
	
	// Recored Parameters for rescheduled JoinRequest
	PADAPTER			ptmpAdapter;// for reschedule reason, need to use original port, 2015.01.07 hsiao_ho
	                                // because we need to use the correct port to restart JOIN / MGNTLINK Request
	BOOLEAN				bTmpBssDesc;
	RT_JOIN_ACTION		tmpJoinAction;
	RT_WLAN_BSS		tmpBssDesc;

	// Recored Parameters for rescheduled MgntLinkRequest
	BOOLEAN				bTmpScanOnly;
	BOOLEAN				bTmpActiveScan;
	BOOLEAN				bTmpFilterHiddenAP;
	BOOLEAN				bTmpUpdateParms;
	u1Byte				tmpSsidBuf[33];
	OCTET_STRING		tmpSsid2Scan;
	BOOLEAN				bTmpSsid2Scan;
	u1Byte				tmpNetworkType;
	u1Byte				tmpChannelNumber;
	u2Byte				tmpBcnPeriod;
	u1Byte				tmpDtimPeriod;
	u2Byte				tmpmCap;
	OCTET_STRING		tmpSuppRateSet;
	u1Byte				tmpSuppRateBuf[MAX_NUM_RATES];
	BOOLEAN				bTmpSuppRate;
	IbssParms			tmpIbpm;
	BOOLEAN				bTmpIbpm;

	//
	// Leisre Poswer Save : Disable RF if connected but traffic is not busy
	//		
	BOOLEAN				bLeisurePs;
	// When AP mode stop in DC mode,  STA mode will return to LPS enable by this backup.
	BOOLEAN				bLeisurePsModeBackup;
	u1Byte				LpsIdleCount;
	u1Byte				PowerSetting;
	u1Byte				PowerPolicy;	
	u4Byte				PowerProfile;
	u4Byte				PowerSaveLevel;	
	u4Byte				ConnectionQuality;	
	u1Byte				PowerMode;
	u1Byte				RegMaxLPSAwakeIntvl;
	u1Byte				LPSAwakeIntvl;
	BOOLEAN				RegAdvancedLPs;
    UINT8 				MaximumOffChannelOperationTime; // Maximum number of milliseconds to be off channel
    UINT8 				RoamingNeededLinkQualityThreshold; // Link quality below which its OK to send a roaming needed indication

	//RF OFF Level
	u4Byte				CurPsLevel;
	u4Byte				RegRfPsLevel;
	u1Byte				RegAMDPciASPM;
	
	// 2009/10/28 MH Add for DMA 64 bit support switch
	u1Byte				RegDMA64Support;

	//Fw Control LPS
	BOOLEAN				bFwCtrlLPS;
	u1Byte				FWCtrlPSMode;
	BOOLEAN				bLowPowerEnable;	// 32k
	RT_WORK_ITEM		FwPsClockOnWorkitem;

	//2009.01.01 added by tynli
	// Record if there is a link request in IPS RF off progress.
	BOOLEAN				LinkReqInIPSRFOffPgs;
	// To make sure that connect info should be executed, so we set the bit to filter the link info which comes after the connect info.
	BOOLEAN				BufConnectinfoBefore;

	//GPIO polling HW Radio On/Off.
	BOOLEAN				bGpioRfSw;

	// WoWLAN
	u1Byte				WoWLANMode;
	BOOLEAN				bWakeOnDisconnect;
	u1Byte				APOffloadEnable;
	BOOLEAN				bEnterLPSDuringSuspend;	// For WoWLAN, enter LPS during S3/S4.
	u1Byte				WoWLANLPSLevel;
	u1Byte				ARPOffloadEnable; // controlled by NDIS OID in WHCK test
	u1Byte				GTKOffloadEnable; // controlled by NDIS OID in WHCK test
	u1Byte				NSOffloadEnable; // controlled by NDIS OID in WHCK test
	u1Byte				RegARPOffloadEnable; // set by registry
	u1Byte				RegGTKOffloadEnable; // set by registry
	u1Byte				RegNSOffloadEnable;
	u1Byte				ProtocolOffloadDecision;
	u1Byte				FSSDetection;
	BOOLEAN				bOSSupportProtocolOffload;
	BOOLEAN				bSetPMParameters;
	u1Byte				WoWLANS5Support;
	u4Byte				EapolRequestIdMessagePatternId;
	u4Byte				MagicPacketPatternId;
	u4Byte				IPv4TcpSynPatternId;
	u4Byte				IPv6TcpSynPatternId;
	BOOLEAN				bSupportWakeUp;
	u1Byte				D2ListenIntvl;
	u4Byte				PMProtocolOffloadIDs[4]; // GTK offload, ARP offoad, NS offload*2
	BOOLEAN				bFakeWoWLAN;
	BOOLEAN				bPnpEnterD2; // for 8723BS connected standby setting.
	BOOLEAN				DxNLOEnable; // FW supports NLO in D2/D3 state.
	RT_AOAC_REPORT		AOACReport;

	// FW IPS
	BOOLEAN				bFwCtrlPwrOff;	// Apply to RF off and IPS state
	BOOLEAN				bInDxFwIPSPeriod; // IPS during device state in Dx (x=1~3)
	BOOLEAN				CardDisableInLowClk;
	u1Byte				FwIPSLevel;
	
	//  Record wake up frame for WoWLAN. by tynli. 2009.06.10.
	RT_PM_WOL_PATTERN_INFO		PmWoLPatternInfo[MAX_SUPPORT_WOL_PATTERN_NUM_COMM]; //by tynli. To keep the WoWLAN wake up pattern information, ex: pattern Id, bit mask.
	u1Byte						WoLPatternNum;
	u1Byte						WoLPktNoPtnNum;
	BOOLEAN						bFindWakePacket;

	// Control LPS by OID. by tynli. 2011.01.04.
	BOOLEAN				bDisableLPSByOID;
	u1Byte				RegLeisurePsMode;
	u1Byte				RegPowerSaveMode;

	u4Byte						WakeUpReason;	// Used for WOL, indicates the reason for waking event.
	u4Byte						SleepMode;		// Record the sleep mode.
	u8Byte						LastWakeUpTime;	// Record the last waking time for comparison with setting key.

	// PNP sleep/wake related info.
	u4Byte				PnpSleepEnterD2Cnt;
	u4Byte				PnpSleepEnterD3Cnt;
	u4Byte				PnpSleepEnterUnknownDxCnt;
	u4Byte				PnpWakeD0Cnt;
	u8Byte				LastPnpSleepTime;
	u8Byte				LastPnpWakeTime;
	u8Byte				LastLPSEnterTime;
	u8Byte				LastLPSLeaveTime;

	//Close power of IO during LPS32K, The feature is only for 8723D 20150623
	BOOLEAN				bRegLPSDeepSleepEnable;
}RT_POWER_SAVE_CONTROL,*PRT_POWER_SAVE_CONTROL;

typedef enum _RT_FRAME_HEADER
{
    RTFrameHeaderUndefined,
    RTFrameHeaderMac,
    RTFrameHeaderArp,
    RTFrameHeaderIPv4,
    RTFrameHeaderIPv6,
    RTFrameHeaderUdp,
    RTFrameHeaderMaximum
}RT_FRAME_HEADER, *PRT_FRAME_HEADER;

typedef enum _RT_MAC_HEADER_FIELD
{
    RTMacHeaderFieldUndefined,
    RTMacHeaderFieldDestinationAddress,
    RTMacHeaderFieldSourceAddress,
    RTMacHeaderFieldProtocol,
    RTMacHeaderFieldVlanId,
    RTMacHeaderFieldPriority,
    RTMacHeaderFieldPacketType,
    RTMacHeaderFieldMaximum
}RT_MAC_HEADER_FIELD, *PRT_MAC_HEADER_FIELD;

typedef enum _RT_MAC_PACKET_TYPE
{
    RTMacPacketTypeUndefined = 0,
    RTMacPacketTypeUnicast = 1,
    RTMacPacketTypeMulticast = 2,
    RTMacPacketTypeBroadcast = 3,
    RTMacPacketTypeMaximum
} RT_MAC_PACKET_TYPE, *PRT_MAC_PACKET_TYPE;

typedef enum _RT_ARP_HEADER_FIELD
{
    RTARPHeaderFieldUndefined,
    RTARPHeaderFieldOperation, // request or response
    RTARPHeaderFieldSPA, // source protocol address
    RTARPHeaderFieldTPA, //  target protocol address
    RTARPHeaderFieldMaximum
} RT_ARP_HEADER_FIELD, *PRT_ARP_HEADER_FIELD;

typedef enum _RT_IPV4_HEADER_FIELD
{
    RTIPv4HeaderFieldUndefined,
    RTIPv4HeaderFieldProtocol,
    RTIPv4HeaderFieldMaximum
}RT_IPV4_HEADER_FIELD, *PRT_IPV4_HEADER_FIELD;

typedef enum _RT_IPV6_HEADER_FIELD
{
    RTIPv6HeaderFieldUndefined,
    RTIPv6HeaderFieldProtocol,
    RTIPv6HeaderFieldMaximum
}RT_IPV6_HEADER_FIELD, *PRT_IPV6_HEADER_FIELD;

typedef enum _RT_UDP_HEADER_FIELD
{
    RTUdpHeaderFieldUndefined,
    RTUdpHeaderFieldDestinationPort,
    RTUdpHeaderFieldMaximum
}RT_UDP_HEADER_FIELD, *PRT_UDP_HEADER_FIELD;

typedef enum _RT_RECEIVE_FILTER_TEST
{
    RTReceiveFilterTestUndefined,
    RTReceiveFilterTestEqual,
    RTReceiveFilterTestMaskEqual,
    RTReceiveFilterTestNotEqual,
    RTReceiveFilterTestMaximum
}RT_RECEIVE_FILTER_TEST, *PRT_RECEIVE_FILTER_TEST;

typedef struct _RT_DO_COALESICING_FIELD_INFO
{
	u1Byte							Heard[8];   // Resvad for ndis 
	RT_FRAME_HEADER				FrameHead;
	RT_RECEIVE_FILTER_TEST			FilterTest;
	
	 union
    	{
	        RT_MAC_HEADER_FIELD       	MacHeaderField;
	        RT_ARP_HEADER_FIELD        	ArpHeaderField;
	        RT_IPV4_HEADER_FIELD      	IPv4HeaderField;
	        RT_IPV6_HEADER_FIELD      	IPv6HeaderField;
	        RT_UDP_HEADER_FIELD       	UdpHeaderField;
    	}HeaderField;
    	
	union 
	{
	        u1Byte			FieldByteValue;
	        u2Byte			FieldShortValue;
	        ULONG               	FieldLongValue;
	        ULONGLONG		FieldLong64Value;
	        u1Byte               	FieldByteArrayValue[16];
	 }FieldValue;

	union                          // used when test operation is MaskEqual
    	{
	        u1Byte               	ResultByteValue;
	        u2Byte              	ResultShortValue;
	        ULONG               	ResultLongValue;
	        ULONGLONG   	ResultLong64Value;
	        u1Byte               	ResultByteArrayValue[16];
    	}ResultValue;
    	
	
}RT_DO_COALESICING_FIELD_INFO,*PRT_DO_COALESICING_FIELD_INFO;


typedef struct _RT_DO_COALESICING_FILTER_PARAMETER
{
	u4Byte									FilterID;       		// != 0 : in used !!
	u4Byte									Delaytime;   	 	// how long to flush packet  
	u4Byte									NumOfElem; 	// For keep number of field !!
	RT_DO_COALESICING_FIELD_INFO			dFieldArry[5];     // We define in 5 in windows 8.1  !! 
	
}RT_DO_COALESICING_FILTER_PARAMETER,*PRT_DO_COALESICING_FILTER_PARAMETER;

typedef struct _RT_DO_COALESICING_FILTER_INFO
{
	BOOLEAN									bEnable;   // Do or not !!
	RT_DO_COALESICING_FILTER_PARAMETER	dFilterArry[10]; // We define in 10 in windows 8.1  !! 
	
}RT_DO_COALESICING_FILTER_INFO,*PRT_DO_COALESICING_FILTER_INFO;

typedef struct _RT_DBG_CONTROL
{
	BOOLEAN		PowerSaveDisable;		// if lock is true, leave and don't do power save anymore.
}RT_DBG_CONTROL, *PRT_DBG_CONTROL;
/*---------------------End Define--------------------------------------*/

#pragma pack(1)

typedef struct _CUSTOMIZED_SCAN_REQUEST{
	BOOLEAN bEnabled; // specifies whether this customized scan request is valid
	u1Byte Channels[MAX_CHANNEL_NUM]; // channels to scan
	u1Byte nChannels; // num of items in Channels[]
	RT_SCAN_TYPE ScanType; // passive/active scan
	u2Byte Duration; // scan duration on each channel of Channels[]
	u1Byte DataRate; // data rate used to send probe req
	u2Byte ProbeReqLen; // length of the customized probe req
	u1Byte ProbeReqBuf[1]; // the customized probe req
} CUSTOMIZED_SCAN_REQUEST, *PCUSTOMIZED_SCAN_REQUEST;

#pragma pack()

#define	IHV_SUPPORT_TYPE				u1Byte
#define	PIHV_SUPPORT_TYPE				pu1Byte

#define	IHV_SUPPORT_NONE				0		// No IHV support
#define	IHV_SUPPORT_DEFAULT				BIT0	// The IHV support default (realtek) functions
#define	IHV_SUPPORT_CCX_SERVICE			BIT1	// The IHV support CCX service

typedef enum BEACON_SEND_STATE {
	BEACON_STOP = 0,			// The configuration is in the stopped state. The SW stopped the SW timer.
	BEACON_STARTING = 1,		// It's in HW/SW beacon configuration state and the Beacon is not sent yet.
	BEACON_STARTED = 2,			// The fist beacon has been sent and the beacons are continuously sent.
} BEACON_SEND_STATE, *PBEACON_SEND_STATE;

typedef   struct _Check_Stable_Traffic
{
	BOOLEAN			bGetValue;
	BOOLEAN			bShake;
	BOOLEAN			bDetectIndicationNumber;
	u1Byte			count;
	u1Byte			ErrorCount;
	u1Byte			levelCount ;
	u1Byte			shakeCount ;
	u1Byte			stableCount ;
	u1Byte			stableIndicationNumber;
}Check_Stable_Traffic, *PCheck_Stable_Traffic;

#define	FT_INFO_VALID				BIT0	// This entry is valid
#define	FT_INFO_WAIT_OS_DECISION	BIT1	// Wait the OS's decision
#define	FT_INFO_OS_DECISION_MADE	BIT2	// OS has made the FT association info.
typedef struct _FT_INFO_ENTRY
{
	u1Byte		Flag;	// FT_INFO_xxx
	u1Byte		targetAddr[6];

	u4Byte		osAssocInfoStatus;

	u2Byte		MDELen;
	u1Byte		MDE[MAX_MD_IE_LEN + SIZE_EID_AND_LEN];

	u2Byte		FTELen;
	u1Byte		FTE[MAX_FT_IE_LEN + SIZE_EID_AND_LEN];

	u2Byte		PMKR0NameLen;
	u1Byte		PMKR0Name[PMKID_LEN]; // PMKR0Name is used to identify the PMK-R0

	u2Byte		RSNELen;
	u1Byte		RSNE[MAX_IE_LEN];	// Including ID and Length of IE
}FT_INFO_ENTRY, *PFT_INFO_ENTRY;
#define	MAX_FT_ENTRY_NUM	6

typedef enum _FT_ENTRY_ACTION
{
	FT_ENTRY_ACTION_DEL_TARGET = 0,
	FT_ENTRY_ACTION_UPDATE_MDE,
	FT_ENTRY_ACTION_UPDATE_FTE,
	FT_ENTRY_ACTION_UPDATE_PMKR0_NAME,
	FT_ENTRY_ACTION_UPDATE_RSNE,
	FT_ENTRY_ACTION_UPDATE_ASSOC_INFO_STATUS,
	FT_WAIT_OS_DECISION,
	FT_OS_DECISION_MADE,
	//=== Insert new method above this line === //
	FT_ENTRY_ACTION_MAX,	// Reserved
	FT_ENTRY_ACTION_FORCE_UINT32 = 0xFFFFFFFF	//Reserved, force this enumeration to compile to 32 bits in size (by Win SDK 8.0).
}FT_ENTRY_ACTION, *PFT_ENTRY_ACTION;

typedef struct _MGNT_INFO{

	// -- Driver operation mode --
	RT_OP_MODE			OpMode;
	u1Byte				Bssid[6];
	OCTET_STRING		Ssid;
	u1Byte				SsidBuf[33];

        // add for 92D hidden ssid scan issue
	//BOOLEAN				bHiddenSSID92D;
	u1Byte				hiddenChannel;
	BOOLEAN				bNeedSkipScan;
	BOOLEAN				bCompleteScan;
	BOOLEAN				bResetScan;
	BOOLEAN				bScanTimeExceed;

	BOOLEAN				bScanOnly;
	BOOLEAN				bDisableScanByOID;
	BOOLEAN				bIbssStarter;
	BOOLEAN				bIbssCoordinator; // TRUE if we are winner of latest Beacon contention and should be the coordinator in this Beacon Period, FALSE otherwise, 2005.12.15, by rcnjko.

	BOOLEAN				bMediaConnect;
	BOOLEAN				init_adpt_in_progress;

	

	BOOLEAN				bFlushScanList; // If this flag is set true when querying scan list, it means the OS has flushed the scan list.

	BOOLEAN				RequestFromUplayer;
	BOOLEAN				RfRequestFromUplayer; //Record Rf state from Windows UI.
	BOOLEAN				bDisconnectRequest;
	BOOLEAN				bRoamRequest;
	BOOLEAN				bPrepareRoaming;
	RT_PREPARE_ROAM_TYPE	PrepareRoamState;
	
	// Set when authentication type, capability changes.
	// Refered in MgntActSet_802_11_SSID().
	RT_MLMESTARTREQ_RSN	bMlmeStartReqRsn;

	RT_JOIN_ACTION			JoinAction;


	RT_LINK_DETECT_T	LinkDetectInfo;
	u2Byte				DisconnectedSlotCount;

	u2Byte				FragThreshold;

	// Scanning flags
	// -----------------------------------------
	BOOLEAN				bScanInProgress;
	BOOLEAN				bActiveScan;
	u1Byte				Scancount;			//To distinguish each scan
	RT_TIMER			ScanTimer;
	u1Byte				ScanStep;
	BOOLEAN				bScanAntDetect;

	// Replace the Windows Ndis6 CheckforHang Timer -----------------------
	RT_TIMER			WatchDogTimer;
	BOOLEAN				WatchDogReturnFlag;
	BOOLEAN				bSetWatchDogTimerByDriver;
	// ---------------------------------------------------------------

#if (DFS_SUPPORT == 1)	
	DFS_MGNT			DFSMgnt;
#endif
	WIRELESS_SETTING_BEFORE_SCAN	SettingBeforeScan;

	RT_CHANNEL_DOMAIN	ChannelPlan;		// The current channel plan.
	u2Byte				RegChannelPlan; 	// Channel Plan specifed by user, read from INF.
	BOOLEAN				bChnlPlanFromHW;	// TRUE is channel plan is defined by HW and can't changed by SW(OID/INF).

        BOOLEAN                    bLoadIMRandIQKSettingFor2G;// True if IMR or IQK  have done  for 2.4G in scan progress

	// bDualModeScanStep
	//  - 0:idle, only one mode
	//  - 1:scanning mode need to change
	//  - 2:scanning mode change to the mode 2
	//  - 3:Complete scan, change to mode 3
	u1Byte				bDualModeScanStep;

	// SSIDs to be scan, 2006.12.19, by shien chang.
	RT_SSIDS_TO_SCAN	SsidsToScan;

	// Join flags
	// -----------------------------------------
	BOOLEAN				bIbssNeedAuth;

	RT_TIMER			JoinTimer;
	RT_TIMER			JoinConfirmTimer;
	RT_TIMER			JoinProbeReqTimer;
	RT_TIMER			WaitingKeyTimer;
	u2Byte				JoinRetryCount;
	BOOLEAN				bJoinInProgress;

	BOOLEAN				bResetInProgress;

	u1Byte				state_Synchronization_Sta;
	u1Byte				state_Synchronization_Sta_BeforeScan;
	u1Byte				FilterHiddenAP;

	// BSS List
	// -----------------------------------------
	u2Byte				NumBssDesc;
	RT_WLAN_BSS			bssDesc[MAX_BSS_DESC];

	// Joseph add for antenna switch
	u2Byte				tmpNumBssDesc;
	RT_WLAN_BSS			tmpbssDesc[MAX_BSS_DESC];

	// For ON_BEACON use. Fix lower memory verifier or BSOD.
	RT_WLAN_BSS			bssDesc_OnBeacon;

	u2Byte				NumBssDesc4Query;
	RT_WLAN_BSS			bssDesc4Query[MAX_BSS_DESC];

	u2Byte				NumBssDesc4Rssi;
	RT_WLAN_RSSI		bssDesc4Rssi[MAX_BSS_DESC];

	RT_WLAN_BSS			targetBSS;	// Target BSS information to connect

	// Current status
	// -----------------------------------------
	WIRELESS_MODE		dot11CurrentWirelessMode;
	WIRELESS_MODE		CurrentBssWirelessMode; // For damn Netgear's request: they want to get wireless mode of BSS not that of STA. 2005.02.17, by rcnjko.
	BOOLEAN				bHalfWiirelessN24GMode;
	u1Byte				dot11CurrentChannelNumber;
	CHANNEL_WIDTH		dot11CurrentChannelBandWidth;
	u2Byte				dot11BeaconPeriod;
	u1Byte				dot11DtimPeriod;
	u1Byte				mDtimCount; // 0 at TBTT of beacon with DTIM.

	u2Byte				dot11AtimWindow;
	u2Byte				dot11RtsThreshold;
	u2Byte				ListenInterval;
	u2Byte				ListenIntervalInDX;

	u2Byte				mCap;

	EXTCHNL_OFFSET		ChannelOffset;
	// 802.11n related HT info
	PRT_CHANNEL_INFO			pChannelInfo;
	PRT_HIGH_THROUGHPUT			pHTInfo;
	PRT_VERY_HIGH_THROUGHPUT	pVHTInfo;

	RT_WORK_ITEM		ChangeBwChnlFromPeerWorkitem;

	u1Byte				ratr_index;
	u1Byte				BT_RatrInx;
	u1Byte				Multi_RatrInx;
	u1Byte				Ratr_State;
	u1Byte				mBratesBuf[MAX_NUM_RATES];
	OCTET_STRING		mBrates;

	u1Byte				HighestBasicRate;
	u1Byte				LowestBasicRate;	
	u1Byte				HighestOperaRate;
	u1Byte				CurrentBasicRate;
	u1Byte				CurrentOperaRate;

	OCTET_STRING		dot11OperationalRateSet;
	u1Byte				dot11OperationalRateBuf[MAX_NUM_RATES];
	OCTET_STRING		Regdot11OperationalRateSet;
	u1Byte				Regdot11OperationalRateBuf[MAX_NUM_RATES];

	// -- Supported Rate --
	OCTET_STRING		SupportedRates; 
	u1Byte				SupportedRatesBuf[MAX_NUM_RATES];
	OCTET_STRING		RegSuppRateSets[MAX_WIRELESS_MODE_CNT];
	u1Byte				RegSuppRateSetsBuf[MAX_WIRELESS_MODE_CNT][MAX_NUM_RATES];

	u1Byte				HTHighestOperaRate;
	u1Byte				RegHTSuppRateSet[16];
	u1Byte				dot11HTOperationalRateSet[16];			//use RATR format
	u1Byte				Regdot11HTOperationalRateSet[16];		//use RATR format

	u1Byte				VHTHighestOperaRate;
	u1Byte				RegVHTHighestOperaRate;
	u1Byte				RegVHTSuppRateSet[2];
	u1Byte				dot11VHTOperationalRateSet[2];		
	u1Byte				Regdot11VHTOperationalRateSet[2];		

	// For basic rate set maintain. 2010.11.03. by tynli.
	u1Byte				SupportRatesfromBCNBuf[MAX_NUM_RATES];
	OCTET_STRING		SupportRatesfromBCN;
	
        // For Min spacing configurations, Added by Roger, 2008.08.30.
	u1Byte				MinSpaceCfg;

	u2Byte				SequenceNumber;
	u1Byte				SoundingSequence;

	// Set TRUE before associate to AP or join an IBSS network
	// Set FALSE when (1) associate to AP, (2) Join an IBSS, (3) AP mode
	BOOLEAN				mDisable;

	// Set TRUE after receiving association response from AP.
	BOOLEAN				mAssoc;
	BOOLEAN				bStartDelayActionFrame;

	// Set TRUE after joining an IBSS or starting an IBSS.
	BOOLEAN				mIbss; //Boolean:= false,
	u1Byte				mMacId;
	u1Byte				mcastMacId;	// macId used for multicast/broadcast
	u2Byte				mAId;
	RT_AP_TYPE			ApType;
	AP_STATE			APState;
	BOOLEAN				bSwitchingAsApInProgress;
	BOOLEAN				bSwitchingSTAStateInProgress;
	BOOLEAN				bDelaySwitchToApMode;
	// For AP mode delay mechanism test ,.
	BOOLEAN				bDelayApBeaconMode;
	u4Byte				DelayApBeaconCnt;
	
	RT_WORK_ITEM		IbssStartRequestWorkItem;
	RT_WORK_ITEM		ApStartRequestWorkItem;
	RT_WORK_ITEM		ApSendDisassocWithOldChnlWorkitem;	 

	// To protect drop packet progress
	BOOLEAN				bDropPktInProgress;
	RT_WORK_ITEM		DropPacketWorkItem;
	
	IbssParms			mIbssParms;    // empty if infrastructure BSS

	
	u1Byte				dot11CurrentPreambleMode; // It represents current preamble mode. 2005.01.06, by rcnjko.

	BOOLEAN				bDisableUseProtection; // TRUE if protection mode forced disabled by User.
	BOOLEAN				bUseProtection; // TRUE if the BSS is now in protection mode.
	BOOLEAN				bProtection;	// The protection status of default port or extension port, the value is independent to each port. by tynli.

	// Authentication status
	// -----------------------------------------
	RT_TIMER			AuthTimer;
	u1Byte				State_AuthReqService;
	u1Byte				AuthReq_auAlg;	// where to set?
	u1Byte				AuthRetryCount;
	u1Byte				AuthStatus; //Indicate OID

	u2Byte				RspCapability;
	u2Byte				RspStatusCode;
	u2Byte				RspAssociationID;

	// Association status
	// -----------------------------------------
	RT_TIMER			AsocTimer;
	u1Byte				State_AsocService;
	u1Byte				AsocRetryCount;
	u8Byte				AsocTimestamp;

	// Beacon frame
	// -----------------------------------------
	OCTET_STRING		beaconframe;
	SHARED_MEMORY		BcnSharedMemory;

	// SW Beacon.
	// -----------------------------------------
	RT_TIMER			SwBeaconTimer;
	BEACON_SEND_STATE	BeaconState;	// The current sending state of Beacon
	RT_WORK_ITEM		TbttPollingWorkItem; // Workitem to simulate TBTT notification by polling MAC.TSF, 070124, by rcnjko.
	
	u4Byte				MaxBeaconSize;	//2010.06.23. Added by tynli. To observe the max beacon size.
	
	// Roaming flags
	// -----------------------------------------
	RT_ROAM_TYPE		RoamingType;
	RT_ROAMING_STATE	RoamingState;
	u1Byte				APaddrbeforeRoaming[6];
	u1Byte				PrepareRoamingCount;
	u1Byte				RoamingFailCount;
	u1Byte				RegRoamingLimitCount;		// Max Roaming count.

	//
	// When wake up, AP would be reset by MgntActSet_802_11_SSID().
	// We should not restart beaconning under win7. 
	// This flag is for AP_StartAPRequest() to determine whether to start beaconning.
	//
	BOOLEAN				bStartApDueToWakeup;
	//default setting ==================
	u1Byte				Regdot11ChannelNumber;
	u1Byte				Regdot112GChannelNumber;
	u1Byte				Regdot115GChannelNumber;
	u2Byte				Regdot11BeaconPeriod;
	u1Byte				Regdot11DtimPeriod;

	//Set when load default value and MgntActSet_802_11_INFRASTRUCTURE_MODE()
	u1Byte				Regdot11networktype; 
	u2Byte				RegmCap;

	
	u1Byte				RegPreambleMode; // It represents default setting about preamble mode. 2006.01.06, by rcnjko. 

	// Power Save Ralated
	BOOLEAN				RegRfOff;
	BOOLEAN				RFChangeInProgress; // RF Chnage in progress, by Bruce, 2007-10-30.
	RT_POWER_SAVE_CONTROL	PowerSaveControl;

	BOOLEAN				bHwWpsPbc;
	BOOLEAN				bSdioDpcIsr;

	u1Byte				LastRxUniSrcAddr[6];		// added by Annie, 2005-07-25.
	u1Byte				LastRxUniFragNum;
	u2Byte				LastRxUniSeqNum;

	// 2005.01.13, by rcnjko.
	u2Byte				ForcedDataRate; // Force Data Rate. 0: Auto, 0x02: 1M ~ 0x6C: 54M.
	u2Byte				ForcedBstDataRate;
	BOOLEAN				ForcedTxDisableRateFallBack;

	u1Byte				bRegHwParaFile;

	u2Byte				RoamingCount;
	u2Byte				DisconnectCount;

	// 2006.09.04, by shien chang.
	BOOLEAN				bPSPXlinkMode; // Current enable XLink mode (promicuous mode).
	BOOLEAN				bDefaultPSPXlinkMode; // Enable XLink mode (promicuous mode) for default settings, i.e., from registry or UI.
	BOOLEAN				bNetMonitorMode;  // 2007.08.06, by shien chang.
	
	//----------------------------------------------------------------------------
	// Power save mode, 070104, by rcnjko.
	//----------------------------------------------------------------------------
	RT_PS_MODE			dot11PowerSaveMode; // Power save mode configured.
	u1Byte				PsPollType;			// 0: Auto, 1: Forced PsPoll, 2: Forced PsNonPoll (null frame)
	RT_PS_STATE			mPss; // Current power save state of the device.
	BOOLEAN				bAwakePktSent; // TRUE if we had sent a packet with PwrMgt bit == 0 since mPss becom eAwake.
	RT_WORK_ITEM		DozeWorkItem;
	RT_TIMER			AwakeTimer;
	RT_WORK_ITEM		AwakeWorkItem;
	u1Byte				TxPollingTimes;
	BOOLEAN				BcnIntvalKeepAwake;

	//----------------------------------------------------------------------------
	// AP mode related, 2005.05.27, by rcnjko.
	//----------------------------------------------------------------------------

	u1Byte				arChalng[128]; // Challenge text for shared-key authenticatin. 

#if 1//(AP_MODE_SUPPORT == 1)
	// 2012/01/12 MH Add one RT_WLAN_STA for ODM default port conpatiable with other SW team.
	// If STA mode is more than 2 we can add the array number.
	RT_WLAN_STA		DefaultPort[1];
	RT_WLAN_STA		AsocEntry[ASSOCIATE_ENTRY_NUM]; // Information of STAs. <RJ_TODO_AP> Revise the data type name and its variables.
#endif
	u2Byte				AvailableAsocEntryNum; // # of entries of AsocEntry[] available.
	u2Byte				PowerSaveStationNum; // # of STA in PS mode. <RJ_TODO_AP> 
	RT_LIST_ENTRY		GroupPsQueue; // Mcst/Bcst packet queued for one or more STA is in power-save mode.
	OCTET_STRING		Tim; // A octet string for TIM IE.
	u1Byte 				TimBuf[254]; // 254=3+251, Buffer for TIM IE.
	int					LiveTime;	// Station live time. (in second)
	u1Byte				AvailableAIDTable[ASSOCIATE_ENTRY_NUM];
	u2Byte				MaxMACID;  //YJ,add for TX RPT,111213

	RT_TIMER			DelaySendBeaconTimer;
	// AP mode global security related data: added by Annie, 2005-06-28.
	AUTH_GLOBAL_KEY_TAG	globalKeyInfo;
	// Annie TOADD: VirtualCAM

	// for indication
	PRT_WLAN_STA		pCurrentSta; // the STA we are dealing with for indication. we get the state machine through here, similar as pMgntInfo->AsocInfo 

	// Additonal IEs for beacon
	u4Byte                   		AdditionalBeaconIESize;
	PVOID                   		AdditionalBeaconIEData; // Allocated when OID_DOT11_ADDITIONAL_IE is set.

	//Additonal IEs for probe response
	u4Byte                   		AdditionalResponseIESize;
	PVOID                   		AdditionalResponseIEData;   

	u4Byte				AdditionalAssocReqIESize;
	PVOID				AdditionalAssocReqIEData;

	u4Byte				AdditionalProbeReqIESize;
	PVOID				AdditionalProbeReqIEData;

	//
	// Customer ID, 060703, by rcnjko.
	//
	RT_CUSTOMER_ID		CustomerID;

	//
	// For Turbo mode. Added by Roger, 2006.12.07.
	//
	RT_TURBO_CA			TurboChannelAccess;
	
	// Security Information
	RT_SECURITY_T		SecurityInfo;

	// 802.11e and WMM Traffic Stream Info (TX)
	RT_LIST_ENTRY		Tx_TS_Admit_List;
	RT_LIST_ENTRY		Tx_TS_Pending_List;
	RT_LIST_ENTRY		Tx_TS_Unused_List;
	TX_TS_RECORD		TxTsRecord[TOTAL_TS_NUM];

	// 802.11e and WMM Traffic Stream Info (RX)
	RT_LIST_ENTRY		Rx_TS_Admit_List;
	RT_LIST_ENTRY		Rx_TS_Pending_List;
	RT_LIST_ENTRY		Rx_TS_Unused_List;
	RX_TS_RECORD		RxTsRecord[TOTAL_TS_NUM];
	RX_REORDER_ENTRY	RxReorderEntry[RX_REORDER_ENTRY_NUM];
	RT_LIST_ENTRY		RxReorder_Unused_List;
#if RX_AGGREGATION
	// Rx Aggr Batch indication
	u1Byte 				IndicateTsCnt;
	PRX_TS_RECORD 		IndicateTsArray[TOTAL_TS_NUM];
#endif

	// Qos related. Added by Annie, 2005-11-01.
	PSTA_QOS			pStaQos;
	
	BOOLEAN				bUpdateTxPowerInProgress; // TRUE if the operation to update all tx power of all channel is in progress.
	s4Byte				PowerToUpdateInDbm; // Tx power to update in dBm.
	s4Byte				ClientConfigPwrInDbm; // Client configured power in dbm for CCX
	RT_WORK_ITEM		UpdateTxPowerWorkItem; // Workitem to update Tx power.

	BOOLEAN				bScanWithMagicPacket; // TRUE if we send magic instead of ProbeReq in scan process for WOL. 2005.06.27, by rcnjko.
	u1Byte				MagicPacketDstAddr[6]; // Destination address of magic packet, i.e. the MAC address of the STA we want to wake up. 2005.06.27, by rcnjko.
	BOOLEAN				bHiddenSSIDEnable;

	//  Auto select channel by an evaluation algorithm when start an AP mode, 2005.12.27, by rcnjko.
	//	<RJ_TODO> It can be applied to IBSS mode too.
	BOOLEAN				bAutoSelChnl;	// TRUE if we will select a channel, FALSE otherwise.
	u1Byte				ChnlWeightMode;	// There are max 10 modes, see also: AutoSelectChannel(). Added by Annie, 2006-07-26.
	//

	// AP mode Hidden SSID flag. Added bY Annie, 2006-02-14.
	BOOLEAN				bHiddenSSID;

	// AP mode locked STA list. Added by Annie, 2006-02-14.
	u1Byte				LockType;
	u1Byte				LockedSTAList[MAX_LOCKED_STA_LIST_NUM][ETHERNET_ADDRESS_LENGTH];
	u4Byte				LockedSTACount;
	// AP mode locked STA list. Added by Ken 2014-02-20.
	u1Byte				LockedSTAList_Reject[MAX_LOCKED_STA_LIST_NUM][ETHERNET_ADDRESS_LENGTH];
	u4Byte				LockedSTACount_Reject;
	BOOLEAN				bDefaultPermited;

	//Cloud Key Offset. requested by 360
	u1Byte				cloud_key_offset;

	//AP Mode Station Count
	u1Byte				StaCount;

	// AP mode: force per-station data rate. 2006.04.11, by rcnjko.
	RT_STA_DATARATE	StaDataRateList[MAX_STA_DATARATE_LIST_NUM];
	u4Byte				StaDataRateCount; // # of entries used in StaDataRateList[].
	u1Byte				RegApModeDefPerStaDataRate;

	// WDS, 2006.06.11, by rcnjko.
	u1Byte				WdsMode;
	u1Byte				WdsApAddr[6];
	
	//
	// For RF state maintance.
	//
	RT_RF_CHANGE_SOURCE	RfOffReason;	
	BOOLEAN				ClkReqState;	

	CHANNEL_ACCESS_SETTING	ChannelAccessSetting;
	
	//ADD for AP HW ENC ,CCW
	BOOLEAN				bAPGlobRest;
	SW_CAM_TABLE           SWCamTable[32];

	//----------------------------------------
	//add for ROGUE AP 2007.07.27 , by CCW
	//----------------------------------------
	BOOLEAN					bROGUEAP;
	MH_CCX_ROGUE_AP_ENTEY	ROGUE_AP_ENTEY[SIZE_ROGUE_AP_ENTEY];
	//----------------------------------------
	//add for ROGUE AP 2007.07.28 , by CCW
	//----------------------------------------
	BOOLEAN				bNETWORKEAP;

	// Workaround by shien chang, please see ASSO_INFO. 2006.10.28, by shien chang.
	ASOC_INFO			AsocInfo;

	//----------------------------------------
	// Connectivity feature, 2006.11.01, by shien chang.
	//----------------------------------------
	u1Byte				ExcludedMacAddr[MAX_EXCLUDED_MAC_ADDRESS_LIST][6];
	u4Byte				ExcludedMacAddrListLength;

	// For smart connect. 2006.11.21, by shien chang.
	u1Byte				RejectedAsocAP[MAX_REJECTED_ACCESS_AP][6];
	u1Byte				NumRejectedAsocAP;

	BOOLEAN				bExcludeUnencrypted;
	BOOLEAN				SafeModeEnabled;	

	//For NDIS6 RSNA ,by CCW
	BOOLEAN				bRSNAPSKMode;

	// For turbo mode. Added by Roger, 2006.12.07.
	// -----------------------------------------
	BOOLEAN				bAutoTurboBy8186;		// 8186 "AutoTurbo" mode switch.
	BOOLEAN				bSupportTurboMode;		// 8187 Turbo mode switch.	
	BOOLEAN				bRealtekAggCapExist;
	//The following variables have NOT checked yet....Roger, 2006.12.07
	BOOLEAN				bRealtekCapType1Exist ;		// 8186 "AutoTurbo" mode: 8186 IE exists or not in current association.
	u1Byte				RTIEType2Buffer[7];			// 8186 "AutoTurbo" mode: Realtek IE buffer, used in AsocReq/ReasocReq/ProbeReq. 00-E0-4C-01-02-01-00.

	RT_RF_POWER_STATE	eSwRfPowerState;		// SW RF power state, 2006.12.18, by shien chang.

	// disable tx power+ by rate 
	BOOLEAN				bDisableTXPowerByRate;
	// disable tx power training
	BOOLEAN				bDisableTXPowerTraining;
	BOOLEAN				bSetTXPowerTrainingByOid;
	BOOLEAN				bDecreaseTXPowerTraining;
	
        //
	// For simple config. by CCW - copy from 818x
	//
#if (WPS_SUPPORT == 1)	
	PVOID				pSimpleConfig;
#endif
	BOOLEAN				bPbcPressed;		

	//For alpha LED control
	u1Byte				CustomizedPbcPressTimer;
	u1Byte				CustomizedLedStateOff;   //when > 0, means LED off, and record origin led state

	// Indication when AP not exist.
	BOOLEAN				bIndicateConnectEvent;

	// <Roger_Notes> Sync from 91su branch, 2009.04.27.
	BOOLEAN				bInToSleep;

	u2Byte				ResumeBeaconTime;
	//
	// For 802.11d.
	//
	PVOID				pDot11dInfo;	

	PVOID				pChannelList;	
	
	//
	// IHV Support Type
	//
	IHV_SUPPORT_TYPE	IhvType;
	
	//
	// For CCX related information.
	//
	// Pointer to a RT_RM_CONTEXT object, which encapulats Radio Measurement related stuff. 2006.04.26, by rcnjko.
	PVOID				pRmInfo;
	PVOID				pCcxInfo;

	//For generic parser.
	PVOID				pGenericParser;
		
	// For AP mode information
#if 1//(AP_MODE_SUPPORT == 1)
	PVOID				pApModeInfo;
#endif

#if (P2P_SUPPORT == 1)
	PVOID				pP2PInfo;
#endif

#if (NAN_SUPPORT == 1)
	PVOID				pNanInfo;
#endif

	// For 802.11z
	PVOID				pTDLSTInfo;
			
#if (MULTICHANNEL_SUPPORT == 1)
	PVOID				pMultiChnl;	
#endif

	PVOID				pCustomScanInfo;

	PVOID				pOffChnlTx;

	//1 if sniffer mode. 0 turn off sniffer mode
	BOOLEAN				SnifferTurnOnFlag;
	u4Byte				SnifferSavedRCR;
	
	// Protection mode related
	u1Byte				ForcedProtectionMode;
	u1Byte				ForcedProtectRate;
	u1Byte				ForcedProtectBW;
	u1Byte				ForcedProtectSC;
	u1Byte				ForcedProtectCCA;
	BOOLEAN				bForcedProtectRTSFrame;
	BOOLEAN				bForcedProtectCTSFrame;
	
	// for HT operation rate set.  we use this one for HT data rate to seperate different descriptors
	//the way fill this is the same as in the IE

	BOOLEAN				bReg11nAdhoc;
	BOOLEAN				bRegIOTBcm256QAM;
	u1Byte				bRegVht24g;
		
	BOOLEAN				bScan_20MHz;

	//For Backup Initial Gain
	INIT_GAIN			InitGain_Backup;

	// WiFi Config
	BOOLEAN				bWiFiConfg;

	u1Byte				DM_Type;
	//Dynamic Tx power for near/far range enable/Disable  , by Jacken , 2008-03-06
	BOOLEAN				bDynamicTxPowerEnable;   

	//2008-05-09  indicate set power progress
	BOOLEAN				bSetPnpPwrInProgress;

	//tx rate display added by cosa 06162008
	u1Byte				OSTxRateDisplayType;
	// 06/19/2008 Action frame allowed flag
	u1Byte				CntAfterLink;

	// Firmware information
	u2Byte				FirmwareVersion;
	u2Byte				FirmwareVersionRev;
	u2Byte				FirmwareSubVersion;
	u2Byte				FirmwareSignature;
	
	// For IOT issue.
	u1Byte				IOTPeer;
	u1Byte				IOTPeerSubtype;
	u4Byte				IOTAction;
	u2Byte				IOTMaxMpduLen;
	BOOLEAN				IOTVhtAmsduSupport;
	BOOLEAN				bDmDisableProtect;

	// 2008/05/19 MH Add for NIC verify packet.
	// 2008/05/20 MH For verification console.
#if DBG_CMD
	RT_WORK_ITEM		NICVerify;
#endif	
	BOOLEAN				bNicVerifyPkt;

	// Roam Sensitive Level 
	s1Byte				RegROAMSensitiveLevel;

	// Roam Hysteresis
	u1Byte				RegRoamHysteresis;

	// For leisure power save in connection state, 2007-12-17 by Isaiah
	u1Byte				LPSDelayCnt;
	
	// Keep Alive mechanism
	u1Byte				keepAliveLevel;

	u8Byte				uConnectedTime;
	//TX power Level For Lenovo
	int					TxPowerLevel;

	u8Byte				DelayLPSLastTimeStamp; // Delay to enter LeisurePS when the nic sent DHCP, ARP, or EAPOL packet. By tynli.

	u1Byte				mDeauthCount; // For Deauth link retry

	u1Byte				AntennaTest;
	u1Byte				AntCurIdx;

	// dm control initial gain flag
	BOOLEAN				bDMInitialGainEnable;

	//Record NDIS Version For Compatibile 
	RT_NDIS_VERSION 		NdisVersion;

	RT_TIMER				PnpWakeUpJoinTimer;

	u4Byte				OutstandingNdisPackets;
	u4Byte				CompleteFlag;

	// Indicate event immediately when reciecing deauth/disassoc. By Bruce, 2009-05-19.
	BOOLEAN				IndicateByDeauth;

	u1Byte				ShowRateMode;
	BOOLEAN				bForcedShowRxRate;
	BOOLEAN				bForcedShowRateStill;

	// 2009/08/05 MH To protect halt adapter process.
	BOOLEAN				bHaltInProgress;
	
	//2009/08/10 This is asked by AzWava Mr. Six. To Make TKIP in N mode
	BOOLEAN				bTKIPinNmodeFromReg;
	BOOLEAN				bWEPinNmodeFromReg;
	BOOLEAN				bTKIPinNmode;

	BOOLEAN				bPwrSaveState;
	BOOLEAN				bDriverIsGoingToSleep;

	BOOLEAN				bRegClevoDriver;
	RT_DBG_CONTROL		dbg_ctrl;

	BOOLEAN				bBTMode;
	u1Byte				btHsMode;

	u1Byte				bDefaultAntenna;

	u8Byte				TSFValue; //by tynli, from Rx beacon
	u8Byte				NowTSF;
	u8Byte				NextBeacon;	

	BOOLEAN				bUseRxInterruptWorkItem;

	BOOLEAN				bWakeupAutoLinkInProgressing;

	u4Byte				uLastDecryptedErrorSeqNum;	

	BOOLEAN				bDisableCck;
	BOOLEAN				bRegVelocity;
	u4Byte				AcceptFirstEAPOLPacket;	

	BOOLEAN				bAPTimExtend;
	BOOLEAN				bConcurrentMode;

	BOOLEAN				bDumpRegs;
	u1Byte				dumpcounter;
	u1Byte				NumforRxFastbatch;


	// 2010/05/11 MH Add a timer for GPIO detect.
	// 2011/11/03 MH Use the same GPIO timer.
	RT_TIMER			GpioDetectTimer;	
	BOOLEAN				GpioDetectTimerStart;
	BOOLEAN				HwPBCDetectTimerStart;

	// 2010/05/18 MH Add for GPIO setting.
	u1Byte				bRegTimerGPIO;
	u2Byte				bRegGPIODelay;
	u2Byte				bRegGPIOBack;

	// 2010/08/25 MH Add to support pdn mode switch.
	u1Byte				bRegPDNMode;
	
	// 2010/09/01 MH According to PM's request, we support dongle selective suspend mode switch.	
	u1Byte				bRegDongleSS;
	
	// 2010/09/13 MH According to PM's request, we support different SS power seave level.	
	u1Byte				bRegSSPwrLvl;
	
	// 2011/02/16 MH Add for SS HW radio detect workaround temporarily.
	u1Byte				bRegSSWakeCnt;
	
	// 2010/12/17 MH Add for RX aggregation mode switch according to TX/RX traffic.	
	u1Byte				bRegAggDMEnable;
        BOOLEAN			bSendPacketByTimer;	

	// 2010/12/31 MH Add for UPHY dynamic chaneg.	
	u1Byte				bRegUPDMEnable;

	// 2011/07/08 MH Add for different link speed display.
	u1Byte				bRegLinkSpeedLevel;	

	u1Byte				RSSI2GridMode;
	
	// 2011/07/14 MH Add for rx short cut.	
	u1Byte				bRegRxSC;

	// 2011/07/15 Sinda Add for tx short cut.	
	u1Byte				bRegTxSC;

	// 2011/09/15 MH Add registry for switching packet compete method.
	u1Byte				RegTxMode;

	// 2012/09/14 MH Add for EDCA turbo mode switch threshold.
	// 2012/09/14 MH Add for 88E rate adaptive mode siwtch.
	u2Byte				RegEdcaThresh;
	u2Byte				NumNonBePkt;
	u2Byte				NumBePkt;
	u2Byte				RegRALvl;

	// 2012/10/26 MH Add for 8812/8821 AC series dynamic switch.
	u1Byte				RegAcUsbDmaTime;
	u1Byte				RegAcUsbDmaSize;
	u1Byte				RegAcUsbDmaTime2;
	u1Byte				RegAcUsbDmaSize2;

	// 2012/10/31 MH Add for power limit table constraint.
	u4Byte				RegTxPwrLimit;

	// 2012/11/28 MH Add for BB team AMPDU test.
	u1Byte				RegAMfactor;	
	u1Byte				RegVHTRSec;

	// 2012/11/07 Awk add PowerBase for customers to define their power base.
	u1Byte				RegPowerBase;

	// 2012/11/23 Awk add RegEnablePowerLimit for enabling/disabling power limit
	u1Byte				RegEnableTxPowerLimit;
	u1Byte				RegEnableTxPowerByRate;

	// 2015/02/11 VincentL Add for primary/secondary power limit table selection (used as initial one)
	u1Byte				RegTxPowerLimitTableSel;
	u1Byte				RegTxPwrLmtDynamicLoading;

	// 2015/04/27 VincentL Add for Toggle Support for TxPwrTable Dump Feature
	u1Byte				RegSupportTxPwrTableDump;

	// 2015/02/25 VincentL Add for UEFI method
	u1Byte				RegLoadSystemSKUfromUEFI;
	u1Byte				RegUEFIProfile;

	// 2015/07/07 VincentL Add for SMBIOS method
	u1Byte				RegLoadSystemSKUfromSMBIOS;
	u1Byte				RegLoadProcessorIDfromSMBIOS;

	OCTET_STRING		RegPwrByRateFile;
	OCTET_STRING		RegPwrLimitFile;
	OCTET_STRING		RegSecondaryPwrLimitFile;
	OCTET_STRING		RegChannelPlan2G;
	OCTET_STRING		RegChannelPlan5G;	
	u1Byte				RegDecryptCustomFile;

	// 2013/04/16 VincentLan Add to switch Spur Calibration Method
	u1Byte				RegSpurCalMethod;
	// 2013/01/23 VincentLan Add to enable IQK Firmware Offload
	u1Byte				RegIQKFWOffload;
	// 2013/11/23 VincentLan add for for KFree Feature Requested by RF David
	u1Byte				RegRfKFreeEnable;

	
	
	u1Byte				RegTxDutyEnable;
	
	// 2011/11/15 MH Add for user can select different region and map to dedicated power gain offset table.
	u1Byte				RegPwrTblSel;

	u1Byte				RegTxPwrLevel;

	// 2011/11/15 MH Add for user can select different tx power by rate switch by default value and registry value.
	u1Byte				RegPwrByRate;	
	u4Byte				RegPwrRaTbl1;	
	u4Byte				RegPwrRaTbl2;	
	u4Byte				RegPwrRaTbl3;	
	u4Byte				RegPwrRaTbl4;	
	u4Byte				RegPwrRaTbl5;	
	u4Byte				RegPwrRaTbl6;	
	u4Byte				RegPwrRaTbl7;	
	u4Byte				RegPwrRaTbl8;	
	u4Byte				RegPwrRaTbl9;	
	u4Byte				RegPwrRaTbl10;	
	u4Byte				RegPwrRaTbl11;	
	u4Byte				RegPwrRaTbl12;	
	u4Byte				RegPwrRaTbl13;	
	u4Byte				RegPwrRaTbl14;	
	u4Byte				RegPwrRaTbl15;	
	u4Byte				RegPwrRaTbl16;	

	// 2012/07/24 MH Add for win8 usb whql tst & WFD multi channel.
	u1Byte				RegUseDefaultCID;

	u1Byte				RegWfdTime;
	u1Byte				RegWfdChnl;
	
#if 1//HARDWARE_TYPE_IS_RTL8192D == 1 // Disable by MH
	//for checkEDCATurbo
	u8Byte			lastTxOkCnt ;
	u8Byte			lastRxOkCnt ;
	u8Byte 			Ext_lastTxOkCnt ;
	u8Byte 			Ext_lastRxOkCnt ;
#endif

	// Justin: Timer for Tx/Rx LED blinking
	RT_TIMER		LedTimer;
	
	// Add for USB/PCIE rx packetr indicate header.
	u1Byte			header[100][(sMacHdrLng+N6_LLC_SIZE)];

	BOOLEAN		bDisSwDmaBcn;

	// If WFD is not support, this pointer shall be NULL.
	// Note:
	//	Do not access this variable outside of WFD_xxx() functoions.
	PVOID				pWFDInfo;

	// Just for need report  power save state to OS or not ( Win8 )!! 
	BOOLEAN				bInAutoPowerSavemode;

	u1Byte				RegFakeRoamSignal[3];	// For testing only, RegFakeRoamSignal[0] = initial signal, RegFakeRoamSignal[1] = reducing signal
	u4Byte				RegCapAKMSuite;
	BOOLEAN				bInBIPMFPMode;
	u4Byte				targetAKMSuite;

	FT_INFO_ENTRY		FtEntryList[MAX_FT_ENTRY_NUM];
		
	OCTET_STRING		TIE;
	u1Byte		TIEBuff[8];

#if (WPS_SUPPORT == 1)
	// Add for customized association IE
	BOOLEAN		bCustomizedAsocIE;
	u2Byte		CustomizedAsocIELength;
	u1Byte		CustomizedAsocIEBuf[MAX_IE_LEN];
#endif	

	// For 92D CCK hang issue when dual band.
	BOOLEAN			bDisableCCKForDualBand;
	BOOLEAN			bRestoreCCKForDualBand;

	// TCP Sequence Reorder
	BOOLEAN			bTcpReorderEnable;
#if TCPREORDER_SUPPORT
	TCP_SEQ_CHECKER		TcpReorderChecker;
	TCP_SEQ_REORDER_ENTRY	TcpReorderEntry[MAX_TCP_SEQ_ENTRY_NUM];
	RT_LIST_ENTRY			TcpSeq_Unused_List;
	RT_LIST_ENTRY			TcpSeqList;
	RT_TIMER				TcpSendTimer;
	u4Byte					AvailCnt;
	u4Byte					QueuedCnt;
	BOOLEAN					bPauseTcpReorder;
#endif

	// For tracking last received mgnt frame seq number
	u2Byte				last_auth_seq;
	u2Byte				last_asoc_req_seq;
	u2Byte				last_asoc_rsp_seq;
	u2Byte				last_Disassoc_seq;
	u2Byte				last_Deauth_seq;

	// For memory management to record the objects allocation info.
	RT_LIST_ENTRY		ObjectList;

       // add for stable  traffic check 
	Check_Stable_Traffic           CheckTraffic;
	   
	BOOLEAN		bDisableCCKRateForVWIFIChangeChannel;

	//For GTK/ARP offload. by tynli.
	u1Byte					rx_mic_key[8];
	RT_PM_IPV4_ARP_PARAMETERS PMIPV4ARPPara;
	RT_PM_IPV6_NS_PARAMETERS PMIPV6NSPara[2];
	RT_PM_DOT11_RSN_REKEY_PARAMETERS PMDot11RSNRekeyPara;
	
	// Add for TCP_CHECK
	RT_SUPPORT_TCPOFFLOAD_CAP		TCPOffloadCap;
	RT_SUPPORT_TCPOFFLOAD_CAP		CurrTCPOffloadCap;
	RT_SUPPORT_TCPOFFLOAD_MODE	RegSupTCPOffload;
	RT_SUPPORT_TCPOFFLOAD_MODE	CurSupTCPOffload;

	// For Ad-hoc mode which is not support WoWLAN. 2011.09.14. by tynli.
	BOOLEAN				bIbssBeforeSleep;

	// For LC
	BOOLEAN				bStopScan;
	BOOLEAN				bReceiveSystemPSOID;		// LC S5 WoWLAN

	// For Intel Bay Trail low power platform.
	BOOLEAN				bIntelPatchPNP;

	//for PCIe Bus work on 2.5GHz or 5GHz  by gw
	BOOLEAN                    ForcePCIeRate;

	// ending
	u1Byte				TestAID;

	//sherry added for support txpacketbuffercolease for 8192DU-VC
	BOOLEAN		bSupportTxPacketBufferColease;
	//Sherry added to allow user disable 1 band in advanced setting of device management
	u1Byte				CurrentWirelessBand;

   	BOOLEAN     bSMSPSupport1by1;

	// Add for Real WOW
	TEREDOINFO			dTeredoInfo;

	// Add S5 WPA Re-Key !!
	u1Byte				mbS5SNose[32];
	u1Byte				mbS5ANose[32];
	char				mbPassphrase[PASSPHRASE_LEN];
	u4Byte				mPasspharseLen;
	u1Byte				mbS5PMK[PMK_LEN];
	u1Byte				mbS5PTK[PTK_LEN_TKIP];

	// Add for D0 filter coalesci 
	RT_DO_COALESICING_FILTER_INFO		mRtD0ColesFilterInfo;

	BOOLEAN				bEDCCASupport;

	u1Byte				RegNByteAccess;  //20121106 page add for Nbyte access BB reg switch
	u1Byte				RegFWOffload;
	u1Byte				RegDownloadFW;

	CHANNEL_WIDTH		currentRABw;
	
	u2Byte				RegScanLarge;
	u2Byte				RegScanMiddle;
	u2Byte				RegScanNormal;
	u2Byte				RegScanActive;
	u2Byte				RegForcedScanPeriod;

	u2Byte				RegPreferBand;

	// Pre-transition for OID_PNP_SET_POWER OID wake up handling, added by Roger, 2012.11.28.
	BOOLEAN		bPreTransPnP;

	BOOLEAN 	bDisconnectInProgress; //indicate disconnect in progress 20130117
	
	//20130117 Used for Invitation No/After to Pass, with skip scan when Go is connected
	u1Byte		bGoSkipScanForWFD;
	u1Byte		bClientSkipScanForWFD;
	
	BOOLEAN		bForceGoTxData20MBw;
	u1Byte		bWaitBeforeGoSkipScan;
	

	BOOLEAN				bHWRTSEnable;

	u1Byte		ScanOffloadType;
	
	// 2013/02/05 MH Add for streamMode switch.
	u1Byte				RegStreamMode;
	// 2013/02/06 MH Add Transmit power control level for all customer in the future.
	u1Byte				RegTPCLvl;
	u1Byte				RegTPCLvlD;
	u1Byte				RegTPCLvl5g;
	u1Byte				RegTPCLvl5gD;

	u1Byte				RegEnableAdaptivity;
	u1Byte				RegL2HForAdaptivity;
	u1Byte				RegHLDiffForAdaptivity;
	u2Byte				RegEnableCarrierSense;
	u2Byte				RegNHMEnable;
	u1Byte				RegDmLinkAdaptivity;
	u1Byte				RegDCbackoff;
	u1Byte				RegAPNumTH;

	BOOLEAN				RegPacketDrop;

	u1Byte				EnableResetTxStuck;

	u1Byte				RegSifsThresh;
	
	u1Byte				RegFwload;	

	u1Byte				RegUsbSafetySwitch;

	u1Byte				RegRspPwr;

	//20130220 ylb Add for 8814A FPGA Verification.
	u1Byte				Reg8814AFPGAVerification;
	u1Byte				bRegAdhocUseHWSec;

	//20140526 haiso_ho Add for 8822B FPGA Verification.
	u1Byte				Reg8822BFPGAVerification;

	BOOLEAN 	RegClearAMDWakeUpStatus; // zhiyuan add for 88ee Lenovo AMD platform
	
	BOOLEAN		RegbCustomizedScanPeriod;
	u2Byte		RegIntelCustomizedScanPeriod;
	u2Byte		RegAMDCustomizedScanPeriod;

	u2Byte		RegDisableBTCoexist;

	RT_LED_ASSOC_STATE	LEDAssocState;

	BOOLEAN		EnableMA;// For Lenovo Mutual Authentication function, added by Roger, 2013.05.03.

	u1Byte				VhtWeakSecurity;
	u1Byte				RegPassiveScan;

	BOOLEAN		bSupportPacketCoalescing;
	BOOLEAN 	IsAMDIOIC;	
	
	BOOLEAN		bPendScanOID;
	BOOLEAN		bCheckScanTime;
	u8Byte		ScanOnlyStartTime;	
	u1Byte		RegMultiChannelFcsMode;
	BOOLEAN		bFcsUpdateTSF;
	u1Byte		WFDOpChannel;

	u4Byte		RegWmmPage;
	u1Byte		WFDPeerOpChannel;
	u2Byte		ConnectionConfigTimeIntv;	
	u4Byte				Regbcndelay;
	BOOLEAN		bWPSProcess;
	u1Byte		RetryTimes;
	u4Byte		RegPktIndicate;

	u1Byte		RegDisableAC;
	u4Byte		RegLedInterval;
	u1Byte		Reg8814auEfuse;
	u1Byte		RegValidRFPath;

	// Network list offload info.
	RT_NLO_INFO			NLOInfo;

#if(AUTO_CHNL_SEL_NHM == 1)
	//
	// Auto Channel Selection mechanism for P2P GO, added by Roger, 2014.05.15.
	//
	RT_AUTO_CHNL_SEL		AutoChnlSel;
#endif

	BOOLEAN		bDisableRtkSupportedP2P;

	u1Byte		DumpDbgRegCnt;

	// For AOAC reconnection policy. 2014.11.05, added by tynli.
	BOOLEAN		bPerformPnpReconnect;
	BOOLEAN		bRegPnpKeepLink;

	// SDIO polling interrupt mode
	BOOLEAN 	bSdioPollIntrHandler;
	u2Byte		SdioIntrPollingLimit;
    
    // For Rx Reorder Control
    u1Byte				RegRxReorder;
    u1Byte				RegRxReorder_WinSize;
    u1Byte				RegRxReorder_PendTime;
   u1Byte			RegPreInitMem;
   u1Byte				Reg88EIOTAction;


	u2Byte		RegTxSendAsap;
	u2Byte		CurTxSendAsap;

	// MAC Address Randomization 
	u1Byte		RegSupportMACRandom;

	//ECSA
	u1Byte		RegSupportECSA;

	// FT
	u1Byte		RegSupportFT;

	// authentication frame
	OCTET_STRING		authReqframe;
	OCTET_STRING		authRspframe;

	u1Byte		RegSuspendTimerInLowPwr;
}MGNT_INFO, *PMGNT_INFO;

typedef struct _IRP_DBG_INFO{
	// For IRP buf/queue initialize info
	BOOLEAN				bListInit;
	BOOLEAN				bAmpAsocBuf;
	BOOLEAN				bBTLocalBufPtr;
	u4Byte				BTLocalBufAllocCnt;
	u4Byte				BTLocalBufFreeCnt;
}IRP_DBG_INFO, *PIRP_DBG_INFO;


#define 	MPHAL_INBUF_SIZE			100
#define 	MPHAL_OUTBUF_SIZE			100

typedef VOID (*DbgActFunc)(IN	PVOID		Adapter);
typedef struct _RT_NDIS_DBG_CONTEXT
{
	BOOLEAN			bDbgDrvUnload;

	// Work Item for debugging. 
	RT_WORK_ITEM 	DbgWorkItem;
	PlatformEvent	DbgWorkItemEvent;
	NDIS_SPIN_LOCK	DbgWorkItemSpinLock; // To protect the following variables.
	BOOLEAN 		bDbgWorkItemInProgress; // Indicate a DbgWorkItem is scheduled and not yet finished.

	DbgActFunc CurrDbgAct; // An instance which implements function and context of DbgWorkItem.

	// Variable needed in each implementation of CurrDbgAct.
	ULONG	DbgActType; // Type of action performed in CurrDbgAct.
	ULONG	DbgIoOffset; // The type of IO operation is depend of DbgActType.  
	ULONG	DbgIoValue; // The type of IO operation is depend of DbgActType.
	ULONG	DbgOutLen;
    u1Byte	DbgOutBuf[100];
	UCHAR	DbgIoBuf[64];
	ULONG	DbgRfPath;	// The RF path of IO operation. Added by Roger, 2008.09.10.

	//cosa add for bt control	
	// Event used to sync H2c for BT control
	PlatformEvent	dbgH2cRspEvent;
	PlatformEvent	dbgBtC2hEvent;
	PlatformEvent	dbgBtCoexEvent;
	u1Byte			h2cReqNum;
	u1Byte			btInBuf[100];	
} RT_NDIS_DBG_CONTEXT, *PRT_NDIS_DBG_CONTEXT;
//
// Reason code for surprisely removing.
//
#define HALT_BY_TX			0x01000000
#define HALT_BY_TX_REQUEST	0x01000001
#define HALT_BY_TX_COMPLETE	0x01000002
#define HALT_BY_RX			0x02000000
#define HALT_BY_RX_REQUEST	0x02000001
#define HALT_BY_RX_COMPLETE	0x02000002
#define HALT_BY_IO			0x04000000
#define HALT_BY_IO_SYNC		0x04000001
#define HALT_BY_IO_ASYNC	0x04000002
#define HALT_BY_PNP_EVENT	0x08000000
#define HALT_BY_DRV_STOP	0x10000000

//
// Function disabled.
//
#define DF_TX_BIT		BIT0
#define DF_RX_BIT		BIT1
#define DF_IO_BIT		BIT2
#define DF_IO_D3_BIT			BIT3

#define RT_DF_TYPE		u4Byte
#define RT_DISABLE_FUNC(__pAdapter, __FuncBits) ((__pAdapter)->DisabledFunctions |= ((RT_DF_TYPE)(__FuncBits)))
#define RT_ENABLE_FUNC(__pAdapter, __FuncBits) ((__pAdapter)->DisabledFunctions &= (~((RT_DF_TYPE)(__FuncBits))))
#define RT_IS_FUNC_DISABLED(__pAdapter, __FuncBits) ( (__pAdapter)->DisabledFunctions & (__FuncBits) )

//
// Driver State
//
#define ADAPTER_STATUS_FIRST_INIT				0x00000001
#define ADAPTER_STATUS_WAKE_UP				0x00000002
#define ADAPTER_STATUS_FW_DOWNLOAD_FAILURE	0x00000004
#define ADAPTER_STATUS_BB_CONFIG_FAILURE		0x00000005 //added by vivi, bb config failure

#define ADAPTER_SET_STATUS_FLAG(_M, _F)            PLATFORM_INTERLOCKED_SET_BITS(&((_M)->Status), (_F))
#define ADAPTER_CLEAR_STATUS_FLAG(_M, _F)        PLATFORM_INTERLOCKED_CLEAR_BITS(&((_M)->Status), (_F))
#define ADAPTER_TEST_STATUS_FLAG(_M, _F)          (((_M)->Status & (_F)) != 0)

#if 0
typedef enum _DUAL_MAC_SCENARIOS_TYPE{
	No_Scenarios = 0,
	STA_STA = 1,
	STA_AP	 = 2,
	STA_P2P = 3
}DUAL_MAC_SCENARIOS_TYPE,*PDUAL_MAC_SCENARIOS_TYPE;


typedef enum _DUAL_MAC_EASY_COCURRENT_CHECK_STATE{
	RT_DO_NOTHING = 0,
	RT_NEED_RETRY = 1,
	RT_NEED_RESCHEDULE = 2
}DUAL_MAC_EASY_COCURRENT_CHECK_STATE,*PDUAL_MAC_EASY_COCURRENT_CHECK_STATE;

typedef enum _DUAL_MAC_TWO_STA_CONCURRENT_MODE{
	RT_NO_SET = 0,
	RT_SECONDARY_DISABLE = 1,
	RT_SECONDARY_ENABLE = 2,
	RT_SECONDARY_AUTO = 3
}DUAL_MAC_TWO_STA_CONCURRENT_MODE,*PDUAL_MAC_TWO_STA_CONCURRENT_MODE;


typedef struct _DUAL_MAC_DMSP_CONTROL{
	BOOLEAN			bActiveScanForSlaveOfDMSP;
	BOOLEAN			bScanForAnotherMacForDMSP;
	BOOLEAN			bScanForItSelfForDMSP;
	BOOLEAN			bWriteDigForAnotherMacOfDMSP;
	u4Byte			CurDigValueForAnotherMacOfDMSP;
	BOOLEAN			bChangeCCKPDStateForAnotherMacOfDMSP;
	u1Byte			CurCCKPDStateForAnotherMacOfDMSP;
	BOOLEAN			bChangeTxHighPowerLvlForAnotherMacOfDMSP;
	u1Byte			CurTxHighLvlForAnotherMacOfDMSP;
	s4Byte			RssiValMinForAnotherMacOfDMSP;
}DUAL_MAC_DMSP_CONTROL,*PDUAL_MAC_DMSP_CONTROL;

typedef struct _DUAL_MAC_EASY_SMART_CONCURRENT_CAHANEL_SETTING_CONTROL{
	BOOLEAN		bConfiguredByUser;
	BAND_TYPE	BandToSet;
	u1Byte		ChannelNumberToSet;
	BOOLEAN		bChangeChannelOnly;
}DUAL_MAC_EASY_SMART_CONCURRENT_CAHANEL_SETTING_CONTROL,*PDUAL_MAC_EASY_SMART_CONCURRENT_CAHANEL_SETTING_CONTROL;

typedef struct _DUAL_MAC_EASY_CONCURRENT_CONTROL{
	BOOLEAN			bConfigurationByUI;
	BOOLEAN			bRegSTAAndSTAEasySmartConcurrentSupport;
	u1Byte			RegTwoStaConcurrentMode;
	u1Byte			CurrentTwoStaConcurrentMode;
	BOOLEAN			bSupportSTAAndSTASmartConcurrent;
	BOOLEAN			bSupportSTAAndAPSmartConcurrent;
	BOOLEAN			bSkipChangeMacPhyMode;
	BOOLEAN			bDisableCCKRateDuringMacPhyModeChange;
	RT_WORK_ITEM	DualMacEasyConcurrentWorkItem;
	RT_TIMER		DualMacEasyConcurrentRetryTimer;
	BOOLEAN			bJoinRequestReschedule;
	BOOLEAN			bJoinRequestRescheduleRetry;
	BOOLEAN			bRestartAPRescheduleRetry;
	BOOLEAN			bRestartAPReschedule;
	BOOLEAN			bNeedSendMimoPs;
	BOOLEAN			bNeedRestartAP;
	u1Byte			BandSetBackForDMDP;
	u1Byte			CurrentBandTypeBackForDMDP;
	u1Byte			ConnectionChannelWidthBack;
	u1Byte			ConnectionChannelOffsetBack;
	u1Byte			DualMacScenariosToSet;
	BOOLEAN			bCloseBBAndRFForDMSP;
	BOOLEAN			bChangeToDMDP;
	BOOLEAN			bChangeToDMSP;
	BOOLEAN			bSwitchInProcess;
	u1Byte			JoinAction;
	RT_WLAN_BSS	BssDesc;	
	BOOLEAN			bTmpBssDesc;
	BOOLEAN			bLinkWithNAP;
	BOOLEAN			bRestartAPForMac0ConnectOnDmsp;  
	BOOLEAN			bLinkInProgress;
	BOOLEAN			bDynamicSwitchToDmspInProgress;
	BOOLEAN			bDynamicSwitchToDmspRetry;
	BOOLEAN			bChangeTwoStaConcurrentModeInProgress;
	BOOLEAN			bChangeTwoStaConcurrentModeRetry;
	BOOLEAN			bAutoSelChnl;
	BOOLEAN			bTxPacketBufferColease;
	BOOLEAN			bMacReconfig;
	BOOLEAN			bMacReconfigIsInProgress;
	BOOLEAN			bOnlyMacConfigToDisassembleTxPacketBuffer;
}DUAL_MAC_EASY_CONCURRENT_CONTROL,*PDUAL_MAC_EASY_CONCURRENT_CONTROL;
#endif
typedef	enum _RT_RF_TYPE_DEFINITION
{
	RF_1T2R			= 0,
	RF_2T4R			= 1,
	RF_2T2R			= 2,
	RF_1T1R			= 3,
	RF_2T2R_GREEN	= 4,
	RF_3T3R			= 5,
	RF_4T4R			= 6,
	RF_2T3R			= 7,
	RF_3T4R			= 8,

	RF_MAX_TYPE     = 0xF, // u1Byte
}RT_RF_TYPE_DEF_E;

// 2010/12/27 Move from type.h to here.
typedef enum _BAND_TYPE{
	BAND_ON_2_4G = 0,
	BAND_ON_5G ,
	BAND_ON_BOTH,
	BANDMAX
}BAND_TYPE,*PBAND_TYPE;


typedef struct _PORT_COMMON_INFO
{

	PADAPTER pDefaultAdapter;


	// ================================================================================================
	// Extracted From RT_NDIS_COMMON: pNdisCommon
	// ================================================================================================


	// Handle All Software Timer ---------------------------------------------------------
	RT_SPIN_LOCK		TimerLock; 					// For timer synchronization
	u4Byte				uNumberOfAllocatedTimers;	// Number of Allocated RT_TIMER objects
	u4Byte				uNumberOfReleasedTimers;	// Number of Released RT_TIMER objects
	s4Byte				ActiveTimerCallbackCount;	// Default 1, only 0 when in Halt state
	RT_EVENT			AllTimerCompletedEvent;		// Notification of all timer completion

#if (DEBUG_RESOURCE_TIMER == 1)
	DEBUG_RESOURCE_TIMER_MONITOR	ResourceTimerMonitor[DEBUG_RESOURCE_TIMER_MONITOR_SIZE];	// For Debugging Purposes
#endif
	// -------------------------------------------------------------------------------


	// ================================================================================================


		PPORT_HELPER	pPortHelper;	
	
	// MacId Context Place Holder for Private Data: ------------------------------------
	//	+ Only access this private data inside the module by MacIdGetCommonContext()
	PRIVATE_DATA_ZONE MacIdCommon[HAL_MAC_ID_SIZE_OF_COMMON_CONTEXT];
	// -------------------------------------------------------------------------------

	// ActionTimer Context Place Holder for Private Data: ------------------------------------
	//	+ Only access this private data inside the module by ActionTimerGetCommonContext()
	PRIVATE_DATA_ZONE ActionTimerCommon[ACTION_TIMER_SIZE_OF_COMMON_CONTEXT];
	// -------------------------------------------------------------------------------

	
#if (MULTIPORT_SUPPORT == 1)

	// MultiPort Common Context Place Holder for Private Data: ----------------------------
	//	+ Only access this private data inside the module by MultiPortGetCommonContext()
	PRIVATE_DATA_ZONE MultiPortCommon[MULTIPORT_SIZE_OF_COMMON_CONTEXT];
	// ---------------------------------------------------------------------------


#if (MULTICHANNEL_SUPPORT == 1)

	// MultiChannel Common Context Place Holder for Private Data: ----------------------------
	//	+ Only access this private data inside the module by MultiChannelGetCommonContext()
	PRIVATE_DATA_ZONE MultiChannelCommon[MULTICHANNEL_SIZE_OF_COMMON_CONTEXT];
	// -------------------------------------------------------------------------------
	
#endif
#endif



	WDI_DATA_STRUCT		WdiData;

	PCHANNEL_COMMON_CONTEXT	pChnlCommInfo;

	RT_LIST_ENTRY			TimerResourceList; // Timer resource list for further handling
	u4Byte					uNumberOfInsertedTimerRes; // Number of Timer resource has been inserted for handling
	
} PORT_COMMON_INFO, * PPORT_COMMON_INFO;

typedef enum
{
	RT_ERR_SUCCESS = 0,
	RT_ERR_DRIVER_FAILURE,
	RT_ERR_IO_FAILURE,
	RT_ERR_WI_TIMEOUT,
	RT_ERR_WI_BUSY,
	RT_ERR_BAD_FORMAT,
	RT_ERR_INVALID_DATA,
	RT_ERR_NOT_ENOUGH_SPACE,
	RT_ERR_WRITE_PROTECT,
	RT_ERR_READ_BACK_FAIL,
	RT_ERR_OUT_OF_RANGE
} RT_ERROR_CODE;

typedef struct _RT_PEER_ENTRY {
	BOOLEAN			bUsed;
	u1Byte			uPeerId;
	u2Byte			uPortId;
	u1Byte			PeerAddr[6];
} RT_PEER_ENTRY, *PRT_PEER_ENTRY;


typedef struct _ADAPTER{
	RT_SDIO_DEVICE			NdisSdioDev;
	PRT_NDIS_COMMON		pNdisCommon;

		PRT_NDIS62_COMMON	pNdis62Common;
	
	//=====================================================
	// Please put tx related variables in TX_GENERAL.
	//=====================================================
	TX_GENERAL			txGen;
	//=====================================================

	// MultiPort Architecture ======================================================
	//	- Final Goal: All Adapter's Common Information should be merged into this structure.
	// =======================================================================
	PPORT_COMMON_INFO	pPortCommonInfo;
	// --------------------------------------------------------------------------------


#if (MULTIPORT_SUPPORT == 1)

	// MultiPort Port Context Place Holder for Private Data: ----------------------------
	//	+ Only access this private data inside the module by MultiPortGetPortContext()
	PRIVATE_DATA_ZONE MultiPort[MULTIPORT_SIZE_OF_PORT_CONTEXT];
	// ------------------------------------------------------------------------

#if (MULTICHANNEL_SUPPORT == 1)

	// MultiChannel Port Context Place Holder for Private Data: ---------------------------
	//	+ Only access this private data inside the module by MultiChannelGetPortContext()	
	PRIVATE_DATA_ZONE MultiChannel[MULTICHANNEL_SIZE_OF_PORT_CONTEXT];
	// --------------------------------------------------------------------------
	
#endif
#endif


	BOOLEAN 		bMPMode;
	RT_ERROR_CODE		LastError; // <20130613, Kordan> Only the functions associated with MP records the error code by now.

	RT_NDIS_DBG_CONTEXT	ndisDbgCtx;

	NDIS_INTERRUPT_TYPE  			InterruptType;
	PIO_INTERRUPT_MESSAGE_INFO		MessageInfoTable;


	BOOLEAN 			bIMRDisable;

	BOOLEAN				bvWifiStopBeforeSleep;	
	BOOLEAN				bDriverStopped;
	BOOLEAN				bSurpriseRemoved;
	u4Byte				SurpriseRemovedReason;
	u1Byte    			MaxIRPPengding;
	u1Byte    			TXPacketShiftBytes;
	u1Byte				HWDescHeadLength;
	u1Byte                        Addr_ePHY;


	u2Byte				MAX_NUM_RFD;
	u2Byte				MAX_NUM_RX_DESC;	
	u2Byte				MAX_RECEIVE_BUFFER_SIZE;
	u2Byte				MAX_SUBFRAME_TOTAL_COUNT;

	u1Byte	 			TotalCamEntry;
    u1Byte 				HalfCamEntry;
   	u1Byte 				CamExtApMaxEntery;
   	u1Byte 				CamExtApP2pStartIndex;
   	u1Byte 				CamExtApP2pMaxEntry;
   	u1Byte 				CamExtStaPairwiseKeyPosition;
   	u1Byte 				CamBtStartIndex;
   	u1Byte 				BtHwCamStart;

	u2Byte				RT_TCB_NUM;
	u2Byte				RT_LOCAL_BUF_NUM;
	u2Byte				RT_LOCAL_FW_BUF_NUM;
	u2Byte				RT_TXDESC_NUM;
	u2Byte				RT_TXDESC_NUM_BE_QUEUE;
	u2Byte				MAX_TRANSMIT_BUFFER_SIZE;
	BOOLEAN				bEarlyQueueTimeout;
	//
	// Union of functions disabled. 
	// Each bit indicated one function that is disabled, such as Tx, Rx, IO.
	// See DF_XX_BIT and related operation defined above.
	//
	RT_DF_TYPE			DisabledFunctions;

	HAL_INTERFACE		HalFunc;
	PVOID				HalData;
	PVOID				pHalBusInfo;

	u2Byte				HardwareType;
	u1Byte				EepromAddressSize;
	u1Byte				PermanentAddress[6];
	u1Byte				CurrentAddress[6];

	WIRELESS_MODE		RegOrigWirelessMode;
	WIRELESS_MODE		RegWirelessMode;
	HT_MODE				RegHTMode;
	
	BOOLEAN				bFWReady;
	BOOLEAN				bHWInitReady;	
	BOOLEAN				bSWInitReady;
	BOOLEAN				bInitializeInProgress;
	BOOLEAN				bInitComplete;

//===========================================
	//2 Transmit related variables
	BOOLEAN				bTxLocked;	// For debugging RT_TX_SPINLOCK. Note that! we SHOULD ONLY change it at PlatformXXXSpinLock(). 2005.11.17, by rcnjko.
	VIRTUAL_MEMORY		TcbBuffer;
	u2Byte				NumTcb;
	u2Byte				NumIdleTcb;
	u2Byte				NumTxDesc[MAX_TX_QUEUE];
	u2Byte				AvailableDescToWrite[MAX_TX_QUEUE];
	u2Byte				CurrentTXWritePoint[MAX_TX_QUEUE];
	u2Byte				CurrentTXReadPoint[MAX_TX_QUEUE];
	//=======================================
	// debug use
	u4Byte				sameTxReadPointCnt;
	u2Byte				CurrentTxReadPointBeQLogIndex;
	u2Byte				CurrentTxReadPointBeQLog[10];
	u2Byte				CurrentTxWritePointBeQLog[10];
	u8Byte				updateBeQReadPointTime[10];
	u8Byte				latestUpdatedTxDescTime;
	u4Byte				wrongrwTxpointCnt;
	
	u4Byte				abnormalAvaiDescLogIndex[MAX_TX_QUEUE];
	u2Byte				abnormalCurrentTXWritePoint[MAX_TX_QUEUE][10];
	//=======================================
	RT_LIST_ENTRY		TcbIdleQueue;
	RT_LIST_ENTRY		TcbWaitQueue[MAX_TX_QUEUE];
	u4Byte				TcbWaitQueueCnt[MAX_TX_QUEUE];

	RT_LIST_ENTRY		TcbBusyQueue[MAX_TX_QUEUE];
	RT_LIST_ENTRY		TcbAggrQueue[MAX_TX_QUEUE]; // For A-MSDU aggreagtion, Joseph
	u4Byte				TcbCountInAggrQueue[MAX_TX_QUEUE];
	RT_TX_AGG_COALES_BUF	TcbAggCoalesBuf[MAX_TX_QUEUE];

	u2Byte				NextTxDescToFill[MAX_TX_QUEUE];
	u2Byte				NextTxDescToCheck[MAX_TX_QUEUE];
	u2Byte				nBufInTxDesc[MAX_TX_QUEUE];
	u1Byte				nFragDescUsed;
	
	// For WIN7 Context switch
	LIST_ENTRY			HVLContextLinked;	

	//2 Receive related variables
	BOOLEAN				bRxLocked;	// For debugging RT_RX_SPINLOCK. Note that! we SHOULD ONLY change it at PlatformXXXSpinLock(). 2005.11.17, by rcnjko.
	VIRTUAL_MEMORY		RfdBuffer;
	u2Byte				NumRfd;
	u2Byte				NumIdleRfd;
	u4Byte				NumBusyRfd[MAX_RX_QUEUE];
	u2Byte				NumRxDesc[MAX_RX_QUEUE];
	RT_LIST_ENTRY		RfdIdleQueue;
	RT_LIST_ENTRY		RfdBusyQueue[MAX_RX_QUEUE];
#if( WDI_SUPPORT == TRUE )
	PNDIS_RECEIVE_THROTTLE_PARAMETERS pThrottle;
	BOOLEAN				bFirstMpduOfInterrupt;
#endif
	
	u2Byte				NextRxDescToFill[MAX_RX_QUEUE];
	u2Byte				NextRxDescToCheck[MAX_RX_QUEUE];
	u2Byte				nBufInRxDesc[MAX_RX_QUEUE];
	DEFRAG_ENTRY		DefragArray[MAX_DEFRAG_PEER];
	u2Byte				LowRfdThreshold;
	u4Byte				RcvRefCount;  // number of packets that have not been returned back
	PALIGNED_SHARED_MEMORY	pAMSDU; // Copy subframe of AMSDU into one contineous buffer before indication (Vista only)
	u2Byte				AmsduIndex;


	//2 Transmit local buffer
	u2Byte				NumLocalBuffer;
	u2Byte				NumLocalBufferIdle;
	u2Byte				NumLocalFWBuffer;	// For download Firmware
	u2Byte				NumLocalFWBufferIdle;	//tynli_test
	VIRTUAL_MEMORY		LocalBufferArray;
	VIRTUAL_MEMORY		LocalFWBufferArray;	// For download Firmware
	RT_LIST_ENTRY		LocalBufferQueue;	
	RT_LIST_ENTRY		LocalFWBufferQueue;	// For download Firmware

	//2 Receive local buffer
	pu4Byte				RfdListAddr[MAX_RFD_ARRAY_NUM + 1];
	pu4Byte				RfdVirtualAddr[MAX_RFD_ARRAY_NUM + 1];
#if RX_AGGREGATION
	RT_RFD				RfdTmpArray[MAX_PKT_AGG_NUM];
	RT_LIST_ENTRY		RfdTmpList;
#endif


	//2 Statictics
	RT_TX_STATISTICS	TxStats;
	RT_RX_STATISTICS	RxStats;
	u4Byte			numInterrupt;

	//2 General Templorary Buffer
	VIRTUAL_MEMORY		GenTempBufferArray;		// for General Templorary Buffer
	RT_LIST_ENTRY		GenTempBufferQueue;	
	u4Byte				NumGenTempBufferIdle;
	u1Byte				TempBufferforcoalsece[2000];

	//2 Management Information
	MGNT_INFO			MgntInfo;
	u1Byte				MCList[MAX_MCAST_LIST_NUM][6];
	u4Byte				MCAddrCount;
	RT_WLAN_BSS			bssDescList[MAX_BSS_DESC];
	u4Byte 				AP_EDCA_PARAM[4];
	u4Byte 				STA_EDCA_PARAM[4];

	// MISC.	
	u8Byte				DriverUpTime; // Yves: Driver up time.
	u8Byte				lastscantime;//lastscantime
	u8Byte				LastScanCompleteTime;	// Last scan complete time.
	u8Byte				LastConnectStartIndicationTime;
	u8Byte				LastRoamingStartIndicationTime;
	u8Byte				LastConnectionActionTime;

	// For PnP Working Time Calculation
	BOOLEAN				bCtrlPnPTime;
	LARGE_INTEGER		PnPTotalTime;
	LARGE_INTEGER		PnPIOTime;

	// For HCT test, 2005.07.15, by rcnjko.
	BOOLEAN				bInHctTest;	
	BOOLEAN				bInWFDTest;	
	BOOLEAN				bScanTimeCheck;	
	BOOLEAN				bFixedMacAddr;
	
	BOOLEAN				bReduceImr;
	
	BOOLEAN				bInChaosTest;
	BOOLEAN 			bDisable11ac;

	// For USB IF test, 060505, by rcnjko.
	BOOLEAN				bInUsbIfTest;
	
	//2 System Reset
	RESET_TYPE			ResetProgress;
	u4Byte				ResetCount;
	BOOLEAN				bForcedSilentReset;
	BOOLEAN				bResetInProgress;

	// General Adapter Status
	u4Byte				Status;

	// Tx/Rx LED
	u2Byte				LedTxCnt;
	u2Byte				LedRxCnt;

	//
	// Pointer to the memory block storing pointers to DRV_LOG_POOL_T objects.
	// Each log pool is corresponding to one type of log.
	// Use GET_DRV_LOG_POOL() and SET_DRV_LOG_POOL() to manipulate log pool, 
	// see LogMgnt.c for details.
	//
#if DRV_LOG
	VOID**				ppLogPools; 

#endif

	//2 Power related variables
	BOOLEAN   			bInSetPower;
	BOOLEAN 			bEnterPnpSleep; // Just for S3/S4 !!!
	BOOLEAN 			bWakeFromPnpSleep;	// Just for S3/S4 !!!
	BOOLEAN				bUnloadDriverwhenS3S4;
	BOOLEAN				bDriverIsGoingToUnload;
	BOOLEAN				bHWSecurityInWoL; //by tynli
	BOOLEAN				bDriverIsGoingToPnpSetPowerSleep;
	BOOLEAN				bIntoIPS;
	BOOLEAN				bReceiveCpwmInt;
	BOOLEAN				bDriverShutdown;
	u4Byte				ipsEnterCnt;
	BOOLEAN				bInWoWLANPowerState;
	BOOLEAN				bReInitHW; // record

	// Interrupt debug message	
	u4Byte				handleIntFailCnt;
	u8Byte				lastHandleIntFailTime[10];
	u8Byte				preHandlIntTime;
	u8Byte				curHandleIntTime;
	u4Byte				intLogTimeIndex;
	u8Byte				handleIntTime[10];

	// Tx hang debug info 
	HW_REG_VALUE		txMacReg[MAX_TX_REG_NUM];
	HW_REG_VALUE		txBbReg[MAX_TX_REG_NUM];
	BOOLEAN				bTxHangCheck;
	u1Byte				dropPktByMacIdCnt;
	u8Byte				txHangHwCollectTime;
	u8Byte				firsttxHangHwTime;
	BOOLEAN				bTcbBusyQEmpty[MAX_TX_QUEUE];
	u8Byte				firstTcbSysTime[MAX_TX_QUEUE];
	u4Byte				txOwnCloseTimeIndex[MAX_TX_QUEUE];
	u8Byte				curTxOwnCloseTime[MAX_TX_QUEUE];
	u8Byte				txOwnCloseTime[MAX_TX_QUEUE][10];

	// Rx hang debug info
	HW_REG_VALUE		rxMacReg[MAX_RX_REG_NUM];
	HW_REG_VALUE		rxBbReg[MAX_RX_REG_NUM];
	BOOLEAN				bRxHangCheck;
	u8Byte				rxHangHwCollectTime;

	// Tx/Rx hang debug info
	HW_REG_VALUE		rfReg[MAX_TX_REG_NUM];


	BOOLEAN				bInitByExtPort;
	BOOLEAN				bSilentReseted;
	BOOLEAN				initfinish;

	BOOLEAN				bStartVwifi;

	//Rx reorder related Reference variables	
	u4Byte				rxReorderRefCount;
	u4Byte				rxReorderIndEnterCnt;
	u4Byte				rxReorderIndAllowCnt;
	u4Byte				rxReorderIndRejectCnt[3];

	//2 Interrupt related variables
	u4Byte				IntrTxRefCount;
	u4Byte				IntrRxRefCount;
	u4Byte				IntrCCKRefCount;
#if TCPREORDER_SUPPORT
	u4Byte				IntrTcpSeqRefCount;
#endif

	u4Byte				IntrNBLRefCount;

	u4Byte				IntrInterruptRefCount;
	u4Byte				RxPktPendingTimeoutRefCount;
	
	BOOLEAN				bIntInProgress; // Interrupt handling is in progress
	PlatformSemaphore	InterruptSema;	//For interrupt sync flow control
	

	// Interrupt Thread
	BOOLEAN				bUseThreadHandleInterrupt;
	RT_THREAD			InterruptThread;
	u8Byte				InterruptThreadCnt;
	BOOLEAN				bDPCISRTest;

	//
	// We need to declare the strcute in other place later!!!
	//
	// 2010/12/25 MH Add for _DFS support flag.
	// 2010/12/27 MH Add for Dual Mode switch support flag
	// 2010/12/28 MH Add for Dual Mode smart concurrent support flag
	// 2010/12/30 MH Add WAPI and other fucuter support support flag.	
#ifdef REMOVE_PACK
#pragma pack(1)
#endif
	UINT32		DFSSupport:1;				// 802.11h support
	//UINT32		DualMacSupport:1;			// dual mac switch support
	//UINT32		DualMacSmartConcurrent:1;	// smart concurrent support
	UINT32		FwProcVenCmdSupport:1;		//??
	// 2010/12/30 MH Add to support in the future.
	UINT32		IcVerifyPhase:2;				// FPGA/Test chip/ASIC
	UINT32		FwLoadFromHeader:1;		// From header/read files
	UINT32		InterfaceType:3;				// USB/PCI/PCIE/SDIO/...
	UINT32		DebugPhase:2;				// Free build/chk build/ debug module.
	UINT32		MpSupport:1;				// Normal or MP driver
	UINT32		MsiSupport:2;				// PCIE different interrupt type support traditional/ MIS/ MSI_X based on different PCIE spec
	UINT32		EthSupport:1;				// Ethner simulation switch. for 8715 in the future.or IC verification	
	UINT32		TDLSSupport:1;
#ifdef REMOVE_PACK
#pragma pack()
#endif
	UINT32		NANSupport:2;
	UINT32		P2PSupport:2;
	UINT32		WPSSupport:1;
	UINT32		TXSCSupport:1;
	UINT32		RXSCSupport:1;
	UINT32		RASupport:1;
	UINT32		Reserve:7;	

	//2 92D related variables
	u1Byte 			MacPhyModeFor8192D;
	// dual MAC: 0--Mac0 1--Mac1
	u1Byte			interfaceIndex;
	u1Byte			EarlyMode_Threshold;
	u1Byte			EarlyMode_QueueNum[8]; //The tid from 0--7
	u1Byte			EarlyModebuf[1600];
		
	// 2010/12/27 MH We need to reduce the declareation to prevent occupying too much memory 
	// The better way is to ise the union structure for bit declaration in the future.
	RT_LIST_ENTRY	List;
	//PADAPTER		BuddyAdapter;   
	//BOOLEAN			bMasterOfDMSP;
	//BOOLEAN			bSlaveOfDMSP;

	u1Byte			RegTwoStaConcurrentMode;
	//DUAL_MAC_DMSP_CONTROL		       DualMacDMSPControl;
	//DUM_CTRL_INFO	DumCtrlInfo;
	//DUSC_INFO		DuscInfo;

	//for 92D HW tx retry bug.	// HARDWARE_TYPE_IS_RTL8192D disable by MH
	u1Byte		CurrentFWTxRate;

	PD_T		DM_PDTable;

	u4Byte 		PS_BBRegBackup[PSBBREG_MAX];

	RT_LIST_ENTRY		ListEntry;
	
	u1Byte				ShortcutIndex;	//record entry index of array for shortcut and it's 1 based
	s1Byte				SNForShortcut;
	
	BOOLEAN				bNeedToTriggerLA;
	
	BOOLEAN				bBtFwSupport;

	BOOLEAN				bNromalRQPN;

#if USB_TX_THREAD_ENABLE
	BOOLEAN				bUseUsbTxThread; // Dynamically enable USB Tx thread. 2012.06.18. by tynli.
#endif	

	BOOLEAN				bRemoveTsInRxPath;

	BOOLEAN				bAPChannelInprogressForNewconnected;

	BOOLEAN				bRecvEnterPSForGo; //to indicate P2P client is active or not
	WIRELESS_MODE		BackupWirelessMode;

	//
	// <Roger_DBG> Using following state flag to trace vWiFi and power transaction event for some vWiFi bus BSOD issue.
	// 2013.1213
	//
#if DRV_LOG_REGISTRY
	u4Byte				DrvState;	
	u4Byte				WriteRegRefCount;
	BOOLEAN				bDrvStateError;
#endif

#if WLAN_ETW_SUPPORT
	u4Byte				TxFrameUniqueueID;
	u4Byte				RxFrameUniqueueID;
#endif

	BOOLEAN				bFixBTTdma;
	u4Byte				CPWMTimeoutCount;
	BOOLEAN				bRxPsWorkAround;
	
	RT_THREAD			RxNotifyThread;	

	// Rx queue.
	RT_SINGLE_LIST_HEAD		RxPeerQueue[MAX_PEER_NUM];
	RT_PEER_ENTRY			RxPeerTable[MAX_PEER_NUM];
	u1Byte					RxPeerUsedCount;
	RES_MON_OBJ			ResMonObjWdi;
}ADAPTER,*PADAPTER;

typedef struct _RT_NDIS_DRIVER_CONTEXT	*PRT_NDIS_DRIVER_CONTEXT, RT_NDIS_DRIVER_CONTEXT;

// The Driver(Module) Context in Driver-Wide variables.
typedef	struct	_RT_DRIVER_CONTEXT
{
	RT_NDIS_DRIVER_CONTEXT	NdisContext;	// The Ndis global context

	RT_LIST_ENTRY	AdapterList; // The list of adapters
	RT_SPIN_LOCK	ContextLock; // To protect the content of RT_DRIVER_CONTEXT.
}RT_DRIVER_CONTEXT, *PRT_DRIVER_CONTEXT;

#define	INSERT_GLOBAL_ADAPTER_LIST(__pAdapter)	\
{	\
	PLATFORM_ACQUIRE_RT_SPINLOCK(GlobalRtDriverContext.ContextLock);	\
	RTInsertTailList(&(GlobalRtDriverContext.AdapterList), &((__pAdapter)->ListEntry));	\
	PLATFORM_RELEASE_RT_SPINLOCK(GlobalRtDriverContext.ContextLock);	\
}

#define	REMOVE_GLOBAL_ADAPTER_LIST(__pAdapter)	\
{	\
	PLATFORM_ACQUIRE_RT_SPINLOCK(GlobalRtDriverContext.ContextLock);	\
	RTRemoveEntryList(&((__pAdapter)->ListEntry));	\
	PLATFORM_RELEASE_RT_SPINLOCK(GlobalRtDriverContext.ContextLock);	\
}

extern	RT_DRIVER_CONTEXT	GlobalRtDriverContext;

//----End of Define RT_SPIN_LOCK for all platform.-----//

#endif // #ifndef __INC_TYPEDEF_H
