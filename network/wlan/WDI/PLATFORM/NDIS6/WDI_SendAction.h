////////////////////////////////////////////////////////////////////////////////
//
//	File name:		WDI_SendAction.h
//	Description:	
//
//	Author:			haich
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __INC_WDI_SEND_ACTION_H
#define __INC_WDI_SEND_ACTION_H


//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------

typedef
VOID
(*WDI_SEND_ACTION_STATE_CB)
(
	IN  VOID					*req,
	IN  int						state,
	IN  VOID					*pCtx
);

typedef struct _WDI_SEND_ACTION_CTX
{
	WDI_P2P_ACTION_FRAME_TYPE 	reqType;	// type of the sending frame
	WDI_P2P_ACTION_FRAME_TYPE 	rspType;	// expected response frame to the sending frame
	ADAPTER						*pAdapter;	// the adapter
	u1Byte						da[6];		// destination of the sending frame
	BOOLEAN						bGo;		// specifies if the target device is a P2P GO
	u1Byte						token;		// the dialog token specified by the OS to be used
	u1Byte						oldToken;	// the original Dialog Token in P2PInfo
	ULONG						timeout;	// time bound for sending the action frame
	VOID						*req;		// the corresp. off chnl tx req
	u1Byte 						compEvent;	// corresp. complete event
}WDI_SEND_ACTION_CTX;

//-----------------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------------
NDIS_STATUS
WDI_SendAction_TerminateReq(
	IN  ADAPTER					*pAdapter
	);

u1Byte
WDI_SendAction_UpdateTxFrameDialogToken(
	IN  VOID 					*req,
	IN  ADAPTER					*pAdapter,
	IN  WDI_P2P_ACTION_FRAME_TYPE frameType
	);

RT_STATUS
WDI_SendAction_SkipProbeTemporarilly(
	IN	VOID					*req
	);

VOID *
WDI_SendAction_PdReq(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	);

VOID *
WDI_SendAction_PdRsp(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	);

VOID *
WDI_SendAction_InvitationReq(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	);

VOID *
WDI_SendAction_InvitationRsp(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	);

VOID *
WDI_SendAction_GoNegReq(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	);

VOID *
WDI_SendAction_GoNegRsp(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	);

VOID *
WDI_SendAction_GoNegConfirm(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	);

VOID *
WDI_SendAction_Req(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	);

VOID *
WDI_SendAction_Rsp(
	IN  ADAPTER					*pAdapter,
	IN  WDI_TASK_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param,
	IN  WDI_SEND_ACTION_STATE_CB stateCb
	);

#endif
