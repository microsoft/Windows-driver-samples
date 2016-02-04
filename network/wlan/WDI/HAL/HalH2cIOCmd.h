#ifndef __INC_HAL_H2CIOCMD_H
#define __INC_HAL_H2CIOCMD_H

//--------------------------------------------
//3				Host Message Box 
//--------------------------------------------

//_RSVDPAGE_LOC_CMD0
#define SET_H2CCMD_RSVDPAGE_LOC_PROBE_RSP(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd), 	0, 8, __Value)
#define SET_H2CCMD_RSVDPAGE_LOC_PSPOLL(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_RSVDPAGE_LOC_NULL_DATA(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)
#define SET_H2CCMD_RSVDPAGE_LOC_QOS_NULL_DATA(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+3, 0, 8, __Value)
#define SET_H2CCMD_RSVDPAGE_LOC_BT_QOS_NULL_DATA(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd)+4, 0, 8, __Value)
#define SET_H2CCMD_RSVDPAGE_LOC_BT_CTS(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd)+5, 0, 8, __Value)

//_MEDIA_STATUS_RPT_PARM_CMD1
#define SET_H2CCMD_MSRRPT_PARM_OPMODE(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE(__pH2CCmd,   0, 1, __Value)
#define SET_H2CCMD_MSRRPT_PARM_MACID_IND(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE(__pH2CCmd,	  1, 1, __Value)
#define SET_H2CCMD_MSRRPT_PARM_MACID(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE(__pH2CCmd+1, 0, 8, __Value)
#define SET_H2CCMD_MSRRPT_PARM_MACID_END(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE(__pH2CCmd+2, 0, 8, __Value)

// _WoWLAN PARAM_CMD5
#define SET_H2CCMD_WOWLAN_FUNC_ENABLE(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 1, __Value)
#define SET_H2CCMD_WOWLAN_PATTERN_MATCH_ENABLE(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	1, 1, __Value)
#define SET_H2CCMD_WOWLAN_MAGIC_PKT_ENABLE(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd),	2, 1, __Value)
#define SET_H2CCMD_WOWLAN_UNICAST_PKT_ENABLE(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	3, 1, __Value)
#define SET_H2CCMD_WOWLAN_ALL_PKT_DROP(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd),	4, 1, __Value)
#define SET_H2CCMD_WOWLAN_GPIO_ACTIVE(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd),	5, 1, __Value)
#define SET_H2CCMD_WOWLAN_REKEY_WAKE_UP(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd),	6, 1, __Value)
#define SET_H2CCMD_WOWLAN_DISCONNECT_WAKE_UP(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	7, 1, __Value)
#define SET_H2CCMD_WOWLAN_GPIONUM(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_WOWLAN_GPIO_DURATION(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)
#define SET_H2CCMD_WOWLAN_GPIO_PULSE_EN(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+3, 0, 8, __Value)
#define SET_H2CCMD_WOWLAN_GPIO_PULSE_CNT(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+3, 1, 8, __Value)

//WLANINFO_PARM
#define SET_H2CCMD_WLANINFO_PARM_OPMODE(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 8, __Value)
#define SET_H2CCMD_WLANINFO_PARM_CHANNEL(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_WLANINFO_PARM_BW40MHZ(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)

// _REMOTE_WAKEUP_CMD7
#define SET_H2CCMD_REMOTE_WAKECTRL_ENABLE(__pH2CCmd, __Value)						SET_BITS_TO_LE_1BYTE(__pH2CCmd, 0, 1, __Value)
#define SET_H2CCMD_REMOTE_WAKE_CTRL_ARP_OFFLOAD_EN(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE(__pH2CCmd, 1, 1, __Value)
#define SET_H2CCMD_REMOTE_WAKE_CTRL_NDP_OFFLOAD_EN(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE(__pH2CCmd, 2, 1, __Value)
#define SET_H2CCMD_REMOTE_WAKE_CTRL_GTK_OFFLOAD_EN(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE(__pH2CCmd, 3, 1, __Value)
#define SET_H2CCMD_REMOTE_WAKE_CTRL_NETWORK_LIST_OFFLOAD_EN(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE(__pH2CCmd, 4, 1, __Value)

// _AP_OFFLOAD_CMD8
#define SET_H2CCMD_AP_OFFLOAD_ON(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 8, __Value)
#define SET_H2CCMD_AP_OFFLOAD_HIDDEN(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_AP_OFFLOAD_DENYANY(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)
#define SET_H2CCMD_AP_OFFLOAD_WAKEUP_EVT_RPT(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd)+3, 0, 8, __Value)

// FCS_LOC_CMD10	(Not appear in 8192EE)
#define SET_H2CCMD_RSVDPAGE_FCS_LOC_NULL_DATA0(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 8, __Value)
#define SET_H2CCMD_RSVDPAGE_FCS_LOC_NULL_DATA1(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_RSVDPAGE_FCS_LOC_NULL_DATA2(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)

// FCS_INFO_CMD11
#define SET_H2CCMD_FCS_INFO_ORDER(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 4, __Value)
#define SET_H2CCMD_FCS_INFO_TOTAL(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd),	4, 4, __Value)
#define SET_H2CCMD_FCS_INFO_CHIDX(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd+1), 0, 8, __Value)
#define SET_H2CCMD_FCS_INFO_BW(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd+2), 0, 2, __Value)
#define SET_H2CCMD_FCS_INFO_BWSC_40(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd+2), 2, 3, __Value)
#define SET_H2CCMD_FCS_INFO_BWSC_80(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd+2), 5, 3, __Value)
#define SET_H2CCMD_FCS_INFO_DURATION(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd+3), 0, 8, __Value)
#define SET_H2CCMD_FCS_INFO_MACID0(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd+4), 0, 8, __Value)
#define SET_H2CCMD_FCS_INFO_MACID1(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd+5), 0, 8, __Value)

//	From Jaguar
#define SET_H2CCMD_FCS_INFO_RFETYPE(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd+6), 0, 8, __Value)

// _PWR_MOD_CMD20
#define SET_H2CCMD_PWRMODE_PARM_MODE(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd), 	0, 8, __Value)
#define SET_H2CCMD_PWRMODE_PARM_RLBM(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 4, __Value)
#define SET_H2CCMD_PWRMODE_PARM_SMART_PS(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 4, 4, __Value)
#define SET_H2CCMD_PWRMODE_PARM_BCN_PASS_TIME(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)
#define SET_H2CCMD_PWRMODE_PARM_ALL_QUEUE_UAPSD(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+3, 0, 8, __Value)
#define SET_H2CCMD_PWRMODE_PARM_PWR_STATE(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+4, 0, 8, __Value)
#define SET_H2CCMD_PWRMODE_PARM_TWO_ANTENNA_EN(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+5, 0, 8, __Value)

#define GET_H2CCMD_PWRMODE_PARM_MODE(__pH2CCmd)							LE_BITS_TO_1BYTE((__pH2CCmd),	0, 8)

// AOAC_GLOBAL_INFO
#define SET_H2CCMD_AOAC_GLOBAL_INFO_PAIRWISE_ENC_ALG(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 8, __Value)
#define SET_H2CCMD_AOAC_GLOBAL_INFO_GROUP_ENC_ALG(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)

// AOAC_RSVDPAGE_LOC
#define SET_H2CCMD_AOAC_RSVDPAGE_LOC_REMOTE_WAKE_CTRL_INFO(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 8, __Value)
#define SET_H2CCMD_AOAC_RSVDPAGE_LOC_ARP_RSP(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_AOAC_RSVDPAGE_LOC_NS(__pH2CCmd, __Value)						SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)
#define SET_H2CCMD_AOAC_RSVDPAGE_LOC_GTK_RSP(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd)+3, 0, 8, __Value)
#define SET_H2CCMD_AOAC_RSVDPAGE_LOC_GTK_INFO(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd)+4, 0, 8, __Value)
#define SET_H2CCMD_AOAC_RSVDPAGE_LOC_GTK_EXT_MEM(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+5, 0, 8, __Value)
#define SET_H2CCMD_AOAC_RSVDPAGE_LOC_PROT_OFFLOAD_INFO(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+6, 0, 8, __Value)

// RSVDPAGE_LOC_3
#define SET_H2CCMD_RSVDPAGE_LOC_3_NLO_INFO(__pH2CCmd, __Value)					SET_BITS_TO_LE_1BYTE((__pH2CCmd), 0, 8, __Value)

//_SCAN_OFFLOAD_CTRL
#define SET_H2CCMD_SCAN_OFFLOAD_CTRL_PARM_D0_SCAN_FUNC_ENABLE(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 1, __Value)
#define SET_H2CCMD_SCAN_OFFLOAD_CTRL_PARM_RTD3_FUNC_ENABLE(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd),	1, 1, __Value)
#define SET_H2CCMD_SCAN_OFFLOAD_CTRL_PARM_U3_SCAN_FUNC_ENABLE(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	2, 1, __Value)
#define SET_H2CCMD_SCAN_OFFLOAD_CTRL_PARM_NLO_FUNC_ENABLE(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd),	3, 1, __Value)
#define SET_H2CCMD_SCAN_OFFLOAD_CTRL_PARM_IPS_DEPENDENT(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd),	4, 1, __Value)
#define SET_H2CCMD_SCAN_OFFLOAD_CTRL_PARM_RSVDPAGE_LOC(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_SCAN_OFFLOAD_CTRL_PARM_CHNL_INFO_LOC(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)
#define SET_H2CCMD_SCAN_OFFLOAD_CTRL_PARM_SSID_INFO_LOC(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+3, 0, 8, __Value)

#define GET_H2CCMD_SCAN_OFFLOAD_CTRL_PARM_ENABLE(__pH2CCmd)							LE_BITS_TO_1BYTE((__pH2CCmd),	0, 8)

//_CHANNEL_PATTERN
#define SET_H2CCMD_CHANNEL_PATTERN_PARM_CHNL_NUM(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 8, __Value)
#define SET_H2CCMD_CHANNEL_PATTERN_PARM_TXPWRIDX(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_CHANNEL_PATTERN_PARM_TIMEOUT(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)
#define SET_H2CCMD_CHANNEL_PATTERN_PARM_ACTIVE(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+3, 0, 8, __Value)

// BT_FW_PATCH
//#define SET_H2CCMD_BT_FW_PATCH_ENABLE(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE(__pH2CCmd, 0, 1, __Value)
#define SET_H2CCMD_BT_FW_PATCH_SIZE(__pH2CCmd, __Value)				SET_BITS_TO_LE_2BYTE((pu1Byte)(__pH2CCmd), 0, 16, __Value)
#define SET_H2CCMD_BT_FW_PATCH_ADDR0(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((pu1Byte)(__pH2CCmd)+2, 0, 8, __Value)
#define SET_H2CCMD_BT_FW_PATCH_ADDR1(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((pu1Byte)(__pH2CCmd)+3, 0, 8, __Value)
#define SET_H2CCMD_BT_FW_PATCH_ADDR2(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((pu1Byte)(__pH2CCmd)+4, 0, 8, __Value)
#define SET_H2CCMD_BT_FW_PATCH_ADDR3(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((pu1Byte)(__pH2CCmd)+5, 0, 8, __Value)

//_INACTIVE_PS_CTRL
#define SET_H2CCMD_INACTIVE_PS_PARM_FUNC_ENABLE(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 1, __Value)
#define SET_H2CCMD_INACTIVE_PS_PARM_IGNORE_PS_CONDITION(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd),	1, 1, __Value)
#define SET_H2CCMD_INACTIVE_PS_PARM_SCAN_FREQUENCY(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)
#define SET_H2CCMD_INACTIVE_PS_PARM_DURATION(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)

#define GET_H2CCMD_INACTIVE_PS_PARM_FUNC_ENABLE(__pH2CCmd)						LE_BITS_TO_1BYTE((__pH2CCmd),	0, 1)

// Disconnect_Decision_Control
// <Note> 
// 1. The value "CHECK_PERIOD" should be larger than 5 which Fw checks beacon lost period.
// 2. "TRY_OK_BCN_LOST_CNT" should be smaller than "CHECK_PERIOD".
#define SET_H2CCMD_DISCONNECT_DECISION_CTRL_ENABLE(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 1, __Value)
#define SET_H2CCMD_DISCONNECT_DECISION_CTRL_USER_SETTING(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd),	1, 1, __Value)
#define SET_H2CCMD_DISCONNECT_DECISION_CTRL_CNT_BCN_LOST_EN(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd),	2, 1, __Value)
#define SET_H2CCMD_DISCONNECT_DECISION_CTRL_CHECK_PERIOD(__pH2CCmd, __Value)		SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value) // unit: beacon period
#define SET_H2CCMD_DISCONNECT_DECISION_CTRL_TRYPKT_NUM(__pH2CCmd, __Value)			SET_BITS_TO_LE_1BYTE((__pH2CCmd)+2, 0, 8, __Value)
#define SET_H2CCMD_DISCONNECT_DECISION_CTRL_TRY_OK_BCN_LOST_CNT(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd)+3, 0, 8, __Value)

// WIFI_CALIBRATION
#define SET_H2CCMD_WIFI_CALIBRATION_EN(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 1, __Value)

// Keep Alive Control
#define SET_H2CCMD_KEEP_ALIVE_ENABLE(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd),	0, 1, __Value)
#define SET_H2CCMD_KEEP_ALIVE_ACCPEPT_USER_DEFINED(__pH2CCmd, __Value)	SET_BITS_TO_LE_1BYTE((__pH2CCmd),	1, 1, __Value)
#define SET_H2CCMD_KEEP_ALIVE_PERIOD(__pH2CCmd, __Value)				SET_BITS_TO_LE_1BYTE((__pH2CCmd)+1, 0, 8, __Value)

typedef enum _H2C_CMD 
{
	H2C_RSVDPAGE = 0,
	H2C_MSRRPT = 1,
	H2C_SCAN = 2,					// No used in 8723B, 8192E, 8812/8821
	H2C_KEEP_ALIVE_CTRL = 3,
	H2C_DISCONNECT_DECISION = 4,
	
	H2C_INIT_OFFLOAD = 6,			// No used in 8723B, 8192E, 8812/8821
	H2C_AP_OFFLOAD = 8,				//
	H2C_BCN_RSVDPAGE = 9,			//
	H2C_PROBERSP_RSVDPAGE = 10,		//
	H2C_TXPOWER_INDEX_OFFLOAD = 0x0B,
	H2C_FCS_LOCATION = 0x10,
	H2C_FCS_INFO = 0x11,

	H2C_NAN	= 0x1B,
	
	H2C_SETPWRMODE = 0x20,		

	H2C_PS_TUNING_PARA = 0x21,		// No used in 8723B, 8192E, 8812/8821
	H2C_PS_TUNING_PARA2 = 0x22,		//
	H2C_PS_LPS_PARA = 0x23,			//
	H2C_P2P_PS_OFFLOAD = 0x24,		//

	H2C_INACTIVE_PS = 0x27,	
	
	H2C_RA_MASK 			= 0x40,
	H2C_TxBF 				= 0x41,				// ADDed from 8192E and 8812
	H2C_RSSI_REPORT 		= 0x42,
	H2C_AP_REQ_TXRPT		= 0x43,
	H2C_INIT_RATE_COLLECT = 0x44,
	H2C_IQ_CALIBRATION 	= 0x45,
	H2C_RA_PARA_ADJUST 	= 0x47,
	H2C_FW_TRACE_EN 		= 0x49,

	
	H2C_BT_FW_PATCH = 0x6a,
	H2C_WIFI_CALIBRATION = 0x6d,	// 8723B used only

	H2C_WO_WLAN = 0x80,
	H2C_REMOTE_WAKE_CTRL = 0x81,
	H2C_AOAC_GLOBAL_INFO = 0x82,
	H2C_AOAC_RSVDPAGE = 0x83,

	H2C_AOAC_RSVDPAGE2 = 0x84,		// 8812 used only

	H2C_SCAN_OFFLOAD_CTRL = 0x86,
	H2C_FW_SWCHANNL = 0x87,
	H2C_AOAC_RSVDPAGE3 = 0x88,

	//Not defined in new 88E H2C CMD Format
	H2C_SELECTIVE_SUSPEND_ROF_CMD,	
	H2C_P2P_PS_MODE,				// This be replaced with H2C_P2P_PS_OFFLOAD(0x24)
	H2C_PSD_RESULT,
	MAX_H2CCMD
}_H2C_CMD;

VOID
FillH2CCmd(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		ElementID,
	IN	u4Byte 		CmdLen,
	IN	pu1Byte		pCmdBuffer
);

BOOLEAN
CheckFwReadLastH2C(
	IN	PADAPTER	Adapter,
	IN	u1Byte		BoxNum
);

VOID
SetFwPwrModeCmd(
	IN PADAPTER		Adapter,
	IN u1Byte		Mode
);

VOID
SetFwMediaStatusRptCmd(
	IN PADAPTER		Adapter,
	IN u1Byte		mstatus,
	IN BOOLEAN		macId_Ind,
	IN u1Byte 		macId,
	IN u1Byte		macId_End
);

VOID
SetFwRemoteWakeCtrlCmd(
	IN PADAPTER		Adapter,
	IN u1Byte		Enable
);

VOID
SetFwGlobalInfoCmd(
	IN PADAPTER		Adapter
);

VOID
SetFwScanOffloadCtrlCmd(
	IN PADAPTER		Adapter,
	IN u1Byte		Type,
	IN u1Byte		ScanOffloadEnable,
	IN u1Byte		NLOEnable
);

VOID
SetFwInactivePSCmd(
	IN PADAPTER		Adapter,
	IN u1Byte		Enable,
	IN BOOLEAN		bActiveWakeup,
	IN BOOLEAN		bForceClkOff
);

VOID
SetFwBTFwPatchCmd(
	IN PADAPTER		Adapter,
	IN u2Byte		FwSize
);

VOID
SetFwDisconnectDecisionCtrlCmd(
	IN PADAPTER		Adapter,
	IN BOOLEAN		bEnabled
);

VOID
SetFwWiFiCalibrationCmd(
	IN PADAPTER		Adapter,
	IN u1Byte		u1Enable
);

VOID
ConstructFwGTKInfo(
	IN PADAPTER		Adapter,
	OUT pu1Byte		pBuffer
);

VOID
ConstructFwRemoteWakeCtrlInfo(
	IN PADAPTER		Adapter,
	OUT pu1Byte		pBuffer
);

VOID
ConstructFwProtrolOffloadInfo(
	IN PADAPTER		Adapter,
	OUT pu1Byte		pBuffer
);

VOID
ConstructFwChannelInfo(
	IN PADAPTER		Adapter,
	OUT pu1Byte		pBuffer,
	OUT pu4Byte		pBufferLen
);

VOID
ConstructFwSsidInfo(
	IN PADAPTER		Adapter,
	OUT pu1Byte		pBuffer,
	OUT pu4Byte		pBufferLen
);

VOID
ConstructFwNLOInfo(
	IN PADAPTER 	Adapter,
	OUT pu1Byte 	pBuffer,
	OUT pu4Byte 	pBufferLen
);

u4Byte
FillH2CCommand(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		ElementID,
	IN	u4Byte 		CmdLen,
	IN	pu1Byte		pCmdBuffer
);

#if(FW_QUEME_MECHANISM_NEW != 1)
VOID
AddH2CCmdQueue(
	IN	PADAPTER	Adapter,
	IN	u1Byte 		ElementID,
	IN	u4Byte 		CmdLen,
	IN	pu1Byte		pCmdBuffer
);

BOOLEAN
RetriveH2CCmdQueue(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		pH2CCmdBuf
);
#endif


VOID
SetFwFcsLocCmd(
	IN	PADAPTER	Adapter
);

VOID
SetFwFcsInfoCmd(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		pContext,
	IN	BOOLEAN		bSet
);

VOID
HalSetFwKeepAliveCmd(	
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		bFuncEn
);

RT_STATUS
HalSetFWWoWlanMode(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		bFuncEn
);
#endif