#ifndef __INC_TRANSMIT_H
#define __INC_TRANSMIT_H
/*---------------------------Define---------------------------------------------*/

//
// <Roger_Notes> We using different FW fragment size for PCIe and Usb interface respectively. 
// 2009.05.05.
//
	#define		RT_LOCAL_FW_BUF_SIZE				0x8000 // size 32k

	#if TX_AGGREGATION
		#define MAX_TX_AGG_PKT						15
	#endif

#define	PACKET_NORMAL			0
#define	PACKET_DHCP				1
#define	PACKET_ARP				2
#define	PACKET_EAPOL			3


#define	MAX_FIRMWARE_CODE_SIZE	0x40000//0x5000 // Firmware Local buffer size. 92C should not use the buffer.

#define UNSPECIFIED_QUEUE_ID				0xff
#define UNSPECIFIED_DATA_RATE				0xffff

#define RT_TCB_FLAG_USE_COALESCE_BUFFER		BIT0	// Set if coalesce is turned ON
#define	RT_TCB_FLAG_USE_PACKET_SHIFT		BIT1
#define	RT_TCB_FLAG_PS_REPLY_PS_POLL		BIT2	// This packet is sent from replying the PS-Poll sent by the PS mode client.
#define	RT_TCB_FLAG_PS_MCAST_DTIM			BIT3	// DTIM reaches, send the multicast packet.
#define	RT_TCB_FLAG_PS_PRE_TRANSMIT			BIT4	// Means that we set it in PS queue before PreTransmitTCB(), we should send it in PreTransmitTCB().
#define	RT_TCB_FLAG_RAW_DATA				BIT5	// Sent by raw data. Do not change the content.

#define NIC_SEND_HANG_THRESHOLD_NORMAL		4        
#define NIC_SEND_HANG_THRESHOLD_POWERSAVE 	8
#define	MAX_DOZE_WAITING_TIMES				100		// The time of micro-second * 10 to wait before config RF OFF/Sleep.

#define TCB_RESOURCE_THRESHOLD			10

// To prevent too many data packets exhaust all TCB resource which blocks the Beacon or Mgnt packets sent,
// reserve some TCBs for important (MGNT) frames.
// By Bruce, 2013-04-10.
#define		RT_RESERVED_TCB_NUM_FOR_MGNT	2


//
// Note:
//	Reserve 1 tx desc to avoid tx stuck under the test "sendrecvstress_cmn" in Vista DTM WLK1.3.
//	We  still have no ideal why the HW DMA apears stuck frequently if the num of descs is exhaused with
//	all of the own bits set; however, once 1 desc is reserved, the HW never gets stuck anymore.
//	W-MAC also has the same trick to keep one buffer desc from the discussion with SD1 Jone.
//	This issue only happens on the PCI interface cards.
//	By Bruce, 2009-05-06.
//
#define VacancyTxDescNum(_Adapter, _Queue)				\
		((_Adapter)->NumTxDesc[_Queue] - (_Adapter)->nBufInTxDesc[_Queue] - 1)

#define IsThereBufferInTxDesc(_Adapter, _Queue)			\
		((_Adapter)->nBufInTxDesc[_Queue] > 0)
		
#define IncrementTxDescToFill(_Adapter, _Queue, _N)		\
		(_Adapter)->NextTxDescToFill[_Queue]=((_Adapter)->NextTxDescToFill[_Queue]+_N)%(_Adapter)->NumTxDesc[_Queue];			\
		(_Adapter)->nBufInTxDesc[_Queue]+=_N;

#define IncrementTxDescToCheck(_Adapter, _Queue, _N)	\
		(_Adapter)->NextTxDescToCheck[_Queue]=((_Adapter)->NextTxDescToCheck[_Queue]+_N)%(_Adapter)->NumTxDesc[_Queue];	\
		(_Adapter)->nBufInTxDesc[_Queue] -=_N;	

// TODO: Modify this !!
#define FRAGMENT_THRESHOLD(_Adapter)		((u2Byte)((_Adapter)->MgntInfo.FragThreshold-4))

#define IS_DRVAGGR_TCB(pTcb)	(pTcb->bDrvAggrFirstPacket || pTcb->bDrvAggrSubframe)

pu1Byte
GET_FRAME_OF_FIRST_FRAG(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	);

pu1Byte
DOT11_HEADER_FROM_TCB(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
);

VOID
GetTcbDestaddr(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	pu1Byte     *    	ppDestaddr
);

VOID
InitializeTxVariables(
	PADAPTER			Adapter
	);

RT_STATUS
PrepareTCBs(
	PADAPTER			Adapter
	);

VOID
FreeTCBs(
	PADAPTER			Adapter,
	BOOLEAN				bReset
	);

VOID
RemoveZeroLengthBuffer(
	PRT_TCB				pTcb
	);

u1Byte
TcbGetTOSField(
	PRT_TCB		pTcb,
	u1Byte		SkipBuffers
	);

BOOLEAN
RetrieveSegmentDataFromTCB(
	PRT_TCB				pTcb,
	u1Byte				SkipBuffers,
	u4Byte				DataOffset,
	PVOID				pDataBuffer,
	u4Byte				DataBufLength
	);

VOID
TranslateHeader(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	);

VOID
FillPartialHeader(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	);

BOOLEAN
PreTransmitTCB(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	);

BOOLEAN
TxCheckCoalesce(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	);

BOOLEAN
TxDoCoalesce(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	);

BOOLEAN
CheckFragment(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	int					nHdrOffset
	);

VOID
FragmentTCB(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	);

RT_STATUS
TransmitTCB(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	);

VOID
ResetTxStatistics(
	PADAPTER	Adapter
	);

VOID
CountTxStatistics(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	);

RESET_TYPE
TxCheckStuck(
	IN	PADAPTER		Adapter
	);

VOID
ReturnTCB(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	RT_STATUS			status
	);

PRT_TX_LOCAL_BUFFER
GetLocalFWBuffer(
	PADAPTER			Adapter
	);

PRT_TX_LOCAL_BUFFER
GetLocalBuffer(
	PADAPTER			Adapter
	);

VOID
ReturnLocalFWBuffer(
	PADAPTER			Adapter,
	PRT_TX_LOCAL_BUFFER	pLocalBuffer
	);

VOID
ReturnLocalBuffer(
	PADAPTER			Adapter,
	PRT_TX_LOCAL_BUFFER	pLocalBuffer
	);

BOOLEAN
WaitTxBusyQueueComplete(
	PADAPTER			Adapter
	);

VOID
ReleaseDataFrameQueued(
	IN	PADAPTER	pAdapter
	);

BOOLEAN
UpdateEarlyModeInfo(
	IN	PADAPTER			Adapter,
	IN	PRT_TCB				pTcb,
	IN	PRT_LIST_ENTRY	plistHead
	);


BOOLEAN
TCB_CopySystemPacketToLocal(
	IN		PADAPTER	pAdapter,
	IN OUT	PRT_TCB		pTcb
	);

VOID
TX_InitThreads(
	IN	PADAPTER	Adapter
	);
VOID
TX_DeInitThreads(
	IN	PADAPTER	Adapter
	);
VOID
TX_InitializeVariables(
	IN	PADAPTER	Adapter
	);
VOID
TX_InitializeSpinlock(
	IN	PADAPTER	Adapter
	);
VOID
TX_FreeSpinlock(
	IN	PADAPTER	Adapter
	);

#endif // #ifndef __INC_TRANSMIT_H
