#ifndef __INC_DEBUG_H
#define __INC_DEBUG_H


//-----------------------------------------------------------------------------
//	Define the debug levels
//
//	1.	DBG_TRACE and DBG_LOUD are used for normal cases.
//	So that, they can help SW engineer to develope or trace states changed 
//	and also help HW enginner to trace every operation to and from HW, 
//	e.g IO, Tx, Rx. 
//
//	2.	DBG_WARNNING and DBG_SERIOUS are used for unusual or error cases, 
//	which help us to debug SW or HW.
//
//-----------------------------------------------------------------------------
//
//	Never used in a call to RT_TRACE()!
//
#define DBG_OFF					0

//
//	Deprecated! Don't use it! 
//	TODO: fix related debug message!
//
//#define DBG_SEC					1

//
//	Fatal bug. 
//	For example, Tx/Rx/IO locked up, OS hangs, memory access violation, 
//	resource allocation failed, unexpected HW behavior, HW BUG and so on.
//
#define DBG_SERIOUS				2	// WPP Error condition

//
//	Abnormal, rare, or unexpeted cases.
//	For example, IRP/Packet/OID canceled, device suprisely unremoved and so on.
//
#define DBG_WARNING			3	// WPP Warning

//
//	Normal case with useful information about current SW or HW state. 
//	For example, Tx/Rx descriptor to fill, Tx/Rx descriptor completed status, 
//	SW protocol state change, dynamic mechanism state change and so on.
//
#define DBG_LOUD				4	// WPP Information

//
//	Normal case with detail execution flow or information.
//
#define DBG_TRACE				5	//WPP most detailed trace


#if WPP_SOFTWARE_TRACE


//
// Trace framework definition when WPP based tracing is enabled
// Re-map WPP trace level in following DBG level, revised by Roger, 2014.02.11.
//		
//#define WPP_LEVEL_NONE				0
//#define WPP_LEVEL_FATAL				1	// fatal error
//#define WPP_LEVEL_ERROR				2	// error condition
//#define WPP_LEVEL_WARNING			3	// warning
//#define WPP_LEVEL_INFORMATION		4	// information
//#define WPP_LEVEL_VERBOSE			5	// most detailed trace
//

//
// These are the various tracing components we can have. The max number is 31
// so always keep that in mind.
//
// WARNING! THIS GUID IS BEING USED BY THE SAMPLE DRIVER.
// PLEASE GENERATE YOUR OWN UNIQUE GUID AND PLACE BELOW.
//
// Use COMP_TESTING when you want to add messages trace through a specific
// piece of code and not want to see trace from all other components
//
#define WPP_CONTROL_GUIDS                                                   \
	WPP_DEFINE_CONTROL_GUID(rtOriGuid1,(e62b367e,b2f0,4571,8c6a,3551a3a5a6b9), \
		WPP_DEFINE_BIT(COMP_DBG)			\
		WPP_DEFINE_BIT(COMP_INIT)			\
		WPP_DEFINE_BIT(COMP_OID_QUERY)		\
		WPP_DEFINE_BIT(COMP_OID_SET)		\
		WPP_DEFINE_BIT(COMP_RECV)			\
		WPP_DEFINE_BIT(COMP_SEND)			\
		WPP_DEFINE_BIT(COMP_IO)				\
		WPP_DEFINE_BIT(COMP_POWER)			\
		WPP_DEFINE_BIT(COMP_MLME)			\
		WPP_DEFINE_BIT(COMP_SCAN)			\
		WPP_DEFINE_BIT(COMP_SYSTEM)			\
		WPP_DEFINE_BIT(COMP_SEC)			\
		WPP_DEFINE_BIT(COMP_AP)				\
		WPP_DEFINE_BIT(COMP_QOS)			\
		WPP_DEFINE_BIT(COMP_AUTHENTICATOR)	\
		WPP_DEFINE_BIT(COMP_BEACON)			\
		WPP_DEFINE_BIT(COMP_WFD)			\
		WPP_DEFINE_BIT(COMP_RM)				\
		WPP_DEFINE_BIT(COMP_MP)				\
		WPP_DEFINE_BIT(COMP_CKIP)			\
		WPP_DEFINE_BIT(COMP_RSNA)			\
		WPP_DEFINE_BIT(COMP_INDIC)			\
		WPP_DEFINE_BIT(COMP_LED)			\
		WPP_DEFINE_BIT(COMP_RF)				\
		WPP_DEFINE_BIT(COMP_COEX)			\
	)	\
	WPP_DEFINE_CONTROL_GUID(rtOriGuid2,(429ffa43,470c,40c2,b552,9cc23c2638ca), \
		WPP_DEFINE_BIT(COMP_HT)				\
		WPP_DEFINE_BIT(COMP_POWER_TRACKING)	\
		WPP_DEFINE_BIT(COMP_RX_REORDER)		\
		WPP_DEFINE_BIT(COMP_AMSDU)			\
		WPP_DEFINE_BIT(COMP_WPS)			\
		WPP_DEFINE_BIT(COMP_CMD)			\
		WPP_DEFINE_BIT(COMP_EFUSE)			\
		WPP_DEFINE_BIT(COMP_CCX)			\
		WPP_DEFINE_BIT(COMP_IOCTL)			\
		WPP_DEFINE_BIT(COMP_HVL)			\
		WPP_DEFINE_BIT(COMP_TEST)			\
		WPP_DEFINE_BIT(COMP_P2P)			\
		WPP_DEFINE_BIT(COMP_TDLS)			\
		WPP_DEFINE_BIT(COMP_OMNIPEEK)		\
		WPP_DEFINE_BIT(COMP_MULTICHANNEL)	\
		WPP_DEFINE_BIT(COMP_TCPCHECKSUM)	\
		WPP_DEFINE_BIT(COMP_DUALMACSWITCH)	\
		WPP_DEFINE_BIT(COMP_EASY_CONCURRENT)\
		WPP_DEFINE_BIT(COMP_DFS)			\
		WPP_DEFINE_BIT(COMP_TXPWR)			\
		WPP_DEFINE_BIT(COMP_HAL_MAC_API)	\
		WPP_DEFINE_BIT(COMP_NAN)			\
	)	\
	WPP_DEFINE_CONTROL_GUID(dbgCtrlGuid,(1bb4a286,2d36,463b,bee4,47d6ba05b5f8), \
		WPP_DEFINE_BIT(COMP_DBG_MON)			\
		WPP_DEFINE_BIT(COMP_TX_HANG)			\
		WPP_DEFINE_BIT(COMP_RX_HANG)			\
	)	\
	WPP_DEFINE_CONTROL_GUID( halMacGuid,(9f9d9e5b,3854,4b87,8bc6,4ce1f284d34b), \
		WPP_DEFINE_BIT(COMP_HALMAC_MSG_INIT) 	\
		WPP_DEFINE_BIT(COMP_HALMAC_MSG_EFUSE)	\
		WPP_DEFINE_BIT(COMP_HALMAC_MSG_FW)		\
		WPP_DEFINE_BIT(COMP_HALMAC_MSG_H2C)		\
		WPP_DEFINE_BIT(COMP_HALMAC_MSG_PWR)		\
	)

	#ifdef WPP_COMPID_LEVEL_ENABLED
		#undef WPP_COMPID_LEVEL_ENABLED
	#endif

	#define WPP_COMPID_LEVEL_ENABLED(CTL,LEVEL)             \
		((WPP_CONTROL(WPP_BIT_ ## CTL).Level >= LEVEL) &&   \
		(WPP_CONTROL(WPP_BIT_ ## CTL).Flags[WPP_FLAG_NO(WPP_BIT_ ## CTL)] & WPP_MASK(WPP_BIT_ ## CTL)))

	#ifndef WPP_COMPID_LEVEL_LOGGER
		#define WPP_COMPID_LEVEL_LOGGER(CTL,LEVEL)      (WPP_CONTROL(WPP_BIT_ ## CTL).Logger),
	#endif

	#ifdef WPP_COMPID_ENABLED
		#undef WPP_COMPID_ENABLED
	#endif

	#define WPP_COMPID_ENABLED(CTL)		\
		(WPP_CONTROL(WPP_BIT_ ## CTL).Flags[WPP_FLAG_NO(WPP_BIT_ ## CTL)] & WPP_MASK(WPP_BIT_ ## CTL))

	#ifndef WPP_COMPID_LOGGER
		#define WPP_COMPID_LOGGER(CTL)	(WPP_CONTROL(WPP_BIT_ ## CTL).Logger),
	#endif
	
#else // WPP_SOFTWARE_TRACE == 0


//-----------------------------------------------------------------------------
// Define the tracing components
//
//-----------------------------------------------------------------------------
#define COMP_DBG				BIT0		// For function call tracing.
#define COMP_INIT				BIT1	// during driver initialization / halt / reset.
#define COMP_OID_QUERY			BIT2		// Query OID.
#define COMP_OID_SET			BIT3		// Set OID.
#define COMP_RECV				BIT4	// Reveive part data path.
#define COMP_SEND				BIT5		// Send part path.
#define COMP_IO					BIT6		// I/O Related. Added by Annie, 2006-03-02.
#define COMP_POWER				BIT7		// 802.11 Power Save mode or System/Device Power state related.
#define COMP_MLME				BIT8	// 802.11 link related: join/start BSS, leave BSS.
#define COMP_SCAN				BIT9	// For site survey.
#define COMP_SYSTEM			BIT10	// For general platform function.
#define COMP_SEC				BIT11	// For Security.
#define COMP_AP					BIT12	// For AP mode related.
#define COMP_QOS				BIT13	// For QoS.
#define COMP_AUTHENTICATOR	BIT14	// For AP mode Authenticator. Added by Annie, 2006-01-30.
#define COMP_BEACON			BIT15	// For Beacon related, by rcnjko.
#define COMP_WFD				BIT16	// WiFi Display
#define COMP_RM					BIT17	// For Radio Measurement. 
#define COMP_MP					BIT18	// For mass production test, by shien chang, 2006.07.13
#define COMP_CKIP				BIT19	// For CCX 1 S13: CKIP. Added by Annie, 2006-08-14.
#define COMP_RSNA				BIT20	// For RSNA IBSS , 061201, by CCW. 
#define COMP_INDIC				BIT21	// For link indication
#define COMP_LED				BIT22	// For LED.
#define COMP_RF					BIT23	// For RF.

//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
//1//1Attention Please!!!<11n or 8190 specific code should be put below this line>
//1!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define COMP_HT					BIT24	// For 802.11n HT related information. by Emily 2006-8-11
#define COMP_POWER_TRACKING	BIT25	//FOR 8190 TX POWER TRACKING
#define COMP_RX_REORDER		BIT26	// 8190 Rx Reorder
#define COMP_AMSDU				BIT27	// For A-MSDU Debugging
#define COMP_WPS				BIT28   //WPS Debug Message 
#define COMP_CMD				BIT29
#define COMP_EFUSE				BIT30
#define COMP_CCX				BIT31	//CCX Debug Flag
#define COMP_IOCTL				BIT32	// IO Control
#define COMP_HVL                           BIT33     // For Ndis 6.2 Context Swirch and Hardware Virtualiztion Layer
#define COMP_TEST				BIT34
#define COMP_P2P				BIT35
#define COMP_TDLS				BIT36
#define COMP_OMNIPEEK			BIT37
#define COMP_MULTICHANNEL		BIT38
#define COMP_TCPCHECKSUM		BIT39
#define COMP_DUALMACSWITCH	BIT40	// 2010/12/27 Add for Dual mac mode debug
#define COMP_EASY_CONCURRENT	BIT41	// 2010/12/27 Add for easy cncurrent mode debug
#define COMP_DFS				BIT42
#define COMP_COEX				BIT43
#define	COMP_TXPWR				BIT44
#define COMP_HAL_MAC_API		BIT46
#define COMP_NAN				BIT47

#define COMP_ALL				UINT64_C(0xFFFFFFFFFFFFFFFF) // All components


// ==============================
//	for dbgCtrlGuid
//	(1bb4a286,2d36,463b,bee4,47d6ba05b5f8)
// ==============================
#define	COMP_DBG_MON			BIT0
#define	COMP_TX_HANG			BIT1
#define	COMP_RX_HANG			BIT2

// ==============================
//	for halMacGuid
//	{9F9D9E5B-3854-4B87-8BC6-4CE1F284D34B}
// ==============================
#define	COMP_HALMAC_MSG_INIT 	BIT0
#define	COMP_HALMAC_MSG_EFUSE 	BIT1
#define	COMP_HALMAC_MSG_FW		BIT2
#define	COMP_HALMAC_MSG_H2C		BIT3
#define	COMP_HALMAC_MSG_PWR		BIT4

#endif


// For debug print flag to use
/*------------------------------Define structure----------------------------*/
/* 2007/07/13 MH  *//*------For DeBuG Print modeue------*/

/* Defnie structure to store different debug flag variable. Every debug flag 
     is a UINT32 integer and you can assign 32 different events. */
typedef struct tag_DBGP_Debug_Flag_Structure
{
	UINT32	Mans;			/* Main Scheduler module. */
	UINT32	Rtos;			/* RTOS module. */
	UINT32	Alarm;		/* Alarm module. */	
	UINT32	Pm;			/* Performance monitor module. */	
}DBGP_FLAG_T;

/* Define debug print header for every service module.*/
typedef struct tag_DBGP_Service_Module_Header_Name_Structure
{
	const char 	*pMANS;
	const char 	*pRTOS;
	const char 	*pALM;
	const char 	*pPEM;
	const char 	*pCMPK;
	const char 	*pRAPD;
	const char 	*pTXPB;
	const char 	*pQUMG;
}DBGP_HEAD_T;


/* Define different debug flag for dedicated service modules in debug flag array. */
// Each module has independt 32 bit debug flag you cnn define the flag as yout require.
typedef enum tag_DBGP_Flag_Type_Definition
{
	FQoS				= 0,	
	FTX					= 1,
	FRX					= 2,	
	FSEC				= 3,
	FMGNT				= 4,
	FMLME				= 5,
	FRESOURCE			= 6,
	FBEACON				= 7,
	FISR				= 8,
	FPHY				= 9,
	FMP					= 10,
	FEEPROM				= 11,
	FPWR				= 12,
	FDM					= 13,
	FDBG_CTRL			= 14,
	FC2H				= 15,
	FBT					= 16,
	FINIT				= 17,
	FIOCTL				= 18,
	FSHORT_CUT			= 19,
	FBATCH_INDICATION	= 20,
	FTCPREORDER			= 21,
	FIOTHREAD			= 22,
	FFW 				= 23,
	FMPBT				= 24,
	FRW_POINT			= 25,
	FBEAM				= 26,
	FMPHAL				= 27,
	FACTION_TIMER		= 28,
	FHAL_MAC_ID			= 29,
	FCHNL				= 30,
	FCCX				= 31,
	FUSWITCH			= 32,
	DBGP_TYPE_MAX
}DBGP_FLAG_E;


// Define Qos Relative debug flag bit		--> FQoS
#define		QoS_INIT			BIT0
#define		QoS_VISTA			BIT1

// Define TX relative debug bit				--> FTX
#define		TX_DESC			BIT0
#define		TX_DESC_TID		BIT1
#define		TX_PATH			BIT2

// Define RX relative debug  bit				--> FRX
#define		RX_DATA				BIT0	
#define		RX_PHY_STS				BIT1
#define		RX_PHY_SS				BIT2
#define		RX_PHY_SQ				BIT3
#define		RX_PHY_ASTS			BIT4
#define		RX_ERR_LEN			BIT5
#define		RX_DEFRAG			BIT6
#define		RX_ERR_RATE			BIT7
#define		RX_PATH				BIT8
#define		RX_BEACON				BIT9
#define		RX_PATH_AP_MODE		BIT10
#define		RX_PATH_ADDR			BIT11

// Define Security relative debug  bit			--> FSEC

// Define MGNT relative debug bit			--> FMGNT

// Define MLME relative debug bit				--> FMLME
#define		MEDIA_STS			BIT0
#define		LINK_STS			BIT1

// Define OS resource check module bit		--> FRESOURCE
#define		OS_CHK				BIT0

// Define beacon content check module bit		--> FBEACON
#define		BCN_SHOW			BIT0
#define		BCN_PEER			BIT1

// Define ISR/IMR check module bit		--> FISR
#define		ISR_CHK				BIT0

// Define PHY-BB/RF/MAC check module bit		--> FPHY
#define		PHY_BBR				BIT0
#define		PHY_BBW				BIT1
#define		PHY_RFR				BIT2
#define		PHY_RFW				BIT3
#define		PHY_MACR				BIT4
#define		PHY_MACW				BIT5
#define		PHY_ALLR				BIT6
#define		PHY_ALLW				BIT7
#define		PHY_TXPWR				BIT8
#define		PHY_PWRDIFF			BIT9
#define		PHY_SICR				BIT10
#define		PHY_SICW				BIT11
#define		PHY_TXPWR_EFUSE			BIT12

// Define MPT driver check module bit		--> FMP
#define		MP_RX					BIT0
#define		MP_SWICH_CH			BIT1

// Define EEPROM and EFUSE  check module bit		--> FEEPROM
#define		EEPROM_W					BIT0
#define		EFUSE_PG					BIT1
#define		EFUSE_READ_ALL			BIT2
#define		EFUSE_ANALYSIS				BIT3
#define		EFUSE_PG_DETAIL			BIT4

// Define power save  check module bit		--> FPWR
#define		LPS					BIT0
#define		IPS					BIT1
#define		PWRSW				BIT2
#define		PWRHW				BIT3
#define		PWRHAL				BIT4

// Define Dynamic Mechanism check module bit		--> FDM
#define		WA_IOT				BIT0
#define		DM_PWDB			BIT1
#define		DM_Monitor			BIT2
#define		DM_DIG				BIT3
#define		DM_EDCA_Turbo		BIT4
#define		DM_BT30			BIT5

// Define Dbg Control module bit		--> FDBG_CTRL
#define		DBG_CTRL_TRACE			BIT0
#define		DBG_CTRL_INBAND_NOISE	BIT1

// Define FW C2H Cmd check module bit		--> FC2H
#define		C2H_Summary				BIT0
#define		C2H_PacketData				BIT1
#define		C2H_ContentData			BIT2
// Define BT Cmd check module bit		--> FBT
#define		BT_TRACE					BIT0
#define		BT_DBG_STATE				BIT1
#define		BT_DBG_CONTENT				BIT2
#define		BT_H2C_STATE				BIT3
#define		BT_NOTIFY_EVENT				BIT4
#define		BT_DBG_CTRL					BIT5

// Define init check for module bit		--> FINIT
#define		INIT_EEPROM				BIT0
#define		INIT_TxPower				BIT1
#define		INIT_IQK					BIT2
#define		INIT_RF						BIT3

// Define IOCTL Cmd check module bit		--> FIOCTL
// section 1 : IRP related
#define		IOCTL_IRP						BIT0
#define		IOCTL_IRP_DETAIL				BIT1
#define		IOCTL_IRP_HANDLE				BIT2
// section 2 : HCI command/event
#define		IOCTL_BT_HCICMD				BIT8
#define		IOCTL_BT_HCICMD_DETAIL		BIT9
#define		IOCTL_BT_HCICMD_EXT			BIT10
#define		IOCTL_BT_EVENT					BIT11
#define		IOCTL_BT_EVENT_DETAIL			BIT12
#define		IOCTL_BT_EVENT_PERIODICAL		BIT13
// section 3 : BT tx/rx data and throughput
#define		IOCTL_BT_TX_ACLDATA			BIT16
#define		IOCTL_BT_TX_ACLDATA_DETAIL	BIT17
#define		IOCTL_BT_RX_ACLDATA			BIT18
#define		IOCTL_BT_RX_ACLDATA_DETAIL	BIT19
#define		IOCTL_BT_TP					BIT20
// section 4 : BT connection state machine.
#define 		IOCTL_STATE					BIT21	
#define		IOCTL_BT_LOGO					BIT22
#define		IOCTL_BT_MGNT_PACKET			BIT23
// section 5 : BT function trace
#define		IOCTL_CALLBACK_FUN			BIT24
#define		IOCTL_PARSE_BT_PKT			BIT25
#define		IOCTL_BT_TX_PKT				BIT26
#define		IOCTL_BT_FLAG_MON				BIT27

//
// Define init check for module bit		--> FSHORT_CUT
// 2011/07/20 MH Add for short but definition.
//
#define		SHCUT_TX				BIT0
#define		SHCUT_RX				BIT1
//
// Define init check for module bit		--> FBATCH_INDICATION
// 2011/09/28 Sinda Add for batch indication definition.
//
#define		INDICATE_TRACE				BIT0


//
// Define init check for module bit		--> FTCPREORDER
// 2012/01/10 hpfan Add for Tcp Reorder definition
//
#define		SEQ_TRACE						BIT0
#define		REORDER_TRACE					BIT1
#define		INIT_TRACE						BIT2

//
// Define init check for module bit		--> FIOTHREAD
// 2012/01/04 Cosa Add for io thread definition.
//
#define		IOTHREAD_INIT				BIT0
#define		IOTHREAD_Q_OP				BIT1
#define		IOTHREAD_STATE				BIT2
#define		IOTHREAD_H2C				BIT3

//
// Define init check for module bit		--> FFW
// 2012/01/12 Cosa Add for Firmware definition.
//
#define		FW_MSG_INIT					BIT0
#define		FW_MSG_Q_OP					BIT1
#define		FW_MSG_H2C_STATE			BIT2
#define		FW_MSG_H2C_CONTENT			BIT3
#define		FW_MSG_RSVD					BIT4

//
// Define init check for module bit		--> FMPBT
// 2012/03/01 Cosa Add for BT MP.
//
#define		FMPBT_CONTENT						BIT0
#define		FMPBT_STATE							BIT1
#define		FMPBT_H2C_CONTENT					BIT2
#define		FMPBT_H2C_STATE						BIT3
#define		FMPBT_C2H_CONTENT					BIT4
#define		FMPBT_C2H_STATE						BIT5

#define		FMPHAL_CONTENT						BIT0
#define		FMPHAL_STATE						BIT1

//
// Define 8192ee Read Write point related bit	--> FRW_POINT
// 2012/03/01 Cosa Add for BT MP.
//
#define		FRW_POINT_TX					BIT0
#define		FRW_POINT_TX_DATA				BIT1
#define		FRW_POINT_RX					BIT2
#define		FRW_POINT_RX_DATA			BIT3
#define		FRW_POINT_DBG					BIT4

#define		FBEAM_FUN						BIT0
#define		FBEAM_ERROR					BIT1
#define		FBEAM_DATA					BIT2

//
// Define ACTION_TIMER Debug Information	--> FACTION_TIMER
//
#define		COMP_ACTION_TIMER_LIFE		BIT0
#define		COMP_ACTION_TIMER_ITEM		BIT1

//
// Define HAL_MAC_ID Debug Information	--> FHAL_MAC_ID
//
#define		COMP_MAC_ID					BIT0

//
// Define CHANNEL Debug Information	--> FCHNL
//
#define		FCHNL_FUN					BIT0
#define		FCHNL_ERROR				BIT1
#define		FCHNL_INFO					BIT2

//
// Define CCX Debug Information	--> FCCX
//
#define		CCX_CELLPWRLMT			BIT0
#define		CCX_RM						BIT1
#define		CCX_CAC					BIT2

#define		FUSWITCH_FUN				BIT0
#define		FUSWITCH_ERROR			BIT1
#define		FUSWITCH_INFO				BIT2

/* 2007/07/13 MH  *//*------For DeBuG Print modeue------*/
/*------------------------------Define structure----------------------------*/


/*------------------------Export Marco Definition---------------------------*/
#define		DEBUG_PRINT				1

// Please add new OS's print API by yourself

#if (DEBUG_PRINT == 1) && DBG
#define	RT_DISP(dbgtype, dbgflag, printstr)\
{\
	if (DBGP_Type[dbgtype] & dbgflag)\
	{\
		DbgPrint printstr;\
	}\
}

#define	RT_DISP_F(dbgtype, dbgflag, printstr)\
{\
	if (DBGP_Type[dbgtype] & dbgflag)\
	{\
		DbgPrint("%s(): ", __FUNCTION__);\
		DbgPrint printstr;\
	}\
}

#define	RT_DISP_ADDR(dbgtype, dbgflag, printstr, _Ptr)\
{\
	if (DBGP_Type[dbgtype] & dbgflag)\
	{\
				int __i;						\
				pu1Byte	ptr = (pu1Byte)_Ptr;	\
				DbgPrint printstr;				\
				DbgPrint(" ");					\
				for( __i=0; __i<6; __i++ )		\
					DbgPrint("%02X%s", ptr[__i], (__i==5)?"":"-");		\
				DbgPrint("\n");							\
	}\
}

#define 	RT_DISP_DATA(dbgtype, dbgflag, _TitleString, _HexData, _HexDataLen)\
{\
	if (DBGP_Type[dbgtype] & dbgflag)\
	{\
		int __i;									\
		pu1Byte	ptr = (pu1Byte)_HexData;			\
		DbgPrint(_TitleString);					\
		for( __i=0; __i<(int)_HexDataLen; __i++ )	\
		{										\
			DbgPrint("%02X%s", ptr[__i], (((__i + 1) % 4) == 0)?"  ":" ");\
			if (((__i + 1) % 16) == 0)	DbgPrint("\n");\
		}										\
		DbgPrint("\n");							\
	}\
}

#define 	FunctionIn(_comp)		RT_TRACE((_comp), DBG_LOUD, ("==========> %s\n",  __FUNCTION__))
#define 	FunctionOut(_comp)		RT_TRACE((_comp), DBG_LOUD, ("<========== %s\n",  __FUNCTION__))

#else

#define	RT_DISP(dbgtype, dbgflag, printstr)
#define	RT_DISP_F(dbgtype, dbgflag, printstr)

#define	RT_DISP_ADDR(dbgtype, dbgflag, printstr, _Ptr)
#define 	RT_DISP_DATA(dbgtype, dbgflag, _TitleString, _HexData, _HexDataLen)

#define 	FunctionIn(_comp)
#define 	FunctionOut(_comp)
#endif
/*------------------------Export Marco Definition---------------------------*/


/*------------------------Export global variable----------------------------*/
extern	UINT32			DBGP_Type[DBGP_TYPE_MAX];
extern	DBGP_HEAD_T	DBGP_Head;

/*------------------------Export global variable----------------------------*/


/*--------------------------Exported Function prototype---------------------*/
extern	void	DBGP_Flag_Init(void);
extern	void	DBG_PrintAllFlag(void);
extern	void	DBG_PrintFlagEvent(UINT8	DbgFlag);

/*--------------------------Exported Function prototype---------------------*/









extern u4Byte GlobalDebugLevel;
extern u8Byte GlobalDebugComponents;
extern u4Byte GLDebugComponents;
extern u4Byte HalMacDebugComponents;



//=================================================================
// Debugging for System Resources
//=================================================================


// + Debugging for Timer Resources ------------------------------------------------------------------
#define	DEBUG_RESOURCE_TIMER		0


#define DebugResourceTimerCreateTimerMirror(_pAdapter, _uStorageIndex, _pTimer)
#define DebugResourceTimerReleaseTimerMirror(_pAdapter, _pTimer)
#define DebugResourceTimerDumpUnreleasedTimer(_pAdapter)

//-----------------------------------------------------------------------------------------------




//=================================================================

#endif //#ifndef __INC_DEBUG_H
