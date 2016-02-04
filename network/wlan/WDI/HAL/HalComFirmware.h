#ifndef __INC_COMFIRMWARE_H__
#define __INC_COMFIRMWARE_H__

#define PageNum_128(_Len)		(u4Byte)(((_Len)>>7) + ((_Len)&0x7F ? 1:0))
#define PageNum_256(_Len)		(u4Byte)(((_Len)>>8) + ((_Len)&0xFF ? 1:0))
#define PageNum_512(_Len)		(u4Byte)(((_Len)>>9) + ((_Len)&0x1FF ? 1:0))

//
// Check if FW header exists. We do not consider the lower 4 bits in this case. 
// By tynli. 2009.12.04.
// Add new FW signature recognized ID 23xx for RTL8723. 
//
// Signature header for RLE0380 Test chip: 0x2300, RTL8723A Normal Chip: 0x2301
// Added by Roger, 2011.07.28.
//
#define IS_FW_HEADER_EXIST(_pFwHdr)	((GET_FIRMWARE_HDR_SIGNATURE(_pFwHdr) & 0xFFF0) == 0x92C0 ||\
									( GET_FIRMWARE_HDR_SIGNATURE(_pFwHdr) & 0xFFF0) == 0x88C0 ||\
									( GET_FIRMWARE_HDR_SIGNATURE(_pFwHdr) & 0xFF00) == 0x2300 ||\
									( GET_FIRMWARE_HDR_SIGNATURE(_pFwHdr) & 0xFF00) == 0x2301 ||\
									( GET_FIRMWARE_HDR_SIGNATURE(_pFwHdr) & 0xFF00) == 0x2302 ||\
									(GET_FIRMWARE_HDR_SIGNATURE(_pFwHdr) &0xFFFF) == 0x92D0 ||\
									(GET_FIRMWARE_HDR_SIGNATURE(_pFwHdr) &0xFFFF) == 0x92D1 ||\
									(GET_FIRMWARE_HDR_SIGNATURE(_pFwHdr) &0xFFFF) == 0x92D2 ||\
									(GET_FIRMWARE_HDR_SIGNATURE(_pFwHdr) &0xFFFF) == 0x92D3)


#if(FW_QUEME_MECHANISM_NEW != 1)
#define IS_H2CQUEUE_EMPTY(_pHalData) (_pHalData->H2CQueueTail == _pHalData->H2CQueueHead)
#endif

#define GET_FIRMWARE(_Adapter)   ((PRT_FIRMWARE) (((HAL_DATA_TYPE*)(_Adapter->HalData))->pFirmware))  


//#define FillH2CCmd(_Adapter, _ElementID, _CmdLen,_pCmdBuffer)  \
//	(IS_HARDWARE_TYPE_JAGUAR(_Adapter) || IS_HARDWARE_TYPE_8723B(_Adapter))?FillH2CCmd8723B(_Adapter, _ElementID, _CmdLen,_pCmdBuffer):		\
//	(IS_HARDWARE_TYPE_8192E(_Adapter))?FillH2CCmd8192E(_Adapter, _ElementID, _CmdLen,_pCmdBuffer):												\
//	(IS_HARDWARE_TYPE_8188E(_Adapter) ? FillH2CCmd88E(_Adapter, _ElementID, _CmdLen,_pCmdBuffer) : FillH2CCmd92C(_Adapter, _ElementID, _CmdLen,_pCmdBuffer))


#define FW_GTK_EXT_MEM_SIZE		256 // bytes

// The value is define to check FW 32k calibration status in REG_MCUTST_WOWLAN(0x1c7).
// Modify to new value from 8723BS FW v15.
#define FW_DBG_32K_CALI_TIMEOUT	0xfd
#define FW_DBG_32K_CALI_LOCK		0xfe

//--------------------------------------------------
typedef enum _DESC_PACKET_TYPE{
	DESC_PACKET_TYPE_INIT = 0,
	DESC_PACKET_TYPE_NORMAL = 1,	
	DESC_PACKET_TYPE_CMD
}DESC_PACKET_TYPE;


#define FW_SIZE					      0x40000  // Compatible with RTL8723 Maximal RAM code size 24K.   modified to 32k, TO compatible with 92d maximal fw size 32k
#define FW_START_ADDRESS			0x1000

#define MAX_PAGE_SIZE			4096	// @ page : 4k bytes

typedef enum _FIRMWARE_SOURCE{
	FW_SOURCE_IMG_FILE = 0,
	FW_SOURCE_HEADER_FILE = 1,		//from header file
}FIRMWARE_SOURCE, *PFIRMWARE_SOURCE;

typedef struct _RT_FIRMWARE{	
	FIRMWARE_SOURCE	eFWSource;	
	u1Byte			szFwBuffer[FW_SIZE];
	u4Byte			ulFwLength;
	u1Byte			szWoWLANFwBuffer[FW_SIZE];
	u4Byte			ulWoWLANFwLength;
	u1Byte			szBTFwBuffer[FW_SIZE];
	u4Byte			ulBTFwLength;
}RT_FIRMWARE, *PRT_FIRMWARE;

//
// This structure must be cared byte-ordering
//
// Added by tynli. 2009.12.04.

//=====================================================
//					Firmware Header(8-byte alinment required)
//=====================================================
//--- LONG WORD 0 ----
#define GET_FIRMWARE_HDR_SIGNATURE(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr, 0, 16) // 92C0: test chip; 92C, 88C0: test chip; 88C1: MP A-cut; 92C1: MP A-cut
#define GET_FIRMWARE_HDR_CATEGORY(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr, 16, 8) // AP/NIC and USB/PCI
#define GET_FIRMWARE_HDR_FUNCTION(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr, 24, 8) // Reserved for different FW function indcation, for further use when driver needs to download different FW in different conditions
#define GET_FIRMWARE_HDR_VERSION(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr+4, 0, 16)// FW Version
#define GET_FIRMWARE_HDR_SUB_VER(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr+4, 16, 8) // FW Subversion, default 0x00
#define GET_FIRMWARE_HDR_RSVD1(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+4, 24, 8) 		

//--- LONG WORD 1 ----
#define GET_FIRMWARE_HDR_MONTH(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+8, 0, 8) // Release time Month field
#define GET_FIRMWARE_HDR_DATE(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+8, 8, 8) // Release time Date field
#define GET_FIRMWARE_HDR_HOUR(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+8, 16, 8)// Release time Hour field
#define GET_FIRMWARE_HDR_MINUTE(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr+8, 24, 8)// Release time Minute field
#define GET_FIRMWARE_HDR_ROMCODE_SIZE(__FwHdr)	LE_BITS_TO_4BYTE(__FwHdr+12, 0, 16)// The size of RAM code
#define GET_FIRMWARE_HDR_RSVD2(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+12, 16, 16)

//--- LONG WORD 2 ----
#define GET_FIRMWARE_HDR_SVN_IDX(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr+16, 0, 32)// The SVN entry index
#define GET_FIRMWARE_HDR_RSVD3(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+20, 0, 32)

//--- LONG WORD 3 ----
#define GET_FIRMWARE_HDR_RSVD4(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+24, 0, 32)
#define GET_FIRMWARE_HDR_RSVD5(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+28, 0, 32)

//
// This structure must be cared byte-ordering
//
// Added by ylb 20130814

//=====================================================
//				New	Firmware Header(8-byte alinment required)
//=====================================================
//--- LONG WORD 0 ----
#define GET_FIRMWARE_HDR_SIGNATURE_3081(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr, 0, 16) 
#define GET_FIRMWARE_HDR_CATEGORY_3081(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr, 16, 8) // AP/NIC and USB/PCI
#define GET_FIRMWARE_HDR_FUNCTION_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr, 24, 8) // Reserved for different FW function indcation, for further use when driver needs to download different FW in different conditions
#define GET_FIRMWARE_HDR_VERSION_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+4, 0, 16)// FW Version
#define GET_FIRMWARE_HDR_SUB_VER_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+4, 16, 8) // FW Subversion, default 0x00
#define GET_FIRMWARE_HDR_SUB_IDX_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+4, 24, 8) // FW Subversion Index

//--- LONG WORD 1 ----
#define GET_FIRMWARE_HDR_SVN_IDX_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+8, 0, 32)// The SVN entry index
#define GET_FIRMWARE_HDR_RSVD1_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+12, 0, 32)

//--- LONG WORD 2 ----
#define GET_FIRMWARE_HDR_MONTH_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+16, 0, 8) // Release time Month field
#define GET_FIRMWARE_HDR_DATE_3081(__FwHdr)				LE_BITS_TO_4BYTE(__FwHdr+16, 8, 8) // Release time Date field
#define GET_FIRMWARE_HDR_HOUR_3081(__FwHdr)				LE_BITS_TO_4BYTE(__FwHdr+16, 16, 8)// Release time Hour field
#define GET_FIRMWARE_HDR_MINUTE_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+16, 24, 8)// Release time Minute field
#define GET_FIRMWARE_HDR_YEAR_3081(__FwHdr)				LE_BITS_TO_4BYTE(__FwHdr+20, 0, 16)// Release time Year field
#define GET_FIRMWARE_HDR_FOUNDRY_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+20, 16, 8)// Release time Foundry field
#define GET_FIRMWARE_HDR_RSVD2_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+20, 24, 8)

//--- LONG WORD 3 ----
#define GET_FIRMWARE_HDR_MEM_UASGE_DL_FROM_3081(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr+24, 0, 1)
#define GET_FIRMWARE_HDR_MEM_UASGE_BOOT_FROM_3081(__FwHdr)	LE_BITS_TO_4BYTE(__FwHdr+24, 1, 1)
#define GET_FIRMWARE_HDR_MEM_UASGE_BOOT_LOADER_3081(__FwHdr)LE_BITS_TO_4BYTE(__FwHdr+24, 2, 1)
#define GET_FIRMWARE_HDR_MEM_UASGE_IRAM_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+24, 3, 1)
#define GET_FIRMWARE_HDR_MEM_UASGE_ERAM_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+24, 4, 1)
#define GET_FIRMWARE_HDR_MEM_UASGE_RSVD4_3081(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr+24, 5, 3)
#define GET_FIRMWARE_HDR_RSVD3_3081(__FwHdr)					LE_BITS_TO_4BYTE(__FwHdr+24, 8, 8)
#define GET_FIRMWARE_HDR_BOOT_LOADER_SZ_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+24, 16, 16)
#define GET_FIRMWARE_HDR_RSVD5_3081(__FwHdr)					LE_BITS_TO_4BYTE(__FwHdr+28, 0, 32)

//--- LONG WORD 4 ----
#define GET_FIRMWARE_HDR_TOTAL_DMEM_SZ_3081(__FwHdr)	LE_BITS_TO_4BYTE(__FwHdr+36, 0, 32)
#define GET_FIRMWARE_HDR_FW_CFG_SZ_3081(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr+36, 0, 16)
#define GET_FIRMWARE_HDR_FW_ATTR_SZ_3081(__FwHdr)		LE_BITS_TO_4BYTE(__FwHdr+36, 16, 16)

//--- LONG WORD 5 ----
#define GET_FIRMWARE_HDR_IROM_3081(__FwHdr)				LE_BITS_TO_4BYTE(__FwHdr+40, 0, 32)
#define GET_FIRMWARE_HDR_EROM_3081(__FwHdr)				LE_BITS_TO_4BYTE(__FwHdr+44, 0, 32)

//--- LONG WORD 6 ----
#define GET_FIRMWARE_HDR_IRAM_SZ_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+48, 0, 32)
#define GET_FIRMWARE_HDR_ERAM_SZ_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+52, 0, 32)

//--- LONG WORD 7 ----
#define GET_FIRMWARE_HDR_RSVD6_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+56, 0, 32)
#define GET_FIRMWARE_HDR_RSVD7_3081(__FwHdr)			LE_BITS_TO_4BYTE(__FwHdr+60, 0, 32)


//=====================================================
//					FW H2C / C2H CMD structure
//=====================================================

#ifdef REMOVE_PACK
#pragma pack(1)
#endif

//-------------------------------------------------------
// H2C Command length 
// The defines should be seperated into 92C series and 88E series ICs 
// because of the H2C Box length are different. 2012.04.18. Noted by tynli.
//-------------------------------------------------------

//-----------------------------------------
// 92C Series
//-----------------------------------------
#define H2C_92C_PWEMODE_LENGTH			5
#define H2C_92C_JOINBSSRPT_LENGTH		1
#define H2C_92C_RSVDPAGE_LOC_LEN		5
#define H2C_92C_RSVDPAGE_LOC_2_LEN		5
#define H2C_92C_WLANINFO_LENGTH			3
#define H2C_92C_AP_OFFLOAD_LENGTH			3
#define H2C_92C_WOWLAN_LENGTH			4
#define H2C_92C_KEEP_ALIVE_CTRL_LENGTH		3
#define H2C_92C_BT_FW_PATCH_LEN			3
#define H2C_92C_BT_RADIO_STATE_LEN			1
#define H2C_92C_REALWOW_RSVDPAGE_LOC_LENGTH	4
#if(USE_OLD_WOWLAN_DEBUG_FW == 0)
#define H2C_92C_REMOTE_WAKE_CTRL_LEN		1
#else
#define H2C_92C_REMOTE_WAKE_CTRL_LEN		3
#endif
#define H2C_92C_ARP_NDP_OFFLOAD_LENGTH		1
#define H2C_92C_GTK_OFFLOAD_LENGTH			1
#define H2C_92C_REMOTE_WAKEUP_SEC_INFO_LEN	3
#define H2C_92C_FW_PATTERN_RSVDPAGE_LOC_LEN 4
#define H2C_92C_DISCONNECT_DECISION_CTRL_LEN	3

//-----------------------------------------
// 88E Series
//-----------------------------------------
#define H2C_88E_RSVDPAGE_LOC_LEN		5
#define H2C_88E_PWEMODE_LENGTH			5
#define H2C_88E_JOINBSSRPT_LENGTH		1
#define H2C_88E_AP_OFFLOAD_LENGTH		3
#define H2C_88E_WOWLAN_LENGTH			4
#define H2C_88E_KEEP_ALIVE_CTRL_LENGTH	3
#if(USE_OLD_WOWLAN_DEBUG_FW == 0)
#define H2C_88E_REMOTE_WAKE_CTRL_LEN	1
#else
#define H2C_88E_REMOTE_WAKE_CTRL_LEN	3
#endif
#define H2C_88E_AOAC_GLOBAL_INFO_LEN	2
#define H2C_88E_AOAC_RSVDPAGE_LOC_LEN	7
#define H2C_88E_AOAC_RSVDPAGE2_LOC_LEN	7
#define H2C_88E_SCAN_OFFLOAD_CTRL_LEN	4
#define H2C_88E_BT_FW_PATCH_LEN			6
#define H2C_88E_INACTIVE_PS_LEN		3
#define H2C_88E_DISCONNECT_DECISION_CTRL_LEN	4
#define H2C_88E_AOAC_RSVDPAGE3_LOC_LEN	7

//-----------------------------------------
// 8723B Series
//-----------------------------------------
#define H2C_8723B_WIFI_CALIBRATION_LEN			1
#define H2C_8723B_FCS_INFO_LEN			7
#define H2C_8723B_FCS_MACID_BITMAP_LEN	7

//-----------------------------------------
// 8703B Series
//-----------------------------------------
#define H2C_8703B_WIFI_CALIBRATION_LEN			1

//-----------------------------------------
// 8188F Series
//-----------------------------------------
#define H2C_8188F_WIFI_CALIBRATION_LEN			1


//-----------------------------------------
// 8812 Series
//-----------------------------------------
#define H2C_8812_RSVDPAGE_FCS_LOC_LEN	3
#define H2C_8812_FCS_INFO_LEN			7

//-----------------------------------------
// 8821 Series
//-----------------------------------------
#define H2C_8821A_RSVDPAGE_FCS_LOC_LEN	3
#define H2C_8821A_FCS_INFO_LEN			7
#define H2C_8821A_FCS_MACID_BITMAP_LEN	7
#define H2C_8821B_FCS_INFO_LEN			7
#define H2C_8821B_RSVDPAGE_FCS_LOC_LEN	3

//-----------------------------------------
// 8192E Series
//-----------------------------------------
#define H2C_8192E_RSVDPAGE_FCS_LOC_LEN	3
#define H2C_8192E_FCS_INFO_LEN				7
#define H2C_8192E_FCS_MACID_BITMAP_LEN	7

typedef struct _H2C_JOINBSSRPT_PARM{	
	u1Byte	OpMode;	
	u1Byte	Ps_Qos_Info;
	u1Byte	Bssid[6];	
	u2Byte	BcnItv;	
	u2Byte	Aid;
}H2C_JOINBSSRPT_PARM, *PH2C_JOINBSSRPT_PARM;


typedef struct _C2H_EVT_HDR{
	u1Byte	CmdID: 4;
	u1Byte	CmdLen: 4; 
	u1Byte	CmdSeq;
}C2H_EVT_HDR, *PC2H_EVT_HDR;

#ifdef REMOVE_PACK
#pragma pack()
#endif

typedef struct _H2C_WPA_PTK {
 	u1Byte 	kck[16]; /* EAPOL-Key Key Confirmation Key (KCK) */
 	u1Byte 	kek[16]; /* EAPOL-Key Key Encryption Key (KEK) */
	u1Byte	tk1[16]; /* Temporal Key 1 (TK1) */
	union {
		u1Byte	tk2[16]; //Temporal Key 2 (TK2)
		struct {
			u1Byte	tx_mic_key[8];
			u1Byte	rx_mic_key[8];
		}Athu;
	}U;
}H2C_WPA_PTK;

// For old Hw security engine. Support on 92C/8723A/88E/8812A, otherwise just use H2C_WPA_PTK structure.
typedef struct _H2C_WPA_TWO_WAY_PARA{
	//algorithm TKIP or AES
	u1Byte			pairwise_en_alg;
	u1Byte			group_en_alg;
	H2C_WPA_PTK		wpa_ptk_value;
#if(USE_OLD_WOWLAN_DEBUG_FW == 1)	
	u8Byte			IV;
#endif
}H2C_WPA_TWO_WAY_PARA, *PH2C_WPA_TWO_WAY_PARA;

typedef struct _H2C_REMOTE_WAKE_CTRL_INFO{
	u1Byte   IV[8];
}H2C_REMOTE_WAKE_CTRL_INFO, *PH2C_REMOTE_WAKE_CTRL_INFO;


typedef struct _NDP_INFO_{
    u1Byte    bEnable:1;
    u1Byte    bCheckRemoveIP:1;   // Need to Check Sender IP or not 
    u1Byte    Rsvd:6;   // Need to Check Sender IP or not 
    u1Byte    NumberOfTargetIP; // Number of Check IP which NA query IP
    u1Byte    TagetLinkAddress[6];  // Maybe support change MAC address !!
        u1Byte     RemoteIPv6Address[16]; // Just respond IP
        u1Byte     TargetIP[2][16]; //  target IP 
}NDP_INFO,*PNDP_INFO;


typedef struct _H2C_PROTOCOL_OFFLOAD_INFO{
        NDP_INFO NDPInfo[2];
} H2C_PROTOCOL_OFFLOAD_INFO, *PH2C_PROTOCOL_OFFLOAD_INFO;



#ifdef REMOVE_PACK
#pragma pack(1)
#endif

typedef struct	_HAL92C_P2P_PS_OFFLOAD
{
	u1Byte		Offload_En:1;
	u1Byte		role:1; // 1: Owner, 0: Client
	u1Byte		CTWindow_En:1;
	u1Byte		NoA0_En:1;
	u1Byte		NoA1_En:1;
	u1Byte		AllStaSleep:1;
	u1Byte		discovery:1;
}HAL92C_P2P_PS_OFFLOAD, *PHAL92C_P2P_PS_OFFLOAD;

#ifdef REMOVE_PACK
#pragma pack()
#endif

typedef struct _H2C_SS_DRV_CTRL{
	u1Byte	ChnlListLen;			// scan channel list length
	u1Byte	OriginalChl;			// current client channel
	u1Byte	OriginalBW;			// current client BW 20M/40M/80M
	u1Byte	Originalch40offset;	// 40M second channel offset
	u1Byte	Originalch80offset;	// 80M second channel offset
	u1Byte	bPriodScan;			// Fw periodically scan.
	u1Byte	PriodScanTime;		// Fw periodical scan time. unit: 1sec.
	u1Byte	bRFEEnable;			// for 8812A switch band
	u1Byte	RFEType;
	u1Byte	Rsvd[7];				
}H2C_SS_DRV_CTRL, *PH2C_SS_DRV_CTRL;


typedef struct _H2C_NLO_HEADER_INFO{
	u1Byte	NumOfEntry;
	u1Byte	Rsvd1[3];
	u1Byte	FastScanPeriod[4];
	u1Byte	FastScanIterations[4];
	u1Byte	SlowScanPeriod[4];
	u1Byte	SsidLen[16];
	u1Byte	Cipher[16];
	u1Byte	ChannelList[16];
}H2C_NLO_HEADER_INFO, *PH2C_NLO_HEADER_INFO;


#define	H2C_MAC_MODE_SEL			9

//#define	MAX_RSVD_PAGE_BUF_LEN		(128*9) // page size * the rsvd page packet number in Tx pkt buf

// Description: Determine the types of C2H events that are the same in driver and Fw.
// Fisrt constructed by tynli. 2009.10.09.
typedef enum _RTL8192C_C2H_EVT
{
	C2H_DBG = 0,
	C2H_TSF = 1,
	C2H_AP_RPT_RSP = 2,
	C2H_CCX_TX_RPT = 3,	// The FW notify the report of the specific tx packet.
	C2H_BT_RSSI = 4,		
	C2H_BT_OP_MODE = 5,
	C2H_EXT_RA_RPT = 6, 
	C2H_HW_INFO_EXCH = 10,
	C2H_C2H_H2C_TEST = 11,
	C2H_BT_INFO = 12,
	C2H_BT_MP = 15,
	MAX_C2HEVENT
}RTL8192C_C2H_EVT;

typedef enum _FW_SCAN_OFFLOAD_TYPE{
	FW_SCAN_OFFLOAD_D0 = 0,
	FW_SCAN_OFFLOAD_D3 = 1,	
}FW_SCAN_OFFLOAD_TYPE;

//==========================================================
//----------------------------------------------------------------------
// Fw Initial Offload command
//----------------------------------------------------------------------
#define INIT_OFFLOAD_ACTION_LLT			BIT0
#define INIT_OFFLOAD_ACTION_R_EFUSE		BIT1
#define INIT_OFFLOAD_ACTION_EFUSE_PATCH	BIT2

typedef enum  _INIT_OFFLOAD_CMD{
	INIT_OFFLOAD_CMD_LLT = 0x01,
	INIT_OFFLOAD_CMD_R_EFUSE = 0x02,
	INIT_OFFLOAD_CMD_EFUSE_PATCH = 0x03,
	INIT_OFFLOAD_CMD_WB_REG = 0x04,
	INIT_OFFLOAD_CMD_WW_REG = 0x05,
	INIT_OFFLOAD_CMD_WD_REG = 0x06,
	INIT_OFFLOAD_CMD_W_RF = 0x07,
	INIT_OFFLOAD_CMD_DELAY_US = 0x10,
	INIT_OFFLOAD_CMD_DELAY_MS = 0x11,
	INIT_OFFLOAD_CMD_END	= 0xff,
}INIT_OFFLOAD_CMD, *PINIT_OFFLOAD_CMD;

//----------------------------------------------------------------------
// Fw wake up reason code
//----------------------------------------------------------------------
// V1 is for 88C and 8723A.
typedef enum _FW_WOW_REASON_V1{
	FW_WOW_V1_PTK_UPDATE_EVENT = 0x01,
	FW_WOW_V1_GTK_UPDATE_EVENT = 0x02,
	FW_WOW_V1_DISASSOC_EVENT = 0x04,
	FW_WOW_V1_DEAUTH_EVENT = 0x08,
	FW_WOW_V1_FW_DISCONNECT_EVENT = 0x10,
	FW_WOW_V1_HW_WAKE_PKT_EVENT = 0x20,
	FW_WOW_V1_APPL_QQ = 0x21, //Application
	FW_WOW_V1_FWPATTERNMATCH0 =  0x40,
	FW_WOW_V1_FWPATTERNMATCH1 = 0x41,
	FW_WOW_V1_FWPATTERNMATCH2 = 0x42,
	FW_WOW_V1_FWPATTERNMATCH3 = 0x43,
	FW_WOW_V1_ONWAKEUPPKT = 0x80,    // 128
	FW_WOW_V1_RSPKTTIMEOUT =  0x81,    // 129
	FW_WOW_V1_DHCPTIMEOUT = 0x82,    // 130
	FW_WOW_V1_NLMATCH = 0x83,    // 131
	FW_WOW_V1_REASON_MAX = 0xff,
}FW_WOW_REASON_V1, *PFW_WOW_REASON_V1;

// After 8188E, we use V2 reason define. 88C/8723A use V1 reason.
typedef enum _FW_WOW_REASON_V2{
	FW_WOW_V2_PTK_UPDATE_EVENT = 0x01,
	FW_WOW_V2_GTK_UPDATE_EVENT = 0x02,
	FW_WOW_V2_DISASSOC_EVENT = 0x04,
	FW_WOW_V2_DEAUTH_EVENT = 0x08,
	FW_WOW_V2_FW_DISCONNECT_EVENT = 0x10,
	FW_WOW_V2_MAGIC_PKT_EVENT = 0x21,
	FW_WOW_V2_UNICAST_PKT_EVENT = 0x22,
	FW_WOW_V2_PATTERN_PKT_EVENT = 0x23,
	FW_WOW_V2_RTD3_SSID_MATCH_EVENT = 0x24,
	FW_WOW_V2_REALWOW_V2_WAKEUPPKT = 0x30,
	FW_WOW_V2_REALWOW_V2_ACKLOST = 0x31,
	FW_WOW_V2_NLO_SSID_MATCH_EVENT = 0x55,
	FW_WOW_V2_REASON_MAX = 0xff,
}FW_WOW_REASON, *PFW_WOW_REASON;

#endif
