
#include "..\PLATFORM\svn_version.h"

/*****************************************************************************
 *	Copyright 2007,  RealTEK Technology Inc. All Right Reserved.
 *
 * Module:		DbgCmd.h	(RTL8190  Header H File)
 *
 *
 * Note:		Declare some variable which will be used by any debug command.
 *			
 *
 * Export:		
 *
 * Abbrev:			
 *
 * History:		
 *	Data		Who		Remark 
 *	08/01/2007  MHC    	Create initial version.
 *	08/02/2007	MHC		Because some variable use strcure defined in HAL 
 *						relative header files. We must copy the structure to
 *						pass all NIC's source files in common directory.
 * 
 *****************************************************************************/
 /* Check to see if the file has been included already.  */
#ifndef	__DBG_CMD_H__
#define __DBG_CMD_H__

#if (DBG_CMD == 1)
/*--------------------------Define Parameters-------------------------------*/
// For test sequeence
#define		VERA_LOCAL_BUF_NUM			0


#define		VERA_ALL_QUEUE				0xFF
#define		VERA_ALL_TXRATE				0xFF
#define		VERA_RANDOM_TXPKTLEN			0xFF
#define		VERA_MAX_TXPKTLEN			500
/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 
/* 2007/08/02 MH The structure must be the same with _TX_FWINFO_8190PCI */

#ifdef REMOVE_PACK
#pragma pack(1)
#endif

typedef struct _TX_FWINFO_STRUCUTRE{
	//DOWRD 0
	u1Byte			TxRate:7;
	u1Byte			CtsEnable:1;
	u1Byte			RtsRate:7;
	u1Byte			RtsEnable:1;
	u1Byte			TxHT:1;
	u1Byte			Short:1;					
	u1Byte			TxBandwidth:1;				
	u1Byte			TxSubCarrier:2;				
	u1Byte			STBC:2;
	u1Byte			AllowAggregation:1;
	u1Byte			RtsHT:1;					
	u1Byte			RtsShort:1;					
	u1Byte			RtsBandwidth:1;				
	u1Byte			RtsSubcarrier:2;			
	u1Byte			RtsSTBC:2;
	u1Byte			EnableCPUDur:1;				
	
	//DWORD 1
	u4Byte			RxMF:2;
	u4Byte			RxAMD:3;
	u4Byte			Reserved1:3;
	u4Byte			TxAGCOffset:4;
	u4Byte			TxAGCSign:1;
	u4Byte			Tx_INFO_RSVD:6;
	u4Byte			PacketID:13;
}TX_FWINFO_T;

#ifdef REMOVE_PACK
#pragma pack()
#endif

/* 2007/07/31 MH Define ISR time structure. */
typedef struct tag_Dbg_Cmd_Interrupt_Service_Time_Count_Structure
{
	u8Byte	enter_time;
	u8Byte	exit_time;
	u8Byte	delta_time;
	u8Byte	max_time;
	u8Byte	min_time;
	/*u4Byte	enter_time;
	u4Byte	exit_time;
	u4Byte	delta_time;
	u4Byte	max_time;
	u4Byte	min_time;*/
}ISR_TIME_T;

typedef enum tag_Interrupt_Service_Type_Definition
{
	ISR_ALL = 0,		
	ISR_TX,
	ISR_RX,
	ISR_NUM_MAX,
	ISR_MP_SEND_PACKET,
	ISR_MP_RETURN_PACKET,
	ISR_MP_CHK_FOR_HANG,
	
}ISR_TYPE_E;

typedef enum tag_Interrupt_Service_Action_Type_Definition
{
	ISR_ENTER = 0,		
	ISR_EXIT
}ISR_ACTION_TYPE_E;


/* 2008/05/22 MH Define for verification console. */
typedef struct tag_Dbg_Verification_Console_Structure
{
	u1Byte				TxRate;
	u1Byte				TxQueue;
	u2Byte				TxPktCnt;
	u2Byte				TxPktLength;
	u4Byte				TransPktNum;
}VERA_CONSOLE_T;
	


// Byte[0] BIT7 in Frame Control is reserved for QoS. Added by Annie, 2005-12-02. 
#define	FC_QOS_BIT				BIT7

// Byte[0] BIT6 in Frame Control seems to be reserved for "No Data". Added by Annie, 2006-01-06.
#define	FC_NO_DATA_BIT		BIT6


// 2008/06/23 Collect all debug variable in the same structure.
typedef struct _DEBUG_CONSOLE_VAIABLE_STRUCTURE{

	// For verficacion consolt statistic.
	BOOLEAN				bBurstMode;
	u4Byte				TxPktCnt, RxPktCnt;

	// For redefine TX queue for non-QoS
	u1Byte			DBG_Tx_Que;

	// --------Use to displat TX/RX DESC/DATA--------
	u1Byte			DBG_Tx_Desc_Display;
	u1Byte			DBG_Tx_Data_Display;
	u1Byte			DBG_Rx_Desc_Display;
	u1Byte			DBG_Rx_Data_Display;
	u1Byte			DBG_Rx_TID;


	// --------Use to estimate driver handle NDIS packet time--------
	// Used to capture the maximum TX DMA pollin range
	u1Byte			DBG_DmaPollCapStart;
	u4Byte			DBG_DmaPollRangeMax;

	// Capture the time when we receve upper later packet until the last polling packet.
	u4Byte			DBG_MpSendRange;
	u4Byte			DBG_MpSendStart;
	u4Byte			DBG_MpSendNum;
	u4Byte			DBG_MpSendEnd;
	u4Byte			DBG_MpcaptureFlag;

	// Capture the max time range from MP sned to inser polling 
	u4Byte			DBG_MpPollCapStart;
	u4Byte			DBG_MpPollRangeMax;
	
	// --------Use to switch debug print or not--------
	// Display current beacom interrupt number from MAC
	u4Byte			DBG_Beacon_Cnt;

	// --------Use to switch TX Descriptor content--------
	// Change AMPDU type
	u4Byte			DBG_AMPDUType;
	// Default bandwidth type
	u4Byte			DBG_BWType;

	// --------Use to control to send MGNT frame or not--------
	// Default not to stop beacon frame
	u4Byte			DBG_Beacon_Stop;
	// Control Probe req TX
	u4Byte			DBG_ProbeReq_Send;

	// --------Use to switch or display RX Descriptor content--------
	// Control to print RX DESC BUF ADDR
	u4Byte				DBG_RxDescBuffAddr;
	
	// Collect MP send packet and complete NDIS packet number
	u4Byte				DBG_MpTxCnt;
	u4Byte				DBG_MpTxCompleteCnt;
	

	// Check RFD memory sapce to check if it is modified incorrectlly
	VIRTUAL_MEMORY		PseudoRfd;

	// Print some debug info
	u4Byte				DbgPrintIsr;

	// For 92SE reset test to stop TX process.
	u4Byte				ResetStop;

	// Force to change TID for different AC queue
	u1Byte				UserSwitchTid;
	u1Byte				UserForceTid;
	u1Byte				UserForceQue;
	
	// Fragmentation debug structure!!
	u1Byte				pSenderAddr[4];
	u1Byte				TID[4];
	u2Byte				SeqNum[4];
	u1Byte				FragNum[4];
	BOOLEAN				bMoreFrag[4];
	u1Byte				bEncrypted[4];
	u1Byte				DeFragBuf[4][500];
	u1Byte				DeFragAllBuf[2000];

	// --------Use to debug c2h handling--------
	u4Byte				c2hInterruptCnt;
	u4Byte				c2hClrReadC2hCnt;
	u4Byte				c2hNotReadyCnt;
	u4Byte				c2hBufAlloFailCnt;
	u4Byte				c2hPacketCnt;
}DBG_VAR_T, *PDBG_VAR_T;



/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/

extern	DBG_VAR_T		DBG_Var;


/* 2007/07/16 MH For test, declare a global variable to record peer bbeacon. */
extern	RT_WLAN_BSS		Peer_bssDesc;
/* 2007/07/25 MH Only 819x support TX FW info now. */
extern	TX_FWINFO_T		Tmp_TxFwInfo;
/* 2007/07/31 MH  Define debug global variable as below. */
extern	ISR_TIME_T		ISR_TimeTbl[ISR_NUM_MAX];

#if 1//
// For 8192S debug, we must remove to proper position later
extern	u2Byte			DBG_Tx_Desc_Idx[MAX_TX_QUEUE];
extern	u1Byte			DBG_Tx_Data[10000];
extern	u1Byte			DBG_Tx_Beacon[512];
extern	u4Byte			DBG_Tx_BeaconLen;
extern	u1Byte			DBG_Rx_Beacon[512];
extern	u4Byte			DBG_Rx_BeaconLen;
extern	u4Byte			DBG_Tx_Data_Index;
extern	u1Byte			DBG_Tx_Data_Ring[10][1000];
extern	u1Byte			DBG_Rx_Data[10000];


#endif
/*------------------------Export global variable----------------------------*/


/*------------------------Export Marco Definition---------------------------*/

/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/
extern	void	DBG_ConsoleInitialize();//	PADAPTER	pAdapter);

extern	void DBG_RecordSvnVersion();

extern	void	DBG_IsrTimeCapture(	PADAPTER	pAdapter, 		
									u1Byte		Isr_Type,
									u1Byte		Isr_Action);
extern	void	DBG_IsrTimeClear(	void);
extern	void	DBG_IsrTimeDisplay(	void);

extern	VOID	DBG_Verify_Console(	IN	PVOID	pContext	);
extern	VOID	DBG_Vera_Input(		IN	VERA_CONSOLE_T	*pTdata	);

extern	VOID	DBG_Display_Tx_Data(void);

extern	VOID	DBG_FreeTCBs(	PADAPTER			Adapter	);
extern	void	DBG_CaptureDmaPollRange(	PADAPTER	Adapter);
extern	void	DBG_CaptureMpPollRange(	PADAPTER	Adapter);
extern	void	DBG_ChkDrvResource(	PADAPTER	Adapter);

/*--------------------------Exported Function prototype---------------------*/

#else	/* #if DBG_CMD */
#define	DBG_ConsoleInitialize()
#define DBG_IsrTimeCapture(pAdapter, Isr_Type, Isr_Action)
#define	DBG_IsrTimeClear()
#define DBG_IsrTimeDisplay()
#define DBG_Verify_Console(pContext)
#define	DBG_Vera_Input(pTdata)
#define	DBG_Display_Tx_Data()
#define DBG_RateChangeCallBack(pTimer)
#define	DBG_FreeTCBs(Adapter)
#define	DBG_CaptureDmaPollRange(Adapter)
#define	DBG_CaptureMpPollRange(Adapter)
#define	DBG_ChkDrvResource(Adapter)

#endif	/* else of #if DBG_CMD */
#endif	/* __DBG_CMD_H__ */


/* End of DbgCmd.h */
