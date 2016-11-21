#ifndef	_CCX_EXTENSION_H
#define	_CCX_EXTENSION_H

#define	CCX_IHV_EXT_SUPPORT	// CCX IHV exists and is capable to handle the ccx extension events.

typedef	struct _OCTET_STRING	OCTET_STRING, *POCTET_STRING;

//-------------------------------------------------------------------------------
//
//Define Marco here
//Edit by Mars 2008/02/21
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//
//Declear function here
//Edit by Mars 2008/02/21
//-------------------------------------------------------------------------------
//===========================================================================
VOID 
CcxIndicateStatusIndication (
	IN	PADAPTER		pAdapter,
	IN	u4Byte			StatusCode,
	IN	PVOID			pStatusBuffer,
	IN	ULONG			StatusBufferSize
	);

NDIS_STATUS
CCX_EventQuerySetInformation(
	IN	PADAPTER		pAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InputBufferLength,
	IN	ULONG			OutputBufferLength,
	IN	ULONG			MethodId,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
	);

NDIS_STATUS
CCX_MpEventSetInformation(
	IN	PADAPTER                  pAdapter,
	IN	NDIS_OID                  Oid,
	IN	PVOID                     InformationBuffer,
	IN	ULONG                     InformationBufferLength,
	OUT	PULONG                    BytesRead,
	OUT	PULONG                    BytesNeeded
	);

NDIS_STATUS
CCX_MpEventQueryInformation(
	IN		PADAPTER		pAdapter,
	IN		NDIS_OID		Oid,
	IN		PVOID			InformationBuffer,
	IN		ULONG			InformationBufferLength,
	OUT		PULONG			BytesWritten,
	OUT		PULONG			BytesNeeded
    );


u4Byte
CCX_N6IndicateRxPacket(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd
	);

u4Byte
CCX_N6IndicateTxPacket(
	IN	PADAPTER		pAdapter,
	IN	PRT_TCB			pTcb
	);
//===========================================================================

#endif
