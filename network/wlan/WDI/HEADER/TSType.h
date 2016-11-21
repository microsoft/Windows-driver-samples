#ifndef __INC_TSTYPE_H
#define __INC_TSTYPE_H

#define TS_SETUP_TIMEOUT	60000  // In micro seconds
#define TS_INACT_TIMEOUT	60000
#define TS_ADDBA_DELAY		60000

#define TOTAL_TS_NUM		64
#define TCLAS_NUM			4

// This define the Tx/Rx directions
typedef enum _TR_SELECT {
	TX_DIR = 0, 
	RX_DIR = 1,
} TR_SELECT, *PTR_SELECT;

#define WMM_TSPEC_BODY_LENGTH	55
typedef struct _TS_COMMON_INFO{
	RT_LIST_ENTRY		List;
	u1Byte				Addr[6];
	u1Byte				TSpec[WMM_TSPEC_BODY_LENGTH];
	QOS_TCLAS			TClass[TCLAS_NUM];
	u1Byte				TClasProc;
	u1Byte				TClasNum;
} TS_COMMON_INFO, *PTS_COMMON_INFO;

typedef struct _TX_TS_RECORD{
	TS_COMMON_INFO		TsCommonInfo;
	u2Byte				TxCurSeq;
	BA_RECORD			TxPendingBARecord;  	// For BA Originator
	BA_RECORD			TxAdmittedBARecord;	// For BA Originator
//	QOS_DL_RECORD		DLRecord;
	BOOLEAN				bAddBaReqInProgress;
	BOOLEAN				bAddBaReqDelayed;
	BOOLEAN				bUsingBa;
	RT_TIMER			TsAddBaTimer;
} TX_TS_RECORD, *PTX_TS_RECORD;

typedef struct _RX_TS_RECORD {
	TS_COMMON_INFO		TsCommonInfo;
	u2Byte				RxIndicateSeq;
	u1Byte				RxIndicateState;
	RT_LIST_ENTRY		RxPendingPktList;
	RT_TIMER			RxPktPendingTimer;
	BA_RECORD			RxAdmittedBARecord;	 // For BA Recepient
	u2Byte				RxLastSeqNum;
	u1Byte				RxLastFragNum;
	u4Byte				RxBatchCount;
	u4Byte				RxTotalSubframeCount;  //for AMSDU subframe	
//	QOS_DL_RECORD		DLRecord;
} RX_TS_RECORD, *PRX_TS_RECORD;


#endif
