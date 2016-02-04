#ifndef __INC_HALCOMDESC_H
#define __INC_HALCOMDESC_H
/*++
Copyright (c) Realtek Semiconductor Corp. All rights reserved.

Module Name:
	HalComDesc.h
	
Abstract:
	Defined 92C/88E common Descriptor structure.
	    
Major Change History:
	When       Who               What
	---------- ---------------   -------------------------------
	2011-12-14 Isaiah            Create.	
--*/

/*--------------------------Define Parameters-------------------------------*/
    
/*------------------------------Define structure----------------------------*/ 


/*------------------------------ Tx Desc definition Macro ------------------------*/ 
//#pragma mark -- Tx Desc related definition. --
//----------------------------------------------------------------------------
//-----------------------------------------------------------
//	Rate
//-----------------------------------------------------------
#define DESC92C_RATEMCS15_SG			0x1c
#define DESC92C_RATEMCS32				0x20

#if 0
// CCK Rates, TxHT = 0
#define DESC_RATE1M				0x00
#define DESC_RATE2M				0x01
#define DESC_RATE5_5M			0x02
#define DESC_RATE11M			0x03
// OFDM Rates, TxHT = 0
#define DESC_RATE6M				0x04
#define DESC_RATE9M				0x05
#define DESC_RATE12M			0x06
#define DESC_RATE18M			0x07
#define DESC_RATE24M			0x08
#define DESC_RATE36M			0x09
#define DESC_RATE48M			0x0A
#define DESC_RATE54M			0x0B
// MCS Rates, TxHT = 1
#define DESC_RATEMCS0			0x0C
#define DESC_RATEMCS1			0x0D
#define DESC_RATEMCS2			0x0E
#define DESC_RATEMCS3			0x0F
#define DESC_RATEMCS4			0x10
#define DESC_RATEMCS5			0x11
#define DESC_RATEMCS6			0x12
#define DESC_RATEMCS7			0x13
#define DESC_RATEMCS8			0x14
#define DESC_RATEMCS9			0x15
#define DESC_RATEMCS10			0x16
#define DESC_RATEMCS11			0x17
#define DESC_RATEMCS12			0x18
#define DESC_RATEMCS13			0x19
#define DESC_RATEMCS14			0x1A
#define DESC_RATEMCS15			0x1B
#define DESC_RATEMCS16			0x1C
#define DESC_RATEMCS17			0x1D
#define DESC_RATEMCS18			0x1E
#define DESC_RATEMCS19			0x1F
#define DESC_RATEMCS20			0x20
#define DESC_RATEMCS21			0x21
#define DESC_RATEMCS22			0x22
#define DESC_RATEMCS23			0x23
#define DESC_RATEMCS24			0x24
#define DESC_RATEMCS25			0x25
#define DESC_RATEMCS26			0x26
#define DESC_RATEMCS27			0x27
#define DESC_RATEMCS28			0x28
#define DESC_RATEMCS29			0x29
#define DESC_RATEMCS30			0x2A
#define DESC_RATEMCS31			0x2B
#define DESC_RATEVHTSS1MCS0		0x2C
#define DESC_RATEVHTSS1MCS1		0x2D
#define DESC_RATEVHTSS1MCS2		0x2E
#define DESC_RATEVHTSS1MCS3		0x2F
#define DESC_RATEVHTSS1MCS4		0x30
#define DESC_RATEVHTSS1MCS5		0x31
#define DESC_RATEVHTSS1MCS6		0x32
#define DESC_RATEVHTSS1MCS7		0x33
#define DESC_RATEVHTSS1MCS8		0x34
#define DESC_RATEVHTSS1MCS9		0x35
#define DESC_RATEVHTSS2MCS0		0x36
#define DESC_RATEVHTSS2MCS1		0x37
#define DESC_RATEVHTSS2MCS2		0x38
#define DESC_RATEVHTSS2MCS3		0x39
#define DESC_RATEVHTSS2MCS4		0x3A
#define DESC_RATEVHTSS2MCS5		0x3B
#define DESC_RATEVHTSS2MCS6		0x3C
#define DESC_RATEVHTSS2MCS7		0x3D
#define DESC_RATEVHTSS2MCS8		0x3E
#define DESC_RATEVHTSS2MCS9		0x3F
#define DESC_RATEVHTSS3MCS0		0x40
#define DESC_RATEVHTSS3MCS1		0x41
#define DESC_RATEVHTSS3MCS2		0x42
#define DESC_RATEVHTSS3MCS3		0x43
#define DESC_RATEVHTSS3MCS4		0x44
#define DESC_RATEVHTSS3MCS5		0x45
#define DESC_RATEVHTSS3MCS6		0x46
#define DESC_RATEVHTSS3MCS7		0x47
#define DESC_RATEVHTSS3MCS8		0x48
#define DESC_RATEVHTSS3MCS9		0x49
#define DESC_RATEVHTSS4MCS0		0x4A
#define DESC_RATEVHTSS4MCS1		0x4B
#define DESC_RATEVHTSS4MCS2		0x4C
#define DESC_RATEVHTSS4MCS3		0x4D
#define DESC_RATEVHTSS4MCS4		0x4E
#define DESC_RATEVHTSS4MCS5		0x4F
#define DESC_RATEVHTSS4MCS6		0x50
#define DESC_RATEVHTSS4MCS7		0x51
#define DESC_RATEVHTSS4MCS8		0x52
#define DESC_RATEVHTSS4MCS9		0x53
#endif
typedef enum _DESC_RATE_{
	// CCK Rates, TxHT = 0
	DESC_RATE1M 	=		0x00,
	DESC_RATE2M 	=		0x01,
	DESC_RATE5_5M		=	0x02,		
	DESC_RATE11M		=	0x03,		
	// OFDM Rates, TxHT = 0
	DESC_RATE6M 	=		0x04,
	DESC_RATE9M 	=		0x05,
	DESC_RATE12M		=	0x06,		
	DESC_RATE18M		=	0x07,		
	DESC_RATE24M		=	0x08,		
	DESC_RATE36M		=	0x09,		
	DESC_RATE48M		=	0x0A,		
	DESC_RATE54M		=	0x0B,		
	// MCS Rates, TxHT = 1
	DESC_RATEMCS0		=	0x0C,		
	DESC_RATEMCS1		=	0x0D,		
	DESC_RATEMCS2		=	0x0E,		
	DESC_RATEMCS3		=	0x0F,		
	DESC_RATEMCS4		=	0x10,		
	DESC_RATEMCS5		=	0x11,		
	DESC_RATEMCS6		=	0x12,		
	DESC_RATEMCS7		=	0x13,		
	DESC_RATEMCS8		=	0x14,		
	DESC_RATEMCS9		=	0x15,		
	DESC_RATEMCS10		=	0x16,		
	DESC_RATEMCS11		=	0x17,		
	DESC_RATEMCS12		=	0x18,		
	DESC_RATEMCS13		=	0x19,		
	DESC_RATEMCS14		=	0x1A,		
	DESC_RATEMCS15		=	0x1B,		
	DESC_RATEMCS16		=	0x1C,		
	DESC_RATEMCS17		=	0x1D,		
	DESC_RATEMCS18		=	0x1E,		
	DESC_RATEMCS19		=	0x1F,		
	DESC_RATEMCS20		=	0x20,		
	DESC_RATEMCS21		=	0x21,		
	DESC_RATEMCS22		=	0x22,		
	DESC_RATEMCS23		=	0x23,		
	DESC_RATEMCS24		=	0x24,		
	DESC_RATEMCS25		=	0x25,		
	DESC_RATEMCS26		=	0x26,		
	DESC_RATEMCS27		=	0x27,		
	DESC_RATEMCS28		=	0x28,		
	DESC_RATEMCS29		=	0x29,		
	DESC_RATEMCS30		=	0x2A,		
	DESC_RATEMCS31		=	0x2B,
	// VHT MCS Rates, 
	DESC_RATEVHTSS1MCS0 	=	0x2C,		
	DESC_RATEVHTSS1MCS1	=	0x2D,		
	DESC_RATEVHTSS1MCS2 	=	0x2E,		
	DESC_RATEVHTSS1MCS3 	=	0x2F,		
	DESC_RATEVHTSS1MCS4 	=	0x30,		
	DESC_RATEVHTSS1MCS5 	=	0x31,		
	DESC_RATEVHTSS1MCS6 	=	0x32,		
	DESC_RATEVHTSS1MCS7 	=	0x33,		
	DESC_RATEVHTSS1MCS8 	=	0x34,		
	DESC_RATEVHTSS1MCS9 	=	0x35,		
	DESC_RATEVHTSS2MCS0 	=	0x36,		
	DESC_RATEVHTSS2MCS1 	=	0x37,		
	DESC_RATEVHTSS2MCS2 	=	0x38,		
	DESC_RATEVHTSS2MCS3 	=	0x39,		
	DESC_RATEVHTSS2MCS4 	=	0x3A,		
	DESC_RATEVHTSS2MCS5 	=	0x3B,		
	DESC_RATEVHTSS2MCS6 	=	0x3C,		
	DESC_RATEVHTSS2MCS7 	=	0x3D,		
	DESC_RATEVHTSS2MCS8 	=	0x3E,		
	DESC_RATEVHTSS2MCS9 	=	0x3F,		
	DESC_RATEVHTSS3MCS0 	=	0x40,		
	DESC_RATEVHTSS3MCS1 	=	0x41,		
	DESC_RATEVHTSS3MCS2 	=	0x42,		
	DESC_RATEVHTSS3MCS3 	=	0x43,		
	DESC_RATEVHTSS3MCS4 	=	0x44,		
	DESC_RATEVHTSS3MCS5 	=	0x45,		
	DESC_RATEVHTSS3MCS6 	=	0x46,		
	DESC_RATEVHTSS3MCS7 	=	0x47,		
	DESC_RATEVHTSS3MCS8 	=	0x48,		
	DESC_RATEVHTSS3MCS9 	=	0x49,		
	DESC_RATEVHTSS4MCS0 	=	0x4A,		
	DESC_RATEVHTSS4MCS1 	=	0x4B,		
	DESC_RATEVHTSS4MCS2 	=	0x4C,		
	DESC_RATEVHTSS4MCS3 	=	0x4D,		
	DESC_RATEVHTSS4MCS4 	=	0x4E,		
	DESC_RATEVHTSS4MCS5 	=	0x4F,		
	DESC_RATEVHTSS4MCS6 	=	0x50,		
	DESC_RATEVHTSS4MCS7 	=	0x51,		
	DESC_RATEVHTSS4MCS8 	=	0x52,		
	DESC_RATEVHTSS4MCS9 	=	0x53,		
	DESC_RATEMAX
}DESC_RATE_E, *PDESC_RATE_E;

#define RX_DRV_INFO_SIZE_UNIT				8

#define TX_DESC_SIZE_8192ES		40
#define TX_DESC_SIZE_8723DS 	40
#define TX_DESC_SIZE_8814AS		40
#define TX_DESC_SIZE_8812		40
#define TX_DESC_SIZE_8723B		40
#define TX_DESC_SIZE_8703B		40
#define TX_DESC_SIZE_8188F		40
#define TX_DESC_SIZE			32
#define TX_DESC_SIZE_8821BS 	40
#define TX_DESC_SIZE_8822BS		48
#define TX_INFO_SIZE_8822B		TX_DESC_SIZE_8822BS

/*------------------------------  Tx Desc field Macro ----------------------------*/ 
//#pragma mark -- Tx Desc field. --
//----------------------------------------------------------------------------

//Early mode
#define SET_EARLYMODE_PKTNUM(__pAddr, __Value)					SET_BITS_TO_LE_4BYTE(__pAddr, 0, 4, __Value)
#define SET_EARLYMODE_LEN0(__pAddr, __Value) 						SET_BITS_TO_LE_4BYTE(__pAddr, 4, 12, __Value)
#define SET_EARLYMODE_LEN1(__pAddr, __Value) 						SET_BITS_TO_LE_4BYTE(__pAddr, 16, 12, __Value)
#define SET_EARLYMODE_LEN2_1(__pAddr, __Value) 					SET_BITS_TO_LE_4BYTE(__pAddr, 28, 4, __Value)
#define SET_EARLYMODE_LEN2_2(__pAddr, __Value) 					SET_BITS_TO_LE_4BYTE(__pAddr+4, 0, 8, __Value)
#define SET_EARLYMODE_LEN3(__pAddr, __Value) 						SET_BITS_TO_LE_4BYTE(__pAddr+4, 8, 12, __Value)
#define SET_EARLYMODE_LEN4(__pAddr, __Value) 						SET_BITS_TO_LE_4BYTE(__pAddr+4, 20, 12, __Value)


#define GET_TX_REPORT_TYPE1_RERTY_0(__pAddr)						LE_BITS_TO_4BYTE( __pAddr, 0, 16)
#define GET_TX_REPORT_TYPE1_RERTY_1(__pAddr)						LE_BITS_TO_1BYTE( __pAddr+2, 0, 8)
#define GET_TX_REPORT_TYPE1_RERTY_2(__pAddr)						LE_BITS_TO_1BYTE( __pAddr+3, 0, 8)
#define GET_TX_REPORT_TYPE1_RERTY_3(__pAddr)						LE_BITS_TO_1BYTE( __pAddr+4, 0, 8)
#define GET_TX_REPORT_TYPE1_RERTY_4(__pAddr)						LE_BITS_TO_1BYTE( __pAddr+4+1, 0, 8)
#define GET_TX_REPORT_TYPE1_DROP_0(__pAddr)						LE_BITS_TO_1BYTE( __pAddr+4+2, 0, 8)
#define GET_TX_REPORT_TYPE1_DROP_1(__pAddr)						LE_BITS_TO_1BYTE( __pAddr+4+3, 0, 8)



/*------------------------------Tx Desc definition----------------------------*/ 
#pragma pack (1)
typedef struct _Phy_CCK_Rx_Status_Report
{
	/* For CCK rate descriptor. This is a unsigned 8:1 variable. LSB bit presend
	   0.5. And MSB 7 bts presend a signed value. Range from -64~+63.5. */
	u1Byte	adc_pwdb_X[4];
	u1Byte	SQ_rpt;	
	u1Byte	cck_agc_rpt;
	u1Byte	cck_bb_pwr;
}PHY_STS_CCK_T;
#pragma pack ()

/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/

/*------------------------Export Marco Definition---------------------------*/

/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Tx Descriptor related function---------------------*/

u1Byte
MTxBFPacketTypeToSNDPktSel(	
	IN	u1Byte			Type
	);


u1Byte
HwRateToMRate(
	IN 	u1Byte		rate
);

u1Byte
MRateToHwRate(	
	IN	u1Byte			rate
);

u1Byte
HwRateToMRate_2(
	IN 	u1Byte		rate
	);

u1Byte
MRateToRateIndex(	
	IN	u1Byte			rate
	);


u1Byte
QueryIsShort(
	IN	PADAPTER	Adapter,
	IN	u1Byte		TxHT,
	IN	u1Byte		TxRate,	
	IN	PRT_TCB		pTcb
);

u1Byte
MapHwQueueToFirmwareQueue(	
	IN	PRT_TCB		pTcb, 
	IN	u1Byte		QueueID
);
	


u1Byte
QueueMapping(
	u1Byte 	QueueID
	);

#if TX_AGGREGATION 	
VOID
TxSdioAggregation(
	IN	PADAPTER		Adapter,
	IN	u1Byte			queueId
	);
#endif
	


/*--------------------------Rx Descriptor related function---------------------*/



#define RX_HAL_IS_CCK_RATE(_Adapter, _pDesc)	\
	IS_HARDWARE_TYPE_8188E(_Adapter) ? RX_HAL_IS_CCK_RATE_88E(_pDesc) : RX_HAL_IS_CCK_RATE_88E(_pDesc)

#define MAX_RX_PKT_HW_VALID_SIZE(_Adapter)	(_Adapter->MAX_RECEIVE_BUFFER_SIZE)


u1Byte
QueryRxPwrPercentage(
	IN	s1Byte	AntPower
);

VOID
UpdateRxSignalStatistics(
	IN	PADAPTER	Adapter,
	IN OUT	PRT_RFD		pRfd
);

s4Byte 
TranslateToDbm(
	IN	PADAPTER	Adapter,
	IN	u1Byte		SignalStrengthIndex	// 0-100 index.
);

u1Byte 
GetSignalQuality(
	IN		PADAPTER	Adapter
);

s4Byte
SignalScaleMapping(
	IN	PADAPTER	Adapter,
	IN	s4Byte		CurrSig
);

u1Byte
SignalScaleProc(
	IN		PADAPTER	Adapter,
	IN		s4Byte 		CurrSig,
	IN		BOOLEAN		Is11n,
	IN		BOOLEAN		IsCCK
);

VOID
RxDesc_BackupScanList(
	IN		PADAPTER	Adapter
);

VOID
RxDesc_FlushScanList(
	IN		PADAPTER	Adapter
);

u1Byte 
EVMdbToPercentage(
	IN	s1Byte		Value
);

VOID
Process_UI_RSSI(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
);

VOID
Process_PWDB(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd,
	IN	PRT_WLAN_STA	pEntry
);

VOID
Process_UiLinkQuality(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
);

void
ProcessPhyInfo(
	IN	PADAPTER	Adapter,
	IN	OCTET_STRING	pduOS,
	IN	PRT_RFD		pRfd,
	IN	PRT_WLAN_STA	pEntry
);

VOID
UpdateRxAMPDUHistogramStatistics(
	IN OUT	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
);

VOID
UpdateRxPktTimeStamp(
	IN	PADAPTER	Adapter,
	IN OUT	PRT_RFD		pRfd
);

VOID
UpdateRxdRateHistogramStatistics(
	IN OUT	PADAPTER				Adapter,
	IN		PRT_RFD					pRfd,
	IN		u1Byte					mgnRate
	);

VOID
CheckRxPacketValidLength(
	IN		PADAPTER		Adapter,
	IN OUT	PRT_RFD			pRfd
	);

#endif //__INC_HALCOMDESC_H
