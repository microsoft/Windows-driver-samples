//---------------------------------------------------------------------------
//
// Copyright (c) 2013 Realtek Semiconductor, Inc. All rights reserved.
// 
//---------------------------------------------------------------------------
// Description:
//		P2P Service interface for internal implementation
//

//
// The P2PSvc code is arranged as the following (regarding to visibility):
//
// --------------------------------
//     Other part of the driver
// --------------------------------
//     P2PSvc.h
// --------------------------------
//     P2PSvc.c
// --------------------------------
//     P2PSvc_Internal.h
// --------------------------------
//  ActionInfo, Construct, Object, ParamSpec, PD, SD, SearchResult,... 
// 
// P2PSvcType.h is visible to all P2PSvc modules.
//


#ifndef __INC_P2P_SVC_INTERNAL_H
#define __INC_P2P_SVC_INTERNAL_H

#include "P2PSvcType.h"
#include "p2p.h"
#include "P2P_Internal.h"

#if (P2P_SUPPORT != 1)
// Note:
//	P2P Services depends on P2P function.
#undef	P2PSVC_SUPPORT
#endif

#define P2PSVC_DATA_SEC
#define P2PSVC_LOCAL_FUNC_SEC
#define P2PSVC_PUBLIC_FUNC_SEC

#define P2PSVC_LEAST_PROBE_RSP_TIME_uS				1000000		// 1000 ms
#define P2PSVC_LEAST_SD_RSP_TIME_uS					300000		// 300 ms
#define P2PSVC_TIMEOUT_WAIT_SD_RSP_uS				500000		// 1000 ms
#define P2PSVC_TIMEOUT_PD_SESSION_TIMEOUT_uS		120000000	// 120000 ms

typedef struct _P2PSVC_INFO
{
	u4Byte							curVer;
	BOOLEAN 						bEnabled;
	RT_SPIN_LOCK					lock;

	// A list of P2PSVC_REQ_INFO_ENTRY each 
	// representing an advertised service
	RT_LIST_ENTRY					advSvcList;
	u4Byte							advSvcListCnt;

	// A list of P2PSVC_REQ_INFO_ENTRY each 
	// representing a seek request
	RT_LIST_ENTRY					seekReqList;
	u4Byte							seekReqListCnt;

	// A list of P2PSVC_SEARCH_RESULT_LIST_ENTRY each 
	// representing the search result of a device
	RT_LIST_ENTRY					searchResultList;
	u4Byte							searchResultListCnt;

	// A list of P2PSVC_PD_ENTRY each 
	// representing a provision discovery session
	RT_LIST_ENTRY					pdSessionList;
	u4Byte							pdSessionListCnt;

	// The SSID that will be used for starting 
	// P2P Group
	u1Byte							goSsidBuf[32];
	u1Byte							goSsidLen;

	// Connection capability bitmap, bit0: new, bit1: cli, bit2: GO
	u1Byte							connCap;

	// The adapter this P2PSVC_INFO is attached to
	PADAPTER						pAdapter;
}P2PSVC_INFO, *PP2PSVC_INFO;

#define P2PSVC_GET_INFO(__pAdapter) ((PP2PSVC_INFO)((GET_P2P_INFO((__pAdapter)))->pP2PSvcInfo))
#define P2PSVC_ENABLED(__pP2PSvcInfo) (((PP2PSVC_INFO)(__pP2PSvcInfo))->bEnabled)

// P2PSVC request handler prototype
typedef RT_STATUS (*P2PSvc_ReqHdlr)(
	IN  PADAPTER			pAdapter,
	IN  PVOID				infoBuf,
	IN  u4Byte				inBufLen,
	IN  u4Byte				outBufLen,
	OUT pu4Byte				pBytesWritten,
	OUT pu4Byte				pBytesRead,
	OUT pu4Byte				pBytesNeeded
	);

typedef struct _P2PSVC_REQUEST_ID_MAP
{
	const u4Byte					type;				// Request Type
	const u4Byte					id;					// Request ID
	const u4Byte					minVer;				// The minimum supported version, 0 means don't care.
	const u4Byte					maxVer;				// The maxmum supported version, 0 means don't care.
	const P2PSvc_ReqHdlr 			reqHdlr;			// Handler function for this combination of Request ID, bSet, minVer, and maxVer.
}P2PSVC_REQUEST_ID_MAP, *PP2PSVC_REQUEST_ID_MAP;

typedef struct _P2PSVC_REQ_INFO_ENTRY
{
	RT_LIST_ENTRY 					List;
	u4Byte							P2PSvcReqId;
	P2PSVC_OBJ_LIST					objList;
}P2PSVC_REQ_INFO_ENTRY, *PP2PSVC_REQ_INFO_ENTRY;

#define P2PSVC_REQ_INFO_LIST_LEN(__pReqInfo) \
	(FIELD_OFFSET(P2PSVC_REQ_INFO_ENTRY, objList) + P2PSVC_OBJ_LIST_LEN(&((__pReqInfo)->objList)))

#define P2PSVC_MAX_ACTION_PARAM_ID		P2PSVC_MAX_OBJ_LIST_ENTRIES - 1 	// minus 1 for sentinel
typedef struct _P2PSVC_ACTION_SPEC_ENTRY
{
	u4Byte							actId;
	u4Byte							paramList[P2PSVC_MAX_OBJ_LIST_ENTRIES]; // reserve 1 for sentinel
}P2PSVC_ACTION_SPEC_ENTRY, *PP2PSVC_ACTION_SPEC_ENTRY;

typedef struct _P2PSVC_PARAM_SPEC_ENTRY
{
	u4Byte							paramId;
	u4Byte							minLen;
	u4Byte							maxLen;
}P2PSVC_PARAM_SPEC_ENTRY, *PP2PSVC_PARAM_SPEC_ENTRY;

typedef struct _P2PSVC_SR_LIST_ENTRY
{
	RT_LIST_ENTRY 					List;
	u8Byte							probeRspRxTime;
	u8Byte							sdReqTxTime;
	u8Byte							sdRspRxTime;
	BOOLEAN							bDirty;
	P2PSVC_OBJ_LIST					srObjList;
}P2PSVC_SR_LIST_ENTRY, *PP2PSVC_SR_LIST_ENTRY;

#define P2PSVC_SR_LIST_LEN(__srEntry) \
	(FIELD_OFFSET(P2PSVC_SR_LIST_ENTRY, srObjList) + P2PSVC_OBJ_LIST_VAR_LEN(&((__srEntry)->srObjList)))

typedef struct _P2PSVC_GROUP_INFO
{
	u1Byte							grpDevAddr[6];
	u1Byte							grpIntfAddr[6];
	u1Byte							ssidLen;
	u1Byte							ssidBuf[32];
	u1Byte							opChnl;
}P2PSVC_GROUP_INFO, *PP2PSVC_GROUP_INFO;

typedef struct _P2PSVC_PD_CONN_TOPOLOGY
{
	BOOLEAN							bInitor;
	BOOLEAN							bDeferred;
	BOOLEAN							bPersistent;
	u4Byte							connAction;
	P2PSVC_GROUP_INFO				grpInfo;
}P2PSVC_PD_CONN_TOPOLOGY, *PP2PSVC_PD_CONN_TOPOLOGY;

typedef struct _P2PSVC_PD_ENTRY
{
	RT_LIST_ENTRY 					List;

	BOOLEAN							bInitor;
	BOOLEAN							bDeferred;
	u8Byte							createTime;

	//
	// If bDone, we have status from the last PD rsp,
	// and if the status is success, we have the topology,
	// which records how we are going to connect with the 
	// peer dev.
	//
	BOOLEAN							bDone;
	P2P_STATUS_CODE					status;
	u1Byte							connCap;			// the resulting connCap
	P2PSVC_PD_CONN_TOPOLOGY			connTopology;

	u4Byte							sessionId;
	u1Byte							peerDevAddr[6];
	u4Byte							advId;

	// If initor, the conn cap sent in the PD req
	// If rspdor, the conn cap in the recvd PD req
	u1Byte							pdReqConnCap;

	// The connCap from P2PSvcInfo.
	// Filled when session is created.
	u1Byte							selfConnCap;

	P2PSVC_GROUP_INFO				selfGrpInfo;

}P2PSVC_PD_ENTRY, *PP2PSVC_PD_ENTRY;

typedef struct _P2PSVC_INITOR_PD_ENTRY
{
	// Common info shared by all PD sessions
	P2PSVC_PD_ENTRY					super;

	// PD req info
	u2Byte							pdReqConfigMethod;

	// PD rsp info
	u8Byte							pdRspRxTime;		// time the initor recvs the PD rsp
	P2P_STATUS_CODE					pdRspStatus;		// the status in PD rsp
	u1Byte							pdRspConnCap;		// conn cap in recvd PD rsp

	// FOPD info, valid if bDeferred is true
	u8Byte							fopdReqRxTime;		// time the initor recvs the FOPD req
	P2P_STATUS_CODE					fopdReqStatus;		// the status in FOPD req, shall be either accept or reject
	u1Byte							fopdReqConnCap;		// conn cap in recvd FOPD req, valid only when FOPD req status is success

	P2P_STATUS_CODE					fopdRspStatus;		// the status to be sent in FOPD rsp
	u1Byte							fopdRspConnCap;		// the conn cap to be sent in FOPD rsp, valid only when FOPD rsp status is success

	P2PSVC_REASON					rxBadPDRspReason;
	P2PSVC_REASON					rxBadFOPDReqReason;

	// If initor, this member contains the obj list from the upper layer (P2PSvc_AddPDInitorData)
	P2PSVC_OBJ_LIST					objList;
}P2PSVC_INITOR_PD_ENTRY, *PP2PSVC_INITOR_PD_ENTRY;

typedef struct _P2PSVC_RSPDOR_PD_ENTRY
{
	// Common info shared by all PD sessions
	P2PSVC_PD_ENTRY					super;

	// PD req info
	u2Byte							configMethod;		// config method from PD req
	u8Byte							pdReqRxTime;		// time the rspdor recvs the PD req

	// PD rsp info
	P2P_STATUS_CODE					pdRspStatus;		// the status to be sent in PD rsp
	u1Byte							pdRspConnCap;		// the conn cap to be sent in PD rsp, valid only when PD rsp is success

	// FOPD info, valid if bDeferred is true
	u8Byte							fopdRspRxTime;		// time the rspdor recvs the FOPD rsp

	P2P_STATUS_CODE					fopdReqStatus;		

	// the status to be sent in FOPD req
	u1Byte							fopdReqConnCap;		// the conn cap to be sent in FOPD req, valid only whe FOPD req status is success

	P2P_STATUS_CODE					fopdRspStatus;		// the status in recvd FOPD rsp
	u1Byte							fopdRspConnCap;		// the conn cap in recvd FOPD rsp, valid only when FOPD rsp status is success

	P2PSVC_REASON					rxBadPDReqReason;
	P2PSVC_REASON					rxBadFOPDRspReason;

	// If rspdor, this member contains the info parsed from thd requested svc (P2PSvc_AddPDRspdorData)
	P2PSVC_OBJ_LIST					objList;
}P2PSVC_RSPDOR_PD_ENTRY, *PP2PSVC_RSPDOR_PD_ENTRY;

typedef struct _P2PSVC_CONN_CAP_MAP_ENTRY
{
	u1Byte 							reqConnCap;
	u1Byte							selfConnCap;
	u1Byte							rspConnCap;
	pu1Byte							pDesc;
}P2PSVC_CONN_CAP_MAP_ENTRY, *PP2PSVC_CONN_CAP_MAP_ENTRY;

//-----------------------------------------------------------------------------
// P2PSvc.c
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// P2PSvc_Utility.c
//-----------------------------------------------------------------------------

#define P2PSVC_FUNC_IN(__level) RT_TRACE(COMP_P2P, (__level), ("===> %s\n",  __FUNCTION__))
#define P2PSVC_FUNC_OUT(__level, __rtStatus) RT_TRACE(COMP_P2P, (__level), ("<=== %s(0x%08x)\n",  __FUNCTION__, (__rtStatus)))
#define P2PSVC_CHECK_NULL(__ptr) RT_ASSERT((__ptr), ("%s(): %s is NULL!!!\n", __FUNCTION__, #__ptr));

extern u4Byte gP2PSvcMemAllocCount;
extern u4Byte gP2PSvcMemFreeCount;

RT_STATUS
P2PSvc_AllocMem(
	IN  PVOID						Adapter,
	OUT PVOID						*ppPtr,
	IN  u4Byte						len
	);

VOID
P2PSvc_FreeMem(
	IN  PVOID						pPtr,
	IN  u4Byte						len
	);

RT_STATUS
P2PSvc_GetAdvSvcByAdvId(
	IN  PP2PSVC_INFO				pP2PSvcInfo, 
	IN  u4Byte						advId,
	OUT PP2PSVC_REQ_INFO_ENTRY 		*ppAdvSvcEntry
	);

BOOLEAN
P2PSvc_GetP2PAttr(
	IN  POCTET_STRING				posP2PAttrs,
	IN  u1Byte						attrId,
	IN  u4Byte						minLen,
	OUT POCTET_STRING				posAttr
	);

RT_STATUS
P2PSvc_Indicate(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  u4Byte						bufLen,
	IN  PVOID						pvBuf
	);

RT_STATUS
P2PSvc_IndicateFrameSent(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN  u4Byte								id,
	IN  pu1Byte								peerMac,
	IN  u2Byte								attrLen,
	IN  pu1Byte								pAttrs
	);

RT_STATUS
P2PSvc_Notify(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  u4Byte						id
	);

BOOLEAN
P2PSvc_SeekReqIsBcst(
	IN  PP2PSVC_OBJ_LIST			pObjList
	);

BOOLEAN
P2PSvc_MatchSubstring(
	IN  pu1Byte						targetBuf,
	IN  u4Byte						targetBufLen,
	IN  pu1Byte						patternBuf,
	IN  u4Byte						patternBufLen
	);

RT_STATUS
P2PSvc_GetDevNameFromDevInfoAttr(
	IN  POCTET_STRING				posP2PAttrs,
	OUT POCTET_STRING				posDevName
	);

//-----------------------------------------------------------------------------
// P2PSvc_Object.c
//-----------------------------------------------------------------------------

u4Byte
P2PSvc_MakeObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						id,
	IN  u4Byte						bufLEn,
	IN  PVOID						pBuf
	);

u4Byte
P2PSvc_MakeSvcDescObjList(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	OUT PP2PSVC_OBJ_LIST			*ppSvcDescObjList
	);

u4Byte
P2PSvc_MakeTimeObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						paramId,
	IN  u8Byte				 		time
	);

u4Byte
P2PSvc_MakeSearchIdObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  PP2PSVC_REQ_INFO_ENTRY 		pSeekInfoEntry,
	OUT pu1Byte						pSearchId
	);

u4Byte
P2PSvc_MakeDevAddrObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  pu1Byte						pDevAddr
	);

u4Byte
P2PSvc_MakeDevNameObj_FromP2PAttrs(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  POCTET_STRING				posP2PAttrs
	);

u4Byte
P2PSvc_MakeDevNameObj_FromDevNameOctet(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  OCTET_STRING				osDevName
	);

u4Byte
P2PSvc_MakeAdvIdObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						advId
	);

u4Byte
P2PSvc_MakeSessionIdObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						sessionId
	);

u4Byte
P2PSvc_MakeSvcNameObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte						svcNameBufLen,
	IN  pu1Byte						pSvcNameBuf
	);

u4Byte
P2PSvc_MakeSvcInfoObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u2Byte						svcInfoBufLen,
	IN  pu1Byte						pSvcInfoBuf
	);

u4Byte
P2PSvc_MakeAutoAcceptObj(
	IN  PP2PSVC_OBJ_LIST 			pObjList,
	IN  BOOLEAN						bAutoAccept
	);

u4Byte
P2PSvc_MakeSvcStatusObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte						svcStatus
	);

u4Byte
P2PSvc_MakeUserAcceptedObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  BOOLEAN						bUserAccepted
	);

u4Byte
P2PSvc_MakeSvcCountObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte						svcCount
	);

u4Byte
P2PSvc_MakeP2PStatusObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  P2P_STATUS_CODE 			P2PStatus
	);

u4Byte
P2PSvc_MakeSsidObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte 						ssidLen,
	IN  pu1Byte						ssidBuf
	);

u4Byte
P2PSvc_MakeIntfAddrObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  pu1Byte						intfAddr
	);

u4Byte
P2PSvc_MakeOpChannelObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u1Byte		 				opChannel
	);

u4Byte
P2PSvc_MakeConfigMethodObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u2Byte 						configMethod
	);

u4Byte
P2PSvc_MakeP2PAttrsObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u2Byte						attrLen,
	IN  pu1Byte						pAttrs
	);

u4Byte
P2PSvc_MakeDeferredObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  BOOLEAN						bDeferred
	);

u4Byte
P2PSvc_MakeIsSDDoneObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  BOOLEAN						bSDDone
	);


u4Byte
P2PSvc_MakeIsPersistentObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  BOOLEAN						bPersistent
	);

u4Byte
P2PSvc_MakeConnActionObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte 						connAction
	);

u4Byte
P2PSvc_MakeReasonObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  P2PSVC_REASON 				reason
	);

u4Byte
P2PSvc_MakeSessionInfoDataInfoObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  OCTET_STRING				osSessionInfoDataInfo
	);

u4Byte
P2PSvc_MakeGrpDevAddrObj(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  pu1Byte						pGrpDevAddr
	);

//-----------------------------------------------------------------------------
// P2PSvc_Construct.c
//-----------------------------------------------------------------------------

RT_STATUS
P2PSvc_MakeAdvSvcInfo(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  POCTET_STRING				posSvcNameHash,
	OUT FRAME_BUF					*pBuf
	);

RT_STATUS
P2PSvc_MakeSvcNameHash(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	OUT FRAME_BUF					*pBuf
	);

RT_STATUS
P2PSvc_MakeInitorPDReqIE(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	OUT FRAME_BUF					*pBuf
	);

RT_STATUS
P2PSvc_MakeRspdorFOPDReqIE(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	OUT FRAME_BUF					*pBuf
	);

RT_STATUS
P2PSvc_MakeInitorFOPDRspIE(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	IN  POCTET_STRING 				posP2PAttrs,
	OUT FRAME_BUF					*pBuf
	);

RT_STATUS
P2PSvc_MakeRspdorPDRspIE(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	IN  POCTET_STRING 				posP2PAttrs,
	OUT FRAME_BUF					*pBuf
	);

RT_STATUS
P2PSvc_MakeSvcInfoDesc(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_REQ_INFO_ENTRY		pAdvInfoEntry,
	IN  u1Byte						svcInfoReqLen,
	IN  pu1Byte						pSvcInfoReqBuf,
	OUT FRAME_BUF					*pBuf
	);

//-----------------------------------------------------------------------------
// P2PSvc_ParamSpec.c
//-----------------------------------------------------------------------------

PP2PSVC_PARAM_SPEC_ENTRY
P2PSvc_GetParamSpec(
	IN  u4Byte						actId
	);

RT_STATUS 
P2PSvc_ValidateReqInfo(
	IN  PVOID						infoBuf,
	IN  u4Byte						inBufLen,
	OUT pu4Byte						pBytesNeeded
	);

RT_STATUS
P2PSvc_ValidateActionParam(
	IN  u4Byte						id,
	IN  PP2PSVC_OBJ_LIST			pObjList
	);

RT_STATUS
P2PSvc_ValidateParamContent(
	IN  PP2PSVC_OBJ_LIST			pObjList
	);

PRT_OBJECT_HEADER
P2PSvc_GetParam(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						paramId,
	IN  u4Byte						fromIdx
	);

RT_STATUS
P2PSvc_UpdateParam(
	IN  PP2PSVC_OBJ_LIST			pObjList,
	IN  u4Byte						paramId,
	IN  u4Byte						fromIdx,
	IN  u4Byte						bufLen,
	IN  PVOID						pBuf
	);

//-----------------------------------------------------------------------------
// P2PSvc_PD.c
//-----------------------------------------------------------------------------

VOID
P2PSvc_DeterminePDSessionGOSsid(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	OUT pu1Byte 					ssidBuf,
	OUT pu1Byte 					pSsidLen
	);

PP2PSVC_OBJ_LIST
P2PSvc_Get_PDEntryObjList(
	IN  PP2PSVC_PD_ENTRY			pPDEntry
	);

VOID
P2PSvc_Free_PDSessionListEntry(
	IN  PP2PSVC_PD_ENTRY			pPDEntry,
	IN  pu4Byte						pListCnt
	);

VOID
P2PSvc_Free_PDSessionList(
	IN  PRT_LIST_ENTRY				pListHead,
	IN  pu4Byte						pListCnt
	);


BOOLEAN
P2PSvc_PDSessionInProgress(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	OUT PP2PSVC_PD_ENTRY			*ppPDEntry
	);

RT_STATUS 
P2PSvc_AddPDInitorData(
	IN  PADAPTER					pAdapter, 
	IN  PVOID						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen
	);

RT_STATUS 
P2PSvc_AddPDRspdorData(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  u2Byte						configMethod,
	IN  POCTET_STRING				posP2PAttrs
	);

RT_STATUS
P2PSvc_RspdorSetFOPDReq(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	IN  BOOLEAN						bAccepted
	);

RT_STATUS
P2PSvc_InitorOnPDRsp(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	IN  u2Byte						reqConfigMethod,
	IN  u2Byte						rspConfigMethod,
	IN  POCTET_STRING				posP2PAttrs
	);

RT_STATUS
P2PSvc_InitorOnFOPDReq(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  u2Byte						configMethod,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	IN  POCTET_STRING				posP2PAttrs
	);

RT_STATUS
P2PSvc_RspdorOnFOPDRsp(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	IN  u2Byte						reqConfigMethod,
	IN  u2Byte						rspConfigMethod,
	IN  POCTET_STRING				posP2PAttrs
	);

RT_STATUS
P2PSvc_InitorOnSendPDReqFailure(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry,
	IN  u1Byte						p2pStatus
	);

RT_STATUS
P2PSvc_RspdorOnSendFOPDReqFailure(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY			pPDEntry,
	IN  u1Byte						p2pStatus
	);

BOOLEAN
P2PSvc_IsLegacyPD(
	IN  POCTET_STRING				posP2PAttrs
	);

VOID
P2PSvc_PDSessionSucceeded(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry
	);

VOID
P2PSvc_PDSessionFailed(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 			pPDEntry
	);

VOID
P2PSvc_PDSessionAgeFunc(
	IN  PP2PSVC_INFO				pP2PSvcInfo
	);

//-----------------------------------------------------------------------------
// P2PSvc_SD.c
//-----------------------------------------------------------------------------

RT_STATUS
P2PSvc_ParseAnqpQueryReq(
	IN  PP2P_SERVICE_REQ_TLV		pAnqpQueryReq,
	OUT pu1Byte						pSvcNameLen,
	OUT pu1Byte						*ppSvcNameBuf,
	OUT pu1Byte						pSvcInfoReqLen,
	OUT pu1Byte						*ppSvcInfoReqBuf
	);

RT_STATUS
P2PSvc_ParseAnqpQueryRsp(
	IN  PP2P_SERVICE_RSP_TLV		pAnqpQueryRsp,
	IN  u1Byte						index,
	OUT pu1Byte						pSvcNameLen,
	OUT pu1Byte						*ppSvcNameBuf,
	OUT pu4Byte						pAdvId,
	OUT pu1Byte						pSvcStatus,
	OUT pu2Byte						pSvcInfoLen,
	OUT pu1Byte						*ppSvcInfoBuf
	);

RT_STATUS
P2PSvc_SDReq(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  PP2PSVC_REQ_INFO_ENTRY		pSeekInfoEntry,
	IN  PRT_OBJECT_HEADER			pSvcInfoReqObj,
	IN  u1Byte						searchId
	);

RT_STATUS
P2PSvc_SDRsp(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  u1Byte						dlgToken,
	IN  u1Byte						transactionId,
	IN  u1Byte						nSvcInfoDesc,
	IN  FRAME_BUF					*pSvcInfoDescBuf
	);

BOOLEAN
P2PSvc_SDMatchSvcName(
	IN  PP2PSVC_REQ_INFO_ENTRY		pAdvInfoEntry,
	IN  u1Byte						svcNameLen,
	IN  pu1Byte						svcNameBuf
	);

BOOLEAN
P2PSvc_ToProcessSDRsp(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  u1Byte						searchId,
	IN  pu1Byte						devAddr
	);

//-----------------------------------------------------------------------------
// P2PSvc_SearchResult.c
//-----------------------------------------------------------------------------

VOID
P2PSvc_Free_SearchResultObjList(
	IN  PRT_LIST_ENTRY				pListHead,
	IN  pu4Byte						pListCnt
	);

BOOLEAN
P2PSvc_ToProcessProbeRsp(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  u1Byte						searchId,
	IN  pu1Byte						devAddr
	);

RT_STATUS
P2PSvc_GetSearchResult(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  u1Byte						searchId,
	IN  pu1Byte						devAddr,
	OUT PP2PSVC_SR_LIST_ENTRY		*ppSREntry
	);

BOOLEAN
P2PSvc_SeekReqNeedSD(
	IN  PP2PSVC_OBJ_LIST			pSeekObjList,
	IN  PP2PSVC_SR_LIST_ENTRY		pSREntryInList,
	OUT PRT_OBJECT_HEADER			*ppSvcInfoReqObj
	);

RT_STATUS
P2PSvc_UpdateSearchResult(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  u1Byte						searchId, 
	IN  pu1Byte						devAddr,
	IN  PP2PSVC_SR_LIST_ENTRY		pSREntryToUpdate,
	OUT PBOOLEAN					pbDuplicatedEntry,
	OUT PP2PSVC_SR_LIST_ENTRY		*ppSREntryOut
	);

RT_STATUS
P2PSvc_MakeSearchResultDataFromProbeRsp(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PP2PSVC_REQ_INFO_ENTRY 		pSeekInfoEntry, 
	IN  pu1Byte						devAddr, 
	IN  POCTET_STRING				posP2PAttrs,
	IN  OCTET_STRING				osAdvSvcInfo,
	OUT PP2PSVC_SR_LIST_ENTRY		*ppSREntryOut
	);

RT_STATUS
P2PSvc_MakeSearchResultDataFromSDRsp(
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  u1Byte						searchId,
	IN  pu1Byte						devAddr,
	IN  PP2P_SERVICE_RSP_TLV		pSvcRspTlv,
	OUT PP2PSVC_SR_LIST_ENTRY		*ppSREntryOut
	);

//-----------------------------------------------------------------------------
// P2PSvc_ActionInfo.c
//-----------------------------------------------------------------------------
BOOLEAN
P2PSvc_Free_AdvSvcList(
	IN  PRT_LIST_ENTRY				pListHead,
	IN  pu4Byte						pListCnt,
	IN  u4Byte						advId
	);

BOOLEAN
P2PSvc_Free_SeekReqList(
	IN  PRT_LIST_ENTRY				pListHead,
	IN  pu4Byte						pListCnt,
	IN  u1Byte						seekId
	);

RT_STATUS 
P2PSvc_Set_AdvSvc(
	IN  PADAPTER 					pAdapter, 
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	);

RT_STATUS 
P2PSvc_Set_CancelAdvSvc(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	); 

RT_STATUS 
P2PSvc_Set_SvcStatus(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	);

RT_STATUS 
P2PSvc_Set_Seek(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	);

RT_STATUS 
P2PSvc_Set_CancelSeek(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	);

RT_STATUS 
P2PSvc_Set_ConnCap(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	);

RT_STATUS 
P2PSvc_Set_PDReq(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	);

RT_STATUS 
P2PSvc_Set_RspdorFOPDReq(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	);

#endif	// #ifndef __INC_P2P_SVC_INTERNAL_H
