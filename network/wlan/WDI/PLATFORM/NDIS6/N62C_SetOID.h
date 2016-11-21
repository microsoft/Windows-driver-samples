/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    N62C_SETOID.h

Abstract:
    Contains Port specific defines
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    04-22-2007    Created

Notes:

--*/

#pragma once

#ifndef _N62_SETOID__H
#define _N62_SETOID__H

NDIS_STATUS	
N62C_SET_OID_DOT11_INCOMING_ASSOCIATION_DECISION(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_DOT11_WPS_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_DOT11_ADDITIONAL_IE(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_DOT11_START_AP_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_DOT11_DISASSOCIATE_PEER_REQUEST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_DOT11_ASSOCIATION_PARAMS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_PM_ADD_WOL_PATTERN(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_PM_REMOVE_WOL_PATTERN(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_PM_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_PM_ADD_PROTOCOL_OFFLOAD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_PM_REMOVE_PROTOCOL_OFFLOAD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_PM_ADD_PROTOCOL_OFFLOAD_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_SET_OID_RECEIVE_FILTER_CLEAR_FILTER(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS	
N62C_QUERYSET_OID_RECEIVE_FILTER_SET_FILTER(
	IN	PADAPTER		pTargetAdapter,
	IN    NDIS_OID		Oid,
	IN    PVOID			InformationBuffer,
	IN    ULONG			InputBufferLength,
	IN    ULONG			OutputBufferLength,
	IN    ULONG			MethodId,
	OUT   PULONG			BytesWritten,
	OUT   PULONG			BytesRead,
	OUT   PULONG			BytesNeeded	
);

#endif //_N62_SETOID__H
