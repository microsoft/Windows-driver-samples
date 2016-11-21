/*****************************************************************************
 *	Copyright(c) 2007,  RealTEK Technology Inc. All Right Reserved.
 *
 * Module:		hal90DbgCmd.h	(RTL8190  Header H File)
 *
 *
 * Note:		The header file records the command format between UI and 
 *				driver. And necessary constant for debug command handler.
 *			
 *
 * Export:		
 *
 * Abbrev:			
 *
 * History:		
 *	Data		Who		Remark 
 *	05/30/2007  MHC    	Create initial version.
 *	05/30/2007	MHC		1. Support debug command from UI. We define command 
 *						   format between driver and UI.
 *						2. We copy debug command definition from firmware.
 *	06/07/2007	MHC		We do not use MIPS GNU header file to support sprintf.
 *	06/08/2007	MHC		Add RX command format structure.
 *	06/11/2007	MHC		1. Copy debug command strcuture from MP UI. Programmer 
 *						   must main the table when they add command.
 *	06/15/2007	MHC		1. Define current debug command index and do test. We do
 *						   not need to send the header files to UI and UI can 
 *						   support the new command immediately.
 *						2. Add directory definition for driver. 
 *	07/13/2007	MHC		1. Define debug print macro which is controlled by 
 *						   different debug flag.
 *						2. Define different debug flag.
 *	07/31/2007	MHC		1. Add debug command to count ISR time.
 *	08/15/2007	MHC		Add debug command index for other commands.
 *	08/16/2007	MHC		Extern global variable and API for debug command betwen
 *						UI and firmware.
 * 
******************************************************************************/
 /* Check to see if the file has been included already.  */
#ifndef	__HAL90_DEBUG_COMMAND_H__
#define __HAL90_DEBUG_COMMAND_H__


/*--------------------------Define Parameters-------------------------------*/
#define		DBG_MODE_EVENT				0x00000001
#define		DCMD_MAX_DIRECTORY			1000
#define		DCMD_STRING_LENGTHH			32
#define		DCMD_MAX_NUM				1000


/* 2007/03/13 MH Support byte/word/long R/W check. */
#define		DCMD_BYTE					1
#define		DCMD_WORD					2
#define		DCMD_LONG					4



/* 2007/05/30 MH Define constatn for debug command from UI. */
#define		DCMD_DRIVER_CMD						0
#define		DCMD_FIRMWARE_CMD					1

/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 
typedef struct tag_Debug_CoMmanD
{
    char	*name;				/* Command string */
    char	*desc;				/* Description */
    char	*usage;				/* Usage of the command */
    int		directory;     		/* Directory */
    int		usage_counter; 		/* Usage counter for optimization */
    int		(*cmd_func)(IN	PADAPTER	pAdapter,
    					IN	INT32		Cmd_Idx, 
    					IN	INT32		Argc, 
    					IN	INT8		*pArgs[]);    
    							/* Command function pointer */
}DCMD_T;

/* 2007/06/15 MH Define driver debug command's directory. */
typedef enum tag_CMD_Directories
{    
	/* General command. */    
    COMMON = 1,
    DYNAMIC_SCHEME = 2,
	//Add for 92S shared commands
	CMD_92S = 3,
    TX_PROCESS_CMD,    
    RX_PROCESS_CMD,
    SECURITY_CMD,
    /* Private command. You can add according your requirement. */
    PRIVATE = 11,
    MY_LOVELY_CMD,
    MY_DISLIKE_CMD,
    
    /* Directory to display all command index. */
    CMD_INDEX_DISPLAY = 100,
    
    /* Last directory definition. */
    OTHERS
}DCMD_DIRECTORY_E;

typedef enum tag_DCMD_Status_Code
{
    CMD_EXECUTE_OK = 0,
    CMD_EXECUTE_FAIL,
    CMD_PASSWORD_OK,
    CMD_PASSWORD_FAIL,
    CMD_ARGUMENT_FAIL,
    CMD_NO_MATCH_COMMAND,
    CMD_NOT_COMMAND,
    CMD_NEW_COMMAND,
    CMD_PREVIOUS_COMMAND
}DCMD_STATUS_E;

/* 2006/11/28 MH We only support below three type for scanf. */
typedef enum tag_DCMD_Scanf_Type_Definition
{
	DCMD_DECIMAL = 1,
	DCMD_HEX,
	DCMD_CHAR,
	DCMD_SCAN_LIMIT = 10
}DCMD_STP_DEF_E;

/* 2007/05/30 MH Define TX command format from UI to driver. */
#define		MAX_ARGC				20				// Only used here.
#define		MAX_ARGV				16				// Only used here.
typedef struct tag_Cmd_Format_UI_To_Driver
{
	INT32	Op;										/* Command packet type. */																			
	INT32	Length;									/* Command packet length. */
	INT32	Cmd_Idx;								/* Command index. */																				
	INT32	Argc;									/* Argument counter. */																				
	INT8	Argv[MAX_ARGC][MAX_ARGV];				/* Argument value array. */	
}DCMD_TMSG_T, *PDCMD_TMSG_T;

/* 2007/06/08 MH Define RX command format from driver to UI.  */
#define		MAX_RETURN_INFO_SIZE	240				// Only used here.
#define		RX_HEADER_SIZE			16				//OP+status+segment+length
typedef struct tag_Cmd_Format_Driver_To_UI
{
	INT32	Op;										/* Command packet type. */																			
	INT32	Status;									/* Command status0/1/2. */
	INT32	Segment;								/* Command segment num. */																				
	INT32	Length;									/* Return info size. */																				
	INT8	ReturnInfo[MAX_RETURN_INFO_SIZE];		/* Argument value array. */	
}DCMD_RMSG_T;

/* The debug command table is used for UI to do parser. */
typedef enum _DBG_DRIVER_CMD_INDEX{
	DBG_CMD_HELP				= 0	,
	DBG_CMD_REG_READ			= 1	,
	DBG_CMD_REG_WRITE			= 2	,
	DBG_CMD_ADAPTER_READ		= 3	,
	DBG_CMD_ADAPTER_WRITE		= 4	,
	DBG_CMD_RF_READ				= 5	,	
	DBG_CMD_UI_DEBUG        	= 6 ,
	DBG_CMD_FLAG_CTRL			= 7 ,
	DBG_CMD_RF_WRITE			= 8	,	
	DBG_CMD_REG_R_2BYTE			= 9 ,
	DBG_CMD_REG_W_2BYTE			= 10,
	DBG_CMD_REG_R_1BYTE			= 11,
	DBG_CMD_REG_W_1BYTE			= 12,
	DBG_CMD_DYNAMIC_CTRL		= 13,
	DBG_CMD_BANDWIDTH_SWITCH	= 14,
	DBG_CMD_TX_RATE_HISTORY		= 15,
	DBG_CMD_TX_FW_CTRL			= 16,
	DBG_CMD_FORCE_TX_FW_INFO	= 17,
	DBG_CMD_FW_DYNAMIC_CTRL		= 18 ,
	DBG_CMD_RX_PATH_SELECTION	= 19,
	DBG_CMD_92SE_FPGA_TEST		= 20,
	DBG_CMD_INFO_DUMP			= 21,
	DBG_CMD_92S_FWBUF_DUMP		= 22,
	DBG_CMD_SEC_CTRL				= 23,
	DBG_CMD_REG_DUMP				= 24,
	DBG_CMD_92S_RESET			= 25,
	DBG_CMD_EEPROM_READ			= 26,
	DBG_CMD_EFUSE_READ			= 27,
	DBG_CMD_SHADOW_OPERATE		= 28,
	DBG_CMD_DBG_CONTROL			= 29,
	DBG_CMD_USB_PHYR				= 30,
	DBG_CMD_USB_PHYW				= 31,
	DBG_CMD_SHORTCUT				= 32,
	DBG_CMD_INDICATE_SETTING		= 33,
	DBG_CMD_MAX_IDX
}DBG_DRIVER_CMD_INDEX;

typedef enum _DBG_CONTROL_TYPE
{
	DBG_CTRL_PWRSAVE_DISABLE	= 0,
	DBG_CTRL_INBANDNOISE			= 1,
	DBG_CTRL_LINK_QUALITY			= 2,
	DBG_CTRL_SIGNAL_STRENGTH		= 3,
	DBG_CTRL_PSD_MONITOR			= 4,
	DBG_CTRL_BT_PROFILE				= 5,
	DBG_CTRL_BT_COEX_ENABLE			= 6,
	DBG_CTRL_TEST					= 7,
	DBG_CTRL_BT_HCI_INFO				= 8,
	DBG_CTRL_BT_HS_INFO				= 9,
	DBG_CTRL_RESET_BT_DBG			= 10,
	DBG_CTRL_BT_COEX_INFO			= 11,	
	DBG_CTRL_BT_LOGO_TEST			= 12,
	DBG_CTRL_SEND_H2C				= 13,
	DBG_CTRL_SET_EDCA				= 14,
	DBG_CTRL_SET_COEX_MODE			= 15,
	DBG_CTRL_DEC_BT_PWR				= 16,
	DBG_CTRL_CTRL_AGGREGATION		= 17,
	DBG_CTRL_AGGREGATE_BUF_SIZE		= 18,
	DBG_CTRL_RX_RATE_INFO			= 19,
	DBG_CTRL_SET_DIG				= 20,
	DBG_CTRL_BT_AFH_MAP				= 21,
	DBG_CTRL_BT_IGNORE_WLAN_ACT		= 22,
	DBG_CTRL_BB_INFO				= 23,
	DBG_CTRL_SET_DBG_MON_COMP		= 24,
	DBG_CTRL_SHOW_WIFI_BASIC		= 25,
	DBG_CTRL_SHOW_TX_INFO			= 26,
	DBG_CTRL_SHOW_NDIS_INFO			= 27,
	DBG_CTRL_SHOW_RX_INFO			= 28,
	DBG_CTRL_SHOW_BASIC_PROFILE     = 29,
	DBG_CTRL_SHOW_WOL_INFO			= 30,
	DBG_CTRL_SHOW_PWR_SVG_INFO		= 31,
	DBG_CTRL_SHOW_antdet_INFO		= 32,
	DBG_CTRL_SHOW_SKU_INFO			= 33,

	DBG_CTRL_SET_RX_REORDER               = 34,  //2015/01/21 gary add
	DBG_CTRL_SET_TX_HT_SUPPORT_CAP    = 35,  //2015/01/22 gary add
	DBG_CTRL_SET_TX_VHT_SUPPORT_CAP  = 36,
	DBG_CTRL_SET_RX_HT_SUPPORT_CAP    = 37,
	DBG_CTRL_SET_RX_VHT_SUPPORT_CAP  = 38,
	DBG_CTRL_SHOW_TX_USB_AGG_INFO           = 39,
	DBG_CTRL_SHOW_RX_USB_AGG_INFO           = 40,
	DBG_CTRL_SET_TX_USB_AGG                        = 41,
	DBG_CTRL_SET_RX_USB_AGG                        = 42,

	DBG_CTRL_BB_RXHANG_INFO			= 43,
	DBG_CTRL_SET_PWR_TABLE			= 44,
	DBG_CTRL_SHOW_CHANNELPLAN_INFO			= 45,
	DBG_CTRL_DUMP_PWR_TABLE			= 46,
	DBG_CTRL_SHOW_RSSI_INFO			= 47,
	/** in order to work with Sigma Console **
	 ************use value 29,30,31***********/
	DBG_CTRL_NAN_OPERATION			= 48,//29,
	DBG_CTRL_NAN_STATE			    = 49,//30,
	DBG_CTRL_NAN_VARIABLE  			= 50,//31,
	/***********************************/

	DBG_CTRL_MAX
}DBG_CONTROL_TYPE;

#define	DCMD_EFUSE_MAP_MAX_LEN		1024
#define	DCMD_EFUSE_MAX_SECTION_NUM	128



// 2008/03/26 MH We define a simple tx command format for 92 series. No dbg support.
typedef struct tag_Tx_Config_Cmd_Format
{
	UINT32	Op;										/* Command packet type. */																			
	UINT32	Length;									/* Command packet length. */
	UINT32	Value;
}DCMD_TXCMD_T, *PDCMD_TXCMD_T;

/* The simple tx command OP code. */
typedef enum _tag_TxCmd_Config_Index{
	TXCMD_TXRA_HISTORY_CTRL				= 0xFF900000,
	TXCMD_RESET_TX_PKT_BUFF				= 0xFF900001,
	TXCMD_RESET_RX_PKT_BUFF				= 0xFF900002,
	TXCMD_SET_TX_DURATION				= 0xFF900003,
	TXCMD_SET_RX_RSSI						= 0xFF900004,
	TXCMD_SET_TX_PWR_TRACKING			= 0xFF900005,
	TXCMD_XXXX_CTRL,
}DCMD_TXCMD_OP;
/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
//extern	DCMD_T dcmd_Commands[];
#if 0	// Move to debug.h
extern	UINT32			DBGP_Type[DBGP_TYPE_MAX];
extern	DBGP_HEAD_T		DBGP_Head;
#endif
extern	INT32			DCMD_OP_Type;

// for test
//extern	UINT8	nheader[100][24];
//extern	UINT8	llcheader[100][24];


// Declare for 92S debug, delete later !!!
extern	u4Byte	usb_speed;
/*------------------------Export global variable----------------------------*/


/*------------------------Export Marco Definition---------------------------*/
#define		dcmd_sprintf			RtlStringCchVPrintfW

/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/
extern	INT32	DCMD_Message_Handler_Tx(IN	PADAPTER	pAdapter,
										IN	ULONG		length,
										IN	PVOID	pBuffer);
extern	INT32	DCMD_Message_Handler_Check(	IN 	ULONG	Length, 
											OUT	PVOID	pBuffer, 
											OUT	ULONG	*pLength);
extern	INT32	DCMD_Message_Handler_Rx(IN	INT32	DbgType,
										IN 	ULONG	Length, 
										OUT	PVOID	pBuffer, 
										OUT	ULONG	*pLength);
extern	INT32	DCMD_Save_FW_Dbg_Info(	IN 	ULONG	Length, 
										IN	PVOID	pBuffer);


void
DbgCmdWorkItemCallBack(
    IN PVOID       pContext
);


#define DCMD_Printf(_pMsg)
#define DCMD_Scanf(_pInput, _Type, _pOutput)



/*--------------------------Exported Function prototype---------------------*/


#endif	/* __HAL90_DEBUG_COMMAND_H__ */


/* End of hal90DbgCmd.h */
