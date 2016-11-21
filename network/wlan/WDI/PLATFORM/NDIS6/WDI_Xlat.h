////////////////////////////////////////////////////////////////////////////////
//
//	File name:		WDI_Xlat.h
//	Description:	
//
//	Author:			haich
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __INC_WDI_XLAT_H
#define __INC_WDI_XLAT_H

#define WDI_XLAT_GET_OID_MP_RSVD_0(__req) ( (*(VOID **)((__req)->MiniportReserved)) )
#define WDI_XLAT_GET_OID_MP_RSVD_1(__req) ( (*(VOID **)((__req)->MiniportReserved + sizeof(VOID *))) )

#define WDI_XLAT_SET_OID_MP_RSVD_0(__req, __p) ( *(VOID **)((__req)->MiniportReserved) = (__p) )
#define WDI_XLAT_SET_OID_MP_RSVD_1(__req, __p) ( *(VOID **)((__req)->MiniportReserved + sizeof(VOID *)) = (__p) )

//-----------------------------------------------------------------------------
// Data
//-----------------------------------------------------------------------------

typedef struct _WDI_TASK_PARAM
{
	RT_OID_HANDLER						*hTask;
	UINT								allocSize;
}WDI_TASK_PARAM;

//-----------------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------------
DOT11_AUTH_ALGORITHM
Wdi_Xlat_AuthMode(
	IN  WDI_AUTH_ALGORITHM		algo
	);

DOT11_CIPHER_ALGORITHM
Wdi_Xlat_CipherAlgo(
	IN  WDI_CIPHER_ALGORITHM	algo
	);

VOID
Wdi_Xlat_FreeOid(
	IN  NDIS_OID_REQUEST		*req
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocCreateMacOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_CREATE_PORT_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocDeleteMacOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_DELETE_PORT_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocStartApReqOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_START_AP_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSetDesiredSsidListOid(
	IN	RT_OID_HANDLER			*hTask,
	IN	WDI_SSID				*param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocResetOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_DOT11_RESET_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocChangeOpModeOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_CHANGE_OPERATION_MODE_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocWfdDiscoverOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_DISCOVER_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocDevInfoOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_DEVICE_INFO_CONTAINER *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocDevCapOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_DEVICE_CAPABILITY_CONTAINER *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocGrpCapOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_GROUP_OWNER_CAPABILITY_CONTAINER *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSecDevTypeOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_SECONDARY_DEVICE_TYPE_CONTAINER *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocListenStateDiscoverabilityOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_LISTEN_STATE_CONTAINER *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocListenChannelOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_P2P_CHANNEL_CONTAINER *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocGetDialogTokenOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_GET_NEXT_ACTION_FRAME_DIALOG_TOKEN_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendGONegReqReqOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendInvitationReqOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendPDReqOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_REQUEST_ACTION_FRAME_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendGONegResponseOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendGONegConfirmOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendInvitationRspOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocSendPDRspOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_P2P_SEND_RESPONSE_ACTION_FRAME_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocIncomingAssociationDecisionOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_TASK_SEND_AP_ASSOCIATION_RESPONSE_PARAMETERS *param
	);

NDIS_OID_REQUEST *
Wdi_Xlat_AllocWpsEnabledOid(
	IN  RT_OID_HANDLER			*hTask,
	IN  WDI_SET_P2P_WPS_ENABLED_PARAMETERS *param
	);

#endif
