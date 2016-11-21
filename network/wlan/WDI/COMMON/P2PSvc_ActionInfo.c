#include "Mp_Precomp.h"

#include "P2PSvc_Internal.h"

#if (P2P_SUPPORT == 1)

#if (P2PSVC_SUPPORT == 1)

//-----------------------------------------------------------------------------
// Local
//-----------------------------------------------------------------------------

// 
// Description:
// 	Add the req info (obj hdr + obj list) to the info list.
//		Assume the req info has been validated by P2PSvc_ValidateReqInfo
//
RT_STATUS 
p2psvc_AddReqInfoToList(
	IN  PADAPTER 					pAdapter,
	IN  PP2PSVC_INFO				pP2PSvcInfo,
	IN  PRT_LIST_ENTRY				pListHead,
	IN  pu4Byte						pListCnt,
	IN  PRT_OBJECT_HEADER 			pObjHdr
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_OBJ_LIST				pObjList = NULL;
	
	P2PSVC_FUNC_IN(DBG_LOUD);

	if(!pP2PSvcInfo->bEnabled)
	{
		RT_TRACE_F(COMP_P2P, DBG_WARNING, ("[WARNING] P2PSvc not enabled!"));
		P2PSVC_FUNC_OUT(DBG_LOUD, RT_STATUS_INVALID_STATE);
		
		return RT_STATUS_INVALID_STATE;
	}

	do
	{
		u4Byte						reqBufSize = 0;
		u4Byte						objListLen = 0;
		PP2PSVC_REQ_INFO_ENTRY		pReqInfoEntry = NULL;

		pObjList = (PP2PSVC_OBJ_LIST)(pObjHdr->Value);
		objListLen = P2PSVC_OBJ_LIST_LEN(pObjList);
		reqBufSize = FIELD_OFFSET(P2PSVC_REQ_INFO_ENTRY, objList) + objListLen;

		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AllocMem(pAdapter, &pReqInfoEntry, reqBufSize)))
		{
			RT_TRACE_F(COMP_OID_SET | COMP_WFD, DBG_WARNING, ("[WARNING] Failed to allocate memory for req info\n"));
			rtStatus = RT_STATUS_RESOURCE;
			break;
		}

		// Construct req info entry
		pReqInfoEntry->P2PSvcReqId = pObjHdr->Id;
		PlatformMoveMemory(&pReqInfoEntry->objList, pObjList, objListLen);

		// Add to the list
		PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);
		RTInsertTailListWithCnt(pListHead, &pReqInfoEntry->List, pListCnt);
		PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

		RT_PRINT_DATA(COMP_P2P, DBG_LOUD, "p2psvc_AddReqInfoToList(): add req info:\n", &pReqInfoEntry->objList, objListLen);
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

//-----------------------------------------------------------------------------
// Exported
//-----------------------------------------------------------------------------
BOOLEAN
P2PSvc_Free_AdvSvcList(
	IN  PRT_LIST_ENTRY				pListHead,
	IN  pu4Byte						pListCnt,
	IN  u4Byte						advId
	)
{
	PRT_LIST_ENTRY					pEntry = NULL;
	BOOLEAN							bFreeAll = (0 == advId) ? (TRUE) : (FALSE);
	BOOLEAN							bFreed = FALSE;
	
	for(pEntry = RTGetHeadList(pListHead); pEntry != pListHead; pEntry = RTNextEntryList(pEntry))
	{
		PP2PSVC_REQ_INFO_ENTRY 		pInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;
		PP2PSVC_OBJ_LIST 			pAdvSvcObjList = &pInfoEntry->objList;
		PRT_OBJECT_HEADER			pAdvIdObj = NULL;
		u4Byte						curAdvId = 0;
		BOOLEAN						bFree = FALSE;

		P2PSVC_CHECK_NULL(pEntry);
		if(!pEntry) break;

		pAdvIdObj = P2PSvc_GetParam(pAdvSvcObjList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
		curAdvId = *(pu4Byte)pAdvIdObj->Value;

		// Assume that cancel advertising advId 0 (ASP) is to cancel ALL services

		if(bFreeAll)
		{
			bFree = TRUE;
		}
		else
		{
			if(advId == curAdvId) bFree = TRUE;
		}

		if(bFree)
		{
			PRT_LIST_ENTRY 			pForeEntry = RTForeEntryList(pEntry);
			
			RTRemoveEntryListWithCnt(pEntry, pListCnt);
			P2PSvc_FreeMem(pEntry, FIELD_OFFSET(P2PSVC_REQ_INFO_ENTRY, objList) + P2PSVC_OBJ_LIST_LEN(pAdvSvcObjList));

			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("svc with id: %u canceled, pEntry is: %p\n", curAdvId, pEntry));

			pEntry = pForeEntry;

			bFreed = TRUE;
		}

		if(!bFreeAll && bFree) break;
	}

	return bFreed;
}

BOOLEAN
P2PSvc_Free_SeekReqList(
	IN  PRT_LIST_ENTRY				pListHead,
	IN  pu4Byte						pListCnt,
	IN  u1Byte						seekId
	)
{
	PRT_LIST_ENTRY					pEntry = NULL;
	BOOLEAN							bFreeAll = (0 == seekId) ? (TRUE) : (FALSE);
	BOOLEAN							bFreed = FALSE;
	
	for(pEntry = RTGetHeadList(pListHead); pEntry != pListHead; pEntry = RTNextEntryList(pEntry))
	{
		PP2PSVC_REQ_INFO_ENTRY 		pInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;
		PP2PSVC_OBJ_LIST 			pSeekObjList = &pInfoEntry->objList;
		PRT_OBJECT_HEADER			pSeekIdObj = NULL;
		u4Byte						curSeekId = 0;
		BOOLEAN						bFree = FALSE;

		P2PSVC_CHECK_NULL(pEntry);
		if(!pEntry) break;

		pSeekIdObj = P2PSvc_GetParam(pSeekObjList, P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, 0);
		curSeekId = *(pu1Byte)pSeekIdObj->Value;

		// Assume that cancel searchId 0 (ASP) is to cancel ALL seek requests

		//RT_TRACE_F(COMP_P2P, DBG_LOUD, ("cur search ID: %u search ID to cancel: %u\n", curSeekId, seekId));

		if(bFreeAll)
		{
			bFree = TRUE;
		}
		else
		{
			if(seekId == curSeekId) bFree = TRUE;
		}

		if(bFree)
		{
			PRT_LIST_ENTRY 			pForeEntry = RTForeEntryList(pEntry);

			RTRemoveEntryListWithCnt(pEntry, pListCnt);
			P2PSvc_FreeMem(pEntry, FIELD_OFFSET(P2PSVC_REQ_INFO_ENTRY, objList) + P2PSVC_OBJ_LIST_LEN(&pInfoEntry->objList));

			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("seek req of ID: %u canceled, pEntry is: %p\n", curSeekId, pEntry));

			pEntry = pForeEntry;

			bFreed = TRUE;
		}

		if(!bFreeAll && bFree) break;
	}

	return bFreed;
}

RT_STATUS 
P2PSvc_Set_AdvSvc(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PRT_OBJECT_HEADER			pObjHdr = (PRT_OBJECT_HEADER)(infoBuf);

		if(RT_STATUS_SUCCESS != (rtStatus = p2psvc_AddReqInfoToList(pAdapter, pP2PSvcInfo, &pP2PSvcInfo->advSvcList, &pP2PSvcInfo->advSvcListCnt, pObjHdr)))
		{
			break;
		}
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS 
P2PSvc_Set_CancelAdvSvc(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PRT_OBJECT_HEADER			pObjHdr = (PRT_OBJECT_HEADER)(infoBuf);
		PP2PSVC_OBJ_LIST 			pObjList = (PP2PSVC_OBJ_LIST)(pObjHdr->Value);
		
		PRT_OBJECT_HEADER 			pAdvIdObj = NULL;
		u4Byte						advIdToCancel = 0;

		pAdvIdObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
		advIdToCancel = *(pu4Byte)pAdvIdObj->Value;

		if(!P2PSvc_Free_AdvSvcList(&pP2PSvcInfo->advSvcList, &pP2PSvcInfo->advSvcListCnt, advIdToCancel))
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS 
P2PSvc_Set_SvcStatus(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PRT_OBJECT_HEADER			pObjHdr = (PRT_OBJECT_HEADER)(infoBuf);
		PP2PSVC_OBJ_LIST 			pObjList = (PP2PSVC_OBJ_LIST)(pObjHdr->Value);

		PRT_OBJECT_HEADER 			pAdvIdObj = NULL;
		PRT_OBJECT_HEADER 			pSvcStatusObj = NULL;

		u4Byte						advIdToSet = 0;
		u1Byte						svcStatusToSet = 0;
		
		PRT_LIST_ENTRY				pEntry = NULL;

		// Get adv-id and svc-status to set
		pAdvIdObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
		advIdToSet = *(pu4Byte)pAdvIdObj->Value;

		pSvcStatusObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS, 0);
		svcStatusToSet = *(pu1Byte)pSvcStatusObj->Value;
	
		for(pEntry = RTGetHeadList(&pP2PSvcInfo->advSvcList); pEntry != &pP2PSvcInfo->advSvcList; pEntry = RTNextEntryList(pEntry))
		{
			PP2PSVC_REQ_INFO_ENTRY 	pInfoEntry = (PP2PSVC_REQ_INFO_ENTRY)pEntry;
			PP2PSVC_OBJ_LIST 		pAdvSvcObjList = &pInfoEntry->objList;
			u4Byte					curAdvId = 0;

			// Get current adv svc's adv id for matching
			pAdvIdObj = P2PSvc_GetParam(pAdvSvcObjList, P2PSVC_OBJ_HDR_ID_DATA_ADV_ID, 0);
			curAdvId = *(pu4Byte)pAdvIdObj->Value;

			if(curAdvId == advIdToSet)
			{
				pu1Byte					pCurSvcStatus = NULL;

				pSvcStatusObj = P2PSvc_GetParam(pAdvSvcObjList, P2PSVC_OBJ_HDR_ID_DATA_SVC_STATUS, 0);
				pCurSvcStatus = (pu1Byte)pSvcStatusObj->Value;
				*pCurSvcStatus = svcStatusToSet;

				break;
			}
		}

		if(pEntry == &pP2PSvcInfo->advSvcList)
		{
			rtStatus = RT_STATUS_INVALID_PARAMETER;
			break;
		}
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS 
P2PSvc_Set_Seek(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PRT_OBJECT_HEADER			pObjHdr = (PRT_OBJECT_HEADER)(infoBuf);

		if(RT_STATUS_SUCCESS != (rtStatus = p2psvc_AddReqInfoToList(pAdapter, pP2PSvcInfo, &pP2PSvcInfo->seekReqList, &pP2PSvcInfo->seekReqListCnt, pObjHdr)))
		{
			break;
		}
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;

}

RT_STATUS 
P2PSvc_Set_CancelSeek(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PRT_OBJECT_HEADER			pObjHdr = (PRT_OBJECT_HEADER)(infoBuf);
		PP2PSVC_OBJ_LIST 			pObjList = (PP2PSVC_OBJ_LIST)(pObjHdr->Value);
		
		PRT_OBJECT_HEADER 			pSearchIdObj = NULL;
		u1Byte						searchIdToCancel = 0;

		pSearchIdObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_SEARCH_ID, 0);
		searchIdToCancel = *(pu1Byte)pSearchIdObj->Value;

		if(!P2PSvc_Free_SeekReqList(&pP2PSvcInfo->seekReqList, &pP2PSvcInfo->seekReqListCnt, searchIdToCancel))
		{
			rtStatus = RT_STATUS_INVALID_DATA;
			break;
		}
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;

}

RT_STATUS 
P2PSvc_Set_ConnCap(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		PP2PSVC_INFO				pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);
		PRT_OBJECT_HEADER			pObjHdr = NULL;
		PP2PSVC_OBJ_LIST			pObjList = NULL;
		PRT_OBJECT_HEADER			pConnCapObj = NULL;

		pObjHdr = (PRT_OBJECT_HEADER)infoBuf;
		pObjList = (PP2PSVC_OBJ_LIST)(pObjHdr->Value);

		pConnCapObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_CONN_CAP, 0);

		PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);
		{
			pP2PSvcInfo->connCap = *(pu1Byte)pConnCapObj->Value;
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("conn cap set to: %u\n", pP2PSvcInfo->connCap));
		}
		PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS 
P2PSvc_Set_PDReq(
	IN  PADAPTER 					pAdapter, 
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;

	P2PSVC_FUNC_IN(DBG_LOUD);

	do
	{
		if(RT_STATUS_SUCCESS != (rtStatus = P2PSvc_AddPDInitorData(pAdapter, infoBuf, inBufLen, outBufLen)))
		{
			break;
		}		
	}while(FALSE);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

RT_STATUS 
P2PSvc_Set_RspdorFOPDReq(
	IN  PADAPTER 					pAdapter,
	IN  PVOID 						infoBuf,
	IN  u4Byte						inBufLen,
	IN  u4Byte						outBufLen,
	OUT pu4Byte						pBytesWritten,
	OUT pu4Byte						pBytesRead,
	OUT pu4Byte						pBytesNeeded
	)
{
	RT_STATUS						rtStatus = RT_STATUS_SUCCESS;
	PP2PSVC_INFO					pP2PSvcInfo = P2PSVC_GET_INFO(pAdapter);

	P2PSVC_FUNC_IN(DBG_LOUD);

	PLATFORM_ACQUIRE_RT_SPINLOCK(pP2PSvcInfo->lock);

	do
	{
		PRT_OBJECT_HEADER 			pObjHdr = NULL;
		PP2PSVC_OBJ_LIST			pObjList = NULL;
		PRT_OBJECT_HEADER 			pObj = NULL;
		BOOLEAN						bUserAccepted = FALSE;
		PP2PSVC_PD_ENTRY			pPDEntry = NULL;

		pObjHdr = (PRT_OBJECT_HEADER)infoBuf;
		pObjList = (PP2PSVC_OBJ_LIST)(pObjHdr->Value);

		pObj = P2PSvc_GetParam(pObjList, P2PSVC_OBJ_HDR_ID_DATA_USER_ACCEPTED, 0);
		bUserAccepted = *((PBOOLEAN)pObj->Value);

		if(P2PSvc_PDSessionInProgress(pP2PSvcInfo, &pPDEntry))
		{
			P2PSvc_RspdorSetFOPDReq(pP2PSvcInfo, pPDEntry, bUserAccepted);
		}
		else
		{
			rtStatus = RT_STATUS_INVALID_STATE;
			RT_TRACE_F(COMP_P2P, DBG_LOUD, ("No PD session in progress\n"));
			break;
		}
	}while(FALSE);

	PLATFORM_RELEASE_RT_SPINLOCK(pP2PSvcInfo->lock);

	P2PSVC_FUNC_OUT(DBG_LOUD, rtStatus);

	return rtStatus;
}

#endif // #if (P2PSVC_SUPPORT == 1)
#endif // #if (P2P_SUPPORT == 1)
