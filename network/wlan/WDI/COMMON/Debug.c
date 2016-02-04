#include "Mp_Precomp.h"

#if WPP_SOFTWARE_TRACE
#include "Debug.tmh"
#endif
 
u4Byte GlobalDebugLevel			= 	DBG_LOUD;
//
// 2009/06/22 MH Allow Fre build to print none debug info at init time.
//
#if DBG
u8Byte GlobalDebugComponents	= 	\
//									COMP_DBG				|
//									COMP_INIT				|
//									COMP_OID_QUERY			|
//									COMP_OID_SET			|
//									COMP_RECV				|
//									COMP_SEND				|
//									COMP_IO					|
//									COMP_POWER				|
//									COMP_MLME				|
//									COMP_SCAN				|
//									COMP_SYSTEM			|
//									COMP_SEC				|
//									COMP_AP				|
//									COMP_QOS				|
//									COMP_AUTHENTICATOR	|
//									COMP_BEACON			|
//									COMP_WFD			|
//									COMP_RM				|
//									COMP_MP				|
//									COMP_CKIP				|
//									COMP_RSNA				|
//									COMP_INDIC				|
//									COMP_LED				|
//									COMP_RF					|
//									COMP_DUALMACSWITCH	|
//									COMP_EASY_CONCURRENT	|

//1!!!!!!!!!!!!!!!!!!!!!!!!!!!
//1//1Attention Please!!!<11n or 8190 specific code should be put below this line>
//1!!!!!!!!!!!!!!!!!!!!!!!!!!!

//									COMP_HT				|
//									COMP_POWER_TRACKING 	|
//									COMP_RX_REORDER		|
//									COMP_AMSDU 			|
//									COMP_WPS				|
//									COMP_CMD				|
//									COMP_EFUSE				|
//									COMP_CCX				|	
//									COMP_IOCTL				|
//									COMP_P2P				|
//									COMP_TDLS				|
//									COMP_OMNIPEEK			|
//									COMP_MULTICHANNEL		|
//									COMP_DFS				|
//									COMP_TEST				|
//									COMP_TXPWR				|
//									COMP_HAL_MAC_API		|
//									COMP_NAN				|
									0;

u4Byte GLDebugComponents	= 	\
//									COMP_DBG_MON			|
//									COMP_TX_HANG			|
//									COMP_RX_HANG			|
									0;

u4Byte HalMacDebugComponents	= 	\
//									COMP_HALMAC_MSG_INIT			|
//									COMP_HALMAC_MSG_EFUSE 		|
//									COMP_HALMAC_MSG_FW			|
//									COMP_HALMAC_MSG_H2C			|
//									COMP_HALMAC_MSG_PWR			|
									0;
#else
u8Byte GlobalDebugComponents	= 0;
u4Byte	GLDebugComponents = 0;
u4Byte HalMacDebugComponents = 0;
#endif


/*------------------Declare variable-----------------------
// Define debug flag array for common debug print macro. */
UINT32			DBGP_Type[DBGP_TYPE_MAX];

/* Define debug print header for every service module. */
DBGP_HEAD_T		DBGP_Head;


/*-----------------------------------------------------------------------------
 * Function:    DBGP_Flag_Init
 *
 * Overview:    Refresh all debug print control flag content to zero.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 *  When		Who		Remark
 *  10/20/2006	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	void	DBGP_Flag_Init(void)
{    
    UINT8	i;
    
	for (i = 0; i < DBGP_TYPE_MAX; i++)
	{
 		DBGP_Type[i] = 0;
	}
    
#if DBG		
	// 2010/06/02 MH Free build driver can not out any debug message!!!
	// Init Debug flag enable condition

	DBGP_Type[FINIT]			=	\
//								INIT_EEPROM						|
//								INIT_TxPower						|
//								INIT_IQK							|
//								INIT_RF							|
								0;

	DBGP_Type[FDM]			=	\
//								WA_IOT							|
//								DM_PWDB						|
//								DM_Monitor						|
//								DM_DIG				|
//								DM_EDCA_Turbo		|
//								DM_BT30				|
								0;

	DBGP_Type[FIOCTL]		= 	\
//								IOCTL_IRP						|
//								IOCTL_IRP_DETAIL					|
//								IOCTL_IRP_HANDLE				|
//								IOCTL_BT_HCICMD					|
//								IOCTL_BT_HCICMD_DETAIL			|
//								IOCTL_BT_HCICMD_EXT				|
//								IOCTL_BT_EVENT					|
//								IOCTL_BT_EVENT_DETAIL			|
//								IOCTL_BT_EVENT_PERIODICAL		|
//								IOCTL_BT_TX_ACLDATA			|
//								IOCTL_BT_TX_ACLDATA_DETAIL		|
//								IOCTL_BT_RX_ACLDATA				|
//								IOCTL_BT_RX_ACLDATA_DETAIL		|
//								IOCTL_BT_TP						|
//								IOCTL_STATE						|
//								IOCTL_BT_LOGO					|
//								IOCTL_BT_MGNT_PACKET			|
//								IOCTL_CALLBACK_FUN				|
//								IOCTL_PARSE_BT_PKT				|
								0;

	DBGP_Type[FC2H]		= 	\
//								C2H_Summary						|
//								C2H_PacketData					|
//								C2H_ContentData					|
								0;

	DBGP_Type[FBT]			= 	\
//								BT_TRACE						|
//								BT_DBG_STATE					|
//								BT_DBG_CONTENT				|
//								BT_H2C_STATE					|
//								BT_NOTIFY_EVENT				|
//								BT_DBG_CTRL						|
								0;

	DBGP_Type[FEEPROM]		=	\
//								EEPROM_W						|
//								EFUSE_PG						|
//								EFUSE_READ_ALL					|
//								EFUSE_ANALYSIS					|
//								EFUSE_PG_DETAIL					|
								0;

	DBGP_Type[FDBG_CTRL]	= 	\
//								DBG_CTRL_TRACE					|
//								DBG_CTRL_INBAND_NOISE			|
								0;
	
	// 2011/07/20 MH Add for short cut 
	DBGP_Type[FSHORT_CUT] = 	\
//								SHCUT_TX 						| 
//								SHCUT_RX						|
								0;
								
	// 2011/09/28 Sinda Add for batch indication
	DBGP_Type[FBATCH_INDICATION] = 	\
//									INDICATE_TRACE					|
									0;

	DBGP_Type[FIOTHREAD]			=	\
//								IOTHREAD_INIT					|
//								IOTHREAD_Q_OP					|
//								IOTHREAD_STATE					|
//								IOTHREAD_H2C					|
								0;

	DBGP_Type[FFW]			=	\
//								FW_MSG_INIT						|
//								FW_MSG_Q_OP						|
//								FW_MSG_H2C_STATE					|
//								FW_MSG_H2C_CONTENT				|
//								FW_MSG_RSVD						|
								0;

	DBGP_Type[FMPBT]			=	\
//								FMPBT_CONTENT					|
//								FMPBT_STATE						|
//								FMPBT_H2C_CONTENT				|
//								FMPBT_H2C_STATE					|
//								FMPBT_C2H_CONTENT				|
//								FMPBT_C2H_STATE					|
								0;
	
	DBGP_Type[FMPHAL]			=	\
//								FMPHAL_CONTENT					|
//								FMPHAL_STATE					|
								0;	
	
	DBGP_Type[FPHY] 		=	\
//								PHY_BBR 		  |
//								PHY_BBW 		  |
//								PHY_RFR 		  |
//								PHY_RFW 		  |
//								PHY_MACR		  |
//								PHY_MACW		  |
//								PHY_ALLR		  |
//								PHY_ALLW		  |
//								PHY_TXPWR		  |
//								PHY_PWRDIFF 	  |
//								PHY_SICR		  |
//								PHY_SICW		  |
//								PHY_TXPWR_EFUSE   |
								0;		

	DBGP_Type[FRW_POINT]			=	\
//								FRW_POINT_TX				|
//								FRW_POINT_TX_DATA			|	
//								FRW_POINT_RX				|	
//								FRW_POINT_RX_DATA			|	
//								FRW_POINT_DBG				|	
								0;

	DBGP_Type[FBEAM]			=	\
//								FBEAM_FUN				|
//								FBEAM_ERROR			|
//								FBEAM_DATA				|
								0;	

	DBGP_Type[FACTION_TIMER]	=	\
//								COMP_ACTION_TIMER_LIFE				|
//								COMP_ACTION_TIMER_ITEM				|
								0;	
	
	DBGP_Type[FHAL_MAC_ID]		=	\
//								COMP_MAC_ID				|
								0;	

	DBGP_Type[FCHNL]			=	\
//								FCHNL_FUN				|
//								FCHNL_ERROR				|
//								FCHNL_INFO				|
								0;	
	DBGP_Type[FCCX]			=	\
//								CCX_CELLPWRLMT			|
//								CCX_RM					|
//								CCX_CAC					|
								0;	

	DBGP_Type[FUSWITCH]			=	\
//								FUSWITCH_FUN				|
//								FUSWITCH_ERROR				|
//								FUSWITCH_INFO				|
								0;	

#endif	
	/* Define debug header of every service module. */
	DBGP_Head.pMANS	= "\n\r[MANS] ";
	DBGP_Head.pRTOS	= "\n\r[RTOS] ";
	DBGP_Head.pALM	= "\n\r[ALM]  ";
	DBGP_Head.pPEM	= "\n\r[PEM]  ";
	DBGP_Head.pCMPK	= "\n\r[CMPK] ";
	DBGP_Head.pRAPD	= "\n\r[RAPD] ";
	DBGP_Head.pTXPB	= "\n\r[TXPB] ";
	DBGP_Head.pQUMG	= "\n\r[QUMG] ";
    	
}	/* DBGP_Flag_Init */


/*-----------------------------------------------------------------------------
 * Function:    DBG_PrintAllFlag
 *
 * Overview:    Print All debug flag
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 *  When		Who		Remark
 *  12/10/2008	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	void	DBG_PrintAllFlag(void)
{
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 0    FQoS\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 1    FTX\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 2    FRX\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 3    FSEC\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 4    FMGNT\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 5    FMLME\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 6    FRESOURCE\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 7    FBEACON\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 8    FISR\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 9    FPHY\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 11   FMP\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 12   FPWR\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 13   FDM\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 14   FDBG_CTRL\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 15   FC2H\n"));
	RT_TRACE(COMP_CMD, DBG_LOUD, 	("DBGFLAG 16   FBT\n"));			
}	// DBG_PrintAllFlag


/*-----------------------------------------------------------------------------
 * Function:    DBG_PrintFlagEvent
 *
 * Overview:    Print dedicated debug flag event
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 *  When		Who		Remark
 *  12/10/2008	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	void	DBG_PrintFlagEvent(UINT8	DbgFlag)
{
	switch(DbgFlag)
	{
		case	FQoS:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    QoS_INIT\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    QoS_VISTA\n"));
		break;

		case	FTX:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    TX_DESC\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    TX_DESC_TID\n"));
		break;

		case	FRX:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    RX_DATA\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    RX_PHY_STS\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 2    RX_PHY_SS\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 3    RX_PHY_SQ\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 4    RX_PHY_ASTS\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 5    RX_ERR_LEN\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 6    RX_DEFRAG\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 7    RX_ERR_RATE\n"));
		break;

		case	FSEC:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("NA\n"));
		break;

		case	FMGNT:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("NA\n"));
		break;

		case	FMLME:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    MEDIA_STS\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    LINK_STS\n"));		
		break;

		case	FRESOURCE:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    OS_CHK\n"));
		break;

		case	FBEACON:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    BCN_SHOW\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    BCN_PEER\n"));		
		break;

		case	FISR:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    ISR_CHK\n"));
		break;

		case	FPHY:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    PHY_BBR\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    PHY_BBW\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 2    PHY_RFR\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 3    PHY_RFW\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 4    PHY_MACR\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 5    PHY_MACW\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 6    PHY_ALLR\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 7    PHY_ALLW\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 8    PHY_TXPWR\n"));
		break;

		case	FMP:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    MP_RX\n"));
		break;
		
		case	FEEPROM:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    EEPROM_W\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    EFUSE_PG\n"));		
		break;
		
		case	FPWR:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    LPS\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    IPS\n"));		
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 2    PWRSW\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 3    PWRHW\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 4    PWRHAL\n"));
		break;

		case	FDM:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    WA_IOT\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    DM_PWDB\n"));		
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 2    DM_Monitor\n"));		
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 3    DM_DIG\n"));		
		break;

		case	FDBG_CTRL:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    DBG_CTRL_TRACE\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    DBG_CTRL_INBAND_NOISE\n"));		
		break;
		
		case	FC2H:
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 0    C2H_Summary\n"));
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 1    C2H_PacketData\n"));		
		RT_TRACE(COMP_CMD, DBG_LOUD, 	("BIT 2    C2H_ContentData\n"));		
		break;
				
		default:
			break;
	}

}	// DBG_PrintFlagEvent



