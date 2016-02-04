
#ifndef __INC_HAL_TX_FEEDBACK_H
#define __INC_HAL_TX_FEEDBACK_H


// The identifier/reason of the tx feedback
#define	RT_TX_FEEDBACK_ID_AP_WMM_PS_PKT				BIT0
#define	RT_TX_FEEDBACK_ID_AP_WMM_EOSP_ENDING		BIT1
#define	RT_TX_FEEDBACK_ID_TDLS_TEARDOWN_PKT			BIT2
#define	RT_TX_FEEDBACK_ID_TDLS_CHNLSWITCH			BIT3
#define	RT_TX_FEEDBACK_ID_TDLS_NULL_PKT				BIT4
#define	RT_TX_FEEDBACK_ID_TDLS_CS_DATA_PKT			BIT5
#define	RT_TX_FEEDBACK_ID_TDLS_CHNLSWITCH_REQ		BIT6
#define	RT_TX_FEEDBACK_ID_TDLS_PTR					BIT7
#define	RT_TX_FEEDBACK_ID_SCAN_TX_NULL				BIT8	// Send Null frame when starting scan process
#define RT_TX_FEEDBACK_ID_P2P_NEGO_TX				BIT9
#define RT_TX_FEEDBACK_ID_CCX_TSM					BIT10
#define RT_TX_FEEDBACK_ID_CCX_LINK_TEST				BIT11
#define RT_TX_FEEDBACK_ID_P2P_PD_RSP_TX				BIT12
#define RT_TX_FEEDBACK_ID_OFF_CHNL_TX				BIT13


#define 	RT_TX_FEEDBACK_ID_MULTICHANNEL_NULL_PKT	BIT31

typedef struct _RT_TX_FEEDBACK_INFO RT_TX_FEEDBACK_INFO, *PRT_TX_FEEDBACK_INFO;

typedef	VOID
(*RtTxFeedbackCallbackHandler)(
	PADAPTER pAdapter,
	const RT_TX_FEEDBACK_INFO * const pTxFeedbackInfo
);

typedef struct _RT_TX_FEEDBACK_INFO_TX_PROFILE
{
	RT_VALID_PAIR_BOOLEAN	bPktOk;
	RT_VALID_PAIR_BOOLEAN	bRetryOver;
	RT_VALID_PAIR_BOOLEAN	bLifeTimeOver;
	RT_VALID_PAIR_BOOLEAN	bUnicast;
	RT_VALID_PAIR_U4BYTE	uQueueID;
	RT_VALID_PAIR_U4BYTE	uMacID;
	RT_VALID_PAIR_U4BYTE	uDataRetryCount;
	RT_VALID_PAIR_U4BYTE	uQueueTimeUs;
	RT_VALID_PAIR_U4BYTE	uFinalDataRateIndex;

} RT_TX_FEEDBACK_INFO_TX_PROFILE, *PRT_TX_FEEDBACK_INFO_TX_PROFILE;

#define GET_TX_PROFILE_PKT_OK(_pTxProfile)						(((_pTxProfile)->bPktOk.bVaild) ? 				((_pTxProfile)->bPktOk.Data) 				: TxFeedbackAssertForInvalidData())
#define GET_TX_PROFILE_RETRY_OVER(_pTxProfile)					(((_pTxProfile)->bRetryOver.bVaild) ? 			((_pTxProfile)->bRetryOver.Data) 			: TxFeedbackAssertForInvalidData())
#define GET_TX_PROFILE_LEFE_TIME_OVER(_pTxProfile)				(((_pTxProfile)->bLifeTimeOver.bVaild) ? 			((_pTxProfile)->bLifeTimeOver.Data) 		: TxFeedbackAssertForInvalidData())
#define GET_TX_PROFILE_IS_UNICAST(_pTxProfile)					(((_pTxProfile)->bUnicast.bVaild) ? 				((_pTxProfile)->bUnicast.Data) 				: TxFeedbackAssertForInvalidData())
#define GET_TX_PROFILE_QUEUE_ID(_pTxProfile)					(((_pTxProfile)->uQueueID.bVaild) ? 			((_pTxProfile)->uQueueID.Data) 			: TxFeedbackAssertForInvalidData())
#define GET_TX_PROFILE_MAC_ID(_pTxProfile)						(((_pTxProfile)->uMacID.bVaild) ? 				((_pTxProfile)->uMacID.Data) 				: TxFeedbackAssertForInvalidData())
#define GET_TX_PROFILE_DATA_RETRY_COUNT(_pTxProfile)			(((_pTxProfile)->uDataRetryCount.bVaild) ? 		((_pTxProfile)->uDataRetryCount.Data) 		: TxFeedbackAssertForInvalidData())
#define GET_TX_PROFILE_QUEUE_TIME_US(_pTxProfile)				(((_pTxProfile)->uQueueTimeUs.bVaild) ? 		((_pTxProfile)->uQueueTimeUs.Data) 		: TxFeedbackAssertForInvalidData())
#define GET_TX_PROFILE_FINAL_DATA_RATE_INDEX(_pTxProfile)		(((_pTxProfile)->uFinalDataRateIndex.bVaild) ? 	((_pTxProfile)->uFinalDataRateIndex.Data) 	: TxFeedbackAssertForInvalidData())


typedef struct	_RT_TX_FEEDBACK_INFO
{
	// User Configuration Part ------------------------------------------------------
	RT_LIST_ENTRY				List;			// The linked list pointer.
	u4Byte						Identifier;		// The ID to identify this tx feedback.
	u4Byte						Reason;			// The reason for this packet.
	PADAPTER					pAdapter;		// The adapter address.
	RtTxFeedbackCallbackHandler	Callback;		// The callback function.
	PVOID						pContext;		// The context address.
	// --------------------------------------------------------------------------

	// Packet Tx Profile -----------------------------------------------------------
	RT_TX_FEEDBACK_INFO_TX_PROFILE	TxProfile;
	// --------------------------------------------------------------------------
	
}RT_TX_FEEDBACK_INFO, *PRT_TX_FEEDBACK_INFO;


#define	HAL_MAX_TX_FEEDBACK_INFO_NUM			16 		// The same number with TID.
#define	HAL_MAX_TX_FEEDBACK_QUEUE_NUM			7		// The number of the edca (TID) queue for the tx feedback information


typedef struct _TX_FEEDBACK_CONTEXT{

	BOOLEAN						bSupportTxFeedBack;
	RT_TX_FEEDBACK_INFO			TxFeedbackArray[HAL_MAX_TX_FEEDBACK_INFO_NUM];

	// Use RT_TX_SPINLOCK to protect the queue context ------------------------------------------------------------
	RT_LIST_ENTRY				TxFeedbackIdleQueue;
	RT_LIST_ENTRY				TxFeedbackEDCAQueue[HAL_MAX_TX_FEEDBACK_QUEUE_NUM];	// The number of TID.
	// --------------------------------------------------------------------------------------------------------

} TX_FEEDBACK_CONTEXT, *PTX_FEEDBACK_CONTEXT;

#define TX_FEEDBACK_SIZE_OF_CONTEXT 	sizeof(TX_FEEDBACK_CONTEXT)



typedef struct _TX_FEEDBACK_TCB_ZONE{

	PRT_TX_FEEDBACK_INFO 	pTxFeedbackInfo;
	
} TX_FEEDBACK_TCB_ZONE, *PTX_FEEDBACK_TCB_ZONE;

#define TX_FEEDBACK_SIZE_OF_TCB_ZONE 	sizeof(TX_FEEDBACK_TCB_ZONE)





VOID
TxFeedbackInitialize(
	PADAPTER pAdapter
);

BOOLEAN
TxFeedbackExistActiveTxFeedbackInfo(
	PADAPTER pAdapter
);

PRT_TX_FEEDBACK_INFO
TxFeedbackGetMatchedTxFeedbackInfoFromSpecificQueue(
	PADAPTER pAdapter,
	u1Byte QueueID,
	u4Byte TxFeedbackID
);

static VOID
TxFeedbackReturnTxFeedbackInfo(
	PADAPTER pAdapter,
	PRT_TX_FEEDBACK_INFO pTxFeedbackInfo
);

VOID
TxFeedbackSetActiveTxFeedbackInfo(
	PADAPTER pAdapter,
	u1Byte QueueID,
	PRT_TCB pTcb
);

VOID
TxFeedbackSetTxFeedbackInfoID(
	PADAPTER pAdapter,
	PRT_TCB pTcb
);

VOID
TxFeedbackExecuteCallbackAndReturnTxFeedbackInfo(
	PADAPTER pAdapter,
	PRT_TX_FEEDBACK_INFO pTxFeedbackInfo
);


BOOLEAN
TxFeedbackInstallTxFeedbackInfoForTcb(
	PADAPTER pAdapter,
	PRT_TCB pTcb
);

VOID
TxFeedbackDeInstallTxFeedbackInfoForTcb(
	PADAPTER pAdapter,
	PRT_TCB pTcb
);

VOID
TxFeedbackFillTxFeedbackInfoUserConfiguration(
	PRT_TCB 					pTcb,
	u4Byte						Reason,
	PADAPTER					pAdapter,
	RtTxFeedbackCallbackHandler	Callback,
	PVOID						pContext
);

BOOLEAN
TxFeedbackIsTxFeedbackInfoInstalled(
	PRT_TCB 					pTcb
);

BOOLEAN
TxFeedbackCheckCurrentFunctionality(
	PADAPTER pAdapter
);

BOOLEAN
TxFeedbackAssertForInvalidData(
);




#endif
