#ifndef __INC_NDIS_COMMONOID_H
#define __INC_NDIS_COMMONOID_H

#pragma pack(1)
typedef struct _RTK_DBG_CTRL_OIDS{
	u2Byte	ctrlType;
	u2Byte	ctrlDataLen;
	ULONG	CtrlData;	// head data
} RTK_DBG_CTRL_OIDS, *PRTK_DBG_CTRL_OIDS;

typedef enum _RTK_DBG_TYPE_OIDS
{	
	RTK_DBG_OIDS_BT_PROFILE			=0,
}RTK_DBG_TYPE_OIDS;
#pragma pack()

//============================================
//	Local function
//============================================


//============================================
//	Extern function
//============================================

//============================================
//	For OID Query
//============================================
NDIS_STATUS
OIDQ_RTKReadReg(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesNeeded,
	IN	PULONG		pulInfo
	);
NDIS_STATUS
OIDQ_RTKReadRegSIC(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesWritten,
	OUT	PULONG		BytesNeeded,
	IN	PULONG		pulInfo
	);

//============================================
//	For OID Set
//============================================
NDIS_STATUS
OIDS_RTKWriteReg(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
	);
NDIS_STATUS
OIDS_RTKWriteRegSIC(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded
	);
NDIS_STATUS
OIDS_RTKDbgControl(
	IN	PADAPTER	Adapter,
	IN	PVOID		InformationBuffer,
	IN	ULONG		InformationBufferLength,
	OUT	PULONG		BytesRead,
	OUT	PULONG		BytesNeeded	
	);
#endif
