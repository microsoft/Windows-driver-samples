/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    N62C_QueryOID.h

Abstract:
    Contains Port specific defines
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    04-22-2007    Created

Notes:

--*/

#pragma once

#ifndef _N62_QUERYOID__H
#define _N62_QUERYOID__H

NDIS_STATUS
N62CQueryInterruptModerationSettings(
	IN PADAPTER			Adapter,
	IN PVOID				InformationBuffer,
	IN ULONG			InformationBufferLength
    );

NDIS_STATUS
N62CQueryLinkParameters(
	IN PADAPTER			Adapter,
	IN PVOID				InformationBuffer,
	IN ULONG			InformationBufferLength
    );

NDIS_STATUS
N62CQueryAdditionalIE(
	IN 	PADAPTER				Adapter,
	OUT PDOT11_ADDITIONAL_IE	AdditionalIe,
	IN	u4Byte					InformationBufferLength,
	OUT	pu4Byte					BytesWritten,
	OUT	pu4Byte					BytesNeeded	
	);

NDIS_STATUS
N62CApEnumPeerInfo(
	IN	PADAPTER				Adapter,
	OUT	PDOT11_PEER_INFO_LIST	PeerInfo,
	IN	u4Byte					InformationBufferLength,
	OUT	pu4Byte 					BytesWritten,
	OUT	pu4Byte 					BytesNeeded
	);

NDIS_STATUS
N62CQuery_DOT11_DESIRED_PHY_LIST(
	IN	PADAPTER						Adapter,
	OUT	PVOID							InformationBuffer,
	IN	ULONG							InformationBufferLength,
	OUT	PULONG							BytesWritten,
	OUT	PULONG							BytesNeeded
	);

NDIS_STATUS
N62C_QUERY_OID_DOT11_WPS_ENABLED(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N62C_QUERY_OID_DOT11_ADDITIONAL_IE(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N62C_QUERY_OID_DOT11_ENUM_PEER_INFO(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N62C_QUERY_OID_DOT11_AVAILABLE_CHANNEL_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N62C_QUERY_OID_DOT11_AVAILABLE_FREQUENCY_LIST(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N62C_QUERY_OID_PM_PARAMETERS(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N62C_QUERY_OID_GEN_INTERRUPT_MODERATION(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N62C_QUERY_OID_PACKET_COALESCING_FILTER_MATCH_COUNT(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N62C_QUERY_OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);

NDIS_STATUS
N62C_QUERY_OID_RECEIVE_FILTER_CURRENT_CAPABILITIES(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesNeeded
);
#endif //_N62_QUERYOID__H
