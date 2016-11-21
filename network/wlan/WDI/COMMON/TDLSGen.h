#ifndef	__INC_TDLSGEN_H
#define	__INC_TDLSGEN_H

#if (TDLS_SUPPORT == 1)
VOID
TDLS_Init(
	PADAPTER		Adapter
	);

VOID
TDLS_Reset(
	PADAPTER		Adapter
	);

VOID
TDLS_QueryCapability(
	PADAPTER	Adapter,
	PRT_TCB		pTcb
	);

BOOLEAN
TDLS_IndicatePacket(
	PADAPTER		Adapter,
	POCTET_STRING	pPduOS
	);

BOOLEAN
TDLS_IsRxTDLSPacket(
	PADAPTER	Adapter,
	PRT_RFD 		pRfd
	);

BOOLEAN
TDLS_IsTxTDLSPacket(
	PADAPTER	Adapter,
	PRT_RFD 		pRfd
	);

VOID
TDLS_OnAsocOK(
	PADAPTER		Adapter,
	OCTET_STRING	asocpdu
	);

VOID
TDLS_OnAddBaRsp(
	PADAPTER		Adapter,
	POCTET_STRING	pPduOS
	);

VOID
TDLS_OnBeacon_BSS(
	PADAPTER			pAdapter,
	OCTET_STRING		osBeacon
	);

PRT_WLAN_STA
TDLS_PS_CheckPsTx(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	);

BOOLEAN
TDLS_PS_OnTx(
	PADAPTER		pAdapter,
	PRT_WLAN_STA	pEntry,
	PRT_TCB			pTcb
	);

PRT_WLAN_STA
TDLS_PS_UpdatePeerPSState(
	PADAPTER		Adapter,
	POCTET_STRING	pPduOS,
	BOOLEAN			bTriggerFrame
	);

VOID
TDLS_PS_Sleep(
	PADAPTER			Adapter,
	RT_TDLS_PS_STATE	StateToSet
	);

BOOLEAN
TDLS_CS_BufferCSTx(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	);

BOOLEAN
TDLS_PrepareTxFeedback(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	);
	
VOID
TDLS_UpdatePeer(
	PADAPTER		Adapter,
	PRT_WLAN_STA	pEntry
	);

VOID
TDLS_UpdatePeerStatus(
	PADAPTER		Adapter
	);

BOOLEAN
TDLS_GetPwrMgntInfo(
	PADAPTER		Adapter,
	PRT_TCB			pTcb
	);

VOID
TDLS_SetConfiguration(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			InformationBuffer,
	IN	u4Byte			InformationBufferLength
	);

BOOLEAN
TDLS_CheckPeer(
	PADAPTER		Adapter,
	pu1Byte			Addr
	);

VOID
TDLS_RemovePeer(
	PADAPTER		Adapter,
	pu1Byte			Addr
	);

VOID
TDLS_LinkStatusWatchDog(
	PADAPTER		Adapter
	);

VOID
TDLS_Stop(
	PADAPTER		Adapter
	);

VOID
TDLS_Release(
	PADAPTER		Adapter
	);

RT_STATUS
TDLS_AllocateMemory(
	IN	PADAPTER		Adapter
	);

VOID
TDLS_FreeMemory(
	IN	PADAPTER		Adapter
	);

RT_STATUS
TDLS_OnDiscoveryRsp(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS
	);

RT_STATUS
TDLS_OnSetupReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnSetupRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnSetupConfirm(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnTearDown(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnDiscoveryReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnTrafficInd(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnTrafficRsp(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnChnlSwitchReq(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnChnlSwitchRsp(
	IN	PADAPTER		Adapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnTunneledProbeReq(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

RT_STATUS
TDLS_OnTunneledProbeRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	pPduOS,
	IN	u4Byte			contentOffset
    );

VOID
TDLS_CountTxStatistics(
	IN	PADAPTER		Adapter,
	IN	PRT_TCB 			pTcb
	);

VOID
TDLS_GetTxMacId(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr,
	OUT	pu4Byte			MacID
	);

VOID
TDLS_GetHtAMPDU(	
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr,
	IN	PRT_TCB			pTcb
	);

VOID
TDLS_GetTxRatrIndex(
	IN	PADAPTER		Adapter,
	IN	pu1Byte			Addr,
	OUT	pu1Byte			RatrIndex
	);

VOID
TDLS_CheckCapability(
	IN	PADAPTER		Adapter
	);

VOID
TDLS_GetValueFromBeaconOrProbeRsp(
	IN	PADAPTER			Adapter,
	IN	POCTET_STRING		pMmpdu,
	IN	PRT_WLAN_BSS		pBssDesc
	);

VOID
TDLS_RxTranslateHeader(
	IN	PADAPTER			Adapter,
	IN	PRT_RFD				pRfd
	);

VOID
TDLS_TxTranslateHeader(
	IN	PADAPTER			Adapter,
	IN	PRT_TCB				pTcb,
	IN	PROTOCOL_TYPE		Type
	);

VOID
TDLS_CheckAggregation(
	IN	PADAPTER			Adapter,
	IN	pu1Byte				Addr,
	IN	PRT_TCB				pTcb
	);

#else // #if (TDLS_SUPPORT != 1)
#define TDLS_Init(Adapter)
#define TDLS_Reset(Adapter)
#define TDLS_QueryCapability(Adapter, pTcb)
#define TDLS_IndicatePacket(Adapter, pPduOS)						(FALSE)
#define TDLS_IsRxTDLSPacket(Adapter, pRfd)							(FALSE)
#define TDLS_IsTxTDLSPacket(Adapter, pTcb)							(FALSE)
#define TDLS_OnAsocOK(Adapter, asocpdu)
#define TDLS_OnAddBaRsp(Adapter, pPduOS)
#define TDLS_OnBeacon_BSS(pAdapter, osBeacon)
#define TDLS_PS_CheckPsTx(Adapter, pTcb)							(NULL)
#define TDLS_PS_OnTx(pAdapter, pEntry, pTcb)						(FALSE)
#define TDLS_PS_UpdatePeerPSState(Adapter, pPduOS, bTriggerFrame)	(NULL)
#define TDLS_PS_Sleep(Adapter, StateToSet)
#define TDLS_CS_BufferCSTx(Adapter, pTcb)							(FALSE)
#define TDLS_PrepareTxFeedback(Adapter, pTcb)						(FALSE)
#define TDLS_UpdatePeer(Adapter, pEntry)
#define TDLS_UpdatePeerStatus(Adapter)
#define TDLS_GetPwrMgntInfo(Adapter,pTcb)							(FALSE)
#define TDLS_SetConfiguration(Adapter, InformationBuffer, InformationBufferLength)
#define TDLS_CheckPeer(Adapter, Addr)								(FALSE)
#define TDLS_RemovePeer(Adapter,Addr)
#define TDLS_LinkStatusWatchDog(Adapter)
#define TDLS_Stop(Adapter)
#define TDLS_Release(Adapter)
#define TDLS_AllocateMemory(Adapter)								(RT_STATUS_SUCCESS)
#define TDLS_FreeMemory(Adapter)
#define TDLS_ResetAsocEntry(_pAdapter, _pEntry)
#define TDLS_CountTxStatistics(_pAdapter, _pTcb)
#define TDLS_GetTxMacId(_pAdapter, _Addr, _MacID)
#define TDLS_GetHtAMPDU(_pAdapter, _Addr, _pTcb)
#define TDLS_GetTxRatrIndex(_pAdapter, _Addr, _RatrIndex)
#define TDLS_CheckCapability(_pAdapter)
#define TDLS_GetValueFromBeaconOrProbeRsp(_pAdapter, _pMmpdu, _pBssDesc)
#define TDLS_RxTranslateHeader(_pAdapter, _pRfd)
#define TDLS_TxTranslateHeader(_pAdapter, _pTcb, _Type)
#define TDLS_CheckAggregation(_pAdapter, _Addr, _pTcb)

__inline RT_STATUS TDLS_OnDiscoveryRsp(PADAPTER Adapter, PRT_RFD pRfd, POCTET_STRING pPduOS) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnSetupReq(PADAPTER pAdapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnSetupRsp(PADAPTER pAdapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnSetupConfirm(PADAPTER pAdapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnTearDown(PADAPTER Adapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnDiscoveryReq(PADAPTER Adapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnTrafficInd(PADAPTER Adapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnTrafficRsp(PADAPTER	Adapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte	contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnChnlSwitchReq(PADAPTER Adapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnChnlSwitchRsp(PADAPTER Adapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnTunneledProbeReq(PADAPTER pAdapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}
__inline RT_STATUS TDLS_OnTunneledProbeRsp(PADAPTER pAdapter, PRT_RFD pRfd, POCTET_STRING pPduOS, u4Byte contentOffset) {return RT_STATUS_SUCCESS;}

#define	GET_TDLS_ENABLED(__pMgntInfo)			FALSE
#define	SET_TDLS_ENABLED(__pMgntInfo, _value)		
#define	GET_TDLS_WIFITEST(__pMgntInfo)			FALSE
#define	IS_TDL_EXIST(_pMgntInfo)					FALSE
#define	GET_TDLS_WIFITESTBED_RADIO_OFF(__pMgntInfo)			FALSE

#endif	// end of else of #if (TDLS_SUPPORT == 1)

	
#endif
