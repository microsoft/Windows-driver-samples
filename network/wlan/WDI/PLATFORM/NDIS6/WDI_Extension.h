////////////////////////////////////////////////////////////////////////////////
//
//	File name:		WDI_Extension.h
//	Description:	Define data structure and export some common helper functions.
//
//	Author:			hpfan
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __INC_WDI_EXTENSION_H
#define __INC_WDI_EXTENSION_H

#define	MAX_OID_INPUT_BUFFER_LEN				1024

#define	RT_OID_HANDLER_STATUS_INITIALIZED		BIT0
#define	RT_OID_HANDLER_STATUS_SET				BIT1
#define	RT_OID_HANDLER_STATUS_FIRED				BIT2
#define	RT_OID_HANDLER_STATUS_CANCELED			BIT3
#define	RT_OID_HANDLER_STATUS_FINISHED			BIT4
#define	RT_OID_HANDLER_STATUS_RELEASED			BIT5
#define	RT_OID_HANDLER_STATUS_RETURNED			BIT6
#define	RT_OID_HANDLER_STATUS_EXECUTE_FAILURE	BIT7


typedef enum _OIDHANDLE_TYPE
{
	OIDHANDLE_TYPE_NONE			= 0,
	OIDHANDLE_TYPE_TASK			= 1,
	OIDHANDLE_TYPE_PROPERTY		= 2,
	OIDHANDLE_TYPE_MAX			= 3,
} OIDHANDLE_TYPE;


typedef enum _OIDHANDLE_EVENT
{
	OIDHANDLE_EVENT_NONE			= 0,
	OIDHANDLE_EVENT_PENDING		= 1,
	OIDHANDLE_EVENT_COMPLETE	= 2,
	OIDHANDLE_EVENT_MAX			= 3,
} OIDHANDLE_EVENT;


BOOLEAN
OidHandle_VerifyTask(
	IN  PADAPTER				pAdapter,
	IN  u4Byte				WdiTaskOid
	);

RT_STATUS
OidHandle_Complete(
	IN  PADAPTER				pAdapter,
	IN  OIDHANDLE_TYPE		Type
	);

RT_STATUS
OidHandle_IndicateM3(
	IN	PADAPTER			pAdapter,
	IN	PRT_OID_HANDLER	pOidHandle
	);

RT_STATUS
OidHandle_IndicateM4(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle,
	IN  RT_TASK_ENTRY			*pTaskEntry
	);

RT_STATUS
OidHandle_Cancel(
	IN  PADAPTER				pAdapter,
	IN  u4Byte				NdisOid,
	IN  u4Byte				TransactionId,
	IN  u2Byte				PortId
	);

RT_STATUS
OidHandle_AbortAction(
	IN  PADAPTER				pAdapter,
	IN  PRT_OID_HANDLER		pOidHandle
	);

NDIS_STATUS
N6WdiHandleOidRequest(
	IN  NDIS_HANDLE			MiniportAdapterContext,
	IN  PNDIS_OID_REQUEST	pNdisRequest
    );

NDIS_STATUS
N6WdiAbortOidRequest(
	IN  NDIS_HANDLE			MiniportAdapterContext,
	IN  PNDIS_OID_REQUEST	pNdisRequest
    );


NDIS_STATUS
WDICommandHandleInit(
	IN  PADAPTER		pAdapter
	);

NDIS_STATUS
WDICommandHandleDeinit(
	IN  PADAPTER		pAdapter
	);

VOID
WDITaskCommandWorkItemCallback(
	IN PVOID			pContext
	);

VOID
WDIPropertyCommandWorkItemCallback(
	IN PVOID			pContext
	);

NDIS_STATUS
WDICommandHangCleanup(
	IN  PADAPTER		pAdapter
	);

#endif
