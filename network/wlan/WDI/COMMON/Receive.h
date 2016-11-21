#ifndef __INC_RECEIVE_H
#define __INC_RECEIVE_H

// Sw calculate crc for debug usage
#define 		SW_CRC_CHECK 			FALSE
#define			RX_DESC_NUM			64

	//
	// <Roger_Notes> We set Maximum Rx Buffer Size to 16K to avoid USB BABBLE error status. 2008.06.20.
	// <Roger_Notes> Revise Maximal Receive buffer size to 32K to overcome Rx FIFO overflow issue. 
	// Suggeseted by Scott, 2008.10.03.
	//
	// 2011/05/31 MH We will fine tue RX aggregation size for USB later.
	//
#define RX_AGG_BLOCK_SIZE						512			// hardware defined the size of a block, for USB mode	
#define RX_AGG_PAGE_SIZE						128			// hardware defined the size of a page, for DMA mode

#define MAX_RX_AGG_BLKCNT					60				// Maximum available RX aggregation block count

#define RX_INTERRUPT_BUFFER_SIZE_8812AU			60
#define RX_INTERRUPT_BUFFER_SIZE_8192EU			60
#define RX_INTERRUPT_BUFFER_SIZE_88EU			60
#define RX_INTERRUPT_BUFFER_SIZE_92CU			56
#define MAX_INTERRUPT_TRANSFER_PACKET_SIZE		64 // 1024Bytes for High speed, 64Bytes for Full speed.


#define IsThereVacancyRxDesc(_Adapter, _QueueID)		\
		((_Adapter)->nBufInRxDesc[_QueueID] < (_Adapter)->NumRxDesc[_QueueID])

#define IsThereBufferInRxDesc(_Adapter, _QueueID)		\
		((_Adapter)->nBufInRxDesc[_QueueID] > 0)
		
#define IncrementRxDescToFill(_Adapter, _QueueID)		\
		(_Adapter)->NextRxDescToFill[_QueueID]=((_Adapter)->NextRxDescToFill[_QueueID]+1)%(_Adapter)->NumRxDesc[_QueueID];			\
		(_Adapter)->nBufInRxDesc[_QueueID]++;

#define IncrementRxDescToCheck(_Adapter, _QueueID)	\
		(_Adapter)->NextRxDescToCheck[_QueueID]=((_Adapter)->NextRxDescToCheck[_QueueID]+1)%(_Adapter)->NumRxDesc[_QueueID];	\
		(_Adapter)->nBufInRxDesc[_QueueID]--;


#define MAKE_RFD_OFFSET_AT_FRONT(_pRfd, _offset)					\
		(_pRfd)->FragOffset+=_offset;									\
		(_pRfd)->FragLength-=_offset;									\
		(_pRfd)->PacketLength-=_offset;

#define MAKE_RFD_OFFSET_AT_BACK(_pRfd, _offset)					\
		(_pRfd)->FragLength-=_offset;									\
		(_pRfd)->PacketLength-=_offset;

#define MAKE_RFD_LIST_OFFSET_AT_FRONT(_pRfd, _offset)				\
		MAKE_RFD_OFFSET_AT_FRONT(_pRfd, _offset)

#define MAKE_RFD_LIST_OFFSET_AT_BACK(_Adapter, _pRfd, _offset)		\
		MakeRFDListOffsetAtBack(_Adapter, _pRfd, _offset)

// Records of received packet not yet returned, 2007.01.19, by shien chang.
#define RT_INC_RCV_REF(_Adapter)	((_Adapter)->RcvRefCount++)
#define RT_DEC_RCV_REF(_Adapter)	(((_Adapter)->RcvRefCount>0)?((_Adapter)->RcvRefCount--):((_Adapter)->RcvRefCount=0))
#define RT_GET_RCV_REF(_Adapter)	((_Adapter)->RcvRefCount)
#define RT_INC_N_RCV_REF(_Adapter, _n)	((_Adapter)->RcvRefCount+=_n)

#if WLAN_ETW_SUPPORT
#define RT_INC_TX_FRAME_UNIQUE_ID(_Adapter)	((GetDefaultAdapter(_Adapter))->TxFrameUniqueueID++)
#define RT_GET_TX_FRAME_UNIQUE_ID(_Adapter)	((GetDefaultAdapter(_Adapter))->TxFrameUniqueueID)
#define RT_INC_RX_FRAME_UNIQUE_ID(_Adapter)	((GetDefaultAdapter(_Adapter))->RxFrameUniqueueID++)
#define RT_GET_RX_FRAME_UNIQUE_ID(_Adapter)	((GetDefaultAdapter(_Adapter))->RxFrameUniqueueID)
#endif

VOID
InitializeRxVariables(
	PADAPTER	Adapter
	);

VOID
DeInitializeRxVariables(
	PADAPTER	Adapter
	);

BOOLEAN
ChkValidRFDs(
	PADAPTER	Adapter,
	pu4Byte		pRfdAddr
	);


BOOLEAN
ChkValidVAs(
	PADAPTER	Adapter,
	pu4Byte		pRfdAddr
	);
	
RT_STATUS
PrepareRFDs(
	PADAPTER	Adapter
	);

VOID
FreeRFDs(
	PADAPTER	Adapter,
	BOOLEAN		bReset
	);

VOID
PrepareAllRxDescBuffer(
	PADAPTER	Adapter
	);

VOID
PrepareOneRxDescBuffer8814AE(
	PADAPTER	Adapter
	);

VOID 
SpareRxDesc(
	PADAPTER	Adapter
	);

VOID 
ResetRxStatistics(
	PADAPTER	Adapter
	);

VOID 
CountRxFragmentStatistics(
	PADAPTER Adapter
	);

VOID 
CountRxStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);


VOID 
CountRxErrStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
CountRxExcludedUnencryptedStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
CountRxTKIPLocalMICFailuresStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
CountRxMgntTKIPLocalMICFailuresStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

VOID
CountRxTKIPDecryptErrorsStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
CountRxCCMPDecryptErrorsStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
CountWEPICVErrorStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
CountRxTKIPRelpayStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

VOID
CountRxMgntTKIPRelpayStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

VOID
CountRxMgntCCMPRelpayStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

VOID
CountRxCCMPRelpayStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

VOID
CountRxMgntTKIPNoEncryptStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD 	pRfd
	);

VOID
CountRxMgntCCMPNoEncryptStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD 	pRfd
	);

VOID
CountRxMgntMFPTKIPMHDRStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD 	pRfd
	);

VOID
CountRxMgntTKIPDecryptErrorsStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

VOID
CountRxMgntCCMPDecryptErrorsStatistics(
	IN	PADAPTER	Adapter,
	IN	PRT_RFD		pRfd
	);

VOID
RxStatisticsWatchdog(
	IN	PADAPTER	Adapter
	);

VOID
CountTKIPICVErrorStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);


VOID
CountRxDecryptSuccessStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);


VOID
CountRxDecryptErrorStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
CountWEPUndecryptableStatistics(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

BOOLEAN
RxCheckSWDecryption(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

PADAPTER 
SelectReceiveAdapter(
	PADAPTER			Adapter,
	OCTET_STRING		frame
);

VOID
ProcessReceivedPacket(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
ProcessReceivedPacketForEachPortSpecific(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
);

BOOLEAN
DefragPacket(
	PADAPTER	Adapter,
	PRT_RFD		*ppRfd,
	PBOOLEAN	pbQueued
	);

BOOLEAN
TranslateRxPacketHeader(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

BOOLEAN
TranslateHandleRxDot11FrameHeader(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
TranslateRxLLCHeaders(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

VOID
ReturnRFDList(
	PADAPTER	pAdapter,
	PRT_RFD		pRfd
);

VOID
MakeRFDListOffsetAtBack(
	PADAPTER	Adapter,
	PRT_RFD		pRfd, 
	u2Byte		offset
	);

RESET_TYPE
RxCheckStuck(
	IN	PADAPTER		Adapter
	);

VOID
RxCheckStuckDbg(
	IN	PADAPTER		Adapter
	);

BOOLEAN
RxCheckLength(
	PADAPTER	Adapter,
	PRT_RFD		pRfd
	);

BOOLEAN
RxCheckResource(
	PADAPTER	pAdapter
	);

VOID
RxNotifyThreadCallback(
	IN	PVOID	pContext
	);


#endif // #ifndef __INC_RECEIVE_H
