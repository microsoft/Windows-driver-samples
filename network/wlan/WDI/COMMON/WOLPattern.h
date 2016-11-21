#ifndef	__INC_WOLPATTERN_H
#define	__INC_WOLPATTERN_H


//#define	MAX_SUPPORT_WOL_PATTERN_NUM		8

//#define	MAX_WOL_BIT_MASK_SIZE		16 //unit: byte
//#define	MAX_WOL_PATTERN_SIZE		128

#define		WOL_REASON_PTK_UPDATE		BIT0	// Pairwise key update event triggers this NIC to wake up
#define		WOL_REASON_GTK_UPDATE		BIT1	// Group key update event triggers this NIC to wake up
#define		WOL_REASON_DISASSOC			BIT2	// Disassociation event triggers this NIC to wake up
#define		WOL_REASON_DEAUTH			BIT3	// Deauthentication event triggers this NIC to wake up
#define		WOL_REASON_AP_LOST			BIT4	// FW decision disconnection event triggers this NIC to wake up
#define		WOL_REASON_MAGIC_PKT		BIT5
#define		WOL_REASON_UNICAST_PKT		BIT6
#define		WOL_REASON_PATTERN_PKT		BIT7
#define		WOL_REASON_RTD3_SSID_MATCH	BIT8
#define		WOL_REASON_REALWOW_V2_WAKEUPPKT	BIT9
#define		WOL_REASON_REALWOW_V2_ACKLOST		BIT10
#define		WOL_REASON_NLO_SSID_MATCH	BIT11

VOID
GetWOLWakeUpPattern(
	IN PADAPTER	pAdapter,
	IN pu1Byte	pWOLPatternMask,
	IN u4Byte		WOLPatternMaskSize,
	IN pu1Byte	pWOLPatternContent,
	IN u4Byte		WOLPatternContentSize,
	IN u1Byte		Index,
	IN BOOLEAN	bMgntFrame
	);

u2Byte
CalculateWOLPatternCRC(
	pu1Byte Pattern,
	u4Byte PatternLength
	);

u2Byte
CRC16_CCITT(
	u1Byte data,
	u2Byte CRC
	);

VOID
ResetWoLPara(
	IN		PADAPTER		Adapter 
	);

VOID
ConstructUserDefinedWakeUpPattern(
	IN		PADAPTER		Adapter
);

VOID
WolByGtkUpdate(
	IN	PADAPTER	pAdapter
	);

VOID
RemoveUserDefinedWoLPattern(
	IN		PADAPTER		Adapter
	);

VOID
AddWoLPatternEntry(
	IN PADAPTER	Adapter,
	IN pu1Byte	pWOLPatternMask,
	IN u4Byte		WOLPatternMaskSize,
	IN pu1Byte	pWOLPatternContent,
	IN u4Byte		WOLPatternContentSize,
	IN u1Byte		Index
	);

VOID
GetWoLPatternMatchOffloadEntries(
	IN PADAPTER	Adapter,
	IN u1Byte	EntryNum
	);

BOOLEAN
WoL_HandleReceivedPacket(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
WoL_ParsingRxBufferPacket(
	PADAPTER	Adapter
	);


#endif //#ifndef __INC_WOLPATTERN_H

