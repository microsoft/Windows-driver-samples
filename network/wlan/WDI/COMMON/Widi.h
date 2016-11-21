#ifndef __INC_WIDI_11_H
#define __INC_WIDI_11_H

#if (P2P_SUPPORT != 1)
// Note:
//	WiFi Display depends on P2P function.
#undef	WIDI_SUPPORT
#endif

#if (WIDI_SUPPORT == 1)

RT_STATUS
WFD_AppendProbeReqIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendAssocReqIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendAssocRspIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendProbeRspIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu,
	IN	PRT_RFD			pRfdProbeReq,
	IN	POCTET_STRING	posProbeReq
	);

RT_STATUS
WFD_AppendTunneledProbeRspIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendP2pGoNegReqIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendP2pGoNegRspIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendP2pGoNegConfirmIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendP2pInvitationReqIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendP2pInvitationRspIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendP2pProvDiscoveryReqIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendP2pProvDiscoveryRspIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendBeaconIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

RT_STATUS
WFD_AppendTDLSSetupIEs(
	IN	PADAPTER		pAdapter,
	IN	u4Byte			maxBufLen,
	OUT POCTET_STRING	posMsdu
	);

VOID 
WFD_ScanListClear(
	IN	PADAPTER	pAdapter
	);

RT_STATUS
WFD_AllocateWfdInfo(
	IN	PADAPTER	pAdapter
	);

VOID
WFD_FreeWfdInfo(
	IN	PADAPTER	pAdapter
	);

RT_STATUS
WFD_Query_Reqeust(
	IN	PADAPTER	pAdapter,
	IN	PVOID		informationBuffer,
	IN	ULONG		inputBufferLength,
	OUT	PULONG		pBytesWritten,
	OUT	PULONG		pBytesNeeded
	);

RT_STATUS
WFD_Reqeust(
	IN	PADAPTER	pAdapter,
	IN	PVOID		informationBuffer,
	IN	u4Byte		inputBufferLength,
	IN	u4Byte		outputBufferLength,
	OUT	pu4Byte		pBytesWritten,
	OUT	pu4Byte		pBytesRead,
	OUT	pu4Byte		pBytesNeeded
	);

RT_STATUS
WFD_OnBeacon(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
WFD_OnProbeRsp(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
WFD_OnAssocReqAccept(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
WFD_OnAssocOK(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

VOID
WFD_RemoveAssocClient(
	IN	PADAPTER		pAdapter,
	IN	pu1Byte			pDeviceAddr
	);

RT_STATUS
WFD_OnP2PActionFrame(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu
	);

RT_STATUS
WFD_OnTunneledMgntFrame(
	IN	PADAPTER		pAdapter,
	IN	PRT_RFD			pRfd,
	IN	POCTET_STRING	posMpdu,
	IN	u4Byte			ieOffset
	);

RT_STATUS
WFD_GetInfoOnIEs(
	IN			PADAPTER		pAdapter,
	IN			u4Byte			InfoID,
	IN	const 	POCTET_STRING	pOsIEs,
	OUT			pu1Byte			pOutputBuffer,
	IN OUT		pu4Byte			pOutputBufferLen
	);
#else // #if (WIDI_SUPPORT != 1)

#define	WFD_AppendProbeReqIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendAssocReqIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendAssocRspIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendProbeRspIEs(_pAdapter, _maxBufLen, _posMsdu, pRfdProbeReq, posProbeReq)	RT_STATUS_SUCCESS
#define	WFD_AppendTunneledProbeRspIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendP2pGoNegReqIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendP2pGoNegRspIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendP2pGoNegConfirmIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendP2pInvitationReqIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendP2pInvitationRspIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendP2pProvDiscoveryReqIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendP2pProvDiscoveryRspIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendBeaconIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_AppendTDLSSetupIEs(_pAdapter, _maxBufLen, _posMsdu)	RT_STATUS_SUCCESS
#define	WFD_ScanListClear(_pAdapter)
#define	WFD_AllocateWfdInfo(_pAdapter)	RT_STATUS_SUCCESS
#define	WFD_FreeWfdInfo(_pAdapter)
#define	WFD_Query_Reqeust(_pAdapter, _informationBuffer, _inputBufferLength, _pBytesWritten, _pBytesNeeded)	RT_STATUS_NOT_SUPPORT
#define	WFD_Reqeust(_pAdapter, _informationBuffer, _inputBufferLength, _outputBufferLength,	\
					_pBytesWritten, _pBytesRead, _pBytesNeeded)	\
					RT_STATUS_NOT_SUPPORT
#define	WFD_OnBeacon(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	WFD_OnProbeRsp(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	WFD_OnAssocReqAccept(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	WFD_OnAssocOK(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	WFD_OnP2PActionFrame(_pAdapter, _pRfd, _posMpdu)	RT_STATUS_SUCCESS
#define	WFD_OnTunneledMgntFrame(_pAdapter, _pRfd, _posMpdu, _ieOffset)	RT_STATUS_SUCCESS
#define	WFD_RemoveAssocClient(_pAdapter, _pDeviceAddr)
#define	WFD_GetInfoOnIEs(_pAdapter, _InfoID, _pOsIEs, _pOutputBuffer, _pOutputBufferLen)	RT_STATUS_NOT_SUPPORT
#endif // #if (WIDI_SUPPORT == 1)
#endif	// #ifndef __INC_WIDI_11_H
