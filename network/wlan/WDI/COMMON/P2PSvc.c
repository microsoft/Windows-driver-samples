// 
// Description:
//		Implement P2PSvc interface to other part of the driver.
//
// Namespace:
//		Local: p2psvc
//		Exported: P2PSvc
//

#include "Mp_Precomp.h"

#include "P2PSvc_Internal.h"

#if (P2P_SUPPORT == 1)
#if (P2PSVC_SUPPORT == 1)

//3 -----------------------------------------------------------------------------
//3 Local
//3  -----------------------------------------------------------------------------

//
// Description:
//		This function frees the P2PSvc info of pAdapter.
//		Everything that is dynamically allocated for the P2PSvc info is freed.
//
VOID
p2psvc_Free_P2PSvcInfo(
	IN  PP2P_INFO					pP2PInfo
	)
{
	PP2PSVC_INFO 					pP2PSvcInfo = NULL;

	P2PSVC_CHECK_NULL(pP2PInfo);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		if(NULL == pP2PInfo) break;

		pP2PSvcInfo = (PP2PSVC_INFO)pP2PInfo->pP2PSvcInfo;

		if(!pP2PSvcInfo->bEnabled) break;

		PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);
		{
			P2PSvc_Free_AdvSvcList(&pP2PSvcInfo->advSvcList, &pP2PSvcInfo->advSvcListCnt, 0);
			P2PSvc_Free_SeekReqList(&pP2PSvcInfo->seekReqList, &pP2PSvcInfo->seekReqListCnt, 0);
			P2PSvc_Free_SearchResultObjList(&pP2PSvcInfo->searchResultList, &pP2PSvcInfo->searchResultListCnt);
			P2PSvc_Free_PDSessionList(&pP2PSvcInfo->pdSessionList, &pP2PSvcInfo->pdSessionListCnt);
 
			pP2PSvcInfo->bEnabled = FALSE;
		}
		PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

		PLATFORM_FREE_RT_SPINLOCK(pP2PSvcInfo->lock);
	}while(FALSE);

	if(pP2PSvcInfo)
	{
		PlatformZeroMemory(pP2PSvcInfo, sizeof(P2PSVC_INFO));
	}

	RT_TRACE_F(COMP_P2P, DBG_WARNING, ("%-32s: %u\n", "Alloc count", gP2PSvcMemAllocCount));
	RT_TRACE_F(COMP_P2P, DBG_WARNING, ("%-32s: %u\n", "Free count", gP2PSvcMemFreeCount));

	gP2PSvcMemAllocCount = 0;
	gP2PSvcMemFreeCount = 0;

	P2PSVC_FUNC_OUT(DBG_LOUD, 0);

	return;
}

RT_STATUS 
p2psvc_Set_Enable(
	IN  PADAPTER					pAdapter,
	IN  PVOID						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT	pu4Byte						pBytesWritten,
	OUT	pu4Byte						pBytesRead,
	OUT	pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2P_INFO						pP2PInfo = GET_P2P_INFO(pAdapter);
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);

	P2PSVC_CHECK_NULL(pAdapter);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PP2PSVC_OBJ_LIST			pObjList = (PP2PSVC_OBJ_LIST)(((PRT_OBJECT_HEADER)infoBuf)->Value);
		PRT_OBJECT_HEADER			pBEnableObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_BENABLE, 0);
		BOOLEAN 					bEnable = *(PBOOLEAN)pBEnableObj->Value;
		
		if(bEnable)
		{// To enable
			if(pP2PSvcInfo->bEnabled)
			{// already enabled => free the old and re-init 
				p2psvc_Free_P2PSvcInfo(pP2PInfo);
				RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Already enable, the old one will be freed!\n"));
			}
			
			PlatformZeroMemory(pP2PSvcInfo, sizeof(P2PSVC_INFO));
			
			PLATFORM_INIT_RT_SPINLOCK(pP2PSvcInfo->lock);
			
			PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);
			{
				pP2PSvcInfo->bEnabled = TRUE;
				RTInitializeListHead(&pP2PSvcInfo->advSvcList);
				RTInitializeListHead(&pP2PSvcInfo->seekReqList);
				RTInitializeListHead(&pP2PSvcInfo->searchResultList);
				RTInitializeListHead(&pP2PSvcInfo->pdSessionList);

				pP2PSvcInfo->advSvcListCnt = 0;
				pP2PSvcInfo->seekReqListCnt = 0;
				pP2PSvcInfo->searchResultListCnt = 0;
				pP2PSvcInfo->pdSessionListCnt = 0;
				
				pP2PSvcInfo->connCap = P2PSVC_CONN_CAP_BMP_NONE;
				pP2PSvcInfo->pAdapter = pAdapter;
			}
			PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);
		}
		else
		{// To disable
			if(!pP2PSvcInfo->bEnabled)
			{
				rtStatus = RT_STATUS_INVALID_STATE;
				break;
			}
			
			p2psvc_Free_P2PSvcInfo(pP2PInfo);
		}
	}while(FALSE);

	*pBytesRead = 0;

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;	
}

RT_STATUS 
p2psvc_Query_Enable(
	IN  PADAPTER					pAdapter,
	IN  PVOID						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT	pu4Byte						pBytesWritten,
	OUT	pu4Byte						pBytesRead,
	OUT	pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);
	PRT_OBJECT_HEADER				pObjHdr = NULL;

	P2PSVC_CHECK_NULL(pAdapter);

	P2PSVC_FUNC_IN(DBG_LOUD);
	
	do
	{
		*pBytesNeeded = RT_OBJECT_HEADER_SIZE + sizeof(u1Byte);

		if(outBufLen < *pBytesNeeded)
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Buffer (%d) too short for required (%d)!\n", outBufLen, *pBytesNeeded));
			rtStatus = RT_STATUS_INVALID_LENGTH;
			break;
		}

		pObjHdr = (PRT_OBJECT_HEADER)(infoBuf);
		
		RT_ASSIGN_OBJECT_HEADER(
				pObjHdr,
				RT_OB_HDR_TYPE_QUERY,
				P2PSVC_OBJ_HDR_ID_DATA_BENABLE,
				pP2PSvcInfo->curVer,
				sizeof(u1Byte));

		*((pu1Byte)(pObjHdr->Value)) = (P2PSVC_ENABLED(pP2PSvcInfo)) ? (1) : (0);

		*pBytesWritten = *pBytesNeeded;

	}while(FALSE);

	*pBytesRead = 0;

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//
// Description:
//		This table maps the request from upper layer to a specifi handler.
//
P2PSVC_REQUEST_ID_MAP lP2PSvcRequestIdMap[] =
{
	// Set Enable/Disable
	{
		RT_OB_HDR_TYPE_SET, 	
		P2PSVC_OBJ_HDR_ID_ACT_ENABLE, 	
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		p2psvc_Set_Enable			
	},
	
	// Query enabled
	{
		RT_OB_HDR_TYPE_QUERY,	
		P2PSVC_OBJ_HDR_ID_ACT_ENABLE, 	
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		p2psvc_Query_Enable		
	},

	// Set seek
	{
		RT_OB_HDR_TYPE_SET, 	
		P2PSVC_OBJ_HDR_ID_ACT_SEEK, 		
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		P2PSvc_Set_Seek
	},

	// Set cancel seek
	{
		RT_OB_HDR_TYPE_SET, 	
		P2PSVC_OBJ_HDR_ID_ACT_CANCEL_SEEK, 		
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		P2PSvc_Set_CancelSeek
	},

	// Set advertise 
	{
		RT_OB_HDR_TYPE_SET, 	
		P2PSVC_OBJ_HDR_ID_ACT_ADVERTISE, 
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		P2PSvc_Set_AdvSvc	
	},

	// Set cancel advertise
	{
		RT_OB_HDR_TYPE_SET,	
		P2PSVC_OBJ_HDR_ID_ACT_CANCEL_ADVERTISE, 
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		P2PSvc_Set_CancelAdvSvc	
	},

	// Set svc status
	{
		RT_OB_HDR_TYPE_SET,	
		P2PSVC_OBJ_HDR_ID_ACT_SVC_STATUS, 
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		P2PSvc_Set_SvcStatus	
	},	

	// PD Req data
	{
		RT_OB_HDR_TYPE_SET, 	
		P2PSVC_OBJ_HDR_ID_ACT_PD_REQ, 
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		P2PSvc_Set_PDReq	
	},

	// Conn cap
	{
		RT_OB_HDR_TYPE_SET, 	
		P2PSVC_OBJ_HDR_ID_ACT_CONN_CAP, 
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		P2PSvc_Set_ConnCap	
	},

	// Follow on PD req
	{
		RT_OB_HDR_TYPE_SET, 	
		P2PSVC_OBJ_HDR_ID_ACT_FOPD_REQ, 
		P2PSVC_MIN_SUPPORT_VER, 
		P2PSVC_MAX_SUPPORT_VER, 
		P2PSvc_Set_RspdorFOPDReq	
	},
	
	// ===== Insert new map above this line ===== //
	{
		RT_OB_HDR_TYPE_UNKNOWN, 
		P2PSVC_OBJ_HDR_ID_UNKNOWN, 
		P2PSVC_OBJECT_VERSION_INVLAID, 
		P2PSVC_OBJECT_VERSION_INVLAID, 
		NULL
	}
};


//3 -----------------------------------------------------------------------------
//3  Exported
//3 -----------------------------------------------------------------------------

BOOLEAN
P2PSvc_Enabled(
	IN  PVOID						pvP2PSvcInfo
	)
{
	return P2PSVC_ENABLED((PP2PSVC_INFO)pvP2PSvcInfo);
}

VOID
P2PSvc_Dump(
	IN  PADAPTER					pAdapter
	)
{
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);
		
	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return;

	RT_TRACE(COMP_P2P, DBG_LOUD, ("\n"));
	RT_TRACE(COMP_P2P, DBG_LOUD, (", nAdvSvc: %u", pP2PSvcInfo->advSvcListCnt));
	RT_TRACE(COMP_P2P, DBG_LOUD, (", nSeekReq: %u", pP2PSvcInfo->seekReqListCnt));
	RT_TRACE(COMP_P2P, DBG_LOUD, (", nSearchResult: %u", pP2PSvcInfo->searchResultListCnt));
	RT_TRACE(COMP_P2P, DBG_LOUD, (", nPDSession: %u", pP2PSvcInfo->pdSessionListCnt));

	RT_TRACE(COMP_P2P, DBG_LOUD, (", allocCnt: %u", gP2PSvcMemAllocCount));
	RT_TRACE(COMP_P2P, DBG_LOUD, (", freeCnt: %u", gP2PSvcMemFreeCount));

	return;
}

RT_STATUS
P2PSvc_Request(
	IN  PADAPTER					pAdapter,
	IN  PVOID						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);
	P2PSvc_ReqHdlr					reqHdlr = NULL;
	PRT_OBJECT_HEADER				pObjHdr = NULL;
	u4Byte							idx = 0;
	BOOLEAN							bMismatchVer = FALSE;
	PP2PSVC_OBJ_LIST				pObjList = NULL;

	P2PSVC_CHECK_NULL(pAdapter);
	P2PSVC_CHECK_NULL(pBytesWritten);
	P2PSVC_CHECK_NULL(pBytesRead);
	P2PSVC_CHECK_NULL(pBytesNeeded);

	*pBytesWritten = 0;
	*pBytesRead = 0;
	*pBytesNeeded = RT_OBJECT_HEADER_SIZE;

	P2PSVC_FUNC_IN(DBG_LOUD);
	
	do
	{
		if(outBufLen < *pBytesNeeded)
		{
			RT_TRACE_F(COMP_P2P | COMP_OID_SET, DBG_WARNING, ("[WARNING] Buffer (%d) too short for required (%d)!\n", outBufLen, *pBytesNeeded));
			rtStatus = RT_STATUS_BUFFER_TOO_SHORT;
			break;
		}

		RT_PRINT_DATA(COMP_P2P, DBG_TRACE, "P2PSvc_Request:", infoBuf, inBufLen);

		pObjHdr = (PRT_OBJECT_HEADER)infoBuf;

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_ValidateReqInfo(infoBuf, inBufLen, pBytesNeeded)))
		{
			break;
		}

		if(RT_OB_HDR_TYPE_SET == pObjHdr->Type)
		{
			// The ObjList is right after an obj header. 
			// The obj header is used for oid.
			pObjList = (PP2PSVC_OBJ_LIST)(((PRT_OBJECT_HEADER)infoBuf)->Value);
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_ValidateActionParam(pObjHdr->Id, pObjList)))
			{
				break;
			}

			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_ValidateParamContent(pObjList)))
			{
				break;
			}
		}

		// Check if P2PSvc is enabled if the action is not query/set enable
		if(P2PSVC_OBJ_HDR_ID_ACT_ENABLE != pObjHdr->Id)
		{
			if(!P2PSVC_ENABLED(pP2PSvcInfo))
			{
				rtStatus = RT_STATUS_INVALID_STATE;
				break;
			}
		}

		for(idx = 0; P2PSVC_OBJ_HDR_ID_UNKNOWN != lP2PSvcRequestIdMap[idx].id; idx++)
		{
			// Check Request is matching
			if(pObjHdr->Type == lP2PSvcRequestIdMap[idx].type && pObjHdr->Id == lP2PSvcRequestIdMap[idx].id)
			{// type and id matched
				// Check Version
				if(pObjHdr->Version >= lP2PSvcRequestIdMap[idx].minVer	
					&& pObjHdr->Version <= lP2PSvcRequestIdMap[idx].maxVer)
				{
					reqHdlr = lP2PSvcRequestIdMap[idx].reqHdlr;
					RT_TRACE_F(COMP_P2P, DBG_LOUD, ("request type = %d, pObjHdr->Id = 0x%08X\n", pObjHdr->Type, pObjHdr->Id));
					break;
				}
				else
				{
					bMismatchVer = TRUE;
				}		
			}
		}

		if(reqHdlr)
		{ // Execute the request handler
			rtStatus = reqHdlr(pAdapter, infoBuf, inBufLen, outBufLen, pBytesWritten, pBytesRead, pBytesNeeded);
			break;
		}
		else if(bMismatchVer)
		{
			RT_TRACE_F(COMP_P2P | COMP_OID_SET, DBG_WARNING, ("[WARNING] Mismatch version (%d) for req ID (%d)!\n", pObjHdr->Version, pObjHdr->Type));
			rtStatus = RT_STATUS_NOT_SUPPORT;
			break;
		}
		else
		{
			RT_TRACE_F(COMP_P2P | COMP_OID_SET, DBG_WARNING, ("[WARNING] Unknown req ID (%d)!\n", pObjHdr->Type));
			rtStatus = RT_STATUS_NOT_RECOGNIZED;
			break;
		}
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//
// Description:
//		P2PSvcInfo has the same life time of P2PInfo.
//
RT_STATUS
P2PSvc_AllocP2PSvcInfo(
	IN  PP2P_INFO 					pP2PInfo
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;

	P2PSVC_CHECK_NULL(pP2PInfo);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = PlatformAllocateMemory(pP2PInfo->pAdapter, &pP2PInfo->pP2PSvcInfo, sizeof(P2PSVC_INFO))))
		{
			break;
		}

		PlatformZeroMemory(pP2PInfo->pP2PSvcInfo, sizeof(P2PSVC_INFO));
		
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//
// Description:
//		P2PSvcInfo has the same life time of P2PInfo.
//
RT_STATUS
P2PSvc_Free_P2PSvcInfo(
	IN  PP2P_INFO 					pP2PInfo
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pP2PInfo->pP2PSvcInfo;

	P2PSVC_CHECK_NULL(pP2PInfo);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{	
		p2psvc_Free_P2PSvcInfo(pP2PInfo);

		PlatformFreeMemory(pP2PSvcInfo, sizeof(P2PSVC_INFO));

	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//-----------------------------------------------------------------------------
// Process probe event
//-----------------------------------------------------------------------------

RT_STATUS
P2PSvc_OnP2PScanComplete(
	IN  PVOID						pvP2PSvcInfo
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PRT_LIST_ENTRY				pListHead = &pP2PSvcInfo->searchResultList;
		PRT_LIST_ENTRY 				pEntry = NULL;
		
		for(pEntry = RTGetHeadList(pListHead); pEntry != pListHead; pEntry = RTNextEntryList(pEntry))
		{
			PP2PSVC_SR_LIST_ENTRY	pSREntry = (PP2PSVC_SR_LIST_ENTRY)pEntry;
			
			if(pSREntry->bDirty)
			{
				P2PSvc_Indicate(pP2PSvcInfo, P2PSVC_OBJ_LIST_LEN(&pSREntry->srObjList), &pSREntry->srObjList);
				pSREntry->bDirty = FALSE;
			}
		}
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock); 

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//
// Description:
// 	On receiving dev disc complete, we need to free:
//		- all the seek requests
//		- the search result
//
RT_STATUS
P2PSvc_OnDevDiscComplete(
	IN  PVOID						pvP2PSvcInfo
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{// To erase all seek reqs (if SD is needed, it's done when probing)
		P2PSvc_Free_SeekReqList(&pP2PSvcInfo->seekReqList, &pP2PSvcInfo->seekReqListCnt, 0);
		P2PSvc_Free_SearchResultObjList(&pP2PSvcInfo->searchResultList, &pP2PSvcInfo->searchResultListCnt);
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock); 

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//
// Description:
// 	On receiving P2PProbeRsp, 
//		- 
//check for every seek req to see if it is done by 
//		checking whether it has svc-info-req or svc-name is ended with * (i.e., whether SD is needed).
//	 	If it is done, indicate search result.
// 	If it needs further SD req, indicate search result up when SD req is done.
//
RT_STATUS
P2PSvc_OnProbeRsp(
	IN  PVOID						pvP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  POCTET_STRING				posP2PAttrs
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;
	OCTET_STRING					osAdvSvcInfo = {NULL, 0};

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(devAddr);
	P2PSVC_CHECK_NULL(posP2PAttrs);

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	// Filter out ProbeRsp w/o dev info and adv svc info (not from a P2PS dev)
	if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_ADV_SVC_INFO, 4, &osAdvSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_TRACE);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PRT_LIST_ENTRY				pEntry = NULL;
		PP2PSVC_SR_LIST_ENTRY 		pSREntry = NULL;

		for(pEntry = RTGetHeadList(&pP2PSvcInfo->seekReqList); pEntry != &pP2PSvcInfo->seekReqList; pEntry = RTNextEntryList(pEntry))
		{
			PP2PSVC_REQ_INFO_ENTRY	pSeekInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;
			PRT_OBJECT_HEADER		pSeekDevAddrObj = NULL;
			PP2PSVC_SR_LIST_ENTRY 	pSREntryInList = NULL;
			PRT_OBJECT_HEADER		pSvcInfoReqObj = NULL;
			PRT_OBJECT_HEADER		pSeekSearchIdObj = NULL;
			u1Byte					searchId = 0;
			BOOLEAN 				bDuplicatedEntry = FALSE;

			// Address filter
			pSeekDevAddrObj = P2PSvc_GetParam(&pSeekInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_DEV_ADDR, 0);
			if(!MacAddr_isBcst(pSeekDevAddrObj->Value) && !eqMacAddr(pSeekDevAddrObj->Value, devAddr))
			{// searching for specific dev and the probe rsp is not from it => ignore it
				continue;
			}

			// Get self search id, this is used as an key to find previous search result
			pSeekSearchIdObj = P2PSvc_GetParam(&pSeekInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, 0);
			searchId = *(pu1Byte)pSeekSearchIdObj->Value;

			// If the search result entry has been updated by SD rsp, don't update it again with ProbeRsp
			if(!P2PSvc_ToProcessProbeRsp(pP2PSvcInfo, searchId, devAddr))
			{
				break;
			}

			// Make data for indication
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_MakeSearchResultDataFromProbeRsp(pP2PSvcInfo, pSeekInfoEntry, devAddr, posP2PAttrs, osAdvSvcInfo, &pSREntry)))
			{
				break;
			}

			P2PSVC_CHECK_NULL(pSREntry);

			// Update the search result list	
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_UpdateSearchResult(pP2PSvcInfo, searchId, devAddr, pSREntry, &bDuplicatedEntry, &pSREntryInList)))
			{
				P2PSvc_FreeMem(pSREntry, P2PSVC_SR_LIST_LEN(pSREntry));
				break;
			}

			pSREntryInList->probeRspRxTime = PlatformGetCurrentTime();

			// Check whether to indicate
			if(bDuplicatedEntry)
			{// already have same entry in the list => free this one
				P2PSvc_FreeMem(pSREntry, P2PSVC_SR_LIST_LEN(pSREntry));
			}
			else
			{
				pSREntryInList->bDirty = TRUE;
			}

			//4 Check if SD is required
			if(P2PSvc_SeekReqNeedSD(&pSeekInfoEntry->objList, pSREntryInList, &pSvcInfoReqObj))
			{
				//1 Note that SD req is sent for each seek req that requires SD req, each has its own transaction id
				//1 The transaction id is set to the search id so we can distinguish from the SD rsp of different seek req

				if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_SDReq(pP2PSvcInfo, devAddr, pSeekInfoEntry, pSvcInfoReqObj, *(pu1Byte)pSeekSearchIdObj->Value)))
				{
					break;
				}

				pSREntryInList->sdReqTxTime = PlatformGetCurrentTime();
			}
			else if(!P2PSvc_SeekReqIsBcst(&pSeekInfoEntry->objList))
			{// search for specific dev => done here
				PRT_LIST_ENTRY 					pForeEntry = RTForeEntryList(pEntry);

				// Remove the seek entry since it is completed
				RTRemoveEntryListWithCnt(pEntry, &pP2PSvcInfo->seekReqListCnt);
				P2PSvc_FreeMem(pEntry, P2PSVC_REQ_INFO_LIST_LEN(pSeekInfoEntry));
				pEntry = pForeEntry;	
			}
			else
			{// search for all dev addr
				continue; // continue searching
			}
		}
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock); 

	P2PSVC_FUNC_OUT(DBG_TRACE, rtStatus);

	return rtStatus;
}

//
// Description
//		Make P2PIE for ProbeRsp. According to addendum v0.1, it shall contain 
// 	the following attributes:
//		* adv-svc-info (id: 23)
//
//		If it is too big to fit into a single P2PIE, it will be splited into 
//		several.
//
RT_STATUS
P2PSvc_MakeProbeRspIE(
	IN  PVOID						pvP2PSvcInfo,
	IN  POCTET_STRING				posP2PAttrs,
	OUT FRAME_BUF					*pBuf,
	OUT PBOOLEAN					pbToSendProbeRsp
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = NULL;
	PP2P_INFO						pP2PInfo = NULL;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(posP2PAttrs);
	P2PSVC_CHECK_NULL(pBuf);
	P2PSVC_CHECK_NULL(pbToSendProbeRsp);

	pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;
	pP2PInfo = (PP2P_INFO)(CONTAINING_RECORD(pP2PSvcInfo, P2P_INFO, pP2PSvcInfo));

	*pbToSendProbeRsp = TRUE; // to send probe rsp by default

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	do
	{
		OCTET_STRING				osSvcNameHash = {NULL, 0};

		if(NULL == posP2PAttrs)
		{// no P2P attr, just ignore
			rtStatus = RT_STATUS_SUCCESS;
			break;
		}

		if(!P2PSvc_GetP2PAttr(posP2PAttrs, P2P_ATTR_SVC_HASH, 6, &osSvcNameHash))
		{// no svc-nams-hash, just return success to make respond with a normal P2P ProbeRsp
			if(P2P_CLIENT == pP2PInfo->Role)
			{// if acting as P2P Client, only respond to Probe when it is a WFDS Probe
				*pbToSendProbeRsp = FALSE;
			}
			break;
		}

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_MakeAdvSvcInfo(pP2PSvcInfo, &osSvcNameHash, pBuf)))
		{
			*pbToSendProbeRsp = FALSE;				// peer is probing a svc we don't have => not to respond
			break;
		}
	}while(FALSE);

	return rtStatus;
}

RT_STATUS
P2PSvc_MakeProbeReqIE(
	IN  PVOID						pvP2PSvcInfo,
	OUT FRAME_BUF					*pBuf
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(pBuf);

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	do
	{		
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_MakeSvcNameHash(pP2PSvcInfo, pBuf)))
		{
			break;
		}

	}while(FALSE);

	return rtStatus;
}

//-----------------------------------------------------------------------------
// Process SD event
//-----------------------------------------------------------------------------

//
// Description:
//		Handle SD req for P2P Services.
//		The ANQP query data field of the SD req shall contain:
//			svc-name-len(1), svc-name(n), svc-info-req-len(1), svc-info-req(n)
//		- Do exact svc-name matching if the query svc-name does not contain *
//			otherwise match substring.
//		- Add svc-info if svc-info-req len is not 0
//		The ANQP query rsp data is constructed 
//
RT_STATUS
P2PSvc_OnSDReq(
	IN  PVOID						pvP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  u1Byte						dlgToken,
	IN  u1Byte						SDReqRecvdSize, 
	IN  PVOID					 	pvSDReqRecvd,
	OUT PBOOLEAN					pbToSendSDRsp
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;
	PP2P_SERVICE_REQ_TLV			pSvcReqTlv = (PP2P_SERVICE_REQ_TLV)pvSDReqRecvd;
	pu1Byte							pBuf = NULL;
	FRAME_BUF						fbuf;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(devAddr);
	P2PSVC_CHECK_NULL(pvSDReqRecvd);
	P2PSVC_CHECK_NULL(pbToSendSDRsp);

	*pbToSendSDRsp = TRUE;
	
	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_INVALID_STATE;
	if(P2P_SD_PROTOCOL_P2PSVC != pSvcReqTlv->ServiceDesc.ServiceType) return RT_STATUS_INVALID_DATA;

	P2PSVC_FUNC_IN(DBG_TRACE);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{	
		u1Byte						svcNameLen = 0;
		pu1Byte						svcNameBuf = NULL;
		u1Byte						svcInfoReqLen = 0;
		pu1Byte						svcInfoReqBuf = NULL;

		u1Byte						nAddedAdvSvc = 0;

		PRT_LIST_ENTRY				pEntry = NULL;

		// Get info from the SD req
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_ParseAnqpQueryReq(pSvcReqTlv, &svcNameLen, &svcNameBuf, &svcInfoReqLen, &svcInfoReqBuf)))
		{
			break;
		}

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pP2PSvcInfo->pAdapter, &pBuf, P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN)))
		{
			RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] Failed to allocate memory for pSvcInfoDescBuf\n"));
			break;
		}

		FrameBuf_Init(P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN, 0, pBuf, &fbuf);

		// For each advertised svc, check if svc-name matches
		for(pEntry = RTGetHeadList(&pP2PSvcInfo->advSvcList); pEntry != &pP2PSvcInfo->advSvcList; pEntry = RTNextEntryList(pEntry))
		{
			PP2PSVC_REQ_INFO_ENTRY		pAdvInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;

			// Match svc-name
			if(!P2PSvc_SDMatchSvcName(pAdvInfoEntry, svcNameLen, svcNameBuf)) continue;

			// Attach svc info desc to 
			P2PSvc_MakeSvcInfoDesc(pP2PSvcInfo, pAdvInfoEntry, svcInfoReqLen, svcInfoReqBuf, &fbuf);

			nAddedAdvSvc++;
		}
		
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_SDRsp(pP2PSvcInfo, devAddr, dlgToken, pSvcReqTlv->TransactionID, nAddedAdvSvc, &fbuf)))
		{
			break;
		}
		
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	if(RT_STATUS_SUCCESS == rtStatus)
	{// SD rsp has been sent in P2PSvc_SDRsp()
		*pbToSendSDRsp = FALSE;
	}

	if(pBuf)
	{
		P2PSvc_FreeMem(pBuf, P2P_MAX_SERVICE_DESCRIPTOR_BUFFER_LEN);
	}

	P2PSVC_FUNC_OUT(DBG_TRACE, rtStatus);

	return rtStatus;
}

//
// Description:
//		Handle SD rsp with P2PSvc protocol type.
//		
//
RT_STATUS
P2PSvc_OnSDRsp(
	IN  PVOID						pvP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  u1Byte						transactionId,
	IN  PVOID						pvSvcRspTlv, 
	OUT PBOOLEAN					pbNeedFurtherProcess
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;
	PP2P_SERVICE_RSP_TLV			pSvcRspTlv = (PP2P_SERVICE_RSP_TLV)pvSvcRspTlv;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(devAddr);
	P2PSVC_CHECK_NULL(pvSvcRspTlv);

	*pbNeedFurtherProcess = TRUE;

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_INVALID_STATE;

	// If P2PSvc is enabled, there shall be no further process of this SD rsp,
	// otherwise P2P state will be changed incorrectly in P2P_OnSDRsp()
	*pbNeedFurtherProcess = FALSE;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PRT_LIST_ENTRY				pEntry = NULL;

		for(pEntry = RTGetHeadList(&pP2PSvcInfo->seekReqList); pEntry != &pP2PSvcInfo->seekReqList; pEntry = RTNextEntryList(pEntry))
		{
			PP2PSVC_REQ_INFO_ENTRY	pSeekInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;
			PRT_OBJECT_HEADER		pSeekSearchIdObj = NULL;
			u1Byte					searchId = 0;
			BOOLEAN					bDuplicatedEntry = FALSE;

			PP2PSVC_SR_LIST_ENTRY 	pSREntry = NULL;
			PP2PSVC_SR_LIST_ENTRY 	pSREntryInList = NULL;

			// Match transaction id, this SD rsp may be the one for other seek req
			pSeekSearchIdObj = P2PSvc_GetParam(&pSeekInfoEntry->objList, P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, 0);
			searchId = *(pu1Byte)pSeekSearchIdObj->Value;
			if(transactionId != searchId)
			{
				continue;
			}

			if(!P2PSvc_ToProcessSDRsp(pP2PSvcInfo, searchId, devAddr))
			{
				break;
			}

			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_MakeSearchResultDataFromSDRsp(pP2PSvcInfo, searchId, devAddr, pSvcRspTlv, &pSREntry)))
			{
				break;
			}

			// Update search result
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_UpdateSearchResult(pP2PSvcInfo, searchId, devAddr, pSREntry, &bDuplicatedEntry, &pSREntryInList)))
			{
				break;
			}

			pSREntryInList->sdRspRxTime = PlatformGetCurrentTime();

			// Check if indicat is needed
			if(bDuplicatedEntry)
			{// already have same entry in the list => free this one
				P2PSvc_FreeMem(pSREntry, P2PSVC_SR_LIST_LEN(pSREntry));
			}
			else
			{
				pSREntryInList->bDirty = TRUE;
			}

			// Check if the seek is completed
			if(!P2PSvc_SeekReqIsBcst(&pSeekInfoEntry->objList))
			{// search for specific dev => done here
				PRT_LIST_ENTRY 			pForeEntry = RTForeEntryList(pEntry);
				RTRemoveEntryListWithCnt(pEntry, &pP2PSvcInfo->seekReqListCnt);
				P2PSvc_FreeMem(pEntry, P2PSVC_REQ_INFO_LIST_LEN(pSeekInfoEntry));
				pEntry = pForeEntry;	
			}
			else
			{// search for all dev addr
				continue; // continue searching
			}

		}

	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	if(RT_STATUS_SUCCESS == rtStatus)
	{
		*pbNeedFurtherProcess = FALSE;
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);
	
	return rtStatus;
}

//-----------------------------------------------------------------------------
// Process PD event
//-----------------------------------------------------------------------------

//
// Description:
//		Process PD req complete event.
//
RT_STATUS
P2PSvc_OnPDRsp(
	IN  PVOID						pvP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  u2Byte						reqConfigMethod,
	IN  u2Byte						rspConfigMethod,
	IN  POCTET_STRING				posP2PAttrs
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(devAddr);
	P2PSVC_CHECK_NULL(posP2PAttrs);

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PP2PSVC_PD_ENTRY 			pPDEntry = NULL;

		if(P2PSvc_IsLegacyPD(posP2PAttrs))
		{
			rtStatus = RT_STATUS_SUCCESS;
			break;
		}

		if(!P2PSvc_PDSessionInProgress(pP2PSvcInfo, &pPDEntry))
		{
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		if(pPDEntry->bInitor)
		{// initor, check PD rsp status, if 1, expect follow on PD req from peer
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_InitorOnPDRsp(pP2PSvcInfo, devAddr, pPDEntry, reqConfigMethod, rspConfigMethod, posP2PAttrs)))
			{
				break;
			}
		}
		else
		{// rspdor, this is a follow on PD rsp
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_RspdorOnFOPDRsp(pP2PSvcInfo, devAddr, pPDEntry, reqConfigMethod, rspConfigMethod, posP2PAttrs)))
			{
				break;
			}
		}
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);
	
	return rtStatus;
}

//
// Description:
//		This function handles PD req. 
//		The P2P module is expected NOT to send PD rsp if this function returns error.
//
RT_STATUS
P2PSvc_OnPDReq(
	IN  PVOID						pvP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  u2Byte						configMethod,
	IN  POCTET_STRING				posP2PAttrs,
	OUT PBOOLEAN					pbToSendPDRsp
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(devAddr);
	P2PSVC_CHECK_NULL(posP2PAttrs);
	P2PSVC_CHECK_NULL(pbToSendPDRsp);

	*pbToSendPDRsp = TRUE;

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PP2PSVC_PD_ENTRY 			pPDEntry = NULL;

		if(P2PSvc_IsLegacyPD(posP2PAttrs))
		{
			rtStatus = RT_STATUS_SUCCESS;
			*pbToSendPDRsp = TRUE;
			break;
		}

		if(P2PSvc_PDSessionInProgress(pP2PSvcInfo, &pPDEntry))
		{
			if(pPDEntry->bInitor)
			{// initor, this shall be a follow on PD req
				if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_InitorOnFOPDReq(pP2PSvcInfo, devAddr, configMethod, pPDEntry, posP2PAttrs)))
				{
					break;
				}
			}
			else
			{// rspdor, currently involved in another session
				// Will attach status attr with code 2 when constructing PD rsp
			}	
		}
		else
		{// no session, we are the rspdor, create a PD session
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AddPDRspdorData(pP2PSvcInfo, devAddr, configMethod, posP2PAttrs)))
			{
				break;
			}
		}
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	if(RT_STATUS_SUCCESS != rtStatus)
	{
		*pbToSendPDRsp = FALSE;
	}

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);
	
	return rtStatus;
}



RT_STATUS
P2PSvc_OnPDRspSent(
	IN  PVOID						pvP2PSvcInfo,
	IN  BOOLEAN						bSendOk
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PP2PSVC_PD_ENTRY			pPDEntry = NULL;

		if(!P2PSvc_PDSessionInProgress(pP2PSvcInfo, &pPDEntry))
		{
			break;
		}
		
		if(bSendOk)
		{
			if(pPDEntry->bInitor)
			{// initor, FOPD rsp sent
				P2PSvc_PDSessionSucceeded(pP2PSvcInfo, pPDEntry);
			}
			else
			{// rspdor, PD rsp sent
				if(!pPDEntry->bDeferred)
				{
					P2PSvc_PDSessionSucceeded(pP2PSvcInfo, pPDEntry);
				}
			}
		}

		break;
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);
	
	return rtStatus;
}

RT_STATUS
P2PSvc_MakePDReqIE(
	IN  PVOID						pvP2PSvcInfo,
	OUT FRAME_BUF					*pBuf
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(pBuf);

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PP2PSVC_PD_ENTRY			pPDEntry = NULL;

		if(!P2PSvc_PDSessionInProgress(pP2PSvcInfo, &pPDEntry))
		{
			break;
		}

		if(pPDEntry->bInitor)
		{// make PD req IE
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_MakeInitorPDReqIE(pP2PSvcInfo, pPDEntry, pBuf)))
			{
				break;
			}
		}
		else
		{// make follow on PD req IE
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_MakeRspdorFOPDReqIE(pP2PSvcInfo, pPDEntry, pBuf)))
			{
				break;
			}
		}
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;

}

RT_STATUS
P2PSvc_MakePDRspIE(
	IN  PVOID						pvP2PSvcInfo,
	IN  POCTET_STRING 				posP2PAttrs,
	OUT FRAME_BUF					*pBuf
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(pBuf);

	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_CHECK_NULL(posP2PAttrs);

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PP2PSVC_PD_ENTRY			pPDEntry = NULL;

		if(!P2PSvc_PDSessionInProgress(pP2PSvcInfo, &pPDEntry))
		{
			break;
		}

		if(pPDEntry->bInitor)
		{// initor, recv follow on PD req, make follow on PD rsp IE
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_MakeInitorFOPDRspIE(pP2PSvcInfo, pPDEntry, posP2PAttrs, pBuf)))
			{
				break;
			}
		}
		else
		{// rspdor, attach status attr with code 1 if !auto-accept
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_MakeRspdorPDRspIE(pP2PSvcInfo, pPDEntry, posP2PAttrs, pBuf)))
			{
				break;
			}
		}

		// If the session is completed, it will be freed when PD rsp sent.

	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//
// Description:
//		This function handles the case when the PD initor fails to send PD req.
//		
//
RT_STATUS
P2PSvc_OnSendPDReqFailure(
	IN  PVOID						pvP2PSvcInfo,
	IN  pu1Byte						devAddr,
	IN  u1Byte						p2pStatus
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	P2PSVC_CHECK_NULL(devAddr);
	
	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_INVALID_STATE;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PP2PSVC_PD_ENTRY 			pPDEntry = NULL;
		OCTET_STRING				osStatus = {NULL, 0};
		P2P_STATUS_CODE				pdStatus = 0;

		if(!P2PSvc_PDSessionInProgress(pP2PSvcInfo, &pPDEntry))
		{
			rtStatus = RT_STATUS_INVALID_STATE;
			break;
		}

		if(pPDEntry->bInitor)
		{// failed to send PD req
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_InitorOnSendPDReqFailure(pP2PSvcInfo, pPDEntry, p2pStatus)))
			{
				break;
			}
		}
		else
		{// failed to send FOPD req
			if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_RspdorOnSendFOPDReqFailure(pP2PSvcInfo, pPDEntry, p2pStatus)))
			{
				break;
			}
		}
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);
	
	return rtStatus;

}

RT_STATUS
P2PSvc_OnWatchdog(
	IN  PVOID						pvP2PSvcInfo
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;
	
	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	
	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		// Timeout out dated PD session
		P2PSvc_PDSessionAgeFunc(pP2PSvcInfo);

	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);
	
	return rtStatus;
}

RT_STATUS
P2PSvc_OnDisconnect(
	IN  PVOID						pvP2PSvcInfo
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = (PP2PSVC_INFO)pvP2PSvcInfo;

	P2PSVC_CHECK_NULL(pvP2PSvcInfo);
	
	if(!P2PSVC_ENABLED(pP2PSvcInfo)) return RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		P2PSvc_Free_PDSessionList(&pP2PSvcInfo->pdSessionList, &pP2PSvcInfo->pdSessionListCnt);
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);
	
	return rtStatus;
}


//-----------------------------------------------------------------------------
// Override default P2P behavior
//-----------------------------------------------------------------------------

RT_STATUS
P2PSvc_DetermineGOSSID(
	IN  PADAPTER					pAdapter,
	OUT pu1Byte 					ssidBuf,
	OUT pu1Byte 					pSsidLen
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PADAPTER						pDefAdapter = GetDefaultAdapter(pAdapter);
	PP2PSVC_INFO					pDefP2PSVcInfo = P2PSVC_GET_INFO(pDefAdapter);

	P2PSVC_CHECK_NULL(pAdapter);
	P2PSVC_CHECK_NULL(ssidBuf);
	P2PSVC_CHECK_NULL(pSsidLen);

	*pSsidLen = 0;

	if(!P2PSVC_ENABLED(pDefP2PSVcInfo)) return RT_STATUS_INVALID_STATE;

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		P2PSvc_DeterminePDSessionGOSsid(pDefP2PSVcInfo, ssidBuf, pSsidLen);

	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

#endif // #if (P2PSVC_SUPPORT == 1)
#endif // #if (P2P_SUPPORT == 1)
