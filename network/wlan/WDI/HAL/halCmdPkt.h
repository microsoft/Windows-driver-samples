/*****************************************************************************
 *	Copyright(c) 2006,  RealTEK Technology Inc. All Right Reserved.
 *
 * Module:	hal90cmdpkt.h	(RTL8190 TX/RX command packet handler Header H File)
 *
 *
 * Note:	We define different command packets' structure and defintion.
 *			Please refer to doc ws-06-0063-rtl8190-command-packet-specification.
 *			1. The defintiion and structure will be the same in DRV and FW.
 *			
 *
 * Export:	Constants, macro, functions(API), global variables(None).
 *
 * Abbrev:	SZ	:	Size
 *			.....For example.
 *
 * History:
 *		Data		Who		Remark 
 *      01/15/2007  MHC    	Create initial version. Define comadn packet 
 *							structure and element definition.
 *		01/16/2007	MHC		According to document, we add defintion in dedicated
 *							field and corresponding structure.
 *		07/05/2007	MHC		Change tx feedback packet structure to fit format
 *							from firmware.
 *		10/09/2007	MHC		Accept all USB/PCI structure and enum. Nerge the
 *							same structure or constant definition.
 * 
 *****************************************************************************/
 /* Check to see if the file has been included already.  */
#ifndef	__HAL90_CMD_PKT_H__
#define __HAL90_CMD_PKT_H__


/*--------------------------Define Parameters-------------------------------*/
/* 2007/01/15 MH Define some constant for comand packet handler module. */
#define 	KB      		1024
#define 	MByte      		KB*KB


/* Define frequently used type definition. */
/* INT8 is defined in WINDDK/3790/inc/wxp/basetsd.h:30. */
//typedef 	char			INT8;
typedef 	short			INT16;
//typedef 	long			INT32;

#if (RT_PLATFORM != PLATFORM_LINUX) && (RT_PLATFORM != PLATFORM_MACOSX)
typedef 	unsigned char	UINT8;
typedef 	unsigned short	UINT16;
//typedef 	unsigned long	UINT32;
#endif

typedef 	void 			(*VOIDFUNCPTR) (void);
typedef 	int  			(*INTFUNCPTR) (void);

/* Different command packet have dedicated message length and definition. */
#define		CMPK_RX_TX_FB_SIZE					sizeof(CMPK_TXFB_T)		//20
#define		CMPK_RX_BEACON_STATE_SIZE			sizeof(CMPK_BCN_STA_T)	//4
#define		CMPK_TX_SET_CONFIG_SIZE				sizeof(CMPK_SET_CFG_T)	//16
#define		CMPK_BOTH_QUERY_CONFIG_SIZE			sizeof(CMPK_SET_CFG_T)	//16
#define		CMPK_RX_TX_STS_SIZE					sizeof(CMPK_TX_STATUS_T)//
#define		CMPK_RX_DBG_MSG_SIZE			sizeof(CMPK_RX_DBGINFO_T)//
#define		CMPK_TX_RAHIS_SIZE			sizeof(CMPK_TX_RAHIS_T)//


/* 	Feedback info field. */
/* 1. Fail Reason encoding. */
#define		CMPK_RETRY_TIME_OUT					0x00	/* */
#define		CMPK_RETRY_MSDU_TIME_OUT			0x01	/* */
#define		CMPK_RETRY_DELAY_TIME_OUT			0x02	/* */

/* 2. TID : VO/VI/BE/BK, ... */

/* 3. Rate Mapping*/
#define		CMPK_CCK_1M						0x00	/* */
#define		CMPK_CCK_2M						0x01	/* */
#define		CMPK_CCK_5P5M					0x02	/* */
#define		CMPK_CCK_11M					0x03	/* */
#define		CMPK_OFDM_6M					0x04	/* */
#define		CMPK_OFDM_9M					0x05	/* */
#define		CMPK_OFDM_12M					0x06	/* */
#define		CMPK_OFDM_18M					0x07	/* */
#define		CMPK_OFDM_24M					0x08	/* */
#define		CMPK_OFDM_36M					0x09	/* */
#define		CMPK_OFDM_48M					0x0A	/* */
#define		CMPK_OFDM_54M					0x0B	/* */
#define		CMPK_HT_MCS0						0x80	/* */
#define		CMPK_HT_MCS1						0x81	/* */
#define		CMPK_HT_MCS2						0x82	/* */
#define		CMPK_HT_MCS3						0x83	/* */
#define		CMPK_HT_MCS4						0x84	/* */
#define		CMPK_HT_MCS5						0x85	/* */
#define		CMPK_HT_MCS6						0x86	/* */
#define		CMPK_HT_MCS7						0x87	/* */
#define		CMPK_HT_MCS8						0x88	/* */
#define		CMPK_HT_MCS9						0x89	/* */
#define		CMPK_HT_MCS10						0x8A	/* */
#define		CMPK_HT_MCS11						0x8B	/* */
#define		CMPK_HT_MCS12						0x8C	/* */
#define		CMPK_HT_MCS13						0x8D	/* */
#define		CMPK_HT_MCS14						0x8E	/* */
#define		CMPK_HT_MCS15						0x8F	/* */
#define		CMPK_HT_MCS32						0xA0	/* */

/* Configuration info field. */
/* 1. Type definition of Query/Set Configuration Element. */
#define		CMPK_CFG_MAC_REG					0x00
#define		CMPK_CFG_BB_REG						0x01
#define		CMPK_CFG_FW_ARG						0x02
#define		CMPK_CFG_TP_REV						0x03	/* Type reserve bit. */

/* 2. Size definition of Query/Set Configuration Element. */
#define		CMPK_CFG_SZ_REV						0x00	/* Size reserve bit. */
#define		CMPK_CFG_BYTE						0x01
#define		CMPK_CFG_WORD						0x02
#define		CMPK_CFG_DWORD						0x03

/* Define RX command queue ID. */
#define		CMPK_RX_QUEUE_ID					1

/* 2007/10/09 MH For USB consatnt. */
#define ISR_TxBcnOk					BIT27			// Transmit Beacon OK
#define ISR_TxBcnErr				BIT26			// Transmit Beacon Error
#define ISR_BcnTimerIntr			BIT13			// Beacon Timer Interrupt
/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 
/* Define different command packet structure. */
/* 1. RX side: TX feedback packet. */

#ifdef REMOVE_PACK
#pragma pack(1)
#endif

typedef struct tag_Cmd_Pkt_Tx_Feedback
{
	// DWORD 0
	UINT8	Element_ID;			/* Command packet type. */
	UINT8	Length;				/* Command packet length. */
	/* 2007/07/05 MH Change tx feedback info field. */
	/*------TX Feedback Info Field */
	UINT8	TID:4;				/* */	
	UINT8	Fail_Reason:3;		/* */		
	UINT8	TOK:1;				/* Transmit ok. */		
	UINT8	Reserve1:4;			/* */
	UINT8	Pkt_Type:2;		/* */
	UINT8	Bandwidth:1;		/* */
	UINT8	Qos_Pkt:1;			/* */	
	
	// DWORD 1	
	UINT8	Reserve2;			/* */
	/*------TX Feedback Info Field */
	UINT8	Retry_Cnt;			/* */
	UINT16	Pkt_ID;				/* */
	
	// DWORD 3	
	UINT16	Seq_Num;			/* */
	UINT8	S_Rate;				/* Start rate. */
	UINT8	F_Rate;				/* Final rate. */
	
	// DWORD 4	
	UINT8	S_RTS_Rate;			/* */
	UINT8	F_RTS_Rate;			/* */
	UINT16	pkt_length;			/* */
	
	// DWORD 5	
	UINT16	Reserve3;			/* */	
	UINT16	Duration;			/* */	
}CMPK_TXFB_T;

// For USB
/* 2. RX side: Interrupt status packet. It includes Beacon State, 
	  Beacon Timer Interrupt and other useful informations in MAC ISR Reg. */
typedef struct tag_Cmd_Pkt_Interrupt_Status
{
	UINT8	Element_ID;			/* Command packet type. */
	UINT8	Length;				/* Command packet length. */
	UINT16	Reserve;
	UINT32	InterruptStatus;				/* Interrupt Status. */	
}CMPK_INTR_STA_T;

// For PCI
/* 2. RX side: Beacon state packet. */
typedef struct tag_Cmd_Pkt_Beacon_State
{
	UINT8	Element_ID;			/* Command packet type. */
	UINT8	Length;				/* Command packet length. */
	UINT16	BOK:1;				/* Beacon info. */	
	UINT16	Reserve:15;			/* Beacon info. */	
}CMPK_BCN_STA_T;


/* 3. TX side: Set configuration packet. */
typedef struct tag_Cmd_Pkt_Set_Configuration
{
	UINT8	Element_ID;			/* Command packet type. */
	UINT8	Length;				/* Command packet length. */
	UINT16	Reserve1;			/* */

	UINT32	Cfg_Action:1;		/* Configuration info. */
	UINT32	Cfg_Type:2;			/* Configuration info. */
	UINT32	Cfg_Size:2;			/* Configuration info. */
	UINT32	Cfg_Reserve2:15;	/* Configuration info. */
	UINT32	Cfg_Page:4;			/* Configuration info. */
	UINT32	Cfg_Offset:8;		/* Configuration info. */

	UINT32	Value;				/* */
	UINT32	Mask;				/* */
}CMPK_SET_CFG_T;

#ifdef REMOVE_PACK
#pragma pack()
#endif



/* 4. Both side : TX/RX query configuraton packet. The query structure is the 
      same as set configuration. */
#define		CMPK_QUERY_CFG_T	CMPK_SET_CFG_T


/* 5. Multi packet feedback status. */
typedef struct tag_Tx_Stats_Feedback // PJ quick rxcmd 09042007
{
	// For endian transfer --> Driver will not the same as firmware structure.
	// DW 0	
	UINT16	Reserve1;			
	UINT8 	Length;				// Command packet length
	UINT8 	Element_ID;			// Command packet type
	
	// DW 1	
	UINT16	TxFail;				// Tx Fail count
	UINT16 	TxOK;				// Tx ok count
	
	// DW 2
	UINT16	TxMCOK;  			// tx multicast
	UINT16 	TxRetry;			// Tx Retry count	
	
	// DW 3
	UINT16  TxUCOK;				// tx unicast
	UINT16	TxBCOK;  			// tx broadcast	
	
	// DW 4
	UINT16	TxBCFail;			//
	UINT16	TxMCFail;			//	
	
	// DW 5
	UINT16	Reserve2;			//
	UINT16	TxUCFail;			//
	
	// DW 6-8
	UINT32	TxMCLength;	
	UINT32	TxBCLength;
	UINT32	TxUCLength;
	
	// DW 9
	UINT16	Reserve3_23;
	UINT8	Reserve3_1;
	UINT8	Rate;
}CMPK_TX_STATUS_T;


/* 6. Debug feedback message. */
/* 2007/10/23 MH Define RX debug message  */
typedef struct tag_Rx_Debug_Message_Feedback
{
	// For endian transfer --> for driver
	// DW 0	
	UINT16	Reserve1;			
	UINT8 	Length;				// Command packet length
	UINT8 	Element_ID;			// Command packet type
	
	// DW 1-??
	// Variable debug message.
	
}CMPK_RX_DBGINFO_T;


/* 2008/03/20 MH Define transmit rate history. For big endian format. */
typedef struct tag_Tx_Rate_History
{	
	// For endian transfer --> for driver
	// DW 0		
	UINT8 	Element_ID;			// Command packet type
	UINT8 	Length;				// Command packet length
	UINT16	Reserved1;				
	
	// DW 1-2	CCK rate counter
	UINT16 	CCK[4];
	
	// DW 3-6
	UINT16 	OFDM[8];
	
	// DW 7-14
	//UINT16	MCS_BW0_SG0[16];
	
	// DW 15-22
	//UINT16	MCS_BW1_SG0[16];
	
	// DW 23-30
	//UINT16	MCS_BW0_SG1[16];
	
	// DW 31-38
	//UINT16	MCS_BW1_SG1[16];

	// DW 7-14	BW=0 SG=0
	// DW 15-22	BW=1 SG=0
	// DW 23-30	BW=0 SG=1
	// DW 31-38	BW=1 SG=1
	UINT16	HT_MCS[4][16];
	
}CMPK_TX_RAHIS_T;

/* Define element ID of command packet. */
typedef enum tag_Command_Packet_Directories
{    
    RX_TX_FEEDBACK = 0,

    RX_BEACON_STATE			= 1,    
    TX_SET_CONFIG				= 2,
    BOTH_QUERY_CONFIG			= 3,    
    RX_TX_STATUS				= 4,
    RX_DBGINFO_FEEDBACK		= 5,
    RX_TX_PER_PKT_FEEDBACK		= 6,
    RX_TX_RATE_HISTORY		= 7,
    RX_TX_TSSI_MEAN_BACK		= 8,
    RX_CMD_ELE_MAX
}CMPK_ELEMENT_E;


/* Define packet type. */
typedef enum tag_Packet_Type
{    
    TYPE_PACKET_BROADCAST = 0,
    TYPE_PACKET_MULTICAST,    
    TYPE_PACKET_UNICAST,    
    TYPE_PACKET_TYPE_MAX
}CMPK_PKT_TYPE_E;
/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/


/*------------------------Export Marco Definition---------------------------*/
/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/

extern RT_STATUS
SendTxCommandPacket(
	IN	PADAPTER		pAdapter,
	IN	PVOID 			pData,
	IN	u4Byte			DataLen
	);

extern	RT_STATUS	CMPK_Message_Handle_Tx(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			CodeVirtualAddrress,
	IN	u4Byte					PacketType,
	IN	u4Byte			BufferLen);
extern	UINT32	CMPK_Message_Handle_Rx(PADAPTER	pAdapter, PRT_RFD pRfd);


/* Temporarily extern for debug utility. */
extern	void	cmpk_DisplayTxStatistic(PADAPTER Adapter);
/*--------------------------Exported Function prototype---------------------*/


#endif	/* __HAL90_CMD_PKT_H__ */


/* End of hal90cmdpkt.h */
