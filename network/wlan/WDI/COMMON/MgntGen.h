#ifndef __INC_MGNTGEN_H
#define __INC_MGNTGEN_H


/*--------------------------Define -------------------------------------------*/ 

typedef enum _MGN_RATE_{
	MGN_1M		= 0x02,
	MGN_2M		= 0x04,
	MGN_5_5M 	= 0x0B,
	MGN_6M	 	= 0x0C,
	MGN_9M		= 0x12,
	MGN_11M 	= 0x16,
	MGN_12M	= 0x18,
	MGN_18M	= 0x24,
	MGN_24M	= 0x30,
	MGN_36M	= 0x48,
	MGN_48M	= 0x60,
	MGN_54M	= 0x6C,
	MGN_MCS32	= 0x7F,
	MGN_MCS0	= 0x80,
	MGN_MCS1,
	MGN_MCS2,
	MGN_MCS3,
	MGN_MCS4,
	MGN_MCS5,
	MGN_MCS6,
	MGN_MCS7	= 0x87,
	MGN_MCS8,
	MGN_MCS9,
	MGN_MCS10,
	MGN_MCS11,
	MGN_MCS12,
	MGN_MCS13,
	MGN_MCS14,
	MGN_MCS15,
	MGN_MCS16	= 0x90,
	MGN_MCS17,
	MGN_MCS18,
	MGN_MCS19,
	MGN_MCS20,
	MGN_MCS21,
	MGN_MCS22,
	MGN_MCS23,
	MGN_MCS24 = 0x98,
	MGN_MCS25,
	MGN_MCS26,
	MGN_MCS27,
	MGN_MCS28,
	MGN_MCS29,
	MGN_MCS30,
	MGN_MCS31,
	MGN_VHT1SS_MCS0	= 0xa0,
	MGN_VHT1SS_MCS1,
	MGN_VHT1SS_MCS2,
	MGN_VHT1SS_MCS3,
	MGN_VHT1SS_MCS4,
	MGN_VHT1SS_MCS5,
	MGN_VHT1SS_MCS6,
	MGN_VHT1SS_MCS7,
	MGN_VHT1SS_MCS8,
	MGN_VHT1SS_MCS9,
	MGN_VHT2SS_MCS0 = 0xaa,
	MGN_VHT2SS_MCS1 = 0xab,
	MGN_VHT2SS_MCS2,
	MGN_VHT2SS_MCS3,
	MGN_VHT2SS_MCS4,
	MGN_VHT2SS_MCS5	= 0xaf,
	MGN_VHT2SS_MCS6	= 0xb0,
	MGN_VHT2SS_MCS7,
	MGN_VHT2SS_MCS8,
	MGN_VHT2SS_MCS9	= 0xb3,
	MGN_VHT3SS_MCS0	= 0xb4,
	MGN_VHT3SS_MCS1,
	MGN_VHT3SS_MCS2,
	MGN_VHT3SS_MCS3,
	MGN_VHT3SS_MCS4,
	MGN_VHT3SS_MCS5,
	MGN_VHT3SS_MCS6,
	MGN_VHT3SS_MCS7 = 0xbb,
	MGN_VHT3SS_MCS8 = 0xbc,
	MGN_VHT3SS_MCS9 = 0xbd,
	MGN_VHT4SS_MCS0 = 0xbe,
	MGN_VHT4SS_MCS1,
	MGN_VHT4SS_MCS2,
	MGN_VHT4SS_MCS3,
	MGN_VHT4SS_MCS4,
	MGN_VHT4SS_MCS5,
	MGN_VHT4SS_MCS6,
	MGN_VHT4SS_MCS7,
	MGN_VHT4SS_MCS8,
	MGN_VHT4SS_MCS9 = 0xc7,
	MGN_UNKNOWN
}MGN_RATE_E, *PMGN_RATE_E;


#define IS_HT_RATE(_rate)				(_rate >= MGN_MCS0 && _rate <= MGN_MCS31)
#define IS_VHT_RATE(_rate)				(_rate >= MGN_VHT1SS_MCS0 && _rate <= MGN_VHT4SS_MCS9)
#define IS_CCK_RATE(_rate) 				(MGN_1M == _rate || _rate == MGN_2M || _rate == MGN_5_5M || _rate == MGN_11M )
#define IS_OFDM_RATE(_rate)				(MGN_6M <= _rate && _rate <= MGN_54M  && _rate != MGN_11M)



#define	GEN_TEMP_BUFFER_NUM		16
#define	GEN_TEMP_BUFFER_SIZE		4096

/*--------------------------Define MACRO--------------------------------------*/

#define IsMgntFrame(pdu)		( ((EF1Byte(pdu[0]) & 0x0C)==0x00) ? TRUE : FALSE )
#define IsDataFrame(pdu)		( ((EF1Byte(pdu[0]) & 0x0C)==0x08) ? TRUE : FALSE )
#define IsCtrlFrame(pdu)		( ((EF1Byte(pdu[0]) & 0x0C)==0x04) ? TRUE : FALSE )
#define IsRetryFrame(pdu)		( ((EF1Byte(pdu[1]) & 0x08)==0x08) ? TRUE : FALSE )//add by ylb 20130808

#define	IsMgntAsocReq(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Asoc_Req ) ? TRUE : FALSE)
#define	IsMgntAsocRsp(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Asoc_Rsp ) ? TRUE : FALSE)
#define	IsMgntReAsocReq(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Reasoc_Req ) ? TRUE : FALSE)
#define	IsMgntReAsocRsp(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Reasoc_Rsp ) ? TRUE : FALSE)
#define	IsMgntProbeReq(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Probe_Req ) ? TRUE : FALSE)
#define	IsMgntProbeRsp(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Probe_Rsp ) ? TRUE : FALSE)
#define	IsMgntBeacon(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Beacon ) ? TRUE : FALSE)
#define	IsMgntAtim(pdu)			( ((EF1Byte(pdu[0]) & 0xFC) == Type_Atim ) ? TRUE : FALSE)
#define	IsMgntDisasoc(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Disasoc ) ? TRUE : FALSE)
#define	IsMgntAuth(pdu)			( ((EF1Byte(pdu[0]) & 0xFC) == Type_Auth ) ? TRUE : FALSE)
#define	IsMgntDeauth(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Deauth ) ? TRUE : FALSE)
#define	IsMgntAction(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Action ) ? TRUE : FALSE)
#define	IsMgntActionNoAck(pdu)	( ((EF1Byte(pdu[0]) & 0xFC) == Type_Action_No_Ack ) ? TRUE : FALSE)
#define	IsMgntCfend(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Cfend ) ? TRUE : FALSE)
#define	IsMgntCfendAck(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Cfend_Ack ) ? TRUE : FALSE)
#define	IsMgntData(pdu)			( ((EF1Byte(pdu[0]) & 0xFC) == Type_Data ) ? TRUE : FALSE)
#define	IsMgntDataAck(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Data_Ack ) ? TRUE : FALSE)
#define	IsMgntDataPoll(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Data_Poll ) ? TRUE : FALSE)
#define	IsMgntDataPoll_Ack(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Data_Poll_Ack ) ? TRUE : FALSE)
#define	IsMgntNullFrame(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Null_Frame ) ? TRUE : FALSE)
#define	IsMgntCfack(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Cfack ) ? TRUE : FALSE)
#define	IsMgntCfpoll(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Cfpoll ) ? TRUE : FALSE)
#define	IsMgntCfpollAck(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_Cfpoll_Ack ) ? TRUE : FALSE)
#define	IsMgntQosData(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_QosData ) ? TRUE : FALSE)
#define	IsMgntQData_Ack(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_QData_Ack ) ? TRUE : FALSE)
#define	IsMgntQData_Poll(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_QData_Poll ) ? TRUE : FALSE)
#define	IsMgntQData_Poll_Ack(pdu)	( ((EF1Byte(pdu[0]) & 0xFC) == Type_QData_Poll_Ack ) ? TRUE : FALSE)
#define	IsMgntQosNull(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_QosNull ) ? TRUE : FALSE)
#define	IsMgntQosCfpoll(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_QosCfpoll ) ? TRUE : FALSE)
#define	IsMgntQosCfpoll_Ack(pdu)	( ((EF1Byte(pdu[0]) & 0xFC) == Type_QosCfpoll_Ack ) ? TRUE : FALSE)

#define	IsCtrlPSpoll(pdu)		( ((EF1Byte(pdu[0]) & 0xFC) == Type_PS_poll ) ? TRUE : FALSE)
#define	IsCtrlAck(pdu)			( ((EF1Byte(pdu[0]) & 0xFC) == Type_Ack ) ? TRUE : FALSE)
#define	IsCtrlNDPA(pdu)			( ((EF1Byte(pdu[0]) & 0xFC) == Type_NDPA) ? TRUE : FALSE)
#define	IsCtrlBFReportPoll(pdu)			( ((EF1Byte(pdu[0]) & 0xFC) == Type_Beamforming_Report_Poll) ? TRUE : FALSE)
#define	IsCtrlBlockAckReq(pdu)			( ((EF1Byte(pdu[0]) & 0xFC) == Type_BlockAckReq) ? TRUE : FALSE)

#define	IsLegacyDataFrame(pdu)		(IsDataFrame(pdu) && (!(EF1Byte(pdu[0]) & FC_QOS_BIT)) )	// Added by Joseph, 2005-12-16.
#define	IsQoSDataFrame(pdu)		(IsDataFrame(pdu) && (EF1Byte(pdu[0]) & FC_QOS_BIT) )
#define	IsNoDataFrame(pdu)		(IsDataFrame(pdu) && (EF1Byte(pdu[0]) & FC_NO_DATA_BIT) )	// Added by Annie, 2006-01-06.

typedef enum _AUTH_STATUS_T{
	opensystem_auth = 0x00,
	sharedkey_auth = 0x01,
	ft_auth = 0x2,
	not_auth = 0x03,
}AUTH_STATUS_T, *PAUTH_STATUS_T;


typedef enum _ASOC_STATUS_T{
	disassoc = 0x00,
	assoc      = 0x01,
}ASOC_STATUS_T, *PASOC_STATUS_T;

BOOLEAN
IsMgntNDPA(
	pu1Byte		pdu
);


BOOLEAN
MgntIsRateSupport(
	u1Byte			nRate,
	OCTET_STRING	osRateSet
	);

VOID
MgntRefreshSuppRateSet(
	PADAPTER			Adapter
	);

RT_STATUS
MgntAllocMemory(
	IN PADAPTER		pAdapter
	);

VOID
MgntFreeMemory(
	IN PADAPTER		pAdapter
	);

VOID
ResetMgntVariables(
	PADAPTER			Adapter
	);

VOID
InitializeMgntVariables(
	PADAPTER			Adapter
	);


VOID
DeInitializeMgntVariables(
	PADAPTER			Adapter
	);
VOID
MgntFreeAllWorkItem(
	PADAPTER			Adapter
	);

VOID
CancelWatchDogTimer(
	PADAPTER	Adapter
	);

VOID
MgntCancelAllTimer(
	PADAPTER			Adapter
	);

VOID
MgntReleaseAllTimer(
	PADAPTER			Adapter
	);
	

BOOLEAN
MgntGetFWBuffer(
	PADAPTER			Adapter,
	PRT_TCB				*ppTcb,
	PRT_TX_LOCAL_BUFFER	*ppBuf
	);

BOOLEAN
MgntGetBuffer(
	PADAPTER			Adapter,
	PRT_TCB				*ppTcb,
	PRT_TX_LOCAL_BUFFER	*ppBuf
	);

VOID
MgntSendPacket(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	PRT_TX_LOCAL_BUFFER	pBuf,
	u4Byte				Length,
	u1Byte				QueueIndex,
	u2Byte				DataRate
	);

BOOLEAN
MgntFilterReceivedPacket(
	PADAPTER			Adapter,
	PRT_RFD				pRfd
	);

BOOLEAN
MgntCheckForwarding(
	PADAPTER			Adapter,
	PRT_RFD				pRfd
	);

BOOLEAN
MgntFilterTransmitPacket(
	PADAPTER			Adapter,
	PRT_TCB				pTcb
	);

RT_STATUS
MgntAllocateBeaconBuf(
	PADAPTER			Adapter
	);

VOID
MgntFreeBeaconBuf(
	PADAPTER			Adapter
	);

BOOLEAN 
MgntIsMacAddrGroup( 
	pu1Byte addr 
	);

VOID
MgntSsInquiry( 
	PADAPTER			Adapter,
	pu1Byte				pSaddr, 
	PAUTH_STATUS_T		state_auth, 
	PASOC_STATUS_T		state_asoc 
	);

BOOLEAN
MgntGetEncryptionInfo(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	PSTA_ENC_INFO_T		pEncInfo,
	BOOLEAN                  	bIncHead
);

u1Byte
MgntQuery_MgntFrameTxRate(
	PADAPTER		Adapter,
	pu1Byte			dstaddr
	);

u2Byte
MgntQuery_FrameTxRate(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	);


u1Byte
MgntQuery_NssTxRate(
	IN	u2Byte			Rate
	);


u2Byte
MgntQuery_SequenceNumber(
	PADAPTER		Adapter,
	pu1Byte			pFrame,
	BOOLEAN			bBTTxPacket,
	pu1Byte			pDestAddr,
	u1Byte			TID
);

BOOLEAN
MgntIsShortPreambleMode(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
);

BOOLEAN
MgntIsMulticastFrame(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
);

BOOLEAN
MgntIsBroadcastFrame(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
);

BOOLEAN
MgntIsDataOnlyFrame(
	PADAPTER		Adapter,
	PRT_TCB 		pTcb
);


BOOLEAN
MgntIsRateValidForWirelessMode(
	u1Byte	rate,		
	u1Byte	wirelessmode
);


u4Byte
MgntGetChannelFrequency(
	u4Byte		channel
);

VOID
MgntUpdateAsocInfo(
	PADAPTER					Adapter,
	UPDATE_ASOC_INFO_ACTION	Action,
	pu1Byte						Content,
	u4Byte						ContentLength
);

BOOLEAN
MgntIsInExcludedMACList(
	PADAPTER					Adapter,
	pu1Byte						pAddr
);

VOID
MgntInitSsidsToScan(
	PADAPTER					Adapter
);

BOOLEAN
MgntAddSsidsToScan(
	PADAPTER					Adapter,
	OCTET_STRING				Ssid
);

BOOLEAN
MgntRemoveSsidsToScan(
	PADAPTER					Adapter,
	OCTET_STRING				Ssid
);

VOID
MgntClearSsidsToScan(
	PADAPTER					Adapter
);

VOID
MgntEnableNetMonitorMode(
	PADAPTER					Adapter,
	BOOLEAN						bInitState
);

VOID
MgntDisableNetMonitorMode(
	PADAPTER					Adapter,
	BOOLEAN						bInitState
);

BOOLEAN 
MgntAllocHashTables(
	IN	PADAPTER		pAdapter
	);

VOID 
MgntFreeHashTables(
	IN	PADAPTER		pAdapter
	);

u1Byte
MgntQuery_TxRateExcludeCCKRates(
	IN	OCTET_STRING	BSSBasicRateSet
	);

BOOLEAN
MgntAllocatePacketParser(
	IN	PADAPTER					Adapter
	);

VOID
MgntFreePacketParser(
	IN	PADAPTER		Adapter
	);

BOOLEAN
MgntGetPwrMgntInfo(
	PADAPTER			Adapter,
	PRT_TCB				pTcb,
	BOOLEAN				bIncHead
	);

VOID
LedTimerCallback(
	PRT_TIMER		pTimer
	);


PRT_GEN_TEMP_BUFFER
GetGenTempBuffer(
	IN PADAPTER	Adapter,
	IN	u4Byte	Length
	);

VOID
ReturnGenTempBuffer(
	IN PADAPTER				Adapter,
	IN PRT_GEN_TEMP_BUFFER	pGenBuffer
	);


// Let each port (Adapter) have common context
RT_STATUS
PortCommonInfoAllocateMemoryWithCriticalInitialization(
	PADAPTER pDefaultAdapter
);

VOID
PortCommonInfoFreeMemory(
	PADAPTER pDefaultAdapter
);

BOOLEAN
MgntIsWoWLANCapabilityEnable(
	PADAPTER 	pAdapter
	);

BOOLEAN
MgntIsSupportRemoteWakeUp(
	PADAPTER 	pAdapter
	);


#if(AUTO_CHNL_SEL_NHM == 1)
VOID
MgntAutoChnlSelWorkitemCallback(
	IN	PVOID	Context
	);
#endif

#endif // #ifndef __INC_MGNTGEN_H
