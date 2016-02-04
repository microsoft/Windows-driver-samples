#ifndef __INC_TCP_H
#define __INC_TCP_H

#if TCPREORDER_SUPPORT

#define TX_TCP_SEQ_CHECK	FALSE
#define RX_TCP_SEQ_CHECK	FALSE

#define TCP_SEQ_CHECK_THRESHOLD	100
#define MAX_HOLD_TCP_TIME		10	//ms
#define MAX_TCP_INTERVAL		50	//ms


#define MAX_TCP_BURST_NUM	128
#define TCP_CONN_CHECK_NUM	8
#define AVAILABLE_TCP_ENTRY	256

typedef struct _TCP_SEQ_REORDER_ENTRY{
	RT_LIST_ENTRY	List;
	u2Byte			nTcpSrcPortNum;
	u2Byte			nTcpDstPortNum;
	u2Byte			nTcpLength;
	u4Byte			nTcpSeqNum;
	u1Byte			nTcpConnIdx;
	PRT_TCB			pTcb;
}TCP_SEQ_REORDER_ENTRY, *PTCP_SEQ_REORDER_ENTRY;

typedef struct _TCP_CONN_ENTRY{
	u2Byte		nTcpSrcPortNum;
	u2Byte		nTcpDestPortNum;
	u4Byte		nLastTCPSeq;
	u4Byte		nNextTCPSeq;		// expected TCP sequence to send/recv
	u4Byte		nMinSkipTcpSeq;
	u4Byte		nMaxTcpSeq;		// out-of-order TCP sequence
	u4Byte		nMaxTcpLength;
	u2Byte		nBlockNum;
	u8Byte		usLastArriveTimeStamp;
} TCP_CONN_ENTRY, *PTCP_CONN_ENTRY;

typedef struct _TCP_SEQ_CHECKER{
	u4Byte			nTcpAbnormalCnt;
	TCP_CONN_ENTRY TcpConnection[TCP_CONN_CHECK_NUM];
} TCP_SEQ_CHECKER, *PTCP_SEQ_CHECKER;

VOID
TxTcpSeqCheck(
	PADAPTER	Adapter,
	PRT_TCB		pTcb,
	u1Byte		SkipBuffers
	);

VOID
RxTcpSeqCheck(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
TcpReorder_Init(
	PADAPTER	Adapter
	);

VOID
TcpReorder_Release(
	PADAPTER	Adapter
	);

VOID
TcpReorder_Send(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	);

#else
#define TcpReorder_Init(_Adapter)
#define TcpReorder_Release(_Adapter)
#define TcpReorder_Send(_Adapter,_Tcb)
#endif

#endif