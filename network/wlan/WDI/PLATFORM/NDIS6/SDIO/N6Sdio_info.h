#ifndef __INC_SDIO_INFO_H
#define __INC_SDIO_INFO_H

NDIS_STATUS
InterfaceSetInformationHandleCustomizedOriginalMPSetOid(
	IN	NDIS_HANDLE		MiniportAdapterContext,
	IN	NDIS_OID		Oid,
	IN	PVOID			InformationBuffer,
	IN	ULONG			InformationBufferLength,
	OUT	PULONG			BytesRead,
	OUT	PULONG			BytesNeeded
);

#endif // #ifndef __INC_SDIO_INFO_H
