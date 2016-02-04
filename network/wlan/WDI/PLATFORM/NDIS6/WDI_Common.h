////////////////////////////////////////////////////////////////////////////////
//
//	File name:		WDI_Common.h
//	Description:	Define basic capability of WDI.
//
//	Author:			hpfan
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __INC_WDI_COMMON_H
#define __INC_WDI_COMMON_H

#if (WDI_SUPPORT == 1)

#define	WDI_802_11_MTU_SIZE						1500
#define	WDI_MAX_MCAST_LIST_NUM					MAX_MCAST_LIST_NUM
#define	WDI_802_11_DATA_BACK_FILL_SIZE			NATIVE_802_11_DATA_BACK_FILL_SIZE
#define	WDI_802_11_MAX_XMIT_LINK_SPEED			NATIVE_802_11_MAX_XMIT_LINK_SPEED
#define	WDI_802_11_MAX_RCV_LINK_SPEED			NATIVE_802_11_MAX_RCV_LINK_SPEED
#define	WDI_802_11_MAX_SCAN_SSID					255	// no match with previous value
#define	WDI_802_11_MAX_DESIRED_BSSID			NATIVE_802_11_MAX_DESIRED_BSSID
#define	WDI_802_11_MAX_DESIRED_SSID				NATIVE_802_11_MAX_DESIRED_SSID
#define	WDI_802_11_MAX_PRIVACY_EXEMPTION		NATIVE_802_11_MAX_PRIVACY_EXEMPTION
#define	WDI_802_11_MAX_KEY_MAPPING_ENTRY		NATIVE_802_11_MAX_KEY_MAPPING_ENTRY
#define	WDI_802_11_MAX_DEFAULT_KEY_ENTRY		NATIVE_802_11_MAX_DEFAULT_KEY_ENTRY
#define	WDI_802_11_MAX_WEP_KEY_LENGTH			NATIVE_802_11_MAX_WEP_KEY_LENGTH
#define	WDI_802_11_MAX_PER_STA_DEFAULT_KEY		NATIVE_802_11_MAX_PER_STA_DEFAULT_KEY
#define	WDI_802_11_MAX_NETWORKOFFLOAD_SIZE	NATIVE_802_11_MAX_NETWORKOFFLOAD_SIZE


NDIS_STATUS
WDI_Initialize(
	IN	PDRIVER_OBJECT							pDriverObject,
	IN	PUNICODE_STRING							RegistryPath,
	IN	PNDIS_MINIPORT_DRIVER_CHARACTERISTICS	pMChars
	);

VOID
WDI_DeInitialize(	
	IN  NDIS_HANDLE			DriverHandle
	);

VOID
WDI_InitRxQueue(	
	IN  PADAPTER		pAdapter
	);

VOID
WDI_DeInitRxQueue(	
	IN  PADAPTER		pAdapter
	);

VOID
WDI_DeInitRxQueue(	
	IN  PADAPTER		pAdapter
	);
	
VOID
WDI_PnPNotiry(	
	IN  PADAPTER				pAdapter
	);

NDIS_STATUS
WDI_HandleOidRequest(
	IN  NDIS_HANDLE			MiniportAdapterContext,
	IN  PNDIS_OID_REQUEST	pNdisRequest
	);


VOID
WdiExt_IndicateApAssocReqReceived(
	IN  ADAPTER					*pAdapter,
	IN  u4Byte					assocReqLen,
	IN  u1Byte					*assocReq
	);


VOID
WdiExt_IndicateApAssocRspSent(
	IN  ADAPTER					*pAdapter,
	IN  u2Byte					status
	);

RT_STATUS
WDIAllocateMetaData(
	IN	PADAPTER				pAdapter,
	IN	PNET_BUFFER_LIST		pNBL,
	IN	PRT_RFD					pRfd
	);


VOID
WDIFreeMetaData(
	IN	PADAPTER	pAdapter,
	IN	PNET_BUFFER_LIST	pNBL
	);

u4Byte
WDI_InsertDataInQueue(
	IN	PADAPTER			pAdapter,
	IN	PRT_RFD				pRfd,
	IN	PNET_BUFFER_LIST	pNBL
);

BOOLEAN
WDI_NotifyDataInQueue(
	IN	PADAPTER				pAdapter,
	IN	u1Byte					ExTid,
	IN	u2Byte					PeerId,
	IN	RT_RX_INDICATION_LEVEL	level
	);

VOID
WDI_CompletePacket(
	IN	PADAPTER			pAdapter,
	IN	PNET_BUFFER_LIST	pNBL
	);

VOID
WDI_IndicateScanComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
	);

VOID
WDI_IndicateBssList(
	PADAPTER		pAdapter,
	RT_STATUS		status
	);

VOID
WDI_IndicateBssListBySSID(
	PADAPTER		pAdapter,
	WDI_SSID		*pSSID
	);

VOID
WDI_IndicateConnectionComplete(	
	PADAPTER		Adapter,
	RT_STATUS		status
	);

VOID
WDI_IndicateAssociationComplete(
	PADAPTER		Adapter,
	RT_STATUS		status
	);

VOID
WDI_IndicateDisassociation(	
	PADAPTER		Adapter,
	u2Byte			reason,
	pu1Byte			pAddr
);

VOID
WDI_IndicateNLODiscovery(
	PADAPTER		pAdapter
	);

VOID
WDI_IndicateCurrentPhyStatus(
	PADAPTER		Adapter
	);

VOID
WDI_IndicateP2PEvent(	
	VOID 			*pvP2PInfo,
	u4Byte 			EventID,
	MEMORY_BUFFER	*pInformation
	);

BOOLEAN
WDI_CompleteCreateDeleteMac(
	IN  PADAPTER				pAdapter,
	IN  NDIS_STATUS				status
	);

VOID
WDI_IndicateRoamingComplete(	
	PADAPTER		Adapter
	);

VOID
WDI_IndicateLinkStateChanged(	
	PADAPTER		pAdapter,
	BOOLEAN			bForceLinkQuality,
	u1Byte			ucLinkQuality
	);

RT_STATUS
WDI_IndicateActionFrame(
	IN	PADAPTER		pAdapter,
	IN	POCTET_STRING	posMpdu
);

VOID
WDI_IndicateWakeReason(
	IN PADAPTER		pAdapter,
	IN BOOLEAN		bWakePacket,
	IN pu1Byte		pBuffer,
	IN u2Byte		BufferLen
	);

VOID
WDI_IndicateRoamingNeeded(	
	PADAPTER				pAdapter,
	RT_PREPARE_ROAM_TYPE	IndicationReason
	);

VOID 
WDI_AddGroupPeer(
	IN  PADAPTER 	pAdapter,
	IN  u2Byte		PortNumber
	);

VOID 
WDI_DeleteGroupPeer(
	IN  PADAPTER 	pAdapter,
	IN  u2Byte		PortNumber
	);

NDIS_STATUS 
WDI_AddDatapathPeer(
	IN	PADAPTER 	pAdapter,
	IN	pu1Byte 		pAddr
	);

void 
WDI_DeleteDatapathPeer(
	IN	PADAPTER 	pAdapter,
	IN	pu1Byte		pAddr
	);

VOID
WDI_FreeRxFrame(
	IN	PADAPTER	pAdapter,
	IN	PNET_BUFFER_LIST pNetBufferList
	);

VOID
WDI_FetchNBLByPort(
	IN		PADAPTER			pAdapter,
	IN		u2Byte				Port_ID,
	OUT		PNET_BUFFER_LIST	*ppNetBufferList
	);

VOID 
WDI_IndicateTKIPMICFailure(
	PADAPTER		pAdapter
	);

VOID
WDI_TxCreditCheck(
	IN  PADAPTER		pAdapter
	);

VOID
WDI_IndicateFWStalled(
	IN	PADAPTER	pAdapter
	);

VOID
WDI_UpdateDefaultSetting(
	IN  PADAPTER				pAdapter
	);

RT_STATUS
WDI_IndicateGeneralEvent(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			Event,
	IN	PVOID			pInfoBuffer,
	IN	u4Byte			InfoBruuferLen
	);

#else


#define	WDI_Initialize(pDriverObject, RegistryPath, pMChars)				NDIS_STATUS_NOT_SUPPORTED
#define	WDI_DeInitialize(DriverHandle)
#define	WDI_InitRxQueue(pAdapter)
#define	WDI_DeInitRxQueue(pAdapter)
#define	WDI_PnPNotiry(pAdapter)
#define	WDI_HandleOidRequest(MiniportAdapterContext, pNdisRequest)	NDIS_STATUS_NOT_RECOGNIZED
#define	WdiExt_IndicateApAssocReqReceived(pAdapter, assocReqLen, assocReq)
#define	WdiExt_IndicateApAssocRspSent(pAdapter, status)
#define	WDIAllocateMetaData(pAdapter, pNBL, pRfd)					RT_STATUS_NOT_SUPPORT
#define	WDIFreeMetaData(pAdapter, pNBL)
#define	WDI_InsertDataInQueue(pAdapter, pRfd, pNBL)					0
#define	WDI_NotifyDataInQueue(pAdapter, ExTid, PeerId, level)			FALSE
#define	WDI_CompletePacket(pAdapter, pNBL)
#define	WDI_IndicateScanComplete(Adapter, status)
#define	WDI_IndicateBssList(pAdapter, status)
#define WDI_IndicateBssListBySSID(pAdapter,pSSID)
#define	WDI_IndicateConnectionComplete(Adapter, status)
#define	WDI_IndicateAssociationComplete(Adapter, status)
#define	WDI_IndicateDisassociation(Adapter, reason, pAddr)
#define	WDI_IndicateNLODiscovery(pAdapter)
#define	WDI_IndicateCurrentPhyStatus(Adapter)
#define	WDI_IndicateP2PEvent(pvP2PInfo, EventID, pInformation)
#define	WDI_CompleteCreateDeleteMac(pAdapter, status)				FALSE
#define	WDI_IndicateRoamingComplete(Adapter)
#define	WDI_IndicateLinkStateChanged(pAdapter, bForceLinkQuality, ucLinkQuality)
#define	WDI_IndicateActionFrame(pAdapter, posMpdu)				RT_STATUS_FAILURE
#define	WDI_IndicateWakeReason(pAdapter, bWakePacket, pBuffer, BufferLen)
#define	WDI_IndicateRoamingNeeded(pAdapter, IndicationReason)
#define	WDI_AddGroupPeer(pAdapter, PortNumber)
#define	WDI_DeleteGroupPeer(pAdapter, PortNumber)
#define	WDI_AddDatapathPeer(pAdapter, pAddr)						FALSE
#define	WDI_DeleteDatapathPeer(pAdapter, pAddr)
#define	WDI_FreeRxFrame(pAdapter, pNetBufferList)
#define	WDI_FetchNBLByPort(pAdapter, Port_ID, ppNetBufferList)
#define	WDI_IndicateTKIPMICFailure(pAdapter)
#define	WDI_TxCreditCheck(pAdapter)
#define	WDI_IndicateFWStalled(pAdapter)
#define	WDI_UpdateDefaultSetting(pAdapter)
#define	WDI_IndicateGeneralEvent(_pAdapter, _Event, _pInfoBuffer, _InfoBruuferLen)	RT_STATUS_NOT_SUPPORT

#endif

#endif
