#ifndef __INC_AMSDU_AGGREGATION_H
#define __INC_AMSDU_AGGREGATION_H

// A-MSDU Aggregation Tx Desc threshold.
// If Adapter->nBufInTxDesc[n] is larger than this threshold, A-MSDU aggregation will be turned on.
#define AMSDU_THRESHOLD					1
#define	GET_AMSDU_SUB_HEADER_LENGTH(_amsduHdr)				N2H2BYTE(*((UNALIGNED pu2Byte)(((pu1Byte)(_amsduHdr)) + ETHERNET_ADDRESS_LENGTH*2)))
#define	SET_AMSDU_SUB_HEADER_LENGTH(_amsduHdr, _val)			((*(UNALIGNED pu2Byte)(((pu1Byte)(_amsduHdr)) + ETHERNET_ADDRESS_LENGTH*2))=H2N2BYTE(_val))

PRT_TCB
AMSDU_Aggregation(
	PADAPTER			Adapter,
	PRT_LIST_ENTRY		pSendList
	);

u1Byte
AMSDU_GetAggregatibleList(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	PRT_LIST_ENTRY		pSendList
	);

BOOLEAN
AMSDUTransmitTCB(
	PADAPTER			Adapter,
	PRT_TCB				*ppTcb,
	pu1Byte 				pHeader
	);

VOID
AMSDU_UpdateForcedValueByReg(
	PADAPTER			Adapter
	);

VOID
AMSDU_QuerySizeAndNum(
	PADAPTER			Adapter,
	pu2Byte				nMaxAMSDUSize,
	pu1Byte				nMaxAMSDUSizeNum
	);

VOID
AMSDU_UpdateRxAMSDUSizeHistogram(
	PADAPTER			Adapter,
	u2Byte				PacketLength	
	);

VOID
AMSDU_UpdateRxAMSDUNumHistogram(
	PADAPTER			Adapter,
	u2Byte				totalSubframe	
	);

VOID
AMSDU_ClearTRxAMSDUHistogram(
	PADAPTER			Adapter
	);

VOID
AMSDU_UpdateTxAMSDUSizeHistogram(
	PADAPTER			Adapter,
	u2Byte				PacketLength	
	);

VOID
AMSDU_UpdateTxAMSDUNumHistogram(
	PADAPTER			Adapter,
	u1Byte				totalSubframe	
	);




#endif
