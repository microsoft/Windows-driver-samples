////////////////////////////////////////////////////////////////////////////////
//
//	File name:		N63C_SendAction.h
//	Description:	
//
//	Author:			haich
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __INC_N63C_SEND_ACTION_H
#define __INC_N63C_SEND_ACTION_H


//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------

typedef
VOID
(*N63C_SEND_ACTION_STATE_CB)
(
	IN  VOID					*req,
	IN  int						state,
	IN  VOID					*pCtx
);

typedef enum _N63C_P2P_ACTION_FRAME_TYPE
{ 
    N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_REQUEST = 1,
    N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_RESPONSE = 2,
    N63C_P2P_ACTION_FRAME_GO_NEGOTIATION_CONFIRM = 3,
    N63C_P2P_ACTION_FRAME_INVITATION_REQUEST = 4,
    N63C_P2P_ACTION_FRAME_INVITATION_RESPONSE = 5,
    N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_REQUEST = 6,
    N63C_P2P_ACTION_FRAME_PROVISION_DISCOVERY_RESPONSE = 7,  
    N63C_P2P_ACTION_FRAME_MAX_VALUE = 0xFFFFFFFF
}N63C_P2P_ACTION_FRAME_TYPE;

typedef struct _N63C_SEND_ACTION_CTX
{
	N63C_P2P_ACTION_FRAME_TYPE 	reqType;	// type of the sending frame
	N63C_P2P_ACTION_FRAME_TYPE 	rspType;	// expected response frame to the sending frame
	ADAPTER						*pAdapter;	// the adapter
	u1Byte						da[6];		// destination of the sending frame
	BOOLEAN						bGo;		// specifies if the target device is a P2P GO
	u1Byte						token;		// the dialog token specified by the OS to be used
	u1Byte						oldToken;	// the original Dialog Token in P2PInfo
	ULONG						timeout;	// time bound for sending the action frame
	VOID						*req;		// the corresp. off chnl tx req
	u1Byte 						compEvent;	// corresp. complete event
}N63C_SEND_ACTION_CTX;

//-----------------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------------
NDIS_STATUS
N63C_SendAction_TerminateReq(
	IN  ADAPTER					*pAdapter
	);

u1Byte
N63C_SendAction_UpdateTxFrameDialogToken(
	IN  VOID 					*req,
	IN  ADAPTER					*pAdapter,
	IN  INT						frameType
	);

RT_STATUS
N63C_SendAction_SkipProbeTemporarilly(
	IN	VOID					*req
	);

VOID *
N63C_SendAction_PdReq(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_PROVISION_DISCOVERY_REQUEST_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	);

VOID *
N63C_SendAction_PdRsp(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_PROVISION_DISCOVERY_RESPONSE_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	);

VOID *
N63C_SendAction_InvitationReq(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_INVITATION_REQUEST_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	);

VOID *
N63C_SendAction_InvitationRsp(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_INVITATION_RESPONSE_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	);

VOID *
N63C_SendAction_GoNegReq(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_GO_NEGOTIATION_REQUEST_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	);

VOID *
N63C_SendAction_GoNegRsp(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_GO_NEGOTIATION_RESPONSE_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	);

VOID *
N63C_SendAction_GoNegConfirm(
	IN  ADAPTER					*pAdapter,
	IN  DOT11_SEND_GO_NEGOTIATION_CONFIRMATION_PARAMETERS *param,
	IN  N63C_SEND_ACTION_STATE_CB stateCb
	);

#endif
