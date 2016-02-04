#ifndef __INC_RXREORDER_H
#define __INC_RXREORDER_H


#define RXTS_INDICATE_IDLE			0	// Idle, No packet needs to process.
#define RXTS_INDICATE_BATCH		1	// Batch, Waiting for other packets to do batch indication
#define RXTS_INDICATE_REORDER		2	// Reorder, Waiting for other packets because of ordering
#define RXTS_INDICATE_PROCESSING	3	// Processing, Packets are indicating now
#define RXTS_INDICATE_FAST			4	// Fast indicate in rx batch indication

#define RX_BATCH_PACKET_NUM			10

typedef enum _RX_REORDER_ACTION {  
	NOT_INDICATE,
	INDICATED,
	RX_REORDER_INDICATE
}RX_REORDER_ACTION;



VOID
IndicateRxReorderList(
	IN	PADAPTER				Adapter,
	IN	PRX_TS_RECORD			pTS,
	IN	BOOLEAN					bForced
	);

VOID
FlushRxTsPendingPkts(
	IN	PADAPTER 				Adapter,
	IN	PRX_TS_RECORD			pTS
	);

VOID
FlushAllRxTsPendingPkts(
	IN	PADAPTER 				Adapter
	);

#if RX_AGGREGATION
VOID
RxReorderAggrBatchIndicate(
	IN	PADAPTER				Adapter,
	IN	PRT_RFD					pRfd
	);

VOID
RxReorderAggrBatchFlushBuf(
	IN	PADAPTER				Adapter
	);
#endif


VOID
RxReorderIndicatePacket(
	IN	PADAPTER				Adapter,
	IN	PRT_RFD					pRfd
	);

VOID
RxPktPendingTimeout(
	PRT_TIMER	pTimer
	);

#endif