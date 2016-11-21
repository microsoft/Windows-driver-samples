#include "Mp_Precomp.h"

#include "P2PSvc_Internal.h"

#if (P2P_SUPPORT == 1)
#if (P2PSVC_SUPPORT == 1)

static P2PSVC_CONN_CAP_MAP_ENTRY P2PSvcConnCapMap[] = 
{
	// requesting NEW
	{
		P2PSVC_CONN_CAP_BMP_NEW,
		P2PSVC_CONN_CAP_BMP_NEW,
		P2PSVC_CONN_CAP_BMP_NEW,
		"GO nego resulting in a new group"
	},

	{
		P2PSVC_CONN_CAP_BMP_NEW,
		P2PSVC_CONN_CAP_BMP_CLI,
		P2PSVC_CONN_CAP_BMP_CLI,
		"Join initor's group"
	},

	{
		P2PSVC_CONN_CAP_BMP_NEW,
		P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_GO,
		"Peer shall join our existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_NEW,								// initor can be either GO or Cli
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,		// we have existing grpup
		P2PSVC_CONN_CAP_BMP_GO,									// initor shall join the existing group
		"Peer shall join our existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_NEW,								// initor can be either GO or Cli
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,		// we have existing grpup and can also join a group
		P2PSVC_CONN_CAP_BMP_GO,									// initor shall join the existing group
		"Peer shall join our existing group"
	},

	// requesting CLI
	{
		P2PSVC_CONN_CAP_BMP_CLI,
		P2PSVC_CONN_CAP_BMP_NEW,
		P2PSVC_CONN_CAP_BMP_GO,									// to start auto GO
		"To start auto GO"
	},

	{
		P2PSVC_CONN_CAP_BMP_CLI,
		P2PSVC_CONN_CAP_BMP_CLI,
		P2PSVC_CONN_CAP_BMP_NONE,								// X
		"ERROR:Do not support Cli/Cli"
	},
	
	{
		P2PSVC_CONN_CAP_BMP_CLI,
		P2PSVC_CONN_CAP_BMP_GO,									// we have existing group
		P2PSVC_CONN_CAP_BMP_GO,									// initor shall join the existing group
		"Peer shall join our existing group"
	},
	
	{
		P2PSVC_CONN_CAP_BMP_CLI,								// initor can be either GO or Cli
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,		// we have existing grpup
		P2PSVC_CONN_CAP_BMP_GO,									// initor shall join the existing group
		"Peer shall join our existing group"
	},
	
	{
		P2PSVC_CONN_CAP_BMP_CLI,								// initor can be either GO or Cli
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,		// we have existing grpup and can join a group
		P2PSVC_CONN_CAP_BMP_GO,									// initor shall join the existing group
		"Peer shall join our existing group"
	},

	// requesting GO
	{
		P2PSVC_CONN_CAP_BMP_GO,									// initor can be only GO
		P2PSVC_CONN_CAP_BMP_NEW,								// we can be either GO or Cli
		P2PSVC_CONN_CAP_BMP_CLI,								// join initor's group
		"Join peer's existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_CLI,								// we can be only Cli
		P2PSVC_CONN_CAP_BMP_CLI,								// join initor's group
		"Join peer's existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_NONE,								// X
		"ERROR: Do not support GO/GO"
	},

	{
		P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,		// we can be GO or a cli
		P2PSVC_CONN_CAP_BMP_CLI,								// join initor's group
		"Join peer's existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,		// we have existing grpup and can also join a group
		P2PSVC_CONN_CAP_BMP_CLI,								// initor shall join the existing group
		"Join peer's existing group"
	},

	// requesting (NEW, GO)
	{
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_NEW,								// we can be either GO or Cli after negotiation
		P2PSVC_CONN_CAP_BMP_GO,
		"Peer shall join our auto GO"
	},

	{
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_CLI,								// we can be only Cli
		P2PSVC_CONN_CAP_BMP_CLI,								// join initor's group
		"Join peer's existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_GO,									// we have existing grpup
		P2PSVC_CONN_CAP_BMP_GO,
		"Peer shall join our existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,		// we have existing grpup
		P2PSVC_CONN_CAP_BMP_GO,
		"Peer shall join our existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,		// we have existing grpup
		P2PSVC_CONN_CAP_BMP_GO,
		"Peer shall join our existing group"
	},

	// requesting (CLI, GO)
	{
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_NEW,								// we can be either GO or Cli after negotiation
		P2PSVC_CONN_CAP_BMP_GO,
		"Peer shall join our existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_CLI,								// we can be only Cli
		P2PSVC_CONN_CAP_BMP_CLI,								// join initor's group
		"Join peer's existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_GO,									// we have existing grpup
		P2PSVC_CONN_CAP_BMP_GO,
		"Peer shall join our existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO,		// we have existing grpup
		P2PSVC_CONN_CAP_BMP_GO,
		"Peer shall join our existing group"
	},

	{
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,
		P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO,		// we have existing grpup
		P2PSVC_CONN_CAP_BMP_GO,
		"Peer shall join our existing group"
	},

	//--------------------------------------------------------------------------
	{
		P2PSVC_CONN_CAP_BMP_NONE,
		P2PSVC_CONN_CAP_BMP_NONE,
		P2PSVC_CONN_CAP_BMP_NONE,
		"ERROR: configuration not supported"
	}
};


//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------
P2P_STATUS_CODE
p2psvc_GetP2PStatusFromAttrs(
	IN  POCTET_STRING					posP2PAttrs
	)
{
	OCTET_STRING						osStatus = {NULL, 0};
	P2P_STATUS_CODE						p2pStatus = P2P_STATUS_MAX;

	do
	{
		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_STATUS, 1, &osStatus))
		{
			break;
		}
		
		if(osStatus.Octet != NULL)
			p2pStatus = (P2P_STATUS_CODE)(*(pu1Byte)osStatus.Octet);
	}
	while(FALSE);

	return p2pStatus;
}

u1Byte
p2psvc_GetConnCapFromAttrs(
	IN  POCTET_STRING					posP2PAttrs
	)
{
	OCTET_STRING						osConnCap = {NULL, 0};
	u1Byte								connCap = 0;

	do
	{
		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_CONN_CAP_INFO, 1, &osConnCap))
		{
			break;
		}
		
		if(osConnCap.Octet != NULL)
			connCap = *(pu1Byte)osConnCap.Octet;
	}while(FALSE);

	return connCap;
}

VOID
p2psvc_FillGroupInfo(
	IN  pu1Byte							grpDevAddr,
	IN  pu1Byte							grpIntfAddr,
	IN  u1Byte							ssidLen,
	IN  pu1Byte							ssidBuf,
	IN  u1Byte							opChnl,
	OUT PP2PSVC_GROUP_INFO				pGrpInfo
	)
{
	PlatformMoveMemory(pGrpInfo->grpDevAddr, grpDevAddr, 6);
	PlatformMoveMemory(pGrpInfo->grpIntfAddr, grpIntfAddr, 6);
	pGrpInfo->ssidLen = ssidLen;
	PlatformMoveMemory(pGrpInfo->ssidBuf, ssidBuf, ssidLen);
	pGrpInfo->opChnl = opChnl;

	return;
}

RT_STATUS
p2psvc_GetGroupInfoFromP2PAttrs(
	IN  POCTET_STRING 					posP2PAttrs,
	OUT PP2PSVC_REASON					pBadReason,
	OUT PP2PSVC_GROUP_INFO				pGrpInfo
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	RT_ASSERT(pBadReason, ("%s(): pBadReason is NULL!!!\n", __FUNCTION__));
	RT_ASSERT(pGrpInfo, ("%s(): pGrpInfo is NULL!!!\n", __FUNCTION__));

	*pBadReason = P2PSVC_REASON_NONE;

	do
	{
		OCTET_STRING					osGrpId = {NULL, 0};
		OCTET_STRING					osIntfAddr = {NULL, 0};
		OCTET_STRING					osOpChnl = {NULL, 0};

		// group id, [grpDevAddr(6), grpSsid(n)]
		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_GROUP_ID, 6 + 1, &osGrpId))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] no P2P GroupID attr (ID: %u) in PD req/rsp\n", 
				P2P_ATTR_GROUP_ID));
			*pBadReason = P2PSVC_REASON_MISS_ATTR_GRP_ID;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// intended intf addr
		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_INTENDED_INTERFACE_ADDRESS, 6, &osIntfAddr))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] no P2P IntfAddr attr (ID: %u) in PD req/rsp\n", 
				P2P_ATTR_INTENDED_INTERFACE_ADDRESS));
			*pBadReason = P2PSVC_REASON_MISS_ATTR_INTF_ADDR;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// op channel, [countryStr(3), regClass(1), chnl(1)]
		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_OP_CHANNEL, 3 + 1 + 1, &osOpChnl))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] no P2P OpChannel attr (ID: %u) in PD req/rsp\n", 
				P2P_ATTR_OP_CHANNEL));
			*pBadReason = P2PSVC_REASON_MISS_ATTR_OP_CHNL;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// copy info
		if (osOpChnl.Octet != NULL)
		{
			p2psvc_FillGroupInfo(osGrpId.Octet + 0, 		// grpDevAddr
				osIntfAddr.Octet, 							// grpIntfAddr
				osGrpId.Length - 6, 						// ssidLen
				osGrpId.Octet + 6, 							// ssidBuf
				*((pu1Byte)(osOpChnl.Octet + 3 + 1)), 		// opChnl
				pGrpInfo);
		}
		
	}while(FALSE);

	return rtStatus;
}

VOID
p2psvc_Init_PDSessionInfo(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY				pPDEntry,
	IN  BOOLEAN							bInitor,
	IN  u4Byte							sessionId,
	IN  pu1Byte							peerDevAddr,
	IN  u4Byte							advId,
	IN  u1Byte							pdReqConnCap,
	IN  u1Byte							selfConnCap
	)
{
	PP2P_INFO							pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
	
	PlatformZeroMemory(pPDEntry, sizeof(P2PSVC_PD_ENTRY));

	RTInitializeListHead(&pPDEntry->List);

	pPDEntry->bInitor = bInitor;
	pPDEntry->bDeferred = FALSE;
	pPDEntry->createTime = PlatformGetCurrentTime();

	pPDEntry->bDone = FALSE;
	pPDEntry->status = P2P_STATUS_MAX;

	pPDEntry->sessionId = sessionId;
	PlatformMoveMemory(pPDEntry->peerDevAddr, peerDevAddr, 6);
	pPDEntry->advId = advId;

	pPDEntry->pdReqConnCap = pdReqConnCap;

	pPDEntry->selfConnCap = selfConnCap;

	if(TEST_FLAG(selfConnCap, P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO))
	{
		UCHAR							ssidBuf[32] = {0};
		UCHAR							ssidLen = 0;
		
		P2PSvc_DeterminePDSessionGOSsid(pP2PSvcInfo, ssidBuf, &ssidLen);
			
		p2psvc_FillGroupInfo(pP2PInfo->DeviceAddress, 
			pP2PInfo->InterfaceAddress, 
			ssidLen, 
			ssidBuf, 
			pP2PInfo->OperatingChannel, 
			&pPDEntry->selfGrpInfo);
	}
	
	return;
}

VOID
p2psvc_Set_PDResult(
	IN  PP2PSVC_PD_ENTRY				pPDEntry,
	IN  P2P_STATUS_CODE					status,
	IN  u1Byte							connCap
	)
{
	pPDEntry->bDone = TRUE;
	pPDEntry->status = status;
	pPDEntry->connCap = connCap;

	return;
}

RT_STATUS
p2psvc_New_InitorPDSessionInfo(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_OBJ_LIST				pObjList,
	OUT PP2PSVC_INITOR_PD_ENTRY 		*ppInitorPDEntry
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INITOR_PD_ENTRY 			pEntry = NULL;

	u4Byte								objListLen = P2PSVC_OBJ_LIST_LEN(pObjList);
	u4Byte								reqBufSize = FIELD_OFFSET(P2PSVC_INITOR_PD_ENTRY, objList) + objListLen;

	*ppInitorPDEntry = NULL;

	do
	{
		PRT_OBJECT_HEADER				pObj = NULL;
		
		u4Byte							sessionId = 0;
		pu1Byte							peerDevAddr = NULL;
		u4Byte							advId = 0;

		//4 Get info req for init base class from obj list
		// session-id
		pObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_SESSION_ID, 0);
		sessionId = *(pu4Byte)pObj->Value;

		// svc-mac
		pObj 	= P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR, 0);
		peerDevAddr = pObj->Value;

		// adv-id
		pObj 	= P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
		advId = *(pu4Byte)pObj->Value;

		//4 Allocate memory
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pEntry, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pInitorPDSessionEntry\n"));
			break;
		}

		//4 Zero memory
		PlatformZeroMemory(pEntry, sizeof(P2PSVC_INITOR_PD_ENTRY));

		//4 Init base class
		p2psvc_Init_PDSessionInfo(pP2PSvcInfo,
			&pEntry->super, 
			TRUE, 
			sessionId, 
			peerDevAddr, 
			advId, 
			pP2PSvcInfo->connCap,
			pP2PSvcInfo->connCap);

		//4 Init self
		pEntry->pdReqConfigMethod = 0;
		pEntry->pdRspRxTime = 0;
		pEntry->pdRspStatus = P2P_STATUS_MAX;
		pEntry->fopdReqRxTime = 0;
		pEntry->fopdReqStatus = P2P_STATUS_MAX;
		pEntry->fopdReqConnCap = 0;
		pEntry->fopdRspStatus = P2P_STATUS_MAX;
		pEntry->fopdRspConnCap = 0;
		PlatformMoveMemory(&pEntry->objList, pObjList, objListLen);

		//4 Done
		*ppInitorPDEntry = pEntry;
	}while(FALSE);

	return rtStatus;
}

VOID
p2psvc_Del_InitorPDSessionEntry(
	IN  PP2PSVC_INITOR_PD_ENTRY			pInitorPDEntry
	)
{
	u4Byte								objListLen = 0;
	u4Byte								totalLen = 0;

	objListLen = P2PSVC_OBJ_LIST_LEN(&pInitorPDEntry->objList);
	totalLen = FIELD_OFFSET(P2PSVC_INITOR_PD_ENTRY, objList) + objListLen;
	
	P2PSvc_FreeMem(pInitorPDEntry, totalLen);
}

RT_STATUS
p2psvc_New_RspdorPDSessionInfo(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  pu1Byte							initorDevAddr,
	IN  PP2PSVC_REQ_INFO_ENTRY			pAdvSvcInfoEntry,
	IN  POCTET_STRING					posPDReqP2PAttrs,
	IN  u2Byte							configMethod,
	OUT PP2PSVC_REASON					pBadReason,
	OUT PP2PSVC_RSPDOR_PD_ENTRY			*ppRspdorPDEntry
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_RSPDOR_PD_ENTRY				pEntry = NULL;

	P2PSVC_FUNC_IN(DBG_LOUD);

	*pBadReason = P2PSVC_REASON_NONE;
	*ppRspdorPDEntry = NULL;

	do
	{
		// Info from self adv svc
		PRT_OBJECT_HEADER			pSvcNameObj = NULL;
		PRT_OBJECT_HEADER			pAutoAcceptObj = NULL;
		PRT_OBJECT_HEADER			pSvcStatusObj = NULL;
		PRT_OBJECT_HEADER			pAdvIdObj = NULL;

		// Info from peer PD req info
		OCTET_STRING				osSessionId = {NULL, 0};
		OCTET_STRING				osDevName = {NULL, 0};
		u1Byte						pdReqConnCap = 0;

		// Obj list
		u4Byte						reqBufSize = 0;

		//4 Get info from SELF adv svc
		pSvcNameObj 	= P2PSvc_GetParam(&pAdvSvcInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_NAME, 0);
		pAutoAcceptObj  = P2PSvc_GetParam(&pAdvSvcInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_AUTO_ACCEPT, 0);
		pSvcStatusObj 	= P2PSvc_GetParam(&pAdvSvcInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS, 0);
		pAdvIdObj 		= P2PSvc_GetParam(&pAdvSvcInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
		
		if(NULL == pSvcNameObj
			|| NULL == pAutoAcceptObj
			|| NULL == pSvcStatusObj
			|| NULL == pAdvIdObj
			)
		{
			rtStatus = RT_STATUS_INVALID_DATA; 
			break;
		}
		
		//4 Get info form peer PD req

		// Get peer connCap
		if(0 == (pdReqConnCap = p2psvc_GetConnCapFromAttrs(posPDReqP2PAttrs)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("peer PD req contains conn cap 0\n"));
			*pBadReason = P2PSVC_REASON_MISS_ATTR_CONN_CAP;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// Get peer session-id and dev-addr
		if(!P2PSvc_GetP2PAttr(posPDReqP2PAttrs, P2P_ATTR_SESSION_ID_INFO, 4 + 6, &osSessionId))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("invalid sessionIDInfo attr\n"));
			*pBadReason = P2PSVC_REASON_MISS_ATTR_SESSION_ID_INFO;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// Get peer dev-name
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_GetDevNameFromDevInfoAttr(posPDReqP2PAttrs, &osDevName)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("failed to get peer dev name\n"));
			*pBadReason = P2PSVC_REASON_MISS_ATTR_DEV_INFO;
			break;
		}

		//4 Eval req buf len
		reqBufSize = 0;
		reqBufSize += FIELD_OFFSET(P2PSVC_RSPDOR_PD_ENTRY, objList);
		reqBufSize += FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart);
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));				// user accept
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 4);								// session-id
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);								// peer dev addr
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 4);								// adv-id
		reqBufSize += (RT_OBJECT_HEADER_SIZE + pSvcNameObj->Length);			// svc-name
		reqBufSize += (RT_OBJECT_HEADER_SIZE + osDevName.Length); 				// dev-name
		reqBufSize += (RT_OBJECT_HEADER_SIZE + pSvcStatusObj->Length); 			// adv svc-status
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 2); 								// peer config method
		reqBufSize += (RT_OBJECT_HEADER_SIZE + posPDReqP2PAttrs->Length);		// P2P attrs in the PD req
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));				// bDeferred
		
		//4 Alloc mem
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pEntry, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for PD rspdor session\n"));
			break;
		}

		PlatformZeroMemory(pEntry, reqBufSize);

		//4 Init base class
		if (osSessionId.Octet != NULL)
		{
			p2psvc_Init_PDSessionInfo(pP2PSvcInfo,
				&pEntry->super,
				FALSE,
				*(pu4Byte)osSessionId.Octet,
				initorDevAddr,
				*(pu4Byte)pAdvIdObj->Value,
				pdReqConnCap,
				pP2PSvcInfo->connCap);
		}

		// Check if is deferred case
		if(FALSE == *(PBOOLEAN)pAutoAcceptObj->Value)
		{// autoAccept == false
			pEntry->super.bDeferred = TRUE;
		}
		else
		{// autoAccept
			if(P2P_WPS_CONFIG_METHODS_KEYPAD == (WPS_CONFIG_METHODS)configMethod
				|| P2P_WPS_CONFIG_METHODS_PUSHBUTTON == (WPS_CONFIG_METHODS)configMethod
				)
			{
				pEntry->super.bDeferred = TRUE;
			}
			else
			{
				pEntry->super.bDeferred = FALSE;
			}
		}

		//4 Init self
		pEntry->configMethod = configMethod;
		pEntry->pdReqRxTime = PlatformGetCurrentTime();
		pEntry->pdRspStatus = P2P_STATUS_MAX;
		pEntry->pdRspConnCap = 0;
		pEntry->fopdRspRxTime = 0;
		pEntry->fopdReqStatus = P2P_STATUS_MAX;
		pEntry->fopdReqConnCap = 0;
		pEntry->fopdRspStatus = P2P_STATUS_MAX;
		pEntry->fopdRspConnCap = 0;

		P2PSVC_OBJ_LIST_INIT(&pEntry->objList, 
			RT_OB_HDR_TYPE_DATA, 
			P2PSVC_OBJ_HDR_ID_INDIC_RSPDOR_ON_PD_REQ, 
			P2PSVC_MIN_SUPPORT_VER);

		P2PSvc_MakeUserAcceptedObj(&pEntry->objList, FALSE);
		//Prefast warning C6011: Dereferencing NULL pointer 'osSessionId.Octet'.
		if(osSessionId.Octet != NULL)
			P2PSvc_MakeSessionIdObj(&pEntry->objList, *(pu4Byte)osSessionId.Octet);

		P2PSvc_MakeDevAddrObj(&pEntry->objList, initorDevAddr);
		P2PSvc_MakeDevNameObj_FromDevNameOctet(&pEntry->objList, osDevName);
		P2PSvc_MakeAdvIdObj(&pEntry->objList, *(pu4Byte)pAdvIdObj->Value);
		P2PSvc_MakeSvcNameObj(&pEntry->objList, (u1Byte)pSvcNameObj->Length, pSvcNameObj->Value);
		P2PSvc_MakeSvcStatusObj(&pEntry->objList, *(pu1Byte)pSvcStatusObj->Value);
		P2PSvc_MakeConfigMethodObj(&pEntry->objList, pEntry->configMethod);
		P2PSvc_MakeP2PAttrsObj(&pEntry->objList, posPDReqP2PAttrs->Length, posPDReqP2PAttrs->Octet);
		P2PSvc_MakeDeferredObj(&pEntry->objList, pEntry->super.bDeferred);

		*ppRspdorPDEntry = pEntry;

	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

VOID
p2psvc_Del_RspdorPDSessionEntry(
	IN  PP2PSVC_RSPDOR_PD_ENTRY			pRspdorPDEntry
	)
{
	u4Byte								objListLen = 0;
	u4Byte								totalLen = 0;

	objListLen = P2PSVC_OBJ_LIST_LEN(&pRspdorPDEntry->objList);
	totalLen = FIELD_OFFSET(P2PSVC_RSPDOR_PD_ENTRY, objList) + objListLen;
	
	P2PSvc_FreeMem(pRspdorPDEntry, totalLen);
}

RT_STATUS 
p2psvc_GetPDRequestedSvc(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  POCTET_STRING					posP2PAttrs,
	OUT PP2PSVC_REQ_INFO_ENTRY 			*ppInfoEntry,
	OUT pu4Byte							pAdvId
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;

	do
	{
		OCTET_STRING					osAdvId = {NULL, 0};
		u4Byte							peerReqAdvId = 0;
		PP2PSVC_REQ_INFO_ENTRY 			pAdvSvcEntry = NULL;
	
		// Get peer requested adv-id
		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_ADV_ID_INFO, 4 + 6, &osAdvId))
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(osAdvId.Octet != NULL)
			peerReqAdvId = *((pu4Byte)osAdvId.Octet);
		
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_GetAdvSvcByAdvId(pP2PSvcInfo, peerReqAdvId, &pAdvSvcEntry)))
		{
			break;	
		}

		if(ppInfoEntry) *ppInfoEntry = pAdvSvcEntry;
		if(pAdvId) *pAdvId = peerReqAdvId;
		break;
	}while(FALSE);

	return rtStatus;
}

BOOLEAN 
p2psvc_GetAdvSvcAvailability(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_REQ_INFO_ENTRY			pInfoEntry
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	BOOLEAN								bAvailable = FALSE;

	do
	{
		PRT_OBJECT_HEADER				pSvcStatuObj = NULL;
		u1Byte							advSvcStatus = 0;

		pSvcStatuObj = P2PSvc_GetParam(&pInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS, 0);
		if(NULL == pSvcStatuObj)
		{
			break;
		}

		advSvcStatus = *(pu1Byte)pSvcStatuObj->Value;

		if(TEST_FLAG(advSvcStatus, BIT0))
		{
			bAvailable = TRUE;
		}

		break;
		
	}while(FALSE);

	return bAvailable;
}

BOOLEAN
p2psvc_MatchAdvId(
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	PRT_OBJECT_HEADER 					pSvcNameObj = NULL;
	BOOLEAN 							bMatch = FALSE;

	do
	{
		OCTET_STRING					osAdvId = {NULL, 0};

		u4Byte							peerAdvId = 0;
		pu1Byte							pSvcMac = NULL;

		// Get peer adv-id info attr: [adv-id(4), svc-mac(6)]
		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_ADV_ID_INFO, 4 + 6, &osAdvId))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Drop for no/invalid adv id attr\n"));
			RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "posP2PAttrs:\n", posP2PAttrs->Octet, posP2PAttrs->Length);
			break;
		}
		
		if (osAdvId.Octet != NULL)
		{
			peerAdvId = *(pu4Byte)osAdvId.Octet;
			pSvcMac = (osAdvId.Octet + 4);
		}

		// Get self adv-id
		if(pPDEntry->advId != peerAdvId)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Drop for mismatched adv id (self: %u, peer: %u)\n", pPDEntry->advId, peerAdvId));
			break;
		}

		bMatch = TRUE;

	}while(FALSE);

	return bMatch;
}

RT_STATUS
p2psvc_IndicateRxBadPD(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  pu1Byte							pPeerDevAddr,
	IN  BOOLEAN							bInitor,
	IN  BOOLEAN							bReq,
	IN  P2PSVC_REASON					reason,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	u4Byte								reqBufSize = 0;
	pu1Byte								pBuf = NULL;

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		u4Byte							indicId = 0;
		PP2PSVC_OBJ_LIST				pObjList = NULL;
		
		if(bInitor)
		{
			indicId = (bReq)
				? (P2PSVC_OBJ_HDR_ID_INDIC_INITOR_ON_BAD_FOPD_REQ)				// req
				: (P2PSVC_OBJ_HDR_ID_INDIC_INITOR_ON_BAD_PD_RSP);				// rsp
		}
		else
		{
			indicId = (bReq)
				? (P2PSVC_OBJ_HDR_ID_INDIC_RSPDOR_ON_BAD_PD_REQ)				// req
				: (P2PSVC_OBJ_HDR_ID_INDIC_RSPDOR_ON_BAD_FOPD_RSP);				// rsp
		}

		reqBufSize = 0;
		reqBufSize += FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart);
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);								// peer dev-addr
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(P2PSVC_REASON));			// PDRsp status code
		reqBufSize += (RT_OBJECT_HEADER_SIZE + posP2PAttrs->Length);			// P2P attrs in PD rsp
		
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pBuf\n"));
			break;
		}

		PlatformZeroMemory(pBuf, reqBufSize);
		
		pObjList = (PP2PSVC_OBJ_LIST)pBuf;

		// Fill obj list
		P2PSVC_OBJ_LIST_INIT(pObjList, 
			RT_OB_HDR_TYPE_DATA, 
			indicId, 
			P2PSVC_MIN_SUPPORT_VER);

		P2PSvc_MakeDevAddrObj(pObjList, pPeerDevAddr);
		P2PSvc_MakeReasonObj(pObjList, reason);
		P2PSvc_MakeP2PAttrsObj(pObjList, posP2PAttrs->Length, posP2PAttrs->Octet);
		
		P2PSvc_Indicate(pP2PSvcInfo, reqBufSize, pBuf);		
	}while(FALSE);

	if(pBuf) P2PSvc_FreeMem(pBuf, reqBufSize);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}


RT_STATUS
p2psvc_IndicateInitorOnPDRsp(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INITOR_PD_ENTRY				pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;
	PP2PSVC_PD_CONN_TOPOLOGY			pConnTopology = &pPDEntry->connTopology;
	u4Byte								reqBufSize = 0;
	pu1Byte								pBuf = NULL;

	P2PSVC_FUNC_IN(DBG_LOUD);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("super.status: %u\n", pInitorPDEntry->super.status));
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("pInitorPDEntry->pdRspStatus: %u\n", pInitorPDEntry->pdRspStatus));

	do
	{
		PP2PSVC_OBJ_LIST				pObjList = NULL;

		OCTET_STRING					osDevName;

		// Get dev-name
		if(RT_STATUS_SUCCESS != P2PSvc_GetDevNameFromDevInfoAttr(posP2PAttrs, &osDevName))
		{
			FillOctetString(osDevName, NULL, 0);
		}

		reqBufSize = 0;
		reqBufSize += FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart);
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);									// peer dev-addr
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(P2P_STATUS_CODE));			// PDRsp status code
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));					// bDeferred
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 2);									// config method in PD req
		reqBufSize += (RT_OBJECT_HEADER_SIZE + posP2PAttrs->Length);				// P2P attrs in PD rsp
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u4Byte));						// session-id
		reqBufSize += (RT_OBJECT_HEADER_SIZE + osDevName.Length);					// dev-name
		
		if(P2P_STATUS_SUCCESS == pInitorPDEntry->super.status)
		{
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));				// bPersistent
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u4Byte));					// connAction
			reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);								// grpDevAddr
			reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);								// grpIntfAddr
			reqBufSize += (RT_OBJECT_HEADER_SIZE + pConnTopology->grpInfo.ssidLen);	// SSID
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u1Byte));					// opChnl
		}
		
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pBuf\n"));
			break;
		}

		PlatformZeroMemory(pBuf, reqBufSize);
		
		pObjList = (PP2PSVC_OBJ_LIST)pBuf;

		// Fill obj list
		P2PSVC_OBJ_LIST_INIT(pObjList, 
			RT_OB_HDR_TYPE_DATA, 
			P2PSVC_OBJ_HDR_ID_INDIC_INITOR_ON_PD_RSP, 
			P2PSVC_MIN_SUPPORT_VER);

		P2PSvc_MakeDevAddrObj(pObjList, pInitorPDEntry->super.peerDevAddr);
		P2PSvc_MakeP2PStatusObj(pObjList, pInitorPDEntry->pdRspStatus);
		P2PSvc_MakeDeferredObj(pObjList, pInitorPDEntry->super.bDeferred);
		P2PSvc_MakeConfigMethodObj(pObjList, pInitorPDEntry->pdReqConfigMethod);
		P2PSvc_MakeP2PAttrsObj(pObjList, posP2PAttrs->Length, posP2PAttrs->Octet);
		P2PSvc_MakeSessionIdObj(pObjList, pPDEntry->sessionId);
		P2PSvc_MakeDevNameObj_FromDevNameOctet(pObjList, osDevName);
		
		if(P2P_STATUS_SUCCESS == pInitorPDEntry->super.status)
		{
			P2PSvc_MakeIsPersistentObj(pObjList, pConnTopology->bPersistent);
			P2PSvc_MakeConnActionObj(pObjList, pConnTopology->connAction);
			P2PSvc_MakeGrpDevAddrObj(pObjList, pConnTopology->grpInfo.grpDevAddr);
			P2PSvc_MakeIntfAddrObj(pObjList, pConnTopology->grpInfo.grpIntfAddr);
			P2PSvc_MakeSsidObj(pObjList, pConnTopology->grpInfo.ssidLen, pConnTopology->grpInfo.ssidBuf);
			P2PSvc_MakeOpChannelObj(pObjList, pConnTopology->grpInfo.opChnl);
		}
		
		P2PSvc_Indicate(pP2PSvcInfo, reqBufSize, pBuf);		
	}while(FALSE);

	if(pBuf) P2PSvc_FreeMem(pBuf, reqBufSize);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS
p2psvc_IndicateInitorOnFOPDReq(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INITOR_PD_ENTRY				pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;
	PP2PSVC_PD_CONN_TOPOLOGY			pConnTopology = &pPDEntry->connTopology;
	u4Byte								reqBufSize = 0;
	pu1Byte								pBuf = NULL;

	P2PSVC_FUNC_IN(DBG_LOUD);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("super.status: %u\n", pInitorPDEntry->super.status));
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("pInitorPDEntry->fopdRspStatus: %u\n", pInitorPDEntry->fopdRspStatus));

	do
	{
		PP2PSVC_OBJ_LIST				pObjList = NULL;

		OCTET_STRING					osDevName;

		// Get dev-name
		if(RT_STATUS_SUCCESS != P2PSvc_GetDevNameFromDevInfoAttr(posP2PAttrs, &osDevName))
		{
			FillOctetString(osDevName, NULL, 0);
		}

		reqBufSize = 0;
		reqBufSize += FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart);
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);										// peer dev-addr
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(P2P_STATUS_CODE));				// FOPDRsp status code
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));						// bDeferred
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));						// accepted by peer user
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 2);										// config method in PD req
		reqBufSize += (RT_OBJECT_HEADER_SIZE + posP2PAttrs->Length);					// P2P attrs in FOPD req
		reqBufSize += (RT_OBJECT_HEADER_SIZE + osDevName.Length);						// dev-name
		
		if(P2P_STATUS_SUCCESS == pInitorPDEntry->super.status)
		{
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));					// bPersistent
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u4Byte));						// connAction
			reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);									// grpDevAddr
			reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);									// grpIntfAddr
			reqBufSize += (RT_OBJECT_HEADER_SIZE + pConnTopology->grpInfo.ssidLen);		// SSID
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u1Byte));						// opChnl
		}
		
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pBuf\n"));
			break;
		}

		PlatformZeroMemory(pBuf, reqBufSize);
		
		pObjList = (PP2PSVC_OBJ_LIST)pBuf;

		// Prefast warning
#pragma warning( disable:6063 )
		RT_ASSERT(pInitorPDEntry->super.bDeferred, ("%s(): initor recv PD req but bDeferred is not set\n"));

		// Fill obj list
		P2PSVC_OBJ_LIST_INIT(pObjList, 
			RT_OB_HDR_TYPE_DATA, 
			P2PSVC_OBJ_HDR_ID_INDIC_INITOR_ON_FOPD_REQ, 
			P2PSVC_MIN_SUPPORT_VER);

		P2PSvc_MakeDevAddrObj(pObjList, pInitorPDEntry->super.peerDevAddr);
		P2PSvc_MakeP2PStatusObj(pObjList, pInitorPDEntry->fopdRspStatus);
		P2PSvc_MakeDeferredObj(pObjList, pInitorPDEntry->super.bDeferred);
		P2PSvc_MakeUserAcceptedObj(pObjList, (P2P_STATUS_SUCCESS_ACCEPTED_BY_USER == pInitorPDEntry->fopdReqStatus));
		P2PSvc_MakeConfigMethodObj(pObjList, pInitorPDEntry->pdReqConfigMethod);
		P2PSvc_MakeP2PAttrsObj(pObjList, posP2PAttrs->Length, posP2PAttrs->Octet);
		P2PSvc_MakeDevNameObj_FromDevNameOctet(pObjList, osDevName);

		if(P2P_STATUS_SUCCESS == pInitorPDEntry->super.status)
		{
			P2PSvc_MakeIsPersistentObj(pObjList, pConnTopology->bPersistent);
			P2PSvc_MakeConnActionObj(pObjList, pConnTopology->connAction);
			P2PSvc_MakeGrpDevAddrObj(pObjList, pConnTopology->grpInfo.grpDevAddr);
			P2PSvc_MakeIntfAddrObj(pObjList, pConnTopology->grpInfo.grpIntfAddr);
			P2PSvc_MakeSsidObj(pObjList, pConnTopology->grpInfo.ssidLen, pConnTopology->grpInfo.ssidBuf);
			P2PSvc_MakeOpChannelObj(pObjList, pConnTopology->grpInfo.opChnl);
		}
		
		P2PSvc_Indicate(pP2PSvcInfo, reqBufSize, pBuf);
		
	}while(FALSE);

	if(pBuf) P2PSvc_FreeMem(pBuf, reqBufSize);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS
p2psvc_IndicateRspdorOnFOPDRsp(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  P2P_STATUS_CODE					p2pStatus,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_RSPDOR_PD_ENTRY				pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;
	PP2PSVC_PD_CONN_TOPOLOGY			pConnTopology = &pPDEntry->connTopology;
	u4Byte								reqBufSize = 0;
	pu1Byte								pBuf = NULL;

	P2PSVC_FUNC_IN(DBG_LOUD);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("super.status: %u\n", pRspdorPDEntry->super.status));
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("pRspdorPDEntry->fopdRspStatus: %u\n", pRspdorPDEntry->fopdRspStatus));

	do
	{
		PP2PSVC_OBJ_LIST				pObjList = NULL;
		PRT_OBJECT_HEADER				pStatusObj = NULL;

		reqBufSize = 0;
		reqBufSize += FIELD_OFFSET(P2PSVC_OBJ_LIST, varStart);
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);										// peer dev-addr
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(P2P_STATUS_CODE));				// FOPDRsp status code
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));						// bDeferred
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 2);										// config method from the intial PD req
		reqBufSize += (RT_OBJECT_HEADER_SIZE + posP2PAttrs->Length);					// P2P attrs in FOPD rsp
		
		if(P2P_STATUS_SUCCESS == pRspdorPDEntry->super.status)
		{
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));					// bPersistent
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u4Byte));						// connAction
			reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);									// grpDevAddr
			reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);									// grpIntfAddr
			reqBufSize += (RT_OBJECT_HEADER_SIZE + pConnTopology->grpInfo.ssidLen);		// SSID
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u1Byte));						// opChnl
		}
		
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pBuf\n"));
			break;
		}

		PlatformZeroMemory(pBuf, reqBufSize);

		pObjList = (PP2PSVC_OBJ_LIST)pBuf;
	
		// Prefast warning
#pragma warning( disable:6063 )
		RT_ASSERT(pRspdorPDEntry->super.bDeferred, ("%s(): rspdor recv PD rsp but bDeferred is not set\n"));

		// Fill obj list
		P2PSVC_OBJ_LIST_INIT(pObjList, 
			RT_OB_HDR_TYPE_DATA, 
			P2PSVC_OBJ_HDR_ID_INDIC_RSPDOR_ON_FOPD_RSP, 
			P2PSVC_MIN_SUPPORT_VER);

		P2PSvc_MakeDevAddrObj(pObjList, pRspdorPDEntry->super.peerDevAddr);
		P2PSvc_MakeP2PStatusObj(pObjList, pRspdorPDEntry->fopdRspStatus);
		P2PSvc_MakeConfigMethodObj(pObjList, pRspdorPDEntry->configMethod);
		P2PSvc_MakeDeferredObj(pObjList, pRspdorPDEntry->super.bDeferred);
		P2PSvc_MakeP2PAttrsObj(pObjList, posP2PAttrs->Length, posP2PAttrs->Octet);
		
		if(P2P_STATUS_SUCCESS == pRspdorPDEntry->super.status)
		{
			P2PSvc_MakeIsPersistentObj(pObjList, pConnTopology->bPersistent);
			P2PSvc_MakeConnActionObj(pObjList, pConnTopology->connAction);
			P2PSvc_MakeGrpDevAddrObj(pObjList, pConnTopology->grpInfo.grpDevAddr);
			P2PSvc_MakeIntfAddrObj(pObjList, pConnTopology->grpInfo.grpIntfAddr);
			P2PSvc_MakeSsidObj(pObjList, pConnTopology->grpInfo.ssidLen, pConnTopology->grpInfo.ssidBuf);
			P2PSvc_MakeOpChannelObj(pObjList, pConnTopology->grpInfo.opChnl);
		}
		
		P2PSvc_Indicate(pP2PSvcInfo, reqBufSize, pBuf);

	}while(FALSE);

	if(pBuf) P2PSvc_FreeMem(pBuf, reqBufSize);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS
p2psvc_IndicateRspdorOnPDReq(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  u2Byte							configMethod,
	IN  POCTET_STRING					posP2PAttrs,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_RSPDOR_PD_ENTRY				pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;
	u4Byte								reqBufSize = 0;
	pu1Byte								pBuf = NULL;

	P2PSVC_FUNC_IN(DBG_LOUD);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("super.status: %u\n", pRspdorPDEntry->super.status));
	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("pRspdorPDEntry->pdRspStatus: %u\n", pRspdorPDEntry->pdRspStatus));

	do
	{
		OCTET_STRING					osSessionInfoDataInfo;
		OCTET_STRING					osDevName;
		PP2PSVC_OBJ_LIST				pObjList = NULL;
		PP2PSVC_PD_CONN_TOPOLOGY		pConnTopology = &pPDEntry->connTopology;

		// Get session-info-data-info (optional)
		P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_SESSION_INFO_DATA_INFO, 0, &osSessionInfoDataInfo);

		// Get dev-name
		if(RT_STATUS_SUCCESS != P2PSvc_GetDevNameFromDevInfoAttr(posP2PAttrs, &osDevName))
		{
			FillOctetString(osDevName, NULL, 0);
		}
		
		// Eval size of buf required
		reqBufSize = 0;
		reqBufSize += P2PSVC_OBJ_LIST_LEN(&pRspdorPDEntry->objList);
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(P2P_STATUS_CODE));				// status in PD rsp
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u2Byte));							// configMethod
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u4Byte));							// advId
		reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u4Byte));							// sessionId
		reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);										// sessionMac
		reqBufSize += (RT_OBJECT_HEADER_SIZE + osSessionInfoDataInfo.Length);			// session-info-data-info
		reqBufSize += (RT_OBJECT_HEADER_SIZE + osDevName.Length);						// dev-name
		
		if(P2P_STATUS_SUCCESS == pRspdorPDEntry->super.status)
		{
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(BOOLEAN));					// bPersistent
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u4Byte));						// connAction
			reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);									// grpDevAddr
			reqBufSize += (RT_OBJECT_HEADER_SIZE + 6);									// grpIntfAddr
			reqBufSize += (RT_OBJECT_HEADER_SIZE + pConnTopology->grpInfo.ssidLen);		// SSID
			reqBufSize += (RT_OBJECT_HEADER_SIZE + sizeof(u1Byte));						// opChnl
		}
		
		// Alloc mem
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pBuf\n"));
			break;
		}

		PlatformZeroMemory(pBuf, reqBufSize);

		pObjList = (PP2PSVC_OBJ_LIST)pBuf;

		PlatformMoveMemory(pObjList, &pRspdorPDEntry->objList, P2PSVC_OBJ_LIST_LEN(&pRspdorPDEntry->objList));

		// Fill obj list
		pObjList->hdr.Id = P2PSVC_OBJ_HDR_ID_INDIC_RSPDOR_ON_PD_REQ;

		P2PSvc_MakeP2PStatusObj(pObjList, pRspdorPDEntry->pdRspStatus);
		P2PSvc_MakeConfigMethodObj(pObjList, configMethod);
		P2PSvc_MakeAdvIdObj(pObjList, pRspdorPDEntry->super.advId);
		P2PSvc_MakeSessionIdObj(pObjList, pRspdorPDEntry->super.sessionId);
		P2PSvc_MakeDevAddrObj(pObjList, pRspdorPDEntry->super.peerDevAddr);
		P2PSvc_MakeSessionInfoDataInfoObj(pObjList, osSessionInfoDataInfo);
		P2PSvc_MakeDevNameObj_FromDevNameOctet(pObjList, osDevName);
		
		if(P2P_STATUS_SUCCESS == pRspdorPDEntry->super.status)
		{
			P2PSvc_MakeIsPersistentObj(pObjList, pConnTopology->bPersistent);
			P2PSvc_MakeConnActionObj(pObjList, pConnTopology->connAction);
			P2PSvc_MakeGrpDevAddrObj(pObjList, pConnTopology->grpInfo.grpDevAddr);
			P2PSvc_MakeIntfAddrObj(pObjList, pConnTopology->grpInfo.grpIntfAddr);
			P2PSvc_MakeSsidObj(pObjList, pConnTopology->grpInfo.ssidLen, pConnTopology->grpInfo.ssidBuf);
			P2PSvc_MakeOpChannelObj(pObjList, pConnTopology->grpInfo.opChnl);
		}

		if(P2P_STATUS_SUCCESS != (rtStatus = P2PSvc_Indicate(pP2PSvcInfo, reqBufSize, pObjList)))
		{
			break;
		
		}
	}while(FALSE);

	if(pBuf) P2PSvc_FreeMem(pBuf, reqBufSize);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;

}

BOOLEAN
p2psvc_ValidateConnCapComination(
	IN  u1Byte							reqConnCap,
	IN  u1Byte							rspConnCap
	)
{
	BOOLEAN								bValid = FALSE;

	do

	{
		if(P2PSVC_CONN_CAP_BMP_NEW == reqConnCap)
		{
			if(P2PSVC_CONN_CAP_BMP_NEW == rspConnCap
				|| P2PSVC_CONN_CAP_BMP_CLI == rspConnCap
				|| P2PSVC_CONN_CAP_BMP_GO == rspConnCap
				)
			{
				bValid = TRUE;
				break;
			}
		}
		else if(P2PSVC_CONN_CAP_BMP_CLI == reqConnCap)
		{
			if(P2PSVC_CONN_CAP_BMP_GO == rspConnCap)
			{
				bValid = TRUE;
				break;
			}
		}
		else if(P2PSVC_CONN_CAP_BMP_GO == reqConnCap)
		{
			if(P2PSVC_CONN_CAP_BMP_CLI == rspConnCap)
			{
				bValid = TRUE;
				break;
			}
		}
		else if((P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO) == reqConnCap)
		{
			if(P2PSVC_CONN_CAP_BMP_NEW == rspConnCap
				|| P2PSVC_CONN_CAP_BMP_CLI == rspConnCap
				|| P2PSVC_CONN_CAP_BMP_GO == rspConnCap
				)
			{
				bValid = TRUE;
				break;
			}
		}
		else if((P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO) == reqConnCap)
		{
			if(P2PSVC_CONN_CAP_BMP_CLI == rspConnCap
				|| P2PSVC_CONN_CAP_BMP_GO == rspConnCap
				)
			{
				bValid = TRUE;
				break;
			}
		}
		else
		{
			break;
		}

		// Invalid
		break;

	}while(FALSE);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("reqConnCap: %u, rspConnCap: %u, bValid: %u\n", 
		reqConnCap, rspConnCap, bValid));

	return bValid;
}

BOOLEAN
p2psvc_IsWscPinCollector(
	IN  PP2PSVC_PD_ENTRY 				pPDEntry
	)
{
	BOOLEAN								bWscPinCollector = FALSE;

	RT_ASSERT(pPDEntry->bDone, ("%s(): try to determine WSC PIN collector when PD is not yet done\n", __FUNCTION__));
	
	do
	{
		if(pPDEntry->bInitor)
		{
			PP2PSVC_INITOR_PD_ENTRY		pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;

			bWscPinCollector = (P2P_WPS_CONFIG_METHODS_DISPLAY == pInitorPDEntry->pdReqConfigMethod);
		}
		else
		{
			PP2PSVC_RSPDOR_PD_ENTRY		pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;

			bWscPinCollector = (P2P_WPS_CONFIG_METHODS_KEYPAD == pRspdorPDEntry->configMethod);
		}
	}while(FALSE);

	return bWscPinCollector;
}

BOOLEAN
p2psvc_IsWfdsDefaultPinUsed(
	IN  PP2PSVC_PD_ENTRY 				pPDEntry
	)
{
	BOOLEAN								bWfdsDefaultPinUsed = FALSE;

	RT_ASSERT(pPDEntry->bDone, ("%s(): try to determine WSC PIN collector when PD is not yet done\n", __FUNCTION__));
	
	do
	{
		if(pPDEntry->bInitor)
		{
			PP2PSVC_INITOR_PD_ENTRY		pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;

			bWfdsDefaultPinUsed = (P2P_WPS_CONFIG_METHODS_SVC_DEFAULT_PIN == pInitorPDEntry->pdReqConfigMethod);
		}
		else
		{
			PP2PSVC_RSPDOR_PD_ENTRY		pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;

			bWfdsDefaultPinUsed = (P2P_WPS_CONFIG_METHODS_SVC_DEFAULT_PIN == pRspdorPDEntry->configMethod);
		}
	}while(FALSE);

	return bWfdsDefaultPinUsed;
}

RT_STATUS
p2psvc_AccessTopology_InitorImmediate(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INITOR_PD_ENTRY				pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;

	P2PSVC_FUNC_IN(DBG_LOUD);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("peer connCap: %u, self connCap: %u\n", pPDEntry->connCap, pPDEntry->selfConnCap));

	do
	{
		//
		// In initor immediate success case, the connCap is
		// from rspdor PD rsp, the P2P attrs are also from PD rsp.
		//

		pPDEntry->connTopology.bInitor = TRUE;
		
		switch(pPDEntry->connCap)
		{
		default:
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
			
		case P2PSVC_CONN_CAP_BMP_NEW:
			// Peer agreed to form a new group
			pPDEntry->connTopology.bPersistent = FALSE;

			if(p2psvc_IsWfdsDefaultPinUsed(pPDEntry)
				|| p2psvc_IsWscPinCollector(pPDEntry)
				)
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_INIT_NEGO;
			}
			else
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_WAIT_NEGO;
			}
			
			break;

		case P2PSVC_CONN_CAP_BMP_CLI:
			// Peer agreed to join our group
			pPDEntry->connTopology.bPersistent = FALSE;

			if((P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO) == pPDEntry->selfConnCap
				|| (P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO) == pPDEntry->selfConnCap
				|| (P2PSVC_CONN_CAP_BMP_GO == pPDEntry->selfConnCap) && GetFirstGOPort(pP2PSvcInfo->pAdapter) // our connCap is GO and we have an existing GO
				)
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_JOIN_BY;
			}
			else if(P2PSVC_CONN_CAP_BMP_NEW == pPDEntry->selfConnCap
				|| P2PSVC_CONN_CAP_BMP_GO == pPDEntry->selfConnCap // our connCap is GO but we don't have an existing GO
				)

			{// start auto GO
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_AUTO_GO;

				p2psvc_FillGroupInfo(pPDEntry->selfGrpInfo.grpDevAddr, 
					pPDEntry->selfGrpInfo.grpIntfAddr, 
					pPDEntry->selfGrpInfo.ssidLen, 
					pPDEntry->selfGrpInfo.ssidBuf, 
					pPDEntry->selfGrpInfo.opChnl, 
					&pPDEntry->connTopology.grpInfo);
			}
			else
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("rx invalid peer connCap\n"));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
			
			break;

		case P2PSVC_CONN_CAP_BMP_GO:
			// Peer has an existing GO that we can join
			if(TEST_FLAG(pPDEntry->selfConnCap, P2PSVC_CONN_CAP_BMP_CLI)
				|| TEST_FLAG(pPDEntry->selfConnCap, P2PSVC_CONN_CAP_BMP_NEW)
				)
			{
				pPDEntry->connTopology.bPersistent = FALSE;
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_JOIN;
				rtStatus = p2psvc_GetGroupInfoFromP2PAttrs(posP2PAttrs, &pInitorPDEntry->rxBadPDRspReason, &pPDEntry->connTopology.grpInfo);
				if(RT_STATUS_SUCCESS != rtStatus) break;
			}
			else
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("rx invalid peer connCap\n"));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
				
			break;
		}

		if(RT_STATUS_SUCCESS != rtStatus) 
		{
			break;
		}
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS
p2psvc_AccessTopology_InitorDeferred(
	IN  PP2PSVC_INFO						pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 					pPDEntry,
	IN  POCTET_STRING						posP2PAttrs
	)
{
	RT_STATUS								rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INITOR_PD_ENTRY					pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;

	P2PSVC_FUNC_IN(DBG_LOUD);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("peer connCap: %u, self connCap: %u\n", pPDEntry->connCap, pPDEntry->selfConnCap));

	do
	{
		//
		// In initor deferred success case, the connCap is
		// from self FOPD rsp, the P2P attrs are also from FOPD req.
		//

		pPDEntry->connTopology.bInitor = TRUE;

		switch(pPDEntry->connCap)
		{
		default:
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
			
		case P2PSVC_CONN_CAP_BMP_NEW:
			// To form a new group
			pPDEntry->connTopology.bPersistent = FALSE;

			if(p2psvc_IsWfdsDefaultPinUsed(pPDEntry)
				|| FALSE == p2psvc_IsWscPinCollector(pPDEntry)
				)

			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_WAIT_NEGO;
			}
			else 
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_INIT_NEGO;
			}
				
			break;

		case P2PSVC_CONN_CAP_BMP_CLI:
			// To join an auto GO or existing group
			pPDEntry->connTopology.bPersistent = FALSE;
			pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_JOIN;

			rtStatus = p2psvc_GetGroupInfoFromP2PAttrs(posP2PAttrs, &pInitorPDEntry->rxBadFOPDReqReason, &pPDEntry->connTopology.grpInfo);
			if(RT_STATUS_SUCCESS != rtStatus) break;

			break;

		case P2PSVC_CONN_CAP_BMP_GO:
			// We have an existing GO that peer can join
			pPDEntry->connTopology.bPersistent = FALSE;

			if(GetFirstGOPort(pP2PSvcInfo->pAdapter))
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_JOIN_BY;
			}
			else
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_AUTO_GO;
			}

			p2psvc_FillGroupInfo(pPDEntry->selfGrpInfo.grpDevAddr, 
					pPDEntry->selfGrpInfo.grpIntfAddr, 
					pPDEntry->selfGrpInfo.ssidLen, 
					pPDEntry->selfGrpInfo.ssidBuf, 
					pPDEntry->selfGrpInfo.opChnl, 
					&pPDEntry->connTopology.grpInfo);
			
			break;
		}

		if(RT_STATUS_SUCCESS != rtStatus) 
		{
			break;
		}
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS
p2psvc_AccessTopology_RspdorImmediate(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_RSPDOR_PD_ENTRY				pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;

	P2PSVC_FUNC_IN(DBG_LOUD);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("connCap in PD rsp: %u, self connCap: %u\n", pRspdorPDEntry->pdRspConnCap, pPDEntry->selfConnCap));

	do
	{
		//
		// In rspdor immediate success case, the connCap is
		// from self PD rsp, the P2P attrs are also from PD req.
		//
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("pdRspConnCap: %u\n", pRspdorPDEntry->pdRspConnCap));
		
		switch(pRspdorPDEntry->pdRspConnCap)
		{
		default:
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
			
		case P2PSVC_CONN_CAP_BMP_NEW:
			// To form a new group
			pPDEntry->connTopology.bPersistent = FALSE;

			if(p2psvc_IsWfdsDefaultPinUsed(pPDEntry)
				|| FALSE == p2psvc_IsWscPinCollector(pPDEntry)
				)

			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_WAIT_NEGO;
			}
			else 
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_INIT_NEGO;
			}
			
			break;

		case P2PSVC_CONN_CAP_BMP_CLI:
			// To join an auto GO or existing group
			pPDEntry->connTopology.bPersistent = FALSE;
			pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_JOIN;

			rtStatus = p2psvc_GetGroupInfoFromP2PAttrs(posP2PAttrs, &pRspdorPDEntry->rxBadPDReqReason, &pPDEntry->connTopology.grpInfo);
			if(RT_STATUS_SUCCESS != rtStatus) break;

			break;

		case P2PSVC_CONN_CAP_BMP_GO:
			pPDEntry->connTopology.bPersistent = FALSE;

			if(GetFirstGOPort(pP2PSvcInfo->pAdapter))
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_JOIN_BY;
			}
			else
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_AUTO_GO;
			}

			p2psvc_FillGroupInfo(pPDEntry->selfGrpInfo.grpDevAddr, 
					pPDEntry->selfGrpInfo.grpIntfAddr, 
					pPDEntry->selfGrpInfo.ssidLen, 
					pPDEntry->selfGrpInfo.ssidBuf, 
					pPDEntry->selfGrpInfo.opChnl, 
					&pPDEntry->connTopology.grpInfo);
			
			break;
		}

		if(RT_STATUS_SUCCESS != rtStatus) 
		{
			break;
		}

		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("connAction: %u\n", pPDEntry->connTopology.connAction));

	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS
p2psvc_AccessTopology_RspdorDeferred(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_RSPDOR_PD_ENTRY				pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;

	P2PSVC_FUNC_IN(DBG_LOUD);

	RT_TRACE_F(COMP_P2P, DBG_LOUD, ("peer connCap: %u, self connCap: %u\n", pPDEntry->connCap, pPDEntry->selfConnCap));

	do
	{
		//
		// In rspdor deferred success case, the connCap is
		// from peer FOPD rsp, the P2P attrs are also from FOPD rsp.
		//
		switch(pPDEntry->connCap)
		{
		default:
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
			
		case P2PSVC_CONN_CAP_BMP_NEW:
			// Peer agreed to form a new group
			pPDEntry->connTopology.bPersistent = FALSE;

			if(p2psvc_IsWfdsDefaultPinUsed(pPDEntry)
				|| p2psvc_IsWscPinCollector(pPDEntry)
				)

			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_INIT_NEGO;
			}
			else 
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_WAIT_NEGO;
			}
			
			break;

		case P2PSVC_CONN_CAP_BMP_CLI:
			// Peer agreed to join our group
			pPDEntry->connTopology.bPersistent = FALSE;
			if((P2PSVC_CONN_CAP_BMP_NEW | P2PSVC_CONN_CAP_BMP_GO) == pPDEntry->selfConnCap
				|| (P2PSVC_CONN_CAP_BMP_CLI | P2PSVC_CONN_CAP_BMP_GO) == pPDEntry->selfConnCap
				|| (P2PSVC_CONN_CAP_BMP_GO == pPDEntry->selfConnCap) && GetFirstGOPort(pP2PSvcInfo->pAdapter) // our connCap is GO and we have an existing GO
				)
			{// we have existing group
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_JOIN_BY;
			}
			else if(P2PSVC_CONN_CAP_BMP_GO == pPDEntry->selfConnCap // our connCap is GO but we don't have an existing GO
				|| P2PSVC_CONN_CAP_BMP_NEW == pPDEntry->selfConnCap
				)
			{
				pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_AUTO_GO;

				p2psvc_FillGroupInfo(pPDEntry->selfGrpInfo.grpDevAddr, 
					pPDEntry->selfGrpInfo.grpIntfAddr, 
					pPDEntry->selfGrpInfo.ssidLen, 
					pPDEntry->selfGrpInfo.ssidBuf, 
					pPDEntry->selfGrpInfo.opChnl, 
					&pPDEntry->connTopology.grpInfo);
			}
			else
			{
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("rx invalid peer connCap\n"));
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
			
			break;

		case P2PSVC_CONN_CAP_BMP_GO:
			// Peer has an existing GO that we can join
			pPDEntry->connTopology.bPersistent = FALSE;
			pPDEntry->connTopology.connAction = P2PSVC_CONN_METHOD_JOIN;

			rtStatus = p2psvc_GetGroupInfoFromP2PAttrs(posP2PAttrs, &pRspdorPDEntry->rxBadFOPDRspReason, &pPDEntry->connTopology.grpInfo);
			if(RT_STATUS_SUCCESS != rtStatus) break;

			break;
		}

		if(RT_STATUS_SUCCESS != rtStatus) 
		{
			break;
		}

	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//
// Description:
//		This function determines conn cap to be used in responding PDRsp/FOPDRsp
//		when PDReq/FOPDReq is received.
//		That is:
//			If initor recv FOPDReq, it determines conn cap in FOPDRsp
//			If rspdor recv PDReq, it determines conn cap in PDRsp
//
BOOLEAN
p2psvc_AccessConnectability(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY				pPDEntry,
	OUT pu1Byte							pRspConnCap,
	OUT PP2P_STATUS_CODE				pPDRspStatus
	)
{
	BOOLEAN								bConnectable = FALSE;

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		u4Byte							connCapMapIdx = 0;
		PP2PSVC_CONN_CAP_MAP_ENTRY 		pConnCapEntry = NULL;
		u1Byte							selfConnCap = pP2PSvcInfo->connCap;
		u1Byte							reqConnCap = 0;

		*pRspConnCap = P2PSVC_CONN_CAP_BMP_NONE;
		*pPDRspStatus = P2P_STATUS_MAX;

		if(pPDEntry->bInitor)
		{
			reqConnCap = ((PP2PSVC_INITOR_PD_ENTRY)pPDEntry)->fopdReqConnCap;
		}
		else
		{
			reqConnCap = pPDEntry->pdReqConnCap;
		}
		
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("req: %u, self: %u\n", reqConnCap, selfConnCap));

		if(0 == reqConnCap)
		{
			break;
		}

		for(connCapMapIdx = 0, pConnCapEntry = NULL; 
			P2PSVC_CONN_CAP_BMP_NONE != P2PSvcConnCapMap[connCapMapIdx].reqConnCap; 
			connCapMapIdx++
			)
		{
			pConnCapEntry = &(P2PSvcConnCapMap[connCapMapIdx]);

			// Match conn cap entry
			if(P2PSvcConnCapMap[connCapMapIdx].reqConnCap != reqConnCap) continue;
			if(P2PSvcConnCapMap[connCapMapIdx].selfConnCap != selfConnCap) continue;
			
			pConnCapEntry = &(P2PSvcConnCapMap[connCapMapIdx]);
			break;
		}

		if(NULL == pConnCapEntry)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Configuration not supported: req: %u, self: %u\n", reqConnCap, selfConnCap));
			*pPDRspStatus = P2P_STATUS_FAIL_INCOMPATIBLE_PARAMETERS;
			break;
		}
		else if(P2PSVC_CONN_CAP_BMP_NONE == pConnCapEntry->rspConnCap)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Configuration infeasible: req: %u, self: %u, msg: %s\n", reqConnCap, selfConnCap, pConnCapEntry->pDesc));
			*pPDRspStatus = P2P_STATUS_FAIL_INCOMPATIBLE_PARAMETERS;
			break;
		}

		// Connectable
		*pRspConnCap = pConnCapEntry->rspConnCap;
		*pPDRspStatus = P2P_STATUS_SUCCESS;
		bConnectable = TRUE;
		
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Connectable, msg: %s\n", pConnCapEntry->pDesc));
		
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, bConnectable);

	return bConnectable;
}

u2Byte
p2psvc_GetCorrespondingConfigMethod(
	IN  u2Byte							configMethod
	)
{
	u2Byte 								cm = 0;

	P2PSVC_FUNC_IN(DBG_LOUD);
	
	if((u2Byte)P2P_WPS_CONFIG_METHODS_DISPLAY == cm) 
		cm = (u2Byte)P2P_WPS_CONFIG_METHODS_KEYPAD;
	else if((u2Byte)P2P_WPS_CONFIG_METHODS_KEYPAD == cm) 
		cm = (u2Byte)P2P_WPS_CONFIG_METHODS_DISPLAY;
	else if((u2Byte)P2P_WPS_CONFIG_METHODS_PUSHBUTTON == cm) 
		cm = (u2Byte)P2P_WPS_CONFIG_METHODS_PUSHBUTTON;
	else if((u2Byte)P2P_WPS_CONFIG_METHODS_SVC_DEFAULT_PIN == cm) 
		cm = (u2Byte)P2P_WPS_CONFIG_METHODS_SVC_DEFAULT_PIN;
	else
	{
		cm = 0;
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Invalid configMethod: %u\n", configMethod));
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, RT_STATUS_SUCCESS);

	return cm;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------

//
// Description:
//		To determine the SSID that will be used in PD.
//
VOID
P2PSvc_DeterminePDSessionGOSsid(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	OUT pu1Byte 						ssidBuf,
	OUT pu1Byte 						pSsidLen
	)
{
	static u1Byte 						p2pTechName[] = {'D', 'I', 'R','E', 'C', 'T', '-'};
	static u1Byte						p2pRandomChars[] = {'R', 'T'};
	pu1Byte								pRandField = NULL;

	PP2P_INFO							pP2PInfo = GET_P2P_INFO(pP2PSvcInfo->pAdapter);
	

	*pSsidLen = 0;

	// SSID prefix
	PlatformMoveMemory(ssidBuf + (*pSsidLen), p2pTechName, sizeof(p2pTechName));
	*pSsidLen += sizeof(p2pTechName);

	// Random chars
	PlatformMoveMemory(ssidBuf + (*pSsidLen), p2pRandomChars, sizeof(p2pRandomChars));
	*pSsidLen += sizeof(p2pRandomChars);
	
	// SSID Postfix, optional
	if(pP2PInfo->SSIDPostfixLen)
	{
		PlatformMoveMemory(ssidBuf + (*pSsidLen), pP2PInfo->SSIDPostfixBuf, pP2PInfo->SSIDPostfixLen);
		*pSsidLen += (pP2PInfo->SSIDPostfixLen);
	}
	
	// Display result
	RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "P2PSvc_DeterminePDSessionGOSsid():\n", ssidBuf, *pSsidLen);
	
	return;
	
}

PP2PSVC_OBJ_LIST
P2PSvc_Get_PDEntryObjList(
	IN  PP2PSVC_PD_ENTRY				pPDEntry
	)
{
	if(pPDEntry->bInitor)
	{
		return &(((PP2PSVC_INITOR_PD_ENTRY)pPDEntry)->objList);
	}
	else
	{
		return &(((PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry)->objList);
	}
}

//
// Assumption:
//	pP2PSvcInfo->lock has been acquired before calling this function.
//
VOID
P2PSvc_Free_PDSessionListEntry(
	IN  PP2PSVC_PD_ENTRY				pPDEntry,
	IN  pu4Byte							pListCnt
	)
{
	P2PSVC_FUNC_IN(DBG_LOUD);
	
	RTRemoveEntryListWithCnt((PRT_LIST_ENTRY)pPDEntry, pListCnt);
				
	if(pPDEntry->bInitor)
	{
		p2psvc_Del_InitorPDSessionEntry((PP2PSVC_INITOR_PD_ENTRY)pPDEntry);
	}
	else
	{
		p2psvc_Del_RspdorPDSessionEntry((PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry);
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, RT_STATUS_SUCCESS);

	return;
}

VOID
P2PSvc_Free_PDSessionList(
	IN  PRT_LIST_ENTRY					pListHead,
	IN  pu4Byte							pListCnt
	)
{
	P2PSVC_FUNC_IN(DBG_LOUD);
	
	while(RTIsListNotEmpty(pListHead))
	{
		PVOID 							pEntry = NULL;
		PP2PSVC_PD_ENTRY 				pPDEntry = NULL;
		
		pEntry = RTGetHeadList(pListHead);
		pPDEntry = (PP2PSVC_PD_ENTRY)pEntry;

		RT_PRINT_ADDR(COMP_P2P, DBG_LOUD, "Free PD session with peer:\n", pPDEntry->peerDevAddr);

		P2PSvc_Free_PDSessionListEntry(pPDEntry, pListCnt);
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, RT_STATUS_SUCCESS);

	return;
}

//
// Description:
//		Check if we have on going PD session.
//
BOOLEAN
P2PSvc_PDSessionInProgress(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	OUT PP2PSVC_PD_ENTRY				*ppPDEntry
	)
{
	BOOLEAN								bPDSessionInProgress = FALSE;

	do
	{
		if(ppPDEntry) *ppPDEntry = NULL;

		if(!RTIsListEmpty(&pP2PSvcInfo->pdSessionList))
		{
			PP2PSVC_PD_ENTRY 			pPDEntry = NULL;
			
			pPDEntry = (PP2PSVC_PD_ENTRY)RTGetHeadList(&pP2PSvcInfo->pdSessionList);

			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("There's an outstanding PD session\n"));
			bPDSessionInProgress = TRUE;
			if(ppPDEntry) *ppPDEntry = pPDEntry;
			break;
		}

		bPDSessionInProgress = FALSE;
		break;

	}while(FALSE);

	RT_TRACE_F(COMP_P2P, DBG_WARNING, ("bPDSessionInProgress: %u\n", bPDSessionInProgress));

	return bPDSessionInProgress;
}

RT_STATUS 
P2PSvc_AddPDInitorData(
	IN  PADAPTER						pAdapter, 
	IN  PVOID							infoBuf,
	IN  u4Byte							inBufLen,
	IN  u4Byte							outBufLen
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO						pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);
	PRT_OBJECT_HEADER					pObjHdr = NULL;
	PP2PSVC_OBJ_LIST					pObjList = NULL;

	P2PSVC_CHECK_NULL(pAdapter);

	if(!pP2PSvcInfo->bEnabled) return RT_STATUS_INVALID_STATE;
	
	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PP2PSVC_INITOR_PD_ENTRY 		pPDInitorEntry = NULL;

		pObjHdr = (PRT_OBJECT_HEADER)(infoBuf);
		pObjList = (PP2PSVC_OBJ_LIST)(pObjHdr->Value);

		//4 Check state
		if(P2PSvc_PDSessionInProgress(pP2PSvcInfo, NULL))
		{
			P2PSvc_Free_PDSessionList(&pP2PSvcInfo->pdSessionList, &pP2PSvcInfo->pdSessionListCnt);
		}

		//4 New a PD initor session
		if(RT_STATUS_SUCCESS != (rtStatus = p2psvc_New_InitorPDSessionInfo(pP2PSvcInfo, pObjList, &pPDInitorEntry)))
		{
			break;
		}

		//4 Add record to the list
		PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);
		RTInsertTailListWithCnt(&pP2PSvcInfo->pdSessionList, &pPDInitorEntry->super.List, &pP2PSvcInfo->pdSessionListCnt);
		PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}


RT_STATUS 
P2PSvc_AddPDRspdorData(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  pu1Byte							devAddr,
	IN  u2Byte							configMethod,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_RSPDOR_PD_ENTRY 			pPDRspdorEntry = NULL;
	P2PSVC_REASON						badPDReqReason = P2PSVC_REASON_NONE;


	if(!pP2PSvcInfo->bEnabled) return RT_STATUS_INVALID_STATE;
	
	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PP2PSVC_REQ_INFO_ENTRY			pAdvSvcInfoEntry = NULL;
		u4Byte							peerReqAdvId = 0;
		BOOLEAN							bSvcAvailable = FALSE;		

		//4 Get requested svc
		rtStatus = p2psvc_GetPDRequestedSvc(pP2PSvcInfo, 
							posP2PAttrs, 
							&pAdvSvcInfoEntry, 
							&peerReqAdvId);
		if(RT_STATUS_SUCCESS != rtStatus)
		{
			p2psvc_IndicateRxBadPD(pP2PSvcInfo, devAddr, FALSE, TRUE, P2PSVC_REASON_NO_MATCHING_SVC, posP2PAttrs);
			break;
		}

		if(NULL == pAdvSvcInfoEntry)
		{
			p2psvc_IndicateRxBadPD(pP2PSvcInfo, devAddr, FALSE, TRUE, P2PSVC_REASON_NO_MATCHING_SVC, posP2PAttrs);
			rtStatus = RT_STATUS_NOT_SUPPORT;
			break;
		}

		if(p2psvc_GetAdvSvcAvailability(pP2PSvcInfo, pAdvSvcInfoEntry))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("The requested svc is available\n"));
			bSvcAvailable = TRUE;
		}
		else
		{
			p2psvc_IndicateRxBadPD(pP2PSvcInfo, devAddr, FALSE, TRUE, P2PSVC_REASON_REQ_SVC_NOT_AVAILABLE, posP2PAttrs);
		}

		//4 New a rspdor PD session
		rtStatus = p2psvc_New_RspdorPDSessionInfo(pP2PSvcInfo, 
							devAddr, 
							pAdvSvcInfoEntry, 
							posP2PAttrs, 
							configMethod, 
							&badPDReqReason,
							&pPDRspdorEntry);
		if(RT_STATUS_SUCCESS != rtStatus)
		{
			p2psvc_IndicateRxBadPD(pP2PSvcInfo, devAddr, FALSE, TRUE, badPDReqReason, posP2PAttrs);
			break;
		}

		//4 Determine PD rsp status
		if(FALSE == bSvcAvailable)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("The requested svc is not in available state, set status to 5\n"));
			pPDRspdorEntry->pdRspStatus = P2P_STATUS_FAIL_UNABLE_TO_ACCOMODATE_REQUEST;
			pPDRspdorEntry->pdRspConnCap = P2PSVC_CONN_CAP_BMP_NONE;
			pPDRspdorEntry->super.bDeferred = FALSE;
		}
		else if(pPDRspdorEntry->super.bDeferred)
		{// to defer
			pPDRspdorEntry->pdRspStatus = P2P_STATUS_FAIL_INFORMATION_IS_UNAVAILABLE;
		}
		else
		{// immediate rsp
			BOOLEAN 					bConnectable = FALSE;
			
			bConnectable = p2psvc_AccessConnectability(pP2PSvcInfo, 
				&pPDRspdorEntry->super,  
				&pPDRspdorEntry->pdRspConnCap,		// connCap to be used in responding PD rsp
				&pPDRspdorEntry->pdRspStatus);		// status to be used in responding PD rsp
			
			p2psvc_Set_PDResult(&pPDRspdorEntry->super, pPDRspdorEntry->pdRspStatus, pPDRspdorEntry->pdRspConnCap);

			if(bConnectable)
			{
				rtStatus = p2psvc_AccessTopology_RspdorImmediate(pP2PSvcInfo, &pPDRspdorEntry->super, posP2PAttrs);
				if(RT_STATUS_SUCCESS != rtStatus)
				{
					badPDReqReason = pPDRspdorEntry->rxBadPDReqReason;
					break;
				}
			}
		}

		//4 Add record to the list. Note: If not deferred, the entry will be freed later after PD rsp is sent, 
		//4 see P2PSvc_MakeRspdorPDRspIE()
		RTInsertTailListWithCnt(&pP2PSvcInfo->pdSessionList, 
			&pPDRspdorEntry->super.List, 
			&pP2PSvcInfo->pdSessionListCnt);
		
		//4 Indicate (after status is updated)
		if(bSvcAvailable)
		{// if svc not available, don't bother user
			p2psvc_IndicateRspdorOnPDReq(pP2PSvcInfo, configMethod, posP2PAttrs, &pPDRspdorEntry->super);
		}
	}while(FALSE);

	if(RT_STATUS_SUCCESS != rtStatus)
	{
		p2psvc_IndicateRxBadPD(pP2PSvcInfo, devAddr, FALSE, TRUE, badPDReqReason, posP2PAttrs);
		if(pPDRspdorEntry) p2psvc_Del_RspdorPDSessionEntry(pPDRspdorEntry);
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS
P2PSvc_RspdorSetFOPDReq(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  BOOLEAN							bAccepted	
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;


	do
	{
		PP2PSVC_RSPDOR_PD_ENTRY 		pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;
		
		P2PSvc_UpdateParam(&pRspdorPDEntry->objList, 
			P2PSVC_OBJ_HDR_ID_DATA_USER_ACCEPTED, 
			0, sizeof(bAccepted), &bAccepted);

		pRspdorPDEntry->fopdReqStatus = (bAccepted) 
								? (P2P_STATUS_SUCCESS_ACCEPTED_BY_USER) 
								: (P2P_STATUS_FAIL_REJECTED_BY_USER);
		
		pRspdorPDEntry->fopdReqConnCap = (bAccepted) 
								? (pP2PSvcInfo->connCap) 
								: (P2PSVC_CONN_CAP_BMP_NONE);
		
		RT_TRACE_F(COMP_P2P, DBG_LOUD, ("bUserAccepted set to: %u\n", bAccepted));
	}while(FALSE);

	return rtStatus;
}

//
// Description:
//		PD initiator receives PD rsp from the responder.
//		We update the PD session entry and indicate the PD rsp.
//
RT_STATUS
P2PSvc_InitorOnPDRsp(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  pu1Byte							devAddr,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  u2Byte							reqConfigMethod,
	IN  u2Byte							rspConfigMethod,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INITOR_PD_ENTRY				pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;

	P2PSVC_FUNC_IN(DBG_LOUD);
	
	do
	{
		P2P_STATUS_CODE					pdRspStatus = P2P_STATUS_MAX;
		u1Byte							pdRspConnCap = 0;

		if(!eqMacAddr(devAddr, pInitorPDEntry->super.peerDevAddr))
		{// not from the peer 
			// Don't set rtStatus so session won't be freed
			break;
		}

		if(!p2psvc_MatchAdvId(&pInitorPDEntry->super, posP2PAttrs))
		{
			pInitorPDEntry->rxBadPDRspReason = P2PSVC_REASON_ADV_ID_MISMATCH;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(P2P_STATUS_MAX == (pdRspStatus = p2psvc_GetP2PStatusFromAttrs(posP2PAttrs)))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Drop for no status attr in PD rsp to initor\n"));
			pInitorPDEntry->rxBadPDRspReason = P2PSVC_REASON_MISS_ATTR_STATUS;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(P2P_STATUS_SUCCESS == pdRspStatus
			&& reqConfigMethod != rspConfigMethod)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("drop for reqConfigMethod(%u) != rspConfigMethod(%u) and status in PDRsp is success\n", reqConfigMethod, rspConfigMethod));
			pInitorPDEntry->rxBadPDRspReason = P2PSVC_REASON_CONFIG_METHOD_MISMATCH;
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		if(P2P_STATUS_SUCCESS == pdRspStatus)
		{
			if(0 == (pdRspConnCap = p2psvc_GetConnCapFromAttrs(posP2PAttrs)))
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("peer respond success but conn cap is 0\n"));
				pInitorPDEntry->rxBadPDRspReason = P2PSVC_REASON_MISS_ATTR_CONN_CAP;
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}

			if(FALSE == p2psvc_ValidateConnCapComination(pInitorPDEntry->super.pdReqConnCap, pdRspConnCap))
			{
				pInitorPDEntry->rxBadPDRspReason = P2PSVC_REASON_INVALID_CONN_CAP;
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
		}
		else
		{
			pdRspConnCap = P2PSVC_CONN_CAP_BMP_NONE;
		}

		pInitorPDEntry->super.bDeferred = (P2P_STATUS_FAIL_INFORMATION_IS_UNAVAILABLE == pdRspStatus);
		pInitorPDEntry->pdReqConfigMethod = reqConfigMethod;
		pInitorPDEntry->pdRspRxTime = PlatformGetCurrentTime();
		pInitorPDEntry->pdRspStatus = pdRspStatus;
		pInitorPDEntry->pdRspConnCap = pdRspConnCap;
		
		if(!pInitorPDEntry->super.bDeferred)
		{// immediate rsp, no further frame exchg
			p2psvc_Set_PDResult(&pInitorPDEntry->super, pdRspStatus, pdRspConnCap);
			
			if(P2P_STATUS_SUCCESS == pdRspStatus)
			{// immediate success
				rtStatus = p2psvc_AccessTopology_InitorImmediate(pP2PSvcInfo, pPDEntry, posP2PAttrs);
				if(RT_STATUS_SUCCESS != rtStatus)
				{
					break;
				}
			}
			else
			{// PD explicitly failed
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("PD rsp status: %u\n", pdRspStatus));
			}			
		}

		//4 Indicate on PD rsp
		p2psvc_IndicateInitorOnPDRsp(pP2PSvcInfo, &pInitorPDEntry->super, posP2PAttrs);

		if(P2P_STATUS_SUCCESS == pdRspStatus)
		{// PD succeeded
			P2PSvc_PDSessionSucceeded(pP2PSvcInfo, pPDEntry);
		}
		else if(P2P_STATUS_FAIL_INFORMATION_IS_UNAVAILABLE == pdRspStatus)
		{
		}
		else
		{// PD explicitly failed, but rtStatus is success
			P2PSvc_PDSessionFailed(pP2PSvcInfo, pPDEntry);
		}

	}while(FALSE);

	if(RT_STATUS_SUCCESS != rtStatus)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("free PD session for error: 0x%08X\n", rtStatus));
		p2psvc_IndicateRxBadPD(pP2PSvcInfo, devAddr, TRUE, FALSE, pInitorPDEntry->rxBadPDRspReason, posP2PAttrs);
		P2PSvc_PDSessionFailed(pP2PSvcInfo, pPDEntry);
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//
// Description:
//		PD requestor handles PD req.
// 	Returns error if FOPD rsp shall not be sent.
//
RT_STATUS
P2PSvc_InitorOnFOPDReq(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  pu1Byte							devAddr,
	IN  u2Byte							configMethod,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INITOR_PD_ENTRY				pInitorPDEntry = (PP2PSVC_INITOR_PD_ENTRY)pPDEntry;
	BOOLEAN 							bToSendFOPDRsp = FALSE;

	P2PSVC_FUNC_IN(DBG_LOUD);
	
	do
	{
		P2P_STATUS_CODE					fopdReqStatus = P2P_STATUS_MAX;
		u1Byte							fopdReqConnCap = 0;

		if(!eqMacAddr(devAddr, pInitorPDEntry->super.peerDevAddr))
		{// not from the peer 
			// Don't set rtStatus so session won't be freed
			break;
		}

		if(!p2psvc_MatchAdvId(&pInitorPDEntry->super, posP2PAttrs))
		{
			pInitorPDEntry->rxBadFOPDReqReason = P2PSVC_REASON_ADV_ID_MISMATCH;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		//4 Check if it is actually a FOPD req by checking whether it has status attribute
		if(P2P_STATUS_MAX == (fopdReqStatus = p2psvc_GetP2PStatusFromAttrs(posP2PAttrs)))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Drop for no status attr in PD rsp to initor\n"));
			pInitorPDEntry->rxBadFOPDReqReason = P2PSVC_REASON_MISS_ATTR_STATUS;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		// Check whether PD rsp has been received
		if(0 == pInitorPDEntry->pdRspRxTime)
		{// PD rsp not received, but now receiving the FOPD req
			// Prefast warning
#pragma warning( disable:6063 )
			RT_ASSERT(FALSE == pInitorPDEntry->super.bDeferred, ("%s(): didn't rx PD rsp but bDeferred is set\n"));

			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Rx FOPD req before PD rsp, ignore PD rsp and assume deferred case\n"));
			pInitorPDEntry->super.bDeferred = TRUE;
			pInitorPDEntry->pdReqConfigMethod = p2psvc_GetCorrespondingConfigMethod(configMethod);
		}

		if(P2P_STATUS_SUCCESS_ACCEPTED_BY_USER == fopdReqStatus)
		{
			if(0 == (fopdReqConnCap = p2psvc_GetConnCapFromAttrs(posP2PAttrs)))
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("peer FOPDReq is accepted by user but conn cap is 0\n"));
				pInitorPDEntry->rxBadFOPDReqReason = P2PSVC_REASON_MISS_ATTR_CONN_CAP;
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
		}
		else
		{
			fopdReqConnCap = 0;
		}

		pInitorPDEntry->fopdReqRxTime = PlatformGetCurrentTime();
		pInitorPDEntry->fopdReqStatus = fopdReqStatus;
		pInitorPDEntry->fopdReqConnCap = fopdReqConnCap;

		//4 Check if rejected by peer user, if rejected, FOPD rsp shall not be sent
		
		if(P2P_STATUS_SUCCESS_ACCEPTED_BY_USER == fopdReqStatus)
		{
			BOOLEAN 					bConnectable = FALSE;
			
			bToSendFOPDRsp = TRUE;
			bConnectable = p2psvc_AccessConnectability(pP2PSvcInfo, 
									&pInitorPDEntry->super,
									&pInitorPDEntry->fopdRspConnCap, 
									&pInitorPDEntry->fopdRspStatus);

			p2psvc_Set_PDResult(&pInitorPDEntry->super, pInitorPDEntry->fopdRspStatus, pInitorPDEntry->fopdRspConnCap);

			if(bConnectable)
			{
				rtStatus = p2psvc_AccessTopology_InitorDeferred(pP2PSvcInfo, &pInitorPDEntry->super, posP2PAttrs);
				if(RT_STATUS_SUCCESS != rtStatus)
				{
					break;
				}
			}
		}
		else
		{
			p2psvc_Set_PDResult(&pInitorPDEntry->super, fopdReqStatus, P2PSVC_CONN_CAP_BMP_NONE);

			if(P2P_STATUS_FAIL_REJECTED_BY_USER == fopdReqStatus)
			{// not to respond with PD rsp
				RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Rejected by peer user\n"));
			}
			else
			{// unexpected status, will free the session
				rtStatus = RT_STATUS_INVALID_STATE;
				break;
			}
		}

		//4 Indicate on PD req
		p2psvc_IndicateInitorOnFOPDReq(pP2PSvcInfo, &pInitorPDEntry->super, posP2PAttrs);

	}while(FALSE);

	if(!bToSendFOPDRsp)
	{
		rtStatus = RT_STATUS_FAILURE;
	}

	if(RT_STATUS_SUCCESS != rtStatus)
	{
		if(P2PSVC_REASON_NONE != pInitorPDEntry->rxBadFOPDReqReason)
		{
			p2psvc_IndicateRxBadPD(pP2PSvcInfo, devAddr, TRUE, TRUE, pInitorPDEntry->rxBadFOPDReqReason, posP2PAttrs);
		}
		P2PSvc_PDSessionFailed(pP2PSvcInfo, pPDEntry);
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS
P2PSvc_RspdorOnFOPDRsp(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  pu1Byte							devAddr,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  u2Byte							reqConfigMethod,
	IN  u2Byte							rspConfigMethod,
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_RSPDOR_PD_ENTRY				pRspdorPDEntry = (PP2PSVC_RSPDOR_PD_ENTRY)pPDEntry;

	P2PSVC_FUNC_IN(DBG_LOUD);
	
	do
	{
		P2P_STATUS_CODE					fopdRspStatus = P2P_STATUS_MAX;
		u1Byte							fopdRspConnCap = 0;
		
		PP2PSVC_OBJ_LIST				pObjList = NULL;

		if(!eqMacAddr(devAddr, pRspdorPDEntry->super.peerDevAddr))
		{// not from peer
			// Don't set rtStatus so session won't be freed
			break;
		}

		if(!p2psvc_MatchAdvId(&pRspdorPDEntry->super, posP2PAttrs))
		{
			pRspdorPDEntry->rxBadFOPDRspReason = P2PSVC_REASON_ADV_ID_MISMATCH;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(P2P_STATUS_MAX == (fopdRspStatus = p2psvc_GetP2PStatusFromAttrs(posP2PAttrs)))
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("Drop for no status attr in PD rsp to initor\n"));
			pRspdorPDEntry->rxBadFOPDRspReason = P2PSVC_REASON_MISS_ATTR_STATUS;
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}

		if(P2P_STATUS_SUCCESS == fopdRspStatus
			&& reqConfigMethod != rspConfigMethod)
		{
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("drop for reqConfigMethod(%u) != rspConfigMethod(%u) and status in PDRsp is success\n", reqConfigMethod, rspConfigMethod));
			pRspdorPDEntry->rxBadFOPDRspReason = P2PSVC_REASON_CONFIG_METHOD_MISMATCH;
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		if(P2P_STATUS_SUCCESS == fopdRspStatus)
		{
			if(P2PSVC_CONN_CAP_BMP_NONE == (fopdRspConnCap = p2psvc_GetConnCapFromAttrs(posP2PAttrs)))
			{
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("peer respond success but conn cap is 0\n"));
				pRspdorPDEntry->rxBadFOPDRspReason = P2PSVC_REASON_INVALID_CONN_CAP;
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}

			if(FALSE == p2psvc_ValidateConnCapComination(pRspdorPDEntry->fopdReqConnCap, fopdRspConnCap))
			{
				pRspdorPDEntry->rxBadFOPDRspReason = P2PSVC_REASON_INVALID_CONN_CAP;
				rtStatus = RT_STATUS_INVALID_DATA;
				break;
			}
		}
		else
		{
			fopdRspConnCap = P2PSVC_CONN_CAP_BMP_NONE;
		}
		
		pRspdorPDEntry->fopdRspRxTime = PlatformGetCurrentTime();
		pRspdorPDEntry->fopdRspStatus = fopdRspStatus;
		pRspdorPDEntry->fopdRspConnCap = fopdRspConnCap;

		p2psvc_Set_PDResult(&pRspdorPDEntry->super, fopdRspStatus, fopdRspConnCap);

		if(P2P_STATUS_SUCCESS == fopdRspStatus)
		{
			rtStatus = p2psvc_AccessTopology_RspdorDeferred(pP2PSvcInfo, &pRspdorPDEntry->super, posP2PAttrs);
			if(RT_STATUS_SUCCESS != rtStatus)
			{
				break;
			}
		}
		
		// Indicate
		p2psvc_IndicateRspdorOnFOPDRsp(pP2PSvcInfo, &pRspdorPDEntry->super, fopdRspStatus, posP2PAttrs);
		
		if(P2P_STATUS_SUCCESS == fopdRspStatus)
		{// PD succeeded
			P2PSvc_PDSessionSucceeded(pP2PSvcInfo, pPDEntry);
		}
		else
		{// PD explicitly failed, but rtStatus is success
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("FOPD rsp status: %u\n", fopdRspStatus));
			P2PSvc_PDSessionFailed(pP2PSvcInfo, pPDEntry);
		}
	}while(FALSE);

	if(RT_STATUS_SUCCESS != rtStatus)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("free PD session for error: 0x%08X\n", rtStatus));
		p2psvc_IndicateRxBadPD(pP2PSvcInfo, devAddr, FALSE, FALSE, pRspdorPDEntry->rxBadFOPDRspReason, posP2PAttrs);
		P2PSvc_PDSessionFailed(pP2PSvcInfo, pPDEntry);
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS
P2PSvc_InitorOnSendPDReqFailure(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry,
	IN  u1Byte							p2pStatus	
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	u4Byte								reqBufSize = 0;
	pu1Byte								pBuf = NULL;
	PP2PSVC_OBJ_LIST					pObjList = NULL;

	do
	{
		reqBufSize = 0
			+ P2PSVC_OBJ_LIST_LEN(P2PSvc_Get_PDEntryObjList(pPDEntry)) 		// the whole objs in PD session
			+ (RT_OBJECT_HEADER_SIZE + sizeof(P2P_STATUS_CODE));			// the status code
		
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pBuf\n"));
			break;
		}

		pObjList = (PP2PSVC_OBJ_LIST)pBuf;

		PlatformMoveMemory(pBuf, P2PSvc_Get_PDEntryObjList(pPDEntry), P2PSVC_OBJ_LIST_LEN(P2PSvc_Get_PDEntryObjList(pPDEntry)));
		
		if(0 == P2PSvc_MakeP2PStatusObj(pObjList, p2pStatus))
		{
			rtStatus = RT_STATUS_INVALID_DATA; 
			break;
		}

		pObjList->hdr.Id = P2PSVC_OBJ_HDR_ID_INDIC_INITOR_PD_REQ_SEND_FAILED;
		
		P2PSvc_Indicate(pP2PSvcInfo, reqBufSize, pBuf);
		
		P2PSvc_PDSessionFailed(pP2PSvcInfo, pPDEntry);
		
	}while(FALSE);

	if(pBuf) P2PSvc_FreeMem(pBuf, reqBufSize);

	return rtStatus;
}

RT_STATUS
P2PSvc_RspdorOnSendFOPDReqFailure(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY				pPDEntry,
	IN  u1Byte							p2pStatus	
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	u4Byte								reqBufSize = 0;
	pu1Byte								pBuf = NULL;
	PP2PSVC_OBJ_LIST					pObjList = NULL;
	PRT_OBJECT_HEADER					pStatusObj = NULL;

	do
	{
		reqBufSize = 0
						+ P2PSVC_OBJ_LIST_LEN(P2PSvc_Get_PDEntryObjList(pPDEntry)) 		// the whole objs in PD session
						+ (RT_OBJECT_HEADER_SIZE + sizeof(P2P_STATUS_CODE));			// the status code
		
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, reqBufSize)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pBuf\n"));
			break;
		}

		pObjList = (PP2PSVC_OBJ_LIST)pBuf;

		PlatformMoveMemory(pBuf, P2PSvc_Get_PDEntryObjList(pPDEntry), P2PSVC_OBJ_LIST_LEN(P2PSvc_Get_PDEntryObjList(pPDEntry)));

		if(NULL != (pStatusObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_P2P_STATUS, 0)))
		{
			// invalidate the prev status obj if exist
			pStatusObj->Id = P2PSVC_OBJ_HDR_ID_DATA_UNKNOWN;
		}
		
		if(0 == P2PSvc_MakeP2PStatusObj(pObjList, p2pStatus))
		{
			rtStatus = RT_STATUS_INVALID_DATA; 
			break;
		}

		pObjList->hdr.Id = P2PSVC_OBJ_HDR_ID_INDIC_RSPDOR_FOPD_REQ_SEND_FAILED;
		
		P2PSvc_Indicate(pP2PSvcInfo, reqBufSize, pBuf);

		// NOT to free the PD session because there may be a retry for sending FOPD req
		// and an "in progress" PD session is required when sending FOPD req.
		// See P2PSvc_Set_RspdorFOPDReq.
		// If sending FOPD req failed in the end, it shall be recycled in when P2PDisconnect or
		// P2PSvc_PDSessionAgeFunc is called.
		//P2PSvc_PDSessionFailed(pP2PSvcInfo, pPDEntry);
		
	}while(FALSE);

	if(pBuf) P2PSvc_FreeMem(pBuf, reqBufSize);

	return rtStatus;
}

BOOLEAN
P2PSvc_IsLegacyPD(
	IN  POCTET_STRING					posP2PAttrs
	)
{
	RT_STATUS							rtStatus = RT_STATUS_SUCCESS;
	BOOLEAN								bLegacyPD = TRUE;

	P2PSVC_CHECK_NULL(posP2PAttrs);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		OCTET_STRING					osAdvId = {NULL, 0};
	
		// If PD req does not contain adv-id attr, it should be from a legacy P2P Device
		bLegacyPD = (P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_ADV_ID_INFO, 4 + 6, &osAdvId)) 
						? (FALSE) 
						: (TRUE);

		if(bLegacyPD)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Rx legacy PD (PD w/o AdvID attr)\n"));
		}

		break;

	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);
	
	return bLegacyPD;
}

VOID
P2PSvc_PDSessionSucceeded(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry
	)
{
	P2PSvc_Free_PDSessionListEntry(pPDEntry, &pP2PSvcInfo->pdSessionListCnt);
	return;
}

VOID
P2PSvc_PDSessionFailed(
	IN  PP2PSVC_INFO					pP2PSvcInfo,
	IN  PP2PSVC_PD_ENTRY 				pPDEntry
	)
{
	P2PSvc_Free_PDSessionListEntry(pPDEntry, &pP2PSvcInfo->pdSessionListCnt);
	return;
}

VOID
P2PSvc_PDSessionAgeFunc(
	IN  PP2PSVC_INFO					pP2PSvcInfo
	)
{
	PRT_LIST_ENTRY						pListHead = &pP2PSvcInfo->pdSessionList;
	PRT_LIST_ENTRY 						pEntry = RTGetHeadList(&pP2PSvcInfo->pdSessionList);
	
	while(pEntry != pListHead)
	{
		PP2PSVC_PD_ENTRY 				pPDEntry = (PP2PSVC_PD_ENTRY)pEntry;
		PRT_LIST_ENTRY 					pForeEntry = RTNextEntryList(pEntry);
		
		if(P2PSVC_TIMEOUT_PD_SESSION_TIMEOUT_uS < PlatformGetCurrentTime() - pPDEntry->createTime)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("Timeout an old PD session\n"));
			P2PSvc_Free_PDSessionListEntry(pPDEntry, &pP2PSvcInfo->pdSessionListCnt);
		}

		pEntry = pForeEntry;
	}

	return;
}

#endif // #if (P2PSVC_SUPPORT == 1)
#endif // #if (P2P_SUPPORT == 1)