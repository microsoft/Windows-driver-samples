/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    N62C_QuerySetOID.h

Abstract:
    Contains Port specific defines
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    05-12-2007    Created

Notes:

--*/

#pragma once

#ifndef _N62_QUERYSETOID__H
#define _N62_QUERYSETOID__H

NDIS_STATUS
N62CAPResetRequest(
	IN	PADAPTER	Adapter,
	OUT	PVOID		InformationBuffer,
	IN	ULONG		InputBufferLength,
	IN	ULONG		OutputBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
	);

NDIS_STATUS
N62C_METHOD_OID_PM_GET_PROTOCOL_OFFLOAD(
	IN	PADAPTER		pTargetAdapter,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InputBufferLength,
	IN	ULONG			OutputBufferLength,
	IN	ULONG			MethodId,
	OUT	PULONG			BytesWritten,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

#endif //_N62_QUERYSETOID__H
