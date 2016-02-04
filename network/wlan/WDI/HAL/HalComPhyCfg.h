/*****************************************************************************
 *	Copyright(c) 2008,  RealTEK Technology Inc. All Right Reserved.
 *
 * Module:	__INC_HALCOMPHYCFG_H
 *
 *
 * Note:	
 *			
 *
 * Export:	Constants, macro, functions(API), global variables(None).
 *
 * Abbrev:	
 *
 * History:
 *		Data		Who		Remark 
 *      08/07/2007  MHC    	1. Porting from 9x series PHYCFG.h.
 *							2. Reorganize code architecture.
 * 
 *****************************************************************************/
 /* Check to see if the file has been included already.  */
#ifndef __INC_HALCOMPHYCFG_H
#define __INC_HALCOMPHYCFG_H


//for PutRegsetting & GetRegSetting BitMask
#define		bMaskLowNibble			0xf
#define		bMaskHighNibble			0xf0
#define		bMaskByte2HighNibble	0x00f00000
#define		bMaskByte3LowNibble	0x0f000000
#define		bMaskL3Bytes			0x00ffffff
#define		bMaskByte0                	0xff	// Reg 0xc50 rOFDM0_XAAGCCore~0xC6f
#define		bMaskByte1                	0xff00
#define		bMaskByte2                	0xff0000
#define		bMaskByte3                	0xff000000
#define		bMaskHWord                	0xffff0000
#define		bMaskLWord                	0x0000ffff
#define		bMaskDWord                	0xffffffff
#define		bMask12Bits				0xfff	
#define		bMask7bits				0x7f
#define		bMaskH4Bits				0xf0000000	
#define		bMaskOFDM_D			0xffc00000
#define		bMaskCCK				0x3f3f3f3f

#define 		bRFRegOffsetMask		0xfffff		


typedef enum _RATE_SECTION {
	CCK = 0,
	OFDM,
	HT_MCS0_MCS7,
	HT_MCS8_MCS15,
	HT_MCS16_MCS23,
	HT_MCS24_MCS31,	
	VHT_1SSMCS0_1SSMCS9,
	VHT_2SSMCS0_2SSMCS9,
	VHT_3SSMCS0_3SSMCS9,
	VHT_4SSMCS0_4SSMCS9,
} RATE_SECTION;

typedef enum _RF_TX_NUM {
	RF_1TX = 0,
	RF_2TX,
	RF_3TX,
	RF_4TX,
	RF_MAX_TX_NUM,
	RF_TX_NUM_NONIMPLEMENT,
} RF_TX_NUM;
	
/*--------------------------Define Parameters-------------------------------*/

#define MAX_DOZE_WAITING_TIMES_9x	64

#define MAX_TXPWR_IDX_NMODE		63

#define HP_THERMAL_NUM			8



/* Channel switch:The size of command tables for switch channel*/
#define MAX_PRECMD_CNT		16
#define MAX_RFDEPENDCMD_CNT 16
#define MAX_POSTCMD_CNT 16


// Tx Power Limit Table Size
#define MAX_REGULATION_NUM						4
#define MAX_RF_PATH_NUM_IN_POWER_LIMIT_TABLE	4
#define MAX_2_4G_BANDWITH_NUM					2
#define MAX_RATE_SECTION_NUM					10
#define MAX_5G_BANDWITH_NUM						4
#define MAX_BASE_NUM_IN_PHY_REG_PG_2_4G			10 //  CCK:1, OFDM:1, HT:4, VHT:4
#define MAX_BASE_NUM_IN_PHY_REG_PG_5G			9 // OFDM:1, HT:4, VHT:4

/*------------------------------Define structure----------------------------*/ 
typedef struct _BB_REGISTER_DEFINITION{
	u4Byte rfintfs;			// set software control: 
							//		0x870~0x877[8 bytes]
							
	u4Byte rfintfo; 		// output data: 
							//		0x860~0x86f [16 bytes]
							
	u4Byte rfintfe; 		// output enable: 
							//		0x860~0x86f [16 bytes]
							
	u4Byte rf3wireOffset;	// LSSI data:
							//		0x840~0x84f [16 bytes]

	u4Byte rfHSSIPara2; 	// wire parameter control2 : 
							//		0x824~0x827,0x82c~0x82f, 0x834~0x837, 0x83c~0x83f [16 bytes]
								
	u4Byte rfLSSIReadBack; 	//LSSI RF readback data SI mode
								//		0x8a0~0x8af [16 bytes]

	u4Byte rfLSSIReadBackPi; 	//LSSI RF readback data PI mode 0x8b8-8bc for Path A and B

}BB_REGISTER_DEFINITION_T, *PBB_REGISTER_DEFINITION_T;

//
// <Roger_TODO> We should take RTL8723B into consideration, 2012.10.08
//
#define phy_BB_Config_ParaFile(_Adapter)  \
	(IS_HARDWARE_TYPE_8821B(_Adapter))?phy_BB8821B_Config_ParaFile(_Adapter):\
	(IS_HARDWARE_TYPE_8814A(_Adapter))?phy_BB8814A_Config_ParaFile(_Adapter):\
	(IS_HARDWARE_TYPE_8812A(_Adapter))?phy_BB8812A_Config_ParaFile(_Adapter):\
	(IS_HARDWARE_TYPE_8821(_Adapter))?phy_BB8821A_Config_ParaFile(_Adapter):\
	(IS_HARDWARE_TYPE_8723B(_Adapter))?phy_BB8723B_Config_ParaFile(_Adapter):\
	(IS_HARDWARE_TYPE_8723D(_Adapter))?phy_BB8723D_Config_ParaFile(_Adapter):\
	(IS_HARDWARE_TYPE_8703B(_Adapter))?phy_BB8703B_Config_ParaFile(_Adapter):\
	(IS_HARDWARE_TYPE_8188F(_Adapter))?phy_BB8188F_Config_ParaFile(_Adapter):\
	(IS_HARDWARE_TYPE_8188E(_Adapter)? phy_BB8188E_Config_ParaFile(_Adapter) :\
	(IS_HARDWARE_TYPE_8822B(_Adapter))?PHY_BB8822B_Config_ParaFile(_Adapter):\
	(IS_HARDWARE_TYPE_8192E(_Adapter)?phy_BB8192E_Config_ParaFile(_Adapter):\
	phy_BB8723B_Config_ParaFile(_Adapter)))

#define PHY_GetTxPowerIndex( pAdapter, RFPath, Rate, BandWidth, Channel ) \
	(IS_HARDWARE_TYPE_8814A(pAdapter)) ? PHY_GetTxPowerIndex_8814A( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8821B(pAdapter)) ? PHY_GetTxPowerIndex_8821B( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8822B(pAdapter)) ? PHY_GetTxPowerIndex_8822B( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8812A(pAdapter)) ? PHY_GetTxPowerIndex_8812A( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8821A(pAdapter)) ? PHY_GetTxPowerIndex_8821A( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8723B(pAdapter)) ? PHY_GetTxPowerIndex_8723B( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8703B(pAdapter)) ? PHY_GetTxPowerIndex_8703B( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8188F(pAdapter)) ? PHY_GetTxPowerIndex_8188F( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8192E(pAdapter)) ? PHY_GetTxPowerIndex_8192E( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8723D(pAdapter)) ? PHY_GetTxPowerIndex_8723D( pAdapter, RFPath, Rate, BandWidth, Channel ):\
	(IS_HARDWARE_TYPE_8188E(pAdapter)) ? PHY_GetTxPowerIndex_8188E( pAdapter, RFPath, Rate, BandWidth, Channel ):0\

#define PHY_SetTxPowerIndex( pAdapter, PowerIndex, RFPath, Rate ) \
	do { \
	PHY_SetTxPowerIndexShadow( pAdapter, PowerIndex, RFPath, Rate );\
	if (IS_HARDWARE_TYPE_8814A(pAdapter)) PHY_SetTxPowerIndex_8814A( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8821B(pAdapter)) PHY_SetTxPowerIndex_8821B( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8822B(pAdapter)) PHY_SetTxPowerIndex_8822B( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8812A(pAdapter)) PHY_SetTxPowerIndex_8812A( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8821A(pAdapter)) PHY_SetTxPowerIndex_8821A( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8723B(pAdapter)) PHY_SetTxPowerIndex_8723B( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8703B(pAdapter)) PHY_SetTxPowerIndex_8703B( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8188F(pAdapter)) PHY_SetTxPowerIndex_8188F( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8192E(pAdapter)) PHY_SetTxPowerIndex_8192E( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8723D(pAdapter)) PHY_SetTxPowerIndex_8723D( pAdapter, PowerIndex, RFPath, Rate );\
	else if (IS_HARDWARE_TYPE_8188E(pAdapter)) PHY_SetTxPowerIndex_8188E( pAdapter, PowerIndex, RFPath, Rate );\
	}while(0)

//----------------------------------------------------------------------
//  Initial MAC/BB/RF config by reading MAC/BB/RF txt.
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//  Initial MAC/BB/RF config by reading Hard Code.
//----------------------------------------------------------------------		
		
RT_STATUS
phy_ConfigBBWithHeaderFile(
	IN	PADAPTER		Adapter,
	IN	u1Byte 			ConfigType
);
													
RT_STATUS
PHY_ConfigRFWithHeaderFile(
	IN	PADAPTER			Adapter,
	IN	RF_CONTENT			Content,
	IN	u1Byte				eRFPath
);


RT_STATUS
phy_ConfigBBWithPgParaFile(
	IN	PADAPTER		Adapter,
	IN	ps1Byte 		pFileName
);

RT_STATUS
PHY_ConfigRFWithPowerLimitTableParaFile(
	IN	PADAPTER			Adapter,
	IN	ps1Byte 			pFileName
);

RT_STATUS
PHY_ConfigRFWithCustomPowerLimitTableParaFile(
	IN	PADAPTER			Adapter,
	IN	pu1Byte 			pFileName,
	IN  BOOLEAN				checkInit
);

RT_STATUS
PHY_ConfigBBWithCustomPgParaFile(
	IN	PADAPTER			Adapter,
	IN	pu1Byte 			pFileName,
	IN  BOOLEAN				checkInit
);




//----------------------------------------------------------------------
//  Initial MAC/BB config from File or Hard Code.
//----------------------------------------------------------------------
RT_STATUS
PHY_MACConfig(
	IN	PADAPTER	Adapter
);
	
RT_STATUS
PHY_BBConfig(
	IN	PADAPTER	Adapter
);

//----------------------------------------------------------------------
s4Byte
phy_TxPwrIdxToDbm(
	IN	PADAPTER		Adapter,
	IN	WIRELESS_MODE	WirelessMode,
	IN	u1Byte			TxPwrIdx	
	);


u1Byte
PHY_DbmToTxPwrIdx(
	IN	PADAPTER		Adapter,
	IN	WIRELESS_MODE	WirelessMode,
	IN	s4Byte			PowerInDbm
	);

RT_CHANNEL_DOMAIN
PHY_MapChannelPlan(
	IN	PADAPTER		Adapter,
	IN	u2Byte			HalChannelPlan
	);

void 
SINGLEMODULE_MAPFUN(	
	IN  PADAPTER 	Adapter,
	IN  u2Byte 		Regchannelplane,
	IN	u1Byte		RegEnableAdaptivity,
	IN 	u2Byte		RegHTMode
	);

VOID
PHY_MapChnlPlanRegulatory(
	IN	PADAPTER		Adapter
	);

VOID
PHY_IQCalibrate(
	IN	PADAPTER 	Adapter,
	IN	BOOLEAN 	bReCovery
	);	


VOID
PHY_SetTxPowerIndexShadow(
	IN	PADAPTER			Adapter,
	IN	u4Byte				PowerIndex,
	IN	u1Byte				RFPath,	
	IN	u1Byte				Rate
	);

u1Byte
PHY_GetTxPowerIndexShadow(
	IN	PADAPTER			Adapter,
	IN	u1Byte				RFPath,	
	IN	u1Byte				Rate
	);

VOID
PHY_LCCalibrate_Dummy(
    IN PDM_ODM_T		pDM_Odm
    );

#define PHY_LCCalibrate(pDM_Odm)	\
	(pDM_Odm->SupportICType == ODM_RTL8723B) ? PHY_LCCalibrate_8723B(pDM_Odm) :\
    PHY_LCCalibrate_Dummy(pDM_Odm)

#define PHY_SetRFPathSwitch(_Adapter,_bMain)	\
	(IS_HARDWARE_TYPE_8814A(_Adapter))  ? PHY_SetRFPathSwitch_8814A(_Adapter,_bMain) :\
	(IS_HARDWARE_TYPE_8821B(_Adapter))  ? PHY_SetRFPathSwitch_8821B(_Adapter,_bMain) :\
	(IS_HARDWARE_TYPE_8822B(_Adapter))  ? PHY_SetRFPathSwitch_8822B(_Adapter,_bMain) :\
	(IS_HARDWARE_TYPE_8723B(_Adapter))  ? PHY_SetRFPathSwitch_8723B(_Adapter,_bMain) :\
	(IS_HARDWARE_TYPE_8703B(_Adapter))  ? PHY_SetRFPathSwitch_8703B(_Adapter,_bMain) :\
	(IS_HARDWARE_TYPE_8723D(_Adapter))  ? PHY_SetRFPathSwitch_8723D(_Adapter,_bMain) :\
	(IS_HARDWARE_TYPE_8192E(_Adapter))  ? PHY_SetRFPathSwitch_8192E(_Adapter,_bMain) :\
    (IS_HARDWARE_TYPE_JAGUAR(_Adapter)) ? PHY_SetRFPathSwitch_8812A(_Adapter,_bMain) :\
    (IS_HARDWARE_TYPE_8188E(_Adapter))  ? PHY_SetRFPathSwitch_8188E(_Adapter,_bMain) :\
    (IS_HARDWARE_TYPE_8188F(_Adapter))  ? PHY_SetRFPathSwitch_8188F(_Adapter,_bMain) : 0

#define PHY_QueryRFPathSwitch(_Adapter)	\
    (IS_HARDWARE_TYPE_8814A(_Adapter))  ? PHY_QueryRFPathSwitch_8814A(_Adapter) :\
    (IS_HARDWARE_TYPE_8821B(_Adapter))  ? PHY_QueryRFPathSwitch_8821B(_Adapter) :\
    (IS_HARDWARE_TYPE_8822B(_Adapter))  ? PHY_QueryRFPathSwitch_8822B(_Adapter) :\
    (IS_HARDWARE_TYPE_8723B(_Adapter))  ? PHY_QueryRFPathSwitch_8723B(_Adapter) :\
    (IS_HARDWARE_TYPE_8723D(_Adapter))  ? PHY_QueryRFPathSwitch_8723D(_Adapter) :\
     (IS_HARDWARE_TYPE_8703B(_Adapter))  ? PHY_QueryRFPathSwitch_8703B(_Adapter) :\
    (IS_HARDWARE_TYPE_8192E(_Adapter))  ? PHY_QueryRFPathSwitch_8192E(_Adapter) :\
    (IS_HARDWARE_TYPE_8812A(_Adapter)) ? PHY_QueryRFPathSwitch_8812(_Adapter) :\
    (IS_HARDWARE_TYPE_8821A(_Adapter)) ? PHY_QueryRFPathSwitch_8821A(_Adapter) :\
    (IS_HARDWARE_TYPE_8188E(_Adapter))  ? PHY_QueryRFPathSwitch_8188E(_Adapter) :\
    (IS_HARDWARE_TYPE_8188F(_Adapter))  ? PHY_QueryRFPathSwitch_8188F(_Adapter): 0


typedef struct __RT_SINGLEMODULE_ENTRY
{
	u4Byte 		country_code;	 
	u2Byte 		Regchannelplane;
	u1Byte		RegEnableAdaptivity;
	u2Byte		RegHTMode;

	void (*Func)(							// The  handler function
		IN  PADAPTER 	pAdapter,
		IN  u2Byte 		Regchannelplane,
		IN	u1Byte		RegEnableAdaptivity,
		IN 	u2Byte		RegHTMode
		);

}RT_SINGLEMODULE_ENTRY;

extern RT_SINGLEMODULE_ENTRY RT_SUPPORT_COUNTRY[];
extern u4Byte	GLNumOfSupportCountry;
/*--------------------------Exported Function prototype---------------------*/

#endif	// __INC_HALCOMPHYCFG_H

