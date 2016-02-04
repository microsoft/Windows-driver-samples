#ifndef __INC_HALPHY_H
#define __INC_HALPHY_H
/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	HalPhy.h
	
Abstract:
	Prototype of PHY_XXX() and related data structure.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2011-08-10 Lanhsin            Create.	
--*/

/*--------------------------Define Parameters-------------------------------*/

#if (DISABLE_BB_RF == 1)
#define		HAL_FW_ENABLE				0
#define		HAL_MAC_ENABLE				0
#define		HAL_BB_ENABLE				0
#define		HAL_RF_ENABLE				0
#else // FPGA_PHY and ASIC
#define 	HAL_FW_ENABLE				1
#define		HAL_MAC_ENABLE				1
#define		HAL_BB_ENABLE				1
#define		HAL_RF_ENABLE				1
#endif


#define delay_ms(_t)			PlatformStallExecution(1000*(_t))
#define delay_us(_t)			PlatformStallExecution(_t)

#define	RF6052_MAX_TX_PWR			0x3F
#define	RF6052_MAX_REG_88E			0xFF
#define	RF6052_MAX_REG_92C			0x7F

#define	RF6052_MAX_REG	\
		(RF6052_MAX_REG_88E > RF6052_MAX_REG_92C) ? RF6052_MAX_REG_88E: RF6052_MAX_REG_92C

#define GET_RF6052_REAL_MAX_REG(_Adapter)	\
		IS_HARDWARE_TYPE_8188E(_Adapter) ? RF6052_MAX_REG_88E : RF6052_MAX_REG_92C


#define	RF6052_MAX_PATH						4

#define	HAL_PRIME_CHNL_OFFSET_DONT_CARE	0
#define	HAL_PRIME_CHNL_OFFSET_LOWER			1
#define	HAL_PRIME_CHNL_OFFSET_UPPER			2


#define	PHY_SUPPORT_92C				1
#define	PHY_SUPPORT_92D				1
#define	PHY_SUPPORT_88E				1
#define	PHY_SUPPORT_8812				1
#define	PHY_SUPPORT_92E				1
#define	PHY_SUPPORT_8723B				1
#define	PHY_SUPPORT_8814A				1
#define	PHY_SUPPORT_8822B				1


#define	SHORT_SLOT_TIME					9
#define	NON_SHORT_SLOT_TIME				20

//
// Antenna detection method, i.e., using single tone detection or RSSI reported from each antenna detected. 
// Added by Roger, 2013.05.22.
//
#define IS_ANT_DETECT_SUPPORT_SINGLE_TONE(__Adapter)		((GET_HAL_DATA(__Adapter)->AntDetection) & BIT0)
#define IS_ANT_DETECT_SUPPORT_RSSI(__Adapter)		((GET_HAL_DATA(__Adapter)->AntDetection) & BIT1)
#define IS_ANT_DETECT_SUPPORT_PSD(__Adapter)		((GET_HAL_DATA(__Adapter)->AntDetection) & BIT2)

/*--------------------------Define Parameters-------------------------------*/
typedef	enum _RF_TYPE{
	RF_TYPE_MIN = 0, 	// 0
	RF_8225=1,			// 1 11b/g RF for verification only
	RF_8256=2,			// 2 11b/g/n 
	RF_8258=3,			// 3 11a/b/g/n RF
	RF_6052=4,			// 4 11b/g/n RF
	RF_PSEUDO_11N=5,	// 5, It is a temporality RF. 
	RF_TYPE_MAX
}RF_TYPE_E,*PRF_TYPE_E;


#define	RF_PATH_MAX_92C_88E 		2
#define	RF_PATH_MAX_90_8812		4	//Max RF number 90 support 
#define   RF_PATH_MAX_90_8814A         4


typedef enum _ANTENNA_PATH{
       ANTENNA_NONE 	= 0,
	ANTENNA_D		= 1,
	ANTENNA_C		= 2,
	ANTENNA_CD	= 3,
	ANTENNA_B		= 4,
	ANTENNA_BD	= 5,
	ANTENNA_BC	= 6,
	ANTENNA_BCD	= 7,
	ANTENNA_A		= 8,
	ANTENNA_AD	= 9,
	ANTENNA_AC	= 10,
	ANTENNA_ACD	= 11,
	ANTENNA_AB	= 12,
	ANTENNA_ABD	= 13,
	ANTENNA_ABC	= 14,
	ANTENNA_ABCD	= 15
} ANTENNA_PATH;

 typedef enum _RF_CONTENT{
	radioa_txt = 0x1000,
	radiob_txt = 0x1001,
	radioc_txt = 0x1002,
	radiod_txt = 0x1003
} RF_CONTENT;

typedef enum _BaseBand_Config_Type{
	BaseBand_Config_PHY_REG = 0,			//Radio Path A
	BaseBand_Config_AGC_TAB = 1,			//Radio Path B
	BaseBand_Config_AGC_TAB_2G = 2,
	BaseBand_Config_AGC_TAB_5G = 3,	
	BaseBand_Config_PHY_REG_PG
}BaseBand_Config_Type, *PBaseBand_Config_Type;



typedef enum _HW_BLOCK{
	HW_BLOCK_MAC = 0,
	HW_BLOCK_PHY0 = 1,
	HW_BLOCK_PHY1 = 2,
	HW_BLOCK_RF = 3,
	HW_BLOCK_MAXIMUM = 4, // Never use this
}HW_BLOCK_E, *PHW_BLOCK_E;

typedef enum _SwChnlCmdID{
	CmdID_End,
	CmdID_SetTxPowerLevel,
	CmdID_WritePortUlong,
	CmdID_WritePortUshort,
	CmdID_WritePortUchar,
	CmdID_RF_WriteReg,
}SwChnlCmdID;



typedef enum _RATR_TABLE_MODE{
	RATR_INX_WIRELESS_NGB = 0,		// BGN 40 Mhz 2SS 1SS
	RATR_INX_WIRELESS_NG = 1,		// GN or N
	RATR_INX_WIRELESS_NB = 2,		// BGN 20 Mhz 2SS 1SS  or BN
	RATR_INX_WIRELESS_N = 3,
	RATR_INX_WIRELESS_GB = 4,
	RATR_INX_WIRELESS_G = 5,
	RATR_INX_WIRELESS_B = 6,
	RATR_INX_WIRELESS_MC = 7,
	RATR_INX_WIRELESS_AC_5N = 8,
	RATR_INX_WIRELESS_AC_24N = 9,
}RATR_TABLE_MODE, *PRATR_TABLE_MODE;


/*------------------------------Define structure----------------------------*/ 
typedef struct _SwChnlCmd{
	SwChnlCmdID		CmdID;
	u4Byte			Para1;
	u4Byte			Para2;
	u4Byte			msDelay;
}SwChnlCmd;


#ifdef REMOVE_PACK
#pragma pack(1)
#endif

typedef struct _R_ANTENNA_SELECT_OFDM{	
	u4Byte			r_tx_antenna:4;	
	u4Byte			r_ant_l:4;
	u4Byte			r_ant_non_ht:4;	
	u4Byte			r_ant_ht1:4;
	u4Byte			r_ant_ht2:4;
	u4Byte			r_ant_ht_s1:4;
	u4Byte			r_ant_non_ht_s1:4;
	u4Byte			OFDM_TXSC:2;
	u4Byte			Reserved:2;
}R_ANTENNA_SELECT_OFDM;

typedef struct _R_ANTENNA_SELECT_CCK{
	u1Byte			r_cckrx_enable_2:2;	
	u1Byte			r_cckrx_enable:2;
	u1Byte			r_ccktx_enable:4;
}R_ANTENNA_SELECT_CCK;


#ifdef REMOVE_PACK
#pragma pack()
#endif


typedef struct RF_Shadow_Compare_Map {
	// Shadow register value
	UINT32		Value;
	// Compare or not flag
	UINT8		Compare;
	// Record If it had ever modified unpredicted
	UINT8		ErrorOrNot;
	// Recorver Flag
	UINT8		Recorver;
	//
	UINT8		Driver_Write;
}RF_SHADOW_T;

/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/

/*------------------------Export Marco Definition---------------------------*/

/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/


u4Byte
PHY_CalculateBitShift(
	u4Byte BitMask
	);

VOID
PHY_SetMacReg(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN	u4Byte		Data
	);

u4Byte
PHY_QueryMacReg(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask
	);

u4Byte
PHY_QueryBBReg(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask
	);

VOID
PHY_SetBBReg(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN	u4Byte		Data
	);

VOID
PHY_SetBBReg1Byte(
	IN	PADAPTER	Adapter,
	IN	u4Byte		RegAddr,
	IN	u4Byte		BitMask,
	IN	u4Byte		Data
	);


u4Byte
PHY_QueryRFReg(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				RegAddr,
	IN	u4Byte				BitMask
	);

VOID
PHY_SetRFReg(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				RegAddr,
	IN	u4Byte				BitMask,
	IN	u4Byte				Data
	);

//
// RF Shadow operation relative API
//
u4Byte
PHY_RFShadowRead(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				Offset);

VOID
PHY_RFShadowWrite(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				Offset,
	IN	u4Byte				Data);

BOOLEAN
PHY_RFShadowCompare(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				Offset);

VOID
PHY_RFShadowRecorver(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				Offset);

VOID
PHY_RFShadowCompareAll(
	IN	PADAPTER			Adapter);

VOID
PHY_RFShadowRecorverAll(
	IN	PADAPTER			Adapter);

VOID
PHY_RFShadowCompareFlagSet(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				Offset,
	IN	u1Byte				Type);

VOID
PHY_RFShadowRecorverFlagSet(
	IN	PADAPTER			Adapter,
	IN	u1Byte				eRFPath,
	IN	u4Byte				Offset,
	IN	u1Byte				Type);

VOID
PHY_RFShadowCompareFlagSetAll(
	IN	PADAPTER			Adapter);

VOID
PHY_RFShadowRecorverFlagSetAll(
	IN	PADAPTER			Adapter);

VOID
PHY_RFShadowRefresh(
	IN	PADAPTER			Adapter);


#endif
