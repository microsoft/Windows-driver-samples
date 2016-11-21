/******************************************************************************
 * 
 *     (c) Copyright  2012, RealTEK Technologies Inc. All Rights Reserved.
 * 
 * Module:	Hal8723BEfuse.c	( Source C File)
 * 
 * Note:		Copy from WMAC for the first version!!!!
 *			
 *
 * Function:	
 * 		 
 * Export:	
 * 
 * Abbrev:	
 * 
 * History:
 * Data			Who		Remark
 * 
 * 07/10/2012	Kordan  Set an independent files.
 * 	
******************************************************************************/
#ifndef __8723B_EFUSE_H__
#define __8723B_EFUSE_H__

#define		HWSET_MAX_SIZE_8723B			512

#define		EFUSE_REAL_CONTENT_LEN_8723B	512

#define		EFUSE_MAP_LEN_8723B			512
#define		EFUSE_MAX_SECTION_8723B		64
#define		EFUSE_MAX_WORD_UNIT_8723B		4
#define		EFUSE_IC_ID_OFFSET_8723B		506	//For some inferiority IC purpose. added by Roger, 2009.09.02.
#define 	AVAILABLE_EFUSE_ADDR_8723B(addr) 	(addr < EFUSE_REAL_CONTENT_LEN_8723B)


//========================================================
//			EFUSE for BT definition
//========================================================
#define		EFUSE_BT_REAL_BANK_CONTENT_LEN_8723B	512
#define		EFUSE_BT_REAL_CONTENT_LEN_8723B			1024	// 512*2
#define		EFUSE_BT_MAP_LEN_8723B					1024	// 1k bytes
#define		EFUSE_BT_MAX_SECTION_8723B				128		// 1024/8

#define		EFUSE_PROTECT_BYTES_BANK_8723B			16
#define 	EFUSE_MAX_BANK_8723B					3
//===========================================================

//			EFUSE Parsing
//===========================================================
VOID 
Hal_EfuseParseBTCoexistInfo8723B(
	IN PADAPTER			Adapter,
	IN pu1Byte			hwinfo,
	IN BOOLEAN			AutoLoadFail
	);

VOID
Hal_ReadChannelPlan8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	);

VOID
Hal_EfuseParseEEPROMVer8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			hwinfo,
	IN	BOOLEAN			AutoLoadFail
	);
VOID
Hal_EfuseParseChnlPlan8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			hwinfo,
	IN	BOOLEAN			AutoLoadFail
	);
VOID
Hal_EfuseParseCustomerID8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			hwinfo,
	IN	BOOLEAN			AutoLoadFail
	);
VOID
Hal_EfuseParseAntennaDiversity8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			hwinfo,
	IN	BOOLEAN			AutoLoadFail
	);

VOID
Hal_EfuseParseIDCode8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			hwinfo
	);

VOID
Hal_InitChannelPlan8723B(
	IN		PADAPTER	pAdapter
	);


VOID
Hal_EfuseParseRateIndicationOption8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			hwinfo,
	IN	BOOLEAN			AutoLoadFail
	);

VOID
Hal_EfuseParseXtal_8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			hwinfo,
	IN	BOOLEAN			AutoLoadFail
	);

VOID
Hal_ReadThermalMeter_8723B(
	IN	PADAPTER	Adapter,	
	IN	pu1Byte 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	);

VOID
Hal_ReadAntennaDiversity8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	);

VOID
Hal_EfuseReadEFuse8723B(
	PADAPTER	Adapter,
	u2Byte		 _offset,
	u2Byte 		_size_byte,
	u1Byte      	*pbuf,
	IN	BOOLEAN		bPseudoTest
	);

VOID
Hal_EfuseReadEFuse8723B(
	PADAPTER	Adapter,
	u2Byte		 _offset,
	u2Byte 		_size_byte,
	u1Byte      	*pbuf,
	IN	BOOLEAN	bPseudoTest
	);



VOID
Hal_ReadTxPowerInfo8723B(
	IN	PADAPTER 		Adapter,
	IN	pu1Byte			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	);

VOID
Hal_ReadBoardType8723B(
	IN	PADAPTER	Adapter,	
	IN	pu1Byte		PROMContent,
	IN	BOOLEAN		AutoloadFail
	);




BOOLEAN
Hal_EfuseSwitchToBank8723B(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		bank,
	IN		BOOLEAN		bPseudoTest
	);


BOOLEAN 
Hal_GetChnlGroup8723B(
	IN	u1Byte Channel,
	OUT pu1Byte	pGroup
	);

VOID
Hal_ReadPROMVersion8723B(
	IN	PADAPTER	Adapter,	
	IN	pu1Byte 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	);

VOID
Hal_ReadPAType_8723B(
	IN	PADAPTER	Adapter,
	IN	pu1Byte		PROMContent,
	IN	BOOLEAN		AutoloadFail
	);

VOID
Hal_InitEfuseVars_8723B(
	IN PEFUSE_HAL		pEfuseHal
	);

VOID
Hal_EfusePowerSwitch8723B(
	IN	PADAPTER	pAdapter,
	IN	UINT8		bWrite,
	IN	UINT8		PwrState);

VOID
Hal_EfusePowerSwitch8723B_TestChip(
	IN	PADAPTER	pAdapter,
	IN	UINT8		bWrite,
	IN	UINT8		PwrState);

VOID
Hal_EfuseParsePackageType_8723B(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			hwinfo,
	IN	BOOLEAN		AutoLoadFail
	);


#endif

 
