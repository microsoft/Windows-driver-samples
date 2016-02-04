//---------------------------------------------------------------------------
//
// Copyright (c) 2013 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		P2P Service interface to other part of the driver
//

#ifndef __INC_P2P_SVC_H
#define __INC_P2P_SVC_H

#ifndef P2PSVC_SUPPORT
	#define	P2PSVC_SUPPORT				1
#endif

#if (P2P_SUPPORT != 1)
// Note:
//	P2P Services depends on P2P function.
#undef	P2PSVC_SUPPORT
#endif

#if (P2PSVC_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

BOOLEAN
P2PSvc_Enabled(
	IN  PVOID						pvP2PSvcInfo
	);

VOID
P2PSvc_Dump(
	IN	 PADAPTER							pAdapter
	);

RT_STATUS
P2PSvc_Request(
	IN	 PADAPTER							pAdapter,
	IN	 PVOID								infoBuf,
	IN	 u4Byte								inBufLen,
	IN	 u4Byte								outBufLen,
	OUT pu4Byte								pBytesWritten,
	OUT pu4Byte								pBytesRead,
	OUT pu4Byte								pBytesNeeded
	);

RT_STATUS
P2PSvc_AllocP2PSvcInfo(
	IN  PP2P_INFO 							pP2PInfo
	);

RT_STATUS
P2PSvc_Free_P2PSvcInfo(
	IN  PP2P_INFO 							pP2PInfo
	);

//-----------------------------------------------------------------------------
// Process probe event
//-----------------------------------------------------------------------------

RT_STATUS
P2PSvc_OnP2PScanComplete(
	IN  PVOID								pvP2PSvcInfo
	);

RT_STATUS
P2PSvc_OnDevDiscComplete(
	IN  PVOID								pvP2PSvcInfo
	);

RT_STATUS
P2PSvc_OnProbeRsp(
	IN	 PVOID								pvP2PSvcInfo,
	IN	 pu1Byte								devAddr,
	IN	 POCTET_STRING						posP2PAttrs
	);

RT_STATUS
P2PSvc_MakeProbeRspIE(
	IN  PVOID								pvP2PSvcInfo,
	IN	 POCTET_STRING						posP2PAttrs,
	OUT FRAME_BUF							*pBuf,
	OUT PBOOLEAN							pbToSendProbeRsp
	);

RT_STATUS
P2PSvc_MakeProbeReqIE(
	IN	 PVOID								pvP2PSvcInfo,
	OUT FRAME_BUF							*pBuf
	);

//-----------------------------------------------------------------------------
// Process SD event
//-----------------------------------------------------------------------------

RT_STATUS
P2PSvc_OnSDReq(
	IN	 PVOID								pvP2PSvcInfo,
	IN  pu1Byte								devAddr,
	IN  u1Byte								dlgToken,
	IN  u1Byte								SDReqRecvdSize, 
	IN  PVOID					 			pvSDReqRecvd,
	OUT PBOOLEAN							pbToSendSDRsp
	);

RT_STATUS
P2PSvc_OnSDRsp(
	IN	 PVOID								pvP2PSvcInfo,
	IN  pu1Byte								devAddr,
	IN  u1Byte								transactionId,
	IN  PVOID								pvSvcRspTlv, 
	OUT PBOOLEAN							pbNeedFurtherProcess
	);

//-----------------------------------------------------------------------------
// Process PD event
//-----------------------------------------------------------------------------

RT_STATUS
P2PSvc_OnPDRsp(
	IN  PVOID								pvP2PSvcInfo,
	IN	 pu1Byte								devAddr,
	IN  u2Byte								reqConfigMethod,
	IN  u2Byte								rspConfigMethod,
	IN	 POCTET_STRING						posP2PAttrs
	);

RT_STATUS
P2PSvc_OnPDReq(
	IN	 PVOID								pvP2PSvcInfo,
	IN	 pu1Byte								devAddr,
	IN  u2Byte								configMethod,
	IN	 POCTET_STRING						posP2PAttrs,
	OUT PBOOLEAN							bToSendPDRsp
	);

RT_STATUS
P2PSvc_OnPDRspSent(
	IN  PVOID						pvP2PSvcInfo,
	IN  BOOLEAN						bSendOk
	);

RT_STATUS
P2PSvc_MakePDReqIE(
	IN	 PVOID								pvP2PSvcInfo,
	OUT FRAME_BUF							*pBuf
	);

RT_STATUS
P2PSvc_MakePDRspIE(
	IN  PVOID						pvP2PSvcInfo,
	IN  POCTET_STRING 				posP2PAttrs,
	OUT FRAME_BUF					*pBuf
	);

RT_STATUS
P2PSvc_OnSendPDReqFailure(
	IN	 PVOID								pvP2PSvcInfo,
	IN	 pu1Byte								devAddr,
	IN  u1Byte								p2pStatus
	);

RT_STATUS
P2PSvc_OnWatchdog(
	IN	 PVOID								pvP2PSvcInfo
	);

RT_STATUS
P2PSvc_OnDisconnect(
	IN	 PVOID								pvP2PSvcInfo
	);

//-----------------------------------------------------------------------------
// Override default P2P behavior
//-----------------------------------------------------------------------------

RT_STATUS
P2PSvc_DetermineGOSSID(
	IN  PADAPTER							pAdapter,
	OUT pu1Byte 							ssidBuf,
	OUT pu1Byte 							pSsidLen
	);

#else

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

#define P2PSvc_Enabled(__pvP2PSvcInfo) FALSE

#define P2PSvc_Dump(__pAdapter)

#define P2PSvc_Request(__pAdapter, __infoBuf, __inBufLen, __outBufLen, __pBytesWritten, __pBytesRead, __pBytesNeeded) RT_STATUS_SUCCESS

#define P2PSvc_AllocP2PSvcInfo(__pP2PInfo) RT_STATUS_SUCCESS

#define P2PSvc_Free_P2PSvcInfo(__pP2PInfo) RT_STATUS_SUCCESS

//-----------------------------------------------------------------------------
// Process probe event
//-----------------------------------------------------------------------------

#define P2PSvc_OnP2PScanComplete(__pvP2PSvcInfo) RT_STATUS_SUCCESS

#define P2PSvc_OnDevDiscComplete(__pvP2PSvcInfo) RT_STATUS_SUCCESS

#define P2PSvc_OnProbeRsp(__pvP2PSvcInfo, __devAddr, __posP2PAttrs) RT_STATUS_SUCCESS

#define P2PSvc_MakeProbeRspIE(__pvP2PSvcInfo, __posP2PAttrs, __pBuf, __pbToSendProbeRsp) RT_STATUS_SUCCESS

#define P2PSvc_MakeProbeReqIE(__pvP2PSvcInfo, __pBuf) RT_STATUS_SUCCESS

//-----------------------------------------------------------------------------
// Process SD event
//-----------------------------------------------------------------------------

#define P2PSvc_OnSDReq(__pvP2PSvcInfo, __devAddr, __dlgToken, __SDReqRecvdSize, __pvSDReqRecvd, __pbToSendSDRsp) RT_STATUS_SUCCESS

#define P2PSvc_OnSDRsp(__pvP2PSvcInfo, __devAddr, __transactionId, __pvSvcRspTlv, __pbNeedFurtherProcess) RT_STATUS_SUCCESS

//-----------------------------------------------------------------------------
// Process PD event
//-----------------------------------------------------------------------------

#define P2PSvc_OnPDRsp(__pvP2PSvcInfo, __devAddr, __reqConfigMethod, __rspConfigMethod, __posP2PAttrs) RT_STATUS_SUCCESS

#define P2PSvc_OnPDReq(__pvP2PSvcInfo, __devAddr, __conifgMethod, __posP2PAttrs, __pbToSendPDRsp) RT_STATUS_SUCCESS

#define P2PSvc_OnPDRspSent(__pvP2PSvcInfo, __bSendOk) RT_STATUS_SUCCESS

#define P2PSvc_MakePDReqIE(__pvP2PSvcInfo, __pBuf) RT_STATUS_SUCCESS

#define P2PSvc_MakePDRspIE(__pvP2PSvcInfo, __posP2PAttrs, __pBuf) RT_STATUS_SUCCESS

#define P2PSvc_OnSendPDReqFailure(__pvP2PSvcInfo,__devAddr, __p2pStatus) RT_STATUS_SUCCESS

#define P2PSvc_OnWatchdog(__pvP2PSvcInfo) RT_STATUS_SUCCESS

#define P2PSvc_OnDisconnect(__pvP2PSvcInfo) RT_STATUS_SUCCESS

//-----------------------------------------------------------------------------
// Override default P2P behavior
//-----------------------------------------------------------------------------

#define P2PSvc_DetermineGOSSID(__pAdapter, __ssidBuf, __pSsidLen) RT_STATUS_SUCCESS

#endif // #if (P2PSVC_SUPPORT == 1)

#endif	// #ifndef __INC_P2P_SVC_H
