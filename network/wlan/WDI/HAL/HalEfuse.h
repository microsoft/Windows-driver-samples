/******************************************************************************
 * 
 *     (c) Copyright  2008, RealTEK Technologies Inc. All Rights Reserved.
 * 
 *		03/04/2011 Cosa Modify
 * 
******************************************************************************/
#ifndef	__INC_HAL_EFUSE_H__
#define	__INC_HAL_EFUSE_H__

//=============================================
//	Here we define which chip need to be supported or not
//=============================================
#define	SUPPORT_EFUSE					1
#if(SUPPORT_EFUSE == 1)

#define	EFUSE_SUPPORT_92S				1
#define	EFUSE_SUPPORT_92C				1
#define	EFUSE_SUPPORT_92D				1
#define	EFUSE_SUPPORT_8188E				1	
#define	EFUSE_SUPPORT_8188F				1	
#define	EFUSE_SUPPORT_8812A				1
#define	EFUSE_SUPPORT_8811A				1	
#define	EFUSE_SUPPORT_8821A				1	
#define	EFUSE_SUPPORT_8192E				1	
#define	EFUSE_SUPPORT_8814A				1	
#define	EFUSE_SUPPORT_8723B				1
#define	EFUSE_SUPPORT_8703B				1
#define	EFUSE_SUPPORT_8822B				1	
#define	EFUSE_SUPPORT_8723D				1	

#else
#define	EFUSE_SUPPORT_92S					0
#define	EFUSE_SUPPORT_92C					0
#define	EFUSE_SUPPORT_92D					0
#define	EFUSE_SUPPORT_8188E					0
#define	EFUSE_SUPPORT_8188F					0
#define	EFUSE_SUPPORT_8812A			       	0
#define	EFUSE_SUPPORT_8192E					0
#define	EFUSE_SUPPORT_8814A					0
#endif
//=============================================


#define	HWSET_MAX_SIZE_128			128
#define	HWSET_MAX_SIZE_256			256
#define	HWSET_MAX_SIZE_512			512

#define	EFUSE_BANK_SIZE				512

#define	EFUSE_ERROE_HANDLE			1
/*--------------------------Define Parameters-------------------------------*/
#define	EFUSE_INIT_MAP					0
#define	EFUSE_MODIFY_MAP				1

#define	_TRUE							1
#define	_FALSE							0

#define	EFUSE_CLK_CTRL					EFUSE_CTRL
#ifdef BIT
	#undef BIT
#endif
#define 	BIT(x)  							(1 << (x))


#define	PG_STATE_HEADER 				0x01
#define	PG_STATE_WORD_0				0x02
#define	PG_STATE_WORD_1				0x04
#define	PG_STATE_WORD_2				0x08
#define	PG_STATE_WORD_3				0x10
#define	PG_STATE_DATA					0x20

#define	PG_SWBYTE_H					0x01
#define	PG_SWBYTE_L					0x02

#define	PGPKT_DATA_SIZE 				8

#define	EFUSE_WIFI						0
#define	EFUSE_BT						1

enum{
		VOLTAGE_V25						= 0x03,
		LDOE25_SHIFT						= 28 ,
	};

enum _EFUSE_DEF_TYPE {
	TYPE_EFUSE_MAX_SECTION				= 0,
	TYPE_EFUSE_REAL_CONTENT_LEN			= 1,
	TYPE_AVAILABLE_EFUSE_BYTES_BANK		= 2,
	TYPE_AVAILABLE_EFUSE_BYTES_TOTAL	= 3,
	TYPE_EFUSE_MAP_LEN					= 4,
	TYPE_EFUSE_PROTECT_BYTES_BANK		= 5,
	TYPE_EFUSE_CONTENT_LEN_BANK			= 6,
};

#define		EFUSE_MAX_MAP_LEN			1024
#define		EFUSE_MAX_HW_SIZE			1024

#define		EFUSE_MAX_SECTION_BASE			16

typedef enum _PACKAGE_TYPE_E
{
    PACKAGE_DEFAULT,
    PACKAGE_QFN68,
    PACKAGE_TFBGA90,
    PACKAGE_TFBGA80,
    PACKAGE_TFBGA79
}PACKAGE_TYPE_E;


#define IS_AVAILABLE_EFUSE_ADDR(_Adapter, _Addr)	\
		(IS_HARDWARE_TYPE_8188E(_Adapter) ? AVAILABLE_EFUSE_ADDR_88E(_Addr) : AVAILABLE_EFUSE_ADDR_92C(_Addr))
#define IS_AVAILABLE_BT_EFUSE_ADDR(_eFuse_Addr, _endBank) (_eFuse_Addr < (_endBank+1) * EFUSE_MAX_BANK_SIZE)

#define EXT_HEADER(header) ((header & 0x1F ) == 0x0F)
#define ALL_WORDS_DISABLED(wde)	((wde & 0x0F) == 0x0F)
#define GET_HDR_OFFSET_2_0(header) ( (header & 0xE0) >> 5)

#define		EFUSE_REPEAT_THRESHOLD_			0
#define		EFUSE_REPEAT_HEADER_THRESHOLD	0

#define		EFUSE_MAP_LEN					128
#define		EFUSE_MAX_SECTION				16
#define		EFUSE_MAX_WORD_UNIT				4

//
// <Roger_Notes> To prevent out of boundary programming case, leave 1byte and program full section
// 9bytes + 1byt + 5bytes and pre 1byte.
// For worst case:
// | 1byte|----8bytes----|1byte|--5bytes--| 
// |         |            Reserved(14bytes)	      |
//
#define		EFUSE_OOB_PROTECT_BYTES 		15	// PG data exclude header, dummy 6 bytes frome CP test and reserved 1byte.


//=============================================

#define IS_MASKED_MP(ic, txt, offset) (EFUSE_IsAddressMasked_MP_##ic##txt(offset))
#define IS_MASKED_TC(ic, txt, offset) (EFUSE_IsAddressMasked_TC_##ic##txt(offset))
#define GET_MASK_ARRAY_LEN_MP(ic, txt) (EFUSE_GetArrayLen_MP_##ic##txt())
#define GET_MASK_ARRAY_LEN_TC(ic, txt) (EFUSE_GetArrayLen_TC_##ic##txt())
#define GET_MASK_ARRAY_MP(ic, txt, offset) (EFUSE_GetMaskArray_MP_##ic##txt(offset))
#define GET_MASK_ARRAY_TC(ic, txt, offset) (EFUSE_GetMaskArray_TC_##ic##txt(offset))


#define IS_MASKED(ic, txt, offset) ((pHalData->bIsMPChip) ? IS_MASKED_MP(ic,txt, offset) : IS_MASKED_TC(ic,txt, offset))
#define GET_MASK_ARRAY_LEN(ic, txt) ((pHalData->bIsMPChip) ? GET_MASK_ARRAY_LEN_MP(ic,txt) : GET_MASK_ARRAY_LEN_TC(ic,txt))
#define GET_MASK_ARRAY(ic, txt, out) do {((pHalData->bIsMPChip) ? GET_MASK_ARRAY_MP(ic,txt, out) : GET_MASK_ARRAY_TC(ic,txt, out));} while(0)



//=============================================
//	The following is for BT Efuse definition
//=============================================
#define		EFUSE_BT_MAX_MAP_LEN		1024
#define		EFUSE_MAX_BANK			4
#define		EFUSE_MAX_BANK_SIZE			512
#define		EFUSE_MAX_BT_BANK		(EFUSE_MAX_BANK-1)
//=============================================
/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 
typedef struct PG_PKT_STRUCT_A{
	UINT8 offset;
	UINT8 word_en;
	UINT8 data[8];	
	u1Byte word_cnts;
}PGPKT_STRUCT,*PPGPKT_STRUCT;


typedef struct _EFUSE_HAL{
	u1Byte	fakeEfuseBank;
	u4Byte	fakeEfuseUsedBytes;
	u1Byte	fakeEfuseContent[EFUSE_MAX_HW_SIZE];
	u1Byte	fakeEfuseInitMap[EFUSE_MAX_MAP_LEN];
	u1Byte	fakeEfuseModifiedMap[EFUSE_MAX_MAP_LEN];

	u4Byte	EfuseUsedBytes;
	u1Byte	EfuseUsedPercentage;

	u2Byte	BTEfuseUsedBytes;
	u1Byte	BTEfuseUsedPercentage;
	u1Byte	BTEfuseContent[EFUSE_MAX_BT_BANK][EFUSE_MAX_HW_SIZE];
	u1Byte	BTEfuseInitMap[EFUSE_BT_MAX_MAP_LEN];
	u1Byte	BTEfuseModifiedMap[EFUSE_BT_MAX_MAP_LEN];

	u2Byte	fakeBTEfuseUsedBytes;
	u1Byte	fakeBTEfuseContent[EFUSE_MAX_BT_BANK][EFUSE_MAX_HW_SIZE];
	u1Byte	fakeBTEfuseInitMap[EFUSE_BT_MAX_MAP_LEN];
	u1Byte	fakeBTEfuseModifiedMap[EFUSE_BT_MAX_MAP_LEN];

	// EFUSE Configuration, initialized in HAL_CmnInitPGData().
	const u2Byte  MaxSecNum_WiFi;
	const u2Byte  MaxSecNum_BT;	
	const u2Byte  WordUnit;	
	const u2Byte  PhysicalLen_WiFi;
	const u2Byte  PhysicalLen_BT;	
	const u2Byte  LogicalLen_WiFi;
	const u2Byte  LogicalLen_BT;	
	const u2Byte  BankSize;
	const u2Byte  TotalBankNum;
	const u2Byte  BankNum_WiFi;
	const u2Byte  BankNum_BT;	
	const u2Byte  OOBProtectBytes;
	const u2Byte  ProtectBytes;
	const u2Byte  BankAvailBytes;	
	const u2Byte  TotalAvailBytes_WiFi;
	const u2Byte  TotalAvailBytes_BT;	

	const u2Byte  HeaderRetry;	
	const u2Byte  DataRetry;	

	RT_ERROR_CODE 	  Status;
}EFUSE_HAL, *PEFUSE_HAL;
/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/

/*------------------------Export Marco Definition---------------------------*/

/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/

extern	VOID
EFUSE_ShadowRead(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		ByteCount,
	IN		u2Byte		Offset,
	IN OUT	u4Byte		*Value	);

extern	VOID
EFUSE_MaskedShadowRead(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		ByteCount,
	IN		u2Byte		Offset,
	IN OUT	u4Byte		*Value	);

extern	VOID
EFUSE_ShadowWrite(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		Type,
	IN		u2Byte		Offset,
	IN OUT	u4Byte		Value	);
BOOLEAN
EFUSE_ShadowUpdate(
	IN		PADAPTER	pAdapter,
	IN		BOOLEAN		bPseudoTest
	);
BOOLEAN
EFUSE_ShadowUpdateChk(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		efuseType,
	IN		BOOLEAN		bPseudoTest);
VOID 
EFUSE_RenewShadowMaps(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		efuseType,
	IN		BOOLEAN		bPseudoTest);


extern	VOID 
EFUSE_ForceWriteVendorId(
	IN		PADAPTER	pAdapter);

BOOLEAN	
EFUSE_ProgramMap(
	IN	PADAPTER	Adapter,
	IN	ps1Byte 		pFileName,
	IN	u1Byte		TableType,
	IN	BOOLEAN		bPseudoTest);		// 0=Shadow 1=Real Efuse

extern	VOID
EFUSE_ResetLoader(
	IN	PADAPTER	pAdapter);



VOID
Hal_InitEfuseVars(
	IN PEFUSE_HAL		pEfuseHal
	);


VOID
Hal_ReadEFuseByIC(
	PADAPTER	Adapter,
	u1Byte		efuseType,
	u2Byte		 _offset,
	u2Byte 		_size_byte,
	u1Byte      	*pbuf,
	IN	BOOLEAN	bPseudoTest
	);
VOID
Hal_ReadEFuse_Pseudo(
	PADAPTER	Adapter,
	u1Byte		efuseType,
	u2Byte		 _offset,
	u2Byte 		_size_byte,
	u1Byte      	*pbuf,
	IN	BOOLEAN	bPseudoTest
	);
VOID 
EFUSE_LoadMap(
	IN		PADAPTER	pAdapter, 
	IN		u1Byte		efuseType,
	IN OUT	u1Byte		*Efuse,
	IN		BOOLEAN		bPseudoTest);

VOID
EFUSE_PowerSwitch(
	IN	PADAPTER	pAdapter,
	IN	UINT8		bWrite,
	IN	UINT8		PwrState);
INT32 
EFUSE_PgPacketWrite(
	IN	PADAPTER	pAdapter,
	IN 	UINT8		efuseType,
	IN	UINT8 		offset,
	IN	UINT8		word_en,
	IN	UINT8		*data,
	IN	BOOLEAN		bPseudoTest);

VOID
EFUSE_FakeShadowRead1Byte(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		efuseType,
	IN		u2Byte		Offset,
	IN OUT	UINT8		*Value	);
VOID
EFUSE_FakeShadowWrite1Byte(
	IN		PADAPTER	pAdapter,
	IN		u2Byte		Offset,
	IN 		UINT8		Value	);
BOOLEAN
EFUSE_FakeContentRead1Byte(
	IN		PADAPTER	pAdapter,
	IN		u2Byte		Offset,
	IN OUT	UINT8		*Value	);
BOOLEAN
EFUSE_FakeContentWrite1Byte(
	IN		PADAPTER	pAdapter,
	IN		u2Byte		Offset,
	IN 		UINT8		Value	);
VOID
EFUSE_GetEfuseDefinition(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		efuseType,
	IN		u1Byte		type,
	OUT		pu2Byte 	pOut,
	IN		BOOLEAN		bPseudoTest
	);
u2Byte
EFUSE_GetCurrentSize(
	IN		PADAPTER		pAdapter,
	IN		u1Byte			efuseType,
	IN		BOOLEAN			bPseudoTest);
UINT8
EFUSE_CalculateWordCnts(
	IN UINT8	word_en);

void
EFUSE_WordEnableDataRead(	IN	UINT8	word_en,
							IN	UINT8	*sourdata,
							IN	UINT8	*targetdata);
INT32
EFUSE_OneByteRead(
	IN	PADAPTER	pAdapter, 
	IN	u2Byte		addr,
	IN	UINT8		*data,
	IN	BOOLEAN		bPseudoTest);
INT32
EFUSE_OneByteWrite(
	IN	PADAPTER	pAdapter,  
	IN	u2Byte		addr, 
	IN	UINT8		data,
	IN	BOOLEAN		bPseudoTest);

INT32
EFUSE_PgPacketRead(	IN	PADAPTER	pAdapter,
					IN	UINT8			offset,
					IN	UINT8			*data,
					IN	BOOLEAN			bPseudoTest);
UINT8
EFUSE_WordEnableDataWrite(	IN	PADAPTER	pAdapter,
							IN	u2Byte		efuse_addr,
							IN	UINT8		word_en, 
							IN	UINT8		*data,
							IN	BOOLEAN		bPseudoTest);

BOOLEAN
EFUSE_ContentWrite1Byte(
	IN		PADAPTER	pAdapter,
	IN		UINT8		bank,
	IN		u2Byte		Offset,
	IN 		UINT8		Value	);
	
VOID
efuse_ShadowRead1Byte_BT(
	IN		PADAPTER	pAdapter,
	IN		u2Byte		Offset,
	IN OUT	UINT8		*Value	);
VOID
EFUSE_ShadowReadBT(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		Type,
	IN		u2Byte		Offset,
	IN OUT	u4Byte		*Value	);
VOID
efuse_ShadowWrite1Byte_BT(
	IN		PADAPTER	pAdapter,
	IN		u2Byte		Offset,
	IN 		UINT8		Value	);
VOID
EFUSE_ShadowWriteBT(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		Type,
	IN		u2Byte		Offset,
	IN OUT	u4Byte		Value	);
VOID
EFUSE_FakeShadowWrite1Byte_BT(
	IN		PADAPTER	pAdapter,
	IN		u2Byte		Offset,
	IN 		UINT8		Value	);

BOOLEAN
EFUSE_SwitchToBank(
	IN		PADAPTER	pAdapter,
	IN		u1Byte		bank,
	IN		BOOLEAN		bPseudoTest
	);
BOOLEAN
EFUSE_ShadowUpdateBT(
	IN		PADAPTER	pAdapter,
	IN		BOOLEAN		bPseudoTest
	);
u1Byte
EFUSE_GetFakeEfusebank(
	IN	PADAPTER	Adapter
	);
VOID
EFUSE_SetFakeEfusebank(
	IN	PADAPTER	Adapter,
	IN	u1Byte		bank
	);

VOID
Hal_EEValueCheck(
	IN		u1Byte		EEType,
	IN		PVOID		pInValue,
	OUT		PVOID		pOutValue
	);

/*--------------------------Exported Function prototype---------------------*/

#endif

