//---------------------------------------------------------------------------
//
// Copyright (c) 2014 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		
//

#ifndef __INC_OFF_CHNL_TX_H
#define __INC_OFF_CHNL_TX_H

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------

#define OFF_CHNL_TX_PER_TX_ATTEMPT_FLAG_CHNL_HIT		BIT0
#define OFF_CHNL_TX_PER_TX_ATTEMPT_FLAG_FRAME_SENT		BIT1
#define OFF_CHNL_TX_PER_TX_ATTEMPT_FLAG_FRAME_ACKED		BIT2

#define OFF_CHNL_TX_PER_REQ_FLAG_RETRY					BIT0
#define OFF_CHNL_TX_PER_REQ_FLAG_COMPLETED				BIT1

typedef struct _OFF_CHNL_TX_STATUS_FLAG
{
	u4Byte						perTxAttemptFlag;
	u4Byte						perReqFlag;
}OFF_CHNL_TX_STATUS_FLAG;

typedef enum _OFF_CHNL_TX_STATE
{
	OFF_CHNL_TX_STATE_CHNL_HIT = 0,
	OFF_CHNL_TX_STATE_COMPLETE,
	OFF_CHNL_TX_STATE_PRE_DESTROY,
	OFF_CHNL_TX_STATE_RETRY,
	OFF_CHNL_TX_STATE_FRAME_SENT,
}OFF_CHNL_TX_STATE;

typedef
VOID
(*OFF_CHNL_TX_STATE_CB)
(
	IN  VOID					*req,
	IN  int						state,
	IN  VOID					*pCtx
);

//-----------------------------------------------------------------------------
// Function, process
//-----------------------------------------------------------------------------

VOID *
OffChnlTx_AllocReq(
	IN  ADAPTER					*pAdapter,
	IN  const u1Byte			*mac,
	IN  OFF_CHNL_TX_STATE_CB	stateCb,
	IN  VOID					*stateCtx,
	IN  u4Byte					cliRsvdLen
	);

RT_STATUS
OffChnlTx_IssueReq(
	IN  ADAPTER					*pAdapter,
	IN  VOID					*req,
	IN  const char				*typeInfo
	);

RT_STATUS
OffChnlTx_AbortReq(
	IN  ADAPTER					*pAdapter
	);

//-----------------------------------------------------------------------------
// Function, setter
//-----------------------------------------------------------------------------
VOID
OffChnlTx_SetupCbCtx(
	IN  VOID					*req,
	IN  OFF_CHNL_TX_STATE_CB	stateCb,
	IN  VOID					*stateCtx
	);

FRAME_BUF *
OffChnlTx_GetTxFrameBuf(
	IN  VOID					*req
	);

FRAME_BUF *
OffChnlTx_GetProbeReqBuf(
	IN  VOID					*req
	);

RT_STATUS
OffChnlTx_SetProbeEnabled(
	IN  VOID					*req,
	IN  BOOLEAN					bEnableProbe
	);

FRAME_BUF *
OffChnlTx_GetProbeReqBuf(
	IN  VOID					*req
	);

RT_STATUS
OffChnlTx_SetPostSearchListeningEnabled(
	IN  VOID					*req,
	IN  BOOLEAN					bEnablePostSeachListening,
	IN  u1Byte					chnl,
	IN  u4Byte					duration
	);

RT_STATUS
OffChnlTx_SetTxChnl(
	IN  VOID					*req,
	IN  u1Byte					txChnl
	);

u1Byte
OffChnlTx_GetTxChnl(
	IN  VOID					*req
	);

RT_STATUS
OffChnlTx_SetTxAttemptCount(
	IN  VOID					*req,
	IN  u1Byte					attemptCount
	);

RT_STATUS
OffChnlTx_SetTimeBound(
	IN  VOID					*req,
	IN  u4Byte					timeBound
	);

RT_STATUS
OffChnlTx_SetPostAckDwellTime(
	IN  VOID					*req,
	IN  u4Byte					postAckDwellTime
	);

RT_STATUS
OffChnlTx_SetRetryIntermittentTime(
	IN  VOID					*req,
	IN  u4Byte					retryIntermittentTime
	);

RT_STATUS
OffChnlTx_SetDataRate(
	IN	VOID					*req,
	IN	u1Byte					dataRate
	);

OFF_CHNL_TX_STATUS_FLAG
OffChnlTx_GetTxStatusFlag(
	IN	VOID					*req
	);

RT_STATUS
OffChnlTx_SkipProbeTemporarilly(
	IN	VOID					*req
	);

VOID *
OffChnlTx_GetCliRsvdBuf(
	IN	VOID					*req
	);

RT_STATUS
OffChnlTx_SetShtPostAckDwellOnPositivelyAcked(
	IN	VOID					*req,
	IN	u1Byte					bShutPostAckDwellOnPositivelyAcked
	);

//-----------------------------------------------------------------------------
// Function, dbg
//-----------------------------------------------------------------------------

#endif	// #ifndef __INC_OFF_CHNL_TX_H
