#ifndef __INC_TSGEN_H
#define __INC_TSGEN_H

// These definition is used for Rx packet reordering.
#define SN_LESS(a, b)		(((a-b)&0x800)!=0)
#define SN_EQUAL(a, b)	(a == b)

// Note :
//		Window size must < REORDER_ENTRY_NUM

#define REORDER_WIN_SIZE		RX_REORDER_ENTRY_NUM		//128
#define REORDER_ENTRY_NUM		RX_REORDER_ENTRY_NUM		//tynli_test. ori value = 128
#define REORDER_WAIT_TIME	10 // (ms)

VOID
TSInitialize(
	PADAPTER	Adapter
	);

VOID
ReleaseAllTSTimer(
	PADAPTER	Adapter
	);

BOOLEAN
GetTs(
	PADAPTER			Adapter,
	PTS_COMMON_INFO	*ppTS,
	pu1Byte				Addr,
	u1Byte				TID,
	TR_SELECT			TxRxSelect,  //Rx:1, Tx:0
	BOOLEAN				bAddNewTs
	);

VOID
RemovePeerTS(
	PADAPTER	Adapter,
	pu1Byte		Addr
	);

VOID
RemoveAllTS(
	PADAPTER	Adapter
	);

VOID
TsStartAddBaProcess(
	PADAPTER		Adapter,
	PTX_TS_RECORD	pTxTS
	);

VOID
RxTSIndicate(
	PADAPTER		Adapter,
	PRT_RFD			pRfd
	);

#endif

