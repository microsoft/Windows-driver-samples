#ifndef __INC_TRANSMITDESC_H
#define __INC_TRANSMITDESC_H

VOID
MgntQuery_ProtectionFrame(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	);

VOID
MgntQuery_PreambleMode(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	);

VOID
MgntQuery_TxTime(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	);

VOID
MgntQuery_AggregationCapability(
	PADAPTER	Adapter,
	pu1Byte		dstaddr,
	PRT_TCB		pTcb
	);


BOOLEAN 
MgntQuery_RA_ShortGI(	
	IN	PADAPTER			Adapter,
	IN	u1Byte				macId,
	IN	PRT_WLAN_STA		pEntry,
	IN	WIRELESS_MODE		WirelessMode,
	IN	CHANNEL_WIDTH		ChnlBW
);


VOID	
MgntQuery_ShortGI(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	);


VOID
MgntQuery_Tx_LDPC(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	);


VOID
MgntSet_TX_LDPC(
	PADAPTER		Adapter,
	u1Byte			MacId,
	BOOLEAN			bLDPC
	);


VOID
MgntQuery_Tx_STBC(
	IN		PADAPTER	Adapter,
	IN OUT	PRT_TCB		pTcb
	);


VOID
MgntQuery_BandwidthMode(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	);


VOID
MgntSet_RA_Ratr_Index(
	IN	PMGNT_INFO			pMgntInfo,	
	IN	MAC_ID_OWNER_TYPE	MacIdOwnerType,
	IN	u1Byte				ratr_index,
	IN	PRT_WLAN_STA		pEntry
	);


VOID
MgntQuery_TxRateSelectMode(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	);


VOID
FillFrameField(
	PADAPTER	Adapter,
	PRT_TCB		pTcb,
	u2Byte		SeqNum
	);

#endif
