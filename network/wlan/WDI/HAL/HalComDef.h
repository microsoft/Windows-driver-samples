#ifndef __INC_HALDEFCOM_H
#define __INC_HALDEFCOM_H
/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	HalComDef.h
	
Abstract:
	Defined 92C/88E common data structure.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2011-12-13 Isaiah            Create.	
--*/

/*--------------------------Define Parameters-------------------------------*/
    


//#pragma mark -- Chip specific. --
extern const char *const GLBwSrc[];

//-------------------------------------------------------------------------
//	Chip specific
//-------------------------------------------------------------------------

#define CHIP_BONDING_IDENTIFIER(_value)		(((_value)>>22)&0x3)
#define CHIP_BONDING_92C_1T2R					0x1
#define CHIP_BONDING_88C_USB_MCARD			0x2
#define CHIP_BONDING_88C_USB_HP				0x1

//
// 2011.01.06. Define new structure of chip version for RTL8723A and so on. Added by tynli.
// Modify Manufacture and ROM version field, revised by Roger, 2012.11.01. 
//
/*
     | BIT15:12        |  BIT11:9     | BIT 8:7        |  BIT6:4  |    BIT3    | BIT 2:0 |
     |-----------------+--------------+----------------+----------+------------+---------|
     | IC version(CUT) | ROM version  |  Manufacturer  | RF type  |  Chip type | IC Name |
     |                 |              | TSMC/UMC/SMIC  |          | TEST/NORMAL|         |
     |                 |              |    	           |          |            |         |
*/
// [15:12] IC version(CUT): A-cut=0, B-cut=1, C-cut=2, D-cut=3
// [11:9] ROM version
// [8:7] Manufacturer: TSMC=0, UMC=1, SMIC=2
// [6:4] RF type: 1T1R=0, 1T2R=1, 2T2R=2, 3T3R=3
// [3] Chip type: TEST=0, NORMAL=1
// [2:0] IC name: 81xxC=0, 8723A=1, 8192D=2, 8188E=3, 8812=4, 8821=5, 8723B=6, 8814A=7,
//
// Since 2014.06.18, we use another new format(v2), which defined below this block.

// IC Name
#define CHIP_8723A				BIT(0)
#define CHIP_92D				BIT(1)
#define CHIP_8812				BIT(2)
#define CHIP_8188E				(BIT(0)|BIT(1))
#define CHIP_8821				(BIT(0)|BIT(2))
#define CHIP_8723B				(BIT(1)|BIT(2))
#define CHIP_8814A				(BIT(0)|BIT(1)|BIT(2))

// Chip type
#define NORMAL_CHIP  			BIT(3)

//RF type
#define RF_TYPE_1T1R			(~(BIT(4)|BIT(5)|BIT(6)))
#define RF_TYPE_1T2R			BIT(4)
#define RF_TYPE_2T2R			BIT(5)
#define RF_TYPE_3T3R			(BIT(4)|BIT(5))

//Manufacture
#define CHIP_VENDOR_UMC			BIT(7)
#define CHIP_VENDOR_SMIC		BIT(8)
//Cut version
#define A_CUT_VERSION			0
#define B_CUT_VERSION			BIT(12)
#define C_CUT_VERSION			BIT(13)
#define D_CUT_VERSION			((BIT(12)|BIT(13)))
#define E_CUT_VERSION			BIT(14)
#define I_CUT_VERSION			BIT(15)
#define J_CUT_VERSION			((BIT(15)|BIT(12)))
#define K_CUT_VERSION			((BIT(15)|BIT(13)))

// MASK
#define IC_TYPE_MASK			(BIT(0)|BIT(1)|BIT(2))
#define CHIP_TYPE_MASK 			BIT(3)
#define RF_TYPE_MASK			(BIT(4)|BIT(5)|BIT(6))
#define MANUFACTUER_MASK		(BIT(7)|BIT(8))
#define ROM_VERSION_MASK		(BIT(9)|BIT(10)|BIT(11))
#define CUT_VERSION_MASK		(BIT(12)|BIT(13)|BIT(14)|BIT(15))

// Get element
#define GET_CVID_IC_TYPE(version)			((version) & IC_TYPE_MASK)
#define GET_CVID_CHIP_TYPE(version)			((version) & CHIP_TYPE_MASK)
#define GET_CVID_RF_TYPE(version)			((version) & RF_TYPE_MASK)
#define GET_CVID_MANUFACTUER(version)		((version) & MANUFACTUER_MASK)
#define GET_CVID_ROM_VERSION(version)		((version) & ROM_VERSION_MASK)
#define GET_CVID_CUT_VERSION(version)		((version) & CUT_VERSION_MASK)

// --------------------------------
// 			ChipVersion V2
// --------------------------------
// 2014.06.18 Define new structure of chip version for RTL8703B and so on. Added by TzuHang.
/*

 | BIT 31:28 |  BIT 27:24  | BIT 23:20 |  BIT 19:17  |    BIT16    |     BIT 15     |  BIT14  |   BIT 13:12   | BIT11:8 | BIT 7:0 |
 |-----------+-------------+-----------+-------------+-------------+----------------+---------+---------------+---------+---------|
 |  RESERVE  | CUT VERSION |  RESERVE  | ROM VERSION |  CHIP TYPE  | BT EXIST IN IC | RESERVE |  MANUFACTURE  | RF TYPE | IC NAME |
 |           |             |           |             | TEST/NORMAL |                |         | TSMC/UMC/SMIC |         |         |
	
*/
// Remaining the meaning corresponding to those value in each column we used before.
// And below is those be added newly
// [27:24] 	CUT VERSION: 	A-cut=0
// [19:17] 	ROM VERSION
// [16] 	CHIP TYPE:
// [15] 	BT exist: 		No=0, Yes=1 
// [13:12] 	MANUFACTURE: 	TSMC=0, UMC=1, SMIC=2
// [11:8] 	RF TYPE: 		1T1R=0, 1T2R=1, 2T2R=2, 3T3R=3
// [7:0] 	IC name: 		8703B=0x0B

// IC Name
#define CHIP_8821B				0x09
#define CHIP_8822B				0x0A
#define CHIP_8703B				0x0B
#define CHIP_8188F				0x0C
#define CHIP_8723D				0x0F

// RF Type
#define RF_TYPE_1T1R_v2			0
#define RF_TYPE_1T2R_v2			0x1
#define RF_TYPE_2T2R_v2			0x2
#define RF_TYPE_3T3R_v2			0x3

// Manufacture
#define CHIP_VENDOR_UMC_v2		0x1
#define CHIP_VENDOR_SMIC_v2		0x2

// BT exist or not
#define BT_NOT_EXIST_IN_CHIP	0
#define BT_EXIST_IN_CHIP		0x1

// Chip type
#define NORMAL_CHIP_v2			0x1

// Cut Version
#define A_CUT_VERSION_v2		0	
#define B_CUT_VERSION_v2		0x1
#define C_CUT_VERSION_v2		0x2
#define D_CUT_VERSION_v2		0x3
#define E_CUT_VERSION_v2		0x4
#define I_CUT_VERSION_v2		0x8

// GET element
#define SFT_AND_GET_FROM_CHIPVER(VERSION, MASK, SFT) ((VERSION>>SFT) & MASK)

#define GET_CVID_IC_TYPE_v2(VERSION)			SFT_AND_GET_FROM_CHIPVER(VERSION, 0xFF,  0)
#define GET_CVID_RF_TYPE_v2(VERSION)			SFT_AND_GET_FROM_CHIPVER(VERSION,  0xF,  8)
#define GET_CVID_MANUFACTUER_v2(VERSION)		SFT_AND_GET_FROM_CHIPVER(VERSION,  0x3, 12)
#define GET_CVID_BT_EXIST_v2(VERSION)			SFT_AND_GET_FROM_CHIPVER(VERSION,  0x1, 15)
#define GET_CVID_CHIP_TYPE_v2(VERSION)			SFT_AND_GET_FROM_CHIPVER(VERSION,  0x1, 16)
#define GET_CVID_ROM_VERSION_v2(VERSION)		SFT_AND_GET_FROM_CHIPVER(VERSION,  0x7, 17)
#define GET_CVID_CUT_VERSION_v2(VERSION)		SFT_AND_GET_FROM_CHIPVER(VERSION,  0xF, 24)

// MASK for fetching info ***From MAC***  (NOT FROM ChipVersion)
#define MK_CHIP_NAME			0xFF
#define MK_RF_TYPE				0xF << 27	// 0x8000000
#define MK_MANU					0x3 << 18	// 0x00C0000
#define MK_BT_EXIST				0x1 << 16	// 0x0010000
#define MK_CHIP_TYPE			0x1 << 23	// 0x0800000
#define MK_CUT_VER				0xF << 12	// 0x000F000

// The value of each info, which be aligned to head of the byte.
#define VAL_CHIP_NAME(BYTE4)	((BYTE4 & MK_CHIP_NAME))
#define VAL_RF_TYPE(BYTE4)		((BYTE4 & MK_RF_TYPE) 	>> 27)
#define VAL_MANU(BYTE4)			((BYTE4 & MK_MANU) 		>> 18)
#define VAL_BT_EXIST(BYTE4)		((BYTE4 & MK_BT_EXIST) 	>> 16)
#define VAL_CHIP_TYPE(BYTE4)	((BYTE4 & MK_CHIP_TYPE) >> 23)
#define VAL_CUT_VER(BYTE4)		((BYTE4 & MK_CUT_VER)	>> 12)

// SET into Chip_Version
#define SFT_AND_SET_IN_CHIPVER(BUFFER, INPUT, SFT) { BUFFER = (VERSION_8192C)(BUFFER | (INPUT<<SFT)); }

#define SET_CVID_CHIP_NAME(BUFFER, INPUT)	SFT_AND_SET_IN_CHIPVER(BUFFER, INPUT, 0)
#define SET_CVID_RF_TYPE(BUFFER, INPUT)		SFT_AND_SET_IN_CHIPVER(BUFFER, INPUT, 8)
#define SET_CVID_MANU(BUFFER, INPUT)		SFT_AND_SET_IN_CHIPVER(BUFFER, INPUT, 12)
#define SET_CVID_BT_EXIST(BUFFER, INPUT)	SFT_AND_SET_IN_CHIPVER(BUFFER, INPUT, 15)
#define SET_CVID_CHIP_TYPE(BUFFER, INPUT)	SFT_AND_SET_IN_CHIPVER(BUFFER, INPUT, 16)
#define SET_CVID_CUT_VER(BUFFER, INPUT)		SFT_AND_SET_IN_CHIPVER(BUFFER, INPUT, 24)

/*------------------------------ Chip/EFUSE Macro ----------------------------*/ 
//#pragma mark -- Chip/EFUSE Macro. --
//----------------------------------------------------------------------------
#define IS_81XXC(version)				((GET_CVID_IC_TYPE(version) == 0)? TRUE : FALSE)
#define IS_8188E_SERIES(version)		((GET_CVID_IC_TYPE(version) == CHIP_8188E)? TRUE : FALSE)
#define IS_8723A_SERIES(version)			((GET_CVID_IC_TYPE(version) == CHIP_8723A)? TRUE : FALSE)
#define IS_92D(version)					((GET_CVID_IC_TYPE(version) == CHIP_92D)? TRUE : FALSE)
#define IS_8812_SERIES(version)			((GET_CVID_IC_TYPE(version) == CHIP_8812)? TRUE : FALSE)
#define IS_8821_SERIES(version)			((GET_CVID_IC_TYPE(version) == CHIP_8821)? TRUE : FALSE)
#define IS_8723B_SERIES(version)			((GET_CVID_IC_TYPE(version) == CHIP_8723B)? TRUE : FALSE)
#define IS_8188F_SERIES(version)			((GET_CVID_IC_TYPE(version) == CHIP_8188F)? TRUE : FALSE)
#define IS_8814A_SERIES(version)			((GET_CVID_IC_TYPE(version) == CHIP_8814A)? TRUE : FALSE)

#define IS_NORMAL_CHIP(version)			((GET_CVID_CHIP_TYPE(version))? TRUE: FALSE)

#define IS_1T1R(version)					((GET_CVID_RF_TYPE(version))? FALSE : TRUE)
#define IS_1T2R(version)					((GET_CVID_RF_TYPE(version) == RF_TYPE_1T2R)? TRUE : FALSE)
#define IS_2T2R(version)					((GET_CVID_RF_TYPE(version) == RF_TYPE_2T2R)? TRUE : FALSE)
#define IS_3T3R(version)					((GET_CVID_RF_TYPE(version) == RF_TYPE_3T3R)? TRUE : FALSE)

#define IS_CHIP_VENDOR_TSMC(version)	((GET_CVID_MANUFACTUER(version) ==0)? TRUE: FALSE)
#define IS_CHIP_VENDOR_UMC(version)	((GET_CVID_MANUFACTUER(version) == CHIP_VENDOR_UMC)? TRUE: FALSE)
#define IS_CHIP_VENDOR_SMIC(version)	((GET_CVID_MANUFACTUER(version) == CHIP_VENDOR_SMIC)? TRUE: FALSE)

#define IS_B_CUT(_Adapter)			((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) == B_CUT_VERSION) ? TRUE : FALSE)
#define IS_C_CUT(_Adapter)			((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) == C_CUT_VERSION) ? TRUE : FALSE)
#define IS_C_CUT_AFTER(_Adapter)	((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) >= C_CUT_VERSION) ? TRUE : FALSE)


#define IS_92C_SERIAL(version)   			((IS_81XXC(version) && IS_2T2R(version)) ? TRUE : FALSE)
#define IS_81xxC_VENDOR_UMC_A_CUT(version)		(IS_81XXC(version)?((IS_CHIP_VENDOR_UMC(version)) ? ((GET_CVID_CUT_VERSION(version)) ? FALSE : TRUE) : FALSE):FALSE)
#define IS_81xxC_VENDOR_UMC_B_CUT(version)		(IS_81XXC(version)?(IS_CHIP_VENDOR_UMC(version) ? ((GET_CVID_CUT_VERSION(version) == B_CUT_VERSION) ? TRUE : FALSE):FALSE): FALSE)

#define IS_92D_SINGLEPHY(version)		((IS_92D(version)) ? (IS_2T2R(version) ? TRUE: FALSE) : FALSE)
#define IS_92D_C_CUT(version)			((IS_92D(version)) ? ((GET_CVID_CUT_VERSION(version) == C_CUT_VERSION) ? TRUE : FALSE) : FALSE)
#define IS_92D_D_CUT(version)			((IS_92D(version)) ? ((GET_CVID_CUT_VERSION(version) == D_CUT_VERSION) ? TRUE : FALSE) : FALSE)
#define IS_92D_E_CUT(version)			((IS_92D(version)) ? ((GET_CVID_CUT_VERSION(version) == E_CUT_VERSION) ? TRUE : FALSE) : FALSE)

#define IS_VENDOR_8723A_A_CUT(_Adapter)		((IS_8723A_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID)) ? FALSE : TRUE) : FALSE)
#define IS_VENDOR_8723A_B_CUT(_Adapter)		((IS_8723A_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) == B_CUT_VERSION) ? TRUE : FALSE) : FALSE)

#define IS_VENDOR_8188F_A_CUT(_Adapter)		((IS_8188F_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID)) ? FALSE : TRUE) : FALSE)
#define IS_VENDOR_8188F_B_CUT(_Adapter)		((IS_8188F_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) == B_CUT_VERSION) ? TRUE : FALSE) : FALSE)

#define IS_VENDOR_8188E_SMIC_C_CUT(_Adapter)		((IS_8188E_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) == K_CUT_VERSION) ? TRUE : FALSE) : FALSE)
#define IS_VENDOR_8188E_SMIC_SERIES(_Adapter)		((IS_8188E_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) >= I_CUT_VERSION) ? TRUE : FALSE) : FALSE)

#define IS_VENDOR_8812A_C_CUT(_Adapter)		((IS_8812_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) == C_CUT_VERSION) ? TRUE : FALSE) : FALSE)

#define IS_VENDOR_8821A_MP_CHIP(_Adapter)		((IS_8821_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((IS_NORMAL_CHIP(GET_HAL_DATA(_Adapter)->VersionID)) ? TRUE : FALSE) : FALSE)
#define IS_VENDOR_8821A_B_CUT(_Adapter)		((IS_8821_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) == B_CUT_VERSION) ? TRUE : FALSE) : FALSE)
#define IS_VENDOR_8821A_C_CUT(_Adapter)		((IS_8821_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) == C_CUT_VERSION) ? TRUE : FALSE) : FALSE)


#define IS_VENDOR_8723B_TEST_CHIP(_Adapter)	((IS_8723B_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((IS_NORMAL_CHIP(GET_HAL_DATA(_Adapter)->VersionID)) ? FALSE : TRUE) : FALSE)
#define IS_VENDOR_8723B_MP_CHIP(_Adapter)		((IS_8723B_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((IS_NORMAL_CHIP(GET_HAL_DATA(_Adapter)->VersionID)) ? TRUE : FALSE) : FALSE)
#define IS_VENDOR_8723B_A_CUT(_Adapter)		((IS_8723B_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) >> 12 == 0) ? TRUE : FALSE) : FALSE)
#define IS_VENDOR_8723B_B_CUT(_Adapter)		((IS_8723B_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) >> 12 == 1) ? TRUE : FALSE) : FALSE)
#define IS_VENDOR_8723B_C_CUT(_Adapter)		((IS_8723B_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) >> 12 == 1) ? TRUE : FALSE) : FALSE)
#define IS_VENDOR_8723B_D_CUT(_Adapter)		((IS_8723B_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) >> 12 == 3) ? TRUE : FALSE) : FALSE)
#define IS_VENDOR_8723B_D_CUT_BEFORE(_Adapter)		((IS_8723B_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) >> 12 <= 3) ? TRUE : FALSE) : FALSE)
#define IS_VENDOR_8723B_AFTER_F_CUT(_Adapter)		((IS_8723B_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((GET_CVID_CUT_VERSION(GET_HAL_DATA(_Adapter)->VersionID) >> 12 >= 4) ? TRUE : FALSE) : FALSE)



#define IS_VENDOR_8814A_TEST_CHIP(_Adapter)	((IS_8814A_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((IS_NORMAL_CHIP(GET_HAL_DATA(_Adapter)->VersionID)) ? FALSE : TRUE) : FALSE)
#define IS_VENDOR_8814A_MP_CHIP(_Adapter)		((IS_8814A_SERIES(GET_HAL_DATA(_Adapter)->VersionID)) ? ((IS_NORMAL_CHIP(GET_HAL_DATA(_Adapter)->VersionID)) ? TRUE : FALSE) : FALSE)


#define GET_RF_TYPE(_Adapter)			 		((u1Byte)(((PHAL_DATA_TYPE)(_Adapter->HalData))->RF_Type))
#define IS_BOOT_FROM_EEPROM(_Adapter)			(((PHAL_DATA_TYPE)(_Adapter->HalData))->EepromOrEfuse)
#define IS_BOOT_FROM_EEPROM_93C46(_Adapter)	(((PHAL_DATA_TYPE)(_Adapter->HalData))->EEType == EEPROM_93C46)
#define IS_BOOT_FROM_EEPROM_93C56(_Adapter)	(((PHAL_DATA_TYPE)(_Adapter->HalData))->EEType == EEPROM_93C56)
#define IS_MULTI_FUNC_CHIP(_Adapter)			(((((PHAL_DATA_TYPE)(_Adapter->HalData))->MultiFunc) & (RT_MULTI_FUNC_BT|RT_MULTI_FUNC_GPS)) ? TRUE : FALSE)
#define IS_POLARITY_HIGH_ACTIVE(_Adapter)		((((PHAL_DATA_TYPE)(_Adapter->HalData))->PolarityCtl) ? TRUE : FALSE)

// --------------------------------
// 			ChipVersion V2
// --------------------------------
#define IS_NORMAL_CHIP_v2(version)			((GET_CVID_CHIP_TYPE_v2(version))? TRUE: FALSE)

#define IS_1T1R_v2(version)					((GET_CVID_RF_TYPE_v2(version))? FALSE : TRUE)
#define IS_1T2R_v2(version)					((GET_CVID_RF_TYPE_v2(version) == RF_TYPE_1T2R_v2)? TRUE : FALSE)
#define IS_2T2R_v2(version)					((GET_CVID_RF_TYPE_v2(version) == RF_TYPE_2T2R_v2)? TRUE : FALSE)
#define IS_3T3R_v2(version)					((GET_CVID_RF_TYPE_v2(version) == RF_TYPE_3T3R_v2)? TRUE : FALSE)

#define IS_CHIP_VENDOR_TSMC_v2(version)		((GET_CVID_MANUFACTUER_v2(version) ==0)? TRUE: FALSE)
#define IS_CHIP_VENDOR_UMC_v2(version)		((GET_CVID_MANUFACTUER_v2(version) == CHIP_VENDOR_UMC_v2)? TRUE: FALSE)
#define IS_CHIP_VENDOR_SMIC_v2(version)		((GET_CVID_MANUFACTUER_v2(version) == CHIP_VENDOR_SMIC_v2)? TRUE: FALSE)

#define IS_88E_SERIES(_Adapter)				(((PHAL_DATA_TYPE)(_Adapter->HalData))->bIs88eSeries)
#define IS_NIC_EXIST_BT(_Adapter)			(((PHAL_DATA_TYPE)(_Adapter->HalData))->bIsNicExistBt)
#define IS_NIC_SUPPORT_WAPI(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->bNicSupportWapi)
#define ENABLE_SINGLE_AMPDU(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->bEnableSingleAmpdu)
#define IS_NEED_EXT_IQK(_Adapter)			(((PHAL_DATA_TYPE)(_Adapter->HalData))->bNeedExtIQK)
#define CHECK_HIDDEN_SSID(_Adapter)			(((PHAL_DATA_TYPE)(_Adapter->HalData))->bCheckHiddenSsid)
#define DISABLE_RTS_ON_AMPDU(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->bDisableRtsOnAmpdu)
#define IS_SUP_CLK_ON_EXCEP_RESN(_Adapter)	(((PHAL_DATA_TYPE)(_Adapter->HalData))->bSupClkOnExcepResn)
#define IS_REMOVE_ZERO_LEN_BUF(_Adapter)	(((PHAL_DATA_TYPE)(_Adapter->HalData))->bRemoveZeroLenBuf)
#define IS_LEAVE_PS_BF_EN_NIC(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->bLeavePsBeforeEnNic)


#define WLAN_802_11N					0		
#define WLAN_802_11AC					1

#define IS_DOT_11N_SERIES(_Adapter)			(((PHAL_DATA_TYPE)(_Adapter->HalData))->bNicWirelessMode == WLAN_802_11N)
#define IS_DOT_11AC_SERIES(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->bNicWirelessMode == WLAN_802_11AC)

#define GET_CHIP_ID(_Adapter)					(((PHAL_DATA_TYPE)(_Adapter->HalData))->ChipID)
#define GET_CHIP_VERSION(_Adapter)				(((PHAL_DATA_TYPE)(_Adapter->HalData))->ChipVer)
#define GET_MAX_RX_DMA_BUFFER_SIZE(_Adapter)	(((PHAL_DATA_TYPE)(_Adapter->HalData))->MaxRxDmaBufferSize)
#define GET_MAX_TX_BUFFER_SIZE(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->MaxTxBufSize)
#define GET_TCB_NUM_OF_MEM_SIZE_0(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->TcbNumOfMemSize0)
#define GET_TCB_NUM_OF_MEM_SIZE_1(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->TcbNumOfMemSize1)
#define GET_TX_DESC_NUM(_Adapter)				(((PHAL_DATA_TYPE)(_Adapter->HalData))->TxDescNum)
#define GET_TX_DESC_NUM_BE_Q(_Adapter)			(((PHAL_DATA_TYPE)(_Adapter->HalData))->TxDescNumBeQ)
#define GET_TX_DESC_NUM_MP(_Adapter)			(((PHAL_DATA_TYPE)(_Adapter->HalData))->TxDescNumMp)
#define GET_TX_DESC_NUM_BE_Q_MP(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->TxDescNumBeQMp)

#define GET_MAX_RX_BUFFER_SIZE(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->MaxRxBufSize)
#define GET_MAX_SUBFRAME_CNT_MM_SIZE0(_Adapter)	(((PHAL_DATA_TYPE)(_Adapter->HalData))->MaxSubframeCntMmSize0)
#define GET_MAX_SUBFRAME_CNT_MM_SIZE1(_Adapter)	(((PHAL_DATA_TYPE)(_Adapter->HalData))->MaxSubframeCntMmSize1)
#define GET_RFD_NUM_OF_MM_SIZE_0(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->RfdNumMmSize0)
#define GET_RFD_DESC_NUM_OF_MM_SIZE_0(_Adapter)	(((PHAL_DATA_TYPE)(_Adapter->HalData))->RxDescNumMmSize0)
#define GET_RFD_NUM_OF_MM_SIZE_1(_Adapter)		(((PHAL_DATA_TYPE)(_Adapter->HalData))->RfdNumMmSize1)
#define GET_RFD_DESC_NUM_OF_MM_SIZE_1(_Adapter)	(((PHAL_DATA_TYPE)(_Adapter->HalData))->RxDescNumMmSize1)

#define IS_PCI_INERFACE(_Adapter)				(((PHAL_DATA_TYPE)(_Adapter->HalData))->ChipVer == 0x1)
#define IS_USB_INERFACE(_Adapter)				(((PHAL_DATA_TYPE)(_Adapter->HalData))->ChipVer == 0x2)
#define IS_SDIO_INERFACE(_Adapter)				(((PHAL_DATA_TYPE)(_Adapter->HalData))->ChipVer == 0x3)




/*------------------------------ RTL81xxxDefCom.h Definiction -----------------------*/ 
//Move 8192CDefCom.h/8188EDefCom.h to here.

#define	HAL_DM_DIG_DISABLE					BIT0	// Disable Dig
#define	HAL_DM_HIPWR_DISABLE				BIT1	// Disable High Power

#define	PHY_RSSI_SLID_WIN_MAX				100
#define	PHY_LINKQUALITY_SLID_WIN_MAX		20


//-----------------------------------------------------------
//
//	Queue mapping
//
//-----------------------------------------------------------
#define BK_QUEUE						0		
#define BE_QUEUE						1		
#define VI_QUEUE						2		
#define VO_QUEUE						3		
#define BEACON_QUEUE					4		
#define TXCMD_QUEUE					5		
#define MGNT_QUEUE						6
#define HIGH_QUEUE						7
#define HCCA_QUEUE						8

#define HIGH0_QUEUE					HIGH_QUEUE
#define HIGH1_QUEUE					9
#define HIGH2_QUEUE					10
#define HIGH3_QUEUE					11
#define HIGH4_QUEUE					12
#define HIGH5_QUEUE					13
#define HIGH6_QUEUE					14
#define HIGH7_QUEUE					15

#define LOW_QUEUE						BE_QUEUE
#define NORMAL_QUEUE					MGNT_QUEUE
#if (NEW_EARLY_MODE_ENABLE==1)
#define NEW_EARLY_MODE_QUEUE			BE_QUEUE //add  by ylb 20130409
#endif

#define IS_DATA_QUEUE(_queueIdx)	(((BK_QUEUE == _queueIdx) || (BE_QUEUE == _queueIdx) || (VI_QUEUE == _queueIdx) || (VO_QUEUE == _queueIdx))? TRUE: FALSE)
//
// Queue Select Value in TxDesc
//
#define QSLT_BK							0x2//0x01
#define QSLT_BE							0x0
#define QSLT_VI							0x5//0x4
#define QSLT_VO							0x7//0x6
#define QSLT_BEACON						0x10
#define QSLT_HIGH						0x11
#define QSLT_MGNT						0x12
#define QSLT_CMD						0x13

#define RX_MPDU_QUEUE					0
#define RX_CMD_QUEUE					1
#define RX_MAX_QUEUE					2



#define RX_DESC_SIZE			32

#define			HAL_DEFAULT_BEACON_TYPE	BEACON_SEND_AUTO_HW

//-----------------------------------------------------------
//	MAC/BB/RF
//-----------------------------------------------------------

#define	MAX_LINES_HWCONFIG_TXT			2000
#define	MAX_BYTES_LINE_HWCONFIG_TXT		256


// Rx smooth factor
#define	Rx_Smooth_Factor				20


//H2C related
typedef enum _H2C_STATUS{
	H2C_STATUS_SUCCESS,
	H2C_STATUS_FAIL_ANOTHER_H2C_UNDER_PROGRESS,
	H2C_STATUS_FAIL_RF_OFF,
	H2C_STATUS_FAIL_FW_READ_CLEAR_TIMEOUT,
	H2C_STATUS_MAX
}H2C_STATUS,*PH2C_STATUS;

#define MAX_H2C_QUEUE_NUM				64

/*------------------------------ end of RTL81xxxDefCom.h Definiction -----------------------*/ 


// For store intial value of BB register
typedef struct _BB_INIT_REGISTER{
	u2Byte	offset;
	
	u4Byte	value;
	
}BB_INIT_REGISTER, *PBB_INIT_REGISTER;



// SDIO Tx FIFO related
#define	SDIO_TX_FREE_PG_QUEUE			5	// The number of Tx FIFO free page, 8821B: HIQ/MIQ/LOQ/PUBQ/EXTQ.
#define	SDIO_TX_FREE_PG_QUEUE_8814A		5	//20130422 KaiYuan add for 8814 Extra Queue

//#define SDIO_TX_FIFO_PAGE_SZ 			128	// For RTL8723A/RTL8723B 32K Tx FIFO Size
//#define	SDIO_TX_FIFO_PAGE_SZ_8821		256

#define SDIO_TX_FIFO_PAGE_SIZE(_Adapter)	\
		((IS_HARDWARE_TYPE_8812A(_Adapter) || IS_HARDWARE_TYPE_8821A(_Adapter) \
			|| IS_HARDWARE_TYPE_8192E(_Adapter))? \
			256 : 128)



typedef struct _QUEUE_INDEX_LIST
{
	u1Byte	QueueIdxArray[32];
	u1Byte	Len;
}QUEUE_INDEX_LIST, *PQUEUE_INDEX_LIST;

#if RX_AGGREGATION
typedef enum _RX_AGG_MODE{
	RX_AGG_DISABLE,
	RX_AGG_DMA,
	RX_AGG_USB,
	RX_AGG_DMA_USB
}RX_AGG_MODE, *PRX_AGG_MODE;
#endif

typedef struct _LOG_SYS_INTERRUPT
{
	u4Byte	nISR_GPIO9;
	u4Byte	nISR_GPIO12_0;
	u4Byte	nISR_SPS_OCP;
	u4Byte	nISR_RON;
	u4Byte	nISR_PDN;
	
} LOG_SYS_INTERRUPT, *PLOG_SYS_INTERRUPT;



//should be renamed and moved to another file
typedef	enum _INTERFACE_SELECT_8192CUSB{
	INTF_SEL0_USB 			= 0,		// USB
	INTF_SEL1_USB_High_Power  	= 1,		// USB with high power PA
	INTF_SEL2_MINICARD		  	= 2,		// Minicard
	INTF_SEL3_USB_Solo 		 	= 3,		// USB solo-Slim module
	INTF_SEL4_USB_Combo			= 4,		// USB Combo-Slim module
	INTF_SEL5_USB_Combo_MF		= 5,		// USB WiFi+BT Multi-Function Combo, i.e., Proprietary layout(AS-VAU) which is the same as SDIO card
} INTERFACE_SELECT_8192CUSB, *PINTERFACE_SELECT_8192CUSB;

typedef INTERFACE_SELECT_8192CUSB INTERFACE_SELECT_USB;

typedef struct _LOG_INTERRUPT
{
	u4Byte	nISR_RX_REQUEST;
	u4Byte	nISR_AVAL;
	u4Byte	nISR_TXERR;
	u4Byte	nISR_RXERR;
	u4Byte	nISR_TXFOVW;
	u4Byte	nISR_RXFOVW;
	u4Byte	nISR_TXBCNOK;
	u4Byte	nISR_TXBCNERR;	
	u4Byte	nISR_BCNERLY_INT;
	u4Byte	nISR_C2HCMD;
	u4Byte	nISR_CPWM1;
	u4Byte	nISR_CPWM2;
	u4Byte	nISR_HSISR_IND;
	u4Byte	nISR_GTINT3_IND;
	u4Byte	nISR_GTINT4_IND;
	u4Byte	nISR_PSTIMEOUT;	
	u4Byte	nISR_OCPINT;	
	u4Byte	nISR_ATIMEND;	
	u4Byte	nISR_ATIMEND_E;	
	u4Byte	nISR_CTWEND;	
	u4Byte	nISR_MCU_ERR; // For 8188ES only
	u4Byte	nISR_BIT32_TOGGLE; // For 8188ES only
	u4Byte	nIMR_C2HCMD;
} LOG_INTERRUPT, *PLOG_INTERRUPT;


u4Byte
HAL_GetCutVersion(
	IN	PADAPTER	pAdapter
	);
VOID
HAL_SetBcnCtrlReg(
	IN	PADAPTER	Adapter,
	IN	u1Byte		SetBits,
	IN	u1Byte		ClearBits
	);
VOID 
HAL_UpdateDefaultSetting(
	IN	PADAPTER		pAdapter
	);

BOOLEAN
HAL_ChipSupport80MHz(
	IN	PADAPTER	Adapter
	);

BOOLEAN
HAL_WorkAroundForLowTxRaise(
	IN	PADAPTER	Adapter
	);

BOOLEAN
HAL_DisableEdcaTurbo(
	IN	PADAPTER	Adapter
	);

BOOLEAN
HAL_EnableBeQTxOpLimit(
	IN	PADAPTER	Adapter
	);

BOOLEAN
HAL_ActBcmRxFailRelink(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pBssDesc
	);

BOOLEAN
HAL_IsEdcaBiasRx(
	IN	PADAPTER		Adapter,
	IN	PRT_WLAN_BSS	pBssDesc
	);

BOOLEAN
HAL_SkipMcsRates(
	IN	PADAPTER	Adapter
	);

VOID
HalSetFcsAdjustTsf(
	IN	PADAPTER			Adapter
	);

VOID
HAL_DownloadRSVD_PreCfg(
	IN PADAPTER	Adapter
);
RT_STATUS
HAL_DownloadRSVD_PostCfg(
	IN PADAPTER	Adapter
);
BOOLEAN IS_HAL_MAC_INIT(
	IN PADAPTER Adapter
);
	
#endif 
